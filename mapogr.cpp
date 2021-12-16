/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGR Link
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *           Frank Warmerdam (warmerdam@pobox.com)
 *
 **********************************************************************
 * Copyright (c) 2000-2005, Daniel Morissette, DM Solutions Group Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include <assert.h>
#include "mapserver.h"
#include "mapproject.h"
#include "mapthread.h"
#include "mapows.h"
#include <string>
#include <vector>

#include "gdal.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_srs_api.h"

#include <memory>

#define ACQUIRE_OGR_LOCK       msAcquireLock( TLOCK_OGR )
#define RELEASE_OGR_LOCK       msReleaseLock( TLOCK_OGR )

// GDAL 1.x API
#include "ogr_api.h"

typedef struct ms_ogr_file_info_t {
  char        *pszFname;
  char        *pszLayerDef;
  int         nLayerIndex;
  OGRDataSourceH hDS;
  OGRLayerH   hLayer;
  OGRFeatureH hLastFeature;

  int         nTileId;                  /* applies on the tiles themselves. */
  projectionObj sTileProj;              /* applies on the tiles themselves. */

  struct ms_ogr_file_info_t *poCurTile; /* exists on tile index, -> tiles */
  bool        rect_is_defined;
  rectObj     rect;                     /* set by TranslateMsExpression (possibly) and WhichShapes */

  int         last_record_index_read;

  const char* dialect; /* NULL, Spatialite or PostgreSQL */
  char *pszSelect;
  char *pszSpatialFilterTableName;
  char *pszSpatialFilterGeometryColumn;
  char *pszMainTableName;
  char *pszRowId;
  int   bIsOKForSQLCompose;
  bool  bHasSpatialIndex; // used only for spatialite for now
  char* pszTablePrefix; // prefix to qualify field names. used only for spatialite & gpkg for now when a join is done for spatial filtering.

  int   bPaging;

  char* pszWHERE;

} msOGRFileInfo;

static int msOGRLayerIsOpen(layerObj *layer);
static int msOGRLayerInitItemInfo(layerObj *layer);
static int msOGRLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c,
                                  shapeObj* shape);
static void msOGRCloseConnection( void *conn_handle );

/* ==================================================================
 * Geometry conversion functions
 * ================================================================== */

/**********************************************************************
 *                     ogrPointsAddPoint()
 *
 * NOTE: This function assumes the line->point array already has been
 * allocated large enough for the point to be added, but that numpoints
 * does not include this new point.
 **********************************************************************/
static void ogrPointsAddPoint(lineObj *line, double dX, double dY, double dZ, int lineindex, rectObj *bounds)
{
  /* Keep track of shape bounds */
  if (line->numpoints == 0 && lineindex == 0) {
    bounds->minx = bounds->maxx = dX;
    bounds->miny = bounds->maxy = dY;
  } else {
    if (dX < bounds->minx)  bounds->minx = dX;
    if (dX > bounds->maxx)  bounds->maxx = dX;
    if (dY < bounds->miny)  bounds->miny = dY;
    if (dY > bounds->maxy)  bounds->maxy = dY;
  }

  line->point[line->numpoints].x = dX;
  line->point[line->numpoints].y = dY;
  line->point[line->numpoints].z = dZ;
  line->point[line->numpoints].m = 0.0;
  line->numpoints++;
}

/**********************************************************************
 *                     ogrGeomPoints()
 **********************************************************************/
static int ogrGeomPoints(OGRGeometryH hGeom, shapeObj *outshp)
{
  int   i;
  int   numpoints;

  if (hGeom == NULL)
    return 0;

  OGRwkbGeometryType eGType =  wkbFlatten( OGR_G_GetGeometryType( hGeom ) );

  /* -------------------------------------------------------------------- */
  /*      Container types result in recursive invocation on each          */
  /*      subobject to add a set of points to the current list.           */
  /* -------------------------------------------------------------------- */
  switch( eGType ) {
    case wkbGeometryCollection:
    case wkbMultiLineString:
    case wkbMultiPolygon:
    case wkbPolygon: {
      /* Treat it as GeometryCollection */
      for (int iGeom=0; iGeom < OGR_G_GetGeometryCount( hGeom ); iGeom++ ) {
        if( ogrGeomPoints( OGR_G_GetGeometryRef( hGeom, iGeom ),
                           outshp ) == -1 )
          return -1;
      }

      return 0;
    }
    break;

    case wkbPoint:
    case wkbMultiPoint:
    case wkbLineString:
    case wkbLinearRing:
      /* We will handle these directly */
      break;

    default:
      /* There shouldn't be any more cases should there? */
      msSetError(MS_OGRERR,
                 "OGRGeometry type `%s' not supported yet.",
                 "ogrGeomPoints()",
                 OGR_G_GetGeometryName( hGeom ) );
      return(-1);
  }


  /* ------------------------------------------------------------------
   * Count total number of points
   * ------------------------------------------------------------------ */
  if ( eGType == wkbPoint ) {
    numpoints = 1;
  } else if ( eGType == wkbLineString
              ||  eGType == wkbLinearRing ) {
    numpoints = OGR_G_GetPointCount( hGeom );
  } else if ( eGType == wkbMultiPoint ) {
    numpoints = OGR_G_GetGeometryCount( hGeom );
  } else {
    msSetError(MS_OGRERR,
               "OGRGeometry type `%s' not supported yet.",
               "ogrGeomPoints()",
               OGR_G_GetGeometryName( hGeom ) );
    return(-1);
  }

  /* ------------------------------------------------------------------
   * Do we need to allocate a line object to contain all our points?
   * ------------------------------------------------------------------ */
  if( outshp->numlines == 0 ) {
    lineObj newline;

    newline.numpoints = 0;
    newline.point = NULL;
    msAddLine(outshp, &newline);
  }

  /* ------------------------------------------------------------------
   * Extend the point array for the new of points to add from the
   * current geometry.
   * ------------------------------------------------------------------ */
  lineObj *line = outshp->line + outshp->numlines-1;

  if( line->point == NULL )
    line->point = (pointObj *) malloc(sizeof(pointObj) * numpoints);
  else
    line->point = (pointObj *)
                  realloc(line->point,sizeof(pointObj) * (numpoints+line->numpoints));

  if(!line->point) {
    msSetError(MS_MEMERR, "Unable to allocate temporary point cache.",
               "ogrGeomPoints()");
    return(-1);
  }

  /* ------------------------------------------------------------------
   * alloc buffer and filter/transform points
   * ------------------------------------------------------------------ */
  if( eGType == wkbPoint ) {
    ogrPointsAddPoint(line, OGR_G_GetX(hGeom, 0), OGR_G_GetY(hGeom, 0), OGR_G_GetZ(hGeom, 0),
                      outshp->numlines-1, &(outshp->bounds));
  } else if( eGType == wkbLineString
             || eGType == wkbLinearRing ) {
    for(i=0; i<numpoints; i++)
      ogrPointsAddPoint(line, OGR_G_GetX(hGeom, i), OGR_G_GetY(hGeom, i), OGR_G_GetZ(hGeom, i),
                        outshp->numlines-1, &(outshp->bounds));
  } else if( eGType == wkbMultiPoint ) {
    for(i=0; i<numpoints; i++) {
      OGRGeometryH hPoint = OGR_G_GetGeometryRef( hGeom, i );
      ogrPointsAddPoint(line, OGR_G_GetX(hPoint, 0), OGR_G_GetY(hPoint, 0), OGR_G_GetZ(hPoint, 0),
                        outshp->numlines-1, &(outshp->bounds));
    }
  }

  outshp->type = MS_SHAPE_POINT;

  return(0);
}


/**********************************************************************
 *                     ogrGeomLine()
 *
 * Recursively convert any OGRGeometry into a shapeObj.  Each part becomes
 * a line in the overall shapeObj.
 **********************************************************************/
static int ogrGeomLine(OGRGeometryH hGeom, shapeObj *outshp,
                       int bCloseRings)
{
  if (hGeom == NULL)
    return 0;

  /* ------------------------------------------------------------------
   * Use recursive calls for complex geometries
   * ------------------------------------------------------------------ */
  OGRwkbGeometryType eGType =  wkbFlatten( OGR_G_GetGeometryType( hGeom ) );


  if ( eGType == wkbPolygon
       || eGType == wkbGeometryCollection
       || eGType == wkbMultiLineString
       || eGType == wkbMultiPolygon ) {
    if (eGType == wkbPolygon && outshp->type == MS_SHAPE_NULL)
      outshp->type = MS_SHAPE_POLYGON;

    /* Treat it as GeometryCollection */
    for (int iGeom=0; iGeom < OGR_G_GetGeometryCount( hGeom ); iGeom++ ) {
      if( ogrGeomLine( OGR_G_GetGeometryRef( hGeom, iGeom ),
                       outshp, bCloseRings ) == -1 )
        return -1;
    }
  }
  /* ------------------------------------------------------------------
   * OGRPoint and OGRMultiPoint
   * ------------------------------------------------------------------ */
  else if ( eGType == wkbPoint || eGType == wkbMultiPoint ) {
    /* Hummmm a point when we're drawing lines/polygons... just drop it! */
  }
  /* ------------------------------------------------------------------
   * OGRLinearRing/OGRLineString ... both are of type wkbLineString
   * ------------------------------------------------------------------ */
  else if ( eGType == wkbLineString ) {
    int       j, numpoints;
    lineObj   line= {0,NULL};
    double    dX, dY;

    if ((numpoints = OGR_G_GetPointCount( hGeom )) < 2)
      return 0;

    if (outshp->type == MS_SHAPE_NULL)
      outshp->type = MS_SHAPE_LINE;

    line.numpoints = 0;
    line.point = (pointObj *)malloc(sizeof(pointObj)*(numpoints+1));
    if(!line.point) {
      msSetError(MS_MEMERR, "Unable to allocate temporary point cache.",
                 "ogrGeomLine");
      return(-1);
    }

    OGR_G_GetPoints(hGeom,
                    &(line.point[0].x), sizeof(pointObj),
                    &(line.point[0].y), sizeof(pointObj),
                    &(line.point[0].z), sizeof(pointObj));

    for(j=0; j<numpoints; j++) {
      dX = line.point[j].x = OGR_G_GetX( hGeom, j);
      dY = line.point[j].y = OGR_G_GetY( hGeom, j);

      /* Keep track of shape bounds */
      if (j == 0 && outshp->numlines == 0) {
        outshp->bounds.minx = outshp->bounds.maxx = dX;
        outshp->bounds.miny = outshp->bounds.maxy = dY;
      } else {
        if (dX < outshp->bounds.minx)  outshp->bounds.minx = dX;
        if (dX > outshp->bounds.maxx)  outshp->bounds.maxx = dX;
        if (dY < outshp->bounds.miny)  outshp->bounds.miny = dY;
        if (dY > outshp->bounds.maxy)  outshp->bounds.maxy = dY;
      }

    }
    line.numpoints = numpoints;

    if (bCloseRings &&
        ( line.point[line.numpoints-1].x != line.point[0].x ||
          line.point[line.numpoints-1].y != line.point[0].y  ) ) {
      line.point[line.numpoints].x = line.point[0].x;
      line.point[line.numpoints].y = line.point[0].y;
      line.point[line.numpoints].z = line.point[0].z;
      line.numpoints++;
    }

    msAddLineDirectly(outshp, &line);
  } else {
    msSetError(MS_OGRERR,
               "OGRGeometry type `%s' not supported.",
               "ogrGeomLine()",
               OGR_G_GetGeometryName( hGeom ) );
    return(-1);
  }

  return(0);
}

/**********************************************************************
 *                     ogrGetLinearGeometry()
 *
 * Fetch geometry from OGR feature. If using GDAL 2.0 or later, the geometry
 * might be of curve type, so linearize it.
 **********************************************************************/
static OGRGeometryH ogrGetLinearGeometry(OGRFeatureH hFeature)
{
#if GDAL_VERSION_MAJOR >= 2
    /* Convert in place and reassign to the feature */
    OGRGeometryH hGeom = OGR_F_StealGeometry(hFeature);
    if( hGeom != NULL )
    {
        hGeom = OGR_G_ForceTo(hGeom, OGR_GT_GetLinear(OGR_G_GetGeometryType(hGeom)), NULL);
        OGR_F_SetGeometryDirectly(hFeature, hGeom);
    }
    return hGeom;
#else
    return OGR_F_GetGeometryRef( hFeature );
#endif
}

/**********************************************************************
 *                     ogrConvertGeometry()
 *
 * Convert OGR geometry into a shape object doing the best possible
 * job to match OGR Geometry type and layer type.
 *
 * If layer type is incompatible with geometry, then shape is returned with
 * shape->type = MS_SHAPE_NULL
 **********************************************************************/
static int ogrConvertGeometry(OGRGeometryH hGeom, shapeObj *outshp,
                              enum MS_LAYER_TYPE layertype)
{
  /* ------------------------------------------------------------------
   * Process geometry according to layer type
   * ------------------------------------------------------------------ */
  int nStatus = MS_SUCCESS;

  if (hGeom == NULL) {
    // Empty geometry... this is not an error... we'll just skip it
    return MS_SUCCESS;
  }

  switch(layertype) {
      /* ------------------------------------------------------------------
       *      POINT layer - Any geometry can be converted to point/multipoint
       * ------------------------------------------------------------------ */
    case MS_LAYER_POINT:
      if(ogrGeomPoints(hGeom, outshp) == -1) {
        nStatus = MS_FAILURE; // Error message already produced.
      }
      break;
      /* ------------------------------------------------------------------
       *      LINE layer
       * ------------------------------------------------------------------ */
    case MS_LAYER_LINE:
      if(ogrGeomLine(hGeom, outshp, MS_FALSE) == -1) {
        nStatus = MS_FAILURE; // Error message already produced.
      }
      if (outshp->type != MS_SHAPE_LINE && outshp->type != MS_SHAPE_POLYGON)
        outshp->type = MS_SHAPE_NULL;  // Incompatible type for this layer
      break;
      /* ------------------------------------------------------------------
       *      POLYGON layer
       * ------------------------------------------------------------------ */
    case MS_LAYER_POLYGON:
      if(ogrGeomLine(hGeom, outshp, MS_TRUE) == -1) {
        nStatus = MS_FAILURE; // Error message already produced.
      }
      if (outshp->type != MS_SHAPE_POLYGON)
        outshp->type = MS_SHAPE_NULL;  // Incompatible type for this layer
      break;
      /* ------------------------------------------------------------------
       *      Chart or Query layers - return real feature type
       * ------------------------------------------------------------------ */
    case MS_LAYER_CHART:
    case MS_LAYER_QUERY:
      switch( OGR_G_GetGeometryType( hGeom ) ) {
        case wkbPoint:
        case wkbPoint25D:
        case wkbMultiPoint:
        case wkbMultiPoint25D:
          if(ogrGeomPoints(hGeom, outshp) == -1) {
            nStatus = MS_FAILURE; // Error message already produced.
          }
          break;
        default:
          // Handle any non-point types as lines/polygons ... ogrGeomLine()
          // will decide the shape type
          if(ogrGeomLine(hGeom, outshp, MS_FALSE) == -1) {
            nStatus = MS_FAILURE; // Error message already produced.
          }
      }
      break;

    default:
      msSetError(MS_MISCERR, "Unknown or unsupported layer type.",
                 "msOGRLayerNextShape()");
      nStatus = MS_FAILURE;
  } /* switch layertype */

  return nStatus;
}

/**********************************************************************
 *                     msOGRGeometryToShape()
 *
 * Utility function to convert from OGR geometry to a mapserver shape
 * object.
 **********************************************************************/
int msOGRGeometryToShape(OGRGeometryH hGeometry, shapeObj *psShape,
                         OGRwkbGeometryType nType)
{
  if (hGeometry && psShape && nType > 0) {
    if (nType == wkbPoint || nType == wkbMultiPoint )
      return ogrConvertGeometry(hGeometry, psShape,  MS_LAYER_POINT);
    else if (nType == wkbLineString || nType == wkbMultiLineString)
      return ogrConvertGeometry(hGeometry, psShape,  MS_LAYER_LINE);
    else if (nType == wkbPolygon || nType == wkbMultiPolygon)
      return ogrConvertGeometry(hGeometry, psShape,  MS_LAYER_POLYGON);
    else
      return MS_FAILURE;
  } else
    return MS_FAILURE;
}


/* ==================================================================
 * Attributes handling functions
 * ================================================================== */

// Special field index codes for handling text string and angle coming from
// OGR style strings.

#define MSOGR_FID_INDEX            -99

#define MSOGR_LABELNUMITEMS        21
#define MSOGR_LABELFONTNAMENAME    "OGR:LabelFont"
#define MSOGR_LABELFONTNAMEINDEX   -100
#define MSOGR_LABELSIZENAME        "OGR:LabelSize"
#define MSOGR_LABELSIZEINDEX       -101
#define MSOGR_LABELTEXTNAME        "OGR:LabelText"
#define MSOGR_LABELTEXTINDEX       -102
#define MSOGR_LABELANGLENAME       "OGR:LabelAngle"
#define MSOGR_LABELANGLEINDEX      -103
#define MSOGR_LABELFCOLORNAME      "OGR:LabelFColor"
#define MSOGR_LABELFCOLORINDEX     -104
#define MSOGR_LABELBCOLORNAME      "OGR:LabelBColor"
#define MSOGR_LABELBCOLORINDEX     -105
#define MSOGR_LABELPLACEMENTNAME   "OGR:LabelPlacement"
#define MSOGR_LABELPLACEMENTINDEX  -106
#define MSOGR_LABELANCHORNAME      "OGR:LabelAnchor"
#define MSOGR_LABELANCHORINDEX     -107
#define MSOGR_LABELDXNAME          "OGR:LabelDx"
#define MSOGR_LABELDXINDEX         -108
#define MSOGR_LABELDYNAME          "OGR:LabelDy"
#define MSOGR_LABELDYINDEX         -109
#define MSOGR_LABELPERPNAME        "OGR:LabelPerp"
#define MSOGR_LABELPERPINDEX       -110
#define MSOGR_LABELBOLDNAME        "OGR:LabelBold"
#define MSOGR_LABELBOLDINDEX       -111
#define MSOGR_LABELITALICNAME      "OGR:LabelItalic"
#define MSOGR_LABELITALICINDEX     -112
#define MSOGR_LABELUNDERLINENAME   "OGR:LabelUnderline"
#define MSOGR_LABELUNDERLINEINDEX  -113
#define MSOGR_LABELPRIORITYNAME    "OGR:LabelPriority"
#define MSOGR_LABELPRIORITYINDEX   -114
#define MSOGR_LABELSTRIKEOUTNAME   "OGR:LabelStrikeout"
#define MSOGR_LABELSTRIKEOUTINDEX  -115
#define MSOGR_LABELSTRETCHNAME     "OGR:LabelStretch"
#define MSOGR_LABELSTRETCHINDEX    -116
#define MSOGR_LABELADJHORNAME      "OGR:LabelAdjHor"
#define MSOGR_LABELADJHORINDEX     -117
#define MSOGR_LABELADJVERTNAME     "OGR:LabelAdjVert"
#define MSOGR_LABELADJVERTINDEX    -118
#define MSOGR_LABELHCOLORNAME      "OGR:LabelHColor"
#define MSOGR_LABELHCOLORINDEX     -119
#define MSOGR_LABELOCOLORNAME      "OGR:LabelOColor"
#define MSOGR_LABELOCOLORINDEX     -120
// Special codes for the OGR style parameters
#define MSOGR_LABELPARAMNAME       "OGR:LabelParam"
#define MSOGR_LABELPARAMNAMELEN    14
#define MSOGR_LABELPARAMINDEX      -500
#define MSOGR_BRUSHPARAMNAME       "OGR:BrushParam"
#define MSOGR_BRUSHPARAMNAMELEN    14
#define MSOGR_BRUSHPARAMINDEX      -600
#define MSOGR_PENPARAMNAME         "OGR:PenParam"
#define MSOGR_PENPARAMNAMELEN      12
#define MSOGR_PENPARAMINDEX        -700
#define MSOGR_SYMBOLPARAMNAME      "OGR:SymbolParam"
#define MSOGR_SYMBOLPARAMNAMELEN   15
#define MSOGR_SYMBOLPARAMINDEX     -800

/**********************************************************************
 *                     msOGRGetValues()
 *
 * Load selected item (i.e. field) values into a char array
 *
 * Some special attribute names are used to return some OGRFeature params
 * like for instance stuff encoded in the OGRStyleString.
 * For now the following pseudo-attribute names are supported:
 *  "OGR:TextString"  OGRFeatureStyle's text string if present
 *  "OGR:TextAngle"   OGRFeatureStyle's text angle, or 0 if not set
 **********************************************************************/
static char **msOGRGetValues(layerObj *layer, OGRFeatureH hFeature)
{
  char **values;
  const char *pszValue = NULL;
  int i;

  if(layer->numitems == 0)
    return(NULL);

  if(!layer->iteminfo)  // Should not happen... but just in case!
    if (msOGRLayerInitItemInfo(layer) != MS_SUCCESS)
      return NULL;

  if((values = (char **)malloc(sizeof(char *)*layer->numitems)) == NULL) {
    msSetError(MS_MEMERR, NULL, "msOGRGetValues()");
    return(NULL);
  }

  OGRStyleMgrH  hStyleMgr = NULL;
  OGRStyleToolH hLabelStyle = NULL;
  OGRStyleToolH hPenStyle = NULL;
  OGRStyleToolH hBrushStyle = NULL;
  OGRStyleToolH hSymbolStyle = NULL;

  int *itemindexes = (int*)layer->iteminfo;

  int nYear;
  int nMonth;
  int nDay;
  int nHour;
  int nMinute;
  int nSecond;
  int nTZFlag;

  for(i=0; i<layer->numitems; i++) {
    if (itemindexes[i] >= 0) {
      // Extract regular attributes
      const char* pszValue = OGR_F_GetFieldAsString(hFeature, itemindexes[i]);
      if( pszValue[0] == 0 )
      {
          values[i] = msStrdup("");
      }
      else
      {
          OGRFieldDefnH hFieldDefnRef = OGR_F_GetFieldDefnRef(hFeature, itemindexes[i]);
          switch(OGR_Fld_GetType(hFieldDefnRef)) {
          case OFTTime:
              OGR_F_GetFieldAsDateTime(hFeature, itemindexes[i], &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond, &nTZFlag);
              switch(nTZFlag) {
              case 0: // Unknown time zone
              case 1: // Local time zone (not specified)
                values[i] = msStrdup(CPLSPrintf("%02d:%02d:%02d", nHour, nMinute, nSecond));
                break;
              case 100: // GMT
                values[i] = msStrdup(CPLSPrintf("%02d:%02d:%02dZ", nHour, nMinute, nSecond));
                break;
              default: // Offset (in quarter-hour units) from GMT
                const int TZOffset = std::abs(nTZFlag - 100) * 15;
                const int TZHour = TZOffset / 60;
                const int TZMinute = TZOffset % 60;
                const char TZSign = (nTZFlag > 100) ? '+' : '-';
                values[i] = msStrdup(CPLSPrintf("%02d:%02d:%02d%c%02d:%02d", nHour, nMinute,
                  nSecond, TZSign, TZHour, TZMinute));
              }
              break;
          case OFTDate:
              OGR_F_GetFieldAsDateTime(hFeature, itemindexes[i], &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond, &nTZFlag);
              values[i] = msStrdup(CPLSPrintf("%04d-%02d-%02d", nYear, nMonth, nDay));
              break;
          case OFTDateTime:
              OGR_F_GetFieldAsDateTime(hFeature, itemindexes[i], &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond, &nTZFlag);
              switch(nTZFlag) {
              case 0: // Unknown time zone
              case 1: // Local time zone (not specified)
                values[i] = msStrdup(CPLSPrintf("%04d-%02d-%02dT%02d:%02d:%02d", nYear, nMonth, nDay, nHour, nMinute, nSecond));
                break;
              case 100: // GMT
                values[i] = msStrdup(CPLSPrintf("%04d-%02d-%02dT%02d:%02d:%02dZ", nYear, nMonth, nDay, nHour, nMinute, nSecond));
                break;
              default: // Offset (in quarter-hour units) from GMT
                const int TZOffset = std::abs(nTZFlag - 100) * 15;
                const int TZHour = TZOffset / 60;
                const int TZMinute = TZOffset % 60;
                const char TZSign = (nTZFlag > 100) ? '+' : '-';
                values[i] = msStrdup(CPLSPrintf("%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d", nYear, nMonth, nDay, nHour, nMinute, 
                  nSecond, TZSign, TZHour, TZMinute));
              }
              break;
          default:
              values[i] = msStrdup(pszValue);
              break;
          }
      }
    } else if (itemindexes[i] == MSOGR_FID_INDEX ) {
      values[i] = msStrdup(CPLSPrintf(CPL_FRMT_GIB,
                                      (GIntBig) OGR_F_GetFID(hFeature)));
    } else {
      // Handle special OGR attributes coming from StyleString
      if (!hStyleMgr) {
        hStyleMgr = OGR_SM_Create(NULL);
        OGR_SM_InitFromFeature(hStyleMgr, hFeature);
        int numParts = OGR_SM_GetPartCount(hStyleMgr, NULL);
        for(int i=0; i<numParts; i++) {
          OGRStyleToolH hStylePart = OGR_SM_GetPart(hStyleMgr, i, NULL);
          if (hStylePart) {
            if (OGR_ST_GetType(hStylePart) == OGRSTCLabel && !hLabelStyle)
              hLabelStyle = hStylePart;
            else if (OGR_ST_GetType(hStylePart) == OGRSTCPen && !hPenStyle)
              hPenStyle = hStylePart;
            else if (OGR_ST_GetType(hStylePart) == OGRSTCBrush && !hBrushStyle)
              hBrushStyle = hStylePart;
            else if (OGR_ST_GetType(hStylePart) == OGRSTCSymbol && !hSymbolStyle)
              hSymbolStyle = hStylePart;
            else {
              OGR_ST_Destroy(hStylePart);
              hStylePart =  NULL;
            }
          }
          /* Setting up the size units according to msOGRLayerGetAutoStyle*/
          if (hStylePart && layer->map)
            OGR_ST_SetUnit(hStylePart, OGRSTUPixel, 
              layer->map->cellsize*layer->map->resolution/layer->map->defresolution*72.0*39.37);
        }
      }
      int bDefault;
      if (itemindexes[i] == MSOGR_LABELTEXTINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelTextString,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELTEXTNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELANGLEINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelAngle,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELANGLENAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELSIZEINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelSize,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELSIZENAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELFCOLORINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelFColor,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("#000000");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELFCOLORNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELFONTNAMEINDEX ) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelFontName,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("Arial");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELFONTNAMENAME " =       \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELBCOLORINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelBColor,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("#000000");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELBCOLORNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELPLACEMENTINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelPlacement,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELPLACEMENTNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELANCHORINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelAnchor,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELANCHORNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELDXINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelDx,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELDXNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELDYINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelDy,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELDYNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELPERPINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelPerp,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELPERPNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELBOLDINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelBold,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELBOLDNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELITALICINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelItalic,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELITALICNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELUNDERLINEINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelUnderline,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELUNDERLINENAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELPRIORITYINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelPriority,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELPRIORITYNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELSTRIKEOUTINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelStrikeout,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELSTRIKEOUTNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELSTRETCHINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelStretch,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("0");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELSTRETCHNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELADJHORINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelAdjHor,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELADJHORNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELADJVERTINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelAdjVert,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELADJVERTNAME " = \"%s\"\n", values[i]);
      } else if (itemindexes[i] == MSOGR_LABELHCOLORINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelHColor,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELHCOLORNAME " = \"%s\"\n", values[i]);
      }
      else if (itemindexes[i] == MSOGR_LABELOCOLORINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               OGRSTLabelOColor,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELOCOLORNAME " = \"%s\"\n", values[i]);
      }
      else if (itemindexes[i] >= MSOGR_LABELPARAMINDEX) {
        if (hLabelStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                               itemindexes[i] - MSOGR_LABELPARAMINDEX,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_LABELPARAMNAME " = \"%s\"\n", values[i]);
      }
      else if (itemindexes[i] >= MSOGR_BRUSHPARAMINDEX) {
        if (hBrushStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hBrushStyle,
                                               itemindexes[i] - MSOGR_BRUSHPARAMINDEX,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_BRUSHPARAMNAME " = \"%s\"\n", values[i]);
      }
      else if (itemindexes[i] >= MSOGR_PENPARAMINDEX) {
        if (hPenStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hPenStyle,
                                               itemindexes[i] - MSOGR_PENPARAMINDEX,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_PENPARAMNAME " = \"%s\"\n", values[i]);
      }
      else if (itemindexes[i] >= MSOGR_SYMBOLPARAMINDEX) {
        if (hSymbolStyle == NULL
            || ((pszValue = OGR_ST_GetParamStr(hSymbolStyle,
                                               itemindexes[i] - MSOGR_SYMBOLPARAMINDEX,
                                               &bDefault)) == NULL))
          values[i] = msStrdup("");
        else
          values[i] = msStrdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug(MSOGR_SYMBOLPARAMNAME " = \"%s\"\n", values[i]);
      }
      else {
        msFreeCharArray(values,i);

        OGR_SM_Destroy(hStyleMgr);
        OGR_ST_Destroy(hLabelStyle);
        OGR_ST_Destroy(hPenStyle);
        OGR_ST_Destroy(hBrushStyle);
        OGR_ST_Destroy(hSymbolStyle);

        msSetError(MS_OGRERR,"Invalid field index!?!","msOGRGetValues()");
        return(NULL);
      }
    }
  }

  OGR_SM_Destroy(hStyleMgr);
  OGR_ST_Destroy(hLabelStyle);
  OGR_ST_Destroy(hPenStyle);
  OGR_ST_Destroy(hBrushStyle);
  OGR_ST_Destroy(hSymbolStyle);

  return(values);
}

/**********************************************************************
 *                     msOGRSpatialRef2ProjectionObj()
 *
 * Init a MapServer projectionObj using an OGRSpatialRef
 * Works only with PROJECTION AUTO
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
static int msOGRSpatialRef2ProjectionObj(OGRSpatialReferenceH hSRS,
    projectionObj *proj, int debug_flag )
{
  // First flush the "auto" name from the projargs[]...
  msFreeProjectionExceptContext( proj );

  if (hSRS == NULL || OSRIsLocal( hSRS ) ) {
    // Dataset had no set projection or is NonEarth (LOCAL_CS)...
    // Nothing else to do. Leave proj empty and no reprojection will happen!
    return MS_SUCCESS;
  }

#if PROJ_VERSION_MAJOR >= 6
  // This could be done also in the < 6 case, but would be useless.
  // Here this helps avoiding going through potentially lossy PROJ4 strings
  const char* pszAuthName = OSRGetAuthorityName(hSRS, NULL);
  if( pszAuthName && EQUAL(pszAuthName, "EPSG") )
  {
    const char* pszAuthCode = OSRGetAuthorityCode(hSRS, NULL);
    if( pszAuthCode )
    {
        char szInitStr[32];
        sprintf(szInitStr, "init=epsg:%d", atoi(pszAuthCode));

        if( debug_flag )
            msDebug( "AUTO = %s\n", szInitStr );

        return msLoadProjectionString(proj, szInitStr) == 0 ? MS_SUCCESS : MS_FAILURE;
    }
  }
#endif

  // Export OGR SRS to a PROJ4 string
  char *pszProj = NULL;

  if (OSRExportToProj4( hSRS, &pszProj ) != OGRERR_NONE ||
      pszProj == NULL || strlen(pszProj) == 0) {
    msSetError(MS_OGRERR, "Conversion from OGR SRS to PROJ4 failed.",
               "msOGRSpatialRef2ProjectionObj()");
    CPLFree(pszProj);
    return(MS_FAILURE);
  }

  if( debug_flag )
    msDebug( "AUTO = %s\n", pszProj );

  if( msLoadProjectionString( proj, pszProj ) != 0 )
    return MS_FAILURE;

  CPLFree(pszProj);

  return MS_SUCCESS;
}

/**********************************************************************
 *                     msOGCWKT2ProjectionObj()
 *
 * Init a MapServer projectionObj using an OGC WKT definition.
 * Works only with PROJECTION AUTO
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/

int msOGCWKT2ProjectionObj( const char *pszWKT,
                            projectionObj *proj,
                            int debug_flag )

{
  OGRSpatialReferenceH        hSRS;
  char      *pszAltWKT = (char *) pszWKT;
  OGRErr  eErr;
  int     ms_result;

  hSRS = OSRNewSpatialReference( NULL );

  if( !EQUALN(pszWKT,"GEOGCS",6)
      && !EQUALN(pszWKT,"PROJCS",6)
      && !EQUALN(pszWKT,"LOCAL_CS",8) )
    eErr = OSRSetFromUserInput( hSRS, pszWKT );
  else
    eErr = OSRImportFromWkt( hSRS, &pszAltWKT );

  if( eErr != OGRERR_NONE ) {
    OSRDestroySpatialReference( hSRS );
    msSetError(MS_OGRERR,
               "Ingestion of WKT string '%s' failed.",
               "msOGCWKT2ProjectionObj()",
               pszWKT );
    return MS_FAILURE;
  }

  ms_result = msOGRSpatialRef2ProjectionObj( hSRS, proj, debug_flag );

  OSRDestroySpatialReference( hSRS );
  return ms_result;
}

/**********************************************************************
 *                     msOGRFileOpen()
 *
 * Open an OGR connection, and initialize a msOGRFileInfo.
 **********************************************************************/

static int bOGRDriversRegistered = MS_FALSE;

void msOGRInitialize(void)

{
  /* ------------------------------------------------------------------
   * Register OGR Drivers, only once per execution
   * ------------------------------------------------------------------ */
  if (!bOGRDriversRegistered) {
    ACQUIRE_OGR_LOCK;

    OGRRegisterAll();
    CPLPushErrorHandler( CPLQuietErrorHandler );

    /* ------------------------------------------------------------------
     * Pass config option GML_FIELDTYPES=ALWAYS_STRING to OGR so that all
     * GML attributes are returned as strings to MapServer. This is most efficient
     * and prevents problems with autodetection of some attribute types.
     * ------------------------------------------------------------------ */
    CPLSetConfigOption("GML_FIELDTYPES","ALWAYS_STRING");

    bOGRDriversRegistered = MS_TRUE;

    RELEASE_OGR_LOCK;
  }
}

/* ==================================================================
 * The following functions closely relate to the API called from
 * maplayer.c, but are intended to be used for the tileindex or direct
 * layer access.
 * ================================================================== */

static void msOGRFileOpenSpatialite( layerObj *layer, 
                                     const char *pszLayerDef,
                                     msOGRFileInfo *psInfo );

/**********************************************************************
 *                     msOGRFileOpen()
 *
 * Open an OGR connection, and initialize a msOGRFileInfo.
 **********************************************************************/

static msOGRFileInfo *
msOGRFileOpen(layerObj *layer, const char *connection )

{
  char *conn_decrypted = NULL;

  msOGRInitialize();

  /* ------------------------------------------------------------------
   * Make sure any encrypted token in the connection string are decrypted
   * ------------------------------------------------------------------ */
  if (connection) {
    conn_decrypted = msDecryptStringTokens(layer->map, connection);
    if (conn_decrypted == NULL)
      return NULL;  /* An error should already have been reported */
  }

  /* ------------------------------------------------------------------
   * Parse connection string into dataset name, and layer name.
   * ------------------------------------------------------------------ */
  char *pszDSName = NULL, *pszLayerDef = NULL;

  if( conn_decrypted == NULL ) {
    /* we don't have anything */
  } else if( layer->data != NULL ) {
    pszDSName = CPLStrdup(conn_decrypted);
    pszLayerDef = CPLStrdup(layer->data);
  } else {
    char **papszTokens = NULL;

    papszTokens = CSLTokenizeStringComplex( conn_decrypted, ",", TRUE, FALSE );

    if( CSLCount(papszTokens) > 0 )
      pszDSName = CPLStrdup( papszTokens[0] );
    if( CSLCount(papszTokens) > 1 )
      pszLayerDef = CPLStrdup( papszTokens[1] );

    CSLDestroy(papszTokens);
  }

  /* Get rid of decrypted connection string. We'll use the original (not
   * decrypted) string for debug and error messages in the rest of the code.
   */
  msFree(conn_decrypted);
  conn_decrypted = NULL;

  if( pszDSName == NULL ) {
    msSetError(MS_OGRERR,
               "Error parsing OGR connection information in layer `%s'",
               "msOGRFileOpen()",
               layer->name?layer->name:"(null)" );
    return NULL;
  }

  if( pszLayerDef == NULL )
    pszLayerDef = CPLStrdup("0");

  /* -------------------------------------------------------------------- */
  /*      Can we get an existing connection for this layer?               */
  /* -------------------------------------------------------------------- */
  OGRDataSourceH hDS;

  hDS = (OGRDataSourceH) msConnPoolRequest( layer );

  /* -------------------------------------------------------------------- */
  /*      If not, open now, and register this connection with the         */
  /*      pool.                                                           */
  /* -------------------------------------------------------------------- */
  if( hDS == NULL ) {
    char szPath[MS_MAXPATHLEN] = "";
    const char *pszDSSelectedName = pszDSName;

    if( layer->debug )
      msDebug("msOGRFileOpen(%s)...\n", connection);

    CPLErrorReset();
    if (msTryBuildPath3(szPath, layer->map->mappath,
                        layer->map->shapepath, pszDSName) != NULL ||
        msTryBuildPath(szPath, layer->map->mappath, pszDSName) != NULL) {
      /* Use relative path */
      pszDSSelectedName = szPath;
    }

    if( layer->debug )
      msDebug("OGROPen(%s)\n", pszDSSelectedName);

    ACQUIRE_OGR_LOCK;
    char** connectionoptions = msGetStringListFromHashTable(&(layer->connectionoptions));
    hDS = (OGRDataSourceH) GDALOpenEx(pszDSSelectedName,
                                GDAL_OF_VECTOR,
                                NULL,
                                (const char* const*)connectionoptions,
                                NULL);
    CSLDestroy(connectionoptions);
    RELEASE_OGR_LOCK;

    if( hDS == NULL ) {
      msSetError(MS_OGRERR,
                   "Open failed for OGR connection in layer `%s'.  "
                   "File not found or unsupported format. Check server logs.",
                   "msOGRFileOpen()",
                   layer->name?layer->name:"(null)" );
      if( strlen(CPLGetLastErrorMsg()) == 0 )
        msDebug("Open failed for OGR connection in layer `%s'.\n%s\n",
                   layer->name?layer->name:"(null)",
                   CPLGetLastErrorMsg() );
      CPLFree( pszDSName );
      CPLFree( pszLayerDef );
      return NULL;
    }

    msConnPoolRegister( layer, hDS, msOGRCloseConnection );
  }

  CPLFree( pszDSName );
  pszDSName = NULL;

  /* ------------------------------------------------------------------
   * Find the layer selected.
   * ------------------------------------------------------------------ */

  int   nLayerIndex = 0;
  OGRLayerH     hLayer = NULL;

  int  iLayer;

  if( EQUALN(pszLayerDef,"SELECT ",7) ) {
    ACQUIRE_OGR_LOCK;
    hLayer = OGR_DS_ExecuteSQL( hDS, pszLayerDef, NULL, NULL );
    if( hLayer == NULL ) {
      msSetError(MS_OGRERR,
                 "ExecuteSQL() failed. Check server logs.",
                 "msOGRFileOpen()");
      if( strlen(CPLGetLastErrorMsg()) == 0 )
        msDebug("ExecuteSQL(%s) failed.\n%s\n",
                pszLayerDef, CPLGetLastErrorMsg() );
      RELEASE_OGR_LOCK;
      msConnPoolRelease( layer, hDS );
      CPLFree( pszLayerDef );
      return NULL;
    }
    RELEASE_OGR_LOCK;
    nLayerIndex = -1;
  }

  for( iLayer = 0; hLayer == NULL && iLayer < OGR_DS_GetLayerCount(hDS); iLayer++ ) {
    hLayer = OGR_DS_GetLayer( hDS, iLayer );
    if( hLayer != NULL
        && EQUAL(OGR_L_GetName(hLayer),pszLayerDef) )
    {
      nLayerIndex = iLayer;
      break;
    } else
      hLayer = NULL;
  }

  if( hLayer == NULL && (atoi(pszLayerDef) > 0 || EQUAL(pszLayerDef,"0")) ) {
    nLayerIndex = atoi(pszLayerDef);
    if( nLayerIndex <  OGR_DS_GetLayerCount(hDS) )
      hLayer = OGR_DS_GetLayer( hDS, nLayerIndex );
  }

  if (hLayer == NULL) {
    msSetError(MS_OGRERR, "GetLayer(%s) failed for OGR connection. Check logs.",
               "msOGRFileOpen()",
               pszLayerDef);
    msDebug("GetLayer(%s) failed for OGR connection `%s'.\n",
            pszLayerDef, connection );
    CPLFree( pszLayerDef );
    msConnPoolRelease( layer, hDS );
    return NULL;
  }

  /* ------------------------------------------------------------------
   * OK... open succeded... alloc and fill msOGRFileInfo inside layer obj
   * ------------------------------------------------------------------ */
  msOGRFileInfo *psInfo =(msOGRFileInfo*)CPLCalloc(1,sizeof(msOGRFileInfo));

  psInfo->pszFname = CPLStrdup(OGR_DS_GetName( hDS ));
  psInfo->pszLayerDef = pszLayerDef;
  psInfo->nLayerIndex = nLayerIndex;
  psInfo->hDS = hDS;
  psInfo->hLayer = hLayer;

  psInfo->nTileId = 0;
  msInitProjection(&(psInfo->sTileProj));
  msProjectionInheritContextFrom(&(psInfo->sTileProj),&(layer->projection));
  psInfo->poCurTile = NULL;
  psInfo->rect_is_defined = false;
  psInfo->rect.minx = psInfo->rect.maxx = 0;
  psInfo->rect.miny = psInfo->rect.maxy = 0;
  psInfo->last_record_index_read = -1;
  psInfo->dialect = NULL;

  psInfo->pszSelect = NULL;
  psInfo->pszSpatialFilterTableName = NULL;
  psInfo->pszSpatialFilterGeometryColumn = NULL;
  psInfo->pszMainTableName = NULL;
  psInfo->pszRowId = NULL;
  psInfo->bIsOKForSQLCompose = true;
  psInfo->bPaging = false;
  psInfo->bHasSpatialIndex = false;
  psInfo->pszTablePrefix = NULL;
  psInfo->pszWHERE = NULL;

    // GDAL 1.x API
  OGRSFDriverH dr = OGR_DS_GetDriver(hDS);
  const char *name = OGR_Dr_GetName(dr);
  if (strcmp(name, "SQLite") == 0) {
    bool have_spatialite = false;

    CPLPushErrorHandler(CPLQuietErrorHandler);

    // test for Spatialite support in driver
    const char *test_spatialite = "SELECT spatialite_version()";
    OGRLayerH l = OGR_DS_ExecuteSQL(hDS, test_spatialite, NULL, NULL);
    if (l) {
        OGR_DS_ReleaseResultSet(hDS, l);
        have_spatialite = true;
    }

    // test for Spatialite enabled db
    if (have_spatialite) {
        have_spatialite = false;
        const char *test_sql = "select 1 from sqlite_master where name = 'geometry_columns' and sql LIKE '%spatial_index_enabled%'";
        OGRLayerH l = OGR_DS_ExecuteSQL(hDS, test_sql, NULL, NULL);
        if (l) {
            if (OGR_L_GetFeatureCount(l, TRUE) == 1)
                have_spatialite = true;
            OGR_DS_ReleaseResultSet(hDS, l);
        }
    }

    CPLPopErrorHandler();
    CPLErrorReset();

    if (have_spatialite)
        psInfo->dialect = "Spatialite";
    else
        msDebug("msOGRFileOpen: Native SQL not available, no Spatialite support and/or not a Spatialite enabled db\n");
  } else if (strcmp(name, "PostgreSQL") == 0) {
    psInfo->dialect = "PostgreSQL";
    // todo: PostgreSQL not yet tested

  } else if (strcmp(name, "GPKG") == 0 && nLayerIndex >= 0 &&
             atoi(GDALVersionInfo("VERSION_NUM")) >= 2000000) {

    bool has_rtree = false;
    const char* test_rtree =
        CPLSPrintf("SELECT 1 FROM sqlite_master WHERE name = 'rtree_%s_%s'",
                   OGR_L_GetName(hLayer), OGR_L_GetGeometryColumn(hLayer));
    OGRLayerH l = OGR_DS_ExecuteSQL(hDS, test_rtree, NULL, NULL);
    if( l )
    {
        if( OGR_L_GetFeatureCount(l, TRUE) == 1 )
        {
            has_rtree = true;
        }
        OGR_DS_ReleaseResultSet(hDS, l);
    }
    if( has_rtree )
    {
        bool have_gpkg_spatialite = false;

        CPLPushErrorHandler(CPLQuietErrorHandler);

        // test for Spatialite >= 4.3 support in driver
        const char *test_spatialite = "SELECT spatialite_version()";
        l = OGR_DS_ExecuteSQL(hDS, test_spatialite, NULL, NULL);
        if (l) {
            OGRFeatureH hFeat = OGR_L_GetNextFeature(l);
            if( hFeat )
            {
                const char* pszVersion = OGR_F_GetFieldAsString(hFeat, 0);
                have_gpkg_spatialite = atof(pszVersion) >= 4.3;
                OGR_F_Destroy(hFeat);
            }
            OGR_DS_ReleaseResultSet(hDS, l);
        }
        CPLPopErrorHandler();
        CPLErrorReset();

        if( have_gpkg_spatialite )
        {
            psInfo->pszMainTableName = msStrdup( OGR_L_GetName(hLayer) );
            psInfo->pszTablePrefix = msStrdup( psInfo->pszMainTableName );
            psInfo->pszSpatialFilterTableName = msStrdup( OGR_L_GetName(hLayer) );
            psInfo->pszSpatialFilterGeometryColumn = msStrdup( OGR_L_GetGeometryColumn(hLayer) );
            psInfo->dialect = "GPKG";
            psInfo->bPaging = true;
            psInfo->bHasSpatialIndex = true;
        }
        else
            msDebug("msOGRFileOpen: Spatialite support in GPKG not enabled\n");
    }
    else
    {
        msDebug("msOGRFileOpen: RTree index not available\n");
    }
  }

  if( psInfo->dialect != NULL && EQUAL(psInfo->dialect, "Spatialite") )
    msOGRFileOpenSpatialite(layer, pszLayerDef, psInfo);

  return psInfo;
}

/************************************************************************/
/*                      msOGRFileOpenSpatialite()                       */
/************************************************************************/

static void msOGRFileOpenSpatialite( layerObj *layer, 
                                     const char* pszLayerDef,
                                     msOGRFileInfo *psInfo )
{
  // In the case of a SQLite DB, check that we can identify the
  // underlying table
  if( psInfo->nLayerIndex == -1 )
  {
      psInfo->bIsOKForSQLCompose = false;

      const char* from = strstr( psInfo->pszLayerDef, " from ");
      if( from == NULL )
        from = strstr( psInfo->pszLayerDef, " FROM ");
      if( from )
      {
        const char* pszBeginningOfTable = from + strlen(" FROM ");
        const char* pszIter = pszBeginningOfTable;
        while( *pszIter && *pszIter != ' ' )
          pszIter ++;
        if( strchr(pszIter, ',') == NULL &&
            strstr(pszIter, " where ") == NULL && strstr(pszIter, " WHERE ") == NULL &&
            strstr(pszIter, " join ") == NULL && strstr(pszIter, " JOIN ") == NULL &&
            strstr(pszIter, " order by ") == NULL && strstr(pszIter, " ORDER BY ") == NULL)
        {
          psInfo->bIsOKForSQLCompose = true;
          psInfo->pszMainTableName = msStrdup(pszBeginningOfTable);
          psInfo->pszMainTableName[pszIter - pszBeginningOfTable] = '\0';
          psInfo->pszSpatialFilterTableName = msStrdup(psInfo->pszMainTableName);
          psInfo->pszSpatialFilterGeometryColumn = msStrdup( OGR_L_GetGeometryColumn(psInfo->hLayer) );

          char* pszRequest = NULL;
          pszRequest = msStringConcatenate(pszRequest,
              "SELECT name FROM sqlite_master WHERE type = 'table' AND name = lower('");
          pszRequest = msStringConcatenate(pszRequest, psInfo->pszMainTableName);
          pszRequest = msStringConcatenate(pszRequest, "')");
          OGRLayerH hLayer = OGR_DS_ExecuteSQL( psInfo->hDS, pszRequest, NULL, NULL );
          msFree(pszRequest);

          if( hLayer )
          {
              OGRFeatureH hFeature = OGR_L_GetNextFeature(hLayer);
              psInfo->bIsOKForSQLCompose = (hFeature != NULL);
              if( hFeature )
              {
                msFree(psInfo->pszMainTableName);
                msFree(psInfo->pszSpatialFilterTableName);
                psInfo->pszMainTableName = msStrdup( OGR_F_GetFieldAsString( hFeature, 0) );
                psInfo->pszSpatialFilterTableName = msStrdup( psInfo->pszMainTableName );
                OGR_F_Destroy(hFeature);
              }
              OGR_DS_ReleaseResultSet( psInfo->hDS, hLayer );
          }
          if( psInfo->bIsOKForSQLCompose )
          {
            psInfo->pszSelect = msStrdup(psInfo->pszLayerDef);
          }
          else
          {
            // Test if it is a spatial view
            pszRequest = msStringConcatenate(NULL,
              "SELECT f_table_name, f_geometry_column, view_rowid FROM views_geometry_columns WHERE view_name = lower('");
            pszRequest = msStringConcatenate(pszRequest, psInfo->pszMainTableName);
            pszRequest = msStringConcatenate(pszRequest, "')");
            CPLPushErrorHandler(CPLQuietErrorHandler);
            OGRLayerH hLayer = OGR_DS_ExecuteSQL( psInfo->hDS, pszRequest, NULL, NULL );
            CPLPopErrorHandler();
            msFree(pszRequest);

            if( hLayer )
            {
                OGRFeatureH hFeature = OGR_L_GetNextFeature(hLayer);
                psInfo->bIsOKForSQLCompose = (hFeature != NULL);
                if( hFeature )
                {
                  psInfo->pszSelect = msStrdup(psInfo->pszLayerDef);
                  msFree(psInfo->pszSpatialFilterTableName);
                  psInfo->pszSpatialFilterTableName = msStrdup( OGR_F_GetFieldAsString( hFeature, 0 ) );
                  CPLFree( psInfo->pszSpatialFilterGeometryColumn );
                  psInfo->pszSpatialFilterGeometryColumn = msStrdup( OGR_F_GetFieldAsString( hFeature, 1 ) );
                  psInfo->pszRowId = msStrdup( OGR_F_GetFieldAsString( hFeature, 2 ) );
                  OGR_F_Destroy(hFeature);
                }
                OGR_DS_ReleaseResultSet( psInfo->hDS, hLayer );
            }
          }
        }
      }
  }
  else
  {
      psInfo->bIsOKForSQLCompose = false;

      char* pszRequest = NULL;
      pszRequest = msStringConcatenate(pszRequest,
          "SELECT * FROM sqlite_master WHERE type = 'table' AND name = lower('");
      pszRequest = msStringConcatenate(pszRequest, OGR_FD_GetName(OGR_L_GetLayerDefn(psInfo->hLayer)));
      pszRequest = msStringConcatenate(pszRequest, "')");
      OGRLayerH hLayer = OGR_DS_ExecuteSQL( psInfo->hDS, pszRequest, NULL, NULL );
      msFree(pszRequest);

      if( hLayer )
      {
          OGRFeatureH hFeature = OGR_L_GetNextFeature(hLayer);
          psInfo->bIsOKForSQLCompose = (hFeature != NULL);
          if( hFeature )
            OGR_F_Destroy(hFeature);
          OGR_DS_ReleaseResultSet( psInfo->hDS, hLayer );
      }
      if( psInfo->bIsOKForSQLCompose )
      {
        psInfo->pszMainTableName = msStrdup(OGR_FD_GetName(OGR_L_GetLayerDefn(psInfo->hLayer)));
        psInfo->pszSpatialFilterTableName = msStrdup(psInfo->pszMainTableName);
        psInfo->pszSpatialFilterGeometryColumn = msStrdup( OGR_L_GetGeometryColumn(psInfo->hLayer) );
      }
      else
      {
        // Test if it is a spatial view
        pszRequest = msStringConcatenate(NULL,
          "SELECT f_table_name, f_geometry_column, view_rowid FROM views_geometry_columns WHERE view_name = lower('");
        pszRequest = msStringConcatenate(pszRequest, OGR_FD_GetName(OGR_L_GetLayerDefn(psInfo->hLayer)));
        pszRequest = msStringConcatenate(pszRequest, "')");
        CPLPushErrorHandler(CPLQuietErrorHandler);
        OGRLayerH hLayer = OGR_DS_ExecuteSQL( psInfo->hDS, pszRequest, NULL, NULL );
        CPLPopErrorHandler();
        msFree(pszRequest);

        if( hLayer )
        {
            OGRFeatureH hFeature = OGR_L_GetNextFeature(hLayer);
            psInfo->bIsOKForSQLCompose = (hFeature != NULL);
            if( hFeature )
            {
              psInfo->pszMainTableName = msStrdup(OGR_FD_GetName(OGR_L_GetLayerDefn(psInfo->hLayer)));
              psInfo->pszSpatialFilterTableName = msStrdup( OGR_F_GetFieldAsString( hFeature, 0 ) );
              psInfo->pszSpatialFilterGeometryColumn = msStrdup( OGR_F_GetFieldAsString( hFeature, 1 ) );
              psInfo->pszRowId = msStrdup( OGR_F_GetFieldAsString( hFeature, 2 ) );
              OGR_F_Destroy(hFeature);
            }
            OGR_DS_ReleaseResultSet( psInfo->hDS, hLayer );
        }
      }
  }

  // in the case we cannot handle the native string, go back to the client
  // side evaluation by unsetting it.
  if( !psInfo->bIsOKForSQLCompose && psInfo->dialect != NULL )
  {
      if (layer->debug >= MS_DEBUGLEVEL_VVV)
      {
        msDebug("msOGRFileOpen(): Falling back to MapServer only evaluation\n");
      }
      psInfo->dialect = NULL;
  }

  // Check if spatial index has been disabled (testing purposes)
  if (msLayerGetProcessingKey(layer, "USE_SPATIAL_INDEX") != NULL &&
      !CSLTestBoolean(msLayerGetProcessingKey(layer, "USE_SPATIAL_INDEX")) )
  {
      if (layer->debug >= MS_DEBUGLEVEL_VVV)
      {
        msDebug("msOGRFileOpen(): Layer %s has spatial index disabled by processing option\n",
                pszLayerDef);
      }
  }
  // Test if spatial index is available
  else if( psInfo->dialect != NULL )
  {
      char* pszRequest = NULL;
      pszRequest = msStringConcatenate(pszRequest,
          "SELECT * FROM sqlite_master WHERE type = 'table' AND name = 'idx_");
      pszRequest = msStringConcatenate(pszRequest,
                                       psInfo->pszSpatialFilterTableName);
      pszRequest = msStringConcatenate(pszRequest, "_");
      pszRequest = msStringConcatenate(pszRequest,
                                    OGR_L_GetGeometryColumn(psInfo->hLayer));
      pszRequest = msStringConcatenate(pszRequest, "'");

      psInfo->bHasSpatialIndex = false;
      //msDebug("msOGRFileOpen(): %s", pszRequest);

      OGRLayerH hLayer = OGR_DS_ExecuteSQL( psInfo->hDS, pszRequest, NULL, NULL );
      if( hLayer )
      {
        OGRFeatureH hFeature = OGR_L_GetNextFeature(hLayer);
        if( hFeature )
        {
            psInfo->bHasSpatialIndex = true;
            OGR_F_Destroy(hFeature);
        }
        OGR_DS_ReleaseResultSet( psInfo->hDS, hLayer );
      }
      msFree(pszRequest);
      pszRequest = NULL;

      if( !psInfo->bHasSpatialIndex )
      {
          if (layer->debug >= MS_DEBUGLEVEL_VVV)
          {
            msDebug("msOGRFileOpen(): Layer %s has no spatial index table\n",
                    pszLayerDef);
          }
      }
      else
      {
          pszRequest = msStringConcatenate(pszRequest,
            "SELECT * FROM geometry_columns WHERE f_table_name = lower('");
          pszRequest = msStringConcatenate(pszRequest,
                                       psInfo->pszSpatialFilterTableName);
          pszRequest = msStringConcatenate(pszRequest, "') AND f_geometry_column = lower('");
          pszRequest = msStringConcatenate(pszRequest,
                                       psInfo->pszSpatialFilterGeometryColumn);
          pszRequest = msStringConcatenate(pszRequest, "') AND spatial_index_enabled = 1");

          psInfo->bHasSpatialIndex = false;

          OGRLayerH hLayer = OGR_DS_ExecuteSQL( psInfo->hDS, pszRequest, NULL, NULL );
          if( hLayer )
          {
            OGRFeatureH hFeature = OGR_L_GetNextFeature(hLayer);
            if( hFeature )
            {
                psInfo->bHasSpatialIndex = true;
                OGR_F_Destroy(hFeature);
            }
            OGR_DS_ReleaseResultSet( psInfo->hDS, hLayer );
          }
          msFree(pszRequest);
          pszRequest = NULL;

          if( !psInfo->bHasSpatialIndex )
          {
            if (layer->debug >= MS_DEBUGLEVEL_VVV)
            {
              msDebug("msOGRFileOpen(): Layer %s has spatial index disabled\n",
                      pszLayerDef);
            }
          }
          else
          {
            if (layer->debug >= MS_DEBUGLEVEL_VVV)
            {
              msDebug("msOGRFileOpen(): Layer %s has spatial index enabled\n",
                      pszLayerDef);
            }

            psInfo->pszTablePrefix = msStrdup( psInfo->pszMainTableName );
          }
      }
  }

  psInfo->bPaging = (psInfo->dialect != NULL);
}

/************************************************************************/
/*                        msOGRCloseConnection()                        */
/*                                                                      */
/*      Callback for thread pool to actually release an OGR             */
/*      connection.                                                     */
/************************************************************************/

static void msOGRCloseConnection( void *conn_handle )

{
  OGRDataSourceH hDS = (OGRDataSourceH) conn_handle;

  ACQUIRE_OGR_LOCK;
  OGR_DS_Destroy( hDS );
  RELEASE_OGR_LOCK;
}

/**********************************************************************
 *                     msOGRFileClose()
 **********************************************************************/
static int msOGRFileClose(layerObj *layer, msOGRFileInfo *psInfo )
{
  if (!psInfo)
    return MS_SUCCESS;

  if( layer->debug )
    msDebug("msOGRFileClose(%s,%d).\n",
            psInfo->pszFname, psInfo->nLayerIndex);

  CPLFree(psInfo->pszFname);
  CPLFree(psInfo->pszLayerDef);

  ACQUIRE_OGR_LOCK;
  if (psInfo->hLastFeature)
    OGR_F_Destroy( psInfo->hLastFeature );

  /* If nLayerIndex == -1 then the layer is an SQL result ... free it */
  if( psInfo->nLayerIndex == -1 )
    OGR_DS_ReleaseResultSet( psInfo->hDS, psInfo->hLayer );

  // Release (potentially close) the datasource connection.
  // Make sure we aren't holding the lock when the callback may need it.
  RELEASE_OGR_LOCK;
  msConnPoolRelease( layer, psInfo->hDS );

  // Free current tile if there is one.
  if( psInfo->poCurTile != NULL )
    msOGRFileClose( layer, psInfo->poCurTile );

  msFreeProjection(&(psInfo->sTileProj));
  msFree(psInfo->pszSelect);
  msFree(psInfo->pszSpatialFilterTableName);
  msFree(psInfo->pszSpatialFilterGeometryColumn);
  msFree(psInfo->pszMainTableName);
  msFree(psInfo->pszRowId);
  msFree(psInfo->pszTablePrefix);
  msFree(psInfo->pszWHERE);

  CPLFree(psInfo);

  return MS_SUCCESS;
}

/************************************************************************/
/*                           msOGREscapeSQLParam                        */
/************************************************************************/
static char *msOGREscapeSQLParam(layerObj *layer, const char *pszString)
{
  char* pszEscapedStr =NULL;
  if(layer && pszString) {
    char* pszEscapedOGRStr =  CPLEscapeString(pszString, strlen(pszString),
                              CPLES_SQL );
    pszEscapedStr = msStrdup(pszEscapedOGRStr);
    CPLFree(pszEscapedOGRStr);
  }
  return pszEscapedStr;
}

// http://www.sqlite.org/lang_expr.html
// http://www.gaia-gis.it/gaia-sins/spatialite-sql-4.3.0.html

static char* msOGRGetQuotedItem(layerObj* layer, const char* pszItem )
{
    msOGRFileInfo *psInfo = (msOGRFileInfo *)layer->layerinfo;
    char* ret = NULL;
    char* escapedItem = msLayerEscapePropertyName(layer, pszItem);
    if( psInfo->pszTablePrefix)
    {
        char* escapedTable = msLayerEscapePropertyName(layer, psInfo->pszTablePrefix);
        ret = msStringConcatenate(ret, "\"");
        ret = msStringConcatenate(ret, escapedTable);
        ret = msStringConcatenate(ret, "\".\"");
        ret = msStringConcatenate(ret, escapedItem);
        ret = msStringConcatenate(ret, "\"");
        msFree(escapedTable);
    }
    else
    {
        ret = msStringConcatenate(ret, "\"");
        ret = msStringConcatenate(ret, escapedItem);
        ret = msStringConcatenate(ret, "\"");
    }
    msFree(escapedItem);
    return ret;
}

static
char *msOGRGetToken(layerObj* layer, tokenListNodeObjPtr *node) {
    msOGRFileInfo *info = (msOGRFileInfo *)layer->layerinfo;
    tokenListNodeObjPtr n = *node;
    if (!n) return NULL;
    char *out = NULL;
    size_t nOutSize;

    switch(n->token) {
    case MS_TOKEN_LOGICAL_AND:
        out = msStrdup(" AND ");
        break;
    case MS_TOKEN_LOGICAL_OR:
        out = msStrdup(" OR ");
        break;
    case MS_TOKEN_LOGICAL_NOT:
        out = msStrdup(" NOT ");
        break;
    case MS_TOKEN_LITERAL_NUMBER:
        nOutSize = 32;
        out = (char *)msSmallMalloc(nOutSize);
        snprintf(out, nOutSize, "%.18g", n->tokenval.dblval);
        break;
    case MS_TOKEN_LITERAL_STRING: {
        char *stresc = msOGREscapeSQLParam(layer, n->tokenval.strval);
        nOutSize = strlen(stresc)+3;
        out = (char *)msSmallMalloc(nOutSize);
        snprintf(out, nOutSize, "'%s'", stresc);
        msFree(stresc);
        break;
    }
    case MS_TOKEN_LITERAL_TIME:
        // seems to require METADATA gml_types => auto
        nOutSize = 80;
        out = (char *)msSmallMalloc(nOutSize);
#if 0
        // FIXME? or perhaps just remove me. tm_zone is not supported on Windows, and not used anywhere else in the code base
        if (n->tokenval.tmval.tm_zone)
            snprintf(out, nOutSize, "'%d-%02d-%02dT%02d:%02d:%02d%s'",
                     n->tokenval.tmval.tm_year+1900, n->tokenval.tmval.tm_mon+1, n->tokenval.tmval.tm_mday,
                     n->tokenval.tmval.tm_hour, n->tokenval.tmval.tm_min, n->tokenval.tmval.tm_sec,
                     n->tokenval.tmval.tm_zone);
        else
#endif
            snprintf(out, nOutSize, "'%d-%02d-%02dT%02d:%02d:%02d'",  
                     n->tokenval.tmval.tm_year+1900, n->tokenval.tmval.tm_mon+1, n->tokenval.tmval.tm_mday,
                     n->tokenval.tmval.tm_hour, n->tokenval.tmval.tm_min, n->tokenval.tmval.tm_sec);
        break;
    case MS_TOKEN_LITERAL_SHAPE: {
        // assumed to be in right srs after FLTGetSpatialComparisonCommonExpression
        char *wkt = msShapeToWKT(n->tokenval.shpval);
        char *stresc = msOGRGetQuotedItem(layer, OGR_L_GetGeometryColumn(info->hLayer)); // which geom field??
        nOutSize = strlen(wkt)+strlen(stresc)+35;
        out = (char *)msSmallMalloc(nOutSize);
        snprintf(out, nOutSize, "ST_GeomFromText('%s',ST_SRID(%s))", wkt, stresc);
        msFree(wkt);
        msFree(stresc);
        break;
    }
    case MS_TOKEN_LITERAL_BOOLEAN:
        out = msStrdup(n->tokenval.dblval == 0 ? "FALSE" : "TRUE");
        break;
    case MS_TOKEN_COMPARISON_EQ:
        if(n->next != NULL && n->next->token == MS_TOKEN_LITERAL_STRING &&
           strcmp(n->next->tokenval.strval, "_MAPSERVER_NULL_") == 0 )
        {
            out = msStrdup(" IS NULL");
            n = n->next;
            break;
        }

        out = msStrdup(" = ");
        break;
    case MS_TOKEN_COMPARISON_NE:
        out = msStrdup(" != ");
        break;
    case MS_TOKEN_COMPARISON_GT:
        out = msStrdup(" > ");
        break;
    case MS_TOKEN_COMPARISON_LT:
        out = msStrdup(" < ");
        break;
    case MS_TOKEN_COMPARISON_LE:
        out = msStrdup(" <= ");
        break;
    case MS_TOKEN_COMPARISON_GE:
        out = msStrdup(" >= ");
        break;
    case MS_TOKEN_COMPARISON_IEQ:
        out = msStrdup(" = ");
        break;
    case MS_TOKEN_COMPARISON_IN:
        out = msStrdup(" IN ");
        break;
    // the origin may be mapfile (complex regexes, layer->map.query.filter.string == NULL, regex may have //) or 
    // OGC Filter (simple patterns only, layer->map.query.filter.string != NULL)
    case MS_TOKEN_COMPARISON_RE: 
    case MS_TOKEN_COMPARISON_IRE: 
    case MS_TOKEN_COMPARISON_LIKE: {
        int case_sensitive = n->token == MS_TOKEN_COMPARISON_RE || n->token == MS_TOKEN_COMPARISON_LIKE;
        // in PostgreSQL and OGR: LIKE (case sensitive) and ILIKE (case insensitive)
        // in SQLite: LIKE (case insensitive) and GLOB (case sensitive)
        const char *op = case_sensitive ? "LIKE" : "ILIKE";
        char wild_any =  '%';
        char wild_one = '_';

        if (EQUAL(info->dialect, "Spatialite") || EQUAL(info->dialect, "GPKG")) {
            if (case_sensitive) {
                op = "GLOB";
                wild_any = '*';
                wild_one = '?';
            } else {
                op = "LIKE";
            }
        }

        n = n->next;
        if (n->token != MS_TOKEN_LITERAL_STRING) return NULL;

        char *regex = msStrdup(n->tokenval.strval);
        int complex_regex = *n->tokenval.strval == '/'; // could be non-complex but that is soo corner case

        // PostgreSQL has POSIX regexes, SQLite does not by default, OGR does not
        if (complex_regex) {
            if (!EQUAL(info->dialect, "PostgreSQL")) {
                msFree(regex);
                return NULL;
            }
            // remove //
            regex++;
            regex[strlen(regex) - 1] = '\0';
            if (case_sensitive)
                op = "~";
            else
                op = "~*";
        }

        char *re = (char *) msSmallMalloc(strlen(regex)+3);
        size_t i = 0, j = 0;
        re[j++] = '\'';
        while (i < strlen(regex)) {
            char c = regex[i];
            char c_next = regex[i+1];
                
            if (c == '.' && c_next == '*') {
                i++;
                c = wild_any;
            }
            else if (c == '.')
                c = wild_one;
            else if (i == 0 && c == '^') {
                i++;
                continue;
            }
            else if( c == '$' && c_next == 0 ) {
                break;
            }

            re[j++] = c;
            i++;
                
        }
        re[j++] = '\'';
        re[j] = '\0';

        nOutSize = 1 + strlen(op)+ 1 + strlen(re) + 1;
        out = (char *)msSmallMalloc(nOutSize);
        snprintf(out, nOutSize, " %s %s", op, re);
        msFree(re);
        msFree(regex);
        break;
    }
    case MS_TOKEN_COMPARISON_INTERSECTS:
        out = msStrdup("ST_Intersects");
        break;
    case MS_TOKEN_COMPARISON_DISJOINT:
        out = msStrdup("ST_Disjoint");
        break;
    case MS_TOKEN_COMPARISON_TOUCHES:
        out = msStrdup("ST_Touches");
        break;
    case MS_TOKEN_COMPARISON_OVERLAPS:
        out = msStrdup("ST_Overlaps");
        break;
    case MS_TOKEN_COMPARISON_CROSSES:
        out = msStrdup("ST_Crosses");
        break;
    case MS_TOKEN_COMPARISON_WITHIN:
        out = msStrdup("ST_Within");
        break;
    case MS_TOKEN_COMPARISON_DWITHIN:
        out = msStrdup("ST_Distance");
        break;
    case MS_TOKEN_COMPARISON_BEYOND:
        out = msStrdup("ST_Distance");
        break;
    case MS_TOKEN_COMPARISON_CONTAINS:
        out = msStrdup("ST_Contains");
        break;
    case MS_TOKEN_COMPARISON_EQUALS:
        out = msStrdup("ST_Equals");
        break;
    case MS_TOKEN_FUNCTION_LENGTH:
        out = msStrdup("ST_Length");
        break;
    case MS_TOKEN_FUNCTION_AREA:
        out = msStrdup("ST_Area");
        break;
    case MS_TOKEN_BINDING_DOUBLE: {
        char *stresc = msOGRGetQuotedItem(layer, n->tokenval.bindval.item);
        nOutSize = strlen(stresc)+ + 30;
        out = (char *)msSmallMalloc(nOutSize);

        char md_item_name[256];
        snprintf( md_item_name, sizeof(md_item_name), "gml_%s_type",
                  n->tokenval.bindval.item );
        const char* type = msLookupHashTable(&(layer->metadata), md_item_name);
        // Do not cast if the variable is of the appropriate type as it can
        // prevent using database indexes, such as for SQlite
        if( type != NULL && (EQUAL(type, "Integer") ||
                             EQUAL(type, "Long") ||
                             EQUAL(type, "Real")) )
        {
             snprintf(out, nOutSize, "%s", stresc);
        }
        else
        {
            const char *SQLtype = "float(16)";
            if (EQUAL(info->dialect, "Spatialite") || EQUAL(info->dialect, "GPKG"))
                SQLtype = "REAL";
            else if (EQUAL(info->dialect, "PostgreSQL"))
                SQLtype = "double precision";
            snprintf(out, nOutSize, "CAST(%s AS %s)", stresc, SQLtype);
        }
        msFree(stresc);
        break;
    }
    case MS_TOKEN_BINDING_INTEGER: {
        char *stresc = msOGRGetQuotedItem(layer, n->tokenval.bindval.item);
        nOutSize = strlen(stresc)+ 20;
        out = (char *)msSmallMalloc(nOutSize);

        char md_item_name[256];
        snprintf( md_item_name, sizeof(md_item_name), "gml_%s_type",
                  n->tokenval.bindval.item );
        const char* type = msLookupHashTable(&(layer->metadata), md_item_name);
        // Do not cast if the variable is of the appropriate type as it can
        // prevent using database indexes, such as for SQlite
        if( type != NULL && (EQUAL(type, "Integer") ||
                             EQUAL(type, "Long") ||
                             EQUAL(type, "Real")) )
        {
            snprintf(out, nOutSize, "%s", stresc);
        }
        else
        {
            snprintf(out, nOutSize, "CAST(%s AS integer)", stresc);
        }
        msFree(stresc);
        break;
    }
    case MS_TOKEN_BINDING_STRING: {
        char *stresc = msOGRGetQuotedItem(layer, n->tokenval.bindval.item);
        nOutSize = strlen(stresc) + 30;
        out = (char *)msSmallMalloc(nOutSize);

        char md_item_name[256];
        snprintf( md_item_name, sizeof(md_item_name), "gml_%s_type",
                  n->tokenval.bindval.item );
        const char* type = msLookupHashTable(&(layer->metadata), md_item_name);
        // Do not cast if the variable is of the appropriate type as it can
        // prevent using database indexes, such as for SQlite
        if( type != NULL && EQUAL(type, "Character") )
        {
            snprintf(out, nOutSize, "%s", stresc);
        }
        else
        {
            snprintf(out, nOutSize, "CAST(%s AS text)", stresc);
        }
        msFree(stresc);
        break;
    }
    case MS_TOKEN_BINDING_TIME: {
        // won't get here unless col is parsed as time and they are not
        char *stresc = msOGRGetQuotedItem(layer, n->tokenval.bindval.item);
        nOutSize = strlen(stresc)+ 10;
        out = (char *)msSmallMalloc(nOutSize);
        snprintf(out, nOutSize, "%s", stresc);
        msFree(stresc);
        break;
    }
    case MS_TOKEN_BINDING_SHAPE: {
        char *stresc = msOGRGetQuotedItem(layer, OGR_L_GetGeometryColumn(info->hLayer)); // which geom field??
        nOutSize = strlen(stresc)+ 10;
        out = (char *)msSmallMalloc(nOutSize);
        snprintf(out, nOutSize, "%s", stresc);
        msFree(stresc);
        break;
    }

    // unhandled below until default

    case MS_TOKEN_FUNCTION_TOSTRING:
    case MS_TOKEN_FUNCTION_COMMIFY:
    case MS_TOKEN_FUNCTION_ROUND:
    case MS_TOKEN_FUNCTION_FROMTEXT:
    case MS_TOKEN_FUNCTION_BUFFER:
    case MS_TOKEN_FUNCTION_DIFFERENCE:
    case MS_TOKEN_FUNCTION_SIMPLIFY:
    case MS_TOKEN_FUNCTION_SIMPLIFYPT:
    case MS_TOKEN_FUNCTION_GENERALIZE:
    case MS_TOKEN_FUNCTION_SMOOTHSIA:
    case MS_TOKEN_FUNCTION_JAVASCRIPT:
    case MS_TOKEN_FUNCTION_UPPER:
    case MS_TOKEN_FUNCTION_LOWER:
    case MS_TOKEN_FUNCTION_INITCAP:
    case MS_TOKEN_FUNCTION_FIRSTCAP:
    case MS_TOKEN_BINDING_MAP_CELLSIZE:
    case MS_TOKEN_BINDING_DATA_CELLSIZE:
    case MS_PARSE_TYPE_BOOLEAN:
    case MS_PARSE_TYPE_STRING:
    case MS_PARSE_TYPE_SHAPE:
        break;

    default:
        if (n->token < 128) {
            char c = n->token;
            out = (char *)msSmallMalloc(2);
            sprintf(out, "%c", c);
        }
        break;
    }

    n = n->next;
    *node = n;
    return out;
}

/*
 * msOGRLayerBuildSQLOrderBy()
 *
 * Returns the content of a SQL ORDER BY clause from the sortBy member of
 * the layer. The string does not contain the "ORDER BY" keywords itself.
 */
static char* msOGRLayerBuildSQLOrderBy(layerObj *layer, msOGRFileInfo *psInfo)
{
  char* strOrderBy = NULL;
  if( layer->sortBy.nProperties > 0 ) {
    int i;
    for(i=0;i<layer->sortBy.nProperties;i++) {
      if( i > 0 )
        strOrderBy = msStringConcatenate(strOrderBy, ", ");
      char* escapedItem = msLayerEscapePropertyName(layer, layer->sortBy.properties[i].item);
      if( psInfo->pszTablePrefix)
      {
          char* escapedTable = msLayerEscapePropertyName(layer, psInfo->pszTablePrefix);
          strOrderBy = msStringConcatenate(strOrderBy, "\"");
          strOrderBy = msStringConcatenate(strOrderBy, escapedTable);
          strOrderBy = msStringConcatenate(strOrderBy, "\".\"");
          strOrderBy = msStringConcatenate(strOrderBy, escapedItem);
          strOrderBy = msStringConcatenate(strOrderBy, "\"");
          msFree(escapedTable);
      }
      else
      {
#if GDAL_VERSION_MAJOR < 2
          // Old GDAL don't like quoted identifiers in ORDER BY
          strOrderBy = msStringConcatenate(strOrderBy,
                                           layer->sortBy.properties[i].item);
#else
          strOrderBy = msStringConcatenate(strOrderBy, "\"");
          strOrderBy = msStringConcatenate(strOrderBy, escapedItem);
          strOrderBy = msStringConcatenate(strOrderBy, "\"");
#endif
      }
      msFree(escapedItem);
      if( layer->sortBy.properties[i].sortOrder == SORT_DESC )
        strOrderBy = msStringConcatenate(strOrderBy, " DESC");
    }
  }
  return strOrderBy;
}

/**********************************************************************
 *                     msOGRFileWhichShapes()
 *
 * Init OGR layer structs ready for calls to msOGRFileNextShape().
 *
 * Returns MS_SUCCESS/MS_FAILURE, or MS_DONE if no shape matching the
 * layer's FILTER overlaps the selected region.
 **********************************************************************/
static int msOGRFileWhichShapes(layerObj *layer, rectObj rect, msOGRFileInfo *psInfo)
{
    // rect is from BBOX parameter in query (In lieu of a FEATUREID or FILTER) or mapfile somehow
    if (psInfo == NULL || psInfo->hLayer == NULL) {
        msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", "msOGRFileWhichShapes()");
        return(MS_FAILURE);
    }

    char *select = (psInfo->pszSelect) ? msStrdup(psInfo->pszSelect) : NULL;
    const rectObj rectInvalid = MS_INIT_INVALID_RECT;
    bool bIsValidRect = memcmp(&rect, &rectInvalid, sizeof(rect)) != 0;

    // we'll go strictly two possible ways: 
    // 1) GetLayer + SetFilter
    // 2) ExecuteSQL (psInfo->hLayer is an SQL result OR sortBy was requested OR have native_string
    // and start from the second

    if ( psInfo->bIsOKForSQLCompose && (psInfo->nLayerIndex == -1 ||
                                        layer->sortBy.nProperties > 0 ||
                                        layer->filter.native_string ||
                                        (psInfo->bPaging && layer->maxfeatures > 0)) ) {

        const bool bHasGeometry = 
                                OGR_L_GetGeomType( psInfo->hLayer ) != wkbNone;

        if( psInfo->nLayerIndex == -1 && select == NULL ) {
            select = msStrdup(psInfo->pszLayerDef);
            /* If nLayerIndex == -1 then the layer is an SQL result ... free it */
            OGR_DS_ReleaseResultSet( psInfo->hDS, psInfo->hLayer );
            psInfo->hLayer = NULL;
        }
        else if( select == NULL ) {
            const char* pszGeometryColumn;
            int i;
            select = msStringConcatenate(select, "SELECT ");
            for(i = 0; i < layer->numitems; i++) {
                if( i > 0 )
                    select = msStringConcatenate(select, ", ");
                char* escaped = msOGRGetQuotedItem(layer, layer->items[i]);
                select = msStringConcatenate(select, escaped);
                msFree(escaped);
                if( psInfo->pszTablePrefix )
                {
                    select = msStringConcatenate(select, " AS \"");
                    escaped = msLayerEscapePropertyName(layer, layer->items[i]);
                    select = msStringConcatenate(select, escaped);
                    msFree(escaped);
                    select = msStringConcatenate(select, "\"");
                }
            }
            if( layer->numitems > 0 )
                select = msStringConcatenate(select, ", ");
            pszGeometryColumn = OGR_L_GetGeometryColumn(psInfo->hLayer);
            if( pszGeometryColumn != NULL && pszGeometryColumn[0] != '\0' ) {
                char* escaped = msOGRGetQuotedItem(layer, pszGeometryColumn);
                select = msStringConcatenate(select, escaped);
                msFree(escaped);
                if( psInfo->pszTablePrefix )
                {
                    select = msStringConcatenate(select, " AS \"");
                    escaped = msLayerEscapePropertyName(layer, pszGeometryColumn);
                    select = msStringConcatenate(select, escaped);
                    msFree(escaped);
                    select = msStringConcatenate(select, "\"");
                }
            } else {
                /* Add ", *" so that we still have an hope to get the geometry */
                if( psInfo->pszTablePrefix )
                {
                    select = msStringConcatenate(select, "\"");
                    char* escaped = msLayerEscapePropertyName(layer, psInfo->pszTablePrefix);
                    select = msStringConcatenate(select, escaped);
                    msFree(escaped);
                    select = msStringConcatenate(select, "\".");
                }
                select = msStringConcatenate(select, "*");
            }
            select = msStringConcatenate(select, " FROM ");
            if( psInfo->nLayerIndex == -1 )
            {
              select = msStringConcatenate(select, "(");
              select = msStringConcatenate(select, psInfo->pszLayerDef);
              select = msStringConcatenate(select, ") MSSUBSELECT");
            }
            else
            {
              select = msStringConcatenate(select, "\"");
              char* escaped = msLayerEscapePropertyName(layer, OGR_FD_GetName(OGR_L_GetLayerDefn(psInfo->hLayer)));
              select = msStringConcatenate(select, escaped);
              msFree(escaped);
              select = msStringConcatenate(select, "\"");
            }
        }

        char *filter = NULL;
        if (msLayerGetProcessingKey(layer, "NATIVE_FILTER") != NULL) {
            filter = msStringConcatenate(filter, "(");
            filter = msStringConcatenate(filter, msLayerGetProcessingKey(layer, "NATIVE_FILTER"));
            filter = msStringConcatenate(filter, ")");
        }

        /* ------------------------------------------------------------------
         * Set Spatial filter... this may result in no features being returned
         * if layer does not overlap current view.
         *
         * __TODO__ We should return MS_DONE if no shape overlaps the selected
         * region and matches the layer's FILTER expression, but there is currently
         * no _efficient_ way to do that with OGR.
         * ------------------------------------------------------------------ */
        if (psInfo->rect_is_defined) {
            rect.minx = MAX(psInfo->rect.minx, rect.minx);
            rect.miny = MAX(psInfo->rect.miny, rect.miny);
            rect.maxx = MIN(psInfo->rect.maxx, rect.maxx);
            rect.maxy = MIN(psInfo->rect.maxy, rect.maxy);
            bIsValidRect = true;
        }
        psInfo->rect = rect;

        bool bSpatialiteOrGPKGAddOrderByFID = false;

        const char *sql = layer->filter.native_string;
        if (psInfo->dialect && sql && *sql != '\0' &&
            (EQUAL(psInfo->dialect, "Spatialite") ||
             EQUAL(psInfo->dialect, "GPKG") ||
             EQUAL(psInfo->dialect, "PostgreSQL")) )
        {
            if (filter) filter = msStringConcatenate(filter, " AND ");
            filter = msStringConcatenate(filter, "(");
            filter = msStringConcatenate(filter, sql);
            filter = msStringConcatenate(filter, ")");
        }
        else if( psInfo->pszWHERE )
        {
            if (filter) filter = msStringConcatenate(filter, " AND ");
            filter = msStringConcatenate(filter, "(");
            filter = msStringConcatenate(filter, psInfo->pszWHERE);
            filter = msStringConcatenate(filter, ")");
        }

        // use spatial index
        if (psInfo->dialect && bIsValidRect ) {
            if (EQUAL(psInfo->dialect, "PostgreSQL")) {
                if (filter) filter = msStringConcatenate(filter, " AND");
                const char *col = OGR_L_GetGeometryColumn(psInfo->hLayer); // which geom field??
                filter = msStringConcatenate(filter, " (\"");
                char* escaped = msLayerEscapePropertyName(layer, col);
                filter = msStringConcatenate(filter, escaped);
                msFree(escaped);
                filter = msStringConcatenate(filter, "\" && ST_MakeEnvelope(");
                char *points = (char *)msSmallMalloc(30*2*5);
                snprintf(points, 30*4, "%lf,%lf,%lf,%lf", rect.minx, rect.miny, rect.maxx, rect.maxy);
                filter = msStringConcatenate(filter, points);
                msFree(points);
                filter = msStringConcatenate(filter, "))");
            }
            else if( psInfo->dialect &&
                     (EQUAL(psInfo->dialect, "Spatialite") ||
                      EQUAL(psInfo->dialect, "GPKG")) &&
                     psInfo->pszMainTableName != NULL )
            {
                if( (EQUAL(psInfo->dialect, "Spatialite") && psInfo->bHasSpatialIndex)
                      || EQUAL(psInfo->dialect, "GPKG") )
                {
                    if (filter) filter = msStringConcatenate(filter, " AND ");
                    char* pszEscapedMainTableName = msLayerEscapePropertyName(
                                                    layer, psInfo->pszMainTableName);
                    filter = msStringConcatenate(filter, "\"");
                    filter = msStringConcatenate(filter, pszEscapedMainTableName);
                    msFree(pszEscapedMainTableName);
                    filter = msStringConcatenate(filter, "\".");
                    if( psInfo->pszRowId )
                    {
                        char* pszEscapedRowId = msLayerEscapePropertyName(
                                                            layer, psInfo->pszRowId);
                        filter = msStringConcatenate(filter, "\"");
                        filter = msStringConcatenate(filter, pszEscapedRowId);
                        filter = msStringConcatenate(filter, "\"");
                        msFree(pszEscapedRowId);
                    }
                    else
                        filter = msStringConcatenate(filter, "ROWID");
                    
                    filter = msStringConcatenate(filter, " IN ");
                    filter = msStringConcatenate(filter, "(");
                    filter = msStringConcatenate(filter, "SELECT ");

                    if( EQUAL(psInfo->dialect, "Spatialite") )
                        filter = msStringConcatenate(filter, "ms_spat_idx.pkid");
                    else
                        filter = msStringConcatenate(filter, "ms_spat_idx.id");

                    filter = msStringConcatenate(filter, " FROM ");

                    char szSpatialIndexName[256];
                    snprintf( szSpatialIndexName, sizeof(szSpatialIndexName),
                                "%s_%s_%s",
                                EQUAL(psInfo->dialect, "Spatialite") ? "idx" : "rtree",
                                psInfo->pszSpatialFilterTableName,
                                psInfo->pszSpatialFilterGeometryColumn );
                    char* pszEscapedSpatialIndexName = msLayerEscapePropertyName(
                                                    layer, szSpatialIndexName);

                    filter = msStringConcatenate(filter, "\"");
                    filter = msStringConcatenate(filter, pszEscapedSpatialIndexName);
                    msFree(pszEscapedSpatialIndexName);
                    
                    filter = msStringConcatenate(filter, "\" ms_spat_idx WHERE ");

                    char szCond[256];
                    if( EQUAL(psInfo->dialect, "Spatialite") )
                    {
                        snprintf(szCond, sizeof(szCond),
                                "ms_spat_idx.xmin <= %.15g AND ms_spat_idx.xmax >= %.15g AND "
                                "ms_spat_idx.ymin <= %.15g AND ms_spat_idx.ymax >= %.15g",
                                rect.maxx, rect.minx, rect.maxy, rect.miny);
                    }
                    else
                    {
                        snprintf(szCond, sizeof(szCond),
                                "ms_spat_idx.minx <= %.15g AND ms_spat_idx.maxx >= %.15g AND "
                                "ms_spat_idx.miny <= %.15g AND ms_spat_idx.maxy >= %.15g",
                                rect.maxx, rect.minx, rect.maxy, rect.miny);
                    }
                    filter = msStringConcatenate(filter, szCond);
        
                    filter = msStringConcatenate(filter, ")");

                    bSpatialiteOrGPKGAddOrderByFID = true;
                }

                const bool isGPKG = EQUAL(psInfo->dialect, "GPKG");
                if (filter) filter = msStringConcatenate(filter, " AND");
                const char *col = OGR_L_GetGeometryColumn(psInfo->hLayer); // which geom field??
                filter = msStringConcatenate(filter, " Intersects(");
                if( isGPKG )
                {
                    // Casting GeoPackage geometries to spatialie ones is done
                    // automatically normally, since GDAL enables the
                    // "amphibious" mode, but without it
                    // explicilty specified, spatialite 4.3.0a does an
                    // out-of-bounds access.
                    filter = msStringConcatenate(filter, "GeomFromGPB(");
                }
                filter = msStringConcatenate(filter, "\"");
                char* escaped = msLayerEscapePropertyName(layer, col);
                filter = msStringConcatenate(filter, escaped);
                msFree(escaped);
                filter = msStringConcatenate(filter, "\"");
                if( isGPKG )
                    filter = msStringConcatenate(filter, ")");
                char *points = (char *)msSmallMalloc(30*2*5);
                if( rect.minx == rect.maxx && rect.miny == rect.maxy ) {
                  filter = msStringConcatenate(filter, ",  ST_GeomFromText(");
                  snprintf(points, 30*4, "'POINT(%lf %lf)'", rect.minx, rect.miny);
                } else {
                  filter = msStringConcatenate(filter, ", BuildMbr(");
                  snprintf(points, 30*4, "%lf,%lf,%lf,%lf", rect.minx, rect.miny, rect.maxx, rect.maxy);
                }
                filter = msStringConcatenate(filter, points);
                msFree(points);
                filter = msStringConcatenate(filter, "))");
            }
        }

        /* get sortBy */
        char *sort = NULL;
        if( layer->sortBy.nProperties > 0) {

            char *strOrderBy = msOGRLayerBuildSQLOrderBy(layer, psInfo);
            if (strOrderBy) {
                if( psInfo->nLayerIndex == -1 ) {
                    if( strcasestr(psInfo->pszLayerDef, " ORDER BY ") == NULL )
                        sort = msStringConcatenate(sort, " ORDER BY ");
                    else
                        sort = msStringConcatenate(sort, ", ");
                } else {
                    sort = msStringConcatenate(sort, " ORDER BY ");
                }
                sort = msStringConcatenate(sort, strOrderBy);
                msFree(strOrderBy);
            }
        }

        if( bSpatialiteOrGPKGAddOrderByFID )
        {
            if( sort == NULL )
                sort = msStringConcatenate(NULL, " ORDER BY ");
            else
                sort = msStringConcatenate(sort, ", ");
            char* pszEscapedMainTableName = msLayerEscapePropertyName(
                                            layer, psInfo->pszMainTableName);
            sort = msStringConcatenate(sort, "\"");
            sort = msStringConcatenate(sort, pszEscapedMainTableName);
            sort = msStringConcatenate(sort, "\".");
            msFree(pszEscapedMainTableName);
            if( psInfo->pszRowId )
            {
                char* pszEscapedRowId = msLayerEscapePropertyName(
                                                    layer, psInfo->pszRowId);
                sort = msStringConcatenate(sort, "\"");
                sort = msStringConcatenate(sort, pszEscapedRowId);
                sort = msStringConcatenate(sort, "\"");
                msFree(pszEscapedRowId);
            }
            else
                sort = msStringConcatenate(sort, "ROWID");
        }

        // compose SQL
        if (filter) {
            select = msStringConcatenate(select, " WHERE ");
            select = msStringConcatenate(select, filter);
            msFree(filter);
        }
        if (sort) {
            select = msStringConcatenate(select, " ");
            select = msStringConcatenate(select, sort);
            msFree(sort);
        }

        if ( psInfo->bPaging && layer->maxfeatures >= 0 ) {
            char szLimit[50];
            snprintf(szLimit, sizeof(szLimit), " LIMIT %d", layer->maxfeatures);
            select = msStringConcatenate(select, szLimit);
        }

        if ( psInfo->bPaging && layer->startindex > 0 ) {
            char szOffset[50];
            snprintf(szOffset, sizeof(szOffset), " OFFSET %d", layer->startindex-1);
            select = msStringConcatenate(select, szOffset);
        }

        if( layer->debug )
            msDebug("msOGRFileWhichShapes: SQL = %s.\n", select);

        ACQUIRE_OGR_LOCK;
        if( psInfo->nLayerIndex == -1 && psInfo->hLayer != NULL )
        {
          OGR_DS_ReleaseResultSet( psInfo->hDS, psInfo->hLayer );
        }

        OGRGeometryH hGeom = NULL;
        if( psInfo->dialect == NULL &&
            bHasGeometry && bIsValidRect ) {
            if (rect.minx == rect.maxx && rect.miny == rect.maxy) {
                hGeom = OGR_G_CreateGeometry( wkbPoint );
                OGR_G_SetPoint_2D( hGeom, 0, rect.minx, rect.miny );
            } else if (rect.minx == rect.maxx || rect.miny == rect.maxy) {
                hGeom = OGR_G_CreateGeometry( wkbLineString );
                OGR_G_AddPoint_2D( hGeom, rect.minx, rect.miny );
                OGR_G_AddPoint_2D( hGeom, rect.maxx, rect.maxy );
            } else {
                hGeom = OGR_G_CreateGeometry( wkbPolygon );
                OGRGeometryH hRing = OGR_G_CreateGeometry( wkbLinearRing );

                OGR_G_AddPoint_2D( hRing, rect.minx, rect.miny);
                OGR_G_AddPoint_2D( hRing, rect.maxx, rect.miny);
                OGR_G_AddPoint_2D( hRing, rect.maxx, rect.maxy);
                OGR_G_AddPoint_2D( hRing, rect.minx, rect.maxy);
                OGR_G_AddPoint_2D( hRing, rect.minx, rect.miny);
                OGR_G_AddGeometryDirectly( hGeom, hRing );
            }

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
            {
                msDebug("msOGRFileWhichShapes: Setting spatial filter to %.15g %.15g %.15g %.15g\n",
                        rect.minx, rect.miny, rect.maxx, rect.maxy );
            }
        }

        psInfo->hLayer = OGR_DS_ExecuteSQL( psInfo->hDS, select, hGeom, NULL );
        psInfo->nLayerIndex = -1;
        if( hGeom != NULL )
            OGR_G_DestroyGeometry(hGeom);

        if( psInfo->hLayer == NULL ) {
            RELEASE_OGR_LOCK;
            msSetError(MS_OGRERR, "ExecuteSQL() failed. Check logs.", "msOGRFileWhichShapes()");
            msDebug("ExecuteSQL(%s) failed.\n%s\n", select, CPLGetLastErrorMsg());
            msFree(select);
            return MS_FAILURE;
        }

        // Update itemindexes / layer->iteminfo
        msOGRLayerInitItemInfo(layer);
    } 
    else {

        // case of 1) GetLayer + SetFilter

        char *pszOGRFilter = NULL;
        if (msLayerGetProcessingKey(layer, "NATIVE_FILTER") != NULL) {
            pszOGRFilter = msStringConcatenate(pszOGRFilter, "(");
            pszOGRFilter = msStringConcatenate(pszOGRFilter, msLayerGetProcessingKey(layer, "NATIVE_FILTER"));
            pszOGRFilter = msStringConcatenate(pszOGRFilter, ")");
        }

        if( psInfo->pszWHERE )
        {
            if( pszOGRFilter )
            {
                pszOGRFilter = msStringConcatenate(pszOGRFilter, " AND (");
                pszOGRFilter = msStringConcatenate(pszOGRFilter, psInfo->pszWHERE);
                pszOGRFilter = msStringConcatenate(pszOGRFilter, ")");
            }
            else
            {
                pszOGRFilter = msStringConcatenate(pszOGRFilter, psInfo->pszWHERE);
            }
        }

        ACQUIRE_OGR_LOCK;

        if( OGR_L_GetGeomType( psInfo->hLayer ) != wkbNone && bIsValidRect ) {
            if (rect.minx == rect.maxx && rect.miny == rect.maxy) {
                OGRGeometryH hSpatialFilterPoint = OGR_G_CreateGeometry( wkbPoint );

                OGR_G_SetPoint_2D( hSpatialFilterPoint, 0, rect.minx, rect.miny );    
                OGR_L_SetSpatialFilter( psInfo->hLayer, hSpatialFilterPoint );
                OGR_G_DestroyGeometry( hSpatialFilterPoint );
            } else if (rect.minx == rect.maxx || rect.miny == rect.maxy) {
                OGRGeometryH hSpatialFilterLine = OGR_G_CreateGeometry( wkbLineString );

                OGR_G_AddPoint_2D( hSpatialFilterLine, rect.minx, rect.miny );
                OGR_G_AddPoint_2D( hSpatialFilterLine, rect.maxx, rect.maxy );
                OGR_L_SetSpatialFilter( psInfo->hLayer, hSpatialFilterLine );
                OGR_G_DestroyGeometry( hSpatialFilterLine );
            } else {
                OGRGeometryH hSpatialFilterPolygon = OGR_G_CreateGeometry( wkbPolygon );
                OGRGeometryH hRing = OGR_G_CreateGeometry( wkbLinearRing );

                OGR_G_AddPoint_2D( hRing, rect.minx, rect.miny);
                OGR_G_AddPoint_2D( hRing, rect.maxx, rect.miny);
                OGR_G_AddPoint_2D( hRing, rect.maxx, rect.maxy);
                OGR_G_AddPoint_2D( hRing, rect.minx, rect.maxy);
                OGR_G_AddPoint_2D( hRing, rect.minx, rect.miny);
                OGR_G_AddGeometryDirectly( hSpatialFilterPolygon, hRing );
                OGR_L_SetSpatialFilter( psInfo->hLayer, hSpatialFilterPolygon );
                OGR_G_DestroyGeometry( hSpatialFilterPolygon );
            }

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
            {
                msDebug("msOGRFileWhichShapes: Setting spatial filter to %.15g %.15g %.15g %.15g\n", rect.minx, rect.miny, rect.maxx, rect.maxy );
            }
        }

        psInfo->rect = rect;

        /* ------------------------------------------------------------------
         * Apply an attribute filter if we have one prefixed with a WHERE
         * keyword in the filter string.  Otherwise, ensure the attribute
         * filter is clear.
         * ------------------------------------------------------------------ */
        if( pszOGRFilter != NULL ) {

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug("msOGRFileWhichShapes: Setting attribute filter to %s\n", pszOGRFilter );

            CPLErrorReset();
            if( OGR_L_SetAttributeFilter( psInfo->hLayer, pszOGRFilter ) != OGRERR_NONE ) {
                msSetError(MS_OGRERR, "SetAttributeFilter() failed on layer %s. Check logs.", "msOGRFileWhichShapes()", layer->name?layer->name:"(null)");
                msDebug("SetAttributeFilter(%s) failed on layer %s.\n%s\n", pszOGRFilter, layer->name?layer->name:"(null)", CPLGetLastErrorMsg() );
                RELEASE_OGR_LOCK;
                msFree(pszOGRFilter);
                msFree(select);
                return MS_FAILURE;
            }
            msFree(pszOGRFilter);
        } else
            OGR_L_SetAttributeFilter( psInfo->hLayer, NULL );
        
    }

    msFree(select);

    /* ------------------------------------------------------------------
     * Reset current feature pointer
     * ------------------------------------------------------------------ */
    OGR_L_ResetReading( psInfo->hLayer );
    psInfo->last_record_index_read = -1;
    
    RELEASE_OGR_LOCK;
  
    return MS_SUCCESS;
}

/**********************************************************************
 *                     msOGRPassThroughFieldDefinitions()
 *
 * Pass the field definitions through to the layer metadata in the
 * "gml_[item]_{type,width,precision}" set of metadata items for
 * defining fields.
 **********************************************************************/

static void
msOGRPassThroughFieldDefinitions( layerObj *layer, msOGRFileInfo *psInfo )

{
  OGRFeatureDefnH hDefn = OGR_L_GetLayerDefn( psInfo->hLayer );
  int numitems, i;

  numitems = OGR_FD_GetFieldCount( hDefn );

  for(i=0; i<numitems; i++) {
    OGRFieldDefnH hField = OGR_FD_GetFieldDefn( hDefn, i );
    char gml_width[32], gml_precision[32];
    const char *gml_type = NULL;
    const char *item = OGR_Fld_GetNameRef( hField );

    gml_width[0] = '\0';
    gml_precision[0] = '\0';

    switch( OGR_Fld_GetType( hField ) ) {
      case OFTInteger:
        gml_type = "Integer";
        if( OGR_Fld_GetWidth( hField) > 0 )
          sprintf( gml_width, "%d", OGR_Fld_GetWidth( hField) );
        break;

#if GDAL_VERSION_MAJOR >= 2
      case OFTInteger64:
        gml_type = "Long";
        if( OGR_Fld_GetWidth( hField) > 0 )
          sprintf( gml_width, "%d", OGR_Fld_GetWidth( hField) );
        break;
#endif

      case OFTReal:
        gml_type = "Real";
        if( OGR_Fld_GetWidth( hField) > 0 )
          sprintf( gml_width, "%d", OGR_Fld_GetWidth( hField) );
        if( OGR_Fld_GetPrecision( hField ) > 0 )
          sprintf( gml_precision, "%d", OGR_Fld_GetPrecision( hField) );
        break;

      case OFTString:
        gml_type = "Character";
        if( OGR_Fld_GetWidth( hField) > 0 )
          sprintf( gml_width, "%d", OGR_Fld_GetWidth( hField) );
        break;

      case OFTDate:
        gml_type = "Date";
        break;
      case OFTTime:
        gml_type = "Time";
        break;
      case OFTDateTime:
        gml_type = "DateTime";
        break;

      default:
        gml_type = "Character";
        break;
    }

    msUpdateGMLFieldMetadata(layer, item, gml_type, gml_width, gml_precision, 0);
  }

  /* Should we try to address style items, or other special items? */
}

/**********************************************************************
 *                     msOGRFileGetItems()
 *
 * Returns a list of field names in a NULL terminated list of strings.
 **********************************************************************/
static char **msOGRFileGetItems(layerObj *layer, msOGRFileInfo *psInfo )
{
  OGRFeatureDefnH hDefn;
  int i, numitems,totalnumitems;
  int numStyleItems = MSOGR_LABELNUMITEMS;
  char **items;
  const char *getShapeStyleItems, *value;

  if((hDefn = OGR_L_GetLayerDefn( psInfo->hLayer )) == NULL) {
    msSetError(MS_OGRERR,
               "OGR Connection for layer `%s' contains no field definition.",
               "msOGRFileGetItems()",
               layer->name?layer->name:"(null)" );
    return NULL;
  }

  totalnumitems = numitems = OGR_FD_GetFieldCount( hDefn );

  getShapeStyleItems = msLayerGetProcessingKey( layer, "GETSHAPE_STYLE_ITEMS" );
  if (getShapeStyleItems && EQUAL(getShapeStyleItems, "all"))
    totalnumitems += numStyleItems;

  if((items = (char**)malloc(sizeof(char *)*(totalnumitems+1))) == NULL) {
    msSetError(MS_MEMERR, NULL, "msOGRFileGetItems()");
    return NULL;
  }

  for(i=0; i<numitems; i++) {
    OGRFieldDefnH hField = OGR_FD_GetFieldDefn( hDefn, i );
    items[i] = msStrdup( OGR_Fld_GetNameRef( hField ));
  }

  if (getShapeStyleItems && EQUAL(getShapeStyleItems, "all")) {
    assert(numStyleItems == 21);
    items[i++] = msStrdup( MSOGR_LABELFONTNAMENAME );
    items[i++] = msStrdup( MSOGR_LABELSIZENAME );
    items[i++] = msStrdup( MSOGR_LABELTEXTNAME );
    items[i++] = msStrdup( MSOGR_LABELANGLENAME );
    items[i++] = msStrdup( MSOGR_LABELFCOLORNAME );
    items[i++] = msStrdup( MSOGR_LABELBCOLORNAME );
    items[i++] = msStrdup( MSOGR_LABELPLACEMENTNAME );
    items[i++] = msStrdup( MSOGR_LABELANCHORNAME );
    items[i++] = msStrdup( MSOGR_LABELDXNAME );
    items[i++] = msStrdup( MSOGR_LABELDYNAME );
    items[i++] = msStrdup( MSOGR_LABELPERPNAME );
    items[i++] = msStrdup( MSOGR_LABELBOLDNAME );
    items[i++] = msStrdup( MSOGR_LABELITALICNAME );
    items[i++] = msStrdup( MSOGR_LABELUNDERLINENAME );
    items[i++] = msStrdup( MSOGR_LABELPRIORITYNAME );
    items[i++] = msStrdup( MSOGR_LABELSTRIKEOUTNAME );
    items[i++] = msStrdup( MSOGR_LABELSTRETCHNAME );
    items[i++] = msStrdup( MSOGR_LABELADJHORNAME );
    items[i++] = msStrdup( MSOGR_LABELADJVERTNAME );
    items[i++] = msStrdup( MSOGR_LABELHCOLORNAME );
    items[i++] = msStrdup( MSOGR_LABELOCOLORNAME );
  }
  items[i++] = NULL;

  /* -------------------------------------------------------------------- */
  /*      consider populating the field definitions in metadata.          */
  /* -------------------------------------------------------------------- */
  if((value = msOWSLookupMetadata(&(layer->metadata), "G", "types")) != NULL
      && strcasecmp(value,"auto") == 0 )
    msOGRPassThroughFieldDefinitions( layer, psInfo );

  return items;
}

/**********************************************************************
 *                     msOGRFileNextShape()
 *
 * Returns shape sequentially from OGR data source.
 * msOGRLayerWhichShape() must have been called first.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
static int
msOGRFileNextShape(layerObj *layer, shapeObj *shape,
                   msOGRFileInfo *psInfo )
{
  OGRFeatureH hFeature = NULL;

  if (psInfo == NULL || psInfo->hLayer == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!",
               "msOGRFileNextShape()");
    return(MS_FAILURE);
  }

  /* ------------------------------------------------------------------
   * Read until we find a feature that matches attribute filter and
   * whose geometry is compatible with current layer type.
   * ------------------------------------------------------------------ */
  msFreeShape(shape);
  shape->type = MS_SHAPE_NULL;

  ACQUIRE_OGR_LOCK;
  while (shape->type == MS_SHAPE_NULL) {
    if( hFeature )
      OGR_F_Destroy( hFeature );

    if( (hFeature = OGR_L_GetNextFeature( psInfo->hLayer )) == NULL ) {
      psInfo->last_record_index_read = -1;
      if( CPLGetLastErrorType() == CE_Failure ) {
        msSetError(MS_OGRERR, "OGR GetNextFeature() error'd. Check logs.",
                   "msOGRFileNextShape()");
        msDebug("msOGRFileNextShape(): %s\n",
                CPLGetLastErrorMsg() );
        RELEASE_OGR_LOCK;
        return MS_FAILURE;
      } else {
        RELEASE_OGR_LOCK;
        if (layer->debug >= MS_DEBUGLEVEL_VV)
          msDebug("msOGRFileNextShape: Returning MS_DONE (no more shapes)\n" );
        return MS_DONE;  // No more features to read
      }
    }

    psInfo->last_record_index_read++;

    if(layer->numitems > 0) {
      if (shape->values) msFreeCharArray(shape->values, shape->numvalues);
      shape->values = msOGRGetValues(layer, hFeature);
      shape->numvalues = layer->numitems;
      if(!shape->values) {
        OGR_F_Destroy( hFeature );
        RELEASE_OGR_LOCK;
        return(MS_FAILURE);
      }
    }

    // Feature matched filter expression... process geometry
    // shape->type will be set if geom is compatible with layer type
    if (ogrConvertGeometry(ogrGetLinearGeometry( hFeature ), shape,
                           layer->type) == MS_SUCCESS) {
      if (shape->type != MS_SHAPE_NULL)
        break; // Shape is ready to be returned!

      if (layer->debug >= MS_DEBUGLEVEL_VVV)
        msDebug("msOGRFileNextShape: Rejecting feature (shapeid = " CPL_FRMT_GIB ", tileid=%d) of incompatible type for this layer (feature wkbType %d, layer type %d)\n",
                (GIntBig)OGR_F_GetFID( hFeature ), psInfo->nTileId,
                OGR_F_GetGeometryRef( hFeature )==NULL ? wkbFlatten(wkbUnknown):wkbFlatten( OGR_G_GetGeometryType( OGR_F_GetGeometryRef( hFeature ) ) ),
                layer->type);

    } else {
      msFreeShape(shape);
      OGR_F_Destroy( hFeature );
      RELEASE_OGR_LOCK;
      return MS_FAILURE; // Error message already produced.
    }

    // Feature rejected... free shape to clear attributes values.
    msFreeShape(shape);
    shape->type = MS_SHAPE_NULL;
  }

  shape->index =  (int)OGR_F_GetFID( hFeature ); // FIXME? GetFID() is a 64bit integer in GDAL 2.0
  shape->resultindex = psInfo->last_record_index_read;
  shape->tileindex = psInfo->nTileId;

  if (layer->debug >= MS_DEBUGLEVEL_VVV)
    msDebug("msOGRFileNextShape: Returning shape=%ld, tile=%d\n",
            shape->index, shape->tileindex );

  // Keep ref. to last feature read in case we need style info.
  if (psInfo->hLastFeature)
    OGR_F_Destroy( psInfo->hLastFeature );
  psInfo->hLastFeature = hFeature;

  RELEASE_OGR_LOCK;

  return MS_SUCCESS;
}

/**********************************************************************
 *                     msOGRFileGetShape()
 *
 * Returns shape from OGR data source by id.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
static int
msOGRFileGetShape(layerObj *layer, shapeObj *shape, long record,
                  msOGRFileInfo *psInfo, int record_is_fid )
{
  OGRFeatureH hFeature;

  if (psInfo == NULL || psInfo->hLayer == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!",
               "msOGRFileNextShape()");
    return(MS_FAILURE);
  }

  /* -------------------------------------------------------------------- */
  /*      Clear previously loaded shape.                                  */
  /* -------------------------------------------------------------------- */
  msFreeShape(shape);
  shape->type = MS_SHAPE_NULL;

  /* -------------------------------------------------------------------- */
  /*      Support reading feature by fid.                                 */
  /* -------------------------------------------------------------------- */
  if( record_is_fid ) {
    ACQUIRE_OGR_LOCK;
    if( (hFeature = OGR_L_GetFeature( psInfo->hLayer, record )) == NULL ) {
      RELEASE_OGR_LOCK;
      return MS_FAILURE;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Support reading shape by offset within the current              */
  /*      resultset.                                                      */
  /* -------------------------------------------------------------------- */
  else {
    ACQUIRE_OGR_LOCK;
    if( record <= psInfo->last_record_index_read
        || psInfo->last_record_index_read == -1 ) {
      OGR_L_ResetReading( psInfo->hLayer );
      psInfo->last_record_index_read = -1;
    }

    hFeature = NULL;
    while( psInfo->last_record_index_read < record ) {
      if( hFeature != NULL ) {
        OGR_F_Destroy( hFeature );
        hFeature = NULL;
      }
      if( (hFeature = OGR_L_GetNextFeature( psInfo->hLayer )) == NULL ) {
        RELEASE_OGR_LOCK;
        return MS_FAILURE;
      }
      psInfo->last_record_index_read++;
    }
  }

  /* ------------------------------------------------------------------
   * Handle shape geometry...
   * ------------------------------------------------------------------ */
  // shape->type will be set if geom is compatible with layer type
  if (ogrConvertGeometry(ogrGetLinearGeometry( hFeature ), shape,
                         layer->type) != MS_SUCCESS) {
    RELEASE_OGR_LOCK;
    return MS_FAILURE; // Error message already produced.
  }

  if (shape->type == MS_SHAPE_NULL) {
    msSetError(MS_OGRERR,
               "Requested feature is incompatible with layer type",
               "msOGRLayerGetShape()");
    RELEASE_OGR_LOCK;
    return MS_FAILURE;
  }

  /* ------------------------------------------------------------------
   * Process shape attributes
   * ------------------------------------------------------------------ */
  if(layer->numitems > 0) {
    shape->values = msOGRGetValues(layer, hFeature);
    shape->numvalues = layer->numitems;
    if(!shape->values) {
      RELEASE_OGR_LOCK;
      return(MS_FAILURE);
    }

  }

  if (record_is_fid) {
    shape->index = record;
    shape->resultindex = -1;
  } else {
    shape->index = (int)OGR_F_GetFID( hFeature ); // FIXME? GetFID() is a 64bit integer in GDAL 2.0
    shape->resultindex = record;
  }

  shape->tileindex = psInfo->nTileId;

  // Keep ref. to last feature read in case we need style info.
  if (psInfo->hLastFeature)
    OGR_F_Destroy( psInfo->hLastFeature );
  psInfo->hLastFeature = hFeature;

  RELEASE_OGR_LOCK;

  return MS_SUCCESS;
}

/************************************************************************/
/*                         msOGRFileReadTile()                          */
/*                                                                      */
/*      Advance to the next tile (or if targetTile is not -1 advance    */
/*      to that tile), causing the tile to become the poCurTile in      */
/*      the tileindexes psInfo structure.  Returns MS_DONE if there     */
/*      are no more available tiles.                                    */
/*                                                                      */
/*      Newly loaded tiles are automatically "WhichShaped" based on     */
/*      the current rectangle.                                          */
/************************************************************************/

int msOGRFileReadTile( layerObj *layer, msOGRFileInfo *psInfo,
                       int targetTile = -1 )

{
  int nFeatureId;

  /* -------------------------------------------------------------------- */
  /*      Close old tile if one is open.                                  */
  /* -------------------------------------------------------------------- */
  if( psInfo->poCurTile != NULL ) {
    msOGRFileClose( layer, psInfo->poCurTile );
    psInfo->poCurTile = NULL;
  }

  /* -------------------------------------------------------------------- */
  /*      If -2 is passed, then seek reset reading of the tileindex.      */
  /*      We want to start from the beginning even if this file is        */
  /*      shared between layers or renders.                               */
  /* -------------------------------------------------------------------- */
  ACQUIRE_OGR_LOCK;
  if( targetTile == -2 ) {
    OGR_L_ResetReading( psInfo->hLayer );
  }

  /* -------------------------------------------------------------------- */
  /*      Get the name (connection string really) of the next tile.       */
  /* -------------------------------------------------------------------- */
  OGRFeatureH hFeature;
  char       *connection = NULL;
  msOGRFileInfo *psTileInfo = NULL;
  int status;

#ifndef IGNORE_MISSING_DATA
NextFile:
#endif

  if( targetTile < 0 )
    hFeature = OGR_L_GetNextFeature( psInfo->hLayer );

  else
    hFeature = OGR_L_GetFeature( psInfo->hLayer, targetTile );

  if( hFeature == NULL ) {
    RELEASE_OGR_LOCK;
    if( targetTile == -1 )
      return MS_DONE;
    else
      return MS_FAILURE;

  }

  connection = msStrdup( OGR_F_GetFieldAsString( hFeature,
                         layer->tileitemindex ));

  char* pszSRS = NULL;
  if( layer->tilesrs != NULL )
  {
      int idx = OGR_F_GetFieldIndex( hFeature, layer->tilesrs);
      if( idx >= 0 )
      {
          pszSRS = msStrdup( OGR_F_GetFieldAsString( hFeature, idx ));
      }
  }

  nFeatureId = (int)OGR_F_GetFID( hFeature ); // FIXME? GetFID() is a 64bit integer in GDAL 2.0

  OGR_F_Destroy( hFeature );

  RELEASE_OGR_LOCK;

  /* -------------------------------------------------------------------- */
  /*      Open the new tile file.                                         */
  /* -------------------------------------------------------------------- */
  psTileInfo = msOGRFileOpen( layer, connection );

  free( connection );

#ifndef IGNORE_MISSING_DATA
  if( psTileInfo == NULL && targetTile == -1 )
  {
    msFree(pszSRS);
    goto NextFile;
  }
#endif

  if( psTileInfo == NULL )
  {
    msFree(pszSRS);
    return MS_FAILURE;
  }
  
  if( pszSRS != NULL )
  {
    if( msOGCWKT2ProjectionObj( pszSRS, &(psInfo->sTileProj),
                                layer->debug ) != MS_SUCCESS )
    {
        msFree(pszSRS);
        return MS_FAILURE;
    }
    msFree(pszSRS);
  }

  psTileInfo->nTileId = nFeatureId;

  /* -------------------------------------------------------------------- */
  /*      Initialize the spatial query on this file.                      */
  /* -------------------------------------------------------------------- */
  if( psInfo->rect.minx != 0 || psInfo->rect.maxx != 0 ) {
    rectObj rect = psInfo->rect;

    if( layer->tileindex != NULL && psInfo->sTileProj.numargs > 0 )
    {
      msProjectRect(&(layer->projection), &(psInfo->sTileProj), &rect);
    }

    status = msOGRFileWhichShapes( layer, rect, psTileInfo );
    if( status != MS_SUCCESS )
      return status;
  }

  psInfo->poCurTile = psTileInfo;

  /* -------------------------------------------------------------------- */
  /*      Update the iteminfo in case this layer has a different field    */
  /*      list.                                                           */
  /* -------------------------------------------------------------------- */
  msOGRLayerInitItemInfo( layer );

  return MS_SUCCESS;
}

/************************************************************************/
/*                               msExprNode                             */
/************************************************************************/

class msExprNode
{
    public:
        std::vector<std::unique_ptr<msExprNode>> m_aoChildren{};
        int         m_nToken = 0;
        std::string m_osVal{};
        double      m_dfVal = 0;
        struct tm   m_tmVal{};
};


/************************************************************************/
/*                        exprGetPriority()                             */
/************************************************************************/

static int exprGetPriority(int token)
{
    if (token == MS_TOKEN_LOGICAL_NOT)
        return 9;
    else if (token == '*' || token == '/' || token == '%' )
        return 8;
    else if (token == '+' || token == '-' )
        return 7;
    else if (token == MS_TOKEN_COMPARISON_GE ||
             token == MS_TOKEN_COMPARISON_GT ||
             token == MS_TOKEN_COMPARISON_LE ||
             token == MS_TOKEN_COMPARISON_LT ||
             token == MS_TOKEN_COMPARISON_IN)
        return 6;
    else if (token == MS_TOKEN_COMPARISON_EQ ||
             token == MS_TOKEN_COMPARISON_IEQ ||
             token == MS_TOKEN_COMPARISON_LIKE ||
             token == MS_TOKEN_COMPARISON_RE ||
             token == MS_TOKEN_COMPARISON_IRE ||
             token == MS_TOKEN_COMPARISON_NE)
        return 5;
    else if (token == MS_TOKEN_LOGICAL_AND)
        return 4;
    else if (token == MS_TOKEN_LOGICAL_OR)
        return 3;
    else
        return 0;
}

/************************************************************************/
/*                           BuildExprTree()                            */
/************************************************************************/

static std::unique_ptr<msExprNode> BuildExprTree(tokenListNodeObjPtr node,
                                 tokenListNodeObjPtr* pNodeNext,
                                 int nParenthesisLevel)
{
    std::vector<std::unique_ptr<msExprNode>> aoStackOp, aoStackVal;
    while( node != NULL )
    {
        if( node->token == '(' )
        {
            auto subExpr = BuildExprTree(node->next, &node,
                                                nParenthesisLevel + 1);
            if( subExpr == NULL )
            {
                return nullptr;
            }
            aoStackVal.emplace_back(std::move(subExpr));
            continue;
        }
        else if( node->token == ')' )
        {
            if( nParenthesisLevel > 0 )
            {
                break;
            }
            return nullptr;
        }
        else if( node->token == '+' ||
                 node->token == '-' ||
                 node->token == '*' ||
                 node->token == '/' ||
                 node->token == '%' ||
                 node->token == MS_TOKEN_LOGICAL_NOT ||
                 node->token == MS_TOKEN_LOGICAL_AND ||
                 node->token == MS_TOKEN_LOGICAL_OR  ||
                 node->token == MS_TOKEN_COMPARISON_GE ||
                 node->token == MS_TOKEN_COMPARISON_GT ||
                 node->token == MS_TOKEN_COMPARISON_LE ||
                 node->token == MS_TOKEN_COMPARISON_LT ||
                 node->token == MS_TOKEN_COMPARISON_EQ ||
                 node->token == MS_TOKEN_COMPARISON_IEQ ||
                 node->token == MS_TOKEN_COMPARISON_LIKE ||
                 node->token == MS_TOKEN_COMPARISON_NE ||
                 node->token == MS_TOKEN_COMPARISON_RE ||
                 node->token == MS_TOKEN_COMPARISON_IRE ||
                 node->token == MS_TOKEN_COMPARISON_IN )
        {
            while( !aoStackOp.empty() &&
                   exprGetPriority(node->token) <=
                        exprGetPriority(aoStackOp.back()->m_nToken))
            {
                std::unique_ptr<msExprNode> val1;
                std::unique_ptr<msExprNode> val2;
                if (aoStackOp.back()->m_nToken != MS_TOKEN_LOGICAL_NOT)
                {
                    if( aoStackVal.empty() )
                        return nullptr;
                    val2.reset(aoStackVal.back().release());
                    aoStackVal.pop_back();
                }
                if( aoStackVal.empty() )
                    return nullptr;
                val1.reset(aoStackVal.back().release());
                aoStackVal.pop_back();

                std::unique_ptr<msExprNode> newNode(new msExprNode);
                newNode->m_nToken = aoStackOp.back()->m_nToken;
                newNode->m_aoChildren.emplace_back(std::move(val1));
                if( val2 )
                    newNode->m_aoChildren.emplace_back(std::move(val2));
                aoStackVal.emplace_back(std::move(newNode));
                aoStackOp.pop_back();
            }

            std::unique_ptr<msExprNode> newNode(new msExprNode);
            newNode->m_nToken = node->token;
            aoStackOp.emplace_back(std::move(newNode));
        }
        else if( node->token == ',' )
        {
        }
        else if( node->token == MS_TOKEN_COMPARISON_INTERSECTS ||
                 node->token == MS_TOKEN_COMPARISON_DISJOINT ||
                 node->token == MS_TOKEN_COMPARISON_TOUCHES ||
                 node->token == MS_TOKEN_COMPARISON_OVERLAPS ||
                 node->token == MS_TOKEN_COMPARISON_CROSSES ||
                 node->token == MS_TOKEN_COMPARISON_DWITHIN ||
                 node->token == MS_TOKEN_COMPARISON_BEYOND ||
                 node->token == MS_TOKEN_COMPARISON_WITHIN ||
                 node->token == MS_TOKEN_COMPARISON_CONTAINS ||
                 node->token == MS_TOKEN_COMPARISON_EQUALS ||
                 node->token == MS_TOKEN_FUNCTION_LENGTH ||
                 node->token == MS_TOKEN_FUNCTION_TOSTRING ||
                 node->token == MS_TOKEN_FUNCTION_COMMIFY ||
                 node->token == MS_TOKEN_FUNCTION_AREA ||
                 node->token == MS_TOKEN_FUNCTION_ROUND ||
                 node->token == MS_TOKEN_FUNCTION_FROMTEXT ||
                 node->token == MS_TOKEN_FUNCTION_BUFFER ||
                 node->token == MS_TOKEN_FUNCTION_DIFFERENCE ||
                 node->token == MS_TOKEN_FUNCTION_SIMPLIFY ||
                 node->token == MS_TOKEN_FUNCTION_SIMPLIFYPT ||
                 node->token == MS_TOKEN_FUNCTION_GENERALIZE ||
                 node->token == MS_TOKEN_FUNCTION_SMOOTHSIA ||
                 node->token == MS_TOKEN_FUNCTION_JAVASCRIPT ||
                 node->token == MS_TOKEN_FUNCTION_UPPER ||
                 node->token == MS_TOKEN_FUNCTION_LOWER ||
                 node->token == MS_TOKEN_FUNCTION_INITCAP ||
                 node->token == MS_TOKEN_FUNCTION_FIRSTCAP )
        {
            if( node->next && node->next->token == '(' )
            {
                int node_token = node->token;
                auto subExpr = BuildExprTree(node->next->next, &node,
                                                    nParenthesisLevel + 1);
                if( subExpr == NULL )
                {
                    return nullptr;
                }
                std::unique_ptr<msExprNode> newNode(new msExprNode);
                newNode->m_nToken = node_token;
                if( subExpr->m_nToken == 0 )
                {
                    newNode->m_aoChildren = std::move(subExpr->m_aoChildren);
                }
                else
                {
                    newNode->m_aoChildren.emplace_back(std::move(subExpr));
                }
                aoStackVal.emplace_back(std::move(newNode));
                continue;
            }
            else
                return nullptr;
        }
        else if( node->token == MS_TOKEN_LITERAL_NUMBER ||
                 node->token == MS_TOKEN_LITERAL_BOOLEAN )
        {
            std::unique_ptr<msExprNode> newNode(new msExprNode);
            newNode->m_nToken = node->token;
            newNode->m_dfVal = node->tokenval.dblval;
            aoStackVal.emplace_back(std::move(newNode));
        }
        else if( node->token == MS_TOKEN_LITERAL_STRING )
        {
            std::unique_ptr<msExprNode> newNode(new msExprNode);
            newNode->m_nToken = node->token;
            newNode->m_osVal = node->tokenval.strval;
            aoStackVal.emplace_back(std::move(newNode));
        }
        else if( node->token == MS_TOKEN_LITERAL_TIME )
        {
            std::unique_ptr<msExprNode> newNode(new msExprNode);
            newNode->m_nToken = node->token;
            newNode->m_tmVal = node->tokenval.tmval;
            aoStackVal.emplace_back(std::move(newNode));
        }
        else if( node->token == MS_TOKEN_LITERAL_SHAPE )
        {
            std::unique_ptr<msExprNode> newNode(new msExprNode);
            newNode->m_nToken = node->token;
            char *wkt = msShapeToWKT(node->tokenval.shpval);
            newNode->m_osVal = wkt;
            msFree(wkt);
            aoStackVal.emplace_back(std::move(newNode));
        }
        else if( node->token == MS_TOKEN_BINDING_DOUBLE ||
                 node->token == MS_TOKEN_BINDING_INTEGER ||
                 node->token == MS_TOKEN_BINDING_STRING ||
                 node->token == MS_TOKEN_BINDING_TIME )
        {
            std::unique_ptr<msExprNode> newNode(new msExprNode);
            newNode->m_nToken = node->token;
            newNode->m_osVal = node->tokenval.bindval.item;
            aoStackVal.emplace_back(std::move(newNode));
        }
        else
        {
            std::unique_ptr<msExprNode> newNode(new msExprNode);
            newNode->m_nToken = node->token;
            aoStackVal.emplace_back(std::move(newNode));
        }

        node = node->next;
    }

    while( !aoStackOp.empty() )
    {
        std::unique_ptr<msExprNode> val1 = NULL;
        std::unique_ptr<msExprNode> val2 = NULL;
        if (aoStackOp.back()->m_nToken != MS_TOKEN_LOGICAL_NOT)
        {
            if( aoStackVal.empty() )
                return nullptr;
            val2.reset(aoStackVal.back().release());
            aoStackVal.pop_back();
        }
        if( aoStackVal.empty() )
            return nullptr;
        val1.reset(aoStackVal.back().release());
        aoStackVal.pop_back();

        std::unique_ptr<msExprNode> newNode(new msExprNode);
        newNode->m_nToken = aoStackOp.back()->m_nToken;
        newNode->m_aoChildren.emplace_back(std::move(val1));
        if( val2 )
            newNode->m_aoChildren.emplace_back(std::move(val2));
        aoStackVal.emplace_back(std::move(newNode));
        aoStackOp.pop_back();
    }

    std::unique_ptr<msExprNode> poRet;
    if( aoStackVal.size() == 1 )
        poRet.reset(aoStackVal.back().release());
    else if( aoStackVal.size() > 1 )
    {
        poRet.reset(new msExprNode);
        poRet->m_aoChildren = std::move(aoStackVal);
    }

    if( pNodeNext )
        *pNodeNext = node ? node->next : NULL;

    return poRet;
}

/**********************************************************************
 *                 msOGRExtractTopSpatialFilter()
 * 
 * Recognize expressions like "Intersects([shape], wkt) == TRUE [AND ....]"
 **********************************************************************/
static int  msOGRExtractTopSpatialFilter( msOGRFileInfo *info,
                                          const msExprNode* expr,
                                          const msExprNode** pSpatialFilterNode )
{
  if( expr == NULL )
      return MS_FALSE;

  if( expr->m_nToken == MS_TOKEN_COMPARISON_EQ &&
      expr->m_aoChildren.size() == 2 &&
      expr->m_aoChildren[1]->m_nToken == MS_TOKEN_LITERAL_BOOLEAN &&
      expr->m_aoChildren[1]->m_dfVal == 1.0 )
  {
      return msOGRExtractTopSpatialFilter(info, expr->m_aoChildren[0].get(),
                                          pSpatialFilterNode);
  }

  if( (((expr->m_nToken == MS_TOKEN_COMPARISON_INTERSECTS ||
      expr->m_nToken == MS_TOKEN_COMPARISON_OVERLAPS ||
      expr->m_nToken == MS_TOKEN_COMPARISON_CROSSES ||
      expr->m_nToken == MS_TOKEN_COMPARISON_WITHIN ||
      expr->m_nToken == MS_TOKEN_COMPARISON_CONTAINS) &&
      expr->m_aoChildren.size() == 2) ||
      (expr->m_nToken == MS_TOKEN_COMPARISON_DWITHIN && expr->m_aoChildren.size() == 3)) &&
      expr->m_aoChildren[1]->m_nToken == MS_TOKEN_LITERAL_SHAPE )
  {
        if( info->rect_is_defined )
        {
            // Several intersects...
            *pSpatialFilterNode = NULL;
            info->rect_is_defined = MS_FALSE;
            return MS_FALSE;
        }
        OGRGeometryH hSpatialFilter = NULL;
        char* wkt = const_cast<char*>(expr->m_aoChildren[1]->m_osVal.c_str());
        OGRErr e = OGR_G_CreateFromWkt(&wkt, NULL, &hSpatialFilter);
        if (e == OGRERR_NONE) {
            OGREnvelope env;
            if( expr->m_nToken == MS_TOKEN_COMPARISON_DWITHIN ) {
                OGRGeometryH hBuffer = OGR_G_Buffer(hSpatialFilter, expr->m_aoChildren[2]->m_dfVal, 30);
                OGR_G_GetEnvelope(hBuffer ? hBuffer : hSpatialFilter, &env);
                OGR_G_DestroyGeometry(hBuffer);
            } else {
                OGR_G_GetEnvelope(hSpatialFilter, &env);
            }
            info->rect.minx = env.MinX;
            info->rect.miny = env.MinY;
            info->rect.maxx = env.MaxX;
            info->rect.maxy = env.MaxY;
            info->rect_is_defined = true;
            *pSpatialFilterNode = expr;
            OGR_G_DestroyGeometry(hSpatialFilter);
            return MS_TRUE;
        }
        return MS_FALSE;
  }

  if( expr->m_nToken == MS_TOKEN_LOGICAL_AND &&
      expr->m_aoChildren.size() == 2 )
  {
      return msOGRExtractTopSpatialFilter(info, expr->m_aoChildren[0].get(),
                                          pSpatialFilterNode) &&
             msOGRExtractTopSpatialFilter(info, expr->m_aoChildren[1].get(),
                                          pSpatialFilterNode);
  }

  return MS_TRUE;
}

/**********************************************************************
 *                 msOGRTranslatePartialMSExpressionToOGRSQL()
 *
 * Tries to partially translate a mapserver expression to SQL
 **********************************************************************/

static std::string msOGRGetTokenText(int nToken)
{
    switch( nToken )
    {
        case '*':
        case '+':
        case '-':
        case '/':
        case '%':
            return std::string(1, static_cast<char>(nToken));

        case MS_TOKEN_COMPARISON_GE: return ">=";
        case MS_TOKEN_COMPARISON_GT: return ">";
        case MS_TOKEN_COMPARISON_LE: return "<=";
        case MS_TOKEN_COMPARISON_LT: return "<";
        case MS_TOKEN_COMPARISON_EQ: return "=";
        case MS_TOKEN_COMPARISON_NE: return "!=";
        case MS_TOKEN_COMPARISON_LIKE: return "LIKE";

        default:
            return std::string();
    }
}

static std::string msOGRTranslatePartialInternal(layerObj* layer,
                                                 const msExprNode* expr,
                                                 const msExprNode* spatialFilterNode,
                                                 bool& bPartialFilter)
{
    switch( expr->m_nToken )
    {
        case MS_TOKEN_LOGICAL_NOT:
        {
            std::string osTmp(msOGRTranslatePartialInternal(
                layer, expr->m_aoChildren[0].get(), spatialFilterNode, bPartialFilter ));
            if( osTmp.empty() )
                return std::string();
            return "(NOT " + osTmp + ")";
        }

        case MS_TOKEN_LOGICAL_AND:
        {
            // We can deal with partially translated children
            std::string osTmp1(msOGRTranslatePartialInternal(
                layer, expr->m_aoChildren[0].get(), spatialFilterNode, bPartialFilter ));
            std::string osTmp2(msOGRTranslatePartialInternal( 
                layer, expr->m_aoChildren[1].get(), spatialFilterNode, bPartialFilter ));
            if( !osTmp1.empty() && !osTmp2.empty() )
            {
                return "(" + osTmp1 + " AND " + osTmp2 + ")";
            }
            else if( !osTmp1.empty() )
                return osTmp1;
            else
                return osTmp2;
        }

        case MS_TOKEN_LOGICAL_OR:
        {
            // We can NOT deal with partially translated children
            std::string osTmp1(msOGRTranslatePartialInternal(
                layer, expr->m_aoChildren[0].get(), spatialFilterNode, bPartialFilter ));
            std::string osTmp2(msOGRTranslatePartialInternal(
                layer, expr->m_aoChildren[1].get(), spatialFilterNode, bPartialFilter ));
            if( !osTmp1.empty() && !osTmp2.empty() )
            {
                return "(" + osTmp1 + " OR " + osTmp2 + ")";
            }
            else
                return std::string();
        }

        case '*':
        case '+':
        case '-':
        case '/':
        case '%':
        case MS_TOKEN_COMPARISON_GE:
        case MS_TOKEN_COMPARISON_GT:
        case MS_TOKEN_COMPARISON_LE:
        case MS_TOKEN_COMPARISON_LT:
        case MS_TOKEN_COMPARISON_EQ:
        case MS_TOKEN_COMPARISON_NE:
        {
            std::string osTmp1(msOGRTranslatePartialInternal(
                layer, expr->m_aoChildren[0].get(), spatialFilterNode, bPartialFilter ));
            std::string osTmp2(msOGRTranslatePartialInternal(
                layer, expr->m_aoChildren[1].get(), spatialFilterNode, bPartialFilter ));
            if( !osTmp1.empty() && !osTmp2.empty() )
            {
                if( expr->m_nToken == MS_TOKEN_COMPARISON_EQ &&
                    osTmp2 == "'_MAPSERVER_NULL_'" )
                {
                    return "(" + osTmp1 + " IS NULL )";
                }
                if( expr->m_aoChildren[1]->m_nToken == MS_TOKEN_LITERAL_STRING )
                {
                    char md_item_name[256];
                    snprintf( md_item_name, sizeof(md_item_name), "gml_%s_type",
                              expr->m_aoChildren[0]->m_osVal.c_str() );
                    const char* type =
                        msLookupHashTable(&(layer->metadata), md_item_name);
                    // Cast if needed (or unsure)
                    if( type == NULL || !EQUAL(type, "Character") )
                    {
                        osTmp1 = "CAST(" + osTmp1 + " AS CHARACTER(4096))";
                    }
                }
                return "(" + osTmp1 + " " + msOGRGetTokenText(expr->m_nToken) +
                       " " + osTmp2 + ")";
            }
            else
                return std::string();
        }

        case MS_TOKEN_COMPARISON_RE:
        {
            std::string osTmp1(msOGRTranslatePartialInternal(
                layer, expr->m_aoChildren[0].get(), spatialFilterNode, bPartialFilter ));
            if( expr->m_aoChildren[1]->m_nToken != MS_TOKEN_LITERAL_STRING )
            {
                return std::string();
            }
            std::string osRE("'");
            const size_t nSize = expr->m_aoChildren[1]->m_osVal.size();
            bool bHasUsedEscape = false;
            for( size_t i=0; i<nSize;i++ )
            {
                if( i == 0 && expr->m_aoChildren[1]->m_osVal[i] == '^' )
                    continue;
                if( i == nSize-1 && expr->m_aoChildren[1]->m_osVal[i] == '$' )
                    break;
                if( expr->m_aoChildren[1]->m_osVal[i] == '.' )
                {
                    if( i+1<nSize &&
                        expr->m_aoChildren[1]->m_osVal[i+1] == '*' )
                    {
                        osRE += "%";
                        i++;
                    }
                    else
                    {
                        osRE += "_";
                    }
                }
                else if( expr->m_aoChildren[1]->m_osVal[i] == '\\' &&
                         i+1<nSize )
                {
                    bHasUsedEscape = true;
                    osRE += 'X';
                    osRE += expr->m_aoChildren[1]->m_osVal[i+1];
                    i++;
                }
                else if( expr->m_aoChildren[1]->m_osVal[i] == 'X' ||
                         expr->m_aoChildren[1]->m_osVal[i] == '%' ||
                         expr->m_aoChildren[1]->m_osVal[i] == '_' )
                {
                    bHasUsedEscape = true;
                    osRE += 'X';
                    osRE += expr->m_aoChildren[1]->m_osVal[i];
                }
                else
                {
                    osRE += expr->m_aoChildren[1]->m_osVal[i];
                }
            }
            osRE += "'";
            char md_item_name[256];
            snprintf( md_item_name, sizeof(md_item_name), "gml_%s_type",
                        expr->m_aoChildren[0]->m_osVal.c_str() );
            const char* type =
                        msLookupHashTable(&(layer->metadata), md_item_name);
            // Cast if needed (or unsure)
            if( type == NULL || !EQUAL(type, "Character") )
            {
                osTmp1 = "CAST(" + osTmp1 + " AS CHARACTER(4096))";
            }
            std::string osRet( "(" + osTmp1 + " LIKE " + osRE );
            if( bHasUsedEscape )
                osRet += " ESCAPE 'X'";
            osRet += ")";
            return osRet;
        }

        case MS_TOKEN_COMPARISON_IN:
        {
            std::string osTmp1(msOGRTranslatePartialInternal(
                layer, expr->m_aoChildren[0].get(), spatialFilterNode, bPartialFilter ));
            std::string osRet = "(" + osTmp1 + " IN (";
            for( size_t i=0; i< expr->m_aoChildren[1]->m_aoChildren.size(); ++i )
            {
                if( i > 0 )
                    osRet += ", ";
                osRet += msOGRTranslatePartialInternal(
                            layer, expr->m_aoChildren[1]->m_aoChildren[i].get(),
                            spatialFilterNode, bPartialFilter );
            }
            osRet += ")";
            return osRet;
        }

        case MS_TOKEN_LITERAL_NUMBER:
        case MS_TOKEN_LITERAL_BOOLEAN:
        {
            return std::string(CPLSPrintf("%.18g", expr->m_dfVal));
        }

        case MS_TOKEN_LITERAL_STRING:
        {
            char *stresc = msOGREscapeSQLParam(layer, expr->m_osVal.c_str());
            std::string osRet("'" + std::string(stresc) + "'");
            msFree(stresc);
            return osRet;
        }

        case MS_TOKEN_LITERAL_TIME:
        {
#ifdef notdef
            // Breaks tests in msautotest/wxs/wfs_time_ogr.map
            return std::string(CPLSPrintf("'%04d/%02d/%02d %02d:%02d:%02d'",
                     expr->m_tmVal.tm_year+1900,
                     expr->m_tmVal.tm_mon+1,
                     expr->m_tmVal.tm_mday,
                     expr->m_tmVal.tm_hour,
                     expr->m_tmVal.tm_min,
                     expr->m_tmVal.tm_sec));
#endif
            return std::string();
        }

        case MS_TOKEN_BINDING_DOUBLE:
        case MS_TOKEN_BINDING_INTEGER:
        case MS_TOKEN_BINDING_STRING:
        case MS_TOKEN_BINDING_TIME:
        {
            char* pszTmp = msOGRGetQuotedItem(layer, expr->m_osVal.c_str());
            std::string osRet(pszTmp);
            msFree(pszTmp);
            return osRet;
        }

        case MS_TOKEN_COMPARISON_INTERSECTS:
        {
            if( expr != spatialFilterNode )
                bPartialFilter = true;
            return std::string();
        }

        default:
        {
            bPartialFilter = true;
            return std::string();
        }
    }
}

/* ==================================================================
 * Here comes the REAL stuff... the functions below are called by maplayer.c
 * ================================================================== */

/**********************************************************************
 *                     msOGRTranslateMsExpressionToOGRSQL()
 *
 * Tries to translate a mapserver expression to OGR or driver native SQL
 **********************************************************************/
static int msOGRTranslateMsExpressionToOGRSQL(layerObj* layer,
                                              expressionObj* psFilter,
                                              char *filteritem)
{
    msOGRFileInfo *info = (msOGRFileInfo *)layer->layerinfo;

    msFree(layer->filter.native_string);
    layer->filter.native_string = NULL;

    msFree(info->pszWHERE);
    info->pszWHERE = NULL;

    // reasons to not produce native string: not simple layer, or an explicit deny
    const char *do_this = msLayerGetProcessingKey(layer, "NATIVE_SQL"); // default is YES
    if (do_this && strcmp(do_this, "NO") == 0) {
        return MS_SUCCESS;
    }

    tokenListNodeObjPtr node = psFilter->tokens;
    auto expr = BuildExprTree(node, NULL, 0);
    info->rect_is_defined = MS_FALSE;
    const msExprNode* spatialFilterNode = NULL;
    if( expr )
        msOGRExtractTopSpatialFilter( info, expr.get(), &spatialFilterNode );

    // more reasons to not produce native string: not a recognized driver
    if (!info->dialect)
    {
        // in which case we might still want to try to get a partial WHERE clause
        if( filteritem == NULL && expr )
        {
            bool bPartialFilter = false;
            std::string osSQL( msOGRTranslatePartialInternal(layer, expr.get(),
                                                             spatialFilterNode,
                                                             bPartialFilter) );
            if( !osSQL.empty() )
            {
                info->pszWHERE = msStrdup(osSQL.c_str());
                if( bPartialFilter )
                {
                    msDebug("Full filter has only been partially "
                            "translated to OGR filter %s\n",
                            info->pszWHERE);
                }
            }
            else if( bPartialFilter )
            {
                msDebug("Filter could not be translated to OGR filter\n");
            }
        }
        return MS_SUCCESS;
    }

    char *sql = NULL;

    // node may be NULL if layer->filter.string != NULL and filteritem != NULL
    // this is simple filter but string is regex
    if (node == NULL && filteritem != NULL && layer->filter.string != NULL) {
        sql = msStringConcatenate(sql, "\"");
        sql = msStringConcatenate(sql, filteritem);
        sql = msStringConcatenate(sql, "\"");
        if (EQUAL(info->dialect, "PostgreSQL") ) {
            sql = msStringConcatenate(sql, " ~ ");
        } else {
            sql = msStringConcatenate(sql, " LIKE ");
        }
        sql = msStringConcatenate(sql, "'");
        sql = msStringConcatenate(sql, layer->filter.string);
        sql = msStringConcatenate(sql, "'");
    }

    while (node != NULL) {

        if (node->next && node->next->token == MS_TOKEN_COMPARISON_IEQ) {
            char *left = msOGRGetToken(layer, &node);
            node = node->next; // skip =
            char *right = msOGRGetToken(layer, &node);
            sql = msStringConcatenate(sql, "upper(");
            sql = msStringConcatenate(sql, left);
            sql = msStringConcatenate(sql, ")");
            sql = msStringConcatenate(sql, "=");
            sql = msStringConcatenate(sql, "upper(");
            sql = msStringConcatenate(sql, right);
            sql = msStringConcatenate(sql, ")");
            int ok = left && right;
            msFree(left);
            msFree(right);
            if (!ok) {
                goto fail;
            }
            continue;
        }

        switch(node->token) {
        case MS_TOKEN_COMPARISON_INTERSECTS:
        case MS_TOKEN_COMPARISON_DISJOINT:
        case MS_TOKEN_COMPARISON_TOUCHES:
        case MS_TOKEN_COMPARISON_OVERLAPS:
        case MS_TOKEN_COMPARISON_CROSSES:
        case MS_TOKEN_COMPARISON_DWITHIN:
        case MS_TOKEN_COMPARISON_BEYOND:
        case MS_TOKEN_COMPARISON_WITHIN:
        case MS_TOKEN_COMPARISON_CONTAINS:
        case MS_TOKEN_COMPARISON_EQUALS:{
            int token = node->token;
            char *fct = msOGRGetToken(layer, &node);
            node = node->next; // skip (
            char *a1 = msOGRGetToken(layer, &node);
            node = node->next; // skip ,
            char *a2 = msOGRGetToken(layer, &node);
            char *a3 = NULL;
            if (token == MS_TOKEN_COMPARISON_DWITHIN || token == MS_TOKEN_COMPARISON_BEYOND) {
                node = node->next; // skip ,
                a3 = msOGRGetToken(layer, &node);
            }
            node = node->next; // skip )
            char *eq = msOGRGetToken(layer, &node);
            char *rval = msOGRGetToken(layer, &node);
            if ((eq && strcmp(eq, " != ") == 0) || (rval && strcmp(rval, "FALSE") == 0)) {
                sql = msStringConcatenate(sql, "NOT ");
            }
            // FIXME: case rval is more complex
            sql = msStringConcatenate(sql, fct);
            sql = msStringConcatenate(sql, "(");
            sql = msStringConcatenate(sql, a1);
            sql = msStringConcatenate(sql, ",");
            sql = msStringConcatenate(sql, a2);
            if (token == MS_TOKEN_COMPARISON_DWITHIN || token == MS_TOKEN_COMPARISON_BEYOND) {
                sql = msStringConcatenate(sql, ")");
                if (token == MS_TOKEN_COMPARISON_DWITHIN)
                    sql = msStringConcatenate(sql, "<=");
                else 
                    sql = msStringConcatenate(sql, ">");
                sql = msStringConcatenate(sql, a3);
            } else {
                sql = msStringConcatenate(sql, ")");
            }
            int ok = fct && a1 && a2 && eq && rval;
            if (token == MS_TOKEN_COMPARISON_DWITHIN) {
                ok = ok && a3;
            }
            msFree(fct);
            msFree(a1);
            msFree(a2);
            msFree(a3);
            msFree(eq);
            msFree(rval);
            if (!ok) {
                goto fail;
            }
            break;
        }
        default: {
            char *token = msOGRGetToken(layer, &node);
            if (!token) {
                goto fail;
            }
            sql = msStringConcatenate(sql, token);
            msFree(token);
        }
        }
    }

    layer->filter.native_string = sql;
    return MS_SUCCESS;
fail:
    // error producing native string
    msDebug("Note: Error parsing token list, could produce only: %s. Trying in partial mode\n", sql);
    msFree(sql);

    // in which case we might still want to try to get a partial WHERE clause
    if( expr )
    {
        bool bPartialFilter = false;
        std::string osSQL( msOGRTranslatePartialInternal(layer, expr.get(),
                                                            spatialFilterNode,
                                                            bPartialFilter) );
        if( !osSQL.empty() )
        {
            info->pszWHERE = msStrdup(osSQL.c_str());
            if( bPartialFilter )
            {
                msDebug("Full filter has only been partially "
                        "translated to OGR filter %s\n",
                        info->pszWHERE);
            }
        }
        else if( bPartialFilter )
        {
            msDebug("Filter could not be translated to OGR filter\n");
        }
    }

    return MS_SUCCESS;
}

/**********************************************************************
 *                     msOGRLayerOpen()
 *
 * Open OGR data source for the specified map layer.
 *
 * If pszOverrideConnection != NULL then this value is used as the connection
 * string instead of lp->connection.  This is used for instance to open
 * a WFS layer, in this case lp->connection is the WFS url, but we want
 * OGR to open the local file on disk that was previously downloaded.
 *
 * An OGR connection string is:   <dataset_filename>[,<layer_index>]
 *  <dataset_filename>   is file format specific
 *  <layer_index>        (optional) is the OGR layer index
 *                       default is 0, the first layer.
 *
 * One can use the "ogrinfo" program to find out the layer indices in a dataset
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerOpen(layerObj *layer, const char *pszOverrideConnection)
{
  msOGRFileInfo *psInfo;

  if (layer->layerinfo != NULL) {
    return MS_SUCCESS;  // Nothing to do... layer is already opened
  }

  /* -------------------------------------------------------------------- */
  /*      If this is not a tiled layer, just directly open the target.    */
  /* -------------------------------------------------------------------- */
  if( layer->tileindex == NULL ) {
    psInfo = msOGRFileOpen( layer,
                            (pszOverrideConnection ? pszOverrideConnection:
                             layer->connection) );
    layer->layerinfo = psInfo;
    layer->tileitemindex = -1;

    if( layer->layerinfo == NULL )
      return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      Otherwise we open the tile index, identify the tile item        */
  /*      index and try to select the first file matching our query       */
  /*      region.                                                         */
  /* -------------------------------------------------------------------- */
  else {
    // Open tile index

    psInfo = msOGRFileOpen( layer, layer->tileindex );
    layer->layerinfo = psInfo;

    if( layer->layerinfo == NULL )
      return MS_FAILURE;

    // Identify TILEITEM
    OGRFeatureDefnH hDefn = OGR_L_GetLayerDefn( psInfo->hLayer );
    layer->tileitemindex = OGR_FD_GetFieldIndex(hDefn, layer->tileitem);
    if( layer->tileitemindex < 0 ) {
      msSetError(MS_OGRERR,
                 "Can't identify TILEITEM %s field in TILEINDEX `%s'.",
                 "msOGRLayerOpen()",
                 layer->tileitem, layer->tileindex );
      msOGRFileClose( layer, psInfo );
      layer->layerinfo = NULL;
      return MS_FAILURE;
    }

    // Identify TILESRS
    if( layer->tilesrs != NULL &&
        OGR_FD_GetFieldIndex(hDefn, layer->tilesrs) < 0 ) {
      msSetError(MS_OGRERR,
                 "Can't identify TILESRS %s field in TILEINDEX `%s'.",
                 "msOGRLayerOpen()",
                 layer->tilesrs, layer->tileindex );
      msOGRFileClose( layer, psInfo );
      layer->layerinfo = NULL;
      return MS_FAILURE;
    }
    if( layer->tilesrs != NULL && layer->projection.numargs == 0 )
    {
        msSetError(MS_OGRERR,
                 "A layer with TILESRS set in TILEINDEX `%s' must have a "
                 "projection set on itself.",
                 "msOGRLayerOpen()",
                 layer->tileindex );
        msOGRFileClose( layer, psInfo );
        layer->layerinfo = NULL;
        return MS_FAILURE;
    }
  }

  /* ------------------------------------------------------------------
   * If projection was "auto" then set proj to the dataset's projection.
   * For a tile index, it is assume the tile index has the projection.
   * ------------------------------------------------------------------ */
  if (layer->projection.numargs > 0 &&
      EQUAL(layer->projection.args[0], "auto")) {
    ACQUIRE_OGR_LOCK;
    OGRSpatialReferenceH hSRS = OGR_L_GetSpatialRef( psInfo->hLayer );

    if (msOGRSpatialRef2ProjectionObj(hSRS,
                                      &(layer->projection),
                                      layer->debug ) != MS_SUCCESS) {
      errorObj *ms_error = msGetErrorObj();

      RELEASE_OGR_LOCK;
      msSetError(MS_OGRERR,
                 "%s  "
                 "PROJECTION AUTO cannot be used for this "
                 "OGR connection (in layer `%s').",
                 "msOGRLayerOpen()",
                 ms_error->message,
                 layer->name?layer->name:"(null)" );
      msOGRFileClose( layer, psInfo );
      layer->layerinfo = NULL;
      return(MS_FAILURE);
    }
    RELEASE_OGR_LOCK;
  }

  return MS_SUCCESS;
}

/**********************************************************************
 *                     msOGRLayerOpenVT()
 *
 * Overloaded version of msOGRLayerOpen for virtual table architecture
 **********************************************************************/
static int msOGRLayerOpenVT(layerObj *layer)
{
  return msOGRLayerOpen(layer, NULL);
}

/**********************************************************************
 *                     msOGRLayerClose()
 **********************************************************************/
int msOGRLayerClose(layerObj *layer)
{
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;

  if (psInfo) {
    if( layer->debug )
      msDebug("msOGRLayerClose(%s).\n", layer->connection);

    msOGRFileClose( layer, psInfo );
    layer->layerinfo = NULL;
  }

  return MS_SUCCESS;
}

/**********************************************************************
 *                     msOGRLayerIsOpen()
 **********************************************************************/
static int msOGRLayerIsOpen(layerObj *layer)
{
  if (layer->layerinfo)
    return MS_TRUE;

  return MS_FALSE;
}

int msOGRSupportsIsNull(layerObj* layer)
{
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  if (psInfo && psInfo->dialect &&
      (EQUAL(psInfo->dialect, "Spatialite") || EQUAL(psInfo->dialect, "GPKG")))
  {
    // reasons to not produce native string: not simple layer, or an explicit deny
    const char *do_this = msLayerGetProcessingKey(layer, "NATIVE_SQL"); // default is YES
    if (do_this && strcmp(do_this, "NO") == 0) {
        return MS_FALSE;
    }
    return MS_TRUE;
  }

  return MS_FALSE;
}


/**********************************************************************
 *                     msOGRLayerWhichShapes()
 *
 * Init OGR layer structs ready for calls to msOGRLayerNextShape().
 *
 * Returns MS_SUCCESS/MS_FAILURE, or MS_DONE if no shape matching the
 * layer's FILTER overlaps the selected region.
 **********************************************************************/
int msOGRLayerWhichShapes(layerObj *layer, rectObj rect, int /*isQuery*/)
{
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  int   status;

  if (psInfo == NULL || psInfo->hLayer == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!",
               "msOGRLayerWhichShapes()");
    return(MS_FAILURE);
  }

  status = msOGRFileWhichShapes( layer, rect, psInfo );

  if( status != MS_SUCCESS || layer->tileindex == NULL )
    return status;

  // If we are using a tile index, we need to advance to the first
  // tile matching the spatial query, and load it.

  return msOGRFileReadTile( layer, psInfo );
}

/**********************************************************************
 *                     msOGRLayerGetItems()
 *
 * Load item (i.e. field) names in a char array.  If we are working
 * with a tiled layer, ensure a tile is loaded and use it for the items.
 * It is implicitly assumed that the schemas will match on all tiles.
 **********************************************************************/
int msOGRLayerGetItems(layerObj *layer)
{
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;

  if (psInfo == NULL || psInfo->hLayer == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!",
               "msOGRLayerGetItems()");
    return(MS_FAILURE);
  }

  if( layer->tileindex != NULL ) {
    if( psInfo->poCurTile == NULL
        && msOGRFileReadTile( layer, psInfo ) != MS_SUCCESS )
      return MS_FAILURE;

    psInfo = psInfo->poCurTile;
  }

  layer->numitems = 0;
  layer->items = msOGRFileGetItems(layer, psInfo);
  if( layer->items == NULL )
    return MS_FAILURE;

  while( layer->items[layer->numitems] != NULL )
    layer->numitems++;

  return msOGRLayerInitItemInfo(layer);
}

/**********************************************************************
 *                     msOGRLayerInitItemInfo()
 *
 * Init the itemindexes array after items[] has been reset in a layer.
 **********************************************************************/
static int msOGRLayerInitItemInfo(layerObj *layer)
{
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  int   i;
  OGRFeatureDefnH hDefn;

  if (layer->numitems == 0)
    return MS_SUCCESS;

  if( layer->tileindex != NULL ) {
    if( psInfo->poCurTile == NULL
        && msOGRFileReadTile( layer, psInfo, -2 ) != MS_SUCCESS )
      return MS_FAILURE;

    psInfo = psInfo->poCurTile;
  }

  if (psInfo == NULL || psInfo->hLayer == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!",
               "msOGRLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  if((hDefn = OGR_L_GetLayerDefn( psInfo->hLayer )) == NULL) {
    msSetError(MS_OGRERR, "Layer contains no fields.",
               "msOGRLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  if (layer->iteminfo)
    free(layer->iteminfo);
  if((layer->iteminfo = (int *)malloc(sizeof(int)*layer->numitems))== NULL) {
    msSetError(MS_MEMERR, NULL, "msOGRLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  int *itemindexes = (int*)layer->iteminfo;
  for(i=0; i<layer->numitems; i++) {
    // Special case for handling text string and angle coming from
    // OGR style strings.  We use special attribute snames.
    if (EQUAL(layer->items[i], MSOGR_LABELFONTNAMENAME))
      itemindexes[i] = MSOGR_LABELFONTNAMEINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELSIZENAME))
      itemindexes[i] = MSOGR_LABELSIZEINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELTEXTNAME))
      itemindexes[i] = MSOGR_LABELTEXTINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELANGLENAME))
      itemindexes[i] = MSOGR_LABELANGLEINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELFCOLORNAME))
      itemindexes[i] = MSOGR_LABELFCOLORINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELBCOLORNAME))
      itemindexes[i] = MSOGR_LABELBCOLORINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELPLACEMENTNAME))
      itemindexes[i] = MSOGR_LABELPLACEMENTINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELANCHORNAME))
      itemindexes[i] = MSOGR_LABELANCHORINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELDXNAME))
      itemindexes[i] = MSOGR_LABELDXINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELDYNAME))
      itemindexes[i] = MSOGR_LABELDYINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELPERPNAME))
      itemindexes[i] = MSOGR_LABELPERPINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELBOLDNAME))
      itemindexes[i] = MSOGR_LABELBOLDINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELITALICNAME))
      itemindexes[i] = MSOGR_LABELITALICINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELUNDERLINENAME))
      itemindexes[i] = MSOGR_LABELUNDERLINEINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELPRIORITYNAME))
      itemindexes[i] = MSOGR_LABELPRIORITYINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELSTRIKEOUTNAME))
      itemindexes[i] = MSOGR_LABELSTRIKEOUTINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELSTRETCHNAME))
      itemindexes[i] = MSOGR_LABELSTRETCHINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELADJHORNAME))
      itemindexes[i] = MSOGR_LABELADJHORINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELADJVERTNAME))
      itemindexes[i] = MSOGR_LABELADJVERTINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELHCOLORNAME))
      itemindexes[i] = MSOGR_LABELHCOLORINDEX;
    else if (EQUAL(layer->items[i], MSOGR_LABELOCOLORNAME))
      itemindexes[i] = MSOGR_LABELOCOLORINDEX;
    else if (EQUALN(layer->items[i], MSOGR_LABELPARAMNAME, MSOGR_LABELPARAMNAMELEN))
        itemindexes[i] = MSOGR_LABELPARAMINDEX 
                          + atoi(layer->items[i] + MSOGR_LABELPARAMNAMELEN);
    else if (EQUALN(layer->items[i], MSOGR_BRUSHPARAMNAME, MSOGR_BRUSHPARAMNAMELEN))
        itemindexes[i] = MSOGR_BRUSHPARAMINDEX 
                          + atoi(layer->items[i] + MSOGR_BRUSHPARAMNAMELEN);
    else if (EQUALN(layer->items[i], MSOGR_PENPARAMNAME, MSOGR_PENPARAMNAMELEN))
        itemindexes[i] = MSOGR_PENPARAMINDEX 
                          + atoi(layer->items[i] + MSOGR_PENPARAMNAMELEN);
    else if (EQUALN(layer->items[i], MSOGR_SYMBOLPARAMNAME, MSOGR_SYMBOLPARAMNAMELEN))
        itemindexes[i] = MSOGR_SYMBOLPARAMINDEX 
                          + atoi(layer->items[i] + MSOGR_SYMBOLPARAMNAMELEN);
    else
    {
      itemindexes[i] = OGR_FD_GetFieldIndex( hDefn, layer->items[i] );
      if( itemindexes[i] == -1 )
      {
          if( EQUAL( layer->items[i], OGR_L_GetFIDColumn( psInfo->hLayer ) ) )
          {
              itemindexes[i] = MSOGR_FID_INDEX;
          }
      }
    }
    if(itemindexes[i] == -1) {
      msSetError(MS_OGRERR,
                 "Invalid Field name: %s in layer `%s'",
                 "msOGRLayerInitItemInfo()",
                 layer->items[i], layer->name ? layer->name : "(null)");
      return(MS_FAILURE);
    }
  }

  return(MS_SUCCESS);
}

/**********************************************************************
 *                     msOGRLayerFreeItemInfo()
 *
 * Free the itemindexes array in a layer.
 **********************************************************************/
void msOGRLayerFreeItemInfo(layerObj *layer)
{

  if (layer->iteminfo)
    free(layer->iteminfo);
  layer->iteminfo = NULL;
}


/**********************************************************************
 *                     msOGRLayerNextShape()
 *
 * Returns shape sequentially from OGR data source.
 * msOGRLayerWhichShape() must have been called first.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerNextShape(layerObj *layer, shapeObj *shape)
{
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  int  status;

  if (psInfo == NULL || psInfo->hLayer == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!",
               "msOGRLayerNextShape()");
    return(MS_FAILURE);
  }

  if( layer->tileindex == NULL )
    return msOGRFileNextShape( layer, shape, psInfo );

  // Do we need to load the first tile?
  if( psInfo->poCurTile == NULL ) {
    status = msOGRFileReadTile( layer, psInfo );
    if( status != MS_SUCCESS )
      return status;
  }

  do {
    // Try getting a shape from this tile.
    status = msOGRFileNextShape( layer, shape, psInfo->poCurTile );
    if( status != MS_DONE )
    {
      if( psInfo->sTileProj.numargs > 0 )
      {
        msProjectShape(&(psInfo->sTileProj), &(layer->projection), shape);
      }

      return status;
    }

    // try next tile.
    status = msOGRFileReadTile( layer, psInfo );
    if( status != MS_SUCCESS )
      return status;
  } while( status == MS_SUCCESS );
  return status; //make compiler happy. this is never reached however
}

/**********************************************************************
 *                     msOGRLayerGetShape()
 *
 * Returns shape from OGR data source by fid.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;

  long shapeindex = record->shapeindex;
  int tileindex = record->tileindex;
  int resultindex = record->resultindex;
  int record_is_fid = TRUE;

  /* set the resultindex as shapeindex if available */
  if (resultindex >= 0) {
    record_is_fid = FALSE;
    shapeindex = resultindex;
  }

  if (psInfo == NULL || psInfo->hLayer == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", "msOGRLayerGetShape()");
    return(MS_FAILURE);
  }

  if( layer->tileindex == NULL )
    return msOGRFileGetShape(layer, shape, shapeindex, psInfo, record_is_fid );
  else {
    if( psInfo->poCurTile == NULL
        || psInfo->poCurTile->nTileId != tileindex ) {
      if( msOGRFileReadTile( layer, psInfo, tileindex ) != MS_SUCCESS )
        return MS_FAILURE;
    }

    int status = msOGRFileGetShape(layer, shape, shapeindex, psInfo->poCurTile, record_is_fid );
    if( status == MS_SUCCESS && psInfo->sTileProj.numargs > 0 )
    {
      msProjectShape(&(psInfo->sTileProj), &(layer->projection), shape);
    }
    return status;
  }
}

/**********************************************************************
 *                     msOGRLayerGetExtent()
 *
 * Returns the layer extents.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerGetExtent(layerObj *layer, rectObj *extent)
{
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  OGREnvelope oExtent;

  if (psInfo == NULL || psInfo->hLayer == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!",
               "msOGRLayerGetExtent()");
    return(MS_FAILURE);
  }

  /* ------------------------------------------------------------------
   * Call OGR's GetExtent()... note that for some formats this will
   * result in a scan of the whole layer and can be an expensive call.
   *
   * For tile indexes layers we assume it is sufficient to get the
   * extents of the tile index.
   * ------------------------------------------------------------------ */
  ACQUIRE_OGR_LOCK;
  if (OGR_L_GetExtent( psInfo->hLayer, &oExtent, TRUE) != OGRERR_NONE) {
    RELEASE_OGR_LOCK;
    msSetError(MS_MISCERR, "Unable to get extents for this layer.",
               "msOGRLayerGetExtent()");
    return(MS_FAILURE);
  }
  RELEASE_OGR_LOCK;

  extent->minx = oExtent.MinX;
  extent->miny = oExtent.MinY;
  extent->maxx = oExtent.MaxX;
  extent->maxy = oExtent.MaxY;

  return MS_SUCCESS;
}

/**********************************************************************
*                     msOGRLayerGetNumFeatures()
*
* Returns the layer feature count.
*
* Returns the number of features on success, -1 on error
**********************************************************************/
int msOGRLayerGetNumFeatures(layerObj *layer)
{
    msOGRFileInfo *psInfo = (msOGRFileInfo*)layer->layerinfo;
    int result;

    if (psInfo == NULL || psInfo->hLayer == NULL) {
        msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!",
            "msOGRLayerGetNumFeatures()");
        return -1;
    }

    /* ------------------------------------------------------------------
    * Call OGR's GetFeatureCount()... note that for some formats this will
    * result in a scan of the whole layer and can be an expensive call.
    * ------------------------------------------------------------------ */
    ACQUIRE_OGR_LOCK;
    result = (int)OGR_L_GetFeatureCount(psInfo->hLayer, TRUE);
    RELEASE_OGR_LOCK;

    return result;
}

/**********************************************************************
 *                     msOGRGetSymbolId()
 *
 * Returns a MapServer symbol number matching one of the symbols from
 * the OGR symbol id string.  If not found then try to locate the
 * default symbol name, and if not found return 0.
 **********************************************************************/
static int msOGRGetSymbolId(symbolSetObj *symbolset, const char *pszSymbolId,
                            const char *pszDefaultSymbol, int try_addimage_if_notfound)
{
  // Symbol name mapping:
  // First look for the native symbol name, then the ogr-...
  // generic name, and in last resort try pszDefaultSymbol if
  // provided by user.
  char  **params;
  int   numparams;
  int   nSymbol = -1;

  if (pszSymbolId && pszSymbolId[0] != '\0') {
    params = msStringSplit(pszSymbolId, ',', &numparams);
    if (params != NULL) {
      for(int j=0; j<numparams && nSymbol == -1; j++) {
        nSymbol = msGetSymbolIndex(symbolset, params[j],
                                   try_addimage_if_notfound);
      }
      msFreeCharArray(params, numparams);
    }
  }
  if (nSymbol == -1 && pszDefaultSymbol) {
    nSymbol = msGetSymbolIndex(symbolset,(char*)pszDefaultSymbol,
                               try_addimage_if_notfound);
  }
  if (nSymbol == -1)
    nSymbol = 0;

  return nSymbol;
}

static int msOGRUpdateStyleParseLabel(mapObj *map, layerObj *layer, classObj *c,
                                      OGRStyleToolH hLabelStyle);
static int msOGRUpdateStyleParsePen(mapObj *map, layerObj *layer, styleObj *s,
                                    OGRStyleToolH hPenStyle, int bIsBrush, int* pbPriority);
static int msOGRUpdateStyleParseBrush(mapObj *map, layerObj *layer, styleObj *s,
                                      OGRStyleToolH hBrushStyle, int* pbIsBrush, int* pbPriority);
static int msOGRUpdateStyleParseSymbol(mapObj *map, styleObj *s,
                                       OGRStyleToolH hSymbolStyle, int* pbPriority);
static int msOGRAddBgColorStyleParseBrush(styleObj* s, OGRStyleToolH hSymbolStyle);

static int msOGRUpdateStyleCheckPenBrushOnly(OGRStyleMgrH hStyleMgr)
{
  int numParts = OGR_SM_GetPartCount(hStyleMgr, NULL);
  int countPen = 0, countBrush = 0;
  int bIsNull;

  for(int i=0; i<numParts; i++) {
    OGRSTClassId eStylePartType;
    OGRStyleToolH hStylePart = OGR_SM_GetPart(hStyleMgr, i, NULL);
    if (!hStylePart)
      continue;

    eStylePartType = OGR_ST_GetType(hStylePart);
    if (eStylePartType == OGRSTCPen) {
      countPen ++;
      OGR_ST_GetParamNum(hStylePart, OGRSTPenPriority, &bIsNull);
      if( !bIsNull ) {
        OGR_ST_Destroy(hStylePart);
        return MS_FALSE;
      }
    }
    else if (eStylePartType == OGRSTCBrush) {
      countBrush ++;
      OGR_ST_GetParamNum(hStylePart, OGRSTBrushPriority, &bIsNull);
      if( !bIsNull ) {
        OGR_ST_Destroy(hStylePart);
        return MS_FALSE;
      }
    }
    else if (eStylePartType == OGRSTCSymbol) {
      OGR_ST_Destroy(hStylePart);
      return MS_FALSE;
    }
    OGR_ST_Destroy(hStylePart);
  }
  return (countPen == 1 && countBrush == 1);
}

/**********************************************************************
 *                     msOGRUpdateStyle()
 *
 * Update the mapserver style according to the ogr style.
 * The function is called by msOGRGetAutoStyle and
 * msOGRUpdateStyleFromString
 **********************************************************************/

typedef struct
{
    int nPriority; /* the explicit priority as specified by the 'l' option of PEN, BRUSH and SYMBOL tools */
    int nApparitionIndex; /* the index of the tool as parsed from the OGR feature style string */
} StyleSortStruct;

static int msOGRUpdateStyleSortFct(const void* pA, const void* pB)
{
    StyleSortStruct* sssa = (StyleSortStruct*)pA;
    StyleSortStruct* sssb = (StyleSortStruct*)pB;
    if( sssa->nPriority < sssb->nPriority )
        return -1;
    else if( sssa->nPriority > sssb->nPriority )
        return 1;
    else if( sssa->nApparitionIndex < sssb->nApparitionIndex )
        return -1;
    else
        return 1;
}

static int msOGRUpdateStyle(OGRStyleMgrH hStyleMgr, mapObj *map, layerObj *layer, classObj *c)
{
  GBool bIsBrush=MS_FALSE;
  int numParts = OGR_SM_GetPartCount(hStyleMgr, NULL);
  int nPriority;
  int bIsPenBrushOnly = msOGRUpdateStyleCheckPenBrushOnly(hStyleMgr);
  StyleSortStruct* pasSortStruct = (StyleSortStruct*) msSmallMalloc(sizeof(StyleSortStruct) * numParts);
  int iSortStruct = 0;
  int iBaseStyleIndex = c->numstyles;
  int i;

  /* ------------------------------------------------------------------
   * Handle each part
   * ------------------------------------------------------------------ */

  for(i=0; i<numParts; i++) {
    OGRSTClassId eStylePartType;
    OGRStyleToolH hStylePart = OGR_SM_GetPart(hStyleMgr, i, NULL);
    if (!hStylePart)
      continue;
    eStylePartType = OGR_ST_GetType(hStylePart);
    nPriority = INT_MIN;

    // We want all size values returned in pixels.
    //
    // The scale factor that OGR expect is the ground/paper scale
    // e.g. if 1 ground unit = 0.01 paper unit then scale=1/0.01=100
    // cellsize if number of ground units/pixel, and OGR assumes that
    // there is 72*39.37 pixels/ground units (since meter is assumed
    // for ground... but what ground units we have does not matter
    // as long as use the same assumptions everywhere)
    // That gives scale = cellsize*72*39.37

    OGR_ST_SetUnit(hStylePart, OGRSTUPixel, 
      map->cellsize*map->resolution/map->defresolution*72.0*39.37);

    if (eStylePartType == OGRSTCLabel) {
      int ret = msOGRUpdateStyleParseLabel(map, layer, c, hStylePart);
      if( ret != MS_SUCCESS ) {
        OGR_ST_Destroy(hStylePart);
        msFree(pasSortStruct);
        return ret;
      }
    } else if (eStylePartType == OGRSTCPen) {
      styleObj* s;
      int nIndex;
      if( bIsPenBrushOnly ) {
        /* Historic behaviour when there is a PEN and BRUSH only */
        if (bIsBrush || layer->type == MS_LAYER_POLYGON)
            // This is a multipart symbology, so pen defn goes in the
            // overlaysymbol params
            nIndex = c->numstyles + 1;
        else
            nIndex = c->numstyles;
      }
      else
        nIndex = c->numstyles;

      if (msMaybeAllocateClassStyle(c, nIndex)) {
        OGR_ST_Destroy(hStylePart);
        msFree(pasSortStruct);
        return(MS_FAILURE);
      }
      s = c->styles[nIndex];

      msOGRUpdateStyleParsePen(map, layer, s, hStylePart, bIsBrush, &nPriority);

    } else if (eStylePartType == OGRSTCBrush) {

      styleObj* s;
      int nIndex = 0;

      GBool bBgColorIsNull = MS_TRUE;
      OGR_ST_GetParamStr(hStylePart, OGRSTBrushBColor, &bBgColorIsNull);

      if (!bBgColorIsNull) {

       if (msMaybeAllocateClassStyle(c, nIndex)) {
            OGR_ST_Destroy(hStylePart);
            msFree(pasSortStruct);
            return(MS_FAILURE);
        }

        // add a backgroundcolor as a separate style
        s = c->styles[nIndex];
        msOGRAddBgColorStyleParseBrush(s, hStylePart);
      }


      nIndex = ( bIsPenBrushOnly ) ? nIndex : c->numstyles;

      if (!bBgColorIsNull) {
          // if we have a bgcolor style we need to increase the index
          nIndex += 1;
      }

      /* We need 1 style */
      if (msMaybeAllocateClassStyle(c, nIndex)) {
        OGR_ST_Destroy(hStylePart);
        msFree(pasSortStruct);
        return(MS_FAILURE);
      }
      s = c->styles[nIndex];

      msOGRUpdateStyleParseBrush(map, layer, s, hStylePart, &bIsBrush, &nPriority);

    } else if (eStylePartType == OGRSTCSymbol) {
      styleObj* s;
      /* We need 1 style */
      int nIndex = c->numstyles;
      if (msMaybeAllocateClassStyle(c, nIndex)) {
        OGR_ST_Destroy(hStylePart);
        msFree(pasSortStruct);
        return(MS_FAILURE);
      }
      s = c->styles[nIndex];

      msOGRUpdateStyleParseSymbol(map, s, hStylePart, &nPriority);
    }

    /* Memorize the explicit priority and apparition order of the parsed tool/style */
    if( !bIsPenBrushOnly &&
        (eStylePartType == OGRSTCPen || eStylePartType == OGRSTCBrush ||
         eStylePartType == OGRSTCSymbol) ) {
        pasSortStruct[iSortStruct].nPriority = nPriority;
        pasSortStruct[iSortStruct].nApparitionIndex = iSortStruct;
        iSortStruct++;
    }

    OGR_ST_Destroy(hStylePart);

  }

  if( iSortStruct > 1 && !bIsPenBrushOnly ) {
      /* Compute style order based on their explicit priority and apparition order */
      qsort(pasSortStruct, iSortStruct, sizeof(StyleSortStruct), msOGRUpdateStyleSortFct);

      /* Now reorder styles in c->styles */
      styleObj** ppsStyleTmp = (styleObj**)msSmallMalloc( iSortStruct * sizeof(styleObj*) );
      memcpy( ppsStyleTmp, c->styles + iBaseStyleIndex, iSortStruct * sizeof(styleObj*) );
      for( i = 0; i < iSortStruct; i++)
      {
          c->styles[iBaseStyleIndex + i] = ppsStyleTmp[pasSortStruct[i].nApparitionIndex];
      }
      msFree(ppsStyleTmp);
  }
  
  msFree(pasSortStruct);
  
  return MS_SUCCESS;
}

static int msOGRUpdateStyleParseLabel(mapObj *map, layerObj *layer, classObj *c,
                                      OGRStyleToolH hLabelStyle)
{
  GBool bIsNull;
  int r=0,g=0,b=0,t=0;

      // Enclose the text string inside quotes to make sure it is seen
      // as a string by the parser inside loadExpression(). (bug185)
      /* See bug 3481 about the isalnum hack */
      const char *labelTextString = OGR_ST_GetParamStr(hLabelStyle,
                                    OGRSTLabelTextString,
                                    &bIsNull);

      if (c->numlabels == 0) {
        /* allocate a new label object */
        if(msGrowClassLabels(c) == NULL) 
          return MS_FAILURE;
        c->numlabels++;
        initLabel(c->labels[0]);
      }
      msFreeExpression(&c->labels[0]->text);
      c->labels[0]->text.type = MS_STRING;
      c->labels[0]->text.string = msStrdup(labelTextString);

      c->labels[0]->angle = OGR_ST_GetParamDbl(hLabelStyle,
                            OGRSTLabelAngle, &bIsNull);

      c->labels[0]->size = OGR_ST_GetParamDbl(hLabelStyle,
                                              OGRSTLabelSize, &bIsNull);
      if( c->labels[0]->size < 1 ) /* no point dropping to zero size */
        c->labels[0]->size = 1;

      // OGR default is anchor point = LL, so label is at UR of anchor
      c->labels[0]->position = MS_UR;

      int nPosition = OGR_ST_GetParamNum(hLabelStyle,
                                         OGRSTLabelAnchor,
                                         &bIsNull);
      if( !bIsNull ) {
        switch( nPosition ) {
          case 1:
            c->labels[0]->position = MS_UR;
            break;
          case 2:
            c->labels[0]->position = MS_UC;
            break;
          case 3:
            c->labels[0]->position = MS_UL;
            break;
          case 4:
            c->labels[0]->position = MS_CR;
            break;
          case 5:
            c->labels[0]->position = MS_CC;
            break;
          case 6:
            c->labels[0]->position = MS_CL;
            break;
          case 7:
            c->labels[0]->position = MS_LR;
            break;
          case 8:
            c->labels[0]->position = MS_LC;
            break;
          case 9:
            c->labels[0]->position = MS_LL;
            break;
          case 10:
            c->labels[0]->position = MS_UR;
            break; /*approximate*/
          case 11:
            c->labels[0]->position = MS_UC;
            break;
          case 12:
            c->labels[0]->position = MS_UL;
            break;
          default:
            break;
        }
      }

      const char *pszColor = OGR_ST_GetParamStr(hLabelStyle,
                             OGRSTLabelFColor,
                             &bIsNull);
      if (!bIsNull && OGR_ST_GetRGBFromString(hLabelStyle, pszColor,
                                              &r, &g, &b, &t)) {
        MS_INIT_COLOR(c->labels[0]->color, r, g, b, t);
      }

      pszColor = OGR_ST_GetParamStr(hLabelStyle,
                                    OGRSTLabelHColor,
                                    &bIsNull);
      if (!bIsNull && OGR_ST_GetRGBFromString(hLabelStyle, pszColor,
                                              &r, &g, &b, &t)) {
        MS_INIT_COLOR(c->labels[0]->shadowcolor, r, g, b, t);
      }

      pszColor = OGR_ST_GetParamStr(hLabelStyle,
                                    OGRSTLabelOColor,
                                    &bIsNull);
      if (!bIsNull && OGR_ST_GetRGBFromString(hLabelStyle, pszColor,
                                              &r, &g, &b, &t)) {
        MS_INIT_COLOR(c->labels[0]->outlinecolor, r, g, b, t);
      }

      const char *pszBold = OGR_ST_GetParamNum(hLabelStyle,
                            OGRSTLabelBold,
                            &bIsNull) ? "-bold" : "";
      const char *pszItalic = OGR_ST_GetParamNum(hLabelStyle,
                              OGRSTLabelItalic,
                              &bIsNull) ? "-italic" : "";
      const char *pszFontName = OGR_ST_GetParamStr(hLabelStyle,
                                OGRSTLabelFontName,
                                &bIsNull);
      /* replace spaces with hyphens to allow mapping to a valid hashtable entry*/
      char* pszFontNameEscaped = NULL;
      if (pszFontName != NULL) {
          pszFontNameEscaped = msStrdup(pszFontName);
          msReplaceChar(pszFontNameEscaped, ' ', '-');
      }

      const char *pszName = CPLSPrintf("%s%s%s", pszFontNameEscaped, pszBold, pszItalic);
      bool bFont = true;

      if (pszFontNameEscaped != NULL && !bIsNull && pszFontNameEscaped[0] != '\0') {
        if (msLookupHashTable(&(map->fontset.fonts), (char*)pszName) != NULL) {
          c->labels[0]->font = msStrdup(pszName);
          if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug("** Using '%s' TTF font **\n", pszName);
        } else if ( (strcmp(pszFontNameEscaped,pszName) != 0) &&
                    msLookupHashTable(&(map->fontset.fonts), (char*)pszFontNameEscaped) != NULL) {
          c->labels[0]->font = msStrdup(pszFontNameEscaped);
          if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug("** Using '%s' TTF font **\n", pszFontNameEscaped);
        } else if (msLookupHashTable(&(map->fontset.fonts),"default") != NULL) {
          c->labels[0]->font = msStrdup("default");
          if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug("** Using 'default' TTF font **\n");
        } else
          bFont = false;
      }

      msFree(pszFontNameEscaped);

      if (!bFont) {
        c->labels[0]->size = MS_MEDIUM;
      }

      return MS_SUCCESS;
}

static int msOGRUpdateStyleParsePen(mapObj *map, layerObj *layer, styleObj *s,
                                    OGRStyleToolH hPenStyle, int bIsBrush,
                                    int* pbPriority)
{
  GBool bIsNull;
  int r=0,g=0,b=0,t=0;

      const char *pszPenName, *pszPattern, *pszCap, *pszJoin;
      colorObj oPenColor;
      int nPenSymbol = 0;
      int nPenSize = 1;
      t =-1;
      double pattern[MS_MAXPATTERNLENGTH];
      int patternlength = 0;
      int linecap = MS_CJC_DEFAULT_CAPS;
      int linejoin = MS_CJC_DEFAULT_JOINS;
      double offsetx = 0.0;
      double offsety = 0.0;

      // Make sure pen is always initialized
      MS_INIT_COLOR(oPenColor, -1, -1, -1,255);

      pszPenName = OGR_ST_GetParamStr(hPenStyle,
                               OGRSTPenId,
                               &bIsNull);
      if (bIsNull) pszPenName = NULL;
      // Check for Pen Pattern "ogr-pen-1": the invisible pen
      // If that's what we have then set pen color to -1
      if (pszPenName && strstr(pszPenName, "ogr-pen-1") != NULL) {
        MS_INIT_COLOR(oPenColor, -1, -1, -1,255);
      } else {
        const char *pszColor = OGR_ST_GetParamStr(hPenStyle,
                               OGRSTPenColor,
                               &bIsNull);
        if (!bIsNull && OGR_ST_GetRGBFromString(hPenStyle, pszColor,
                                                &r, &g, &b, &t)) {
          MS_INIT_COLOR(oPenColor, r, g, b, t);
          if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug("** PEN COLOR = %d %d %d **\n", r,g,b);
        }

        nPenSize = OGR_ST_GetParamNum(hPenStyle,
                                      OGRSTPenWidth, &bIsNull);
        if (bIsNull)
          nPenSize = 1;
        if (pszPenName!=NULL) {
          // Try to match pen name in symbol file
          nPenSymbol = msOGRGetSymbolId(&(map->symbolset),
                                        pszPenName, NULL, MS_FALSE);
        }
      }

      if (layer->debug >= MS_DEBUGLEVEL_VVV)
        msDebug("** PEN COLOR = %d %d %d **\n", oPenColor.red,oPenColor.green,oPenColor.blue);

      pszPattern = OGR_ST_GetParamStr(hPenStyle, OGRSTPenPattern, &bIsNull);
      if (bIsNull) pszPattern = NULL;
      if( pszPattern != NULL )
      {
          char** papszTokens = CSLTokenizeStringComplex(pszPattern, " ", FALSE, FALSE);
          int nTokenCount = CSLCount(papszTokens);
          int bValidFormat = TRUE;
          if( nTokenCount >= 2 && nTokenCount <= MS_MAXPATTERNLENGTH)
          {
              for(int i=0;i<nTokenCount;i++)
              {
                  if( strlen(papszTokens[i]) > 2 &&
                      strcmp(papszTokens[i] + strlen(papszTokens[i]) - 2, "px") == 0 )
                  {
                      pattern[patternlength++] = CPLAtof(papszTokens[i]);
                  }
                  else
                  {
                      bValidFormat = FALSE;
                      patternlength = 0;
                      break;
                  }
              }
          }
          else
              bValidFormat = FALSE;
          if( !bValidFormat && layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug("Invalid/unhandled pen pattern format = %s\n", pszPattern);
          CSLDestroy(papszTokens);
      }
      
      pszCap = OGR_ST_GetParamStr(hPenStyle, OGRSTPenCap, &bIsNull);
      if (bIsNull) pszCap = NULL;
      if( pszCap != NULL )
      {
          /* Note: the default in OGR Feature style is BUTT, but the MapServer */
          /* default is ROUND. Currently use MapServer default. */
          if( strcmp(pszCap, "b") == 0 ) /* BUTT */
              linecap = MS_CJC_BUTT;
          else if( strcmp(pszCap, "r") == 0 ) /* ROUND */
              linecap = MS_CJC_ROUND;
          else if( strcmp(pszCap, "p") == 0 ) /* PROJECTING */
              linecap = MS_CJC_SQUARE;
          else if( layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug("Invalid/unhandled pen cap = %s\n", pszCap);
      }
      
      pszJoin = OGR_ST_GetParamStr(hPenStyle, OGRSTPenJoin, &bIsNull);
      if (bIsNull) pszJoin = NULL;
      if( pszJoin != NULL )
      {
          /* Note: the default in OGR Feature style is MITER, but the MapServer */
          /* default is NONE. Currently use MapServer default. */
          if( strcmp(pszJoin, "m") == 0 ) /* MITTER */
              linejoin = MS_CJC_MITER;
          else if( strcmp(pszJoin, "r") == 0 ) /* ROUND */
              linejoin = MS_CJC_ROUND;
          else if( strcmp(pszJoin, "b") == 0 ) /* BEVEL */
              linejoin = MS_CJC_BEVEL;
          else if( layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug("Invalid/unhandled pen join = %s\n", pszJoin);
      }

      offsetx = OGR_ST_GetParamDbl(hPenStyle, OGRSTPenPerOffset, &bIsNull);
      if( bIsNull ) offsetx = 0;
      if( offsetx != 0.0 )
      {
          /* OGR feature style and MapServer conventions related to offset */
          /* sign are the same : negative values for left of line, positive for */
          /* right of line */
          offsety = MS_STYLE_SINGLE_SIDED_OFFSET;
      }

      if (bIsBrush || layer->type == MS_LAYER_POLYGON) {
        // This is a multipart symbology, so pen defn goes in the
        // overlaysymbol params
        s->outlinecolor = oPenColor;
      } else {
        // Single part symbology
        s->color = oPenColor;
      }

      s->symbol = nPenSymbol;
      s->size = nPenSize;
      s->width = nPenSize;
      s->linecap = linecap;
      s->linejoin = linejoin;
      s->offsetx = offsetx;
      s->offsety = offsety;
      s->patternlength = patternlength;
      if( patternlength > 0 )
          memcpy(s->pattern, pattern, sizeof(double) * patternlength);

      int nPriority = OGR_ST_GetParamNum(hPenStyle, OGRSTPenPriority, &bIsNull);
      if( !bIsNull )
          *pbPriority = nPriority;

      return MS_SUCCESS;
}

static int msOGRAddBgColorStyleParseBrush(styleObj* s, OGRStyleToolH hBrushStyle)
{
    GBool bIsNull;
    int r = 0, g = 0, b = 0, t = 0;
    const char* pszColor = OGR_ST_GetParamStr(hBrushStyle,
        OGRSTBrushBColor, &bIsNull);

    if (!bIsNull && OGR_ST_GetRGBFromString(hBrushStyle,
        pszColor,
        &r, &g, &b, &t)) {
        MS_INIT_COLOR(s->color, r, g, b, t);
    }
    return MS_SUCCESS;
}

static int msOGRUpdateStyleParseBrush(mapObj *map, layerObj *layer, styleObj *s,
                                      OGRStyleToolH hBrushStyle, int* pbIsBrush,
                                      int* pbPriority)
{
  GBool bIsNull;
  int r=0,g=0,b=0,t=0;

      const char *pszBrushName = OGR_ST_GetParamStr(hBrushStyle,
                                 OGRSTBrushId,
                                 &bIsNull);
      if (bIsNull) pszBrushName = NULL;

      // Check for Brush Pattern "ogr-brush-1": the invisible fill
      // If that's what we have then set fill color to -1
      if (pszBrushName && strstr(pszBrushName, "ogr-brush-1") != NULL) {
        MS_INIT_COLOR(s->color, -1, -1, -1, 255);
      } else {
        *pbIsBrush = TRUE;
        const char *pszColor = OGR_ST_GetParamStr(hBrushStyle,
                               OGRSTBrushFColor,
                               &bIsNull);
        if (!bIsNull && OGR_ST_GetRGBFromString(hBrushStyle,
                                                pszColor,
                                                &r, &g, &b, &t)) {
          MS_INIT_COLOR(s->color, r, g, b, t);

          if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug("** BRUSH COLOR = %d %d %d **\n", r,g,b);
        }

        // Symbol name mapping:
        // First look for the native symbol name, then the ogr-...
        // generic name.
        // If none provided or found then use 0: solid fill

        const char *pszName = OGR_ST_GetParamStr(hBrushStyle,
                              OGRSTBrushId,
                              &bIsNull);
        s->symbol = msOGRGetSymbolId(&(map->symbolset),
                                                pszName, NULL, MS_FALSE);

        double angle = OGR_ST_GetParamDbl(hBrushStyle, OGRSTBrushAngle, &bIsNull);
        if( !bIsNull )
            s->angle = angle;
        
        double size = OGR_ST_GetParamDbl(hBrushStyle, OGRSTBrushSize, &bIsNull);
        if( !bIsNull )
            s->size = size;
        
        double spacingx = OGR_ST_GetParamDbl(hBrushStyle, OGRSTBrushDx, &bIsNull);
        if( !bIsNull )
        {
            double spacingy = OGR_ST_GetParamDbl(hBrushStyle, OGRSTBrushDy, &bIsNull);
            if( !bIsNull )
            {
                if( spacingx == spacingy )
                    s->gap = spacingx;
                else if( layer->debug >= MS_DEBUGLEVEL_VVV )
                    msDebug("Ignoring brush dx and dy since they don't have the same value\n");
            }
        }
      }

      int nPriority = OGR_ST_GetParamNum(hBrushStyle, OGRSTBrushPriority, &bIsNull);
      if( !bIsNull )
          *pbPriority = nPriority;

      return MS_SUCCESS;
}

static int msOGRUpdateStyleParseSymbol(mapObj *map, styleObj *s,
                                       OGRStyleToolH hSymbolStyle,
                                       int* pbPriority)
{
  GBool bIsNull;
  int r=0,g=0,b=0,t=0;

      const char *pszColor = OGR_ST_GetParamStr(hSymbolStyle,
                             OGRSTSymbolColor,
                             &bIsNull);
      if (!bIsNull && OGR_ST_GetRGBFromString(hSymbolStyle,
                                              pszColor,
                                              &r, &g, &b, &t)) {
        MS_INIT_COLOR(s->color, r, g, b, t);
      }

      pszColor = OGR_ST_GetParamStr(hSymbolStyle,
                                    OGRSTSymbolOColor,
                                    &bIsNull);
      if (!bIsNull && OGR_ST_GetRGBFromString(hSymbolStyle,
                                              pszColor,
                                              &r, &g, &b, &t)) {
        MS_INIT_COLOR(s->outlinecolor, r, g, b, t);
      }

      s->angle = OGR_ST_GetParamNum(hSymbolStyle,
                            OGRSTSymbolAngle,
                            &bIsNull);
      double dfTmp = OGR_ST_GetParamNum(hSymbolStyle, OGRSTSymbolSize, &bIsNull);
      if (!bIsNull)
        s->size = dfTmp;

      // Symbol name mapping:
      // First look for the native symbol name, then the ogr-...
      // generic name, and in last resort try "default-marker" if
      // provided by user.
      const char *pszName = OGR_ST_GetParamStr(hSymbolStyle,
                            OGRSTSymbolId,
                            &bIsNull);
      if (bIsNull)
        pszName = NULL;

      int try_addimage_if_notfound = MS_FALSE;
#ifdef USE_CURL
      if (pszName && strncasecmp(pszName, "http", 4) == 0)
        try_addimage_if_notfound =MS_TRUE;
#endif
      if (!s->symbolname)
        s->symbol = msOGRGetSymbolId(&(map->symbolset),
                                                pszName,
                                                "default-marker",  try_addimage_if_notfound);

      int nPriority = OGR_ST_GetParamNum(hSymbolStyle, OGRSTSymbolPriority, &bIsNull);
      if( !bIsNull )
          *pbPriority = nPriority;

      return MS_SUCCESS;
}


/**********************************************************************
 *                     msOGRLayerGetAutoStyle()
 *
 * Fills a classObj with style info from the specified shape.
 * For optimal results, this should be called immediately after
 * GetNextShape() or GetShape() so that the shape doesn't have to be read
 * twice.
 *
 * The returned classObj is a ref. to a static structure valid only until
 * the next call and that shouldn't be freed by the caller.
 **********************************************************************/
static int msOGRLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c,
                                  shapeObj* shape)
{
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;

  if (psInfo == NULL || psInfo->hLayer == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!",
               "msOGRLayerGetAutoStyle()");
    return(MS_FAILURE);
  }

  if( layer->tileindex != NULL ) {
    if( (psInfo->poCurTile == NULL || shape->tileindex != psInfo->poCurTile->nTileId)
        && msOGRFileReadTile( layer, psInfo ) != MS_SUCCESS )
      return MS_FAILURE;

    psInfo = psInfo->poCurTile;
  }

  /* ------------------------------------------------------------------
   * Read shape or reuse ref. to last shape read.
   * ------------------------------------------------------------------ */
  ACQUIRE_OGR_LOCK;
  if (psInfo->hLastFeature == NULL ||
      psInfo->last_record_index_read != shape->resultindex) {
    RELEASE_OGR_LOCK;
    msSetError(MS_MISCERR,
               "Assertion failed: AutoStyle not requested on loaded shape.",
               "msOGRLayerGetAutoStyle()");
    return(MS_FAILURE);
  }

  /* ------------------------------------------------------------------
   * Reset style info in the class to defaults
   * the only members we don't touch are name, expression, and join/query stuff
   * ------------------------------------------------------------------ */
  resetClassStyle(c);
  if (msMaybeAllocateClassStyle(c, 0)) {
    RELEASE_OGR_LOCK;
    return(MS_FAILURE);
  }

  // __TODO__ label cache incompatible with styleitem feature.
  layer->labelcache = MS_OFF;

  int nRetVal = MS_SUCCESS;
  if (psInfo->hLastFeature) {
    OGRStyleMgrH hStyleMgr = OGR_SM_Create(NULL);
    OGR_SM_InitFromFeature(hStyleMgr, psInfo->hLastFeature);
    nRetVal = msOGRUpdateStyle(hStyleMgr, map, layer, c);
    OGR_SM_Destroy(hStyleMgr);
  }

  RELEASE_OGR_LOCK;
  return nRetVal;
}


/**********************************************************************
 *                     msOGRUpdateStyleFromString()
 *
 * Fills a classObj with style info from the specified style string.
 * For optimal results, this should be called immediately after
 * GetNextShape() or GetShape() so that the shape doesn't have to be read
 * twice.
 *
 * The returned classObj is a ref. to a static structure valid only until
 * the next call and that shouldn't be freed by the caller.
 **********************************************************************/
int msOGRUpdateStyleFromString(mapObj *map, layerObj *layer, classObj *c,
                               const char *stylestring)
{
  /* ------------------------------------------------------------------
   * Reset style info in the class to defaults
   * the only members we don't touch are name, expression, and join/query stuff
   * ------------------------------------------------------------------ */
  resetClassStyle(c);
  if (msMaybeAllocateClassStyle(c, 0)) {
    return(MS_FAILURE);
  }

  // __TODO__ label cache incompatible with styleitem feature.
  layer->labelcache = MS_OFF;

  int nRetVal = MS_SUCCESS;

  ACQUIRE_OGR_LOCK;

  OGRStyleMgrH hStyleMgr = OGR_SM_Create(NULL);
  OGR_SM_InitStyleString(hStyleMgr, stylestring);
  nRetVal = msOGRUpdateStyle(hStyleMgr, map, layer, c);
  OGR_SM_Destroy(hStyleMgr);

  RELEASE_OGR_LOCK;
  return nRetVal;
}

/************************************************************************/
/*                           msOGRLCleanup()                            */
/************************************************************************/

void msOGRCleanup( void )

{
  ACQUIRE_OGR_LOCK;
  if( bOGRDriversRegistered == MS_TRUE ) {
    CPLPopErrorHandler();
    bOGRDriversRegistered = MS_FALSE;
  }
  RELEASE_OGR_LOCK;
}

/************************************************************************/
/*                           msOGREscapeSQLParam                        */
/************************************************************************/
char *msOGREscapePropertyName(layerObj *layer, const char *pszString)
{
  char* pszEscapedStr =NULL;
  if(layer && pszString && strlen(pszString) > 0) {
    pszEscapedStr = (char*) msSmallMalloc( strlen(pszString) * 2 + 1 );
    int j = 0;
    for( int i = 0; pszString[i] != '\0'; ++i )
    {
        if( pszString[i] == '"' )
        {
            pszEscapedStr[j++] = '"';
            pszEscapedStr[j++] = '"';
        }
        else
            pszEscapedStr[j++] = pszString[i];
    }
    pszEscapedStr[j] = 0;
  }
  return pszEscapedStr;
}

static int msOGRLayerSupportsCommonFilters(layerObj *)
{
  return MS_FALSE;
}

static void msOGREnablePaging(layerObj *layer, int value)
{
  msOGRFileInfo *layerinfo = NULL;

  if (layer->debug) {
    msDebug("msOGREnablePaging(%d) called.\n", value);
  }

  if(!msOGRLayerIsOpen(layer))
    msOGRLayerOpenVT(layer);

  assert( layer->layerinfo != NULL);

  layerinfo = (msOGRFileInfo *)layer->layerinfo;
  layerinfo->bPaging = value;
}

static int msOGRGetPaging(layerObj *layer)
{
  msOGRFileInfo *layerinfo = NULL;

  if (layer->debug) {
    msDebug("msOGRGetPaging called.\n");
  }

  if(!msOGRLayerIsOpen(layer))
    msOGRLayerOpenVT(layer);

  assert( layer->layerinfo != NULL);

  layerinfo = (msOGRFileInfo *)layer->layerinfo;
  return layerinfo->bPaging;
}

/************************************************************************/
/*                  msOGRLayerInitializeVirtualTable()                  */
/************************************************************************/
int msOGRLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerTranslateFilter = msOGRTranslateMsExpressionToOGRSQL;

  layer->vtable->LayerSupportsCommonFilters = msOGRLayerSupportsCommonFilters;
  layer->vtable->LayerInitItemInfo = msOGRLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msOGRLayerFreeItemInfo;
  layer->vtable->LayerOpen = msOGRLayerOpenVT;
  layer->vtable->LayerIsOpen = msOGRLayerIsOpen;
  layer->vtable->LayerWhichShapes = msOGRLayerWhichShapes;
  layer->vtable->LayerNextShape = msOGRLayerNextShape;
  layer->vtable->LayerGetShape = msOGRLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */
  layer->vtable->LayerClose = msOGRLayerClose;
  layer->vtable->LayerGetItems = msOGRLayerGetItems;
  layer->vtable->LayerGetExtent = msOGRLayerGetExtent;
  layer->vtable->LayerGetAutoStyle = msOGRLayerGetAutoStyle;
  /* layer->vtable->LayerCloseConnection, use default */
  layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;
  layer->vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  layer->vtable->LayerGetNumFeatures = msOGRLayerGetNumFeatures;
  /* layer->vtable->LayerGetAutoProjection, use defaut*/

  layer->vtable->LayerEscapeSQLParam = msOGREscapeSQLParam;
  layer->vtable->LayerEscapePropertyName = msOGREscapePropertyName;
  layer->vtable->LayerEnablePaging = msOGREnablePaging;
  layer->vtable->LayerGetPaging = msOGRGetPaging;
  return MS_SUCCESS;
}

/************************************************************************/
/*                         msOGRShapeFromWKT()                          */
/************************************************************************/
shapeObj *msOGRShapeFromWKT(const char *string)
{

  OGRGeometryH hGeom = NULL;
  shapeObj *shape=NULL;

  if(!string)
    return NULL;

  if( OGR_G_CreateFromWkt( (char **)&string, NULL, &hGeom ) != OGRERR_NONE ) {
    msSetError(MS_OGRERR, "Failed to parse WKT string.",
               "msOGRShapeFromWKT()" );
    return NULL;
  }

  /* Initialize a corresponding shapeObj */

  shape = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(shape);

  /* translate WKT into an OGRGeometry. */

  if( msOGRGeometryToShape( hGeom, shape,
                            wkbFlatten(OGR_G_GetGeometryType(hGeom)) )
      == MS_FAILURE ) {
    free( shape );
    return NULL;
  }

  OGR_G_DestroyGeometry( hGeom );

  return shape;
}

/************************************************************************/
/*                          msOGRShapeToWKT()                           */
/************************************************************************/
char *msOGRShapeToWKT(shapeObj *shape)
{
  OGRGeometryH hGeom = NULL;
  int          i;
  char        *wkt = NULL;

  if(!shape)
    return NULL;

  if( shape->type == MS_SHAPE_POINT && shape->numlines == 1
      && shape->line[0].numpoints == 1 ) {
    hGeom = OGR_G_CreateGeometry( wkbPoint );
    OGR_G_SetPoint_2D( hGeom, 0,
                       shape->line[0].point[0].x,
                       shape->line[0].point[0].y );
  } else if( shape->type == MS_SHAPE_POINT && shape->numlines == 1
             && shape->line[0].numpoints > 1 ) {
    hGeom = OGR_G_CreateGeometry( wkbMultiPoint );
    for( i = 0; i < shape->line[0].numpoints; i++ ) {
      OGRGeometryH hPoint;

      hPoint = OGR_G_CreateGeometry( wkbPoint );
      OGR_G_SetPoint_2D( hPoint, 0,
                         shape->line[0].point[i].x,
                         shape->line[0].point[i].y );
      OGR_G_AddGeometryDirectly( hGeom, hPoint );
    }
  } else if( shape->type == MS_SHAPE_LINE && shape->numlines == 1 ) {
    hGeom = OGR_G_CreateGeometry( wkbLineString );
    for( i = 0; i < shape->line[0].numpoints; i++ ) {
      OGR_G_AddPoint_2D( hGeom,
                         shape->line[0].point[i].x,
                         shape->line[0].point[i].y );
    }
  } else if( shape->type == MS_SHAPE_LINE && shape->numlines > 1 ) {
    OGRGeometryH hMultiLine = OGR_G_CreateGeometry( wkbMultiLineString );
    int iLine;

    for( iLine = 0; iLine < shape->numlines; iLine++ ) {
      hGeom = OGR_G_CreateGeometry( wkbLineString );
      for( i = 0; i < shape->line[iLine].numpoints; i++ ) {
        OGR_G_AddPoint_2D( hGeom,
                           shape->line[iLine].point[i].x,
                           shape->line[iLine].point[i].y );
      }

      OGR_G_AddGeometryDirectly( hMultiLine, hGeom );
    }

    hGeom = hMultiLine;
  } else if( shape->type == MS_SHAPE_POLYGON ) {
    int iLine;

    /* actually, it is pretty hard to be sure rings 1+ are interior */
    hGeom = OGR_G_CreateGeometry( wkbPolygon );
    for( iLine = 0; iLine < shape->numlines; iLine++ ) {
      OGRGeometryH hRing;
      hRing = OGR_G_CreateGeometry( wkbLinearRing );

      for( i = 0; i < shape->line[iLine].numpoints; i++ ) {
        OGR_G_AddPoint_2D( hRing,
                           shape->line[iLine].point[i].x,
                           shape->line[iLine].point[i].y );
      }
      OGR_G_AddGeometryDirectly( hGeom, hRing );
    }
  } else {
    msSetError(MS_OGRERR, "OGR support is not available.", "msOGRShapeToWKT()");
  }

  if( hGeom != NULL ) {
    char *pszOGRWkt;

    OGR_G_ExportToWkt( hGeom, &pszOGRWkt );
    wkt = msStrdup( pszOGRWkt );
    CPLFree( pszOGRWkt );
  }

  return wkt;
}

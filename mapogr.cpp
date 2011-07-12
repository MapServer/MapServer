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

#if defined(USE_OGR) || defined(USE_GDAL)
#  include "gdal_version.h"
#  include "cpl_conv.h"
#  include "cpl_string.h"
#  include "ogr_srs_api.h"
#endif

MS_CVSID("$Id$")

#if defined(GDAL_VERSION_NUM) && (GDAL_VERSION_NUM < 1400)
#  define ACQUIRE_OLD_OGR_LOCK   msAcquireLock( TLOCK_OGR )
#  define RELEASE_OLD_OGR_LOCK   msReleaseLock( TLOCK_OGR )
#else
#  define ACQUIRE_OLD_OGR_LOCK 
#  define RELEASE_OLD_OGR_LOCK 
#endif

#define ACQUIRE_OGR_LOCK       msAcquireLock( TLOCK_OGR )
#define RELEASE_OGR_LOCK       msReleaseLock( TLOCK_OGR )

#ifdef USE_OGR

#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
#include "ogr_api.h"
#else /* Use OGR Style C++ */
#include "ogrsf_frmts.h"
#include "ogr_featurestyle.h"
#endif

typedef struct ms_ogr_file_info_t
{
  char        *pszFname;
  int         nLayerIndex;
  OGRDataSourceH hDS;
  OGRLayerH   hLayer;
  OGRFeatureH hLastFeature;

  int         nTileId;                  /* applies on the tiles themselves. */

  struct ms_ogr_file_info_t *poCurTile; /* exists on tile index, -> tiles */
  rectObj     rect;                     /* set by WhichShapes */

  int         last_record_index_read;

} msOGRFileInfo;

static int msOGRLayerIsOpen(layerObj *layer);
static int msOGRLayerInitItemInfo(layerObj *layer);
static int msOGRLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c, 
                                  int tile, long record);
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
static void ogrPointsAddPoint(lineObj *line, double dX, double dY, 
                              int lineindex, rectObj *bounds)
{
    /* Keep track of shape bounds */
    if (line->numpoints == 0 && lineindex == 0)
    {
        bounds->minx = bounds->maxx = dX;
        bounds->miny = bounds->maxy = dY;
    }
    else
    {
        if (dX < bounds->minx)  bounds->minx = dX;
        if (dX > bounds->maxx)  bounds->maxx = dX;
        if (dY < bounds->miny)  bounds->miny = dY;
        if (dY > bounds->maxy)  bounds->maxy = dY;
    }

    line->point[line->numpoints].x = dX;
    line->point[line->numpoints].y = dY;
#ifdef USE_POINT_Z_M
    line->point[line->numpoints].z = 0.0;
    line->point[line->numpoints].m = 0.0;
#endif
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
  switch( eGType )
  {
      case wkbGeometryCollection:
      case wkbMultiLineString:
      case wkbMultiPolygon:
      case wkbPolygon:
      {
          /* Treat it as GeometryCollection */
          for (int iGeom=0; iGeom < OGR_G_GetGeometryCount( hGeom ); iGeom++ )
          {
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
  if ( eGType == wkbPoint )
  {
      numpoints = 1;
  }
  else if ( eGType == wkbLineString 
            ||  eGType == wkbLinearRing )
  {
      numpoints = OGR_G_GetPointCount( hGeom );
  }
  else if ( eGType == wkbMultiPoint )
  {
      numpoints = OGR_G_GetGeometryCount( hGeom );
  }
  else
  {
      msSetError(MS_OGRERR, 
                 "OGRGeometry type `%s' not supported yet.", 
                 "ogrGeomPoints()",
                 OGR_G_GetGeometryName( hGeom ) );
      return(-1);
  }

/* ------------------------------------------------------------------
 * Do we need to allocate a line object to contain all our points? 
 * ------------------------------------------------------------------ */
  if( outshp->numlines == 0 )
  {
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
  
  if(!line->point) 
  {
      msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", 
                 "ogrGeomPoints()");
      return(-1);
  }
   
/* ------------------------------------------------------------------
 * alloc buffer and filter/transform points
 * ------------------------------------------------------------------ */
  if( eGType == wkbPoint )
  {
      ogrPointsAddPoint(line, OGR_G_GetX(hGeom, 0), OGR_G_GetY(hGeom, 0), 
                        outshp->numlines-1, &(outshp->bounds));
  }
  else if( eGType == wkbLineString 
           || eGType == wkbLinearRing )
  {
      for(i=0; i<numpoints; i++)
          ogrPointsAddPoint(line, OGR_G_GetX(hGeom, i), OGR_G_GetY(hGeom, i),
                            outshp->numlines-1, &(outshp->bounds));
  }
  else if( eGType == wkbMultiPoint )
  {
      for(i=0; i<numpoints; i++)
      {
          OGRGeometryH hPoint = OGR_G_GetGeometryRef( hGeom, i );
          ogrPointsAddPoint(line, OGR_G_GetX(hPoint, 0), OGR_G_GetY(hPoint, 0),
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
       || eGType == wkbMultiPolygon )
  {
      if (eGType == wkbPolygon && outshp->type == MS_SHAPE_NULL)
          outshp->type = MS_SHAPE_POLYGON;

      /* Treat it as GeometryCollection */
      for (int iGeom=0; iGeom < OGR_G_GetGeometryCount( hGeom ); iGeom++ )
      {
          if( ogrGeomLine( OGR_G_GetGeometryRef( hGeom, iGeom ), 
                           outshp, bCloseRings ) == -1 )
              return -1;
      }
  }
/* ------------------------------------------------------------------
 * OGRPoint and OGRMultiPoint
 * ------------------------------------------------------------------ */
  else if ( eGType == wkbPoint || eGType == wkbMultiPoint )
  {
      /* Hummmm a point when we're drawing lines/polygons... just drop it! */
  }
/* ------------------------------------------------------------------
 * OGRLinearRing/OGRLineString ... both are of type wkbLineString
 * ------------------------------------------------------------------ */
  else if ( eGType == wkbLineString )
  {
      int       j, numpoints;
      lineObj   line={0,NULL};
      double    dX, dY;

      if ((numpoints = OGR_G_GetPointCount( hGeom )) < 2)
          return 0;

      if (outshp->type == MS_SHAPE_NULL)
          outshp->type = MS_SHAPE_LINE;

      line.numpoints = 0;
      line.point = (pointObj *)malloc(sizeof(pointObj)*(numpoints+1));
      if(!line.point) 
      {
          msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", 
                     "ogrGeomLine");
          return(-1);
      }

      for(j=0; j<numpoints; j++)
      {
          dX = line.point[j].x = OGR_G_GetX( hGeom, j); 
          dY = line.point[j].y = OGR_G_GetY( hGeom, j);

          /* Keep track of shape bounds */
          if (j == 0 && outshp->numlines == 0)
          {
              outshp->bounds.minx = outshp->bounds.maxx = dX;
              outshp->bounds.miny = outshp->bounds.maxy = dY;
          }
          else
          {
              if (dX < outshp->bounds.minx)  outshp->bounds.minx = dX;
              if (dX > outshp->bounds.maxx)  outshp->bounds.maxx = dX;
              if (dY < outshp->bounds.miny)  outshp->bounds.miny = dY;
              if (dY > outshp->bounds.maxy)  outshp->bounds.maxy = dY;
          }

      }
      line.numpoints = numpoints; 

      if (bCloseRings &&
          ( line.point[line.numpoints-1].x != line.point[0].x ||
            line.point[line.numpoints-1].y != line.point[0].y  ) )
      {
          line.point[line.numpoints].x = line.point[0].x;
          line.point[line.numpoints].y = line.point[0].y;
          line.numpoints++;
      }

      msAddLineDirectly(outshp, &line);
  }
  else
  {
      msSetError(MS_OGRERR, 
                 "OGRGeometry type `%s' not supported.",
                 "ogrGeomLine()",
                 OGR_G_GetGeometryName( hGeom ) );
      return(-1);
  }

  return(0);
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

  if (hGeom == NULL)
  {
      // Empty geometry... this is not an error... we'll just skip it
      return MS_SUCCESS;
  }

  switch(layertype) 
  {
/* ------------------------------------------------------------------
 *      POINT layer - Any geometry can be converted to point/multipoint
 * ------------------------------------------------------------------ */
    case MS_LAYER_POINT:
      if(ogrGeomPoints(hGeom, outshp) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      break;
/* ------------------------------------------------------------------
 *      LINE layer
 * ------------------------------------------------------------------ */
    case MS_LAYER_LINE:
      if(ogrGeomLine(hGeom, outshp, MS_FALSE) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      if (outshp->type != MS_SHAPE_LINE && outshp->type != MS_SHAPE_POLYGON)
          outshp->type = MS_SHAPE_NULL;  // Incompatible type for this layer
      break;
/* ------------------------------------------------------------------
 *      POLYGON layer
 * ------------------------------------------------------------------ */
    case MS_LAYER_POLYGON:
      if(ogrGeomLine(hGeom, outshp, MS_TRUE) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      if (outshp->type != MS_SHAPE_POLYGON)
          outshp->type = MS_SHAPE_NULL;  // Incompatible type for this layer
      break;
/* ------------------------------------------------------------------
 *      MS_ANNOTATION layer - return real feature type
 * ------------------------------------------------------------------ */
    case MS_LAYER_ANNOTATION:
    case MS_LAYER_CHART:
    case MS_LAYER_QUERY:
      switch( OGR_G_GetGeometryType( hGeom ) )
      {
        case wkbPoint:
        case wkbPoint25D:
        case wkbMultiPoint:
        case wkbMultiPoint25D:
          if(ogrGeomPoints(hGeom, outshp) == -1)
          {
              nStatus = MS_FAILURE; // Error message already produced.
          }
          break;
        default:
          // Handle any non-point types as lines/polygons ... ogrGeomLine()
          // will decide the shape type
          if(ogrGeomLine(hGeom, outshp, MS_FALSE) == -1)
          {
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
    if (hGeometry && psShape && nType > 0)
    {
        if (nType == wkbPoint || nType == wkbMultiPoint )
            return ogrConvertGeometry(hGeometry, psShape,  MS_LAYER_POINT);
        else if (nType == wkbLineString || nType == wkbMultiLineString)
            return ogrConvertGeometry(hGeometry, psShape,  MS_LAYER_LINE);
        else if (nType == wkbPolygon || nType == wkbMultiPolygon)
            return ogrConvertGeometry(hGeometry, psShape,  MS_LAYER_POLYGON);
        else
            return MS_FAILURE;
    }
    else
        return MS_FAILURE;
}


/* ==================================================================
 * Attributes handling functions
 * ================================================================== */

// Special field index codes for handling text string and angle coming from
// OGR style strings.
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

  if((values = (char **)malloc(sizeof(char *)*layer->numitems)) == NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRGetValues()");
    return(NULL);
  }

#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
  OGRStyleMgrH  hStyleMgr = NULL;
  OGRStyleToolH hLabelStyle = NULL;
#else
  OGRStyleMgr *poStyleMgr = NULL;
  OGRStyleLabel *poLabelStyle = NULL;
#endif
  int *itemindexes = (int*)layer->iteminfo;

  for(i=0;i<layer->numitems;i++)
  {
    if (itemindexes[i] >= 0)
    {
        // Extract regular attributes
        values[i] = strdup(OGR_F_GetFieldAsString( hFeature, itemindexes[i]));
    }
    else
#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
    {
        // Handle special OGR attributes coming from StyleString
        if (!hStyleMgr)
        {
            hStyleMgr = OGR_SM_Create(NULL);
            OGR_SM_InitFromFeature(hStyleMgr, hFeature);
            OGRStyleToolH hStylePart = OGR_SM_GetPart(hStyleMgr, 0, NULL);
            if (hStylePart && OGR_ST_GetType(hStylePart) == OGRSTCLabel)
                hLabelStyle = hStylePart;
            else if (hStylePart)
            {
                OGR_ST_Destroy(hStylePart);
                hStylePart =  NULL;
            }

          /* Setting up the size units according to msOGRLayerGetAutoStyle*/
          if (hStylePart && layer->map)
            OGR_ST_SetUnit(hStylePart, OGRSTUPixel, layer->map->cellsize*72.0*39.37);
        }
        int bDefault;
        if (itemindexes[i] == MSOGR_LABELTEXTINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelTextString,
                                                          &bDefault)) == NULL))
                values[i] = strdup("");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELTEXTNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELANGLEINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelAngle,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELANGLENAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELSIZEINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelSize,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELSIZENAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELFCOLORINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelFColor,
                                                          &bDefault)) == NULL))
                values[i] = strdup("#000000");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELFCOLORNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELFONTNAMEINDEX )
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelFontName,
                                                          &bDefault)) == NULL))
                values[i] = strdup("Arial");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELFONTNAMENAME " =       \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELBCOLORINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelBColor,
                                                          &bDefault)) == NULL))
                values[i] = strdup("#000000");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELBCOLORNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELPLACEMENTINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelPlacement,
                                                          &bDefault)) == NULL))
                values[i] = strdup("");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELPLACEMENTNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELANCHORINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelAnchor,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELANCHORNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELDXINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelDx,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELDXNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELDYINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelDy,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELDYNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELPERPINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelPerp,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELPERPNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELBOLDINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelBold,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELBOLDNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELITALICINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelItalic,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELITALICNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELUNDERLINEINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelUnderline,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELUNDERLINENAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELPRIORITYINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelPriority,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELPRIORITYNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELSTRIKEOUTINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelStrikeout,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELSTRIKEOUTNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELSTRETCHINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelStretch,
                                                          &bDefault)) == NULL))
                values[i] = strdup("0");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELSTRETCHNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELADJHORINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelAdjHor,
                                                          &bDefault)) == NULL))
                values[i] = strdup("");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELADJHORNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELADJVERTINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelAdjVert,
                                                          &bDefault)) == NULL))
                values[i] = strdup("");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELADJVERTNAME " = \"%s\"\n", values[i]);
        }
        else if (itemindexes[i] == MSOGR_LABELHCOLORINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelHColor,
                                                          &bDefault)) == NULL))
                values[i] = strdup("");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELHCOLORNAME " = \"%s\"\n", values[i]);
        }
#if GDAL_VERSION_NUM >= 1600
        else if (itemindexes[i] == MSOGR_LABELOCOLORINDEX)
        {
            if (hLabelStyle == NULL 
                || ((pszValue = OGR_ST_GetParamStr(hLabelStyle,
                                                          OGRSTLabelOColor,
                                                          &bDefault)) == NULL))
                values[i] = strdup("");
            else
                values[i] = strdup(pszValue);

            if (layer->debug >= MS_DEBUGLEVEL_VVV)
                msDebug(MSOGR_LABELOCOLORNAME " = \"%s\"\n", values[i]);
        }
#endif /* GDAL_VERSION_NUM >= 1600 */
        else
        {
            msSetError(MS_OGRERR,"Invalid field index!?!","msOGRGetValues()");
            return(NULL);
        }
    }
#else /* OGRStyle C++ */
    {
    if (!poStyleMgr)
    {
        poStyleMgr = new OGRStyleMgr(NULL);
        poStyleMgr->InitFromFeature((OGRFeature *)hFeature);
        OGRStyleTool *poStylePart = poStyleMgr->GetPart(0);
        if (poStylePart && poStylePart->GetType() == OGRSTCLabel)
            poLabelStyle = (OGRStyleLabel*)poStylePart;
        else if (poStylePart)
        {
            delete poStylePart;
            poStylePart = NULL;
        }
        /* Setting up the size units according to msOGRLayerGetAutoStyle*/
        if (poStylePart && layer->map)
            poStylePart->SetUnit(OGRSTUPixel, layer->map->cellsize*72.0*39.37);
    }
    GBool bDefault;
    if (itemindexes[i] == MSOGR_LABELTEXTINDEX)
    {
        if (poLabelStyle == NULL 
            || ((pszValue = poLabelStyle->TextString(bDefault)) == NULL))
           values[i] = strdup("");
        else
            values[i] = strdup(pszValue);
        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELTEXTNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELANGLEINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelAngle,bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);
        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELANGLENAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELSIZEINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelSize,bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);
        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELSIZENAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELFCOLORINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelFColor, bDefault)) == NULL))
           values[i] = strdup("#000000");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELSIZENAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELFONTNAMEINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelFontName, bDefault)) == NULL))
           values[i] = strdup("Arial");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELFONTNAMENAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELBCOLORINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelBColor, bDefault)) == NULL))
           values[i] = strdup("#000000");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELBCOLORNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELPLACEMENTINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelPlacement, bDefault)) == NULL))
           values[i] = strdup("");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELPLACEMENTNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELANCHORINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelAnchor, bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELANCHORNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELDXINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelDx, bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELDXNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELDYINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelDy, bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELDYNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELPERPINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelPerp, bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELPERPNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELBOLDINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelBold, bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELBOLDNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELITALICINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelItalic, bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELITALICNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELUNDERLINEINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelUnderline, bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);
        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELUNDERLINENAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELPRIORITYINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelPriority, bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELPRIORITYNAME " = \"%s\"\n", values[i]);
    }
#if GDAL_VERSION_NUM >= 1400
    else if (itemindexes[i] == MSOGR_LABELSTRIKEOUTINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelStrikeout, bDefault)) == NULL))
            values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELSTRIKEOUTNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELSTRETCHINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelStretch, bDefault)) == NULL))
           values[i] = strdup("0");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELSTRETCHNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELADJHORINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelAdjHor, bDefault)) == NULL))
           values[i] = strdup("");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELADJHORNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELADJVERTINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelAdjVert, bDefault)) == NULL))
           values[i] = strdup("");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELADJVERTNAME " = \"%s\"\n", values[i]);
    }
    else if (itemindexes[i] == MSOGR_LABELHCOLORINDEX)
    {
        if (poLabelStyle == NULL
            || ((pszValue = poLabelStyle->GetParamStr(OGRSTLabelHColor, bDefault)) == NULL))
            values[i] = strdup("");
        else
           values[i] = strdup(pszValue);

        if (layer->debug >= MS_DEBUGLEVEL_VVV)
            msDebug(MSOGR_LABELHCOLORNAME " = \"%s\"\n", values[i]);
    }
#endif
    else
    {
        msSetError(MS_OGRERR,"Invalid field index!?!","msOGRGetValues()");
        return(NULL);
    }
  }
#endif /* OGRStyle C API */
  }

#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
  OGR_SM_Destroy(hStyleMgr);
  OGR_ST_Destroy(hLabelStyle);
#else
  delete poStyleMgr;
  delete poLabelStyle;
#endif

  return(values);
}

#endif  /* USE_OGR */

#if defined(USE_OGR) || defined(USE_GDAL)

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
#ifdef USE_PROJ
  // First flush the "auto" name from the projargs[]... 
  msFreeProjection( proj );

  if (hSRS == NULL || OSRIsLocal( hSRS ) )
  {
      // Dataset had no set projection or is NonEarth (LOCAL_CS)... 
      // Nothing else to do. Leave proj empty and no reprojection will happen!
      return MS_SUCCESS;  
  }

  // Export OGR SRS to a PROJ4 string
  char *pszProj = NULL;

  ACQUIRE_OLD_OGR_LOCK;
  if (OSRExportToProj4( hSRS, &pszProj ) != OGRERR_NONE ||
      pszProj == NULL || strlen(pszProj) == 0)
  {
      RELEASE_OLD_OGR_LOCK;
      msSetError(MS_OGRERR, "Conversion from OGR SRS to PROJ4 failed.",
                 "msOGRSpatialRef2ProjectionObj()");
      CPLFree(pszProj);
      return(MS_FAILURE);
  }

  RELEASE_OLD_OGR_LOCK;

  if( debug_flag )
      msDebug( "AUTO = %s\n", pszProj );

  if( msLoadProjectionString( proj, pszProj ) != 0 )
      return MS_FAILURE;

  CPLFree(pszProj);
#endif

  return MS_SUCCESS;
}
#endif // defined(USE_OGR) || defined(USE_GDAL)

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
#if defined(USE_OGR) || defined(USE_GDAL)

    OGRSpatialReferenceH        hSRS;
    char			*pszAltWKT = (char *) pszWKT;
    OGRErr  eErr;
    int     ms_result;

    ACQUIRE_OLD_OGR_LOCK;
    hSRS = OSRNewSpatialReference( NULL );

    if( !EQUALN(pszWKT,"GEOGCS",6) 
        && !EQUALN(pszWKT,"PROJCS",6)
        && !EQUALN(pszWKT,"LOCAL_CS",8) )
        eErr = OSRSetFromUserInput( hSRS, pszWKT );
    else
        eErr = OSRImportFromWkt( hSRS, &pszAltWKT );

    RELEASE_OLD_OGR_LOCK;

    if( eErr != OGRERR_NONE )
    {
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
#else
    msSetError(MS_OGRERR, 
               "Not implemented since neither OGR nor GDAL is enabled.",
               "msOGCWKT2ProjectionObj()");
    return MS_FAILURE;
#endif
}

/* ==================================================================
 * The following functions closely relate to the API called from
 * maplayer.c, but are intended to be used for the tileindex or direct
 * layer access.
 * ================================================================== */

#ifdef USE_OGR

/**********************************************************************
 *                     msOGRFileOpen()
 *
 * Open an OGR connection, and initialize a msOGRFileInfo.
 **********************************************************************/

static int bOGRDriversRegistered = MS_FALSE;

static msOGRFileInfo *
msOGRFileOpen(layerObj *layer, const char *connection ) 

{
  char *conn_decrypted = NULL;

/* ------------------------------------------------------------------
 * Register OGR Drivers, only once per execution
 * ------------------------------------------------------------------ */
  if (!bOGRDriversRegistered)
  {
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

/* ------------------------------------------------------------------
 * Make sure any encrypted token in the connection string are decrypted
 * ------------------------------------------------------------------ */
  if (connection)
  {
      conn_decrypted = msDecryptStringTokens(layer->map, connection);
      if (conn_decrypted == NULL)
          return NULL;  /* An error should already have been reported */
  }

/* ------------------------------------------------------------------
 * Parse connection string into dataset name, and layer name. 
 * ------------------------------------------------------------------ */
  char *pszDSName = NULL, *pszLayerDef = NULL;

  if( conn_decrypted == NULL )
  {
      /* we don't have anything */
  }
  else if( layer->data != NULL )
  {
      pszDSName = CPLStrdup(conn_decrypted);
      pszLayerDef = CPLStrdup(layer->data);
  }
  else
  {
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

  if( pszDSName == NULL )
  {
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
  if( hDS == NULL )
  {
      char szPath[MS_MAXPATHLEN] = "";
      const char *pszDSSelectedName = pszDSName;
      
      if( layer->debug )
          msDebug("msOGRFileOpen(%s)...\n", connection);
      
      CPLErrorReset();
      if (msTryBuildPath3(szPath, layer->map->mappath, 
                          layer->map->shapepath, pszDSName) != NULL ||
          msTryBuildPath(szPath, layer->map->mappath, pszDSName) != NULL)
      {
          /* Use relative path */
          pszDSSelectedName = szPath;
      }
      
      if( layer->debug )
          msDebug("OGROPen(%s)\n", pszDSSelectedName);

      ACQUIRE_OGR_LOCK;
      hDS = OGROpen( pszDSSelectedName, MS_FALSE, NULL );
      RELEASE_OGR_LOCK;
      
      if( hDS == NULL )
      {
          if( strlen(CPLGetLastErrorMsg()) == 0 )
              msSetError(MS_OGRERR, 
                         "Open failed for OGR connection in layer `%s'.  "
                         "File not found or unsupported format.", 
                         "msOGRFileOpen()",
                         layer->name?layer->name:"(null)" );
          else
              msSetError(MS_OGRERR, 
                         "Open failed for OGR connection in layer `%s'.\n%s\n",
                         "msOGRFileOpen()", 
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

  for( iLayer = 0; iLayer < OGR_DS_GetLayerCount(hDS); iLayer++ )
  {
      hLayer = OGR_DS_GetLayer( hDS, iLayer );
      if( hLayer != NULL 
          && EQUAL(OGR_FD_GetName( OGR_L_GetLayerDefn(hLayer) ),pszLayerDef) )
      {
          nLayerIndex = iLayer;
          break;
      }
      else
          hLayer = NULL;
  }
  
  if( hLayer == NULL && (atoi(pszLayerDef) > 0 || EQUAL(pszLayerDef,"0")) )
  {
      nLayerIndex = atoi(pszLayerDef);
      if( nLayerIndex <  OGR_DS_GetLayerCount(hDS) )
          hLayer = OGR_DS_GetLayer( hDS, nLayerIndex );
  }

  if( hLayer == NULL && EQUALN(pszLayerDef,"SELECT",6) )
  {
      ACQUIRE_OGR_LOCK;
      hLayer = OGR_DS_ExecuteSQL( hDS, pszLayerDef, NULL, NULL );
      if( hLayer == NULL )
      {
          msSetError(MS_OGRERR, 
                     "ExecuteSQL(%s) failed.\n%s",
                     "msOGRFileOpen()", 
                     pszLayerDef, CPLGetLastErrorMsg() );
          OGR_DS_Destroy( hDS );
          CPLFree( pszLayerDef );
          RELEASE_OGR_LOCK;
          return NULL;
      }
      RELEASE_OGR_LOCK;
      nLayerIndex = -1;
  }

  if (hLayer == NULL)
  {
      msSetError(MS_OGRERR, "GetLayer(%s) failed for OGR connection `%s'.",
                 "msOGRFileOpen()", 
                 pszLayerDef, connection );
      CPLFree( pszLayerDef );
      ACQUIRE_OGR_LOCK;
      OGR_DS_Destroy( hDS );
      RELEASE_OGR_LOCK;
      return NULL;
  }

  CPLFree( pszLayerDef );

/* ------------------------------------------------------------------
 * OK... open succeded... alloc and fill msOGRFileInfo inside layer obj
 * ------------------------------------------------------------------ */
  msOGRFileInfo *psInfo =(msOGRFileInfo*)CPLCalloc(1,sizeof(msOGRFileInfo));

  psInfo->pszFname = CPLStrdup(OGR_DS_GetName( hDS ));
  psInfo->nLayerIndex = nLayerIndex;
  psInfo->hDS = hDS;
  psInfo->hLayer = hLayer;

  psInfo->nTileId = 0;
  psInfo->poCurTile = NULL;
  psInfo->rect.minx = psInfo->rect.maxx = 0;
  psInfo->rect.miny = psInfo->rect.maxy = 0;
  psInfo->last_record_index_read = -1;

  return psInfo;
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
  ACQUIRE_OGR_LOCK;

  // Free current tile if there is one.
  if( psInfo->poCurTile != NULL )
      msOGRFileClose( layer, psInfo->poCurTile );

  CPLFree(psInfo);

  RELEASE_OGR_LOCK;

  return MS_SUCCESS;
}

/**********************************************************************
 *                     msOGRFileWhichShapes()
 *
 * Init OGR layer structs ready for calls to msOGRFileNextShape().
 *
 * Returns MS_SUCCESS/MS_FAILURE, or MS_DONE if no shape matching the
 * layer's FILTER overlaps the selected region.
 **********************************************************************/
static int msOGRFileWhichShapes(layerObj *layer, rectObj rect,
                                msOGRFileInfo *psInfo ) 
{
  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRFileWhichShapes()");
    return(MS_FAILURE);
  }

/* ------------------------------------------------------------------
 * Set Spatial filter... this may result in no features being returned
 * if layer does not overlap current view.
 *
 * __TODO__ We should return MS_DONE if no shape overlaps the selected 
 * region and matches the layer's FILTER expression, but there is currently
 * no _efficient_ way to do that with OGR.
 * ------------------------------------------------------------------ */
  ACQUIRE_OGR_LOCK;

  OGRGeometryH hSpatialFilterPolygon = OGR_G_CreateGeometry( wkbPolygon );
  OGRGeometryH hRing = OGR_G_CreateGeometry( wkbLinearRing );

#if GDAL_VERSION_NUM >= 1310
  OGR_G_AddPoint_2D( hRing, rect.minx, rect.miny);
  OGR_G_AddPoint_2D( hRing, rect.maxx, rect.miny);
  OGR_G_AddPoint_2D( hRing, rect.maxx, rect.maxy);
  OGR_G_AddPoint_2D( hRing, rect.minx, rect.maxy);
  OGR_G_AddPoint_2D( hRing, rect.minx, rect.miny);
#else
  OGR_G_AddPoint( hRing, rect.minx, rect.miny, 0);
  OGR_G_AddPoint( hRing, rect.maxx, rect.miny, 0);
  OGR_G_AddPoint( hRing, rect.maxx, rect.maxy, 0);
  OGR_G_AddPoint( hRing, rect.minx, rect.maxy, 0);
  OGR_G_AddPoint( hRing, rect.minx, rect.miny, 0);
#endif

  OGR_G_AddGeometryDirectly( hSpatialFilterPolygon, hRing );

  OGR_L_SetSpatialFilter( psInfo->hLayer, hSpatialFilterPolygon );

  OGR_G_DestroyGeometry( hSpatialFilterPolygon );

  psInfo->rect = rect;

  if (layer->debug >= MS_DEBUGLEVEL_VVV)
      msDebug("msOGRFileWhichShapes: Setting spatial filter to %f %f %f %f\n",
              rect.minx, rect.miny, rect.maxx, rect.maxy );

/* ------------------------------------------------------------------
 * Apply an attribute filter if we have one prefixed with a WHERE 
 * keyword in the filter string.  Otherwise, ensure the attribute
 * filter is clear. 
 * ------------------------------------------------------------------ */
  if( layer->filter.string && EQUALN(layer->filter.string,"WHERE ",6) )
  {
      CPLErrorReset();
      if( OGR_L_SetAttributeFilter( psInfo->hLayer, layer->filter.string+6 )
          != OGRERR_NONE )
      {
          msSetError(MS_OGRERR,
                     "SetAttributeFilter(%s) failed on layer %s.\n%s", 
                     "msOGRFileWhichShapes()",
                     layer->filter.string+6, layer->name?layer->name:"(null)", 
                     CPLGetLastErrorMsg() );
          RELEASE_OGR_LOCK;
          return MS_FAILURE;
      }
  }
  else
      OGR_L_SetAttributeFilter( psInfo->hLayer, NULL );

/* ------------------------------------------------------------------
 * Reset current feature pointer
 * ------------------------------------------------------------------ */
  OGR_L_ResetReading( psInfo->hLayer );
  psInfo->last_record_index_read = -1;

  RELEASE_OGR_LOCK;

  return MS_SUCCESS;
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
  const char *getShapeStyleItems;

  if((hDefn = OGR_L_GetLayerDefn( psInfo->hLayer )) == NULL) 
  {
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

  if((items = (char**)malloc(sizeof(char *)*(totalnumitems+1))) == NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRFileGetItems()");
    return NULL;
  }

  for(i=0;i<numitems;i++)
  {
      OGRFieldDefnH hField = OGR_FD_GetFieldDefn( hDefn, i );
      items[i] = strdup( OGR_Fld_GetNameRef( hField ));
  }

  if (getShapeStyleItems && EQUAL(getShapeStyleItems, "all"))
  {
      assert(numStyleItems == 21);
      items[i++] = strdup( MSOGR_LABELFONTNAMENAME );
      items[i++] = strdup( MSOGR_LABELSIZENAME );
      items[i++] = strdup( MSOGR_LABELTEXTNAME );
      items[i++] = strdup( MSOGR_LABELANGLENAME );
      items[i++] = strdup( MSOGR_LABELFCOLORNAME );
      items[i++] = strdup( MSOGR_LABELBCOLORNAME );
      items[i++] = strdup( MSOGR_LABELPLACEMENTNAME );
      items[i++] = strdup( MSOGR_LABELANCHORNAME );
      items[i++] = strdup( MSOGR_LABELDXNAME );
      items[i++] = strdup( MSOGR_LABELDYNAME );
      items[i++] = strdup( MSOGR_LABELPERPNAME );
      items[i++] = strdup( MSOGR_LABELBOLDNAME );
      items[i++] = strdup( MSOGR_LABELITALICNAME );
      items[i++] = strdup( MSOGR_LABELUNDERLINENAME );
      items[i++] = strdup( MSOGR_LABELPRIORITYNAME );
      items[i++] = strdup( MSOGR_LABELSTRIKEOUTNAME );
      items[i++] = strdup( MSOGR_LABELSTRETCHNAME );
      items[i++] = strdup( MSOGR_LABELADJHORNAME );
      items[i++] = strdup( MSOGR_LABELADJVERTNAME );
      items[i++] = strdup( MSOGR_LABELHCOLORNAME );
      items[i++] = strdup( MSOGR_LABELOCOLORNAME );
  }
  items[i++] = NULL;

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

  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
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
  while (shape->type == MS_SHAPE_NULL)
  {
      if( hFeature )
          OGR_F_Destroy( hFeature );

      if( (hFeature = OGR_L_GetNextFeature( psInfo->hLayer )) == NULL )
      {
          psInfo->last_record_index_read = -1;
          if( CPLGetLastErrorType() == CE_Failure )
          {
              msSetError(MS_OGRERR, "%s", "msOGRFileNextShape()",
                         CPLGetLastErrorMsg() );
              RELEASE_OGR_LOCK;
              return MS_FAILURE;
          }
          else
          {
              RELEASE_OGR_LOCK;
              if (layer->debug >= MS_DEBUGLEVEL_VV)
                  msDebug("msOGRFileNextShape: Returning MS_DONE (no more shapes)\n" );
              return MS_DONE;  // No more features to read
          }
      }

      psInfo->last_record_index_read++;

      if(layer->numitems > 0) 
      {
          shape->values = msOGRGetValues(layer, hFeature);
          shape->numvalues = layer->numitems;
          if(!shape->values)
          {
              OGR_F_Destroy( hFeature );
              RELEASE_OGR_LOCK;
              return(MS_FAILURE);
          }
      }

      // Check the expression unless it is a WHERE clause already 
      // handled by OGR. 
      if( (layer->filter.string && EQUALN(layer->filter.string,"WHERE ",6))
          || msEvalExpression(&(layer->filter), layer->filteritemindex, 
                              shape->values, layer->numitems) == MS_TRUE)
      {
          // Feature matched filter expression... process geometry
          // shape->type will be set if geom is compatible with layer type
          if (ogrConvertGeometry(OGR_F_GetGeometryRef( hFeature ), shape,
                                 layer->type) == MS_SUCCESS)
          {
              if (shape->type != MS_SHAPE_NULL)
                  break; // Shape is ready to be returned!

              if (layer->debug >= MS_DEBUGLEVEL_VVV)
                  msDebug("msOGRFileNextShape: Rejecting feature (shapeid = %d, tileid=%d) of incompatible type for this layer (feature wkbType %d, layer type %d)\n",
                          OGR_F_GetFID( hFeature ), psInfo->nTileId, 
                          wkbFlatten( OGR_G_GetGeometryType( OGR_F_GetGeometryRef( hFeature ) ) ), 
                          layer->type);

          }
          else
          {
              msFreeShape(shape);
              OGR_F_Destroy( hFeature );
              RELEASE_OGR_LOCK;
              return MS_FAILURE; // Error message already produced.
          }
      }

      // Feature rejected... free shape to clear attributes values.
      msFreeShape(shape);
      shape->type = MS_SHAPE_NULL;
  }

  shape->index = psInfo->last_record_index_read;
  shape->tileindex = psInfo->nTileId;

  if (layer->debug >= MS_DEBUGLEVEL_VVV)
      msDebug("msOGRFileNextShape: Returning shape=%d, tile=%d\n",
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

  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
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
  if( record_is_fid )
  {
      ACQUIRE_OGR_LOCK;
      if( (hFeature = OGR_L_GetFeature( psInfo->hLayer, record )) == NULL )
      {
          RELEASE_OGR_LOCK;
          return MS_FAILURE;
      }
  }

/* -------------------------------------------------------------------- */
/*      Support reading shape by offset within the current              */
/*      resultset.                                                      */
/* -------------------------------------------------------------------- */
  else if( !record_is_fid )
  {
      ACQUIRE_OGR_LOCK;
      if( record <= psInfo->last_record_index_read 
          || psInfo->last_record_index_read == -1 )
      {
          OGR_L_ResetReading( psInfo->hLayer );
          psInfo->last_record_index_read = -1;
      }

      hFeature = NULL;
      while( psInfo->last_record_index_read < record )
      {
          if( hFeature != NULL )
          {
              OGR_F_Destroy( hFeature );
              hFeature = NULL;
          }
          if( (hFeature = OGR_L_GetNextFeature( psInfo->hLayer )) == NULL )
          {
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
  if (ogrConvertGeometry(OGR_F_GetGeometryRef( hFeature ), shape,
                         layer->type) != MS_SUCCESS)
  {
      RELEASE_OGR_LOCK;
      return MS_FAILURE; // Error message already produced.
  }
  
  if (shape->type == MS_SHAPE_NULL)
  {
      msSetError(MS_OGRERR, 
                 "Requested feature is incompatible with layer type",
                 "msOGRLayerGetShape()");
      RELEASE_OGR_LOCK;
      return MS_FAILURE;
  }

/* ------------------------------------------------------------------
 * Process shape attributes
 * ------------------------------------------------------------------ */
  if(layer->numitems > 0) 
  {
      shape->values = msOGRGetValues(layer, hFeature);
      shape->numvalues = layer->numitems;
      if(!shape->values)
      {
          RELEASE_OGR_LOCK;
          return(MS_FAILURE);
      }

  }   

  shape->index = record;
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
    if( psInfo->poCurTile != NULL )
    {
        msOGRFileClose( layer, psInfo->poCurTile );
        psInfo->poCurTile = NULL;
    }

/* -------------------------------------------------------------------- */
/*      If -2 is passed, then seek reset reading of the tileindex.      */
/*      We want to start from the beginning even if this file is        */
/*      shared between layers or renders.                               */
/* -------------------------------------------------------------------- */
    ACQUIRE_OGR_LOCK;
    if( targetTile == -2 )
    {
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

    if( hFeature == NULL )
    {
        RELEASE_OGR_LOCK;
        if( targetTile == -1 )
            return MS_DONE;
        else
            return MS_FAILURE;
        
    }

    connection = strdup( OGR_F_GetFieldAsString( hFeature, 
                                                 layer->tileitemindex ));
    
    nFeatureId = OGR_F_GetFID( hFeature );

    OGR_F_Destroy( hFeature );
                        
    RELEASE_OGR_LOCK;

/* -------------------------------------------------------------------- */
/*      Open the new tile file.                                         */
/* -------------------------------------------------------------------- */
    psTileInfo = msOGRFileOpen( layer, connection );

    free( connection );

#ifndef IGNORE_MISSING_DATA
    if( psTileInfo == NULL && targetTile == -1 )
        goto NextFile;
#endif

    if( psTileInfo == NULL )
        return MS_FAILURE;

    psTileInfo->nTileId = nFeatureId;

/* -------------------------------------------------------------------- */
/*      Initialize the spatial query on this file.                      */
/* -------------------------------------------------------------------- */
    if( psInfo->rect.minx != 0 || psInfo->rect.maxx != 0 )
    {
        status = msOGRFileWhichShapes( layer, psInfo->rect, psTileInfo );
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

#endif /* def USE_OGR */

/* ==================================================================
 * Here comes the REAL stuff... the functions below are called by maplayer.c
 * ================================================================== */

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
#ifdef USE_OGR

  msOGRFileInfo *psInfo;

  if (layer->layerinfo != NULL)
  {
      return MS_SUCCESS;  // Nothing to do... layer is already opened
  }

/* -------------------------------------------------------------------- */
/*      If this is not a tiled layer, just directly open the target.    */
/* -------------------------------------------------------------------- */
  if( layer->tileindex == NULL )
  {
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
  else
  {
      // Open tile index

      psInfo = msOGRFileOpen( layer, layer->tileindex );
      layer->layerinfo = psInfo;
      
      if( layer->layerinfo == NULL )
          return MS_FAILURE;

      // Identify TILEITEM

      OGRFeatureDefnH hDefn = OGR_L_GetLayerDefn( psInfo->hLayer );
      for( layer->tileitemindex = 0; 
           layer->tileitemindex < OGR_FD_GetFieldCount( hDefn )
           && !EQUAL( OGR_Fld_GetNameRef( OGR_FD_GetFieldDefn( hDefn, layer->tileitemindex) ),
                      layer->tileitem); 
           layer->tileitemindex++ ) {}

      if( layer->tileitemindex == OGR_FD_GetFieldCount( hDefn ) )
      {
          msSetError(MS_OGRERR, 
                     "Can't identify TILEITEM %s field in TILEINDEX `%s'.",
                     "msOGRLayerOpen()", 
                     layer->tileitem, layer->tileindex );
          msOGRFileClose( layer, psInfo );
          layer->layerinfo = NULL;
          return MS_FAILURE;
      }
  }

/* ------------------------------------------------------------------
 * If projection was "auto" then set proj to the dataset's projection.
 * For a tile index, it is assume the tile index has the projection.
 * ------------------------------------------------------------------ */
#ifdef USE_PROJ
  if (layer->projection.numargs > 0 && 
      EQUAL(layer->projection.args[0], "auto"))
  {
      ACQUIRE_OGR_LOCK;
      OGRSpatialReferenceH hSRS = OGR_L_GetSpatialRef( psInfo->hLayer );

      if (msOGRSpatialRef2ProjectionObj(hSRS,
                                        &(layer->projection),
                                        layer->debug ) != MS_SUCCESS)
      {
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
#endif

  return MS_SUCCESS;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", "msOGRLayerOpen()");
  return(MS_FAILURE);

#endif /* USE_OGR */
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
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;

  if (psInfo)
  {
      if( layer->debug )
          msDebug("msOGRLayerClose(%s).\n", layer->connection);

      msOGRFileClose( layer, psInfo );
      layer->layerinfo = NULL;
  }

  return MS_SUCCESS;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", "msOGRLayerClose()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerIsOpen()
 **********************************************************************/
static int msOGRLayerIsOpen(layerObj *layer) 
{
#ifdef USE_OGR
  if (layer->layerinfo)
      return MS_TRUE;

  return MS_FALSE;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", "msOGRLayerIsOpen()");
  return(MS_FALSE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerWhichShapes()
 *
 * Init OGR layer structs ready for calls to msOGRLayerNextShape().
 *
 * Returns MS_SUCCESS/MS_FAILURE, or MS_DONE if no shape matching the
 * layer's FILTER overlaps the selected region.
 **********************************************************************/
int msOGRLayerWhichShapes(layerObj *layer, rectObj rect) 
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  int   status;

  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
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

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerWhichShapes()");
  return(MS_FAILURE);

#endif /* USE_OGR */
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
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  
  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerGetItems()");
    return(MS_FAILURE);
  }

  if( layer->tileindex != NULL )
  {
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

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetItems()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerInitItemInfo()
 *
 * Init the itemindexes array after items[] has been reset in a layer.
 **********************************************************************/
static int msOGRLayerInitItemInfo(layerObj *layer)
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  int   i;
  OGRFeatureDefnH hDefn;

  if (layer->numitems == 0)
      return MS_SUCCESS;

  if( layer->tileindex != NULL )
  {
      if( psInfo->poCurTile == NULL 
          && msOGRFileReadTile( layer, psInfo, -2 ) != MS_SUCCESS )
          return MS_FAILURE;
      
      psInfo = psInfo->poCurTile;
  }

  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
      msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
                 "msOGRLayerInitItemInfo()");
      return(MS_FAILURE);
  }

  if((hDefn = OGR_L_GetLayerDefn( psInfo->hLayer )) == NULL) 
  {
    msSetError(MS_OGRERR, "Layer contains no fields.",  
               "msOGRLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  if (layer->iteminfo)
      free(layer->iteminfo);
  if((layer->iteminfo = (int *)malloc(sizeof(int)*layer->numitems))== NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  int *itemindexes = (int*)layer->iteminfo;
  for(i=0;i<layer->numitems;i++) 
  {
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
#if GDAL_VERSION_NUM >= 1400
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
#endif /* GDAL_VERSION_NUM >= 1400 */
#if GDAL_VERSION_NUM >= 1600
      else if (EQUAL(layer->items[i], MSOGR_LABELOCOLORNAME))
          itemindexes[i] = MSOGR_LABELOCOLORINDEX;
#endif /* GDAL_VERSION_NUM >= 1600 */
      else
          itemindexes[i] = OGR_FD_GetFieldIndex( hDefn, layer->items[i] );
      if(itemindexes[i] == -1)
      {
          msSetError(MS_OGRERR, 
                     (char*)CPLSPrintf("Invalid Field name: %s", 
                                       layer->items[i]), 
                     "msOGRLayerInitItemInfo()");
          return(MS_FAILURE);
      }
  }

  return(MS_SUCCESS);
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerInitItemInfo()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerFreeItemInfo()
 *
 * Free the itemindexes array in a layer.
 **********************************************************************/
void msOGRLayerFreeItemInfo(layerObj *layer)
{
#ifdef USE_OGR

  if (layer->iteminfo)
      free(layer->iteminfo);
  layer->iteminfo = NULL;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerFreeItemInfo()");

#endif /* USE_OGR */
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
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  int  status;

  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerNextShape()");
    return(MS_FAILURE);
  }

  if( layer->tileindex == NULL )
      return msOGRFileNextShape( layer, shape, psInfo );

  // Do we need to load the first tile? 
  if( psInfo->poCurTile == NULL )
  {
      status = msOGRFileReadTile( layer, psInfo );
      if( status != MS_SUCCESS )
          return status;
  }

  do 
  {
      // Try getting a shape from this tile.
      status = msOGRFileNextShape( layer, shape, psInfo->poCurTile );
      if( status != MS_DONE )
          return status;
  
      // try next tile.
      status = msOGRFileReadTile( layer, psInfo );
      if( status != MS_SUCCESS )
          return status;
  } while( status == MS_SUCCESS );

  return status;
  
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerNextShape()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerGetShape()
 *
 * Returns shape from OGR data source by fid.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerGetShape(layerObj *layer, shapeObj *shape, int tile, 
                       long fid)
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;

  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerNextShape()");
    return(MS_FAILURE);
  }

  if( layer->tileindex == NULL )
      return msOGRFileGetShape(layer, shape, fid, psInfo, TRUE );
  else
  {
      if( psInfo->poCurTile == NULL
          || psInfo->poCurTile->nTileId != tile )
      {
          if( msOGRFileReadTile( layer, psInfo, tile ) != MS_SUCCESS )
              return MS_FAILURE;
      }

      return msOGRFileGetShape(layer, shape, fid, psInfo->poCurTile, TRUE );
  }
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetShape()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerResultGetShape()
 *
 * Returns shape from OGR data source by index into the current results
 * set.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerResultGetShape(layerObj *layer, shapeObj *shape, int tile, 
                             long record)
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;

  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerNextShape()");
    return(MS_FAILURE);
  }

  if( layer->tileindex == NULL )
      return msOGRFileGetShape(layer, shape, record, psInfo, FALSE );
  else
  {
      if( psInfo->poCurTile == NULL
          || psInfo->poCurTile->nTileId != tile )
      {
          if( msOGRFileReadTile( layer, psInfo, tile ) != MS_SUCCESS )
              return MS_FAILURE;
      }

      return msOGRFileGetShape(layer, shape, record, psInfo->poCurTile, FALSE );
  }
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetShape()");
  return(MS_FAILURE);

#endif /* USE_OGR */
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
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;
  OGREnvelope oExtent;

  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
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
  if (OGR_L_GetExtent( psInfo->hLayer, &oExtent, TRUE) != OGRERR_NONE)
  {
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
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetExtent()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}


/**********************************************************************
 *                     msOGRGetSymbolId()
 *
 * Returns a MapServer symbol number matching one of the symbols from
 * the OGR symbol id string.  If not found then try to locate the
 * default symbol name, and if not found return 0.
 **********************************************************************/
#ifdef USE_OGR
static int msOGRGetSymbolId(symbolSetObj *symbolset, const char *pszSymbolId, 
                            const char *pszDefaultSymbol)
{
    // Symbol name mapping:
    // First look for the native symbol name, then the ogr-...
    // generic name, and in last resort try pszDefaultSymbol if
    // provided by user.
    char  **params;
    int   numparams;
    int   nSymbol = -1;

    if (pszSymbolId && pszSymbolId[0] != '\0')
    {
#if GDAL_VERSION_NUM >= 1800 /* Use comma as the separator */
        params = msStringSplit(pszSymbolId, ',', &numparams);
#else
        params = msStringSplit(pszSymbolId, '.', &numparams);
#endif
        if (params != NULL)
        {
            for(int j=0; j<numparams && nSymbol == -1; j++)
            {
                nSymbol = msGetSymbolIndex(symbolset, params[j], MS_FALSE);
            }
            msFreeCharArray(params, numparams);
        }
    }
    if (nSymbol == -1 && pszDefaultSymbol)
    {
        nSymbol = msGetSymbolIndex(symbolset,(char*)pszDefaultSymbol,MS_FALSE);
    }
    if (nSymbol == -1)
        nSymbol = 0;

    return nSymbol;
}
#endif

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
                                  int tile, long record)
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;

  if (psInfo == NULL || psInfo->hLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerGetAutoStyle()");
    return(MS_FAILURE);
  }

  if( layer->tileindex != NULL )
  {
      if( (psInfo->poCurTile == NULL || tile != psInfo->poCurTile->nTileId)
          && msOGRFileReadTile( layer, psInfo ) != MS_SUCCESS )
          return MS_FAILURE;
      
      psInfo = psInfo->poCurTile;
  }

/* ------------------------------------------------------------------
 * Read shape or reuse ref. to last shape read.
 * ------------------------------------------------------------------ */
  ACQUIRE_OGR_LOCK;
  if (psInfo->hLastFeature == NULL || 
      psInfo->last_record_index_read != record)
  {
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
  if (msMaybeAllocateStyle(c, 0)) {
      RELEASE_OGR_LOCK;
      return(MS_FAILURE);
  }

  // __TODO__ label cache incompatible with styleitem feature.
  layer->labelcache = MS_OFF;

/* ------------------------------------------------------------------
 * Handle each part
 * ------------------------------------------------------------------ */
  if (psInfo->hLastFeature)
  {
      GBool bIsNull, bIsBrush=MS_FALSE, bIsPen=MS_FALSE;
      int r=0,g=0,b=0,t=0;

#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
      OGRStyleMgrH hStyleMgr = OGR_SM_Create(NULL);
      OGR_SM_InitFromFeature(hStyleMgr, psInfo->hLastFeature);
      int numParts = OGR_SM_GetPartCount(hStyleMgr, NULL);
#else /* OGRStyle C++ */
      OGRStyleMgr *poStyleMgr = new OGRStyleMgr(NULL);
      poStyleMgr->InitFromFeature((OGRFeature *)psInfo->hLastFeature);
      int numParts = poStyleMgr->GetPartCount();
#endif /* OGRStyle C API */

      for(int i=0; i<numParts; i++)
      {
          OGRSTClassId eStylePartType;
#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
          OGRStyleToolH hStylePart = OGR_SM_GetPart(hStyleMgr, i, NULL);
          if (!hStylePart)
              continue;
          eStylePartType = OGR_ST_GetType(hStylePart);
#else /* OGRStyle C++ */
          OGRStyleTool *poStylePart = poStyleMgr->GetPart(i);
          if (!poStylePart)
              continue;
          eStylePartType = poStylePart->GetType();
#endif /* OGRStyle C API */

          // We want all size values returned in pixels.
          //
          // The scale factor that OGR expect is the ground/paper scale
          // e.g. if 1 ground unit = 0.01 paper unit then scale=1/0.01=100
          // cellsize if number of ground units/pixel, and OGR assumes that
          // there is 72*39.37 pixels/ground units (since meter is assumed 
          // for ground... but what ground units we have does not matter
          // as long as use the same assumptions everywhere)
          // That gives scale = cellsize*72*39.37

#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
          OGR_ST_SetUnit(hStylePart, OGRSTUPixel, map->cellsize*72.0*39.37);
#else
          poStylePart->SetUnit(OGRSTUPixel, map->cellsize*72.0*39.37);
#endif

          if (eStylePartType == OGRSTCLabel)
#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
          {
              OGRStyleToolH hLabelStyle = hStylePart;

              // Enclose the text string inside quotes to make sure it is seen
              // as a string by the parser inside loadExpression(). (bug185)
              msLoadExpressionString(&(c->text), 
                         (char*)CPLSPrintf("\"%s\"", 
                                           OGR_ST_GetParamStr(hLabelStyle, 
                                                              OGRSTLabelTextString, 
                                                              &bIsNull)));

              c->label.angle = OGR_ST_GetParamDbl(hLabelStyle, 
                                                  OGRSTLabelAngle, &bIsNull);

              c->label.size = OGR_ST_GetParamDbl(hLabelStyle, 
                                                 OGRSTLabelSize, &bIsNull);
              if( c->label.size < 1 ) /* no point dropping to zero size */
                  c->label.size = 1;

              // OGR default is anchor point = LL, so label is at UR of anchor
              c->label.position = MS_UR;

              const char *pszColor = OGR_ST_GetParamStr(hLabelStyle, 
                                                        OGRSTLabelFColor,
                                                        &bIsNull);
              if (!bIsNull && OGR_ST_GetRGBFromString(hLabelStyle, pszColor,
                                                      &r, &g, &b, &t))
              {
                  MS_INIT_COLOR(c->label.color, r, g, b);
              }

              pszColor = OGR_ST_GetParamStr(hLabelStyle, 
                                            OGRSTLabelBColor,
                                            &bIsNull);
              if (!bIsNull && OGR_ST_GetRGBFromString(hLabelStyle, pszColor,
                                                      &r, &g, &b, &t))
              {
                  MS_INIT_COLOR(c->label.backgroundcolor, r, g, b);
              }

              pszColor = OGR_ST_GetParamStr(hLabelStyle, 
                                            OGRSTLabelHColor,
                                            &bIsNull);
              if (!bIsNull && OGR_ST_GetRGBFromString(hLabelStyle, pszColor,
                                                      &r, &g, &b, &t))
              {
                  MS_INIT_COLOR(c->label.shadowcolor, r, g, b);
              }

#if GDAL_VERSION_NUM >= 1600
              pszColor = OGR_ST_GetParamStr(hLabelStyle, 
                                            OGRSTLabelOColor,
                                            &bIsNull);
              if (!bIsNull && OGR_ST_GetRGBFromString(hLabelStyle, pszColor,
                                                      &r, &g, &b, &t))
              {
                  MS_INIT_COLOR(c->label.outlinecolor, r, g, b);
              }
#endif /* GDAL_VERSION_NUM >= 1600 */

              // Label font... do our best to use TrueType fonts, otherwise
              // fallback on bitmap fonts.
#if defined(USE_GD_TTF) || defined (USE_GD_FT)
              const char *pszBold = OGR_ST_GetParamNum(hLabelStyle, 
                                                           OGRSTLabelBold, 
                                                           &bIsNull) ? "-bold" : "";
              const char *pszItalic = OGR_ST_GetParamNum(hLabelStyle, 
                                                           OGRSTLabelItalic, 
                                                           &bIsNull) ? "-italic" : "";
              const char *pszFontName = OGR_ST_GetParamStr(hLabelStyle, 
                                                           OGRSTLabelFontName,
                                                           &bIsNull);
              const char *pszName = CPLSPrintf("%s%s%s", pszFontName, pszBold, pszItalic);
              bool bFont = true;

              if (pszFontName != NULL && !bIsNull && pszFontName[0] != '\0')
              {
                  if (msLookupHashTable(&(map->fontset.fonts), (char*)pszName) != NULL)
                  {
                      c->label.type = MS_TRUETYPE;
                      c->label.font = strdup(pszName);
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** Using '%s' TTF font **\n", pszName);
                  }
                  else if ( (strcmp(pszFontName,pszName) != 0) &&
                            msLookupHashTable(&(map->fontset.fonts), (char*)pszFontName) != NULL)
                  {
                      c->label.type = MS_TRUETYPE;
                      c->label.font = strdup(pszFontName);
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** Using '%s' TTF font **\n", pszFontName);
                  }
                  else if (msLookupHashTable(&(map->fontset.fonts),"default") != NULL)
                  {
                      c->label.type = MS_TRUETYPE;
                      c->label.font = strdup("default");
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** Using 'default' TTF font **\n");
                  }
                  else
                      bFont = false;
              }

              if (!bFont)
#endif /* USE_GD_FT || USE_GD_FT */
              {
                  c->label.type = MS_BITMAP;
                  c->label.size = MS_MEDIUM;
                  if (layer->debug >= MS_DEBUGLEVEL_VVV)
                      msDebug("** Using 'medium' BITMAP font **\n");
              }
          }
#else /* OGR Style C++ */
          {
              OGRStyleLabel *poLabelStyle = (OGRStyleLabel*)poStylePart;

              // Enclose the text string inside quotes to make sure it is seen
              // as a string by the parser inside loadExpression(). (bug185)
              msLoadExpressionString(&(c->text), 
                         (char*)CPLSPrintf("\"%s\"", 
                                           poLabelStyle->TextString(bIsNull)));

              c->label.angle = poLabelStyle->Angle(bIsNull);

              c->label.size = (int)poLabelStyle->Size(bIsNull);
              if( c->label.size < 1 ) /* no point dropping to zero size */
                  c->label.size = 1;

              // OGR default is anchor point = LL, so label is at UR of anchor
              c->label.position = MS_UR;

              const char *pszColor = poLabelStyle->ForeColor(bIsNull);
              if (!bIsNull && poLabelStyle->GetRGBFromString(pszColor,r,g,b,t))
              {
                  MS_INIT_COLOR(c->label.color, r, g, b);
              }

              pszColor = poLabelStyle->BackColor(bIsNull);
              if (!bIsNull && poLabelStyle->GetRGBFromString(pszColor,r,g,b,t))
              {
                  MS_INIT_COLOR(c->label.backgroundcolor, r, g, b);
              }
#if GDAL_VERSION_NUM >= 1400
              pszColor = poLabelStyle->ShadowColor(bIsNull);
              if (!bIsNull && poLabelStyle->GetRGBFromString(pszColor,r,g,b,t))
              {
                  MS_INIT_COLOR(c->label.shadowcolor, r, g, b);
              }
#endif
              // Label font... do our best to use TrueType fonts, otherwise
              // fallback on bitmap fonts.
#if defined(USE_GD_TTF) || defined (USE_GD_FT)
              const char *pszBold = poLabelStyle->Bold(bIsNull)  ? "-bold" : "";
              const char *pszItalic = poLabelStyle->Italic(bIsNull) ? "-italic" : "";
              const char *pszFontName = poLabelStyle->FontName(bIsNull);
              const char *pszName = CPLSPrintf("%s%s%s", pszFontName, pszBold, pszItalic);
              bool bFont = true;

              if (pszFontName != NULL && !bIsNull && pszFontName[0] != '\0')
              {
                  if (msLookupHashTable(&(map->fontset.fonts), (char*)pszName) != NULL)
                  {
                      c->label.type = MS_TRUETYPE;
                      c->label.font = strdup(pszName);
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** Using '%s' TTF font **\n", pszName);
                  }
                  else if ( (strcmp(pszFontName,pszName) != 0) &&
                            msLookupHashTable(&(map->fontset.fonts), (char*)pszFontName) != NULL)
                  {
                      c->label.type = MS_TRUETYPE;
                      c->label.font = strdup(pszFontName);
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** Using '%s' TTF font **\n", pszFontName);
                  }
                  else if (msLookupHashTable(&(map->fontset.fonts),"default") != NULL)
                  {
                      c->label.type = MS_TRUETYPE;
                      c->label.font = strdup("default");
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** Using 'default' TTF font **\n");
                  }
                  else
                      bFont = false;
              }

              if (!bFont)
#endif /* USE_GD_FT || USE_GD_FT */
              {
                  c->label.type = MS_BITMAP;
                  c->label.size = MS_MEDIUM;
                  if (layer->debug >= MS_DEBUGLEVEL_VVV)
                      msDebug("** Using 'medium' BITMAP font **\n");
              }
          }
#endif /* OGRStyle C API */
          else if (eStylePartType == OGRSTCPen)
#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
          {
              OGRStyleToolH hPenStyle = hStylePart;
              bIsPen = TRUE;

              const char *pszPenName = OGR_ST_GetParamStr(hPenStyle, 
                                                          OGRSTPenId, 
                                                          &bIsNull);
              if (bIsNull) pszPenName = NULL;
              colorObj oPenColor;
              int nPenSymbol = 0;
              int nPenSize = 1;

              // Make sure pen is always initialized
              MS_INIT_COLOR(oPenColor, -1, -1, -1);

              // Check for Pen Pattern "ogr-pen-1": the invisible pen
              // If that's what we have then set pen color to -1
              if (pszPenName && strstr(pszPenName, "ogr-pen-1") != NULL)
              {
                  MS_INIT_COLOR(oPenColor, -1, -1, -1);
              }
              else
              {
                  const char *pszColor = OGR_ST_GetParamStr(hPenStyle, 
                                                            OGRSTPenColor,
                                                            &bIsNull);
                  if (!bIsNull && OGR_ST_GetRGBFromString(hPenStyle, pszColor,
                                                          &r, &g, &b, &t))
                  {
                      MS_INIT_COLOR(oPenColor, r, g, b);
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** PEN COLOR = %d %d %d **\n", r,g,b);
                  }

                  nPenSize = OGR_ST_GetParamNum(hPenStyle, 
                                                OGRSTPenWidth, &bIsNull);
                  if (bIsNull)
                      nPenSize = 1;
                  if (pszPenName!=NULL)
                  {
                      // Try to match pen name in symbol file
                      nPenSymbol = msOGRGetSymbolId(&(map->symbolset),
                                                    pszPenName, NULL);
                  }
              }
              if (layer->debug >= MS_DEBUGLEVEL_VVV)
                  msDebug("** PEN COLOR = %d %d %d **\n", oPenColor.red,oPenColor.green,oPenColor.blue);
              if (bIsBrush || layer->type == MS_LAYER_POLYGON)
              {
                  // This is a multipart symbology, so pen defn goes in the
                  // overlaysymbol params 
                  if (msMaybeAllocateStyle(c, 1))
                  {
                      RELEASE_OGR_LOCK;
                      return(MS_FAILURE);
                  }

                  c->styles[1]->outlinecolor = oPenColor;
                  c->styles[1]->size = nPenSize;
                  c->styles[1]->symbol = nPenSymbol;
                  c->styles[1]->width = nPenSize;
              }
              else
              {
                  // Single part symbology
                  if (msMaybeAllocateStyle(c, 0))
                  {
                      RELEASE_OGR_LOCK;
                      return(MS_FAILURE);
                  }

                  if(layer->type == MS_LAYER_POLYGON)
                      c->styles[0]->outlinecolor = c->styles[0]->color = 
                          oPenColor;
                  else
                      c->styles[0]->color = oPenColor;
                  c->styles[0]->symbol = nPenSymbol;
                  c->styles[0]->size = nPenSize;
                  c->styles[0]->width = nPenSize;
              }

          }
#else /* OGR Style C++ */
          {
              OGRStylePen *poPenStyle = (OGRStylePen*)poStylePart;
              bIsPen = TRUE;

              const char *pszPenName = poPenStyle->Id(bIsNull);
              if (bIsNull) pszPenName = NULL;
              colorObj oPenColor;
              int nPenSymbol = 0;
              int nPenSize = 1;

              // Make sure pen is always initialized
              MS_INIT_COLOR(oPenColor, -1, -1, -1);

              // Check for Pen Pattern "ogr-pen-1": the invisible pen
              // If that's what we have then set pen color to -1
              if (pszPenName && strstr(pszPenName, "ogr-pen-1") != NULL)
              {
                  MS_INIT_COLOR(oPenColor, -1, -1, -1);
              }
              else
              {
                  const char *pszColor = poPenStyle->Color(bIsNull);
                  if (!bIsNull && poPenStyle->GetRGBFromString(pszColor,r,g,b,t))
                  {
                      MS_INIT_COLOR(oPenColor, r, g, b);
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** PEN COLOR = %d %d %d **\n", r,g,b);
                  }

                  nPenSize = (int)poPenStyle->Width(bIsNull);
                  if (bIsNull)
                      nPenSize = 1;
                  if (pszPenName!=NULL)
                  {
                      // Try to match pen name in symbol file
                      nPenSymbol = msOGRGetSymbolId(&(map->symbolset),
                                                    pszPenName, NULL);
                  }
              }
              if (layer->debug >= MS_DEBUGLEVEL_VVV)
                  msDebug("** PEN COLOR = %d %d %d **\n", oPenColor.red,oPenColor.green,oPenColor.blue);
              if (bIsBrush || layer->type == MS_LAYER_POLYGON)
              {
                  // This is a multipart symbology, so pen defn goes in the
                  // overlaysymbol params 
                  if (msMaybeAllocateStyle(c, 1))
                  {
                      RELEASE_OGR_LOCK;
                      return(MS_FAILURE);
                  }

                  c->styles[1]->outlinecolor = oPenColor;
                  c->styles[1]->size = nPenSize;
                  c->styles[1]->symbol = nPenSymbol;
                  c->styles[1]->width = nPenSize;
              }
              else
              {
                  // Single part symbology
                  if (msMaybeAllocateStyle(c, 0))
                  {
                      RELEASE_OGR_LOCK;
                      return(MS_FAILURE);
                  }

                  if(layer->type == MS_LAYER_POLYGON)
                      c->styles[0]->outlinecolor = c->styles[0]->color = 
                          oPenColor;
                  else
                      c->styles[0]->color = oPenColor;
                  c->styles[0]->symbol = nPenSymbol;
                  c->styles[0]->size = nPenSize;
                  c->styles[0]->width = nPenSize;
              }

          }
#endif /* OGRStyle C API */
          else if (eStylePartType == OGRSTCBrush)
#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
          {
              OGRStyleToolH hBrushStyle = hStylePart;

              const char *pszBrushName = OGR_ST_GetParamStr(hBrushStyle, 
                                                            OGRSTBrushId, 
                                                            &bIsNull);
              if (bIsNull) pszBrushName = NULL;

              /* We need 1 style */
              if (msMaybeAllocateStyle(c, 0))
              {
                  RELEASE_OGR_LOCK;
                  return(MS_FAILURE);
              }

              // Check for Brush Pattern "ogr-brush-1": the invisible fill
              // If that's what we have then set fill color to -1
              if (pszBrushName && strstr(pszBrushName, "ogr-brush-1") != NULL)
              {
                  MS_INIT_COLOR(c->styles[0]->color, -1, -1, -1);
              }
              else
              {
                  bIsBrush = TRUE;
                  const char *pszColor = OGR_ST_GetParamStr(hBrushStyle, 
                                                            OGRSTBrushFColor,
                                                            &bIsNull);
                  if (!bIsNull && OGR_ST_GetRGBFromString(hBrushStyle, 
                                                          pszColor,
                                                          &r, &g, &b, &t))
                  {
                      MS_INIT_COLOR(c->styles[0]->color, r, g, b);
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** BRUSH COLOR = %d %d %d **\n", r,g,b);
                  }

                  pszColor = OGR_ST_GetParamStr(hBrushStyle, 
                                                OGRSTBrushBColor, &bIsNull);
                  if (!bIsNull && OGR_ST_GetRGBFromString(hBrushStyle, 
                                                          pszColor,
                                                          &r, &g, &b, &t))
                  {
                      MS_INIT_COLOR(c->styles[0]->backgroundcolor, r, g, b);
                  }

                  // Symbol name mapping:
                  // First look for the native symbol name, then the ogr-...
                  // generic name.  
                  // If none provided or found then use 0: solid fill
              
                  const char *pszName = OGR_ST_GetParamStr(hBrushStyle, 
                                                            OGRSTBrushId, 
                                                            &bIsNull);
                  c->styles[0]->symbol = msOGRGetSymbolId(&(map->symbolset), 
                                                         pszName, NULL);
              }
          }
#else /* OGR Style C++ */
          {
              OGRStyleBrush *poBrushStyle = (OGRStyleBrush*)poStylePart;

              const char *pszBrushName = poBrushStyle->Id(bIsNull);
              if (bIsNull) pszBrushName = NULL;

              /* We need 1 style */
              if (msMaybeAllocateStyle(c, 0))
              {
                  RELEASE_OGR_LOCK;
                  return(MS_FAILURE);
              }

              // Check for Brush Pattern "ogr-brush-1": the invisible fill
              // If that's what we have then set fill color to -1
              if (pszBrushName && strstr(pszBrushName, "ogr-brush-1") != NULL)
              {
                  MS_INIT_COLOR(c->styles[0]->color, -1, -1, -1);
              }
              else
              {
                  bIsBrush = TRUE;
                  const char *pszColor = poBrushStyle->ForeColor(bIsNull);
                  if (!bIsNull && poBrushStyle->GetRGBFromString(pszColor,r,g,b,t))
                  {
                      MS_INIT_COLOR(c->styles[0]->color, r, g, b);
                      if (layer->debug >= MS_DEBUGLEVEL_VVV)
                          msDebug("** BRUSH COLOR = %d %d %d **\n", r,g,b);
                  }

                  pszColor = poBrushStyle->BackColor(bIsNull);
                  if (!bIsNull && poBrushStyle->GetRGBFromString(pszColor,r,g,b,t))
                  {
                      MS_INIT_COLOR(c->styles[0]->backgroundcolor, r, g, b);
                  }

                  // Symbol name mapping:
                  // First look for the native symbol name, then the ogr-...
                  // generic name.  
                  // If none provided or found then use 0: solid fill
              
                  const char *pszName = poBrushStyle->Id(bIsNull);
                  if (bIsNull)
                      pszName = NULL;
                  c->styles[0]->symbol = msOGRGetSymbolId(&(map->symbolset), 
                                                         pszName, NULL);
              }
          }
#endif /* OGRStyle C API */
          else if (eStylePartType == OGRSTCSymbol)
#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
          {
              OGRStyleToolH hSymbolStyle = hStylePart;

              /* We need 1 style */
              if (msMaybeAllocateStyle(c, 0))
              {
                  RELEASE_OGR_LOCK;
                  return(MS_FAILURE);
              }

              const char *pszColor = OGR_ST_GetParamStr(hSymbolStyle, 
                                                        OGRSTSymbolColor,
                                                        &bIsNull);
              if (!bIsNull && OGR_ST_GetRGBFromString(hSymbolStyle, 
                                                      pszColor,
                                                      &r, &g, &b, &t))
              {
                  MS_INIT_COLOR(c->styles[0]->color, r, g, b);
              }

#if GDAL_VERSION_NUM >= 1600              
              pszColor = OGR_ST_GetParamStr(hSymbolStyle,
                                            OGRSTSymbolOColor,
                                            &bIsNull);
              if (!bIsNull && OGR_ST_GetRGBFromString(hSymbolStyle,
                                                      pszColor,
                                                      &r, &g, &b, &t))
              {
                  MS_INIT_COLOR(c->styles[0]->outlinecolor, r, g, b);
              }
#endif /* GDAL_VERSION_NUM >= 1600 */
              c->styles[0]->angle = OGR_ST_GetParamNum(hSymbolStyle,
                                                       OGRSTSymbolAngle,
                                                       &bIsNull);
              
              c->styles[0]->size = OGR_ST_GetParamNum(hSymbolStyle, 
                                                      OGRSTSymbolSize, 
                                                      &bIsNull);

              // Symbol name mapping:
              // First look for the native symbol name, then the ogr-...
              // generic name, and in last resort try "default-marker" if
              // provided by user.
              const char *pszName = OGR_ST_GetParamStr(hSymbolStyle, 
                                                       OGRSTSymbolId, 
                                                       &bIsNull);
              if (bIsNull)
                  pszName = NULL;

              c->styles[0]->symbol = msOGRGetSymbolId(&(map->symbolset),
                                                    pszName, 
                                                    "default-marker");
          }
#else /* OGR Style C++ */
          {
              OGRStyleSymbol *poSymbolStyle = (OGRStyleSymbol*)poStylePart;

              /* We need 1 style */
              if (msMaybeAllocateStyle(c, 0))
              {
                  RELEASE_OGR_LOCK;
                  return(MS_FAILURE);
              }

              const char *pszColor = poSymbolStyle->Color(bIsNull);
              if (!bIsNull && poSymbolStyle->GetRGBFromString(pszColor,r,g,b,t))
              {
                  MS_INIT_COLOR(c->styles[0]->color, r, g, b);
              }

              c->styles[0]->angle = poSymbolStyle->Angle(bIsNull);
                  
              c->styles[0]->size = (int)poSymbolStyle->Size(bIsNull);

              // Symbol name mapping:
              // First look for the native symbol name, then the ogr-...
              // generic name, and in last resort try "default-marker" if
              // provided by user.
              const char *pszName = poSymbolStyle->Id(bIsNull);
              if (bIsNull)
                  pszName = NULL;

              c->styles[0]->symbol = msOGRGetSymbolId(&(map->symbolset),
                                                    pszName, 
                                                    "default-marker");
          }
#endif /* OGRStyle C API */

#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
          OGR_ST_Destroy(hStylePart);
#else
          delete poStylePart;
#endif

      }

#if GDAL_VERSION_NUM >= 1500 /* Use OGR Style C API */
      OGR_SM_Destroy(hStyleMgr);
#else
      delete poStyleMgr;
#endif

  }

  RELEASE_OGR_LOCK;
  return MS_SUCCESS;
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetAutoStyle()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/************************************************************************/
/*                           msOGRLCleanup()                            */
/************************************************************************/

void msOGRCleanup( void )

{
#if defined(USE_OGR)
    ACQUIRE_OGR_LOCK;
    if( bOGRDriversRegistered == MS_TRUE )
    {
#if GDAL_VERSION_NUM >= 1400
        OGRCleanupAll();
        bOGRDriversRegistered = MS_FALSE;
#endif
    }
    RELEASE_OGR_LOCK;
#endif
}

/************************************************************************/
/*                           msOGREscapeSQLParam                        */
/************************************************************************/
char *msOGREscapeSQLParam(layerObj *layer, const char *pszString)
{
    char* pszEscapedStr =NULL;
#ifdef USE_OGR
    if(layer && pszString && strlen(pszString) > 0)
    {
        char* pszEscapedOGRStr =  CPLEscapeString(pszString, strlen(pszString),  
		                                    CPLES_SQL ); 
	pszEscapedStr = strdup(pszEscapedOGRStr);
        CPLFree(pszEscapedOGRStr);
	return pszEscapedStr; 
    }
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGREscapeSQLParam()");
  return NULL;

#endif /* USE_OGR */  
}


/************************************************************************/
/*                           msOGREscapeSQLParam                        */
/************************************************************************/
char *msOGREscapePropertyName(layerObj *layer, const char *pszString)
{
    char* pszEscapedStr =NULL;
    int i =0;
#ifdef USE_OGR
    if(layer && pszString && strlen(pszString) > 0)
    {
        unsigned char ch;
        for(i=0; (ch = ((unsigned char*)pszString)[i]) != '\0'; i++)
        {
            if ( !(isalnum(ch) || ch == '_' || ch > 127) )
            {
                return strdup("invalid_property_name");
            }
        }
        pszEscapedStr = strdup(pszString);
    }
    return pszEscapedStr;
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGREscapePropertyName()");
  return NULL;

#endif /* USE_OGR */  
}
/************************************************************************/
/*                  msOGRLayerInitializeVirtualTable()                  */
/************************************************************************/
int
msOGRLayerInitializeVirtualTable(layerObj *layer)
{
    assert(layer != NULL);
    assert(layer->vtable != NULL);

    layer->vtable->LayerInitItemInfo = msOGRLayerInitItemInfo;
    layer->vtable->LayerFreeItemInfo = msOGRLayerFreeItemInfo;
    layer->vtable->LayerOpen = msOGRLayerOpenVT;
    layer->vtable->LayerIsOpen = msOGRLayerIsOpen;
    layer->vtable->LayerWhichShapes = msOGRLayerWhichShapes;
    layer->vtable->LayerNextShape = msOGRLayerNextShape;
    layer->vtable->LayerResultsGetShape = msOGRLayerResultGetShape; 
    layer->vtable->LayerGetShape = msOGRLayerGetShape;
    layer->vtable->LayerClose = msOGRLayerClose;
    layer->vtable->LayerGetItems = msOGRLayerGetItems;
    layer->vtable->LayerGetExtent = msOGRLayerGetExtent;
    layer->vtable->LayerGetAutoStyle = msOGRLayerGetAutoStyle;
    /* layer->vtable->LayerCloseConnection, use default */
    layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;
    layer->vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
    /* layer->vtable->LayerCreateItems, use default */
    /* layer->vtable->LayerGetNumFeatures, use default */

    layer->vtable->LayerEscapeSQLParam = msOGREscapeSQLParam;
    layer->vtable->LayerEscapePropertyName = msOGREscapePropertyName;

    return MS_SUCCESS;
}

/************************************************************************/
/*                         msOGRShapeFromWKT()                          */
/************************************************************************/
shapeObj *msOGRShapeFromWKT(const char *string)
{
#ifdef USE_OGR
    OGRGeometryH hGeom = NULL;
    shapeObj *shape=NULL;
    
    if(!string) 
        return NULL;
    
    ACQUIRE_OLD_OGR_LOCK;
    if( OGR_G_CreateFromWkt( (char **)&string, NULL, &hGeom ) != OGRERR_NONE )
    {
        msSetError(MS_OGRERR, "Failed to parse WKT string.", 
                   "msOGRShapeFromWKT()" );
        RELEASE_OLD_OGR_LOCK;
        return NULL;
    }

    /* Initialize a corresponding shapeObj */

    shape = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(shape);

    /* translate WKT into an OGRGeometry. */
  
    if( msOGRGeometryToShape( hGeom, shape, 
                              wkbFlatten(OGR_G_GetGeometryType(hGeom)) )
                              == MS_FAILURE )
    {
        RELEASE_OLD_OGR_LOCK;
        free( shape );
        return NULL;
    }

    OGR_G_DestroyGeometry( hGeom );

    RELEASE_OLD_OGR_LOCK;
    return shape;
#else
  msSetError(MS_OGRERR, "OGR support is not available.","msOGRShapeFromWKT()");
  return NULL;
#endif
}

/************************************************************************/
/*                          msOGRShapeToWKT()                           */
/************************************************************************/
char *msOGRShapeToWKT(shapeObj *shape)
{
#ifdef USE_OGR
    OGRGeometryH hGeom = NULL;
    int          i;
    char        *wkt = NULL;

    if(!shape) 
        return NULL;

    ACQUIRE_OLD_OGR_LOCK;
    if( shape->type == MS_SHAPE_POINT && shape->numlines == 1
        && shape->line[0].numpoints == 1 )
    {
        hGeom = OGR_G_CreateGeometry( wkbPoint );
#if GDAL_VERSION_NUM >= 1310
        OGR_G_SetPoint_2D( hGeom, 0, 
                           shape->line[0].point[0].x, 
                           shape->line[0].point[0].y );
#else
        OGR_G_SetPoint( hGeom, 0, 
                        shape->line[0].point[0].x, 
                        shape->line[0].point[0].y,
                        0.0 );
#endif
    }
    else if( shape->type == MS_SHAPE_POINT && shape->numlines == 1 
             && shape->line[0].numpoints > 1 )
    {
        hGeom = OGR_G_CreateGeometry( wkbMultiPoint );
        for( i = 0; i < shape->line[0].numpoints; i++ )
        {
            OGRGeometryH hPoint;
            
            hPoint = OGR_G_CreateGeometry( wkbPoint );
#if GDAL_VERSION_NUM >= 1310
            OGR_G_SetPoint_2D( hPoint, 0, 
                               shape->line[0].point[i].x, 
                               shape->line[0].point[i].y );
#else
            OGR_G_SetPoint( hPoint, 0, 
                            shape->line[0].point[i].x, 
                            shape->line[0].point[i].y,
                            0.0 );
#endif
            OGR_G_AddGeometryDirectly( hGeom, hPoint );
        }
    }
    else if( shape->type == MS_SHAPE_LINE && shape->numlines == 1 )
    {
        hGeom = OGR_G_CreateGeometry( wkbLineString );
        for( i = 0; i < shape->line[0].numpoints; i++ )
        {
#if GDAL_VERSION_NUM >= 1310
            OGR_G_AddPoint_2D( hGeom, 
                               shape->line[0].point[i].x, 
                               shape->line[0].point[i].y );
#else
            OGR_G_AddPoint( hGeom, 
                            shape->line[0].point[i].x, 
                            shape->line[0].point[i].y,
                            0.0 );
#endif
        }
    }
    else if( shape->type == MS_SHAPE_LINE && shape->numlines > 1 )
    {
        OGRGeometryH hMultiLine = OGR_G_CreateGeometry( wkbMultiLineString );
        int iLine;

        for( iLine = 0; iLine < shape->numlines; iLine++ )
        {
            hGeom = OGR_G_CreateGeometry( wkbLineString );
            for( i = 0; i < shape->line[iLine].numpoints; i++ )
            {
#if GDAL_VERSION_NUM >= 1310
                OGR_G_AddPoint_2D( hGeom, 
                                   shape->line[iLine].point[i].x, 
                                   shape->line[iLine].point[i].y );
#else
                OGR_G_AddPoint( hGeom, 
                            shape->line[iLine].point[i].x, 
                            shape->line[iLine].point[i].y,
                            0.0 );
#endif
            }

            OGR_G_AddGeometryDirectly( hMultiLine, hGeom );
        }

        hGeom = hMultiLine;
    }
    else if( shape->type == MS_SHAPE_POLYGON )
    {
        int iLine;

        /* actually, it is pretty hard to be sure rings 1+ are interior */
        hGeom = OGR_G_CreateGeometry( wkbPolygon );
        for( iLine = 0; iLine < shape->numlines; iLine++ )
        {
            OGRGeometryH hRing;
            hRing = OGR_G_CreateGeometry( wkbLinearRing );
            
            for( i = 0; i < shape->line[iLine].numpoints; i++ )
            {
#if GDAL_VERSION_NUM >= 1310
                OGR_G_AddPoint_2D( hRing, 
                                   shape->line[iLine].point[i].x, 
                                   shape->line[iLine].point[i].y );
#else
                OGR_G_AddPoint( hRing, 
                                shape->line[iLine].point[i].x, 
                                shape->line[iLine].point[i].y, 
                                0.0 );
#endif
            }
            OGR_G_AddGeometryDirectly( hGeom, hRing );
        }
    }
    else
    {
        msSetError(MS_OGRERR, "OGR support is not available.", "msOGRShapeToWKT()");
    }

    if( hGeom != NULL )
    {
        char *pszOGRWkt;

        OGR_G_ExportToWkt( hGeom, &pszOGRWkt );
        wkt = strdup( pszOGRWkt );
        CPLFree( pszOGRWkt );
    }

    RELEASE_OLD_OGR_LOCK;
    return wkt;
#else
    msSetError(MS_OGRERR, "OGR support is not available.", "msOGRShapeToWKT()");
    return NULL;
#endif
}


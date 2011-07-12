/**********************************************************************
 * $Id$
 *
 * Name:     mapogr.cpp
 * Project:  MapServer
 * Language: C++
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
#  include "ogr_spatialref.h"
#  include "gdal_version.h"
#  include "cpl_conv.h"
#  include "cpl_string.h"
#  include "gdal_version.h"
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

#include "ogrsf_frmts.h"
#include "ogr_featurestyle.h"

typedef struct ms_ogr_file_info_t
{
  char        *pszFname;
  int         nLayerIndex;
  OGRDataSource *poDS;
  OGRLayer    *poLayer;
  OGRFeature  *poLastFeature;

  int         nTileId;                  /* applies on the tiles themselves. */

  struct ms_ogr_file_info_t *poCurTile; /* exists on tile index, -> tiles */
  rectObj     rect;                     /* set by WhichShapes */

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
static int ogrGeomPoints(OGRGeometry *poGeom, shapeObj *outshp) 
{
  int   i;
  int   numpoints;

  if (poGeom == NULL)   
      return 0;

/* -------------------------------------------------------------------- */
/*      Container types result in recursive invocation on each          */
/*      subobject to add a set of points to the current list.           */
/* -------------------------------------------------------------------- */
  switch( wkbFlatten(poGeom->getGeometryType()) )
  {
      case wkbGeometryCollection:
      case wkbMultiLineString:
      case wkbMultiPolygon:
      {
          OGRGeometryCollection *poCollection = (OGRGeometryCollection *)
              poGeom;
          
          for (int iGeom=0; iGeom < poCollection->getNumGeometries(); iGeom++ )
          {
              if( ogrGeomPoints( poCollection->getGeometryRef( iGeom ), 
                                 outshp ) == -1 )
                  return -1;
          }

          return 0;							
      }
      break;

      case wkbPolygon:
      {
          OGRPolygon *poPoly = (OGRPolygon *)poGeom;

          if( ogrGeomPoints( poPoly->getExteriorRing(), outshp ) == -1 )
              return -1;
          
          for (int iRing=0; iRing < poPoly->getNumInteriorRings(); iRing++)
          {
              if( ogrGeomPoints( poPoly->getInteriorRing(iRing), outshp )
                  == -1 )
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
                     poGeom->getGeometryName() );
          return(-1);
  }


/* ------------------------------------------------------------------
 * Count total number of points
 * ------------------------------------------------------------------ */
  if ( wkbFlatten(poGeom->getGeometryType()) == wkbPoint )
  {
      numpoints = 1;
  }
  else if ( wkbFlatten(poGeom->getGeometryType()) == wkbLineString 
            || wkbFlatten(poGeom->getGeometryType()) == wkbLinearRing )
  {
      numpoints = ((OGRLineString*)poGeom)->getNumPoints();
  }
  else if ( wkbFlatten(poGeom->getGeometryType()) == wkbMultiPoint )
  {
      numpoints = ((OGRMultiPoint*)poGeom)->getNumGeometries();
  }
  else
  {
      msSetError(MS_OGRERR, 
                 "OGRGeometry type `%s' not supported yet.", 
                 "ogrGeomPoints()",
                 poGeom->getGeometryName() );
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
  if( wkbFlatten(poGeom->getGeometryType()) == wkbPoint )
  {
      OGRPoint *poPoint = (OGRPoint *)poGeom;
      ogrPointsAddPoint(line, poPoint->getX(), poPoint->getY(), 
                        outshp->numlines-1, &(outshp->bounds));
  }
  else if( wkbFlatten(poGeom->getGeometryType()) == wkbLineString 
           || wkbFlatten(poGeom->getGeometryType()) == wkbLinearRing )
  {
      OGRLineString *poLine = (OGRLineString *)poGeom;
      for(i=0; i<numpoints; i++)
          ogrPointsAddPoint(line, poLine->getX(i), poLine->getY(i),
                            outshp->numlines-1, &(outshp->bounds));
  }
  else if( wkbFlatten(poGeom->getGeometryType()) == wkbMultiPoint )
  {
      OGRPoint *poPoint;
      OGRMultiPoint *poMultiPoint = (OGRMultiPoint *)poGeom;
      for(i=0; i<numpoints; i++)
      {
          poPoint = (OGRPoint *)poMultiPoint->getGeometryRef( i );
          ogrPointsAddPoint(line, poPoint->getX(), poPoint->getY(), 
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
static int ogrGeomLine(OGRGeometry *poGeom, shapeObj *outshp,
                       int bCloseRings) 
{
  if (poGeom == NULL)
      return 0;

/* ------------------------------------------------------------------
 * Use recursive calls for complex geometries
 * ------------------------------------------------------------------ */
  OGRwkbGeometryType eGType = wkbFlatten(poGeom->getGeometryType());

  
  if ( eGType == wkbPolygon )
  {
      OGRPolygon *poPoly = (OGRPolygon *)poGeom;
      OGRLinearRing *poRing;

      if (outshp->type == MS_SHAPE_NULL)
          outshp->type = MS_SHAPE_POLYGON;

      for (int iRing=-1; iRing < poPoly->getNumInteriorRings(); iRing++)
      {
          if (iRing == -1)
              poRing = poPoly->getExteriorRing();
          else
              poRing = poPoly->getInteriorRing(iRing);

          if (ogrGeomLine(poRing, outshp, bCloseRings) == -1)
          {
              return -1;
          }
      }
  }
  else if ( eGType == wkbGeometryCollection
            || eGType == wkbMultiLineString 
            || eGType == wkbMultiPolygon )
  {
      OGRGeometryCollection *poColl = (OGRGeometryCollection *)poGeom;

      for (int iGeom=0; iGeom < poColl->getNumGeometries(); iGeom++)
      {
          if (ogrGeomLine(poColl->getGeometryRef(iGeom),
                          outshp, bCloseRings) == -1)
          {
              return -1;
          }
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
      OGRLineString *poLine = (OGRLineString *)poGeom;
      int       j, numpoints;
      lineObj   line={0,NULL};
      double    dX, dY;

      if ((numpoints = poLine->getNumPoints()) < 2)
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
          dX = line.point[j].x = poLine->getX(j); 
          dY = line.point[j].y = poLine->getY(j);

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
                 poGeom->getGeometryName() );
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
static int ogrConvertGeometry(OGRGeometry *poGeom, shapeObj *outshp,
                              enum MS_LAYER_TYPE layertype) 
{
/* ------------------------------------------------------------------
 * Process geometry according to layer type
 * ------------------------------------------------------------------ */
  int nStatus = MS_SUCCESS;

  if (poGeom == NULL)
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
      if(ogrGeomPoints(poGeom, outshp) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      break;
/* ------------------------------------------------------------------
 *      LINE layer
 * ------------------------------------------------------------------ */
    case MS_LAYER_LINE:
      if(ogrGeomLine(poGeom, outshp, MS_FALSE) == -1)
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
      if(ogrGeomLine(poGeom, outshp, MS_TRUE) == -1)
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
      switch(poGeom->getGeometryType())
      {
        case wkbPoint:
        case wkbPoint25D:
        case wkbMultiPoint:
        case wkbMultiPoint25D:
          if(ogrGeomPoints(poGeom, outshp) == -1)
          {
              nStatus = MS_FAILURE; // Error message already produced.
          }
          break;
        default:
          // Handle any non-point types as lines/polygons ... ogrGeomLine()
          // will decide the shape type
          if(ogrGeomLine(poGeom, outshp, MS_FALSE) == -1)
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
            return ogrConvertGeometry((OGRGeometry *)hGeometry,
                                      psShape,  MS_LAYER_POINT);
        else if (nType == wkbLineString || nType == wkbMultiLineString)
            return ogrConvertGeometry((OGRGeometry *)hGeometry,
                                      psShape,  MS_LAYER_LINE);
        else if (nType == wkbPolygon || nType == wkbMultiPolygon)
            return ogrConvertGeometry((OGRGeometry *)hGeometry,
                                      psShape,  MS_LAYER_POLYGON);
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
#define MSOGR_LABELTEXTNAME     "OGR:LabelText"
#define MSOGR_LABELTEXTINDEX    -100
#define MSOGR_LABELANGLENAME    "OGR:LabelAngle"
#define MSOGR_LABELANGLEINDEX   -101
#define MSOGR_LABELSIZENAME     "OGR:LabelSize"
#define MSOGR_LABELSIZEINDEX    -102


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
static char **msOGRGetValues(layerObj *layer, OGRFeature *poFeature)
{
  char **values;
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

  OGRStyleMgr *poStyleMgr = NULL;
  OGRStyleLabel *poLabelStyle = NULL;
  int *itemindexes = (int*)layer->iteminfo;

  for(i=0;i<layer->numitems;i++)
  {
    if (itemindexes[i] >= 0)
    {
        // Extract regular attributes
        values[i] = strdup(poFeature->GetFieldAsString(itemindexes[i]));
    }
    else
    {
        // Handle special OGR attributes coming from StyleString
        if (!poStyleMgr)
        {
            poStyleMgr = new OGRStyleMgr(NULL);
            poStyleMgr->InitFromFeature(poFeature);
            OGRStyleTool *poStylePart = poStyleMgr->GetPart(0);
            if (poStylePart && poStylePart->GetType() == OGRSTCLabel)
                poLabelStyle = (OGRStyleLabel*)poStylePart;
            else if (poStylePart)
                delete poStylePart;
        }
        GBool bDefault;
        if (poLabelStyle && itemindexes[i] == MSOGR_LABELTEXTINDEX)
        {
            values[i] = strdup(poLabelStyle->TextString(bDefault));
            //msDebug(MSOGR_LABELTEXTNAME " = \"%s\"\n", values[i]);
        }
        else if (poLabelStyle && itemindexes[i] == MSOGR_LABELANGLEINDEX)
        {
            values[i] = strdup(poLabelStyle->GetParamStr(OGRSTLabelAngle,
                                                         bDefault));
            //msDebug(MSOGR_LABELANGLENAME " = \"%s\"\n", values[i]);
        }
        else if (poLabelStyle && itemindexes[i] == MSOGR_LABELSIZEINDEX)
        {
            values[i] = strdup(poLabelStyle->GetParamStr(OGRSTLabelSize,
                                                         bDefault));
            //msDebug(MSOGR_LABELSIZENAME " = \"%s\"\n", values[i]);
        }
        else
        {
          msSetError(MS_OGRERR,"Invalid field index!?!","msOGRGetValues()");
          return(NULL);
        }
    }
  }

  if (poStyleMgr)
      delete poStyleMgr;
  if (poLabelStyle)
      delete poLabelStyle;

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
static int msOGRSpatialRef2ProjectionObj(OGRSpatialReference *poSRS,
                                         projectionObj *proj, int debug_flag )
{
#ifdef USE_PROJ
  // First flush the "auto" name from the projargs[]... 
  msFreeProjection( proj );

  if (poSRS == NULL || poSRS->IsLocal())
  {
      // Dataset had no set projection or is NonEarth (LOCAL_CS)... 
      // Nothing else to do. Leave proj empty and no reprojection will happen!
      return MS_SUCCESS;  
  }

  // Export OGR SRS to a PROJ4 string
  char *pszProj = NULL;

  ACQUIRE_OLD_OGR_LOCK;
  if (poSRS->exportToProj4( &pszProj ) != OGRERR_NONE ||
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

    OGRSpatialReference		oSRS;
    char			*pszAltWKT = (char *) pszWKT;
    OGRErr  eErr;

    ACQUIRE_OLD_OGR_LOCK;
    if( !EQUALN(pszWKT,"GEOGCS",6) 
        && !EQUALN(pszWKT,"PROJCS",6)
        && !EQUALN(pszWKT,"LOCAL_CS",8) )
        eErr = oSRS.SetFromUserInput( pszWKT );
    else
        eErr = oSRS.importFromWkt( &pszAltWKT );

    RELEASE_OLD_OGR_LOCK;

    if( eErr != OGRERR_NONE )
    {
        msSetError(MS_OGRERR, 
                   "Ingestion of WKT string '%s' failed.",
                   "msOGCWKT2ProjectionObj()",
                   pszWKT );
        return MS_FAILURE;
    }

    return msOGRSpatialRef2ProjectionObj( &oSRS, proj, debug_flag );
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
  OGRDataSource *poDS;

  poDS = (OGRDataSource *) msConnPoolRequest( layer );

/* -------------------------------------------------------------------- */
/*      If not, open now, and register this connection with the         */
/*      pool.                                                           */
/* -------------------------------------------------------------------- */
  if( poDS == NULL )
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
      poDS = OGRSFDriverRegistrar::Open( pszDSSelectedName );
      RELEASE_OGR_LOCK;
      
      if( poDS == NULL )
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

      msConnPoolRegister( layer, poDS, msOGRCloseConnection );
  }
      
  CPLFree( pszDSName );
  pszDSName = NULL;

/* ------------------------------------------------------------------
 * Find the layer selected.
 * ------------------------------------------------------------------ */
  
  int   nLayerIndex = 0;
  OGRLayer    *poLayer = NULL;

  int  iLayer;

  for( iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++ )
  {
      poLayer = poDS->GetLayer( iLayer );
      if( poLayer != NULL 
          && EQUAL(poLayer->GetLayerDefn()->GetName(),pszLayerDef) )
      {
          nLayerIndex = iLayer;
          break;
      }
      else
          poLayer = NULL;
  }
  
  if( poLayer == NULL && (atoi(pszLayerDef) > 0 || EQUAL(pszLayerDef,"0")) )
  {
      nLayerIndex = atoi(pszLayerDef);
      if( nLayerIndex < poDS->GetLayerCount() )
          poLayer = poDS->GetLayer( nLayerIndex );
  }

  if( poLayer == NULL && EQUALN(pszLayerDef,"SELECT",6) )
  {
      ACQUIRE_OGR_LOCK;
      poLayer = poDS->ExecuteSQL( pszLayerDef, NULL, NULL );
      if( poLayer == NULL )
      {
          msSetError(MS_OGRERR, 
                     "ExecuteSQL(%s) failed.\n%s",
                     "msOGRFileOpen()", 
                     pszLayerDef, CPLGetLastErrorMsg() );
          OGRDataSource::DestroyDataSource( poDS );
          CPLFree( pszLayerDef );
          RELEASE_OGR_LOCK;
          return NULL;
      }
      RELEASE_OGR_LOCK;
      nLayerIndex = -1;
  }

  if (poLayer == NULL)
  {
      msSetError(MS_OGRERR, "GetLayer(%s) failed for OGR connection `%s'.",
                 "msOGRFileOpen()", 
                 pszLayerDef, connection );
      CPLFree( pszLayerDef );
      ACQUIRE_OGR_LOCK;
      OGRDataSource::DestroyDataSource( poDS );
      RELEASE_OGR_LOCK;
      return NULL;
  }

  CPLFree( pszLayerDef );

/* ------------------------------------------------------------------
 * OK... open succeded... alloc and fill msOGRFileInfo inside layer obj
 * ------------------------------------------------------------------ */
  msOGRFileInfo *psInfo =(msOGRFileInfo*)CPLCalloc(1,sizeof(msOGRFileInfo));

  psInfo->pszFname = CPLStrdup(poDS->GetName());
  psInfo->nLayerIndex = nLayerIndex;
  psInfo->poDS = poDS;
  psInfo->poLayer = poLayer;

  psInfo->nTileId = 0;
  psInfo->poCurTile = NULL;
  psInfo->rect.minx = psInfo->rect.maxx = 0;
  psInfo->rect.miny = psInfo->rect.maxy = 0;

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
    OGRDataSource *poDS = (OGRDataSource *) conn_handle;

    ACQUIRE_OGR_LOCK;
    OGRDataSource::DestroyDataSource( poDS );
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
  if (psInfo->poLastFeature)
      OGRFeature::DestroyFeature( psInfo->poLastFeature );

  /* If nLayerIndex == -1 then the layer is an SQL result ... free it */
  if( psInfo->nLayerIndex == -1 )
      psInfo->poDS->ReleaseResultSet( psInfo->poLayer );

  // Release (potentially close) the datasource connection.
  // Make sure we aren't holding the lock when the callback may need it.
  RELEASE_OGR_LOCK;
  msConnPoolRelease( layer, psInfo->poDS );
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
  if (psInfo == NULL || psInfo->poLayer == NULL)
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

  OGRPolygon oSpatialFilter;
  OGRLinearRing oRing;

  oRing.setNumPoints(5);
  oRing.setPoint(0, rect.minx, rect.miny);
  oRing.setPoint(1, rect.maxx, rect.miny);
  oRing.setPoint(2, rect.maxx, rect.maxy);
  oRing.setPoint(3, rect.minx, rect.maxy);
  oRing.setPoint(4, rect.minx, rect.miny);

  oSpatialFilter.addRing( &oRing );

  psInfo->poLayer->SetSpatialFilter( &oSpatialFilter );

  psInfo->rect = rect;

/* ------------------------------------------------------------------
 * Apply an attribute filter if we have one prefixed with a WHERE 
 * keyword in the filter string.  Otherwise, ensure the attribute
 * filter is clear. 
 * ------------------------------------------------------------------ */
  if( layer->filter.string && EQUALN(layer->filter.string,"WHERE ",6) )
  {
      CPLErrorReset();
      if( psInfo->poLayer->SetAttributeFilter( layer->filter.string+6 )
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
      psInfo->poLayer->SetAttributeFilter( NULL );

/* ------------------------------------------------------------------
 * Reset current feature pointer
 * ------------------------------------------------------------------ */
  psInfo->poLayer->ResetReading();

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
  OGRFeatureDefn *poDefn;
  int i, numitems;
  char **items;

  if((poDefn = psInfo->poLayer->GetLayerDefn()) == NULL ||
     (numitems = poDefn->GetFieldCount()) == 0) 
  {
    msSetError(MS_OGRERR, 
               "OGR Connection for layer `%s' contains no fields.", 
               "msOGRFileGetItems()",
               layer->name?layer->name:"(null)" );
    return NULL;
  }

  if((items = (char**)malloc(sizeof(char *)*(numitems+1))) == NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRFileGetItems()");
    return NULL;
  }

  for(i=0;i<numitems;i++)
  {
      OGRFieldDefn *poField = poDefn->GetFieldDefn(i);
      items[i] = strdup(poField->GetNameRef());
  }                                  
  items[numitems] = NULL;

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
  OGRFeature *poFeature = NULL;

  if (psInfo == NULL || psInfo->poLayer == NULL)
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
      if( poFeature )
          OGRFeature::DestroyFeature(poFeature);

      if( (poFeature = psInfo->poLayer->GetNextFeature()) == NULL )
      {
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
              return MS_DONE;  // No more features to read
          }
      }

      if(layer->numitems > 0) 
      {
          shape->values = msOGRGetValues(layer, poFeature);
          shape->numvalues = layer->numitems;
          if(!shape->values)
          {
              OGRFeature::DestroyFeature(poFeature);
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
          if (ogrConvertGeometry(poFeature->GetGeometryRef(), shape,
                                 layer->type) == MS_SUCCESS)
          {
              if (shape->type != MS_SHAPE_NULL)
                  break; // Shape is ready to be returned!
          }
          else
          {
              msFreeShape(shape);
              OGRFeature::DestroyFeature(poFeature);
              RELEASE_OGR_LOCK;
              return MS_FAILURE; // Error message already produced.
          }
      }

      // Feature rejected... free shape to clear attributes values.
      msFreeShape(shape);
      shape->type = MS_SHAPE_NULL;
  }

  shape->index = poFeature->GetFID();
  shape->tileindex = psInfo->nTileId;

  // Keep ref. to last feature read in case we need style info.
  if (psInfo->poLastFeature)
      OGRFeature::DestroyFeature(psInfo->poLastFeature);
  psInfo->poLastFeature = poFeature;

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
                  msOGRFileInfo *psInfo )
{
  OGRFeature *poFeature;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRFileNextShape()");
    return(MS_FAILURE);
  }

/* ------------------------------------------------------------------
 * Handle shape geometry... 
 * ------------------------------------------------------------------ */
  msFreeShape(shape);
  shape->type = MS_SHAPE_NULL;

  ACQUIRE_OGR_LOCK;
  if( (poFeature = psInfo->poLayer->GetFeature(record)) == NULL )
  {
      RELEASE_OGR_LOCK;
      return MS_FAILURE;
  }

  // shape->type will be set if geom is compatible with layer type
  if (ogrConvertGeometry(poFeature->GetGeometryRef(), shape,
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
      shape->values = msOGRGetValues(layer, poFeature);
      shape->numvalues = layer->numitems;
      if(!shape->values)
      {
          RELEASE_OGR_LOCK;
          return(MS_FAILURE);
      }

  }   

  shape->index = poFeature->GetFID();

  // Keep ref. to last feature read in case we need style info.
  if (psInfo->poLastFeature)
      OGRFeature::DestroyFeature(psInfo->poLastFeature);
  psInfo->poLastFeature = poFeature;

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
        psInfo->poLayer->ResetReading();
    }
        
/* -------------------------------------------------------------------- */
/*      Get the name (connection string really) of the next tile.       */
/* -------------------------------------------------------------------- */
    OGRFeature *poFeature;
    char       *connection = NULL;
    msOGRFileInfo *psTileInfo = NULL;
    int status;

#ifndef IGNORE_MISSING_DATA
  NextFile:
#endif

    if( targetTile < 0 )
        poFeature = psInfo->poLayer->GetNextFeature();
    
    else
        poFeature = psInfo->poLayer->GetFeature( targetTile );

    if( poFeature == NULL )
    {
        RELEASE_OGR_LOCK;
        if( targetTile == -1 )
            return MS_DONE;
        else
            return MS_FAILURE;
        
    }

    connection = strdup(poFeature->GetFieldAsString( layer->tileitem ));
    
    nFeatureId = poFeature->GetFID();

    OGRFeature::DestroyFeature(poFeature);
                        
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

      OGRFeatureDefn *poDefn = psInfo->poLayer->GetLayerDefn();
      for( layer->tileitemindex = 0; 
           layer->tileitemindex < poDefn->GetFieldCount()
           && !EQUAL(poDefn->GetFieldDefn(layer->tileitemindex)->GetNameRef(),
                         layer->tileitem); 
           layer->tileitemindex++ ) {}

      if( layer->tileitemindex == poDefn->GetFieldCount() )
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
      OGRSpatialReference *poSRS = psInfo->poLayer->GetSpatialRef();

      if (msOGRSpatialRef2ProjectionObj(poSRS,
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

  if (psInfo == NULL || psInfo->poLayer == NULL)
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
  OGRFeatureDefn *poDefn;

  if (layer->numitems == 0)
      return MS_SUCCESS;

  if( layer->tileindex != NULL )
  {
      if( psInfo->poCurTile == NULL 
          && msOGRFileReadTile( layer, psInfo, -2 ) != MS_SUCCESS )
          return MS_FAILURE;
      
      psInfo = psInfo->poCurTile;
  }

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
      msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
                 "msOGRLayerInitItemInfo()");
      return(MS_FAILURE);
  }

  if((poDefn = psInfo->poLayer->GetLayerDefn()) == NULL) 
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
      // OGR style strings.  We use special attribute names.
      if (EQUAL(layer->items[i], MSOGR_LABELTEXTNAME))
          itemindexes[i] = MSOGR_LABELTEXTINDEX;
      else if (EQUAL(layer->items[i], MSOGR_LABELANGLENAME))
          itemindexes[i] = MSOGR_LABELANGLEINDEX;
      else if (EQUAL(layer->items[i], MSOGR_LABELSIZENAME))
          itemindexes[i] = MSOGR_LABELSIZEINDEX;
      else
          itemindexes[i] = poDefn->GetFieldIndex(layer->items[i]);
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

  if (psInfo == NULL || psInfo->poLayer == NULL)
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
 * Returns shape from OGR data source by id.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerGetShape(layerObj *layer, shapeObj *shape, int tile, 
                       long record)
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->layerinfo;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerNextShape()");
    return(MS_FAILURE);
  }

  if( layer->tileindex == NULL )
      return msOGRFileGetShape(layer, shape, record, psInfo );
  else
  {
      if( psInfo->poCurTile == NULL
          || psInfo->poCurTile->nTileId != tile )
      {
          if( msOGRFileReadTile( layer, psInfo, tile ) != MS_SUCCESS )
              return MS_FAILURE;
      }

      return msOGRFileGetShape(layer, shape, record, psInfo->poCurTile );
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

  if (psInfo == NULL || psInfo->poLayer == NULL)
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
  if (psInfo->poLayer->GetExtent(&oExtent, TRUE) != OGRERR_NONE)
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

    if (pszSymbolId && pszSymbolId[0] != '\0' &&
        (params = msStringSplit(pszSymbolId, '.', &numparams))!=NULL)
    {
        for(int j=0; j<numparams && nSymbol == -1; j++)
        {
            nSymbol = msGetSymbolIndex(symbolset, params[j], MS_FALSE);
        }
        msFreeCharArray(params, numparams);
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

  if (psInfo == NULL || psInfo->poLayer == NULL)
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
  if (psInfo->poLastFeature == NULL || 
      psInfo->poLastFeature->GetFID() != record)
  {
      if (psInfo->poLastFeature)
          OGRFeature::DestroyFeature(psInfo->poLastFeature);

      psInfo->poLastFeature = psInfo->poLayer->GetFeature(record);
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
  if (psInfo->poLastFeature)
  {
      GBool bIsNull, bIsBrush=FALSE, bIsPen=FALSE;
      int r=0,g=0,b=0,t=0;

      OGRStyleMgr *poStyleMgr = new OGRStyleMgr(NULL);
      poStyleMgr->InitFromFeature(psInfo->poLastFeature);
      // msDebug("OGRStyle: %s\n", psInfo->poLastFeature->GetStyleString());

      int numParts = poStyleMgr->GetPartCount();
      for(int i=0; i<numParts; i++)
      {
          OGRStyleTool *poStylePart = poStyleMgr->GetPart(i);
          if (!poStylePart)
              continue;

          // We want all size values returned in pixels.
          //
          // The scale factor that OGR expect is the ground/paper scale
          // e.g. if 1 ground unit = 0.01 paper unit then scale=1/0.01=100
          // cellsize if number of ground units/pixel, and OGR assumes that
          // there is 72*39.37 pixels/ground units (since meter is assumed 
          // for ground... but what ground units we have does not matter
          // as long as use the same assumptions everywhere)
          // That gives scale = cellsize*72*39.37

          poStylePart->SetUnit(OGRSTUPixel, map->cellsize*72.0*39.37);

          if (poStylePart->GetType() == OGRSTCLabel)
          {
              OGRStyleLabel *poLabelStyle = (OGRStyleLabel*)poStylePart;

              // Enclose the text sting inside quotes to make sure it is seen
              // as a string by the parser inside loadExpression(). (bug185)
              msLoadExpressionString(&(c->text), 
                         (char*)CPLSPrintf("\"%s\"", 
                                           poLabelStyle->TextString(bIsNull)));

              c->label.angle = poLabelStyle->Angle(bIsNull);

              c->label.size = (int)poLabelStyle->Size(bIsNull);
              if( c->label.size < 1 ) /* no point dropping to zero size */
                  c->label.size = 1;

              // msDebug("** Label size=%d, angle=%f, string=%s **\n", c->label.size, c->label.angle, poLabelStyle->TextString(bIsNull));

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
                  MS_INIT_COLOR(c->label.color, r, g, b);
              }

              // Label font... do our best to use TrueType fonts, otherwise
              // fallback on bitmap fonts.
#if defined(USE_GD_TTF) || defined (USE_GD_FT)
              const char *pszName;
              if ((pszName = poLabelStyle->FontName(bIsNull)) != NULL && 
                  !bIsNull && pszName[0] != '\0' &&
                  msLookupHashTable(&(map->fontset.fonts), (char*)pszName) != NULL)
              {
                  c->label.type = MS_TRUETYPE;
                  c->label.font = strdup(pszName);
                  // msDebug("** Using '%s' TTF font **\n", pszName);
              }
              else if (msLookupHashTable(&(map->fontset.fonts),"default") != NULL)
              {
                  c->label.type = MS_TRUETYPE;
                  c->label.font = strdup("default");
                  // msDebug("** Using 'default' TTF font **\n");
              }
              else
#endif
              {
                  c->label.type = MS_BITMAP;
                  c->label.size = MS_MEDIUM;
                  // msDebug("** Using 'medium' BITMAP font **\n");
              }
          }
          else if (poStylePart->GetType() == OGRSTCPen)
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
                      // msDebug("** PEN COLOR = %d %d %d (%d)**\n", r,g,b, nPenColor);
                  }

                  nPenSize = (int)poPenStyle->Width(bIsNull);
                  if (bIsNull)
                      nPenSize = 1;
                  if (pszPenName!=NULL || nPenSize > 1)
                  {
                      // Thick line or patterned line style
                      //
                      // First try to match pen name in symbol file
                      // If not found then look for a "default-circle" symbol
                      // that we'll use for producing thick lines.  
                      // Otherwise symbol will be set to 0 and line will 
                      // be 1 pixel wide.
                      nPenSymbol = msOGRGetSymbolId(&(map->symbolset),
                                                    pszPenName, 
                                           (nPenSize>1)?"default-circle":NULL);
                  }
              }
              //msDebug("** PEN COLOR = %d %d %d **\n", oPenColor.red,oPenColor.green,oPenColor.blue);
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
              }

          }
          else if (poStylePart->GetType() == OGRSTCBrush)
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
                      // msDebug("** BRUSH COLOR = %d %d %d (%d)**\n", r,g,b,c->color);
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
          else if (poStylePart->GetType() == OGRSTCSymbol)
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

          delete poStylePart;

      }

      if (poStyleMgr)
          delete poStyleMgr;

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
        delete OGRSFDriverRegistrar::GetRegistrar();
        CPLFinderClean();
        bOGRDriversRegistered = MS_FALSE;
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


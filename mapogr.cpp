/**********************************************************************
 * $Id$
 *
 * Name:     mapogr.cpp
 * Project:  MapServer
 * Language: C++
 * Purpose:  OGR Link
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2000-2002, Daniel Morissette, DM Solutions Group Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 * $Log$
 * Revision 1.58  2002/12/13 00:57:31  dan
 * Modified WFS implementation to behave more as a real vector data source
 *
 * Revision 1.57  2002/11/17 17:47:51  frank
 * use msTryBuildPath() for open statement
 *
 * Revision 1.56  2002/09/23 13:33:26  julien
 * Swapped map_path for mappath for consistency.
 *
 * Revision 1.55  2002/09/17 13:08:28  julien
 * Remove all chdir() function and replace them with the new msBuildPath function.
 * This have been done to make MapServer thread safe. (Bug 152)
 *
 * Revision 1.54  2002/08/16 20:50:10  julien
 * Fixed for new styleObj except for styleitem auto
 *
 * Revision 1.53  2002/08/14 17:30:44  dan
 * Fixed problem with numeric labels with STYLEITEM AUTO (bug 185)
 *
 * Revision 1.52  2002/07/11 18:38:57  frank
 * allow layer names as well as layer indexes in connection string.
 *
 * Revision 1.51  2002/04/26 18:00:35  julien
 * Enabled OGRMultiPoint in ogrGeomLine and ogrGeomPoint
 *
 * Revision 1.50  2002/04/04 19:05:41  frank
 * improve error reporting
 *
 * Revision 1.49  2002/03/26 21:00:04  frank
 * Set the tileindex value in shapeObjs.  Support fetching based on tile+recordid.
 * Support refetching features for autostyling in tiled layers.
 *
 * Revision 1.48  2002/03/26 16:53:12  dan
 * Fixed msDebug() format string in msOGRFileClose()
 *
 * Revision 1.47  2002/03/18 20:40:14  frank
 * initial pass on tiled ogr support
 *
 * Revision 1.46  2002/02/22 22:56:26  dan
 * Avoid filling an object with a pen and no brush in layers of type POLYGON
 * when using auto styling.
 *
 * Revision 1.45  2002/02/20 22:28:23  frank
 * fixed some problems with 3D geometry support
 *
 * Revision 1.44  2002/02/20 19:49:22  frank
 * fixed memory leak of OGRFeatures under some circumstanes
 *
 * Revision 1.43  2002/02/08 21:29:18  frank
 * ensure 3D geometries supported as well as 2D geometries
 *
 * Revision 1.42  2002/01/15 01:00:51  dan
 * Attempt at fixing thick polygon outline problem (bug 92).  Also rewrote
 * the symbol mapping for brushes and pens at the same time to use the same
 * rules as point symbols.
 *
 * Revision 1.41  2002/01/09 05:10:28  frank
 * avoid use of ms_error global
 *
 * ...
 *
 * Revision 1.1  2000/08/25 18:41:05  dan
 * Added optional OGR support
 *
 **********************************************************************/

#include "map.h"
#include "mapproject.h"

#if defined(USE_OGR) || defined(USE_GDAL)
#  include "ogr_spatialref.h"
#  include "cpl_conv.h"
#  include "cpl_string.h"
#endif

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

/* ==================================================================
 * Geometry conversion functions
 * ================================================================== */

/**********************************************************************
 *                     ogrPointsAddPoint()
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
    line->numpoints++;
}

/**********************************************************************
 *                     ogrGeomPoints()
 **********************************************************************/
static int ogrGeomPoints(OGRGeometry *poGeom, shapeObj *outshp) 
{
  int   i;
  int   numpoints;
  lineObj line={0,NULL};

  if (poGeom == NULL)   
      return 0;

/* ------------------------------------------------------------------
 * Count total number of points
 * ------------------------------------------------------------------ */
  if (poGeom->getGeometryType() == wkbPoint
      || poGeom->getGeometryType() == wkbPoint25D )
  {
      numpoints = 1;
  }
  else if (poGeom->getGeometryType() == wkbLineString
           || poGeom->getGeometryType() == wkbLineString25D )
  {
      numpoints = ((OGRLineString*)poGeom)->getNumPoints();
  }
  else if (poGeom->getGeometryType() == wkbPolygon
           || poGeom->getGeometryType() == wkbPolygon25D )
  {
      OGRPolygon *poPoly = (OGRPolygon *)poGeom;
      OGRLinearRing *poRing = poPoly->getExteriorRing();

      numpoints=0;
      if (poRing)
          numpoints = poRing->getNumPoints();

      for (int iRing=0; iRing < poPoly->getNumInteriorRings(); iRing++)
      {
          poRing = poPoly->getInteriorRing(iRing);
          if (poRing)
              numpoints += poRing->getNumPoints();
      }
  }
  else if (poGeom->getGeometryType() == wkbMultiPoint )
  {
      numpoints = ((OGRMultiPoint*)poGeom)->getNumGeometries();
  }
  else
  {
      msSetError(MS_OGRERR, 
                 (char*)CPLSPrintf("OGRGeometry type `%s' not supported yet.", 
                                   poGeom->getGeometryName()),
                 "ogrGeomPoints()");
      return(-1);
  }

/* ------------------------------------------------------------------
 * alloc buffer and filter/transform points
 * ------------------------------------------------------------------ */
  line.numpoints = 0;
  line.point = (pointObj *)malloc(sizeof(pointObj)*numpoints);
  if(!line.point) 
  {
      msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", 
                 "ogrGeomPoints()");
      return(-1);
  }
   
  if (poGeom->getGeometryType() == wkbPoint
      || poGeom->getGeometryType() == wkbPoint25D )
  {
      OGRPoint *poPoint = (OGRPoint *)poGeom;
      ogrPointsAddPoint(&line, poPoint->getX(), poPoint->getY(), 
                        outshp->numlines, &(outshp->bounds));
  }
  else if (poGeom->getGeometryType() == wkbLineString
           || poGeom->getGeometryType() == wkbLineString25D)
  {
      OGRLineString *poLine = (OGRLineString *)poGeom;
      for(i=0; i<numpoints; i++)
          ogrPointsAddPoint(&line, poLine->getX(i), poLine->getY(i),
                            outshp->numlines, &(outshp->bounds));
  }
  else if (poGeom->getGeometryType() == wkbPolygon
           || poGeom->getGeometryType() == wkbPolygon25D)
  {
      OGRPolygon *poPoly = (OGRPolygon *)poGeom;
      OGRLinearRing *poRing;

      for (int iRing=-1; iRing < poPoly->getNumInteriorRings(); iRing++)
      {
          if (iRing == -1)
              poRing = poPoly->getExteriorRing();
          else
              poRing = poPoly->getInteriorRing(iRing);

          if (poRing)
          {
              for(i=0; i<poRing->getNumPoints(); i++)
                  ogrPointsAddPoint(&line, poRing->getX(i), poRing->getY(i),
                                    outshp->numlines, &(outshp->bounds));
          }
      }
  }
  if (poGeom->getGeometryType() == wkbMultiPoint )
  {
      OGRPoint *poPoint;
      OGRMultiPoint *poMultiPoint = (OGRMultiPoint *)poGeom;
      for(i=0; i<numpoints; i++)
      {
          poPoint = (OGRPoint *)poMultiPoint->getGeometryRef( i );
          ogrPointsAddPoint(&line, poPoint->getX(), poPoint->getY(), 
                            outshp->numlines, &(outshp->bounds));
      }
  }

  msAddLine(outshp, &line);
  free(line.point);

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
  if (poGeom->getGeometryType() == wkbPolygon 
      || poGeom->getGeometryType() == wkbPolygon25D )
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
  else if (poGeom->getGeometryType() == wkbGeometryCollection ||
           poGeom->getGeometryType() == wkbMultiLineString ||
           poGeom->getGeometryType() == wkbMultiPolygon ||
           poGeom->getGeometryType() == wkbMultiPolygon25D )
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
  else if (poGeom->getGeometryType() == wkbPoint
           || poGeom->getGeometryType() == wkbPoint25D 
           || poGeom->getGeometryType() == wkbMultiPoint )
  {
      /* Hummmm a point when we're drawing lines/polygons... just drop it! */
  }
/* ------------------------------------------------------------------
 * OGRLinearRing/OGRLineString ... both are of type wkbLineString
 * ------------------------------------------------------------------ */
  else if (poGeom->getGeometryType() == wkbLineString
           || poGeom->getGeometryType() == wkbLineString25D )
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

      msAddLine(outshp, &line);
      free(line.point);
  }
  else
  {
      msSetError(MS_OGRERR, 
                 (char*)CPLSPrintf("OGRGeometry type `%s' not supported.",
                                   poGeom->getGeometryName()),
                 "ogrGeomLine()");
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

/**********************************************************************
 *                    msLoadWKTProjectionString()
 *
 * Init a MapServer projectionObj using an OGC WKT string.
 * Like the existing msOGCWKT2ProjectionObj only _without_ the
 * checks for projection AUTO used by gdal layers.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msLoadWKTProjectionString( const char *pszWKT, 
                            projectionObj *proj )

{
#if defined(USE_OGR) || defined(USE_GDAL)

    OGRSpatialReference		oSRS;
    char			*pszAltWKT = (char *) pszWKT;

    if( oSRS.importFromWkt( &pszAltWKT ) != OGRERR_NONE )
    {
        msSetError(MS_OGRERR, 
                   "Ingestion of WKT string failed.",
                   "msLoadWKTProjectionString()");
        return MS_FAILURE;
    }

#ifdef USE_PROJ
    // First flush the projargs[]... 
    msFreeProjection( proj );

    if (oSRS.IsLocal())
    {
        // Dataset had no set projection or is NonEarth (LOCAL_CS)... 
        // Nothing else to do. Leave proj empty and no reprojection will happen!
        return MS_SUCCESS;  
    }

    // Export OGR SRS to a PROJ4 string
    char *pszProj = NULL;

    if (oSRS.exportToProj4( &pszProj ) != OGRERR_NONE ||
      pszProj == NULL || strlen(pszProj) == 0)
    {
      msSetError(MS_OGRERR, "Conversion from OGR SRS to PROJ4 failed.",
                 "msLoadWKTProjectionString()");
      CPLFree(pszProj);
      return(MS_FAILURE);
    }

    msDebug( "AUTO = %s\n", pszProj );

    if( msLoadProjectionString( proj, pszProj ) != 0 )
      return MS_FAILURE;

    CPLFree(pszProj);
#endif

    return MS_SUCCESS;

#else
    msSetError(MS_OGRERR, 
               "Not implemented since neither OGR nor GDAL is enabled.",
               "msLoadWKTProjectionString()");
    return MS_FAILURE;
#endif
}


#if defined(USE_OGR) || defined(USE_GDAL)

/**********************************************************************
 *                     msOGRSpatialRef2ProjectionObj()
 *
 * Init a MapServer projectionObj using an OGRSpatialRef
 * Works only with PROJECTION AUTO
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRSpatialRef2ProjectionObj(OGRSpatialReference *poSRS,
                                  projectionObj *proj)
{
  if (proj->numargs == 0  || !EQUAL(proj->args[0], "auto"))
      return MS_SUCCESS;  // Nothing to do!

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

  if (poSRS->exportToProj4( &pszProj ) != OGRERR_NONE ||
      pszProj == NULL || strlen(pszProj) == 0)
  {
      msSetError(MS_OGRERR, "Conversion from OGR SRS to PROJ4 failed.",
                 "msOGRSpatialRef2ProjectionObj()");
      CPLFree(pszProj);
      return(MS_FAILURE);
  }

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
                            projectionObj *proj )

{
#if defined(USE_OGR) || defined(USE_GDAL)

    OGRSpatialReference		oSRS;
    char			*pszAltWKT = (char *) pszWKT;

    if( oSRS.importFromWkt( &pszAltWKT ) != OGRERR_NONE )
    {
        msSetError(MS_OGRERR, 
                   "Ingestion of WKT string failed.",
                   "msOGCWKT2ProjectionObj()");
        return MS_FAILURE;
    }

    return msOGRSpatialRef2ProjectionObj( &oSRS, proj );
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

static msOGRFileInfo *
msOGRFileOpen(layerObj *layer, const char *connection ) 

{
/* ------------------------------------------------------------------
 * Register OGR Drivers, only once per execution
 * ------------------------------------------------------------------ */
  static int bDriversRegistered = MS_FALSE;
  if (!bDriversRegistered)
      OGRRegisterAll();
  bDriversRegistered = MS_TRUE;

/* ------------------------------------------------------------------
 * Parse connection string into dataset name, and layer name. 
 * ------------------------------------------------------------------ */
  char **papszTokens = NULL, szPath[MS_MAXPATHLEN];

  if( connection != NULL )
      papszTokens = CSLTokenizeStringComplex( connection, ",", TRUE, FALSE );

  if(connection==NULL || CSLCount( papszTokens ) < 1 )
  {
      msSetError(MS_OGRERR, 
                 "Error parsing OGR connection information:%s", 
                 "msOGRFileOpen()",
                 connection );
      CSLDestroy( papszTokens );
      return NULL;
  }

/* ------------------------------------------------------------------
 * Attempt to open OGR dataset
 * ------------------------------------------------------------------ */
  OGRDataSource *poDS;

  msDebug("msOGRFileOpen(%s)...\n", connection);

  poDS = OGRSFDriverRegistrar::Open( 
      msTryBuildPath(szPath, layer->map->mappath, papszTokens[0]) );
  if( poDS == NULL )
  {
      msSetError(MS_OGRERR, 
                 (char*)CPLSPrintf("Open failed for OGR connection `%s'.  "
                                   "File not found or unsupported format.", 
                                   connection),
                 "msOGRFileOpen()");
      CSLDestroy( papszTokens );
      return NULL;
  }

/* ------------------------------------------------------------------
 * Find the layer selected.
 * ------------------------------------------------------------------ */
  int   nLayerIndex = 0;
  OGRLayer    *poLayer = NULL;

  if( CSLCount(papszTokens) == 1 )
  {
      poLayer = poDS->GetLayer( 0 );
      nLayerIndex = 0;
  }
  else
  {
      int  iLayer;

      for( iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++ )
      {
          poLayer = poDS->GetLayer( iLayer );
          if( poLayer != NULL 
              && EQUAL(poLayer->GetLayerDefn()->GetName(),papszTokens[1]) )
          {
              nLayerIndex = iLayer;
              break;
          }
          else
              poLayer = NULL;
      }

      if( poLayer == NULL 
          && (atoi(papszTokens[1]) > 0 || EQUAL(papszTokens[1],"0")) )
      {
          nLayerIndex = atoi(papszTokens[1]);
          poLayer = poDS->GetLayer( nLayerIndex );
      }
  }

  if (poLayer == NULL)
  {
      msSetError(MS_OGRERR, "GetLayer(%s) failed for OGR connection `%s'.",
                 "msOGRFileOpen()", 
                 papszTokens[1], connection );
      CSLDestroy( papszTokens );
      delete poDS;
      return NULL;
  }

/* ------------------------------------------------------------------
 * OK... open succeded... alloc and fill msOGRFileInfo inside layer obj
 * ------------------------------------------------------------------ */
  msOGRFileInfo *psInfo =(msOGRFileInfo*)CPLCalloc(1,sizeof(msOGRFileInfo));

  psInfo->pszFname = CPLStrdup(papszTokens[0]);
  psInfo->nLayerIndex = nLayerIndex;
  psInfo->poDS = poDS;
  psInfo->poLayer = poLayer;

  psInfo->nTileId = 0;
  psInfo->poCurTile = NULL;
  psInfo->rect.minx = psInfo->rect.maxx = 0;
  psInfo->rect.miny = psInfo->rect.maxy = 0;

  // Cleanup and exit;
  CSLDestroy( papszTokens );

  return psInfo;
}

/**********************************************************************
 *                     msOGRFileClose()
 **********************************************************************/
static int msOGRFileClose(layerObj *layer, msOGRFileInfo *psInfo ) 
{
  if (!psInfo)
      return MS_SUCCESS;

  msDebug("msOGRFileClose(%s,%d).\n", psInfo->pszFname, psInfo->nLayerIndex);

  CPLFree(psInfo->pszFname);

  if (psInfo->poLastFeature)
      delete psInfo->poLastFeature;

  /* Destroying poDS automatically closes files, destroys the layer, etc. */
  delete psInfo->poDS;

  // Free current tile if there is one.
  if( psInfo->poCurTile != NULL )
      msOGRFileClose( layer, psInfo->poCurTile );

  CPLFree(psInfo);

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
  OGRLinearRing oSpatialFilter;

  oSpatialFilter.setNumPoints(5);
  oSpatialFilter.setPoint(0, rect.minx, rect.miny);
  oSpatialFilter.setPoint(1, rect.maxx, rect.miny);
  oSpatialFilter.setPoint(2, rect.maxx, rect.maxy);
  oSpatialFilter.setPoint(3, rect.minx, rect.maxy);
  oSpatialFilter.setPoint(4, rect.minx, rect.miny);

  psInfo->poLayer->SetSpatialFilter( &oSpatialFilter );

  psInfo->rect = rect;

/* ------------------------------------------------------------------
 * Reset current feature pointer
 * ------------------------------------------------------------------ */
  psInfo->poLayer->ResetReading();

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
               "Layer %s,%d contains no fields.", 
               "msOGRFileGetItems()",
               psInfo->pszFname, psInfo->nLayerIndex );
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

  while (shape->type == MS_SHAPE_NULL)
  {
      if( poFeature )
          delete poFeature;

      if( (poFeature = psInfo->poLayer->GetNextFeature()) == NULL )
      {
          return MS_DONE;  // No more features to read
      }

      if(layer->numitems > 0) 
      {
          shape->values = msOGRGetValues(layer, poFeature);
          shape->numvalues = layer->numitems;
          if(!shape->values)
          {
              delete poFeature;
              return(MS_FAILURE);
          }
      }

      if (msEvalExpression(&(layer->filter), layer->filteritemindex, 
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
              delete poFeature;
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
      delete psInfo->poLastFeature;
  psInfo->poLastFeature = poFeature;

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

  if( (poFeature = psInfo->poLayer->GetFeature(record)) == NULL )
  {
      return MS_FAILURE;
  }

  // shape->type will be set if geom is compatible with layer type
  if (ogrConvertGeometry(poFeature->GetGeometryRef(), shape,
                         layer->type) != MS_SUCCESS)
  {
      return MS_FAILURE; // Error message already produced.
  }
  
  if (shape->type == MS_SHAPE_NULL)
  {
      msSetError(MS_OGRERR, 
                 "Requested feature is incompatible with layer type",
                 "msOGRLayerGetShape()");
      return MS_FAILURE;
  }

/* ------------------------------------------------------------------
 * Process shape attributes
 * ------------------------------------------------------------------ */
  if(layer->numitems > 0) 
  {
      shape->values = msOGRGetValues(layer, poFeature);
      shape->numvalues = layer->numitems;
      if(!shape->values) return(MS_FAILURE);
  }   

  shape->index = poFeature->GetFID();

  // Keep ref. to last feature read in case we need style info.
  if (psInfo->poLastFeature)
      delete psInfo->poLastFeature;
  psInfo->poLastFeature = poFeature;

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
/*      Get the name (connection string really) of the next tile.       */
/* -------------------------------------------------------------------- */
    OGRFeature *poFeature;
    char       *connection = NULL;
    msOGRFileInfo *psTileInfo = NULL;
    int status;

#ifndef IGNORE_MISSING_DATA
  NextFile:
#endif
    
    if( targetTile == -1 )
        poFeature = psInfo->poLayer->GetNextFeature();
    
    else
        poFeature = psInfo->poLayer->GetFeature( targetTile );

    if( poFeature == NULL )
    {
        if( targetTile == -1 )
            return MS_DONE;
        else
            return MS_FAILURE;
        
    }

    connection = strdup(poFeature->GetFieldAsString( layer->tileitem ));
    
    nFeatureId = poFeature->GetFID();

    delete poFeature;
                        
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

  if (layer->ogrlayerinfo != NULL)
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
      layer->ogrlayerinfo = psInfo;
      layer->tileitemindex = -1;
      
      if( layer->ogrlayerinfo == NULL )
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
      layer->ogrlayerinfo = psInfo;
      
      if( layer->ogrlayerinfo == NULL )
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
          layer->ogrlayerinfo = NULL;
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
      OGRSpatialReference *poSRS = psInfo->poLayer->GetSpatialRef();

      if (msOGRSpatialRef2ProjectionObj(poSRS,
                                        &(layer->projection)) != MS_SUCCESS)
      {
          errorObj *ms_error = msGetErrorObj();

          msSetError(MS_OGRERR, 
                     "%s  "
                     "PROJECTION AUTO cannot be used for this "
                     "OGR connection (`%s').",
                     "msOGRLayerOpen()",
                     ms_error->message, 
                     (pszOverrideConnection ? pszOverrideConnection:
                                              layer->connection) );
          msOGRFileClose( layer, psInfo );
          layer->ogrlayerinfo = NULL;
          return(MS_FAILURE);
      }
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
 *                     msOGRLayerClose()
 **********************************************************************/
int msOGRLayerClose(layerObj *layer) 
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->ogrlayerinfo;

  if (psInfo)
  {
      msDebug("msOGRLayerClose(%s).\n", layer->connection);

      msOGRFileClose( layer, psInfo );
      layer->ogrlayerinfo = NULL;
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
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->ogrlayerinfo;
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
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->ogrlayerinfo;
  
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
int msOGRLayerInitItemInfo(layerObj *layer)
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->ogrlayerinfo;
  int   i;
  OGRFeatureDefn *poDefn;

  if (layer->numitems == 0)
      return MS_SUCCESS;

  if( layer->tileindex != NULL )
  {
      if( psInfo->poCurTile == NULL 
          && msOGRFileReadTile( layer, psInfo ) != MS_SUCCESS )
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
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->ogrlayerinfo;
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
int msOGRLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record)
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->ogrlayerinfo;

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
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->ogrlayerinfo;
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
  if (psInfo->poLayer->GetExtent(&oExtent, TRUE) != OGRERR_NONE)
  {
    msSetError(MS_MISCERR, "Unable to get extents for this layer.", 
               "msOGRLayerGetExtent()");
    return(MS_FAILURE);
  }

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
        (params = split(pszSymbolId, '.', &numparams))!=NULL)
    {
        for(int j=0; j<numparams && nSymbol == -1; j++)
        {
            nSymbol = msGetSymbolIndex(symbolset, params[j]);
        }
        msFreeCharArray(params, numparams);
    }
    if (nSymbol == -1 && pszDefaultSymbol)
    {
        nSymbol = msGetSymbolIndex(symbolset, (char*)pszDefaultSymbol);
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
int msOGRLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c,
                           int tile, long record)
{
#ifdef USE_OGR
  msOGRFileInfo *psInfo =(msOGRFileInfo*)layer->ogrlayerinfo;

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
  if (psInfo->poLastFeature == NULL || 
      psInfo->poLastFeature->GetFID() != record)
  {
      if (psInfo->poLastFeature)
          delete psInfo->poLastFeature;

      psInfo->poLastFeature = psInfo->poLayer->GetFeature(record);
  }

/* ------------------------------------------------------------------
 * Reset style info in the class to defaults
 * the only members we don't touch are name, expression, and join/query stuff
 * ------------------------------------------------------------------ */
  resetClassStyle(c);

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
              loadExpressionString(&(c->text), 
                         (char*)CPLSPrintf("\"%s\"", 
                                           poLabelStyle->TextString(bIsNull)));

              c->label.angle = poLabelStyle->Angle(bIsNull);

              c->label.size = (int)poLabelStyle->Size(bIsNull);

              // msDebug("** Label size=%d, angle=%f, string=%s **\n", c->label.size, c->label.angle, poLabelStyle->TextString(bIsNull));

              // OGR default is anchor point = LL, so label is at UR of anchor
              c->label.position = MS_UR;

              if (poLabelStyle->GetRGBFromString(poLabelStyle->
                                                 ForeColor(bIsNull),r,g,b,t))
              {
                  MS_INIT_COLOR(c->label.color, r, g, b);
              }
              if (poLabelStyle->GetRGBFromString(poLabelStyle->
                                                 BackColor(bIsNull),r,g,b,t) 
                  && !bIsNull)
              {
                  MS_INIT_COLOR(c->label.color, r, g, b);
              }

              // Label font... do our best to use TrueType fonts, otherwise
              // fallback on bitmap fonts.
#if defined(USE_GD_TTF) || defined (USE_GD_FT)
              const char *pszName;
              if ((pszName = poLabelStyle->FontName(bIsNull)) != NULL && 
                  !bIsNull && pszName[0] != '\0' &&
                  msLookupHashTable(map->fontset.fonts, (char*)pszName) != NULL)
              {
                  c->label.type = MS_TRUETYPE;
                  c->label.font = strdup(pszName);
                  // msDebug("** Using '%s' TTF font **\n", pszName);
              }
              else if (msLookupHashTable(map->fontset.fonts,"default") != NULL)
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

              // Check for Pen Pattern "ogr-pen-1": the invisible pen
              // If that's what we have then set pen color to -1
              if (pszPenName && strstr(pszPenName, "ogr-pen-1") != NULL)
              {
                  MS_INIT_COLOR(oPenColor, -1, -1, -1);
              }
              else
              {
                  if (poPenStyle->GetRGBFromString(poPenStyle->
                                               Color(bIsNull),r,g,b,t))
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
                  // overlaysymbol params (also set outlinecolor just in case)
                  c->styles[0].outlinecolor = c->styles[1].outlinecolor = 
                      oPenColor;
                  c->styles[1].size = nPenSize;
                  c->styles[1].symbol = nPenSymbol;
                  c->numstyles = 2;
              }
              else
              {
                  // Single part symbology
                  c->styles[0].outlinecolor = c->styles[0].color = oPenColor;
                  c->styles[0].symbol = nPenSymbol;
                  c->styles[0].size = nPenSize;
              }

          }
          else if (poStylePart->GetType() == OGRSTCBrush)
          {
              OGRStyleBrush *poBrushStyle = (OGRStyleBrush*)poStylePart;
              bIsBrush = TRUE;

              if (poBrushStyle->GetRGBFromString(poBrushStyle->
                                                 ForeColor(bIsNull),r,g,b,t))
              {
                  MS_INIT_COLOR(c->styles[0].color, r, g, b);
                  // msDebug("** BRUSH COLOR = %d %d %d (%d)**\n", r,g,b,c->color);
              }
              if (poBrushStyle->GetRGBFromString(poBrushStyle->
                                                 BackColor(bIsNull),r,g,b,t) 
                  && !bIsNull)
              {
                  MS_INIT_COLOR(c->styles[0].backgroundcolor, r, g, b);
              }

              // Symbol name mapping:
              // First look for the native symbol name, then the ogr-...
              // generic name.  
              // If none provided or found then use 0: solid fill
              
              const char *pszName = poBrushStyle->Id(bIsNull);
              if (bIsNull)
                  pszName = NULL;
              c->styles[0].symbol = msOGRGetSymbolId(&(map->symbolset), 
                                                    pszName, NULL);

          }
          else if (poStylePart->GetType() == OGRSTCSymbol)
          {
              OGRStyleSymbol *poSymbolStyle = (OGRStyleSymbol*)poStylePart;

              if (poSymbolStyle->GetRGBFromString(poSymbolStyle->
                                                  Color(bIsNull),r,g,b,t))
              {
                  MS_INIT_COLOR(c->styles[0].color, r, g, b);
              }

              c->styles[0].size = (int)poSymbolStyle->Size(bIsNull);

              // Symbol name mapping:
              // First look for the native symbol name, then the ogr-...
              // generic name, and in last resort try "default-marker" if
              // provided by user.
              const char *pszName = poSymbolStyle->Id(bIsNull);
              if (bIsNull)
                  pszName = NULL;

              c->styles[0].symbol = msOGRGetSymbolId(&(map->symbolset),
                                                    pszName, 
                                                    "default-marker");
          }

          delete poStylePart;

      }

      if (poStyleMgr)
          delete poStyleMgr;

  }

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

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
 * Copyright (c) 2000, 2001, Daniel Morissette, DM Solutions Group Inc
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
 * Revision 1.25  2001/03/12 19:02:46  dan
 * Added query-related stuff in PHP MapScript
 *
 * Revision 1.24  2001/03/12 16:05:07  sdlime
 * Fixed parameters for msLayerGetItems. Added preprocessor wrapper to OGR version of the same function.
 *
 * Revision 1.23  2001/03/11 22:01:32  dan
 * Implemented msOGRLayerGetItems()
 *
 * Revision 1.22  2001/03/05 23:15:46  sdlime
 * Many fixes in mapsde.c, now compiles. Switched shape index to long from int. Layer ...WhichShapes() functions now return MS_DONE if there is no overlap.
 *
 * Revision 1.21  2001/03/03 01:01:55  dan
 * Set shape->index in GetNextShape() for queries to work.
 *
 * Revision 1.20  2001/03/02 05:42:43  sdlime
 * Checking in Dan's new configure stuff. Updated code to essentially be GD versionless. Needs lot's of testing.
 *
 * Revision 1.19  2001/03/02 05:05:19  dan
 * Fixed problem reading LOCAL_CS SpatialRef. Handle as if no PROJECTION was set
 *
 * Revision 1.18  2001/03/02 03:48:08  frank
 * fixed to ensure it compiles with gdal and without ogr
 *
 * Revision 1.17  2001/03/01 22:54:07  dan
 * Test for geographic proj using IsGeographic() before conversion to PROJ4
 *
 * Revision 1.16  2001/03/01 22:37:14  dan
 * Added support for PROJECTION AUTO to use projection from dataset
 *
 * Revision 1.15  2001/02/28 04:56:39  dan
 * Support for OGR Label text, angle, size, implemented msOGRLayerGetShape
 * and reworked msLayerNextShape, added layer FILTER support, annotation, etc.
 *
 * Revision 1.14  2001/02/25 23:46:17  sdlime
 * Fully implemented FILTER option for shapefiles. Changed parameters for 
 * ...NextShapes and ...GetShapes functions.
 *
 * Revision 1.13  2001/02/23 21:58:00  dan
 * PHP MapScript working with new 3.5 stuff, but query stuff is disabled
 *
 * Revision 1.12  2001/02/20 20:48:44  dan
 * OGR support working for classified maps
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

typedef struct ms_ogr_layer_info_t
{
    char        *pszFname;
    int         nLayerIndex;
    OGRDataSource *poDS;
    OGRLayer    *poLayer;
} msOGRLayerInfo;


/* ==================================================================
 * Geometry conversion functions
 * ================================================================== */

/**********************************************************************
 *                     ogrPointsAddPoint()
 **********************************************************************/
static void ogrPointsAddPoint(lineObj *line, double dX, double dY)
{
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
  if (poGeom->getGeometryType() == wkbPoint)
  {
      numpoints = 1;
  }
  else if (poGeom->getGeometryType() == wkbLineString)
  {
      numpoints = ((OGRLineString*)poGeom)->getNumPoints();
  }
  else if (poGeom->getGeometryType() == wkbPolygon)
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
   
  if (poGeom->getGeometryType() == wkbPoint)
  {
      OGRPoint *poPoint = (OGRPoint *)poGeom;
      ogrPointsAddPoint(&line, poPoint->getX(), poPoint->getY());
  }
  else if (poGeom->getGeometryType() == wkbLineString)
  {
      OGRLineString *poLine = (OGRLineString *)poGeom;
      for(i=0; i<numpoints; i++)
          ogrPointsAddPoint(&line, poLine->getX(i), poLine->getY(i));
  }
  else if (poGeom->getGeometryType() == wkbPolygon)
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
                  ogrPointsAddPoint(&line, poRing->getX(i), poRing->getY(i));
          }
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
  if (poGeom->getGeometryType() == wkbPolygon )
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
           poGeom->getGeometryType() == wkbMultiPolygon )
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
 * OGRPoint
 * ------------------------------------------------------------------ */
  else if (poGeom->getGeometryType() == wkbPoint)
  {
      /* Hummmm a point when we're drawing lines/polygons... just drop it! */
  }
/* ------------------------------------------------------------------
 * OGRLinearRing/OGRLineString ... both are of type wkbLineString
 * ------------------------------------------------------------------ */
  else if (poGeom->getGeometryType() == wkbLineString)
  {
      OGRLineString *poLine = (OGRLineString *)poGeom;
      int       j, numpoints;
      lineObj   line={0,NULL};

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
          line.point[j].x = poLine->getX(j); 
          line.point[j].y = poLine->getY(j);
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
                 (char*)CPLSPrintf("OGRGeometry type `%s' not supported yet.", 
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
 *      LINE / POLYLINE layer
 * ------------------------------------------------------------------ */
    case MS_LAYER_LINE:
    case MS_LAYER_POLYLINE:
      if(ogrGeomLine(poGeom, outshp, MS_FALSE) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      break;
/* ------------------------------------------------------------------
 *      POLYGON layer
 * ------------------------------------------------------------------ */
    case MS_LAYER_POLYGON:
      if(ogrGeomLine(poGeom, outshp, MS_TRUE) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      break;
/* ------------------------------------------------------------------
 *      MS_ANNOTATION layer - return real feature type
 * ------------------------------------------------------------------ */
    case MS_LAYER_ANNOTATION:
    case MS_LAYER_QUERY:
      switch(poGeom->getGeometryType())
      {
        case wkbPoint:
        case wkbMultiPoint:
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
 *                     msOGRGetValueList()
 *
 * Load selected item (i.e. field) values into a char array
 *
 * Some special attribute names are used to return some OGRFeature params
 * like for instance stuff encoded in the OGRStyleString.
 * For now the following pseudo-attribute names are supported:
 *  "OGR:TextString"  OGRFeatureStyle's text string if present
 *  "OGR:TextAngle"   OGRFeatureStyle's text angle, or 0 if not set
 **********************************************************************/
static char **msOGRGetValueList(OGRFeature *poFeature,
                                char **items, int **itemindexes, int numitems)
{
  char **values;
  int i;

  if(numitems == 0 || poFeature->GetFieldCount() == 0) 
      return(NULL);

  if(!(*itemindexes)) 
  { // build the list
    (*itemindexes) = (int *)malloc(sizeof(int)*numitems);
    if(!(*itemindexes)) 
    {
      msSetError(MS_MEMERR, NULL, "msOGRGetValueList()");
      return(NULL);
    }

    for(i=0;i<numitems;i++) 
    {
      // Special case for handling text string and angle coming from
      // OGR style strings.  We use special attribute names.
      if (EQUAL(items[i], MSOGR_LABELTEXTNAME))
          (*itemindexes)[i] = MSOGR_LABELTEXTINDEX;
      else if (EQUAL(items[i], MSOGR_LABELANGLENAME))
          (*itemindexes)[i] = MSOGR_LABELANGLEINDEX;
      else if (EQUAL(items[i], MSOGR_LABELSIZENAME))
          (*itemindexes)[i] = MSOGR_LABELSIZEINDEX;
      else
          (*itemindexes)[i] = poFeature->GetFieldIndex(items[i]);
      if((*itemindexes)[i] == -1) 
      {
          msSetError(MS_OGRERR, 
                     (char*)CPLSPrintf("Invalid Field name: %s", items[i]), 
                     "msOGRGetValueList()");
          return(NULL);
      }
    }
  }

  if((values = (char **)malloc(sizeof(char *)*numitems)) == NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRGetValueList()");
    return(NULL);
  }

  OGRStyleMgr *poStyleMgr = NULL;
  OGRStyleLabel *poLabelStyle = NULL;

  for(i=0;i<numitems;i++)
  {
    if ((*itemindexes)[i] >= 0)
    {
        // Extract regular attributes
        values[i] = strdup(poFeature->GetFieldAsString((*itemindexes)[i]));
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
        if (poLabelStyle && (*itemindexes)[i] == MSOGR_LABELTEXTINDEX)
        {
            values[i] = strdup(poLabelStyle->TextString(bDefault));
            //msDebug(MSOGR_LABELTEXTNAME " = \"%s\"\n", values[i]);
        }
        else if (poLabelStyle && (*itemindexes)[i] == MSOGR_LABELANGLEINDEX)
        {
            values[i] = strdup(poLabelStyle->GetParamStr(OGRSTLabelAngle,
                                                         bDefault));
            //msDebug(MSOGR_LABELANGLENAME " = \"%s\"\n", values[i]);
        }
        else if (poLabelStyle && (*itemindexes)[i] == MSOGR_LABELSIZEINDEX)
        {
            values[i] = strdup(poLabelStyle->GetParamStr(OGRSTLabelSize,
                                                         bDefault));
            //msDebug(MSOGR_LABELSIZENAME " = \"%s\"\n", values[i]);
        }
        else
        {
          msSetError(MS_OGRERR,"Invalid field index!?!","msOGRGetValueList()");
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


/**********************************************************************
 *                     msOGRGetValues()
 *
 * Load item (i.e. field) values into a char array
 **********************************************************************/
static char **msOGRGetValues(OGRFeature *poFeature)
{
  char **values;
  int i, nFields;

  if((nFields = poFeature->GetFieldCount()) == 0)
  {
    return(NULL);
  }

  if((values = (char **)malloc(sizeof(char *)*nFields)) == NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRGetValues()");
    return(NULL);
  }

  for(i=0;i<nFields;i++)
    values[i] = strdup(poFeature->GetFieldAsString(i));

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
int msOGRSpatialRef2ProjectionObj(OGRSpatialReference *poSRS,
                                  projectionObj *proj)
{
  if (proj->numargs == 0  || !EQUAL(proj->projargs[0], "auto"))
      return MS_SUCCESS;  // Nothing to do!

#ifdef USE_PROJ
  int i;

  // First flush the "auto" name from the projargs[]... 
  if(proj->proj) pj_free(proj->proj);
  proj->proj = NULL;
  msFreeCharArray(proj->projargs, proj->numargs);  
  proj->projargs = NULL;
  proj->numargs = 0;

  if (poSRS == NULL || poSRS->IsLocal())
  {
      // Dataset had no set projection or is NonEarth (LOCAL_CS)... 
      // Nothing else to do. Leave proj empty and no reprojection will happen!
      return MS_SUCCESS;  
  }

  // +proj=longlat is a special case because versions of PROJ4 prior to 4.4.2
  // didn't accept it.  Just catch that case for now and when we switch to
  // 4.4.2 to use pj_transform() then we can get rid of that special case.
  if (poSRS->IsGeographic())
  {
      // Do the same as what loadProjection() does for GEOGRAPHIC
      proj->projargs = split("GEOGRAPHIC", ' ', &proj->numargs);
      proj->proj = NULL;
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

  // Set new proj params in projargs[] array
  // Since OGR and MapServer use different memory management funcs
  // we have to convert CPL stringlist to mapserver stringlist
  //
  char **papszArgs = CSLTokenizeStringComplex(pszProj,"+ ",TRUE,FALSE);

  proj->numargs = CSLCount(papszArgs);
  proj->projargs =(char**)malloc(proj->numargs*sizeof(char*));
  if (!proj->projargs) {
      msSetError(MS_MEMERR, NULL, "msOGRSpatialRef2ProjectionObj()");
      return(MS_FAILURE);
  }

  for(i=0; i< proj->numargs; i++)
      proj->projargs[i] = strdup(papszArgs[i]);

  if( !(proj->proj = pj_init(proj->numargs, proj->projargs))) 
  {
      msSetError(MS_PROJERR,(char*) CPLSPrintf("pj_init( %s ) failed: %s. ",
                                               pszProj, pj_strerrno(pj_errno)),
                 "msOGRSpatialRef2ProjectionObj()");
      CPLFree(pszProj);
      CSLDestroy(papszArgs);
      return(MS_FAILURE);
  }

  CPLFree(pszProj);
  CSLDestroy(papszArgs);

#endif

  return MS_SUCCESS;
}
#endif // defined(USE_OGR) || defined(USE_GDAL)


/* ==================================================================
 * Here comes the REAL stuff... the functions below are called by maplayer.c
 * ================================================================== */

/**********************************************************************
 *                     msOGRLayerOpen()
 *
 * Open OGR data source for the specified map layer.
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
int msOGRLayerOpen(layerObj *layer, char *shapepath) 
{
#ifdef USE_OGR

  if (layer->ogrlayerinfo != NULL)
  {
      return MS_SUCCESS;  // Nothing to do... layer is already opened
  }

/* ------------------------------------------------------------------
 * Register OGR Drivers, only once per execution
 * ------------------------------------------------------------------ */
  static int bDriversRegistered = MS_FALSE;
  if (!bDriversRegistered)
      OGRRegisterAll();
  bDriversRegistered = MS_TRUE;

/* ------------------------------------------------------------------
 * Attempt to open OGR dataset
 * ------------------------------------------------------------------ */
  int   nLayerIndex = 0;
  char  **params;
  int   numparams;
  OGRDataSource *poDS;
  OGRLayer    *poLayer;

  if(layer->connection==NULL || 
     (params = split(layer->connection, ',', &numparams))==NULL || 
     numparams < 1) 
  {
      msSetError(MS_OGRERR, "Error parsing OGR connection information.", 
                 "msOGRLayerOpen()");
      return(MS_FAILURE);
  }

  msDebug("msOGRLayerOpen(%s)...\n", layer->connection);

  poDS = OGRSFDriverRegistrar::Open( params[0] );
  if( poDS == NULL )
  {
      msSetError(MS_OGRERR, 
                 (char*)CPLSPrintf("Open failed for OGR connection `%s'.  "
                                   "File not found or unsupported format.", 
                                   layer->connection),
                 "msOGRLayerOpen()");
      return(MS_FAILURE);
  }

  if(numparams > 1) 
      nLayerIndex = atoi(params[1]);

  poLayer = poDS->GetLayer(nLayerIndex);
  if (poLayer == NULL)
  {
      msSetError(MS_OGRERR, 
             (char*)CPLSPrintf("GetLayer(%d) failed for OGR connection `%s'.",
                               nLayerIndex, layer->connection),
                 "msOGRLayerOpen()");
      delete poDS;
      return(MS_FAILURE);
  }

/* ------------------------------------------------------------------
 * If projection was "auto" then set proj to the dataset's projection
 * ------------------------------------------------------------------ */
#ifdef USE_PROJ
  if (layer->projection.numargs > 0 && 
      EQUAL(layer->projection.projargs[0], "auto"))
  {
      OGRSpatialReference *poSRS = poLayer->GetSpatialRef();

      if (msOGRSpatialRef2ProjectionObj(poSRS,
                                        &(layer->projection)) != MS_SUCCESS)
      {
          msSetError(MS_OGRERR, 
                  (char*)CPLSPrintf("%s  "
                                    "PROJECTION AUTO cannot be used for this "
                                    "OGR connection (`%s').",
                                    ms_error.message, layer->connection),
                     "msOGRLayerOpen()");
          delete poDS;
          return(MS_FAILURE);
      }
  }
#endif

/* ------------------------------------------------------------------
 * OK... open succeded... alloc and fill msOGRLayerInfo inside layer obj
 * ------------------------------------------------------------------ */
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)CPLCalloc(1,sizeof(msOGRLayerInfo));
  layer->ogrlayerinfo = psInfo;

  psInfo->pszFname = CPLStrdup(params[0]);
  psInfo->nLayerIndex = nLayerIndex;
  psInfo->poDS = poDS;
  psInfo->poLayer = poLayer;

  // Cleanup and exit;
  msFreeCharArray(params, numparams);

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
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;

  if (psInfo)
  {
    msDebug("msOGRLayerClose(%s).\n", layer->connection);

    CPLFree(psInfo->pszFname);

    /* Destroying poDS automatically closes files, destroys the layer, etc. */
    delete psInfo->poDS;

    CPLFree(psInfo);
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
int msOGRLayerWhichShapes(layerObj *layer, char *shapepath, rectObj rect) 
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerWhichShapes()");
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

/* ------------------------------------------------------------------
 * Reset current feature pointer
 * ------------------------------------------------------------------ */
  psInfo->poLayer->ResetReading();

  return MS_SUCCESS;

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
 * Load item (i.e. field) names in a char array.
 **********************************************************************/
int msOGRLayerGetItems(layerObj *layer, char ***items, int *numitems)
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;
  int   i;
  OGRFeatureDefn *poDefn;

  if((poDefn = psInfo->poLayer->GetLayerDefn()) == NULL ||
     (*numitems = poDefn->GetFieldCount()) == 0) 
  {
    msSetError(MS_OGRERR, "Layer contains no fields.", "msOGRLayerGetItems()");
    return(MS_FAILURE);
  }

  if((*items = (char **)malloc(sizeof(char *)*(*numitems))) == NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRLayerGetItems()");
    return(MS_FAILURE);
  }

  for(i=0;i<*numitems;i++) 
  {
      OGRFieldDefn *poField = poDefn->GetFieldDefn(i);
      (*items)[i] = strdup(poField->GetNameRef());
  }

  return(MS_SUCCESS);
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
 *                     msOGRLayerNextShape()
 *
 * Returns shape sequentially from OGR data source.
 * msOGRLayerWhichShape() must have been called first.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerNextShape(layerObj *layer, char *shapepath, shapeObj *shape) 
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;
  OGRFeature *poFeature;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerNextShape()");
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
      if( (poFeature = psInfo->poLayer->GetNextFeature()) == NULL )
      {
          return MS_DONE;  // No more features to read
      }

      if(layer->numitems > 0) 
      {
          shape->values = msOGRGetValueList(poFeature, layer->items, 
                                                &(layer->itemindexes), 
                                                layer->numitems);
          shape->numvalues = layer->numitems;
          if(!shape->values) return(MS_FAILURE);
      }

      if (msEvalExpression(&(layer->filter), layer->filteritemindex, 
                           shape->values, layer->numitems) == MS_TRUE)
      {
          // Feature matched filter expression... process geometry
          // shape->type will be set if geom is compatible with layer type
          if (ogrConvertGeometry(poFeature->GetGeometryRef(), shape,
                                 layer->type) == MS_SUCCESS)
          {
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

  delete poFeature;

  return MS_SUCCESS;

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
int msOGRLayerGetShape(layerObj *layer, char *shapepath, shapeObj *shape, 
                       int tile, long record, int allitems) 
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;
  OGRFeature *poFeature;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerNextShape()");
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
  if (allitems == MS_TRUE)
  {
      // fill the items layer variable if not already filled
      // __TODO__ This will eventually go away when allitems is replaced
      // by the use of msOGRLayerGetItems() in maplayer.c

      if(!layer->items &&
         msOGRLayerGetItems(layer, &layer->items, 
                            &layer->numitems) != MS_SUCCESS)
      {
          return(MS_FAILURE);
      }
      shape->values = msOGRGetValues(poFeature);
      shape->numvalues = layer->numitems;
      if(!shape->values) return (MS_FAILURE);
  }
  else if(layer->numitems > 0) 
  {
      shape->values = msOGRGetValueList(poFeature, layer->items, 
                                            &(layer->itemindexes), 
                                            layer->numitems);
      shape->numvalues = layer->numitems;
      if(!shape->values) return(MS_FAILURE);
  }   

  shape->index = poFeature->GetFID();

  delete poFeature;

  return MS_SUCCESS;
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetShape()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}


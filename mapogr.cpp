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

#ifdef USE_OGR

#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "cpl_string.h"
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

  outshp->type = MS_POINT;

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

      if (outshp->type == MS_NULL)
          outshp->type = MS_POLYGON;

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

      if (outshp->type == MS_NULL)
          outshp->type = MS_LINE;

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
 * shape->type = MS_NULL
 **********************************************************************/
static int ogrConvertGeometry(OGRGeometry *poGeom, shapeObj *outshp,
                              enum MS_FEATURE_TYPE layertype) 
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
 *      MS_POINT layer - Any geometry can be converted to point/multipoint
 * ------------------------------------------------------------------ */
    case MS_POINT:
      if(ogrGeomPoints(poGeom, outshp) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      break;
/* ------------------------------------------------------------------
 *      MS_LINE / MS_POLYLINE layer
 * ------------------------------------------------------------------ */
    case MS_LINE:
    case MS_POLYLINE:
      if(ogrGeomLine(poGeom, outshp, MS_FALSE) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      break;
/* ------------------------------------------------------------------
 *      MS_POLYGON layer
 * ------------------------------------------------------------------ */
    case MS_POLYGON:
      if(ogrGeomLine(poGeom, outshp, MS_TRUE) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      break;
/* ------------------------------------------------------------------
 *      MS_ANNOTATION layer - return real feature type
 * ------------------------------------------------------------------ */
    case MS_ANNOTATION:
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
 *                     msOGRGetItems()
 *
 * Load item (i.e. field) names in a char array
 **********************************************************************/
static char **msOGRGetItems(OGRLayer *poLayer)
{
  char **items;
  int i, nFields;
  OGRFeatureDefn *poDefn;

  if((poDefn = poLayer->GetLayerDefn()) == NULL ||
     (nFields = poDefn->GetFieldCount()) == 0) 
  {
    msSetError(MS_OGRERR, "Layer contains no fields.", "msOGRGetItems()");
    return(NULL);
  }

  if((items = (char **)malloc(sizeof(char *)*nFields)) == NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRGetItems()");
    return(NULL);
  }

  for(i=0;i<nFields;i++) 
  {
      OGRFieldDefn *poField = poDefn->GetFieldDefn(i);
      items[i] = strdup(poField->GetNameRef());
  }

  return(items);
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
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerWhichShapes(layerObj *layer, char *shapepath, 
                          rectObj rect, projectionObj *proj) 
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
 * __TODO__: for now we assume rect is in same proj as data
 * __TODO__: Do we need to handle expression filter at this point???
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
  shape->type = MS_NULL;

  while (shape->type == MS_NULL)
  {
      if( (poFeature = psInfo->poLayer->GetNextFeature()) == NULL )
      {
          return MS_DONE;  // No more features to read
      }

      if(layer->numitems > 0) 
      {
          shape->attributes = msOGRGetValueList(poFeature, layer->items, 
                                                &(layer->itemindexes), 
                                                layer->numitems);
          if(!shape->attributes) return(MS_FAILURE);
      }

      if (msEvalExpression(&(layer->filter), layer->filteritemindex, 
                           shape->attributes, layer->numitems) == MS_TRUE)
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
      shape->type = MS_NULL;
  }

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
                       int tile, int record, int allitems) 
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
  shape->type = MS_NULL;

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
  
  if (shape->type == MS_NULL)
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
      if(!layer->items)
      {
      	// fill the items layer variable if not already filled
          layer->numitems = poFeature->GetFieldCount();
          layer->items = msOGRGetItems(psInfo->poLayer);
          if(!layer->items)
              return(MS_FAILURE);
      }
      shape->attributes = msOGRGetValues(poFeature);
      if(!shape->attributes)
          return(MS_FAILURE);
  }
  else if(layer->numitems > 0) 
  {
      shape->attributes = msOGRGetValueList(poFeature, layer->items, 
                                            &(layer->itemindexes), 
                                            layer->numitems);
      if(!shape->attributes) return(MS_FAILURE);
  }   

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


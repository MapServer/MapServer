/**********************************************************************
 * $Id$
 *
 * Name:     mapogr.cpp
 * Project:  MapServer
 * Language: C++
 * Purpose:  OGR Link
 * Author:   Daniel Morissette, danmo@videotron.ca
 *           Based on mapsde.c from Steve Lime
 *
 **********************************************************************
 * Copyright (c) 2000, Daniel Morissette
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
 *
 * $Log$
 * Revision 1.9  2000/11/01 16:55:38  sdlime
 * Changed overlaysize handling (when scaled) to be relative to the main class size.
 *
 * Revision 1.8  2000/11/01 04:23:32  sdlime
 * Changed insertFeatureList to make a copy of the input shape rather than simply pointing to it. Added msCopyShape function that might be useful in other places.
 *
 * Revision 1.7  2000/09/18 19:45:25  dan
 * Added support of overlaying symbols
 *
 * Revision 1.6  2000/09/17 17:35:21  dan
 * Fixed label point generation for polygons
 *
 * Revision 1.5  2000/09/17 03:10:19  sdlime
 * Fixed a few more things. Real close, just needs some testing.
 *
 * Revision 1.4  2000/09/11 14:28:29  dan
 * Added extern "C" in MS headers with external functions (was in mapogr.cpp)
 *
 * Revision 1.3  2000/09/06 18:40:45  dan
 * getClassIndex() changed name to msGetClassIndex()
 *
 * Revision 1.2  2000/08/28 02:00:25  dan
 * Fixed compile problem when OGR not enabled
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

/**********************************************************************
 *                     ogrTransformPointsAddPoint()
 **********************************************************************/
static void ogrTransformPointsAddPoint(lineObj *line, rectObj *extent, 
                                       double cellsize, double dX, double dY)
{
    if(dX < extent->minx || dX > extent->maxx ||
       dY < extent->miny || dY > extent->maxy  )
        return;
    
    line->point[line->numpoints].x = MS_NINT((dX - extent->minx)/cellsize);
    line->point[line->numpoints].y = MS_NINT((extent->maxy - dY)/cellsize);
    line->numpoints++;
}

/**********************************************************************
 *                     ogrTransformGeomPoints()
 **********************************************************************/
static int ogrTransformGeomPoints(rectObj *extent, double cellsize, 
                                   OGRGeometry *poGeom, shapeObj *outshp) 
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
                 "ogrTransformGeomPoints()");
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
                 "ogrTransformAddPoint()");
      return(-1);
  }
   
  if (poGeom->getGeometryType() == wkbPoint)
  {
      OGRPoint *poPoint = (OGRPoint *)poGeom;
      ogrTransformPointsAddPoint(&line, extent, cellsize,
                                 poPoint->getX(), poPoint->getY());
  }
  else if (poGeom->getGeometryType() == wkbLineString)
  {
      OGRLineString *poLine = (OGRLineString *)poGeom;
      for(i=0; i<numpoints; i++)
          ogrTransformPointsAddPoint(&line, extent, cellsize,
                                     poLine->getX(i), poLine->getY(i));
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
                  ogrTransformPointsAddPoint(&line, extent, cellsize,
                                             poRing->getX(i), poRing->getY(i));
          }
      }
  }

  msAddLine(outshp, &line);
  free(line.point);

  return(0);
}


/**********************************************************************
 *                     ogrTransformGeomLine()
 *
 * Recursively convert any OGRGeometry into a shapeObj.  Each part becomes
 * a line in the overall shapeObj.
 **********************************************************************/
static int ogrTransformGeomLine(rectObj *extent, double cellsize, 
                                OGRGeometry *poGeom, shapeObj *outshp,
                                int bCloseRings) 
{
  if (poGeom == NULL)
      return 0;

/* ------------------------------------------------------------------
 * Use recursive calls for complex geometries
 * ------------------------------------------------------------------ */
  if (poGeom->getGeometryType() == wkbPolygon)
  {
      OGRPolygon *poPoly = (OGRPolygon *)poGeom;
      OGRLinearRing *poRing;

      for (int iRing=-1; iRing < poPoly->getNumInteriorRings(); iRing++)
      {
          if (iRing == -1)
              poRing = poPoly->getExteriorRing();
          else
              poRing = poPoly->getInteriorRing(iRing);

          if (ogrTransformGeomLine(extent, cellsize,
                                   poRing, outshp, bCloseRings) == -1)
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
          if (ogrTransformGeomLine(extent, cellsize,
                                   poColl->getGeometryRef(iGeom),
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
      int       j, k, numpoints;
      lineObj   line={0,NULL};

      if ((numpoints = poLine->getNumPoints()) < 2)
          return 0;

      line.numpoints = 0;
      line.point = (pointObj *)malloc(sizeof(pointObj)*(numpoints+1));
      if(!line.point) 
      {
          msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", 
                     "ogrTransformGeomLine");
          return(-1);
      }

      line.point[0].x = MS_NINT((poLine->getX(0) - extent->minx)/cellsize);
      line.point[0].y = MS_NINT((extent->maxy - poLine->getY(0))/cellsize);

      for(j=1, k=1; j<numpoints; j++)
      {
          line.point[k].x = MS_NINT((poLine->getX(j)-extent->minx)/cellsize); 
          line.point[k].y = MS_NINT((extent->maxy-poLine->getY(j))/cellsize);
      
          if(k == 1) 
          {
              if((line.point[0].x != line.point[1].x) || 
                 (line.point[0].y != line.point[1].y))
                  k++;
          } 
          else 
          {
              if((line.point[k-1].x != line.point[k].x) || 
                 (line.point[k-1].y != line.point[k].y)) 
              {
                  if(((line.point[k-2].y - line.point[k-1].y)*
                      (line.point[k-1].x - line.point[k].x)) == 
                           ((line.point[k-2].x - line.point[k-1].x)*
                            (line.point[k-1].y - line.point[k].y))) 
                  {
                      line.point[k-1].x = line.point[k].x;
                      line.point[k-1].y = line.point[k].y;	
                  } 
                  else 
                  {
                      k++;
                  }
              }
          }
      }
      line.numpoints = k; /* save actual number kept */

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
                 "ogrTransformGeomLine()");
      return(-1);
  }

  return(0);
}


#endif  /* USE_OGR */

/**********************************************************************
 *                     msDrawOGRLayer()
 **********************************************************************/
int msDrawOGRLayer(mapObj *map, layerObj *layer, gdImagePtr img) 
{
#ifdef USE_OGR

  int i,j;

  double scalefactor=1;
  double angle, length;

  char **params;
  int numparams;

  short annotate=MS_TRUE;

  shapeObj transformedshape={0,NULL,{-1,-1,-1,-1},MS_NULL};
  pointObj annopnt, *pnt;

  featureListNodeObjPtr shpcache=NULL, current=NULL;           

/* ------------------------------------------------------------------
 * Register OGR Drivers, only once per execution
 * ------------------------------------------------------------------ */
  static int bDriversRegistered = MS_FALSE;
  if (!bDriversRegistered)
      OGRRegisterAll();
  bDriversRegistered = MS_TRUE;

/* ------------------------------------------------------------------
 * Check layer status, min/max scale, etc.
 * ------------------------------------------------------------------ */
  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT))
    return(0);

  if(map->scale > 0) {
    if((layer->maxscale > 0) && (map->scale > layer->maxscale))
      return(0);
    if((layer->minscale > 0) && (map->scale <= layer->minscale))
      return(0);
    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
      annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
      annotate = MS_FALSE;
  }
  
  /* ------------------------------------------------------------------
   * apply scaling to symbols and fonts
   * ------------------------------------------------------------------ */
  if(layer->symbolscale > 0) scalefactor = layer->symbolscale/map->scale;

  for(i=0; i<layer->numclasses; i++) {
    layer->_class[i].sizescaled = MS_NINT(layer->_class[i].size * scalefactor);
    layer->_class[i].sizescaled = MS_MAX(layer->_class[i].sizescaled, layer->_class[i].minsize);
    layer->_class[i].sizescaled = MS_MIN(layer->_class[i].sizescaled, layer->_class[i].maxsize);
    layer->class[i].overlaysizescaled = layer->class[i].sizescaled - (layer->class[i].size - layer->class[i].overlaysize);
    // layer->_class[i].overlaysizescaled = MS_NINT(layer->_class[i].overlaysize * scalefactor);
    layer->_class[i].overlaysizescaled = MS_MAX(layer->_class[i].overlaysizescaled, layer->_class[i].overlayminsize);
    layer->_class[i].overlaysizescaled = MS_MIN(layer->_class[i].overlaysizescaled, layer->_class[i].overlaymaxsize);
#ifdef USE_TTF
    if(layer->_class[i].label.type == MS_TRUETYPE) { 
      layer->_class[i].label.sizescaled = MS_NINT(layer->_class[i].label.size * scalefactor);
      layer->_class[i].label.sizescaled = MS_MAX(layer->_class[i].label.sizescaled, layer->_class[i].label.minsize);
      layer->_class[i].label.sizescaled = MS_MIN(layer->_class[i].label.sizescaled, layer->_class[i].label.maxsize);
    }
#endif
  }


/* ------------------------------------------------------------------
 * Attempt to open OGR dataset
 *
 * An OGR connection string is:   <dataset_filename>[,<layer_index>]
 *  <dataset_filename>   is file format specific
 *  <layer_index>        (optional) is the OGR layer index
 *                       default is 0, the first layer.
 *
 * One can use the "ogrinfo" program to find out the layer indices in a dataset
 * ------------------------------------------------------------------ */
  OGRDataSource *poDS;
  OGRLayer      *poLayer = NULL;
  int           nLayerIndex = 0;

  params = split(layer->connection, ',', &numparams);
  if(!params || numparams < 1) 
  {
      msSetError(MS_MEMERR, "Error spliting OGR connection information.", 
                 "msDrawOGRLayer()");
      return(-1);
  }

  poDS = OGRSFDriverRegistrar::Open( params[0] );
  if( poDS == NULL )
  {
      msSetError(MS_OGRERR, 
                 (char*)CPLSPrintf("Open failed for OGR connection `%s'.  "
                                   "File not found or unsupported format.", 
                                   layer->connection),
                 "msDrawOGRLayer()");
      return(-1);
  }

  if(numparams > 1) 
      nLayerIndex = atoi(params[1]);

  poLayer = poDS->GetLayer(nLayerIndex);
  if (poLayer == NULL)
  {
      msSetError(MS_OGRERR, 
             (char*)CPLSPrintf("GetLayer(%d) failed for OGR connection `%s'.",
                               nLayerIndex, layer->connection),
                 "msDrawOGRLayer()");
      return(-1);
  }

  msFreeCharArray(params, numparams);

/* ------------------------------------------------------------------
 * Set Spatial filter... this may result in no features being returned
 * if layer does not overlap current view.
 * ------------------------------------------------------------------ */
  OGRLinearRing oSpatialFilter;

  rectObj rect;
  rect.minx = map->extent.minx - 2*map->cellsize; 
  rect.miny = map->extent.miny - 2*map->cellsize;
  rect.maxx = map->extent.maxx + 2*map->cellsize;
  rect.maxy = map->extent.maxy + 2*map->cellsize;

  oSpatialFilter.setNumPoints(5);
  oSpatialFilter.setPoint(0, rect.minx, rect.miny);
  oSpatialFilter.setPoint(1, rect.maxx, rect.miny);
  oSpatialFilter.setPoint(2, rect.maxx, rect.maxy);
  oSpatialFilter.setPoint(3, rect.minx, rect.maxy);
  oSpatialFilter.setPoint(4, rect.minx, rect.miny);

  poLayer->SetSpatialFilter( &oSpatialFilter );


/* ------------------------------------------------------------------
 * Get index of some attribute fields
 * ------------------------------------------------------------------ */
  OGRFeatureDefn *poDefn = poLayer->GetLayerDefn();

  int nClassItem = -1;
  int nLabelItem = -1;

  if (layer->classitem)
      nClassItem = poDefn->GetFieldIndex(layer->classitem);

  if (layer->labelitem && annotate)
      nLabelItem = poDefn->GetFieldIndex(layer->labelitem);


/* ------------------------------------------------------------------
 * Retrieve features until EOF
 * ------------------------------------------------------------------ */
  OGRFeature *poFeature;
  int   nClassId;
  char *pszLabel;

  poLayer->ResetReading();
  while( (poFeature = poLayer->GetNextFeature()) != NULL )
  {

/* ------------------------------------------------------------------
 * Establish class to which feature belongs
 * __TODO__ With msGetClassIndex(), logical expressions work only with a
 *   variable "[value]" in the expression and that will be replaced with
 *   the value of the classitem attribute.
 *   We need to support any field name in expressions like with DBF files.
 * ------------------------------------------------------------------ */
      nClassId = 0;
      if (nClassItem != -1)
      {
          const char *pszValue = poFeature->GetFieldAsString(nClassItem);

          if ( (nClassId = msGetClassIndex(layer, (char*)pszValue)) == -1 )
          {
              /* Feature does not belong to any class */
              delete poFeature;
              continue;
          }
      }

/* ------------------------------------------------------------------
 * Annotation
 * __TODO__ We need to handle expressions too ... see shpGetAnnotation()
 * ------------------------------------------------------------------ */
      pszLabel = NULL;
      if (nLabelItem != -1)
          pszLabel = CPLStrdup(poFeature->GetFieldAsString(nLabelItem));
      else if (annotate && layer->_class[nClassId].text.string)
          pszLabel = CPLStrdup(layer->_class[nClassId].text.string);

/* ------------------------------------------------------------------
 * Process geometry according to layer type
 * ------------------------------------------------------------------ */
      switch(layer->type) 
      {
/* ------------------------------------------------------------------
 *      MS_ANNOTATION
 * ------------------------------------------------------------------ */
        case MS_ANNOTATION:
          msSetError(MS_OGRERR, "OGR annotation layers are not yet supported.",
                     "msDrawOGRLayer()");
          return(-1);
          break;
/* ------------------------------------------------------------------
 *      MS_POINT
 * ------------------------------------------------------------------ */
        case MS_POINT:
	  if(ogrTransformGeomPoints(&(map->extent), map->cellsize, 
                                    poFeature->GetGeometryRef(), 
                                    &transformedshape) != -1)
          {
              for(j=0; j<transformedshape.line[0].numpoints; j++) 
              {
                  /* point to the correct point */
                  pnt = &(transformedshape.line[0].point[j]); 

                  msDrawMarkerSymbol(&map->symbolset, img, pnt,
                                     layer->_class[nClassId].symbol, 
                                     layer->_class[nClassId].color, 
                                     layer->_class[nClassId].backgroundcolor,
                                     layer->_class[nClassId].outlinecolor, 
                                     layer->_class[nClassId].sizescaled);
                  if (layer->_class[nClassId].overlaysymbol >= 0)
                      msDrawMarkerSymbol(&map->symbolset, img, pnt,
                                layer->_class[nClassId].overlaysymbol, 
                                layer->_class[nClassId].overlaycolor, 
                                layer->_class[nClassId].overlaybackgroundcolor,
                                layer->_class[nClassId].overlayoutlinecolor, 
                                layer->_class[nClassId].overlaysizescaled);

                  if(pszLabel) 
                  {
                      if(layer->labelcache)
                          msAddLabel(map, layer->index, nClassId, -1, -1, 
                                     *pnt, pszLabel, -1);
                      else
                          msDrawLabel(img, map, *pnt, pszLabel,
                                      &(layer->_class[nClassId].label));
                  }
              }
          }
	  
	  msFreeShape(&transformedshape);
          break;
/* ------------------------------------------------------------------
 *      MS_LINE
 * ------------------------------------------------------------------ */
        case MS_LINE:
	  if(ogrTransformGeomLine(&(map->extent), map->cellsize, 
                                  poFeature->GetGeometryRef(), 
                                  &transformedshape, MS_FALSE) != -1)
          {

              msDrawLineSymbol(&map->symbolset, img, &transformedshape,
                                     layer->_class[nClassId].symbol, 
                                     layer->_class[nClassId].color, 
                                     layer->_class[nClassId].backgroundcolor,
                                     layer->_class[nClassId].outlinecolor, 
                                     layer->_class[nClassId].sizescaled);

              // Linear Overlaying Symbols are cached... see below

              if(pszLabel &&
                 msPolylineLabelPoint(&transformedshape, &annopnt, 
                                 layer->_class[nClassId].label.minfeaturesize, 
                                      &angle, &length) != -1)
              {
                  if(layer->_class[nClassId].label.autoangle)
                      layer->_class[nClassId].label.angle = angle;

                  if(layer->labelcache)
                      msAddLabel(map, layer->index, nClassId, -1, -1, 
                                 annopnt, pszLabel, length);
                  else
                      msDrawLabel(img, map, annopnt, pszLabel,
                                  &(layer->_class[nClassId].label));
              }
          }

	  if(layer->_class[nClassId].overlaysymbol >= 0) // cache shape
          {
              transformedshape.classindex = nClassId;
              if(insertFeatureList(&shpcache, &transformedshape) == NULL) 
                  return(-1);
	  } 

	  msFreeShape(&transformedshape);

          break;
/* ------------------------------------------------------------------
 *      MS_POLYGON / MS_POLYLINE
 * ------------------------------------------------------------------ */
    case MS_POLYLINE:
    case MS_POLYGON:
	  if(ogrTransformGeomLine(&(map->extent), map->cellsize, 
                                  poFeature->GetGeometryRef(), 
                                  &transformedshape, MS_TRUE) != -1)
          {
              int nLabelStatus = -1;

              angle = layer->_class[nClassId].label.angle;
              length = -1;

              if(layer->type == MS_POLYGON)
              {
                  msDrawShadeSymbol(&map->symbolset, img, &transformedshape,
                                   layer->_class[nClassId].symbol, 
                                   layer->_class[nClassId].color, 
                                   layer->_class[nClassId].backgroundcolor,
                                   layer->_class[nClassId].outlinecolor, 
                                   layer->_class[nClassId].sizescaled);
                  if (layer->_class[nClassId].overlaysymbol >= 0)
                      msDrawShadeSymbol(&map->symbolset, img, 
                                &transformedshape,
                                layer->_class[nClassId].overlaysymbol, 
                                layer->_class[nClassId].overlaycolor, 
                                layer->_class[nClassId].overlaybackgroundcolor,
                                layer->_class[nClassId].overlayoutlinecolor,
                                layer->_class[nClassId].overlaysizescaled);
                  if(pszLabel)
                      nLabelStatus = msPolygonLabelPoint(&transformedshape, 
                                                          &annopnt, 
                                 layer->_class[nClassId].label.minfeaturesize);
              }
              else
              {
                  msDrawLineSymbol(&map->symbolset, img, &transformedshape,
                                   layer->_class[nClassId].symbol, 
                                   layer->_class[nClassId].color, 
                                   layer->_class[nClassId].backgroundcolor,
                                   layer->_class[nClassId].outlinecolor, 
                                   layer->_class[nClassId].sizescaled);

                  // Linear Overlaying Symbols are cached... see below

                  if(pszLabel)
                      nLabelStatus = msPolylineLabelPoint(&transformedshape, 
                                                          &annopnt, 
                                 layer->_class[nClassId].label.minfeaturesize, 
                                                          &angle, &length);
              }

              if(nLabelStatus != -1)
              {
                  if(layer->_class[nClassId].label.autoangle)
                      layer->_class[nClassId].label.angle = angle;

                  if(layer->labelcache)
                      msAddLabel(map, layer->index, nClassId, -1, -1, 
                                 annopnt, pszLabel, length);
                  else
                      msDrawLabel(img, map, annopnt, pszLabel,
                                  &(layer->_class[nClassId].label));
              }
          }

	  if(layer->type == MS_POLYLINE &&
             layer->_class[nClassId].overlaysymbol >= 0) // cache shape
          {
              transformedshape.classindex = nClassId;
              if(insertFeatureList(&shpcache, &transformedshape) == NULL) 
                  return(-1);
	  } 

	  msFreeShape(&transformedshape);
          break;
    default:
      msSetError(MS_MISCERR, "Unknown or unsupported layer type.", 
                 "msDrawOGRLayer()");
      return(-1);
      } /* switch layer->type */

      CPLFree(pszLabel);
      delete poFeature;

  } /* while GetNextFeature() */

/* ------------------------------------------------------------------
 * Display overlay symbols, LINE/POLYLINE only
 * ------------------------------------------------------------------ */
  if(shpcache) 
  {
      for(current=shpcache; current; current=current->next) 
      {
	  nClassId = current->shape.classindex;
          msDrawLineSymbol(&map->symbolset, img, &current->shape,
                           layer->_class[nClassId].overlaysymbol,
                           layer->_class[nClassId].overlaycolor,
                           layer->_class[nClassId].overlaybackgroundcolor,
                           layer->_class[nClassId].overlayoutlinecolor,
                           layer->_class[nClassId].overlaysizescaled);
      }
      freeFeatureList(shpcache);
  }

/* ------------------------------------------------------------------
 * OK, we're done ... cleanup
 * ------------------------------------------------------------------ */
  delete poDS;  /* This automatically closes files, destroys the layer, etc. */

  return(0);

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", "msDrawOGRLayer()");
  return(-1);

#endif /* USE_OGR */
}


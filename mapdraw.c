/**********************************************************************
 * $Id$
 */


#include "map.h"

static double inchesPerUnit[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};
/*
 * Generic function to render the map file.
 * The type of the image created is based on the
 * imagetype parameter in the map file.
*/

imageObj *msDrawMap(mapObj *map)
{
    int i;
    layerObj *lp=NULL;
    int status;
    imageObj *image = NULL;
       
    if(map->width == -1 || map->height == -1) {
        msSetError(MS_MISCERR, "Image dimensions not specified.", "msDrawMap()");
        return(NULL);
    }
 
    switch (map->imagetype)
    {
        case (MS_GIF):
        case (MS_PNG):
        case (MS_JPEG):
        case (MS_WBMP):
            image = msImageCreateGD(map->width, map->height, map->imagetype,
                                    map->web.imagepath, map->web.imageurl);
            break;

#ifdef USE_MING_FLASH
        case (MS_SWF):
            image = msImageCreateSWF(map->width, map->height, 
                                     map->web.imagepath, map->web.imageurl,
                                     map);
            break;
#endif
        default:
            image = msImageCreateGD(map->width, map->height, map->imagetype,
                                    map->web.imagepath, map->web.imageurl);
    }
  
    if(!image) {
        msSetError(MS_GDERR, "Unable to initialize image.", "msDrawMap()");
        return(NULL);
    }

    if (map->imagetype == MS_GIF ||
        map->imagetype == MS_PNG ||
        map->imagetype == MS_JPEG ||
        map->imagetype == MS_WBMP)
    {
        if(msLoadPalette(image->img.gd, &(map->palette), map->imagecolor) == -1)
            return(NULL);
    }

    map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
    status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scale);
    if(status != MS_SUCCESS) return(NULL);

    for(i=0; i<map->numlayers; i++) {

        if (map->layerorder[i] != -1) {
            lp = &(map->layers[ map->layerorder[i]]);
            //lp = &(map->layers[i]);

            if(lp->postlabelcache) // wait to draw
                continue;

            status = msDrawLayer(map, lp, image);
            if(status != MS_SUCCESS) return(NULL);
        }
    }

    
  if(map->scalebar.status == MS_EMBED && !map->scalebar.postlabelcache)
    msEmbedScalebar(map, image->img.gd); //TODO  

  if(map->legend.status == MS_EMBED && !map->legend.postlabelcache)
    msEmbedLegend(map, image->img.gd); //TODO  

  if(msDrawLabelCache(image, map) == -1)
    return(NULL);

  for(i=0; i<map->numlayers; i++) { // for each layer, check for postlabelcache layers

    lp = &(map->layers[i]);

    if(!lp->postlabelcache)
      continue;

    status = msDrawLayer(map, lp, image);
    if(status != MS_SUCCESS) return(NULL);
  }

  if(map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache)
      msEmbedScalebar(map, image->img.gd); //TODO

  if(map->legend.status == MS_EMBED && map->legend.postlabelcache)
      msEmbedLegend(map, image->img.gd); //TODO

  return(image);
}


/**
 * Utility method to render the query map.
 */
imageObj *msDrawQueryMap(mapObj *map)
{
  int i, status;
  imageObj *image = NULL;
  layerObj *lp=NULL;

  if(map->querymap.width != -1) map->width = map->querymap.width;
  if(map->querymap.height != -1) map->height = map->querymap.height;

  if(map->querymap.style == MS_NORMAL) return(msDrawMap(map)); // no need to do anything fancy

  if(map->width == -1 || map->height == -1) {
    msSetError(MS_MISCERR, "Image dimensions not specified.", "msDrawQueryMap()");
    return(NULL);
  }

  switch (map->imagetype)
  {
      case (MS_GIF):
      case (MS_PNG):
      case (MS_JPEG):
      case (MS_WBMP):
 
          image = msImageCreateGD(map->width, map->height, map->imagetype,
                                    map->web.imagepath, map->web.imageurl);
          break;
        
      default:
          image = msImageCreateGD(map->width, map->height, map->imagetype,
                                    map->web.imagepath, map->web.imageurl);
  }
  
  if(!image) {
    msSetError(MS_GDERR, "Unable to initialize image.", "msDrawQueryMap()");
    return(NULL);
  }

  if (map->imagetype == MS_GIF ||
        map->imagetype == MS_PNG ||
        map->imagetype == MS_JPEG ||
        map->imagetype == MS_WBMP)
  {
      if(msLoadPalette(image->img.gd, &(map->palette), map->imagecolor) == -1)
          return(NULL);
  }

  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scale);
  if(status != MS_SUCCESS) return(NULL);

  for(i=0; i<map->numlayers; i++) {
    lp = &(map->layers[i]);

    if(lp->postlabelcache) // wait to draw
      continue;

    status = msDrawQueryLayer(map, lp, image); 
    if(status != MS_SUCCESS) return(NULL);
  }

  if(map->scalebar.status == MS_EMBED && !map->scalebar.postlabelcache)
      msEmbedScalebar(map, image->img.gd); //TODO

  if(map->legend.status == MS_EMBED && !map->legend.postlabelcache)
      msEmbedLegend(map, image->img.gd); //TODO

  if(msDrawLabelCache(image, map) == -1) //TODO
    return(NULL);

  for(i=0; i<map->numlayers; i++) { // for each layer, check for postlabelcache layers
    lp = &(map->layers[i]);

    if(!lp->postlabelcache)
      continue;

    status = msDrawQueryLayer(map, lp, image); //TODO
    if(status != MS_SUCCESS) return(NULL);
  }

  if(map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache)
      msEmbedScalebar(map, image->img.gd); //TODO

  if(map->legend.status == MS_EMBED && map->legend.postlabelcache)
      msEmbedLegend(map, image->img.gd); //TODO

  return(image);
}

/*
 * Generic function to render a layer object.
*/
int msDrawLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  int i;
  int status;
  char annotate=MS_TRUE, cache=MS_FALSE;
  shapeObj shape;
  rectObj searchrect;
  gdImagePtr img_cache=NULL;
  int retcode=MS_SUCCESS;

  featureListNodeObjPtr shpcache=NULL, current=NULL;

  msImageStartLayer(map, layer, image);

  if(!layer->data && !layer->tileindex && !layer->connection && !layer->features)
  return(MS_SUCCESS); // no data associated with this layer, not an error since layer may be used as a template from MapScript

  if(layer->type == MS_LAYER_QUERY) return(MS_SUCCESS);

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) return(MS_SUCCESS);

  if(msEvalContext(map, layer->requires) == MS_FALSE) return(MS_SUCCESS);
  annotate = msEvalContext(map, layer->labelrequires);

  if(map->scale > 0) {
    
    // layer scale boundaries should be checked first
    if((layer->maxscale > 0) && (map->scale > layer->maxscale))
      return(MS_SUCCESS);
    if((layer->minscale > 0) && (map->scale <= layer->minscale))
      return(MS_SUCCESS);

    // now check class scale boundaries (all layers *must* pass these tests)
    if(layer->numclasses > 0) {
      for(i=0; i<layer->numclasses; i++) {
        if((layer->class[i].maxscale > 0) && (map->scale > layer->class[i].maxscale))
          continue; // can skip this one, next class
        if((layer->class[i].minscale > 0) && (map->scale <= layer->class[i].minscale))
          continue; // can skip this one, next class

        break; // can't skip this class (or layer for that matter)
      } 
      if(i == layer->numclasses) return(MS_SUCCESS);
    }

    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
      annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
      annotate = MS_FALSE;
  }

  if (map->imagetype == MS_GIF ||
      map->imagetype == MS_PNG ||
      map->imagetype == MS_JPEG ||
      map->imagetype == MS_WBMP)
  {
      // Create a temp image for this layer tranparency
      if (layer->transparency > 0) {
          img_cache = image->img.gd;
          image->img.gd = gdImageCreate(img_cache->sx, img_cache->sy);
          if(!image->img.gd) {
              msSetError(MS_GDERR, "Unable to initialize image.", "msDrawLayer()");
              return(MS_FAILURE);
          }
          if(msLoadPalette(image->img.gd, &(map->palette), map->imagecolor) == -1)
              return(MS_FAILURE);
          gdImageColorTransparent(image->img.gd, 0);
      }     
  }
  //For destroy img_temp before return 
  while (1) 
  {
    
    // Redirect procesing of some layer types.
    if(layer->connectiontype == MS_WMS) 
    {
        retcode = msDrawWMSLayer(map, layer, image);
        break;
    }
  
    if(layer->type == MS_LAYER_RASTER) 
    {
        retcode = msDrawRasterLayer(map, layer, image);
      break;
    }
  
    // open this layer
    status = msLayerOpen(layer, map->shapepath);
    if(status != MS_SUCCESS) 
    {
      retcode = MS_FAILURE;
      break;
    }
  
    // build item list
    status = msLayerWhichItems(layer, MS_TRUE, annotate);
    if(status != MS_SUCCESS) 
    {
      retcode = MS_FAILURE;
      break;
    }
  
    // identify target shapes
    if(layer->transform == MS_TRUE)
      searchrect = map->extent;
    else {
      searchrect.minx = searchrect.miny = 0;
      searchrect.maxx = map->width-1;
      searchrect.maxy = map->height-1;
    }
  
  #ifdef USE_PROJ
    if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
      msProjectRect(&map->projection, &layer->projection, &searchrect); // project the searchrect to source coords
  #endif
    status = msLayerWhichShapes(layer, searchrect);
    if(status == MS_DONE) { // no overlap
      msLayerClose(layer);
      retcode = MS_SUCCESS;
      break;
    } else if(status != MS_SUCCESS) {
      retcode = MS_FAILURE;
      break;
    }
  
    // step through the target shapes
    msInitShape(&shape);
  
    while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
  
      shape.classindex = msShapeGetClass(layer, &shape, map->scale);
      if((shape.classindex == -1) || (layer->class[shape.classindex].status == MS_OFF)) {
        msFreeShape(&shape);
        continue;
      }
  
      cache = MS_FALSE;
      if(layer->type == MS_LAYER_LINE || (layer->type == MS_LAYER_POLYGON && layer->class[shape.classindex].color < 0)) 
        cache = MS_TRUE; // only line/polyline layers need to (potentially) be cached with overlayed symbols
  
      // With 'STYLEITEM AUTO', we will have the datasource fill the class'
      // style parameters for this shape.
      if (layer->styleitem && strcasecmp(layer->styleitem, "AUTO") == 0)
      {
          if (msLayerGetAutoStyle(map, layer, &(layer->class[shape.classindex]),
                                  shape.tileindex, shape.index) != MS_SUCCESS)
          {
              retcode = MS_FAILURE;
              break;
          }
  
          // Dynamic class update may have extended the color palette...
          
          if (map->imagetype == MS_GIF ||
              map->imagetype == MS_PNG ||
              map->imagetype == MS_JPEG ||
              map->imagetype == MS_WBMP)
          {
              if (!msUpdatePalette(image->img.gd, &(map->palette)))
              {
                  retcode = MS_FAILURE;
                  break;
              }
          }
          // __TODO__ For now, we can't cache features with 'AUTO' style
          cache = MS_FALSE;
      }
  
      if(annotate && (layer->class[shape.classindex].text.string || layer->labelitem) && layer->class[shape.classindex].label.size != -1)
        shape.text = msShapeGetAnnotation(layer, &shape);
  
      //TODO
      status = msDrawShape(map, layer, &shape, image, !cache); // if caching we DON'T want to do overlays at this time
      if(status != MS_SUCCESS) {
        msLayerClose(layer);
        retcode = MS_FAILURE;
        break;
      }
  
      if(shape.numlines == 0) { // once clipped the shape didn't need to be drawn
        msFreeShape(&shape);
        continue;
      }
  
      if(cache && layer->class[shape.classindex].overlaysymbol >= 0)
        if(insertFeatureList(&shpcache, &shape) == NULL) 
        {
          retcode = MS_FAILURE; // problem adding to the cache
          break;
        }
  
      msFreeShape(&shape);
    }
    
    if (retcode == MS_FAILURE) break;
  
    if(status != MS_DONE) 
    {
      retcode = MS_FAILURE;
      break;
    }
  
    if(shpcache) {
      int c;
  
      for(current=shpcache; current; current=current->next) {
        c = current->shape.classindex;

        msDrawLineSymbol(&map->symbolset, image, &current->shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);
      }
  
      freeFeatureList(shpcache);
      shpcache = NULL;
    }
  
    msLayerClose(layer);
    
    retcode = MS_SUCCESS;
    break;
  }

  // Destroy the temp image for this layer tranparency
  if (map->imagetype == MS_GIF ||
      map->imagetype == MS_PNG ||
      map->imagetype == MS_JPEG ||
      map->imagetype == MS_WBMP)
  {
      if (layer->transparency > 0)
      {
          gdImageCopyMerge(img_cache, image->img.gd, 0, 0, 0, 0, 
                           image->img.gd->sx, 
                           image->img.gd->sy, layer->transparency);
          gdImageDestroy(image->img.gd);
          image->img.gd = img_cache;
      }
  }

  return(retcode);
}



/*
** Function to draw a layer IF it already has a result cache associated with it. Called by msDrawQuery and via MapScript.
*/
int msDrawQueryLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  int i, status;
  char annotate=MS_TRUE, cache=MS_FALSE;
  shapeObj shape;

  featureListNodeObjPtr shpcache=NULL, current=NULL;

  int colorbuffer[MS_MAXCLASSES];

  if(!layer->resultcache || map->querymap.style == MS_NORMAL) // done
    return(msDrawLayer(map, layer, image));

  if(!layer->data && !layer->tileindex && !layer->connection && !layer->features) 
   return(MS_SUCCESS); // no data associated with this layer, not an error since layer may be used as a template from MapScript

  if(layer->type == MS_LAYER_QUERY) return(MS_SUCCESS); // query only layers simply can't be drawn, not an error

  if(map->querymap.style == MS_HILITE) { // first, draw normally, but don't return
    status = msDrawLayer(map, layer, image);
    if(status != MS_SUCCESS) return(MS_FAILURE); // oops
  }

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) return(MS_SUCCESS);

  if(msEvalContext(map, layer->requires) == MS_FALSE) return(MS_SUCCESS);
  annotate = msEvalContext(map, layer->labelrequires);

  if(map->scale > 0) {
    if((layer->maxscale > 0) && (map->scale > layer->maxscale))
      return(MS_SUCCESS);
    if((layer->minscale > 0) && (map->scale <= layer->minscale))
      return(MS_SUCCESS);
    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
      annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
      annotate = MS_FALSE;
  }

  // if MS_HILITE, alter the first class (always at least 1 class)
  if(map->querymap.style == MS_HILITE) {
    for(i=0; i<layer->numclasses; i++) {
      colorbuffer[i] = layer->class[i].color; // save the color
      layer->class[i].color = map->querymap.color;
    }
  }

  // open this layer
  status = msLayerOpen(layer, map->shapepath);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  // build item list
  status = msLayerWhichItems(layer, MS_FALSE, annotate); // FIX: results have already been classified (this may change)
  if(status != MS_SUCCESS) return(MS_FAILURE);

  msInitShape(&shape);

  for(i=0; i<layer->resultcache->numresults; i++) {
    status = msLayerGetShape(layer, &shape, layer->resultcache->results[i].tileindex, layer->resultcache->results[i].shapeindex);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    shape.classindex = layer->resultcache->results[i].classindex;
    if(layer->class[shape.classindex].status == MS_OFF) {
      msFreeShape(&shape);
      continue;
    }

    cache = MS_FALSE;
    if(layer->type == MS_LAYER_LINE || (layer->type == MS_LAYER_POLYGON && layer->class[shape.classindex].color < 0)) 
      cache = MS_TRUE; // only line/polyline layers need to (potentially) be cached with overlayed symbols

    if(annotate && (layer->class[shape.classindex].text.string || layer->labelitem) && layer->class[shape.classindex].label.size != -1)
      shape.text = msShapeGetAnnotation(layer, &shape);

    //TODO
    status = msDrawShape(map, layer, &shape, image, !cache); // if caching we DON'T want to do overlays at this time
    if(status != MS_SUCCESS) return(MS_FAILURE);

    if(shape.numlines == 0) { // once clipped the shape didn't need to be drawn
      msFreeShape(&shape);
      continue;
    }

    if(cache && layer->class[shape.classindex].overlaysymbol >= 0)
      if(insertFeatureList(&shpcache, &shape) == NULL) return(MS_FAILURE); // problem adding to the cache

    msFreeShape(&shape);
  }

  if(shpcache) {
    int c;

    for(current=shpcache; current; current=current->next) {
      c = current->shape.classindex;
      //TODO
      msDrawLineSymbol(&map->symbolset, image, &current->shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);
    }

    freeFeatureList(shpcache);
    shpcache = NULL;
  }

  // if MS_HILITE, restore values
  if(map->querymap.style == MS_HILITE)
    for(i=0; i<layer->numclasses; i++) layer->class[i].color = colorbuffer[i]; // restore the color

  msLayerClose(layer);

  return(MS_SUCCESS);
}

/**
 * Generic function to render raster layers.
 */
int msDrawRasterLayer(mapObj *map, layerObj *layer, imageObj *image) 
{
    if (image && map && layer)
    {
        switch (image->imagetype)
        {
            case (MS_GIF):
            case (MS_PNG):
            case (MS_JPEG):
            case (MS_WBMP):
                return msDrawRasterLayerGD(map, layer, image->img.gd) ;

#ifdef USE_MING_FLASH
            case (MS_SWF):
                return  msDrawRasterLayerSWF(map, layer, image);
#endif

          default:
              break;
        }
    }
    return 0;
}

int msDrawWMSLayer(mapObj *map, layerObj *layer, imageObj *image)
{
    if (image && map && layer)
    {
        switch (image->imagetype)
        {
            case (MS_GIF):
            case (MS_PNG):
            case (MS_JPEG):
            case (MS_WBMP):
                return msDrawWMSLayerGD(map, layer, image->img.gd) ;

#ifdef USE_MING_FLASH                
            case (MS_SWF):
                return  msDrawWMSLayerSWF(map, layer, image);
#endif

          default:
              break;
        }
    }
    return 0;
}

/*
** Function to render an individual shape, the overlay boolean variable enables/disables drawing of the
** overlay symbology. This is necessary when drawing entire layers as proper overlay can only be achived
** through caching.
*/
int msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, 
                imageObj *image, int overlay)
{
  int i,j,c;
  rectObj cliprect;
  pointObj annopnt, *point;
  double angle, length, scalefactor=1.0;
 
  pointObj center; // circle origin
  double r; // circle radius
  int csz; // clip over size
  
/* Steve's original code
  cliprect.minx = map->extent.minx - 2*map->cellsize; // set clipping rectangle just a bit larger than the map extent
  cliprect.miny = map->extent.miny - 2*map->cellsize;
  cliprect.maxx = map->extent.maxx + 2*map->cellsize;
  cliprect.maxy = map->extent.maxy + 2*map->cellsize;
*/

  c = shape->classindex;

  if(layer->symbolscale > 0 && map->scale > 0) scalefactor = layer->symbolscale/map->scale;

  if(layer->sizeunits != MS_PIXELS) {
    layer->class[c].sizescaled = (int)(layer->class[c].size * (inchesPerUnit[layer->sizeunits]/inchesPerUnit[map->units]) / map->cellsize);
    layer->class[c].overlaysizescaled = (int)(layer->class[c].overlaysize * (inchesPerUnit[layer->sizeunits]/inchesPerUnit[map->units]) / map->cellsize);
  } else {
    layer->class[c].sizescaled = MS_NINT(layer->class[c].size * scalefactor);
    layer->class[c].sizescaled = MS_MAX(layer->class[c].sizescaled, layer->class[c].minsize);
    layer->class[c].sizescaled = MS_MIN(layer->class[c].sizescaled, layer->class[c].maxsize);
    layer->class[c].overlaysizescaled = layer->class[c].sizescaled - (layer->class[c].size - layer->class[c].overlaysize); // layer->class[c].overlaysizescaled = MS_NINT(layer->class[c].overlaysize * scalefactor);
    layer->class[c].overlaysizescaled = MS_MAX(layer->class[c].overlaysizescaled, layer->class[c].overlayminsize);
    layer->class[c].overlaysizescaled = MS_MIN(layer->class[c].overlaysizescaled, layer->class[c].overlaymaxsize);
  }

  if(layer->class[c].sizescaled == 0) return(MS_SUCCESS);

  csz = MS_NINT(layer->class[c].sizescaled/2.0); // changed when Tomas added CARTOLINE symbols
  cliprect.minx = map->extent.minx - csz*map->cellsize;
  cliprect.miny = map->extent.miny - csz*map->cellsize;
  cliprect.maxx = map->extent.maxx + csz*map->cellsize;
  cliprect.maxy = map->extent.maxy + csz*map->cellsize;

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  if(layer->class[c].label.type == MS_TRUETYPE) {
    layer->class[c].label.sizescaled = MS_NINT(layer->class[c].label.size * scalefactor);
    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
  }
#endif

  switch(layer->type) {
  case MS_LAYER_CIRCLE:
    if(shape->numlines != 1) return(MS_SUCCESS); // invalid shape
    if(shape->line[0].numpoints != 2) return(MS_SUCCESS); // invalid shape

    center.x = (shape->line[0].point[0].x + shape->line[0].point[1].x)/2.0;
    center.y = (shape->line[0].point[0].y + shape->line[0].point[1].y)/2.0;
    r = MS_ABS(center.x - shape->line[0].point[0].x);

#ifdef USE_PROJ
    if(msProjectionsDiffer(&(layer->projection), &(map->projection)))
        msProjectPoint(&layer->projection, &map->projection, &center);
#endif

    if(layer->transform) {
      center.x = MS_MAP2IMAGE_X(center.x, map->extent.minx, map->cellsize);
      center.y = MS_MAP2IMAGE_Y(center.y, map->extent.maxy, map->cellsize);
      r *= (inchesPerUnit[layer->units]/inchesPerUnit[map->units])/map->cellsize;      
    }

    if(layer->class[c].color < 0)
        msCircleDrawLineSymbol(&map->symbolset, image, &center, r, layer->class[c].symbol, layer->class[c].outlinecolor, layer->class[c].backgroundcolor, layer->class[c].sizescaled);
    else
        msCircleDrawShadeSymbol(&map->symbolset, image, &center, r, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
  

    if(overlay && layer->class[c].overlaysymbol >= 0) {
      if(layer->class[c].overlaycolor < 0)
	msCircleDrawLineSymbol(&map->symbolset, image, &center, r, layer->class[c].overlaysymbol, layer->class[c].overlayoutlinecolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);
      else
        msCircleDrawShadeSymbol(&map->symbolset, image, &center, r, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
    }

    break;
  case MS_LAYER_ANNOTATION:
    if(!shape->text) return(MS_SUCCESS); // nothing to draw

#ifdef USE_PROJ
    if(msProjectionsDiffer(&(layer->projection), &(map->projection)))
       msProjectShape(&layer->projection, &map->projection, shape);
#endif

    switch(shape->type) {
    case(MS_SHAPE_LINE):
      if(layer->transform) {
	msClipPolylineRect(shape, cliprect);
	if(shape->numlines == 0) return(MS_SUCCESS);
	msTransformShape(shape, map->extent, map->cellsize);
      }

      if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) == MS_SUCCESS) {

	if(layer->labelangleitemindex != -1)
	  layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}

	if(layer->class[c].label.autoangle)
	  layer->class[c].label.angle = angle;

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, length);
	else {
	  if(layer->class[c].color != -1) {
	    msDrawMarkerSymbol(&map->symbolset, image, &annopnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, image, &annopnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	  }
	  msDrawLabel(image, annopnt, shape->text, &(layer->class[c].label), 
                      &map->fontset);
	}
      }

      break;
    case(MS_SHAPE_POLYGON):

      if(layer->transform) {
	msClipPolygonRect(shape, cliprect);
	if(shape->numlines == 0) return(MS_SUCCESS);
	msTransformShape(shape, map->extent, map->cellsize);
      }

      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) == MS_SUCCESS) {

	if(layer->labelangleitemindex != -1)
	  layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, length);
	else {
	  if(layer->class[c].color != -1) {
	    msDrawMarkerSymbol(&map->symbolset, image, &annopnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, image, &annopnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	  }
	  msDrawLabel(image, annopnt, shape->text, &(layer->class[c].label), &map->fontset);
	}
      }
      break;
    default: // points and anything with out a proper type
      for(j=0; j<shape->numlines;j++) {
	for(i=0; i<shape->line[j].numpoints;i++) {

	  point = &(shape->line[j].point[i]);

	  if(layer->transform) {
	    if(!msPointInRect(point, &map->extent)) return(MS_SUCCESS);
	    point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
	    point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
	  }

	  if(layer->labelangleitemindex != -1)
	    layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	  if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	    layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	  }

	  if(shape->text) {
	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, shape->tileindex, shape->index, *point, shape->text, -1);
	    else {
	      if(layer->class[c].color == -1) {
		msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
		if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	      }
	      msDrawLabel(image, *point, shape->text, &layer->class[c].label, &map->fontset);
	    }
	  }
	}
      }
    }
    break;

  case MS_LAYER_POINT:

#ifdef USE_PROJ
      if(msProjectionsDiffer(&(layer->projection), &(map->projection)))
          msProjectShape(&layer->projection, &map->projection, shape);
#endif

    for(j=0; j<shape->numlines;j++) {
      for(i=0; i<shape->line[j].numpoints;i++) {

	point = &(shape->line[j].point[i]);

	if(layer->transform) {
	  if(!msPointInRect(point, &map->extent)) continue; // next point
	  point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
	  point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
	}

	msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	if(overlay && layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);

	if(shape->text) {
	  if(layer->labelangleitemindex != -1)
	    layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	  if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	    layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	  }

	  if(layer->labelcache)
	    msAddLabel(map, layer->index, c, shape->tileindex, shape->index, *point, shape->text, -1);
	  else
	    msDrawLabel(image, *point, shape->text, &layer->class[c].label, &map->fontset);
	}
      }
    }
    break;

  case MS_LAYER_LINE:
    if(shape->type != MS_SHAPE_POLYGON && shape->type != MS_SHAPE_LINE){
      msSetError(MS_MISCERR, "Only polygon or line shapes can be drawn using a line layer definition.", "msDrawShape()");
      return(MS_FAILURE);
    }

#ifdef USE_PROJ
    if(msProjectionsDiffer(&(layer->projection), &(map->projection)))
        msProjectShape(&layer->projection, &map->projection, shape);
#endif

    if(layer->transform) {
      msClipPolylineRect(shape, cliprect);
      if(shape->numlines == 0) return(MS_SUCCESS);
      msTransformShape(shape, map->extent, map->cellsize);
    }

    msDrawLineSymbol(&map->symbolset, image, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].sizescaled);
    if(overlay && layer->class[c].overlaysymbol >= 0) msDrawLineSymbol(&map->symbolset, image, shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);

    if(shape->text) {
      if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) == MS_SUCCESS) {
	if(layer->labelangleitemindex != -1)
	  layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}

	if(layer->class[c].label.autoangle)
	  layer->class[c].label.angle = angle;

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, length);
	else
            msDrawLabel(image, annopnt, shape->text, &layer->class[c].label, &map->fontset);
      }
    }
    break;

  case MS_LAYER_POLYGON:
    if(shape->type != MS_SHAPE_POLYGON){
      msSetError(MS_MISCERR, "Only polygon shapes can be drawn using a POLYGON layer definition.", "msDrawShape()");
      return(MS_FAILURE);
    }

#ifdef USE_PROJ
    if(msProjectionsDiffer(&(layer->projection), &(map->projection))) 
        msProjectShape(&layer->projection, &map->projection, shape);
#endif

    if(layer->transform) {
      msClipPolygonRect(shape, cliprect);
      if(shape->numlines == 0) return(MS_SUCCESS);
      msTransformShape(shape, map->extent, map->cellsize);
    }

    if(layer->class[c].color < 0)
      msDrawLineSymbol(&map->symbolset, image, shape, layer->class[c].symbol, layer->class[c].outlinecolor, layer->class[c].backgroundcolor, layer->class[c].sizescaled);
    else
      msDrawShadeSymbol(&map->symbolset, image, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);

    if(overlay && layer->class[c].overlaysymbol >= 0) {
      if(layer->class[c].overlaycolor < 0)
	msDrawLineSymbol(&map->symbolset, image, shape, layer->class[c].overlaysymbol, layer->class[c].overlayoutlinecolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);
      else
        msDrawShadeSymbol(&map->symbolset, image, shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
    }

    if(shape->text) {
      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) == MS_SUCCESS) {
	if(layer->labelangleitemindex != -1)
	  layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, -1);
	else
	  msDrawLabel(image, annopnt, shape->text, &layer->class[c].label, &map->fontset);
      }
    }
    break;
  default:
    msSetError(MS_MISCERR, "Unknown layer type.", "msDrawShape()");
    return(MS_FAILURE);
  }

  return(MS_SUCCESS);
}


/*
** Function to render an individual point, used as a helper function for mapscript only. Since a point
** can't carry attributes you can't do attribute based font size or angle.
*/
int msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, imageObj *image, 
                int classindex, char *labeltext)
{
  int c;
  double scalefactor=1.0;

  c = classindex;

  if(layer->symbolscale > 0 && map->scale > 0) scalefactor = layer->symbolscale/map->scale;

  if(layer->sizeunits != MS_PIXELS) {
    layer->class[c].sizescaled = layer->class[c].size * (inchesPerUnit[layer->sizeunits]/inchesPerUnit[map->units]) / map->cellsize;
    layer->class[c].overlaysizescaled = layer->class[c].overlaysize * (inchesPerUnit[layer->sizeunits]/inchesPerUnit[map->units]) / map->cellsize;
  } else {
    layer->class[c].sizescaled = MS_NINT(layer->class[c].size * scalefactor);
    layer->class[c].sizescaled = MS_MAX(layer->class[c].sizescaled, layer->class[c].minsize);
    layer->class[c].sizescaled = MS_MIN(layer->class[c].sizescaled, layer->class[c].maxsize);
    layer->class[c].overlaysizescaled = layer->class[c].sizescaled - (layer->class[c].size - layer->class[c].overlaysize); // layer->class[c].overlaysizescaled = MS_NINT(layer->class[c].overlaysize * scalefactor);
    layer->class[c].overlaysizescaled = MS_MAX(layer->class[c].overlaysizescaled, layer->class[c].overlayminsize);
    layer->class[c].overlaysizescaled = MS_MIN(layer->class[c].overlaysizescaled, layer->class[c].overlaymaxsize);
  }

  if(layer->class[c].sizescaled == 0) return(MS_SUCCESS);

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  if(layer->class[c].label.type == MS_TRUETYPE) {
    layer->class[c].label.sizescaled = MS_NINT(layer->class[c].label.size * scalefactor);
    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
  }
#endif

#ifdef USE_PROJ
    if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
      msProjectPoint(&layer->projection, &map->projection, point);
#endif

  switch(layer->type) {
  case MS_LAYER_ANNOTATION:
    if(layer->transform) {
      if(!msPointInRect(point, &map->extent)) return(0);
      point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
      point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
    }

    if(labeltext) {
      if(layer->labelcache)
	msAddLabel(map, layer->index, c, -1, -1, *point, labeltext, -1);
      else {
	if(layer->class[c].color == -1) {
	  msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	  if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	}
	msDrawLabel(image, *point, labeltext, &layer->class[c].label, &map->fontset);
      }
    }
    break;

  case MS_LAYER_POINT:
    if(layer->transform) {
      if(!msPointInRect(point, &map->extent)) return(0);
      point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
      point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
    }

    msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);

    if(labeltext) {
      if(layer->labelcache)
	msAddLabel(map, layer->index, c, -1, -1, *point, labeltext, -1);
      else
	msDrawLabel(image, *point, labeltext, &layer->class[c].label, 
                    &map->fontset);
    }
    break;
  default:
    break; /* don't do anything with layer of other types */
  }

  return(MS_SUCCESS); /* all done, no cleanup */
}


/************************************************************************/
/*                          msCircleDrawLineSymbol                      */
/*                                                                      */
/*      Note : the map parameter is only used to be able to converet    */
/*      the color index to rgb values.                                  */
/************************************************************************/
void msCircleDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, 
                            pointObj *p, double r,  int sy, int fc, 
                            int bc, double sz)
{
    /*
    getRgbColor(map, layer->class[c].outlinecolor, &sOutlineColor.red, 
                    &sOutlineColor.green, &sOutlineColor.blue);
        getRgbColor(map, layer->class[c].backgroundcolor, &sBackgroundColor.red, 
                    &sBackgroundColor.green, &sBackgroundColor.blue);
    */
    if (image)
    {
        switch (image->imagetype)
        {
            case (MS_GIF):
            case (MS_PNG):
            case (MS_JPEG):
            case (MS_WBMP):
                msCircleDrawLineSymbolGD(symbolset, image->img.gd, p, r, sy, 
                                         fc, bc, sz);
              
              break;

          default:
             msSetError(MS_MISCERR, "Unknown image type", 
                        "msCircleDrawLineSymbol()"); 
        }
    }
}

void msCircleDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, 
                             pointObj *p, double r, int sy, int fc, int bc, 
                             int oc, double sz)
{
    if (image)
    {
        switch (image->imagetype)
        {
            case (MS_GIF):
            case (MS_PNG):
            case (MS_JPEG):
            case (MS_WBMP):
                msCircleDrawShadeSymbolGD(symbolset, image->img.gd, p, r, sy, 
                                          fc, bc, oc, sz);
              
              break;

          default:
             msSetError(MS_MISCERR, "Unknown image type", 
                        "msCircleDrawShadeSymbol()"); 
        }
    }
}


void msDrawMarkerSymbol(symbolSetObj *symbolset,imageObj *image, pointObj *p,  
                        int sy, int fc, int bc, int oc, double sz)
{
    /*Note : should we use color obkect instead of id's. Easier to handle for 
             vector output format as SWF, PDF
    */
   if (image)
   {
        switch (image->imagetype)
        {
            case (MS_GIF):
            case (MS_PNG):
            case (MS_JPEG):
            case (MS_WBMP):
                msDrawMarkerSymbolGD(symbolset, image->img.gd, p, sy, 
                                     fc, bc, oc, sz);
              
                break;

#ifdef USE_MING_FLASH              
            case (MS_SWF):
                msDrawMarkerSymbolSWF(symbolset, image, p, sy, fc, bc, oc, sz);
                break;
#endif
          default:
              break;
        }
    }
}



int msDrawLabel(imageObj *image, pointObj labelPnt, char *string, 
                labelObj *label, fontSetObj *fontset)
{
  if(!string)
    return(0); /* not an error, just don't want to do anything */

  if(strlen(string) == 0)
    return(0); /* not an error, just don't want to do anything */


/* ==================================================================== */
/*      TODO : This is a temporary hack to call the drawlableswf directly. */
/*      Normally the only functions that should be wrapped here is      */
/*      draw_text. We did this since msGetLabelSize has not yet been    */
/*      implemented for MING FDB fonts.                                 */
/* ==================================================================== */

#ifdef USE_MING_FLASH
  if (image->imagetype == MS_SWF)
      return msDrawLabelSWF(image, labelPnt, string, label, fontset);
#endif

  if(label->position != MS_XY) {
    pointObj p;
    rectObj r;

    if(msGetLabelSize(string, label, &r, fontset) == -1) return(-1);
    p = get_metrics(&labelPnt, label->position, r, label->offsetx, label->offsety, label->angle, 0, NULL);
    draw_text(image, p, string, label, fontset); /* actually draw the label */
  } else {
    labelPnt.x += label->offsetx;
    labelPnt.y += label->offsety;
    draw_text(image, labelPnt, string, label, fontset); /* actually draw the label */
  }

  return(0);
}


int draw_text(imageObj *image, pointObj labelPnt, char *string, 
              labelObj *label, fontSetObj *fontset)
{
    int nReturnVal = -1;
    if (image)
    {
         switch (image->imagetype)
        {
            case (MS_GIF):
            case (MS_PNG):
            case (MS_JPEG):
            case (MS_WBMP):
                nReturnVal = draw_textGD(image->img.gd, labelPnt, string, 
                                      label, fontset);
                break;

#ifdef USE_MING_FLASH
            case (MS_SWF): 
               nReturnVal = draw_textSWF(image, labelPnt, string, label, 
                                         fontset); 
               break;
#endif

          default:
              break;
        }
    }

    return nReturnVal;
}

int msDrawLabelCache(imageObj *image, mapObj *map)
{
    int nReturnVal = MS_SUCCESS;
    if (image)
    {
        switch (image->imagetype)
        {
            case (MS_GIF):
            case (MS_PNG):
            case (MS_JPEG):
            case (MS_WBMP):
                nReturnVal = msDrawLabelCacheGD(image->img.gd, map);
                break;

#ifdef USE_MING_FLASH
            case (MS_SWF):
                nReturnVal = msDrawLabelCacheSWF(image, map);
                break;
#endif

            default:
                break;
        }
    }

    return nReturnVal;
}


void msDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, 
                      int sy, int fc, int bc, double sz)
{
    if (image)
    {
        switch (image->imagetype)
        {
            case (MS_GIF):
            case (MS_PNG):
            case (MS_JPEG):
            case (MS_WBMP):
                msDrawLineSymbolGD(symbolset, image->img.gd, p,
                                   sy, fc, bc, sz);
                break;
                
#ifdef USE_MING_FLASH
            case (MS_SWF):
                msDrawLineSymbolSWF(symbolset, image, p, sy, fc, bc, sz);
                break;
#endif

          default:
              break;
        }
    }
}

void msDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, 
                       int sy, int fc, int bc, int oc, double sz)
{
    if (image)
    {
        switch (image->imagetype)
        {
            case (MS_GIF):
            case (MS_PNG):
            case (MS_JPEG):
            case (MS_WBMP):
                msDrawShadeSymbolGD(symbolset, image->img.gd, p,
                                    sy, fc, bc, oc, sz);
                break;

#ifdef USE_MING_FLASH
            case (MS_SWF):
                msDrawShadeSymbolSWF(symbolset, image, p, sy, fc, bc, oc, sz);
                break;
#endif

          default:
              break;
        }
    }
}
    

/**
 * Generic function to tell the underline device that layer 
 * drawing is stating
 */

void msImageStartLayer(mapObj *map, layerObj *layer, imageObj *image)
{
    if (image)
    {
        switch (image->imagetype)
        {

#ifdef USE_MING_FLASH
            case (MS_SWF):
                msImageStartLayerSWF(map, layer, image);
                break;
#endif
               
            default:
                break;
       }
    }
}

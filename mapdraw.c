/**********************************************************************
 * $Id$
 */


#include "map.h"

static double inchesPerUnit[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};

/*
 * Function to reset any pen (color index) values previously set. Used primarily to reset things when
 * using MapScript to create multiple images. How the pen values are set is irrelevant (definitely output
 * format type specific) which is why this function is here instead of the GD, PDF or SWF source files.
*/
void msClearPenValues(mapObj *map) {
  int i,j,k;

  for(i=0; i<map->numlayers; i++) {
    for(j=0; j<map->layers[i].numclasses; j++) {
      map->layers[i].class[j].label.backgroundcolor.pen = MS_PEN_UNSET; // set in billboardXX function
      map->layers[i].class[j].label.backgroundshadowcolor.pen = MS_PEN_UNSET;
      map->layers[i].class[j].label.color.pen = MS_PEN_UNSET; // set in MSXXDrawText function
      map->layers[i].class[j].label.outlinecolor.pen = MS_PEN_UNSET;
      map->layers[i].class[j].label.shadowcolor.pen = MS_PEN_UNSET;      

      for(k=0; k<map->layers[i].class[j].numstyles; k++) {
        map->layers[i].class[j].styles[k].backgroundcolor.pen = MS_PEN_UNSET; // set in various symbol drawing functions
	map->layers[i].class[j].styles[k].color.pen = MS_PEN_UNSET;
        map->layers[i].class[j].styles[k].outlinecolor.pen = MS_PEN_UNSET; 
      }
    }
  }

  return;
}

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
    httpRequestObj asWMSReqInfo[MS_MAXLAYERS];
    int numWMSRequests=0;

    if(map->width == -1 || map->height == -1) {
        msSetError(MS_MISCERR, "Image dimensions not specified.", "msDrawMap()");
        return(NULL);
    }

    if( MS_RENDERER_GD(map->outputformat) )
    {
        image = msImageCreateGD(map->width, map->height, map->outputformat, 
				map->web.imagepath, map->web.imageurl);        
        if( image != NULL ) msImageInitGD( image, &map->imagecolor );
    }
    else if( MS_RENDERER_RAWDATA(map->outputformat) )
    {
        image = msImageCreate(map->width, map->height, map->outputformat,
                              map->web.imagepath, map->web.imageurl);
    }
#ifdef USE_MING_FLASH
    else if( MS_RENDERER_SWF(map->outputformat) )
    {
        image = msImageCreateSWF(map->width, map->height, map->outputformat,
                                 map->web.imagepath, map->web.imageurl,
                                 map);
    }
#endif
    else
    {
        image = NULL;
    }
  
    if(!image) {
        msSetError(MS_GDERR, "Unable to initialize image.", "msDrawMap()");
        return(NULL);
    }

    map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
    status = msCalculateScale(map->extent, map->units, map->width, map->height,
                              map->resolution, &map->scale);
    if(status != MS_SUCCESS) return(NULL);

    // compute layer scale factors now
    for(i=0;i<map->numlayers; i++) {
      if(map->layers[i].symbolscale > 0 && map->scale > 0) {
    	if(map->layers[i].sizeunits != MS_PIXELS)
      	  map->layers[i].scalefactor = (inchesPerUnit[map->layers[i].sizeunits]/inchesPerUnit[map->units]) / map->cellsize; 
    	else
      	  map->layers[i].scalefactor = map->layers[i].symbolscale/map->scale;
      }
    }

#ifdef USE_WMS_LYR
    // Pre-download all WMS layers in parallel before starting to draw map
    for(i=0; i<map->numlayers; i++) 
    {
        // layerorder doesn't matter at this point
        if (map->layers[i].connectiontype == MS_WMS)  
        {
            if ( msPrepareWMSLayerRequest(i, map, &(map->layers[i]),
                                          asWMSReqInfo, 
                                          &numWMSRequests) == MS_FAILURE)
            {
                return NULL;
            }
        }
    }
    if (numWMSRequests && 
        msWMSExecuteRequests(asWMSReqInfo, numWMSRequests) == MS_FAILURE)
    {
        return NULL;
    }
#endif
    // OK, now we can start drawing

    for(i=0; i<map->numlayers; i++) {

        if (map->layerorder[i] != -1) {
            lp = &(map->layers[ map->layerorder[i]]);
            //lp = &(map->layers[i]);

            if(lp->postlabelcache) // wait to draw
                continue;

            if (lp->connectiontype == MS_WMS) 
#ifdef USE_WMS_LYR 
                status = msDrawWMSLayerLow(i, asWMSReqInfo, numWMSRequests, 
                                           map, lp, image);
#else
	        status = MS_FAILURE;
#endif
            else 
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

    if (lp->connectiontype == MS_WMS)  
#ifdef USE_WMS_LYR 
        status = msDrawWMSLayerLow(i, asWMSReqInfo, numWMSRequests, 
                                   map, lp, image);
#else
	status = MS_FAILURE;
#endif
    else 
        status = msDrawLayer(map, lp, image);
    if(status != MS_SUCCESS) return(NULL);
  }

  if(map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache)
      msEmbedScalebar(map, image->img.gd); //TODO

  if(map->legend.status == MS_EMBED && map->legend.postlabelcache)
      msEmbedLegend(map, image->img.gd); //TODO

  // Cleanup WMS Request stuff
  msFreeRequestObj(asWMSReqInfo, numWMSRequests);

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

  if( MS_RENDERER_GD(map->outputformat) )
  {
      image = msImageCreateGD(map->width, map->height, map->outputformat, map->web.imagepath, map->web.imageurl);      
      if( image != NULL ) msImageInitGD( image, &map->imagecolor );
  }
  
  if(!image) {
    msSetError(MS_GDERR, "Unable to initialize image.", "msDrawQueryMap()");
    return(NULL);
  }

  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scale);
  if(status != MS_SUCCESS) return(NULL);

  // compute layer scale factors now
  for(i=0;i<map->numlayers; i++) {
    if(map->layers[i].symbolscale > 0 && map->scale > 0) {
      if(map->layers[i].sizeunits != MS_PIXELS)
      	map->layers[i].scalefactor = (inchesPerUnit[map->layers[i].sizeunits]/inchesPerUnit[map->units]) / map->cellsize; 
      else
      	map->layers[i].scalefactor = map->layers[i].symbolscale/map->scale;
    }
  }

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
  gdImagePtr img_cache=NULL;
  int retcode=MS_SUCCESS;

  msImageStartLayer(map, layer, image);

  if(!layer->data && !layer->tileindex && !layer->connection && !layer->features)
  return(MS_SUCCESS); // no data associated with this layer, not an error since layer may be used as a template from MapScript

  if(layer->type == MS_LAYER_QUERY) return(MS_SUCCESS);
  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) return(MS_SUCCESS);
  if(msEvalContext(map, layer->requires) == MS_FALSE) return(MS_SUCCESS);

  if(map->scale > 0) {
    
    // layer scale boundaries should be checked first
    if((layer->maxscale > 0) && (map->scale > layer->maxscale)) return(MS_SUCCESS);
    if((layer->minscale > 0) && (map->scale <= layer->minscale)) return(MS_SUCCESS);

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

    //if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
    // annotate = MS_FALSE;
    //if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
    // annotate = MS_FALSE;
  }

  if ( MS_RENDERER_GD(map->outputformat) ) {
    // Create a temp image for this layer tranparency
    if (layer->transparency > 0) {
      img_cache = image->img.gd;
      image->img.gd = gdImageCreate(img_cache->sx, img_cache->sy);
      if(!image->img.gd) {
        msSetError(MS_GDERR, "Unable to initialize image.", "msDrawLayer()");
        return(MS_FAILURE);
      }
      msImageInitGD(image, &map->imagecolor);
      gdImageColorTransparent(image->img.gd, 0);
    }
  }

  // Redirect procesing of some layer types.
  if(layer->connectiontype == MS_WMS) 
  {
#ifdef USE_WMS_LYR
      retcode = msDrawWMSLayer(map, layer, image);
#else  
      retcode = MS_FAILURE;
#endif
  }
  else if(layer->type == MS_LAYER_RASTER) 
  {
      retcode = msDrawRasterLayer(map, layer, image);
  }
  //Must be a Vector layer
  else
      retcode = msDrawVectorLayer(map, layer, image);

  // Destroy the temp image for this layer tranparency
  if( MS_RENDERER_GD(map->outputformat) && layer->transparency > 0 ) {
    gdImageCopyMerge(img_cache, image->img.gd, 0, 0, 0, 0, image->img.gd->sx, image->img.gd->sy, layer->transparency);
    gdImageDestroy(image->img.gd);
    image->img.gd = img_cache;
  }

  return(retcode);
}

int msDrawVectorLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  int         status, retcode=MS_SUCCESS;
  char        annotate=MS_TRUE;
  shapeObj    shape;
  rectObj     searchrect;
  char        cache=MS_FALSE;
  int         maxnumstyles=1;

  featureListNodeObjPtr shpcache=NULL, current=NULL;

/* ==================================================================== */
/*      For Flash, we use a metadata called SWFOUTPUT that              */
/*      is used to render vector layers as rasters.                     */
/* ==================================================================== */
#ifdef USE_MING_FLASH
  if(image &&  MS_RENDERER_SWF(image->format) && 
     msLookupHashTable(layer->metadata, "SWFOUTPUT") &&
     strcasecmp(msLookupHashTable(layer->metadata, "SWFOUTPUT"), "RASTER")==0)
    return msDrawVectorLayerAsRasterSWF(map, layer, image);
#endif

  annotate = msEvalContext(map, layer->labelrequires);
  if(map->scale > 0) {
    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale)) annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale)) annotate = MS_FALSE;
  }

  // open this layer
  status = msLayerOpen(layer, map->shapepath);
  if(status != MS_SUCCESS) return MS_FAILURE;
  
  // build item list
/* ==================================================================== */
/*      For Flash, we use a metadata called SWFDUMPATTRIBUTES that      */
/*      contains a list of attributes that we want to write to the      */
/*      flash movie for query purpose.                                  */
/* ==================================================================== */
  if(image && MS_RENDERER_SWF(image->format))
    status = msLayerWhichItems(layer, MS_TRUE, annotate, msLookupHashTable(layer->metadata, "SWFDUMPATTRIBUTES"));                                
  else        
    status = msLayerWhichItems(layer, MS_TRUE, annotate, NULL);
  if(status != MS_SUCCESS) return MS_FAILURE;
  
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
    return MS_SUCCESS;
  } else if(status != MS_SUCCESS) {
    msLayerClose(layer);
    return MS_FAILURE;
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
    if(layer->type == MS_LAYER_LINE && layer->class[shape.classindex].numstyles > 1) 
      cache = MS_TRUE; // only line layers with multiple styles need be cached (I don't think POLYLINE layers need caching - SDL)
         
    // With 'STYLEITEM AUTO', we will have the datasource fill the class'
    // style parameters for this shape.
    if(layer->styleitem && strcasecmp(layer->styleitem, "AUTO") == 0) {
      if(msLayerGetAutoStyle(map, layer, &(layer->class[shape.classindex]), shape.tileindex, shape.index) != MS_SUCCESS) {
        retcode = MS_FAILURE;
        break;
      }
                  
      // __TODO__ For now, we can't cache features with 'AUTO' style
      cache = MS_FALSE;
    }
  
    if(annotate && (layer->class[shape.classindex].text.string || layer->labelitem) && layer->class[shape.classindex].label.size != -1)
      shape.text = msShapeGetAnnotation(layer, &shape);

    if(cache)
      status = msDrawShape(map, layer, &shape, image, 0); // draw only the first style
    else
      status = msDrawShape(map, layer, &shape, image, -1); // all styles 
    if(status != MS_SUCCESS) {
      msLayerClose(layer);
      retcode = MS_FAILURE;
      break;
    }

    if(shape.numlines == 0) { // once clipped the shape didn't need to be drawn
      msFreeShape(&shape);
      continue;
    }
  
    if(cache) {
      if(insertFeatureList(&shpcache, &shape) == NULL) {
        retcode = MS_FAILURE; // problem adding to the cache
        break;
      }
    }  

    maxnumstyles = MS_MAX(maxnumstyles, layer->class[shape.classindex].numstyles);
    msFreeShape(&shape);
  }
    
  if(retcode == MS_FAILURE) return MS_FAILURE;  
  if(status != MS_DONE) return MS_FAILURE;
  
  if(shpcache) {
    int s;
      
    for(s=1; s<maxnumstyles; s++) {
      for(current=shpcache; current; current=current->next) {        
        if(layer->class[current->shape.classindex].numstyles > s)
	  msDrawLineSymbol(&map->symbolset, image, &current->shape, &(layer->class[current->shape.classindex].styles[s]), layer->scalefactor);
      }
    }
    
    freeFeatureList(shpcache);
    shpcache = NULL;  
  }

  msLayerClose(layer);  
  return MS_SUCCESS;
}

/*
** Function to draw a layer IF it already has a result cache associated with it. Called by msDrawQuery and via MapScript.
*/
int msDrawQueryLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  int i, status;
  char annotate=MS_TRUE, cache=MS_FALSE;
  shapeObj shape;
  int maxnumstyles=1;

  featureListNodeObjPtr shpcache=NULL, current=NULL;

  colorObj colorbuffer[MS_MAXCLASSES];

  if(!layer->resultcache || map->querymap.style == MS_NORMAL) 
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
    if((layer->maxscale > 0) && (map->scale > layer->maxscale)) return(MS_SUCCESS);
    if((layer->minscale > 0) && (map->scale <= layer->minscale)) return(MS_SUCCESS);

    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale)) annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale)) annotate = MS_FALSE;
  }

  // if MS_HILITE, alter the first class (always at least 1 class)
  if(map->querymap.style == MS_HILITE) {
    for(i=0; i<layer->numclasses; i++) {
      if(MS_VALID_COLOR(&(layer->class[i].styles[layer->class[i].numstyles-1].color))) {
        colorbuffer[i] = layer->class[i].styles[layer->class[i].numstyles-1].color; // save the color from the TOP style
        layer->class[i].styles[layer->class[i].numstyles-1].color = map->querymap.color;
      } else if(MS_VALID_COLOR(&(layer->class[i].styles[layer->class[i].numstyles-1].outlinecolor))) {
	colorbuffer[i] = layer->class[i].styles[layer->class[i].numstyles-1].outlinecolor; // if no color, save the outlinecolor from the TOP style
        layer->class[i].styles[layer->class[i].numstyles-1].outlinecolor = map->querymap.color;
      }
    }
  }

  // open this layer
  status = msLayerOpen(layer, map->shapepath);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  // build item list
  status = msLayerWhichItems(layer, MS_FALSE, annotate, NULL); // FIX: results have already been classified (this may change)
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
    if(layer->type == MS_LAYER_LINE && layer->class[shape.classindex].numstyles > 1) 
      cache = MS_TRUE; // only line layers with multiple styles need be cached (I don't think POLYLINE layers need caching - SDL)

    if(annotate && (layer->class[shape.classindex].text.string || layer->labelitem) && layer->class[shape.classindex].label.size != -1)
      shape.text = msShapeGetAnnotation(layer, &shape);

    if(cache)
      status = msDrawShape(map, layer, &shape, image, 0); // draw only the first style
    else
      status = msDrawShape(map, layer, &shape, image, -1); // all styles 
    if(status != MS_SUCCESS) {
      msLayerClose(layer);
      return MS_FAILURE;
    }

    if(shape.numlines == 0) { // once clipped the shape didn't need to be drawn
      msFreeShape(&shape);
      continue;
    }

    if(cache) {
      if(insertFeatureList(&shpcache, &shape) == NULL) return(MS_FAILURE); // problem adding to the cache
    }

    maxnumstyles = MS_MAX(maxnumstyles, layer->class[shape.classindex].numstyles);
    msFreeShape(&shape);
  }

  if(shpcache) {
    int s;
  
    for(s=1; s<maxnumstyles; s++) {
      for(current=shpcache; current; current=current->next) {        
        if(layer->class[current->shape.classindex].numstyles > s)
	  msDrawLineSymbol(&map->symbolset, image, &current->shape, &(layer->class[current->shape.classindex].styles[s]), layer->scalefactor);
      }
    }
    
    freeFeatureList(shpcache);
    shpcache = NULL;  
  }

  // if MS_HILITE, restore values
  if(map->querymap.style == MS_HILITE) {
    for(i=0; i<layer->numclasses; i++) {
      if(MS_VALID_COLOR(&(layer->class[i].styles[layer->class[i].numstyles-1].color)))
        layer->class[i].styles[layer->class[i].numstyles-1].color = colorbuffer[i];        
      else if(MS_VALID_COLOR(&(layer->class[i].styles[layer->class[i].numstyles-1].outlinecolor)))
	layer->class[i].styles[layer->class[i].numstyles-1].outlinecolor = colorbuffer[i]; // if no color, restore outlinecolor for the TOP style
    }
  }

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
        if( MS_RENDERER_GD(image->format) )
            return msDrawRasterLayerLow(map, layer, image);
        else if( MS_RENDERER_RAWDATA(image->format) )
            return msDrawRasterLayerLow(map, layer, image);
#ifdef USE_MING_FLASH
        else if( MS_RENDERER_SWF(image->format) )
            return  msDrawRasterLayerSWF(map, layer, image);
#endif
    }

    return MS_FAILURE;
}

/**
 * msDrawWMSLayer()
 *
 * Draw a single WMS layer.  
 * Multiple WMS layers in a map are preloaded and then drawn using
 * msDrawWMSLayerLow()
 */

#ifdef USE_WMS_LYR
int msDrawWMSLayer(mapObj *map, layerObj *layer, imageObj *image)
{
    int nStatus = MS_FAILURE;

    if (image && map && layer)
    {
/* ------------------------------------------------------------------
 * Start by downloading the layer
 * ------------------------------------------------------------------ */
        httpRequestObj asReqInfo[1];
        int numReq = 0;
        
        if ( msPrepareWMSLayerRequest(1, map, layer,
                                      asReqInfo, &numReq) == MS_FAILURE  ||
             msWMSExecuteRequests(asReqInfo, numReq) == MS_FAILURE )
        {
            return MS_FAILURE;
        }

/* ------------------------------------------------------------------
 * Then draw layer based on output format
 * ------------------------------------------------------------------ */
        if( MS_RENDERER_GD(image->format) )
            nStatus = msDrawWMSLayerLow(1, asReqInfo, numReq,
                                        map, layer, image) ;

        else if( MS_RENDERER_RAWDATA(image->format) )
            nStatus = msDrawWMSLayerLow(1, asReqInfo, numReq,
                                        map, layer, image) ;

#ifdef USE_MING_FLASH                
        else if( MS_RENDERER_SWF(image->format) )
            nStatus = msDrawWMSLayerSWF(1, asReqInfo, numReq,
                                        map, layer, image);
#endif
        else
            nStatus = MS_SUCCESS; // Should we fail if output doesn't support WMS?
        // Cleanup
        msFreeRequestObj(asReqInfo, numReq);
    }

    return nStatus;
}
#endif

/*
** Function to render an individual shape, the style variable enables/disables the drawing of a single style
** versus a single style. This is necessary when drawing entire layers as proper overlay can only be achived
** through caching. 
*/
int msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, imageObj *image, int style)
{
  int i,j,c,s;
  rectObj cliprect;
  pointObj annopnt, *point;
  double angle, length;
 
  pointObj center; // circle origin
  double r; // circle radius
  int csz; // clip over size
  
/* Steve's original code
  cliprect.minx = map->extent.minx - 2*map->cellsize; // set clipping rectangle just a bit larger than the map extent
  cliprect.miny = map->extent.miny - 2*map->cellsize;
  cliprect.maxx = map->extent.maxx + 2*map->cellsize;
  cliprect.maxy = map->extent.maxy + 2*map->cellsize;
*/

  msDrawStartShape(map, layer, image, shape);

  c = shape->classindex;
  
  csz = MS_NINT((layer->class[c].styles[0].size*layer->scalefactor)/2.0); // changed when Tomas added CARTOLINE symbols
  cliprect.minx = map->extent.minx - csz*map->cellsize;
  cliprect.miny = map->extent.miny - csz*map->cellsize;
  cliprect.maxx = map->extent.maxx + csz*map->cellsize;
  cliprect.maxy = map->extent.maxy + csz*map->cellsize;

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

    // shade symbol drawing will call outline function if color not set
    if(style != -1)
      msCircleDrawShadeSymbol(&map->symbolset, image, &center, r, &(layer->class[c].styles[style]), layer->scalefactor);
    else
      for(s=0; s<layer->class[c].numstyles; s++)
        msCircleDrawShadeSymbol(&map->symbolset, image, &center, r, &(layer->class[c].styles[s]), layer->scalefactor);

    // TO DO: need to handle circle annotation

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
	msTransformShape(shape, map->extent, map->cellsize, image);
      }

      if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) == MS_SUCCESS) {

	if(layer->labelangleitemindex != -1) layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);
	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) layer->class[c].label.size = atoi(shape->values[layer->labelsizeitemindex]);

	if(layer->class[c].label.autoangle) layer->class[c].label.angle = angle;

	if(layer->labelcache) {
	  if(msAddLabel(map, layer->index, c, shape->tileindex, shape->index, &annopnt, shape->text, length) != MS_SUCCESS) return(MS_FAILURE);
	} else {
	  if(MS_VALID_COLOR(&(layer->class[c].styles[0].color))) {
            for(s=0; s<layer->class[c].numstyles; s++)
	      msDrawMarkerSymbol(&map->symbolset, image, &annopnt, &(layer->class[c].styles[s]), layer->scalefactor);
	  }
	  msDrawLabel(image, annopnt, shape->text, &(layer->class[c].label), &map->fontset, layer->scalefactor);
	}
      }

      break;
    case(MS_SHAPE_POLYGON):

      if(layer->transform) {
	msClipPolygonRect(shape, cliprect);
	if(shape->numlines == 0) return(MS_SUCCESS);
	msTransformShape(shape, map->extent, map->cellsize, image);
      }

      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) == MS_SUCCESS) {

	if(layer->labelangleitemindex != -1) layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);
	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) layer->class[c].label.size = atoi(shape->values[layer->labelsizeitemindex]);

	if(layer->labelcache) {
	  if(msAddLabel(map, layer->index, c, shape->tileindex, shape->index, &annopnt, shape->text, length) != MS_SUCCESS) return(MS_FAILURE);
	} else {
	  if(MS_VALID_COLOR(&(layer->class[c].styles[0].color))) {
            for(s=0; s<layer->class[c].numstyles; s++)
	      msDrawMarkerSymbol(&map->symbolset, image, &annopnt, &(layer->class[c].styles[s]), layer->scalefactor);
	  }
	  msDrawLabel(image, annopnt, shape->text, &(layer->class[c].label), &map->fontset, layer->scalefactor);
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

	  if(layer->labelangleitemindex != -1) layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);
	  if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) layer->class[c].label.size = atoi(shape->values[layer->labelsizeitemindex]);   

	  if(shape->text) {
	    if(layer->labelcache) {
	      if(msAddLabel(map, layer->index, c, shape->tileindex, shape->index, point, shape->text, -1) != MS_SUCCESS) return(MS_FAILURE);
	    } else {
	      if(MS_VALID_COLOR(&(layer->class[c].styles[0].color))) {
                for(s=0; s<layer->class[c].numstyles; s++)
	          msDrawMarkerSymbol(&map->symbolset, image, point, &(layer->class[c].styles[s]), layer->scalefactor);
	      }
	      msDrawLabel(image, *point, shape->text, &layer->class[c].label, &map->fontset, layer->scalefactor);
	    }
	  }
	}
      }
    }
    break;  // end MS_LAYER_ANNOTATION

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

	for(s=0; s<layer->class[c].numstyles; s++)
  	  msDrawMarkerSymbol(&map->symbolset, image, point, &(layer->class[c].styles[s]), layer->scalefactor);

	if(shape->text) {
	  if(layer->labelangleitemindex != -1) layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);	  
	  if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) layer->class[c].label.size = atoi(shape->values[layer->labelsizeitemindex]);   

	  if(layer->labelcache) {
	    if(msAddLabel(map, layer->index, c, shape->tileindex, shape->index, point, shape->text, -1) != MS_SUCCESS) return(MS_FAILURE);
	  } else
	    msDrawLabel(image, *point, shape->text, &layer->class[c].label, &map->fontset, layer->scalefactor);
	}
      }
    }
    break; // end MS_LAYER_POINT

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
      msTransformShape(shape, map->extent, map->cellsize, image);
    }
 
    if(style != -1)
      msDrawLineSymbol(&map->symbolset, image, shape, &(layer->class[c].styles[style]), layer->scalefactor);
    else
      for(s=0; s<layer->class[c].numstyles; s++)
        msDrawLineSymbol(&map->symbolset, image, shape, &(layer->class[c].styles[s]), layer->scalefactor);

    if(shape->text) {
      if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) == MS_SUCCESS) {
	if(layer->labelangleitemindex != -1) layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);
	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) layer->class[c].label.size = atoi(shape->values[layer->labelsizeitemindex]);

	if(layer->class[c].label.autoangle) layer->class[c].label.angle = angle;

	if(layer->labelcache) {
	  if(msAddLabel(map, layer->index, c, shape->tileindex, shape->index, &annopnt, shape->text, length) != MS_SUCCESS) return(MS_FAILURE);
	} else
          msDrawLabel(image, annopnt, shape->text, &layer->class[c].label, &map->fontset, layer->scalefactor);
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
      msTransformShape(shape, map->extent, map->cellsize, image);
    }
 
    for(s=0; s<layer->class[c].numstyles; s++)
      msDrawShadeSymbol(&map->symbolset, image, shape, &(layer->class[c].styles[s]), layer->scalefactor);
    
    if(shape->text) {
      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) == MS_SUCCESS) {	
	if(layer->labelangleitemindex != -1) layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);
	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) layer->class[c].label.size = atoi(shape->values[layer->labelsizeitemindex]);

	if(layer->labelcache) {
	  if(msAddLabel(map, layer->index, c, shape->tileindex, shape->index, &annopnt, shape->text, -1) != MS_SUCCESS) return(MS_FAILURE);
	} else
	  msDrawLabel(image, annopnt, shape->text, &layer->class[c].label, &map->fontset, layer->scalefactor);
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
  int c, s;

  c = classindex;
 
#ifdef USE_PROJ
  if(msProjectionsDiffer(&(layer->projection), &(map->projection)))
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
      if(layer->labelcache) {
	if(msAddLabel(map, layer->index, c, -1, -1, point, labeltext, -1) != MS_SUCCESS) return(MS_FAILURE);
      } else {
	if(MS_VALID_COLOR(&(layer->class[c].styles[0].color))) {
          for(s=0; s<layer->class[c].numstyles; s++)
  	    msDrawMarkerSymbol(&map->symbolset, image, point, &(layer->class[c].styles[s]), layer->scalefactor);
	}
	msDrawLabel(image, *point, labeltext, &layer->class[c].label, &map->fontset, layer->scalefactor);
      }
    }
    break;

  case MS_LAYER_POINT:
    if(layer->transform) {
      if(!msPointInRect(point, &map->extent)) return(0);
      point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
      point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
    }

    for(s=0; s<layer->class[c].numstyles; s++)
      msDrawMarkerSymbol(&map->symbolset, image, point, &(layer->class[c].styles[s]), layer->scalefactor);

    if(labeltext) {
      if(layer->labelcache) {
	if(msAddLabel(map, layer->index, c, -1, -1, point, labeltext, -1) != MS_SUCCESS) return(MS_FAILURE);
      } else
	msDrawLabel(image, *point, labeltext, &layer->class[c].label, &map->fontset, layer->scalefactor);
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
void msCircleDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor)
{
    if (image)
    {
        if( MS_RENDERER_GD(image->format) )
            msCircleDrawLineSymbolGD(symbolset, image->img.gd, p, r, style, scalefactor);
        else
             msSetError(MS_MISCERR, "Unknown image type", 
                        "msCircleDrawLineSymbol()");
    }
}

void msCircleDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor)
{
    if (image)
    {
        if( MS_RENDERER_GD(image->format) )
            msCircleDrawShadeSymbolGD(symbolset, image->img.gd, p, r, style, scalefactor);
              
        else
             msSetError(MS_MISCERR, "Unknown image type", 
                        "msCircleDrawShadeSymbol()"); 
    }
}


void msDrawMarkerSymbol(symbolSetObj *symbolset,imageObj *image, pointObj *p, styleObj *style, double scalefactor)
{    
   if (image)
   {
       if( MS_RENDERER_GD(image->format) )
           msDrawMarkerSymbolGD(symbolset, image->img.gd, p, style, scalefactor);
       
#ifdef USE_MING_FLASH              
       else if( MS_RENDERER_SWF(image->format) )
           msDrawMarkerSymbolSWF(symbolset, image, p, style, scalefactor);
#endif
    }
}

void msDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor)
{
    if (image)
    {
        if( MS_RENDERER_GD(image->format) )
            msDrawLineSymbolGD(symbolset, image->img.gd, p, style, scalefactor);

#ifdef USE_MING_FLASH
        else if( MS_RENDERER_SWF(image->format) )
            msDrawLineSymbolSWF(symbolset, image, p,  style, scalefactor);
#endif
    }
}

void msDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor)
{
    if (image)
    {
        if( MS_RENDERER_GD(image->format) )
            msDrawShadeSymbolGD(symbolset, image->img.gd, p, style, scalefactor);

#ifdef USE_MING_FLASH
        else if( MS_RENDERER_SWF(image->format) )
            msDrawShadeSymbolSWF(symbolset, image, p, style, scalefactor);
#endif
    }
}

int msDrawLabel(imageObj *image, pointObj labelPnt, char *string, 
                labelObj *label, fontSetObj *fontset, double scalefactor)
{
  if(!string)
    return(0); /* not an error, just don't want to do anything */

  if(strlen(string) == 0)
    return(0); /* not an error, just don't want to do anything */


/* ======================================================================= */
/*      TODO : This is a temporary hack to call the drawlableswf directly. */
/*      Normally the only functions that should be wrapped here is         */
/*      draw_text. We did this since msGetLabelSize has not yet been       */
/*      implemented for MING FDB fonts.                                    */
/* ======================================================================= */

#ifdef USE_MING_FLASH
  if ( MS_RENDERER_SWF(image->format) )
      return msDrawLabelSWF(image, labelPnt, string, label, fontset, scalefactor);
#endif

  if(label->position != MS_XY) {
    pointObj p;
    rectObj r;

    if(msGetLabelSize(string, label, &r, fontset) == -1) return(-1);
    p = get_metrics(&labelPnt, label->position, r, label->offsetx, label->offsety, label->angle, 0, NULL);
    msDrawText(image, p, string, label, fontset, scalefactor); /* actually draw the label */
  } else {
    labelPnt.x += label->offsetx;
    labelPnt.y += label->offsety;
    msDrawText(image, labelPnt, string, label, fontset, scalefactor); /* actually draw the label */
  }

  return(0);
}


int msDrawText(imageObj *image, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor)
{
    int nReturnVal = -1;

    if (image)
    {
        if( MS_RENDERER_GD(image->format) )
            nReturnVal = msDrawTextGD(image->img.gd, labelPnt, string, 
                                     label, fontset, scalefactor);
#ifdef USE_MING_FLASH
        else if( MS_RENDERER_SWF(image->format) )
            nReturnVal = draw_textSWF(image, labelPnt, string, label, 
                                      fontset, scalefactor); 
#endif
    }

    return nReturnVal;
}

int msDrawLabelCache(imageObj *image, mapObj *map)
{
    int nReturnVal = MS_SUCCESS;
    if (image)
    {
        if( MS_RENDERER_GD(image->format) )
            nReturnVal = msDrawLabelCacheGD(image->img.gd, map);

#ifdef USE_MING_FLASH
        else if( MS_RENDERER_SWF(image->format) )
            nReturnVal = msDrawLabelCacheSWF(image, map);
#endif
    }

    return nReturnVal;
}

/**
 * Generic function to tell the underline device that layer 
 * drawing is stating
 */

void msImageStartLayer(mapObj *map, layerObj *layer, imageObj *image)
{
    if (image)
    {
#ifdef USE_MING_FLASH
        if( MS_RENDERER_SWF(image->format) )
            msImageStartLayerSWF(map, layer, image);
#endif
    }
}


/**
 * Generic function to tell the underline device that layer 
 * drawing is ending
 */

void msImageEndLayer(mapObj *map, layerObj *layer, imageObj *image)
{
}

/**
 * Generic function to tell the underline device that shape 
 * drawing is stating
 */

void msDrawStartShape(mapObj *map, layerObj *layer, imageObj *image,
                      shapeObj *shape)
{
    if (image)
    {

#ifdef USE_MING_FLASH
          if( MS_RENDERER_SWF(map->outputformat) )
            msDrawStartShapeSWF(map, layer, image, shape);
#endif
               
    }
}


/**
 * Generic function to tell the underline device that shape 
 * drawing is ending
 */

void msDrawEndShape(mapObj *map, layerObj *layer, imageObj *image, 
                    shapeObj *shape)
{
}


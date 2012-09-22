/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  High level msDrawMap() implementation and related functions.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include <assert.h>
#include <math.h>
#include "mapserver.h"
#include "maptime.h"
#include "mapcopy.h"



#ifdef USE_GD
/*
 * Functions to reset any pen (color index) values previously set. Used primarily to reset things when
 * using MapScript to create multiple images. How the pen values are set is irrelevant (definitely output
 * format type specific) which is why this function is here instead of the GD, PDF or SWF source files.
*/
void msClearLayerPenValues(layerObj *layer)
{
  int i, j, k;

  for(i=0; i<layer->numclasses; i++) {
    for(j=0; j<layer->class[i]->numlabels; j++) {
      layer->class[i]->labels[j]->color.pen = MS_PEN_UNSET; /* set in MSXXDrawText function */
      layer->class[i]->labels[j]->outlinecolor.pen = MS_PEN_UNSET;
      layer->class[i]->labels[j]->shadowcolor.pen = MS_PEN_UNSET;
      for(k=0; k<layer->class[i]->labels[j]->numstyles; k++) {
        layer->class[i]->labels[j]->styles[k]->backgroundcolor.pen = MS_PEN_UNSET; /* set in various symbol drawing functions */
        layer->class[i]->labels[j]->styles[k]->color.pen = MS_PEN_UNSET;
        layer->class[i]->labels[j]->styles[k]->outlinecolor.pen = MS_PEN_UNSET;
      }
    }

    for(j=0; j<layer->class[i]->numstyles; j++) {
      layer->class[i]->styles[j]->backgroundcolor.pen = MS_PEN_UNSET; /* set in various symbol drawing functions */
      layer->class[i]->styles[j]->color.pen = MS_PEN_UNSET;
      layer->class[i]->styles[j]->outlinecolor.pen = MS_PEN_UNSET;
    }
  }
}

void msClearScalebarPenValues(scalebarObj *scalebar)
{
  if (scalebar) {
    scalebar->color.pen = MS_PEN_UNSET;
    scalebar->backgroundcolor.pen = MS_PEN_UNSET;
    scalebar->outlinecolor.pen = MS_PEN_UNSET;
    scalebar->imagecolor.pen = MS_PEN_UNSET;

    scalebar->label.color.pen = MS_PEN_UNSET;
    scalebar->label.outlinecolor.pen = MS_PEN_UNSET;
    scalebar->label.shadowcolor.pen = MS_PEN_UNSET;
    /* TODO: deal with label styles here */
  }
}

void msClearLegendPenValues(legendObj *legend)
{
  if (legend) {
    legend->outlinecolor.pen = MS_PEN_UNSET;
    legend->imagecolor.pen = MS_PEN_UNSET;

    legend->label.color.pen = MS_PEN_UNSET;
    legend->label.outlinecolor.pen = MS_PEN_UNSET;
    legend->label.shadowcolor.pen = MS_PEN_UNSET;
    /* TODO: deal with label styles here */
  }
}

void msClearReferenceMapPenValues(referenceMapObj *referencemap)
{
  if (referencemap) {
    referencemap->outlinecolor.pen = MS_PEN_UNSET;
    referencemap->color.pen = MS_PEN_UNSET;
  }
}


void msClearQueryMapPenValues(queryMapObj *querymap)
{
  if (querymap)
    querymap->color.pen = MS_PEN_UNSET;
}


void msClearPenValues(mapObj *map)
{
  int i;

  for(i=0; i<map->numlayers; i++)
    msClearLayerPenValues((GET_LAYER(map, i)));

  msClearLegendPenValues(&(map->legend));
  msClearScalebarPenValues(&(map->scalebar));
  msClearReferenceMapPenValues(&(map->reference));
  msClearQueryMapPenValues(&(map->querymap));

}
#endif

/* msPrepareImage()
 *
 * Returns a new imageObj ready for rendering the current map.
 *
 * If allow_nonsquare is set to MS_TRUE then the caller should call
 * msMapRestoreRealExtent() once they are done with the image.
 * This should be set to MS_TRUE only when called from msDrawMap(), see bug 945.
 */
imageObj *msPrepareImage(mapObj *map, int allow_nonsquare)
{
  int i, status;
  imageObj *image=NULL;
  double geo_cellsize;

  if(map->width == -1 || map->height == -1) {
    msSetError(MS_MISCERR, "Image dimensions not specified.", "msPrepareImage()");
    return(NULL);
  }

  msInitLabelCache(&(map->labelcache)); /* this clears any previously allocated cache */

  /* clear any previously created mask layer images */
  for(i=0; i<map->numlayers; i++) {
    if(GET_LAYER(map, i)->maskimage) {
      msFreeImage(GET_LAYER(map, i)->maskimage);
      GET_LAYER(map, i)->maskimage = NULL;
    }
  }

  status = msValidateContexts(map); /* make sure there are no recursive REQUIRES or LABELREQUIRES expressions */
  if(status != MS_SUCCESS) return NULL;

  if(!map->outputformat) {
    msSetError(MS_GDERR, "Map outputformat not set!", "msPrepareImage()");
    return(NULL);
  } else if (MS_RENDERER_PLUGIN(map->outputformat)) {
    rendererVTableObj *renderer = map->outputformat->vtable;
    colorObj *bg = &map->imagecolor;
    map->imagecolor.alpha=255;
    if(map->transparent == MS_TRUE) {
      /* don't set the image color */
      bg = NULL;
    }
    image = renderer->createImage(map->width, map->height, map->outputformat,bg);
    if (image == NULL)
      return(NULL);
    image->format = map->outputformat;
    image->format->refcount++;
    image->width = map->width;
    image->height = map->height;

    image->resolution = map->resolution;
    image->resolutionfactor = map->resolution/map->defresolution;
    if (map->web.imagepath)
      image->imagepath = msStrdup(map->web.imagepath);
    if (map->web.imageurl)
      image->imageurl = msStrdup(map->web.imageurl);

  } else if( MS_RENDERER_IMAGEMAP(map->outputformat) ) {
    image = msImageCreateIM(map->width, map->height, map->outputformat,
                            map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution);
  } else if( MS_RENDERER_RAWDATA(map->outputformat) ) {
    image = msImageCreate(map->width, map->height, map->outputformat,
                          map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution, &map->imagecolor);
  } else {
    image = NULL;
  }

  if(!image) {
    msSetError(MS_GDERR, "Unable to initialize image.", "msPrepareImage()");
    return(NULL);
  }

  /*
   * If we want to support nonsquare pixels, note that now, otherwise
   * adjust the extent size to have square pixels.
   *
   * If allow_nonsquare is set to MS_TRUE then the caller should call
   * msMapRestoreRealExtent() once they are done with the image.
   * This should be set to MS_TRUE only when called from msDrawMap(), see bug 945.
   */
  if( allow_nonsquare && msTestConfigOption( map, "MS_NONSQUARE", MS_FALSE ) ) {
    double cellsize_x = (map->extent.maxx - map->extent.minx)/map->width;
    double cellsize_y = (map->extent.maxy - map->extent.miny)/map->height;

    if( cellsize_y != 0.0
        && (fabs(cellsize_x/cellsize_y) > 1.00001
            || fabs(cellsize_x/cellsize_y) < 0.99999) ) {
      map->gt.need_geotransform = MS_TRUE;
      if (map->debug)
        msDebug( "msDrawMap(): kicking into non-square pixel preserving mode.\n" );
    }
    map->cellsize = (cellsize_x*0.5 + cellsize_y*0.5);
  } else
    map->cellsize = msAdjustExtent(&(map->extent),map->width,map->height);

  status = msCalculateScale(map->extent,map->units,map->width,map->height, map->resolution, &map->scaledenom);
  if(status != MS_SUCCESS) {
    msFreeImage(image);
    return(NULL);
  }

  /* update geotransform based on adjusted extent. */
  msMapComputeGeotransform( map );

  /* Do we need to fake out stuff for rotated support? */
  if( map->gt.need_geotransform )
    msMapSetFakedExtent( map );

  /* We will need a cellsize that represents a real georeferenced */
  /* coordinate cellsize here, so compute it from saved extents.   */

  geo_cellsize = map->cellsize;
  if( map->gt.need_geotransform == MS_TRUE ) {
    double cellsize_x = (map->saved_extent.maxx - map->saved_extent.minx)
                        / map->width;
    double cellsize_y = (map->saved_extent.maxy - map->saved_extent.miny)
                        / map->height;

    geo_cellsize = sqrt(cellsize_x*cellsize_x + cellsize_y*cellsize_y)
                   / sqrt(2.0);
  }

  /* compute layer scale factors now */
  for(i=0; i<map->numlayers; i++) {
    if(GET_LAYER(map, i)->sizeunits != MS_PIXELS)
      GET_LAYER(map, i)->scalefactor = (msInchesPerUnit(GET_LAYER(map, i)->sizeunits,0)/msInchesPerUnit(map->units,0)) / geo_cellsize;
    else if(GET_LAYER(map, i)->symbolscaledenom > 0 && map->scaledenom > 0)
      GET_LAYER(map, i)->scalefactor = GET_LAYER(map, i)->symbolscaledenom/map->scaledenom*map->resolution/map->defresolution;
    else
      GET_LAYER(map, i)->scalefactor = map->resolution/map->defresolution;
  }

  image->refpt.x = MS_MAP2IMAGE_X_IC_DBL(0, map->extent.minx, 1.0/map->cellsize);
  image->refpt.y = MS_MAP2IMAGE_Y_IC_DBL(0, map->extent.maxy, 1.0/map->cellsize);

  return image;
}


/*
 * Generic function to render the map file.
 * The type of the image created is based on the imagetype parameter in the map file.
 *
 * mapObj *map - map object loaded in MapScript or via a mapfile to use
 * int querymap - is this map the result of a query operation, MS_TRUE|MS_FALSE
*/
imageObj *msDrawMap(mapObj *map, int querymap)
{
  int i;
  layerObj *lp=NULL;
  int status = MS_FAILURE;
  imageObj *image = NULL;
  struct mstimeval mapstarttime, mapendtime;
  struct mstimeval starttime, endtime;

#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
  enum MS_CONNECTION_TYPE lastconnectiontype;
  httpRequestObj *pasOWSReqInfo=NULL;
  int numOWSLayers=0, numOWSRequests=0;
  wmsParamsObj sLastWMSParams;
#endif

  if(map->debug >= MS_DEBUGLEVEL_TUNING) msGettimeofday(&mapstarttime, NULL);

  if(querymap) { /* use queryMapObj image dimensions */
    if(map->querymap.width != -1) map->width = map->querymap.width;
    if(map->querymap.height != -1) map->height = map->querymap.height;
  }

  msApplyMapConfigOptions(map);
  image = msPrepareImage(map, MS_TRUE);

  if(!image) {
    msSetError(MS_IMGERR, "Unable to initialize image.", "msDrawMap()");
    return(NULL);
  }

  if( map->debug >= MS_DEBUGLEVEL_DEBUG )
    msDebug( "msDrawMap(): rendering using outputformat named %s (%s).\n",
             map->outputformat->name,
             map->outputformat->driver );

#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

  /* Time the OWS query phase */
  if(map->debug >= MS_DEBUGLEVEL_TUNING ) msGettimeofday(&starttime, NULL);

  /* How many OWS (WMS/WFS) layers do we have to draw?
   * Note: numOWSLayers is the number of actual layers and numOWSRequests is
   * the number of HTTP requests which could be lower if multiple layers
   * are merged into the same request.
   */
  numOWSLayers=0;
  for(i=0; i<map->numlayers; i++) {
    if(map->layerorder[i] != -1 &&
        msLayerIsVisible(map, GET_LAYER(map,map->layerorder[i])))
      numOWSLayers++;
  }


  if (numOWSLayers > 0) {
    /* Alloc and init pasOWSReqInfo...
     */
    pasOWSReqInfo = (httpRequestObj *)malloc((numOWSLayers+1)*sizeof(httpRequestObj));
    if (pasOWSReqInfo == NULL) {
      msSetError(MS_MEMERR, "Allocation of httpRequestObj failed.", "msDrawMap()");
      return NULL;
    }
    msHTTPInitRequestObj(pasOWSReqInfo, numOWSLayers+1);
    msInitWmsParamsObj(&sLastWMSParams);

    /* Pre-download all WMS/WFS layers in parallel before starting to draw map */
    lastconnectiontype = MS_SHAPEFILE;
    for(i=0; numOWSLayers && i<map->numlayers; i++) {
      if(map->layerorder[i] == -1 || !msLayerIsVisible(map, GET_LAYER(map,map->layerorder[i])))
        continue;

      lp = GET_LAYER(map,map->layerorder[i]);

#ifdef USE_WMS_LYR
      if(lp->connectiontype == MS_WMS) {
        if(msPrepareWMSLayerRequest(map->layerorder[i], map, lp, 1, lastconnectiontype, &sLastWMSParams, 0, 0, 0, NULL, pasOWSReqInfo, &numOWSRequests) == MS_FAILURE) {
          msFreeWmsParamsObj(&sLastWMSParams);
          msFreeImage(image);
          msFree(pasOWSReqInfo);
          return NULL;
        }
      }
#endif

#ifdef USE_WFS_LYR
      if(lp->connectiontype == MS_WFS) {
        if(msPrepareWFSLayerRequest(map->layerorder[i], map, lp, pasOWSReqInfo, &numOWSRequests) == MS_FAILURE) {
          msFreeWmsParamsObj(&sLastWMSParams);
          msFreeImage(image);
          msFree(pasOWSReqInfo);
          return NULL;
        }
      }
#endif

      lastconnectiontype = lp->connectiontype;
    }

#ifdef USE_WMS_LYR
    msFreeWmsParamsObj(&sLastWMSParams);
#endif
  } /* if numOWSLayers > 0 */

  if(numOWSRequests && msOWSExecuteRequests(pasOWSReqInfo, numOWSRequests, map, MS_TRUE) == MS_FAILURE) {
    msFreeImage(image);
    msFree(pasOWSReqInfo);
    return NULL;
  }

  if(map->debug >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&endtime, NULL);
    msDebug("msDrawMap(): WMS/WFS set-up and query, %.3fs\n",
            (endtime.tv_sec+endtime.tv_usec/1.0e6)-
            (starttime.tv_sec+starttime.tv_usec/1.0e6) );
  }

#endif /* USE_WMS_LYR || USE_WFS_LYR */

  /* OK, now we can start drawing */
  for(i=0; i<map->numlayers; i++) {

    if(map->layerorder[i] != -1) {
      lp = (GET_LAYER(map,  map->layerorder[i]));

      if(lp->postlabelcache) /* wait to draw */
        continue;

      if(map->debug >= MS_DEBUGLEVEL_TUNING || lp->debug >= MS_DEBUGLEVEL_TUNING ) msGettimeofday(&starttime, NULL);

      if(!msLayerIsVisible(map, lp)) continue;

      if(lp->connectiontype == MS_WMS) {
#ifdef USE_WMS_LYR
        if(MS_RENDERER_PLUGIN(image->format) || MS_RENDERER_RAWDATA(image->format))
          status = msDrawWMSLayerLow(map->layerorder[i], pasOWSReqInfo, numOWSRequests,  map, lp, image);
        else {
          msSetError(MS_WMSCONNERR, "Output format '%s' doesn't support WMS layers.", "msDrawMap()", image->format->name);
          status = MS_FAILURE;
        }

        if(status == MS_FAILURE) {
          msSetError(MS_WMSCONNERR,
                     "Failed to draw WMS layer named '%s'. This most likely happened because "
                     "the remote WMS server returned an invalid image, and XML exception "
                     "or another unexpected result in response to the GetMap request. Also check "
                     "and make sure that the layer's connection URL is valid.",
                     "msDrawMap()", lp->name);
          msFreeImage(image);
          msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
          msFree(pasOWSReqInfo);
          return(NULL);
        }


#else /* ndef USE_WMS_LYR */
        msSetError(MS_WMSCONNERR, "MapServer not built with WMS Client support, unable to render layer '%s'.", "msDrawMap()", lp->name);
        msFreeImage(image);
        return(NULL);
#endif
      } else { /* Default case: anything but WMS layers */
        if(querymap)
          status = msDrawQueryLayer(map, lp, image);
        else
          status = msDrawLayer(map, lp, image);
        if(status == MS_FAILURE) {
          msSetError(MS_IMGERR, "Failed to draw layer named '%s'.", "msDrawMap()", lp->name);
          msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
          if (pasOWSReqInfo) {
            msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
            msFree(pasOWSReqInfo);
          }
#endif /* USE_WMS_LYR || USE_WFS_LYR */
          return(NULL);
        }
      }
    }

    if(map->debug >= MS_DEBUGLEVEL_TUNING || lp->debug >= MS_DEBUGLEVEL_TUNING) {
      msGettimeofday(&endtime, NULL);
      msDebug("msDrawMap(): Layer %d (%s), %.3fs\n",
              map->layerorder[i], lp->name?lp->name:"(null)",
              (endtime.tv_sec+endtime.tv_usec/1.0e6)-
              (starttime.tv_sec+starttime.tv_usec/1.0e6) );
    }
  }

  if(map->scalebar.status == MS_EMBED && !map->scalebar.postlabelcache) {

    /* We need to temporarily restore the original extent for drawing */
    /* the scalebar as it uses the extent to recompute cellsize. */
    if(map->gt.need_geotransform)
      msMapRestoreRealExtent(map);


    if(MS_SUCCESS != msEmbedScalebar(map, image)) {
      msFreeImage( image );
      return NULL;
    }


    if(map->gt.need_geotransform)
      msMapSetFakedExtent(map);
  }

  if(map->legend.status == MS_EMBED && !map->legend.postlabelcache) {
    if( msEmbedLegend(map, image) != MS_SUCCESS ) {
      msFreeImage( image );
      return NULL;
    }
  }

  if(map->debug >= MS_DEBUGLEVEL_TUNING) msGettimeofday(&starttime, NULL);

  if(msDrawLabelCache(image, map) != MS_SUCCESS) {
    msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
    if (pasOWSReqInfo) {
      msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
      msFree(pasOWSReqInfo);
    }
#endif /* USE_WMS_LYR || USE_WFS_LYR */
    return(NULL);
  }

  if(map->debug >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&endtime, NULL);
    msDebug("msDrawMap(): Drawing Label Cache, %.3fs\n",
            (endtime.tv_sec+endtime.tv_usec/1.0e6)-
            (starttime.tv_sec+starttime.tv_usec/1.0e6) );
  }

  for(i=0; i<map->numlayers; i++) { /* for each layer, check for postlabelcache layers */

    lp = (GET_LAYER(map, map->layerorder[i]));

    if(!lp->postlabelcache) continue;
    if(!msLayerIsVisible(map, lp)) continue;

    if(map->debug >= MS_DEBUGLEVEL_TUNING || lp->debug >= MS_DEBUGLEVEL_TUNING) msGettimeofday(&starttime, NULL);

    if(lp->connectiontype == MS_WMS) {
#ifdef USE_WMS_LYR
      if(MS_RENDERER_PLUGIN(image->format) || MS_RENDERER_RAWDATA(image->format))
        status = msDrawWMSLayerLow(map->layerorder[i], pasOWSReqInfo, numOWSRequests, map, lp, image);

#else
      status = MS_FAILURE;
#endif
    } else {
      if(querymap)
        status = msDrawQueryLayer(map, lp, image);
      else
        status = msDrawLayer(map, lp, image);
    }

    if(status == MS_FAILURE) {
      msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
      if (pasOWSReqInfo) {
        msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
        msFree(pasOWSReqInfo);
      }
#endif /* USE_WMS_LYR || USE_WFS_LYR */
      return(NULL);
    }

    if(map->debug >= MS_DEBUGLEVEL_TUNING || lp->debug >= MS_DEBUGLEVEL_TUNING) {
      msGettimeofday(&endtime, NULL);
      msDebug("msDrawMap(): Layer %d (%s), %.3fs\n",
              map->layerorder[i], lp->name?lp->name:"(null)",
              (endtime.tv_sec+endtime.tv_usec/1.0e6)-
              (starttime.tv_sec+starttime.tv_usec/1.0e6) );
    }

  }

  /* Do we need to fake out stuff for rotated support? */
  /* This really needs to be done on every preceeding exit point too... */
  if(map->gt.need_geotransform)
    msMapRestoreRealExtent(map);

  if(map->legend.status == MS_EMBED && map->legend.postlabelcache)
    msEmbedLegend(map, image); /* TODO */

  if(map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache) {

    /* We need to temporarily restore the original extent for drawing */
    /* the scalebar as it uses the extent to recompute cellsize. */
    if(map->gt.need_geotransform)
      msMapRestoreRealExtent(map);


    if(MS_SUCCESS != msEmbedScalebar(map, image)) {
      msFreeImage( image );
      return NULL;
    }


    if(map->gt.need_geotransform)
      msMapSetFakedExtent(map);
  }

#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
  /* Cleanup WMS/WFS Request stuff */
  if (pasOWSReqInfo) {
    msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
    msFree(pasOWSReqInfo);
  }
#endif

  if(map->debug >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&mapendtime, NULL);
    msDebug("msDrawMap() total time: %.3fs\n",
            (mapendtime.tv_sec+mapendtime.tv_usec/1.0e6)-
            (mapstarttime.tv_sec+mapstarttime.tv_usec/1.0e6) );
  }

  return(image);
}

/*
 * Test whether a layer should be drawn or not in the current map view and
 * at the current scale.
 * Returns TRUE if layer is visible, FALSE if not.
*/
int msLayerIsVisible(mapObj *map, layerObj *layer)
{
  int i;

  if(!layer->data && !layer->tileindex && !layer->connection && !layer->features && !layer->layerinfo)
    return(MS_FALSE); /* no data associated with this layer, not an error since layer may be used as a template from MapScript */

  if(layer->type == MS_LAYER_QUERY || layer->type == MS_LAYER_TILEINDEX) return(MS_FALSE);
  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) return(MS_FALSE);

  /* Only return MS_FALSE if it is definitely false. Sometimes it will return MS_UNKNOWN, which we
  ** consider true, for this use case (it might be visible, try and draw it, see what happens). */
  if ( msExtentsOverlap(map, layer) == MS_FALSE ) {
    if( layer->debug >= MS_DEBUGLEVEL_V ) {
      msDebug("msLayerIsVisible(): Skipping layer (%s) because LAYER.EXTENT does not intersect MAP.EXTENT\n", layer->name);
    }
    return(MS_FALSE);
  }

  if(msEvalContext(map, layer, layer->requires) == MS_FALSE) return(MS_FALSE);

  if(map->scaledenom > 0) {

    /* layer scale boundaries should be checked first */
    if((layer->maxscaledenom > 0) && (map->scaledenom > layer->maxscaledenom)) {
      if( layer->debug >= MS_DEBUGLEVEL_V ) {
        msDebug("msLayerIsVisible(): Skipping layer (%s) because LAYER.MAXSCALE is too small for this MAP scale\n", layer->name);
      }
      return(MS_FALSE);
    }
    if((layer->minscaledenom > 0) && (map->scaledenom <= layer->minscaledenom)) {
      if( layer->debug >= MS_DEBUGLEVEL_V ) {
        msDebug("msLayerIsVisible(): Skipping layer (%s) because LAYER.MINSCALE is too large for this MAP scale\n", layer->name);
      }
      return(MS_FALSE);
    }

    /* now check class scale boundaries (all layers *must* pass these tests) */
    if(layer->numclasses > 0) {
      for(i=0; i<layer->numclasses; i++) {
        if((layer->class[i]->maxscaledenom > 0) && (map->scaledenom > layer->class[i]->maxscaledenom))
          continue; /* can skip this one, next class */
        if((layer->class[i]->minscaledenom > 0) && (map->scaledenom <= layer->class[i]->minscaledenom))
          continue; /* can skip this one, next class */

        break; /* can't skip this class (or layer for that matter) */
      }
      if(i == layer->numclasses) {
        if( layer->debug >= MS_DEBUGLEVEL_V ) {
          msDebug("msLayerIsVisible(): Skipping layer (%s) because no CLASS in the layer is in-scale for this MAP scale\n", layer->name);
        }
        return(MS_FALSE);
      }
    }

  }

  if (layer->maxscaledenom <= 0 && layer->minscaledenom <= 0) {
    if((layer->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > layer->maxgeowidth)) {
      if( layer->debug >= MS_DEBUGLEVEL_V ) {
        msDebug("msLayerIsVisible(): Skipping layer (%s) because LAYER width is much smaller than map width\n", layer->name);
      }
      return(MS_FALSE);
    }
    if((layer->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < layer->mingeowidth)) {
      if( layer->debug >= MS_DEBUGLEVEL_V ) {
        msDebug("msLayerIsVisible(): Skipping layer (%s) because LAYER width is much larger than map width\n", layer->name);
      }
      return(MS_FALSE);
    }
  }

  return MS_TRUE;  /* All tests passed.  Layer is visible. */
}
/*
 * Generic function to render a layer object.
*/
int msDrawLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  imageObj *image_draw = image;
  outputFormatObj *altFormat=NULL;
  int retcode=MS_SUCCESS;
  int originalopacity = layer->opacity;
  const char *alternativeFomatString = NULL;
  layerObj *maskLayer = NULL;

  if(!msLayerIsVisible(map, layer))
    return MS_SUCCESS;

  if(layer->opacity == 0) return MS_SUCCESS; /* layer is completely transparent, skip it */

  /* conditions may have changed since this layer last drawn, so set
     layer->project true to recheck projection needs (Bug #673) */
  layer->project = MS_TRUE;

  if(layer->mask) {
    int maskLayerIdx;
    /* render the mask layer in its own imageObj */
    if(!MS_IMAGE_RENDERER(image)->supports_pixel_buffer) {
      msSetError(MS_MISCERR, "Layer (%s) references references a mask layer, but the selected renderer does not support them", "msDrawLayer()",
                 layer->name);
      return (MS_FAILURE);
    }
    maskLayerIdx = msGetLayerIndex(map,layer->mask);
    if(maskLayerIdx == -1) {
      msSetError(MS_MISCERR, "Layer (%s) references unknown mask layer (%s)", "msDrawLayer()",
                 layer->name,layer->mask);
      return (MS_FAILURE);
    }
    maskLayer = GET_LAYER(map, maskLayerIdx);
    if(!maskLayer->maskimage) {
      int i;
      int origstatus, origlabelcache;
      altFormat =  msSelectOutputFormat(map, "png24");
      msInitializeRendererVTable(altFormat);
      /* TODO: check the png24 format hasn't been tampered with, i.e. it's agg */
      maskLayer->maskimage= msImageCreate(image->width, image->height,altFormat,
                                          image->imagepath, image->imageurl, map->resolution, map->defresolution, NULL);
      if (!maskLayer->maskimage) {
        msSetError(MS_MISCERR, "Unable to initialize mask image.", "msDrawLayer()");
        return (MS_FAILURE);
      }

      /*
       * force the masked layer to status on, and turn off the labelcache so that
       * eventual labels are added to the temporary image instead of being added
       * to the labelcache
       */
      origstatus = maskLayer->status;
      origlabelcache = maskLayer->labelcache;
      maskLayer->status = MS_ON;
      maskLayer->labelcache = MS_OFF;

      /* draw the mask layer in the temporary image */
      retcode = msDrawLayer(map, maskLayer, maskLayer->maskimage);
      maskLayer->status = origstatus;
      maskLayer->labelcache = origlabelcache;
      if(retcode != MS_SUCCESS) {
        return MS_FAILURE;
      }
      /*
       * hack to work around bug #3834: if we have use an alternate renderer, the symbolset may contain
       * symbols that reference it. We want to remove those references before the altFormat is destroyed
       * to avoid a segfault and/or a leak, and so the the main renderer doesn't pick the cache up thinking
       * it's for him.
       */
      for(i=0; i<map->symbolset.numsymbols; i++) {
        if (map->symbolset.symbol[i]!=NULL) {
          symbolObj *s = map->symbolset.symbol[i];
          if(s->renderer == MS_IMAGE_RENDERER(maskLayer->maskimage)) {
            MS_IMAGE_RENDERER(maskLayer->maskimage)->freeSymbol(s);
            s->renderer = NULL;
          }
        }
      }
      /* set the imagetype from the original outputformat back (it was removed by msSelectOutputFormat() */
      msFree(map->imagetype);
      map->imagetype = msStrdup(image->format->name);
    }

  }
  altFormat = NULL;
  /* inform the rendering device that layer draw is starting. */
  msImageStartLayer(map, layer, image);


  /*check if an alternative renderer should be used for this layer*/
  alternativeFomatString = msLayerGetProcessingKey( layer, "RENDERER");
  if (MS_RENDERER_PLUGIN(image_draw->format) && alternativeFomatString!=NULL &&
      (altFormat=  msSelectOutputFormat(map, alternativeFomatString))) {
    rendererVTableObj *renderer=NULL;
    msInitializeRendererVTable(altFormat);

    image_draw = msImageCreate(image->width, image->height,
                               altFormat, image->imagepath, image->imageurl, map->resolution, map->defresolution, &map->imagecolor);
    renderer = MS_IMAGE_RENDERER(image_draw);
    renderer->startLayer(image_draw,map,layer);
  } else if (MS_RENDERER_PLUGIN(image_draw->format)) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image_draw);
    if (layer->mask || (layer->opacity > 0 && layer->opacity < 100)) {
      if (!renderer->supports_transparent_layers) {
        image_draw = msImageCreate(image->width, image->height,
                                   image->format, image->imagepath, image->imageurl, map->resolution, map->defresolution, NULL);
        if (!image_draw) {
          msSetError(MS_MISCERR, "Unable to initialize temporary transparent image.",
                     "msDrawLayer()");
          return (MS_FAILURE);
        }
        /* set opacity to full, as the renderer should be rendering a fully opaque image */
        layer->opacity=100;
        renderer->startLayer(image_draw,map,layer);
      }
    }
  }
  /*
  ** redirect procesing of some layer types.
  */
  if(layer->connectiontype == MS_WMS) {
#ifdef USE_WMS_LYR
    retcode = msDrawWMSLayer(map, layer, image_draw);
#else
    retcode = MS_FAILURE;
#endif
  } else if(layer->type == MS_LAYER_RASTER) {
    retcode = msDrawRasterLayer(map, layer, image_draw);
  } else if(layer->type == MS_LAYER_CHART) {
    retcode = msDrawChartLayer(map, layer, image_draw);
  } else {   /* must be a Vector layer */
    retcode = msDrawVectorLayer(map, layer, image_draw);
  }

  if (altFormat) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image);
    rendererVTableObj *altrenderer = MS_IMAGE_RENDERER(image_draw);
    rasterBufferObj rb;
    int i;
    memset(&rb,0,sizeof(rasterBufferObj));

    altrenderer->endLayer(image_draw,map,layer);

    altrenderer->getRasterBufferHandle(image_draw,&rb);
    renderer->mergeRasterBuffer(image,&rb,layer->opacity*0.01,0,0,0,0,rb.width,rb.height);

    /*
     * hack to work around bug #3834: if we have use an alternate renderer, the symbolset may contain
     * symbols that reference it. We want to remove those references before the altFormat is destroyed
     * to avoid a segfault and/or a leak, and so the the main renderer doesn't pick the cache up thinking
     * it's for him.
     */
    for(i=0; i<map->symbolset.numsymbols; i++) {
      if (map->symbolset.symbol[i]!=NULL) {
        symbolObj *s = map->symbolset.symbol[i];
        if(s->renderer == altrenderer) {
          altrenderer->freeSymbol(s);
          s->renderer = NULL;
        }
      }
    }
    msFreeImage(image_draw);

    /* set the imagetype from the original outputformat back (it was removed by msSelectOutputFormat() */
    msFree(map->imagetype);
    map->imagetype = msStrdup(image->format->name);
  } else if( image != image_draw) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image_draw);
    rasterBufferObj rb;
    memset(&rb,0,sizeof(rasterBufferObj));

    renderer->endLayer(image_draw,map,layer);
    layer->opacity = originalopacity;

    renderer->getRasterBufferHandle(image_draw,&rb);
    if(maskLayer && maskLayer->maskimage) {
      rasterBufferObj mask;
      unsigned int row,col;
      memset(&mask,0,sizeof(rasterBufferObj));
      MS_IMAGE_RENDERER(maskLayer->maskimage)->getRasterBufferHandle(maskLayer->maskimage,&mask);
      /* modify the pixels of the overlay */

      if(rb.type == MS_BUFFER_BYTE_RGBA) {
        for(row=0; row<rb.height; row++) {
          unsigned char *ma,*a,*r,*g,*b;
          r=rb.data.rgba.r+row*rb.data.rgba.row_step;
          g=rb.data.rgba.g+row*rb.data.rgba.row_step;
          b=rb.data.rgba.b+row*rb.data.rgba.row_step;
          a=rb.data.rgba.a+row*rb.data.rgba.row_step;
          ma=mask.data.rgba.a+row*mask.data.rgba.row_step;
          for(col=0; col<rb.width; col++) {
            if(*ma == 0) {
              *a = *r = *g = *b = 0;
            }
            a+=rb.data.rgba.pixel_step;
            r+=rb.data.rgba.pixel_step;
            g+=rb.data.rgba.pixel_step;
            b+=rb.data.rgba.pixel_step;
            ma+=mask.data.rgba.pixel_step;
          }
        }
#ifdef USE_GD
      } else if(rb.type == MS_BUFFER_GD) {
        for(row=0; row<rb.height; row++) {
          unsigned char *ma;
          ma=mask.data.rgba.a+row*mask.data.rgba.row_step;
          for(col=0; col<rb.width; col++) {
            if(*ma == 0) {
              gdImageSetPixel(rb.data.gd_img,col,row,0);
            }
            ma+=mask.data.rgba.pixel_step;
          }
        }
#endif
      }
    }
    renderer->mergeRasterBuffer(image,&rb,layer->opacity*0.01,0,0,0,0,rb.width,rb.height);
    msFreeImage(image_draw);
  }

  msImageEndLayer(map,layer,image);
  return(retcode);
}

int msDrawVectorLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  int         status, retcode=MS_SUCCESS;
  int         drawmode=MS_DRAWMODE_FEATURES;
  char        annotate=MS_TRUE;
  shapeObj    shape;
  rectObj     searchrect;
  char        cache=MS_FALSE;
  int         maxnumstyles=1;
  featureListNodeObjPtr shpcache=NULL, current=NULL;
  int nclasses = 0;
  int *classgroup = NULL;
  double minfeaturesize = -1;
  int maxfeatures=-1;
  int featuresdrawn=0;

  if (image)
    maxfeatures=msLayerGetMaxFeaturesToDraw(layer, image->format);

  /* TODO TBT: draw as raster layer in vector renderers */

  annotate = msEvalContext(map, layer, layer->labelrequires);
  if(map->scaledenom > 0) {
    if((layer->labelmaxscaledenom != -1) && (map->scaledenom >= layer->labelmaxscaledenom)) annotate = MS_FALSE;
    if((layer->labelminscaledenom != -1) && (map->scaledenom < layer->labelminscaledenom)) annotate = MS_FALSE;
  }

#ifdef USE_GD
  /* reset layer pen values just in case the map has been previously processed */
  msClearLayerPenValues(layer);
#endif

  /* open this layer */
  status = msLayerOpen(layer);
  if(status != MS_SUCCESS) return MS_FAILURE;

  /* build item list */
  status = msLayerWhichItems(layer, MS_FALSE, NULL);

  if(status != MS_SUCCESS) {
    msLayerClose(layer);
    return MS_FAILURE;
  }

  /* identify target shapes */
  if(layer->transform == MS_TRUE)
    searchrect = map->extent;
  else {
    searchrect.minx = searchrect.miny = 0;
    searchrect.maxx = map->width-1;
    searchrect.maxy = map->height-1;
  }

#ifdef USE_PROJ
  if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
    msProjectRect(&map->projection, &layer->projection, &searchrect); /* project the searchrect to source coords */
#endif

  status = msLayerWhichShapes(layer, searchrect, MS_FALSE);
  if(status == MS_DONE) { /* no overlap */
    msLayerClose(layer);
    return MS_SUCCESS;
  } else if(status != MS_SUCCESS) {
    msLayerClose(layer);
    return MS_FAILURE;
  }

  /* step through the target shapes */
  msInitShape(&shape);

  nclasses = 0;
  classgroup = NULL;
  if(layer->classgroup && layer->numclasses > 0)
    classgroup = msAllocateValidClassGroups(layer, &nclasses);

  if(layer->minfeaturesize > 0)
    minfeaturesize = Pix2LayerGeoref(map, layer, layer->minfeaturesize);

  while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {

    /* Check if the shape size is ok to be drawn */
    if((shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) && (minfeaturesize > 0) && (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE)) {
      if(layer->debug >= MS_DEBUGLEVEL_V)
        msDebug("msDrawVectorLayer(): Skipping shape (%d) because LAYER::MINFEATURESIZE is bigger than shape size\n", shape.index);
      msFreeShape(&shape);
      continue;
    }

    shape.classindex = msShapeGetClass(layer, map, &shape, classgroup, nclasses);
    if((shape.classindex == -1) || (layer->class[shape.classindex]->status == MS_OFF)) {
      msFreeShape(&shape);
      continue;
    }

    if(maxfeatures >=0 && featuresdrawn >= maxfeatures) {
      status = MS_DONE;
      break;
    }
    featuresdrawn++;

    cache = MS_FALSE;
    if(layer->type == MS_LAYER_LINE && (layer->class[shape.classindex]->numstyles > 1 || (layer->class[shape.classindex]->numstyles == 1 && layer->class[shape.classindex]->styles[0]->outlinewidth > 0))) {
      int i;
      cache = MS_TRUE; /* only line layers with multiple styles need be cached (I don't think POLYLINE layers need caching - SDL) */

      /* we can't handle caching with attribute binding other than for the first style (#3976) */
      for(i=1; i<layer->class[shape.classindex]->numstyles; i++) {
        if(layer->class[shape.classindex]->styles[i]->numbindings > 0) cache = MS_FALSE;
      }
    }

    /* With 'STYLEITEM AUTO', we will have the datasource fill the class' */
    /* style parameters for this shape. */
    if(layer->styleitem) {
      if(strcasecmp(layer->styleitem, "AUTO") == 0) {
        if(msLayerGetAutoStyle(map, layer, layer->class[shape.classindex], &shape) != MS_SUCCESS) {
          retcode = MS_FAILURE;
          break;
        }
      } else {
        /* Generic feature style handling as per RFC-61 */
        if(msLayerGetFeatureStyle(map, layer, layer->class[shape.classindex], &shape) != MS_SUCCESS) {
          retcode = MS_FAILURE;
          break;
        }
      }

      /* __TODO__ For now, we can't cache features with 'AUTO' style */
      cache = MS_FALSE;
    }

    /* RFC77 TODO: check return value, may need a more sophisticated if-then test. */
    if(annotate && layer->class[shape.classindex]->numlabels > 0) {
      msShapeGetAnnotation(layer, &shape);
      drawmode |= MS_DRAWMODE_LABELS;
      if (msLayerGetProcessingKey(layer, "LABEL_NO_CLIP")) {
        drawmode |= MS_DRAWMODE_UNCLIPPEDLABELS;
      }
    }

    if (layer->type == MS_LAYER_LINE && msLayerGetProcessingKey(layer, "POLYLINE_NO_CLIP")) {
      drawmode |= MS_DRAWMODE_UNCLIPPEDLINES;
    }
    
    if (cache) {
      styleObj *pStyle = layer->class[shape.classindex]->styles[0];
      colorObj tmp;
      if (pStyle->outlinewidth > 0) {
        /*
         * RFC 49 implementation
         * if an outlinewidth is used:
         *  - augment the style's width to account for the outline width
         *  - swap the style color and outlinecolor
         *  - draw the shape (the outline) in the first pass of the
         *    caching mechanism
         */

        /* adapt width (must take scalefactor into account) */
        pStyle->width += (pStyle->outlinewidth / (layer->scalefactor/image->resolutionfactor)) * 2;
        pStyle->minwidth += pStyle->outlinewidth * 2;
        pStyle->maxwidth += pStyle->outlinewidth * 2;
        pStyle->size += (pStyle->outlinewidth/layer->scalefactor*(map->resolution/map->defresolution));

        /*swap color and outlinecolor*/
        tmp = pStyle->color;
        pStyle->color = pStyle->outlinecolor;
        pStyle->outlinecolor = tmp;
      }
      status = msDrawShape(map, layer, &shape, image, 0, drawmode|MS_DRAWMODE_SINGLESTYLE); /* draw a single style */
      if (pStyle->outlinewidth > 0) {
        /*
         * RFC 49 implementation: switch back the styleobj to its
         * original state, so the line fill will be drawn in the
         * second pass of the caching mechanism
         */

        /* reset widths to original state */
        pStyle->width -= (pStyle->outlinewidth / (layer->scalefactor/image->resolutionfactor)) * 2;
        pStyle->minwidth -= pStyle->outlinewidth * 2;
        pStyle->maxwidth -= pStyle->outlinewidth * 2;
        pStyle->size -= (pStyle->outlinewidth/layer->scalefactor*(map->resolution/map->defresolution));

        /*reswap colors to original state*/
        tmp = pStyle->color;
        pStyle->color = pStyle->outlinecolor;
        pStyle->outlinecolor = tmp;
      }
    }

    else
      status = msDrawShape(map, layer, &shape, image, -1, drawmode); /* all styles  */
    if(status != MS_SUCCESS) {
      msFreeShape(&shape);
      retcode = MS_FAILURE;
      break;
    }

    if(shape.numlines == 0) { /* once clipped the shape didn't need to be drawn */
      msFreeShape(&shape);
      continue;
    }

    if(cache) {
      if(insertFeatureList(&shpcache, &shape) == NULL) {
        retcode = MS_FAILURE; /* problem adding to the cache */
        break;
      }
    }

    maxnumstyles = MS_MAX(maxnumstyles, layer->class[shape.classindex]->numstyles);
    msFreeShape(&shape);
  }

  if (classgroup)
    msFree(classgroup);

  if(status != MS_DONE || retcode == MS_FAILURE) {
    msLayerClose(layer);
    if(shpcache) {
      freeFeatureList(shpcache);
      shpcache = NULL;
    }
    return MS_FAILURE;
  }

  if(shpcache && MS_DRAW_FEATURES(drawmode)) {
    int s;
    for(s=0; s<maxnumstyles; s++) {
      for(current=shpcache; current; current=current->next) {
        if(layer->class[current->shape.classindex]->numstyles > s) {
          styleObj *pStyle = layer->class[current->shape.classindex]->styles[s];
          if(pStyle->_geomtransform.type != MS_GEOMTRANSFORM_NONE)
            continue; /*skip this as it has already been rendered*/
          if(map->scaledenom > 0) {
            if((pStyle->maxscaledenom != -1) && (map->scaledenom >= pStyle->maxscaledenom))
              continue;
            if((pStyle->minscaledenom != -1) && (map->scaledenom < pStyle->minscaledenom))
              continue;
          }
          if(s==0 && pStyle->outlinewidth>0 && MS_VALID_COLOR(pStyle->color)) {
            msDrawLineSymbol(&map->symbolset, image, &current->shape, pStyle, layer->scalefactor);
          } else if(s>0) {
            if (pStyle->outlinewidth > 0 && MS_VALID_COLOR(pStyle->outlinecolor)) {
              colorObj tmp;
              /*
               * RFC 49 implementation
               * if an outlinewidth is used:
               *  - augment the style's width to account for the outline width
               *  - swap the style color and outlinecolor
               *  - draw the shape (the outline) in the first pass of the
               *    caching mechanism
               */

              /* adapt width (must take scalefactor into account) */
              pStyle->width += (pStyle->outlinewidth / (layer->scalefactor/image->resolutionfactor)) * 2;
              pStyle->minwidth += pStyle->outlinewidth * 2;
              pStyle->maxwidth += pStyle->outlinewidth * 2;
              pStyle->size += (pStyle->outlinewidth/layer->scalefactor*(map->resolution/map->defresolution));

              /*swap color and outlinecolor*/
              tmp = pStyle->color;
              pStyle->color = pStyle->outlinecolor;
              pStyle->outlinecolor = tmp;
              msDrawLineSymbol(&map->symbolset, image, &current->shape, pStyle, layer->scalefactor);
              /*
               * RFC 49 implementation: switch back the styleobj to its
               * original state, so the line fill will be drawn in the
               * second pass of the caching mechanism
               */

              /* reset widths to original state */
              pStyle->width -= (pStyle->outlinewidth / (layer->scalefactor/image->resolutionfactor)) * 2;
              pStyle->minwidth -= pStyle->outlinewidth * 2;
              pStyle->maxwidth -= pStyle->outlinewidth * 2;
              pStyle->size -= (pStyle->outlinewidth/layer->scalefactor*(map->resolution/map->defresolution));

              /*reswap colors to original state*/
              tmp = pStyle->color;
              pStyle->color = pStyle->outlinecolor;
              pStyle->outlinecolor = tmp;
            }
            /* draw a valid line, i.e. one with a color defined or of type pixmap*/
            if(MS_VALID_COLOR(pStyle->color) || 
                    (
                      pStyle->symbol<map->symbolset.numsymbols &&
                      ( 
                        map->symbolset.symbol[pStyle->symbol]->type == MS_SYMBOL_PIXMAP ||
                        map->symbolset.symbol[pStyle->symbol]->type == MS_SYMBOL_SVG
                      )
                    )
              ) {
              msDrawLineSymbol(&map->symbolset, image, &current->shape, pStyle, layer->scalefactor);
            }
          }
        }
      }
    }

    freeFeatureList(shpcache);
    shpcache = NULL;
  }

  msLayerClose(layer);
  return MS_SUCCESS;

}

/*
** Function to draw a layer IF it already has a result cache associated with it. Called by msDrawMap and via MapScript.
*/
int msDrawQueryLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  int i, status;
  char annotate=MS_TRUE, cache=MS_FALSE;
  int drawmode = MS_DRAWMODE_FEATURES|MS_DRAWMODE_QUERY;
  shapeObj shape;
  int maxnumstyles=1;

  featureListNodeObjPtr shpcache=NULL, current=NULL;

  colorObj *colorbuffer = NULL;
  int *mindistancebuffer = NULL;

  if(!layer->resultcache) return(msDrawLayer(map, layer, image));

  if(!msLayerIsVisible(map, layer)) return(MS_SUCCESS); /* not an error, just nothing to do */

  /* conditions may have changed since this layer last drawn, so set
     layer->project true to recheck projection needs (Bug #673) */
  layer->project = MS_TRUE;

  /* set annotation status */
  annotate = msEvalContext(map, layer, layer->labelrequires);
  if(map->scaledenom > 0) {
    if((layer->labelmaxscaledenom != -1) && (map->scaledenom >= layer->labelmaxscaledenom)) annotate = MS_FALSE;
    if((layer->labelminscaledenom != -1) && (map->scaledenom < layer->labelminscaledenom)) annotate = MS_FALSE;
  }

  /*
  ** Certain query map styles require us to render all features only (MS_NORMAL) or first (MS_HILITE). With
  ** single-pass queries we have to make a copy of the layer and work from it instead.
  */
  if(map->querymap.style == MS_NORMAL || map->querymap.style == MS_HILITE) {
    layerObj tmp_layer;

    if(initLayer(&tmp_layer, map) == -1)
      return(MS_FAILURE);

    if (msCopyLayer(&tmp_layer, layer) != MS_SUCCESS)
      return(MS_FAILURE);

    /* disable the connection pool for this layer */
    msLayerSetProcessingKey(&tmp_layer, "CLOSE_CONNECTION", "ALWAYS");

    status = msDrawLayer(map, &tmp_layer, image);

    freeLayer(&tmp_layer);

    if(map->querymap.style == MS_NORMAL || status != MS_SUCCESS) return(status);
  }

#ifdef USE_GD
  /* reset layer pen values just in case the map has been previously processed */
  msClearLayerPenValues(layer);
#endif

  /* if MS_HILITE, alter the one style (always at least 1 style), and set a MINDISTANCE for the labelObj to avoid duplicates */
  if(map->querymap.style == MS_HILITE) {
    if (layer->numclasses > 0) {
      colorbuffer = (colorObj*)malloc(layer->numclasses*sizeof(colorObj));
      mindistancebuffer = (int*)malloc(layer->numclasses*sizeof(int));

      if (colorbuffer == NULL || mindistancebuffer == NULL) {
        msSetError(MS_MEMERR, "Failed to allocate memory for colorbuffer/mindistancebuffer", "msDrawQueryLayer()");
        return MS_FAILURE;
      }
    }

    for(i=0; i<layer->numclasses; i++) {
      if(layer->type == MS_LAYER_POLYGON) { /* alter BOTTOM style since that's almost always the fill */
        if (layer->class[i]->styles == NULL) {
          msSetError(MS_MISCERR, "Don't know how to draw class %s of layer %s without a style definition.", "msDrawQueryLayer()", layer->class[i]->name, layer->name);
          return(MS_FAILURE);
        }
        if(MS_VALID_COLOR(layer->class[i]->styles[0]->color)) {
          colorbuffer[i] = layer->class[i]->styles[0]->color; /* save the color from the BOTTOM style */
          layer->class[i]->styles[0]->color = map->querymap.color;
        } else if(MS_VALID_COLOR(layer->class[i]->styles[0]->outlinecolor)) {
          colorbuffer[i] = layer->class[i]->styles[0]->outlinecolor; /* if no color, save the outlinecolor from the BOTTOM style */
          layer->class[i]->styles[0]->outlinecolor = map->querymap.color;
        }
      } else {
        if(MS_VALID_COLOR(layer->class[i]->styles[layer->class[i]->numstyles-1]->color)) {
          colorbuffer[i] = layer->class[i]->styles[layer->class[i]->numstyles-1]->color; /* save the color from the TOP style */
          layer->class[i]->styles[layer->class[i]->numstyles-1]->color = map->querymap.color;
        } else if(MS_VALID_COLOR(layer->class[i]->styles[layer->class[i]->numstyles-1]->outlinecolor)) {
          colorbuffer[i] = layer->class[i]->styles[layer->class[i]->numstyles-1]->outlinecolor; /* if no color, save the outlinecolor from the TOP style */
          layer->class[i]->styles[layer->class[i]->numstyles-1]->outlinecolor = map->querymap.color;
        }
      }

      mindistancebuffer[i] = -1; /* RFC77 TODO: only using the first label, is that cool? */
      if(layer->class[i]->numlabels > 0) {
        mindistancebuffer[i] = layer->class[i]->labels[0]->mindistance;
        layer->class[i]->labels[0]->mindistance = MS_MAX(0, layer->class[i]->labels[0]->mindistance);
      }
    }
  }

  /*
  ** Layer was opened as part of the query process, msLayerWhichItems() has also been run, shapes have been classified - start processing!
  */

  msInitShape(&shape);

  for(i=0; i<layer->resultcache->numresults; i++) {
    status = msLayerGetShape(layer, &shape, &(layer->resultcache->results[i]));
    if(status != MS_SUCCESS) {
      msFree(colorbuffer);
      msFree(mindistancebuffer);
      return(MS_FAILURE);
    }

    shape.classindex = layer->resultcache->results[i].classindex;
    /* classindex may be -1 here if there was a template at the top level
     * in this layer and the current shape selected even if it didn't
     * match any class
     *
     * FrankW: classindex is also sometimes 0 even if there are no classes, so
     * we are careful not to use out of range class values as an index.
     */
    if(shape.classindex==-1
        || shape.classindex >= layer->numclasses
        || layer->class[shape.classindex]->status == MS_OFF) {
      msFreeShape(&shape);
      continue;
    }

    cache = MS_FALSE;
    if(layer->type == MS_LAYER_LINE && layer->class[shape.classindex]->numstyles > 1)
      cache = MS_TRUE; /* only line layers with multiple styles need be cached (I don't think POLYLINE layers need caching - SDL) */

    /* RFC 77 TODO: check return value for msShapeGetAnnotation() */
    if(annotate && layer->class[shape.classindex]->numlabels > 0) {
      msShapeGetAnnotation(layer, &shape);
      drawmode |= MS_DRAWMODE_LABELS;
    }

    if(cache) {
      drawmode |= MS_DRAWMODE_SINGLESTYLE;
      status = msDrawShape(map, layer, &shape, image, 0, drawmode); /* draw only the first style */
    }
    else
      status = msDrawShape(map, layer, &shape, image, -1, drawmode); /* all styles  */
    if(status != MS_SUCCESS) {
      msLayerClose(layer);
      msFree(colorbuffer);
      msFree(mindistancebuffer);
      return(MS_FAILURE);
    }

    if(shape.numlines == 0) { /* once clipped the shape didn't need to be drawn */
      msFreeShape(&shape);
      continue;
    }

    if(cache) {
      if(insertFeatureList(&shpcache, &shape) == NULL) return(MS_FAILURE); /* problem adding to the cache */
    }

    maxnumstyles = MS_MAX(maxnumstyles, layer->class[shape.classindex]->numstyles);
    msFreeShape(&shape);
  }

  if(shpcache) {
    int s;

    for(s=1; s<maxnumstyles; s++) {
      for(current=shpcache; current; current=current->next) {
        if(layer->class[current->shape.classindex]->numstyles > s) {
          styleObj *curStyle = layer->class[current->shape.classindex]->styles[s];
          if(map->scaledenom > 0) {
            if((curStyle->maxscaledenom != -1) && (map->scaledenom >= curStyle->maxscaledenom))
              continue;
            if((curStyle->minscaledenom != -1) && (map->scaledenom < curStyle->minscaledenom))
              continue;
          }
          msDrawLineSymbol(&map->symbolset, image, &current->shape, (layer->class[current->shape.classindex]->styles[s]), layer->scalefactor);
        }
      }
    }

    freeFeatureList(shpcache);
    shpcache = NULL;
  }

  /* if MS_HILITE, restore color and mindistance values */
  if(map->querymap.style == MS_HILITE) {
    for(i=0; i<layer->numclasses; i++) {
      if(layer->type == MS_LAYER_POLYGON) {
        if(MS_VALID_COLOR(layer->class[i]->styles[0]->color))
          layer->class[i]->styles[0]->color = colorbuffer[i];
        else if(MS_VALID_COLOR(layer->class[i]->styles[0]->outlinecolor))
          layer->class[i]->styles[0]->outlinecolor = colorbuffer[i]; /* if no color, restore outlinecolor for the BOTTOM style */
      } else {
        if(MS_VALID_COLOR(layer->class[i]->styles[layer->class[i]->numstyles-1]->color))
          layer->class[i]->styles[layer->class[i]->numstyles-1]->color = colorbuffer[i];
        else if(MS_VALID_COLOR(layer->class[i]->styles[layer->class[i]->numstyles-1]->outlinecolor))
          layer->class[i]->styles[layer->class[i]->numstyles-1]->outlinecolor = colorbuffer[i]; /* if no color, restore outlinecolor for the TOP style */
      }

      if(layer->class[i]->numlabels > 0)
        layer->class[i]->labels[0]->mindistance = mindistancebuffer[i]; /* RFC77 TODO: again, only using the first label, is that cool? */

    }

    msFree(colorbuffer);
    msFree(mindistancebuffer);
  }

  return(MS_SUCCESS);
}

/**
 * msDrawRasterLayerPlugin()
 */

static int
msDrawRasterLayerPlugin( mapObj *map, layerObj *layer, imageObj *image)

{
  rendererVTableObj *renderer = MS_IMAGE_RENDERER(image);
  rasterBufferObj  *rb = msSmallCalloc(1,sizeof(rasterBufferObj));
  int ret;
  if( renderer->supports_pixel_buffer ) {
    if (MS_SUCCESS != renderer->getRasterBufferHandle( image, rb )) {
      msSetError(MS_MISCERR,"renderer failed to extract raster buffer","msDrawRasterLayer()");
      return MS_FAILURE;
    }
    ret = msDrawRasterLayerLow( map, layer, image, rb );
  } else {
    if (MS_SUCCESS != renderer->initializeRasterBuffer( rb, image->width, image->height, MS_IMAGEMODE_RGBA )) {
      msSetError(MS_MISCERR,"renderer failed to create raster buffer","msDrawRasterLayer()");
      return MS_FAILURE;
    }

    ret = msDrawRasterLayerLow( map, layer, image, rb );

    if( ret == 0 ) {
      renderer->mergeRasterBuffer( image, rb, 1.0, 0, 0, 0, 0, rb->width, rb->height );
    }

    msFreeRasterBuffer(rb);
  }
#define RB_GET_R(rb,x,y) *((rb)->data.rgba.r + (x) * (rb)->data.rgba.pixel_step + (y) * (rb)->data.rgba.row_step)
#define RB_GET_G(rb,x,y) *((rb)->data.rgba.g + (x) * (rb)->data.rgba.pixel_step + (y) * (rb)->data.rgba.row_step)
#define RB_GET_B(rb,x,y) *((rb)->data.rgba.b + (x) * (rb)->data.rgba.pixel_step + (y) * (rb)->data.rgba.row_step)
#define RB_GET_A(rb,x,y) *((rb)->data.rgba.a + (x) * (rb)->data.rgba.pixel_step + (y) * (rb)->data.rgba.row_step)

  free(rb);

  return ret;
}

/**
 * Generic function to render raster layers.
 */
int msDrawRasterLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  if (image && map && layer) {
    if( MS_RENDERER_PLUGIN(image->format) ) {
      return msDrawRasterLayerPlugin(map, layer, image);
    } else if( MS_RENDERER_RAWDATA(image->format) )
      return msDrawRasterLayerLow(map, layer, image, NULL);
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

  if (image && map && layer) {
    /* ------------------------------------------------------------------
     * Start by downloading the layer
     * ------------------------------------------------------------------ */
    httpRequestObj asReqInfo[2];
    int numReq = 0;

    msHTTPInitRequestObj(asReqInfo, 2);

    if ( msPrepareWMSLayerRequest(1, map, layer, 1,
                                  0, NULL, 0, 0, 0, NULL,
                                  asReqInfo, &numReq) == MS_FAILURE  ||
         msOWSExecuteRequests(asReqInfo, numReq, map, MS_TRUE) == MS_FAILURE ) {
      return MS_FAILURE;
    }

    /* ------------------------------------------------------------------
     * Then draw layer based on output format
     * ------------------------------------------------------------------ */
    if( MS_RENDERER_PLUGIN(image->format) )
      nStatus = msDrawWMSLayerLow(1, asReqInfo, numReq,
                                  map, layer, image) ;
    else if( MS_RENDERER_RAWDATA(image->format) )
      nStatus = msDrawWMSLayerLow(1, asReqInfo, numReq,
                                  map, layer, image) ;

    else {
      msSetError(MS_WMSCONNERR,
                 "Output format '%s' doesn't support WMS layers.",
                 "msDrawWMSLayer()", image->format->name);
      nStatus = MS_SUCCESS; /* Should we fail if output doesn't support WMS? */
    }
    /* Cleanup */
    msHTTPFreeRequestObj(asReqInfo, numReq);
  }

  return nStatus;
}
#endif

int circleLayerDrawShape(mapObj *map, imageObj *image, layerObj *layer, shapeObj *shape)
{
  pointObj center;
  double r;
  int s;
  int c = shape->classindex;

  if (shape->numlines != 1) return (MS_SUCCESS); /* invalid shape */
  if (shape->line[0].numpoints != 2) return (MS_SUCCESS); /* invalid shape */

  center.x = (shape->line[0].point[0].x + shape->line[0].point[1].x) / 2.0;
  center.y = (shape->line[0].point[0].y + shape->line[0].point[1].y) / 2.0;
  r = MS_ABS(center.x - shape->line[0].point[0].x);
  if (r == 0)
    r = MS_ABS(center.y - shape->line[0].point[0].y);
  if (r == 0)
    return (MS_SUCCESS);

  if (layer->transform == MS_TRUE) {

#ifdef USE_PROJ
    if (layer->project && msProjectionsDiffer(&(layer->projection), &(map->projection)))
      msProjectPoint(&layer->projection, &map->projection, &center);
    else
      layer->project = MS_FALSE;
#endif

    center.x = MS_MAP2IMAGE_X(center.x, map->extent.minx, map->cellsize);
    center.y = MS_MAP2IMAGE_Y(center.y, map->extent.maxy, map->cellsize);
    r /= map->cellsize;
  } else
    msOffsetPointRelativeTo(&center, layer);

  for (s = 0; s < layer->class[c]->numstyles; s++) {
    if (msScaleInBounds(map->scaledenom,
                        layer->class[c]->styles[s]->minscaledenom,
                        layer->class[c]->styles[s]->maxscaledenom))
      msCircleDrawShadeSymbol(&map->symbolset, image, &center, r,
                              layer->class[c]->styles[s], layer->scalefactor);
  }
  return MS_SUCCESS;
  /* TODO: need to handle circle annotation */
}

int annotationLayerDrawShape(mapObj *map, imageObj *image, layerObj *layer, shapeObj *shape)
{
  int c = shape->classindex;
  rectObj cliprect;
  labelObj *label;
  double minfeaturesize;
  labelPathObj **annopaths = NULL; /* Curved label path. Bug #1620 implementation */
  pointObj **annopoints = NULL;
  pointObj annopnt;
  pointObj *point;
  int *regularLines = NULL;
  double** angles = NULL, **lengths = NULL;
  int ret = MS_SUCCESS;
  int numpaths = 1, numpoints = 1, numRegularLines = 0, i,j,s;
  if (layer->class[c]->numlabels == 0) return (MS_SUCCESS); /* nothing to draw (RFC77 TDOO: could expand this test) */
  if (layer->class[c]->numlabels > 1) {
    msSetError(MS_MISCERR, "Multiple labels not supported on annotation layers.", "annotationLayerDrawShape()");
    return MS_FAILURE;
  }

  cliprect.minx = cliprect.miny = 0;
  cliprect.maxx = image->width;
  cliprect.maxy = image->height;



  /* annotation layers can only have one layer, don't treat the multi layer case */
  label = layer->class[c]->labels[0]; /* for brevity */
  minfeaturesize = label->minfeaturesize * image->resolutionfactor;


  switch (shape->type) {
    case(MS_SHAPE_LINE):

      if (label->anglemode == MS_FOLLOW) { /* bug #1620 implementation */
#ifndef NDEBUG
        /* this test should already occur at the parser level in loadLabel() */
        if (label->type != MS_TRUETYPE) {
          msSetError(MS_MISCERR, "Angle mode 'FOLLOW' is supported only with truetype fonts.", "annotationLayerDrawShape()");
          return (MS_FAILURE);
        }
#endif
        if(!layer->labelcache) {
          msSetError(MS_MISCERR, "Angle mode 'FOLLOW' is supported only with LABELCACHE ON", "annotationLayerDrawShape()");
          return (MS_FAILURE);
        }
        annopaths = msPolylineLabelPath(map, image, shape, minfeaturesize, &(map->fontset),
                                        label->annotext, label, layer->scalefactor, &numpaths, &regularLines, &numRegularLines);

        for (i = 0; i < numpaths; i++) {
          label->position = MS_CC; /* force label position to MS_CC regardless if a path is computed (WHY HERE?) */

          if (annopaths[i]) {
            if (msAddLabel(map, label, layer->index, c, shape, NULL, annopaths[i], -1) != MS_SUCCESS) {
              ret = MS_FAILURE;
              goto anno_cleanup_line;
            }
          }
        }

        /* handle regular lines that have to be drawn with the regular algorithm */
        if (numRegularLines > 0) {
          annopoints = msPolylineLabelPointExtended(shape, minfeaturesize, label->repeatdistance,
                       &angles, &lengths, &numpoints, regularLines, numRegularLines, MS_FALSE);

          for (i = 0; i < numpoints; i++) {
            label->angle = *angles[i];
            if (msAddLabel(map, label, layer->index, c, shape, annopoints[i], NULL, *lengths[i]) != MS_SUCCESS) {
              ret = MS_FAILURE;
              goto anno_cleanup_line;
            }
          }
        }
      } else {
        int s;
        annopoints = msPolylineLabelPoint(shape, minfeaturesize, label->repeatdistance, &angles, &lengths, &numpoints, label->anglemode);

        if (label->angle != 0)
          label->angle -= map->gt.rotation_angle; /* apply rotation angle */

        for (i = 0; i < numpoints; i++) {
          if (label->anglemode != MS_NONE) label->angle = *angles[i]; /* angle derived from line overrides even the rotation value. */

          if (layer->labelcache) {
            if (msAddLabel(map, label, layer->index, c, shape, annopoints[i], NULL, *lengths[i]) != MS_SUCCESS) return (MS_FAILURE);
          } else {
            if (layer->class[c]->numstyles > 0 && MS_VALID_COLOR(layer->class[c]->styles[0]->color)) {
              for (s = 0; s < layer->class[c]->numstyles; s++) {
                if (msScaleInBounds(map->scaledenom, layer->class[c]->styles[s]->minscaledenom, layer->class[c]->styles[s]->maxscaledenom))
                  msDrawMarkerSymbol(&map->symbolset, image, annopoints[i], layer->class[c]->styles[s], layer->scalefactor);
              }
            }
            msDrawLabel(map, image, *annopoints[i], label->annotext, label, layer->scalefactor);
          }
        }
      }

anno_cleanup_line:
      msFree(annopaths);
      msFree(regularLines);

      if (annopoints) {
        for (i = 0; i < numpoints; i++) {
          if (annopoints[i]) free(annopoints[i]);
          if (angles[i]) free(angles[i]);
          if (lengths[i]) free(lengths[i]);
        }
        free(angles);
        free(annopoints);
        free(lengths);
      }
      return ret;
    case(MS_SHAPE_POLYGON):

      if (msPolygonLabelPoint(shape, &annopnt, minfeaturesize) == MS_SUCCESS) {
        if(annopnt.x>0 && annopnt.y >0 && annopnt.x <= image->width && annopnt.y<=image->height) {
          if (label->angle != 0)
            label->angle -= map->gt.rotation_angle; /* TODO: isn't this a bug, the label angle will be changed at each feature ? */
          if(layer->labelcache) {
            if (msAddLabel(map, label, layer->index, c, shape, &annopnt, NULL,
                           MS_MIN(shape->bounds.maxx - shape->bounds.minx, shape->bounds.maxy - shape->bounds.miny)) != MS_SUCCESS) {
              return (MS_FAILURE);
            }
          } else {
            if (layer->class[c]->numstyles > 0 && MS_VALID_COLOR(layer->class[c]->styles[0]->color)) {
              for (i = 0; i < layer->class[c]->numstyles; i++) {
                if (msScaleInBounds(map->scaledenom, layer->class[c]->styles[i]->minscaledenom, layer->class[c]->styles[i]->maxscaledenom))
                  msDrawMarkerSymbol(&map->symbolset, image, &annopnt, layer->class[c]->styles[i], layer->scalefactor);
              }
            }
            msDrawLabel(map, image, annopnt, label->annotext, label, layer->scalefactor);
          }
        }
      }
      break;
    default: /* points and anything with out a proper type */
      if (label->angle != 0) label->angle -= map->gt.rotation_angle;
      for (j = 0; j < shape->numlines; j++) {
        for (i = 0; i < shape->line[j].numpoints; i++) {
          point = &(shape->line[j].point[i]);
          if (!msPointInRect(point, &cliprect)) continue;
          if (layer->labelcache) {
            if (msAddLabel(map, label, layer->index, c, shape, point, NULL, -1) != MS_SUCCESS) return (MS_FAILURE);
          } else {
            if (layer->class[c]->numstyles > 0 && MS_VALID_COLOR(layer->class[c]->styles[0]->color)) {
              for (s = 0; s < layer->class[c]->numstyles; s++) {
                if (msScaleInBounds(map->scaledenom, layer->class[c]->styles[s]->minscaledenom, layer->class[c]->styles[s]->maxscaledenom))
                  msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c]->styles[s], layer->scalefactor);
              }
            }
            msDrawLabel(map, image, *point, label->annotext, label, layer->scalefactor);
          }
        }
      }
  }
  return MS_SUCCESS;
}

int pointLayerDrawShape(mapObj *map, imageObj *image, layerObj *layer, shapeObj *shape, int drawmode)
{
  int l, c = shape->classindex, j, i, s;
  pointObj *point;

#ifdef USE_PROJ
  if (layer->project && layer->transform == MS_TRUE && msProjectionsDiffer(&(layer->projection), &(map->projection)))
    msProjectShape(&layer->projection, &map->projection, shape);
  else
    layer->project = MS_FALSE;
#endif

  for (l = 0; l < layer->class[c]->numlabels; l++)
    if (layer->class[c]->labels[l]->angle != 0) layer->class[c]->labels[l]->angle -= map->gt.rotation_angle; /* TODO: is this right???? */

  for (j = 0; j < shape->numlines; j++) {
    for (i = 0; i < shape->line[j].numpoints; i++) {
      point = &(shape->line[j].point[i]);
      if (layer->transform == MS_TRUE) {
        if (!msPointInRect(point, &map->extent)) continue; /* next point */
        msTransformPoint(point, &map->extent, map->cellsize, image);
      } else
        msOffsetPointRelativeTo(point, layer);

      if(MS_DRAW_FEATURES(drawmode)) {
        for (s = 0; s < layer->class[c]->numstyles; s++) {
          if (msScaleInBounds(map->scaledenom,
              layer->class[c]->styles[s]->minscaledenom,
              layer->class[c]->styles[s]->maxscaledenom))
            msDrawMarkerSymbol(&map->symbolset, image, point, layer->class[c]->styles[s], layer->scalefactor);
        }
      }
      if(MS_DRAW_LABELS(drawmode)) {
        if (layer->labelcache) {
          if (msAddLabelGroup(map, layer->index, c, shape, point, -1) != MS_SUCCESS) return (MS_FAILURE);
        } else {
          for (l = 0; l < layer->class[c]->numlabels; l++)
            msDrawLabel(map, image, *point, layer->class[c]->labels[l]->annotext, layer->class[c]->labels[l], layer->scalefactor);
        }
      }
    }
  }
  return MS_SUCCESS;
}

int lineLayerDrawShape(mapObj *map, imageObj *image, layerObj *layer, shapeObj *shape,
                       shapeObj *anno_shape, shapeObj *unclipped_shape, int style, int drawmode)
{
  int c = shape->classindex;
  double minfeaturesize;
  labelPathObj **annopaths = NULL; /* Curved label path. Bug #1620 implementation */
  pointObj **annopoints = NULL;
  int *regularLines = NULL;
  double** angles = NULL, **lengths = NULL;
  int ret = MS_SUCCESS;
  int numpaths = 1, numpoints = 1, numRegularLines = 0, i, j, s, l = 0;

  /* RFC48: loop through the styles, and pass off to the type-specific
  function if the style has an appropriate type */
  if(MS_DRAW_FEATURES(drawmode)) {
    for (s = 0; s < layer->class[c]->numstyles; s++) {
      if (msScaleInBounds(map->scaledenom,
          layer->class[c]->styles[s]->minscaledenom,
          layer->class[c]->styles[s]->maxscaledenom)) {
        if (layer->class[c]->styles[s]->_geomtransform.type != MS_GEOMTRANSFORM_NONE)
          msDrawTransformedShape(map, &map->symbolset, image, unclipped_shape, layer->class[c]->styles[s], layer->scalefactor);
        else if (!MS_DRAW_SINGLESTYLE(drawmode) || s == style)
          msDrawLineSymbol(&map->symbolset, image, shape, layer->class[c]->styles[s], layer->scalefactor);
      }
    }
  }
  
  if(MS_DRAW_LABELS(drawmode)) {
    for (l = 0; l < layer->class[c]->numlabels; l++) {
      labelObj *label = layer->class[c]->labels[l];
      minfeaturesize = label->minfeaturesize * image->resolutionfactor;

      if (label->anglemode == MS_FOLLOW) { /* bug #1620 implementation */
        if (label->type != MS_TRUETYPE) {
          msSetError(MS_MISCERR, "Angle mode 'FOLLOW' is supported only with truetype fonts.", "msDrawShape()");
          ret = MS_FAILURE;
          goto line_cleanup;
        }
        annopaths = msPolylineLabelPath(map, image, anno_shape, minfeaturesize, &(map->fontset),
                                        label->annotext, label, layer->scalefactor, &numpaths, &regularLines, &numRegularLines);

        for (i = 0; i < numpaths; i++) {
          label->position = MS_CC; /* force all label positions to MS_CC regardless if a path is computed */

          if (annopaths[i]) {
            if (layer->labelcache) {
              if (msAddLabel(map, label, layer->index, c, anno_shape, NULL, annopaths[i], -1) != MS_SUCCESS) {
                ret = MS_FAILURE;
                goto line_cleanup;
              }
            } else {
              /* TODO: handle drawing curved labels outside the cache */
              msFreeLabelPathObj(annopaths[i]);
              annopaths[i] = NULL;
            }
          }
        }

        /* handle regular lines that have to be drawn with the regular algorithm */
        if (numRegularLines > 0) {
          annopoints = msPolylineLabelPointExtended(anno_shape, minfeaturesize, label->repeatdistance,
                       &angles, &lengths, &numpoints, regularLines, numRegularLines, MS_FALSE);

          for (i = 0; i < numpoints; i++) {
            label->angle = *angles[i];
            if (layer->labelcache) {
              if (msAddLabel(map, label, layer->index, c, anno_shape, annopoints[i], NULL, *lengths[i]) != MS_SUCCESS) {
                ret = MS_FAILURE;
                goto line_cleanup;
              }
            } else {
              msDrawLabel(map, image, *annopoints[i], label->annotext, label, layer->scalefactor);
            }
          }
        }
      } else {
        annopoints = msPolylineLabelPoint(anno_shape, minfeaturesize, label->repeatdistance, &angles,
                                          &lengths, &numpoints, label->anglemode);

        if (label->angle != 0)
          label->angle -= map->gt.rotation_angle; /* apply rotation angle */

        for (i = 0; i < numpoints; i++) {
          if (label->anglemode != MS_NONE) label->angle = *angles[i]; /* angle derived from line overrides even the rotation value. */

          if (layer->labelcache) {
            if (msAddLabel(map, label, layer->index, c, anno_shape, annopoints[i], NULL, *lengths[i]) != MS_SUCCESS) {
              ret = MS_FAILURE;
              goto line_cleanup;
            }
          } else {
            msDrawLabel(map, image, *annopoints[i], label->annotext, label, layer->scalefactor);
          }
        }
      }

line_cleanup:
      /* clean up and reset */
      if (annopaths) {
        free(annopaths);
        annopaths = NULL;
      }

      if (regularLines) {
        free(regularLines);
        regularLines = NULL;
      }

      if (annopoints) {
        for (j = 0; j < numpoints; j++) {
          if (annopoints[j]) free(annopoints[j]);
          if (angles[j]) free(angles[j]);
          if (lengths[j]) free(lengths[j]);
        }
        free(angles);
        free(annopoints);
        free(lengths);
        annopoints = NULL;
        angles = NULL;
        lengths = NULL;
      }
      if (ret == MS_FAILURE) {
        break; /* from the label looping */
      }
    } /* next label */
  }

  return ret;

}

int polygonLayerDrawShape(mapObj *map, imageObj *image, layerObj *layer,
                          shapeObj *shape, shapeObj *anno_shape, shapeObj *unclipped_shape, int drawmode)
{

  int c = shape->classindex;
  pointObj annopnt;
  int i;

  if(MS_DRAW_FEATURES(drawmode)) {
    for (i = 0; i < layer->class[c]->numstyles; i++) {
      if (msScaleInBounds(map->scaledenom, layer->class[c]->styles[i]->minscaledenom,
                          layer->class[c]->styles[i]->maxscaledenom)) {
        if (layer->class[c]->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_NONE)
          msDrawShadeSymbol(&map->symbolset, image, shape, layer->class[c]->styles[i], layer->scalefactor);
        else
          msDrawTransformedShape(map, &map->symbolset, image, unclipped_shape,
                                 layer->class[c]->styles[i], layer->scalefactor);
      }
    }
  }

  if(MS_DRAW_LABELS(drawmode)) {
    if (layer->class[c]->numlabels > 0) {
      double minfeaturesize = layer->class[c]->labels[0]->minfeaturesize * image->resolutionfactor;
      if (msPolygonLabelPoint(anno_shape, &annopnt, minfeaturesize) == MS_SUCCESS) {
        for (i = 0; i < layer->class[c]->numlabels; i++)
          if (layer->class[c]->labels[i]->angle != 0) layer->class[c]->labels[i]->angle -= map->gt.rotation_angle; /* TODO: is this correct ??? */
        if (layer->labelcache) {
          if (msAddLabelGroup(map, layer->index, c, anno_shape, &annopnt,
                              MS_MIN(shape->bounds.maxx - shape->bounds.minx, shape->bounds.maxy - shape->bounds.miny)) != MS_SUCCESS) {
            return MS_FAILURE;
          }
        } else {
          for (i = 0; i < layer->class[c]->numlabels; i++)
            msDrawLabel(map, image, annopnt, layer->class[c]->labels[i]->annotext,
                        layer->class[c]->labels[i], layer->scalefactor);
        }
      }
    }
  }
  return MS_SUCCESS;
}

/*
** Function to render an individual shape, the style variable enables/disables the drawing of a single style
** versus a single style. This is necessary when drawing entire layers as proper overlay can only be achived
** through caching. "querymapMode" parameter is used to tell msBindLayerToShape to not override the
** QUERYMAP HILITE color.
*/
int msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, imageObj *image, int style, int drawmode)
{
  int c,s,ret=MS_SUCCESS;
  shapeObj *anno_shape, *unclipped_shape = shape;
  int bNeedUnclippedShape = MS_FALSE;
  int bNeedUnclippedAnnoShape = MS_FALSE;
  int bShapeNeedsClipping = MS_TRUE;

  if(shape->numlines == 0 || shape->type == MS_SHAPE_NULL) return MS_SUCCESS;

  msDrawStartShape(map, layer, image, shape);
  c = shape->classindex;

  /* Before we do anything else, we will check for a rangeitem.
     If its there, we need to change the style's color to map
     the range to the shape */
  for(s=0; s<layer->class[c]->numstyles; s++) {
    styleObj *style = layer->class[c]->styles[s];
    if(style->rangeitem !=  NULL)
      msShapeToRange((layer->class[c]->styles[s]), shape);
  }

  /* circle and point layers go through their own treatment */
  if(layer->type == MS_LAYER_CIRCLE) {
    if(msBindLayerToShape(layer, shape, drawmode) != MS_SUCCESS) return MS_FAILURE;
    ret = circleLayerDrawShape(map,image,layer,shape);
    msDrawEndShape(map,layer,image,shape);
    return ret;
  } else if(layer->type == MS_LAYER_POINT || layer->type == MS_LAYER_RASTER) {
    if(msBindLayerToShape(layer, shape, drawmode) != MS_SUCCESS) return MS_FAILURE;
    ret = pointLayerDrawShape(map,image,layer,shape,drawmode);
    msDrawEndShape(map,layer,image,shape);
    return ret;
  }

  if (layer->type == MS_LAYER_POLYGON && shape->type != MS_SHAPE_POLYGON) {
    msSetError(MS_MISCERR, "Only polygon shapes can be drawn using a polygon layer definition.", "polygonLayerDrawShape()");
    return (MS_FAILURE);
  }
  if (layer->type == MS_LAYER_LINE && shape->type != MS_SHAPE_POLYGON && shape->type != MS_SHAPE_LINE) {
    msSetError(MS_MISCERR, "Only polygon or line shapes can be drawn using a line layer definition.", "msDrawShape()");
    return (MS_FAILURE);
  }

#ifdef USE_PROJ
  if (layer->project && layer->transform == MS_TRUE && msProjectionsDiffer(&(layer->projection), &(map->projection)))
    msProjectShape(&layer->projection, &map->projection, shape);
  else
    layer->project = MS_FALSE;
#endif

  /* check if we'll need the unclipped shape */
  if (shape->type != MS_SHAPE_POINT) {
    if(MS_DRAW_FEATURES(drawmode)) {
      for (s = 0; s < layer->class[c]->numstyles; s++) {
        styleObj *style = layer->class[c]->styles[s];
        if (style->_geomtransform.type != MS_GEOMTRANSFORM_NONE)
          bNeedUnclippedShape = MS_TRUE;
      }
    }
    /* check if we need to clip the shape */
    if (shape->bounds.minx < map->extent.minx ||
        shape->bounds.miny < map->extent.miny ||
        shape->bounds.maxx > map->extent.maxx ||
        shape->bounds.maxy > map->extent.maxy) {
      bShapeNeedsClipping = MS_TRUE;
    }
   
    if(MS_DRAW_LABELS(drawmode) && MS_DRAW_UNCLIPPED_LABELS(drawmode)) {
      bNeedUnclippedAnnoShape = MS_TRUE;
      bNeedUnclippedShape = MS_TRUE;
    }

    if(MS_DRAW_UNCLIPPED_LINES(drawmode)) {
      bShapeNeedsClipping = MS_FALSE;
    }
  } else {
    bShapeNeedsClipping = MS_FALSE;
  }

  if(layer->transform == MS_TRUE && bShapeNeedsClipping) {
    /* compute the size of the clipping buffer, in pixels. This buffer must account
     for the size of symbols drawn to avoid artifacts around the image edges */
    int clip_buf = 0;
    int s;
    rectObj cliprect;
    for (s=0;s<layer->class[c]->numstyles;s++) {
      double maxsize, maxunscaledsize;
      symbolObj *symbol;
      styleObj *style = layer->class[c]->styles[s];
      if (!MS_IS_VALID_ARRAY_INDEX(style->symbol, map->symbolset.numsymbols)) {
        msSetError(MS_SYMERR, "Invalid symbol index: %d", "msDrawShape()", style->symbol);
        return MS_FAILURE;
      }
      symbol = map->symbolset.symbol[style->symbol];
      if (symbol->type == MS_SYMBOL_PIXMAP) {
        if (MS_SUCCESS != msPreloadImageSymbol(MS_MAP_RENDERER(map), symbol))
          return MS_FAILURE;
      } else if (symbol->type == MS_SYMBOL_SVG) {
#ifdef USE_SVG_CAIRO
        if (MS_SUCCESS != msPreloadSVGSymbol(symbol))
          return MS_FAILURE;
#else
        msSetError(MS_SYMERR, "SVG symbol support is not enabled.", "msDrawShape()");
        return MS_FAILURE;
#endif
      }
      maxsize = MS_MAX(msSymbolGetDefaultSize(symbol), MS_MAX(style->size, style->width));
      maxunscaledsize = MS_MAX(style->minsize*image->resolutionfactor, style->minwidth*image->resolutionfactor);
      clip_buf = MS_MAX(clip_buf,MS_NINT(MS_MAX(maxsize * layer->scalefactor, maxunscaledsize) + 1));
    }


    /* if we need a copy of the unclipped shape, transform first, then clip to avoid transforming twice */
    if(bNeedUnclippedShape) {
      msTransformShape(shape, map->extent, map->cellsize, image);
      if(shape->numlines == 0) return MS_SUCCESS;
      msComputeBounds(shape);

      /* TODO: there's an optimization here that can be implemented:
         - no need to allocate unclipped_shape for each call to this function
         - the calls to msClipXXXRect will discard the original lineObjs, whereas
           we have just copied them because they where needed. These two functions
           could be changed so they are instructed not to free the original lineObjs. */
      unclipped_shape = (shapeObj *) msSmallMalloc(sizeof (shapeObj));
      msInitShape(unclipped_shape);
      msCopyShape(shape, unclipped_shape);
      if(shape->type == MS_SHAPE_POLYGON) {
         /* #179: additional buffer for polygons */
         clip_buf += 2;
      }

      cliprect.minx = cliprect.miny = -clip_buf;
      cliprect.maxx = image->width + clip_buf;
      cliprect.maxy = image->height + clip_buf;
      if(shape->type == MS_SHAPE_POLYGON) {
        msClipPolygonRect(shape, cliprect);
      } else {
        assert(shape->type == MS_SHAPE_LINE);
        msClipPolylineRect(shape, cliprect);
      }
      if(bNeedUnclippedAnnoShape) {
        anno_shape = unclipped_shape;
      } else {
        anno_shape = shape;
      }
    } else {
      /* clip first, then transform. This means we are clipping in geographical space */
      double clip_buf_d;
      if(shape->type == MS_SHAPE_POLYGON) {
         /*
          * add a small buffer around the cliping rectangle to
          * avoid lines around the edges : #179
          */
         clip_buf += 2;
      }
      clip_buf_d = clip_buf * map->cellsize;
      cliprect.minx = map->extent.minx - clip_buf_d;
      cliprect.miny = map->extent.miny - clip_buf_d;
      cliprect.maxx = map->extent.maxx + clip_buf_d;
      cliprect.maxy = map->extent.maxy + clip_buf_d;
      if(shape->type == MS_SHAPE_POLYGON) {
        msClipPolygonRect(shape, cliprect);
      } else {
        assert(shape->type == MS_SHAPE_LINE);
        msClipPolylineRect(shape, cliprect);
      }
      msTransformShape(shape, map->extent, map->cellsize, image);
      msComputeBounds(shape);
      anno_shape = shape;
    }

  } else {
    /* the shape is fully in the map extent,
     * or is a point type layer where out of bounds points are treated differently*/
    if (layer->transform == MS_TRUE) {
      msTransformShape(shape, map->extent, map->cellsize, image);
      msComputeBounds(shape);
    } else {
      msOffsetShapeRelativeTo(shape, layer);
    }
    anno_shape = shape;
  }
  if(shape->numlines == 0) {
    ret = MS_SUCCESS; /* error message is set in msBindLayerToShape() */
    goto draw_shape_cleanup;
  }

  if(msBindLayerToShape(layer, shape, drawmode) != MS_SUCCESS) {
    ret = MS_FAILURE; /* error message is set in msBindLayerToShape() */
    goto draw_shape_cleanup;
  }

  switch(layer->type) {
    case MS_LAYER_ANNOTATION:
      if(MS_DRAW_LABELS(drawmode))
        ret = annotationLayerDrawShape(map, image, layer, anno_shape);
      else
        ret = MS_SUCCESS;
      break;
    case MS_LAYER_LINE:
      ret = lineLayerDrawShape(map, image, layer, shape, anno_shape, unclipped_shape, style, drawmode);
      break;
    case MS_LAYER_POLYGON:
      ret = polygonLayerDrawShape(map, image, layer, shape, anno_shape, unclipped_shape, drawmode);
      break;
    case MS_LAYER_POINT:
    case MS_LAYER_RASTER:
      assert(0); //bug !
    default:
      msSetError(MS_MISCERR, "Unknown layer type.", "msDrawShape()");
      ret = MS_FAILURE;
  }

draw_shape_cleanup:
  msDrawEndShape(map,layer,image,shape);
  if(unclipped_shape != shape) {
    msFreeShape(unclipped_shape);
    msFree(unclipped_shape);
  }
  return ret;
}

/*
** Function to render an individual point, used as a helper function for mapscript only. Since a point
** can't carry attributes you can't do attribute based font size or angle.
*/
int msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, imageObj *image, int classindex, char *labeltext)
{
  int s;
  classObj *theclass=layer->class[classindex];
  labelObj *label=NULL;

#ifdef USE_PROJ
  if(layer->transform == MS_TRUE && layer->project && msProjectionsDiffer(&(layer->projection), &(map->projection)))
    msProjectPoint(&layer->projection, &map->projection, point);
  else
    layer->project = MS_FALSE;
#endif

  if(labeltext && theclass->numlabels > 0) {
    label = theclass->labels[0];

    msFree(label->annotext); /* free any previously allocated annotation text */
    if(labeltext && (label->encoding || label->wrap || label->maxlength))
      label->annotext = msTransformLabelText(map,label,labeltext); /* apply wrap character and encoding to the label text */
    else
      label->annotext = msStrdup(labeltext);
  }

  switch(layer->type) {
    case MS_LAYER_ANNOTATION:
      if(layer->transform == MS_TRUE) {
        if(!msPointInRect(point, &map->extent)) return(0);
        point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
        point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
      } else
        msOffsetPointRelativeTo(point, layer);

      if(labeltext) {
        if(layer->labelcache) {
          if(msAddLabel(map, label, layer->index, classindex, NULL, point, NULL, -1) != MS_SUCCESS) return(MS_FAILURE);
        } else {
          if(theclass->numstyles > 0 && MS_VALID_COLOR(theclass->styles[0]->color)) {
            for(s=0; s<theclass->numstyles; s++) {
              if(msScaleInBounds(map->scaledenom, theclass->styles[s]->minscaledenom, theclass->styles[s]->maxscaledenom))
                msDrawMarkerSymbol(&map->symbolset, image, point, theclass->styles[s], layer->scalefactor);
            }
          }
          msDrawLabel(map, image, *point, label->annotext, label, layer->scalefactor);
        }
      }
      break;

    case MS_LAYER_POINT:
      if(layer->transform == MS_TRUE) {
        if(!msPointInRect(point, &map->extent)) return(0);
        point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
        point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
      } else
        msOffsetPointRelativeTo(point, layer);

      for(s=0; s<theclass->numstyles; s++) {
        if(msScaleInBounds(map->scaledenom, theclass->styles[s]->minscaledenom, theclass->styles[s]->maxscaledenom))
          msDrawMarkerSymbol(&map->symbolset, image, point, theclass->styles[s], layer->scalefactor);
      }
      if(labeltext) {
        if(layer->labelcache) {
          if(msAddLabel(map, label, layer->index, classindex, NULL, point, NULL, -1) != MS_SUCCESS) return(MS_FAILURE);
        } else
          msDrawLabel(map, image, *point, label->annotext, label, layer->scalefactor);
      }
      break;
    default:
      break; /* don't do anything with layer of other types */
  }

  return(MS_SUCCESS); /* all done, no cleanup */
}

/*
** Draws a single label independently of the label cache. No collision avoidance is performed.
*/
int msDrawLabel(mapObj *map, imageObj *image, pointObj labelPnt, char *string, labelObj *label, double scalefactor)
{
  shapeObj labelPoly;
  lineObj labelPolyLine;
  pointObj labelPolyPoints[5];
  int needLabelPoly=MS_TRUE;
  int needLabelPoint=MS_TRUE;

  int label_offset_x, label_offset_y;
  double size;
  rectObj r;

  if(!string) return MS_SUCCESS; /* not an error, just don't need to do anything more */
  if(strlen(string) == 0) return MS_SUCCESS; /* not an error, just don't need to do anything more */



  if(label->type == MS_TRUETYPE) {
    size = label->size * scalefactor;
    size = MS_MAX(size, label->minsize*image->resolutionfactor);
    size = MS_MIN(size, label->maxsize*image->resolutionfactor);
  } else {
    size = label->size;
  }
  if(msGetLabelSize(map, label, string, size, &r, NULL)!= MS_SUCCESS)
    return MS_FAILURE;

  label_offset_x = label->offsetx*scalefactor;
  label_offset_y = label->offsety*scalefactor;

  if(label->position != MS_XY) {
    pointObj p;

    if(label->numstyles > 0) {
      int i;

      for(i=0; i<label->numstyles; i++) {
        if(label->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT) {
          msDrawMarkerSymbol(&map->symbolset, image, &labelPnt, label->styles[i], scalefactor);
        } else if(label->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOLY) {
          if(needLabelPoly) {
            labelPoly.line = &labelPolyLine; /* setup the label polygon structure */
            labelPoly.numlines = 1;
            labelPoly.line->point = labelPolyPoints;
            labelPoly.line->numpoints = 5;
            p = get_metrics_line(&labelPnt, label->position, r, label_offset_x, label_offset_y, label->angle, 1, labelPoly.line);
            needLabelPoint = MS_FALSE; /* don't re-compute */
            needLabelPoly = MS_FALSE;
          }
          msDrawShadeSymbol(&map->symbolset, image, &labelPoly, label->styles[i], scalefactor);
        } else {
          /* TODO: need error msg about unsupported geomtransform */
          return MS_FAILURE;
        }
      }
    }

    if(needLabelPoint)
      p = get_metrics_line(&labelPnt, label->position, r, label_offset_x, label_offset_y, label->angle, 0, NULL);

    /* draw the label text */
    msDrawText(image, p, string, label, &(map->fontset), scalefactor); /* actually draw the label */
  } else {
    labelPnt.x += label_offset_x;
    labelPnt.y += label_offset_y;

    if(label->numstyles > 0) {
      int i;

      for(i=0; i<label->numstyles; i++) {
        if(label->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT)
          msDrawMarkerSymbol(&map->symbolset, image, &labelPnt, label->styles[i], scalefactor);
        else if(label->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOLY) {
          if(needLabelPoly) {
            labelPoly.line = &labelPolyLine; /* setup the label polygon structure */
            labelPoly.numlines = 1;
            labelPoly.line->point = labelPolyPoints;
            labelPoly.line->numpoints = 5;
            get_metrics_line(&labelPnt, label->position, r, label_offset_x, label_offset_y, label->angle, 1, labelPoly.line);
            needLabelPoly = MS_FALSE; /* don't re-compute */
          }
          msDrawShadeSymbol(&map->symbolset, image, &labelPoly, label->styles[i], scalefactor);
        } else {
          /* TODO: need error msg about unsupported geomtransform */
          return MS_FAILURE;
        }
      }
    }

    /* draw the label text */
    msDrawText(image, labelPnt, string, label, &(map->fontset), scalefactor); /* actually draw the label */
  }

  return MS_SUCCESS;
}

/* private shortcut function to try a leader offsetted label */
void offsetAndTest(imageObj*image, mapObj *map, labelCacheMemberObj *cachePtr, double ox, double oy,
                   int priority, int label_idx, shapeObj *unoffsetedpoly)
{
  /* offset cachePtr->poly and cachePtr->point */
  int i,j;
  for(i=cachePtr->poly->numlines-1; i>=0; i--) {
    for(j=cachePtr->poly->line[i].numpoints-1; j>=0; j--) {
      cachePtr->poly->line[i].point[j].x = unoffsetedpoly->line[i].point[j].x + ox;
      cachePtr->poly->line[i].point[j].y = unoffsetedpoly->line[i].point[j].y + oy;
    }
  }
  cachePtr->poly->bounds.minx = unoffsetedpoly->bounds.minx + ox;
  cachePtr->poly->bounds.miny = unoffsetedpoly->bounds.miny + oy;
  cachePtr->poly->bounds.maxx = unoffsetedpoly->bounds.maxx + ox;
  cachePtr->poly->bounds.maxy = unoffsetedpoly->bounds.maxy + oy;

  cachePtr->point.x = cachePtr->leaderline->point[0].x + ox;
  cachePtr->point.y = cachePtr->leaderline->point[0].y + oy;

  /* set the second point of the leader line */
  cachePtr->leaderline->point[1].x = cachePtr->point.x;
  cachePtr->leaderline->point[1].y = cachePtr->point.y;

  /* compute leader line bbox */
  if(ox>0) {
    cachePtr->leaderbbox->minx = cachePtr->leaderline->point[0].x;
    cachePtr->leaderbbox->maxx = cachePtr->point.x;
  } else {
    cachePtr->leaderbbox->maxx = cachePtr->leaderline->point[0].x;
    cachePtr->leaderbbox->minx = cachePtr->point.x;
  }
  if(oy>0) {
    cachePtr->leaderbbox->miny = cachePtr->leaderline->point[0].y;
    cachePtr->leaderbbox->maxy = cachePtr->point.y;
  } else {
    cachePtr->leaderbbox->maxy = cachePtr->leaderline->point[0].y;
    cachePtr->leaderbbox->miny = cachePtr->point.y;
  }
  cachePtr->status = msTestLabelCacheCollisions(map, cachePtr, cachePtr->poly, cachePtr->labels[0].mindistance,priority,-label_idx);
  if(cachePtr->status) {
    int ll;
    for(ll=0; ll<cachePtr->numlabels; ll++) {
      cachePtr->labels[ll].annopoint.x += ox;
      cachePtr->labels[ll].annopoint.y += oy;
      if(cachePtr->labels[ll].annopoly) {
        for(i=0; i<5; i++) {
          cachePtr->labels[ll].annopoly->line[0].point[i].x += ox;
          cachePtr->labels[ll].annopoly->line[0].point[i].y += oy;
        }
      }
    }
  }
}

int msDrawOffsettedLabels(imageObj *image, mapObj *map, int priority)
{
  int retval = MS_SUCCESS;
  int l;
  labelCacheObj *labelcache = &(map->labelcache);
  labelCacheSlotObj *cacheslot;
  labelCacheMemberObj *cachePtr;
  assert(MS_RENDERER_PLUGIN(image->format));
  cacheslot = &(labelcache->slots[priority]);
  for(l=cacheslot->numlabels-1; l>=0; l--) {
    cachePtr = &(cacheslot->labels[l]); /* point to right spot in the label cache */
    if(cachePtr->status == MS_FALSE && !cachePtr->labelpath && cachePtr->poly) {
      /* only test regular labels that have had their bounding box computed
       and that haven't been rendered  */
      classObj *classPtr = (GET_CLASS(map,cachePtr->layerindex,cachePtr->classindex));
      layerObj *layerPtr = (GET_LAYER(map,cachePtr->layerindex));
      if(classPtr->leader.maxdistance) { /* only test labels that can be offsetted */
        shapeObj origPoly;
        int steps,i;
        if(cachePtr->point.x < labelcache->gutter ||
            cachePtr->point.y < labelcache->gutter ||
            cachePtr->point.x >= image->width - labelcache->gutter ||
            cachePtr->point.y >= image->height - labelcache->gutter) {
          /* don't look for leaders if point is in edge buffer as the leader line would end up chopped off */
          continue;
        }

        /* TODO: check the cachePtr->point doesn't intersect a rendered label before event trying to offset ?*/

        /* TODO: if the entry has a single label and it has position != CC,
         * recompute the cachePtr->poly and labelPtr->annopoint using POSITION CC */
        msInitShape(&origPoly);
        msCopyShape(cachePtr->poly,&origPoly);

        cachePtr->leaderline = msSmallMalloc(sizeof(lineObj));
        cachePtr->leaderline->numpoints = 2;
        cachePtr->leaderline->point = msSmallMalloc(2*sizeof(pointObj));
        cachePtr->leaderline->point[0] = cachePtr->point;
        cachePtr->leaderbbox = msSmallMalloc(sizeof(rectObj));

        steps = classPtr->leader.maxdistance / classPtr->leader.gridstep;

#define x0 (cachePtr->leaderline->point[0].x)
#define y0 (cachePtr->leaderline->point[0].y)
#define gridstepsc (classPtr->leader.gridstep)


#define otest(ox,oy) if((x0+(ox)) >= labelcache->gutter &&\
                  (y0+(oy)) >= labelcache->gutter &&\
                  (x0+(ox)) < image->width + labelcache->gutter &&\
                  (y0+(oy)) < image->height + labelcache->gutter) {\
                     offsetAndTest(image,map,cachePtr,(ox),(oy),priority,l,&origPoly); \
                     if(cachePtr->status) break;\
                  }

        /* loop through possible offsetted positions */
        for(i=1; i<=steps; i++) {




          /* test the intermediate points on the ring */

          /* (points marked "0" are the ones being tested)

             X00X00X
             0XXXXX0
             0XXXXX0
             XXX.XXX
             0XXXXX0
             0XXXXX0
             X00X00X
          */
          int j;
          for(j=1; j<i-1; j++) {
            /* test the right positions */
            otest(i*gridstepsc,j * gridstepsc);
            otest(i*gridstepsc,- j * gridstepsc);
            /* test the left positions */
            otest(- i*gridstepsc,j * gridstepsc);
            otest(- i*gridstepsc,- j * gridstepsc);
            /* test the top positions */
            otest(j*gridstepsc,- i * gridstepsc);
            otest(- j *gridstepsc,- i * gridstepsc);
            /* test the bottom positions */
            otest(j*gridstepsc,i * gridstepsc);
            otest(- j *gridstepsc,i * gridstepsc);
          }
          if(j<(i-1)) break;

          otest(i*gridstepsc,i*gridstepsc);
          otest(-i*gridstepsc,-i*gridstepsc);
          otest(i*gridstepsc,-i*gridstepsc);
          otest(-i*gridstepsc,i*gridstepsc);


          /* test the intermediate points on the ring */

          /* (points marked "0" are the ones being tested)

             X00X00X
             0XXXXX0
             0XXXXX0
             XXX.XXX
             0XXXXX0
             0XXXXX0
             X00X00X

          */

          /* test the extreme diagonal points */

          /* (points marked "0" are the ones being tested)

             0XXXXX0
             XXXXXXX
             XXXXXXX
             XXX.XXX
             XXXXXXX
             XXXXXXX
             0XXXXX0

             (x0+i*gridstep, y0+i*gridstep), pos lr
             (x0-i*gridstep, y0-i*gridstep), pos ul
             (x0+i*gridstep, y0-i*gridstep), pos ur
             (x0-i*gridstep, y0+i*gridstep), pos ll

          */

          /* test the 4 cardinal points on the ring */

          /* (points marked "0" are the ones being tested)

             XXX0XXX
             XXXXXXX
             XXXXXXX
             0XX.XX0
             XXXXXXX
             XXXXXXX
             XXX0XXX

           * (x0+i*gridtep,y0), pos cr

           * (x0-i*gridstep,y0), pos cl
           * (x0,y0-i*gridstep), pos uc
           * (x0,y0+i*gridstep), pos lc
           */
          otest(i*gridstepsc,0);
          otest(-i*gridstepsc,0);
          otest(0,-i*gridstepsc);
          otest(0,i*gridstepsc);
        }
        if(cachePtr->status) {
          int ll;
          shapeObj labelLeader; /* label polygon (bounding box, possibly rotated) */
          labelLeader.line = cachePtr->leaderline; /* setup the label polygon structure */
          labelLeader.numlines = 1;

          for(ll=0; ll<classPtr->leader.numstyles; ll++) {
            msDrawLineSymbol(&map->symbolset, image,&labelLeader , classPtr->leader.styles[ll], layerPtr->scalefactor);
          }
          for(ll=0; ll<cachePtr->numlabels; ll++) {
            labelObj *labelPtr = &(cachePtr->labels[ll]);

            /* here's where we draw the label styles */
            if(labelPtr->numstyles > 0) {
              for(i=0; i<labelPtr->numstyles; i++) {
                if(labelPtr->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT)
                  msDrawMarkerSymbol(&map->symbolset, image, &(cachePtr->point), labelPtr->styles[i], layerPtr->scalefactor);
                else if(labelPtr->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOLY) {
                  msDrawShadeSymbol(&map->symbolset, image, labelPtr->annopoly, labelPtr->styles[i], layerPtr->scalefactor);
                } else {
                  msSetError(MS_MISCERR,"Labels only support LABELPNT and LABELPOLY GEOMTRANSFORMS", "msDrawOffsettedLabels()");
                  return MS_FAILURE;
                }
              }
            }
            if(labelPtr->annotext)
              msDrawText(image, labelPtr->annopoint, labelPtr->annotext, labelPtr, &(map->fontset), layerPtr->scalefactor); /* actually draw the label */
          }
          /* TODO: draw cachePtr->marker, but where ? */

          /*
           styleObj tstyle;
           static int foo =0;
           if(!foo) {
              srand(time(NULL));
              foo = 1;
              initStyle(&tstyle);
              tstyle.width = 1;
              tstyle.color.alpha = 255;
           }
           tstyle.color.red = random()%255;
           tstyle.color.green = random()%255;
           tstyle.color.blue =random()%255;
           msDrawLineSymbol(&map->symbolset, image, cachePtr->poly, &tstyle, layerPtr->scalefactor);
          */

        } else {
          msFree(cachePtr->leaderline->point);
          msFree(cachePtr->leaderline);
          msFree(cachePtr->leaderbbox);
          cachePtr->leaderline = NULL;
        }
        msFreeShape(&origPoly);
      }
    }
  }


  return retval;
}

int computeMarkerPoly(mapObj *map, imageObj *image, labelCacheMemberObj *cachePtr,
                      labelCacheSlotObj *cacheslot, shapeObj *markerPoly)
{
  layerObj *layerPtr = (GET_LAYER(map, cachePtr->layerindex));
  if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) {
    /* TODO: at the moment only checks the bottom (first) marker style since it *should* be the
       largest, perhaps we should check all of them and build a composite size */
    double marker_width,marker_height;
    pointObj *point = markerPoly->line[0].point;
    if(msGetMarkerSize(&map->symbolset, &(cachePtr->styles[0]), &marker_width, &marker_height, layerPtr->scalefactor) != MS_SUCCESS)
      return MS_FAILURE;
    markerPoly->numlines = 1;
    markerPoly->bounds.minx = cachePtr->point.x - .5 * marker_width;
    markerPoly->bounds.miny = cachePtr->point.y - .5 * marker_height;
    markerPoly->bounds.maxx = markerPoly->bounds.minx + marker_width;
    markerPoly->bounds.maxy = markerPoly->bounds.miny + marker_height;
    point[0].x = markerPoly->bounds.minx;
    point[0].y = markerPoly->bounds.miny;
    point[1].x = markerPoly->bounds.minx;
    point[1].y = markerPoly->bounds.maxy;
    point[2].x = markerPoly->bounds.maxx;
    point[2].y = markerPoly->bounds.maxy;
    point[3].x = markerPoly->bounds.maxx;
    point[3].y = markerPoly->bounds.miny;
    point[4].x = markerPoly->bounds.minx;
    point[4].y = markerPoly->bounds.miny;
  }
  return MS_SUCCESS;
}

void fastComputeBounds(shapeObj *shape)
{
  int i,j;
  shape->bounds.minx = shape->bounds.maxx = shape->line[0].point[0].x;
  shape->bounds.miny = shape->bounds.maxy = shape->line[0].point[0].y;


  for( i=0; i<shape->numlines; i++ ) {
    for( j=0; j<shape->line[i].numpoints; j++ ) {
      shape->bounds.minx = MS_MIN(shape->bounds.minx, shape->line[i].point[j].x);
      shape->bounds.maxx = MS_MAX(shape->bounds.maxx, shape->line[i].point[j].x);
      shape->bounds.miny = MS_MIN(shape->bounds.miny, shape->line[i].point[j].y);
      shape->bounds.maxy = MS_MAX(shape->bounds.maxy, shape->line[i].point[j].y);
    }
  }
}

int computeLabelMarkerPoly(mapObj *map, imageObj *img, labelCacheMemberObj *cachePtr,
                           labelObj *label, shapeObj *markerPoly)
{
  int i;
  layerObj *layerPtr = (GET_LAYER(map, cachePtr->layerindex));
  markerPoly->numlines = 0;
  for (i=0; i<label->numstyles; i++) {
    styleObj *style = label->styles[i];
    if(style->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT &&
        style->symbol < map->symbolset.numsymbols && style->symbol > 0) {
      double sx,sy;
      pointObj *point;
      int p;
      double aox,aoy;
      symbolObj *symbol = map->symbolset.symbol[style->symbol];
      if(msGetMarkerSize(&map->symbolset, style, &sx, &sy, layerPtr->scalefactor) != MS_SUCCESS)
        return MS_FAILURE;
      point = markerPoly->line[0].point;
      point[0].x = sx / 2.0;
      point[0].y = sy / 2.0;
      point[1].x =  point[0].x;
      point[1].y = -point[0].y;
      point[2].x = -point[0].x;
      point[2].y = -point[0].y;
      point[3].x = -point[0].x;
      point[3].y =  point[0].y;
      point[4].x =  point[0].x;
      point[4].y =  point[0].y;
      if(symbol->anchorpoint_x != 0.5 || symbol->anchorpoint_y != 0.5) {
        aox = (0.5 - symbol->anchorpoint_x) * sx;
        aoy = (0.5 - symbol->anchorpoint_y) * sy;
        for(p=0; p<5; p++) {
          point[p].x += aox;
          point[p].y += aoy;
        }
      }
      if(style->angle) {
        double rot = -style->angle * MS_DEG_TO_RAD;
        double sina = sin(rot);
        double cosa = cos(rot);
        for(p=0; p<5; p++) {
          double tmpx = point[p].x;
          point[p].x = point[p].x * cosa - point[p].y * sina;
          point[p].y = tmpx * sina + point[p].y * cosa;
        }
      }
      aox = cachePtr->point.x + style->offsetx * layerPtr->scalefactor;
      aoy = cachePtr->point.y + style->offsety * layerPtr->scalefactor;
      for(p=0; p<5; p++) {
        point[p].x += aox;
        point[p].y += aoy;
      }
      markerPoly->numlines = 1;
      fastComputeBounds(markerPoly);
      break;
    }
  }
  return MS_SUCCESS;
}

int msDrawLabelCache(imageObj *image, mapObj *map)
{
  int nReturnVal = MS_SUCCESS;

  if(image) {
    if(MS_RENDERER_PLUGIN(image->format)) {
      int i, l, ll, priority;
      rectObj r;
      shapeObj marker_poly;
      lineObj  marker_line;
      pointObj marker_points[5];
      shapeObj label_marker_poly;
      lineObj  label_marker_line;
      pointObj label_marker_points[5];
      shapeObj metrics_poly;
      lineObj metrics_line;
      pointObj metrics_points[5];

      double marker_offset_x, marker_offset_y;
      int label_offset_x, label_offset_y;
      int label_mindistance=-1, label_buffer=0;
      const char *value;

      labelCacheMemberObj *cachePtr=NULL;
      layerObj *layerPtr=NULL;
      classObj *classPtr=NULL;
      labelObj *labelPtr=NULL;

      /* holds the contour of the label styles that correspond to markers */
      marker_line.point = marker_points;
      marker_line.numpoints = 5;
      msInitShape(&marker_poly);
      marker_poly.line = &marker_line;
      marker_poly.numlines = 0;
      marker_poly.type = MS_SHAPE_POLYGON;

      label_marker_line.point = label_marker_points;
      label_marker_line.numpoints = 5;
      msInitShape(&label_marker_poly);
      label_marker_poly.line = &label_marker_line;
      label_marker_poly.numlines = 0;
      label_marker_poly.type = MS_SHAPE_POLYGON;

      metrics_line.point = metrics_points;
      metrics_line.numpoints = 5;
      msInitShape(&metrics_poly);
      metrics_poly.line = &metrics_line;
      metrics_poly.numlines = 1;
      metrics_poly.type = MS_SHAPE_POLYGON;

      /* Look for labelcache_map_edge_buffer map metadata
       * If set then the value defines a buffer (in pixels) along the edge of the
       * map image where labels can't fall
       */
      if((value = msLookupHashTable(&(map->web.metadata), "labelcache_map_edge_buffer")) != NULL) {
        map->labelcache.gutter = MS_ABS(atoi(value));
        if(map->debug) msDebug("msDrawLabelCache(): labelcache_map_edge_buffer = %d\n", map->labelcache.gutter);
      }

      for(priority=MS_MAX_LABEL_PRIORITY-1; priority>=0; priority--) {
        labelCacheSlotObj *cacheslot;
        cacheslot = &(map->labelcache.slots[priority]);

        for(l=cacheslot->numlabels-1; l>=0; l--) {
          double scalefactor,size;
          cachePtr = &(cacheslot->labels[l]); /* point to right spot in the label cache */

          layerPtr = (GET_LAYER(map, cachePtr->layerindex)); /* set a couple of other pointers, avoids nasty references */
          classPtr = (GET_CLASS(map, cachePtr->layerindex, cachePtr->classindex));

          if (layerPtr->type == MS_LAYER_ANNOTATION && (cachePtr->numlabels > 1 || classPtr->leader.maxdistance)) {
            msSetError(MS_MISCERR, "Multiple Labels and/or LEADERs are not supported with annotation layers", "msDrawLabelCache()");
            return MS_FAILURE;
          }

          /* TODO: classes with a labelpath do not respect multi label rendering */
          if(cachePtr->labelpath) { /* path-based label */
            labelPtr = &(cachePtr->labels[0]);

            if(labelPtr->status != MS_ON) continue; /* skip this label */
            if(!labelPtr->annotext) continue; /* skip this label, nothing to with curved labels when there is nothing to draw */

            /* path-following labels *must* be TRUETYPE */
            size = labelPtr->size * layerPtr->scalefactor;
            size = MS_MAX(size, labelPtr->minsize*image->resolutionfactor);
            size = MS_MIN(size, labelPtr->maxsize*image->resolutionfactor);
            scalefactor = size / labelPtr->size;

            label_buffer = (labelPtr->buffer*image->resolutionfactor);
            label_mindistance = (labelPtr->mindistance*image->resolutionfactor);


            /* compare against image bounds, rendered labels and markers (sets cachePtr->status), if FORCE=TRUE then skip it */
            cachePtr->status = MS_TRUE;
            assert(cachePtr->poly == NULL);

            if(!labelPtr->force)
              cachePtr->status = msTestLabelCacheCollisions(map,cachePtr,&cachePtr->labelpath->bounds,label_mindistance,priority,l);


            if(!cachePtr->status) {
              msFreeShape(&cachePtr->labelpath->bounds);
              continue;
            } else {
              /* take ownership of cachePtr->poly*/
              cachePtr->poly = (shapeObj*)msSmallMalloc(sizeof(shapeObj));
              msInitShape(cachePtr->poly);
              cachePtr->poly->type = MS_SHAPE_POLYGON;
              cachePtr->poly->numlines = cachePtr->labelpath->bounds.numlines;
              cachePtr->poly->line = cachePtr->labelpath->bounds.line;
              cachePtr->labelpath->bounds.numlines = 0;
              cachePtr->labelpath->bounds.line = NULL;
              cachePtr->poly->bounds.minx = cachePtr->labelpath->bounds.bounds.minx;
              cachePtr->poly->bounds.miny = cachePtr->labelpath->bounds.bounds.miny;
              cachePtr->poly->bounds.maxx = cachePtr->labelpath->bounds.bounds.maxx;
              cachePtr->poly->bounds.maxy = cachePtr->labelpath->bounds.bounds.maxy;
              msFreeShape(&cachePtr->labelpath->bounds);
            }

            msDrawTextLine(image, labelPtr->annotext, labelPtr, cachePtr->labelpath, &(map->fontset), layerPtr->scalefactor); /* Draw the curved label */

          } else { /* point-based label */
            scalefactor = layerPtr->scalefactor; /* FIXME avoid compiler warning, see also #4408  */
            marker_offset_x = marker_offset_y = 0; /* assume no marker */

            /* compute label bbox of a marker used in an annotation layer and/or
             * the offset needed for point layer with markerPtr */
            marker_poly.numlines = 0;
            if(layerPtr->type == MS_LAYER_ANNOTATION) {
              if(computeMarkerPoly(map,image,cachePtr,cacheslot,&marker_poly)!=MS_SUCCESS) return MS_FAILURE;
              if(marker_poly.numlines) {
                marker_offset_x = (marker_poly.bounds.maxx-marker_poly.bounds.minx)/2.0;
                marker_offset_y = (marker_poly.bounds.maxy-marker_poly.bounds.miny)/2.0;
                /* if this is an annotation layer, transfer the markerPoly */
                if( MS_OFF == msTestLabelCacheCollisions(map, cachePtr, &marker_poly, 0,priority, l)) {
                  continue; /* the marker collided, no point continuing */
                }
              }
            } else if (layerPtr->type == MS_LAYER_POINT && cachePtr->markerid!=-1) { /* there is a marker already in the image that we need to account for */
              markerCacheMemberObj *markerPtr = &(cacheslot->markers[cachePtr->markerid]); /* point to the right spot in the marker cache*/
              marker_offset_x = (markerPtr->poly->bounds.maxx-markerPtr->poly->bounds.minx)/2.0;
              marker_offset_y = (markerPtr->poly->bounds.maxy-markerPtr->poly->bounds.miny)/2.0;
            }


            /*
            ** all other cases *could* have multiple labels defined
            */
            cachePtr->status = MS_OFF; /* assume this cache element *can't* be placed */
            for(ll=0; ll<cachePtr->numlabels; ll++) { /* RFC 77 TODO: Still may want to step through backwards... */
              int label_marker_status = MS_ON;
              labelPtr = &(cachePtr->labels[ll]);
              labelPtr->status = MS_OFF;

              /* first check if there's anything to do with this label */
              if(!labelPtr->annotext) {
                int s;
                for(s=0; s<labelPtr->numstyles; s++) {
                  if(labelPtr->styles[s]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT)
                    break;
                }
                if(s == labelPtr->numstyles) {
                  /* no label text, and no markers to render, skip this label */
                  labelPtr->status = MS_ON;
                  continue;
                }
              }

              /* compute the poly of the label styles */
              if(computeLabelMarkerPoly(map,image,cachePtr,labelPtr,&label_marker_poly)!=MS_SUCCESS) return MS_FAILURE;
              if(label_marker_poly.numlines) {
                if(cachePtr->numlabels > 1) { /* FIXME this test doesn't seem right, should probably check if we have an annotation layer with a regular style defined */
                  marker_offset_x = (label_marker_poly.bounds.maxx-label_marker_poly.bounds.minx)/2.0;
                  marker_offset_y = (label_marker_poly.bounds.maxy-label_marker_poly.bounds.miny)/2.0;
                } else {
                  /* we might be using an old style behavior with a markerPtr */
                  marker_offset_x = MS_MAX(marker_offset_x,(label_marker_poly.bounds.maxx-label_marker_poly.bounds.minx)/2.0);
                  marker_offset_y = MS_MAX(marker_offset_y,(label_marker_poly.bounds.maxy-label_marker_poly.bounds.miny)/2.0);
                }
                /* add marker to cachePtr->poly */
                if(labelPtr->force != MS_TRUE) {
                  label_marker_status = msTestLabelCacheCollisions(map, cachePtr,&label_marker_poly, 0,priority, l);
                }
                if(label_marker_status == MS_OFF &&
                    !(labelPtr->force || classPtr->leader.maxdistance))
                  break; /* the marker collided, break from multi-label loop */
              }


              if(labelPtr->annotext) {
                /* compute label size */
                if(labelPtr->type == MS_TRUETYPE) {
                  size = labelPtr->size * layerPtr->scalefactor;
                  size = MS_MAX(size, labelPtr->minsize*image->resolutionfactor);
                  size = MS_MIN(size, labelPtr->maxsize*image->resolutionfactor);
                  scalefactor = size / labelPtr->size;
                } else {
                  size = labelPtr->size;
                  scalefactor = 1;
                }
                if(msGetLabelSize(map, labelPtr, labelPtr->annotext, size, &r, NULL) != MS_SUCCESS)
                  return MS_FAILURE;
                /* if our label has an outline, adjust the marker offset so the outlinecolor does
                 * not bleed into the marker */
                if(marker_offset_x && MS_VALID_COLOR(labelPtr->outlinecolor)) {
                  marker_offset_x += labelPtr->outlinewidth/2.0;
                  marker_offset_y += labelPtr->outlinewidth/2.0;
                }

                if(labelPtr->autominfeaturesize && (cachePtr->featuresize != -1) && ((r.maxx-r.minx) > cachePtr->featuresize)) {
                  /* label too large relative to the feature */
                  /* this label cannot be rendered, go on to next cachePtr */
                  break;
                  /* TODO: treat the case with multiple labels and/or leader lines */
                }

                /* apply offset and buffer settings */
                label_offset_x = labelPtr->offsetx*scalefactor;
                label_offset_y = labelPtr->offsety*scalefactor;
                label_buffer = (labelPtr->buffer*image->resolutionfactor);
                label_mindistance = (labelPtr->mindistance*image->resolutionfactor);

#ifdef oldcode
                /* adjust the baseline (see #1449) */
                if(labelPtr->type == MS_TRUETYPE) {
                  char *lastline = strrchr(labelPtr->annotext,'\n');
                  if(!lastline || !*(++lastline)) {
                    label_offset_y += ((r.miny + r.maxy) + size) / 2.0;
                  } else {
                    rectObj rect2; /* bbox of first line only */
                    msGetLabelSize(map, labelPtr, lastline, size, &rect2, NULL);
                    label_offset_y += ((rect2.miny+rect2.maxy) + size) / 2.0;
                  }
                }
#endif


                /* compute the label annopoly  if we need to render the background billboard */
                if(labelPtr->numstyles > 0) {
                  for(i=0; i<labelPtr->numstyles; i++) {
                    if(labelPtr->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOLY) {
                      /* initialize the annotation polygon */
                      labelPtr->annopoly = (shapeObj *) msSmallMalloc(sizeof(shapeObj));
                      msInitShape(labelPtr->annopoly);
                      labelPtr->annopoly->line = (lineObj *) malloc(sizeof(lineObj));
                      labelPtr->annopoly->numlines = 1;
                      labelPtr->annopoly->line->point =  (pointObj *) malloc(5*sizeof(pointObj));
                      labelPtr->annopoly->line->numpoints = 5;
                      break;
                    }
                  }
                }
              }

              /* TODO: no point in using auto positionning if the marker cannot be placed? */
              if(labelPtr->annotext && labelPtr->position == MS_AUTO) {
                /* no point in using auto positionning if the marker cannot be placed */
                int positions[MS_POSITIONS_LENGTH], npositions=0;

                /*
                ** If the ANNOTATION has an associated marker then the position is handled like a point regardless of underlying shape type. (#2993)
                **   (Note: might be able to re-order this for more speed.)
                */
                if((layerPtr->type == MS_LAYER_POLYGON && marker_offset_x==0 )|| (layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->shapetype == MS_SHAPE_POLYGON && cachePtr->numstyles == 0)) {
                  positions[0]=MS_CC;
                  positions[1]=MS_UC;
                  positions[2]=MS_LC;
                  positions[3]=MS_CL;
                  positions[4]=MS_CR;
                  npositions = 5;
                } else if((layerPtr->type == MS_LAYER_LINE && marker_offset_x == 0) || (layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->shapetype == MS_SHAPE_LINE && cachePtr->numstyles == 0)) {
                  positions[0]=MS_UC;
                  positions[1]=MS_LC;
                  positions[2]=MS_CC;
                  npositions = 3;
                } else {
                  positions[0]=MS_UL;
                  positions[1]=MS_LR;
                  positions[2]=MS_UR;
                  positions[3]=MS_LL;
                  positions[4]=MS_CR;
                  positions[5]=MS_CL;
                  positions[6]=MS_UC;
                  positions[7]=MS_LC;
                  npositions = 8;
                }

                for(i=0; i<npositions; i++) {
                  // RFC 77 TODO: take label_marker_offset_x/y into account
                  labelPtr->annopoint = get_metrics_line(&(cachePtr->point), positions[i], r,
                                                         marker_offset_x + label_offset_x, marker_offset_y + label_offset_y,
                                                         labelPtr->angle, label_buffer, &metrics_line);
                  fastComputeBounds(&metrics_poly);

                  if(labelPtr->force == MS_OFF) {
                    /* check for collisions inside the label group */
                    if(cachePtr->poly && cachePtr->poly->numlines && intersectLabelPolygons(&metrics_poly, cachePtr->poly) == MS_TRUE) {
                      /* there was a self intersection */
                      continue; /* next position, labelPtr->status is left to MS_OFF */
                    }
                  }

                  labelPtr->status = msTestLabelCacheCollisions(map, cachePtr,&metrics_poly, label_mindistance,priority, l);

                  /* found a suitable place for this label */
                  if(labelPtr->status == MS_TRUE || (i==(npositions-1) && labelPtr->force == MS_ON)) {
                    labelPtr->status = MS_TRUE; /* set to true in case we are forcing it */
                    if(labelPtr->annopoly) get_metrics_line(&(cachePtr->point), positions[i], r,
                                                              marker_offset_x + label_offset_x, marker_offset_y + label_offset_y,
                                                              labelPtr->angle, 1, labelPtr->annopoly->line);
                    break; /* ...out of position loop */
                  }
                } /* next position */

                /* if position auto didn't manage to find a position, but we have leader configured
                 * for the class, then we want to compute the label poly anyways */
                if(classPtr->leader.maxdistance && labelPtr->status == MS_FALSE) {
                  labelPtr->annopoint = get_metrics_line(&(cachePtr->point), MS_CC, r,
                                                         marker_offset_x + label_offset_x, marker_offset_y + label_offset_y,
                                                         labelPtr->angle, label_buffer, &metrics_line);
                  if(labelPtr->annopoly) get_metrics_line(&(cachePtr->point), MS_CC, r,
                                                            marker_offset_x + label_offset_x, marker_offset_y + label_offset_y,
                                                            labelPtr->angle, 1, labelPtr->annopoly->line);
                  fastComputeBounds(&metrics_poly);
                }
              } else { /* explicit position */

                if(labelPtr->annotext) {
                  if(labelPtr->position == MS_CC) { /* don't need the marker_offset */
                    labelPtr->annopoint = get_metrics_line(&(cachePtr->point), labelPtr->position, r,
                                                           label_offset_x, label_offset_y, labelPtr->angle, label_buffer, &metrics_line);
                    if(labelPtr->annopoly) get_metrics_line(&(cachePtr->point), labelPtr->position, r,
                                                              label_offset_x, label_offset_y, labelPtr->angle, 1, labelPtr->annopoly->line);
                  } else {
                    labelPtr->annopoint = get_metrics_line(&(cachePtr->point), labelPtr->position, r,
                                                           marker_offset_x + label_offset_x, marker_offset_y + label_offset_y,
                                                           labelPtr->angle, label_buffer, &metrics_line);
                    if(labelPtr->annopoly) get_metrics_line(&(cachePtr->point), labelPtr->position, r,
                                                              marker_offset_x + label_offset_x, marker_offset_y + label_offset_y,
                                                              labelPtr->angle, 1, labelPtr->annopoly->line);
                  }
                  fastComputeBounds(&metrics_poly);

                  if(labelPtr->force == MS_ON) {
                    labelPtr->status = MS_ON;
                  } else {
                    if(labelPtr->force == MS_OFF) {
                      /* check for collisions inside the label group unless the label is FORCE GROUP */
                      if(cachePtr->poly && cachePtr->poly->numlines && intersectLabelPolygons(&metrics_poly, cachePtr->poly) == MS_TRUE) {
                        break; /* collision within the group */
                      }
                    }
                    /* TODO: in case we have leader lines and multiple labels, there's no use in testing for labelcache collisions
                    * once a first collision has been found. we only need to know that the label group has collided, and the
                    * poly of the whole label group: if(label_group) testLabelCacheCollisions */
                    labelPtr->status = msTestLabelCacheCollisions(map, cachePtr,&metrics_poly, label_mindistance, priority, l);
                  }
                } else {
                  labelPtr->status = MS_ON;
                }
              } /* end POSITION AUTO vs Fixed POSITION */

              if((!labelPtr->status || !label_marker_status) && classPtr->leader.maxdistance == 0) {
                break; /* no point looking at more labels, unless their is a leader defined, in which
                case we still want to compute the full cachePtr->poly to be used for offset tests */
                labelPtr->status = MS_OFF;
              } else {
                if(!cachePtr->poly) {
                  cachePtr->poly = (shapeObj*)msSmallMalloc(sizeof(shapeObj));
                  msInitShape(cachePtr->poly);
                }
                if(labelPtr->annotext) {
                  msAddLine(cachePtr->poly, metrics_poly.line);
                }
                if(label_marker_poly.numlines) {
                  msAddLine(cachePtr->poly, label_marker_poly.line);
                }
                if(!label_marker_status)
                  labelPtr->status = MS_OFF;
              }
            } /* next label in the group */


            /*
             * cachePtr->status can be set to ON only if all it's labels didn't collide
             */
            cachePtr->status = MS_ON;
            for(ll=0; ll<cachePtr->numlabels; ll++) {
              if(cachePtr->labels[ll].status == MS_OFF) {
                cachePtr->status = MS_OFF;
                break;
              }
            }
            if(cachePtr->status == MS_ON || classPtr->leader.maxdistance) {
              /* add the marker polygon if we have one */
              if(marker_poly.numlines) {
                if(!cachePtr->poly) {
                  cachePtr->poly = (shapeObj*)msSmallMalloc(sizeof(shapeObj));
                  msInitShape(cachePtr->poly);
                }
                msAddLine(cachePtr->poly, marker_poly.line);
              }
              if(cachePtr->poly)
                msComputeBounds(cachePtr->poly);
            }

            if(cachePtr->status == MS_OFF)
              continue; /* next label, as we had a collision */


            if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) { /* need to draw a marker */
              for(i=0; i<cachePtr->numstyles; i++)
                msDrawMarkerSymbol(&map->symbolset, image, &(cachePtr->point), &(cachePtr->styles[i]), layerPtr->scalefactor);
            }

            for(ll=0; ll<cachePtr->numlabels; ll++) {
              labelPtr = &(cachePtr->labels[ll]);

              /* here's where we draw the label styles */
              if(labelPtr->numstyles > 0) {
                for(i=0; i<labelPtr->numstyles; i++) {
                  if(labelPtr->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT)
                    msDrawMarkerSymbol(&map->symbolset, image, &(cachePtr->point), labelPtr->styles[i], layerPtr->scalefactor);
                  else if(labelPtr->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOLY && labelPtr->annotext) {
                    msDrawShadeSymbol(&map->symbolset, image, labelPtr->annopoly, labelPtr->styles[i], scalefactor); /* FIXME: scalefactor here should be adjusted by the label minsize/maxsize adjustments of each label, not only the last one. see #4408 */
                  } else {
                    msSetError(MS_MISCERR,"Labels only support LABELPNT and LABELPOLY GEOMTRANSFORMS", "msDrawLabelCAche()");
                    return MS_FAILURE;
                  }
                }
              }
              if(labelPtr->annotext)
                msDrawText(image, labelPtr->annopoint, labelPtr->annotext, labelPtr, &(map->fontset), layerPtr->scalefactor); /* actually draw the label */
            }
          } /* end else */
        } /* next label(group) from cacheslot */
        msDrawOffsettedLabels(image, map, priority);
      } /* next priority */
#ifdef TBDEBUG
      styleObj tstyle;
      initStyle(&tstyle);
      tstyle.width = 1;
      tstyle.color.alpha = 255;
      tstyle.color.red = 255;
      tstyle.color.green = 0;
      tstyle.color.blue = 0;
      for(priority=MS_MAX_LABEL_PRIORITY-1; priority>=0; priority--) {
        labelCacheSlotObj *cacheslot;
        cacheslot = &(map->labelcache.slots[priority]);

        for(l=cacheslot->numlabels-1; l>=0; l--) {
          cachePtr = &(cacheslot->labels[l]); /* point to right spot in the label cache */
          /*
          assert((cachePtr->poly == NULL && cachePtr->status == MS_OFF) ||
                  (cachePtr->poly && (cachePtr->status == MS_ON));
           */
          if(cachePtr->status) {
            msDrawLineSymbol(&map->symbolset, image, cachePtr->poly, &tstyle, layerPtr->scalefactor);
          }
        }
      }
#endif

      return MS_SUCCESS; /* necessary? */
    }
  }

  return nReturnVal;
}


/**
 * Generic function to tell the underline device that layer
 * drawing is stating
 */

void msImageStartLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  if (image) {
    if( MS_RENDERER_PLUGIN(image->format) ) {
      char *approximation_scale = msLayerGetProcessingKey( layer, "APPROXIMATION_SCALE" );
      if(approximation_scale) {
        if(!strncasecmp(approximation_scale,"ROUND",5)) {
          MS_IMAGE_RENDERER(image)->transform_mode = MS_TRANSFORM_ROUND;
        } else if(!strncasecmp(approximation_scale,"FULL",4)) {
          MS_IMAGE_RENDERER(image)->transform_mode = MS_TRANSFORM_FULLRESOLUTION;
        } else if(!strncasecmp(approximation_scale,"SIMPLIFY",8)) {
          MS_IMAGE_RENDERER(image)->transform_mode = MS_TRANSFORM_SIMPLIFY;
        } else {
          MS_IMAGE_RENDERER(image)->transform_mode = MS_TRANSFORM_SNAPTOGRID;
          MS_IMAGE_RENDERER(image)->approximation_scale = atof(approximation_scale);
        }
      } else {
        MS_IMAGE_RENDERER(image)->transform_mode = MS_IMAGE_RENDERER(image)->default_transform_mode;
        MS_IMAGE_RENDERER(image)->approximation_scale = MS_IMAGE_RENDERER(image)->default_approximation_scale;
      }
      MS_IMAGE_RENDERER(image)->startLayer(image, map, layer);
    } else if( MS_RENDERER_IMAGEMAP(image->format) )
      msImageStartLayerIM(map, layer, image);

  }
}


/**
 * Generic function to tell the underline device that layer
 * drawing is ending
 */

void msImageEndLayer(mapObj *map, layerObj *layer, imageObj *image)
{
  if(image) {
    if( MS_RENDERER_PLUGIN(image->format) ) {
      MS_IMAGE_RENDERER(image)->endLayer(image,map,layer);
    }
  }
}

/**
 * Generic function to tell the underline device that shape
 * drawing is stating
 */

void msDrawStartShape(mapObj *map, layerObj *layer, imageObj *image,
                      shapeObj *shape)
{
  if (image) {
    if(MS_RENDERER_PLUGIN(image->format)) {
      if (image->format->vtable->startShape)
        image->format->vtable->startShape(image, shape);
    }


  }
}


/**
 * Generic function to tell the underline device that shape
 * drawing is ending
 */

void msDrawEndShape(mapObj *map, layerObj *layer, imageObj *image,
                    shapeObj *shape)
{
  if(MS_RENDERER_PLUGIN(image->format)) {
    if (image->format->vtable->endShape)
      image->format->vtable->endShape(image, shape);
  }
}
/**
 * take the value from the shape and use it to change the
 * color in the style to match the range map
 */
int msShapeToRange(styleObj *style, shapeObj *shape)
{
  double fieldVal;
  char* fieldStr;

  /*first, get the value of the rangeitem, which should*/
  /*evaluate to a double*/
  fieldStr = shape->values[style->rangeitemindex];
  if (fieldStr == NULL) { /*if there's not value, bail*/
    return MS_FAILURE;
  }
  fieldVal = 0.0;
  fieldVal = atof(fieldStr); /*faith that it's ok -- */
  /*should switch to strtod*/
  return msValueToRange(style, fieldVal);
}

/**
 * Allow direct mapping of a value so that mapscript can use the
 * Ranges.  The styls passed in is updated to reflect the right color
 * based on the fieldVal
 */
int msValueToRange(styleObj *style, double fieldVal)
{
  double range;
  double scaledVal;

  range = style->maxvalue - style->minvalue;
  scaledVal = (fieldVal - style->minvalue)/range;

  /*At this point, we know where on the range we need to be*/
  /*However, we don't know how to map it yet, since RGB(A) can */
  /*Go up or down*/
  style->color.red = (int)(MS_MAX(0,(MS_MIN(255, (style->mincolor.red + ((style->maxcolor.red - style->mincolor.red) * scaledVal))))));
  style->color.green = (int)(MS_MAX(0,(MS_MIN(255,(style->mincolor.green + ((style->maxcolor.green - style->mincolor.green) * scaledVal))))));
  style->color.blue = (int)(MS_MAX(0,(MS_MIN(255,(style->mincolor.blue + ((style->maxcolor.blue - style->mincolor.blue) * scaledVal))))));
#ifdef USE_GD
  style->color.pen = MS_PEN_UNSET; /*so it will recalculate pen*/
#endif

  /*( "msMapRange(): %i %i %i", style->color.red , style->color.green, style->color.blue);*/

#if ALPHACOLOR_ENABLED
  /*NO ALPHA RANGE YET  style->color.alpha = style->mincolor.alpha + ((style->maxcolor.alpha - style->mincolor.alpha) * scaledVal);*/
#endif

  return MS_SUCCESS;
}

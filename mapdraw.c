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
#include "mapfile.h"
#include "mapows.h"

/* msPrepareImage()
 *
 * Returns a new imageObj ready for rendering the current map.
 *
 * If allow_nonsquare is set to MS_TRUE then the caller should call
 * msMapRestoreRealExtent() once they are done with the image.
 * This should be set to MS_TRUE only when called from msDrawMap(), see bug 945.
 */
imageObj *msPrepareImage(mapObj *map, int allow_nonsquare) {
  int i, status;
  imageObj *image = NULL;
  double geo_cellsize;

  if (map->width == -1 || map->height == -1) {
    msSetError(MS_MISCERR, "Image dimensions not specified.",
               "msPrepareImage()");
    return (NULL);
  }

  msFreeLabelCache(&(map->labelcache));
  msInitLabelCache(
      &(map->labelcache)); /* this clears any previously allocated cache */

  /* clear any previously created mask layer images */
  for (i = 0; i < map->numlayers; i++) {
    if (GET_LAYER(map, i)->maskimage) {
      msFreeImage(GET_LAYER(map, i)->maskimage);
      GET_LAYER(map, i)->maskimage = NULL;
    }
  }

  status = msValidateContexts(map); /* make sure there are no recursive REQUIRES
                                       or LABELREQUIRES expressions */
  if (status != MS_SUCCESS)
    return NULL;

  if (!map->outputformat) {
    msSetError(MS_IMGERR, "Map outputformat not set!", "msPrepareImage()");
    return (NULL);
  } else if (MS_RENDERER_PLUGIN(map->outputformat)) {
    rendererVTableObj *renderer = map->outputformat->vtable;
    colorObj *bg = &map->imagecolor;
    map->imagecolor.alpha = 255;

    image =
        renderer->createImage(map->width, map->height, map->outputformat, bg);
    if (image == NULL)
      return (NULL);
    image->format = map->outputformat;
    image->format->refcount++;
    image->width = map->width;
    image->height = map->height;

    image->resolution = map->resolution;
    image->resolutionfactor = map->resolution / map->defresolution;
    if (map->web.imagepath)
      image->imagepath = msStrdup(map->web.imagepath);
    if (map->web.imageurl)
      image->imageurl = msStrdup(map->web.imageurl);

  } else if (MS_RENDERER_IMAGEMAP(map->outputformat)) {
    image = msImageCreateIM(map->width, map->height, map->outputformat,
                            map->web.imagepath, map->web.imageurl,
                            map->resolution, map->defresolution);
  } else if (MS_RENDERER_RAWDATA(map->outputformat)) {
    image =
        msImageCreate(map->width, map->height, map->outputformat,
                      map->web.imagepath, map->web.imageurl, map->resolution,
                      map->defresolution, &map->imagecolor);
  } else {
    image = NULL;
  }

  if (!image) {
    msSetError(MS_IMGERR, "Unable to initialize image.", "msPrepareImage()");
    return (NULL);
  }

  image->map = map;

  /*
   * If we want to support nonsquare pixels, note that now, otherwise
   * adjust the extent size to have square pixels.
   *
   * If allow_nonsquare is set to MS_TRUE then the caller should call
   * msMapRestoreRealExtent() once they are done with the image.
   * This should be set to MS_TRUE only when called from msDrawMap(), see bug
   * 945.
   */
  if (allow_nonsquare && msTestConfigOption(map, "MS_NONSQUARE", MS_FALSE)) {
    double cellsize_x = (map->extent.maxx - map->extent.minx) / map->width;
    double cellsize_y = (map->extent.maxy - map->extent.miny) / map->height;

    if (cellsize_y != 0.0 && (fabs(cellsize_x / cellsize_y) > 1.00001 ||
                              fabs(cellsize_x / cellsize_y) < 0.99999)) {
      map->gt.need_geotransform = MS_TRUE;
      if (map->debug)
        msDebug(
            "msDrawMap(): kicking into non-square pixel preserving mode.\n");
    }
    map->cellsize = (cellsize_x * 0.5 + cellsize_y * 0.5);
  } else
    map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);

  status = msCalculateScale(map->extent, map->units, map->width, map->height,
                            map->resolution, &map->scaledenom);
  if (status != MS_SUCCESS) {
    msFreeImage(image);
    return (NULL);
  }

  /* update geotransform based on adjusted extent. */
  msMapComputeGeotransform(map);

  /* Do we need to fake out stuff for rotated support? */
  if (map->gt.need_geotransform)
    msMapSetFakedExtent(map);

  /* We will need a cellsize that represents a real georeferenced */
  /* coordinate cellsize here, so compute it from saved extents.   */

  geo_cellsize = map->cellsize;
  if (map->gt.need_geotransform == MS_TRUE) {
    double cellsize_x =
        (map->saved_extent.maxx - map->saved_extent.minx) / map->width;
    double cellsize_y =
        (map->saved_extent.maxy - map->saved_extent.miny) / map->height;

    geo_cellsize =
        sqrt(cellsize_x * cellsize_x + cellsize_y * cellsize_y) / sqrt(2.0);
  }

  /* compute layer/class/style/label scale factors now */
  for (int lid = 0; lid < map->numlayers; lid++) {
    layerObj *layer = GET_LAYER(map, lid);
    if (layer->sizeunits != MS_PIXELS)
      layer->scalefactor = (msInchesPerUnit(layer->sizeunits, 0) /
                            msInchesPerUnit(map->units, 0)) /
                           geo_cellsize;
    else if (layer->symbolscaledenom > 0 && map->scaledenom > 0)
      layer->scalefactor = layer->symbolscaledenom / map->scaledenom *
                           map->resolution / map->defresolution;
    else
      layer->scalefactor = map->resolution / map->defresolution;
    for (int cid = 0; cid < layer->numclasses; cid++) {
      classObj *class = GET_CLASS(map, lid, cid);
      if (class->sizeunits == MS_INHERIT)
        class->scalefactor = layer->scalefactor;
      else if (class->sizeunits != MS_PIXELS)
        class->scalefactor = (msInchesPerUnit(class->sizeunits, 0) /
                              msInchesPerUnit(map->units, 0)) /
                             geo_cellsize;
      else if (layer->symbolscaledenom > 0 && map->scaledenom > 0)
        class->scalefactor = layer->symbolscaledenom / map->scaledenom *
                             map->resolution / map->defresolution;
      else
        class->scalefactor = map->resolution / map->defresolution;
      for (int sid = 0; sid < class->numstyles; sid++) {
        styleObj *style = class->styles[sid];
        if (style->sizeunits == MS_INHERIT)
          style->scalefactor = class->scalefactor;
        else if (style->sizeunits != MS_PIXELS)
          style->scalefactor = (msInchesPerUnit(style->sizeunits, 0) /
                                msInchesPerUnit(map->units, 0)) /
                               geo_cellsize;
        else if (layer->symbolscaledenom > 0 && map->scaledenom > 0)
          style->scalefactor = layer->symbolscaledenom / map->scaledenom *
                               map->resolution / map->defresolution;
        else
          style->scalefactor = map->resolution / map->defresolution;
      }
      for (int sid = 0; sid < class->numlabels; sid++) {
        labelObj *label = class->labels[sid];
        if (label->sizeunits == MS_INHERIT)
          label->scalefactor = class->scalefactor;
        else if (label->sizeunits != MS_PIXELS)
          label->scalefactor = (msInchesPerUnit(label->sizeunits, 0) /
                                msInchesPerUnit(map->units, 0)) /
                               geo_cellsize;
        else if (layer->symbolscaledenom > 0 && map->scaledenom > 0)
          label->scalefactor = layer->symbolscaledenom / map->scaledenom *
                               map->resolution / map->defresolution;
        else
          label->scalefactor = map->resolution / map->defresolution;
        for (int lsid = 0; lsid < label->numstyles; lsid++) {
          styleObj *lstyle = label->styles[lsid];
          if (lstyle->sizeunits == MS_INHERIT)
            lstyle->scalefactor = label->scalefactor;
          else if (lstyle->sizeunits != MS_PIXELS)
            lstyle->scalefactor = (msInchesPerUnit(lstyle->sizeunits, 0) /
                                   msInchesPerUnit(map->units, 0)) /
                                  geo_cellsize;
          else if (layer->symbolscaledenom > 0 && map->scaledenom > 0)
            lstyle->scalefactor = layer->symbolscaledenom / map->scaledenom *
                                  map->resolution / map->defresolution;
          else
            lstyle->scalefactor = map->resolution / map->defresolution;
        }
      }
    }
  }

  image->refpt.x =
      MS_MAP2IMAGE_X_IC_DBL(0, map->extent.minx, 1.0 / map->cellsize);
  image->refpt.y =
      MS_MAP2IMAGE_Y_IC_DBL(0, map->extent.maxy, 1.0 / map->cellsize);

  return image;
}

static int msCompositeRasterBuffer(mapObj *map, imageObj *img,
                                   rasterBufferObj *rb, LayerCompositer *comp) {
  int ret = MS_SUCCESS;
  if (MS_IMAGE_RENDERER(img)->compositeRasterBuffer) {
    while (comp && ret == MS_SUCCESS) {
      rasterBufferObj *rb_ptr = rb;
      CompositingFilter *filter = comp->filter;
      if (filter && comp->next) {
        /* if we have another compositor to apply, then we need to copy the
         * rasterBufferObj. Otherwise we can work on it directly */
        rb_ptr = (rasterBufferObj *)msSmallCalloc(sizeof(rasterBufferObj), 1);
        msCopyRasterBuffer(rb_ptr, rb);
      }
      while (filter && ret == MS_SUCCESS) {
        ret = msApplyCompositingFilter(map, rb_ptr, filter);
        filter = filter->next;
      }
      if (ret == MS_SUCCESS)
        ret = MS_IMAGE_RENDERER(img)->compositeRasterBuffer(
            img, rb_ptr, comp->comp_op, comp->opacity);
      if (rb_ptr != rb) {
        msFreeRasterBuffer(rb_ptr);
        msFree(rb_ptr);
      }
      comp = comp->next;
    }
  } else {
    ret = MS_FAILURE;
  }
  return ret;
}

/*
 * Generic function to render the map file.
 * The type of the image created is based on the imagetype parameter in the map
 * file.
 *
 * mapObj *map - map object loaded in MapScript or via a mapfile to use
 * int querymap - is this map the result of a query operation, MS_TRUE|MS_FALSE
 */
imageObj *msDrawMap(mapObj *map, int querymap) {
  int i;
  layerObj *lp = NULL;
  int status = MS_FAILURE;
  imageObj *image = NULL;
  struct mstimeval mapstarttime = {0}, mapendtime = {0};
  struct mstimeval starttime = {0}, endtime = {0};

#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
  enum MS_CONNECTION_TYPE lastconnectiontype;
  httpRequestObj *pasOWSReqInfo = NULL;
  int numOWSLayers = 0;
  int numOWSRequests = 0;
  wmsParamsObj sLastWMSParams;
#endif

  if (map->debug >= MS_DEBUGLEVEL_TUNING)
    msGettimeofday(&mapstarttime, NULL);

  if (querymap) { /* use queryMapObj image dimensions */
    if (map->querymap.width > 0 && map->querymap.width <= map->maxsize)
      map->width = map->querymap.width;
    if (map->querymap.height > 0 && map->querymap.height <= map->maxsize)
      map->height = map->querymap.height;
  }

  msApplyMapConfigOptions(map);
  image = msPrepareImage(map, MS_TRUE);

  if (!image) {
    msSetError(MS_IMGERR, "Unable to initialize image.", "msDrawMap()");
    return (NULL);
  }

  if (map->debug >= MS_DEBUGLEVEL_DEBUG)
    msDebug("msDrawMap(): rendering using outputformat named %s (%s).\n",
            map->outputformat->name, map->outputformat->driver);

#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

  /* Time the OWS query phase */
  if (map->debug >= MS_DEBUGLEVEL_TUNING)
    msGettimeofday(&starttime, NULL);

  /* How many OWS (WMS/WFS) layers do we have to draw?
   * Note: numOWSLayers is the number of actual layers and numOWSRequests is
   * the number of HTTP requests which could be lower if multiple layers
   * are merged into the same request.
   */
  numOWSLayers = 0;
  for (i = 0; i < map->numlayers; i++) {
    if (map->layerorder[i] == -1)
      continue;

    lp = GET_LAYER(map, map->layerorder[i]);
    if (lp->connectiontype != MS_WMS && lp->connectiontype != MS_WFS) {
      continue;
    }
    numOWSLayers++;
  }

  if (numOWSLayers > 0) {

    /* Alloc and init pasOWSReqInfo...
     */
    pasOWSReqInfo =
        (httpRequestObj *)malloc(numOWSLayers * sizeof(httpRequestObj));
    if (pasOWSReqInfo == NULL) {
      msSetError(MS_MEMERR, "Allocation of httpRequestObj failed.",
                 "msDrawMap()");
      return NULL;
    }
    msHTTPInitRequestObj(pasOWSReqInfo, numOWSLayers);
    msInitWmsParamsObj(&sLastWMSParams);

    /* Pre-download all WMS/WFS layers in parallel before starting to draw map
     */
    lastconnectiontype = MS_SHAPEFILE;
    for (i = 0; i < map->numlayers; i++) {
      if (map->layerorder[i] == -1)
        continue;

      lp = GET_LAYER(map, map->layerorder[i]);
      if (lp->connectiontype != MS_WMS && lp->connectiontype != MS_WFS) {
        continue;
      }

      if (!msLayerIsVisible(map, lp))
        continue;

#ifdef USE_WMS_LYR
      if (lp->connectiontype == MS_WMS) {
        if (msPrepareWMSLayerRequest(map->layerorder[i], map, lp, 1,
                                     lastconnectiontype, &sLastWMSParams, 0, 0,
                                     0, NULL, pasOWSReqInfo,
                                     &numOWSRequests) == MS_FAILURE) {
          msFreeWmsParamsObj(&sLastWMSParams);
          msFreeImage(image);
          msFree(pasOWSReqInfo);
          return NULL;
        }
      }
#endif

#ifdef USE_WFS_LYR
      if (lp->connectiontype == MS_WFS) {
        if (msPrepareWFSLayerRequest(map->layerorder[i], map, lp, pasOWSReqInfo,
                                     &numOWSRequests) == MS_FAILURE) {
          msFreeWmsParamsObj(&sLastWMSParams);
          msFreeImage(image);
          msFree(pasOWSReqInfo);
          return NULL;
        }
      }
#endif

      lastconnectiontype = lp->connectiontype;
    }

    msFreeWmsParamsObj(&sLastWMSParams);
  } /* if numOWSLayers > 0 */

  if (numOWSRequests && msOWSExecuteRequests(pasOWSReqInfo, numOWSRequests, map,
                                             MS_TRUE) == MS_FAILURE) {
    msFreeImage(image);
    msFree(pasOWSReqInfo);
    return NULL;
  }

  if (map->debug >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&endtime, NULL);
    msDebug("msDrawMap(): WMS/WFS set-up and query, %.3fs\n",
            (endtime.tv_sec + endtime.tv_usec / 1.0e6) -
                (starttime.tv_sec + starttime.tv_usec / 1.0e6));
  }

#endif /* USE_WMS_LYR || USE_WFS_LYR */

  /* OK, now we can start drawing */
  for (i = 0; i < map->numlayers; i++) {

    if (map->layerorder[i] != -1) {
      const char *force_draw_label_cache = NULL;

      lp = (GET_LAYER(map, map->layerorder[i]));

      if (lp->postlabelcache) /* wait to draw */
        continue;

      if (map->debug >= MS_DEBUGLEVEL_TUNING ||
          lp->debug >= MS_DEBUGLEVEL_TUNING)
        msGettimeofday(&starttime, NULL);

      if (!msLayerIsVisible(map, lp))
        continue;

      if (lp->connectiontype == MS_WMS) {
#ifdef USE_WMS_LYR
        if (MS_RENDERER_PLUGIN(image->format) ||
            MS_RENDERER_RAWDATA(image->format)) {
          assert(pasOWSReqInfo);
          status = msDrawWMSLayerLow(map->layerorder[i], pasOWSReqInfo,
                                     numOWSRequests, map, lp, image);
        } else {
          msSetError(MS_WMSCONNERR,
                     "Output format '%s' doesn't support WMS layers.",
                     "msDrawMap()", image->format->name);
          status = MS_FAILURE;
        }

        if (status == MS_FAILURE) {
          msSetError(MS_WMSCONNERR,
                     "Failed to draw WMS layer named '%s'. This most likely "
                     "happened because "
                     "the remote WMS server returned an invalid image, and XML "
                     "exception "
                     "or another unexpected result in response to the GetMap "
                     "request. Also check "
                     "and make sure that the layer's connection URL is valid.",
                     "msDrawMap()", lp->name);
          msFreeImage(image);
          msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
          msFree(pasOWSReqInfo);
          return (NULL);
        }

#else /* ndef USE_WMS_LYR */
        msSetError(MS_WMSCONNERR,
                   "MapServer not built with WMS Client support, unable to "
                   "render layer '%s'.",
                   "msDrawMap()", lp->name);
        msFreeImage(image);
        return (NULL);
#endif
      } else { /* Default case: anything but WMS layers */
        if (querymap)
          status = msDrawQueryLayer(map, lp, image);
        else
          status = msDrawLayer(map, lp, image);
        if (status == MS_FAILURE) {
          msSetError(MS_IMGERR, "Failed to draw layer named '%s'.",
                     "msDrawMap()", lp->name);
          msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
          if (pasOWSReqInfo) {
            msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
            msFree(pasOWSReqInfo);
          }
#endif /* USE_WMS_LYR || USE_WFS_LYR */
          return (NULL);
        }
      }
      if (map->debug >= MS_DEBUGLEVEL_TUNING ||
          lp->debug >= MS_DEBUGLEVEL_TUNING) {
        msGettimeofday(&endtime, NULL);
        msDebug("msDrawMap(): Layer %d (%s), %.3fs\n", map->layerorder[i],
                lp->name ? lp->name : "(null)",
                (endtime.tv_sec + endtime.tv_usec / 1.0e6) -
                    (starttime.tv_sec + starttime.tv_usec / 1.0e6));
      }

      /* Flush layer cache in-between layers if requested by PROCESSING
       * directive*/
      force_draw_label_cache =
          msLayerGetProcessingKey(lp, "FORCE_DRAW_LABEL_CACHE");
      if (force_draw_label_cache &&
          strncasecmp(force_draw_label_cache, "FLUSH", 5) == 0) {
        if (map->debug >= MS_DEBUGLEVEL_V)
          msDebug(
              "msDrawMap(): PROCESSING FORCE_DRAW_LABEL_CACHE=FLUSH found.\n");
        if (msDrawLabelCache(map, image) != MS_SUCCESS) {
          msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
          if (pasOWSReqInfo) {
            msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
            msFree(pasOWSReqInfo);
          }
#endif /* USE_WMS_LYR || USE_WFS_LYR */
          return (NULL);
        }
        msFreeLabelCache(&(map->labelcache));
        msInitLabelCache(&(map->labelcache));
      } /* PROCESSING FORCE_DRAW_LABEL_CACHE */
    }
  }

  if (map->scalebar.status == MS_EMBED && !map->scalebar.postlabelcache) {

    /* We need to temporarily restore the original extent for drawing */
    /* the scalebar as it uses the extent to recompute cellsize. */
    if (map->gt.need_geotransform)
      msMapRestoreRealExtent(map);

    if (MS_SUCCESS != msEmbedScalebar(map, image)) {
      msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
      /* Cleanup WMS/WFS Request stuff */
      if (pasOWSReqInfo) {
        msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
        msFree(pasOWSReqInfo);
      }
#endif
      return NULL;
    }

    if (map->gt.need_geotransform)
      msMapSetFakedExtent(map);
  }

  if (map->legend.status == MS_EMBED && !map->legend.postlabelcache) {
    if (msEmbedLegend(map, image) != MS_SUCCESS) {
      msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
      /* Cleanup WMS/WFS Request stuff */
      if (pasOWSReqInfo) {
        msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
        msFree(pasOWSReqInfo);
      }
#endif
      return NULL;
    }
  }

  if (msDrawLabelCache(map, image) != MS_SUCCESS) {
    msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
    if (pasOWSReqInfo) {
      msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
      msFree(pasOWSReqInfo);
    }
#endif /* USE_WMS_LYR || USE_WFS_LYR */
    return (NULL);
  }

  for (i = 0; i < map->numlayers;
       i++) { /* for each layer, check for postlabelcache layers */

    lp = (GET_LAYER(map, map->layerorder[i]));

    if (!lp->postlabelcache)
      continue;
    if (!msLayerIsVisible(map, lp))
      continue;

    if (map->debug >= MS_DEBUGLEVEL_TUNING || lp->debug >= MS_DEBUGLEVEL_TUNING)
      msGettimeofday(&starttime, NULL);

    if (lp->connectiontype == MS_WMS) {
#ifdef USE_WMS_LYR
      if (MS_RENDERER_PLUGIN(image->format) ||
          MS_RENDERER_RAWDATA(image->format)) {
        assert(pasOWSReqInfo);
        status = msDrawWMSLayerLow(map->layerorder[i], pasOWSReqInfo,
                                   numOWSRequests, map, lp, image);
      }

#else
      status = MS_FAILURE;
#endif
    } else {
      if (querymap)
        status = msDrawQueryLayer(map, lp, image);
      else
        status = msDrawLayer(map, lp, image);
    }

    if (status == MS_FAILURE) {
      msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
      if (pasOWSReqInfo) {
        msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
        msFree(pasOWSReqInfo);
      }
#endif /* USE_WMS_LYR || USE_WFS_LYR */
      return (NULL);
    }

    if (map->debug >= MS_DEBUGLEVEL_TUNING ||
        lp->debug >= MS_DEBUGLEVEL_TUNING) {
      msGettimeofday(&endtime, NULL);
      msDebug("msDrawMap(): Layer %d (%s), %.3fs\n", map->layerorder[i],
              lp->name ? lp->name : "(null)",
              (endtime.tv_sec + endtime.tv_usec / 1.0e6) -
                  (starttime.tv_sec + starttime.tv_usec / 1.0e6));
    }
  }

  /* Do we need to fake out stuff for rotated support? */
  /* This really needs to be done on every preceeding exit point too... */
  if (map->gt.need_geotransform)
    msMapRestoreRealExtent(map);

  if (map->legend.status == MS_EMBED && map->legend.postlabelcache)
    if (MS_UNLIKELY(MS_FAILURE == msEmbedLegend(map, image))) {
      msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
      /* Cleanup WMS/WFS Request stuff */
      if (pasOWSReqInfo) {
        msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
        msFree(pasOWSReqInfo);
      }
#endif
      return NULL;
    }

  if (map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache) {

    /* We need to temporarily restore the original extent for drawing */
    /* the scalebar as it uses the extent to recompute cellsize. */
    if (map->gt.need_geotransform)
      msMapRestoreRealExtent(map);

    if (MS_SUCCESS != msEmbedScalebar(map, image)) {
      msFreeImage(image);
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
      /* Cleanup WMS/WFS Request stuff */
      if (pasOWSReqInfo) {
        msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
        msFree(pasOWSReqInfo);
      }
#endif
      return NULL;
    }

    if (map->gt.need_geotransform)
      msMapSetFakedExtent(map);
  }

#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
  /* Cleanup WMS/WFS Request stuff */
  if (pasOWSReqInfo) {
    msHTTPFreeRequestObj(pasOWSReqInfo, numOWSRequests);
    msFree(pasOWSReqInfo);
  }
#endif

  if (map->debug >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&mapendtime, NULL);
    msDebug("msDrawMap() total time: %.3fs\n",
            (mapendtime.tv_sec + mapendtime.tv_usec / 1.0e6) -
                (mapstarttime.tv_sec + mapstarttime.tv_usec / 1.0e6));
  }

  return (image);
}

/*
 * Test whether a layer should be drawn or not in the current map view and
 * at the current scale.
 * Returns TRUE if layer is visible, FALSE if not.
 */
int msLayerIsVisible(mapObj *map, layerObj *layer) {
  int i;

  if (!layer->data && !layer->tileindex && !layer->connection &&
      !layer->features && !layer->grid)
    return (MS_FALSE); /* no data associated with this layer, not an error since
                          layer may be used as a template from MapScript */

  if (layer->type == MS_LAYER_QUERY || layer->type == MS_LAYER_TILEINDEX)
    return (MS_FALSE);
  if ((layer->status != MS_ON) && (layer->status != MS_DEFAULT))
    return (MS_FALSE);

  /* Do comparisons of layer scale vs map scale now, since msExtentsOverlap() */
  /* can be slow */
  if (map->scaledenom > 0) {

    /* layer scale boundaries should be checked first */
    if ((layer->maxscaledenom > 0) &&
        (map->scaledenom > layer->maxscaledenom)) {
      if (layer->debug >= MS_DEBUGLEVEL_V) {
        msDebug("msLayerIsVisible(): Skipping layer (%s) because "
                "LAYER.MAXSCALE is too small for this MAP scale\n",
                layer->name);
      }
      return (MS_FALSE);
    }
    if (/*(layer->minscaledenom > 0) &&*/ (map->scaledenom <=
                                           layer->minscaledenom)) {
      if (layer->debug >= MS_DEBUGLEVEL_V) {
        msDebug("msLayerIsVisible(): Skipping layer (%s) because "
                "LAYER.MINSCALE is too large for this MAP scale\n",
                layer->name);
      }
      return (MS_FALSE);
    }
  }

  /* Only return MS_FALSE if it is definitely false. Sometimes it will return
   *MS_UNKNOWN, which we
   ** consider true, for this use case (it might be visible, try and draw it,
   *see what happens). */
  if (msExtentsOverlap(map, layer) == MS_FALSE) {
    if (layer->debug >= MS_DEBUGLEVEL_V) {
      msDebug("msLayerIsVisible(): Skipping layer (%s) because LAYER.EXTENT "
              "does not intersect MAP.EXTENT\n",
              layer->name);
    }
    return (MS_FALSE);
  }

  if (msEvalContext(map, layer, layer->requires) == MS_FALSE)
    return (MS_FALSE);

  if (map->scaledenom > 0) {

    /* now check class scale boundaries (all layers *must* pass these tests) */
    if (layer->numclasses > 0) {
      for (i = 0; i < layer->numclasses; i++) {
        if ((layer->class[i] -> maxscaledenom > 0) &&
            (map->scaledenom > layer->class[i] -> maxscaledenom))
          continue; /* can skip this one, next class */
        if ((layer->class[i] -> minscaledenom > 0) &&
            (map->scaledenom <= layer->class[i] -> minscaledenom))
          continue; /* can skip this one, next class */

        break; /* can't skip this class (or layer for that matter) */
      }
      if (i == layer->numclasses) {
        if (layer->debug >= MS_DEBUGLEVEL_V) {
          msDebug("msLayerIsVisible(): Skipping layer (%s) because no CLASS in "
                  "the layer is in-scale for this MAP scale\n",
                  layer->name);
        }
        return (MS_FALSE);
      }
    }
  }

  if (layer->maxscaledenom <= 0 && layer->minscaledenom <= 0) {
    if ((layer->maxgeowidth > 0) &&
        ((map->extent.maxx - map->extent.minx) > layer->maxgeowidth)) {
      if (layer->debug >= MS_DEBUGLEVEL_V) {
        msDebug("msLayerIsVisible(): Skipping layer (%s) because LAYER width "
                "is much smaller than map width\n",
                layer->name);
      }
      return (MS_FALSE);
    }
    if ((layer->mingeowidth > 0) &&
        ((map->extent.maxx - map->extent.minx) < layer->mingeowidth)) {
      if (layer->debug >= MS_DEBUGLEVEL_V) {
        msDebug("msLayerIsVisible(): Skipping layer (%s) because LAYER width "
                "is much larger than map width\n",
                layer->name);
      }
      return (MS_FALSE);
    }
  }

  return MS_TRUE; /* All tests passed.  Layer is visible. */
}

#define LAYER_NEEDS_COMPOSITING(layer)                                         \
  (((layer)->compositer != NULL) &&                                            \
   ((layer)->compositer->next || (layer)->compositor->opacity < 100 ||         \
    (layer)->compositor->compop != MS_COMPOP_SRC_OVER ||                       \
    (layer)->compositer->filter))
/*
 * Generic function to render a layer object.
 */
int msDrawLayer(mapObj *map, layerObj *layer, imageObj *image) {
  imageObj *image_draw = image;
  outputFormatObj *altFormat = NULL;
  int retcode = MS_SUCCESS;
  const char *alternativeFomatString = NULL;
  layerObj *maskLayer = NULL;

  if (!msLayerIsVisible(map, layer))
    return MS_SUCCESS;

  if (layer->compositer && !layer->compositer->next &&
      layer->compositer->opacity == 0)
    return MS_SUCCESS; /* layer is completely transparent, skip it */

  /* conditions may have changed since this layer last drawn, so retest
     layer->project (Bug #673) */
  layer->project =
      msProjectionsDiffer(&(layer->projection), &(map->projection));

  /* make sure labelcache setting is set correctly if postlabelcache is set.
     This is done by the parser but may have been altered by a mapscript. see
     #5142 */
  if (layer->postlabelcache) {
    layer->labelcache = MS_FALSE;
  }

  if (layer->mask) {
    int maskLayerIdx;
    /* render the mask layer in its own imageObj */
    if (!MS_IMAGE_RENDERER(image)->supports_pixel_buffer) {
      msSetError(MS_MISCERR,
                 "Layer (%s) references references a mask layer, but the "
                 "selected renderer does not support them",
                 "msDrawLayer()", layer->name);
      return (MS_FAILURE);
    }
    maskLayerIdx = msGetLayerIndex(map, layer->mask);
    if (maskLayerIdx == -1) {
      msSetError(MS_MISCERR, "Layer (%s) references unknown mask layer (%s)",
                 "msDrawLayer()", layer->name, layer->mask);
      return (MS_FAILURE);
    }
    maskLayer = GET_LAYER(map, maskLayerIdx);
    if (!maskLayer->maskimage) {
      int i;
      int origstatus, origlabelcache;
      altFormat = msSelectOutputFormat(map, "png24");
      msInitializeRendererVTable(altFormat);
      /* TODO: check the png24 format hasn't been tampered with, i.e. it's agg
       */
      maskLayer->maskimage = msImageCreate(
          image->width, image->height, altFormat, image->imagepath,
          image->imageurl, map->resolution, map->defresolution, NULL);
      if (!maskLayer->maskimage) {
        msSetError(MS_MISCERR, "Unable to initialize mask image.",
                   "msDrawLayer()");
        return (MS_FAILURE);
      }

      /*
       * force the masked layer to status on, and turn off the labelcache so
       * that eventual labels are added to the temporary image instead of being
       * added to the labelcache
       */
      origstatus = maskLayer->status;
      origlabelcache = maskLayer->labelcache;
      maskLayer->status = MS_ON;
      maskLayer->labelcache = MS_OFF;

      /* draw the mask layer in the temporary image */
      retcode = msDrawLayer(map, maskLayer, maskLayer->maskimage);
      maskLayer->status = origstatus;
      maskLayer->labelcache = origlabelcache;
      if (retcode != MS_SUCCESS) {
        return MS_FAILURE;
      }
      /*
       * hack to work around bug #3834: if we have use an alternate renderer,
       * the symbolset may contain symbols that reference it. We want to remove
       * those references before the altFormat is destroyed to avoid a segfault
       * and/or a leak, and so the the main renderer doesn't pick the cache up
       * thinking it's for him.
       */
      for (i = 0; i < map->symbolset.numsymbols; i++) {
        if (map->symbolset.symbol[i] != NULL) {
          symbolObj *s = map->symbolset.symbol[i];
          if (s->renderer == MS_IMAGE_RENDERER(maskLayer->maskimage)) {
            MS_IMAGE_RENDERER(maskLayer->maskimage)->freeSymbol(s);
            s->renderer = NULL;
          }
        }
      }
      /* set the imagetype from the original outputformat back (it was removed
       * by msSelectOutputFormat() */
      msFree(map->imagetype);
      map->imagetype = msStrdup(image->format->name);
    }
  }
  altFormat = NULL;
  /* inform the rendering device that layer draw is starting. */
  msImageStartLayer(map, layer, image);

  /*check if an alternative renderer should be used for this layer*/
  alternativeFomatString = msLayerGetProcessingKey(layer, "RENDERER");
  if (MS_RENDERER_PLUGIN(image_draw->format) &&
      alternativeFomatString != NULL &&
      (altFormat = msSelectOutputFormat(map, alternativeFomatString))) {
    rendererVTableObj *renderer = NULL;
    msInitializeRendererVTable(altFormat);

    image_draw = msImageCreate(
        image->width, image->height, altFormat, image->imagepath,
        image->imageurl, map->resolution, map->defresolution, &map->imagecolor);
    renderer = MS_IMAGE_RENDERER(image_draw);
    renderer->startLayer(image_draw, map, layer);
  } else if (MS_RENDERER_PLUGIN(image_draw->format)) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image_draw);
    if ((layer->mask && layer->connectiontype != MS_WMS &&
         layer->type != MS_LAYER_RASTER) ||
        layer->compositer) {
      /* masking occurs at the pixel/layer level for raster images, so we don't
       need to create a temporary image in these cases
       */
      if (layer->mask || renderer->compositeRasterBuffer) {
        image_draw = msImageCreate(image->width, image->height, image->format,
                                   image->imagepath, image->imageurl,
                                   map->resolution, map->defresolution, NULL);
        if (!image_draw) {
          msSetError(MS_MISCERR,
                     "Unable to initialize temporary transparent image.",
                     "msDrawLayer()");
          return (MS_FAILURE);
        }
        image_draw->map = map;
        renderer->startLayer(image_draw, map, layer);
      }
    }
  }
  /*
  ** redirect procesing of some layer types.
  */
  if (layer->connectiontype == MS_WMS) {
#ifdef USE_WMS_LYR
    retcode = msDrawWMSLayer(map, layer, image_draw);
#else
    retcode = MS_FAILURE;
#endif
  } else if (layer->type == MS_LAYER_RASTER) {
    retcode = msDrawRasterLayer(map, layer, image_draw);
  } else if (layer->type == MS_LAYER_CHART) {
    retcode = msDrawChartLayer(map, layer, image_draw);
  } else { /* must be a Vector layer */
    retcode = msDrawVectorLayer(map, layer, image_draw);
  }

  if (altFormat) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image);
    rendererVTableObj *altrenderer = MS_IMAGE_RENDERER(image_draw);
    rasterBufferObj rb;
    int i;
    memset(&rb, 0, sizeof(rasterBufferObj));

    altrenderer->endLayer(image_draw, map, layer);

    retcode = altrenderer->getRasterBufferHandle(image_draw, &rb);
    if (MS_UNLIKELY(retcode == MS_FAILURE)) {
      goto altformat_cleanup;
    }
    retcode = renderer->mergeRasterBuffer(
        image, &rb,
        ((layer->compositer) ? (layer->compositer->opacity * 0.01) : (1.0)), 0,
        0, 0, 0, rb.width, rb.height);
    if (MS_UNLIKELY(retcode == MS_FAILURE)) {
      goto altformat_cleanup;
    }

  altformat_cleanup:
    /*
     * hack to work around bug #3834: if we have use an alternate renderer, the
     * symbolset may contain symbols that reference it. We want to remove those
     * references before the altFormat is destroyed to avoid a segfault and/or a
     * leak, and so the the main renderer doesn't pick the cache up thinking
     * it's for him.
     */
    for (i = 0; i < map->symbolset.numsymbols; i++) {
      if (map->symbolset.symbol[i] != NULL) {
        symbolObj *s = map->symbolset.symbol[i];
        if (s->renderer == altrenderer) {
          altrenderer->freeSymbol(s);
          s->renderer = NULL;
        }
      }
    }
    msFreeImage(image_draw);
    /* set the imagetype from the original outputformat back (it was removed by
     * msSelectOutputFormat() */
    msFree(map->imagetype);
    map->imagetype = msStrdup(image->format->name);
  } else if (image != image_draw) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image_draw);
    rasterBufferObj rb;
    memset(&rb, 0, sizeof(rasterBufferObj));

    renderer->endLayer(image_draw, map, layer);

    retcode = renderer->getRasterBufferHandle(image_draw, &rb);
    if (MS_UNLIKELY(retcode == MS_FAILURE)) {
      goto imagedraw_cleanup;
    }
    if (maskLayer && maskLayer->maskimage) {
      rasterBufferObj mask;
      unsigned int row, col;
      memset(&mask, 0, sizeof(rasterBufferObj));
      retcode = MS_IMAGE_RENDERER(maskLayer->maskimage)
                    ->getRasterBufferHandle(maskLayer->maskimage, &mask);
      if (MS_UNLIKELY(retcode == MS_FAILURE)) {
        goto imagedraw_cleanup;
      }
      /* modify the pixels of the overlay */

      if (rb.type == MS_BUFFER_BYTE_RGBA) {
        for (row = 0; row < rb.height; row++) {
          unsigned char *ma, *a, *r, *g, *b;
          r = rb.data.rgba.r + row * rb.data.rgba.row_step;
          g = rb.data.rgba.g + row * rb.data.rgba.row_step;
          b = rb.data.rgba.b + row * rb.data.rgba.row_step;
          a = rb.data.rgba.a + row * rb.data.rgba.row_step;
          ma = mask.data.rgba.a + row * mask.data.rgba.row_step;
          for (col = 0; col < rb.width; col++) {
            if (*ma == 0) {
              *a = *r = *g = *b = 0;
            }
            a += rb.data.rgba.pixel_step;
            r += rb.data.rgba.pixel_step;
            g += rb.data.rgba.pixel_step;
            b += rb.data.rgba.pixel_step;
            ma += mask.data.rgba.pixel_step;
          }
        }
      }
    }
    if (!layer->compositer) {
      /*we have a mask layer with no composition configured, do a nomral blend
       */
      retcode = renderer->mergeRasterBuffer(image, &rb, 1.0, 0, 0, 0, 0,
                                            rb.width, rb.height);
    } else {
      retcode = msCompositeRasterBuffer(map, image, &rb, layer->compositer);
    }
    if (MS_UNLIKELY(retcode == MS_FAILURE)) {
      goto imagedraw_cleanup;
    }

  imagedraw_cleanup:
    msFreeImage(image_draw);
  }

  msImageEndLayer(map, layer, image);
  return (retcode);
}

int msDrawVectorLayer(mapObj *map, layerObj *layer, imageObj *image) {
  int status, retcode = MS_SUCCESS;
  int drawmode = MS_DRAWMODE_FEATURES;
  char annotate = MS_TRUE;
  shapeObj shape;
  shapeObj savedShape;
  rectObj searchrect;
  char cache = MS_FALSE;
  int maxnumstyles = 1;
  featureListNodeObjPtr shpcache = NULL, current = NULL;
  int nclasses = 0;
  int *classgroup = NULL;
  double minfeaturesize = -1;
  int maxfeatures = -1;
  int featuresdrawn = 0;

  if (image)
    maxfeatures = msLayerGetMaxFeaturesToDraw(layer, image->format);

  /* TODO TBT: draw as raster layer in vector renderers */

  annotate = msEvalContext(map, layer, layer->labelrequires);
  if (map->scaledenom > 0) {
    if ((layer->labelmaxscaledenom != -1) &&
        (map->scaledenom >= layer->labelmaxscaledenom))
      annotate = MS_FALSE;
    if ((layer->labelminscaledenom != -1) &&
        (map->scaledenom < layer->labelminscaledenom))
      annotate = MS_FALSE;
  }

  /* open this layer */
  status = msLayerOpen(layer);
  if (status != MS_SUCCESS)
    return MS_FAILURE;

  /* build item list. STYLEITEM javascript needs the shape attributes */
  if (layer->styleitem &&
      (strncasecmp(layer->styleitem, "javascript://", 13) == 0)) {
    status = msLayerWhichItems(layer, MS_TRUE, NULL);
  } else {
    status = msLayerWhichItems(layer, MS_FALSE, NULL);
  }

  if (status != MS_SUCCESS) {
    msLayerClose(layer);
    return MS_FAILURE;
  }

  /* identify target shapes */
  if (layer->transform == MS_TRUE) {
    searchrect = map->extent;

    if ((map->projection.numargs > 0) && (layer->projection.numargs > 0)) {
      int bDone = MS_FALSE;

      if (layer->connectiontype == MS_UVRASTER) {
        /* Nasty hack to make msUVRASTERLayerWhichShapes() aware that the */
        /* original area of interest is (map->extent, map->projection)... */
        /* Useful when dealin with UVRASTER that extend beyond 180 deg */
        msUVRASTERLayerUseMapExtentAndProjectionForNextWhichShapes(layer, map);

        searchrect = msUVRASTERGetSearchRect(layer, map);
        bDone = MS_TRUE;
      }

      if (!bDone)
        msProjectRect(
            &map->projection, &layer->projection,
            &searchrect); /* project the searchrect to source coords */
    }

  } else {
    searchrect.minx = searchrect.miny = 0;
    searchrect.maxx = map->width - 1;
    searchrect.maxy = map->height - 1;
  }

  status = msLayerWhichShapes(layer, searchrect, MS_FALSE);

  if (layer->connectiontype == MS_UVRASTER) {
    msUVRASTERLayerUseMapExtentAndProjectionForNextWhichShapes(layer, NULL);
  }

  if (status == MS_DONE) { /* no overlap */
    msLayerClose(layer);
    return MS_SUCCESS;
  } else if (status != MS_SUCCESS) {
    msLayerClose(layer);
    return MS_FAILURE;
  }

  nclasses = 0;
  classgroup = NULL;
  if (layer->classgroup && layer->numclasses > 0)
    classgroup = msAllocateValidClassGroups(layer, &nclasses);

  if (layer->minfeaturesize > 0)
    minfeaturesize = Pix2LayerGeoref(map, layer, layer->minfeaturesize);

  // Select how to render classes
  //    MS_FIRST_MATCHING_CLASS: Default and historic MapServer behavior
  //    MS_ALL_MATCHING_CLASSES: SLD behavior
  int ref_rendermode;
  const char *rendermodestr = msLayerGetProcessingKey(layer, "RENDERMODE");
  if (layer->rendermode == MS_ALL_MATCHING_CLASSES) {
    // SLD takes precedence
    ref_rendermode = MS_ALL_MATCHING_CLASSES;
  } else if (!rendermodestr) {
    // Default Mapfile
    ref_rendermode = MS_FIRST_MATCHING_CLASS;
  } else if (!strcmp(rendermodestr, "FIRST_MATCHING_CLASS")) {
    // Explicit default Mapfile
    ref_rendermode = MS_FIRST_MATCHING_CLASS;
  } else if (!strcmp(rendermodestr, "ALL_MATCHING_CLASSES")) {
    // SLD-like Mapfile
    ref_rendermode = MS_ALL_MATCHING_CLASSES;
  } else {
    msLayerClose(layer);
    msSetError(MS_MISCERR,
               "Unknown RENDERMODE: %s, should be one of: "
               "FIRST_MATCHING_CLASS, ALL_MATCHING_CLASSES.",
               "msDrawVectorLayer()", rendermodestr);
    return MS_FAILURE;
  }

  /* step through the target shapes and their classes */
  msInitShape(&shape);
  int classindex = -1;
  int classcount = 0;
  for (;;) {
    int rendermode;
    if (classindex == -1) {
      msFreeShape(&shape);
      status = msLayerNextShape(layer, &shape);
      if (status != MS_SUCCESS) {
        break;
      }

      /* Check if the shape size is ok to be drawn */
      if ((shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) &&
          (minfeaturesize > 0) &&
          (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE)) {
        if (layer->debug >= MS_DEBUGLEVEL_V)
          msDebug("msDrawVectorLayer(): Skipping shape (%ld) because "
                  "LAYER::MINFEATURESIZE is bigger than shape size\n",
                  shape.index);
        continue;
      }
      classcount = 0;
    }

    classindex = msShapeGetNextClass(classindex, layer, map, &shape, classgroup,
                                     nclasses);
    if ((classindex == -1) || (layer->class[classindex] -> status == MS_OFF)) {
      continue;
    }
    shape.classindex = classindex;

    // When only one class is applicable, rendering mode is forced to its
    // default, i.e. only the first applicable class is actually applied. As a
    // consequence, cache can be enabled when relevant.
    classcount++;
    rendermode = ref_rendermode;
    if ((classcount == 1) &&
        (msShapeGetNextClass(classindex, layer, map, &shape, classgroup,
                             nclasses) == -1)) {
      rendermode = MS_FIRST_MATCHING_CLASS;
    }

    if (rendermode == MS_FIRST_MATCHING_CLASS) {
      classindex = -1;
    }

    if (maxfeatures >= 0 && featuresdrawn >= maxfeatures) {
      status = MS_DONE;
      break;
    }
    featuresdrawn++;

    cache = MS_FALSE;
    if (layer->type == MS_LAYER_LINE &&
        (layer->class[shape.classindex]
         -> numstyles > 1 ||
                (layer->class[shape.classindex] -> numstyles == 1 && layer
                 -> class[shape.classindex] -> styles[0] -> outlinewidth >
                                                                0))) {
      int i;
      cache = MS_TRUE; /* only line layers with multiple styles need be cached
                          (I don't think POLYLINE layers need caching - SDL) */

      /* we can't handle caching with attribute binding other than for the first
       * style (#3976) */
      for (i = 1; i < layer->class[shape.classindex] -> numstyles; i++) {
        if (layer->class[shape.classindex] -> styles[i] -> numbindings > 0)
          cache = MS_FALSE;
      }
    }

    /* With 'STYLEITEM AUTO', we will have the datasource fill the class' */
    /* style parameters for this shape. */
    if (layer->styleitem) {
      if (strcasecmp(layer->styleitem, "AUTO") == 0) {
        if (msLayerGetAutoStyle(map, layer, layer->class[shape.classindex],
                                &shape) != MS_SUCCESS) {
          retcode = MS_FAILURE;
          break;
        }
      } else {
        /* Generic feature style handling as per RFC-61 */
        if (msLayerGetFeatureStyle(map, layer, layer->class[shape.classindex],
                                   &shape) != MS_SUCCESS) {
          retcode = MS_FAILURE;
          break;
        }
      }

      /* __TODO__ For now, we can't cache features with 'AUTO' style */
      cache = MS_FALSE;
    }

    if (rendermode == MS_ALL_MATCHING_CLASSES) {
      // Cache is designed to handle only one class. Therefore it is
      // disabled when using SLD "painters model" rendering mode.
      cache = MS_FALSE;
    }

    /* RFC77 TODO: check return value, may need a more sophisticated if-then
     * test. */
    if (annotate && layer->class[shape.classindex] -> numlabels > 0) {
      drawmode |= MS_DRAWMODE_LABELS;
      if (msLayerGetProcessingKey(layer, "LABEL_NO_CLIP")) {
        drawmode |= MS_DRAWMODE_UNCLIPPEDLABELS;
      }
    }

    if (layer->type == MS_LAYER_LINE &&
        msLayerGetProcessingKey(layer, "POLYLINE_NO_CLIP")) {
      drawmode |= MS_DRAWMODE_UNCLIPPEDLINES;
    }

    if (rendermode == MS_ALL_MATCHING_CLASSES) {
      // In SLD "painters model" rendering mode, all applicable classes are
      // actually applied. Coordinates stored in the shape must keep their
      // original values for the shape to be drawn multiple times. Here the
      // original shape is saved.
      msInitShape(&savedShape);
      msCopyShape(&shape, &savedShape);
    }

    if (cache) {
      styleObj *pStyle = layer->class[shape.classindex]->styles[0];
      if (pStyle->outlinewidth > 0) {
        /*
         * RFC 49 implementation
         * if an outlinewidth is used:
         *  - augment the style's width to account for the outline width
         *  - swap the style color and outlinecolor
         *  - draw the shape (the outline) in the first pass of the
         *    caching mechanism
         */
        msOutlineRenderingPrepareStyle(pStyle, map, layer, image);
      }
      status = msDrawShape(
          map, layer, &shape, image, 0,
          drawmode | MS_DRAWMODE_SINGLESTYLE); /* draw a single style */
      if (pStyle->outlinewidth > 0) {
        /*
         * RFC 49 implementation: switch back the styleobj to its
         * original state, so the line fill will be drawn in the
         * second pass of the caching mechanism
         */
        msOutlineRenderingRestoreStyle(pStyle, map, layer, image);
      }
    }

    else
      status =
          msDrawShape(map, layer, &shape, image, -1, drawmode); /* all styles */

    if (rendermode == MS_ALL_MATCHING_CLASSES) {
      // In SLD "painters model" rendering mode, all applicable classes are
      // actually applied. Coordinates stored in the shape must keep their
      // original values for the shape to be drawn multiple times. Here the
      // original shape is restored.
      msFreeShape(&shape);
      msCopyShape(&savedShape, &shape);
      msFreeShape(&savedShape);
    }

    if (status != MS_SUCCESS) {
      retcode = MS_FAILURE;
      break;
    }

    if (shape.numlines ==
        0) { /* once clipped the shape didn't need to be drawn */
      continue;
    }

    if (cache) {
      if (insertFeatureList(&shpcache, &shape) == NULL) {
        retcode = MS_FAILURE; /* problem adding to the cache */
        break;
      }
    }

    maxnumstyles =
        MS_MAX(maxnumstyles, layer->class[shape.classindex] -> numstyles);
  }
  msFreeShape(&shape);

  if (classgroup)
    msFree(classgroup);

  if (status != MS_DONE || retcode == MS_FAILURE) {
    msLayerClose(layer);
    if (shpcache) {
      freeFeatureList(shpcache);
      shpcache = NULL;
    }
    return MS_FAILURE;
  }

  if (shpcache && MS_DRAW_FEATURES(drawmode)) {
    int s;
    for (s = 0; s < maxnumstyles; s++) {
      for (current = shpcache; current; current = current->next) {
        if (layer->class[current->shape.classindex] -> numstyles > s) {
          styleObj *pStyle = layer->class[current->shape.classindex]->styles[s];
          if (pStyle->_geomtransform.type != MS_GEOMTRANSFORM_NONE)
            continue; /*skip this as it has already been rendered*/
          if (map->scaledenom > 0) {
            if ((pStyle->maxscaledenom != -1) &&
                (map->scaledenom >= pStyle->maxscaledenom))
              continue;
            if ((pStyle->minscaledenom != -1) &&
                (map->scaledenom < pStyle->minscaledenom))
              continue;
          }
          if (s == 0 && pStyle->outlinewidth > 0 &&
              MS_VALID_COLOR(pStyle->color)) {
            if (MS_UNLIKELY(MS_FAILURE ==
                            msDrawLineSymbol(map, image, &current->shape,
                                             pStyle, pStyle->scalefactor))) {
              return MS_FAILURE;
            }
          } else if (s > 0) {
            if (pStyle->outlinewidth > 0 &&
                MS_VALID_COLOR(pStyle->outlinecolor)) {
              /*
               * RFC 49 implementation
               * if an outlinewidth is used:
               *  - augment the style's width to account for the outline width
               *  - swap the style color and outlinecolor
               *  - draw the shape (the outline) in the first pass of the
               *    caching mechanism
               */
              msOutlineRenderingPrepareStyle(pStyle, map, layer, image);
              if (MS_UNLIKELY(MS_FAILURE ==
                              msDrawLineSymbol(map, image, &current->shape,
                                               pStyle, pStyle->scalefactor))) {
                return MS_FAILURE;
              }
              /*
               * RFC 49 implementation: switch back the styleobj to its
               * original state, so the line fill will be drawn in the
               * second pass of the caching mechanism
               */
              msOutlineRenderingRestoreStyle(pStyle, map, layer, image);
            }
            /* draw a valid line, i.e. one with a color defined or of type
             * pixmap*/
            if (MS_VALID_COLOR(pStyle->color) ||
                (pStyle->symbol < map->symbolset.numsymbols &&
                 (map->symbolset.symbol[pStyle->symbol]->type ==
                      MS_SYMBOL_PIXMAP ||
                  map->symbolset.symbol[pStyle->symbol]->type ==
                      MS_SYMBOL_SVG))) {
              if (MS_UNLIKELY(MS_FAILURE ==
                              msDrawLineSymbol(map, image, &current->shape,
                                               pStyle, pStyle->scalefactor)))
                return MS_FAILURE;
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
** Function to draw a layer IF it already has a result cache associated with it.
*Called by msDrawMap and via MapScript.
*/
int msDrawQueryLayer(mapObj *map, layerObj *layer, imageObj *image) {
  int i, status;
  char annotate = MS_TRUE, cache = MS_FALSE;
  int drawmode = MS_DRAWMODE_FEATURES;
  shapeObj shape;
  int maxnumstyles = 1;

  featureListNodeObjPtr shpcache = NULL, current = NULL;

  colorObj *colorbuffer = NULL;
  int *mindistancebuffer = NULL;

  if (!layer->resultcache)
    return (msDrawLayer(map, layer, image));

  if (!msLayerIsVisible(map, layer))
    return (MS_SUCCESS); /* not an error, just nothing to do */

  /* conditions may have changed since this layer last drawn, so reset
     layer->project to recheck projection needs (Bug #673) */
  layer->project =
      msProjectionsDiffer(&(layer->projection), &(map->projection));

  /* set annotation status */
  annotate = msEvalContext(map, layer, layer->labelrequires);
  if (map->scaledenom > 0) {
    if ((layer->labelmaxscaledenom != -1) &&
        (map->scaledenom >= layer->labelmaxscaledenom))
      annotate = MS_FALSE;
    if ((layer->labelminscaledenom != -1) &&
        (map->scaledenom < layer->labelminscaledenom))
      annotate = MS_FALSE;
  }

  /*
  ** Certain query map styles require us to render all features only (MS_NORMAL)
  *or first (MS_HILITE). With
  ** single-pass queries we have to make a copy of the layer and work from it
  *instead.
  */
  if (map->querymap.style == MS_NORMAL || map->querymap.style == MS_HILITE) {
    layerObj tmp_layer;

    if (initLayer(&tmp_layer, map) == -1)
      return (MS_FAILURE);

    if (msCopyLayer(&tmp_layer, layer) != MS_SUCCESS)
      return (MS_FAILURE);

    /* disable the connection pool for this layer */
    msLayerSetProcessingKey(&tmp_layer, "CLOSE_CONNECTION", "ALWAYS");

    status = msDrawLayer(map, &tmp_layer, image);

    freeLayer(&tmp_layer);

    if (map->querymap.style == MS_NORMAL || status != MS_SUCCESS)
      return (status);
  }

  /* if MS_HILITE, alter the one style (always at least 1 style), and set a
   * MINDISTANCE for the labelObj to avoid duplicates */
  if (map->querymap.style == MS_HILITE) {

    drawmode |= MS_DRAWMODE_QUERY;

    if (layer->numclasses > 0) {
      colorbuffer =
          (colorObj *)msSmallMalloc(layer->numclasses * sizeof(colorObj));
      mindistancebuffer = (int *)msSmallMalloc(layer->numclasses * sizeof(int));
    }

    for (i = 0; i < layer->numclasses; i++) {
      if (layer->type == MS_LAYER_POLYGON &&
              layer->class[i] -> numstyles >
                                     0) { /* alter BOTTOM style since that's
                                             almost always the fill */
        if (layer->class[i] -> styles == NULL) {
          msSetError(MS_MISCERR,
                     "Don't know how to draw class %s of layer %s without a "
                     "style definition.",
                     "msDrawQueryLayer()", layer->class[i] -> name,
                     layer -> name);
          msFree(colorbuffer);
          msFree(mindistancebuffer);
          return (MS_FAILURE);
        }
        if (MS_VALID_COLOR(layer->class[i] -> styles[0] -> color)) {
          colorbuffer[i] =
              layer->class[i]
                  ->styles[0]
                  ->color; /* save the color from the BOTTOM style */
          layer->class[i]->styles[0]->color = map->querymap.color;
        } else if (MS_VALID_COLOR(
                       layer->class[i] -> styles[0] -> outlinecolor)) {
          colorbuffer[i] =
              layer->class[i]
                  ->styles[0]
                  ->outlinecolor; /* if no color, save the outlinecolor from the
                                     BOTTOM style */
          layer->class[i]->styles[0]->outlinecolor = map->querymap.color;
        }
      } else if (layer->type == MS_LAYER_LINE &&
                     layer->class[i] -> numstyles > 0 &&
                                            layer -> class[i] -> styles[0]
                 -> outlinewidth >
                        0) { /* alter BOTTOM style for lines with outlines */
        if (MS_VALID_COLOR(layer->class[i] -> styles[0] -> color)) {
          colorbuffer[i] =
              layer->class[i]
                  ->styles[0]
                  ->color; /* save the color from the BOTTOM style */
          layer->class[i]->styles[0]->color = map->querymap.color;
        } /* else ??? */
      } else if (layer->class[i] -> numstyles > 0) {
        if (MS_VALID_COLOR(
                layer->class[i] -> styles[layer->class[i] -> numstyles - 1]
                -> color)) {
          colorbuffer[i] = layer->class[i]
                               ->styles[layer->class[i] -> numstyles - 1]
                               ->color; /* save the color from the TOP style */
          layer->class[i]->styles[layer->class[i] -> numstyles - 1]->color =
              map->querymap.color;
        } else if (MS_VALID_COLOR(layer->class[i]
                                  -> styles[layer->class[i] -> numstyles - 1]
                                  -> outlinecolor)) {
          colorbuffer[i] =
              layer->class[i]
                  ->styles[layer->class[i] -> numstyles - 1]
                  ->outlinecolor; /* if no color, save the outlinecolor from the
                                     TOP style */
          layer->class[i]
              ->styles[layer->class[i] -> numstyles - 1]
              ->outlinecolor = map->querymap.color;
        }
      } else if (layer->class[i] -> numlabels > 0) {
        colorbuffer[i] = layer->class[i]->labels[0]->color;
        layer->class[i]->labels[0]->color = map->querymap.color;
      } /* else ??? */

      mindistancebuffer[i] =
          -1; /* RFC77 TODO: only using the first label, is that cool? */
      if (layer->class[i] -> numlabels > 0) {
        mindistancebuffer[i] = layer->class[i]->labels[0]->mindistance;
        layer->class[i]->labels[0]->mindistance =
            MS_MAX(0, layer->class[i] -> labels[0] -> mindistance);
      }
    }
  }

  /*
  ** Layer was opened as part of the query process, msLayerWhichItems() has also
  *been run, shapes have been classified - start processing!
  */

  msInitShape(&shape);

  for (i = 0; i < layer->resultcache->numresults; i++) {
    status = msLayerGetShape(layer, &shape, &(layer->resultcache->results[i]));
    if (status != MS_SUCCESS) {
      msFree(colorbuffer);
      msFree(mindistancebuffer);
      return (MS_FAILURE);
    }

    shape.classindex = layer->resultcache->results[i].classindex;
    /* classindex may be -1 here if there was a template at the top level
     * in this layer and the current shape selected even if it didn't
     * match any class
     *
     * FrankW: classindex is also sometimes 0 even if there are no classes, so
     * we are careful not to use out of range class values as an index.
     */
    if (shape.classindex == -1 || shape.classindex >= layer->numclasses ||
            layer->class[shape.classindex] -> status == MS_OFF) {
      msFreeShape(&shape);
      continue;
    }

    cache = MS_FALSE;
    if (layer->type == MS_LAYER_LINE &&
        (layer->class[shape.classindex]
         -> numstyles > 1 ||
                (layer->class[shape.classindex] -> numstyles == 1 && layer
                 -> class[shape.classindex] -> styles[0] -> outlinewidth >
                                                                0))) {
      int i;
      cache = MS_TRUE; /* only line layers with multiple styles need be cached
                          (I don't think POLYLINE layers need caching - SDL) */

      /* we can't handle caching with attribute binding other than for the first
       * style (#3976) */
      for (i = 1; i < layer->class[shape.classindex] -> numstyles; i++) {
        if (layer->class[shape.classindex] -> styles[i] -> numbindings > 0)
          cache = MS_FALSE;
      }
    }

    if (annotate && layer->class[shape.classindex] -> numlabels > 0) {
      drawmode |= MS_DRAWMODE_LABELS;
    }

    if (cache) {
      styleObj *pStyle = layer->class[shape.classindex]->styles[0];
      if (pStyle->outlinewidth > 0)
        msOutlineRenderingPrepareStyle(pStyle, map, layer, image);
      status = msDrawShape(
          map, layer, &shape, image, 0,
          drawmode | MS_DRAWMODE_SINGLESTYLE); /* draw only the first style */
      if (pStyle->outlinewidth > 0)
        msOutlineRenderingRestoreStyle(pStyle, map, layer, image);
    } else {
      status =
          msDrawShape(map, layer, &shape, image, -1, drawmode); /* all styles */
    }
    if (status != MS_SUCCESS) {
      msLayerClose(layer);
      msFree(colorbuffer);
      msFree(mindistancebuffer);
      return (MS_FAILURE);
    }

    if (shape.numlines ==
        0) { /* once clipped the shape didn't need to be drawn */
      msFreeShape(&shape);
      continue;
    }

    if (cache) {
      if (insertFeatureList(&shpcache, &shape) == NULL) {
        msFree(colorbuffer);
        msFree(mindistancebuffer);
        return (MS_FAILURE); /* problem adding to the cache */
      }
    }

    maxnumstyles =
        MS_MAX(maxnumstyles, layer->class[shape.classindex] -> numstyles);
    msFreeShape(&shape);
  }

  if (shpcache) {
    int s;
    for (s = 0; s < maxnumstyles; s++) {
      for (current = shpcache; current; current = current->next) {
        if (layer->class[current->shape.classindex] -> numstyles > s) {
          styleObj *pStyle = layer->class[current->shape.classindex]->styles[s];
          if (pStyle->_geomtransform.type != MS_GEOMTRANSFORM_NONE)
            continue; /* skip this as it has already been rendered */
          if (map->scaledenom > 0) {
            if ((pStyle->maxscaledenom != -1) &&
                (map->scaledenom >= pStyle->maxscaledenom))
              continue;
            if ((pStyle->minscaledenom != -1) &&
                (map->scaledenom < pStyle->minscaledenom))
              continue;
          }
          if (s == 0 && pStyle->outlinewidth > 0 &&
              MS_VALID_COLOR(pStyle->color)) {
            if (MS_UNLIKELY(MS_FAILURE ==
                            msDrawLineSymbol(map, image, &current->shape,
                                             pStyle, pStyle->scalefactor))) {
              return MS_FAILURE;
            }
          } else if (s > 0) {
            if (pStyle->outlinewidth > 0 &&
                MS_VALID_COLOR(pStyle->outlinecolor)) {
              msOutlineRenderingPrepareStyle(pStyle, map, layer, image);
              if (MS_UNLIKELY(MS_FAILURE ==
                              msDrawLineSymbol(map, image, &current->shape,
                                               pStyle, pStyle->scalefactor))) {
                return MS_FAILURE;
              }
              msOutlineRenderingRestoreStyle(pStyle, map, layer, image);
            }
            /* draw a valid line, i.e. one with a color defined or of type
             * pixmap */
            if (MS_VALID_COLOR(pStyle->color) ||
                (pStyle->symbol < map->symbolset.numsymbols &&
                 (map->symbolset.symbol[pStyle->symbol]->type ==
                      MS_SYMBOL_PIXMAP ||
                  map->symbolset.symbol[pStyle->symbol]->type ==
                      MS_SYMBOL_SVG))) {
              if (MS_UNLIKELY(MS_FAILURE ==
                              msDrawLineSymbol(map, image, &current->shape,
                                               pStyle, pStyle->scalefactor)))
                return MS_FAILURE;
            }
          }
        }
      }
    }

    freeFeatureList(shpcache);
    shpcache = NULL;
  }

  /* if MS_HILITE, restore color and mindistance values */
  if (map->querymap.style == MS_HILITE) {
    for (i = 0; i < layer->numclasses; i++) {
      if (layer->type == MS_LAYER_POLYGON && layer->class[i] -> numstyles > 0) {
        if (MS_VALID_COLOR(layer->class[i] -> styles[0] -> color))
          layer->class[i]->styles[0]->color = colorbuffer[i];
        else if (MS_VALID_COLOR(layer->class[i] -> styles[0] -> outlinecolor))
          layer->class[i]->styles[0]->outlinecolor =
              colorbuffer[i]; /* if no color, restore outlinecolor for the
                                 BOTTOM style */
      } else if (layer->type == MS_LAYER_LINE &&
                     layer->class[i] -> numstyles > 0 && layer -> class[i]
                 -> styles[0] -> outlinewidth > 0) {
        if (MS_VALID_COLOR(layer->class[i] -> styles[0] -> color))
          layer->class[i]->styles[0]->color = colorbuffer[i];
      } else if (layer->class[i] -> numstyles > 0) {
        if (MS_VALID_COLOR(
                layer->class[i] -> styles[layer->class[i] -> numstyles - 1]
                -> color))
          layer->class[i]->styles[layer->class[i] -> numstyles - 1]->color =
              colorbuffer[i];
        else if (MS_VALID_COLOR(
                     layer->class[i] -> styles[layer->class[i] -> numstyles - 1]
                     -> outlinecolor))
          layer->class[i]
              ->styles[layer->class[i] -> numstyles - 1]
              ->outlinecolor =
              colorbuffer[i]; /* if no color, restore outlinecolor for the TOP
                                 style */
      } else if (layer->class[i] -> numlabels > 0) {
        if (MS_VALID_COLOR(layer->class[i] -> labels[0] -> color))
          layer->class[i]->labels[0]->color = colorbuffer[i];
      }

      if (layer->class[i] -> numlabels > 0)
        layer->class[i]->labels[0]->mindistance =
            mindistancebuffer[i]; /* RFC77 TODO: again, only using the first
                                     label, is that cool? */
    }

    msFree(colorbuffer);
    msFree(mindistancebuffer);
  }

  return (MS_SUCCESS);
}

/**
 * msDrawRasterLayerPlugin()
 */

static int msDrawRasterLayerPlugin(mapObj *map, layerObj *layer,
                                   imageObj *image)

{
  rendererVTableObj *renderer = MS_IMAGE_RENDERER(image);
  rasterBufferObj *rb = msSmallCalloc(1, sizeof(rasterBufferObj));
  int ret;
  if (renderer->supports_pixel_buffer) {
    if (MS_SUCCESS != renderer->getRasterBufferHandle(image, rb)) {
      msSetError(MS_MISCERR, "renderer failed to extract raster buffer",
                 "msDrawRasterLayer()");
      return MS_FAILURE;
    }
    ret = msDrawRasterLayerLow(map, layer, image, rb);
  } else {
    if (MS_SUCCESS != renderer->initializeRasterBuffer(
                          rb, image->width, image->height, MS_IMAGEMODE_RGBA)) {
      msSetError(MS_MISCERR, "renderer failed to create raster buffer",
                 "msDrawRasterLayer()");
      return MS_FAILURE;
    }

    ret = msDrawRasterLayerLow(map, layer, image, rb);

    if (ret == 0) {
      ret = renderer->mergeRasterBuffer(image, rb, 1.0, 0, 0, 0, 0, rb->width,
                                        rb->height);
    }

    msFreeRasterBuffer(rb);
  }
#define RB_GET_R(rb, x, y)                                                     \
  *((rb)->data.rgba.r + (x) * (rb)->data.rgba.pixel_step +                     \
    (y) * (rb)->data.rgba.row_step)
#define RB_GET_G(rb, x, y)                                                     \
  *((rb)->data.rgba.g + (x) * (rb)->data.rgba.pixel_step +                     \
    (y) * (rb)->data.rgba.row_step)
#define RB_GET_B(rb, x, y)                                                     \
  *((rb)->data.rgba.b + (x) * (rb)->data.rgba.pixel_step +                     \
    (y) * (rb)->data.rgba.row_step)
#define RB_GET_A(rb, x, y)                                                     \
  *((rb)->data.rgba.a + (x) * (rb)->data.rgba.pixel_step +                     \
    (y) * (rb)->data.rgba.row_step)

  free(rb);

  return ret;
}

/**
 * Generic function to render raster layers.
 */
int msDrawRasterLayer(mapObj *map, layerObj *layer, imageObj *image) {

  int rv = MS_FAILURE;
  if (!image || !map || !layer) {
    return rv;
  }

  /* RFC-86 Scale dependant token replacements*/
  rv = msLayerApplyScaletokens(layer,
                               (layer->map) ? layer->map->scaledenom : -1);
  if (rv != MS_SUCCESS)
    return rv;
  if (MS_RENDERER_PLUGIN(image->format))
    rv = msDrawRasterLayerPlugin(map, layer, image);
  else if (MS_RENDERER_RAWDATA(image->format))
    rv = msDrawRasterLayerLow(map, layer, image, NULL);
  msLayerRestoreFromScaletokens(layer);
  return rv;
}

/**
 * msDrawWMSLayer()
 *
 * Draw a single WMS layer.
 * Multiple WMS layers in a map are preloaded and then drawn using
 * msDrawWMSLayerLow()
 */

#ifdef USE_WMS_LYR
int msDrawWMSLayer(mapObj *map, layerObj *layer, imageObj *image) {
  int nStatus = MS_FAILURE;

  if (image && map && layer) {
    /* ------------------------------------------------------------------
     * Start by downloading the layer
     * ------------------------------------------------------------------ */
    httpRequestObj asReqInfo[2];
    int numReq = 0;

    msHTTPInitRequestObj(asReqInfo, 2);

    if (msPrepareWMSLayerRequest(1, map, layer, 1, 0, NULL, 0, 0, 0, NULL,
                                 asReqInfo, &numReq) == MS_FAILURE ||
        msOWSExecuteRequests(asReqInfo, numReq, map, MS_TRUE) == MS_FAILURE) {
      return MS_FAILURE;
    }

    /* ------------------------------------------------------------------
     * Then draw layer based on output format
     * ------------------------------------------------------------------ */
    if (MS_RENDERER_PLUGIN(image->format))
      nStatus = msDrawWMSLayerLow(1, asReqInfo, numReq, map, layer, image);
    else if (MS_RENDERER_RAWDATA(image->format))
      nStatus = msDrawWMSLayerLow(1, asReqInfo, numReq, map, layer, image);

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

int circleLayerDrawShape(mapObj *map, imageObj *image, layerObj *layer,
                         shapeObj *shape) {
  pointObj center;
  double r;
  int s;
  int c = shape->classindex;

  if (shape->numlines != 1)
    return (MS_SUCCESS); /* invalid shape */
  if (shape->line[0].numpoints != 2)
    return (MS_SUCCESS); /* invalid shape */

  center.x = (shape->line[0].point[0].x + shape->line[0].point[1].x) / 2.0;
  center.y = (shape->line[0].point[0].y + shape->line[0].point[1].y) / 2.0;
  r = MS_ABS(center.x - shape->line[0].point[0].x);
  if (r == 0)
    r = MS_ABS(center.y - shape->line[0].point[0].y);
  if (r == 0)
    return (MS_SUCCESS);

  if (layer->transform == MS_TRUE) {

    if (layer->project)
      msProjectPoint(&layer->projection, &map->projection, &center);

    center.x = MS_MAP2IMAGE_X(center.x, map->extent.minx, map->cellsize);
    center.y = MS_MAP2IMAGE_Y(center.y, map->extent.maxy, map->cellsize);
    r /= map->cellsize;
  } else
    msOffsetPointRelativeTo(&center, layer);

  for (s = 0; s < layer->class[c] -> numstyles; s++) {
    if (msScaleInBounds(map->scaledenom,
                        layer->class[c] -> styles[s] -> minscaledenom,
                        layer -> class[c] -> styles[s] -> maxscaledenom))
      if (MS_UNLIKELY(MS_FAILURE ==
                      msCircleDrawShadeSymbol(
                          map, image, &center, r, layer->class[c] -> styles[s],
                          layer -> class[c] -> styles[s] -> scalefactor))) {
        return MS_FAILURE;
      }
  }
  return MS_SUCCESS;
  /* TODO: need to handle circle annotation */
}

static int pointLayerDrawShape(mapObj *map, imageObj *image, layerObj *layer,
                               shapeObj *shape, int drawmode) {
  int l, c = shape->classindex, j, i, s;
  pointObj *point;
  int ret = MS_FAILURE;

  if (layer->project && layer->transform == MS_TRUE) {
    reprojectionObj *reprojector = msLayerGetReprojectorToMap(layer, map);
    if (reprojector == NULL) {
      return MS_FAILURE;
    }
    msProjectShapeEx(reprojector, shape);
  }

  // Only take into account map rotation if the label and style angles are
  // non-zero.
  if (map->gt.rotation_angle) {
    for (l = 0; l < layer->class[c] -> numlabels; l++) {
      if (layer->class[c] -> labels[l] -> angle != 0)
        layer->class[c]->labels[l]->angle -= map->gt.rotation_angle;
    }

    for (s = 0; s < layer->class[c] -> numstyles; s++) {
      if (layer->class[c] -> styles[s] -> angle != 0)
        layer->class[c]->styles[s]->angle -= map->gt.rotation_angle;
    }
  }

  for (j = 0; j < shape->numlines; j++) {
    for (i = 0; i < shape->line[j].numpoints; i++) {
      point = &(shape->line[j].point[i]);
      if (layer->transform == MS_TRUE) {
        if (!msPointInRect(point, &map->extent))
          continue; /* next point */
        msTransformPoint(point, &map->extent, map->cellsize, image);
      } else
        msOffsetPointRelativeTo(point, layer);

      if (MS_DRAW_FEATURES(drawmode)) {
        for (s = 0; s < layer->class[c] -> numstyles; s++) {
          if (msScaleInBounds(map->scaledenom,
                              layer->class[c] -> styles[s] -> minscaledenom,
                              layer -> class[c] -> styles[s] -> maxscaledenom))
            if (MS_UNLIKELY(
                    MS_FAILURE ==
                    msDrawMarkerSymbol(
                        map, image, point, layer->class[c] -> styles[s],
                        layer -> class[c] -> styles[s] -> scalefactor))) {
              goto end;
            }
        }
      }
      if (MS_DRAW_LABELS(drawmode)) {
        if (layer->labelcache) {
          if (msAddLabelGroup(map, image, layer, c, shape, point, -1) !=
              MS_SUCCESS)
            goto end;
        } else {
          for (l = 0; l < layer->class[c] -> numlabels; l++)
            if (msGetLabelStatus(map, layer, shape,
                                 layer->class[c] -> labels[l]) == MS_ON) {
              char *annotext = msShapeGetLabelAnnotation(
                  layer, shape, layer->class[c] -> labels[l]);
              if (MS_UNLIKELY(MS_FAILURE ==
                              msDrawLabel(map, image, *point, annotext,
                                          layer->class[c] -> labels[l],
                                          layer -> class[c] -> labels[l]
                                          -> scalefactor))) {
                goto end;
              }
            }
        }
      }
    }
  }
  ret = MS_SUCCESS;

end:
  if (map->gt.rotation_angle) {
    for (l = 0; l < layer->class[c] -> numlabels; l++) {
      if (layer->class[c] -> labels[l] -> angle != 0)
        layer->class[c]->labels[l]->angle += map->gt.rotation_angle;
    }

    for (s = 0; s < layer->class[c] -> numstyles; s++) {
      if (layer->class[c] -> styles[s] -> angle != 0)
        layer->class[c]->styles[s]->angle += map->gt.rotation_angle;
    }
  }

  return ret;
}

int lineLayerDrawShape(mapObj *map, imageObj *image, layerObj *layer,
                       shapeObj *shape, shapeObj *anno_shape,
                       shapeObj *unclipped_shape, int style, int drawmode) {
  int c = shape->classindex;
  int ret = MS_SUCCESS;

  /* RFC48: loop through the styles, and pass off to the type-specific
  function if the style has an appropriate type */
  if (MS_DRAW_FEATURES(drawmode)) {
    for (int s = 0; s < layer->class[c] -> numstyles; s++) {
      if (msScaleInBounds(map->scaledenom,
                          layer->class[c] -> styles[s] -> minscaledenom,
                          layer -> class[c] -> styles[s] -> maxscaledenom)) {
        if (layer->class[c] -> styles[s] -> _geomtransform.type !=
                                                MS_GEOMTRANSFORM_NONE) {
          if (MS_UNLIKELY(MS_FAILURE ==
                          msDrawTransformedShape(
                              map, image, unclipped_shape,
                              layer->class[c] -> styles[s],
                              layer -> class[c] -> styles[s] -> scalefactor))) {
            return MS_FAILURE;
          }
        } else if (!MS_DRAW_SINGLESTYLE(drawmode) || s == style) {
          if (MS_UNLIKELY(MS_FAILURE ==
                          msDrawLineSymbol(
                              map, image, shape, layer->class[c] -> styles[s],
                              layer -> class[c] -> styles[s] -> scalefactor))) {
            return MS_FAILURE;
          }
        }
      }
    }
  }

  if (MS_DRAW_LABELS(drawmode)) {
    for (int l = 0; l < layer->class[c] -> numlabels; l++) {
      labelObj *label = layer->class[c]->labels[l];
      textSymbolObj ts;
      char *annotext;
      if (!msGetLabelStatus(map, layer, shape, label)) {
        continue;
      }

      annotext = msShapeGetLabelAnnotation(layer, anno_shape, label);
      if (!annotext)
        continue;
      initTextSymbol(&ts);
      msPopulateTextSymbolForLabelAndString(
          &ts, label, annotext, label->scalefactor, image->resolutionfactor,
          layer->labelcache);

      if (label->anglemode == MS_FOLLOW) { /* bug #1620 implementation */
        struct label_follow_result lfr;

        if (!layer->labelcache) {
          msSetError(MS_MISCERR,
                     "Angle mode 'FOLLOW' is supported only with labelcache on",
                     "msDrawShape()");
          ret = MS_FAILURE;
          goto line_cleanup;
        }

        memset(&lfr, 0, sizeof(lfr));
        msPolylineLabelPath(map, image, anno_shape, &ts, label, &lfr);

        for (int i = 0; i < lfr.num_follow_labels; i++) {
          if (msAddLabel(map, image, label, layer->index, c, anno_shape, NULL,
                         -1, lfr.follow_labels[i]) != MS_SUCCESS) {
            ret = MS_FAILURE;
            goto line_cleanup;
          }
        }
        free(lfr.follow_labels);
        for (int i = 0; i < lfr.lar.num_label_points; i++) {
          textSymbolObj *ts_auto = msSmallMalloc(sizeof(textSymbolObj));
          initTextSymbol(ts_auto);
          msCopyTextSymbol(ts_auto, &ts);
          ts_auto->rotation = lfr.lar.angles[i];
          {
            if (msAddLabel(map, image, label, layer->index, c, anno_shape,
                           &lfr.lar.label_points[i], -1,
                           ts_auto) != MS_SUCCESS) {
              ret = MS_FAILURE;
              free(lfr.lar.angles);
              free(lfr.lar.label_points);
              goto line_cleanup;
            }
          }
        }
        free(lfr.lar.angles);
        free(lfr.lar.label_points);
      } else {
        struct label_auto_result lar;
        memset(&lar, 0, sizeof(struct label_auto_result));
        ret = msPolylineLabelPoint(map, anno_shape, &ts, label, &lar,
                                   image->resolutionfactor);
        if (MS_UNLIKELY(MS_FAILURE == ret))
          goto line_cleanup;

        if (label->angle != 0)
          label->angle -= map->gt.rotation_angle; /* apply rotation angle */

        for (int i = 0; i < lar.num_label_points; i++) {
          textSymbolObj *ts_auto = msSmallMalloc(sizeof(textSymbolObj));
          initTextSymbol(ts_auto);
          msCopyTextSymbol(ts_auto, &ts);
          ts_auto->rotation = lar.angles[i];
          if (layer->labelcache) {
            if (msAddLabel(map, image, label, layer->index, c, anno_shape,
                           &lar.label_points[i], -1, ts_auto) != MS_SUCCESS) {
              ret = MS_FAILURE;
              free(lar.angles);
              free(lar.label_points);
              freeTextSymbol(ts_auto);
              free(ts_auto);
              goto line_cleanup;
            }
          } else {
            if (!ts_auto->textpath) {
              if (MS_UNLIKELY(MS_FAILURE == msComputeTextPath(map, ts_auto))) {
                ret = MS_FAILURE;
                free(lar.angles);
                free(lar.label_points);
                freeTextSymbol(ts_auto);
                free(ts_auto);
                goto line_cleanup;
              }
            }
            ret = msDrawTextSymbol(map, image, lar.label_points[i], ts_auto);
            freeTextSymbol(ts_auto);
            free(ts_auto); /* TODO RFC98: could we not re-use the original ts
                            * instead of duplicating into ts_auto ? we cannot
                            * for now, as the rendering code will modify the
                            * glyph positions to apply the labelpoint and
                            * rotation offsets */
            ts_auto = NULL;
            if (MS_UNLIKELY(MS_FAILURE == ret))
              goto line_cleanup;
          }
        }
        free(lar.angles);
        free(lar.label_points);
      }

    line_cleanup:
      /* clean up and reset */
      if (ret == MS_FAILURE) {
        break; /* from the label looping */
      }
      freeTextSymbol(&ts);
    } /* next label */
  }

  return ret;
}

int polygonLayerDrawShape(mapObj *map, imageObj *image, layerObj *layer,
                          shapeObj *shape, shapeObj *anno_shape,
                          shapeObj *unclipped_shape, int drawmode) {

  int c = shape->classindex;
  pointObj annopnt = {0}; // initialize
  int i;

  if (MS_DRAW_FEATURES(drawmode)) {
    for (i = 0; i < layer->class[c] -> numstyles; i++) {
      if (msScaleInBounds(map->scaledenom,
                          layer->class[c] -> styles[i] -> minscaledenom,
                          layer -> class[c] -> styles[i] -> maxscaledenom)) {
        if (layer->class[c] -> styles[i] -> _geomtransform.type ==
                                                MS_GEOMTRANSFORM_NONE) {
          if (MS_UNLIKELY(MS_FAILURE ==
                          msDrawShadeSymbol(
                              map, image, shape, layer->class[c] -> styles[i],
                              layer -> class[c] -> styles[i] -> scalefactor))) {
            return MS_FAILURE;
          }
        } else {
          if (MS_UNLIKELY(MS_FAILURE ==
                          msDrawTransformedShape(
                              map, image, unclipped_shape,
                              layer->class[c] -> styles[i],
                              layer -> class[c] -> styles[i] -> scalefactor))) {
            return MS_FAILURE;
          }
        }
      }
    }
  }

  if (MS_DRAW_LABELS(drawmode)) {
    if (layer->class[c] -> numlabels > 0) {
      double minfeaturesize =
          layer->class[c]->labels[0]->minfeaturesize * image->resolutionfactor;
      if (msPolygonLabelPoint(anno_shape, &annopnt, minfeaturesize) ==
          MS_SUCCESS) {
        for (i = 0; i < layer->class[c] -> numlabels; i++)
          if (layer->class[c] -> labels[i] -> angle != 0)
            layer->class[c]->labels[i]->angle -=
                map->gt.rotation_angle; /* TODO: is this correct ??? */
        if (layer->labelcache) {
          if (msAddLabelGroup(map, image, layer, c, anno_shape, &annopnt,
                              MS_MIN(shape->bounds.maxx - shape->bounds.minx,
                                     shape->bounds.maxy -
                                         shape->bounds.miny)) != MS_SUCCESS) {
            return MS_FAILURE;
          }
        } else {
          for (i = 0; i < layer->class[c] -> numlabels; i++)
            if (msGetLabelStatus(map, layer, shape,
                                 layer->class[c] -> labels[i]) == MS_ON) {
              char *annotext = msShapeGetLabelAnnotation(
                  layer, shape,
                  layer->class[c]
                  -> labels[i]); /*ownership taken by msDrawLabel, no need to
                                    free */
              if (MS_UNLIKELY(MS_FAILURE ==
                              msDrawLabel(map, image, annopnt, annotext,
                                          layer->class[c] -> labels[i],
                                          layer -> class[c] -> labels[i]
                                          -> scalefactor))) {
                return MS_FAILURE;
              }
            }
        }
      }
    }
  }
  return MS_SUCCESS;
}

/*
** Function to render an individual shape, the style variable enables/disables
*the drawing of a single style
** versus a single style. This is necessary when drawing entire layers as proper
*overlay can only be achived
** through caching. "querymapMode" parameter is used to tell msBindLayerToShape
*to not override the
** QUERYMAP HILITE color.
*/
int msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, imageObj *image,
                int style, int drawmode) {
  int c, s, ret = MS_SUCCESS;
  shapeObj *anno_shape, *unclipped_shape = shape;
  int bNeedUnclippedShape = MS_FALSE;
  int bNeedUnclippedAnnoShape = MS_FALSE;
  int bShapeNeedsClipping = MS_TRUE;

  if (shape->numlines == 0 || shape->type == MS_SHAPE_NULL)
    return MS_SUCCESS;

  c = shape->classindex;

  /* Before we do anything else, we will check for a rangeitem.
     If its there, we need to change the style's color to map
     the range to the shape */
  for (s = 0; s < layer->class[c] -> numstyles; s++) {
    styleObj *style = layer->class[c]->styles[s];
    if (style->rangeitem != NULL)
      msShapeToRange((layer->class[c] -> styles[s]), shape);
  }

  /* circle and point layers go through their own treatment */
  if (layer->type == MS_LAYER_CIRCLE) {
    if (msBindLayerToShape(layer, shape, drawmode) != MS_SUCCESS)
      return MS_FAILURE;
    msDrawStartShape(map, layer, image, shape);
    ret = circleLayerDrawShape(map, image, layer, shape);
    msDrawEndShape(map, layer, image, shape);
    return ret;
  } else if (layer->type == MS_LAYER_POINT || layer->type == MS_LAYER_RASTER) {
    if (msBindLayerToShape(layer, shape, drawmode) != MS_SUCCESS)
      return MS_FAILURE;
    msDrawStartShape(map, layer, image, shape);
    ret = pointLayerDrawShape(map, image, layer, shape, drawmode);
    msDrawEndShape(map, layer, image, shape);
    return ret;
  }

  if (layer->type == MS_LAYER_POLYGON && shape->type != MS_SHAPE_POLYGON) {
    msSetError(
        MS_MISCERR,
        "Only polygon shapes can be drawn using a polygon layer definition.",
        "polygonLayerDrawShape()");
    return (MS_FAILURE);
  }
  if (layer->type == MS_LAYER_LINE && shape->type != MS_SHAPE_POLYGON &&
      shape->type != MS_SHAPE_LINE) {
    msSetError(MS_MISCERR,
               "Only polygon or line shapes can be drawn using a line layer "
               "definition.",
               "msDrawShape()");
    return (MS_FAILURE);
  }

  if (layer->project && layer->transform == MS_TRUE) {
    reprojectionObj *reprojector = msLayerGetReprojectorToMap(layer, map);
    if (reprojector == NULL) {
      return MS_FAILURE;
    }
    msProjectShapeEx(reprojector, shape);
  }

  /* check if we'll need the unclipped shape */
  if (shape->type != MS_SHAPE_POINT) {
    if (MS_DRAW_FEATURES(drawmode)) {
      for (s = 0; s < layer->class[c] -> numstyles; s++) {
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

    if (MS_DRAW_LABELS(drawmode) && MS_DRAW_UNCLIPPED_LABELS(drawmode)) {
      bNeedUnclippedAnnoShape = MS_TRUE;
      bNeedUnclippedShape = MS_TRUE;
    }

    if (MS_DRAW_UNCLIPPED_LINES(drawmode)) {
      bShapeNeedsClipping = MS_FALSE;
    }
  } else {
    bShapeNeedsClipping = MS_FALSE;
  }

  if (layer->transform == MS_TRUE && bShapeNeedsClipping) {
    /* compute the size of the clipping buffer, in pixels. This buffer must
     account for the size of symbols drawn to avoid artifacts around the image
     edges */
    int clip_buf = 0;
    int s;
    rectObj cliprect;
    for (s = 0; s < layer->class[c] -> numstyles; s++) {
      double maxsize, maxunscaledsize;
      symbolObj *symbol;
      styleObj *style = layer->class[c]->styles[s];
      if (!MS_IS_VALID_ARRAY_INDEX(style->symbol, map->symbolset.numsymbols)) {
        msSetError(MS_SYMERR, "Invalid symbol index: %d", "msDrawShape()",
                   style->symbol);
        return MS_FAILURE;
      }
      symbol = map->symbolset.symbol[style->symbol];
      if (symbol->type == MS_SYMBOL_PIXMAP) {
        if (MS_SUCCESS != msPreloadImageSymbol(MS_MAP_RENDERER(map), symbol))
          return MS_FAILURE;
      } else if (symbol->type == MS_SYMBOL_SVG) {
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
        if (MS_SUCCESS != msPreloadSVGSymbol(symbol))
          return MS_FAILURE;
#else
        msSetError(MS_SYMERR, "SVG symbol support is not enabled.",
                   "msDrawShape()");
        return MS_FAILURE;
#endif
      }
      maxsize = MS_MAX(msSymbolGetDefaultSize(symbol),
                       MS_MAX(style->size, style->width));
      maxunscaledsize = MS_MAX(style->minsize * image->resolutionfactor,
                               style->minwidth * image->resolutionfactor);
      if (shape->type == MS_SHAPE_POLYGON &&
          !IS_PARALLEL_OFFSET(style->offsety)) {
        maxsize += MS_MAX(fabs(style->offsety), fabs(style->offsetx));
      }
      clip_buf = MS_MAX(
          clip_buf,
          MS_NINT(MS_MAX(maxsize * style->scalefactor, maxunscaledsize) + 1));
    }

    /* if we need a copy of the unclipped shape, transform first, then clip to
     * avoid transforming twice */
    if (bNeedUnclippedShape) {
      msTransformShape(shape, map->extent, map->cellsize, image);
      if (shape->numlines == 0)
        return MS_SUCCESS;
      msComputeBounds(shape);

      /* TODO: there's an optimization here that can be implemented:
         - no need to allocate unclipped_shape for each call to this function
         - the calls to msClipXXXRect will discard the original lineObjs,
         whereas we have just copied them because they where needed. These two
         functions could be changed so they are instructed not to free the
         original lineObjs. */
      unclipped_shape = (shapeObj *)msSmallMalloc(sizeof(shapeObj));
      msInitShape(unclipped_shape);
      msCopyShape(shape, unclipped_shape);
      if (shape->type == MS_SHAPE_POLYGON) {
        /* #179: additional buffer for polygons */
        clip_buf += 2;
      }

      cliprect.minx = cliprect.miny = -clip_buf;
      cliprect.maxx = image->width + clip_buf;
      cliprect.maxy = image->height + clip_buf;
      if (shape->type == MS_SHAPE_POLYGON) {
        msClipPolygonRect(shape, cliprect);
      } else {
        assert(shape->type == MS_SHAPE_LINE);
        msClipPolylineRect(shape, cliprect);
      }
      if (bNeedUnclippedAnnoShape) {
        anno_shape = unclipped_shape;
      } else {
        anno_shape = shape;
      }
    } else {
      /* clip first, then transform. This means we are clipping in geographical
       * space */
      double clip_buf_d;
      if (shape->type == MS_SHAPE_POLYGON) {
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
      if (shape->type == MS_SHAPE_POLYGON) {
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
     * or is a point type layer where out of bounds points are treated
     * differently*/
    if (layer->transform == MS_TRUE) {
      msTransformShape(shape, map->extent, map->cellsize, image);
      msComputeBounds(shape);
    } else {
      msOffsetShapeRelativeTo(shape, layer);
    }
    anno_shape = shape;
  }
  if (shape->numlines == 0) {
    ret = MS_SUCCESS; /* error message is set in msBindLayerToShape() */
    goto draw_shape_cleanup;
  }

  if (msBindLayerToShape(layer, shape, drawmode) != MS_SUCCESS) {
    ret = MS_FAILURE; /* error message is set in msBindLayerToShape() */
    goto draw_shape_cleanup;
  }

  switch (layer->type) {
  case MS_LAYER_LINE:
    msDrawStartShape(map, layer, image, shape);
    ret = lineLayerDrawShape(map, image, layer, shape, anno_shape,
                             unclipped_shape, style, drawmode);
    break;
  case MS_LAYER_POLYGON:
    msDrawStartShape(map, layer, image, shape);
    ret = polygonLayerDrawShape(map, image, layer, shape, anno_shape,
                                unclipped_shape, drawmode);
    break;
  case MS_LAYER_POINT:
  case MS_LAYER_RASTER:
    assert(0); // bug !
  default:
    msSetError(MS_MISCERR, "Unknown layer type.", "msDrawShape()");
    ret = MS_FAILURE;
  }

draw_shape_cleanup:
  msDrawEndShape(map, layer, image, shape);
  if (unclipped_shape != shape) {
    msFreeShape(unclipped_shape);
    msFree(unclipped_shape);
  }
  return ret;
}

/*
** Function to render an individual point, used as a helper function for
*mapscript only. Since a point
** can't carry attributes you can't do attribute based font size or angle.
*/
int msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, imageObj *image,
                int classindex, char *labeltext) {
  int s, ret;
  classObj *theclass = NULL;
  labelObj *label = NULL;

  if (layer->transform == MS_TRUE && layer->project &&
      msProjectionsDiffer(&(layer->projection), &(map->projection))) {
    msProjectPoint(&(layer->projection), &(map->projection), point);
  }

  if (classindex > layer->numclasses) {
    msSetError(MS_MISCERR, "Invalid classindex (%d)", "msDrawPoint()",
               classindex);
    return MS_FAILURE;
  }
  theclass = layer->class[classindex];

  if (labeltext && theclass->numlabels > 0) {
    label = theclass->labels[0];
  }

  switch (layer->type) {
  case MS_LAYER_POINT:
    if (layer->transform == MS_TRUE) {
      if (!msPointInRect(point, &map->extent))
        return (0);
      point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
      point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
    } else
      msOffsetPointRelativeTo(point, layer);

    for (s = 0; s < theclass->numstyles; s++) {
      if (msScaleInBounds(map->scaledenom, theclass->styles[s]->minscaledenom,
                          theclass->styles[s]->maxscaledenom))
        if (MS_UNLIKELY(MS_FAILURE ==
                        msDrawMarkerSymbol(map, image, point,
                                           theclass->styles[s],
                                           theclass->styles[s]->scalefactor))) {
          return MS_FAILURE;
        }
    }
    if (label && labeltext && *labeltext) {
      textSymbolObj *ts = msSmallMalloc(sizeof(textSymbolObj));
      initTextSymbol(ts);
      msPopulateTextSymbolForLabelAndString(
          ts, label, msStrdup(labeltext), label->scalefactor,
          image->resolutionfactor, layer->labelcache);
      if (layer->labelcache) {
        if (msAddLabel(map, image, label, layer->index, classindex, NULL, point,
                       -1, ts) != MS_SUCCESS) {
          return (MS_FAILURE);
        }
      } else {
        if (MS_UNLIKELY(MS_FAILURE == msComputeTextPath(map, ts))) {
          freeTextSymbol(ts);
          free(ts);
          return MS_FAILURE;
        }
        ret = msDrawTextSymbol(map, image, *point, ts);
        freeTextSymbol(ts);
        free(ts);
        if (MS_UNLIKELY(ret == MS_FAILURE))
          return MS_FAILURE;
      }
    }
    break;
  default:
    break; /* don't do anything with layer of other types */
  }

  return (MS_SUCCESS); /* all done, no cleanup */
}

/*
** Draws a single label independently of the label cache. No collision avoidance
*is performed.
*/
int msDrawLabel(mapObj *map, imageObj *image, pointObj labelPnt, char *string,
                labelObj *label, double scalefactor) {
  shapeObj labelPoly;
  label_bounds lbounds = {0};
  lineObj labelPolyLine;
  pointObj labelPolyPoints[5];
  textSymbolObj ts = {0};
  int needLabelPoly = MS_TRUE;
  int needLabelPoint = MS_TRUE;
  int haveLabelText = MS_TRUE;

  if (!string || !*string)
    haveLabelText = MS_FALSE;

  if (haveLabelText) {
    initTextSymbol(&ts);
    msPopulateTextSymbolForLabelAndString(&ts, label, string, scalefactor,
                                          image->resolutionfactor, 0);
    if (MS_UNLIKELY(MS_FAILURE == msComputeTextPath(map, &ts))) {
      freeTextSymbol(&ts);
      return MS_FAILURE;
    }
  }

  labelPoly.line = &labelPolyLine; /* setup the label polygon structure */
  labelPoly.numlines = 1;
  lbounds.poly = &labelPolyLine; /* setup the label polygon structure */
  labelPoly.line->point = labelPolyPoints;
  labelPoly.line->numpoints = 5;

  if (label->position != MS_XY) {
    pointObj p = {0};

    if (label->numstyles > 0) {
      int i;

      for (i = 0; i < label->numstyles; i++) {
        if (label->styles[i]->_geomtransform.type ==
                MS_GEOMTRANSFORM_LABELPOINT ||
            label->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_NONE) {
          if (MS_UNLIKELY(MS_FAILURE ==
                          msDrawMarkerSymbol(map, image, &labelPnt,
                                             label->styles[i], scalefactor))) {
            if (haveLabelText)
              freeTextSymbol(&ts);
            return MS_FAILURE;
          }
        } else if (haveLabelText && (label->styles[i]->_geomtransform.type ==
                                         MS_GEOMTRANSFORM_LABELPOLY ||
                                     label->styles[i]->_geomtransform.type ==
                                         MS_GEOMTRANSFORM_LABELCENTER)) {
          if (needLabelPoly) {
            p = get_metrics(&labelPnt, label->position, ts.textpath,
                            label->offsetx * ts.scalefactor,
                            label->offsety * ts.scalefactor, ts.rotation, 1,
                            &lbounds);
            if (!lbounds.poly) {
              /* we need the full shape to draw the label background */
              labelPolyPoints[0].x = labelPolyPoints[4].x = lbounds.bbox.minx;
              labelPolyPoints[0].y = labelPolyPoints[4].y = lbounds.bbox.miny;
              labelPolyPoints[1].x = lbounds.bbox.minx;
              labelPolyPoints[1].y = lbounds.bbox.maxy;
              labelPolyPoints[2].x = lbounds.bbox.maxx;
              labelPolyPoints[2].y = lbounds.bbox.maxy;
              labelPolyPoints[3].x = lbounds.bbox.maxx;
              labelPolyPoints[3].y = lbounds.bbox.miny;
            }
            needLabelPoint = MS_FALSE; /* don't re-compute */
            needLabelPoly = MS_FALSE;
          }
          if (label->styles[i]->_geomtransform.type ==
              MS_GEOMTRANSFORM_LABELPOLY) {
            if (MS_UNLIKELY(MS_FAILURE == msDrawShadeSymbol(map, image,
                                                            &labelPoly,
                                                            label->styles[i],
                                                            ts.scalefactor))) {
              freeTextSymbol(&ts);
              return MS_FAILURE;
            }
          } else {
            pointObj labelCenter;
            labelCenter.x = (lbounds.bbox.maxx + lbounds.bbox.minx) / 2;
            labelCenter.y = (lbounds.bbox.maxy + lbounds.bbox.miny) / 2;
            if (MS_UNLIKELY(MS_FAILURE == msDrawMarkerSymbol(
                                              map, image, &labelCenter,
                                              label->styles[i], scalefactor))) {
              freeTextSymbol(&ts);
              return MS_FAILURE;
            }
          }
        } else {
          msSetError(MS_MISCERR, "Unknown label geomtransform %s",
                     "msDrawLabel()", label->styles[i]->_geomtransform.string);
          if (haveLabelText)
            freeTextSymbol(&ts);
          return MS_FAILURE;
        }
      }
    }

    if (haveLabelText) {
      if (needLabelPoint)
        p = get_metrics(&labelPnt, label->position, ts.textpath,
                        label->offsetx * ts.scalefactor,
                        label->offsety * ts.scalefactor, ts.rotation, 0,
                        &lbounds);

      /* draw the label text */
      if (MS_UNLIKELY(MS_FAILURE == msDrawTextSymbol(map, image, p, &ts))) {
        freeTextSymbol(&ts);
        return MS_FAILURE;
      }
    }
  } else {
    labelPnt.x += label->offsetx * ts.scalefactor;
    labelPnt.y += label->offsety * ts.scalefactor;

    if (label->numstyles > 0) {
      int i;

      for (i = 0; i < label->numstyles; i++) {
        if (label->styles[i]->_geomtransform.type ==
                MS_GEOMTRANSFORM_LABELPOINT ||
            label->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_NONE) {
          if (MS_UNLIKELY(MS_FAILURE ==
                          msDrawMarkerSymbol(map, image, &labelPnt,
                                             label->styles[i], scalefactor))) {
            freeTextSymbol(&ts);
            return MS_FAILURE;
          }
        } else if (haveLabelText && (label->styles[i]->_geomtransform.type ==
                                         MS_GEOMTRANSFORM_LABELPOLY ||
                                     label->styles[i]->_geomtransform.type ==
                                         MS_GEOMTRANSFORM_LABELCENTER)) {
          if (needLabelPoly) {
            get_metrics(&labelPnt, label->position, ts.textpath,
                        label->offsetx * ts.scalefactor,
                        label->offsety * ts.scalefactor, ts.rotation, 1,
                        &lbounds);
            needLabelPoly = MS_FALSE; /* don't re-compute */
            if (!lbounds.poly) {
              /* we need the full shape to draw the label background */
              labelPolyPoints[0].x = labelPolyPoints[4].x = lbounds.bbox.minx;
              labelPolyPoints[0].y = labelPolyPoints[4].y = lbounds.bbox.miny;
              labelPolyPoints[1].x = lbounds.bbox.minx;
              labelPolyPoints[1].y = lbounds.bbox.maxy;
              labelPolyPoints[2].x = lbounds.bbox.maxx;
              labelPolyPoints[2].y = lbounds.bbox.maxy;
              labelPolyPoints[3].x = lbounds.bbox.maxx;
              labelPolyPoints[3].y = lbounds.bbox.miny;
            }
          }
          if (label->styles[i]->_geomtransform.type ==
              MS_GEOMTRANSFORM_LABELPOLY) {
            if (MS_UNLIKELY(MS_FAILURE ==
                            msDrawShadeSymbol(map, image, &labelPoly,
                                              label->styles[i], scalefactor))) {
              freeTextSymbol(&ts);
              return MS_FAILURE;
            }
          } else {
            pointObj labelCenter;
            labelCenter.x = (lbounds.bbox.maxx + lbounds.bbox.minx) / 2;
            labelCenter.y = (lbounds.bbox.maxy + lbounds.bbox.miny) / 2;
            if (MS_UNLIKELY(MS_FAILURE == msDrawMarkerSymbol(
                                              map, image, &labelCenter,
                                              label->styles[i], scalefactor))) {
              freeTextSymbol(&ts);
              return MS_FAILURE;
            }
          }
        } else {
          msSetError(MS_MISCERR, "Unknown label geomtransform %s",
                     "msDrawLabel()", label->styles[i]->_geomtransform.string);
          if (haveLabelText)
            freeTextSymbol(&ts);
          return MS_FAILURE;
        }
      }
    }

    if (haveLabelText) {
      /* draw the label text */
      if (MS_UNLIKELY(MS_FAILURE ==
                      msDrawTextSymbol(map, image, labelPnt, &ts))) {
        freeTextSymbol(&ts);
        return MS_FAILURE;
      }
    }
  }
  if (haveLabelText)
    freeTextSymbol(&ts);

  return MS_SUCCESS;
}

static inline void offset_bbox(const rectObj *from, rectObj *to, double ox,
                               double oy) {
  to->minx = from->minx + ox;
  to->miny = from->miny + oy;
  to->maxx = from->maxx + ox;
  to->maxy = from->maxy + oy;
}

static inline void offset_label_bounds(const label_bounds *from,
                                       label_bounds *to, double ox, double oy) {
  if (from->poly) {
    int i;
    for (i = 0; i < from->poly->numpoints; i++) {
      to->poly->point[i].x = from->poly->point[i].x + ox;
      to->poly->point[i].y = from->poly->point[i].y + oy;
    }
    to->poly->numpoints = from->poly->numpoints;
  } else {
    to->poly = NULL;
  }
  offset_bbox(&from->bbox, &to->bbox, ox, oy);
}

/* private shortcut function to try a leader offsetted label
 * the caller must ensure that scratch->poly->points has been sufficiently
 * allocated to hold the points from the cachePtr's label_bounds */
void offsetAndTest(mapObj *map, labelCacheMemberObj *cachePtr, double ox,
                   double oy, int priority, int label_idx,
                   label_bounds *scratch) {
  int i, j, status;
  pointObj leaderpt;
  lineObj *scratch_line = scratch->poly;

  for (i = 0; i < cachePtr->numtextsymbols; i++) {
    textSymbolObj *ts = cachePtr->textsymbols[i];
    if (ts->textpath) {
      offset_label_bounds(&ts->textpath->bounds, scratch, ox, oy);
      status = msTestLabelCacheCollisions(map, cachePtr, scratch, priority,
                                          label_idx);
      if (!status) {
        return;
      }
    }
    for (j = 0; j < ts->label->numstyles; j++) {
      if (ts->label->styles[j]->_geomtransform.type ==
          MS_GEOMTRANSFORM_LABELPOINT) {
        scratch->poly = scratch_line;
        offset_label_bounds(ts->style_bounds[j], scratch, ox, oy);
        status = msTestLabelCacheCollisions(map, cachePtr, scratch, priority,
                                            label_idx);
        if (!status) {
          return;
        }
      }
    }
  }

  leaderpt.x = cachePtr->point.x + ox;
  leaderpt.y = cachePtr->point.y + oy;

  status = msTestLabelCacheLeaderCollision(map, &cachePtr->point, &leaderpt);
  if (!status) {
    return;
  }

  /* the current offset is ok */
  cachePtr->leaderbbox = msSmallMalloc(sizeof(rectObj));
  cachePtr->leaderline = msSmallMalloc(sizeof(lineObj));
  cachePtr->leaderline->point = msSmallMalloc(2 * sizeof(pointObj));
  cachePtr->leaderline->numpoints = 2;
  cachePtr->leaderline->point[0] = cachePtr->point;
  cachePtr->leaderline->point[1] = leaderpt;
  cachePtr->leaderbbox->minx = MS_MIN(leaderpt.x, cachePtr->point.x);
  cachePtr->leaderbbox->maxx = MS_MAX(leaderpt.x, cachePtr->point.x);
  cachePtr->leaderbbox->miny = MS_MIN(leaderpt.y, cachePtr->point.y);
  cachePtr->leaderbbox->maxy = MS_MAX(leaderpt.y, cachePtr->point.y);
  cachePtr->status = MS_ON;

  offset_bbox(&cachePtr->bbox, &cachePtr->bbox, ox, oy);

  for (i = 0; i < cachePtr->numtextsymbols; i++) {
    textSymbolObj *ts = cachePtr->textsymbols[i];
    if (ts->textpath) {
      offset_label_bounds(&ts->textpath->bounds, &ts->textpath->bounds, ox, oy);
      ts->annopoint.x += ox;
      ts->annopoint.y += oy;
    }
    if (ts->style_bounds) {
      for (j = 0; j < ts->label->numstyles; j++) {
        if (ts->label->styles[j]->_geomtransform.type != MS_GEOMTRANSFORM_NONE)
          offset_label_bounds(ts->style_bounds[j], ts->style_bounds[j], ox, oy);
      }
    }
  }
}

int msDrawOffsettedLabels(imageObj *image, mapObj *map, int priority) {
  int retval = MS_SUCCESS;
  int l;
  labelCacheObj *labelcache = &(map->labelcache);
  labelCacheSlotObj *cacheslot;
  labelCacheMemberObj *cachePtr;
  label_bounds scratch;
  lineObj scratch_line;
  pointObj *scratch_points = NULL;
  int num_allocated_scratch_points = 0;
  assert(MS_RENDERER_PLUGIN(image->format));
  cacheslot = &(labelcache->slots[priority]);
  scratch.poly = &scratch_line;

  for (l = cacheslot->numlabels - 1; l >= 0; l--) {
    cachePtr =
        &(cacheslot->labels[l]); /* point to right spot in the label cache */
    if (cachePtr->status == MS_OFF) {
      /* only test regular labels that have had their bounding box computed
       and that haven't been rendered  */
      classObj *classPtr =
          (GET_CLASS(map, cachePtr->layerindex, cachePtr->classindex));
      layerObj *layerPtr = (GET_LAYER(map, cachePtr->layerindex));
      int steps, i, num_scratch_points_to_allocate = 0;

      assert(classPtr->leader); /* cachePtrs that don't need to be tested have
                                   been marked as status on or delete */

      if (cachePtr->point.x < labelcache->gutter ||
          cachePtr->point.y < labelcache->gutter ||
          cachePtr->point.x >= image->width - labelcache->gutter ||
          cachePtr->point.y >= image->height - labelcache->gutter) {
        /* don't look for leaders if point is in edge buffer as the leader line
         * would end up chopped off */
        continue;
      }

      for (i = 0; i < cachePtr->numtextsymbols; i++) {
        int j;
        textSymbolObj *ts = cachePtr->textsymbols[i];
        if (ts->textpath && ts->textpath->bounds.poly) {
          num_scratch_points_to_allocate =
              MS_MAX(num_scratch_points_to_allocate,
                     ts->textpath->bounds.poly->numpoints);
        }
        if (ts->style_bounds) {
          for (j = 0; j < ts->label->numstyles; j++) {
            if (ts->label->styles[j]->_geomtransform.type ==
                    MS_GEOMTRANSFORM_LABELPOINT &&
                ts->style_bounds[j]->poly) {
              num_scratch_points_to_allocate =
                  MS_MAX(num_scratch_points_to_allocate,
                         ts->style_bounds[j]->poly->numpoints);
            }
          }
        }
      }
      if (num_scratch_points_to_allocate > num_allocated_scratch_points) {
        scratch_points = msSmallRealloc(
            scratch_points, num_scratch_points_to_allocate * sizeof(pointObj));
        num_allocated_scratch_points = num_scratch_points_to_allocate;
      }

      steps = classPtr->leader->maxdistance / classPtr->leader->gridstep;

#define x0 (cachePtr->point.x)
#define y0 (cachePtr->point.y)
#define gridstepsc (classPtr->leader->gridstep)

#define otest(ox, oy)                                                          \
  if ((x0 + (ox)) >= labelcache->gutter &&                                     \
      (y0 + (oy)) >= labelcache->gutter &&                                     \
      (x0 + (ox)) < image->width - labelcache->gutter &&                       \
      (y0 + (oy)) < image->height - labelcache->gutter) {                      \
    scratch_line.point = scratch_points;                                       \
    scratch.poly = &scratch_line;                                              \
    offsetAndTest(map, cachePtr, (ox), (oy), priority, l, &scratch);           \
    if (cachePtr->status == MS_ON)                                             \
      break;                                                                   \
  }

      /* loop through possible offsetted positions */
      for (i = 1; i <= steps; i++) {

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
        for (j = 1; j < i - 1; j++) {
          /* test the right positions */
          otest(i * gridstepsc, j * gridstepsc);
          otest(i * gridstepsc, -j * gridstepsc);
          /* test the left positions */
          otest(-i * gridstepsc, j * gridstepsc);
          otest(-i * gridstepsc, -j * gridstepsc);
          /* test the top positions */
          otest(j * gridstepsc, -i * gridstepsc);
          otest(-j * gridstepsc, -i * gridstepsc);
          /* test the bottom positions */
          otest(j * gridstepsc, i * gridstepsc);
          otest(-j * gridstepsc, i * gridstepsc);
        }
        if (j < (i - 1))
          break;

        otest(i * gridstepsc, i * gridstepsc);
        otest(-i * gridstepsc, -i * gridstepsc);
        otest(i * gridstepsc, -i * gridstepsc);
        otest(-i * gridstepsc, i * gridstepsc);

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
        otest(i * gridstepsc, 0);
        otest(-i * gridstepsc, 0);
        otest(0, -i * gridstepsc);
        otest(0, i * gridstepsc);
      }
      if (cachePtr->status == MS_ON) {
        int ll;
        shapeObj
            labelLeader; /* label polygon (bounding box, possibly rotated) */
        labelLeader.line =
            cachePtr->leaderline; /* setup the label polygon structure */
        labelLeader.numlines = 1;
        insertRenderedLabelMember(map, cachePtr);

        for (ll = 0; ll < classPtr->leader->numstyles; ll++) {
          retval = msDrawLineSymbol(map, image, &labelLeader,
                                    classPtr->leader->styles[ll],
                                    layerPtr->scalefactor);
          if (MS_UNLIKELY(retval == MS_FAILURE)) {
            goto offset_cleanup;
          }
        }
        for (ll = 0; ll < cachePtr->numtextsymbols; ll++) {
          textSymbolObj *ts = cachePtr->textsymbols[ll];

          if (ts->style_bounds) {
            /* here's where we draw the label styles */
            for (i = 0; i < ts->label->numstyles; i++) {
              if (ts->label->styles[i]->_geomtransform.type ==
                  MS_GEOMTRANSFORM_LABELPOINT) {
                retval = msDrawMarkerSymbol(
                    map, image, &(labelLeader.line->point[1]),
                    ts->label->styles[i], layerPtr->scalefactor);
                if (MS_UNLIKELY(retval == MS_FAILURE)) {
                  goto offset_cleanup;
                }
              } else if (ts->label->styles[i]->_geomtransform.type ==
                         MS_GEOMTRANSFORM_LABELPOLY) {
                retval =
                    msDrawLabelBounds(map, image, ts->style_bounds[i],
                                      ts->label->styles[i], ts->scalefactor);
                if (MS_UNLIKELY(retval == MS_FAILURE)) {
                  goto offset_cleanup;
                }
              } else if (ts->label->styles[i]->_geomtransform.type ==
                         MS_GEOMTRANSFORM_LABELCENTER) {
                pointObj labelCenter;
                labelCenter.x = (ts->style_bounds[i]->bbox.maxx +
                                 ts->style_bounds[i]->bbox.minx) /
                                2;
                labelCenter.y = (ts->style_bounds[i]->bbox.maxy +
                                 ts->style_bounds[i]->bbox.miny) /
                                2;
                retval = msDrawMarkerSymbol(map, image, &labelCenter,
                                            ts->label->styles[i],
                                            layerPtr->scalefactor);
                if (MS_UNLIKELY(retval == MS_FAILURE)) {
                  goto offset_cleanup;
                }
              } else {
                msSetError(MS_MISCERR,
                           "Labels only support LABELPNT, LABELPOLY and "
                           "LABELCENTER GEOMTRANSFORMS",
                           "msDrawOffsettedLabels()");
                retval = MS_FAILURE;
              }
            }
          }
          if (ts->annotext) {
            retval = msDrawTextSymbol(map, image, ts->annopoint, ts);
            if (MS_UNLIKELY(retval == MS_FAILURE)) {
              goto offset_cleanup;
            }
          }
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
         msDrawLineSymbol(&map->symbolset, image, cachePtr->poly, &tstyle,
         layerPtr->scalefactor);
        */
      }
    }
  }

offset_cleanup:

  free(scratch_points);

  return retval;
}

void fastComputeBounds(lineObj *line, rectObj *bounds) {
  int j;
  bounds->minx = bounds->maxx = line->point[0].x;
  bounds->miny = bounds->maxy = line->point[0].y;

  for (j = 0; j < line->numpoints; j++) {
    bounds->minx = MS_MIN(bounds->minx, line->point[j].x);
    bounds->maxx = MS_MAX(bounds->maxx, line->point[j].x);
    bounds->miny = MS_MIN(bounds->miny, line->point[j].y);
    bounds->maxy = MS_MAX(bounds->maxy, line->point[j].y);
  }
}

int computeMarkerBounds(mapObj *map, pointObj *annopoint, textSymbolObj *ts,
                        label_bounds *poly) {
  int i;
  for (i = 0; i < ts->label->numstyles; i++) {
    styleObj *style = ts->label->styles[i];
    if (style->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT &&
        style->symbol < map->symbolset.numsymbols && style->symbol >= 0) {
      double sx, sy;
      int p;
      double aox, aoy;
      symbolObj *symbol = map->symbolset.symbol[style->symbol];
      if (msGetMarkerSize(map, style, &sx, &sy, ts->scalefactor) != MS_SUCCESS)
        return -1; /* real error, different from MS_FALSE, return -1 so we can
                      trap it */
      if (style->angle) {
        pointObj *point = poly->poly->point;
        point[0].x = sx / 2.0;
        point[0].y = sy / 2.0;
        point[1].x = point[0].x;
        point[1].y = -point[0].y;
        point[2].x = -point[0].x;
        point[2].y = -point[0].y;
        point[3].x = -point[0].x;
        point[3].y = point[0].y;
        point[4].x = point[0].x;
        point[4].y = point[0].y;
        if (symbol->anchorpoint_x != 0.5 || symbol->anchorpoint_y != 0.5) {
          aox = (0.5 - symbol->anchorpoint_x) * sx;
          aoy = (0.5 - symbol->anchorpoint_y) * sy;
          for (p = 0; p < 5; p++) {
            point[p].x += aox;
            point[p].y += aoy;
          }
        }
        {
          double rot = -style->angle * MS_DEG_TO_RAD;
          double sina = sin(rot);
          double cosa = cos(rot);
          for (p = 0; p < 5; p++) {
            double tmpx = point[p].x;
            point[p].x = point[p].x * cosa - point[p].y * sina;
            point[p].y = tmpx * sina + point[p].y * cosa;
          }
        }
        aox = annopoint->x + style->offsetx * ts->scalefactor;
        aoy = annopoint->y + style->offsety * ts->scalefactor;
        for (p = 0; p < 5; p++) {
          point[p].x += aox;
          point[p].y += aoy;
        }
        fastComputeBounds(poly->poly, &poly->bbox);
      } else {
        double aox = (0.5 - symbol->anchorpoint_x) * sx + annopoint->x +
                     style->offsetx * ts->scalefactor;
        double aoy = (0.5 - symbol->anchorpoint_y) * sy + annopoint->y +
                     style->offsety * ts->scalefactor;
        poly->poly = NULL;
        poly->bbox.maxx = sx / 2.0 + aox;
        poly->bbox.minx = -sx / 2.0 + aox;
        poly->bbox.maxy = sy / 2.0 + aoy;
        poly->bbox.miny = -sy / 2.0 + aoy;
      }
      break;
    }
  }
  if (i == ts->label->numstyles)
    return MS_FALSE; /* the label has no marker styles */
  else
    return MS_TRUE;
}

/* check that the current entry does not fall close to a label with identical
 * text, if configured so. Currently only checks the first label/text */

int msCheckLabelMinDistance(mapObj *map, labelCacheMemberObj *lc) {
  int i;
  textSymbolObj *s; /* shortcut */
  textSymbolObj *ts;
  rectObj buffered;
  if (lc->numtextsymbols == 0)
    return MS_FALSE; /* no label with text */
  s = lc->textsymbols[0];

  if (!s->annotext || s->label->mindistance <= 0.0 ||
      s->label->force == MS_TRUE)
    return MS_FALSE; /*  min distance is not checked */

  /* we buffer the label and check for intersection instead of calculating
     the distance of two textpaths. we also buffer only the bbox of lc for
     faster computation (it is still compared to the full textpath
     of the label cache members).
  */
  buffered = lc->bbox;
  buffered.minx -= s->label->mindistance * s->resolutionfactor;
  buffered.miny -= s->label->mindistance * s->resolutionfactor;
  buffered.maxx += s->label->mindistance * s->resolutionfactor;
  buffered.maxy += s->label->mindistance * s->resolutionfactor;

  for (i = 0; i < map->labelcache.num_rendered_members; i++) {
    labelCacheMemberObj *ilc = map->labelcache.rendered_text_symbols[i];
    if (ilc->numtextsymbols == 0 || !ilc->textsymbols[0]->annotext)
      continue;

    ts = ilc->textsymbols[0];
    if (strcmp(s->annotext, ts->annotext) != 0) {
      /* only check min distance against same label */
      continue;
    }

    if (msPointInRect(&ilc->point, &buffered) == MS_TRUE) {
      return MS_TRUE;
    }

    if (ts->textpath && ts->textpath->absolute) {
      if (intersectLabelPolygons(ts->textpath->bounds.poly, &ilc->bbox, NULL,
                                 &buffered) == MS_TRUE) {
        return MS_TRUE;
      }
      continue;
    }

    if (intersectLabelPolygons(NULL, &ilc->bbox, NULL, &buffered) == MS_TRUE) {
      return MS_TRUE;
    }
  }
  return MS_FALSE;
}

void copyLabelBounds(label_bounds *dst, label_bounds *src) {
  *dst = *src;
  if (src->poly) {
    int i;
    dst->poly = msSmallMalloc(sizeof(lineObj));
    dst->poly->numpoints = src->poly->numpoints;
    dst->poly->point = msSmallMalloc(dst->poly->numpoints * sizeof(pointObj));
    for (i = 0; i < dst->poly->numpoints; i++) {
      dst->poly->point[i] = src->poly->point[i];
    }
  }
}

static int getLabelPositionFromString(char *pszString) {
  if (strcasecmp(pszString, "UL") == 0)
    return MS_UL;
  else if (strcasecmp(pszString, "LR") == 0)
    return MS_LR;
  else if (strcasecmp(pszString, "UR") == 0)
    return MS_UR;
  else if (strcasecmp(pszString, "LL") == 0)
    return MS_LL;
  else if (strcasecmp(pszString, "CR") == 0)
    return MS_CR;
  else if (strcasecmp(pszString, "CL") == 0)
    return MS_CL;
  else if (strcasecmp(pszString, "UC") == 0)
    return MS_UC;
  else if (strcasecmp(pszString, "LC") == 0)
    return MS_LC;
  else
    return MS_CC;
}

int msDrawLabelCache(mapObj *map, imageObj *image) {
  int nReturnVal = MS_SUCCESS;
  struct mstimeval starttime = {0}, endtime = {0};

  if (map->debug >= MS_DEBUGLEVEL_TUNING)
    msGettimeofday(&starttime, NULL);

  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
      int i, l, ll, priority, its;

      double marker_offset_x, marker_offset_y;
      int label_offset_x, label_offset_y;
      const char *value;

      labelCacheMemberObj *cachePtr = NULL;
      layerObj *layerPtr = NULL;
      classObj *classPtr = NULL;
      textSymbolObj *textSymbolPtr = NULL;

      /*
       * some statically allocated containers for storing label bounds before
       * copying them into the cachePtr: we avoid allocating these structures
       * at runtime, except for the labels that are actually rendered.
       */
      lineObj labelpoly_line;
      pointObj labelpoly_points[5];
      label_bounds labelpoly_bounds = {0};
      lineObj label_marker_line;
      pointObj label_marker_points[5];
      label_bounds label_marker_bounds;
      lineObj metrics_line;
      pointObj metrics_points[5];
      label_bounds metrics_bounds;

      label_marker_line.point = label_marker_points;
      label_marker_line.numpoints = 5;
      metrics_line.point = metrics_points;
      metrics_line.numpoints = 5;
      labelpoly_line.point = labelpoly_points;
      labelpoly_line.numpoints = 5;

      /* Look for labelcache_map_edge_buffer map metadata
       * If set then the value defines a buffer (in pixels) along the edge of
       * the map image where labels can't fall
       */
      if ((value = msLookupHashTable(&(map->web.metadata),
                                     "labelcache_map_edge_buffer")) != NULL) {
        map->labelcache.gutter = MS_ABS(atoi(value));
        if (map->debug)
          msDebug("msDrawLabelCache(): labelcache_map_edge_buffer = %d\n",
                  map->labelcache.gutter);
      }

      for (priority = MS_MAX_LABEL_PRIORITY - 1; priority >= 0; priority--) {
        labelCacheSlotObj *cacheslot;
        cacheslot = &(map->labelcache.slots[priority]);

        for (l = cacheslot->numlabels - 1; l >= 0; l--) {
          cachePtr =
              &(cacheslot
                    ->labels[l]); /* point to right spot in the label cache */

          layerPtr = (GET_LAYER(
              map, cachePtr->layerindex)); /* set a couple of other pointers,
                                              avoids nasty references */
          classPtr =
              (GET_CLASS(map, cachePtr->layerindex, cachePtr->classindex));

          if (cachePtr->textsymbols[0]->textpath &&
              cachePtr->textsymbols[0]->textpath->absolute) {
            /* we have an angle follow label */
            cachePtr->bbox = cachePtr->textsymbols[0]->textpath->bounds.bbox;

            /* before going any futher, check that mindistance is respected */
            if (cachePtr->numtextsymbols &&
                cachePtr->textsymbols[0]->label->mindistance > 0.0 &&
                cachePtr->textsymbols[0]->annotext) {
              if (msCheckLabelMinDistance(map, cachePtr) == MS_TRUE) {
                cachePtr->status = MS_DELETE;
                MS_DEBUG(MS_DEBUGLEVEL_DEVDEBUG, map,
                         "Skipping labelgroup %d \"%s\" in layer \"%s\": too "
                         "close to an identical label (mindistance)\n",
                         l, cachePtr->textsymbols[0]->annotext, layerPtr->name);
                continue; /* move on to next entry, this one is too close to an
                             already placed one */
              }
            }

            if (!cachePtr->textsymbols[0]->label->force)
              cachePtr->status = msTestLabelCacheCollisions(
                  map, cachePtr, &cachePtr->textsymbols[0]->textpath->bounds,
                  priority, l);
            else
              cachePtr->status = MS_ON;
            if (cachePtr->status) {

              if (MS_UNLIKELY(
                      MS_FAILURE ==
                      msDrawTextSymbol(
                          map, image,
                          cachePtr->textsymbols[0]->annopoint /*not used*/,
                          cachePtr->textsymbols[0]))) {
                return MS_FAILURE;
              }
              insertRenderedLabelMember(map, cachePtr);
            } else {
              MS_DEBUG(MS_DEBUGLEVEL_DEVDEBUG, map,
                       "Skipping follow labelgroup %d \"%s\" in layer \"%s\": "
                       "text collided\n",
                       l, cachePtr->textsymbols[0]->annotext, layerPtr->name);
            }
            cachePtr->status =
                MS_DELETE; /* we're done with this label, it won't even have a
                              second chance in the leader phase */
          } else {
            marker_offset_x = marker_offset_y = 0; /* assume no marker */

            if (layerPtr->type == MS_LAYER_POINT &&
                cachePtr->markerid !=
                    -1) { /* there is a marker already in the image that we need
                             to account for */
              markerCacheMemberObj *markerPtr = &(
                  cacheslot
                      ->markers[cachePtr->markerid]); /* point to the right spot
                                                         in the marker cache*/
              marker_offset_x =
                  (markerPtr->bounds.maxx - markerPtr->bounds.minx) / 2.0;
              marker_offset_y =
                  (markerPtr->bounds.maxy - markerPtr->bounds.miny) / 2.0;
            }

            /*
            ** all other cases *could* have multiple labels defined
            */
            cachePtr->status =
                MS_ON; /* assume this cache element can be placed */
            for (ll = 0; ll < cachePtr->numtextsymbols;
                 ll++) { /* RFC 77 TODO: Still may want to step through
                            backwards... */
              int label_marker_status = MS_ON, have_label_marker,
                  metrics_status = MS_ON;
              int need_labelpoly = 0;

              /* reset the lineObj which may have been unset by a previous call
               * to get_metrics() */
              label_marker_bounds.poly = &label_marker_line;
              labelpoly_bounds.poly = &labelpoly_line;

              textSymbolPtr = cachePtr->textsymbols[ll];
              for (i = 0; i < textSymbolPtr->label->numstyles; i++) {
                if (textSymbolPtr->label->styles[i]->_geomtransform.type ==
                        MS_GEOMTRANSFORM_LABELPOLY ||
                    textSymbolPtr->label->styles[i]->_geomtransform.type ==
                        MS_GEOMTRANSFORM_LABELCENTER) {
                  need_labelpoly = 1;
                  break;
                }
              }

              /* compute the poly of the label styles */
              if ((have_label_marker =
                       computeMarkerBounds(map, &cachePtr->point, textSymbolPtr,
                                           &label_marker_bounds)) == MS_TRUE) {
                if (cachePtr->numtextsymbols >
                    1) { /* FIXME this test doesn't seem right, should probably
                            check if we have an annotation layer with a regular
                            style defined */
                  marker_offset_x = (label_marker_bounds.bbox.maxx -
                                     label_marker_bounds.bbox.minx) /
                                    2.0;
                  marker_offset_y = (label_marker_bounds.bbox.maxy -
                                     label_marker_bounds.bbox.miny) /
                                    2.0;
                } else {
                  /* we might be using an old style behavior with a markerPtr */
                  marker_offset_x =
                      MS_MAX(marker_offset_x, (label_marker_bounds.bbox.maxx -
                                               label_marker_bounds.bbox.minx) /
                                                  2.0);
                  marker_offset_y =
                      MS_MAX(marker_offset_y, (label_marker_bounds.bbox.maxy -
                                               label_marker_bounds.bbox.miny) /
                                                  2.0);
                }
                /* add marker to cachePtr->poly */
                if (textSymbolPtr->label->force != MS_TRUE) {
                  label_marker_status = msTestLabelCacheCollisions(
                      map, cachePtr, &label_marker_bounds, priority, l);
                }
                if (label_marker_status == MS_OFF &&
                    !(textSymbolPtr->label->force == MS_ON ||
                      classPtr->leader)) {
                  cachePtr->status = MS_DELETE;
                  MS_DEBUG(MS_DEBUGLEVEL_DEVDEBUG, map,
                           "Skipping label %d of labelgroup %d of class %d in "
                           "layer \"%s\": marker collided\n",
                           ll, l, cachePtr->classindex, layerPtr->name);
                  break; /* the marker collided, break from multi-label loop */
                }
              }
              if (have_label_marker == -1)
                return MS_FAILURE; /* error occured (symbol not found, etc...)
                                    */

              if (textSymbolPtr->annotext) {
                /*
                 * if we don't have an offset defined, first check that the
                 * labelpoint itself does not collide this helps speed things up
                 * in dense labelling, as if the labelpoint collides there's no
                 * use in computing the labeltext bounds (i.e. going into
                 * shaping+freetype). We do however skip collision testing
                 * against the marker cache, as we want to allow rendering a
                 * label for points that overlap each other: this is done by
                 * setting priority to MAX_PRIORITY in the call to
                 * msTestLabelCacheCollisions (which is a hack!!)
                 */
                if (!have_label_marker &&
                    textSymbolPtr->label->force != MS_TRUE &&
                    !classPtr->leader && !textSymbolPtr->label->offsetx &&
                    !textSymbolPtr->label->offsety) {
                  label_bounds labelpoint_bounds;
                  labelpoint_bounds.poly = NULL;
                  labelpoint_bounds.bbox.minx = cachePtr->point.x - 0.1;
                  labelpoint_bounds.bbox.maxx = cachePtr->point.x + 0.1;
                  labelpoint_bounds.bbox.miny = cachePtr->point.y - 0.1;
                  labelpoint_bounds.bbox.maxy = cachePtr->point.y + 0.1;
                  if (MS_OFF == msTestLabelCacheCollisions(
                                    map, cachePtr, &labelpoint_bounds,
                                    MS_MAX_LABEL_PRIORITY, l)) {
                    cachePtr->status =
                        MS_DELETE; /* we won't check for leader offseted
                                      positions, as the anchor point colided */
                    MS_DEBUG(MS_DEBUGLEVEL_DEVDEBUG, map,
                             "Skipping label %d \"%s\" of labelgroup %d of "
                             "class %d in layer \"%s\": labelpoint collided\n",
                             ll, textSymbolPtr->annotext, l,
                             cachePtr->classindex, layerPtr->name);
                    break;
                  }
                }

                /* compute label size */
                if (!textSymbolPtr->textpath) {
                  if (MS_UNLIKELY(MS_FAILURE ==
                                  msComputeTextPath(map, textSymbolPtr))) {
                    return MS_FAILURE;
                  }
                }

                /* if our label has an outline, adjust the marker offset so the
                 * outlinecolor does not bleed into the marker */
                if (marker_offset_x &&
                    MS_VALID_COLOR(textSymbolPtr->label->outlinecolor)) {
                  marker_offset_x += textSymbolPtr->label->outlinewidth / 2.0 *
                                     textSymbolPtr->scalefactor;
                  marker_offset_y += textSymbolPtr->label->outlinewidth / 2.0 *
                                     textSymbolPtr->scalefactor;
                }

                /* apply offset and buffer settings */
                if (textSymbolPtr->label->anglemode != MS_FOLLOW) {
                  label_offset_x = textSymbolPtr->label->offsetx *
                                   textSymbolPtr->scalefactor;
                  label_offset_y = textSymbolPtr->label->offsety *
                                   textSymbolPtr->scalefactor;
                } else {
                  label_offset_x = 0;
                  label_offset_y = 0;
                }

                if (textSymbolPtr->label->position == MS_AUTO) {
                  /* no point in using auto positionning if the marker cannot be
                   * placed */
                  int positions[MS_POSITIONS_LENGTH], npositions = 0;

                  /*
                  **   (Note: might be able to re-order this for more speed.)
                  */
                  if (msLayerGetProcessingKey(layerPtr, "LABEL_POSITIONS")) {
                    int p, ncustom_positions = 0;
                    char **custom_positions = msStringSplitComplex(
                        msLayerGetProcessingKey(layerPtr, "LABEL_POSITIONS"),
                        ",", &ncustom_positions,
                        MS_STRIPLEADSPACES | MS_STRIPENDSPACES);
                    for (p = 0; p < MS_MIN(9, ncustom_positions); p++)
                      positions[p] =
                          getLabelPositionFromString(custom_positions[p]);
                    npositions = p;
                    msFree(custom_positions);
                  } else if (layerPtr->type == MS_LAYER_POLYGON &&
                             marker_offset_x == 0) {
                    positions[0] = MS_CC;
                    positions[1] = MS_UC;
                    positions[2] = MS_LC;
                    positions[3] = MS_CL;
                    positions[4] = MS_CR;
                    npositions = 5;
                  } else if (layerPtr->type == MS_LAYER_LINE &&
                             marker_offset_x == 0) {
                    positions[0] = MS_UC;
                    positions[1] = MS_LC;
                    positions[2] = MS_CC;
                    npositions = 3;
                  } else {
                    positions[0] = MS_UL;
                    positions[1] = MS_LR;
                    positions[2] = MS_UR;
                    positions[3] = MS_LL;
                    positions[4] = MS_CR;
                    positions[5] = MS_CL;
                    positions[6] = MS_UC;
                    positions[7] = MS_LC;
                    npositions = 8;
                  }

                  for (i = 0; i < npositions; i++) {
                    // RFC 77 TODO: take label_marker_offset_x/y into account
                    metrics_bounds.poly = &metrics_line;
                    textSymbolPtr->annopoint =
                        get_metrics(&(cachePtr->point), positions[i],
                                    textSymbolPtr->textpath,
                                    marker_offset_x + label_offset_x,
                                    marker_offset_y + label_offset_y,
                                    textSymbolPtr->rotation,
                                    textSymbolPtr->label->buffer *
                                        textSymbolPtr->scalefactor,
                                    &metrics_bounds);
                    if (textSymbolPtr->label->force == MS_OFF) {
                      for (its = 0; its < ll; its++) {
                        /* check for collisions inside the label group */
                        if (intersectTextSymbol(cachePtr->textsymbols[its],
                                                &metrics_bounds) == MS_TRUE) {
                          /* there was a self intersection */
                          break; /* next position, will break out to next
                                    position in containing loop*/
                        }
                        if (its != ll)
                          continue; /* goto next position, this one had an
                                       intersection with our own label group */
                      }
                    }

                    metrics_status = msTestLabelCacheCollisions(
                        map, cachePtr, &metrics_bounds, priority, l);

                    /* found a suitable place for this label */
                    if (metrics_status == MS_TRUE ||
                        (i == (npositions - 1) &&
                         textSymbolPtr->label->force == MS_ON)) {
                      metrics_status =
                          MS_TRUE; /* set to true in case we are forcing it */
                      /* compute anno poly for label background if needed */
                      if (need_labelpoly)
                        get_metrics(&(cachePtr->point), positions[i],
                                    textSymbolPtr->textpath,
                                    marker_offset_x + label_offset_x,
                                    marker_offset_y + label_offset_y,
                                    textSymbolPtr->rotation, 1,
                                    &labelpoly_bounds);

                      break; /* ...out of position loop */
                    }
                  } /* next position */

                  /* if position auto didn't manage to find a position, but we
                   * have leader configured for the class, then we want to
                   * compute the label poly anyway, placed as MS_CC */
                  if (classPtr->leader && metrics_status == MS_FALSE) {
                    metrics_bounds.poly = &metrics_line;
                    textSymbolPtr->annopoint = get_metrics(
                        &(cachePtr->point), MS_CC, textSymbolPtr->textpath,
                        label_offset_x, label_offset_y, textSymbolPtr->rotation,
                        textSymbolPtr->label->buffer *
                            textSymbolPtr->scalefactor,
                        &metrics_bounds);
                    if (need_labelpoly)
                      get_metrics(&(cachePtr->point), MS_CC,
                                  textSymbolPtr->textpath, label_offset_x,
                                  label_offset_y, textSymbolPtr->rotation, 1,
                                  &labelpoly_bounds);
                  }
                } else { /* explicit position */
                  if (textSymbolPtr->label->position ==
                      MS_CC) { /* don't need the marker_offset */
                    metrics_bounds.poly = &metrics_line;
                    textSymbolPtr->annopoint = get_metrics(
                        &(cachePtr->point), MS_CC, textSymbolPtr->textpath,
                        label_offset_x, label_offset_y, textSymbolPtr->rotation,
                        textSymbolPtr->label->buffer *
                            textSymbolPtr->scalefactor,
                        &metrics_bounds);
                    if (need_labelpoly)
                      get_metrics(&(cachePtr->point), MS_CC,
                                  textSymbolPtr->textpath, label_offset_x,
                                  label_offset_y, textSymbolPtr->rotation, 1,
                                  &labelpoly_bounds);
                  } else {
                    metrics_bounds.poly = &metrics_line;
                    textSymbolPtr->annopoint = get_metrics(
                        &(cachePtr->point), textSymbolPtr->label->position,
                        textSymbolPtr->textpath,
                        marker_offset_x + label_offset_x,
                        marker_offset_y + label_offset_y,
                        textSymbolPtr->rotation,
                        textSymbolPtr->label->buffer *
                            textSymbolPtr->scalefactor,
                        &metrics_bounds);
                    if (need_labelpoly)
                      get_metrics(
                          &(cachePtr->point), textSymbolPtr->label->position,
                          textSymbolPtr->textpath,
                          marker_offset_x + label_offset_x,
                          marker_offset_y + label_offset_y,
                          textSymbolPtr->rotation, 1, &labelpoly_bounds);
                  }

                  if (textSymbolPtr->label->force == MS_ON) {
                    metrics_status = MS_ON;
                  } else {
                    if (textSymbolPtr->label->force == MS_OFF) {
                      /* check for collisions inside the label group unless the
                       * label is FORCE GROUP */
                      for (its = 0; its < ll; its++) {
                        /* check for collisions inside the label group */
                        if (intersectTextSymbol(cachePtr->textsymbols[its],
                                                &metrics_bounds) == MS_TRUE) {
                          /* there was a self intersection */
                          break; /* will break out to next position in
                                    containing loop*/
                        }
                        if (its != ll) {
                          cachePtr->status = MS_DELETE; /* TODO RFC98: check if
                                                           this is correct */
                          MS_DEBUG(
                              MS_DEBUGLEVEL_DEVDEBUG, map,
                              "Skipping label %d (\"%s\") of labelgroup %d of "
                              "class %d in layer \"%s\": intercollision with "
                              "label text inside labelgroup\n",
                              ll, textSymbolPtr->annotext, l,
                              cachePtr->classindex, layerPtr->name);
                          break; /* collision within the group, break out of
                                    textSymbol loop */
                        }
                      }
                    }
                    /* TODO: in case we have leader lines and multiple labels,
                     * there's no use in testing for labelcache collisions once
                     * a first collision has been found. we only need to know
                     * that the label group has collided, and the poly of the
                     * whole label group: if(label_group)
                     * testLabelCacheCollisions */
                    metrics_status = msTestLabelCacheCollisions(
                        map, cachePtr, &metrics_bounds, priority, l);
                  }
                } /* end POSITION AUTO vs Fixed POSITION */

                if (!metrics_status && classPtr->leader == 0) {
                  cachePtr->status = MS_DELETE;
                  MS_DEBUG(MS_DEBUGLEVEL_DEVDEBUG, map,
                           "Skipping label %d \"%s\" of labelgroup %d of class "
                           "%d in layer \"%s\": text collided\n",
                           ll, textSymbolPtr->annotext, l, cachePtr->classindex,
                           layerPtr->name);
                  break; /* no point looking at more labels, unless their is a
                            leader defined */
                }
              }

              /* if we're here, we can either fit the label directly, or we need
               * to put it in the leader queue */
              assert((metrics_status && label_marker_status) ||
                     classPtr->leader);

              if (textSymbolPtr->annotext) {
                copyLabelBounds(&textSymbolPtr->textpath->bounds,
                                &metrics_bounds);
              }
              if (have_label_marker) {
                if (!textSymbolPtr->style_bounds)
                  textSymbolPtr->style_bounds = msSmallCalloc(
                      textSymbolPtr->label->numstyles, sizeof(label_bounds *));
                for (its = 0; its < textSymbolPtr->label->numstyles; its++) {
                  if (textSymbolPtr->label->styles[its]->_geomtransform.type ==
                      MS_GEOMTRANSFORM_LABELPOINT) {
                    textSymbolPtr->style_bounds[its] =
                        msSmallMalloc(sizeof(label_bounds));
                    copyLabelBounds(textSymbolPtr->style_bounds[its],
                                    &label_marker_bounds);
                  }
                }
              }
              if (!label_marker_status || !metrics_status) {
                MS_DEBUG(MS_DEBUGLEVEL_DEVDEBUG, map,
                         "Putting label %d of labelgroup %d of class %d , "
                         "layer \"%s\" in leader queue\n",
                         ll, l, cachePtr->classindex, layerPtr->name);
                cachePtr->status =
                    MS_OFF; /* we have a collision, but this entry is a
                               candidate for leader testing */
              }

              /* do we need to copy the labelpoly, or can we use the static one
               * ?*/
              if (cachePtr->numtextsymbols > 1 ||
                  (cachePtr->status == MS_OFF && classPtr->leader)) {
                /*
                 * if we have more than one label, or if we have a single one
                 * which didn't fit but needs to go through leader offset
                 * testing
                 */
                if (!textSymbolPtr->style_bounds)
                  textSymbolPtr->style_bounds = msSmallCalloc(
                      textSymbolPtr->label->numstyles, sizeof(label_bounds *));
                for (its = 0; its < textSymbolPtr->label->numstyles; its++) {
                  if (textSymbolPtr->label->styles[its]->_geomtransform.type ==
                          MS_GEOMTRANSFORM_LABELPOLY ||
                      textSymbolPtr->label->styles[its]->_geomtransform.type ==
                          MS_GEOMTRANSFORM_LABELCENTER) {
                    textSymbolPtr->style_bounds[its] =
                        msSmallMalloc(sizeof(label_bounds));
                    copyLabelBounds(textSymbolPtr->style_bounds[its],
                                    &labelpoly_bounds);
                  }
                }

              } /* else: we'll use labelpoly_bounds directly below */
            }   /* next label in the group */

            if (cachePtr->status != MS_DELETE) {
              /* compute the global label bbox */
              int inited = 0, s;
              for (its = 0; its < cachePtr->numtextsymbols; its++) {
                if (cachePtr->textsymbols[its]->annotext) {
                  if (inited == 0) {
                    cachePtr->bbox =
                        cachePtr->textsymbols[its]->textpath->bounds.bbox;
                    inited = 1;
                  } else {
                    cachePtr->bbox.minx = MS_MIN(
                        cachePtr->bbox.minx,
                        cachePtr->textsymbols[its]->textpath->bounds.bbox.minx);
                    cachePtr->bbox.miny = MS_MIN(
                        cachePtr->bbox.miny,
                        cachePtr->textsymbols[its]->textpath->bounds.bbox.miny);
                    cachePtr->bbox.maxx = MS_MAX(
                        cachePtr->bbox.maxx,
                        cachePtr->textsymbols[its]->textpath->bounds.bbox.maxx);
                    cachePtr->bbox.maxy = MS_MAX(
                        cachePtr->bbox.maxy,
                        cachePtr->textsymbols[its]->textpath->bounds.bbox.maxy);
                  }
                }
                for (s = 0; s < cachePtr->textsymbols[its]->label->numstyles;
                     s++) {
                  if (cachePtr->textsymbols[its]
                          ->label->styles[s]
                          ->_geomtransform.type ==
                      MS_GEOMTRANSFORM_LABELPOINT) {
                    if (inited == 0) {
                      cachePtr->bbox =
                          cachePtr->textsymbols[its]->style_bounds[s]->bbox;
                      inited = 1;
                      break;
                    } else {
                      cachePtr->bbox.minx =
                          MS_MIN(cachePtr->bbox.minx, cachePtr->textsymbols[its]
                                                          ->style_bounds[s]
                                                          ->bbox.minx);
                      cachePtr->bbox.miny =
                          MS_MIN(cachePtr->bbox.miny, cachePtr->textsymbols[its]
                                                          ->style_bounds[s]
                                                          ->bbox.miny);
                      cachePtr->bbox.maxx =
                          MS_MAX(cachePtr->bbox.maxx, cachePtr->textsymbols[its]
                                                          ->style_bounds[s]
                                                          ->bbox.maxx);
                      cachePtr->bbox.maxy =
                          MS_MAX(cachePtr->bbox.maxy, cachePtr->textsymbols[its]
                                                          ->style_bounds[s]
                                                          ->bbox.maxy);
                      break;
                    }
                  }
                }
              }
            }

            /* check that mindistance is respected */
            if (cachePtr->numtextsymbols &&
                cachePtr->textsymbols[0]->label->mindistance > 0.0 &&
                cachePtr->textsymbols[0]->annotext) {
              if (msCheckLabelMinDistance(map, cachePtr) == MS_TRUE) {
                cachePtr->status = MS_DELETE;
                MS_DEBUG(MS_DEBUGLEVEL_DEVDEBUG, map,
                         "Skipping labelgroup %d \"%s\" in layer \"%s\": too "
                         "close to an identical label (mindistance)\n",
                         l, cachePtr->textsymbols[0]->annotext, layerPtr->name);
              }
            }

            if (cachePtr->status == MS_OFF || cachePtr->status == MS_DELETE)
              continue; /* next labelCacheMemberObj, as we had a collision */

            /* insert the rendered label */
            insertRenderedLabelMember(map, cachePtr);

            for (ll = 0; ll < cachePtr->numtextsymbols; ll++) {
              textSymbolPtr = cachePtr->textsymbols[ll];

              /* here's where we draw the label styles */
              for (i = 0; i < textSymbolPtr->label->numstyles; i++) {
                if (textSymbolPtr->label->styles[i]->_geomtransform.type ==
                    MS_GEOMTRANSFORM_LABELPOINT) {
                  if (MS_UNLIKELY(
                          MS_FAILURE ==
                          msDrawMarkerSymbol(map, image, &(cachePtr->point),
                                             textSymbolPtr->label->styles[i],
                                             textSymbolPtr->scalefactor))) {
                    return MS_FAILURE;
                  }
                } else if (textSymbolPtr->annotext &&
                           textSymbolPtr->label->styles[i]
                                   ->_geomtransform.type ==
                               MS_GEOMTRANSFORM_LABELPOLY) {
                  if (textSymbolPtr->style_bounds &&
                      textSymbolPtr->style_bounds[i]) {
                    if (MS_UNLIKELY(
                            MS_FAILURE ==
                            msDrawLabelBounds(map, image,
                                              textSymbolPtr->style_bounds[i],
                                              textSymbolPtr->label->styles[i],
                                              textSymbolPtr->scalefactor))) {
                      return MS_FAILURE;
                    }
                  } else {
                    if (MS_UNLIKELY(
                            MS_FAILURE ==
                            msDrawLabelBounds(map, image, &labelpoly_bounds,
                                              textSymbolPtr->label->styles[i],
                                              textSymbolPtr->scalefactor))) {
                      return MS_FAILURE;
                    }
                  }
                } else if (textSymbolPtr->annotext &&
                           textSymbolPtr->label->styles[i]
                                   ->_geomtransform.type ==
                               MS_GEOMTRANSFORM_LABELCENTER) {
                  pointObj labelCenter;

                  if (textSymbolPtr->style_bounds &&
                      textSymbolPtr->style_bounds[i]) {
                    labelCenter.x =
                        (textSymbolPtr->style_bounds[i]->bbox.maxx +
                         textSymbolPtr->style_bounds[i]->bbox.minx) /
                        2;
                    labelCenter.y =
                        (textSymbolPtr->style_bounds[i]->bbox.maxy +
                         textSymbolPtr->style_bounds[i]->bbox.miny) /
                        2;
                    if (MS_UNLIKELY(
                            MS_FAILURE ==
                            msDrawMarkerSymbol(map, image, &labelCenter,
                                               textSymbolPtr->label->styles[i],
                                               textSymbolPtr->scalefactor))) {
                      return MS_FAILURE;
                    }
                  } else {
                    labelCenter.x = (labelpoly_bounds.bbox.maxx +
                                     labelpoly_bounds.bbox.minx) /
                                    2;
                    labelCenter.y = (labelpoly_bounds.bbox.maxy +
                                     labelpoly_bounds.bbox.miny) /
                                    2;
                    if (MS_UNLIKELY(
                            MS_FAILURE ==
                            msDrawMarkerSymbol(map, image, &labelCenter,
                                               textSymbolPtr->label->styles[i],
                                               textSymbolPtr->scalefactor))) {
                      return MS_FAILURE;
                    }
                  }

                } else {
                  msSetError(MS_MISCERR,
                             "Labels only support LABELPNT, LABELPOLY and "
                             "LABELCENTER GEOMTRANSFORMS",
                             "msDrawLabelCache()");
                  return MS_FAILURE;
                }
              }

              if (textSymbolPtr->annotext) {
                if (MS_UNLIKELY(MS_FAILURE ==
                                msDrawTextSymbol(map, image,
                                                 textSymbolPtr->annopoint,
                                                 textSymbolPtr))) {
                  return MS_FAILURE;
                }
              }
            }
          }
        } /* next label(group) from cacheslot */
        if (MS_UNLIKELY(MS_FAILURE ==
                        msDrawOffsettedLabels(image, map, priority))) {
          return MS_FAILURE;
        }
      } /* next priority */
#ifdef TBDEBUG
      styleObj tstyle;
      initStyle(&tstyle);
      tstyle.width = 1;
      tstyle.color.alpha = 255;
      tstyle.color.red = 255;
      tstyle.color.green = 0;
      tstyle.color.blue = 0;
      for (priority = MS_MAX_LABEL_PRIORITY - 1; priority >= 0; priority--) {
        labelCacheSlotObj *cacheslot;
        cacheslot = &(map->labelcache.slots[priority]);

        for (l = cacheslot->numlabels - 1; l >= 0; l--) {
          cachePtr =
              &(cacheslot
                    ->labels[l]); /* point to right spot in the label cache */
          /*
          assert((cachePtr->poly == NULL && cachePtr->status == MS_OFF) ||
                  (cachePtr->poly && (cachePtr->status == MS_ON));
           */
          if (cachePtr->status) {
            msDrawLineSymbol(map, image, cachePtr->poly, &tstyle,
                             layerPtr->scalefactor);
          }
        }
      }
#endif

      nReturnVal = MS_SUCCESS; /* necessary? */
    }
  }

  if (map->debug >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&endtime, NULL);
    msDebug("msDrawMap(): Drawing Label Cache, %.3fs\n",
            (endtime.tv_sec + endtime.tv_usec / 1.0e6) -
                (starttime.tv_sec + starttime.tv_usec / 1.0e6));
  }

  return nReturnVal;
}

/**
 * Generic function to tell the underline device that layer
 * drawing is stating
 */

void msImageStartLayer(mapObj *map, layerObj *layer, imageObj *image) {
  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
      const char *approximation_scale =
          msLayerGetProcessingKey(layer, "APPROXIMATION_SCALE");
      if (approximation_scale) {
        if (!strncasecmp(approximation_scale, "ROUND", 5)) {
          MS_IMAGE_RENDERER(image)->transform_mode = MS_TRANSFORM_ROUND;
        } else if (!strncasecmp(approximation_scale, "FULL", 4)) {
          MS_IMAGE_RENDERER(image)->transform_mode =
              MS_TRANSFORM_FULLRESOLUTION;
        } else if (!strncasecmp(approximation_scale, "SIMPLIFY", 8)) {
          MS_IMAGE_RENDERER(image)->transform_mode = MS_TRANSFORM_SIMPLIFY;
        } else {
          MS_IMAGE_RENDERER(image)->transform_mode = MS_TRANSFORM_SNAPTOGRID;
          MS_IMAGE_RENDERER(image)->approximation_scale =
              atof(approximation_scale);
        }
      } else {
        MS_IMAGE_RENDERER(image)->transform_mode =
            MS_IMAGE_RENDERER(image)->default_transform_mode;
        MS_IMAGE_RENDERER(image)->approximation_scale =
            MS_IMAGE_RENDERER(image)->default_approximation_scale;
      }
      MS_IMAGE_RENDERER(image)->startLayer(image, map, layer);
    } else if (MS_RENDERER_IMAGEMAP(image->format))
      msImageStartLayerIM(map, layer, image);
  }
}

/**
 * Generic function to tell the underline device that layer
 * drawing is ending
 */

void msImageEndLayer(mapObj *map, layerObj *layer, imageObj *image) {
  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
      MS_IMAGE_RENDERER(image)->endLayer(image, map, layer);
    }
  }
}

/**
 * Generic function to tell the underline device that shape
 * drawing is starting
 */

void msDrawStartShape(mapObj *map, layerObj *layer, imageObj *image,
                      shapeObj *shape) {
  (void)map;
  (void)layer;
  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
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
                    shapeObj *shape) {
  (void)map;
  (void)layer;
  if (MS_RENDERER_PLUGIN(image->format)) {
    if (image->format->vtable->endShape)
      image->format->vtable->endShape(image, shape);
  }
}
/**
 * take the value from the shape and use it to change the
 * color in the style to match the range map
 */
int msShapeToRange(styleObj *style, shapeObj *shape) {
  /*first, get the value of the rangeitem, which should*/
  /*evaluate to a double*/
  const char *fieldStr = shape->values[style->rangeitemindex];
  if (fieldStr == NULL) { /*if there's not value, bail*/
    return MS_FAILURE;
  }
  double fieldVal = atof(fieldStr); /*faith that it's ok -- */
  /*should switch to strtod*/
  return msValueToRange(style, fieldVal, MS_COLORSPACE_RGB);
}

/**
 * Allow direct mapping of a value so that mapscript can use the
 * Ranges.  The styls passed in is updated to reflect the right color
 * based on the fieldVal
 */
int msValueToRange(styleObj *style, double fieldVal, colorspace cs) {
  double range;
  double scaledVal;

  range = style->maxvalue - style->minvalue;
  scaledVal = (fieldVal - style->minvalue) / range;

  if (cs == MS_COLORSPACE_RGB) {
    /*At this point, we know where on the range we need to be*/
    /*However, we don't know how to map it yet, since RGB(A) can */
    /*Go up or down*/
    style->color.red = (int)(MS_MAX(
        0, (MS_MIN(255, (style->mincolor.red +
                         ((style->maxcolor.red - style->mincolor.red) *
                          scaledVal))))));
    style->color.green = (int)(MS_MAX(
        0, (MS_MIN(255, (style->mincolor.green +
                         ((style->maxcolor.green - style->mincolor.green) *
                          scaledVal))))));
    style->color.blue = (int)(MS_MAX(
        0, (MS_MIN(255, (style->mincolor.blue +
                         ((style->maxcolor.blue - style->mincolor.blue) *
                          scaledVal))))));
    style->color.alpha = (int)(MS_MAX(
        0, (MS_MIN(255, (style->mincolor.alpha +
                         ((style->maxcolor.alpha - style->mincolor.alpha) *
                          scaledVal))))));
  } else {
    /* HSL */
    assert(cs == MS_COLORSPACE_HSL);
    if (fieldVal <= style->minvalue)
      style->color = style->mincolor;
    else if (fieldVal >= style->maxvalue)
      style->color = style->maxcolor;
    else {
      double mh, ms, ml, Mh, Ms, Ml;
      msRGBtoHSL(&style->mincolor, &mh, &ms, &ml);
      msRGBtoHSL(&style->maxcolor, &Mh, &Ms, &Ml);
      mh += (Mh - mh) * scaledVal;
      ms += (Ms - ms) * scaledVal;
      ml += (Ml - ml) * scaledVal;
      msHSLtoRGB(mh, ms, ml, &style->color);
      style->color.alpha =
          style->mincolor.alpha +
          (style->maxcolor.alpha - style->mincolor.alpha) * scaledVal;
    }
  }
  /*( "msMapRange(): %i %i %i", style->color.red , style->color.green,
   * style->color.blue);*/

#if ALPHACOLOR_ENABLED
  /*NO ALPHA RANGE YET  style->color.alpha = style->mincolor.alpha +
   * ((style->maxcolor.alpha - style->mincolor.alpha) * scaledVal);*/
#endif

  return MS_SUCCESS;
}

/**********************************************************************
 * Project:  MapServer
 * Purpose:  Raster Label Layer
 * Author:   Even Rouault, <even.rouault@spatialys.com>
 *
 **********************************************************************
 * Copyright (c) 2024, Even Rouault, <even.rouault@spatialys.com>
 * Copyright (c) 2011, Alan Boudreault, MapGears
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

#include "mapserver.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include "mapows.h"
#include "mapresample.h"
#include "mapthread.h"

#include <cmath>
#include <limits>

#define MSRASTERLABEL_NUMITEMS 1
#define MSRASTERLABEL_VALUE "value"
#define MSRASTERLABEL_VALUEINDEX -100

typedef struct {

  /* query cache results */
  int query_results;

  int refcount;

  double *raster_values; /* raster values */
  int width;
  int height;
  rectObj extent;
  int next_shape;

  /* To improve performance of GetShape() when queried on increasing shapeindex
   */
  long last_queried_shapeindex; // value in [0, query_results[ range
  size_t last_raster_off;       // value in [0, width*height[ range

  mapObj
      *mapToUseForWhichShapes; /* set if the map->extent and map->projection are
                                  valid in msRasterLabelLayerWhichShapes() */

} RasterLabelLayerInfo;

void msRasterLabelLayerUseMapExtentAndProjectionForNextWhichShapes(
    layerObj *layer, mapObj *map) {
  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;
  rllinfo->mapToUseForWhichShapes = map;
}

static int msRasterLabelLayerInitItemInfo(layerObj *layer) {
  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;
  int i;
  int *itemindexes;
  int failed = 0;

  if (layer->numitems == 0)
    return MS_SUCCESS;

  if (rllinfo == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: GDAL layer not opened!!!",
               "msRasterLabelLayerInitItemInfo()");
    return (MS_FAILURE);
  }

  if (layer->iteminfo)
    free(layer->iteminfo);

  if ((layer->iteminfo = (int *)malloc(sizeof(int) * layer->numitems)) ==
      NULL) {
    msSetError(MS_MEMERR, NULL, "msRasterLabelLayerInitItemInfo()");
    return (MS_FAILURE);
  }

  itemindexes = (int *)layer->iteminfo;
  for (i = 0; i < layer->numitems; i++) {
    /* Special attribute names. */
    if (EQUAL(layer->items[i], MSRASTERLABEL_VALUE)) {
      itemindexes[i] = MSRASTERLABEL_VALUEINDEX;
    } else {
      itemindexes[i] = -1;
      msSetError(MS_MISCERR, "Invalid Field name: %s",
                 "msRasterLabelLayerInitItemInfo()", layer->items[i]);
      failed = 1;
    }
  }

  return failed ? (MS_FAILURE) : (MS_SUCCESS);
}

void msRasterLabelLayerFreeItemInfo(layerObj *layer) {
  if (layer->iteminfo)
    free(layer->iteminfo);
  layer->iteminfo = NULL;
}

static void msRasterLabelLayerInfoInitialize(layerObj *layer) {
  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;

  if (rllinfo != NULL)
    return;

  rllinfo =
      (RasterLabelLayerInfo *)msSmallCalloc(1, sizeof(RasterLabelLayerInfo));
  layer->layerinfo = rllinfo;

  rllinfo->raster_values = NULL;
  rllinfo->width = 0;
  rllinfo->height = 0;

  /* Set attribute type to Real, unless the user has explicitly set */
  /* something else. */
  {
    const char *const items[] = {
        MSRASTERLABEL_VALUE,
    };
    size_t i;
    for (i = 0; i < sizeof(items) / sizeof(items[0]); ++i) {
      char szTmp[100];
      snprintf(szTmp, sizeof(szTmp), "%s_type", items[i]);
      if (msOWSLookupMetadata(&(layer->metadata), "OFG", szTmp) == NULL) {
        snprintf(szTmp, sizeof(szTmp), "gml_%s_type", items[i]);
        msInsertHashTable(&(layer->metadata), szTmp, "Real");
      }
    }
  }
}

static void msRasterLabelLayerInfoFree(layerObj *layer)

{
  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;

  if (rllinfo == NULL)
    return;

  free(rllinfo->raster_values);

  free(rllinfo);

  layer->layerinfo = NULL;
}

int msRasterLabelLayerOpen(layerObj *layer) {
  /* If we don't have info, initialize an empty one now */
  if (layer->layerinfo == NULL)
    msRasterLabelLayerInfoInitialize(layer);
  if (layer->layerinfo == NULL)
    return MS_FAILURE;

  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;

  rllinfo->refcount = rllinfo->refcount + 1;

  return MS_SUCCESS;
}

int msRasterLabelLayerIsOpen(layerObj *layer) {
  if (layer->layerinfo)
    return MS_TRUE;
  return MS_FALSE;
}

int msRasterLabelLayerClose(layerObj *layer) {
  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;

  if (rllinfo != NULL) {
    rllinfo->refcount--;

    if (rllinfo->refcount < 1)
      msRasterLabelLayerInfoFree(layer);
  }
  return MS_SUCCESS;
}

int msRasterLabelLayerGetItems(layerObj *layer) {
  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;

  if (rllinfo == NULL)
    return MS_FAILURE;

  layer->numitems = 0;
  layer->items =
      (char **)msSmallCalloc(sizeof(char *), (MSRASTERLABEL_NUMITEMS + 1));

  layer->items[layer->numitems++] = msStrdup(MSRASTERLABEL_VALUE);
  layer->items[layer->numitems] = NULL;

  return msRasterLabelLayerInitItemInfo(layer);
}

/**********************************************************************
 *                     msRasterLabelGetValues()
 **********************************************************************/
static char **msRasterLabelGetValues(layerObj *layer, double value) {
  char **values;
  int i = 0;
  char tmp[100];
  int *itemindexes = (int *)layer->iteminfo;

  if (layer->numitems == 0)
    return (NULL);

  if (!layer->iteminfo) { /* Should not happen... but just in case! */
    if (msRasterLabelLayerInitItemInfo(layer) != MS_SUCCESS)
      return NULL;
    itemindexes = (int *)layer->iteminfo; /* reassign after malloc */
  }

  if ((values = (char **)malloc(sizeof(char *) * layer->numitems)) == NULL) {
    msSetError(MS_MEMERR, NULL, "msRasterLabelGetValues()");
    return (NULL);
  }

  for (i = 0; i < layer->numitems; i++) {
    if (itemindexes[i] == MSRASTERLABEL_VALUEINDEX) {
      snprintf(tmp, sizeof(tmp), "%f", value);
      values[i] = msStrdup(tmp);
    } else {
      values[i] = NULL;
    }
  }

  return values;
}

rectObj msRasterLabelGetSearchRect(layerObj *layer, mapObj *map) {
  rectObj searchrect = map->extent;
  int bDone = MS_FALSE;

  /* For RasterLabel, it is important that the searchrect is not too large */
  /* to avoid insufficient intermediate raster resolution, which could */
  /* happen if we use the default code path, given potential reprojection */
  /* issues when using a map extent that is not in the validity area of */
  /* the layer projection. */
  if (!layer->projection.gt.need_geotransform &&
      !(msProjIsGeographicCRS(&(map->projection)) &&
        msProjIsGeographicCRS(&(layer->projection)))) {
    rectObj layer_ori_extent;

    if (msLayerGetExtent(layer, &layer_ori_extent) == MS_SUCCESS) {
      projectionObj map_proj;

      double map_extent_minx = map->extent.minx;
      double map_extent_miny = map->extent.miny;
      double map_extent_maxx = map->extent.maxx;
      double map_extent_maxy = map->extent.maxy;
      rectObj layer_extent = layer_ori_extent;

      /* Create a variant of map->projection without geotransform for */
      /* conveniency */
      msInitProjection(&map_proj);
      msCopyProjection(&map_proj, &map->projection);
      map_proj.gt.need_geotransform = MS_FALSE;
      if (map->projection.gt.need_geotransform) {
        map_extent_minx =
            map->projection.gt.geotransform[0] +
            map->projection.gt.geotransform[1] * map->extent.minx +
            map->projection.gt.geotransform[2] * map->extent.miny;
        map_extent_miny =
            map->projection.gt.geotransform[3] +
            map->projection.gt.geotransform[4] * map->extent.minx +
            map->projection.gt.geotransform[5] * map->extent.miny;
        map_extent_maxx =
            map->projection.gt.geotransform[0] +
            map->projection.gt.geotransform[1] * map->extent.maxx +
            map->projection.gt.geotransform[2] * map->extent.maxy;
        map_extent_maxy =
            map->projection.gt.geotransform[3] +
            map->projection.gt.geotransform[4] * map->extent.maxx +
            map->projection.gt.geotransform[5] * map->extent.maxy;
      }

      /* Reproject layer extent to map projection */
      msProjectRect(&layer->projection, &map_proj, &layer_extent);

      if (layer_extent.minx <= map_extent_minx &&
          layer_extent.miny <= map_extent_miny &&
          layer_extent.maxx >= map_extent_maxx &&
          layer_extent.maxy >= map_extent_maxy) {
        /* do nothing special if area to map is inside layer extent */
      } else {
        if (layer_extent.minx >= map_extent_minx &&
            layer_extent.maxx <= map_extent_maxx &&
            layer_extent.miny >= map_extent_miny &&
            layer_extent.maxy <= map_extent_maxy) {
          /* if the area to map is larger than the layer extent, then */
          /* use full layer extent and add some margin to reflect the */
          /* proportion of the useful area over the requested bbox */
          double extra_x = (map_extent_maxx - map_extent_minx) /
                           (layer_extent.maxx - layer_extent.minx) *
                           (layer_ori_extent.maxx - layer_ori_extent.minx);
          double extra_y = (map_extent_maxy - map_extent_miny) /
                           (layer_extent.maxy - layer_extent.miny) *
                           (layer_ori_extent.maxy - layer_ori_extent.miny);
          searchrect.minx = layer_ori_extent.minx - extra_x / 2;
          searchrect.maxx = layer_ori_extent.maxx + extra_x / 2;
          searchrect.miny = layer_ori_extent.miny - extra_y / 2;
          searchrect.maxy = layer_ori_extent.maxy + extra_y / 2;
        } else {
          /* otherwise clip the map extent with the reprojected layer */
          /* extent */
          searchrect.minx = MS_MAX(map_extent_minx, layer_extent.minx);
          searchrect.maxx = MS_MIN(map_extent_maxx, layer_extent.maxx);
          searchrect.miny = MS_MAX(map_extent_miny, layer_extent.miny);
          searchrect.maxy = MS_MIN(map_extent_maxy, layer_extent.maxy);
          /* and reproject into the layer projection */
          msProjectRect(&map_proj, &layer->projection, &searchrect);
        }
        bDone = MS_TRUE;
      }

      msFreeProjection(&map_proj);
    }
  }

  if (!bDone)
    msProjectRect(&map->projection, &layer->projection,
                  &searchrect); /* project the searchrect to source coords */

  return searchrect;
}

static GDALDatasetH OpenRaster(layerObj *layer, std::string &osDecryptedPath) {
  GDALDatasetH hDS = NULL;
  char *decrypted_path;

  if (strncmp(layer->data, "<VRTDataset", strlen("<VRTDataset")) == 0) {
    decrypted_path = msStrdup(layer->data);
  } else {
    char szPath[MS_MAXPATHLEN];
    msTryBuildPath3(szPath, layer->map->mappath, layer->map->shapepath,
                    layer->data);
    decrypted_path = msDecryptStringTokens(layer->map, szPath);
  }

  if (decrypted_path) {
    char **connectionoptions;
    GDALAllRegister();
    connectionoptions =
        msGetStringListFromHashTable(&(layer->connectionoptions));
    hDS = GDALOpenEx(decrypted_path, GDAL_OF_RASTER, NULL,
                     (const char *const *)connectionoptions, NULL);
    CSLDestroy(connectionoptions);

    osDecryptedPath = decrypted_path;
    msFree(decrypted_path);
    if (!hDS) {
      msDebug(
          "msRasterLabelLayerWhichShapes(): cannot open DATA %s of layer %s\n",
          layer->data, layer->name);
      msSetError(MS_MISCERR, "Cannot open DATA of layer %s",
                 "msRasterLabelLayerWhichShapes()", layer->name);
    }
  }
  return hDS;
}

int msRasterLabelLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery) {
  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;
  imageObj *image_tmp;
  outputFormatObj *outputformat = NULL;
  mapObj *map_tmp;
  double map_cellsize;
  unsigned int spacing;
  int width, height;
  char **alteredProcessing = NULL, *saved_layer_mask;
  char **savedProcessing = NULL;
  int bHasLonWrap = MS_FALSE;
  double dfLonWrap = 0.0;
  rectObj oldLayerExtent;
  char *oldLayerData = NULL;
  projectionObj oldLayerProjection;
  int ret;

  memset(&oldLayerExtent, 0, sizeof(oldLayerExtent));
  memset(&oldLayerProjection, 0, sizeof(oldLayerProjection));

  if (layer->debug)
    msDebug("Entering msRasterLabelLayerWhichShapes().\n");

  if (rllinfo == NULL)
    return MS_FAILURE;

  std::string osDecryptedPath;
  GDALDatasetH hDS = OpenRaster(layer, osDecryptedPath);
  if (!hDS) {
    return MS_FAILURE;
  }

  if (CSLFetchNameValue(layer->processing, "BANDS") == NULL) {
    if (GDALGetRasterCount(hDS) == 1) {
      msLayerSetProcessingKey(layer, "BANDS", "1");
    } else {
      GDALClose(hDS);
      msSetError(MS_MISCERR,
                 "BANDS processing option is required for RASTER_LABEL layer. "
                 "You have to specify 1 band.",
                 "msRasterLabelLayerWhichShapes()");
      return MS_FAILURE;
    }
  }

  /*
  ** Allocate mapObj structure
  */
  map_tmp = (mapObj *)msSmallCalloc(sizeof(mapObj), 1);
  if (initMap(map_tmp) == -1) { /* initialize this map */
    msFree(map_tmp);
    GDALClose(hDS);
    return (MS_FAILURE);
  }

  /* Initialize our dummy map */
  MS_INIT_COLOR(map_tmp->imagecolor, 255, 255, 255, 255);
  map_tmp->resolution = layer->map->resolution;
  map_tmp->defresolution = layer->map->defresolution;

  outputformat = (outputFormatObj *)msSmallCalloc(1, sizeof(outputFormatObj));
  outputformat->bands = 1;
  outputformat->name = NULL;
  outputformat->driver = NULL;
  outputformat->refcount = 0;
  outputformat->vtable = NULL;
  outputformat->device = NULL;
  outputformat->renderer = MS_RENDER_WITH_RAWDATA;
  outputformat->imagemode = MS_IMAGEMODE_FLOAT64;
  msAppendOutputFormat(map_tmp, outputformat);

  msCopyHashTable(&map_tmp->configoptions, &layer->map->configoptions);
  map_tmp->mappath = msStrdup(layer->map->mappath);
  map_tmp->shapepath = msStrdup(layer->map->shapepath);
  map_tmp->gt.rotation_angle = 0.0;

  /* Custom msCopyProjection() that removes lon_wrap parameter */
  {
    int i;

    map_tmp->projection.numargs = 0;
    map_tmp->projection.gt = layer->projection.gt;

    for (i = 0; i < layer->projection.numargs; i++) {
      if (strncmp(layer->projection.args[i],
                  "lon_wrap=", strlen("lon_wrap=")) == 0) {
        bHasLonWrap = MS_TRUE;
        dfLonWrap = atof(layer->projection.args[i] + strlen("lon_wrap="));
      } else {
        map_tmp->projection.args[map_tmp->projection.numargs++] =
            msStrdup(layer->projection.args[i]);
      }
    }
    if (map_tmp->projection.numargs != 0) {
      msProcessProjection(&(map_tmp->projection));
    }

    map_tmp->projection.wellknownprojection =
        layer->projection.wellknownprojection;
  }

  /* Very special case to improve quality for rasters referenced from lon=0 to
   * 360 */
  /* We create a temporary VRT that swiches the 2 hemispheres, and then we */
  /* modify the georeferencing to be in the more standard [-180, 180] range */
  /* and we adjust the layer->data, extent and projection accordingly */
  if (layer->tileindex == NULL && rllinfo->mapToUseForWhichShapes &&
      bHasLonWrap && dfLonWrap == 180.0) {
    rectObj layerExtent;
    msLayerGetExtent(layer, &layerExtent);
    if (layerExtent.minx == 0 && layerExtent.maxx == 360) {
      int iBand;
      int nXSize = GDALGetRasterXSize(hDS);
      int nYSize = GDALGetRasterYSize(hDS);
      int nBands = GDALGetRasterCount(hDS);
      int nMaxLen = 100 + nBands * (800 + 2 * osDecryptedPath.size());
      int nOffset = 0;
      char *pszInlineVRT = static_cast<char *>(msSmallMalloc(nMaxLen));

      snprintf(pszInlineVRT, nMaxLen,
               "<VRTDataset rasterXSize=\"%d\" rasterYSize=\"%d\">", nXSize,
               nYSize);
      nOffset = strlen(pszInlineVRT);
      for (iBand = 1; iBand <= nBands; iBand++) {
        const char *pszDataType = "Byte";
        switch (GDALGetRasterDataType(GDALGetRasterBand(hDS, iBand))) {
        case GDT_Byte:
          pszDataType = "Byte";
          break;
        case GDT_Int16:
          pszDataType = "Int16";
          break;
        case GDT_UInt16:
          pszDataType = "UInt16";
          break;
        case GDT_Int32:
          pszDataType = "Int32";
          break;
        case GDT_UInt32:
          pszDataType = "UInt32";
          break;
        case GDT_Float32:
          pszDataType = "Float32";
          break;
        case GDT_Float64:
          pszDataType = "Float64";
          break;
        default:
          break;
        }

        snprintf(pszInlineVRT + nOffset, nMaxLen - nOffset,
                 "    <VRTRasterBand dataType=\"%s\" band=\"%d\">"
                 "        <SimpleSource>"
                 "            <SourceFilename "
                 "relativeToVrt=\"1\"><![CDATA[%s]]></SourceFilename>"
                 "            <SourceBand>%d</SourceBand>"
                 "            <SrcRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" "
                 "ySize=\"%d\"/>"
                 "            <DstRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" "
                 "ySize=\"%d\"/>"
                 "        </SimpleSource>"
                 "        <SimpleSource>"
                 "            <SourceFilename "
                 "relativeToVrt=\"1\"><![CDATA[%s]]></SourceFilename>"
                 "            <SourceBand>%d</SourceBand>"
                 "            <SrcRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" "
                 "ySize=\"%d\"/>"
                 "            <DstRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" "
                 "ySize=\"%d\"/>"
                 "        </SimpleSource>"
                 "    </VRTRasterBand>",
                 pszDataType, iBand, osDecryptedPath.c_str(), iBand, nXSize / 2,
                 0, nXSize - nXSize / 2, nYSize, 0, 0, nXSize - nXSize / 2,
                 nYSize, osDecryptedPath.c_str(), iBand, 0, 0, nXSize / 2,
                 nYSize, nXSize - nXSize / 2, 0, nXSize / 2, nYSize);

        nOffset += strlen(pszInlineVRT + nOffset);
      }
      snprintf(pszInlineVRT + nOffset, nMaxLen - nOffset, "</VRTDataset>");

      oldLayerExtent = layer->extent;
      oldLayerData = layer->data;
      oldLayerProjection = layer->projection;
      layer->extent.minx = -180;
      layer->extent.maxx = 180;
      layer->data = pszInlineVRT;
      layer->projection = map_tmp->projection;

      /* map_tmp->projection is actually layer->projection without lon_wrap */
      rect = rllinfo->mapToUseForWhichShapes->extent;
      msProjectRect(&rllinfo->mapToUseForWhichShapes->projection,
                    &map_tmp->projection, &rect);
      bHasLonWrap = MS_FALSE;
    }
  }

  if (isQuery) {
    /* For query mode, use layer->map->extent reprojected rather than */
    /* the provided rect. Generic query code will filter returned features. */
    rect = msRasterLabelGetSearchRect(layer, layer->map);
  }

  /* -------------------------------------------------------------------- */
  /*      Determine desired spacing.  Default to 32 if not otherwise set  */
  /* -------------------------------------------------------------------- */
  spacing = 32;
  if (CSLFetchNameValue(layer->processing, "LABEL_SPACING") != NULL) {
    spacing = atoi(CSLFetchNameValue(layer->processing, "LABEL_SPACING"));
    if (spacing == 0)
      spacing = 32;
  }

  width = (int)(layer->map->width / spacing);
  height = (int)(layer->map->height / spacing);

  map_cellsize = MS_MAX(MS_CELLSIZE(rect.minx, rect.maxx, layer->map->width),
                        MS_CELLSIZE(rect.miny, rect.maxy, layer->map->height));
  map_tmp->cellsize = map_cellsize * spacing;

  // By default, do not oversample beyond the resolution of the layer
  if (!CPLTestBoolean(
          CSLFetchNameValueDef(layer->processing, "ALLOW_OVERSAMPLE", "NO"))) {
    double adfGT[6];
    if (GDALGetGeoTransform(hDS, adfGT) == CE_None) {
      if (!msProjectionsDiffer(&(layer->map->projection),
                               &(layer->projection)) &&
          map_tmp->cellsize < adfGT[1]) {
        map_tmp->cellsize = adfGT[1];
        width = MS_MAX(2, (int)(rect.maxx - rect.minx) / map_tmp->cellsize);
        height = MS_MAX(2, (int)(rect.maxy - rect.miny) / map_tmp->cellsize);
        map_tmp->cellsize = MS_MAX(MS_CELLSIZE(rect.minx, rect.maxx, width),
                                   MS_CELLSIZE(rect.miny, rect.maxy, height));
      }
    }
  }

  map_tmp->extent.minx =
      rect.minx - (0.5 * map_cellsize) + (0.5 * map_tmp->cellsize);
  map_tmp->extent.miny =
      rect.miny - (0.5 * map_cellsize) + (0.5 * map_tmp->cellsize);
  map_tmp->extent.maxx =
      map_tmp->extent.minx + ((width - 1) * map_tmp->cellsize);
  map_tmp->extent.maxy =
      map_tmp->extent.miny + ((height - 1) * map_tmp->cellsize);

  GDALClose(hDS);

  if (bHasLonWrap && dfLonWrap == 180.0) {
    if (map_tmp->extent.minx >= 180) {
      /* Request on the right half of the shifted raster (= western hemisphere)
       */
      map_tmp->extent.minx -= 360;
      map_tmp->extent.maxx -= 360;
    } else if (map_tmp->extent.maxx >= 180.0) {
      /* Request spanning on the 2 hemispheres => drawing whole planet */
      /* Take only into account vertical resolution, as horizontal one */
      /* will be unreliable (assuming square pixels...) */
      map_cellsize = MS_CELLSIZE(rect.miny, rect.maxy, layer->map->height);
      map_tmp->cellsize = map_cellsize * spacing;

      width = 360.0 / map_tmp->cellsize;
      map_tmp->extent.minx = -180.0 + (0.5 * map_tmp->cellsize);
      map_tmp->extent.maxx = 180.0 - (0.5 * map_tmp->cellsize);
    }
  }

  if (layer->debug)
    msDebug("msRasterLabelLayerWhichShapes(): width: %d, height: %d, cellsize: "
            "%g\n",
            width, height, map_tmp->cellsize);

  if (layer->debug == MS_DEBUGLEVEL_VVV)
    msDebug("msRasterLabelLayerWhichShapes(): extent: %g %g %g %g\n",
            map_tmp->extent.minx, map_tmp->extent.miny, map_tmp->extent.maxx,
            map_tmp->extent.maxy);

  /* important to use that function, to compute map
     geotransform, used by the resampling*/
  msMapSetSize(map_tmp, width, height);

  if (layer->debug == MS_DEBUGLEVEL_VVV)
    msDebug(
        "msRasterLabelLayerWhichShapes(): geotransform: %g %g %g %g %g %g\n",
        map_tmp->gt.geotransform[0], map_tmp->gt.geotransform[1],
        map_tmp->gt.geotransform[2], map_tmp->gt.geotransform[3],
        map_tmp->gt.geotransform[4], map_tmp->gt.geotransform[5]);

  rllinfo->extent = map_tmp->extent;

  image_tmp = msImageCreate(width, height, map_tmp->outputformatlist[0], NULL,
                            NULL, map_tmp->resolution, map_tmp->defresolution,
                            &(map_tmp->imagecolor));

  /* Default set to AVERAGE resampling */
  if (CSLFetchNameValue(layer->processing, "RESAMPLE") == NULL) {
    alteredProcessing = CSLDuplicate(layer->processing);
    alteredProcessing =
        CSLSetNameValue(alteredProcessing, "RESAMPLE", "AVERAGE");
    savedProcessing = layer->processing;
    layer->processing = alteredProcessing;
  }

  /* disable masking at this level: we don't want to apply the mask at the
   * raster level, it will be applied with the correct cellsize and image size
   * in the vector rendering phase.
   */
  saved_layer_mask = layer->mask;
  layer->mask = NULL;
  ret = msDrawRasterLayerLow(map_tmp, layer, image_tmp, NULL);

  /* restore layer attributes if we went through the above on-the-fly VRT */
  if (oldLayerData) {
    msFree(layer->data);
    layer->data = oldLayerData;
    layer->extent = oldLayerExtent;
    layer->projection = oldLayerProjection;
  }

  /* restore layer mask */
  layer->mask = saved_layer_mask;

  /* restore the saved processing */
  if (alteredProcessing != NULL) {
    layer->processing = savedProcessing;
    CSLDestroy(alteredProcessing);
  }

  if (ret == MS_FAILURE) {
    msSetError(MS_MISCERR, "Unable to draw raster data.",
               "msRasterLabelLayerWhichShapes()");

    msFreeMap(map_tmp);
    msFreeImage(image_tmp);

    return MS_FAILURE;
  }

  /* Update our rl layer structure */
  rllinfo->width = width;
  rllinfo->height = height;
  rllinfo->query_results = 0;

  rllinfo->last_queried_shapeindex = 0;
  rllinfo->last_raster_off = 0;

  free(rllinfo->raster_values);
  rllinfo->raster_values =
      (double *)msSmallMalloc(sizeof(double) * width * height);

  for (size_t off = 0; off < static_cast<size_t>(width) * height; ++off) {
    if (MS_GET_BIT(image_tmp->img_mask, off)) {
      rllinfo->raster_values[off] = image_tmp->img.raw_double[off];
      rllinfo->query_results++;
    } else
      rllinfo->raster_values[off] = std::numeric_limits<double>::quiet_NaN();
  }

  msFreeImage(image_tmp); /* we do not need the imageObj anymore */
  msFreeMap(map_tmp);

  rllinfo->next_shape = 0;

  return MS_SUCCESS;
}

int msRasterLabelLayerGetShape(layerObj *layer, shapeObj *shape,
                               resultObj *record) {
  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;
  lineObj line;
  pointObj point;
  const long shapeindex = record->shapeindex;

  msFreeShape(shape);
  shape->type = MS_SHAPE_NULL;

  if (shapeindex < 0 || shapeindex >= rllinfo->query_results) {
    msSetError(MS_MISCERR,
               "Out of range shape index requested.  Requested %ld\n"
               "but only %d shapes available.",
               "msRasterLabelLayerGetShape()", shapeindex,
               rllinfo->query_results);
    return MS_FAILURE;
  }

  /* loop to the next valid value */
  size_t raster_off = (shapeindex >= rllinfo->last_queried_shapeindex)
                          ? rllinfo->last_raster_off
                          : 0;
  for (long curshapeindex = (shapeindex >= rllinfo->last_queried_shapeindex)
                                ? rllinfo->last_queried_shapeindex
                                : 0;
       raster_off < static_cast<size_t>(rllinfo->width) * rllinfo->height;
       ++raster_off) {
    if (!std::isnan(rllinfo->raster_values[raster_off])) {
      if (curshapeindex == shapeindex) {
        rllinfo->last_queried_shapeindex = shapeindex;
        rllinfo->last_raster_off = raster_off;
        break;
      }
      ++curshapeindex;
    }
  }
  assert(raster_off < static_cast<size_t>(rllinfo->width) * rllinfo->height);

  const int x = static_cast<int>(raster_off % rllinfo->width);
  const int y = static_cast<int>(raster_off / rllinfo->width);
  point.x = Pix2Georef(x, 0, rllinfo->width - 1, rllinfo->extent.minx,
                       rllinfo->extent.maxx, MS_FALSE);
  point.y = Pix2Georef(y, 0, rllinfo->height - 1, rllinfo->extent.miny,
                       rllinfo->extent.maxy, MS_TRUE);
  if (layer->debug == MS_DEBUGLEVEL_VVV)
    msDebug("msRasterLabelLayerWhichShapes(): shapeindex: %ld, x: %g, y: %g\n",
            shapeindex, point.x, point.y);

  point.m = 0.0;

  shape->type = MS_SHAPE_POINT;
  line.numpoints = 1;
  line.point = &point;
  msAddLine(shape, &line);
  msComputeBounds(shape);

  shape->numvalues = layer->numitems;
  shape->values =
      msRasterLabelGetValues(layer, rllinfo->raster_values[raster_off]);
  shape->index = shapeindex;
  shape->resultindex = shapeindex;

  return MS_SUCCESS;
}

int msRasterLabelLayerNextShape(layerObj *layer, shapeObj *shape) {
  RasterLabelLayerInfo *rllinfo = (RasterLabelLayerInfo *)layer->layerinfo;

  if (rllinfo->next_shape < 0 ||
      rllinfo->next_shape >= rllinfo->query_results) {
    msFreeShape(shape);
    shape->type = MS_SHAPE_NULL;
    return MS_DONE;
  } else {
    resultObj record;

    record.shapeindex = rllinfo->next_shape++;
    record.tileindex = 0;
    record.classindex = record.resultindex = -1;

    return msRasterLabelLayerGetShape(layer, shape, &record);
  }
}

/************************************************************************/
/*                       msRasterLabelLayerGetExtent()                     */
/* Simple copy of the maprasterquery.c file. might change in the future */
/************************************************************************/

int msRasterLabelLayerGetExtent(layerObj *layer, rectObj *extent)

{
  char szPath[MS_MAXPATHLEN];
  mapObj *map = layer->map;
  shapefileObj *tileshpfile;

  if ((!layer->data || strlen(layer->data) == 0) && layer->tileindex == NULL) {
    /* should we be issuing a specific error about not supporting
       extents for tileindexed raster layers? */
    return MS_FAILURE;
  }

  if (map == NULL)
    return MS_FAILURE;

  /* If the layer use a tileindex, return the extent of the tileindex
   * shapefile/referenced layer */
  if (layer->tileindex) {
    const int tilelayerindex = msGetLayerIndex(map, layer->tileindex);
    if (tilelayerindex != -1) /* does the tileindex reference another layer */
      return msLayerGetExtent(GET_LAYER(map, tilelayerindex), extent);
    else {
      tileshpfile = (shapefileObj *)malloc(sizeof(shapefileObj));
      MS_CHECK_ALLOC(tileshpfile, sizeof(shapefileObj), MS_FAILURE);

      if (msShapefileOpen(tileshpfile, "rb",
                          msBuildPath3(szPath, map->mappath, map->shapepath,
                                       layer->tileindex),
                          MS_TRUE) == -1)
        if (msShapefileOpen(tileshpfile, "rb",
                            msBuildPath(szPath, map->mappath, layer->tileindex),
                            MS_TRUE) == -1)
          return MS_FAILURE;

      *extent = tileshpfile->bounds;
      msShapefileClose(tileshpfile);
      free(tileshpfile);
      return MS_SUCCESS;
    }
  }

  msTryBuildPath3(szPath, map->mappath, map->shapepath, layer->data);
  char *decrypted_path = msDecryptStringTokens(map, szPath);
  if (!decrypted_path)
    return MS_FAILURE;

  GDALAllRegister();

  char **connectionoptions =
      msGetStringListFromHashTable(&(layer->connectionoptions));
  GDALDatasetH hDS = GDALOpenEx(decrypted_path, GDAL_OF_RASTER, NULL,
                                (const char *const *)connectionoptions, NULL);
  CSLDestroy(connectionoptions);
  msFree(decrypted_path);
  if (hDS == NULL) {
    return MS_FAILURE;
  }

  const int nXSize = GDALGetRasterXSize(hDS);
  const int nYSize = GDALGetRasterYSize(hDS);
  double adfGeoTransform[6] = {0};
  const CPLErr eErr = GDALGetGeoTransform(hDS, adfGeoTransform);
  if (eErr != CE_None) {
    GDALClose(hDS);
    return MS_FAILURE;
  }

  /* If this appears to be an ungeoreferenced raster than flip it for
     mapservers purposes. */
  if (adfGeoTransform[5] == 1.0 && adfGeoTransform[3] == 0.0) {
    adfGeoTransform[5] = -1.0;
    adfGeoTransform[3] = nYSize;
  }

  extent->minx = adfGeoTransform[0];
  extent->maxy = adfGeoTransform[3];

  extent->maxx = adfGeoTransform[0] + nXSize * adfGeoTransform[1];
  extent->miny = adfGeoTransform[3] + nYSize * adfGeoTransform[5];

  return MS_SUCCESS;
}

/************************************************************************/
/*                  msRasterLabelLayerSetTimeFilter()                   */
/*                                                                      */
/*      This function is actually just used in the context of           */
/*      setting a filter on the tileindex for time based queries.       */
/*      For instance via WMS requests.  So it isn't really related      */
/*      to the "raster query" support at all.                           */
/*                                                                      */
/*      If a local shapefile tileindex is in use, we will set a         */
/*      backtics filter (shapefile compatible).  If another layer is    */
/*      being used as the tileindex then we will forward the            */
/*      SetTimeFilter call to it.  If there is no tileindex in          */
/*      place, we do nothing.                                           */
/************************************************************************/

int msRasterLabelLayerSetTimeFilter(layerObj *layer, const char *timestring,
                                    const char *timefield) {
  int tilelayerindex;

  /* -------------------------------------------------------------------- */
  /*      If we don't have a tileindex the time filter has no effect.     */
  /* -------------------------------------------------------------------- */
  if (layer->tileindex == NULL)
    return MS_SUCCESS;

  /* -------------------------------------------------------------------- */
  /*      Find the tileindex layer.                                       */
  /* -------------------------------------------------------------------- */
  tilelayerindex = msGetLayerIndex(layer->map, layer->tileindex);

  /* -------------------------------------------------------------------- */
  /*      If we are using a local shapefile as our tileindex (that is     */
  /*      to say, the tileindex name is not of another layer), then we    */
  /*      just install a backtics style filter on the raster layer.       */
  /*      This is propagated to the "working layer" created for the       */
  /*      tileindex by code in mapraster.c.                               */
  /* -------------------------------------------------------------------- */
  if (tilelayerindex == -1)
    return msLayerMakeBackticsTimeFilter(layer, timestring, timefield);

  /* -------------------------------------------------------------------- */
  /*      Otherwise we invoke the tileindex layers SetTimeFilter          */
  /*      method.                                                         */
  /* -------------------------------------------------------------------- */
  if (msCheckParentPointer(layer->map, "map") == MS_FAILURE)
    return MS_FAILURE;
  return msLayerSetTimeFilter(layer->GET_LAYER(map, tilelayerindex), timestring,
                              timefield);
}

/************************************************************************/
/*                msRASTERLayerInitializeVirtualTable()                 */
/************************************************************************/

int msRasterLabelLayerInitializeVirtualTable(layerObj *layer) {
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerInitItemInfo = msRasterLabelLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msRasterLabelLayerFreeItemInfo;
  layer->vtable->LayerOpen = msRasterLabelLayerOpen;
  layer->vtable->LayerIsOpen = msRasterLabelLayerIsOpen;
  layer->vtable->LayerWhichShapes = msRasterLabelLayerWhichShapes;
  layer->vtable->LayerNextShape = msRasterLabelLayerNextShape;
  layer->vtable->LayerGetShape = msRasterLabelLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */
  layer->vtable->LayerClose = msRasterLabelLayerClose;
  layer->vtable->LayerGetItems = msRasterLabelLayerGetItems;
  layer->vtable->LayerGetExtent = msRasterLabelLayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  /* layer->vtable->LayerCloseConnection = msRasterLabelLayerClose; */
  /* we use backtics for proper tileindex shapefile functioning */
  layer->vtable->LayerSetTimeFilter = msRasterLabelLayerSetTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */

  return MS_SUCCESS;
}

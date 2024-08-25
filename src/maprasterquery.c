/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of query operations on rasters.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
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
 ****************************************************************************/

#include <assert.h>
#include "mapserver.h"
#include "mapresample.h"
#include "mapthread.h"
#include "mapraster.h"

int msRASTERLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record);
int msRASTERLayerGetItems(layerObj *layer);

/* ==================================================================== */
/*      For now the rasterLayerInfo lives here since it is just used    */
/*      to hold information related to queries.                         */
/* ==================================================================== */
typedef struct {

  /* query cache results */
  int query_results;
  int query_alloc_max;
  int query_request_max;
  int query_result_hard_max;
  int raster_query_mode;
  int band_count;

  int refcount;

  rectObj which_rect;
  int next_shape;

  double *qc_x;
  double *qc_y;
  double *qc_x_reproj;
  double *qc_y_reproj;
  float *qc_values;
  int *qc_class;
  int *qc_red;
  int *qc_green;
  int *qc_blue;
  int *qc_count;
  int *qc_tileindex;

  /* query bound in force */
  shapeObj *searchshape;

  /* Only nearest result to this point. */
  int range_mode; /* MS_QUERY_SINGLE, MS_QUERY_MULTIPLE or -1 (skip test) */
  double range_dist;
  pointObj target_point;

  GDALColorTableH hCT;

  double shape_tolerance;

} rasterLayerInfo;

#define RQM_UNKNOWN 0
#define RQM_ENTRY_PER_PIXEL 1
#define RQM_HIST_ON_CLASS 2
#define RQM_HIST_ON_VALUE 3

extern int InvGeoTransform(double *gt_in, double *gt_out);
#define GEO_TRANS(tr, x, y) ((tr)[0] + (tr)[1] * (x) + (tr)[2] * (y))

/************************************************************************/
/*                             addResult()                              */
/*                                                                      */
/*      this is a copy of the code in mapquery.c.  Should we prepare    */
/*      a small public API for managing results caches?                 */
/************************************************************************/

static int addResult(resultCacheObj *cache, int classindex, int shapeindex,
                     int tileindex) {
  int i;

  if (cache->numresults == cache->cachesize) { /* just add it to the end */
    if (cache->cachesize == 0)
      cache->results =
          (resultObj *)malloc(sizeof(resultObj) * MS_RESULTCACHEINCREMENT);
    else
      cache->results = (resultObj *)realloc(
          cache->results,
          sizeof(resultObj) * (cache->cachesize + MS_RESULTCACHEINCREMENT));
    if (!cache->results) {
      msSetError(MS_MEMERR, "Realloc() error.", "addResult()");
      return (MS_FAILURE);
    }
    cache->cachesize += MS_RESULTCACHEINCREMENT;
  }

  i = cache->numresults;

  cache->results[i].classindex = classindex;
  cache->results[i].tileindex = tileindex;
  cache->results[i].shapeindex = shapeindex;
  cache->results[i].resultindex = -1; /* unused */
  cache->results[i].shape = NULL;
  cache->numresults++;

  return (MS_SUCCESS);
}

/************************************************************************/
/*                       msRasterLayerInfoFree()                        */
/************************************************************************/

static void msRasterLayerInfoFree(layerObj *layer)

{
  rasterLayerInfo *rlinfo = (rasterLayerInfo *)layer->layerinfo;

  if (rlinfo == NULL)
    return;

  if (rlinfo->qc_x != NULL) {
    free(rlinfo->qc_x);
    free(rlinfo->qc_y);
    free(rlinfo->qc_x_reproj);
    free(rlinfo->qc_y_reproj);
  }

  if (rlinfo->qc_values)
    free(rlinfo->qc_values);

  if (rlinfo->qc_class) {
    free(rlinfo->qc_class);
  }

  if (rlinfo->qc_red) {
    free(rlinfo->qc_red);
    free(rlinfo->qc_green);
    free(rlinfo->qc_blue);
  }

  if (rlinfo->qc_count != NULL)
    free(rlinfo->qc_count);

  if (rlinfo->qc_tileindex != NULL)
    free(rlinfo->qc_tileindex);

  free(rlinfo);

  layer->layerinfo = NULL;
}

/************************************************************************/
/*                    msRasterLayerInfoInitialize()                     */
/************************************************************************/

static void msRasterLayerInfoInitialize(layerObj *layer)

{
  rasterLayerInfo *rlinfo = (rasterLayerInfo *)layer->layerinfo;

  if (rlinfo != NULL)
    return;

  rlinfo = (rasterLayerInfo *)msSmallCalloc(1, sizeof(rasterLayerInfo));
  layer->layerinfo = rlinfo;

  rlinfo->band_count = -1;
  rlinfo->raster_query_mode = RQM_ENTRY_PER_PIXEL;
  rlinfo->range_mode = -1; /* inactive */
  rlinfo->refcount = 0;
  rlinfo->shape_tolerance = 0.0;

  /* We need to do this or the layer->layerinfo will be interpreted */
  /* as shapefile access info because the default connectiontype is */
  /* MS_SHAPEFILE. */
  if (layer->connectiontype != MS_WMS &&
      layer->connectiontype != MS_KERNELDENSITY)
    layer->connectiontype = MS_RASTER;

  rlinfo->query_result_hard_max = 1000000;

  if (CSLFetchNameValue(layer->processing, "RASTER_QUERY_MAX_RESULT") != NULL) {
    rlinfo->query_result_hard_max =
        atoi(CSLFetchNameValue(layer->processing, "RASTER_QUERY_MAX_RESULT"));
  }
}

/************************************************************************/
/*                       msRasterQueryAddPixel()                        */
/************************************************************************/

static void msRasterQueryAddPixel(layerObj *layer, pointObj *location,
                                  pointObj *reprojectedLocation, float *values)

{
  rasterLayerInfo *rlinfo = (rasterLayerInfo *)layer->layerinfo;
  int red = 0, green = 0, blue = 0, nodata = FALSE;
  int p_class = -1;

  if (rlinfo->query_results == rlinfo->query_result_hard_max)
    return;

  /* -------------------------------------------------------------------- */
  /*      Is this our first time in?  If so, do an initial allocation     */
  /*      for the data arrays suitable to our purposes.                   */
  /* -------------------------------------------------------------------- */
  if (rlinfo->query_alloc_max == 0) {
    rlinfo->query_alloc_max = 2;

    switch (rlinfo->raster_query_mode) {
    case RQM_ENTRY_PER_PIXEL:
      rlinfo->qc_x =
          (double *)msSmallCalloc(sizeof(double), rlinfo->query_alloc_max);
      rlinfo->qc_y =
          (double *)msSmallCalloc(sizeof(double), rlinfo->query_alloc_max);
      rlinfo->qc_x_reproj =
          (double *)msSmallCalloc(sizeof(double), rlinfo->query_alloc_max);
      rlinfo->qc_y_reproj =
          (double *)msSmallCalloc(sizeof(double), rlinfo->query_alloc_max);
      rlinfo->qc_values = (float *)msSmallCalloc(
          sizeof(float),
          ((size_t)rlinfo->query_alloc_max) * rlinfo->band_count);
      rlinfo->qc_red =
          (int *)msSmallCalloc(sizeof(int), rlinfo->query_alloc_max);
      rlinfo->qc_green =
          (int *)msSmallCalloc(sizeof(int), rlinfo->query_alloc_max);
      rlinfo->qc_blue =
          (int *)msSmallCalloc(sizeof(int), rlinfo->query_alloc_max);
      if (layer->numclasses > 0)
        rlinfo->qc_class =
            (int *)msSmallCalloc(sizeof(int), rlinfo->query_alloc_max);
      break;

    case RQM_HIST_ON_CLASS:
      break;

    case RQM_HIST_ON_VALUE:
      break;

    default:
      assert(FALSE);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Reallocate the data arrays larger if they are near the max      */
  /*      now.                                                            */
  /* -------------------------------------------------------------------- */
  if (rlinfo->query_results == rlinfo->query_alloc_max) {
    rlinfo->query_alloc_max = rlinfo->query_alloc_max * 2 + 100;

    if (rlinfo->qc_x != NULL)
      rlinfo->qc_x = msSmallRealloc(rlinfo->qc_x,
                                    sizeof(double) * rlinfo->query_alloc_max);
    if (rlinfo->qc_y != NULL)
      rlinfo->qc_y = msSmallRealloc(rlinfo->qc_y,
                                    sizeof(double) * rlinfo->query_alloc_max);
    if (rlinfo->qc_x_reproj != NULL)
      rlinfo->qc_x_reproj = msSmallRealloc(
          rlinfo->qc_x_reproj, sizeof(double) * rlinfo->query_alloc_max);
    if (rlinfo->qc_y_reproj != NULL)
      rlinfo->qc_y_reproj = msSmallRealloc(
          rlinfo->qc_y_reproj, sizeof(double) * rlinfo->query_alloc_max);
    if (rlinfo->qc_values != NULL)
      rlinfo->qc_values = msSmallRealloc(
          rlinfo->qc_values,
          sizeof(float) * rlinfo->query_alloc_max * rlinfo->band_count);
    if (rlinfo->qc_class != NULL)
      rlinfo->qc_class = msSmallRealloc(rlinfo->qc_class,
                                        sizeof(int) * rlinfo->query_alloc_max);
    if (rlinfo->qc_red != NULL)
      rlinfo->qc_red =
          msSmallRealloc(rlinfo->qc_red, sizeof(int) * rlinfo->query_alloc_max);
    if (rlinfo->qc_green != NULL)
      rlinfo->qc_green = msSmallRealloc(rlinfo->qc_green,
                                        sizeof(int) * rlinfo->query_alloc_max);
    if (rlinfo->qc_blue != NULL)
      rlinfo->qc_blue = msSmallRealloc(rlinfo->qc_blue,
                                       sizeof(int) * rlinfo->query_alloc_max);
    if (rlinfo->qc_count != NULL)
      rlinfo->qc_count = msSmallRealloc(rlinfo->qc_count,
                                        sizeof(int) * rlinfo->query_alloc_max);
    if (rlinfo->qc_tileindex != NULL)
      rlinfo->qc_tileindex = msSmallRealloc(
          rlinfo->qc_tileindex, sizeof(int) * rlinfo->query_alloc_max);
  }

  /* -------------------------------------------------------------------- */
  /*      Handle colormap                                                 */
  /* -------------------------------------------------------------------- */
  if (rlinfo->hCT != NULL) {
    int pct_index = (int)floor(values[0]);
    GDALColorEntry sEntry;

    if (GDALGetColorEntryAsRGB(rlinfo->hCT, pct_index, &sEntry)) {
      red = sEntry.c1;
      green = sEntry.c2;
      blue = sEntry.c3;

      if (sEntry.c4 == 0)
        nodata = TRUE;
    } else
      nodata = TRUE;
  }

  /* -------------------------------------------------------------------- */
  /*      Color derived from greyscale value.                             */
  /* -------------------------------------------------------------------- */
  else {
    if (rlinfo->band_count >= 3) {
      red = (int)MS_MAX(0, MS_MIN(255, values[0]));
      green = (int)MS_MAX(0, MS_MIN(255, values[1]));
      blue = (int)MS_MAX(0, MS_MIN(255, values[2]));
    } else {
      red = green = blue = (int)MS_MAX(0, MS_MIN(255, values[0]));
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Handle classification.                                          */
  /*                                                                      */
  /*      NOTE: The following is really quite inadequate to deal with     */
  /*      classifications based on [red], [green] and [blue] as           */
  /*      described in:                                                   */
  /*       http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=1021         */
  /* -------------------------------------------------------------------- */
  if (rlinfo->qc_class != NULL) {
    p_class = msGetClass_FloatRGB(layer, values[0], red, green, blue);

    if (p_class == -1)
      nodata = TRUE;
    else {
      nodata = FALSE;
      rlinfo->qc_class[rlinfo->query_results] = p_class;
      if (layer->class[p_class] -> numstyles > 0) {
        red = layer->class[p_class]->styles[0]->color.red;
        green = layer->class[p_class]->styles[0]->color.green;
        blue = layer->class[p_class]->styles[0]->color.blue;
      } else {
        red = green = blue = 0;
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Record the color.                                               */
  /* -------------------------------------------------------------------- */
  rlinfo->qc_red[rlinfo->query_results] = red;
  rlinfo->qc_green[rlinfo->query_results] = green;
  rlinfo->qc_blue[rlinfo->query_results] = blue;

  /* -------------------------------------------------------------------- */
  /*      Record spatial location.                                        */
  /* -------------------------------------------------------------------- */
  if (rlinfo->qc_x != NULL) {
    rlinfo->qc_x[rlinfo->query_results] = location->x;
    rlinfo->qc_y[rlinfo->query_results] = location->y;
    rlinfo->qc_x_reproj[rlinfo->query_results] = reprojectedLocation->x;
    rlinfo->qc_y_reproj[rlinfo->query_results] = reprojectedLocation->y;
  }

  /* -------------------------------------------------------------------- */
  /*      Record actual pixel value(s).                                   */
  /* -------------------------------------------------------------------- */
  if (rlinfo->qc_values != NULL)
    memcpy(rlinfo->qc_values + rlinfo->query_results * rlinfo->band_count,
           values, sizeof(float) * rlinfo->band_count);

  /* -------------------------------------------------------------------- */
  /*      Add to the results cache.                                       */
  /* -------------------------------------------------------------------- */
  if (!nodata) {
    addResult(layer->resultcache, p_class, rlinfo->query_results, 0);
    rlinfo->query_results++;
  }
}

/************************************************************************/
/*                       msRasterQueryByRectLow()                       */
/************************************************************************/

static int msRasterQueryByRectLow(mapObj *map, layerObj *layer,
                                  GDALDatasetH hDS, rectObj queryRect)

{
  double adfGeoTransform[6], adfInvGeoTransform[6];
  double dfXMin, dfYMin, dfXMax, dfYMax, dfX, dfY, dfAdjustedRange;
  int nWinXOff, nWinYOff, nWinXSize, nWinYSize;
  int nRXSize, nRYSize;
  float *pafRaster;
  int nBandCount, *panBandMap, iPixel, iLine;
  CPLErr eErr;
  rasterLayerInfo *rlinfo;
  rectObj searchrect;

  rlinfo = (rasterLayerInfo *)layer->layerinfo;

  /* -------------------------------------------------------------------- */
  /*      Reproject the search rect into the projection of this           */
  /*      layer/file if needed.                                           */
  /* -------------------------------------------------------------------- */
  searchrect = queryRect;
  layer->project =
      msProjectionsDiffer(&(layer->projection), &(map->projection));
  if (layer->project)
    msProjectRect(&(map->projection), &(layer->projection), &searchrect);

  /* -------------------------------------------------------------------- */
  /*      Transform the rectangle in target ground coordinates to         */
  /*      pixel/line extents on the file.  Process all 4 corners, to      */
  /*      build extents.                                                  */
  /* -------------------------------------------------------------------- */
  nRXSize = GDALGetRasterXSize(hDS);
  nRYSize = GDALGetRasterYSize(hDS);

  msGetGDALGeoTransform(hDS, map, layer, adfGeoTransform);
  InvGeoTransform(adfGeoTransform, adfInvGeoTransform);

  /* top left */
  dfXMin = dfXMax =
      GEO_TRANS(adfInvGeoTransform, searchrect.minx, searchrect.maxy);
  dfYMin = dfYMax =
      GEO_TRANS(adfInvGeoTransform + 3, searchrect.minx, searchrect.maxy);

  /* top right */
  dfX = GEO_TRANS(adfInvGeoTransform, searchrect.maxx, searchrect.maxy);
  dfY = GEO_TRANS(adfInvGeoTransform + 3, searchrect.maxx, searchrect.maxy);
  dfXMin = MS_MIN(dfXMin, dfX);
  dfXMax = MS_MAX(dfXMax, dfX);
  dfYMin = MS_MIN(dfYMin, dfY);
  dfYMax = MS_MAX(dfYMax, dfY);

  /* bottom left */
  dfX = GEO_TRANS(adfInvGeoTransform, searchrect.minx, searchrect.miny);
  dfY = GEO_TRANS(adfInvGeoTransform + 3, searchrect.minx, searchrect.miny);
  dfXMin = MS_MIN(dfXMin, dfX);
  dfXMax = MS_MAX(dfXMax, dfX);
  dfYMin = MS_MIN(dfYMin, dfY);
  dfYMax = MS_MAX(dfYMax, dfY);

  /* bottom right */
  dfX = GEO_TRANS(adfInvGeoTransform, searchrect.maxx, searchrect.miny);
  dfY = GEO_TRANS(adfInvGeoTransform + 3, searchrect.maxx, searchrect.miny);
  dfXMin = MS_MIN(dfXMin, dfX);
  dfXMax = MS_MAX(dfXMax, dfX);
  dfYMin = MS_MIN(dfYMin, dfY);
  dfYMax = MS_MAX(dfYMax, dfY);

  /* -------------------------------------------------------------------- */
  /*      Trim the rectangle to the area of the file itself, but out      */
  /*      to the edges of the touched edge pixels.                        */
  /* -------------------------------------------------------------------- */
  dfXMin = MS_MAX(0.0, MS_MIN(nRXSize, floor(dfXMin)));
  dfYMin = MS_MAX(0.0, MS_MIN(nRYSize, floor(dfYMin)));
  dfXMax = MS_MAX(0.0, MS_MIN(nRXSize, ceil(dfXMax)));
  dfYMax = MS_MAX(0.0, MS_MIN(nRYSize, ceil(dfYMax)));

  /* -------------------------------------------------------------------- */
  /*      Convert to integer offset/size values.                          */
  /* -------------------------------------------------------------------- */
  nWinXOff = (int)dfXMin;
  nWinYOff = (int)dfYMin;
  nWinXSize = (int)(dfXMax - dfXMin);
  nWinYSize = (int)(dfYMax - dfYMin);

  /* -------------------------------------------------------------------- */
  /*      What bands are we operating on?                                 */
  /* -------------------------------------------------------------------- */
  panBandMap = msGetGDALBandList(layer, hDS, 0, &nBandCount);

  if (rlinfo->band_count == -1)
    rlinfo->band_count = nBandCount;

  if (nBandCount != rlinfo->band_count) {
    msSetError(MS_IMGERR, "Got %d bands, but expected %d bands.",
               "msRasterQueryByRectLow()", nBandCount, rlinfo->band_count);

    return -1;
  }

  /* -------------------------------------------------------------------- */
  /*      Try to load the raster data.  For now we just load the first    */
  /*      band in the file.  Later we will deal with the various band     */
  /*      selection criteria.                                             */
  /* -------------------------------------------------------------------- */
  pafRaster = (float *)calloc(((size_t)nWinXSize) * nWinYSize * nBandCount,
                              sizeof(float));
  MS_CHECK_ALLOC(pafRaster, sizeof(float) * nWinXSize * nWinYSize * nBandCount,
                 -1);

  eErr = GDALDatasetRasterIO(hDS, GF_Read, nWinXOff, nWinYOff, nWinXSize,
                             nWinYSize, pafRaster, nWinXSize, nWinYSize,
                             GDT_Float32, nBandCount, panBandMap,
                             4 * nBandCount, 4 * nBandCount * nWinXSize, 4);

  if (eErr != CE_None) {
    msSetError(MS_IOERR, "GDALDatasetRasterIO() failed: %s",
               "msRasterQueryByRectLow()", CPLGetLastErrorMsg());

    free(pafRaster);
    return -1;
  }

  /* -------------------------------------------------------------------- */
  /*      Fetch color table for interpreting colors if needed.             */
  /* -------------------------------------------------------------------- */
  rlinfo->hCT = GDALGetRasterColorTable(GDALGetRasterBand(hDS, panBandMap[0]));

  free(panBandMap);

  /* -------------------------------------------------------------------- */
  /*      When computing whether pixels are within range we do it         */
  /*      based on the center of the pixel to the target point but        */
  /*      really it ought to be the nearest point on the pixel.  It       */
  /*      would be too much trouble to do this rigerously, so we just     */
  /*      add a fudge factor so that a range of zero will find the        */
  /*      pixel the target falls in at least.                             */
  /* -------------------------------------------------------------------- */
  dfAdjustedRange = sqrt(adfGeoTransform[1] * adfGeoTransform[1] +
                         adfGeoTransform[2] * adfGeoTransform[2] +
                         adfGeoTransform[4] * adfGeoTransform[4] +
                         adfGeoTransform[5] * adfGeoTransform[5]) *
                        0.5 * 1.41421356237 +
                    sqrt(rlinfo->range_dist);
  dfAdjustedRange = dfAdjustedRange * dfAdjustedRange;

  reprojectionObj *reprojector = NULL;
  if (layer->project) {
    reprojector =
        msProjectCreateReprojector(&(layer->projection), &(map->projection));
  }

  /* -------------------------------------------------------------------- */
  /*      Loop over all pixels determining which are "in".                */
  /* -------------------------------------------------------------------- */
  for (iLine = 0; iLine < nWinYSize; iLine++) {
    for (iPixel = 0; iPixel < nWinXSize; iPixel++) {
      pointObj sPixelLocation = {0};

      if (rlinfo->query_results == rlinfo->query_result_hard_max)
        break;

      /* transform pixel/line to georeferenced */
      sPixelLocation.x = GEO_TRANS(adfGeoTransform, iPixel + nWinXOff + 0.5,
                                   iLine + nWinYOff + 0.5);
      sPixelLocation.y = GEO_TRANS(adfGeoTransform + 3, iPixel + nWinXOff + 0.5,
                                   iLine + nWinYOff + 0.5);

      /* If projections differ, convert this back into the map  */
      /* projection for distance testing, and comprison to the  */
      /* search shape.  Save the original pixel location coordinates */
      /* in sPixelLocationInLayerSRS, so that we can return those */
      /* coordinates if we have a hit */
      pointObj sReprojectedPixelLocation = sPixelLocation;
      if (reprojector) {
        msProjectPointEx(reprojector, &sReprojectedPixelLocation);
      }

      /* If we are doing QueryByShape, check against the shape now */
      if (rlinfo->searchshape != NULL) {
        if (rlinfo->shape_tolerance == 0.0 &&
            rlinfo->searchshape->type == MS_SHAPE_POLYGON) {
          if (msIntersectPointPolygon(&sReprojectedPixelLocation,
                                      rlinfo->searchshape) == MS_FALSE)
            continue;
        } else {
          shapeObj tempShape;
          lineObj tempLine;

          memset(&tempShape, 0, sizeof(shapeObj));
          tempShape.type = MS_SHAPE_POINT;
          tempShape.numlines = 1;
          tempShape.line = &tempLine;
          tempLine.numpoints = 1;
          tempLine.point = &sReprojectedPixelLocation;

          if (msDistanceShapeToShape(rlinfo->searchshape, &tempShape) >
              rlinfo->shape_tolerance)
            continue;
        }
      }

      if (rlinfo->range_mode >= 0) {
        double dist;

        dist = (rlinfo->target_point.x - sReprojectedPixelLocation.x) *
                   (rlinfo->target_point.x - sReprojectedPixelLocation.x) +
               (rlinfo->target_point.y - sReprojectedPixelLocation.y) *
                   (rlinfo->target_point.y - sReprojectedPixelLocation.y);

        if (dist >= dfAdjustedRange)
          continue;

        /* If we can only have one feature, trim range and clear */
        /* previous result.  */
        if (rlinfo->range_mode == MS_QUERY_SINGLE) {
          rlinfo->range_dist = dist;
          rlinfo->query_results = 0;
        }
      }

      msRasterQueryAddPixel(layer,
                            &sPixelLocation, // return coords in layer SRS
                            &sReprojectedPixelLocation,
                            pafRaster +
                                (iLine * nWinXSize + iPixel) * nBandCount);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Cleanup.                                                        */
  /* -------------------------------------------------------------------- */
  free(pafRaster);
  msProjectDestroyReprojector(reprojector);

  return MS_SUCCESS;
}

/************************************************************************/
/*                        msRasterQueryByRect()                         */
/************************************************************************/

int msRasterQueryByRect(mapObj *map, layerObj *layer, rectObj queryRect)

{
  int status = MS_SUCCESS;
  char *filename = NULL;

  layerObj *tlp = NULL; /* pointer to the tile layer either real or temporary */
  int tileitemindex = -1, tilelayerindex = -1, tilesrsindex = -1;
  shapeObj tshp;
  char tilename[MS_PATH_LENGTH], tilesrsname[1024];
  int done, destroy_on_failure;

  char szPath[MS_MAXPATHLEN];
  rectObj searchrect;
  rasterLayerInfo *rlinfo = NULL;

  /* -------------------------------------------------------------------- */
  /*      Get the layer info.                                             */
  /* -------------------------------------------------------------------- */
  if (!layer->layerinfo) {
    msRasterLayerInfoInitialize(layer);
    destroy_on_failure = 1;
  } else {
    destroy_on_failure = 0;
  }
  rlinfo = (rasterLayerInfo *)layer->layerinfo;

  /* -------------------------------------------------------------------- */
  /*      Clear old results cache.                                        */
  /* -------------------------------------------------------------------- */
  if (layer->resultcache) {
    if (layer->resultcache->results)
      free(layer->resultcache->results);
    free(layer->resultcache);
    layer->resultcache = NULL;
  }

  /* -------------------------------------------------------------------- */
  /*      Initialize the results cache.                                   */
  /* -------------------------------------------------------------------- */
  layer->resultcache = (resultCacheObj *)msSmallMalloc(sizeof(resultCacheObj));
  layer->resultcache->results = NULL;
  layer->resultcache->numresults = layer->resultcache->cachesize = 0;
  layer->resultcache->bounds.minx = layer->resultcache->bounds.miny =
      layer->resultcache->bounds.maxx = layer->resultcache->bounds.maxy = -1;

  /* -------------------------------------------------------------------- */
  /*      Check if we should really be acting on this layer and           */
  /*      provide debug info in various cases.                            */
  /* -------------------------------------------------------------------- */
  if (layer->debug > 0 || map->debug > 1)
    msDebug("msRasterQueryByRect(%s): entering.\n", layer->name);

  if (!layer->data && !layer->tileindex) {
    if (layer->debug > 0 || map->debug > 0)
      msDebug("msRasterQueryByRect(%s): layer data and tileindex NULL ... "
              "doing nothing.",
              layer->name);
    return MS_SUCCESS;
  }

  if ((layer->status != MS_ON) && layer->status != MS_DEFAULT) {
    if (layer->debug > 0)
      msDebug(
          "msRasterQueryByRect(%s): not status ON or DEFAULT, doing nothing.",
          layer->name);
    return MS_SUCCESS;
  }

  /* ==================================================================== */
  /*      Handle setting up tileindex layer.                              */
  /* ==================================================================== */
  if (layer->tileindex) { /* we have an index file */
    msInitShape(&tshp);
    searchrect = queryRect;

    status = msDrawRasterSetupTileLayer(map, layer, &searchrect, MS_TRUE,
                                        &tilelayerindex, &tileitemindex,
                                        &tilesrsindex, &tlp);
    if (status != MS_SUCCESS) {
      goto cleanup;
    }
  } else {
    /* we have to manually apply to scaletoken logic as we're not going through
     * msLayerOpen() */
    status = msLayerApplyScaletokens(layer, map->scaledenom);
    if (status != MS_SUCCESS) {
      goto cleanup;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Iterate over all tiles (just one in untiled case).              */
  /* -------------------------------------------------------------------- */
  done = MS_FALSE;
  while (done == MS_FALSE && status == MS_SUCCESS) {

    GDALDatasetH hDS;
    char *decrypted_path = NULL;

    /* -------------------------------------------------------------------- */
    /*      Get filename.                                                   */
    /* -------------------------------------------------------------------- */
    if (layer->tileindex) {
      status = msDrawRasterIterateTileIndex(
          layer, tlp, &tshp, tileitemindex, tilesrsindex, tilename,
          sizeof(tilename), tilesrsname, sizeof(tilesrsname));
      if (status == MS_FAILURE) {
        break;
      }

      if (status == MS_DONE)
        break; /* no more tiles/images */
      filename = tilename;
    } else {
      filename = layer->data;
      done = MS_TRUE; /* only one image so we're done after this */
    }

    if (strlen(filename) == 0)
      continue;

    /* -------------------------------------------------------------------- */
    /*      Open the file.                                                  */
    /* -------------------------------------------------------------------- */
    msGDALInitialize();

    msDrawRasterBuildRasterPath(map, layer, filename, szPath);

    decrypted_path = msDecryptStringTokens(map, szPath);
    if (!decrypted_path) {
      status = MS_FAILURE;
      goto cleanup;
    }

    msAcquireLock(TLOCK_GDAL);
    if (!layer->tileindex) {
      char **connectionoptions =
          msGetStringListFromHashTable(&(layer->connectionoptions));
      hDS = GDALOpenEx(decrypted_path, GDAL_OF_RASTER, NULL,
                       (const char *const *)connectionoptions, NULL);
      CSLDestroy(connectionoptions);
    } else {
      hDS = GDALOpen(decrypted_path, GA_ReadOnly);
    }

    if (hDS == NULL) {
      int ignore_missing = msMapIgnoreMissingData(map);
      const char *cpl_error_msg =
          msDrawRasterGetCPLErrorMsg(decrypted_path, szPath);

      msFree(decrypted_path);
      decrypted_path = NULL;

      msReleaseLock(TLOCK_GDAL);

      if (ignore_missing == MS_MISSING_DATA_FAIL) {
        if (layer->debug || map->debug)
          msSetError(MS_IMGERR,
                     "Unable to open file %s for layer %s ... fatal error.\n%s",
                     "msRasterQueryByRect()", szPath, layer->name,
                     cpl_error_msg);
        status = MS_FAILURE;
        goto cleanup;
      }
      if (ignore_missing == MS_MISSING_DATA_LOG) {
        if (layer->debug || map->debug)
          msDebug("Unable to open file %s for layer %s ... ignoring this "
                  "missing data.\n%s",
                  filename, layer->name, cpl_error_msg);
      }
      continue;
    }

    msFree(decrypted_path);
    decrypted_path = NULL;

    if (msDrawRasterLoadProjection(layer, hDS, filename, tilesrsindex,
                                   tilesrsname) != MS_SUCCESS) {
      msReleaseLock(TLOCK_GDAL);
      status = MS_FAILURE;
      goto cleanup;
    }

    /* -------------------------------------------------------------------- */
    /*      Perform actual query on this file.                              */
    /* -------------------------------------------------------------------- */
    if (status == MS_SUCCESS)
      status = msRasterQueryByRectLow(map, layer, hDS, queryRect);

    GDALClose(hDS);
    msReleaseLock(TLOCK_GDAL);

  } /* next tile */

  /* -------------------------------------------------------------------- */
  /*      Cleanup tileindex if it is open.                                */
  /* -------------------------------------------------------------------- */
cleanup:
  if (layer->tileindex) { /* tiling clean-up */
    msDrawRasterCleanupTileLayer(tlp, tilelayerindex);
  } else {
    msLayerRestoreFromScaletokens(layer);
  }

  /* -------------------------------------------------------------------- */
  /*      On failure, or empty result set, cleanup the rlinfo since we    */
  /*      likely won't ever have it accessed or cleaned up later.         */
  /* -------------------------------------------------------------------- */
  if (status == MS_FAILURE || rlinfo->query_results == 0) {
    if (destroy_on_failure) {
      msRasterLayerInfoFree(layer);
    }
  } else {
    /* populate the items/numitems layer-level values */
    msRASTERLayerGetItems(layer);
  }

  return status;
}

/************************************************************************/
/*                        msRasterQueryByShape()                        */
/************************************************************************/

int msRasterQueryByShape(mapObj *map, layerObj *layer, shapeObj *selectshape)

{
  rasterLayerInfo *rlinfo = NULL;
  int status;
  double tolerance;
  rectObj searchrect;

  msRasterLayerInfoInitialize(layer);

  rlinfo = (rasterLayerInfo *)layer->layerinfo;

  /* If the selection shape is point or line we use the default tolerance of
     3, but for polygons we require an exact hit. */
  if (layer->tolerance == -1) {
    if (selectshape->type == MS_SHAPE_POINT ||
        selectshape->type == MS_SHAPE_LINE)
      tolerance = 3;
    else
      tolerance = 0;
  } else
    tolerance = layer->tolerance;

  if (layer->toleranceunits == MS_PIXELS)
    tolerance =
        tolerance * msAdjustExtent(&(map->extent), map->width, map->height);
  else
    tolerance = tolerance * (msInchesPerUnit(layer->toleranceunits, 0) /
                             msInchesPerUnit(map->units, 0));

  rlinfo->searchshape = selectshape;
  rlinfo->shape_tolerance = tolerance;

  msComputeBounds(selectshape);
  searchrect = selectshape->bounds;

  searchrect.minx -= tolerance; /* expand the search box to account for layer
                                   tolerances (e.g. buffered searches) */
  searchrect.maxx += tolerance;
  searchrect.miny -= tolerance;
  searchrect.maxy += tolerance;

  status = msRasterQueryByRect(map, layer, searchrect);

  rlinfo->searchshape = NULL;

  return status;
}

/************************************************************************/
/*                        msRasterQueryByPoint()                        */
/************************************************************************/

int msRasterQueryByPoint(mapObj *map, layerObj *layer, int mode, pointObj p,
                         double buffer, int maxresults) {
  int result;
  double layer_tolerance;
  rectObj bufferRect;
  rasterLayerInfo *rlinfo = NULL;

  msRasterLayerInfoInitialize(layer);

  rlinfo = (rasterLayerInfo *)layer->layerinfo;

  /* -------------------------------------------------------------------- */
  /*      If the buffer is not set, then use layer tolerances             */
  /*      instead.   The "buffer" distince is now in georeferenced        */
  /*      units.  Note that tolerances in pixels are basically map        */
  /*      display pixels, not underlying raster pixels.  It isn't         */
  /*      clear that there is any way of requesting a buffer size in      */
  /*      underlying pixels.                                              */
  /* -------------------------------------------------------------------- */
  if (buffer <= 0) { /* use layer tolerance */
    if (layer->tolerance == -1)
      layer_tolerance = 3;
    else
      layer_tolerance = layer->tolerance;

    if (layer->toleranceunits == MS_PIXELS)
      buffer = layer_tolerance *
               msAdjustExtent(&(map->extent), map->width, map->height);
    else
      buffer = layer_tolerance * (msInchesPerUnit(layer->toleranceunits, 0) /
                                  msInchesPerUnit(map->units, 0));
  }

  /* -------------------------------------------------------------------- */
  /*      Setup target point information, at this point they are in       */
  /*      map coordinates.                                                */
  /* -------------------------------------------------------------------- */
  rlinfo->range_dist = buffer * buffer;
  rlinfo->target_point = p;

  /* -------------------------------------------------------------------- */
  /*      if we are in the MS_QUERY_SINGLE mode, first try a query with   */
  /*      zero tolerance.  If this gets a raster pixel then we can be     */
  /*      reasonably assured that it is the closest to the query          */
  /*      point.  This will potentially be must more efficient than       */
  /*      processing all pixels within the tolerance.                     */
  /* -------------------------------------------------------------------- */
  if (mode == MS_QUERY_SINGLE) {
    rectObj pointRect;

    pointRect.minx = p.x;
    pointRect.maxx = p.x;
    pointRect.miny = p.y;
    pointRect.maxy = p.y;

    rlinfo->range_mode = MS_QUERY_SINGLE;
    result = msRasterQueryByRect(map, layer, pointRect);
    if (rlinfo->query_results > 0)
      return result;
  }

  /* -------------------------------------------------------------------- */
  /*      Setup a rectangle that is everything within the designated      */
  /*      range and do a search against that.                             */
  /* -------------------------------------------------------------------- */
  bufferRect.minx = p.x - buffer;
  bufferRect.maxx = p.x + buffer;
  bufferRect.miny = p.y - buffer;
  bufferRect.maxy = p.y + buffer;

  rlinfo->range_mode = mode;

  if (maxresults != 0) {
    const int previous_maxresults = rlinfo->query_result_hard_max;
    rlinfo->query_result_hard_max = maxresults;
    result = msRasterQueryByRect(map, layer, bufferRect);
    rlinfo->query_result_hard_max = previous_maxresults;
  } else {
    result = msRasterQueryByRect(map, layer, bufferRect);
  }

  return result;
}

/************************************************************************/
/* ==================================================================== */
/*                          RASTER Query Layer                          */
/*                                                                      */
/*      The results of a raster query are treated as a set of shapes    */
/*      that can be accessed using the normal layer semantics.          */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                         msRASTERLayerOpen()                          */
/************************************************************************/

int msRASTERLayerOpen(layerObj *layer) {
  rasterLayerInfo *rlinfo;

  /* If we don't have info, initialize an empty one now */
  if (layer->layerinfo == NULL)
    msRasterLayerInfoInitialize(layer);
  if (layer->layerinfo == NULL)
    return MS_FAILURE;

  rlinfo = (rasterLayerInfo *)layer->layerinfo;

  rlinfo->refcount = rlinfo->refcount + 1;

  return MS_SUCCESS;
}

/************************************************************************/
/*                         msRASTERIsLayerOpen()                        */
/************************************************************************/

int msRASTERLayerIsOpen(layerObj *layer) {
  if (layer->layerinfo)
    return MS_TRUE;
  return MS_FALSE;
}

/************************************************************************/
/*                     msRASTERLayerFreeItemInfo()                      */
/************************************************************************/
void msRASTERLayerFreeItemInfo(layerObj *layer) { (void)layer; }

/************************************************************************/
/*                     msRASTERLayerInitItemInfo()                      */
/*                                                                      */
/*      Perhaps we should be validating the requested items here?       */
/************************************************************************/

int msRASTERLayerInitItemInfo(layerObj *layer) {
  (void)layer;
  return MS_SUCCESS;
}

/************************************************************************/
/*                      msRASTERLayerWhichShapes()                      */
/************************************************************************/
int msRASTERLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery)

{
  (void)isQuery;
  rasterLayerInfo *rlinfo = (rasterLayerInfo *)layer->layerinfo;

  rlinfo->which_rect = rect;
  rlinfo->next_shape = 0;

  return MS_SUCCESS;
}

/************************************************************************/
/*                         msRASTERLayerClose()                         */
/************************************************************************/

int msRASTERLayerClose(layerObj *layer) {
  rasterLayerInfo *rlinfo = (rasterLayerInfo *)layer->layerinfo;

  if (rlinfo != NULL) {
    rlinfo->refcount--;

    if (rlinfo->refcount < 0)
      msRasterLayerInfoFree(layer);
  }
  return MS_SUCCESS;
}

/************************************************************************/
/*                       msRASTERLayerNextShape()                       */
/************************************************************************/

int msRASTERLayerNextShape(layerObj *layer, shapeObj *shape) {
  rasterLayerInfo *rlinfo = (rasterLayerInfo *)layer->layerinfo;

  if (rlinfo->next_shape < 0 || rlinfo->next_shape >= rlinfo->query_results) {
    msFreeShape(shape);
    shape->type = MS_SHAPE_NULL;
    return MS_DONE;
  } else {
    resultObj record;

    record.shapeindex = rlinfo->next_shape++;
    record.tileindex = 0;
    record.classindex = record.resultindex = -1;

    return msRASTERLayerGetShape(layer, shape, &record);
  }
}

/************************************************************************/
/*                       msRASTERLayerGetShape()                        */
/************************************************************************/

int msRASTERLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record) {
  rasterLayerInfo *rlinfo = (rasterLayerInfo *)layer->layerinfo;
  int i;

  long shapeindex = record->shapeindex;

  msFreeShape(shape);
  shape->type = MS_SHAPE_NULL;

  /* -------------------------------------------------------------------- */
  /*      Validate requested record id.                                   */
  /* -------------------------------------------------------------------- */
  if (shapeindex < 0 || shapeindex >= rlinfo->query_results) {
    msSetError(MS_MISCERR,
               "Out of range shape index requested.  Requested %ld\n"
               "but only %d shapes available.",
               "msRASTERLayerGetShape()", shapeindex, rlinfo->query_results);
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      Apply the geometry.                                             */
  /* -------------------------------------------------------------------- */
  if (rlinfo->qc_x != NULL) {
    lineObj line;
    pointObj point;

    shape->type = MS_SHAPE_POINT;

    line.numpoints = 1;
    line.point = &point;

    point.x = rlinfo->qc_x[shapeindex];
    point.y = rlinfo->qc_y[shapeindex];
    point.m = 0.0;

    msAddLine(shape, &line);
    msComputeBounds(shape);
  }

  /* -------------------------------------------------------------------- */
  /*      Apply the requested items.                                      */
  /* -------------------------------------------------------------------- */
  if (layer->numitems > 0) {
    shape->values = (char **)msSmallMalloc(sizeof(char *) * layer->numitems);
    shape->numvalues = layer->numitems;

    for (i = 0; i < layer->numitems; i++) {
      const size_t bufferSize = 1000;
      char szWork[1000];

      szWork[0] = '\0';
      if (EQUAL(layer->items[i], "x") && rlinfo->qc_x_reproj)
        snprintf(szWork, bufferSize, "%.8g", rlinfo->qc_x_reproj[shapeindex]);
      else if (EQUAL(layer->items[i], "y") && rlinfo->qc_y_reproj)
        snprintf(szWork, bufferSize, "%.8g", rlinfo->qc_y_reproj[shapeindex]);

      else if (EQUAL(layer->items[i], "value_list") && rlinfo->qc_values) {
        int iValue;

        for (iValue = 0; iValue < rlinfo->band_count; iValue++) {
          if (iValue != 0)
            strlcat(szWork, ",", bufferSize);

          snprintf(szWork + strlen(szWork), bufferSize - strlen(szWork), "%.8g",
                   rlinfo->qc_values[shapeindex * rlinfo->band_count + iValue]);
        }
      } else if (EQUALN(layer->items[i], "value_", 6) && rlinfo->qc_values) {
        int iValue = atoi(layer->items[i] + 6);
        snprintf(szWork, bufferSize, "%.8g",
                 rlinfo->qc_values[shapeindex * rlinfo->band_count + iValue]);
      } else if (EQUAL(layer->items[i], "class") && rlinfo->qc_class) {
        int p_class = rlinfo->qc_class[shapeindex];
        if (layer->class[p_class] -> name != NULL)
          snprintf(szWork, bufferSize, "%.999s", layer->class[p_class] -> name);
        else
          snprintf(szWork, bufferSize, "%d", p_class);
      } else if (EQUAL(layer->items[i], "red") && rlinfo->qc_red)
        snprintf(szWork, bufferSize, "%d", rlinfo->qc_red[shapeindex]);
      else if (EQUAL(layer->items[i], "green") && rlinfo->qc_green)
        snprintf(szWork, bufferSize, "%d", rlinfo->qc_green[shapeindex]);
      else if (EQUAL(layer->items[i], "blue") && rlinfo->qc_blue)
        snprintf(szWork, bufferSize, "%d", rlinfo->qc_blue[shapeindex]);
      else if (EQUAL(layer->items[i], "count") && rlinfo->qc_count)
        snprintf(szWork, bufferSize, "%d", rlinfo->qc_count[shapeindex]);

      shape->values[i] = msStrdup(szWork);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Eventually we should likey apply the geometry properly but      */
  /*      we don't really care about the geometry for query purposes.     */
  /* -------------------------------------------------------------------- */

  return MS_SUCCESS;
}

/************************************************************************/
/*                       msRASTERLayerGetItems()                        */
/************************************************************************/

int msRASTERLayerGetItems(layerObj *layer) {
  int maxnumitems = 0;
  rasterLayerInfo *rlinfo = (rasterLayerInfo *)layer->layerinfo;

  if (rlinfo == NULL)
    return MS_FAILURE;

  maxnumitems = 8 + (rlinfo->qc_values ? rlinfo->band_count : 0);
  layer->items = (char **)msSmallCalloc(sizeof(char *), maxnumitems);

  layer->numitems = 0;
  if (rlinfo->qc_x_reproj)
    layer->items[layer->numitems++] = msStrdup("x");
  if (rlinfo->qc_y_reproj)
    layer->items[layer->numitems++] = msStrdup("y");
  if (rlinfo->qc_values) {
    int i;
    for (i = 0; i < rlinfo->band_count; i++) {
      char szName[100];
      snprintf(szName, sizeof(szName), "value_%d", i);
      layer->items[layer->numitems++] = msStrdup(szName);
    }
    layer->items[layer->numitems++] = msStrdup("value_list");
  }
  if (rlinfo->qc_class)
    layer->items[layer->numitems++] = msStrdup("class");
  if (rlinfo->qc_red)
    layer->items[layer->numitems++] = msStrdup("red");
  if (rlinfo->qc_green)
    layer->items[layer->numitems++] = msStrdup("green");
  if (rlinfo->qc_blue)
    layer->items[layer->numitems++] = msStrdup("blue");
  if (rlinfo->qc_count)
    layer->items[layer->numitems++] = msStrdup("count");

  assert(layer->numitems <= maxnumitems);

  return msRASTERLayerInitItemInfo(layer);
}

/************************************************************************/
/*                       msRASTERLayerGetExtent()                       */
/************************************************************************/

int msRASTERLayerGetExtent(layerObj *layer, rectObj *extent)

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
/*                     msRASTERLayerSetTimeFilter()                     */
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

int msRASTERLayerSetTimeFilter(layerObj *layer, const char *timestring,
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

int msRASTERLayerInitializeVirtualTable(layerObj *layer) {
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerInitItemInfo = msRASTERLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msRASTERLayerFreeItemInfo;
  layer->vtable->LayerOpen = msRASTERLayerOpen;
  layer->vtable->LayerIsOpen = msRASTERLayerIsOpen;
  layer->vtable->LayerWhichShapes = msRASTERLayerWhichShapes;
  layer->vtable->LayerNextShape = msRASTERLayerNextShape;
  layer->vtable->LayerGetShape = msRASTERLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */
  layer->vtable->LayerClose = msRASTERLayerClose;
  layer->vtable->LayerGetItems = msRASTERLayerGetItems;
  layer->vtable->LayerGetExtent = msRASTERLayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  /* layer->vtable->LayerCloseConnection = msRASTERLayerClose; */
  /* we use backtics for proper tileindex shapefile functioning */
  layer->vtable->LayerSetTimeFilter = msRASTERLayerSetTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */

  return MS_SUCCESS;
}

/**********************************************************************
 * $Id: mapcontour.c 12629 2011-10-06 18:06:34Z aboudreault $
 *
 * Project:  MapServer
 * Purpose:  Contour Layer
 * Author:   Alan Boudreault (aboudreault@mapgears.com)
 * Author:   Daniel Morissette (dmorissette@mapgears.com)
 *
 **********************************************************************
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
#include "mapcopy.h"
#include "mapresample.h"

#include "ogr_api.h"
#include "ogr_srs_api.h"
#include "gdal.h"
#include "gdal_alg.h"

#include "mapows.h"
#include "mapthread.h"
#include "mapraster.h"
#include "cpl_string.h"

#define GEO_TRANS(tr, x, y) ((tr)[0] + (tr)[1] * (x) + (tr)[2] * (y))

extern int InvGeoTransform(double *gt_in, double *gt_out);

typedef struct {

  /* OGR DataSource */
  layerObj ogrLayer;

  /* internal use */
  GDALDatasetH hOrigDS;
  GDALDatasetH hDS;
  double *buffer; /* memory dataset buffer */
  rectObj extent; /* original dataset extent */
  OGRDataSourceH hOGRDS;
  double cellsize;

} contourLayerInfo;

static int msContourLayerInitItemInfo(layerObj *layer) {
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (clinfo == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: Contour layer not opened!!!",
               "msContourLayerInitItemInfo()");
    return MS_FAILURE;
  }

  return msLayerInitItemInfo(&clinfo->ogrLayer);
}

static void msContourLayerFreeItemInfo(layerObj *layer) {
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (clinfo == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: Contour layer not opened!!!",
               "msContourLayerFreeItemInfo()");
    return;
  }

  msLayerFreeItemInfo(&clinfo->ogrLayer);
}

static void msContourLayerInfoInitialize(layerObj *layer) {
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (clinfo != NULL)
    return;

  clinfo = (contourLayerInfo *)msSmallCalloc(1, sizeof(contourLayerInfo));
  layer->layerinfo = clinfo;
  clinfo->hOrigDS = NULL;
  clinfo->hDS = NULL;
  clinfo->extent.minx = -1.0;
  clinfo->extent.miny = -1.0;
  clinfo->extent.maxx = -1.0;
  clinfo->extent.maxy = -1.0;

  initLayer(&clinfo->ogrLayer, layer->map);
  clinfo->ogrLayer.type = layer->type;
  clinfo->ogrLayer.debug = layer->debug;
  clinfo->ogrLayer.connectiontype = MS_OGR;
  clinfo->ogrLayer.name = msStrdup(layer->name);
  clinfo->ogrLayer.connection =
      (char *)msSmallMalloc(strlen(clinfo->ogrLayer.name) + 13);
  sprintf(clinfo->ogrLayer.connection, "__%s_CONTOUR__", clinfo->ogrLayer.name);
  clinfo->ogrLayer.units = layer->units;

  if (msOWSLookupMetadata(&(layer->metadata), "OFG", "ID_type") == NULL) {
    msInsertHashTable(&(layer->metadata), "gml_ID_type", "Integer");
  }
  {
    const char *elevItem = CSLFetchNameValue(layer->processing, "CONTOUR_ITEM");
    if (elevItem && strlen(elevItem) > 0) {
      char szTmp[100];
      snprintf(szTmp, sizeof(szTmp), "%s_type", elevItem);
      if (msOWSLookupMetadata(&(layer->metadata), "OFG", szTmp) == NULL) {
        snprintf(szTmp, sizeof(szTmp), "gml_%s_type", elevItem);
        msInsertHashTable(&(layer->metadata), szTmp, "Real");
      }
    }
  }
}

static void msContourLayerInfoFree(layerObj *layer) {
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (clinfo == NULL)
    return;

  freeLayer(&clinfo->ogrLayer);
  free(clinfo);

  layer->layerinfo = NULL;
}

static int msContourLayerReadRaster(layerObj *layer, rectObj rect) {
  mapObj *map = layer->map;
  char **bands;
  int band = 1;
  double adfGeoTransform[6], adfInvGeoTransform[6];
  double llx, lly, urx, ury;
  rectObj copyRect, mapRect;
  int dst_xsize, dst_ysize;
  int virtual_grid_step_x, virtual_grid_step_y;
  int src_xoff, src_yoff, src_xsize, src_ysize;
  double map_cellsize_x, map_cellsize_y, dst_cellsize_x, dst_cellsize_y;
  GDALRasterBandH hBand = NULL;
  CPLErr eErr;

  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (layer->debug)
    msDebug("Entering msContourLayerReadRaster().\n");

  if (clinfo == NULL || clinfo->hOrigDS == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: Contour layer not opened!!!",
               "msContourLayerReadRaster()");
    return MS_FAILURE;
  }

  bands = CSLTokenizeStringComplex(
      CSLFetchNameValue(layer->processing, "BANDS"), " ,", FALSE, FALSE);
  if (CSLCount(bands) > 0) {
    band = atoi(bands[0]);
    if (band < 1 || band > GDALGetRasterCount(clinfo->hOrigDS)) {
      msSetError(MS_IMGERR,
                 "BANDS PROCESSING directive includes illegal band '%d', "
                 "should be from 1 to %d.",
                 "msContourLayerReadRaster()", band,
                 GDALGetRasterCount(clinfo->hOrigDS));
      CSLDestroy(bands);
      return MS_FAILURE;
    }
  }
  CSLDestroy(bands);

  hBand = GDALGetRasterBand(clinfo->hOrigDS, band);
  if (hBand == NULL) {
    msSetError(MS_IMGERR, "Band %d does not exist on dataset.",
               "msContourLayerReadRaster()", band);
    return MS_FAILURE;
  }

  if (layer->projection.numargs > 0 &&
      EQUAL(layer->projection.args[0], "auto")) {
    const char *wkt;
    wkt = GDALGetProjectionRef(clinfo->hOrigDS);
    if (wkt != NULL && strlen(wkt) > 0) {
      if (msOGCWKT2ProjectionObj(wkt, &(layer->projection), layer->debug) !=
          MS_SUCCESS) {
        char msg[MESSAGELENGTH * 2];
        errorObj *ms_error = msGetErrorObj();

        snprintf(msg, sizeof(msg),
                 "%s\n"
                 "PROJECTION AUTO cannot be used for this "
                 "GDAL raster (`%s').",
                 ms_error->message, layer->data);
        msg[MESSAGELENGTH - 1] = '\0';

        msSetError(MS_OGRERR, "%s", "msDrawRasterLayer()", msg);
        return MS_FAILURE;
      }
    }
  }

  /*
   * Compute the georeferenced window of overlap, and read the source data
   * downsampled to match output resolution, or at full resolution if
   * output resolution is lower than the source resolution.
   *
   * A large portion of this overlap calculation code was borrowed from
   * msDrawRasterLayerGDAL().
   * Would be possible to move some of this to a reusable function?
   *
   * Note: This code works only if no reprojection is involved. It would
   * need rework to support cases where output projection differs from source
   * data file projection.
   */

  src_xsize = GDALGetRasterXSize(clinfo->hOrigDS);
  src_ysize = GDALGetRasterYSize(clinfo->hOrigDS);

  /* set the Dataset extent */
  msGetGDALGeoTransform(clinfo->hOrigDS, map, layer, adfGeoTransform);
  clinfo->extent.minx = adfGeoTransform[0];
  clinfo->extent.maxy = adfGeoTransform[3];
  clinfo->extent.maxx = adfGeoTransform[0] + src_xsize * adfGeoTransform[1];
  clinfo->extent.miny = adfGeoTransform[3] + src_ysize * adfGeoTransform[5];

  if (layer->transform) {
    if (layer->debug)
      msDebug("msContourLayerReadRaster(): Entering transform.\n");

    InvGeoTransform(adfGeoTransform, adfInvGeoTransform);

    mapRect = rect;
    if (map->cellsize == 0) {
      map->cellsize = msAdjustExtent(&mapRect, map->width, map->height);
    }
    map_cellsize_x = map_cellsize_y = map->cellsize;
    /* if necessary, project the searchrect to source coords */
    if (msProjectionsDiffer(&(map->projection), &(layer->projection))) {
      if (msProjectRect(&map->projection, &layer->projection, &mapRect) !=
          MS_SUCCESS) {
        msDebug("msContourLayerReadRaster(%s): unable to reproject map request "
                "rectangle into layer projection, canceling.\n",
                layer->name);
        return MS_FAILURE;
      }

      map_cellsize_x = MS_CELLSIZE(mapRect.minx, mapRect.maxx, map->width);
      map_cellsize_y = MS_CELLSIZE(mapRect.miny, mapRect.maxy, map->height);

      /* if the projection failed to project the extent requested, we need to
         calculate the cellsize to preserve the initial map cellsize ratio */
      if ((mapRect.minx < GEO_TRANS(adfGeoTransform, 0, src_ysize)) ||
          (mapRect.maxx > GEO_TRANS(adfGeoTransform, src_xsize, 0)) ||
          (mapRect.miny < GEO_TRANS(adfGeoTransform + 3, 0, src_ysize)) ||
          (mapRect.maxy > GEO_TRANS(adfGeoTransform + 3, src_xsize, 0))) {

        int src_unit, dst_unit;
        src_unit = GetMapserverUnitUsingProj(&map->projection);
        dst_unit = GetMapserverUnitUsingProj(&layer->projection);
        if (src_unit == -1 || dst_unit == -1) {
          msDebug("msContourLayerReadRaster(%s): unable to reproject map "
                  "request rectangle into layer projection, canceling.\n",
                  layer->name);
          return MS_FAILURE;
        }

        map_cellsize_x = MS_CONVERT_UNIT(
            src_unit, dst_unit, MS_CELLSIZE(rect.minx, rect.maxx, map->width));
        map_cellsize_y = MS_CONVERT_UNIT(
            src_unit, dst_unit, MS_CELLSIZE(rect.miny, rect.maxy, map->height));
      }
    }

    if (map_cellsize_x == 0 || map_cellsize_y == 0) {
      if (layer->debug)
        msDebug("msContourLayerReadRaster(): Cellsize can't be 0.\n");
      return MS_FAILURE;
    }

    /* Adjust MapServer pixel model to GDAL pixel model */
    mapRect.minx -= map_cellsize_x * 0.5;
    mapRect.maxx += map_cellsize_x * 0.5;
    mapRect.miny -= map_cellsize_y * 0.5;
    mapRect.maxy += map_cellsize_y * 0.5;

    /*
     * If raw data cellsize (from geotransform) is larger than output
     * map_cellsize then we want to extract only enough data to match the output
     * map resolution which means that GDAL will automatically sample the data
     * on read.
     *
     * To prevent bad contour effects on tile edges, we adjust the target
     * cellsize to align the extracted window with a virtual grid based on the
     * origin of the raw data and a virtual grid step size corresponding to an
     * integer sampling step.
     *
     * If source data has a greater cellsize (i.e. lower res) that requested
     * output map then we use the raw data cellsize as target cellsize since
     * there is no point in interpolating the data for contours in this case.
     */

    virtual_grid_step_x = (int)floor(map_cellsize_x / ABS(adfGeoTransform[1]));
    if (virtual_grid_step_x < 1)
      virtual_grid_step_x =
          1; /* Do not interpolate data if grid sampling step < 1 */

    virtual_grid_step_y = (int)floor(map_cellsize_y / ABS(adfGeoTransform[5]));
    if (virtual_grid_step_y < 1)
      virtual_grid_step_y =
          1; /* Do not interpolate data if grid sampling step < 1 */

    /* target cellsize is a multiple of raw data cellsize based on grid step*/
    dst_cellsize_x = ABS(adfGeoTransform[1]) * virtual_grid_step_x;
    dst_cellsize_y = ABS(adfGeoTransform[5]) * virtual_grid_step_y;

    /* Compute overlap between source and target views */

    copyRect = mapRect;

    if (copyRect.minx < GEO_TRANS(adfGeoTransform, 0, src_ysize))
      copyRect.minx = GEO_TRANS(adfGeoTransform, 0, src_ysize);
    if (copyRect.maxx > GEO_TRANS(adfGeoTransform, src_xsize, 0))
      copyRect.maxx = GEO_TRANS(adfGeoTransform, src_xsize, 0);
    if (copyRect.miny < GEO_TRANS(adfGeoTransform + 3, 0, src_ysize))
      copyRect.miny = GEO_TRANS(adfGeoTransform + 3, 0, src_ysize);
    if (copyRect.maxy > GEO_TRANS(adfGeoTransform + 3, src_xsize, 0))
      copyRect.maxy = GEO_TRANS(adfGeoTransform + 3, src_xsize, 0);

    if (copyRect.minx >= copyRect.maxx || copyRect.miny >= copyRect.maxy) {
      if (layer->debug)
        msDebug("msContourLayerReadRaster(): No overlap.\n");
      return MS_SUCCESS;
    }

    /*
     * Convert extraction window to raster coordinates
     */
    llx = GEO_TRANS(adfInvGeoTransform + 0, copyRect.minx, copyRect.miny);
    lly = GEO_TRANS(adfInvGeoTransform + 3, copyRect.minx, copyRect.miny);
    urx = GEO_TRANS(adfInvGeoTransform + 0, copyRect.maxx, copyRect.maxy);
    ury = GEO_TRANS(adfInvGeoTransform + 3, copyRect.maxx, copyRect.maxy);

    /*
     * Align extraction window with virtual grid
     * (keep in mind raster coordinates origin is at upper-left)
     * We also add an extra buffer to fix tile boundarie issues when zoomed
     */
    llx = floor(llx / virtual_grid_step_x) * virtual_grid_step_x -
          (virtual_grid_step_x * 5);
    urx = ceil(urx / virtual_grid_step_x) * virtual_grid_step_x +
          (virtual_grid_step_x * 5);
    ury = floor(ury / virtual_grid_step_y) * virtual_grid_step_y -
          (virtual_grid_step_x * 5);
    lly = ceil(lly / virtual_grid_step_y) * virtual_grid_step_y +
          (virtual_grid_step_x * 5);

    src_xoff = MS_MAX(0, (int)floor(llx + 0.5));
    src_yoff = MS_MAX(0, (int)floor(ury + 0.5));
    src_xsize = MS_MIN(MS_MAX(0, (int)(urx - llx + 0.5)),
                       GDALGetRasterXSize(clinfo->hOrigDS) - src_xoff);
    src_ysize = MS_MIN(MS_MAX(0, (int)(lly - ury + 0.5)),
                       GDALGetRasterYSize(clinfo->hOrigDS) - src_yoff);

    /* Update the geographic extent (buffer added) */
    /* TODO: a better way to go the geo_trans */
    copyRect.minx = GEO_TRANS(adfGeoTransform + 0, src_xoff, 0);
    copyRect.maxx = GEO_TRANS(adfGeoTransform + 0, src_xoff + src_xsize, 0);
    copyRect.miny = GEO_TRANS(adfGeoTransform + 3, 0, src_yoff + src_ysize);
    copyRect.maxy = GEO_TRANS(adfGeoTransform + 3, 0, src_yoff);

    /*
     * If input window is to small then stop here
     */
    if (src_xsize < 2 || src_ysize < 2) {
      if (layer->debug)
        msDebug("msContourLayerReadRaster(): input window too small, or no "
                "apparent overlap between map view and this window(1).\n");
      return MS_SUCCESS;
    }

    /* Target buffer size */
    dst_xsize = (int)ceil((copyRect.maxx - copyRect.minx) / dst_cellsize_x);
    dst_ysize = (int)ceil((copyRect.maxy - copyRect.miny) / dst_cellsize_y);

    if (dst_xsize == 0 || dst_ysize == 0) {
      if (layer->debug)
        msDebug("msContourLayerReadRaster(): no apparent overlap between map "
                "view and this window(2).\n");
      return MS_SUCCESS;
    }

    if (layer->debug)
      msDebug("msContourLayerReadRaster(): src=%d,%d,%d,%d, dst=%d,%d,%d,%d\n",
              src_xoff, src_yoff, src_xsize, src_ysize, 0, 0, dst_xsize,
              dst_ysize);
  } else {
    src_xoff = 0;
    src_yoff = 0;
    dst_xsize = src_xsize = MS_MIN(map->width, src_xsize);
    dst_ysize = src_ysize = MS_MIN(map->height, src_ysize);
    copyRect.minx = 0;
    copyRect.miny = 0;
    (void)copyRect.miny;
    copyRect.maxx = map->width;
    copyRect.maxy = map->height;
    (void)copyRect.maxx;
    dst_cellsize_y = dst_cellsize_x = 1;
  }

  /* -------------------------------------------------------------------- */
  /*      Allocate buffer, and read data into it.                         */
  /* -------------------------------------------------------------------- */

  clinfo->buffer = (double *)malloc(sizeof(double) * dst_xsize * dst_ysize);
  if (clinfo->buffer == NULL) {
    msSetError(MS_MEMERR, "Malloc(): Out of memory.",
               "msContourLayerReadRaster()");
    return MS_FAILURE;
  }

  eErr = GDALRasterIO(hBand, GF_Read, src_xoff, src_yoff, src_xsize, src_ysize,
                      clinfo->buffer, dst_xsize, dst_ysize, GDT_Float64, 0, 0);

  if (eErr != CE_None) {
    msSetError(MS_IOERR, "GDALRasterIO() failed: %s",
               "msContourLayerReadRaster()", CPLGetLastErrorMsg());
    free(clinfo->buffer);
    clinfo->buffer = NULL;
    return MS_FAILURE;
  }

  GDALDriverH hMemDRV = GDALGetDriverByName("MEM");
  if (!hMemDRV) {
    msSetError(MS_IOERR, "GDAL MEM driver not available",
               "msContourLayerReadRaster()");
    free(clinfo->buffer);
    clinfo->buffer = NULL;
    return MS_FAILURE;
  }

  char pointer[64];
  memset(pointer, 0, sizeof(pointer));
  CPLPrintPointer(pointer, clinfo->buffer, sizeof(pointer));

  clinfo->hDS = GDALCreate(hMemDRV, "", dst_xsize, dst_ysize, 0, 0, NULL);
  if (clinfo->hDS == NULL) {
    msSetError(MS_IMGERR, "Unable to create GDAL Memory dataset.",
               "msContourLayerReadRaster()");
    free(clinfo->buffer);
    clinfo->buffer = NULL;
    return MS_FAILURE;
  }

  char **papszOptions = CSLSetNameValue(NULL, "DATAPOINTER", pointer);
  eErr = GDALAddBand(clinfo->hDS, GDT_Float64, papszOptions);
  CSLDestroy(papszOptions);
  if (eErr != CE_None) {
    msSetError(MS_IMGERR, "Unable to add band to GDAL Memory dataset.",
               "msContourLayerReadRaster()");
    free(clinfo->buffer);
    clinfo->buffer = NULL;
    return MS_FAILURE;
  }

  {
    // Copy nodata value from source dataset to memory dataset
    int bHasNoData = FALSE;
    double dfNoDataValue = GDALGetRasterNoDataValue(hBand, &bHasNoData);
    if (bHasNoData) {
      GDALSetRasterNoDataValue(GDALGetRasterBand(clinfo->hDS, 1),
                               dfNoDataValue);
    }
  }

  adfGeoTransform[0] = copyRect.minx;
  adfGeoTransform[1] = dst_cellsize_x;
  adfGeoTransform[2] = 0;
  adfGeoTransform[3] = copyRect.maxy;
  adfGeoTransform[4] = 0;
  adfGeoTransform[5] = -dst_cellsize_y;

  clinfo->cellsize = MS_MAX(dst_cellsize_x, dst_cellsize_y);
  {
    char buf[64];
    sprintf(buf, "%lf", clinfo->cellsize);
    msInsertHashTable(&layer->metadata, "__data_cellsize__", buf);
  }

  GDALSetGeoTransform(clinfo->hDS, adfGeoTransform);
  return MS_SUCCESS;
}

static void msContourOGRCloseConnection(void *conn_handle) {
  OGRDataSourceH hDS = (OGRDataSourceH)conn_handle;

  msAcquireLock(TLOCK_OGR);
  OGR_DS_Destroy(hDS);
  msReleaseLock(TLOCK_OGR);
}

/* Function that parses multiple options in the a list. It also supports
   min/maxscaledenom checks. ie. "CONTOUR_INTERVAL=0,3842942:10" */
static char *msContourGetOption(layerObj *layer, const char *name) {
  int c, i, found = MS_FALSE;
  char **values, **tmp, **options;
  double maxscaledenom, minscaledenom;
  char *value = NULL;

  options = CSLFetchNameValueMultiple(layer->processing, name);
  c = CSLCount(options);

  /* First pass to find the value among options that have min/maxscaledenom */
  /* specified */
  for (i = 0; i < c && found == MS_FALSE; ++i) {
    values = CSLTokenizeStringComplex(options[i], ":", FALSE, FALSE);
    if (CSLCount(values) == 2) {
      tmp = CSLTokenizeStringComplex(values[0], ",", FALSE, FALSE);
      if (CSLCount(tmp) == 2) {
        minscaledenom = atof(tmp[0]);
        maxscaledenom = atof(tmp[1]);
        if (layer->map->scaledenom <= 0 ||
            (((maxscaledenom <= 0) ||
              (layer->map->scaledenom <= maxscaledenom)) &&
             ((minscaledenom <= 0) ||
              (layer->map->scaledenom > minscaledenom)))) {
          value = msStrdup(values[1]);
          found = MS_TRUE;
        }
      }
      CSLDestroy(tmp);
    }
    CSLDestroy(values);
  }

  /* Second pass to find the value among options that do NOT have */
  /* min/maxscaledenom specified */
  for (i = 0; i < c && found == MS_FALSE; ++i) {
    values = CSLTokenizeStringComplex(options[i], ":", FALSE, FALSE);
    if (CSLCount(values) == 1) {
      value = msStrdup(values[0]);
      found = MS_TRUE;
    }
    CSLDestroy(values);
  }

  CSLDestroy(options);

  return value;
}

static int msContourLayerGenerateContour(layerObj *layer) {
  OGRSFDriverH hDriver;
  OGRFieldDefnH hFld;
  OGRLayerH hLayer;
  const char *elevItem;
  char *option;
  double interval = 1.0, levels[1000];
  int levelCount = 0;
  GDALRasterBandH hBand = NULL;
  CPLErr eErr;
  int bHasNoData = FALSE;
  double dfNoDataValue;

  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  OGRRegisterAll();

  if (clinfo == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: Contour layer not opened!!!",
               "msContourLayerCreateOGRDataSource()");
    return MS_FAILURE;
  }

  if (!clinfo->hDS) { /* no overlap */
    return MS_SUCCESS;
  }

  hBand = GDALGetRasterBand(clinfo->hDS, 1);
  if (hBand == NULL) {
    msSetError(MS_IMGERR, "Band %d does not exist on dataset.",
               "msContourLayerGenerateContour()", 1);
    return MS_FAILURE;
  }

  /* Create the OGR DataSource */
#if GDAL_VERSION_MAJOR > 3 ||                                                  \
    (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
  const char *pszDrvName = "MEM";
#else
  const char *pszDrvName = "Memory";
#endif
  hDriver = OGRGetDriverByName(pszDrvName);
  if (hDriver == NULL) {
    msSetError(MS_OGRERR, "Unable to get OGR driver '%s'.",
               "msContourLayerCreateOGRDataSource()", pszDrvName);
    return MS_FAILURE;
  }

  clinfo->hOGRDS = OGR_Dr_CreateDataSource(hDriver, "", NULL);
  if (clinfo->hOGRDS == NULL) {
    msSetError(MS_OGRERR, "Unable to create OGR DataSource.",
               "msContourLayerCreateOGRDataSource()");
    return MS_FAILURE;
  }

  hLayer = OGR_DS_CreateLayer(clinfo->hOGRDS, clinfo->ogrLayer.name, NULL,
                              wkbLineString, NULL);

  hFld = OGR_Fld_Create("ID", OFTInteger);
  OGR_Fld_SetWidth(hFld, 8);
  OGR_L_CreateField(hLayer, hFld, FALSE);
  OGR_Fld_Destroy(hFld);

  /* Check if we have a coutour item specified */
  elevItem = CSLFetchNameValue(layer->processing, "CONTOUR_ITEM");
  if (elevItem && strlen(elevItem) > 0) {
    hFld = OGR_Fld_Create(elevItem, OFTReal);
    OGR_Fld_SetWidth(hFld, 12);
    OGR_Fld_SetPrecision(hFld, 3);
    OGR_L_CreateField(hLayer, hFld, FALSE);
    OGR_Fld_Destroy(hFld);
  } else {
    elevItem = NULL;
  }

  option = msContourGetOption(layer, "CONTOUR_INTERVAL");
  if (option) {
    interval = atof(option);
    free(option);
  }

  option = msContourGetOption(layer, "CONTOUR_LEVELS");
  if (option) {
    int i, c;
    char **levelsTmp;
    levelsTmp = CSLTokenizeStringComplex(option, ",", FALSE, FALSE);
    c = CSLCount(levelsTmp);
    for (i = 0; i < c && i < (int)(sizeof(levels) / sizeof(double)); ++i)
      levels[levelCount++] = atof(levelsTmp[i]);

    CSLDestroy(levelsTmp);
    free(option);
  }

  dfNoDataValue = GDALGetRasterNoDataValue(hBand, &bHasNoData);

  eErr = GDALContourGenerate(
      hBand, interval, 0.0, levelCount, levels, bHasNoData, dfNoDataValue,
      hLayer, OGR_FD_GetFieldIndex(OGR_L_GetLayerDefn(hLayer), "ID"),
      (elevItem == NULL)
          ? -1
          : OGR_FD_GetFieldIndex(OGR_L_GetLayerDefn(hLayer), elevItem),
      NULL, NULL);

  if (eErr != CE_None) {
    msSetError(MS_IOERR, "GDALContourGenerate() failed: %s",
               "msContourLayerGenerateContour()", CPLGetLastErrorMsg());
    return MS_FAILURE;
  }

  msConnPoolRegister(&clinfo->ogrLayer, clinfo->hOGRDS,
                     msContourOGRCloseConnection);

  return MS_SUCCESS;
}

int msContourLayerOpen(layerObj *layer) {
  char *decrypted_path;
  char szPath[MS_MAXPATHLEN];
  contourLayerInfo *clinfo;

  if (layer->debug)
    msDebug("Entering msContourLayerOpen().\n");

  /* If we don't have info, initialize an empty one now */
  if (layer->layerinfo == NULL)
    msContourLayerInfoInitialize(layer);

  clinfo = (contourLayerInfo *)layer->layerinfo;
  if (layer->data == NULL && layer->tileindex == NULL) {
    msSetError(MS_MISCERR, "Layer %s has neither DATA nor TILEINDEX defined.",
               "msContourLayerOpen()", layer->name);
    return MS_FAILURE;
  }

  if (layer->tileindex != NULL) {
    char szTilename[MS_MAXPATHLEN];
    int status;
    int tilelayerindex, tileitemindex, tilesrsindex;
    rectObj searchrect;
    layerObj *tlp;
    shapeObj tshp;
    char tilesrsname[1];

    msInitShape(&tshp);
    searchrect = layer->map->extent;

    status = msDrawRasterSetupTileLayer(layer->map, layer, &searchrect,
                                        MS_FALSE, &tilelayerindex,
                                        &tileitemindex, &tilesrsindex, &tlp);
    if (status == MS_FAILURE) {
      return MS_FAILURE;
    }

    status = msDrawRasterIterateTileIndex(layer, tlp, &tshp, tileitemindex, -1,
                                          szTilename, sizeof(szTilename),
                                          tilesrsname, sizeof(tilesrsname));
    if (status == MS_FAILURE || status == MS_DONE) {
      if (status == MS_DONE) {
        if (layer->debug)
          msDebug("No raster matching filter.\n");
      }
      msDrawRasterCleanupTileLayer(tlp, tilelayerindex);
      return MS_FAILURE;
    }

    msDrawRasterCleanupTileLayer(tlp, tilelayerindex);

    msDrawRasterBuildRasterPath(layer->map, layer, szTilename, szPath);
    decrypted_path = msStrdup(szPath);

    /* Cancel the time filter that might have been set on ours in case of */
    /* a inline tileindex */
    msFreeExpression(&layer->filter);
    msInitExpression(&layer->filter);
  } else {
    msTryBuildPath3(szPath, layer->map->mappath, layer->map->shapepath,
                    layer->data);
    decrypted_path = msDecryptStringTokens(layer->map, szPath);
  }

  GDALAllRegister();

  /* Open the original Dataset */

  msAcquireLock(TLOCK_GDAL);
  if (decrypted_path) {
    clinfo->hOrigDS = GDALOpen(decrypted_path, GA_ReadOnly);
    msFree(decrypted_path);
  } else
    clinfo->hOrigDS = NULL;

  msReleaseLock(TLOCK_GDAL);

  if (clinfo->hOrigDS == NULL) {
    msSetError(MS_IMGERR, "Unable to open GDAL dataset.",
               "msContourLayerOpen()");
    return MS_FAILURE;
  }

  /* Open the raster source */
  if (msContourLayerReadRaster(layer, layer->map->extent) != MS_SUCCESS)
    return MS_FAILURE;

  /* Generate Contour Dataset */
  if (msContourLayerGenerateContour(layer) != MS_SUCCESS)
    return MS_FAILURE;

  if (clinfo->hDS) {
    GDALClose(clinfo->hDS);
    clinfo->hDS = NULL;
    free(clinfo->buffer);
  }

  /* Open our virtual ogr layer */
  if (clinfo->hOGRDS && (msLayerOpen(&clinfo->ogrLayer) != MS_SUCCESS))
    return MS_FAILURE;

  return MS_SUCCESS;
}

int msContourLayerIsOpen(layerObj *layer) {
  if (layer->layerinfo)
    return MS_TRUE;
  return MS_FALSE;
}

int msContourLayerClose(layerObj *layer) {
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (layer->debug)
    msDebug("Entering msContourLayerClose().\n");

  if (clinfo) {
    if (clinfo->hOGRDS)
      msConnPoolRelease(&clinfo->ogrLayer, clinfo->hOGRDS);

    msLayerClose(&clinfo->ogrLayer);

    if (clinfo->hDS) {
      GDALClose(clinfo->hDS);
      clinfo->hDS = NULL;
      free(clinfo->buffer);
    }

    if (clinfo->hOrigDS) {
      GDALClose(clinfo->hOrigDS);
      clinfo->hOrigDS = NULL;
    }

    msContourLayerInfoFree(layer);
  }

  return MS_SUCCESS;
}

int msContourLayerGetItems(layerObj *layer) {
  const char *elevItem;
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (clinfo == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: Contour layer not opened!!!",
               "msContourLayerGetItems()");
    return MS_FAILURE;
  }

  layer->numitems = 0;
  layer->items = (char **)msSmallCalloc(sizeof(char *), 2);

  layer->items[layer->numitems++] = msStrdup("ID");
  elevItem = CSLFetchNameValue(layer->processing, "CONTOUR_ITEM");
  if (elevItem && strlen(elevItem) > 0) {
    layer->items[layer->numitems++] = msStrdup(elevItem);
  }

  return msLayerGetItems(&clinfo->ogrLayer);
}

int msContourLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery) {
  int i;
  rectObj newRect;
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (layer->debug)
    msDebug("Entering msContourLayerWhichShapes().\n");

  if (clinfo == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: Contour layer not opened!!!",
               "msContourLayerWhichShapes()");
    return MS_FAILURE;
  }

  if (isQuery) {
    newRect = layer->map->extent;
  } else {
    newRect = rect;
    /* if necessary, project the searchrect to source coords */
    if (msProjectionsDiffer(&(layer->map->projection), &(layer->projection))) {
      if (msProjectRect(&layer->projection, &layer->map->projection,
                        &newRect) != MS_SUCCESS) {
        msDebug("msContourLayerWhichShapes(%s): unable to reproject map "
                "request rectangle into layer projection, canceling.\n",
                layer->name);
        return MS_FAILURE;
      }
    }
  }

  /* regenerate the raster io */
  if (clinfo->hOGRDS)
    msConnPoolRelease(&clinfo->ogrLayer, clinfo->hOGRDS);

  msLayerClose(&clinfo->ogrLayer);

  /* Open the raster source */
  if (msContourLayerReadRaster(layer, newRect) != MS_SUCCESS)
    return MS_FAILURE;

  /* Generate Contour Dataset */
  if (msContourLayerGenerateContour(layer) != MS_SUCCESS)
    return MS_FAILURE;

  if (clinfo->hDS) {
    GDALClose(clinfo->hDS);
    clinfo->hDS = NULL;
    free(clinfo->buffer);
  }

  if (!clinfo->hOGRDS) /* no overlap */
    return MS_DONE;

  /* Open our virtual ogr layer */
  if (msLayerOpen(&clinfo->ogrLayer) != MS_SUCCESS)
    return MS_FAILURE;

  clinfo->ogrLayer.numitems = layer->numitems;
  clinfo->ogrLayer.items =
      (char **)msSmallMalloc(sizeof(char *) * layer->numitems);
  for (i = 0; i < layer->numitems; ++i) {
    clinfo->ogrLayer.items[i] = msStrdup(layer->items[i]);
  }

  return msLayerWhichShapes(&clinfo->ogrLayer, rect, isQuery);
}

int msContourLayerGetShape(layerObj *layer, shapeObj *shape,
                           resultObj *record) {
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (layer->debug)
    msDebug("Entering msContourLayerGetShape().\n");

  if (clinfo == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: Contour layer not opened!!!",
               "msContourLayerGetShape()");
    return MS_FAILURE;
  }

  return msLayerGetShape(&clinfo->ogrLayer, shape, record);
}

int msContourLayerNextShape(layerObj *layer, shapeObj *shape) {
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (layer->debug)
    msDebug("Entering msContourLayerNextShape().\n");

  if (clinfo == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: Contour layer not opened!!!",
               "msContourLayerNextShape()");
    return MS_FAILURE;
  }

  return msLayerNextShape(&clinfo->ogrLayer, shape);
}

/************************************************************************/
/*                       msContourLayerGetExtent()                     */
/* Simple copy of the maprasterquery.c file. might change in the future */
/************************************************************************/

int msContourLayerGetExtent(layerObj *layer, rectObj *extent) {
  contourLayerInfo *clinfo = (contourLayerInfo *)layer->layerinfo;

  if (layer->debug)
    msDebug("Entering msContourLayerGetExtent().\n");

  if (clinfo == NULL) {
    msSetError(MS_MISCERR, "Assertion failed: Contour layer not opened!!!",
               "msContourLayerGetExtent()");
    return MS_FAILURE;
  }

  MS_COPYRECT(extent, &clinfo->extent);

  return MS_SUCCESS;
}

/************************************************************************/
/*                     msContourLayerSetTimeFilter()                    */
/*                                                                      */
/*      This function is actually just used in the context of           */
/*      setting a filter on the tileindex for time based queries.       */
/*      For instance via WMS requests.                                  */
/*                                                                      */
/*      If a local shapefile tileindex is in use, we will set a         */
/*      backtics filter (shapefile compatible).  If another layer is    */
/*      being used as the tileindex then we will forward the            */
/*      SetTimeFilter call to it.  If there is no tileindex in          */
/*      place, we do nothing.                                           */
/************************************************************************/

int msContourLayerSetTimeFilter(layerObj *layer, const char *timestring,
                                const char *timefield) {
  int tilelayerindex;

  if (layer->debug)
    msDebug("msContourLayerSetTimeFilter(%s,%s).\n", timestring, timefield);

  /* -------------------------------------------------------------------- */
  /*      If we don't have a tileindex the time filter has no effect.     */
  /* -------------------------------------------------------------------- */
  if (layer->tileindex == NULL) {
    if (layer->debug)
      msDebug("msContourLayerSetTimeFilter(): time filter without effect on "
              "layers without tileindex.\n");
    return MS_SUCCESS;
  }

  /* -------------------------------------------------------------------- */
  /*      Find the tileindex layer.                                       */
  /* -------------------------------------------------------------------- */
  tilelayerindex = msGetLayerIndex(layer->map, layer->tileindex);

  /* -------------------------------------------------------------------- */
  /*      If we are using a local shapefile as our tileindex (that is     */
  /*      to say, the tileindex name is not of another layer), then we    */
  /*      just install a backtics style filter on the current layer.      */
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

int msContourLayerInitializeVirtualTable(layerObj *layer) {
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerInitItemInfo = msContourLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msContourLayerFreeItemInfo;
  layer->vtable->LayerOpen = msContourLayerOpen;
  layer->vtable->LayerIsOpen = msContourLayerIsOpen;
  layer->vtable->LayerWhichShapes = msContourLayerWhichShapes;
  layer->vtable->LayerNextShape = msContourLayerNextShape;
  layer->vtable->LayerGetShape = msContourLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */
  layer->vtable->LayerClose = msContourLayerClose;
  layer->vtable->LayerGetItems = msContourLayerGetItems;
  layer->vtable->LayerGetExtent = msContourLayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  /*layer->vtable->LayerCloseConnection = msContourLayerClose;*/
  /* we use backtics for proper tileindex shapefile functioning */
  layer->vtable->LayerSetTimeFilter = msContourLayerSetTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */

  return MS_SUCCESS;
}

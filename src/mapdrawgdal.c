/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Code for drawing GDAL raster layers.  Called from
 *           msDrawRasterLayerLow() in mapraster.c.
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
 *****************************************************************************/

#include <assert.h>
#include <stdbool.h>
#include "mapserver.h"
#include "mapresample.h"
#include "mapthread.h"
#include "maptime.h"

extern int InvGeoTransform(double *gt_in, double *gt_out);

#define MAXCOLORS 256
#define GEO_TRANS(tr, x, y) ((tr)[0] + (tr)[1] * (x) + (tr)[2] * (y))
#define SKIP_MASK(x, y)                                                        \
  (mask_rb && !*(mask_rb->data.rgba.a + (y)*mask_rb->data.rgba.row_step +      \
                 (x)*mask_rb->data.rgba.pixel_step))

#include "gdal.h"
#include "cpl_string.h"

#include "gdal_alg.h"

static bool IsNoData(double dfValue, double dfNoDataValue);

static int LoadGDALImages(GDALDatasetH hDS, int band_numbers[4], int band_count,
                          layerObj *layer, int src_xoff, int src_yoff,
                          int src_xsize, int src_ysize, GByte *pabyBuffer,
                          int dst_xsize, int dst_ysize, int *pbHaveRGBNoData,
                          int *pnNoData1, int *pnNoData2, int *pnNoData3,
                          bool *pbScaled, double *pdfScaleMin,
                          double *pdfScaleRatio, bool **ppbIsNoDataBuffer);
static int msDrawRasterLayerGDAL_RawMode(mapObj *map, layerObj *layer,
                                         imageObj *image, GDALDatasetH hDS,
                                         int src_xoff, int src_yoff,
                                         int src_xsize, int src_ysize,
                                         int dst_xoff, int dst_yoff,
                                         int dst_xsize, int dst_ysize);

static int msDrawRasterLayerGDAL_16BitClassification(
    mapObj *map, layerObj *layer, rasterBufferObj *rb, GDALRasterBandH hBand,
    int src_xoff, int src_yoff, int src_xsize, int src_ysize, int dst_xoff,
    int dst_yoff, int dst_xsize, int dst_ysize);

/*
 * rasterBufferObj setting macros.
 */

/************************************************************************/
/*                       msDrawRasterLayerGDAL()                        */
/************************************************************************/

int msDrawRasterLayerGDAL(mapObj *map, layerObj *layer, imageObj *image,
                          rasterBufferObj *rb, void *hDSVoid)

{
  int i; /* loop counters */
  int cmap[MAXCOLORS];
#ifndef NDEBUG
  int cmap_set = FALSE;
#endif
  unsigned char rb_cmap[4][MAXCOLORS];
  double adfGeoTransform[6], adfInvGeoTransform[6];
  int dst_xoff, dst_yoff, dst_xsize, dst_ysize;
  int src_xoff, src_yoff, src_xsize, src_ysize;
  double llx, lly, urx, ury;
  rectObj copyRect, mapRect;
  unsigned char *pabyRaw1 = NULL, *pabyRaw2 = NULL, *pabyRaw3 = NULL,
                *pabyRawAlpha = NULL;
  int classified = FALSE;
  int red_band = 0, green_band = 0, blue_band = 0, alpha_band = 0;
  int band_count, band_numbers[4];
  GDALDatasetH hDS = hDSVoid;
  GDALColorTableH hColorMap = NULL;
  GDALRasterBandH hBand1 = NULL, hBand2 = NULL, hBand3 = NULL,
                  hBandAlpha = NULL;
  int bHaveRGBNoData = FALSE;
  int nNoData1 = -1, nNoData2 = -1, nNoData3 = -1;
  rasterBufferObj *mask_rb = NULL;
  rasterBufferObj s_mask_rb;

  if (layer->mask) {
    int ret;
    layerObj *maskLayer = GET_LAYER(map, msGetLayerIndex(map, layer->mask));
    mask_rb = &s_mask_rb;
    memset(mask_rb, 0, sizeof(s_mask_rb));
    ret = MS_IMAGE_RENDERER(maskLayer->maskimage)
              ->getRasterBufferHandle(maskLayer->maskimage, mask_rb);
    if (ret != MS_SUCCESS)
      return -1;
  }

  /*only support rawdata and pluggable renderers*/
  assert(MS_RENDERER_RAWDATA(image->format) ||
         (MS_RENDERER_PLUGIN(image->format) && rb));

  memset(cmap, 0xff, MAXCOLORS * sizeof(int));
  memset(rb_cmap, 0, sizeof(rb_cmap));

  /* -------------------------------------------------------------------- */
  /*      Test the image format instead of the map format.                */
  /*      Normally the map format and the image format should be the      */
  /*      same but In some cases like swf and pdf support, a temporary   */
  /*      GD image object is created and used to render raster layers     */
  /*      and then dumped into the SWF or the PDF file.                   */
  /* -------------------------------------------------------------------- */

  src_xsize = GDALGetRasterXSize(hDS);
  src_ysize = GDALGetRasterYSize(hDS);

  /*
   * If the RAW_WINDOW attribute is set, use that to establish what to
   * load.  This is normally just set by the mapresample.c module to avoid
   * problems with rotated maps.
   */

  if (CSLFetchNameValue(layer->processing, "RAW_WINDOW") != NULL) {
    char **papszTokens =
        CSLTokenizeString(CSLFetchNameValue(layer->processing, "RAW_WINDOW"));

    if (layer->debug)
      msDebug("msDrawGDAL(%s): using RAW_WINDOW=%s, dst=0,0,%d,%d\n",
              layer->name, CSLFetchNameValue(layer->processing, "RAW_WINDOW"),
              image->width, image->height);

    if (CSLCount(papszTokens) != 4) {
      CSLDestroy(papszTokens);
      msSetError(MS_IMGERR, "RAW_WINDOW PROCESSING directive corrupt.",
                 "msDrawGDAL()");
      return -1;
    }

    src_xoff = atoi(papszTokens[0]);
    src_yoff = atoi(papszTokens[1]);
    src_xsize = atoi(papszTokens[2]);
    src_ysize = atoi(papszTokens[3]);

    dst_xoff = 0;
    dst_yoff = 0;
    dst_xsize = image->width;
    dst_ysize = image->height;

    CSLDestroy(papszTokens);
  }

  /*
   * Compute the georeferenced window of overlap, and do nothing if there
   * is no overlap between the map extents, and the file we are operating on.
   */
  else if (layer->transform) {
    int dst_lrx, dst_lry;

    if (layer->debug)
      msDebug("msDrawRasterLayerGDAL(): Entering transform.\n");

    msGetGDALGeoTransform(hDS, map, layer, adfGeoTransform);
    InvGeoTransform(adfGeoTransform, adfInvGeoTransform);

    mapRect = map->extent;

    mapRect.minx -= map->cellsize * 0.5;
    mapRect.maxx += map->cellsize * 0.5;
    mapRect.miny -= map->cellsize * 0.5;
    mapRect.maxy += map->cellsize * 0.5;

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
        msDebug("msDrawRasterLayerGDAL(): Error in overlap calculation.\n");
      return 0;
    }

    /*
     * Copy the source and destination raster coordinates.
     */
    llx = GEO_TRANS(adfInvGeoTransform + 0, copyRect.minx, copyRect.miny);
    lly = GEO_TRANS(adfInvGeoTransform + 3, copyRect.minx, copyRect.miny);
    urx = GEO_TRANS(adfInvGeoTransform + 0, copyRect.maxx, copyRect.maxy);
    ury = GEO_TRANS(adfInvGeoTransform + 3, copyRect.maxx, copyRect.maxy);

    src_xoff = MS_MAX(0, (int)floor(llx + 0.5));
    src_yoff = MS_MAX(0, (int)floor(ury + 0.5));
    src_xsize = MS_MIN(MS_MAX(0, (int)(urx - llx + 0.5)),
                       GDALGetRasterXSize(hDS) - src_xoff);
    src_ysize = MS_MIN(MS_MAX(0, (int)(lly - ury + 0.5)),
                       GDALGetRasterYSize(hDS) - src_yoff);

    /* We want very small windows to use at least one source pixel (#4172) */
    if (src_xsize == 0 && (urx - llx) > 0.0) {
      src_xsize = 1;
      src_xoff = MS_MIN(src_xoff, GDALGetRasterXSize(hDS) - 1);
    }
    if (src_ysize == 0 && (lly - ury) > 0.0) {
      src_ysize = 1;
      src_yoff = MS_MIN(src_yoff, GDALGetRasterYSize(hDS) - 1);
    }

    if (src_xsize == 0 || src_ysize == 0) {
      if (layer->debug)
        msDebug("msDrawRasterLayerGDAL(): no apparent overlap between map view "
                "and this window(1).\n");
      return 0;
    }

    if (map->cellsize == 0) {
      if (layer->debug)
        msDebug("msDrawRasterLayerGDAL(): Cellsize can't be 0.\n");
      return 0;
    }

    dst_xoff = (int)((copyRect.minx - mapRect.minx) / map->cellsize);
    dst_yoff = (int)((mapRect.maxy - copyRect.maxy) / map->cellsize);

    dst_lrx = (int)((copyRect.maxx - mapRect.minx) / map->cellsize + 0.5);
    dst_lry = (int)((mapRect.maxy - copyRect.miny) / map->cellsize + 0.5);
    dst_lrx = MS_MAX(0, MS_MIN(image->width, dst_lrx));
    dst_lry = MS_MAX(0, MS_MIN(image->height, dst_lry));

    dst_xsize = MS_MAX(0, MS_MIN(image->width, dst_lrx - dst_xoff));
    dst_ysize = MS_MAX(0, MS_MIN(image->height, dst_lry - dst_yoff));

    if (dst_xsize == 0 || dst_ysize == 0) {
      if (layer->debug)
        msDebug("msDrawRasterLayerGDAL(): no apparent overlap between map view "
                "and this window(2).\n");
      return 0;
    }

    if (layer->debug)
      msDebug("msDrawRasterLayerGDAL(): src=%d,%d,%d,%d, dst=%d,%d,%d,%d\n",
              src_xoff, src_yoff, src_xsize, src_ysize, dst_xoff, dst_yoff,
              dst_xsize, dst_ysize);
#ifndef notdef
    if (layer->debug) {
      double d_src_xoff, d_src_yoff, geo_x, geo_y;

      geo_x = mapRect.minx + dst_xoff * map->cellsize;
      geo_y = mapRect.maxy - dst_yoff * map->cellsize;

      d_src_xoff = (geo_x - adfGeoTransform[0]) / adfGeoTransform[1];
      d_src_yoff = (geo_y - adfGeoTransform[3]) / adfGeoTransform[5];

      msDebug("msDrawRasterLayerGDAL(): source raster PL (%.3f,%.3f) for dst "
              "PL (%d,%d).\n",
              d_src_xoff, d_src_yoff, dst_xoff, dst_yoff);
    }
#endif
  }

  /*
   * If layer transforms are turned off, just map 1:1.
   */
  else {
    dst_xoff = src_xoff = 0;
    dst_yoff = src_yoff = 0;
    dst_xsize = src_xsize = MS_MIN(image->width, src_xsize);
    dst_ysize = src_ysize = MS_MIN(image->height, src_ysize);
  }

  /*
   * In RAWDATA mode we don't fool with colors.  Do the raw processing,
   * and return from the function early.
   */
  if (MS_RENDERER_RAWDATA(image->format)) {
    return msDrawRasterLayerGDAL_RawMode(
        map, layer, image, hDS, src_xoff, src_yoff, src_xsize, src_ysize,
        dst_xoff, dst_yoff, dst_xsize, dst_ysize);
  }

  /*
   * Is this image classified?  We consider it classified if there are
   * classes with an expression string *or* a color range.  We don't want
   * to treat the raster as classified if there is just a bogus class here
   * to force inclusion in the legend.
   */
  for (i = 0; i < layer->numclasses; i++) {
    int s;

    /* change colour based on colour range? */
    for (s = 0; s < layer->class[i] -> numstyles; s++) {
      if (MS_VALID_COLOR(layer->class[i] -> styles[s] -> mincolor) &&
          MS_VALID_COLOR(layer->class[i] -> styles[s] -> maxcolor)) {
        classified = TRUE;
        break;
      }
    }

    if (layer->class[i] -> expression.string != NULL) {
      classified = TRUE;
      break;
    }
  }

  /*
   * Set up the band selection.  We look for a BANDS directive in the
   * the PROCESSING options.  If not found we default to grey=1, grey=1,alpha=2,
   * red=1,green=2,blue=3 or red=1,green=2,blue=3,alpha>=4.
   */

  if (CSLFetchNameValue(layer->processing, "BANDS") == NULL) {
    const int gdal_band_count = GDALGetRasterCount(hDS);
    red_band = 1;

    if (gdal_band_count >= 4) {
      /* The alpha band is not necessarily the 4th one */
      for (i = 4; i <= gdal_band_count; i++) {
        if (GDALGetRasterColorInterpretation(GDALGetRasterBand(hDS, i)) ==
            GCI_AlphaBand) {
          alpha_band = i;
          break;
        }
      }
    }

    if (gdal_band_count >= 3) {
      green_band = 2;
      blue_band = 3;
    }

    if (gdal_band_count == 2 && GDALGetRasterColorInterpretation(
                                    GDALGetRasterBand(hDS, 2)) == GCI_AlphaBand)
      alpha_band = 2;

    hBand1 = GDALGetRasterBand(hDS, red_band);
    if (classified || GDALGetRasterColorTable(hBand1) != NULL) {
      alpha_band = 0;
      green_band = 0;
      blue_band = 0;
    }
  } else {
    int *band_list;

    band_list = msGetGDALBandList(layer, hDS, 4, &band_count);
    if (band_list == NULL)
      return -1;

    if (band_count > 0)
      red_band = band_list[0];
    else
      red_band = 0;
    if (band_count > 2) {
      green_band = band_list[1];
      blue_band = band_list[2];
    } else {
      green_band = 0;
      blue_band = 0;
    }

    if (band_count > 3)
      alpha_band = band_list[3];
    else
      alpha_band = 0;

    free(band_list);
  }

  band_numbers[0] = red_band;
  band_numbers[1] = green_band;
  band_numbers[2] = blue_band;
  band_numbers[3] = 0;

  if (blue_band != 0 && alpha_band != 0) {
    band_numbers[3] = alpha_band;
    band_count = 4;
  } else if (blue_band != 0 && alpha_band == 0)
    band_count = 3;
  else if (alpha_band != 0) {
    band_numbers[1] = alpha_band;
    band_count = 2;
  } else
    band_count = 1;

  if (layer->debug > 1 || (layer->debug > 0 && green_band != 0)) {
    msDebug(
        "msDrawRasterLayerGDAL(): red,green,blue,alpha bands = %d,%d,%d,%d\n",
        red_band, green_band, blue_band, alpha_band);
  }

  /*
   * Get band handles for PC256, RGB or RGBA cases.
   */
  hBand1 = GDALGetRasterBand(hDS, red_band);
  if (hBand1 == NULL)
    return -1;

  hBand2 = hBand3 = hBandAlpha = NULL;

  if (green_band != 0) {
    hBand1 = GDALGetRasterBand(hDS, red_band);
    hBand2 = GDALGetRasterBand(hDS, green_band);
    hBand3 = GDALGetRasterBand(hDS, blue_band);
    if (hBand1 == NULL || hBand2 == NULL || hBand3 == NULL)
      return -1;
  }

  if (alpha_band != 0)
    hBandAlpha = GDALGetRasterBand(hDS, alpha_band);

  /*
   * The logic for a classification rendering of non-8bit raster bands
   * is sufficiently different than the normal mechanism of loading
   * into an 8bit buffer, that we isolate it into it's own subfunction.
   */
  if (classified && hBand1 != NULL &&
      GDALGetRasterDataType(hBand1) != GDT_Byte) {
    return msDrawRasterLayerGDAL_16BitClassification(
        map, layer, rb, hBand1, src_xoff, src_yoff, src_xsize, src_ysize,
        dst_xoff, dst_yoff, dst_xsize, dst_ysize);
  }

  /*
   * Get colormap for this image.  If there isn't one, and we have only
   * one band create a greyscale colormap.
   */
  bool isDefaultGreyscale = false;

  if (hBand2 != NULL)
    hColorMap = NULL;
  else {
    hColorMap = GDALGetRasterColorTable(hBand1);
    if (hColorMap != NULL)
      hColorMap = GDALCloneColorTable(hColorMap);
    else {
      hColorMap = GDALCreateColorTable(GPI_RGB);
      isDefaultGreyscale = true;

      for (i = 0; i < 256; i++) {
        colorObj pixel = {i, i, i, 0};
        GDALColorEntry sEntry;

        if (MS_COMPARE_COLORS(pixel, layer->offsite)) {
          sEntry.c1 = 0;
          sEntry.c2 = 0;
          sEntry.c3 = 0;
          sEntry.c4 = 0; /* alpha set to zero */
        } else {
          sEntry.c1 = i;
          sEntry.c2 = i;
          sEntry.c3 = i;
          sEntry.c4 = 255;
        }

        GDALSetColorEntry(hColorMap, i, &sEntry);
      }
    }

    /*
    ** If we have a known NODATA value, mark it now as transparent.
    */
    {
      int bGotNoData;
      double dfNoDataValue = msGetGDALNoDataValue(layer, hBand1, &bGotNoData);

      if (bGotNoData && dfNoDataValue >= 0 &&
          dfNoDataValue < GDALGetColorEntryCount(hColorMap)) {
        GDALColorEntry sEntry;

        memcpy(&sEntry, GDALGetColorEntry(hColorMap, (int)dfNoDataValue),
               sizeof(GDALColorEntry));

        sEntry.c4 = 0;
        GDALSetColorEntry(hColorMap, (int)dfNoDataValue, &sEntry);
      }
    }
  }

  /*
   * Allocate imagery buffers.
   */
  pabyRaw1 =
      (unsigned char *)malloc(((size_t)dst_xsize) * dst_ysize * band_count);
  if (pabyRaw1 == NULL) {
    msSetError(MS_MEMERR, "Allocating work image of size %dx%dx%d failed.",
               "msDrawRasterLayerGDAL()", dst_xsize, dst_ysize, band_count);
    return -1;
  }

  if (hBand2 != NULL && hBand3 != NULL) {
    pabyRaw2 = pabyRaw1 + dst_xsize * dst_ysize * 1;
    pabyRaw3 = pabyRaw1 + dst_xsize * dst_ysize * 2;
  }

  if (hBandAlpha != NULL) {
    if (hBand2 != NULL)
      pabyRawAlpha = pabyRaw1 + dst_xsize * dst_ysize * 3;
    else
      pabyRawAlpha = pabyRaw1 + dst_xsize * dst_ysize * 1;
  }

  /*
   * Load image data into buffers with scaling, etc.
   */
  bool bScaled = false;
  double dfScaleMin = 0;
  double dfScaleRatio = 0;
  bool *pbIsNoDataBuffer = NULL;
  if (LoadGDALImages(hDS, band_numbers, band_count, layer, src_xoff, src_yoff,
                     src_xsize, src_ysize, pabyRaw1, dst_xsize, dst_ysize,
                     &bHaveRGBNoData, &nNoData1, &nNoData2, &nNoData3, &bScaled,
                     &dfScaleMin, &dfScaleRatio, &pbIsNoDataBuffer) == -1) {
    free(pabyRaw1);
    free(pbIsNoDataBuffer);
    return -1;
  }

  /*
   * Setup the mapping between source eight bit pixel values, and the
   * output images color table.  There are two general cases, where the
   * class colors are provided by the MAP file, or where we use the native
   * color table.
   */
  if (classified) {
    int c;
    const char *pszRangeColorspace =
        msLayerGetProcessingKey(layer, "RANGE_COLORSPACE");
    colorspace iRangeColorspace;

#ifndef NDEBUG
    cmap_set = TRUE;
#endif

    if (hColorMap == NULL) {
      msSetError(MS_IOERR,
                 "Attempt to classify 24bit image, this is unsupported.",
                 "drawGDAL()");
      free(pabyRaw1);
      free(pbIsNoDataBuffer);
      return -1;
    }

    if (!pszRangeColorspace || !strcasecmp(pszRangeColorspace, "RGB")) {
      iRangeColorspace = MS_COLORSPACE_RGB;
    } else if (!strcasecmp(pszRangeColorspace, "HSL")) {
      iRangeColorspace = MS_COLORSPACE_HSL;
    } else {
      msSetError(MS_MISCERR,
                 "Unknown RANGE_COLORSPACE \"%s\", expecting RGB or HSL",
                 "drawGDAL()", pszRangeColorspace);
      GDALDestroyColorTable(hColorMap);
      free(pabyRaw1);
      free(pbIsNoDataBuffer);
      return -1;
    }

    int color_count = GDALGetColorEntryCount(hColorMap);
    const bool bScaleColors = bScaled && !isDefaultGreyscale;

    if (!bScaleColors && color_count > 256)
      color_count = 256;

    for (i = 0; i < color_count; i++) {
      colorObj pixel;
      int colormap_index;
      GDALColorEntry sEntry;

      GDALGetColorEntryAsRGB(hColorMap, i, &sEntry);

      const int j =
          bScaleColors
              ? MAX(0, MIN(255, (int)((i - dfScaleMin) * dfScaleRatio)))
              : i;

      pixel.red = sEntry.c1;
      pixel.green = sEntry.c2;
      pixel.blue = sEntry.c3;
      colormap_index = i;

      if (!MS_COMPARE_COLORS(pixel, layer->offsite)) {
        c = msGetClass(layer, &pixel, colormap_index);

        if (c != -1) { /* belongs to any class */
          int s;

          /* change colour based on colour range?  Currently we
             only address the greyscale case properly. */

          if (MS_VALID_COLOR(layer->class[c] -> styles[0] -> mincolor)) {
            for (s = 0; s < layer->class[c] -> numstyles; s++) {
              if (MS_VALID_COLOR(layer->class[c] -> styles[s] -> mincolor) &&
                  MS_VALID_COLOR(layer->class[c] -> styles[s] -> maxcolor)) {
                if (sEntry.c1 >= layer->class[c] -> styles[s]
                    -> minvalue &&
                           sEntry.c1 <=
                               layer->class[c] -> styles[s] -> maxvalue) {
                  msValueToRange(layer->class[c] -> styles[s], sEntry.c1,
                                 iRangeColorspace);
                  if (MS_VALID_COLOR(layer->class[c] -> styles[s] -> color)) {
                    rb_cmap[0][j] = layer->class[c]->styles[s]->color.red;
                    rb_cmap[1][j] = layer->class[c]->styles[s]->color.green;
                    rb_cmap[2][j] = layer->class[c]->styles[s]->color.blue;
                    rb_cmap[3][j] =
                        (layer->class[c] -> styles[s] -> color.alpha != 255)
                            ? (layer->class[c] -> styles[s] -> color.alpha)
                            : (255 * layer->class[c] -> styles[0] -> opacity /
                                                                         100);
                    break;
                  }
                }
              }
            }
          } else if (MS_TRANSPARENT_COLOR(
                         layer->class[c] -> styles[0] -> color))
            /* leave it transparent */;

          else if (MS_VALID_COLOR(layer->class[c] -> styles[0] -> color)) {
            rb_cmap[0][j] = layer->class[c]->styles[0]->color.red;
            rb_cmap[1][j] = layer->class[c]->styles[0]->color.green;
            rb_cmap[2][j] = layer->class[c]->styles[0]->color.blue;
            rb_cmap[3][j] =
                (255 * layer->class[c] -> styles[0] -> opacity / 100);
          }

          else { /* Use raster color */
            rb_cmap[0][j] = pixel.red;
            rb_cmap[1][j] = pixel.green;
            rb_cmap[2][j] = pixel.blue;
            rb_cmap[3][j] = 255;
          }
        }
      }
    }
  } else if (hBand2 == NULL && hColorMap != NULL &&
             rb->type == MS_BUFFER_BYTE_RGBA) {
    int color_count;
#ifndef NDEBUG
    cmap_set = TRUE;
#endif

    color_count = MS_MIN(256, GDALGetColorEntryCount(hColorMap));
    const bool bScaleColors = bScaled && !isDefaultGreyscale;

    for (i = 0; i < color_count; i++) {
      GDALColorEntry sEntry;

      GDALGetColorEntryAsRGB(hColorMap, i, &sEntry);
      const int j =
          bScaleColors
              ? MAX(0, MIN(255, (int)((i - dfScaleMin) * dfScaleRatio)))
              : i;

      if (sEntry.c4 != 0 &&
          (!MS_VALID_COLOR(layer->offsite) || layer->offsite.red != sEntry.c1 ||
           layer->offsite.green != sEntry.c2 ||
           layer->offsite.blue != sEntry.c3)) {
        rb_cmap[0][j] = sEntry.c1;
        rb_cmap[1][j] = sEntry.c2;
        rb_cmap[2][j] = sEntry.c3;
        rb_cmap[3][j] = sEntry.c4;
      }
    }
  }

  if (bHaveRGBNoData && layer->debug)
    msDebug("msDrawGDAL(): using RGB nodata values from GDAL dataset.\n");

  /* -------------------------------------------------------------------- */
  /*      If there was no alpha band, but we have a dataset level mask    */
  /*      load it as massage it so it will function as our alpha for      */
  /*      transparency purposes.                                          */
  /* -------------------------------------------------------------------- */
  if (hBandAlpha == NULL) {
    int nMaskFlags = GDALGetMaskFlags(hBand1);

    if (CSLTestBoolean(
            CSLFetchNameValueDef(layer->processing, "USE_MASK_BAND", "YES")) &&
        (nMaskFlags & GMF_PER_DATASET) != 0 &&
        (nMaskFlags & (GMF_NODATA | GMF_ALL_VALID)) == 0) {
      CPLErr eErr;
      unsigned char *pabyOrig = pabyRaw1;

      if (layer->debug)
        msDebug("msDrawGDAL(): using GDAL mask band for alpha.\n");

      band_count++;

      pabyRaw1 = (unsigned char *)realloc(pabyOrig, ((size_t)dst_xsize) *
                                                        dst_ysize * band_count);

      if (pabyRaw1 == NULL) {
        msSetError(MS_MEMERR, "Allocating work image of size %dx%dx%d failed.",
                   "msDrawRasterLayerGDAL()", dst_xsize, dst_ysize, band_count);
        free(pabyOrig);
        if (hColorMap != NULL)
          GDALDestroyColorTable(hColorMap);
        free(pbIsNoDataBuffer);
        return -1;
      }

      if (hBand2 != NULL) {
        pabyRaw2 = pabyRaw1 + dst_xsize * dst_ysize * 1;
        pabyRaw3 = pabyRaw1 + dst_xsize * dst_ysize * 2;
        pabyRawAlpha = pabyRaw1 + dst_xsize * dst_ysize * 3;
      } else {
        pabyRawAlpha = pabyRaw1 + dst_xsize * dst_ysize * 1;
      }

      hBandAlpha = GDALGetMaskBand(hBand1);

      eErr = GDALRasterIO(hBandAlpha, GF_Read, src_xoff, src_yoff, src_xsize,
                          src_ysize, pabyRawAlpha, dst_xsize, dst_ysize,
                          GDT_Byte, 0, 0);

      if (eErr != CE_None) {
        msSetError(MS_IOERR, "GDALRasterIO() failed: %s", "drawGDAL()",
                   CPLGetLastErrorMsg());
        if (hColorMap != NULL)
          GDALDestroyColorTable(hColorMap);
        free(pabyRaw1);
        free(pbIsNoDataBuffer);
        return -1;
      }

      /* In case the mask is not an alpha channel, expand values of 1 to 255, */
      /* so we can deal as it was an alpha band afterwards */
      if ((nMaskFlags & GMF_ALPHA) == 0) {
        for (i = 0; i < dst_xsize * dst_ysize; i++)
          if (pabyRawAlpha[i])
            pabyRawAlpha[i] = 255;
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Single band plus colormap and alpha to truecolor. (RB)          */
  /* -------------------------------------------------------------------- */
  if (hBand2 == NULL && rb->type == MS_BUFFER_BYTE_RGBA && hBandAlpha != NULL) {
    assert(cmap_set);

    int k = 0;
    for (i = dst_yoff; i < dst_yoff + dst_ysize; i++) {
      for (int j = dst_xoff; j < dst_xoff + dst_xsize; j++, k++) {
        int src_pixel, src_alpha, cmap_alpha, merged_alpha;
        if (SKIP_MASK(j, i) || (pbIsNoDataBuffer && pbIsNoDataBuffer[k]))
          continue;

        src_pixel = pabyRaw1[k];
        src_alpha = pabyRawAlpha[k];
        cmap_alpha = rb_cmap[3][src_pixel];

        merged_alpha = (src_alpha * cmap_alpha) / 255;

        if (merged_alpha < 2)
          /* do nothing - transparent */;
        else if (merged_alpha > 253) {
          RB_SET_PIXEL(rb, j, i, rb_cmap[0][src_pixel], rb_cmap[1][src_pixel],
                       rb_cmap[2][src_pixel], cmap_alpha);
        } else {
          RB_MIX_PIXEL(rb, j, i, rb_cmap[0][src_pixel], rb_cmap[1][src_pixel],
                       rb_cmap[2][src_pixel], merged_alpha);
        }
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Single band plus colormap (no alpha) to truecolor (RB)          */
  /* -------------------------------------------------------------------- */
  else if (hBand2 == NULL && rb->type == MS_BUFFER_BYTE_RGBA) {
    assert(cmap_set);

    int k = 0;
    for (i = dst_yoff; i < dst_yoff + dst_ysize; i++) {
      for (int j = dst_xoff; j < dst_xoff + dst_xsize; j++, k++) {
        int src_pixel = pabyRaw1[k];
        if (SKIP_MASK(j, i) || (pbIsNoDataBuffer && pbIsNoDataBuffer[k]))
          continue;

        if (rb_cmap[3][src_pixel] > 253) {
          RB_SET_PIXEL(rb, j, i, rb_cmap[0][src_pixel], rb_cmap[1][src_pixel],
                       rb_cmap[2][src_pixel], rb_cmap[3][src_pixel]);
        } else if (rb_cmap[3][src_pixel] > 1) {
          RB_MIX_PIXEL(rb, j, i, rb_cmap[0][src_pixel], rb_cmap[1][src_pixel],
                       rb_cmap[2][src_pixel], rb_cmap[3][src_pixel]);
        }
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Input is 3 band RGB.  Alpha blending is mixed into the loop     */
  /*      since this case is less commonly used and has lots of other     */
  /*      overhead. (RB)                                                  */
  /* -------------------------------------------------------------------- */
  else if (hBand3 != NULL && rb->type == MS_BUFFER_BYTE_RGBA) {
    int k = 0;
    for (i = dst_yoff; i < dst_yoff + dst_ysize; i++) {
      for (int j = dst_xoff; j < dst_xoff + dst_xsize; j++, k++) {
        if (SKIP_MASK(j, i) || (pbIsNoDataBuffer && pbIsNoDataBuffer[k]))
          continue;
        if (MS_VALID_COLOR(layer->offsite) &&
            pabyRaw1[k] == layer->offsite.red &&
            pabyRaw2[k] == layer->offsite.green &&
            pabyRaw3[k] == layer->offsite.blue)
          continue;

        if (bHaveRGBNoData && pabyRaw1[k] == nNoData1 &&
            pabyRaw2[k] == nNoData2 && pabyRaw3[k] == nNoData3)
          continue;

        if (pabyRawAlpha == NULL || pabyRawAlpha[k] == 255) {
          RB_SET_PIXEL(rb, j, i, pabyRaw1[k], pabyRaw2[k], pabyRaw3[k], 255);
        } else if (pabyRawAlpha[k] != 0) {
          RB_MIX_PIXEL(rb, j, i, pabyRaw1[k], pabyRaw2[k], pabyRaw3[k],
                       pabyRawAlpha[k]);
        }
      }
    }
  }

  /*
  ** Cleanup
  */
  free(pabyRaw1);
  free(pbIsNoDataBuffer);

  if (hColorMap != NULL)
    GDALDestroyColorTable(hColorMap);

  return 0;
}

/************************************************************************/
/*                          ParseDefaultLUT()                           */
/************************************************************************/

static int ParseDefaultLUT(const char *lut_def, GByte *lut, int nMaxValIn)

{
  const char *lut_read;
  int last_in = 0, last_out = 0, all_done = FALSE;

  /* -------------------------------------------------------------------- */
  /*      Parse definition, applying to lut on the fly.                   */
  /* -------------------------------------------------------------------- */
  lut_read = lut_def;
  while (!all_done) {
    int this_in = 0, this_out = 0;
    int lut_i;

    while (isspace(*lut_read))
      lut_read++;

    /* if we are at end, assume nMaxValIn:255 */
    if (*lut_read == '\0') {
      all_done = TRUE;
      if (last_in != nMaxValIn) {
        this_in = nMaxValIn;
        this_out = 255;
      }
    }

    /* otherwise read "in:out", and skip past */
    else {
      this_in = atoi(lut_read);
      while (*lut_read != ':' && *lut_read)
        lut_read++;
      if (*lut_read == ':')
        lut_read++;
      while (isspace(*lut_read))
        lut_read++;
      this_out = atoi(lut_read);
      while (*lut_read && *lut_read != ',' && *lut_read != '\n')
        lut_read++;
      if (*lut_read == ',' || *lut_read == '\n')
        lut_read++;
      while (isspace(*lut_read))
        lut_read++;
    }

    this_in = MS_MAX(0, MS_MIN(nMaxValIn, this_in));
    this_out = MS_MAX(0, MS_MIN(255, this_out));

    /* apply linear values from last in:out to this in:out */
    for (lut_i = last_in; lut_i <= this_in; lut_i++) {
      double ratio;

      if (last_in == this_in)
        ratio = 1.0;
      else
        ratio = (lut_i - last_in) / (double)(this_in - last_in);

      lut[lut_i] =
          (int)floor(((1.0 - ratio) * last_out + ratio * this_out) + 0.5);
    }

    last_in = this_in;
    last_out = this_out;
  }

  return 0;
}

/************************************************************************/
/*                          LutFromGimpLine()                           */
/************************************************************************/

static int LutFromGimpLine(char *lut_line, GByte *lut)

{
  char wrkLUTDef[1000];
  int i, count = 0;
  char **tokens;

  /* cleanup white space at end of line (DOS LF, etc) */
  i = strlen(lut_line) - 1;
  while (i > 0 && isspace(lut_line[i]))
    lut_line[i--] = '\0';

  while (*lut_line == 10 || *lut_line == 13)
    lut_line++;

  /* tokenize line */
  tokens = CSLTokenizeString(lut_line);
  if (CSLCount(tokens) != 17 * 2) {
    CSLDestroy(tokens);
    msSetError(MS_MISCERR, "GIMP curve file appears corrupt.",
               "LutFromGimpLine()");
    return -1;
  }

  /* Convert to our own format */
  wrkLUTDef[0] = '\0';
  for (i = 0; i < 17; i++) {
    if (atoi(tokens[i * 2]) >= 0) {
      if (count++ > 0)
        strlcat(wrkLUTDef, ",", sizeof(wrkLUTDef));

      snprintf(wrkLUTDef + strlen(wrkLUTDef),
               sizeof(wrkLUTDef) - strlen(wrkLUTDef), "%s:%s", tokens[i * 2],
               tokens[i * 2 + 1]);
    }
  }

  CSLDestroy(tokens);

  return ParseDefaultLUT(wrkLUTDef, lut, 255);
}

/************************************************************************/
/*                            ParseGimpLUT()                            */
/*                                                                      */
/*      Parse a Gimp style LUT.                                         */
/************************************************************************/

static int ParseGimpLUT(const char *lut_def, GByte *lut, int iColorIndex)

{
  int i;
  GByte lutValue[256];
  GByte lutColorBand[256];
  char **lines = CSLTokenizeStringComplex(lut_def, "\n", FALSE, FALSE);

  if (!EQUALN(lines[0], "# GIMP Curves File", 18) || CSLCount(lines) < 6) {
    msSetError(MS_MISCERR, "GIMP curve file appears corrupt.",
               "ParseGimpLUT()");
    CSLDestroy(lines);
    return -1;
  }

  /*
   * Convert the overall curve, and the color band specific curve into LUTs.
   */
  if (LutFromGimpLine(lines[1], lutValue) != 0 ||
      LutFromGimpLine(lines[iColorIndex + 1], lutColorBand) != 0) {
    CSLDestroy(lines);
    return -1;
  }
  CSLDestroy(lines);

  /*
   * Compose the two luts as if the raw value passed through the color band
   * specific lut, and then the general value lut.  Usually one or the
   * other will be the identity mapping, but not always.
   */

  for (i = 0; i < 256; i++) {
    lut[i] = lutValue[lutColorBand[i]];
  }

  return 0;
}

/************************************************************************/
/*                              LoadLUT()                               */
/*                                                                      */
/*      Load a LUT according to RFC 21.                                 */
/************************************************************************/

static int LoadLUT(layerObj *layer, int iColorIndex, char **ppszLutDef)

{
  const char *lut_def;
  char key[20], lut_def_fromfile[2500];

  *ppszLutDef = NULL;

  /* -------------------------------------------------------------------- */
  /*      Get lut specifier from processing directives.  Do nothing if    */
  /*      none are found.                                                 */
  /* -------------------------------------------------------------------- */
  sprintf(key, "LUT_%d", iColorIndex);
  lut_def = msLayerGetProcessingKey(layer, key);
  if (lut_def == NULL)
    lut_def = msLayerGetProcessingKey(layer, "LUT");
  if (lut_def == NULL)
    return 0;

  /* -------------------------------------------------------------------- */
  /*      Does this look like a file?  If so, read it into memory.        */
  /* -------------------------------------------------------------------- */
  if (strspn(lut_def, "0123456789:, ") != strlen(lut_def)) {
    FILE *fp;
    char path[MS_MAXPATHLEN];
    int len;
    msBuildPath(path, layer->map->mappath, lut_def);
    fp = fopen(path, "rb");
    if (fp == NULL) {
      msSetError(MS_IOERR, "Failed to open LUT file '%s'.", "drawGDAL()", path);
      return -1;
    }

    len = fread(lut_def_fromfile, 1, sizeof(lut_def_fromfile), fp);
    fclose(fp);

    if (len == sizeof(lut_def_fromfile)) {
      msSetError(MS_IOERR,
                 "LUT definition from file %s longer than maximum buffer size "
                 "(%d bytes).",
                 "drawGDAL()", path, (int)sizeof(lut_def_fromfile));
      return -1;
    }

    lut_def_fromfile[len] = '\0';

    lut_def = lut_def_fromfile;
  }

  *ppszLutDef = msStrdup(lut_def);

  return 0;
}

/************************************************************************/
/*                              FreeLUTs()                              */
/************************************************************************/

static void FreeLUTs(char **apszLUTs) {
  int i;
  for (i = 0; i < 4; i++)
    msFree(apszLUTs[i]);
  msFree(apszLUTs);
}

/************************************************************************/
/*                            LoadLUTs()                                */
/*                                                                      */
/* Return an array of 4 strings (some possibly NULL) with loaded LUTs   */
/* or NULL in case of failure.                                          */
/************************************************************************/

static char **LoadLUTs(layerObj *layer, int band_count) {
  int i;
  char **apszLUTs;

  assert(band_count <= 4);

  apszLUTs = (char **)msSmallCalloc(4, sizeof(char *));
  for (i = 0; i < band_count; i++) {
    if (LoadLUT(layer, i + 1, &apszLUTs[i]) != 0) {
      FreeLUTs(apszLUTs);
      return NULL;
    }
  }

  return apszLUTs;
}

/************************************************************************/
/*                  GetDataTypeAppropriateForLUTS()                     */
/*                                                                      */
/*      This does a quick examination of the LUT strings to determine   */
/*      if they have input values > 255, in which case the raster data  */
/*      must be queries on 16-bits.                                     */
/************************************************************************/

static GDALDataType GetDataTypeAppropriateForLUTS(char **apszLUTs) {
  GDALDataType eDT = GDT_Byte;
  int i;
  for (i = 0; i < 4; i++) {
    const char *pszLastTuple;
    int nLastInValue;
    if (apszLUTs[i] == NULL)
      continue;
    if (EQUALN(apszLUTs[i], "# GIMP", 6))
      continue;
    /* Find last in:out tuple in string */
    pszLastTuple = strrchr(apszLUTs[i], ',');
    if (pszLastTuple == NULL)
      pszLastTuple = apszLUTs[i];
    else
      pszLastTuple++;
    while (*pszLastTuple == ' ')
      pszLastTuple++;
    nLastInValue = atoi(pszLastTuple);
    if (nLastInValue > 255) {
      eDT = GDT_UInt16;
      break;
    }
  }
  return eDT;
}

/************************************************************************/
/*                              ApplyLUT()                              */
/************************************************************************/

static int ApplyLUT(int iColorIndex, const char *lut_def, const void *pInBuffer,
                    GDALDataType eDT, GByte *pabyOutBuffer, int nPixelCount) {
  int i, err;
  GByte byteLut[256];
  GByte *uint16Lut = NULL;

  assert(eDT == GDT_Byte || eDT == GDT_UInt16);

  if (lut_def == NULL) {
    if (pInBuffer != pabyOutBuffer) {
      GDALCopyWords((void *)pInBuffer, eDT, GDALGetDataTypeSize(eDT) / 8,
                    pabyOutBuffer, GDT_Byte, 1, nPixelCount);
    }
    return 0;
  }

  /* -------------------------------------------------------------------- */
  /*      Parse the LUT description.                                      */
  /* -------------------------------------------------------------------- */
  if (EQUALN(lut_def, "# GIMP", 6)) {
    if (eDT != GDT_Byte) {
      msSetError(MS_MISCERR, "Cannot apply a GIMP LUT on a 16-bit buffer",
                 "ApplyLUT()");
      return -1;
    }
    err = ParseGimpLUT(lut_def, byteLut, iColorIndex);
  } else {
    if (eDT == GDT_Byte)
      err = ParseDefaultLUT(lut_def, byteLut, 255);
    else {
      uint16Lut = (GByte *)malloc(65536);
      if (uint16Lut == NULL) {
        msSetError(MS_MEMERR, "Cannot allocate 16-bit LUT", "ApplyLUT()");
        return -1;
      }
      err = ParseDefaultLUT(lut_def, uint16Lut, 65535);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Apply LUT.                                                      */
  /* -------------------------------------------------------------------- */
  if (eDT == GDT_Byte) {
    for (i = 0; i < nPixelCount; i++)
      pabyOutBuffer[i] = byteLut[((GByte *)pInBuffer)[i]];
  } else {
    for (i = 0; i < nPixelCount; i++)
      pabyOutBuffer[i] = uint16Lut[((GUInt16 *)pInBuffer)[i]];
    free(uint16Lut);
  }

  return err;
}

/************************************************************************/
/*                           LoadGDALImages()                           */
/*                                                                      */
/*      This call will load and process 1-4 bands of input for the      */
/*      selected rectangle, loading the result into the passed 8bit     */
/*      buffer.  The processing options include scaling.                */
/************************************************************************/

static int LoadGDALImages(GDALDatasetH hDS, int band_numbers[4], int band_count,
                          layerObj *layer, int src_xoff, int src_yoff,
                          int src_xsize, int src_ysize, GByte *pabyWholeBuffer,
                          int dst_xsize, int dst_ysize, int *pbHaveRGBNoData,
                          int *pnNoData1, int *pnNoData2, int *pnNoData3,
                          bool *pbScaled, double *pdfScaleMin,
                          double *pdfScaleRatio, bool **ppbIsNoDataBuffer)

{
  int iColorIndex, result_code = 0;
  CPLErr eErr;
  float *pafWholeRawData;
  char **papszLUTs;

  *pbScaled = false;
  *pdfScaleMin = 0;
  *pdfScaleRatio = 0;
  *ppbIsNoDataBuffer = NULL;

  /* -------------------------------------------------------------------- */
  /*      If we have no alpha band, but we do have three input            */
  /*      bands, then check for nodata values.  If we only have one       */
  /*      input band, then nodata will already have been adderssed as     */
  /*      part of the real or manufactured color table.                   */
  /* -------------------------------------------------------------------- */
  if (band_count == 3) {
    *pnNoData1 = (int)msGetGDALNoDataValue(
        layer, GDALGetRasterBand(hDS, band_numbers[0]), pbHaveRGBNoData);

    if (*pbHaveRGBNoData)
      *pnNoData2 = (int)msGetGDALNoDataValue(
          layer, GDALGetRasterBand(hDS, band_numbers[1]), pbHaveRGBNoData);
    if (*pbHaveRGBNoData)
      *pnNoData3 = (int)msGetGDALNoDataValue(
          layer, GDALGetRasterBand(hDS, band_numbers[2]), pbHaveRGBNoData);
  }

  /* -------------------------------------------------------------------- */
  /*      Are we doing a simple, non-scaling case?  If so, read directly  */
  /*      and return.                                                     */
  /* -------------------------------------------------------------------- */
  if (CSLFetchNameValue(layer->processing, "SCALE") == NULL &&
      CSLFetchNameValue(layer->processing, "SCALE_1") == NULL &&
      CSLFetchNameValue(layer->processing, "SCALE_2") == NULL &&
      CSLFetchNameValue(layer->processing, "SCALE_3") == NULL &&
      CSLFetchNameValue(layer->processing, "SCALE_4") == NULL) {

    GDALDataType eDT;
    GUInt16 *panBuffer = NULL;
    void *pBuffer;

    papszLUTs = LoadLUTs(layer, band_count);
    if (papszLUTs == NULL) {
      return -1;
    }

    eDT = GetDataTypeAppropriateForLUTS(papszLUTs);
    if (eDT == GDT_UInt16) {
      panBuffer = (GUInt16 *)malloc(sizeof(GUInt16) * dst_xsize * dst_ysize *
                                    band_count);

      if (panBuffer == NULL) {
        msSetError(MS_MEMERR,
                   "Allocating work uint16 image of size %dx%dx%d failed.",
                   "msDrawRasterLayerGDAL()", dst_xsize, dst_ysize, band_count);
        FreeLUTs(papszLUTs);
        return -1;
      }
      pBuffer = panBuffer;
    } else
      pBuffer = pabyWholeBuffer;

    eErr = GDALDatasetRasterIO(hDS, GF_Read, src_xoff, src_yoff, src_xsize,
                               src_ysize, pBuffer, dst_xsize, dst_ysize, eDT,
                               band_count, band_numbers, 0, 0, 0);

    if (eErr != CE_None) {
      msSetError(MS_IOERR, "GDALDatasetRasterIO() failed: %s", "drawGDAL()",
                 CPLGetLastErrorMsg());
      FreeLUTs(papszLUTs);
      msFree(panBuffer);
      return -1;
    }

    for (iColorIndex = 0; iColorIndex < band_count && result_code == 0;
         iColorIndex++) {
      result_code =
          ApplyLUT(iColorIndex + 1, papszLUTs[iColorIndex],
                   (GByte *)pBuffer + dst_xsize * dst_ysize * iColorIndex *
                                          (GDALGetDataTypeSize(eDT) / 8),
                   eDT, pabyWholeBuffer + dst_xsize * dst_ysize * iColorIndex,
                   dst_xsize * dst_ysize);
    }

    FreeLUTs(papszLUTs);
    msFree(panBuffer);
    return result_code;
  }

  /* -------------------------------------------------------------------- */
  /*      Disable use of nodata if we are doing scaling.                  */
  /* -------------------------------------------------------------------- */
  *pbHaveRGBNoData = FALSE;

  /* -------------------------------------------------------------------- */
  /*      We need to do some scaling.  Will load into either a 16bit      */
  /*      unsigned or a floating point buffer depending on the source     */
  /*      data.  We offer a special case for 16U data because it is       */
  /*      common and it is a substantial win to avoid a lot of floating    */
  /*      point operations on it.                                         */
  /* -------------------------------------------------------------------- */
  /* TODO */

  /* -------------------------------------------------------------------- */
  /*      Allocate the raw imagery buffer, and load into it (band         */
  /*      interleaved).                                                   */
  /* -------------------------------------------------------------------- */
  pafWholeRawData =
      (float *)malloc(sizeof(float) * dst_xsize * dst_ysize * band_count);

  if (pafWholeRawData == NULL) {
    msSetError(MS_MEMERR,
               "Allocating work float image of size %dx%dx%d failed.",
               "msDrawRasterLayerGDAL()", dst_xsize, dst_ysize, band_count);
    return -1;
  }

  eErr = GDALDatasetRasterIO(hDS, GF_Read, src_xoff, src_yoff, src_xsize,
                             src_ysize, pafWholeRawData, dst_xsize, dst_ysize,
                             GDT_Float32, band_count, band_numbers, 0, 0, 0);

  if (eErr != CE_None) {
    msSetError(MS_IOERR, "GDALDatasetRasterIO() failed: %s", "drawGDAL()",
               CPLGetLastErrorMsg());

    free(pafWholeRawData);
    return -1;
  }

  papszLUTs = LoadLUTs(layer, band_count);
  if (papszLUTs == NULL) {
    free(pafWholeRawData);
    return -1;
  }

  if (GetDataTypeAppropriateForLUTS(papszLUTs) != GDT_Byte) {
    msDebug("LoadGDALImage(%s): One of the LUT contains a input value > 255.\n"
            "This is not properly supported in combination with SCALE\n",
            layer->name);
  }

  /* -------------------------------------------------------------------- */
  /*      Fetch the scale processing option.                              */
  /* -------------------------------------------------------------------- */
  for (iColorIndex = 0; iColorIndex < band_count; iColorIndex++) {
    unsigned char *pabyBuffer;
    double dfScaleMin = 0.0, dfScaleMax = 255.0, dfScaleRatio, dfNoDataValue;
    const char *pszScaleInfo;
    float *pafRawData;
    int nPixelCount = dst_xsize * dst_ysize, i, bGotNoData = FALSE;
    GDALRasterBandH hBand = GDALGetRasterBand(hDS, band_numbers[iColorIndex]);
    pszScaleInfo = CSLFetchNameValue(layer->processing, "SCALE");
    if (pszScaleInfo == NULL) {
      char szBandScalingName[20];

      sprintf(szBandScalingName, "SCALE_%d", iColorIndex + 1);
      pszScaleInfo = CSLFetchNameValue(layer->processing, szBandScalingName);
    }

    if (pszScaleInfo != NULL) {
      char **papszTokens;

      papszTokens = CSLTokenizeStringComplex(pszScaleInfo, " ,", FALSE, FALSE);
      if (CSLCount(papszTokens) == 1 && EQUAL(papszTokens[0], "AUTO")) {
        dfScaleMin = dfScaleMax = 0.0;
      } else if (CSLCount(papszTokens) != 2) {
        free(pafWholeRawData);
        CSLDestroy(papszTokens);
        FreeLUTs(papszLUTs);
        msSetError(MS_MISCERR,
                   "SCALE PROCESSING option unparsable for layer %s.",
                   "msDrawGDAL()", layer->name);
        return -1;
      } else {
        dfScaleMin = atof(papszTokens[0]);
        dfScaleMax = atof(papszTokens[1]);
      }
      CSLDestroy(papszTokens);
    }

    /* -------------------------------------------------------------------- */
    /*      If we are using autoscaling, then compute the max and min       */
    /*      now.  Perhaps we should eventually honour the offsite value     */
    /*      as a nodata value, or get it from GDAL.                         */
    /* -------------------------------------------------------------------- */
    pafRawData = pafWholeRawData + iColorIndex * dst_xsize * dst_ysize;

    dfNoDataValue = msGetGDALNoDataValue(layer, hBand, &bGotNoData);

    /* we force assignment to a float rather than letting pafRawData[i]
       get promoted to double later to avoid float precision issues. */
    float fNoDataValue = (float)dfNoDataValue;

    if (dfScaleMin == dfScaleMax) {
      int bMinMaxSet = 0;

      /* we force assignment to a float rather than letting pafRawData[i]
         get promoted to double later to avoid float precision issues. */
      float fNoDataValue = (float)dfNoDataValue;

      for (i = 0; i < nPixelCount; i++) {
        if (bGotNoData && IsNoData(pafRawData[i], fNoDataValue))
          continue;

        if (CPLIsNan(pafRawData[i]))
          continue;

        if (!bMinMaxSet) {
          dfScaleMin = dfScaleMax = pafRawData[i];
          bMinMaxSet = TRUE;
        }

        dfScaleMin = MS_MIN(dfScaleMin, pafRawData[i]);
        dfScaleMax = MS_MAX(dfScaleMax, pafRawData[i]);
      }

      if (dfScaleMin == dfScaleMax)
        dfScaleMax = dfScaleMin + 1.0;
    }

    if (layer->debug > 0)
      msDebug("msDrawGDAL(%s): scaling to 8bit, src range=%g,%g\n", layer->name,
              dfScaleMin, dfScaleMax);

    /* -------------------------------------------------------------------- */
    /*      Now process the data.                                           */
    /* -------------------------------------------------------------------- */
    dfScaleRatio = 256.0 / (dfScaleMax - dfScaleMin);
    pabyBuffer = pabyWholeBuffer + iColorIndex * nPixelCount;

    if (iColorIndex == 0 && bGotNoData)
      *ppbIsNoDataBuffer = (bool *)calloc(nPixelCount, 1);

    for (i = 0; i < nPixelCount; i++) {
      float fScaledValue;

      if (bGotNoData && IsNoData(pafRawData[i], fNoDataValue)) {
        if (iColorIndex == 0)
          (*ppbIsNoDataBuffer)[i] = true;
        continue;
      }

      fScaledValue = (float)((pafRawData[i] - dfScaleMin) * dfScaleRatio);

      if (fScaledValue < 0.0)
        pabyBuffer[i] = 0;
      else if (fScaledValue > 255.0)
        pabyBuffer[i] = 255;
      else
        pabyBuffer[i] = (int)fScaledValue;
    }

    if (iColorIndex == 0) {
      *pbScaled = true;
      *pdfScaleMin = dfScaleMin;
      *pdfScaleRatio = dfScaleRatio;
    }

    /* -------------------------------------------------------------------- */
    /*      Report a warning if NODATA keyword was applied.  We are         */
    /*      unable to utilize it since we can't return any pixels marked    */
    /*      as nodata from this function.  Need to fix someday.             */
    /* -------------------------------------------------------------------- */
    if (bGotNoData)
      msDebug("LoadGDALImage(%s): NODATA value %g in GDAL\n"
              "file or PROCESSING directive largely ignored.  Not yet fully "
              "supported for\n"
              "unclassified scaled data.  The NODATA value is excluded from "
              "auto-scaling\n"
              "min/max computation, but will not be transparent.\n",
              layer->name, dfNoDataValue);

    /* -------------------------------------------------------------------- */
    /*      Apply LUT if there is one.                                      */
    /* -------------------------------------------------------------------- */
    result_code = ApplyLUT(iColorIndex + 1, papszLUTs[iColorIndex], pabyBuffer,
                           GDT_Byte, pabyBuffer, dst_xsize * dst_ysize);
    if (result_code == -1) {
      free(pafWholeRawData);
      FreeLUTs(papszLUTs);
      return result_code;
    }
  }

  free(pafWholeRawData);
  FreeLUTs(papszLUTs);

  return result_code;
}

/************************************************************************/
/*                       msGetGDALGeoTransform()                        */
/*                                                                      */
/*      Cover function that tries GDALGetGeoTransform(), a world        */
/*      file or OWS extents.  It is assumed that TLOCK_GDAL is held     */
/*      before this function is called.                                 */
/************************************************************************/

int msGetGDALGeoTransform(GDALDatasetH hDS, mapObj *map, layerObj *layer,
                          double *padfGeoTransform)

{
  const char *extent_priority = NULL;
  const char *value;
  const char *gdalDesc;
  const char *fullPath;
  char *fileExtension = NULL;
  char szPath[MS_MAXPATHLEN];
  int fullPathLen;
  int success = 0;
  rectObj rect;

  /* -------------------------------------------------------------------- */
  /*      some GDAL drivers (ie. GIF) don't set geotransform on failure.  */
  /* -------------------------------------------------------------------- */
  padfGeoTransform[0] = 0.0;
  padfGeoTransform[1] = 1.0;
  padfGeoTransform[2] = 0.0;
  padfGeoTransform[3] = GDALGetRasterYSize(hDS);
  padfGeoTransform[4] = 0.0;
  padfGeoTransform[5] = -1.0;

  /* -------------------------------------------------------------------- */
  /*      Do we want to override GDAL with a worldfile if one is present? */
  /* -------------------------------------------------------------------- */
  extent_priority = CSLFetchNameValue(layer->processing, "EXTENT_PRIORITY");

  if (extent_priority != NULL && EQUALN(extent_priority, "WORLD", 5)) {
    /* is there a user configured worldfile? */
    value = CSLFetchNameValue(layer->processing, "WORLDFILE");

    if (value != NULL) {
      /* at this point, fullPath may be a filePath or path only */
      fullPath = msBuildPath(szPath, map->mappath, value);

      /* fullPath is a path only, so let's append the dataset filename */
      if (strrchr(szPath, '.') == NULL) {
        fullPathLen = strlen(fullPath);
        gdalDesc = msStripPath((char *)GDALGetDescription(hDS));

        if ((fullPathLen + strlen(gdalDesc)) < MS_MAXPATHLEN) {
          strcpy((char *)(fullPath + fullPathLen), gdalDesc);
        } else {
          msDebug(
              "Path length to alternative worldfile exceeds MS_MAXPATHLEN.\n");
          fullPath = NULL;
        }
      }
      /* fullPath has a filename included, so get the extension */
      else {
        fileExtension = msStrdup(strrchr(szPath, '.') + 1);
      }
    }
    /* common behavior with worldfile generated from basename + .wld */
    else {
      fullPath = GDALGetDescription(hDS);
      fileExtension = msStrdup("wld");
    }

    if (fullPath)
      success = GDALReadWorldFile(fullPath, fileExtension, padfGeoTransform);

    if (success && layer->debug >= MS_DEBUGLEVEL_VVV) {
      msDebug("Worldfile location: %s.\n", fullPath);
    } else if (layer->debug >= MS_DEBUGLEVEL_VVV) {
      msDebug("Failed using worldfile location: %s.\n", fullPath);
    }

    msFree(fileExtension);

    if (success)
      return MS_SUCCESS;
  }

  /* -------------------------------------------------------------------- */
  /*      Try GDAL.                                                       */
  /*                                                                      */
  /*      Make sure that ymax is always at the top, and ymin at the       */
  /*      bottom ... that is flip any files without the usual             */
  /*      orientation.  This is intended to enable display of "raw"       */
  /*      files with no coordinate system otherwise they break down in    */
  /*      many ways.                                                      */
  /* -------------------------------------------------------------------- */
  if (GDALGetGeoTransform(hDS, padfGeoTransform) == CE_None) {
    if (padfGeoTransform[5] == 1.0 && padfGeoTransform[3] == 0.0) {
      padfGeoTransform[5] = -1.0;
      padfGeoTransform[3] = GDALGetRasterYSize(hDS);
    }

    return MS_SUCCESS;
  }

  /* -------------------------------------------------------------------- */
  /*      Try worldfile.                                                  */
  /* -------------------------------------------------------------------- */
  if (GDALGetDescription(hDS) != NULL &&
      GDALReadWorldFile(GDALGetDescription(hDS), "wld", padfGeoTransform)) {
    return MS_SUCCESS;
  }

  /* -------------------------------------------------------------------- */
  /*      Do we have the extent keyword on the layer?  We only use        */
  /*      this if we have a single raster associated with the layer as    */
  /*      opposed to a tile index.                                        */
  /*                                                                      */
  /*      Arguably this ought to take priority over all the other         */
  /*      stuff.                                                          */
  /* -------------------------------------------------------------------- */
  if (MS_VALID_EXTENT(layer->extent) && layer->data != NULL) {
    rect = layer->extent;

    padfGeoTransform[0] = rect.minx;
    padfGeoTransform[1] =
        (rect.maxx - rect.minx) / (double)GDALGetRasterXSize(hDS);
    padfGeoTransform[2] = 0;
    padfGeoTransform[3] = rect.maxy;
    padfGeoTransform[4] = 0;
    padfGeoTransform[5] =
        (rect.miny - rect.maxy) / (double)GDALGetRasterYSize(hDS);

    return MS_SUCCESS;
  }

  /* -------------------------------------------------------------------- */
  /*      We didn't find any info ... use the default.                    */
  /*      Reset our default geotransform.  GDALGetGeoTransform() may      */
  /*      have altered it even if GDALGetGeoTransform() failed.           */
  /* -------------------------------------------------------------------- */
  padfGeoTransform[0] = 0.0;
  padfGeoTransform[1] = 1.0;
  padfGeoTransform[2] = 0.0;
  padfGeoTransform[3] = GDALGetRasterYSize(hDS);
  padfGeoTransform[4] = 0.0;
  padfGeoTransform[5] = -1.0;

  return MS_FAILURE;
}

/************************************************************************/
/*                   msDrawRasterLayerGDAL_RawMode()                    */
/*                                                                      */
/*      Handle the loading and application of data in rawmode.          */
/************************************************************************/

static int
msDrawRasterLayerGDAL_RawMode(mapObj *map, layerObj *layer, imageObj *image,
                              GDALDatasetH hDS, int src_xoff, int src_yoff,
                              int src_xsize, int src_ysize, int dst_xoff,
                              int dst_yoff, int dst_xsize, int dst_ysize)

{
  void *pBuffer;
  GDALDataType eDataType;
  int *band_list, band_count;
  int i, j, k, band;
  CPLErr eErr;
  float *f_nodatas = NULL;
  unsigned char *b_nodatas = NULL;
  GInt16 *i_nodatas = NULL;
  int got_nodata = FALSE;
  rasterBufferObj *mask_rb = NULL;
  rasterBufferObj s_mask_rb;
  if (layer->mask) {
    int ret;
    layerObj *maskLayer = GET_LAYER(map, msGetLayerIndex(map, layer->mask));
    mask_rb = &s_mask_rb;
    memset(mask_rb, 0, sizeof(s_mask_rb));
    ret = MS_IMAGE_RENDERER(maskLayer->maskimage)
              ->getRasterBufferHandle(maskLayer->maskimage, mask_rb);
    if (ret != MS_SUCCESS)
      return -1;
  }

  if (image->format->bands > 256) {
    msSetError(MS_IMGERR, "Too many bands (more than 256).",
               "msDrawRasterLayerGDAL_RawMode()");
    return -1;
  }

  /* -------------------------------------------------------------------- */
  /*      What data type do we need?                                      */
  /* -------------------------------------------------------------------- */
  if (image->format->imagemode == MS_IMAGEMODE_INT16)
    eDataType = GDT_Int16;
  else if (image->format->imagemode == MS_IMAGEMODE_FLOAT32)
    eDataType = GDT_Float32;
  else if (image->format->imagemode == MS_IMAGEMODE_BYTE)
    eDataType = GDT_Byte;
  else
    return -1;

  /* -------------------------------------------------------------------- */
  /*      Work out the band list.                                         */
  /* -------------------------------------------------------------------- */
  band_list = msGetGDALBandList(layer, hDS, image->format->bands, &band_count);
  if (band_list == NULL)
    return -1;

  if (band_count != image->format->bands) {
    free(band_list);
    msSetError(MS_IMGERR,
               "BANDS PROCESSING directive has wrong number of bands, expected "
               "%d, got %d.",
               "msDrawRasterLayerGDAL_RawMode()", image->format->bands,
               band_count);
    return -1;
  }

  /* -------------------------------------------------------------------- */
  /*      Do we have nodata values?                                       */
  /* -------------------------------------------------------------------- */
  f_nodatas = (float *)calloc(sizeof(float), band_count);
  if (f_nodatas == NULL) {
    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n",
               "msDrawRasterLayerGDAL_RawMode()", __FILE__, __LINE__,
               (unsigned int)(sizeof(float) * band_count));
    free(band_list);
    return -1;
  }

  if (band_count <= 3 &&
      (layer->offsite.red != -1 || layer->offsite.green != -1 ||
       layer->offsite.blue != -1)) {
    if (band_count > 0)
      f_nodatas[0] = layer->offsite.red;
    if (band_count > 1)
      f_nodatas[1] = layer->offsite.green;
    if (band_count > 2)
      f_nodatas[2] = layer->offsite.blue;
    got_nodata = TRUE;
  }

  if (!got_nodata) {
    got_nodata = TRUE;
    for (band = 0; band < band_count && got_nodata; band++) {
      f_nodatas[band] = msGetGDALNoDataValue(
          layer, GDALGetRasterBand(hDS, band_list[band]), &got_nodata);
    }
  }

  if (!got_nodata) {
    free(f_nodatas);
    f_nodatas = NULL;
  } else if (eDataType == GDT_Byte) {
    b_nodatas = (unsigned char *)f_nodatas;
    for (band = 0; band < band_count; band++)
      b_nodatas[band] = (unsigned char)f_nodatas[band];
  } else if (eDataType == GDT_Int16) {
    i_nodatas = (GInt16 *)f_nodatas;
    for (band = 0; band < band_count; band++)
      i_nodatas[band] = (GInt16)f_nodatas[band];
  }

  /* -------------------------------------------------------------------- */
  /*      Allocate buffer, and read data into it.                         */
  /* -------------------------------------------------------------------- */
  pBuffer = malloc(((size_t)dst_xsize) * dst_ysize * image->format->bands *
                   (GDALGetDataTypeSize(eDataType) / 8));
  if (pBuffer == NULL) {
    msSetError(MS_MEMERR, "Allocating work image of size %dx%d failed.",
               "msDrawRasterLayerGDAL()", dst_xsize, dst_ysize);
    free(band_list);
    free(f_nodatas);
    return -1;
  }

  eErr =
      GDALDatasetRasterIO(hDS, GF_Read, src_xoff, src_yoff, src_xsize,
                          src_ysize, pBuffer, dst_xsize, dst_ysize, eDataType,
                          image->format->bands, band_list, 0, 0, 0);
  free(band_list);

  if (eErr != CE_None) {
    msSetError(MS_IOERR, "GDALRasterIO() failed: %s",
               "msDrawRasterLayerGDAL_RawMode()", CPLGetLastErrorMsg());
    free(pBuffer);
    free(f_nodatas);
    return -1;
  }

  /* -------------------------------------------------------------------- */
  /*      Transfer the data to the imageObj.                              */
  /* -------------------------------------------------------------------- */
  k = 0;
  for (band = 0; band < image->format->bands; band++) {
    for (i = dst_yoff; i < dst_yoff + dst_ysize; i++) {
      if (image->format->imagemode == MS_IMAGEMODE_INT16) {
        for (j = dst_xoff; j < dst_xoff + dst_xsize; j++) {
          int off = j + i * image->width + band * image->width * image->height;
          int off_mask = j + i * image->width;

          if ((i_nodatas && ((GInt16 *)pBuffer)[k] == i_nodatas[band]) ||
              SKIP_MASK(j, i)) {
            k++;
            continue;
          }

          image->img.raw_16bit[off] = ((GInt16 *)pBuffer)[k++];
          MS_SET_BIT(image->img_mask, off_mask);
        }
      } else if (image->format->imagemode == MS_IMAGEMODE_FLOAT32) {
        for (j = dst_xoff; j < dst_xoff + dst_xsize; j++) {
          int off = j + i * image->width + band * image->width * image->height;
          int off_mask = j + i * image->width;

          if ((f_nodatas && ((float *)pBuffer)[k] == f_nodatas[band]) ||
              SKIP_MASK(j, i)) {
            k++;
            continue;
          }

          image->img.raw_float[off] = ((float *)pBuffer)[k++];
          MS_SET_BIT(image->img_mask, off_mask);
        }
      } else if (image->format->imagemode == MS_IMAGEMODE_BYTE) {
        for (j = dst_xoff; j < dst_xoff + dst_xsize; j++) {
          int off = j + i * image->width + band * image->width * image->height;
          int off_mask = j + i * image->width;

          if ((b_nodatas && ((unsigned char *)pBuffer)[k] == b_nodatas[band]) ||
              SKIP_MASK(j, i)) {
            k++;
            continue;
          }

          image->img.raw_byte[off] = ((unsigned char *)pBuffer)[k++];
          MS_SET_BIT(image->img_mask, off_mask);
        }
      }
    }
  }

  free(pBuffer);
  free(f_nodatas);

  return 0;
}

/************************************************************************/
/*              msDrawRasterLayerGDAL_16BitClassifcation()              */
/*                                                                      */
/*      Handle the rendering of rasters going through a 16bit           */
/*      classification lookup instead of the more common 8bit one.      */
/*                                                                      */
/*      Currently we are using one data path where we load the          */
/*      raster into a floating point buffer, and then scale to          */
/*      16bit.  Eventually we could add optimizations for some of       */
/*      the 16bit cases at the cost of some complication.               */
/************************************************************************/

static int msDrawRasterLayerGDAL_16BitClassification(
    mapObj *map, layerObj *layer, rasterBufferObj *rb, GDALRasterBandH hBand,
    int src_xoff, int src_yoff, int src_xsize, int src_ysize, int dst_xoff,
    int dst_yoff, int dst_xsize, int dst_ysize)

{
  float *pafRawData;
  double dfScaleMin = 0.0, dfScaleMax = 0.0, dfScaleRatio;
  int nPixelCount = dst_xsize * dst_ysize, i, nBucketCount = 0;
  GDALDataType eDataType;
  float fDataMin = 0.0, fDataMax = 255.0, fNoDataValue;
  const char *pszScaleInfo;
  const char *pszBuckets;
  int *cmap, c, j, k, bGotNoData = FALSE, bGotFirstValue;
  unsigned char *rb_cmap[4];
  CPLErr eErr;
  rasterBufferObj *mask_rb = NULL;
  rasterBufferObj s_mask_rb;
  int lastC;
  struct mstimeval starttime = {0}, endtime = {0};

  const char *pszClassifyScaled;
  int bClassifyScaled = FALSE;

  if (layer->mask) {
    int ret;
    layerObj *maskLayer = GET_LAYER(map, msGetLayerIndex(map, layer->mask));
    mask_rb = &s_mask_rb;
    memset(mask_rb, 0, sizeof(s_mask_rb));
    ret = MS_IMAGE_RENDERER(maskLayer->maskimage)
              ->getRasterBufferHandle(maskLayer->maskimage, mask_rb);
    if (ret != MS_SUCCESS)
      return -1;
  }

  assert(rb->type == MS_BUFFER_BYTE_RGBA);

  /* ==================================================================== */
  /*      Read the requested data in one gulp into a floating point       */
  /*      buffer.                                                         */
  /* ==================================================================== */
  pafRawData = (float *)malloc(sizeof(float) * dst_xsize * dst_ysize);
  if (pafRawData == NULL) {
    msSetError(MS_MEMERR, "Out of memory allocating working buffer.",
               "msDrawRasterLayerGDAL_16BitClassification()");
    return -1;
  }

  eErr = GDALRasterIO(hBand, GF_Read, src_xoff, src_yoff, src_xsize, src_ysize,
                      pafRawData, dst_xsize, dst_ysize, GDT_Float32, 0, 0);

  if (eErr != CE_None) {
    free(pafRawData);
    msSetError(MS_IOERR, "GDALRasterIO() failed: %s",
               "msDrawRasterLayerGDAL_16BitClassification()",
               CPLGetLastErrorMsg());
    return -1;
  }

  fNoDataValue = (float)msGetGDALNoDataValue(layer, hBand, &bGotNoData);

  /* ==================================================================== */
  /*      Determine scaling.                                              */
  /* ==================================================================== */
  eDataType = GDALGetRasterDataType(hBand);

  /* -------------------------------------------------------------------- */
  /*      Scan for absolute min/max of this block.                        */
  /* -------------------------------------------------------------------- */
  bGotFirstValue = FALSE;

  for (i = 1; i < nPixelCount; i++) {
    if (bGotNoData && IsNoData(pafRawData[i], fNoDataValue))
      continue;

    if (CPLIsNan(pafRawData[i]))
      continue;

    if (!bGotFirstValue) {
      fDataMin = fDataMax = pafRawData[i];
      bGotFirstValue = TRUE;
    } else {
      fDataMin = MS_MIN(fDataMin, pafRawData[i]);
      fDataMax = MS_MAX(fDataMax, pafRawData[i]);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Fetch the scale classification option.                          */
  /* -------------------------------------------------------------------- */
  pszClassifyScaled = CSLFetchNameValue(layer->processing, "CLASSIFY_SCALED");
  if (pszClassifyScaled != NULL)
    bClassifyScaled = CSLTestBoolean(pszClassifyScaled);

  /* -------------------------------------------------------------------- */
  /*      Fetch the scale processing option.                              */
  /* -------------------------------------------------------------------- */
  pszBuckets = CSLFetchNameValue(layer->processing, "SCALE_BUCKETS");
  pszScaleInfo = CSLFetchNameValue(layer->processing, "SCALE");

  if (pszScaleInfo != NULL) {
    char **papszTokens;

    papszTokens = CSLTokenizeStringComplex(pszScaleInfo, " ,", FALSE, FALSE);
    if (CSLCount(papszTokens) == 1 && EQUAL(papszTokens[0], "AUTO")) {
      dfScaleMin = dfScaleMax = 0.0;
    } else if (CSLCount(papszTokens) != 2) {
      free(pafRawData);
      CSLDestroy(papszTokens);
      msSetError(MS_MISCERR, "SCALE PROCESSING option unparsable for layer %s.",
                 "msDrawGDAL()", layer->name);
      return -1;
    } else {
      dfScaleMin = atof(papszTokens[0]);
      dfScaleMax = atof(papszTokens[1]);
    }
    CSLDestroy(papszTokens);
  }

  /* -------------------------------------------------------------------- */
  /*      Special integer cases for scaling.                              */
  /*                                                                      */
  /*      TODO: Treat Int32 and UInt32 case the same way *if* the min     */
  /*      and max are less than 65536 apart.                              */
  /* -------------------------------------------------------------------- */
  if (eDataType == GDT_Byte || eDataType == GDT_Int16 ||
      eDataType == GDT_UInt16) {
    if (pszScaleInfo == NULL) {
      dfScaleMin = fDataMin - 0.5;
      dfScaleMax = fDataMax + 0.5;
    }

    if (pszBuckets == NULL) {
      nBucketCount = (int)floor(fDataMax - fDataMin + 1.1);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      General case if no scaling values provided in mapfile.          */
  /* -------------------------------------------------------------------- */
  else if (dfScaleMin == 0.0 && dfScaleMax == 0.0) {
    double dfEpsilon = (fDataMax - fDataMin) / (65536 * 2);
    dfScaleMin = fDataMin - dfEpsilon;
    dfScaleMax = fDataMax + dfEpsilon;
  }

  /* -------------------------------------------------------------------- */
  /*      How many buckets?  Only set if we don't already have a value.   */
  /* -------------------------------------------------------------------- */
  if (pszBuckets == NULL) {
    if (nBucketCount == 0)
      nBucketCount = 65536;
  } else {
    nBucketCount = atoi(pszBuckets);
    if (nBucketCount < 2) {
      free(pafRawData);
      msSetError(
          MS_MISCERR,
          "SCALE_BUCKETS PROCESSING option is not a value of 2 or more: %s.",
          "msDrawRasterLayerGDAL_16BitClassification()", pszBuckets);
      return -1;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Compute scaling ratio.                                          */
  /* -------------------------------------------------------------------- */
  if (dfScaleMax == dfScaleMin)
    dfScaleMax = dfScaleMin + 1.0;

  dfScaleRatio = nBucketCount / (dfScaleMax - dfScaleMin);

  if (layer->debug > 0)
    msDebug("msDrawRasterGDAL_16BitClassification(%s):\n"
            "  scaling to %d buckets from range=%g,%g.\n",
            layer->name, nBucketCount, dfScaleMin, dfScaleMax);

  /* ==================================================================== */
  /*      Compute classification lookup table.                            */
  /* ==================================================================== */

  cmap = (int *)msSmallCalloc(sizeof(int), nBucketCount);
  rb_cmap[0] = (unsigned char *)msSmallCalloc(1, nBucketCount);
  rb_cmap[1] = (unsigned char *)msSmallCalloc(1, nBucketCount);
  rb_cmap[2] = (unsigned char *)msSmallCalloc(1, nBucketCount);
  rb_cmap[3] = (unsigned char *)msSmallCalloc(1, nBucketCount);

  if (layer->debug >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&starttime, NULL);
  }

  lastC = -1;
  for (i = 0; i < nBucketCount; i++) {
    double dfOriginalValue;

    cmap[i] = -1;

    // i = (int) ((dfOriginalValue - dfScaleMin) * dfScaleRatio+1)-1;
    dfOriginalValue = (i + 0.5) / dfScaleRatio + dfScaleMin;

    /* The creation of buckets takes a significant time when they are many, and
       many classes as well. When iterating over buckets, a faster strategy is
       to reuse first the last used class index. */
    if (bClassifyScaled == TRUE)
      c = msGetClass_FloatRGB_WithFirstClassToTry(layer, (float)i, -1, -1, -1,
                                                  lastC);
    else
      c = msGetClass_FloatRGB_WithFirstClassToTry(layer, (float)dfOriginalValue,
                                                  -1, -1, -1, lastC);

    lastC = c;
    if (c != -1) {
      int s;
      int styleindex = 0;

      /* change colour based on colour range? */
      if (MS_VALID_COLOR(layer->class[c] -> styles[0] -> mincolor)) {
        styleindex = -1;
        for (s = 0; s < layer->class[c] -> numstyles; s++) {
          if (MS_VALID_COLOR(layer->class[c] -> styles[s] -> mincolor) &&
                  MS_VALID_COLOR(layer->class[c] -> styles[s] -> maxcolor) &&
                  dfOriginalValue + 0.5 / dfScaleRatio >=
                      layer->class[c] -> styles[s]
              -> minvalue && dfOriginalValue - 0.5 / dfScaleRatio <=
                                 layer -> class[c] -> styles[s] -> maxvalue) {
            msValueToRange(layer->class[c] -> styles[s], dfOriginalValue,
                           MS_COLORSPACE_RGB);
            styleindex = s;
            break;
          }
        }
      }
      if (styleindex >= 0) {
        if (MS_TRANSPARENT_COLOR(
                layer->class[c] -> styles[styleindex] -> color)) {
          /* leave it transparent */
        } else if (MS_VALID_COLOR(
                       layer->class[c] -> styles[styleindex] -> color)) {
          /* use class color */
          rb_cmap[0][i] = layer->class[c]->styles[styleindex]->color.red;
          rb_cmap[1][i] = layer->class[c]->styles[styleindex]->color.green;
          rb_cmap[2][i] = layer->class[c]->styles[styleindex]->color.blue;
          rb_cmap[3][i] =
              (255 * layer->class[c] -> styles[styleindex] -> opacity / 100);
        }
      }
    }
  }

  if (layer->debug >= MS_DEBUGLEVEL_TUNING) {
    msGettimeofday(&endtime, NULL);
    msDebug(
        "msDrawRasterGDAL_16BitClassification() bucket creation time: %.3fs\n",
        (endtime.tv_sec + endtime.tv_usec / 1.0e6) -
            (starttime.tv_sec + starttime.tv_usec / 1.0e6));
  }

  /* ==================================================================== */
  /*      Now process the data, applying to the working imageObj.         */
  /* ==================================================================== */
  k = 0;

  for (i = dst_yoff; i < dst_yoff + dst_ysize; i++) {
    for (j = dst_xoff; j < dst_xoff + dst_xsize; j++) {
      float fRawValue = pafRawData[k++];
      int iMapIndex;

      /*
       * Skip nodata pixels ... no processing.
       */
      if (bGotNoData && IsNoData(fRawValue, fNoDataValue))
        continue;

      if (CPLIsNan(fRawValue))
        continue;

      if (SKIP_MASK(j, i))
        continue;

      /*
       * The funny +1/-1 is to avoid odd rounding around zero.
       * We could use floor() but sometimes it is expensive.
       */
      iMapIndex = (int)((fRawValue - dfScaleMin) * dfScaleRatio + 1) - 1;

      if (iMapIndex >= nBucketCount || iMapIndex < 0) {
        continue;
      }
      /* currently we never have partial alpha so keep simple */
      if (rb_cmap[3][iMapIndex] > 0)
        RB_SET_PIXEL(rb, j, i, rb_cmap[0][iMapIndex], rb_cmap[1][iMapIndex],
                     rb_cmap[2][iMapIndex], rb_cmap[3][iMapIndex]);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Cleanup                                                         */
  /* -------------------------------------------------------------------- */
  free(pafRawData);
  free(cmap);
  free(rb_cmap[0]);
  free(rb_cmap[1]);
  free(rb_cmap[2]);
  free(rb_cmap[3]);

  assert(k == dst_xsize * dst_ysize);

  return 0;
}

/************************************************************************/
/*                          IsNoData()                                  */
/************************************************************************/
static bool IsNoData(double dfValue, double dfNoDataValue) {
  return isnan(dfValue) || dfValue == dfNoDataValue;
}

/************************************************************************/
/*                          msGetGDALNoDataValue()                      */
/************************************************************************/
double msGetGDALNoDataValue(layerObj *layer, void *hBand, int *pbGotNoData)

{
  const char *pszNODATAOpt;

  *pbGotNoData = FALSE;

  /* -------------------------------------------------------------------- */
  /*      Check for a NODATA setting in the .map file.                    */
  /* -------------------------------------------------------------------- */
  pszNODATAOpt = CSLFetchNameValue(layer->processing, "NODATA");
  if (pszNODATAOpt != NULL) {
    if (EQUAL(pszNODATAOpt, "OFF") || strlen(pszNODATAOpt) == 0)
      return -1234567.0;
    if (!EQUAL(pszNODATAOpt, "AUTO")) {
      *pbGotNoData = TRUE;
      return atof(pszNODATAOpt);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Check for a NODATA setting on the raw file.                     */
  /* -------------------------------------------------------------------- */
  if (hBand == NULL)
    return -1234567.0;
  else
    return GDALGetRasterNoDataValue(hBand, pbGotNoData);
}

/************************************************************************/
/*                         msGetGDALBandList()                          */
/*                                                                      */
/*      max_bands - pass zero for no limit.                             */
/*      band_count - location in which length of the return band        */
/*      list is placed.                                                 */
/*                                                                      */
/*      returns malloc() list of band numbers (*band_count long).       */
/************************************************************************/

int *msGetGDALBandList(layerObj *layer, void *hDS, int max_bands,
                       int *band_count)

{
  int i, file_bands;
  int *band_list = NULL;

  file_bands = GDALGetRasterCount(hDS);
  if (file_bands == 0) {
    msSetError(MS_IMGERR,
               "Attempt to operate on GDAL file with no bands, layer=%s.",
               "msGetGDALBandList()", layer->name);

    return NULL;
  }

  /* -------------------------------------------------------------------- */
  /*      Use all (or first) bands.                                       */
  /* -------------------------------------------------------------------- */
  if (CSLFetchNameValue(layer->processing, "BANDS") == NULL) {
    if (max_bands > 0)
      *band_count = MS_MIN(file_bands, max_bands);
    else
      *band_count = file_bands;

    band_list = (int *)malloc(sizeof(int) * *band_count);

    /* FIXME MS_CHECK_ALLOC leaks papszItems */
    MS_CHECK_ALLOC(band_list, sizeof(int) * *band_count, NULL);

    for (i = 0; i < *band_count; i++)
      band_list[i] = i + 1;
    return band_list;
  }

  /* -------------------------------------------------------------------- */
  /*      get bands from BANDS processing directive.                      */
  /* -------------------------------------------------------------------- */
  else {
    char **papszItems = CSLTokenizeStringComplex(
        CSLFetchNameValue(layer->processing, "BANDS"), " ,", FALSE, FALSE);

    if (CSLCount(papszItems) == 0) {
      CSLDestroy(papszItems);
      msSetError(MS_IMGERR, "BANDS PROCESSING directive has no items.",
                 "msGetGDALBandList()");
      return NULL;
    } else if (max_bands != 0 && CSLCount(papszItems) > max_bands) {
      msSetError(MS_IMGERR,
                 "BANDS PROCESSING directive has wrong number of bands, "
                 "expected at most %d, got %d.",
                 "msGetGDALBandList()", max_bands, CSLCount(papszItems));
      CSLDestroy(papszItems);
      return NULL;
    }

    *band_count = CSLCount(papszItems);
    band_list = (int *)malloc(sizeof(int) * *band_count);
    MS_CHECK_ALLOC(band_list, sizeof(int) * *band_count, NULL);

    for (i = 0; i < *band_count; i++) {
      band_list[i] = atoi(papszItems[i]);
      if (band_list[i] < 1 || band_list[i] > GDALGetRasterCount(hDS)) {
        msSetError(MS_IMGERR,
                   "BANDS PROCESSING directive includes illegal band '%s', "
                   "should be from 1 to %d.",
                   "msGetGDALBandList()", papszItems[i],
                   GDALGetRasterCount(hDS));
        CSLDestroy(papszItems);
        free(band_list);
        return NULL;
      }
    }
    CSLDestroy(papszItems);

    return band_list;
  }
}

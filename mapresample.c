/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Assorted code related to resampling rasters.
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
#include "mapresample.h"
#include "mapthread.h"

#define SKIP_MASK(x, y)                                                        \
  (mask_rb && !*(mask_rb->data.rgba.a + (y)*mask_rb->data.rgba.row_step +      \
                 (x)*mask_rb->data.rgba.pixel_step))

/************************************************************************/
/*                          InvGeoTransform()                           */
/*                                                                      */
/*      Invert a standard 3x2 "GeoTransform" style matrix with an       */
/*      implicit [1 0 0] final row.                                     */
/************************************************************************/

int InvGeoTransform(double *gt_in, double *gt_out)

{
  double det, inv_det;

  /* we assume a 3rd row that is [1 0 0] */

  /* Compute determinate */

  det = gt_in[1] * gt_in[5] - gt_in[2] * gt_in[4];

  if (fabs(det) < 0.000000000000001)
    return 0;

  inv_det = 1.0 / det;

  /* compute adjoint, and devide by determinate */

  gt_out[1] = gt_in[5] * inv_det;
  gt_out[4] = -gt_in[4] * inv_det;

  gt_out[2] = -gt_in[2] * inv_det;
  gt_out[5] = gt_in[1] * inv_det;

  gt_out[0] = (gt_in[2] * gt_in[3] - gt_in[0] * gt_in[5]) * inv_det;
  gt_out[3] = (-gt_in[1] * gt_in[3] + gt_in[0] * gt_in[4]) * inv_det;

  return 1;
}

/************************************************************************/
/*                      msNearestRasterResample()                       */
/************************************************************************/

static int msNearestRasterResampler(
    imageObj *psSrcImage, rasterBufferObj *src_rb, imageObj *psDstImage,
    rasterBufferObj *dst_rb, SimpleTransformer pfnTransform, void *pCBData,
    int debug, rasterBufferObj *mask_rb, int bWrapAtLeftRight)

{
  double *x, *y;
  int nDstX, nDstY;
  int *panSuccess;
  int nDstXSize = psDstImage->width;
  int nDstYSize = psDstImage->height;
  int nSrcXSize = psSrcImage->width;
  int nSrcYSize = psSrcImage->height;
  int nFailedPoints = 0, nSetPoints = 0;
  assert(!MS_RENDERER_PLUGIN(psSrcImage->format) ||
         src_rb->type == MS_BUFFER_BYTE_RGBA);

  x = (double *)msSmallMalloc(sizeof(double) * nDstXSize);
  y = (double *)msSmallMalloc(sizeof(double) * nDstXSize);
  panSuccess = (int *)msSmallMalloc(sizeof(int) * nDstXSize);

  for (nDstY = 0; nDstY < nDstYSize; nDstY++) {
    for (nDstX = 0; nDstX < nDstXSize; nDstX++) {
      x[nDstX] = nDstX + 0.5;
      y[nDstX] = nDstY + 0.5;
    }

    pfnTransform(pCBData, nDstXSize, x, y, panSuccess);

    for (nDstX = 0; nDstX < nDstXSize; nDstX++) {
      int nSrcX, nSrcY;
      if (SKIP_MASK(nDstX, nDstY))
        continue;

      if (!panSuccess[nDstX]) {
        nFailedPoints++;
        continue;
      }

      nSrcX = (int)x[nDstX];
      nSrcY = (int)y[nDstX];

      if (bWrapAtLeftRight && nSrcX >= nSrcXSize && nSrcX < 2 * nSrcXSize)
        nSrcX -= nSrcXSize;

      /*
       * We test the original floating point values to
       * avoid errors related to asymmetric rounding around zero.
       * (Also note bug #3120 regarding nearly redundent x/y < 0 checks).
       */
      if (x[nDstX] < 0.0 || y[nDstX] < 0.0 || nSrcX < 0 || nSrcY < 0 ||
          nSrcX >= nSrcXSize || nSrcY >= nSrcYSize) {
        continue;
      }

      if (MS_RENDERER_PLUGIN(psSrcImage->format)) {
        int src_rb_off;
        rgbaArrayObj *src, *dst;
        assert(src_rb && dst_rb);
        assert(src_rb->type == MS_BUFFER_BYTE_RGBA);
        src = &src_rb->data.rgba;
        dst = &dst_rb->data.rgba;
        src_rb_off = nSrcX * src->pixel_step + nSrcY * src->row_step;

        if (src->a == NULL || src->a[src_rb_off] == 255) {
          int dst_rb_off;

          dst_rb_off = nDstX * dst->pixel_step + nDstY * dst->row_step;

          nSetPoints++;

          dst->r[dst_rb_off] = src->r[src_rb_off];
          dst->g[dst_rb_off] = src->g[src_rb_off];
          dst->b[dst_rb_off] = src->b[src_rb_off];
          if (dst->a)
            dst->a[dst_rb_off] = 255;
        } else if (src->a[src_rb_off] != 0) {
          int dst_rb_off;

          dst_rb_off = nDstX * dst->pixel_step + nDstY * dst->row_step;

          nSetPoints++;

          /* actual alpha blending is required */
          msAlphaBlendPM(
              src->r[src_rb_off], src->g[src_rb_off], src->b[src_rb_off],
              src->a[src_rb_off], dst->r + dst_rb_off, dst->g + dst_rb_off,
              dst->b + dst_rb_off, dst->a ? dst->a + dst_rb_off : NULL);
        }
      } else if (MS_RENDERER_RAWDATA(psSrcImage->format)) {
        int band, src_off, dst_off;

        src_off = nSrcX + nSrcY * psSrcImage->width;

        if (!MS_GET_BIT(psSrcImage->img_mask, src_off))
          continue;

        nSetPoints++;

        dst_off = nDstX + nDstY * psDstImage->width;

        MS_SET_BIT(psDstImage->img_mask, dst_off);

        for (band = 0; band < psSrcImage->format->bands; band++) {
          if (psSrcImage->format->imagemode == MS_IMAGEMODE_INT16) {
            psDstImage->img.raw_16bit[dst_off] =
                psSrcImage->img.raw_16bit[src_off];
          } else if (psSrcImage->format->imagemode == MS_IMAGEMODE_FLOAT32) {
            psDstImage->img.raw_float[dst_off] =
                psSrcImage->img.raw_float[src_off];
          } else if (psSrcImage->format->imagemode == MS_IMAGEMODE_BYTE) {
            psDstImage->img.raw_byte[dst_off] =
                psSrcImage->img.raw_byte[src_off];
          } else {
            assert(0);
          }
          src_off += psSrcImage->width * psSrcImage->height;
          dst_off += psDstImage->width * psDstImage->height;
        }
      }
    }
  }

  free(panSuccess);
  free(x);
  free(y);

  /* -------------------------------------------------------------------- */
  /*      Some debugging output.                                          */
  /* -------------------------------------------------------------------- */
  if (nFailedPoints > 0 && debug) {
    msDebug("msNearestRasterResampler: "
            "%d failed to transform, %d actually set.\n",
            nFailedPoints, nSetPoints);
  }

  return 0;
}

/************************************************************************/
/*                            msSourceSample()                          */
/************************************************************************/

static void msSourceSample(imageObj *psSrcImage, rasterBufferObj *rb, int iSrcX,
                           int iSrcY, double *padfPixelSum, double dfWeight,
                           double *pdfWeightSum)

{
  if (MS_RENDERER_PLUGIN(psSrcImage->format)) {
    rgbaArrayObj *rgba;
    int rb_off;
    assert(rb && rb->type == MS_BUFFER_BYTE_RGBA);
    rgba = &(rb->data.rgba);
    rb_off = iSrcX * rgba->pixel_step + iSrcY * rgba->row_step;

    if (rgba->a == NULL || rgba->a[rb_off] > 1) {
      padfPixelSum[0] += rgba->r[rb_off] * dfWeight;
      padfPixelSum[1] += rgba->g[rb_off] * dfWeight;
      padfPixelSum[2] += rgba->b[rb_off] * dfWeight;

      if (rgba->a == NULL)
        *pdfWeightSum += dfWeight;
      else
        *pdfWeightSum += dfWeight * (rgba->a[rb_off] / 255.0);
    }
  } else if (MS_RENDERER_RAWDATA(psSrcImage->format)) {
    int band;
    int src_off;

    src_off = iSrcX + iSrcY * psSrcImage->width;

    if (!MS_GET_BIT(psSrcImage->img_mask, src_off))
      return;

    for (band = 0; band < psSrcImage->format->bands; band++) {
      if (psSrcImage->format->imagemode == MS_IMAGEMODE_INT16) {
        int nValue;

        nValue = psSrcImage->img.raw_16bit[src_off];

        padfPixelSum[band] += dfWeight * nValue;
      } else if (psSrcImage->format->imagemode == MS_IMAGEMODE_FLOAT32) {
        float fValue;

        fValue = psSrcImage->img.raw_float[src_off];

        padfPixelSum[band] += fValue * dfWeight;
      } else if (psSrcImage->format->imagemode == MS_IMAGEMODE_BYTE) {
        int nValue;

        nValue = psSrcImage->img.raw_byte[src_off];

        padfPixelSum[band] += nValue * dfWeight;
      } else {
        assert(0);
        return;
      }

      src_off += psSrcImage->width * psSrcImage->height;
    }
    *pdfWeightSum += dfWeight;
  }
}

/************************************************************************/
/*                      msBilinearRasterResample()                      */
/************************************************************************/

static int msBilinearRasterResampler(
    imageObj *psSrcImage, rasterBufferObj *src_rb, imageObj *psDstImage,
    rasterBufferObj *dst_rb, SimpleTransformer pfnTransform, void *pCBData,
    int debug, rasterBufferObj *mask_rb, int bWrapAtLeftRight)

{
  double *x, *y;
  int nDstX, nDstY, i;
  int *panSuccess;
  int nDstXSize = psDstImage->width;
  int nDstYSize = psDstImage->height;
  int nSrcXSize = psSrcImage->width;
  int nSrcYSize = psSrcImage->height;
  int nFailedPoints = 0, nSetPoints = 0;
  double *padfPixelSum;
  int bandCount = MS_MAX(4, psSrcImage->format->bands);

  padfPixelSum = (double *)msSmallMalloc(sizeof(double) * bandCount);

  x = (double *)msSmallMalloc(sizeof(double) * nDstXSize);
  y = (double *)msSmallMalloc(sizeof(double) * nDstXSize);
  panSuccess = (int *)msSmallMalloc(sizeof(int) * nDstXSize);

  for (nDstY = 0; nDstY < nDstYSize; nDstY++) {
    for (nDstX = 0; nDstX < nDstXSize; nDstX++) {
      x[nDstX] = nDstX + 0.5;
      y[nDstX] = nDstY + 0.5;
    }

    pfnTransform(pCBData, nDstXSize, x, y, panSuccess);

    for (nDstX = 0; nDstX < nDstXSize; nDstX++) {
      int nSrcX, nSrcY, nSrcX2, nSrcY2;
      double dfRatioX2, dfRatioY2, dfWeightSum = 0.0;
      if (SKIP_MASK(nDstX, nDstY))
        continue;

      if (!panSuccess[nDstX]) {
        nFailedPoints++;
        continue;
      }

      /* If we are right off the source, skip this pixel */
      nSrcX = (int)floor(x[nDstX]);
      nSrcY = (int)floor(y[nDstX]);
      if (nSrcX < 0 || (!bWrapAtLeftRight && nSrcX >= nSrcXSize) || nSrcY < 0 ||
          nSrcY >= nSrcYSize)
        continue;

      /*
      ** Offset to treat TL pixel corners as pixel location instead
      ** of the center.
      */
      x[nDstX] -= 0.5;
      y[nDstX] -= 0.5;

      nSrcX = (int)floor(x[nDstX]);
      nSrcY = (int)floor(y[nDstX]);

      nSrcX2 = nSrcX + 1;
      nSrcY2 = nSrcY + 1;

      dfRatioX2 = x[nDstX] - nSrcX;
      dfRatioY2 = y[nDstX] - nSrcY;

      /* Trim in stuff one pixel off the edge */
      nSrcX = MS_MAX(nSrcX, 0);
      nSrcY = MS_MAX(nSrcY, 0);
      if (!bWrapAtLeftRight)
        nSrcX2 = MS_MIN(nSrcX2, nSrcXSize - 1);
      nSrcY2 = MS_MIN(nSrcY2, nSrcYSize - 1);

      memset(padfPixelSum, 0, sizeof(double) * bandCount);

      msSourceSample(psSrcImage, src_rb, nSrcX % nSrcXSize, nSrcY, padfPixelSum,
                     (1.0 - dfRatioX2) * (1.0 - dfRatioY2), &dfWeightSum);

      msSourceSample(psSrcImage, src_rb, nSrcX2 % nSrcXSize, nSrcY,
                     padfPixelSum, (dfRatioX2) * (1.0 - dfRatioY2),
                     &dfWeightSum);

      msSourceSample(psSrcImage, src_rb, nSrcX % nSrcXSize, nSrcY2,
                     padfPixelSum, (1.0 - dfRatioX2) * (dfRatioY2),
                     &dfWeightSum);

      msSourceSample(psSrcImage, src_rb, nSrcX2 % nSrcXSize, nSrcY2,
                     padfPixelSum, (dfRatioX2) * (dfRatioY2), &dfWeightSum);

      if (dfWeightSum == 0.0)
        continue;

      if (MS_RENDERER_PLUGIN(psSrcImage->format)) {
        assert(src_rb && dst_rb);
        assert(src_rb->type == MS_BUFFER_BYTE_RGBA);
        assert(src_rb->type == dst_rb->type);

        nSetPoints++;

        if (dfWeightSum > 0.001) {
          int dst_rb_off = nDstX * dst_rb->data.rgba.pixel_step +
                           nDstY * dst_rb->data.rgba.row_step;
          unsigned char red, green, blue, alpha;

          red = (unsigned char)MS_MAX(0, MS_MIN(255, padfPixelSum[0]));
          green = (unsigned char)MS_MAX(0, MS_MIN(255, padfPixelSum[1]));
          blue = (unsigned char)MS_MAX(0, MS_MIN(255, padfPixelSum[2]));
          alpha = (unsigned char)MS_MAX(0, MS_MIN(255, 255.5 * dfWeightSum));

          msAlphaBlendPM(
              red, green, blue, alpha, dst_rb->data.rgba.r + dst_rb_off,
              dst_rb->data.rgba.g + dst_rb_off,
              dst_rb->data.rgba.b + dst_rb_off,
              (dst_rb->data.rgba.a == NULL) ? NULL
                                            : dst_rb->data.rgba.a + dst_rb_off);
        }
      } else if (MS_RENDERER_RAWDATA(psSrcImage->format)) {
        int band;
        int dst_off = nDstX + nDstY * psDstImage->width;

        for (i = 0; i < bandCount; i++)
          padfPixelSum[i] /= dfWeightSum;

        MS_SET_BIT(psDstImage->img_mask, dst_off);

        for (band = 0; band < psSrcImage->format->bands; band++) {
          if (psSrcImage->format->imagemode == MS_IMAGEMODE_INT16) {
            psDstImage->img.raw_16bit[dst_off] = (short)padfPixelSum[band];
          } else if (psSrcImage->format->imagemode == MS_IMAGEMODE_FLOAT32) {
            psDstImage->img.raw_float[dst_off] = (float)padfPixelSum[band];
          } else if (psSrcImage->format->imagemode == MS_IMAGEMODE_BYTE) {
            psDstImage->img.raw_byte[dst_off] =
                (unsigned char)MS_MAX(0, MS_MIN(255, padfPixelSum[band]));
          }

          dst_off += psDstImage->width * psDstImage->height;
        }
      }
    }
  }

  free(padfPixelSum);
  free(panSuccess);
  free(x);
  free(y);

  /* -------------------------------------------------------------------- */
  /*      Some debugging output.                                          */
  /* -------------------------------------------------------------------- */
  if (nFailedPoints > 0 && debug) {
    msDebug("msBilinearRasterResampler: "
            "%d failed to transform, %d actually set.\n",
            nFailedPoints, nSetPoints);
  }

  return 0;
}

/************************************************************************/
/*                          msAverageSample()                           */
/************************************************************************/

static int msAverageSample(imageObj *psSrcImage, rasterBufferObj *src_rb,
                           double dfXMin, double dfYMin, double dfXMax,
                           double dfYMax, double *padfPixelSum,
                           double *pdfAlpha01)

{
  int nXMin, nXMax, nYMin, nYMax, iX, iY;
  double dfWeightSum = 0.0;
  double dfMaxWeight = 0.0;

  nXMin = (int)dfXMin;
  nYMin = (int)dfYMin;
  nXMax = (int)ceil(dfXMax);
  nYMax = (int)ceil(dfYMax);

  *pdfAlpha01 = 0.0;

  for (iY = nYMin; iY < nYMax; iY++) {
    double dfYCellMin, dfYCellMax;

    dfYCellMin = MS_MAX(iY, dfYMin);
    dfYCellMax = MS_MIN(iY + 1, dfYMax);

    for (iX = nXMin; iX < nXMax; iX++) {
      double dfXCellMin, dfXCellMax, dfWeight;

      dfXCellMin = MS_MAX(iX, dfXMin);
      dfXCellMax = MS_MIN(iX + 1, dfXMax);

      dfWeight = (dfXCellMax - dfXCellMin) * (dfYCellMax - dfYCellMin);

      msSourceSample(psSrcImage, src_rb, iX, iY, padfPixelSum, dfWeight,
                     &dfWeightSum);
      dfMaxWeight += dfWeight;
    }
  }

  if (dfWeightSum == 0.0)
    return MS_FALSE;

  for (iX = 0; iX < 4; iX++)
    padfPixelSum[iX] /= dfWeightSum;

  *pdfAlpha01 = dfWeightSum / dfMaxWeight;

  return MS_TRUE;
}

/************************************************************************/
/*                      msAverageRasterResample()                       */
/************************************************************************/

static int
msAverageRasterResampler(imageObj *psSrcImage, rasterBufferObj *src_rb,
                         imageObj *psDstImage, rasterBufferObj *dst_rb,
                         SimpleTransformer pfnTransform, void *pCBData,
                         int debug, rasterBufferObj *mask_rb)

{
  double *x1, *y1, *x2, *y2;
  int nDstX, nDstY;
  int *panSuccess1, *panSuccess2;
  int nDstXSize = psDstImage->width;
  int nDstYSize = psDstImage->height;
  int nFailedPoints = 0, nSetPoints = 0;
  double *padfPixelSum;

  int bandCount = MS_MAX(4, psSrcImage->format->bands);

  padfPixelSum = (double *)msSmallMalloc(sizeof(double) * bandCount);

  x1 = (double *)msSmallMalloc(sizeof(double) * (nDstXSize + 1));
  y1 = (double *)msSmallMalloc(sizeof(double) * (nDstXSize + 1));
  x2 = (double *)msSmallMalloc(sizeof(double) * (nDstXSize + 1));
  y2 = (double *)msSmallMalloc(sizeof(double) * (nDstXSize + 1));
  panSuccess1 = (int *)msSmallMalloc(sizeof(int) * (nDstXSize + 1));
  panSuccess2 = (int *)msSmallMalloc(sizeof(int) * (nDstXSize + 1));

  for (nDstY = 0; nDstY < nDstYSize; nDstY++) {
    for (nDstX = 0; nDstX <= nDstXSize; nDstX++) {
      x1[nDstX] = nDstX;
      y1[nDstX] = nDstY;
      x2[nDstX] = nDstX;
      y2[nDstX] = nDstY + 1;
    }

    pfnTransform(pCBData, nDstXSize + 1, x1, y1, panSuccess1);
    pfnTransform(pCBData, nDstXSize + 1, x2, y2, panSuccess2);

    for (nDstX = 0; nDstX < nDstXSize; nDstX++) {
      double dfXMin, dfYMin, dfXMax, dfYMax;
      double dfAlpha01;
      if (SKIP_MASK(nDstX, nDstY))
        continue;

      /* Do not generate a pixel unless all four corners transformed */
      if (!panSuccess1[nDstX] || !panSuccess1[nDstX + 1] ||
          !panSuccess2[nDstX] || !panSuccess2[nDstX + 1]) {
        nFailedPoints++;
        continue;
      }

      dfXMin = MS_MIN(MS_MIN(x1[nDstX], x1[nDstX + 1]),
                      MS_MIN(x2[nDstX], x2[nDstX + 1]));
      dfYMin = MS_MIN(MS_MIN(y1[nDstX], y1[nDstX + 1]),
                      MS_MIN(y2[nDstX], y2[nDstX + 1]));
      dfXMax = MS_MAX(MS_MAX(x1[nDstX], x1[nDstX + 1]),
                      MS_MAX(x2[nDstX], x2[nDstX + 1]));
      dfYMax = MS_MAX(MS_MAX(y1[nDstX], y1[nDstX + 1]),
                      MS_MAX(y2[nDstX], y2[nDstX + 1]));

      dfXMin = MS_MIN(MS_MAX(dfXMin, 0), psSrcImage->width + 1);
      dfYMin = MS_MIN(MS_MAX(dfYMin, 0), psSrcImage->height + 1);
      dfXMax = MS_MIN(MS_MAX(-1, dfXMax), psSrcImage->width);
      dfYMax = MS_MIN(MS_MAX(-1, dfYMax), psSrcImage->height);

      memset(padfPixelSum, 0, sizeof(double) * bandCount);

      if (!msAverageSample(psSrcImage, src_rb, dfXMin, dfYMin, dfXMax, dfYMax,
                           padfPixelSum, &dfAlpha01))
        continue;

      if (MS_RENDERER_PLUGIN(psSrcImage->format)) {
        assert(dst_rb && src_rb);
        assert(dst_rb->type == MS_BUFFER_BYTE_RGBA);
        assert(src_rb->type == dst_rb->type);

        nSetPoints++;

        if (dfAlpha01 > 0) {
          unsigned char red, green, blue, alpha;

          red = (unsigned char)MS_MAX(0, MS_MIN(255, padfPixelSum[0] + 0.5));
          green = (unsigned char)MS_MAX(0, MS_MIN(255, padfPixelSum[1] + 0.5));
          blue = (unsigned char)MS_MAX(0, MS_MIN(255, padfPixelSum[2] + 0.5));
          alpha = (unsigned char)MS_MAX(0, MS_MIN(255, 255 * dfAlpha01 + 0.5));

          RB_MIX_PIXEL(dst_rb, nDstX, nDstY, red, green, blue, alpha);
        }
      } else if (MS_RENDERER_RAWDATA(psSrcImage->format)) {
        int band;
        int dst_off = nDstX + nDstY * psDstImage->width;

        MS_SET_BIT(psDstImage->img_mask, dst_off);

        for (band = 0; band < psSrcImage->format->bands; band++) {
          if (psSrcImage->format->imagemode == MS_IMAGEMODE_INT16) {
            psDstImage->img.raw_16bit[dst_off] =
                (short)(padfPixelSum[band] + 0.5);
          } else if (psSrcImage->format->imagemode == MS_IMAGEMODE_FLOAT32) {
            psDstImage->img.raw_float[dst_off] = (float)padfPixelSum[band];
          } else if (psSrcImage->format->imagemode == MS_IMAGEMODE_BYTE) {
            psDstImage->img.raw_byte[dst_off] =
                (unsigned char)padfPixelSum[band];
          }

          dst_off += psDstImage->width * psDstImage->height;
        }
      }
    }
  }

  free(padfPixelSum);
  free(panSuccess1);
  free(x1);
  free(y1);
  free(panSuccess2);
  free(x2);
  free(y2);

  /* -------------------------------------------------------------------- */
  /*      Some debugging output.                                          */
  /* -------------------------------------------------------------------- */
  if (nFailedPoints > 0 && debug) {
    msDebug("msAverageRasterResampler: "
            "%d failed to transform, %d actually set.\n",
            nFailedPoints, nSetPoints);
  }

  return 0;
}

/************************************************************************/
/* ==================================================================== */
/*      PROJ based transformer.         */
/* ==================================================================== */
/************************************************************************/

typedef struct {
  projectionObj *psSrcProjObj;
  int bSrcIsGeographic;
  double adfInvSrcGeoTransform[6];

  projectionObj *psDstProjObj;
  int bDstIsGeographic;
  double adfDstGeoTransform[6];

  int bUseProj;
#if PROJ_VERSION_MAJOR >= 6
  reprojectionObj *pReprojectionDstToSrc;
#endif
} msProjTransformInfo;

/************************************************************************/
/*                       msInitProjTransformer()                        */
/************************************************************************/

void *msInitProjTransformer(projectionObj *psSrc, double *padfSrcGeoTransform,
                            projectionObj *psDst, double *padfDstGeoTransform)

{
  int backup_src_need_gt;
  int backup_dst_need_gt;
  msProjTransformInfo *psPTInfo;

  psPTInfo =
      (msProjTransformInfo *)msSmallCalloc(1, sizeof(msProjTransformInfo));

  /* -------------------------------------------------------------------- */
  /*      We won't even use PROJ.4 if either coordinate system is         */
  /*      NULL.                                                           */
  /* -------------------------------------------------------------------- */
  backup_src_need_gt = psSrc->gt.need_geotransform;
  psSrc->gt.need_geotransform = 0;
  backup_dst_need_gt = psDst->gt.need_geotransform;
  psDst->gt.need_geotransform = 0;
  psPTInfo->bUseProj = (psSrc->proj != NULL && psDst->proj != NULL &&
                        msProjectionsDiffer(psSrc, psDst));
  psSrc->gt.need_geotransform = backup_src_need_gt;
  psDst->gt.need_geotransform = backup_dst_need_gt;

  /* -------------------------------------------------------------------- */
  /*      Record source image information.  We invert the source          */
  /*      transformation for more convenient inverse application in       */
  /*      the transformer.                                                */
  /* -------------------------------------------------------------------- */
  psPTInfo->psSrcProjObj = psSrc;
  if (psPTInfo->bUseProj)
    psPTInfo->bSrcIsGeographic = msProjIsGeographicCRS(psSrc);
  else
    psPTInfo->bSrcIsGeographic = MS_FALSE;

  if (!InvGeoTransform(padfSrcGeoTransform, psPTInfo->adfInvSrcGeoTransform)) {
    free(psPTInfo);
    return NULL;
  }

  /* -------------------------------------------------------------------- */
  /*      Record destination image information.                           */
  /* -------------------------------------------------------------------- */
  psPTInfo->psDstProjObj = psDst;
  if (psPTInfo->bUseProj)
    psPTInfo->bDstIsGeographic = msProjIsGeographicCRS(psDst);
  else
    psPTInfo->bDstIsGeographic = MS_FALSE;
  memcpy(psPTInfo->adfDstGeoTransform, padfDstGeoTransform, sizeof(double) * 6);

#if PROJ_VERSION_MAJOR >= 6
  if (psPTInfo->bUseProj) {
    psPTInfo->pReprojectionDstToSrc = msProjectCreateReprojector(
        psPTInfo->psDstProjObj, psPTInfo->psSrcProjObj);
    if (!psPTInfo->pReprojectionDstToSrc) {
      free(psPTInfo);
      return NULL;
    }
  }
#endif

  return psPTInfo;
}

/************************************************************************/
/*                       msFreeProjTransformer()                        */
/************************************************************************/

void msFreeProjTransformer(void *pCBData)

{
#if PROJ_VERSION_MAJOR >= 6
  if (pCBData) {
    msProjTransformInfo *psPTInfo = (msProjTransformInfo *)pCBData;
    msProjectDestroyReprojector(psPTInfo->pReprojectionDstToSrc);
  }
#endif
  free(pCBData);
}

/************************************************************************/
/*                          msProjTransformer                           */
/************************************************************************/

int msProjTransformer(void *pCBData, int nPoints, double *x, double *y,
                      int *panSuccess)

{
  int i;
  msProjTransformInfo *psPTInfo = (msProjTransformInfo *)pCBData;
  double x_out;

  /* -------------------------------------------------------------------- */
  /*      Transform into destination georeferenced space.                 */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < nPoints; i++) {
    x_out = psPTInfo->adfDstGeoTransform[0] +
            psPTInfo->adfDstGeoTransform[1] * x[i] +
            psPTInfo->adfDstGeoTransform[2] * y[i];
    y[i] = psPTInfo->adfDstGeoTransform[3] +
           psPTInfo->adfDstGeoTransform[4] * x[i] +
           psPTInfo->adfDstGeoTransform[5] * y[i];
    x[i] = x_out;

    panSuccess[i] = 1;
  }

#if PROJ_VERSION_MAJOR >= 6
  if (psPTInfo->bUseProj) {
    if (msProjectTransformPoints(psPTInfo->pReprojectionDstToSrc, nPoints, x,
                                 y) != MS_SUCCESS) {
      for (i = 0; i < nPoints; i++)
        panSuccess[i] = 0;

      return MS_FALSE;
    }
    for (i = 0; i < nPoints; i++) {
      if (x[i] == HUGE_VAL || y[i] == HUGE_VAL)
        panSuccess[i] = 0;
    }
  }
#else
  /* -------------------------------------------------------------------- */
  /*      Transform from degrees to radians if geographic.                */
  /* -------------------------------------------------------------------- */
  if (psPTInfo->bDstIsGeographic) {
    for (i = 0; i < nPoints; i++) {
      x[i] = x[i] * DEG_TO_RAD;
      y[i] = y[i] * DEG_TO_RAD;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Transform back to source projection space.                      */
  /* -------------------------------------------------------------------- */
  if (psPTInfo->bUseProj) {
    double *z;
    int tr_result;

    z = (double *)msSmallCalloc(sizeof(double), nPoints);

    msAcquireLock(TLOCK_PROJ);
    tr_result = pj_transform(psPTInfo->psDstProjObj->proj,
                             psPTInfo->psSrcProjObj->proj, nPoints, 1, x, y, z);
    msReleaseLock(TLOCK_PROJ);

    if (tr_result != 0) {
      free(z);
      for (i = 0; i < nPoints; i++)
        panSuccess[i] = 0;

      return MS_FALSE;
    }
    free(z);

    for (i = 0; i < nPoints; i++) {
      if (x[i] == HUGE_VAL || y[i] == HUGE_VAL)
        panSuccess[i] = 0;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Transform back to degrees if source is geographic.              */
  /* -------------------------------------------------------------------- */
  if (psPTInfo->bSrcIsGeographic) {
    for (i = 0; i < nPoints; i++) {
      if (panSuccess[i]) {
        x[i] = x[i] * RAD_TO_DEG;
        y[i] = y[i] * RAD_TO_DEG;
      }
    }
  }
#endif

  /* -------------------------------------------------------------------- */
  /*      Transform to source raster space.                               */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < nPoints; i++) {
    if (panSuccess[i]) {
      x_out = psPTInfo->adfInvSrcGeoTransform[0] +
              psPTInfo->adfInvSrcGeoTransform[1] * x[i] +
              psPTInfo->adfInvSrcGeoTransform[2] * y[i];
      y[i] = psPTInfo->adfInvSrcGeoTransform[3] +
             psPTInfo->adfInvSrcGeoTransform[4] * x[i] +
             psPTInfo->adfInvSrcGeoTransform[5] * y[i];
      x[i] = x_out;
    } else {
      x[i] = -1;
      y[i] = -1;
    }
  }

  return 1;
}

/************************************************************************/
/* ==================================================================== */
/*      Approximate transformer.                                        */
/* ==================================================================== */
/************************************************************************/

typedef struct {
  SimpleTransformer pfnBaseTransformer;
  void *pBaseCBData;

  double dfMaxError;
} msApproxTransformInfo;

/************************************************************************/
/*                      msInitApproxTransformer()                       */
/************************************************************************/

static void *msInitApproxTransformer(SimpleTransformer pfnBaseTransformer,
                                     void *pBaseCBData, double dfMaxError)

{
  msApproxTransformInfo *psATInfo;

  psATInfo =
      (msApproxTransformInfo *)msSmallMalloc(sizeof(msApproxTransformInfo));
  psATInfo->pfnBaseTransformer = pfnBaseTransformer;
  psATInfo->pBaseCBData = pBaseCBData;
  psATInfo->dfMaxError = dfMaxError;

  return psATInfo;
}

/************************************************************************/
/*                      msFreeApproxTransformer()                       */
/************************************************************************/

static void msFreeApproxTransformer(void *pCBData)

{
  free(pCBData);
}

/************************************************************************/
/*                         msApproxTransformer                          */
/************************************************************************/

static int msApproxTransformer(void *pCBData, int nPoints, double *x, double *y,
                               int *panSuccess)

{
  msApproxTransformInfo *psATInfo = (msApproxTransformInfo *)pCBData;
  double x2[3], y2[3], dfDeltaX, dfDeltaY, dfError, dfDist;
  int nMiddle, anSuccess2[3], i, bSuccess;

  nMiddle = (nPoints - 1) / 2;

  /* -------------------------------------------------------------------- */
  /*      Bail if our preconditions are not met, or if error is not       */
  /*      acceptable.                                                     */
  /* -------------------------------------------------------------------- */
  if (y[0] != y[nPoints - 1] || y[0] != y[nMiddle] || x[0] == x[nPoints - 1] ||
      x[0] == x[nMiddle] || psATInfo->dfMaxError == 0.0 || nPoints <= 5) {
    return psATInfo->pfnBaseTransformer(psATInfo->pBaseCBData, nPoints, x, y,
                                        panSuccess);
  }

  /* -------------------------------------------------------------------- */
  /*      Transform first, last and middle point.                         */
  /* -------------------------------------------------------------------- */
  x2[0] = x[0];
  y2[0] = y[0];
  x2[1] = x[nMiddle];
  y2[1] = y[nMiddle];
  x2[2] = x[nPoints - 1];
  y2[2] = y[nPoints - 1];

  bSuccess = psATInfo->pfnBaseTransformer(psATInfo->pBaseCBData, 3, x2, y2,
                                          anSuccess2);
  if (!bSuccess || !anSuccess2[0] || !anSuccess2[1] || !anSuccess2[2])
    return psATInfo->pfnBaseTransformer(psATInfo->pBaseCBData, nPoints, x, y,
                                        panSuccess);

  /* -------------------------------------------------------------------- */
  /*      Is the error at the middle acceptable relative to an            */
  /*      interpolation of the middle position?                           */
  /* -------------------------------------------------------------------- */
  dfDeltaX = (x2[2] - x2[0]) / (x[nPoints - 1] - x[0]);
  dfDeltaY = (y2[2] - y2[0]) / (x[nPoints - 1] - x[0]);

  dfError = fabs((x2[0] + dfDeltaX * (x[nMiddle] - x[0])) - x2[1]) +
            fabs((y2[0] + dfDeltaY * (x[nMiddle] - x[0])) - y2[1]);

  if (dfError > psATInfo->dfMaxError) {
    bSuccess = msApproxTransformer(psATInfo, nMiddle, x, y, panSuccess);

    if (!bSuccess) {
      return psATInfo->pfnBaseTransformer(psATInfo->pBaseCBData, nPoints, x, y,
                                          panSuccess);
    }

    bSuccess = msApproxTransformer(psATInfo, nPoints - nMiddle, x + nMiddle,
                                   y + nMiddle, panSuccess + nMiddle);

    if (!bSuccess) {
      return psATInfo->pfnBaseTransformer(psATInfo->pBaseCBData, nPoints, x, y,
                                          panSuccess);
    }

    return 1;
  }

  /* -------------------------------------------------------------------- */
  /*      Error is OK, linearly interpolate all points along line.        */
  /* -------------------------------------------------------------------- */
  for (i = nPoints - 1; i >= 0; i--) {
    dfDist = (x[i] - x[0]);
    y[i] = y2[0] + dfDeltaY * dfDist;
    x[i] = x2[0] + dfDeltaX * dfDist;
    panSuccess[i] = 1;
  }

  return 1;
}

/************************************************************************/
/*                       msTransformMapToSource()                       */
/*                                                                      */
/*      Compute the extents of the current map view if transformed      */
/*      onto the source raster.                                         */
/************************************************************************/

static int
msTransformMapToSource(int nDstXSize, int nDstYSize, double *adfDstGeoTransform,
                       projectionObj *psDstProj, int nSrcXSize, int nSrcYSize,
                       double *adfInvSrcGeoTransform, projectionObj *psSrcProj,
                       rectObj *psSrcExtent, int bUseGrid)

{
  int nFailures = 0;

#define EDGE_STEPS 10
#define MAX_SIZE ((EDGE_STEPS + 1) * (EDGE_STEPS + 1))

  int i, nSamples = 0, bOutInit = 0;
  double dfRatio;
  double x[MAX_SIZE], y[MAX_SIZE];
#if PROJ_VERSION_MAJOR < 6
  double z[MAX_SIZE];
#endif

  /* -------------------------------------------------------------------- */
  /*      Collect edges in map image pixel/line coordinates               */
  /* -------------------------------------------------------------------- */
  if (!bUseGrid) {
    for (dfRatio = 0.0; dfRatio <= 1.001; dfRatio += (1.0 / EDGE_STEPS)) {
      assert(nSamples < MAX_SIZE);
      x[nSamples] = dfRatio * nDstXSize;
      y[nSamples++] = 0.0;
      x[nSamples] = dfRatio * nDstXSize;
      y[nSamples++] = nDstYSize;
      x[nSamples] = 0.0;
      y[nSamples++] = dfRatio * nDstYSize;
      x[nSamples] = nDstXSize;
      y[nSamples++] = dfRatio * nDstYSize;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Collect a grid in the hopes of a more accurate region.          */
  /* -------------------------------------------------------------------- */
  else {
    double dfRatio2;

    for (dfRatio = 0.0; dfRatio <= 1.001; dfRatio += (1.0 / EDGE_STEPS)) {
      for (dfRatio2 = 0.0; dfRatio2 <= 1.001; dfRatio2 += (1.0 / EDGE_STEPS)) {
        assert(nSamples < MAX_SIZE);
        x[nSamples] = dfRatio2 * nDstXSize;
        y[nSamples++] = dfRatio * nDstYSize;
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      transform to map georeferenced units                            */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < nSamples; i++) {
    double x_out, y_out;

    x_out = adfDstGeoTransform[0] + x[i] * adfDstGeoTransform[1] +
            y[i] * adfDstGeoTransform[2];

    y_out = adfDstGeoTransform[3] + x[i] * adfDstGeoTransform[4] +
            y[i] * adfDstGeoTransform[5];

    x[i] = x_out;
    y[i] = y_out;
#if PROJ_VERSION_MAJOR < 6
    z[i] = 0.0;
#endif
  }

  /* -------------------------------------------------------------------- */
  /*      Transform to layer georeferenced coordinates.                   */
  /* -------------------------------------------------------------------- */
  if (psDstProj->proj && psSrcProj->proj) {
#if PROJ_VERSION_MAJOR >= 6
    reprojectionObj *reprojector =
        msProjectCreateReprojector(psDstProj, psSrcProj);
    if (!reprojector)
      return MS_FALSE;
    if (msProjectTransformPoints(reprojector, nSamples, x, y) != MS_SUCCESS) {
      msProjectDestroyReprojector(reprojector);
      return MS_FALSE;
    }
    msProjectDestroyReprojector(reprojector);
#else
    int tr_result;
    if (msProjIsGeographicCRS(psDstProj)) {
      for (i = 0; i < nSamples; i++) {
        x[i] = x[i] * DEG_TO_RAD;
        y[i] = y[i] * DEG_TO_RAD;
      }
    }

    msAcquireLock(TLOCK_PROJ);
    tr_result =
        pj_transform(psDstProj->proj, psSrcProj->proj, nSamples, 1, x, y, z);
    msReleaseLock(TLOCK_PROJ);

    if (tr_result != 0)
      return MS_FALSE;

    if (msProjIsGeographicCRS(psSrcProj)) {
      for (i = 0; i < nSamples; i++) {
        if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
          x[i] = x[i] * RAD_TO_DEG;
          y[i] = y[i] * RAD_TO_DEG;
        }
      }
    }
#endif
  }

  /* -------------------------------------------------------------------- */
  /*      If we just using the edges (not a grid) and we go some          */
  /*      errors, then we need to restart using a grid pattern.           */
  /* -------------------------------------------------------------------- */
  if (!bUseGrid) {
    for (i = 0; i < nSamples; i++) {
      if (x[i] == HUGE_VAL || y[i] == HUGE_VAL) {
        return msTransformMapToSource(
            nDstXSize, nDstYSize, adfDstGeoTransform, psDstProj, nSrcXSize,
            nSrcYSize, adfInvSrcGeoTransform, psSrcProj, psSrcExtent, 1);
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      transform to layer raster coordinates, and collect bounds.      */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < nSamples; i++) {
    double x_out, y_out;

    if (x[i] == HUGE_VAL || y[i] == HUGE_VAL) {
      nFailures++;
      continue;
    }

    x_out = adfInvSrcGeoTransform[0] + x[i] * adfInvSrcGeoTransform[1] +
            y[i] * adfInvSrcGeoTransform[2];
    y_out = adfInvSrcGeoTransform[3] + x[i] * adfInvSrcGeoTransform[4] +
            y[i] * adfInvSrcGeoTransform[5];

    if (!bOutInit) {
      psSrcExtent->minx = psSrcExtent->maxx = x_out;
      psSrcExtent->miny = psSrcExtent->maxy = y_out;
      bOutInit = 1;
    } else {
      psSrcExtent->minx = MS_MIN(psSrcExtent->minx, x_out);
      psSrcExtent->maxx = MS_MAX(psSrcExtent->maxx, x_out);
      psSrcExtent->miny = MS_MIN(psSrcExtent->miny, y_out);
      psSrcExtent->maxy = MS_MAX(psSrcExtent->maxy, y_out);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Deal with discontinuities related to lon_wrap=XXX in source     */
  /*      projection. In that case we must check if the points at         */
  /*      lon_wrap +/- 180deg are in the output raster.                   */
  /* -------------------------------------------------------------------- */
  if (bOutInit && msProjIsGeographicCRS(psSrcProj)) {
    double dfLonWrap = 0;
    int bHasLonWrap = msProjectHasLonWrap(psSrcProj, &dfLonWrap);

    if (bHasLonWrap) {
      double x2[2], y2[2];
#if PROJ_VERSION_MAJOR < 6
      double z2[2];
#endif
      int nCountY = 0;
      double dfY = 0.0;
      double dfXMinOut = 0.0;
      double dfYMinOut = 0.0;
      double dfXMaxOut = 0.0;
      double dfYMaxOut = 0.0;
      const double dfHalfRes = adfDstGeoTransform[1] / 2;

      /* Find out average y coordinate in src projection */
      for (i = 0; i < nSamples; i++) {
        if (y[i] != HUGE_VAL) {
          dfY += y[i];
          nCountY++;
        }
      }
      dfY /= nCountY;

      /* Compute bounds of output raster */
      for (i = 0; i < 4; i++) {
        double dfX =
            adfDstGeoTransform[0] +
            ((i == 1 || i == 2) ? nDstXSize : 0) * adfDstGeoTransform[1] +
            ((i == 1 || i == 3) ? nDstYSize : 0) * adfDstGeoTransform[2];
        double dfY =
            adfDstGeoTransform[3] +
            ((i == 1 || i == 2) ? nDstXSize : 0) * adfDstGeoTransform[4] +
            ((i == 1 || i == 3) ? nDstYSize : 0) * adfDstGeoTransform[5];
        if (i == 0 || dfX < dfXMinOut)
          dfXMinOut = dfX;
        if (i == 0 || dfY < dfYMinOut)
          dfYMinOut = dfY;
        if (i == 0 || dfX > dfXMaxOut)
          dfXMaxOut = dfX;
        if (i == 0 || dfY > dfYMaxOut)
          dfYMaxOut = dfY;
      }

      x2[0] = dfLonWrap - 180 + 1e-7;
      y2[0] = dfY;

      x2[1] = dfLonWrap + 180 - 1e-7;
      y2[1] = dfY;

#if PROJ_VERSION_MAJOR >= 6
      {
        reprojectionObj *reprojector =
            msProjectCreateReprojector(psSrcProj, psDstProj);
        if (reprojector) {
          msProjectTransformPoints(reprojector, 2, x2, y2);
          msProjectDestroyReprojector(reprojector);
        }
      }
#else
      z2[0] = 0.0;
      z2[1] = 0.0;
      msAcquireLock(TLOCK_PROJ);
      pj_transform(psSrcProj->proj, psDstProj->proj, 2, 1, x2, y2, z2);
      msReleaseLock(TLOCK_PROJ);
#endif

      if (x2[0] >= dfXMinOut - dfHalfRes && x2[0] <= dfXMaxOut + dfHalfRes &&
          y2[0] >= dfYMinOut && y2[0] <= dfYMaxOut) {
        double x_out = adfInvSrcGeoTransform[0] +
                       (dfLonWrap - 180) * adfInvSrcGeoTransform[1] +
                       dfY * adfInvSrcGeoTransform[2];
        double y_out = adfInvSrcGeoTransform[3] +
                       (dfLonWrap - 180) * adfInvSrcGeoTransform[4] +
                       dfY * adfInvSrcGeoTransform[5];

        /* Does the raster cover, at least, a whole 360 deg range ? */
        if (nSrcXSize >= (int)(adfInvSrcGeoTransform[1] * 360)) {
          psSrcExtent->minx = 0;
          psSrcExtent->maxx = nSrcXSize;
        } else {
          psSrcExtent->minx = MS_MIN(psSrcExtent->minx, x_out);
          psSrcExtent->maxx = MS_MAX(psSrcExtent->maxx, x_out);
        }
        psSrcExtent->miny = MS_MIN(psSrcExtent->miny, y_out);
        psSrcExtent->maxy = MS_MAX(psSrcExtent->maxy, y_out);
      }

      if (x2[1] >= dfXMinOut - dfHalfRes && x2[1] <= dfXMaxOut + dfHalfRes &&
          y2[1] >= dfYMinOut && y2[1] <= dfYMaxOut) {
        double x_out = adfInvSrcGeoTransform[0] +
                       (dfLonWrap + 180) * adfInvSrcGeoTransform[1] +
                       dfY * adfInvSrcGeoTransform[2];
        double y_out = adfInvSrcGeoTransform[3] +
                       (dfLonWrap + 180) * adfInvSrcGeoTransform[4] +
                       dfY * adfInvSrcGeoTransform[5];

        /* Does the raster cover, at least, a whole 360 deg range ? */
        if (nSrcXSize >= (int)(adfInvSrcGeoTransform[1] * 360)) {
          psSrcExtent->minx = 0;
          psSrcExtent->maxx = nSrcXSize;
        } else {
          psSrcExtent->minx = MS_MIN(psSrcExtent->minx, x_out);
          psSrcExtent->maxx = MS_MAX(psSrcExtent->maxx, x_out);
        }
        psSrcExtent->miny = MS_MIN(psSrcExtent->miny, y_out);
        psSrcExtent->maxy = MS_MAX(psSrcExtent->maxy, y_out);
      }
    }
  }

  if (!bOutInit)
    return MS_FALSE;

  /* -------------------------------------------------------------------- */
  /*      If we had some failures, we need to expand the region to        */
  /*      represent our very coarse sampling grid.                        */
  /* -------------------------------------------------------------------- */
  if (nFailures > 0) {
    int nGrowAmountX =
        (int)(psSrcExtent->maxx - psSrcExtent->minx) / EDGE_STEPS + 1;
    int nGrowAmountY =
        (int)(psSrcExtent->maxy - psSrcExtent->miny) / EDGE_STEPS + 1;

    psSrcExtent->minx = MS_MAX(psSrcExtent->minx - nGrowAmountX, 0);
    psSrcExtent->miny = MS_MAX(psSrcExtent->miny - nGrowAmountY, 0);
    psSrcExtent->maxx = MS_MIN(psSrcExtent->maxx + nGrowAmountX, nSrcXSize);
    psSrcExtent->maxy = MS_MIN(psSrcExtent->maxy + nGrowAmountY, nSrcYSize);
  }

  return MS_TRUE;
}

/************************************************************************/
/*                        msResampleGDALToMap()                         */
/************************************************************************/

int msResampleGDALToMap(mapObj *map, layerObj *layer, imageObj *image,
                        rasterBufferObj *rb, GDALDatasetH hDS)

{
  int nSrcXSize, nSrcYSize, nDstXSize, nDstYSize;
  int result, bSuccess;
  double adfSrcGeoTransform[6], adfDstGeoTransform[6];
  double adfInvSrcGeoTransform[6], dfNominalCellSize;
  rectObj sSrcExtent = {0}, sOrigSrcExtent;
  mapObj sDummyMap;
  imageObj *srcImage;
  void *pTCBData;
  void *pACBData;
  char **papszAlteredProcessing = NULL;
  int nLoadImgXSize, nLoadImgYSize;
  double dfOversampleRatio;
  rasterBufferObj src_rb, *psrc_rb = NULL, *mask_rb = NULL;
  int bAddPixelMargin = MS_TRUE;
  int bWrapAtLeftRight = MS_FALSE;

  const char *resampleMode = CSLFetchNameValue(layer->processing, "RESAMPLE");

  if (resampleMode == NULL)
    resampleMode = "NEAREST";

  if (layer->mask) {
    int ret, maskLayerIdx;
    layerObj *maskLayer;
    maskLayerIdx = msGetLayerIndex(map, layer->mask);
    if (maskLayerIdx == -1) {
      msSetError(MS_MISCERR, "Invalid mask layer specified",
                 "msResampleGDALToMap()");
      return -1;
    }
    maskLayer = GET_LAYER(map, maskLayerIdx);
    mask_rb = msSmallCalloc(1, sizeof(rasterBufferObj));
    ret = MS_IMAGE_RENDERER(maskLayer->maskimage)
              ->getRasterBufferHandle(maskLayer->maskimage, mask_rb);
    if (ret != MS_SUCCESS) {
      free(mask_rb);
      return -1;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      We will require source and destination to have a valid          */
  /*      projection object.                                              */
  /* -------------------------------------------------------------------- */
  if (map->projection.proj == NULL || layer->projection.proj == NULL) {
    if (layer->debug)
      msDebug("msResampleGDALToMap(): "
              "Either map or layer projection is NULL, assuming compatible.\n");
  }

  /* -------------------------------------------------------------------- */
  /*      Initialize some information.                                    */
  /* -------------------------------------------------------------------- */
  nDstXSize = image->width;
  nDstYSize = image->height;

  memcpy(adfDstGeoTransform, map->gt.geotransform, sizeof(double) * 6);

  msGetGDALGeoTransform(hDS, map, layer, adfSrcGeoTransform);

  nSrcXSize = GDALGetRasterXSize(hDS);
  nSrcYSize = GDALGetRasterYSize(hDS);

  InvGeoTransform(adfSrcGeoTransform, adfInvSrcGeoTransform);

  /* -------------------------------------------------------------------- */
  /*      We need to find the extents in the source layer projection      */
  /*      of the output requested region.  We will accomplish this by     */
  /*      collecting the extents of a region around the edge of the       */
  /*      destination chunk.                                              */
  /* -------------------------------------------------------------------- */
  if (CSLFetchBoolean(layer->processing, "LOAD_WHOLE_IMAGE", FALSE))
    bSuccess = FALSE;
  else {
    bSuccess = msTransformMapToSource(nDstXSize, nDstYSize, adfDstGeoTransform,
                                      &(map->projection), nSrcXSize, nSrcYSize,
                                      adfInvSrcGeoTransform,
                                      &(layer->projection), &sSrcExtent, FALSE);
    if (bSuccess) {
      /* -------------------------------------------------------------------- */
      /*      Repeat transformation for a rectangle interior to the output    */
      /*      requested region.  If the latter results in a more extreme y    */
      /*      extent, then extend extents in source layer projection to       */
      /*      southern/northing bounds and entire x extent.                   */
      /* -------------------------------------------------------------------- */
      memcpy(&sOrigSrcExtent, &sSrcExtent, sizeof(sSrcExtent));
      adfDstGeoTransform[0] = adfDstGeoTransform[0] + adfDstGeoTransform[1];
      adfDstGeoTransform[3] = adfDstGeoTransform[3] + adfDstGeoTransform[5];
      bSuccess = msTransformMapToSource(
          nDstXSize - 2, nDstYSize - 2, adfDstGeoTransform, &(map->projection),
          nSrcXSize, nSrcYSize, adfInvSrcGeoTransform, &(layer->projection),
          &sSrcExtent, FALSE);
      /* Reset this array to its original value! */
      memcpy(adfDstGeoTransform, map->gt.geotransform, sizeof(double) * 6);

      if (bSuccess) {
        if (sSrcExtent.maxy > sOrigSrcExtent.maxy ||
            sSrcExtent.miny < sOrigSrcExtent.miny) {
          msDebug("msTransformMapToSource(): extending bounds.\n");
          sOrigSrcExtent.minx = 0;
          sOrigSrcExtent.maxx = nSrcXSize;
          if (sSrcExtent.maxy > sOrigSrcExtent.maxy)
            sOrigSrcExtent.maxy = nSrcYSize;
          if (sSrcExtent.miny < sOrigSrcExtent.miny)
            sOrigSrcExtent.miny = 0;
        }
      }
      memcpy(&sSrcExtent, &sOrigSrcExtent, sizeof(sOrigSrcExtent));
      bSuccess = TRUE;
    }
  }
  /* -------------------------------------------------------------------- */
  /*      If the transformation failed, it is likely that we have such    */
  /*      broad extents that the projection transformation failed at      */
  /*      points around the extents.  If so, we will assume we need       */
  /*      the whole raster.  This and later assumptions are likely to     */
  /*      result in the raster being loaded at a higher resolution        */
  /*      than really needed but should give decent results.              */
  /* -------------------------------------------------------------------- */
  if (!bSuccess) {
    if (layer->debug) {
      if (CSLFetchBoolean(layer->processing, "LOAD_WHOLE_IMAGE", FALSE))
        msDebug("msResampleGDALToMap(): "
                "LOAD_WHOLE_IMAGE set, loading whole image.\n");
      else
        msDebug(
            "msTransformMapToSource(): "
            "pj_transform() failed.  Out of bounds?  Loading whole image.\n");
    }

    sSrcExtent.minx = 0;
    sSrcExtent.maxx = nSrcXSize;
    sSrcExtent.miny = 0;
    sSrcExtent.maxy = nSrcYSize;
  }

  /* -------------------------------------------------------------------- */
  /*      If requesting at the raster resolution, on pixel boundaries,    */
  /*      and no reprojection is  involved, we don't need any resampling. */
  /*      And if they match an integral subsampling factor, no need to    */
  /*      add pixel margin.                                               */
  /*      This optimization helps a lot when operating with mode=tile     */
  /*      and that the underlying raster is tiled and share the same      */
  /*      tiling scheme as the queried tile mode.                         */
  /* -------------------------------------------------------------------- */
#define IS_ALMOST_INTEGER(x, eps) (fabs((x) - (int)((x) + 0.5)) < (eps))

  if (adfSrcGeoTransform[1] > 0.0 && adfSrcGeoTransform[2] == 0.0 &&
      adfSrcGeoTransform[4] == 0.0 && adfSrcGeoTransform[5] < 0.0 &&
      IS_ALMOST_INTEGER(sSrcExtent.minx, 0.1) &&
      IS_ALMOST_INTEGER(sSrcExtent.miny, 0.1) &&
      IS_ALMOST_INTEGER(sSrcExtent.maxx, 0.1) &&
      IS_ALMOST_INTEGER(sSrcExtent.maxy, 0.1) &&
      !msProjectionsDiffer(&(map->projection), &(layer->projection))) {
    double dfXFactor, dfYFactor;

    sSrcExtent.minx = (int)(sSrcExtent.minx + 0.5);
    sSrcExtent.miny = (int)(sSrcExtent.miny + 0.5);
    sSrcExtent.maxx = (int)(sSrcExtent.maxx + 0.5);
    sSrcExtent.maxy = (int)(sSrcExtent.maxy + 0.5);

    if ((int)(sSrcExtent.maxx - sSrcExtent.minx + 0.5) == nDstXSize &&
        (int)(sSrcExtent.maxy - sSrcExtent.miny + 0.5) == nDstYSize) {
      if (layer->debug)
        msDebug("msResampleGDALToMap(): Request matching raster resolution and "
                "pixel boundaries. "
                "No need to do resampling/reprojection.\n");
      msFree(mask_rb);
      return msDrawRasterLayerGDAL(map, layer, image, rb, hDS);
    }

    dfXFactor = (sSrcExtent.maxx - sSrcExtent.minx) / nDstXSize;
    dfYFactor = (sSrcExtent.maxy - sSrcExtent.miny) / nDstYSize;
    if (IS_ALMOST_INTEGER(dfXFactor, 1e-5) &&
        IS_ALMOST_INTEGER(dfYFactor, 1e-5) &&
        IS_ALMOST_INTEGER(sSrcExtent.minx / dfXFactor, 0.1) &&
        IS_ALMOST_INTEGER(sSrcExtent.miny / dfXFactor, 0.1) &&
        IS_ALMOST_INTEGER(sSrcExtent.maxx / dfYFactor, 0.1) &&
        IS_ALMOST_INTEGER(sSrcExtent.maxy / dfYFactor, 0.1)) {
      bAddPixelMargin = MS_FALSE;
      if (layer->debug)
        msDebug(
            "msResampleGDALToMap(): Request matching raster resolution "
            "and pixel boundaries matching an integral subsampling factor\n");
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Project desired extents out by 2 pixels, and then strip to      */
  /*      available data.                                                 */
  /* -------------------------------------------------------------------- */
  memcpy(&sOrigSrcExtent, &sSrcExtent, sizeof(sSrcExtent));

  if (bAddPixelMargin) {
    sSrcExtent.minx = floor(sSrcExtent.minx - 1.0);
    sSrcExtent.maxx = ceil(sSrcExtent.maxx + 1.0);
    sSrcExtent.miny = floor(sSrcExtent.miny - 1.0);
    sSrcExtent.maxy = ceil(sSrcExtent.maxy + 1.0);
  }

  sSrcExtent.minx = MS_MAX(0, sSrcExtent.minx);
  sSrcExtent.maxx = MS_MIN(sSrcExtent.maxx, nSrcXSize);
  sSrcExtent.miny = MS_MAX(sSrcExtent.miny, 0);
  sSrcExtent.maxy = MS_MIN(sSrcExtent.maxy, nSrcYSize);

  if (sSrcExtent.maxx <= sSrcExtent.minx ||
      sSrcExtent.maxy <= sSrcExtent.miny) {
    if (layer->debug)
      msDebug("msResampleGDALToMap(): no overlap ... no result.\n");
    msFree(mask_rb);
    return 0;
  }

  /* -------------------------------------------------------------------- */
  /*      Determine desired oversampling ratio.  Default to 2.0 if not    */
  /*      otherwise set.                                                  */
  /* -------------------------------------------------------------------- */
  dfOversampleRatio = 2.0;

  if (CSLFetchNameValue(layer->processing, "OVERSAMPLE_RATIO") != NULL) {
    dfOversampleRatio =
        atof(CSLFetchNameValue(layer->processing, "OVERSAMPLE_RATIO"));
  }

  /* -------------------------------------------------------------------- */
  /*      Decide on a resolution to read from the source image at.  We    */
  /*      will operate from full resolution data, if we are requesting    */
  /*      at near to full resolution.  Otherwise we will read the data    */
  /*      at twice the resolution of the eventual map.                    */
  /* -------------------------------------------------------------------- */
  dfNominalCellSize = sqrt(adfSrcGeoTransform[1] * adfSrcGeoTransform[1] +
                           adfSrcGeoTransform[2] * adfSrcGeoTransform[2]);

  /* Check first that the requested extent is not well beyond than the source */
  /* raster. This might be the case for example if asking to visualize */
  /* -180,-89,180,90 in EPSG:4326 from a raster in Arctic Polar Stereographic */
  /* But restrict that to rasters of modest size, otherwise we may end up */
  /* requesting very large dimensions in other legit reprojection cases */
  /* See https://github.com/mapserver/mapserver/issues/5402 */
  if (!(sOrigSrcExtent.minx <= -4 * nSrcXSize &&
        sOrigSrcExtent.miny <= -4 * nSrcYSize &&
        sOrigSrcExtent.maxx >= 5 * nSrcXSize &&
        sOrigSrcExtent.maxy >= 5 * nSrcYSize && nSrcXSize < 4000 &&
        nSrcYSize < 4000) &&
      (sOrigSrcExtent.maxx - sOrigSrcExtent.minx) >
          dfOversampleRatio * nDstXSize &&
      !CSLFetchBoolean(layer->processing, "LOAD_FULL_RES_IMAGE", FALSE))
    sDummyMap.cellsize =
        (dfNominalCellSize * (sOrigSrcExtent.maxx - sOrigSrcExtent.minx)) /
        (dfOversampleRatio * nDstXSize);
  else
    sDummyMap.cellsize = dfNominalCellSize;

  nLoadImgXSize = MS_MAX(1, (int)(sSrcExtent.maxx - sSrcExtent.minx) *
                                (dfNominalCellSize / sDummyMap.cellsize));
  nLoadImgYSize = MS_MAX(1, (int)(sSrcExtent.maxy - sSrcExtent.miny) *
                                (dfNominalCellSize / sDummyMap.cellsize));

  /*
  ** Because the previous calculation involved some round off, we need
  ** to fixup the cellsize to ensure the map region represents the whole
  ** RAW_WINDOW (at least in X).  Re: bug 1715.
  */
  sDummyMap.cellsize =
      ((sSrcExtent.maxx - sSrcExtent.minx) * dfNominalCellSize) / nLoadImgXSize;

  if (layer->debug)
    msDebug("msResampleGDALToMap in effect: cellsize = %f\n",
            sDummyMap.cellsize);

  adfSrcGeoTransform[0] += +adfSrcGeoTransform[1] * sSrcExtent.minx +
                           adfSrcGeoTransform[2] * sSrcExtent.miny;
  adfSrcGeoTransform[1] *= (sDummyMap.cellsize / dfNominalCellSize);
  adfSrcGeoTransform[2] *= (sDummyMap.cellsize / dfNominalCellSize);

  adfSrcGeoTransform[3] += +adfSrcGeoTransform[4] * sSrcExtent.minx +
                           adfSrcGeoTransform[5] * sSrcExtent.miny;
  adfSrcGeoTransform[4] *= (sDummyMap.cellsize / dfNominalCellSize);
  adfSrcGeoTransform[5] *= (sDummyMap.cellsize / dfNominalCellSize);

  /* In the non-rotated case, make sure that the geotransform exactly */
  /* matches the sSrcExtent, even if that generates non-square pixels (#1715) */
  /* The rotated case should ideally be dealt with, but not for now... */
  if (adfSrcGeoTransform[2] == 0 && adfSrcGeoTransform[4] == 0 &&
      adfSrcGeoTransform[5] < 0 &&
      /* But do that only if the pixels were square before, otherwise */
      /* this is going to mess with source rasters whose pixels aren't at */
      /* all square (#5445) */
      fabs(fabs(adfSrcGeoTransform[1]) - fabs(adfSrcGeoTransform[5])) <
          0.01 * fabs(adfSrcGeoTransform[1])) {
    adfSrcGeoTransform[1] =
        (sSrcExtent.maxx - sSrcExtent.minx) * dfNominalCellSize / nLoadImgXSize;
    adfSrcGeoTransform[5] = -(sSrcExtent.maxy - sSrcExtent.miny) *
                            dfNominalCellSize / nLoadImgYSize;
  }

  papszAlteredProcessing = CSLDuplicate(layer->processing);
  papszAlteredProcessing = CSLSetNameValue(
      papszAlteredProcessing, "RAW_WINDOW",
      CPLSPrintf("%d %d %d %d", (int)sSrcExtent.minx, (int)sSrcExtent.miny,
                 (int)(sSrcExtent.maxx - sSrcExtent.minx),
                 (int)(sSrcExtent.maxy - sSrcExtent.miny)));

  /* -------------------------------------------------------------------- */
  /*      We clone this without referencing it knowing that the           */
  /*      srcImage will take a reference on it.  The sDummyMap is         */
  /*      destroyed off the stack, so the missing map reference is        */
  /*      never a problem.  The image's dereference of the                */
  /*      outputformat during the msFreeImage() calls will result in      */
  /*      the output format being cleaned up.                             */
  /*                                                                      */
  /*      We make a copy so we can easily modify the outputformat used    */
  /*      for the temporary image to include transparentency support.     */
  /* -------------------------------------------------------------------- */
  sDummyMap.outputformat = msCloneOutputFormat(image->format);
  sDummyMap.width = nLoadImgXSize;
  sDummyMap.height = nLoadImgYSize;
  sDummyMap.mappath = map->mappath;
  sDummyMap.shapepath = map->shapepath;

  /* -------------------------------------------------------------------- */
  /*      If we are working in 256 color GD mode, allocate 0 as the       */
  /*      transparent color on the temporary image so it will be          */
  /*      initialized to see-through.  We pick an arbitrary rgb tuple     */
  /*      as our transparent color, but ensure it is initalized in the    */
  /*      map so that normal transparent avoidance will apply.            */
  /* -------------------------------------------------------------------- */
  if (MS_RENDERER_PLUGIN(sDummyMap.outputformat)) {
    assert(rb);
    msInitializeRendererVTable(sDummyMap.outputformat);
    assert(sDummyMap.outputformat->imagemode == MS_IMAGEMODE_RGB ||
           sDummyMap.outputformat->imagemode == MS_IMAGEMODE_RGBA);

    sDummyMap.outputformat->transparent = MS_TRUE;
    sDummyMap.outputformat->imagemode = MS_IMAGEMODE_RGBA;
    MS_INIT_COLOR(sDummyMap.imagecolor, -1, -1, -1, 255);
  }

  /* -------------------------------------------------------------------- */
  /*      Setup a dummy map object we can use to read from the source     */
  /*      raster, with the newly established extents, and resolution.     */
  /* -------------------------------------------------------------------- */
  srcImage = msImageCreate(nLoadImgXSize, nLoadImgYSize, sDummyMap.outputformat,
                           NULL, NULL, map->resolution, map->defresolution,
                           &(sDummyMap.imagecolor));

  if (srcImage == NULL) {
    msFree(mask_rb);
    return -1; /* msSetError() should have been called already */
  }

  if (MS_RENDERER_PLUGIN(srcImage->format)) {
    psrc_rb = &src_rb;
    memset(psrc_rb, 0, sizeof(rasterBufferObj));
    if (srcImage->format->vtable->supports_pixel_buffer) {
      if (MS_UNLIKELY(MS_FAILURE ==
                      srcImage->format->vtable->getRasterBufferHandle(
                          srcImage, psrc_rb))) {
        msFree(mask_rb);
        return -1;
      }
    } else {
      if (MS_UNLIKELY(
              MS_FAILURE ==
              srcImage->format->vtable->initializeRasterBuffer(
                  psrc_rb, nLoadImgXSize, nLoadImgYSize, MS_IMAGEMODE_RGBA))) {
        msFree(mask_rb);
        return -1;
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Draw into the temporary image.  Temporarily replace the         */
  /*      layer processing directive so that we use our RAW_WINDOW.       */
  /* -------------------------------------------------------------------- */
  {
    char **papszSavedProcessing = layer->processing;
    char *origMask = layer->mask;
    layer->mask = NULL;
    layer->processing = papszAlteredProcessing;

    result = msDrawRasterLayerGDAL(&sDummyMap, layer, srcImage, psrc_rb, hDS);

    layer->processing = papszSavedProcessing;
    layer->mask = origMask;
    CSLDestroy(papszAlteredProcessing);

    if (result) {
      if (MS_RENDERER_PLUGIN(srcImage->format) &&
          !srcImage->format->vtable->supports_pixel_buffer)
        msFreeRasterBuffer(psrc_rb);

      msFreeImage(srcImage);
      msFree(mask_rb);

      return result;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Setup transformations between our source image, and the         */
  /*      target map image.                                               */
  /* -------------------------------------------------------------------- */
  pTCBData = msInitProjTransformer(&(layer->projection), adfSrcGeoTransform,
                                   &(map->projection), adfDstGeoTransform);

  if (pTCBData == NULL) {
    if (layer->debug)
      msDebug("msInitProjTransformer() returned NULL.\n");
    if (MS_RENDERER_PLUGIN(srcImage->format) &&
        !srcImage->format->vtable->supports_pixel_buffer)
      msFreeRasterBuffer(psrc_rb);
    msFreeImage(srcImage);
    msFree(mask_rb);
    return MS_PROJERR;
  }

  /* -------------------------------------------------------------------- */
  /*      It is cheaper to use linear approximations as long as our       */
  /*      error is modest (less than 0.333 pixels).                       */
  /* -------------------------------------------------------------------- */
  pACBData = msInitApproxTransformer(msProjTransformer, pTCBData, 0.333);

  if (msProjIsGeographicCRS(&(layer->projection))) {
    /* Does the raster cover a whole 360 deg range ? */
    if (nSrcXSize == (int)(adfInvSrcGeoTransform[1] * 360 + 0.5))
      bWrapAtLeftRight = MS_TRUE;
  }

  /* -------------------------------------------------------------------- */
  /*      Perform the resampling.                                         */
  /* -------------------------------------------------------------------- */
  if (EQUAL(resampleMode, "AVERAGE"))
    result = msAverageRasterResampler(srcImage, psrc_rb, image, rb,
                                      msApproxTransformer, pACBData,
                                      layer->debug, mask_rb);
  else if (EQUAL(resampleMode, "BILINEAR"))
    result = msBilinearRasterResampler(srcImage, psrc_rb, image, rb,
                                       msApproxTransformer, pACBData,
                                       layer->debug, mask_rb, bWrapAtLeftRight);
  else
    result = msNearestRasterResampler(srcImage, psrc_rb, image, rb,
                                      msApproxTransformer, pACBData,
                                      layer->debug, mask_rb, bWrapAtLeftRight);

  /* -------------------------------------------------------------------- */
  /*      cleanup                                                         */
  /* -------------------------------------------------------------------- */
  msFree(mask_rb);
  if (MS_RENDERER_PLUGIN(srcImage->format) &&
      !srcImage->format->vtable->supports_pixel_buffer)
    msFreeRasterBuffer(psrc_rb);
  msFreeImage(srcImage);

  msFreeProjTransformer(pTCBData);
  msFreeApproxTransformer(pACBData);

  return result;
}

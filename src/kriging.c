/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Kriging / Gaussian-Process layer interpolation via a localized
 *           Nearest-Neighbor Gaussian Process (NNGP). Each output pixel is
 *           predicted from its m nearest samples (ordinary kriging by default),
 *           so cost per pixel depends on the neighbor count, not the sample
 *           count -- the same prediction reduction NNGP uses to scale.
 *           Emits two byte bands: 1 = predictive mean, 2 = predictive std dev
 *           (the kriging standard error). Select with PROCESSING "BANDS=n".
 *
 * Author:   Hermes L. Herrera and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 2024 Regents of the University of Minnesota.
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

#include "mapserver.h"
#include "kriging_nngp.h"
#include <float.h>

/* one sample point, used only for coincidence dedup */
typedef struct {
  float x, y, z;
} krSample;

static int krSampleCmp(const void *a, const void *b) {
  const krSample *p = (const krSample *)a, *q = (const krSample *)b;
  if (p->y != q->y)
    return p->y < q->y ? -1 : 1;
  if (p->x != q->x)
    return p->x < q->x ? -1 : 1;
  return 0;
}

static unsigned char krClamp(double v) {
  if (v <= 0.0)
    return 0;
  if (v >= 255.0)
    return 255;
  return (unsigned char)(v + 0.5);
}

void msKrigingProcessing(layerObj *layer,
                         interpolationProcessingParams *interpParams) {
  const char *key;

  key = msLayerGetProcessingKey(layer, "KRIGING_MODEL");
  if (key && !strcasecmp(key, "GAUSSIAN"))
    interpParams->kriging_model = KR_GAUSSIAN;
  else if (key && !strcasecmp(key, "SPHERICAL"))
    interpParams->kriging_model = KR_SPHERICAL;
  else
    interpParams->kriging_model = KR_EXPONENTIAL; /* Matern 1/2, the default */

  key = msLayerGetProcessingKey(layer, "KRIGING_TYPE");
  if (key && !strcasecmp(key, "SIMPLE"))
    interpParams->kriging_type = KR_SIMPLE;
  else
    interpParams->kriging_type = KR_ORDINARY;

  key = msLayerGetProcessingKey(layer, "KRIGING_NEIGHBORS");
  interpParams->kriging_neighbors = key ? atoi(key) : 16;
  if (interpParams->kriging_neighbors < 1)
    interpParams->kriging_neighbors = 1;

  key = msLayerGetProcessingKey(layer, "KRIGING_RANGE");
  interpParams->kriging_range = key ? atof(key) : 0.0; /* 0 => auto */

  key = msLayerGetProcessingKey(layer, "KRIGING_SILL");
  interpParams->kriging_sill = key ? atof(key) : 0.0; /* 0 => auto */

  key = msLayerGetProcessingKey(layer, "KRIGING_NUGGET");
  interpParams->kriging_nugget = key ? atof(key) : 0.0;

  /* NNGP works in the image frame; locality comes from the neighbor count, not
   * a border expansion, so the search rect is not grown (as with IDW
   * default).*/
  interpParams->expand_searchrect = 0;
}

void msKriging(float *xyz, int width, int height, int npoints,
               interpolationProcessingParams *interpParams,
               unsigned char *iMean, unsigned char *iSD) {
  krSample *pts;
  double *sx, *sy, *sz;
  double sigma2, range, nugget, sum, mean;
  int n, i, m, px, py;
  kr_grid grid;
  kr_knn kn;
  int *nbidx;
  double *nbd2, *A, *k0, *w;
  int model = interpParams->kriging_model;
  int ktype = interpParams->kriging_type;

  if (npoints <= 0)
    return;

  /* Collapse coincident samples: two points on the same pixel give identical
   * rows in K, hence a singular system when nugget == 0. msInterpolationDataset
   * hands us a running per-pixel accumulation, so the max over a duplicate
   * group is that pixel's final total. Sort by (y,x) to bring duplicates
   * together. */
  pts = (krSample *)msSmallMalloc((size_t)npoints * sizeof(krSample));
  for (i = 0; i < npoints; i++) {
    pts[i].x = xyz[i * 3];
    pts[i].y = xyz[i * 3 + 1];
    pts[i].z = xyz[i * 3 + 2];
  }
  qsort(pts, npoints, sizeof(krSample), krSampleCmp);

  sx = (double *)msSmallMalloc((size_t)npoints * sizeof(double));
  sy = (double *)msSmallMalloc((size_t)npoints * sizeof(double));
  sz = (double *)msSmallMalloc((size_t)npoints * sizeof(double));
  n = 0;
  for (i = 0; i < npoints;) {
    int j = i;
    double best = pts[i].z;
    while (j < npoints && pts[j].x == pts[i].x && pts[j].y == pts[i].y) {
      if (pts[j].z > best)
        best = pts[j].z;
      j++;
    }
    sx[n] = pts[i].x;
    sy[n] = pts[i].y;
    sz[n] = best;
    n++;
    i = j;
  }
  free(pts);

  /* covariance parameters: honour the PROCESSING keys, else auto-fit them. */
  sigma2 = interpParams->kriging_sill;
  if (sigma2 <= 0.0) { /* auto sill = sample variance of the values */
    sum = 0.0;
    for (i = 0; i < n; i++)
      sum += sz[i];
    mean = sum / n;
    sigma2 = 0.0;
    for (i = 0; i < n; i++)
      sigma2 += (sz[i] - mean) * (sz[i] - mean);
    sigma2 = n > 1 ? sigma2 / (n - 1) : 1.0;
    if (sigma2 <= 0.0)
      sigma2 = 1.0;
  }
  range = interpParams->kriging_range;
  if (range <= 0.0) /* auto range ~ 1/3 of the image diagonal (in pixels) */
    range = 0.33 * sqrt((double)width * width + (double)height * height);
  nugget = interpParams->kriging_nugget;

  m = interpParams->kriging_neighbors;
  if (m > n)
    m = n;

  /* Build the neighbour index once, and size the per-solve scratch for the
   * bordered system (m+1) up front so the per-pixel loop never allocates. */
  if (kr_grid_build(&grid, sx, sy, n) != 0) {
    free(sx);
    free(sy);
    free(sz);
    return;
  }
  nbidx = (int *)msSmallMalloc((size_t)m * sizeof(int));
  nbd2 = (double *)msSmallMalloc((size_t)m * sizeof(double));
  A = (double *)msSmallMalloc((size_t)(m + 1) * (m + 1) * sizeof(double));
  k0 = (double *)msSmallMalloc((size_t)(m + 1) * sizeof(double));
  w = (double *)msSmallMalloc((size_t)(m + 1) * sizeof(double));
  kn.idx = nbidx;
  kn.d2 = nbd2;
  kn.cap = m;

  /* Predict every output pixel from its m nearest samples. The pixels are
   * independent, so this parallelises with an OpenMP "parallel for" once each
   * thread is given its own kn/A/k0/w scratch. */
  for (py = 0; py < height; py++) {
    for (px = 0; px < width; px++) {
      double pmean, pvar;
      kr_grid_knn(&grid, sx, sy, (double)px, (double)py, m, &kn);
      if (kr_predict(sx, sy, sz, kn.idx, kn.count, (double)px, (double)py,
                     model, ktype, sigma2, range, nugget, A, k0, w, &pmean,
                     &pvar) != 0)
        continue; /* leave no-data (0) on a singular local system */
      iMean[py * width + px] = krClamp(pmean);
      iSD[py * width + px] = krClamp(sqrt(pvar));
    }
  }

  free(nbidx);
  free(nbd2);
  free(A);
  free(k0);
  free(w);
  kr_grid_free(&grid);
  free(sx);
  free(sy);
  free(sz);
}

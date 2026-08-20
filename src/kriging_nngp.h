/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Nearest-Neighbor Gaussian Process (NNGP) prediction core.
 *           Pure C, depends only on <math.h>/<stdlib.h>/<string.h>; no
 *           MapServer types. Compiled into a single translation unit
 *           (kriging.c) and exercised independently by the unit test, so the
 *           production code and the test run the identical math.
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

#ifndef KRIGING_NNGP_H
#define KRIGING_NNGP_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Covariance families. RANGE is the *practical* range: the lag at which the
 * correlation has decayed to ~0.05 (exp/gauss are scaled so this holds). */
enum {
  KR_EXPONENTIAL = 0, /* Matern nu=1/2 */
  KR_GAUSSIAN = 1,    /* squared-exponential */
  KR_SPHERICAL = 2
};

/* Kriging flavor for the local solve. */
enum { KR_ORDINARY = 0, KR_SIMPLE = 1 };

/* Stationary covariance C(h); sigma2 = partial sill, a = practical range.
 * The exp/gauss forms carry a factor of 3 (exp(-3) ~= 0.05) so correlation has
 * decayed to ~5% at h = a -- i.e. "range" means the same thing for every model,
 * including the spherical one which reaches its sill exactly at h = a. */
static double kr_cov(int model, double h, double sigma2, double a) {
  double r;
  if (a <= 0.0)
    a = 1.0;
  r = h / a; /* normalized lag h/a */
  switch (model) {
  case KR_GAUSSIAN: /* sigma2 * exp(-3 r^2) */
    return sigma2 * exp(-3.0 * r * r);
  case KR_SPHERICAL: /* sigma2 * (1 - 1.5 r + 0.5 r^3), 0 for h >= a */
    return h >= a ? 0.0 : sigma2 * (1.0 - 1.5 * r + 0.5 * r * r * r);
  case KR_EXPONENTIAL: /* sigma2 * exp(-3 r) */
  default:
    return sigma2 * exp(-3.0 * r);
  }
}

/* Dense LU with partial pivoting. Solves A x = b (row-major, n x n) in place;
 * the solution is written back into b. Returns 0 on success, -1 if singular.
 * The ordinary-kriging system is symmetric *indefinite* (a saddle point), so a
 * pivoted LU is used rather than Cholesky. n is tiny (<= neighbors + 1). */
static int kr_solve(double *A, double *b, int n) {
  int i, j, k;
  for (k = 0; k < n; k++) { /* eliminate column k */
    /* partial pivoting: choose the largest-magnitude entry in column k as the
     * pivot, which keeps the elimination numerically stable */
    int p = k;
    double maxv = fabs(A[k * n + k]);
    for (i = k + 1; i < n; i++) {
      double v = fabs(A[i * n + k]);
      if (v > maxv) {
        maxv = v;
        p = i;
      }
    }
    if (maxv < 1e-300)
      return -1;  /* singular */
    if (p != k) { /* bring the pivot row up to row k (in A and the rhs b) */
      for (j = 0; j < n; j++) {
        double t = A[p * n + j];
        A[p * n + j] = A[k * n + j];
        A[k * n + j] = t;
      }
      double tb = b[p];
      b[p] = b[k];
      b[k] = tb;
    }
    double piv = A[k * n + k];
    /* subtract the right multiple of row k from each row below so column k goes
     * to zero there, carrying the same combination through the rhs */
    for (i = k + 1; i < n; i++) {
      double f = A[i * n + k] / piv;
      A[i * n + k] = 0.0;
      for (j = k + 1; j < n; j++)
        A[i * n + j] -= f * A[k * n + j];
      b[i] -= f * b[k];
    }
  }
  /* back-substitution: A is now upper-triangular, so solve from the last row
   * upward; the solution overwrites b */
  for (i = n - 1; i >= 0; i--) {
    double s = b[i];
    for (j = i + 1; j < n; j++)
      s -= A[i * n + j] * b[j];
    b[i] = s / A[i * n + i];
  }
  return 0;
}

/* Bounded "m nearest" accumulator: holds the m smallest squared distances seen
 * so far, ascending. m is tiny, so a shifting insertion sort is cheaper than a
 * heap and keeps the current worst (d2[count-1]) at hand for the query cutoff.
 */
typedef struct {
  int *idx;
  double *d2;
  int count, cap;
} kr_knn;

static void kr_knn_insert(kr_knn *k, int id, double d2) {
  int i;
  if (k->count < k->cap) {
    i = k->count++;
  } else if (d2 < k->d2[k->cap - 1]) {
    i = k->cap - 1; /* full: this is nearer than the worst kept, so evict it */
  } else {
    return; /* full and no closer than the worst kept -> nothing to do */
  }
  /* insertion sort: slide the larger kept entries up one slot until d2's place
   * is open, then drop it in -- the array stays sorted ascending */
  for (; i > 0 && k->d2[i - 1] > d2; i--) {
    k->d2[i] = k->d2[i - 1];
    k->idx[i] = k->idx[i - 1];
  }
  k->d2[i] = d2;
  k->idx[i] = id;
}

/* Uniform-grid spatial index over the samples (in pixel coordinates), packed
 * CSR-style so a neighbour query only scans the cells around the target. */
typedef struct {
  int nx, ny;
  double minx, miny, cell, invcell;
  int *start; /* CSR offsets, length nx*ny + 1 */
  int *items; /* sample indices bucketed by cell, length n */
} kr_grid;

static int kr_grid_build(kr_grid *g, const double *sx, const double *sy,
                         int n) {
  int i, nc;
  double mnx, mxx, mny, mxy, w, h, cell;
  int *cur;
  memset(g, 0, sizeof(*g));
  if (n <= 0)
    return -1;

  /* bounding box of the sample cloud -> sets the grid's origin and extent */
  mnx = mxx = sx[0];
  mny = mxy = sy[0];
  for (i = 1; i < n; i++) {
    if (sx[i] < mnx)
      mnx = sx[i];
    if (sx[i] > mxx)
      mxx = sx[i];
    if (sy[i] < mny)
      mny = sy[i];
    if (sy[i] > mxy)
      mxy = sy[i];
  }
  w = mxx - mnx;
  h = mxy - mny;
  if (w <= 0)
    w = 1;
  if (h <= 0)
    h = 1;
  cell = sqrt((w * h) / (double)n); /* ~1 sample / cell */
  if (!(cell > 0))
    cell = 1;
  g->minx = mnx;
  g->miny = mny;
  g->cell = cell;
  g->invcell = 1.0 / cell;
  g->nx = (int)(w * g->invcell) + 1; /* grid width in cells  */
  g->ny = (int)(h * g->invcell) + 1; /* grid height in cells */
  nc = g->nx * g->ny;
  /* start[]: per-cell CSR offsets; items[]: sample ids grouped by cell;
   * cur[]: scratch write cursors used only while scattering below */
  g->start = (int *)calloc((size_t)nc + 1, sizeof(int));
  g->items = (int *)malloc((size_t)n * sizeof(int));
  cur = (int *)malloc((size_t)nc * sizeof(int));
  if (!g->start || !g->items || !cur) {
    free(g->start);
    free(g->items);
    free(cur);
    return -1;
  }
  /* counting sort into per-cell buckets (CSR): tally each cell's count, prefix-
   * sum the counts into start offsets, then scatter the sample ids into place.
   */
  for (i = 0; i < n; i++) {
    /* which cell does sample i fall in? clamp to the grid's edge cells */
    int cx = (int)((sx[i] - mnx) * g->invcell);
    int cy = (int)((sy[i] - mny) * g->invcell);
    if (cx < 0)
      cx = 0;
    else if (cx >= g->nx)
      cx = g->nx - 1;
    if (cy < 0)
      cy = 0;
    else if (cy >= g->ny)
      cy = g->ny - 1;
    g->start[cy * g->nx + cx +
             1]++; /* tally into the next cell's offset slot */
  }
  for (i = 0; i < nc; i++) /* counts -> CSR start offsets */
    g->start[i + 1] += g->start[i];
  memcpy(cur, g->start, (size_t)nc * sizeof(int)); /* per-cell write cursors */
  for (i = 0; i < n; i++) {
    int cx = (int)((sx[i] - mnx) * g->invcell);
    int cy = (int)((sy[i] - mny) * g->invcell);
    if (cx < 0)
      cx = 0;
    else if (cx >= g->nx)
      cx = g->nx - 1;
    if (cy < 0)
      cy = 0;
    else if (cy >= g->ny)
      cy = g->ny - 1;
    g->items[cur[cy * g->nx + cx]++] = i; /* place sample i, advance cursor */
  }
  free(cur);
  return 0;
}

static void kr_grid_free(kr_grid *g) {
  free(g->start);
  free(g->items);
  g->start = NULL;
  g->items = NULL;
}

/* Exact m-nearest-neighbor query: expand square rings of cells until the ring
 * distance proves no closer sample can remain unexamined. */
static void kr_grid_knn(const kr_grid *g, const double *sx, const double *sy,
                        double qx, double qy, int m, kr_knn *kn) {
  int r, maxr, hcx, hcy;
  kn->count = 0;
  /* home cell: the grid cell containing the query, clamped into range */
  hcx = (int)((qx - g->minx) * g->invcell);
  hcy = (int)((qy - g->miny) * g->invcell);
  if (hcx < 0)
    hcx = 0;
  else if (hcx >= g->nx)
    hcx = g->nx - 1;
  if (hcy < 0)
    hcy = 0;
  else if (hcy >= g->ny)
    hcy = g->ny - 1;
  maxr = g->nx + g->ny; /* an upper bound on rings: spans the whole grid */
  /* Grow a square "ring" of cells outward from the query's home cell. Ring r is
   * the border at Chebyshev (chessboard) distance r: the box [hcx-r, hcx+r] x
   * [hcy-r, hcy+r], with its interior skipped -- earlier rings already covered
   * it -- so each cell is scanned exactly once across all rings. */
  for (r = 0; r <= maxr; r++) {
    int cy, cx, x0 = hcx - r, x1 = hcx + r, y0 = hcy - r, y1 = hcy + r;
    for (cy = y0; cy <= y1; cy++) {
      if (cy < 0 || cy >= g->ny)
        continue; /* row off the grid */
      for (cx = x0; cx <= x1; cx++) {
        int ci, t;
        if (cx < 0 || cx >= g->nx)
          continue; /* column off the grid */
        if (r > 0 && cx > x0 && cx < x1 && cy > y0 && cy < y1)
          continue; /* interior cell already visited on an earlier ring */
        ci = cy * g->nx + cx;
        /* test every sample bucketed in this cell against the query */
        for (t = g->start[ci]; t < g->start[ci + 1]; t++) {
          int id = g->items[t];
          double dx = sx[id] - qx, dy = sy[id] - qy;
          kr_knn_insert(kn, id, dx * dx + dy * dy); /* ranked by squared dist */
        }
      }
    }
    /* rings 0..r are fully scanned, so any sample not yet seen sits in a cell
     * at ring >= r+1 -- at least r*cell from the query. once that lower bound
     * meets the current m-th distance no nearer sample can remain, so we stop
     * with the *exact* m nearest rather than an approximation. */
    if (kn->count >= m) {
      double bound = (double)r * g->cell;
      if (bound * bound >= kn->d2[kn->count - 1])
        break;
    }
  }
}

/* Local kriging / NNGP predictive distribution at (qx,qy) from the m samples in
 * nb[]. Writes the predictive mean and (latent) variance. Scratch buffers must
 * hold (m+1)*(m+1) doubles (A) and (m+1) doubles (k0, w). Returns 0, or -1 if
 * the local system is singular (caller should leave the pixel as no-data). */
static int kr_predict(const double *sx, const double *sy, const double *sz,
                      const int *nb, int m, double qx, double qy, int model,
                      int ktype, double sigma2, double range, double nugget,
                      double *A, double *k0, double *w, double *mean_out,
                      double *var_out) {
  int M =
      (ktype == KR_ORDINARY) ? m + 1 : m; /* ordinary adds one bordered row */
  int i, j;
  double mean = 0.0, quad = 0.0, var;
  if (m <= 0)
    return -1;
  /* A <- K, the covariance matrix among the m neighbours; k0 <- the covariance
   * of each neighbour with the target pixel. */
  for (i = 0; i < m; i++) {
    double xi = sx[nb[i]], yi = sy[nb[i]];
    for (j = 0; j < m; j++) {
      double dx = xi - sx[nb[j]], dy = yi - sy[nb[j]];
      A[i * M + j] = kr_cov(model, sqrt(dx * dx + dy * dy), sigma2, range);
    }
    A[i * M + i] +=
        nugget; /* nugget on the diagonal => smoothing / stability */
    {
      double dx = xi - qx, dy = yi - qy;
      k0[i] = kr_cov(model, sqrt(dx * dx + dy * dy), sigma2, range);
    }
  }
  if (ktype == KR_ORDINARY) {
    /* Unknown local mean: border K with the unbiasedness constraint (weights
     * must sum to 1). The extra unknown w[m] is the Lagrange multiplier, so the
     * system becomes [K 1; 1' 0] [w; mu] = [k0; 1]. */
    for (i = 0; i < m; i++) {
      A[i * M + m] = 1.0;
      A[m * M + i] = 1.0;
    }
    A[m * M + m] = 0.0;
    k0[m] = 1.0;
  }
  memcpy(w, k0,
         (size_t)M * sizeof(double)); /* solve in w; keep k0 for the variance */
  if (kr_solve(A, w, M) != 0)
    return -1;
  /* predictor z*(x0) = sum_i w_i z_i ; quad = sum_i w_i C(x0, x_i) */
  for (i = 0; i < m; i++) {
    mean += w[i] * sz[nb[i]];
    quad += w[i] * k0[i];
  }
  /* kriging variance = sigma2 - sum_i w_i C(x0, x_i), less the Lagrange term mu
   * (= w[m]) for ordinary kriging; clamp round-off below zero. */
  var = sigma2 - quad - (ktype == KR_ORDINARY ? w[m] : 0.0);
  if (var < 0.0)
    var = 0.0;
  *mean_out = mean;
  *var_out = var;
  return 0;
}

#endif /* KRIGING_NNGP_H */

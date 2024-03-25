/******************************************************************************
 * $Id$
 *
 * Project:  CFS OGC MapServer
 * Purpose:  Definitions related to raster resampling support.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam, DM Solutions Group Inc
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

#ifndef RESAMPLE_H
#define RESAMPLE_H

#include "mapserver.h"
#include "mapproject.h"

#include <gdal.h>
#include <cpl_string.h>

typedef int (*SimpleTransformer)(void *pCBData, int nPoints, double *x,
                                 double *y, int *panSuccess);

void *msInitProjTransformer(projectionObj *psSrc, double *padfSrcGeoTransform,
                            projectionObj *psDst, double *padfDstGeoTransform);
void msFreeProjTransformer(void *);
int msProjTransformer(void *pCBData, int nPoints, double *x, double *y,
                      int *panSuccess);

int msResampleGDALToMap(mapObj *map, layerObj *layer, imageObj *image,
                        rasterBufferObj *rb, GDALDatasetH hDS);

#endif /* ndef RESAMPLE_H */

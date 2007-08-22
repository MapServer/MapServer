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
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.4  2003/02/24 21:25:00  frank
 * include cpl_string.h
 *
 * Revision 1.3  2002/10/29 16:40:59  frank
 * removed prototype for function that is now static
 *
 * Revision 1.2  2002/06/21 18:33:45  frank
 * added INT16 and FLOAT image mode support
 *
 * Revision 1.1  2001/03/14 17:39:16  frank
 * New
 *
 */

#ifndef RESAMPLE_H
#define RESAMPLE_H

#include <gd.h>
#include "map.h"
#include "mapproject.h"

#ifdef USE_GDAL
#  include <gdal.h>
#  include <cpl_string.h>
#endif

typedef int (*SimpleTransformer)( void *pCBData, int nPoints, 
                                  double *x, double *y, int *panSuccess );

void *msInitProjTransformer( projectionObj *psSrc, 
                             double *padfSrcGeoTransform, 
                             projectionObj *psDst, 
                             double *padfDstGeoTransform );
void msFreeProjTransformer( void * );
int msProjTransformer( void *pCBData, int nPoints, 
                       double *x, double *y, int *panSuccess );
#ifdef USE_GDAL
int msResampleGDALToMap( mapObj *map, layerObj *layer, 
                         imageObj *image,
                         GDALDatasetH hDS );
#endif
#endif /* ndef RESAMPLE_H */

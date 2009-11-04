/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Declarations of the projectionObj and related functions.
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
 ****************************************************************************/

#ifndef MAPPROJECT_H
#define MAPPROJECT_H

#include "mapprimitive.h"
#include "maphash.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_PROJ
#  include <proj_api.h>
#endif


typedef struct {
#ifdef SWIG
  %immutable;
#endif
  int numargs; /* actual number of projection args */
#ifdef SWIG
  %mutable;
#endif
#ifndef SWIG
  char **args; /* variable number of projection args */
#ifdef USE_PROJ
  projPJ proj; /* a projection structure for the PROJ package */
#else
  void *proj;
#endif
  geotransformObj gt; /* extra transformation to apply */
#endif
} projectionObj;

#ifndef SWIG
MS_DLL_EXPORT int msProjectPoint(projectionObj *in, projectionObj *out, pointObj *point);
MS_DLL_EXPORT int msProjectShape(projectionObj *in, projectionObj *out, shapeObj *shape);
MS_DLL_EXPORT int msProjectLine(projectionObj *in, projectionObj *out, lineObj *line);
MS_DLL_EXPORT int msProjectRect(projectionObj *in, projectionObj *out, rectObj *rect);
MS_DLL_EXPORT int msProjectionsDiffer(projectionObj *, projectionObj *);
MS_DLL_EXPORT int msOGCWKT2ProjectionObj( const char *pszWKT, projectionObj *proj, int
                            debug_flag );

MS_DLL_EXPORT void msFreeProjection(projectionObj *p);
MS_DLL_EXPORT int msInitProjection(projectionObj *p);
MS_DLL_EXPORT int msProcessProjection(projectionObj *p);
MS_DLL_EXPORT int msLoadProjectionString(projectionObj *p, const char *value);
MS_DLL_EXPORT int msLoadProjectionStringEPSG(projectionObj *p, const char *value);
MS_DLL_EXPORT char *msGetProjectionString(projectionObj *proj);
MS_DLL_EXPORT void msAxisNormalizePoints( projectionObj *proj, int count,
                                          double *x, double *y );
MS_DLL_EXPORT void msAxisDenormalizePoints( projectionObj *proj, int count,
                                            double *x, double *y );

MS_DLL_EXPORT void msSetPROJ_LIB( const char * );

/* Provides compatiblity with PROJ.4 4.4.2 */
#ifndef PJ_VERSION
#  define pj_is_latlong(x)	((x)->is_latlong)
#endif

/*utility functions */
MS_DLL_EXPORT int GetMapserverUnitUsingProj(projectionObj *psProj);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPPROJECT_H */

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

#if PROJ_VERSION_MAJOR >= 6
#  include <proj.h>
#if PROJ_VERSION_MAJOR == 6 && PROJ_VERSION_MINOR == 0
#error "PROJ 6.0 is not supported. Use PROJ 6.1 or later"
#endif
#else
#  include <proj_api.h>
#if PJ_VERSION >= 470 && PJ_VERSION < 480
   void pj_clear_initcache();
#endif
#endif

#define wkp_none 0
#define wkp_lonlat 1
#define wkp_gmerc 2

typedef struct projectionContext projectionContext;

#ifndef SWIG
typedef enum
{
    LINE_CUTTING_UNKNOWN = -1,
    LINE_CUTTING_NONE = 0,
    LINE_CUTTING_FROM_POLAR = 1,
    LINE_CUTTING_FROM_LONGLAT_WRAP0 = 2
} msLineCuttingCase;
#endif

/**
The :ref:`PROJECTION <projection>` object
MapServer's Maps and Layers have Projection attributes, and these are C projectionObj structures, 
but are not directly exposed by the mapscript module
*/
  typedef struct {
#ifndef SWIG
      char **args; /* variable number of projection args */
#if PROJ_VERSION_MAJOR >= 6
      PJ* proj;
      projectionContext* proj_ctx;
#else
      projPJ proj; /* a projection structure for the PROJ package */
#if PJ_VERSION >= 480
      projCtx proj_ctx;
#endif
#endif
      geotransformObj gt; /* extra transformation to apply */
#endif

#ifdef SWIG
    %immutable;
#endif
    int numargs; ///< Actual number of projection args
    short automatic; ///< Projection object was to fetched from the layer
    unsigned short generation_number; ///< To be incremented when the content of the object changes, so that a reprojector can be invalidated
#ifdef SWIG
    %mutable;
#endif

    int wellknownprojection; ///< One of ``wkp_none 0``, ``wkp_lonlat 1``, or ``wkp_gmerc 2``
  } projectionObj;

/**
A holder object for projection coordinate transformations, introduced in RFC 126.
This allows caching of reprojections improving performance.
*/
  typedef struct {
#if PROJ_VERSION_MAJOR >= 6
#ifndef SWIG
    projectionObj* in;
    projectionObj* out;
    PJ* pj;
    msLineCuttingCase lineCuttingCase;
    shapeObj splitShape;
    int bFreePJ;
#endif
#else
#ifndef SWIG
    projectionObj* in;
    projectionObj* out;
    msLineCuttingCase lineCuttingCase;
    shapeObj splitShape;
    int no_op;
#endif
#endif
    unsigned short generation_number_in; ///< A counter that is incremented when the input projectionObj changes
    unsigned short generation_number_out; ///< A counter that is incremented when the output projectionObj changes
  } reprojectionObj;

#ifndef SWIG

  MS_DLL_EXPORT reprojectionObj* msProjectCreateReprojector(projectionObj* in, projectionObj* out);
  MS_DLL_EXPORT void msProjectDestroyReprojector(reprojectionObj* reprojector);
  MS_DLL_EXPORT int msProjectIsReprojectorStillValid(reprojectionObj* reprojector);

  MS_DLL_EXPORT projectionContext* msProjectionContextGetFromPool(void);
  MS_DLL_EXPORT void msProjectionContextReleaseToPool(projectionContext* ctx);
  MS_DLL_EXPORT void msProjectionContextPoolCleanup(void);

  MS_DLL_EXPORT int msIsAxisInverted(int epsg_code);
  MS_DLL_EXPORT int msProjectPoint(projectionObj *in, projectionObj *out, pointObj *point); /* legacy interface */
  MS_DLL_EXPORT int msProjectPointEx(reprojectionObj* reprojector, pointObj *point);
  MS_DLL_EXPORT int msProjectShape(projectionObj *in, projectionObj *out, shapeObj *shape); /* legacy interface */
  MS_DLL_EXPORT int msProjectShapeEx(reprojectionObj* reprojector, shapeObj *shape);
  MS_DLL_EXPORT int msProjectLine(projectionObj *in, projectionObj *out, lineObj *line); /* legacy interface */
  MS_DLL_EXPORT int msProjectLineEx(reprojectionObj* reprojector, lineObj *line);
  MS_DLL_EXPORT int msProjectRect(projectionObj *in, projectionObj *out, rectObj *rect); /* legacy interface */
  MS_DLL_EXPORT int msProjectRectAsPolygon(reprojectionObj* reprojector, rectObj *rect);
  MS_DLL_EXPORT int msProjectionsDiffer(projectionObj *, projectionObj *);
  MS_DLL_EXPORT int msOGCWKT2ProjectionObj( const char *pszWKT, projectionObj *proj, int debug_flag );
  MS_DLL_EXPORT char *msProjectionObj2OGCWKT( projectionObj *proj );

  MS_DLL_EXPORT void msFreeProjection(projectionObj *p);
  MS_DLL_EXPORT void msFreeProjectionExceptContext(projectionObj *p);
  MS_DLL_EXPORT int msInitProjection(projectionObj *p);
  MS_DLL_EXPORT void msProjectionInheritContextFrom(projectionObj *pDst, const projectionObj* pSrc);
  MS_DLL_EXPORT void msProjectionSetContext(projectionObj *p, projectionContext* ctx);
  MS_DLL_EXPORT int msProcessProjection(projectionObj *p);
  MS_DLL_EXPORT int msLoadProjectionString(projectionObj *p, const char *value);
  MS_DLL_EXPORT int msLoadProjectionStringEPSG(projectionObj *p, const char *value);
  MS_DLL_EXPORT char *msGetProjectionString(projectionObj *proj);
  int msIsAxisInvertedProj( projectionObj *proj );
  void msAxisSwapShape(shapeObj *shape);
  MS_DLL_EXPORT void msAxisNormalizePoints( projectionObj *proj, int count,
      double *x, double *y );
  MS_DLL_EXPORT void msAxisDenormalizePoints( projectionObj *proj, int count,
      double *x, double *y );

  MS_DLL_EXPORT void msSetPROJ_DATA( const char *, const char * );
  MS_DLL_EXPORT void msProjDataInitFromEnv();

  int msProjIsGeographicCRS(projectionObj* proj);
#if PROJ_VERSION_MAJOR >= 6
  int msProjectTransformPoints( reprojectionObj* reprojector,
                                int npoints, double* x, double* y );
#endif

  /*utility functions */
  MS_DLL_EXPORT int GetMapserverUnitUsingProj(projectionObj *psProj);

  int msProjectHasLonWrap(projectionObj *in, double* pdfLonWrap);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPPROJECT_H */

#ifndef MAPPROJECT_H
#define MAPPROJECT_H

#include "mapprimitive.h"
#include "maphash.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_PROJ
#  ifdef USE_PROJ_API_H
#    include <proj_api.h>
#  else
#    include <projects.h>
     typedef PJ *projPJ;
#  endif
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
MS_DLL_EXPORT int msLoadProjectionString(projectionObj *p, char *value);

MS_DLL_EXPORT const char *msGetEPSGProj(projectionObj *proj, hashTableObj metadata, int bReturnOnlyFirstOne);

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

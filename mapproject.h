#ifndef MAPPROJECT_H
#define MAPPROJECT_H

#include "mapprimitive.h"

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
  char **args; /* variable number of projection args */
  int numargs; /* actual number of projection args */ 
#ifdef USE_PROJ
  projPJ proj; /* a projection structure for the PROJ package */
#endif
} projectionObj;

#ifndef SWIG
int msProjectPoint(projectionObj *in, projectionObj *out, pointObj *point);
int msProjectShape(projectionObj *in, projectionObj *out, shapeObj *shape);
int msProjectLine(projectionObj *in, projectionObj *out, lineObj *line);
int msProjectRect(projectionObj *in, projectionObj *out, rectObj *rect);

int msOGCWKT2ProjectionObj( const char *pszWKT, 
                            projectionObj *proj );
void msFreeProjection(projectionObj *p);
int msInitProjection(projectionObj *p);
int msProcessProjection(projectionObj *p);
int msLoadProjectionString(projectionObj *p, char *value);

/* Provides compatiblity with PROJ.4 4.4.2 */
#ifndef PJ_VERSION
#  define pj_is_latlong(x)	((x)->is_latlong)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPPROJECT_H */

#ifndef MAPPROJECT_H
#define MAPPROJECT_H

#include "mapprimitive.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_PROJ
#include <projects.h>
#endif

typedef struct {
  char **projargs; /* variable number of projection args */
  int numargs; /* actual number of projection args */ 
#ifdef USE_PROJ
  PJ *proj; /* a projection structure for the PROJ package */
#endif
} projectionObj;

#ifdef USE_PROJ
void msProjectPoint(PJ *in, PJ *out, pointObj *point);
void msProjectPolyline(PJ *in, PJ *out, shapeObj *poly);
void msProjectLine(PJ *in, PJ *out, lineObj *line);
void msProjectRect(PJ *in, PJ *out, rectObj *rect);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPPROJECT_H */

#ifndef MAPPRIMITIVE_H
#define MAPPRIMITIVE_H

/* feature primitives */
typedef struct {
  double minx, miny, maxx, maxy;
} rectObj;

typedef struct {
  double x;
  double y;
} pointObj;

typedef struct {
  int numpoints;
  pointObj *point;
} lineObj;

typedef struct {
  int numlines;
  lineObj *line;
  rectObj bounds;
  int type;
} shapeObj;

typedef lineObj multipointObj;

#endif /* MAPPRIMITIVE_H */

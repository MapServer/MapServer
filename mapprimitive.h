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

  long index;
  int tileindex;

  int classindex;

  char *text;
  char **attributes;
} shapeObj;

typedef lineObj multipointObj;

#endif /* MAPPRIMITIVE_H */

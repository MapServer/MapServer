#ifndef MAPPRIMITIVE_H
#define MAPPRIMITIVE_H

/* feature primitives */
typedef struct {
  double minx, miny, maxx, maxy;
} rectObj;

typedef struct {
  double x;
  double y;
} vectorObj;

typedef struct {
  double x;
  double y;
  double z;  
  double m;  
} pointObj;

typedef struct {
#ifdef SWIG
%immutable;
#endif
  int numpoints;
  pointObj *point;
#ifdef SWIG
%mutable;
#endif
} lineObj;

typedef struct {
#ifdef SWIG
%immutable;
#endif
  int numlines;
  int numvalues;
  lineObj *line;
  char **values;
#ifdef SWIG
%mutable;
#endif

  rectObj bounds;
  int type; // MS_SHAPE_TYPE
  long index;
  int tileindex;
  int classindex;
  char *text;

#ifdef USE_GEOS
  void *geometry;
#endif

} shapeObj;

typedef lineObj multipointObj;

/* attribute primatives */
typedef struct {
  char *name;
  long type;
  int index;
  int size;
  short numdecimals;
} itemObj;

typedef struct {
  int  need_geotransform;
  double rotation_angle;  
  double geotransform[6];    // Pixel/line to georef.
  double invgeotransform[6]; // georef to pixel/line  
} geotransformObj;

#endif /* MAPPRIMITIVE_H */

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Declarations for rectObj, pointObj, lineObj, shapeObj, etc.
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

#ifndef MAPPRIMITIVE_H
#define MAPPRIMITIVE_H

/* feature primitives */
typedef struct {
  double minx, miny, maxx, maxy;
} rectObj;

#ifndef SWIG
typedef struct {
  double x;
  double y;
} vectorObj;
#endif /*SWIG*/

typedef struct {
  double x;
  double y;
#ifdef USE_POINT_Z_M
  double z;
  double m;
#endif
} pointObj;

typedef struct {
#ifdef SWIG
  %immutable;
#endif
  int numpoints;
#ifndef SWIG
  pointObj *point;
#endif
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
#ifndef SWIG
  lineObj *line;
  char **values;
  void *geometry;
  void *renderer_cache;
#endif

#ifdef SWIG
  %mutable;
#endif

  rectObj bounds;
  int type; /* MS_SHAPE_TYPE */
  long index;
  int tileindex;
  int classindex;
  char *text;

  int scratch;
  int resultindex; /* index within a query result set */
} shapeObj;

typedef lineObj multipointObj;

#ifndef SWIG
/* attribute primatives */
typedef struct {
  char *name;
  long type;
  int index;
  int size;
  short numdecimals;
} itemObj;
#endif

#ifndef SWIG
typedef struct {
  int  need_geotransform;
  double rotation_angle;
  double geotransform[6];    /* Pixel/line to georef. */
  double invgeotransform[6]; /* georef to pixel/line */
} geotransformObj;
#endif

#endif /* MAPPRIMITIVE_H */

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

 /**
 A rectObj represents a rectangle or bounding box.
 A rectObj may be a lone object or an attribute of another object and has no other associations.
 */
typedef struct {
  double minx; ///< Minimum easting
  double miny; ///< Minimum northing
  double maxx; ///< Maximum easting
  double maxy; ///< Maximum northing
} rectObj;

#ifndef SWIG
typedef struct {
  double x;
  double y;
} vectorObj;
#endif /*SWIG*/

/**
A :class:`pointObj` has an x, y, z and m values.
A :class:`pointObj` instance may be associated with a :class:`lineObj`.
*/
typedef struct {
  double x; ///< The x coordinate of the point
  double y; ///< The y coordinate of the point
  double z; ///< The z (height) coordinate of the point
  double m; ///< The m (measure) of the point, used for linear referencing
} pointObj;

/**
A :class:`lineObj` is composed of one or more :class:`pointObj` instances
*/
typedef struct {
#ifdef SWIG
  %immutable;
#endif
  int numpoints; ///< Number of points in the line
#ifndef SWIG
  pointObj *point;
#endif
#ifdef SWIG
  %mutable;
#endif
} lineObj;

/**
Each feature of a layer's data is a :class:`shapeObj`. Each part of the shape is a closed :class:`lineObj`.
*/
typedef struct {

#ifndef SWIG
    lineObj *line;
    char **values;
    void *geometry;
    void *renderer_cache;
#endif

#ifdef SWIG
  %immutable;
#endif
  int numlines; ///< Number of parts
  int numvalues; ///< Number of shape attributes

#ifdef SWIG
  %mutable;
#endif

  rectObj bounds; ///< Bounding box of shape
  int type; ///< MS_SHAPE_POINT, MS_SHAPE_LINE, MS_SHAPE_POLYGON, or MS_SHAPE_NULL
  long index; ///< Feature index within the layer
  int tileindex; ///< Index of tiled file for tile-indexed layers
  int classindex; ///< The class index for features of a classified layer
  char *text; ///< Shape annotation

  int scratch;
  int resultindex; ///< Index within a query result set
} shapeObj;

typedef lineObj multipointObj;

#ifndef SWIG
/* attribute primitives */
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

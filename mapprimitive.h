/******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.21  2006/09/01 18:03:19  umberto
 * Removed USE_GEOS defines, bug #1890
 *
 * Revision 1.20  2006/06/27 06:53:08  sdlime
 * Excluded shapeObj geometry member from Swig wrapping.
 *
 * Revision 1.19  2006/02/22 05:04:34  sdlime
 * Applied patch for bug 1660 to hide certain structures from Swig-based MapScript.
 *
 * Revision 1.18  2005/06/14 16:03:34  dan
 * Updated copyright date to 2005
 *
 * Revision 1.17  2005/04/21 15:09:28  julien
 * Bug1244: Replace USE_SHAPE_Z_M by USE_POINT_Z_M
 *
 * Revision 1.16  2005/04/14 15:17:14  julien
 * Bug 1244: Remove Z and M from point by default to gain performance.
 *
 * Revision 1.15  2005/02/18 03:06:46  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.14  2004/11/29 15:09:34  sean
 * hide itemObj and geotransformObj from swig, these are not ready for use
 *
 * Revision 1.13  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

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

#ifndef SWIG
  void *geometry;
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

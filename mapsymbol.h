/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  symbolObj related declarations.
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

#ifndef MAPSYMBOL_H
#define MAPSYMBOL_H

#include <gd.h>

enum MS_SYMBOL_TYPE {MS_SYMBOL_SIMPLE=1000, MS_SYMBOL_VECTOR, MS_SYMBOL_ELLIPSE, MS_SYMBOL_PIXMAP, MS_SYMBOL_TRUETYPE, MS_SYMBOL_CARTOLINE, MS_SYMBOL_HATCH};

#define MS_SYMBOL_ALLOCSIZE 64      /* number of symbolObj ptrs to allocate for a symbolset at once */
#define MS_MAXVECTORPOINTS 100      /* shade, marker and line symbol parameters */
#define MS_MAXPATTERNLENGTH 10

#define MS_IMAGECACHESIZE 6

/* COLOR OBJECT */
typedef struct {
  int pen;
  int red;
  int green;
  int blue;
  int alpha;
} colorObj;

#ifndef SWIG
struct imageCacheObj {
  int symbol;
  int size;
  colorObj color;
  colorObj outlinecolor;
  colorObj backgroundcolor;
  gdImagePtr img;
  struct imageCacheObj *next;
};

typedef struct {
    unsigned char *pixelbuffer;
    unsigned int width,height;
    unsigned int pixel_step, row_step;
    unsigned char *a,*r,*g,*b;
} rasterBufferObj;

#endif /* SWIG */


typedef struct {
  char *name;
  int type;
  int inmapfile; /* boolean value for writing */

#ifndef SWIG
  /*
  ** Pointer to his map
  */
  struct map_obj *map;
#endif /* SWIG */
  /*
  ** MS_SYMBOL_VECTOR and MS_SYMBOL_ELLIPSE options
  */
  double sizex, sizey;
  double minx,miny,maxx,maxy;

#ifndef SWIG
  pointObj points[MS_MAXVECTORPOINTS];
#endif

#ifdef SWIG
  %immutable;
#endif /* SWIG */
  int refcount;
  int numpoints;
#ifdef SWIG
  %mutable;
#endif /* SWIG */
  int filled;
  
  
  /*deprecated, moved to styleObj*/
  int patternlength;                      /* Number of intervals (eg. dashes) in the pattern (was style, see bug 2119) */
  int pattern[MS_MAXPATTERNLENGTH];

  /*
  ** MS_SYMBOL_PIXMAP options
  */
#ifndef SWIG
  gdImagePtr img;
  void *renderer_cache; /* Renderer storage */
  rendererVTableObj *renderer;
  rasterBufferObj *pixmap_buffer;
  char *full_font_path;
#endif /* SWIG */

#ifdef SWIG
  %immutable;
#endif /* SWIG */
  char *imagepath;
#ifdef SWIG
  %mutable;
#endif /* SWIG */

  int transparent;
  int transparentcolor;

  /*
  ** MS_SYMBOL_TRUETYPE options
  */
  char *character;
  int antialias;
  char *font;
  int gap; /*deprecated, moved to styleObj*/
  int position; /*deprecated, moved to styleObj*/

  /*
  ** MS_SYMBOL_CARTOLINE options
  */
  int linecap, linejoin; /*deprecated, moved to styleObj*/
  double linejoinmaxsize;/*deprecated, moved to styleObj*/

} symbolObj;

#endif /* MAPSYMBOL_H */

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

#ifdef USE_GD
#include <gd.h>
#endif

#include "mapserver-api.h"
#include <assert.h>

enum MS_SYMBOL_TYPE {MS_SYMBOL_SIMPLE=1000, MS_SYMBOL_VECTOR, MS_SYMBOL_ELLIPSE, MS_SYMBOL_PIXMAP, MS_SYMBOL_TRUETYPE, MS_SYMBOL_HATCH, MS_SYMBOL_SVG};

#define MS_SYMBOL_ALLOCSIZE 64      /* number of symbolObj ptrs to allocate for a symbolset at once */
#define MS_MAXVECTORPOINTS 100      /* shade, marker and line symbol parameters */
#define MS_MAXPATTERNLENGTH 10

#define MS_IMAGECACHESIZE 6

/* COLOR OBJECT */
typedef struct {
#ifdef USE_GD
  int pen;
#endif
  int red;
  int green;
  int blue;
  int alpha;
} colorObj;

#ifndef SWIG
enum MS_RASTER_BUFFER_TYPE { MS_BUFFER_NONE=2000, MS_BUFFER_BYTE_RGBA, MS_BUFFER_BYTE_PALETTE, MS_BUFFER_GD };

typedef struct {
  unsigned char *pixels;
  unsigned int pixel_step, row_step;
  unsigned char *a,*r,*g,*b;
} rgbaArrayObj;

typedef struct {
  unsigned char b,g,r,a;
} rgbaPixel;

typedef struct {
  unsigned char r,g,b;
} rgbPixel;


typedef struct {
  unsigned char *pixels; /*stores the actual pixel indexes*/
  rgbaPixel *palette; /*rgba palette entries*/
  unsigned int num_entries; /*number of palette entries*/
  unsigned int scaling_maxval;
} paletteArrayObj;

typedef struct {
  int type;
  unsigned int width,height;
  union {
    rgbaArrayObj rgba;
    paletteArrayObj palette;
#ifdef USE_GD
    gdImagePtr gd_img;
#endif
  } data;
} rasterBufferObj;

/* NOTE: RB_SET_PIXEL() will premultiply by alpha, inputs should not be
         premultiplied */

#define RB_SET_PIXEL(rb,x,y,red,green,blue,alpha) \
    {  \
        int _rb_off = (x) * (rb)->data.rgba.pixel_step + (y) * (rb)->data.rgba.row_step;   \
        if( !(rb)->data.rgba.a ) { \
           (rb)->data.rgba.r[_rb_off] = red; \
           (rb)->data.rgba.g[_rb_off] = green; \
           (rb)->data.rgba.b[_rb_off] = blue; \
        } else { \
           double a = alpha/255.0; \
           (rb)->data.rgba.r[_rb_off] = red * a; \
           (rb)->data.rgba.g[_rb_off] = green * a; \
           (rb)->data.rgba.b[_rb_off] = blue * a; \
           (rb)->data.rgba.a[_rb_off] = alpha; \
        } \
    }

/* This versions receives an input red/green/blue that is already
   premultiplied with alpha */
#define RB_SET_PIXEL_PM(rb,x,y,red,green,blue,alpha) \
    {  \
        int _rb_off = (x) * (rb)->data.rgba.pixel_step + (y) * (rb)->data.rgba.row_step;   \
        (rb)->data.rgba.r[_rb_off] = red; \
        (rb)->data.rgba.g[_rb_off] = green; \
        (rb)->data.rgba.b[_rb_off] = blue; \
        if( rb->data.rgba.a ) { \
           (rb)->data.rgba.a[_rb_off] = alpha; \
        } \
    }

/* NOTE: RB_MIX_PIXEL() will premultiply by alpha, inputs should not be
         premultiplied */

#define RB_MIX_PIXEL(rb,x,y,red,green,blue,alpha) \
    {  \
        int _rb_off = (x) * (rb)->data.rgba.pixel_step + (y) * (rb)->data.rgba.row_step;   \
        \
        msAlphaBlend(  red, green, blue, alpha, \
                       (rb)->data.rgba.r + _rb_off, \
                       (rb)->data.rgba.g + _rb_off, \
                       (rb)->data.rgba.b + _rb_off, \
                       ((rb)->data.rgba.a == NULL ) ? NULL : (rb)->data.rgba.a + _rb_off );  \
    }



struct imageCacheObj {
  int symbol;
  int size;
  colorObj color;
  colorObj outlinecolor;
  colorObj backgroundcolor;
  rasterBufferObj img;
  struct imageCacheObj *next;
};


#endif /* SWIG */


struct symbolObj{
  char *name;
  int type;
  int inmapfile; /* boolean value for writing */

#ifndef SWIG
  /*
  ** Pointer to his map
  */
  struct mapObj *map;
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

  double anchorpoint_x, anchorpoint_y;

  /*
  ** MS_SYMBOL_PIXMAP options
  */
#ifndef SWIG
  rendererVTableObj *renderer;
  void (*renderer_free_func)(symbolObj *self);
  rasterBufferObj *pixmap_buffer;
  void *renderer_cache;
  char *full_font_path;
  char *full_pixmap_path;
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
} ;

#endif /* MAPSYMBOL_H */

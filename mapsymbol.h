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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.23  2005/12/15 05:42:05  sdlime
 * Fixed a problem in the symbol reader that basically turned TYPE SIMPLE into TYPE TRUETYPE. Fix what to start the symbol type enum at some larger value (e.g. 1000).
 *
 * Revision 1.22  2005/11/24 04:35:50  dan
 * Use dynamic allocation for ellipse symbol's STYLE array, avoiding the
 * static limitation on the STYLE argument values. (bug 1539)
 *
 * Revision 1.21  2005/10/14 05:05:19  sdlime
 * Added MS_MAXPATTERNSIZE.
 *
 * Revision 1.20  2005/09/23 03:47:56  sdlime
 * Made symbol->imagepath readonly within SWIG-based scripts. Use setImagepath instead. (bug 1472)
 *
 * Revision 1.19  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.18  2005/02/18 03:06:47  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.17  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#ifndef MAPSYMBOL_H
#define MAPSYMBOL_H

#include <gd.h>

enum MS_SYMBOL_TYPE {MS_SYMBOL_SIMPLE=1000, MS_SYMBOL_VECTOR, MS_SYMBOL_ELLIPSE, MS_SYMBOL_PIXMAP, MS_SYMBOL_TRUETYPE, MS_SYMBOL_CARTOLINE, MS_SYMBOL_HATCH};

#define MS_MAXSYMBOLS 64            /* maximum number of symbols in a symbol file */
#define MS_MAXVECTORPOINTS 100      /* shade, marker and line symbol parameters */
#define MS_MAXSTYLELENGTH 10

#define MS_IMAGECACHESIZE 6

/* COLOR OBJECT */
typedef struct {
  int pen;
  int red;
  int green;
  int blue;
#if ALPHACOLOR_ENABLED
  int alpha;
#endif
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
#endif /* SWIG */


typedef struct {
  char *name;
  int type;
#ifndef SWIG
  int inmapfile; /* boolean value for writing */

  /*
  ** Pointer to his map
  */
  struct map_obj *map;
#endif /* SWIG */
  /*
  ** MS_SYMBOL_VECTOR and MS_SYMBOL_ELLIPSE options
  */
  double sizex, sizey;

#ifndef SWIG
  pointObj points[MS_MAXVECTORPOINTS];
#endif

#ifdef SWIG
  %immutable;
#endif /* SWIG */
  int numpoints;
#ifdef SWIG
  %mutable;
#endif /* SWIG */
  int filled;

  int stylelength;                      /* Number of intervals (eg. dashes) in the style */
  int style[MS_MAXSTYLELENGTH];

  /*
  ** MS_SYMBOL_PIXMAP options
  */
#ifndef SWIG
  gdImagePtr img;
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
  int gap;
  int position;

  /*
  ** MS_SYMBOL_CARTOLINE options
  */
  int linecap, linejoin;
  double linejoinmaxsize;

} symbolObj;

#endif /* MAPSYMBOL_H */

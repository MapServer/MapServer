#ifndef MAPSYMBOL_H
#define MAPSYMBOL_H

#include <gd.h>

enum MS_SYMBOL_TYPE {MS_SYMBOL_SIMPLE, MS_SYMBOL_VECTOR, MS_SYMBOL_ELLIPSE, MS_SYMBOL_PIXMAP, MS_SYMBOL_TRUETYPE, MS_SYMBOL_CARTOLINE};

#define MS_MAXSYMBOLS 64            // maximum number of symbols in a symbol file
#define MS_MAXVECTORPOINTS 100      // shade, marker and line symbol parameters
#define MS_MAXSTYLELENGTH 10

#define MS_IMAGECACHESIZE 6

// COLOR OBJECT
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
#endif // SWIG


typedef struct {
  char *name;
  int type;
#ifndef SWIG
  int inmapfile; //boolean value for writing

  /*
  ** Pointer to his map
  */
  struct map_obj *map;
#endif // SWIG
  /*
  ** MS_SYMBOL_VECTOR and MS_SYMBOL_ELLIPSE options
  */
  double sizex, sizey;

#ifndef SWIG
  pointObj points[MS_MAXVECTORPOINTS];
#endif

#ifdef SWIG
  %immutable;
#endif // SWIG
  int numpoints;
#ifdef SWIG
  %mutable;
#endif // SWIG
  int filled;

  int stylelength;                      // Number of intervals (eg. dashes) in the style
  int style[MS_MAXSTYLELENGTH];

  /*
  ** MS_SYMBOL_PIXMAP options
  */
#ifndef SWIG
  gdImagePtr img;
#endif // SWIG
  char *imagepath;
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

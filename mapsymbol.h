#ifndef MAPSYMBOL_H
#define MAPSYMBOL_H

#include <gd.h>

enum MS_SYMBOL_TYPE {MS_SYMBOL_SIMPLE, MS_SYMBOL_VECTOR, MS_SYMBOL_ELLIPSE, MS_SYMBOL_PIXMAP, MS_SYMBOL_TRUETYPE, MS_SYMBOL_CARTOLINE};

#define MS_MAXSYMBOLS 64            // maximum number of symbols in a symbol file
#define MS_MAXVECTORPOINTS 100      // shade, marker and line symbol parameters
#define MS_MAXSTYLELENGTH 10

#define MS_IMAGECACHESIZE 6

struct imageCacheObj {
  int color;
  int symbol;
  int size;
  gdImagePtr img;
  struct imageCacheObj *next;
};

typedef struct {
  char *name;
  int type;
  int inmapfile; //boolean value for writing

  /*
  ** Pointer to his map
  */
  struct map_obj *map;
 
  /*
  ** MS_SYMBOL_VECTOR and MS_SYMBOL_ELLIPSE options
  */
  double sizex, sizey;
  pointObj points[MS_MAXVECTORPOINTS];
  int numpoints;
  int filled;

  int stylelength;                      // Number of intervals (eg. dashes) in the style
  int style[MS_MAXSTYLELENGTH];

  /*
  ** MS_SYMBOL_PIXMAP options
  */
  gdImagePtr img;
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

#ifndef MAPSYMBOL_H
#define MAPSYMBOL_H

#include <gd.h>

enum MS_SYMBOL_TYPE {MS_SYMBOL_VECTOR, MS_SYMBOL_ELLIPSE, MS_SYMBOL_PIXMAP, MS_SYMBOL_STYLED, MS_SYMBOL_TRUETYPE};

#define MS_MAXSYMBOLS 32            // maximum number of symbols in a symbol file
#define MS_MAXVECTORPOINTS 100      // shade, marker and line symbol parameters
#define MS_MAXSTYLESIZE 11

typedef struct {
  char *name;
  int type;                         // Type of the marker symbol (i.e. symboltypes)

  /*
  ** MS_SYMBOL_STYLED options
  */
  int numarcs;                      // Number of arcs in the line symbol
  int numon[MS_MAXSTYLESIZE];       // number of pixels on - a bit finer control
  int numoff[MS_MAXSTYLESIZE];      // number of pixels off
  int offset[MS_MAXSTYLESIZE];      // Offset from a non-offseted line

  /*
  ** MS_SYMBOL_VECTOR and MS_SYMBOL_ELLIPSE options
  */
  double sizex, sizey;
  pointObj points[MS_MAXVECTORPOINTS];
  int numpoints;
  int filled;

  /*
  ** MS_SYMBOL_PIXMAP options
  */
  gdImagePtr img;
  int transparent;
  int transparentcolor;

  /*
  ** MS_SYMBOL_TRUETYPE options
  */
  char *character;
  int antialias;
  char *font;

} symbolObj;

#endif /* MAPSYMBOL_H */

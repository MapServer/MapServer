#ifndef MAPSYMBOL_H
#define MAPSYMBOL_H

#include <gd.h>

enum ms_symbolfile {MS_MARKERSET, MS_LINESET, MS_SHADESET}; // all we need for scripting

#ifndef SWIG
enum ms_symbol {MS_VECTOR, MS_ELLIPSE, MS_PIXMAP, MS_STYLED};

#define MS_MAXSYMBOLS 32            // maximum number of symbols in a symbol file
#define MS_MAXVECTORPOINTS 100      // shade, marker and line symbol parameters
#define MS_MAXSTYLESIZE 11

typedef struct {
  char *name;
  int type;                         // Type of the marker symbol (i.e. symboltypes)

  /*
  ** MS_STYLED options
  */
  int numarcs;                      // Number of arcs in the line symbol
  int numon[MS_MAXSTYLESIZE];    // number of pixels on - a bit finer control
  int numoff[MS_MAXSTYLESIZE];   // number of pixels off
  int offset[MS_MAXSTYLESIZE];      // Offset from a non-offseted line

  /*
  ** MS_VECTOR and MS_ELLIPSE options
  */
  double sizex, sizey;
  pointObj points[MS_MAXVECTORPOINTS];
  int numpoints;
  int filled;

  /*
  ** MS_PIXMAP options
  */
  gdImagePtr img;           
  int transparent;
  int transparentcolor;
} symbolObj;

typedef struct {
  char *filename;
  int type;
  int numsymbols;
  symbolObj symbol[MS_MAXSYMBOLS];
} symbolSetObj;
#endif

#endif /* MAPSYMBOL_H */

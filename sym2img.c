/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Commandline utility to render symbols to a raster.
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

#include <gd.h>
#include <gdfontt.h>

#include "mapserver.h"

#define MAXCOLORS 256
#define CELLSIZE 100
#define NCOLS 5
#define RATIO .5
#define LBUF 5 /* space to reserve around line symbols */

int main(int argc, char *argv[]) {
  FILE *stream;
  int ns, n, k;
  gdImagePtr img;
  shapeObj p;
  int i, j;
  int ncols, nrows;
  char buffer[256];
  int gray, green, red, black, white;
  classObj class;
  symbolSetObj symbolSet;

  /* ---- check the number of arguments, return syntax if not correct ---- */
  if (argc < 2) {
    fprintf(stdout, "Syntax: sym2img [symbolset] [outfile]\n");
    exit(0);
  }

  /* Initialize the polygon/polyline */
  p.line = (lineObj *)malloc(sizeof(lineObj));
  p.numlines = 1;
  p.line[0].point = (pointObj *)malloc(sizeof(pointObj) * 4);
  p.line[0].numpoints = 4;

  /* Initialize the symbol and font sets */
  symbolSet.filename = msStrdup(argv[1]);

  /*
  ** load the symbol file
  */
  if (msLoadSymbolSet(&symbolSet) == -1) {
    msWriteError(stderr);
    exit(1);
  }

  ns = symbolSet.numsymbols;

  if (ns < NCOLS) {
    ncols = ns;
    nrows = 1;
  } else {
    ncols = NCOLS;
    nrows = (int)ceil((double)ns / NCOLS);
  }

  img = gdImageCreate(ncols * CELLSIZE, nrows * CELLSIZE);

  gray = gdImageColorAllocate(img, 222, 222, 222);
  white = gdImageColorAllocate(img, 255, 255, 255);
  green = gdImageColorAllocate(img, 40, 170, 40);
  black = gdImageColorAllocate(img, 0, 0, 0);
  red = gdImageColorAllocate(img, 255, 0, 0);

  class.color = red;
  class.backgroundcolor = white;
  class.outlinecolor = black;

  n = 0;

  for (i = 0; n < ns; i += CELLSIZE) {
    k = 0;
    for (j = 0; n < ns; j += CELLSIZE) {
      if (k == ncols)
        break;
      k++;
      gdImageFilledRectangle(img, j, i, j + CELLSIZE, i + CELLSIZE, gray);

      class.symbol = n;

      switch (symbolSet.type) {
      case (MS_MARKERSET):
        class.sizescaled = RATIO * CELLSIZE;
        p.line[0].point[0].x = MS_NINT(j + CELLSIZE / 2);
        p.line[0].point[0].y = MS_NINT(i + CELLSIZE / 2);
        p.line[0].numpoints = 1;
        msDrawMarkerSymbol(&(symbolSet), img, &(p.line[0].point[0]), &(class));
        break;

      case (MS_LINESET):
        class.sizescaled = 1;
        p.line[0].point[0].x = j;
        p.line[0].point[0].y = i + (CELLSIZE - LBUF) - 1;
        p.line[0].point[1].x = j + MS_NINT((CELLSIZE - LBUF) / 3.0) - 1;
        p.line[0].point[1].y = i;
        p.line[0].point[2].x = j + MS_NINT(2 * (CELLSIZE - LBUF) / 3.0) - 1;
        p.line[0].point[2].y = i + (CELLSIZE - LBUF) - 1;
        p.line[0].point[3].x = j + (CELLSIZE - LBUF) - 1;
        p.line[0].point[3].y = i;
        p.line[0].numpoints = 4;
        msDrawLineSymbol(map, img, &p, &(class));
        break;

      case (MS_SHADESET):
        class.sizescaled = 5;
        p.line[0].point[0].x = j;
        p.line[0].point[0].y = i;
        p.line[0].point[1].x = j + CELLSIZE - 1;
        p.line[0].point[1].y = i;
        p.line[0].point[2].x = j + CELLSIZE - 1;
        p.line[0].point[2].y = i + CELLSIZE - 1;
        p.line[0].point[3].x = j;
        p.line[0].point[3].y = i + CELLSIZE - 1;
        p.line[0].numpoints = 4;
        msDrawShadeSymbol(&(symbolSet), img, &p, &(class));
        break;

      default:
        break;
      }

      if (symbolSet.symbol[n]->name)
        snprintf(buffer, sizeof(buffer), "%d - %s", n,
                 symbolSet.symbol[n]->name);
      else
        snprintf(buffer, sizeof(buffer), "%d", n);
      gdImageString(img, gdFontTiny, j + 1, i + 1, buffer, black);

      n++;
    }
  }

  if ((stream = fopen(argv[2], "wb")) == NULL) { /* open the file */
    fprintf(stderr, "Unable to open output file: %s\n", argv[2]);
    exit(1);
  }
#ifndef USE_GD_GIF
  gdImageGif(img, stream);
#else
  gdImagePng(img, stream);
#endif
  gdImageDestroy(img);
  free(symbolSet.filename);
  fclose(stream);

  return (MS_TRUE);
}

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Mainline of commandline legend generation test utility.
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

#include "mapserver.h"

int main(int argc, char *argv[]) {
  mapObj *map = NULL;
  imageObj *img = NULL;

  msSetup();
  if (argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  if (argc < 3) {
    fprintf(stdout, "Syntax: legend [mapfile] [output image]\n");
    exit(0);
  }

  map = msLoadMap(argv[1], NULL, NULL);
  if (!map) {
    msWriteError(stderr);
    exit(0);
  }

  img = msDrawLegend(map, MS_FALSE, NULL);
  if (!img) {
    msWriteError(stderr);
    exit(0);
  }

  msSaveImage(NULL, img, argv[2]);

  msFreeImage(img);
  msFreeMap(map);

  return (MS_TRUE);
}

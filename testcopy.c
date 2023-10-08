/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Test of msCopyMap()
 * Author:   Sean Gilles
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

void printAtts(mapObj *, const char *);

int main(int argc, char *argv[]) {

  mapObj *original_map, *clone_map;

  /* ---------------------------------------------------------------------
   * Test 1: free original before freeing clone
   * --------------------------------------------------------------------- */

  /* Load map file */
  original_map = msLoadMap("tests/test.map", NULL);

  if (original_map == NULL) {
    /* Write errors */
    msWriteError(stderr);
    msResetErrorList();
    exit(0);
  }

  /* Dump out some attributes */
  printAtts(original_map, "Original");

  /* Clone it */
  clone_map = msNewMapObj();
  msCopyMap(clone_map, original_map);

  /* Write errors */
  msWriteError(stderr);
  msResetErrorList();

  /* Free */
  msFreeMap(original_map);

  /* Dump clone's attributes */
  printAtts(clone_map, "Clone");

  /* Free clone */
  msFreeMap(clone_map);

  exit(0);
}

void printAtts(mapObj *map, const char *title) {
  printf("\n%s Attributes\n----------------------\n", title);
  printf("Map Name: %s\n", map->name);
  printf("Numlayers: %d\n", map->numlayers);
  printf("Map Fontset Filename: %s\n", map->fontset.filename);
  printf("Map Symbolset Filename: %s\n", map->symbolset.filename);
}

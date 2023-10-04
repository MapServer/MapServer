/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Utility program to visualize a quadtree search
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
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdlib.h>
#include <limits.h>

#ifdef SHPT_POLYGON
#undef MAPSERVER
#else
#define MAPSERVER 1
#define SHPT_POLYGON MS_SHP_POLYGON
#endif

char *AddFileSuffix(const char *Filename, const char *Suffix) {
  char *pszFullname, *pszBasename;
  int i;

  /* -------------------------------------------------------------------- */
  /*  Compute the base (layer) name.  If there is any extension     */
  /*  on the passed in filename we will strip it off.         */
  /* -------------------------------------------------------------------- */
  pszBasename = (char *)msSmallMalloc(strlen(Filename) + 5);
  strcpy(pszBasename, Filename);
  for (i = (int)strlen(pszBasename) - 1;
       i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' &&
       pszBasename[i] != '\\';
       i--) {
  }

  if (pszBasename[i] == '.')
    pszBasename[i] = '\0';

  /* -------------------------------------------------------------------- */
  /*  Open the .shp and .shx files.  Note that files pulled from      */
  /*  a PC to Unix with upper case filenames won't work!        */
  /* -------------------------------------------------------------------- */
  pszFullname = (char *)msSmallMalloc(strlen(pszBasename) + 5);
  sprintf(pszFullname, "%s%s", pszBasename, Suffix);

  return (pszFullname);
}

int main(int argc, char **argv)

{
  SHPTreeHandle qix;

  int i, j;
  rectObj rect;

  int pos;
  ms_bitarray bitmap = NULL;

  /*
  char  mBigEndian;
  */
  treeNodeObj *node = NULL;

  /* -------------------------------------------------------------------- */
  /*      Display a usage message.                                        */
  /* -------------------------------------------------------------------- */
  if (argc <= 1) {
    printf("shptreetst shapefile {minx miny maxx maxy}\n");
    exit(1);
  }

  /*
  i = 1;
  if( *((unsigned char *) &i) == 1 )
    mBigEndian = 0;
  else
    mBigEndian = 1;
  */

  qix = msSHPDiskTreeOpen(AddFileSuffix(argv[1], ".qix"), 0 /* no debug*/);
  if (qix == NULL) {
    printf("unable to open index file %s \n", argv[1]);
    exit(-1);
  }

  printf("This %s %s index supports a shapefile with %d shapes, %d depth \n",
         (qix->version ? "new" : "old"), (qix->LSB_order ? "LSB" : "MSB"),
         (int)qix->nShapes, (int)qix->nDepth);

  /* -------------------------------------------------------------------- */
  /*  Skim over the list of shapes, printing all the vertices.  */
  /* -------------------------------------------------------------------- */

  pos = ftell(qix->fp);
  j = 0;

  while (pos && j < 20) {
    j++;
    /*      fprintf (stderr,"box %d, at %d pos \n", j, (int) ftell(qix));
     */

    node = readTreeNode(qix);
    if (node) {
      fprintf(stdout, "shapes %d, node %d, %f,%f,%f,%f \n",
              (int)node->numshapes, node->numsubnodes, node->rect.minx,
              node->rect.miny, node->rect.maxx, node->rect.maxy);

    } else {
      pos = 0;
    }
  }

  printf("read entire file now at quad box rec %d file pos %ld\n", j,
         ftell(qix->fp));

  j = qix->nShapes;
  msSHPDiskTreeClose(qix);

  if (argc >= 5) {
    rect.minx = atof(argv[2]);
    rect.miny = atof(argv[3]);
    rect.maxx = atof(argv[4]);
    rect.maxy = atof(argv[5]);
  } else {
    if (node == NULL) {
      printf("node == NULL");
      return 1;
    }
    printf("using last read box as a search \n");
    rect.minx = node->rect.minx;
    rect.miny = node->rect.miny;
    rect.maxx = node->rect.maxx;
    rect.maxy = node->rect.maxy;
  }

  bitmap = msSearchDiskTree(argv[1], rect, 0 /* no debug*/, j);

  /* j < INT_MAX - 1 test just to please Coverity untrusted bound loop warning
   */
  if (bitmap && j < INT_MAX - 1) {
    printf("result of rectangle search was \n");
    for (i = 0; i < j; i++) {
      if (msGetBit(bitmap, i)) {
        printf(" %d,", i);
      }
    }
  }
  printf("\n");

  return (0);
}

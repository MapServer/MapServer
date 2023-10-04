/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Commandline utility to generate .qix shapefile spatial indexes.
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

#include "../mapserver.h"
#include "../maptree.h"
#include <string.h>

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

  free(pszBasename);
  return (pszFullname);
}

int main(int argc, char *argv[]) {
  shapefileObj shapefile;

  treeObj *tree;
  int byte_order = MS_NEW_LSB_ORDER, i;
  int depth = 0;

  if (argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  /* -------------------------------------------------------------------- */
  /*  Establish the byte order on this machine to decide default        */
  /*    index format                                                      */
  /* -------------------------------------------------------------------- */
  i = 1;
  /* cppcheck-suppress knownConditionTrueFalse */
  if (*((uchar *)&i) == 1)
    byte_order = MS_NEW_LSB_ORDER;
  else
    byte_order = MS_NEW_MSB_ORDER;

  if (argc < 2) {
    fprintf(stdout, "Syntax:\n");
    fprintf(stdout, "    shptree <shpfile> [<depth>] [<index_format>]\n");
    fprintf(stdout, "Where:\n");
    fprintf(stdout, " <shpfile> is the name of the .shp file to index.\n");
    fprintf(stdout,
            " <depth>   (optional) is the maximum depth of the index\n");
    fprintf(stdout,
            "           to create, default is 0 meaning that shptree\n");
    fprintf(stdout, "           will calculate a reasonable default depth.\n");
    fprintf(stdout, " <index_format> (optional) is one of:\n");
    fprintf(stdout, "           NL: LSB byte order, using new index format\n");
    fprintf(stdout, "           NM: MSB byte order, using new index format\n");
    fprintf(stdout,
            "       The following old format options are deprecated:\n");
    fprintf(stdout, "           N:  Native byte order\n");
    fprintf(stdout, "           L:  LSB (intel) byte order\n");
    fprintf(stdout, "           M:  MSB byte order\n");
    fprintf(stdout, "       The default index_format on this system is: %s\n\n",
            (byte_order == MS_NEW_LSB_ORDER) ? "NL" : "NM");
    exit(0);
  }

  if (argc >= 3)
    depth = atoi(argv[2]);

  if (argc >= 4) {
    if (!strcasecmp(argv[3], "N"))
      byte_order = MS_NATIVE_ORDER;
    if (!strcasecmp(argv[3], "L"))
      byte_order = MS_LSB_ORDER;
    if (!strcasecmp(argv[3], "M"))
      byte_order = MS_MSB_ORDER;
    if (!strcasecmp(argv[3], "NL"))
      byte_order = MS_NEW_LSB_ORDER;
    if (!strcasecmp(argv[3], "NM"))
      byte_order = MS_NEW_MSB_ORDER;
  }

  if (msShapefileOpen(&shapefile, "rb", argv[1], MS_TRUE) == -1) {
    fprintf(stdout, "Error opening shapefile %s.\n", argv[1]);
    exit(0);
  }

  printf(
      "creating index of %s %s format\n",
      (byte_order < 1 ? "old (deprecated)" : "new"),
      ((byte_order == MS_NATIVE_ORDER)
           ? "native"
           : ((byte_order == MS_LSB_ORDER) || (byte_order == MS_NEW_LSB_ORDER)
                  ? " LSB"
                  : "MSB")));

  tree = msCreateTree(&shapefile, depth);
  if (!tree) {
#if MAX_SUBNODE == 2
    fprintf(stdout, "Error generating binary tree.\n");
#else
    fprintf(stdout, "Error generating quadtree.\n");
#endif
    exit(0);
  }

  msWriteTree(tree, AddFileSuffix(argv[1], MS_INDEX_EXTENSION), byte_order);
  msDestroyTree(tree);

  /*
  ** Clean things up
  */
  msShapefileClose(&shapefile);

  return (0);
}

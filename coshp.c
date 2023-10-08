/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Command line utility to sort a shapefile based on the
 *           spatial index file.
 *
 ******************************************************************************
 * Copyright (c) 2021 Paul Ramsey <pramsey@cleverelephant.ca>
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mapserver.h"

typedef struct {
  SHPHandle inSHP, outSHP;
  DBFHandle inDBF, outDBF;
  int currentOutRecord;
  int numDbfFields;
} writeState;

static char *AddFileSuffix(const char *Filename, const char *Suffix) {
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

static int writeShape(writeState *state, int shapeId) {
  int j = 0;
  int i = state->currentOutRecord;
  char fName[20];
  int fWidth, fnDecimals;
  shapeObj shape;

  /* Copy the DBF record over */
  for (j = 0; j < state->numDbfFields; j++) {

    DBFFieldType dbfField =
        msDBFGetFieldInfo(state->inDBF, j, fName, &fWidth, &fnDecimals);

    switch (dbfField) {
    case FTInteger:
      msDBFWriteIntegerAttribute(
          state->outDBF, i, j,
          msDBFReadIntegerAttribute(state->inDBF, shapeId, j));
      break;
    case FTDouble:
      msDBFWriteDoubleAttribute(
          state->outDBF, i, j,
          msDBFReadDoubleAttribute(state->inDBF, shapeId, j));
      break;
    case FTString:
      msDBFWriteStringAttribute(
          state->outDBF, i, j,
          msDBFReadStringAttribute(state->inDBF, shapeId, j));
      break;
    default:
      fprintf(stderr, "Unsupported data type for field: %s, exiting.\n", fName);
      exit(0);
    }
  }

  /* Copy the SHP record over */
  msSHPReadShape(state->inSHP, shapeId, &shape);
  msSHPWriteShape(state->outSHP, &shape);
  msFreeShape(&shape);

  /* Increment output row number */
  state->currentOutRecord++;

  return MS_SUCCESS;
}

static int writeNode(writeState *state, treeNodeObj *node) {
  int i;
  if (node->ids) {
    for (i = 0; i < node->numshapes; i++) {
      int curRec = state->currentOutRecord;
      writeShape(state, node->ids[i]);
      node->ids[i] = curRec;
    }
  }
  for (i = 0; i < node->numsubnodes; i++) {
    if (node->subnode[i])
      writeNode(state, node->subnode[i]);
  }
  return MS_SUCCESS;
}

int main(int argc, char *argv[]) {
  treeObj *tree;          /* In-memory index */
  shapefileObj shapefile; /* Input shapefile */
  SHPHandle outSHP;       /* Output SHP */
  DBFHandle outDBF;       /* Output DBF */
  DBFFieldType dbfField;
  writeState state;
  int shpType, nShapes;
  char fName[20];
  int fWidth, fnDecimals;
  char buffer[1024];
  int numFields;
  int i;

  if (argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  /* -------------------------------------------------------------------------------
   */
  /*       Check the number of arguments, return syntax if not correct */
  /* -------------------------------------------------------------------------------
   */
  if (argc != 3) {
    fprintf(stderr, "Syntax: %s [infile] [outfile]\n", argv[0]);
    exit(1);
  }

  msSetErrorFile("stderr", NULL);

  /* -------------------------------------------------------------------------------
   */
  /*       Open the shapefile */
  /* -------------------------------------------------------------------------------
   */
  if (msShapefileOpen(&shapefile, "rb", argv[1], MS_TRUE) < 0) {
    fprintf(stdout, "Error opening shapefile %s.\n", argv[1]);
    exit(0);
  }

  nShapes = shapefile.numshapes;
  shpType = shapefile.type;
  numFields = msDBFGetFieldCount(shapefile.hDBF);

  /* -------------------------------------------------------------------------------
   */
  /*       Index the shapefile (auto-calculate depth) */
  /* -------------------------------------------------------------------------------
   */
  tree = msCreateTree(&shapefile, 0);
  msTreeTrim(tree);

  /* -------------------------------------------------------------------------------
   */
  /*       Setup the output .shp/.shx and .dbf files */
  /* -------------------------------------------------------------------------------
   */
  outSHP = msSHPCreate(argv[2], shpType);
  if (outSHP == NULL) {
    fprintf(stderr, "Failed to create file '%s'.\n", argv[2]);
    exit(1);
  }

  sprintf(buffer, "%s.dbf", argv[2]);
  outDBF = msDBFCreate(buffer);
  if (outDBF == NULL) {
    fprintf(stderr, "Failed to create dbf file '%s'.\n", buffer);
    exit(1);
  }

  /* Clone the fields from the input to the output definition */
  for (i = 0; i < numFields; i++) {
    dbfField = msDBFGetFieldInfo(
        shapefile.hDBF, i, fName, &fWidth,
        &fnDecimals); /* ---- Get field info from in file ---- */
    msDBFAddField(outDBF, fName, dbfField, fWidth, fnDecimals);
  }

  /* -------------------------------------------------------------------------------
   */
  /*       Write the sorted .shp/.shx and .dbf files */
  /* -------------------------------------------------------------------------------
   */

  /* Set up writer state */
  state.inSHP = shapefile.hSHP;
  state.outSHP = outSHP;
  state.inDBF = shapefile.hDBF;
  state.outDBF = outDBF;
  state.currentOutRecord = 0;
  state.numDbfFields = numFields;

  writeNode(&state, tree->root);

  /* -------------------------------------------------------------------------------
   */
  /*       Write the spatial index */
  /* -------------------------------------------------------------------------------
   */
  msWriteTree(tree, AddFileSuffix(argv[2], MS_INDEX_EXTENSION),
              MS_NEW_LSB_ORDER);
  msDestroyTree(tree);

  /* -------------------------------------------------------------------------------
   */
  /*       Close all files */
  /* -------------------------------------------------------------------------------
   */
  msShapefileClose(&shapefile);
  msSHPClose(outSHP);
  msDBFClose(outDBF);

  fprintf(stdout, "Wrote %d spatially sorted shapes into shapefile '%s'\n",
          nShapes, argv[2]);
  return (0);
}

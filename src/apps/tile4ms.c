/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Create shapefile of rectangles from extents of several shapefiles
 *           (=tiles)
 *           Create DBF with file names for shape tiles, in column LOCATION as
 *           required by mapserv.
 *           For use with Mapserv tiling capability.
 * Author:   Herbie Freytag hfreytag@dlwc.nsw.gov.au
 *
 * Note:
 * Resulting shape files do not display in ArcView.
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
 *****************************************************************************/

/*
 * This is a modified version of Herbie's program that works with MapServer's
 * shapelib and should be using the MapServer makefile.
 *
 */

#include "../mapserver.h"
#include <string.h>

/***********************************************************************/
int process_shapefiles(char *metaFileNameP, char *tileFileNameP,
                       int tile_path_only) {
  SHPHandle hSHP, tileSHP;
  rectObj extentRect;
  lineObj line;
  shapeObj shapeRect;
  DBFHandle tileDBF;
  DBFHandle shpDBF;

  typedef struct DBFFieldDef_struct {
    DBFFieldType type;
    char name[12];
    int width;
    int decimals;
  } DBFFieldDef;

  DBFFieldDef *theFields = NULL;
  char fldname[256];
  int width;
  int decimals;
  int fieldCnt;
  int i;

  FILE *metaFP = NULL;
  char *p;
  char tileshapeName[256];
  char tiledbfName[256];
  char shapeFileName[256];
  int entityNum;

  int tilesFound = 0;
  int tilesProcessed = 0;

  msInitShape(&shapeRect);
  line.point = (pointObj *)msSmallMalloc(sizeof(pointObj) * 5);
  line.numpoints = 5;

  /* open metafile */
  /* ------------- */
  if (NULL == (metaFP = fopen(metaFileNameP, "r"))) {
    printf("Unable to open:%s\n", metaFileNameP);
    return (1);
  }

  /* create new tileindex shapefiles and create a header */
  /* -------------------------------------------------- */
  snprintf(tileshapeName, sizeof(tileshapeName), "%s.shp", tileFileNameP);

  if (NULL == (tileSHP = msSHPCreate(tileFileNameP, SHP_POLYGON))) {
    fclose(metaFP);
    printf("Unable to create %s.shp (.shx)\n", tileFileNameP);
    return (1);
  }

  /* create new tileindex dbf-file */
  /* ----------------------------- */
  snprintf(tiledbfName, sizeof(tiledbfName), "%s.dbf", tileFileNameP);
  if (NULL == (tileDBF = msDBFCreate(tiledbfName))) {
    fclose(metaFP);
    msSHPClose(tileSHP);
    printf("DBFCreate(%s) failed.\n", tiledbfName);
    return (1);
  }

  if (msDBFAddField(tileDBF, "LOCATION", FTString, 255, 0) == -1) {
    fclose(metaFP);
    msSHPClose(tileSHP);
    msDBFClose(tileDBF);
    printf("DBFAddField(fieldname='LOCATION') failed.\n");
    return (1);
  }

  /* loop through files listed in metafile */
  /* ===================================== */
  while (fgets(shapeFileName, 255, metaFP)) {

    if ((p = strchr(shapeFileName, '\n')) != NULL)
      *p = '\0';

    if (!strlen(shapeFileName))
      break;

    tilesFound++;

    /* read the DBFFields for this shapefile */
    /* and save them if the first, otherwise compare them */

    shpDBF = msDBFOpen(shapeFileName, "rb");

    if (shpDBF == NULL) {
      printf("Aborted. Unable to open DBF:%s\n", shapeFileName);
      break;
    }

    if (theFields == NULL) {
      fieldCnt = msDBFGetFieldCount(shpDBF);
      theFields = (DBFFieldDef *)msSmallCalloc(fieldCnt, sizeof(DBFFieldDef));
      for (i = 0; i < fieldCnt; i++) {
        theFields[i].type =
            msDBFGetFieldInfo(shpDBF, i, theFields[i].name, &theFields[i].width,
                              &theFields[i].decimals);
        if (theFields[i].type == FTInvalid) {
          printf("Aborted. DBF Invalid field (%d of %d) file:%s\n", i, fieldCnt,
                 shapeFileName);
          break;
        }
      }
    } else {
      fieldCnt = msDBFGetFieldCount(shpDBF);
      for (i = 0; i < fieldCnt; i++) {
        if (theFields[i].type !=
                msDBFGetFieldInfo(shpDBF, i, fldname, &width, &decimals) ||
            strcmp(theFields[i].name, fldname) || theFields[i].width != width ||
            theFields[i].decimals != decimals) {
          printf("Aborted. DBF fields do not match for file:%s\n",
                 shapeFileName);
          break;
        }
      }
    }
    msDBFClose(shpDBF);

    /* Get rid of .shp extension if it was included. */
    if (strlen(shapeFileName) > 4 &&
        (p = shapeFileName + strlen(shapeFileName) - 4) &&
        strcasecmp(p, ".shp") == 0)
      *p = '\0';

    if (!strlen(shapeFileName))
      break;

    /* read extent from shapefile */
    /* -------------------------- */
    hSHP = msSHPOpen(shapeFileName, "rb");

    if (hSHP == NULL) {
      printf("Aborted. Unable to open SHP:%s\n", shapeFileName);
      break;
    }

    msSHPReadBounds(hSHP, -1, &extentRect);
    /* SHPGetInfo(hSHP, &nEntities, &nShapeType, adfBndsMin, adfBndsMax); */

    /* printf("File:  %s Bounds 0/1:
     * (%15.10lg,%15.10lg)\n\t(%15.10lg,%15.10lg)\n", shapeFileName,
     * adfBndsMin[0], adfBndsMin[1], adfBndsMax[0], adfBndsMax[1] ); */

    msSHPClose(hSHP);

    /* create rectangle describing current shapefile extent */
    /* ---------------------------------------------------- */

    line.point[0].x = line.point[4].x = extentRect.minx; /* bottom left */
    line.point[0].y = line.point[4].y = extentRect.miny;
    line.point[1].x = extentRect.minx; /* top left */
    line.point[1].y = extentRect.maxy;
    line.point[2].x = extentRect.maxx; /* top left */
    line.point[2].y = extentRect.maxy;
    line.point[3].x = extentRect.maxx; /* bottom right */
    line.point[3].y = extentRect.miny;

    /* create and add shape object.  Returns link to entry in DBF file */
    /* --------------------------------------------------------------- */

    shapeRect.type = MS_SHAPE_POLYGON;
    msAddLine(&shapeRect, &line);
    entityNum = msSHPWriteShape(tileSHP, &shapeRect);

    msFreeShape(&shapeRect);

    /* store filepath of current shapefile as attribute of rectangle */
    /* ------------------------------------------------------------- */

    /* Strip off filename if requested */
    if (tile_path_only) {
      char *pszTmp;
      if ((pszTmp = strrchr(shapeFileName, '/')) != NULL ||
          (pszTmp = strrchr(shapeFileName, '\\')) != NULL) {
        *(pszTmp + 1) = '\0'; /* Keep the trailing '/' only. */
      }
    }

    msDBFWriteStringAttribute(tileDBF, entityNum, 0, shapeFileName);

    tilesProcessed++;
  }

  msSHPClose(tileSHP);
  msDBFClose(tileDBF);

  fclose(metaFP);

  free(line.point);
  free(theFields);

  printf("Processed %i of %i files\n", tilesProcessed, tilesFound);

  return (0);
}

/***********************************************************************/
void print_usage_and_exit(void) {

  printf("\nusage: tile4ms <meta-file> <tile-file> [-tile-path-only]\n");
  printf("<meta-file>\tINPUT  file containing list of shapefile "
         "names\n\t\t(complete paths 255 chars max, no extension)\n");
  printf("<tile-file>\tOUTPUT shape file of extent rectangles and "
         "names\n\t\tof tiles in <tile-file>.dbf\n");
  printf("-tile-path-only\tOptional flag.  If specified then only the path to "
         "the \n\t\tshape files will be stored in the LOCATION "
         "field\n\t\tinstead of storing the full filename.\n\n");
  exit(1);
}

/***********************************************************************/
int main(int argc, char **argv) {
  int tile_path_only = 0;

  /* stun user with existence of help  */
  /* -------------------------------- */
  if ((argc == 2) && (strstr(argv[1], "-h"))) {
    print_usage_and_exit();
  }

  /* check arguments */
  /* --------------- */
  if (argc < 3) {
    print_usage_and_exit();
  }

  if (argc == 4 && strcmp(argv[3], "-tile-path-only") == 0) {
    tile_path_only = 1;
  }

  process_shapefiles(argv[1], argv[2], tile_path_only);

  exit(0);
}

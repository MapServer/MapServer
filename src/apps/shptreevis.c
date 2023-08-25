/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Utility program to visualize a quadtree
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
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdlib.h>



#ifdef SHPT_POLYGON
#undef MAPSERVER
#else
#define MAPSERVER 1
#define SHPT_POLYGON SHP_POLYGON
#endif

char* AddFileSuffix ( const char * Filename, const char * Suffix )
{
  char  *pszFullname, *pszBasename;
  size_t i;

  /* -------------------------------------------------------------------- */
  /*  Compute the base (layer) name.  If there is any extension     */
  /*  on the passed in filename we will strip it off.         */
  /* -------------------------------------------------------------------- */
  pszBasename = (char *) msSmallMalloc(strlen(Filename)+5);
  strcpy( pszBasename, Filename );
  for( i = strlen(pszBasename)-1;
       i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
       && pszBasename[i] != '\\';
       i-- ) {}

  if( pszBasename[i] == '.' )
    pszBasename[i] = '\0';

  /* -------------------------------------------------------------------- */
  /*  Open the .shp and .shx files.  Note that files pulled from      */
  /*  a PC to Unix with upper case filenames won't work!        */
  /* -------------------------------------------------------------------- */
  pszFullname = (char *) msSmallMalloc(strlen(pszBasename) + 5);
  sprintf( pszFullname, "%s%s", pszBasename, Suffix);

  free(pszBasename);
  return (pszFullname);
}


int main( int argc, char ** argv )

{
  SHPHandle hSHP;
  DBFHandle   hDBF;
  SHPTreeHandle qix;

  char  *myfile = NULL;

  treeNodeObj *node;

#ifdef MAPSERVER
  shapeObj  shape;
  lineObj line[3];
  pointObj  pts[6];
#else
  SHPObject *shape;
  double  X[6], Y[6];
#endif
  int   result;
  
  /*
  char  mBigEndian;
  int   i;
  */

  int   this_rec;

  /* -------------------------------------------------------------------- */
  /*      Display a usage message.                                        */
  /* -------------------------------------------------------------------- */
  if( argc <= 2 ) {
    printf( "shptreevis shapefile new_shapefile \n" );
    exit( 1 );
  }

  /*
  i = 1;
  if( *((unsigned char *) &i) == 1 )
    mBigEndian = 0;
  else
    mBigEndian = 1;
  */


  qix = msSHPDiskTreeOpen (AddFileSuffix(argv[1],".qix"), 0 /* no debug*/);
  if( qix == NULL ) {
    printf("unable to open index file %s \n", argv[1]);
    exit(-1);
  }

  /* -------------------------------------------------------------------- */
  /*      Open the passed shapefile.                                      */
  /* -------------------------------------------------------------------- */
  myfile = AddFileSuffix(argv[2],".shp");

#ifdef MAPSERVER
  hSHP = msSHPCreate ( myfile, SHPT_POLYGON );
  hDBF = msDBFCreate (  AddFileSuffix(argv[2],".dbf") );
#else
  hSHP = SHPCreate ( myfile, SHPT_POLYGON );
  hDBF = DBFCreate (  AddFileSuffix(argv[2],".dbf") );
#endif

  if ( (!hSHP) || (!hDBF) ) {
    printf ("create error for %s    ... exiting \n", myfile);
    exit (-1);
  }

  /* add fields to dbf */
#ifdef MAPSERVER
  msDBFAddField ( hDBF, "ITEMS", FTInteger, 15,0 );
  msDBFAddField ( hDBF, "SUBNODES", FTInteger, 15,0 );
  msDBFAddField ( hDBF, "FACTOR", FTInteger, 15,0 );
#else
  DBFAddField ( hDBF, "ITEMS", FTInteger, 15,0 );
  DBFAddField ( hDBF, "SUBNODES", FTInteger, 15,0 );
  DBFAddField ( hDBF, "FACTOR", FTInteger, 15,0 );
#endif

#ifndef MAPSERVER
  SHPClose ( hSHP );
  hSHP = SHPOpen ( myfile, "r+b" );

  DBFClose (hDBF);
  hDBF = DBFOpen ( myfile, "r+b");
#endif

  printf ("This %s %s index supports a shapefile with %d shapes, %d depth \n",
          (qix->version ? "new": "old"), (qix->LSB_order? "LSB": "MSB"), (int) qix->nShapes, (int) qix->nDepth);


  /* -------------------------------------------------------------------- */
  /*  Skim over the list of shapes, printing all the vertices.  */
  /* -------------------------------------------------------------------- */

  while( 1 ) {
    node = readTreeNode (qix);
    if (node ) {

      this_rec = hDBF->nRecords;

#ifdef  MAPSERVER
      msDBFWriteIntegerAttribute( hDBF, this_rec, 0, node->numshapes);
      msDBFWriteIntegerAttribute( hDBF, this_rec, 1, node->numsubnodes);
#else
      DBFWriteIntegerAttribute( hDBF, this_rec, 0, node->numshapes);
      DBFWriteIntegerAttribute( hDBF, this_rec, 1, node->numsubnodes);
#endif

#ifdef  MAPSERVER
      shape.numlines = 1;
      shape.type = SHPT_POLYGON;

      pts[0].x = node->rect.minx;
      pts[0].y = node->rect.miny;
      pts[1].x = node->rect.maxx;
      pts[1].y = node->rect.miny;
      pts[2].x = node->rect.maxx;
      pts[2].y = node->rect.maxy;
      pts[3].x = node->rect.minx;
      pts[3].y = node->rect.maxy;
      pts[4].x = node->rect.minx;
      pts[4].y = node->rect.miny;

      line[0].numpoints = 5;
      line[0].point = &pts[0];
      shape.line = &line[0];
      shape.bounds = node->rect;

      result = msSHPWriteShape ( hSHP, &shape );
      if ( result < 0 ) {
        printf ("unable to write shape \n");
        exit (0);
      }

#else
      X[0] = node->rect.minx;
      X[1] = node->rect.maxx;
      X[2] = node->rect.maxx;
      X[3] = node->rect.minx;
      X[4] = node->rect.minx;

      Y[0] = node->rect.miny;
      Y[1] = node->rect.miny;
      Y[2] = node->rect.maxy;
      Y[3] = node->rect.maxy;
      Y[4] = node->rect.miny;

      shape = SHPCreateSimpleObject( SHPT_POLYGON, 5, X, Y, NULL);
      SHPWriteObject(hSHP, -1, shape);
      SHPDestroyObject ( shape );
#endif
    } else
      break;
  }

#ifdef MAPSERVER
  msSHPClose( hSHP );
  msDBFClose( hDBF );
#else
  SHPClose( hSHP );
  DBFClose( hDBF );
#endif

  msSHPDiskTreeClose (qix);

  return(0);
}

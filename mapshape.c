/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implements support for shapefile access.
 * Authors:  Steve Lime and Frank Warmerdam
 *
 * Note:
 * This code is entirely based on the previous work of Frank Warmerdam. It is
 * essentially shapelib 1.1.5. However, there were enough changes that it was
 * incorporated into the MapServer source to avoid confusion. Relicensed with
 * permission of Frank Warmerdam (shapelib author). See the README
 * for licence details.
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

#define NEED_IGNORE_RET_VAL

#include <limits.h>
#include <assert.h>
#include "mapserver.h"
#include "mapows.h"

#include <cpl_conv.h>
#include <ogr_srs_api.h>

/* Only use this macro on 32-bit integers! */
#define SWAP_FOUR_BYTES(data) \
  ( ((data >> 24) & 0x000000FF) | ((data >>  8) & 0x0000FF00) | \
    ((data <<  8) & 0x00FF0000) | ((data << 24) & 0xFF000000) )


#define ByteCopy( a, b, c )     memcpy( b, a, c )

static int      bBigEndian;

/************************************************************************/
/*                              SwapWord()                              */
/*                                                                      */
/*      Swap a 2, 4 or 8 byte word.                                     */
/************************************************************************/
static void SwapWord( int length, void * wordP )
{
  int i;
  uchar temp;

  for( i=0; i < length/2; i++ ) {
    temp = ((uchar *) wordP)[i];
    ((uchar *)wordP)[i] = ((uchar *) wordP)[length-i-1];
    ((uchar *) wordP)[length-i-1] = temp;
  }
}

/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/
static void * SfRealloc( void * pMem, int nNewSize )
{
  return realloc(pMem, nNewSize);
}

/************************************************************************/
/*                          writeHeader()                               */
/*                                                                      */
/*      Write out a header for the .shp and .shx files as well as the */
/*  contents of the index (.shx) file.        */
/************************************************************************/
static void writeHeader( SHPHandle psSHP )
{
  uchar abyHeader[100];
  int i;
  ms_int32 i32;
  double dValue;
  ms_int32 *panSHX;

  /* -------------------------------------------------------------------- */
  /*      Prepare header block for .shp file.                             */
  /* -------------------------------------------------------------------- */
  for( i = 0; i < 100; i++ )
    abyHeader[i] = 0;

  abyHeader[2] = 0x27;        /* magic cookie */
  abyHeader[3] = 0x0a;

  i32 = psSHP->nFileSize/2;       /* file size */
  ByteCopy( &i32, abyHeader+24, 4 );
  if( !bBigEndian ) SwapWord( 4, abyHeader+24 );

  i32 = 1000;           /* version */
  ByteCopy( &i32, abyHeader+28, 4 );
  if( bBigEndian ) SwapWord( 4, abyHeader+28 );

  i32 = psSHP->nShapeType;        /* shape type */
  ByteCopy( &i32, abyHeader+32, 4 );
  if( bBigEndian ) SwapWord( 4, abyHeader+32 );

  dValue = psSHP->adBoundsMin[0];     /* set bounds */
  ByteCopy( &dValue, abyHeader+36, 8 );
  if( bBigEndian ) SwapWord( 8, abyHeader+36 );

  dValue = psSHP->adBoundsMin[1];
  ByteCopy( &dValue, abyHeader+44, 8 );
  if( bBigEndian ) SwapWord( 8, abyHeader+44 );

  dValue = psSHP->adBoundsMax[0];
  ByteCopy( &dValue, abyHeader+52, 8 );
  if( bBigEndian ) SwapWord( 8, abyHeader+52 );

  dValue = psSHP->adBoundsMax[1];
  ByteCopy( &dValue, abyHeader+60, 8 );
  if( bBigEndian ) SwapWord( 8, abyHeader+60 );

  dValue = psSHP->adBoundsMin[2];     /* z */
  ByteCopy( &dValue, abyHeader+68, 8 );
  if( bBigEndian ) SwapWord( 8, abyHeader+68 );

  dValue = psSHP->adBoundsMax[2];
  ByteCopy( &dValue, abyHeader+76, 8 );
  if( bBigEndian ) SwapWord( 8, abyHeader+76 );


  dValue = psSHP->adBoundsMin[3];     /* m */
  ByteCopy( &dValue, abyHeader+84, 8 );
  if( bBigEndian ) SwapWord( 8, abyHeader+84 );

  dValue = psSHP->adBoundsMax[3];
  ByteCopy( &dValue, abyHeader+92, 8 );
  if( bBigEndian ) SwapWord( 8, abyHeader+92 );

  /* -------------------------------------------------------------------- */
  /*      Write .shp file header.                                         */
  /* -------------------------------------------------------------------- */
  VSIFSeekL( psSHP->fpSHP, 0, 0 );
  VSIFWriteL( abyHeader, 100, 1, psSHP->fpSHP );

  /* -------------------------------------------------------------------- */
  /*      Prepare, and write .shx file header.                            */
  /* -------------------------------------------------------------------- */
  i32 = (psSHP->nRecords * 2 * sizeof(ms_int32) + 100)/2;   /* file size */
  ByteCopy( &i32, abyHeader+24, 4 );
  if( !bBigEndian ) SwapWord( 4, abyHeader+24 );

  VSIFSeekL( psSHP->fpSHX, 0, 0 );
  VSIFWriteL( abyHeader, 100, 1, psSHP->fpSHX );

  /* -------------------------------------------------------------------- */
  /*      Write out the .shx contents.                                    */
  /* -------------------------------------------------------------------- */
  panSHX = (ms_int32 *) msSmallMalloc(sizeof(ms_int32) * 2 * psSHP->nRecords);

  for( i = 0; i < psSHP->nRecords; i++ ) {
    panSHX[i*2  ] = psSHP->panRecOffset[i]/2;
    panSHX[i*2+1] = psSHP->panRecSize[i]/2;
    if( !bBigEndian ) {
      *(panSHX+i*2) = SWAP_FOUR_BYTES(*(panSHX+i*2));
      *(panSHX+i*2+1) = SWAP_FOUR_BYTES(*(panSHX+i*2+1));
    }
  }

  VSIFWriteL( panSHX, sizeof(ms_int32) * 2, psSHP->nRecords, psSHP->fpSHX );

  free( panSHX );
}

/************************************************************************/
/*                              msSHPOpen()                             */
/*                                                                      */
/*      Open the .shp and .shx files based on the basename of the       */
/*      files or either file name.                                      */
/************************************************************************/
SHPHandle msSHPOpen( const char * pszLayer, const char * pszAccess )
{
  char *pszFullname, *pszBasename;
  SHPHandle psSHP;

  uchar *pabyBuf;
  int i;
  double dValue;

  /* -------------------------------------------------------------------- */
  /*      Ensure the access string is one of the legal ones.  We          */
  /*      ensure the result string indicates binary to avoid common       */
  /*      problems on Windows.                                            */
  /* -------------------------------------------------------------------- */
  if( strcmp(pszAccess,"rb+") == 0 || strcmp(pszAccess,"r+b") == 0 || strcmp(pszAccess,"r+") == 0 )
    pszAccess = "r+b";
  else
    pszAccess = "rb";

  /* -------------------------------------------------------------------- */
  /*  Establish the byte order on this machine.         */
  /* -------------------------------------------------------------------- */
  i = 1;
  /* cppcheck-suppress knownConditionTrueFalse */
  if( *((uchar *) &i) == 1 )
    bBigEndian = MS_FALSE;
  else
    bBigEndian = MS_TRUE;

  /* -------------------------------------------------------------------- */
  /*  Initialize the info structure.              */
  /* -------------------------------------------------------------------- */
  psSHP = (SHPHandle) msSmallMalloc(sizeof(SHPInfo));

  psSHP->bUpdated = MS_FALSE;

  psSHP->pabyRec = NULL;
  psSHP->panParts = NULL;
  psSHP->nBufSize = psSHP->nPartMax = 0;

  /* -------------------------------------------------------------------- */
  /*  Compute the base (layer) name.  If there is any extension     */
  /*  on the passed in filename we will strip it off.         */
  /* -------------------------------------------------------------------- */
  pszBasename = (char *) msSmallMalloc(strlen(pszLayer)+5);
  strcpy( pszBasename, pszLayer );
  for( i = strlen(pszBasename)-1;
       i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' && pszBasename[i] != '\\';
       i-- ) {}

  if( pszBasename[i] == '.' )
    pszBasename[i] = '\0';

  /* -------------------------------------------------------------------- */
  /*  Open the .shp and .shx files.  Note that files pulled from      */
  /*  a PC to Unix with upper case filenames won't work!        */
  /* -------------------------------------------------------------------- */
  pszFullname = (char *) msSmallMalloc(strlen(pszBasename) + 5);
  sprintf( pszFullname, "%s.shp", pszBasename );
  psSHP->fpSHP = VSIFOpenL(pszFullname, pszAccess );
  if( psSHP->fpSHP == NULL ) {
    sprintf( pszFullname, "%s.SHP", pszBasename );
    psSHP->fpSHP = VSIFOpenL(pszFullname, pszAccess );
  }
  if( psSHP->fpSHP == NULL ) {
    msFree(pszBasename);
    msFree(pszFullname);
    msFree(psSHP);
    return( NULL );
  }

  sprintf( pszFullname, "%s.shx", pszBasename );
  psSHP->fpSHX = VSIFOpenL(pszFullname, pszAccess );
  if( psSHP->fpSHX == NULL ) {
    sprintf( pszFullname, "%s.SHX", pszBasename );
    psSHP->fpSHX = VSIFOpenL(pszFullname, pszAccess );
  }
  if( psSHP->fpSHX == NULL ) {
    VSIFCloseL(psSHP->fpSHP);
    msFree(pszBasename);
    msFree(pszFullname);
    msFree(psSHP);
    return( NULL );
  }

  free( pszFullname );
  free( pszBasename );

  /* -------------------------------------------------------------------- */
  /*   Read the file size from the SHP file.            */
  /* -------------------------------------------------------------------- */
  pabyBuf = (uchar *) msSmallMalloc(100);
  if(1 != VSIFReadL( pabyBuf, 100, 1, psSHP->fpSHP )) {
    VSIFCloseL( psSHP->fpSHP );
    VSIFCloseL( psSHP->fpSHX );
    free( psSHP );
    free(pabyBuf);
    return( NULL );
  }

  psSHP->nFileSize = (pabyBuf[24] * 256 * 256 * 256
                      + pabyBuf[25] * 256 * 256
                      + pabyBuf[26] * 256
                      + pabyBuf[27]) * 2;

  /* -------------------------------------------------------------------- */
  /*  Read SHX file Header info                                           */
  /* -------------------------------------------------------------------- */
  if(1 != VSIFReadL( pabyBuf, 100, 1, psSHP->fpSHX )) {
    msSetError(MS_SHPERR, "Corrupted .shx file", "msSHPOpen()");
    VSIFCloseL( psSHP->fpSHP );
    VSIFCloseL( psSHP->fpSHX );
    free( psSHP );
    free(pabyBuf);
    return( NULL );
  }

  if( pabyBuf[0] != 0 || pabyBuf[1] != 0 || pabyBuf[2] != 0x27  || (pabyBuf[3] != 0x0a && pabyBuf[3] != 0x0d) ) {
    msSetError(MS_SHPERR, "Corrupted .shp file", "msSHPOpen()");
    VSIFCloseL( psSHP->fpSHP );
    VSIFCloseL( psSHP->fpSHX );
    free( psSHP );
    free(pabyBuf);

    return( NULL );
  }

  psSHP->nRecords = pabyBuf[27] + pabyBuf[26] * 256 + pabyBuf[25] * 256 * 256 + pabyBuf[24] * 256 * 256 * 256;
  if (psSHP->nRecords != 0)
    psSHP->nRecords = (psSHP->nRecords*2 - 100) / 8;

  if( psSHP->nRecords < 0 || psSHP->nRecords > 256000000 ) {
    msSetError(MS_SHPERR, "Corrupted .shp file : nRecords = %d.", "msSHPOpen()",
               psSHP->nRecords);
    VSIFCloseL( psSHP->fpSHP );
    VSIFCloseL( psSHP->fpSHX );
    free( psSHP );
    free(pabyBuf);
    return( NULL );
  }

  psSHP->nShapeType = pabyBuf[32];

  if( bBigEndian ) SwapWord( 8, pabyBuf+36 );
  memcpy( &dValue, pabyBuf+36, 8 );
  psSHP->adBoundsMin[0] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+44 );
  memcpy( &dValue, pabyBuf+44, 8 );
  psSHP->adBoundsMin[1] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+52 );
  memcpy( &dValue, pabyBuf+52, 8 );
  psSHP->adBoundsMax[0] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+60 );
  memcpy( &dValue, pabyBuf+60, 8 );
  psSHP->adBoundsMax[1] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+68 );   /* z */
  memcpy( &dValue, pabyBuf+68, 8 );
  psSHP->adBoundsMin[2] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+76 );
  memcpy( &dValue, pabyBuf+76, 8 );
  psSHP->adBoundsMax[2] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+84 );   /* m */
  memcpy( &dValue, pabyBuf+84, 8 );
  psSHP->adBoundsMin[3] = dValue;

  if( bBigEndian ) SwapWord( 8, pabyBuf+92 );
  memcpy( &dValue, pabyBuf+92, 8 );
  psSHP->adBoundsMax[3] = dValue;
  free( pabyBuf );

  /* -------------------------------------------------------------------- */
  /*  Read the .shx file to get the offsets to each record in       */
  /*  the .shp file.                  */
  /* -------------------------------------------------------------------- */
  psSHP->nMaxRecords = psSHP->nRecords;

  /* Our in-memory cache of offset information */
  psSHP->panRecOffset = (int *) malloc(sizeof(int) * psSHP->nMaxRecords );
  /* Our in-memory cache of size information */
  psSHP->panRecSize = (int *) malloc(sizeof(int) * psSHP->nMaxRecords );
  /* The completeness information for our in-memory cache */
  psSHP->panRecLoaded = msAllocBitArray( 1 + (psSHP->nMaxRecords / SHX_BUFFER_PAGE) ) ;
  /* Is our in-memory cache completely populated? */
  psSHP->panRecAllLoaded = 0;

  /* malloc failed? clean up and shut down */
  if (psSHP->panRecOffset == NULL ||
      psSHP->panRecSize == NULL ||
      psSHP->panRecLoaded == NULL) {
    free(psSHP->panRecOffset);
    free(psSHP->panRecSize);
    free(psSHP->panRecLoaded);
    VSIFCloseL( psSHP->fpSHP );
    VSIFCloseL( psSHP->fpSHX );
    free( psSHP );
    msSetError(MS_MEMERR, "Out of memory", "msSHPOpen()");
    return( NULL );
  }


  return( psSHP );
}

/************************************************************************/
/*                              msSHPClose()                            */
/*                        */
/*  Close the .shp and .shx files.          */
/************************************************************************/
void msSHPClose(SHPHandle psSHP )
{
  /* -------------------------------------------------------------------- */
  /*  Update the header if we have modified anything.           */
  /* -------------------------------------------------------------------- */
  if( psSHP->bUpdated )
    writeHeader( psSHP );

  /* -------------------------------------------------------------------- */
  /*      Free all resources, and close files.                            */
  /* -------------------------------------------------------------------- */
  free( psSHP->panRecOffset );
  free( psSHP->panRecSize );
  free( psSHP->panRecLoaded );


  free(psSHP->pabyRec);
  free(psSHP->panParts);

  VSIFCloseL( psSHP->fpSHX );
  VSIFCloseL( psSHP->fpSHP );

  free( psSHP );
}

/************************************************************************/
/*                             msSHPGetInfo()                           */
/*                                                                      */
/*      Fetch general information about the shape file.                 */
/************************************************************************/
void msSHPGetInfo(SHPHandle psSHP, int * pnEntities, int * pnShapeType )
{
  if( pnEntities )
    *pnEntities = psSHP->nRecords;

  if( pnShapeType )
    *pnShapeType = psSHP->nShapeType;
}

/************************************************************************/
/*                             msSHPCreate()                            */
/*                                                                      */
/*      Create a new shape file and return a handle to the open         */
/*      shape file with read/write access.                              */
/************************************************************************/
SHPHandle msSHPCreate( const char * pszLayer, int nShapeType )
{
  char *pszBasename, *pszFullname;
  int i;
  VSILFILE *fpSHP, *fpSHX;
  uchar abyHeader[100];
  ms_int32 i32;
  double dValue;

  /* -------------------------------------------------------------------- */
  /*      Establish the byte order on this system.                        */
  /* -------------------------------------------------------------------- */
  i = 1;
  /* cppcheck-suppress knownConditionTrueFalse */
  if( *((uchar *) &i) == 1 )
    bBigEndian = MS_FALSE;
  else
    bBigEndian = MS_TRUE;

  /* -------------------------------------------------------------------- */
  /*  Compute the base (layer) name.  If there is any extension       */
  /*  on the passed in filename we will strip it off.         */
  /* -------------------------------------------------------------------- */
  pszBasename = (char *) msSmallMalloc(strlen(pszLayer)+5);
  strcpy( pszBasename, pszLayer );
  for( i = strlen(pszBasename)-1;
       i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' && pszBasename[i] != '\\';
       i-- ) {}

  if( pszBasename[i] == '.' )
    pszBasename[i] = '\0';

  /* -------------------------------------------------------------------- */
  /*      Open the two files so we can write their headers.               */
  /* -------------------------------------------------------------------- */
  pszFullname = (char *) msSmallMalloc(strlen(pszBasename) + 5);
  sprintf( pszFullname, "%s.shp", pszBasename );
  fpSHP = VSIFOpenL(pszFullname, "wb" );
  if( fpSHP == NULL ) {
    free( pszFullname );
    free(pszBasename);
    return( NULL );
  }

  sprintf( pszFullname, "%s.shx", pszBasename );
  fpSHX = VSIFOpenL(pszFullname, "wb" );
  if( fpSHX == NULL ) {
    VSIFCloseL(fpSHP);
    free( pszFullname );
    free(pszBasename);
    return( NULL );
  }

  free( pszFullname );
  free(pszBasename);

  /* -------------------------------------------------------------------- */
  /*      Prepare header block for .shp file.                             */
  /* -------------------------------------------------------------------- */
  for( i = 0; i < 100; i++ )
    abyHeader[i] = 0;

  abyHeader[2] = 0x27;        /* magic cookie */
  abyHeader[3] = 0x0a;

  i32 = 50;           /* file size */
  ByteCopy( &i32, abyHeader+24, 4 );
  if( !bBigEndian ) SwapWord( 4, abyHeader+24 );

  i32 = 1000;           /* version */
  ByteCopy( &i32, abyHeader+28, 4 );
  if( bBigEndian ) SwapWord( 4, abyHeader+28 );

  i32 = nShapeType;         /* shape type */
  ByteCopy( &i32, abyHeader+32, 4 );
  if( bBigEndian ) SwapWord( 4, abyHeader+32 );

  dValue = 0.0;         /* set bounds */
  ByteCopy( &dValue, abyHeader+36, 8 );
  ByteCopy( &dValue, abyHeader+44, 8 );
  ByteCopy( &dValue, abyHeader+52, 8 );
  ByteCopy( &dValue, abyHeader+60, 8 );

  /* -------------------------------------------------------------------- */
  /*      Write .shp file header.                                         */
  /* -------------------------------------------------------------------- */
  VSIFWriteL( abyHeader, 100, 1, fpSHP );

  /* -------------------------------------------------------------------- */
  /*      Prepare, and write .shx file header.                            */
  /* -------------------------------------------------------------------- */
  i32 = 50;           /* file size */
  ByteCopy( &i32, abyHeader+24, 4 );
  if( !bBigEndian ) SwapWord( 4, abyHeader+24 );

  VSIFWriteL( abyHeader, 100, 1, fpSHX );

  /* -------------------------------------------------------------------- */
  /*      Close the files, and then open them as regular existing files.  */
  /* -------------------------------------------------------------------- */
  VSIFCloseL( fpSHP );
  VSIFCloseL( fpSHX );

  return( msSHPOpen( pszLayer, "rb+" ) );
}

/************************************************************************/
/*                           writeBounds()                              */
/*                                                                      */
/*      Compute a bounds rectangle for a shape, and set it into the     */
/*      indicated location in the record.                               */
/************************************************************************/
static void writeBounds( uchar * pabyRec, shapeObj *shape, int nVCount )
{
  double dXMin, dXMax, dYMin, dYMax;
  int i, j;

  if( nVCount == 0 ) {
    dXMin = dYMin = dXMax = dYMax = 0.0;
  } else {
    dXMin = dXMax = shape->line[0].point[0].x;
    dYMin = dYMax = shape->line[0].point[0].y;

    for( i=0; i<shape->numlines; i++ ) {
      for( j=0; j<shape->line[i].numpoints; j++ ) {
        dXMin = MS_MIN(dXMin, shape->line[i].point[j].x);
        dXMax = MS_MAX(dXMax, shape->line[i].point[j].x);
        dYMin = MS_MIN(dYMin, shape->line[i].point[j].y);
        dYMax = MS_MAX(dYMax, shape->line[i].point[j].y);
      }
    }
  }

  if( bBigEndian ) {
    SwapWord( 8, &dXMin );
    SwapWord( 8, &dYMin );
    SwapWord( 8, &dXMax );
    SwapWord( 8, &dYMax );
  }

  ByteCopy( &dXMin, pabyRec +  0, 8 );
  ByteCopy( &dYMin, pabyRec +  8, 8 );
  ByteCopy( &dXMax, pabyRec + 16, 8 );
  ByteCopy( &dYMax, pabyRec + 24, 8 );
}

int msSHPWritePoint(SHPHandle psSHP, pointObj *point )
{
  int nRecordOffset, nRecordSize=0;
  uchar *pabyRec;
  ms_int32  i32, nPoints, nParts;

  if( psSHP->nShapeType != SHP_POINT) return(-1);

  psSHP->bUpdated = MS_TRUE;

  /* Fill the SHX buffer if it is not already full. */
  if( ! psSHP->panRecAllLoaded ) msSHXLoadAll( psSHP );

  /* -------------------------------------------------------------------- */
  /*      Add the new entity to the in memory index.                      */
  /* -------------------------------------------------------------------- */
  psSHP->nRecords++;
  if( psSHP->nRecords > psSHP->nMaxRecords ) {
    psSHP->nMaxRecords = (int) (psSHP->nMaxRecords * 1.3 + 100);

    psSHP->panRecOffset = (int *) SfRealloc(psSHP->panRecOffset,sizeof(int) * psSHP->nMaxRecords );
    psSHP->panRecSize = (int *) SfRealloc(psSHP->panRecSize,sizeof(int) * psSHP->nMaxRecords );
  }

  /* -------------------------------------------------------------------- */
  /*      Compute a few things.                                           */
  /* -------------------------------------------------------------------- */
  nPoints = 1;
  nParts = 1;

  /* -------------------------------------------------------------------- */
  /*      Initialize record.                                              */
  /* -------------------------------------------------------------------- */
  psSHP->panRecOffset[psSHP->nRecords-1] = nRecordOffset = psSHP->nFileSize;

  pabyRec = (uchar *) msSmallMalloc(nPoints * 2 * sizeof(double) + nParts * 4 + 128);

  /* -------------------------------------------------------------------- */
  /*      Write vertices for a point.                                     */
  /* -------------------------------------------------------------------- */
  ByteCopy( &(point->x), pabyRec + 12, 8 );
  ByteCopy( &(point->y), pabyRec + 20, 8 );


  if( bBigEndian ) {
    SwapWord( 8, pabyRec + 12 );
    SwapWord( 8, pabyRec + 20 );
  }

  nRecordSize = 20;

  /* -------------------------------------------------------------------- */
  /*      Set the shape type, record number, and record size.             */
  /* -------------------------------------------------------------------- */
  i32 = psSHP->nRecords-1+1;          /* record # */
  if( !bBigEndian ) i32 = SWAP_FOUR_BYTES(i32);
  ByteCopy( &i32, pabyRec, 4 );

  i32 = nRecordSize/2;        /* record size */
  if( !bBigEndian ) i32 = SWAP_FOUR_BYTES(i32);
  ByteCopy( &i32, pabyRec + 4, 4 );

  i32 = psSHP->nShapeType;        /* shape type */
  if( bBigEndian ) i32 = SWAP_FOUR_BYTES(i32);
  ByteCopy( &i32, pabyRec + 8, 4 );

  /* -------------------------------------------------------------------- */
  /*      Write out record.                                               */
  /* -------------------------------------------------------------------- */
  if(VSIFSeekL( psSHP->fpSHP, nRecordOffset, 0 ) == 0) {
    VSIFWriteL( pabyRec, nRecordSize+8, 1, psSHP->fpSHP );

    psSHP->panRecSize[psSHP->nRecords-1] = nRecordSize;
    psSHP->nFileSize += nRecordSize + 8;

    /* -------------------------------------------------------------------- */
    /*  Expand file wide bounds based on this shape.        */
    /* -------------------------------------------------------------------- */
    if( psSHP->nRecords == 1 ) {
      psSHP->adBoundsMin[0] = psSHP->adBoundsMax[0] = point->x;
      psSHP->adBoundsMin[1] = psSHP->adBoundsMax[1] = point->y;
    } else {
      psSHP->adBoundsMin[0] = MS_MIN(psSHP->adBoundsMin[0], point->x);
      psSHP->adBoundsMin[1] = MS_MIN(psSHP->adBoundsMin[1], point->y);
      psSHP->adBoundsMax[0] = MS_MAX(psSHP->adBoundsMax[0], point->x);
      psSHP->adBoundsMax[1] = MS_MAX(psSHP->adBoundsMax[1], point->y);
    }
  } else {
    psSHP->nRecords -- ;
  }
  free( pabyRec );
  return( psSHP->nRecords - 1 );
}

int msSHPWriteShape(SHPHandle psSHP, shapeObj *shape )
{
  int nRecordOffset, i, j, k, nRecordSize=0;
  uchar *pabyRec;
  int nShapeType;

  ms_int32  i32, nPoints, nParts;
  psSHP->bUpdated = MS_TRUE;

  /* Fill the SHX buffer if it is not already full. */
  if( ! psSHP->panRecAllLoaded ) msSHXLoadAll( psSHP );

  /* -------------------------------------------------------------------- */
  /*      Add the new entity to the in memory index.                      */
  /* -------------------------------------------------------------------- */
  psSHP->nRecords++;
  if( psSHP->nRecords > psSHP->nMaxRecords ) {
    psSHP->nMaxRecords = (int) (psSHP->nMaxRecords * 1.3 + 100);

    psSHP->panRecOffset = (int *) SfRealloc(psSHP->panRecOffset,sizeof(int) * psSHP->nMaxRecords );
    psSHP->panRecSize = (int *) SfRealloc(psSHP->panRecSize,sizeof(int) * psSHP->nMaxRecords );
  }

  /* -------------------------------------------------------------------- */
  /*      Compute a few things.                                           */
  /* -------------------------------------------------------------------- */
  nPoints = 0;
  for(i=0; i<shape->numlines; i++)
    nPoints += shape->line[i].numpoints;

  nParts = shape->numlines;

  /* -------------------------------------------------------------------- */
  /*      Initialize record.                                              */
  /* -------------------------------------------------------------------- */
  psSHP->panRecOffset[psSHP->nRecords-1] = nRecordOffset = psSHP->nFileSize;

  pabyRec = (uchar *) msSmallMalloc(nPoints * 4 * sizeof(double) + nParts * 8 + 128);
  nShapeType = psSHP->nShapeType;

  if (shape->type == MS_SHAPE_NULL) {
    nShapeType = 0;
    nRecordSize = 12;
  }
  /* -------------------------------------------------------------------- */
  /*  Write vertices for a Polygon or Arc.            */
  /* -------------------------------------------------------------------- */
  else if(psSHP->nShapeType == SHP_POLYGON || psSHP->nShapeType == SHP_ARC ||
          psSHP->nShapeType == SHP_POLYGONM || psSHP->nShapeType == SHP_ARCM ||
          psSHP->nShapeType == SHP_ARCZ ||  psSHP->nShapeType == SHP_POLYGONZ) {
    ms_int32 t_nParts, t_nPoints, partSize;

    t_nParts = nParts;
    t_nPoints = nPoints;

    writeBounds( pabyRec + 12, shape, t_nPoints );

    if( bBigEndian ) {
      nPoints = SWAP_FOUR_BYTES(nPoints);
      nParts = SWAP_FOUR_BYTES(nParts);
    }

    ByteCopy( &nPoints, pabyRec + 40 + 8, 4 );
    ByteCopy( &nParts, pabyRec + 36 + 8, 4 );

    partSize = 0; /* first part always starts at 0 */
    ByteCopy( &partSize, pabyRec + 44 + 8 + 4*0, 4 );
    if( bBigEndian ) SwapWord( 4, pabyRec + 44 + 8 + 4*0);

    for( i = 1; i < t_nParts; i++ ) {
      partSize += shape->line[i-1].numpoints;
      ByteCopy( &partSize, pabyRec + 44 + 8 + 4*i, 4 );
      if( bBigEndian ) SwapWord( 4, pabyRec + 44 + 8 + 4*i);

    }

    k = 0; /* overall point counter */
    for( i = 0; i < shape->numlines; i++ ) {
      for( j = 0; j < shape->line[i].numpoints; j++ ) {
        ByteCopy( &(shape->line[i].point[j].x), pabyRec + 44 + 4*t_nParts + 8 + k * 16, 8 );
        ByteCopy( &(shape->line[i].point[j].y), pabyRec + 44 + 4*t_nParts + 8 + k * 16 + 8, 8 );

        if( bBigEndian ) {
          SwapWord( 8, pabyRec + 44+4*t_nParts+8+k*16 );
          SwapWord( 8, pabyRec + 44+4*t_nParts+8+k*16+8 );
        }

        k++;
      }
    }

    nRecordSize = 44 + 4*t_nParts + 16 * t_nPoints;

    /* -------------------------------------------------------------------- */
    /*      measured shape : polygon and arc.                               */
    /* -------------------------------------------------------------------- */
    if(psSHP->nShapeType == SHP_POLYGONM || psSHP->nShapeType == SHP_ARCM) {
      const double dfMMin = shape->line[0].point[0].m;
      const double dfMMax = shape->line[shape->numlines-1].point[shape->line[shape->numlines-1].numpoints-1].m;

      nRecordSize = 44 + 4*t_nParts + 8 + (t_nPoints* 16);

      ByteCopy( &(dfMMin), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;

      ByteCopy( &(dfMMax), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;

      for( i = 0; i < shape->numlines; i++ ) {
        for( j = 0; j < shape->line[i].numpoints; j++ ) {
          ByteCopy( &(shape->line[i].point[j].m), pabyRec + nRecordSize, 8 );
          if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
          nRecordSize += 8;
        }
      }
    }

    /* -------------------------------------------------------------------- */
    /*      Polygon. Arc with Z                                             */
    /* -------------------------------------------------------------------- */
    if (psSHP->nShapeType == SHP_POLYGONZ || psSHP->nShapeType == SHP_ARCZ
        || psSHP->nShapeType == SHP_POLYGONM || psSHP->nShapeType == SHP_ARCM) {
      const double dfMMin = shape->line[0].point[0].z;
      const double dfMMax = shape->line[shape->numlines-1].point[shape->line[shape->numlines-1].numpoints-1].z;

      nRecordSize = 44 + 4*t_nParts + 8 + (t_nPoints* 16);

      ByteCopy( &(dfMMin), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;

      ByteCopy( &(dfMMax), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;

      for( i = 0; i < shape->numlines; i++ ) {
        for( j = 0; j < shape->line[i].numpoints; j++ ) {
          ByteCopy( &(shape->line[i].point[j].z), pabyRec + nRecordSize, 8 );
          if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
          nRecordSize += 8;
        }
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*  Write vertices for a MultiPoint.                                    */
  /* -------------------------------------------------------------------- */
  else if( psSHP->nShapeType == SHP_MULTIPOINT ||
           psSHP->nShapeType == SHP_MULTIPOINTM ||
           psSHP->nShapeType == SHP_MULTIPOINTZ) {
    ms_int32 t_nPoints;

    t_nPoints = nPoints;

    writeBounds( pabyRec + 12, shape, nPoints );

    if( bBigEndian ) nPoints = SWAP_FOUR_BYTES(nPoints);
    ByteCopy( &nPoints, pabyRec + 44, 4 );

    for( i = 0; i < shape->line[0].numpoints; i++ ) {
      ByteCopy( &(shape->line[0].point[i].x), pabyRec + 48 + i*16, 8 );
      ByteCopy( &(shape->line[0].point[i].y), pabyRec + 48 + i*16 + 8, 8 );

      if( bBigEndian ) {
        SwapWord( 8, pabyRec + 48 + i*16 );
        SwapWord( 8, pabyRec + 48 + i*16 + 8 );
      }
    }

    nRecordSize = 40 + 16 * t_nPoints;

    if (psSHP->nShapeType == SHP_MULTIPOINTM) {
      const double dfMMin = shape->line[0].point[0].m;
      const double dfMMax = shape->line[0].point[shape->line[0].numpoints-1].m;

      ByteCopy( &(dfMMin), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;

      ByteCopy( &(dfMMax), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;

      for( i = 0; i < shape->line[0].numpoints; i++ ) {
        ByteCopy( &(shape->line[0].point[i].m), pabyRec + nRecordSize, 8 );
        if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
        nRecordSize += 8;
      }
    }

    if (psSHP->nShapeType == SHP_MULTIPOINTZ) {
      const double dfMMin = shape->line[0].point[0].z;
      const double dfMMax = shape->line[0].point[shape->line[0].numpoints-1].z;

      ByteCopy( &(dfMMin), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;

      ByteCopy( &(dfMMax), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;

      for( i = 0; i < shape->line[0].numpoints; i++ ) {
        ByteCopy( &(shape->line[0].point[i].z), pabyRec + nRecordSize, 8 );
        if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
        nRecordSize += 8;
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Write vertices for a point.                                     */
  /* -------------------------------------------------------------------- */
  else if( psSHP->nShapeType == SHP_POINT ||  psSHP->nShapeType == SHP_POINTM ||
           psSHP->nShapeType == SHP_POINTZ) {
    ByteCopy( &(shape->line[0].point[0].x), pabyRec + 12, 8 );
    ByteCopy( &(shape->line[0].point[0].y), pabyRec + 20, 8 );

    if( bBigEndian ) {
      SwapWord( 8, pabyRec + 12 );
      SwapWord( 8, pabyRec + 20 );
    }

    nRecordSize = 20;

    if (psSHP->nShapeType == SHP_POINTM) {
      ByteCopy( &(shape->line[0].point[0].m), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;
    }

    if (psSHP->nShapeType == SHP_POINTZ) {
      ByteCopy( &(shape->line[0].point[0].z), pabyRec + nRecordSize, 8 );
      if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
      nRecordSize += 8;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Set the shape type, record number, and record size.             */
  /* -------------------------------------------------------------------- */
  i32 = psSHP->nRecords-1+1;          /* record # */
  if( !bBigEndian ) i32 = SWAP_FOUR_BYTES(i32);
  ByteCopy( &i32, pabyRec, 4 );

  i32 = nRecordSize/2;        /* record size */
  if( !bBigEndian ) i32 = SWAP_FOUR_BYTES(i32);
  ByteCopy( &i32, pabyRec + 4, 4 );

  i32 = nShapeType;     /* shape type */
  if( bBigEndian ) i32 = SWAP_FOUR_BYTES(i32);
  ByteCopy( &i32, pabyRec + 8, 4 );

  /* -------------------------------------------------------------------- */
  /*      Write out record.                                               */
  /* -------------------------------------------------------------------- */
  if(VSIFSeekL( psSHP->fpSHP, nRecordOffset, 0 ) == 0) {
    VSIFWriteL( pabyRec, nRecordSize+8, 1, psSHP->fpSHP );

    psSHP->panRecSize[psSHP->nRecords-1] = nRecordSize;
    psSHP->nFileSize += nRecordSize + 8;

    /* -------------------------------------------------------------------- */
    /*  Expand file wide bounds based on this shape.        */
    /* -------------------------------------------------------------------- */
    if( psSHP->nRecords == 1 ) {
      psSHP->adBoundsMin[0] = psSHP->adBoundsMax[0] = shape->line[0].point[0].x;
      psSHP->adBoundsMin[1] = psSHP->adBoundsMax[1] = shape->line[0].point[0].y;
      psSHP->adBoundsMin[2] = psSHP->adBoundsMax[2] = shape->line[0].point[0].z;
      psSHP->adBoundsMin[3] = psSHP->adBoundsMax[3] = shape->line[0].point[0].m;
    }

    for( i=0; i<shape->numlines; i++ ) {
      for( j=0; j<shape->line[i].numpoints; j++ ) {
        psSHP->adBoundsMin[0] = MS_MIN(psSHP->adBoundsMin[0], shape->line[i].point[j].x);
        psSHP->adBoundsMin[1] = MS_MIN(psSHP->adBoundsMin[1], shape->line[i].point[j].y);
        psSHP->adBoundsMin[2] = MS_MIN(psSHP->adBoundsMin[2], shape->line[i].point[j].z);
        psSHP->adBoundsMin[3] = MS_MIN(psSHP->adBoundsMin[3], shape->line[i].point[j].m);
        psSHP->adBoundsMax[0] = MS_MAX(psSHP->adBoundsMax[0], shape->line[i].point[j].x);
        psSHP->adBoundsMax[1] = MS_MAX(psSHP->adBoundsMax[1], shape->line[i].point[j].y);
        psSHP->adBoundsMax[2] = MS_MAX(psSHP->adBoundsMax[2], shape->line[i].point[j].z);
        psSHP->adBoundsMax[3] = MS_MAX(psSHP->adBoundsMax[3], shape->line[i].point[j].m);
      }
    }

  } else {
    psSHP->nRecords --;
    /* there was an error writing the record */
  }
  free( pabyRec );
  return( psSHP->nRecords - 1 );
}

/*
 ** msSHPReadAllocateBuffer() - Ensure our record buffer is large enough.
 */
static int msSHPReadAllocateBuffer( SHPHandle psSHP, int hEntity, const char* pszCallingFunction)
{

  int nEntitySize = msSHXReadSize(psSHP, hEntity);
  if( nEntitySize > INT_MAX - 8 ) {
      msSetError(MS_MEMERR, "Out of memory. Cannot allocate %d bytes. Probably broken shapefile at feature %d",
                 pszCallingFunction, nEntitySize, hEntity);
      return(MS_FAILURE);
  }
  nEntitySize += 8;
  /* -------------------------------------------------------------------- */
  /*      Ensure our record buffer is large enough.                       */
  /* -------------------------------------------------------------------- */
  if( nEntitySize > psSHP->nBufSize ) {
    uchar* pabyRec = (uchar *) SfRealloc(psSHP->pabyRec,nEntitySize);
    if (pabyRec == NULL) {
      msSetError(MS_MEMERR, "Out of memory. Cannot allocate %d bytes. Probably broken shapefile at feature %d",
                 pszCallingFunction, nEntitySize, hEntity);
      return(MS_FAILURE);
    }
    psSHP->pabyRec = pabyRec;
    psSHP->nBufSize = nEntitySize;
  }
  if (psSHP->pabyRec == NULL) {
    msSetError(MS_MEMERR, "Out of memory", pszCallingFunction);
    return(MS_FAILURE);
  }
  return MS_SUCCESS;
}

/*
** msSHPReadPoint() - Reads a single point from a POINT shape file.
*/
int msSHPReadPoint( SHPHandle psSHP, int hEntity, pointObj *point )
{
  int nEntitySize;

  /* -------------------------------------------------------------------- */
  /*      Only valid for point shapefiles                                 */
  /* -------------------------------------------------------------------- */
  if( psSHP->nShapeType != SHP_POINT ) {
    msSetError(MS_SHPERR, "msSHPReadPoint only operates on point shapefiles.", "msSHPReadPoint()");
    return(MS_FAILURE);
  }

  /* -------------------------------------------------------------------- */
  /*      Validate the record/entity number.                              */
  /* -------------------------------------------------------------------- */
  if( hEntity < 0 || hEntity >= psSHP->nRecords ) {
    msSetError(MS_SHPERR, "Record index out of bounds.", "msSHPReadPoint()");
    return(MS_FAILURE);
  }

  nEntitySize = msSHXReadSize( psSHP, hEntity) + 8;

  if( msSHXReadSize( psSHP, hEntity) == 4 ) {
    msSetError(MS_SHPERR, "NULL feature encountered.", "msSHPReadPoint()");
    return(MS_FAILURE);
  } else if ( nEntitySize < 28 ) {
    msSetError(MS_SHPERR, "Corrupted feature encountered.  hEntity=%d, nEntitySize=%d", "msSHPReadPoint()",
               hEntity, nEntitySize);
    return(MS_FAILURE);
  }

  if (msSHPReadAllocateBuffer(psSHP, hEntity, "msSHPReadPoint()") == MS_FAILURE) {
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      Read the record.                                                */
  /* -------------------------------------------------------------------- */
  if( 0 != VSIFSeekL( psSHP->fpSHP, msSHXReadOffset( psSHP, hEntity), 0 )) {
    msSetError(MS_IOERR, "failed to seek offset", "msSHPReadPoint()");
    return(MS_FAILURE);
  }
  if( 1 != VSIFReadL( psSHP->pabyRec, nEntitySize, 1, psSHP->fpSHP )) {
    msSetError(MS_IOERR, "failed to fread record", "msSHPReadPoint()");
    return(MS_FAILURE);
  }


  memcpy( &(point->x), psSHP->pabyRec + 12, 8 );
  memcpy( &(point->y), psSHP->pabyRec + 20, 8 );

  if( bBigEndian ) {
    SwapWord( 8, &(point->x));
    SwapWord( 8, &(point->y));
  }

  return(MS_SUCCESS);
}

/*
** msSHXLoadPage()
**
** The SHX tells us what the byte offsets of the shapes in the SHP file are.
** We read the SHX file in ~8K pages and store those pages in memory for
** successive accesses during the reading cycle (first bounds are read,
** then entire shapes). Each time we read a page, we mark it as read.
*/
int msSHXLoadPage( SHPHandle psSHP, int shxBufferPage )
{
  int i;

  /* Each SHX record is 8 bytes long (two ints), hence our buffer size. */
  char buffer[SHX_BUFFER_PAGE * 8];

  /*  Validate the page number. */
  if( shxBufferPage < 0  )
    return(MS_FAILURE);

  if( 0 != VSIFSeekL( psSHP->fpSHX, 100 + shxBufferPage * SHX_BUFFER_PAGE * 8, 0 )) {
    /*
     * msSetError(MS_IOERR, "failed to seek offset", "msSHXLoadPage()");
     * return(MS_FAILURE);
    */
  }
  if( SHX_BUFFER_PAGE != VSIFReadL( buffer, 8, SHX_BUFFER_PAGE, psSHP->fpSHX )) {
    /*
     * msSetError(MS_IOERR, "failed to fread SHX record", "msSHXLoadPage()");
     * return(MS_FAILURE);
     */
  }

  /* Copy the buffer contents out into the working arrays. */
  for( i = 0; i < SHX_BUFFER_PAGE; i++ ) {
    int tmpOffset, tmpSize;

    /* Don't write information past the end of the arrays, please. */
    if(psSHP->nRecords <= (shxBufferPage * SHX_BUFFER_PAGE + i) )
      break;

    memcpy( &tmpOffset, (buffer + (8*i)), 4);
    memcpy( &tmpSize, (buffer + (8*i) + 4), 4);

    /* SHX uses big endian numbers for the offsets, so we have to flip them */
    /* if we are a little endian machine. */
    if( !bBigEndian ) {
      tmpOffset = SWAP_FOUR_BYTES(tmpOffset);
      tmpSize = SWAP_FOUR_BYTES(tmpSize);
    }

    /* SHX stores the offsets in 2 byte units, so we double them to get */
    /* an offset in bytes. */
    tmpOffset = tmpOffset * 2;
    tmpSize = tmpSize * 2;

    /* Write the answer into the working arrays on the SHPHandle */
    psSHP->panRecOffset[shxBufferPage * SHX_BUFFER_PAGE + i] = tmpOffset;
    psSHP->panRecSize[shxBufferPage * SHX_BUFFER_PAGE + i] = tmpSize;
  }

  msSetBit(psSHP->panRecLoaded, shxBufferPage, 1);

  return(MS_SUCCESS);
}

int msSHXLoadAll( SHPHandle psSHP )
{

  int i;
  uchar *pabyBuf;

  pabyBuf = (uchar *) msSmallMalloc(8 * psSHP->nRecords );
  if((size_t)psSHP->nRecords != VSIFReadL( pabyBuf, 8, psSHP->nRecords, psSHP->fpSHX )) {
    msSetError(MS_IOERR, "failed to read shx records", "msSHXLoadAll()");
    free(pabyBuf);
    return MS_FAILURE;
  }
  for( i = 0; i < psSHP->nRecords; i++ ) {
    ms_int32 nOffset, nLength;

    memcpy( &nOffset, pabyBuf + i * 8, 4 );
    memcpy( &nLength, pabyBuf + i * 8 + 4, 4 );

    if( !bBigEndian ) {
      nOffset = SWAP_FOUR_BYTES( nOffset );
      nLength = SWAP_FOUR_BYTES( nLength );
    }

    psSHP->panRecOffset[i] = nOffset*2;
    psSHP->panRecSize[i] = nLength*2;
  }
  free(pabyBuf);
  psSHP->panRecAllLoaded = 1;

  return(MS_SUCCESS);

}

int msSHXReadOffset( SHPHandle psSHP, int hEntity )
{

  int shxBufferPage = hEntity / SHX_BUFFER_PAGE;

  /*  Validate the record/entity number. */
  if( hEntity < 0 || hEntity >= psSHP->nRecords )
    return(MS_FAILURE);

  if( ! (psSHP->panRecAllLoaded || msGetBit(psSHP->panRecLoaded, shxBufferPage)) ) {
    msSHXLoadPage( psSHP, shxBufferPage );
  }

  return psSHP->panRecOffset[hEntity];

}

int msSHXReadSize( SHPHandle psSHP, int hEntity )
{

  int shxBufferPage = hEntity / SHX_BUFFER_PAGE;

  /*  Validate the record/entity number. */
  if( hEntity < 0 || hEntity >= psSHP->nRecords )
    return(MS_FAILURE);

  if( ! (psSHP->panRecAllLoaded || msGetBit(psSHP->panRecLoaded, shxBufferPage)) ) {
    msSHXLoadPage( psSHP, shxBufferPage );
  }

  return psSHP->panRecSize[hEntity];

}

/*
** msSHPReadShape() - Reads the vertices for one shape from a shape file.
*/
void msSHPReadShape( SHPHandle psSHP, int hEntity, shapeObj *shape )
{
  int i, j, k;
  int nEntitySize, nRequiredSize;

  msInitShape(shape); /* initialize the shape */

  /* -------------------------------------------------------------------- */
  /*      Validate the record/entity number.                              */
  /* -------------------------------------------------------------------- */
  if( hEntity < 0 || hEntity >= psSHP->nRecords )
    return;

  if( msSHXReadSize(psSHP, hEntity) == 4 ) {
    shape->type = MS_SHAPE_NULL;
    return;
  }

  nEntitySize = msSHXReadSize(psSHP, hEntity) + 8;
  if (msSHPReadAllocateBuffer(psSHP, hEntity, "msSHPReadShape()") == MS_FAILURE) {
    shape->type = MS_SHAPE_NULL;
    return;
  }

  /* -------------------------------------------------------------------- */
  /*      Read the record.                                                */
  /* -------------------------------------------------------------------- */
  if( 0 != VSIFSeekL( psSHP->fpSHP, msSHXReadOffset( psSHP, hEntity), 0 )) {
    msSetError(MS_IOERR, "failed to seek offset", "msSHPReadShape()");
    shape->type = MS_SHAPE_NULL;
    return;
  }
  if( 1 != VSIFReadL( psSHP->pabyRec, nEntitySize, 1, psSHP->fpSHP )) {
    msSetError(MS_IOERR, "failed to fread record", "msSHPReadPoint()");
    shape->type = MS_SHAPE_NULL;
    return;
  }

  /* -------------------------------------------------------------------- */
  /*  Extract vertices for a Polygon or Arc.            */
  /* -------------------------------------------------------------------- */
  if( psSHP->nShapeType == SHP_POLYGON || psSHP->nShapeType == SHP_ARC ||
      psSHP->nShapeType == SHP_POLYGONM || psSHP->nShapeType == SHP_ARCM ||
      psSHP->nShapeType == SHP_POLYGONZ || psSHP->nShapeType == SHP_ARCZ) {
    ms_int32  nPoints, nParts;

    if (nEntitySize < 40 + 8 + 4) {
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_SHPERR, "Corrupted feature encountered.  hEntity = %d, nEntitySize=%d", "msSHPReadShape()",
                 hEntity, nEntitySize);
      return;
    }

    /* copy the bounding box */
    memcpy( &shape->bounds.minx, psSHP->pabyRec + 8 + 4, 8 );
    memcpy( &shape->bounds.miny, psSHP->pabyRec + 8 + 12, 8 );
    memcpy( &shape->bounds.maxx, psSHP->pabyRec + 8 + 20, 8 );
    memcpy( &shape->bounds.maxy, psSHP->pabyRec + 8 + 28, 8 );

    if( bBigEndian ) {
      SwapWord( 8, &shape->bounds.minx);
      SwapWord( 8, &shape->bounds.miny);
      SwapWord( 8, &shape->bounds.maxx);
      SwapWord( 8, &shape->bounds.maxy);
    }

    memcpy( &nPoints, psSHP->pabyRec + 40 + 8, 4 );
    memcpy( &nParts, psSHP->pabyRec + 36 + 8, 4 );

    if( bBigEndian ) {
      nPoints = SWAP_FOUR_BYTES(nPoints);
      nParts = SWAP_FOUR_BYTES(nParts);
    }

    if (nPoints < 0 || nParts < 0 ||
        nPoints > 50 * 1000 * 1000 || nParts > 10 * 1000 * 1000) {
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_SHPERR, "Corrupted feature encountered.  hEntity = %d, nPoints =%d, nParts = %d", "msSHPReadShape()",
                 hEntity, nPoints, nParts);
      return;
    }

    /* -------------------------------------------------------------------- */
    /*      Copy out the part array from the record.                        */
    /* -------------------------------------------------------------------- */
    if( psSHP->nPartMax < nParts ) {
      psSHP->panParts = (int *) SfRealloc(psSHP->panParts, nParts * sizeof(int) );
      if (psSHP->panParts == NULL) {
        /* Reallocate previous successfull size for following features */
        psSHP->panParts = (int *) msSmallMalloc(psSHP->nPartMax * sizeof(int) );

        shape->type = MS_SHAPE_NULL;
        msSetError(MS_MEMERR, "Out of memory. Cannot allocate %d bytes. Probably broken shapefile at feature %d",
                   "msSHPReadShape()", (int)(nParts * sizeof(int)), hEntity);
        return;
      }
      psSHP->nPartMax = nParts;
    }
    if (psSHP->panParts == NULL) {
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_MEMERR, "Out of memory", "msSHPReadShape()");
      return;
    }

    /* With the previous checks on nPoints and nParts, */
    /* we should not overflow here and after */
    /* since 50 M * (16 + 8 + 8) = 1 600 MB */
    if (44 + 8 + 4 * nParts + 16 * nPoints > nEntitySize) {
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_SHPERR, "Corrupted .shp file : shape %d, nPoints=%d, nParts=%d.",
                 "msSHPReadShape()", hEntity, nPoints, nParts);
      return;
    }

    memcpy( psSHP->panParts, psSHP->pabyRec + 44 + 8, 4 * nParts );
    if( bBigEndian ) {
      for( i = 0; i < nParts; i++ ) {
        *(psSHP->panParts+i) = SWAP_FOUR_BYTES(*(psSHP->panParts+i));
      }
    }

    /* -------------------------------------------------------------------- */
    /*      Fill the shape structure.                                       */
    /* -------------------------------------------------------------------- */
    shape->line = (lineObj *)malloc(sizeof(lineObj)*nParts);
    MS_CHECK_ALLOC_NO_RET(shape->line, sizeof(lineObj)*nParts);

    shape->numlines = nParts;

    k = 0; /* overall point counter */
    for( i = 0; i < nParts; i++) {
      const ms_int32 end = i == nParts - 1
        ? nPoints
        : psSHP->panParts[i+1];
      shape->line[i].numpoints = end - psSHP->panParts[i];
      if (psSHP->panParts[i] < 0 || end < 0 || end > nPoints ||
          psSHP->panParts[i] >= end) {
        msSetError(MS_SHPERR, "Corrupted .shp file : shape %d, shape->line[%d].numpoints=%d", "msSHPReadShape()",
                   hEntity, i, shape->line[i].numpoints);
        while(--i >= 0)
          free(shape->line[i].point);
        free(shape->line);
        shape->line = NULL;
        shape->numlines = 0;
        shape->type = MS_SHAPE_NULL;
        return;
      }

      if( (shape->line[i].point = (pointObj *)malloc(sizeof(pointObj)*shape->line[i].numpoints)) == NULL ) {
        while(--i >= 0)
          free(shape->line[i].point);
        free(shape->line);
        shape->numlines = 0;
        shape->type = MS_SHAPE_NULL;
        msSetError(MS_MEMERR, "Out of memory", "msSHPReadShape()");
        return;
      }

      /* nOffset = 44 + 8 + 4*nParts; */
      for( j = 0; j < shape->line[i].numpoints; j++ ) {
        memcpy(&(shape->line[i].point[j].x), psSHP->pabyRec + 44 + 4*nParts + 8 + k * 16, 8 );
        memcpy(&(shape->line[i].point[j].y), psSHP->pabyRec + 44 + 4*nParts + 8 + k * 16 + 8, 8 );

        if( bBigEndian ) {
          SwapWord( 8, &(shape->line[i].point[j].x) );
          SwapWord( 8, &(shape->line[i].point[j].y) );
        }

        /* -------------------------------------------------------------------- */
        /*      Polygon, Arc with Z values.                                     */
        /* -------------------------------------------------------------------- */
        shape->line[i].point[j].z = 0.0; /* initialize */
        if (psSHP->nShapeType == SHP_POLYGONZ || psSHP->nShapeType == SHP_ARCZ) {
          const int nOffset = 44 + 8 + (4*nParts) + (16*nPoints) ;
          if( nEntitySize >= nOffset + 16 + 8*nPoints ) {
            memcpy(&(shape->line[i].point[j].z), psSHP->pabyRec + nOffset + 16 + k*8, 8 );
            if( bBigEndian ) SwapWord( 8, &(shape->line[i].point[j].z) );
          }
        }

        /* -------------------------------------------------------------------- */
        /*      Measured arc and polygon support.                               */
        /* -------------------------------------------------------------------- */
        shape->line[i].point[j].m = 0; /* initialize */
        if (psSHP->nShapeType == SHP_POLYGONM || psSHP->nShapeType == SHP_ARCM) {
          const int nOffset = 44 + 8 + (4*nParts) + (16*nPoints) ;
          if( nEntitySize >= nOffset + 16 + 8*nPoints ) {
            memcpy(&(shape->line[i].point[j].m), psSHP->pabyRec + nOffset + 16 + k*8, 8 );
            if( bBigEndian ) SwapWord( 8, &(shape->line[i].point[j].m) );
          }
        }
        k++;
      }
    }

    if(psSHP->nShapeType == SHP_POLYGON
        || psSHP->nShapeType == SHP_POLYGONZ
        || psSHP->nShapeType == SHP_POLYGONM)
      shape->type = MS_SHAPE_POLYGON;
    else
      shape->type = MS_SHAPE_LINE;

  }

  /* -------------------------------------------------------------------- */
  /*  Extract a MultiPoint.                                               */
  /* -------------------------------------------------------------------- */
  else if( psSHP->nShapeType == SHP_MULTIPOINT || psSHP->nShapeType == SHP_MULTIPOINTM ||
           psSHP->nShapeType == SHP_MULTIPOINTZ) {
    ms_int32 nPoints;

    if (nEntitySize < 44 + 4) {
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_SHPERR, "Corrupted feature encountered.  recSize of feature %d=%d", "msSHPReadShape()",
                 hEntity, msSHXReadSize(psSHP, hEntity));
      return;
    }

    /* copy the bounding box */
    memcpy( &shape->bounds.minx, psSHP->pabyRec + 8 + 4, 8 );
    memcpy( &shape->bounds.miny, psSHP->pabyRec + 8 + 12, 8 );
    memcpy( &shape->bounds.maxx, psSHP->pabyRec + 8 + 20, 8 );
    memcpy( &shape->bounds.maxy, psSHP->pabyRec + 8 + 28, 8 );

    if( bBigEndian ) {
      SwapWord( 8, &shape->bounds.minx);
      SwapWord( 8, &shape->bounds.miny);
      SwapWord( 8, &shape->bounds.maxx);
      SwapWord( 8, &shape->bounds.maxy);
    }

    memcpy( &nPoints, psSHP->pabyRec + 44, 4 );
    if( bBigEndian ) nPoints = SWAP_FOUR_BYTES(nPoints);

    /* -------------------------------------------------------------------- */
    /*      Fill the shape structure.                                       */
    /* -------------------------------------------------------------------- */
    if( (shape->line = (lineObj *)malloc(sizeof(lineObj))) == NULL ) {
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_MEMERR, "Out of memory", "msSHPReadShape()");
      return;
    }

    if (nPoints < 0 || nPoints > 50 * 1000 * 1000) {
      free(shape->line);
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_SHPERR, "Corrupted .shp file : shape %d, nPoints=%d.",
                 "msSHPReadShape()", hEntity, nPoints);
      return;
    }

    nRequiredSize = 48 + nPoints * 16;
    if (psSHP->nShapeType == SHP_MULTIPOINTZ || psSHP->nShapeType == SHP_MULTIPOINTM)
      nRequiredSize += 16 + nPoints * 8;
    if (nRequiredSize > nEntitySize) {
      free(shape->line);
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_SHPERR, "Corrupted .shp file : shape %d : nPoints = %d, nEntitySize = %d",
                 "msSHPReadShape()", hEntity, nPoints, nEntitySize);
      return;
    }

    shape->numlines = 1;
    shape->line[0].numpoints = nPoints;
    shape->line[0].point = (pointObj *) malloc( nPoints * sizeof(pointObj) );
    if (shape->line[0].point == NULL) {
      free(shape->line);
      shape->numlines = 0;
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_MEMERR, "Out of memory", "msSHPReadShape()");
      return;
    }

    for( i = 0; i < nPoints; i++ ) {
      memcpy(&(shape->line[0].point[i].x), psSHP->pabyRec + 48 + 16 * i, 8 );
      memcpy(&(shape->line[0].point[i].y), psSHP->pabyRec + 48 + 16 * i + 8, 8 );

      if( bBigEndian ) {
        SwapWord( 8, &(shape->line[0].point[i].x) );
        SwapWord( 8, &(shape->line[0].point[i].y) );
      }

      /* -------------------------------------------------------------------- */
      /*      MulipointZ                                                      */
      /* -------------------------------------------------------------------- */
      shape->line[0].point[i].z = 0; /* initialize */
      if (psSHP->nShapeType == SHP_MULTIPOINTZ) {
        const int nOffset = 48 + 16*nPoints;
        memcpy(&(shape->line[0].point[i].z), psSHP->pabyRec + nOffset + 16 + i*8, 8 );
        if( bBigEndian ) SwapWord( 8, &(shape->line[0].point[i].z));
      }

      /* -------------------------------------------------------------------- */
      /*      Measured shape : multipont.                                     */
      /* -------------------------------------------------------------------- */
      shape->line[0].point[i].m = 0; /* initialize */
      if (psSHP->nShapeType == SHP_MULTIPOINTM) {
        const int nOffset = 48 + 16*nPoints;
        memcpy(&(shape->line[0].point[i].m), psSHP->pabyRec + nOffset + 16 + i*8, 8 );
        if( bBigEndian ) SwapWord( 8, &(shape->line[0].point[i].m));
      }
    }

    shape->type = MS_SHAPE_POINT;
  }

  /* -------------------------------------------------------------------- */
  /*  Extract a Point.                            */
  /* -------------------------------------------------------------------- */
  else if(psSHP->nShapeType == SHP_POINT ||  psSHP->nShapeType == SHP_POINTM ||
          psSHP->nShapeType == SHP_POINTZ) {

    if (nEntitySize < 20 + 8) {
      shape->type = MS_SHAPE_NULL;
      msSetError(MS_SHPERR, "Corrupted feature encountered.  recSize of feature %d=%d", "msSHPReadShape()",
                 hEntity, msSHXReadSize(psSHP, hEntity));
      return;
    }

    /* -------------------------------------------------------------------- */
    /*      Fill the shape structure.                                       */
    /* -------------------------------------------------------------------- */
    shape->line = (lineObj *)malloc(sizeof(lineObj));
    MS_CHECK_ALLOC_NO_RET(shape->line, sizeof(lineObj));

    shape->numlines = 1;
    shape->line[0].numpoints = 1;
    shape->line[0].point = (pointObj *) msSmallMalloc(sizeof(pointObj));

    memcpy( &(shape->line[0].point[0].x), psSHP->pabyRec + 12, 8 );
    memcpy( &(shape->line[0].point[0].y), psSHP->pabyRec + 20, 8 );

    if( bBigEndian ) {
      SwapWord( 8, &(shape->line[0].point[0].x));
      SwapWord( 8, &(shape->line[0].point[0].y));
    }

    /* -------------------------------------------------------------------- */
    /*      PointZ                                                          */
    /* -------------------------------------------------------------------- */
    shape->line[0].point[0].z = 0; /* initialize */
    if (psSHP->nShapeType == SHP_POINTZ) {
      const int nOffset = 20 + 8;
      if( nEntitySize >= nOffset + 8 ) {
        memcpy(&(shape->line[0].point[0].z), psSHP->pabyRec + nOffset, 8 );
        if( bBigEndian ) SwapWord( 8, &(shape->line[0].point[0].z));
      }
    }

    /* -------------------------------------------------------------------- */
    /*      Measured support : point.                                       */
    /* -------------------------------------------------------------------- */
    shape->line[0].point[0].m = 0; /* initialize */
    if (psSHP->nShapeType == SHP_POINTM) {
      const int nOffset = 20 + 8;
      if( nEntitySize >= nOffset + 8 ) {
        memcpy(&(shape->line[0].point[0].m), psSHP->pabyRec + nOffset, 8 );
        if( bBigEndian ) SwapWord( 8, &(shape->line[0].point[0].m));
      }
    }

    /* set the bounding box to the point */
    shape->bounds.minx = shape->bounds.maxx = shape->line[0].point[0].x;
    shape->bounds.miny = shape->bounds.maxy = shape->line[0].point[0].y;

    shape->type = MS_SHAPE_POINT;
  }

  shape->index = hEntity;

  return;
}

int msSHPReadBounds( SHPHandle psSHP, int hEntity, rectObj *padBounds)
{
  /* -------------------------------------------------------------------- */
  /*      Validate the record/entity number.                              */
  /* -------------------------------------------------------------------- */
  if( psSHP->nRecords <= 0 || hEntity < -1 || hEntity >= psSHP->nRecords ) {
    padBounds->minx = padBounds->miny = padBounds->maxx = padBounds->maxy = 0.0;
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*  If the entity is -1 we fetch the bounds for the whole file.   */
  /* -------------------------------------------------------------------- */
  if( hEntity == -1 ) {
    padBounds->minx = psSHP->adBoundsMin[0];
    padBounds->miny = psSHP->adBoundsMin[1];
    padBounds->maxx = psSHP->adBoundsMax[0];
    padBounds->maxy = psSHP->adBoundsMax[1];
  } else {

    if( msSHXReadSize(psSHP, hEntity) == 4 ) { /* NULL shape */
      padBounds->minx = padBounds->miny = padBounds->maxx = padBounds->maxy = 0.0;
      return MS_FAILURE;
    }

    if( psSHP->nShapeType != SHP_POINT && psSHP->nShapeType != SHP_POINTZ && psSHP->nShapeType != SHP_POINTM) {
      if( 0 != VSIFSeekL( psSHP->fpSHP, msSHXReadOffset( psSHP, hEntity) + 12, 0 )) {
        msSetError(MS_IOERR, "failed to seek offset", "msSHPReadBounds()");
        return(MS_FAILURE);
      }
      if( 1 != VSIFReadL( padBounds, sizeof(double)*4, 1, psSHP->fpSHP )) {
        msSetError(MS_IOERR, "failed to fread record", "msSHPReadBounds()");
        return(MS_FAILURE);
      }

      if( bBigEndian ) {
        SwapWord( 8, &(padBounds->minx) );
        SwapWord( 8, &(padBounds->miny) );
        SwapWord( 8, &(padBounds->maxx) );
        SwapWord( 8, &(padBounds->maxy) );
      }

      if(msIsNan(padBounds->minx)) { /* empty shape */
        padBounds->minx = padBounds->miny = padBounds->maxx = padBounds->maxy = 0.0;
        return MS_FAILURE;
      }
    } else {
      /* -------------------------------------------------------------------- */
      /*      For points we fetch the point, and duplicate it as the          */
      /*      minimum and maximum bound.                                      */
      /* -------------------------------------------------------------------- */

      if( 0 != VSIFSeekL( psSHP->fpSHP, msSHXReadOffset( psSHP, hEntity) + 12, 0 )) {
        msSetError(MS_IOERR, "failed to seek offset", "msSHPReadBounds()");
        return(MS_FAILURE);
      }
      if( 1 != VSIFReadL( padBounds, sizeof(double)*2, 1, psSHP->fpSHP )) {
        msSetError(MS_IOERR, "failed to fread record", "msSHPReadBounds()");
        return(MS_FAILURE);
      }

      if( bBigEndian ) {
        SwapWord( 8, &(padBounds->minx) );
        SwapWord( 8, &(padBounds->miny) );
      }

      padBounds->maxx = padBounds->minx;
      padBounds->maxy = padBounds->miny;
    }
  }

  return MS_SUCCESS;
}

int msShapefileOpen(shapefileObj *shpfile, const char *mode, const char *filename, int log_failures)
{
  int i;
  char *dbfFilename;
  size_t bufferSize = 0;

  if(!filename) {
    if( log_failures )
      msSetError(MS_IOERR, "No (NULL) filename provided.", "msShapefileOpen()");
    return(-1);
  }

  /* initialize a few things */
  shpfile->status = NULL;
  shpfile->lastshape = -1;
  shpfile->isopen = MS_FALSE;

  /* open the shapefile file (appending ok) and get basic info */
  if(!mode)
    shpfile->hSHP = msSHPOpen( filename, "rb");
  else
    shpfile->hSHP = msSHPOpen( filename, mode);

  if(!shpfile->hSHP) {
    if( log_failures )
      msSetError(MS_IOERR, "(%s)", "msShapefileOpen()", filename);
    return(-1);
  }

  strlcpy(shpfile->source, filename, sizeof(shpfile->source));

  /* load some information about this shapefile */
  msSHPGetInfo( shpfile->hSHP, &shpfile->numshapes, &shpfile->type);

  if( shpfile->numshapes < 0 || shpfile->numshapes > 256000000 ) {
    msSetError(MS_SHPERR, "Corrupted .shp file : numshapes = %d.",
               "msShapefileOpen()", shpfile->numshapes);
    msSHPClose(shpfile->hSHP);
    return -1;
  }

  msSHPReadBounds( shpfile->hSHP, -1, &(shpfile->bounds));

  bufferSize = strlen(filename)+5;
  dbfFilename = (char *)msSmallMalloc(bufferSize);
  strcpy(dbfFilename, filename);

  /* clean off any extention the filename might have */
  for (i = strlen(dbfFilename) - 1;
       i > 0 && dbfFilename[i] != '.' && dbfFilename[i] != '/' && dbfFilename[i] != '\\';
       i-- ) {}

  if( dbfFilename[i] == '.' )
    dbfFilename[i] = '\0';

  strlcat(dbfFilename, ".dbf", bufferSize);

  shpfile->hDBF = msDBFOpen(dbfFilename, "rb");

  if(!shpfile->hDBF) {
    if( log_failures )
      msSetError(MS_IOERR, "(%s)", "msShapefileOpen()", dbfFilename);
    free(dbfFilename);
    msSHPClose(shpfile->hSHP);
    return(-1);
  }
  free(dbfFilename);

  shpfile->isopen = MS_TRUE;
  return(0); /* all o.k. */
}

/* Creates a new shapefile */
int msShapefileCreate(shapefileObj *shpfile, char *filename, int type)
{
  if(type != SHP_POINT && type != SHP_MULTIPOINT && type != SHP_ARC &&
      type != SHP_POLYGON &&
      type != SHP_POINTM && type != SHP_MULTIPOINTM &&
      type != SHP_ARCM && type != SHP_POLYGONM &&
      type != SHP_POINTZ && type != SHP_MULTIPOINTZ &&
      type != SHP_ARCZ && type != SHP_POLYGONZ) {
    msSetError(MS_SHPERR, "Invalid shape type.", "msNewSHPFile()");
    return(-1);
  }

  /* create the spatial portion */
  shpfile->hSHP = msSHPCreate(filename, type);
  if(!shpfile->hSHP) {
    msSetError(MS_IOERR, "(%s)", "msNewSHPFile()",filename);
    return(-1);
  }

  /* retrieve a few things about this shapefile */
  msSHPGetInfo( shpfile->hSHP, &shpfile->numshapes, &shpfile->type);
  msSHPReadBounds( shpfile->hSHP, -1, &(shpfile->bounds));

  /* initialize a few other things */
  shpfile->status = NULL;
  shpfile->lastshape = -1;
  shpfile->isopen = MS_TRUE;

  shpfile->hDBF = NULL; /* XBase file is NOT created here... */
  return(0);
}

void msShapefileClose(shapefileObj *shpfile)
{
  if (shpfile && shpfile->isopen == MS_TRUE) { /* Silently return if called with NULL shpfile by freeLayer() */
    if(shpfile->hSHP) msSHPClose(shpfile->hSHP);
    if(shpfile->hDBF) msDBFClose(shpfile->hDBF);
    free(shpfile->status);
    shpfile->isopen = MS_FALSE;
  }
}

/* status array lives in the shpfile, can return MS_SUCCESS/MS_FAILURE/MS_DONE */
int msShapefileWhichShapes(shapefileObj *shpfile, rectObj rect, int debug)
{
  int i;
  rectObj shaperect;
  char *filename;

  free(shpfile->status);
  shpfile->status = NULL;

  /* rect and shapefile DON'T overlap... */
  if(msRectOverlap(&shpfile->bounds, &rect) != MS_TRUE)
    return(MS_DONE);

  if(msRectContained(&shpfile->bounds, &rect) == MS_TRUE) {
    shpfile->status = msAllocBitArray(shpfile->numshapes);
    if(!shpfile->status) {
      msSetError(MS_MEMERR, NULL, "msShapefileWhichShapes()");
      return(MS_FAILURE);
    }
    msSetAllBits(shpfile->status, shpfile->numshapes, 1);
  } else {

    /* deal with case where sourcename is of the form 'file.shp' */
    char* sourcename = msStrdup(shpfile->source); /* shape file source string from map file */
    char* s = strstr(sourcename, ".shp");
    if( s )
      *s = '\0';
    else {
      s = strstr(sourcename, ".SHP");
      if( s )
        *s = '\0';
    }

    filename = (char *)malloc(strlen(sourcename)+strlen(MS_INDEX_EXTENSION)+1);
    MS_CHECK_ALLOC(filename, strlen(sourcename)+strlen(MS_INDEX_EXTENSION)+1, MS_FAILURE);

    sprintf(filename, "%s%s", sourcename, MS_INDEX_EXTENSION);

    shpfile->status = msSearchDiskTree(filename, rect, debug, shpfile->numshapes);
    free(filename);
    free(sourcename);

    if(shpfile->status) { /* index  */
      msFilterTreeSearch(shpfile, shpfile->status, rect);
    } else { /* no index  */
      shpfile->status = msAllocBitArray(shpfile->numshapes);
      if(!shpfile->status) {
        msSetError(MS_MEMERR, NULL, "msShapefileWhichShapes()");
        return(MS_FAILURE);
      }

      for(i=0; i<shpfile->numshapes; i++) {
        if(msSHPReadBounds(shpfile->hSHP, i, &shaperect) == MS_SUCCESS)
          if(msRectOverlap(&shaperect, &rect) == MS_TRUE) msSetBit(shpfile->status, i, 1);
      }
    }
  }

  shpfile->lastshape = -1;

  return(MS_SUCCESS); /* success */
}

/* Return the absolute path to the given layer's tileindex file's directory */
void msTileIndexAbsoluteDir(char *tiFileAbsDir, layerObj *layer)
{
  char tiFileAbsPath[MS_MAXPATHLEN];
  char *tiFileAbsDirTmp=NULL;

  msBuildPath(tiFileAbsPath, layer->map->mappath, layer->tileindex); /* absolute path to tileindex file */
  tiFileAbsDirTmp = msGetPath(tiFileAbsPath); /* tileindex file's directory */
  strlcpy(tiFileAbsDir, tiFileAbsDirTmp, MS_MAXPATHLEN);
  free(tiFileAbsDirTmp);
}

/*
** Build possible paths we might find the tile file at:
**   map dir + shape path + filename?
**   tile dir + shape path + filename?
**   map dir + filename?
**
** Returns
** MS_SUCCESS - found a file
** MS_FAILURE - no file, and map is configured to fail on missing
** MS_DONE - no file, and map is configured to continue on missing
*/
static
int msTiledSHPTryOpen(shapefileObj *shpfile, layerObj *layer, char *tiFileAbsDir, const char *filename)
{
  char szPath[MS_MAXPATHLEN];
  int ignore_missing = msMapIgnoreMissingData(layer->map);
  int log_failures = MS_TRUE;

  if( ignore_missing == MS_MISSING_DATA_IGNORE )
    log_failures = MS_FALSE;

  if(msShapefileOpen(shpfile, "rb", msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, filename), log_failures) == -1) {
    if(msShapefileOpen(shpfile, "rb", msBuildPath3(szPath, tiFileAbsDir, layer->map->shapepath, filename), log_failures) == -1) {
      if(msShapefileOpen(shpfile, "rb", msBuildPath(szPath, layer->map->mappath, filename), log_failures) == -1) {
        if(ignore_missing == MS_MISSING_DATA_FAIL) {
          msSetError(MS_IOERR, "Unable to open shapefile '%s' for layer '%s' ... fatal error.", "msTiledSHPTryOpen()", filename, layer->name);
          return(MS_FAILURE);
        } else if( ignore_missing == MS_MISSING_DATA_LOG ) {
          if( layer->debug || layer->map->debug ) {
            msDebug( "Unable to open shapefile '%s' for layer '%s' ... ignoring this missing data.\n", szPath, layer->name );
          }
          return(MS_DONE);
        } else if( ignore_missing == MS_MISSING_DATA_IGNORE ) {
          return(MS_DONE);
        } else {
          /* never get here */
          msSetError(MS_IOERR, "msIgnoreMissingData returned unexpected value.", "msTiledSHPTryOpen()");
          return(MS_FAILURE);
        }
      }
    }
  }
  return(MS_SUCCESS);
}

static const char* msTiledSHPLoadEntry(layerObj *layer, int i, char* tilename, size_t tilenamesize)
{
    const char* filename;
    msTiledSHPLayerInfo *tSHP= layer->layerinfo;

    msProjectDestroyReprojector(tSHP->reprojectorFromTileProjToLayerProj);
    tSHP->reprojectorFromTileProjToLayerProj = NULL;

    msFreeProjection(&(tSHP->sTileProj));
    if( layer->tilesrs != NULL )
    {
        int idx = msDBFGetItemIndex(tSHP->tileshpfile->hDBF, layer->tilesrs);
        const char* pszWKT = msDBFReadStringAttribute(tSHP->tileshpfile->hDBF, i, idx);
        IGNORE_RET_VAL(msOGCWKT2ProjectionObj(pszWKT, &(tSHP->sTileProj), layer->debug ));
    }

    if(!layer->data) /* assume whole filename is in attribute field */
      filename =msDBFReadStringAttribute(tSHP->tileshpfile->hDBF, i, layer->tileitemindex);
    else {
      snprintf(tilename, tilenamesize, "%s/%s",
               msDBFReadStringAttribute(tSHP->tileshpfile->hDBF, i, layer->tileitemindex) ,
               layer->data);
      filename = tilename;
    }

    return filename;
}

static
int msTiledSHPOpenFile(layerObj *layer)
{
  int i;
  const char *filename;
  char tilename[MS_MAXPATHLEN], szPath[MS_MAXPATHLEN];
  char tiFileAbsDir[MS_MAXPATHLEN];

  msTiledSHPLayerInfo *tSHP=NULL;

  if (layer->layerinfo != NULL) {
    return MS_SUCCESS;  // Nothing to do... layer is already opened
  }

  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  /* allocate space for a shapefileObj using layer->layerinfo  */
  tSHP = (msTiledSHPLayerInfo *) calloc(1, sizeof(msTiledSHPLayerInfo));
  MS_CHECK_ALLOC(tSHP, sizeof(msTiledSHPLayerInfo), MS_FAILURE);
  msInitProjection(&(tSHP->sTileProj));
  msProjectionInheritContextFrom(&(tSHP->sTileProj), &layer->projection);

  tSHP->shpfile = (shapefileObj *) malloc(sizeof(shapefileObj));
  if (tSHP->shpfile == NULL) {
    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msTiledSHPOpenFile()",
               __FILE__, __LINE__, (unsigned int)sizeof(shapefileObj));
    msFreeProjection(&(tSHP->sTileProj));
    free(tSHP);
    return MS_FAILURE;
  }
  
  tSHP->shpfile->isopen = MS_FALSE; /* in case of error: do not try to close the shpfile */
  tSHP->tileshpfile = NULL; /* may need this if not using a tile layer, look for malloc later */
  layer->layerinfo = tSHP;

  tSHP->tilelayerindex = msGetLayerIndex(layer->map, layer->tileindex);
  if(tSHP->tilelayerindex != -1) { /* does the tileindex reference another layer */
    int status;
    layerObj *tlp;

    tlp = (GET_LAYER(layer->map, tSHP->tilelayerindex));

    if(tlp->connectiontype != MS_SHAPEFILE) {
      msSetError(MS_SHPERR, "Tileindex layer must be a shapefile.", "msTiledSHPOpenFile()");
      return(MS_FAILURE);
    }

    status = msLayerOpen(tlp);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* build item list */
    status = msLayerWhichItems(tlp, MS_FALSE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    tSHP->tileshpfile = (shapefileObj *) tlp->layerinfo; /* shapefiles use layerinfo to point to a shapefileObj */

  } else { /* or reference a shapefile directly */

    /* we need tSHP->tileshpfile if we're not working with a layer */
    tSHP->tileshpfile = (shapefileObj *) malloc(sizeof(shapefileObj));
    if (tSHP->tileshpfile == NULL) {
      msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msTiledSHPOpenFile()",
                 __FILE__, __LINE__, (unsigned int)sizeof(shapefileObj));
      free(tSHP->shpfile);
      msFreeProjection(&(tSHP->sTileProj));
      free(tSHP);
      layer->layerinfo = NULL;
      return MS_FAILURE;
    }


    if(msShapefileOpen(tSHP->tileshpfile, "rb", msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, layer->tileindex), MS_TRUE) == -1)
      if(msShapefileOpen(tSHP->tileshpfile, "rb", msBuildPath(szPath, layer->map->mappath, layer->tileindex), MS_TRUE) == -1)
        return(MS_FAILURE);
  }

  if((layer->tileitemindex = msDBFGetItemIndex(tSHP->tileshpfile->hDBF, layer->tileitem)) == -1) return(MS_FAILURE);

  if( layer->tilesrs != NULL &&
      msDBFGetItemIndex(tSHP->tileshpfile->hDBF, layer->tilesrs) < 0 )
  {
    msSetError(MS_SHPERR,
                "Cannot identify TILESRS field.",
                "msTiledSHPOpenFile()");
    return MS_FAILURE;
  }
  if( layer->tilesrs != NULL && layer->projection.numargs == 0 )
  {
    msSetError(MS_SHPERR,
                "A layer with TILESRS set in TILEINDEX `%s' must have a "
                "projection set on itself.",
                "msOGRLayerOpen()",
                layer->tileindex );
    return MS_FAILURE;
  }

  msTileIndexAbsoluteDir(tiFileAbsDir, layer);

  /* position the source at the FIRST tile to use as a template, this is so the functions that fill the iteminfo array have something to work from */
  for(i=0; i<tSHP->tileshpfile->numshapes; i++) {
    int try_open;

    filename = msTiledSHPLoadEntry(layer, i, tilename, sizeof(tilename));
    if(strlen(filename) == 0) continue; /* check again */

    try_open = msTiledSHPTryOpen(tSHP->shpfile, layer, tiFileAbsDir, filename);
    if( try_open == MS_DONE )
      continue;
    else if (try_open == MS_FAILURE )
      return(MS_FAILURE);

    return(MS_SUCCESS); /* found a template, ok to proceed */
  }

  msSetError(MS_SHPERR, "Unable to open a single tile to use as a template in layer %s.", "msTiledSHPOpenFile()", layer->name?layer->name:"(null)");
  return(MS_FAILURE);
}

static
int msTiledSHPWhichShapes(layerObj *layer, rectObj rect, int isQuery)
{
  int i, status;
  const char *filename;
  char tilename[MS_MAXPATHLEN];
  char tiFileAbsDir[MS_MAXPATHLEN];

  msTiledSHPLayerInfo *tSHP=NULL;

  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  tSHP = layer->layerinfo;
  if(!tSHP) {
    msSetError(MS_SHPERR, "Tiled shapefile layer has not been opened.", "msTiledSHPWhichShapes()");
    return(MS_FAILURE);
  }

  msShapefileClose(tSHP->shpfile); /* close previously opened files */

  tSHP->searchrect = rect; /* save the search extent */

  if(tSHP->tilelayerindex != -1) {  /* does the tileindex reference another layer */
    layerObj *tlp;
    shapeObj tshape;

    tlp = (GET_LAYER(layer->map, tSHP->tilelayerindex));
    status= msLayerWhichShapes(tlp, rect, isQuery);
    if(status != MS_SUCCESS) return(status); /* could be MS_DONE or MS_FAILURE */

    msTileIndexAbsoluteDir(tiFileAbsDir, layer);

    msInitShape(&tshape);
    while((status = msLayerNextShape(tlp, &tshape)) == MS_SUCCESS) {
      int try_open;
      rectObj rectTile = rect;

      filename = msTiledSHPLoadEntry(layer, tshape.index, tilename, sizeof(tilename));
      if(strlen(filename) == 0) continue; /* check again */

      try_open = msTiledSHPTryOpen(tSHP->shpfile, layer, tiFileAbsDir, filename);
      if( try_open == MS_DONE )
        continue;
      else if (try_open == MS_FAILURE )
        return(MS_FAILURE);

      if( tSHP->sTileProj.numargs > 0 )
      {
        msProjectRect(&(layer->projection), &(tSHP->sTileProj), &rectTile);
      }

      status = msShapefileWhichShapes(tSHP->shpfile, rectTile, layer->debug);
      if(status == MS_DONE) {
        /* Close and continue to next tile */
        msShapefileClose(tSHP->shpfile);
        continue;
      } else if(status != MS_SUCCESS) {
        msShapefileClose(tSHP->shpfile);
        return(MS_FAILURE);
      }

      /* the layer functions keeps track of this */
      /* tSHP->tileshpfile->lastshape = tshape.index; */
      break;
    }
    return(status); /* if we reach here we either 1) ran out of tiles or 2) had an error reading a tile */

  } else { /* or reference a shapefile directly */
    int try_open;

    status = msShapefileWhichShapes(tSHP->tileshpfile, rect, layer->debug);
    if(status != MS_SUCCESS) return(status); /* could be MS_DONE or MS_FAILURE */

    msTileIndexAbsoluteDir(tiFileAbsDir, layer);

    /* position the source at the FIRST shapefile */
    for(i=0; i<tSHP->tileshpfile->numshapes; i++) {
      rectObj rectTile = rect;
      if(msGetBit(tSHP->tileshpfile->status,i)) {

        filename = msTiledSHPLoadEntry(layer, i, tilename, sizeof(tilename));
        if(strlen(filename) == 0) continue; /* check again */

        try_open = msTiledSHPTryOpen(tSHP->shpfile, layer, tiFileAbsDir, filename);
        if( try_open == MS_DONE )
          continue;
        else if (try_open == MS_FAILURE )
          return(MS_FAILURE);

        if( tSHP->sTileProj.numargs > 0 )
        {
          msProjectRect(&(layer->projection), &(tSHP->sTileProj), &rectTile);
        }

        status = msShapefileWhichShapes(tSHP->shpfile, rectTile, layer->debug);
        if(status == MS_DONE) {
          /* Close and continue to next tile */
          msShapefileClose(tSHP->shpfile);
          continue;
        } else if(status != MS_SUCCESS) {
          msShapefileClose(tSHP->shpfile);
          return(MS_FAILURE);
        }

        tSHP->tileshpfile->lastshape = i;
        break;
      }
    }

    if(i == tSHP->tileshpfile->numshapes)
      return(MS_DONE); /* no more tiles */
    else
      return(MS_SUCCESS);
  }

  return(MS_FAILURE); /* should *never* get here */
}

static
int msTiledSHPNextShape(layerObj *layer, shapeObj *shape)
{
  int i, status, filter_passed = MS_FALSE;
  const char *filename;
  char tilename[MS_MAXPATHLEN];
  char tiFileAbsDir[MS_MAXPATHLEN];

  msTiledSHPLayerInfo *tSHP=NULL;

  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  tSHP = layer->layerinfo;
  if(!tSHP) {
    msSetError(MS_SHPERR, "Tiled shapefile layer has not been opened.", "msTiledSHPNextShape()");
    return(MS_FAILURE);
  }

  msTileIndexAbsoluteDir(tiFileAbsDir, layer);

  do {
    i = tSHP->shpfile->lastshape + 1;
    while(i<tSHP->shpfile->numshapes && !msGetBit(tSHP->shpfile->status,i)) i++; /* next "in" shape */

    if(i == tSHP->shpfile->numshapes) { /* done with this tile, need a new one */
      msShapefileClose(tSHP->shpfile); /* clean up */

      /* position the source to the NEXT shapefile based on the tileindex */
      if(tSHP->tilelayerindex != -1) { /* does the tileindex reference another layer */
        layerObj *tlp;
        shapeObj tshape;
        int try_open;

        tlp = (GET_LAYER(layer->map, tSHP->tilelayerindex));

        msInitShape(&tshape);
        while((status = msLayerNextShape(tlp, &tshape)) == MS_SUCCESS) {
          rectObj rectTile = tSHP->searchrect;

          filename = msTiledSHPLoadEntry(layer, tshape.index, tilename, sizeof(tilename));
          if(strlen(filename) == 0) continue; /* check again */

          try_open = msTiledSHPTryOpen(tSHP->shpfile, layer, tiFileAbsDir, filename);
          if( try_open == MS_DONE )
            continue;
          else if (try_open == MS_FAILURE )
            return(MS_FAILURE);

          if( tSHP->sTileProj.numargs > 0 )
          {
            msProjectRect(&(layer->projection), &(tSHP->sTileProj), &rectTile);
          }

          status = msShapefileWhichShapes(tSHP->shpfile, rectTile, layer->debug);
          if(status == MS_DONE) {
            /* Close and continue to next tile */
            msShapefileClose(tSHP->shpfile);
            continue;
          } else if(status != MS_SUCCESS) {
            msShapefileClose(tSHP->shpfile);
            tSHP->tileshpfile->lastshape = -1;
            return(MS_FAILURE);
          }

          /* the layer functions keeps track of this */
          /* tSHP->tileshpfile->lastshape = tshape.index; */
          break;
        }

        if(status == MS_DONE) {
            tSHP->tileshpfile->lastshape = -1;
            return(MS_DONE); /* no more tiles */
        }
        else {
          msFreeShape(&tshape);
          continue; /* we've got shapes */
        }

      } else { /* or reference a shapefile directly   */

        for(i=(tSHP->tileshpfile->lastshape + 1); i<tSHP->tileshpfile->numshapes; i++) {
          if(msGetBit(tSHP->tileshpfile->status,i)) {
            rectObj rectTile = tSHP->searchrect;
            int try_open;

            filename = msTiledSHPLoadEntry(layer, i, tilename, sizeof(tilename));
            if(strlen(filename) == 0) continue; /* check again */

            try_open = msTiledSHPTryOpen(tSHP->shpfile, layer, tiFileAbsDir, filename);
            if( try_open == MS_DONE )
              continue;
            else if (try_open == MS_FAILURE )
              return(MS_FAILURE);

            if( tSHP->sTileProj.numargs > 0 )
            {
              msProjectRect(&(layer->projection), &(tSHP->sTileProj), &rectTile);
            }

            status = msShapefileWhichShapes(tSHP->shpfile, rectTile, layer->debug);
            if(status == MS_DONE) {
              /* Close and continue to next tile */
              msShapefileClose(tSHP->shpfile);
              continue;
            } else if(status != MS_SUCCESS) {
              msShapefileClose(tSHP->shpfile);
              tSHP->tileshpfile->lastshape = -1;
              return(MS_FAILURE);
            }

            tSHP->tileshpfile->lastshape = i;
            break;
          }
        } /* end for loop */

        if(i == tSHP->tileshpfile->numshapes) {
            tSHP->tileshpfile->lastshape = -1;
            return(MS_DONE); /* no more tiles */
        }
        else continue; /* we've got shapes */
      }
    }

    tSHP->shpfile->lastshape = i;

    msSHPReadShape(tSHP->shpfile->hSHP, i, shape);
    if(shape->type == MS_SHAPE_NULL) {
      msFreeShape(shape);
      continue; /* skip NULL shapes */
    }

    if( tSHP->sTileProj.numargs > 0 )
    {
      if( tSHP->reprojectorFromTileProjToLayerProj == NULL )
      {
          tSHP->reprojectorFromTileProjToLayerProj = msProjectCreateReprojector(&(tSHP->sTileProj), &(layer->projection));
      }
      if( tSHP->reprojectorFromTileProjToLayerProj )
      {
        msProjectShapeEx( tSHP->reprojectorFromTileProjToLayerProj, shape);
      }
    }

    shape->tileindex = tSHP->tileshpfile->lastshape;
    shape->numvalues = layer->numitems;
    shape->values = msDBFGetValueList(tSHP->shpfile->hDBF, i, layer->iteminfo, layer->numitems);
    if(!shape->values) shape->numvalues = 0;

    filter_passed = MS_TRUE;  /* By default accept ANY shape */
    if(layer->numitems > 0 && layer->iteminfo) {
      filter_passed = msEvalExpression(layer, shape, &(layer->filter), layer->filteritemindex);
    }

    if(!filter_passed) msFreeShape(shape); /* free's values as well */

  } while(!filter_passed);  /* Loop until both spatial and attribute filters match  */

  return(MS_SUCCESS);
}

static
int msTiledSHPGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
  const char *filename;
  char tilename[MS_MAXPATHLEN], szPath[MS_MAXPATHLEN];

  msTiledSHPLayerInfo *tSHP=NULL;
  char tiFileAbsDir[MS_MAXPATHLEN];

  long shapeindex = record->shapeindex;
  int tileindex = record->tileindex;

  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  tSHP = layer->layerinfo;
  if(!tSHP) {
    msSetError(MS_SHPERR, "Tiled shapefile layer has not been opened.", "msTiledSHPGetShape()");
    return(MS_FAILURE);
  }

  msTileIndexAbsoluteDir(tiFileAbsDir, layer);

  if((tileindex < 0) || (tileindex >= tSHP->tileshpfile->numshapes)) return(MS_FAILURE); /* invalid tile id */

  if(tileindex != tSHP->tileshpfile->lastshape) { /* correct tile is not currenly open so open the correct tile */
    msShapefileClose(tSHP->shpfile); /* close current tile */

    filename = msTiledSHPLoadEntry(layer, tileindex, tilename, sizeof(tilename));

    /* open the shapefile, since a specific tile was request an error should be generated if that tile does not exist */
    if(strlen(filename) == 0) return(MS_FAILURE);
    if(msShapefileOpen(tSHP->shpfile, "rb", msBuildPath3(szPath, tiFileAbsDir, layer->map->shapepath, filename), MS_TRUE) == -1) {
      if(msShapefileOpen(tSHP->shpfile, "rb", msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, filename), MS_TRUE) == -1) {
        if(msShapefileOpen(tSHP->shpfile, "rb", msBuildPath(szPath, layer->map->mappath, filename), MS_TRUE) == -1) {
          return(MS_FAILURE);
        }
      }
    }

  }

  if((shapeindex < 0) || (shapeindex >= tSHP->shpfile->numshapes)) return(MS_FAILURE);

  msSHPReadShape(tSHP->shpfile->hSHP, shapeindex, shape);
  tSHP->shpfile->lastshape = shapeindex;
  tSHP->tileshpfile->lastshape = tileindex;

  if( tSHP->sTileProj.numargs > 0 )
  {
      if( tSHP->reprojectorFromTileProjToLayerProj == NULL )
      {
          tSHP->reprojectorFromTileProjToLayerProj = msProjectCreateReprojector(&(tSHP->sTileProj), &(layer->projection));
      }
      if( tSHP->reprojectorFromTileProjToLayerProj )
      {
        msProjectShapeEx( tSHP->reprojectorFromTileProjToLayerProj, shape);
      }
  }

  if(layer->numitems > 0 && layer->iteminfo) {
    shape->numvalues = layer->numitems;
    shape->values = msDBFGetValueList(tSHP->shpfile->hDBF, shapeindex, layer->iteminfo, layer->numitems);
    if(!shape->values) return(MS_FAILURE);
  }

  shape->tileindex = tileindex;

  return(MS_SUCCESS);
}

static
void msTiledSHPClose(layerObj *layer)
{
  msTiledSHPLayerInfo *tSHP=NULL;

  tSHP = layer->layerinfo;
  if(tSHP) {
    msShapefileClose(tSHP->shpfile);
    free(tSHP->shpfile);

    if(tSHP->tilelayerindex != -1) {
      layerObj *tlp;
      if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
        return;
      tlp = (GET_LAYER(layer->map, tSHP->tilelayerindex));
      msLayerClose(tlp);
    } else {
      msShapefileClose(tSHP->tileshpfile);
      free(tSHP->tileshpfile);
    }

    msProjectDestroyReprojector(tSHP->reprojectorFromTileProjToLayerProj);

    msFreeProjection(&(tSHP->sTileProj));

    free(tSHP);
  }
  layer->layerinfo = NULL;
}
/************************************************************************/
/*                              msTiledSHPClose()                       */
/* Overloaded version of msTiledSHPClose for virtual table architecture */
/************************************************************************/
int msTiledSHPCloseVT(layerObj *layer)
{
  msTiledSHPClose(layer);
  return MS_SUCCESS;
}

void msTiledSHPLayerFreeItemInfo(layerObj *layer)
{
  if(layer->iteminfo) {
    free(layer->iteminfo);
    layer->iteminfo = NULL;
  }
}

int msTiledSHPLayerInitItemInfo(layerObj *layer)
{
  msTiledSHPLayerInfo *tSHP=NULL;

  tSHP = layer->layerinfo;
  if(!tSHP) {
    msSetError(MS_SHPERR, "Tiled shapefile layer has not been opened.", "msTiledSHPLayerInitItemInfo()");
    return MS_FAILURE;
  }

  msTiledSHPLayerFreeItemInfo(layer);
  layer->iteminfo = (int *) msDBFGetItemIndexes(tSHP->shpfile->hDBF, layer->items, layer->numitems);
  if(!layer->iteminfo) return(MS_FAILURE);

  return MS_SUCCESS;
}

static void msSHPPassThroughFieldDefinitions( layerObj *layer, DBFHandle hDBF )
{
  int numitems, i;

  numitems = msDBFGetFieldCount( hDBF );

  for(i=0; i<numitems; i++) {
    char item[16];
    int  nWidth=0, nPrecision=0;
    char gml_width[32], gml_precision[32];
    DBFFieldType eType;
    const char *gml_type = NULL;

    eType = msDBFGetFieldInfo( hDBF, i, item, &nWidth, &nPrecision );

    gml_width[0] = '\0';
    gml_precision[0] = '\0';

    switch( eType ) {
      case FTInteger:
        gml_type = "Integer";
        sprintf( gml_width, "%d", nWidth );
        break;

      case FTDouble:
        gml_type = "Real";
        sprintf( gml_width, "%d", nWidth );
        sprintf( gml_precision, "%d", nPrecision );
        break;

      case FTString:
      default:
        gml_type = "Character";
        sprintf( gml_width, "%d", nWidth );
        break;
    }

    msUpdateGMLFieldMetadata(layer, item, gml_type, gml_width, gml_precision, 0);

  }
}

int msTiledSHPLayerGetItems(layerObj *layer)
{
  msTiledSHPLayerInfo *tSHP=NULL;
  const char *value;

  tSHP = layer->layerinfo;
  if(!tSHP) {
    msSetError(MS_SHPERR, "Tiled shapefile layer has not been opened.", "msTiledSHPLayerGetItems()");
    return MS_FAILURE;
  }

  layer->numitems = msDBFGetFieldCount(tSHP->shpfile->hDBF);
  layer->items = msDBFGetItems(tSHP->shpfile->hDBF);
  if(!layer->items) return MS_FAILURE;

  /* -------------------------------------------------------------------- */
  /*      consider populating the field definitions in metadata.          */
  /* -------------------------------------------------------------------- */
  if((value = msOWSLookupMetadata(&(layer->metadata), "G", "types")) != NULL
      && strcasecmp(value,"auto") == 0 )
    msSHPPassThroughFieldDefinitions( layer, tSHP->shpfile->hDBF );

  return msTiledSHPLayerInitItemInfo(layer);
}

int msTiledSHPLayerGetExtent(layerObj *layer, rectObj *extent)
{
  msTiledSHPLayerInfo *tSHP=NULL;

  tSHP = layer->layerinfo;
  if(!tSHP) {
    msSetError(MS_SHPERR, "Tiled shapefile layer has not been opened.", "msTiledSHPLayerGetExtent()");
    return MS_FAILURE;
  }

  *extent = tSHP->tileshpfile->bounds;
  return MS_SUCCESS;
}

int msTiledSHPLayerIsOpen(layerObj *layer)
{
  if(layer->layerinfo)
    return MS_TRUE;
  else
    return MS_FALSE;
}

int msTiledSHPLayerSupportsCommonFilters(layerObj *layer)
{
  (void)layer;
  return MS_TRUE;
}

int msTiledSHPLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerSupportsCommonFilters = msTiledSHPLayerSupportsCommonFilters;
  layer->vtable->LayerInitItemInfo = msTiledSHPLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msTiledSHPLayerFreeItemInfo;
  layer->vtable->LayerOpen = msTiledSHPOpenFile;
  layer->vtable->LayerIsOpen = msTiledSHPLayerIsOpen;
  layer->vtable->LayerWhichShapes = msTiledSHPWhichShapes;
  layer->vtable->LayerNextShape = msTiledSHPNextShape;
  /* no special version, use ...GetShape() */
  /* layer->vtable->LayerResultsGetShape = msTiledSHPGetShape; */
  layer->vtable->LayerGetShape = msTiledSHPGetShape;
  layer->vtable->LayerClose = msTiledSHPCloseVT;
  layer->vtable->LayerGetItems = msTiledSHPLayerGetItems;
  layer->vtable->LayerGetExtent = msTiledSHPLayerGetExtent;
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerCloseConnection, use default */;
  layer->vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */
  /* layer->vtable->LayerGetAutoProjection, use defaut*/

  return MS_SUCCESS;
}

/* SHAPEFILE Layer virtual table functions */

void msSHPLayerFreeItemInfo(layerObj *layer)
{
  if(layer->iteminfo) {
    free(layer->iteminfo);
    layer->iteminfo = NULL;
  }
}

int msSHPLayerInitItemInfo(layerObj *layer)
{
  shapefileObj *shpfile = layer->layerinfo;
  if( ! shpfile) {
    msSetError(MS_SHPERR, "Shapefile layer has not been opened.", "msSHPLayerInitItemInfo()");
    return MS_FAILURE;
  }

  /* iteminfo needs to be a bit more complex, a list of indexes plus the length of the list */
  msSHPLayerFreeItemInfo(layer);
  layer->iteminfo = (int *) msDBFGetItemIndexes(shpfile->hDBF, layer->items, layer->numitems);
  if( ! layer->iteminfo) {
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

int msSHPLayerOpen(layerObj *layer)
{
  char szPath[MS_MAXPATHLEN];
  shapefileObj *shpfile;

  if(layer->layerinfo) return MS_SUCCESS; /* layer already open */

  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  /* allocate space for a shapefileObj using layer->layerinfo  */
  shpfile = (shapefileObj *) malloc(sizeof(shapefileObj));
  MS_CHECK_ALLOC(shpfile, sizeof(shapefileObj), MS_FAILURE);


  layer->layerinfo = shpfile;

  if(msShapefileOpen(shpfile, "rb", msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, layer->data), MS_TRUE) == -1) {
    if(msShapefileOpen(shpfile, "rb", msBuildPath(szPath, layer->map->mappath, layer->data), MS_TRUE) == -1) {
      layer->layerinfo = NULL;
      free(shpfile);
      return MS_FAILURE;
    }
  }
  
  if (layer->projection.numargs > 0 &&
      EQUAL(layer->projection.args[0], "auto"))
  {
    const char* pszPRJFilename = CPLResetExtension(szPath, "prj");
    int bOK = MS_FALSE;
    VSILFILE* fp = VSIFOpenL(pszPRJFilename, "rb");
    if( fp != NULL )
    {
        char szPRJ[2048];
        OGRSpatialReferenceH hSRS;
        int nRead;

        nRead = (int)VSIFReadL(szPRJ, 1, sizeof(szPRJ) - 1, fp);
        szPRJ[nRead] = '\0';
        hSRS = OSRNewSpatialReference(szPRJ);
        if( hSRS != NULL )
        {
            if( OSRMorphFromESRI( hSRS ) == OGRERR_NONE )
            {
                char* pszWKT = NULL;
                if( OSRExportToWkt( hSRS, &pszWKT ) == OGRERR_NONE )
                {
                    if( msOGCWKT2ProjectionObj(pszWKT, &(layer->projection),
                                               layer->debug ) == MS_SUCCESS )
                    {
                        bOK = MS_TRUE;
                    }
                }
                CPLFree(pszWKT);
            }
            OSRDestroySpatialReference(hSRS);
        }
      VSIFCloseL(fp);
    }

    if( bOK != MS_TRUE )
    {
        if( layer->debug || layer->map->debug ) {
            msDebug( "Unable to get SRS from shapefile '%s' for layer '%s'.\n", szPath, layer->name );
        }
    }
  }

  return MS_SUCCESS;
}

int msSHPLayerIsOpen(layerObj *layer)
{
  if(layer->layerinfo)
    return MS_TRUE;
  else
    return MS_FALSE;
}

int msSHPLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery)
{
  (void)isQuery;
  int status;
  shapefileObj *shpfile;

  shpfile = layer->layerinfo;

  if(!shpfile) {
    msSetError(MS_SHPERR, "Shapefile layer has not been opened.", "msSHPLayerWhichShapes()");
    return MS_FAILURE;
  }

  status = msShapefileWhichShapes(shpfile, rect, layer->debug);
  if(status != MS_SUCCESS) {
    return status;
  }

  return MS_SUCCESS;
}

int msSHPLayerNextShape(layerObj *layer, shapeObj *shape)
{
  int i;
  shapefileObj *shpfile;

  shpfile = layer->layerinfo;

  if(!shpfile) {
    msSetError(MS_SHPERR, "Shapefile layer has not been opened.", "msSHPLayerNextShape()");
    return MS_FAILURE;
  }

  i = msGetNextBit(shpfile->status, shpfile->lastshape + 1, shpfile->numshapes);
  shpfile->lastshape = i;
  if(i == -1) return(MS_DONE); /* nothing else to read */

  msSHPReadShape(shpfile->hSHP, i, shape);
  if(shape->type == MS_SHAPE_NULL) {
    msFreeShape(shape);
    return msSHPLayerNextShape(layer, shape); /* skip NULL shapes */
  }
  shape->numvalues = layer->numitems;
  shape->values = msDBFGetValueList(shpfile->hDBF, i, layer->iteminfo, layer->numitems);
  if(!shape->values) shape->numvalues = 0;

  return MS_SUCCESS;
}

int msSHPLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
  shapefileObj *shpfile;
  long shapeindex;

  shpfile = layer->layerinfo;

  shapeindex = record->shapeindex;

  if(!shpfile) {
    msSetError(MS_SHPERR, "Shapefile layer has not been opened.", "msSHPLayerGetShape()");
    return MS_FAILURE;
  }

  /* msSHPReadShape *should* return success or failure so we don't have to test here */
  if(shapeindex < 0 || shapeindex >= shpfile->numshapes) {
    msSetError(MS_MISCERR, "Invalid feature id.", "msSHPLayerGetShape()");
    return MS_FAILURE;
  }

  msSHPReadShape(shpfile->hSHP, shapeindex, shape);
  if(layer->numitems > 0 && layer->iteminfo) {
    shape->numvalues = layer->numitems;
    shape->values = msDBFGetValueList(shpfile->hDBF, shapeindex, layer->iteminfo, layer->numitems);
    if(!shape->values) return MS_FAILURE;
  }

  shpfile->lastshape = shapeindex;

  return MS_SUCCESS;
}

int msSHPLayerClose(layerObj *layer)
{
  shapefileObj *shpfile;
  shpfile = layer->layerinfo;
  if(!shpfile) return MS_SUCCESS; /* nothing to do */

  msShapefileClose(shpfile);
  free(layer->layerinfo);
  layer->layerinfo = NULL;

  return MS_SUCCESS;
}

int msSHPLayerGetItems(layerObj *layer)
{
  shapefileObj *shpfile;
  const char *value;

  shpfile = layer->layerinfo;

  if(!shpfile) {
    msSetError(MS_SHPERR, "Shapefile layer has not been opened.", "msSHPLayerGetItems()");
    return MS_FAILURE;
  }

  layer->numitems = msDBFGetFieldCount(shpfile->hDBF);
  layer->items = msDBFGetItems(shpfile->hDBF);
  if(layer->numitems == 0) return MS_SUCCESS; /* No items is a valid case (#3147) */
  if(!layer->items) return MS_FAILURE;

  /* -------------------------------------------------------------------- */
  /*      consider populating the field definitions in metadata.          */
  /* -------------------------------------------------------------------- */
  if((value = msOWSLookupMetadata(&(layer->metadata), "G", "types")) != NULL
      && strcasecmp(value,"auto") == 0 )
    msSHPPassThroughFieldDefinitions( layer, shpfile->hDBF );

  return msLayerInitItemInfo(layer);
}

int msSHPLayerGetExtent(layerObj *layer, rectObj *extent)
{
  *extent = ((shapefileObj*)layer->layerinfo)->bounds;
  return MS_SUCCESS;
}

int msSHPLayerSupportsCommonFilters(layerObj *layer)
{
  (void)layer;
  return MS_TRUE;
}

int msSHPLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerSupportsCommonFilters = msSHPLayerSupportsCommonFilters;
  layer->vtable->LayerInitItemInfo = msSHPLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msSHPLayerFreeItemInfo;
  layer->vtable->LayerOpen = msSHPLayerOpen;
  layer->vtable->LayerIsOpen = msSHPLayerIsOpen;
  layer->vtable->LayerWhichShapes = msSHPLayerWhichShapes;
  layer->vtable->LayerNextShape = msSHPLayerNextShape;
  layer->vtable->LayerGetShape = msSHPLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */
  layer->vtable->LayerClose = msSHPLayerClose;
  layer->vtable->LayerGetItems = msSHPLayerGetItems;
  layer->vtable->LayerGetExtent = msSHPLayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerCloseConnection, use default */
  layer->vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
  /* layer->vtable->LayerTranslateFilter, use default */
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */

  return MS_SUCCESS;
}

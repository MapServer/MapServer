/*
** This code is entirely based on the previous work of Frank Warmerdam. It is
** essentially shapelib 1.1.5. However, there were enough changes that it was
** incorporated into the MapServer source to avoid confusion. See the README 
** for licence details.
*/

#include "map.h"
#include <limits.h>

typedef unsigned char uchar;

#if UINT_MAX == 65535
typedef long          int32;
#else
typedef int           int32;
#endif

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
  uchar	temp;
  
  for( i=0; i < length/2; i++ )
    {
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
  if( pMem == NULL )
    return( (void *) malloc(nNewSize) );
  else
    return( (void *) realloc(pMem,nNewSize) );
}

/************************************************************************/
/*                          writeHeader()                               */
/*                                                                      */
/*      Write out a header for the .shp and .shx files as well as the	*/
/*	contents of the index (.shx) file.				*/
/************************************************************************/
static void writeHeader( SHPHandle psSHP )
{
  uchar     	abyHeader[100];
  int		i;
  int32	i32;
  double	dValue;
  int32	*panSHX;
  
  /* -------------------------------------------------------------------- */
  /*      Prepare header block for .shp file.                             */
  /* -------------------------------------------------------------------- */
  for( i = 0; i < 100; i++ )
    abyHeader[i] = 0;
  
  abyHeader[2] = 0x27;				/* magic cookie */
  abyHeader[3] = 0x0a;
  
  i32 = psSHP->nFileSize/2;				/* file size */
  ByteCopy( &i32, abyHeader+24, 4 );
  if( !bBigEndian ) SwapWord( 4, abyHeader+24 );
  
  i32 = 1000;						/* version */
  ByteCopy( &i32, abyHeader+28, 4 );
  if( bBigEndian ) SwapWord( 4, abyHeader+28 );
  
  i32 = psSHP->nShapeType;				/* shape type */
  ByteCopy( &i32, abyHeader+32, 4 );
  if( bBigEndian ) SwapWord( 4, abyHeader+32 );
  
  dValue = psSHP->adBoundsMin[0];			/* set bounds */
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
  
  /* -------------------------------------------------------------------- */
  /*      Write .shp file header.                                         */
  /* -------------------------------------------------------------------- */
  fseek( psSHP->fpSHP, 0, 0 );
  fwrite( abyHeader, 100, 1, psSHP->fpSHP );
  
  /* -------------------------------------------------------------------- */
  /*      Prepare, and write .shx file header.                            */
  /* -------------------------------------------------------------------- */
  i32 = (psSHP->nRecords * 2 * sizeof(int32) + 100)/2;   /* file size */
  ByteCopy( &i32, abyHeader+24, 4 );
  if( !bBigEndian ) SwapWord( 4, abyHeader+24 );
  
  fseek( psSHP->fpSHX, 0, 0 );
  fwrite( abyHeader, 100, 1, psSHP->fpSHX );
  
  /* -------------------------------------------------------------------- */
  /*      Write out the .shx contents.                                    */
  /* -------------------------------------------------------------------- */
  panSHX = (int32 *) malloc(sizeof(int32) * 2 * psSHP->nRecords);
  
  for( i = 0; i < psSHP->nRecords; i++ ) {
    panSHX[i*2  ] = psSHP->panRecOffset[i]/2;
    panSHX[i*2+1] = psSHP->panRecSize[i]/2;
    if( !bBigEndian ) SwapWord( 4, panSHX+i*2 );
    if( !bBigEndian ) SwapWord( 4, panSHX+i*2+1 );
  }
  
  fwrite( panSHX, sizeof(int32) * 2, psSHP->nRecords, psSHP->fpSHX );
  
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
  char		*pszFullname, *pszBasename;
  SHPHandle		psSHP;
  
  uchar		*pabyBuf;
  int			i;
  double		dValue;
  
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
  /*	Establish the byte order on this machine.			    */
  /* -------------------------------------------------------------------- */
  i = 1;
  if( *((uchar *) &i) == 1 )
    bBigEndian = MS_FALSE;
  else
    bBigEndian = MS_TRUE;
  
  /* -------------------------------------------------------------------- */
  /*	Initialize the info structure.					    */
  /* -------------------------------------------------------------------- */
  psSHP = (SHPHandle) malloc(sizeof(SHPInfo));
  
  psSHP->bUpdated = MS_FALSE;
  
  /* -------------------------------------------------------------------- */
  /*	Compute the base (layer) name.  If there is any extension	    */
  /*	on the passed in filename we will strip it off.			    */
  /* -------------------------------------------------------------------- */
  pszBasename = (char *) malloc(strlen(pszLayer)+5);
  strcpy( pszBasename, pszLayer );
  for( i = strlen(pszBasename)-1; 
       i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
	 && pszBasename[i] != '\\';
       i-- ) {}
  
  if( pszBasename[i] == '.' )
    pszBasename[i] = '\0';
  
  /* -------------------------------------------------------------------- */
  /*	Open the .shp and .shx files.  Note that files pulled from	    */
  /*	a PC to Unix with upper case filenames won't work!		    */
  /* -------------------------------------------------------------------- */
  pszFullname = (char *) malloc(strlen(pszBasename) + 5);
  sprintf( pszFullname, "%s.shp", pszBasename );
  psSHP->fpSHP = fopen(pszFullname, pszAccess );
  if( psSHP->fpSHP == NULL )
    return( NULL );
  
  sprintf( pszFullname, "%s.shx", pszBasename );
  psSHP->fpSHX = fopen(pszFullname, pszAccess );
  if( psSHP->fpSHX == NULL )
    return( NULL );
  
  free( pszFullname );
  
  /* -------------------------------------------------------------------- */
  /*   Read the file size from the SHP file.				    */
  /* -------------------------------------------------------------------- */
  pabyBuf = (uchar *) malloc(100);
  fread( pabyBuf, 100, 1, psSHP->fpSHP );
  
  psSHP->nFileSize = (pabyBuf[24] * 256 * 256 * 256
		      + pabyBuf[25] * 256 * 256
		      + pabyBuf[26] * 256
		      + pabyBuf[27]) * 2;
  
  /* -------------------------------------------------------------------- */
  /*  Read SHX file Header info                                           */
  /* -------------------------------------------------------------------- */
  fread( pabyBuf, 100, 1, psSHP->fpSHX );
  
  if( pabyBuf[0] != 0 
      || pabyBuf[1] != 0 
      || pabyBuf[2] != 0x27 
      || (pabyBuf[3] != 0x0a && pabyBuf[3] != 0x0d) )
    {
      fclose( psSHP->fpSHP );
      fclose( psSHP->fpSHX );
      free( psSHP );
      
      return( NULL );
    }
  
  psSHP->nRecords = pabyBuf[27] + pabyBuf[26] * 256
    + pabyBuf[25] * 256 * 256 + pabyBuf[24] * 256 * 256 * 256;
  psSHP->nRecords = (psSHP->nRecords*2 - 100) / 8;
  
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
  
  free( pabyBuf );
  
  /* -------------------------------------------------------------------- */
  /*	Read the .shx file to get the offsets to each record in 	    */
  /*	the .shp file.							    */
  /* -------------------------------------------------------------------- */
  psSHP->nMaxRecords = psSHP->nRecords;
  
  psSHP->panRecOffset = (int *) malloc(sizeof(int) * psSHP->nMaxRecords );
  psSHP->panRecSize = (int *) malloc(sizeof(int) * psSHP->nMaxRecords );
  
  pabyBuf = (uchar *) malloc(8 * psSHP->nRecords );
  fread( pabyBuf, 8, psSHP->nRecords, psSHP->fpSHX );
  
  for( i = 0; i < psSHP->nRecords; i++ ) {
    int32 nOffset, nLength;
    
    memcpy( &nOffset, pabyBuf + i * 8, 4 );
    if( !bBigEndian ) SwapWord( 4, &nOffset );
    
    memcpy( &nLength, pabyBuf + i * 8 + 4, 4 );
    if( !bBigEndian ) SwapWord( 4, &nLength );
    
    psSHP->panRecOffset[i] = nOffset*2;
    psSHP->panRecSize[i] = nLength*2;
  }
  free( pabyBuf );
  
  return( psSHP );
}

/************************************************************************/
/*                              msSHPClose()                            */
/*								       	*/
/*	Close the .shp and .shx files.					*/
/************************************************************************/
void msSHPClose(SHPHandle psSHP )
{
  /* -------------------------------------------------------------------- */
  /*	Update the header if we have modified anything.		    	  */
  /* -------------------------------------------------------------------- */
  if( psSHP->bUpdated )
    writeHeader( psSHP );
  
  /* -------------------------------------------------------------------- */
  /*      Free all resources, and close files.                            */
  /* -------------------------------------------------------------------- */
  free( psSHP->panRecOffset );
  free( psSHP->panRecSize );
  
  fclose( psSHP->fpSHX );
  fclose( psSHP->fpSHP );
  
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
  char	*pszBasename, *pszFullname;
  int		i;
  FILE	*fpSHP, *fpSHX;
  uchar     	abyHeader[100];
  int32	i32;
  double	dValue;
  
  /* -------------------------------------------------------------------- */
  /*      Establish the byte order on this system.                        */
  /* -------------------------------------------------------------------- */
  i = 1;
  if( *((uchar *) &i) == 1 )
    bBigEndian = MS_FALSE;
  else
    bBigEndian = MS_TRUE;
  
  /* -------------------------------------------------------------------- */
  /*	Compute the base (layer) name.  If there is any extension  	    */
  /*	on the passed in filename we will strip it off.			    */
  /* -------------------------------------------------------------------- */
  pszBasename = (char *) malloc(strlen(pszLayer)+5);
  strcpy( pszBasename, pszLayer );
  for( i = strlen(pszBasename)-1; 
       i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
	 && pszBasename[i] != '\\';
       i-- ) {}
  
  if( pszBasename[i] == '.' )
    pszBasename[i] = '\0';
  
  /* -------------------------------------------------------------------- */
  /*      Open the two files so we can write their headers.               */
  /* -------------------------------------------------------------------- */
  pszFullname = (char *) malloc(strlen(pszBasename) + 5);
  sprintf( pszFullname, "%s.shp", pszBasename );
  fpSHP = fopen(pszFullname, "wb" );
  if( fpSHP == NULL )
    return( NULL );
  
  sprintf( pszFullname, "%s.shx", pszBasename );
  fpSHX = fopen(pszFullname, "wb" );
  if( fpSHX == NULL )
    return( NULL );
  
  free( pszFullname );
  
  /* -------------------------------------------------------------------- */
  /*      Prepare header block for .shp file.                             */
  /* -------------------------------------------------------------------- */
  for( i = 0; i < 100; i++ )
    abyHeader[i] = 0;
  
  abyHeader[2] = 0x27;				/* magic cookie */
  abyHeader[3] = 0x0a;
  
  i32 = 50;						/* file size */
  ByteCopy( &i32, abyHeader+24, 4 );
  if( !bBigEndian ) SwapWord( 4, abyHeader+24 );
  
  i32 = 1000;						/* version */
  ByteCopy( &i32, abyHeader+28, 4 );
  if( bBigEndian ) SwapWord( 4, abyHeader+28 );
  
  i32 = nShapeType;					/* shape type */
  ByteCopy( &i32, abyHeader+32, 4 );
  if( bBigEndian ) SwapWord( 4, abyHeader+32 );
  
  dValue = 0.0;					/* set bounds */
  ByteCopy( &dValue, abyHeader+36, 8 );
  ByteCopy( &dValue, abyHeader+44, 8 );
  ByteCopy( &dValue, abyHeader+52, 8 );
  ByteCopy( &dValue, abyHeader+60, 8 );
  
  /* -------------------------------------------------------------------- */
  /*      Write .shp file header.                                         */
  /* -------------------------------------------------------------------- */
  fwrite( abyHeader, 100, 1, fpSHP );
  
  /* -------------------------------------------------------------------- */
  /*      Prepare, and write .shx file header.                            */
  /* -------------------------------------------------------------------- */
  i32 = 50;						/* file size */
  ByteCopy( &i32, abyHeader+24, 4 );
  if( !bBigEndian ) SwapWord( 4, abyHeader+24 );
  
  fwrite( abyHeader, 100, 1, fpSHX );
  
  /* -------------------------------------------------------------------- */
  /*      Close the files, and then open them as regular existing files.  */
  /* -------------------------------------------------------------------- */
  fclose( fpSHP );
  fclose( fpSHX );
  
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
  double	dXMin, dXMax, dYMin, dYMax;
  int		i, j;
  
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

int msSHPWriteShape(SHPHandle psSHP, shapeObj *shape )
{
  int	       	nRecordOffset, i, j, k, nRecordSize=0;
  uchar	*pabyRec;
  int32	i32, nPoints, nParts;
  
  psSHP->bUpdated = MS_TRUE;
  
  /* -------------------------------------------------------------------- */
  /*      Add the new entity to the in memory index.                      */
  /* -------------------------------------------------------------------- */
  psSHP->nRecords++;
  if( psSHP->nRecords > psSHP->nMaxRecords ) {
    psSHP->nMaxRecords = psSHP->nMaxRecords * 1.3 + 100;
    
    psSHP->panRecOffset = (int *) 
      SfRealloc(psSHP->panRecOffset,sizeof(int) * psSHP->nMaxRecords );
    psSHP->panRecSize = (int *) 
      SfRealloc(psSHP->panRecSize,sizeof(int) * psSHP->nMaxRecords );
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
  
  pabyRec = (uchar *) malloc(nPoints * 2 * sizeof(double) + nParts * 4 + 128);
  
  
  /* -------------------------------------------------------------------- */
  /*  Write vertices for a Polygon or Arc.				    */
  /* -------------------------------------------------------------------- */
  if( psSHP->nShapeType == MS_SHP_POLYGON || psSHP->nShapeType == MS_SHP_ARC ) {
    int32 t_nParts, t_nPoints, partSize;
    
    t_nParts = nParts;
    t_nPoints = nPoints;
    
    writeBounds( pabyRec + 12, shape, t_nPoints );
    
    if( bBigEndian ) { 
      SwapWord( 4, &nPoints );
      SwapWord( 4, &nParts );
    }
    
    ByteCopy( &nPoints, pabyRec + 40 + 8, 4 );
    ByteCopy( &nParts, pabyRec + 36 + 8, 4 );

    partSize = 0; // first part always starts at 0
    ByteCopy( &partSize, pabyRec + 44 + 8 + 4*0, 4 );
    if( bBigEndian ) SwapWord( 4, pabyRec + 44 + 8 + 4*0);

    for( i = 1; i < t_nParts; i++ ) {
      partSize += shape->line[i-1].numpoints;
      ByteCopy( &partSize, pabyRec + 44 + 8 + 4*i, 4 );
      if( bBigEndian ) SwapWord( 4, pabyRec + 44 + 8 + 4*i);
    }
    
    k = 0; // overall point counter
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
  }
  
  /* -------------------------------------------------------------------- */
  /*  Write vertices for a MultiPoint.				    */
  /* -------------------------------------------------------------------- */
  else if( psSHP->nShapeType == MS_SHP_MULTIPOINT ) {
    int32 t_nPoints;
    
    t_nPoints = nPoints;
    
    writeBounds( pabyRec + 12, shape, nPoints );
    
    if( bBigEndian ) SwapWord( 4, &nPoints );
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
  }
  
  /* -------------------------------------------------------------------- */
  /*      Write vertices for a point.                                     */
  /* -------------------------------------------------------------------- */
  else if( psSHP->nShapeType == MS_SHP_POINT ) {
    ByteCopy( &(shape->line[0].point[0].x), pabyRec + 12, 8 );
    ByteCopy( &(shape->line[0].point[0].y), pabyRec + 20, 8 );
    
    if( bBigEndian ) {
      SwapWord( 8, pabyRec + 12 );
      SwapWord( 8, pabyRec + 20 );
    }
    
    nRecordSize = 20;
  }
  
  /* -------------------------------------------------------------------- */
  /*      Set the shape type, record number, and record size.             */
  /* -------------------------------------------------------------------- */
  i32 = psSHP->nRecords-1+1;					/* record # */
  if( !bBigEndian ) SwapWord( 4, &i32 );
  ByteCopy( &i32, pabyRec, 4 );
  
  i32 = nRecordSize/2;				/* record size */
  if( !bBigEndian ) SwapWord( 4, &i32 );
  ByteCopy( &i32, pabyRec + 4, 4 );
  
  i32 = psSHP->nShapeType;				/* shape type */
  if( bBigEndian ) SwapWord( 4, &i32 );
  ByteCopy( &i32, pabyRec + 8, 4 );
  
  /* -------------------------------------------------------------------- */
  /*      Write out record.                                               */
  /* -------------------------------------------------------------------- */
  fseek( psSHP->fpSHP, nRecordOffset, 0 );
  fwrite( pabyRec, nRecordSize+8, 1, psSHP->fpSHP );
  free( pabyRec );
  
  psSHP->panRecSize[psSHP->nRecords-1] = nRecordSize;
  psSHP->nFileSize += nRecordSize + 8;
  
  /* -------------------------------------------------------------------- */
  /*	Expand file wide bounds based on this shape.			    */
  /* -------------------------------------------------------------------- */
  if( psSHP->nRecords == 1 ) {
    psSHP->adBoundsMin[0] = psSHP->adBoundsMax[0] = shape->line[0].point[0].x;
    psSHP->adBoundsMin[1] = psSHP->adBoundsMax[1] = shape->line[0].point[0].y;
  }
  
  for( i=0; i<shape->numlines; i++ ) {
    for( j=0; j<shape->line[i].numpoints; j++ ) {
      psSHP->adBoundsMin[0] = MS_MIN(psSHP->adBoundsMin[0], shape->line[i].point[j].x);
      psSHP->adBoundsMin[1] = MS_MIN(psSHP->adBoundsMin[1], shape->line[i].point[j].y);
      psSHP->adBoundsMax[0] = MS_MAX(psSHP->adBoundsMax[0], shape->line[i].point[j].x);
      psSHP->adBoundsMax[1] = MS_MAX(psSHP->adBoundsMax[1], shape->line[i].point[j].y);
    }
  }
  
  return( psSHP->nRecords - 1 );
}

/*
** msSHPReadShape() - Reads the vertices for one shape from a shape file.
*/
void msSHPReadShape( SHPHandle psSHP, int hEntity, shapeObj *shape )
{
    int	       		i, j, k;
    static uchar	*pabyRec = NULL;   
    static int		nPartMax = 0, *panParts = NULL;
    static int		nBufSize = 0;

    msInitShape(shape); /* initialize the shape */

    /* -------------------------------------------------------------------- */
    /*      Validate the record/entity number.                              */
    /* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity >= psSHP->nRecords )
      return;

    if( psSHP->panRecSize[hEntity] == 4 )
      return;

    /* -------------------------------------------------------------------- */
    /*      Ensure our record buffer is large enough.                       */
    /* -------------------------------------------------------------------- */
    if( psSHP->panRecSize[hEntity]+8 > nBufSize )
    {
	nBufSize = psSHP->panRecSize[hEntity]+8;
	pabyRec = (uchar *) SfRealloc(pabyRec,nBufSize);
    }    

    /* -------------------------------------------------------------------- */
    /*      Read the record.                                                */
    /* -------------------------------------------------------------------- */
    fseek( psSHP->fpSHP, psSHP->panRecOffset[hEntity], 0 );
    fread( pabyRec, psSHP->panRecSize[hEntity]+8, 1, psSHP->fpSHP );

    /* -------------------------------------------------------------------- */
    /*  Extract vertices for a Polygon or Arc.				    */
    /* -------------------------------------------------------------------- */
    if( psSHP->nShapeType == MS_SHP_POLYGON || psSHP->nShapeType == MS_SHP_ARC )
    {
      int32		nPoints, nParts;      

      // copy the bounding box
      memcpy( &shape->bounds.minx, pabyRec + 8 + 4, 8 );
      memcpy( &shape->bounds.miny, pabyRec + 8 + 12, 8 );
      memcpy( &shape->bounds.maxx, pabyRec + 8 + 20, 8 );
      memcpy( &shape->bounds.maxy, pabyRec + 8 + 28, 8 );

      if( bBigEndian ) {
	SwapWord( 8, &shape->bounds.minx);
	SwapWord( 8, &shape->bounds.miny);
	SwapWord( 8, &shape->bounds.maxx);
	SwapWord( 8, &shape->bounds.maxy);
      }

      memcpy( &nPoints, pabyRec + 40 + 8, 4 );
      memcpy( &nParts, pabyRec + 36 + 8, 4 );
      
      if( bBigEndian ) {
	SwapWord( 4, &nPoints );
        SwapWord( 4, &nParts );
      }

      /* -------------------------------------------------------------------- */
      /*      Copy out the part array from the record.                        */
      /* -------------------------------------------------------------------- */
      if( nPartMax < nParts )
      {
	nPartMax = nParts;
	panParts = (int *) SfRealloc(panParts, nPartMax * sizeof(int) );
      }
      
      memcpy( panParts, pabyRec + 44 + 8, 4 * nParts );
      for( i = 0; i < nParts; i++ )
	if( bBigEndian ) SwapWord( 4, panParts+i );
      
      /* -------------------------------------------------------------------- */
      /*      Fill the shape structure.                                       */
      /* -------------------------------------------------------------------- */
      if( (shape->line = (lineObj *)malloc(sizeof(lineObj)*nParts)) == NULL ) {
	msSetError(MS_MEMERR, NULL, "SHPReadShape()");
	return;
      }

      shape->numlines = nParts;
      
      k = 0; /* overall point counter */
      for( i = 0; i < nParts; i++) 
      { 	  
	if( i == nParts-1)
	  shape->line[i].numpoints = nPoints - panParts[i];
	else
	  shape->line[i].numpoints = panParts[i+1] - panParts[i];
	
	if( (shape->line[i].point = (pointObj *)malloc(sizeof(pointObj)*shape->line[i].numpoints)) == NULL ) {
	  free(shape->line);
	  shape->numlines = 0;
	  return;
	}

	for( j = 0; j < shape->line[i].numpoints; j++ )
	{
	  memcpy(&(shape->line[i].point[j].x), pabyRec + 44 + 4*nParts + 8 + k * 16, 8 );
	  memcpy(&(shape->line[i].point[j].y), pabyRec + 44 + 4*nParts + 8 + k * 16 + 8, 8 );
	  
	  if( bBigEndian ) {
	    SwapWord( 8, &(shape->line[i].point[j].x) );
	    SwapWord( 8, &(shape->line[i].point[j].y) );
	  }

	  k++;
	}
      }

      if(psSHP->nShapeType == MS_SHP_POLYGON)
	shape->type = MS_POLYGON;
      else
	shape->type = MS_LINE;

    }

    /* -------------------------------------------------------------------- */
    /*  Extract a MultiPoint.   			                    */
    /* -------------------------------------------------------------------- */
    else if( psSHP->nShapeType == MS_SHP_MULTIPOINT)
    {
      int32		nPoints;

      // copy the bounding box
      memcpy( &shape->bounds.minx, pabyRec + 8 + 4, 8 );
      memcpy( &shape->bounds.miny, pabyRec + 8 + 12, 8 );
      memcpy( &shape->bounds.maxx, pabyRec + 8 + 20, 8 );
      memcpy( &shape->bounds.maxy, pabyRec + 8 + 28, 8 );

      if( bBigEndian ) {
	SwapWord( 8, &shape->bounds.minx);
	SwapWord( 8, &shape->bounds.miny);
	SwapWord( 8, &shape->bounds.maxx);
	SwapWord( 8, &shape->bounds.maxy);
      }

      memcpy( &nPoints, pabyRec + 44, 4 );
      if( bBigEndian ) SwapWord( 4, &nPoints );
    
      /* -------------------------------------------------------------------- */
      /*      Fill the shape structure.                                       */
      /* -------------------------------------------------------------------- */
      if( (shape->line = (lineObj *)malloc(sizeof(lineObj))) == NULL ) {
	msSetError(MS_MEMERR, NULL, "SHPReadShape()");
	return;
      }

      shape->numlines = 1;
      shape->line[0].numpoints = nPoints;
      shape->line[0].point = (pointObj *) malloc( nPoints * sizeof(pointObj) );
      
      for( i = 0; i < nPoints; i++ ) {
	memcpy(&(shape->line[0].point[i].x), pabyRec + 48 + 16 * i, 8 );
	memcpy(&(shape->line[0].point[i].y), pabyRec + 48 + 16 * i + 8, 8 );
	
	if( bBigEndian ) {
	  SwapWord( 8, &(shape->line[0].point[i].x) );
	  SwapWord( 8, &(shape->line[0].point[i].y) );
	}
      }

      shape->type = MS_POINT;
    }

    /* -------------------------------------------------------------------- */
    /*  Extract a Point.   			                    */
    /* -------------------------------------------------------------------- */
    else if( psSHP->nShapeType == MS_SHP_POINT )
    {    

      /* -------------------------------------------------------------------- */
      /*      Fill the shape structure.                                       */
      /* -------------------------------------------------------------------- */
      if( (shape->line = (lineObj *)malloc(sizeof(lineObj))) == NULL ) {
	msSetError(MS_MEMERR, NULL, "SHPReadShape()");
	return;
      }

      shape->numlines = 1;
      shape->line[0].numpoints = 1;
      shape->line[0].point = (pointObj *) malloc(sizeof(pointObj));
      
      memcpy( &(shape->line[0].point[0].x), pabyRec + 12, 8 );
      memcpy( &(shape->line[0].point[0].y), pabyRec + 20, 8 );
      
      if( bBigEndian ) {
	SwapWord( 8, &(shape->line[0].point[0].x));
	SwapWord( 8, &(shape->line[0].point[0].y));
      }

      // set the bounding box to the point
      shape->bounds.minx = shape->bounds.maxx = shape->line[0].point[0].x;
      shape->bounds.miny = shape->bounds.maxy = shape->line[0].point[0].y;

      shape->type = MS_POINT;
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
    return(-1);
  }

  /* -------------------------------------------------------------------- */
  /*	If the entity is -1 we fetch the bounds for the whole file.	  */
  /* -------------------------------------------------------------------- */
  if( hEntity == -1 ) {
    padBounds->minx = psSHP->adBoundsMin[0];
    padBounds->miny = psSHP->adBoundsMin[1];
    padBounds->maxx = psSHP->adBoundsMax[0];
    padBounds->maxy = psSHP->adBoundsMax[1];
  } else {    
    if( psSHP->panRecSize[hEntity] == 4 ) { // NULL shape
      padBounds->minx = padBounds->miny = padBounds->maxx = padBounds->maxy = 0.0;
      return(-1);
    } 
    
    if( psSHP->nShapeType != MS_SHP_POINT ) {
      fseek( psSHP->fpSHP, psSHP->panRecOffset[hEntity]+12, 0 );
      fread( padBounds, sizeof(double)*4, 1, psSHP->fpSHP );
      
      if( bBigEndian ) {
	SwapWord( 8, &(padBounds->minx) );
	SwapWord( 8, &(padBounds->miny) );
	SwapWord( 8, &(padBounds->maxx) );
	SwapWord( 8, &(padBounds->maxy) );
      }
    } else {
      /* -------------------------------------------------------------------- */
      /*      For points we fetch the point, and duplicate it as the          */
      /*      minimum and maximum bound.                                      */
      /* -------------------------------------------------------------------- */
      
      fseek( psSHP->fpSHP, psSHP->panRecOffset[hEntity]+12, 0 );
      fread( padBounds, sizeof(double)*2, 1, psSHP->fpSHP );
      
      if( bBigEndian ) {
	SwapWord( 8, &(padBounds->minx) );
	SwapWord( 8, &(padBounds->miny) );
      }
      
      padBounds->maxx = padBounds->minx;
      padBounds->maxy = padBounds->miny;
    }
  }

  return(0);
}

int msSHPOpenFile(shapefileObj *shpfile, char *mode, char *shapepath, char *filename)
{
  int i;
  char *dbfFilename;

  char old_path[MS_PATH_LENGTH];

  getcwd(old_path, MS_PATH_LENGTH); /* save old working directory */
  if(shapepath) chdir(shapepath);

  /* initialize a few things */
  shpfile->status = NULL;
  shpfile->lastshape = -1;

  /* open the shapefile file (appending ok) and get basic info */
  if(!mode) 	
    shpfile->hSHP = msSHPOpen(filename, "rb");
  else
    shpfile->hSHP = msSHPOpen(filename, mode);

  if(!shpfile->hSHP) {
    sprintf(ms_error.message, "(%s)", filename);
    msSetError(MS_IOERR, ms_error.message, "msOpenSHPFile()");    
    chdir(old_path); /* restore old cwd */
    return(-1);
  }

  if((*filename == '\\') || (*filename == '/')) { /* already full path */
    strcpy(shpfile->source, filename);
  } else {
    getcwd(shpfile->source, MS_PATH_LENGTH); /* save the source (fullpath) information */
    strcat(shpfile->source, "/");
    strcat(shpfile->source, filename);
  }

  /* load some information about this shapefile */
  msSHPGetInfo( shpfile->hSHP, &shpfile->numshapes, &shpfile->type);
  msSHPReadBounds( shpfile->hSHP, -1, &(shpfile->bounds));
  
  dbfFilename = (char *)malloc(strlen(filename)+5);
  strcpy(dbfFilename, filename);
  
  /* clean off any extention the filename might have */
  for (i = strlen(dbfFilename) - 1; 
       i > 0 && dbfFilename[i] != '.' && dbfFilename[i] != '/' && dbfFilename[i] != '\\';
       i-- ) {}

  if( dbfFilename[i] == '.' )
    dbfFilename[i] = '\0';
  
  strcat(dbfFilename, ".dbf");

  shpfile->hDBF = msDBFOpen(dbfFilename, "rb");
  free(dbfFilename);

  if(!shpfile->hDBF) {
    sprintf(ms_error.message, "(%s)", dbfFilename);
    msSetError(MS_IOERR, ms_error.message, "msOpenSHPFile()");    
    chdir(old_path); /* restore old cwd */
    return(-1);
  }
  
  chdir(old_path); /* restore old cwd */

  return(0); /* all o.k. */
}

// Creates a new shapefile
int msSHPCreateFile(shapefileObj *shpfile, char *filename, int type)
{
  if(type != MS_SHP_POINT && type != MS_SHP_MULTIPOINT && type != MS_SHP_ARC && type != MS_SHP_POLYGON) {
    msSetError(MS_SHPERR, "Invalid shape type.", "msNewSHPFile()");
    return(-1);
  }

  // create the spatial portion
  shpfile->hSHP = msSHPCreate(filename, type);
  if(!shpfile->hSHP) {
    sprintf(ms_error.message, "(%s)", filename);
    msSetError(MS_IOERR, NULL, "msNewSHPFile()");    
    return(-1);
  }

  // retrieve a few things about this shapefile
  msSHPGetInfo( shpfile->hSHP, &shpfile->numshapes, &shpfile->type);
  msSHPReadBounds( shpfile->hSHP, -1, &(shpfile->bounds));

  shpfile->hDBF = NULL; // XBase file is NOT created here...
  return(0);
}

void msSHPCloseFile(shapefileObj *shpfile)
{
  if(shpfile->hSHP) msSHPClose(shpfile->hSHP);
  if(shpfile->hDBF) msDBFClose(shpfile->hDBF);
  if(shpfile->status) free(shpfile->status);
}

char *msSHPWhichShapes(shapefileObj *shpfile, rectObj rect)
{
  int i;
  rectObj shaperect;
  char *status=NULL;
  char *filename;

  // FIX: need an overlap test here (i.e. rect and shapefile DON"T overlap)...

  if(msRectContained(&shpfile->bounds, &rect) == MS_TRUE) {
    status = msAllocBitArray(shpfile->numshapes);
    if(!status) {
      msSetError(MS_MEMERR, NULL, "msSHPWhichShapes()");
      return(NULL);
    }
    for(i=0;i<shpfile->numshapes;i++) 
      msSetBit(status, i, 1);
  } else {
    if((filename = (char *)malloc(strlen(shpfile->source)+strlen(MS_INDEX_EXTENSION)+1)) == NULL) {
      msSetError(MS_MEMERR, NULL, "msSHPWhichShapes()");    
      return(NULL);
    }
    sprintf(filename, "%s%s", shpfile->source, MS_INDEX_EXTENSION);
    
    status = msSearchDiskTree(filename, rect);
    free(filename);

    if(status) // index 
      msFilterTreeSearch(shpfile, status, rect);
    else { // no index 
      status = msAllocBitArray(shpfile->numshapes);
      if(!status) {
	msSetError(MS_MEMERR, NULL, "msSHPWhichShapes()");       
	return(NULL);
      }
      
      for(i=0;i<shpfile->numshapes;i++) {
	if(!msSHPReadBounds(shpfile->hSHP, i, &shaperect))
	  if(msRectOverlap(&shaperect, &rect) == MS_TRUE)
	    msSetBit(status, i, 1);
      }
    }   
  }
 
  return(status); /* success */
}

int msTiledSHPOpenFile(layerObj *layer, char *shapepath)
{
  if(msSHPOpenFile(&(layer->tileshpfile), "rb", shapepath, layer->tileindex) == -1) return(MS_FAILURE);
  if((layer->tileitemindex = msDBFGetItemIndex(layer->tileshpfile.hDBF, layer->tileitem)) == -1) return(MS_FAILURE);
 
  return(MS_SUCCESS);
}

int msTiledSHPWhichShapes(layerObj *layer, char *shapepath, rectObj rect)
{
  int i;
  char *filename, tilename[MS_PATH_LENGTH];

  layer->tileshpfile.status = msSHPWhichShapes(&(layer->tileshpfile), rect);
  if(!(layer->tileshpfile.status)) return(MS_FAILURE);

  // position the source at the FIRST shapefile
  for(i=0; i<layer->tileshpfile.numshapes; i++) {
    if(msGetBit(layer->tileshpfile.status,i)) {
      if(!layer->data) // assume whole filename is in attribute field
	filename = msDBFReadStringAttribute(layer->tileshpfile.hDBF, i, layer->tileitemindex);
      else {  
	sprintf(tilename,"%s/%s", msDBFReadStringAttribute(layer->tileshpfile.hDBF, i, layer->tileitemindex) , layer->data);
	filename = tilename;
      }
      
      if(strlen(filename) == 0) continue; // check again
      
      // open the shapefile
#ifndef IGNORE_MISSING_DATA
      if(msSHPOpenFile(&(layer->shpfile), "rb", shapepath, filename) == -1) return(MS_FAILURE);
#else
      if(msSHPOpenFile(&(layer->shpfile), "rb", shapepath, filename) == -1) continue; // check again
#endif
      
      layer->tileshpfile.lastshape = i;
      break;
    }
  }

  layer->shpfile.status = msSHPWhichShapes(&(layer->shpfile), rect);
  if(!(layer->shpfile.status)) return(MS_FAILURE);

  return(MS_SUCCESS);
}

int msTiledSHPNextShape(layerObj *layer, char *shapepath, shapeObj *shape) 
{
  int i;
  char **values=NULL;
  
  i = layer->shpfile.lastshape + 1;
  while(i<layer->shpfile.numshapes && !msGetBit(layer->shpfile.status,i)) i++; // next "in" shape

  if(i == layer->shpfile.numshapes) { // next tile
    layer->shpfile.lastshape = -1;    
    msSHPCloseFile(&(layer->shpfile));

    i = layer->tileshpfile.lastshape + 1;    
    while(i<layer->tileshpfile.numshapes && !msGetBit(layer->tileshpfile.status,i)) i++; // next "in" tile
    if(i == layer->tileshpfile.numshapes) return(MS_DONE); // no more tiles

    layer->tileshpfile.lastshape = i;
    
    // more to it here!
    
    return(msTiledSHPNextShape(layer, shapepath, shape)); // next shape
  }

  layer->shpfile.lastshape = i;

  if(layer->numitems > 0) {
    values = msDBFGetValueList(layer->shpfile.hDBF, i, layer->items, &(layer->itemindexes), layer->numitems);
    if(!values) return(MS_FAILURE);

    if(msEvalExpression(&(layer->filter), layer->filteritemindex, values, layer->numitems) != MS_TRUE) return(msTiledSHPNextShape(layer, shapepath, shape)); // next shape
  }

  shape->tileindex = layer->tileshpfile.lastshape;

  msSHPReadShape(layer->shpfile.hSHP, i, shape); // ok to read the data now

  return(MS_SUCCESS);
}

int msTiledSHPGetShape(layerObj *layer, char *shapepath, shapeObj *shape, int tile, int record, int allitems) 
{
  if((tile < 0) || (tile >= layer->tileshpfile.numshapes)) return(MS_FAILURE);
  if(tile != layer->tileshpfile.lastshape) { // open the correct tile
    msSHPCloseFile(&(layer->shpfile));
    
    // more to it here!
    
  }

  if((record < 0) || (record >= layer->shpfile.numshapes)) return(MS_FAILURE);

  msSHPReadShape(layer->shpfile.hSHP, record, shape);
  layer->shpfile.lastshape = record;

  if(allitems == MS_TRUE) {
    if(!layer->items) { // fill the items layer variable if not already filled
      layer->numitems = msDBFGetFieldCount(layer->shpfile.hDBF);
      layer->items = msDBFGetItems(layer->shpfile.hDBF);
      if(!layer->items) return(MS_FAILURE);
    }
    shape->attributes = msDBFGetValues(layer->shpfile.hDBF, record);
    if(!shape->attributes) return(MS_FAILURE);
  } else if(layer->numitems > 0) {
    shape->attributes = msDBFGetValueList(layer->shpfile.hDBF, record, layer->items, &(layer->itemindexes), layer->numitems);
    if(!shape->attributes) return(MS_FAILURE);
  }

  shape->tileindex = tile;

  return(MS_SUCCESS);
}

void msTiledSHPClose(layerObj *layer) 
{  
  msSHPCloseFile(&(layer->tileshpfile));
}

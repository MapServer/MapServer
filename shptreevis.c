 /******************************************************************************
 * shptreevis
 *
 *  Utility program to visualize a quadtree
 *  New Format  
 *
 *
 */

#include "mapshape.h"
#include "maptree.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

typedef unsigned char uchar;

#ifdef SHPT_POLYGON
   #undef MAPSERVER
#else
   #define MAPSERVER 1
   #define SHPT_POLYGON MS_SHP_POLYGON
#endif


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

char* AddFileSuffix ( const char * Filename, const char * Suffix ) {
    char	*pszFullname, *pszBasename;
    int	i;
      
  /* -------------------------------------------------------------------- */
  /*	Compute the base (layer) name.  If there is any extension	    */
  /*	on the passed in filename we will strip it off.			    */
  /* -------------------------------------------------------------------- */
    pszBasename = (char *) malloc(strlen(Filename)+5);
    strcpy( pszBasename, Filename );
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
    sprintf( pszFullname, "%s%s", pszBasename, Suffix); 
      
    return (pszFullname);
}


int main( int argc, char ** argv )

{
    SHPHandle	hSHP;
    DBFHandle   hDBF;
    FILE	*qix;
    
    int		i, j;
    char	*myfile = NULL;

#ifdef MAPSERVER
    shapeObj	shape;
    lineObj	line[3];
    pointObj	pts[6];
    rectObj	rect;
#else
    SHPObject	*shape;
    double	rect[4], X[6], Y[6];
#endif   
    int		maxDepth, numShapes, pos;
    int		ids, result;

    unsigned int 	offset;
    unsigned int	nShapes,nSubNodes;
    int		res;
    char	mBigEndian;
    char	pabyBuf[64];
    char	needswap,version; 
    char	signature[3];

    int		this_rec, factor;

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc <= 2 )
    {
	printf( "shptreevis shapefile new_shapefile \n" );
	exit( 1 );
    }

    i = 1;
    if( *((unsigned char *) &i) == 1 )
      mBigEndian = 0;
    else
      mBigEndian = 1;


    qix = fopen( AddFileSuffix(argv[1],".qix"), "rb" );
    if( qix == NULL )
    {
      printf("unable to open index file %s \n", argv[0]);
      exit(-1);
    }

/* -------------------------------------------------------------------- */
/*      Open the passed shapefile.                                      */
/* -------------------------------------------------------------------- */
    myfile = AddFileSuffix(argv[2],".shp");

    hSHP = SHPCreate ( myfile, SHPT_POLYGON );
    hDBF = DBFCreate (  AddFileSuffix(argv[2],".dbf") );
    if ( (!hSHP) || (!hDBF) )
    {
      printf ("create error for %s    ... exiting \n", myfile);
      exit (-1);
    }

    /* add fields to dbf */
    DBFAddField ( hDBF, "ITEMS", FTInteger, 15,0 );
    DBFAddField ( hDBF, "SUBNODES", FTInteger, 15,0 );
    DBFAddField ( hDBF, "FACTOR", FTInteger, 15,0 );
#ifndef MAPSERVER
    SHPClose ( hSHP );
    hSHP = SHPOpen ( myfile, "r+b" );
    
    DBFClose (hDBF);
    hDBF = DBFOpen ( myfile, "r+b");
#endif

    fread( pabyBuf, 8, 1, qix );
    
    memcpy( &signature, pabyBuf, 3 );
    if( strncmp(signature,"SQT",3) )
    {
      needswap = (( pabyBuf[0] == 0 ) ^ ( mBigEndian ));
  /* ---------------------------------------------------------------------- */
  /*     poor hack to see if this quadtree was created by a computer with a */
  /*     different Endian                                                   */
  /* ---------------------------------------------------------------------- */
      version = 0;
      printf ("old style index\n");
    }
    else
    {
      needswap = (( pabyBuf[0] == MS_NEW_LSB_ORDER ) ^ ( mBigEndian ));
  /* 3rd char in file is non zero if this is MSB order                     */

      if( needswap ) SwapWord( 4, pabyBuf+4 );
      memcpy( &version, pabyBuf+4, 4 );   
      printf ("new style index" );

      fread( pabyBuf, 8, 1, qix );
    }

    if( needswap ) SwapWord( 4, pabyBuf );
    memcpy( &numShapes, pabyBuf, 4 );
  
    if( needswap ) SwapWord( 4, pabyBuf+4 );
    memcpy( &maxDepth, pabyBuf+4, 4 );

    printf ("This index supports a shapefile with %d shapes, %d depth \n",numShapes, maxDepth);

/* -------------------------------------------------------------------- */
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */

    pos = ftell (qix);
    j = 0;

    while( pos )
    {
      j ++;
/*      fprintf (stderr,"box %d, at %d pos \n", j, (int) ftell(qix));
*/

      res = fread( &offset, 4, 1, qix );
      if ( res > 0 )
      {
        if ( needswap ) SwapWord ( 4, &offset );
        fread( &rect, sizeof(double)*4, 1, qix );    
        if ( needswap ) SwapWord ( 8, ((void *) &rect) );
        if ( needswap ) SwapWord ( 8, ((void *) &rect)+8 );
        if ( needswap ) SwapWord ( 8, ((void *) &rect)+16 );
        if ( needswap ) SwapWord ( 8, ((void *) &rect)+24 );
      
        fread( &nShapes, 4, 1, qix );
        if ( needswap ) SwapWord ( 4, &nShapes );
        for( i=0; i < nShapes; i++ )
        {
          fread( &ids, sizeof(int), 1, qix );
        }

        fread( &nSubNodes, 4, 1, qix );
        if ( needswap ) SwapWord ( 4, &nSubNodes );

#ifdef MAPSERVER
/*        fprintf (stderr,"%d, # %d, %d, %f,%f,%f,%f \n",offset,nShapes,nSubNodes,rect.minx, rect.miny, rect.maxx, rect.maxy);
*/

#else
/*        fprintf (stderr,"%d, # %ld, %f,%f,%f,%f \n",offset,nShapes,rect[0], rect[1], rect[2], rect[3]);
*/
#endif        
      
#ifdef SMAPSERVER
        this_rec = hDBF->nRecords - 1;
#else
        this_rec = hDBF->nRecords;	
#endif

        DBFWriteIntegerAttribute( hDBF, this_rec, 0, nShapes);
        DBFWriteIntegerAttribute( hDBF, this_rec, 1, nSubNodes);
        factor = nShapes + nSubNodes;
        DBFWriteIntegerAttribute( hDBF, this_rec, 2, factor);  
        
#ifdef  MAPSERVER
	shape.numlines = 1;
	shape.type = SHPT_POLYGON;
	((pointObj) pts[0]).x = rect.minx;  ((pointObj) pts[0]).y = rect.miny;
	((pointObj) pts[1]).x = rect.maxx;  ((pointObj) pts[1]).y = rect.miny;
	((pointObj) pts[2]).x = rect.maxx;  ((pointObj) pts[2]).y = rect.maxy;
	((pointObj) pts[3]).x = rect.minx;  ((pointObj) pts[3]).y = rect.maxy;
	((pointObj) pts[4]).x = rect.minx;  ((pointObj) pts[4]).y = rect.miny;
	line[0].numpoints = 5;
	line[0].point = &pts[0];
	shape.line = &line[0];
	shape.bounds = rect;
	
	result = SHPWriteShape ( hSHP, &shape );
	if ( result < 0 )
	{ 
	  printf ("unable to write shape \n");  
	  exit (0);
	}

	
#else
        X[0] = rect[0];
        X[1] = rect[2];
        X[2] = rect[2];
        X[3] = rect[0];
        X[4] = rect[0];

        Y[0] = rect[1];
        Y[1] = rect[1];
        Y[2] = rect[3];
        Y[3] = rect[3];
        Y[4] = rect[1];
        
        shape = SHPCreateSimpleObject( SHPT_POLYGON, 5, X, Y, NULL);
        SHPWriteObject(hSHP, -1, shape); 
        SHPDestroyObject ( shape );
#endif
        }
        else 
        { pos = 0; }
    }
    
/*    printf ("read entire file now pos %ld\n", ftell (hSHP->fpSHP));
*/
    SHPClose( hSHP );
    DBFClose( hDBF );
    fclose (qix);
    
    return(0);
}

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
    SHPTreeHandle	qix;
    
    int		i, j;
    char	*myfile = NULL;

    treeNodeObj *node;    

#ifdef MAPSERVER
    shapeObj	shape;
    lineObj	line[3];
    pointObj	pts[6];
#else
    SHPObject	*shape;
    double	X[6], Y[6];
#endif   
    int		pos, result;
    char	mBigEndian;
    char	pabyBuf[64];

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


    qix = msSHPDiskTreeOpen (AddFileSuffix(argv[1],".qix"));
    if( qix == NULL )
    {
      printf("unable to open index file %s \n", argv[1]);
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

    printf ("This %s %s index supports a shapefile with %d shapes, %d depth \n",
	(qix->version ? "new": "old"), (qix->LSB_order? "LSB": "MSB"), qix->nShapes, qix->nDepth);


/* -------------------------------------------------------------------- */
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */

    pos = ftell (qix->fp);
    j = 0;

    while( pos && (j < qix->nShapes) )
    {
      j ++;

      node = readTreeNode (qix);
      if (node )
      {

#ifdef MAPSERVER
        this_rec = hDBF->nRecords - 1;
#else
        this_rec = hDBF->nRecords;	
/*      Shapelib currently works this way
 *      Map server is based on an old version of shapelib      
 */
#endif

        DBFWriteIntegerAttribute( hDBF, this_rec, 0, node->numshapes);
        DBFWriteIntegerAttribute( hDBF, this_rec, 1, node->numsubnodes);
        factor = node->numshapes + node->numsubnodes;
        
#ifdef  MAPSERVER
	shape.numlines = 1;
	shape.type = SHPT_POLYGON;
	((pointObj) pts[0]).x = node->rect.minx;  ((pointObj) pts[0]).y = node->rect.miny;
	((pointObj) pts[1]).x = node->rect.maxx;  ((pointObj) pts[1]).y = node->rect.miny;
	((pointObj) pts[2]).x = node->rect.maxx;  ((pointObj) pts[2]).y = node->rect.maxy;
	((pointObj) pts[3]).x = node->rect.minx;  ((pointObj) pts[3]).y = node->rect.maxy;
	((pointObj) pts[4]).x = node->rect.minx;  ((pointObj) pts[4]).y = node->rect.miny;
	line[0].numpoints = 5;
	line[0].point = &pts[0];
	shape.line = &line[0];
	shape.bounds = node->rect;
	
	result = SHPWriteShape ( hSHP, &shape );
	if ( result < 0 )
	{ 
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
        }
        else 
        { pos = 0; }
    }
    
    SHPClose( hSHP );
    DBFClose( hDBF );
    msSHPDiskTreeClose (qix);    
    
    return(0);
}

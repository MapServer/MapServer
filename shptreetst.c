 /******************************************************************************
 * shptreetst
 *
 *  Utility program to visualize a quadtree search
 *  New Format  
 *
 *
 */

#include "mapshape.h"
#include "maptree.h"
#include "map.h"
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
    SHPTreeHandle	qix;
    
    int		i, j;
    rectObj	rect;
 
    int		pos;
    char	*bitmap = NULL;
     
    char	mBigEndian;
    treeNodeObj *node;


/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc <= 1 )
    {
	printf( "shptreetst shapefile {minx miny maxx maxy}\n" );
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

    printf ("This %s %s index supports a shapefile with %d shapes, %d depth \n",
	(qix->version ? "new": "old"), (qix->LSB_order? "LSB": "MSB"), qix->nShapes, qix->nDepth);

/* -------------------------------------------------------------------- */
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */

    pos = ftell (qix->fp);
    j = 0;

    while( pos && j < 20)
    {
      j ++;
/*      fprintf (stderr,"box %d, at %d pos \n", j, (int) ftell(qix));
*/

      node = readTreeNode (qix);
      if (node )
      {
        fprintf (stdout,"shapes %d, node %d, %f,%f,%f,%f \n",node->numshapes,node->numsubnodes,node->rect.minx, node->rect.miny, node->rect.maxx, node->rect.maxy);
            
      }
      else 
      { pos = 0; }
    }
    
    printf ("read entire file now at quad box rec %d file pos %ld\n", j, ftell (qix->fp));

    j = qix->nShapes;
    msSHPDiskTreeClose (qix);
    
    if( argc >= 5 )
    {
      rect.minx = atof (argv[2]);
      rect.miny = atof (argv[3]);
      rect.maxx = atof (argv[4]);
      rect.maxy = atof (argv[5]);
    }
    else
    {
      printf ("using last read box as a search \n");
      rect.minx =  node->rect.minx;
      rect.miny =  node->rect.miny;
      rect.maxx =  node->rect.maxx;
      rect.maxy =  node->rect.maxy;
    }
    
    bitmap = msSearchDiskTree( argv[1], rect );

    if ( bitmap )
    {
      printf ("result of rectangle search was \n");
      for ( i=0; i<j; i++)
      {
        if ( msGetBit(bitmap,i) )
        {
          printf(" %d,",i); 
        }
      }
    }
    printf("\n");



    return(0);
}

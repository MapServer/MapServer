#include "map.h"
#include "maptree.h"
#include <string.h>

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


int main(int argc, char *argv[])
{
  shapefileObj shapefile;

  char *filename;
  treeObj *tree;
  int byte_order;
  int depth=0;

  if(argc<2) {
   fprintf(stdout,"Syntax: shptree [shpfile] {depth} {N | L | M | NL | NM}\n" );
   fprintf(stdout,"     N:  Native byte order\n");
   fprintf(stdout,"     L:  LSB (intel) byte order\n");
   fprintf(stdout,"     M:  MSB byte order\n");
   fprintf(stdout,"     NL: LSB byte order, using new index format\n");
   fprintf(stdout,"     NM: MSB byte order, using new index format\n\n");
   exit(0);
  }

  if(argc >= 3)
    depth = atoi(argv[2]);

  if(argc >= 4)
  {
    if( !strcasecmp(argv[3],"N" ))
      byte_order = MS_NATIVE_ORDER; 
    if( !strcasecmp(argv[3],"L" ))
      byte_order = MS_LSB_ORDER; 
    if( !strcasecmp(argv[3],"M" ))
      byte_order = MS_MSB_ORDER; 
    if( !strcasecmp(argv[3],"NL" ))
      byte_order = MS_NEW_LSB_ORDER; 
    if( !strcasecmp(argv[3],"NM" ))
      byte_order = MS_NEW_MSB_ORDER; 
  }
  else
  { byte_order = MS_NATIVE_ORDER; }
    
  if(msOpenSHPFile(&shapefile, "rb", NULL, NULL, argv[1]) == -1) {
    fprintf(stdout, "Error opening shapefile %s.\n", argv[1]);
    exit(0);
  }

  printf( "creating index of %s %s format\n",(byte_order < 1 ? "old" :"new"), 
     ((byte_order == MS_NATIVE_ORDER) ? "native" : 
     ((byte_order == MS_LSB_ORDER) || (byte_order == MS_NEW_LSB_ORDER)? " LSB":"MSB")));

  tree = msCreateTree(&shapefile, depth);
  if(!tree) {
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
  msCloseSHPFile(&shapefile);
  free(filename);

  exit(0);
}





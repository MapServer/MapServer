#include "map.h"
#include "maptree.h"

int main(int argc, char *argv[])
{
  shapefileObj shapefile;

  char *filename;
  treeObj *tree;

  int depth=0;

  if(argc<2) {
   fprintf(stdout,"Syntax: shptree [shpfile] {depth}\n" );
   exit(0);
  }

  if(argc == 3)
    depth = atoi(argv[2]);

  if(msOpenSHPFile(&shapefile, NULL, NULL, argv[1]) == -1) {
    fprintf(stdout, "Error opening shapefile %s.\n", argv[1]);
    exit(0);
  }

  tree = msCreateTree(&shapefile, depth);
  if(!tree) {
#if MAX_SUBNODE == 2
    fprintf(stdout, "Error generating binary tree.\n");
#else
    fprintf(stdout, "Error generating quadtree.\n");
#endif
    exit(0);
  }

  filename = (char *)malloc(strlen(argv[1])+strlen(MS_INDEX_EXTENSION)+1);
  sprintf(filename, "%s%s", argv[1], MS_INDEX_EXTENSION);
  msWriteTree(tree, filename);
  msDestroyTree(tree);

  /*
  ** Clean things up
  */
  msCloseSHPFile(&shapefile);
  free(filename);

  exit(0);
}





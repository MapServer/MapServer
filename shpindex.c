#include "map.h"
#include "mapindex.h"

int main(int argc, char *argv[])
{
  SHPHandle hSHP;
  int nShapeType;
  int nEntities;
  rectObj shpBounds;
  int i;

  char *filename;
  indexObj *idx;

  if(argc<4) {
   fprintf(stdout,"Syntax: shpindex [shpfile] [sx] [sy]\n" );
   exit(0);
  }
  
  /*
  ** Open shapefile
  */
  if((hSHP = SHPOpen(argv[1], "rb" )) == NULL) {
    fprintf(stderr, "Error opening shapefile %s.\n", argv[1]);
    exit(0);
  }

  /* 
  ** Load some information about this shapefile, don't need much
  */
  SHPGetInfo(hSHP, &nEntities, &nShapeType);
  SHPReadBounds(hSHP, -1, &shpBounds);

  /*
  ** Initialize the index
  */
  if((idx = msNewIndex(shpBounds, atoi(argv[2]), atoi(argv[3]), nEntities)) == NULL) {
    msWriteError(stderr);
    exit(0);
  }

  /* 
  ** For each shape 
  */
  for(i=0;i<nEntities;i++) {
    SHPReadBounds(hSHP, i, &shpBounds); /* grab the rectangle for this shape */

    if(msInsertRect(idx, i, shpBounds) == -1) { /* add it to the index */
      msWriteError(stderr);
      msCloseIndex(idx);
      exit(0);
    }
  }

  filename = (char *)malloc(strlen(argv[1])+strlen(MS_INDEX_EXTENSION)+1);
  sprintf(filename, "%s%s", argv[1], MS_INDEX_EXTENSION);
  msWriteIndex(idx, filename);
  msCloseIndex(idx);

  /*
  ** Clean things up
  */
  SHPClose(hSHP);
  free(filename);

  exit(0);
}





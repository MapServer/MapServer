#include "map.h"

int main(int argc, char *argv[])
{
  mapObj *map=NULL;
  imageObj         *image = NULL;

  if(argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  /* ---- check the number of arguments, return syntax if not correct ---- */
  if( argc < 3 ) {
      fprintf(stdout,"Syntax: scalebar [mapfile] [output image]\n" );
      exit(0);
  }

  map = msLoadMap(argv[1], NULL);
  if(!map) { 
    msWriteError(stderr);
    exit(0);
  }

  image = msDrawScalebar(map);

  if(!image) { 
    msWriteError(stderr);
    exit(0);
  }

  msSaveImage(NULL, image, argv[2]);
  msFreeImage(image);

  msFreeMap(map);  
  return(MS_TRUE);
}

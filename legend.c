#include "map.h"

int main(int argc, char *argv[])
{
  mapObj *map=NULL;
  imageObj *img=NULL;

  if(argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  if( argc < 3 ) {
      fprintf(stdout,"Syntax: legend [mapfile] [output image]\n" );
      exit(0);
  }

  map = msLoadMap(argv[1], NULL);
  if(!map) {
    msWriteError(stderr);
    exit(0);
  }

  img = msDrawLegend(map);
  if(!img) {
    msWriteError(stderr);
    exit(0);
  }

  msSaveImage(NULL, img, argv[2]);

  msFreeImage(img);
  msFreeMap(map);
  
  return(MS_TRUE);
}

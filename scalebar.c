#include "map.h"

int main(int argc, char *argv[])
{
  mapObj *map=NULL;
  gdImagePtr img=NULL;

  if(argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  /* ---- check the number of arguments, return syntax if not correct ---- */
  if( argc < 3 ) {
      fprintf(stdout,"Syntax: scalebar [mapfile] [output image]\n" );
      exit(0);
  }

  map = msLoadMap(argv[1]);
  if(!map) { 
    msWriteError(stderr);
    exit(0);
  }

  img = msDrawScalebar(map);
  if(!img) { 
    msWriteError(stderr);
    exit(0);
  }

#ifndef USE_GD_1_8
  msSaveImage(img, argv[2], map->scalebar.transparent, map->scalebar.interlace);
#else
  msSaveImage(img, argv[2], map->imagetype, map->scalebar.transparent, map->scalebar.interlace, map->imagequality);
#endif

  gdImageDestroy(img);

  msFreeMap(map);  
  return(MS_TRUE);
}

#include "map.h"

int main(int argc, char *argv[])
{
  int i,j,k;

  mapObj    	   *map=NULL;
  imageObj         *image = NULL;
     
  char **layers=NULL;
  int num_layers=0;

  char *outfile=NULL; // no -o sends image to STDOUT

  if(argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  /* ---- check the number of arguments, return syntax if not correct ---- */
  if( argc < 3 ) {
    fprintf(stdout,
            "Syntax: shp2img -m [mapfile] -o [image] -e minx miny maxx maxy\n"
            "                -t -l [layers] -i [format]\n");

    fprintf(stdout,"  -m mapfile: Map file to operate on - required.\n" );
    fprintf(stdout,"  -i format: Override the IMAGETYPE value to pick output format.\n" );
    fprintf(stdout,"  -t: enable transparency\n" );
    fprintf(stdout,"  -o image: output filename (stdout if not provided)\n");
    fprintf(stdout,"  -e minx miny maxx maxy: extents to render - optional\n");
    fprintf(stdout,"  -l layers: layers to enable - optional\n" );
    fprintf(stdout,"  -all_debug n: Set debug level for map and all layers.\n" );
    fprintf(stdout,"  -map_debug n: Set map debug level.\n" );
    fprintf(stdout,"  -layer_debug layer_name n: Set layer debug level.\n" );
    exit(0);
  }
  
  for(i=1;i<argc;i++) { /* Step though the user arguments, 1st to find map file */
 
    if(strncmp(argv[i],"-m",2) == 0) {
      map = msLoadMap(argv[i+1], NULL);
      if(!map) {
	msWriteError(stderr);
	exit(0);
      }
    }
  }
  
  if(!map) {
    fprintf(stderr, "Mapfile (-m) option not specified.\n");
    exit(0);
  }

  for(i=1;i<argc;i++) { /* Step though the user arguments */

    if(strncmp(argv[i],"-m",2) == 0) { /* skip it */
      i+=1;
    }

    if(strncmp(argv[i],"-o",2) == 0) { /* load the output image filename */
      outfile = strdup(argv[i+1]);
      i+=1;
    }

    if(strncmp(argv[i],"-i",2) == 0) { 
      outputFormatObj *format;

      format = msSelectOutputFormat( map, argv[i+1] );

      if( format == NULL )
          printf( "No such OUTPUTFORMAT as %s.\n", argv[i+1] );
      else
      {
          msFree( (char *) map->imagetype );
          map->imagetype = strdup( argv[i+1] );
          msApplyOutputFormat( &(map->outputformat), format, MS_NOOVERRIDE,
                               MS_NOOVERRIDE, MS_NOOVERRIDE );
      }
      i+=1;
    }

    if(strncmp(argv[i],"-d",2) == 0) { /* swap layer data */
      for(j=0; j<map->numlayers; j++) {
	 if(strcmp(map->layers[j].name, argv[i+1]) == 0) {
	   free(map->layers[j].data);
	   map->layers[j].data = strdup(argv[i+2]);
	   break;
	 }
      }
      i+=2;
    }

    if(strncmp(argv[i], "-t", 2) == 0) /* transparency */
      map->transparent = MS_ON;
    
    if(strncmp(argv[i], "-all_debug", 10) == 0) /* debug */
    {
        int debug_level = atoi(argv[++i]);

        map->debug = debug_level;
        for(j=0; j<map->numlayers; j++) {
            map->layers[j].debug = debug_level;
        }
    }
    
    if(strncmp(argv[i], "-map_debug", 10) == 0) /* debug */
    {
        map->debug = atoi(argv[++i]);
    }
    
    if(strncmp(argv[i], "-layer_debug", 12) == 0) /* debug */
    {
        const char *layer_name = argv[++i];
        int debug_level = atoi(argv[++i]);
        int got_layer = 0;

        for(j=0; j<map->numlayers; j++) {
            if(strcmp(map->layers[j].name,layer_name) == 0 ) {
                map->layers[j].debug = debug_level;
                got_layer = 1;
            }
        }
        if( !got_layer )
            fprintf( stderr, 
                     " Did not find layer '%s' from -layer_debug switch.\n", 
                     layer_name );
    }
    
    if(strncmp(argv[i],"-e",2) == 0) { /* change extent */
      map->extent.minx = atof(argv[i+1]);
      map->extent.miny = atof(argv[i+2]);
      map->extent.maxx = atof(argv[i+3]);
      map->extent.maxy = atof(argv[i+4]);
      i+=4;
    }

    if(strncmp(argv[i],"-l",2) == 0) { /* load layer list */
      layers = split(argv[i+1], ' ', &(num_layers));

      for(j=0; j<map->numlayers; j++) {
	if(map->layers[j].status == MS_DEFAULT)
	  continue;
	else {
	  map->layers[j].status = MS_OFF;
	  for(k=0; k<num_layers; k++) {
	    if(strcmp(map->layers[j].name, layers[k]) == 0) {
	      map->layers[j].status = MS_ON;
	      break;
	    }
	  }
	}
      }
      msFreeCharArray(layers, num_layers);

      i+=1;
     }
  }

  image = msDrawMap(map);

  if(!image) {
    msWriteError(stderr);
    exit(0);
  }

  if( msSaveImage(map, image, outfile) != MS_SUCCESS ) {
    msWriteError(stderr);
    exit(0);
  }
  msFreeImage(image);

  msFreeMap(map);
  free(outfile);


  msCleanup();

  return(0);
} /* ---- END Main Routine ---- */

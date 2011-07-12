/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Commandline .map rendering utility, mostly for testing.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.20  2006/05/12 20:59:07  frank
 * Removed -t switch, it doesn't work.
 *
 * Revision 1.19  2006/02/10 05:51:45  sdlime
 * Applied patch to resolve issue with layers with no names if -l option is specified. (bug 1655)
 *
 * Revision 1.18  2006/02/01 04:08:35  sdlime
 * Converted strncmp to strcmp in shp2img.c to avoid parameter confusion and for consistency. (bug 1635)
 *
 * Revision 1.17  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.16  2005/02/18 03:06:48  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.15  2004/11/05 02:28:39  frank
 * Reorganize error cases a bit to cleanup properly so we can test error
 * logic leak issues, such as in bug 139.
 *
 * Revision 1.14  2004/11/04 21:08:45  frank
 * added -p flag
 *
 * Revision 1.13  2004/10/28 02:14:16  frank
 * Also enable MS_ERRORFILE if -all_debug selected.
 *
 * Revision 1.12  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"

MS_CVSID("$Id$")

int main(int argc, char *argv[])
{
  int i,j,k;

  mapObj    	   *map=NULL;
  imageObj         *image = NULL;
     
  char **layers=NULL;
  int num_layers=0;

  char *outfile=NULL; /* no -o sends image to STDOUT */

  if(argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  /* ---- check the number of arguments, return syntax if not correct ---- */
  if( argc < 3 ) {
    fprintf(stdout,
            "Syntax: shp2img -m [mapfile] -o [image] -e minx miny maxx maxy\n"
            "                -l [layers] -i [format]\n");

    fprintf(stdout,"  -m mapfile: Map file to operate on - required.\n" );
    fprintf(stdout,"  -i format: Override the IMAGETYPE value to pick output format.\n" );
    fprintf(stdout,"  -o image: output filename (stdout if not provided)\n");
    fprintf(stdout,"  -e minx miny maxx maxy: extents to render - optional\n");
    fprintf(stdout,"  -l layers: layers to enable - optional\n" );
    fprintf(stdout,"  -all_debug n: Set debug level for map and all layers.\n" );
    fprintf(stdout,"  -map_debug n: Set map debug level.\n" );
    fprintf(stdout,"  -layer_debug layer_name n: Set layer debug level.\n" );
    fprintf(stdout,"  -p n: pause for n seconds after reading the map\n" );

    exit(0);
  }
  
  for(i=1;i<argc;i++) { /* Step though the user arguments, 1st to find map file */
 
    if(strcmp(argv[i],"-m") == 0) {
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

    if(strcmp(argv[i],"-m") == 0) { /* skip it */
      i+=1;
    }

    if(strcmp(argv[i],"-p") == 0) {
        int pause_length = atoi(argv[i+1]);
        time_t start_time = time(NULL);

        printf( "Start pause of %d seconds.\n", pause_length );
        while( time(NULL) < start_time + pause_length ) {}
        printf( "Done pause.\n" );
            
        i+=1;
    }

    if(strcmp(argv[i],"-o") == 0) { /* load the output image filename */
      outfile = argv[i+1];
      i+=1;
    }

    if(strcmp(argv[i],"-i") == 0) { 
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

    if(strcmp(argv[i],"-d") == 0) { /* swap layer data */
      for(j=0; j<map->numlayers; j++) {
	 if(strcmp(map->layers[j].name, argv[i+1]) == 0) {
	   free(map->layers[j].data);
	   map->layers[j].data = strdup(argv[i+2]);
	   break;
	 }
      }
      i+=2;
    }

    if(strcmp(argv[i], "-all_debug") == 0) /* debug */
    {
        int debug_level = atoi(argv[++i]);

        map->debug = debug_level;
        for(j=0; j<map->numlayers; j++) {
            map->layers[j].debug = debug_level;
        }
        if( getenv( "MS_ERRORFILE" ) == NULL )
            putenv( "MS_ERRORFILE=stderr" );
    }
    
    if(strcmp(argv[i], "-map_debug") == 0) /* debug */
    {
        map->debug = atoi(argv[++i]);
    }
    
    if(strcmp(argv[i], "-layer_debug") == 0) /* debug */
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
    
    if(strcmp(argv[i],"-e") == 0) { /* change extent */
      map->extent.minx = atof(argv[i+1]);
      map->extent.miny = atof(argv[i+2]);
      map->extent.maxx = atof(argv[i+3]);
      map->extent.maxy = atof(argv[i+4]);
      i+=4;
    }

    if(strcmp(argv[i],"-l") == 0) { /* load layer list */
      layers = split(argv[i+1], ' ', &(num_layers));

      for(j=0; j<map->numlayers; j++) {
	if(map->layers[j].status == MS_DEFAULT)
	  continue;
	else {
	  map->layers[j].status = MS_OFF;
	  for(k=0; k<num_layers; k++) {
	    if(map->layers[j].name && strcmp(map->layers[j].name, layers[k]) == 0) {
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

    msFreeMap(map);
    msCleanup();
    exit(0);
  }

  if( msSaveImage(map, image, outfile) != MS_SUCCESS ) {
    msWriteError(stderr);
  }

  msFreeImage(image);
  msFreeMap(map);
  msCleanup();

  return(0);
} /* ---- END Main Routine ---- */

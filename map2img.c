/******************************************************************************
 * $Id$
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
 ****************************************************************************/

#include <stdbool.h>

#include "mapserver.h"
#include "maptime.h"

#include "limits.h"

int main(int argc, char *argv[])
{
  int i,j,k;

  mapObj         *map=NULL;
  imageObj         *image = NULL;
  configObj* config = NULL;

  char **layers=NULL;
  int num_layers=0;

  int layer_found=0;

  char *outfile=NULL; /* no -o sends image to STDOUT */

  int iterations = 1;
  int draws = 0;

  /* ---- output version info and exit --- */
  if(argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }
  
  /* ---- check the number of arguments, return syntax if not correct ---- */
  if( argc < 3 ) {
    fprintf(stdout, "\nPurpose: convert a mapfile to an image\n\n");
    fprintf(stdout,
            "Syntax: map2img -m mapfile [-o image] [-e minx miny maxx maxy] [-s sizex sizey]\n"
            "               [-l \"layer1 [layers2...]\"] [-i format]\n"
            "               [-all_debug n] [-map_debug n] [-layer_debug n] [-p n] [-c n] [-d layername datavalue]\n"
            "               [-conf filename]\n");
    fprintf(stdout,"  -m mapfile: Map file to operate on - required\n" );
    fprintf(stdout,"  -i format: Override the IMAGETYPE value to pick output format\n" );
    fprintf(stdout,"  -o image: output filename (stdout if not provided)\n");
    fprintf(stdout,"  -e minx miny maxx maxy: extents to render\n");
    fprintf(stdout,"  -s sizex sizey: output image size\n");
    fprintf(stdout,"  -l layers: layers / groups to enable - make sure they are quoted and space separated if more than one listed\n" );
    fprintf(stdout,"  -all_debug n: Set debug level for map and all layers\n" );
    fprintf(stdout,"  -map_debug n: Set map debug level\n" );
    fprintf(stdout,"  -layer_debug layer_name n: Set layer debug level\n" );
    fprintf(stdout,"  -c n: draw map n number of times\n" );
    fprintf(stdout,"  -p n: pause for n seconds after reading the map\n" );
    fprintf(stdout,"  -d layername datavalue: change DATA value for layer\n" );
    fprintf(stdout,"  -conf filename: filename of the MapServer configuration file.\n" );
    exit(0);
  }

  if ( msSetup() != MS_SUCCESS ) {
    msWriteError(stderr);
    exit(1);
  }

  bool some_debug_requested = FALSE;
  const char* config_filename = NULL;
  for(i=1; i<argc; i++) {
    if (strcmp(argv[i],"-c") == 0) { /* user specified number of draws */
      iterations = atoi(argv[i+1]);
      if( iterations < 0 || iterations > INT_MAX - 1 ) {
        printf("Invalid number of iterations");
        return 1;
      }
      printf("We will draw %d times...\n", iterations);
      continue;
    }

    if(i < argc-1 && strcmp(argv[i], "-all_debug") == 0) { /* global debug */
      int debug_level = atoi(argv[++i]);

      some_debug_requested = TRUE;

      msSetGlobalDebugLevel(debug_level);

      continue;
    }

    if(i < argc-1 && (strcmp(argv[i], "-map_debug") == 0 ||
                      strcmp(argv[i], "-layer_debug") == 0)) {

      some_debug_requested = TRUE;
      continue;
    }

    if(i < argc-1 && strcmp(argv[i], "-conf") == 0) {
      config_filename = argv[i+1];
      ++i;
      continue;
    }
  }

  if( some_debug_requested ) {
    /* Send output to stderr by default */
    if (msGetErrorFile() == NULL)
      msSetErrorFile("stderr", NULL);
  }

  config = msLoadConfig(config_filename);

  for(draws=0; draws<iterations; draws++) {

    struct mstimeval requeststarttime, requestendtime;

    if(msGetGlobalDebugLevel() >= MS_DEBUGLEVEL_TUNING)
      msGettimeofday(&requeststarttime, NULL);

    /* Use PROJ_DATA/PROJ_LIB env vars if set */
    msProjDataInitFromEnv();

    /* Use MS_ERRORFILE and MS_DEBUGLEVEL env vars if set */
    if ( msDebugInitFromEnv() != MS_SUCCESS ) {
      msWriteError(stderr);
      msCleanup();
      msFreeConfig(config);
      exit(1);
    }



    for(i=1; i<argc; i++) { /* Step though the user arguments, 1st to find map file */

      if(strcmp(argv[i],"-m") == 0) {
        map = msLoadMap(argv[i+1], NULL, config);
        if(!map) {
          msWriteError(stderr);
          msCleanup();
          msFreeConfig(config);
          exit(1);
        }
        msApplyDefaultSubstitutions(map);
      }
    }

    if(!map) {
      fprintf(stderr, "Mapfile (-m) option not specified.\n");
      msCleanup();
      msFreeConfig(config);
      exit(1);
    }


    for(i=1; i<argc; i++) { /* Step though the user arguments */

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
        else {
          msFree( (char *) map->imagetype );
          map->imagetype = msStrdup( argv[i+1] );
          msApplyOutputFormat( &(map->outputformat), format, MS_NOOVERRIDE);
        }
        i+=1;
      }

      if(strcmp(argv[i],"-d") == 0) { /* swap layer data */
        for(j=0; j<map->numlayers; j++) {
          if(strcmp(GET_LAYER(map, j)->name, argv[i+1]) == 0) {
            free(GET_LAYER(map, j)->data);
            GET_LAYER(map, j)->data = msStrdup(argv[i+2]);
            break;
          }
        }
        i+=2;
      }

      if(i < argc-1 && strcmp(argv[i], "-all_debug") == 0) { /* global debug */
        int debug_level = atoi(argv[++i]);

        /* msSetGlobalDebugLevel() already called. Just need to force debug
         * level in map and all layers
         */
        map->debug = debug_level;
        for(j=0; j<map->numlayers; j++) {
          GET_LAYER(map, j)->debug = debug_level;
        }

      }

      if(i < argc-1 && strcmp(argv[i], "-map_debug") == 0) { /* debug */
        map->debug = atoi(argv[++i]);
      }

      if(i < argc-1 && strcmp(argv[i], "-layer_debug") == 0) { /* debug */
        const char *layer_name = argv[++i];
        int debug_level = atoi(argv[++i]);
        int got_layer = 0;

        for(j=0; j<map->numlayers; j++) {
          if(strcmp(GET_LAYER(map, j)->name,layer_name) == 0 ) {
            GET_LAYER(map, j)->debug = debug_level;
            got_layer = 1;
          }
        }
        if( !got_layer )
          fprintf( stderr,
                   " Did not find layer '%s' from -layer_debug switch.\n",
                   layer_name );
      }

      if(strcmp(argv[i],"-e") == 0) { /* change extent */
        if( argc <= i+4 ) {
          fprintf( stderr,
                   "Argument -e needs 4 space separated numbers as argument.\n" );
          msCleanup();
          msFreeConfig(config);
          exit(1);
        }
        map->extent.minx = atof(argv[i+1]);
        map->extent.miny = atof(argv[i+2]);
        map->extent.maxx = atof(argv[i+3]);
        map->extent.maxy = atof(argv[i+4]);
        i+=4;
      }

      if (strcmp(argv[i], "-s") == 0) {
        msMapSetSize(map, atoi(argv[i+1]), atoi(argv[i+2]));
        i+=2;
      }

      if(strcmp(argv[i],"-l") == 0) { /* load layer list */
        layers = msStringSplit(argv[i+1], ' ', &(num_layers));

        for(j=0; j<num_layers; j++) { /* loop over -l */
          layer_found=0;
          for(k=0; k<map->numlayers; k++) {
            if((GET_LAYER(map, k)->name && strcasecmp(GET_LAYER(map, k)->name, layers[j]) == 0) || (GET_LAYER(map, k)->group && strcasecmp(GET_LAYER(map, k)->group, layers[j]) == 0)) {
              layer_found = 1;
              break;
            }
          }
          if (layer_found==0) {
            fprintf(stderr, "Layer (-l) \"%s\" not found\n", layers[j]);
            msCleanup();
            msFreeConfig(config);
            exit(1);
          }
        }

        for(j=0; j<map->numlayers; j++) {
          if(GET_LAYER(map, j)->status == MS_DEFAULT)
            continue;
          else {
            GET_LAYER(map, j)->status = MS_OFF;
            for(k=0; k<num_layers; k++) {
              if((GET_LAYER(map, j)->name && strcasecmp(GET_LAYER(map, j)->name, layers[k]) == 0) ||
                  (GET_LAYER(map, j)->group && strcasecmp(GET_LAYER(map, j)->group, layers[k]) == 0)) {
                GET_LAYER(map, j)->status = MS_ON;
                break;
              }
            }
          }
        }

        msFreeCharArray(layers, num_layers);

        i+=1;
      }
    }

    image = msDrawMap(map, MS_FALSE);

    if(!image) {
      msWriteError(stderr);

      msFreeMap(map);
      msCleanup();
      msFreeConfig(config);
      exit(1);
    }

    if( msSaveImage(map, image, outfile) != MS_SUCCESS ) {
      msWriteError(stderr);
    }

    msFreeImage(image);
    msFreeMap(map);

    if(msGetGlobalDebugLevel() >= MS_DEBUGLEVEL_TUNING) {
      msGettimeofday(&requestendtime, NULL);
      msDebug("map2img total time: %.3fs\n",
              (requestendtime.tv_sec+requestendtime.tv_usec/1.0e6)-
              (requeststarttime.tv_sec+requeststarttime.tv_usec/1.0e6) );
    }

  } /*   for(draws=0; draws<iterations; draws++) { */
  msCleanup();
  msFreeConfig(config);
  return(0);
} /* ---- END Main Routine ---- */

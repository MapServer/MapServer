/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Commandline shape to pdf converter.
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
 * Revision 1.7  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.6  2005/02/18 03:06:48  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.5  2004/11/04 21:48:47  frank
 * Removed unused variables.
 *
 * Revision 1.4  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"

MS_CVSID("$Id$")

#ifdef USE_PDF
#include <pdflib.h>

PDF *
initializePDF(char *filename)
{
  PDF *pdf = PDF_new();
  PDF_open_file(pdf, filename);
  PDF_set_info(pdf, "Creator", "Mapserver");
  PDF_set_info(pdf, "Author", "Mapserver");
  PDF_set_info(pdf, "Title", "Mapserver PDF Map");
  return pdf;
}

void
releasePDF(PDF *pdf){

  PDF_end_page(pdf);
  PDF_close(pdf);
  PDF_delete(pdf);
}
#endif


int main(int argc, char *argv[])
{
#ifndef USE_PDF
  fprintf(stdout, "%s\n", msGetVersion());
  fprintf(stdout, "Compiled without PDF support.");
  exit(0);
#else
  int i,j,k;

  imageObj *image;
  
  mapObj *map=NULL;
  outputFormatObj *format=NULL;
  /* gdImagePtr       img=NULL; */

  char **layers=NULL;
  int num_layers=0;

  char *outfile=NULL; /* no -o sends image to STDOUT */

  /* A4 Portrait */
  int PagePixelWidth = 595;
  int PagePixelHeight = 842;
  int PagePixelMargin = 50;

  if(argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  /* ---- check the number of arguments, return syntax if not correct ---- */
  if( argc < 3 ) {
    fprintf(stdout,"Syntax: shp2pdf -m [mapfile] -o [pdf] -l [layers]\n" );
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

    if(strncmp(argv[i],"-o",2) == 0) {
      outfile = strdup(argv[i+1]);
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

    /*if(strncmp(argv[i], "-t", 2) == 0)  transparency
      map->transparent = MS_ON;*/

    if(strncmp(argv[i],"-e",2) == 0) { /* change extent */
      map->extent.minx = atof(argv[i+1]);
      map->extent.miny = atof(argv[i+2]);
      map->extent.maxx = atof(argv[i+3]);
      map->extent.maxy = atof(argv[i+4]);
      i+=4;
    }
    format = msCreateDefaultOutputFormat(map, "pdf");
    map->outputformat = format;

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

  /*img = msDrawMap(map);
  if(!img) {
    msWriteError(stderr);
    exit(0);
  }*/

  /*msSaveImage(img, outfile, map->imagetype, map->transparent, map->interlace, map->imagequality);

  gdImageDestroy(img);*/
  map->height = PagePixelHeight-(2*PagePixelMargin);
  map->width = PagePixelWidth-(2*PagePixelMargin);
  image = msDrawMap(map);
  msSaveImage(map, image, outfile);
    
  msFreeMap(map);
  free(outfile);
#endif
  return(0);
} /* ---- END Main Routine ---- */

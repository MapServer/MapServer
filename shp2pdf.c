#include "map.h"

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

  mapObj    	   *map=NULL;
  gdImagePtr       img=NULL;

  char **layers=NULL;
  int num_layers=0;

  char *outfile=NULL; // no -o sends image to STDOUT

  PDF *pdf;
  /* A4 Portrait */
  int PagePixelWidth = 595;
  int PagePixelHeight = 842;
  int PagePixelMargin = 50;
  int MapScaleFactor = 1;
  int MapPixelSize = 500;
  hashTableObj FontHash=NULL;

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
      map = msLoadMap(argv[i+1]);
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
  pdf = initializePDF(outfile);
  PDF_begin_page(pdf,PagePixelWidth, PagePixelHeight);
  PDF_set_value(pdf,"textrendering",2);
  PDF_translate(pdf, PagePixelMargin, PagePixelHeight-PagePixelMargin);
  PDF_scale(pdf,(float)1,(float)-1);
  pdf=(PDF*)msDrawMapPDF(map, pdf, NULL);
  releasePDF(pdf);

  msFreeMap(map);
  free(outfile);
#endif
  return(0);
} /* ---- END Main Routine ---- */

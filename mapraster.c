#include "map.h"
#include "mapresample.h"
#include "mapthread.h"

extern int msyyparse();
extern int msyylex();
extern char *msyytext;

extern int msyyresult; // result of parsing, true/false
extern int msyystate;
extern char *msyystring;

#ifdef USE_EGIS
#include <errLog.h>
#include <imgLib.h>
#include <sys/time.h>
#endif

#ifdef USE_TIFF
#include <tiffio.h>
#include <tiff.h>
#endif

#ifdef USE_GDAL
#include "gdal.h"
#endif

#ifdef USE_EPPL
#include "epplib.h"
#endif

#ifdef USE_JPEG
#include "jpeglib.h"
#endif

extern int InvGeoTransform( double *gt_in, double *gt_out );

#define MAXCOLORS 256
#define BUFLEN 1024
#define HDRLEN 8

#define	CVT(x) ((x) >> 8) /* converts to 8-bit color value */

#define NUMGRAYS 16

// FIX: need WBMP sig
unsigned char PNGsig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; // 89 50 4E 47 0D 0A 1A 0A hex
unsigned char JPEGsig[3] = {255, 216, 255}; // FF D8 FF hex

#ifdef USE_EGIS
#define rasterDebug 0
struct timeval stTime, endTime;
double diffTime;
#endif

#define RED_LEVELS 5
#define RED_DIV 52
//#define GREEN_LEVELS 7
#define GREEN_LEVELS 5
//#define GREEN_DIV 37
#define GREEN_DIV 52
#define BLUE_LEVELS 5
#define BLUE_DIV 52

#define RGB_LEVEL_INDEX(r,g,b) ((r)*GREEN_LEVELS*BLUE_LEVELS + (g)*BLUE_LEVELS+(b))
#define RGB_INDEX(r,g,b) RGB_LEVEL_INDEX(((r)/RED_DIV),((g)/GREEN_DIV),((b)/BLUE_DIV))

/* 
** NEED TO FIX THIS, SPECIAL FOR RASTERS!
*/
static int getClass(layerObj *layer, char *str)
{
  int i;
  char *tmpstr=NULL;

  if((layer->numclasses == 1) && !(layer->class[0].expression.string)) /* no need to do lookup */
    return(0);

  if(!str) return(-1);

  for(i=0; i<layer->numclasses; i++) {
    if (layer->class[i].expression.string == NULL) /* Empty expression - always matches */
      return(i);
    switch(layer->class[i].expression.type) {
    case(MS_STRING):
      if(strcmp(layer->class[i].expression.string, str) == 0) /* got a match */
	return(i);
      break;
    case(MS_REGEX):
      if(!layer->class[i].expression.compiled) {
	if(regcomp(&(layer->class[i].expression.regex), layer->class[i].expression.string, REG_EXTENDED|REG_NOSUB) != 0) { // compile the expression 
	  msSetError(MS_REGEXERR, "Invalid regular expression.", "getClass()");
	  return(-1);
	}
	layer->class[i].expression.compiled = MS_TRUE;
      }
      if(regexec(&(layer->class[i].expression.regex), str, 0, NULL, 0) == 0) /* got a match */
	return(i);
      break;
    case(MS_EXPRESSION):
      tmpstr = strdup(layer->class[i].expression.string);
      tmpstr = gsub(tmpstr, "[pixel]", str);

      msyystate = 4; msyystring = tmpstr;
      if(msyyparse() != 0)
	return(-1);

      free(tmpstr);

      if(msyyresult) /* got a match */
	return(i);
    }
  }

  return(-1); /* not found */
}

/*
** Function to add a color to an existing color map. It first looks for
** an exact match, then tries to add it to the end of the existing color map,
** and if all else fails it finds the closest color.
*/
static int add_color(mapObj *map, gdImagePtr img, int r, int g, int b)
{
  int c; 
  int ct = -1;
  int op = -1;
  long rd, gd, bd, dist;
  long mindist = 3*255*255;  /* init to max poss dist */
  
  /*
  ** We want to avoid using a color that matches a transparent background
  ** color exactly.  If this is the case, we will permute the value slightly.
  ** When perterbing greyscale images we try to keep them greyscale, otherwise
  ** we just perterb the red component.
  */
  if( map->transparent 
      && map->imagecolor.red == r 
      && map->imagecolor.green == g 
      && map->imagecolor.blue == b )
  {
      if( r == 0 && g == 0 && b == 0 )
      {
          r = g = b = 1;
      }
      else if( r == g && r == b )
      {
          r = g = b = r-1;
      }
      else if( r == 0 )
      {
          r = 1;
      }
      else
      {
          r = r-1;
      }
  }

  /* 
  ** Find the nearest color in the color table.  If we get an exact match
  ** return it right away.
  */
  for (c = 0; c < img->colorsTotal; c++) {

    if (img->open[c]) {
      op = c; /* Save open slot */
      continue; /* Color not in use */
    }
    
    /* don't try to use the transparent color */
    if (map->transparent 
        && img->red  [c] == map->imagecolor.red
        && img->green[c] == map->imagecolor.green
        && img->blue [c] == map->imagecolor.blue )
        continue;

    rd = (long)(img->red  [c] - r);
    gd = (long)(img->green[c] - g);
    bd = (long)(img->blue [c] - b);
/* -------------------------------------------------------------------- */
/*      special case for grey colors (r=g=b). we will try to find       */
/*      either the nearest grey or a color that is almost grey.         */
/* -------------------------------------------------------------------- */
    if (r == g && r == b)
    {
        if (img->red == img->green && img->red ==  img->blue)
          dist = rd;
        else
          dist = rd * rd + gd * gd + bd * bd;
    }
    else
      dist = rd * rd + gd * gd + bd * bd;
    if (dist < mindist) {
      if (dist == 0) {
	return c; /* Return exact match color */
      }
      mindist = dist;
      ct = c;
    }
  }       

  /* no exact match.  We now know closest, but first try to allocate exact */
  if (op == -1) {
    op = img->colorsTotal;
    if (op == gdMaxColors) { /* No room for more colors */
      return ct; /* Return closest available color */
    }
    img->colorsTotal++;
  }

  img->red  [op] = r;
  img->green[op] = g;
  img->blue [op] = b;
  img->open [op] = 0;

  return op; /* Return newly allocated color */  
}

/*
** Allocate color table entries as best possible for a color cube.
*/

int allocColorCube(mapObj *map, gdImagePtr img, int *panColorCube)

{
    int	 r, g, b;
    int i = 0;
    int nGreyColors = 32;
    int nSpaceGreyColors = 8;
    int iColors = 0;
    int	red, green, blue;

    for( r = 0; r < RED_LEVELS; r++ )
    {
        for( g = 0; g < GREEN_LEVELS; g++ )
        {
            for( b = 0; b < BLUE_LEVELS; b++ )
            {                
                red = MS_MIN(255,r * (255 / (RED_LEVELS-1)));
                green = MS_MIN(255,g * (255 / (GREEN_LEVELS-1)));
                blue = MS_MIN(255,b * (255 / (BLUE_LEVELS-1)));

                panColorCube[RGB_LEVEL_INDEX(r,g,b)] = 
                    add_color(map, img, red, green, blue );
                iColors++;
            }
        }
    }
/* -------------------------------------------------------------------- */
/*      Adding 32 grey colors                                           */
/* -------------------------------------------------------------------- */
    
    for (i=0; i<nGreyColors; i++)
    {
        red = i*nSpaceGreyColors;
        green = red;
        blue = red;
        if(iColors < 256)
        {
            panColorCube[iColors] = add_color(map, img, red, green, blue );
            iColors++;
        }
    }
    return MS_SUCCESS;
}

#if defined(USE_GDAL)
/*
** GDAL Support.
*/

#define GEO_TRANS(tr,x,y)  ((tr)[0]+(tr)[1]*(x)+(tr)[2]*(y))

int drawGDAL(mapObj *map, layerObj *layer, gdImagePtr img, 
             GDALDatasetH hDS )

{
  int i,j, k; /* loop counters */
  int cmap[MAXCOLORS];
  double adfGeoTransform[6], adfInvGeoTransform[6];
  int	dst_xoff, dst_yoff, dst_xsize, dst_ysize;
  int	src_xoff, src_yoff, src_xsize, src_ysize;
  int   anColorCube[256];
  double llx, lly, urx, ury;
  rectObj copyRect, mapRect;
  unsigned char *pabyRaw1, *pabyRaw2, *pabyRaw3, *pabyRawAlpha;

  GDALColorTableH hColorMap;
  GDALRasterBandH hBand1, hBand2, hBand3, hBandAlpha;


  /*
   * Compute the georeferenced window of overlap, and do nothing if there
   * is no overlap between the map extents, and the file we are operating on.
   */
  src_xsize = GDALGetRasterXSize( hDS );
  src_ysize = GDALGetRasterYSize( hDS );

  if (GDALGetGeoTransform( hDS, adfGeoTransform ) != CE_None)
  {
      GDALReadWorldFile(GDALGetDescription(hDS), "wld", adfGeoTransform);
  }
  InvGeoTransform( adfGeoTransform, adfInvGeoTransform );

  mapRect = map->extent;

  mapRect.minx -= map->cellsize*0.5;
  mapRect.maxx += map->cellsize*0.5;
  mapRect.miny -= map->cellsize*0.5;
  mapRect.maxy += map->cellsize*0.5;

  copyRect = mapRect;

  if( copyRect.minx < GEO_TRANS(adfGeoTransform,0,src_ysize) )
      copyRect.minx = GEO_TRANS(adfGeoTransform,0,src_ysize);
  if( copyRect.maxx > GEO_TRANS(adfGeoTransform,src_xsize,0) )
      copyRect.maxx = GEO_TRANS(adfGeoTransform,src_xsize,0);

  if( copyRect.miny < GEO_TRANS(adfGeoTransform+3,0,src_ysize) )
      copyRect.miny = GEO_TRANS(adfGeoTransform+3,0,src_ysize);
  if( copyRect.maxy > GEO_TRANS(adfGeoTransform+3,src_xsize,0) )
      copyRect.maxy = GEO_TRANS(adfGeoTransform+3,src_xsize,0);

  if( copyRect.minx >= copyRect.maxx || copyRect.miny >= copyRect.maxy )
      return 0;

  /*
   * Copy the source and destination raster coordinates.
   */
  llx = GEO_TRANS(adfInvGeoTransform+0,copyRect.minx,copyRect.miny);
  lly = GEO_TRANS(adfInvGeoTransform+3,copyRect.minx,copyRect.miny);
  urx = GEO_TRANS(adfInvGeoTransform+0,copyRect.maxx,copyRect.maxy);
  ury = GEO_TRANS(adfInvGeoTransform+3,copyRect.maxx,copyRect.maxy);

  src_xoff = MAX(0,(int) llx);
  src_yoff = MAX(0,(int) ury);
  src_xsize = MIN(MAX(0,(int) (urx - llx + 0.5)),
                  GDALGetRasterXSize(hDS) - src_xoff);
  src_ysize = MIN(MAX(0,(int) (lly - ury + 0.5)),
                  GDALGetRasterYSize(hDS) - src_yoff);

  if( src_xsize == 0 || src_ysize == 0 )
      return 0;

  dst_xoff = (int) ((copyRect.minx - mapRect.minx) / map->cellsize);
  dst_yoff = (int) ((mapRect.maxy - copyRect.maxy) / map->cellsize);
  dst_xsize = (int) ((copyRect.maxx - copyRect.minx) / map->cellsize + 0.5);
  dst_xsize = MIN(MAX(1,dst_xsize),gdImageSX(img) - dst_xoff);
  dst_ysize = (int) ((copyRect.maxy - copyRect.miny) / map->cellsize + 0.5);
  dst_ysize = MIN(MAX(1,dst_ysize),gdImageSY(img) - dst_yoff);

  if( dst_xsize == 0 || dst_ysize == 0 )
      return 0;

  /*
   * Are we in a one band, RGB or RGBA situation?
   */
  hBand1 = GDALGetRasterBand( hDS, 1 );
  hBand2 = hBand3 = hBandAlpha = NULL;

  if( GDALGetRasterCount( hDS ) >= 3
      && GDALGetRasterColorTable( hBand1 ) == NULL 
      && layer->numclasses == 0 )
  {
      hBand1 = GDALGetRasterBand( hDS, 1 );
      hBand2 = GDALGetRasterBand( hDS, 2 );
      hBand3 = GDALGetRasterBand( hDS, 3 );
      if( hBand1 == NULL || hBand2 == NULL || hBand3 == NULL )
          return -1;

      if( GDALGetRasterCount( hDS ) >= 4 )
          hBandAlpha = GDALGetRasterBand( hDS, 4 );
      if( hBandAlpha != NULL 
          && GDALGetRasterColorInterpretation( hBandAlpha ) != GCI_AlphaBand )
          hBandAlpha = NULL;
  }
      
  /*
   * Get colormap for this image.  If there isn't one, create a greyscale
   * colormap. 
   */
  hColorMap = GDALGetRasterColorTable( hBand1 );
  if( hColorMap != NULL )
      hColorMap = GDALCloneColorTable( hColorMap );

  else if( hBand2 == NULL )
  {
      hColorMap = GDALCreateColorTable( GPI_RGB );

      for( i = 0; i < 256; i++ )
      {
          GDALColorEntry sEntry;

          sEntry.c1 = i;
          sEntry.c2 = i;
          sEntry.c3 = i;
          sEntry.c4 = 255;

          GDALSetColorEntry( hColorMap, i, &sEntry );
      }
  }

  /*
   * Setup the mapping between source eightbit pixel values, and the
   * output images color table.  There are two general cases, where the
   * class colors are provided by the MAP file, or where we use the native
   * color table.
   */
  if(layer->numclasses > 0) {
    char tmpstr[3];
    int c;

    for(i=0; i<GDALGetColorEntryCount(hColorMap); i++) {
      if(i != layer->offsite) {
        GDALColorEntry sEntry;

	sprintf(tmpstr,"%d", i);
	c = getClass(layer, tmpstr);

        GDALGetColorEntryAsRGB( hColorMap, i, &sEntry );

	if(c == -1) /* doesn't belong to any class, so handle like offsite */
	  cmap[i] = -1;
	else {
	  if(layer->class[c].color == -1) /* use raster color */
	    cmap[i] = add_color(map, img, sEntry.c1, sEntry.c2, sEntry.c3);
	  else
	    cmap[i] = layer->class[c].color; /* use class color */
	}
      } else
	cmap[i] = -1;
    }
  } else if( hColorMap != NULL ) {
    for(i=0; i<GDALGetColorEntryCount(hColorMap); i++) {
        GDALColorEntry sEntry;

        GDALGetColorEntryAsRGB( hColorMap, i, &sEntry );

        if(i != layer->offsite && sEntry.c4 != 0) 
            cmap[i] = add_color(map,img, sEntry.c1, sEntry.c2, sEntry.c3);
        else
            cmap[i] = -1;
    }
  }
  else
  {
      allocColorCube( map, img, anColorCube );
  }

  /*
   * Read the entire region of interest in one gulp.  GDAL will take
   * care of downsampling and window logic.  
   */
  pabyRaw1 = (unsigned char *) malloc(dst_xsize * dst_ysize);
  if( pabyRaw1 == NULL )
      return -1;

  GDALRasterIO( hBand1, GF_Read, src_xoff, src_yoff, src_xsize, src_ysize, 
                pabyRaw1, dst_xsize, dst_ysize, GDT_Byte, 0, 0 );

  /*
  ** Process single band using the provided, or greyscale colormap
  */
  if( hBand2 == NULL )
  {
      k = 0;
      for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
      {
          int	result;
          
          for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
          {
              result = cmap[pabyRaw1[k++]];
              if( result != -1 )
#ifndef USE_GD_1_2
                  img->pixels[i][j] = result;
#else
                  img->pixels[j][i] = result;
#endif
          }
      }
  }

  /*
  ** Otherwise read green and blue data, and process through the color cube.
  */
  else
  {
      pabyRaw2 = (unsigned char *) malloc(dst_xsize * dst_ysize);
      if( pabyRaw2 == NULL )
          return -1;
      
      GDALRasterIO( hBand2, GF_Read, src_xoff, src_yoff, src_xsize, src_ysize, 
                    pabyRaw2, dst_xsize, dst_ysize, GDT_Byte, 0, 0 );

      pabyRaw3 = (unsigned char *) malloc(dst_xsize * dst_ysize);
      if( pabyRaw3 == NULL )
          return -1;
      
      GDALRasterIO( hBand3, GF_Read, src_xoff, src_yoff, src_xsize, src_ysize, 
                    pabyRaw3, dst_xsize, dst_ysize, GDT_Byte, 0, 0 );

      if( hBandAlpha != NULL )
      {
          pabyRawAlpha = (unsigned char *) malloc(dst_xsize * dst_ysize);
          if( pabyRawAlpha == NULL )
              return -1;
          
          GDALRasterIO( hBandAlpha, GF_Read, 
                        src_xoff, src_yoff, src_xsize, src_ysize, 
                        pabyRawAlpha, dst_xsize, dst_ysize, GDT_Byte, 0, 0 );
      }
      else
          pabyRawAlpha = NULL;

      k = 0;
      for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
      {
          for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
          {
              int	cc_index;

              if( pabyRawAlpha == NULL || pabyRawAlpha[k] != 0 )
              {
                  cc_index = RGB_INDEX(pabyRaw1[k], pabyRaw2[k], pabyRaw3[k]);
#ifndef USE_GD_1_2
                  img->pixels[i][j] = anColorCube[cc_index];
#else
                  img->pixels[j][i] = anColorCube[cc_index];
#endif
              }

              k++;
          }
      }

      free( pabyRaw2 );
      free( pabyRaw3 );
      if( pabyRawAlpha != NULL )
          free( pabyRawAlpha );
  }

  free( pabyRaw1 );
  if( hColorMap != NULL )
      GDALDestroyColorTable( hColorMap );

  return 0;
}
#endif

  
/*
** Function to read georeferencing information for an image from an
** ESRI world file.
*/
static int readWorldFile(char *filename, double *ulx, double *uly, double *cx, double *cy) {
  FILE *stream;
  char *wld_filename;
  int i=0;
  char buffer[BUFLEN];

  wld_filename = strdup(filename);

  strcpy(strrchr(wld_filename, '.'), ".wld");
  stream = fopen(wld_filename, "r");
  if(!stream) {
    strcpy(strrchr(wld_filename, '.'), ".tfw");
    stream = fopen(wld_filename, "r");
    if(!stream) {
      strcpy(strrchr(wld_filename, '.'), ".jgw");
      stream = fopen(wld_filename, "r");
      if(!stream) {
        strcpy(strrchr(wld_filename, '.'), ".gfw");
        stream = fopen(wld_filename, "r");
        if(!stream) {	
	  msSetError(MS_IOERR, "Unable to open world file for reading.", "readWorldFile()");
	  free(wld_filename);
	  return(-1);
        }
      }
    }
  }

  while(fgets(buffer, BUFLEN, stream)) {    
    switch(i) {
    case 0:
      *cx = atof(buffer);
      break;
    case 3:
      *cy = MS_ABS(atof(buffer));
      break;
    case 4:
      *ulx = atof(buffer);
      break;
    case 5:
      *uly = atof(buffer);
      break;
    default:
      break;
    }

    i++;
  }
    
  fclose(stream);
  free(wld_filename);

  return(0);
}

/* read georeferencing info from geoTIFF header, if it exists */
#ifdef USE_TIFF 
static int readGEOTiff(TIFF *tif, double *ulx, double *uly, double *cx, double *cy)
{
  short entries;
  int i,fpos,swap,tiepos,cellpos;
  double tie[6],cell[6];
  TIFFDirEntry tdir;
  FILE *f;
  
  swap=TIFFIsByteSwapped(tif);
  fpos=TIFFCurrentDirOffset(tif);
  f=fopen(TIFFFileName(tif),"rb");
  if (f==NULL) return(-1);
  fseek(f,fpos,0);
  fread(&entries,2,1,f);
  if (swap) TIFFSwabShort(&entries);
  tiepos=0; cellpos=0;
  for (i=0; i<entries; i++) {
    fread(&tdir,sizeof(tdir),1,f);
    if (swap) TIFFSwabShort(&tdir.tdir_tag);
    if (tdir.tdir_tag==0x8482) {  /* geoTIFF tie point */
      tiepos=tdir.tdir_offset;
      if (swap) TIFFSwabLong((long *)&tiepos);
      }
    if (tdir.tdir_tag==0x830e) {  /* geoTIFF cell size */
      cellpos=tdir.tdir_offset;
      if (swap) TIFFSwabLong((long *)&cellpos);
      }
    }
  if (tiepos==0 || cellpos==0) {
    fclose(f);
    return(-1);    
  }
  fseek(f,tiepos,0);
  fread(tie,sizeof(double),6,f);
  if (swap) TIFFSwabArrayOfDouble(tie,6);
  fseek(f,cellpos,0);
  fread(cell,sizeof(double),3,f);
  if (swap) TIFFSwabArrayOfDouble(cell,3);
  *cx=fabs(cell[0]);  *cy=fabs(cell[1]);
  *ulx=tie[3]-(tie[0]*cell[0]);
  *uly=tie[4]+(tie[1]*cell[1]);
  fclose(f);
  return(0);
}
#endif  
  
static int drawTIFF(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_TIFF
  int i,j; /* loop counters */

  double x,y;
  TIFF *tif=NULL; 
  unsigned char *buf; 
  long w, h;
  short type, nbits;
  unsigned short compression;
  unsigned short *red, *green, *blue;
  int cmap[MAXCOLORS];

  double startx, starty; /* this is where we start out reading */
  int st,strip;  /* current strip */
  int xi,yi,startj,endj,boffset,vv;

  double ulx, uly; /* upper left-hand coordinates */
  double skipx,skipy; /* skip factors (x and y) */  
  double cx,cy; /* cell sizes (x and y) */
  long rowsPerStrip;    

  TIFFSetWarningHandler(NULL); /* can these be directed to the mapserver error functions... */
  TIFFSetErrorHandler(NULL);

  tif = TIFFOpen(filename, "rb");
  if(!tif) {
    msSetError(MS_IMGERR, "Error loading TIFF image.", "drawTIFF()");
    return(-1);
  }
  
  if(readGEOTiff(tif, &ulx, &uly, &cx, &cy) != 0) {
    if(readWorldFile(filename, &ulx, &uly, &cx, &cy) != 0) {
      TIFFClose(tif);
      return(-1);
    }   
  }

  skipx = map->cellsize/cx;
  skipy = map->cellsize/cy;
  startx = (map->extent.minx - ulx)/cx;
  starty = (uly - map->extent.maxy)/cy;

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &nbits);
  if(nbits != 8 && nbits != 4 && nbits != 1) {
    msSetError(MS_IMGERR, "Only 1, 4, and 8-bit images are supported.", "drawTIFF()");
    TIFFClose(tif);
    return(-1); 
  }

  TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);
  TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);

  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &type); /* check image type */
  switch(type) {  
  case(PHOTOMETRIC_PALETTE):    
    TIFFGetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue);
    if(layer->numclasses > 0) {
      char tmpstr[3];
      int c;

      for(i=0; i<MAXCOLORS; i++) {
	if(i != layer->offsite) {
	  sprintf(tmpstr,"%d", i);
	  c = getClass(layer, tmpstr);
	  
	  if(c == -1) /* doesn't belong to any class, so handle like offsite */
	    cmap[i] = -1;
	  else {
	    if(layer->class[c].color == -1) /* use raster color */
	      cmap[i] = add_color(map,img, CVT(red[i]), CVT(green[i]), CVT(blue[i]));
	    else
	      if(layer->class[c].color == -255) /* make transparent */
		cmap[i] = -1;
	      else
		cmap[i] = layer->class[c].color; /* use class color */
	  }
	} else
	  cmap[i] = -1;
      }
    } else {
      for(i=0; i<MAXCOLORS; i++) {
	if(i != layer->offsite)
	  cmap[i] = add_color(map,img, CVT(red[i]), CVT(green[i]), CVT(blue[i]));
        else
	  cmap[i] = -1;
      }
    }
    break;
  case PHOTOMETRIC_MINISBLACK: /* classes are NOT honored for non-colormapped data */
    if (nbits==1) {
      if(layer->offsite >= 0)
	cmap[layer->offsite] = -1;
      if(layer->offsite != 0)
	cmap[0]=add_color(map,img,0,0,0);
      if(layer->offsite != 1)
	cmap[1]=add_color(map,img,255,255,255);
    } else { 
      if (nbits==4) {
	for (i=0; i<16; i++) {
	  if(i != layer->offsite)
	    cmap[i]=add_color(map,img,i*17,i*17,i*17);
	  else
	    cmap[i] = -1;
	}
      } else { /* 8-bit */
	for (i=0; i<256; i++) {
	  if(i != layer->offsite)
	    cmap[i]=add_color(map,img,(i>>4)*17,(i>>4)*17,(i>>4)*17);
	  else
	    cmap[i] = -1;
	}
      }
    }
    break;
  case PHOTOMETRIC_MINISWHITE: /* classes are NOT honored for non-colormapped data */
    if (nbits==1) {
      if(layer->offsite >= 0)
	cmap[layer->offsite] = -1;
      if(layer->offsite != 0)
	cmap[0]=add_color(map,img,255,255,255);	
      if(layer->offsite != 1)
	cmap[1]=add_color(map,img,0,0,0);
    }
    else {
      msSetError(MS_IMGERR,"Can't do inverted grayscale images","drawTIFF()");
      TIFFClose(tif);
      return(-1);
    }
    break;
  default:
    msSetError(MS_IMGERR, "Only colormapped and grayscale images are supported.", "drawTIFF()");
    TIFFClose(tif);
    return(-1);
  }

  buf = (unsigned char *)_TIFFmalloc(TIFFStripSize(tif)); /* allocate strip buffer */
  
  /* 
  ** some set-up;  the startx calculation is problematical analytically so we
  ** find it iteratively 
  */
  x=startx;
  for (j=0; j<img->sx && MS_NINT(x)<0; j++) x+=skipx;
  startx=x; startj=j;
  for (j=startj; j<img->sx && MS_NINT(x)<w; j++) x+=skipx;
  endj=j;
  y=starty;
  strip=-1;
  for(i=0; i<img->sy; i++) { /* for each row */
    yi=MS_NINT(y);
    if((yi >= 0) && (yi < h)) {
      st=TIFFComputeStrip(tif,yi,0);
      if (st!=strip) {
        TIFFReadEncodedStrip(tif, st, buf, TIFFStripSize(tif));
        strip=st;
      }  
      x = startx;

      switch (nbits) {
      case 8:
        boffset=(yi % rowsPerStrip) * w;
        for(j=startj; j<endj; j++) {    
	  xi=MS_NINT(x);
	  vv=buf[xi+boffset];  
	  
	  if (cmap[vv] != -1) 
	    img->pixels[i][j] = cmap[vv];
	  
	  x+=skipx;
        }
        break;
      case 4:
        boffset=(yi % rowsPerStrip) * ((w+1) >> 1);
        for(j=startj; j<endj; j++) {
          xi=MS_NINT(x);
          vv=(buf[(xi >> 1) + boffset] >> (4*((yi+1) & 1))) & 15;

	  if (cmap[vv] != -1) 
	    img->pixels[i][j] = cmap[vv];

	  x+=skipx;
	}  
        break;  
      case 1:
        boffset=(yi % rowsPerStrip) * ((w+7) >> 3);
        for(j=startj; j<endj; j++) {
          xi=MS_NINT(x);    
          vv=(buf[(xi >> 3) + boffset] >> (7-(xi & 7))) & 1;         

	  if (cmap[vv] != -1) 
	    img->pixels[i][j] = cmap[vv];

	  x+=skipx;
	}  
        break; 
      }   
    }
    y+=skipy;  
  }
  _TIFFfree(buf);

  TIFFClose(tif);
  return(0);
#else
   msSetError(MS_IMGERR, "TIFF support is not available.", "drawTIFF()");
   return(-1);
#endif
} 

static int drawPNG(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_GD_PNG
  int i,j; /* loop counters */
  double x,y;
  int cmap[MAXCOLORS];
  double w, h;

  FILE *pngStream;
  gdImagePtr png;

  int pixel;

  double startx, starty; /* this is where we start out reading */

  double ulx, uly; /* upper left-hand coordinates */
  double skipx,skipy; /* skip factors (x and y) */  
  double cx,cy; /* cell sizes (x and y) */

  pngStream = fopen(filename, "rb");
  if(!pngStream) {
    msSetError(MS_IOERR, "Error open image file.", "drawPNG()");
    return(-1);
  }

  png = gdImageCreateFromPng(pngStream);
  fclose(pngStream);

  if(!png) {
    msSetError(MS_IMGERR, "Error loading PNG file.", "drawPNG()");
    return(-1);
  }

  w = gdImageSX(png) - .5; /* to avoid rounding problems */
  h = gdImageSY(png) - .5;

  if(layer->transform) {
    if(readWorldFile(filename, &ulx, &uly, &cx, &cy) != 0)
      return(-1);

    skipx = map->cellsize/cx;
    skipy = map->cellsize/cy;
    startx = (map->extent.minx - ulx)/cx;
    starty = (uly - map->extent.maxy)/cy;
  } else {
    skipx = skipy = 1;
    startx = starty = 0;    
  }

  if(layer->numclasses > 0) {
    char tmpstr[3];
    int c;

    for(i=0; i<gdImageColorsTotal(png); i++) {
      if(i != layer->offsite && i != gdImageGetTransparent(png)) {
	sprintf(tmpstr,"%d", i);
	c = getClass(layer, tmpstr);

	if(c == -1) /* doesn't belong to any class, so handle like offsite */
	  cmap[i] = -1;
	else {
	  if(layer->class[c].color == -1) /* use raster color */
	    cmap[i] = add_color(map,img, gdImageRed(png,i), gdImageGreen(png,i), gdImageBlue(png,i));
	  else
	    cmap[i] = layer->class[c].color; /* use class color */
	}
      } else
	cmap[i] = -1;
    }
  } else {
    for(i=0; i<gdImageColorsTotal(png); i++) {
      if(i != layer->offsite && i != gdImageGetTransparent(png)) 
	cmap[i] = add_color(map,img, gdImageRed(png,i), gdImageGreen(png,i), gdImageBlue(png,i));
      else
	cmap[i] = -1;
      /* fprintf(stderr, "\t\t%d %d\n", i, cmap[i]); */
    }
  }

  y=starty;
  for(i=0; i<img->sy; i++) { /* for each row */
    if((y >= -0.5) && (y < h)) {
      x = startx;
      for(j=0; j<img->sx; j++) {
	if((x >= -0.5) && (x < w)) {
	  pixel = png->pixels[(y == -0.5)?0:MS_NINT(y)][(x == -0.5)?0:MS_NINT(x)];	  
	  if(cmap[pixel] != -1)
	    img->pixels[i][j] = cmap[pixel];
	}
	x+=skipx;
      }
    }
    y+=skipy;  
  }

  gdImageDestroy(png);
  return(0);
#else
  msSetError(MS_IMGERR, "PNG support is not available.", "drawPNG()");
  return(-1);
#endif
}

static int drawGIF(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_GD_GIF
  int i,j; /* loop counters */
  double x,y;
  int cmap[MAXCOLORS];
  double w, h;

  FILE *gifStream;
  gdImagePtr gif;

  int pixel;

  double startx, starty; /* this is where we start out reading */

  double ulx, uly; /* upper left-hand coordinates */
  double skipx,skipy; /* skip factors (x and y) */  
  double cx,cy; /* cell sizes (x and y) */

  gifStream = fopen(filename, "rb");
  if(!gifStream) {
    msSetError(MS_IOERR, "Error open image file.", "drawGIF()");
    return(-1);
  }

  gif = gdImageCreateFromGif(gifStream);
  fclose(gifStream);

  if(!gif) {
    msSetError(MS_IMGERR, "Error loading GIF file.", "drawGIF()");
    return(-1);
  }

  w = gdImageSX(gif) - .5; /* to avoid rounding problems */
  h = gdImageSY(gif) - .5;

  if(layer->transform) {
    if(readWorldFile(filename, &ulx, &uly, &cx, &cy) != 0)
      return(-1);

    skipx = map->cellsize/cx;
    skipy = map->cellsize/cy;
    startx = (map->extent.minx - ulx)/cx;
    starty = (uly - map->extent.maxy)/cy;
  } else {
    skipx = skipy = 1;
    startx = starty = 0;    
  }

  if(layer->numclasses > 0) {
    char tmpstr[3];
    int c;

    for(i=0; i<gdImageColorsTotal(gif); i++) {
      if(i != layer->offsite && i != gdImageGetTransparent(gif)) {
	sprintf(tmpstr,"%d", i);
	c = getClass(layer, tmpstr);

	if(c == -1) /* doesn't belong to any class, so handle like offsite */
	  cmap[i] = -1;
	else {
	  if(layer->class[c].color == -1) /* use raster color */
	    cmap[i] = add_color(map,img, gdImageRed(gif,i), gdImageGreen(gif,i), gdImageBlue(gif,i));
	  else
	    cmap[i] = layer->class[c].color; /* use class color */
	}
      } else
	cmap[i] = -1;
    }
  } else {
    for(i=0; i<gdImageColorsTotal(gif); i++) {
      if(i != layer->offsite && i != gdImageGetTransparent(gif)) 
	cmap[i] = add_color(map,img, gdImageRed(gif,i), gdImageGreen(gif,i), gdImageBlue(gif,i));
      else
	cmap[i] = -1;
      /* fprintf(stderr, "\t\t%d %d\n", i, cmap[i]); */
    }
  }

  y=starty;
  for(i=0; i<img->sy; i++) { /* for each row */
    if((y >= -0.5) && (y < h)) {
      x = startx;
      for(j=0; j<img->sx; j++) {
	if((x >= -0.5) && (x < w)) {
	  pixel = gif->pixels[(y == -0.5)?0:MS_NINT(y)][(x == -0.5)?0:MS_NINT(x)];	  
	  if(cmap[pixel] != -1)
	    img->pixels[i][j] = cmap[pixel];
	}
	x+=skipx;
      }
    }
    y+=skipy;  
  }

  gdImageDestroy(gif);
  return(0);
#else
  msSetError(MS_IMGERR, "GIF support is not available.", "drawGIF()");
  return(-1);
#endif
}


static int drawJPEG(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_JPEG
  int i,j,k; /* loop counters */  
  int cmap[MAXCOLORS];
  double w, h;

  unsigned char pixel;
  JSAMPARRAY buffer;

  double startx, starty; /* this is where we start out reading */
  double skipx,skipy; /* skip factors (x and y) */ 
  double x,y;

  double ulx, uly; /* upper left-hand coordinates */  
  double cx,cy; /* cell sizes (x and y) */

  FILE *jpegStream;

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpegStream = fopen(filename, "rb");
  if(!jpegStream) {
    msSetError(MS_IOERR, "Error open image file.", "drawJPEG()");
    return(-1);
  }
  jpeg_stdio_src(&cinfo, jpegStream);

  jpeg_read_header(&cinfo, TRUE);

  if(cinfo.jpeg_color_space != JCS_GRAYSCALE) {
    jpeg_destroy_decompress(&cinfo);
    fclose(jpegStream);
    msSetError(MS_IOERR, "Only grayscale JPEG images are supported.", "drawJPEG()");
    return(-1);
  }

  // set up the color map
  for (i=0; i<256; i++) {
    if(i != layer->offsite)
      cmap[i]=add_color(map,img,(i>>4)*17,(i>>4)*17,(i>>4)*17);
    else
      cmap[i] = -1;
  }

  if(readWorldFile(filename, &ulx, &uly, &cx, &cy) != 0)
    return(-1);
  
  skipx = map->cellsize/cx;
  skipy = map->cellsize/cy;

  // muck with scaling
  if(MS_MIN(skipx, skipy) >= 8)
    cinfo.scale_denom = 8;
  else
    if(MS_MIN(skipx, skipy) >= 4)
      cinfo.scale_denom = 4;
    else
      if(MS_MIN(skipx, skipy) >= 2)
	cinfo.scale_denom = 2;

  skipx = skipx/cinfo.scale_denom;
  skipy = skipy/cinfo.scale_denom;
  startx = (map->extent.minx - ulx)/(cinfo.scale_denom*cx);
  starty = (uly - map->extent.maxy)/(cinfo.scale_denom*cy);

  // initialize the decompressor, this sets output sizes etc...
  jpeg_start_decompress(&cinfo);

  w = cinfo.output_width - .5; /* to avoid rounding problems */
  h = cinfo.output_height - .5;

  // one pixel high buffer as wide as the image, goes away when done with the image
  buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, cinfo.output_width, 1);

  // skip any unneeded scanlines *yuck*
  for(i=0; i<starty-1; i++)
    jpeg_read_scanlines(&cinfo, buffer, 1);

  y=starty;

  for(i=0; i<img->sy; i++) { /* for each row */
    if((y > -0.5) && (y < cinfo.output_height)) {
      
      jpeg_read_scanlines(&cinfo, buffer, 1);
      
      x = startx;
      for(j=0; j<img->sx; j++) {
	if((x >= -0.5) && (x < cinfo.output_width)) {
	  pixel = buffer[0][(x == -0.5)?0:MS_NINT(x)];
	  if(cmap[pixel] != -1)
	    img->pixels[i][j] = cmap[pixel];
	}
	x+=skipx;
	if(x>=cinfo.output_width) // next x is out of the image, so quit
	  break;
      }
    }
    
    y+=skipy;

    if(y>=cinfo.output_height) // next y is out of the image, so quit
      break;

    for(k=cinfo.output_scanline; k<y-1; k++)
      jpeg_read_scanlines(&cinfo, buffer, 1);
  } 

  jpeg_destroy((j_common_ptr) &cinfo);
  fclose(jpegStream);

  return(0);
#else
  msSetError(MS_IMGERR, "JPEG support is not available.", "drawJPEG()");
  return(-1);
#endif
}

typedef struct erdhead {
  char hdword[6];
  short pack,bands;
  char res1[6];
  long cols,rows,col1,row1;
  char res2[56];
  short maptyp,nclass;
  char res3[14];
  short utyp;
  float area,xl,yt,xcell,ycell;
} erdhead;

static int drawERD(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_EPPL
  int i,j,rowsz,reverse;
  char trec[128],tname[128];
  unsigned char gc[256],rc[256],bc[256],*pb;
  int startj,endj,rr,yi,vv,off;
  double startx,starty,skipx,skipy,x,y;
  int cmap[MAXCOLORS];
  FILE *erd,*trl;
  erdhead hd;

  {union {long i;char c[4];} u; u.i=1; reverse=(u.c[0]==0);}
  erd=fopen(filename,"rb");
  if (erd==NULL) {  
    msSetError(MS_IMGERR, "Error loading ERDAS image.", "drawERD()");
    return(-1);
  }
  fread(&hd,128,1,erd);
  /* need byte swapper here */
  if (reverse) {
    swap2(&hd.pack,2);
    swap4(&hd.cols,4);
    swap2(&hd.maptyp,2);
    swap2(&hd.utyp,1);
    swap4((long *)&hd.area,5);
  }  
  if (!strncmp(hd.hdword,"HEADER",6)) {   /*old format,convert to new*/
    memcpy(hd.hdword,"HEAD74",6);
    hd.cols=(long)MS_NINT((float)hd.cols);
    hd.rows=(long)MS_NINT((float)hd.rows);
    hd.col1=(long)MS_NINT((float)hd.col1);
    hd.row1=(long)MS_NINT((float)hd.row1);
  }
  if (strncmp(hd.hdword,"HEAD74",6) || hd.pack > 1 || hd.bands!=1) {
    msSetError(MS_IMGERR,"Bad header or unsupported header options.","drawERD()");
    fclose(erd);
    return -1;
  }
  strcpy(tname,filename);
  strcpy(strrchr(tname,'.'),".trl");
  trl=fopen(tname,"rb");
  if (trl!=NULL) {  /**** check for trailer file*/
    fread(trec,128,1,trl);
    if (!strncmp(trec,"TRAILER",7)) memcpy(trec,"TRAIL74",7);
    if (!strncmp(trec,"TRAIL74",7)) {
      fread(gc,256,1,trl);
      fread(rc,256,1,trl);
      fread(bc,256,1,trl);

      if(layer->numclasses > 0) {
	char tmpstr[3];
	int c;

	for(i=0; i<hd.nclass; i++) {
	  if(i != layer->offsite) {
	    sprintf(tmpstr,"%d", i);
	    c = getClass(layer, tmpstr);
	    
	    if(c == -1) /* doesn't belong to any class, so handle like offsite */
	      cmap[i] = -1;
	    else {
	      if(layer->class[c].color == -1) /* use raster color */
		cmap[i] = add_color(map,img, rc[i], gc[i], bc[i]);
	      else
		cmap[i] = layer->class[c].color; /* use class color */
	    }
	  } else
	    cmap[i] = -1;
	}
      } else {
	for(i=0; i<hd.nclass; i++) {
	  if(i != layer->offsite) 
	    cmap[i] = add_color(map,img, rc[i], gc[i], bc[i]);
	  else
	    cmap[i] = -1;
	}
      }
    }
    fclose(trl);
  } else {
    for (i=0; i<hd.nclass; i++) {  /* no trailer file, make gray, classes are not honored */
      if(i != layer->offsite) {
	j=((i*16)/hd.nclass)*17; 
	cmap[i] = add_color(map,img, j, j, j);
      } else
	cmap[i] = -1;
    } 
  }

  skipx=map->cellsize/hd.xcell;
  skipy=map->cellsize/hd.ycell;
  startx=(map->extent.minx-hd.xl)/hd.xcell;
  starty=(hd.yt-map->extent.maxy)/hd.ycell;
  pb=(unsigned char *)malloc(hd.cols);
  x=startx;
  for (j=0; j<img->sx && MS_NINT(x)<0; j++) x+=skipx;
  startx=x; startj=j;
  for (j=startj; j<img->sx && MS_NINT(x)<hd.cols; j++) x+=skipx;
  endj=j;
  rr=-1;
  y=starty;
  off=layer->offsite;
  if (hd.pack==0) rowsz=hd.cols;
   else rowsz=(hd.cols+1)/2;
  for (i=0; i<img->sy; i++) {
    yi=MS_NINT(y);
    if (yi>=0 && yi<hd.rows) {
      if (yi!=rr) {
        fseek(erd,128+rr*rowsz,0);
        fread(pb,rowsz,1,erd);
        rr=yi;
      }  
      x=startx;
      for (j=startj; j<endj; j++) {
        if (hd.pack==0) vv=pb[(int)MS_NINT(x)];
        else {
          vv=(int)MS_NINT(x);
          if (vv & 1) vv=(pb[vv >> 1]) >> 4;
          else vv=(pb[vv >> 1]) & 15;
        }  
        
	if(cmap[vv] != -1)
	  img->pixels[i][j] = cmap[vv];

       x+=skipx;
      }
    }
    y+=skipy;
  }
  free(pb);
  fclose(erd);
  return 0;
#else
   msSetError(MS_IMGERR, "ERDAS support is not available.", "drawERD()");
   return(-1);
#endif
}
  
static int drawEPP(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_EPPL
  int ncol,nrow,yi,rr,i,j,off,startj,endj,vv;
  double cx,cy,skipx,skipy,x,y,startx,starty;
  int cmap[MAXCOLORS];
  eppfile epp;
  TRGB color;
  clrfile clr;

  for(i=0; i<MAXCOLORS; i++) cmap[i] = -1; /* initialize the colormap to all transparent */

  strcpy(epp.filname,filename);
  if (!eppreset(&epp)) return -1;
  
  ncol=epp.lc-epp.fc+1;
  nrow=epp.lr-epp.fr+1;
  cx=(epp.lcx-epp.fcx)/ncol;
  cy=(epp.fry-epp.lry)/nrow;
  skipx=map->cellsize/cx;
  skipy=map->cellsize/cy;
  startx=(map->extent.minx-epp.fcx)/cx;
  starty=(epp.fry-map->extent.maxy)/cy;
  
  if (epp.kind!=8) {
    msSetError(MS_IMGERR,"Only 8 bit EPPL files supported.","drawEPP()");
    eppclose(&epp);
    return -3;
  }
  
  /* set up colors here */
  strcpy(clr.filname,filename);
  if (!clrreset(&clr)) { /* use gray from min to max if no color file, classes not honored for greyscale */
    for (i=epp.minval; i<=epp.maxval; i++) {
      if(i != layer->offsite) {
	j=(((i-epp.minval)*16) / (epp.maxval-epp.minval+1))*17; 
	cmap[i]=add_color(map,img,j,j,j);
      }
    }  
  } else {
    if(layer->numclasses > 0) {
      char tmpstr[3];
      int c;
      
      for (i=epp.minval; i<=epp.maxval; i++) {
	if(i != layer->offsite) {
	  sprintf(tmpstr,"%d", i);
	  c = getClass(layer, tmpstr);
	  
	  if(c != -1) {
	    if(layer->class[c].color == -1) { /* use raster color */
	      clrget(&clr,i,&color);
	      cmap[i] = add_color(map,img, color.red, color.green, color.blue);
	    } else
	      cmap[i] = layer->class[c].color; /* use class color */
	  }
	}
      }
    } else {
      for (i=epp.minval; i<=epp.maxval; i++) {
	if(i != layer->offsite) {
	  clrget(&clr,i,&color);
	  cmap[i] = add_color(map,img, color.red, color.green, color.blue);
	}
      }
    }
    clrclose(&clr);
  }
  
  /* loop setup */
  x=startx;
  for (j=0; j<img->sx && MS_NINT(x)<0; j++) x+=skipx;
  startx=x; startj=j;
  for (j=startj; j<img->sx && MS_NINT(x)<ncol; j++) x+=skipx;
  endj=j;
  rr=-1;
  y=starty;
  off=layer->offsite;
  for (i=0; i<img->sy; i++) {
    yi=MS_NINT(y);
    if (yi>=0 && yi<nrow) {
      if (yi>rr+1) if (!position(&epp,yi+epp.fr)) return -2;
      if (yi>rr) if (!get_row(&epp)) return -2;
      rr=yi;
      x=startx;
      for (j=startj; j<endj; j++) {
        vv=epp.rptr[(int)MS_NINT(x)+1];

#ifndef USE_GD_1_2
	if(cmap[vv] != -1) 
	  img->pixels[i][j] = cmap[vv];
#else
	if(cmap[vv] != -1)
	  img->pixels[j][i] = cmap[vv];
#endif

        x+=skipx;
      }
    }
    y+=skipy;
  }
  eppclose(&epp);
  return 0;
#else
  msSetError(MS_IMGERR, "EPPL7 support is not available.", "drawEPP()");
  return(-1);
#endif  
}

#ifdef USE_EGIS
static int drawGEN(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
  	int i,j; 		/* loop counters */
  	double x,y;
  	int cmap[MAXCOLORS];
  	int w, h;

  	int pixel;
  	double startx, starty; 	/* this is where we start out reading */

  	double ulx, uly; 	/* upper left-hand coordinates */
  	double skipx,skipy; 	/* skip factors (x and y) */  
  	double cx,cy; 		/* cell sizes (x and y) */

	char hName[256];
	stRawImg stImgInfo;
	char errLogMsg[80];
	int status;
	int length;
	int maxPixels;
	int bSize;
	int nBands = 1;
	int bNos[1];
	int tmp = 1;

	unsigned char *tImg;

	// Initialize image header parameters

	errLogMsg[0] = '\0';
  	sprintf(errLogMsg, "From GEN: %s\n", filename); 
	writeErrLog(errLogMsg);
	
	// It is assumed that filename extension is .img and .hdr for header
	// Remove .img from filename 

	length = strlen(filename);
	strncpy(hName, filename, length-4);
	hName[length-4] = '\0';

	errLogMsg[0] = '\0';
  	sprintf(errLogMsg, "From GEN: %s, %s\n", filename, hName); 
  	writeErrLog(errLogMsg);

	// Initialize timer just before opening the image
        gettimeofday(&stTime, 0);

        if ( (status = initImgInfo(hName, &stImgInfo) ) != 1 )
        {
		writeErrLog("In drawGEN - initImgInfo failed\n");
                return -1;
        }

  	// stImgInfo.Bands = 1; 
  	w = stImgInfo.Pixels; // = 1321; 
  	h = stImgInfo.Lines; // = 964;

	ulx = stImgInfo.ulx; // = 115400.0;
	uly = stImgInfo.uly; // = 5547400.0;

	cx = stImgInfo.pwidth; // = 1000.0;
	cy = stImgInfo.pheight; // = 1000.0; 

	if (rasterDebug)
	{
	errLogMsg[0] = '\0';
  	sprintf(errLogMsg, "From GEN: %f %f %f %f\n",  ulx, uly, cx, cy); 
  	writeErrLog(errLogMsg);
	}

  	skipx = map->cellsize/cx;
  	skipy = map->cellsize/cy;
  	startx = (map->extent.minx - ulx)/cx;
  	starty = (int) (uly - map->extent.maxy)/cy;

	if (rasterDebug)
	{
  	errLogMsg[0] = '\0';
  	sprintf(errLogMsg, "skipx %f, skipy %f, startx %d, starty %d, w%d h%d\n",  
			skipx, skipy, startx, starty, w, h); 
	writeErrLog(errLogMsg);
	}

	// Read image and fill in the img structure

	maxPixels = img->sx * (skipx +1);
	if (maxPixels >=  stImgInfo.Pixels) maxPixels = stImgInfo.Pixels; 

	bSize = w * skipx + 1; 
	if (bSize < maxPixels) bSize = maxPixels + 1;

	tImg = malloc( bSize * sizeof(char));
	if (tImg == NULL)
	{
  		errLogMsg[0] = '\0';
  		sprintf(errLogMsg, "Error allocating mem to hold a line\n");  
		writeErrLog(errLogMsg);
		return -1;
	}

	bNos[0] = 1;
  	y = starty;

	// Add code to do color mapping - For the time its b&w 

    	for(i = 0; i < 255; i++) 
	{
      		if(i != layer->offsite)
		{
			j = (i * 16) / 255 * 17;
			cmap[i] = add_color(map, img, j, j, j); 
		}
		else
			cmap[i] = -1;
	}

	if (tmp == 0)
	{
		writeErrLog("In drawGEN - not doing anything\n");
		return (0);
	}

	// for each row 
  	for(i = 0; i < img->sy; i++) 
	{ 
		/*
		errLogMsg[0] = '\0';
  		sprintf(errLogMsg, "Line %d - Y = %g\n",  i, y); 
  		writeErrLog(errLogMsg);
		*/

    	   if((y >= 0) && (y < h)) 
	   {
      		x = startx;

		// Read a line here

		/*
		if((x >= 0) && (x < w)) 
		{
		*/
			
		status = imgReadLine(&stImgInfo, (int)y, (int)x, 
					maxPixels, nBands, bNos, tImg); 

		/*
  			errLogMsg[0] = '\0';
  			sprintf(errLogMsg, "reading line %d pix %d mx %d\n",
					 (int)y, (int)x, maxPixels);  
			writeErrLog(errLogMsg);
		}
		*/

		if (status != 1)
		{

  			errLogMsg[0] = '\0';
  			sprintf(errLogMsg, "Err reading line %d pix %d mx %d\n",
					 (int)y, (int)x, maxPixels);  
			writeErrLog(errLogMsg);
			// return -1;
		}

      		for(j = 0; j < img->sx; j++) 
		{
			if((x >= 0) && (x < w)) 
			{
				pixel = (int) tImg[(int) (j * skipx)];
	  			if(cmap[pixel] != -1)
	    				img->pixels[j][i] =  cmap[pixel]; 
			}
			x += skipx;
      		}

		/*
		errLogMsg[0] = '\0';
  		sprintf(errLogMsg, "j = %d - x = %g\n", j, x); 
  		writeErrLog(errLogMsg);
		*/
    	   }
           y+=skipy;  
  	}

	// Stop timer and get total time
        gettimeofday(&endTime, 0);

        diffTime = endTime.tv_sec - stTime.tv_sec;
        diffTime = diffTime + (endTime.tv_usec - stTime.tv_usec) / 1e6;
 
			errLogMsg[0] = '\0';
      	sprintf(errLogMsg, "\ndrawGen Took %f Secs for L %d, P %d\n", 
							diffTime, img->sy, maxPixels); 

  	writeErrLog(errLogMsg);

	errLogMsg[0] = '\0';
  	sprintf(errLogMsg, "Image is generated\n"); 
  	writeErrLog(errLogMsg);

	return (0);
}
#endif

int msDrawRasterLayerGD(mapObj *map, layerObj *layer, gdImagePtr img) {

  /*
  ** Check for various file types and act appropriately.
  */
  int status;
  FILE *f;  
  char dd[8];
  char *filename=NULL;

  char old_path[MS_PATH_LENGTH];

  int t;
  int tileitemindex=-1;
  shapefileObj tilefile;
  char tilename[MS_PATH_LENGTH];
  int numtiles=1; /* always at least one tile */

  rectObj searchrect;

#ifdef USE_EGIS
  char *ext; // OV -egis- temp variable
#endif


  if(!layer->data && !layer->tileindex)
    return(0);

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT))
    return(0);

  if(map->scale > 0) {
    if((layer->maxscale > 0) && (map->scale > layer->maxscale))
      return(0);
    if((layer->minscale > 0) && (map->scale <= layer->minscale))
      return(0);
  }

  getcwd(old_path, MS_PATH_LENGTH); /* save old working directory */
  if(map->shapepath)
  {
      int	error;
      
      error = chdir(map->shapepath);
      if( error != 0 )
      {
          msSetError( MS_IOERR, "chdir(%s)", "msDrawRasterLayer()", 
                      map->shapepath );
          return -1;
      }
  }

  if(layer->tileindex) { /* we have in index file */
    if(msSHPOpenFile(&tilefile, "rb", map->shapepath, layer->tileindex) == -1) return(-1);    
    if((tileitemindex = msDBFGetItemIndex(tilefile.hDBF, layer->tileitem)) == -1) return(-1);
    searchrect = map->extent;
#ifdef USE_PROJ
    if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
      msProjectRect(&map->projection, &layer->projection, &searchrect); // project the searchrect to source coords
#endif
    status = msSHPWhichShapes(&tilefile, searchrect);
    if(status != MS_SUCCESS) 
      numtiles = 0; // could be MS_DONE or MS_FAILURE
    else
      numtiles = tilefile.numshapes;
  }

  for(t=0;t<numtiles;t++) { /* for each tile, always at least 1 tile */

    if(layer->tileindex) {
      if(!msGetBit(tilefile.status,t)) continue; /* on to next tile */
      if(layer->data == NULL) /* assume whole filename is in attribute field */
	filename = (char*)msDBFReadStringAttribute(tilefile.hDBF, t, tileitemindex);
      else {  
	sprintf(tilename,"%s/%s", msDBFReadStringAttribute(tilefile.hDBF, t, tileitemindex) , layer->data);
	filename = tilename;
      }
    } else {
      filename = layer->data;
    }

    if(strlen(filename) == 0) continue;

    f = fopen(filename,"rb");
    if (!f) {
      memset( dd, 0, 8 );
    }
    else
    {
        fread(dd,8,1,f); // read some bytes to try and identify the file
        fclose(f);
    }

#if !defined(USE_GDAL) || defined(USE_TIFF)
    if (memcmp(dd,"II*\0",4)==0 || memcmp(dd,"MM\0*",4)==0) {
      if(layer->transform && msProjectionsDiffer(&(map->projection), &(layer->projection))) {
        msSetError(MS_MISCERR, "Raster reprojection supported only with the GDAL library.", "msDrawRasterLayer( TIFF )");
        return(-1);
      }
      status = drawTIFF(map, layer, img, filename);
      if(status == -1) {
	chdir(old_path); /* restore old cwd */
	return(-1);
      }
      continue;
    }
#endif

    if (memcmp(dd,"GIF8",4)==0) {
      if(layer->transform && 
         msProjectionsDiffer(&(map->projection), &(layer->projection))) {
#ifdef USE_GDAL
        /*
        ** Image should be reprojected... We'll set the offsite to the GIF
        ** transparent colour (since GDAL ignores it) and then let GDAL
        ** render the layer (if it has GIF support included).
        **
        ** __TODO__ Yes, this is a hack and should be improved...
        **          but hey!  It works for now.
        */
        if (layer->offsite == -1)
        {
#ifdef USE_GD_GIF
            FILE *gifStream;
            gdImagePtr gif;

            gifStream = fopen(filename, "rb");
            if(!gifStream) {
                msSetError(MS_IOERR, "Error open image file.", "drawGIF()");
                chdir(old_path); /* restore old cwd */
                return(-1);
            }
            gif = gdImageCreateFromGif(gifStream);
            fclose(gifStream);

            if(!gif) {
                msSetError(MS_IMGERR, "Error loading GIF file.", "drawGIF()");
                chdir(old_path); /* restore old cwd */
                return(-1);
            }
            layer->offsite = gdImageGetTransparent(gif);  // both default to -1
            gdImageDestroy(gif);
#else
            msSetError(MS_MISCERR, "GIF Format not supported.", "msDrawRasterLayer( GIF )");
            return(-1);
#endif /* USE_GD_GIF */
        }
#else
        msSetError(MS_MISCERR, "Raster reprojection supported only with the GDAL library.", "msDrawRasterLayer( GIF )");
        return(-1);
#endif  /* USE_GDAL */
      }
      else
      {
        // No reprojection...

        status = drawGIF(map, layer, img, filename);
        if(status == -1) {
          chdir(old_path); /* restore old cwd */
          return(-1);
        }
        continue;
      }
    }

#if !defined(USE_GDAL) || defined(USE_GD_PNG)
    if (memcmp(dd,PNGsig,8)==0) {
      if(layer->transform && 
         msProjectionsDiffer(&(map->projection), &(layer->projection))) {
#ifdef USE_GDAL
        /*
        ** Image should be reprojected... We'll set the offsite to the PNG
        ** transparent colour (since GDAL ignores it) and then let GDAL
        ** render the layer (if it has PNG support included).
        **
        ** __TODO__ Yes, this is a hack and should be improved...
        **          but hey!  It works for now.
        */
        if (layer->offsite == -1)
        {
#ifdef USE_GD_PNG
            FILE *pngStream;
            gdImagePtr png;

            pngStream = fopen(filename, "rb");
            if(!pngStream) {
                msSetError(MS_IOERR, "Error open image file.", "drawPNG()");
                chdir(old_path); /* restore old cwd */
                return(-1);
            }
            png = gdImageCreateFromPng(pngStream);
            fclose(pngStream);

            if(!png) {
                msSetError(MS_IMGERR, "Error loading PNG file.", "drawPNG()");
                chdir(old_path); /* restore old cwd */
                return(-1);
            }
            layer->offsite = gdImageGetTransparent(png);  // both default to -1
            gdImageDestroy(png);
#else
            msSetError(MS_MISCERR, "PNG Format not supported.", "msDrawRasterLayer( PNG )");
            return(-1);
#endif /* USE_GD_PNG */
        }
#else
        msSetError(MS_MISCERR, "Raster reprojection supported only with the GDAL library.", "msDrawRasterLayer( PNG )");
        return(-1);
#endif
      }
      else
      {
        // No reprojection...

        status = drawPNG(map, layer, img, filename);
        if(status == -1) {
	  chdir(old_path); /* restore old cwd */
	  return(-1);
        }
        continue;
      }
    }
#endif /* !defined(USE_GDAL) || defined(USE_GD_PNG) */

#if !defined(USE_GDAL) || defined(USE_JPEG)
    if (memcmp(dd,JPEGsig,3)==0) 
    {
      if((layer->transform && 
          msProjectionsDiffer(&(map->projection), &(layer->projection))) ||
          layer->connectiontype == MS_WMS )
      {
        // If reprojection requested, or layer is a WMS connection then we
        // want to delegate JPEG drawing to GDAL because drawJPEG() supports
        // only greyscale jpegs and doesn't support reprojections.
#ifdef USE_GDAL
        // Do nothing here... GDAL will pick the image later.
#else
        msSetError(MS_MISCERR, "Raster reprojection supported only with the GDAL library.", "msDrawRasterLayer( JPEG )");
        return(-1);
#endif
      }
      else
      {
        // No reprojection and not a WMS connection
        status = drawJPEG(map, layer, img, filename);
        if(status == -1) {
	  chdir(old_path); /* restore old cwd */
          return(-1);
        }
        continue;
      }
    }
#endif /* !defined(USE_GDAL) || defined(USE_JPEG) */

    if (memcmp(dd,"HEAD",4)==0) {
      if(layer->transform && msProjectionsDiffer(&(map->projection), &(layer->projection))) {
        msSetError(MS_MISCERR, "Raster reprojection supported only with the GDAL library.", "msDrawRasterLayer( ERD )");
        return(-1);
      }
      status = drawERD(map, layer, img, filename);
      if(status == -1) {
	chdir(old_path); /* restore old cwd */
	return(-1);
      }
      continue;
    }
    
#ifdef USE_EGIS
    // OV - egis- Call Generic format function here
    // Note : modify this to find elegent way of deciding its generic
    // May be put any flag?
    
    ext = strstr(filename, ".img");
    if (strcmp(ext, ".img") == 0)
      {
      if(layer->transform && msProjectionsDiffer(&(map->projection, &(layer->projection))) {
          msSetError(MS_MISCERR, "Raster reprojection supported only with the GDAL library.", "msDrawRasterLayer( EGIS )");
          return(-1);
        }
	status = drawGEN(map, layer, img, filename);
	chdir(old_path); // restore old cwd
	
	//ext = NULL;
	return(status);
      }
#endif

#ifdef USE_GDAL
    {
        GDALDatasetH  hDS;
        static int    bGDALInitialized = 0;

        if( !bGDALInitialized )
        {
            GDALAllRegister();
            CPLPushErrorHandler( CPLQuietErrorHandler );
            bGDALInitialized = 1;
        }
        
        msAcquireLock( TLOCK_GDAL );
        hDS = GDALOpen( filename, GA_ReadOnly );
        if( hDS != NULL )
        {
            double	adfGeoTransform[6];

            if (layer->projection.numargs > 0 && 
                EQUAL(layer->projection.args[0], "auto"))
            {
                const char *pszWKT;

                pszWKT = GDALGetProjectionRef( hDS );

                if( pszWKT != NULL && strlen(pszWKT) > 0 )
                {
                    if (msOGCWKT2ProjectionObj(pszWKT,
                                     &(layer->projection)) != MS_SUCCESS)
                    {
                        char	szLongMsg[MESSAGELENGTH*2];
                        errorObj *ms_error = msGetErrorObj();

                        sprintf( szLongMsg, 
                                 "%s\n"
                                 "PROJECTION AUTO cannot be used for this "
                                 "GDAL raster (`%s').",
                                 ms_error->message, filename);
                        szLongMsg[MESSAGELENGTH-1] = '\0';

                        msSetError(MS_OGRERR, "%s","msDrawRasterLayer()",
                                   szLongMsg);

                        msReleaseLock( TLOCK_GDAL );
                        return(MS_FAILURE);
                    }
                }
            }

            if (GDALGetGeoTransform( hDS, adfGeoTransform ) != CE_None)
            {
                GDALReadWorldFile(filename, "wld", adfGeoTransform);
            }

            /* 
            ** We want to resample if the source image is rotated, or if
            ** the projections differ.  However, due to limitations in 
            ** msResampleGDALToMap() we can only resample rotated images
            ** if they also have fully defined projections.
            */
#ifdef USE_PROJ
            if( ((adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0)
                 && map->projection.proj != NULL 
                 && layer->projection.proj != NULL)
                || msProjectionsDiffer( &(map->projection), 
                                        &(layer->projection) ) )
            {
                status = msResampleGDALToMap( map, layer, img, hDS );
            }
            else
#endif
            {
                status = drawGDAL(map, layer, img, hDS );
            }
            msReleaseLock( TLOCK_GDAL );

            if( status == -1 )
            {
                chdir(old_path); /* restore old cwd */
                continue;
            }

            GDALClose( hDS );
            continue;
        }
    }
#endif

    /* If GDAL doesn't recognise it, and it wasn't successfully opened 
    ** Generate an error.
    */
    if( !f ) {
      msSetError(MS_IOERR, "(%s)", "msDrawRaster()", filename);
#ifndef IGNORE_MISSING_DATA
      chdir(old_path);
      return(-1);
#else
      continue; // skip it, next tile
#endif
    }

    /* put others which may require checks here */  
    
    /* No easy check for EPPL so put here */      
    status=drawEPP(map, layer, img, filename);
    if(status != 0) {
      if (status == -2) msSetError(MS_IMGERR, "Error reading EPPL file; probably corrupt.", "msDrawEPP()");
      if (status == -1) msSetError(MS_IMGERR, "Unrecognized or unsupported image format", "msDrawRaster()");
      chdir(old_path); /* restore old cwd */
      return(-1);
    }
    continue;
  } // next tile

  if(layer->tileindex) /* tiling clean-up */
    msSHPCloseFile(&tilefile);    

  chdir(old_path); /* restore old cwd */
  return 0;
  
}

//TODO : this will msDrawReferenceMapGD
imageObj *msDrawReferenceMap(mapObj *map) {
  FILE *stream;
  gdImagePtr img=NULL;
  double cellsize;
  int c=-1, oc=-1;
  int x1,y1,x2,y2;

  char bytes[8];
  imageObj      *image = NULL;

  image = (imageObj *)malloc(sizeof(imageObj));
  
  stream = fopen(map->reference.image,"rb"); // allocate input and output images (same size)
  if(!stream) {
    msSetError(MS_IOERR, "(%s)", "msDrawReferenceMap()",
               map->reference.image);
    return(NULL);
  }

  fread(bytes,8,1,stream); // read some bytes to try and identify the file
  rewind(stream); // reset the image for the readers
  if (memcmp(bytes,"GIF8",4)==0) {
#ifdef USE_GD_GIF
    img = gdImageCreateFromGif(stream);
    map->reference.imagetype = MS_GIF;
    image->imagetype = MS_GIF;
    image->img.gd = img;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->width = img->sx;
    image->height = img->sy;
        
    if (map->web.imagepath)
       image->imagepath = strdup(map->web.imagepath);
    if (map->web.imageurl)
       image->imageurl = strdup(map->web.imageurl);
    
#else
    msSetError(MS_MISCERR, "Unable to load GIF reference image.", "msDrawReferenceMap()");
    fclose(stream);
    return(NULL);
#endif
  } else if (memcmp(bytes,PNGsig,8)==0) {
#ifdef USE_GD_PNG
    img = gdImageCreateFromPng(stream);
    map->reference.imagetype = MS_PNG;
    image->imagetype = MS_PNG;
    image->img.gd = img;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->width = img->sx;
    image->height = img->sy;
    if (map->web.imagepath)
       image->imagepath = strdup(map->web.imagepath);
    if (map->web.imageurl)
       image->imageurl = strdup(map->web.imageurl);
#else
    msSetError(MS_MISCERR, "Unable to load PNG reference image.", "msDrawReferenceMap()");
    fclose(stream);
    return(NULL);
#endif
  } else if (memcmp(bytes,JPEGsig,3)==0) {
#ifdef USE_GD_JPEG
    img = gdImageCreateFromJpeg(stream);
    map->reference.imagetype = MS_JPEG;
    image->imagetype = MS_JPEG;
    image->img.gd = img;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->width = img->sx;
    image->height = img->sy;
    if (map->web.imagepath)
       image->imagepath = strdup(map->web.imagepath);
    if (map->web.imageurl)
       image->imageurl = strdup(map->web.imageurl);
#else
    msSetError(MS_MISCERR, "Unable to load JPEG reference image.", "msDrawReferenceMap()");
    fclose(stream);
    return(NULL);
#endif
  }

  if(!img) {
    msSetError(MS_GDERR, "Unable to initialize image.", "msDrawReferenceMap()");
    fclose(stream);
    return(NULL);
  }

  cellsize = msAdjustExtent(&(map->reference.extent), img->sx, img->sy); // make sure the extent given in mapfile fits the image

  // allocate some colors
  if(map->reference.outlinecolor.red != -1 && map->reference.outlinecolor.green != -1 && map->reference.outlinecolor.blue != -1)
    oc = gdImageColorAllocate(img, map->reference.outlinecolor.red, map->reference.outlinecolor.green, map->reference.outlinecolor.blue);
  if(map->reference.color.red != -1 && map->reference.color.green != -1 && map->reference.color.blue != -1)
    c = gdImageColorAllocate(img, map->reference.color.red, map->reference.color.green, map->reference.color.blue); 

  // convert map extent to reference image coordinates
  x1 = MS_MAP2IMAGE_X(map->extent.minx,  map->reference.extent.minx, cellsize);
  x2 = MS_MAP2IMAGE_X(map->extent.maxx,  map->reference.extent.minx, cellsize);
  y1 = MS_MAP2IMAGE_Y(map->extent.miny,  map->reference.extent.maxy, cellsize);
  y2 = MS_MAP2IMAGE_Y(map->extent.maxy,  map->reference.extent.maxy, cellsize);
  
  // if extent are smaller than minbox size
  // draw that extent on the reference image
  if( (abs(x2 - x1) > map->reference.minboxsize) || 
          (abs(y2 - y1) > map->reference.minboxsize) )
  {
      if( map->reference.maxboxsize == 0 || 
              ((abs(x2 - x1) < map->reference.maxboxsize) && 
                  (abs(y2 - y1) < map->reference.maxboxsize)) )
      {
          if(c != -1)
              gdImageFilledRectangle(img,x1,y1,x2,y2,c);
          if(oc != -1)
              gdImageRectangle(img,x1,y1,x2,y2,oc);
      }
  
  }
  else // else draw the marker symbol
  {
      if( map->reference.maxboxsize == 0 || 
              ((abs(x2 - x1) < map->reference.maxboxsize) && 
                  (abs(y2 - y1) < map->reference.maxboxsize)) )
      {
          //if the marker symbol is specify draw this symbol else draw a cross
          if(map->reference.marker != 0)
          {
              pointObj *point = NULL;

              point = malloc(sizeof(pointObj));
              point->x = (double)x1 + x2 - x1;
              point->y = (double)y1 + y2 - y1;

              msDrawMarkerSymbol(&map->symbolset, image, point, 
                                 map->reference.marker, c, -1, oc, 
                                 map->reference.markersize);
              free(point);
          }
          else if(map->reference.markername != NULL)
          {
              int marker = msGetSymbolIndex(&map->symbolset, 
                                            map->reference.markername);
              pointObj *point = NULL;

              point = malloc(sizeof(pointObj));
              point->x = (double)x1 + ((x2 - x1) / 2);
              point->y = (double)y1 + ((y2 - y1) / 2);

              msDrawMarkerSymbol(&map->symbolset,image,point, marker, c, -1, oc, 
                                 map->reference.markersize);
              free(point);
          }
          else
          {
              int x21, y21;
              //determine the center point
              x21 = x1 + ((x2 - x1) / 2);
              y21 = y1 + ((y2 - y1) / 2);

              //get the color
              if(c != -1)
                  oc = c;

              //draw a cross
              if(c != -1)
              {
                  gdImageLine(img, x21-8, y21, x21-3, y21, oc);
                  gdImageLine(img, x21, y21-8, x21, y21-3, c);
                  gdImageLine(img, x21, y21+3, x21, y21+8, c);
                  gdImageLine(img, x21+3, y21, x21+8, y21, c);
              }
          }
      }
  }

  fclose(stream);
  return(image);
}

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  msDrawRasterLayer(): generic raster layer drawing, including
 *           implementation of non-GDAL raster layer renderers.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *           Pete Olson (LMIC)            
 *           Steve Lime
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

#include <assert.h>
#include "mapserver.h"
#include "mapfile.h"
#include "mapresample.h"
#include "mapthread.h"

MS_CVSID("$Id$")

extern int msyyparse(void);
extern int msyylex(void);
extern char *msyytext;

extern int msyyresult; /* result of parsing, true/false */
extern int msyystate;
extern char *msyystring;

#ifdef USE_TIFF
#include <tiffio.h>
#include <tiff.h>
#endif

#ifdef USE_GDAL
#include "gdal.h"
#include "cpl_string.h"
#endif

#ifdef USE_EPPL
#include "epplib.h"
#endif

#ifdef USE_JPEG
#include "jpeglib.h"
#endif


#define MAXCOLORS 256
#define BUFLEN 1024
#define HDRLEN 8

#define	CVT(x) ((x) >> 8) /* converts to 8-bit color value */

#define NUMGRAYS 16

/* FIX: need WBMP sig */
unsigned char PNGsig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; /* 89 50 4E 47 0D 0A 1A 0A hex */
unsigned char JPEGsig[3] = {255, 216, 255}; /* FF D8 FF hex */

/************************************************************************/
/*                             msGetClass()                             */
/************************************************************************/

int msGetClass(layerObj *layer, colorObj *color)
{
  int i;
  char *tmpstr1=NULL;
  char tmpstr2[12]; /* holds either a single color index or something like 'rrr ggg bbb' */
  int status;
  int expresult;
  
  if((layer->numclasses == 1) && !(layer->class[0]->expression.string)) /* no need to do lookup */
    return(0);

  if(!color) return(-1);

  for(i=0; i<layer->numclasses; i++) {
    if (layer->class[i]->expression.string == NULL) /* Empty expression - always matches */
      return(i);
    switch(layer->class[i]->expression.type) {
    case(MS_STRING):
      sprintf(tmpstr2, "%d %d %d", color->red, color->green, color->blue);
      if(strcmp(layer->class[i]->expression.string, tmpstr2) == 0) return(i); /* matched */
      sprintf(tmpstr2, "%d", color->pen);
      if(strcmp(layer->class[i]->expression.string, tmpstr2) == 0) return(i); /* matched */
      break;
    case(MS_REGEX):
      if(!layer->class[i]->expression.compiled) {
	if(ms_regcomp(&(layer->class[i]->expression.regex), layer->class[i]->expression.string, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) { /* compile the expression  */
	  msSetError(MS_REGEXERR, "Invalid regular expression.", "msGetClass()");
	  return(-1);
	}
	layer->class[i]->expression.compiled = MS_TRUE;
      }

      sprintf(tmpstr2, "%d %d %d", color->red, color->green, color->blue);
      if(ms_regexec(&(layer->class[i]->expression.regex), tmpstr2, 0, NULL, 0) == 0) return(i); /* got a match */
      sprintf(tmpstr2, "%d", color->pen);
      if(ms_regexec(&(layer->class[i]->expression.regex), tmpstr2, 0, NULL, 0) == 0) return(i); /* got a match */
      break;
    case(MS_EXPRESSION):
      tmpstr1 = strdup(layer->class[i]->expression.string);

      sprintf(tmpstr2, "%d", color->red);
      tmpstr1 = msReplaceSubstring(tmpstr1, "[red]", tmpstr2);
      sprintf(tmpstr2, "%d", color->green);
      tmpstr1 = msReplaceSubstring(tmpstr1, "[green]", tmpstr2);
      sprintf(tmpstr2, "%d", color->blue);
      tmpstr1 = msReplaceSubstring(tmpstr1, "[blue]", tmpstr2);

      sprintf(tmpstr2, "%d", color->pen);
      tmpstr1 = msReplaceSubstring(tmpstr1, "[pixel]", tmpstr2);

      msAcquireLock( TLOCK_PARSER );
      msyystate = MS_TOKENIZE_EXPRESSION; msyystring = tmpstr1;
      status = msyyparse();
      expresult = msyyresult;
      msReleaseLock( TLOCK_PARSER );

      free(tmpstr1);

      if( status != 0 ) return -1; /* error parsing expression. */
      if( expresult ) return i;    /* got a match? */
    }
  }

  return(-1); /* not found */
}

/************************************************************************/
/*                          msGetClass_Float()                          */
/*                                                                      */
/*      Returns the class based on classification of a floating         */
/*      pixel value.                                                    */
/************************************************************************/

int msGetClass_Float(layerObj *layer, float fValue)
{
    int i;
    char *tmpstr1=NULL;
    char tmpstr2[100];
    int status, expresult;
  
    if((layer->numclasses == 1) && !(layer->class[0]->expression.string)) /* no need to do lookup */
        return(0);

    for(i=0; i<layer->numclasses; i++) {
        if (layer->class[i]->expression.string == NULL) /* Empty expression - always matches */
            return(i);

        switch(layer->class[i]->expression.type) {
          case(MS_STRING):
            sprintf(tmpstr2, "%18g", fValue );
            /* trim junk white space */
            tmpstr1= tmpstr2;
            while( *tmpstr1 == ' ' )
                tmpstr1++;

            if(strcmp(layer->class[i]->expression.string, tmpstr1) == 0) return(i); /* matched */
            break;

          case(MS_REGEX):
            if(!layer->class[i]->expression.compiled) {
                if(ms_regcomp(&(layer->class[i]->expression.regex), layer->class[i]->expression.string, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) { /* compile the expression  */
                    msSetError(MS_REGEXERR, "Invalid regular expression.", "msGetClass()");
                    return(-1);
                }
                layer->class[i]->expression.compiled = MS_TRUE;
            }

            sprintf(tmpstr2, "%18g", fValue );
            if(ms_regexec(&(layer->class[i]->expression.regex), tmpstr2, 0, NULL, 0) == 0) return(i); /* got a match */
            break;

          case(MS_EXPRESSION):
            tmpstr1 = strdup(layer->class[i]->expression.string);

            sprintf(tmpstr2, "%18g", fValue);
            tmpstr1 = msReplaceSubstring(tmpstr1, "[pixel]", tmpstr2);

            msAcquireLock( TLOCK_PARSER );
            msyystate = MS_TOKENIZE_EXPRESSION; msyystring = tmpstr1;
            status = msyyparse();
            expresult = msyyresult;
            msReleaseLock( TLOCK_PARSER );

            free(tmpstr1);

            if( status != 0 ) return -1;
            if( expresult ) return i;
        }
    }

    return(-1); /* not found */
}

/************************************************************************/
/*                            msAddColorGD()                            */
/*                                                                      */
/*      Function to add a color to an existing color map.  It first     */
/*      looks for an exact match, then tries to add it to the end of    */
/*      the existing color map, and if all else fails it finds the      */
/*      closest color.                                                  */
/************************************************************************/

int msAddColorGD(mapObj *map, gdImagePtr img, int cmt, int r, int g, int b)
{
  int c; 
  int ct = -1;
  int op = -1;
  long rd, gd, bd, dist;
  long mindist = 3*255*255;  /* init to max poss dist */

  if( gdImageTrueColor( img ) )
      return gdTrueColor( r, g, b );
  
  /*
  ** We want to avoid using a color that matches a transparent background
  ** color exactly.  If this is the case, we will permute the value slightly.
  ** When perterbing greyscale images we try to keep them greyscale, otherwise
  ** we just perterb the red component.
  */
  if( map->outputformat && map->outputformat->transparent 
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
    if (map->outputformat && map->outputformat->transparent 
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
          dist = rd*rd;
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

  /* no exact match, is the closest within our "color match threshold"? */
  if( mindist <= cmt*cmt )
      return ct;

  /* no exact match.  If there are no open colors we return the closest
     color found.  */
  if (op == -1) {
    op = img->colorsTotal;
    if (op == gdMaxColors) { /* No room for more colors */
      return ct; /* Return closest available color */
    }
    img->colorsTotal++;
  }

  /* allocate a new exact match */
  img->red  [op] = r;
  img->green[op] = g;
  img->blue [op] = b;
  img->open [op] = 0;

  return op; /* Return newly allocated color */  
}

#ifdef USE_AGG
int msAddColorAGG(mapObj *map, gdImagePtr img, int cmt, int r, int g, int b)
{
	 return msAddColorGD( map, img, cmt, r, g, b );
}
#endif

/************************************************************************/
/*                           readWorldFile()                            */
/*                                                                      */
/*      Function to read georeferencing information for an image        */
/*      from an ESRI world file.                                        */
/************************************************************************/

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

/************************************************************************/
/*                            readGEOTiff()                             */
/*                                                                      */
/*      read georeferencing info from geoTIFF header, if it exists      */
/************************************************************************/

#ifdef USE_TIFF 
static int readGEOTiff(TIFF *tif, double *ulx, double *uly, double *cx, double *cy)
{
  short entries;
  int i,fpos,swap;
  uint32 tiepos, cellpos;
  double tie[6],cell[6];
  TIFFDirEntry tdir;
  FILE *f;
  
  swap=TIFFIsByteSwapped(tif);
  fpos=TIFFCurrentDirOffset(tif);
  f=fopen((char*)TIFFFileName(tif),"rb");
  if (f==NULL) return(-1);
  fseek(f,fpos,0);
  fread(&entries,2,1,f);
  if (swap) TIFFSwabShort(&entries);
  tiepos=0; cellpos=0;
  for (i=0; i<entries; i++) {
    fread(&tdir,sizeof(tdir),1,f);
    if (swap) TIFFSwabShort((unsigned short *) &tdir.tdir_tag);
    if (tdir.tdir_tag==0x8482) {  /* geoTIFF tie point */
      tiepos=tdir.tdir_offset;
      if (swap) TIFFSwabLong(&tiepos);
      }
    if (tdir.tdir_tag==0x830e) {  /* geoTIFF cell size */
      cellpos=tdir.tdir_offset;
      if (swap) TIFFSwabLong(&cellpos);
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
  
/************************************************************************/
/*                              drawTIFF()                              */
/************************************************************************/
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

  colorObj pixel;

  double startx, starty; /* this is where we start out reading */
  int st,strip;  /* current strip */
  int xi,yi,startj,endj,boffset,vv;

  double ulx, uly; /* upper left-hand coordinates */
  double skipx,skipy; /* skip factors (x and y) */  
  double cx,cy; /* cell sizes (x and y) */
  long rowsPerStrip;    

  char szPath[MS_MAXPATHLEN];

  if( layer->debug )
      msDebug( "drawTIFF(%s): entering\n", layer->name );

  for(i=0; i<MAXCOLORS; i++) cmap[i] = -1; /* initialize the colormap to all transparent */

  TIFFSetWarningHandler(NULL); /* can these be directed to the mapserver error functions? */
  TIFFSetErrorHandler(NULL);

  tif = TIFFOpen(msBuildPath3(szPath, map->mappath, map->shapepath, filename), "rb");
  if(!tif) {
    msSetError(MS_IMGERR, "Error loading TIFF image.", "drawTIFF()");
    return(-1);
  }
  
  if(readGEOTiff(tif, &ulx, &uly, &cx, &cy ) != 0) {
    if(readWorldFile(msBuildPath3(szPath,map->mappath,map->shapepath,filename),
                     &ulx, &uly, &cx, &cy) != 0) {
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
      int c;

      for(i=0; i<MAXCOLORS; i++) {

	pixel.red = CVT(red[i]);
	pixel.green = CVT(green[i]);
	pixel.blue = CVT(blue[i]);
	pixel.pen = i;

	if(!MS_COMPARE_COLORS(pixel, layer->offsite)) {	  
	  c = msGetClass(layer, &pixel);
	  
	  if(c == -1) /* doesn't belong to any class, so handle like offsite */
	    cmap[i] = -1;
	  else {
            RESOLVE_PEN_GD(img, layer->class[c]->styles[0]->color);
	    if(MS_VALID_COLOR(layer->class[c]->styles[0]->color)) 
	      cmap[i] = msAddColorGD(map,img, 0, layer->class[c]->styles[0]->color.red, layer->class[c]->styles[0]->color.green, layer->class[c]->styles[0]->color.blue); /* use class color */
	    else if(MS_TRANSPARENT_COLOR(layer->class[c]->styles[0]->color)) 
	      cmap[i] = -1; /* make transparent */
	    else
              cmap[i] = msAddColorGD(map,img, 0, pixel.red, pixel.green, pixel.blue); /* use raster color */
	  }
	} 
      }
    } else {
      for(i=0; i<MAXCOLORS; i++) {

	pixel.red = CVT(red[i]);
	pixel.green = CVT(green[i]);
	pixel.blue = CVT(blue[i]);
	pixel.pen = i;

	if(!MS_COMPARE_COLORS(pixel, layer->offsite))
	  cmap[i] = msAddColorGD(map,img, 0, pixel.red, pixel.green, pixel.blue); /* use raster color         */
      }
    }
    break;
  case PHOTOMETRIC_MINISBLACK: /* classes are NOT honored for non-colormapped data */
    if (nbits==1) {
      for (i=0; i<2; i++) {
	pixel.red = pixel.green = pixel.blue = pixel.pen = i; /* offsite would be specified as 0 or 1 */

	if(!MS_COMPARE_COLORS(pixel, layer->offsite))
          cmap[i]=msAddColorGD(map,img, 0, i*255,i*255,i*255); /* use raster color, stretched to use entire grayscale range */
      }      
    } else { 
      if (nbits==4) {	
	for (i=0; i<16; i++) {
	  pixel.red = pixel.green = pixel.blue = pixel.pen = i; /* offsite would be specified in range 0 to 15 */

	  if(!MS_COMPARE_COLORS(pixel, layer->offsite))
	    cmap[i] = msAddColorGD(map,img,0, i*17, i*17, i*17); /* use raster color, stretched to use entire grayscale range	   */
	}
      } else { /* 8-bit */
	for (i=0; i<256; i++) {
	  pixel.red = pixel.green = pixel.blue = pixel.pen = i; /* offsite would be specified in range 0 to 255 */

	  if(!MS_COMPARE_COLORS(pixel, layer->offsite))	    
	    cmap[i] = msAddColorGD(map,img, 0, (i>>4)*17, (i>>4)*17, (i>>4)*17); /* use raster color */
	}
      }
    }
    break;
  case PHOTOMETRIC_MINISWHITE: /* classes are NOT honored for non-colormapped data */
    if (nbits==1) {
      for (i=0; i<2; i++) {
	pixel.red = pixel.green = pixel.blue = pixel.pen = i; /* offsite would be specified as 0 or 1 */

	if(!MS_COMPARE_COLORS(pixel, layer->offsite))
	  cmap[i]=msAddColorGD(map,img, 0, i*255,i*255,i*255); /* use raster color, stretched to use entire grayscale range */
      }      
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

/************************************************************************/
/*                              drawPNG()                               */
/************************************************************************/
static int drawPNG(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_GD_PNG
  int i,j; /* loop counters */
  double x,y;
  int cmap[MAXCOLORS];
  double w, h;

  FILE *pngStream;
  gdImagePtr png;

  int vv;

  colorObj pixel;

  double startx, starty; /* this is where we start out reading */

  double ulx, uly; /* upper left-hand coordinates */
  double skipx,skipy; /* skip factors (x and y) */  
  double cx,cy; /* cell sizes (x and y) */
  char szPath[MS_MAXPATHLEN];

  for(i=0; i<MAXCOLORS; i++) cmap[i] = -1; /* initialize the colormap to all transparent */

  pngStream = fopen(msBuildPath3(szPath, map->mappath, map->shapepath, filename), "rb");
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
    if(readWorldFile(msBuildPath3(szPath, map->mappath,map->shapepath,filename), 
                     &ulx, &uly, &cx, &cy) != 0)
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
    int c;

    for(i=0; i<gdImageColorsTotal(png); i++) {

      pixel.red = gdImageRed(png,i);
      pixel.green = gdImageGreen(png,i);
      pixel.blue = gdImageBlue(png,i);
      pixel.pen = i; 

      if(!MS_COMPARE_COLORS(pixel, layer->offsite) && i != gdImageGetTransparent(png)) {	
	c = msGetClass(layer, &pixel);

	if(c == -1) /* doesn't belong to any class, so handle like offsite */
	  cmap[i] = -1;
	else {
          RESOLVE_PEN_GD(img, layer->class[c]->styles[0]->color);
          if(MS_VALID_COLOR(layer->class[c]->styles[0]->color)) 
	    cmap[i] = msAddColorGD(map,img, 0, layer->class[c]->styles[0]->color.red, layer->class[c]->styles[0]->color.green, layer->class[c]->styles[0]->color.blue); /* use class color */
	  else if(MS_TRANSPARENT_COLOR(layer->class[c]->styles[0]->color)) 
	    cmap[i] = -1; /* make transparent */
	  else
            cmap[i] = msAddColorGD(map,img, 0, pixel.red, pixel.green, pixel.blue); /* use raster color	   */
	}
      }
    }
  } else {
    for(i=0; i<gdImageColorsTotal(png); i++) {

      pixel.red = gdImageRed(png,i);
      pixel.green = gdImageGreen(png,i);
      pixel.blue = gdImageBlue(png,i);
      pixel.pen = i;

      if(!MS_COMPARE_COLORS(pixel, layer->offsite) && i != gdImageGetTransparent(png)) 
	cmap[i] = msAddColorGD(map,img, 0, pixel.red, pixel.green, pixel.blue);
    }
  }

  y=starty;
  for(i=0; i<img->sy; i++) { /* for each row */
    if((y >= -0.5) && (y < h)) {
      x = startx;
      for(j=0; j<img->sx; j++) {
	if((x >= -0.5) && (x < w)) {
	  vv = png->pixels[(y == -0.5)?0:MS_NINT(y)][(x == -0.5)?0:MS_NINT(x)];	  
	  if(cmap[vv] != -1)
	    img->pixels[i][j] = cmap[vv];
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

/************************************************************************/
/*                              drawGIF()                               */
/************************************************************************/
static int drawGIF(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_GD_GIF
  int i,j; /* loop counters */
  double x,y;
  int cmap[MAXCOLORS];
  double w, h;

  FILE *gifStream;
  gdImagePtr gif;

  int vv;
 
  colorObj pixel;

  double startx, starty; /* this is where we start out reading */

  double ulx, uly; /* upper left-hand coordinates */
  double skipx,skipy; /* skip factors (x and y) */  
  double cx,cy; /* cell sizes (x and y) */
  char szPath[MS_MAXPATHLEN];

  for(i=0; i<MAXCOLORS; i++) cmap[i] = -1; /* initialize the colormap to all transparent */

  gifStream = fopen(msBuildPath3(szPath, map->mappath, map->shapepath, filename), "rb");
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
    if(readWorldFile(msBuildPath3(szPath,map->mappath,map->shapepath,filename),
                     &ulx, &uly, &cx, &cy) != 0)
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
    int c;

    for(i=0; i<gdImageColorsTotal(gif); i++) {

      pixel.red = gdImageRed(gif,i);
      pixel.green = gdImageGreen(gif,i);
      pixel.blue = gdImageBlue(gif,i);	
      pixel.pen = i;

      if(!MS_COMPARE_COLORS(pixel, layer->offsite) && i != gdImageGetTransparent(gif)) {	
	c = msGetClass(layer, &pixel);

        if(c == -1) /* doesn't belong to any class, so handle like offsite */
	  cmap[i] = -1;
	else {
          RESOLVE_PEN_GD(img, layer->class[c]->styles[0]->color);
          if(MS_VALID_COLOR(layer->class[c]->styles[0]->color)) 
	    cmap[i] = msAddColorGD(map,img, 0, layer->class[c]->styles[0]->color.red, layer->class[c]->styles[0]->color.green, layer->class[c]->styles[0]->color.blue); /* use class color */
	  else if(MS_TRANSPARENT_COLOR(layer->class[c]->styles[0]->color)) 
	    cmap[i] = -1; /* make transparent */
	  else
            cmap[i] = msAddColorGD(map,img, 0, pixel.red, pixel.green, pixel.blue); /* use raster color	   */
	}	
      }
    }
  } else {
    for(i=0; i<gdImageColorsTotal(gif); i++) {

      pixel.red = gdImageRed(gif,i);
      pixel.green = gdImageGreen(gif,i);
      pixel.blue = gdImageBlue(gif,i);
      pixel.pen = i;

      if(!MS_COMPARE_COLORS(pixel, layer->offsite) && i != gdImageGetTransparent(gif)) 
	cmap[i] = msAddColorGD(map,img, 0, pixel.red, pixel.green, pixel.blue);      
    }    
  }

  y=starty;
  for(i=0; i<img->sy; i++) { /* for each row */
    if((y >= -0.5) && (y < h)) {
      x = startx;
      for(j=0; j<img->sx; j++) {
	if((x >= -0.5) && (x < w)) {
	  vv = gif->pixels[(y == -0.5)?0:MS_NINT(y)][(x == -0.5)?0:MS_NINT(x)];	  
	  if(cmap[vv] != -1)
	    img->pixels[i][j] = cmap[vv];
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

/************************************************************************/
/*                              drawJPEG()                              */
/************************************************************************/

static int drawJPEG(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_JPEG
  int i,j,k; /* loop counters */  
  int cmap[MAXCOLORS];
  double w, h;

  unsigned char vv;
  JSAMPARRAY buffer;

  double startx, starty; /* this is where we start out reading */
  double skipx,skipy; /* skip factors (x and y) */ 
  double x,y;

  double ulx, uly; /* upper left-hand coordinates */  
  double cx,cy; /* cell sizes (x and y) */

  colorObj pixel;

  FILE *jpegStream;

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  char szPath[MS_MAXPATHLEN];

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpegStream = fopen(msBuildPath3(szPath, map->mappath, map->shapepath, filename), "rb");
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

  /* set up the color map */
  for (i=0; i<MAXCOLORS; i++) {
    pixel.red = pixel.green = pixel.blue = pixel.pen = i;

    cmap[i] = -1; /* initialize to transparent */
    if(!MS_COMPARE_COLORS(pixel, layer->offsite))      
      cmap[i] = msAddColorGD(map,img, 0, (i>>4)*17,(i>>4)*17,(i>>4)*17);
  }

  if(readWorldFile(msBuildPath3(szPath, map->mappath, map->shapepath,filename),
                   &ulx, &uly, &cx, &cy) != 0)
    return(-1);
  
  skipx = map->cellsize/cx;
  skipy = map->cellsize/cy;

  /* muck with scaling */
  cinfo.scale_num = 1;
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

  /* initialize the decompressor, this sets output sizes etc... */
  jpeg_start_decompress(&cinfo);

  w = cinfo.output_width - .5; /* to avoid rounding problems */
  h = cinfo.output_height - .5;

  /* one pixel high buffer as wide as the image, goes away when done with the image */
  buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, cinfo.output_width, 1);

  /* skip any unneeded scanlines *yuck* */
  for(i=0; i<starty-1; i++)
    jpeg_read_scanlines(&cinfo, buffer, 1);

  y=starty;

  for(i=0; i<img->sy; i++) { /* for each row */
    if((y > -0.5) && (y < cinfo.output_height)) {
      
      jpeg_read_scanlines(&cinfo, buffer, 1);
      
      x = startx;
      for(j=0; j<img->sx; j++) {
	if((x >= -0.5) && (x < cinfo.output_width)) {
	  vv = buffer[0][(x == -0.5)?0:MS_NINT(x)];
	  if(cmap[vv] != -1)
	    img->pixels[i][j] = cmap[vv];
	}
	x+=skipx;
	if(x>=cinfo.output_width) /* next x is out of the image, so quit */
	  break;
      }
    }
    
    y+=skipy;

    if(y>=cinfo.output_height) /* next y is out of the image, so quit */
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

/************************************************************************/
/*                              drawEPP()                               */
/************************************************************************/
  
static int drawEPP(mapObj *map, layerObj *layer, gdImagePtr img, char *filename) 
{
#ifdef USE_EPPL
  int ncol,nrow,yi,rr,i,j,startj,endj,vv;
  double cx,cy,skipx,skipy,x,y,startx,starty;
  int cmap[MAXCOLORS];
  char szPath[MS_MAXPATHLEN];
  eppfile epp;
  TRGB color;
  clrfile clr;

  colorObj pixel;

  for(i=0; i<MAXCOLORS; i++) cmap[i] = -1; /* initialize the colormap to all transparent */

  strcpy(epp.filname,msBuildPath3(szPath, map->mappath, map->shapepath,
                                  filename));
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
  strcpy(clr.filname,msBuildPath3(szPath, map->mappath, map->shapepath,
                                  filename));
  if (!clrreset(&clr)) { /* use gray from min to max if no color file, classes not honored for greyscale */
    for (i=epp.minval; i<=epp.maxval; i++) {

      pixel.red = pixel.green = pixel.blue = pixel.pen = i;

      if(!MS_COMPARE_COLORS(pixel, layer->offsite)) {
	j=(((i-epp.minval)*16) / (epp.maxval-epp.minval+1))*17; 
	cmap[i]=msAddColorGD(map,img,0,j,j,j);
      }
    }
  } else {
    if(layer->numclasses > 0) {
      int c;
      
      for (i=epp.minval; i<=epp.maxval; i++) {

	pixel.red = color.red;
	pixel.green = color.green;
	pixel.blue = color.blue;
	pixel.pen = i;

	if(!MS_COMPARE_COLORS(pixel, layer->offsite)) {	  
	  c = msGetClass(layer, &pixel);
	  
	  if(c == -1) 
	    cmap[i] = -1;
	  else {
            RESOLVE_PEN_GD(img, layer->class[c]->styles[0]->color);
	    if(MS_VALID_COLOR(layer->class[c]->styles[0]->color)) 
	      cmap[i] = msAddColorGD(map,img, 0, layer->class[c]->styles[0]->color.red, layer->class[c]->styles[0]->color.green, layer->class[c]->styles[0]->color.blue); /* use class color */
	    else if(MS_TRANSPARENT_COLOR(layer->class[c]->styles[0]->color)) 
	      cmap[i] = -1; /* make transparent */
	    else {              
	      clrget(&clr,i,&color);
	      cmap[i] = msAddColorGD(map,img, 0, color.red, color.green, color.blue); /* use raster color */
	    }
	  }
	}
      }
    } else {
      for (i=epp.minval; i<=epp.maxval; i++) {
	pixel.red = color.red;
	pixel.green = color.green;
	pixel.blue = color.blue;
	pixel.pen = i;

	if(!MS_COMPARE_COLORS(pixel, layer->offsite)) {
	  clrget(&clr,i,&color);
	  cmap[i] = msAddColorGD(map,img, 0, color.red, color.green, color.blue);
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
  for (i=0; i<img->sy; i++) {
    yi=MS_NINT(y);
    if (yi>=0 && yi<nrow) {
      if (yi>rr+1) if (!position(&epp,yi+epp.fr)) return -2;
      if (yi>rr) if (!get_row(&epp)) return -2;
      rr=yi;
      x=startx;
      for (j=startj; j<endj; j++) {
        vv=epp.rptr[(int)MS_NINT(x)+1];

	if(cmap[vv] != -1)
	  img->pixels[i][j] = cmap[vv];

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
 
/************************************************************************/
/*                        msDrawRasterLayerLow()                        */
/*                                                                      */
/*      Check for various file types and act appropriately.  Handle     */
/*      tile indexing.                                                  */
/************************************************************************/

int msDrawRasterLayerLow(mapObj *map, layerObj *layer, imageObj *image) 
{
  int status, i, done;
  FILE *f;  
  char dd[8];
  char *filename=NULL, tilename[MS_MAXPATHLEN];

  layerObj *tlp=NULL; /* pointer to the tile layer either real or temporary */
  int tileitemindex=-1, tilelayerindex=-1;
  shapeObj tshp;

  int force_gdal;
  char szPath[MS_MAXPATHLEN];
  int final_status = MS_SUCCESS;

  rectObj searchrect;
  gdImagePtr img;
  char *pszTmp = NULL;
  char tiAbsFilePath[MS_MAXPATHLEN];
  char *tiAbsDirPath = NULL;

  if(layer->debug > 0 || map->debug > 1)
    msDebug( "msDrawRasterLayerLow(%s): entering.\n", layer->name );

  if(!layer->data && !layer->tileindex) {
    if(layer->debug == MS_TRUE) msDebug( "msDrawRasterLayerLow(%s): layer data and tileindex NULL ... doing nothing.", layer->name );
    return(0);
  }

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) {
    if(layer->debug == MS_TRUE) msDebug( "msDrawRasterLayerLow(%s): not status ON or DEFAULT, doing nothing.", layer->name );
    return(0);
  }

  if(map->scaledenom > 0) {
    if((layer->maxscaledenom > 0) && (map->scaledenom > layer->maxscaledenom)) {
      if(layer->debug == MS_TRUE) msDebug( "msDrawRasterLayerLow(%s): skipping, map scale %.2g > MAXSCALEDENOM=%g\n", layer->name, map->scaledenom, layer->maxscaledenom );
      return(0);
    }
    if((layer->minscaledenom > 0) && (map->scaledenom <= layer->minscaledenom)) {
      if(layer->debug == MS_TRUE) msDebug( "msDrawRasterLayerLow(%s): skipping, map scale %.2g < MINSCALEDENOM=%g\n", layer->name, map->scaledenom, layer->minscaledenom );
      return(0);
    }
  }

  if(layer->maxscaledenom <= 0 && layer->minscaledenom <= 0) {
    if((layer->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > layer->maxgeowidth)) {
      if(layer->debug == MS_TRUE) msDebug( "msDrawRasterLayerLow(%s): skipping, map width %.2g > MAXSCALEDENOM=%g\n", layer->name, 
          (map->extent.maxx - map->extent.minx), layer->maxgeowidth );
      return(0);
    }
    if((layer->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < layer->mingeowidth)) {
      if(layer->debug == MS_TRUE) msDebug( "msDrawRasterLayerLow(%s): skipping, map width %.2g < MINSCALEDENOM=%g\n", layer->name, 
          (map->extent.maxx - map->extent.minx), layer->mingeowidth );
      return(0);
    }
  }

  force_gdal = MS_FALSE;
  if(MS_RENDERER_GD(image->format))
    img = image->img.gd;
#ifdef USE_AGG
  else if(MS_RENDERER_AGG(image->format))
    img = image->img.gd;
#endif
  else {
    img = NULL;
    force_gdal = MS_TRUE;
  }

  /* Only GDAL supports 24bit GD output. */
  if(img != NULL && gdImageTrueColor(img)){
#ifndef USE_GDAL
    msSetError(MS_MISCERR, "Attempt to render raster layer to IMAGEMODE RGB or RGBA but\nwithout GDAL available.  24bit output requires GDAL.", "msDrawRasterLayerLow()" );
    return MS_FAILURE;
#else
    force_gdal = MS_TRUE;
#endif
  }

  /* Only GDAL support image warping. */
  if(layer->transform && msProjectionsDiffer(&(map->projection), &(layer->projection))) {
#ifndef USE_GDAL
    msSetError(MS_MISCERR, "Attempt to render raster layer that requires reprojection but\nwithout GDAL available.  Image reprojection requires GDAL.", "msDrawRasterLayerLow()" );
    return MS_FAILURE;
#else
    force_gdal = MS_TRUE;
#endif
  }

  /* This force use of GDAL if available.  This is usually but not always a */
  /* good idea.  Remove this line for local builds if necessary. */
#ifdef USE_GDAL 
  force_gdal = MS_TRUE;
#endif

  if(layer->tileindex) { /* we have an index file */
    
    msInitShape(&tshp);
    
    tilelayerindex = msGetLayerIndex(layer->map, layer->tileindex);
    if(tilelayerindex == -1) { /* the tileindex references a file, not a layer */

      /* so we create a temporary layer */
      tlp = (layerObj *) malloc(sizeof(layerObj));
      if(!tlp) {
        msSetError(MS_MEMERR, "Error allocating temporary layerObj.", "msDrawRasterLayerLow()");
        return(MS_FAILURE);
      }
      initLayer(tlp, map);

      /* set a few parameters for a very basic shapefile-based layer */
      tlp->name = strdup("TILE");
      tlp->type = MS_LAYER_TILEINDEX;
      tlp->data = strdup(layer->tileindex);
      if (layer->filteritem)
        tlp->filteritem = strdup(layer->filteritem);
      if (layer->filter.string)
      {
          if (layer->filter.type == MS_EXPRESSION)
          {
              pszTmp = 
                (char *)malloc(sizeof(char)*(strlen(layer->filter.string)+3));
              sprintf(pszTmp,"(%s)",layer->filter.string);
              msLoadExpressionString(&tlp->filter, pszTmp);
              free(pszTmp);
          }
          else if (layer->filter.type == MS_REGEX || 
                   layer->filter.type == MS_IREGEX)
          {
              pszTmp = 
                (char *)malloc(sizeof(char)*(strlen(layer->filter.string)+3));
              sprintf(pszTmp,"/%s/",layer->filter.string);
              msLoadExpressionString(&tlp->filter, pszTmp);
              free(pszTmp);
          }
          else
            msLoadExpressionString(&tlp->filter, layer->filter.string);
                   
          tlp->filter.type = layer->filter.type;
      }

    } else {
    	if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
			return MS_FAILURE;
	tlp = (GET_LAYER(layer->map, tilelayerindex));
    }
    status = msLayerOpen(tlp);
    if(status != MS_SUCCESS)
    {
        final_status = status;
        goto cleanup;
    }

    status = msLayerWhichItems(tlp, MS_FALSE, layer->tileitem);
    if(status != MS_SUCCESS)
    {
        final_status = status;
        goto cleanup;
    }
 
    /* get the tileitem index */
    for(i=0; i<tlp->numitems; i++) {
      if(strcasecmp(tlp->items[i], layer->tileitem) == 0) {
        tileitemindex = i;
        break;
      }
    }
    if(i == tlp->numitems) { /* didn't find it */
        msSetError(MS_MEMERR, 
                   "Could not find attribute %s in tileindex.", 
                   "msDrawRasterLayerLow()", 
                   layer->tileitem);
        final_status = MS_FAILURE;
        goto cleanup;
    }
 
    searchrect = map->extent;
#ifdef USE_PROJ
    /* if necessary, project the searchrect to source coords */
    if((map->projection.numargs > 0) && (layer->projection.numargs > 0)) msProjectRect(&map->projection, &layer->projection, &searchrect);
#endif
    status = msLayerWhichShapes(tlp, searchrect);
    if (status != MS_SUCCESS) {
        /* Can be either MS_DONE or MS_FAILURE */
        if (status != MS_DONE) 
            final_status = status;

        goto cleanup;
    }
  }

  done = MS_FALSE;
  while(done != MS_TRUE) { 
    if(layer->tileindex) {
      status = msLayerNextShape(tlp, &tshp);
      if( status == MS_FAILURE)
      {
          final_status = MS_FAILURE;
          break;
      }

      if(status == MS_DONE) break; /* no more tiles/images */
       
      if(layer->data == NULL || strlen(layer->data) == 0 ) /* assume whole filename is in attribute field */
          strcpy( tilename, tshp.values[tileitemindex] );
      else
          sprintf(tilename, "%s/%s", tshp.values[tileitemindex], layer->data);
      filename = tilename;
      
      msFreeShape(&tshp); /* done with the shape */
    } else {
      filename = layer->data;
      done = MS_TRUE; /* only one image so we're done after this */
    }

    if(strlen(filename) == 0) continue;

/*#ifdef USE_SDE
    if (layer->connectiontype == MS_SDE) {
      status = drawSDE(map, layer, img);
      if(status == -1) {
          final_status = status;
          goto cleanup;
      }
      continue;
    }
#endif 
*/

    /*
    ** If using a tileindex then build the path relative to that file if SHAPEPATH is not set.
    */
    if(layer->tileindex && !map->shapepath) { 
      msBuildPath(tiAbsFilePath, map->mappath, layer->tileindex); /* absolute path to tileindex file */
      tiAbsDirPath = msGetPath(tiAbsFilePath); /* tileindex file's directory */
      msBuildPath(szPath, tiAbsDirPath, filename); 
      free(tiAbsDirPath);
    } else {
      msBuildPath3(szPath, map->mappath, map->shapepath, filename);
    }

    /*
    ** Try to open the file, and read the first 8 bytes as a signature. 
    ** If the open fails for a reason other than "bigness" then we use
    ** the filename unaltered by path logic since it might be something
    ** very virtual.
    */
    f = fopen( szPath, "rb");
    if(!f) {
      memset( dd, 0, 8 );
#ifdef EFBIG
      if( errno != EFBIG )
          strcpy( szPath, filename );
#else
      strcpy( szPath, filename );
#endif
    } else {
      fread(dd,8,1,f); /* read some bytes to try and identify the file */
      fclose(f);
    }

    if((memcmp(dd,"II*\0",4)==0 || memcmp(dd,"MM\0*",4)==0) && !force_gdal) {
      status = drawTIFF(map, layer, img, filename);
      if(status == -1) {
        return(MS_FAILURE);
      }
      continue;
    }

    if(memcmp(dd,"GIF8",4)==0 && !force_gdal ) {
      status = drawGIF(map, layer, img, filename);
      if(status == -1) {
        return(MS_FAILURE);
      }
      continue;
    }

    if(memcmp(dd,PNGsig,8)==0 && !force_gdal) {
      status = drawPNG(map, layer, img, filename);
      if(status == -1) {
        return(MS_FAILURE);
      }
      continue;
    }

    if(memcmp(dd,JPEGsig,3)==0 && !force_gdal) {
      status = drawJPEG(map, layer, img, filename);
      if(status == -1) {
        return(MS_FAILURE);
      }
      continue;
    }

#ifdef USE_GDAL
    {
      GDALDatasetH  hDS;

      msGDALInitialize();

      msAcquireLock( TLOCK_GDAL );
      hDS = GDALOpenShared(szPath, GA_ReadOnly );

      if(hDS != NULL) {
          double	adfGeoTransform[6];
          const char *close_connection;

            if (layer->projection.numargs > 0 && 
                EQUAL(layer->projection.args[0], "auto"))
            {
                const char *pszWKT;

                pszWKT = GDALGetProjectionRef( hDS );

                if( pszWKT != NULL && strlen(pszWKT) > 0 )
                {
                    if( msOGCWKT2ProjectionObj(pszWKT, &(layer->projection),
                                               layer->debug ) != MS_SUCCESS )
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
                        final_status = MS_FAILURE;
                        break;
                    }
                }
            }

            msGetGDALGeoTransform( hDS, map, layer, adfGeoTransform );

            /* 
            ** We want to resample if the source image is rotated, if
            ** the projections differ or if resampling has been explicitly
            ** requested, or if the image has north-down instead of north-up.
            */
#ifdef USE_PROJ
            if( ((adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0
                  || adfGeoTransform[5] > 0.0 || adfGeoTransform[1] < 0.0 )
                 && layer->transform )
                || msProjectionsDiffer( &(map->projection), 
                                        &(layer->projection) ) 
                || CSLFetchNameValue( layer->processing, "RESAMPLE" ) != NULL )
            {
                status = msResampleGDALToMap( map, layer, image, hDS );
            }
            else
#endif
            {
                if( adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0 )
                {
                    if( layer->debug || map->debug )
                        msDebug( 
                            "Layer %s has rotational coefficients but we\n"
                            "are unable to use them, projections support\n"
                            "needs to be built in.", 
                            layer->name );
                    
                }
                status = msDrawRasterLayerGDAL(map, layer, image, hDS );
            }

            if( status == -1 )
            {
                GDALClose( hDS );
                msReleaseLock( TLOCK_GDAL );
                final_status = MS_FAILURE;
                break;
            }

            close_connection = msLayerGetProcessingKey( layer, 
                                                        "CLOSE_CONNECTION" );
            
            /* default to keeping open for single data files, and 
               to closing for tile indexes */
            if( close_connection == NULL && layer->tileindex == NULL )
                close_connection = "DEFER";

            if( close_connection != NULL
                && strcasecmp(close_connection,"DEFER") == 0 )
            {
                GDALDereferenceDataset( hDS );
            }
            else
            {
                GDALClose( hDS );
            }
            msReleaseLock( TLOCK_GDAL );
            continue;
        }
        else
        {
            msReleaseLock( TLOCK_GDAL );
        }
    }
#endif /* ifdef USE_GDAL */

    /* If GDAL doesn't recognise it, and it wasn't successfully opened 
    ** Generate an error.
    */
    if( !f ) {
      int ignore_missing = msMapIgnoreMissingData( map );
      if( ignore_missing != MS_MISSING_DATA_IGNORE ) {
        msSetError(MS_IOERR, "%s using full path %s", "msDrawRaster()", filename, szPath);
      }

      if( ignore_missing == MS_MISSING_DATA_FAIL ) {
        if( layer->debug || map->debug )  
          msDebug( "Unable to open file %s for layer %s ... fatal error.\n", filename, layer->name );
        final_status = MS_FAILURE;
        break;
      }

      if( ignore_missing == MS_MISSING_DATA_LOG ) {
        if( layer->debug || map->debug )
          msDebug( "Unable to open file %s for layer %s ... ignoring this missing data.\n", filename, layer->name );
        continue;
      }

      if( ignore_missing == MS_MISSING_DATA_IGNORE ) {
        continue; /* skip it, next tile */
      }
    }

    /* put others which may require checks here */  
    
    /* No easy check for EPPL so put here */      
    status=drawEPP(map, layer, img, filename);
    if(status != 0) {
      if (status == -2) msSetError(MS_IMGERR, "Error reading EPPL file; probably corrupt.", "msDrawEPP()");
      if (status == -1) msSetError(MS_IMGERR, "Unrecognized or unsupported image format", "msDrawRaster()");
      return(MS_FAILURE);
    }
    continue;
  } /* next tile */

cleanup:
  if(layer->tileindex) { /* tiling clean-up */
    msLayerClose(tlp);
    if(tilelayerindex == -1) {
      freeLayer(tlp);
      free(tlp);
    }
  }

  return final_status;
}

/************************************************************************/
/*                         msDrawReferenceMap()                         */
/************************************************************************/

/* TODO : this will msDrawReferenceMapGD */
imageObj *msDrawReferenceMap(mapObj *map) {
  double cellsize;
  int c=-1, oc=-1;
  int x1,y1,x2,y2;
  char szPath[MS_MAXPATHLEN];

  imageObj   *image = NULL;
  gdImagePtr img=NULL;

  image = msImageLoadGD( msBuildPath(szPath, map->mappath, 
                                     map->reference.image) );
  if( image == NULL )
      return NULL;

  if (map->web.imagepath)
      image->imagepath = strdup(map->web.imagepath);
  if (map->web.imageurl)
      image->imageurl = strdup(map->web.imageurl);

  img = image->img.gd;
  
  /* make sure the extent given in mapfile fits the image */
  cellsize = msAdjustExtent(&(map->reference.extent), 
                            image->width, image->height);

  /* Allocate a fake bg color because when using gd-1.8.4 with a PNG reference */
  /* image, the box color could end up being set to color index 0 which is  */
  /* transparent (yes, that's odd!). */
  gdImageColorAllocate(img, 255,255,255);

  /* allocate some colors */
  if( MS_VALID_COLOR(map->reference.outlinecolor) )
    oc = gdImageColorAllocate(img, map->reference.outlinecolor.red, map->reference.outlinecolor.green, map->reference.outlinecolor.blue);
  if( MS_VALID_COLOR(map->reference.color) )
    c = gdImageColorAllocate(img, map->reference.color.red, map->reference.color.green, map->reference.color.blue); 

  /* convert map extent to reference image coordinates */
  x1 = MS_MAP2IMAGE_X(map->extent.minx,  map->reference.extent.minx, cellsize);
  x2 = MS_MAP2IMAGE_X(map->extent.maxx,  map->reference.extent.minx, cellsize);  
  y1 = MS_MAP2IMAGE_Y(map->extent.maxy,  map->reference.extent.maxy, cellsize);
  y2 = MS_MAP2IMAGE_Y(map->extent.miny,  map->reference.extent.maxy, cellsize);

  /* if extent are smaller than minbox size */
  /* draw that extent on the reference image */
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
  else /* else draw the marker symbol */
  {
      if( map->reference.maxboxsize == 0 || 
              ((abs(x2 - x1) < map->reference.maxboxsize) && 
                  (abs(y2 - y1) < map->reference.maxboxsize)) )
      {
	  styleObj style;

          initStyle(&style);
          style.color = map->reference.color;
          style.outlinecolor = map->reference.outlinecolor;
          style.size = map->reference.markersize;

          /* if the marker symbol is specify draw this symbol else draw a cross */
          if(map->reference.marker != 0)
          {
              pointObj *point = NULL;

              point = malloc(sizeof(pointObj));
              point->x = (double)(x1 + x2)/2;
              point->y = (double)(y1 + y2)/2;

	      style.symbol = map->reference.marker;

              msDrawMarkerSymbol(&map->symbolset, image, point, &style, 1.0);
              free(point);
          }
          else if(map->reference.markername != NULL)
          {              
              pointObj *point = NULL;

              point = malloc(sizeof(pointObj));
              point->x = (double)(x1 + x2)/2;
              point->y = (double)(y1 + y2)/2;

	      style.symbol = msGetSymbolIndex(&map->symbolset,  map->reference.markername, MS_TRUE);

              msDrawMarkerSymbol(&map->symbolset, image, point, &style, 1.0);
              free(point);
          }
          else
          {
              int x21, y21;
              /* determine the center point */
              x21 = MS_NINT((x1 + x2)/2);
              y21 = MS_NINT((y1 + y2)/2);

              /* get the color */
              if(c == -1)
                  c = oc;

              /* draw a cross */
              if(c != -1)
              {
                  gdImageLine(img, x21-8, y21, x21-3, y21, c);
                  gdImageLine(img, x21, y21-8, x21, y21-3, c);
                  gdImageLine(img, x21, y21+3, x21, y21+8, c);
                  gdImageLine(img, x21+3, y21, x21+8, y21, c);
              }
          }
      }
  }

  return(image);
}

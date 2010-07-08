/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implements imagemap outputformat support.
 * Author:   Attila Csipa
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

#include "mapserver.h"
#include "dxfcolor.h"
 
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

MS_CVSID("$Id$")

#define MYDEBUG 0
#define DEBUG_IF if (MYDEBUG > 2) 

/*
 * Client-side imagemap support was originally written by
 * Attila Csipa (http://prometheus.org.yu/me.php).  C. Scott Ananian
 * (http://cscott.net) cleaned up the code somewhat and made it more flexible:
 * you can now customize the generated HREFs and create mouseover and
 * mouseout attributes.
 *
 * Use
 *   IMAGETYPE imagemap
 * to select this driver.
 *
 * The following FORMATOPTIONs are available. If any are set to the empty
 * string the associated attribute will be suppressed.
 *   POLYHREF   the href string to use for the <area> elements.
 *              use a %s to interpolate the area title.
 *              default:   "javascript:Clicked('%s');"
 *   POLYMOUSEOVER the onMouseOver string to use for the <area> elements.
 *              use a %s to interpolate the area title.
 *              default:   "" (ie the attribute is suppressed)
 *   POLYMOUSEOUT the onMouseOut string to use for the <area> elements.
 *              use a %s to interpolate the area title.
 *              default:   "" (ie the attribute is suppressed)
 *   SYMBOLMOUSEOVER and SYMBOLMOUSEOUT are equivalent properties for
 *              <area> tags representing symbols, with the same defaults.
 *   MAPNAME    the string which will be used in the name attribute
 *              of the <map> tag.  There is no %s interpolation.
 *              default: "map1"
 *   SUPPRESS   if "yes", then we will suppress area declarations with
 *              no title.
 *              default: "NO"
 * 
 * For example, http://vevo.verifiedvoting.org/verifier contains this
 * .map file fragment:
 *         OUTPUTFORMAT
 *              NAME imagemap
 *              DRIVER imagemap
 *              FORMATOPTION "POLYHREF=/verifier/map.php?state=%s"
 *              FORMATOPTION "SYMBOLHREF=#"
 *              FORMATOPTION "SUPPRESS=YES"
 *              FORMATOPTION "MAPNAME=map-48"
 *              FORMATOPTION "POLYMOUSEOUT=return nd();"
 *              FORMATOPTION "POLYMOUSEOVER=return overlib('%s');"
 *         END
 */

/*-------------------------------------------------------------------------*/

/* A pString is a variable-length (appendable) string, stored as a triple:
 * character pointer, allocated length, and used length.  The one wrinkle
 * is that we use pointers to the allocated size and character pointer,
 * in order to support refering to fields declared elsewhere (ie in the
 * 'image' data structure) for these.  The 'iprintf' function appends
 * to a pString. */
typedef struct pString {
	/* these two fields are somewhere else */
	char **string;
	int *alloc_size;
	/* this field is stored locally. */
	int string_len;
} pString;
/* These are the pStrings we declare statically.  One is the 'output'
 * imagemap/dxf file; parts of this live in another data structure and
 * are only referenced indirectly here.  The other contains layer-specific
 * information for the dxf output. */
static char *layerlist=NULL;
static int layersize=0;
static pString imgStr, layerStr={ &layerlist, &layersize, 0 };

/* Format strings for the various bits of a client-side imagemap. */
static const char *polyHrefFmt, *polyMOverFmt, *polyMOutFmt;
static const char *symbolHrefFmt, *symbolMOverFmt, *symbolMOutFmt;
static const char *mapName;
/* Should we suppress AREA declarations in imagemaps with no title? */
static int suppressEmpty=0;

/* Prevent buffer-overrun and format-string attacks by "sanitizing" any
 * provided format string to fit the number of expected string arguments
 * (MAX) exactly. */
static const char *makeFmtSafe(const char *fmt, int MAX) {
  /* format string should have exactly 'MAX' %s */

  char *result = malloc(strlen(fmt)+1+3*MAX), *cp;
  int numstr=0, saw_percent=0;

  strcpy(result, fmt);
  for (cp=result; *cp; cp++) {
    if (saw_percent) {
      if (*cp=='%') {
	/* safe */
      } else if (*cp=='s' && numstr<MAX) {
	numstr++; /* still safe */
      } else {
	/* disable this unsafe format string! */
	*(cp-1)=' ';
      }
      saw_percent=0;
    } else if (*cp=='%')
      saw_percent=1;
  }
  /* fixup format strings without enough %s in them */
  while (numstr<MAX) {
    strcpy(cp, "%.s"); /* print using zero-length precision */
    cp+=3;
    numstr++;
  }
  return result;
}

/* Append the given printf-style formatted string to the pString 'ps'.
 * This is much cleaner (and faster) than the technique this file
 * used to use! */
static void im_iprintf(pString *ps, const char *fmt, ...) {
	int n, remaining;
	va_list ap;
	do {
		remaining = *(ps->alloc_size) - ps->string_len;
		va_start(ap, fmt);
#if defined(HAVE_VSNPRINTF)
		n = vsnprintf((*(ps->string)) + ps->string_len, 
			      remaining, fmt, ap);
#else
                /* If vsnprintf() is not available then require a minimum
                 * of 512 bytes of free space to prevent a buffer overflow
                 * This is not fully bulletproof but should help, see bug 1613
                 */
                if (remaining < 512)
                    n = -1;  
                else
                    n = vsprintf((*(ps->string)) + ps->string_len, fmt, ap);
#endif
		va_end(ap);
		/* if that worked, we're done! */
		if (-1<n && n<remaining) {
			ps->string_len += n;
			return;
		} else { /* double allocated string size */
			*(ps->alloc_size) *= 2;/* these keeps realloc linear.*/
			if (*(ps->alloc_size) < 1024)
				/* careful: initial size can be 0 */
				*(ps->alloc_size)=1024;
			if (n>-1 && *(ps->alloc_size) <= (n + ps->string_len))
				/* ensure at least as much as what is needed */
				*(ps->alloc_size) = n+ps->string_len+1;
			*(ps->string) = (char *) realloc
				(*(ps->string), *(ps->alloc_size));
			/* if realloc doesn't work, we're screwed! */
		}
	} while (1); /* go back and try again. */
}



static int lastcolor=-1;
static int matchdxfcolor(colorObj col)
{
	int best=7;
	int delta=128*255;
	int tcolor = 0;
	if (lastcolor != -1)
		return lastcolor;
	while (tcolor < 256 && (ctable[tcolor].r != col.red || ctable[tcolor].g != col.green || ctable[tcolor].b != col.blue)){
		tcolor++;
		if (abs(
					(ctable[tcolor].r - col.red) * (ctable[tcolor].r - col.red)+ 
					(ctable[tcolor].b - col.blue) * (ctable[tcolor].b - col.blue) + 
					(ctable[tcolor].g - col.green) * (ctable[tcolor].g - col.green) < delta)
		   ){
			best = tcolor;
			delta = abs(
					(ctable[tcolor].r - col.red) * (ctable[tcolor].r - col.red)+ 
					(ctable[tcolor].b - col.blue) * (ctable[tcolor].b - col.blue) + 
					(ctable[tcolor].g - col.green) * (ctable[tcolor].g - col.green)
				);
		}
	}
	if (tcolor >= 256) tcolor = best;
/* DEBUG_IF	printf("%d/%d/%d (%d/%d - %d), %d : %d/%d/%d<BR>\n", ctable[tcolor].r, ctable[tcolor].g, ctable[tcolor].b, tcolor, best, delta, lastcolor, col.red, col.green, col.blue); */
	lastcolor = tcolor;
	return tcolor;
}

#ifdef notdef /* not currently used */
static gdImagePtr searchImageCache(struct imageCacheObj *ic, styleObj *style, int size) {
/* struct imageCacheObj *icp; */
DEBUG_IF printf("searchImageCache\n<BR>");
/*
  icp = ic;
  while(icp) {
    if(icp->symbol == symbol && icp->color == color && icp->size == size) return(icp->img);
    icp = icp->next;
  }
*/
  return(NULL);
}
#endif /* def notdef */

static char* lname;
static int dxf;

void msImageStartLayerIM(mapObj *map, layerObj *layer, imageObj *image){
DEBUG_IF printf("ImageStartLayerIM\n<BR>");
	free(lname);
	if (layer->name)
		lname = strdup(layer->name);
	else
		lname = strdup("NONE");
	if (dxf == 2){
		im_iprintf(&layerStr, "LAYER\n%s\n", lname);
	} else if (dxf) {
		im_iprintf(&layerStr,
			"  0\nLAYER\n  2\n%s\n"
			" 70\n  64\n 6\nCONTINUOUS\n", lname);
	}
	lastcolor = -1;
}
	
#ifdef notdef /* not currently used */
static struct imageCacheObj *addImageCache(struct imageCacheObj *ic, int *icsize, styleObj *style, int size, gdImagePtr img) {
  struct imageCacheObj *icp;
DEBUG_IF printf("addImageCache\n<BR>");

  if(*icsize > MS_IMAGECACHESIZE) { /* remove last element, size stays the same */
    icp = ic;
    while(icp->next && icp->next->next) icp = icp->next;
    freeImageCache(icp->next);
    icp->next = NULL;
  } else 
    *icsize += 1;

  if((icp = (struct imageCacheObj *)malloc(sizeof(struct imageCacheObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "initImageCache()");
    return(NULL);
  }
  
  icp->img = img;
  icp->color = style->color;
  icp->symbol = style->symbol;
  icp->size = size;
  icp->next = ic; /* insert at the beginning */
 
  return(icp);
}
#endif /* def notdef */


/*
 * Utility function to create a IM image. Returns
 * a pointer to an imageObj structure.
 */
imageObj *msImageCreateIM(int width, int height, outputFormatObj *format,
                          char *imagepath, char *imageurl, double resolution, double defresolution)
{
    imageObj  *image=NULL;
    if (setvbuf(stdout, NULL, _IONBF , 0)){
               printf("Whoops...");
    };
DEBUG_IF printf("msImageCreateIM<BR>\n");
    if (width > 0 && height > 0)
    {
        image = (imageObj *)calloc(1,sizeof(imageObj));
/*

        if( format->imagemode == MS_IMAGEMODE_RGB 
            || format->imagemode == MS_IMAGEMODE_RGBA )
        {
#if IM2_VERS > 1
            image->img.gd = gdImageCreateTrueColor(width, height);
#else
            msSetError(MS_IMGERR, 
                       "Attempt to use RGB or RGBA IMAGEMODE with IM 1.x, please upgrade to GD 2.x.", "msImageCreateGD()" );
#endif
        }
        else
            image->img.gd = gdImageCreate(width, height);
 */   
        if (image)
        {
	    imgStr.string = &(image->img.imagemap);
	    imgStr.alloc_size = &(image->size);

            image->format = format;
            format->refcount++;

            image->width = width;
            image->height = height;
            image->imagepath = NULL;
            image->imageurl = NULL;
            image->resolution = resolution;
            image->resolutionfactor = resolution/defresolution;

	    if( strcasecmp("ON",msGetOutputFormatOption( format, "DXF", "OFF" )) == 0){
		    dxf = 1;
		    im_iprintf(&layerStr, "  2\nLAYER\n 70\n  10\n");
	    } else
		    dxf = 0;

	    if( strcasecmp("ON",msGetOutputFormatOption( format, "SCRIPT", "OFF" )) == 0){
		    dxf = 2;
		    im_iprintf(&layerStr, "");
	    }

	    /* get href formation string options */
	    polyHrefFmt = makeFmtSafe(msGetOutputFormatOption
	      ( format, "POLYHREF", "javascript:Clicked('%s');"), 1);
            polyMOverFmt = makeFmtSafe(msGetOutputFormatOption
	      ( format, "POLYMOUSEOVER", ""), 1);
	    polyMOutFmt = makeFmtSafe(msGetOutputFormatOption
	      ( format, "POLYMOUSEOUT", ""), 1);
	    symbolHrefFmt = makeFmtSafe(msGetOutputFormatOption
	      ( format, "SYMBOLHREF", "javascript:SymbolClicked();"), 1);
	    symbolMOverFmt = makeFmtSafe(msGetOutputFormatOption
	      ( format, "SYMBOLMOUSEOVER", ""), 1);
	    symbolMOutFmt = makeFmtSafe(msGetOutputFormatOption
	      ( format, "SYMBOLMOUSEOUT", ""), 1);
	    /* get name of client-side image map */
	    mapName = msGetOutputFormatOption
	      ( format, "MAPNAME", "map1" );
	    /* should we suppress area declarations with no title? */
	    if( strcasecmp("YES",msGetOutputFormatOption( format, "SUPPRESS", "NO" )) == 0){
	      suppressEmpty=1;
	    }

	    lname = strdup("NONE");
	    *(imgStr.string) = strdup("");
	    if (*(imgStr.string)){
		    *(imgStr.alloc_size) =
			    imgStr.string_len = strlen(*(imgStr.string));
	    } else {
		    *(imgStr.alloc_size) =
			    imgStr.string_len = 0;
	    }
            if (imagepath)
            {
                image->imagepath = strdup(imagepath);
            }
            if (imageurl)
            {
                image->imageurl = strdup(imageurl);
            }
            
            return image;
        }
        else
            free( image );   
    }
    else
    {
        msSetError(MS_IMGERR, 
                   "Cannot create IM image of size %d x %d.", 
                   "msImageCreateIM()", width, height );
    }
    return image;
}

/*
 * Utility function to initialize the color of an image.  The background
 * color is passed, but the outputFormatObj is consulted to see if the
 * transparency should be set (for RGBA images).   Note this function only
 * affects TrueColor images.
 */

void msImageInitIM( imageObj *image )

{
DEBUG_IF printf("msImageInitIM<BR>\n");
/*    if( image->format->imagemode == MS_IMAGEMODE_PC256 ) {
        gdImageColorAllocate(image->img.gd, background->red, background->green, background->blue);
        return;
    }

#if IM2_VERS > 1
    {
        int		pen, pixels, line;
        int             *tpixels;

        if( image->format->imagemode == MS_IMAGEMODE_RGBA )
            pen = imTrueColorAlpha( background->red, 
                                    background->green, 
                                    background->blue,
                                    image->format->transparent ? 127 : 0 );
        else
            pen = imTrueColor( background->red, 
                               background->green, 
                               background->blue );

        for( line = 0; line < image->img.gd->sy; line++ )
        {
            pixels = image->img.gd->sx;
            tpixels = image->img.gd->tpixels[line];

            while( pixels-- > 0 )
                *(tpixels++) = pen;
        }
    }
#endif
*/
}

/*
 * Utility function to load an image in a IM supported format, and
 * return it as a valid imageObj.
 */
imageObj *msImageLoadIM( const char *filename )
{
/*  FILE *stream;
  gdImagePtr img=NULL;
  const char *driver = NULL;
  char bytes[8];
*/  imageObj      *image = NULL;

  DEBUG_IF printf("msImageLoadIM<BR>\n");
/*
  image = (imageObj *)calloc(1,sizeof(imageObj));
  
  stream = fopen(filename,"rb"); // allocate input and output images (same size)
  if(!stream) {
    msSetError(MS_IOERR, "(%s)", "msImageLoadIM()", filename );
    return(NULL);
  }

  fread(bytes,8,1,stream); // read some bytes to try and identify the file
  rewind(stream); // reset the image for the readers
  if (memcmp(bytes,"GIF8",4)==0) {
#ifdef USE_GD_GIF
    img = gdImageCreateFromGif(stream);
    driver = "IM/GIF";
    image->img.gd = img;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->width = img->sx;
    image->height = img->sy;
#else
    msSetError(MS_MISCERR, "Unable to load GIF reference image.", "msImageLoadIM()");
    fclose(stream);
    return(NULL);
#endif
  } else if (memcmp(bytes,PNGsig,8)==0) {
#ifdef USE_GD_PNG
    img = gdImageCreateFromPng(stream);
    driver = "IM/PNG";
    image->img.gd = img;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->width = img->sx;
    image->height = img->sy;
#else
    msSetError(MS_MISCERR, "Unable to load PNG reference image.", "msImageLoadIM()");
    fclose(stream);
    return(NULL);
#endif
  } else if (memcmp(bytes,JPEGsig,3)==0) {
#ifdef USE_GD_JPEG
    img = gdImageCreateFromJpeg(stream);
    driver = "IM/JPEG";
    image->img.gd = img;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->width = img->sx;
    image->height = img->sy;
#else
    msSetError(MS_MISCERR, "Unable to load JPEG reference image.", "msImageLoadIM()");
    fclose(stream);
    return(NULL);
#endif
  }

  if(!img) {
    msSetError(MS_IMERR, "Unable to initialize image '%s'", "msLoadImage()", 
               filename);
    fclose(stream);
    return(NULL);
  }

  // Create an outputFormatObj for the format. //
  image->format = msCreateDefaultOutputFormat( NULL, driver );

  if( image->format == NULL )
  {
    msSetError(MS_IMERR, "Unable to create default OUTPUTFORMAT definition for driver '%s'.", "msImageLoadGD()", driver );
    return(NULL);
  }
*/
  return image;
}

#ifdef notdef /* not currently used */

/* TODO: might be a way to optimize this (halve the number of additions) */
static void imageOffsetPolyline(gdImagePtr img, shapeObj *p, int color, int offsetx, int offsety)
{
/*  int i, j;
  double dx, dy;
  int ox=0, oy=0;
*/
  DEBUG_IF printf("imageOffsetPolyline\n<BR>");
/*
  if(offsety == -99) { // old-style offset (version 3.3 and earlier)
    for (i = 0; i < p->numlines; i++) {
      for(j=1; j<p->line[i].numpoints; j++) {        
	dx = abs(p->line[i].point[j-1].x - p->line[i].point[j].x);
	dy = abs(p->line[i].point[j-1].y - p->line[i].point[j].y);
	if(dx<=dy)
	  ox=offsetx;
	else
	  oy=offsetx;
        
        gdImageLine(img, (int)p->line[i].point[j-1].x+ox, (int)p->line[i].point[j-1].y+oy, (int)p->line[i].point[j].x+ox, (int)p->line[i].point[j].y+oy, color);
        ox=0;
        oy=0;
      }
    }
  } else { // normal offset (eg. drop shadow)
    for (i = 0; i < p->numlines; i++)
      for(j=1; j<p->line[i].numpoints; j++)
        gdImageLine(img, (int)p->line[i].point[j-1].x+offsetx, (int)p->line[i].point[j-1].y+offsety, (int)p->line[i].point[j].x+offsetx, (int)p->line[i].point[j].y+offsety, color);
  }
  */
}

static void imagePolyline(gdImagePtr img, shapeObj *p, int color, int offsetx, int offsety)
{
/* int i, j; */
  
DEBUG_IF printf("imagePolyline\n<BR>");
/*
 if(offsetx != 0 || offsetx != 0) 
    imageOffsetPolyline(img, p, color, offsetx, offsety);
  else {
    for (i = 0; i < p->numlines; i++)
      for(j=1; j<p->line[i].numpoints; j++)
        gdImageLine(img, (int)p->line[i].point[j-1].x, (int)p->line[i].point[j-1].y, (int)p->line[i].point[j].x, (int)p->line[i].point[j].y, color);
  }
  */
}
#endif /* def notdef */

/* ------------------------------------------------------------------------- */
/* Stroke an ellipse with a line symbol of the specified size and color      */
/* ------------------------------------------------------------------------- */
void msCircleDrawLineSymbolIM(symbolSetObj *symbolset, imageObj* img, pointObj *p, double r, styleObj *style, double scalefactor)
{
/*  int i, j;
  symbolObj *symbol;
  int styleDashed[100];
  int x, y, ox, oy;
  int bc, fc;
  int brush_bc, brush_fc;
  double size, d;
  gdImagePtr brush=NULL;
  gdPoint points[MS_MAXVECTORPOINTS];
*/
  DEBUG_IF printf("msCircleDrawLineSymbolIM\n<BR>");
/*  
  if(!p) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor));
  
  symbol = symbolset->symbol[style->symbol];
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  if(fc==-1) fc = style->outlinecolor.pen;
  ox = style->offsetx; // TODO: add scaling?
  oy = style->offsety;
  size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; // no such symbol, 0 is OK
  if(fc < 0) return; // nothing to do
  if(size < 1) return; // size too small

  if(symbol == 0) { // just draw a single width line
    gdImageArc(img, (int)p->x + ox, (int)p->y + oy, (int)r, (int)r, 0, 360, fc);
    return;
  }

  switch(symbol->type) {
  case(MS_SYMBOL_SIMPLE):
    if(bc == -1) bc = imTransparent;
    break;
  case(MS_SYMBOL_TRUETYPE):
    // msImageTruetypePolyline(img, p, symbol, fc, size, symbolset->fontset);
    return;
    break;
  case(MS_SYMBOL_CARTOLINE):
    return;
    break;
  case(MS_SYMBOL_ELLIPSE):   
    bc = imTransparent;

    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);
   
    if((x < 2) && (y < 2)) break;
    
    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style, size)) == NULL) { // not in cache, create
      brush = gdImageCreate(x, y);
      brush_bc = gdImageColorAllocate(brush, gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));    
      gdImageColorTransparent(brush,0);
      brush_fc = gdImageColorAllocate(brush, style->color.red, style->color.green, style->color.blue);
      
      x = MS_NINT(brush->sx/2); // center the ellipse
      y = MS_NINT(brush->sy/2);
      
      // draw in the brush image
      gdImageArc(brush, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, brush_fc);
      if(symbol->filled)
	gdImageFillToBorder(brush, x, y, brush_fc, brush_fc);
      
      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, style, size, brush);
    }

    gdImageSetBrush(img, brush);
    fc=1; bc=0;
    break;
  case(MS_SYMBOL_PIXMAP):
    gdImageSetBrush(img, symbol->img);
    fc=1; bc=0;
    break;
  case(MS_SYMBOL_VECTOR):
    if(bc == -1) bc = imTransparent;

    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);

    if((x < 2) && (y < 2)) break;

    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style, size)) == NULL) { // not in cache, create
      brush = gdImageCreate(x, y);
      if(style->backgroundcolor.pen >= 0)
	brush_bc = gdImageColorAllocate(brush, style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);
      else {
	brush_bc = gdImageColorAllocate(brush, gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));
	gdImageColorTransparent(brush,0);
      }
      brush_fc = gdImageColorAllocate(brush, style->color.red, style->color.green, style->color.blue);
      
      // draw in the brush image
      for(i=0;i < symbol->numpoints;i++) {
	points[i].x = MS_NINT(d*symbol->points[i].x);
	points[i].y = MS_NINT(d*symbol->points[i].y);
      }
      gdImageFilledPolygon(brush, points, symbol->numpoints, brush_fc);

      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, style, size, brush);
    }

    gdImageSetBrush(img, brush);
    fc = 1; bc = 0;
    break;
  }  

  if(symbol->patternlength > 0) {
    int k=0, sc;
   
    sc = fc; // start with foreground color
    for(i=0; i<symbol->patternlength; i++) {      
      for(j=0; j<symbol->pattern[i]; j++) {
	styleDashed[k] = sc;
	k++;
      } 
      if(sc==fc) sc = bc; else sc = fc;
    }
    gdImageSetStyle(img, styleDashed, k);

    // gdImageArc(brush, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, brush_fc);

    if(!brush && !symbol->img)
      gdImageArc(img, (int)p->x + ox, (int)p->y + oy, (int)(2*r), (int)(2*r), 0, 360, gdStyled);      
    else 
      gdImageArc(img, (int)p->x + ox, (int)p->y + oy, (int)(2*r), (int)(2*r), 0, 360, gdStyledBrushed);
  } else {
    if(!brush && !symbol->img)
      gdImageArc(img, (int)p->x + ox, (int)p->y + oy, (int)(2*r), (int)(2*r), 0, 360, fc);
    else
      gdImageArc(img, (int)p->x + ox, (int)p->y + oy, (int)(2*r), (int)(2*r), 0, 360, gdBrushed);
  }
*/
  return;
}

/* ------------------------------------------------------------------------- */
/* Fill a circle with a shade symbol of the specified size and color         */
/* ------------------------------------------------------------------------- */
void msCircleDrawShadeSymbolIM(symbolSetObj *symbolset, imageObj* img, 
                               pointObj *p, double r, styleObj *style, double scalefactor)
{
/*  symbolObj *symbol;
  int i;
  gdPoint oldpnt,newpnt;
  gdPoint sPoints[MS_MAXVECTORPOINTS];
  gdImagePtr tile=NULL;
  int x, y, ox, oy;

  int fc, bc, oc;
  int tile_bc=-1, tile_fc=-1; // colors (background and foreground) //
  double size, d;  

  int bbox[8];
  rectObj rect;
  char *font=NULL;
*/
DEBUG_IF printf("msCircleDrawShadeSymbolIM<BR>\n");
/*
  if(!p) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor));

  symbol = symbolset->symbol[style->symbol];
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  oc = style->outlinecolor.pen;
  ox = style->offsetx; // TODO: add scaling?
  oy = style->offsety;
  size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  if(fc==-1 && oc!=-1) { // use msDrawLineSymbolIM() instead (POLYLINE)
    msCircleDrawLineSymbolIM(symbolset, img, p, r, style, scalefactor);
    return;
  }

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; // no such symbol, 0 is OK
  if(fc ) return; // invalid color, -1 is valid
  if(size < 1) return; // size too small
      
  if(style->symbol == 0) { // simply draw a single pixel of the specified color
    msImageFilledCircle(img, p, r, fc);
    if(oc>-1) gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);
    return;
  }

  switch(symbol->type) {
  case(MS_SYMBOL_TRUETYPE):    
    
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    font = msLookupHashTable(&(symbolset->fontset->fonts), symbol->font);
    if(!font) return;

    if(msGetCharacterSize(symbol->character, size, font, &rect) != MS_SUCCESS) return;
    x = rect.maxx - rect.minx;
    y = rect.maxy - rect.miny;

    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bc = gdImageColorAllocate(tile, style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);
    else {
      tile_bc = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }    
    tile_fc = gdImageColorAllocate(tile, style->color.red, style->color.green, style->color.blue);
    
    x = -rect.minx;
    y = -rect.miny;

#ifdef USE_GD_TTF
    gdImageStringTTF(tile, bbox, ((symbol->antialias)?(tile_fc):-(tile_fc)), font, size, 0, x, y, symbol->character);
#else
    gdImageStringFT(tile, bbox, ((symbol->antialias)?(tile_fc):-(tile_fc)), font, size, 0, x, y, symbol->character);
#endif    

    gdImageSetTile(img, tile);
#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    
    gdImageSetTile(img, symbol->img);

    break;
  case(MS_SYMBOL_ELLIPSE):    
   
    d = size/symbol->sizey; // size ~ height in pixels
    x = MS_NINT(symbol->sizex*d)+1;
    y = MS_NINT(symbol->sizey*d)+1;

    if((x <= 1) && (y <= 1)) { // No sense using a tile, just fill solid
      msImageFilledCircle(img, p, r, fc);
      if(oc>-1) gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);
      return;
    }

    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bc = gdImageColorAllocate(tile, style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);
    else {
      tile_bc = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }    
    tile_fc = gdImageColorAllocate(tile, style->color.red, style->color.green, style->color.blue);
    
    x = MS_NINT(tile->sx/2);
    y = MS_NINT(tile->sy/2);

    //
   //draw in the tile image
    //
    gdImageArc(tile, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, tile_fc);
    if(symbol->filled)
      gdImageFillToBorder(tile, x, y, tile_fc, tile_fc);

    //
   //Fill the polygon in the main image
    //
    gdImageSetTile(img, tile);
 
    break;
  case(MS_SYMBOL_VECTOR):

    if(fc < 0)
      return;

    d = size/symbol->sizey; // size ~ height in pixels
    x = MS_NINT(symbol->sizex*d)+1;    
    y = MS_NINT(symbol->sizey*d)+1;

    if((x <= 1) && (y <= 1)) { // No sense using a tile, just fill solid
      msImageFilledCircle(img, p, r, fc);
      if(oc>-1) gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);
      return;
    }

    //
   //create tile image
    //
    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bc = gdImageColorAllocate(tile, style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);
    else {
      tile_bc = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }
    tile_fc = gdImageColorAllocate(tile, style->color.red, style->color.green, style->color.blue);
   
    //
   //draw in the tile image
    //
    if(symbol->filled) {

      for(i=0;i < symbol->numpoints;i++) {
	sPoints[i].x = MS_NINT(d*symbol->points[i].x);
	sPoints[i].y = MS_NINT(d*symbol->points[i].y);
      }
      gdImageFilledPolygon(tile, sPoints, symbol->numpoints, tile_fc);      

    } else  { // shade is a vector drawing //
      
      oldpnt.x = MS_NINT(d*symbol->points[0].x); // convert first point in shade smbol //
      oldpnt.y = MS_NINT(d*symbol->points[0].y);

      // step through the shade symbol //
      for(i=1;i < symbol->numpoints;i++) {
	if((symbol->points[i].x < 0) && (symbol->points[i].y < 0)) {
	  oldpnt.x = MS_NINT(d*symbol->points[i].x);
	  oldpnt.y = MS_NINT(d*symbol->points[i].y);
	} else {
	  if((symbol->points[i-1].x < 0) && (symbol->points[i-1].y < 0)) { // Last point was PENUP, now a new beginning //
	    oldpnt.x = MS_NINT(d*symbol->points[i].x);
	    oldpnt.y = MS_NINT(d*symbol->points[i].y);
	  } else {
	    newpnt.x = MS_NINT(d*symbol->points[i].x);
	    newpnt.y = MS_NINT(d*symbol->points[i].y);
	    gdImageLine(tile, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, tile_fc);
	    oldpnt = newpnt;
	  }
	}
      } // end for loop //
    }

    //
   //Fill the polygon in the main image
    //
    gdImageSetTile(img, tile);
 
    break;
  default:
    break;
  }

  //
 //Fill the polygon in the main image
  //
  msImageFilledCircle(img, p, r, imTiled);
  if(oc>-1) gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);
  if(tile) gdImageDestroy(tile);
*/
  return;
}


/* ------------------------------------------------------------------------- */
/* Draw a single marker symbol of the specified size and color               */
/* ------------------------------------------------------------------------- */
void msDrawMarkerSymbolIM(symbolSetObj *symbolset, imageObj* img, pointObj *p, styleObj *style, double scalefactor)
{
  symbolObj *symbol;
/*  int offset_x, offset_y, x, y;
  int j;
  gdPoint oldpnt,newpnt;
  gdPoint mPoints[MS_MAXVECTORPOINTS];

  gdImagePtr tmp;
  int tmp_fc=-1, tmp_bc, tmp_oc=-1;
*/  int fc, bc, oc;
/*  double d;

  
  int bbox[8];
  rectObj rect;
  char *font=NULL;
*/
  int ox, oy;
  double size;
	
DEBUG_IF printf("msDrawMarkerSymbolIM\n<BR>");

/* skip this, we don't do text */



  if(!p) return;

/* if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor)); */
/* if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color)); */
/* if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor)); */

  symbol = symbolset->symbol[style->symbol];
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  oc = style->outlinecolor.pen;
  ox = style->offsetx*scalefactor;
  oy = style->offsety*scalefactor;
  if(style->size == -1) {
      size = msSymbolGetDefaultSize( symbol );
      size = MS_NINT(size*scalefactor);
  }
  else
      size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize*img->resolutionfactor);
  size = MS_MIN(size, style->maxsize*img->resolutionfactor);

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; /* no such symbol, 0 is OK */
/* if(fc<0 && oc<0) return; // nothing to do */
  if(size < 1) return; /* size too small */

/* DEBUG_IF printf(".%d.%d.%d.", symbol->type, style->symbol, fc); */
  if(style->symbol == 0) { /* simply draw a single pixel of the specified color */
/* gdImageSetPixel(img, p->x + ox, p->y + oy, fc); */
		
    if (dxf) {
      if (dxf==2)
	im_iprintf (&imgStr, "POINT\n%.0f %.0f\n%d\n",
		 p->x + ox, p->y + oy, matchdxfcolor(style->color));
      else
	im_iprintf (&imgStr, 
		 "  0\nPOINT\n 10\n%f\n 20\n%f\n 30\n0.0\n"
		 " 62\n%6d\n  8\n%s\n",
		 p->x + ox, p->y + oy, matchdxfcolor(style->color), lname);
    } else {
      im_iprintf (&imgStr, "<area ");
      if (strcmp(symbolHrefFmt,"%.s")!=0) {
	      im_iprintf (&imgStr, "href=\"");
	      im_iprintf (&imgStr, symbolHrefFmt, lname);
	      im_iprintf (&imgStr, "\" ");
      }
      if (strcmp(symbolMOverFmt,"%.s")!=0) {
	      im_iprintf (&imgStr, "onMouseOver=\"");
	      im_iprintf (&imgStr, symbolMOverFmt, lname);
	      im_iprintf (&imgStr, "\" ");
      }
      if (strcmp(symbolMOutFmt,"%.s")!=0) {
	      im_iprintf (&imgStr, "onMouseOut=\"");
	      im_iprintf (&imgStr, symbolMOutFmt, lname);
	      im_iprintf (&imgStr, "\" ");
      }
      im_iprintf (&imgStr, "shape=\"circle\" coords=\"%.0f,%.0f, 3\" />\n",
	       p->x + ox, p->y + oy);
    }
		      
	/* point2 = &( p->line[j].point[i] ); */
	/* if(point1->y == point2->y) {} */
    return;
  }  
DEBUG_IF printf("A");
  switch(symbol->type) {  
  case(MS_SYMBOL_TRUETYPE):
DEBUG_IF printf("T");
/*
 * #if defined (USE_GD_FT) || defined (USE_GD_TTF)
    font = msLookupHashTable(&(symbolset->fontset->fonts), symbol->font);
    if(!font) return;

    if(msGetCharacterSize(symbol->character, size, font, &rect) != MS_SUCCESS) return;

    x = p->x + ox - (rect.maxx - rect.minx)/2 - rect.minx;
    y = p->y + oy - rect.maxy + (rect.maxy - rect.miny)/2;  

#ifdef USE_GD_TTF
    gdImageStringTTF(img, bbox, ((symbol->antialias)?(fc):-(fc)), font, size, 0, x, y, symbol->character);
#else
    gdImageStringFT(img, bbox, ((symbol->antialias)?(fc):-(fc)), font, size, 0, x, y, symbol->character);
#endif

#endif
*/
    break;
  case(MS_SYMBOL_PIXMAP):
DEBUG_IF printf("P");
/*    if(size == 1) { // don't scale
      offset_x = MS_NINT(p->x - .5*symbol->img->sx + ox);
      offset_y = MS_NINT(p->y - .5*symbol->img->sy + oy);
      gdImageCopy(img, symbol->img, offset_x, offset_y, 0, 0, symbol->img->sx, symbol->img->sy);
    } else {
      d = size/symbol->img->sy;
      offset_x = MS_NINT(p->x - .5*symbol->img->sx*d + ox);
      offset_y = MS_NINT(p->y - .5*symbol->img->sy*d + oy);
      gdImageCopyResized(img, symbol->img, offset_x, offset_y, 0, 0, symbol->img->sx*d, symbol->img->sy*d, symbol->img->sx, symbol->img->sy);
    }
    break;
  case(MS_SYMBOL_ELLIPSE):
    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d)+1;
    y = MS_NINT(symbol->sizey*d)+1;

    // create temporary image and allocate a few colors //
    tmp = gdImageCreate(x, y);
    tmp_bc = gdImageColorAllocate(tmp, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
    gdImageColorTransparent(tmp, 0);
    if(fc >= 0)
      tmp_fc = gdImageColorAllocate(tmp, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc));
    if(oc >= 0)
      tmp_oc = gdImageColorAllocate(tmp, gdImageRed(img, oc), gdImageGreen(img, oc), gdImageBlue(img, oc));

    x = MS_NINT(tmp->sx/2);
    y = MS_NINT(tmp->sy/2);

    // draw in the temporary image //
    if(tmp_oc >= 0) {
      gdImageArc(tmp, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, tmp_oc);
      if(symbol->filled && tmp_fc >= 0)
	gdImageFillToBorder(tmp, x, y, tmp_oc, tmp_fc);
    } else {
      if(tmp_fc >= 0) {
	gdImageArc(tmp, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, tmp_fc);
	if(symbol->filled)
	  gdImageFillToBorder(tmp, x, y, tmp_fc, tmp_fc);
      }
    }

    // paste the tmp image in the main image //
    offset_x = MS_NINT(p->x - .5*tmp->sx + ox);
    offset_y = MS_NINT(p->y - .5*tmp->sx + oy);
    gdImageCopy(img, tmp, offset_x, offset_y, 0, 0, tmp->sx, tmp->sy);

    gdImageDestroy(tmp);
*/
    break;
  case(MS_SYMBOL_VECTOR):
DEBUG_IF printf("V");
 {
     double d, offset_x, offset_y;
     
     d = size/symbol->sizey;
     offset_x = MS_NINT(p->x - d*.5*symbol->sizex + ox);
     offset_y = MS_NINT(p->y - d*.5*symbol->sizey + oy);

     /* For now I only render filled vector symbols in imagemap, and no */
     /* dxf support available yet.  */
     if(symbol->filled && !dxf /* && fc >= 0 */ ) {
/* char *title=(p->numvalues) ? p->values[0] : ""; */
         char *title = "";
         int  j;

         im_iprintf (&imgStr, "<area ");
         if (strcmp(symbolHrefFmt,"%.s")!=0) {
             im_iprintf (&imgStr, "href=\"");
             im_iprintf (&imgStr, symbolHrefFmt, lname);
             im_iprintf (&imgStr, "\" ");
         }
         if (strcmp(symbolMOverFmt,"%.s")!=0) {
             im_iprintf (&imgStr, "onMouseOver=\"");
             im_iprintf (&imgStr, symbolMOverFmt, lname);
             im_iprintf (&imgStr, "\" ");
         }
         if (strcmp(symbolMOutFmt,"%.s")!=0) {
             im_iprintf (&imgStr, "onMouseOut=\"");
             im_iprintf (&imgStr, symbolMOutFmt, lname);
             im_iprintf (&imgStr, "\" ");
         }
         
         im_iprintf (&imgStr, "title=\"%s\" shape=\"poly\" coords=\"", title);
         
         for(j=0;j < symbol->numpoints;j++) {
             im_iprintf (&imgStr, "%s %d,%d", j == 0 ? "": ",", 
                         MS_NINT(d*symbol->points[j].x + offset_x),
                         MS_NINT(d*symbol->points[j].y + offset_y) );
         }
         im_iprintf (&imgStr, "\" />\n");
     } /* end of imagemap, filled case. */
 }
 break;

  default:
DEBUG_IF printf("DEF");
    break;
  } /* end switch statement */

  return;
}

/* ------------------------------------------------------------------------- */
/* Draw a line symbol of the specified size and color                        */
/* ------------------------------------------------------------------------- */
void msDrawLineSymbolIM(symbolSetObj *symbolset, imageObj* img, shapeObj *p, styleObj *style, double scalefactor)
{
  symbolObj *symbol;
/* int styleDashed[100]; */
/* int x, y; */
/* int brush_bc, brush_fc; */
/* double size, d; */
/* gdImagePtr brush=NULL; */
/* gdPoint points[MS_MAXVECTORPOINTS]; */
  int fc, bc;
  int i,j,l;
char first = 1;
double size;
DEBUG_IF printf("msDrawLineSymbolIM<BR>\n");


  if(!p) return;
  if(p->numlines <= 0) return;

/* if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor)); */
/* if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor)); */
/* if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color)); */

  symbol = symbolset->symbol[style->symbol];
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  if(fc==-1) fc = style->outlinecolor.pen;
  if(style->size == -1) {
      size = msSymbolGetDefaultSize( symbol );
      size = MS_NINT(size*scalefactor);
  }
  else
      size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize*img->resolutionfactor);
  size = MS_MIN(size, style->maxsize*img->resolutionfactor);

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; /* no such symbol, 0 is OK */
  if (suppressEmpty && p->numvalues==0) return;/* suppress area with empty title */
/* if(fc < 0) return; // nothing to do */
/* if(size < 1) return; // size too small */
  if(style->symbol == 0) { /* just draw a single width line */
/* imagePolyline(img, p, fc, style->offsetx, style->offsety); */
		  for(l=0,j=0; j<p->numlines; j++) {
		    if (dxf == 2){
		      im_iprintf (&imgStr, "LINE\n%d\n", matchdxfcolor(style->color));
		    } else if (dxf){
		      im_iprintf (&imgStr, "  0\nPOLYLINE\n 70\n     0\n 62\n%6d\n  8\n%s\n", matchdxfcolor(style->color), lname);
		    } else {
                      char *title;

                      first = 1;
		      title=(p->numvalues) ? p->values[0] : "";
		      im_iprintf (&imgStr, "<area ");
		      if (strcmp(polyHrefFmt,"%.s")!=0) {
			im_iprintf (&imgStr, "href=\"");
			im_iprintf (&imgStr, polyHrefFmt, title);
			im_iprintf (&imgStr, "\" ");
		      }
		      if (strcmp(polyMOverFmt,"%.s")!=0) {
			im_iprintf (&imgStr, "onMouseOver=\"");
			im_iprintf (&imgStr, polyMOverFmt, title);
			im_iprintf (&imgStr, "\" ");
		      }
		      if (strcmp(polyMOutFmt,"%.s")!=0) {
			im_iprintf (&imgStr, "onMouseOut=\"");
			im_iprintf (&imgStr, polyMOutFmt, title);
			im_iprintf (&imgStr, "\" ");
		      }
		      im_iprintf (&imgStr, "title=\"%s\" shape=\"poly\" coords=\"", title);
		    }
		/* point1 = &( p->line[j].point[p->line[j].numpoints-1] ); */
		      for(i=0; i < p->line[j].numpoints; i++,l++) {
				if (dxf == 2){
					im_iprintf (&imgStr, "%.0f %.0f\n", p->line[j].point[i].x, p->line[j].point[i].y);
				} else if (dxf){
					im_iprintf (&imgStr, "  0\nVERTEX\n 10\n%f\n 20\n%f\n 30\n%f\n", p->line[j].point[i].x, p->line[j].point[i].y, 0.0);
				} else {
					im_iprintf (&imgStr, "%s %.0f,%.0f", first ? "": ",", p->line[j].point[i].x, p->line[j].point[i].y);
				}
				first = 0;
			      
		/* point2 = &( p->line[j].point[i] ); */
		/* if(point1->y == point2->y) {} */
		      }
		      im_iprintf (&imgStr, dxf ? (dxf == 2 ? "": "  0\nSEQEND\n") : "\" />\n");
		  }

/* DEBUG_IF printf ("%d, ",strlen(img->img.imagemap) ); */
    return;
  }

  DEBUG_IF printf("-%d-",symbol->type);
  
/*  switch(symbol->type) {
  case(MS_SYMBOL_SIMPLE):
    if(bc == -1) bc = imTransparent;
    break;
  case(MS_SYMBOL_TRUETYPE):
    msImageTruetypePolyline(symbolset, img, p, style, scalefactor);
    return;
    break;
  case(MS_SYMBOL_CARTOLINE):
    // Single line //
    if (size == 1) {
      bc = imTransparent;
      break;
    } else {
      msImageCartographicPolyline(img, p, fc, size, symbol->linecap, symbol->linejoin, symbol->linejoinmaxsize);
    }
    return;
    break;
  case(MS_SYMBOL_ELLIPSE):
    bc = imTransparent;

    d = (size)/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);
    
    if((x < 2) && (y < 2)) break;
    
    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style, size)) == NULL) { // not in cache, create
      brush = gdImageCreate(x, y);
      brush_bc = gdImageColorAllocate(brush,gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));    
      gdImageColorTransparent(brush,0);
      brush_fc = gdImageColorAllocate(brush, style->color.red, style->color.green, style->color.blue);
      
      x = MS_NINT(brush->sx/2); // center the ellipse
      y = MS_NINT(brush->sy/2);
      
      // draw in the brush image
      gdImageArc(brush, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, brush_fc);
      if(symbol->filled)
	gdImageFillToBorder(brush, x, y, brush_fc, brush_fc);
      
      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, style, size, brush);
    }

    gdImageSetBrush(img, brush);
    fc = 1; bc = 0;
    break;
  case(MS_SYMBOL_PIXMAP):
    gdImageSetBrush(img, symbol->img);
    fc = 1; bc = 0;
    break;
  case(MS_SYMBOL_VECTOR):
    if(bc == -1) bc = imTransparent;

    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);

    if((x < 2) && (y < 2)) break;

    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style, fc, size)) == NULL) { // not in cache, create
      brush = gdImageCreate(x, y);
      if(bc >= 0)
	brush_bc = gdImageColorAllocate(brush, style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);
      else {
	brush_bc = gdImageColorAllocate(brush, gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));
	gdImageColorTransparent(brush,0);
      }
      brush_fc = gdImageColorAllocate(brush,style->color.red, style->color.green, style->color.blue);
      
      // draw in the brush image 
      for(i=0;i < symbol->numpoints;i++) {
	points[i].x = MS_NINT(d*symbol->points[i].x);
	points[i].y = MS_NINT(d*symbol->points[i].y);
      }
      gdImageFilledPolygon(brush, points, symbol->numpoints, brush_fc);

      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, style, size, brush);
    }

    gdImageSetBrush(img, brush);
    fc = 1; bc = 0;
    break;
  }  

  if(symbol->patternlength > 0) {
    int k=0, sc;
   
    sc = fc; // start with foreground color
    for(i=0; i<symbol->patternlength; i++) {      
      for(j=0; j<symbol->pattern[i]; j++) {
	styleDashed[k] = sc;
	k++;
      } 
      if(sc==fc) sc = bc; else sc = fc;
    }
    gdImageSetStyle(img, styleDashed, k);

    if(!brush && !symbol->img)
      imagePolyline(img, p, imStyled, style->offsetx, style->offsety);
    else 
      imagePolyline(img, p, imStyledBrushed, style->offsetx, style->offsety);
  } else {
    if(!brush && !symbol->img)
      imagePolyline(img, p, fc, style->offsetx, style->offsety);
    else
      imagePolyline(img, p, imBrushed, style->offsetx, style->offsety);
  }
*/
  return;
}


/* ------------------------------------------------------------------------- */
/* Draw a shade symbol of the specified size and color                       */
/* ------------------------------------------------------------------------- */
void msDrawShadeSymbolIM(symbolSetObj *symbolset, imageObj* img, shapeObj *p, styleObj *style, double scalefactor)
{
  symbolObj *symbol;
/*  gdPoint oldpnt, newpnt;
  gdPoint sPoints[MS_MAXVECTORPOINTS];
  gdImagePtr tile;
  int x, y;
  int tile_bc=-1, tile_fc=-1; // colors (background and foreground) //
  int fc, bc, oc;
  double d;
  int bbox[8];
  rectObj rect;
  char *font=NULL;
  */
  int i,j,l;
char first = 1;
double size;

DEBUG_IF printf("msDrawShadeSymbolIM\n<BR>");
  if(!p) return;
  if(p->numlines <= 0) return;
/* if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor)); */
/* if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color)); */
/* if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor)); */

  symbol = symbolset->symbol[style->symbol];
/* bc = style->backgroundcolor.pen; */
/* fc = style->color.pen; */
/* oc = style->outlinecolor.pen; */
  if(style->size == -1) {
      size = msSymbolGetDefaultSize( symbol );
      size = MS_NINT(size*scalefactor);
  }
  else
      size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize*img->resolutionfactor);
  size = MS_MIN(size, style->maxsize*img->resolutionfactor);

/* DEBUG_IF printf ("a"); */
/* if(fc==-1 && oc!=-1) { // use msDrawLineSymbolIM() instead (POLYLINE) */
/* msDrawLineSymbolIM(symbolset, img, p, style, scalefactor); */
/* return; */
/* } */
/* DEBUG_IF printf ("b"); */

/* if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; // no such symbol, 0 is OK */
  if (suppressEmpty && p->numvalues==0) return;/* suppress area with empty title */
/* DEBUG_IF printf ("1"); */
/* if(fc < 0) return; // nothing to do */
/* DEBUG_IF printf ("2"); */
/* if(size < 1) return; // size too small */
/* DEBUG_IF printf ("3"); */
      
/* DEBUG_IF printf("BEF%s", img->img.imagemap); */
	  if(style->symbol == 0) { /* simply draw a single pixel of the specified color //     */
		  for(l=0,j=0; j<p->numlines; j++) {
		    if (dxf == 2){
		      im_iprintf (&imgStr, "POLY\n%d\n", matchdxfcolor(style->color));
		    } else if (dxf){
		      im_iprintf (&imgStr, "  0\nPOLYLINE\n 73\n     1\n 62\n%6d\n  8\n%s\n", matchdxfcolor(style->color), lname);
		    } else {
		      char *title=(p->numvalues) ? p->values[0] : "";
                      first = 1;
		      im_iprintf (&imgStr, "<area ");
		      if (strcmp(polyHrefFmt,"%.s")!=0) {
			im_iprintf (&imgStr, "href=\"");
			im_iprintf (&imgStr, polyHrefFmt, title);
			im_iprintf (&imgStr, "\" ");
		      }
		      if (strcmp(polyMOverFmt,"%.s")!=0) {
			im_iprintf (&imgStr, "onMouseOver=\"");
			im_iprintf (&imgStr, polyMOverFmt, title);
			im_iprintf (&imgStr, "\" ");
		      }
		      if (strcmp(polyMOutFmt,"%.s")!=0) {
			im_iprintf (&imgStr, "onMouseOut=\"");
			im_iprintf (&imgStr, polyMOutFmt, title);
			im_iprintf (&imgStr, "\" ");
		      }
		      im_iprintf (&imgStr, "title=\"%s\" shape=\"poly\" coords=\"", title);
		    }

		/* point1 = &( p->line[j].point[p->line[j].numpoints-1] ); */
		      for(i=0; i < p->line[j].numpoints; i++,l++) {
				if (dxf == 2){
					im_iprintf (&imgStr, "%.0f %.0f\n", p->line[j].point[i].x, p->line[j].point[i].y);
				} else if (dxf){
					im_iprintf (&imgStr, "  0\nVERTEX\n 10\n%f\n 20\n%f\n 30\n%f\n", p->line[j].point[i].x, p->line[j].point[i].y, 0.0);
				} else {
					im_iprintf (&imgStr, "%s %.0f,%.0f", first ? "": ",", p->line[j].point[i].x, p->line[j].point[i].y);
				}
				first = 0;
			      
		/* point2 = &( p->line[j].point[i] ); */
		/* if(point1->y == point2->y) {} */
		      }
		      im_iprintf (&imgStr, dxf ? (dxf == 2 ? "": "  0\nSEQEND\n") : "\" />\n");
		  }
  
/* DEBUG_IF printf("AFT%s", img->img.imagemap); */
/* STOOPID. GD draws polygons pixel by pixel ?! */
	
/* msImageFilledPolygon(img, p, fc); */
/* if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety); */
  	     return;
	  }
/* DEBUG_IF printf ("d"); */
  DEBUG_IF printf("-%d-",symbol->type);
/*  
  switch(symbol->type) {
  case(MS_SYMBOL_TRUETYPE):    
    
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    font = msLookupHashTable(&(symbolset->fontset->fonts), symbol->font);
    if(!font) return;

    if(msGetCharacterSize(symbol->character, size, font, &rect) != MS_SUCCESS) return;
    x = rect.maxx - rect.minx;
    y = rect.maxy - rect.miny;

    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bc = gdImageColorAllocate(tile, style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);
    else {
      tile_bc = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }    
    tile_fc = gdImageColorAllocate(tile, style->color.red, style->color.green, style->color.blue);
    
    x = -rect.minx;
    y = -rect.miny;

#ifdef USE_GD_TTF
    gdImageStringTTF(tile, bbox, ((symbol->antialias)?(tile_fc):-(tile_fc)), font, size, 0, x, y, symbol->character);
#else
    gdImageStringFT(tile, bbox, ((symbol->antialias)?(tile_fc):-(tile_fc)), font, size, 0, x, y, symbol->character);
#endif    

    gdImageSetTile(img, tile);
    msImageFilledPolygon(img,p,gdTiled);
    if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);
    gdImageDestroy(tile);
#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    
    gdImageSetTile(img, symbol->img);
    msImageFilledPolygon(img, p, imTiled);
    if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);

    break;
  case(MS_SYMBOL_ELLIPSE):    
    d = size/symbol->sizey; // size ~ height in pixels
    x = MS_NINT(symbol->sizex*d)+1;
    y = MS_NINT(symbol->sizey*d)+1;

    if((x <= 1) && (y <= 1)) { // No sense using a tile, just fill solid //
      msImageFilledPolygon(img, p, fc);
      if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);
      return;
    }

    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bc = gdImageColorAllocate(tile, style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);
    else {
      tile_bc = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }    
    tile_fc = gdImageColorAllocate(tile, style->color.red, style->color.green, style->color.blue);
    
    x = MS_NINT(tile->sx/2);
    y = MS_NINT(tile->sy/2);

    //
   //draw in the tile image
    //
    gdImageArc(tile, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, tile_fc);
    if(symbol->filled)
      gdImageFillToBorder(tile, x, y, tile_fc, tile_fc);

    //
   //Fill the polygon in the main image
    //
    gdImageSetTile(img, tile);
    msImageFilledPolygon(img,p,gdTiled);
    if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);
    gdImageDestroy(tile);

    break;
  case(MS_SYMBOL_VECTOR):
    d = size/symbol->sizey; // size ~ height in pixels
    x = MS_NINT(symbol->sizex*d)+1;    
    y = MS_NINT(symbol->sizey*d)+1;

    if((x <= 1) && (y <= 1)) { // No sense using a tile, just fill solid //
      msImageFilledPolygon(img, p, fc);
      if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);
      return;
    }

    //
   //create tile image
    //
    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bc = gdImageColorAllocate(tile, gdImageRed(img, bc), gdImageGreen(img, bc), gdImageBlue(img, bc));
    else {
      tile_bc = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }
    tile_fc = gdImageColorAllocate(tile, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc));
   
    //
   //draw in the tile image
    //
    if(symbol->filled) {

      for(i=0;i < symbol->numpoints;i++) {
	sPoints[i].x = MS_NINT(d*symbol->points[i].x);
	sPoints[i].y = MS_NINT(d*symbol->points[i].y);
      }
      gdImageFilledPolygon(tile, sPoints, symbol->numpoints, tile_fc);      

    } else  { // shade is a vector drawing //
      
      oldpnt.x = MS_NINT(d*symbol->points[0].x); // convert first point in shade smbol //
      oldpnt.y = MS_NINT(d*symbol->points[0].y);

      // step through the shade sy //
      for(i=1;i < symbol->numpoints;i++) {
	if((symbol->points[i].x < 0) && (symbol->points[i].y < 0)) {
	  oldpnt.x = MS_NINT(d*symbol->points[i].x);
	  oldpnt.y = MS_NINT(d*symbol->points[i].y);
	} else {
	  if((symbol->points[i-1].x < 0) && (symbol->points[i-1].y < 0)) { // Last point was PENUP, now a new beginning //
	    oldpnt.x = MS_NINT(d*symbol->points[i].x);
	    oldpnt.y = MS_NINT(d*symbol->points[i].y);
	  } else {
	    newpnt.x = MS_NINT(d*symbol->points[i].x);
	    newpnt.y = MS_NINT(d*symbol->points[i].y);
	    gdImageLine(tile, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, tile_fc);
	    oldpnt = newpnt;
	  }
	}
      } // end for loop //
    }

    //
   //Fill the polygon in the main image
    //
    gdImageSetTile(img, tile);
    msImageFilledPolygon(img, p, imTiled);
    if(oc>-1)
      imagePolyline(img, p, oc, style->offsetx, style->offsety);

    break;
  default:
    break;
  }
*/
  return;
}

#ifdef notdef /* not currently used */
static void billboardIM(imageObj* img, shapeObj *shape, labelObj *label)
{
/* int i; */
/* shapeObj temp; */
DEBUG_IF printf("billboardIM<BR>\n");
/*
  msInitShape(&temp);
  msAddLine(&temp, &shape->line[0]);

  if(label->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(label->backgroundcolor));
  if(label->backgroundshadowcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(label->backgroundshadowcolor));

  if(label->backgroundshadowcolor.pen >= 0) {
    for(i=0; i<temp.line[0].numpoints; i++) {
      temp.line[0].point[i].x += label->backgroundshadowsizex;
      temp.line[0].point[i].y += label->backgroundshadowsizey;
    }
    msImageFilledPolygon(img, &temp, label->backgroundshadowcolor.pen);
    for(i=0; i<temp.line[0].numpoints; i++) {
      temp.line[0].point[i].x -= label->backgroundshadowsizex;
      temp.line[0].point[i].y -= label->backgroundshadowsizey;
    }
  }

  msImageFilledPolygon(img, &temp, label->backgroundcolor.pen);

  msFreeShape(&temp);
*/
  
}
#endif /* def notdef */

/*
 * Simply draws a label based on the label point and the supplied label object.
 */
int msDrawTextIM(imageObj* img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor)
{
  int x, y;
		

  DEBUG_IF printf("msDrawText<BR>\n");
  if(!string) return(0); /* not errors, just don't want to do anything */
  if(strlen(string) == 0) return(0);
  if(!dxf) return (0);
  x = MS_NINT(labelPnt.x);
  y = MS_NINT(labelPnt.y);

	if (dxf == 2) {
		im_iprintf (&imgStr, "TEXT\n%d\n%s\n%.0f\n%.0f\n%.0f\n" , matchdxfcolor(label->color), string, labelPnt.x, labelPnt.y, -label->angle);
	} else if (dxf) {
		im_iprintf (&imgStr, "  0\nTEXT\n  1\n%s\n 10\n%f\n 20\n%f\n 30\n0.0\n 40\n%f\n 50\n%f\n 62\n%6d\n  8\n%s\n 73\n   2\n 72\n   1\n" , string, labelPnt.x, labelPnt.y, label->size * scalefactor *100, -label->angle, matchdxfcolor(label->color), lname);
	}
/*
  if(label->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(label->color));
  if(label->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(label->outlinecolor));
  if(label->shadowcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(label->shadowcolor));

  if(label->type == MS_TRUETYPE) {
    char *error=NULL, *font=NULL;
    int bbox[8];
    double angle_radians = MS_DEG_TO_RAD*label->angle;
    double size;

    size = label->size*scalefactor;

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    if(!fontset) {
      msSetError(MS_TTFERR, "No fontset defined.", "msDrawTextIM()");
      return(-1);
    }

    if(!label->font) {
      msSetError(MS_TTFERR, "No Trueype font defined.", "msDrawTextIM()");
      return(-1);
    }

    font = msLookupHashTable(&(fontset->fonts), label->font);
    if(!font) {
       msSetError(MS_TTFERR, "Requested font (%s) not found.", "msDrawTextIM()",
                  label->font);
      return(-1);
    }

    if(label->outlinecolor.pen >= 0) { // handle the outline color //
#ifdef USE_GD_TTF
      error = gdImageStringTTF(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x-1, y-1, string);
#else
      error = gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x-1, y-1, string);
#endif
      if(error) {
	msSetError(MS_TTFERR, error, "msDrawTextIM()");
	return(-1);
      }

#ifdef USE_GD_TTF
      gdImageStringTTF(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x-1, y+1, string);
      gdImageStringTTF(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x+1, y+1, string);
      gdImageStringTTF(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x+1, y-1, string);
#else
      gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x-1, y+1, string);
      gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x+1, y+1, string);
      gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x+1, y-1, string);
#endif

    }

    if(label->shadowcolor.pen >= 0) { // handle the shadow color //
#ifdef USE_GD_TTF
      error = gdImageStringTTF(img, bbox, ((label->antialias)?(label->shadowcolor.pen):-(label->shadowcolor.pen)), font, size, angle_radians, x+label->shadowsizex, y+label->shadowsizey, string);
#else
      error = gdImageStringFT(img, bbox, ((label->antialias)?(label->shadowcolor.pen):-(label->shadowcolor.pen)), font, size, angle_radians, x+label->shadowsizex, y+label->shadowsizey, string);
#endif
      if(error) {
	msSetError(MS_TTFERR, error, "msDrawTextIM()");
	return(-1);
      }
    }

#ifdef USE_GD_TTF
    gdImageStringTTF(img, bbox, ((label->antialias)?(label->color.pen):-(label->color.pen)), font, size, angle_radians, x, y, string);
#else
    gdImageStringFT(img, bbox, ((label->antialias)?(label->color.pen):-(label->color.pen)), font, size, angle_radians, x, y, string);
#endif

#else
    msSetError(MS_TTFERR, "TrueType font support is not available.", "msDrawTextIM()");
    return(-1);
#endif

  } else { // MS_BITMAP //
    char **token=NULL;
    int t, num_tokens;
    imFontPtr fontPtr;

    if((fontPtr = msGetBitmapFont(label->size)) == NULL)
      return(-1);

    if(label->wrap != '\0') {
      if((token = msStringSplit(string, label->wrap, &(num_tokens))) == NULL)
	return(-1);

      y -= fontPtr->h*num_tokens;
      for(t=0; t<num_tokens; t++) {
	if(label->outlinecolor.pen >= 0) {
	  gdImageString(img, fontPtr, x-1, y-1, token[t], label->outlinecolor.pen);
	  gdImageString(img, fontPtr, x-1, y+1, token[t], label->outlinecolor.pen);
	  gdImageString(img, fontPtr, x+1, y+1, token[t], label->outlinecolor.pen);
	  gdImageString(img, fontPtr, x+1, y-1, token[t], label->outlinecolor.pen);
	}

	if(label->shadowcolor.pen >= 0)
	  gdImageString(img, fontPtr, x+label->shadowsizex, y+label->shadowsizey, token[t], label->shadowcolor.pen);

	gdImageString(img, fontPtr, x, y, token[t], label->color.pen);

	y += fontPtr->h; // shift down //
      }

      msFreeCharArray(token, num_tokens);
    } else {
      y -= fontPtr->h;

      if(label->outlinecolor.pen >= 0) {
	gdImageString(img, fontPtr, x-1, y-1, string, label->outlinecolor.pen);
	gdImageString(img, fontPtr, x-1, y+1, string, label->outlinecolor.pen);
	gdImageString(img, fontPtr, x+1, y+1, string, label->outlinecolor.pen);
	gdImageString(img, fontPtr, x+1, y-1, string, label->outlinecolor.pen);
      }

      if(label->shadowcolor.pen >= 0)
	gdImageString(img, fontPtr, x+label->shadowsizex, y+label->shadowsizey, string, label->shadowcolor.pen);

      gdImageString(img, fontPtr, x, y, string, label->color.pen);
    }
  }
*/
  return(0);
}

int msDrawLabelCacheIM(imageObj* img, mapObj *map)
{
  pointObj p;
  int i, j, l, priority;
  rectObj r;
  
  labelCacheMemberObj *cachePtr=NULL;
  layerObj *layerPtr=NULL;
  labelObj *labelPtr=NULL;

  int marker_width, marker_height;
  int marker_offset_x, marker_offset_y, label_offset_x, label_offset_y;
  rectObj marker_rect;
  int label_mindistance, label_buffer;

  label_mindistance=-1;
  label_buffer=0;

  DEBUG_IF printf("msDrawLabelCacheIM\n<BR>");
  for(priority=MS_MAX_LABEL_PRIORITY-1; priority>=0; priority--) {
   labelCacheSlotObj *cacheslot;
   cacheslot = &(map->labelcache.slots[priority]);

   for(l=cacheslot->numlabels-1; l>=0; l--) {

    cachePtr = &(cacheslot->labels[l]); /* point to right spot in the label cache */

    layerPtr = (GET_LAYER(map, cachePtr->layerindex)); /* set a couple of other pointers, avoids nasty references */
    labelPtr = &(cachePtr->label);

    if(!cachePtr->text || strlen(cachePtr->text) == 0)
      continue; /* not an error, just don't want to do anything */

    if(cachePtr->label.type == MS_TRUETYPE)
      cachePtr->label.size = (cachePtr->label.size*layerPtr->scalefactor);

    if(msGetLabelSize(img,cachePtr->text, labelPtr, &r, &(map->fontset), layerPtr->scalefactor, MS_TRUE,NULL) == -1)
      return(-1);

    label_offset_x = labelPtr->offsetx*layerPtr->scalefactor;
    label_offset_y = labelPtr->offsety*layerPtr->scalefactor;
    label_buffer = MS_NINT(labelPtr->buffer*img->resolutionfactor);
    label_mindistance = MS_NINT(labelPtr->mindistance*img->resolutionfactor);

    if(labelPtr->autominfeaturesize && (cachePtr->featuresize != -1) && ((r.maxx-r.minx) > cachePtr->featuresize))
      continue; /* label too large relative to the feature */

    marker_offset_x = marker_offset_y = 0; /* assume no marker */
    if((layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) || layerPtr->type == MS_LAYER_POINT) { /* there *is* a marker       */

      /* TO DO: at the moment only checks the bottom style, perhaps should check all of them */
      if (msGetMarkerSize(&map->symbolset, &(cachePtr->styles[0]), &marker_width, &marker_height, layerPtr->scalefactor) != MS_SUCCESS)
	return(-1);

      marker_width = (int) (marker_width * layerPtr->scalefactor);
      marker_height = (int) (marker_height * layerPtr->scalefactor);

      marker_offset_x = MS_NINT(marker_width/2.0);
      marker_offset_y = MS_NINT(marker_height/2.0);      

      marker_rect.minx = MS_NINT(cachePtr->point.x - .5 * marker_width);
      marker_rect.miny = MS_NINT(cachePtr->point.y - .5 * marker_height);
      marker_rect.maxx = marker_rect.minx + (marker_width-1);
      marker_rect.maxy = marker_rect.miny + (marker_height-1);

      for(i=0; i<cachePtr->numstyles; i++) {
        if(cachePtr->styles[i].size == -1)
          cachePtr->styles[i].size = (int) msSymbolGetDefaultSize( 
              map->symbolset.symbol[cachePtr->styles[i].symbol] );
	cachePtr->styles[i].size = (int)
            ( cachePtr->styles[i].size * layerPtr->scalefactor);
      }
    }

    if(labelPtr->position == MS_AUTO) {

      if(layerPtr->type == MS_LAYER_LINE) {
	int position = MS_UC;

	for(j=0; j<2; j++) { /* Two angles or two positions, depending on angle. Steep angles will use the angle approach, otherwise we'll rotate between UC and LC. */

	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

	  p = get_metrics(&(cachePtr->point), position, r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, label_buffer, cachePtr->poly);

	  if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
	    msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon */

          /* Compare against rendered labels and markers (sets cachePtr->status) */
          msTestLabelCacheCollisions(&(map->labelcache), labelPtr, 
                                     -1, -1, label_buffer, cachePtr, priority, l, label_mindistance, (r.maxx-r.minx));

	  if(cachePtr->status) /* found a suitable place for this label */
	    break;

	} /* next angle */

      } else {
	for(j=0; j<=7; j++) { /* loop through the outer label positions */

	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

	  p = get_metrics(&(cachePtr->point), j, r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, label_buffer, cachePtr->poly);

	  if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
	    msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon */

          /* Compare against rendered labels and markers (sets cachePtr->status) */
          msTestLabelCacheCollisions(&(map->labelcache), labelPtr, 
                                     -1, -1, label_buffer, cachePtr, priority, l,label_mindistance, (r.maxx-r.minx));

	  if(cachePtr->status) /* found a suitable place for this label */
	    break;
	} /* next position */
      }

      if(labelPtr->force) cachePtr->status = MS_TRUE; /* draw in spite of collisions based on last position, need a *best* position */

    } else {

      cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

      if(labelPtr->position == MS_CC) /* don't need the marker_offset */
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, label_offset_x, label_offset_y, labelPtr->angle, label_buffer, cachePtr->poly);
      else
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, label_buffer, cachePtr->poly);

      if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
	msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon, part of overlap tests */

      if(!labelPtr->force) { /* no need to check anything else */

        /* Compare against rendered labels and markers (sets cachePtr->status) */
        msTestLabelCacheCollisions(&(map->labelcache), labelPtr, 
                                   -1, -1, label_buffer, cachePtr, priority, l,label_mindistance, (r.maxx-r.minx));
      }
    } /* end position if-then-else */

    /* imagePolyline(img, cachePtr->poly, 1, 0, 0); */

    if(!cachePtr->status)
      continue; /* next label */

/* if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) { // need to draw a marker */
/* for(i=0; i<cachePtr->numstyles; i++) */
/* msDrawMarkerSymbolIM(&map->symbolset, img, &(cachePtr->point), &(cachePtr->styles[i]), layerPtr->scalefactor); */
/* } */

/* if(MS_VALID_COLOR(labelPtr->backgroundcolor)) billboardIM(img, cachePtr->poly, labelPtr); */
    msDrawTextIM(img, p, cachePtr->text, labelPtr, &(map->fontset), layerPtr->scalefactor); /* actually draw the label, we scaled it in msAddLabel */

   } /* next label in cache */
  } /* next priority */

  return(0);
}

/*
 * Save an image to a file named filename, if filename is NULL it goes to stdout
 */

int msSaveImageIM(imageObj* img, char *filename, outputFormatObj *format )

{
    FILE *stream;
    char workbuffer[5000];
    int nSize=0, size=0, iIndice=0; 

DEBUG_IF printf("msSaveImageIM\n<BR>");

if(filename != NULL && strlen(filename) > 0) {
    stream = fopen(filename, "wb");
    if(!stream) {
      msSetError(MS_IOERR, "(%s)", "msSaveImage()", filename);
      return(MS_FAILURE);
    }
  } else { /* use stdout */

#ifdef _WIN32
    /*
     * Change stdout mode to binary on win32 platforms
     */
    if(_setmode( _fileno(stdout), _O_BINARY) == -1) {
      msSetError(MS_IOERR, "Unable to change stdout to binary mode.", "msSaveImage()");
      return(MS_FAILURE);
    }
#endif
    stream = stdout;
  }

/*  if( strcasecmp("ON",msGetOutputFormatOption( format, "INTERLACE", "ON" ))
      == 0 )
      gdImageInterlace(img, 1);

  if(format->transparent)
    gdImageColorTransparent(img, 0);
*/
  if( strcasecmp(format->driver,"imagemap") == 0 )
  {
DEBUG_IF printf("ALLOCD %d<BR>\n", img->size);
/* DEBUG_IF printf("F %s<BR>\n", img->img.imagemap); */
DEBUG_IF printf("FLEN %d<BR>\n", (int)strlen(img->img.imagemap));
	  if (dxf == 2){
	    msIO_fprintf(stream, "%s", layerlist); 
	  } else if (dxf){
	    msIO_fprintf(stream, "  0\nSECTION\n  2\nHEADER\n  9\n$ACADVER\n  1\nAC1009\n0\nENDSEC\n  0\nSECTION\n  2\nTABLES\n  0\nTABLE\n%s0\nENDTAB\n0\nENDSEC\n  0\nSECTION\n  2\nBLOCKS\n0\nENDSEC\n  0\nSECTION\n  2\nENTITIES\n", layerlist); 
	  } else {
	    msIO_fprintf(stream, "<map name=\"%s\" width=\"%d\" height=\"%d\">\n", mapName, img->width, img->height);
    	  }
          nSize = sizeof(workbuffer);

          size = strlen(img->img.imagemap);
          if (size > nSize)
          {
              iIndice = 0;
              while ((iIndice + nSize) <= size)
              {
                  snprintf(workbuffer, sizeof(workbuffer), "%s", img->img.imagemap+iIndice );
                  workbuffer[nSize-1] = '\0';
                  msIO_fwrite(workbuffer, strlen(workbuffer), 1, stream);
                  iIndice +=nSize-1;
              }
              if (iIndice < size)
              {
                  sprintf(workbuffer, "%s", img->img.imagemap+iIndice );
                  msIO_fprintf(stream, workbuffer);
              }
          }
          else
              msIO_fwrite(img->img.imagemap, size, 1, stream);
	  if( strcasecmp("OFF",msGetOutputFormatOption( format, "SKIPENDTAG", "OFF" )) == 0){
		  if (dxf == 2)
			  msIO_fprintf(stream, "END");
		  else if (dxf)
			  msIO_fprintf(stream, "0\nENDSEC\n0\nEOF\n");
		  else 
			  msIO_fprintf(stream, "</map>");
/*#ifdef USE_GD_GIF
    gdImageGif(img, stream);
#else
    msSetError(MS_MISCERR, "GIF output is not available.", "msSaveImage()");
    return(MS_FAILURE);
#endif*/
	  }
  }
  else
  {
      msSetError(MS_MISCERR, "Unknown output image type driver: %s.", 
                 "msSaveImage()", format->driver );
      return(MS_FAILURE);
  }

  if(filename != NULL && strlen(filename) > 0) fclose(stream);
  return(MS_SUCCESS);
}


/*
 * Free gdImagePtr
 */
void msFreeImageIM(imageObj* img)
{
  free(img->img.imagemap);
}


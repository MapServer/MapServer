/* $Id$ */

#include "map.h"
#include "mapparser.h"

#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

static unsigned char PNGsig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; // 89 50 4E 47 0D 0A 1A 0A hex
static unsigned char JPEGsig[3] = {255, 216, 255}; // FF D8 FF hex

static gdImagePtr searchImageCache(struct imageCacheObj *ic, int symbol, int color, int size) {
  struct imageCacheObj *icp;

  icp = ic;
  while(icp) {
    if(icp->symbol == symbol && icp->color == color && icp->size == size) return(icp->img);
    icp = icp->next;
  }

  return(NULL);
}

static struct imageCacheObj *addImageCache(struct imageCacheObj *ic, int *icsize, int symbol, int color, int size, gdImagePtr img) {
  struct imageCacheObj *icp;

  if(*icsize > MS_IMAGECACHESIZE) { // remove last element, size stays the same
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
  icp->color = color;
  icp->symbol = symbol;
  icp->size = size;
  icp->next = ic; // insert at the beginning
 
  return(icp);
}


/**
 * Utility function to create a GD image. Returns
 * a pointer to an imageObj structure.
 */  
imageObj *msImageCreateGD(int width, int height, outputFormatObj *format,
                          char *imagepath, char *imageurl)
{
    imageObj  *image;

    if (width > 0 && height > 0)
    {
        image = (imageObj *)calloc(1,sizeof(imageObj));

        if( format->imagemode == MS_IMAGEMODE_RGB 
            || format->imagemode == MS_IMAGEMODE_RGBA )
        {
#if GD2_VERS > 1
            image->img.gd = gdImageCreateTrueColor(width, height);
#else
            msSetError(MS_IMGERR, 
                       "Attempt to use RGB or RGBA IMAGEMODE with GD 1.x, please upgrade to GD 2.x.", "msImageCreateGD()" );
#endif
        }
        else
            image->img.gd = gdImageCreate(width, height);
    
        if (image->img.gd)
        {
            image->format = format;
            format->refcount++;

            image->width = width;
            image->height = height;
            image->imagepath = NULL;
            image->imageurl = NULL;
            
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
    return NULL;
}

/**
 * Utility function to initialize the color of an image.  The background
 * color is passed, but the outputFormatObj is consulted to see if the
 * transparency should be set (for RGBA images).   Note this function only
 * affects TrueColor images. 
 */  

void msImageInitGD( imageObj *image, colorObj background )

{
    if( image->format->imagemode == MS_IMAGEMODE_PC256 )
        return;

#if GD2_VERS > 1
    {
        int		pen, pixels, line;
        int             *tpixels;

        if( image->format->imagemode == MS_IMAGEMODE_RGBA )
            pen = gdTrueColorAlpha( background.red, 
                                    background.green, 
                                    background.blue,
                                    image->format->transparent ? 127 : 0 );
        else
            pen = gdTrueColor( background.red, 
                               background.green, 
                               background.blue );

        for( line = 0; line < image->img.gd->sy; line++ )
        {
            pixels = image->img.gd->sx;
            tpixels = image->img.gd->tpixels[line];

            while( pixels-- > 0 )
                *(tpixels++) = pen;
        }
    }
#endif
}

/**
 * Utility function to load an image in a GD supported format, and
 * return it as a valid imageObj.
 */  
imageObj *msImageLoadGD( const char *filename )
{
  FILE *stream;
  gdImagePtr img=NULL;
  const char *driver = NULL;
  char bytes[8];
  imageObj      *image = NULL;

  image = (imageObj *)calloc(1,sizeof(imageObj));
  
  stream = fopen(filename,"rb"); // allocate input and output images (same size)
  if(!stream) {
    msSetError(MS_IOERR, "(%s)", "msImageLoadGD()", filename );
    return(NULL);
  }

  fread(bytes,8,1,stream); // read some bytes to try and identify the file
  rewind(stream); // reset the image for the readers
  if (memcmp(bytes,"GIF8",4)==0) {
#ifdef USE_GD_GIF
    img = gdImageCreateFromGif(stream);
    driver = "GD/GIF";
    image->img.gd = img;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->width = img->sx;
    image->height = img->sy;
#else
    msSetError(MS_MISCERR, "Unable to load GIF reference image.", "msImageLoadGD()");
    fclose(stream);
    return(NULL);
#endif
  } else if (memcmp(bytes,PNGsig,8)==0) {
#ifdef USE_GD_PNG
    img = gdImageCreateFromPng(stream);
    driver = "GD/PNG";
    image->img.gd = img;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->width = img->sx;
    image->height = img->sy;
#else
    msSetError(MS_MISCERR, "Unable to load PNG reference image.", "msImageLoadGD()");
    fclose(stream);
    return(NULL);
#endif
  } else if (memcmp(bytes,JPEGsig,3)==0) {
#ifdef USE_GD_JPEG
    img = gdImageCreateFromJpeg(stream);
    driver = "GD/JPEG";
    image->img.gd = img;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->width = img->sx;
    image->height = img->sy;
#else
    msSetError(MS_MISCERR, "Unable to load JPEG reference image.", "msImageLoadGD()");
    fclose(stream);
    return(NULL);
#endif
  }

  if(!img) {
    msSetError(MS_GDERR, "Unable to initialize image '%s'", "msLoadImage()", 
               filename);
    fclose(stream);
    return(NULL);
  }

  /* Create an outputFormatObj for the format. */
  image->format = msCreateDefaultOutputFormat( NULL, driver );

  if( image->format == NULL )
  {
    msSetError(MS_GDERR, "Unable to create default OUTPUTFORMAT definition for driver '%s'.", "msImageLoadGD()", driver );
    return(NULL);
  }

  return image;
}

/* ---------------------------------------------------------------------------*/
/*       Stroke an ellipse with a line symbol of the specified size and color */
/* ---------------------------------------------------------------------------*/
void msCircleDrawLineSymbolGD(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, int sy, int fc, int bc, double sz)
{
  int i, j;
  symbolObj *symbol;
  int styleDashed[100];
  int x, y;
  int brush_bc, brush_fc;
  gdImagePtr brush=NULL;
  gdPoint points[MS_MAXVECTORPOINTS];

  double scale=1.0;
  
  if(!p) return;

  if(sy > symbolset->numsymbols || sy < 0) // no such symbol, 0 is OK
    return;

  if((fc < 0) || (fc >= gdImageColorsTotal(img))) // invalid color
    return;

  if(sz < 1) // size too small
    return;

  if(sy == 0) { // just draw a single width line
    gdImageArc(img, (int)p->x, (int)p->y, (int)r, (int)r, 0, 360, fc);
    return;
  }

  symbol = &(symbolset->symbol[sy]);

  switch(symbol->type) {
  case(MS_SYMBOL_SIMPLE):
    if(bc == -1) bc = gdTransparent;
    break;
  case(MS_SYMBOL_TRUETYPE):
    // msImageTruetypePolyline(img, p, symbol, fc, sz, symbolset->fontset);
    return;
    break;
  case(MS_SYMBOL_CARTOLINE):
    return;
    break;
  case(MS_SYMBOL_ELLIPSE):
    bc = gdTransparent;

    scale = (sz)/symbol->sizey;
    x = MS_NINT(symbol->sizex*scale);    
    y = MS_NINT(symbol->sizey*scale);
    
    if((x < 2) && (y < 2)) break;
    
    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, sy, fc, sz)) == NULL) { // not in cache, create
      brush = gdImageCreate(x, y);
      brush_bc = gdImageColorAllocate(brush,gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));    
      gdImageColorTransparent(brush,0);
      brush_fc = gdImageColorAllocate(brush, gdImageRed(img,fc), gdImageGreen(img,fc), gdImageBlue(img,fc));
      
      x = MS_NINT(brush->sx/2); // center the ellipse
      y = MS_NINT(brush->sy/2);
      
      // draw in the brush image
      gdImageArc(brush, x, y, MS_NINT(scale*symbol->points[0].x), MS_NINT(scale*symbol->points[0].y), 0, 360, brush_fc);
      if(symbol->filled)
	gdImageFillToBorder(brush, x, y, brush_fc, brush_fc);
      
      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, sy, fc, sz, brush);
    }

    gdImageSetBrush(img, brush);
    fc = 1; bc = 0;
    break;
  case(MS_SYMBOL_PIXMAP):
    gdImageSetBrush(img, symbol->img);
    fc = 1; bc = 0;
    break;
  case(MS_SYMBOL_VECTOR):
    if(bc == -1) bc = gdTransparent;

    scale = sz/symbol->sizey;
    x = MS_NINT(symbol->sizex*scale);    
    y = MS_NINT(symbol->sizey*scale);

    if((x < 2) && (y < 2)) break;

    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, sy, fc, sz)) == NULL) { // not in cache, create
      brush = gdImageCreate(x, y);
      if(bc >= 0)
	brush_bc = gdImageColorAllocate(brush, gdImageRed(img,bc), gdImageGreen(img,bc), gdImageBlue(img,bc));
      else {
	brush_bc = gdImageColorAllocate(brush, gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));
	gdImageColorTransparent(brush,0);
      }
      brush_fc = gdImageColorAllocate(brush,gdImageRed(img,fc), gdImageGreen(img,fc), gdImageBlue(img,fc));
      
      // draw in the brush image 
      for(i=0;i < symbol->numpoints;i++) {
	points[i].x = MS_NINT(scale*symbol->points[i].x);
	points[i].y = MS_NINT(scale*symbol->points[i].y);
      }
      gdImageFilledPolygon(brush, points, symbol->numpoints, brush_fc);

      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, sy, fc, sz, brush);
    }

    gdImageSetBrush(img, brush);
    fc = 1; bc = 0;
    break;
  }  

  if(symbol->stylelength > 0) {
    int k=0, sc;
   
    sc = fc; // start with foreground color
    for(i=0; i<symbol->stylelength; i++) {      
      for(j=0; j<symbol->style[i]; j++) {
	styleDashed[k] = sc;
	k++;
      } 
      if(sc==fc) sc = bc; else sc = fc;
    }
    gdImageSetStyle(img, styleDashed, k);

    // gdImageArc(brush, x, y, MS_NINT(scale*symbol->points[0].x), MS_NINT(scale*symbol->points[0].y), 0, 360, brush_fc);

    if(!brush && !symbol->img)
      gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, gdStyled);      
    else 
      gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, gdStyledBrushed);
  } else {
    if(!brush && !symbol->img)
      gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, fc);
    else
      gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, gdBrushed);
  }

  return;
}

/* ------------------------------------------------------------------------------- */
/*       Fill a circle with a shade symbol of the specified size and color       */
/* ------------------------------------------------------------------------------- */
void msCircleDrawShadeSymbolGD(symbolSetObj *symbolset, gdImagePtr img, 
                               pointObj *p, double r, int sy, int fc, int bc, 
                               int oc, double sz)
{
  symbolObj *symbol;
  int i;
  gdPoint oldpnt,newpnt;
  gdPoint sPoints[MS_MAXVECTORPOINTS];
  gdImagePtr tile=NULL;
  int x,y;
  int tile_bg=-1, tile_fg=-1; /* colors (background and foreground) */
  
  double scale=1.0;
  
  int bbox[8];
  rectObj rect;
  char *font=NULL;

  if(!p) return;
  if(sy > symbolset->numsymbols || sy < 0) return; // no such symbol, 0 is OK
  if(fc >= gdImageColorsTotal(img)) return; // invalid color, -1 is valid
  if(sz < 1) return; // size too small
      
  if(sy == 0) { // simply draw a single pixel of the specified color
    if(fc>-1)
      msImageFilledCircle(img, p, r, fc);
    if(oc>-1)
      gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);
    return;
  }

  if(fc<0) {
    if(oc>-1)
      gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);
    return;
  }

  symbol = &(symbolset->symbol[sy]);

  switch(symbol->type) {
  case(MS_SYMBOL_TRUETYPE):    
    
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    font = msLookupHashTable(symbolset->fontset->fonts, symbolset->symbol[sy].font);
    if(!font) return;

    if(getCharacterSize(symbol->character, sz, font, &rect) == -1) return;
    x = rect.maxx - rect.minx;
    y = rect.maxy - rect.miny;

    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, bc), gdImageGreen(img, bc), gdImageBlue(img, bc));
    else {
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }    
    tile_fg = gdImageColorAllocate(tile, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc));
    
    x = -rect.minx;
    y = -rect.miny;

#ifdef USE_GD_TTF
    gdImageStringTTF(tile, bbox, ((symbol->antialias)?(tile_fg):-(tile_fg)), font, sz, 0, x, y, symbol->character);
#else
    gdImageStringFT(tile, bbox, ((symbol->antialias)?(tile_fg):-(tile_fg)), font, sz, 0, x, y, symbol->character);
#endif    

    gdImageSetTile(img, tile);
#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    
    gdImageSetTile(img, symbol->img);

    break;
  case(MS_SYMBOL_ELLIPSE):    
   
    scale = sz/symbol->sizey; // sz ~ height in pixels
    x = MS_NINT(symbol->sizex*scale)+1;
    y = MS_NINT(symbol->sizey*scale)+1;

    if((x <= 1) && (y <= 1)) { // No sense using a tile, just fill solid
      msImageFilledCircle(img, p, r, fc);
      if(oc>-1)
        gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);
      return;
    }

    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, bc), gdImageGreen(img, bc), gdImageBlue(img, bc));
    else {
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }    
    tile_fg = gdImageColorAllocate(tile, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc));
    
    x = MS_NINT(tile->sx/2);
    y = MS_NINT(tile->sy/2);

    /*
    ** draw in the tile image
    */
    gdImageArc(tile, x, y, MS_NINT(scale*symbol->points[0].x), MS_NINT(scale*symbol->points[0].y), 0, 360, tile_fg);
    if(symbol->filled)
      gdImageFillToBorder(tile, x, y, tile_fg, tile_fg);

    /*
    ** Fill the polygon in the main image
    */
    gdImageSetTile(img, tile);
 
    break;
  case(MS_SYMBOL_VECTOR):

    if(fc < 0)
      return;

    scale = sz/symbol->sizey; // sz ~ height in pixels
    x = MS_NINT(symbol->sizex*scale)+1;    
    y = MS_NINT(symbol->sizey*scale)+1;

    if((x <= 1) && (y <= 1)) { // No sense using a tile, just fill solid
      msImageFilledCircle(img, p, r, fc);
      if(oc>-1)
        gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);
      return;
    }

    /*
    ** create tile image
    */
    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, bc), gdImageGreen(img, bc), gdImageBlue(img, bc));
    else {
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }
    tile_fg = gdImageColorAllocate(tile, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc));
   
    /*
    ** draw in the tile image
    */
    if(symbol->filled) {

      for(i=0;i < symbol->numpoints;i++) {
	sPoints[i].x = MS_NINT(scale*symbol->points[i].x);
	sPoints[i].y = MS_NINT(scale*symbol->points[i].y);
      }
      gdImageFilledPolygon(tile, sPoints, symbol->numpoints, tile_fg);      

    } else  { /* shade is a vector drawing */
      
      oldpnt.x = MS_NINT(scale*symbol->points[0].x); /* convert first point in shade smbol */
      oldpnt.y = MS_NINT(scale*symbol->points[0].y);

      /* step through the shade sy */
      for(i=1;i < symbol->numpoints;i++) {
	if((symbol->points[i].x < 0) && (symbol->points[i].y < 0)) {
	  oldpnt.x = MS_NINT(scale*symbol->points[i].x);
	  oldpnt.y = MS_NINT(scale*symbol->points[i].y);
	} else {
	  if((symbol->points[i-1].x < 0) && (symbol->points[i-1].y < 0)) { /* Last point was PENUP, now a new beginning */
	    oldpnt.x = MS_NINT(scale*symbol->points[i].x);
	    oldpnt.y = MS_NINT(scale*symbol->points[i].y);
	  } else {
	    newpnt.x = MS_NINT(scale*symbol->points[i].x);
	    newpnt.y = MS_NINT(scale*symbol->points[i].y);
	    gdImageLine(tile, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, tile_fg);
	    oldpnt = newpnt;
	  }
	}
      } /* end for loop */
    }

    /*
    ** Fill the polygon in the main image
    */
    gdImageSetTile(img, tile);
 
    break;
  default:
    break;
  }

  /*
  ** Fill the polygon in the main image
  */
  msImageFilledCircle(img, p, r, gdTiled);
  if(oc>-1)
    gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);

  if(tile) gdImageDestroy(tile);

  return;
}


/* ------------------------------------------------------------------------------- */
/*       Draw a single marker symbol of the specified size and color               */
/* ------------------------------------------------------------------------------- */
void msDrawMarkerSymbolGD(symbolSetObj *symbolset, gdImagePtr img, 
                          pointObj *p, int sy, int fc, int bc, int oc, 
                          double sz)
{
  symbolObj *symbol;
  int offset_x, offset_y, x, y;
  int j;
  gdPoint oldpnt,newpnt;
  gdPoint mPoints[MS_MAXVECTORPOINTS];

  gdImagePtr tmp;
  int tmp_fc=-1, tmp_bc, tmp_oc=-1;

  double scale=1.0;

  int bbox[8];
  rectObj rect;
  char *font=NULL;

  if(sy > symbolset->numsymbols || sy < 0) /* no such symbol, 0 is OK */
    return;

  if(fc >= gdImageColorsTotal(img)) /* invalid color, -1 is valid */
    return;

  if(sz < 1) /* size too small */
    return;

  if(sy == 0 && fc >= 0) { /* simply draw a single pixel of the specified color */
    gdImageSetPixel(img, p->x, p->y, fc);
    return;
  }  

  symbol = &(symbolset->symbol[sy]);
  
  switch(symbol->type) {  
  case(MS_SYMBOL_TRUETYPE):

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    font = msLookupHashTable(symbolset->fontset->fonts, symbol->font);
    if(!font) return;

    if(getCharacterSize(symbol->character, sz, font, &rect) == -1) return;

    x = p->x - (rect.maxx - rect.minx)/2 - rect.minx;
    y = p->y - rect.maxy + (rect.maxy - rect.miny)/2;  

#ifdef USE_GD_TTF
    gdImageStringTTF(img, bbox, ((symbol->antialias)?(fc):-(fc)), font, sz, 0, x, y, symbol->character);
#else
    gdImageStringFT(img, bbox, ((symbol->antialias)?(fc):-(fc)), font, sz, 0, x, y, symbol->character);
#endif

#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    if(sz == 1) { // don't scale
      offset_x = MS_NINT(p->x - .5*symbol->img->sx);
      offset_y = MS_NINT(p->y - .5*symbol->img->sy);
      gdImageCopy(img, symbol->img, offset_x, offset_y, 0, 0, symbol->img->sx, symbol->img->sy);
    } else {
      scale = sz/symbol->img->sy;
      offset_x = MS_NINT(p->x - .5*symbol->img->sx*scale);
      offset_y = MS_NINT(p->y - .5*symbol->img->sy*scale);
      gdImageCopyResized(img, symbol->img, offset_x, offset_y, 0, 0, symbol->img->sx*scale, symbol->img->sy*scale, symbol->img->sx, symbol->img->sy);
    }
    break;
  case(MS_SYMBOL_ELLIPSE):
 
    scale = sz/symbol->sizey;
    x = MS_NINT(symbol->sizex*scale)+1;
    y = MS_NINT(symbol->sizey*scale)+1;

    /* create temporary image and allocate a few colors */
    tmp = gdImageCreate(x, y);
    tmp_bc = gdImageColorAllocate(tmp, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
    gdImageColorTransparent(tmp, 0);
    if(fc >= 0)
      tmp_fc = gdImageColorAllocate(tmp, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc));
    if(oc >= 0)
      tmp_oc = gdImageColorAllocate(tmp, gdImageRed(img, oc), gdImageGreen(img, oc), gdImageBlue(img, oc));

    x = MS_NINT(tmp->sx/2);
    y = MS_NINT(tmp->sy/2);

    /* draw in the temporary image */
    if(tmp_oc >= 0) {
      gdImageArc(tmp, x, y, MS_NINT(scale*symbol->points[0].x), MS_NINT(scale*symbol->points[0].y), 0, 360, tmp_oc);
      if(symbol->filled && tmp_fc >= 0)
	gdImageFillToBorder(tmp, x, y, tmp_oc, tmp_fc);
    } else {
      if(tmp_fc >= 0) {
	gdImageArc(tmp, x, y, MS_NINT(scale*symbol->points[0].x), MS_NINT(scale*symbol->points[0].y), 0, 360, tmp_fc);
	if(symbol->filled)
	  gdImageFillToBorder(tmp, x, y, tmp_fc, tmp_fc);
      }
    }

    /* paste the tmp image in the main image */
    offset_x = MS_NINT(p->x - .5*tmp->sx);
    offset_y = MS_NINT(p->y - .5*tmp->sx);
    gdImageCopy(img, tmp, offset_x, offset_y, 0, 0, tmp->sx, tmp->sy);

    gdImageDestroy(tmp);

    break;
  case(MS_SYMBOL_VECTOR):

    scale = sz/symbol->sizey;

    offset_x = MS_NINT(p->x - scale*.5*symbol->sizex);
    offset_y = MS_NINT(p->y - scale*.5*symbol->sizey);
    
    if(symbol->filled) { /* if filled */
      for(j=0;j < symbol->numpoints;j++) {
	mPoints[j].x = MS_NINT(scale*symbol->points[j].x + offset_x);
	mPoints[j].y = MS_NINT(scale*symbol->points[j].y + offset_y);
      }
      if(fc >= 0)
	gdImageFilledPolygon(img, mPoints, symbol->numpoints, fc);
      if(oc >= 0)
	gdImagePolygon(img, mPoints, symbol->numpoints, oc);
      
    } else  { /* NOT filled */     

      if(fc < 0) return;
      
      oldpnt.x = MS_NINT(scale*symbol->points[0].x + offset_x); /* convert first point in marker s */
      oldpnt.y = MS_NINT(scale*symbol->points[0].y + offset_y);
      
      for(j=1;j < symbol->numpoints;j++) { /* step through the marker s */
	if((symbol->points[j].x < 0) && (symbol->points[j].x < 0)) {
	  oldpnt.x = MS_NINT(scale*symbol->points[j].x + offset_x);
	  oldpnt.y = MS_NINT(scale*symbol->points[j].y + offset_y);
	} else {
	  if((symbol->points[j-1].x < 0) && (symbol->points[j-1].y < 0)) { /* Last point was PENUP, now a new beginning */
	    oldpnt.x = MS_NINT(scale*symbol->points[j].x + offset_x);
	    oldpnt.y = MS_NINT(scale*symbol->points[j].y + offset_y);
	  } else {
	    newpnt.x = MS_NINT(scale*symbol->points[j].x + offset_x);
	    newpnt.y = MS_NINT(scale*symbol->points[j].y + offset_y);
	    gdImageLine(img, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, fc);
	    oldpnt = newpnt;
	  }
	}
      } /* end for loop */      
    } /* end if-then-else */
    break;
  default:
    break;
  } /* end switch statement */

  return;
}


/*
** Simply draws a label based on the label point and the supplied label object.
*/
int draw_textGD(gdImagePtr img, pointObj labelPnt, char *string, 
                labelObj *label, fontSetObj *fontset)
{
  int x, y;

  if(!string)
    return(0); /* not an error, just don't want to do anything */

  if(strlen(string) == 0)
    return(0); /* not an error, just don't want to do anything */

  x = MS_NINT(labelPnt.x);
  y = MS_NINT(labelPnt.y);

  if(label->type == MS_TRUETYPE) {
    char *error=NULL, *font=NULL;
    int bbox[8];
    double angle_radians = MS_DEG_TO_RAD*label->angle;

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    if(!fontset) {
      msSetError(MS_TTFERR, "No fontset defined.", "draw_text()");
      return(-1);
    }

    if(!label->font) {
      msSetError(MS_TTFERR, "No TrueType font defined.", "draw_text()");
      return(-1);
    }

    font = msLookupHashTable(fontset->fonts, label->font);
    if(!font) {
       msSetError(MS_TTFERR, "Requested font (%s) not found.", "draw_text()",
                  label->font);
      return(-1);
    }

    if(label->outlinecolor >= 0) { /* handle the outline color */
#ifdef USE_GD_TTF
      error = gdImageStringTTF(img, bbox, ((label->antialias)?(label->outlinecolor):-(label->outlinecolor)), font, label->sizescaled, angle_radians, x-1, y-1, string);
#else
      error = gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor):-(label->outlinecolor)), font, label->sizescaled, angle_radians, x-1, y-1, string);
#endif
      if(error) {
	msSetError(MS_TTFERR, error, "draw_text()");
	return(-1);
      }

#ifdef USE_GD_TTF
      gdImageStringTTF(img, bbox, ((label->antialias)?(label->outlinecolor):-(label->outlinecolor)), font, label->sizescaled, angle_radians, x-1, y+1, string);
      gdImageStringTTF(img, bbox, ((label->antialias)?(label->outlinecolor):-(label->outlinecolor)), font, label->sizescaled, angle_radians, x+1, y+1, string);
      gdImageStringTTF(img, bbox, ((label->antialias)?(label->outlinecolor):-(label->outlinecolor)), font, label->sizescaled, angle_radians, x+1, y-1, string);
#else
      gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor):-(label->outlinecolor)), font, label->sizescaled, angle_radians, x-1, y+1, string);
      gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor):-(label->outlinecolor)), font, label->sizescaled, angle_radians, x+1, y+1, string);
      gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor):-(label->outlinecolor)), font, label->sizescaled, angle_radians, x+1, y-1, string);
#endif

    }

    if(label->shadowcolor >= 0) { /* handle the shadow color */
#ifdef USE_GD_TTF
      error = gdImageStringTTF(img, bbox, ((label->antialias)?(label->shadowcolor):-(label->shadowcolor)), font, label->sizescaled, angle_radians, x+label->shadowsizex, y+label->shadowsizey, string);
#else
      error = gdImageStringFT(img, bbox, ((label->antialias)?(label->shadowcolor):-(label->shadowcolor)), font, label->sizescaled, angle_radians, x+label->shadowsizex, y+label->shadowsizey, string);
#endif
      if(error) {
	msSetError(MS_TTFERR, error, "draw_text()");
	return(-1);
      }
    }

#ifdef USE_GD_TTF
    gdImageStringTTF(img, bbox, ((label->antialias)?(label->color):-(label->color)), font, label->sizescaled, angle_radians, x, y, string);
#else
    gdImageStringFT(img, bbox, ((label->antialias)?(label->color):-(label->color)), font, label->sizescaled, angle_radians, x, y, string);
#endif

#else
    msSetError(MS_TTFERR, "TrueType font support is not available.", "draw_text()");
    return(-1);
#endif

  } else { /* MS_BITMAP */
    char **token=NULL;
    int t, num_tokens;
    gdFontPtr fontPtr;

    if((fontPtr = msGetBitmapFont(label->size)) == NULL)
      return(-1);

    if(label->wrap != '\0') {
      if((token = split(string, label->wrap, &(num_tokens))) == NULL)
	return(-1);

      y -= fontPtr->h*num_tokens;
      for(t=0; t<num_tokens; t++) {
	if(label->outlinecolor >= 0) {
	  gdImageString(img, fontPtr, x-1, y-1, token[t], label->outlinecolor);
	  gdImageString(img, fontPtr, x-1, y+1, token[t], label->outlinecolor);
	  gdImageString(img, fontPtr, x+1, y+1, token[t], label->outlinecolor);
	  gdImageString(img, fontPtr, x+1, y-1, token[t], label->outlinecolor);
	}

	if(label->shadowcolor >= 0)
	  gdImageString(img, fontPtr, x+label->shadowsizex, y+label->shadowsizey, token[t], label->shadowcolor);

	gdImageString(img, fontPtr, x, y, token[t], label->color);

	y += fontPtr->h; /* shift down */
      }

      msFreeCharArray(token, num_tokens);
    } else {
      y -= fontPtr->h;

      if(label->outlinecolor >= 0) {
	gdImageString(img, fontPtr, x-1, y-1, string, label->outlinecolor);
	gdImageString(img, fontPtr, x-1, y+1, string, label->outlinecolor);
	gdImageString(img, fontPtr, x+1, y+1, string, label->outlinecolor);
	gdImageString(img, fontPtr, x+1, y-1, string, label->outlinecolor);
      }

      if(label->shadowcolor >= 0)
	gdImageString(img, fontPtr, x+label->shadowsizex, y+label->shadowsizey, string, label->shadowcolor);

      gdImageString(img, fontPtr, x, y, string, label->color);
    }
  }

  return(0);
}


int msDrawLabelCacheGD(gdImagePtr img, mapObj *map)
{
  pointObj p;
  int i, j, l;

  rectObj r;
  
  labelCacheMemberObj *cachePtr=NULL;
  classObj *classPtr=NULL;
  layerObj *layerPtr=NULL;
  labelObj *labelPtr=NULL;

  int draw_marker;
  int marker_width, marker_height;
  int marker_offset_x, marker_offset_y;
  rectObj marker_rect;

  for(l=map->labelcache.numlabels-1; l>=0; l--) {

    cachePtr = &(map->labelcache.labels[l]); /* point to right spot in cache */

    layerPtr = &(map->layers[cachePtr->layeridx]); /* set a couple of other pointers, avoids nasty references */
    classPtr = &(cachePtr->class);
    labelPtr = &(cachePtr->class.label);

    if(!cachePtr->string)
      continue; /* not an error, just don't want to do anything */

    if(strlen(cachePtr->string) == 0)
      continue; /* not an error, just don't want to do anything */

    if(msGetLabelSize(cachePtr->string, labelPtr, &r, &(map->fontset)) == -1)
      return(-1);

    if(labelPtr->autominfeaturesize && ((r.maxx-r.minx) > cachePtr->featuresize))
      continue; /* label too large relative to the feature */

    draw_marker = marker_offset_x = marker_offset_y = 0; /* assume no marker */
    if((layerPtr->type == MS_LAYER_ANNOTATION || layerPtr->type == MS_LAYER_POINT) && (classPtr->color >= 0 || classPtr->outlinecolor > 0 || classPtr->symbol > 0)) { // there *is* a marker

      msGetMarkerSize(&map->symbolset, classPtr, &marker_width, &marker_height);
      marker_offset_x = MS_NINT(marker_width/2.0);
      marker_offset_y = MS_NINT(marker_height/2.0);      

      marker_rect.minx = MS_NINT(cachePtr->point.x - .5 * marker_width);
      marker_rect.miny = MS_NINT(cachePtr->point.y - .5 * marker_height);
      marker_rect.maxx = marker_rect.minx + (marker_width-1);
      marker_rect.maxy = marker_rect.miny + (marker_height-1);

      if(layerPtr->type == MS_LAYER_ANNOTATION) draw_marker = 1; /* actually draw the marker */
    }

    if(labelPtr->position == MS_AUTO) {

      if(layerPtr->type == MS_LAYER_LINE) {
	int position = MS_UC;

	for(j=0; j<2; j++) { /* Two angles or two positions, depending on angle. Steep angles will use the angle approach, otherwise we'll rotate between UC and LC. */

	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

	  if(j == 1) {
	    if(fabs(cos(labelPtr->angle)) < LINE_VERT_THRESHOLD)
	      labelPtr->angle += 180.0;
	    else
	      position = MS_LC;
	  }

	  p = get_metrics(&(cachePtr->point), position, r, (marker_offset_x + labelPtr->offsetx), (marker_offset_y + labelPtr->offsety), labelPtr->angle, labelPtr->buffer, cachePtr->poly);

	  if(draw_marker)
	    msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon

	  if(!labelPtr->partials) { // check against image first
	    if(labelInImage(img->sx, img->sy, cachePtr->poly, labelPtr->buffer) == MS_FALSE) {
	      cachePtr->status = MS_FALSE;
	      continue; // next angle
	    }
	  }

	  for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	    if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	      if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(!cachePtr->status)
	    continue; // next angle

	  for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
	    if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */

	      if((labelPtr->mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= labelPtr->mindistance)) { /* label is a duplicate */
		cachePtr->status = MS_FALSE;
		break;
	      }

	      if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(cachePtr->status) // found a suitable place for this label
	    break;

	} // next angle

      } else {
	for(j=0; j<=7; j++) { /* loop through the outer label positions */

	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

	  p = get_metrics(&(cachePtr->point), j, r, (marker_offset_x + labelPtr->offsetx), (marker_offset_y + labelPtr->offsety), labelPtr->angle, labelPtr->buffer, cachePtr->poly);

	  if(draw_marker)
	    msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon

	  if(!labelPtr->partials) { // check against image first
	    if(labelInImage(img->sx, img->sy, cachePtr->poly, labelPtr->buffer) == MS_FALSE) {
	      cachePtr->status = MS_FALSE;
	      continue; // next position
	    }
	  }

	  for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	    if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	      if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(!cachePtr->status)
	    continue; // next position

	  for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
	    if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */

	      if((labelPtr->mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= labelPtr->mindistance)) { /* label is a duplicate */
		cachePtr->status = MS_FALSE;
		break;
	      }

	      if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(cachePtr->status) // found a suitable place for this label
	    break;
	} // next position
      }

      if(labelPtr->force) cachePtr->status = MS_TRUE; /* draw in spite of collisions based on last position, need a *best* position */

    } else {

      cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

      if(labelPtr->position == MS_CC) // don't need the marker_offset
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, labelPtr->offsetx, labelPtr->offsety, labelPtr->angle, labelPtr->buffer, cachePtr->poly);
      else
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, (marker_offset_x + labelPtr->offsetx), (marker_offset_y + labelPtr->offsety), labelPtr->angle, labelPtr->buffer, cachePtr->poly);

      if(draw_marker)
	msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon, part of overlap tests */

      if(!labelPtr->force) { // no need to check anything else

	if(!labelPtr->partials) {
	  if(labelInImage(img->sx, img->sy, cachePtr->poly, labelPtr->buffer) == MS_FALSE)
	    cachePtr->status = MS_FALSE;
	}

	if(!cachePtr->status)
	  continue; // next label

	for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	  if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	    if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
	      cachePtr->status = MS_FALSE;
	      break;
	    }
	  }
	}

	if(!cachePtr->status)
	  continue; // next label

	for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered label
	  if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */
	    if((labelPtr->mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string, map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= labelPtr->mindistance)) { /* label is a duplicate */
	      cachePtr->status = MS_FALSE;
	      break;
	    }

	    if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
	      cachePtr->status = MS_FALSE;
	      break;
	    }
	  }
	}
      }
    } /* end position if-then-else */

    /* msImagePolyline(img, cachePtr->poly, 1); */

    if(!cachePtr->status)
      continue; /* next label */

    if(draw_marker) { /* need to draw a marker */
      msDrawMarkerSymbolGD(&map->symbolset, img, &(cachePtr->point), classPtr->symbol, classPtr->color, classPtr->backgroundcolor, classPtr->outlinecolor, classPtr->sizescaled);
      if(classPtr->overlaysymbol >= 0) msDrawMarkerSymbolGD(&map->symbolset, img, &(cachePtr->point), classPtr->overlaysymbol, classPtr->overlaycolor, classPtr->overlaybackgroundcolor, classPtr->overlayoutlinecolor, classPtr->overlaysizescaled);
    }

    if(labelPtr->backgroundcolor >= 0)
      billboard(img, cachePtr->poly, labelPtr);

    draw_textGD(img, p, cachePtr->string, labelPtr, &(map->fontset)); /* actually draw the label */

  } /* next in cache */

  return(0);
}


/* ------------------------------------------------------------------------------- */
/*       Draw a line symbol of the specified size and color                        */
/* ------------------------------------------------------------------------------- */
void msDrawLineSymbolGD(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, int sy, int fc, int bc, double sz)
{
  int i, j;
  symbolObj *symbol;
  int styleDashed[100];
  int x, y;
  int brush_bc, brush_fc;
  gdImagePtr brush=NULL;
  gdPoint points[MS_MAXVECTORPOINTS];

  double scale=1.0;
  
  if(p->numlines <= 0)
    return;

  if(sy > symbolset->numsymbols || sy < 0) // no such symbol, 0 is OK
    return;

  if((fc < 0) || (fc >= gdImageColorsTotal(img))) // invalid color
    return;

  if(sz < 1) // size too small
    return;

  if(sy == 0) { // just draw a single width line
    msImagePolyline(img, p, fc);
    return;
  }

  symbol = &(symbolset->symbol[sy]);

  switch(symbol->type) {
  case(MS_SYMBOL_SIMPLE):
    if(bc == -1) bc = gdTransparent;
    break;
  case(MS_SYMBOL_TRUETYPE):
    msImageTruetypePolyline(img, p, symbol, fc, sz, symbolset->fontset);
    return;
    break;
  case(MS_SYMBOL_CARTOLINE):
    /* Single line */
    if (sz == 1) {
      bc = gdTransparent;
      break;
    } else {
      msImageCartographicPolyline(img, p, fc, sz, symbol->linecap, symbol->linejoin, symbol->linejoinmaxsize);
    }
    return;
    break;
  case(MS_SYMBOL_ELLIPSE):
    bc = gdTransparent;

    scale = (sz)/symbol->sizey;
    x = MS_NINT(symbol->sizex*scale);    
    y = MS_NINT(symbol->sizey*scale);
    
    if((x < 2) && (y < 2)) break;
    
    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, sy, fc, sz)) == NULL) { // not in cache, create
      brush = gdImageCreate(x, y);
      brush_bc = gdImageColorAllocate(brush,gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));    
      gdImageColorTransparent(brush,0);
      brush_fc = gdImageColorAllocate(brush, gdImageRed(img,fc), gdImageGreen(img,fc), gdImageBlue(img,fc));
      
      x = MS_NINT(brush->sx/2); // center the ellipse
      y = MS_NINT(brush->sy/2);
      
      // draw in the brush image
      gdImageArc(brush, x, y, MS_NINT(scale*symbol->points[0].x), MS_NINT(scale*symbol->points[0].y), 0, 360, brush_fc);
      if(symbol->filled)
	gdImageFillToBorder(brush, x, y, brush_fc, brush_fc);
      
      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, sy, fc, sz, brush);
    }

    gdImageSetBrush(img, brush);
    fc = 1; bc = 0;
    break;
  case(MS_SYMBOL_PIXMAP):
    gdImageSetBrush(img, symbol->img);
    fc = 1; bc = 0;
    break;
  case(MS_SYMBOL_VECTOR):
    if(bc == -1) bc = gdTransparent;

    scale = sz/symbol->sizey;
    x = MS_NINT(symbol->sizex*scale);    
    y = MS_NINT(symbol->sizey*scale);

    if((x < 2) && (y < 2)) break;

    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, sy, fc, sz)) == NULL) { // not in cache, create
      brush = gdImageCreate(x, y);
      if(bc >= 0)
	brush_bc = gdImageColorAllocate(brush, gdImageRed(img,bc), gdImageGreen(img,bc), gdImageBlue(img,bc));
      else {
	brush_bc = gdImageColorAllocate(brush, gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));
	gdImageColorTransparent(brush,0);
      }
      brush_fc = gdImageColorAllocate(brush,gdImageRed(img,fc), gdImageGreen(img,fc), gdImageBlue(img,fc));
      
      // draw in the brush image 
      for(i=0;i < symbol->numpoints;i++) {
	points[i].x = MS_NINT(scale*symbol->points[i].x);
	points[i].y = MS_NINT(scale*symbol->points[i].y);
      }
      gdImageFilledPolygon(brush, points, symbol->numpoints, brush_fc);

      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, sy, fc, sz, brush);
    }

    gdImageSetBrush(img, brush);
    fc = 1; bc = 0;
    break;
  }  

  if(symbol->stylelength > 0) {
    int k=0, sc;
   
    sc = fc; // start with foreground color
    for(i=0; i<symbol->stylelength; i++) {      
      for(j=0; j<symbol->style[i]; j++) {
	styleDashed[k] = sc;
	k++;
      } 
      if(sc==fc) sc = bc; else sc = fc;
    }
    gdImageSetStyle(img, styleDashed, k);

    if(!brush && !symbol->img)
      msImagePolyline(img, p, gdStyled);
    else 
      msImagePolyline(img, p, gdStyledBrushed);
  } else {
    if(!brush && !symbol->img)
      msImagePolyline(img, p, fc);
    else
      msImagePolyline(img, p, gdBrushed);
  }

  return;
}


/* ------------------------------------------------------------------------------- */
/*       Draw a shade symbol of the specified size and color                       */
/* ------------------------------------------------------------------------------- */
void msDrawShadeSymbolGD(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, 
                         int sy, int fc, int bc, int oc, double sz)
{
  symbolObj *symbol;
  int i;
  gdPoint oldpnt,newpnt;
  gdPoint sPoints[MS_MAXVECTORPOINTS];
  gdImagePtr tile;
  int x,y;
  int tile_bg=-1, tile_fg=-1; /* colors (background and foreground) */
  
  double scale=1.0;
  
  int bbox[8];
  rectObj rect;
  char *font=NULL;

  if(p->numlines <= 0)
    return;

  if(sy > symbolset->numsymbols || sy < 0) /* no such symbol, 0 is OK */
    return;

  if(!gdImageTrueColor(img) 
     && fc >= gdImageColorsTotal(img)) /* invalid color, -1 is valid */
    return;

  if(sz < 1) /* size too small */
    return;
  
  if(sy == 0) { /* simply draw a single pixel of the specified color */
    if(fc>-1)
      msImageFilledPolygon(img, p, fc);
    if(oc>-1)
      msImagePolyline(img, p, oc);
    return;
  }

  if(fc<0) {
    if(oc>-1)
      msImagePolyline(img, p, oc);
    return;
  }

  symbol = &(symbolset->symbol[sy]);

  switch(symbol->type) {
  case(MS_SYMBOL_TRUETYPE):    
    
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    font = msLookupHashTable(symbolset->fontset->fonts, symbolset->symbol[sy].font);
    if(!font) return;

    if(getCharacterSize(symbol->character, sz, font, &rect) == -1) return;
    x = rect.maxx - rect.minx;
    y = rect.maxy - rect.miny;

    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, bc), gdImageGreen(img, bc), gdImageBlue(img, bc));
    else {
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }    
    tile_fg = gdImageColorAllocate(tile, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc));
    
    x = -rect.minx;
    y = -rect.miny;

#ifdef USE_GD_TTF
    gdImageStringTTF(tile, bbox, ((symbol->antialias)?(tile_fg):-(tile_fg)), font, sz, 0, x, y, symbol->character);
#else
    gdImageStringFT(tile, bbox, ((symbol->antialias)?(tile_fg):-(tile_fg)), font, sz, 0, x, y, symbol->character);
#endif    

    gdImageSetTile(img, tile);
    msImageFilledPolygon(img,p,gdTiled);
    if(oc>-1)
      msImagePolyline(img, p, oc);
    gdImageDestroy(tile);
#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    
    gdImageSetTile(img, symbol->img);
    msImageFilledPolygon(img, p, gdTiled);
    if(oc>-1)
      msImagePolyline(img, p, oc);

    break;
  case(MS_SYMBOL_ELLIPSE):    
   
    scale = sz/symbol->sizey; // sz ~ height in pixels
    x = MS_NINT(symbol->sizex*scale)+1;
    y = MS_NINT(symbol->sizey*scale)+1;

    if((x <= 1) && (y <= 1)) { /* No sense using a tile, just fill solid */
      msImageFilledPolygon(img, p, fc);
      if(oc>-1)
        msImagePolyline(img, p, oc);
      return;
    }

    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, bc), gdImageGreen(img, bc), gdImageBlue(img, bc));
    else {
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }    
    tile_fg = gdImageColorAllocate(tile, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc));
    
    x = MS_NINT(tile->sx/2);
    y = MS_NINT(tile->sy/2);

    /*
    ** draw in the tile image
    */
    gdImageArc(tile, x, y, MS_NINT(scale*symbol->points[0].x), MS_NINT(scale*symbol->points[0].y), 0, 360, tile_fg);
    if(symbol->filled)
      gdImageFillToBorder(tile, x, y, tile_fg, tile_fg);

    /*
    ** Fill the polygon in the main image
    */
    gdImageSetTile(img, tile);
    msImageFilledPolygon(img,p,gdTiled);
    if(oc>-1)
      msImagePolyline(img, p, oc);
    gdImageDestroy(tile);

    break;
  case(MS_SYMBOL_VECTOR):

    if(fc < 0)
      return;

    scale = sz/symbol->sizey; // sz ~ height in pixels
    x = MS_NINT(symbol->sizex*scale)+1;    
    y = MS_NINT(symbol->sizey*scale)+1;

    if((x <= 1) && (y <= 1)) { /* No sense using a tile, just fill solid */
      msImageFilledPolygon(img, p, fc);
      if(oc>-1)
        msImagePolyline(img, p, oc);
      return;
    }

    /*
    ** create tile image
    */
    tile = gdImageCreate(x, y);
    if(bc >= 0)
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, bc), gdImageGreen(img, bc), gdImageBlue(img, bc));
    else {
      tile_bg = gdImageColorAllocate(tile, gdImageRed(img, 0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(tile,0);
    }
    tile_fg = gdImageColorAllocate(tile, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc));
   
    /*
    ** draw in the tile image
    */
    if(symbol->filled) {

      for(i=0;i < symbol->numpoints;i++) {
	sPoints[i].x = MS_NINT(scale*symbol->points[i].x);
	sPoints[i].y = MS_NINT(scale*symbol->points[i].y);
      }
      gdImageFilledPolygon(tile, sPoints, symbol->numpoints, tile_fg);      

    } else  { /* shade is a vector drawing */
      
      oldpnt.x = MS_NINT(scale*symbol->points[0].x); /* convert first point in shade smbol */
      oldpnt.y = MS_NINT(scale*symbol->points[0].y);

      /* step through the shade sy */
      for(i=1;i < symbol->numpoints;i++) {
	if((symbol->points[i].x < 0) && (symbol->points[i].y < 0)) {
	  oldpnt.x = MS_NINT(scale*symbol->points[i].x);
	  oldpnt.y = MS_NINT(scale*symbol->points[i].y);
	} else {
	  if((symbol->points[i-1].x < 0) && (symbol->points[i-1].y < 0)) { /* Last point was PENUP, now a new beginning */
	    oldpnt.x = MS_NINT(scale*symbol->points[i].x);
	    oldpnt.y = MS_NINT(scale*symbol->points[i].y);
	  } else {
	    newpnt.x = MS_NINT(scale*symbol->points[i].x);
	    newpnt.y = MS_NINT(scale*symbol->points[i].y);
	    gdImageLine(tile, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, tile_fg);
	    oldpnt = newpnt;
	  }
	}
      } /* end for loop */
    }

    /*
    ** Fill the polygon in the main image
    */
    gdImageSetTile(img, tile);
    msImageFilledPolygon(img, p, gdTiled);
    if(oc>-1)
      msImagePolyline(img, p, oc);

    break;
  default:
    break;
  }

  return;
}


/*
** Save an image to a file named filename, if filename is NULL it goes to stdout
*/

int msSaveImageGD(gdImagePtr img, char *filename, outputFormatObj *format )

{
    FILE *stream;

#if GD2_VERS > 1
    if( format->imagemode == MS_IMAGEMODE_RGBA )
        gdImageSaveAlpha( img, 1 );
    else if( format->imagemode == MS_IMAGEMODE_RGB )
        gdImageSaveAlpha( img, 0 );
#endif

  if(filename != NULL && strlen(filename) > 0) {
    stream = fopen(filename, "wb");
    if(!stream) {
      msSetError(MS_IOERR, "(%s)", "msSaveImage()", filename);
      return(MS_FAILURE);
    }
  } else { /* use stdout */

#ifdef _WIN32
    /*
    ** Change stdout mode to binary on win32 platforms
    */
    if(_setmode( _fileno(stdout), _O_BINARY) == -1) {
      msSetError(MS_IOERR, "Unable to change stdout to binary mode.", "msSaveImage()");
      return(MS_FAILURE);
    }
#endif
    stream = stdout;
  }

  if( strcasecmp("ON",msGetOutputFormatOption( format, "INTERLACE", "ON" ))
      == 0 )
      gdImageInterlace(img, 1);

  if(format->transparent)
    gdImageColorTransparent(img, 0);

  if( strcasecmp(format->driver,"gd/gif") == 0 )
  {
#ifdef USE_GD_GIF
    gdImageGif(img, stream);
#else
    msSetError(MS_MISCERR, "GIF output is not available.", "msSaveImage()");
    return(MS_FAILURE);
#endif
  }
  else if( strcasecmp(format->driver,"gd/png") == 0 )
  {
#ifdef USE_GD_PNG
      gdImagePng(img, stream);
#else
      msSetError(MS_MISCERR, "PNG output is not available.", "msSaveImage()");
      return(MS_FAILURE);
#endif
  }
  else if( strcasecmp(format->driver,"gd/jpeg") == 0 )
  {
#ifdef USE_GD_JPEG
    gdImageJpeg(img, stream, 
                atoi(msGetOutputFormatOption( format, "QUALITY", "75" )) );
#else
     msSetError(MS_MISCERR, "JPEG output is not available.", "msSaveImage()");
     return(MS_FAILURE);
#endif
  }
  else if( strcasecmp(format->driver,"gd/wbmp") == 0 )
  {
#ifdef USE_GD_WBMP
      gdImageWBMP(img, 1, stream);
#else
      msSetError(MS_MISCERR, "WBMP output is not available.", "msSaveImage()");
      return(MS_FAILURE);
#endif
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


/**
 * Free gdImagePtr
 */
void msFreeImageGD(gdImagePtr img)
{
  gdImageDestroy(img);
}

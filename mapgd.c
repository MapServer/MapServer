/* $Id$ */

#include "map.h"
#include "mapparser.h"
#include "mapthread.h"

#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

static void msFixedImageCopy (gdImagePtr dst, gdImagePtr src, int dstX, int dstY, int srcX, int srcY, int w, int h);

static unsigned char PNGsig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; // 89 50 4E 47 0D 0A 1A 0A hex
static unsigned char JPEGsig[3] = {255, 216, 255}; // FF D8 FF hex

int msImageSetPenGD(gdImagePtr img, colorObj *color) {
  if(MS_VALID_COLOR(*color))
    color->pen = gdImageColorResolve(img, color->red, color->green, color->blue);
  else
    color->pen = -1;

  return(MS_SUCCESS);
}

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

        if( format->imagemode == MS_IMAGEMODE_RGB || format->imagemode == MS_IMAGEMODE_RGBA ) {
            image->img.gd = gdImageCreateTrueColor(width, height);
        } else
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
    else
    {
        msSetError(MS_IMGERR, 
                   "Cannot create GD image of size %d x %d.", 
                   "msImageCreateGD()", width, height );
    }

    return NULL;
}

/**
 * Utility function to initialize the color of an image.  The background
 * color is passed, but the outputFormatObj is consulted to see if the
 * transparency should be set (for RGBA images).   Note this function only
 * affects TrueColor images. 
 */  

void msImageInitGD( imageObj *image, colorObj *background )
{
    if( image->format->imagemode == MS_IMAGEMODE_PC256 ) {
        gdImageColorAllocate(image->img.gd, background->red, background->green, background->blue);
        return;
    }

    {
        int		pen, pixels, line;
        int             *tpixels;

        if( image->format->imagemode == MS_IMAGEMODE_RGBA )
            pen = gdTrueColorAlpha( background->red, 
                                    background->green, 
                                    background->blue,
                                    image->format->transparent ? 127 : 0 );
        else
            pen = gdTrueColor( background->red, 
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
  image->format->refcount++;

  if( image->format == NULL )
  {
    msSetError(MS_GDERR, "Unable to create default OUTPUTFORMAT definition for driver '%s'.", "msImageLoadGD()", driver );
    return(NULL);
  }

  return image;
}

static gdImagePtr createBrush(gdImagePtr img, int width, int height, styleObj *style, int *fgcolor, int *bgcolor)
{
  gdImagePtr brush;

  if(!gdImageTrueColor(img)) {
    brush = gdImageCreate(width, height);
    if(style->backgroundcolor.pen >= 0)
      *bgcolor = gdImageColorAllocate(brush, style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);
    else {
      *bgcolor = gdImageColorAllocate(brush, gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));          
      gdImageColorTransparent(brush,0);
    }
    if(style->color.pen >= 0)
      *fgcolor = gdImageColorAllocate(brush, style->color.red, style->color.green, style->color.blue);
    else // try outline color
      *fgcolor = gdImageColorAllocate(brush, style->outlinecolor.red, style->outlinecolor.green, style->outlinecolor.blue); 
  } else {
    brush = gdImageCreateTrueColor(width, height);
    gdImageAlphaBlending(brush, 0);
    if(style->backgroundcolor.pen >= 0)
      *bgcolor = gdTrueColor(style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);      
    else
      *bgcolor = -1;      
    gdImageFilledRectangle(brush, 0, 0, width, height, *bgcolor);
    if(style->color.pen >= 0)
      *fgcolor = gdTrueColor(style->color.red, style->color.green, style->color.blue);
    else // try outline color
      *fgcolor = gdTrueColor(style->outlinecolor.red, style->outlinecolor.green, style->outlinecolor.blue);
  }

  return(brush);
}

static void imageOffsetPolyline(gdImagePtr img, shapeObj *p, int color, int offsetx, int offsety)
{
  int i, j, first;
  int dx, dy, dx0, dy0,  ox, oy, limit;
  double x, y, x0, y0, k, k0, q, q0;
  float par=0.71;

  if(offsety == -99) {
    limit = offsetx*offsetx/4;
    for (i = 0; i < p->numlines; i++) {
      first = 1;
      for(j=1; j<p->line[i].numpoints; j++) {        
        ox=0; oy=0;
        dx = p->line[i].point[j].x - p->line[i].point[j-1].x;
        dy = p->line[i].point[j].y - p->line[i].point[j-1].y;
        //offset setting - quick approximation, may be changed with goniometric functions
        if(dx==0) { //vertical line
          if(dy==0) continue; //checking unique points
          ox=(dy>0) ? -offsetx : offsetx;
        } else {
          k = (double)dy/(double)dx;
          if(MS_ABS(k)<0.5) {
            oy = (dx>0) ? offsetx : -offsetx;
          } else {
            if (MS_ABS(k)<2.1) {
              oy = (dx>0) ? offsetx*par : -offsetx*par;
              ox = (dy>0) ? -offsetx*par : offsetx*par;
            } else
              ox = (dy>0) ? -offsetx : offsetx;
          }
          q = p->line[i].point[j-1].y+oy - k*(p->line[i].point[j-1].x+ox);
        }
        //offset line points computation
        if(first==1) { // first point
          first = 0;
          x = p->line[i].point[j-1].x+ox;
          y = p->line[i].point[j-1].y+oy;
        } else { //middle points
          if((dx*dx+dy*dy)>limit){ //if the points are too close
            if(dx0==0) { //checking verticals
              if(dx==0) continue;
              x = x0;
              y = k*x + q;
            } else {
              if(dx==0) {
                x = p->line[i].point[j-1].x+ox;
                y = k0*x + q0;
              } else {
                if(k==k0) continue; //checking equal direction
                x = (q-q0)/(k0-k);
                y = k*x+q;
              }
            }
          }else{//need to be refined
            x = p->line[i].point[j-1].x+ox;
            y = p->line[i].point[j-1].y+oy;
          }
          gdImageLine(img, (int)x0, (int)y0, (int)x, (int)y, color);
        }
        dx0 = dx; dy0 = dy; x0 = x, y0 = y; k0 = k; q0=q;
      }
      //last point
      if(first==0)gdImageLine(img, (int)x0, (int)y0, p->line[i].point[p->line[i].numpoints-1].x+ox, p->line[i].point[p->line[i].numpoints-1].y+oy, color);
    }
  } else { // normal offset (eg. drop shadow)
    for (i = 0; i < p->numlines; i++)
      for(j=1; j<p->line[i].numpoints; j++)
        gdImageLine(img, (int)p->line[i].point[j-1].x+offsetx, (int)p->line[i].point[j-1].y+offsety, (int)p->line[i].point[j].x+offsetx, (int)p->line[i].point[j].y+offsety, color);
  }
}

static void imagePolyline(gdImagePtr img, shapeObj *p, int color, int offsetx, int offsety)
{
  int i, j;
  
  if(offsetx != 0 || offsetx != 0) 
    imageOffsetPolyline(img, p, color, offsetx, offsety);
  else {
    for (i = 0; i < p->numlines; i++)
      for(j=1; j<p->line[i].numpoints; j++)
        gdImageLine(img, (int)p->line[i].point[j-1].x, (int)p->line[i].point[j-1].y, (int)p->line[i].point[j].x, (int)p->line[i].point[j].y, color);
  }
}

/* ---------------------------------------------------------------------------*/
/*       Stroke an ellipse with a line symbol of the specified size and color */
/* ---------------------------------------------------------------------------*/
void msCircleDrawLineSymbolGD(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, styleObj *style, double scalefactor)
{
  int i, j;
  symbolObj *symbol;
  int styleDashed[100];
  int x, y, ox, oy;
  int bc, fc;
  int brush_bc, brush_fc;
  double size, d;
  gdImagePtr brush=NULL;
  gdPoint points[MS_MAXVECTORPOINTS];
  
  if(!p) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->outlinecolor));
  
  symbol = &(symbolset->symbol[style->symbol]);
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
    if(bc == -1) bc = gdTransparent;
    break;
  case(MS_SYMBOL_TRUETYPE):
    // msImageTruetypePolyline(img, p, symbol, fc, size, symbolset->fontset);
    return;
    break;
  case(MS_SYMBOL_CARTOLINE):
    return;
    break;
  case(MS_SYMBOL_ELLIPSE):   
    bc = gdTransparent;

    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);
   
    if((x < 2) && (y < 2)) break;
    
    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style->symbol, fc, size)) == NULL) { 
      brush = createBrush(img, x, y, style, &brush_fc, &brush_bc); // not in cache, create

      x = MS_NINT(brush->sx/2); // center the ellipse
      y = MS_NINT(brush->sy/2);
      
      // draw in the brush image
      gdImageArc(brush, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, brush_fc);
      if(symbol->filled)
	gdImageFillToBorder(brush, x, y, brush_fc, brush_fc);
      
      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, style->symbol, fc, size, brush);
    }

    gdImageSetBrush(img, brush);
    fc=1; bc=0;
    break;
  case(MS_SYMBOL_PIXMAP):
    gdImageSetBrush(img, symbol->img);
    fc=1; bc=0;
    break;
  case(MS_SYMBOL_VECTOR):
    if(bc == -1) bc = gdTransparent;

    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);

    if((x < 2) && (y < 2)) break;

    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style->symbol, fc, size)) == NULL) {
      brush = createBrush(img, x, y, style, &brush_fc, &brush_bc);  // not in cache, create

      // draw in the brush image 
      for(i=0;i < symbol->numpoints;i++) {
	points[i].x = MS_NINT(d*symbol->points[i].x);
	points[i].y = MS_NINT(d*symbol->points[i].y);
      }
      gdImageFilledPolygon(brush, points, symbol->numpoints, brush_fc);

      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, style->symbol, fc, size, brush);
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

  return;
}

/* ------------------------------------------------------------------------------- */
/*       Fill a circle with a shade symbol of the specified size and color       */
/* ------------------------------------------------------------------------------- */
void msCircleDrawShadeSymbolGD(symbolSetObj *symbolset, gdImagePtr img, 
                               pointObj *p, double r, styleObj *style, double scalefactor)
{
  symbolObj *symbol;
  int i;
  gdPoint oldpnt,newpnt;
  gdPoint sPoints[MS_MAXVECTORPOINTS];
  gdImagePtr tile=NULL;
  int x, y, ox, oy;

  int fc, bc, oc;
  int tile_bc=-1, tile_fc=-1; /* colors (background and foreground) */
  double size, d;  

  int bbox[8];
  rectObj rect;
  char *font=NULL;

  if(!p) return;

  if(!MS_VALID_COLOR(style->color) && MS_VALID_COLOR(style->outlinecolor)) { // use msDrawLineSymbolGD() instead (POLYLINE)
    msCircleDrawLineSymbolGD(symbolset, img, p, r, style, scalefactor);
    return;
  }

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->outlinecolor));

  symbol = &(symbolset->symbol[style->symbol]);
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  oc = style->outlinecolor.pen;
  ox = style->offsetx; // TODO: add scaling?
  oy = style->offsety;
  size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

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
    
#ifdef USE_GD_FT
    font = msLookupHashTable(symbolset->fontset->fonts, symbol->font);
    if(!font) return;

    if(msGetCharacterSize(symbol->character, size, font, &rect) != MS_SUCCESS) return;
    x = rect.maxx - rect.minx;
    y = rect.maxy - rect.miny;

    tile = createBrush(img, x, y, style, &tile_fc, &tile_bc); // create the tile image

    x = -rect.minx; // center the glyph
    y = -rect.miny;

    gdImageStringFT(tile, bbox, ((symbol->antialias)?(tile_fc):-(tile_fc)), font, size, 0, x, y, symbol->character);
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

    tile = createBrush(img, x, y, style, &tile_fc, &tile_bc); // create tile image
    
    x = MS_NINT(tile->sx/2); // center the ellipse
    y = MS_NINT(tile->sy/2);
    
    // draw in the tile image
    gdImageArc(tile, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, tile_fc);
    if(symbol->filled)
      gdImageFillToBorder(tile, x, y, tile_fc, tile_fc);

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

    tile = createBrush(img, x, y, style, &tile_fc, &tile_bc); // create tile image

    // draw in the tile image
    if(symbol->filled) {

      for(i=0;i < symbol->numpoints;i++) {
	sPoints[i].x = MS_NINT(d*symbol->points[i].x);
	sPoints[i].y = MS_NINT(d*symbol->points[i].y);
      }
      gdImageFilledPolygon(tile, sPoints, symbol->numpoints, tile_fc);      

    } else  { /* shade is a vector drawing */
      
      oldpnt.x = MS_NINT(d*symbol->points[0].x); /* convert first point in shade smbol */
      oldpnt.y = MS_NINT(d*symbol->points[0].y);

      /* step through the shade symbol */
      for(i=1;i < symbol->numpoints;i++) {
	if((symbol->points[i].x < 0) && (symbol->points[i].y < 0)) {
	  oldpnt.x = MS_NINT(d*symbol->points[i].x);
	  oldpnt.y = MS_NINT(d*symbol->points[i].y);
	} else {
	  if((symbol->points[i-1].x < 0) && (symbol->points[i-1].y < 0)) { /* Last point was PENUP, now a new beginning */
	    oldpnt.x = MS_NINT(d*symbol->points[i].x);
	    oldpnt.y = MS_NINT(d*symbol->points[i].y);
	  } else {
	    newpnt.x = MS_NINT(d*symbol->points[i].x);
	    newpnt.y = MS_NINT(d*symbol->points[i].y);
	    gdImageLine(tile, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, tile_fc);
	    oldpnt = newpnt;
	  }
	}
      } /* end for loop */
    }

    gdImageSetTile(img, tile);
 
    break;
  default:
    break;
  }

  // fill the circle in the main image
  msImageFilledCircle(img, p, r, gdTiled);
  if(oc>-1) gdImageArc(img, (int)p->x, (int)p->y, (int)(2*r), (int)(2*r), 0, 360, oc);
  if(tile) gdImageDestroy(tile);

  return;
}


/* ------------------------------------------------------------------------------- */
/*       Draw a single marker symbol of the specified size and color               */
/* ------------------------------------------------------------------------------- */
void msDrawMarkerSymbolGD(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, styleObj *style, double scalefactor)
{
  symbolObj *symbol;
  int offset_x, offset_y, x, y;
  int j;
  gdPoint oldpnt,newpnt;
  gdPoint mPoints[MS_MAXVECTORPOINTS];
  char *error=NULL;

  gdImagePtr tmp;
  int tmp_fc=-1, tmp_bc, tmp_oc=-1;
  int fc, bc, oc;
  double size,d;

  int ox, oy;
  
  int bbox[8];
  rectObj rect;
  char *font=NULL;

  if(!p) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->outlinecolor));

  symbol = &(symbolset->symbol[style->symbol]);
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  oc = style->outlinecolor.pen;
  ox = style->offsetx; // TODO: add scaling?
  oy = style->offsety;
  size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; // no such symbol, 0 is OK
  if(fc<0 && oc<0 && symbol->type != MS_SYMBOL_PIXMAP) return; // nothing to do (color not required for a pixmap symbol)
  if(size < 1) return; // size too small

  if(style->symbol == 0 && fc >= 0) { /* simply draw a single pixel of the specified color */
    gdImageSetPixel(img, p->x + ox, p->y + oy, fc);
    return;
  }  

  switch(symbol->type) {  
  case(MS_SYMBOL_TRUETYPE): // TODO: Need to leverage the image cache!

#ifdef USE_GD_FT
    font = msLookupHashTable(symbolset->fontset->fonts, symbol->font);
    if(!font) return;

    if(msGetCharacterSize(symbol->character, size, font, &rect) != MS_SUCCESS) return;

    x = p->x + ox - (rect.maxx - rect.minx)/2 - rect.minx;
    y = p->y + oy - rect.maxy + (rect.maxy - rect.miny)/2;  

    // ============== MOD BY DHC MAR 14, 2003 -- Can we get outline color (and rotation) for truetype symbols?
    if( oc >= 0 ) {
      error = gdImageStringFT(img, bbox, ((symbol->antialias)?(oc):-(oc)), font, size, 0, x-1, y-1, symbol->character);
      if(error) {
	msSetError(MS_TTFERR, error, "msDrawMarkerSymbolGD()");
	return;
      }

      gdImageStringFT(img, bbox, ((symbol->antialias)?(oc):-(oc)), font, size, 0, x-1, y+1, symbol->character);
      gdImageStringFT(img, bbox, ((symbol->antialias)?(oc):-(oc)), font, size, 0, x+1, y+1, symbol->character);
      gdImageStringFT(img, bbox, ((symbol->antialias)?(oc):-(oc)), font, size, 0, x+1, y-1, symbol->character);
    }
    // END OF DHC MOD

    gdImageStringFT(img, bbox, ((symbol->antialias)?(fc):-(fc)), font, size, 0, x, y, symbol->character);
#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    if(size == 1) { // don't scale
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
  case(MS_SYMBOL_ELLIPSE): // TODO: Need to leverage the image cache!
    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d)+1;
    y = MS_NINT(symbol->sizey*d)+1;

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

    /* paste the tmp image in the main image */
    offset_x = MS_NINT(p->x - .5*tmp->sx + ox);
    offset_y = MS_NINT(p->y - .5*tmp->sx + oy);
    msFixedImageCopy(img, tmp, offset_x, offset_y, 0, 0, tmp->sx, tmp->sy);

    gdImageDestroy(tmp);

    break;
  case(MS_SYMBOL_VECTOR): // TODO: Need to leverage the image cache!
    d = size/symbol->sizey;
    offset_x = MS_NINT(p->x - d*.5*symbol->sizex + ox);
    offset_y = MS_NINT(p->y - d*.5*symbol->sizey + oy);
    
    if(symbol->filled) { /* if filled */
      for(j=0;j < symbol->numpoints;j++) {
	mPoints[j].x = MS_NINT(d*symbol->points[j].x + offset_x);
	mPoints[j].y = MS_NINT(d*symbol->points[j].y + offset_y);
      }
      if(fc >= 0)
	gdImageFilledPolygon(img, mPoints, symbol->numpoints, fc);
      if(oc >= 0)
	gdImagePolygon(img, mPoints, symbol->numpoints, oc);
      
    } else  { /* NOT filled */     

      if(fc < 0) return;
      
      oldpnt.x = MS_NINT(d*symbol->points[0].x + offset_x); /* convert first point in marker s */
      oldpnt.y = MS_NINT(d*symbol->points[0].y + offset_y);
      
      for(j=1;j < symbol->numpoints;j++) { /* step through the marker s */
	if((symbol->points[j].x < 0) && (symbol->points[j].x < 0)) {
	  oldpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
	  oldpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
	} else {
	  if((symbol->points[j-1].x < 0) && (symbol->points[j-1].y < 0)) { /* Last point was PENUP, now a new beginning */
	    oldpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
	    oldpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
	  } else {
	    newpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
	    newpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
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

/* ------------------------------------------------------------------------------- */
/*       Draw a line symbol of the specified size and color                        */
/* ------------------------------------------------------------------------------- */
void msDrawLineSymbolGD(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor)
{
  int i, j;
  symbolObj *symbol;
  int styleDashed[100];
  int x, y;
  int ox, oy;
  int fc, bc;
  int brush_bc, brush_fc;
  double size, d;
  gdImagePtr brush=NULL;
  gdPoint points[MS_MAXVECTORPOINTS];

  if(!p) return;
  if(p->numlines <= 0) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->backgroundcolor));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->outlinecolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->color));

  symbol = &(symbolset->symbol[style->symbol]);
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  if(fc==-1) fc = style->outlinecolor.pen;
  if(style->size*scalefactor > style->maxsize) scalefactor = (float)style->maxsize/(float)style->size;
  if(style->size*scalefactor < style->minsize) scalefactor = (float)style->minsize/(float)style->size;
  size = MS_NINT(style->size*scalefactor);
  //size = MS_MAX(size, style->minsize);
  //size = MS_MIN(size, style->maxsize);

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; // no such symbol, 0 is OK
  if(fc < 0 && symbol->type != MS_SYMBOL_PIXMAP) return; // nothing to do (color not required for a pixmap symbol)
  if(size < 1) return; // size too small
  ox = MS_NINT(style->offsetx*scalefactor);
  oy = (style->offsety == -99) ? -99 : style->offsety*scalefactor;

  if(style->symbol == 0) { // just draw a single width line
    imagePolyline(img, p, fc, ox, oy);
    return;
  }

  switch(symbol->type) {
  case(MS_SYMBOL_SIMPLE):
    if(bc == -1) bc = gdTransparent;
    break;
  case(MS_SYMBOL_TRUETYPE):
    msImageTruetypePolyline(symbolset, img, p, style, scalefactor);
    return;
    break;
  case(MS_SYMBOL_CARTOLINE):
    msImageCartographicPolyline(img, p, style, symbol, fc, size, scalefactor);
    return;
    break;
  case(MS_SYMBOL_ELLIPSE):
    bc = gdTransparent;

    d = (size)/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);
    
    if((x < 2) && (y < 2)) break;
    
    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style->symbol, fc, size)) == NULL) { 
      brush = createBrush(img, x, y, style, &brush_fc, &brush_bc); // not in cache, create it

      x = MS_NINT(brush->sx/2); // center the ellipse
      y = MS_NINT(brush->sy/2);
      
      // draw in the brush image
      gdImageArc(brush, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, brush_fc);
      if(symbol->filled)
        gdImageFillToBorder(brush, x, y, brush_fc, brush_fc);
      
      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, style->symbol, fc, size, brush);
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

    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);

    if((x < 2) && (y < 2)) break;

    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style->symbol, fc, size)) == NULL) { 
      brush = createBrush(img, x, y, style, &brush_fc, &brush_bc); // not in cache, create it

      // draw in the brush image 
      for(i=0;i < symbol->numpoints;i++) {
        points[i].x = MS_NINT(d*symbol->points[i].x);
        points[i].y = MS_NINT(d*symbol->points[i].y);
      }
      gdImageFilledPolygon(brush, points, symbol->numpoints, brush_fc);

      symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, style->symbol, fc, size, brush);
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
      imagePolyline(img, p, gdStyled, ox, oy);
    else 
      imagePolyline(img, p, gdStyledBrushed, ox, oy);
  } else {
    if(!brush && !symbol->img) {
      if(style->antialias==MS_TRUE) {       
        gdImageSetAntiAliased(img, fc);
        imagePolyline(img, p, gdAntiAliased, ox, oy);
      } else
        imagePolyline(img, p, fc, ox, oy);
    } else
      imagePolyline(img, p, gdBrushed, ox, oy);
  }

  return;
}

/* ------------------------------------------------------------------------------- */
/*       Draw a shade symbol of the specified size and color                       */
/* ------------------------------------------------------------------------------- */
void msDrawShadeSymbolGD(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor)
{
  symbolObj *symbol;
  int i;
  gdPoint oldpnt, newpnt;
  gdPoint sPoints[MS_MAXVECTORPOINTS];
  gdImagePtr tile;
  int x, y;
  int tile_bc=-1, tile_fc=-1; /* colors (background and foreground) */
  int fc, bc, oc;
  double size, d;
  
  int bbox[8];
  rectObj rect;
  char *font=NULL;

  if(!p) return;
  if(p->numlines <= 0) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->outlinecolor));

  symbol = &(symbolset->symbol[style->symbol]);
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  oc = style->outlinecolor.pen;
  size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  if(fc==-1 && oc!=-1) { // use msDrawLineSymbolGD() instead (POLYLINE)
    msDrawLineSymbolGD(symbolset, img, p, style, scalefactor);
    return;
  }

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; // no such symbol, 0 is OK  
  if(fc < 0) return; // nothing to do
  if(size < 1) return; // size too small
      
  if(style->symbol == 0) { /* simply draw a single pixel of the specified color */    
    if(style->antialias==MS_TRUE) {      
      msImageFilledPolygon(img, p, fc); // fill is NOT anti-aliased, the outline IS
      if(oc>-1)
        gdImageSetAntiAliased(img, oc);
      else
	gdImageSetAntiAliased(img, fc);
      imagePolyline(img, p, gdAntiAliased, style->offsetx, style->offsety);
    } else {
      msImageFilledPolygon(img, p, fc);
      if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);
    }   
    return;
  }
  
  switch(symbol->type) {
  case(MS_SYMBOL_TRUETYPE):    
    
#ifdef USE_GD_FT
    font = msLookupHashTable(symbolset->fontset->fonts, symbol->font);
    if(!font) return;

    if(msGetCharacterSize(symbol->character, size, font, &rect) != MS_SUCCESS) return;
    x = rect.maxx - rect.minx;
    y = rect.maxy - rect.miny;
    
    // create tile image
    tile = createBrush(img, x, y, style, &tile_fc, &tile_bc);

    // center the glyph
    x = -rect.minx;
    y = -rect.miny;

    gdImageStringFT(tile, bbox, ((symbol->antialias)?(tile_fc):-(tile_fc)), font, size, 0, x, y, symbol->character);

    gdImageSetTile(img, tile);
    msImageFilledPolygon(img,p,gdTiled);

    if(style->antialias==MS_TRUE && oc>-1) { 
      gdImageSetAntiAliased(img, oc);     
      imagePolyline(img, p, gdAntiAliased, style->offsetx, style->offsety);
    } else 
      if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);

    gdImageDestroy(tile);
#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    
    gdImageSetTile(img, symbol->img);
    msImageFilledPolygon(img, p, gdTiled);

    if(style->antialias==MS_TRUE && oc>-1) { 
      gdImageSetAntiAliased(img, oc);     
      imagePolyline(img, p, gdAntiAliased, style->offsetx, style->offsety);
    } else 
      if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);

    break;
  case(MS_SYMBOL_ELLIPSE):    
    d = size/symbol->sizey; // size ~ height in pixels
    x = MS_NINT(symbol->sizex*d)+1;
    y = MS_NINT(symbol->sizey*d)+1;

    if((x <= 1) && (y <= 1)) { /* No sense using a tile, just fill solid */
      if(style->antialias==MS_TRUE) {      
        msImageFilledPolygon(img, p, fc); // fill is NOT anti-aliased, the outline IS
        if(oc>-1)
          gdImageSetAntiAliased(img, oc);
        else
	  gdImageSetAntiAliased(img, fc);
        imagePolyline(img, p, gdAntiAliased, style->offsetx, style->offsety);
      } else {
        msImageFilledPolygon(img, p, fc);
        if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);
      }
      return;
    }
    
    // create tile image
    tile = createBrush(img, x, y, style, &tile_fc, &tile_bc);

    // center ellipse
    x = MS_NINT(tile->sx/2);
    y = MS_NINT(tile->sy/2);

    // draw in tile image
    gdImageArc(tile, x, y, MS_NINT(d*symbol->points[0].x), MS_NINT(d*symbol->points[0].y), 0, 360, tile_fc);
    if(symbol->filled)
      gdImageFillToBorder(tile, x, y, tile_fc, tile_fc);

    // fill the polygon in the main image
    gdImageSetTile(img, tile);
    msImageFilledPolygon(img,p,gdTiled);

    if(style->antialias==MS_TRUE && oc>-1) { 
      gdImageSetAntiAliased(img, oc);     
      imagePolyline(img, p, gdAntiAliased, style->offsetx, style->offsety);
    } else 
      if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);

    gdImageDestroy(tile);
    break;
  case(MS_SYMBOL_VECTOR):
    d = size/symbol->sizey; // size ~ height in pixels
    x = MS_NINT(symbol->sizex*d)+1;    
    y = MS_NINT(symbol->sizey*d)+1;

    if((x <= 1) && (y <= 1)) { /* No sense using a tile, just fill solid */
      if(style->antialias==MS_TRUE) {      
        msImageFilledPolygon(img, p, fc); // fill is NOT anti-aliased, the outline IS
        if(oc>-1)
          gdImageSetAntiAliased(img, oc);
        else
	  gdImageSetAntiAliased(img, fc);
        imagePolyline(img, p, gdAntiAliased, style->offsetx, style->offsety);
      } else {
        msImageFilledPolygon(img, p, fc);
        if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);
      }      
      return;
    }

    // create tile image
    tile = createBrush(img, x, y, style, &tile_fc, &tile_bc);
   
    // draw in the tile image    
    if(symbol->filled) {

      for(i=0;i < symbol->numpoints;i++) {
	sPoints[i].x = MS_NINT(d*symbol->points[i].x);
	sPoints[i].y = MS_NINT(d*symbol->points[i].y);
      }
      gdImageFilledPolygon(tile, sPoints, symbol->numpoints, tile_fc);      

    } else  { /* shade is a vector drawing */
      
      oldpnt.x = MS_NINT(d*symbol->points[0].x); /* convert first point in shade smbol */
      oldpnt.y = MS_NINT(d*symbol->points[0].y);

      /* step through the shade sy */
      for(i=1;i < symbol->numpoints;i++) {
	if((symbol->points[i].x < 0) && (symbol->points[i].y < 0)) {
	  oldpnt.x = MS_NINT(d*symbol->points[i].x);
	  oldpnt.y = MS_NINT(d*symbol->points[i].y);
	} else {
	  if((symbol->points[i-1].x < 0) && (symbol->points[i-1].y < 0)) { /* Last point was PENUP, now a new beginning */
	    oldpnt.x = MS_NINT(d*symbol->points[i].x);
	    oldpnt.y = MS_NINT(d*symbol->points[i].y);
	  } else {
	    newpnt.x = MS_NINT(d*symbol->points[i].x);
	    newpnt.y = MS_NINT(d*symbol->points[i].y);
	    gdImageLine(tile, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, tile_fc);
	    oldpnt = newpnt;
	  }
	}
      } /* end for loop */
    }

    // Fill the polygon in the main image
    gdImageSetTile(img, tile);
    msImageFilledPolygon(img, p, gdTiled);

    if(style->antialias==MS_TRUE && oc>-1) { 
      gdImageSetAntiAliased(img, oc);     
      imagePolyline(img, p, gdAntiAliased, style->offsetx, style->offsety);
    } else
      if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);

    gdImageDestroy(tile);
    break;
  default:
    break;
  }

  return;
}

static void billboardGD(gdImagePtr img, shapeObj *shape, labelObj *label)
{
  int i;
  shapeObj temp;

  msInitShape(&temp);
  msAddLine(&temp, &shape->line[0]);

  if(label->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(label->backgroundcolor));
  if(label->backgroundshadowcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(label->backgroundshadowcolor));

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
}

/*
** Simply draws a label based on the label point and the supplied label object.
*/
int msDrawTextGD(gdImagePtr img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor)
{
  int x, y;

  if(!string) return(0); /* not errors, just don't want to do anything */
  if(strlen(string) == 0) return(0);

  x = MS_NINT(labelPnt.x);
  y = MS_NINT(labelPnt.y);

  if(label->color.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(label->color));
  if(label->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(label->outlinecolor));
  if(label->shadowcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(label->shadowcolor));

  if(label->type == MS_TRUETYPE) {
    char *error=NULL, *font=NULL;
    int bbox[8];
    double angle_radians = MS_DEG_TO_RAD*label->angle;
    double size;

    size = label->size*scalefactor;
    size = MS_MAX(size, label->minsize);
    size = MS_MIN(size, label->maxsize);

#ifdef USE_GD_FT
    if(!fontset) {
      msSetError(MS_TTFERR, "No fontset defined.", "msDrawTextGD()");
      return(-1);
    }

    if(!label->font) {
      msSetError(MS_TTFERR, "No Trueype font defined.", "msDrawTextGD()");
      return(-1);
    }

    font = msLookupHashTable(fontset->fonts, label->font);
    if(!font) {
       msSetError(MS_TTFERR, "Requested font (%s) not found.", "msDrawTextGD()",
                  label->font);
      return(-1);
    }

    if(label->outlinecolor.pen >= 0) { /* handle the outline color */
      error = gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x-1, y-1, string);
      if(error) {
	msSetError(MS_TTFERR, error, "msDrawTextGD()");
	return(-1);
      }

      gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x-1, y+1, string);
      gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x+1, y+1, string);
      gdImageStringFT(img, bbox, ((label->antialias)?(label->outlinecolor.pen):-(label->outlinecolor.pen)), font, size, angle_radians, x+1, y-1, string);
    }

    if(label->shadowcolor.pen >= 0) { /* handle the shadow color */
      error = gdImageStringFT(img, bbox, ((label->antialias)?(label->shadowcolor.pen):-(label->shadowcolor.pen)), font, size, angle_radians, x+label->shadowsizex, y+label->shadowsizey, string);
      if(error) {
	msSetError(MS_TTFERR, error, "msDrawTextGD()");
	return(-1);
      }
    }

    gdImageStringFT(img, bbox, ((label->antialias)?(label->color.pen):-(label->color.pen)), font, size, angle_radians, x, y, string);
#else
    msSetError(MS_TTFERR, "TrueType font support is not available.", "msDrawTextGD()");
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
	if(label->outlinecolor.pen >= 0) {
	  gdImageString(img, fontPtr, x-1, y-1, token[t], label->outlinecolor.pen);
	  gdImageString(img, fontPtr, x-1, y+1, token[t], label->outlinecolor.pen);
	  gdImageString(img, fontPtr, x+1, y+1, token[t], label->outlinecolor.pen);
	  gdImageString(img, fontPtr, x+1, y-1, token[t], label->outlinecolor.pen);
	}

	if(label->shadowcolor.pen >= 0)
	  gdImageString(img, fontPtr, x+label->shadowsizex, y+label->shadowsizey, token[t], label->shadowcolor.pen);

	gdImageString(img, fontPtr, x, y, token[t], label->color.pen);

	y += fontPtr->h; /* shift down */
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

  return(0);
}

// TO DO: fix returned values to be MS_SUCCESS/MS_FAILURE
int msDrawLabelCacheGD(gdImagePtr img, mapObj *map)
{
  pointObj p;
  int i, j, l;

  rectObj r;
  
  labelCacheMemberObj *cachePtr=NULL;
  layerObj *layerPtr=NULL;
  labelObj *labelPtr=NULL;

  int marker_width, marker_height;
  int marker_offset_x, marker_offset_y;
  rectObj marker_rect;

  for(l=map->labelcache.numlabels-1; l>=0; l--) {

    cachePtr = &(map->labelcache.labels[l]); // point to right spot in the label cache

    layerPtr = &(map->layers[cachePtr->layerindex]); // set a couple of other pointers, avoids nasty references
    labelPtr = &(cachePtr->label);

    if(!cachePtr->string || strlen(cachePtr->string) == 0)
      continue; // not an error, just don't want to do anything

    if(msGetLabelSize(cachePtr->string, labelPtr, &r, &(map->fontset), layerPtr->scalefactor) == -1)
      return(-1);

    if(labelPtr->autominfeaturesize && ((r.maxx-r.minx) > cachePtr->featuresize))
      continue; /* label too large relative to the feature */

    marker_offset_x = marker_offset_y = 0; /* assume no marker */
    if((layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) || layerPtr->type == MS_LAYER_POINT) { // there *is* a marker      

      // TO DO: at the moment only checks the bottom style, perhaps should check all of them
      if(msGetMarkerSize(&map->symbolset, &(cachePtr->styles[0]), &marker_width, &marker_height, layerPtr->scalefactor) != MS_SUCCESS)
	return(-1);

      marker_offset_x = MS_NINT(marker_width/2.0);
      marker_offset_y = MS_NINT(marker_height/2.0);      

      marker_rect.minx = MS_NINT(cachePtr->point.x - .5 * marker_width);
      marker_rect.miny = MS_NINT(cachePtr->point.y - .5 * marker_height);
      marker_rect.maxx = marker_rect.minx + (marker_width-1);
      marker_rect.maxy = marker_rect.miny + (marker_height-1); 
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

	  if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
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

	      if((labelPtr->mindistance != -1) && (cachePtr->classindex == map->labelcache.labels[i].classindex) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (msDistancePointToPoint(&(cachePtr->point), &(map->labelcache.labels[i].point)) <= labelPtr->mindistance)) { /* label is a duplicate */
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

	  if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
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

	      if((labelPtr->mindistance != -1) && (cachePtr->classindex == map->labelcache.labels[i].classindex) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (msDistancePointToPoint(&(cachePtr->point), &(map->labelcache.labels[i].point)) <= labelPtr->mindistance)) { /* label is a duplicate */
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

      if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
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
	    if((labelPtr->mindistance != -1) && (cachePtr->classindex == map->labelcache.labels[i].classindex) && (strcmp(cachePtr->string, map->labelcache.labels[i].string) == 0) && (msDistancePointToPoint(&(cachePtr->point), &(map->labelcache.labels[i].point)) <= labelPtr->mindistance)) { /* label is a duplicate */
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

    /* imagePolyline(img, cachePtr->poly, 1, 0, 0); */

    if(!cachePtr->status)
      continue; /* next label */

    if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) { /* need to draw a marker */
      for(i=0; i<cachePtr->numstyles; i++)
        msDrawMarkerSymbolGD(&map->symbolset, img, &(cachePtr->point), &(cachePtr->styles[i]), layerPtr->scalefactor);
    }

    if(MS_VALID_COLOR(labelPtr->backgroundcolor)) billboardGD(img, cachePtr->poly, labelPtr);
    msDrawTextGD(img, p, cachePtr->string, labelPtr, &(map->fontset), layerPtr->scalefactor); // actually draw the label

  } // next label

  return(0);
}

/*
** Save an image to a file named filename, if filename is NULL it goes to stdout
*/

int msSaveImageGD(gdImagePtr img, char *filename, outputFormatObj *format )
{
  FILE *stream;

  if( format->imagemode == MS_IMAGEMODE_RGBA )
    gdImageSaveAlpha( img, 1 );
  else if( format->imagemode == MS_IMAGEMODE_RGB )
    gdImageSaveAlpha( img, 0 );

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

static void msFixedImageCopy (gdImagePtr dst, gdImagePtr src,  int dstX, int dstY, int srcX, int srcY, int w, int h)
{
    int x, y;

    /* for most cases the GD copy is fine */
    if( !gdImageTrueColor(dst) || gdImageTrueColor(src) )
        return gdImageCopy( dst, src, dstX, dstY, srcX, srcY, w, h );

    /* But for copying 8bit to 24bit the GD 2.0.1 copy has a bug with
       transparency */

    for (y = 0; (y < h); y++)
    {
        for (x = 0; (x < w); x++)
        {
            int c_8 = gdImageGetPixel (src, srcX + x, srcY + y);

            if (c_8 != src->transparent)
            {
                int c;

                c = gdTrueColorAlpha (src->red[c_8], 
                                      src->green[c_8], 
                                      src->blue[c_8],
                                      gdAlphaOpaque );
                gdImageSetPixel (dst,
                                 dstX + x,
                                 dstY + y,
                                 c);
            }
        }
    }
    return;
}

void msImageCopyMerge (gdImagePtr dst, gdImagePtr src, int dstX, int dstY, int srcX, int srcY, int w, int h, int pct)
{
    int x, y;

    /* for most cases the GD copy is fine */
    if( !gdImageTrueColor(dst) || !gdImageTrueColor(src) )
        return gdImageCopyMerge( dst, src, dstX, dstY, srcX, srcY, w, h, pct );

    /* 
    ** Turn off blending in output image to prevent it doing it's own attempt
    ** at blending instead of using our result. 
    */
    gdImageAlphaBlending( dst, 0 );

    for (y = 0; (y < h); y++)
    {
        for (x = 0; (x < w); x++)
        {
            int src_c = gdImageGetPixel (src, srcX + x, srcY + y);
            int dst_c = gdImageGetPixel (dst, dstX + x, dstY + y);
            int red, green, blue, res_alpha;
            int src_alpha = (127-gdTrueColorGetAlpha(src_c));
            int dst_alpha = (127-gdTrueColorGetAlpha(dst_c));

            if( gdTrueColorGetAlpha(src_c) == gdAlphaTransparent )
                continue;

            /* Adjust dst alpha according to percentages */
            dst_alpha = dst_alpha * ((100-pct)*src_alpha/127) / 100;

            /* adjust source according to transparency percentage */
            src_alpha = src_alpha * (pct) / 100;

            /* Use simple additive model for resulting transparency */
            res_alpha = src_alpha + dst_alpha;
            if( res_alpha > 127 )
                res_alpha = 127;
            
            if( src_alpha + dst_alpha == 0 )
                dst_alpha = 1;

            red = ((gdTrueColorGetRed( src_c ) * src_alpha)
                   + (gdTrueColorGetRed( dst_c ) * dst_alpha))
                / (src_alpha+dst_alpha);
            green = ((gdTrueColorGetGreen( src_c ) * src_alpha)
                     + (gdTrueColorGetGreen( dst_c ) * dst_alpha))
                / (src_alpha+dst_alpha);
            blue = ((gdTrueColorGetBlue( src_c ) * src_alpha)
                    + (gdTrueColorGetBlue( dst_c ) * dst_alpha))
                / (src_alpha+dst_alpha);
            
            gdImageSetPixel(dst,dstX+x,dstY+y,
                            gdTrueColorAlpha( red, green, blue, 
                                              127-res_alpha ));
        }
    }

    /*
    ** Restore original alpha blending flag. 
    */
    gdImageAlphaBlending( dst, 0 );
}

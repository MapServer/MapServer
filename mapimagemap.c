// $Id$ //

#include "map.h"
#include "mapparser.h"

#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define MYDEBUG 3
#define DEBUG if (MYDEBUG > 2) 

//static unsigned char PNGsig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; // 89 50 4E 47 0D 0A 1A 0A hex
//static unsigned char JPEGsig[3] = {255, 216, 255}; // FF D8 FF hex

static gdImagePtr searchImageCache(struct imageCacheObj *ic, int symbol, int color, int size) {
  struct imageCacheObj *icp;
DEBUG printf("searchImageCache\n<BR>");
/*
  icp = ic;
  while(icp) {
    if(icp->symbol == symbol && icp->color == color && icp->size == size) return(icp->img);
    icp = icp->next;
  }
*/
  return(NULL);
}

static struct imageCacheObj *addImageCache(struct imageCacheObj *ic, int *icsize, int symbol, int color, int size, gdImagePtr img) {
  struct imageCacheObj *icp;
DEBUG printf("addImageCache\n<BR>");
/*
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
 */
  return(icp);
}


//*
// * Utility function to create a IM image. Returns
// * a pointer to an imageObj structure.
 //  
imageObj *msImageCreateIM(int width, int height, outputFormatObj *format,
                          char *imagepath, char *imageurl)
{
    imageObj  *image;
    if (setvbuf(stdout, NULL, _IONBF , 0)){
               printf("Whoops...");
    };
DEBUG printf("msImageCreateIM<BR>\n");
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
	    char head[500] = "";
		
            image->format = format;
            format->refcount++;

            image->width = width;
            image->height = height;
            image->imagepath = NULL;
            image->imageurl = NULL;
	    sprintf(head, "<MAP name='map1' width=%d height=%d>", image->width, image->height);
	    image->img.imagemap = strdup(head);
//	    if (image->img.imagemap)
//	            image->img.IMsize = strlen(image->img.imagemap);
//	    else
		    image->size = 0;
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

//*
// * Utility function to initialize the color of an image.  The background
// * color is passed, but the outputFormatObj is consulted to see if the
// * transparency should be set (for RGBA images).   Note this function only
// * affects TrueColor images. 
 //  

void msImageInitIM( imageObj *image )

{
DEBUG printf("msImageInitIM<BR>\n");
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

//*
// * Utility function to load an image in a IM supported format, and
// * return it as a valid imageObj.
 //  
imageObj *msImageLoadIM( const char *filename )
{
  FILE *stream;
  gdImagePtr img=NULL;
  const char *driver = NULL;
  char bytes[8];
  imageObj      *image = NULL;
DEBUG printf("msImageLoadIM<BR>\n");
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

// TODO: might be a way to optimize this (halve the number of additions)
static void imageOffsetPolyline(gdImagePtr img, shapeObj *p, int color, int offsetx, int offsety)
{
  int i, j;
  double dx, dy;
  int ox=0, oy=0;
DEBUG printf("imageOffsetPolyline\n<BR>");
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
  int i, j;
  
DEBUG printf("imagePolyline\n<BR>");
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

// ---------------------------------------------------------------------------//
//       Stroke an ellipse with a line symbol of the specified size and color //
// ---------------------------------------------------------------------------//
void msCircleDrawLineSymbolIM(symbolSetObj *symbolset, imageObj* img, pointObj *p, double r, styleObj *style, double scalefactor)
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
DEBUG printf("msCircleDrawLineSymbolIM\n<BR>");
/*  
  if(!p) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor));
  
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
    if((brush = searchImageCache(symbolset->imagecache, style->symbol, fc, size)) == NULL) { // not in cache, create
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
    if(bc == -1) bc = imTransparent;

    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);

    if((x < 2) && (y < 2)) break;

    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style->symbol, fc, size)) == NULL) { // not in cache, create
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
*/
  return;
}

// ------------------------------------------------------------------------------- //
//       Fill a circle with a shade symbol of the specified size and color       //
// ------------------------------------------------------------------------------- //
void msCircleDrawShadeSymbolIM(symbolSetObj *symbolset, imageObj* img, 
                               pointObj *p, double r, styleObj *style, double scalefactor)
{
  symbolObj *symbol;
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

DEBUG printf("msCircleDrawShadeSymbolIM<BR>\n");
/*
  if(!p) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor));

  symbol = &(symbolset->symbol[style->symbol]);
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
    font = msLookupHashTable(symbolset->fontset->fonts, symbol->font);
    if(!font) return;

    if(getCharacterSize(symbol->character, size, font, &rect) == -1) return;
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


// ------------------------------------------------------------------------------- //
//       Draw a single marker symbol of the specified size and color               //
// ------------------------------------------------------------------------------- //
void msDrawMarkerSymbolIM(symbolSetObj *symbolset, imageObj* img, pointObj *p, styleObj *style, double scalefactor)
{
  symbolObj *symbol;
  int offset_x, offset_y, x, y;
  int j;
  gdPoint oldpnt,newpnt;
  gdPoint mPoints[MS_MAXVECTORPOINTS];

  gdImagePtr tmp;
  int tmp_fc=-1, tmp_bc, tmp_oc=-1;
  int fc, bc, oc;
  double size,d;

  int ox, oy;
  
  int bbox[8];
  rectObj rect;
  char *font=NULL;
//DEBUG printf("msDrawMarkerSymbolIM\n<BR>");

// skip this, we don't do text



  if(!p) return;

//  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor));
//  if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color));
//  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor));

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
//  if(fc<0 && oc<0) return; // nothing to do
  if(size < 1) return; // size too small

  if(style->symbol == 0) { // simply draw a single pixel of the specified color //
//    gdImageSetPixel(img, p->x + ox, p->y + oy, fc);
		int slen = 0;
		int nchars = 0;
		char buffer[200] = "";
		nchars = snprintf (buffer, 200, "<AREA href='javascript:SymbolClicked();' shape='circle' coords='%.0f,%.0f, 3'></A>\n", p->x + ox, p->y + oy);
//DEBUG printf ("%d, ",strlen(img->img.imagemap) );
// DEBUG printf("nchars %d<BR>\n", nchars);
		slen = nchars + strlen(img->img.imagemap) + 8; // add 8 to accomodate </A> tag
		if (slen > img->size){
			img->img.imagemap = (char *) realloc (img->img.imagemap, slen*2); // double allocated string size if needed
			if (img->img.imagemap)
				img->size = slen*2;
		}
		strcat(img->img.imagemap, buffer);
		      
	//        point2 = &( p->line[j].point[i] );
	//        if(point1->y == point2->y) {}
    return;
  }  

  switch(symbol->type) {  
  case(MS_SYMBOL_TRUETYPE):
DEBUG printf("T");
/*
 * #if defined (USE_GD_FT) || defined (USE_GD_TTF)
    font = msLookupHashTable(symbolset->fontset->fonts, symbol->font);
    if(!font) return;

    if(getCharacterSize(symbol->character, size, font, &rect) == -1) return;

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
DEBUG printf("P");
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
DEBUG printf("V");
/*    d = size/symbol->sizey;
    offset_x = MS_NINT(p->x - d*.5*symbol->sizex + ox);
    offset_y = MS_NINT(p->y - d*.5*symbol->sizey + oy);
    
    if(symbol->filled) { // if filled //
      for(j=0;j < symbol->numpoints;j++) {
	mPoints[j].x = MS_NINT(d*symbol->points[j].x + offset_x);
	mPoints[j].y = MS_NINT(d*symbol->points[j].y + offset_y);
      }
      if(fc >= 0)
	gdImageFilledPolygon(img, mPoints, symbol->numpoints, fc);
      if(oc >= 0)
	gdImagePolygon(img, mPoints, symbol->numpoints, oc);
      
    } else  { // NOT filled //     

      if(fc < 0) return;
      
      oldpnt.x = MS_NINT(d*symbol->points[0].x + offset_x); // convert first point in marker s //
      oldpnt.y = MS_NINT(d*symbol->points[0].y + offset_y);
      
      for(j=1;j < symbol->numpoints;j++) { // step through the marker s //
	if((symbol->points[j].x < 0) && (symbol->points[j].x < 0)) {
	  oldpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
	  oldpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
	} else {
	  if((symbol->points[j-1].x < 0) && (symbol->points[j-1].y < 0)) { // Last point was PENUP, now a new beginning //
	    oldpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
	    oldpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
	  } else {
	    newpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
	    newpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
	    gdImageLine(img, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, fc);
	    oldpnt = newpnt;
	  }
	}
      } // end for loop //      
    } // end if-then-else //
*/    break;
  default:
    break;
  } // end switch statement //

  return;
}

// ------------------------------------------------------------------------------- //
//       Draw a line symbol of the specified size and color                        //
// ------------------------------------------------------------------------------- //
void msDrawLineSymbolIM(symbolSetObj *symbolset, imageObj* img, shapeObj *p, styleObj *style, double scalefactor)
{
  int i, j;
  symbolObj *symbol;
  int styleDashed[100];
  int x, y;
  int fc, bc;
  int brush_bc, brush_fc;
  double size, d;
  gdImagePtr brush=NULL;
  gdPoint points[MS_MAXVECTORPOINTS];
//DEBUG printf("msDrawLineSymbolIM<BR>\n");

// Lines are also of no interest to us

/*

  if(!p) return;
  if(p->numlines <= 0) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color));

  symbol = &(symbolset->symbol[style->symbol]);
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  if(fc==-1) fc = style->outlinecolor.pen;
  size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; // no such symbol, 0 is OK
  if(fc < 0) return; // nothing to do
  if(size < 1) return; // size too small

  if(style->symbol == 0) { // just draw a single width line
    imagePolyline(img, p, fc, style->offsetx, style->offsety);
    return;
  }

  switch(symbol->type) {
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
    if((brush = searchImageCache(symbolset->imagecache, style->symbol, fc, size)) == NULL) { // not in cache, create
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
    if(bc == -1) bc = imTransparent;

    d = size/symbol->sizey;
    x = MS_NINT(symbol->sizex*d);    
    y = MS_NINT(symbol->sizey*d);

    if((x < 2) && (y < 2)) break;

    // create the brush image
    if((brush = searchImageCache(symbolset->imagecache, style->symbol, fc, size)) == NULL) { // not in cache, create
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


// ------------------------------------------------------------------------------- //
//       Draw a shade symbol of the specified size and color                       //
// ------------------------------------------------------------------------------- //
void msDrawShadeSymbolIM(symbolSetObj *symbolset, imageObj* img, shapeObj *p, styleObj *style, double scalefactor)
{
  symbolObj *symbol;
  int i,j,k,l;
  gdPoint oldpnt, newpnt;
  gdPoint sPoints[MS_MAXVECTORPOINTS];
  gdImagePtr tile;
  int x, y;
  int tile_bc=-1, tile_fc=-1; // colors (background and foreground) //
  int fc, bc, oc;
  double size, d;
  int bbox[8];
  rectObj rect;
  char *font=NULL;
int bsize = 100;
char first = 1;
char *buffer = (char *) malloc (bsize);
char *fbuffer = (char *) malloc (bsize);
int nchars = 0;

//DEBUG printf("msDrawShadeSymbolIM\n<BR>");
  if(!p) return;
  if(p->numlines <= 0) return;
//  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->backgroundcolor));
//  if(style->color.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->color));
//  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenIM(img, &(style->outlinecolor));

//  symbol = &(symbolset->symbol[style->symbol]);
//  bc = style->backgroundcolor.pen;
//  fc = style->color.pen;
//  oc = style->outlinecolor.pen;
  size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

//DEBUG printf ("a");
//  if(fc==-1 && oc!=-1) { // use msDrawLineSymbolIM() instead (POLYLINE)
//    msDrawLineSymbolIM(symbolset, img, p, style, scalefactor);
//    return;
//  }
//DEBUG printf ("b");

//  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; // no such symbol, 0 is OK
//DEBUG printf ("1");
//  if(fc < 0) return; // nothing to do
//DEBUG printf ("2");
//  if(size < 1) return; // size too small
//DEBUG printf ("3");
      
//DEBUG printf("BEF%s", img->img.imagemap);
  nchars = snprintf (fbuffer, bsize, "<AREA href='javascript:Clicked(%s);' title='%s' shape='poly' coords='", p->numvalues ? p->values[0] : "", p->numvalues ? p->values[0] : "");
  if(style->symbol == 0) { // simply draw a single pixel of the specified color //    
	  for(l=0,j=0; j<p->numlines; j++) {
	//      point1 = &( p->line[j].point[p->line[j].numpoints-1] );
	      for(i=0; i < p->line[j].numpoints; i++,l++) {
		        int slen = 0;
			nchars = snprintf (buffer, bsize, "%s %.0f,%.0f", first ? fbuffer: ",", p->line[j].point[i].x, p->line[j].point[i].y);
//DEBUG printf ("%d, ",strlen(img->img.imagemap) );
// DEBUG printf("nchars %d<BR>\n", nchars);
			slen = nchars + strlen(img->img.imagemap) + 8; // add 8 to accomodate </A> tag
 			if (slen > img->size){
				img->img.imagemap = (char *) realloc (img->img.imagemap, slen*2); // double allocated string size if needed
				if (img->img.imagemap)
					img->size = slen*2;
			}
			strcat(img->img.imagemap, buffer);
			first = 0;
		      
	//        point2 = &( p->line[j].point[i] );
	//        if(point1->y == point2->y) {}
	      }
	  }
    strcat(img->img.imagemap, "'></A>\n");
  
//DEBUG printf("AFT%s", img->img.imagemap);
// STOOPID. GD draws polygons pixel by pixel ?!
	
//    msImageFilledPolygon(img, p, fc);
//    if(oc>-1) imagePolyline(img, p, oc, style->offsetx, style->offsety);
    return;
  }
DEBUG printf ("d");
  DEBUG printf("-%d-",symbol->type);
/*  
  switch(symbol->type) {
  case(MS_SYMBOL_TRUETYPE):    
    
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    font = msLookupHashTable(symbolset->fontset->fonts, symbol->font);
    if(!font) return;

    if(getCharacterSize(symbol->character, size, font, &rect) == -1) return;
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

static void billboardIM(imageObj* img, shapeObj *shape, labelObj *label)
{
  int i;
  shapeObj temp;
DEBUG printf("billboardIM<BR>\n");
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
}

//
** Simply draws a label based on the label point and the supplied label object.
//
int msDrawTextIM(gdImagePtr img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor)
{
  int x, y;

  if(!string) return(0); // not errors, just don't want to do anything //
  if(strlen(string) == 0) return(0);

  x = MS_NINT(labelPnt.x);
  y = MS_NINT(labelPnt.y);

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

    font = msLookupHashTable(fontset->fonts, label->font);
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

int msDrawLabelCacheIM(char* img, mapObj *map)
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

DEBUG printf("msDrawLabelCacheIM\n<BR>");
/*for(l=map->labelcache.numlabels-1; l>=0; l--) {

    cachePtr = &(map->labelcache.labels[l]); // point to right spot in the label cache

    layerPtr = &(map->layers[cachePtr->layerindex]); // set a couple of other pointers, avoids nasty references
    labelPtr = &(cachePtr->label);

    if(!cachePtr->string || strlen(cachePtr->string) == 0)
      continue; // not an error, just don't want to do anything

    if(cachePtr->label.type == MS_TRUETYPE)
      cachePtr->label.size *= layerPtr->scalefactor;

    if(msGetLabelSize(cachePtr->string, labelPtr, &r, &(map->fontset)) == -1)
      return(-1);

    if(labelPtr->autominfeaturesize && ((r.maxx-r.minx) > cachePtr->featuresize))
      continue; // label too large relative to the feature //

    marker_offset_x = marker_offset_y = 0; // assume no marker //
    if((layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) || layerPtr->type == MS_LAYER_POINT) { // there *is* a marker      
      msGetMarkerSize(&map->symbolset, &cachePtr->styles, cachePtr->numstyles, &marker_width, &marker_height);
      marker_width *= layerPtr->scalefactor;
      marker_height *= layerPtr->scalefactor;

      marker_offset_x = MS_NINT(marker_width/2.0);
      marker_offset_y = MS_NINT(marker_height/2.0);      

      marker_rect.minx = MS_NINT(cachePtr->point.x - .5 * marker_width);
      marker_rect.miny = MS_NINT(cachePtr->point.y - .5 * marker_height);
      marker_rect.maxx = marker_rect.minx + (marker_width-1);
      marker_rect.maxy = marker_rect.miny + (marker_height-1);

      for(i=0; i<cachePtr->numstyles; i++)
	cachePtr->styles[i].size *= layerPtr->scalefactor;
    }

    if(labelPtr->position == MS_AUTO) {

      if(layerPtr->type == MS_LAYER_LINE) {
	int position = MS_UC;

	for(j=0; j<2; j++) { // Two angles or two positions, depending on angle. Steep angles will use the angle approach, otherwise we'll rotate between UC and LC. //

	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; // assume label *can* be drawn //

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
	      if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { // polys intersect //
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(!cachePtr->status)
	    continue; // next angle

	  for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
	    if(map->labelcache.labels[i].status == MS_TRUE) { // compare bounding polygons and check for duplicates //

	      if((labelPtr->mindistance != -1) && (cachePtr->classindex == map->labelcache.labels[i].classindex) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= labelPtr->mindistance)) { // label is a duplicate //
		cachePtr->status = MS_FALSE;
		break;
	      }

	      if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { // polys intersect //
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(cachePtr->status) // found a suitable place for this label
	    break;

	} // next angle

      } else {
	for(j=0; j<=7; j++) { // loop through the outer label positions //

	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; // assume label *can* be drawn //

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
	      if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { // polys intersect //
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(!cachePtr->status)
	    continue; // next position

	  for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
	    if(map->labelcache.labels[i].status == MS_TRUE) { // compare bounding polygons and check for duplicates //

	      if((labelPtr->mindistance != -1) && (cachePtr->classindex == map->labelcache.labels[i].classindex) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= labelPtr->mindistance)) { // label is a duplicate //
		cachePtr->status = MS_FALSE;
		break;
	      }

	      if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { // polys intersect //
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(cachePtr->status) // found a suitable place for this label
	    break;
	} // next position
      }

      if(labelPtr->force) cachePtr->status = MS_TRUE; // draw in spite of collisions based on last position, need a *best* position //

    } else {

      cachePtr->status = MS_TRUE; // assume label *can* be drawn //

      if(labelPtr->position == MS_CC) // don't need the marker_offset
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, labelPtr->offsetx, labelPtr->offsety, labelPtr->angle, labelPtr->buffer, cachePtr->poly);
      else
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, (marker_offset_x + labelPtr->offsetx), (marker_offset_y + labelPtr->offsety), labelPtr->angle, labelPtr->buffer, cachePtr->poly);

      if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
	msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon, part of overlap tests //

      if(!labelPtr->force) { // no need to check anything else

	if(!labelPtr->partials) {
	  if(labelInImage(img->sx, img->sy, cachePtr->poly, labelPtr->buffer) == MS_FALSE)
	    cachePtr->status = MS_FALSE;
	}

	if(!cachePtr->status)
	  continue; // next label

	for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	  if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	    if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { // polys intersect //
	      cachePtr->status = MS_FALSE;
	      break;
	    }
	  }
	}

	if(!cachePtr->status)
	  continue; // next label

	for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered label
	  if(map->labelcache.labels[i].status == MS_TRUE) { // compare bounding polygons and check for duplicates //
	    if((labelPtr->mindistance != -1) && (cachePtr->classindex == map->labelcache.labels[i].classindex) && (strcmp(cachePtr->string, map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= labelPtr->mindistance)) { // label is a duplicate //
	      cachePtr->status = MS_FALSE;
	      break;
	    }

	    if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { // polys intersect //
	      cachePtr->status = MS_FALSE;
	      break;
	    }
	  }
	}
      }
    } // end position if-then-else //

    // imagePolyline(img, cachePtr->poly, 1, 0, 0); //

    if(!cachePtr->status)
      continue; // next label //

    if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) { // need to draw a marker //
      for(i=0; i<cachePtr->numstyles; i++)
        msDrawMarkerSymbolIM(&map->symbolset, img, &(cachePtr->point), &(cachePtr->styles[i]), layerPtr->scalefactor);
    }

    if(MS_VALID_COLOR(labelPtr->backgroundcolor)) billboardIM(img, cachePtr->poly, labelPtr);
    msDrawTextIM(img, p, cachePtr->string, labelPtr, &(map->fontset), layerPtr->scalefactor); // actually draw the label, we scaled it in msAddLabel

  } // next label in cache //
*/
  return(0);
}

//
//** Save an image to a file named filename, if filename is NULL it goes to stdout
//

int msSaveImageIM(imageObj* img, char *filename, outputFormatObj *format )

{
    FILE *stream;
DEBUG printf("msSaveImageIM\n<BR>");

if(filename != NULL && strlen(filename) > 0) {
    stream = fopen(filename, "wb");
    if(!stream) {
      msSetError(MS_IOERR, "(%s)", "msSaveImage()", filename);
      return(MS_FAILURE);
    }
  } else { // use stdout //

#ifdef _WIN32
    //
   //Change stdout mode to binary on win32 platforms
    //
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
DEBUG printf("ALLOCD %d<BR>\n", img->size);
//DEBUG printf("F %s<BR>\n", img->img.imagemap);
DEBUG printf("FLEN %d<BR>\n", strlen(img->img.imagemap));
	  fprintf(stream, img->img.imagemap);
	  fprintf(stream, "</MAP>");
/*#ifdef USE_GD_GIF
    gdImageGif(img, stream);
#else
    msSetError(MS_MISCERR, "GIF output is not available.", "msSaveImage()");
    return(MS_FAILURE);
#endif*/
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


//*
// * Free gdImagePtr
 //
void msFreeImageIM(imageObj* img)
{
  free(img->img.imagemap);
}

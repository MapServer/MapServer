#include <stdarg.h> /* variable number of function arguments support */

#include "map.h"
#include "mapfile.h"
#include "mapparser.h"

#ifdef USE_TTF
#include <gdcache.h>
#include <gdttf.h>
#include "freetype.h"
#endif

extern int msyylex(); /* lexer globals */
extern void msyyrestart();
extern double msyynumber;
extern char *msyytext;
extern int msyylineno;
extern FILE *msyyin;
extern int msyyfiletype;

static gdImagePtr searchImageCache(struct imageCacheObj *ic, int symbol, int color, int size) {
  struct imageCacheObj *icp;

  icp = ic;
  while(icp) {
    if(icp->symbol == symbol && icp->color == color && icp->size == size) return(icp->img);
    icp = icp->next;
  }

  return(NULL);
}

static void freeImageCache(struct imageCacheObj *ic)
{
  if(ic) {
    freeImageCache(ic->next); /* free any children */
    gdImageDestroy(ic->img);
    free(ic);  
  }
  return;
}

static struct imageCacheObj *addImageCache(struct imageCacheObj *ic, int *icsize, int symbol, int color, int size, gdImagePtr img) {
  struct imageCacheObj *icp;

  if(*icsize > MS_IMAGECACHESIZE) { // remove last element, size stays the same
    icp = ic;
    while(icp->next) icp = icp->next;
    freeImageCache(icp);
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

static void initSymbol(symbolObj *s)
{
  s->type = MS_SYMBOL_VECTOR;
  s->transparent = MS_FALSE;
  s->transparentcolor = 0;
  s->numarcs = 1; // always at least 1 arc
  s->numon[0] = 0;
  s->numoff[0] = 0;
  s->offset[0] = 0;
  s->sizex = 0;
  s->sizey = 0;
  s->filled = MS_FALSE;
  s->numpoints=0;
  s->img = NULL;
  s->name = NULL;

  s->antialias = -1;
  s->font = NULL;
  s->character = '\0';
}

static void freeSymbol(symbolObj *s) {
  if(!s) return;
  free(s->name);
  if(s->img) gdImageDestroy(s->img);
  free(s->font);
}

static int loadSymbol(symbolObj *s)
{
  int i=0;
  int done=MS_FALSE;
  FILE *stream;

  initSymbol(s);

  for(;;) {
    switch(msyylex()) {
    case(ANTIALIAS):
      s->antialias = 1;
      break;
    case(CHARACTER):
      if((s->character = getString()) == NULL) return(-1);
      break;
    case(END): /* do some error checking */

      if((s->type == MS_SYMBOL_PIXMAP) && (s->img == NULL)) {
	msSetError(MS_SYMERR, "Symbol of type PIXMAP has no image data.", "loadSymbol()"); 
	return(-1);
      }
      if(((s->type == MS_SYMBOL_ELLIPSE) || (s->type == MS_SYMBOL_VECTOR))  && (s->numpoints == 0)) {
	msSetError(MS_SYMERR, "Symbol of type VECTOR or ELLIPSE has no point data.", "loadSymbol()"); 
	return(-1);
      }
      if(s->type == MS_SYMBOL_PIXMAP && s->transparent)
	gdImageColorTransparent(s->img, s->transparentcolor);

      return(0);
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadSymbol()");      
      return(-1);
      break;
    case(FILLED):
      s->filled = MS_TRUE;
      break;
    case(FONT):
      if((s->font = getString()) == NULL) return(-1);
      break;  
    case(IMAGE):
      if(msyylex() != MS_STRING) { /* get image location from next token */
	msSetError(MS_TYPEERR, NULL, "loadSymbol()"); 
	sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
	fclose(msyyin);
	return(-1);
      }
      
      if((stream = fopen(msyytext, "rb")) == NULL) {
	msSetError(MS_IOERR, NULL, "loadSymbol()");
	sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
	fclose(msyyin);
	return(-1);
      }
      
#ifndef USE_GD_1_6
      s->img = gdImageCreateFromGif(stream);
#else	
      s->img = gdImageCreateFromPng(stream);
#endif
      fclose(stream);
      
      if(s->img == NULL) {
	msSetError(MS_GDERR, NULL, "loadSymbol()");
	fclose(msyyin);
	return(-1);
      }
      break;
    case(NAME):
      if((s->name = getString()) == NULL) return(-1);
      break;
    case(POINTS):
      i = 0;
      done = MS_FALSE;
      for(;;) {
	switch(msyylex()) { 
	case(END):
	  done = MS_TRUE;
	  break;
	case(MS_NUMBER):
	  s->points[i].x = atof(msyytext); /* grab the x */
	  if(getDouble(&(s->points[i].y)) == -1) return(-1); /* grab the y */
	  s->sizex = MS_MAX(s->sizex, s->points[i].x);
	  s->sizey = MS_MAX(s->sizey, s->points[i].y);	
	  i++;
	  break;
	default:
	  msSetError(MS_TYPEERR, NULL, "loadSymbol()"); 
	  sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno); 
	  fclose(msyyin);
	  return(-1);
	}

	if(done == MS_TRUE)
	  break;
      }      
      s->numpoints = i;
      break;    
    case(STYLE):
      i = 0;
      done = MS_FALSE;
      for(;;) { /* read till the next END */
	switch(msyylex()) {  
	case(END):
	  done = MS_TRUE;
	  break;
	case(MS_NUMBER): /* read the style values */
	  s->offset[i] = atoi(msyytext);
	  if(getInteger(&(s->numon[i])) == -1) return(-1);
	  if(getInteger(&(s->numoff[i])) == -1) return(-1);
	  i++;
	  break;
	default:
	  msSetError(MS_TYPEERR, NULL, "loadSymbol()"); 
	  sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno); 
	  fclose(msyyin);
	  return(-1);
	}
	if(done == MS_TRUE)
	  break;
      }
      s->numarcs = i;
      break;
    case(TRANSPARENT):
      s->transparent = MS_TRUE;
      if(getInteger(&(s->transparentcolor)) == -1) return(-1);
      break;
    case(TYPE):
#ifdef USE_TTF
      if((s->type = getSymbol(6,MS_SYMBOL_VECTOR,MS_SYMBOL_ELLIPSE,MS_SYMBOL_PIXMAP,MS_SYMBOL_STYLED,MS_SYMBOL_TRUETYPE)) == -1)
	return(-1);	
#else
      if((s->type = getSymbol(5,MS_SYMBOL_VECTOR,MS_SYMBOL_ELLIPSE,MS_SYMBOL_PIXMAP,MS_SYMBOL_STYLED)) == -1)
	return(-1);
#endif
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadSymbol()");
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      fclose(msyyin);
      return(-1);
    } /* end switch */
  } /* end for */
}

/*
** Little helper function to allow us to build symbol files on-the-fly from just a file name.
*/
int msAddImageSymbol(symbolSetObj *symbolset, char *filename) 
{
  FILE *stream;
  int i;

  if(!symbolset) {
    msSetError(MS_SYMERR, "Symbol structure unallocated.", "msAddImageSymbol()");
    return(-1);
  }

  if(!filename || strlen(filename) == 0) return(-1);

  if(symbolset->numsymbols == MS_MAXSYMBOLS) { // no room
    msSetError(MS_SYMERR, "Maximum number of symbols reached.", "msAddImageSymbol()");
    return(-1);
  }

  if((stream = fopen(filename, "rb")) == NULL) {
    msSetError(MS_IOERR, NULL, "msAddImageSymbol()");
    return(-1);
  }

  i = symbolset->numsymbols;  

  initSymbol(&symbolset->symbol[i]);

#ifndef USE_GD_1_6
  symbolset->symbol[i].img = gdImageCreateFromGif(stream);
#else
  symbolset->symbol[i].img = gdImageCreateFromPng(stream);
#endif
  fclose(stream);
  
  if(!symbolset->symbol[i].img) {
    msSetError(MS_GDERR, NULL, "msAddImageSymbol()");
    return(-1);
  }

  symbolset->symbol[i].name = strdup(filename);
  symbolset->symbol[i].type = MS_SYMBOL_PIXMAP;
  symbolset->numsymbols++;

  return(i);
}

void msFreeSymbolSet(symbolSetObj *symbolset)
{
  int i;

  freeImageCache(symbolset->imagecache);

  for(i=0; i<symbolset->numsymbols; i++)
    freeSymbol(&(symbolset->symbol[i]));

#ifdef USE_TTF
  if(symbolset->fontset.filename) {
    free(symbolset->fontset.filename);
    msFreeHashTable(symbolset->fontset.fonts);
  }
#endif
}

/*
** Load the symbols contained in the given file
*/
int msLoadSymbolSet(symbolSetObj *symbolset)
{
  int n=1;
  char old_path[MS_PATH_LENGTH];
  char *symbol_path;
  int status=1;

  if(!symbolset) {
    msSetError(MS_SYMERR, "Symbol structure unallocated.", "msLoadSymbolFile()");
    return(-1);
  }

  if(!symbolset->filename) return(0);

  /*
  ** Open the file
  */
  if((msyyin = fopen(symbolset->filename, "r")) == NULL) {
    msSetError(MS_IOERR, NULL, "msLoadSymbolFile()");
    sprintf(ms_error.message, "(%s)", symbolset->filename);
    return(-1);
  }

  getcwd(old_path, MS_PATH_LENGTH); /* save old working directory */
  symbol_path = getPath(symbolset->filename);
  chdir(symbol_path);
  free(symbol_path);

  msyylineno = 0; /* reset line counter */
  msyyrestart(msyyin); /* flush the scanner - there's a better way but this works for now */
  msyyfiletype = MS_FILE_SYMBOL;

#ifdef USE_TTF
  symbolset->fontset.filename = NULL;
  symbolset->fontset.numfonts = 0;  
  symbolset->fontset.fonts = NULL;
#endif

  /*
  ** Read the symbol file
  */
  for(;;) {
    switch(msyylex()) {
    case(EOF):
      symbolset->numsymbols = n;
      status = 0;

#ifdef USE_TTF
      if(msLoadFontSet(&(symbolset->fontset)) == -1) return(-1);
#endif

      break;
    case(FONTSET):
#ifdef USE_TTF
      if((symbolset->fontset.filename = getString()) == NULL) return(-1);
#endif
      break;    
    case(SYMBOL):
      if((loadSymbol(&(symbolset->symbol[n])) == -1)) 
	status = -1;
      n++;
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "msLoadSymbolFile()");
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      status = -1;
    } /* end switch */

    if(status != 1) break;
  } /* end for */

  msyyfiletype = MS_FILE_DEFAULT;
  fclose(msyyin);
  chdir(old_path);
  return(status);
}

static int getCharacterSize(char *character, int size, char *font, rectObj *rect) {
  int bbox[8];
  char *error=NULL;

#ifdef USE_TTF
  error = imageStringTTF(NULL, bbox, 0, font, size, 0, 0, 0, character, '\n');
  if(error) {
    msSetError(MS_TTFERR, error, "getCharacterSize()");
    return(-1);
  }    
  
  rect->minx = bbox[0];
  rect->miny = bbox[5];
  rect->maxx = bbox[2];
  rect->maxy = bbox[1];

  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "getCharacterSize()");
  return(-1);
#endif
}

/* ------------------------------------------------------------------------------- */
/*       Draw a shade symbol of the specified size and color                       */
/* ------------------------------------------------------------------------------- */
void msDrawShadeSymbol(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, int sy, int fc, int bc, int oc, double sz)
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

  if(fc >= gdImageColorsTotal(img)) /* invalid color, -1 is valid */
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
    
#ifdef USE_TTF
    font = msLookupHashTable(symbolset->fontset.fonts, symbolset->symbol[sy].font);
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
    imageStringTTF(tile, bbox, symbol->antialias*tile_fg, font, sz, 0, x, y, symbol->character, '\n');
    
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

/* NEED TO CHANGE THIS FUNCTION TO DEAL WITH OVERLAY SYMBOLS */

/*
** Returns the size, in pixels, of a marker symbol defined for a specific class. Use for annotation
** layer collision avoidance.
*/
void msGetMarkerSize(symbolSetObj *symbolset, classObj *class, int *width, int *height)
{
  rectObj rect;
  char *font=NULL;

  if(class->symbol > symbolset->numsymbols) { /* no such symbol, 0 is OK */
    *width = 0;
    *height = 0;
    return;
  }

  if(class->symbol == 0) { /* single point */
    *width = 1;
    *height = 1;
    return;
  }

  switch(symbolset->symbol[class->symbol].type) {  
  case(MS_SYMBOL_TRUETYPE):

#ifdef USE_TTF
    font = msLookupHashTable(symbolset->fontset.fonts, symbolset->symbol[class->symbol].font);
    if(!font) return;

    if(getCharacterSize(symbolset->symbol[class->symbol].character, class->sizescaled, font, &rect) == -1) return;

    *width = rect.maxx - rect.minx;
    *height = rect.maxy - rect.miny;
#else
    *width = 0;
    *height = 0;
#endif
    break;

  case(MS_SYMBOL_PIXMAP):
    *width = symbolset->symbol[class->symbol].img->sx;
    *height = symbolset->symbol[class->symbol].img->sy;
    break;
  default: /* vector and ellipses, scalable */
    if(class->sizescaled > 0) {
      *height = class->sizescaled;
      *width = MS_NINT((class->sizescaled/symbolset->symbol[class->symbol].sizey) * symbolset->symbol[class->symbol].sizex);
    } else { /* use symbol defaults */
      *height = symbolset->symbol[class->symbol].sizey;
      *width = symbolset->symbol[class->symbol].sizex;
    }
    break;
  }  

  return;
}

/* ------------------------------------------------------------------------------- */
/*       Draw a single marker symbol of the specified size and color               */
/* ------------------------------------------------------------------------------- */
void msDrawMarkerSymbol(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, int sy, int fc, int bc, int oc, double sz)
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

#ifdef USE_TTF
    font = msLookupHashTable(symbolset->fontset.fonts, symbol->font);
    if(!font) return;

    if(getCharacterSize(symbol->character, sz, font, &rect) == -1) return;

    x = p->x - (rect.maxx - rect.minx)/2 - rect.minx;
    y = p->y - rect.maxy + (rect.maxy - rect.miny)/2;  

    imageStringTTF(img, bbox, symbol->antialias*fc, font, sz, 0, x, y, symbol->character, '\n');
#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    offset_x = MS_NINT(p->x - .5*symbol->img->sx);
    offset_y = MS_NINT(p->y - .5*symbol->img->sy);
    gdImageCopy(img, symbol->img, offset_x, offset_y, 0, 0, symbol->img->sx, symbol->img->sy);
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

/* ------------------------------------------------------------------------------- */
/*       Draw a line symbol of the specified size and color                        */
/* ------------------------------------------------------------------------------- */
void msDrawLineSymbol(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, , int sy, int fc, int bc, int oc, double sz)
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

  if(sy > symbolset->numsymbols || sy < 0) /* no such symbol, 0 is OK */
    return;

  if((fc < 0) || (fc >= gdImageColorsTotal(img))) /* invalid color */
    return;

  if(sz < 1) /* size too small */
    return;

  if(sy == 0) { /* just draw a single width line */
    msImagePolyline(img, p, fc);
    return;
  }

  symbol = &(symbolset->symbol[sy]);

  switch(symbol->type) {
  case(MS_SYMBOL_STYLED):
    if(bc == -1) bc = gdTransparent;
    break;
  case(MS_SYMBOL_ELLIPSE):
     
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

  for(i=0;i<symbol->numarcs;i++) { // Draw each line in the style
    if(symbol->numon[i] > 0) {
      for(j=0; j<symbol->numon[i]+symbol->numoff[i]; j++) {
	if(j<symbol->numon[i])
	  styleDashed[j] = fc;
	else
	  styleDashed[j] = bc;
      }
      gdImageSetStyle(img, styleDashed, j);    

      if(!brush && !symbol->img)
	msImagePolylineOffset(img, p, MS_NINT(scale*symbol->offset[i]), gdStyled);
      else 
	msImagePolylineOffset(img, p, MS_NINT(scale*symbol->offset[i]), gdStyledBrushed);
    } else {
      if(!brush && !symbol->img)
	msImagePolylineOffset(img, p, MS_NINT(scale*symbol->offset[i]), fc);
      else
	msImagePolylineOffset(img, p, MS_NINT(scale*symbol->offset[i]), gdBrushed);
    }
  }

  // if(brush)
  //   gdImageDestroy(brush);

  return;
}

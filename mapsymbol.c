#include <stdarg.h> /* variable number of function arguments support */

#include "map.h"
#include "mapfile.h"
#include "mapparser.h"

extern int msyylex(); /* lexer globals */
extern void msyyrestart();
extern double msyynumber;
extern char *msyytext;
extern int msyylineno;
extern FILE *msyyin;
extern int msyyfiletype;

extern unsigned char PNGsig[8];
extern unsigned char JPEGsig[3];

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

void initSymbol(symbolObj *s)
{
  s->type = MS_SYMBOL_VECTOR;
  s->transparent = MS_FALSE;
  s->transparentcolor = 0;
  s->stylelength = 0; // solid line
  s->sizex = 0;
  s->sizey = 0;
  s->filled = MS_FALSE;
  s->numpoints=0;
  s->img = NULL;
  s->name = NULL;
  s->gap = 0;

  s->antialias = MS_FALSE;
  s->font = NULL;
  s->character = '\0';
  s->position = MS_CC;

  s->linecap = MS_CJC_BUTT;
  s->linejoin = MS_CJC_ROUND;
  s->linejoinmaxsize = 3;
}

static void freeSymbol(symbolObj *s) {
  if(!s) return;
  if(s->name) free(s->name);
  if(s->img) gdImageDestroy(s->img);
  if(s->font) free(s->font);
}

int loadSymbol(symbolObj *s)
{
  int done=MS_FALSE;
  FILE *stream;
  char bytes[8];

  initSymbol(s);

  for(;;) {
    switch(msyylex()) {
    case(ANTIALIAS):
      if((s->antialias = getSymbol(2,MS_TRUE,MS_FALSE)) == -1)
	return(-1);
      break;    
    case(CHARACTER):
      if((s->character = getString()) == NULL) return(-1);
      break;
    case(END): /* do some error checking */

      if((s->type == MS_SYMBOL_PIXMAP) && (s->img == NULL)) {
	msSetError(MS_SYMERR, "Symbol of type PIXMAP has no image data.", "loadSymbol()"); 
	return(-1);
      }
      if(((s->type == MS_SYMBOL_ELLIPSE) || (s->type == MS_SYMBOL_VECTOR)) && (s->numpoints == 0)) {
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
      if((s->filled = getSymbol(2,MS_TRUE,MS_FALSE)) == -1)
	return(-1);
      break;
    case(FONT):
      if((s->font = getString()) == NULL) return(-1);
      break;  
    case(GAP):
      if((getInteger(&s->gap)) == -1) return(-1);
      break;
    case(POSITION):
      if((s->position = getSymbol(3, MS_UC,MS_CC,MS_LC)) == -1) 
	return(-1);
      break;
    case(IMAGE):
      if(msyylex() != MS_STRING) { /* get image location from next token */
	msSetError(MS_TYPEERR, "(%s):(%d)", "loadSymbol()", msyylineno);
	fclose(msyyin);
	return(-1);
      }
      
      if((stream = fopen(msyytext, "rb")) == NULL) {
	msSetError(MS_IOERR, "(%s):(%d)", "loadSymbol()", 
                   msyytext, msyylineno);
	fclose(msyyin);
	return(-1);
      }

      fread(bytes,8,1,stream); // read some bytes to try and identify the file
      rewind(stream); // reset the image for the readers
      if (memcmp(bytes,"GIF8",4)==0) {
#ifdef USE_GD_GIF
	s->img = gdImageCreateFromGif(stream);
#else
	msSetError(MS_MISCERR, "Unable to load GIF symbol.", "loadSymbol()");
	fclose(stream);
	return(-1);
#endif
      } else if (memcmp(bytes,PNGsig,8)==0) {
#ifdef USE_GD_PNG
	s->img = gdImageCreateFromPng(stream);
#else
	msSetError(MS_MISCERR, "Unable to load PNG symbol.", "loadSymbol()");
	fclose(stream);
	return(-1);
#endif
      }

      fclose(stream);
      
      if(s->img == NULL) {
	msSetError(MS_GDERR, NULL, "loadSymbol()");
	fclose(msyyin);
	return(-1);
      }
      break;
    case(LINECAP):
      if((s->linecap = getSymbol(4,MS_CJC_BUTT, MS_CJC_ROUND, MS_CJC_SQUARE, MS_CJC_TRIANGLE)) == -1)
        return(-1);
      break;
    case(LINEJOIN):
      if((s->linejoin = getSymbol(4,MS_CJC_NONE, MS_CJC_ROUND, MS_CJC_MITER, MS_CJC_BEVEL)) == -1)
        return(-1);
      break;
    case(LINEJOINMAXSIZE):
      if((getDouble(&s->linejoinmaxsize)) == -1) return(-1);
      break;
    case(NAME):
      if((s->name = getString()) == NULL) return(-1);
      break;
    case(POINTS):
      done = MS_FALSE;
      for(;;) {
	switch(msyylex()) { 
	case(END):
	  done = MS_TRUE;
	  break;
	case(MS_NUMBER):
	  s->points[s->numpoints].x = atof(msyytext); /* grab the x */
	  if(getDouble(&(s->points[s->numpoints].y)) == -1) return(-1); /* grab the y */
	  s->sizex = MS_MAX(s->sizex, s->points[s->numpoints].x);
	  s->sizey = MS_MAX(s->sizey, s->points[s->numpoints].y);	
	  s->numpoints++;
	  break;
	default:
	  msSetError(MS_TYPEERR, "(%s):(%d)", "loadSymbol()",  
                     msyytext, msyylineno); 	  
	  fclose(msyyin);
	  return(-1);
	}

	if(done == MS_TRUE)
	  break;
      }
      break;    
    case(STYLE):      
      done = MS_FALSE;
      for(;;) { /* read till the next END */
	switch(msyylex()) {  
	case(END):
	  if(s->stylelength < 2) {
	    msSetError(MS_SYMERR, "Not enough style elements. A minimum of 2 are required", "loadSymbol()");
	    return(-1);
	  }	  
	  done = MS_TRUE;
	  break;
	case(MS_NUMBER): /* read the style values */
	  if(s->stylelength == MS_MAXSTYLELENGTH) {
	    msSetError(MS_SYMERR, "Style too long.", "loadSymbol()");
	    return(-1);
	  }
	  s->style[s->stylelength] = atoi(msyytext);
	  s->stylelength++;
	  break;
	default:
	  msSetError(MS_TYPEERR, "(%s):(%d)", "loadSymbol()", 
                     msyytext, msyylineno); 	  
	  fclose(msyyin);
	  return(-1);
	}
	if(done == MS_TRUE)
	  break;
      }      
      break;
    case(TRANSPARENT):
      s->transparent = MS_TRUE;
      if(getInteger(&(s->transparentcolor)) == -1) return(-1);
      break;
    case(TYPE):
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
      if((s->type = getSymbol(6,MS_SYMBOL_VECTOR,MS_SYMBOL_ELLIPSE,MS_SYMBOL_PIXMAP,MS_SYMBOL_SIMPLE,MS_SYMBOL_TRUETYPE, MS_SYMBOL_CARTOLINE)) == -1)
	return(-1);	
#else
      if((s->type = getSymbol(5,MS_SYMBOL_VECTOR,MS_SYMBOL_ELLIPSE,MS_SYMBOL_PIXMAP,MS_SYMBOL_SIMPLE, MS_SYMBOL_CARTOLINE)) == -1)
	return(-1);
#endif
      break;
    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadSymbol()",
                 msyytext, msyylineno);
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
  char bytes[8];

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

  fread(bytes,8,1,stream); // read some bytes to try and identify the file
  rewind(stream); // reset the image for the readers
  if (memcmp(bytes,"GIF8",4)==0) {
#ifdef USE_GD_GIF
    symbolset->symbol[i].img = gdImageCreateFromGif(stream);
#else
    msSetError(MS_MISCERR, "Unable to load GIF symbol.", "msAddImageSymbol()");
    fclose(stream);
    return(-1);
#endif
  } else if (memcmp(bytes,PNGsig,8)==0) {
#ifdef USE_GD_PNG
    symbolset->symbol[i].img = gdImageCreateFromPng(stream);
#else
    msSetError(MS_MISCERR, "Unable to load PNG symbol.", "msAddImageSymbol()");
    fclose(stream);
    return(-1);
#endif
  }

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

  for(i=1; i<symbolset->numsymbols; i++)
    freeSymbol(&(symbolset->symbol[i]));

  /* no need to deal with fontset, it's a pointer */
}

void msInitSymbolSet(symbolSetObj *symbolset) 
{
  symbolset->filename = NULL;
  symbolset->numsymbols = 1; /* always 1 symbol */
  symbolset->imagecache = NULL;
  symbolset->imagecachesize = 0; /* 0 symbols in the cache */

  symbolset->fontset = NULL;
}

/*
** Load the symbols contained in the given file
*/
int msLoadSymbolSet(symbolSetObj *symbolset)
{
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
    msSetError(MS_IOERR, "(%s)", "msLoadSymbolFile()", symbolset->filename);
    return(-1);
  }

  getcwd(old_path, MS_PATH_LENGTH); /* save old working directory */
  symbol_path = getPath(symbolset->filename);
  chdir(symbol_path);
  free(symbol_path);

  msyylineno = 0; /* reset line counter */
  msyyrestart(msyyin); /* flush the scanner - there's a better way but this works for now */
  msyyfiletype = MS_FILE_SYMBOL;

  /*
  ** Read the symbol file
  */
  for(;;) {
    switch(msyylex()) {
    case(END):
    case(EOF):      
      status = 0;
      break;
    case(SYMBOL):
      if(symbolset->numsymbols == MS_MAXSYMBOLS) { 
	msSetError(MS_SYMERR, "Too many symbols defined.", "msLoadSymbolSet()");
	status = -1;      
      }
      if((loadSymbol(&(symbolset->symbol[symbolset->numsymbols])) == -1)) 
	status = -1;
      symbolset->numsymbols++;
      break;
    case(SYMBOLSET):
      break;
    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "msLoadSymbolFile()",
                 msyytext, msyylineno);      
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
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  int bbox[8];
  char *error=NULL;

#ifdef USE_GD_TTF
  error = gdImageStringTTF(NULL, bbox, 0, font, size, 0, 0, 0, character);
#else
  error = gdImageStringFT(NULL, bbox, 0, font, size, 0, 0, 0, character);
#endif
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
** Returns the size, in pixels, of a marker symbol defined for a specific class. Use for annotation
** layer collision avoidance.
*/
void msGetMarkerSize(symbolSetObj *symbolset, classObj *class, int *width, int *height)
{
  rectObj rect;
  char *font=NULL;

  if(class->symbol > symbolset->numsymbols ||
     class->symbol == -1) { /* no such symbol, 0 is OK */
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

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    font = msLookupHashTable(symbolset->fontset->fonts, symbolset->symbol[class->symbol].font);
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
    if(class->sizescaled == 1) {
      *width = symbolset->symbol[class->symbol].img->sx;
      *height = symbolset->symbol[class->symbol].img->sy;
    } else {
      *height = class->sizescaled;
      *width = MS_NINT((class->sizescaled/symbolset->symbol[class->symbol].img->sy) * symbolset->symbol[class->symbol].img->sx);
    }
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

/* ------------------------------------------------------------------------------- */
/*       Draw a line symbol of the specified size and color                        */
/* ------------------------------------------------------------------------------- */
void msDrawLineSymbol(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, int sy, int fc, int bc, double sz)
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
/*       Fill a circle with a shade symbol of the specified size and color       */
/* ------------------------------------------------------------------------------- */
void msCircleDrawShadeSymbol(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, int sy, int fc, int bc, int oc, double sz)
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
/*       Stroke an ellipse with a line symbol of the specified size and color      */
/* ------------------------------------------------------------------------------- */
void msCircleDrawLineSymbol(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, int sy, int fc, int bc, double sz)
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

#include <stdarg.h> /* variable number of function arguments support */

#include "map.h"
#include "mapfile.h"
#include "mapparser.h"

#ifdef USE_TTF
#include <gdcache.h>
#include <gdttf.h>
#include "freetype.h"
#endif

extern int yylex(); /* lexer globals */
extern void yyrestart();
extern double yynumber;
extern char *yytext;
extern int yylineno;
extern FILE *yyin;
extern int yyfiletype;

static void initSymbol(symbolObj *s)
{
  s->type = MS_SYMBOL_VECTOR;
  s->transparent = MS_FALSE;
  s->transparentcolor = 0;
  s->numarcs = 1; // always at least 1 arc
  s->numon[0] = 0;
  s->numoff[0] = 0;
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
    switch(yylex()) {
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
      if(yylex() != MS_STRING) { /* get image location from next token */
	msSetError(MS_TYPEERR, NULL, "loadSymbol()"); 
	sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
	fclose(yyin);
	return(-1);
      }
      
      if((stream = fopen(yytext, "rb")) == NULL) {
	msSetError(MS_IOERR, NULL, "loadSymbol()");
	sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
	fclose(yyin);
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
	fclose(yyin);
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
	switch(yylex()) { 
	case(END):
	  done = MS_TRUE;
	  break;
	case(MS_NUMBER):
	  s->points[i].x = atof(yytext); /* grab the x */
	  if(getDouble(&(s->points[i].y)) == -1) return(-1); /* grab the y */
	  s->sizex = MS_MAX(s->sizex, s->points[i].x);
	  s->sizey = MS_MAX(s->sizey, s->points[i].y);	
	  i++;
	  break;
	default:
	  msSetError(MS_TYPEERR, NULL, "loadSymbol()"); 
	  sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno); 
	  fclose(yyin);
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
	switch(yylex()) {  
	case(END):
	  done = MS_TRUE;
	  break;
	case(MS_NUMBER): /* read the style values */
	  s->offset[i] = atoi(yytext);
	  if(getInteger(&(s->numon[i])) == -1) return(-1);
	  if(getInteger(&(s->numoff[i])) == -1) return(-1);
	  i++;
	  break;
	default:
	  msSetError(MS_TYPEERR, NULL, "loadSymbol()"); 
	  sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno); 
	  fclose(yyin);
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
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      fclose(yyin);
      return(-1);
    } /* end switch */
  } /* end for */
}

/*
** Load the symbols contained in the given file
*/
int msLoadSymbolFile(symbolSetObj *symbolset)
{
  int n=1;
  char old_path[MS_PATH_LENGTH];
  char *symbol_path;
  int status=1;

  if(!symbolset) {
    msSetError(MS_SYMERR, "Symbol structure unallocated.", "msLoadSymbolFile()");
    return(-1);
  }

  if(!symbolset->filename)
    return(0);

  if(symbolset->numsymbols != 0) /* already loaded */
    return(0);

  /*
  ** Open the file
  */
  if((yyin = fopen(symbolset->filename, "r")) == NULL) {
    msSetError(MS_IOERR, NULL, "msLoadSymbolFile()");
    sprintf(ms_error.message, "(%s)", symbolset->filename);
    return(-1);
  }

  getcwd(old_path, MS_PATH_LENGTH); /* save old working directory */
  symbol_path = getPath(symbolset->filename);
  chdir(symbol_path);
  free(symbol_path);

  yylineno = 0; /* reset line counter */
  yyrestart(yyin); /* flush the scanner - there's a better way but this works for now */
  yyfiletype = MS_FILE_SYMBOL;

  /*
  ** Read the symbol file
  */
  for(;;) {
    switch(yylex()) {
    case(END):      
      symbolset->numsymbols = n;
      status = 0;
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "msLoadSymbolFile()");      
      status = -1;
      break;
    case(LINESET):
      symbolset->type = MS_LINESET;
      symbolset->symbol[0].type = MS_SYMBOL_STYLED; /* a simple line */
      break;
    case(MARKERSET):
      symbolset->type = MS_MARKERSET;
      symbolset->symbol[0].type = MS_SYMBOL_VECTOR; /* a simple point */
      symbolset->symbol[0].sizex = 1.0;
      symbolset->symbol[0].sizey = 1.0;
      break;
    case(SHADESET):
      symbolset->type = MS_SHADESET;
      symbolset->symbol[0].type = MS_SYMBOL_VECTOR; /* a solid fill */
      symbolset->symbol[0].sizex = 1.0;
      symbolset->symbol[0].sizey = 1.0;      
      break;
    case(SYMBOL):
      if(symbolset->type == -1) {
	msSetError(MS_MISCERR, NULL, "msLoadSymbolFile()");
	sprintf(ms_error.message, "Symbol type not set for %s.", symbolset->filename);
	status = -1;
      }
      if((loadSymbol(&(symbolset->symbol[n])) == -1)) 
	status = -1;
      n++;
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "msLoadSymbolFile()");
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      status = -1;
    } /* end switch */

    if(status != 1) break;
  } /* end for */

  yyfiletype = MS_FILE_DEFAULT;
  fclose(yyin);
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
void msDrawShadeSymbol(mapObj *map, gdImagePtr img, shapeObj *p, classObj *class)
{
  int i;
  gdPoint oldpnt,newpnt;
  gdPoint sPoints[MS_MAXVECTORPOINTS];
  gdImagePtr tile;
  int x,y;
  int tile_bg=-1, tile_fg=-1; /* colors (background and foreground) */
  
  int oc,bc,fc;
  int s;
  double sz,scale=1.0;
  
  int bbox[8];
  rectObj rect;
  char *font=NULL;
  
  symbolSetObj *shadeset;

  shadeset = &(map->shadeset);

  if(class->symbol > shadeset->numsymbols) /* no such symbol, 0 is OK */
    return;

  if(class->color >= gdImageColorsTotal(img)) /* invalid color, -1 is valid */
    return;

  if(class->size < 1) /* size too small */
    return;
  
  if(p->numlines <= 0)
    return;

  oc = class->outlinecolor;
  bc = class->backgroundcolor;
  fc = class->color;
  s = class->symbol;
  sz = class->size;

  if(s == 0) { /* simply draw a single pixel of the specified color */
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

  switch(shadeset->symbol[s].type) {
  case(MS_SYMBOL_TRUETYPE):    
    
#ifdef USE_TTF
    font = msLookupHashTable(map->fontset.fonts, shadeset->symbol[s].font);
    if(!font) return;

    if(getCharacterSize(shadeset->symbol[s].character, sz, font, &rect) == -1) return;
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
    imageStringTTF(tile, bbox, shadeset->symbol[s].antialias*tile_fg, font, sz, 0, x, y, shadeset->symbol[s].character, '\n');
    
    gdImageSetTile(img, tile);
    msImageFilledPolygon(img,p,gdTiled);
    if(oc>-1)
      msImagePolyline(img, p, oc);
    gdImageDestroy(tile);
#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    
    gdImageSetTile(img, shadeset->symbol[s].img);
    msImageFilledPolygon(img, p, gdTiled);
    if(oc>-1)
      msImagePolyline(img, p, oc);

    break;
  case(MS_SYMBOL_ELLIPSE):    
   
    scale = sz/shadeset->symbol[s].sizey; // sz ~ height in pixels
    x = MS_NINT(shadeset->symbol[s].sizex*scale)+1;
    y = MS_NINT(shadeset->symbol[s].sizey*scale)+1;

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
    gdImageArc(tile, x, y, MS_NINT(scale*shadeset->symbol[s].points[0].x), MS_NINT(scale*shadeset->symbol[s].points[0].y), 0, 360, tile_fg);
    if(shadeset->symbol[s].filled)
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

    scale = sz/shadeset->symbol[s].sizey; // sz ~ height in pixels
    x = MS_NINT(shadeset->symbol[s].sizex*scale)+1;    
    y = MS_NINT(shadeset->symbol[s].sizey*scale)+1;

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
    if(shadeset->symbol[s].filled) {

      for(i=0;i < shadeset->symbol[s].numpoints;i++) {
	sPoints[i].x = MS_NINT(scale*shadeset->symbol[s].points[i].x);
	sPoints[i].y = MS_NINT(scale*shadeset->symbol[s].points[i].y);
      }
      gdImageFilledPolygon(tile, sPoints, shadeset->symbol[s].numpoints, tile_fg);      

    } else  { /* shade is a vector drawing */
      
      oldpnt.x = MS_NINT(scale*shadeset->symbol[s].points[0].x); /* convert first point in shade smbol */
      oldpnt.y = MS_NINT(scale*shadeset->symbol[s].points[0].y);

      /* step through the shade sy */
      for(i=1;i < shadeset->symbol[s].numpoints;i++) {
	if((shadeset->symbol[s].points[i].x < 0) && (shadeset->symbol[s].points[i].y < 0)) {
	  oldpnt.x = MS_NINT(scale*shadeset->symbol[s].points[i].x);
	  oldpnt.y = MS_NINT(scale*shadeset->symbol[s].points[i].y);
	} else {
	  if((shadeset->symbol[s].points[i-1].x < 0) && (shadeset->symbol[s].points[i-1].y < 0)) { /* Last point was PENUP, now a new beginning */
	    oldpnt.x = MS_NINT(scale*shadeset->symbol[s].points[i].x);
	    oldpnt.y = MS_NINT(scale*shadeset->symbol[s].points[i].y);
	  } else {
	    newpnt.x = MS_NINT(scale*shadeset->symbol[s].points[i].x);
	    newpnt.y = MS_NINT(scale*shadeset->symbol[s].points[i].y);
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
void msGetMarkerSize(mapObj *map, classObj *class, int *width, int *height)
{
  rectObj rect;
  char *font=NULL;

  symbolSetObj *markerset;
  
  markerset = &(map->markerset);

  if(class->symbol > markerset->numsymbols) { /* no such symbol, 0 is OK */
    *width = 0;
    *height = 0;
    return;
  }

  if(class->symbol == 0) { /* single point */
    *width = 1;
    *height = 1;
    return;
  }

  switch(markerset->symbol[class->symbol].type) {  
  case(MS_SYMBOL_TRUETYPE):

#ifdef USE_TTF
    font = msLookupHashTable(map->fontset.fonts, markerset->symbol[class->symbol].font);
    if(!font) return;

    if(getCharacterSize(markerset->symbol[class->symbol].character, class->size, font, &rect) == -1) return;

    *width = rect.maxx - rect.minx;
    *height = rect.maxy - rect.miny;
#else
    *width = 0;
    *height = 0;
#endif
    break;

  case(MS_SYMBOL_PIXMAP):
    *width = markerset->symbol[class->symbol].img->sx;
    *height = markerset->symbol[class->symbol].img->sy;
    break;
  default: /* vector and ellipses, scalable */
    if(class->size > 0) {
      *height = class->size;
      *width = MS_NINT((class->size/markerset->symbol[class->symbol].sizey) * markerset->symbol[class->symbol].sizex);
    } else { /* use symbol defaults */
      *height = markerset->symbol[class->symbol].sizey;
      *width = markerset->symbol[class->symbol].sizex;
    }
    break;
  }  

  return;
}

/* ------------------------------------------------------------------------------- */
/*       Draw a single marker symbol of the specified size and color               */
/* ------------------------------------------------------------------------------- */
void msDrawMarkerSymbol(mapObj *map, gdImagePtr img, pointObj *p, classObj *class)
{
  int offset_x, offset_y, x, y;
  int j;
  gdPoint oldpnt,newpnt;
  gdPoint mPoints[MS_MAXVECTORPOINTS];

  gdImagePtr tmp;
  int tmp_fc=-1, tmp_bc, tmp_oc=-1;

  int oc,fc;
  int s;
  double sz,scale=1.0;

  int bbox[8];
  rectObj rect;
  char *font=NULL;

  symbolSetObj *markerset;
  
  markerset = &(map->markerset);

  if(class->symbol > markerset->numsymbols) /* no such symbol, 0 is OK */
    return;

  if(class->color >= gdImageColorsTotal(img)) /* invalid color, -1 is valid */
    return;

  if(class->size < 1) /* size too small */
    return;

  oc = class->outlinecolor;
  fc = class->color;
  s = class->symbol;
  sz = class->size;

  if(s == 0 && fc >= 0) { /* simply draw a single pixel of the specified color */
    gdImageSetPixel(img, p->x, p->y, fc);
    return;
  }

  switch(markerset->symbol[s].type) {  
  case(MS_SYMBOL_TRUETYPE):    

#ifdef USE_TTF
    font = msLookupHashTable(map->fontset.fonts, markerset->symbol[s].font);
    if(!font) return;

    if(getCharacterSize(markerset->symbol[s].character, sz, font, &rect) == -1) return;

    x = p->x - (rect.maxx - rect.minx)/2 - rect.minx;
    y = p->y - rect.maxy + (rect.maxy - rect.miny)/2;  

    imageStringTTF(img, bbox, markerset->symbol[s].antialias*fc, font, sz, 0, x, y, markerset->symbol[s].character, '\n');
#endif

    break;
  case(MS_SYMBOL_PIXMAP):
    offset_x = MS_NINT(p->x - .5*markerset->symbol[s].img->sx);
    offset_y = MS_NINT(p->y - .5*markerset->symbol[s].img->sy);
    gdImageCopy(img, markerset->symbol[s].img, offset_x, offset_y, 0, 0, markerset->symbol[s].img->sx, markerset->symbol[s].img->sy);
    break;
  case(MS_SYMBOL_ELLIPSE):
 
    scale = sz/markerset->symbol[s].sizey;
    x = MS_NINT(markerset->symbol[s].sizex*scale)+1;
    y = MS_NINT(markerset->symbol[s].sizey*scale)+1;

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
      gdImageArc(tmp, x, y, MS_NINT(scale*markerset->symbol[s].points[0].x), MS_NINT(scale*markerset->symbol[s].points[0].y), 0, 360, tmp_oc);
      if(markerset->symbol[s].filled && tmp_fc >= 0)
	gdImageFillToBorder(tmp, x, y, tmp_oc, tmp_fc);
    } else {
      if(tmp_fc >= 0) {
	gdImageArc(tmp, x, y, MS_NINT(scale*markerset->symbol[s].points[0].x), MS_NINT(scale*markerset->symbol[s].points[0].y), 0, 360, tmp_fc);
	if(markerset->symbol[s].filled)
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

    scale = sz/markerset->symbol[s].sizey;

    offset_x = MS_NINT(p->x - scale*.5*markerset->symbol[s].sizex);
    offset_y = MS_NINT(p->y - scale*.5*markerset->symbol[s].sizey);
    
    if(markerset->symbol[s].filled) { /* if filled */
      for(j=0;j < markerset->symbol[s].numpoints;j++) {
	mPoints[j].x = MS_NINT(scale*markerset->symbol[s].points[j].x + offset_x);
	mPoints[j].y = MS_NINT(scale*markerset->symbol[s].points[j].y + offset_y);
      }
      if(fc >= 0)
	gdImageFilledPolygon(img, mPoints, markerset->symbol[s].numpoints, fc);
      if(oc >= 0)
	gdImagePolygon(img, mPoints, markerset->symbol[s].numpoints, oc);
      
    } else  { /* NOT filled */     

      if(fc < 0) return;
      
      oldpnt.x = MS_NINT(scale*markerset->symbol[s].points[0].x + offset_x); /* convert first point in marker s */
      oldpnt.y = MS_NINT(scale*markerset->symbol[s].points[0].y + offset_y);
      
      for(j=1;j < markerset->symbol[s].numpoints;j++) { /* step through the marker s */
	if((markerset->symbol[s].points[j].x < 0) && (markerset->symbol[s].points[j].x < 0)) {
	  oldpnt.x = MS_NINT(scale*markerset->symbol[s].points[j].x + offset_x);
	  oldpnt.y = MS_NINT(scale*markerset->symbol[s].points[j].y + offset_y);
	} else {
	  if((markerset->symbol[s].points[j-1].x < 0) && (markerset->symbol[s].points[j-1].y < 0)) { /* Last point was PENUP, now a new beginning */
	    oldpnt.x = MS_NINT(scale*markerset->symbol[s].points[j].x + offset_x);
	    oldpnt.y = MS_NINT(scale*markerset->symbol[s].points[j].y + offset_y);
	  } else {
	    newpnt.x = MS_NINT(scale*markerset->symbol[s].points[j].x + offset_x);
	    newpnt.y = MS_NINT(scale*markerset->symbol[s].points[j].y + offset_y);
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
void msDrawLineSymbol(mapObj *map, gdImagePtr img, shapeObj *p, classObj *class)
{
  int i, j;
  symbolObj *sym;
  int styleDashed[100];
  int x, y;
  int brush_bc, brush_fc;
  gdImagePtr brush=NULL;
  gdPoint points[MS_MAXVECTORPOINTS];
  int fc, bc, size;
  double scale=1.0;
  
  symbolSetObj *lineset;
  
  lineset = &(map->lineset);

  if(p->numlines <= 0)
    return;

  if(class->symbol > lineset->numsymbols) /* no such symbol, 0 is OK */
    return;

  if((class->color < 0) || (class->color >= gdImageColorsTotal(img))) /* invalid color */
    return;

  if(class->size < 1) /* size too small */
    return;

  fc = class->color;
  bc = class->backgroundcolor;
  size = class->size;

  if(class->symbol == 0) { /* just draw a single width line */
    msImagePolyline(img, p, fc);
    return;
  }

  sym = &(lineset->symbol[class->symbol]);

  switch(sym->type) {
  case(MS_SYMBOL_STYLED):
    if(bc == -1) bc = gdTransparent;
    break;
  case(MS_SYMBOL_ELLIPSE):
     
    scale = (size)/sym->sizey;
    x = MS_NINT(sym->sizex*scale);    
    y = MS_NINT(sym->sizey*scale);
    
    if((x < 2) && (y < 2)) break;
    
    // create the brush image
    brush = gdImageCreate(x, y);
    brush_bc = gdImageColorAllocate(brush,gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));    
    gdImageColorTransparent(brush,0);
    brush_fc = gdImageColorAllocate(brush, gdImageRed(img,fc), gdImageGreen(img,fc), gdImageBlue(img,fc));
    
    x = MS_NINT(brush->sx/2); // center the ellipse
    y = MS_NINT(brush->sy/2);

    // draw in the brush image
    gdImageArc(brush, x, y, MS_NINT(scale*sym->points[0].x), MS_NINT(scale*sym->points[0].y), 0, 360, brush_fc);
    if(sym->filled)
      gdImageFillToBorder(brush, x, y, brush_fc, brush_fc);

    gdImageSetBrush(img, brush);
    fc = 1; bc = 0;
    break;
  case(MS_SYMBOL_PIXMAP):
    gdImageSetBrush(img, sym->img);
    fc = 1; bc = 0;
    break;
  case(MS_SYMBOL_VECTOR):
    scale = size/sym->sizey;
    x = MS_NINT(sym->sizex*scale);    
    y = MS_NINT(sym->sizey*scale);

    if((x < 2) && (y < 2)) break;

    // create the brush image
    brush = gdImageCreate(x, y);
    if(bc >= 0)
      brush_bc = gdImageColorAllocate(brush, gdImageRed(img,bc), gdImageGreen(img,bc), gdImageBlue(img,bc));
    else {
      brush_bc = gdImageColorAllocate(brush, gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));
      gdImageColorTransparent(brush,0);
    }
    brush_fc = gdImageColorAllocate(brush,gdImageRed(img,fc), gdImageGreen(img,fc), gdImageBlue(img,fc));

    // draw in the brush image 
    for(i=0;i < sym->numpoints;i++) {
      points[i].x = MS_NINT(scale*sym->points[i].x);
      points[i].y = MS_NINT(scale*sym->points[i].y);
    }
    gdImageFilledPolygon(brush, points, sym->numpoints, brush_fc);

    gdImageSetBrush(img, brush);
    fc = 1; bc = 0;
    break;
  }

  for(i=0;i<sym->numarcs;i++) { // Draw each line in the style
    if(sym->numon[i] > 0) {
      for(j=0; j<sym->numon[i]+sym->numoff[i]; j++) {
	if(j<sym->numon[i])
	  styleDashed[j] = fc;
	else
	  styleDashed[j] = bc;
      }
      gdImageSetStyle(img, styleDashed, j);    

      if(!brush && !sym->img)
	msImagePolylineOffset(img, p, MS_NINT(scale*sym->offset[i]), gdStyled);
      else 
	msImagePolylineOffset(img, p, MS_NINT(scale*sym->offset[i]), gdStyledBrushed);
    } else {
      if(!brush && !sym->img)
	msImagePolylineOffset(img, p, MS_NINT(scale*sym->offset[i]), fc);
      else
	msImagePolylineOffset(img, p, MS_NINT(scale*sym->offset[i]), gdBrushed);
    }
  }

  if(brush)
    gdImageDestroy(brush);

  return;
}

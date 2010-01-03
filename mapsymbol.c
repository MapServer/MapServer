/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  symbolObj related functions.
 * Author:   Steve Lime and the MapServer team.
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
 *****************************************************************************/

#include <stdarg.h> /* variable number of function arguments support */
#include <time.h> /* since the parser handles time/date we need this */

#include "mapserver.h"
#include "mapfile.h"
#include "mapcopy.h"
#include "mapthread.h"

MS_CVSID("$Id$")

extern int msyylex(void); /* lexer globals */
extern void msyyrestart(FILE *);
extern double msyynumber;
extern char *msyytext;
extern int msyylineno;
extern FILE *msyyin;

extern unsigned char PNGsig[8];
extern unsigned char JPEGsig[3];

/*
** Symbol to string static arrays needed for writing map files.
** Must be kept in sync with enumerations and defines found in mapserver.h.
*/
extern char *msPositionsText[]; /* Defined in mapfile.c */
static char *msCapsJoinsCorners[7]={"NONE", "BEVEL", "BUTT", "MITER", "ROUND", "SQUARE", "TRIANGLE"};

void freeImageCache(struct imageCacheObj *ic)
{
  if(ic) {
    freeImageCache(ic->next); /* free any children */
   	gdImageDestroy(ic->img);
    free(ic);  
  }
  return;
}

int msGetCharacterSize(char *character, int size, char *font, rectObj *rect) {
#ifdef USE_GD_FT
  int bbox[8];
  char *error=NULL;


  error = gdImageStringFT(NULL, bbox, 0, font, size, 0, 0, 0, character);

  if(error) {
    msSetError(MS_TTFERR, error, "msGetCharacterSize()");
    return(MS_FAILURE);
  }    
  
  rect->minx = bbox[0];
  rect->miny = bbox[5];
  rect->maxx = bbox[2];
  rect->maxy = bbox[1];

  return(MS_SUCCESS);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msGetCharacterSize()");
  return(MS_FAILURE);
#endif
}

/*
** msSymbolGetDefaultSize()
**
** Return the default size of a symbol.
** Note: The size of a symbol is the height. Everywhere in the code, the width
** is adjusted to the size that becomes the height.
** See mapgd.c // size ~ height in pixels
*/
double msSymbolGetDefaultSize(symbolObj *s) {
  double size;

  if(s == NULL)
      return 1;

  switch(s->type) {  
    case(MS_SYMBOL_TRUETYPE):
      size = 1;
      break;
    case(MS_SYMBOL_PIXMAP):
      size = (double)s->img->sy;
      break;
    default: /* vector and ellipses, scalable */
      size = s->sizey;
      break;
  }

  if(size <= 0)
      return 1;

  return size;
}

void initSymbol(symbolObj *s)
{
  MS_REFCNT_INIT(s);
  s->type = MS_SYMBOL_VECTOR;
  s->transparent = MS_FALSE;
  s->transparentcolor = 0;
  s->patternlength = 0; /* solid line */
  s->sizex = 1;
  s->sizey = 1;
  s->filled = MS_FALSE;
  s->numpoints=0;
  s->img = NULL;
  s->renderer_cache=NULL;
  s->renderer=NULL;
  s->pixmap_buffer=NULL;
  s->imagepath = NULL;
  s->name = NULL;
  s->gap = 0;
  s->inmapfile = MS_FALSE;
  s->antialias = MS_FALSE;
  s->font = NULL;
  s->full_font_path = NULL;
  s->character = NULL;
  s->position = MS_CC;

  s->linecap = MS_CJC_BUTT;
  s->linejoin = MS_CJC_NONE;
  s->linejoinmaxsize = 3;
}

int msFreeSymbol(symbolObj *s) {
  if(!s) return MS_FAILURE;
  if( MS_REFCNT_DECR_IS_NOT_ZERO(s) ) {
  	return MS_FAILURE;
  }
  
  if(s->name) free(s->name);
  if(s->img) gdImageDestroy(s->img);
  if(s->renderer!=NULL) {
	  s->renderer->freeSymbol(s);
  }
#ifdef USE_AGG
  else
	  if(s->renderer_cache) msFreeSymbolCacheAGG(s->renderer_cache);
#endif
  if(s->pixmap_buffer) {
      msFreeRasterBuffer(s->pixmap_buffer);
      free(s->pixmap_buffer);
  }

  if(s->font) free(s->font);
  msFree(s->full_font_path);
  if(s->imagepath) free(s->imagepath);
  if(s->character) free(s->character);
  
  return MS_SUCCESS;
}

int loadSymbol(symbolObj *s, char *symbolpath)
{
  int done=MS_FALSE;
  FILE *stream;
  char bytes[8], szPath[MS_MAXPATHLEN];
  gdIOCtx *ctx;
  
  initSymbol(s);

  for(;;) {
    switch(msyylex()) {
    case(ANTIALIAS):
      if((s->antialias = getSymbol(2,MS_TRUE,MS_FALSE)) == -1)
	return(-1);
      break;    
    case(CHARACTER):
      if(getString(&s->character) == MS_FAILURE) return(-1);
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
      if(getString(&s->font) == MS_FAILURE) return(-1);
      break;  
    case(GAP):
      if((getInteger(&s->gap)) == -1) return(-1);
      break;
    case(POSITION):
      /* if((s->position = getSymbol(3, MS_UC,MS_CC,MS_LC)) == -1)  */
      /* return(-1); */
      if((s->position = getSymbol(9, MS_UL,MS_UC,MS_UR,MS_CL,MS_CC,MS_CR,MS_LL,MS_LC,MS_LR)) == -1) 
	return(-1);
      break;
    case(IMAGE):
      if(msyylex() != MS_STRING) { /* get image location from next token */
	msSetError(MS_TYPEERR, "Parsing error near (%s):(line %d)", "loadSymbol()", msyytext, msyylineno);
	return(-1);
      }
      
      if((stream = fopen(msBuildPath(szPath, symbolpath, msyytext), "rb")) == NULL)
      {
	msSetError(MS_IOERR, "Parsing error near (%s):(line %d)", "loadSymbol()", 
                   msyytext, msyylineno);
	return(-1);
      }
      
      /* Set imagepath */
      s->imagepath = strdup(msyytext);

      fread(bytes,8,1,stream); /* read some bytes to try and identify the file */
      rewind(stream); /* reset the image for the readers */
      if (memcmp(bytes,"GIF8",4)==0) 
      {
#ifdef USE_GD_GIF
        ctx = msNewGDFileCtx(stream);
	s->img = gdImageCreateFromGifCtx(ctx);
        ctx->gd_free(ctx);
#else
	msSetError(MS_MISCERR, "Unable to load GIF symbol.", "loadSymbol()");
	fclose(stream);
	return(-1);
#endif
      } 
      else if (memcmp(bytes,PNGsig,8)==0) 
      {
#ifdef USE_GD_PNG
        ctx = msNewGDFileCtx(stream);
	s->img = gdImageCreateFromPngCtx(ctx);
        ctx->gd_free(ctx);
#else
	msSetError(MS_MISCERR, "Unable to load PNG symbol.", "loadSymbol()");
	fclose(stream);
	return(-1);
#endif
      }

      fclose(stream);
      
      if(s->img == NULL) {
	msSetError(MS_GDERR, NULL, "loadSymbol()");	
	return(-1);
      }
      s->sizex = s->img->sx;
      s->sizey = s->img->sy;
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
      if(getString(&s->name) == MS_FAILURE) return(-1);
      break;
    case(STYLE): /* depricated */
      /* TODO: output warning */
    case(PATTERN):
      done = MS_FALSE;
      for(;;) { /* read till the next END */
	switch(msyylex()) {  
	case(END):
	  if(s->patternlength < 2) {
	    msSetError(MS_SYMERR, "Not enough pattern elements. A minimum of 2 are required", "loadSymbol()");
	    return(-1);
	  }	  
	  done = MS_TRUE;
	  break;
	case(MS_NUMBER): /* read the pattern values */
	  if(s->patternlength == MS_MAXPATTERNLENGTH) {
	    msSetError(MS_SYMERR, "Pattern too long.", "loadSymbol()");
	    return(-1);
	  }
	  s->pattern[s->patternlength] = atoi(msyytext);
	  s->patternlength++;
	  break;
	default:
	  msSetError(MS_TYPEERR, "Parsing error near (%s):(line %d)", "loadSymbol()", msyytext, msyylineno);
	  return(-1);
	}
	if(done == MS_TRUE)
	  break;
      }      
      break;
    case(POINTS):
      done = MS_FALSE;
      s->sizex = 0;
      s->sizey = 0;
      for(;;) {
	switch(msyylex()) { 
	case(END):
	  done = MS_TRUE;
	  break;
	case(MS_NUMBER):
	  s->points[s->numpoints].x = atof(msyytext); /* grab the x */
	  if(getDouble(&(s->points[s->numpoints].y)) == -1) return(-1); /* grab the y */
	  if(s->points[s->numpoints].x!=-99) {
	  s->sizex = MS_MAX(s->sizex, s->points[s->numpoints].x);
	  s->sizey = MS_MAX(s->sizey, s->points[s->numpoints].y);
	  }
	  s->numpoints++;
	  break;
	default:
	  msSetError(MS_TYPEERR, "Parsing error near (%s):(line %d)", "loadSymbol()", msyytext, msyylineno);
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
#ifdef USE_GD_FT
      if((s->type = getSymbol(7,MS_SYMBOL_VECTOR,MS_SYMBOL_ELLIPSE,MS_SYMBOL_PIXMAP,MS_SYMBOL_SIMPLE,MS_TRUETYPE,MS_SYMBOL_CARTOLINE,MS_SYMBOL_HATCH)) == -1)
	return(-1);	
#else
      if((s->type = getSymbol(6,MS_SYMBOL_VECTOR,MS_SYMBOL_ELLIPSE,MS_SYMBOL_PIXMAP,MS_SYMBOL_SIMPLE,MS_SYMBOL_CARTOLINE,MS_SYMBOL_HATCH)) == -1)
	return(-1);
#endif
      if(s->type == MS_TRUETYPE) /* TrueType keyword is valid several place in map files and symbol files, this simplifies the lexer */
	s->type = MS_SYMBOL_TRUETYPE;
      break;
    default:
      msSetError(MS_IDENTERR, "Parsing error near (%s):(line %d)", "loadSymbol()", msyytext, msyylineno);
      return(-1);
    } /* end switch */
  } /* end for */
}

void writeSymbol(symbolObj *s, FILE *stream)
{
  int i;

  if(s->inmapfile != MS_TRUE) return;

  fprintf(stream, "  SYMBOL\n");
  if(s->name != NULL) fprintf(stream, "    NAME \"%s\"\n", s->name);
  
  switch (s->type) {
  case(MS_SYMBOL_HATCH):
    fprintf(stream, "    TYPE HATCH\n");
    break;
  case(MS_SYMBOL_PIXMAP):
    fprintf(stream, "    TYPE PIXMAP\n");
    if(s->imagepath != NULL) fprintf(stream, "    IMAGE \"%s\"\n", s->imagepath);
    if (s->gap != 0)
      fprintf(stream, "    GAP %d\n", s->gap);
    fprintf(stream, "    TRANSPARENT %d\n", s->transparentcolor);
    break;
  case(MS_SYMBOL_TRUETYPE):
    fprintf(stream, "    TYPE TRUETYPE\n");
    if(s->antialias == MS_TRUE) fprintf(stream, "    ANTIALIAS TRUE\n");
    if (s->character != NULL) fprintf(stream, "    CHARACTER \"%s\"\n", s->character);
    fprintf(stream, "    GAP %d\n", s->gap);
    if (s->font != NULL) fprintf(stream, "    FONT \"%s\"\n", s->font);
    fprintf(stream, "    POSITION %s\n", msPositionsText[s->position - MS_UL]);
    break;
  case(MS_SYMBOL_CARTOLINE):
    fprintf(stream, "    TYPE CARTOLINE\n");
    fprintf(stream, "    LINECAP %s\n", msCapsJoinsCorners[s->linecap]);
    fprintf(stream, "    LINEJOIN %s\n", msCapsJoinsCorners[s->linejoin]);
    fprintf(stream, "    LINEJOINMAXSIZE %g\n", s->linejoinmaxsize);
    break;
  default:
    if(s->type == MS_SYMBOL_ELLIPSE)
      fprintf(stream, "    TYPE ELLIPSE\n");
    else if(s->type == MS_SYMBOL_VECTOR)
      fprintf(stream, "    TYPE VECTOR\n");
    else
      fprintf(stream, "    TYPE SIMPLE\n");
    
    if(s->filled == MS_TRUE) fprintf(stream, "    FILLED TRUE\n");
    
    /* POINTS */
    if(s->numpoints != 0) {
      fprintf(stream, "    POINTS\n");
      for(i=0; i<s->numpoints; i++) {
	fprintf(stream, "      %g %g\n", s->points[i].x, s->points[i].y);
      }
      fprintf(stream, "    END\n");
    }

    /* PATTERN */
    if(s->patternlength != 0) {
      fprintf(stream, "    PATTERN\n     ");
      for(i=0; i<s->patternlength; i++) {
	fprintf(stream, " %d", s->pattern[i]);
      }
      fprintf(stream, "\n    END\n");
    }
    break;
  }
      
  fprintf(stream, "  END\n\n");
}

/*
** Little helper function to allow us to build symbol files on-the-fly 
** from just a file name.
**
** Returns the symbol index or -1 if it could not be added.
*/
int msAddImageSymbol(symbolSetObj *symbolset, char *filename) 
{
  FILE *stream;
  int i;
  char bytes[8], szPath[MS_MAXPATHLEN];

  if(!symbolset) {
    msSetError(MS_SYMERR, "Symbol structure unallocated.", "msAddImageSymbol()");
    return(-1);
  }

  if(!filename || strlen(filename) == 0) return(-1);

  /* Allocate/init memory for new symbol if needed */
  if (msGrowSymbolSet(symbolset) == NULL)
      return -1;
  
  if(symbolset->map) {
    if((stream = fopen(msBuildPath(szPath, symbolset->map->mappath, filename), "rb")) == NULL) {
      msSetError(MS_IOERR, "Error opening image file %s.", "msAddImageSymbol()", szPath);
      return(-1);
    }
  } else {
    if((stream = fopen(msBuildPath(szPath, NULL, filename), "rb")) == NULL) {
      msSetError(MS_IOERR, "Error opening image file %s.", "msAddImageSymbol()", szPath);
      return(-1);
    }
  }

  i = symbolset->numsymbols;  

  fread(bytes,8,1,stream); /* read some bytes to try and identify the file */
  rewind(stream); /* reset the image for the readers */
  if(memcmp(bytes,"GIF8",4)==0) {
#ifdef USE_GD_GIF
    gdIOCtx *ctx;
    ctx = msNewGDFileCtx(stream);
    symbolset->symbol[i]->img = gdImageCreateFromGifCtx(ctx);
    ctx->gd_free(ctx);
#else
    msSetError(MS_MISCERR, "Unable to load GIF symbol.", "msAddImageSymbol()");
    fclose(stream);
    return(-1);
#endif
  } else if (memcmp(bytes,PNGsig,8)==0) {
#ifdef USE_GD_PNG
    gdIOCtx *ctx;
    ctx = msNewGDFileCtx(stream);
    symbolset->symbol[i]->img = gdImageCreateFromPngCtx(ctx);
    ctx->gd_free(ctx);
#else
    msSetError(MS_MISCERR, "Unable to load PNG symbol.", "msAddImageSymbol()");
    fclose(stream);
    return(-1);
#endif
  }

  fclose(stream);
  
  if(!symbolset->symbol[i]->img) {
    msSetError(MS_GDERR, NULL, "msAddImageSymbol()");
    return(-1);
  }

  symbolset->symbol[i]->name = strdup(filename);
  symbolset->symbol[i]->imagepath = strdup(filename);
  symbolset->symbol[i]->type = MS_SYMBOL_PIXMAP;
  symbolset->symbol[i]->sizex = symbolset->symbol[i]->img->sx;
  symbolset->symbol[i]->sizey = symbolset->symbol[i]->img->sy;
  symbolset->numsymbols++;

  return(i);
}

int msFreeSymbolSet(symbolSetObj *symbolset)
{
  int i;

  freeImageCache(symbolset->imagecache);
  for(i=0; i<symbolset->numsymbols; i++) {
	  if (symbolset->symbol[i]!=NULL) {
		  if ( msFreeSymbol((symbolset->symbol[i])) == MS_SUCCESS ) {
			  msFree(symbolset->symbol[i]);
			  symbolset->symbol[i]=NULL;
		  }
	  }
  }
  msFree(symbolset->symbol);

  /* no need to deal with fontset, it's a pointer */
  return MS_SUCCESS;
}

void msInitSymbolSet(symbolSetObj *symbolset)
{
  symbolset->filename = NULL;

  symbolset->imagecache = NULL;
  symbolset->imagecachesize = 0; /* 0 symbols in the cache */

  symbolset->fontset = NULL;
  symbolset->map = NULL;

  symbolset->numsymbols = 0;
  symbolset->maxsymbols = 0;
  symbolset->symbol = NULL;

  /* Alloc symbol[] array and ensure there is at least 1 symbol:
   * symbol 0 which is the default symbol with all default params.
   */
  if (msGrowSymbolSet(symbolset) == NULL)
      return; /* alloc failed */

  /* Just increment numsymbols to reserve symbol 0.
   * initSymbol() has already been called
   */
  symbolset->numsymbols = 1;

}


/*
** Ensure there is at least one free entry in the symbol array of this
** symbolSetObj. Grow the allocated symbol[] array if necessary and 
** allocate a new symbol for symbol[numsymbols] if there is not already one
** and call initSymbol() on it.
**
** This function is safe to use for the initial allocation of the symbol[]
** array as well (i.e. when maxsymbols==0 and symbol==NULL)
**
** Returns a reference to the new symbolObj on success, NULL on error.
*/
symbolObj *msGrowSymbolSet( symbolSetObj *symbolset )
{
    /* Do we need to increase the size of symbol[] by MS_SYMBOL_ALLOCSIZE? */
    if (symbolset->numsymbols == symbolset->maxsymbols) {
        int i;
        if (symbolset->maxsymbols == 0) {
            /* Initial allocation of array */
            symbolset->maxsymbols += MS_SYMBOL_ALLOCSIZE;
            symbolset->numsymbols = 0;
            symbolset->symbol = (symbolObj**)malloc(symbolset->maxsymbols*sizeof(symbolObj*));
        } else {
            /* realloc existing array */
            symbolset->maxsymbols += MS_SYMBOL_ALLOCSIZE;
            symbolset->symbol = (symbolObj**)realloc(symbolset->symbol,
                                                     symbolset->maxsymbols*sizeof(symbolObj*));
        }

        if (symbolset->symbol == NULL) {
            msSetError(MS_MEMERR, "Failed to allocate memory for symbol array.", "msGrowSymbolSet()");
            return NULL;
        }

        for(i=symbolset->numsymbols; i<symbolset->maxsymbols; i++)
            symbolset->symbol[i] = NULL;
    }

    if (symbolset->symbol[symbolset->numsymbols]==NULL) {
        symbolset->symbol[symbolset->numsymbols]=(symbolObj*)malloc(sizeof(symbolObj));
        if (symbolset->symbol[symbolset->numsymbols]==NULL) {
          msSetError(MS_MEMERR, "Failed to allocate memory for a symbolObj", "msGrowSymbolSet()");
          return NULL;
        }
    }

    /* Always call initSymbol() even if we didn't allocate a new symbolObj
     * Since it's possible to dynamically remove/reuse symbols */
    initSymbol(symbolset->symbol[symbolset->numsymbols]);

    return symbolset->symbol[symbolset->numsymbols];
}



/* ---------------------------------------------------------------------------
   msLoadSymbolSet and loadSymbolSet

   msLoadSymbolSet wraps calls to loadSymbolSet with mutex acquisition and
   release.  It should be used everywhere outside the mapfile loading
   phase of an application.  loadSymbolSet should only be used when a mutex
   exists.  It does not check for existence of a mutex!

   See bug 339 for more details -- SG.
   ------------------------------------------------------------------------ */

int msLoadSymbolSet(symbolSetObj *symbolset, mapObj *map)
{
    int retval = MS_FAILURE;
    
    msAcquireLock( TLOCK_PARSER );
    retval = loadSymbolSet( symbolset, map );
    msReleaseLock( TLOCK_PARSER );

    return retval;
}

int loadSymbolSet(symbolSetObj *symbolset, mapObj *map)
{
  int status=1;
  char szPath[MS_MAXPATHLEN], *pszSymbolPath=NULL;

  int foundSymbolSetToken=MS_FALSE; 
  int token;

  if(!symbolset) {
    msSetError(MS_SYMERR, "Symbol structure unallocated.", "loadSymbolSet()");
    return(-1);
  }

  symbolset->map = (mapObj *)map;

  if(!symbolset->filename) return(0);

  /*
  ** Open the file
  */
  if((msyyin = fopen(msBuildPath(szPath, symbolset->map->mappath, symbolset->filename), "r")) == NULL) {
    msSetError(MS_IOERR, "(%s)", "loadSymbolSet()", symbolset->filename);
    return(-1);
  }

  pszSymbolPath = msGetPath(szPath);

  msyylineno = 0; /* reset line counter */
  msyyrestart(msyyin); /* flush the scanner - there's a better way but this works for now */

  /*
  ** Read the symbol file
  */
  for(;;) {
    token = msyylex(); 

    if(!foundSymbolSetToken && token != SYMBOLSET) { 
      msSetError(MS_IDENTERR, "First token must be SYMBOLSET, this doesn't look like a symbol file.", "msLoadSymbolSet()"); 
      return(-1); 
    }

    switch(token) {
    case(END):
    case(EOF):      
      status = 0;
      break;
    case(SYMBOL):
      /* Allocate/init memory for new symbol if needed */
      if (msGrowSymbolSet(symbolset) == NULL) {
	status = -1;
      }
      else if((loadSymbol((symbolset->symbol[symbolset->numsymbols]), pszSymbolPath) == -1)) 
	  status = -1;
      else
          symbolset->numsymbols++;
      break;
    case(SYMBOLSET):
      foundSymbolSetToken = MS_TRUE;
      break;
    default:
      msSetError(MS_IDENTERR, "Parsing error near (%s):(line %d)", "loadSymbolSet()", msyytext, msyylineno);
      status = -1;
    } /* end switch */

    if(status != 1) break;
  } /* end for */

  fclose(msyyin);
  msyyin = NULL;
  free(pszSymbolPath);
  return(status);
}

/*
** Returns the size, in pixels, of a marker symbol defined by a specific style and scalefactor. Used for annotation
** layer collision avoidance. A marker is made up of a number of styles so the calling code must either do the looping
** itself or call this function for the bottom style which should be the largest.
*/
int msGetMarkerSize(symbolSetObj *symbolset, styleObj *style, int *width, int *height, double scalefactor)
{  
  rectObj rect;
  const char *font=NULL;
  int size;

  *width = *height = 0; /* set a starting value */

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return(MS_FAILURE); /* no such symbol, 0 is OK */

  if(style->symbol == 0) { /* single point */
    *width = 1;
    *height = 1;
    return(MS_SUCCESS);
  }

  if(style->size == -1) {
      size = MS_NINT(
          msSymbolGetDefaultSize( ( symbolset->symbol[style->symbol] ) )
          * scalefactor );
  }
  else
      size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  switch(symbolset->symbol[style->symbol]->type) {  
   
#ifdef USE_GD_FT
  case(MS_SYMBOL_TRUETYPE):
    font = msLookupHashTable(&symbolset->fontset->fonts, symbolset->symbol[style->symbol]->font);
    if(!font) return(MS_FAILURE);

    if(msGetCharacterSize(symbolset->symbol[style->symbol]->character, size, 
                          (char *) font, &rect) != MS_SUCCESS) 
      return(MS_FAILURE);

    *width = (int) MS_MAX(*width, rect.maxx - rect.minx);
    *height = (int) MS_MAX(*height, rect.maxy - rect.miny);

    break;
#endif

  case(MS_SYMBOL_PIXMAP):
    if(size == 1) {        
      *width = MS_MAX(*width, symbolset->symbol[style->symbol]->img->sx);
      *height = MS_MAX(*height, symbolset->symbol[style->symbol]->img->sy);
    } else {
      *width = MS_MAX(*width, MS_NINT((size/symbolset->symbol[style->symbol]->img->sy) * symbolset->symbol[style->symbol]->img->sx));
      *height = MS_MAX(*height, size);
    }
    break;
  default: /* vector and ellipses, scalable */
    if(style->size > 0) {
      *width = MS_MAX(*width, MS_NINT((size/symbolset->symbol[style->symbol]->sizey) * symbolset->symbol[style->symbol]->sizex));
      *height = MS_MAX(*height, size);
    } else { /* use symbol defaults */
      *width = (int) MS_MAX(*width, symbolset->symbol[style->symbol]->sizex);
      *height = (int) MS_MAX(*height, symbolset->symbol[style->symbol]->sizey);
    }
    break;
  }  

  return(MS_SUCCESS);
}

/*
 * Add a default new symbol. If the symbol name exists
 * return the id of the symbol.
 */
int msAddNewSymbol(mapObj *map, char *name)
{
    int i = 0;
 
    if (!map || !name)
      return -1;

    i = msGetSymbolIndex(&map->symbolset, name, MS_TRUE);
    if (i >= 0)
      return i;

    /* Allocate memory for new symbol if needed */
    if (msGrowSymbolSet(&(map->symbolset)) == NULL)
        return -1;

    i = map->symbolset.numsymbols;  
    map->symbolset.symbol[i]->name = strdup(name);

    map->symbolset.numsymbols++;

    return i;
}

/* msAppendSymbol and msRemoveSymbol are part of the work to resolve
 * MapServer bug 579.
 * http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=579 */


int msAppendSymbol(symbolSetObj *symbolset, symbolObj *symbol) {
    /* Allocate memory for new symbol if needed */
    if (msGrowSymbolSet(symbolset) == NULL)
        return -1;
    symbolset->numsymbols++;
    symbolset->symbol[symbolset->numsymbols-1]=symbol;
    MS_REFCNT_INCR(symbol);
    return symbolset->numsymbols-1;
}


symbolObj *msRemoveSymbol(symbolSetObj *symbolset, int nSymbolIndex) {
    int i;
    symbolObj *symbol;
    if (symbolset->numsymbols == 1) {
        msSetError(MS_CHILDERR, "Cannot remove a symbolset's sole symbol", "removeSymbol()");
        return NULL;
    }
    else if (nSymbolIndex < 0 || nSymbolIndex >= symbolset->numsymbols) {
        msSetError(MS_CHILDERR, "Cannot remove symbol, invalid nSymbolIndex %d", "removeSymbol()", nSymbolIndex);
        return NULL;
    }
    else {
        symbol=symbolset->symbol[nSymbolIndex];
        for (i=nSymbolIndex+1; i<symbolset->numsymbols; i++) {
            symbolset->symbol[i-1] = symbolset->symbol[i];
        }
		symbolset->symbol[i-1]=NULL;
        symbolset->numsymbols--;
        MS_REFCNT_DECR(symbol);
        return symbol;
    }
}

int msSaveSymbolSetStream(symbolSetObj *symbolset, FILE *stream) {
    int i;
    if (!symbolset || !stream) {
        msSetError(MS_SYMERR, "Cannot save symbolset.", "msSaveSymbolSetStream()");
        return MS_FAILURE;
    }
    /* Don't ever write out the default symbol at index 0 */
    for (i=1; i<symbolset->numsymbols; i++) {
        symbolset->symbol[i]->inmapfile = MS_TRUE;
        writeSymbol((symbolset->symbol[i]), stream);
    }
    return MS_SUCCESS;
}

int msSaveSymbolSet(symbolSetObj *symbolset, const char *filename) {
    FILE *stream;
    int retval;
    if (!filename || strlen(filename) == 0) {
        msSetError(MS_SYMERR, "Invalid filename.", "msSaveSymbolSet()");
        return MS_FAILURE;
    }
    stream = fopen(filename, "w");
    if (stream)
    {
        retval = msSaveSymbolSetStream(symbolset, stream);
        fclose(stream);
    }
    else 
    {
        msSetError(MS_SYMERR, "Could not write to %s", "msSaveSymbolSet()",
                   filename);
        retval = MS_FAILURE;
    }
    return retval;
}

int msLoadImageSymbol(symbolObj *symbol, const char *filename) {
    FILE *stream;
    char bytes[8];
    gdIOCtx *ctx;
    
    if (!filename || strlen(filename) == 0) {
        msSetError(MS_SYMERR, "Invalid filename.", "msLoadImageSymbol()");
        return MS_FAILURE;
    }

    if ((stream = fopen(filename, "rb")) == NULL) {
          msSetError(MS_IOERR, "Error opening image file %s.", "msLoadImageSymbol()", filename);
          return MS_FAILURE;
    }

    if(symbol->imagepath) free(symbol->imagepath);
    symbol->imagepath = strdup(filename);

    if(symbol->img) gdImageDestroy(symbol->img);

    fread(bytes,8,1,stream); /* read some bytes to try and identify the file */
    rewind(stream); /* reset the image for the readers */
    if (memcmp(bytes,"GIF8",4)==0) 
    {
#ifdef USE_GD_GIF
        ctx = msNewGDFileCtx(stream);
        symbol->img = gdImageCreateFromGifCtx(ctx);
        ctx->gd_free(ctx);
#else
        msSetError(MS_MISCERR, "Unable to load GIF symbol.",
                   "msLoadImageSymbol()");
        fclose(stream);
        return MS_FAILURE;
#endif
    } 
    else if (memcmp(bytes,PNGsig,8)==0) 
    {
#ifdef USE_GD_PNG
        ctx = msNewGDFileCtx(stream);
        symbol->img = gdImageCreateFromPngCtx(ctx);
        ctx->gd_free(ctx);
#else
        msSetError(MS_MISCERR, "Unable to load PNG symbol.",
                   "msAddImageSymbol()");
        fclose(stream);
        return MS_FAILURE;
#endif
    }

    fclose(stream);
  
    if (!symbol->img) {
        msSetError(MS_GDERR, NULL, "msAddImageSymbol()");
        return MS_FAILURE;
    }

    symbol->type = MS_SYMBOL_PIXMAP;
    symbol->sizex = symbol->img->sx;
    symbol->sizey = symbol->img->sy;

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopySymbol()                                                      *
 *                                                                     *
 * Copy a symbolObj, using mapfile.c:initSymbol(), msCopyPoint()       *
 * gdImageCreate(), gdImageCopy()                                      *
 **********************************************************************/

int msCopySymbol(symbolObj *dst, symbolObj *src, mapObj *map) {
  int i;
  
  initSymbol(dst);
  
  MS_COPYSTRING(dst->name, src->name);
  MS_COPYSTELEM(type);
  MS_COPYSTELEM(inmapfile);
  
  /* map is a special case */
  dst->map = map;
  
  MS_COPYSTELEM(sizex);
  MS_COPYSTELEM(sizey);
  
  for (i=0; i < src->numpoints; i++) {
    MS_COPYPOINT(&(dst->points[i]), &(src->points[i]));
  }
  
  MS_COPYSTELEM(numpoints);
  MS_COPYSTELEM(filled);
  MS_COPYSTELEM(patternlength);

  for (i=0; i < src->patternlength; i++) {
    dst->pattern[i] = src->pattern[i];
  }

  MS_COPYSTRING(dst->imagepath, src->imagepath);
  MS_COPYSTELEM(transparent);
  MS_COPYSTELEM(transparentcolor);
  MS_COPYSTRING(dst->character, src->character);
  MS_COPYSTELEM(antialias);
  MS_COPYSTRING(dst->font, src->font);
  MS_COPYSTELEM(gap);
  MS_COPYSTELEM(position);
  MS_COPYSTELEM(linecap);
  MS_COPYSTELEM(linejoin);
  MS_COPYSTELEM(linejoinmaxsize);

  /* Copy the actual symbol imagery */
  if (src->img) {
    if (dst->img)
      gdFree(dst->img);
    
    if (gdImageTrueColor(src->img)) {
      dst->img = gdImageCreateTrueColor(gdImageSX(src->img), gdImageSY(src->img));
      gdImageFilledRectangle(dst->img, 0, 0, gdImageSX(src->img), gdImageSY(src->img), gdImageColorAllocateAlpha(dst->img, 0, 0, 0, gdAlphaTransparent)); 
      gdImageAlphaBlending(dst->img, 0);
      gdImageCopy(dst->img, src->img, 0, 0, 0, 0, gdImageSX(src->img), gdImageSY(src->img));
    } else {
      int tc = gdImageGetTransparent(src->img);

      dst->img = gdImageCreate(gdImageSX(src->img), gdImageSY(src->img));
      if(tc != -1)
        gdImageColorTransparent(dst->img, gdImageColorAllocate(dst->img, gdImageRed(src->img, tc), gdImageGreen(src->img, tc), gdImageBlue(src->img, tc)));

      gdImageCopy(dst->img, src->img, 0, 0, 0, 0, gdImageSX(src->img), gdImageSY(src->img));      
    }
  }

  return(MS_SUCCESS);
} 

/***********************************************************************
 * msCopySymbolSet()                                                   *
 *                                                                     *
 * Copy a symbolSetObj using msCopyFontSet(), msCopySymbol()           *
 **********************************************************************/

int msCopySymbolSet(symbolSetObj *dst, symbolSetObj *src, mapObj *map)
{
  int i, return_value;
  
  MS_COPYSTRING(dst->filename, src->filename);
  
  dst->map = map;
  dst->fontset = &(map->fontset);
  
  /* Copy child symbols */
  for (i = 0; i < src->numsymbols; i++) {
    if (msGrowSymbolSet(dst) == NULL)
        return MS_FAILURE;
    return_value = msCopySymbol(dst->symbol[i], src->symbol[i], map);
    if (return_value != MS_SUCCESS) {
      msSetError(MS_MEMERR,"Failed to copy symbol.","msCopySymbolSet()");
      return(MS_FAILURE);
    }
    dst->numsymbols++;
  }

  /* MS_COPYSTELEM(imagecachesize); */
  
  /* I have a feeling that the code below is not quite right - Sean */
  /*copyProperty(&(dst->imagecache), &(src->imagecache),
               sizeof(struct imageCacheObj));
   */

  dst->imagecachesize = 0; /* since we are not copying the cache set the cache  to NUNLL and the size to 0 (bug 1521) */
  dst->imagecache = NULL;

  return(MS_SUCCESS);
}

/* ----------------------------------------------------------------------------
   msSymbolGetImageGD
   
   Get a symbolObj as an imageObj with the specified format.
---------------------------------------------------------------------------- */
imageObj *msSymbolGetImageGD(symbolObj *symbol, outputFormatObj *input_format)
{
    imageObj *image=NULL;
    int width, height;
    outputFormatObj *format=NULL;

    if (!symbol || !input_format)
    {
        msSetError(MS_SYMERR, "NULL symbol or format", "msSymbolGetImageGD()");
        return NULL;
    }

    if (symbol->type != MS_SYMBOL_PIXMAP)
    {
        msSetError(MS_SYMERR, "Can't return image from non-pixmap symbol",
                   "msSymbolGetImageGD()");
        return NULL;
    }
    
    if (symbol->img) 
    {
        if (input_format)
        {
            if (MS_DRIVER_GD(input_format))
                format = input_format;
            else
            {
                msSetError(MS_IMGERR, "Non-GD drivers not allowed",
                           "msSymbolGetImageGD()");
                return NULL;
            }
        }
        else 
        {
            format = msCreateDefaultOutputFormat(NULL, "GD/GIF");
            if (format == NULL)
                format = msCreateDefaultOutputFormat(NULL, "GD/PNG");
            if (format == NULL)
                format = msCreateDefaultOutputFormat(NULL, "GD/JPEG");
            if (format == NULL)
                format = msCreateDefaultOutputFormat(NULL, "GD/WBMP");
        }
        
        if (format == NULL) 
        {
            msSetError(MS_IMGERR, "Could not create output format",
                       "msSymbolGetImageGD()");
            return NULL;
        }
      
        width = gdImageSX(symbol->img);
        height = gdImageSY(symbol->img);
        
        image = msImageCreate(width, height, format, NULL, NULL, NULL);

        if (symbol->img->trueColor)
        {
            gdImageAlphaBlending(image->img.gd, 0);
            gdImageCopy(image->img.gd, symbol->img, 0, 0, 0, 0, width, height);
        }
        else
        {
            /*gdImageColorAllocate(image->img.gd,
                                 gdImageRed(symbol->img, 0),
                                 gdImageGreen(symbol->img, 0),
                                 gdImageBlue(symbol->img, 0));*/
            gdImageCopy(image->img.gd, symbol->img, 0, 0, 0, 0, width, height);
        }
    }

    /* returned reference may be NULL */
    return image;
}

/* ----------------------------------------------------------------------------
   msSymbolSetImageGD

   Sets the symbolObj's image by copying from a provided imageObj
   ------------------------------------------------------------------------- */
int msSymbolSetImageGD(symbolObj *symbol, imageObj *image)
{
    if (!symbol || !image)
    {
        msSetError(MS_SYMERR, "NULL symbol or image", "msSymbolSetImageGD()");
        return MS_FAILURE;
    }
    
    if (symbol->img) {
        gdImageDestroy(symbol->img);
        symbol->img = NULL;
    }

    /* Allocate new GD image */
    if (image->format->imagemode == MS_IMAGEMODE_RGB
    || image->format->imagemode == MS_IMAGEMODE_RGBA)
    {
        symbol->img = gdImageCreateTrueColor(image->width, image->height);
        gdImageAlphaBlending(symbol->img, 0);
    }
    else 
    {
        symbol->img = gdImageCreate(image->width, image->height);
        gdImagePaletteCopy(symbol->img, image->img.gd);
        gdImageColorTransparent(symbol->img, 
                                gdImageGetTransparent(image->img.gd));
    }
    
    gdImageCopy(symbol->img, image->img.gd, 0, 0, 0, 0,
                image->width, image->height);

    symbol->type = MS_SYMBOL_PIXMAP; /* just in case the symbol wasn't a PIXMAP symbol before */
    symbol->sizex = symbol->img->sx;
    symbol->sizey = symbol->img->sy;

    return MS_SUCCESS;
}

static void get_bbox(pointObj *poiList, int numpoints, double *minx, double *miny, double *maxx, double *maxy) {
  int j;

  *minx = *maxx = poiList[0].x;
  *miny = *maxy = poiList[0].y;
  for(j=1; j<numpoints; j++) {
    if ((poiList[j].x==-99.0) || (poiList[j].y==-99.0)) continue;
    *minx = MS_MIN(*minx, poiList[j].x);
    *maxx = MS_MAX(*maxx, poiList[j].x);
    *miny = MS_MIN(*miny, poiList[j].y);
    *maxy = MS_MAX(*maxy, poiList[j].y);
  }

  return;
}

/*
** msRotateSymbol - Clockwise rotation of a symbol definition. Contributed
** by MapMedia, with clean up by SDL. Currently only type VECTOR and PIXMAP 
** symbols are handled.
*/
symbolObj *msRotateSymbol(symbolObj *symbol, double angle)
{
  double angle_rad=0.0;
  double cos_a, sin_a;
  double minx=0.0, miny=0.0, maxx=0.0, maxy=0.0;
  symbolObj *newSymbol = NULL;

  if(!(symbol->type == MS_SYMBOL_VECTOR || symbol->type == MS_SYMBOL_PIXMAP)) {
    msSetError(MS_SYMERR, "Only symbols with type VECTOR or PIXMAP may be rotated.", "msRotateSymbol()");
    return NULL;
  }

  newSymbol = (symbolObj *) malloc(sizeof(symbolObj));
  msCopySymbol(newSymbol, symbol, NULL); /* TODO: do we really want to do this for all symbol types? */

  angle_rad = (MS_DEG_TO_RAD*angle);

  switch(symbol->type) {
  case(MS_SYMBOL_VECTOR):
    {
      double dp_x, dp_y, xcor, ycor;
      double TOL=0.00000000001;
      int i;

      sin_a = sin(angle_rad);
      cos_a = cos(angle_rad);

      dp_x = symbol->sizex * .5; /* get the shift vector at 0,0 */
      dp_y = symbol->sizey * .5;

      /* center at 0,0 and rotate; then move back */
      for( i=0;i < symbol->numpoints;i++) {
	/* don't rotate PENUP commands (TODO: should use a constant here) */
	if ((symbol->points[i].x == -99.0) || (symbol->points[i].x == -99.0) ) {
	  newSymbol->points[i].x = -99.0;
	  newSymbol->points[i].y = -99.0;
	  continue;
	}
            
	newSymbol->points[i].x = dp_x + ((symbol->points[i].x-dp_x)*cos_a - (symbol->points[i].y-dp_y)*sin_a);
	newSymbol->points[i].y = dp_y + ((symbol->points[i].x-dp_x)*sin_a + (symbol->points[i].y-dp_y)*cos_a);
      }
	
      /* get the new bbox of the symbol, because we need it to get the new dimensions of the new symbol */
      get_bbox(newSymbol->points, newSymbol->numpoints, &minx, &miny, &maxx, &maxy);
      if ( (fabs(minx)>TOL) || (fabs(miny)>TOL) ) {
	xcor = minx*-1.0; /* symbols always start at 0,0 so get the shift vector */
	ycor = miny*-1.0;
	for( i=0;i < newSymbol->numpoints;i++) {
	  if ((newSymbol->points[i].x == -99.0) || (newSymbol->points[i].x == -99.0))
	    continue;
	  newSymbol->points[i].x = newSymbol->points[i].x + xcor;
	  newSymbol->points[i].y = newSymbol->points[i].y + ycor;
	}
	
	/* update the bbox to get the final dimension values for the symbol */
	get_bbox(newSymbol->points, newSymbol->numpoints, &minx, &miny, &maxx, &maxy);
      }

      newSymbol->sizex = maxx;
      newSymbol->sizey = maxy;
      break;
    }
  case(MS_SYMBOL_PIXMAP):
    {
      double cos_a, sin_a;

      double x1 = 0.0, y1 = 0.0; /* destination rectangle */
      double x2 = 0.0, y2 = 0.0;
      double x3 = 0.0, y3 = 0.0;
      double x4 = 0.0, y4 = 0.0;

      long minx, miny, maxx, maxy;

      int width=0, height=0;
      /* int color; */

      sin_a = sin(angle_rad);
      cos_a = cos(angle_rad);

      /* compute distination rectangle (x1,y1 is known) */
      x1 = 0 ; y1 = 0 ;
      x2 = symbol->img->sy * sin_a;
      y2 = -symbol->img->sy * cos_a;
      x3 = (symbol->img->sx * cos_a) + (symbol->img->sy * sin_a);
      y3 = (symbol->img->sx * sin_a) - (symbol->img->sy * cos_a);
      x4 = (symbol->img->sx * cos_a);
      y4 = (symbol->img->sx * sin_a);
		
      minx = (long) MS_MIN(x1,MS_MIN(x2,MS_MIN(x3,x4)));
      miny = (long) MS_MIN(y1,MS_MIN(y2,MS_MIN(y3,y4)));
      maxx = (long) MS_MAX(x1,MS_MAX(x2,MS_MAX(x3,x4)));
      maxy = (long) MS_MAX(y1,MS_MAX(y2,MS_MAX(y3,y4)));

      width = (int)ceil(maxx-minx);
      height = (int)ceil(maxy-miny);

      /* create the new image based on the computed width/height */
      gdFree(newSymbol->img);
      if (gdImageTrueColor(symbol->img)) {
	newSymbol->img = gdImageCreateTrueColor(width, height);
        gdImageAlphaBlending(newSymbol->img, 0);
        gdImageFilledRectangle(newSymbol->img, 0, 0, width, height, gdImageColorAllocateAlpha(newSymbol->img, 0, 0, 0, gdAlphaTransparent)); 
      } else {
        int tc = gdImageGetTransparent(symbol->img);
	newSymbol->img = gdImageCreate(width, height);	
        if(tc != -1)
          gdImageColorTransparent(newSymbol->img, gdImageColorAllocate(newSymbol->img, gdImageRed(symbol->img, tc), gdImageGreen(symbol->img, tc), gdImageBlue(symbol->img, tc)));
      }

      newSymbol->sizex = maxx;
      newSymbol->sizey = maxy;

      gdImageCopyRotated (newSymbol->img, symbol->img, width*0.5, height*0.5, 0, 0, gdImageSX(symbol->img), gdImageSY(symbol->img), angle);
      break;
    }
  default:
    break; /* should never get here */
  } /* end symbol type switch */

  return newSymbol;
}


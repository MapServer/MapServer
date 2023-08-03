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
#include "fontcache.h"
#include "mapows.h"



extern int msyylex(void); /* lexer globals */
extern void msyyrestart(FILE *);
extern double msyynumber;
extern char *msyystring_buffer;
extern int msyylineno;
extern FILE *msyyin;

extern int msyystate;

void freeImageCache(struct imageCacheObj *ic)
{
  if(ic) {
    freeImageCache(ic->next); /* free any children */
    msFreeRasterBuffer(&(ic->img));
    free(ic);
  }
  return;
}

/*
** msSymbolGetDefaultSize()
**
** Return the default size of a symbol.
** Note: The size of a symbol is the height. Everywhere in the code, the width
** is adjusted to the size that becomes the height.
** See mapgd.c // size ~ height in pixels
*/
double msSymbolGetDefaultSize(symbolObj *s)
{
  double size = 1;
  if(s == NULL)
    return 1;

  switch(s->type) {
    case(MS_SYMBOL_TRUETYPE):
      break;
    case(MS_SYMBOL_PIXMAP):
      assert(s->pixmap_buffer != NULL);
      if(s->pixmap_buffer == NULL) return 1; /* FIXME */
      size = (double)s->pixmap_buffer->height;
      break;
    case(MS_SYMBOL_SVG):
#if defined(USE_SVG_CAIRO) || defined (USE_RSVG)
      assert(s->renderer_cache != NULL);
      size = s->sizey;
#endif
      break;
    default: /* vector and ellipses, scalable */
      size = (s->sizey<=0)?s->sizex:s->sizey;
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
  s->sizex = 1;
  s->sizey = 1;
  s->filled = MS_FALSE;
  s->numpoints=0;
  s->renderer=NULL;
  s->renderer_free_func = NULL;
  s->renderer_cache = NULL;
  s->pixmap_buffer=NULL;
  s->imagepath = NULL;
  s->name = NULL;
  s->inmapfile = MS_FALSE;
  s->font = NULL;
  s->full_pixmap_path = NULL;
  s->character = NULL;
  s->anchorpoint_x = s->anchorpoint_y = 0.5;

}

int msFreeSymbol(symbolObj *s)
{
  if(!s) return MS_FAILURE;
  if( MS_REFCNT_DECR_IS_NOT_ZERO(s) ) {
    return MS_FAILURE;
  }

  if(s->name) free(s->name);
  if(s->renderer_free_func) {
    s->renderer_free_func(s);
  } else {
    if(s->renderer!=NULL) {
      s->renderer->freeSymbol(s);
    }
  }
  if(s->pixmap_buffer) {
    msFreeRasterBuffer(s->pixmap_buffer);
    free(s->pixmap_buffer);
  }


  if(s->font) free(s->font);
  msFree(s->full_pixmap_path);
  if(s->imagepath) free(s->imagepath);
  if(s->character) free(s->character);

  return MS_SUCCESS;
}

int loadSymbol(symbolObj *s, char *symbolpath)
{
  int done=MS_FALSE;
  char szPath[MS_MAXPATHLEN];

  initSymbol(s);

  for(;;) {
    switch(msyylex()) {
      case(ANCHORPOINT):
	if(getDouble(&(s->anchorpoint_x), MS_NUM_CHECK_RANGE, 0, 1) == -1) {
	  msSetError(MS_SYMERR, "ANCHORPOINT must be between 0 and 1", "loadSymbol()");
          return -1;
        }
        if(getDouble(&(s->anchorpoint_y), MS_NUM_CHECK_RANGE, 0, 1) == -1) {        
          msSetError(MS_SYMERR, "ANCHORPOINT must be between 0 and 1", "loadSymbol()");
          return(-1);
	}
        break;

      case(ANTIALIAS): /*ignore*/
        msyylex();
        break;
      case(CHARACTER):
        if(getString(&s->character) == MS_FAILURE) return(-1);
        break;
      case(END): /* do some error checking */
        if((s->type == MS_SYMBOL_SVG) && (s->imagepath == NULL)) {
          msSetError(MS_SYMERR, "Symbol of type SVG has no file path specified.", "loadSymbol()");
          return(-1);
        }
        if((s->type == MS_SYMBOL_PIXMAP) && (s->full_pixmap_path == NULL)) {
          msSetError(MS_SYMERR, "Symbol of type PIXMAP has no image data.", "loadSymbol()");
          return(-1);
        }
        if(((s->type == MS_SYMBOL_ELLIPSE) || (s->type == MS_SYMBOL_VECTOR)) && (s->numpoints == 0)) {
          msSetError(MS_SYMERR, "Symbol of type VECTOR or ELLIPSE has no point data.", "loadSymbol()");
          return(-1);
        }
        if(s->type == MS_SYMBOL_VECTOR) {
          double minx = s->points[0].x;
          double miny = s->points[0].y;
          /* should only negative points be shifted? (#4116)*/
          int shiftpositive = ((s->anchorpoint_x!=0.5)||(s->anchorpoint_y!=0.5));
          int i;
          for(i=1; i<s->numpoints; i++) {
            if(s->points[i].x != -99 && s->points[i].y != -99) {
              if(s->points[i].x<minx) minx = s->points[i].x;
              if(s->points[i].y<miny) miny = s->points[i].y;
            }
          }
          if(minx<0 || miny<0 || (shiftpositive && (minx!=0 || miny!=0))) {
            for(i=0; i<s->numpoints; i++) {
              if(s->points[i].x != -99 && s->points[i].y != -99) {
                s->points[i].x -= minx;
                s->points[i].y -= miny;
              }
            }
            s->sizex -= minx;
            s->sizey -= miny;
          }
        }

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
      case(IMAGE):
        if(msyylex() != MS_STRING) { /* get image location from next token */
          msSetError(MS_TYPEERR, "Parsing error near (%s):(line %d)", "loadSymbol()", msyystring_buffer, msyylineno);
          return(-1);
        }
        msFree(s->full_pixmap_path);
        s->full_pixmap_path = msStrdup(msBuildPath(szPath, symbolpath, msyystring_buffer));
        /* Set imagepath */
        msFree(s->imagepath);
        s->imagepath = msStrdup(msyystring_buffer);
        break;
      case(NAME):
        if(getString(&s->name) == MS_FAILURE) return(-1);
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
              if(s->numpoints == MS_MAXVECTORPOINTS) {
                msSetError(MS_SYMERR, "POINT block contains too many points.", "loadSymbol()");
		return(-1);
	      }
              s->points[s->numpoints].x = atof(msyystring_buffer); /* grab the x */
              if(getDouble(&(s->points[s->numpoints].y), MS_NUM_CHECK_NONE, -1, -1) == -1) return(-1); /* grab the y */
              if(s->points[s->numpoints].x!=-99) {
                s->sizex = MS_MAX(s->sizex, s->points[s->numpoints].x);
                s->sizey = MS_MAX(s->sizey, s->points[s->numpoints].y);
              }
              s->numpoints++;
              break;
            default:
              msSetError(MS_TYPEERR, "Parsing error near (%s):(line %d)", "loadSymbol()", msyystring_buffer, msyylineno);
              return(-1);
          }

          if(done == MS_TRUE)
            break;
        }
        break;
      case(TRANSPARENT):
        s->transparent = MS_TRUE;
        if(getInteger(&(s->transparentcolor), MS_NUM_CHECK_RANGE, 0, 255) == -1) return(-1);
        break;
      case(TYPE):
        if((s->type = getSymbol(8,MS_SYMBOL_VECTOR,MS_SYMBOL_ELLIPSE,MS_SYMBOL_PIXMAP,MS_SYMBOL_SIMPLE,MS_TRUETYPE,MS_SYMBOL_HATCH,MS_SYMBOL_SVG)) == -1)
          return(-1);
        if(s->type == MS_TRUETYPE) /* TrueType keyword is valid several place in map files and symbol files, this simplifies the lexer */
          s->type = MS_SYMBOL_TRUETYPE;
        break;
      default:
        msSetError(MS_IDENTERR, "Parsing error near (%s):(line %d)", "loadSymbol()", msyystring_buffer, msyylineno);
        return(-1);
    } /* end switch */
  } /* end for */
  return done;
}

void writeSymbol(symbolObj *s, FILE *stream)
{
  int i;

  msIO_fprintf(stream, "  SYMBOL\n");
  if(s->name != NULL) msIO_fprintf(stream, "    NAME \"%s\"\n", s->name);

  switch (s->type) {
    case(MS_SYMBOL_HATCH):
      msIO_fprintf(stream, "    TYPE HATCH\n");
      break;
    case(MS_SYMBOL_PIXMAP):
      msIO_fprintf(stream, "    TYPE PIXMAP\n");
      if(s->imagepath != NULL) msIO_fprintf(stream, "    IMAGE \"%s\"\n", s->imagepath);
      if(s->anchorpoint_y!=0.5 || s->anchorpoint_x!=0.5) {
        msIO_fprintf(stream, "    ANCHORPOINT %g %g\n", s->anchorpoint_x, s->anchorpoint_y);
      }
      msIO_fprintf(stream, "    TRANSPARENT %d\n", s->transparentcolor);
      break;
    case(MS_SYMBOL_TRUETYPE):
      msIO_fprintf(stream, "    TYPE TRUETYPE\n");
      if (s->character != NULL) msIO_fprintf(stream, "    CHARACTER \"%s\"\n", s->character);
      if (s->font != NULL) msIO_fprintf(stream, "    FONT \"%s\"\n", s->font);
      if(s->anchorpoint_y!=0.5 || s->anchorpoint_x!=0.5) {
        msIO_fprintf(stream, "    ANCHORPOINT %g %g\n", s->anchorpoint_x, s->anchorpoint_y);
      }
      break;
    default:
      if(s->type == MS_SYMBOL_ELLIPSE)
        msIO_fprintf(stream, "    TYPE ELLIPSE\n");
      else if(s->type == MS_SYMBOL_VECTOR)
        msIO_fprintf(stream, "    TYPE VECTOR\n");
      else if(s->type == MS_SYMBOL_SVG)
        msIO_fprintf(stream, "    TYPE SVG\n");
      else
        msIO_fprintf(stream, "    TYPE SIMPLE\n");

      if(s->filled == MS_TRUE) msIO_fprintf(stream, "    FILLED TRUE\n");
      if(s->imagepath != NULL) msIO_fprintf(stream, "    IMAGE \"%s\"\n", s->imagepath);
      if(s->anchorpoint_y!=0.5 || s->anchorpoint_x!=0.5) {
        msIO_fprintf(stream, "    ANCHORPOINT %g %g\n", s->anchorpoint_x, s->anchorpoint_y);
      }

      /* POINTS */
      if(s->numpoints != 0) {
        msIO_fprintf(stream, "    POINTS\n");
        for(i=0; i<s->numpoints; i++) {
          msIO_fprintf(stream, "      %g %g\n", s->points[i].x, s->points[i].y);
        }
        msIO_fprintf(stream, "    END\n");
      }
      break;
  }

  msIO_fprintf(stream, "  END\n\n");
}


/*
** Little helper function to allow us to build symbol files on-the-fly
** from just a file name.
**
** Returns the symbol index or -1 if it could not be added.
*/
int msAddImageSymbol(symbolSetObj *symbolset, char *filename)
{
  char szPath[MS_MAXPATHLEN];
  symbolObj *symbol=NULL;
  char *extension=NULL;
  int symbolidx;

  if(!symbolset) {
    msSetError(MS_SYMERR, "Symbol structure unallocated.", "msAddImageSymbol()");
    return(-1);
  }

  if(!filename || strlen(filename) == 0) return(-1);

  /* Allocate/init memory for new symbol if needed */
  if (msGrowSymbolSet(symbolset) == NULL)
    return -1;
  symbolidx = symbolset->numsymbols;
  symbolset->numsymbols++;
  symbol = symbolset->symbol[symbolidx];

  /* check if svg checking extension otherwise assume it's a pixmap */
  extension = strrchr(filename, '.');
  if (extension == NULL)
    extension = "";
  if (strncasecmp(extension, ".svg", 4) == 0) {
    symbol->type = MS_SYMBOL_SVG;
  } else {
    symbol->type = MS_SYMBOL_PIXMAP;
  }

#ifdef USE_CURL
  if (strncasecmp(filename, "http", 4) == 0) {
    char *tmpfullfilename = NULL;
    char *tmpfilename = NULL;
    char *tmppath = NULL;
    int status = 0;
    char szPath[MS_MAXPATHLEN];
    int bCheckLocalCache = MS_TRUE;

    tmppath = msTmpPath(NULL, NULL, NULL);
    if (tmppath) {
      tmpfilename = msEncodeUrl(filename);
      tmpfullfilename = msBuildPath(szPath, tmppath, tmpfilename);
      if (tmpfullfilename) {
        /*use the url for now as a caching mechanism*/
        if (msHTTPGetFile(filename, tmpfullfilename, &status, -1, bCheckLocalCache, 0, 1024*1024 /* 1 MegaByte */) == MS_SUCCESS) {
          symbol->imagepath = msStrdup(tmpfullfilename);
          symbol->full_pixmap_path = msStrdup(tmpfullfilename);
        } else {
          unlink(tmpfullfilename); 
          msFree(tmpfilename);
          msFree(tmppath);
          return -1;
        }
      }
      msFree(tmpfilename);
      msFree(tmppath);
    }
  }
#endif
  /*if the http did not work, allow it to be treated as a file*/
  if (!symbol->full_pixmap_path) {
    if(symbolset->map) {
      symbol->full_pixmap_path = msStrdup(msBuildPath(szPath, symbolset->map->mappath, filename));
    } else {
      symbol->full_pixmap_path = msStrdup(msBuildPath(szPath, NULL, filename));
    }
    symbol->imagepath = msStrdup(filename);
  }
  symbol->name = msStrdup(filename);
  return symbolidx;
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
  symbolObj* symbol = msGrowSymbolSet(symbolset);
  if (symbol == NULL)
    return; /* alloc failed */
  symbol->type = MS_SYMBOL_ELLIPSE;
  symbol->filled = MS_TRUE;
  symbol->numpoints = 1;
  symbol->points[0].x = 1;
  symbol->points[0].y = 1;

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
  int symbolSetLevel=0;
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

  msyystate = MS_TOKENIZE_FILE; /* restore lexer state to INITIAL, and do return comments */
  msyylex(); /* sets things up, but doesn't process any tokens */

  msyylineno = 0; /* reset line counter */
  msyyrestart(msyyin); /* flush the scanner - there's a better way but this works for now */

  /*
  ** Read the symbol file
  */
  for(;;) {
    token = msyylex();

    if(!foundSymbolSetToken && token != SYMBOLSET) {
      msSetError(MS_IDENTERR, "First token must be SYMBOLSET, this doesn't look like a symbol file.", "msLoadSymbolSet()");
      status = -1;
      break;
    }

    switch(token) {
      case(END):
        if (--symbolSetLevel < 0) {
          msSetError(MS_IDENTERR, "END token found outside SYMBOLSET context. When nesting multiple SYMBOLSETs, make sure the SYMBOLSET/END pairs match.", "msLoadSymbolSet()");
          status = -1;
        }
        break;
      case(EOF):
        status = 0;
        break;
      case(SYMBOL):
        /* Allocate/init memory for new symbol if needed */
        if (symbolSetLevel == 0) {
          msSetError(MS_IDENTERR, "SYMBOL token found outside SYMBOLSET context. When nesting multiple SYMBOLSETs, make sure the SYMBOLSET/END pairs match.", "msLoadSymbolSet()");
          status = -1;
        } else if (msGrowSymbolSet(symbolset) == NULL) {
          status = -1;
        } else if((loadSymbol((symbolset->symbol[symbolset->numsymbols]), pszSymbolPath) == -1))
          status = -1;
        else
          symbolset->numsymbols++;
        break;
      case(SYMBOLSET):
        foundSymbolSetToken = MS_TRUE;
        symbolSetLevel++;
        break;
      default:
        msSetError(MS_IDENTERR, "Parsing error near (%s):(line %d)", "loadSymbolSet()", msyystring_buffer, msyylineno);
        status = -1;
    } /* end switch */

    if(status != 1) break;
  } /* end for */

  fclose(msyyin);
  msyyin = NULL;
  free(pszSymbolPath);
  return(status);
}

int msGetCharacterSize(mapObj *map, char* font, int size, char *character, rectObj *r) {
  unsigned int unicode, codepoint;
  glyph_element *glyph;
  face_element *face = msGetFontFace(font, &map->fontset);
  if(MS_UNLIKELY(!face)) return MS_FAILURE;
  msUTF8ToUniChar(character, &unicode);
  codepoint = msGetGlyphIndex(face,unicode);
  glyph = msGetGlyphByIndex(face,size,codepoint);
  if(MS_UNLIKELY(!glyph)) return MS_FAILURE;
  r->minx = glyph->metrics.minx;
  r->maxx = glyph->metrics.maxx;
  r->miny = - glyph->metrics.maxy;
  r->maxy = - glyph->metrics.miny;
  return MS_SUCCESS;
}

/*
** Returns the size, in pixels, of a marker symbol defined by a specific style and scalefactor. Used for annotation
** layer collision avoidance. A marker is made up of a number of styles so the calling code must either do the looping
** itself or call this function for the bottom style which should be the largest.
*/
int msGetMarkerSize(mapObj *map, styleObj *style, double *width, double *height, double scalefactor)
{
  int size;
  symbolObj *symbol;
  *width = *height = 0; /* set a starting value */

  if(style->symbol > map->symbolset.numsymbols || style->symbol < 0) return(MS_FAILURE); /* no such symbol, 0 is OK */

  if(style->symbol == 0) { /* single point */
    *width = 1;
    *height = 1;
    return(MS_SUCCESS);
  }

  symbol = map->symbolset.symbol[style->symbol];
  if (symbol->type == MS_SYMBOL_PIXMAP && !symbol->pixmap_buffer) {
    if (MS_SUCCESS != msPreloadImageSymbol(MS_MAP_RENDERER(map), symbol))
      return MS_FAILURE;
  }
  if(symbol->type == MS_SYMBOL_SVG && !symbol->renderer_cache) {
#if defined(USE_SVG_CAIRO) || defined (USE_RSVG)
    if(MS_SUCCESS != msPreloadSVGSymbol(symbol))
      return MS_FAILURE;
#else
    msSetError(MS_SYMERR, "SVG symbol support is not enabled.", "msGetMarkerSize()");
    return MS_FAILURE;
#endif
  }
  if(style->size == -1) {
    size = ( msSymbolGetDefaultSize(symbol) * scalefactor );
  } else
    size = (style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  switch(symbol->type) {

    case(MS_SYMBOL_TRUETYPE): {
      rectObj gbounds;
      if(MS_UNLIKELY(MS_FAILURE == msGetCharacterSize(map,symbol->font,size,symbol->character, &gbounds)))
        return MS_FAILURE;

      *width = MS_MAX(*width, (gbounds.maxx-gbounds.minx));
      *height = MS_MAX(*height, (gbounds.maxy-gbounds.miny));
    }

      break;

    case(MS_SYMBOL_PIXMAP):
      if(size == 1) {
        *width = MS_MAX(*width, symbol->pixmap_buffer->width);
        *height = MS_MAX(*height, symbol->pixmap_buffer->height);
      } else {
        *width = MS_MAX(*width, (((double)size/(double)symbol->pixmap_buffer->height) * symbol->pixmap_buffer->width));
        *height = MS_MAX(*height, size);
      }
      break;
    default: /* vector and ellipses, scalable */
      if(style->size > 0) {
        *width = MS_MAX(*width, ((size/symbol->sizey) * symbol->sizex));
        *height = MS_MAX(*height, size);
      } else { /* use symbol defaults */
        *width = MS_MAX(*width, symbol->sizex);
        *height = MS_MAX(*height, symbol->sizey);
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
  map->symbolset.symbol[i]->name = msStrdup(name);

  map->symbolset.numsymbols++;

  return i;
}

/* msAppendSymbol and msRemoveSymbol are part of the work to resolve
 * MapServer bug 579.
 * http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=579 */


int msAppendSymbol(symbolSetObj *symbolset, symbolObj *symbol)
{
  /* Allocate memory for new symbol if needed */
  if (msGrowSymbolSet(symbolset) == NULL)
    return -1;

  /* we need to free the symbolObj that was already allocated as we are
   going to replace it with the provided symbolObj*. Not the most efficient
   technique, but this function should be rarely called, and in any case only
   by mapscript. Another option could be to use msCopySymbol(), in which case
   the call to MS_REFCNT_INCR(symbol) should be removed.*/
  if(symbolset->symbol[symbolset->numsymbols]) {
    msFreeSymbol(symbolset->symbol[symbolset->numsymbols]);
    msFree(symbolset->symbol[symbolset->numsymbols]);
  }
  symbolset->symbol[symbolset->numsymbols]=symbol;
  MS_REFCNT_INCR(symbol);
  return symbolset->numsymbols++;
}


symbolObj *msRemoveSymbol(symbolSetObj *symbolset, int nSymbolIndex)
{
  int i;
  symbolObj *symbol;
  if (symbolset->numsymbols == 1) {
    msSetError(MS_CHILDERR, "Cannot remove a symbolset's sole symbol", "removeSymbol()");
    return NULL;
  } else if (nSymbolIndex < 0 || nSymbolIndex >= symbolset->numsymbols) {
    msSetError(MS_CHILDERR, "Cannot remove symbol, invalid nSymbolIndex %d", "removeSymbol()", nSymbolIndex);
    return NULL;
  } else {
    symbol=symbolset->symbol[nSymbolIndex];
    for (i=nSymbolIndex+1; i<symbolset->numsymbols; i++) {
      symbolset->symbol[i-1] = symbolset->symbol[i];
    }
    symbolset->symbol[i-1]=NULL;
    symbolset->numsymbols--;
    MS_REFCNT_DECR(symbol);
    /* update symbol references in the map */
    if (symbolset->map) {
        int l,c,s,lb;
        layerObj *layer;
        classObj *cl;
        styleObj *style;
        labelObj *label;
        for (l = 0; l < symbolset->map->numlayers; l++) {
            layer = GET_LAYER(symbolset->map, l);
            for (c = 0; c < layer->numclasses; c++) {
                cl = layer->class[c];
                for (s = 0; s < cl->numstyles; s++) {
                    style = cl->styles[s];
                    if (style->symbol >= nSymbolIndex)
                        --style->symbol;
                }
                for (lb = 0; lb < cl->numlabels; lb++) {
                    label = cl->labels[lb];
                    for (s = 0; s < label->numstyles; s++) {
                        style = label->styles[s];
                        if (style->symbol >= nSymbolIndex)
                            --style->symbol;
                    }
                }
            }
        }
        /* Update symbol references in labelcache */
        for(c = 0; c < MS_MAX_LABEL_PRIORITY; c++) {
            labelCacheSlotObj *cacheslot = &(symbolset->map->labelcache.slots[c]);
            for(l = 0; l < cacheslot->numlabels; l++) {
                labelCacheMemberObj *cachePtr = &(cacheslot->labels[l]);
                for(lb = 0; lb < cachePtr->numtextsymbols; lb++) {
                    label = cachePtr->textsymbols[lb]->label;
                    for (s = 0; s < label->numstyles; s++) {
                        style = label->styles[s];
                        if (style->symbol >= nSymbolIndex)
                            --style->symbol;
                    }
                }
            }
        }
    }
    return symbol;
  }
}

int msSaveSymbolSetStream(symbolSetObj *symbolset, FILE *stream)
{
  int i;
  if (!symbolset || !stream) {
    msSetError(MS_SYMERR, "Cannot save symbolset.", "msSaveSymbolSetStream()");
    return MS_FAILURE;
  }
  /* Don't ever write out the default symbol at index 0 */
  for (i=1; i<symbolset->numsymbols; i++) {
    if(!symbolset->symbol[i]->inmapfile) writeSymbol((symbolset->symbol[i]), stream);
  }
  return MS_SUCCESS;
}

int msSaveSymbolSet(symbolSetObj *symbolset, const char *filename)
{
  FILE *stream;
  int retval;
  if (!filename || strlen(filename) == 0) {
    msSetError(MS_SYMERR, "Invalid filename.", "msSaveSymbolSet()");
    return MS_FAILURE;
  }
  stream = fopen(filename, "w");
  if (stream) {
    fprintf(stream, "SYMBOLSET\n");
    retval = msSaveSymbolSetStream(symbolset, stream);
    fprintf(stream, "END\n");
    fclose(stream);
  } else {
    msSetError(MS_SYMERR, "Could not write to %s", "msSaveSymbolSet()",
               filename);
    retval = MS_FAILURE;
  }
  return retval;
}

int msLoadImageSymbol(symbolObj *symbol, const char *filename)
{
  msFree(symbol->full_pixmap_path);
  symbol->full_pixmap_path = msStrdup(filename);
  return MS_SUCCESS;
}

int msPreloadImageSymbol(rendererVTableObj *renderer, symbolObj *symbol)
{
  if(symbol->pixmap_buffer && symbol->renderer == renderer)
    return MS_SUCCESS;
  if(symbol->pixmap_buffer) { /* other renderer was used, start again */
    msFreeRasterBuffer(symbol->pixmap_buffer);
  } else {
    symbol->pixmap_buffer = (rasterBufferObj*)calloc(1,sizeof(rasterBufferObj));
  }
  if(MS_SUCCESS != renderer->loadImageFromFile(symbol->full_pixmap_path, symbol->pixmap_buffer)) {
    /* Free pixmap_buffer already allocated */
    free(symbol->pixmap_buffer);
    symbol->pixmap_buffer = NULL;
    return MS_FAILURE;
  }
  symbol->renderer = renderer;
  symbol->sizex = symbol->pixmap_buffer->width;
  symbol->sizey = symbol->pixmap_buffer->height;
  return MS_SUCCESS;

}

/***********************************************************************
 * msCopySymbol()                                                      *
 *                                                                     *
 * Copy a symbolObj, using mapfile.c:initSymbol(), msCopyPoint()       *
 * gdImageCreate(), gdImageCopy()                                      *
 **********************************************************************/

int msCopySymbol(symbolObj *dst, const symbolObj *src, mapObj *map)
{
  int i;

  initSymbol(dst);

  MS_COPYSTRING(dst->name, src->name);
  MS_COPYSTELEM(type);
  MS_COPYSTELEM(inmapfile);

  /* map is a special case */
  dst->map = map;

  MS_COPYSTELEM(sizex);
  MS_COPYSTELEM(sizey);
  MS_COPYSTELEM(anchorpoint_x);
  MS_COPYSTELEM(anchorpoint_y);

  for (i=0; i < src->numpoints; i++) {
    MS_COPYPOINT(&(dst->points[i]), &(src->points[i]));
  }

  MS_COPYSTELEM(numpoints);
  MS_COPYSTELEM(filled);

  MS_COPYSTRING(dst->imagepath, src->imagepath);
  MS_COPYSTELEM(transparent);
  MS_COPYSTELEM(transparentcolor);
  MS_COPYSTRING(dst->character, src->character);
  MS_COPYSTRING(dst->font, src->font);
  MS_COPYSTRING(dst->full_pixmap_path,src->full_pixmap_path);

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopySymbolSet()                                                   *
 *                                                                     *
 * Copy a symbolSetObj using msCopyFontSet(), msCopySymbol()           *
 **********************************************************************/

int msCopySymbolSet(symbolSetObj *dst, const symbolSetObj *src, mapObj *map)
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

static void get_bbox(pointObj *poiList, int numpoints, double *minx, double *miny, double *maxx, double *maxy)
{
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
symbolObj *msRotateVectorSymbol(symbolObj *symbol, double angle)
{
  double angle_rad=0.0;
  double cos_a, sin_a;
  double minx=0.0, miny=0.0, maxx=0.0, maxy=0.0;
  symbolObj *newSymbol = NULL;
  double dp_x, dp_y, xcor, ycor;
  double TOL=0.00000000001;
  int i;

  /* assert(symbol->type == MS_SYMBOL_VECTOR); */

  newSymbol = (symbolObj *) malloc(sizeof(symbolObj));
  msCopySymbol(newSymbol, symbol, NULL); /* TODO: do we really want to do this for all symbol types? */

  angle_rad = (MS_DEG_TO_RAD*angle);


  sin_a = sin(angle_rad);
  cos_a = cos(angle_rad);

  dp_x = symbol->sizex * .5; /* get the shift vector at 0,0 */
  dp_y = symbol->sizey * .5;

  /* center at 0,0 and rotate; then move back */
  for( i=0; i < symbol->numpoints; i++) {
    /* don't rotate PENUP commands (TODO: should use a constant here) */
    if ((symbol->points[i].x == -99.0) && (symbol->points[i].y == -99.0) ) {
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
    for( i=0; i < newSymbol->numpoints; i++) {
      if ((newSymbol->points[i].x == -99.0) && (newSymbol->points[i].y == -99.0))
        continue;
      newSymbol->points[i].x = newSymbol->points[i].x + xcor;
      newSymbol->points[i].y = newSymbol->points[i].y + ycor;
    }

    /* update the bbox to get the final dimension values for the symbol */
    get_bbox(newSymbol->points, newSymbol->numpoints, &minx, &miny, &maxx, &maxy);
  }

  newSymbol->sizex = maxx;
  newSymbol->sizey = maxy;
  return newSymbol;
}

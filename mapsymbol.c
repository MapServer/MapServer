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

extern unsigned char PNGsig[8];
extern unsigned char JPEGsig[3];


void freeImageCache(struct imageCacheObj *ic)
{
  if(ic) {
    freeImageCache(ic->next); /* free any children */
    gdImageDestroy(ic->img);
    free(ic);  
  }
  return;
}

int getCharacterSize(char *character, int size, char *font, rectObj *rect) {
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
      // if((s->position = getSymbol(3, MS_UC,MS_CC,MS_LC)) == -1) 
      //   return(-1);
      if((s->position = getSymbol(9, MS_UL,MS_UC,MS_UR,MS_CL,MS_CC,MS_CR,MS_LL,MS_LC,MS_LR)) == -1) 
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
      if((s->type = getSymbol(6,MS_SYMBOL_VECTOR,MS_SYMBOL_ELLIPSE,MS_SYMBOL_PIXMAP,MS_SYMBOL_SIMPLE,MS_TRUETYPE,MS_SYMBOL_CARTOLINE)) == -1)
	return(-1);	
#else
      if((s->type = getSymbol(5,MS_SYMBOL_VECTOR,MS_SYMBOL_ELLIPSE,MS_SYMBOL_PIXMAP,MS_SYMBOL_SIMPLE,MS_SYMBOL_CARTOLINE)) == -1)
	return(-1);
#endif
      if(s->type == MS_TRUETYPE) // TrueType keyword is valid several place in map files and symbol files, this simplifies the lexer
	s->type = MS_SYMBOL_TRUETYPE;
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

  fclose(msyyin);
  chdir(old_path);
  return(status);
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





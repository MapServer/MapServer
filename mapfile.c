#include <stdarg.h>

#include "map.h"
#include "mapfile.h"
#include "mapparser.h"

extern int yylex();
extern double yynumber;
extern char *yytext;
extern int yylineno;
extern FILE *yyin;

extern int yystate;
extern char *yystring;

/*
** Palette maniputation routines. Temporary until a good 24-bit
** graphics package for PNG shows up... Function returns a palette
** index value from 0 to MS_MAXCOLORS.
*/
int msAddColor(mapObj *map, int red, int green, int blue)
{
  int i, rd, bd, gd;
  double d, mind=0;
  int ci=-1;

  if((red == -255) || (green < -255) || (blue < -255))
    return(-255);

  if((red < 0) || (green < 0) || (blue < 0))
    return(-1);

  if(map->palette.numcolors == MS_MAXCOLORS-1) { /* no more room, find closest (leave room for the background color) */
    for(i=0; i<map->palette.numcolors; i++) {
      rd = map->palette.colors[i].red - red;
      gd = map->palette.colors[i].green - green;
      bd = map->palette.colors[i].blue - blue;
      d = rd*rd + gd*gd + bd*bd;
      if ((i == 0) || (d < mind)) {
	mind = d;
	ci = i;
      }
    }
    return(ci+1);
  } else { /* there is room, but check for dups */
    for(i=0; i<map->palette.numcolors; i++) {
      if((map->palette.colors[i].red == red) && (map->palette.colors[i].green == green) && (map->palette.colors[i].blue == blue))
	return(i+1);
    }
    map->palette.colors[map->palette.numcolors].red = red;
    map->palette.colors[map->palette.numcolors].green = green;
    map->palette.colors[map->palette.numcolors].blue = blue;
    map->palette.numcolors++;
    return(map->palette.numcolors);
  }
}

/*
** Applies a palette to a particular image
*/
int msLoadPalette(gdImagePtr img, paletteObj *palette, colorObj color)
{  
  int i;

  if(img == NULL) {
    msSetError(MS_GDERR, "Image not initialized.", "msLoadPalette()");
    return(-1);
  }

  /* allocate the background color */
  gdImageColorAllocate(img, color.red, color.green, color.blue);

  /* now the palette */
  for(i=0; i<palette->numcolors; i++)
    gdImageColorAllocate(img, palette->colors[i].red, palette->colors[i].green, palette->colors[i].blue);

  return(1);
}

/*
** Free memory allocated for a character array
*/
void msFreeCharArray(char **array, int num_items)
{
  int i;

  if((num_items == 0) || (array == NULL))
    return;

  for(i=0;i<num_items;i++)
    free(array[i]);
  free(array);
  
  return;
}

/*
** Checks symbol from lexer against variable length list of
** legal symbols.
*/
int getSymbol(int n, ...) {
  int symbol;
  va_list argp;
  int i=0;

  symbol = yylex();

  va_start(argp, n);
  while(i<n) { /* check each symbol in the list */
    if(symbol == va_arg(argp, int)) {
      va_end(argp);
      return(symbol);
    }
    i++;
  }

  va_end(argp);

  msSetError(MS_SYMERR, NULL, "getSymbol()"); 
  sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno); 

  return(-1);
}

/*
** Load a string from the map file. A "string" is defined
** in lexer.l.
*/
char *getString() {
  
  if(yylex() == MS_STRING)
    return(strdup(yytext));

  msSetError(MS_TYPEERR, NULL, "loadString()"); 
  sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno); 

  return(NULL);
}

/*
** Load a floating point number from the map file. (see lexer.l)
*/
int getDouble(double *d) {

  if(yylex() == MS_NUMBER) {
    *d = yynumber;
    return(0); /* success */
  }

  msSetError(MS_TYPEERR, NULL, "getDouble()"); 
  sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno); 
  
  return(-1);
}

/*
** Load a integer from the map file. (see lexer.l)
*/
int getInteger(int *i) {

  if(yylex() == MS_NUMBER) {
    *i = (int)yynumber;
    return(0); /* success */
  }

  msSetError(MS_TYPEERR, NULL, "getInteger()"); 
  sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno); 

  return(-1);
}

int getCharacter(char *c) {
  if(yylex() == MS_STRING) {
    *c = yytext[0];
    return(0);
  }

  msSetError(MS_TYPEERR, NULL, "getCharacter()"); 
  sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno); 

  return(-1);
}

int msGetSymbolIndex(symbolSetObj *set, char *name)
{
  int i;

  if(!name) return(-1);

  for(i=0; i<set->numsymbols; i++) {
    if(set->symbol[i].name)
      if(strcasecmp(set->symbol[i].name, name) == 0) return(i);
  }

  return(-1); /* symbol not found */
}

/*
** Return the index number for a given layer based on its name.
*/
int msGetLayerIndex(mapObj *map, char *name)
{
  int i;

  if(name == NULL)
    return(-1);

  for(i=0;i<map->numlayers; i++) {
    if(!map->layers[i].name) /* skip it */
      continue;
    if(strcmp(name, map->layers[i].name) == 0)
      return(i);
  }
  return(-1);
}

/* inserts a feature at the start of the list rather than at the end */
struct featureObj *addFeature(struct featureObj *list) {
  struct featureObj *f;

  if((f = (struct featureObj *)malloc(sizeof(struct featureObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "initFeature()");
    return(NULL);
  }
  
  f->shape.line = NULL; /* nothing loaded yet */
  f->shape.numlines = 0;
  f->class = NULL;
  f->text = NULL;
  f->next = list;
 
  return(f);
}

struct featureObj *initFeature() {
  struct featureObj *f;

  if((f = (struct featureObj *)malloc(sizeof(struct featureObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "initFeature()");
    return(NULL);
  }

  f->shape.line = NULL; /* nothing loaded yet */
  f->shape.numlines = 0;
  f->class = NULL;
  f->text = NULL;
  f->next = NULL;

  return(f);
}

void freeFeature(struct featureObj *list)
{
  if(list) {
    freeFeature(list->next); /* free any children */
    msFreeShape(&list->shape);
    free(list->class);
    free(list->text);
    free(list);  
  }
  return;
}

/* lineObj = multipointObj */
static int loadFeaturePoints(lineObj *points)
{
  int buffer_size=0;

  if((points->point = (pointObj *)malloc(sizeof(pointObj)*MS_FEATUREINITSIZE)) == NULL) {
    msSetError(MS_MEMERR, NULL, "loadFeaturePoints()");
    return(-1);
  }
  points->numpoints = 0;
  buffer_size = MS_FEATUREINITSIZE;

  for(;;) {
    switch(yylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadFeaturePoints()");      
      return(-1);
    case(END):
      return(0);
    case(MS_NUMBER):
      if(points->numpoints == buffer_size) { /* just add it to the end */
	points->point = (pointObj *) realloc(points->point, sizeof(pointObj)*(buffer_size+MS_FEATUREINCREMENT));    
	if(points->point == NULL) {
	  msSetError(MS_MEMERR, "Realloc() error.", "loadFeaturePoints()");
	  return(-1);
	}   
	buffer_size+=MS_FEATUREINCREMENT;
      }

      points->point[points->numpoints].x = atof(yytext);
      if(getDouble(&(points->point[points->numpoints].y)) == -1) return(-1);

      points->numpoints++;
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadFeaturePoints()");    
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);      
    }
  }
}

static int loadFeature(struct featureObj *feature)
{
  multipointObj points={0,NULL};

  for(;;) {
    switch(yylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadFeature()");      
      return(-1);
    case(END):
      return(0);
    case(CLASS):
      if((feature->class = getString()) == NULL) return(-1);
      break;
    case(POINTS):      
      if(loadFeaturePoints(&points) == -1) return(-1);
      if(msAddLine(&feature->shape, &points) == -1) return(-1);

      free(points.point); /* reset */
      points.numpoints = 0;
      break;
    case(TEXT):
      if((feature->text = getString()) == NULL) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadfeature()");    
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  } /* next token */  
}


/*
** Initialize, load and free a projectionObj structure
*/
static int initProjection(projectionObj *p)
{
#ifdef USE_PROJ  
  p->numargs = 0;
  p->proj = NULL;
  if((p->projargs = (char **)malloc(MS_MAXPROJARGS*sizeof(char *))) == NULL) {
    msSetError(MS_MEMERR, NULL, "initProjection()");
    return(-1);
  }
#endif
  return(0);

}

static void freeProjection(projectionObj *p) {
#ifdef USE_PROJ
  if(p->proj) pj_free(p->proj);
  msFreeCharArray(p->projargs, p->numargs);  
  p->numargs = 0;
#endif
}

static int loadProjection(projectionObj *p)
{
#ifdef USE_PROJ
  int i=0;

  for(;;) {
    switch(yylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadProjection()");      
       return(-1);
    case(END):
      p->numargs = i;
      if(strcasecmp(p->projargs[0], "GEOGRAPHIC") == 0) {
	p->proj = NULL;
      } else {
	if( !(p->proj = pj_init(p->numargs, p->projargs)) ) {
	  msSetError(MS_PROJERR, NULL, "loadProjection()");      
	  sprintf(ms_error.message, "%s", pj_strerrno(pj_errno));
	  return(-1);
	}
      }      
      return(0);
      break;    
    case(MS_STRING):
      p->projargs[i] = strdup(yytext);
      i++;
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadProjection()");
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  } /* next token */
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "loadProjection()");
  return(-1);
#endif
}

int loadProjectionString(projectionObj *p, char *value)
{
#ifdef USE_PROJ
  if(p) freeProjection(p);
  p->projargs = split(value, ',', &p->numargs);      
  if(strcasecmp(p->projargs[0], "GEOGRAPHIC") == 0) {
    p->proj = NULL;
  } else {
    if( !(p->proj = pj_init(p->numargs, p->projargs)) ) {
      msSetError(MS_PROJERR, NULL, "loadProjectionString()");
      sprintf(ms_error.message, "%s", pj_strerrno(pj_errno));
      return(-1);
    }
  }

  return(0);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "loadProjectionString()");
  return(-1);
#endif
}

/*
** Initialize, load and free a labelObj structure
*/
static void initLabel(labelObj *label)
{
  label->antialias = -1; /* off */

  label->color = 1;
  label->outlinecolor = -1; /* don't use it */
  label->shadowcolor = -1; /* don't use it */
  label->shadowsizex = label->shadowsizey = 1;

  label->font = NULL;
  label->type = MS_BITMAP;
  label->size = MS_MEDIUM;
  label->position = MS_CC;
  label->angle = 0;
  label->autoangle = MS_FALSE;
  label->minsize = MS_MINFONTSIZE;
  label->maxsize = MS_MAXFONTSIZE;
  label->buffer = 0;
  label->offsetx = label->offsety = 0;

  label->minfeaturesize = -1; /* no limit */
  label->autominfeaturesize = MS_FALSE;
  label->mindistance = -1; /* no limit */
  label->partials = MS_TRUE;
  label->wrap = '\n';

  label->force = MS_FALSE;
  return;
}

static void freeLabel(labelObj *label)
{
  free(label->font);
}

static int loadLabel(labelObj *label, mapObj *map)
{
  int red, green, blue;
  int symbol;

  for(;;) {
    switch(yylex()) {
    case(ANGLE):
      if((symbol = getSymbol(2, MS_NUMBER,MS_AUTO)) == -1) 
	return(-1);

      if(symbol == MS_NUMBER)
	label->angle = MS_DEG_TO_RAD*yynumber;
      else
	label->autoangle = MS_TRUE;
      break;
    case(ANTIALIAS):
      label->antialias = 1;
      break;
    case(BUFFER):
      if(getInteger(&(label->buffer)) == -1) return(-1);
      break;
    case(COLOR): 
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      label->color = msAddColor(map,red,green,blue);      
      break;
    case(END):
      return(0);
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadLabel()");      
      return(-1);
    case(FONT):
#ifdef USE_TTF
      if((label->font = getString()) == NULL) return(-1);
#else
      msSetError(MS_IDENTERR, "Keyword FONT is not valid without TrueType font support.", "loadlabel()");    
      return(-1);
#endif
      break;
    case(FORCE):
      if((label->force = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return(-1);
      break;
    case(MAXSIZE):      
      if(getInteger(&(label->maxsize)) == -1) return(-1);
      break;
    case(MINDISTANCE):      
      if(getInteger(&(label->mindistance)) == -1) return(-1);
      break;
    case(MINFEATURESIZE):
      if((symbol = getSymbol(2, MS_NUMBER,MS_AUTO)) == -1) 
	return(-1);

      if(symbol == MS_NUMBER)
	label->minfeaturesize = yynumber;
      else
	label->autominfeaturesize = MS_TRUE;
      break;
    case(MINSIZE):
      if(getInteger(&(label->minsize)) == -1) return(-1);
      break;
    case(OFFSET):
      if(getInteger(&(label->offsetx)) == -1) return(-1);
      if(getInteger(&(label->offsety)) == -1) return(-1);
      break;
    case(OUTLINECOLOR):
    case(BACKGROUNDCOLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      label->outlinecolor = msAddColor(map,red,green,blue);
      break;    
    case(PARTIALS):
      if((label->partials = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return(-1);
      break;
    case(POSITION):
      if((label->position = getSymbol(10, MS_UL,MS_UC,MS_UR,MS_CL,MS_CC,MS_CR,MS_LL,MS_LC,MS_LR,MS_AUTO)) == -1) 
	return(-1);
      break;
    case(SHADOWCOLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      label->shadowcolor = msAddColor(map,red,green,blue);
      break;
    case(SHADOWSIZE):
      if(getInteger(&(label->shadowsizex)) == -1) return(-1);
      if(getInteger(&(label->shadowsizey)) == -1) return(-1);
      break;
    case(SIZE):
#ifdef USE_TTF
      if((label->size = getSymbol(6, MS_NUMBER,MS_TINY,MS_SMALL,MS_MEDIUM,MS_LARGE,MS_GIANT)) == -1) 
	return(-1);
      if(label->size == MS_NUMBER)
	label->size = yynumber;
#else
      if((label->size = getSymbol(5, MS_TINY,MS_SMALL,MS_MEDIUM,MS_LARGE,MS_GIANT)) == -1) 
	return(-1);
#endif
      break; 
    case(TYPE):
      if((label->type = getSymbol(2, MS_TRUETYPE,MS_BITMAP)) == -1) return(-1);
      break;    
    case(WRAP):
      if(getCharacter(&(label->wrap)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadlabel()");    
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  } /* next token */
}

static void loadLabelString(mapObj *map, labelObj *label, char *value)
{
  int red, green, blue;
  int symbol;

  switch(yylex()) {
  case(ANGLE):
    yystate = 2; yystring = value;
    label->autoangle = MS_FALSE;
    if((symbol = getSymbol(2, MS_NUMBER,MS_AUTO)) == -1) 
      return;
    
    if(symbol == MS_NUMBER)
      label->angle = MS_DEG_TO_RAD*yynumber;
    else
      label->autoangle = MS_TRUE;
    break;
  case(ANTIALIAS):
    label->antialias = 1;
    break;  
  case(BUFFER):
    yystate = 2; yystring = value;
    if(getInteger(&(label->buffer)) == -1) break;
    break;
  case(COLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    label->color =msAddColor(map,red,green,blue);     
    break;    
  case(FONT):
    free(label->font);
    label->font = strdup(value);

#ifdef USE_TTF
    free(label->font);
    label->font = strdup(value);
    
    if(!(msLookupHashTable(map->fontset.fonts, label->font))) {
      msSetError(MS_IDENTERR, "Unknown font alias.", "loadLabelString()");    
      sprintf(ms_error.message, "(%s)", yytext);
      return;
    }
#else
    msSetError(MS_IDENTERR, "Keyword FONT is not valid without TrueType font support.", "loadLabelString()");    
    return;
#endif

    break;
  case(FORCE):
      yystate = 2; yystring = value;
      if((label->force = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
      break; 
  case(MAXSIZE):
    yystate = 2; yystring = value;
    if(getInteger(&(label->maxsize)) == -1) return;
    break;
  case(MINDISTANCE):
    yystate = 2; yystring = value;
    if(getInteger(&(label->mindistance)) == -1) return;
    break;
  case(MINFEATURESIZE):
    yystate = 2; yystring = value;
    if(getInteger(&(label->minfeaturesize)) == -1) return;
    break;
  case(MINSIZE):
    yystate = 2; yystring = value;
    if(getInteger(&(label->minsize)) == -1) return;
    break;
  case(OFFSET):
    yystate = 2; yystring = value;
    if(getInteger(&(label->offsetx)) == -1) return;
    if(getInteger(&(label->offsety)) == -1) return;
    break;  
  case(OUTLINECOLOR):
  case(BACKGROUNDCOLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    label->outlinecolor =msAddColor(map,red,green,blue);
    break;
  case(PARTIALS):
    yystate = 2; yystring = value;
    if((label->partials = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
    break;
  case(POSITION):
    yystate = 2; yystring = value;
    if((label->position = getSymbol(10, MS_UL,MS_UC,MS_UR,MS_CL,MS_CC,MS_CR,MS_LL,MS_LC,MS_LR,MS_AUTO)) == -1) return;
    break;
  case(SHADOWCOLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    label->shadowcolor =msAddColor(map,red,green,blue);
    break;
  case(SHADOWSIZE):
    yystate = 2; yystring = value;
    if(getInteger(&(label->shadowsizex)) == -1) return;
    if(getInteger(&(label->shadowsizey)) == -1) return;
    break;
  case(SIZE):
    yystate = 2; yystring = value;
#ifdef USE_TTF
    if((label->size = getSymbol(6, MS_NUMBER,MS_TINY,MS_SMALL,MS_MEDIUM,MS_LARGE,MS_GIANT)) == -1) return;
    if(label->size == MS_NUMBER)
      label->size = yynumber;
#else
    if((label->size = getSymbol(5, MS_TINY,MS_SMALL,MS_MEDIUM,MS_LARGE,MS_GIANT)) == -1) return;
#endif
    break;
  case(TYPE):
    yystate = 2; yystring = value;
    if((label->type = getSymbol(2, MS_TRUETYPE,MS_BITMAP)) == -1) return;
    break;    
  case(WRAP):
    if(value) label->wrap = value[0];
  default:
    break;
  }
}

void initExpression(expressionObj *exp)
{
  exp->type = MS_STRING;
  exp->string = NULL;
  exp->items = NULL;
  exp->indexes = NULL;
  exp->numitems = 0;
}

void freeExpression(expressionObj *exp)
{
  if(!exp) return;

  free(exp->string);
  if(exp->type == MS_REGEX)
    regfree(&(exp->regex));
  if(exp->type == MS_EXPRESSION) {
    msFreeCharArray(exp->items, exp->numitems);
    free(exp->indexes);
  }
}

int loadExpression(expressionObj *exp)
{
  if((exp->type = getSymbol(3, MS_STRING,MS_EXPRESSION,MS_REGEX)) == -1) return(-1);
  exp->string = strdup(yytext);
  
  if(exp->type == MS_REGEX) {
    if(regcomp(&(exp->regex), exp->string, REG_EXTENDED|REG_NOSUB) != 0) { // compile the expression 
      msSetError(MS_REGEXERR, NULL, "loadExpression()");
      sprintf(ms_error.message, "(%s):(%d)", exp->string, yylineno);
      return(-1);
    }
  }

  return(0);
}

int loadExpressionString(expressionObj *exp, char *value)
{
  yystate = 2; yystring = value;

  freeExpression(exp); // we're totally replacing the old expression so free then init to start over
  initExpression(exp);

  if((exp->type = getSymbol(3, MS_STRING,MS_EXPRESSION,MS_REGEX)) == -1) return(-1);
  exp->string = strdup(yytext);
  
  if(exp->type == MS_REGEX) {
    if(regcomp(&(exp->regex), exp->string, REG_EXTENDED|REG_NOSUB) != 0) { // compile the expression 
      msSetError(MS_REGEXERR, NULL, "loadExpression()");
      sprintf(ms_error.message, "(%s):(%d)", exp->string, yylineno);
      return(-1);
    }
  }

  return(0); 
}

/*
** Initialize, load and free a single class
*/
int initClass(classObj *class)
{
  initExpression(&(class->expression));
  class->name = NULL;
  initExpression(&(class->text));
  class->color = -1; /* must explictly set a color */
  class->symbol = 0;
  class->size = 1; /* one pixel */
  class->minsize = MS_MINSYMBOLSIZE;
  class->maxsize = MS_MAXSYMBOLSIZE;
  class->backgroundcolor = -1;
  class->outlinecolor = -1;
  initLabel(&(class->label));
  class->symbolname = NULL;

  return(0);
}

void freeClass(classObj *class)
{
  freeLabel(&(class->label));
  freeExpression(&(class->expression));
  freeExpression(&(class->text));
  free(class->name);
  free(class->symbolname);
}

int loadClass(classObj *class, mapObj *map)
{
  int red, green, blue;
  int state;

  initClass(class);

  for(;;) {
    switch(yylex()) {
    case(BACKGROUNDCOLOR): 
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      class->backgroundcolor = msAddColor(map, red, green, blue);
      break;
    case(COLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      class->color = msAddColor(map, red, green, blue);
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadClass()");
      return(-1);
    case(END):
      return(0);
      break;
    case(EXPRESSION):
      if(loadExpression(&(class->expression)) == -1) return(-1);
      break;
    case(LABEL):
      if(loadLabel(&(class->label), map) == -1) return(-1);
      break;
    case(MAXSIZE):
      if(getInteger(&(class->maxsize)) == -1) return(-1);
      break;
    case(MINSIZE):      
      if(getInteger(&(class->minsize)) == -1) return(-1);
      break;
    case(NAME):
      if((class->name = getString()) == NULL) return(-1);
      break; 
    case(OUTLINECOLOR):      
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      class->outlinecolor = msAddColor(map, red, green, blue);
      break;
    case(SIZE):
      if(getInteger(&(class->size)) == -1) return(-1);
      break;
    case(SYMBOL):
      if((state = getSymbol(2, MS_NUMBER,MS_STRING)) == -1) return(-1);

      if(state == MS_NUMBER)
	class->symbol = (int) yynumber;
      else
	class->symbolname = strdup(yytext);
      break;
    case(TEXT):
      if(loadExpression(&(class->text)) == -1) return(-1);
      if((class->text.type != MS_STRING) && (class->text.type != MS_EXPRESSION)) {
	msSetError(MS_MISCERR, "Text expressions support constant or replacement strings." , "loadClass()");
	return(-1);
      }
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadClass()");    
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  }
}

static void loadClassString(mapObj *map, classObj *class, char *value, int type)
{
  int red, green, blue;
  int state;

  switch(yylex()) {
  case(BACKGROUNDCOLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    class->backgroundcolor = msAddColor(map, red, green, blue);
    break;
  case(COLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    class->color = msAddColor(map, red, green, blue);
    break;
  case(EXPRESSION):    
    loadExpressionString(&(class->expression), value);
    break;
  case(LABEL):
    loadLabelString(map, &(class->label), value);
    break;
  case(MAXSIZE):
    yystate = 2; yystring = value;
    getInteger(&(class->maxsize));
    break;
  case(MINSIZE):
    yystate = 2; yystring = value;
    getInteger(&(class->minsize));
    break;
  case(OUTLINECOLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    class->outlinecolor = msAddColor(map, red, green, blue);
    break;
  case(SIZE):
    yystate = 2; yystring = value;
    getInteger(&(class->size));
    break;
  case(SYMBOL):
    yystate = 2; yystring = value;
    if((state = getSymbol(2, MS_NUMBER,MS_STRING)) == -1) return;

    if(state == MS_NUMBER)
      class->symbol = (int) yynumber;
    else {
      switch(type) {
      case(MS_ANNOTATION):
      case(MS_POINT):
	if((class->symbol = msGetSymbolIndex(&(map->markerset), yytext)) == -1) msSetError(MS_EOFERR, "Undefined symbol.", "loadClassString()");
	break;
      case(MS_LINE):
      case(MS_POLYLINE):
	if((class->symbol = msGetSymbolIndex(&(map->lineset), yytext)) == -1) msSetError(MS_EOFERR, "Undefined symbol.", "loadClassString()");
	break;
      case(MS_POLYGON):
	if((class->symbol = msGetSymbolIndex(&(map->shadeset), yytext)) == -1) msSetError(MS_EOFERR, "Undefined symbol.", "loadClassString()");
	break;
      default:
	break;
      }      
    }
    break;
  case(TEXT):
    if(loadExpressionString(&(class->text), value) == -1) return;
    if((class->text.type != MS_STRING) && (class->text.type != MS_EXPRESSION)) msSetError(MS_MISCERR, "Text expressions support constant or replacement strings." , "loadClass()");
  default:
    break;
  }

  return;
}

/*
** Initialize, load and free a single join
*/
void initJoin(joinObj *join)
{
  join->numitems = 0;

  join->name = NULL; /* unique join name, used for variable substitution */

  join->items = NULL; /* array to hold item names for the joined table */
  join->data = NULL; /* arrays of strings to holds 1 or more records worth of data */
  join->numrecords = 0;

  join->table = NULL;

  join->match = NULL;

  join->from = NULL; /* join items */
  join->to = NULL;

  join->header = NULL;
  join->template = NULL; /* only html type templates are supported */
  join->footer = NULL;

  join->type = MS_SINGLE; /* one-to-one */
}

void freeJoin(joinObj *join) 
{
  int i;

  free(join->name);

  free(join->table);

  free(join->from);
  free(join->to);

  free(join->match);

  free(join->header);
  free(join->template);
  free(join->footer);

  msFreeCharArray(join->items, join->numitems); /* these may have been free'd elsewhere */
  for(i=0; i<join->numrecords; i++)
    msFreeCharArray(join->data[i], join->numitems);
  free(join->data);
}

int loadJoin(joinObj *join)
{
  initJoin(join);

  for(;;) {
    switch(yylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadJoin()");
      return(-1);
    case(END):      
      if((join->from == NULL) || (join->to == NULL) || (join->table == NULL)) {
	msSetError(MS_EOFERR, "Join must define table, name, from and to properties.", "loadJoin()");
	return(-1);
      }
      if((join->type == MS_MULTIPLE) && ((join->template == NULL) || (join->name == NULL))) {
	msSetError(MS_EOFERR, "One-to-many joins must define template and name properties.", "loadJoin()");
	return(-1);
      }
      return(0);
    case(FOOTER):
      if((join->footer = getString()) == NULL) return(-1);
      break;
    case(FROM):
      if((join->from = getString()) == NULL) return(-1);
      break;      
    case(HEADER):
      if((join->header = getString()) == NULL) return(-1);
      break;
    case(NAME):
      if((join->name = getString()) == NULL) return(-1);
      break;
    case(TABLE):
      if((join->table = getString()) == NULL) return(-1);
      break;
    case(TEMPLATE):
      if((join->template = getString()) == NULL) return(-1);
      break;
    case(TO):
      if((join->to = getString()) == NULL) return(-1);
      break;
    case(TYPE):
      if((join->type = getSymbol(2, MS_SINGLE, MS_MULTIPLE)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadJoin()");
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  } /* next token */
}

/*
** Initialize, load and free a single query
*/
int initQuery(queryObj *query)
{
  initExpression(&(query->expression));
  query->numjoins = 0;
  query->template = NULL;
  if((query->joins = (joinObj *)malloc(MS_MAXJOINS*sizeof(joinObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "initQuery()");
    return(-1);
  }

  query->items = NULL;
  query->numitems = 0;
  query->data = NULL;

  return(0);
}

void freeQuery(queryObj *query) 
{
  int i;
  
  freeExpression(&(query->expression));
  free(query->template);
  for(i=0;i<query->numjoins;i++) /* each join */    
    freeJoin(&(query->joins[i]));
  free(query->joins);

  msFreeCharArray(query->items, query->numitems); /* these may have been free'd elsewhere */
  msFreeCharArray(query->data, query->numitems);
}

int loadQuery(queryObj *query)
{
  int j=0;

  if(initQuery(query) == -1)
    return(-1);

  for(;;) {
    switch(yylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadQuery()");
      return(-1);
    case(END):
      if(query->template == NULL) {
	msSetError(MS_MISCERR, "No query template specified.", "loadQuery()");
	return(-1);
      }
      query->numjoins = j;
      return(0);
      break;
    case(EXPRESSION):
      if(loadExpression(&(query->expression)) == -1) return(-1);
      break;
    case(JOIN):
      if(loadJoin(&(query->joins[j])) == -1) return(-1);
      j++;
      break;
    case(TEMPLATE):      
      if((query->template = getString()) == NULL) return(-1);
      break;    
    default:
      msSetError(MS_IDENTERR, NULL, "loadQuery()");    
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  } /* next token */
}

void loadQueryString(queryObj *query, char *value)
{
  switch(yylex()) {
  case(EXPRESSION):
    loadExpressionString(&(query->expression), value);
    break;
  case(TEMPLATE):
    free(query->template);
    query->template = strdup(value);
    break;
  }

  return;
}

/*
** Initialize, load and free a single layer structure
*/
int initLayer(layerObj *layer)
{

  if((layer->class = (classObj *)malloc(sizeof(classObj)*MS_MAXCLASSES)) == NULL) {
    msSetError(MS_MEMERR, NULL, "initLayer()");
    return(-1);
  }
  if((layer->query = (queryObj *)malloc(sizeof(queryObj)*MS_MAXQUERIES)) == NULL) {
    msSetError(MS_MEMERR, NULL, "initLayer()");
    return(-1);
  }

  layer->name = NULL;
  layer->group = NULL;
  layer->description = NULL;
  layer->legend = NULL;
  layer->status = MS_OFF;
  layer->data = NULL;
  layer->type = MS_POINT;

  layer->toleranceunits = MS_PIXELS;
  layer->tolerance = 3; /* perhaps this should have a different value based on type */

  layer->symbolscale = -1.0; /* -1 means nothing is set */
  layer->maxscale = -1.0;
  layer->minscale = -1.0;

  layer->maxfeatures = -1; /* no quota */

  layer->queryitem = NULL;
  layer->numqueries = 0;

  layer->header = NULL;  
  layer->footer = NULL;

  layer->annotate = MS_FALSE;
  layer->transform = MS_TRUE;

  layer->classitem = NULL;
  layer->numclasses = 0;

  if(initProjection(&(layer->projection)) == -1)
    return(-1);

  layer->offsite = -1;

  layer->labelcache = MS_ON;
  layer->postlabelcache = MS_FALSE;

  layer->labelitem = NULL;  
  layer->labelsizeitem = NULL;  
  layer->labelangleitem = NULL;  
  layer->labelmaxscale = -1;
  layer->labelminscale = -1;

  layer->tileitem = strdup("location");
  layer->tileindex = NULL;

  layer->features = NULL;

  layer->connection = NULL;
  layer->connectiontype = MS_LOCAL;

  return(0);
}

void freeLayer(layerObj *layer) {
  int i;

  free(layer->name);
  free(layer->group);
  free(layer->data);
  free(layer->description);
  free(layer->legend);
  free(layer->classitem);
  free(layer->labelitem);
  free(layer->labelsizeitem);
  free(layer->labelangleitem);
  free(layer->queryitem);
  free(layer->header);
  free(layer->footer);
  free(layer->tileindex);
  free(layer->tileitem);
  free(layer->connection);

  freeProjection(&(layer->projection));

  for(i=0;i<layer->numqueries;i++)
    freeQuery(&(layer->query[i]));
  free(layer->query);

  for(i=0;i<layer->numclasses;i++)
    freeClass(&(layer->class[i]));
  free(layer->class);

  if(layer->features)
    freeFeature(layer->features);
}

int loadLayer(layerObj *layer, mapObj *map)
{
  int q=0, c=0; /* query and class counters */

  if(initLayer(layer) == -1)
    return(-1);

  for(;;) {
    switch(yylex()) {
    case(CLASS):
      if(loadClass(&(layer->class[c]), map) == -1) return(-1);
      if(layer->class[c].text.string) layer->annotate = MS_TRUE;
      c++;
      break;
    case(CLASSITEM):
      if((layer->classitem = getString()) == NULL) return(-1);
      break;
    case(CONNECTION):
      if((layer->connection = getString()) == NULL) return(-1);
      break;
    case(CONNECTIONTYPE):
      if((layer->connectiontype = getSymbol(2, MS_LOCAL, MS_SDE)) == -1) return(-1);
      break;
    case(DATA):
      if((layer->data = getString()) == NULL) return(-1);
      break;
    case(DESCRIPTION):
      if((layer->description = getString()) == NULL) return(-1);
      break;  
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadLayer()");      
      return(-1);
      break;
    case(END):
      layer->numclasses = c;
      layer->numqueries = q;

      if(layer->numclasses > 1 && !layer->classitem) {
	msSetError(MS_MISCERR, "Multiple classes defined but no classitem?", "loadLayer()");      
	return(-1);
      }

      return(0);
      break;
    case(FEATURE):
      if(layer->features == NULL) {
	if((layer->features = initFeature()) == NULL) return(-1); /* create initial feature */
      } else {
	if((layer->features = addFeature(layer->features)) == NULL) return(-1); /* add an empty feature */
      }
      if(loadFeature(layer->features) == -1) return(-1);
      if(layer->features->text) layer->annotate = MS_TRUE;
      break;
    case(FOOTER):
      if((layer->footer = getString()) == NULL) return(-1);
      break;
    case(GROUP):
      if((layer->group = getString()) == NULL) return(-1);
      break;
    case(HEADER):      
      if((layer->header = getString()) == NULL) return(-1);
      break;
    case(LABELANGLEITEM):
      if((layer->labelangleitem = getString()) == NULL) return(-1);
      break;
    case(LABELCACHE):
      if((layer->labelcache = getSymbol(2, MS_ON, MS_OFF)) == -1) return(-1);
      break;
    case(LABELITEM):
      if((layer->labelitem = getString()) == NULL) return(-1);
      layer->annotate = MS_TRUE;
      break;
    case(LABELMAXSCALE):      
      if(getDouble(&(layer->labelmaxscale)) == -1) return(-1);
      break;
    case(LABELMINSCALE):      
      if(getDouble(&(layer->labelminscale)) == -1) return(-1);
      break;    
    case(LABELSIZEITEM):
      if((layer->labelsizeitem = getString()) == NULL) return(-1);
      break;
    case(LEGEND):
      if((layer->legend = getString()) == NULL) return(-1);
      break;
    case(MAXFEATURES):
      if(getInteger(&(layer->maxfeatures)) == -1) return(-1);
      break;
    case(MAXSCALE):      
      if(getDouble(&(layer->maxscale)) == -1) return(-1);
      break;
    case(MINSCALE):      
      if(getDouble(&(layer->minscale)) == -1) return(-1);
      break;
    case(NAME):
      if((layer->name = getString()) == NULL) return(-1);
      break;
    case(OFFSITE):
      if(getInteger(&(layer->offsite)) == -1) return(-1); /* using an index rather than an actual color */        
      break;
    case(POSTLABELCACHE):
      if((layer->postlabelcache = getSymbol(2, MS_TRUE, MS_FALSE)) == -1) return(-1);
      if(layer->postlabelcache)
	layer->labelcache = MS_OFF;
      break;
    case(PROJECTION):
      if(loadProjection(&(layer->projection)) == -1) return(-1);
      break;
    case(QUERY):
      if(loadQuery(&(layer->query[q])) == -1) return(-1);
      q++;
      break;
    case(QUERYITEM):
      if((layer->queryitem = getString()) == NULL) return(-1);
      break;
    case(STATUS):
      if((layer->status = getSymbol(4, MS_ON,MS_OFF,MS_QUERY,MS_DEFAULT)) == -1) return(-1);
      break;
    case(SYMBOLSCALE):      
      if(getDouble(&(layer->symbolscale)) == -1) return(-1);
      break;
    case(TILEINDEX):
      if((layer->tileindex = getString()) == NULL) return(-1);
      break;
    case(TILEITEM):
      free(layer->tileitem); /* free initialization */
      if((layer->tileitem = getString()) == NULL) return(-1);
      break;
    case(TOLERANCE):
      if(getDouble(&(layer->tolerance)) == -1) return(-1);
      break;
    case(TOLERANCEUNITS):
      if((layer->toleranceunits = getSymbol(7, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD,MS_PIXELS)) == -1) return(-1);
      break;
    case(TRANSFORM):
      if((layer->transform = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return(-1);
      break;
    case(TYPE):
      if((layer->type = getSymbol(6, MS_POINT,MS_LINE,MS_RASTER,MS_POLYGON,MS_POLYLINE,MS_ANNOTATION)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadLayer()");
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  } /* next token */
}

static void loadLayerString(mapObj *map, layerObj *layer, char *value)
{
  int i=0,buffer_size=0;
  multipointObj points={0,NULL};
  int done=0;

  switch(yylex()) {
  case(CLASS):
    if(layer->numclasses != 1) { /* if only 1 class no need for index */
      if(getInteger(&i) == -1) break;
      if((i < 0) || (i >= layer->numclasses))
        break;
    }
    loadClassString(map, &(layer->class[i]), value, layer->type);
    if(layer->class[i].text.string) layer->annotate = MS_TRUE;
    break;
  case(CLASSITEM):
    free(layer->classitem);
    layer->classitem = strdup(value);
    break;
  case(DATA):
    free(layer->data);
    layer->data = strdup(value);
    break;
  case(DESCRIPTION):
    free(layer->description);
    layer->description = strdup(value);
    break;
  case(FEATURE):    
    
    switch(yylex()) {      
    case(POINTS):
      yystate = 2; yystring = value;

      if(layer->features == NULL)
	if((layer->features = initFeature()) == NULL) return; /* create initial feature */

      if((points.point = (pointObj *)malloc(sizeof(pointObj)*MS_FEATUREINITSIZE)) == NULL) {
	msSetError(MS_MEMERR, NULL, "loadLayerString()");
	return;
      }
      points.numpoints = 0;
      buffer_size = MS_FEATUREINITSIZE;
            
      while(!done) {
	switch(yylex()) {	  
	case(MS_NUMBER):
	  if(points.numpoints == buffer_size) { /* just add it to the end */
	    points.point = (pointObj *) realloc(points.point, sizeof(pointObj)*(buffer_size+MS_FEATUREINCREMENT));    
	    if(!points.point) {
	      msSetError(MS_MEMERR, "Realloc() error.", "loadlayerString()");
	      return;
	    }   
	    buffer_size+=MS_FEATUREINCREMENT;
	  }
	  
	  points.point[points.numpoints].x = atof(yytext);
	  if(getDouble(&(points.point[points.numpoints].y)) == -1) return;
	  
	  points.numpoints++;	  
	  break;
	default: /* \0 */
	  done=1;
	  break;
	}	
      }

      if(msAddLine(&(layer->features->shape), &points) == -1) break;

      break;
    case(CLASS):
      layer->features->class = strdup(value);
      break;
    case(TEXT):
      layer->annotate = MS_TRUE;
      layer->features->text = strdup(value);
      break;
    default:
      if(layer->features == NULL) {
	if((layer->features = initFeature()) == NULL) return; /* create initial feature */
      } else {
	if((layer->features = addFeature(layer->features)) == NULL) return; /* add an empty feature */
      }
      break;
    }

    break;
  case(FOOTER):
    free(layer->footer);
    layer->footer = strdup(value);
    break;
  case(HEADER):      
    free(layer->header);
    layer->header = strdup(value);
    break;
  case(LABELANGLEITEM):
    free(layer->labelangleitem);
    layer->labelangleitem = strdup(value);
    break;
  case(LABELCACHE):
    yystate = 2; yystring = value;
    if((layer->labelcache = getSymbol(2, MS_ON, MS_OFF)) == -1) return;
    break;
  case(LABELITEM):
    free(layer->labelitem);
    layer->labelitem = strdup(value);
    layer->annotate = MS_TRUE;
    break;
  case(LABELMAXSCALE):
    yystate = 2; yystring = value;
    if(getDouble(&(layer->labelmaxscale)) == -1) return;
    break;
  case(LABELMINSCALE):
    yystate = 2; yystring = value;
    if(getDouble(&(layer->labelminscale)) == -1) return;
    break;    
  case(LABELSIZEITEM):
    free(layer->labelsizeitem);
    layer->labelsizeitem = strdup(value);
    break;
  case(LEGEND):
    free(layer->legend);
    layer->legend = strdup(value);
    break;
  case(MAXFEATURES):
    yystate = 2; yystring = value;
    if(getInteger(&(layer->maxfeatures)) == -1) return;
    break;
  case(MAXSCALE):
    yystate = 2; yystring = value;
    if(getDouble(&(layer->maxscale)) == -1) return;
    break;
  case(MINSCALE):
    yystate = 2; yystring = value;
    if(getDouble(&(layer->minscale)) == -1) return;
    break;    
  case(OFFSITE):
    yystate = 2; yystring = value;
    if(getInteger(&(layer->offsite)) == -1) return; /* using an index rather than an actual color */        
    break;
  case(POSTLABELCACHE):
    yystate = 2; yystring = value;
    if((layer->postlabelcache = getSymbol(2, MS_TRUE, MS_FALSE)) == -1) return;
    if(layer->postlabelcache)
      layer->labelcache = MS_OFF;
    break;
  case(PROJECTION):
    loadProjectionString(&(layer->projection), value);
    break;
  case(QUERY):
    if(layer->numqueries != 1) { /* if only 1 query no need for index */
      if(getInteger(&i) == -1) break;
      if((i < 0) || (i >= layer->numqueries))
        break;
    }
    loadQueryString(&(layer->query[i]), value);
    break;
  case(MS_STRING):    
    for(i=0;i<layer->numclasses; i++) {
      if(!layer->class[i].name) /* skip it */
	continue;
      if(strcmp(yytext, layer->class[i].name) == 0) {	
	loadClassString(map, &(layer->class[i]), value, layer->type);
	break;
      }
    }
    break;
  case(SYMBOLSCALE):
    yystate = 2; yystring = value;
    if(getDouble(&(layer->symbolscale)) == -1) return;
    break;
  case(TOLERANCE):
    yystate = 2; yystring = value;
    if(getDouble(&(layer->tolerance)) == -1) return;
    break;
  case(TOLERANCEUNITS):
    yystate = 2; yystring = value;
    if((layer->toleranceunits = getSymbol(7, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD,MS_PIXELS)) == -1) return;
    break;
  case(TYPE):
    yystate = 2; yystring = value;
    if((layer->type = getSymbol(6, MS_POINT,MS_LINE,MS_RASTER,MS_POLYGON,MS_POLYLINE,MS_ANNOTATION)) == -1) return;
    break;
  default:
    break;
  }

  return;
}

/*
** Initialize, load and free a referenceMapObj structure
*/
void initReferenceMap(referenceMapObj *ref)
{
  ref->height = ref->width = 0;
  ref->extent.minx = ref->extent.miny = ref->extent.maxx = ref->extent.maxy = -1.0;
  ref->image = NULL;
  ref->color.red = 255;
  ref->color.green = 0;
  ref->color.blue = 0;
  ref->outlinecolor.red = 0;
  ref->outlinecolor.green = 0;
  ref->outlinecolor.blue = 0;  
  ref->status = MS_OFF;
}

void freeReferenceMap(referenceMapObj *ref)
{
  free(ref->image);
}

int loadReferenceMap(referenceMapObj *ref)
{
  for(;;) {
    switch(yylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadReferenceMap()");      
      return(-1);
    case(END):
      return(0);
      break;
    case(COLOR):      
      if(getInteger(&(ref->color.red)) == -1) return(-1);
      if(getInteger(&(ref->color.green)) == -1) return(-1);
      if(getInteger(&(ref->color.blue)) == -1) return(-1);
      break;
    case(EXTENT):
      if(getDouble(&(ref->extent.minx)) == -1) return(-1);
      if(getDouble(&(ref->extent.miny)) == -1) return(-1);
      if(getDouble(&(ref->extent.maxx)) == -1) return(-1);
      if(getDouble(&(ref->extent.maxy)) == -1) return(-1);
      break;  
    case(IMAGE):
      if((ref->image = getString()) == NULL) return(-1);
      break;
    case(OUTLINECOLOR):      
      if(getInteger(&(ref->outlinecolor.red)) == -1) return(-1);
      if(getInteger(&(ref->outlinecolor.green)) == -1) return(-1);
      if(getInteger(&(ref->outlinecolor.blue)) == -1) return(-1);
      break;
    case(SIZE):
      if(getInteger(&(ref->width)) == -1) return(-1);
      if(getInteger(&(ref->height)) == -1) return(-1);
      break;          
    case(STATUS):
      if((ref->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;   
    default:
      msSetError(MS_IDENTERR, NULL, "loadReferenceMap()");
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  } /* next token */
}

static void loadReferenceMapString(mapObj *map, referenceMapObj *ref, char *value)
{
  switch(yylex()) {
  case(COLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(ref->color.red)) == -1) return;
    if(getInteger(&(ref->color.green)) == -1) return;
    if(getInteger(&(ref->color.blue)) == -1) return;
    break;
  case(EXTENT):
    yystate = 2; yystring = value;
    if(getDouble(&(ref->extent.minx)) == -1) return;
    if(getDouble(&(ref->extent.miny)) == -1) return;
    if(getDouble(&(ref->extent.maxx)) == -1) return;
    if(getDouble(&(ref->extent.maxy)) == -1) return;
    break;  
  case(IMAGE):
    yystate = 2; yystring = value;
    if((ref->image = getString()) == NULL) return;
    break;
  case(OUTLINECOLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(ref->outlinecolor.red)) == -1) return;
    if(getInteger(&(ref->outlinecolor.green)) == -1) return;
    if(getInteger(&(ref->outlinecolor.blue)) == -1) return;
    break;
  case(SIZE):
    yystate = 2; yystring = value;
    if(getInteger(&(ref->width)) == -1) return;
    if(getInteger(&(ref->height)) == -1) return;
    break;          
  case(STATUS):
    yystate = 2; yystring = value;
    if((ref->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return;
    break;   
  default:
    break;
  }

  return;
}


/*
** Initialize, load and free a legendObj structure
*/
void initLegend(legendObj *legend)
{
  legend->height = legend->width = 0;
  legend->imagecolor.red = 255;
  legend->imagecolor.green = 255;
  legend->imagecolor.blue = 255;
  initLabel(&legend->label);
  legend->label.position = MS_XY; /* override */
  legend->keysizex = 20;
  legend->keysizey = 10;
  legend->keyspacingx = 5;
  legend->keyspacingy = 5;
  legend->outlinecolor = -1; /* i,e. off */
  legend->status = MS_OFF;
  legend->transparent = MS_OFF; /* no transparency */
  legend->interlace = MS_ON;
  legend->position = MS_LL;
  legend->postlabelcache = MS_FALSE; // draw with labels
}

void freeLegend(legendObj *legend)
{
  freeLabel(&(legend->label));
}

int loadLegend(legendObj *legend, mapObj *map)
{
  int red, green, blue;

  for(;;) {
    switch(yylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadLegend()");      
      return(-1);
    case(END):
      return(0);
      break;
    case(IMAGECOLOR):      
      if(getInteger(&(legend->imagecolor.red)) == -1) return(-1);
      if(getInteger(&(legend->imagecolor.green)) == -1) return(-1);
      if(getInteger(&(legend->imagecolor.blue)) == -1) return(-1);
      break;
    case(INTERLACE):
      if((legend->interlace = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;
    case(KEYSIZE):
      if(getInteger(&(legend->keysizex)) == -1) return(-1);
      if(getInteger(&(legend->keysizey)) == -1) return(-1);
      break;      
    case(KEYSPACING):
      if(getInteger(&(legend->keyspacingx)) == -1) return(-1);
      if(getInteger(&(legend->keyspacingy)) == -1) return(-1);
      break;  
    case(LABEL):
      if(loadLabel(&(legend->label), map) == -1) return(-1);
      legend->label.angle = 0; /* force */
      break;
    case(OUTLINECOLOR):      
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      legend->outlinecolor =msAddColor(map,red,green,blue);
      break;
    case(POSITION):
      if((legend->position = getSymbol(6, MS_UL,MS_UR,MS_LL,MS_LR,MS_UC,MS_LC)) == -1) return(-1);
      break;
    case(POSTLABELCACHE):
      if((legend->postlabelcache = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return(-1);      
      break;
    case(STATUS):
      if((legend->status = getSymbol(3, MS_ON,MS_OFF,MS_EMBED)) == -1) return(-1);      
      break;
    case(TRANSPARENT):
      if((legend->transparent = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadLegend()");
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  } /* next token */
}

static void loadLegendString(mapObj *map, legendObj *legend, char *value)
{
  int red, green, blue;

  switch(yylex()) {
  case(IMAGECOLOR):      
    yystate = 2; yystring = value;
    if(getInteger(&(legend->imagecolor.red)) == -1) return;
    if(getInteger(&(legend->imagecolor.green)) == -1) return;
    if(getInteger(&(legend->imagecolor.blue)) == -1) return;
    break;
  case(KEYSIZE):
    yystate = 2; yystring = value;
    if(getInteger(&(legend->keysizex)) == -1) return;
    if(getInteger(&(legend->keysizey)) == -1) return;
    break;      
  case(KEYSPACING):
    yystate = 2; yystring = value;
    if(getInteger(&(legend->keyspacingx)) == -1) return;
    if(getInteger(&(legend->keyspacingy)) == -1) return;
    break;  
  case(LABEL):
    loadLabelString(map, &(legend->label), value);
    legend->label.angle = 0; /* force */
    break;
  case(OUTLINECOLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    legend->outlinecolor =msAddColor(map,red,green,blue);
    break;
  case(POSITION):
    yystate = 2; yystring = value;
    if((legend->position = getSymbol(6, MS_UL,MS_UR,MS_LL,MS_LR,MS_UC,MS_LC)) == -1) return;
    break;
  case(POSTLABELCACHE):
    yystate = 2; yystring = value;
    if((legend->postlabelcache = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
    break;
  case(STATUS):
    yystate = 2; yystring = value;
    if((legend->status = getSymbol(3, MS_ON,MS_OFF,MS_EMBED)) == -1) return;      
    break;   
  default:
    break;
  }
 
  return;
}

/*
** Initialize, load and free a scalebarObj structure
*/
void initScalebar(scalebarObj *scalebar)
{
  scalebar->imagecolor.red = 255;
  scalebar->imagecolor.green = 255;
  scalebar->imagecolor.blue = 255;
  scalebar->width = 200; 
  scalebar->height = 3;
  scalebar->style = 0; // only 2 styles at this point
  scalebar->intervals = 4;
  initLabel(&scalebar->label);
  scalebar->label.position = MS_XY; /*  override */
  scalebar->backgroundcolor = 0;
  scalebar->color = 1;
  scalebar->outlinecolor = -1; /* no outline */
  scalebar->units = MS_MILES;
  scalebar->status = MS_OFF;
  scalebar->position = MS_LL;
  scalebar->transparent = MS_OFF; /* no transparency */
  scalebar->interlace = MS_ON;
  scalebar->postlabelcache = MS_FALSE; // draw with labels
}

void freeScalebar(scalebarObj *scalebar) {
  freeLabel(&(scalebar->label));
}

int loadScalebar(scalebarObj *scalebar, mapObj *map)
{
  int red, green, blue;

  for(;;) {
    switch(yylex()) {
    case(BACKGROUNDCOLOR):      
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      scalebar->backgroundcolor =msAddColor(map,red,green,blue);
      break;
    case(COLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      scalebar->color =msAddColor(map,red,green,blue);
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadScalebar()");      
      return(-1);
    case(END):
      return(0);
      break;
    case(IMAGECOLOR):      
      if(getInteger(&(scalebar->imagecolor.red)) == -1) return(-1);
      if(getInteger(&(scalebar->imagecolor.green)) == -1) return(-1);
      if(getInteger(&(scalebar->imagecolor.blue)) == -1) return(-1);
      break;
    case(INTERLACE):
      if((scalebar->interlace = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;
    case(INTERVALS):      
      if(getInteger(&(scalebar->intervals)) == -1) return(-1);
      break;
    case(LABEL):
      if(loadLabel(&(scalebar->label), map) == -1) return(-1);
      scalebar->label.angle = 0;
      if(scalebar->label.type != MS_BITMAP) {
	 msSetError(MS_TYPEERR, "Scalebars only support bitmapped fonts.", "loadScalebar()");
	 return(-1);
      }
      break;
    case(OUTLINECOLOR):      
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      scalebar->outlinecolor =msAddColor(map,red,green,blue);
      break;
    case(POSITION):
      if((scalebar->position = getSymbol(6, MS_UL,MS_UR,MS_LL,MS_LR,MS_UC,MS_LC)) == -1) 
	return(-1);
      break;
    case(POSTLABELCACHE):
      if((scalebar->postlabelcache = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return(-1);
      break;
    case(SIZE):
      if(getInteger(&(scalebar->width)) == -1) return(-1);
      if(getInteger(&(scalebar->height)) == -1) return(-1);
      break;
    case(STATUS):
      if((scalebar->status = getSymbol(3, MS_ON,MS_OFF,MS_EMBED)) == -1) return(-1);
      break;
    case(STYLE):      
      if(getInteger(&(scalebar->style)) == -1) return(-1);
      break;  
    case(TRANSPARENT):
      if((scalebar->transparent = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;
    case(UNITS):
      if((scalebar->units = getSymbol(5, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadScalebar()");
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  } /* next token */
}

static void loadScalebarString(mapObj *map, scalebarObj *scalebar, char *value)
{
  int red, green, blue;

  switch(yylex()) {
  case(BACKGROUNDCOLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    scalebar->backgroundcolor =msAddColor(map,red,green,blue);
    break;
  case(COLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    scalebar->color =msAddColor(map,red,green,blue);
    break;
  case(IMAGECOLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(scalebar->imagecolor.red)) == -1) return;
    if(getInteger(&(scalebar->imagecolor.green)) == -1) return;
    if(getInteger(&(scalebar->imagecolor.blue)) == -1) return;
    break;
  case(INTERVALS):
    yystate = 2; yystring = value;
    if(getInteger(&(scalebar->intervals)) == -1) return;
    break;
  case(LABEL):
    yystate = 2; yystring = value;
    loadLabelString(map, &(scalebar->label), value);
    scalebar->label.angle = 0;
    if(scalebar->label.type != MS_BITMAP) {
      msSetError(MS_TYPEERR, "Scalebars only support bitmapped fonts.", "loadScalebarString()");
      return;
    }
    break;
  case(OUTLINECOLOR):
    yystate = 2; yystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    scalebar->outlinecolor =msAddColor(map,red,green,blue);
    break;
  case(POSITION):
    yystate = 2; yystring = value;
    if((scalebar->position = getSymbol(4, MS_UL,MS_UR,MS_LL,MS_LR)) == -1) return;
    break;
  case(POSTLABELCACHE):
    yystate = 2; yystring = value;
    if((scalebar->postlabelcache = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
    break;
  case(SIZE):
    yystate = 2; yystring = value;
    if(getInteger(&(scalebar->width)) == -1) return;
    if(getInteger(&(scalebar->height)) == -1) return;
    break;
  case(STATUS):
    yystate = 2; yystring = value;
    if((scalebar->status = getSymbol(3, MS_ON,MS_OFF,MS_EMBED)) == -1) return;      
    break;
  case(STYLE):
    yystate = 2; yystring = value;
    if(getInteger(&(scalebar->style)) == -1) return;
    break;  
  case(UNITS):
    yystate = 2; yystring = value;
    if((scalebar->units = getSymbol(5, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS)) == -1) return;
    break;
  default:
    break;
  }

  return;
}

/*
** Initialize a queryMapObj structure
*/
void initQueryMap(queryMapObj *queryMap)
{
  queryMap->width = queryMap->height = -1;
  queryMap->style = MS_HILITE;
  queryMap->status = MS_OFF;
  queryMap->color = -1;
}

int loadQueryMap(queryMapObj *queryMap, mapObj *map)
{
  int red, green, blue;

  for(;;) {
    switch(yylex()) {
    case(COLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      queryMap->color = msAddColor(map,red,green,blue);
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadQueryMap()");
      return(-1);
    case(END):
      if(queryMap->color == -1) queryMap->color = msAddColor(map,255,255,0); /* default to yellow */
      return(0);
      break;
    case(SIZE):
      if(getInteger(&(queryMap->width)) == -1) return(-1);
      if(getInteger(&(queryMap->height)) == -1) return(-1);
      break;
    case(STATUS):
      if((queryMap->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;
    case(STYLE):
      if((queryMap->style = getSymbol(3, MS_NORMAL,MS_HILITE,MS_SELECTED)) == -1) return(-1);
      break;
    }
  }
}

/*
** Initialize a webObj structure
*/
void initWeb(webObj *web)
{
  web->extent.minx = web->extent.miny = web->extent.maxx = web->extent.maxy = -1.0;
  web->template = NULL;
  web->header = web->footer = NULL;
  web->error =  web->empty = NULL;
  web->mintemplate = web->maxtemplate = NULL;
  web->minscale = web->maxscale = -1;
  web->log = NULL;
  web->imagepath = strdup("");
  web->imageurl = strdup("");
}

void freeWeb(webObj *web)
{
  free(web->template);
  free(web->header);
  free(web->footer);
  free(web->error);
  free(web->empty);
  free(web->maxtemplate);
  free(web->mintemplate);
  free(web->log);
  free(web->imagepath);
  free(web->imageurl);
  free(web->empty);
}

int loadWeb(webObj *web)
{
  for(;;) {
    switch(yylex()) {
    case(EMPTY):
      if((web->empty = getString()) == NULL) return(-1);
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadWeb()");
      return(-1);
    case(END):
      return(0);
      break;
    case(ERROR):
      if((web->error = getString()) == NULL) return(-1);
      break;
    case(EXTENT):
      if(getDouble(&(web->extent.minx)) == -1) return(-1);
      if(getDouble(&(web->extent.miny)) == -1) return(-1);
      if(getDouble(&(web->extent.maxx)) == -1) return(-1);
      if(getDouble(&(web->extent.maxy)) == -1) return(-1);
      break;
    case(FOOTER):
      if((web->footer = getString()) == NULL) return(-1);
      break;
    case(HEADER):
      if((web->header = getString()) == NULL) return(-1);
      break;
    case(IMAGEPATH):
      free(web->imagepath);
      if((web->imagepath = getString()) == NULL) return(-1);
      break;
    case(IMAGEURL):
      free(web->imageurl);
      if((web->imageurl = getString()) == NULL) return(-1);
      break;
    case(LOG):
      if((web->log = getString()) == NULL) return(-1);
      break;
    case(MAXSCALE):
      if(getDouble(&web->maxscale) == -1) return(-1);
      break;
    case(MAXTEMPLATE):
      if((web->maxtemplate = getString()) == NULL) return(-1);
      break;
    case(MINSCALE):
      if(getDouble(&web->minscale) == -1) return(-1);
      break;
    case(MINTEMPLATE):
      if((web->mintemplate = getString()) == NULL) return(-1);
      break;    
    case(TEMPLATE):      
      if((web->template = getString()) == NULL) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadWeb()");
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(-1);
    }
  }
}

static void loadWebString(webObj *web, char *value)
{
  switch(yylex()) {
  case(EMPTY):
    free(web->empty);
    web->empty = strdup(value);
    break;
  case(ERROR):
    free(web->error);
    web->error = strdup(value);
    break;
  case(EXTENT):
    yystate = 2; yystring = value;
    if(getDouble(&(web->extent.minx)) == -1) return;
    if(getDouble(&(web->extent.miny)) == -1) return;
    if(getDouble(&(web->extent.maxx)) == -1) return;
    if(getDouble(&(web->extent.maxy)) == -1) return;
    break;
  case(FOOTER):
    free(web->footer);
    web->footer = strdup(value);	
    break;
  case(HEADER):
    free(web->header);
    web->header = strdup(value);
    break;
  case(MAXSCALE):
    yystate = 2; yystring = value;
    if(getDouble(&web->maxscale) == -1) return;
    break;
  case(MAXTEMPLATE):
    free(web->maxtemplate);
    web->maxtemplate = strdup(value);
    break;
  case(MINSCALE):
    yystate = 2; yystring = value;
    if(getDouble(&web->minscale) == -1) return;
    break;
  case(MINTEMPLATE):
    free(web->mintemplate);
    web->mintemplate = strdup(value);
    break;
  case(TEMPLATE):
    free(web->template);
    web->template = strdup(value);	
    break;
  default:
    break;
  }

  return;
}

/*
** Initialize, load and free a mapObj structure
*/
int initMap(mapObj *map)
{
  map->numlayers = 0;
  if((map->layers = (layerObj *)malloc(sizeof(layerObj)*MS_MAXLAYERS)) == NULL) {
    msSetError(MS_MEMERR, NULL, "initMap()");
    return(-1);
  }

  map->status = MS_ON;
  map->name = strdup("MS");;
  map->extent.minx = map->extent.miny = map->extent.maxx = map->extent.maxy = -1.0;

  map->scale = -1.0;
  map->scaled = MS_FALSE;

  map->height = map->width = -1;

  map->units = MS_METERS;
  map->cellsize = 0;
  map->shapepath = NULL;
  map->tile = NULL;
  map->imagecolor.red = 255;
  map->imagecolor.green = 255;
  map->imagecolor.blue = 255;

  map->palette.numcolors = 0;

  map->transparent = MS_OFF; /* no transparency */
  map->interlace = MS_ON;

  map->labelcache.labels = (labelCacheMemberObj *)malloc(sizeof(labelCacheMemberObj)*MS_LABELCACHEINITSIZE);
  if(map->labelcache.labels == NULL) {
    msSetError(MS_MEMERR, "Malloc() error.", "initMap()");
    return(-1);
  }
  map->labelcache.cachesize = MS_LABELCACHEINITSIZE;
  map->labelcache.numlabels = 0;

  map->labelcache.markers = (markerCacheMemberObj *)malloc(sizeof(markerCacheMemberObj)*MS_LABELCACHEINITSIZE);
  if(map->labelcache.markers == NULL) {
    msSetError(MS_MEMERR, "Malloc() error.", "initMap()");
    return(-1);
  }
  map->labelcache.markercachesize = MS_LABELCACHEINITSIZE;
  map->labelcache.nummarkers = 0;

  map->markerset.filename = NULL;
  map->markerset.numsymbols = 0;

  map->lineset.filename = NULL;
  map->lineset.numsymbols = 0;

  map->shadeset.filename = NULL;
  map->shadeset.numsymbols = 0;

#ifdef USE_TTF
  map->fontset.filename = NULL;
  map->fontset.numfonts = 0;  
  map->fontset.fonts = NULL;
#endif

  initLegend(&map->legend);
  initScalebar(&map->scalebar);
  initWeb(&map->web);
  initReferenceMap(&map->reference);
  initQueryMap(&map->querymap);

  if(initProjection(&map->projection) == -1)
    return(-1);

  return(0);
}

void msFreeMap(mapObj *map) {
  int i;

  if(!map) return;

  free(map->name);
  free(map->tile);
  free(map->shapepath);

  /* NEED TO FREE SYMBOL SETS! */
  free(map->markerset.filename);
  free(map->lineset.filename);
  free(map->shadeset.filename);  

  freeProjection(&(map->projection));

  for(i=0; i<map->labelcache.numlabels; i++) {
    free(map->labelcache.labels[i].string);
    msFreeShape(map->labelcache.labels[i].poly);
    free(map->labelcache.labels[i].poly);
  }
  free(map->labelcache.labels);

  for(i=0; i<map->labelcache.nummarkers; i++) {
    msFreeShape(map->labelcache.markers[i].poly);
    free(map->labelcache.markers[i].poly);
  }
  free(map->labelcache.markers);

#ifdef USE_TTF
  free(map->fontset.filename);
  msFreeHashTable(map->fontset.fonts);
#endif

  freeWeb(&(map->web));

  freeScalebar(&(map->scalebar));
  freeReferenceMap(&(map->reference));
  freeLegend(&(map->legend));  

  for(i=0; i<map->numlayers; i++)
    freeLayer(&(map->layers[i]));
  free(map->layers);

  free(map);
}

mapObj *msLoadMap(char *filename)
{
  int n=0;
  regex_t re;
  mapObj *map=NULL;
  char *map_path=NULL;
  int i,j;
  
  /*
  ** Check map filename to make sure it's legal
  */
  if(regcomp(&re, MS_MAPFILE_EXPR, REG_EXTENDED|REG_NOSUB) != 0) {
   msSetError(MS_REGEXERR, NULL, "msLoadMap()");
   sprintf(ms_error.message, "(%s)", MS_MAPFILE_EXPR);
   return(NULL);
  }
  if(regexec(&re, filename, 0, NULL, 0) != 0) { /* no match */
    regfree(&re);
    msSetError(MS_IOERR, "Illegal name.", "msLoadMap()");
    return(NULL);
  }
  regfree(&re);
  
  if((yyin = fopen(filename,"r")) == NULL) {
    msSetError(MS_IOERR, NULL, "msLoadMap()");
    sprintf(ms_error.message, "(%s)", filename);
    return(NULL);
  }

  /*
  ** Allocate mapObj structure
  */
  map = (mapObj *)malloc(sizeof(mapObj));
  if(!map) {
    msSetError(MS_MEMERR, NULL, "msLoadMap()");
    return(NULL);
  }

  map_path = getPath(filename);
  chdir(map_path); /* switch so all filenames are relative to the location of the map file */
  free(map_path);

  yylineno = 0; /* reset line counter */

  if(initMap(map) == -1) /* initialize this map */
    return(NULL);

  for(;;) {

    switch(yylex()) {   
    case(END):
      map->numlayers = n;
      fclose(yyin);

      if(msLoadSymbolFile(&(map->markerset)) == -1) return(NULL);
      if(msLoadSymbolFile(&(map->shadeset)) == -1) return(NULL); 
      if(msLoadSymbolFile(&(map->lineset)) == -1) return(NULL);

      /* step through layers and classes to resolve symbol names */
      for(i=0; i<map->numlayers; i++) {
	for(j=0; j<map->layers[i].numclasses; j++) {
	  if(map->layers[i].class[j].symbolname) {
	    switch(map->layers[i].type) {
	    case(MS_ANNOTATION):
	    case(MS_POINT):
	      if((map->layers[i].class[j].symbol = msGetSymbolIndex(&(map->markerset), map->layers[i].class[j].symbolname)) == -1) {
		msSetError(MS_EOFERR, "Undefined symbol.", "msLoadMap()");
		return(NULL);
	      }
	      break;
	    case(MS_LINE):
	    case(MS_POLYLINE):
	      if((map->layers[i].class[j].symbol = msGetSymbolIndex(&(map->lineset), map->layers[i].class[j].symbolname)) == -1) {
		msSetError(MS_EOFERR, "Undefined symbol.", "msLoadMap()");
		return(NULL);
	      }
	      break;
	    case(MS_POLYGON):
	      if((map->layers[i].class[j].symbol = msGetSymbolIndex(&(map->shadeset), map->layers[i].class[j].symbolname)) == -1) {
		msSetError(MS_EOFERR, "Undefined symbol.", "msLoadMap()");
		return(NULL);
	      }
	      break;
	    default:
	      break;
	    }
	  }
	}
      }

#ifdef USE_TTF
      if(msLoadFontSet(&(map->fontset)) == -1) return(NULL);
#endif

      return(map);
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "msLoadMap()");
      return(NULL);
    case(EXTENT):
      if(getDouble(&(map->extent.minx)) == -1) return(NULL);
      if(getDouble(&(map->extent.miny)) == -1) return(NULL);
      if(getDouble(&(map->extent.maxx)) == -1) return(NULL);
      if(getDouble(&(map->extent.maxy)) == -1) return(NULL);
      break;
    case(FONTSET):
#ifdef USE_TTF
      free(map->fontset.filename);
      if((map->fontset.filename = getString()) == NULL) return(NULL);
#endif
      break;
    case(INTERLACE):
      if((map->interlace = getSymbol(2, MS_ON,MS_OFF)) == -1) return(NULL);
      break;
    case(IMAGECOLOR):
      if(getInteger(&(map->imagecolor.red)) == -1) return(NULL);
      if(getInteger(&(map->imagecolor.green)) == -1) return(NULL);
      if(getInteger(&(map->imagecolor.blue)) == -1) return(NULL);
      break; 
    case(LAYER):
      if(loadLayer(&(map->layers[n]), map) == -1) return(NULL);
      map->layers[n].index = n; /* save the index */
      n++;
      break;
    case(LEGEND):
      if(loadLegend(&(map->legend), map) == -1) return(NULL);
      break;
    case(LINESET):
      if((map->lineset.filename = getString()) == NULL) return(NULL);
      break;
    case(MAP):
      break;
    case(MARKERSET):
      if((map->markerset.filename = getString()) == NULL) return(NULL);
      break;
    case(NAME):
      free(map->name); /* erase default */
      if((map->name = getString()) == NULL) return(NULL);
      break;
    case(PROJECTION):
      if(loadProjection(&map->projection) == -1) return(NULL);
      break;
    case(QUERYMAP):
      if(loadQueryMap(&(map->querymap), map) == -1) return(NULL);
      break;
    case(REFERENCE):
      if(loadReferenceMap(&(map->reference)) == -1) return(NULL);
      break;
    case(SCALE):
      if(getDouble(&(map->scale)) == -1) return(NULL);
      break;
    case(SCALEBAR):
      if(loadScalebar(&(map->scalebar), map) == -1) return(NULL);
      break;
    case(SHADESET):
      if((map->shadeset.filename = getString()) == NULL) return(NULL);
      break;
    case(SHAPEPATH):
      if((map->shapepath = getString()) == NULL) return(NULL);
      break;
    case(SIZE):
      if(getInteger(&(map->width)) == -1) return(NULL);
      if(getInteger(&(map->height)) == -1) return(NULL);
      break;
    case(STATUS):
      if((map->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return(NULL);
      break;
    case(TILE):
      if((map->tile = getString()) == NULL) return(NULL);
      break; 
    case(TRANSPARENT):
      if((map->transparent = getSymbol(2, MS_ON,MS_OFF)) == -1) return(NULL);
      break;
    case(UNITS):
      if((map->units = getSymbol(6, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD)) == -1) return(NULL);
      break;
    case(WEB):
      if(loadWeb(&map->web) == -1) return(NULL);
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "msLoadMap()");
      sprintf(ms_error.message, "(%s):(%d)", yytext, yylineno);
      return(NULL);
    }
  } /* next token */
}

/*
** READ FROM STRINGS INSTEAD OF FILE - ASSUMES A MAP FILE HAS ALREADY BEEN LOADED
*/
int msLoadMapString(mapObj *map, char *object, char *value)
{
  int i;

  yystate = 1; /* set lexer state */
  yystring = object;

  ms_error.code = MS_NOERR; /* init error code */

  switch(yylex()) {
  case(MAP):
    switch(yylex()) {
    case(INTERLACE):
      yystate = 2; yystring = value;
      if((map->interlace = getSymbol(2, MS_ON,MS_OFF)) == -1) break;
      break;
    case(IMAGECOLOR):
      yystate = 2; yystring = value;
      if(getInteger(&(map->imagecolor.red)) == -1) break;
      if(getInteger(&(map->imagecolor.green)) == -1) break;
      if(getInteger(&(map->imagecolor.blue)) == -1) break;
      break;
    case(LAYER):
      if(getInteger(&i) == -1) break;
      if(i>=map->numlayers || i<0) break;
      loadLayerString(map, &(map->layers[i]), value);
      break;
    case(LEGEND):
      loadLegendString(map, &(map->legend), value);
      break;
    case(PROJECTION):
      loadProjectionString(&(map->projection), value);
      break;
    case(REFERENCE):
      loadReferenceMapString(map, &(map->reference), value);
      break;
    case(SCALEBAR):
      loadScalebarString(map, &(map->scalebar), value);
      break;
    case(SIZE):
      yystate = 2; yystring = value;
      if(getInteger(&(map->width)) == -1) break;
      if(getInteger(&(map->height)) == -1) break;
      break;
    case(SHAPEPATH):
      free(map->shapepath);
      map->shapepath = strdup(value);
      break;
    case(MS_STRING):
      i = msGetLayerIndex(map, yytext);
      if(i>=map->numlayers || i<0) break;
      loadLayerString(map, &(map->layers[i]), value);
    case(TILE):
      free(map->tile);
      map->tile = strdup(value);
      break; 
    case(TRANSPARENT):
      yystate = 2; yystring = value;
      if((map->transparent = getSymbol(2, MS_ON,MS_OFF)) == -1) break;
      break;
    case(UNITS):
      yystate = 2; yystring = value;
      if((map->units = getSymbol(6, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD)) == -1) break;
      break;
    case(WEB):
      loadWebString(&(map->web), value);
      break;      
    default:
      break; /* malformed string */
    }
    break;
  default:
    break;
  }

  yystate = 3; /* restore lexer state */
  yylex();

  if(ms_error.code != MS_NOERR)
    return(-1);

  return(0);
}

#include <stdarg.h>

#include "map.h"
#include "mapfile.h"
#include "mapparser.h"

extern int msyylex();
extern double msyynumber;
extern char *msyytext;
extern int msyylineno;
extern FILE *msyyin;

extern int msyystate;
extern char *msyystring;

/*
** Symbol to string static arrays needed for writing map files.
** Must be kept in sync with enumerations and defines found in map.h.
*/
static char *msUnits[7]={"INCHES", "FEET", "MILES", "METERS", "KILOMETERS", "DD", "PIXELS"};
static char *msLayerTypes[6]={"POINT", "LINE", "POLYGON", "POLYLINE", "RASTER", "ANNOTATION"};
static char *msLabelPositions[10]={"UL", "LR", "UR", "LL", "CR", "CL", "UC", "LC", "CC", "AUTO"};
static char *msBitmapFontSizes[5]={"TINY", "SMALL", "MEDIUM", "LARGE", "GIANT"};
static char *msQueryMapStyles[4]={"NORMAL", "HILITE", "SELECTED", "INVERTED"};
static char *msStatus[5]={"OFF", "ON", "DEFAULT", "QUERYONLY", "EMBED"};
// static char *msOnOff[2]={"OFF", "ON"};
static char *msTrueFalse[2]={"FALSE", "TRUE"};
// static char *msYesNo[2]={"NO", "YES"};
static char *msJoinType[2]={"SINGLE", "MULTIPLE"};

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

  symbol = msyylex();

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
  sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno); 

  return(-1);
}

/*
** Load a string from the map file. A "string" is defined
** in lexer.l.
*/
char *getString() {
  
  if(msyylex() == MS_STRING)
    return(strdup(msyytext));

  msSetError(MS_TYPEERR, NULL, "loadString()"); 
  sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno); 

  return(NULL);
}

/*
** Load a floating point number from the map file. (see lexer.l)
*/
int getDouble(double *d) {

  if(msyylex() == MS_NUMBER) {
    *d = msyynumber;
    return(0); /* success */
  }

  msSetError(MS_TYPEERR, NULL, "getDouble()"); 
  sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno); 
  
  return(-1);
}

/*
** Load a integer from the map file. (see lexer.l)
*/
int getInteger(int *i) {

  if(msyylex() == MS_NUMBER) {
    *i = (int)msyynumber;
    return(0); /* success */
  }

  msSetError(MS_TYPEERR, NULL, "getInteger()"); 
  sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno); 

  return(-1);
}

int getCharacter(char *c) {
  if(msyylex() == MS_STRING) {
    *c = msyytext[0];
    return(0);
  }

  msSetError(MS_TYPEERR, NULL, "getCharacter()"); 
  sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno); 

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
    switch(msyylex()) {
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

      points->point[points->numpoints].x = atof(msyytext);
      if(getDouble(&(points->point[points->numpoints].y)) == -1) return(-1);

      points->numpoints++;
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadFeaturePoints()");    
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);      
    }
  }
}

static int loadFeature(struct featureObj *feature)
{
  multipointObj points={0,NULL};

  for(;;) {
    switch(msyylex()) {
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  } /* next token */  
}

static void writeFeature(struct featureObj *feature, FILE *stream) 
{
  int i,j;

  fprintf(stream, "    FEATURE\n");
  if(feature->class) fprintf(stream, "      CLASS \"%s\"\n", feature->class);

  for(i=0; i<feature->shape.numlines; i++) {
    fprintf(stream, "      POINTS\n");
    for(j=0; j<feature->shape.line[i].numpoints; j++)
      fprintf(stream, "        %g %g\n", feature->shape.line[i].point[j].x, feature->shape.line[i].point[j].y);
    fprintf(stream, "      END\n");
  }

  if(feature->text) fprintf(stream, "      TEXT \"%s\"\n", feature->text);
  fprintf(stream, "    END\n");
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
    switch(msyylex()) {
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
      p->projargs[i] = strdup(msyytext);
      i++;
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadProjection()");
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
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

static void writeProjection(projectionObj *p, FILE *stream, char *tab) {
#ifdef USE_PROJ  
  int i;

  if(p->numargs > 0);
  fprintf(stream, "%sPROJECTION\n", tab);
  for(i=0; i<p->numargs; i++)
    fprintf(stream, "  %s%s\n", tab, p->projargs[i]);
  fprintf(stream, "%sEND\n", tab);
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
  label->sizescaled = label->size = MS_MEDIUM;

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
    switch(msyylex()) {
    case(ANGLE):
      if((symbol = getSymbol(2, MS_NUMBER,MS_AUTO)) == -1) 
	return(-1);

      if(symbol == MS_NUMBER)
	label->angle = MS_DEG_TO_RAD*msyynumber;
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
	label->minfeaturesize = msyynumber;
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
	label->size = msyynumber;
#else
      if((label->size = getSymbol(5, MS_TINY,MS_SMALL,MS_MEDIUM,MS_LARGE,MS_GIANT)) == -1) 
	return(-1);
#endif
      label->sizescaled = label->size;
      break; 
    case(TYPE):
      if((label->type = getSymbol(2, MS_TRUETYPE,MS_BITMAP)) == -1) return(-1);
      break;    
    case(WRAP):
      if(getCharacter(&(label->wrap)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, NULL, "loadlabel()");    
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  } /* next token */
}

static void loadLabelString(mapObj *map, labelObj *label, char *value)
{
  int red, green, blue;
  int symbol;

  switch(msyylex()) {
  case(ANGLE):
    msyystate = 2; msyystring = value;
    label->autoangle = MS_FALSE;
    if((symbol = getSymbol(2, MS_NUMBER,MS_AUTO)) == -1) 
      return;
    
    if(symbol == MS_NUMBER)
      label->angle = MS_DEG_TO_RAD*msyynumber;
    else
      label->autoangle = MS_TRUE;
    break;
  case(ANTIALIAS):
    label->antialias = 1;
    break;  
  case(BUFFER):
    msyystate = 2; msyystring = value;
    if(getInteger(&(label->buffer)) == -1) break;
    break;
  case(COLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    label->color = msAddColor(map,red,green,blue);     
    break;    
  case(FONT):
    free(label->font);
    label->font = strdup(value);

#ifdef USE_TTF
    free(label->font);
    label->font = strdup(value);
    
    if(!(msLookupHashTable(map->fontset.fonts, label->font))) {
      msSetError(MS_IDENTERR, "Unknown font alias.", "loadLabelString()");    
      sprintf(ms_error.message, "(%s)", msyytext);
      return;
    }
#else
    msSetError(MS_IDENTERR, "Keyword FONT is not valid without TrueType font support.", "loadLabelString()");    
    return;
#endif

    break;
  case(FORCE):
      msyystate = 2; msyystring = value;
      if((label->force = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
      break; 
  case(MAXSIZE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(label->maxsize)) == -1) return;
    break;
  case(MINDISTANCE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(label->mindistance)) == -1) return;
    break;
  case(MINFEATURESIZE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(label->minfeaturesize)) == -1) return;
    break;
  case(MINSIZE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(label->minsize)) == -1) return;
    break;
  case(OFFSET):
    msyystate = 2; msyystring = value;
    if(getInteger(&(label->offsetx)) == -1) return;
    if(getInteger(&(label->offsety)) == -1) return;
    break;  
  case(OUTLINECOLOR):
  case(BACKGROUNDCOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    label->outlinecolor = msAddColor(map,red,green,blue);
    break;
  case(PARTIALS):
    msyystate = 2; msyystring = value;
    if((label->partials = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
    break;
  case(POSITION):
    msyystate = 2; msyystring = value;
    if((label->position = getSymbol(10, MS_UL,MS_UC,MS_UR,MS_CL,MS_CC,MS_CR,MS_LL,MS_LC,MS_LR,MS_AUTO)) == -1) return;
    break;
  case(SHADOWCOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    label->shadowcolor =msAddColor(map,red,green,blue);
    break;
  case(SHADOWSIZE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(label->shadowsizex)) == -1) return;
    if(getInteger(&(label->shadowsizey)) == -1) return;
    break;
  case(SIZE):
    msyystate = 2; msyystring = value;
#ifdef USE_TTF
    if((label->size = getSymbol(6, MS_NUMBER,MS_TINY,MS_SMALL,MS_MEDIUM,MS_LARGE,MS_GIANT)) == -1) return;
    if(label->size == MS_NUMBER)
      label->size = msyynumber;
#else
    if((label->size = getSymbol(5, MS_TINY,MS_SMALL,MS_MEDIUM,MS_LARGE,MS_GIANT)) == -1) return;
#endif    
    label->sizescaled = label->size;
    break;
  case(TYPE):
    msyystate = 2; msyystring = value;
    if((label->type = getSymbol(2, MS_TRUETYPE,MS_BITMAP)) == -1) return;
    break;    
  case(WRAP):
    if(value) label->wrap = value[0];
  default:
    break;
  }
}

static void writeLabel(mapObj *map, labelObj *label, FILE *stream, char *tab)
{
  fprintf(stream, "%sLABEL\n", tab);
  if(label->type == MS_BITMAP) {
    fprintf(stream, "  %sSIZE %s\n", tab, msBitmapFontSizes[label->size]);
    fprintf(stream, "  %sTYPE BITMAP\n", tab);
  } else {
    if(label->autoangle)
      fprintf(stream, "  %sANGLE AUTO\n", tab);
    else
      fprintf(stream, "  %sANGLE %f\n", tab, label->angle*MS_RAD_TO_DEG);
    if(label->antialias) fprintf(stream, "  %sANTIALIAS\n", tab);
    fprintf(stream, "  %sFONT %s\n", tab, label->font);
    fprintf(stream, "  %sMAXSIZE %d\n", tab, label->maxsize);
    fprintf(stream, "  %sMINSIZE %d\n", tab, label->minsize);
    fprintf(stream, "  %sSIZE %d\n", tab, label->size);
    fprintf(stream, "  %sTYPE TRUETYPE\n", tab);
  }  

  fprintf(stream, "  %sBUFFER %d\n", tab, label->buffer);
  if(label->color > -1) fprintf(stream, "  %sCOLOR %d %d %d\n", tab, map->palette.colors[label->color].red, map->palette.colors[label->color].green, map->palette.colors[label->color].blue);
  fprintf(stream, "  %sFORCE %s\n", tab, msTrueFalse[label->force]);
  fprintf(stream, "  %sMINDISTANCE %d\n", tab, label->mindistance);
  if(label->autominfeaturesize)
    fprintf(stream, "  %sMINFEATURESIZE AUTO\n", tab);
  else
    fprintf(stream, "  %sMINFEATURESIZE %d\n", tab, label->minfeaturesize);
  fprintf(stream, "  %sOFFSET %d %d\n", tab, label->offsetx, label->offsety);
  if(label->outlinecolor > -1) fprintf(stream, "  %sOUTLINECOLOR %d %d %d\n", tab, map->palette.colors[label->outlinecolor].red, map->palette.colors[label->outlinecolor].green, map->palette.colors[label->outlinecolor].blue);
  fprintf(stream, "  %sPARTIALS %s\n", tab, msTrueFalse[label->partials]);
  fprintf(stream, "  %sPOSITION %s\n", tab, msLabelPositions[label->position]);
  if(label->shadowcolor > -1) {
    fprintf(stream, "  %sSHADOWCOLOR %d %d %d\n", tab, map->palette.colors[label->shadowcolor].red, map->palette.colors[label->shadowcolor].green, map->palette.colors[label->shadowcolor].blue);
    fprintf(stream, "  %sSHADOWSIZE %d %d\n", tab, label->shadowsizex, label->shadowsizey);
  }
  fprintf(stream, "  %sWRAP %c\n", tab, label->wrap);
  fprintf(stream, "%sEND\n", tab);  
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
  exp->string = strdup(msyytext);
  
  if(exp->type == MS_REGEX) {
    if(regcomp(&(exp->regex), exp->string, REG_EXTENDED|REG_NOSUB) != 0) { // compile the expression 
      msSetError(MS_REGEXERR, NULL, "loadExpression()");
      sprintf(ms_error.message, "(%s):(%d)", exp->string, msyylineno);
      return(-1);
    }
  }

  return(0);
}

int loadExpressionString(expressionObj *exp, char *value)
{
  msyystate = 2; msyystring = value;

  freeExpression(exp); // we're totally replacing the old expression so free then init to start over
  initExpression(exp);

  if((exp->type = getSymbol(3, MS_STRING,MS_EXPRESSION,MS_REGEX)) == -1) return(-1);
  exp->string = strdup(msyytext);
  
  if(exp->type == MS_REGEX) {
    if(regcomp(&(exp->regex), exp->string, REG_EXTENDED|REG_NOSUB) != 0) { // compile the expression 
      msSetError(MS_REGEXERR, NULL, "loadExpression()");
      sprintf(ms_error.message, "(%s):(%d)", exp->string, msyylineno);
      return(-1);
    }
  }

  return(0); 
}

static void writeExpression(expressionObj *exp, FILE *stream)
{
  switch(exp->type) {
  case(MS_REGEX):
    fprintf(stream, "/%s/", exp->string);
    break;
  case(MS_STRING):
    fprintf(stream, "\"%s\"", exp->string);
    break;
  case(MS_EXPRESSION):
    fprintf(stream, "(%s)", exp->string);
    break;
  }
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
  class->sizescaled = class->size = 1; /* one pixel */
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
    switch(msyylex()) {
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
      class->sizescaled = class->size;
      break;
    case(SYMBOL):
      if((state = getSymbol(2, MS_NUMBER,MS_STRING)) == -1) return(-1);

      if(state == MS_NUMBER)
	class->symbol = (int) msyynumber;
      else
	class->symbolname = strdup(msyytext);
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  }
}

static void loadClassString(mapObj *map, classObj *class, char *value, int type)
{
  int red, green, blue;
  int state;

  switch(msyylex()) {
  case(BACKGROUNDCOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    class->backgroundcolor = msAddColor(map, red, green, blue);
    break;
  case(COLOR):
    msyystate = 2; msyystring = value;
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
    msyystate = 2; msyystring = value;
    getInteger(&(class->maxsize));
    break;
  case(MINSIZE):
    msyystate = 2; msyystring = value;
    getInteger(&(class->minsize));
    break;
  case(OUTLINECOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    class->outlinecolor = msAddColor(map, red, green, blue);
    break;
  case(SIZE):
    msyystate = 2; msyystring = value;
    getInteger(&(class->size));
    class->sizescaled = class->size;
    break;
  case(SYMBOL):
    msyystate = 2; msyystring = value;
    if((state = getSymbol(2, MS_NUMBER,MS_STRING)) == -1) return;

    if(state == MS_NUMBER)
      class->symbol = (int) msyynumber;
    else {
      switch(type) {
      case(MS_ANNOTATION):
      case(MS_POINT):
	if((class->symbol = msGetSymbolIndex(&(map->markerset), msyytext)) == -1) msSetError(MS_EOFERR, "Undefined symbol.", "loadClassString()");
	break;
      case(MS_LINE):
      case(MS_POLYLINE):
	if((class->symbol = msGetSymbolIndex(&(map->lineset), msyytext)) == -1) msSetError(MS_EOFERR, "Undefined symbol.", "loadClassString()");
	break;
      case(MS_POLYGON):
	if((class->symbol = msGetSymbolIndex(&(map->shadeset), msyytext)) == -1) msSetError(MS_EOFERR, "Undefined symbol.", "loadClassString()");
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

static void writeClass(mapObj *map, classObj *class, FILE *stream)
{
  fprintf(stream, "    CLASS\n");
  if(class->backgroundcolor > -1) fprintf(stream, "      BACKGROUNDCOLOR %d %d %d\n", map->palette.colors[class->backgroundcolor].red, map->palette.colors[class->backgroundcolor].green, map->palette.colors[class->backgroundcolor].blue);
  if(class->color > -1) fprintf(stream, "      COLOR %d %d %d\n", map->palette.colors[class->color].red, map->palette.colors[class->color].green, map->palette.colors[class->color].blue);
  if(class->expression.string) {
    fprintf(stream, "      EXPRESSION ");
    writeExpression(&(class->expression), stream);
    fprintf(stream, "\n");
  }
  writeLabel(map, &(class->label), stream, "      ");
  if(class->maxsize > -1) fprintf(stream, "      MAXSIZE %d\n", class->maxsize);
  if(class->minsize > -1) fprintf(stream, "      MINSIZE %d\n", class->minsize);
  if(class->outlinecolor > -1) fprintf(stream, "      OUTLINECOLOR %d %d %d\n", map->palette.colors[class->outlinecolor].red, map->palette.colors[class->outlinecolor].green, map->palette.colors[class->outlinecolor].blue);
  fprintf(stream, "      SIZE %d\n", class->size);
  if(class->symbolname)
    fprintf(stream, "      SYMBOL \"%s\"\n", class->symbolname);
  else
    fprintf(stream, "      SYMBOL %d\n", class->symbol);
  if(class->text.string) {
    fprintf(stream, "      TEXT ");
    writeExpression(&(class->text), stream);
    fprintf(stream, "\n");
  }
  fprintf(stream, "    END\n");
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
    switch(msyylex()) {
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  } /* next token */
}

static void writeJoin(joinObj *join, FILE *stream) 
{
  fprintf(stream, "      JOIN\n");
  if(join->footer) fprintf(stream, "        FOOTER \"%s\"\n", join->footer);
  if(join->from) fprintf(stream, "        FROM \"%s\"\n", join->from);
  if(join->header) fprintf(stream, "        HEADER \"%s\"\n", join->header);
  if(join->name) fprintf(stream, "        NAME \"%s\"\n", join->name);
  if(join->table) fprintf(stream, "        TABLE \"%s\"\n", join->table);
  if(join->to) fprintf(stream, "        TO \"%s\"\n", join->to);
  fprintf(stream, "        TYPE %s\n", msJoinType[join->type]);
  fprintf(stream, "      END\n");
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
    switch(msyylex()) {
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  } /* next token */
}

void loadQueryString(queryObj *query, char *value)
{
  switch(msyylex()) {
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

static void writeQuery(queryObj *query, FILE *stream) 
{
  int i;

  fprintf(stream, "    QUERY\n");
  if(query->expression.string) {
    fprintf(stream, "      EXPRESSION ");
    writeExpression(&(query->expression), stream);
    fprintf(stream, "\n");
  }
  for(i=0; i<query->numjoins; i++)
    writeJoin(&(query->joins[i]), stream);
  if(query->template) fprintf(stream, "      TEMPATE \"%s\"\n", query->template);
  fprintf(stream, "    END\n");
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
    switch(msyylex()) {
    case(CLASS):
      if(loadClass(&(layer->class[c]), map) == -1) return(-1);
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  } /* next token */
}

static void loadLayerString(mapObj *map, layerObj *layer, char *value)
{
  int i=0,buffer_size=0;
  multipointObj points={0,NULL};
  int done=0;

  switch(msyylex()) {
  case(CLASS):
    if(layer->numclasses != 1) { /* if only 1 class no need for index */
      if(getInteger(&i) == -1) break;
      if((i < 0) || (i >= layer->numclasses))
        break;
    }
    loadClassString(map, &(layer->class[i]), value, layer->type);
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
    
    switch(msyylex()) {      
    case(POINTS):
      msyystate = 2; msyystring = value;

      if(layer->features == NULL)
	if((layer->features = initFeature()) == NULL) return; /* create initial feature */

      if((points.point = (pointObj *)malloc(sizeof(pointObj)*MS_FEATUREINITSIZE)) == NULL) {
	msSetError(MS_MEMERR, NULL, "loadLayerString()");
	return;
      }
      points.numpoints = 0;
      buffer_size = MS_FEATUREINITSIZE;
            
      while(!done) {
	switch(msyylex()) {	  
	case(MS_NUMBER):
	  if(points.numpoints == buffer_size) { /* just add it to the end */
	    points.point = (pointObj *) realloc(points.point, sizeof(pointObj)*(buffer_size+MS_FEATUREINCREMENT));    
	    if(!points.point) {
	      msSetError(MS_MEMERR, "Realloc() error.", "loadlayerString()");
	      return;
	    }   
	    buffer_size+=MS_FEATUREINCREMENT;
	  }
	  
	  points.point[points.numpoints].x = atof(msyytext);
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
    msyystate = 2; msyystring = value;
    if((layer->labelcache = getSymbol(2, MS_ON, MS_OFF)) == -1) return;
    break;
  case(LABELITEM):
    free(layer->labelitem);
    layer->labelitem = strdup(value);
    break;
  case(LABELMAXSCALE):
    msyystate = 2; msyystring = value;
    if(getDouble(&(layer->labelmaxscale)) == -1) return;
    break;
  case(LABELMINSCALE):
    msyystate = 2; msyystring = value;
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
    msyystate = 2; msyystring = value;
    if(getInteger(&(layer->maxfeatures)) == -1) return;
    break;
  case(MAXSCALE):
    msyystate = 2; msyystring = value;
    if(getDouble(&(layer->maxscale)) == -1) return;
    break;
  case(MINSCALE):
    msyystate = 2; msyystring = value;
    if(getDouble(&(layer->minscale)) == -1) return;
    break;    
  case(OFFSITE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(layer->offsite)) == -1) return; /* using an index rather than an actual color */        
    break;
  case(POSTLABELCACHE):
    msyystate = 2; msyystring = value;
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
      if(strcmp(msyytext, layer->class[i].name) == 0) {	
	loadClassString(map, &(layer->class[i]), value, layer->type);
	break;
      }
    }
    break;
  case(SYMBOLSCALE):
    msyystate = 2; msyystring = value;
    if(getDouble(&(layer->symbolscale)) == -1) return;
    break;
  case(TOLERANCE):
    msyystate = 2; msyystring = value;
    if(getDouble(&(layer->tolerance)) == -1) return;
    break;
  case(TOLERANCEUNITS):
    msyystate = 2; msyystring = value;
    if((layer->toleranceunits = getSymbol(7, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD,MS_PIXELS)) == -1) return;
    break;
  case(TYPE):
    msyystate = 2; msyystring = value;
    if((layer->type = getSymbol(6, MS_POINT,MS_LINE,MS_RASTER,MS_POLYGON,MS_POLYLINE,MS_ANNOTATION)) == -1) return;
    break;
  default:
    break;
  }

  return;
}

static void writeLayer(mapObj *map, layerObj *layer, FILE *stream)
{
  int i;
  struct featureObj *fptr=NULL;

  fprintf(stream, "  LAYER\n");
  for(i=0; i<layer->numclasses; i++) writeClass(map, &(layer->class[i]), stream);
  if(layer->classitem) fprintf(stream, "    CLASSITEM \"%s\"\n", layer->classitem);
  if(layer->data) fprintf(stream, "    DATA \"%s\"\n", layer->data);
  if(layer->description) fprintf(stream, "    DESCRIPTION \"%s\"\n", layer->description);
  for(fptr=layer->features; fptr; fptr=fptr->next) writeFeature(fptr, stream);
  if(layer->footer) fprintf(stream, "    FOOTER \"%s\"\n", layer->footer);
  if(layer->group) fprintf(stream, "    GROUP \"%s\"\n", layer->group);
  if(layer->header) fprintf(stream, "    HEADER \"%s\"\n", layer->header);
  if(layer->labelangleitem) fprintf(stream, "    LABELANGLEITEM \"%s\"\n", layer->labelangleitem);
  if(!layer->labelcache) fprintf(stream, "    LABELCACHE OFF\n");
  if(layer->labelitem) fprintf(stream, "    LABELITEM \"%s\"\n", layer->labelitem);
  if(layer->labelmaxscale > -1) fprintf(stream, "    LABELMAXSCALE %g\n", layer->labelmaxscale);
  if(layer->labelminscale > -1) fprintf(stream, "    LABELMINSCALE %g\n", layer->labelminscale);
  if(layer->labelsizeitem) fprintf(stream, "    LABELSIZEITEM \"%s\"\n", layer->labelsizeitem);
  if(layer->legend) fprintf(stream, "    LEGEND \"%s\"\n", layer->legend);
  if(layer->maxfeatures > 0) fprintf(stream, "    MAXFEATURES %d\n", layer->maxfeatures);
  if(layer->maxscale > -1) fprintf(stream, "    MAXSCALE %g\n", layer->maxscale); 
  if(layer->minscale > -1) fprintf(stream, "    MINSCALE %g\n", layer->minscale);
  fprintf(stream, "    NAME \"%s\"\n", layer->name);
  if(layer->offsite > -1) fprintf(stream, "    OFFSITE %d\n", layer->offsite);
  if(layer->postlabelcache) fprintf(stream, "    POSTLABELCACHE TRUE\n");
  writeProjection(&(layer->projection), stream, "    ");
  for(i=0; i<layer->numqueries; i++) writeQuery(&(layer->query[i]), stream);
  if(layer->queryitem) fprintf(stream, "    QUERYITEM \"%s\"\n", layer->queryitem);
  fprintf(stream, "    STATUS %s\n", msStatus[layer->status]);
  if(layer->symbolscale > -1) fprintf(stream, "    SYMBOLSCALE %g\n", layer->symbolscale);
  if(layer->tileindex) fprintf(stream, "    TILEINDEX \"%s\"\n", layer->tileindex);
  if(layer->tileitem) fprintf(stream, "    TILEITEM \"%s\"\n", layer->tileitem);
  fprintf(stream, "    TOLERANCE %g\n", layer->tolerance);
  fprintf(stream, "    TOLERANCEUNITS %s\n", msUnits[layer->toleranceunits]);
  if(!layer->transform) fprintf(stream, "    TRANSFORM FALSE\n");
  fprintf(stream, "    TYPES %s\n", msLayerTypes[layer->type]);
  fprintf(stream, "  END\n");
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
    switch(msyylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadReferenceMap()");
      return(-1);
    case(END):
      if(!ref->image) {
	msSetError(MS_MISCERR, "No image defined for the reference map.", "loadReferenceMap()");
	return(-1);
      }
      if(ref->width == 0 || ref->height == 0) {
	msSetError(MS_MISCERR, "No image size defined for the reference map.", "loadReferenceMap()");
	return(-1);
      }
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  } /* next token */
}

static void loadReferenceMapString(mapObj *map, referenceMapObj *ref, char *value)
{
  switch(msyylex()) {
  case(COLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(ref->color.red)) == -1) return;
    if(getInteger(&(ref->color.green)) == -1) return;
    if(getInteger(&(ref->color.blue)) == -1) return;
    break;
  case(EXTENT):
    msyystate = 2; msyystring = value;
    if(getDouble(&(ref->extent.minx)) == -1) return;
    if(getDouble(&(ref->extent.miny)) == -1) return;
    if(getDouble(&(ref->extent.maxx)) == -1) return;
    if(getDouble(&(ref->extent.maxy)) == -1) return;
    break;  
  case(IMAGE):
    msyystate = 2; msyystring = value;
    if((ref->image = getString()) == NULL) return;
    break;
  case(OUTLINECOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(ref->outlinecolor.red)) == -1) return;
    if(getInteger(&(ref->outlinecolor.green)) == -1) return;
    if(getInteger(&(ref->outlinecolor.blue)) == -1) return;
    break;
  case(SIZE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(ref->width)) == -1) return;
    if(getInteger(&(ref->height)) == -1) return;
    break;          
  case(STATUS):
    msyystate = 2; msyystring = value;
    if((ref->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return;
    break;   
  default:
    break;
  }

  return;
}

static void writeReferenceMap(referenceMapObj *ref, FILE *stream)
{
  if(!ref->image) return;

  fprintf(stream, "  REFERENCE\n");
  fprintf(stream, "    COLOR %d %d %d\n", ref->color.red, ref->color.green, ref->color.blue);
  fprintf(stream, "    EXTENT %g %g %g %g\n", ref->extent.minx, ref->extent.miny, ref->extent.maxx, ref->extent.maxy);
  fprintf(stream, "    IMAGE %s\n", ref->image);
  fprintf(stream, "    OUTLINECOLOR %d %d %d\n", ref->outlinecolor.red, ref->outlinecolor.green, ref->outlinecolor.blue);
  fprintf(stream, "    SIZE %d %d\n", ref->width, ref->height);
  fprintf(stream, "    STATUS %s\n", msStatus[ref->status]);
  fprintf(stream, "  END\n");
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
    switch(msyylex()) {
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
      legend->outlinecolor = msAddColor(map,red,green,blue);
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  } /* next token */
}

static void loadLegendString(mapObj *map, legendObj *legend, char *value)
{
  int red, green, blue;

  switch(msyylex()) {
  case(IMAGECOLOR):      
    msyystate = 2; msyystring = value;
    if(getInteger(&(legend->imagecolor.red)) == -1) return;
    if(getInteger(&(legend->imagecolor.green)) == -1) return;
    if(getInteger(&(legend->imagecolor.blue)) == -1) return;
    break;
  case(KEYSIZE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(legend->keysizex)) == -1) return;
    if(getInteger(&(legend->keysizey)) == -1) return;
    break;      
  case(KEYSPACING):
    msyystate = 2; msyystring = value;
    if(getInteger(&(legend->keyspacingx)) == -1) return;
    if(getInteger(&(legend->keyspacingy)) == -1) return;
    break;  
  case(LABEL):
    loadLabelString(map, &(legend->label), value);
    legend->label.angle = 0; /* force */
    break;
  case(OUTLINECOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    legend->outlinecolor =msAddColor(map,red,green,blue);
    break;
  case(POSITION):
    msyystate = 2; msyystring = value;
    if((legend->position = getSymbol(6, MS_UL,MS_UR,MS_LL,MS_LR,MS_UC,MS_LC)) == -1) return;
    break;
  case(POSTLABELCACHE):
    msyystate = 2; msyystring = value;
    if((legend->postlabelcache = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
    break;
  case(STATUS):
    msyystate = 2; msyystring = value;
    if((legend->status = getSymbol(3, MS_ON,MS_OFF,MS_EMBED)) == -1) return;      
    break;   
  default:
    break;
  }
 
  return;
}

static void writeLegend(mapObj *map, legendObj *legend, FILE *stream)
{
  fprintf(stream, "  LEGEND\n");
  fprintf(stream, "    IMAGECOLOR %d %d %d\n", legend->imagecolor.red, legend->imagecolor.green, legend->imagecolor.blue);
  fprintf(stream, "    INTERLACE %s\n", msTrueFalse[legend->interlace]);
  fprintf(stream, "    KEYSIZE %d %d\n", legend->keysizex, legend->keysizey);
  fprintf(stream, "    KEYSPACING %d %d\n", legend->keyspacingx, legend->keyspacingy);
  writeLabel(map, &(legend->label), stream, "    ");
  if(legend->outlinecolor > -1) fprintf(stream, "    OUTLINECOLOR %d %d %d\n", map->palette.colors[legend->outlinecolor].red, map->palette.colors[legend->outlinecolor].green, map->palette.colors[legend->outlinecolor].blue);
  fprintf(stream, "    POSITION %s\n", msLabelPositions[legend->position]);
  if(legend->postlabelcache) fprintf(stream, "    POSTLABELCACHE TRUE\n");
  fprintf(stream, "    STATUS %s\n", msStatus[legend->status]);
  fprintf(stream, "    TRANSPARENT %s\n", msTrueFalse[legend->transparent]);
  fprintf(stream, "  END\n");
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
    switch(msyylex()) {
    case(BACKGROUNDCOLOR):      
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      scalebar->backgroundcolor = msAddColor(map,red,green,blue);
      break;
    case(COLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      scalebar->color = msAddColor(map,red,green,blue);
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  } /* next token */
}

static void loadScalebarString(mapObj *map, scalebarObj *scalebar, char *value)
{
  int red, green, blue;

  switch(msyylex()) {
  case(BACKGROUNDCOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    scalebar->backgroundcolor =msAddColor(map,red,green,blue);
    break;
  case(COLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    scalebar->color =msAddColor(map,red,green,blue);
    break;
  case(IMAGECOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(scalebar->imagecolor.red)) == -1) return;
    if(getInteger(&(scalebar->imagecolor.green)) == -1) return;
    if(getInteger(&(scalebar->imagecolor.blue)) == -1) return;
    break;
  case(INTERVALS):
    msyystate = 2; msyystring = value;
    if(getInteger(&(scalebar->intervals)) == -1) return;
    break;
  case(LABEL):
    msyystate = 2; msyystring = value;
    loadLabelString(map, &(scalebar->label), value);
    scalebar->label.angle = 0;
    if(scalebar->label.type != MS_BITMAP) {
      msSetError(MS_TYPEERR, "Scalebars only support bitmapped fonts.", "loadScalebarString()");
      return;
    }
    break;
  case(OUTLINECOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    scalebar->outlinecolor =msAddColor(map,red,green,blue);
    break;
  case(POSITION):
    msyystate = 2; msyystring = value;
    if((scalebar->position = getSymbol(4, MS_UL,MS_UR,MS_LL,MS_LR)) == -1) return;
    break;
  case(POSTLABELCACHE):
    msyystate = 2; msyystring = value;
    if((scalebar->postlabelcache = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
    break;
  case(SIZE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(scalebar->width)) == -1) return;
    if(getInteger(&(scalebar->height)) == -1) return;
    break;
  case(STATUS):
    msyystate = 2; msyystring = value;
    if((scalebar->status = getSymbol(3, MS_ON,MS_OFF,MS_EMBED)) == -1) return;      
    break;
  case(STYLE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(scalebar->style)) == -1) return;
    break;  
  case(UNITS):
    msyystate = 2; msyystring = value;
    if((scalebar->units = getSymbol(5, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS)) == -1) return;
    break;
  default:
    break;
  }

  return;
}

static void writeScalebar(mapObj *map, scalebarObj *scalebar, FILE *stream)
{
  fprintf(stream, "  LEGEND\n");
  if(scalebar->backgroundcolor > -1) fprintf(stream, "    BACKGROUNDCOLOR %d %d %d\n", map->palette.colors[scalebar->backgroundcolor].red, map->palette.colors[scalebar->backgroundcolor].green, map->palette.colors[scalebar->backgroundcolor].blue);
  if(scalebar->color > -1) fprintf(stream, "    COLOR %d %d %d\n", map->palette.colors[scalebar->color].red, map->palette.colors[scalebar->color].green, map->palette.colors[scalebar->color].blue);
  fprintf(stream, "    IMAGECOLOR %d %d %d\n", scalebar->imagecolor.red, scalebar->imagecolor.green, scalebar->imagecolor.blue);
  fprintf(stream, "    INTERLACE %s\n", msTrueFalse[scalebar->interlace]);
  fprintf(stream, "    INTERVALS %d\n", scalebar->intervals);
  writeLabel(map, &(scalebar->label), stream, "    ");
  if(scalebar->outlinecolor > -1) fprintf(stream, "    OUTLINECOLOR %d %d %d\n", map->palette.colors[scalebar->outlinecolor].red, map->palette.colors[scalebar->outlinecolor].green, map->palette.colors[scalebar->outlinecolor].blue);
  fprintf(stream, "    POSITION %s\n", msLabelPositions[scalebar->position]);
  if(scalebar->postlabelcache) fprintf(stream, "    POSTLABELCACHE TRUE\n");
  fprintf(stream, "    SIZE %d %d\n", scalebar->width, scalebar->height);
  fprintf(stream, "    STATUS %s\n", msStatus[scalebar->status]);
  fprintf(stream, "    STYLE %d\n", scalebar->style);
  fprintf(stream, "    TRANSPARENT %s\n", msTrueFalse[scalebar->transparent]);
  fprintf(stream, "    UNITS %s\n", msUnits[scalebar->units]);
  fprintf(stream, "  END\n");
}

/*
** Initialize a queryMapObj structure
*/
void initQueryMap(queryMapObj *querymap)
{
  querymap->width = querymap->height = -1;
  querymap->style = MS_HILITE;
  querymap->status = MS_OFF;
  querymap->color = -1;
}

int loadQueryMap(queryMapObj *querymap, mapObj *map)
{
  int red, green, blue;

  for(;;) {
    switch(msyylex()) {
    case(COLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      querymap->color = msAddColor(map,red,green,blue);
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadQueryMap()");
      return(-1);
    case(END):
      if(querymap->color == -1) querymap->color = msAddColor(map,255,255,0); /* default to yellow */
      return(0);
      break;
    case(SIZE):
      if(getInteger(&(querymap->width)) == -1) return(-1);
      if(getInteger(&(querymap->height)) == -1) return(-1);
      break;
    case(STATUS):
      if((querymap->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;
    case(STYLE):
      if((querymap->style = getSymbol(3, MS_NORMAL,MS_HILITE,MS_SELECTED)) == -1) return(-1);
      break;
    }
  }
}

static void writeQueryMap(mapObj *map, queryMapObj *querymap, FILE *stream)
{
  fprintf(stream, "  QUERYMAP\n");
  if(querymap->color > -1) fprintf(stream, "    COLOR %d %d %d\n", map->palette.colors[querymap->color].red, map->palette.colors[querymap->color].green, map->palette.colors[querymap->color].blue);
  fprintf(stream, "    SIZE %d %d\n", querymap->width, querymap->height);
  fprintf(stream, "    STATUS %s\n", msStatus[querymap->status]);
  fprintf(stream, "    STYLE %s\n", msQueryMapStyles[querymap->style]);  
  fprintf(stream, "  END\n");
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

static void writeWeb(webObj *web, FILE *stream)
{
  fprintf(stream, "  WEB\n");
  if(web->empty) fprintf(stream, "    EMPTY \"%s\"\n", web->empty);
  if(web->error) fprintf(stream, "    ERROR \"%s\"\n", web->error);
  if(web->footer) fprintf(stream, "    FOOTER \"%s\"\n", web->footer);
  if(web->header) fprintf(stream, "    HEADER \"%s\"\n", web->header);
  if(web->imagepath) fprintf(stream, "    IMAGEPATH \"%s\"\n", web->imagepath);
  if(web->imageurl) fprintf(stream, "    IMAGEURL \"%s\"\n", web->imageurl);
  if(web->log) fprintf(stream, "    LOG \"%s\"\n", web->log);
  if(web->maxscale > -1) fprintf(stream, "    MAXSCALE %g", web->maxscale);
  if(web->maxtemplate) fprintf(stream, "    MAXTEMPLATE \"%s\"\n", web->maxtemplate);
  if(web->minscale > -1) fprintf(stream, "    MINSCALE %g", web->minscale);
  if(web->mintemplate) fprintf(stream, "    MINTEMPLATE \"%s\"\n", web->mintemplate);
  if(web->template) fprintf(stream, "    TEMPLATE \"%s\"\n", web->template);
  fprintf(stream, "  END\n");
}

int loadWeb(webObj *web)
{
  for(;;) {
    switch(msyylex()) {
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
      return(-1);
    }
  }
}

static void loadWebString(webObj *web, char *value)
{
  switch(msyylex()) {
  case(EMPTY):
    free(web->empty);
    web->empty = strdup(value);
    break;
  case(ERROR):
    free(web->error);
    web->error = strdup(value);
    break;
  case(EXTENT):
    msyystate = 2; msyystring = value;
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
    msyystate = 2; msyystring = value;
    if(getDouble(&web->maxscale) == -1) return;
    break;
  case(MAXTEMPLATE):
    free(web->maxtemplate);
    web->maxtemplate = strdup(value);
    break;
  case(MINSCALE):
    msyystate = 2; msyystring = value;
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

  map->fontset.filename = NULL;
  map->fontset.numfonts = 0;  
  map->fontset.fonts = NULL;

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

  if(map->fontset.filename) {
    free(map->fontset.filename);
    msFreeHashTable(map->fontset.fonts);
  }

  freeWeb(&(map->web));

  freeScalebar(&(map->scalebar));
  freeReferenceMap(&(map->reference));
  freeLegend(&(map->legend));  

  for(i=0; i<map->numlayers; i++)
    freeLayer(&(map->layers[i]));
  free(map->layers);

  free(map);
}

int msSaveMap(mapObj *map, char *filename)
{
  int i;
  FILE *stream;

  if(!map) {
    msSetError(MS_MISCERR, "Map is undefined.", "msSaveMap()");
    return(-1);
  }

  if(!filename) {
    msSetError(MS_MISCERR, "Filename is undefined.", "msSaveMap()");
    return(-1);
  }

  stream = fopen(filename, "w");
  if(!stream) {
    msSetError(MS_IOERR, NULL, "msSaveMap()");
    sprintf(ms_error.message, "(%s)", filename);
    return(-1);
  }

  fprintf(stream, "MAP\n");
  fprintf(stream, "  EXTENT %g %g %g %g\n", map->extent.minx, map->extent.miny, map->extent.maxx, map->extent.maxy);
  if(map->fontset.filename) fprintf(stream, "  FONTSET \"%s\"\n", map->fontset.filename);
  fprintf(stream, "  IMAGECOLOR %d %d %d\n", map->imagecolor.red, map->imagecolor.green, map->imagecolor.blue);
  fprintf(stream, "  INTERLACE %s\n", msTrueFalse[map->interlace]);
  for(i=0; i<map->numlayers; i++) writeLayer(map, &(map->layers[i]), stream);
  writeLegend(map, &(map->legend), stream);
  if(map->lineset.filename) fprintf(stream, "  LINESET \"%s\"\n", map->lineset.filename);
  if(map->markerset.filename) fprintf(stream, "  MARKERSET \"%s\"\n", map->markerset.filename);
  fprintf(stream, "  NAME \"%s\"\n", map->name);
  writeProjection(&(map->projection), stream, "  ");
  writeQueryMap(map, &(map->querymap), stream);
  writeReferenceMap(&(map->reference), stream);
  writeScalebar(map, &(map->scalebar), stream);
  if(map->shadeset.filename) fprintf(stream, "  SHADESET \"%s\"\n", map->shadeset.filename);
  if(map->shapepath) fprintf(stream, "  SHAPEPATH \"%s\"\n", map->shapepath);
  fprintf(stream, "    SIZE %d %d\n", map->width, map->height);
  fprintf(stream, "    STATUS %s\n", msStatus[map->status]);
  if(map->tile) fprintf(stream, "  TILE \"%s\"\n", map->tile);
  fprintf(stream, "  TRANSPARENT %s\n", msTrueFalse[map->transparent]);
  fprintf(stream, "  UNITS %s\n", msUnits[map->units]);
  writeWeb(&(map->web), stream);
  fprintf(stream, "END\n");

  return(0);
}

mapObj *msLoadMap(char *filename)
{
  int n=0;
  regex_t re;
  mapObj *map=NULL;
  char *map_path=NULL;
  int i,j;

  if(!filename) {
    msSetError(MS_MISCERR, "Filename is undefined.", "msLoadMap()");
    return(NULL);
  }
  
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
  
  if((msyyin = fopen(filename,"r")) == NULL) {
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

  msyylineno = 0; /* reset line counter */

  if(initMap(map) == -1) /* initialize this map */
    return(NULL);

  for(;;) {

    switch(msyylex()) {   
    case(END):
      map->numlayers = n;
      fclose(msyyin);

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
      if((map->fontset.filename = getString()) == NULL) return(NULL);
      break;
    case(IMAGECOLOR):
      if(getInteger(&(map->imagecolor.red)) == -1) return(NULL);
      if(getInteger(&(map->imagecolor.green)) == -1) return(NULL);
      if(getInteger(&(map->imagecolor.blue)) == -1) return(NULL);
      break; 
    case(INTERLACE):
      if((map->interlace = getSymbol(2, MS_ON,MS_OFF)) == -1) return(NULL);
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
      sprintf(ms_error.message, "(%s):(%d)", msyytext, msyylineno);
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

  msyystate = 1; /* set lexer state */
  msyystring = object;

  ms_error.code = MS_NOERR; /* init error code */

  switch(msyylex()) {
  case(MAP):
    switch(msyylex()) {
    case(INTERLACE):
      msyystate = 2; msyystring = value;
      if((map->interlace = getSymbol(2, MS_ON,MS_OFF)) == -1) break;
      break;
    case(IMAGECOLOR):
      msyystate = 2; msyystring = value;
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
      msyystate = 2; msyystring = value;
      if(getInteger(&(map->width)) == -1) break;
      if(getInteger(&(map->height)) == -1) break;
      break;
    case(SHAPEPATH):
      free(map->shapepath);
      map->shapepath = strdup(value);
      break;
    case(MS_STRING):
      i = msGetLayerIndex(map, msyytext);
      if(i>=map->numlayers || i<0) break;
      loadLayerString(map, &(map->layers[i]), value);
    case(TILE):
      free(map->tile);
      map->tile = strdup(value);
      break; 
    case(TRANSPARENT):
      msyystate = 2; msyystring = value;
      if((map->transparent = getSymbol(2, MS_ON,MS_OFF)) == -1) break;
      break;
    case(UNITS):
      msyystate = 2; msyystring = value;
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

  msyystate = 3; /* restore lexer state */
  msyylex();

  if(ms_error.code != MS_NOERR)
    return(-1);

  return(0);
}

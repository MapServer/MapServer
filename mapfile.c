#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "map.h"
#include "mapfile.h"
#include "mapparser.h"
#include "mapthread.h"

extern int msyylex();
extern void msyyrestart();
extern double msyynumber;
extern char *msyytext;
extern int msyylineno;
extern FILE *msyyin;

extern int msyystate;
extern char *msyystring;

extern int loadSymbol(symbolObj *s); // in mapsymbol.c

/*
** Symbol to string static arrays needed for writing map files.
** Must be kept in sync with enumerations and defines found in map.h.
*/
static char *msUnits[7]={"INCHES", "FEET", "MILES", "METERS", "KILOMETERS", "DD", "PIXELS"};
static char *msLayerTypes[7]={"POINT", "LINE", "POLYGON", "RASTER", "ANNOTATION", "QUERY", "CIRCLE"};
static char *msLabelPositions[11]={"UL", "LR", "UR", "LL", "CR", "CL", "UC", "LC", "CC", "AUTO", "XY"};
static char *msBitmapFontSizes[5]={"TINY", "SMALL", "MEDIUM", "LARGE", "GIANT"};
static char *msQueryMapStyles[4]={"NORMAL", "HILITE", "SELECTED", "INVERTED"};
static char *msStatus[5]={"OFF", "ON", "DEFAULT", "QUERYONLY", "EMBED"};
// static char *msOnOff[2]={"OFF", "ON"};
static char *msTrueFalse[2]={"FALSE", "TRUE"};
// static char *msYesNo[2]={"NO", "YES"};
static char *msJoinType[2]={"SINGLE", "MULTIPLE"};

void msFree(void *p) {
  if(p) free(p);
}

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
** Lookup a color index from the palette returning the color value to 
** use with GD (or other?) drawing functions.
*/

int msLookupColor( mapObj *map, int color_index )

{
    if( color_index == -1 )
        return -1;
    else if( color_index < 1 || color_index > map->palette.numcolors )
        return -1;
    else
    {
/* -------------------------------------------------------------------- */
/*      For drivers other that GD return the color_index.               */
/* -------------------------------------------------------------------- */
      if( MS_RENDERER_GD(map->outputformat))
        return map->palette.colorvalue[color_index-1];
      else
        return  color_index;
    }
}

/*
** Applies a palette to a particular image
*/
int msLoadPalette(gdImagePtr img, paletteObj *palette, colorObj color)
{  
  int i;

  if(!img) {
    msSetError(MS_GDERR, "Image not initialized, can't allocate colors yet.", "msLoadPalette()");
    return(-1);
  }

  /* allocate the background color */
  gdImageColorAllocate(img, color.red, color.green, color.blue);

  /* now the palette */
  for(i=0; i<palette->numcolors; i++)
  {
      palette->colorvalue[i] = 
          gdImageColorAllocate(img, 
                               palette->colors[i].red, 
                               palette->colors[i].green, 
                               palette->colors[i].blue);
  }

  return(1);
}

/*
** Updates the gdImage palette, i.e. add to the gdImage any color from the
** mapserver palette that it didn't contain yet.
*/
int msUpdatePalette(gdImagePtr img, paletteObj *palette)
{  
  int i;

  if(!img) {
    msSetError(MS_GDERR, "Image not initialized, can't allocate colors yet.", "msUpdatePalette()");
    return(-1);
  }

  /* Add to image palette any color that it didn't contain yet. */
  for(i=gdImageColorsTotal(img)-1; i<palette->numcolors; i++)
    gdImageColorAllocate(img, palette->colors[i].red, palette->colors[i].green, palette->colors[i].blue);

  return(1);
}

/*
** Free memory allocated for a character array
*/
void msFreeCharArray(char **array, int num_items)
{
  int i;

  if((num_items < 0) || !array) return;

  for(i=0;i<num_items;i++)
    free(array[i]);
  msFree(array);

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

  msSetError(MS_SYMERR, "(%s):(%d)", "getSymbol()", msyytext, msyylineno);
  return(-1);
}

/*
** Get a string or symbol as a string.   Operates like getString(), but also
** supports symbols. 
*/
static char *getToken() {

    msyylex();

    return strdup(msyytext);
}



/*
** Load a string from the map file. A "string" is defined
** in lexer.l.
*/
char *getString() {
  
  if(msyylex() == MS_STRING)
    return(strdup(msyytext));

  msSetError(MS_SYMERR, "(%s):(%d)", "getString()", msyytext, msyylineno);
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

  msSetError(MS_SYMERR, "(%s):(%d)", "getDouble()", msyytext, msyylineno); 
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

  msSetError(MS_SYMERR, "(%s):(%d)", "getInteger()", msyytext, msyylineno); 
  return(-1);
}

int getCharacter(char *c) {
  if(msyylex() == MS_STRING) {
    *c = msyytext[0];
    return(0);
  }

  msSetError(MS_SYMERR, "(%s):(%d)", "getCharacter()", msyytext, msyylineno); 
  return(-1);
}

int msGetSymbolIndex(symbolSetObj *symbols, char *name)
{
  int i;

  if(!symbols || !name) return(-1);

  // symbol 0 has no name
  for(i=1; i<symbols->numsymbols; i++) {
    if(symbols->symbol[i].name)
      if(strcasecmp(symbols->symbol[i].name, name) == 0) return(i);
  }

  return(msAddImageSymbol(symbols, name)); /* make sure it's not a filename */
}

/*
** Return the index number for a given layer based on its name.
*/
int msGetLayerIndex(mapObj *map, char *name)
{
  int i;

  if(!name) return(-1);

  for(i=0;i<map->numlayers; i++) {
    if(!map->layers[i].name) /* skip it */
      continue;
    if(strcmp(name, map->layers[i].name) == 0)
      return(i);
  }
  return(-1);
}

// Initialize and load a single color object
void initColor(colorObj *color, int red, int green, int blue) {
  color->red = red;
  color->green = green;
  color->blue = blue;
  color->pen = MS_PEN_UNSET;
}

// converts a 2 character hexidecimal string to an integer
static int hex2int(char *hex) {
  int number;

  number = (hex[0] >= 'A' ? ((hex[0] & 0xdf) - 'A')+10 : (hex[0] - '0'));
  number *= 16;
  number += (hex[1] >= 'A' ? ((hex[1] & 0xdf) - 'A')+10 : (hex[1] - '0'));
   
  return(number);
}

int loadColor(colorObj *color) {
  char hex[2];

  if(getInteger(&(color->red)) == -1) {
    if(msyytext[0] == '#' && strlen(msyytext) == 7) { // got a hex color
      hex[0] = msyytext[1];
      hex[1] = msyytext[2];
      color->red = hex2int(hex);      
      hex[0] = msyytext[3];
      hex[1] = msyytext[4];
      color->green = hex2int(hex);
      hex[0] = msyytext[5];
      hex[1] = msyytext[6];
      color->blue = hex2int(hex);

      return(MS_SUCCESS);
    }
    return(MS_FAILURE);
  }
  if(getInteger(&(color->green)) == -1) return(MS_FAILURE);
  if(getInteger(&(color->blue)) == -1) return(MS_FAILURE);

  return(MS_SUCCESS);
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

  msFree(join->name);
  msFree(join->table);
  msFree(join->from);
  msFree(join->to);

  msFree(join->match);

  msFree(join->header);
  msFree(join->template);
  msFree(join->footer);

  msFreeCharArray(join->items, join->numitems); /* these may have been free'd elsewhere */
  for(i=0; i<join->numrecords; i++)
    msFreeCharArray(join->data[i], join->numitems);
  msFree(join->data);
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
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadJoin()", msyytext, msyylineno);
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

/* inserts a feature at the end of the list, can create a new list */
featureListNodeObjPtr insertFeatureList(featureListNodeObjPtr *list, shapeObj *shape)
{
  featureListNodeObjPtr node, current, previous;
  int i=0;

  if((node = (featureListNodeObjPtr) malloc(sizeof(featureListNodeObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "insertFeature()");
    return(NULL);
  }

  msInitShape(&(node->shape));
  if(msCopyShape(shape, &(node->shape)) == -1) return(NULL);

  node->next = NULL;

  previous = NULL;
  current = *list;
  while(current != NULL) {
    previous = current;
    current = current->next;
    i++;
  }
 
  if(previous == NULL) {
    *list = node;
  } else
    previous->next = node;

  return(node); // a pointer to last object in the list
}

void freeFeatureList(featureListNodeObjPtr list)
{
  if(list) {
    freeFeatureList(list->next); /* free any children */
    msFreeShape(&(list->shape));
    msFree(list);
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
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadFeaturePoints()", 
                 msyytext, msyylineno);          
      return(-1);      
    }
  }
}

static int loadFeature(featureListNodeObjPtr *list, int type)
{
  multipointObj points={0,NULL};
  shapeObj shape;

  msInitShape(&shape);
  shape.type = type;

  for(;;) {
    switch(msyylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadFeature()");      
      return(-1);
    case(END):
      if(insertFeatureList(list, &shape) == NULL) return(-1);
      msFreeShape(&shape);
      return(0);
    case(POINTS):
      if(loadFeaturePoints(&points) == -1) return(-1);
      if(msAddLine(&shape, &points) == -1) return(-1);
      msFree(points.point); /* reset */
      points.numpoints = 0;
      break;
    case(TEXT):
      if((shape.text = getString()) == NULL) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadfeature()",
                 msyytext, msyylineno);
      return(-1);
    }
  } /* next token */  
}

static void writeFeature(shapeObj *shape, FILE *stream) 
{
  int i,j;

  fprintf(stream, "    FEATURE\n");

  for(i=0; i<shape->numlines; i++) {
    fprintf(stream, "      POINTS\n");
    for(j=0; j<shape->line[i].numpoints; j++)
      fprintf(stream, "        %g %g\n", shape->line[i].point[j].x, shape->line[i].point[j].y);
    fprintf(stream, "      END\n");
  }

  if(shape->text) fprintf(stream, "      TEXT \"%s\"\n", shape->text);
  fprintf(stream, "    END\n");
}

/*
** Initialize, load and free a projectionObj structure
*/
int msInitProjection(projectionObj *p)
{
#ifdef USE_PROJ  
  p->numargs = 0;
  p->proj = NULL;
  if((p->args = (char **)malloc(MS_MAXPROJARGS*sizeof(char *))) == NULL) {
    msSetError(MS_MEMERR, NULL, "initProjection()");
    return(-1);
  }
#endif
  return(0);

}

void msFreeProjection(projectionObj *p) {
#ifdef USE_PROJ
  if(p->proj)
  {
      pj_free(p->proj);
      p->proj = NULL;
  }

  msFreeCharArray(p->args, p->numargs);  
  p->args = NULL;
  p->numargs = 0;
#endif
}

int msProcessProjection(projectionObj *p)
{
#ifdef USE_PROJ    
    assert( p->proj == NULL );
    
    if( strcasecmp(p->args[0],"GEOGRAPHIC") == 0 ) {
        msSetError(MS_PROJERR, 
                   "PROJECTION 'GEOGRAPHIC' no longer supported.\n"
                   "Provide explicit definition.\n"
                   "ie. proj=latlong\n"
                   "    ellps=clrk66\n",
                   "msProcessProjection()");	  
        return(-1);
    }

    if (strcasecmp(p->args[0], "AUTO") == 0) {
	p->proj = NULL;
        return 0;
    }

    msAcquireLock( TLOCK_PROJ );
    if( !(p->proj = pj_init(p->numargs, p->args)) ) {
        msReleaseLock( TLOCK_PROJ );
        msSetError(MS_PROJERR, pj_strerrno(pj_errno), 
                   "msProcessProjection()");	  
        return(-1);
    }
    
    msReleaseLock( TLOCK_PROJ );

    return(0);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", 
             "msProcessProjection()");
  return(-1);
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
      return msProcessProjection(p);
      break;    
    case(MS_STRING):
    case(MS_AUTO):
      p->args[i] = strdup(msyytext);
      i++;
      break;

    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadProjection()",
                 msyytext, msyylineno);
      return(-1);
    }
  } /* next token */
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "loadProjection()");
  return(-1);
#endif
}

int msLoadProjectionString(projectionObj *p, char *value)
{
#ifdef USE_PROJ
  if(p) msFreeProjection(p);

  /*
   * Handle new style definitions, the same as they would be given to
   * the proj program.  
   * eg. 
   *    "+proj=utm +zone=11 +ellps=WGS84"
   */
  if( value[0] == '+' )
  {
      char 	*trimmed;
      int	i, i_out=0;

      trimmed = strdup(value);
      for( i = 0; value[i] != '\0'; i++ )
      {
          if( !isspace( value[i] ) )
              trimmed[i_out++] = value[i];
      }
      trimmed[i_out] = '\0';
      
      p->args = split(trimmed,'+', &p->numargs);
      free( trimmed );
  }

  /*
   * Handle old style comma delimited.  eg. "proj=utm,zone=11,ellps=WGS84".
   */
  else
  {
      p->args = split(value,',', &p->numargs);
  }

  return msProcessProjection( p );
#else
  msSetError(MS_PROJERR, "Projection support is not available.", 
             "msLoadProjectionString()");
  return(-1);
#endif
}

static void writeProjection(projectionObj *p, FILE *stream, char *tab) {
#ifdef USE_PROJ  
  int i;

  if(p->numargs > 0) {
    fprintf(stream, "%sPROJECTION\n", tab);
    for(i=0; i<p->numargs; i++)
      fprintf(stream, "  %s\"%s\"\n", tab, p->args[i]);
    fprintf(stream, "%sEND\n", tab);
  }
#endif
}

/*
** Initialize, load and free a labelObj structure
*/
void initLabel(labelObj *label)
{
  label->antialias = -1; /* off */

  label->color = 1;
  label->outlinecolor = -1; /* don't use it */

  label->shadowcolor = -1; /* don't use it */
  label->shadowsizex = label->shadowsizey = 1;

  label->backgroundcolor = -1; /* don't use it */
  label->backgroundshadowcolor = -1; /* don't use it */
  label->backgroundshadowsizex = label->backgroundshadowsizey = 1;

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
  label->wrap = '\0';

  label->force = MS_FALSE;
  return;
}

static void freeLabel(labelObj *label)
{
  msFree(label->font);
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
	label->angle = msyynumber;
      else
	label->autoangle = MS_TRUE;
      break;
    case(ANTIALIAS):
      if((label->antialias = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) 
	return(-1);
      break;
    case(BACKGROUNDCOLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      label->backgroundcolor = msAddColor(map,red,green,blue);
      break;
    case(BACKGROUNDSHADOWCOLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      label->backgroundshadowcolor = msAddColor(map,red,green,blue);
      break;
   case(BACKGROUNDSHADOWSIZE):
      if(getInteger(&(label->backgroundshadowsizex)) == -1) return(-1);
      if(getInteger(&(label->backgroundshadowsizey)) == -1) return(-1);
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
#if defined (USE_GD_TTF) || defined (USE_GD_FT)
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
#if defined (USE_GD_TTF) || defined (USE_GD_FT)
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
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadlabel()", 
                 msyytext, msyylineno);
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
      label->angle = msyynumber;
    else
      label->autoangle = MS_TRUE;
    break;
  case(ANTIALIAS):
    msyystate = 2; msyystring = value;
    if((label->antialias = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
    break;  
  case(BUFFER):
    msyystate = 2; msyystring = value;
    if(getInteger(&(label->buffer)) == -1) break;
    break;
  case(BACKGROUNDCOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    label->backgroundcolor = msAddColor(map,red,green,blue);     
    break;
  case(BACKGROUNDSHADOWCOLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    label->backgroundshadowcolor = msAddColor(map,red,green,blue);     
    break;
  case(BACKGROUNDSHADOWSIZE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(label->backgroundshadowsizex)) == -1) return;
    if(getInteger(&(label->backgroundshadowsizey)) == -1) return;
    break;
  case(COLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    label->color = msAddColor(map,red,green,blue);     
    break;    
  case(FONT):
#if defined (USE_GD_TTF) || defined (USE_GD_FT)
    msFree(label->font);
    label->font = strdup(value);
    
    if(!(msLookupHashTable(map->fontset.fonts, label->font))) {
      msSetError(MS_IDENTERR, "Unknown font alias. (%s)", "loadLabelString()",
                 msyytext);      
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
#if defined (USE_GD_TTF) || defined (USE_GD_FT)
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
    label->wrap = value[0];
    break;
  default:
    break;
  }
}

static void writeLabel(mapObj *map, labelObj *label, FILE *stream, char *tab)
{
  if(label->size == -1) return; // there is no default label anymore

  fprintf(stream, "%sLABEL\n", tab);
  if(label->type == MS_BITMAP) {
    fprintf(stream, "  %sSIZE %s\n", tab, msBitmapFontSizes[label->size]);
    fprintf(stream, "  %sTYPE BITMAP\n", tab);
  } else {
    if(label->autoangle)
      fprintf(stream, "  %sANGLE AUTO\n", tab);
    else
      fprintf(stream, "  %sANGLE %f\n", tab, label->angle);
    if(label->antialias) fprintf(stream, "  %sANTIALIAS TRUE\n", tab);
    fprintf(stream, "  %sFONT %s\n", tab, label->font);
    fprintf(stream, "  %sMAXSIZE %d\n", tab, label->maxsize);
    fprintf(stream, "  %sMINSIZE %d\n", tab, label->minsize);
    fprintf(stream, "  %sSIZE %d\n", tab, label->size);
    fprintf(stream, "  %sTYPE TRUETYPE\n", tab);
  }  

  if(label->backgroundcolor > -1) {
    fprintf(stream, "  %sBACKGROUNDCOLOR %d %d %d\n", tab, map->palette.colors[label->backgroundcolor-1].red, map->palette.colors[label->backgroundcolor-1].green, map->palette.colors[label->backgroundcolor-1].blue);
    if(label->backgroundshadowcolor > -1) {
      fprintf(stream, "  %sBACKGROUNDSHADOWCOLOR %d %d %d\n", tab, map->palette.colors[label->backgroundshadowcolor-1].red, map->palette.colors[label->backgroundshadowcolor-1].green, map->palette.colors[label->backgroundshadowcolor-1].blue);
      fprintf(stream, "  %sBACKGROUNDSHADOWSIZE %d %d\n", tab, label->backgroundshadowsizex, label->backgroundshadowsizey);
    }
  }

  fprintf(stream, "  %sBUFFER %d\n", tab, label->buffer);
  if(label->color > -1) fprintf(stream, "  %sCOLOR %d %d %d\n", tab, map->palette.colors[label->color-1].red, map->palette.colors[label->color-1].green, map->palette.colors[label->color-1].blue);
  fprintf(stream, "  %sFORCE %s\n", tab, msTrueFalse[label->force]);
  fprintf(stream, "  %sMINDISTANCE %d\n", tab, label->mindistance);
  if(label->autominfeaturesize)
    fprintf(stream, "  %sMINFEATURESIZE AUTO\n", tab);
  else
    fprintf(stream, "  %sMINFEATURESIZE %d\n", tab, label->minfeaturesize);
  fprintf(stream, "  %sOFFSET %d %d\n", tab, label->offsetx, label->offsety);
  if(label->outlinecolor > -1) fprintf(stream, "  %sOUTLINECOLOR %d %d %d\n", tab, map->palette.colors[label->outlinecolor-1].red, map->palette.colors[label->outlinecolor-1].green, map->palette.colors[label->outlinecolor-1].blue);
  fprintf(stream, "  %sPARTIALS %s\n", tab, msTrueFalse[label->partials]);
  fprintf(stream, "  %sPOSITION %s\n", tab, msLabelPositions[label->position]);
  if(label->shadowcolor > -1) {
    fprintf(stream, "  %sSHADOWCOLOR %d %d %d\n", tab, map->palette.colors[label->shadowcolor-1].red, map->palette.colors[label->shadowcolor-1].green, map->palette.colors[label->shadowcolor-1].blue);
    fprintf(stream, "  %sSHADOWSIZE %d %d\n", tab, label->shadowsizex, label->shadowsizey);
  }
  if(label->wrap) fprintf(stream, "  %sWRAP %c\n", tab, label->wrap);
  fprintf(stream, "%sEND\n", tab);  
}

void initExpression(expressionObj *exp)
{
  exp->type = MS_STRING;
  exp->string = NULL;
  exp->items = NULL;
  exp->indexes = NULL;
  exp->numitems = 0;
  exp->compiled = MS_FALSE;
}

void freeExpression(expressionObj *exp)
{
  if(!exp) return;

  msFree(exp->string);
  if(exp->type == MS_REGEX && exp->compiled) regfree(&(exp->regex));
  if(exp->type == MS_EXPRESSION && exp->numitems > 0) msFreeCharArray(exp->items, exp->numitems);
  msFree(exp->indexes);

  initExpression(exp); // re-initialize
}

int loadExpression(expressionObj *exp)
{
  if((exp->type = getSymbol(3, MS_STRING,MS_EXPRESSION,MS_REGEX)) == -1) return(-1);
  exp->string = strdup(msyytext);
  
  // if(exp->type == MS_REGEX) {
  //   if(regcomp(&(exp->regex), exp->string, REG_EXTENDED|REG_NOSUB) != 0) { // compile the expression 
  //     sprintf(ms_error.message, "(%s):(%d)", exp->string, msyylineno);
  //     msSetError(MS_REGEXERR, ms_error.message, "loadExpression()");
  //     return(-1);
  //   }
  // }

  return(0);
}

int loadExpressionString(expressionObj *exp, char *value)
{
  msyystate = 2; msyystring = value;

  freeExpression(exp); // we're totally replacing the old expression so free then init to start over
  // initExpression(exp);

  if((exp->type = getSymbol(3, MS_STRING,MS_EXPRESSION,MS_REGEX)) == -1) return(-1);
  exp->string = strdup(msyytext);
  
  // if(exp->type == MS_REGEX) {
  //   if(regcomp(&(exp->regex), exp->string, REG_EXTENDED|REG_NOSUB) != 0) { // compile the expression 
  //     sprintf(ms_error.message, "(%s):(%d)", exp->string, msyylineno);
  //     msSetError(MS_REGEXERR, ms_error.message, "loadExpression()");
  //     return(-1);
  //   }
  // }

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

int loadHashTable(hashTableObj *ptable)
{
  char *key=NULL, *data=NULL;
  
  if (!(*ptable)) *ptable = msCreateHashTable();

  for(;;) {
    switch(msyylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadHashTable()");
      return(MS_FAILURE);
    case(END):
      return(MS_SUCCESS);
    case(MS_STRING):
      key = strdup(msyytext);

      data = getString();
      if(!data) return(MS_FAILURE);
      
      msInsertHashTable(*ptable, key, data);
      
      free(key);
      free(data);
      break;
    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadHashTable()",
                 msyytext, msyylineno);
      return(MS_FAILURE);
    }
  }

  return(MS_SUCCESS);
}

static void writeHashTable(hashTableObj table, FILE *stream, char *tab, char *title) {
  struct hashObj *tp;
  int i;

  if(!table) return;

  fprintf(stream, "%s%s\n", tab, title);  

  for (i=0;i<MS_HASHSIZE; i++) {
    if (table[i] != NULL) {
      for (tp=table[i]; tp!=NULL; tp=tp->next)
	fprintf(stream, "%s  \"%s\"\t\"%s\"\n", tab, tp->key, tp->data);
    }
  }

  fprintf(stream, "%sEND\n", tab);
}

/*
** Initialize, load and free a single class
*/
int initClass(classObj *class)
{
  class->status = MS_ON;

  initExpression(&(class->expression));
  class->name = NULL;
  class->title = NULL;
  initExpression(&(class->text));
  class->color = -1; /* must explictly set a color */
  class->symbol = 0;
  class->sizescaled = class->size = 1; /* one pixel */
  class->minsize = MS_MINSYMBOLSIZE;
  class->maxsize = MS_MAXSYMBOLSIZE;
  class->backgroundcolor = -1;
  class->outlinecolor = -1;

  class->overlaybackgroundcolor = -1;
  class->overlaycolor = -1;
  class->overlayoutlinecolor = -1;
  class->overlaysizescaled = class->overlaysize = 1;
  class->overlayminsize = MS_MINSYMBOLSIZE;
  class->overlaymaxsize = MS_MAXSYMBOLSIZE;
  class->overlaysymbol = -1;
  class->overlaysymbolname = NULL;

  initLabel(&(class->label));
  class->label.sizescaled = class->label.size = -1; // no default
  class->symbolname = NULL;

  class->numjoins = 0;
  class->template = NULL;
  if((class->joins = (joinObj *)malloc(MS_MAXJOINS*sizeof(joinObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "initClass()");
    return(-1);
  }

  class->type = -1;

  class->metadata = NULL;

  class->maxscale = class->minscale = -1.0;

  return(0);
}

void freeClass(classObj *class)
{
  int i;

  freeLabel(&(class->label));
  freeExpression(&(class->expression));
  freeExpression(&(class->text));
  msFree(class->name);
  msFree(class->title);
  msFree(class->symbolname);
  msFree(class->overlaysymbolname);
  msFree(class->template);
  for(i=0;i<class->numjoins;i++) /* each join */    
    freeJoin(&(class->joins[i]));
  msFree(class->joins);
  if(class->metadata) msFreeHashTable(class->metadata);
}

/*
 * Reset style info in the class to defaults
 * the only members we don't touch are name, expression, and join/query stuff
 * This is used with STYLEITEM before overwriting the contents of a class.
 */
void resetClassStyle(classObj *class)
{
  freeLabel(&(class->label));
  freeExpression(&(class->text));
  msFree(class->symbolname);
  msFree(class->overlaysymbolname);

  initExpression(&(class->text));
  class->color = -1; /* must explictly set a color */
  class->symbol = 0;
  class->sizescaled = class->size = 1; /* one pixel */
  class->minsize = MS_MINSYMBOLSIZE;
  class->maxsize = MS_MAXSYMBOLSIZE;
  class->backgroundcolor = -1;
  class->outlinecolor = -1;

  class->overlaybackgroundcolor = -1;
  class->overlaycolor = -1;
  class->overlayoutlinecolor = -1;
  class->overlaysizescaled = class->overlaysize = 1;
  class->overlayminsize = MS_MINSYMBOLSIZE;
  class->overlaymaxsize = MS_MAXSYMBOLSIZE;
  class->overlaysymbol = -1;
  class->overlaysymbolname = NULL;

  initLabel(&(class->label));
  class->label.sizescaled = class->label.size = -1; // no default
  class->symbolname = NULL;

  class->type = -1;
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
    case(JOIN):
      if(loadJoin(&(class->joins[class->numjoins])) == -1) return(-1);
      class->numjoins++;
      break;
    case(LABEL):
      class->label.sizescaled = class->label.size = MS_MEDIUM; // only set a default if the LABEL section is present
      if(loadLabel(&(class->label), map) == -1) return(-1);
      break;
    case(MAXSCALE):      
      if(getDouble(&(class->maxscale)) == -1) return(-1);
      break;
    case(MAXSIZE):
      if(getInteger(&(class->maxsize)) == -1) return(-1);
      break;
    case(METADATA):
      if(loadHashTable(&(class->metadata)) != MS_SUCCESS) return(-1);
      break;
    case(MINSCALE):      
      if(getDouble(&(class->minscale)) == -1) return(-1);
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
    case(OVERLAYBACKGROUNDCOLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      class->overlaybackgroundcolor = msAddColor(map, red, green, blue);
      break;
    case(OVERLAYCOLOR):
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      class->overlaycolor = msAddColor(map, red, green, blue);
      break;
    case(OVERLAYMAXSIZE):
      if(getInteger(&(class->overlaymaxsize)) == -1) return(-1);
      break;
    case(OVERLAYMINSIZE):      
      if(getInteger(&(class->overlayminsize)) == -1) return(-1);
      break;
    case(OVERLAYOUTLINECOLOR):      
      if(getInteger(&(red)) == -1) return(-1);
      if(getInteger(&(green)) == -1) return(-1);
      if(getInteger(&(blue)) == -1) return(-1);
      class->overlayoutlinecolor = msAddColor(map, red, green, blue);
      break;
    case(OVERLAYSIZE):
      if(getInteger(&(class->overlaysize)) == -1) return(-1);
      class->overlaysizescaled = class->overlaysize;
      break;
    case(OVERLAYSYMBOL):
      if((state = getSymbol(2, MS_NUMBER,MS_STRING)) == -1) return(-1);

      if(state == MS_NUMBER)
	class->overlaysymbol = (int) msyynumber;
      else
	class->overlaysymbolname = strdup(msyytext);
      break;
    case(SIZE):
      if(getInteger(&(class->size)) == -1) return(-1);
      class->sizescaled = class->size;
      break;
    case(STATUS):
      if((class->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;
    case(SYMBOL):
      if((state = getSymbol(2, MS_NUMBER,MS_STRING)) == -1) return(-1);

      if(state == MS_NUMBER)
	class->symbol = (int) msyynumber;
      else
	class->symbolname = strdup(msyytext);
      break;
    case(TEMPLATE):      
      if((class->template = getString()) == NULL) return(-1);
      break;
    case(TEXT):
      if(loadExpression(&(class->text)) == -1) return(-1);
      if((class->text.type != MS_STRING) && (class->text.type != MS_EXPRESSION)) {
	msSetError(MS_MISCERR, "Text expressions support constant or replacement strings." , "loadClass()");
	return(-1);
      }
      break;
    case(TITLE):
      if((class->title = getString()) == NULL) return(-1);
      break;
    case(TYPE):
      if((class->type = getSymbol(6, MS_LAYER_POINT,MS_LAYER_LINE,MS_LAYER_RASTER,MS_LAYER_POLYGON,MS_LAYER_ANNOTATION,MS_LAYER_CIRCLE)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadClass()",
                 msyytext, msyylineno);
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
  case(MAXSCALE):
    msyystate = 2; msyystring = value;
    getDouble(&(class->maxscale));
    break;
  case(MAXSIZE):
    msyystate = 2; msyystring = value;
    getInteger(&(class->maxsize));
    break; 
  case(MINSCALE):
    msyystate = 2; msyystring = value;
    getDouble(&(class->minscale));
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
  case(STATUS):
    msyystate = 2; msyystring = value;
    if((class->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return;      
    break;
  case(SYMBOL):
    msyystate = 2; msyystring = value;
    if((state = getSymbol(2, MS_NUMBER,MS_STRING)) == -1) return;

    if(state == MS_NUMBER)
      class->symbol = (int) msyynumber;
    else {
      if((class->symbol = msGetSymbolIndex(&(map->symbolset), msyytext)) == -1)
	msSetError(MS_EOFERR, "Undefined symbol.", "loadClassString()");
    }
    break;
  case(TEMPLATE):
    msFree(class->template);
    class->template = strdup(value);
    break;
  case(TEXT):
    if(loadExpressionString(&(class->text), value) == -1) return;
    if((class->text.type != MS_STRING) && (class->text.type != MS_EXPRESSION)) msSetError(MS_MISCERR, "Text expressions support constant or replacement strings." , "loadClass()");
  case(TITLE):
    msFree(class->title);
    class->title = strdup(value);
    break;
  case(TYPE):
    msyystate = 2; msyystring = value;
    if((class->type = getSymbol(6, MS_LAYER_POINT,MS_LAYER_LINE,MS_LAYER_RASTER,MS_LAYER_POLYGON,MS_LAYER_ANNOTATION,MS_LAYER_CIRCLE)) == -1) return;
    break;
  default:
    break;
  }

  return;
}

static void writeClass(mapObj *map, classObj *class, FILE *stream)
{
  int i;

  if (class->status == MS_DELETE)
      return;

  fprintf(stream, "    CLASS\n");
  if(class->name) fprintf(stream, "      NAME \"%s\"\n", class->name);
  if(class->backgroundcolor > -1) fprintf(stream, "      BACKGROUNDCOLOR %d %d %d\n", map->palette.colors[class->backgroundcolor-1].red, map->palette.colors[class->backgroundcolor-1].green, map->palette.colors[class->backgroundcolor-1].blue);
  if(class->color > -1) fprintf(stream, "      COLOR %d %d %d\n", map->palette.colors[class->color-1].red, map->palette.colors[class->color-1].green, map->palette.colors[class->color-1].blue);
  if(class->expression.string) {
    fprintf(stream, "      EXPRESSION ");
    writeExpression(&(class->expression), stream);
    fprintf(stream, "\n");
  }
  for(i=0; i<class->numjoins; i++)
    writeJoin(&(class->joins[i]), stream);
  writeLabel(map, &(class->label), stream, "      ");
  if(class->maxscale > -1) fprintf(stream, "      MAXSCALE %g\n", class->maxscale);
  if(class->maxsize > -1) fprintf(stream, "      MAXSIZE %d\n", class->maxsize);
  if(class->metadata) writeHashTable(class->metadata, stream, "      ", "METADATA");
  if(class->minscale > -1) fprintf(stream, "      MINSCALE %g\n", class->minscale);
  if(class->minsize > -1) fprintf(stream, "      MINSIZE %d\n", class->minsize);
  if(class->outlinecolor > -1) fprintf(stream, "      OUTLINECOLOR %d %d %d\n", map->palette.colors[class->outlinecolor-1].red, map->palette.colors[class->outlinecolor-1].green, map->palette.colors[class->outlinecolor-1].blue);
  if(class->overlaycolor > -1) {
    fprintf(stream, "      OVERLAYCOLOR %d %d %d\n", map->palette.colors[class->overlaycolor-1].red, map->palette.colors[class->overlaycolor-1].green, map->palette.colors[class->overlaycolor-1].blue);
    fprintf(stream, "      OVERLAYSIZE %d\n", class->overlaysize);
    if(class->overlaysymbolname)
      fprintf(stream, "      OVERLAYSYMBOL \"%s\"\n", class->overlaysymbolname);
    else
      fprintf(stream, "      OVERLAYSYMBOL %d\n", class->overlaysymbol);
  }
  fprintf(stream, "      SIZE %d\n", class->size);
  if(class->status == MS_OFF) fprintf(stream, "      STATUS OFF\n");
  if(class->symbolname)
    fprintf(stream, "      SYMBOL \"%s\"\n", class->symbolname);
  else
    fprintf(stream, "      SYMBOL %d\n", class->symbol);
  if(class->template) fprintf(stream, "      TEMPLATE \"%s\"\n", class->template);
  if(class->text.string) {
    fprintf(stream, "      TEXT ");
    writeExpression(&(class->text), stream);
    fprintf(stream, "\n");
  }  
  if(class->title) 
    fprintf(stream, "      TITLE \"%s\"\n", class->title);
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
  
  layer->name = NULL;
  layer->group = NULL;
  layer->status = MS_OFF;
  layer->data = NULL;

  layer->type = -1;

  layer->annotate = MS_FALSE;

  layer->toleranceunits = MS_PIXELS;
  layer->tolerance = 3; /* perhaps this should have a different value based on type */

  layer->symbolscale = -1.0; /* -1 means nothing is set */
  layer->maxscale = -1.0;
  layer->minscale = -1.0;

  layer->sizeunits = MS_PIXELS;

  layer->maxfeatures = -1; /* no quota */
  
  layer->template = layer->header = layer->footer = NULL;

  layer->transform = MS_TRUE;

  layer->classitem = NULL;
  layer->classitemindex = -1;
  layer->numclasses = 0;

  layer->units = MS_METERS;
  if(msInitProjection(&(layer->projection)) == -1)
    return(-1);

  layer->offsite = -1;

  layer->labelcache = MS_ON;
  layer->postlabelcache = MS_FALSE;

  layer->labelitem = layer->labelsizeitem = layer->labelangleitem = NULL;
  layer->labelitemindex = layer->labelsizeitemindex = layer->labelangleitemindex = -1;

  layer->labelmaxscale = -1;
  layer->labelminscale = -1;

  layer->tileitem = strdup("location");
  layer->tileindex = NULL;

  layer->currentfeature = layer->features = NULL;

  layer->connection = NULL;
  layer->connectiontype = MS_SHAPEFILE;

  layer->ogrlayerinfo = layer->sdelayerinfo = layer->postgislayerinfo = layer->oraclespatiallayerinfo = NULL;

  layer->items = NULL;
  layer->iteminfo = NULL;
  layer->numitems = 0;

  layer->resultcache= NULL;

  initExpression(&(layer->filter));
  layer->filteritem = NULL;
  layer->filteritemindex = -1;

  layer->requires = layer->labelrequires = NULL;

  layer->metadata = NULL;
  layer->dump = MS_FALSE;

  layer->styleitem = NULL;
  layer->styleitemindex = -1;

  layer->transparency = 0;
  
  return(0);
}

void freeLayer(layerObj *layer) {
  int i;

  msFree(layer->name);
  msFree(layer->group);
  msFree(layer->data);
  msFree(layer->classitem);
  msFree(layer->labelitem);
  msFree(layer->labelsizeitem);
  msFree(layer->labelangleitem);
  msFree(layer->header);
  msFree(layer->footer);
  msFree(layer->template);
  msFree(layer->tileindex);
  msFree(layer->tileitem);
  msFree(layer->connection);

  msFreeProjection(&(layer->projection));

  for(i=0;i<layer->numclasses;i++)
    freeClass(&(layer->class[i]));
  msFree(layer->class);

  if(layer->features)
    freeFeatureList(layer->features);

  if(layer->resultcache) {    
    if(layer->resultcache->results) free(layer->resultcache->results);
    msFree(layer->resultcache);
  }

  msFree(layer->filteritem);
  freeExpression(&(layer->filter));

  msFree(layer->requires);
  msFree(layer->labelrequires);

  if(layer->metadata) msFreeHashTable(layer->metadata);

  msFree(layer->styleitem);
}

int loadLayer(layerObj *layer, mapObj *map)
{
  int c=0; // class counter
  int type;

  if(initLayer(layer) == -1)
    return(-1);

  for(;;) {
    switch(msyylex()) {
    case(CLASS):
      if(loadClass(&(layer->class[c]), map) == -1) return(-1);
      if(layer->class[c].type == -1) layer->class[c].type = layer->type;
      c++;
      break;
    case(CLASSITEM):
      if((layer->classitem = getString()) == NULL) return(-1);
      break;
    case(CONNECTION):
      if((layer->connection = getString()) == NULL) return(-1);
      break;
    case(CONNECTIONTYPE):
      if((layer->connectiontype = getSymbol(5, MS_SDE, MS_OGR, MS_POSTGIS, MS_WMS, MS_ORACLESPATIAL)) == -1) return(-1);
      break;
    case(DATA):
      if((layer->data = getString()) == NULL) return(-1);
      break;
    case(DUMP):
      if((layer->dump = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return(-1);
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadLayer()");      
      return(-1);
      break;
    case(END):
      layer->numclasses = c;
 
      // May want to re-write this test to be more specific.
      // if(!layer->connection && layer->data && layer->numclasses > 1 && !layer->classitem) {
      //   msSetError(MS_MISCERR, "Multiple classes defined but no classitem?", "loadLayer()");      
      //   return(-1);
      // }

      if(layer->type == -1) {
	msSetError(MS_MISCERR, "Layer type not set.", "loadLayer()");      
	return(-1);
      }

      return(0);
      break;
    case(FEATURE):
      if(layer->type == -1) {
	msSetError(MS_MISCERR, "Layer type must be set before defining inline features.", "loadLayer()");      
	return(-1);
      }

      if(layer->type == MS_LAYER_POLYGON)
	type = MS_SHAPE_POLYGON;
      else if(layer->type == MS_LAYER_LINE)
	type = MS_SHAPE_LINE;
      else
	type = MS_SHAPE_POINT;	

      if(loadFeature(&(layer->features), type) == -1) return(-1);      
      layer->connectiontype = MS_INLINE;
      break;
    case(FILTER):
      if(loadExpression(&(layer->filter)) == -1) return(-1);
      break;
    case(FILTERITEM):
      if((layer->filteritem = getString()) == NULL) return(-1);
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
    case(LABELREQUIRES):
      if((layer->labelrequires = getString()) == NULL) return(-1);
      break;
    case(LABELSIZEITEM):
      if((layer->labelsizeitem = getString()) == NULL) return(-1);
      break;
    case(MAXFEATURES):
      if(getInteger(&(layer->maxfeatures)) == -1) return(-1);
      break;
    case(MAXSCALE):      
      if(getDouble(&(layer->maxscale)) == -1) return(-1);
      break;
    case(METADATA):
      if(loadHashTable(&(layer->metadata)) != MS_SUCCESS) return(-1);
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
    case(TRANSPARENCY):
      if(getInteger(&(layer->transparency)) == -1) return(-1);         
      break;
    case(POSTLABELCACHE):
      if((layer->postlabelcache = getSymbol(2, MS_TRUE, MS_FALSE)) == -1) return(-1);
      if(layer->postlabelcache)
	layer->labelcache = MS_OFF;
      break;
    case(PROJECTION):
      if(loadProjection(&(layer->projection)) == -1) return(-1);
      break;
    case(REQUIRES):
      if((layer->requires = getString()) == NULL) return(-1);
      break;
    case(SIZEUNITS):
      if((layer->sizeunits = getSymbol(7, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD,MS_PIXELS)) == -1) return(-1);
      break;
    case(STATUS):
      if((layer->status = getSymbol(3, MS_ON,MS_OFF,MS_DEFAULT)) == -1) return(-1);
      break;
    case(STYLEITEM):
      if((layer->styleitem = getString()) == NULL) return(-1);
      break;
    case(SYMBOLSCALE):      
      if(getDouble(&(layer->symbolscale)) == -1) return(-1);
      break;
    case(TEMPLATE):
      if((layer->template = getString()) == NULL) return(-1);
      break;
    case(TILEINDEX):
      if((layer->tileindex = getString()) == NULL) return(-1);
      break;
    case(TILEITEM):
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
      if((layer->type = getSymbol(8, MS_LAYER_POINT,MS_LAYER_LINE,MS_LAYER_RASTER,MS_LAYER_POLYGON,MS_LAYER_ANNOTATION,MS_LAYER_QUERY,MS_LAYER_CIRCLE)) == -1) return(-1);
      break;    
    case(UNITS):
      if((layer->units = getSymbol(7, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD,MS_PIXELS)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadLayer()",
                 msyytext, msyylineno);      
      return(-1);
    }
  } /* next token */
}

static void loadLayerString(mapObj *map, layerObj *layer, char *value)
{
  int i=0,buffer_size=0;
  multipointObj points={0,NULL};
  int done=0, type;

  static featureListNodeObjPtr current=NULL;
  shapeObj shape;

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
    msFree(layer->classitem);
    layer->classitem = strdup(value);
    break;
  case(DATA):
    msFree(layer->data);
    layer->data = strdup(value);
    break;
  case(FEATURE):    
    if(layer->type == MS_LAYER_POLYGON)
      type = MS_SHAPE_POLYGON;
    else if(layer->type == MS_LAYER_LINE)
      type = MS_SHAPE_LINE;
    else
      type = MS_SHAPE_POINT;

    switch(msyylex()) {      
    case(POINTS):
      msyystate = 2; msyystring = value;

      if(layer->features == NULL) {
	msInitShape(&shape);
	shape.type = type;
	if((current = insertFeatureList(&(layer->features), &shape)) == NULL) return; /* create initial feature */
      }

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

      if(msAddLine(&(current->shape), &points) == -1) break;
      
      break;

    case(TEXT):
      current->shape.text = strdup(value);
      break;
    default:
      msInitShape(&shape);
      shape.type = type;
      if((current = insertFeatureList(&(layer->features), &shape)) == NULL) return;
      break;
    }

    break;
  case(FILTER):    
    loadExpressionString(&(layer->filter), value);
    break;
  case(FILTERITEM):
    msFree(layer->filteritem);
    layer->filteritem = strdup(value);
    break; 
  case(FOOTER):
    msFree(layer->footer);
    layer->footer = strdup(value);
    break;
  case(HEADER):      
    msFree(layer->header);
    layer->header = strdup(value);
    break;
  case(LABELANGLEITEM):
    msFree(layer->labelangleitem);
    if(strcasecmp(value, "null") == 0) 
      layer->labelangleitem = NULL;
    else
      layer->labelangleitem = strdup(value); 
    break;
  case(LABELCACHE):
    msyystate = 2; msyystring = value;
    if((layer->labelcache = getSymbol(2, MS_ON, MS_OFF)) == -1) return;
    break;
  case(LABELITEM):
    msFree(layer->labelitem);
    if(strcasecmp(value, "null") == 0) 
      layer->labelitem = NULL;
    else
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
  case(LABELREQUIRES):
    msFree(layer->labelrequires);
    layer->labelrequires = strdup(value);
    break;
  case(LABELSIZEITEM):
    msFree(layer->labelsizeitem);
    if(strcasecmp(value, "null") == 0) 
      layer->labelsizeitem = NULL;
    else
      layer->labelsizeitem = strdup(value);
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
    msLoadProjectionString(&(layer->projection), value);
    break;
  case(REQUIRES):
    msFree(layer->requires);
    layer->requires = strdup(value);
    break;
  case(SIZEUNITS):
    msyystate = 2; msyystring = value;
    if((layer->sizeunits = getSymbol(7, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD,MS_PIXELS)) == -1) return;
    break;
  case(STATUS): // enables turning a layer OFF
    msyystate = 2; msyystring = value;
    if((layer->status = getSymbol(3, MS_ON,MS_OFF,MS_DEFAULT)) == -1) return;
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
  case(STYLEITEM):
    msFree(layer->styleitem);
    layer->styleitem = strdup(value);
    break;
  case(SYMBOLSCALE):
    msyystate = 2; msyystring = value;
    if(getDouble(&(layer->symbolscale)) == -1) return;
    break;
  case(TEMPLATE):
    msFree(layer->template);
    layer->template = strdup(value);
    break;
  case(TOLERANCE):
    msyystate = 2; msyystring = value;
    if(getDouble(&(layer->tolerance)) == -1) return;
    break;
  case(TOLERANCEUNITS):
    msyystate = 2; msyystring = value;
    if((layer->toleranceunits = getSymbol(7, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD,MS_PIXELS)) == -1) return;
    break;
  case(TRANSFORM):
    msyystate = 2; msyystring = value;
    if((layer->transform = getSymbol(2, MS_TRUE,MS_FALSE)) == -1) return;
    break;
  case(TYPE):
    msyystate = 2; msyystring = value;
    if((layer->type = getSymbol(7, MS_LAYER_POINT,MS_LAYER_LINE,MS_LAYER_RASTER,MS_LAYER_POLYGON,MS_LAYER_ANNOTATION,MS_LAYER_QUERY,MS_LAYER_CIRCLE)) == -1) return;
    break;
  case(UNITS):
    msyystate = 2; msyystring = value;
    if((layer->units = getSymbol(7, MS_INCHES,MS_FEET,MS_MILES,MS_METERS,MS_KILOMETERS,MS_DD,MS_PIXELS)) == -1) return;
    break;
  default:
    break;
  }

  return;
}

static void writeLayer(mapObj *map, layerObj *layer, FILE *stream)
{
  int i;
  featureListNodeObjPtr current=NULL;

  if (layer->status == MS_DELETE)
      return;

  fprintf(stream, "  LAYER\n");  
  if(layer->classitem) fprintf(stream, "    CLASSITEM \"%s\"\n", layer->classitem);
  if(layer->connection) {
    fprintf(stream, "    CONNECTION \"%s\"\n", layer->connection);
    if(layer->connectiontype == MS_SDE)
      fprintf(stream, "    CONNECTIONTYPE SDE\n");
    else if(layer->connectiontype == MS_OGR)
      fprintf(stream, "    CONNECTIONTYPE OGR\n");
    else if(layer->connectiontype == MS_POSTGIS)
      fprintf(stream, "    CONNECTIONTYPE POSTGIS\n");
    else if(layer->connectiontype == MS_WMS)
      fprintf(stream, "    CONNECTIONTYPE WMS\n");
    else if(layer->connectiontype == MS_ORACLESPATIAL)
      fprintf(stream, "    CONNECTIONTYPE ORACLESPATIAL\n");
  }
  if(layer->data) fprintf(stream, "    DATA \"%s\"\n", layer->data);
  if(layer->dump) fprintf(stream, "    DUMP TRUE\n");

  if(layer->filter.string) {
    fprintf(stream, "      FILTER ");
    writeExpression(&(layer->filter), stream);
    fprintf(stream, "\n");
  }
  if(layer->filteritem) fprintf(stream, "    FILTERITEM \"%s\"\n", layer->filteritem);

  if(layer->footer) fprintf(stream, "    FOOTER \"%s\"\n", layer->footer);
  if(layer->group) fprintf(stream, "    GROUP \"%s\"\n", layer->group);
  if(layer->header) fprintf(stream, "    HEADER \"%s\"\n", layer->header);
  if(layer->labelangleitem) fprintf(stream, "    LABELANGLEITEM \"%s\"\n", layer->labelangleitem);
  if(!layer->labelcache) fprintf(stream, "    LABELCACHE OFF\n");
  if(layer->labelitem) fprintf(stream, "    LABELITEM \"%s\"\n", layer->labelitem);
  if(layer->labelmaxscale > -1) fprintf(stream, "    LABELMAXSCALE %g\n", layer->labelmaxscale);
  if(layer->labelminscale > -1) fprintf(stream, "    LABELMINSCALE %g\n", layer->labelminscale);
  if(layer->labelrequires) fprintf(stream, "    LABELREQUIRES \"%s\"\n", layer->labelrequires);
  if(layer->labelsizeitem) fprintf(stream, "    LABELSIZEITEM \"%s\"\n", layer->labelsizeitem);
  if(layer->maxfeatures > 0) fprintf(stream, "    MAXFEATURES %d\n", layer->maxfeatures);
  if(layer->maxscale > -1) fprintf(stream, "    MAXSCALE %g\n", layer->maxscale); 
  if(layer->metadata) writeHashTable(layer->metadata, stream, "      ", "METADATA");
  if(layer->minscale > -1) fprintf(stream, "    MINSCALE %g\n", layer->minscale);
  fprintf(stream, "    NAME \"%s\"\n", layer->name);
  if(layer->offsite > -1) fprintf(stream, "    OFFSITE %d\n", layer->offsite);
  if(layer->postlabelcache) fprintf(stream, "    POSTLABELCACHE TRUE\n");
  writeProjection(&(layer->projection), stream, "    ");
  if(layer->requires) fprintf(stream, "    REQUIRES \"%s\"\n", layer->requires);
  fprintf(stream, "    SIZEUNITS %s\n", msUnits[layer->sizeunits]);
  fprintf(stream, "    STATUS %s\n", msStatus[layer->status]);
  if(layer->styleitem) fprintf(stream, "    STYLEITEM \"%s\"\n", layer->styleitem);
  if(layer->symbolscale > -1) fprintf(stream, "    SYMBOLSCALE %g\n", layer->symbolscale);
  if(layer->template) fprintf(stream, "    TEMPLATE \"%s\"\n", layer->template);
  if(layer->tileindex) {
    fprintf(stream, "    TILEINDEX \"%s\"\n", layer->tileindex);
    if(layer->tileitem) fprintf(stream, "    TILEITEM \"%s\"\n", layer->tileitem);
  }
  fprintf(stream, "    TOLERANCE %g\n", layer->tolerance);
  fprintf(stream, "    TOLERANCEUNITS %s\n", msUnits[layer->toleranceunits]);
  if(!layer->transform) fprintf(stream, "    TRANSFORM FALSE\n");
  fprintf(stream, "    TYPE %s\n", msLayerTypes[layer->type]);
  fprintf(stream, "    UNITS %s\n", msUnits[layer->units]);

  // write potentially multiply occuring features last
  for(i=0; i<layer->numclasses; i++) writeClass(map, &(layer->class[i]), stream);

  current = layer->features;
  while(current != NULL) {
    writeFeature(&(current->shape), stream);
    current = current->next;
  }

  fprintf(stream, "  END\n\n");
}

/*
** Initialize, load and free a referenceMapObj structure
*/
void initReferenceMap(referenceMapObj *ref)
{
  ref->height = ref->width = 0;
  ref->extent.minx = ref->extent.miny = ref->extent.maxx = ref->extent.maxy = -1.0;
  ref->image = NULL;
  initColor(&(ref->color), 255, 0, 0);
  initColor(&(ref->outlinecolor), 0, 0, 0);  
  ref->status = MS_OFF;
  ref->marker = 0;
  ref->markername = NULL;
  ref->markersize = 0;
  ref->minboxsize = 3;
  ref->maxboxsize = 0;
}

void freeReferenceMap(referenceMapObj *ref)
{
  msFree(ref->image);
  msFree(ref->markername);
}

int loadReferenceMap(referenceMapObj *ref)
{
  int state;

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
      if(loadColor(&(ref->color)) != MS_SUCCESS) return(-1);
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
      if(loadColor(&(ref->outlinecolor)) != MS_SUCCESS) return(-1);
      break;
    case(SIZE):
      if(getInteger(&(ref->width)) == -1) return(-1);
      if(getInteger(&(ref->height)) == -1) return(-1);
      break;          
    case(STATUS):
      if((ref->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;
    case(MARKER):
      if((state = getSymbol(2, MS_NUMBER,MS_STRING)) == -1) return(-1);

      if(state == MS_NUMBER)
	ref->marker = (int) msyynumber;
      else
	ref->markername = strdup(msyytext);
      break;
    case(MARKERSIZE):
      if(getInteger(&(ref->markersize)) == -1) return(-1);
      break;
    case(MINBOXSIZE):
      if(getInteger(&(ref->minboxsize)) == -1) return(-1);
      break;
    case(MAXBOXSIZE):
      if(getInteger(&(ref->maxboxsize)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadReferenceMap()", 
                 msyytext, msyylineno);
      return(-1);
    }
  } /* next token */
}

static void loadReferenceMapString(mapObj *map, referenceMapObj *ref, char *value)
{
  int state;

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
    msFree(ref->image);
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
  case(MARKER):
    msyystate = 2; msyystring = value;
    if((state = getSymbol(2, MS_NUMBER,MS_STRING)) == -1) return;

    if(state == MS_NUMBER)
      ref->marker = (int) msyynumber;
    else {
      if((ref->marker = msGetSymbolIndex(&(map->symbolset), msyytext)) == -1)
	msSetError(MS_EOFERR, "Undefined symbol.", "loadClassString()");
    }
    break;
  case(MARKERSIZE):
    msyystate = 2; msyystring = value;
    getInteger(&(ref->markersize));
    break;
  case(MAXBOXSIZE):
    msyystate = 2; msyystring = value;
    getInteger(&(ref->maxboxsize));
    break;
  case(MINBOXSIZE):
    msyystate = 2; msyystring = value;
    getInteger(&(ref->minboxsize));
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
  fprintf(stream, "    IMAGE \"%s\"\n", ref->image);
  fprintf(stream, "    OUTLINECOLOR %d %d %d\n", ref->outlinecolor.red, ref->outlinecolor.green, ref->outlinecolor.blue);
  fprintf(stream, "    SIZE %d %d\n", ref->width, ref->height);
  fprintf(stream, "    STATUS %s\n", msStatus[ref->status]);
  if(ref->markername)
    fprintf(stream, "      MARKER \"%s\"\n", ref->markername);
  else
    fprintf(stream, "      MARKER %d\n", ref->marker);
  fprintf(stream, "      MARKERSIZE %d\n", ref->markersize);
  fprintf(stream, "      MINBOXSIZE %d\n", ref->minboxsize);
  fprintf(stream, "      MAXBOXSIZE %d\n", ref->maxboxsize);
  fprintf(stream, "  END\n\n");
}

#define MAX_FORMATOPTIONS 100

static int loadOutputFormat(mapObj *map)
{
  char *name = NULL;
  char *mimetype = NULL;
  char *driver = NULL;
  char *extension = NULL;
  int imagemode = MS_NOOVERRIDE;
  int transparent = MS_NOOVERRIDE;
  char *formatoptions[MAX_FORMATOPTIONS];
  int  numformatoptions = 0;
  char *value;

  for(;;) {
    switch(msyylex()) {
    case(EOF):
      msSetError(MS_EOFERR, NULL, "loadOutputFormat()");      
      return(-1);

    case(END):
    {
        outputFormatObj *format;

        if( driver == NULL )
        {
            msSetError(MS_MISCERR, 
                       "OUTPUTFORMAT clause lacks DRIVER keyword near (%s):(%d)",
                       "loadOutputFormat()",
                       msyytext, msyylineno );
            return -1;
        }
        format = msCreateDefaultOutputFormat( map, driver );
        if( format == NULL )
        {
            msSetError(MS_MISCERR, 
                       "OUTPUTFORMAT clause references driver %s, but this driver isn't configured.", 
                       "loadOutputFormat()", driver );
            return -1;
        }

        if( name != NULL )
        {
            msFree( format->name );
            format->name = name;
        }
        if( transparent != MS_NOOVERRIDE )
            format->transparent = transparent;
        if( extension != NULL )
        {
            msFree( format->extension );
            format->extension = extension;
        }
        if( mimetype != NULL )
        {
            msFree( format->mimetype );
            format->mimetype = mimetype;
        }
        if( format->imagemode != MS_NOOVERRIDE )
        {
            format->imagemode = imagemode;
            if( format->imagemode == MS_IMAGEMODE_INT16 
                || format->imagemode == MS_IMAGEMODE_FLOAT32 )
                format->renderer = MS_RENDER_WITH_RAWDATA;
        }

        format->numformatoptions = numformatoptions;
        if( numformatoptions > 0 )
        {
            format->formatoptions = (char **) 
                malloc(sizeof(char *)*numformatoptions );
            memcpy( format->formatoptions, formatoptions, 
                    sizeof(char *)*numformatoptions );
        }

        // We really needs some sort of generic and driver specific
        // validation at this point.

        return(0);
    }
    case(NAME):
      msFree( name );
      if((name = getToken()) == NULL) return(-1);
      break;
    case(MIMETYPE):
      msFree( mimetype );
      if((mimetype = getString()) == NULL) return(-1);
      break;
    case(DRIVER):
      msFree( driver );
      if((driver = getString()) == NULL) return(-1);
      break;
    case(FORMATOPTION):
      if((value = getString()) == NULL) return(-1);
      if( numformatoptions < MAX_FORMATOPTIONS )
          formatoptions[numformatoptions++] = value;
      break;
    case(IMAGEMODE):
      if((value = getString()) == NULL) return(-1);
      if( strcasecmp(value,"PC256") == 0 )
          imagemode = MS_IMAGEMODE_PC256;
      else if( strcasecmp(value,"RGB") == 0 )
          imagemode = MS_IMAGEMODE_RGB;
      else if( strcasecmp(value,"RGBA") == 0)
          imagemode = MS_IMAGEMODE_RGBA;
      else if( strcasecmp(value,"INT16") == 0)
          imagemode = MS_IMAGEMODE_INT16;
      else if( strcasecmp(value,"FLOAT32") == 0)
          imagemode = MS_IMAGEMODE_FLOAT32;
      else
      {
          msSetError(MS_IDENTERR, "(%s):(%d)", "loadOutputFormat()", 
                     msyytext, msyylineno);      
          return -1;
      }
      msFree( value );
      break;
    case(TRANSPARENT):
      if((transparent = getSymbol(2, MS_ON,MS_OFF)) == -1) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadOutputFormat()", 
                 msyytext, msyylineno);      
      return(-1);
    }
  } /* next token */
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
  legend->keysizex = 20;
  legend->keysizey = 10;
  legend->keyspacingx = 5;
  legend->keyspacingy = 5;
  legend->outlinecolor = -1; /* i,e. off */
  legend->status = MS_OFF;
  legend->transparent = MS_NOOVERRIDE;
  legend->interlace = MS_NOOVERRIDE;
  legend->position = MS_LL;
  legend->postlabelcache = MS_FALSE; // draw with labels
  legend->template = NULL;
}

void freeLegend(legendObj *legend)
{
  if (legend->template)
     free(legend->template);
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
      legend->label.position = MS_XY; /* overrides go here */
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
    case(TEMPLATE):
      if((legend->template = getString()) == NULL) return(-1);
      break;
    default:
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadLegend()", 
                 msyytext, msyylineno);      
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
  case(TEMPLATE):
    msFree(legend->template);
    legend->template = strdup(value);
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
  if( legend->interlace != MS_NOOVERRIDE )
      fprintf(stream, "    INTERLACE %s\n", msTrueFalse[legend->interlace]);
  fprintf(stream, "    KEYSIZE %d %d\n", legend->keysizex, legend->keysizey);
  fprintf(stream, "    KEYSPACING %d %d\n", legend->keyspacingx, legend->keyspacingy);
  writeLabel(map, &(legend->label), stream, "    ");
  if(legend->outlinecolor > -1) fprintf(stream, "    OUTLINECOLOR %d %d %d\n", map->palette.colors[legend->outlinecolor-1].red, map->palette.colors[legend->outlinecolor-1].green, map->palette.colors[legend->outlinecolor-1].blue);
  fprintf(stream, "    POSITION %s\n", msLabelPositions[legend->position]);
  if(legend->postlabelcache) fprintf(stream, "    POSTLABELCACHE TRUE\n");
  fprintf(stream, "    STATUS %s\n", msStatus[legend->status]);
  if( legend->transparent != MS_NOOVERRIDE )
     fprintf(stream, "    TRANSPARENT %s\n", msTrueFalse[legend->transparent]);
  if (legend->template) fprintf(stream, "    TEMPLATE \"%s\"\n", legend->template);
  fprintf(stream, "  END\n\n");
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
  scalebar->backgroundcolor = 0; /* image background */
  scalebar->color = 1; /* at least it won't dump core if not defined */
  scalebar->outlinecolor = -1; /* no outline */
  scalebar->units = MS_MILES;
  scalebar->status = MS_OFF;
  scalebar->position = MS_LL;
  scalebar->transparent = MS_NOOVERRIDE; /* no transparency */
  scalebar->interlace = MS_NOOVERRIDE;
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
      if(scalebar->color == -1) scalebar->color = msAddColor(map,0,0,0); /* default to black */
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
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadScalebar()",
                 msyytext, msyylineno);      
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
    scalebar->color = msAddColor(map,red,green,blue);
    if(scalebar->color == -1) scalebar->color = msAddColor(map,0,0,0); /* default to black */
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
  fprintf(stream, "  SCALEBAR\n");
  if(scalebar->backgroundcolor > -1) fprintf(stream, "    BACKGROUNDCOLOR %d %d %d\n", map->palette.colors[scalebar->backgroundcolor-1].red, map->palette.colors[scalebar->backgroundcolor-1].green, map->palette.colors[scalebar->backgroundcolor-1].blue);
  if(scalebar->color > -1) fprintf(stream, "    COLOR %d %d %d\n", map->palette.colors[scalebar->color-1].red, map->palette.colors[scalebar->color-1].green, map->palette.colors[scalebar->color-1].blue);
  fprintf(stream, "    IMAGECOLOR %d %d %d\n", scalebar->imagecolor.red, scalebar->imagecolor.green, scalebar->imagecolor.blue);
  if( scalebar->interlace != MS_NOOVERRIDE )
      fprintf(stream, "    INTERLACE %s\n", msTrueFalse[scalebar->interlace]);
  fprintf(stream, "    INTERVALS %d\n", scalebar->intervals);
  writeLabel(map, &(scalebar->label), stream, "    ");
  if(scalebar->outlinecolor > -1) fprintf(stream, "    OUTLINECOLOR %d %d %d\n", map->palette.colors[scalebar->outlinecolor-1].red, map->palette.colors[scalebar->outlinecolor-1].green, map->palette.colors[scalebar->outlinecolor-1].blue);
  fprintf(stream, "    POSITION %s\n", msLabelPositions[scalebar->position]);
  if(scalebar->postlabelcache) fprintf(stream, "    POSTLABELCACHE TRUE\n");
  fprintf(stream, "    SIZE %d %d\n", scalebar->width, scalebar->height);
  fprintf(stream, "    STATUS %s\n", msStatus[scalebar->status]);
  fprintf(stream, "    STYLE %d\n", scalebar->style);
  if( scalebar->transparent != MS_NOOVERRIDE )
      fprintf(stream, "    TRANSPARENT %s\n", 
              msTrueFalse[scalebar->transparent]);
  fprintf(stream, "    UNITS %s\n", msUnits[scalebar->units]);
  fprintf(stream, "  END\n\n");
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

static void loadQueryMapString(mapObj *map, queryMapObj *querymap, char *value)
{
  int red, green, blue;

  switch(msyylex()) {
  case(COLOR):
    msyystate = 2; msyystring = value;
    if(getInteger(&(red)) == -1) return;
    if(getInteger(&(green)) == -1) return;
    if(getInteger(&(blue)) == -1) return;
    querymap->color = msAddColor(map,red,green,blue);
    if(querymap->color == -1) querymap->color = msAddColor(map,255,255,0); /* default back to yellow */
    break;
  case(SIZE):
    msyystate = 2; msyystring = value;
    if(getInteger(&(querymap->width)) == -1) return;
    if(getInteger(&(querymap->height)) == -1) return;
    break;
  case(STATUS):
    msyystate = 2; msyystring = value;
    if((querymap->status = getSymbol(2, MS_ON,MS_OFF)) == -1) return;      
    break;
  case(STYLE):      
    msyystate = 2; msyystring = value;
    if((querymap->style = getSymbol(3, MS_NORMAL,MS_HILITE,MS_SELECTED)) == -1) return;
    break;      
  }

  return;
}

static void writeQueryMap(mapObj *map, queryMapObj *querymap, FILE *stream)
{
  fprintf(stream, "  QUERYMAP\n");
  if(querymap->color > -1) fprintf(stream, "    COLOR %d %d %d\n", map->palette.colors[querymap->color-1].red, map->palette.colors[querymap->color-1].green, map->palette.colors[querymap->color-1].blue);
  fprintf(stream, "    SIZE %d %d\n", querymap->width, querymap->height);
  fprintf(stream, "    STATUS %s\n", msStatus[querymap->status]);
  fprintf(stream, "    STYLE %s\n", msQueryMapStyles[querymap->style]);  
  fprintf(stream, "  END\n\n");
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
  web->metadata = NULL;
}

void freeWeb(webObj *web)
{
  msFree(web->template);
  msFree(web->header);
  msFree(web->footer);
  msFree(web->error);
  msFree(web->empty);
  msFree(web->maxtemplate);
  msFree(web->mintemplate);
  msFree(web->log);
  msFree(web->imagepath);
  msFree(web->imageurl);
  if(web->metadata) msFreeHashTable(web->metadata);
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
  if(web->maxscale > -1) fprintf(stream, "    MAXSCALE %g\n", web->maxscale);
  if(web->maxtemplate) fprintf(stream, "    MAXTEMPLATE \"%s\"\n", web->maxtemplate);
  if(web->metadata) writeHashTable(web->metadata, stream, "      ", "METADATA");
  if(web->minscale > -1) fprintf(stream, "    MINSCALE %g\n", web->minscale);
  if(web->mintemplate) fprintf(stream, "    MINTEMPLATE \"%s\"\n", web->mintemplate);
  if(web->template) fprintf(stream, "    TEMPLATE \"%s\"\n", web->template);
  fprintf(stream, "  END\n\n");
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
      free(web->imagepath); // there is a default
      if((web->imagepath = getString()) == NULL) return(-1);
      break;
    case(IMAGEURL):
      free(web->imageurl); // there is a default
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
    case(METADATA):
      if(loadHashTable(&(web->metadata)) != MS_SUCCESS) return(-1);
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
      msSetError(MS_IDENTERR, "(%s):(%d)", "loadWeb()", msyytext, msyylineno);
      return(-1);
    }
  }
}

static void loadWebString(webObj *web, char *value)
{
  switch(msyylex()) {
  case(EMPTY):
    msFree(web->empty);
    web->empty = strdup(value);
    break;
  case(ERROR):
    msFree(web->error);
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
    msFree(web->footer);
    web->footer = strdup(value);	
    break;
  case(HEADER):
    msFree(web->header);
    web->header = strdup(value);
    break;
  case(IMAGEPATH):
    msFree(web->imagepath);
    web->imagepath = strdup(value);
    break;
  case(IMAGEURL):
    msFree(web->imageurl);
    web->imageurl = strdup(value);
    break;
  case(MAXSCALE):
    msyystate = 2; msyystring = value;
    if(getDouble(&web->maxscale) == -1) return;
    break;
  case(MAXTEMPLATE):
    msFree(web->maxtemplate);
    web->maxtemplate = strdup(value);
    break;
  case(MINSCALE):
    msyystate = 2; msyystring = value;
    if(getDouble(&web->minscale) == -1) return;
    break;
  case(MINTEMPLATE):
    msFree(web->mintemplate);
    web->mintemplate = strdup(value);
    break;
  case(TEMPLATE):
    msFree(web->template);
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
  map->resolution = 72; // pixels per inch
 
  map->height = map->width = -1;

  map->units = MS_METERS;
  map->cellsize = 0;
  map->shapepath = NULL;
  map->imagecolor.red = 255;
  map->imagecolor.green = 255;
  map->imagecolor.blue = 255;

  map->numoutputformats = 0;
  map->outputformatlist = NULL;
  map->outputformat = NULL;

  map->imagetype = NULL;

  map->palette.numcolors = 0;

  map->transparent = MS_NOOVERRIDE;
  map->interlace = MS_NOOVERRIDE;
  map->imagequality = MS_NOOVERRIDE;

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
 
  map->fontset.filename = NULL;
  map->fontset.numfonts = 0;  
  map->fontset.fonts = NULL;

  msInitSymbolSet(&map->symbolset);
  map->symbolset.fontset =  &(map->fontset);

  initLegend(&map->legend);
  initScalebar(&map->scalebar);
  initWeb(&map->web);
  initReferenceMap(&map->reference);
  initQueryMap(&map->querymap);

#ifdef USE_PROJ
  if(msInitProjection(&(map->projection)) == -1)
    return(-1);
  if(msInitProjection(&(map->latlon)) == -1)
    return(-1);

  // initialize a default "geographic" projection
  map->latlon.numargs = 2;
  map->latlon.args[0] = strdup("proj=latlong");
  map->latlon.args[1] = strdup("ellps=clrk66"); // probably want a different ellipsoid
  if(msProcessProjection(&(map->latlon)) == -1) return(-1);
#endif

  //Initialize the layer order list (used to modify the order in 
  //which the layers are drawn).
  map->layerorder = (int *)malloc(sizeof(int)*MS_MAXLAYERS);


  return(0);
}

void msFreeMap(mapObj *map) {
  int i;

  if(!map) return;

  msFree(map->name);
  msFree(map->shapepath);

  msFreeSymbolSet(&map->symbolset); // free symbols
  msFree(map->symbolset.filename);

  msFreeProjection(&(map->projection));
  msFreeProjection(&(map->latlon));

  for(i=0; i<map->labelcache.numlabels; i++) {
    msFree(map->labelcache.labels[i].string);
    msFreeShape(map->labelcache.labels[i].poly);
    msFree(map->labelcache.labels[i].poly);
    freeClass(&(map->labelcache.labels[i].class));
  }
  msFree(map->labelcache.labels);

  if( map->outputformat && --map->outputformat->refcount < 1 )
      msFreeOutputFormat( map->outputformat );

  for(i=0; i < map->numoutputformats; i++ ) {
      msFreeOutputFormat( map->outputformatlist[i] );
  }
  if( map->outputformatlist != NULL )
      msFree( map->outputformatlist );

  msFree( map->imagetype );

  for(i=0; i<map->labelcache.nummarkers; i++) {
    msFreeShape(map->labelcache.markers[i].poly);
    msFree(map->labelcache.markers[i].poly);
  }
  msFree(map->labelcache.markers);

  if(map->fontset.fonts) msFreeHashTable(map->fontset.fonts);
  msFree(map->fontset.filename);

  freeWeb(&(map->web));

  freeScalebar(&(map->scalebar));
  freeReferenceMap(&(map->reference));
  freeLegend(&(map->legend));  

  for(i=0; i<map->numlayers; i++)
    freeLayer(&(map->layers[i]));
  msFree(map->layers);

  if (map->layerorder)
      free(map->layerorder);

  msFree(map);
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
    msSetError(MS_IOERR, "(%s)", "msSaveMap()", filename);    
    return(-1);
  }

  fprintf(stream, "MAP\n");
  fprintf(stream, "  EXTENT %g %g %g %g\n", map->extent.minx, map->extent.miny, map->extent.maxx, map->extent.maxy);
  if(map->fontset.filename) fprintf(stream, "  FONTSET \"%s\"\n", map->fontset.filename);
  fprintf(stream, "  IMAGECOLOR %d %d %d\n", map->imagecolor.red, map->imagecolor.green, map->imagecolor.blue);

  if( map->imagetype != NULL )
      fprintf(stream, "  IMAGETYPE %s\n", map->imagetype );

  if(map->resolution != 72) fprintf(stream, "  RESOLUTION %d\n", map->resolution);

  if( map->interlace != MS_NOOVERRIDE )
      fprintf(stream, "  INTERLACE %s\n", msTrueFalse[map->interlace]);
  if(map->symbolset.filename) fprintf(stream, "  SYMBOLSET \"%s\"\n", map->symbolset.filename);
  if(map->shapepath) fprintf(stream, "  SHAPEPATH \"%s\"\n", map->shapepath);
  fprintf(stream, "  SIZE %d %d\n", map->width, map->height);
  fprintf(stream, "  STATUS %s\n", msStatus[map->status]);
  if( map->transparent != MS_NOOVERRIDE )
      fprintf(stream, "  TRANSPARENT %s\n", msTrueFalse[map->transparent]);

  fprintf(stream, "  UNITS %s\n", msUnits[map->units]);
  fprintf(stream, "  NAME \"%s\"\n\n", map->name);

  writeProjection(&(map->projection), stream, "  ");
  
  writeLegend(map, &(map->legend), stream);
  writeQueryMap(map, &(map->querymap), stream);
  writeReferenceMap(&(map->reference), stream);
  writeScalebar(map, &(map->scalebar), stream);
  writeWeb(&(map->web), stream);

  for(i=0; i<map->numlayers; i++)
  {
      writeLayer(map, &(map->layers[map->layerorder[i]]), stream);
      //writeLayer(map, &(map->layers[i]), stream);
  }

  //once the map is saved the layer order should be reset
  for(i=0; i<map->numlayers; i++)
  {
      map->layerorder[i] = i;
  }


  fprintf(stream, "END\n");

  fclose(stream);

  return(0);
}

static mapObj *loadMapInternal(char *filename, char *new_map_path)
{
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
   msSetError(MS_REGEXERR, "(%s)", "msLoadMap()", MS_MAPFILE_EXPR);   
   return(NULL);
  }
  if(regexec(&re, filename, 0, NULL, 0) != 0) { /* no match */
    regfree(&re);
    msSetError(MS_IOERR, "Illegal mapfile name.", "msLoadMap()");
    return(NULL);
  }
  regfree(&re);
  
  if((msyyin = fopen(filename,"r")) == NULL) {
    msSetError(MS_IOERR, "(%s)", "msLoadMap()", filename);
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

  // If new_map_path is provided then use it, otherwise use the location
  // of the mapfile as the default path
  if (new_map_path)
      map_path = strdup(new_map_path);
  else
      map_path = getPath(filename);
  /* switch so all filenames are relative to the location of the map file */
  if (chdir(map_path) != 0)
  {
    msSetError(MS_IOERR, "chdir(%s) failed.", "msLoadMap()", map_path);
    free(map_path);
    free(map);
    return(NULL);
  } 
  free(map_path);

  msyystate = 5; // restore lexer state to INITIAL
  msyylex();
  msyyrestart(msyyin);
  msyylineno = 0;

  if(initMap(map) == -1) /* initialize this map */
    return(NULL);

  for(;;) {

    switch(msyylex()) {   
    case(END):
      fclose(msyyin);      

      /*** OUTPUTFORMAT related setup ***/
      if( msPostMapParseOutputFormatSetup( map ) == MS_FAILURE )
          return NULL;

      if(msLoadSymbolSet(&(map->symbolset)) == -1) return(NULL);

      /* step through layers and classes to resolve symbol names */
       for(i=0; i<map->numlayers; i++) 
       {
          for(j=0; j<map->layers[i].numclasses; j++)
          {
             if (map->layers[i].class[j].overlaysymbolname)
                if ((map->layers[i].class[j].overlaysymbol =  msGetSymbolIndex(&(map->symbolset), map->layers[i].class[j].overlaysymbolname)) == -1) {
                    msSetError(MS_MISCERR, "Undefined overlay symbol \"%s\" in class %d of layer %s.", 
			       "msLoadMap()",
                               map->layers[i].class[j].overlaysymbolname,
                               j, map->layers[i].name);

                    return(NULL);
                 }
             
             if (map->layers[i].class[j].symbolname)
                if ((map->layers[i].class[j].symbol = msGetSymbolIndex(&(map->symbolset), map->layers[i].class[j].symbolname)) == -1)
                 {
                    msSetError(MS_MISCERR,
                               "Undefined symbol \"%s\" in class %d of layer %s.",
                               "msLoadMap()",
                               map->layers[i].class[j].symbolname,
                               j, map->layers[i].name);
                    
                   return (NULL);
                 }
             
          }
       }

#if defined (USE_GD_TTF) || defined (USE_GD_FT)
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
    case(IMAGEQUALITY):
      if(getInteger(&(map->imagequality)) == -1) return(NULL);
      break;
    case(IMAGETYPE):
      map->imagetype = getToken();
      break;
    case(INTERLACE):
      if((map->interlace = getSymbol(2, MS_ON,MS_OFF)) == -1) return(NULL);
      break;
    case(LATLON):
      msFreeProjection(&map->latlon);
      if(loadProjection(&map->latlon) == -1) return(NULL);
      break;
    case(LAYER):
      if(map->numlayers == MS_MAXLAYERS) { 
	msSetError(MS_IDENTERR, "Too many layers defined.", "msLoadMap()");
	return(NULL);
      }
      if(loadLayer(&(map->layers[map->numlayers]), map) == -1) return(NULL);
      map->layers[map->numlayers].index = map->numlayers; /* save the index */
      //Update the layer order list with the layer's index.
      map->layerorder[map->numlayers] = map->numlayers;
      map->numlayers++;
      break;
    case(OUTPUTFORMAT):
      if(loadOutputFormat(map) == -1) return(NULL);
      break;
    case(LEGEND):
      if(loadLegend(&(map->legend), map) == -1) return(NULL);
      break;
    case(MAP):
      break;   
    case(NAME):
      msFree(map->name); /* erase default */
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
    case(RESOLUTION):
      if(getInteger(&(map->resolution)) == -1) return(NULL);
      break;
    case(SCALE):
      if(getDouble(&(map->scale)) == -1) return(NULL);
      break;
    case(SCALEBAR):
      if(loadScalebar(&(map->scalebar), map) == -1) return(NULL);
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
    case(SYMBOL):
      if(map->symbolset.numsymbols == MS_MAXSYMBOLS) { 
	msSetError(MS_SYMERR, "Too many symbols defined.", "msLoadMap()");
	return(NULL);
      }
      if((loadSymbol(&(map->symbolset.symbol[map->symbolset.numsymbols])) == -1)) return(NULL);
      map->symbolset.numsymbols++;
      break;
    case(SYMBOLSET):
      if((map->symbolset.filename = getString()) == NULL) return(NULL);
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
      msSetError(MS_IDENTERR, "(%s):(%d)", "msLoadMap()", 
                 msyytext, msyylineno);
      return(NULL);
    }
  } /* next token */
}

//
// Wraps loadMapInternal
//
mapObj *msLoadMap(char *filename, char *new_map_path)
{
    mapObj *map;

    msAcquireLock( TLOCK_PARSER );
    map = loadMapInternal( filename, new_map_path );
    msReleaseLock( TLOCK_PARSER );

    return map;
}

/*
** READ FROM STRINGS INSTEAD OF FILE - ASSUMES A MAP FILE HAS ALREADY BEEN LOADED
*/
int msLoadMapString(mapObj *map, char *object, char *value)
{
  int i;
  errorObj *ms_error;

  msyystate = 1; /* set lexer state */
  msyystring = object;

  ms_error = msGetErrorObj();
  ms_error->code = MS_NOERR; /* init error code */

  switch(msyylex()) {
  case(MAP):
    switch(msyylex()) {
    case(EXTENT):
      msyystate = 2; msyystring = value;
      if(getDouble(&(map->extent.minx)) == -1) break;
      if(getDouble(&(map->extent.miny)) == -1) break;
      if(getDouble(&(map->extent.maxx)) == -1) break;
      if(getDouble(&(map->extent.maxy)) == -1) break;
      break;
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
    case(IMAGEQUALITY):
      msyystate = 2; msyystring = value;
      if(getInteger(&(map->imagequality)) == -1) break;
      break;
    case(IMAGETYPE):
      msyystate = 2; msyystring = value;
      map->imagetype = getToken();
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
      msLoadProjectionString(&(map->projection), value);
      break;
    case(QUERYMAP):
      loadQueryMapString(map, &(map->querymap), value);
      break;
    case(REFERENCE):
      loadReferenceMapString(map, &(map->reference), value);
      break;
    case(RESOLUTION):
      msyystate = 2; msyystring = value;
      if(getInteger(&(map->resolution)) == -1) break;      
      break;
    case(SCALEBAR):
      loadScalebarString(map, &(map->scalebar), value);
      break;
    case(SIZE):
      msyystate = 2; msyystring = value;
      if(getInteger(&(map->width)) == -1) break;
      if(getInteger(&(map->height)) == -1) break;

      if(map->width > MS_MAXIMGSIZE || map->height > MS_MAXIMGSIZE || map->width < 0 || map->height < 0) {
	msSetError(MS_WEBERR, "Image size out of range.", "msLoadMapString()");
	break;
      }

      break;
    case(SHAPEPATH):
      msFree(map->shapepath);
      map->shapepath = strdup(value);
      break;
    case(STATUS):
      msyystate = 2; msyystring = value;
      if((map->status = getSymbol(2, MS_ON,MS_OFF)) == -1) break;
      break;
    case(MS_STRING):
      i = msGetLayerIndex(map, msyytext);
      if(i>=map->numlayers || i<0) break;
      loadLayerString(map, &(map->layers[i]), value);
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

  if(ms_error->code != MS_NOERR) return(-1);

  return(0);
}

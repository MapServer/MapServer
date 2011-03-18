%{
/*
** READ ME FIRST!
**
** When this file is altered, it is necessary to do "make lexer".  Due to
** problems detailed in #2310 the lexer is no longer automatically rebuilt
** when maplexer.l is altered.
*/


/* C declarations */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <time.h>

#include "mapserver.h"
#include "maperror.h"
#include "mapfile.h"
#include "maptime.h"
#include "mapsymbol.h"
#include "mapparser.h"
#include "mapprimitive.h"

/* msyylineno is required for flex 2.5.4 and older, but is already defined by
 * flex 2.5.31 (bug 975).
 * Unfortunately there is no clean way to differenciate the two versions,
 * so we use the symbol YY_CURRENT_BUFFER_LVALUE to base our test since it
 * was not present in 2.5.4 and is present in 2.5.31. Hopefully that won't
 * put us in trouble with other versions. If that happens then we can
 * switch to using autoconf to detect the version.
 */
#ifndef YY_CURRENT_BUFFER_LVALUE
int msyylineno = 1;
#endif

int msyysource=MS_STRING_TOKENS;
double msyynumber;
int msyystate=MS_TOKENIZE_DEFAULT;
char *msyystring=NULL;
char *msyybasepath=NULL;
char *msyystring_buffer_ptr;
int  msyystring_buffer_size = 256;
int  msyystring_size;
char msyystring_begin;
char *msyystring_buffer = NULL;
int  msyystring_icase = MS_FALSE;
int  msyystring_state;

int msyyreturncomments = 0;

#define MS_LEXER_STRING_REALLOC(string, string_size, max_size, string_ptr)   \
   if (string_size >= max_size) {         \
       max_size = ((max_size*2) > string_size) ? max_size*2 : string_size+1;                     \
       string = (char *) msSmallRealloc(string, sizeof(char *) * max_size);  \
       string_ptr = string;    \
       string_ptr += (max_size/2); \
   }

#define MAX_INCLUDE_DEPTH 5
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
int include_lineno[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;
char path[MS_MAXPATHLEN];

%}

%s URL_VARIABLE
%s URL_STRING
%s EXPRESSION_STRING
%s INCLUDE
%s MSSTRING

%%
       if (msyystring_buffer == NULL)
           msyystring_buffer = (char*) msSmallMalloc(sizeof(char) * msyystring_buffer_size);

       switch(msyystate) {
       case(MS_TOKENIZE_DEFAULT):
         break;
       case(MS_TOKENIZE_FILE):
         BEGIN(INITIAL);
         msyysource=MS_FILE_TOKENS;
         msyystate=MS_TOKENIZE_DEFAULT;
         msyystring=NULL;
         msyyreturncomments=0;
         include_stack_ptr=0;
         return(0);
         break;
       case(MS_TOKENIZE_STRING):
         BEGIN(INITIAL);
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyy_scan_string(msyystring);
         msyysource=MS_STRING_TOKENS;
         msyystate=MS_TOKENIZE_DEFAULT;
         msyyin=NULL;
         msyyreturncomments=0;
         include_stack_ptr=0;
         return(0);
         break;
       case(MS_TOKENIZE_URL_VARIABLE):
         BEGIN(URL_VARIABLE);
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyy_scan_string(msyystring);
         msyysource=MS_URL_TOKENS;
         msyystate=MS_TOKENIZE_DEFAULT;
         msyyreturncomments=0; 
         (void) yyunput; /* just to avoid warning about it being unrefed */
         break;
       case(MS_TOKENIZE_URL_STRING):
         BEGIN(URL_STRING);
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyy_scan_string(msyystring);
         msyysource=MS_URL_TOKENS;
         msyystate=MS_TOKENIZE_DEFAULT;
         msyyin=NULL;
         msyyreturncomments=0; 
         return(0);
         break;
       case(MS_TOKENIZE_EXPRESSION):
         BEGIN(EXPRESSION_STRING);
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyy_scan_string(msyystring);
         msyystate=MS_TOKENIZE_DEFAULT;
         msyyreturncomments=0;
         break;
       case(99):
         BEGIN(INITIAL); /* may not need this one */
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyystate=MS_TOKENIZE_DEFAULT;
         msyystring=NULL;
         msyyreturncomments=0; 
         return(0);
         break;
       default:
         break;
       }

       /* Used to get the proper error messages */ 
       MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                               msyystring_buffer_size, msyystring_buffer_ptr);
       strcpy(msyystring_buffer, msyytext); 

<INITIAL,URL_STRING,EXPRESSION_STRING,INCLUDE>[ \t\r]+             ;

<INITIAL>#.*                                   { if (msyyreturncomments) return(MS_COMMENT); }

<URL_VARIABLE>_|\.                             ;

<EXPRESSION_STRING>or|\|\|                     { return(MS_TOKEN_LOGICAL_OR); }
<EXPRESSION_STRING>and|&&                      { return(MS_TOKEN_LOGICAL_AND); }
<EXPRESSION_STRING>not|!                       { return(MS_TOKEN_LOGICAL_NOT); }
<EXPRESSION_STRING>eq|=|==                     { return(MS_TOKEN_COMPARISON_EQ); }
<EXPRESSION_STRING>ne|!=                       { return(MS_TOKEN_COMPARISON_NE); }
<EXPRESSION_STRING>gt|>                        { return(MS_TOKEN_COMPARISON_GT); }
<EXPRESSION_STRING>lt|<                        { return(MS_TOKEN_COMPARISON_LT); }
<EXPRESSION_STRING>ge|>=                       { return(MS_TOKEN_COMPARISON_GE); }
<EXPRESSION_STRING>le|<=                       { return(MS_TOKEN_COMPARISON_LE); }
<EXPRESSION_STRING>~                           { return(MS_TOKEN_COMPARISON_RE); }

<EXPRESSION_STRING>=\*                         { return(MS_TOKEN_COMPARISON_IEQ); }
<EXPRESSION_STRING>~\*                         { return(MS_TOKEN_COMPARISON_IRE); }

<EXPRESSION_STRING>in                          { return(IN); }

<EXPRESSION_STRING>area                        { return(MS_TOKEN_FUNCTION_AREA); }
<EXPRESSION_STRING>length                      { return(MS_TOKEN_FUNCTION_LENGTH); }
<EXPRESSION_STRING>tostring                    { return(MS_TOKEN_FUNCTION_TOSTRING); }
<EXPRESSION_STRING>commify                     { return(MS_TOKEN_FUNCTION_COMMIFY); }
<EXPRESSION_STRING>round                       { return(MS_TOKEN_FUNCTION_ROUND); }

<EXPRESSION_STRING>buffer                      { return(MS_TOKEN_FUNCTION_BUFFER); }

<EXPRESSION_STRING>intersects                  { return(MS_TOKEN_COMPARISON_INTERSECTS); }
<EXPRESSION_STRING>disjoint                    { return(MS_TOKEN_COMPARISON_DISJOINT); }
<EXPRESSION_STRING>touches                     { return(MS_TOKEN_COMPARISON_TOUCHES); }
<EXPRESSION_STRING>overlaps                    { return(MS_TOKEN_COMPARISON_OVERLAPS); }
<EXPRESSION_STRING>crosses                     { return(MS_TOKEN_COMPARISON_CROSSES); }
<EXPRESSION_STRING>within                      { return(MS_TOKEN_COMPARISON_WITHIN); }
<EXPRESSION_STRING>contains                    { return(MS_TOKEN_COMPARISON_CONTAINS); }
<EXPRESSION_STRING>beyond                      { return(MS_TOKEN_COMPARISON_BEYOND); }
<EXPRESSION_STRING>dwithin                     { return(MS_TOKEN_COMPARISON_DWITHIN); }

<EXPRESSION_STRING>fromtext                    { return(MS_TOKEN_FUNCTION_FROMTEXT); }

<INITIAL>colorrange                            { return(COLORRANGE); }
<INITIAL>datarange                             { return(DATARANGE); }
<INITIAL>rangeitem                             { return(RANGEITEM); }

<INITIAL,URL_STRING>align                       { return(ALIGN); }
<INITIAL,URL_VARIABLE,URL_STRING>angle         { return(ANGLE); }
<INITIAL,URL_STRING>antialias                  { return(ANTIALIAS); }
<INITIAL,URL_STRING>backgroundcolor            { return(BACKGROUNDCOLOR); }
<INITIAL>bandsitem                             { return(BANDSITEM); }
<INITIAL>bindvals                              { return(BINDVALS); }
<INITIAL>browseformat                          { return(BROWSEFORMAT); }
<INITIAL>buffer                                { return(BUFFER); }
<INITIAL>character                             { return(CHARACTER); }
<INITIAL,URL_VARIABLE>class                    { return(CLASS); }
<INITIAL,URL_STRING>classitem                  { return(CLASSITEM); }
<INITIAL,URL_STRING>classgroup                 { return(CLASSGROUP); }
<INITIAL>cluster                               { return(CLUSTER); }
<INITIAL,URL_STRING>color                      { return(COLOR); }
<INITIAL>config                                { return(CONFIG); }
<INITIAL,URL_STRING>connection                 { return(CONNECTION); }
<INITIAL,URL_STRING>connectiontype             { return(CONNECTIONTYPE); }
<INITIAL,URL_STRING>data                       { return(DATA); }
<INITIAL>datapattern                           { return(DATAPATTERN); }
<INITIAL>debug                                 { return(DEBUG); }
<INITIAL>driver                                { return(DRIVER); }
<INITIAL>dump                                  { return(DUMP); }
<INITIAL>empty                                 { return(EMPTY); }
<INITIAL>encoding                              { return(ENCODING); }
<INITIAL,URL_STRING>end                        { return(END); }
<INITIAL>error                                 { return(ERROR); }
<INITIAL,URL_STRING>expression                 { return(EXPRESSION); }
<INITIAL,URL_VARIABLE,URL_STRING>extent        { return(EXTENT); }
<INITIAL>extension                             { return(EXTENSION); }
<INITIAL,URL_STRING>feature                    { return(FEATURE); }
<INITIAL>filled                                { return(FILLED); }
<INITIAL,URL_STRING>filter                     { return(FILTER); }
<INITIAL,URL_STRING>filteritem                 { return(FILTERITEM); }
<INITIAL,URL_STRING>footer                     { return(FOOTER); }
<INITIAL,URL_STRING>font                       { return(FONT); }
<INITIAL>fontset                               { return(FONTSET); }
<INITIAL>force                                 { return(FORCE); }
<INITIAL>formatoption                          { return(FORMATOPTION); }
<INITIAL>from                                  { return(FROM); }
<INITIAL>gap                                   { return(GAP); }
<INITIAL>geomtransform                         { return(GEOMTRANSFORM); }
<INITIAL>grid                                  { return(GRID); }
<INITIAL>graticule                             { return(GRATICULE); }
<INITIAL,URL_STRING>group                      { return(GROUP); }
<INITIAL,URL_STRING>header                     { return(HEADER); }
<INITIAL>image                                 { return(IMAGE); }
<INITIAL,URL_VARIABLE,URL_STRING>imagecolor    { return(IMAGECOLOR); }
<INITIAL,URL_VARIABLE>imagetype                { return(IMAGETYPE); }
<INITIAL>imagequality                          { return(IMAGEQUALITY); }
<INITIAL>imagemode                             { return(IMAGEMODE); }
<INITIAL>imagepath                             { return(IMAGEPATH); }
<INITIAL>temppath                              { return(TEMPPATH); }
<INITIAL>imageurl                              { return(IMAGEURL); }
<INITIAL>include                               { BEGIN(INCLUDE); }
<INITIAL>index                                 { return(INDEX); }
<INITIAL>interlace                             { return(INTERLACE); }
<INITIAL,URL_STRING>intervals                  { return(INTERVALS); } 
<INITIAL>join                                  { return(JOIN); }
<INITIAL,URL_STRING>keyimage                   { return(KEYIMAGE); }
<INITIAL,URL_STRING>keysize                    { return(KEYSIZE); }
<INITIAL>keyspacing                            { return(KEYSPACING); }
<INITIAL,URL_STRING>label                      { return(LABEL); }
<INITIAL>labelcache                            { return(LABELCACHE); }
<INITIAL>labelformat                           { return(LABELFORMAT); }
<INITIAL,URL_STRING>labelitem                  { return(LABELITEM); }
<INITIAL>labelmaxscale                         { return(LABELMAXSCALE); }
<INITIAL>labelmaxscaledenom                    { return(LABELMAXSCALEDENOM); }
<INITIAL>labelminscale                         { return(LABELMINSCALE); }
<INITIAL>labelminscaledenom                    { return(LABELMINSCALEDENOM); }
<INITIAL,URL_STRING>labelrequires              { return(LABELREQUIRES); }
<INITIAL>latlon                                { return(LATLON); }
<INITIAL,URL_VARIABLE>layer                    { return(LAYER); }
<INITIAL,URL_VARIABLE>legend                   { return(LEGEND); }
<INITIAL>legendformat                          { return(LEGENDFORMAT); }
<INITIAL>linecap                               { return(LINECAP); }
<INITIAL>linejoin                              { return(LINEJOIN); }
<INITIAL>linejoinmaxsize                       { return(LINEJOINMAXSIZE); }
<INITIAL>log                                   { return(LOG); }
<INITIAL,URL_VARIABLE>map                      { return(MAP); }
<INITIAL>marker                                { return(MARKER); }
<INITIAL>markersize                            { return(MARKERSIZE); }
<INITIAL>maxarcs                               { return(MAXARCS); }
<INITIAL>maxboxsize                            { return(MAXBOXSIZE); }
<INITIAL>maxdistance                           { return(MAXDISTANCE); }
<INITIAL>maxfeatures                           { return(MAXFEATURES); }
<INITIAL>maxinterval                           { return(MAXINTERVAL); }
<INITIAL,URL_STRING>maxscale                   { return(MAXSCALE); }
<INITIAL,URL_STRING>maxscaledenom              { return(MAXSCALEDENOM); }
<INITIAL>maxgeowidth                           { return(MAXGEOWIDTH); }
<INITIAL>maxlength                             { return(MAXLENGTH); }
<INITIAL>maxsize                               { return(MAXSIZE); }
<INITIAL>maxsubdivide                          { return(MAXSUBDIVIDE); }
<INITIAL>maxtemplate                           { return(MAXTEMPLATE); }
<INITIAL>maxwidth                              { return(MAXWIDTH); }
<INITIAL>metadata                              { return(METADATA); }
<INITIAL>mimetype                              { return(MIMETYPE); }
<INITIAL>minarcs                               { return(MINARCS); }
<INITIAL>minboxsize                            { return(MINBOXSIZE); }
<INITIAL>mindistance                           { return(MINDISTANCE); }
<INITIAL>repeatdistance                        { return(REPEATDISTANCE); }
<INITIAL>maxoverlapangle                       { return(MAXOVERLAPANGLE); } 
<INITIAL>minfeaturesize                        { return(MINFEATURESIZE); }
<INITIAL>mininterval                           { return(MININTERVAL); }
<INITIAL,URL_STRING>minscale                   { return(MINSCALE); }
<INITIAL,URL_STRING>minscaledenom              { return(MINSCALEDENOM); }
<INITIAL>mingeowidth                           { return(MINGEOWIDTH); }
<INITIAL>minlength                             { return(MINLENGTH); }
<INITIAL>minsize                               { return(MINSIZE); }
<INITIAL>minsubdivide                          { return(MINSUBDIVIDE); }
<INITIAL>mintemplate                           { return(MINTEMPLATE); }
<INITIAL>minwidth                              { return(MINWIDTH); }
<INITIAL>name                                  { return(NAME); }
<INITIAL>offset                                { return(OFFSET); }
<INITIAL>offsite                               { return(OFFSITE); }
<INITIAL,URL_STRING>opacity                    { return(OPACITY); }
<INITIAL,URL_STRING>outlinecolor               { return(OUTLINECOLOR); }
<INITIAL,URL_STRING>outlinewidth               { return(OUTLINEWIDTH); }
<INITIAL>outputformat                          { return(OUTPUTFORMAT); }
<INITIAL,URL_STRING>overlaybackgroundcolor     { return(OVERLAYBACKGROUNDCOLOR); }
<INITIAL,URL_STRING>overlaycolor               { return(OVERLAYCOLOR); }
<INITIAL>overlaymaxsize                        { return(OVERLAYMAXSIZE); }
<INITIAL>overlayminsize                        { return(OVERLAYMINSIZE); }
<INITIAL,URL_STRING>overlayoutlinecolor        { return(OVERLAYOUTLINECOLOR); }
<INITIAL,URL_STRING>overlaysize                { return(OVERLAYSIZE); }
<INITIAL,URL_STRING>overlaysymbol              { return(OVERLAYSYMBOL); }
<INITIAL>partials                              { return(PARTIALS); }
<INITIAL>pattern                               { return(PATTERN); }
<INITIAL,URL_STRING>points                     { return(POINTS); }
<INITIAL>items                                 { return(ITEMS); }
<INITIAL,URL_STRING>position                   { return(POSITION); }
<INITIAL>postlabelcache                        { return(POSTLABELCACHE); }
<INITIAL>priority                              { return(PRIORITY); }
<INITIAL,URL_STRING>processing                 { return(PROCESSING); }
<INITIAL,URL_VARIABLE,URL_STRING>projection    { return(PROJECTION); }
<INITIAL>queryformat                           { return(QUERYFORMAT); }
<INITIAL,URL_VARIABLE>querymap                 { return(QUERYMAP); }
<INITIAL,URL_VARIABLE>reference                { return(REFERENCE); }
<INITIAL>region                                { return(REGION); }
<INITIAL>relativeto                            { return(RELATIVETO); }
<INITIAL,URL_STRING>requires                   { return(REQUIRES); }
<INITIAL,URL_VARIABLE>resolution               { return(RESOLUTION); }
<INITIAL,URL_VARIABLE>defresolution            { return(DEFRESOLUTION); }
<INITIAL>scale                                 { return(SCALE); }
<INITIAL>scaledenom                            { return(SCALEDENOM); }
<INITIAL,URL_VARIABLE>scalebar                 { return(SCALEBAR); }
<INITIAL,URL_STRING>shadowcolor                { return(SHADOWCOLOR); }
<INITIAL,URL_STRING>shadowsize                 { return(SHADOWSIZE); }
<INITIAL>shapepath                             { return(SHAPEPATH); }
<INITIAL,URL_VARIABLE,URL_STRING>size          { return(SIZE); }
<INITIAL>sizeunits                             { return(SIZEUNITS); }
<INITIAL,URL_STRING>status                     { return(STATUS); }
<INITIAL,URL_VARIABLE>style                    { return(STYLE); }
<INITIAL>styleitem                             { return(STYLEITEM); }
<INITIAL,URL_STRING>symbol                     { return(SYMBOL); }
<INITIAL>symbolscale                           { return(SYMBOLSCALE); }
<INITIAL>symbolscaledenom                      { return(SYMBOLSCALEDENOM); }
<INITIAL>symbolset                             { return(SYMBOLSET); }
<INITIAL>table                                 { return(TABLE); }
<INITIAL,URL_STRING>template                   { return(TEMPLATE); }
<INITIAL>templatepattern                       { return(TEMPLATEPATTERN); }
<INITIAL,URL_STRING>text                       { return(TEXT); }
<INITIAL,URL_STRING>tileindex                  { return(TILEINDEX); }
<INITIAL,URL_STRING>tileitem                   { return(TILEITEM); }
<INITIAL,URL_STRING>title                      { return(TITLE); }
<INITIAL>to                                    { return(TO); }
<INITIAL,URL_STRING>tolerance                  { return(TOLERANCE); }
<INITIAL,URL_STRING>toleranceunits             { return(TOLERANCEUNITS); }
<INITIAL>transparency                          { return(TRANSPARENCY); }
<INITIAL,URL_VARIABLE>transparent              { return(TRANSPARENT); }
<INITIAL>transform                             { return(TRANSFORM); }
<INITIAL>type                                  { return(TYPE); }
<INITIAL,URL_VARIABLE,URL_STRING>units         { return(UNITS); }
<INITIAL>validation                            { return(VALIDATION); }
<INITIAL,URL_VARIABLE>web                      { return(WEB); }
<INITIAL,URL_STRING>width                      { return(WIDTH); }
<INITIAL,URL_STRING>wkt                        { return(WKT); }
<INITIAL>wrap                                  { return(WRAP); }

<INITIAL,URL_STRING>annotation                 { return(MS_LAYER_ANNOTATION); }
<INITIAL,URL_STRING>auto                       { return(MS_AUTO); }
<INITIAL,URL_STRING>auto2                      { return(MS_AUTO2); }
<INITIAL>bevel                                 { return(MS_CJC_BEVEL); }
<INITIAL>bitmap                                { return(MS_BITMAP); }
<INITIAL>butt                                  { return(MS_CJC_BUTT); }
<INITIAL>cartoline                             { return(MS_SYMBOL_CARTOLINE); }
<INITIAL,URL_STRING>cc                         { return(MS_CC); }
<INITIAL,URL_STRING>center                     { return(MS_ALIGN_CENTER); }
<INITIAL>chart                                 { return(MS_LAYER_CHART); }
<INITIAL>circle                                { return(MS_LAYER_CIRCLE); }
<INITIAL,URL_STRING>cl                         { return(MS_CL); }
<INITIAL,URL_STRING>cr                         { return(MS_CR); }
<INITIAL>csv                                   { return(MS_DB_CSV); }
<INITIAL>postgresql                            { return(MS_DB_POSTGRES); }
<INITIAL,URL_STRING>default                    { return(MS_DEFAULT); }
<INITIAL,URL_STRING>dd                         {
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext); 
                                                 return(MS_DD); 
                                               }
<INITIAL>ellipse                               { return(MS_SYMBOL_ELLIPSE); }
<INITIAL,URL_STRING>embed                      { return(MS_EMBED); }
<INITIAL,URL_STRING>false                      { return(MS_FALSE); }
<INITIAL,URL_STRING>feet                       { return(MS_FEET); }
<INITIAL,URL_STRING>follow                     { return(MS_FOLLOW); }
<INITIAL>giant                                 { return(MS_GIANT); }
<INITIAL>hatch                                 { return(MS_SYMBOL_HATCH); }
<INITIAL>hilite                                { return(MS_HILITE); }
<INITIAL,URL_STRING>inches                     { return(MS_INCHES); }
<INITIAL,URL_STRING>kilometers                 { return(MS_KILOMETERS); }
<INITIAL>large                                 { return(MS_LARGE); }
<INITIAL,URL_STRING>lc                         { return(MS_LC); }
<INITIAL,URL_STRING>left                       { return(MS_ALIGN_LEFT); }
<INITIAL>line                                  { return(MS_LAYER_LINE); }
<INITIAL,URL_STRING>ll                         { return(MS_LL); }
<INITIAL,URL_STRING>lr                         { return(MS_LR); }
<INITIAL>medium                                { return(MS_MEDIUM); }
<INITIAL,URL_STRING>meters                     { return(MS_METERS); }
<INITIAL,URL_STRING>nauticalmiles              { return(MS_NAUTICALMILES); }
<INITIAL,URL_STRING>miles                      { return(MS_MILES); }
<INITIAL>miter                                 { return(MS_CJC_MITER); }
<INITIAL>multiple                              { return(MS_MULTIPLE); }
<INITIAL>none                                  { return(MS_CJC_NONE); }
<INITIAL>normal                                { return(MS_NORMAL); }
<INITIAL,URL_STRING>off                        { return(MS_OFF); }
<INITIAL>ogr                                   { return(MS_OGR); }
<INITIAL,URL_STRING>on                         { return(MS_ON); }
<INITIAL>one-to-one                            { return(MS_JOIN_ONE_TO_ONE); }
<INITIAL>one-to-many                           { return(MS_JOIN_ONE_TO_MANY); }
<INITIAL>oraclespatial                         { return(MS_ORACLESPATIAL); }
<INITIAL>percentages                           { return(MS_PERCENTAGES); }
<INITIAL>pixmap                                { return(MS_SYMBOL_PIXMAP); }
<INITIAL,URL_STRING>pixels                     { return(MS_PIXELS); }
<INITIAL>point                                 { return(MS_LAYER_POINT); }
<INITIAL>polygon                               { return(MS_LAYER_POLYGON); }
<INITIAL>postgis                               { return(MS_POSTGIS); }
<INITIAL>plugin                                { return(MS_PLUGIN); }
<INITIAL>query                                 { return(MS_LAYER_QUERY); }
<INITIAL>raster                                { return(MS_LAYER_RASTER); }
<INITIAL,URL_STRING>right                      { return(MS_ALIGN_RIGHT); }
<INITIAL>round                                 { return(MS_CJC_ROUND); }
<INITIAL>sde                                   { return(MS_SDE); }
<INITIAL>selected                              { return(MS_SELECTED); }
<INITIAL>simple                                { return(MS_SYMBOL_SIMPLE); }
<INITIAL>single                                { return(MS_SINGLE); }
<INITIAL>small                                 { return(MS_SMALL); }
<INITIAL>square                                { return(MS_CJC_SQUARE); }
<INITIAL>svg                                   { return(MS_SYMBOL_SVG); }
<INITIAL>tiny                                  { return(MS_TINY); }
<INITIAL>triangle                              { return(MS_CJC_TRIANGLE); }
<INITIAL,URL_STRING>true                       { return(MS_TRUE); }
<INITIAL>truetype                              { return(MS_TRUETYPE); }
<INITIAL,URL_STRING>uc                         { return(MS_UC); }
<INITIAL,URL_STRING>ul                         { return(MS_UL); }
<INITIAL,URL_STRING>ur                         { return(MS_UR); }
<INITIAL>union                                 { return(MS_UNION); }
<INITIAL>vector                                { return(MS_SYMBOL_VECTOR); }
<INITIAL>wfs                                   { return(MS_WFS); }
<INITIAL>wms                                   { return(MS_WMS); }
<INITIAL>alpha                                 { return(MS_GD_ALPHA); }

<URL_VARIABLE>\[[a-z/\.][a-z0-9/\.\-\=_ ]*\]   {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer,msyytext);
                                                 return(MS_STRING);
                                               }
<URL_VARIABLE>\[[0-9]*\]                       {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer,msyytext);                                                 
                                                 msyynumber = atof(msyytext);
                                                 return(MS_NUMBER);
                                               }

<INITIAL>\[[^\]]*\]                            {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer,msyytext);
                                                 return(MS_BINDING);
                                               }

<EXPRESSION_STRING>\[shape\] { 
  /* attribute binding - shape (fixed value) */
  return(MS_TOKEN_BINDING_SHAPE);
}
<EXPRESSION_STRING>\[[^\]]*\] {
  /* attribute binding - numeric (no quotes) */
  msyytext++;
  msyytext[strlen(msyytext)-1] = '\0';
  MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                          msyystring_buffer_size, msyystring_buffer_ptr);
  strcpy(msyystring_buffer, msyytext);
  return(MS_TOKEN_BINDING_DOUBLE);
}
<EXPRESSION_STRING>\"\[[^\"]*\]\"|\'\[[^\']*\]\' {
  /* attribute binding - string (single or double quotes) */
  msyytext+=2;
  msyytext[strlen(msyytext)-2] = '\0';
  MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                          msyystring_buffer_size, msyystring_buffer_ptr);
  strcpy(msyystring_buffer, msyytext);
  return(MS_TOKEN_BINDING_STRING);
}

<INITIAL,URL_STRING>-?[0-9]+|-?[0-9]+\.[0-9]*|-?\.[0-9]*|-?[0-9]+[eE][+-]?[0-9]+|-?[0-9]+\.[0-9]*[eE][+-]?[0-9]+|-?\.[0-9]*[eE][+-]?[0-9]+ {
  MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                          msyystring_buffer_size, msyystring_buffer_ptr);
  strcpy(msyystring_buffer,msyytext);
  msyynumber = atof(msyytext);
  return(MS_NUMBER); 
}

<EXPRESSION_STRING>-?[0-9]+|-?[0-9]+\.[0-9]*|-?\.[0-9]*|-?[0-9]+[eE][+-]?[0-9]+|-?[0-9]+\.[0-9]*[eE][+-]?[0-9]+|-?\.[0-9]*[eE][+-]?[0-9]+ {
  MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                          msyystring_buffer_size, msyystring_buffer_ptr);
  strcpy(msyystring_buffer,msyytext);
  msyynumber = atof(msyytext);
  return(MS_TOKEN_LITERAL_NUMBER);
}

<EXPRESSION_STRING>\`[^\`]*\` {
  msyytext++;
  msyytext[strlen(msyytext)-1] = '\0';
  MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                          msyystring_buffer_size, msyystring_buffer_ptr);
  strcpy(msyystring_buffer, msyytext);
  return(MS_TOKEN_LITERAL_TIME);
}

<INITIAL,URL_STRING>\/[^\/]*\/i                {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-2] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext);
                                                 return(MS_IREGEX);
                                               }

<INITIAL,URL_STRING>\/[^\/]*\/                 {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext);
                                                 return(MS_REGEX);
                                               }

<INITIAL,URL_STRING>\(.*\)                     {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext);
                                                 return(MS_EXPRESSION);
                                               }

<INITIAL,URL_STRING>\'|\"                     {
                                                 msyystring_state = MS_STRING;
                                                 msyystring_begin = msyytext[0]; 
                                                 msyystring_size = 0;
                                                 msyystring_buffer_ptr = msyystring_buffer;
                                                 BEGIN(MSSTRING);
                                              }

<MSSTRING>\'|\"|\"i|\'i                       {
                                                MS_LEXER_STRING_REALLOC(msyystring_buffer, msyystring_size, 
                                                                                           msyystring_buffer_size, msyystring_buffer_ptr);
                                                if (msyystring_begin == msyytext[0]) {
                                                   if (msyystring_state == MS_TOKEN_LITERAL_STRING)
                                                       BEGIN(EXPRESSION_STRING);
                                                   else
                                                       BEGIN(INITIAL);

                                                   *msyystring_buffer_ptr = '\0';

                                                   if (msyystring_state == MS_STRING) {
                                                      if (msyystring_icase && strlen(msyytext)==2) {
                                                         msyystring_icase = MS_FALSE; // reset
                                                         return MS_ISTRING;
                                                      } else
                                                        return MS_STRING;
                                                   }
                                                   return msyystring_state;

                                                }
                                                else {
                                                  ++msyystring_size;
                                                  *msyystring_buffer_ptr++ = *msyytext;
                                                  if (strlen(msyytext)==2) {
                                                     ++msyystring_size;
                                                     *msyystring_buffer_ptr++ = msyytext[1];
                                                  }
                                                }
                                              }

<MSSTRING>\\\'|\\\"                          { 
                                                MS_LEXER_STRING_REALLOC(msyystring_buffer, msyystring_size, 
                                                                                           msyystring_buffer_size, msyystring_buffer_ptr);

                                                ++msyystring_size;
                                                *msyystring_buffer_ptr++ = msyytext[1]; 
                                             }

<MSSTRING>[^\\\'\\\"]+                       {
                                                 char *yptr = msyytext;
                                                 while ( *yptr ) { 
                                                   MS_LEXER_STRING_REALLOC(msyystring_buffer, msyystring_size, 
                                                                           msyystring_buffer_size, msyystring_buffer_ptr);
                                                   ++msyystring_size;
                                                   *msyystring_buffer_ptr++ = *yptr++;
                                                 }
                                             }

<INCLUDE>\"[^\"]*\"|\'[^\']*\'                 {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';

                                                 if(include_stack_ptr >= MAX_INCLUDE_DEPTH) {
                                                   msSetError(MS_IOERR, "Includes nested to deeply.", "msyylex()");
                                                   return(-1);
                                                 }

                                                 include_stack[include_stack_ptr] = YY_CURRENT_BUFFER; /* save state */
                                                 include_lineno[include_stack_ptr] = msyylineno;
                                                 include_stack_ptr++;

                                                 msyyin = fopen(msBuildPath(path, msyybasepath, msyytext), "r");
                                                 if(!msyyin) {
                                                   msSetError(MS_IOERR, "Error opening included file \"%s\".", "msyylex()", msyytext);
                                                   return(-1);
                                                 }

                                                 msyy_switch_to_buffer( msyy_create_buffer(msyyin, YY_BUF_SIZE) );
                                                 msyylineno = 1;

                                                 BEGIN(INITIAL);
                                               }

<EXPRESSION_STRING>\'|\"                      {
                                                 msyystring_state = MS_TOKEN_LITERAL_STRING;
                                                 msyystring_begin = msyytext[0]; 
                                                 msyystring_size = 0;
                                                 msyystring_buffer_ptr = msyystring_buffer;
                                                 BEGIN(MSSTRING);
                                              }

<INITIAL,URL_STRING>[a-z/\.][a-z0-9/\._\-\=]*   { 
                                                    MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                            msyystring_buffer_size, msyystring_buffer_ptr);
                                                    strcpy(msyystring_buffer, msyytext); 
                                                    return(MS_STRING); 
                                                }

<INITIAL>\n                                     { msyylineno++; }

<INITIAL><<EOF>>                                {
                                                  if( --include_stack_ptr < 0 )
                                                    return(EOF); /* end of main file */
                                                  else {
                                                    fclose(YY_CURRENT_BUFFER->yy_input_file);
                                                    msyy_delete_buffer( YY_CURRENT_BUFFER );
                                                    msyy_switch_to_buffer(include_stack[include_stack_ptr]);
                                                    msyylineno = include_lineno[include_stack_ptr];
                                                  }
                                                }

<EXPRESSION_STRING,URL_VARIABLE,URL_STRING>[\r|\n|\0] {
  return(0); 
}

<INITIAL,URL_VARIABLE,URL_STRING>.              { 
                                                  MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                          msyystring_buffer_size, msyystring_buffer_ptr);
                                                  strcpy(msyystring_buffer, msyytext); 
                                                  return(0); 
                                                }
<EXPRESSION_STRING>.                            { return(msyytext[0]); }
%%

/*
** Any extra C functions
*/

int msyywrap() /* override */
{
  return(1);
}

int msyyerror(char *s) {
  msSetError(MS_PARSEERR, s, "msyyparse()");
  return(0);
}

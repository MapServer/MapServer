%option never-interactive

%{
/*
** READ ME FIRST!
**
** When this file is altered, it is necessary to do "make lexer".  Due to
** problems detailed in #2310 the lexer is no longer automatically rebuilt
** when maplexer.l is altered.
*/

#define YY_NO_INPUT

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

#define YY_NO_INPUT

int msyysource=MS_STRING_TOKENS;
double msyynumber;
int msyystate=MS_TOKENIZE_DEFAULT;
char *msyystring=NULL;
char *msyybasepath=NULL;
char *msyystring_buffer_ptr;
int  msyystring_buffer_size = 0;
int  msyystring_size;
char msyystring_begin;
char *msyystring_buffer = NULL;
int  msyystring_icase = MS_FALSE;
int  msyystring_return_state;
int  msyystring_begin_state;
int  msyystring_size_tmp;

int msyyreturncomments = 0;

#define MS_LEXER_STRING_REALLOC(string, string_size, max_size, string_ptr)   \
  do { \
   const int string_size_macro = (int)(string_size); \
   if (string_size_macro >= (int)(max_size)) {         \
       msyystring_size_tmp = (max_size);     \
       max_size = (((int)(max_size)*2) > string_size_macro) ? ((int)(max_size))*2 : string_size_macro+1;   \
       string = (char *) msSmallRealloc(string, sizeof(char *) * (max_size));  \
       string_ptr = string;    \
       string_ptr += msyystring_size_tmp; \
   } \
  } while(0)

#define MS_LEXER_RETURN_TOKEN(token) \
   MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext),  \
                           msyystring_buffer_size, msyystring_buffer_ptr); \
   strcpy(msyystring_buffer, msyytext); \
   return(token); 

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
%s MULTILINE_COMMENT
%s CONFIG_FILE

%%
       if (msyystring_buffer == NULL)
       {
           msyystring_buffer_size = 256;
           msyystring_buffer = (char*) msSmallMalloc(sizeof(char) * msyystring_buffer_size);
       }

       msyystring_buffer[0] = '\0';
       switch(msyystate) {
       case(MS_TOKENIZE_DEFAULT):
         break;
       case(MS_TOKENIZE_FILE):
         BEGIN(INITIAL);
         msyystring_begin_state = INITIAL;
         msyysource=MS_FILE_TOKENS;
         msyystate=MS_TOKENIZE_DEFAULT;
         msyystring=NULL;
         msyyreturncomments=0;
         include_stack_ptr=0;
         return(0);
         break;
       case(MS_TOKENIZE_STRING):
         BEGIN(INITIAL);
         msyystring_begin_state = INITIAL;
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
         msyystring_begin_state = URL_VARIABLE;
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyy_scan_string(msyystring);
         msyysource=MS_URL_TOKENS;
         msyystate=MS_TOKENIZE_DEFAULT;
         msyyreturncomments=0;
         (void) yyunput; /* just to avoid warning about it being unrefed */
         break;
       case(MS_TOKENIZE_URL_STRING):
         BEGIN(URL_STRING);
         msyystring_begin_state = URL_STRING;
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
         msyystring_begin_state = EXPRESSION_STRING;
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyy_scan_string(msyystring);
         msyystate=MS_TOKENIZE_DEFAULT;
         msyyreturncomments=0;
         break;
       case(MS_TOKENIZE_CONFIG):
         BEGIN(CONFIG_FILE);
         msyystring_begin_state = CONFIG_FILE;
         msyysource=MS_FILE_TOKENS;
         msyystate=MS_TOKENIZE_DEFAULT;
         msyystring=NULL;
         msyyreturncomments=0;
         return(0);
         break;
       case(99):
         BEGIN(INITIAL); /* may not need this one */
         msyystring_begin_state = INITIAL;
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyystate=MS_TOKENIZE_DEFAULT;
         msyystring=NULL;
         msyyreturncomments=0; 
         return(0);
         break;
       default:
         break;
       }

<INITIAL,URL_STRING,EXPRESSION_STRING,INCLUDE,CONFIG_FILE>[ \t\r]+             ;

<INITIAL,CONFIG_FILE>(\/\/|#).*             { if (msyyreturncomments) return(MS_COMMENT); }

<INITIAL>\/\*                                  { BEGIN(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>\*\/                        { BEGIN(INITIAL); }
<MULTILINE_COMMENT>[^*\n]+                     ;
<MULTILINE_COMMENT>\*                          ;
<MULTILINE_COMMENT>\n                          { msyylineno++; }

<URL_VARIABLE>_|\.                             ;

<CONFIG_FILE>config                            { MS_LEXER_RETURN_TOKEN(MS_CONFIG_SECTION); }
<CONFIG_FILE>env                               { MS_LEXER_RETURN_TOKEN(MS_CONFIG_SECTION_ENV); }
<CONFIG_FILE>maps                              { MS_LEXER_RETURN_TOKEN(MS_CONFIG_SECTION_MAPS); }
<CONFIG_FILE>plugins                           { MS_LEXER_RETURN_TOKEN(MS_CONFIG_SECTION_PLUGINS) }

<EXPRESSION_STRING>or|\|\|                     { MS_LEXER_RETURN_TOKEN(MS_TOKEN_LOGICAL_OR); }
<EXPRESSION_STRING>and|&&                      { MS_LEXER_RETURN_TOKEN(MS_TOKEN_LOGICAL_AND); }
<EXPRESSION_STRING>not|!                       { MS_LEXER_RETURN_TOKEN(MS_TOKEN_LOGICAL_NOT); }
<EXPRESSION_STRING>eq|=|==                     { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_EQ); }
<EXPRESSION_STRING>ne|!=                       { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_NE); }
<EXPRESSION_STRING>gt|>                        { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_GT); }
<EXPRESSION_STRING>lt|<                        { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_LT); }
<EXPRESSION_STRING>ge|>=                       { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_GE); }
<EXPRESSION_STRING>le|<=                       { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_LE); }
<EXPRESSION_STRING>~                           { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_RE); }

<EXPRESSION_STRING>=\*                         { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_IEQ); }
<EXPRESSION_STRING>~\*                         { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_IRE); }

<EXPRESSION_STRING>in                          { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_IN); /* was IN */ }

<EXPRESSION_STRING>area                        { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_AREA); }
<EXPRESSION_STRING>length                      { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_LENGTH); }
<EXPRESSION_STRING>tostring                    { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_TOSTRING); }
<EXPRESSION_STRING>commify                     { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_COMMIFY); }
<EXPRESSION_STRING>round                       { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_ROUND); }
<EXPRESSION_STRING>upper                       { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_UPPER); }
<EXPRESSION_STRING>lower                       { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_LOWER); }
<EXPRESSION_STRING>initcap                     { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_INITCAP); }
<EXPRESSION_STRING>firstcap                    { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_FIRSTCAP); }

<EXPRESSION_STRING>buffer                      { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_BUFFER); }
<EXPRESSION_STRING>difference                  { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_DIFFERENCE); }
<EXPRESSION_STRING>simplify                    { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_SIMPLIFY); }
<EXPRESSION_STRING>simplifypt                  { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_SIMPLIFYPT); }
<EXPRESSION_STRING>generalize                  { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_GENERALIZE); }
<EXPRESSION_STRING>smoothsia                   { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_SMOOTHSIA); }
<EXPRESSION_STRING>centerline                  { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_CENTERLINE); }
<EXPRESSION_STRING>densify                     { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_DENSIFY); }
<EXPRESSION_STRING>outer                       { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_OUTER); }
<EXPRESSION_STRING>inner                       { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_INNER); } 
<EXPRESSION_STRING>javascript                  { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_JAVASCRIPT); }

<EXPRESSION_STRING>intersects                  { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_INTERSECTS); }
<EXPRESSION_STRING>disjoint                    { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_DISJOINT); }
<EXPRESSION_STRING>touches                     { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_TOUCHES); }
<EXPRESSION_STRING>overlaps                    { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_OVERLAPS); }
<EXPRESSION_STRING>crosses                     { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_CROSSES); }
<EXPRESSION_STRING>within                      { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_WITHIN); }
<EXPRESSION_STRING>contains                    { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_CONTAINS); }
<EXPRESSION_STRING>equals                      { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_EQUALS); }
<EXPRESSION_STRING>beyond                      { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_BEYOND); }
<EXPRESSION_STRING>dwithin                     { MS_LEXER_RETURN_TOKEN(MS_TOKEN_COMPARISON_DWITHIN); }

<EXPRESSION_STRING>fromtext                    { MS_LEXER_RETURN_TOKEN(MS_TOKEN_FUNCTION_FROMTEXT); }

<EXPRESSION_STRING>true                        { msyynumber=MS_TRUE; return(MS_TOKEN_LITERAL_BOOLEAN); }
<EXPRESSION_STRING>false                       { msyynumber=MS_FALSE; return(MS_TOKEN_LITERAL_BOOLEAN); }

<INITIAL>colorrange                            { MS_LEXER_RETURN_TOKEN(COLORRANGE); }
<INITIAL>datarange                             { MS_LEXER_RETURN_TOKEN(DATARANGE); }
<INITIAL>rangeitem                             { MS_LEXER_RETURN_TOKEN(RANGEITEM); }

<INITIAL,URL_STRING>align                      { MS_LEXER_RETURN_TOKEN(ALIGN); }
<INITIAL>anchorpoint                           { MS_LEXER_RETURN_TOKEN(ANCHORPOINT); }
<INITIAL,URL_VARIABLE,URL_STRING>angle         { MS_LEXER_RETURN_TOKEN(ANGLE); }
<INITIAL,URL_STRING>antialias                  { MS_LEXER_RETURN_TOKEN(ANTIALIAS); }
<INITIAL,URL_STRING>backgroundcolor            { MS_LEXER_RETURN_TOKEN(BACKGROUNDCOLOR); }
<INITIAL>bandsitem                             { MS_LEXER_RETURN_TOKEN(BANDSITEM); }
<INITIAL>bindvals                              { MS_LEXER_RETURN_TOKEN(BINDVALS); }
<INITIAL>\xEF\xBB\xBF                          { MS_LEXER_RETURN_TOKEN(BOM); }
<INITIAL>browseformat                          { MS_LEXER_RETURN_TOKEN(BROWSEFORMAT); }
<INITIAL>buffer                                { MS_LEXER_RETURN_TOKEN(BUFFER); }
<INITIAL>character                             { MS_LEXER_RETURN_TOKEN(CHARACTER); }
<INITIAL,URL_VARIABLE>class                    { MS_LEXER_RETURN_TOKEN(CLASS); }
<INITIAL,URL_STRING>classitem                  { MS_LEXER_RETURN_TOKEN(CLASSITEM); }
<INITIAL,URL_STRING>classgroup                 { MS_LEXER_RETURN_TOKEN(CLASSGROUP); }
<INITIAL>cluster                               { MS_LEXER_RETURN_TOKEN(CLUSTER); }
<INITIAL,URL_STRING>color                      { MS_LEXER_RETURN_TOKEN(COLOR); }
<INITIAL>compfilter                            { MS_LEXER_RETURN_TOKEN(COMPFILTER); }
<INITIAL>composite                             { MS_LEXER_RETURN_TOKEN(COMPOSITE); }
<INITIAL>compop                                { MS_LEXER_RETURN_TOKEN(COMPOP); }
<INITIAL>config                                { MS_LEXER_RETURN_TOKEN(CONFIG); }
<INITIAL,URL_STRING>connection                 { MS_LEXER_RETURN_TOKEN(CONNECTION); }
<INITIAL,URL_STRING>connectiontype             { MS_LEXER_RETURN_TOKEN(CONNECTIONTYPE); }
<INITIAL,URL_STRING>data                       { MS_LEXER_RETURN_TOKEN(DATA); }
<INITIAL>datapattern                           { MS_LEXER_RETURN_TOKEN(DATAPATTERN); }
<INITIAL>debug                                 { MS_LEXER_RETURN_TOKEN(DEBUG); }
<INITIAL>driver                                { MS_LEXER_RETURN_TOKEN(DRIVER); }
<INITIAL>empty                                 { MS_LEXER_RETURN_TOKEN(EMPTY); }
<INITIAL>encoding                              { MS_LEXER_RETURN_TOKEN(ENCODING); }
<INITIAL,URL_STRING,CONFIG_FILE>end           { MS_LEXER_RETURN_TOKEN(END); }
<INITIAL>error                                 { MS_LEXER_RETURN_TOKEN(ERROR); }
<INITIAL,URL_STRING>expression                 { MS_LEXER_RETURN_TOKEN(EXPRESSION); }
<INITIAL,URL_VARIABLE,URL_STRING>extent        { MS_LEXER_RETURN_TOKEN(EXTENT); }
<INITIAL>extension                             { MS_LEXER_RETURN_TOKEN(EXTENSION); }
<INITIAL,URL_STRING>feature                    { MS_LEXER_RETURN_TOKEN(FEATURE); }
<INITIAL>filled                                { MS_LEXER_RETURN_TOKEN(FILLED); }
<INITIAL,URL_STRING>filter                     { MS_LEXER_RETURN_TOKEN(FILTER); }
<INITIAL,URL_STRING>filteritem                 { MS_LEXER_RETURN_TOKEN(FILTERITEM); }
<INITIAL,URL_STRING>footer                     { MS_LEXER_RETURN_TOKEN(FOOTER); }
<INITIAL,URL_STRING>font                       { MS_LEXER_RETURN_TOKEN(FONT); }
<INITIAL>fontset                               { MS_LEXER_RETURN_TOKEN(FONTSET); }
<INITIAL>force                                 { MS_LEXER_RETURN_TOKEN(FORCE); }
<INITIAL>formatoption                          { MS_LEXER_RETURN_TOKEN(FORMATOPTION); }
<INITIAL>from                                  { MS_LEXER_RETURN_TOKEN(FROM); }
<INITIAL,URL_STRING>gap                        { MS_LEXER_RETURN_TOKEN(GAP); }
<INITIAL>geomtransform                         { MS_LEXER_RETURN_TOKEN(GEOMTRANSFORM); }
<INITIAL>grid                                  { MS_LEXER_RETURN_TOKEN(GRID); }
<INITIAL>gridstep                              { MS_LEXER_RETURN_TOKEN(GRIDSTEP); }
<INITIAL>graticule                             { MS_LEXER_RETURN_TOKEN(GRATICULE); }
<INITIAL,URL_STRING>group                      { MS_LEXER_RETURN_TOKEN(GROUP); }
<INITIAL,URL_STRING>header                     { MS_LEXER_RETURN_TOKEN(HEADER); }
<INITIAL>image                                 { MS_LEXER_RETURN_TOKEN(IMAGE); }
<INITIAL,URL_VARIABLE,URL_STRING>imagecolor    { MS_LEXER_RETURN_TOKEN(IMAGECOLOR); }
<INITIAL,URL_VARIABLE>imagetype                { MS_LEXER_RETURN_TOKEN(IMAGETYPE); }
<INITIAL>imagemode                             { MS_LEXER_RETURN_TOKEN(IMAGEMODE); }
<INITIAL>imagepath                             { MS_LEXER_RETURN_TOKEN(IMAGEPATH); }
<INITIAL>temppath                              { MS_LEXER_RETURN_TOKEN(TEMPPATH); }
<INITIAL>imageurl                              { MS_LEXER_RETURN_TOKEN(IMAGEURL); }
<INITIAL>include                               { BEGIN(INCLUDE); }
<INITIAL>index                                 { MS_LEXER_RETURN_TOKEN(INDEX); }
<INITIAL,URL_STRING>initialgap                 { MS_LEXER_RETURN_TOKEN(INITIALGAP); }
<INITIAL,URL_STRING>intervals                  { MS_LEXER_RETURN_TOKEN(INTERVALS); } 
<INITIAL>join                                  { MS_LEXER_RETURN_TOKEN(JOIN); }
<INITIAL,URL_STRING>keyimage                   { MS_LEXER_RETURN_TOKEN(KEYIMAGE); }
<INITIAL,URL_STRING>keysize                    { MS_LEXER_RETURN_TOKEN(KEYSIZE); }
<INITIAL>keyspacing                            { MS_LEXER_RETURN_TOKEN(KEYSPACING); }
<INITIAL,URL_VARIABLE>label                    { MS_LEXER_RETURN_TOKEN(LABEL); }
<INITIAL>labelcache                            { MS_LEXER_RETURN_TOKEN(LABELCACHE); }
<INITIAL>labelformat                           { MS_LEXER_RETURN_TOKEN(LABELFORMAT); }
<INITIAL,URL_STRING>labelitem                  { MS_LEXER_RETURN_TOKEN(LABELITEM); }
<INITIAL>labelmaxscale                         { MS_LEXER_RETURN_TOKEN(LABELMAXSCALE); }
<INITIAL>labelmaxscaledenom                    { MS_LEXER_RETURN_TOKEN(LABELMAXSCALEDENOM); }
<INITIAL>labelminscale                         { MS_LEXER_RETURN_TOKEN(LABELMINSCALE); }
<INITIAL>labelminscaledenom                    { MS_LEXER_RETURN_TOKEN(LABELMINSCALEDENOM); }
<INITIAL,URL_STRING>labelrequires              { MS_LEXER_RETURN_TOKEN(LABELREQUIRES); }
<INITIAL>latlon                                { MS_LEXER_RETURN_TOKEN(LATLON); }
<INITIAL,URL_VARIABLE>layer                    { MS_LEXER_RETURN_TOKEN(LAYER); }
<INITIAL>leader                                { MS_LEXER_RETURN_TOKEN(LEADER); }
<INITIAL,URL_VARIABLE>legend                   { MS_LEXER_RETURN_TOKEN(LEGEND); }
<INITIAL>legendformat                          { MS_LEXER_RETURN_TOKEN(LEGENDFORMAT); }
<INITIAL>linecap                               { MS_LEXER_RETURN_TOKEN(LINECAP); }
<INITIAL>linejoin                              { MS_LEXER_RETURN_TOKEN(LINEJOIN); }
<INITIAL>linejoinmaxsize                       { MS_LEXER_RETURN_TOKEN(LINEJOINMAXSIZE); }
<INITIAL,URL_VARIABLE>map                      { MS_LEXER_RETURN_TOKEN(MAP); }
<INITIAL>marker                                { MS_LEXER_RETURN_TOKEN(MARKER); }
<INITIAL>markersize                            { MS_LEXER_RETURN_TOKEN(MARKERSIZE); }
<INITIAL>mask                                  { MS_LEXER_RETURN_TOKEN(MASK); }
<INITIAL>maxarcs                               { MS_LEXER_RETURN_TOKEN(MAXARCS); }
<INITIAL>maxboxsize                            { MS_LEXER_RETURN_TOKEN(MAXBOXSIZE); }
<INITIAL>maxdistance                           { MS_LEXER_RETURN_TOKEN(MAXDISTANCE); }
<INITIAL>maxfeatures                           { MS_LEXER_RETURN_TOKEN(MAXFEATURES); }
<INITIAL>maxinterval                           { MS_LEXER_RETURN_TOKEN(MAXINTERVAL); }
<INITIAL>maxscale                              { MS_LEXER_RETURN_TOKEN(MAXSCALE); }
<INITIAL>maxscaledenom                         { MS_LEXER_RETURN_TOKEN(MAXSCALEDENOM); }
<INITIAL>maxgeowidth                           { MS_LEXER_RETURN_TOKEN(MAXGEOWIDTH); }
<INITIAL>maxlength                             { MS_LEXER_RETURN_TOKEN(MAXLENGTH); }
<INITIAL>maxsize                               { MS_LEXER_RETURN_TOKEN(MAXSIZE); }
<INITIAL>maxsubdivide                          { MS_LEXER_RETURN_TOKEN(MAXSUBDIVIDE); }
<INITIAL>maxtemplate                           { MS_LEXER_RETURN_TOKEN(MAXTEMPLATE); }
<INITIAL>maxwidth                              { MS_LEXER_RETURN_TOKEN(MAXWIDTH); }
<INITIAL>metadata                              { MS_LEXER_RETURN_TOKEN(METADATA); }
<INITIAL>mimetype                              { MS_LEXER_RETURN_TOKEN(MIMETYPE); }
<INITIAL>minarcs                               { MS_LEXER_RETURN_TOKEN(MINARCS); }
<INITIAL>minboxsize                            { MS_LEXER_RETURN_TOKEN(MINBOXSIZE); }
<INITIAL>mindistance                           { MS_LEXER_RETURN_TOKEN(MINDISTANCE); }
<INITIAL>repeatdistance                        { MS_LEXER_RETURN_TOKEN(REPEATDISTANCE); }
<INITIAL>maxoverlapangle                       { MS_LEXER_RETURN_TOKEN(MAXOVERLAPANGLE); } 
<INITIAL>minfeaturesize                        { MS_LEXER_RETURN_TOKEN(MINFEATURESIZE); }
<INITIAL>mininterval                           { MS_LEXER_RETURN_TOKEN(MININTERVAL); }
<INITIAL>minscale                              { MS_LEXER_RETURN_TOKEN(MINSCALE); }
<INITIAL>minscaledenom                         { MS_LEXER_RETURN_TOKEN(MINSCALEDENOM); }
<INITIAL>mingeowidth                           { MS_LEXER_RETURN_TOKEN(MINGEOWIDTH); }
<INITIAL>minsize                               { MS_LEXER_RETURN_TOKEN(MINSIZE); }
<INITIAL>minsubdivide                          { MS_LEXER_RETURN_TOKEN(MINSUBDIVIDE); }
<INITIAL>mintemplate                           { MS_LEXER_RETURN_TOKEN(MINTEMPLATE); }
<INITIAL>minwidth                              { MS_LEXER_RETURN_TOKEN(MINWIDTH); }
<INITIAL>name                                  { MS_LEXER_RETURN_TOKEN(NAME); }
<INITIAL,URL_STRING>offset                     { MS_LEXER_RETURN_TOKEN(OFFSET); }
<INITIAL>offsite                               { MS_LEXER_RETURN_TOKEN(OFFSITE); }
<INITIAL,URL_STRING>opacity                    { MS_LEXER_RETURN_TOKEN(OPACITY); }
<INITIAL>connectionoptions                     { MS_LEXER_RETURN_TOKEN(CONNECTIONOPTIONS); }
<INITIAL,URL_STRING>outlinecolor               { MS_LEXER_RETURN_TOKEN(OUTLINECOLOR); }
<INITIAL,URL_STRING>outlinewidth               { MS_LEXER_RETURN_TOKEN(OUTLINEWIDTH); }
<INITIAL>outputformat                          { MS_LEXER_RETURN_TOKEN(OUTPUTFORMAT); }
<INITIAL>partials                              { MS_LEXER_RETURN_TOKEN(PARTIALS); }
<INITIAL,URL_STRING>pattern                    { MS_LEXER_RETURN_TOKEN(PATTERN); }
<INITIAL,URL_STRING>points                     { MS_LEXER_RETURN_TOKEN(POINTS); }
<INITIAL,URL_STRING>items                      { MS_LEXER_RETURN_TOKEN(ITEMS); }
<INITIAL,URL_STRING>position                   { MS_LEXER_RETURN_TOKEN(POSITION); }
<INITIAL>postlabelcache                        { MS_LEXER_RETURN_TOKEN(POSTLABELCACHE); }
<INITIAL>priority                              { MS_LEXER_RETURN_TOKEN(PRIORITY); }
<INITIAL,URL_STRING>processing                 { MS_LEXER_RETURN_TOKEN(PROCESSING); }
<INITIAL,URL_VARIABLE,URL_STRING>projection    { MS_LEXER_RETURN_TOKEN(PROJECTION); }
<INITIAL>queryformat                           { MS_LEXER_RETURN_TOKEN(QUERYFORMAT); }
<INITIAL,URL_VARIABLE>querymap                 { MS_LEXER_RETURN_TOKEN(QUERYMAP); }
<INITIAL,URL_VARIABLE>reference                { MS_LEXER_RETURN_TOKEN(REFERENCE); }
<INITIAL>region                                { MS_LEXER_RETURN_TOKEN(REGION); }
<INITIAL>relativeto                            { MS_LEXER_RETURN_TOKEN(RELATIVETO); }
<INITIAL,URL_STRING>requires                   { MS_LEXER_RETURN_TOKEN(REQUIRES); }
<INITIAL,URL_VARIABLE>resolution               { MS_LEXER_RETURN_TOKEN(RESOLUTION); }
<INITIAL,URL_VARIABLE>defresolution            { MS_LEXER_RETURN_TOKEN(DEFRESOLUTION); }
<INITIAL>scale                                 { MS_LEXER_RETURN_TOKEN(SCALE); }
<INITIAL>scaledenom                            { MS_LEXER_RETURN_TOKEN(SCALEDENOM); }
<INITIAL,URL_VARIABLE>scalebar                 { MS_LEXER_RETURN_TOKEN(SCALEBAR); }
<INITIAL>scaletoken                            { MS_LEXER_RETURN_TOKEN(SCALETOKEN); }
<INITIAL,URL_STRING>shadowcolor                { MS_LEXER_RETURN_TOKEN(SHADOWCOLOR); }
<INITIAL,URL_STRING>shadowsize                 { MS_LEXER_RETURN_TOKEN(SHADOWSIZE); }
<INITIAL>shapepath                             { MS_LEXER_RETURN_TOKEN(SHAPEPATH); }
<INITIAL,URL_VARIABLE,URL_STRING>size          { MS_LEXER_RETURN_TOKEN(SIZE); }
<INITIAL>sizeunits                             { MS_LEXER_RETURN_TOKEN(SIZEUNITS); }
<INITIAL,URL_STRING>status                     { MS_LEXER_RETURN_TOKEN(STATUS); }
<INITIAL,URL_VARIABLE>style                    { MS_LEXER_RETURN_TOKEN(STYLE); }
<INITIAL>styleitem                             { MS_LEXER_RETURN_TOKEN(STYLEITEM); }
<INITIAL,URL_STRING>symbol                     { MS_LEXER_RETURN_TOKEN(SYMBOL); }
<INITIAL>symbolscale                           { MS_LEXER_RETURN_TOKEN(SYMBOLSCALE); }
<INITIAL>symbolscaledenom                      { MS_LEXER_RETURN_TOKEN(SYMBOLSCALEDENOM); }
<INITIAL>symbolset                             { MS_LEXER_RETURN_TOKEN(SYMBOLSET); }
<INITIAL>table                                 { MS_LEXER_RETURN_TOKEN(TABLE); }
<INITIAL,URL_STRING>template                   { MS_LEXER_RETURN_TOKEN(TEMPLATE); }
<INITIAL>templatepattern                       { MS_LEXER_RETURN_TOKEN(TEMPLATEPATTERN); }
<INITIAL,URL_STRING>text                       { MS_LEXER_RETURN_TOKEN(TEXT); }
<INITIAL,URL_STRING>tileindex                  { MS_LEXER_RETURN_TOKEN(TILEINDEX); }
<INITIAL,URL_STRING>tileitem                   { MS_LEXER_RETURN_TOKEN(TILEITEM); }
<INITIAL,URL_STRING>tilesrs                    { MS_LEXER_RETURN_TOKEN(TILESRS); }
<INITIAL,URL_STRING>title                      { MS_LEXER_RETURN_TOKEN(TITLE); }
<INITIAL>to                                    { MS_LEXER_RETURN_TOKEN(TO); }
<INITIAL,URL_STRING>tolerance                  { MS_LEXER_RETURN_TOKEN(TOLERANCE); }
<INITIAL,URL_STRING>toleranceunits             { MS_LEXER_RETURN_TOKEN(TOLERANCEUNITS); }
<INITIAL,URL_VARIABLE>transparent              { MS_LEXER_RETURN_TOKEN(TRANSPARENT); }
<INITIAL>transform                             { MS_LEXER_RETURN_TOKEN(TRANSFORM); }
<INITIAL>type                                  { MS_LEXER_RETURN_TOKEN(TYPE); }
<INITIAL,URL_VARIABLE,URL_STRING>units         { MS_LEXER_RETURN_TOKEN(UNITS); }
<INITIAL>utfdata                               { MS_LEXER_RETURN_TOKEN(UTFDATA); }
<INITIAL>utfitem                               { MS_LEXER_RETURN_TOKEN(UTFITEM); }
<INITIAL>validation                            { MS_LEXER_RETURN_TOKEN(VALIDATION); }
<INITIAL>values                                { MS_LEXER_RETURN_TOKEN(VALUES); }
<INITIAL,URL_VARIABLE>web                      { MS_LEXER_RETURN_TOKEN(WEB); }
<INITIAL,URL_STRING>width                      { MS_LEXER_RETURN_TOKEN(WIDTH); }
<INITIAL,URL_STRING>wkt                        { MS_LEXER_RETURN_TOKEN(WKT); }
<INITIAL>wrap                                  { MS_LEXER_RETURN_TOKEN(WRAP); }

<INITIAL,URL_STRING>annotation                 { MS_LEXER_RETURN_TOKEN(MS_LAYER_ANNOTATION); }
<INITIAL,URL_STRING>auto                       { MS_LEXER_RETURN_TOKEN(MS_AUTO); }
<INITIAL,URL_STRING>auto2                      { MS_LEXER_RETURN_TOKEN(MS_AUTO2); }
<INITIAL>bevel                                 { MS_LEXER_RETURN_TOKEN(MS_CJC_BEVEL); }
<INITIAL>bitmap                                { MS_LEXER_RETURN_TOKEN(MS_BITMAP); }
<INITIAL>butt                                  { MS_LEXER_RETURN_TOKEN(MS_CJC_BUTT); }
<INITIAL,URL_STRING>cc                         { MS_LEXER_RETURN_TOKEN(MS_CC); }
<INITIAL,URL_STRING>center                     { MS_LEXER_RETURN_TOKEN(MS_ALIGN_CENTER); }
<INITIAL>chart                                 { MS_LEXER_RETURN_TOKEN(MS_LAYER_CHART); }
<INITIAL>circle                                { MS_LEXER_RETURN_TOKEN(MS_LAYER_CIRCLE); }
<INITIAL,URL_STRING>cl                         { MS_LEXER_RETURN_TOKEN(MS_CL); }
<INITIAL,URL_STRING>cr                         { MS_LEXER_RETURN_TOKEN(MS_CR); }
<INITIAL>csv                                   { MS_LEXER_RETURN_TOKEN(MS_DB_CSV); }
<INITIAL>postgresql                            { MS_LEXER_RETURN_TOKEN(MS_DB_POSTGRES); }
<INITIAL>mysql                                 { MS_LEXER_RETURN_TOKEN(MS_DB_MYSQL); }
<INITIAL,URL_STRING>default                    { MS_LEXER_RETURN_TOKEN(MS_DEFAULT); }
<INITIAL,URL_STRING>dd                         { MS_LEXER_RETURN_TOKEN(MS_DD); }
<INITIAL>ellipse                               { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_ELLIPSE); }
<INITIAL,URL_STRING>embed                      { MS_LEXER_RETURN_TOKEN(MS_EMBED); }
<INITIAL,URL_STRING>false                      { MS_LEXER_RETURN_TOKEN(MS_FALSE); }
<INITIAL,URL_STRING>feet                       { MS_LEXER_RETURN_TOKEN(MS_FEET); }
<INITIAL,URL_STRING>follow                     { MS_LEXER_RETURN_TOKEN(MS_FOLLOW); }
<INITIAL>giant                                 { MS_LEXER_RETURN_TOKEN(MS_GIANT); }
<INITIAL>hatch                                 { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_HATCH); }
<INITIAL>kerneldensity                         { MS_LEXER_RETURN_TOKEN(MS_KERNELDENSITY); }
<INITIAL>idw                                   { MS_LEXER_RETURN_TOKEN(MS_IDW); }
<INITIAL>hilite                                { MS_LEXER_RETURN_TOKEN(MS_HILITE); }
<INITIAL,URL_STRING>inches                     { MS_LEXER_RETURN_TOKEN(MS_INCHES); }
<INITIAL,URL_STRING>kilometers                 { MS_LEXER_RETURN_TOKEN(MS_KILOMETERS); }
<INITIAL>large                                 { MS_LEXER_RETURN_TOKEN(MS_LARGE); }
<INITIAL,URL_STRING>lc                         { MS_LEXER_RETURN_TOKEN(MS_LC); }
<INITIAL,URL_STRING>left                       { MS_LEXER_RETURN_TOKEN(MS_ALIGN_LEFT); }
<INITIAL>line                                  { MS_LEXER_RETURN_TOKEN(MS_LAYER_LINE); }
<INITIAL,URL_STRING>ll                         { MS_LEXER_RETURN_TOKEN(MS_LL); }
<INITIAL,URL_STRING>lr                         { MS_LEXER_RETURN_TOKEN(MS_LR); }
<INITIAL>medium                                { MS_LEXER_RETURN_TOKEN(MS_MEDIUM); }
<INITIAL,URL_STRING>meters                     { MS_LEXER_RETURN_TOKEN(MS_METERS); }
<INITIAL,URL_STRING>nauticalmiles              { MS_LEXER_RETURN_TOKEN(MS_NAUTICALMILES); }
<INITIAL,URL_STRING>miles                      { MS_LEXER_RETURN_TOKEN(MS_MILES); }
<INITIAL>miter                                 { MS_LEXER_RETURN_TOKEN(MS_CJC_MITER); }
<INITIAL>multiple                              { MS_LEXER_RETURN_TOKEN(MS_MULTIPLE); }
<INITIAL>none                                  { MS_LEXER_RETURN_TOKEN(MS_CJC_NONE); }
<INITIAL>normal                                { MS_LEXER_RETURN_TOKEN(MS_NORMAL); }
<INITIAL,URL_STRING>off                        { MS_LEXER_RETURN_TOKEN(MS_OFF); }
<INITIAL>ogr                                   { MS_LEXER_RETURN_TOKEN(MS_OGR); }
<INITIAL,URL_STRING>on                         { MS_LEXER_RETURN_TOKEN(MS_ON); }
<INITIAL>one-to-one                            { MS_LEXER_RETURN_TOKEN(MS_JOIN_ONE_TO_ONE); }
<INITIAL>one-to-many                           { MS_LEXER_RETURN_TOKEN(MS_JOIN_ONE_TO_MANY); }
<INITIAL>oraclespatial                         { MS_LEXER_RETURN_TOKEN(MS_ORACLESPATIAL); }
<INITIAL>percentages                           { MS_LEXER_RETURN_TOKEN(MS_PERCENTAGES); }
<INITIAL>pixmap                                { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_PIXMAP); }
<INITIAL,URL_STRING>pixels                     { MS_LEXER_RETURN_TOKEN(MS_PIXELS); }
<INITIAL>point                                 { MS_LEXER_RETURN_TOKEN(MS_LAYER_POINT); }
<INITIAL>polygon                               { MS_LEXER_RETURN_TOKEN(MS_LAYER_POLYGON); }
<INITIAL>postgis                               { MS_LEXER_RETURN_TOKEN(MS_POSTGIS); }
<INITIAL>plugin                                { MS_LEXER_RETURN_TOKEN(MS_PLUGIN); }
<INITIAL>query                                 { MS_LEXER_RETURN_TOKEN(MS_LAYER_QUERY); }
<INITIAL>raster                                { MS_LEXER_RETURN_TOKEN(MS_LAYER_RASTER); }
<INITIAL,URL_STRING>right                      { MS_LEXER_RETURN_TOKEN(MS_ALIGN_RIGHT); }
<INITIAL>round                                 { MS_LEXER_RETURN_TOKEN(MS_CJC_ROUND); }
<INITIAL>selected                              { MS_LEXER_RETURN_TOKEN(MS_SELECTED); }
<INITIAL>simple                                { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_SIMPLE); }
<INITIAL>single                                { MS_LEXER_RETURN_TOKEN(MS_SINGLE); }
<INITIAL>small                                 { MS_LEXER_RETURN_TOKEN(MS_SMALL); }
<INITIAL>square                                { MS_LEXER_RETURN_TOKEN(MS_CJC_SQUARE); }
<INITIAL>svg                                   { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_SVG); }
<INITIAL>polaroffset                           { MS_LEXER_RETURN_TOKEN(POLAROFFSET); }
<INITIAL>tiny                                  { MS_LEXER_RETURN_TOKEN(MS_TINY); }
<INITIAL>triangle                              { MS_LEXER_RETURN_TOKEN(MS_CJC_TRIANGLE); }
<INITIAL,URL_STRING>true                       { MS_LEXER_RETURN_TOKEN(MS_TRUE); }
<INITIAL>truetype                              { MS_LEXER_RETURN_TOKEN(MS_TRUETYPE); }
<INITIAL,URL_STRING>uc                         { MS_LEXER_RETURN_TOKEN(MS_UC); }
<INITIAL,URL_STRING>ul                         { MS_LEXER_RETURN_TOKEN(MS_UL); }
<INITIAL,URL_STRING>ur                         { MS_LEXER_RETURN_TOKEN(MS_UR); }
<INITIAL>union                                 { MS_LEXER_RETURN_TOKEN(MS_UNION); }
<INITIAL>uvraster                              { MS_LEXER_RETURN_TOKEN(MS_UVRASTER); }
<INITIAL>contour                               { MS_LEXER_RETURN_TOKEN(MS_CONTOUR); }
<INITIAL>vector                                { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_VECTOR); }
<INITIAL>wfs                                   { MS_LEXER_RETURN_TOKEN(MS_WFS); }
<INITIAL>wms                                   { MS_LEXER_RETURN_TOKEN(MS_WMS); }

<URL_VARIABLE>\[[a-z/\.][a-z0-9/:\.\-\=_ ]*\]  {
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
<EXPRESSION_STRING>\[map_cellsize\] { 
  /* attribute binding - map cellsize */
  return(MS_TOKEN_BINDING_MAP_CELLSIZE);
}
<EXPRESSION_STRING>\[data_cellsize\] { 
  /* attribute binding - data cellsize */
  return(MS_TOKEN_BINDING_DATA_CELLSIZE);
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
<EXPRESSION_STRING>\`\[[^\`]*\]\` {
  /* attribute binding - time */
  msyytext+=2;
  msyytext[strlen(msyytext)-2] = '\0';
  MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext),
                          msyystring_buffer_size, msyystring_buffer_ptr);
  strcpy(msyystring_buffer, msyytext);
  return(MS_TOKEN_BINDING_TIME);
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

<INITIAL,URL_STRING>\/[^*]{1}[^\/]*\/i         {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-2] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext);
                                                 return(MS_IREGEX);
                                               }

<INITIAL,URL_STRING>\/[^*]{1}[^\/]*\/          {
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

<INITIAL,URL_STRING>\{.*\}                     {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext);
                                                 return(MS_LIST);
                                               }

<INITIAL,URL_STRING,CONFIG_FILE>\'|\"         {
                                                 msyystring_return_state = MS_STRING;
                                                 msyystring_begin = msyytext[0]; 
                                                 msyystring_size = 0;
                                                 msyystring_buffer_ptr = msyystring_buffer;
                                                 BEGIN(MSSTRING);
                                              }

<MSSTRING>\'|\"|\"i|\'i                       {
                                                MS_LEXER_STRING_REALLOC(msyystring_buffer, msyystring_size, 
                                                                                           msyystring_buffer_size, msyystring_buffer_ptr);
                                                if (msyystring_begin == msyytext[0]) {
                                                   BEGIN(msyystring_begin_state);

                                                   *msyystring_buffer_ptr = '\0';

                                                   if (msyystring_return_state == MS_STRING) {
                                                      if (msyystring_icase && strlen(msyytext)==2) {
                                                         msyystring_icase = MS_FALSE; // reset
                                                         return MS_ISTRING;
                                                      } else
                                                        return MS_STRING;
                                                   }
                                                   return msyystring_return_state;

                                                }
                                                else {
                                                  ++msyystring_size;
                                                  *msyystring_buffer_ptr++ = *msyytext;
                                                  if (strlen(msyytext)==2) {
                                                      MS_LEXER_STRING_REALLOC(msyystring_buffer, msyystring_size, 
                                                                              msyystring_buffer_size, msyystring_buffer_ptr);
                                                     ++msyystring_size;
                                                     *msyystring_buffer_ptr++ = msyytext[1];
                                                  }
                                                }
                                              }

<MSSTRING>\\\'|\\\"|\\\\|\\                  { 
                                                MS_LEXER_STRING_REALLOC(msyystring_buffer, msyystring_size, 
                                                                                           msyystring_buffer_size, msyystring_buffer_ptr);

                                                ++msyystring_size;
                                                if (strlen(msyytext) == 2)
                                                    *msyystring_buffer_ptr++ = msyytext[1]; 
                                                else
                                                    *msyystring_buffer_ptr++ = msyytext[0];
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
                                                   msyyin = YY_CURRENT_BUFFER->yy_input_file;
                                                   return(-1);
                                                 }

                                                 msyy_switch_to_buffer( msyy_create_buffer(msyyin, YY_BUF_SIZE) );
                                                 msyylineno = 1;

                                                 BEGIN(INITIAL);
                                               }

<EXPRESSION_STRING>\'|\"                      {
                                                 msyystring_return_state = MS_TOKEN_LITERAL_STRING;
                                                 msyystring_begin = msyytext[0]; 
                                                 msyystring_size = 0;
                                                 msyystring_buffer_ptr = msyystring_buffer;
                                                 BEGIN(MSSTRING);
                                              }

<INITIAL,URL_STRING,CONFIG_FILE>[a-z/\.][a-z0-9/\._\-\=]*   { 
                                                    MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                            msyystring_buffer_size, msyystring_buffer_ptr);
                                                    strcpy(msyystring_buffer, msyytext); 
                                                    return(MS_STRING); 
                                                }

<INITIAL,CONFIG_FILE>\n                        { msyylineno++; }

<INITIAL,CONFIG_FILE><<EOF>>                   {
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

<INITIAL,URL_VARIABLE,URL_STRING,CONFIG_FILE>. { 
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
  msSetError(MS_PARSEERR, "%s", "msyyparse()", s);
  return(0);
}

/* cleanup any open includes */
void msyycleanup_includes() {
  while( --include_stack_ptr >= 0 ) {
    fclose(YY_CURRENT_BUFFER->yy_input_file);
    msyy_delete_buffer( YY_CURRENT_BUFFER );
    msyy_switch_to_buffer(include_stack[include_stack_ptr]);
  }
}

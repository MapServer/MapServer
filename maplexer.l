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

<INITIAL,EXPRESSION_STRING,INCLUDE,CONFIG_FILE>[ \t\r]+             ;

<INITIAL,CONFIG_FILE>(\/\/|#).*                { if (msyyreturncomments) return(MS_COMMENT); }

<INITIAL>\/\*                                  { BEGIN(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>\*\/                        { BEGIN(INITIAL); }
<MULTILINE_COMMENT>[^*\n]+                     ;
<MULTILINE_COMMENT>\*                          ;
<MULTILINE_COMMENT>\n                          { msyylineno++; }

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

<INITIAL>align                                 { MS_LEXER_RETURN_TOKEN(ALIGN); }
<INITIAL>anchorpoint                           { MS_LEXER_RETURN_TOKEN(ANCHORPOINT); }
<INITIAL>angle                                 { MS_LEXER_RETURN_TOKEN(ANGLE); }
<INITIAL>antialias                             { MS_LEXER_RETURN_TOKEN(ANTIALIAS); }
<INITIAL>backgroundcolor                       { MS_LEXER_RETURN_TOKEN(BACKGROUNDCOLOR); }
<INITIAL>bandsitem                             { MS_LEXER_RETURN_TOKEN(BANDSITEM); }
<INITIAL>bindvals                              { MS_LEXER_RETURN_TOKEN(BINDVALS); }
<INITIAL>\xEF\xBB\xBF                          { MS_LEXER_RETURN_TOKEN(BOM); }
<INITIAL>browseformat                          { MS_LEXER_RETURN_TOKEN(BROWSEFORMAT); }
<INITIAL>buffer                                { MS_LEXER_RETURN_TOKEN(BUFFER); }
<INITIAL>character                             { MS_LEXER_RETURN_TOKEN(CHARACTER); }
<INITIAL>class                                 { MS_LEXER_RETURN_TOKEN(CLASS); }
<INITIAL>classitem                             { MS_LEXER_RETURN_TOKEN(CLASSITEM); }
<INITIAL>classgroup                            { MS_LEXER_RETURN_TOKEN(CLASSGROUP); }
<INITIAL>cluster                               { MS_LEXER_RETURN_TOKEN(CLUSTER); }
<INITIAL>color                                 { MS_LEXER_RETURN_TOKEN(COLOR); }
<INITIAL>compfilter                            { MS_LEXER_RETURN_TOKEN(COMPFILTER); }
<INITIAL>composite                             { MS_LEXER_RETURN_TOKEN(COMPOSITE); }
<INITIAL>compop                                { MS_LEXER_RETURN_TOKEN(COMPOP); }
<INITIAL>config                                { MS_LEXER_RETURN_TOKEN(CONFIG); }
<INITIAL>connection                            { MS_LEXER_RETURN_TOKEN(CONNECTION); }
<INITIAL>connectiontype                        { MS_LEXER_RETURN_TOKEN(CONNECTIONTYPE); }
<INITIAL>data                                  { MS_LEXER_RETURN_TOKEN(DATA); }
<INITIAL>datapattern                           { MS_LEXER_RETURN_TOKEN(DATAPATTERN); }
<INITIAL>debug                                 { MS_LEXER_RETURN_TOKEN(DEBUG); }
<INITIAL>driver                                { MS_LEXER_RETURN_TOKEN(DRIVER); }
<INITIAL>empty                                 { MS_LEXER_RETURN_TOKEN(EMPTY); }
<INITIAL>encoding                              { MS_LEXER_RETURN_TOKEN(ENCODING); }
<INITIAL,CONFIG_FILE>end                       { MS_LEXER_RETURN_TOKEN(END); }
<INITIAL>error                                 { MS_LEXER_RETURN_TOKEN(ERROR); }
<INITIAL>expression                            { MS_LEXER_RETURN_TOKEN(EXPRESSION); }
<INITIAL>extent                                { MS_LEXER_RETURN_TOKEN(EXTENT); }
<INITIAL>extension                             { MS_LEXER_RETURN_TOKEN(EXTENSION); }
<INITIAL>feature                               { MS_LEXER_RETURN_TOKEN(FEATURE); }
<INITIAL>filled                                { MS_LEXER_RETURN_TOKEN(FILLED); }
<INITIAL>filter                                { MS_LEXER_RETURN_TOKEN(FILTER); }
<INITIAL>filteritem                            { MS_LEXER_RETURN_TOKEN(FILTERITEM); }
<INITIAL>footer                                { MS_LEXER_RETURN_TOKEN(FOOTER); }
<INITIAL>font                                  { MS_LEXER_RETURN_TOKEN(FONT); }
<INITIAL>fontset                               { MS_LEXER_RETURN_TOKEN(FONTSET); }
<INITIAL>force                                 { MS_LEXER_RETURN_TOKEN(FORCE); }
<INITIAL>formatoption                          { MS_LEXER_RETURN_TOKEN(FORMATOPTION); }
<INITIAL>from                                  { MS_LEXER_RETURN_TOKEN(FROM); }
<INITIAL>gap                                   { MS_LEXER_RETURN_TOKEN(GAP); }
<INITIAL>geomtransform                         { MS_LEXER_RETURN_TOKEN(GEOMTRANSFORM); }
<INITIAL>grid                                  { MS_LEXER_RETURN_TOKEN(GRID); }
<INITIAL>gridstep                              { MS_LEXER_RETURN_TOKEN(GRIDSTEP); }
<INITIAL>graticule                             { MS_LEXER_RETURN_TOKEN(GRATICULE); }
<INITIAL>group                                 { MS_LEXER_RETURN_TOKEN(GROUP); }
<INITIAL>header                                { MS_LEXER_RETURN_TOKEN(HEADER); }
<INITIAL>image                                 { MS_LEXER_RETURN_TOKEN(IMAGE); }
<INITIAL>imagecolor                            { MS_LEXER_RETURN_TOKEN(IMAGECOLOR); }
<INITIAL>imagetype                             { MS_LEXER_RETURN_TOKEN(IMAGETYPE); }
<INITIAL>imagemode                             { MS_LEXER_RETURN_TOKEN(IMAGEMODE); }
<INITIAL>imagepath                             { MS_LEXER_RETURN_TOKEN(IMAGEPATH); }
<INITIAL>temppath                              { MS_LEXER_RETURN_TOKEN(TEMPPATH); }
<INITIAL>imageurl                              { MS_LEXER_RETURN_TOKEN(IMAGEURL); }
<INITIAL>include                               { BEGIN(INCLUDE); }
<INITIAL>index                                 { MS_LEXER_RETURN_TOKEN(INDEX); }
<INITIAL>initialgap                            { MS_LEXER_RETURN_TOKEN(INITIALGAP); }
<INITIAL>intervals                             { MS_LEXER_RETURN_TOKEN(INTERVALS); } 
<INITIAL>join                                  { MS_LEXER_RETURN_TOKEN(JOIN); }
<INITIAL>keyimage                              { MS_LEXER_RETURN_TOKEN(KEYIMAGE); }
<INITIAL>keysize                               { MS_LEXER_RETURN_TOKEN(KEYSIZE); }
<INITIAL>keyspacing                            { MS_LEXER_RETURN_TOKEN(KEYSPACING); }
<INITIAL>label                                 { MS_LEXER_RETURN_TOKEN(LABEL); }
<INITIAL>labelcache                            { MS_LEXER_RETURN_TOKEN(LABELCACHE); }
<INITIAL>labelformat                           { MS_LEXER_RETURN_TOKEN(LABELFORMAT); }
<INITIAL>labelitem                             { MS_LEXER_RETURN_TOKEN(LABELITEM); }
<INITIAL>labelmaxscale                         { MS_LEXER_RETURN_TOKEN(LABELMAXSCALE); }
<INITIAL>labelmaxscaledenom                    { MS_LEXER_RETURN_TOKEN(LABELMAXSCALEDENOM); }
<INITIAL>labelminscale                         { MS_LEXER_RETURN_TOKEN(LABELMINSCALE); }
<INITIAL>labelminscaledenom                    { MS_LEXER_RETURN_TOKEN(LABELMINSCALEDENOM); }
<INITIAL>labelrequires                         { MS_LEXER_RETURN_TOKEN(LABELREQUIRES); }
<INITIAL>latlon                                { MS_LEXER_RETURN_TOKEN(LATLON); }
<INITIAL>layer                                 { MS_LEXER_RETURN_TOKEN(LAYER); }
<INITIAL>leader                                { MS_LEXER_RETURN_TOKEN(LEADER); }
<INITIAL>legend                                { MS_LEXER_RETURN_TOKEN(LEGEND); }
<INITIAL>legendformat                          { MS_LEXER_RETURN_TOKEN(LEGENDFORMAT); }
<INITIAL>linecap                               { MS_LEXER_RETURN_TOKEN(LINECAP); }
<INITIAL>linejoin                              { MS_LEXER_RETURN_TOKEN(LINEJOIN); }
<INITIAL>linejoinmaxsize                       { MS_LEXER_RETURN_TOKEN(LINEJOINMAXSIZE); }
<INITIAL>map                                   { MS_LEXER_RETURN_TOKEN(MAP); }
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
<INITIAL>offset                                { MS_LEXER_RETURN_TOKEN(OFFSET); }
<INITIAL>offsite                               { MS_LEXER_RETURN_TOKEN(OFFSITE); }
<INITIAL>opacity                               { MS_LEXER_RETURN_TOKEN(OPACITY); }
<INITIAL>connectionoptions                     { MS_LEXER_RETURN_TOKEN(CONNECTIONOPTIONS); }
<INITIAL>outlinecolor                          { MS_LEXER_RETURN_TOKEN(OUTLINECOLOR); }
<INITIAL>outlinewidth                          { MS_LEXER_RETURN_TOKEN(OUTLINEWIDTH); }
<INITIAL>outputformat                          { MS_LEXER_RETURN_TOKEN(OUTPUTFORMAT); }
<INITIAL>partials                              { MS_LEXER_RETURN_TOKEN(PARTIALS); }
<INITIAL>pattern                               { MS_LEXER_RETURN_TOKEN(PATTERN); }
<INITIAL>points                                { MS_LEXER_RETURN_TOKEN(POINTS); }
<INITIAL>items                                 { MS_LEXER_RETURN_TOKEN(ITEMS); }
<INITIAL>position                              { MS_LEXER_RETURN_TOKEN(POSITION); }
<INITIAL>postlabelcache                        { MS_LEXER_RETURN_TOKEN(POSTLABELCACHE); }
<INITIAL>priority                              { MS_LEXER_RETURN_TOKEN(PRIORITY); }
<INITIAL>processing                            { MS_LEXER_RETURN_TOKEN(PROCESSING); }
<INITIAL>projection                            { MS_LEXER_RETURN_TOKEN(PROJECTION); }
<INITIAL>queryformat                           { MS_LEXER_RETURN_TOKEN(QUERYFORMAT); }
<INITIAL>querymap                              { MS_LEXER_RETURN_TOKEN(QUERYMAP); }
<INITIAL>reference                             { MS_LEXER_RETURN_TOKEN(REFERENCE); }
<INITIAL>region                                { MS_LEXER_RETURN_TOKEN(REGION); }
<INITIAL>relativeto                            { MS_LEXER_RETURN_TOKEN(RELATIVETO); }
<INITIAL>requires                              { MS_LEXER_RETURN_TOKEN(REQUIRES); }
<INITIAL>resolution                            { MS_LEXER_RETURN_TOKEN(RESOLUTION); }
<INITIAL>defresolution                         { MS_LEXER_RETURN_TOKEN(DEFRESOLUTION); }
<INITIAL>scale                                 { MS_LEXER_RETURN_TOKEN(SCALE); }
<INITIAL>scaledenom                            { MS_LEXER_RETURN_TOKEN(SCALEDENOM); }
<INITIAL>scalebar                              { MS_LEXER_RETURN_TOKEN(SCALEBAR); }
<INITIAL>scaletoken                            { MS_LEXER_RETURN_TOKEN(SCALETOKEN); }
<INITIAL>shadowcolor                           { MS_LEXER_RETURN_TOKEN(SHADOWCOLOR); }
<INITIAL>shadowsize                            { MS_LEXER_RETURN_TOKEN(SHADOWSIZE); }
<INITIAL>shapepath                             { MS_LEXER_RETURN_TOKEN(SHAPEPATH); }
<INITIAL>size                                  { MS_LEXER_RETURN_TOKEN(SIZE); }
<INITIAL>sizeunits                             { MS_LEXER_RETURN_TOKEN(SIZEUNITS); }
<INITIAL>status                                { MS_LEXER_RETURN_TOKEN(STATUS); }
<INITIAL>style                                 { MS_LEXER_RETURN_TOKEN(STYLE); }
<INITIAL>styleitem                             { MS_LEXER_RETURN_TOKEN(STYLEITEM); }
<INITIAL>symbol                                { MS_LEXER_RETURN_TOKEN(SYMBOL); }
<INITIAL>symbolscale                           { MS_LEXER_RETURN_TOKEN(SYMBOLSCALE); }
<INITIAL>symbolscaledenom                      { MS_LEXER_RETURN_TOKEN(SYMBOLSCALEDENOM); }
<INITIAL>symbolset                             { MS_LEXER_RETURN_TOKEN(SYMBOLSET); }
<INITIAL>table                                 { MS_LEXER_RETURN_TOKEN(TABLE); }
<INITIAL>template                              { MS_LEXER_RETURN_TOKEN(TEMPLATE); }
<INITIAL>templatepattern                       { MS_LEXER_RETURN_TOKEN(TEMPLATEPATTERN); }
<INITIAL>text                                  { MS_LEXER_RETURN_TOKEN(TEXT); }
<INITIAL>tileindex                             { MS_LEXER_RETURN_TOKEN(TILEINDEX); }
<INITIAL>tileitem                              { MS_LEXER_RETURN_TOKEN(TILEITEM); }
<INITIAL>tilesrs                               { MS_LEXER_RETURN_TOKEN(TILESRS); }
<INITIAL>title                                 { MS_LEXER_RETURN_TOKEN(TITLE); }
<INITIAL>to                                    { MS_LEXER_RETURN_TOKEN(TO); }
<INITIAL>tolerance                             { MS_LEXER_RETURN_TOKEN(TOLERANCE); }
<INITIAL>toleranceunits                        { MS_LEXER_RETURN_TOKEN(TOLERANCEUNITS); }
<INITIAL>transparent                           { MS_LEXER_RETURN_TOKEN(TRANSPARENT); }
<INITIAL>transform                             { MS_LEXER_RETURN_TOKEN(TRANSFORM); }
<INITIAL>type                                  { MS_LEXER_RETURN_TOKEN(TYPE); }
<INITIAL>units                                 { MS_LEXER_RETURN_TOKEN(UNITS); }
<INITIAL>utfdata                               { MS_LEXER_RETURN_TOKEN(UTFDATA); }
<INITIAL>utfitem                               { MS_LEXER_RETURN_TOKEN(UTFITEM); }
<INITIAL>validation                            { MS_LEXER_RETURN_TOKEN(VALIDATION); }
<INITIAL>values                                { MS_LEXER_RETURN_TOKEN(VALUES); }
<INITIAL>web                                   { MS_LEXER_RETURN_TOKEN(WEB); }
<INITIAL>width                                 { MS_LEXER_RETURN_TOKEN(WIDTH); }
<INITIAL>wkt                                   { MS_LEXER_RETURN_TOKEN(WKT); }
<INITIAL>wrap                                  { MS_LEXER_RETURN_TOKEN(WRAP); }

<INITIAL>annotation                            { MS_LEXER_RETURN_TOKEN(MS_LAYER_ANNOTATION); }
<INITIAL>auto                                  { MS_LEXER_RETURN_TOKEN(MS_AUTO); }
<INITIAL>auto2                                 { MS_LEXER_RETURN_TOKEN(MS_AUTO2); }
<INITIAL>bevel                                 { MS_LEXER_RETURN_TOKEN(MS_CJC_BEVEL); }
<INITIAL>bitmap                                { MS_LEXER_RETURN_TOKEN(MS_BITMAP); }
<INITIAL>butt                                  { MS_LEXER_RETURN_TOKEN(MS_CJC_BUTT); }
<INITIAL>cc                                    { MS_LEXER_RETURN_TOKEN(MS_CC); }
<INITIAL>center                                { MS_LEXER_RETURN_TOKEN(MS_ALIGN_CENTER); }
<INITIAL>chart                                 { MS_LEXER_RETURN_TOKEN(MS_LAYER_CHART); }
<INITIAL>circle                                { MS_LEXER_RETURN_TOKEN(MS_LAYER_CIRCLE); }
<INITIAL>cl                                    { MS_LEXER_RETURN_TOKEN(MS_CL); }
<INITIAL>cr                                    { MS_LEXER_RETURN_TOKEN(MS_CR); }
<INITIAL>csv                                   { MS_LEXER_RETURN_TOKEN(MS_DB_CSV); }
<INITIAL>postgresql                            { MS_LEXER_RETURN_TOKEN(MS_DB_POSTGRES); }
<INITIAL>mysql                                 { MS_LEXER_RETURN_TOKEN(MS_DB_MYSQL); }
<INITIAL>default                               { MS_LEXER_RETURN_TOKEN(MS_DEFAULT); }
<INITIAL>dd                                    { MS_LEXER_RETURN_TOKEN(MS_DD); }
<INITIAL>ellipse                               { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_ELLIPSE); }
<INITIAL>embed                                 { MS_LEXER_RETURN_TOKEN(MS_EMBED); }
<INITIAL>false                                 { MS_LEXER_RETURN_TOKEN(MS_FALSE); }
<INITIAL>feet                                  { MS_LEXER_RETURN_TOKEN(MS_FEET); }
<INITIAL>follow                                { MS_LEXER_RETURN_TOKEN(MS_FOLLOW); }
<INITIAL>giant                                 { MS_LEXER_RETURN_TOKEN(MS_GIANT); }
<INITIAL>hatch                                 { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_HATCH); }
<INITIAL>kerneldensity                         { MS_LEXER_RETURN_TOKEN(MS_KERNELDENSITY); }
<INITIAL>idw                                   { MS_LEXER_RETURN_TOKEN(MS_IDW); }
<INITIAL>hilite                                { MS_LEXER_RETURN_TOKEN(MS_HILITE); }
<INITIAL>inches                                { MS_LEXER_RETURN_TOKEN(MS_INCHES); }
<INITIAL>kilometers                            { MS_LEXER_RETURN_TOKEN(MS_KILOMETERS); }
<INITIAL>large                                 { MS_LEXER_RETURN_TOKEN(MS_LARGE); }
<INITIAL>lc                                    { MS_LEXER_RETURN_TOKEN(MS_LC); }
<INITIAL>left                                  { MS_LEXER_RETURN_TOKEN(MS_ALIGN_LEFT); }
<INITIAL>line                                  { MS_LEXER_RETURN_TOKEN(MS_LAYER_LINE); }
<INITIAL>ll                                    { MS_LEXER_RETURN_TOKEN(MS_LL); }
<INITIAL>lr                                    { MS_LEXER_RETURN_TOKEN(MS_LR); }
<INITIAL>medium                                { MS_LEXER_RETURN_TOKEN(MS_MEDIUM); }
<INITIAL>meters                                { MS_LEXER_RETURN_TOKEN(MS_METERS); }
<INITIAL>nauticalmiles                         { MS_LEXER_RETURN_TOKEN(MS_NAUTICALMILES); }
<INITIAL>miles                                 { MS_LEXER_RETURN_TOKEN(MS_MILES); }
<INITIAL>miter                                 { MS_LEXER_RETURN_TOKEN(MS_CJC_MITER); }
<INITIAL>multiple                              { MS_LEXER_RETURN_TOKEN(MS_MULTIPLE); }
<INITIAL>none                                  { MS_LEXER_RETURN_TOKEN(MS_CJC_NONE); }
<INITIAL>normal                                { MS_LEXER_RETURN_TOKEN(MS_NORMAL); }
<INITIAL>off                                   { MS_LEXER_RETURN_TOKEN(MS_OFF); }
<INITIAL>ogr                                   { MS_LEXER_RETURN_TOKEN(MS_OGR); }
<INITIAL>on                                    { MS_LEXER_RETURN_TOKEN(MS_ON); }
<INITIAL>one-to-one                            { MS_LEXER_RETURN_TOKEN(MS_JOIN_ONE_TO_ONE); }
<INITIAL>one-to-many                           { MS_LEXER_RETURN_TOKEN(MS_JOIN_ONE_TO_MANY); }
<INITIAL>oraclespatial                         { MS_LEXER_RETURN_TOKEN(MS_ORACLESPATIAL); }
<INITIAL>percentages                           { MS_LEXER_RETURN_TOKEN(MS_PERCENTAGES); }
<INITIAL>pixmap                                { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_PIXMAP); }
<INITIAL>pixels                                { MS_LEXER_RETURN_TOKEN(MS_PIXELS); }
<INITIAL>point                                 { MS_LEXER_RETURN_TOKEN(MS_LAYER_POINT); }
<INITIAL>polygon                               { MS_LEXER_RETURN_TOKEN(MS_LAYER_POLYGON); }
<INITIAL>postgis                               { MS_LEXER_RETURN_TOKEN(MS_POSTGIS); }
<INITIAL>plugin                                { MS_LEXER_RETURN_TOKEN(MS_PLUGIN); }
<INITIAL>query                                 { MS_LEXER_RETURN_TOKEN(MS_LAYER_QUERY); }
<INITIAL>raster                                { MS_LEXER_RETURN_TOKEN(MS_LAYER_RASTER); }
<INITIAL>right                                 { MS_LEXER_RETURN_TOKEN(MS_ALIGN_RIGHT); }
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
<INITIAL>true                                  { MS_LEXER_RETURN_TOKEN(MS_TRUE); }
<INITIAL>truetype                              { MS_LEXER_RETURN_TOKEN(MS_TRUETYPE); }
<INITIAL>uc                                    { MS_LEXER_RETURN_TOKEN(MS_UC); }
<INITIAL>ul                                    { MS_LEXER_RETURN_TOKEN(MS_UL); }
<INITIAL>ur                                    { MS_LEXER_RETURN_TOKEN(MS_UR); }
<INITIAL>union                                 { MS_LEXER_RETURN_TOKEN(MS_UNION); }
<INITIAL>uvraster                              { MS_LEXER_RETURN_TOKEN(MS_UVRASTER); }
<INITIAL>contour                               { MS_LEXER_RETURN_TOKEN(MS_CONTOUR); }
<INITIAL>vector                                { MS_LEXER_RETURN_TOKEN(MS_SYMBOL_VECTOR); }
<INITIAL>wfs                                   { MS_LEXER_RETURN_TOKEN(MS_WFS); }
<INITIAL>wms                                   { MS_LEXER_RETURN_TOKEN(MS_WMS); }

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

<INITIAL>-?[0-9]+|-?[0-9]+\.[0-9]*|-?\.[0-9]*|-?[0-9]+[eE][+-]?[0-9]+|-?[0-9]+\.[0-9]*[eE][+-]?[0-9]+|-?\.[0-9]*[eE][+-]?[0-9]+ {
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

<INITIAL>\/[^*]{1}[^\/]*\/i         {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-2] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext);
                                                 return(MS_IREGEX);
                                               }

<INITIAL>\/[^*]{1}[^\/]*\/          {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext);
                                                 return(MS_REGEX);
                                               }

<INITIAL>\(.*\)                     {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext);
                                                 return(MS_EXPRESSION);
                                               }

<INITIAL>\{.*\}                     {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                         msyystring_buffer_size, msyystring_buffer_ptr);
                                                 strcpy(msyystring_buffer, msyytext);
                                                 return(MS_LIST);
                                               }

<INITIAL,CONFIG_FILE>\'|\"         {
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

<INITIAL,CONFIG_FILE>[a-z/\.][a-z0-9/\._\-\=]*   { 
                                                    MS_LEXER_STRING_REALLOC(msyystring_buffer, strlen(msyytext), 
                                                                            msyystring_buffer_size, msyystring_buffer_ptr);
                                                    strcpy(msyystring_buffer, msyytext); 
                                                    return(MS_STRING); 
                                                }

<INITIAL,CONFIG_FILE>\n                        { msyylineno++; }

<INITIAL,CONFIG_FILE><<EOF>>                    {
                                                  if( --include_stack_ptr < 0 )
                                                    return(EOF); /* end of main file */
                                                  else {
                                                    fclose(YY_CURRENT_BUFFER->yy_input_file);
                                                    msyy_delete_buffer( YY_CURRENT_BUFFER );
                                                    msyy_switch_to_buffer(include_stack[include_stack_ptr]);
                                                    msyylineno = include_lineno[include_stack_ptr];
                                                  }
                                                }

<EXPRESSION_STRING>[\r|\n|\0] {
  return(0); 
}

<INITIAL,CONFIG_FILE>.                          { 
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

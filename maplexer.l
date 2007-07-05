%{
/* C declarations */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <time.h>

#include "map.h"
#include "maperror.h"
#include "mapfile.h"
#include "maptime.h"
#include "mapsymbol.h"
#include "mapparser.h"

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

double msyynumber;
int msyystate=MS_TOKENIZE_DEFAULT;
char *msyystring=NULL;
char *msyybasepath=NULL;

int msyyreturncomments = 0;

#define MAX_INCLUDE_DEPTH 5
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;
char path[MS_MAXPATHLEN];
%}

%s NAME_STRING
%s VALUE_STRING
%s EXPRESSION_STRING
%s INCLUDE

%%
       switch(msyystate) {
       case(MS_TOKENIZE_DEFAULT):
         break;
       case(MS_TOKENIZE_FILE):
         BEGIN(INITIAL);
         msyystate=MS_TOKENIZE_DEFAULT;
         msyystring=NULL;
         msyyreturncomments=0;
         return(0);
         break;
       case(MS_TOKENIZE_STRING):
         BEGIN(INITIAL);
         msyystate=MS_TOKENIZE_DEFAULT;
         msyy_scan_string(msyystring);
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
       case(MS_TOKENIZE_NAME):         
         BEGIN(NAME_STRING);
         msyy_scan_string(msyystring);
         msyystate=MS_TOKENIZE_DEFAULT;
         msyyreturncomments=0; 
         (void) yyunput; /* just to avoid warning about it being unrefed */
         break;
       case(MS_TOKENIZE_VALUE):
         BEGIN(VALUE_STRING);
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

<INITIAL,VALUE_STRING,EXPRESSION_STRING,INCLUDE>[ \t\r]+             ;

<INITIAL>#.*                                   { if (msyyreturncomments) return(MS_COMMENT); }

<NAME_STRING>_+                                          ;

<EXPRESSION_STRING>or|\|\|                     { return(OR); }
<EXPRESSION_STRING>and|&&                      { return(AND); }
<EXPRESSION_STRING>eq|=|==                     { return(EQ); }
<EXPRESSION_STRING>ne|!=                       { return(NE); }
<EXPRESSION_STRING>gt|>                        { return(GT); }
<EXPRESSION_STRING>lt|<                        { return(LT); }
<EXPRESSION_STRING>ge|>=                       { return(GE); }
<EXPRESSION_STRING>le|<=                       { return(LE); }
<EXPRESSION_STRING>=~                          { return(RE); }
<EXPRESSION_STRING>in                          { return(IN); }
<EXPRESSION_STRING>not|!                       { return(NOT); }

<EXPRESSION_STRING>length                      { return(LENGTH); }
<EXPRESSION_STRING>ieq                         { return(IEQ); }

<INITIAL,NAME_STRING>colorrange              { return(COLORRANGE); }
<INITIAL,NAME_STRING>alphacolorrange         { return(ALPHACOLORRANGE); }
<INITIAL,NAME_STRING>datarange               { return(DATARANGE); }
<INITIAL,NAME_STRING>rangeitem               { return(RANGEITEM); }

<INITIAL,NAME_STRING>alphacolor              { return(ALPHACOLOR); }
<INITIAL,NAME_STRING>angle                   { return(ANGLE); }
<INITIAL,NAME_STRING>antialias               { return(ANTIALIAS); }
<INITIAL,NAME_STRING>backgroundcolor         { return(BACKGROUNDCOLOR); }
<INITIAL,NAME_STRING>backgroundshadowcolor   { return(BACKGROUNDSHADOWCOLOR); }
<INITIAL,NAME_STRING>backgroundshadowsize    { return(BACKGROUNDSHADOWSIZE); }
<INITIAL,NAME_STRING>bandsitem               { return(BANDSITEM); }
<INITIAL,NAME_STRING>browseformat            { return(BROWSEFORMAT); }
<INITIAL,NAME_STRING>buffer                  { return(BUFFER); }
<INITIAL>character                             { return(CHARACTER); }
<INITIAL,NAME_STRING>class                   { return(CLASS); }
<INITIAL,NAME_STRING>classitem               { return(CLASSITEM); }
<INITIAL,NAME_STRING>color                   { return(COLOR); }
<INITIAL,NAME_STRING>config                  { return(CONFIG); }
<INITIAL,NAME_STRING>connection              { return(CONNECTION); }
<INITIAL,NAME_STRING>connectiontype          { return(CONNECTIONTYPE); }
<INITIAL,NAME_STRING>data                    { return(DATA); }
<INITIAL,NAME_STRING>datapattern             { return(DATAPATTERN); }
<INITIAL,NAME_STRING>debug                   { return(DEBUG); }
<INITIAL,NAME_STRING>driver                  { return(DRIVER); }
<INITIAL,NAME_STRING>dump                    { return(DUMP); }
<INITIAL,NAME_STRING>empty                   { return(EMPTY); }
<INITIAL,NAME_STRING>encoding                { return(ENCODING); }
<INITIAL>end                                   { return(END); }
<INITIAL,NAME_STRING>error                   { return(ERROR); }
<INITIAL,NAME_STRING>expression              { return(EXPRESSION); }
<INITIAL,NAME_STRING>extent                  { return(EXTENT); }
<INITIAL,NAME_STRING>extension               { return(EXTENSION); }
<INITIAL,NAME_STRING>feature                 { return(FEATURE); }
<INITIAL>filled                                { return(FILLED); }
<INITIAL,NAME_STRING>filter                  { return(FILTER); }
<INITIAL,NAME_STRING>filteritem              { return(FILTERITEM); }
<INITIAL,NAME_STRING>footer                  { return(FOOTER); }
<INITIAL,NAME_STRING>font                    { return(FONT); }
<INITIAL>fontset                               { return(FONTSET); }
<INITIAL,NAME_STRING>force                   { return(FORCE); }
<INITIAL,NAME_STRING>formatoption            { return(FORMATOPTION); }
<INITIAL,NAME_STRING>from                    { return(FROM); }
<INITIAL>gap                                   { return(GAP); }
<INITIAL,NAME_STRING>grid                    { return(GRID); }
<INITIAL,NAME_STRING>graticule               { return(GRATICULE); }
<INITIAL,NAME_STRING>group                   { return(GROUP); }
<INITIAL,NAME_STRING>header                  { return(HEADER); }
<INITIAL,NAME_STRING>image                   { return(IMAGE); }
<INITIAL,NAME_STRING>imagecolor              { return(IMAGECOLOR); }
<INITIAL,NAME_STRING>imagetype               { return(IMAGETYPE); }
<INITIAL,NAME_STRING>imagequality            { return(IMAGEQUALITY); }
<INITIAL,NAME_STRING>imagemode               { return(IMAGEMODE); }
<INITIAL,NAME_STRING>imagepath               { return(IMAGEPATH); }
<INITIAL,NAME_STRING>imageurl                { return(IMAGEURL); }
<INITIAL>include                               { BEGIN(INCLUDE); }
<INITIAL,NAME_STRING>index                   { return(INDEX); }
<INITIAL,NAME_STRING>interlace               { return(INTERLACE); }
<INITIAL,NAME_STRING>intervals               { return(INTERVALS); } 
<INITIAL,NAME_STRING>join                    { return(JOIN); }
<INITIAL,NAME_STRING>keyimage                { return(KEYIMAGE); }
<INITIAL,NAME_STRING>keysize                 { return(KEYSIZE); }
<INITIAL,NAME_STRING>keyspacing              { return(KEYSPACING); }
<INITIAL,NAME_STRING>label                   { return(LABEL); }
<INITIAL,NAME_STRING>labelcache              { return(LABELCACHE); }
<INITIAL,NAME_STRING>labelformat             { return(LABELFORMAT); }
<INITIAL,NAME_STRING>labelitem               { return(LABELITEM); }
<INITIAL,NAME_STRING>labelmaxscale           { return(LABELMAXSCALE); }
<INITIAL,NAME_STRING>labelminscale           { return(LABELMINSCALE); }
<INITIAL,NAME_STRING>labelrequires           { return(LABELREQUIRES); }
<INITIAL,NAME_STRING>latlon                  { return(LATLON); }
<INITIAL,NAME_STRING>layer                   { return(LAYER); }
<INITIAL,NAME_STRING>legend                  { return(LEGEND); }
<INITIAL,NAME_STRING>legendformat            { return(LEGENDFORMAT); }
<INITIAL>linecap                               { return(LINECAP); }
<INITIAL>linejoin                              { return(LINEJOIN); }
<INITIAL>linejoinmaxsize                       { return(LINEJOINMAXSIZE); }
<INITIAL>log                                   { return(LOG); }
<INITIAL,NAME_STRING>map                     { return(MAP); }
<INITIAL,NAME_STRING>marker                  { return(MARKER); }
<INITIAL,NAME_STRING>markersize              { return(MARKERSIZE); }
<INITIAL,NAME_STRING>maxarcs                 { return(MAXARCS); }
<INITIAL,NAME_STRING>maxboxsize              { return(MAXBOXSIZE); }
<INITIAL,NAME_STRING>maxfeatures             { return(MAXFEATURES); }
<INITIAL,NAME_STRING>maxinterval             { return(MAXINTERVAL); }
<INITIAL,NAME_STRING>maxscale                { return(MAXSCALE); }
<INITIAL,NAME_STRING>maxsize                 { return(MAXSIZE); }
<INITIAL,NAME_STRING>maxsubdivide            { return(MAXSUBDIVIDE); }
<INITIAL,NAME_STRING>maxtemplate             { return(MAXTEMPLATE); }
<INITIAL,NAME_STRING>maxwidth                { return(MAXWIDTH); }
<INITIAL,NAME_STRING>metadata                { return(METADATA); }
<INITIAL,NAME_STRING>mimetype                { return(MIMETYPE); }
<INITIAL,NAME_STRING>minarcs                 { return(MINARCS); }
<INITIAL,NAME_STRING>minboxsize              { return(MINBOXSIZE); }
<INITIAL,NAME_STRING>mindistance             { return(MINDISTANCE); }
<INITIAL,NAME_STRING>minfeaturesize          { return(MINFEATURESIZE); }
<INITIAL,NAME_STRING>mininterval             { return(MININTERVAL); }
<INITIAL,NAME_STRING>minscale                { return(MINSCALE); }
<INITIAL,NAME_STRING>minsize                 { return(MINSIZE); }
<INITIAL,NAME_STRING>minsubdivide            { return(MINSUBDIVIDE); }
<INITIAL,NAME_STRING>mintemplate             { return(MINTEMPLATE); }
<INITIAL,NAME_STRING>minwidth                { return(MINWIDTH); }
<INITIAL,NAME_STRING>name                    { return(NAME); }
<INITIAL,NAME_STRING>offset                  { return(OFFSET); }
<INITIAL,NAME_STRING>offsite                 { return(OFFSITE); }
<INITIAL,NAME_STRING>opacity                 { return(OPACITY); }
<INITIAL,NAME_STRING>outlinecolor            { return(OUTLINECOLOR); }
<INITIAL,NAME_STRING>outputformat            { return(OUTPUTFORMAT); }
<INITIAL,NAME_STRING>overlaybackgroundcolor  { return(OVERLAYBACKGROUNDCOLOR); }
<INITIAL,NAME_STRING>overlaycolor            { return(OVERLAYCOLOR); }
<INITIAL,NAME_STRING>overlaymaxsize          { return(OVERLAYMAXSIZE); }
<INITIAL,NAME_STRING>overlayminsize          { return(OVERLAYMINSIZE); }
<INITIAL,NAME_STRING>overlayoutlinecolor     { return(OVERLAYOUTLINECOLOR); }
<INITIAL,NAME_STRING>overlaysize             { return(OVERLAYSIZE); }
<INITIAL,NAME_STRING>overlaysymbol           { return(OVERLAYSYMBOL); }
<INITIAL,NAME_STRING>partials                { return(PARTIALS); }
<INITIAL,NAME_STRING>points                  { return(POINTS); }
<INITIAL,NAME_STRING>position                { return(POSITION); }
<INITIAL,NAME_STRING>postlabelcache          { return(POSTLABELCACHE); }
<INITIAL,NAME_STRING>priority                { return(PRIORITY); }
<INITIAL,NAME_STRING>processing              { return(PROCESSING); }
<INITIAL,NAME_STRING>projection              { return(PROJECTION); }
<INITIAL,NAME_STRING>queryformat             { return(QUERYFORMAT); }
<INITIAL,NAME_STRING>querymap                { return(QUERYMAP); }
<INITIAL,NAME_STRING>reference               { return(REFERENCE); }
<INITIAL,NAME_STRING>relativeto              { return(RELATIVETO); }
<INITIAL,NAME_STRING>requires                { return(REQUIRES); }
<INITIAL,NAME_STRING>resolution              { return(RESOLUTION); }
<INITIAL,NAME_STRING>scale                   { return(SCALE); }
<INITIAL,NAME_STRING>scalebar                { return(SCALEBAR); }
<INITIAL,NAME_STRING>shadowcolor             { return(SHADOWCOLOR); }
<INITIAL,NAME_STRING>shadowsize              { return(SHADOWSIZE); }
<INITIAL,NAME_STRING>shapepath               { return(SHAPEPATH); }
<INITIAL,NAME_STRING>size                    { return(SIZE); }
<INITIAL,NAME_STRING>sizeunits               { return(SIZEUNITS); }
<INITIAL,NAME_STRING>status                  { return(STATUS); }
<INITIAL,NAME_STRING>style                   { return(STYLE); }
<INITIAL,NAME_STRING>styleitem               { return(STYLEITEM); }
<INITIAL,NAME_STRING>symbol                  { return(SYMBOL); }
<INITIAL,NAME_STRING>symbolscale             { return(SYMBOLSCALE); }
<INITIAL,NAME_STRING>symbolset               { return(SYMBOLSET); }
<INITIAL,NAME_STRING>table                   { return(TABLE); }
<INITIAL,NAME_STRING>template                { return(TEMPLATE); }
<INITIAL,NAME_STRING>templatepattern         { return(TEMPLATEPATTERN); }
<INITIAL,NAME_STRING>text                    { return(TEXT); }
<INITIAL,NAME_STRING>tileindex               { return(TILEINDEX); }
<VALUE_STRING>tileindex                      { return(MS_LAYER_TILEINDEX); }
<INITIAL,NAME_STRING>tileitem                { return(TILEITEM); }
<INITIAL,NAME_STRING>title                   { return(TITLE); }
<INITIAL,NAME_STRING>to                      { return(TO); }
<INITIAL,NAME_STRING>tolerance               { return(TOLERANCE); }
<INITIAL,NAME_STRING>toleranceunits          { return(TOLERANCEUNITS); }
<INITIAL,NAME_STRING>transparency            { return(TRANSPARENCY); }
<INITIAL,NAME_STRING>transparent             { return(TRANSPARENT); }
<INITIAL,NAME_STRING>transform               { return(TRANSFORM); }
<INITIAL,NAME_STRING>type                    { return(TYPE); }
<INITIAL,NAME_STRING>units                   { return(UNITS); }
<INITIAL,NAME_STRING>web                     { return(WEB); }
<INITIAL,NAME_STRING>width                   { return(WIDTH); }
<INITIAL,NAME_STRING>wkt                     { return(WKT); }
<INITIAL,NAME_STRING>wrap                    { return(WRAP); }

<INITIAL,VALUE_STRING>annotation               { return(MS_LAYER_ANNOTATION); }
<INITIAL,VALUE_STRING>auto                     { return(MS_AUTO); }
<INITIAL>bevel                                 { return(MS_CJC_BEVEL); }
<INITIAL>bitmap                                { return(MS_BITMAP); }
<INITIAL>butt                                  { return(MS_CJC_BUTT); }
<INITIAL>cartoline                             { return(MS_SYMBOL_CARTOLINE); }
<INITIAL,VALUE_STRING>cc                       { return(MS_CC); }
<INITIAL,VALUE_STRING>circle                   { return(MS_LAYER_CIRCLE); }
<INITIAL,VALUE_STRING>cl                       { return(MS_CL); }
<INITIAL,VALUE_STRING>cr                       { return(MS_CR); }
<INITIAL,VALUE_STRING>csv                      { return(MS_DB_CSV); }
<INITIAL,VALUE_STRING>mysql                    { return(MS_DB_MYSQL); }
<INITIAL,VALUE_STRING>postgresql               { return(MS_DB_POSTGRES); }
<INITIAL,VALUE_STRING>default                  { return(MS_DEFAULT); }
<INITIAL,VALUE_STRING>dd                       { return(MS_DD); }
<INITIAL>ellipse                               { return(MS_SYMBOL_ELLIPSE); }
<INITIAL,VALUE_STRING>embed                    { return(MS_EMBED); }
<INITIAL,VALUE_STRING>false                    { return(MS_FALSE); }
<INITIAL,VALUE_STRING>feet                     { return(MS_FEET); }
<INITIAL,VALUE_STRING>follow                   { return(MS_FOLLOW); }
<INITIAL,VALUE_STRING>giant                    { return(MS_GIANT); }
<INITIAL>hatch                                 { return(MS_SYMBOL_HATCH); }
<INITIAL,VALUE_STRING>hilite                   { return(MS_HILITE); }
<INITIAL,VALUE_STRING>inches                   { return(MS_INCHES); }
<INITIAL,VALUE_STRING>kilometers               { return(MS_KILOMETERS); }
<INITIAL,VALUE_STRING>large                    { return(MS_LARGE); }
<INITIAL,VALUE_STRING>lc                       { return(MS_LC); }
<INITIAL,VALUE_STRING>line                     { return(MS_LAYER_LINE); }
<INITIAL,VALUE_STRING>ll                       { return(MS_LL); }
<INITIAL,VALUE_STRING>lr                       { return(MS_LR); }
<INITIAL,VALUE_STRING>medium                   { return(MS_MEDIUM); }
<INITIAL,VALUE_STRING>meters                   { return(MS_METERS); }
<INITIAL,VALUE_STRING>miles                    { return(MS_MILES); }
<INITIAL,VALUE_STRING>miter                    { return(MS_CJC_MITER); }
<INITIAL,VALUE_STRING>multiple                 { return(MS_MULTIPLE); }

<INITIAL>none                                  { return(MS_CJC_NONE); }
<INITIAL,VALUE_STRING>normal                   { return(MS_NORMAL); }
<INITIAL,VALUE_STRING>off                      { return(MS_OFF); }
<INITIAL,VALUE_STRING>ogr                      { return(MS_OGR); }
<INITIAL,VALUE_STRING>on                       { return(MS_ON); }
<INITIAL,VALUE_STRING>one-to-one               { return(MS_JOIN_ONE_TO_ONE); }
<INITIAL,VALUE_STRING>one-to-many              { return(MS_JOIN_ONE_TO_MANY); }
<INITIAL,VALUE_STRING>oraclespatial            { return(MS_ORACLESPATIAL); }
<INITIAL,VALUE_STRING>percentages              { return(MS_PERCENTAGES); }
<INITIAL>pixmap                                { return(MS_SYMBOL_PIXMAP); }
<INITIAL,VALUE_STRING>pixels                   { return(MS_PIXELS); }
<INITIAL,VALUE_STRING>point                    { return(MS_LAYER_POINT); }
<INITIAL,VALUE_STRING>polygon                  { return(MS_LAYER_POLYGON); }
<INITIAL,VALUE_STRING>postgis                  { return(MS_POSTGIS); }
<INITIAL,VALUE_STRING>mygis                    { return(MS_MYGIS); }
<INITIAL,VALUE_STRING>plugin                   { return(MS_PLUGIN); }
<INITIAL,VALUE_STRING>query                    { return(MS_LAYER_QUERY); }
<INITIAL,VALUE_STRING>raster                   { return(MS_LAYER_RASTER); }
<INITIAL,VALUE_STRING>round                    { return(MS_CJC_ROUND); }
<INITIAL,VALUE_STRING>sde                      { return(MS_SDE); }
<INITIAL,VALUE_STRING>selected                 { return(MS_SELECTED); }
<INITIAL>simple                                { return(MS_SYMBOL_SIMPLE); }
<INITIAL,VALUE_STRING>single                   { return(MS_SINGLE); }
<INITIAL,VALUE_STRING>small                    { return(MS_SMALL); }
<INITIAL,VALUE_STRING>square                   { return(MS_CJC_SQUARE); }
<INITIAL,VALUE_STRING>tiny                     { return(MS_TINY); }
<INITIAL,VALUE_STRING>triangle                 { return(MS_CJC_TRIANGLE); }
<INITIAL,VALUE_STRING>true                     { return(MS_TRUE); }
<INITIAL,VALUE_STRING>truetype                 { return(MS_TRUETYPE); }
<INITIAL,VALUE_STRING>uc                       { return(MS_UC); }
<INITIAL,VALUE_STRING>ul                       { return(MS_UL); }
<INITIAL,VALUE_STRING>ur                       { return(MS_UR); }
<INITIAL>vector                                { return(MS_SYMBOL_VECTOR); }
<INITIAL,VALUE_STRING>wfs                      { return(MS_WFS); }
<INITIAL,VALUE_STRING>wms                      { return(MS_WMS); }
<INITIAL,VALUE_STRING>alpha                    { return(MS_GD_ALPHA); }

<INITIAL,VALUE_STRING>\[.*\]                   {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 return(MS_BINDING);
                                               }

<NAME_STRING>[0-9]+                            {
                                                 msyynumber = atof(msyytext);
                                                 return(MS_NUMBER);
                                               }

<INITIAL,VALUE_STRING>-?[0-9]+|-?[0-9]+\.[0-9]*|-?\.[0-9]*|-?[0-9]+[eE][+-]?[0-9]+|-?[0-9]+\.[0-9]*[eE][+-]?[0-9]+|-?\.[0-9]*[eE][+-]?[0-9]+ {
                                                 msyynumber = atof(msyytext);
                                                 return(MS_NUMBER); 
                                               }

<EXPRESSION_STRING>-?[0-9]+|-?[0-9]+\.[0-9]*|-?\.[0-9]*|-?[0-9]+[eE][+-]?[0-9]+|-?[0-9]+\.[0-9]*[eE][+-]?[0-9]+|-?\.[0-9]*[eE][+-]?[0-9]+ { 
                                                 msyylval.dblval = atof(msyytext); 
                                                 return(NUMBER); 
                                               }

<EXPRESSION_STRING>\`[^\`]*\`                  {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 msTimeInit(&msyylval.tmval);
                                                 msParseTime(msyytext, &msyylval.tmval);
                                                 /* TODO: need to trap bad date formats somehow, should return a parse error (need an error token in mapparser.y)*/
                                                 return(TIME);
                                               }

<INITIAL,VALUE_STRING>\/[^\/]*\/i              {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-2] = '\0';
                                                 return(MS_IREGEX);
                                               }

<EXPRESSION_STRING>\/[^\/]*\/i                 {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-2] = '\0';
                                                 msyylval.strval = strdup(msyytext);
                                                 return(IREGEX);
                                               }

<INITIAL,VALUE_STRING>\/[^\/]*\/               {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 return(MS_REGEX);
                                               }

<EXPRESSION_STRING>\/[^\/]*\/                  {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 msyylval.strval = strdup(msyytext);
                                                 return(REGEX);
                                               }

<INITIAL,VALUE_STRING>\(.*\)                   {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 return(MS_EXPRESSION);
                                               }

<INITIAL,VALUE_STRING>\"[^\"]*\"i|\'[^\']*\'i  {                                                 
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-2] = '\0';
                                                 return(MS_ISTRING);
                                               }

<INCLUDE>\"[^\"]*\"|\'[^\']*\'                 {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';

                                                 if(include_stack_ptr >= MAX_INCLUDE_DEPTH) {
                                                   msSetError(MS_IOERR, "Includes nested to deeply.", "msyylex()");
                                                   return(-1);
                                                 }

                                                 include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;

                                                 msyyin = fopen(msBuildPath(path, msyybasepath, msyytext), "r");
                                                 if(!msyyin) {
                                                   msSetError(MS_IOERR, "Error opening included file \"%s\".", "msyylex()", msyytext);
                                                   return(-1);
                                                 }

                                                 msyy_switch_to_buffer( msyy_create_buffer(msyyin, YY_BUF_SIZE) );

                                                 BEGIN(INITIAL);
                                               }

<INITIAL,VALUE_STRING>\"[^\"]*\"|\'[^\']*\'    {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 return(MS_STRING);
                                               }

<EXPRESSION_STRING>\"[^\"]*\"i|\'[^\']*\'i     {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-2] = '\0';
                                                 /* msyylval.strval = strdup(msyytext);*/
                                                 msyylval.strval = msyytext;
                                                 return(ISTRING);
                                               }

<EXPRESSION_STRING>\"[^\"]*\"|\'[^\']*\'       {
                                                 msyytext++;
                                                 msyytext[strlen(msyytext)-1] = '\0';
                                                 /* msyylval.strval = strdup(msyytext);*/
                                                 msyylval.strval = msyytext;
                                                 return(STRING);
                                               }

<INITIAL>[a-z/\.][a-z0-9/\._\-\=]*             { return(MS_STRING); }
<NAME_STRING>[a-z/\.][a-z0-9/\.\-\=]*        { return(MS_STRING); }
<VALUE_STRING>[a-z/\.][a-z0-9/\. \-\=]*        { return(MS_STRING); }

<INITIAL>\n                                    { msyylineno++; }

<INITIAL><<EOF>>                               {
                                                 if( --include_stack_ptr < 0 )
                                                   return(EOF); /* end of main file */
                                                 else {
                                                   fclose(YY_CURRENT_BUFFER->yy_input_file);
                                                   msyy_delete_buffer( YY_CURRENT_BUFFER );
                                                   msyy_switch_to_buffer(include_stack[include_stack_ptr]);
                                                 }
                                               }

<EXPRESSION_STRING,VALUE_STRING,NAME_STRING>[\r|\n|\0] { 
                                                 return(0); 
                                               }

<INITIAL,NAME_STRING,VALUE_STRING>.          { return(0); }
<EXPRESSION_STRING>.                           { return(msyytext[0]); }
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

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
int msyystate=0;
char *msyystring=NULL;
char *msyybasepath=NULL;

int msyyreturncomments = 0;

void lexer_cleanup(void) {
#ifndef YY_CURRENT_BUFFER_LVALUE
    /* yy_current_buffer was there only in older versions (e.g. 2.5.4)
     * but is gone in newer versions (e.g. 2.5.31)
     */
    if( yy_current_buffer != NULL )
    {
        yy_delete_buffer( yy_current_buffer );
        yy_current_buffer = 0;
    }
#endif
}

#define MAX_INCLUDE_DEPTH 5
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;
char path[MS_MAXPATHLEN];
%}

%s OBJECT_STRING
%s VALUE_STRING
%s EXPRESSION_STRING
%s INCLUDE

%%
       switch(msyystate) {
       case(0):
         break;
       case(1):         
         BEGIN(OBJECT_STRING);         
         msyy_scan_string(msyystring);
         msyystate=0;
         msyyreturncomments=0; 
         (void) yyunput; /* just to avoid warning about it being unrefed */
         break;
       case(2):
         BEGIN(VALUE_STRING);
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyy_scan_string(msyystring);
         msyystate=0;
         msyyreturncomments=0; 
         break;
       case(3):
         BEGIN(INITIAL);
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyystate=0;
         msyystring=NULL;
         msyyreturncomments=0; 
         return(0);
         break;
       case(4):
         BEGIN(EXPRESSION_STRING);
         msyy_delete_buffer(YY_CURRENT_BUFFER);
         msyy_scan_string(msyystring);
         msyystate=0;
         msyyreturncomments=0; 
         break;
       case(5):
         BEGIN(INITIAL);        
         msyystate=0;
         msyystring=NULL;
         msyyreturncomments=0; 
         return(0);
         break;
       case(6):
         BEGIN(INITIAL);        
         msyystate=0;
         msyystring=NULL;
         msyyreturncomments=1;  /* Return comments in tokenizer mode */
         return(0);
         break;
       default:
         break;
       }

<INITIAL,VALUE_STRING,EXPRESSION_STRING,INCLUDE>[ \t\r]+             ;

<INITIAL>#.*                                   { if (msyyreturncomments) return(MS_COMMENT); }

<OBJECT_STRING>_+                                          ;

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


<INITIAL,OBJECT_STRING>colorrange              { return(COLORRANGE); }
<INITIAL,OBJECT_STRING>alphacolorrange	       { return(ALPHACOLORRANGE); }
<INITIAL,OBJECT_STRING>datarange               { return(DATARANGE); }
<INITIAL,OBJECT_STRING>rangeitem               { return(RANGEITEM); }

<INITIAL,OBJECT_STRING>alphacolor		       { return(ALPHACOLOR); }
<INITIAL,OBJECT_STRING>angle                   { return(ANGLE); }
<INITIAL,OBJECT_STRING>angleitem               { return(ANGLEITEM); }
<INITIAL,OBJECT_STRING>antialias               { return(ANTIALIAS); }
<INITIAL,OBJECT_STRING>backgroundcolor	       { return(BACKGROUNDCOLOR); }
<INITIAL,OBJECT_STRING>backgroundshadowcolor   { return(BACKGROUNDSHADOWCOLOR); }
<INITIAL,OBJECT_STRING>backgroundshadowsize    { return(BACKGROUNDSHADOWSIZE); }
<INITIAL,OBJECT_STRING>bandsitem               { return(BANDSITEM); }
<INITIAL,OBJECT_STRING>browseformat            { return(BROWSEFORMAT); }
<INITIAL,OBJECT_STRING>buffer                  { return(BUFFER); }
<INITIAL>character		               { return(CHARACTER); }
<INITIAL,OBJECT_STRING>class		       { return(CLASS); }
<INITIAL,OBJECT_STRING>classitem               { return(CLASSITEM); }
<INITIAL,OBJECT_STRING>color		       { return(COLOR); }
<INITIAL,OBJECT_STRING>config    	       { return(CONFIG); }
<INITIAL,OBJECT_STRING>connection	       { return(CONNECTION); }
<INITIAL,OBJECT_STRING>connectiontype	       { return(CONNECTIONTYPE); }
<INITIAL,OBJECT_STRING>data		       { return(DATA); }
<INITIAL,OBJECT_STRING>datapattern	       { return(DATAPATTERN); }
<INITIAL,OBJECT_STRING>debug		       { return(DEBUG); }
<INITIAL,OBJECT_STRING>driver		       { return(DRIVER); }
<INITIAL,OBJECT_STRING>dump		       { return(DUMP); }
<INITIAL,OBJECT_STRING>empty		       { return(EMPTY); }
<INITIAL,OBJECT_STRING>encoding		       { return(ENCODING); }
<INITIAL>end			               { return(END); }
<INITIAL,OBJECT_STRING>error		       { return(ERROR); }
<INITIAL,OBJECT_STRING>expression              { return(EXPRESSION); }
<INITIAL,OBJECT_STRING>extent		       { return(EXTENT); }
<INITIAL,OBJECT_STRING>extension	       { return(EXTENSION); }
<INITIAL,OBJECT_STRING>feature                 { return(FEATURE); }
<INITIAL>filled                                { return(FILLED); }
<INITIAL,OBJECT_STRING>filter                  { return(FILTER); }
<INITIAL,OBJECT_STRING>filteritem              { return(FILTERITEM); }
<INITIAL,OBJECT_STRING>footer		       { return(FOOTER); }
<INITIAL,OBJECT_STRING>font		       { return(FONT); }
<INITIAL>fontset		               { return(FONTSET); }
<INITIAL,OBJECT_STRING>force		       { return(FORCE); }
<INITIAL,OBJECT_STRING>formatoption	       { return(FORMATOPTION); }
<INITIAL,OBJECT_STRING>from                    { return(FROM); }
<INITIAL>gap                                   { return(GAP); }
<INITIAL,OBJECT_STRING>grid		       { return(GRID); }
<INITIAL,OBJECT_STRING>graticule               { return(GRATICULE); }
<INITIAL,OBJECT_STRING>group                   { return(GROUP); }
<INITIAL,OBJECT_STRING>header		       { return(HEADER); }
<INITIAL,OBJECT_STRING>image		       { return(IMAGE); }
<INITIAL,OBJECT_STRING>imagecolor	       { return(IMAGECOLOR); }
<INITIAL,OBJECT_STRING>imagetype	       { return(IMAGETYPE); }
<INITIAL,OBJECT_STRING>imagequality	       { return(IMAGEQUALITY); }
<INITIAL,OBJECT_STRING>imagemode	       { return(IMAGEMODE); }
<INITIAL>imagepath	                       { return(IMAGEPATH); }
<INITIAL>imageurl          		       { return(IMAGEURL); }
<INITIAL>include                               { BEGIN(INCLUDE); }
<INITIAL,OBJECT_STRING>index                   { return(INDEX); }
<INITIAL,OBJECT_STRING>interlace               { return(INTERLACE); }
<INITIAL,OBJECT_STRING>intervals               { return(INTERVALS); } 
<INITIAL,OBJECT_STRING>join                    { return(JOIN); }
<INITIAL,OBJECT_STRING>keyimage                { return(KEYIMAGE); }
<INITIAL,OBJECT_STRING>keysize                 { return(KEYSIZE); }
<INITIAL,OBJECT_STRING>keyspacing              { return(KEYSPACING); }
<INITIAL,OBJECT_STRING>label		       { return(LABEL); }
<INITIAL,OBJECT_STRING>labelangleitem          { return(LABELANGLEITEM); }
<INITIAL,OBJECT_STRING>labelcache	       { return(LABELCACHE); }
<INITIAL,OBJECT_STRING>labelformat             { return(LABELFORMAT); }
<INITIAL,OBJECT_STRING>labelitem               { return(LABELITEM); }
<INITIAL,OBJECT_STRING>labelmaxscale	       { return(LABELMAXSCALE); }
<INITIAL,OBJECT_STRING>labelminscale	       { return(LABELMINSCALE); }
<INITIAL,OBJECT_STRING>labelrequires           { return(LABELREQUIRES); }
<INITIAL,OBJECT_STRING>labelsizeitem           { return(LABELSIZEITEM); }
<INITIAL,OBJECT_STRING>latlon	               { return(LATLON); }
<INITIAL,OBJECT_STRING>layer		       { return(LAYER); }
<INITIAL,OBJECT_STRING>legend		       { return(LEGEND); }
<INITIAL,OBJECT_STRING>legendformat	       { return(LEGENDFORMAT); }
<INITIAL>linecap                               { return(LINECAP); }
<INITIAL>linejoin                              { return(LINEJOIN); }
<INITIAL>linejoinmaxsize                       { return(LINEJOINMAXSIZE); }
<INITIAL>log		                       { return(LOG); }
<INITIAL,OBJECT_STRING>map	               { return(MAP); }
<INITIAL,OBJECT_STRING>marker	               { return(MARKER); }
<INITIAL,OBJECT_STRING>markersize              { return(MARKERSIZE); }
<INITIAL,OBJECT_STRING>maxarcs		       { return(MAXARCS); }
<INITIAL,OBJECT_STRING>maxboxsize              { return(MAXBOXSIZE); }
<INITIAL,OBJECT_STRING>maxfeatures             { return(MAXFEATURES); }
<INITIAL,OBJECT_STRING>maxinterval	       { return(MAXINTERVAL); }
<INITIAL,OBJECT_STRING>maxscale		       { return(MAXSCALE); }
<INITIAL,OBJECT_STRING>maxsize	               { return(MAXSIZE); }
<INITIAL,OBJECT_STRING>maxsubdivide	       { return(MAXSUBDIVIDE); }
<INITIAL,OBJECT_STRING>maxtemplate	       { return(MAXTEMPLATE); }
<INITIAL,OBJECT_STRING>maxwidth                { return(MAXWIDTH); }
<INITIAL,OBJECT_STRING>metadata	               { return(METADATA); }
<INITIAL,OBJECT_STRING>mimetype	               { return(MIMETYPE); }
<INITIAL,OBJECT_STRING>minarcs		       { return(MINARCS); }
<INITIAL,OBJECT_STRING>minboxsize              { return(MINBOXSIZE); }
<INITIAL,OBJECT_STRING>mindistance             { return(MINDISTANCE); }
<INITIAL,OBJECT_STRING>minfeaturesize          { return(MINFEATURESIZE); }
<INITIAL,OBJECT_STRING>mininterval	       { return(MININTERVAL); }
<INITIAL,OBJECT_STRING>minscale		       { return(MINSCALE); }
<INITIAL,OBJECT_STRING>minsize		       { return(MINSIZE); }
<INITIAL,OBJECT_STRING>minsubdivide	       { return(MINSUBDIVIDE); }
<INITIAL,OBJECT_STRING>mintemplate	       { return(MINTEMPLATE); }
<INITIAL,OBJECT_STRING>minwidth                { return(MINWIDTH); }
<INITIAL,OBJECT_STRING>name		       { return(NAME); }
<INITIAL,OBJECT_STRING>offset                  { return(OFFSET); }
<INITIAL,OBJECT_STRING>offsite                 { return(OFFSITE); }
<INITIAL,OBJECT_STRING>outlinecolor	       { return(OUTLINECOLOR); }
<INITIAL,OBJECT_STRING>outputformat	       { return(OUTPUTFORMAT); }
<INITIAL,OBJECT_STRING>overlaybackgroundcolor  { return(OVERLAYBACKGROUNDCOLOR); }
<INITIAL,OBJECT_STRING>overlaycolor            { return(OVERLAYCOLOR); }
<INITIAL,OBJECT_STRING>overlaymaxsize          { return(OVERLAYMAXSIZE); }
<INITIAL,OBJECT_STRING>overlayminsize          { return(OVERLAYMINSIZE); }
<INITIAL,OBJECT_STRING>overlayoutlinecolor     { return(OVERLAYOUTLINECOLOR); }
<INITIAL,OBJECT_STRING>overlaysize             { return(OVERLAYSIZE); }
<INITIAL,OBJECT_STRING>overlaysymbol           { return(OVERLAYSYMBOL); }
<INITIAL,OBJECT_STRING>partials                { return(PARTIALS); }
<INITIAL,OBJECT_STRING>points                  { return(POINTS); }
<INITIAL,OBJECT_STRING>position		       { return(POSITION); }
<INITIAL,OBJECT_STRING>postlabelcache	       { return(POSTLABELCACHE); }
<INITIAL,OBJECT_STRING>processing	       { return(PROCESSING); }
<INITIAL,OBJECT_STRING>projection	       { return(PROJECTION); }
<INITIAL,OBJECT_STRING>queryformat	       { return(QUERYFORMAT); }
<INITIAL,OBJECT_STRING>querymap	               { return(QUERYMAP); }
<INITIAL,OBJECT_STRING>reference	       { return(REFERENCE); }
<INITIAL,OBJECT_STRING>relativeto	       { return(RELATIVETO); }
<INITIAL,OBJECT_STRING>requires	               { return(REQUIRES); }
<INITIAL,OBJECT_STRING>resolution	       { return(RESOLUTION); }
<INITIAL,OBJECT_STRING>scale		       { return(SCALE); }
<INITIAL,OBJECT_STRING>scalebar		       { return(SCALEBAR); }
<INITIAL,OBJECT_STRING>shadowcolor             { return(SHADOWCOLOR); }
<INITIAL,OBJECT_STRING>shadowsize              { return(SHADOWSIZE); }
<INITIAL,OBJECT_STRING>shapepath               { return(SHAPEPATH); }
<INITIAL,OBJECT_STRING>size		       { return(SIZE); }
<INITIAL,OBJECT_STRING>sizeitem		       { return(SIZEITEM); }
<INITIAL,OBJECT_STRING>sizeunits	       { return(SIZEUNITS); }
<INITIAL,OBJECT_STRING>status		       { return(STATUS); }
<INITIAL,OBJECT_STRING>style		       { return(STYLE); }
<INITIAL,OBJECT_STRING>styleitem	       { return(STYLEITEM); }
<INITIAL,OBJECT_STRING>symbol		       { return(SYMBOL); }
<INITIAL,OBJECT_STRING>symbolscale             { return(SYMBOLSCALE); }
<INITIAL,OBJECT_STRING>symbolset               { return(SYMBOLSET); }
<INITIAL,OBJECT_STRING>table                   { return(TABLE); }
<INITIAL,OBJECT_STRING>template		       { return(TEMPLATE); }
<INITIAL,OBJECT_STRING>templatepattern	       { return(TEMPLATEPATTERN); }
<INITIAL,OBJECT_STRING>text		       { return(TEXT); }
<INITIAL,OBJECT_STRING>tileindex	       { return(TILEINDEX); }
<VALUE_STRING>tileindex                        { return(MS_LAYER_TILEINDEX); }
<INITIAL,OBJECT_STRING>tileitem		       { return(TILEITEM); }
<INITIAL,OBJECT_STRING>title	               { return(TITLE); }
<INITIAL,OBJECT_STRING>to                      { return(TO); }
<INITIAL,OBJECT_STRING>tolerance               { return(TOLERANCE); }
<INITIAL,OBJECT_STRING>toleranceunits          { return(TOLERANCEUNITS); }
<INITIAL,OBJECT_STRING>transparency            { return(TRANSPARENCY); }
<INITIAL,OBJECT_STRING>transparent             { return(TRANSPARENT); }
<INITIAL,OBJECT_STRING>transform               { return(TRANSFORM); }
<INITIAL,OBJECT_STRING>type		       { return(TYPE); }
<INITIAL,OBJECT_STRING>units		       { return(UNITS); }
<INITIAL,OBJECT_STRING>web		       { return(WEB); }
<INITIAL,OBJECT_STRING>width                   { return(WIDTH); }
<INITIAL,OBJECT_STRING>wkt                     { return(WKT); }
<INITIAL,OBJECT_STRING>wrap                    { return(WRAP); }

<INITIAL,VALUE_STRING>annotation	       { return(MS_LAYER_ANNOTATION); }
<INITIAL,VALUE_STRING>auto                     { return(MS_AUTO); }
<INITIAL>bevel                                 { return(MS_CJC_BEVEL); }
<INITIAL>bitmap			               { return(MS_BITMAP); }
<INITIAL>butt                                  { return(MS_CJC_BUTT); }
<INITIAL>cartoline                             { return(MS_SYMBOL_CARTOLINE); }
<INITIAL,VALUE_STRING>cc                       { return(MS_CC); }
<INITIAL,VALUE_STRING>circle                   { return(MS_LAYER_CIRCLE); }
<INITIAL,VALUE_STRING>cl                       { return(MS_CL); }
<INITIAL,VALUE_STRING>cr                       { return(MS_CR); }
<INITIAL,VALUE_STRING>csv                      { return(MS_DB_CSV); }
<INITIAL,VALUE_STRING>mysql                    { return(MS_DB_MYSQL); }
<INITIAL,VALUE_STRING>postgresql               { return(MS_DB_POSTGRES); }
<INITIAL,VALUE_STRING>default		       { return(MS_DEFAULT); }
<INITIAL,VALUE_STRING>dd                       { return(MS_DD); }
<INITIAL>ellipse                               { return(MS_SYMBOL_ELLIPSE); }
<INITIAL,VALUE_STRING>embed		       { return(MS_EMBED); }
<INITIAL,VALUE_STRING>false                    { return(MS_FALSE); }
<INITIAL,VALUE_STRING>feet		       { return(MS_FEET); }
<INITIAL,VALUE_STRING>follow		       { return(MS_FOLLOW); }
<INITIAL,VALUE_STRING>giant		       { return(MS_GIANT); }
<INITIAL>hatch                                 { return(MS_SYMBOL_HATCH); }
<INITIAL,VALUE_STRING>hilite		       { return(MS_HILITE); }
<INITIAL,VALUE_STRING>inches		       { return(MS_INCHES); }
<INITIAL,VALUE_STRING>kilometers	       { return(MS_KILOMETERS); }
<INITIAL,VALUE_STRING>large		       { return(MS_LARGE); }
<INITIAL,VALUE_STRING>lc                       { return(MS_LC); }
<INITIAL,VALUE_STRING>line		       { return(MS_LAYER_LINE); }
<INITIAL,VALUE_STRING>ll                       { return(MS_LL); }
<INITIAL,VALUE_STRING>lr                       { return(MS_LR); }
<INITIAL,VALUE_STRING>medium		       { return(MS_MEDIUM); }
<INITIAL,VALUE_STRING>meters		       { return(MS_METERS); }
<INITIAL,VALUE_STRING>miles		       { return(MS_MILES); }
<INITIAL,VALUE_STRING>miter		       { return(MS_CJC_MITER); }
<INITIAL,VALUE_STRING>multiple                 { return(MS_MULTIPLE); }

<INITIAL>none                                  { return(MS_CJC_NONE); }
<INITIAL,VALUE_STRING>normal                   { return(MS_NORMAL); }
<INITIAL,VALUE_STRING>off		       { return(MS_OFF); }
<INITIAL,VALUE_STRING>ogr		       { return(MS_OGR); }
<INITIAL,VALUE_STRING>on		       { return(MS_ON); }
<INITIAL,VALUE_STRING>one-to-one	       { return(MS_JOIN_ONE_TO_ONE); }
<INITIAL,VALUE_STRING>one-to-many	       { return(MS_JOIN_ONE_TO_MANY); }
<INITIAL,VALUE_STRING>oraclespatial	       { return(MS_ORACLESPATIAL); }
<INITIAL,VALUE_STRING>percentages              { return(MS_PERCENTAGES); }
<INITIAL>pixmap                                { return(MS_SYMBOL_PIXMAP); }
<INITIAL,VALUE_STRING>pixels                   { return(MS_PIXELS); }
<INITIAL,VALUE_STRING>point		       { return(MS_LAYER_POINT); }
<INITIAL,VALUE_STRING>polygon 		       { return(MS_LAYER_POLYGON); }
<INITIAL,VALUE_STRING>postgis		       { return(MS_POSTGIS); }
<INITIAL,VALUE_STRING>mygis		       { return(MS_MYGIS); }
<INITIAL,VALUE_STRING>plugin                   { return(MS_PLUGIN); }
<INITIAL,VALUE_STRING>query                    { return(MS_LAYER_QUERY); }
<INITIAL,VALUE_STRING>raster		       { return(MS_LAYER_RASTER); }
<INITIAL,VALUE_STRING>round		       { return(MS_CJC_ROUND); }
<INITIAL,VALUE_STRING>sde		       { return(MS_SDE); }
<INITIAL,VALUE_STRING>selected		       { return(MS_SELECTED); }
<INITIAL>simple                                { return(MS_SYMBOL_SIMPLE); }
<INITIAL,VALUE_STRING>single                   { return(MS_SINGLE); }
<INITIAL,VALUE_STRING>small		       { return(MS_SMALL); }
<INITIAL,VALUE_STRING>square		       { return(MS_CJC_SQUARE); }
<INITIAL,VALUE_STRING>tiny		       { return(MS_TINY); }
<INITIAL,VALUE_STRING>triangle		       { return(MS_CJC_TRIANGLE); }
<INITIAL,VALUE_STRING>true                     { return(MS_TRUE); }
<INITIAL,VALUE_STRING>truetype		       { return(MS_TRUETYPE); }
<INITIAL,VALUE_STRING>uc                       { return(MS_UC); }
<INITIAL,VALUE_STRING>ul                       { return(MS_UL); }
<INITIAL,VALUE_STRING>ur                       { return(MS_UR); }
<INITIAL>vector                                { return(MS_SYMBOL_VECTOR); }
<INITIAL,VALUE_STRING>wfs		       { return(MS_WFS); }
<INITIAL,VALUE_STRING>wms		       { return(MS_WMS); }
<INITIAL,VALUE_STRING>alpha		       { return(MS_GD_ALPHA); }

<OBJECT_STRING>[0-9]+                          {
                                                 msyynumber = atof(msyytext);
			                         return(MS_NUMBER);
			                       }

<INITIAL,VALUE_STRING>-?[0-9]+|-?[0-9]+\.[0-9]*|-?\.[0-9]*|-?[0-9]+[eE][+-]?[0-9]+|-?[0-9]+\.[0-9]*[eE][+-]?[0-9]+|-?\.[0-9]*[eE][+-]?[0-9]+ {
                                                                                                                                               msyynumber = atof(msyytext);
                                                                                                                                               return(MS_NUMBER); 
                                                                                                                                             }

<EXPRESSION_STRING>-?[0-9]+|-?[0-9]+\.[0-9]*|-?\.[0-9]*|-?[0-9]+[eE][+-]?[0-9]+|-?[0-9]+\.[0-9]*[eE][+-]?[0-9]+|-?\.[0-9]*[eE][+-]?[0-9]+    { 
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

<INITIAL>[a-z/\.][a-z0-9/\._\-\=]*	       { return(MS_STRING); }
<OBJECT_STRING>[a-z/\.][a-z0-9/\.\-\=]*	       { return(MS_STRING); }
<VALUE_STRING>[a-z/\.][a-z0-9/\. \-\=]*        { return(MS_STRING); }

<INITIAL>\n                                    { msyylineno++; }

<INITIAL><<EOF>>                               {
                                                 if( --include_stack_ptr < 0 )
                                                   return(EOF); /* end of main file */
                                                 else {
                                                   msyy_delete_buffer( YY_CURRENT_BUFFER );
                                                   msyy_switch_to_buffer(include_stack[include_stack_ptr]);
                                                 }
                                               }

<EXPRESSION_STRING,VALUE_STRING,OBJECT_STRING>[\r|\n|\0]          { return(0); }

<INITIAL,OBJECT_STRING,VALUE_STRING>.          { return(0); }
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

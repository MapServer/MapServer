%{

/* C declarations */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "map.h"
#include "mapfile.h"
#include "mapsymbol.h"
#include "mapparser.h"

double msyynumber;
int msyylineno = 1;

int msyystate=0;
char *msyystring=NULL;

int msyyreturncomments = 0;
%}

%s OBJECT_STRING
%s VALUE_STRING
%s EXPRESSION_STRING

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

<INITIAL,VALUE_STRING,EXPRESSION_STRING>[ \t\r]+             ;

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

<EXPRESSION_STRING>length                      { return(LENGTH); }

<INITIAL,OBJECT_STRING>angle                   { return(ANGLE); }
<INITIAL,OBJECT_STRING>antialias               { return(ANTIALIAS); }
<INITIAL,OBJECT_STRING>backgroundcolor	       { return(BACKGROUNDCOLOR); }
<INITIAL,OBJECT_STRING>backgroundshadowcolor   { return(BACKGROUNDSHADOWCOLOR); }
<INITIAL,OBJECT_STRING>backgroundshadowsize    { return(BACKGROUNDSHADOWSIZE); }

<INITIAL,OBJECT_STRING>buffer                  { return(BUFFER); }
<INITIAL>character		               { return(CHARACTER); }
<INITIAL,OBJECT_STRING>class		       { return(CLASS); }
<INITIAL,OBJECT_STRING>classitem               { return(CLASSITEM); }
<INITIAL,OBJECT_STRING>color		       { return(COLOR); }
<INITIAL,OBJECT_STRING>connection	       { return(CONNECTION); }
<INITIAL,OBJECT_STRING>connectiontype	       { return(CONNECTIONTYPE); }
<INITIAL,OBJECT_STRING>data		       { return(DATA); }
<INITIAL,OBJECT_STRING>driver		       { return(DRIVER); }
<INITIAL,OBJECT_STRING>dump		       { return(DUMP); }
<INITIAL,OBJECT_STRING>empty		       { return(EMPTY); }
<INITIAL>end			               { return(END); }
<INITIAL,OBJECT_STRING>error		       { return(ERROR); }
<INITIAL,OBJECT_STRING>expression              { return(EXPRESSION); }
<INITIAL>extent		                       { return(EXTENT); }
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
<INITIAL,OBJECT_STRING>graticule               { return(GRATICULE); }
<INITIAL,OBJECT_STRING>group                   { return(GROUP); }
<INITIAL,OBJECT_STRING>header		       { return(HEADER); }
<INITIAL>image			               { return(IMAGE); }
<INITIAL,OBJECT_STRING>imagecolor	       { return(IMAGECOLOR); }
<INITIAL,OBJECT_STRING>imagetype	       { return(IMAGETYPE); }
<INITIAL,OBJECT_STRING>imagequality	       { return(IMAGEQUALITY); }
<INITIAL,OBJECT_STRING>imagemode	       { return(IMAGEMODE); }
<INITIAL,OBJECT_STRING>imagepath	       { return(IMAGEPATH); }
<INITIAL,OBJECT_STRING>imageurl		       { return(IMAGEURL); }
<INITIAL,OBJECT_STRING>index                   { return(INDEX); }
<INITIAL,OBJECT_STRING>interlace               { return(INTERLACE); }
<INITIAL,OBJECT_STRING>intervals               { return(INTERVALS); } 
<INITIAL,OBJECT_STRING>join                    { return(JOIN); }
<INITIAL,OBJECT_STRING>keysize                 { return(KEYSIZE); }
<INITIAL,OBJECT_STRING>keyspacing              { return(KEYSPACING); }
<INITIAL,OBJECT_STRING>label		       { return(LABEL); }
<INITIAL,OBJECT_STRING>labelangleitem          { return(LABELANGLEITEM); }
<INITIAL,OBJECT_STRING>labelcache	       { return(LABELCACHE); }
<INITIAL,OBJECT_STRING>labelitem               { return(LABELITEM); }
<INITIAL,OBJECT_STRING>labelmaxscale	       { return(LABELMAXSCALE); }
<INITIAL,OBJECT_STRING>labelminscale	       { return(LABELMINSCALE); }
<INITIAL,OBJECT_STRING>labelrequires           { return(LABELREQUIRES); }
<INITIAL,OBJECT_STRING>labelsizeitem           { return(LABELSIZEITEM); }
<INITIAL,OBJECT_STRING>latlon	               { return(LATLON); }
<INITIAL,OBJECT_STRING>layer		       { return(LAYER); }
<INITIAL,OBJECT_STRING>legend		       { return(LEGEND); }
<INITIAL>linecap                               { return(LINECAP); }
<INITIAL>linejoin                              { return(LINEJOIN); }
<INITIAL>linejoinmaxsize                       { return(LINEJOINMAXSIZE); }
<INITIAL>log		                       { return(LOG); }
<INITIAL,OBJECT_STRING>map	               { return(MAP); }
<INITIAL,OBJECT_STRING>marker	               { return(MARKER); }
<INITIAL,OBJECT_STRING>markersize              { return(MARKERSIZE); }
<INITIAL,OBJECT_STRING>maxboxsize              { return(MAXBOXSIZE); }
<INITIAL,OBJECT_STRING>maxfeatures             { return(MAXFEATURES); }
<INITIAL,OBJECT_STRING>maxscale		       { return(MAXSCALE); }
<INITIAL,OBJECT_STRING>maxsize	               { return(MAXSIZE); }
<INITIAL,OBJECT_STRING>maxtemplate	       { return(MAXTEMPLATE); }
<INITIAL,OBJECT_STRING>metadata	               { return(METADATA); }
<INITIAL,OBJECT_STRING>mimetype	               { return(MIMETYPE); }
<INITIAL,OBJECT_STRING>minboxsize              { return(MINBOXSIZE); }
<INITIAL,OBJECT_STRING>mindistance             { return(MINDISTANCE); }
<INITIAL,OBJECT_STRING>minfeaturesize          { return(MINFEATURESIZE); }
<INITIAL,OBJECT_STRING>minscale		       { return(MINSCALE); }
<INITIAL,OBJECT_STRING>minsize		       { return(MINSIZE); }
<INITIAL,OBJECT_STRING>mintemplate	       { return(MINTEMPLATE); }
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
<INITIAL,OBJECT_STRING>projection	       { return(PROJECTION); }
<INITIAL,OBJECT_STRING>querymap	               { return(QUERYMAP); }
<INITIAL,OBJECT_STRING>reference	       { return(REFERENCE); }
<INITIAL,OBJECT_STRING>requires	               { return(REQUIRES); }
<INITIAL,OBJECT_STRING>resolution	       { return(RESOLUTION); }
<INITIAL,OBJECT_STRING>scale		       { return(SCALE); }
<INITIAL,OBJECT_STRING>scalebar		       { return(SCALEBAR); }
<INITIAL,OBJECT_STRING>shadowcolor             { return(SHADOWCOLOR); }
<INITIAL,OBJECT_STRING>shadowsize              { return(SHADOWSIZE); }
<INITIAL,OBJECT_STRING>shapepath               { return(SHAPEPATH); }
<INITIAL,OBJECT_STRING>size		       { return(SIZE); }
<INITIAL,OBJECT_STRING>sizeunits	       { return(SIZEUNITS); }
<INITIAL,OBJECT_STRING>status		       { return(STATUS); }
<INITIAL,OBJECT_STRING>style		       { return(STYLE); }
<INITIAL,OBJECT_STRING>styleitem	       { return(STYLEITEM); }
<INITIAL,OBJECT_STRING>symbol		       { return(SYMBOL); }
<INITIAL,OBJECT_STRING>symbolscale             { return(SYMBOLSCALE); }
<INITIAL,OBJECT_STRING>symbolset               { return(SYMBOLSET); }
<INITIAL,OBJECT_STRING>table                   { return(TABLE); }
<INITIAL,OBJECT_STRING>template		       { return(TEMPLATE); }
<INITIAL,OBJECT_STRING>text		       { return(TEXT); }
<INITIAL,OBJECT_STRING>tileindex	       { return(TILEINDEX); }
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
<INITIAL,VALUE_STRING>default		       { return(MS_DEFAULT); }
<INITIAL,VALUE_STRING>dd                       { return(MS_DD); }
<INITIAL>ellipse                               { return(MS_SYMBOL_ELLIPSE); }
<INITIAL,VALUE_STRING>embed		       { return(MS_EMBED); }
<INITIAL,VALUE_STRING>false                    { return(MS_FALSE); }
<INITIAL,VALUE_STRING>feet		       { return(MS_FEET); }
<INITIAL,VALUE_STRING>giant		       { return(MS_GIANT); }
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

<INITIAL,VALUE_STRING>normal                   { return(MS_NORMAL); }
<INITIAL,VALUE_STRING>off		       { return(MS_OFF); }
<INITIAL,VALUE_STRING>ogr		       { return(MS_OGR); }
<INITIAL,VALUE_STRING>on		       { return(MS_ON); }
<INITIAL,VALUE_STRING>oraclespatial	       { return(MS_ORACLESPATIAL); }
<INITIAL>pixmap                                { return(MS_SYMBOL_PIXMAP); }
<INITIAL,VALUE_STRING>pixels                   { return(MS_PIXELS); }
<INITIAL,VALUE_STRING>point		       { return(MS_LAYER_POINT); }
<INITIAL,VALUE_STRING>polygon 		       { return(MS_LAYER_POLYGON); }
<INITIAL,VALUE_STRING>postgis		       { return(MS_POSTGIS); }
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
<INITIAL,VALUE_STRING>wms		       { return(MS_WMS); }

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

<INITIAL,VALUE_STRING>\"[^\"]*\"	       {
			                         msyytext++;
			                         msyytext[strlen(msyytext)-1] = '\0';
     			                         return(MS_STRING);
			                       }

<INITIAL,VALUE_STRING>\'[^\']*\'	       {                                                 
                                                 msyytext++;
			                         msyytext[strlen(msyytext)-1] = '\0';
       			                         return(MS_STRING);
			                       }

<EXPRESSION_STRING>\"[^\"]*\"	               {
                                                 msyytext++;
			                         msyytext[strlen(msyytext)-1] = '\0';
			                         /* msyylval.strval = strdup(msyytext);*/
			                         msyylval.strval = msyytext;
			                         return(STRING);
			                       }

<EXPRESSION_STRING>\'[^\']*\'	               {
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

<INITIAL><<EOF>>                               { return(EOF); }

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

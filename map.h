#ifndef MAP_H
#define MAP_H

/*
** Main includes. If a particular header was needed by several .c files then
** I just put it here. What the hell, it works and it's all right here. -SDL-
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <direct.h>
#include <memory.h>
#include <malloc.h>
#else
#include <unistd.h>
#endif

#ifdef USE_MPATROL
#include "mpatrol.h"
#endif

#include "maperror.h"
#include "mapprimitive.h"
#include "mapshape.h"
#include "mapsymbol.h"
#include "maptree.h" // quadtree spatial index
#include "maphash.h"

#include "mapproject.h"


#include <gd.h>

#if defined USE_PDF
#include <pdflib.h>
#endif

#ifdef USE_MING_FLASH
#include "ming.h"
#endif

#include <sys/types.h> /* regular expression support */
#include <regex.h>

#ifdef USE_MING
#include "ming.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// General defines, wrapable

#define MS_VERSION "3.7 (development)"

#define MS_TRUE 1 /* logical control variables */
#define MS_FALSE 0
#define MS_ON 1
#define MS_OFF 0
#define MS_DEFAULT 2
#define MS_EMBED 3
#define MS_DELETE 4
#define MS_YES 1
#define MS_NO 0

#define MS_SINGLE 0 /* modes for searching (spatial/database) */
#define MS_MULTIPLE 1

// General defines, not wrapable
#ifndef SWIG
#define MS_MAPFILE_EXPR "\\.map$"
#define MS_TEMPLATE_EXPR "\\.(jsp|asp|cfm|xml|wml|html|htm|shtml|phtml)$"

#define MS_INDEX_EXTENSION ".qix"
#define MS_QUERY_EXTENSION ".qy"

#define MS_DEG_TO_RAD	.0174532925199432958
#define MS_RAD_TO_DEG   57.29577951

#define MS_RED 0
#define MS_GREEN 1
#define MS_BLUE 2

#define MS_MAXCOLORS 256

#define MS_BUFFER_LENGTH 2048 /* maximum input line length */
#define MS_URL_LENGTH 1024

#define MS_MAXIMGSIZE 1024

#define MS_MAXCLASSES 50
#define MS_MAXQUERIES MS_MAXCLASSES
#define MS_MAXPROJARGS 20
#define MS_MAXLAYERS 100 /* maximum number of layers in a map file */
#define MS_MAXJOINS 5
#define MS_ITEMNAMELEN 32
#define MS_NAMELEN 20

#define MS_MINSYMBOLSIZE 1   // in pixels
#define MS_MAXSYMBOLSIZE 100

#define MS_URL 0 /* template types */
#define MS_FILE 1

#define MS_MINFONTSIZE 4
#define MS_MAXFONTSIZE 256

#define MS_LABELCACHEINITSIZE 100
#define MS_LABELCACHEINCREMENT 10

#define MS_RESULTCACHEINITSIZE 10
#define MS_RESULTCACHEINCREMENT 10

#define MS_FEATUREINITSIZE 10 /* how many points initially can a feature have */
#define MS_FEATUREINCREMENT 10

#define MS_EXPRESSION 2000
#define MS_REGEX 2001
#define MS_STRING 2002
#define MS_NUMBER 2003
#define MS_COMMENT 2004

// General macro definitions
#define MS_MIN(a,b)     (((a)<(b))?(a):(b))
#define MS_MAX(a,b)	(((a)>(b))?(a):(b))
#define MS_ABS(a)	(((a)<0) ? -(a) : (a))
#define MS_SGN(a)	(((a)<0) ? -1 : 1)
#define MS_NINT(x)      ((x) >= 0.0 ? ((long) ((x)+.5)) : ((long) ((x)-.5)))

#define MS_PEN_UNSET	-4

#define MS_VALID_EXTENT(minx, miny, maxx, maxy)  (((minx<maxx) && (miny<maxy))?MS_TRUE:MS_FALSE)

#define MS_IMAGE_MIME_TYPE(format) (format->mimetype ? format->mimetype : "unknown")
#define MS_IMAGE_EXTENSION(format)  (format->extension ? format->extension : "unknown")

#define MS_DRIVER_GD(format)	(strncasecmp((format)->driver,"gd/",3)==0)
#define MS_DRIVER_SWF(format)	(strncasecmp((format)->driver,"swf",3)==0)
#define MS_DRIVER_GDAL(format)	(strncasecmp((format)->driver,"gdal/",5)==0)

#define MS_RENDER_WITH_GD	1
#define MS_RENDER_WITH_SWF    	2
#define MS_RENDER_WITH_RAWDATA 	3

#define MS_RENDERER_GD(format)	((format)->renderer == MS_RENDER_WITH_GD)
#define MS_RENDERER_SWF(format)	((format)->renderer == MS_RENDER_WITH_SWF)
#define MS_RENDERER_RAWDATA(format) ((format)->renderer == MS_RENDER_WITH_RAWDATA)

// ok, we'll switch to an UL cell model to make this work with WMS
#define MS_CELLSIZE(min,max,d)    ((max - min)/d)
#define MS_MAP2IMAGE_X(x,minx,cx) (MS_NINT((x - minx)/cx))
#define MS_MAP2IMAGE_Y(y,maxy,cy) (MS_NINT((maxy - y)/cy))
#define MS_IMAGE2MAP_X(x,minx,cx) (minx + cx*x)
#define MS_IMAGE2MAP_Y(y,maxy,cy) (maxy - cy*y)

// For maplabel and mappdf
#define LINE_VERT_THRESHOLD .17 // max absolute value of cos of line angle, the closer to zero the more vertical the line must be

// For CARTO symbols
#define MS_PI    3.14159265358979323846
#define MS_PI2   1.57079632679489661923  // (MS_PI / 2)
#define MS_3PI2  4.71238898038468985769  // (3 * MS_PI2)
#define MS_2PI   6.28318530717958647693  // (2 * MS_PI)

#endif

// General enumerated types - needed by scripts
enum MS_FILE_TYPE {MS_FILE_MAP, MS_FILE_SYMBOL};
enum MS_UNITS {MS_INCHES, MS_FEET, MS_MILES, MS_METERS, MS_KILOMETERS, MS_DD, MS_PIXELS};
enum MS_SHAPE_TYPE {MS_SHAPE_POINT, MS_SHAPE_LINE, MS_SHAPE_POLYGON, MS_SHAPE_NULL};
enum MS_LAYER_TYPE {MS_LAYER_POINT, MS_LAYER_LINE, MS_LAYER_POLYGON, MS_LAYER_RASTER, MS_LAYER_ANNOTATION, MS_LAYER_QUERY, MS_LAYER_CIRCLE};
enum MS_FONT_TYPE {MS_TRUETYPE, MS_BITMAP};
enum MS_LABEL_POSITIONS {MS_UL, MS_LR, MS_UR, MS_LL, MS_CR, MS_CL, MS_UC, MS_LC, MS_CC, MS_AUTO, MS_XY}; // arrangement matters for auto placement, don't change it
enum MS_BITMAP_FONT_SIZES {MS_TINY , MS_SMALL, MS_MEDIUM, MS_LARGE, MS_GIANT};
enum MS_QUERYMAP_STYLES {MS_NORMAL, MS_HILITE, MS_SELECTED};
enum MS_CONNECTION_TYPE {MS_INLINE, MS_SHAPEFILE, MS_TILED_SHAPEFILE, MS_SDE, MS_OGR, MS_UNUSED_1, MS_POSTGIS, MS_WMS, MS_ORACLESPATIAL};

enum MS_CAPS_JOINS_AND_CORNERS {MS_CJC_NONE, MS_CJC_BEVEL, MS_CJC_BUTT, MS_CJC_MITER, MS_CJC_ROUND, MS_CJC_SQUARE, MS_CJC_TRIANGLE}; 

enum MS_RETURN_VALUE {MS_SUCCESS, MS_FAILURE, MS_DONE};

enum MS_IMAGEMODE { MS_IMAGEMODE_PC256, MS_IMAGEMODE_RGB, MS_IMAGEMODE_RGBA, MS_IMAGEMODE_INT16, MS_IMAGEMODE_FLOAT32 };

#define MS_FILE_DEFAULT MS_FILE_MAP   
   

// FONTSET OBJECT - used to hold aliases for TRUETYPE fonts
#ifndef SWIG
typedef struct {
  char *filename;
  hashTableObj fonts;
  int numfonts;
} fontSetObj;
#endif

// FEATURE LIST OBJECT - for inline features, shape caches and queries
#ifndef SWIG
typedef struct listNode {
  shapeObj shape;
  struct listNode *next;
} featureListNodeObj;

typedef featureListNodeObj * featureListNodeObjPtr;
#endif

// COLOR OBJECT
typedef struct {
  int pen;
  int red;
  int green;
  int blue;
} colorObj;

#ifndef SWIG
// PALETTE OBJECT - used to hold colors while a map file is read
typedef struct {
  colorObj colors[MS_MAXCOLORS-1];
  int      colorvalue[MS_MAXCOLORS-1];
  int numcolors;
} paletteObj;
#endif

// EXPRESSION OBJECT
#ifndef SWIG
typedef struct {
  char *string;
  int type;

  // logical expression options
  char **items;
  int *indexes;
  int numitems;

  // regular expression options
  regex_t regex; // compiled regular expression to be matched
  int compiled;
} expressionObj;
#endif

#ifndef SWIG
// JOIN OBJECT - simple way to access other XBase files, one-to-one or one-to-many supported
typedef struct {
  char *name;

  char **items; /* array of item names */
  char numitems;
  char ***data; /* array of data for 1 or more records */
  int numrecords; /* number of records we have data for */

  char *table;
  char *from, *to;
  char *header;
#ifndef __cplusplus
  char *template;
#else
  char *_template;
#endif
  char *footer;

  char *match;

  int type;
} joinObj;
#endif

// OUTPUT FORMAT OBJECT - see mapoutput.c for most related code.
typedef struct {
  char *name;
  char *mimetype;
  char *driver;
  char *extension;
  int  renderer;  // MS_RENDER_WITH_*
  int  imagemode; // MS_IMAGEMODE_* value.
  int  transparent;
  int  numformatoptions;
  char **formatoptions;
  int  refcount;
} outputFormatObj;

// The following is used for "don't care" values in transparent, interlace and
// imagequality values. 
#define MS_NOOVERRIDE  -1111 

// QUERY MAP OBJECT - used to visualize query results
typedef struct {
  int height, width;
  int status;
  int style; /* HILITE, SELECTED or NORMAL */
  int color;
} queryMapObj;

// LABEL OBJECT - parameters needed to annotate a layer, legend or scalebar
typedef struct {
  char *font;
  enum MS_FONT_TYPE type;

  int color;
  int outlinecolor;

  int shadowcolor;
  int shadowsizex, shadowsizey;

  int backgroundcolor;
  int backgroundshadowcolor;
  int backgroundshadowsizex, backgroundshadowsizey;

  int size;
  int sizescaled;
  int minsize, maxsize;

  int position;
  int offsetx, offsety;

  double angle;
  int autoangle; // true or false

  int buffer; /* space to reserve around a label */

  int antialias;

  char wrap;

  int minfeaturesize; /* minimum feature size (in pixels) to label */
  int autominfeaturesize; // true or false

  int mindistance;
  int partials; /* can labels run of an image */

  int force; // labels *must* be drawn
} labelObj;

// WEB OBJECT - holds parameters for a mapserver/mapscript interface
typedef struct {
  char *log;
  char *imagepath, *imageurl;

#ifndef __cplusplus
  char *template;
#else
  char *_template;
#endif

  char *header, *footer;
  char *empty, *error; /* error handling */
  rectObj extent; /* clipping extent */
  double minscale, maxscale;
  char *mintemplate, *maxtemplate;

#ifndef SWIG
  hashTableObj metadata;
#endif

} webObj;

// STYLE OBJECT - holds parameters for symbolization, multiple styles may be applied within a classObj
typedef struct {
  int color;
  int backgroundcolor;
  int outlinecolor;

  int symbol;
  char *symbolname;

  int size;
  int sizescaled; // may not need this
  int minsize, maxsize;
} styleObj;

// CLASS OBJECT - basic symbolization and classification information
typedef struct {
#ifndef SWIG
  expressionObj expression; // the expression to be matched
#endif

  int status;

  int color;
  int backgroundcolor;
  int outlinecolor;
  int overlaycolor;
  int overlaybackgroundcolor;
  int overlayoutlinecolor;

  int symbol;
  char *symbolname;
  int overlaysymbol;
  char *overlaysymbolname;

  int size;
  int sizescaled;
  int minsize, maxsize;

  int overlaysize;
  int overlaysizescaled;
  int overlayminsize, overlaymaxsize;

  labelObj label;

  char *name; // should be unique within a layer
  char *title; // used for legend labeling

#ifndef SWIG
  expressionObj text;
#endif

#ifndef __cplusplus
  char *template;
#else
  char *_template;
#endif

  joinObj *joins;
  int numjoins;

  int type;

#ifndef SWIG
  hashTableObj metadata;
#endif

  double minscale, maxscale;

} classObj;

// LABELCACHE OBJECTS - structures to implement label caching and collision avoidance etc
// Note: These are scriptable, but are read only.
#ifdef SWIG
%readonly
#endif
typedef struct {
  char *string;
  double featuresize;

#ifndef __cplusplus
  classObj class; // we store the whole class cause lots of things might vary for each label
#else
  classObj _class;
#endif

  int layeridx; // indexes
  int classidx;
  int tileidx;
  int shapeidx;

  pointObj point; // label point
  shapeObj *poly; // label bounding box

  int status; /* has this label been drawn or not */
} labelCacheMemberObj;

typedef struct {
  int id; // corresponding label
  shapeObj *poly; // marker bounding box (POINT layers only)
} markerCacheMemberObj;

typedef struct {
  labelCacheMemberObj *labels;
  int numlabels;
  int cachesize;
  markerCacheMemberObj *markers;
  int nummarkers;
  int markercachesize;
} labelCacheObj;

typedef struct {
  long shapeindex;
  int tileindex;
  char classindex;
} resultCacheMemberObj;
#ifdef SWIG
%readwrite
#endif


typedef struct {

#ifndef SWIG
  resultCacheMemberObj *results;
  int cachesize;
#endif

#ifdef SWIG
%readonly
#endif
  int numresults;
  rectObj bounds;
#ifdef SWIG
%readwrite
#endif

} resultCacheObj;


// SYMBOLSET OBJECT
#ifndef SWIG
typedef struct {
  char *filename;

  fontSetObj *fontset; // a pointer to the main mapObj version

  int numsymbols;
  symbolObj symbol[MS_MAXSYMBOLS];

  struct imageCacheObj *imagecache;
  int imagecachesize;
} symbolSetObj;
#endif

// REFERENCE MAP OBJECT
typedef struct {
  rectObj extent;
  int height, width;
  colorObj color;
  colorObj outlinecolor;
  char *image;
  int status;
  int marker;
  char *markername;
  int markersize;
  int minboxsize;
  int maxboxsize;
} referenceMapObj;

// SCALEBAR OBJECT
typedef struct {
  colorObj imagecolor;
  int height, width;
  int style;
  int intervals;
  labelObj label;
  int color;
  int backgroundcolor;
  int outlinecolor;
  int units;
  int status; // ON, OFF or EMBED
  int position; // for embeded scalebars
  int transparent;
  int interlace;
  int postlabelcache;
} scalebarObj;

// LEGEND OBJECT
typedef struct {
  colorObj imagecolor;
  labelObj label;
  int keysizex, keysizey;
  int keyspacingx, keyspacingy;
  int outlinecolor; // Color of outline of box, -1 for no outline
  int status; // ON, OFF or EMBED
  int height, width;
  int position; // for embeded legends
  int transparent;
  int interlace;
  int postlabelcache;
#ifndef __cplusplus
   char *template;
#else
   char *_template;
#endif
} legendObj;

// LAYER OBJECT - basic unit of a map
typedef struct {
  int index;

  char *classitem; // .DBF item to be used for symbol lookup
  int classitemindex;

#ifndef SWIG
#ifndef __cplusplus
  classObj *class; // always at least 1 class
#else
  classObj *_class;
#endif
#endif

#ifdef SWIG
%readonly
#endif
  int numclasses;
#ifdef SWIG
%readwrite
#endif

  char *header, *footer; // only used with multi result queries

#ifndef __cplusplus
  char *template; // global template, used across all classes
#else
  char *_template;
#endif

  resultCacheObj *resultcache; // holds the results of a query against this layer

  char *name; // should be unique
  char *group; // shouldn't be unique it's supposed to be a group right?

  int status; // on or off
  char *data; // filename, can be relative or full path

  enum MS_LAYER_TYPE type;

  int annotate; // boolean flag for annotation

  double tolerance; // search buffer for point and line queries (in toleranceunits)
  int toleranceunits;

  double symbolscale; // scale at which symbols are default size
  double minscale, maxscale;
  double labelminscale, labelmaxscale;

  int sizeunits; // applies to all classes

  int maxfeatures;

  int offsite; // transparent pixel value for raster images

  int transform; // does this layer have to be transformed to file coordinates

  int labelcache, postlabelcache; // on or off

  char *labelitem, *labelsizeitem, *labelangleitem;
  int labelitemindex, labelsizeitemindex, labelangleitemindex;

  char *tileitem;
  int tileitemindex;
  char *tileindex; // layer index file for tiling support

#ifndef SWIG
  projectionObj projection; // projection information for the layer
#endif

  int units; // units of the projection

#ifndef SWIG
  featureListNodeObjPtr features; // linked list so we don't need a counter
  featureListNodeObjPtr currentfeature; // pointer to the current feature
#endif

  char *connection;
  enum MS_CONNECTION_TYPE connectiontype;

  // a variety of data connection objects (probably should be pointers!)
#ifndef SWIG
  shapefileObj shpfile;
  shapefileObj tileshpfile;
#endif

#ifndef SWIG
  void *ogrlayerinfo; // For OGR layers, will contain a msOGRLayerInfo struct
  void *sdelayerinfo; // For SDE layers, will contain a sdeLayerObj struct
  void *postgislayerinfo; // For PostGIS layers, this will contain a msPOSTGISLayerInfo struct
  void *oraclespatiallayerinfo;
#endif

  // attribute/classification handling components
  char **items;
  int numitems;

#ifndef SWIG
  void *iteminfo; // connection specific information necessary to retrieve values
#endif

#ifndef SWIG
  expressionObj filter; // connection specific attribute filter
#endif

  char *filteritem;
  int filteritemindex;

  char *styleitem; // item to be used for style lookup - can also be 'AUTO'
  int styleitemindex;

  char *requires; // context expressions, simple enough to not use expressionObj
  char *labelrequires;

#ifndef SWIG
  hashTableObj metadata;
#endif
  
  int transparency; // transparency value 0-100 

  int dump;
} layerObj;

// MAP OBJECT - encompasses everything used in an Internet mapping application
typedef struct { /* structure for a map */
  char *name; /* small identifier for naming etc. */
  int status; /* is map creation on or off */
  int height, width;

  layerObj *layers;

#ifdef SWIG
%readonly
#endif
  int numlayers; /* number of layers in mapfile */
#ifdef SWIG
%readwrite
#endif

#ifndef SWIG
  symbolSetObj symbolset;
  fontSetObj fontset;
#endif

  labelCacheObj labelcache; /* we need this here so multiple feature processors can access it */

  int transparent; // TODO - Deprecated
  int interlace; // TODO - Deprecated
  int imagequality; // TODO - Deprecated

  rectObj extent; /* map extent array */
  double cellsize; /* in map units */

  enum MS_UNITS units; /* units of the projection */
  double scale; /* scale of the output image */
  int resolution;

  char *shapepath; /* where are the shape files located */

  paletteObj palette; /* holds a map palette */
  colorObj imagecolor; /* holds the initial image color value */

  int numoutputformats;
  outputFormatObj **outputformatlist;
  outputFormatObj *outputformat;

#ifdef SWIG
%readonly
#endif
  char *imagetype; /* name of current outputformat */

#ifndef SWIG
  projectionObj projection; /* projection information for output map */
  projectionObj latlon; /* geographic projection definition */
#endif

  referenceMapObj reference;
  scalebarObj scalebar;
  legendObj legend;

  queryMapObj querymap;

  webObj web;

  int *layerorder;
   
} mapObj;

//SWF Object structure
#ifdef USE_MING_FLASH
typedef struct 
{
    mapObj *map;
    SWFMovie sMainMovie;
    int nLayerMovies;
    SWFMovie *pasMovies;
    int nCurrentMovie;
    int nCurrentLayerIdx;
    int nCurrentShapeIdx;
    
} SWFObj; 

#endif

// IMAGE OBJECT - a wrapper for GD images
typedef struct {
#ifdef SWIG
%readonly
#endif
  int width, height;
  char *imagepath, *imageurl;
#ifdef SWIG
%readwrite
#endif

  outputFormatObj *format;
  int              renderer;

#ifndef SWIG
  union
  {
      gdImagePtr gd;
#ifdef USE_MING_FLASH
      SWFObj     *swf;
#endif
      //PDF       *pdf;

      short       *raw_16bit;
      float       *raw_float;
  }img;

#endif
} imageObj;

// Function prototypes, wrapable
int msSaveImage(mapObj *map, imageObj *img, char *filename);
void msFreeImage(imageObj *img);


// Function prototypes, not wrapable

#ifndef SWIG
int msDrawSDELayer(mapObj *map, layerObj *layer, gdImagePtr img); /* in mapsde.c */

/*
** helper functions not part of the general API but needed in
** a few other places (like mapscript)... found in mapfile.c
*/
char *getString(void);
int getDouble(double *d);
int getInteger(int *i);
int getSymbol(int n, ...);
int getCharacter(char *c);

void initSymbol(symbolObj *s);
int initMap(mapObj *map);
int initLayer(layerObj *layer);
int initClass(classObj *_class);
void initLabel(labelObj *label);
void resetClassStyle(classObj *_class);

featureListNodeObjPtr insertFeatureList(featureListNodeObjPtr *list, shapeObj *shape);
void freeFeatureList(featureListNodeObjPtr list);

int msLoadProjectionString(projectionObj *p, char *value);

int loadExpressionString(expressionObj *exp, char *value);
void freeExpression(expressionObj *exp);

int getClassIndex(layerObj *layer, char *str);

// For maplabel and mappdf
int labelInImage(int width, int height, shapeObj *lpoly, int buffer);
int intersectLabelPolygons(shapeObj *p1, shapeObj *p2);
pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly);
double dist(pointObj a, pointObj b);


   
/*
** Main API Functions
*/
   
int msGetLayerIndex(mapObj *map, char *name); /* in mapfile.c */
int msGetSymbolIndex(symbolSetObj *set, char *name);
mapObj *msLoadMap(char *filename, char *new_map_path);
int msSaveMap(mapObj *map, char *filename);
void msFreeMap(mapObj *map);
void msFreeCharArray(char **array, int num_items);
int msLoadPalette(gdImagePtr img, paletteObj *palette, colorObj color);
int msUpdatePalette(gdImagePtr img, paletteObj *palette);
int msAddColor(mapObj *map, int red, int green, int blue);
int msLookupColor(mapObj *map, int color_index );
int msLoadMapString(mapObj *map, char *object, char *value);
void msFree(void *p);
char **msTokenizeMap(char *filename, int *numtokens);


#if defined USE_PDF
PDF *msDrawMapPDF(mapObj *map, PDF *pdf, hashTableObj fontHash); // mappdf.c
#endif

   
imageObj *msDrawScalebar(mapObj *map); // in mapscale.c
int msCalculateScale(rectObj extent, int units, int width, int height, int resolution, double *scale);
int msEmbedScalebar(mapObj *map, gdImagePtr img);


int msPointInRect(pointObj *p, rectObj *rect); // in mapsearch.c
int msRectOverlap(rectObj *a, rectObj *b);
int msRectContained(rectObj *a, rectObj *b);
void msMergeRect(rectObj *a, rectObj *b);
double msDistanceBetweenPoints(pointObj *a, pointObj *b);
double msDistanceFromPointToLine(pointObj *p, pointObj *a, pointObj *b);
int msPointInPolygon(pointObj *p, lineObj *c);
double msDistanceFromPointToMultipoint(pointObj *p, multipointObj *points);
double msDistanceFromPointToPolyline(pointObj *p, shapeObj *polyline);
double msDistanceFromPointToPolygon(pointObj *p, shapeObj *polyline);
int msIntersectMultipointPolygon(multipointObj *points, shapeObj *polygon);
int msIntersectPointPolygon(pointObj *p, shapeObj *polygon);
int msIntersectPolylinePolygon(shapeObj *line, shapeObj *poly);
int msIntersectPolygons(shapeObj *p1, shapeObj *p2);
int msIntersectPolylines(shapeObj *line1, shapeObj *line2);

int msSaveQuery(mapObj *map, char *filename); // in mapquery.c
int msLoadQuery(mapObj *map, char *filename);
int msQueryByIndex(mapObj *map, int qlayer, int tileindex, int shapeindex);
int msQueryByAttributes(mapObj *map, int qlayer, char *qitem, char *qstring, int mode);
int msQueryByPoint(mapObj *map, int qlayer, int mode, pointObj p, double buffer);
int msQueryByRect(mapObj *map, int qlayer, rectObj rect);
int msQueryByFeatures(mapObj *map, int qlayer, int slayer);
int msQueryByShape(mapObj *map, int qlayer, shapeObj *search_shape);
int msIsLayerQueryable(layerObj *lp);

int msJoinDBFTables(joinObj *join, char *path, char *tile);

void trimBlanks(char *string); // in mapstring.c
char *chop(char *string);
void trimEOL(char *string);
char *gsub(char *str, const char *old, const char *sznew);
char *stripPath(char *fn);
char *getPath(char *fn);
char **split(const char *string, char cd, int *num_tokens);
int countChars(char *str, char ch);
char *long2string(long value);
char *double2string(double value);
char *msEncodeUrl(char*);

#ifdef NEED_STRDUP
char *strdup(char *s);
#endif

#ifdef NEED_STRNCASECMP
int strncasecmp(char *s1, char *s2, int len);
#endif

#ifdef NEED_STRCASECMP
int strcasecmp(char *s1, char *s2);
#endif

int msLoadSymbolSet(symbolSetObj *symbolset); // in mapsymbol.c
void msInitSymbolSet(symbolSetObj *symbolset);
int msAddImageSymbol(symbolSetObj *symbolset, char *filename);
void msFreeSymbolSet(symbolSetObj *symbolset);



void msGetMarkerSize(symbolSetObj *symbolset, classObj *_class, int *width, int *height);
int getCharacterSize(char *character, int size, char *font, rectObj *rect);
void billboard(gdImagePtr img, shapeObj *shape, labelObj *label);
void freeImageCache(struct imageCacheObj *ic);

imageObj *msDrawLegend(mapObj *map); // in maplegend.c
int msEmbedLegend(mapObj *map, gdImagePtr img);
int msDrawLegendIcon(mapObj* map, layerObj* lp, classObj* myClass, int width, int height, gdImagePtr img, int dstX, int dstY);
gdImagePtr msCreateLegendIcon(mapObj* map, layerObj* lp, classObj* myClass, int width, int height);
   
int msLoadFontSet(fontSetObj *fontSet); // in maplabel.c

int msGetLabelSize(char *string, labelObj *label, rectObj *rect, fontSetObj *fontSet);
int msAddLabel(mapObj *map, int layeridx, int classidx, int tileidx, int shapeidx, pointObj point, char *string, double featuresize);

gdFontPtr msGetBitmapFont(int size);
int msImageTruetypePolyline(gdImagePtr img, shapeObj *p, symbolObj *s, int color, int size, fontSetObj *fontset);
int msImageTruetypeArrow(gdImagePtr img, shapeObj *p, symbolObj *s, int color, int size, fontSetObj *fontset);

  void msFreeShape(shapeObj *shape); // in mapprimative.c
void msInitShape(shapeObj *shape);
int msCopyShape(shapeObj *from, shapeObj *to);
void msComputeBounds(shapeObj *shape);
void msRectToPolygon(rectObj rect, shapeObj *poly);
void msClipPolylineRect(shapeObj *shape, rectObj rect);
void msClipPolygonRect(shapeObj *shape, rectObj rect);
void msTransformShape(shapeObj *shape, rectObj extent, double cellsize,
                      imageObj *image);
void msTransformShapeToPixel(shapeObj *shape, rectObj extent, double cellsize);
void msImageScanline(gdImagePtr img, int x1, int x2, int y, int c);
void msImagePolyline(gdImagePtr img, shapeObj *p, int c);
void msImageFilledCircle(gdImagePtr im, pointObj *p, int r, int c);
void msImageFilledPolygon(gdImagePtr img, shapeObj *p, int c);
void msImageCartographicPolyline(gdImagePtr im, shapeObj *p, int c, double sz, int captype, int jointype, double joinmaxsize);
int msPolylineLabelPoint(shapeObj *p, pointObj *lp, int min_length, double *angle, double *length);
int msPolygonLabelPoint(shapeObj *p, pointObj *lp, int min_dimension);
int msAddLine(shapeObj *p, lineObj *new_line);

int msDrawRasterLayer(mapObj *map, layerObj *layer, imageObj *image); // in mapraster.c
imageObj *msDrawReferenceMap(mapObj *map);

size_t msGetBitArraySize(int numbits); // in mapbits.c
char *msAllocBitArray(int numbits);
int msGetBit(char *array, int index);
void msSetBit(char *array, int index, int value);
void msFlipBit(char *array, int index);

int msLayerOpen(layerObj *layer, char *shapepath); // in maplayer.c
void msLayerClose(layerObj *layer);
int msLayerWhichShapes(layerObj *layer, rectObj rect);
int msLayerWhichItems(layerObj *layer, int classify, int annotate, char *metadata);
int msLayerNextShape(layerObj *layer, shapeObj *shape);
int msLayerGetItems(layerObj *layer);
int msLayerSetItems(layerObj *layer, char **items, int numitems);
int msLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record);
int msLayerGetExtent(layerObj *layer, rectObj *extent);
int msLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c,
                        int tile, long record);

int msTiledSHPOpenFile(layerObj *layer, char *shapepath); // in mapshape.c
int msTiledSHPWhichShapes(layerObj *layer, rectObj rect);
int msTiledSHPNextShape(layerObj *layer, shapeObj *shape);
int msTiledSHPGetShape(layerObj *layer, shapeObj *shape, int tile, long record);
void msTiledSHPClose(layerObj *layer);

int msOGRLayerOpen(layerObj *layer);   // in mapogr.cpp
int msOGRLayerClose(layerObj *layer);
int msOGRLayerWhichShapes(layerObj *layer, rectObj rect);
int msOGRLayerNextShape(layerObj *layer, shapeObj *shape);
int msOGRLayerGetItems(layerObj *layer);
int msOGRLayerInitItemInfo(layerObj *layer);
void msOGRLayerFreeItemInfo(layerObj *layer);
int msOGRLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record);
int msOGRLayerGetExtent(layerObj *layer, rectObj *extent);
int msOGRLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c,
                           int tile, long record);

int msPOSTGISLayerOpen(layerObj *layer); // in mappostgis.c
void msPOSTGISLayerFreeItemInfo(layerObj *layer);
int msPOSTGISLayerInitItemInfo(layerObj *layer);
int msPOSTGISLayerWhichShapes(layerObj *layer, rectObj rect);
int msPOSTGISLayerClose(layerObj *layer);
int msPOSTGISLayerNextShape(layerObj *layer, shapeObj *shape);
int msPOSTGISLayerGetShape(layerObj *layer, shapeObj *shape, long record);
int msPOSTGISLayerGetExtent(layerObj *layer, rectObj *extent);
int msPOSTGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record);
int msPOSTGISLayerGetItems(layerObj *layer);

int msSDELayerOpen(layerObj *layer); // in mapsde.c
void msSDELayerClose(layerObj *layer);
int msSDELayerWhichShapes(layerObj *layer, rectObj rect);
int msSDELayerNextShape(layerObj *layer, shapeObj *shape);
int msSDELayerGetItems(layerObj *layer);
int msSDELayerGetShape(layerObj *layer, shapeObj *shape, long record);
int msSDELayerGetExtent(layerObj *layer, rectObj *extent);
int msSDELayerInitItemInfo(layerObj *layer);
void msSDELayerFreeItemInfo(layerObj *layer);
char *msSDELayerGetSpatialColumn(layerObj *layer);
char *msSDELayerGetRowIDColumn(void);

int msOracleSpatialLayerOpen(layerObj *layer);
int msOracleSpatialLayerClose(layerObj *layer);
int msOracleSpatialLayerWhichShapes(layerObj *layer, rectObj rect);
int msOracleSpatialLayerNextShape(layerObj *layer, shapeObj *shape);
int msOracleSpatialLayerGetItems(layerObj *layer);
int msOracleSpatialLayerGetShape(layerObj *layer, shapeObj *shape, long record);
int msOracleSpatialLayerGetExtent(layerObj *layer, rectObj *extent);
int msOracleSpatialLayerInitItemInfo(layerObj *layer);
void msOracleSpatialLayerFreeItemInfo(layerObj *layer);
int msOracleSpatialLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c, int tile, long record);   
   
int msWMSDispatch(mapObj *map, char **names, char **values, int numentries); // mapwms.c

int msDrawWMSLayer(mapObj *map, layerObj *lp, imageObj *image); 
char *msWMSGetFeatureInfoURL(mapObj *map, layerObj *lp,
                             int nClickX, int nClickY, int nFeatureCount,
                             const char *pszInfoFormat); 

int msGMLWriteQuery(mapObj *map, char *filename); // mapgml.c


/* ==================================================================== */
/*      Prototypes for functions in mapdraw.c                           */
/* ==================================================================== */
imageObj *msDrawMap(mapObj *map);

imageObj *msDrawQueryMap(mapObj *map);

int msDrawLayer(mapObj *map, layerObj *layer, imageObj *image);

int msDrawVectorLayer(mapObj *map, layerObj *layer, imageObj *image);

int msDrawQueryLayer(mapObj *map, layerObj *layer, imageObj *image);

int msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, 
                imageObj *image, int overlay);

int msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, imageObj *image, 
                int classindex, char *labeltext);

void msCircleDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, 
                            pointObj *p, double r,  int sy, int fc, 
                            int bc, double sz);
void msCircleDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, 
                             pointObj *p, double r, int sy, int fc, int bc, 
                             int oc, double sz);

void msDrawMarkerSymbol(symbolSetObj *symbolset,imageObj *image, pointObj *p,  
                        int sy, int fc, int bc, int oc, double sz);

int msDrawLabel(imageObj *image, pointObj labelPnt, char *string, 
                labelObj *label, fontSetObj *fontset);

int draw_text(imageObj *image, pointObj labelPnt, char *string, 
              labelObj *label, fontSetObj *fontset);

int msDrawLabelCache(imageObj *image, mapObj *map);

void msDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, 
                      int sy, int fc, int bc, double sz);

void msDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, 
                       int sy, int fc, int bc, int oc, double sz);

void msImageStartLayer(mapObj *map, layerObj *layer, imageObj *image);
void msImageEndLayer(mapObj *map, layerObj *layer, imageObj *image);


void msDrawStartShape(mapObj *map, layerObj *layer, imageObj *image,  
                      shapeObj *shape);
void msDrawEndShape(mapObj *map, layerObj *layer, imageObj *image,
                    shapeObj *shape);



/* ==================================================================== */
/*      End of Prototypes for functions in mapdraw.c                    */
/* ==================================================================== */


/* ==================================================================== */
/*      Prototypes for functions in mapgd.c                             */
/* ==================================================================== */
imageObj *msImageCreateGD(int width, int height, outputFormatObj *format,
                          char *imagepath, char *imageurl);
imageObj *msImageLoadGD( const char *filename );
void      msImageInitGD( imageObj *image, colorObj background );

int msSaveImageGD(gdImagePtr img, char *filename, outputFormatObj *format);
int msSaveImageGD_LL(gdImagePtr img, char *filename, int type,
                     int transparent, int interlace, int quality);

void msFreeImageGD(gdImagePtr img);


void msCircleDrawLineSymbolGD(symbolSetObj *symbolset, gdImagePtr img, 
                              pointObj *p, double r, int sy, int fc, int bc, 
                              double sz);

void msCircleDrawShadeSymbolGD(symbolSetObj *symbolset, gdImagePtr img, 
                               pointObj *p, double r, int sy, int fc, int bc, 
                               int oc, double sz);

void msDrawMarkerSymbolGD(symbolSetObj *symbolset, gdImagePtr img, 
                          pointObj *p, int sy, int fc, int bc, int oc, 
                          double sz);

int draw_textGD(gdImagePtr img, pointObj labelPnt, char *string, 
                labelObj *label, fontSetObj *fontset);

int msDrawLabelCacheGD(gdImagePtr img, mapObj *map);

void msDrawLineSymbolGD(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, 
                        int sy, int fc, int bc, double sz);

void msDrawShadeSymbolGD(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, 
                         int sy, int fc, int bc, int oc, double sz);

//in mapraster.c
int msDrawRasterLayerLow(mapObj *map, layerObj *layer, imageObj *image);

//in mapwmslayer.c
int msDrawWMSLayerLow(mapObj *map, layerObj *lp, imageObj *image);

/* ==================================================================== */
/*      End of prototypes for functions in mapgd.c                      */
/* ==================================================================== */


/* ==================================================================== */
/*      Prototypes for functions in maputil.c                           */
/* ==================================================================== */
// For mappdf
int getRgbColor(mapObj *map,int i,int *r,int *g,int *b); // maputil.c
int msEvalContext(mapObj *map, char *context);
int msEvalExpression(expressionObj *expression, int itemindex, char **items, 
                     int numitems);
int msShapeGetClass(layerObj *layer, shapeObj *shape, double scale);
char *msShapeGetAnnotation(layerObj *layer, shapeObj *shape);
int msAdjustImage(rectObj rect, int *width, int *height);
double msAdjustExtent(rectObj *rect, int width, int height);

int *msGetLayersIndexByGroup(mapObj *map, char *groupname, int *nCount);

//Functions to chnage the drawing order of the layers.
//Defined in maputil.c
int msMoveLayerUp(mapObj *map, int nLayerIndex);
int msMoveLayerDown(mapObj *map, int nLayerIndex);
int msSetLayersdrawingOrder(mapObj *self, int *panIndexes);

char *msGetProjectionString(projectionObj *proj);

// Measured shape utility functions.   
pointObj *getPointUsingMeasure(shapeObj *shape, double m);
pointObj *getMeasureUsingPoint(shapeObj *shape, pointObj *point);

char **msGetAllGroupNames(mapObj* map, int *numTok);
char *msTmpFile(const char *path, const char *ext);

imageObj *msImageCreate(int width, int height, outputFormatObj *format, 
                        char *imagepath, char *imageurl);

/* ==================================================================== */
/*      End of prototypes for functions in maputil.c                    */
/* ==================================================================== */
/* ==================================================================== */
/*      prototypes for functions in mapswf.c                            */
/* ==================================================================== */
#ifdef USE_MING_FLASH
imageObj *msImageCreateSWF(int width, int height, outputFormatObj *format,
                           char *imagepath, char *imageurl, mapObj *map);

void msImageStartLayerSWF(mapObj *map, layerObj *layer, imageObj *image);

int msDrawLabelSWF(imageObj *image, pointObj labelPnt, char *string, 
                   labelObj *label, fontSetObj *fontset);

int msDrawLabelCacheSWF(imageObj *image, mapObj *map);

void msDrawLineSymbolSWF(symbolSetObj *symbolset, imageObj *image, shapeObj *p, 
                         int sy, int fc, int bc, double sz);

void msDrawShadeSymbolSWF(symbolSetObj *symbolset, imageObj *image, 
                          shapeObj *p, int sy, int fc, int bc, int oc, 
                          double sz);

void msDrawMarkerSymbolSWF(symbolSetObj *symbolset, imageObj *image, 
                           pointObj *p, 
                           int sy, int fc, int bc, int oc, double sz);
int msDrawRasterLayerSWF(mapObj *map, layerObj *layer, imageObj *image);
int msDrawVectorLayerAsRasterSWF(mapObj *map, layerObj *layer, imageObj*image);

int msDrawWMSLayerSWF(mapObj *map, layerObj *layer, imageObj *image);

void msTransformShapeSWF(shapeObj *shape, rectObj extent, double cellsize);

int msSaveImageSWF(imageObj *image, char *filename);

void msFreeImageSWF(imageObj *image);

int draw_textSWF(imageObj *image, pointObj labelPnt, char *string, 
                 labelObj *label, fontSetObj *fontset);
void msDrawStartShapeSWF(mapObj *map, layerObj *layer, imageObj *image,
                         shapeObj *shape);
#endif


/* ==================================================================== */
/*      End of prototypes for functions in mapswf.c                     */
/* ==================================================================== */

/* ==================================================================== */
/*      prototypes for functions in mapoutput.c                         */
/* ==================================================================== */

void msApplyDefaultOutputFormats( mapObj * );
void msFreeOutputFormat( outputFormatObj * );
outputFormatObj *msSelectOutputFormat( mapObj *map, const char *imagetype );
void msApplyOutputFormat( outputFormatObj **target, outputFormatObj *format,
                          int transparent, int interlaced, int imagequality );
const char *msGetOutputFormatOption( outputFormatObj *format, 
                                     const char *optionkey, 
                                     const char *defaultresult );
outputFormatObj *msCreateDefaultOutputFormat( mapObj *map, 
                                              const char *driver );
int msPostMapParseOutputFormatSetup( mapObj *map );
void msSetOutputFormatOption( outputFormatObj *format, const char *key, 
                              const char *value );
void msGetOutputFormatMimeList( mapObj *map, char **mime_list, int max_mime );
outputFormatObj *msCloneOutputFormat( outputFormatObj *format );

#ifndef gdImageTrueColor
#  define gdImageTrueColor(x) (0)
#endif

/* ==================================================================== */
/*      prototypes for functions in mapgdal.c                           */
/* ==================================================================== */
int msSaveImageGDAL( mapObj *map, imageObj *image, char *filename );
int msInitDefaultGDALOutputFormat( outputFormatObj *format );

/* ==================================================================== */
/*      End of prototypes for functions in mapoutput.c                  */
/* ==================================================================== */

#endif

#ifdef __cplusplus
}
#endif

#endif /* MAP_H */

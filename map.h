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

#include <sys/types.h> /* regular expression support */
#include <regex.h>

#ifdef __cplusplus
extern "C" {
#endif

// General defines, wrapable

#define MS_VERSION "3.5 (pre-alpha)"

#define MS_TRUE 1 /* logical control variables */
#define MS_FALSE 0
#define MS_ON 1
#define MS_OFF 0
#define MS_DEFAULT 2
#define MS_EMBED 3
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

// General macro definitions
#define MS_MIN(a,b)     (((a)<(b))?(a):(b))
#define MS_MAX(a,b)	(((a)>(b))?(a):(b))
#define MS_ABS(a)	(((a)<0) ? -(a) : (a))
#define MS_SGN(a)	(((a)<0) ? -1 : 1)
#define MS_NINT(x)      ((x) >= 0.0 ? ((long) ((x)+.5)) : ((long) ((x)-.5)))

#define MS_VALID_EXTENT(minx, miny, maxx, maxy)  (((minx<maxx) && (miny<maxy))?MS_TRUE:MS_FALSE)

#define MS_IMAGE_MIME_TYPE(type)  ((type)==MS_GIF?"image/gif": \
                                   (type)==MS_PNG?"image/png": \
                                   (type)==MS_JPEG?"image/jpeg": \
                                   (type)==MS_GML?"application/xml": \
                                   (type)==MS_WBMP?"image/wbmp":"unknown")

#define MS_IMAGE_EXTENSION(type)  ((type)==MS_GIF?"gif": \
                                   (type)==MS_PNG?"png": \
                                   (type)==MS_JPEG?"jpg": \
                                   (type)==MS_WBMP?"wbmp":"unknown")

// ok, we'll switch to an UL cell model to make this work with WMS
#define MS_CELLSIZE(min,max,d)    ((max - min)/d)
#define MS_MAP2IMAGE_X(x,minx,cx) (MS_NINT((x - minx)/cx))
#define MS_MAP2IMAGE_Y(y,maxy,cy) (MS_NINT((maxy - y)/cy))
#define MS_IMAGE2MAP_X(x,minx,cx) (minx + cx*x)
#define MS_IMAGE2MAP_Y(y,maxy,cy) (maxy - cy*y)

// For maplabel and mappdf
#define LINE_VERT_THRESHOLD .17 // max absolute value of cos of line angle, the closer to zero the more vertical the line must be

#endif

// General enumerated types - needed by scripts
enum MS_FILE_TYPE {MS_FILE_MAP, MS_FILE_SYMBOL};
enum MS_UNITS {MS_INCHES, MS_FEET, MS_MILES, MS_METERS, MS_KILOMETERS, MS_DD, MS_PIXELS};
enum MS_SHAPE_TYPE {MS_SHAPE_POINT, MS_SHAPE_LINE, MS_SHAPE_POLYGON, MS_SHAPE_NULL};
enum MS_LAYER_TYPE {MS_LAYER_POINT, MS_LAYER_LINE, MS_LAYER_POLYGON, MS_LAYER_POLYLINE, MS_LAYER_RASTER, MS_LAYER_ANNOTATION, MS_LAYER_QUERY, MS_LAYER_ELLIPSE};
enum MS_FONT_TYPE {MS_TRUETYPE, MS_BITMAP};
enum MS_LABEL_POSITIONS {MS_UL, MS_LR, MS_UR, MS_LL, MS_CR, MS_CL, MS_UC, MS_LC, MS_CC, MS_AUTO, MS_XY}; // arrangement matters for auto placement, don't change it
enum MS_BITMAP_FONT_SIZES {MS_TINY , MS_SMALL, MS_MEDIUM, MS_LARGE, MS_GIANT};
enum MS_QUERYMAP_STYLES {MS_NORMAL, MS_HILITE, MS_SELECTED};
enum MS_CONNECTION_TYPE {MS_INLINE, MS_SHAPEFILE, MS_TILED_SHAPEFILE, MS_SDE, MS_OGR, MS_TILED_OGR, MS_POSTGIS, MS_WMS, MS_ORACLESPATIAL};
enum MS_OUTPUT_IMAGE_TYPE {MS_GIF, MS_PNG, MS_JPEG, MS_WBMP, MS_GML};

enum MS_RETURN_VALUE {MS_SUCCESS, MS_FAILURE, MS_DONE};

#define MS_FILE_DEFAULT MS_FILE_MAP

#ifndef SWIG
// FONTSET OBJECT - used to hold aliases for TRUETYPE fonts
typedef struct {
  char *filename;
  hashTableObj fonts;
  int numfonts;
} fontSetObj;
#endif

// FEATURE LIST OBJECT - for inline features, shape caches and queries
typedef struct listNode {
  shapeObj shape;
  struct listNode *next;
} featureListNodeObj;

typedef featureListNodeObj * featureListNodeObjPtr;

// COLOR OBJECT
typedef struct {
  int red;
  int green;
  int blue;
} colorObj;

#ifndef SWIG
// PALETTE OBJECT - used to hold colors while a map file is read
typedef struct {
  colorObj colors[MS_MAXCOLORS-1];
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
  hashTableObj metadata;
} webObj;

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
  char *name; /* used for legend labeling */
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

  hashTableObj metadata;
} classObj;

// LABELCACHE OBJECTS - structures to implement label caching and collision avoidance etc
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

typedef struct {
  resultCacheMemberObj *results;
  int numresults;
  int cachesize;
  rectObj bounds;
} resultCacheObj;

// SYMBOLSET OBJECT
typedef struct {
  char *filename;
#ifndef SWIG
  fontSetObj *fontset; // a pointer to the main mapObj version
#endif
  int numsymbols;
  symbolObj symbol[MS_MAXSYMBOLS];

  struct imageCacheObj *imagecache;
  int imagecachesize;
} symbolSetObj;

// REFERENCE MAP OBJECT
typedef struct {
  rectObj extent;
  int height, width;
  colorObj color;
  colorObj outlinecolor;
  char *image;
#ifndef SWIG
  int imagetype;
#endif
  int status;
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
  int outlinecolor; /* Color of outline of box, -1 for no outline */
  int status; // ON, OFF or EMBED
  int height, width;
  int position; // for embeded legends
  int transparent;
  int interlace;
  int postlabelcache;
} legendObj;

// LAYER OBJECT - basic unit of a map
typedef struct {
  int index;

  char *classitem; /* .DBF item to be used for symbol lookup */
  int classitemindex;

#ifndef __cplusplus
  classObj *class; /* always at least 1 class */
#else
  classObj *_class;
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
  char *tileindex; /* layer index file for tiling support */

  int units;
  projectionObj projection; /* projection information for the layer */

  featureListNodeObjPtr features; // linked list so we don't need a counter
  featureListNodeObjPtr currentfeature; // pointer to the current feature

  char *connection;
  enum MS_CONNECTION_TYPE connectiontype;

  // a variety of data connection objects (probably should be pointers!)
  shapefileObj shpfile;
  shapefileObj tileshpfile;

  void *ogrlayerinfo; // For OGR layers, will contain a msOGRLayerInfo struct
  void *sdelayerinfo; // For SDE layers, will contain a sdeLayerObj struct
  void *postgislayerinfo; // For PostGIS layers, this will contain a msPOSTGISLayerInfo struct
  void *oraclespatiallayerinfo;

  // attribute/classification handling components
  char **items;
  int numitems;
  void *iteminfo; // connection specific information necessary to retrieve values

  #ifndef SWIG
  expressionObj filter; // connection specific attribute filter
  #endif

  char *filteritem;
  int filteritemindex;

  char *styleitem; /* item to be used for style lookup - can also be 'AUTO' */
  int styleitemindex;

  char *requires; // context expressions, simple enough to not use expressionObj
  char *labelrequires;

  hashTableObj metadata;

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

  int transparent;
  int interlace;

  rectObj extent; /* map extent array */
  double cellsize; /* in map units */

  enum MS_UNITS units; /* units of the projection */
  double scale; /* scale of the output image */
  int resolution;

  char *shapepath; /* where are the shape files located */

  paletteObj palette; /* holds a map palette */
  colorObj imagecolor; /* holds the initial image color value */

  int imagetype, imagequality;

  projectionObj projection; /* projection information for output map */
  projectionObj latlon; /* geographic projection definition */

  referenceMapObj reference;
  scalebarObj scalebar;
  legendObj legend;

#ifndef SWIG
  queryMapObj querymap;
#endif

  webObj web;
} mapObj;

// Function prototypes, wrapable
int msSaveImage(gdImagePtr img, char *filename, int type, int transparent, int interlace, int quality);
void msFreeImage(gdImagePtr img);

// Function prototypes, not wrapable

#ifndef SWIG
int msDrawSDELayer(mapObj *map, layerObj *layer, gdImagePtr img); /* in mapsde.c */

/*
** helper functions not part of the general API but needed in
** a few other places (like mapscript)... found in mapfile.c
*/
char *getString();
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

// For mappdf
int getRgbColor(mapObj *map,int i,int *r,int *g,int *b); // maputil.c

/*
** Main API Functions
*/
int msGetLayerIndex(mapObj *map, char *name); /* in mapfile.c */
int *msGetLayersIndexByGroup(mapObj *map, char *groupname, int *nCount);
int msGetSymbolIndex(symbolSetObj *set, char *name);
mapObj *msLoadMap(char *filename);
int msSaveMap(mapObj *map, char *filename);
void msFreeMap(mapObj *map);
void msFreeCharArray(char **array, int num_items);
int msLoadPalette(gdImagePtr img, paletteObj *palette, colorObj color);
int msUpdatePalette(gdImagePtr img, paletteObj *palette);
int msAddColor(mapObj *map, int red, int green, int blue);
int msLoadMapString(mapObj *map, char *object, char *value);
void msFree(void *p);

int msEvalExpression(expressionObj *expression, int itemindex, char **items, int numitems); // in maputil.c
int msEvalContext(mapObj *map, char *context);
int msShapeGetClass(layerObj *layer, shapeObj *shape);
char *msShapeGetAnnotation(layerObj *layer, shapeObj *shape);
double msAdjustExtent(rectObj *rect, int width, int height);
int msAdjustImage(rectObj rect, int *width, int *height);
gdImagePtr msDrawMap(mapObj *map);
#if defined USE_PDF
PDF *msDrawMapPDF(mapObj *map, PDF *pdf, hashTableObj fontHash); // mappdf.c
#endif
gdImagePtr msDrawQueryMap(mapObj *map);
int msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, gdImagePtr img, int overlay);
int msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, gdImagePtr img, int classindex, char *text);
int msDrawLayer(mapObj *map, layerObj *layer, gdImagePtr img);
int msDrawQueryLayer(mapObj *map, layerObj *layer, gdImagePtr img);

gdImagePtr msDrawScalebar(mapObj *map); // in mapscale.c
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
int msQueryByAttributes(mapObj *map, int qlayer, int mode);
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
void msDrawShadeSymbol(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, int sy, int fc, int bc, int oc, double sz);
void msGetMarkerSize(symbolSetObj *symbolset, classObj *_class, int *width, int *height);
void msDrawMarkerSymbol(symbolSetObj *symbolset,gdImagePtr img, pointObj *p,  int sy, int fc, int bc, int oc, double sz);
void msDrawLineSymbol(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, int sy, int fc, int bc, int oc, double sz);

gdImagePtr msDrawLegend(mapObj *map); // in maplegend.c
int msEmbedLegend(mapObj *map, gdImagePtr img);

int msLoadFontSet(fontSetObj *fontSet); // in maplabel.c
int msDrawLabel(gdImagePtr img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset);
int msGetLabelSize(char *string, labelObj *label, rectObj *rect, fontSetObj *fontSet);
int msAddLabel(mapObj *map, int layeridx, int classidx, int tileidx, int shapeidx, pointObj point, char *string, double featuresize);
int msDrawLabelCache(gdImagePtr img, mapObj *map);
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
void msTransformShape(shapeObj *shape, rectObj extent, double cellsize);
void msImageScanline(gdImagePtr img, int x1, int x2, int y, int c);
void msImagePolyline(gdImagePtr img, shapeObj *p, int c);
void msImageFilledPolygon(gdImagePtr img, shapeObj *p, int c);
int msPolylineLabelPoint(shapeObj *p, pointObj *lp, int min_length, double *angle, double *length);
int msPolygonLabelPoint(shapeObj *p, pointObj *lp, int min_dimension);
int msAddLine(shapeObj *p, lineObj *new_line);

int msDrawRasterLayer(mapObj *map, layerObj *layer, gdImagePtr img); // in mapraster.c
gdImagePtr msDrawReferenceMap(mapObj *map);

size_t msGetBitArraySize(int numbits); // in mapbits.c
char *msAllocBitArray(int numbits);
int msGetBit(char *array, int index);
void msSetBit(char *array, int index, int value);
void msFlipBit(char *array, int index);

int msLayerOpen(layerObj *layer, char *shapepath); // in maplayer.c
void msLayerClose(layerObj *layer);
int msLayerWhichShapes(layerObj *layer, rectObj rect);
int msLayerWhichItems(layerObj *layer, int classify, int annotate);
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
char *msSDELayerGetRowIDColumn();

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
const char *msWMSGetEPSGProj(projectionObj *proj, hashTableObj metadata,
                             int bReturnOnlyFirstOne);

int msDrawWMSLayer(mapObj *map, layerObj *lp, gdImagePtr img); // mapwmslayer.c

int msGMLWriteQuery(mapObj *map, char *filename); // mapgml.c
int msGMLStart(FILE *stream, const char *prjElement, const char *prjURI, const char *schemaLocation, const char *prjDescription);
int msGMLCollectionStart(FILE *stream, const char *colName,  const char *colFID);
int msGMLWriteShape(FILE *stream, shapeObj *shape, char *name, char *srsName, char **items, char *tab);
int msGMLCollectionFinish(FILE *stream, const char *colName);
int msGMLFinish(FILE *stream, const char *prjElement);

#endif

#ifdef __cplusplus
}
#endif

#endif /* MAP_H */

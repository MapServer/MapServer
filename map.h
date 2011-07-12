/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Primary MapServer include file.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */


#ifndef MAP_H
#define MAP_H

/* $Id$
**
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
#include <stdint.h>
#endif

#ifdef _WIN32
#ifndef SIZE_MAX
#ifdef _WIN64 
#define SIZE_MAX _UI64_MAX
#else
#define SIZE_MAX UINT_MAX
#endif
#endif
#endif

#ifndef DISABLE_CVSID
#  define MS_CVSID(string)     static char ms_cvsid[] = string; \
static char *cvsid_aw(void) { return( cvsid_aw() ? ((char *) NULL) : ms_cvsid ); }
#else
#  define MS_CVSID(string)
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define MS_DLL_EXPORT     __declspec(dllexport)
#else
#define  MS_DLL_EXPORT
#endif

#if UINT_MAX == 65535
typedef long ms_int32;
#else
typedef int ms_int32;
#endif
    
#include "maperror.h"
#include "mapprimitive.h"
#include "mapshape.h"
#include "mapsymbol.h"
#include "maptree.h" /* quadtree spatial index */
#include "maphash.h"
#include "mapio.h"

#include "mapproject.h"
#include "cgiutil.h"

#include <gd.h>

#if defined USE_PDF
#include <pdflib.h>
#endif

#include <sys/types.h> /* regular expression support */

/* The regex lib from the system and the regex lib from PHP needs to
 * be separated here. We separate here via its directory location.
 */
#include "mapregex.h"


#ifdef USE_OGR
#include "ogr_api.h"
#endif


#if defined(_WIN32) && !defined(__CYGWIN__)
#define snprintf _snprintf
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* General defines, wrapable */

#define MS_VERSION "4.10.7"

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

/* For layer transparency, allows alpha transparent pixmaps to be used
   with RGB map images */
#define MS_GD_ALPHA 1000

/* Bounds on the lengths of layer, class, and style arrays are now exposed
   to mapscript (bug 1522) */
#define MS_MAXCLASSES 250
#define MS_MAXSTYLES 5
#define MS_MAXLAYERS 200

/* General defines, not wrapable */
#ifndef SWIG
#define MS_DEFAULT_MAPFILE_PATTERN "\\.map$"

#define MS_TEMPLATE_MAGIC_STRING "MapServer Template"
#define MS_TEMPLATE_EXPR "\\.(xml|wml|html|htm|svg|kml|gml|js|tmpl)$"

#define MS_INDEX_EXTENSION ".qix"
#define MS_QUERY_EXTENSION ".qy"

#define MS_DEG_TO_RAD .0174532925199432958
#define MS_RAD_TO_DEG   57.29577951

#define MS_RED 0
#define MS_GREEN 1
#define MS_BLUE 2

#define MS_MAXCOLORS 256

#define MS_BUFFER_LENGTH 2048 /* maximum input line length */
#define MS_URL_LENGTH 1024
#define MS_MAXPATHLEN 1024

#define MS_MAXIMAGESIZE_DEFAULT 2048

#define MS_MAXPROJARGS 20
#define MS_MAXJOINS 20
#define MS_ITEMNAMELEN 32
#define MS_NAMELEN 20

#define MS_MINSYMBOLSIZE 1   /* in pixels */
#define MS_MAXSYMBOLSIZE 500

#define MS_MINSYMBOLWIDTH 1   /* in pixels */
#define MS_MAXSYMBOLWIDTH 32

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
#define MS_IREGEX 2005
#define MS_ISTRING 2006

/* boolean options for the expression object. */
#define MS_EXP_INSENSITIVE 1

/* General macro definitions */
#define MS_MIN(a,b) (((a)<(b))?(a):(b))
#define MS_MAX(a,b) (((a)>(b))?(a):(b))
#define MS_ABS(a) (((a)<0) ? -(a) : (a))
#define MS_SGN(a) (((a)<0) ? -1 : 1)

#define MS_NINT_GENERIC(x) ((x) >= 0.0 ? ((long) ((x)+.5)) : ((long) ((x)-.5)))

/* see http://mega-nerd.com/FPcast/ for some discussion of fast
   conversion to nearest int.  We avoid lrint() for now because it
   would be hard to include math.h "properly". */

#if defined(WE_HAVE_THE_C99_LRINT) && !defined(USE_GENERIC_MS_NINT)
#   define MS_NINT(x) lrint(x)
/*#   define MS_NINT(x) lround(x) */
#elif defined(_MSC_VER) && defined(_WIN32) && !defined(USE_GENERIC_MS_NINT)
    static __inline long int MS_NINT (double flt) 
    {	int intgr;
  
    _asm
        {	fld flt
                    fistp intgr
                    } ;
			
    return intgr ;
    } 
#elif defined(i386) && defined(__GNUC_PREREQ) && !defined(USE_GENERIC_MS_NINT)
    static __inline long int MS_NINT( double __x ) 
    {
        long int __lrintres;                                                        
        __asm__ __volatile__                                                        
          ("fistpl %0"                                                              
           : "=m" (__lrintres) : "t" (__x) : "st");                                 
        return __lrintres;
    }
#else
#  define MS_NINT(x)      MS_NINT_GENERIC(x)
#endif


#define MS_PEN_TRANSPARENT -1
#define MS_PEN_UNSET     -4

/* #define MS_VALID_EXTENT(minx, miny, maxx, maxy)  (((minx<maxx) && (miny<maxy))?MS_TRUE:MS_FALSE) */
#define MS_VALID_EXTENT(rect)  (((rect.minx < rect.maxx && rect.miny < rect.maxy))?MS_TRUE:MS_FALSE)

#define MS_INIT_COLOR(color,r,g,b) { (color).red = r; (color).green = g; (color).blue = b; (color).pen = MS_PEN_UNSET; }
#define MS_VALID_COLOR(color) (((color).red==-1 || (color).green==-1 || (color).blue==-1)?MS_FALSE:MS_TRUE)
#define MS_TRANSPARENT_COLOR(color) (((color).red==-255 || (color).green==-255 || (color).blue==-255)?MS_TRUE:MS_FALSE)
#define MS_COMPARE_COLORS(a,b) (((a).red!=(b).red || (a).green!=(b).green || (a).blue!=(b).blue)?MS_FALSE:MS_TRUE)

#define MS_IMAGE_MIME_TYPE(format) (format->mimetype ? format->mimetype : "unknown")
#define MS_IMAGE_EXTENSION(format)  (format->extension ? format->extension : "unknown")

#define MS_DRIVER_GD(format)  (strncasecmp((format)->driver,"gd/",3)==0)
#define MS_DRIVER_SWF(format) (strncasecmp((format)->driver,"swf",3)==0)
#define MS_DRIVER_GDAL(format)  (strncasecmp((format)->driver,"gdal/",5)==0)
#define MS_DRIVER_PDF(format) (strncasecmp((format)->driver,"pdf",3)==0)
#define MS_DRIVER_IMAGEMAP(format)  (strncasecmp((format)->driver,"imagemap",8)==0)
#define MS_DRIVER_SVG(format) (strncasecmp((format)->driver,"svg",3)==0)


#define MS_RENDER_WITH_GD 1
#define MS_RENDER_WITH_SWF      2
#define MS_RENDER_WITH_RAWDATA  3
#define MS_RENDER_WITH_PDF  4
#define MS_RENDER_WITH_IMAGEMAP 5
#define MS_RENDER_WITH_SVG      6

#define MS_RENDERER_GD(format)  ((format)->renderer == MS_RENDER_WITH_GD)
#define MS_RENDERER_SWF(format) ((format)->renderer == MS_RENDER_WITH_SWF)
#define MS_RENDERER_RAWDATA(format) ((format)->renderer == MS_RENDER_WITH_RAWDATA)
#define MS_RENDERER_PDF(format) ((format)->renderer == MS_RENDER_WITH_PDF)
#define MS_RENDERER_IMAGEMAP(format) ((format)->renderer == MS_RENDER_WITH_IMAGEMAP)
#define MS_RENDERER_SVG(format) ((format)->renderer == MS_RENDER_WITH_SVG)

/* ok, we'll switch to an UL cell model to make this work with WMS */
#define MS_CELLSIZE(min,max,d)    ((max - min)/d)
#define MS_MAP2IMAGE_X(x,minx,cx) (MS_NINT((x - minx)/cx))
#define MS_MAP2IMAGE_Y(y,maxy,cy) (MS_NINT((maxy - y)/cy))
#define MS_IMAGE2MAP_X(x,minx,cx) (minx + cx*x)
#define MS_IMAGE2MAP_Y(y,maxy,cy) (maxy - cy*y)

/* this version of MS_MAP2IMAGE takes 1/cellsize and is much faster */
#define MS_MAP2IMAGE_X_IC(x,minx,icx) (MS_NINT((x - minx)*icx))
#define MS_MAP2IMAGE_Y_IC(y,maxy,icy) (MS_NINT((maxy - y)*icy))

/* For CARTO symbols */
#define MS_PI    3.14159265358979323846
#define MS_PI2   1.57079632679489661923  /* (MS_PI / 2) */
#define MS_3PI2  4.71238898038468985769  /* (3 * MS_PI2) */
#define MS_2PI   6.28318530717958647693  /* (2 * MS_PI) */

#define MS_ENCRYPTION_KEY_SIZE  16   /* Key size: 128 bits = 16 bytes */

#endif

/* General enumerated types - needed by scripts */
enum MS_FILE_TYPE {MS_FILE_MAP, MS_FILE_SYMBOL};
enum MS_UNITS {MS_INCHES, MS_FEET, MS_MILES, MS_METERS, MS_KILOMETERS, MS_DD, MS_PIXELS, MS_PERCENTAGES};
enum MS_SHAPE_TYPE {MS_SHAPE_POINT, MS_SHAPE_LINE, MS_SHAPE_POLYGON, MS_SHAPE_NULL};
enum MS_LAYER_TYPE {MS_LAYER_POINT, MS_LAYER_LINE, MS_LAYER_POLYGON, MS_LAYER_RASTER, MS_LAYER_ANNOTATION, MS_LAYER_QUERY, MS_LAYER_CIRCLE, MS_LAYER_TILEINDEX};
enum MS_FONT_TYPE {MS_TRUETYPE, MS_BITMAP};

#define MS_POSITIONS_LENGTH 12
enum MS_POSITIONS_ENUM {MS_UL=101, MS_LR, MS_UR, MS_LL, MS_CR, MS_CL, MS_UC, MS_LC, MS_CC, MS_AUTO, MS_XY, MS_FOLLOW}; /* arrangement matters for auto placement, don't change it.  Added MS_FOLLOW for bug #1620 implementation */

enum MS_BITMAP_FONT_SIZES {MS_TINY , MS_SMALL, MS_MEDIUM, MS_LARGE, MS_GIANT};
enum MS_QUERYMAP_STYLES {MS_NORMAL, MS_HILITE, MS_SELECTED};
enum MS_CONNECTION_TYPE {MS_INLINE, MS_SHAPEFILE, MS_TILED_SHAPEFILE, MS_SDE, MS_OGR, MS_UNUSED_1, MS_POSTGIS, MS_WMS, MS_ORACLESPATIAL, MS_WFS, MS_GRATICULE, MS_MYGIS, MS_RASTER, MS_PLUGIN };
enum MS_JOIN_CONNECTION_TYPE {MS_DB_XBASE, MS_DB_CSV, MS_DB_MYSQL, MS_DB_ORACLE, MS_DB_POSTGRES};
enum MS_JOIN_TYPE {MS_JOIN_ONE_TO_ONE, MS_JOIN_ONE_TO_MANY};

enum MS_CAPS_JOINS_AND_CORNERS {MS_CJC_NONE, MS_CJC_BEVEL, MS_CJC_BUTT, MS_CJC_MITER, MS_CJC_ROUND, MS_CJC_SQUARE, MS_CJC_TRIANGLE}; 
enum MS_RETURN_VALUE {MS_SUCCESS, MS_FAILURE, MS_DONE};
enum MS_IMAGEMODE { MS_IMAGEMODE_PC256, MS_IMAGEMODE_RGB, MS_IMAGEMODE_RGBA, MS_IMAGEMODE_INT16, MS_IMAGEMODE_FLOAT32, MS_IMAGEMODE_BYTE, MS_IMAGEMODE_NULL };

#define MS_FILE_DEFAULT MS_FILE_MAP   

#ifndef SWIG
/* Filter object */    
typedef enum 
{
    FILTER_NODE_TYPE_UNDEFINED = -1,
    FILTER_NODE_TYPE_LOGICAL = 0,
    FILTER_NODE_TYPE_SPATIAL = 1,
    FILTER_NODE_TYPE_COMPARISON = 2,
    FILTER_NODE_TYPE_PROPERTYNAME = 3,
    FILTER_NODE_TYPE_BBOX = 4,
    FILTER_NODE_TYPE_LITERAL = 5,
    FILTER_NODE_TYPE_BOUNDARY = 6,
    FILTER_NODE_TYPE_GEOMETRY_POINT = 7,
    FILTER_NODE_TYPE_GEOMETRY_LINE = 8,
    FILTER_NODE_TYPE_GEOMETRY_POLYGON = 9
} FilterNodeType;


typedef struct _FilterNode
{
    FilterNodeType      eType;
    char                *pszValue;
    void                *pOther;

    struct _FilterNode  *psLeftNode;
    struct _FilterNode  *psRightNode;

      
}FilterEncodingNode;


/* Label path object - used to hold path and bounds of curved labels - Bug #1620 implementation. */
typedef struct {
  multipointObj path;
  shapeObj bounds;
  double *angles;
} labelPathObj;
#endif /*SWIG*/

/* FONTSET OBJECT - used to hold aliases for TRUETYPE fonts */
    typedef struct {
#ifdef SWIG
        %immutable;
#endif
        char *filename; 
        int numfonts;
        hashTableObj fonts;
#ifdef SWIG
        %mutable;
#endif

#ifndef SWIG
        struct map_obj *map;
#endif
    } fontSetObj;

/* FEATURE LIST OBJECT - for inline features, shape caches and queries */
#ifndef SWIG
    typedef struct listNode {
        shapeObj shape;
        struct listNode *next;
        struct listNode *tailifhead; /* this is the tail node in the list, if this is the head element, otherwise NULL */
    } featureListNodeObj;

    typedef featureListNodeObj * featureListNodeObjPtr;
#endif

#ifndef SWIG
/* PALETTE OBJECT - used to hold colors while a map file is read */
    typedef struct {
        colorObj colors[MS_MAXCOLORS-1];
        int      colorvalue[MS_MAXCOLORS-1];
        int numcolors;
    } paletteObj;
#endif

/* EXPRESSION OBJECT */
#ifndef SWIG
    typedef struct {
        char *string;
        int type;
        /* container for expression options such as case-insensitiveness */
        /* This is a boolean container. */
        int flags;

        /* logical expression options */
        char **items;
        int *indexes;
        int numitems;

        /* regular expression options */
        ms_regex_t regex; /* compiled regular expression to be matched */
        int compiled;
    } expressionObj;
#endif

#ifndef SWIG
/* JOIN OBJECT - simple way to access other XBase files, one-to-one or one-to-many supported */
    typedef struct {
        char *name;
        char **items, **values; /* items/values (process 1 record at a time) */
        int numitems;

        char *table;
        char *from, *to; /* item names */

        void *joininfo; /* vendor specific (i.e. XBase, MySQL, etc.) stuff to allow for persistant access */
 
        char *header, *footer;
#ifndef __cplusplus
        char *template;
#else
        char *_template;
#endif

        enum MS_JOIN_TYPE type;
        char *connection;
        enum MS_JOIN_CONNECTION_TYPE connectiontype;
    } joinObj;
#endif

/* OUTPUT FORMAT OBJECT - see mapoutput.c for most related code. */
    typedef struct {
        char *name;
        char *mimetype;
        char *driver;
        char *extension;
        int  renderer;  /* MS_RENDER_WITH_* */
        int  imagemode; /* MS_IMAGEMODE_* value. */
        int  transparent;
        int  bands;
        int  numformatoptions;
        char **formatoptions;
        int  refcount;
        int inmapfile; /* boolean value for writing */
    } outputFormatObj;

/* The following is used for "don't care" values in transparent, interlace and
   imagequality values. */
#define MS_NOOVERRIDE  -1111 

/* QUERY MAP OBJECT - used to visualize query results */
    typedef struct {
        int height, width;
        int status;
        int style; /* HILITE, SELECTED or NORMAL */
        colorObj color;
    } queryMapObj;

/* LABEL OBJECT - parameters needed to annotate a layer, legend or scalebar */
    typedef struct {
        char *font;
        enum MS_FONT_TYPE type;

        colorObj color;
        colorObj outlinecolor;

        colorObj shadowcolor;
        int shadowsizex, shadowsizey;

        colorObj backgroundcolor;
        colorObj backgroundshadowcolor;
        int backgroundshadowsizex, backgroundshadowsizey;

        int size;
        int minsize, maxsize;

        int position;
        int offsetx, offsety;

        double angle;
        int autoangle; /* true or false */
        int autofollow;  /* true or false, bug #1620 implementation */

        int buffer; /* space to reserve around a label */

        int antialias;

        char wrap;

  int minfeaturesize; /* minimum feature size (in pixels) to label */
  int autominfeaturesize; /* true or false */

  int mindistance;
  int partials; /* can labels run of an image */

  int force; /* labels *must* be drawn */

  char *encoding;

} labelObj;

/* WEB OBJECT - holds parameters for a mapserver/mapscript interface */
typedef struct {
  char *log;
  char *imagepath, *imageurl;

#ifdef SWIG
%immutable;
#endif /* SWIG */
  struct map_obj *map;
#ifdef SWIG
%mutable;
#endif /* SWIG */

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

  char *queryformat; /* what format is the query to be returned, given as a MIME type */
  char *legendformat;
  char *browseformat;

#ifdef SWIG
%immutable;
#endif /* SWIG */
  hashTableObj metadata;
#ifdef SWIG
%mutable;
#endif /* SWIG */

} webObj;

/* STYLE OBJECT - holds parameters for symbolization, multiple styles may be applied within a classObj */
typedef struct {
  colorObj color;
  colorObj backgroundcolor;
  colorObj outlinecolor;

  /* Stuff to handle Color Range Styles */
  colorObj mincolor;
  colorObj maxcolor;
  double minvalue;
  double maxvalue;
  char *rangeitem;
  int rangeitemindex;
  
  int symbol;
  char *symbolname;

  int size;
  int minsize, maxsize;

  int width;
  int minwidth, maxwidth;

  int offsetx, offsety; /* for shadows, hollow symbols, etc... */

  double angle; /* for future use */

  int antialias;

#ifndef SWIG
  /* Whether the style is within a class MS_TRUE or MS_FALSE.
   * If true, then memory deallocation is handled by the class
   * if false, then memory deallocation must be handled by the
   * application.
   */
  int isachild;
#endif

  char *angleitem, *sizeitem;
#ifndef SWIG
  int angleitemindex, sizeitemindex;
#endif
} styleObj;

/* CLASS OBJECT - basic symbolization and classification information */
typedef struct class_obj{
#ifndef SWIG
  expressionObj expression; /* the expression to be matched */
#endif

  int status;

#ifndef SWIG
  styleObj *styles;
#endif

  int numstyles;

#ifdef SWIG
%immutable;
#endif /* SWIG */
  labelObj label;
#ifdef SWIG
%mutable;
#endif /* SWIG */

  char *name; /* should be unique within a layer */
  char *title; /* used for legend labeling */

#ifndef SWIG
  expressionObj text;
#endif /* not SWIG */

#ifndef __cplusplus
  char *template;
#else /* __cplusplus */
  char *_template;
#endif /* __cplusplus */

  int type;

#ifdef SWIG
%immutable;
#endif /* SWIG */
  hashTableObj metadata;
#ifdef SWIG
%mutable;
#endif /* SWIG */

  double minscale, maxscale;
#ifdef SWIG
%immutable;
#endif /* SWIG */
  struct layer_obj *layer;
#ifdef SWIG
%mutable;
#endif /* SWIG */
  int debug;

  char *keyimage;
} classObj;

/* LABELCACHE OBJECTS - structures to implement label caching and collision avoidance etc
   Note: These are scriptable, but are read only. */
#ifdef SWIG
%immutable;
#endif /* SWIG */
typedef struct {
  char *text;
  double featuresize;

  styleObj *styles; /* copied from the classObj, only present if there is a marker to be drawn */
  int numstyles;

  labelObj label; /* copied from the classObj */

  int layerindex; /* indexes */
  int classindex;
  int tileindex;
  int shapeindex;

  pointObj point; /* label point */
  shapeObj *poly; /* label bounding box */

  int status; /* has this label been drawn or not */

  labelPathObj *labelpath;  /* Path & bounds of curved labels.  Bug #1620 implementation */
  
} labelCacheMemberObj;

typedef struct {
  int id; /* corresponding label */
  shapeObj *poly; /* marker bounding box (POINT layers only) */
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
  int classindex;
} resultCacheMemberObj;
#ifdef SWIG
%mutable;
#endif /* SWIG */


typedef struct {

#ifndef SWIG
  resultCacheMemberObj *results;
  int cachesize;
#endif /* not SWIG */

#ifdef SWIG
%immutable;
#endif /* SWIG */
  int numresults;
  rectObj bounds;
#ifdef SWIG
%mutable;
#endif /* SWIG */

} resultCacheObj;


/* SYMBOLSET OBJECT */
typedef struct {
  char *filename;
  int imagecachesize;
#ifdef SWIG
  %immutable;
#endif /* SWIG */
  int numsymbols;
#ifdef SWIG
  %mutable;
#endif /* SWIG */
  symbolObj symbol[MS_MAXSYMBOLS];
#ifndef SWIG
  struct map_obj *map;
  fontSetObj *fontset; /* a pointer to the main mapObj version */
  struct imageCacheObj *imagecache;
#endif /* not SWIG */
} symbolSetObj;

/* REFERENCE MAP OBJECT */
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
#ifdef SWIG
%immutable;
#endif /* SWIG */
  struct map_obj *map;
#ifdef SWIG
%mutable;
#endif /* SWIG */
} referenceMapObj;

/* SCALEBAR OBJECT */
typedef struct {
  colorObj imagecolor;
  int height, width;
  int style;
  int intervals;
  labelObj label;
  colorObj color;
  colorObj backgroundcolor;
  colorObj outlinecolor;
  int units;
  int status; /* ON, OFF or EMBED */
  int position; /* for embeded scalebars */
#ifndef SWIG
  int transparent;
  int interlace;
#endif /* not SWIG */
  int postlabelcache;
} scalebarObj;

/* LEGEND OBJECT */
typedef struct {
  colorObj imagecolor;
#ifdef SWIG
        %immutable;
#endif
  labelObj label;
#ifdef SWIG
        %mutable;
#endif
  int keysizex, keysizey;
  int keyspacingx, keyspacingy;
  colorObj outlinecolor; /* Color of outline of box, -1 for no outline */
  int status; /* ON, OFF or EMBED */
  int height, width;
  int position; /* for embeded legends */
#ifndef SWIG
  int transparent;
  int interlace;
#endif /* not SWIG */
  int postlabelcache;
#ifndef __cplusplus
   char *template;
#else /* __cplusplus */
   char *_template;
#endif /* __cplusplus */
#ifdef SWIG
%immutable;
#endif /* SWIG */
  struct map_obj *map;
#ifdef SWIG
%mutable;
#endif /* SWIG */
} legendObj;

#ifndef SWIG
typedef struct
{
  double    dwhichlatitude;
  double    dwhichlongitude;
  double    dstartlatitude;
  double    dstartlongitude;
  double    dendlatitude;
  double    dendlongitude;
  double    dincrementlatitude;
  double    dincrementlongitude;
  double    minarcs;
  double    maxarcs;
  double    minincrement;
  double    maxincrement;
  double    minsubdivides;
  double    maxsubdivides;
  int     bvertical;
  int     blabelaxes;
  int     ilabelstate;
  int     ilabeltype;
  rectObj   extent;
  lineObj   *pboundinglines;
  pointObj  *pboundingpoints;
  char    *labelformat;
} graticuleObj;


struct layerVTable;
typedef struct layerVTable layerVTableObj;

#endif /*SWIG*/

/* LAYER OBJECT - basic unit of a map */
typedef struct layer_obj {

  char *classitem; /* .DBF item to be used for symbol lookup */

#ifndef SWIG
  int classitemindex;
  resultCacheObj *resultcache; /* holds the results of a query against this layer */
  int annotate; /* boolean flag for annotation */
  double scalefactor; /* computed, not set */
#ifndef __cplusplus
  classObj *class; /* always at least 1 class */
#else /* __cplusplus */
  classObj *_class;
#endif /* __cplusplus */
#endif /* not SWIG */

#ifdef SWIG
%immutable;
#endif /* SWIG */
  int numclasses;
  int index;
  struct map_obj *map;
#ifdef SWIG
%mutable;
#endif /* SWIG */

  char *header, *footer; /* only used with multi result queries */

#ifndef __cplusplus
  char *template; /* global template, used across all classes */
#else /* __cplusplus */
  char *_template;
#endif /* __cplusplus */

  char *name; /* should be unique */
  char *group; /* shouldn't be unique it's supposed to be a group right? */

  int status; /* on or off */
  char *data; /* filename, can be relative or full path */

  enum MS_LAYER_TYPE type;

  double tolerance; /* search buffer for point and line queries (in toleranceunits) */
  int toleranceunits;

  double symbolscale; /* scale at which symbols are default size */
  double minscale, maxscale;
  double labelminscale, labelmaxscale;
  int sizeunits; /* applies to all classes */

  int maxfeatures;

  colorObj offsite; /* transparent pixel value for raster images */

  int transform; /* does this layer have to be transformed to file coordinates */

  int labelcache, postlabelcache; /* on or off */

  char *labelitem, *labelsizeitem, *labelangleitem;
#ifndef SWIG
  int labelitemindex, labelsizeitemindex, labelangleitemindex;
#endif /* not SWIG */

  char *tileitem;
  char *tileindex; /* layer index file for tiling support */

#ifndef SWIG
  int tileitemindex;
  projectionObj projection; /* projection information for the layer */
  int project; /* boolean variable, do we need to project this layer or not */
#endif /* not SWIG */

  int units; /* units of the projection */

#ifndef SWIG
  featureListNodeObjPtr features; /* linked list so we don't need a counter */
  featureListNodeObjPtr currentfeature; /* pointer to the current feature */
#endif /* SWIG */

  char *connection;
  char *plugin_library;
  char *plugin_library_original; /* this is needed for mapfile writing */
  enum MS_CONNECTION_TYPE connectiontype;

#ifndef SWIG
  layerVTableObj *vtable;

  /* SDL has converted OracleSpatial, SDE, Graticules, MyGIS */
  void *layerinfo; /* all connection types should use this generic pointer to a vendor specific structure */
  void *wfslayerinfo; /* For WFS layers, will contain a msWFSLayerInfo struct */
#endif /* not SWIG */

  /* attribute/classification handling components */
#ifdef SWIG
%immutable;
#endif /* SWIG */
  int numitems;
#ifdef SWIG
%mutable;
#endif /* SWIG */

#ifndef SWIG
  char **items;
  void *iteminfo; /* connection specific information necessary to retrieve values */
  expressionObj filter; /* connection specific attribute filter */
  int bandsitemindex;
  int filteritemindex;
  int styleitemindex;
#endif /* not SWIG */

  char *bandsitem; /* which item in a tile contains bands to use (tiled raster data only) */
  char *filteritem;
  char *styleitem; /* item to be used for style lookup - can also be 'AUTO' */

  char *requires; /* context expressions, simple enough to not use expressionObj */
  char *labelrequires;

#ifdef SWIG
%immutable;
#endif /* SWIG */
  hashTableObj metadata;
#ifdef SWIG
%mutable;
#endif /* SWIG */

  int transparency; /* transparency value 0-100 */
  
  int dump;
  int debug;
#ifndef SWIG
  char **processing;
  joinObj *joins;
#endif /* not SWIG */
#ifdef SWIG
%immutable;
#endif /* SWIG */
  
#ifndef SWIGJAVA  
  rectObj extent;
#endif /* not SWIGJAVA */

  int numprocessing;
  int numjoins;
#ifdef SWIG
%mutable;
#endif /* SWIG */
} layerObj;


/* MAP OBJECT - encompasses everything used in an Internet mapping application */
typedef struct map_obj{ /* structure for a map */
  char *name; /* small identifier for naming etc. */
  int status; /* is map creation on or off */
  int height, width;
  int maxsize;

#ifndef SWIG
  layerObj *layers;
#endif /* SWIG */

#ifdef SWIG
%immutable;
#endif /* SWIG */
  int numlayers; /* number of layers in mapfile */

  symbolSetObj symbolset;
  fontSetObj fontset;

  labelCacheObj labelcache; /* we need this here so multiple feature processors can access it */
#ifdef SWIG
%mutable;
#endif /* SWIG */

  int transparent; /* TODO - Deprecated */
  int interlace; /* TODO - Deprecated */
  int imagequality; /* TODO - Deprecated */

  rectObj extent; /* map extent array */
  double cellsize; /* in map units */


#ifndef SWIG
  geotransformObj gt; /* rotation / geotransform */
#endif /*SWIG*/
  rectObj saved_extent;

  enum MS_UNITS units; /* units of the projection */
  double scale; /* scale of the output image */
  double resolution;

  char *shapepath; /* where are the shape files located */
  char *mappath; /* path of the mapfile, all path are relative to this path */

#ifndef SWIG
  paletteObj palette; /* holds a map palette */
#endif /*SWIG*/
  colorObj imagecolor; /* holds the initial image color value */

#ifdef SWIG
%immutable;
#endif /* SWIG */
  int numoutputformats;
  outputFormatObj **outputformatlist;
  outputFormatObj *outputformat;

  char *imagetype; /* name of current outputformat */
#ifdef SWIG
  %mutable;
#endif /* SWIG */

#ifndef SWIG
  projectionObj projection; /* projection information for output map */
  projectionObj latlon; /* geographic projection definition */
#endif /* not SWIG */

#ifdef SWIG
%immutable;
#endif /* SWIG */  
  referenceMapObj reference;
  scalebarObj scalebar;
  legendObj legend;

  queryMapObj querymap;

  webObj web;
#ifdef SWIG
%mutable;
#endif /* SWIG */

  int *layerorder;

  int debug;   

  char *datapattern, *templatepattern;   

#ifdef SWIG
%immutable;
#endif /* SWIG */
  hashTableObj configoptions;
#ifdef SWIG
%mutable;
#endif /* SWIG */

#ifndef SWIG
  /* Private encryption key information - see mapcrypto.c */
  int encryption_key_loaded;        /* MS_TRUE once key has been loaded */
  unsigned char encryption_key[MS_ENCRYPTION_KEY_SIZE]; /* 128bits encryption key */
#endif
} mapObj;



/* PDF Object structure */
#ifdef USE_PDF
typedef struct {
  mapObj *map;
  PDF *pdf;
  void    *imagetmp;  /* used when the FORMATOPTION "OUTPUT_TYPE=RASTER" */
} PDFObj; 
#endif

#ifndef SWIG
typedef struct  {
  mapObj *map;
  FILE *stream;
  char *filename;
  int streamclosed; /* track if a save image is done */
  int compressed; /*track if output is set to be svgz */
} SVGObj;
#endif /*SWIG*/

/* IMAGE OBJECT - a wrapper for GD images */
typedef struct {
#ifdef SWIG
%immutable;
#endif
  int width, height;
  char *imagepath, *imageurl;

  outputFormatObj *format;
#ifdef SWIG
%mutable;
#endif
  int renderer;
  int size;

#ifndef SWIG
  union {
    gdImagePtr gd;
#ifdef USE_MING_FLASH
    void *swf;
#endif
#ifdef USE_PDF
    PDFObj *pdf;
#endif

    SVGObj *svg;

    char *imagemap;
    short *raw_16bit;
    float *raw_float;
    unsigned char *raw_byte;
  } img;
#endif
} imageObj;


/* LAYER_VTABLE, contains function pointers to the layer operations 
 * If you add new functions to here, remember to update
 * populateVirtualTable in maplayer.c
 */
#ifndef SWIG
struct layerVTable {
    int (*LayerInitItemInfo)(layerObj *layer);
    void (*LayerFreeItemInfo)(layerObj *layer);
    int (*LayerOpen)(layerObj *layer);
    int (*LayerIsOpen)(layerObj *layer);
    int (*LayerWhichShapes)(layerObj *layer, rectObj rect);
    int (*LayerNextShape)(layerObj *layer, shapeObj *shape);
    int (*LayerGetShape)(layerObj *layer, shapeObj *shape, 
                         int tile, long record);
    int (*LayerClose)(layerObj *layer);
    int (*LayerGetItems)(layerObj *layer);
    int (*LayerGetExtent)(layerObj *layer, rectObj *extent);
    int (*LayerGetAutoStyle)(mapObj *map, layerObj *layer, classObj *c, 
                             int tile, long record);

    int (*LayerCloseConnection)(layerObj *layer);

    int (*LayerSetTimeFilter)(layerObj *layer, 
                              const char *timestring, 
                              const char *timefield);

    int (*LayerApplyFilterToLayer)(FilterEncodingNode *psNode, mapObj *map,
                                   int iLayerIndex, 
                                   int bOnlySpatialFilter);

    int (*LayerCreateItems)(layerObj *layer, int nt);
    int (*LayerGetNumFeatures)(layerObj *layer);
    char* (*LayerEscapeSQLParam)(layerObj *layer, const char* pszString);
    char* (*LayerEscapePropertyName)(layerObj *layer, const char* pszString);
};
#endif /*SWIG*/


/* Function prototypes, wrapable */
MS_DLL_EXPORT int msSaveImage(mapObj *map, imageObj *img, char *filename);
MS_DLL_EXPORT void msFreeImage(imageObj *img);
MS_DLL_EXPORT int msSetup(void);
MS_DLL_EXPORT void msCleanup(void);

/* Function prototypes, not wrapable */

#ifndef SWIG

/*
** helper functions not part of the general API but needed in
** a few other places (like mapscript)... found in mapfile.c
*/
int getString(char **s);
int getDouble(double *d);
int getInteger(int *i);
int getSymbol(int n, ...);
int getCharacter(char *c);

int msBuildPluginLibraryPath(char **dest, const char *lib_str, mapObj *map);

MS_DLL_EXPORT int  hex2int(char *hex);

MS_DLL_EXPORT void initSymbol(symbolObj *s);
MS_DLL_EXPORT int initMap(mapObj *map);
MS_DLL_EXPORT int initLayer(layerObj *layer, mapObj *map);
MS_DLL_EXPORT void freeLayer( layerObj * );
MS_DLL_EXPORT int initClass(classObj *_class);
MS_DLL_EXPORT void freeClass( classObj * );
MS_DLL_EXPORT void initLabel(labelObj *label);
MS_DLL_EXPORT void resetClassStyle(classObj *_class);
MS_DLL_EXPORT int initStyle(styleObj *style);
MS_DLL_EXPORT void initReferenceMap(referenceMapObj *ref);
MS_DLL_EXPORT void initScalebar(scalebarObj *scalebar);
MS_DLL_EXPORT void initGrid( graticuleObj *pGraticule );
MS_DLL_EXPORT void initWeb(webObj *web);
MS_DLL_EXPORT void freeWeb(webObj *web);

MS_DLL_EXPORT featureListNodeObjPtr insertFeatureList(featureListNodeObjPtr *list, shapeObj *shape);
MS_DLL_EXPORT void freeFeatureList(featureListNodeObjPtr list);

MS_DLL_EXPORT int msLoadProjectionString(projectionObj *p, char *value);

/* To be used *only* within the mapfile loading phase */
MS_DLL_EXPORT int loadExpressionString(expressionObj *exp, char *value);
/* Use this next, thread safe wrapper, function everywhere else */
MS_DLL_EXPORT int msLoadExpressionString(expressionObj *exp, char *value);
MS_DLL_EXPORT char *msGetExpressionString(expressionObj *exp);
MS_DLL_EXPORT void freeExpression(expressionObj *exp);
MS_DLL_EXPORT char *msGetExpressionString(expressionObj *exp);

MS_DLL_EXPORT int getClassIndex(layerObj *layer, char *str);



/* For maplabel and mappdf */
int labelInImage(int width, int height, shapeObj *lpoly, int buffer);
int intersectLabelPolygons(shapeObj *p1, shapeObj *p2);
pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly);
double dist(pointObj a, pointObj b);


   
/*
** Main API Functions
*/

/* mapobject.c */

MS_DLL_EXPORT void msFreeMap(mapObj *map);
MS_DLL_EXPORT mapObj *msNewMapObj(void);
MS_DLL_EXPORT const char *msGetConfigOption( mapObj *map, const char *key);
MS_DLL_EXPORT void msSetConfigOption( mapObj *map, const char *key, const char *value);
MS_DLL_EXPORT int msTestConfigOption( mapObj *map, const char *key, 
                                      int default_result );
MS_DLL_EXPORT void msApplyMapConfigOptions( mapObj *map );
MS_DLL_EXPORT int msMapComputeGeotransform( mapObj *map );

MS_DLL_EXPORT void msMapPixelToGeoref( mapObj *map, double *x, double *y );
MS_DLL_EXPORT void msMapGeorefToPixel( mapObj *map, double *x, double *y );

MS_DLL_EXPORT int msMapSetExtent(mapObj *map, double minx, double miny, 
                                 double maxx, double maxy);
MS_DLL_EXPORT int msMapSetRotation( mapObj *map, double rotation_angle );
MS_DLL_EXPORT int msMapSetSize( mapObj *map, int width, int height );
MS_DLL_EXPORT int msMapSetSize( mapObj *map, int width, int height );
MS_DLL_EXPORT int msMapSetFakedExtent( mapObj *map );
MS_DLL_EXPORT int msMapRestoreRealExtent( mapObj *map );
MS_DLL_EXPORT int msMapLoadOWSParameters( mapObj *map, cgiRequestObj *request,
                                          const char *wmtver_string );

/* mapfile.c */
   
MS_DLL_EXPORT int msGetLayerIndex(mapObj *map, char *name); /* in mapfile.c */
MS_DLL_EXPORT int msGetSymbolIndex(symbolSetObj *set, char *name, int try_addimage_if_notfound);
MS_DLL_EXPORT mapObj  *msLoadMap(char *filename, char *new_mappath);
MS_DLL_EXPORT int msSaveMap(mapObj *map, char *filename);
MS_DLL_EXPORT void msFreeCharArray(char **array, int num_items);
MS_DLL_EXPORT int msLoadMapString(mapObj *map, char *object, char *value);
MS_DLL_EXPORT int msEvalRegex(char *e, char *s);
MS_DLL_EXPORT void msFree(void *p);
MS_DLL_EXPORT char **msTokenizeMap(char *filename, int *numtokens);
MS_DLL_EXPORT int msInitLabelCache(labelCacheObj *cache);
MS_DLL_EXPORT int msFreeLabelCache(labelCacheObj *cache);
MS_DLL_EXPORT int msCheckConnection(layerObj * layer); /* connection pooling functions (mapfile.c) */
MS_DLL_EXPORT void msCloseConnections(mapObj *map); 

#if defined USE_PDF
MS_DLL_EXPORT PDF *msDrawMapPDF(mapObj *map, PDF *pdf, hashTableObj fontHash); /* mappdf.c */
#endif /* USE_PDF */

MS_DLL_EXPORT void msOGRCleanup(void);
MS_DLL_EXPORT void msGDALCleanup(void);
MS_DLL_EXPORT void msGDALInitialize(void);
   

MS_DLL_EXPORT imageObj *msDrawScalebar(mapObj *map); /* in mapscale.c */
MS_DLL_EXPORT int msCalculateScale(rectObj extent, int units, int width, int height, double resolution, double *scale);
MS_DLL_EXPORT double msInchesPerUnit(int units, double center_lat);
MS_DLL_EXPORT int msEmbedScalebar(mapObj *map, gdImagePtr img);


MS_DLL_EXPORT int msPointInRect(pointObj *p, rectObj *rect); /* in mapsearch.c */
MS_DLL_EXPORT int msRectOverlap(rectObj *a, rectObj *b);
MS_DLL_EXPORT int msRectContained(rectObj *a, rectObj *b);

MS_DLL_EXPORT void msRectToFormattedString(rectObj *rect, char *format,
                                           char *buffer, int buffer_length);
MS_DLL_EXPORT void msPointToFormattedString(pointObj *point, const char*format,
                                           char *buffer, int buffer_length);

MS_DLL_EXPORT void msMergeRect(rectObj *a, rectObj *b);
MS_DLL_EXPORT double msDistancePointToPoint(pointObj *a, pointObj *b);
MS_DLL_EXPORT double msDistancePointToSegment(pointObj *p, pointObj *a, pointObj *b);
MS_DLL_EXPORT double msDistancePointToShape(pointObj *p, shapeObj *shape);
MS_DLL_EXPORT double msDistanceSegmentToSegment(pointObj *pa, pointObj *pb, pointObj *pc, pointObj *pd);
MS_DLL_EXPORT double msDistanceShapeToShape(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT int msIntersectSegments(pointObj *a, pointObj *b, pointObj *c, pointObj *d);
MS_DLL_EXPORT int msPointInPolygon(pointObj *p, lineObj *c);
MS_DLL_EXPORT int msIntersectMultipointPolygon(multipointObj *points, shapeObj *polygon);
MS_DLL_EXPORT int msIntersectPointPolygon(pointObj *p, shapeObj *polygon);
MS_DLL_EXPORT int msIntersectPolylinePolygon(shapeObj *line, shapeObj *poly);
MS_DLL_EXPORT int msIntersectPolygons(shapeObj *p1, shapeObj *p2);
MS_DLL_EXPORT int msIntersectPolylines(shapeObj *line1, shapeObj *line2);

MS_DLL_EXPORT int msSaveQuery(mapObj *map, char *filename); /* in mapquery.c */
MS_DLL_EXPORT int msLoadQuery(mapObj *map, char *filename);
MS_DLL_EXPORT int msQueryByIndex(mapObj *map, int qlayer, int tileindex, int shapeindex);
MS_DLL_EXPORT int msQueryByIndexAdd(mapObj *map, int qlayer, int tileindex, int shapeindex);
MS_DLL_EXPORT int msQueryByAttributes(mapObj *map, int qlayer, char *qitem, char *qstring, int mode);
MS_DLL_EXPORT int msQueryByPoint(mapObj *map, int qlayer, int mode, pointObj p, double buffer);
MS_DLL_EXPORT int msQueryByRect(mapObj *map, int qlayer, rectObj rect);
MS_DLL_EXPORT int msQueryByFeatures(mapObj *map, int qlayer, int slayer);
MS_DLL_EXPORT int msQueryByShape(mapObj *map, int qlayer, shapeObj *selectshape);
MS_DLL_EXPORT int msGetQueryResultBounds(mapObj *map, rectObj *bounds);
MS_DLL_EXPORT int msIsLayerQueryable(layerObj *lp);
MS_DLL_EXPORT void msQueryFree(mapObj *map, int qlayer);
MS_DLL_EXPORT int msRasterQueryByShape(mapObj *map, layerObj *layer, shapeObj *selectshape);
MS_DLL_EXPORT int msRasterQueryByRect(mapObj *map, layerObj *layer, rectObj queryRect);
MS_DLL_EXPORT int msRasterQueryByPoint(mapObj *map, layerObj *layer, int mode, 
                         pointObj p, double buffer );


MS_DLL_EXPORT void trimBlanks(char *string); /* in mapstring.c */
MS_DLL_EXPORT char *trimLeft(char *string);
MS_DLL_EXPORT char *chop(char *string);
MS_DLL_EXPORT void trimEOL(char *string);
MS_DLL_EXPORT char *gsub(char *str, const char *old, const char *sznew);
MS_DLL_EXPORT char *stripPath(char *fn);
MS_DLL_EXPORT char *getPath(char *fn);
MS_DLL_EXPORT char *msBuildPath(char *pszReturnPath, const char *abs_path, const char *path);
MS_DLL_EXPORT char *msBuildPath3(char *pszReturnPath, const char *abs_path, const char *path1, const char *path2);
MS_DLL_EXPORT char *msTryBuildPath(char *szReturnPath, const char *abs_path, const char *path);
MS_DLL_EXPORT char *msTryBuildPath3(char *szReturnPath, const char *abs_path, const char *path1, const char *path2);
MS_DLL_EXPORT char **split(const char *string, char cd, int *num_tokens);
MS_DLL_EXPORT int countChars(char *str, char ch);
MS_DLL_EXPORT char *long2string(long value);
MS_DLL_EXPORT char *double2string(double value);
MS_DLL_EXPORT char *msEncodeUrl(const char*);
MS_DLL_EXPORT char *msEncodeHTMLEntities(const char *string);
MS_DLL_EXPORT void msDecodeHTMLEntities(const char *string);
MS_DLL_EXPORT int msIsXMLTagValid(const char *string);
MS_DLL_EXPORT char *strcatalloc(char *pszDest, char *pszSrc);
MS_DLL_EXPORT char *msJoinStrings(char **array, int arrayLength, const char *delimeter);
MS_DLL_EXPORT char *msHashString(const char *pszStr);
MS_DLL_EXPORT char *msCommifyString(char *str);
MS_DLL_EXPORT const char *msCaseFindSubstring(const char *haystack, const char *needle);

#ifdef NEED_STRDUP
MS_DLL_EXPORT char *strdup(char *s);
#endif /* NEED_STRDUP */

#ifdef NEED_STRNCASECMP
MS_DLL_EXPORT int strncasecmp(const char *s1, const char *s2, int len);
#endif /* NEED_STRNCASECMP */

#ifdef NEED_STRCASECMP
MS_DLL_EXPORT int strcasecmp(const char *s1, const char *s2);
#endif /* NEED_STRCASECMP */

#ifdef NEED_STRLCAT
MS_DLL_EXPORT size_t strlcat(char *dst, const char *src, size_t siz);
#endif /* NEED_STRLCAT */

/* in mapsymbol.c */
/* Use this function *only* with mapfile loading phase */
MS_DLL_EXPORT int loadSymbolSet(symbolSetObj *symbolset, mapObj *map);
/* Use this threadsafe wrapper everywhere else */
MS_DLL_EXPORT int msLoadSymbolSet(symbolSetObj *symbolset, mapObj *map);
MS_DLL_EXPORT int msCopySymbol(symbolObj *dst, symbolObj *src, mapObj *map);
MS_DLL_EXPORT int msCopySymbolSet(symbolSetObj *dst, symbolSetObj *src, mapObj *map);
MS_DLL_EXPORT void msInitSymbolSet(symbolSetObj *symbolset);
MS_DLL_EXPORT int msAddImageSymbol(symbolSetObj *symbolset, char *filename);
MS_DLL_EXPORT void msFreeSymbolSet(symbolSetObj *symbolset);
MS_DLL_EXPORT void msFreeSymbol(symbolObj *symbol);
MS_DLL_EXPORT int msAddNewSymbol(mapObj *map, char *name);
MS_DLL_EXPORT int msAppendSymbol(symbolSetObj *symbolset, symbolObj *symbol);
MS_DLL_EXPORT symbolObj *msRemoveSymbol(symbolSetObj *symbolset, int index);
MS_DLL_EXPORT int msSaveSymbolSet(symbolSetObj *symbolset, const char *filename);
MS_DLL_EXPORT int msLoadImageSymbol(symbolObj *symbol, const char *filename);
MS_DLL_EXPORT symbolObj *msRotateSymbol(symbolObj *symbol, double angle);

MS_DLL_EXPORT imageObj *msSymbolGetImageGD(symbolObj *symbol, outputFormatObj *format);
MS_DLL_EXPORT int msSymbolSetImageGD(symbolObj *symbol, imageObj *image);

MS_DLL_EXPORT int msGetMarkerSize(symbolSetObj *symbolset, styleObj *style, int *width, int *height, double scalefactor);
MS_DLL_EXPORT int msGetCharacterSize(char *character, int size, char *font, rectObj *rect);
MS_DLL_EXPORT double msSymbolGetDefaultSize(symbolObj *s);
MS_DLL_EXPORT void freeImageCache(struct imageCacheObj *ic);

MS_DLL_EXPORT imageObj *msDrawLegend(mapObj *map, int scale_independent); /* in maplegend.c */
MS_DLL_EXPORT int msEmbedLegend(mapObj *map, gdImagePtr img);
MS_DLL_EXPORT int msDrawLegendIcon(mapObj* map, layerObj* lp, classObj* myClass, int width, int height, gdImagePtr img, int dstX, int dstY);
MS_DLL_EXPORT imageObj *msCreateLegendIcon(mapObj* map, layerObj* lp, classObj* myClass, int width, int height);
   
MS_DLL_EXPORT int msLoadFontSet(fontSetObj *fontSet, mapObj *map); /* in maplabel.c */
MS_DLL_EXPORT int msInitFontSet(fontSetObj *fontset);
MS_DLL_EXPORT int msFreeFontSet(fontSetObj *fontset);

MS_DLL_EXPORT int msGetLabelSize(char *string, labelObj *label, rectObj *rect, fontSetObj *fontSet, double scalefactor, int adjustBaseline);
MS_DLL_EXPORT int msGetLabelSizeEx(char *string, labelObj *label, rectObj *rect, fontSetObj *fontSet, double scalefactor, int adjustBaseline, double **offsets);
MS_DLL_EXPORT int msAddLabel(mapObj *map, int layerindex, int classindex, int shapeindex, int tileindex, pointObj *point, labelPathObj *labelpath, char *string, double featuresize, labelObj *label);

MS_DLL_EXPORT gdFontPtr msGetBitmapFont(int size);
MS_DLL_EXPORT int msImageTruetypePolyline(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT int msImageTruetypeArrow(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor);

MS_DLL_EXPORT void msFreeShape(shapeObj *shape); /* in mapprimative.c */
MS_DLL_EXPORT void msFreeLabelPathObj(labelPathObj *path);
MS_DLL_EXPORT shapeObj *msShapeFromWKT(const char *string);
MS_DLL_EXPORT char *msShapeToWKT(shapeObj *shape);
MS_DLL_EXPORT void msInitShape(shapeObj *shape);
MS_DLL_EXPORT void msShapeDeleteLine( shapeObj *shape, int line );
MS_DLL_EXPORT int msCopyShape(shapeObj *from, shapeObj *to);
MS_DLL_EXPORT int *msGetOuterList(shapeObj *shape);
MS_DLL_EXPORT int *msGetInnerList(shapeObj *shape, int r, int *outerlist);
MS_DLL_EXPORT void msComputeBounds(shapeObj *shape);
MS_DLL_EXPORT void msRectToPolygon(rectObj rect, shapeObj *poly);
MS_DLL_EXPORT void msClipPolylineRect(shapeObj *shape, rectObj rect);
MS_DLL_EXPORT void msClipPolygonRect(shapeObj *shape, rectObj rect);
MS_DLL_EXPORT void msTransformShape(shapeObj *shape, rectObj extent, double cellsize, imageObj *image);
MS_DLL_EXPORT void msOffsetPointRelativeTo(pointObj *point, layerObj *layer);
MS_DLL_EXPORT void msOffsetShapeRelativeTo(shapeObj *shape, layerObj *layer);
MS_DLL_EXPORT void msTransformShapeToPixel(shapeObj *shape, rectObj extent, double cellsize);
MS_DLL_EXPORT void msTransformPixelToShape(shapeObj *shape, rectObj extent, double cellsize);
MS_DLL_EXPORT void msImageCartographicPolyline(gdImagePtr im, shapeObj *p, styleObj *style, symbolObj *symbol, int c, double size, double scalefactor);
MS_DLL_EXPORT int msPolylineLabelPoint(shapeObj *p, pointObj *lp, int min_length, double *angle, double *length);
MS_DLL_EXPORT labelPathObj* msPolylineLabelPath(shapeObj *p, int min_length, fontSetObj *fontset, char *string, labelObj *label, double scalefactor, int *status);
MS_DLL_EXPORT int msPolygonLabelPoint(shapeObj *p, pointObj *lp, int min_dimension);
MS_DLL_EXPORT int msAddLine(shapeObj *p, lineObj *new_line);
MS_DLL_EXPORT int msAddLineDirectly(shapeObj *p, lineObj *new_line);
MS_DLL_EXPORT int msAddPointToLine(lineObj *line, pointObj *point );

MS_DLL_EXPORT int msDrawRasterLayer(mapObj *map, layerObj *layer, imageObj *image); /* in mapraster.c */
MS_DLL_EXPORT imageObj *msDrawReferenceMap(mapObj *map);

MS_DLL_EXPORT size_t msGetBitArraySize(int numbits); /* in mapbits.c */
MS_DLL_EXPORT char *msAllocBitArray(int numbits);
MS_DLL_EXPORT int msGetBit(char *array, int index);
MS_DLL_EXPORT void msSetBit(char *array, int index, int value);
MS_DLL_EXPORT void msFlipBit(char *array, int index);

MS_DLL_EXPORT int msLayerInitItemInfo(layerObj *layer);
MS_DLL_EXPORT void msLayerFreeItemInfo(layerObj *layer); 

MS_DLL_EXPORT int msLayerOpen(layerObj *layer); /* in maplayer.c */
MS_DLL_EXPORT int msLayerIsOpen(layerObj *layer);
MS_DLL_EXPORT void msLayerClose(layerObj *layer);
MS_DLL_EXPORT int msLayerWhichShapes(layerObj *layer, rectObj rect);
MS_DLL_EXPORT int msLayerWhichItems(layerObj *layer, int classify, int annotate, char *metadata);
MS_DLL_EXPORT int msLayerNextShape(layerObj *layer, shapeObj *shape);
MS_DLL_EXPORT int msLayerGetItems(layerObj *layer);
MS_DLL_EXPORT int msLayerSetItems(layerObj *layer, char **items, int numitems);
MS_DLL_EXPORT int msLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record);
MS_DLL_EXPORT int msLayerGetExtent(layerObj *layer, rectObj *extent);
MS_DLL_EXPORT int msLayerSetExtent( layerObj *layer, double minx, double miny, double maxx, double maxy);
MS_DLL_EXPORT int msLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c, int tile, long record);
MS_DLL_EXPORT void msLayerAddProcessing( layerObj *layer, const char *directive );
MS_DLL_EXPORT void msLayerSetProcessingKey( layerObj *layer, const char *key, 
                                            const char *value);
MS_DLL_EXPORT char *msLayerGetProcessing( layerObj *layer, int proc_index);
MS_DLL_EXPORT char *msLayerGetProcessingKey( layerObj *layer, const char *);
MS_DLL_EXPORT int msLayerClearProcessing( layerObj *layer );
MS_DLL_EXPORT char* msLayerGetFilterString( layerObj *layer );

MS_DLL_EXPORT int msLayerSetTimeFilter(layerObj *lp, const char *timestring, 
                                       const char *timefield);
/* Helper functions for layers */ 
MS_DLL_EXPORT int msLayerMakeBackticsTimeFilter(layerObj *lp, const char *timestring, 
                                                 const char *timefield);

MS_DLL_EXPORT int msLayerMakePlainTimeFilter(layerObj *lp, const char *timestring, 
                                             const char *timefield);

MS_DLL_EXPORT int msLayerApplyCondSQLFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                                   int iLayerIndex, int bOnlySpatialFilter);

MS_DLL_EXPORT int msLayerApplyPlainFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                                 int iLayerIndex, int bOnlySpatialFilter);


/* maplayer.c */
MS_DLL_EXPORT int msLayerGetNumFeatures(layerObj *layer);

MS_DLL_EXPORT char *msLayerEscapeSQLParam(layerObj *layer, const char* pszString);
MS_DLL_EXPORT char *msLayerEscapePropertyName(layerObj *layer, const char* pszString);

/* These are special because SWF is using these */
int msOGRLayerNextShape(layerObj *layer, shapeObj *shape);
int msOGRLayerGetItems(layerObj *layer);
void msOGRLayerFreeItemInfo(layerObj *layer);
int msOGRLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record);
int msOGRLayerGetExtent(layerObj *layer, rectObj *extent);

#ifdef USE_OGR
MS_DLL_EXPORT int msOGRGeometryToShape(OGRGeometryH hGeometry, shapeObj *shape,
                         OGRwkbGeometryType type);
#endif /* USE_OGR */

MS_DLL_EXPORT int msPOSTGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record);
MS_DLL_EXPORT int msMYGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record);
MS_DLL_EXPORT int drawSDE(mapObj *map, layerObj *layer, gdImagePtr img);

MS_DLL_EXPORT int msInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msConnectLayer(layerObj *layer, const int connectiontype, 
                                 const char *library_str);

MS_DLL_EXPORT int msINLINELayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msShapeFileLayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msTiledSHPLayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msSDELayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msOGRLayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msPOSTGISLayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msOracleSpatialLayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msWFSLayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msGraticuleLayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msMYGISLayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msRASTERLayerInitializeVirtualTable(layerObj *layer);
MS_DLL_EXPORT int msPluginLayerInitializeVirtualTable(layerObj *layer);

/* ==================================================================== */
/*      Prototypes for functions in mapdraw.c                           */
/* ==================================================================== */
MS_DLL_EXPORT void msClearLayerPenValues(layerObj *layer);
MS_DLL_EXPORT void msClearScalebarPenValues(scalebarObj *scalebar);
MS_DLL_EXPORT void msClearLegendPenValues(legendObj *legend);
MS_DLL_EXPORT void msClearReferenceMapPenValues(referenceMapObj *referencemap);
MS_DLL_EXPORT void msClearQueryMapPenValues(queryMapObj *querymap);
MS_DLL_EXPORT void msClearPenValues(mapObj *map);

MS_DLL_EXPORT imageObj *msPrepareImage(mapObj *map, int allow_nonsquare);
MS_DLL_EXPORT imageObj *msDrawMap(mapObj *map);
MS_DLL_EXPORT imageObj *msDrawQueryMap(mapObj *map);
MS_DLL_EXPORT int msLayerIsVisible(mapObj *map, layerObj *layer);
MS_DLL_EXPORT int msDrawLayer(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msDrawVectorLayer(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msDrawQueryLayer(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msDrawWMSLayer(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msDrawWFSLayer(mapObj *map, layerObj *layer, imageObj *image);

MS_DLL_EXPORT int msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, imageObj *image, int style);
MS_DLL_EXPORT int msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, imageObj *image, int classindex, char *labeltext);

  /*Range Support*/
MS_DLL_EXPORT int msShapeToRange(styleObj *style, shapeObj *shape);
MS_DLL_EXPORT int msValueToRange(styleObj *style, double fieldVal);

MS_DLL_EXPORT void msCircleDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msCircleDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawMarkerSymbol(symbolSetObj *symbolset,imageObj *image, pointObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor);

MS_DLL_EXPORT int msDrawLabel(imageObj *image, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor);
MS_DLL_EXPORT int msDrawText(imageObj *image, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor);
MS_DLL_EXPORT int msDrawLabelCache(imageObj *image, mapObj *map);

MS_DLL_EXPORT void msImageStartLayer(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT void msImageEndLayer(mapObj *map, layerObj *layer, imageObj *image);

MS_DLL_EXPORT void msDrawStartShape(mapObj *map, layerObj *layer, imageObj *image, shapeObj *shape);
MS_DLL_EXPORT void msDrawEndShape(mapObj *map, layerObj *layer, imageObj *image, shapeObj *shape);
/* ==================================================================== */
/*      End of Prototypes for functions in mapdraw.c                    */
/* ==================================================================== */

/* ==================================================================== */
/*      Prototypes for functions in mapimagemap.c                       */
/* ==================================================================== */
MS_DLL_EXPORT imageObj *msImageCreateIM(int width, int height, outputFormatObj *format, char *imagepath, char *imageurl);
MS_DLL_EXPORT imageObj *msImageLoadIM( const char *filename );
MS_DLL_EXPORT imageObj *msImageLoadGD( const char *filename );
MS_DLL_EXPORT imageObj *msImageLoadGDCtx( gdIOCtx *ctx, const char *driver );
MS_DLL_EXPORT void msImageInitIM( imageObj *image );
MS_DLL_EXPORT void msImageStartLayerIM(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msSaveImageIM(imageObj* img, char *filename, outputFormatObj *format);
MS_DLL_EXPORT int msSaveImageIM_LL(imageObj* img, char *filename, int type, int transparent, int interlace, int quality);
MS_DLL_EXPORT void msFreeImagexsIM(imageObj* img);
MS_DLL_EXPORT void msFreeImageIM(imageObj* img);
MS_DLL_EXPORT void msCircleDrawLineSymbolIM(symbolSetObj *symbolset, imageObj* img, pointObj *p, double r, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msCircleDrawShadeSymbolIM(symbolSetObj *symbolset, imageObj* img, pointObj *p, double r, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawMarkerSymbolIM(symbolSetObj *symbolset, imageObj* img, pointObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawLineSymbolIM(symbolSetObj *symbolset, imageObj* img, shapeObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawShadeSymbolIM(symbolSetObj *symbolset, imageObj* img, shapeObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT int msDrawTextIM(imageObj* img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor);
MS_DLL_EXPORT int msDrawLabelCacheIM(imageObj* img, mapObj *map);
/* ==================================================================== */
/*      End of Prototypes for functions in mapimagemap.c                */
/* ==================================================================== */

/* ==================================================================== */
/*      Prototypes for functions in mapgd.c                             */
/* ==================================================================== */
MS_DLL_EXPORT imageObj *msImageLoadGDCtx(gdIOCtx* ctx, const char *driver);
MS_DLL_EXPORT int msCompareColors(colorObj *c1, colorObj *c2);
MS_DLL_EXPORT void msPreAllocateColorsGD(imageObj *image, mapObj *map);
MS_DLL_EXPORT imageObj *msImageCreateGD(int width, int height, outputFormatObj *format, char *imagepath, char *imageurl);
MS_DLL_EXPORT imageObj *msImageLoadGD( const char *filename );
MS_DLL_EXPORT void msImageInitGD( imageObj *image, colorObj *background );
MS_DLL_EXPORT int msImageSetPenGD(gdImagePtr img, colorObj *color);

#define RESOLVE_PEN_GD(img,color) { if( (color).pen == MS_PEN_UNSET ) msImageSetPenGD( img, &(color) ); }

MS_DLL_EXPORT gdIOCtx *msNewGDFileCtx(FILE *file);
MS_DLL_EXPORT int msSaveImageGD(gdImagePtr img, char *filename, outputFormatObj *format);
MS_DLL_EXPORT unsigned char *msSaveImageBufferGD(gdImagePtr img, int *bufsize, outputFormatObj *format);
MS_DLL_EXPORT int msSaveImageGDCtx(gdImagePtr img, gdIOCtx* ctx, outputFormatObj *format);
MS_DLL_EXPORT int msSaveImageGD_LL(gdImagePtr img, char *filename, int type, int transparent, int interlace, int quality);
MS_DLL_EXPORT void msFreeImageGD(gdImagePtr img);

MS_DLL_EXPORT void msCircleDrawLineSymbolGD(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msCircleDrawShadeSymbolGD(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawMarkerSymbolGD(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawLineSymbolGD(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawShadeSymbolGD(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor);

MS_DLL_EXPORT int msDrawTextGD(gdImagePtr img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor);
MS_DLL_EXPORT int msDrawTextLineGD(gdImagePtr img, char *string, labelObj *label, labelPathObj *labelpath, fontSetObj *fontset, double scalefactor);
MS_DLL_EXPORT int msDrawLabelCacheGD(gdImagePtr img, mapObj *map);

MS_DLL_EXPORT void msImageCopyMerge (gdImagePtr dst, gdImagePtr src, int dstX, int dstY, int srcX, int srcY, int w, int h, int pct);

/* Modify the character encoding. */
char *msGetEncodedString(const char *string, const char *encoding);

/* various JOIN functions (in mapjoin.c) */
MS_DLL_EXPORT int msJoinConnect(layerObj *layer, joinObj *join);
MS_DLL_EXPORT int msJoinPrepare(joinObj *join, shapeObj *shape);
MS_DLL_EXPORT int msJoinNext(joinObj *join);
MS_DLL_EXPORT int msJoinClose(joinObj *join);

/*in mapraster.c */
MS_DLL_EXPORT int msDrawRasterLayerLow(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msAddColorGD(mapObj *map, gdImagePtr img, int cmt, int r, int g, int b);
MS_DLL_EXPORT int msGetClass(layerObj *layer, colorObj *color);
MS_DLL_EXPORT int msGetClass_Float(layerObj *layer, float fValue);

/* in mapdrawgdal.c */
MS_DLL_EXPORT int msDrawRasterLayerGDAL(mapObj *map, layerObj *layer, imageObj *image, void *hDSVoid );
MS_DLL_EXPORT int msGetGDALGeoTransform(void *hDS, mapObj *map, layerObj *layer, double *padfGeoTransform );
MS_DLL_EXPORT int *msGetGDALBandList( layerObj *layer, void *hDS, int max_bands, int *band_count );
MS_DLL_EXPORT double msGetGDALNoDataValue( layerObj *layer, void *hBand, int *pbGotNoData );

/* ==================================================================== */
/*      End of prototypes for functions in mapgd.c                      */
/* ==================================================================== */

/* ==================================================================== */
/*      Prototypes for functions in maputil.c                           */
/* ==================================================================== */
/* For mappdf */
MS_DLL_EXPORT int getRgbColor(mapObj *map,int i,int *r,int *g,int *b); /* maputil.c */
MS_DLL_EXPORT int msValidateContexts(mapObj *map);
MS_DLL_EXPORT int msEvalContext(mapObj *map, layerObj *layer, char *context);
MS_DLL_EXPORT int msEvalExpression(expressionObj *expression, int itemindex, char **items, int numitems);
MS_DLL_EXPORT int msShapeGetClass(layerObj *layer, shapeObj *shape, double scale);
MS_DLL_EXPORT char *msShapeGetAnnotation(layerObj *layer, shapeObj *shape);
MS_DLL_EXPORT int msAdjustImage(rectObj rect, int *width, int *height);
MS_DLL_EXPORT double msAdjustExtent(rectObj *rect, int width, int height);
MS_DLL_EXPORT int msConstrainExtent(rectObj *bounds, rectObj *rect, double overlay);
MS_DLL_EXPORT int *msGetLayersIndexByGroup(mapObj *map, char *groupname, int *nCount);

/* Functions to chnage the drawing order of the layers. */
/* Defined in mapobject.c */
MS_DLL_EXPORT int msMoveLayerUp(mapObj *map, int nLayerIndex);
MS_DLL_EXPORT int msMoveLayerDown(mapObj *map, int nLayerIndex);
MS_DLL_EXPORT int msSetLayersdrawingOrder(mapObj *self, int *panIndexes);
MS_DLL_EXPORT int msInsertLayer(mapObj *map, layerObj *layer, int nIndex);
MS_DLL_EXPORT layerObj *msRemoveLayer(mapObj *map, int nIndex);

/* Defined in layerobject.c */
MS_DLL_EXPORT int msInsertClass(layerObj *layer,classObj *classobj,int nIndex);
MS_DLL_EXPORT classObj *msRemoveClass(layerObj *layer, int nIndex);
MS_DLL_EXPORT int msMoveClassUp(layerObj *layer, int nClassIndex);
MS_DLL_EXPORT int msMoveClassDown(layerObj *layer, int nClassIndex);

/* classobject.c */
MS_DLL_EXPORT int msMoveStyleUp(classObj *classo, int nStyleIndex);
MS_DLL_EXPORT int msMoveStyleDown(classObj *classo, int nStyleIndex);
MS_DLL_EXPORT int msDeleteStyle(classObj *classo, int iStyleIndex);
MS_DLL_EXPORT int msInsertStyle(classObj *classo, styleObj *style,
                                int nStyleIndex);
MS_DLL_EXPORT styleObj *msRemoveStyle(classObj *classo, int index);

/* Measured shape utility functions. */
MS_DLL_EXPORT pointObj *msGetPointUsingMeasure(shapeObj *shape, double m);
MS_DLL_EXPORT pointObj *msGetMeasureUsingPoint(shapeObj *shape, pointObj *point);

MS_DLL_EXPORT char **msGetAllGroupNames(mapObj* map, int *numTok);
MS_DLL_EXPORT char *msTmpFile(const char *mappath, const char *tmppath, const char *ext);
MS_DLL_EXPORT void msForceTmpFileBase( const char *new_base );


MS_DLL_EXPORT imageObj *msImageCreate(int width, int height, outputFormatObj *format, char *imagepath, char *imageurl, mapObj *map);

MS_DLL_EXPORT int msAlphaBlend (int dst, int src);


/* ==================================================================== */
/*      End of prototypes for functions in maputil.c                    */
/* ==================================================================== */

/* ==================================================================== */
/*      prototypes for functions in mapswf.c                            */
/* ==================================================================== */
#ifdef USE_MING_FLASH
MS_DLL_EXPORT imageObj *msImageCreateSWF(int width, int height, outputFormatObj *format, char *imagepath, char *imageurl, mapObj *map);
MS_DLL_EXPORT void msImageStartLayerSWF(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msDrawLabelSWF(imageObj *image, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor);
MS_DLL_EXPORT int msDrawLabelCacheSWF(imageObj *image, mapObj *map);
MS_DLL_EXPORT void msDrawLineSymbolSWF(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawShadeSymbolSWF(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawMarkerSymbolSWF(symbolSetObj *symbolset, imageObj *image, pointObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT int msDrawRasterLayerSWF(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msDrawVectorLayerAsRasterSWF(mapObj *map, layerObj *layer, imageObj*image);
/* int msDrawWMSLayerSWF(int nLayerId, httpRequestObj *pasReqInfo, int numRequests, mapObj *map, layerObj *layer, imageObj *image); */
MS_DLL_EXPORT void msTransformShapeSWF(shapeObj *shape, rectObj extent, double cellsize);
MS_DLL_EXPORT int msSaveImageSWF(imageObj *image, char *filename);
MS_DLL_EXPORT void msFreeImageSWF(imageObj *image);
MS_DLL_EXPORT int draw_textSWF(imageObj *image, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor);
MS_DLL_EXPORT void msDrawStartShapeSWF(mapObj *map, layerObj *layer, imageObj *image, shapeObj *shape);
#endif /* USE_MING_FLASH */

/* ==================================================================== */
/*      End of prototypes for functions in mapswf.c                     */
/* ==================================================================== */

/* ==================================================================== */

/* ==================================================================== */
/*      prototypes for functions in mappdf.c                            */
/* ==================================================================== */
#ifdef USE_PDF
MS_DLL_EXPORT PDF *msDrawMapPDF(mapObj *map, PDF *pdf, hashTableObj fontHash);
MS_DLL_EXPORT imageObj *msImageCreatePDF(int width, int height, outputFormatObj *format, char *imagepath, char *imageurl, mapObj *map);
MS_DLL_EXPORT void msImageStartLayerPDF(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msDrawLabelPDF(imageObj *image, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor);
MS_DLL_EXPORT int msDrawLabelCachePDF(imageObj *image, mapObj *map);
MS_DLL_EXPORT void msDrawLineSymbolPDF(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawShadeSymbolPDF(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT void msDrawMarkerSymbolPDF(symbolSetObj *symbolset, imageObj *image, pointObj *p, styleObj *style, double scalefactor);
MS_DLL_EXPORT int msDrawRasterLayerPDF(mapObj *map, layerObj *layer, imageObj *image);
MS_DLL_EXPORT int msDrawVectorLayerAsRasterPDF(mapObj *map, layerObj *layer, imageObj*image);
MS_DLL_EXPORT void msTransformShapePDF(shapeObj *shape, rectObj extent, double cellsize);
MS_DLL_EXPORT int msSaveImagePDF(imageObj *image, char *filename);
MS_DLL_EXPORT void msFreeImagePDF(imageObj *image);
MS_DLL_EXPORT int msDrawTextPDF(imageObj *image, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor);
MS_DLL_EXPORT void msDrawStartShapePDF(mapObj *map, layerObj *layer, imageObj *image, shapeObj *shape);
#endif /* USE_PDF */

/* ==================================================================== */
/*      End of prototypes for functions in mappdf.c                     */
/* ==================================================================== */

/* ==================================================================== */
/*      prototypes for functions in mapsvg.c                            */
/* ==================================================================== */

MS_DLL_EXPORT imageObj *msImageCreateSVG(int width, int height, 
                                         outputFormatObj *format, char *imagepath, 
                                         char *imageurl, mapObj *map);

MS_DLL_EXPORT void msImageStartLayerSVG(mapObj *map, layerObj *layer, 
                                          imageObj *image);

MS_DLL_EXPORT void msDrawLineSymbolSVG(symbolSetObj *symbolset, imageObj *image, 
                                       shapeObj *p, styleObj *style, 
                                       double scalefactor);
MS_DLL_EXPORT void msTransformShapeSVG(shapeObj *shape, rectObj extent, double cellsize,
                                       imageObj *image);
MS_DLL_EXPORT int msSaveImageSVG(imageObj *image, char *filename);
MS_DLL_EXPORT int msSaveImagetoFpSVG(imageObj *image, FILE *fp);
MS_DLL_EXPORT void msFreeImageSVG(imageObj *image);

MS_DLL_EXPORT void msDrawMarkerSymbolSVG(symbolSetObj *symbolset, 
                                         imageObj *image, 
                                         pointObj *p, styleObj *style, 
                                         double scalefactor);

MS_DLL_EXPORT int msDrawTextSVG(imageObj *image, pointObj labelPnt, char *string, 
                                labelObj *label, fontSetObj *fontset, 
                                double scalefactor);
MS_DLL_EXPORT int msDrawLabelCacheSVG(imageObj *image, mapObj *map);

MS_DLL_EXPORT int msDrawRasterLayerSVG(mapObj *map, layerObj *layer, 
                                       imageObj *image);
MS_DLL_EXPORT void msDrawShadeSymbolSVG(symbolSetObj *symbolset, 
                                        imageObj *image, shapeObj *p, 
                                        styleObj *style, double scalefactor);


/* ==================================================================== */
/*      prototypes for functions in mapoutput.c                         */
/* ==================================================================== */

MS_DLL_EXPORT void msApplyDefaultOutputFormats( mapObj * );
MS_DLL_EXPORT void msFreeOutputFormat( outputFormatObj * );
MS_DLL_EXPORT int msGetOutputFormatIndex(mapObj *map, const char *imagetype);
MS_DLL_EXPORT int msRemoveOutputFormat(mapObj *map, const char *imagetype);
MS_DLL_EXPORT int msAppendOutputFormat(mapObj *map, outputFormatObj *format);
MS_DLL_EXPORT outputFormatObj *msSelectOutputFormat( mapObj *map, const char *imagetype );
MS_DLL_EXPORT void msApplyOutputFormat( outputFormatObj **target, outputFormatObj *format, int transparent, int interlaced, int imagequality );
MS_DLL_EXPORT const char *msGetOutputFormatOption( outputFormatObj *format, const char *optionkey, const char *defaultresult );
MS_DLL_EXPORT outputFormatObj *msCreateDefaultOutputFormat( mapObj *map, const char *driver );
MS_DLL_EXPORT int msPostMapParseOutputFormatSetup( mapObj *map );
MS_DLL_EXPORT void msSetOutputFormatOption( outputFormatObj *format, const char *key, const char *value );
MS_DLL_EXPORT void msGetOutputFormatMimeList( mapObj *map, char **mime_list, int max_mime );
MS_DLL_EXPORT void msGetOutputFormatMimeListGD( mapObj *map, char **mime_list, int max_mime );
MS_DLL_EXPORT void msGetOutputFormatMimeListRaster( mapObj *map, char **mime_list, int max_mime );
MS_DLL_EXPORT outputFormatObj *msCloneOutputFormat( outputFormatObj *format );
MS_DLL_EXPORT int msOutputFormatValidate( outputFormatObj *format );

#ifndef gdImageTrueColor
#  define gdImageTrueColor(x) (0)
#endif /* not gdImageTrueColor */

/* ==================================================================== */
/*      prototypes for functions in mapgdal.c                           */
/* ==================================================================== */
MS_DLL_EXPORT int msSaveImageGDAL( mapObj *map, imageObj *image, char *filename );
MS_DLL_EXPORT int msInitDefaultGDALOutputFormat( outputFormatObj *format );

/* ==================================================================== */
/*      End of prototypes for functions in mapoutput.c                  */
/* ==================================================================== */

/* ==================================================================== */
/*      Public prototype for mapogr.cpp functions.                      */
/* ==================================================================== */
int MS_DLL_EXPORT msOGRLayerWhichShapes(layerObj *layer, rectObj rect);
int MS_DLL_EXPORT msOGRLayerOpen(layerObj *layer, const char *pszOverrideConnection); /* in mapogr.cpp */
int MS_DLL_EXPORT msOGRLayerClose(layerObj *layer);

char MS_DLL_EXPORT *msOGRShapeToWKT(shapeObj *shape);
shapeObj MS_DLL_EXPORT *msOGRShapeFromWKT(const char *string);

/* ==================================================================== */
/*      prototypes for functions in mapcopy                             */
/* ==================================================================== */
MS_DLL_EXPORT int msCopyMap(mapObj *dst, mapObj *src);
MS_DLL_EXPORT int msCopyLayer(layerObj *dst, layerObj *src);
MS_DLL_EXPORT int msCopyPoint(pointObj *dst, pointObj *src);
MS_DLL_EXPORT int msCopyFontSet(fontSetObj *dst, fontSetObj *src, mapObj *map);
MS_DLL_EXPORT void copyProperty(void *dst, void *src, int size);
MS_DLL_EXPORT char *copyStringProperty(char **dst, char *src);
MS_DLL_EXPORT int msCopyClass(classObj *dst, classObj *src, layerObj *layer);
MS_DLL_EXPORT int msCopyStyle(styleObj *dst, styleObj *src);
 
/* ==================================================================== */
/*      end prototypes for functions in mapcopy                         */
/* ==================================================================== */

/* ==================================================================== */
/*      mappool.c: connection pooling API.                              */
/* ==================================================================== */
MS_DLL_EXPORT void *msConnPoolRequest( layerObj *layer );
MS_DLL_EXPORT void msConnPoolRelease( layerObj *layer, void * );
MS_DLL_EXPORT void msConnPoolRegister( layerObj *layer,
                                        void *conn_handle,
                                        void (*close)( void * ) );
MS_DLL_EXPORT void msConnPoolCloseUnreferenced( void );
MS_DLL_EXPORT void msConnPoolFinalCleanup( void );

/* ==================================================================== */
/*      prototypes for functions in mapcpl.c                            */
/* ==================================================================== */
MS_DLL_EXPORT const char *msGetBasename( const char *pszFullFilename );
MS_DLL_EXPORT void *msGetSymbol(const char *pszLibrary, 
                                const char *pszEntryPoint);

/* ==================================================================== */
/*      include definitions from mapows.h                               */
/* ==================================================================== */
#include "mapows.h"

/* ==================================================================== */
/*      prototypes for functions in mapgeos.c                         */
/* ==================================================================== */
MS_DLL_EXPORT void msGEOSSetup();
MS_DLL_EXPORT void msGEOSCleanup();
MS_DLL_EXPORT void msGEOSFreeGeometry(shapeObj *shape);

MS_DLL_EXPORT shapeObj *msGEOSShapeFromWKT(const char *string);
MS_DLL_EXPORT char *msGEOSShapeToWKT(shapeObj *shape);

MS_DLL_EXPORT shapeObj *msGEOSBuffer(shapeObj *shape, double width);
MS_DLL_EXPORT shapeObj *msGEOSConvexHull(shapeObj *shape);
MS_DLL_EXPORT shapeObj *msGEOSBoundary(shapeObj *shape);
MS_DLL_EXPORT pointObj *msGEOSGetCentroid(shapeObj *shape);
MS_DLL_EXPORT shapeObj *msGEOSUnion(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT shapeObj *msGEOSIntersection(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT shapeObj *msGEOSDifference(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT shapeObj *msGEOSSymDifference(shapeObj *shape1, shapeObj *shape2);

MS_DLL_EXPORT int msGEOSContains(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT int msGEOSOverlaps(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT int msGEOSWithin(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT int msGEOSCrosses(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT int msGEOSIntersects(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT int msGEOSTouches(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT int msGEOSEquals(shapeObj *shape1, shapeObj *shape2);
MS_DLL_EXPORT int msGEOSDisjoint(shapeObj *shape1, shapeObj *shape2);

MS_DLL_EXPORT double msGEOSArea(shapeObj *shape);
MS_DLL_EXPORT double msGEOSLength(shapeObj *shape);
MS_DLL_EXPORT double msGEOSDistance(shapeObj *shape1, shapeObj *shape2);

/* ==================================================================== */
/*      prototypes for functions in mapcrypto.c                         */
/* ==================================================================== */
MS_DLL_EXPORT int msGenerateEncryptionKey(unsigned char *k);
MS_DLL_EXPORT int msReadEncryptionKeyFromFile(const char *keyfile, unsigned char *k);
MS_DLL_EXPORT void msEncryptStringWithKey(const unsigned char *key, const char *in, char *out);
MS_DLL_EXPORT void msDecryptStringWithKey(const unsigned char *key, const char *in, char *out);
MS_DLL_EXPORT char *msDecryptStringTokens(mapObj *map, const char *in);
MS_DLL_EXPORT void msHexEncode(const unsigned char *in, char *out, int numbytes);
MS_DLL_EXPORT int msHexDecode(const char *in, unsigned char *out, int numchars);


#endif

#ifdef __cplusplus
}
#endif

#endif /* MAP_H */

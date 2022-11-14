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
 *****************************************************************************/

#ifndef MAP_H
#define MAP_H

#include "mapserver-config.h"

/*
** Main includes. If a particular header was needed by several .c files then
** I just put it here. What the hell, it works and it's all right here. -SDL-
*/
#if defined(HAVE_STRCASESTR) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE /* Required for <string.h> strcasestr() defn */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <direct.h>
#include <memory.h>
#include <malloc.h>
#include <process.h>
#include <float.h>
#else
#include <unistd.h>
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define MS_DLL_EXPORT     __declspec(dllexport)
#define USE_MSFREE
#else
#define  MS_DLL_EXPORT
#endif

#if defined(__GNUC__)
#define WARN_UNUSED __attribute__((warn_unused_result))
#define MS_LIKELY(x)   __builtin_expect((x),1)
#define MS_UNLIKELY(x) __builtin_expect((x),0)
#else
#define WARN_UNUSED
#define MS_LIKELY(x) (x)
#define MS_UNLIKELY(x) (x)
#endif

/* definition of  ms_int32/ms_uint32 */
#include <limits.h>
#ifndef _WIN32
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

#if ULONG_MAX == 0xffffffff
typedef long            ms_int32;
typedef unsigned long   ms_uint32;
#elif UINT_MAX == 0xffffffff
typedef int             ms_int32;
typedef unsigned int    ms_uint32;
#else
typedef int32_t         ms_int32;
typedef uint32_t        ms_uint32;
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
/* Need to use _vsnprintf() with VS2003 */
#define vsnprintf _vsnprintf
#endif

#include "mapserver-api.h"

#ifndef SWIG
/*forward declaration of rendering object*/
typedef struct rendererVTableObj rendererVTableObj;
typedef struct tileCacheObj tileCacheObj;
typedef struct textPathObj textPathObj;
typedef struct textRunObj textRunObj;
typedef struct glyph_element glyph_element;
typedef struct face_element face_element;
#endif


/* ms_bitarray is used by the bit mask in mapbit.c */
typedef ms_uint32 *ms_bitarray;
typedef const ms_uint32 *ms_const_bitarray;

#include "maperror.h"
#include "mapprimitive.h"
#include "mapshape.h"
#include "mapflatgeobuf.h"
#include "mapsymbol.h"
#include "maptree.h" /* quadtree spatial index */
#include "maphash.h"
#include "mapio.h"
#include <assert.h>
#include "mapproject.h"
#include "cgiutil.h"
#include "mapserv-config.h"

#include <sys/types.h> /* regular expression support */

/* The regex lib from the system and the regex lib from PHP needs to
 * be separated here. We separate here via its directory location.
 */
#include "mapregex.h"


#define CPL_SUPRESS_CPLUSPLUS
#include "ogr_api.h"


/* EQUAL and EQUALN are defined in cpl_port.h, so add them in here if ogr was not included */

#ifndef EQUAL
#if defined(_WIN32) || defined(WIN32CE)
#  define EQUAL(a,b)              (stricmp(a,b)==0)
#else
#  define EQUAL(a,b)              (strcasecmp(a,b)==0)
#endif
#endif

#ifndef EQUALN
#if defined(_WIN32) || defined(WIN32CE)
#  define EQUALN(a,b,n)           (strnicmp(a,b,n)==0)
#else
#  define EQUALN(a,b,n)           (strncasecmp(a,b,n)==0)
#endif
#endif


#if defined(_WIN32) && !defined(__CYGWIN__)
#if (defined(_MSC_VER) && (_MSC_VER < 1900)) || !defined(_MSC_VER)
#define snprintf _snprintf
#endif
#endif

#ifdef __cplusplus
#include <string>
#include <vector>
extern "C" {
#endif

// hide from swig or ruby will choke on the __FUNCTION__ name
#ifndef SWIG
  /* Memory allocation check utility */
#ifndef __FUNCTION__
#   define __FUNCTION__ "MapServer"
#endif
#endif

#define MS_CHECK_ALLOC(var, size, retval)     \
    if (!var) {   \
        msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", __FUNCTION__, \
                   __FILE__, __LINE__, (unsigned int)(size));  \
        return retval;                         \
    }

#define MS_CHECK_ALLOC_NO_RET(var, size)                                   \
    if (!var) {                                                       \
        msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", __FUNCTION__, \
                   __FILE__, __LINE__, (unsigned int)(size));                           \
        return;                                                         \
    }

  /* General defines, wrapable */

#define MS_TRUE 1 /* logical control variables */
#define MS_FALSE 0
#define MS_UNKNOWN -1
#define MS_ON 1
#define MS_OFF 0
#define MS_DEFAULT 2
#define MS_EMBED 3
#define MS_DELETE 4
#define MS_YES 1
#define MS_NO 0


  /* Number of layer, class and style ptrs to alloc at once in the
     corresponding msGrow...() functions. Replaces former MS_MAXLAYERS,
     MS_MAXCLASSES and MS_MAXSTYLES with dynamic allocation (see RFC-17). */
#define MS_LAYER_ALLOCSIZE 64
#define MS_CLASS_ALLOCSIZE 8
#define MS_STYLE_ALLOCSIZE 4
#define MS_LABEL_ALLOCSIZE 2 /* not too common */

#define MS_MAX_LABEL_PRIORITY     10
#define MS_MAX_LABEL_FONTS     5
#define MS_DEFAULT_LABEL_PRIORITY 1
#define MS_LABEL_FORCE_GROUP 2 /* other values are MS_ON/MS_OFF */

  /* General defines, not wrapable */
#ifndef SWIG
#ifdef USE_XMLMAPFILE
#define MS_DEFAULT_MAPFILE_PATTERN "\\.(map|xml)$"
#define MS_DEFAULT_XMLMAPFILE_PATTERN "\\.xml$"
#else
#define MS_DEFAULT_MAPFILE_PATTERN "\\.map$"
#endif
#define MS_TEMPLATE_MAGIC_STRING "MapServer Template"
#define MS_TEMPLATE_EXPR "\\.(xml|wml|html|htm|svg|kml|gml|js|tmpl)$"

#define MS_INDEX_EXTENSION ".qix"

#define MS_QUERY_RESULTS_MAGIC_STRING "MapServer Query Results"
#define MS_QUERY_PARAMS_MAGIC_STRING "MapServer Query Params"
#define MS_QUERY_EXTENSION ".qy"

#define MS_DEG_TO_RAD .0174532925199432958
#define MS_RAD_TO_DEG   57.29577951

#define MS_DEFAULT_RESOLUTION 72

#define MS_RED 0
#define MS_GREEN 1
#define MS_BLUE 2

#define MS_MAXCOLORS 256

#define MS_MISSING_DATA_IGNORE 0
#define MS_MISSING_DATA_FAIL 1
#define MS_MISSING_DATA_LOG 2

#define MS_BUFFER_LENGTH 2048 /* maximum input line length */
#define MS_URL_LENGTH 1024
#define MS_MAXPATHLEN 1024

#define MS_MAXIMAGESIZE_DEFAULT 4096

#define MS_MAXPROJARGS 20
#define MS_MAXJOINS 20
#define MS_ITEMNAMELEN 32
#define MS_NAMELEN 20

#define MS_MINSYMBOLSIZE 0   /* in pixels */
#define MS_MAXSYMBOLSIZE 500

#define MS_MINSYMBOLWIDTH 0   /* in pixels */
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

#define MS_EXPRESSION 2000 /* todo: make this an enum */
#define MS_REGEX 2001
#define MS_STRING 2002
#define MS_NUMBER 2003
#define MS_COMMENT 2004
#define MS_IREGEX 2005
#define MS_ISTRING 2006
#define MS_BINDING 2007
#define MS_LIST 2008

  /* string split flags */
#define MS_HONOURSTRINGS      0x0001
#define MS_ALLOWEMPTYTOKENS   0x0002
#define MS_PRESERVEQUOTES     0x0004
#define MS_PRESERVEESCAPES    0x0008
#define MS_STRIPLEADSPACES    0x0010
#define MS_STRIPENDSPACES     0x0020

  /* boolean options for the expression object. */
#define MS_EXP_INSENSITIVE 1

  /* General macro definitions */
#define MS_MIN(a,b) (((a)<(b))?(a):(b))
#define MS_MAX(a,b) (((a)>(b))?(a):(b))
#define MS_ABS(a) (((a)<0) ? -(a) : (a))
#define MS_SGN(a) (((a)<0) ? -1 : 1)

#define MS_STRING_IS_NULL_OR_EMPTY(s) ((!s || s[0] == '\0') ? MS_TRUE:MS_FALSE)

#define MS_NINT_GENERIC(x) ((x) >= 0.0 ? ((long) ((x)+.5)) : ((long) ((x)-.5)))

#ifdef _MSC_VER
#define msIsNan(x) _isnan(x)
#else
#define msIsNan(x) isnan(x)
#endif

  /* see http://mega-nerd.com/FPcast/ for some discussion of fast
     conversion to nearest int.  We avoid lrint() for now because it
     would be hard to include math.h "properly". */

#if defined(HAVE_LRINT) && !defined(USE_GENERIC_MS_NINT)
#   define MS_NINT(x) lrint(x)
  /*#   define MS_NINT(x) lround(x) */
/* note that lrint rounds .5 to the nearest *even* integer, i.e. lrint(0.5)=0,lrint(1.5)=2 */
#elif defined(_MSC_VER) && defined(_WIN32) && !defined(USE_GENERIC_MS_NINT)
  static __inline long int MS_NINT (double flt)
  {
    int intgr;

    _asm {
      fld flt
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

  /* #define MS_VALID_EXTENT(minx, miny, maxx, maxy)  (((minx<maxx) && (miny<maxy))?MS_TRUE:MS_FALSE) */
#define MS_VALID_EXTENT(rect)  ((((rect).minx < (rect).maxx && (rect).miny < (rect).maxy))?MS_TRUE:MS_FALSE)
#define MS_VALID_SEARCH_EXTENT(rect)  ((((rect).minx <= (rect).maxx && (rect).miny <= (rect).maxy))?MS_TRUE:MS_FALSE)

#define MS_INIT_COLOR(color,r,g,b,a) { (color).red = r; (color).green = g; (color).blue = b; (color).alpha=a; }
#define MS_VALID_COLOR(color) (((color).red==-1 || (color).green==-1 || (color).blue==-1)?MS_FALSE:MS_TRUE)
#define MS_COMPARE_COLOR(color1, color2) (((color2).red==(color1).red && (color2).green==(color1).green && (color2).blue==(color1).blue)?MS_TRUE:MS_FALSE)
#define MS_TRANSPARENT_COLOR(color) (((color).alpha==0 || (color).red==-255 || (color).green==-255 || (color).blue==-255)?MS_TRUE:MS_FALSE)
#define MS_COMPARE_COLORS(a,b) (((a).red!=(b).red || (a).green!=(b).green || (a).blue!=(b).blue)?MS_FALSE:MS_TRUE)
#define MS_COLOR_GETRGB(color) (MS_VALID_COLOR(color)?((color).red *0x10000 + (color).green *0x100 + (color).blue):-1)

#define MS_IMAGE_MIME_TYPE(format) (format->mimetype ? format->mimetype : "unknown")
#define MS_IMAGE_EXTENSION(format)  (format->extension ? format->extension : "unknown")
#define MS_DRIVER_SWF(format) (strncasecmp((format)->driver,"swf",3)==0)
#define MS_DRIVER_GDAL(format)  (strncasecmp((format)->driver,"gdal/",5)==0)
#define MS_DRIVER_IMAGEMAP(format)  (strncasecmp((format)->driver,"imagemap",8)==0)
#define MS_DRIVER_AGG(format) (strncasecmp((format)->driver,"agg/",4)==0)
#define MS_DRIVER_MVT(format) (strncasecmp((format)->driver,"mvt",3)==0)
#define MS_DRIVER_CAIRO(format) (strncasecmp((format)->driver,"cairo/",6)==0)
#define MS_DRIVER_OGL(format) (strncasecmp((format)->driver,"ogl/",4)==0)
#define MS_DRIVER_TEMPLATE(format) (strncasecmp((format)->driver,"template",8)==0)

#endif /*SWIG*/

#define MS_RENDER_WITH_SWF      2
#define MS_RENDER_WITH_RAWDATA  3
#define MS_RENDER_WITH_IMAGEMAP 5
#define MS_RENDER_WITH_TEMPLATE 8 /* query results only */
#define MS_RENDER_WITH_OGR 16

#define MS_RENDER_WITH_PLUGIN 100
#define MS_RENDER_WITH_CAIRO_RASTER   101
#define MS_RENDER_WITH_CAIRO_PDF 102
#define MS_RENDER_WITH_CAIRO_SVG 103
#define MS_RENDER_WITH_OGL      104
#define MS_RENDER_WITH_AGG 105
#define MS_RENDER_WITH_KML 106
#define MS_RENDER_WITH_UTFGRID 107
#define MS_RENDER_WITH_MVT 108

#ifndef SWIG

#define MS_RENDERER_SWF(format) ((format)->renderer == MS_RENDER_WITH_SWF)
#define MS_RENDERER_RAWDATA(format) ((format)->renderer == MS_RENDER_WITH_RAWDATA)
#define MS_RENDERER_IMAGEMAP(format) ((format)->renderer == MS_RENDER_WITH_IMAGEMAP)
#define MS_RENDERER_TEMPLATE(format) ((format)->renderer == MS_RENDER_WITH_TEMPLATE)
#define MS_RENDERER_KML(format) ((format)->renderer == MS_RENDER_WITH_KML)
#define MS_RENDERER_OGR(format) ((format)->renderer == MS_RENDER_WITH_OGR)
#define MS_RENDERER_MVT(format) ((format)->renderer == MS_RENDER_WITH_MVT)

#define MS_RENDERER_PLUGIN(format) ((format)->renderer > MS_RENDER_WITH_PLUGIN)

#define MS_CELLSIZE(min,max,d) (((max) - (min))/((d)-1)) /* where min/max are from an MapServer pixel center-to-pixel center extent */
#define MS_OWS_CELLSIZE(min,max,d) (((max) - (min))/(d)) /* where min/max are from an OGC pixel outside edge-to-pixel outside edge extent */
#define MS_MAP2IMAGE_X(x,minx,cx) (MS_NINT(((x) - (minx))/(cx)))
#define MS_MAP2IMAGE_Y(y,maxy,cy) (MS_NINT(((maxy) - (y))/(cy)))
#define MS_IMAGE2MAP_X(x,minx,cx) ((minx) + (cx)*(x))
#define MS_IMAGE2MAP_Y(y,maxy,cy) ((maxy) - (cy)*(y))

  /* these versions of MS_MAP2IMAGE takes 1/cellsize and is much faster */
#define MS_MAP2IMAGE_X_IC(x,minx,icx) (MS_NINT(((x) - (minx))*(icx)))
#define MS_MAP2IMAGE_Y_IC(y,maxy,icy) (MS_NINT(((maxy) - (y))*(icy)))
#define MS_MAP2IMAGE_XCELL_IC(x,minx,icx) ((int)(((x) - (minx))*(icx)))
#define MS_MAP2IMAGE_YCELL_IC(y,maxy,icy) ((int)(((maxy) - (y))*(icy)))

#define MS_MAP2IMAGE_X_IC_DBL(x,minx,icx) (((x) - (minx))*(icx))
#define MS_MAP2IMAGE_Y_IC_DBL(y,maxy,icy) (((maxy) - (y))*(icy))

#define MS_MAP2IMAGE_X_IC_SNAP(x,minx,icx,res) ((MS_NINT(((x) - (minx))*(icx)*(res)))/(res))
#define MS_MAP2IMAGE_Y_IC_SNAP(y,maxy,icy,res) ((MS_NINT(((maxy) - (y))*(icy)*(res)))/(res))

  /* For CARTO symbols */
#define MS_PI    3.14159265358979323846
#define MS_PI2   1.57079632679489661923  /* (MS_PI / 2) */
#define MS_3PI2  4.71238898038468985769  /* (3 * MS_PI2) */
#define MS_2PI   6.28318530717958647693  /* (2 * MS_PI) */

#define MS_ENCRYPTION_KEY_SIZE  16   /* Key size: 128 bits = 16 bytes */

#define GET_LAYER(map, pos) map->layers[pos]
#define GET_CLASS(map, lid, cid) map->layers[lid]->class[cid]

#ifdef USE_THREAD
#if defined(HAVE_SYNC_FETCH_AND_ADD)
#define MS_REFCNT_INCR(obj) __sync_fetch_and_add(&obj->refcount, +1)
#define MS_REFCNT_DECR(obj) __sync_sub_and_fetch(&obj->refcount, +1)
#define MS_REFCNT_INIT(obj) obj->refcount=1, __sync_synchronize()
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#pragma intrinsic (_InterlockedExchangeAdd)
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
#define MS_REFCNT_INCR(obj) ( _InterlockedExchangeAdd((long*)(&obj->refcount), (long)(+1)) +1 )
#define MS_REFCNT_DECR(obj) ( _InterlockedExchangeAdd((long*)(&obj->refcount), (long)(-1)) -1 )
#define MS_REFCNT_INIT(obj) obj->refcount=1
#else
#define MS_REFCNT_INCR(obj) ( _InterlockedExchangeAdd((volatile long*)(&obj->refcount), (long)(+1)) +1 )
#define MS_REFCNT_DECR(obj) ( _InterlockedExchangeAdd((volatile long*)(&obj->refcount), (long)(-1)) -1 )
#define MS_REFCNT_INIT(obj) obj->refcount=1
#endif
#elif defined(__MINGW32__) && defined(__i386__)
#define MS_REFCNT_INCR(obj) ( InterlockedExchangeAdd((long*)(&obj->refcount), (long)(+1)) +1 )
#define MS_REFCNT_DECR(obj) ( InterlockedExchangeAdd((long*)(&obj->refcount), (long)(-1)) -1 )
#define MS_REFCNT_INIT(obj) obj->refcount=1
#else
  // unsafe fallback
#define MS_REFCNT_INCR(obj) obj->refcount++
#define MS_REFCNT_DECR(obj) (--(obj->refcount))
#define MS_REFCNT_INIT(obj) obj->refcount=1
#endif // close if defined(_MSC..
#else /*USE_THREAD*/
#define MS_REFCNT_INCR(obj) obj->refcount++
#define MS_REFCNT_DECR(obj) (--(obj->refcount))
#define MS_REFCNT_INIT(obj) obj->refcount=1
#endif /*USE_THREAD*/

#define MS_REFCNT_DECR_IS_NOT_ZERO(obj) (MS_REFCNT_DECR(obj))>0
#define MS_REFCNT_DECR_IS_ZERO(obj) (MS_REFCNT_DECR(obj))<=0

#define MS_IS_VALID_ARRAY_INDEX(index, size) ((index<0 || index>=size)?MS_FALSE:MS_TRUE)

#define MS_CONVERT_UNIT(src_unit, dst_unit, value) (value * msInchesPerUnit(src_unit,0) / msInchesPerUnit(dst_unit,0))

#define MS_INIT_INVALID_RECT { -1e300, -1e300, 1e300, 1e300 }

#endif

  enum MS_NUM_CHECK_TYPES { MS_NUM_CHECK_NONE=0, MS_NUM_CHECK_RANGE, MS_NUM_CHECK_GT, MS_NUM_CHECK_GTE };

  /* General enumerated types - needed by scripts */
  enum MS_FILE_TYPE {MS_FILE_MAP, MS_FILE_SYMBOL};
  enum MS_UNITS {MS_INCHES, MS_FEET, MS_MILES, MS_METERS, MS_KILOMETERS, MS_DD, MS_PIXELS, MS_PERCENTAGES, MS_NAUTICALMILES, MS_INHERIT = -1};
  enum MS_SHAPE_TYPE {MS_SHAPE_POINT, MS_SHAPE_LINE, MS_SHAPE_POLYGON, MS_SHAPE_NULL};
  enum MS_LAYER_TYPE {MS_LAYER_POINT, MS_LAYER_LINE, MS_LAYER_POLYGON, MS_LAYER_RASTER, MS_LAYER_ANNOTATION /* only used for parser backwards compatibility */, MS_LAYER_QUERY, MS_LAYER_CIRCLE, MS_LAYER_TILEINDEX, MS_LAYER_CHART};
  enum MS_FONT_TYPE {MS_TRUETYPE, MS_BITMAP};
  enum MS_RENDER_MODE {MS_FIRST_MATCHING_CLASS, MS_ALL_MATCHING_CLASSES};

#define MS_POSITIONS_LENGTH 14
  enum MS_POSITIONS_ENUM {MS_UL=101, MS_LR, MS_UR, MS_LL, MS_CR, MS_CL, MS_UC, MS_LC, MS_CC, MS_AUTO, MS_XY, MS_NONE, MS_AUTO2,MS_FOLLOW};
#define MS_TINY 5
#define MS_SMALL 7
#define MS_MEDIUM 10
#define MS_LARGE 13
#define MS_GIANT 16
  enum MS_QUERYMAP_STYLES {MS_NORMAL, MS_HILITE, MS_SELECTED};
  enum MS_CONNECTION_TYPE {MS_INLINE, MS_SHAPEFILE, MS_TILED_SHAPEFILE, MS_UNUSED_2, MS_OGR, MS_UNUSED_1, MS_POSTGIS, MS_WMS, MS_ORACLESPATIAL, MS_WFS, MS_GRATICULE, MS_MYSQL, MS_RASTER, MS_PLUGIN, MS_UNION, MS_UVRASTER, MS_CONTOUR, MS_KERNELDENSITY, MS_IDW, MS_FLATGEOBUF };
#define IS_THIRDPARTY_LAYER_CONNECTIONTYPE(type) ((type) == MS_UNION || (type) == MS_KERNELDENSITY || (type) == MS_IDW)
  enum MS_JOIN_CONNECTION_TYPE {MS_DB_XBASE, MS_DB_CSV, MS_DB_MYSQL, MS_DB_ORACLE, MS_DB_POSTGRES};
  enum MS_JOIN_TYPE {MS_JOIN_ONE_TO_ONE, MS_JOIN_ONE_TO_MANY};

#define MS_SINGLE 0 /* modes for searching (spatial/database) */
#define MS_MULTIPLE 1

  enum MS_QUERY_MODE {MS_QUERY_SINGLE, MS_QUERY_MULTIPLE};
  enum MS_QUERY_TYPE {MS_QUERY_IS_NULL, MS_QUERY_BY_POINT, MS_QUERY_BY_RECT, MS_QUERY_BY_SHAPE, MS_QUERY_BY_ATTRIBUTE, MS_QUERY_BY_INDEX, MS_QUERY_BY_FILTER};

  enum MS_ALIGN_VALUE {MS_ALIGN_DEFAULT, MS_ALIGN_LEFT, MS_ALIGN_CENTER, MS_ALIGN_RIGHT};

  enum MS_CAPS_JOINS_AND_CORNERS {MS_CJC_NONE, MS_CJC_BEVEL, MS_CJC_BUTT, MS_CJC_MITER, MS_CJC_ROUND, MS_CJC_SQUARE, MS_CJC_TRIANGLE};

#define MS_CJC_DEFAULT_CAPS MS_CJC_ROUND
#define MS_CJC_DEFAULT_JOINS MS_CJC_NONE
#define MS_CJC_DEFAULT_JOIN_MAXSIZE 3

  enum MS_RETURN_VALUE {MS_SUCCESS, MS_FAILURE, MS_DONE};
  enum MS_IMAGEMODE { MS_IMAGEMODE_PC256, MS_IMAGEMODE_RGB, MS_IMAGEMODE_RGBA, MS_IMAGEMODE_INT16, MS_IMAGEMODE_FLOAT32, MS_IMAGEMODE_BYTE, MS_IMAGEMODE_FEATURE, MS_IMAGEMODE_NULL };

  enum MS_GEOS_OPERATOR {MS_GEOS_EQUALS, MS_GEOS_DISJOINT, MS_GEOS_TOUCHES, MS_GEOS_OVERLAPS, MS_GEOS_CROSSES, MS_GEOS_INTERSECTS, MS_GEOS_WITHIN, MS_GEOS_CONTAINS, MS_GEOS_BEYOND, MS_GEOS_DWITHIN};
#define MS_FILE_DEFAULT MS_FILE_MAP

#if defined(_MSC_VER) && (_MSC_VER <= 1310)
#define MS_DEBUG msDebug2
#else
#ifdef USE_EXTENDED_DEBUG
#define MS_DEBUG(level,elt,fmt, ...) if((elt)->debug >= (level)) msDebug(fmt,##__VA_ARGS__)
#else
#define MS_DEBUG(level,elt,fmt, ...) /* no-op */
#endif
#endif

  /* coordinate to pixel simplification modes, used in msTransformShape */
  enum MS_TRANSFORM_MODE {
    MS_TRANSFORM_NONE, /* no geographic to pixel transformation */
    MS_TRANSFORM_ROUND, /* round to integer, might create degenerate geometries (used for GD)*/
    MS_TRANSFORM_SNAPTOGRID, /* snap to a grid, should be user configurable in the future*/
    MS_TRANSFORM_FULLRESOLUTION, /* keep full resolution */
    MS_TRANSFORM_SIMPLIFY /* keep full resolution */
  };

  typedef enum {
        MS_COMPOP_CLEAR,
        MS_COMPOP_SRC,
        MS_COMPOP_DST,
        MS_COMPOP_SRC_OVER,
        MS_COMPOP_DST_OVER,
        MS_COMPOP_SRC_IN,
        MS_COMPOP_DST_IN,
        MS_COMPOP_SRC_OUT,
        MS_COMPOP_DST_OUT,
        MS_COMPOP_SRC_ATOP,
        MS_COMPOP_DST_ATOP,
        MS_COMPOP_XOR,
        MS_COMPOP_PLUS,
        MS_COMPOP_MINUS,
        MS_COMPOP_MULTIPLY,
        MS_COMPOP_SCREEN,
        MS_COMPOP_OVERLAY,
        MS_COMPOP_DARKEN,
        MS_COMPOP_LIGHTEN,
        MS_COMPOP_COLOR_DODGE,
        MS_COMPOP_COLOR_BURN,
        MS_COMPOP_HARD_LIGHT,
        MS_COMPOP_SOFT_LIGHT,
        MS_COMPOP_DIFFERENCE,
        MS_COMPOP_EXCLUSION,
        MS_COMPOP_CONTRAST,
        MS_COMPOP_INVERT,
        MS_COMPOP_INVERT_RGB
  } CompositingOperation;

  typedef struct _CompositingFilter{
    char *filter;
    struct _CompositingFilter *next;
  } CompositingFilter;

  typedef struct _LayerCompositer{
    CompositingOperation comp_op;
    int opacity;
    CompositingFilter *filter;
    struct _LayerCompositer *next;
  } LayerCompositer;

#ifndef SWIG
  /* Filter object */
  typedef enum {
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
    FILTER_NODE_TYPE_GEOMETRY_POLYGON = 9,
    FILTER_NODE_TYPE_FEATUREID = 10,
    FILTER_NODE_TYPE_TEMPORAL = 11,
    FILTER_NODE_TYPE_TIME_PERIOD = 12
  } FilterNodeType;


  /************************************************************************/
  /*                          FilterEncodingNode                          */
  /************************************************************************/

  typedef struct _FilterNode {
    FilterNodeType      eType;
    char                *pszValue;
    void                *pOther;
    char                *pszSRS;
    struct _FilterNode  *psLeftNode;
    struct _FilterNode  *psRightNode;
  } FilterEncodingNode;
#endif /*SWIG*/

  /* Define supported bindings here (only covers existing bindings at first). Not accessible directly using MapScript. */
#define MS_STYLE_BINDING_LENGTH 12
  enum MS_STYLE_BINDING_ENUM { MS_STYLE_BINDING_SIZE, MS_STYLE_BINDING_WIDTH, MS_STYLE_BINDING_ANGLE, MS_STYLE_BINDING_COLOR, MS_STYLE_BINDING_OUTLINECOLOR, MS_STYLE_BINDING_SYMBOL, MS_STYLE_BINDING_OUTLINEWIDTH, MS_STYLE_BINDING_OPACITY, MS_STYLE_BINDING_OFFSET_X, MS_STYLE_BINDING_OFFSET_Y, MS_STYLE_BINDING_POLAROFFSET_PIXEL, MS_STYLE_BINDING_POLAROFFSET_ANGLE };
#define MS_LABEL_BINDING_LENGTH 12
  enum MS_LABEL_BINDING_ENUM { MS_LABEL_BINDING_SIZE, MS_LABEL_BINDING_ANGLE, MS_LABEL_BINDING_COLOR, MS_LABEL_BINDING_OUTLINECOLOR, MS_LABEL_BINDING_FONT, MS_LABEL_BINDING_PRIORITY, MS_LABEL_BINDING_POSITION, MS_LABEL_BINDING_SHADOWSIZEX, MS_LABEL_BINDING_SHADOWSIZEY, MS_LABEL_BINDING_OFFSET_X, MS_LABEL_BINDING_OFFSET_Y,MS_LABEL_BINDING_ALIGN };

  /************************************************************************/
  /*                         attributeBindingObj                          */
  /************************************************************************/
#ifndef SWIG
  typedef struct {
    char *item;
    int index;
  } attributeBindingObj;
#endif /*SWIG*/

  /************************************************************************/
  /*                             labelPathObj                             */
  /*                                                                      */
  /*      Label path object - used to hold path and bounds of curved      */
  /*      labels - Bug #1620 implementation.                              */
  /************************************************************************/
#ifndef SWIG
  typedef struct {
    multipointObj path;
    shapeObj bounds;
    double *angles;
  } labelPathObj;
#endif /*SWIG*/

  /************************************************************************/
  /*                              fontSetObj                              */
  /*                                                                      */
  /*      used to hold aliases for TRUETYPE fonts                         */
  /************************************************************************/

  /**
  The :ref:`FONTSET <fontset>` object
  */
  typedef struct {
#ifdef SWIG
    %immutable;
#endif
    char *filename; ///< The filename of the fonset
    int numfonts; ///< The number of fonts in the fontset
    hashTableObj fonts; ///< Key, value pairs of font name and font file
#ifdef SWIG
    %mutable;
#endif

#ifndef SWIG
    struct mapObj *map;
#endif
  } fontSetObj;

  /************************************************************************/
  /*                         featureListNodeObj                           */
  /*                                                                      */
  /*      for inline features, shape caches and queries                   */
  /************************************************************************/
#ifndef SWIG
  typedef struct listNode {
    shapeObj shape;
    struct listNode *next;
    struct listNode *tailifhead; /* this is the tail node in the list, if this is the head element, otherwise NULL */
  } featureListNodeObj;

  typedef featureListNodeObj * featureListNodeObjPtr;
#endif

  /************************************************************************/
  /*                              paletteObj                              */
  /*                                                                      */
  /*      used to hold colors while a map file is read                    */
  /************************************************************************/
#ifndef SWIG
  typedef struct {
    colorObj colors[MS_MAXCOLORS-1];
    int      colorvalue[MS_MAXCOLORS-1];
    int numcolors;
  } paletteObj;
#endif

  /************************************************************************/
  /*                     expressionObj & tokenObj                         */
  /************************************************************************/

  enum MS_TOKEN_LOGICAL_ENUM { MS_TOKEN_LOGICAL_AND=300, MS_TOKEN_LOGICAL_OR, MS_TOKEN_LOGICAL_NOT };
  enum MS_TOKEN_LITERAL_ENUM { MS_TOKEN_LITERAL_NUMBER=310, MS_TOKEN_LITERAL_STRING, MS_TOKEN_LITERAL_TIME, MS_TOKEN_LITERAL_SHAPE, MS_TOKEN_LITERAL_BOOLEAN };
  enum MS_TOKEN_COMPARISON_ENUM {
    MS_TOKEN_COMPARISON_EQ=320, MS_TOKEN_COMPARISON_NE, MS_TOKEN_COMPARISON_GT, MS_TOKEN_COMPARISON_LT, MS_TOKEN_COMPARISON_LE, MS_TOKEN_COMPARISON_GE, MS_TOKEN_COMPARISON_IEQ,
    MS_TOKEN_COMPARISON_RE, MS_TOKEN_COMPARISON_IRE,
    MS_TOKEN_COMPARISON_IN,
#ifdef MAPSERVER_BUILD
    MS_TOKEN_COMPARISON_LIKE_is_unused,
#else
    MS_TOKEN_COMPARISON_LIKE,
#endif
    MS_TOKEN_COMPARISON_INTERSECTS, MS_TOKEN_COMPARISON_DISJOINT, MS_TOKEN_COMPARISON_TOUCHES, MS_TOKEN_COMPARISON_OVERLAPS, MS_TOKEN_COMPARISON_CROSSES, MS_TOKEN_COMPARISON_WITHIN, MS_TOKEN_COMPARISON_CONTAINS, MS_TOKEN_COMPARISON_EQUALS, MS_TOKEN_COMPARISON_BEYOND, MS_TOKEN_COMPARISON_DWITHIN
  };
  enum MS_TOKEN_FUNCTION_ENUM {
    MS_TOKEN_FUNCTION_LENGTH=350, MS_TOKEN_FUNCTION_TOSTRING, MS_TOKEN_FUNCTION_COMMIFY, MS_TOKEN_FUNCTION_AREA, MS_TOKEN_FUNCTION_ROUND, MS_TOKEN_FUNCTION_FROMTEXT,
    MS_TOKEN_FUNCTION_BUFFER, MS_TOKEN_FUNCTION_DIFFERENCE, MS_TOKEN_FUNCTION_SIMPLIFY, MS_TOKEN_FUNCTION_SIMPLIFYPT, MS_TOKEN_FUNCTION_GENERALIZE, MS_TOKEN_FUNCTION_SMOOTHSIA, 
    MS_TOKEN_FUNCTION_CENTERLINE, MS_TOKEN_FUNCTION_DENSIFY, MS_TOKEN_FUNCTION_OUTER, MS_TOKEN_FUNCTION_INNER,
    MS_TOKEN_FUNCTION_JAVASCRIPT, MS_TOKEN_FUNCTION_UPPER, MS_TOKEN_FUNCTION_LOWER, MS_TOKEN_FUNCTION_INITCAP, MS_TOKEN_FUNCTION_FIRSTCAP
  };
  enum MS_TOKEN_BINDING_ENUM { MS_TOKEN_BINDING_DOUBLE=380, MS_TOKEN_BINDING_INTEGER, MS_TOKEN_BINDING_STRING, MS_TOKEN_BINDING_TIME, MS_TOKEN_BINDING_SHAPE, MS_TOKEN_BINDING_MAP_CELLSIZE, MS_TOKEN_BINDING_DATA_CELLSIZE };
  enum MS_PARSE_TYPE_ENUM { MS_PARSE_TYPE_BOOLEAN, MS_PARSE_TYPE_STRING, MS_PARSE_TYPE_SHAPE, MS_PARSE_TYPE_SLD };

#ifndef SWIG
  typedef union {
    int intval;
    char *strval;
    shapeObj *shpval;
  } parseResultObj;

  typedef union {
    double dblval;
    int intval;
    char *strval;
    struct tm tmval;
    shapeObj *shpval;
    attributeBindingObj bindval;
  } tokenValueObj;

  typedef struct tokenListNode {
    int token;
    tokenValueObj tokenval;
    char *tokensrc; /* on occassion we may want to access to the original source string (e.g. date/time) */
    struct tokenListNode *next;
    struct tokenListNode *tailifhead; /* this is the tail node in the list if this is the head element, otherwise NULL */
  } tokenListNodeObj;

  typedef tokenListNodeObj * tokenListNodeObjPtr;

  typedef struct {
    char *string;
    int type;
    /* container for expression options such as case-insensitiveness */
    /* This is a boolean container. */
    int flags;

    /* logical expression options */
    tokenListNodeObjPtr tokens;
    tokenListNodeObjPtr curtoken;

    /* regular expression options */
    ms_regex_t regex; /* compiled regular expression to be matched */
    int compiled;

    char *native_string; /* RFC 91 */
  } expressionObj;

  typedef struct {
    colorObj *pixel; /* for raster layers */
    shapeObj *shape; /* for vector layers */
    double dblval; /* for map cellsize used by simplify */
    double dblval2; /* for data cellsize */
    expressionObj *expr; /* expression to be evaluated (contains tokens) */
    int type; /* type of parse: boolean, string/text or shape/geometry */
    parseResultObj result; /* parse result */
  } parseObj;
#endif

  /************************************************************************/
  /*                          clusterObj                                  */
  /************************************************************************/

/**
The :ref:`CLUSTER <cluster>` object. See :ref:`RFC 69 <rfc69>`.
*/
  typedef struct {
    double maxdistance; ///< Maximum distance between clusters - see :ref:`MAXDISTANCE <mapfile-cluster-maxdistance>`
    double buffer; ///< The buffer size around the selection area - see :ref:`BUFFER <mapfile-cluster-buffer>`
    char* region; ///< The type of the cluster region (rectangle or ellipse) - see :ref:`REGION <mapfile-cluster-region>`
#ifndef SWIG
    expressionObj group; /* expression to identify the groups */
    expressionObj filter; /* expression for filtering the shapes */
#endif
  } clusterObj;

  /************************************************************************/
  /*                          processingParams                            */
  /************************************************************************/

#ifndef SWIG
/* Used by idw.c and kerneldensity.c */
  typedef struct {
      float normalization_scale;
      int expand_searchrect;
      int radius;
      float power;
  } interpolationProcessingParams;
#endif

  /************************************************************************/
  /*                               joinObj                                */
  /*                                                                      */
  /*      simple way to access other XBase files, one-to-one or           */
  /*      one-to-many supported                                           */
  /************************************************************************/

#ifndef SWIG
  typedef struct {
    char *name;
    char **items, **values; /* items/values (process 1 record at a time) */
    int numitems;

    char *table;
    char *from, *to; /* item names */

    void *joininfo; /* vendor specific (i.e. XBase, MySQL, etc.) stuff to allow for persistent access */

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

  /************************************************************************/
  /*                           outputFormatObj                            */
  /*                                                                      */
  /*      see mapoutput.c for most related code.                          */
  /************************************************************************/
 
/**
The :ref:`OUTPUTFORMAT <outputformat>` object
*/
  typedef struct {
#ifndef SWIG
    int refcount;
    char **formatoptions;
    rendererVTableObj *vtable;
    void *device; /* for supporting direct rendering onto a device context */
#endif /* SWIG */
#ifdef SWIG
    %immutable;
#endif /* SWIG */

    /**
    The number of option values set on this format - can be used to
    iterate over the options array in conjunction with :func:`outputFormatObj.getOptionAt`
    */
    int  numformatoptions;

#ifdef SWIG
    %mutable;
#endif /* SWIG */
    char *name; ///< See :ref:`NAME <mapfile-outputformat-name>`
    char *mimetype; ///< See :ref:`MIMETYPE <mapfile-outputformat-mimetype>`
    char *driver; ///< See :ref:`DRIVER <mapfile-outputformat-driver>`
    char *extension; ///< See :ref:`EXTENSION <mapfile-outputformat-extension>`
    int  renderer;  ///< A :ref:`render mode constant<mapfile-constants-render>` - normally set internally based on the driver and some other setting in the constructor.
    int  imagemode; ///< An :ref:`Image mode constant<mapfile-constants-imagemode>` - see :ref:`IMAGEMODE <mapfile-outputformat-imagemode>`
    int  transparent; ///< See :ref:`TRANSPARENT <mapfile-outputformat-transparent>`
    int  bands; ///< The number of bands in the raster, normally set via the BAND_COUNT formatoption - this field should be considered read-only
                ///< Only used for the "raw" modes, MS_IMAGEMODE_BYTE, MS_IMAGEMODE_INT16, and MS_IMAGEMODE_FLOAT32
    int inmapfile; ///< Boolean value indicating if the format is in the Mapfile
  } outputFormatObj;

  /* The following is used for "don't care" values in transparent, interlace and
     imagequality values. */
#define MS_NOOVERRIDE  -1111

  /************************************************************************/
  /*                             queryObj                                 */
  /*                                                                      */
  /*      encapsulates the information necessary to perform a query       */
  /************************************************************************/
#ifndef SWIG
  typedef struct {
    int type; /* MS_QUERY_TYPE */
    int mode; /* MS_QUERY_MODE */

    int layer;

    pointObj point; /* by point */
    double buffer;
    int maxresults;

    rectObj rect; /* by rect */
    shapeObj *shape; /* by shape & operator (OGC filter) */

    long shapeindex; /* by index */
    long tileindex;
    int clear_resultcache;

    int  maxfeatures; /* global maxfeatures */
    int  startindex;
    int  only_cache_result_count; /* set to 1 sometimes by WFS 2.0 GetFeature request */

    expressionObj filter; /* by filter */
    char *filteritem;

    int slayer; /* selection layer, used for msQueryByFeatures() (note this is not a query mode per se) */

    int cache_shapes; /* whether to cache shapes in resultCacheObj */
    int max_cached_shape_count; /* maximum number of shapes cached in the total number of resultCacheObj */
    int max_cached_shape_ram_amount; /* maximum number of bytes taken by shapes cached in the total number of resultCacheObj */
  } queryObj;
#endif

  /************************************************************************/
  /*                             queryMapObj                              */
  /*                                                                      */
  /*      used to visualize query results                                 */
  /************************************************************************/

/**
The :ref:`QUERYMAP <querymap>` object.
Instances of querymapObj are always are always embedded inside the :class:`mapObj`.
*/
  typedef struct {
#ifdef SWIG
    %immutable;
#endif /* SWIG */
    struct mapObj *map; ///< Reference to parent :class:`mapObj`                                                                                                                 
#ifdef SWIG
    %mutable;
#endif /* SWIG */
    int height; ///< See :ref:`SIZE <mapfile-querymap-size>`
    int width; ///< See :ref:`SIZE <mapfile-querymap-size>`
    int status; ///< See :ref:`STATUS <mapfile-querymap-status>`
    int style; ///< ``HILITE``, ``SELECTED`` or ``NORMAL`` - see :ref:`STYLE <mapfile-querymap-style>`
    colorObj color; ///< See :ref:`COLOR <mapfile-querymap-color>`
  } queryMapObj;

  /************************************************************************/
  /*                                webObj                                */
  /*                                                                      */
  /*      holds parameters for a mapserver/mapscript interface            */
  /************************************************************************/

/**
The :ref:`WEB <web>` object.
Has no other existence than as an attribute of a :class:`mapObj`. 
Serves as a container for various run-time web application definitions like temporary file paths, template paths, etc.
*/
  typedef struct {

#ifdef SWIG
      %immutable;
#endif /* SWIG */
      hashTableObj metadata; ///< Metadata hash table - see :ref:`METADATA <mapfile-web-metadata>`
      hashTableObj validation; ///< See :ref:`VALIDATION <mapfile-web-validation>`
      struct mapObj *map; ///< Reference to parent :class:`mapObj`

#ifdef SWIG
      %mutable;
#endif /* SWIG */

    char *imagepath; ///< Filesystem path to temporary image location - see :ref:`IMAGEPATH <mapfile-web-imagepath>`
    char *imageurl; ///< URL to temporary image location - see :ref:`IMAGEURL <mapfile-web-imageurl>`
    char *temppath; ///< See :ref:`TEMPPATH <mapfile-web-temppath>`
    char *header; ///< Path to header document - see :ref:`HEADER <mapfile-web-header>`
    char *footer; ///< Path to footer document - see :ref:`FOOTER <mapfile-web-footer>`
    char *empty; ///< See :ref:`EMPTY <mapfile-web-empty>`
    char *error; ///< Error handling - see :ref:`ERROR <mapfile-web-error>`

    double minscaledenom; ///< Maximum map scale - see :ref:`MINSCALEDENOM <mapfile-web-minscaledenom>`
    double maxscaledenom; ///< Minimum map scale - see :ref:`MAXSCALEDENOM <mapfile-web-maxscaledenom>`
    char *mintemplate; ///< See :ref:`MINTEMPLATE <mapfile-web-mintemplate>`
    char *maxtemplate; ///< See :ref:`MAXTEMPLATE <mapfile-web-maxtemplate>`

    char *queryformat; ///< See :ref:`QUERYFORMAT <mapfile-web-queryformat>` /* what format is the query to be returned, given as a MIME type */
    char *legendformat; ///< See :ref:`LEGENDFORMAT <mapfile-web-legendformat>`
    char *browseformat; ///< See :ref:`BROWSEFORMAT <mapfile-web-browseformat>`

#ifndef __cplusplus
    char *template; ///< Path to template document - see :ref:`TEMPLATE <mapfile-web-template>`
#else
    char *_template;
#endif

  } webObj;

  /************************************************************************/
  /*                               styleObj                               */
  /*                                                                      */
  /*      holds parameters for symbolization, multiple styles may be      */
  /*      applied within a classObj                                       */
  /************************************************************************/

/**
The :ref:`STYLE <style>` object. An instance of styleObj is associated with one instance of :class:`classObj`.
*/
  struct styleObj{

#ifndef SWIG
      /* private vars for rfc 48 & 64 */
      expressionObj _geomtransform;
      double scalefactor; // computed, not set
      attributeBindingObj bindings[MS_STYLE_BINDING_LENGTH];
      int numbindings;
      expressionObj exprBindings[MS_STYLE_BINDING_LENGTH];
      int nexprbindings;
#endif


#ifdef SWIG
    %immutable;
#endif /* SWIG */

    int refcount; ///< number of references to this object
    char *symbolname; ///< Name of the style's symbol - see :ref:`symbolname <mapfile-style-symbol>`

#ifdef SWIG
    %mutable;
#endif /* SWIG */

#if defined(SWIG) && defined(SWIGPYTHON) /* would probably make sense to mark it immutable for other binding languages than Python */
    %immutable;
#endif
    int patternlength; ///< Number of elements in the pattern attribute
#if defined(SWIG) && defined(SWIGPYTHON)
    %mutable;
#endif

#if !(defined(SWIG) && defined(SWIGPYTHON)) /* in Python we use a special typemap for this */
    double pattern[MS_MAXPATTERNLENGTH]; ///< List of on, off values to define a dash pattern for line work (lines, polygon outlines, hatch lines)
#endif

    double angle; ///< Angle, given in degrees, to draw the line work, default is 0, for symbols of Type HATCH, this is the angle of the 
                  ///< hatched lines - see :ref:`ANGLE <mapfile-style-angle>`
    /*should an angle be automatically computed*/
    int autoangle; ///< If the angle is set to ``AUTO`` - see :ref:`ANGLE <mapfile-style-angle>`

    /* should lines be drawn with antialiasing (default)? */
    int antialiased; ///< See :ref:`ANTIALIAS <mapfile-style-antialias>`

    colorObj color; ///< Foreground or fill pen color - see :ref:`COLOR <mapfile-style-color>`
    colorObj outlinecolor; ///< Outline pen color - see :ref:`OUTLINECOLOR <mapfile-style-outlinecolor>`

    int opacity; ///< See :ref:`OPACITY <mapfile-style-opacity>`

    /* Stuff to handle Color Range Styles */
    colorObj mincolor; ///< Minimum color in the :ref:`COLORRANGE <mapfile-style-colorrange>`
    colorObj maxcolor; ///< Maximum color in the :ref:`COLORRANGE <mapfile-style-colorrange>`
    double minvalue; ///< related to color ranges
    double maxvalue; ///< related to color ranges
    char *rangeitem; ///< Attribute/field that stores the values for the Color Range Mapping  - see :ref:`RANGEITEM <mapfile-style-rangeitem>`
    int rangeitemindex; ///< The index of the range item - see :ref:`RANGEITEM <mapfile-style-rangeitem>`

    int symbol; ///< The index within the map symbolset of the style's symbol - see :ref:`SYMBOL <mapfile-style-symbol>`
    double size; ///< Pixel width of the style's pen or symbol - see :ref:`SIZE <mapfile-style-size>`
    double minsize; ///< Minimum pen or symbol width for scaling styles - see :ref:`MINSIZE <mapfile-style-minsize>`
    double maxsize; ///< Maximum pen or symbol width for scaling - see :ref:`MAXSIZE <mapfile-style-maxsize>`

    double gap; ///< See :ref:`GAP <mapfile-style-gap>` - moved from symbolObj in version 6.0
    double initialgap; ///< See :ref:`INITIALGAP <mapfile-style-initialgap>`

    int linecap; ///< See :ref:`LINECAP <mapfile-style-linecap>`
    int linejoin; ///< See :ref:`LINEJOIN <mapfile-style-linejoin>` - moved from symbolObj in version 6.0
    double linejoinmaxsize; ///< See :ref:`LINEJOINMAXSIZE <mapfile-style-linejoinmaxsize>` - moved from symbolObj in version 6.0

    double width; ///< Width refers to the thickness of line work drawn, in pixels - default is 1,
                  ///< for symbols of type ``HATCH``, the with is how thick the hatched lines are - see :ref:`WIDTH <mapfile-style-width>`
    double outlinewidth; ///< See :ref:`OUTLINEWIDTH <mapfile-style-outlinewidth>`
    double minwidth; ///< Minimum width of the symbol - see :ref:`MINWIDTH <mapfile-style-minwidth>`
    double maxwidth; ///< Maximum width of the symbol - see :ref:`MAXWIDTH <mapfile-style-maxwidth>`

    double offsetx; ///< Draw with pen or symbol offset from map data, for shadows, hollow symbols, etc - see :ref:`OFFSET <mapfile-style-offset>`
    double offsety; ///< Draw with pen or symbol offset from map data, for shadows, hollow symbols, etc - see :ref:`OFFSET <mapfile-style-offset>`
    double polaroffsetpixel; ///< Specifies the radius/distance - see :ref:`POLAROFFSET <mapfile-style-polaroffset>`
    double polaroffsetangle; ///< Specified the angle - see :ref:`POLAROFFSET <mapfile-style-polaroffset>`


    double minscaledenom; ///< See :ref:`MINSCALEDENOM <mapfile-style-minscaledenom>`
    double maxscaledenom; ///< See :ref:`MAXSCALEDENOM <mapfile-style-maxscaledenom>`
    int sizeunits; ///< Supersedes class's :ref:`SIZEUNITS <mapfile-layer-sizeunits>` to allow fine-grained sizing for improved SLD (RFC 124)
  };

#define MS_STYLE_SINGLE_SIDED_OFFSET -99
#define MS_STYLE_DOUBLE_SIDED_OFFSET -999
#define IS_PARALLEL_OFFSET(offsety) ((offsety) == MS_STYLE_SINGLE_SIDED_OFFSET || (offsety) == MS_STYLE_DOUBLE_SIDED_OFFSET)

  /********************************************************************/
  /*                          labelLeaderObj                          */
  /*                                                                  */
  /*  parameters defining how a label or a group of labels may be     */
  /*  offsetted from its original position                            */
  /********************************************************************/

/**
The :ref:`LEADER <leader>` object
*/
  typedef struct {
#ifndef SWIG
    styleObj **styles;
    int maxstyles;
#endif
#ifdef SWIG
    %immutable;
#endif
    int numstyles; ///< Number of styles used
#ifdef SWIG
    %mutable;
#endif
    int maxdistance; ///< See :ref:`MAXDISTANCE <mapfile-leader-maxdistance>`
    int gridstep; ///< See :ref:`GRIDSTEP <mapfile-leader-gridstep>`
  } labelLeaderObj;


  /************************************************************************/
  /*                               labelObj                               */
  /*                                                                      */
  /*      parameters needed to annotate a layer, legend or scalebar       */
  /************************************************************************/

/**
The :ref:`LABEL <label>` object
*/
  struct labelObj{
#ifdef SWIG
    %immutable;
#endif /* SWIG */
    int refcount;
#ifdef SWIG
    %mutable;
#endif /* SWIG */

    char *font; ///< Name of TrueType font  see :ref:`FONT <mapfile-label-font>`
    colorObj color; ///< Foreground color - see :ref:`COLOR <mapfile-label-color>`
    colorObj outlinecolor; ///< Color of one point outline - see :ref:`OUTLINECOLOR <mapfile-label-outlinecolor>`
    int outlinewidth; ///< See :ref:`OUTLINEWIDTH <mapfile-label-outlinewidth>`

    colorObj shadowcolor; ///< See :ref:`SHADOWCOLOR <mapfile-label-shadowcolor>`
    int shadowsizex; ///< See :ref:`SHADOWSIZE <mapfile-label-shadowsize>`
    int shadowsizey; ///< See :ref:`SHADOWSIZE <mapfile-label-shadowsize>`

    int size; ///< Annotation height in pixels - see :ref:`SIZE <mapfile-label-size>`
    int minsize; ///< Minimum height in pixels - see :ref:`MINSIZE <mapfile-label-minsize>`
    int maxsize; ///< Maximum height in pixels for scaled labels. See :ref:`MAXSIZE <mapfile-label-maxsize>`

    int position; ///< See :ref:`POSTION <mapfile-label-position>`
    int offsetx; ///< Horizontal offset of label - see :ref:`OFFSET <mapfile-label-offset>`
    int offsety; ///< Vertical offset of label - see :ref:`OFFSET <mapfile-label-offset>`

    double angle; ///< See :ref:`ANGLE <mapfile-label-angle>`
    enum MS_POSITIONS_ENUM anglemode; ///< One of :data:`AUTO` :data:`AUTO2`, or :data:`FOLLOW` - see :ref:`ANGLE <mapfile-label-angle>`

    int buffer; ///< Space to reserve around a label (padding) - see :ref:`BUFFER <mapfile-label-buffer>`

    int align; ///< See :ref:`ALIGN <mapfile-label-align>`

    char wrap; ///< See :ref:`WRAP <mapfile-label-wrap>`
    int maxlength; ///< See :ref:`MAXLENGTH <mapfile-label-maxlength>`
    double space_size_10; ///< Cached size of a single space character used for label text alignment of rfc40

    /**
    Minimum feature size (in pixels) to label, features of this size or greater will be 
    labeled - see :ref:`MINFEATURESIZE  <mapfile-label-minfeaturesize>`
    */
    int minfeaturesize;
    int autominfeaturesize; ///< :data:`MS_TRUE` or :data:`MS_FALSE`

    double minscaledenom; ///< See :ref:`MINSCALEDENOM  <mapfile-label-minscaledenom>`
    double maxscaledenom; ///< See :ref:`MAXSCALEDENOM  <mapfile-label-maxscaledenom>`

    int mindistance; ///< Minimum distance in pixels between duplicate labels - see :ref:`MINDISTANCE <mapfile-label-mindistance>`
    int repeatdistance; ///< See :ref:`REPEATDISTANCE <mapfile-label-repeatdistance>`
    double maxoverlapangle; ///< See :ref:`MAXOVERLAPANGLE <mapfile-label-maxoverlapangle>`

    /**
    Indicates if labels can run off the edge of an image, either :data:`MS_TRUE` 
    or :data:`MS_FALSE` (default) - see :ref:`PARTIALS <mapfile-label-partials>`
    */
    int partials;

    int force; ///< Indicates if labels **must** be drawn - see :ref:`FORCE <mapfile-label-force>`

    char *encoding; ///< Supported encoding format to be used for labels - see :ref:`ENCODING <mapfile-label-encoding>`

    int priority; ///< Priority level 1 to :data:`MS_MAX_LABEL_PRIORITY`, default=1 - see :ref:`PRIORITY <mapfile-label-priority>`
    int numstyles; ///< The number of styles associated with the label

    int sizeunits; ///< Supersedes class's sizeunits, defaults to :data:`MS_INHERIT`.

#ifndef SWIG
    styleObj **styles;
    int maxstyles;

    expressionObj expression;
    expressionObj text;

    labelLeaderObj *leader;

    attributeBindingObj bindings[MS_LABEL_BINDING_LENGTH];
    int numbindings;
    expressionObj exprBindings[MS_LABEL_BINDING_LENGTH];
    int nexprbindings;

    double scalefactor; // computed, not set
#endif /* not SWIG */

  };

#ifdef SWIG
#ifdef	__cplusplus
extern "C" {
#endif
typedef struct labelObj labelObj;
#ifdef	__cplusplus
}
#endif
#endif

#ifndef SWIG
  /* lightweight structure containing information to render a labelObj */
  typedef struct {
    lineObj *poly;
    rectObj bbox;
  } label_bounds;

  typedef struct {
    labelObj *label;
    char *annotext;
    double scalefactor,resolutionfactor;
    pointObj annopoint;
    double rotation;
    textPathObj *textpath;
    //rectObj bbox;
    label_bounds **style_bounds;
  } textSymbolObj;
#endif

#define MS_LABEL_PERPENDICULAR_OFFSET -99
#define MS_LABEL_PERPENDICULAR_TOP_OFFSET 99
#define IS_PERPENDICULAR_OFFSET(offsety) ((offsety) == MS_LABEL_PERPENDICULAR_OFFSET || (offsety) == MS_LABEL_PERPENDICULAR_TOP_OFFSET)

  /************************************************************************/
  /*                          classObj                                    */
  /************************************************************************/

/**
 * The :ref:`CLASS <class>` object. Used for symbolization and classification information.
 *
 */
  struct classObj {

#ifdef SWIG
    %immutable;
#endif /* SWIG */
    
    hashTableObj metadata; ///< see :ref:`METADATA <mapfile-class-metadata>`
    hashTableObj validation; ///< see :ref:`VALIDATION <mapfile-class-validation>`
    int numstyles; ///< number of styles for class
    int numlabels;  ///< number of labels for class
    int refcount; ///< number of references to this object
    struct layerObj *layer; ///< reference to the parent layer
    labelLeaderObj *leader; ///< see :ref:`LEADER <mapfile-class-leader>`

#ifdef SWIG
    %mutable;
#endif

    int status; ///< :data:`MS_ON` or :data:`MS_OFF` - draw features of this class or do not - see :ref:`STATUS <mapfile-class-status>`
    int isfallback; ///< :data:`MS_TRUE` or :data:`MS_FALSE` use :data:`MS_TRUE` if this class should be applied if and only if no other class is applicable
    char *name; ///< See :ref:`NAME <mapfile-class-name>` should be unique within a layer
    char *title; ///< See :ref:`TITLE <mapfile-class-title>`
    double minscaledenom; ///< See :ref:`MINSCALEDENOM <mapfile-class-minscaledenom>`
    double maxscaledenom; ///< See :ref:`MAXSCALEDENOM <mapfile-CLASS-maxscaledenom>`
    int minfeaturesize; ///< See :ref:`MINFEATURESIZE <mapfile-class-minfeaturesize>` minimum feature size in pixels
    int debug; ///< :data:`MS_TRUE` or :data:`MS_FALSE` - see :ref:`DEBUG <mapfile-class-debug>`
    char *keyimage; ///< See :ref:`KEYIMAGE <mapfile-class-keyimage>`
    char *group; ///< See :ref:`GROUP <mapfile-class-group>`
    int sizeunits; ///< Supersedes layer's sizeunits and applies to all styles and labels - see :ref:`LAYER SIZEUNITS <mapfile-layer-sizeunits>`

#ifndef __cplusplus
    char *template; ///< See :ref:`TEMPLATE <mapfile-class-template>`
#else /* __cplusplus */
    char *_template; // keyword in cplusplus
#endif /* __cplusplus */

#ifndef SWIG
    expressionObj text;
    expressionObj expression; /* the expression to be matched */
    labelObj **labels;
    int maxlabels;
    int maxstyles;
    styleObj **styles;
    double scalefactor; // computed, not set
#endif /* not SWIG */

  };

  /************************************************************************/
  /*                         labelCacheMemberObj                          */
  /*                                                                      */
  /*      structures to implement label caching and collision             */
  /*      avoidance etc                                                   */
  /*                                                                      */
  /*        Note: These are scriptable, but are read only.                */
  /************************************************************************/


#ifdef SWIG
  %immutable;
#endif /* SWIG */
  /**
  An individual feature label. The labelCacheMemberObj class is associated with labelCacheObj.
  */
  typedef struct {
    int numtextsymbols; ///< Number of text symbols found in textsymbols
    int layerindex; ///< The index of the layer of the labelled feature
    int classindex; ///< Index of the class of the labelled feature
    int status; ///< Has this label been drawn or not?
    int markerid;  ///< Corresponding marker (POINT layers only)

    pointObj point; ///< Label point
    rectObj bbox; ///< Bounds of the whole cachePtr. Individual text and symbol sub bounds are found in textsymbols

    lineObj *leaderline; ///< Leader lineObj
    rectObj *leaderbbox; ///< Leader rectObj

#ifndef SWIG
    textSymbolObj **textsymbols;
#endif

  } labelCacheMemberObj;

  /************************************************************************/
  /*                         markerCacheMemberObj                         */
  /************************************************************************/
  /**
  An individual marker. The markerCacheMemberObj class is associated with labelCacheObj.
  */
  typedef struct {
    int id; ///< Corresponding label
    rectObj bounds; ///< Bounds of the markerCacheMemberObj
  } markerCacheMemberObj;

  /************************************************************************/
  /*                          labelCacheSlotObj                           */
  /************************************************************************/
  /**
  A cache slot to hold labels grouped by priority
  */
  typedef struct {
    int numlabels; ///< Number of label members
    int cachesize; ///< TODO
    int nummarkers; ///< Number of marker members
    int markercachesize; ///< TODO
    labelCacheMemberObj *labels;
    markerCacheMemberObj *markers;
  } labelCacheSlotObj;

  /************************************************************************/
  /*                            labelCacheObj                             */
  /************************************************************************/
  /**
  Set of a map's cached labels. Has no other existence other than as a labelcache
  attribute of a mapObj. Associated with labelCacheMemberObj and markerCacheMemberObj.
  */
  typedef struct {
    int num_rendered_members; ///< Number of rendered labels
#ifndef SWIG
    /* One labelCacheSlotObj for each priority level */
    labelCacheSlotObj slots[MS_MAX_LABEL_PRIORITY];
    int gutter; /* space in pixels around the image where labels cannot be placed */
    labelCacheMemberObj **rendered_text_symbols;
    int num_allocated_rendered_members;
#endif
  } labelCacheObj;

  /************************************************************************/
  /*                         resultObj                                    */
  /************************************************************************/
  /**
  The result object is a handle, of sorts, for a feature of the layer
  */
  typedef struct {
#ifndef SWIG
      shapeObj* shape;
#endif
    long shapeindex; ///< The shape index of the result
    int tileindex; ///< The tile index of the result
    int resultindex; ///< The index of the result
    int classindex; ///< The class index of the result
  } resultObj;

  /************************************************************************/
  /*                            resultCacheObj                            */
  /************************************************************************/
  /**
  A cached result object
  */
  typedef struct {

#ifndef SWIG
    resultObj *results;
    int cachesize;
    rectObj previousBounds; /* bounds at previous iteration */
#endif /* not SWIG */

#ifdef SWIG
    %immutable;
#endif /* SWIG */
    int numresults; ///< Length of result set
    rectObj bounds; ///< Bounding box of query results

#ifdef SWIG
    %mutable;
#endif /* SWIG */
  } resultCacheObj;


  /************************************************************************/
  /*                             symbolSetObj                             */
  /************************************************************************/
  /**
  A :class:`symbolSetObj` is an attribute of a :class:`mapObj` and is associated with instances of :class:`symbolObj`.
  */
  typedef struct {
#ifndef SWIG
      int refcount;
      symbolObj** symbol;
      struct mapObj *map;
      fontSetObj *fontset; /* a pointer to the main mapObj version */
      struct imageCacheObj *imagecache;
#endif /* not SWIG */

#ifdef SWIG
    %immutable;
#endif /* SWIG */
    int numsymbols; ///< Number of symbols in the set
    int maxsymbols; ///< Maximum number of allowed symbols
#ifdef SWIG
    %mutable;
#endif /* SWIG */

    char *filename; ///< Symbolset filename
    int imagecachesize; ///< Symbols in the cache

  } symbolSetObj;

  /************************************************************************/
  /*                           referenceMapObj                            */
  /************************************************************************/

/**
The :ref:`REFERENCE <reference>` object
*/
  typedef struct {
#ifdef SWIG
      %immutable;
#endif /* SWIG */
      struct mapObj *map; ///< Reference to parent :class:`mapObj`
#ifdef SWIG
      %mutable;
#endif /* SWIG */
    rectObj extent; ///< Spatial extent of reference in units of parent map - see :ref:`EXTENT <mapfile-reference-extent>`
    int height; ///< Height of reference map in pixels - see :ref:`SIZE <mapfile-reference-size>`
    int width; ///< Width of reference map in pixels - see :ref:`SIZE <mapfile-reference-size>`
    colorObj color; ///< Color of reference box - see :ref:`COLOR <mapfile-reference-color>`
    colorObj outlinecolor; ///< Outline color of reference box - see :ref:`OUTLINECOLOR <mapfile-reference-outlinecolor>`
    char *image; ///< Filename of reference map image - see :ref:`IMAGE <mapfile-reference-image>`
    int status; ///< :data:`MS_ON` or :data:`MS_OFF` - see :ref:`STATUS <mapfile-reference-status>`
    int marker; ///< Index of a symbol in the map symbol set to use for marker - see :ref:`MARKER <mapfile-reference-marker>`
    char *markername; ///< Name of a symbol - see :ref:`MARKER <mapfile-reference-marker>`
    int markersize; ///< Size of marker - see :ref:`MARKERSIZE <mapfile-reference-markersize>`
    int minboxsize; ///< In pixels - see :ref:`MINBOXSIZE <mapfile-reference-minboxsize>`
    int maxboxsize; ///< In pixels - see :ref:`MAXBOXSIZE <mapfile-reference-maxboxsize>`
  } referenceMapObj;

  /************************************************************************/
  /*                             scalebarObj                              */
  /************************************************************************/

  #define MS_SCALEBAR_INTERVALS_MIN 1
  #define MS_SCALEBAR_INTERVALS_MAX 100

  #define MS_SCALEBAR_WIDTH_MIN 5
  #define MS_SCALEBAR_WIDTH_MAX 1000
  #define MS_SCALEBAR_HEIGHT_MIN 2
  #define MS_SCALEBAR_HEIGHT_MAX 100

  #define MS_SCALEBAR_OFFSET_MIN -50
  #define MS_SCALEBAR_OFFSET_MAX 50
  
  /**
  The :ref:`SCALEBAR <scalebar>` object
  */
  typedef struct {

     int transparent; ///< Allows transparency for an embedded scalebar - see :ref:`TRANSPARENT <mapfile-scalebar-transparent>`
     colorObj imagecolor; ///< Background color of scalebar - see :ref:`IMAGECOLOR <mapfile-scalebar-imagecolor>`
      int height; ///< Height in pixels - see :ref:`SIZE <mapfile-scalebar-size>`
      int width; ///< Height in pixels - see :ref:`SIZE <mapfile-scalebar-size>`
      int style; ///< 0 or 1 - see :ref:`STYLE <mapfile-scalebar-style>`
      int intervals; ///< Number of intervals - see :ref:`INTERVALS <mapfile-scalebar-intervals>`
      labelObj label; ///< Scalebar label - see :ref:`LABEL <mapfile-scalebar-label>`
      colorObj color; ///< Scalebar foreground color - see :ref:`COLOR <mapfile-scalebar-color>`
      colorObj backgroundcolor; ///< Scalebar background color - see :ref:`BACKGROUNDCOLOR <mapfile-scalebar-backgroundcolor>`
      colorObj outlinecolor; ///< Foreground outline color - see :ref:`OUTLINECOLOR <mapfile-scalebar-outlinecolor>`
      int units; ///< See :ref:`UNITS <mapfile-scalebar-units>`
      int status; ///< ON, OFF or EMBED - see :ref:`STATUS <mapfile-scalebar-status>` - :data:`MS_ON`, :data:`MS_OFF`, or :data:`MS_EMBED`.
      int position; ///< For embeded scalebars - see :ref:`POSITION <mapfile-scalebar-position>` - 
                    ///< :data:`MS_UL`, :data:`MS_UC`, :data:`MS_UR`, :data:`MS_LL`, :data:`MS_LC`, or :data:`MS_LR`

      int postlabelcache; ///< See :ref:`POSTLABELCACHE <mapfile-scalebar-postlabelcache>` - :data:`MS_TRUE` or :data:`MS_FALSE`
      int align; ///< See :ref:`ALIGN <mapfile-scalebar-align>`
      int offsetx; ///< See :ref:`OFFSET <mapfile-scalebar-offset>`
      int offsety; ///< See :ref:`OFFSET <mapfile-scalebar-offset>`
  } scalebarObj;

  /************************************************************************/
  /*                              legendObj                               */
  /************************************************************************/

  #define MS_LEGEND_KEYSIZE_MIN 5
  #define MS_LEGEND_KEYSIZE_MAX 200
  #define MS_LEGEND_KEYSPACING_MIN 0
  #define MS_LEGEND_KEYSPACING_MAX 50

  /**
  The :ref:`LEGEND <legend>` object
  */
  typedef struct {

#ifdef SWIG
    %immutable;
#endif
    labelObj label; ///< See :ref:`LABEL <mapfile-legend-label>`
    struct mapObj *map; ///< Instances of legendObj are always are always embedded inside the mapObj
#ifdef SWIG
    %mutable;
#endif
    int transparent; ///< Allows transparency for an embedded legend - see :ref:`TRANSPARENT <mapfile-legend-transparent>`
    colorObj imagecolor; ///< Legend background color - see :ref:`IMAGECOLOR <mapfile-legend-imagecolor>`
    int keysizex; ///< Width in pixels of legend keys - see :ref:`KEYSIZE <mapfile-legend-keysize>`
    int keysizey; ///< Height in pixels of legend keys - see :ref:`KEYSIZE <mapfile-legend-keysize>`
    int keyspacingx; ///< Horizontal padding around keys in pixels - see :ref:`KEYSPACING <mapfile-legend-keyspacing>`
    int keyspacingy; ///< Vertical padding around keys in pixels - see :ref:`KEYSPACING <mapfile-legend-keyspacing>`
    colorObj outlinecolor; ///< Key outline color, -1 for no outline - see :ref:`OUTLINECOLOR <mapfile-legend-outlinecolor>`
    int status; ///< :data:`MS_ON`, :data:`MS_OFF` or :data:`MS_EMBED`- see :ref:`STATUS <mapfile-legend-status>`
    int height; ///< Legend height
    int width; ///< Legend width
    int position; ///< Used for embedded legends, see :ref:`POSITION  <mapfile-legend-position>`
    int postlabelcache; ///< :data:`MS_TRUE`, :data:`MS_FALSE` - see :ref:`POSTLABELCACHE  <mapfile-legend-postlabelcache>`
#ifndef __cplusplus
    char *template; ///< See :ref:`TEMPLATE <mapfile-legend-template>`
#else /* __cplusplus */
    char *_template;
#endif /* __cplusplus */
  } legendObj;

  /************************************************************************/
  /*                             graticuleObj                             */
  /************************************************************************/
#ifndef SWIG
  typedef struct {
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

  typedef struct {
    int nTop;
    pointObj *pasTop;
    char  **papszTopLabels;
    int nBottom;
    pointObj *pasBottom;
    char  **papszBottomLabels;
    int nLeft;
    pointObj *pasLeft;
    char  **papszLeftLabels;
    int nRight;
    pointObj *pasRight;
    char  **papszRightLabels;

  } graticuleIntersectionObj;

  struct layerVTable;
  typedef struct layerVTable layerVTableObj;

#endif /*SWIG*/

  /************************************************************************/
  /*                               imageObj                               */
  /************************************************************************/

/**
 * An image object is a wrapper for images generated by MapServer. 
 *
 */
  struct imageObj{
#ifdef SWIG
    %immutable;
#endif
    int width; ///< image width in pixels
    int height; ///< image height in pixels
    double resolution; ///< image resolution in pixels per inch
    double resolutionfactor; ///< resolution factor

    char *imagepath; ///< if image is drawn by :func:`mapObj.draw` this is :ref:`IMAGEPATH <mapfile-web-imagepath>`
    char *imageurl; ///< if image is drawn by :func:`mapObj.draw` this is :ref:`IMAGEURL <mapfile-web-imageurl>`
    outputFormatObj *format; ///< an :class:`outputFormatObj` representing the output format of the image

#ifndef SWIG
    tileCacheObj *tilecache;
    int ntiles;
    int size;
#endif
#ifdef SWIG
    %mutable;
#endif

#ifndef SWIG
    union {
      void *plugin;

      char *imagemap;
      short *raw_16bit;
      float *raw_float;
      unsigned char *raw_byte;
    } img;
    ms_bitarray  img_mask;
    pointObj refpt;
    mapObj *map;
#endif
  };

  /************************************************************************/
  /*                               layerObj                               */
  /*                                                                      */
  /*      base unit of a map.                                             */
  /************************************************************************/

  /**
  An individual value within the :ref:`SCALETOKEN <mapfile-layer-scaletoken>` object
  */
  typedef struct {
    double minscale; ///< The minimum scale for the replacement
    double maxscale; ///< The maximum scale for the replacement
    char *value; ///< The token replacement value
  } scaleTokenEntryObj;

  /**
  The :ref:`SCALETOKEN <mapfile-layer-scaletoken>` object
  */
  typedef struct {
     char *name; ///< The name of the token to replace in the :ref:`DATA <mapfile-layer-data>` statement
     int n_entries; ///< The number of values within the scaletoken
     scaleTokenEntryObj *tokens; ///< A reference to the values
  } scaleTokenObj;

#ifndef SWIG
  typedef enum {
      SORT_ASC,
      SORT_DESC
  } sortOrderEnum;

  typedef struct {
      char* item;
      sortOrderEnum sortOrder;
  } sortByProperties;

  typedef struct {
      int nProperties;
      sortByProperties* properties;
  } sortByClause;

  typedef struct {
    /* The following store original members if they have been modified at runtime by a rfc86 scaletoken */
    char *data;
    char *tileitem;
    char *tileindex;
    char *filteritem;
    char *filter;
    char **processing;
    int *processing_idx;
    int n_processing;
  } originalScaleTokenStrings;
#endif

  /**
  The :ref:`LAYER <layer>` object
  */
  struct layerObj {

#ifndef SWIG
    /* RFC86 Scale-dependent token replacements */
    scaleTokenObj *scaletokens;
    int numscaletokens;
    originalScaleTokenStrings *orig_st;
    int labelitemindex;

    int tileitemindex;
    projectionObj projection; /* projection information for the layer */
    int project; /* boolean variable, do we need to project this layer or not */
    reprojectionObj* reprojectorLayerToMap;
    reprojectionObj* reprojectorMapToLayer;

    featureListNodeObjPtr features; /* linked list so we don't need a counter */
    featureListNodeObjPtr currentfeature; /* pointer to the current feature */

    layerVTableObj *vtable;

    /* SDL has converted OracleSpatial, SDE, Graticules */
    void *layerinfo; /* all connection types should use this generic pointer to a vendor specific structure */
    void *wfslayerinfo; /* For WFS layers, will contain a msWFSLayerInfo struct */

    char **items;
    void *iteminfo; /* connection specific information necessary to retrieve values */
    expressionObj filter; /* connection specific attribute filter */
    int bandsitemindex;
    int filteritemindex;
    int styleitemindex;

    char **processing;
    joinObj *joins;

    imageObj *maskimage;
    graticuleObj* grid;
    expressionObj _geomtransform;
    sortByClause sortBy;

#endif

#ifndef SWIG
    int classitemindex;
    resultCacheObj *resultcache; /* holds the results of a query against this layer */
    double scalefactor; /* computed, not set */
#ifndef __cplusplus
    classObj **class; /* always at least 1 class */
#else /* __cplusplus */
    classObj **_class;
#endif /* __cplusplus */
#endif /* not SWIG */

#ifdef SWIG
    %immutable;
#endif /* SWIG */
    int refcount; ///< reference counting, RFC24
    int numclasses; ///< Number of layer classes
    int maxclasses; ///< Used to track of the maximum number of classes - see RFC-17
    int index; ///< Index of layer within parent map's layers array
    struct mapObj *map; ///< Reference to parent map
    int numitems; ///< Number of layer feature attributes (items)
    hashTableObj metadata; ///< See :ref:`METADATA <mapfile-layer-metadata>`
    hashTableObj validation; ///< See :ref:`VALIDATION <mapfile-layer-validation>`
    hashTableObj bindvals; ///< Relates to RFC59
    hashTableObj connectionoptions; ///< See :ref:`CONNECTIONOPTIONS <mapfile-layer-connectionoptions>`

    clusterObj cluster; ///< See :ref:`CLUSTER <mapfile-layer-cluster>`
    rectObj extent; ///< optional limiting extent for layer features - see :ref:`EXTENT <mapfile-layer-extent>`
    int numprocessing; ///< Number of raster processing directives
    int numjoins; ///< Number of layer joins

    expressionObj utfdata; ///< See :ref:`UTFDATA <mapfile-layer-utfdata>`
    LayerCompositer *compositer; ///< See :ref:`COMPOSITE <mapfile-layer-composite>`

#ifdef SWIG
    %mutable;
#endif /* SWIG */

    char *classitem; ///< the attribute used to classify layer data - see :ref:`CLASSITEM <mapfile-layer-classitem>`
    char *header; ///< filename to a template for result's header - see :ref:`HEADER <mapfile-layer-header>`
    char *footer; ///< filename to a template for result's footer - see :ref:`FOOTER <mapfile-layer-footer>`

#ifndef __cplusplus
    char *template; ///< See :ref:`TEMPLATE <mapfile-layer-template>` - global template, used across all classes
#else /* __cplusplus */
    char *_template;
#endif /* __cplusplus */

    char *name; ///< See :ref:`NAME <mapfile-layer-name>` - should be unique
    char *group; ///< Name of a group of layers - see :ref:`GROUP <mapfile-layer-group>`
    int status; ///< See :ref:`STATUS <mapfile-layer-status>`:data:`MS_ON`, :data:`MS_OFF` or :data:`MS_DEFAULT`
    enum MS_RENDER_MODE rendermode; ///< :data:`MS_FIRST_MATCHING_CLASS` - default and historic MapServer behavior, :data:`MS_ALL_MATCHING_CLASSES` - SLD behavior
    char *data; ///< Layer data definition, values depend upon connectiontype - see :ref:`DATA <mapfile-layer-data>`
    enum MS_LAYER_TYPE type; ///< the layer type - see :ref:`TYPE <mapfile-layer-type>`
    enum MS_CONNECTION_TYPE connectiontype; ///< the layer connection type - see :ref:`CONNECTIONTYPE <mapfile-layer-connectiontype>`

    double tolerance; ///< search buffer for point and line queries (in toleranceunits) - see :ref:`TOLERANCE <mapfile-layer-tolerance>`
    int toleranceunits; ///< See :ref:`TOLERANCEUNITS <mapfile-layer-toleranceunits>`

    double symbolscaledenom; ///< scale at which symbols are default size - see :ref:`SYMBOLSCALEDENOM <mapfile-layer-symbolscaledenom>`
    double minscaledenom; ///< Maximum scale at which layer will be drawn - see :ref:`MINSCALEDENOM <mapfile-layer-minscaledenom>`
    double maxscaledenom; ///< Minimum scale at which layer will be drawn - see :ref:`MAXSCALEDENOM <mapfile-layer-maxscaledenom>`
    int minfeaturesize; ///< minimum feature size (in pixels) for shape
    double labelminscaledenom; ///< See :ref:`LABELMINSCALEDENOM <mapfile-layer-labelminscaledenom>`
    double labelmaxscaledenom; ///< See :ref:`LABELMAXSCALEDENOM <mapfile-layer-labelmaxscaledenom>`
    double mingeowidth; ///< min map width (in map units) at which the layer should be drawn - see :ref:`MAXGEOWIDTH <mapfile-layer-maxgeowidth>`
    double maxgeowidth; ///< max map width (in map units) at which the layer should be drawn - see :ref:`MAXGEOWIDTH <mapfile-layer-maxgeowidth>`

    int sizeunits; ///< applies to all classes - see :ref:`SIZEUNITS <mapfile-layer-sizeunits>`

    int maxfeatures; ///< Maximum number of layer features that will be drawn - see :ref:`MAXFEATURES <mapfile-layer-maxfeatures>`
    int startindex; ///< Feature start index - used for paging

    colorObj offsite; ///< transparent pixel value for raster images - see :ref:`OFFSITE <mapfile-layer-offsite>`

    /**
    :data:`MS_TRUE` (default) or :data:`MS_FALSE` whether or not layer data is to be transformed to 
    image units - see :ref:`TRANSFORM <mapfile-layer-transform>`
    */
    int transform;

    int labelcache; ///< :data:`MS_ON` (default) or :data:`MS_OFF` - see :ref:`LABELCACHE <mapfile-layer-labelcache>`
    int postlabelcache; ///< :data:`MS_ON` or :data:`MS_OFF` (default) - see :ref:`POSTLABELCACHE <mapfile-layer-postlabelcache>`

    char *labelitem; ///< attribute defining feature label text - see :ref:`LABELITEM <mapfile-layer-labelitem>`
    char *tileitem; ///< attribute defining tile paths - see :ref:`TILEITEM <mapfile-layer-tileitem>`
    char *tileindex; ///< layer index file for tiling support - see :ref:`TILEINDEX <mapfile-layer-tileindex>`
    char *tilesrs; ///< name of the attribute that contains the SRS of an individual tile - see :ref:`TILESRS <mapfile-layer-tilesrs>`
    int units; ///< units of the layer - see :ref:`UNITS <mapfile-layer-units>`

    char *connection; ///< layer connection or data source name - see :ref:`CONNECTION <mapfile-layer-connection>`
    char *plugin_library; ///< Used to select the library to load by MapServer
    char *plugin_library_original; ///< this is needed for Mapfile writing

    /**
    The attribute from the index file used to select the source raster band(s) to be 
    used - normally NULL for default bands processing
    */
    char *bandsitem;

    char *filteritem; ///< Attribute defining filter - see :ref:`FILTERITEM <mapfile-layer-filteritem>`
    char *styleitem; ///< item to be used for style lookup - can also be 'AUTO' - see :ref:`STYLEITEM <mapfile-layer-styleitem>`

    char *requires; ///< Context expressions, simple enough to not use expressionObj - see :ref:`REQUIRES <mapfile-layer-requires>`
    char *labelrequires;  ///< Simple logical expression - see :ref:`LABELREQUIRES <mapfile-layer-labelrequires>`

    int debug; ///< Enable debugging of layer-  :data:`MS_ON` or :data:`MS_OFF` (default) - see :ref:`DEBUG <mapfile-layer-debug>`
    char *classgroup; ///< See :ref:`CLASSGROUP <mapfile-layer-classgroup>`
    char *mask; ///< See :ref:`MASK <mapfile-layer-mask>`
    char *encoding; ///< For iconving shape attributes, ignored if NULL or "utf-8" - see :ref:`ENCODING <mapfile-layer-encoding>`

    /* RFC93 UTFGrid support */
    char *utfitem; ///< See :ref:`UTFITEM <mapfile-layer-utfitem>`
    int utfitemindex; ///< See :ref:`CLASSITEM <mapfile-layer-classitem>`
  };


#ifndef SWIG
void msFontCacheSetup();
void msFontCacheCleanup();

typedef struct {
  double minx,miny,maxx,maxy,advance;
} glyph_metrics;

typedef struct {
  glyph_element *glyph;
  face_element *face;
  pointObj pnt;
  double rot;
} glyphObj;

struct textPathObj{
  int numglyphs;
  int numlines;
  int line_height;
  int glyph_size;
  int absolute; /* are the glyph positions absolutely placed, or relative to the origin */
  glyphObj *glyphs;
  label_bounds bounds;
};

typedef enum {
  duplicate_never,
  duplicate_always,
  duplicate_if_needed
} label_cache_mode;

void initTextPath(textPathObj *tp);
int WARN_UNUSED msComputeTextPath(mapObj *map, textSymbolObj *ts);
void msCopyTextPath(textPathObj *dst, textPathObj *src);
void freeTextPath(textPathObj *tp);
void initTextSymbol(textSymbolObj *ts);
void freeTextSymbol(textSymbolObj *ts);
void freeTextSymbolEx(textSymbolObj *ts, int doFreeLabel);
void copyLabelBounds(label_bounds *dst, label_bounds *src);
void msCopyTextSymbol(textSymbolObj *dst, textSymbolObj *src);
void msPopulateTextSymbolForLabelAndString(textSymbolObj *ts, labelObj *l, char *string, double scalefactor, double resolutionfactor, label_cache_mode cache);
#endif /* SWIG */


  /************************************************************************/
  /*                                mapObj                                */
  /*                                                                      */
  /*      encompasses everything used in an Internet mapping              */
  /*      application.                                                    */
  /************************************************************************/

  #define MS_RESOLUTION_MAX 1000 /* applies to resolution and defresolution */
  #define MS_RESOLUTION_MIN 10

  /**
  The :ref:`MAP <map>` object
  */
  struct mapObj { /* structure for a map */

#ifndef SWIG
      layerObj **layers;
      geotransformObj gt; /* rotation / geotransform */
      rectObj saved_extent;
      paletteObj palette; /* holds a map palette */
      outputFormatObj **outputformatlist;
      projectionObj projection; /* projection information for output map */
      projectionObj latlon; /* geographic projection definition */

      /* Private encryption key information - see mapcrypto.c */
      int encryption_key_loaded;        /* MS_TRUE once key has been loaded */
      unsigned char encryption_key[MS_ENCRYPTION_KEY_SIZE]; /* 128bits encryption key */
      queryObj query;
      projectionContext* projContext;

#endif /* SWIG */

#ifdef SWIG
      %immutable;
#endif /* SWIG */
      int refcount; ///< Used for reference counting see RFC24
      int numlayers; ///< Number of layers in mapfile
      int maxlayers; ///< Allocated size of layers[] array

      /**
      A hash table of configuration options from CONFIG keywords 
      in the map - see :ref:`CONFIG <mapfile-map-config>`
      */
      hashTableObj configoptions;

      symbolSetObj symbolset; ///< See :ref:`SYMBOLSET <mapfile-map-symbolset>`
      fontSetObj fontset; ///< See :ref:`FONTSET <mapfile-map-fontset>`

      labelCacheObj labelcache; ///< We need this here so multiple feature processors can access it
      int numoutputformats; ///< Number of output formats available in the map
      outputFormatObj *outputformat; ///< See :ref:`OUTPUTFORMAT <mapfile-map-outputformat>`
      char *imagetype; ///< Name of current outputformat

      referenceMapObj reference; ///< See :ref:`SCALEBAR <mapfile-map-scalebar>`
      scalebarObj scalebar; ///< See :ref:`SCALEBAR <mapfile-map-scalebar>`
      legendObj legend; ///< See :ref:`LEGEND <mapfile-map-legend>`
      queryMapObj querymap; ///< See :ref:`QUERYMAP <mapfile-map-querymap>`
      webObj web; ///< See :ref:`WEB <mapfile-map-web>`

      const configObj *config;
#ifdef SWIG
    %mutable;
#endif /* SWIG */

    char *datapattern; ///< TODO - Deprecated use VALIDATION ... END block instead
    char *templatepattern; ///< TODO - Deprecated use VALIDATION ... END block instead

    char *name; ///< Small identifier for naming etc - see :ref:`NAME <mapfile-map-name>`
    int status;  ///< Is map creation on or off - see :ref:`STATUS <mapfile-map-status>`
    int height; ///< See :ref:`SIZE <mapfile-map-size>`
    int width; ///< See :ref:`SIZE <mapfile-map-size>`
    int maxsize; ///< See :ref:`MAXSIZE  <mapfile-map-maxsize>`

    rectObj extent; ///< Map extent array - see :ref:`EXTENT <mapfile-map-extent>`
    double cellsize; ///< Pixel size in map units

    enum MS_UNITS units; ///< Units of the projection - see :ref:`UNITS <mapfile-map-units>`
    double scaledenom; ///< The nominal map scale, a value of 25000 means 1:25000 scale - see :ref:`SCALEDENOM  <mapfile-map-scaledenom>`
    double resolution; ///< See :ref:`RESOLUTION <mapfile-map-resolution>`
    double defresolution; ///< Default resolution - used to calculate the scalefactor, see :ref:`DEFRESOLUTION <mapfile-map-defresolution>`

    char *shapepath; ///< Where are the shape files located - see :ref:`SHAPEPATH <mapfile-map-shapepath>`
    char *mappath; ///< Path of the mapfile, all paths are relative to this path

    /**
    URL of SLD document as specified with "&SLD=..." WMS parameter d- currently this reference is 
    used only in mapogcsld.c and has a NULL value outside that context
    */
    char *sldurl;

    colorObj imagecolor; ///< Holds the initial image color value - see :ref:`IMAGECOLOR <mapfile-map-imagecolor>`

    int *layerorder; ///< Used to modify the order in which the layers are drawn - TODO should be immutable?
    int debug; ///< See :ref:`DEBUG <mapfile-map-debug>`

#ifdef USE_V8_MAPSCRIPT
    void *v8context;
#endif

  };

  /************************************************************************/
  /*                             layerVTable                              */
  /*                                                                      */
  /*      contains function pointers to the layer operations.  If you     */
  /*      add new functions to here, remember to update                   */
  /*      populateVirtualTable in maplayer.c                              */
  /************************************************************************/
#ifndef SWIG
  struct layerVTable {
    int (*LayerTranslateFilter)(layerObj *layer, expressionObj *filter, char *filteritem);
    int (*LayerSupportsCommonFilters)(layerObj *layer);
    int (*LayerInitItemInfo)(layerObj *layer);
    void (*LayerFreeItemInfo)(layerObj *layer);
    int (*LayerOpen)(layerObj *layer);
    int (*LayerIsOpen)(layerObj *layer);
    int (*LayerWhichShapes)(layerObj *layer, rectObj rect, int isQuery);
    int (*LayerNextShape)(layerObj *layer, shapeObj *shape);
    int (*LayerGetShape)(layerObj *layer, shapeObj *shape, resultObj *record);
    int (*LayerGetShapeCount)(layerObj *layer, rectObj rect, projectionObj *rectProjection);
    int (*LayerClose)(layerObj *layer);
    int (*LayerGetItems)(layerObj *layer);
    int (*LayerGetExtent)(layerObj *layer, rectObj *extent);
    int (*LayerGetAutoStyle)(mapObj *map, layerObj *layer, classObj *c, shapeObj *shape);
    int (*LayerCloseConnection)(layerObj *layer);
    int (*LayerSetTimeFilter)(layerObj *layer, const char *timestring, const char *timefield);
    int (*LayerApplyFilterToLayer)(FilterEncodingNode *psNode, mapObj *map, int iLayerIndex);
    int (*LayerCreateItems)(layerObj *layer, int nt);
    int (*LayerGetNumFeatures)(layerObj *layer);
    int (*LayerGetAutoProjection)(layerObj *layer, projectionObj *projection);
    char* (*LayerEscapeSQLParam)(layerObj *layer, const char* pszString);
    char* (*LayerEscapePropertyName)(layerObj *layer, const char* pszString);
    void (*LayerEnablePaging)(layerObj *layer, int value);
    int (*LayerGetPaging)(layerObj *layer);
  };
#endif /*SWIG*/

  /* Function prototypes, wrapable */

  /**
  Saves a map image to a file
  */
  MS_DLL_EXPORT int msSaveImage(mapObj *map, imageObj *img, const char *filename); 

  /**
  Generic function to free a imageObj
  */
  MS_DLL_EXPORT void msFreeImage(imageObj *img);

  /**
  Sets up threads and font cache - called when MapScript is initialised
  */
  MS_DLL_EXPORT int msSetup(void);

  /**
  Attempts to recover all dynamically allocated resources allocated by MapServer code and
  dependent libraries. It is used primarily for final clean-up in scripts that need to do
  memory leak testing to get rid of "noise" one-time allocations.
  It should not normally be used by production code.
  */
  MS_DLL_EXPORT void msCleanup(void);

  /**
  Sets up string-based mapfile loading and calls loadMapInternal to do the work
  */
  MS_DLL_EXPORT mapObj *msLoadMapFromString(char *buffer, char *new_mappath, const configObj* config);

  /* Function prototypes, not wrapable */

#ifndef SWIG

  /*
  ** helper functions not part of the general API but needed in
  ** a few other places (like mapscript)... found in mapfile.c
  */
  int getString(char **s);
  int getDouble(double *d, int num_check_type, double value1, double value2); // getDouble(double *d);
  int getInteger(int *i, int num_check_type, int value1, int value2); // getInteger(int *i);
  int getSymbol(int n, ...);
  int getCharacter(char *c);

  int msBuildPluginLibraryPath(char **dest, const char *lib_str, mapObj *map);

  MS_DLL_EXPORT int  hex2int(char *hex);

  MS_DLL_EXPORT void initJoin(joinObj *join);
  MS_DLL_EXPORT void initSymbol(symbolObj *s);
  MS_DLL_EXPORT int initMap(mapObj *map);
  MS_DLL_EXPORT layerObj *msGrowMapLayers( mapObj *map );
  MS_DLL_EXPORT int initLayer(layerObj *layer, mapObj *map);
  MS_DLL_EXPORT int freeLayer( layerObj * );
  MS_DLL_EXPORT classObj *msGrowLayerClasses( layerObj *layer );
  MS_DLL_EXPORT scaleTokenObj *msGrowLayerScaletokens( layerObj *layer );
  MS_DLL_EXPORT int initScaleToken(scaleTokenObj *scaleToken);
  MS_DLL_EXPORT int initClass(classObj *_class);
  MS_DLL_EXPORT int freeClass( classObj * );
  MS_DLL_EXPORT styleObj *msGrowClassStyles( classObj *_class );
  MS_DLL_EXPORT labelObj *msGrowClassLabels( classObj *_class );
  MS_DLL_EXPORT styleObj *msGrowLabelStyles( labelObj *label );
  MS_DLL_EXPORT styleObj *msGrowLeaderStyles( labelLeaderObj *leader );
  MS_DLL_EXPORT int msMaybeAllocateClassStyle(classObj* c, int idx);
  MS_DLL_EXPORT void initLabel(labelObj *label);
  MS_DLL_EXPORT int  freeLabel(labelObj *label);
  MS_DLL_EXPORT int  freeLabelLeader(labelLeaderObj *leader);
  MS_DLL_EXPORT void resetClassStyle(classObj *_class);
  MS_DLL_EXPORT int initStyle(styleObj *style);
  MS_DLL_EXPORT int freeStyle(styleObj *style);
  MS_DLL_EXPORT void initReferenceMap(referenceMapObj *ref);
  MS_DLL_EXPORT void initScalebar(scalebarObj *scalebar);
  MS_DLL_EXPORT void initGrid( graticuleObj *pGraticule );
  MS_DLL_EXPORT void initWeb(webObj *web);
  MS_DLL_EXPORT void freeWeb(webObj *web);
  MS_DLL_EXPORT void initResultCache(resultCacheObj *resultcache);
  void cleanupResultCache(resultCacheObj *resultcache);
  MS_DLL_EXPORT int initLayerCompositer(LayerCompositer *compositer);
  MS_DLL_EXPORT void initLeader(labelLeaderObj *leader);
  MS_DLL_EXPORT void freeGrid( graticuleObj *pGraticule);
  int loadHashTable(hashTableObj *ptable); // used by other file loading code

  MS_DLL_EXPORT featureListNodeObjPtr insertFeatureList(featureListNodeObjPtr *list, shapeObj *shape);
  MS_DLL_EXPORT void freeFeatureList(featureListNodeObjPtr list);

  /* To be used *only* within the mapfile loading phase */
  MS_DLL_EXPORT int loadExpressionString(expressionObj *exp, char *value);
  /* Use this next, thread safe wrapper, function everywhere else */
  MS_DLL_EXPORT int msLoadExpressionString(expressionObj *exp, char *value);
  MS_DLL_EXPORT char *msGetExpressionString(expressionObj *exp);
  MS_DLL_EXPORT void msInitExpression(expressionObj *exp);
  MS_DLL_EXPORT void msFreeExpressionTokens(expressionObj *exp);
  MS_DLL_EXPORT void msFreeExpression(expressionObj *exp);

  MS_DLL_EXPORT void msApplySubstitutions(mapObj *map, char **names, char **values, int npairs);
  MS_DLL_EXPORT void msApplyDefaultSubstitutions(mapObj *map);

  MS_DLL_EXPORT int getClassIndex(layerObj *layer, char *str);

  /* For maplabel */
  int intersectLabelPolygons(lineObj *p1, rectObj *r1, lineObj *p2, rectObj *r2);
  int intersectTextSymbol(textSymbolObj *ts, label_bounds *lb);
  pointObj get_metrics(pointObj *p, int position, textPathObj *tp, int ox, int oy, double rotation, int buffer, label_bounds *metrics);
  double dist(pointObj a, pointObj b);
  void fastComputeBounds(lineObj *line, rectObj *bounds);

  /*
  ** Main API Functions
  */

  /* mapobject.c */

  MS_DLL_EXPORT void msFreeMap(mapObj *map);
  MS_DLL_EXPORT mapObj *msNewMapObj(void);
  MS_DLL_EXPORT const char *msGetConfigOption( mapObj *map, const char *key);
  MS_DLL_EXPORT int msSetConfigOption( mapObj *map, const char *key, const char *value);
  MS_DLL_EXPORT int msTestConfigOption( mapObj *map, const char *key,
                                        int default_result );
  MS_DLL_EXPORT void msApplyMapConfigOptions( mapObj *map );
  MS_DLL_EXPORT int msMapComputeGeotransform( mapObj *map );
  int msMapComputeGeotransformEx( mapObj * map, double resolutionX, double resolutionY );

  MS_DLL_EXPORT void msMapPixelToGeoref( mapObj *map, double *x, double *y );
  MS_DLL_EXPORT void msMapGeorefToPixel( mapObj *map, double *x, double *y );

  MS_DLL_EXPORT int msMapSetExtent(mapObj *map, double minx, double miny,
                                   double maxx, double maxy);
  MS_DLL_EXPORT int msMapOffsetExtent( mapObj *map, double x, double y);
  MS_DLL_EXPORT int msMapScaleExtent( mapObj *map, double zoomfactor,
                                      double minscaledenom, double maxscaledenom);
  MS_DLL_EXPORT int msMapSetCenter( mapObj *map, pointObj *center);
  MS_DLL_EXPORT int msMapSetRotation( mapObj *map, double rotation_angle );
  MS_DLL_EXPORT int msMapSetSize( mapObj *map, int width, int height );
  MS_DLL_EXPORT int msMapSetSize( mapObj *map, int width, int height );
  MS_DLL_EXPORT int msMapSetFakedExtent( mapObj *map );
  MS_DLL_EXPORT int msMapRestoreRealExtent( mapObj *map );
  MS_DLL_EXPORT int msMapLoadOWSParameters( mapObj *map, cgiRequestObj *request,
      const char *wmtver_string );
  MS_DLL_EXPORT int msMapIgnoreMissingData( mapObj *map );

  /* mapfile.c */

  MS_DLL_EXPORT int msValidateParameter(const char *value, const char *pattern1, const char *pattern2, const char *pattern3, const char *pattern4);
  MS_DLL_EXPORT int msGetLayerIndex(mapObj *map, const char *name);
  MS_DLL_EXPORT int msGetSymbolIndex(symbolSetObj *set, char *name, int try_addimage_if_notfound);
  MS_DLL_EXPORT mapObj  *msLoadMap(const char *filename, const char *new_mappath, const configObj *config);
  MS_DLL_EXPORT int msTransformXmlMapfile(const char *stylesheet, const char *xmlMapfile, FILE *tmpfile);
  MS_DLL_EXPORT int msSaveMap(mapObj *map, char *filename);
  MS_DLL_EXPORT int msSaveConfig(configObj *map, const char *filename);
  MS_DLL_EXPORT void msFreeCharArray(char **array, int num_items);
  MS_DLL_EXPORT int msUpdateScalebarFromString(scalebarObj *scalebar, char *string);
  MS_DLL_EXPORT int msUpdateQueryMapFromString(queryMapObj *querymap, char *string);
  MS_DLL_EXPORT int msUpdateLabelFromString(labelObj *label, char *string);
  MS_DLL_EXPORT int msUpdateClusterFromString(clusterObj *cluster, char *string);
  MS_DLL_EXPORT int msUpdateReferenceMapFromString(referenceMapObj *ref, char *string);
  MS_DLL_EXPORT int msUpdateLegendFromString(legendObj *legend, char *string);
  MS_DLL_EXPORT int msUpdateWebFromString(webObj *web, char *string);
  MS_DLL_EXPORT int msUpdateStyleFromString(styleObj *style, char *string);
  MS_DLL_EXPORT int msUpdateClassFromString(classObj *_class, char *string);
  MS_DLL_EXPORT int msUpdateLayerFromString(layerObj *layer, char *string);
  MS_DLL_EXPORT char *msWriteLayerToString(layerObj *layer);
  MS_DLL_EXPORT char *msWriteMapToString(mapObj *map);
  MS_DLL_EXPORT char *msWriteClassToString(classObj *_class);
  MS_DLL_EXPORT char *msWriteStyleToString(styleObj *style);
  MS_DLL_EXPORT char *msWriteLabelToString(labelObj *label);
  MS_DLL_EXPORT char *msWriteWebToString(webObj *web);
  MS_DLL_EXPORT char *msWriteScalebarToString(scalebarObj *scalebar);
  MS_DLL_EXPORT char *msWriteQueryMapToString(queryMapObj *querymap);
  MS_DLL_EXPORT char *msWriteReferenceMapToString(referenceMapObj *ref);
  MS_DLL_EXPORT char *msWriteLegendToString(legendObj *legend);
  MS_DLL_EXPORT char *msWriteClusterToString(clusterObj *cluster);
  MS_DLL_EXPORT int msIsValidRegex(const char* e);
  MS_DLL_EXPORT int msEvalRegex(const char *e, const char *s);
  MS_DLL_EXPORT int msCaseEvalRegex(const char *e, const char *s);
#ifdef USE_MSFREE
  MS_DLL_EXPORT void msFree(void *p);
#else
#define msFree free
#endif
  MS_DLL_EXPORT char **msTokenizeMap(char *filename, int *numtokens);
  MS_DLL_EXPORT int msInitLabelCache(labelCacheObj *cache);
  MS_DLL_EXPORT int msFreeLabelCache(labelCacheObj *cache);
  MS_DLL_EXPORT int msCheckConnection(layerObj * layer); /* connection pooling functions (mapfile.c) */
  MS_DLL_EXPORT void msCloseConnections(mapObj *map);

  MS_DLL_EXPORT void msOGRInitialize(void);
  MS_DLL_EXPORT void msOGRCleanup(void);
  MS_DLL_EXPORT void msGDALCleanup(void);
  MS_DLL_EXPORT void msGDALInitialize(void);

  MS_DLL_EXPORT imageObj *msDrawScalebar(mapObj *map); /* in mapscale.c */
  MS_DLL_EXPORT int msCalculateScale(rectObj extent, int units, int width, int height, double resolution, double *scaledenom);
  MS_DLL_EXPORT double GetDeltaExtentsUsingScale(double scale, int units, double centerLat, int width, double resolution);
  MS_DLL_EXPORT double Pix2Georef(int nPixPos, int nPixMin, int nPixMax, double dfGeoMin, double dfGeoMax, int bULisYOrig);
  MS_DLL_EXPORT double Pix2LayerGeoref(mapObj *map, layerObj *layer, int value);
  MS_DLL_EXPORT double msInchesPerUnit(int units, double center_lat);
  MS_DLL_EXPORT int msEmbedScalebar(mapObj *map, imageObj *img);

  MS_DLL_EXPORT int msPointInRect(const pointObj *p, const rectObj *rect); /* in mapsearch.c */
  MS_DLL_EXPORT int msRectOverlap(const rectObj *a, const rectObj *b);
  MS_DLL_EXPORT int msRectContained(const rectObj *a, const rectObj *b);
  MS_DLL_EXPORT int msRectIntersect(rectObj *a, const rectObj *b);

  MS_DLL_EXPORT void msRectToFormattedString(rectObj *rect, char *format,
      char *buffer, int buffer_length);
  MS_DLL_EXPORT void msPointToFormattedString(pointObj *point, const char*format,
      char *buffer, int buffer_length);
  MS_DLL_EXPORT int msIsDegenerateShape(shapeObj *shape);

  MS_DLL_EXPORT void msMergeRect(rectObj *a, rectObj *b);
  MS_DLL_EXPORT double msDistancePointToPoint(pointObj *a, pointObj *b);
  MS_DLL_EXPORT double msSquareDistancePointToPoint(pointObj *a, pointObj *b);
  MS_DLL_EXPORT double msDistancePointToSegment(pointObj *p, pointObj *a, pointObj *b);
  MS_DLL_EXPORT double msSquareDistancePointToSegment(pointObj *p, pointObj *a, pointObj *b);
  MS_DLL_EXPORT double msDistancePointToShape(pointObj *p, shapeObj *shape);
  MS_DLL_EXPORT double msSquareDistancePointToShape(pointObj *p, shapeObj *shape);
  MS_DLL_EXPORT double msDistanceSegmentToSegment(pointObj *pa, pointObj *pb, pointObj *pc, pointObj *pd);
  MS_DLL_EXPORT double msDistanceShapeToShape(shapeObj *shape1, shapeObj *shape2);
  MS_DLL_EXPORT int msIntersectSegments(const pointObj *a, const pointObj *b, const pointObj *c, const pointObj *d);
  MS_DLL_EXPORT int msPointInPolygon(pointObj *p, lineObj *c);
  MS_DLL_EXPORT int msIntersectMultipointPolygon(shapeObj *multipoint, shapeObj *polygon);
  MS_DLL_EXPORT int msIntersectPointPolygon(pointObj *p, shapeObj *polygon);
  MS_DLL_EXPORT int msIntersectPolylinePolygon(shapeObj *line, shapeObj *poly);
  MS_DLL_EXPORT int msIntersectPolygons(shapeObj *p1, shapeObj *p2);
  MS_DLL_EXPORT int msIntersectPolylines(shapeObj *line1, shapeObj *line2);

  MS_DLL_EXPORT int msInitQuery(queryObj *query); /* in mapquery.c */
  MS_DLL_EXPORT void msFreeQuery(queryObj *query);
  MS_DLL_EXPORT int msSaveQuery(mapObj *map, char *filename, int results);
  MS_DLL_EXPORT int msLoadQuery(mapObj *map, char *filename);
  MS_DLL_EXPORT int msExecuteQuery(mapObj *map);

  MS_DLL_EXPORT int msQueryByIndex(mapObj *map); /* various query methods, all rely on the queryObj hung off the mapObj */
  MS_DLL_EXPORT int msQueryByAttributes(mapObj *map);
  MS_DLL_EXPORT int msQueryByPoint(mapObj *map);
  MS_DLL_EXPORT int msQueryByRect(mapObj *map);
  MS_DLL_EXPORT int msQueryByFeatures(mapObj *map);
  MS_DLL_EXPORT int msQueryByShape(mapObj *map);
  MS_DLL_EXPORT int msQueryByFilter(mapObj *map);

  MS_DLL_EXPORT int msGetQueryResultBounds(mapObj *map, rectObj *bounds);
  MS_DLL_EXPORT int msIsLayerQueryable(layerObj *lp);
  MS_DLL_EXPORT void msQueryFree(mapObj *map, int qlayer); /* todo: rename */
  MS_DLL_EXPORT int msRasterQueryByShape(mapObj *map, layerObj *layer, shapeObj *selectshape);
  MS_DLL_EXPORT int msRasterQueryByRect(mapObj *map, layerObj *layer, rectObj queryRect);
  MS_DLL_EXPORT int msRasterQueryByPoint(mapObj *map, layerObj *layer, int mode, pointObj p, double buffer, int maxresults );

  /* in mapstring.c */
  MS_DLL_EXPORT void msStringTrim(char *str);
  MS_DLL_EXPORT void msStringTrimBlanks(char *string);
  MS_DLL_EXPORT char *msStringTrimLeft(char *string);
  MS_DLL_EXPORT char *msStringChop(char *string);
  MS_DLL_EXPORT void msStringTrimEOL(char *string);
  MS_DLL_EXPORT char *msReplaceSubstring(char *str, const char *old, const char *sznew);
  MS_DLL_EXPORT void msReplaceChar(char *str, char old, char sznew);
  MS_DLL_EXPORT char *msCaseReplaceSubstring(char *str, const char *old, const char *sznew);
  MS_DLL_EXPORT char *msStripPath(char *fn);
  MS_DLL_EXPORT char *msGetPath(const char *fn);
  MS_DLL_EXPORT char *msBuildPath(char *pszReturnPath, const char *abs_path, const char *path);
  MS_DLL_EXPORT char *msBuildPath3(char *pszReturnPath, const char *abs_path, const char *path1, const char *path2);
  MS_DLL_EXPORT char *msTryBuildPath(char *szReturnPath, const char *abs_path, const char *path);
  MS_DLL_EXPORT char *msTryBuildPath3(char *szReturnPath, const char *abs_path, const char *path1, const char *path2);
  MS_DLL_EXPORT char **msStringSplit(const char *string, char cd, int *num_tokens);
  MS_DLL_EXPORT char ** msStringSplitComplex( const char * pszString, const char * pszDelimiters, int *num_tokens, int nFlags);
  MS_DLL_EXPORT int msStringArrayContains(char **array, const char *element, int numElements);
  MS_DLL_EXPORT char **msStringTokenize( const char *pszLine, const char *pszDelim, int *num_tokens, int preserve_quote);
  MS_DLL_EXPORT int msCountChars(char *str, char ch);
  MS_DLL_EXPORT char *msLongToString(long value);
  MS_DLL_EXPORT char *msDoubleToString(double value, int force_f);
  MS_DLL_EXPORT char *msIntToString(int value);
  MS_DLL_EXPORT void msStringToUpper(char *string);
  MS_DLL_EXPORT void msStringToLower(char *string);
  MS_DLL_EXPORT void msStringInitCap(char *string);
  MS_DLL_EXPORT void msStringFirstCap(char *string);
  MS_DLL_EXPORT int msEncodeChar(const char);
  MS_DLL_EXPORT char *msEncodeUrlExcept(const char*, const char);
  MS_DLL_EXPORT char *msEncodeUrl(const char*);
  MS_DLL_EXPORT char *msEscapeJSonString(const char* pszJSonString);
  MS_DLL_EXPORT char *msEncodeHTMLEntities(const char *string);
  MS_DLL_EXPORT void msDecodeHTMLEntities(const char *string);
  MS_DLL_EXPORT int msIsXMLTagValid(const char *string);
  MS_DLL_EXPORT char *msStringConcatenate(char *pszDest, const char *pszSrc);
  MS_DLL_EXPORT char *msJoinStrings(char **array, int arrayLength, const char *delimeter);
  MS_DLL_EXPORT char *msHashString(const char *pszStr);
  MS_DLL_EXPORT char *msCommifyString(char *str);
  MS_DLL_EXPORT int msHexToInt(char *hex);
  MS_DLL_EXPORT char *msGetEncodedString(const char *string, const char *encoding);
  MS_DLL_EXPORT char *msConvertWideStringToUTF8 (const wchar_t* string, const char* encoding);
  MS_DLL_EXPORT wchar_t *msConvertWideStringFromUTF8 (const char* string, const char* encoding);
  MS_DLL_EXPORT int msGetNextGlyph(const char **in_ptr, char *out_string);
  MS_DLL_EXPORT int msGetNumGlyphs(const char *in_ptr);
  MS_DLL_EXPORT int msGetUnicodeEntity(const char *inptr, unsigned int *unicode);
  MS_DLL_EXPORT int msStringIsInteger(const char *string);
  MS_DLL_EXPORT int msUTF8ToUniChar(const char *str, unsigned int *chPtr); /* maptclutf.c */
  MS_DLL_EXPORT char* msStringEscape( const char * pszString );
  MS_DLL_EXPORT int msStringInArray( const char * pszString, char **array, int numelements);

  typedef struct msStringBuffer msStringBuffer;
  MS_DLL_EXPORT msStringBuffer* msStringBufferAlloc(void);
  MS_DLL_EXPORT void msStringBufferFree(msStringBuffer* sb);
  MS_DLL_EXPORT const char* msStringBufferGetString(msStringBuffer* sb);
  MS_DLL_EXPORT char* msStringBufferReleaseStringAndFree(msStringBuffer* sb);
  MS_DLL_EXPORT int msStringBufferAppend(msStringBuffer* sb, const char* pszAppendedString);

  MS_DLL_EXPORT int msStringToInt(const char *str, int *value, int base);
  MS_DLL_EXPORT int msStringToDouble(const char *str, double *value);

#ifdef __cplusplus
}
  std::string msStdStringEscape( const char * pszString );
  void msStringTrim(std::string& string);
  void msStringTrimBlanks(std::string& string);
  void msStringTrimLeft(std::string& string);
  std::vector<std::string> msStringSplit(const char *string, char cd);
  std::string msStringToLower(const std::string& s);
  bool msStringInArray( const char * pszString, const std::vector<std::string>& array);
extern "C" {
#endif

#ifndef HAVE_STRRSTR
  MS_DLL_EXPORT char *strrstr(const char *string, const char *find);
#endif /* NEED_STRRSTR */

#ifndef HAVE_STRCASESTR
  MS_DLL_EXPORT char *strcasestr(const char *s, const char *find);
#endif /* NEED_STRCASESTR */

#ifndef HAVE_STRNCASECMP
  MS_DLL_EXPORT int strncasecmp(const char *s1, const char *s2, size_t len);
#endif /* NEED_STRNCASECMP */

#ifndef HAVE_STRCASECMP
  MS_DLL_EXPORT int strcasecmp(const char *s1, const char *s2);
#endif /* NEED_STRCASECMP */

#ifndef HAVE_STRLCAT
  MS_DLL_EXPORT size_t strlcat(char *dst, const char *src, size_t siz);
#endif /* NEED_STRLCAT */

#ifndef HAVE_STRLCPY
  MS_DLL_EXPORT size_t strlcpy(char *dst, const char *src, size_t siz);
#endif /* NEED_STRLCAT */

  MS_DLL_EXPORT char *msStrdup( const char * pszString );

#include "hittest.h"

  /* in mapsymbol.c */
  /* Use this function *only* with mapfile loading phase */
  MS_DLL_EXPORT int loadSymbolSet(symbolSetObj *symbolset, mapObj *map);
  /* Use this threadsafe wrapper everywhere else */
  MS_DLL_EXPORT int msLoadSymbolSet(symbolSetObj *symbolset, mapObj *map);
  MS_DLL_EXPORT int msCopySymbol(symbolObj *dst, const symbolObj *src, mapObj *map);
  MS_DLL_EXPORT int msCopySymbolSet(symbolSetObj *dst, const symbolSetObj *src, mapObj *map);
  MS_DLL_EXPORT int msCopyHashTable(hashTableObj *dst, const hashTableObj *src);
  MS_DLL_EXPORT void msInitSymbolSet(symbolSetObj *symbolset);
  MS_DLL_EXPORT symbolObj *msGrowSymbolSet( symbolSetObj *symbolset );
  MS_DLL_EXPORT int msAddImageSymbol(symbolSetObj *symbolset, char *filename);
  MS_DLL_EXPORT int msFreeSymbolSet(symbolSetObj *symbolset);
  MS_DLL_EXPORT int msFreeSymbol(symbolObj *symbol);
  MS_DLL_EXPORT int msAddNewSymbol(mapObj *map, char *name);
  MS_DLL_EXPORT int msAppendSymbol(symbolSetObj *symbolset, symbolObj *symbol);
  MS_DLL_EXPORT symbolObj *msRemoveSymbol(symbolSetObj *symbolset, int index);
  MS_DLL_EXPORT int msSaveSymbolSet(symbolSetObj *symbolset, const char *filename);
  MS_DLL_EXPORT int msLoadImageSymbol(symbolObj *symbol, const char *filename);
  MS_DLL_EXPORT int msPreloadImageSymbol(rendererVTableObj *renderer, symbolObj *symbol);
  MS_DLL_EXPORT int msPreloadSVGSymbol(symbolObj *symbol);
  MS_DLL_EXPORT symbolObj *msRotateSymbol(symbolObj *symbol, double angle);

  MS_DLL_EXPORT int WARN_UNUSED msGetCharacterSize(mapObj *map, char* font, int size, char *character, rectObj *r);
  MS_DLL_EXPORT int WARN_UNUSED msGetMarkerSize(mapObj *map, styleObj *style, double *width, double *height, double scalefactor);
  /* MS_DLL_EXPORT int msGetCharacterSize(char *character, int size, char *font, rectObj *rect); */
  MS_DLL_EXPORT double msSymbolGetDefaultSize(symbolObj *s);
  MS_DLL_EXPORT void freeImageCache(struct imageCacheObj *ic);

  MS_DLL_EXPORT imageObj WARN_UNUSED *msDrawLegend(mapObj *map, int scale_independent, map_hittest *hittest); /* in maplegend.c */
  MS_DLL_EXPORT int WARN_UNUSED msLegendCalcSize(mapObj *map, int scale_independent, int *size_x, int *size_y,
                                     int *alayers, int numl_ayer, map_hittest *hittest, double resolutionfactor);
  MS_DLL_EXPORT int WARN_UNUSED msEmbedLegend(mapObj *map, imageObj *img);
  MS_DLL_EXPORT int WARN_UNUSED msDrawLegendIcon(mapObj* map, layerObj* lp, classObj* myClass, int width, int height, imageObj *img, int dstX, int dstY, int scale_independant, class_hittest *hittest);
  MS_DLL_EXPORT imageObj WARN_UNUSED *msCreateLegendIcon(mapObj* map, layerObj* lp, classObj* myClass, int width, int height, int scale_independant);

  MS_DLL_EXPORT int msLoadFontSet(fontSetObj *fontSet, mapObj *map); /* in maplabel.c */
  MS_DLL_EXPORT int msInitFontSet(fontSetObj *fontset);
  MS_DLL_EXPORT int msFreeFontSet(fontSetObj *fontset);

  MS_DLL_EXPORT int WARN_UNUSED msGetTextSymbolSize(mapObj *map, textSymbolObj *ts, rectObj *r);
  MS_DLL_EXPORT int WARN_UNUSED msGetStringSize(mapObj *map, labelObj *label, int size, char *string, rectObj *r);

  MS_DLL_EXPORT int WARN_UNUSED msAddLabel(mapObj *map, imageObj *image, labelObj *label, int layerindex, int classindex, shapeObj *shape, pointObj *point, double featuresize, textSymbolObj *ts);
  MS_DLL_EXPORT int WARN_UNUSED msAddLabelGroup(mapObj *map, imageObj *image, layerObj *layer, int classindex, shapeObj *shape, pointObj *point, double featuresize);
  MS_DLL_EXPORT void insertRenderedLabelMember(mapObj *map, labelCacheMemberObj *cachePtr);
  MS_DLL_EXPORT int msTestLabelCacheCollisions(mapObj *map, labelCacheMemberObj *cachePtr, label_bounds *lb, int current_priority, int current_label);
  MS_DLL_EXPORT int msTestLabelCacheLeaderCollision(mapObj *map, pointObj *lp1, pointObj *lp2);
  MS_DLL_EXPORT labelCacheMemberObj *msGetLabelCacheMember(labelCacheObj *labelcache, int i);

  MS_DLL_EXPORT void msFreeShape(shapeObj *shape); /* in mapprimitive.c */
  int msGetShapeRAMSize(shapeObj* shape); /* in mapprimitive.c */
  MS_DLL_EXPORT void msFreeLabelPathObj(labelPathObj *path);
  MS_DLL_EXPORT shapeObj *msShapeFromWKT(const char *string);
  MS_DLL_EXPORT char *msShapeToWKT(shapeObj *shape);
  MS_DLL_EXPORT void msInitShape(shapeObj *shape);
  MS_DLL_EXPORT void msShapeDeleteLine( shapeObj *shape, int line );
  MS_DLL_EXPORT int msCopyShape(const shapeObj *from, shapeObj *to);
  MS_DLL_EXPORT int msIsOuterRing(shapeObj *shape, int r);
  MS_DLL_EXPORT int *msGetOuterList(shapeObj *shape);
  MS_DLL_EXPORT int *msGetInnerList(shapeObj *shape, int r, int *outerlist);
  MS_DLL_EXPORT void msComputeBounds(shapeObj *shape);
  MS_DLL_EXPORT void msRectToPolygon(rectObj rect, shapeObj *poly);
  MS_DLL_EXPORT void msClipPolylineRect(shapeObj *shape, rectObj rect);
  MS_DLL_EXPORT void msClipPolygonRect(shapeObj *shape, rectObj rect);
  MS_DLL_EXPORT void msTransformShape(shapeObj *shape, rectObj extent, double cellsize, imageObj *image);
  MS_DLL_EXPORT void msTransformPoint(pointObj *point, rectObj *extent, double cellsize, imageObj *image);

  MS_DLL_EXPORT void msOffsetPointRelativeTo(pointObj *point, layerObj *layer);
  MS_DLL_EXPORT void msOffsetShapeRelativeTo(shapeObj *shape, layerObj *layer);
  MS_DLL_EXPORT void msTransformShapeSimplify(shapeObj *shape, rectObj extent, double cellsize);
  MS_DLL_EXPORT void msTransformShapeToPixelSnapToGrid(shapeObj *shape, rectObj extent, double cellsize, double grid_resolution);
  MS_DLL_EXPORT void msTransformShapeToPixelRound(shapeObj *shape, rectObj extent, double cellsize);
  MS_DLL_EXPORT void msTransformShapeToPixelDoublePrecision(shapeObj *shape, rectObj extent, double cellsize);

  MS_DLL_EXPORT shapeObj *msDensify(shapeObj *shape, double tolerance);
  MS_DLL_EXPORT shapeObj *msRings2Shape(shapeObj *shape, int outer);

#ifndef SWIG

  struct line_lengths {
    double *segment_lengths;
    double total_length;
    int longest_segment_index;
  };

  struct polyline_lengths {
    struct line_lengths *ll;
    double total_length;
    int longest_line_index;
    int longest_segment_line_index, longest_segment_point_index;
  };

  struct label_auto_result {
    pointObj *label_points;
    double *angles;
    int num_label_points;
  };

  struct label_follow_result {
    textSymbolObj **follow_labels;
    int num_follow_labels;
    struct label_auto_result lar;
  };

#endif

  MS_DLL_EXPORT void msTransformPixelToShape(shapeObj *shape, rectObj extent, double cellsize);
  MS_DLL_EXPORT void msPolylineComputeLineSegments(shapeObj *shape, struct polyline_lengths *pll);
  MS_DLL_EXPORT int msPolylineLabelPath(mapObj *map, imageObj *image, shapeObj *p, textSymbolObj *ts, labelObj *label, struct label_follow_result *lfr);
  MS_DLL_EXPORT int WARN_UNUSED msPolylineLabelPoint(mapObj *map, shapeObj *p, textSymbolObj *ts, labelObj *label, struct label_auto_result *lar, double resolutionfactor);
  MS_DLL_EXPORT int WARN_UNUSED msLineLabelPath(mapObj *map, imageObj *img, lineObj *p, textSymbolObj *ts, struct line_lengths *ll, struct label_follow_result *lfr, labelObj *lbl);
  MS_DLL_EXPORT int WARN_UNUSED msLineLabelPoint(mapObj *map, lineObj *p, textSymbolObj *ts, struct line_lengths *ll, struct label_auto_result *lar, labelObj *lbl, double resolutionfactor);
  MS_DLL_EXPORT int msPolygonLabelPoint(shapeObj *p, pointObj *lp, double min_dimension);
  MS_DLL_EXPORT int msAddLine(shapeObj *p, const lineObj *new_line);
  MS_DLL_EXPORT int msAddLineDirectly(shapeObj *p, lineObj *new_line);
  MS_DLL_EXPORT int msAddPointToLine(lineObj *line, pointObj *point );
  MS_DLL_EXPORT double msGetPolygonArea(shapeObj *p);
  MS_DLL_EXPORT int msGetPolygonCentroid(shapeObj *p, pointObj *lp, double *miny, double *maxy);

  MS_DLL_EXPORT int msDrawRasterLayer(mapObj *map, layerObj *layer, imageObj *image); /* in mapraster.c */
  MS_DLL_EXPORT imageObj *msDrawReferenceMap(mapObj *map);

  /* mapbits.c - bit array handling functions and macros */

#define MS_ARRAY_BIT 32

#define MS_GET_BIT(array,i) (array[i>>5] & (1 <<(i & 0x3f)))
#define MS_SET_BIT(array,i) {array[i>>5] |= (1 <<(i & 0x3f));}
#define MS_CLR_BIT(array,i) {array[i>>5] &= (~(1 <<(i & 0x3f)));}

  MS_DLL_EXPORT size_t msGetBitArraySize(int numbits); /* in mapbits.c */
  MS_DLL_EXPORT ms_bitarray msAllocBitArray(int numbits);
  MS_DLL_EXPORT int msGetBit(ms_const_bitarray array, int index);
  MS_DLL_EXPORT void msSetBit(ms_bitarray array, int index, int value);
  MS_DLL_EXPORT void msSetAllBits(ms_bitarray array, int index, int value);
  MS_DLL_EXPORT void msFlipBit(ms_bitarray array, int index);
  MS_DLL_EXPORT int msGetNextBit(ms_const_bitarray array, int index, int size);

  /* maplayer.c - layerObj  api */

  MS_DLL_EXPORT int msLayerInitItemInfo(layerObj *layer);
  MS_DLL_EXPORT void msLayerFreeItemInfo(layerObj *layer);

  MS_DLL_EXPORT int msLayerOpen(layerObj *layer); /* in maplayer.c */
  MS_DLL_EXPORT int msLayerApplyScaletokens(layerObj *layer, double scale);
  MS_DLL_EXPORT int msLayerRestoreFromScaletokens(layerObj *layer);
  MS_DLL_EXPORT int msClusterLayerOpen(layerObj *layer); /* in mapcluster.c */
  MS_DLL_EXPORT int msLayerIsOpen(layerObj *layer);
  MS_DLL_EXPORT void msLayerClose(layerObj *layer);
  MS_DLL_EXPORT void msLayerFreeExpressions(layerObj *layer);
  MS_DLL_EXPORT int msLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery);
  MS_DLL_EXPORT int msLayerGetItemIndex(layerObj *layer, char *item);
  MS_DLL_EXPORT int msLayerWhichItems(layerObj *layer, int get_all, const char *metadata);
  MS_DLL_EXPORT int msLayerNextShape(layerObj *layer, shapeObj *shape);
  MS_DLL_EXPORT int msLayerGetItems(layerObj *layer);
  MS_DLL_EXPORT int msLayerSetItems(layerObj *layer, char **items, int numitems);
  MS_DLL_EXPORT int msLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record);
  MS_DLL_EXPORT int msLayerGetShapeCount(layerObj *layer, rectObj rect, projectionObj *rectProjection);
  MS_DLL_EXPORT int msLayerGetExtent(layerObj *layer, rectObj *extent);
  MS_DLL_EXPORT int msLayerSetExtent( layerObj *layer, double minx, double miny, double maxx, double maxy);
  MS_DLL_EXPORT int msLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c, shapeObj* shape);
  MS_DLL_EXPORT int msLayerGetFeatureStyle(mapObj *map, layerObj *layer, classObj *c, shapeObj* shape);
  MS_DLL_EXPORT void msLayerAddProcessing( layerObj *layer, const char *directive );
  MS_DLL_EXPORT void msLayerSetProcessingKey( layerObj *layer, const char *key, const char *value);
  MS_DLL_EXPORT const char *msLayerGetProcessing( const layerObj *layer, int proc_index);
  MS_DLL_EXPORT const char *msLayerGetProcessingKey( const layerObj *layer, const char *);
  MS_DLL_EXPORT int msLayerClearProcessing( layerObj *layer );
  MS_DLL_EXPORT void msLayerSubstituteProcessing( layerObj *layer, const char *from, const char *to );
  MS_DLL_EXPORT char *msLayerGetFilterString( layerObj *layer );
  MS_DLL_EXPORT int msLayerEncodeShapeAttributes( layerObj *layer, shapeObj *shape);

  MS_DLL_EXPORT int msLayerTranslateFilter(layerObj *layer, expressionObj *filter, char *filteritem);
  MS_DLL_EXPORT int msLayerSupportsCommonFilters(layerObj *layer);
  MS_DLL_EXPORT const char *msExpressionTokenToString(int token);
  MS_DLL_EXPORT int msTokenizeExpression(expressionObj *expression, char **list, int *listsize);

  MS_DLL_EXPORT int msLayerSetTimeFilter(layerObj *lp, const char *timestring, const char *timefield);
  MS_DLL_EXPORT int msLayerMakeBackticsTimeFilter(layerObj *lp, const char *timestring, const char *timefield);
  MS_DLL_EXPORT int msLayerMakePlainTimeFilter(layerObj *lp, const char *timestring, const char *timefield);

  MS_DLL_EXPORT int msLayerApplyCondSQLFilterToLayer(FilterEncodingNode *psNode, mapObj *map, int iLayerIndex);
  MS_DLL_EXPORT int msLayerApplyPlainFilterToLayer(FilterEncodingNode *psNode, mapObj *map, int iLayerIndex);

  /* maplayer.c */
  MS_DLL_EXPORT int msLayerGetNumFeatures(layerObj *layer);

  MS_DLL_EXPORT int msLayerSupportsPaging(layerObj *layer);

  MS_DLL_EXPORT void msLayerEnablePaging(layerObj *layer, int value);
  MS_DLL_EXPORT int msLayerGetPaging(layerObj *layer);

  MS_DLL_EXPORT int msLayerGetMaxFeaturesToDraw(layerObj *layer, outputFormatObj *format);

  MS_DLL_EXPORT char *msLayerEscapeSQLParam(layerObj *layer, const char* pszString);
  MS_DLL_EXPORT char *msLayerEscapePropertyName(layerObj *layer, const char* pszString);

  int msLayerSupportsSorting(layerObj *layer);
  void msLayerSetSort(layerObj *layer, const sortByClause* sortBy);
  MS_DLL_EXPORT char* msLayerBuildSQLOrderBy(layerObj *layer);

  /* These are special because SWF is using these */
  int msOGRLayerNextShape(layerObj *layer, shapeObj *shape);
  int msOGRLayerGetItems(layerObj *layer);
  void msOGRLayerFreeItemInfo(layerObj *layer);
  int msOGRLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record);
  int msOGRLayerGetExtent(layerObj *layer, rectObj *extent);

  MS_DLL_EXPORT int msOGRGeometryToShape(OGRGeometryH hGeometry, shapeObj *shape,
                                         OGRwkbGeometryType type);

  MS_DLL_EXPORT int msInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msConnectLayer(layerObj *layer, const int connectiontype,
                                   const char *library_str);

  MS_DLL_EXPORT int msINLINELayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msSHPLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msFlatGeobufLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msTiledSHPLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msOGRLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msPostGISLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msOracleSpatialLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msWFSLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msGraticuleLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msRASTERLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msUVRASTERLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msContourLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msPluginLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT int msUnionLayerInitializeVirtualTable(layerObj *layer);
  MS_DLL_EXPORT void msPluginFreeVirtualTableFactory(void);

  MS_DLL_EXPORT int LayerDefaultGetShapeCount(layerObj *layer, rectObj rect, projectionObj *rectProjection);
  void msUVRASTERLayerUseMapExtentAndProjectionForNextWhichShapes(layerObj* layer, mapObj* map);
  rectObj msUVRASTERGetSearchRect( layerObj* layer, mapObj* map );

  /* ==================================================================== */
  /*      Prototypes for functions in mapdraw.c                           */
  /* ==================================================================== */

  MS_DLL_EXPORT imageObj *msPrepareImage(mapObj *map, int allow_nonsquare);
  MS_DLL_EXPORT imageObj *msDrawMap(mapObj *map, int querymap);
  MS_DLL_EXPORT int msLayerIsVisible(mapObj *map, layerObj *layer);
  MS_DLL_EXPORT int msDrawLayer(mapObj *map, layerObj *layer, imageObj *image);
  MS_DLL_EXPORT int msDrawVectorLayer(mapObj *map, layerObj *layer, imageObj *image);
  MS_DLL_EXPORT int msDrawQueryLayer(mapObj *map, layerObj *layer, imageObj *image);
  MS_DLL_EXPORT int msDrawWMSLayer(mapObj *map, layerObj *layer, imageObj *image);
  MS_DLL_EXPORT int msDrawWFSLayer(mapObj *map, layerObj *layer, imageObj *image);

#define MS_DRAWMODE_FEATURES    0x00001
#define MS_DRAW_FEATURES(mode) (MS_DRAWMODE_FEATURES&(mode))
#define MS_DRAWMODE_LABELS      0x00002
#define MS_DRAW_LABELS(mode) (MS_DRAWMODE_LABELS&(mode))
#define MS_DRAWMODE_SINGLESTYLE 0x00004
#define MS_DRAW_SINGLESTYLE(mode) (MS_DRAWMODE_SINGLESTYLE&(mode))
#define MS_DRAWMODE_QUERY       0x00008
#define MS_DRAW_QUERY(mode) (MS_DRAWMODE_QUERY&(mode))
#define MS_DRAWMODE_UNCLIPPEDLABELS       0x00010
#define MS_DRAW_UNCLIPPED_LABELS(mode) (MS_DRAWMODE_UNCLIPPEDLABELS&(mode))
#define MS_DRAWMODE_UNCLIPPEDLINES       0x00020
#define MS_DRAW_UNCLIPPED_LINES(mode) (MS_DRAWMODE_UNCLIPPEDLINES&(mode))

  MS_DLL_EXPORT int WARN_UNUSED msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, imageObj *image, int style, int mode);
  MS_DLL_EXPORT int WARN_UNUSED msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, imageObj *image, int classindex, char *labeltext);

  /*Range Support*/
  typedef enum {
    MS_COLORSPACE_RGB,
    MS_COLORSPACE_HSL
  } colorspace;
  MS_DLL_EXPORT int msShapeToRange(styleObj *style, shapeObj *shape);
  MS_DLL_EXPORT int msValueToRange(styleObj *style, double fieldVal, colorspace cs);

  MS_DLL_EXPORT int WARN_UNUSED msDrawMarkerSymbol(mapObj *map, imageObj *image, pointObj *p, styleObj *style, double scalefactor);
  MS_DLL_EXPORT int WARN_UNUSED msDrawLineSymbol(mapObj *map, imageObj *image, shapeObj *p, styleObj *style, double scalefactor);
  MS_DLL_EXPORT int WARN_UNUSED msDrawShadeSymbol(mapObj *map, imageObj *image, shapeObj *p, styleObj *style, double scalefactor);
  MS_DLL_EXPORT int WARN_UNUSED msCircleDrawLineSymbol(mapObj *map, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor);
  MS_DLL_EXPORT int WARN_UNUSED msCircleDrawShadeSymbol(mapObj *map, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor);
  MS_DLL_EXPORT int WARN_UNUSED msDrawPieSlice(mapObj *map, imageObj *image, pointObj *p, styleObj *style, double radius, double start, double end);
  MS_DLL_EXPORT int WARN_UNUSED msDrawLabelBounds(mapObj *map, imageObj *image, label_bounds *bnds, styleObj *style, double scalefactor);

  MS_DLL_EXPORT void msOutlineRenderingPrepareStyle(styleObj *pStyle, mapObj *map, layerObj *layer, imageObj *image);
  MS_DLL_EXPORT void msOutlineRenderingRestoreStyle(styleObj *pStyle, mapObj *map, layerObj *layer, imageObj *image);

  MS_DLL_EXPORT int WARN_UNUSED msDrawLabel(mapObj *map, imageObj *image, pointObj labelPnt, char *string, labelObj *label, double scalefactor);
  MS_DLL_EXPORT int WARN_UNUSED msDrawTextSymbol(mapObj *map, imageObj *image, pointObj labelPnt, textSymbolObj *ts);
  MS_DLL_EXPORT int WARN_UNUSED msDrawLabelCache(mapObj *map, imageObj *image);

  MS_DLL_EXPORT void msImageStartLayer(mapObj *map, layerObj *layer, imageObj *image);
  MS_DLL_EXPORT void msImageEndLayer(mapObj *map, layerObj *layer, imageObj *image);

  MS_DLL_EXPORT void msDrawStartShape(mapObj *map, layerObj *layer, imageObj *image, shapeObj *shape);
  MS_DLL_EXPORT void msDrawEndShape(mapObj *map, layerObj *layer, imageObj *image, shapeObj *shape);
  /* ==================================================================== */
  /*      End of Prototypes for functions in mapdraw.c                    */
  /* ==================================================================== */

  /* ==================================================================== */
  /*      Prototypes for functions in mapgeomutil.cpp                       */
  /* ==================================================================== */
  MS_DLL_EXPORT shapeObj *msRasterizeArc(double x0, double y0, double radius, double startAngle, double endAngle, int isSlice);
  MS_DLL_EXPORT int msHatchPolygon(imageObj *img, shapeObj *poly, double spacing, double width, double *pattern, int patternlength, double angle, colorObj *color);



  /* ==================================================================== */
  /*      Prototypes for functions in mapimagemap.c                       */
  /* ==================================================================== */
  MS_DLL_EXPORT imageObj *msImageCreateIM(int width, int height, outputFormatObj *format, char *imagepath, char *imageurl, double resolution, double defresolution);
  MS_DLL_EXPORT void msImageStartLayerIM(mapObj *map, layerObj *layer, imageObj *image);
  MS_DLL_EXPORT int msSaveImageIM(imageObj* img, const char *filename, outputFormatObj *format);
  MS_DLL_EXPORT void msFreeImageIM(imageObj* img);
  MS_DLL_EXPORT void msDrawMarkerSymbolIM(mapObj *map, imageObj* img, pointObj *p, styleObj *style, double scalefactor);
  MS_DLL_EXPORT void msDrawLineSymbolIM(mapObj *map, imageObj* img, shapeObj *p, styleObj *style, double scalefactor);
  MS_DLL_EXPORT void msDrawShadeSymbolIM(mapObj *map, imageObj* img, shapeObj *p, styleObj *style, double scalefactor);
  MS_DLL_EXPORT int msDrawTextIM(mapObj *map, imageObj* img, pointObj labelPnt, char *string, labelObj *label, double scalefactor);
  /* ==================================================================== */
  /*      End of Prototypes for functions in mapimagemap.c                */
  /* ==================================================================== */


  /* various JOIN functions (in mapjoin.c) */
  MS_DLL_EXPORT int msJoinConnect(layerObj *layer, joinObj *join);
  MS_DLL_EXPORT int msJoinPrepare(joinObj *join, shapeObj *shape);
  MS_DLL_EXPORT int msJoinNext(joinObj *join);
  MS_DLL_EXPORT int msJoinClose(joinObj *join);

  /*in mapraster.c */
  int msDrawRasterLayerLowCheckIfMustDraw(mapObj *map, layerObj *layer);
  void* msDrawRasterLayerLowOpenDataset(mapObj *map, layerObj *layer,
                                      const char* filename,
                                      char szPath[MS_MAXPATHLEN],
                                      char** p_decrypted_path);
  void msDrawRasterLayerLowCloseDataset(layerObj *layer, void* hDataset);
  int msDrawRasterLayerLowWithDataset(mapObj *map, layerObj *layer, imageObj *image, rasterBufferObj *rb, void* hDatasetIn );

  MS_DLL_EXPORT int msDrawRasterLayerLow(mapObj *map, layerObj *layer, imageObj *image, rasterBufferObj *rb );
  MS_DLL_EXPORT int msGetClass(layerObj *layer, colorObj *color, int colormap_index);
  MS_DLL_EXPORT int msGetClass_FloatRGB(layerObj *layer, float fValue,
                                        int red, int green, int blue );
  int msGetClass_FloatRGB_WithFirstClassToTry(layerObj *layer, float fValue, int red, int green, int blue, int firstClassToTry );

  /* in mapdrawgdal.c */
  MS_DLL_EXPORT int msDrawRasterLayerGDAL(mapObj *map, layerObj *layer, imageObj *image, rasterBufferObj *rb, void *hDSVoid );
  MS_DLL_EXPORT int msGetGDALGeoTransform(void *hDS, mapObj *map, layerObj *layer, double *padfGeoTransform );
  MS_DLL_EXPORT int *msGetGDALBandList( layerObj *layer, void *hDS, int max_bands, int *band_count );
  MS_DLL_EXPORT double msGetGDALNoDataValue( layerObj *layer, void *hBand, int *pbGotNoData );

  /* in interpolation.c */
  MS_DLL_EXPORT int msInterpolationDataset(mapObj *map, imageObj *image, layerObj *layer, void **hDSvoid, void **cleanup_ptr);
  MS_DLL_EXPORT int msCleanupInterpolationDataset(mapObj *map, imageObj *image, layerObj *layer, void *cleanup_ptr);

  /* in mapchart.c */
  MS_DLL_EXPORT int msDrawChartLayer(mapObj *map, layerObj *layer, imageObj *image);

  /* ==================================================================== */
  /*      End of prototypes for functions in mapgd.c                      */
  /* ==================================================================== */

  /* ==================================================================== */
  /*      Prototypes for functions in maputil.c                           */
  /* ==================================================================== */

  MS_DLL_EXPORT int msScaleInBounds(double scale, double minscale, double maxscale);
  MS_DLL_EXPORT void *msSmallMalloc( size_t nSize );
  MS_DLL_EXPORT void * msSmallRealloc( void * pData, size_t nNewSize );
  MS_DLL_EXPORT void *msSmallCalloc( size_t nCount, size_t nSize );
  MS_DLL_EXPORT int msIntegerInArray(const int value, int *array, int numelements);

  MS_DLL_EXPORT int msExtentsOverlap(mapObj *map, layerObj *layer);
  MS_DLL_EXPORT char *msBuildOnlineResource(mapObj *map, cgiRequestObj *req);

  /* For mapswf */
  MS_DLL_EXPORT int getRgbColor(mapObj *map,int i,int *r,int *g,int *b); /* maputil.c */

  MS_DLL_EXPORT int msBindLayerToShape(layerObj *layer, shapeObj *shape, int querymapMode);
  MS_DLL_EXPORT int msValidateContexts(mapObj *map);
  MS_DLL_EXPORT int msEvalContext(mapObj *map, layerObj *layer, char *context);
  MS_DLL_EXPORT int msEvalExpression(layerObj *layer, shapeObj *shape, expressionObj *expression, int itemindex);
  MS_DLL_EXPORT int msShapeGetClass(layerObj *layer, mapObj *map, shapeObj *shape, int *classgroup, int numclasses);
  MS_DLL_EXPORT int msShapeGetNextClass(int currentclass, layerObj *layer, mapObj *map, shapeObj *shape, int *classgroup, int numclasses);
  MS_DLL_EXPORT int msShapeCheckSize(shapeObj *shape, double minfeaturesize);
  MS_DLL_EXPORT char* msShapeGetLabelAnnotation(layerObj *layer, shapeObj *shape, labelObj *lbl);
  MS_DLL_EXPORT int msGetLabelStatus(mapObj *map, layerObj *layer, shapeObj *shape, labelObj *lbl);
  MS_DLL_EXPORT int msAdjustImage(rectObj rect, int *width, int *height);
  MS_DLL_EXPORT char *msEvalTextExpression(expressionObj *expr, shapeObj *shape);
  MS_DLL_EXPORT char *msEvalTextExpressionJSonEscape(expressionObj *expr, shapeObj *shape);
  MS_DLL_EXPORT double msEvalDoubleExpression(expressionObj *expr, shapeObj *shape);
  MS_DLL_EXPORT double msAdjustExtent(rectObj *rect, int width, int height);
  MS_DLL_EXPORT int msConstrainExtent(rectObj *bounds, rectObj *rect, double overlay);
  MS_DLL_EXPORT int *msGetLayersIndexByGroup(mapObj *map, char *groupname, int *nCount);
  MS_DLL_EXPORT unsigned char *msSaveImageBuffer(imageObj* image, int *size_ptr, outputFormatObj *format);
  MS_DLL_EXPORT shapeObj* msOffsetPolyline(shapeObj* shape, double offsetx, double offsety);
  MS_DLL_EXPORT int msMapSetLayerProjections(mapObj* map);

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
  MS_DLL_EXPORT int msAddLabelToClass(classObj *classo, labelObj *label);
  MS_DLL_EXPORT labelObj *msRemoveLabelFromClass(classObj *classo, int nLabelIndex);
  MS_DLL_EXPORT int msMoveStyleUp(classObj *classo, int nStyleIndex);
  MS_DLL_EXPORT int msMoveStyleDown(classObj *classo, int nStyleIndex);
  MS_DLL_EXPORT int msDeleteStyle(classObj *classo, int nStyleIndex);
  MS_DLL_EXPORT int msInsertStyle(classObj *classo, styleObj *style, int nStyleIndex);
  MS_DLL_EXPORT styleObj *msRemoveStyle(classObj *classo, int index);

  /* maplabel.c */
  MS_DLL_EXPORT int msInsertLabelStyle(labelObj *label, styleObj *style,
                                       int nStyleIndex);
  MS_DLL_EXPORT int msMoveLabelStyleUp(labelObj *label, int nStyleIndex);
  MS_DLL_EXPORT int msMoveLabelStyleDown(labelObj *label, int nStyleIndex);
  MS_DLL_EXPORT int msDeleteLabelStyle(labelObj *label, int nStyleIndex);
  MS_DLL_EXPORT styleObj *msRemoveLabelStyle(labelObj *label, int nStyleIndex);

  /* Measured shape utility functions. */
  MS_DLL_EXPORT pointObj *msGetPointUsingMeasure(shapeObj *shape, double m);
  MS_DLL_EXPORT pointObj *msGetMeasureUsingPoint(shapeObj *shape, pointObj *point);

  MS_DLL_EXPORT char **msGetAllGroupNames(mapObj* map, int *numTok);
  MS_DLL_EXPORT char *msTmpFile(mapObj *map, const char *mappath, const char *tmppath, const char *ext);
  MS_DLL_EXPORT char *msTmpPath(mapObj *map, const char *mappath, const char *tmppath);
  MS_DLL_EXPORT char *msTmpFilename(const char *ext);
  MS_DLL_EXPORT void msForceTmpFileBase( const char *new_base );


  MS_DLL_EXPORT imageObj *msImageCreate(int width, int height, outputFormatObj *format, char *imagepath, char *imageurl, double resolution, double defresolution, colorObj *bg);

  MS_DLL_EXPORT void msAlphaBlend(
    unsigned char red_src, unsigned char green_src,
    unsigned char blue_src, unsigned char alpha_src,
    unsigned char *red_dst, unsigned char *green_dst,
    unsigned char *blue_dst, unsigned char *alpha_dst );
  MS_DLL_EXPORT void msAlphaBlendPM(
    unsigned char red_src, unsigned char green_src,
    unsigned char blue_src, unsigned char alpha_src,
    unsigned char *red_dst, unsigned char *green_dst,
    unsigned char *blue_dst, unsigned char *alpha_dst );

  MS_DLL_EXPORT void msRGBtoHSL(colorObj *rgb, double *h, double *s, double *l);

  MS_DLL_EXPORT void msHSLtoRGB(double h, double s, double l, colorObj *rgb);

  MS_DLL_EXPORT int msCheckParentPointer(void* p, const char* objname);

  MS_DLL_EXPORT int *msAllocateValidClassGroups(layerObj *lp, int *nclasses);

  MS_DLL_EXPORT void msFreeRasterBuffer(rasterBufferObj *b);
  MS_DLL_EXPORT void msSetLayerOpacity(layerObj *layer, int opacity);

  void msMapSetLanguageSpecificConnection(mapObj* map, const char* validated_language);
  MS_DLL_EXPORT shapeObj* msGeneralize(shapeObj * shape, double tolerance);
  /* ==================================================================== */
  /*      End of prototypes for functions in maputil.c                    */
  /* ==================================================================== */


  /* ==================================================================== */
  /*      prototypes for functions in mapoutput.c                         */
  /* ==================================================================== */

  MS_DLL_EXPORT void msApplyDefaultOutputFormats( mapObj * );
  MS_DLL_EXPORT void msFreeOutputFormat( outputFormatObj * );
  MS_DLL_EXPORT int msGetOutputFormatIndex(mapObj *map, const char *imagetype);
  MS_DLL_EXPORT int msRemoveOutputFormat(mapObj *map, const char *imagetype);
  MS_DLL_EXPORT int msAppendOutputFormat(mapObj *map, outputFormatObj *format);
  MS_DLL_EXPORT outputFormatObj *msSelectOutputFormat( mapObj *map, const char *imagetype );
  MS_DLL_EXPORT void msApplyOutputFormat( outputFormatObj **target, outputFormatObj *format, int transparent);
  MS_DLL_EXPORT const char *msGetOutputFormatOption( outputFormatObj *format, const char *optionkey, const char *defaultresult );
  MS_DLL_EXPORT outputFormatObj *msCreateDefaultOutputFormat( mapObj *map, const char *driver, const char *name, const char *mimetype );
  MS_DLL_EXPORT int msPostMapParseOutputFormatSetup( mapObj *map );
  MS_DLL_EXPORT void msSetOutputFormatOption( outputFormatObj *format, const char *key, const char *value );
  MS_DLL_EXPORT void msGetOutputFormatMimeList( mapObj *map, char **mime_list, int max_mime );
  MS_DLL_EXPORT void msGetOutputFormatMimeListImg( mapObj *map, const char **mime_list, int max_mime );
  MS_DLL_EXPORT void msGetOutputFormatMimeListWMS( mapObj *map, const char **mime_list, int max_mime );
  MS_DLL_EXPORT outputFormatObj *msCloneOutputFormat( outputFormatObj *format );
  MS_DLL_EXPORT int msOutputFormatValidate( outputFormatObj *format,
      int issue_error );
  void  msOutputFormatResolveFromImage( mapObj *map, imageObj* img );

  /* ==================================================================== */
  /*      End of prototypes for functions in mapoutput.c                  */
  /* ==================================================================== */

  /* ==================================================================== */
  /*      prototypes for functions in mapgdal.c                           */
  /* ==================================================================== */
  MS_DLL_EXPORT int msSaveImageGDAL( mapObj *map, imageObj *image, const char *filename );
  MS_DLL_EXPORT int msInitDefaultGDALOutputFormat( outputFormatObj *format );
  void msCleanVSIDir( const char *pszDir );
  char** msGetStringListFromHashTable(hashTableObj* table);

  /* ==================================================================== */
  /*      prototypes for functions in mapogroutput.c                      */
  /* ==================================================================== */
  MS_DLL_EXPORT int msInitDefaultOGROutputFormat( outputFormatObj *format );
  MS_DLL_EXPORT int msOGRWriteFromQuery( mapObj *map, outputFormatObj *format,
                                         int sendheaders );

  /* ==================================================================== */
  /*      Public prototype for mapogr.cpp functions.                      */
  /* ==================================================================== */
  int MS_DLL_EXPORT msOGRLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery);
  int MS_DLL_EXPORT msOGRLayerOpen(layerObj *layer, const char *pszOverrideConnection); /* in mapogr.cpp */
  int MS_DLL_EXPORT msOGRLayerClose(layerObj *layer);

  char MS_DLL_EXPORT *msOGRShapeToWKT(shapeObj *shape);
  shapeObj MS_DLL_EXPORT *msOGRShapeFromWKT(const char *string);
  int msOGRUpdateStyleFromString(mapObj *map, layerObj *layer, classObj *c,
                                 const char *stylestring);

  /* ==================================================================== */
  /*      prototypes for functions in mapcopy                             */
  /* ==================================================================== */
  MS_DLL_EXPORT int msCopyMap(mapObj *dst, const mapObj *src);
  MS_DLL_EXPORT int msCopyLayer(layerObj *dst, const layerObj *src);
  MS_DLL_EXPORT int msCopyScaleToken(const scaleTokenObj *src, scaleTokenObj *dst);
  MS_DLL_EXPORT int msCopyPoint(pointObj *dst, const pointObj *src);
  MS_DLL_EXPORT int msCopyFontSet(fontSetObj *dst, const fontSetObj *src, mapObj *map);
  MS_DLL_EXPORT void copyProperty(void *dst, const void *src, int size);
  MS_DLL_EXPORT char *copyStringProperty(char **dst, const char *src);
  MS_DLL_EXPORT int msCopyClass(classObj *dst, const classObj *src, layerObj *layer);
  MS_DLL_EXPORT int msCopyStyle(styleObj *dst, const styleObj *src);
  MS_DLL_EXPORT int msCopyLabel(labelObj *dst, const labelObj *src);
  MS_DLL_EXPORT int msCopyLabelLeader(labelLeaderObj *dst, const labelLeaderObj *src);
  MS_DLL_EXPORT int msCopyLine(lineObj *dst, const lineObj *src);
  MS_DLL_EXPORT int msCopyProjection(projectionObj *dst, const projectionObj *src);
  MS_DLL_EXPORT int msCopyProjectionExtended(projectionObj *dst, const projectionObj *src, char ** args, int num_args);
  int msCopyExpression(expressionObj *dst, const expressionObj *src);
  int msCopyProjection(projectionObj *dst, const projectionObj *src);
  MS_DLL_EXPORT int msCopyRasterBuffer(rasterBufferObj *dst, const rasterBufferObj *src);

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
  /*      prototypes for functions in mapgeos.c                         */
  /* ==================================================================== */
  MS_DLL_EXPORT void msGEOSSetup(void);
  MS_DLL_EXPORT void msGEOSCleanup(void);
  MS_DLL_EXPORT void msGEOSFreeGeometry(shapeObj *shape);

  MS_DLL_EXPORT shapeObj *msGEOSShapeFromWKT(const char *string);
  MS_DLL_EXPORT char *msGEOSShapeToWKT(shapeObj *shape);
  MS_DLL_EXPORT void msGEOSFreeWKT(char* pszGEOSWKT);

  MS_DLL_EXPORT shapeObj *msGEOSBuffer(shapeObj *shape, double width);
  MS_DLL_EXPORT shapeObj *msGEOSSimplify(shapeObj *shape, double tolerance);
  MS_DLL_EXPORT shapeObj *msGEOSTopologyPreservingSimplify(shapeObj *shape, double tolerance);
  MS_DLL_EXPORT shapeObj *msGEOSConvexHull(shapeObj *shape);
  MS_DLL_EXPORT shapeObj *msGEOSBoundary(shapeObj *shape);
  MS_DLL_EXPORT pointObj *msGEOSGetCentroid(shapeObj *shape);
  MS_DLL_EXPORT shapeObj *msGEOSUnion(shapeObj *shape1, shapeObj *shape2);
  MS_DLL_EXPORT shapeObj *msGEOSIntersection(shapeObj *shape1, shapeObj *shape2);
  MS_DLL_EXPORT shapeObj *msGEOSDifference(shapeObj *shape1, shapeObj *shape2);
  MS_DLL_EXPORT shapeObj *msGEOSSymDifference(shapeObj *shape1, shapeObj *shape2);
  MS_DLL_EXPORT shapeObj *msGEOSOffsetCurve(shapeObj *p, double offset);
  MS_DLL_EXPORT shapeObj *msGEOSVoronoiDiagram(shapeObj *shape, double tolerance, int onlyEdges);
  MS_DLL_EXPORT shapeObj *msGEOSCenterline(shapeObj *shape);

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

  /* ==================================================================== */
  /*      prototypes for functions in mapxmp.c                            */
  /* ==================================================================== */
  MS_DLL_EXPORT int msXmpPresent(mapObj *map);
  MS_DLL_EXPORT int msXmpWrite(mapObj *map, const char *filename);

  /* ==================================================================== */
  /*      prototypes for functions in mapgeomtransform.c                  */
  /* ==================================================================== */
  enum MS_GEOMTRANSFORM_TYPE {
    MS_GEOMTRANSFORM_NONE,
    MS_GEOMTRANSFORM_EXPRESSION,
    MS_GEOMTRANSFORM_START,
    MS_GEOMTRANSFORM_END,
    MS_GEOMTRANSFORM_VERTICES,
    MS_GEOMTRANSFORM_BBOX,
    MS_GEOMTRANSFORM_CENTROID,
    MS_GEOMTRANSFORM_BUFFER,
    MS_GEOMTRANSFORM_CONVEXHULL,
    MS_GEOMTRANSFORM_LABELPOINT,
    MS_GEOMTRANSFORM_LABELPOLY,
    MS_GEOMTRANSFORM_LABELCENTER
  };

  MS_DLL_EXPORT int msDrawTransformedShape(mapObj *map, imageObj *image, shapeObj *shape, styleObj *style, double scalefactor);
  MS_DLL_EXPORT void msStyleSetGeomTransform(styleObj *s, char *transform);
  MS_DLL_EXPORT char *msStyleGetGeomTransform(styleObj *style);

  MS_DLL_EXPORT int msGeomTransformShape(mapObj *map, layerObj *layer, shapeObj *shape);

  /* ==================================================================== */
  /*      end of prototypes for functions in mapgeomtransform.c                 */
  /* ==================================================================== */


  /* ==================================================================== */
  /*      prototypes for functions in mapgraticule.c                      */
  /* ==================================================================== */
  MS_DLL_EXPORT graticuleIntersectionObj *msGraticuleLayerGetIntersectionPoints(mapObj *map, layerObj *layer);
  MS_DLL_EXPORT void msGraticuleLayerFreeIntersectionPoints( graticuleIntersectionObj *psValue);

  /* ==================================================================== */
  /*      end of prototypes for functions in mapgraticule.c               */
  /* ==================================================================== */

  /* ==================================================================== */
  /*      prototypes for functions in mapsmoothing.c                      */
  /* ==================================================================== */
  MS_DLL_EXPORT shapeObj* msSmoothShapeSIA(shapeObj *shape, int ss, int si, char *preprocessing);

  /* ==================================================================== */
  /*      end of prototypes for functions in mapsmoothing.c               */
  /* ==================================================================== */

  /* ==================================================================== */
  /*      prototypes for functions in mapv8.cpp                           */
  /* ==================================================================== */
#ifdef USE_V8_MAPSCRIPT
  MS_DLL_EXPORT void msV8CreateContext(mapObj *map);
  MS_DLL_EXPORT void msV8ContextSetLayer(mapObj *map, layerObj *layer);
  MS_DLL_EXPORT void msV8FreeContext(mapObj *map);
  MS_DLL_EXPORT char* msV8GetFeatureStyle(mapObj *map, const char *filename,
                                          layerObj *layer, shapeObj *shape);
  MS_DLL_EXPORT shapeObj *msV8TransformShape(shapeObj *shape,
                                             const char* filename);
#endif
  /* ==================================================================== */
  /*      end of prototypes for functions in mapv8.cpp                    */
  /* ==================================================================== */

#endif


#ifndef SWIG
  /*
   * strokeStyleObj
   */
  typedef struct {
    double width; /* line width in pixels */

    /* line pattern, e.g. dots, dashes, etc.. */
    int patternlength;
    double pattern[MS_MAXPATTERNLENGTH];
    double patternoffset;

    /* must be a valid color if not NULL */
    /* color.alpha must be used if supported by the renderer */
    colorObj *color;

    int linecap; /* MS_CJC_TRIANGLE, MS_CJC_SQUARE, MS_CJC_ROUND, MS_CJC_BUTT */
    int linejoin; /* MS_CJC_BEVEL MS_CJC_ROUND MS_CJC_MITER */
    double linejoinmaxsize;
    int antialiased;
  } strokeStyleObj;

#define INIT_STROKE_STYLE(s) { (s).width=0; (s).patternlength=0; (s).color=NULL; (s).linecap=MS_CJC_ROUND; (s).linejoin=MS_CJC_ROUND; (s).linejoinmaxsize=0; (s).antialiased=MS_TRUE; }


  /*
   * symbolStyleObj
   */
  typedef struct {
    /* must be valid colors if not NULL */
    /* color.alpha must be used if supported by the renderer */
    colorObj *color;
    colorObj *backgroundcolor;

    double outlinewidth;
    colorObj *outlinecolor;

    /* scalefactor to be applied on the tile or symbol*/
    double scale;

    /* rotation to apply on the symbol (and the tile?)
     * in radians */
    double rotation;

    /* the gap to space symbols appart when used as a polygon tile
     */
    double gap;

    /* style object, necessary for vector type renderers to be able
     * to render symbols through other renders such as cairo/agg */
    styleObj *style;
  } symbolStyleObj;

#define INIT_SYMBOL_STYLE(s) {(s).color=NULL; (s).backgroundcolor=NULL; (s).outlinewidth=0; (s).outlinecolor=NULL; (s).scale=1.0; (s).rotation=0; (s).style=NULL;}

  struct tileCacheObj {
    symbolObj *symbol;
    int width;
    int height;
    colorObj color, outlinecolor, backgroundcolor;
    double outlinewidth, rotation,scale;
    imageObj *image;
    tileCacheObj *next;
  };


  /*
   * labelStyleObj
   */
  typedef struct {
    /* full paths to truetype font file */
    char* fonts[MS_MAX_LABEL_FONTS];
    int numfonts;
    double size;
    double rotation;
    colorObj *color;
    double outlinewidth;
    colorObj *outlinecolor;
  } labelStyleObj;

#define INIT_LABEL_STYLE(s) {memset(&(s),'\0',sizeof(labelStyleObj));}

#endif

#ifndef SWIG
  MS_DLL_EXPORT int msInitializeDummyRenderer(rendererVTableObj *vtable);
  MS_DLL_EXPORT int msInitializeRendererVTable(outputFormatObj *outputformat);
  MS_DLL_EXPORT int msPopulateRendererVTableCairoRaster( rendererVTableObj *renderer );
  MS_DLL_EXPORT int msPopulateRendererVTableCairoSVG( rendererVTableObj *renderer );
  MS_DLL_EXPORT int msPopulateRendererVTableCairoPDF( rendererVTableObj *renderer );
  MS_DLL_EXPORT int msPopulateRendererVTableOGL( rendererVTableObj *renderer );
  MS_DLL_EXPORT int msPopulateRendererVTableAGG( rendererVTableObj *renderer );
  MS_DLL_EXPORT int msPopulateRendererVTableUTFGrid( rendererVTableObj *renderer );
  MS_DLL_EXPORT int msPopulateRendererVTableKML( rendererVTableObj *renderer );
  MS_DLL_EXPORT int msPopulateRendererVTableOGR( rendererVTableObj *renderer );
  MS_DLL_EXPORT int msPopulateRendererVTableMVT( rendererVTableObj *renderer );

  MS_DLL_EXPORT int msMVTWriteTile( mapObj *map, int sendheaders );

#ifdef USE_CAIRO
  MS_DLL_EXPORT void msCairoCleanup(void);
#endif

  /* allocate 50k for starters */
#define MS_DEFAULT_BUFFER_ALLOC 50000

  typedef struct _autobuffer {
    unsigned char *data;
    size_t size;
    size_t available;
    size_t _next_allocation_size;
  } bufferObj;


  /* in mapimageio.c */
  int msQuantizeRasterBuffer(rasterBufferObj *rb, unsigned int *reqcolors, rgbaPixel *palette,
                             rgbaPixel *forced_palette, int num_forced_palette_entries,
                             unsigned int *palette_scaling_maxval);
  int msClassifyRasterBuffer(rasterBufferObj *rb, rasterBufferObj *qrb);
  int msSaveRasterBuffer(mapObj *map, rasterBufferObj *data, FILE *stream, outputFormatObj *format);
  int msSaveRasterBufferToBuffer(rasterBufferObj *data, bufferObj *buffer, outputFormatObj *format);
  int msLoadMSRasterBufferFromFile(char *path, rasterBufferObj *rb);

  /* in mapagg.cpp */
  void msApplyBlurringCompositingFilter(rasterBufferObj *rb, unsigned int radius);

  int WARN_UNUSED msApplyCompositingFilter(mapObj *map, rasterBufferObj *rb, CompositingFilter *filter);

  void msBufferInit(bufferObj *buffer);
  void msBufferResize(bufferObj *buffer, size_t target_size);
  MS_DLL_EXPORT void msBufferFree(bufferObj *buffer);
  MS_DLL_EXPORT void msBufferAppend(bufferObj *buffer, void *data, size_t length);

  typedef struct {
    int charWidth, charHeight;
  } fontMetrics;

  struct rendererVTableObj {
    int supports_pixel_buffer;
    int supports_clipping;
    int supports_svg;
    int use_imagecache;
    enum MS_TRANSFORM_MODE default_transform_mode;
    enum MS_TRANSFORM_MODE transform_mode;
    double default_approximation_scale;
    double approximation_scale;

    void *renderer_data;

    int WARN_UNUSED (*renderLine)(imageObj *img, shapeObj *p, strokeStyleObj *style);
    int WARN_UNUSED (*renderPolygon)(imageObj *img, shapeObj *p, colorObj *color);
    int WARN_UNUSED (*renderPolygonTiled)(imageObj *img, shapeObj *p, imageObj *tile);
    int WARN_UNUSED (*renderLineTiled)(imageObj *img, shapeObj *p, imageObj *tile);

    /* only ts->textpath is guaranteed to be populated. Other fields might be 0/NULL */
    int WARN_UNUSED (*renderGlyphs)(imageObj *img, const textSymbolObj *ts, colorObj *clr, colorObj *olcolor, int olwidth, int isMarker);

    int WARN_UNUSED (*renderVectorSymbol)(imageObj *img, double x, double y,
                              symbolObj *symbol, symbolStyleObj *style);

    int WARN_UNUSED (*renderPixmapSymbol)(imageObj *img, double x, double y,
                              symbolObj *symbol, symbolStyleObj *style);

    int WARN_UNUSED (*renderEllipseSymbol)(imageObj *image, double x, double y,
                               symbolObj *symbol, symbolStyleObj *style);

    int WARN_UNUSED (*renderSVGSymbol)(imageObj *img, double x, double y,
                           symbolObj *symbol, symbolStyleObj *style);

    int WARN_UNUSED (*renderTile)(imageObj *img, imageObj *tile, double x, double y);

    int WARN_UNUSED (*loadImageFromFile)(char *path, rasterBufferObj *rb);

    int WARN_UNUSED (*getRasterBufferHandle)(imageObj *img, rasterBufferObj *rb);
    int WARN_UNUSED (*getRasterBufferCopy)(imageObj *img, rasterBufferObj *rb);
    int WARN_UNUSED (*initializeRasterBuffer)(rasterBufferObj *rb, int width, int height, int mode);

    int WARN_UNUSED (*mergeRasterBuffer)(imageObj *dest, rasterBufferObj *overlay, double opacity, int srcX, int srcY, int dstX, int dstY, int width, int height);
    int WARN_UNUSED (*compositeRasterBuffer)(imageObj *dest, rasterBufferObj *overlay, CompositingOperation comp_op, int opacity);


    /* image i/o */
    imageObj* WARN_UNUSED (*createImage)(int width, int height, outputFormatObj *format, colorObj* bg);
    int WARN_UNUSED (*saveImage)(imageObj *img, mapObj *map, FILE *fp, outputFormatObj *format);
    unsigned char* WARN_UNUSED (*saveImageBuffer)(imageObj *img, int *size_ptr, outputFormatObj *format);
    /*...*/

    int (*startLayer)(imageObj *img, mapObj *map, layerObj *layer);
    int (*endLayer)(imageObj *img, mapObj *map, layerObj *layer);

    int (*startShape)(imageObj *img, shapeObj *shape);
    int (*endShape)(imageObj *img, shapeObj *shape);
    int (*setClip)(imageObj *img, rectObj clipRect);
    int (*resetClip)(imageObj *img);

    int (*freeImage)(imageObj *image);
    int (*freeSymbol)(symbolObj *symbol);
    int (*cleanup)(void *renderer_data);
  } ;
  MS_DLL_EXPORT int msRenderRasterizedSVGSymbol(imageObj* img, double x, double y, symbolObj* symbol, symbolStyleObj* style);

#define MS_IMAGE_RENDERER(im) ((im)->format->vtable)
#define MS_RENDERER_CACHE(renderer) ((renderer)->renderer_data)
#define MS_IMAGE_RENDERER_CACHE(im) MS_RENDERER_CACHE(MS_IMAGE_RENDERER((im)))
#define MS_MAP_RENDERER(map) ((map)->outputformat->vtable)

shapeObj *msOffsetCurve(shapeObj *p, double offset);
#if defined USE_GEOS && (GEOS_VERSION_MAJOR > 3 || (GEOS_VERSION_MAJOR == 3 && GEOS_VERSION_MINOR >= 3))
shapeObj *msGEOSOffsetCurve(shapeObj *p, double offset);
#endif

int msOGRSupportsIsNull(layerObj* layer);

#ifdef NEED_IGNORE_RET_VAL
static inline void IGNORE_RET_VAL(int x) { (void)x; }
#endif

#endif /* SWIG */

#ifdef __cplusplus
}
#endif

#endif /* MAP_H */

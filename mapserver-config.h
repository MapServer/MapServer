#ifndef _MAPSERVER_CONFIG_H
#define _MAPSERVER_CONFIG_H

#define USE_PROJ 1
#define USE_POSTGIS 1
#define USE_GDAL 1
#define USE_OGR 1
#define USE_WMS_SVR 1
#define USE_WCS_SVR 1
#define USE_WFS_SVR 1
/* #undef USE_SOS_SVR */
/* #undef USE_WFS_LYR */
#define USE_WMS_LYR 1
#define USE_CURL 1
#define USE_CAIRO 1
#define USE_GEOS 1
#define USE_GIF 1
#define USE_JPEG 1
#define USE_PNG 1
#define USE_ICONV 1
/* #undef USE_FRIBIDI */
/* #undef USE_HARFBUZZ */
#define USE_LIBXML2 1
#define USE_FASTCGI 1
/* #undef USE_MYSQL */
/* #undef USE_THREAD */
/* #undef USE_KML */
/* #undef USE_POINT_Z_M */
/* #undef USE_ORACLESPATIAL */
/* #undef USE_EXEMPI */
/* #undef USE_XMLMAPFILE */
/* #undef USE_GENERIC_MS_NINT */
#define POSTGIS_HAS_SERVER_VERSION 1
/* #undef USE_SVG_CAIRO */
/* #undef USE_RSVG */
/* #undef USE_SDE */
/* #undef SDE64 */
#define USE_EXTENDED_DEBUG 1
#define USE_V8_MAPSCRIPT 1

/*windows specific hacks*/
#if defined(_WIN32)
/* #undef REGEX_MALLOC */
/* #undef USE_GENERIC_MS_NINT */
#endif


/* #undef HAVE_STRRSTR */
#define HAVE_STRCASECMP 1
#define HAVE_STRCASESTR 1
#define HAVE_STRDUP 1
/* #undef HAVE_STRLCAT */
/* #undef HAVE_STRLCPY */
#define HAVE_STRLEN 1
#define HAVE_STRNCASECMP 1
#define HAVE_VSNPRINTF 1
#define HAVE_DLFCN_H 1

#define HAVE_LRINTF 1
#define HAVE_LRINT 1
#define HAVE_SYNC_FETCH_AND_ADD 1
     

#endif

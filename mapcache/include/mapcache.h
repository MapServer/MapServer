/*
 *  Copyright 2010 Thomas Bonfort
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*! \file mapcache.h
    \brief global function and structure declarations
 */


#ifndef MAPCACHE_H_
#define MAPCACHE_H_

#include <apr_tables.h>
#include <apr_hash.h>
#include <apr_global_mutex.h>
#include "util.h"
#include "ezxml.h"

#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
#include "cJSON.h"
#endif

#include "errors.h"

#if 0
#ifdef USE_GDAL
#include <gdal.h>
#include <cpl_conv.h>
#endif
#endif

#include <assert.h>
#include <apr_time.h>

#ifdef USE_PCRE
#include <pcre.h>
#else
#include <regex.h>
#endif

#ifdef USE_MEMCACHE
#include <apr_memcache.h>
#endif

#define MAPCACHE_SUCCESS 0
#define MAPCACHE_FAILURE 1
#define MAPCACHE_TRUE 1
#define MAPCACHE_FALSE 0
#define MAPCACHE_TILESET_WRONG_SIZE 2
#define MAPCACHE_TILESET_WRONG_RESOLUTION 3
#define MAPCACHE_TILESET_WRONG_EXTENT 4
#define MAPCACHE_CACHE_MISS 5
#define MAPCACHE_FILE_LOCKED 6

#define MAPCACHE_VERSION "0.5-dev"
#define MAPCACHE_USERAGENT "mod-mapcache/"MAPCACHE_VERSION

#define MAPCACHE_LOCKFILE_PREFIX "_gc_lock"



typedef struct mapcache_image_format mapcache_image_format;
typedef struct mapcache_image_format_mixed mapcache_image_format_mixed;
typedef struct mapcache_image_format_png mapcache_image_format_png;
typedef struct mapcache_image_format_png_q mapcache_image_format_png_q;
typedef struct mapcache_image_format_jpeg mapcache_image_format_jpeg;
typedef struct mapcache_cfg mapcache_cfg;
typedef struct mapcache_tileset mapcache_tileset;
typedef struct mapcache_cache mapcache_cache;
typedef struct mapcache_source mapcache_source;
typedef struct mapcache_buffer mapcache_buffer;
typedef struct mapcache_tile mapcache_tile;
typedef struct mapcache_metatile mapcache_metatile;
typedef struct mapcache_feature_info mapcache_feature_info;
typedef struct mapcache_request_get_feature_info mapcache_request_get_feature_info;
typedef struct mapcache_map mapcache_map;
typedef struct mapcache_http_response mapcache_http_response;
typedef struct mapcache_source_wms mapcache_source_wms;
#if 0
typedef struct mapcache_source_gdal mapcache_source_gdal;
#endif
typedef struct mapcache_cache_disk mapcache_cache_disk;
typedef struct mapcache_http mapcache_http;
typedef struct mapcache_request mapcache_request;
typedef struct mapcache_request_proxy mapcache_request_proxy;
typedef struct mapcache_request_get_capabilities mapcache_request_get_capabilities;
typedef struct mapcache_request_get_capabilities_demo mapcache_request_get_capabilities_demo;
typedef struct mapcache_request_get_capabilities_wms mapcache_request_get_capabilities_wms;
typedef struct mapcache_request_get_capabilities_wmts mapcache_request_get_capabilities_wmts;
typedef struct mapcache_forwarding_rule mapcache_forwarding_rule;
typedef struct mapcache_request_get_capabilities_tms mapcache_request_get_capabilities_tms;
typedef struct mapcache_request_get_capabilities_kml mapcache_request_get_capabilities_kml;

typedef struct mapcache_request_get_tile mapcache_request_get_tile;
typedef struct mapcache_request_get_map mapcache_request_get_map;
typedef struct mapcache_service mapcache_service;
typedef struct mapcache_service_wms mapcache_service_wms;
typedef struct mapcache_service_wmts mapcache_service_wmts;
typedef struct mapcache_service_gmaps mapcache_service_gmaps;
typedef struct mapcache_service_ve mapcache_service_ve;
typedef struct mapcache_service_tms mapcache_service_tms;
typedef struct mapcache_service_kml mapcache_service_kml;
typedef struct mapcache_service_demo mapcache_service_demo;
typedef struct mapcache_server_cfg mapcache_server_cfg;
typedef struct mapcache_image mapcache_image;
typedef struct mapcache_grid mapcache_grid;
typedef struct mapcache_grid_level mapcache_grid_level;
typedef struct mapcache_grid_link mapcache_grid_link;
typedef struct mapcache_context mapcache_context;
typedef struct mapcache_dimension mapcache_dimension;
typedef struct mapcache_dimension_time mapcache_dimension_time;
typedef struct mapcache_dimension_intervals mapcache_dimension_intervals;
typedef struct mapcache_dimension_values mapcache_dimension_values;
typedef struct mapcache_dimension_regex mapcache_dimension_regex;

/** \defgroup utility Utility */
/** @{ */



mapcache_image *mapcache_error_image(mapcache_context *ctx, int width, int height, char *msg);

/**
 * \interface mapcache_context
 * \brief structure passed to most mapcache functions to abstract common functions
 */
struct mapcache_context {
    /**
     * \brief indicate that an error has happened
     * \memberof mapcache_context
     * \param c
     * \param code the error code
     * \param message human readable message of what happened
     */
    void (*set_error)(mapcache_context *ctx, int code, char *message, ...);

    /**
     * \brief query context to know if an error has occured
     * \memberof mapcache_context
     */
    int (*get_error)(mapcache_context * ctx);

    /**
     * \brief get human readable message for the error
     * \memberof mapcache_context
     */
    char* (*get_error_message)(mapcache_context * ctx);
    
    /**
     * \brief get human readable message for the error
     * \memberof mapcache_context
     */
    void (*clear_errors)(mapcache_context * ctx);


    /**
     * \brief log a message
     * \memberof mapcache_context
     */
    void (*log)(mapcache_context *ctx, mapcache_log_level level, char *message, ...);

    /**
     * \brief aquire a lock shared between all instances of mapcache.
     *
     * depending on the underlying backend, this lock should be effective between threads and/or processes
     * \memberof mapcache_context
     * \param c
     * \param nonblocking should the call block while waiting for the lock. if set to 0, the function will
     *        return once the lock has been aquired. if set to 1, the function will return immediately, and
     *        the return value should be checked to verify if the lock was successfully aquired, or if
     *        another instance of mapcache had already quired the lock
     * \returns MAPCACHE_SUCCESS if the lock was aquired
     * \returns MAPCACHE_FAILURE if there was an error aquiring the lock
     * \returns MAPCACHE_LOCKED if \param nonblocking was set to 1 and another instance has already aquired
     *          the lock
     */
    void (*global_lock_aquire)(mapcache_context *ctx);

    /**
     * \brief release a previously aquired lock
     *
     * \memberof mapcache_context
     * calling this function after an unsuccessful call to global_lock_aquire() or from a different thread
     * or process that has called global_lock_aquire() has undefined behavior
     */
    void (*global_lock_release)(mapcache_context * ctx);

    const char* (*get_instance_id)(mapcache_context * ctx);

    apr_pool_t *pool;
    char *_contenttype;
    char *_errmsg;
    int _errcode;
    mapcache_cfg *config;
};

void mapcache_context_init(mapcache_context *ctx);

#define GC_CHECK_ERROR_RETURN(ctx) (if(((mapcache_context*)ctx)->_errcode) return MAPCACHE_FAILURE;)
#define GC_CHECK_ERROR(ctx) if(((mapcache_context*)ctx)->_errcode) return;
#define GC_HAS_ERROR(ctx) (((mapcache_context*)ctx)->_errcode > 0)

/**
 * \brief autoexpanding buffer that allocates memory from a pool
 * \sa mapcache_buffer_create()
 * \sa mapcache_buffer_append()
 * 
 */
struct mapcache_buffer {
    void* buf; /**< pointer to the actual data contained in buffer */
    size_t size; /**< number of bytes actually used in the buffer */
    size_t avail; /**< number of bytes allocated */
    apr_pool_t* pool; /**< apache pool to allocate from */
};

/* in buffer.c */
/**
 * \brief create and initialize a mapcache_buffer
 * \memberof mapcache_buffer
 * \param initialStorage the initial size that should be allocated in the buffer.
 *        defaults to #INITIAL_BUFFER_SIZE.
 * \param pool the pool from which to allocate memory.
 */
mapcache_buffer *mapcache_buffer_create(size_t initialStorage, apr_pool_t* pool);

/**
 * \brief append data
 * \memberof mapcache_buffer
 * \param buffer
 * \param len the lenght of the data to append.
 * \param data the data to append
 */
int mapcache_buffer_append(mapcache_buffer *buffer, size_t len, void *data);

/** @} */

/** \defgroup source Sources */

/** @{ */

typedef enum {
    MAPCACHE_SOURCE_WMS,
    MAPCACHE_SOURCE_GDAL
} mapcache_source_type;

/**\interface mapcache_source
 * \brief a source of data that can return image data
 */
struct mapcache_source {
    char *name; /**< the key this source can be referenced by */
    double data_extent[4]; /**< extent in which this source can produce data */
    mapcache_source_type type;
    apr_table_t *metadata;

    apr_array_header_t *info_formats;
    /**
     * \brief get the data for the metatile
     *
     * sets the mapcache_metatile::tile::data for the given tile
     */
    void (*render_map)(mapcache_context *ctx, mapcache_map *map);

    void (*query_info)(mapcache_context *ctx, mapcache_feature_info *fi);

    void (*configuration_parse_xml)(mapcache_context *ctx, ezxml_t xml, mapcache_source * source);
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
    void (*configuration_parse_json)(mapcache_context *ctx, cJSON *node, mapcache_source * source);
#endif
    void (*configuration_check)(mapcache_context *ctx, mapcache_source * source);
};

mapcache_http* mapcache_http_configuration_parse_xml(mapcache_context *ctx,ezxml_t node);
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
mapcache_http* mapcache_http_configuration_parse_json(mapcache_context *ctx,cJSON *node);
#endif
mapcache_http* mapcache_http_clone(mapcache_context *ctx, mapcache_http *orig);

struct mapcache_http {
   char *url; /**< the base url to request */
   apr_table_t *headers; /**< additional headers to add to the http request, eg, Referer */
   int connection_timeout;
   /* TODO: authentication */
};

/**\class mapcache_source_wms
 * \brief WMS mapcache_source
 * \implements mapcache_source
 */
struct mapcache_source_wms {
    mapcache_source source;
    apr_table_t *wms_default_params; /**< default WMS parameters (SERVICE,REQUEST,STYLES,VERSION) */
    apr_table_t *getmap_params; /**< WMS parameters specified in configuration */
    apr_table_t *getfeatureinfo_params; /**< WMS parameters specified in configuration */
    mapcache_http *http;
};

#if 0
#ifdef USE_GDAL
/**\class mapcache_source_gdal
 * \brief GDAL mapcache_source
 * \implements mapcache_source
 */
struct mapcache_source_gdal {
    mapcache_source source;
    char *datastr; /**< the gdal source string*/
    apr_table_t *gdal_params; /**< GDAL parameters specified in configuration */
    GDALDatasetH *poDataset;
};
#endif
/** @} */
#endif


/** \defgroup cache Caches */

/** @{ */
typedef enum {
    MAPCACHE_CACHE_DISK
#ifdef USE_MEMCACHE
       ,MAPCACHE_CACHE_MEMCACHE
#endif
#ifdef USE_SQLITE
       ,MAPCACHE_CACHE_SQLITE
#endif
} mapcache_cache_type;

/** \interface mapcache_cache
 * \brief a place to cache a mapcache_tile
 */
struct mapcache_cache {
    char *name; /**< key this cache is referenced by */
    mapcache_cache_type type;
    apr_table_t *metadata;
    
    /**
     * get tile content from cache
     * \returns MAPCACHE_SUCCESS if the data was correctly loaded from the disk
     * \returns MAPCACHE_FAILURE if the file exists but contains no data
     * \returns MAPCACHE_CACHE_MISS if the file does not exist on the disk
     * \memberof mapcache_cache
     */
    int (*tile_get)(mapcache_context *ctx, mapcache_tile * tile);
    
    /**
     * delete tile from cache
     * 
     * \memberof mapcache_cache
     */
    void (*tile_delete)(mapcache_context *ctx, mapcache_tile * tile);

    int (*tile_exists)(mapcache_context *ctx, mapcache_tile * tile);

    /**
     * set tile content to cache
     * \memberof mapcache_cache
     */
    void (*tile_set)(mapcache_context *ctx, mapcache_tile * tile);

    void (*configuration_parse_xml)(mapcache_context *ctx, ezxml_t xml, mapcache_cache * cache);
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
    void (*configuration_parse_json)(mapcache_context *ctx, cJSON *node, mapcache_cache * cache);
#endif
    void (*configuration_post_config)(mapcache_context *ctx, mapcache_cache * cache, mapcache_cfg *config);
};

/**\class mapcache_cache_disk
 * \brief a mapcache_cache on a filesytem
 * \implements mapcache_cache
 */
struct mapcache_cache_disk {
    mapcache_cache cache;
    char *base_directory;
    char *filename_template;
    int symlink_blank;
};


#ifdef USE_SQLITE
/**\class mapcache_cache_sqlite
 * \brief a mapcache_cache on a filesytem
 * \implements mapcache_cache
 */
typedef struct mapcache_cache_sqlite mapcache_cache_sqlite;
typedef struct mapcache_cache_sqlite_stmt mapcache_cache_sqlite_stmt;

struct mapcache_cache_sqlite_stmt {
   char *sql;
};

struct mapcache_cache_sqlite {
   mapcache_cache cache;
   char *dbname_template;
   int hitstats;
   mapcache_cache_sqlite_stmt create_stmt;
   mapcache_cache_sqlite_stmt exists_stmt;
   mapcache_cache_sqlite_stmt get_stmt;
   mapcache_cache_sqlite_stmt hitstat_stmt;
   mapcache_cache_sqlite_stmt set_stmt;
   mapcache_cache_sqlite_stmt delete_stmt;
};

/**
 * \memberof mapcache_cache_sqlite
 */
mapcache_cache* mapcache_cache_sqlite_create(mapcache_context *ctx);
mapcache_cache* mapcache_cache_mbtiles_create(mapcache_context *ctx);
#endif

#ifdef USE_MEMCACHE
typedef struct mapcache_cache_memcache mapcache_cache_memcache;
/**\class mapcache_cache_memcache
 * \brief a mapcache_cache on memcached servers
 * \implements mapcache_cache
 */
struct mapcache_cache_memcache {
    mapcache_cache cache;
    apr_memcache_t *memcache;
};

/**
 * \memberof mapcache_cache_memcache
 */
mapcache_cache* mapcache_cache_memcache_create(mapcache_context *ctx);
#endif

/** @} */


typedef enum {
   MAPCACHE_REQUEST_UNKNOWN,
   MAPCACHE_REQUEST_GET_TILE,
   MAPCACHE_REQUEST_GET_MAP,
   MAPCACHE_REQUEST_GET_CAPABILITIES,
   MAPCACHE_REQUEST_GET_FEATUREINFO,
   MAPCACHE_REQUEST_PROXY
} mapcache_request_type;

typedef enum {
   MAPCACHE_GETMAP_ERROR,
   MAPCACHE_GETMAP_ASSEMBLE,
   MAPCACHE_GETMAP_FORWARD
} mapcache_getmap_strategy;

typedef enum {
   MAPCACHE_RESAMPLE_NEAREST,
   MAPCACHE_RESAMPLE_BILINEAR
} mapcache_resample_mode;

/**
 * \brief a request sent by a client
 */

struct mapcache_request {
   mapcache_request_type type;
   mapcache_service *service;
};

struct mapcache_request_get_tile {
   mapcache_request request;

   /**
    * a list of tiles requested by the client
    */
   mapcache_tile **tiles;

   /**
    * the number of tiles requested by the client.
    * If more than one, and merging is enabled,
    * the supplied tiles will be merged together
    * before being returned to the client
    */
   int ntiles;
   mapcache_image_format *format;
   
};

struct mapcache_http_response {
   mapcache_buffer *data;
   apr_table_t *headers;
   long code;
   apr_time_t mtime;
};

struct mapcache_map {
   mapcache_tileset *tileset;
   mapcache_grid_link *grid_link;
   apr_table_t *dimensions;
   mapcache_buffer *data;
   int width, height;
   double extent[4];
   apr_time_t mtime; /**< last modification time */
   int expires; /**< time in seconds after which the tile should be rechecked for validity */
};

struct mapcache_feature_info {
   mapcache_map map;
   int i,j;
   char *format;
};

struct mapcache_request_get_feature_info {
   mapcache_request request;
   mapcache_feature_info *fi;
};

struct mapcache_request_get_map {
   mapcache_request request;
   mapcache_map **maps;
   int nmaps;
   mapcache_getmap_strategy getmap_strategy;
   mapcache_resample_mode resample_mode;
   mapcache_image_format *getmap_format;
};

struct mapcache_request_get_capabilities {
   mapcache_request request;

   /**
    * the body of the capabilities
    */
   char *capabilities;

   /**
    * the mime type
    */
   char *mime_type;
};

struct mapcache_request_get_capabilities_tms {
   mapcache_request_get_capabilities request;
   mapcache_tileset *tileset;
   mapcache_grid_link *grid_link;
   char *version;
};

struct mapcache_request_get_capabilities_kml {
   mapcache_request_get_capabilities request;
   mapcache_tile *tile;
};

struct mapcache_request_get_capabilities_wms {
   mapcache_request_get_capabilities request;
};

struct mapcache_request_get_capabilities_wmts {
   mapcache_request_get_capabilities request;
};

/**
 * the capabilities request for a specific service, to be able to create
 * demo pages specific to a given service
 */
struct mapcache_request_get_capabilities_demo {
   mapcache_request_get_capabilities request;
   mapcache_service *service;
};

struct mapcache_request_proxy {
   mapcache_request request;
   mapcache_http *http;
   apr_table_t *params;
   const char *pathinfo;
};

struct mapcache_forwarding_rule {
   char *name;
   mapcache_http *http;
   apr_array_header_t *match_params;  /* actually those are mapcache_dimensions */
   int append_pathinfo;
};




/** \defgroup services Services*/
/** @{ */

#define MAPCACHE_SERVICES_COUNT 7

typedef enum {
    MAPCACHE_SERVICE_TMS=0, MAPCACHE_SERVICE_WMTS,
    MAPCACHE_SERVICE_DEMO, MAPCACHE_SERVICE_GMAPS, MAPCACHE_SERVICE_KML,
    MAPCACHE_SERVICE_VE, MAPCACHE_SERVICE_WMS
} mapcache_service_type;

#define MAPCACHE_UNITS_COUNT 3
typedef enum {
    MAPCACHE_UNIT_METERS=0, MAPCACHE_UNIT_DEGREES, MAPCACHE_UNIT_FEET
} mapcache_unit;

/* defined in util.c*/
extern const double mapcache_meters_per_unit[MAPCACHE_UNITS_COUNT];

/** \interface mapcache_service
 * \brief a standard service (eg WMS, TMS)
 */
struct mapcache_service {
    char *name;
    mapcache_service_type type;
    
    /**
     * the pathinfo prefix of the url that routes to this service
     * eg, for accessing a wms service on http://host/mapcache/mywmsservice? ,
     * url_prefix would take the value "mywmsservice"
     */
    char *url_prefix;
    
    /**
     * \brief allocates and populates a mapcache_request corresponding to the parameters received
     */
    void (*parse_request)(mapcache_context *ctx, mapcache_service *service, mapcache_request **request, const char *path_info, apr_table_t *params, mapcache_cfg * config);

    /**
     * \param request the received request (should be of type MAPCACHE_REQUEST_CAPABILITIES
     * \param url the full url at which the service is available
     */
    void (*create_capabilities_response)(mapcache_context *ctx, mapcache_request_get_capabilities *request, char *url, char *path_info, mapcache_cfg *config);
    
    /**
     * parse advanced configuration options for the selected service
     */
    void (*configuration_parse_xml)(mapcache_context *ctx, ezxml_t xml, mapcache_service * service, mapcache_cfg *config);
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
    void (*configuration_parse_json)(mapcache_context *ctx, cJSON *node, mapcache_service * service, mapcache_cfg *config);
#endif
};

/**\class mapcache_service_wms
 * \brief an OGC WMS service
 * \implements mapcache_service
 */
struct mapcache_service_wms {
    mapcache_service service;
    apr_array_header_t *forwarding_rules;
    mapcache_getmap_strategy getmap_strategy;
    mapcache_resample_mode resample_mode;
    mapcache_image_format *getmap_format;
};

/**\class mapcache_service_kml
 * \brief a KML superoverlay service
 * \implements mapcache_service
 */
struct mapcache_service_kml {
    mapcache_service service;
};

/**\class mapcache_service_tms
 * \brief a TMS service
 * \implements mapcache_service
 */
struct mapcache_service_tms {
    mapcache_service service;
    int reverse_y;
};

/**\class mapcache_service_wmts
 * \brief a WMTS service
 * \implements mapcache_service
 */
struct mapcache_service_wmts {
    mapcache_service service;
};

/**\class mapcache_service_demo
 * \brief a demo service
 * \implements mapcache_service
 */
struct mapcache_service_demo {
    mapcache_service service;

};

/**\class mapcache_service_ve
 * \brief a virtualearth service
 * \implements mapcache_service
 */
struct mapcache_service_ve {
    mapcache_service service;
};

/**
 * \brief create and initialize a mapcache_service_wms
 * \memberof mapcache_service_wms
 */
mapcache_service* mapcache_service_wms_create(mapcache_context *ctx);

/**
 * \brief create and initialize a mapcache_service_ve
 * \memberof mapcache_service_ve
 */
mapcache_service* mapcache_service_ve_create(mapcache_context *ctx);

/**
 * \brief create and initialize a mapcache_service_gmaps
 * \memberof mapcache_service_gmaps
 */
mapcache_service* mapcache_service_gmaps_create(mapcache_context *ctx);

/**
 * \brief create and initialize a mapcache_service_kml
 * \memberof mapcache_service_kml
 */
mapcache_service* mapcache_service_kml_create(mapcache_context *ctx);

/**
 * \brief create and initialize a mapcache_service_tms
 * \memberof mapcache_service_tms
 */
mapcache_service* mapcache_service_tms_create(mapcache_context *ctx);

/**
 * \brief create and initialize a mapcache_service_wmts
 * \memberof mapcache_service_wtms
 */
mapcache_service* mapcache_service_wmts_create(mapcache_context *ctx);

/**
 * \brief create and initialize a mapcache_service_demo
 * \memberof mapcache_service_demo
 */
mapcache_service* mapcache_service_demo_create(mapcache_context *ctx);

/**
 * \brief return the request that corresponds to the given url
 */
void mapcache_service_dispatch_request(mapcache_context *ctx,
      mapcache_request **request,
      char *pathinfo,
      apr_table_t *params,
      mapcache_cfg *config);


/** @} */

/** \defgroup image Image Data Handling */

/** @{ */

typedef enum {
    GC_UNKNOWN, GC_PNG, GC_JPEG
} mapcache_image_format_type;

/**\class mapcache_image
 * \brief representation of an RGBA image
 * 
 * to access a pixel at position x,y, you should use the #GET_IMG_PIXEL macro
 */
struct mapcache_image {
    unsigned char *data; /**< pointer to the beginning of image data, stored in rgba order */
    size_t w; /**< width of the image */
    size_t h; /**< height of the image */
    size_t stride; /**< stride of an image row */
};

/** \def GET_IMG_PIXEL
 * return the address of a pixel
 * \param y the row
 * \param x the column
 * \param img the mapcache_image
 * \returns a pointer to the pixel
 */
#define GET_IMG_PIXEL(img,x,y) (&((img).data[(y)*(img).stride + (x)*4]))


/**
 * \brief initialize a new mapcache_image
 */
mapcache_image* mapcache_image_create(mapcache_context *ctx);

void mapcache_image_copy_resampled_nearest(mapcache_context *ctx, mapcache_image *src, mapcache_image *dst,
      double off_x, double off_y, double scale_x, double scale_y);
void mapcache_image_copy_resampled_bilinear(mapcache_context *ctx, mapcache_image *src, mapcache_image *dst,
      double off_x, double off_y, double scale_x, double scale_y);


/**
 * \brief merge two images
 * \param base the imae to merge onto
 * \param overlay the image to overlay onto
 * \param ctx the context
 * when finished, base will be modified and have overlay merged onto it
 */
void mapcache_image_merge(mapcache_context *ctx, mapcache_image *base, mapcache_image *overlay);

void mapcache_image_copy_resampled(mapcache_context *ctx, mapcache_image *src, mapcache_image *dst,
      int srcX, int srcY, int srcW, int srcH,
      int dstX, int dstY, int dstW, int dstH);

/**
 * \brief split the given metatile into tiles
 * \param mt the metatile to split
 * \param r the context
 */
void mapcache_image_metatile_split(mapcache_context *ctx, mapcache_metatile *mt);

/**
 * \brief check if given image is composed of a unique color
 * \param image the mapcache_image to process
 * \returns MAPCACHE_TRUE if the image contains a single color
 * \returns MAPCACHE_FALSE if the image has more than one color
 */
int mapcache_image_blank_color(mapcache_image* image);


/**
 * \brief check if image has some non opaque pixels
 */
int mapcache_image_has_alpha(mapcache_image *img);

/** @} */


/** \defgroup http HTTP Request handling*/
/** @{ */
void mapcache_http_do_request(mapcache_context *ctx, mapcache_http *req, mapcache_buffer *data, apr_table_t *headers, long *http_code);
void mapcache_http_do_request_with_params(mapcache_context *ctx, mapcache_http *req, apr_table_t *params,
      mapcache_buffer *data, apr_table_t *headers, long *http_code);
char* mapcache_http_build_url(mapcache_context *ctx, char *base, apr_table_t *params);
apr_table_t *mapcache_http_parse_param_string(mapcache_context *ctx, char *args);
/** @} */

/** \defgroup configuration Configuration*/

/** @{ */

struct mapcache_server_cfg {
   apr_global_mutex_t *mutex;
   char *mutex_name;
   apr_hash_t *aliases; /**< list of mapcache configurations aliased to a server uri */
};

/**
 * a configuration that will be served
 */
struct mapcache_cfg {
    char *configFile; /**< the filename from which this configuration was loaded */

    /**
     * a list of services that will be responded to
     */
    mapcache_service * services[MAPCACHE_SERVICES_COUNT];

    /**
     * hashtable containing configured mapcache_source%s
     */
    apr_hash_t *sources;

    /**
     * hashtable containing configured mapcache_cache%s
     */
    apr_hash_t *caches;

    /**
     * hashtable containing configured mapcache_tileset%s
     */
    apr_hash_t *tilesets;

    /**
     * hashtable containing configured mapcache_image_format%s
     */
    apr_hash_t *image_formats;

    /**
     * hashtable containing (pre)defined grids
     */
    apr_hash_t *grids;

    /**
     * the format to use for some miscelaneaous operations:
     *  - creating an empty image
     *  - creating an error image
     *  - as a fallback when merging multiple tiles
     */
    mapcache_image_format *default_image_format;

    /**
     * how should error messages be reported to the user
     */
    mapcache_error_reporting reporting;

    /**
     * encoded empty (tranpsarent) image that will be returned to clients if cofigured
     * to return blank images upon error
     */
    mapcache_buffer *empty_image;

    apr_table_t *metadata;

    const char *lockdir;
    const char *endpoint; /**< the uri where the base of the service is mapped */
};

/**
 *
 * @param filename
 * @param config
 * @param pool
 * @return
 */
void mapcache_configuration_parse(mapcache_context *ctx, const char *filename, mapcache_cfg *config, int cgi);
void mapcache_configuration_post_config(mapcache_context *ctx, mapcache_cfg *config);
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
void mapcache_configuration_parse_json(mapcache_context *ctx, const char *filename, mapcache_cfg *config);
void parse_keyvalues(mapcache_context *ctx, cJSON *node, apr_table_t *tbl);
#endif
void mapcache_configuration_parse_xml(mapcache_context *ctx, const char *filename, mapcache_cfg *config);
mapcache_cfg* mapcache_configuration_create(apr_pool_t *pool);
mapcache_source* mapcache_configuration_get_source(mapcache_cfg *config, const char *key);
mapcache_cache* mapcache_configuration_get_cache(mapcache_cfg *config, const char *key);
mapcache_grid *mapcache_configuration_get_grid(mapcache_cfg *config, const char *key);
mapcache_tileset* mapcache_configuration_get_tileset(mapcache_cfg *config, const char *key);
mapcache_image_format *mapcache_configuration_get_image_format(mapcache_cfg *config, const char *key);
void mapcache_configuration_add_image_format(mapcache_cfg *config, mapcache_image_format *format, const char * key);
void mapcache_configuration_add_source(mapcache_cfg *config, mapcache_source *source, const char * key);
void mapcache_configuration_add_grid(mapcache_cfg *config, mapcache_grid *grid, const char * key);
void mapcache_configuration_add_tileset(mapcache_cfg *config, mapcache_tileset *tileset, const char * key);
void mapcache_configuration_add_cache(mapcache_cfg *config, mapcache_cache *cache, const char * key);

/** @} */
/**
 * \memberof mapcache_source
 */
void mapcache_source_init(mapcache_context *ctx, mapcache_source *source);

/**
 * \memberof mapcache_source_gdal
 */
mapcache_source* mapcache_source_gdal_create(mapcache_context *ctx);

/**
 * \memberof mapcache_source_wms
 */
mapcache_source* mapcache_source_wms_create(mapcache_context *ctx);

/**
 * \memberof mapcache_cache_disk
 */
mapcache_cache* mapcache_cache_disk_create(mapcache_context *ctx);


/** \defgroup tileset Tilesets*/
/** @{ */

/**
 * \brief Tile
 * \sa mapcache_metatile
 * \sa mapcache_tileset::metasize_x mapcache_tileset::metasize_x mapcache_tileset::metabuffer
 */
struct mapcache_tile {
    mapcache_tileset *tileset; /**< the mapcache_tileset that corresponds to the tile*/
    mapcache_grid_link *grid_link;
    int x; /**< tile x index */
    int y; /**< tile y index */
    int z; /**< tile z index (zoom level) */
    /**
     * encoded image data for the tile.
     * \sa mapcache_cache::tile_get()
     * \sa mapcache_source::render_map()
     * \sa mapcache_image_format
     */
    mapcache_buffer *data;
    apr_time_t mtime; /**< last modification time */
    int expires; /**< time in seconds after which the tile should be rechecked for validity */
    
    apr_table_t *dimensions;
};

/**
 * \brief  MetaTile
 * \extends mapcache_tile
 */
struct mapcache_metatile {
   mapcache_map map;
   int x,y,z;
   int metasize_x, metasize_y;
   int ntiles; /**< the number of mapcache_metatile::tiles contained in this metatile */
   mapcache_tile *tiles; /**< the list of mapcache_tile s contained in this metatile */
   void *lock; /**< pointer to opaque structure set by the locking mechanism */
};


struct mapcache_grid_level {
   double resolution;
   unsigned int maxx, maxy;
};

struct mapcache_grid {
   char *name;
   int nlevels;
   char *srs;
   apr_array_header_t *srs_aliases;
   double extent[4];
   mapcache_unit unit;
   int tile_sx, tile_sy; /**<width and height of a tile in pixels */
   mapcache_grid_level **levels;
   apr_table_t *metadata;
};


struct mapcache_grid_link {
   mapcache_grid *grid;
   /**
    * precalculated limits for available each level: [minTileX, minTileY, maxTileX, maxTileY].
    *
    * a request is valid if x is in [minTileX, maxTileX[ and y in [minTileY,maxTileY]
    */
   double *restricted_extent;
   int **grid_limits;
};

/**\class mapcache_tileset
 * \brief a set of tiles that can be requested by a client, created from a mapcache_source
 *        stored by a mapcache_cache in a mapcache_format
 */
struct mapcache_tileset {
    /**
     * the name this tileset will be referenced by.
     * this is the key that is passed by clients e.g. in a WMS LAYERS= parameter
     */
    char *name;

    /**
     * the extent of the tileset in lonlat
     */
    double wgs84bbox[4];

    /**
     * list of grids that will be cached
     */
    apr_array_header_t *grid_links;

    /**
     * size of the metatile that should be requested to the mapcache_tileset::source
     */
    int metasize_x, metasize_y;

    /**
     * size of the gutter around the metatile that should be requested to the mapcache_tileset::source
     */
    int metabuffer;

    /**
     * number of seconds that should be returned to the client in an Expires: header
     *
     * \sa auto_expire
     */
    int expires;
    
    /**
     * number of seconds after which a tile will be regenerated from the source
     * 
     * will take precedence over the #expires parameter.
     * \sa expires
     */
    int auto_expire;

    /**
     * the cache in which the tiles should be stored
     */
    mapcache_cache *cache;

    /**
     * the source from which tiles should be requested
     */
    mapcache_source *source;

    /**
     * the format to use when storing tiles coming from a metatile
     */
    mapcache_image_format *format;

    /**
     * a list of parameters that can be forwarded from the client to the mapcache_tileset::source
     */
    apr_array_header_t *dimensions;
    
    /**
     * image to be used as a watermark
     */
    mapcache_image *watermark;

    /**
     * handle to the configuration this tileset belongs to
     */
    mapcache_cfg *config;
   
    /**
     * should we service wms requests not aligned to a grid
     */
    int full_wms;

    apr_table_t *metadata;
};

void mapcache_tileset_get_map_tiles(mapcache_context *ctx, mapcache_tileset *tileset,
      mapcache_grid_link *grid_link,
      double *bbox, int width, int height,
      int *ntiles,
      mapcache_tile ***tiles);

mapcache_image* mapcache_tileset_assemble_map_tiles(mapcache_context *ctx, mapcache_tileset *tileset,
      mapcache_grid_link *grid_link,
      double *bbox, int width, int height,
      int ntiles,
      mapcache_tile **tiles,
      mapcache_resample_mode mode);

/**
 * compute x,y,z value given a bbox.
 * will return MAPCACHE_FAILURE
 * if the bbox does not correspond to the tileset's configuration
 */
int mapcache_grid_get_cell(mapcache_context *ctx, mapcache_grid *grid, double *bbox,
      int *x, int *y, int *z);

/**
 * \brief verify the created tile respects configured constraints
 * @param tile
 * @param r
 * @return
 */
void mapcache_tileset_tile_validate(mapcache_context *ctx, mapcache_tile *tile);

/**
 * compute level for a given resolution
 * 
 * computes the integer level for the given resolution. the input resolution will be set to the exact
 * value configured for the tileset, to compensate for rounding errors that could creep in if using
 * the resolution calculated from input parameters
 * 
 * \returns MAPCACHE_TILESET_WRONG_RESOLUTION if the given resolution is't configured
 * \returns MAPCACHE_SUCCESS if the level was found
 */
void mapcache_tileset_get_level(mapcache_context *ctx, mapcache_tileset *tileset, double *resolution, int *level);

void mapcache_grid_get_closest_level(mapcache_context *ctx, mapcache_grid *grid, double resolution, int *level);
void mapcache_tileset_tile_get(mapcache_context *ctx, mapcache_tile *tile);

/**
 * \brief delete tile from cache
 * @param whole_metatile delete all the other tiles from the metatile to
 */
void mapcache_tileset_tile_delete(mapcache_context *ctx, mapcache_tile *tile, int whole_metatile);

int mapcache_grid_is_bbox_aligned(mapcache_context *ctx, mapcache_grid *grid, double *bbox);

/**
 * \brief create and initialize a tile for the given tileset and grid_link
 * @param tileset
 * @param grid_link
 * @param pool
 * @return
 */
mapcache_tile* mapcache_tileset_tile_create(apr_pool_t *pool, mapcache_tileset *tileset, mapcache_grid_link *grid_link);

/**
 * \brief create and initialize a map for the given tileset and grid_link
 * @param tileset
 * @param grid_link
 * @param pool
 * @return
 */
mapcache_map* mapcache_tileset_map_create(apr_pool_t *pool, mapcache_tileset *tileset, mapcache_grid_link *grid_link);


/**
 * \brief create and initialize a feature_info for the given tileset and grid_link
 */
mapcache_feature_info* mapcache_tileset_feature_info_create(apr_pool_t *pool, mapcache_tileset *tileset,
      mapcache_grid_link *grid_link);

/**
 * \brief create and initalize a tileset
 * @param pool
 * @return
 */
mapcache_tileset* mapcache_tileset_create(mapcache_context *ctx);

void mapcache_tileset_configuration_check(mapcache_context *ctx, mapcache_tileset *tileset);
void mapcache_tileset_add_watermark(mapcache_context *ctx, mapcache_tileset *tileset, const char *filename);


/**
 * lock the tile
 */
void mapcache_tileset_metatile_lock(mapcache_context *ctx, mapcache_metatile *tile);

mapcache_metatile* mapcache_tileset_metatile_get(mapcache_context *ctx, mapcache_tile *tile);

/**
 * unlock the tile
 */
void mapcache_tileset_metatile_unlock(mapcache_context *ctx, mapcache_metatile *tile);

/**
 * check if there is a lock on the tile (the lock will have been set by another process/thread)
 * \returns MAPCACHE_TRUE if the tile is locked by another process/thread
 * \returns MAPCACHE_FALSE if there is no lock on the tile
 */
int mapcache_tileset_metatile_lock_exists(mapcache_context *ctx, mapcache_metatile *tile);

/**
 * wait for the lock set on the tile (the lock will have been set by another process/thread)
 */
void mapcache_tileset_metatile_lock_wait(mapcache_context *ctx, mapcache_metatile *tile);


/** @} */



mapcache_http_response* mapcache_core_get_capabilities(mapcache_context *ctx, mapcache_service *service, mapcache_request_get_capabilities *req_caps, char *url, char *path_info, mapcache_cfg *config);
mapcache_http_response* mapcache_core_get_tile(mapcache_context *ctx, mapcache_request_get_tile *req_tile);

mapcache_http_response* mapcache_core_get_map(mapcache_context *ctx, mapcache_request_get_map *req_map);

mapcache_http_response* mapcache_core_get_featureinfo(mapcache_context *ctx, mapcache_request_get_feature_info *req_fi);

mapcache_http_response* mapcache_core_proxy_request(mapcache_context *ctx, mapcache_request_proxy *req_proxy);
mapcache_http_response* mapcache_core_respond_to_error(mapcache_context *ctx, mapcache_service *service);


/* in grid.c */
mapcache_grid* mapcache_grid_create(apr_pool_t *pool);

const char* mapcache_grid_get_crs(mapcache_context *ctx, mapcache_grid *grid);
const char* mapcache_grid_get_srs(mapcache_context *ctx, mapcache_grid *grid);

void mapcache_grid_get_extent(mapcache_context *ctx, mapcache_grid *grid,
      int x, int y, int z, double *bbox);
/**
 * \brief compute x y value for given lon/lat (dx/dy) and given zoomlevel
 * @param ctx
 * @param tileset
 * @param dx
 * @param dy
 * @param z
 * @param x
 * @param y
 */
void mapcache_grid_get_xy(mapcache_context *ctx, mapcache_grid *grid, double dx, double dy, int z, int *x, int *y);

double mapcache_grid_get_resolution(double *bbox, int sx, int sy);
double mapcache_grid_get_horizontal_resolution(double *bbox, int width);
double mapcache_grid_get_vertical_resolution(double *bbox, int height);

/**
 * \brief compute grid level given a resolution
 * \param grid
 * \param resolution
 * \param level
 */
int mapcache_grid_get_level(mapcache_context *ctx, mapcache_grid *grid, double *resolution, int *level);

/**
 * \brief precompute min/max x/y values for the given extent
 * \param grid 
 * \param extent
 * \param tolerance the number of tiles around the given extent that can be requested without returning an error.
 */
void mapcache_grid_compute_limits(const mapcache_grid *grid, const double *extent, int **limits, int tolerance);

/* in util.c */
int mapcache_util_extract_int_list(mapcache_context *ctx, const char* args, const char *sep, int **numbers,
        int *numbers_count);
int mapcache_util_extract_double_list(mapcache_context *ctx, const char* args, const char *sep, double **numbers,
        int *numbers_count);
char *mapcache_util_str_replace(apr_pool_t *pool, const char *string, const char *substr,
      const char *replacement );

/*
int mapcache_util_mutex_aquire(mapcache_context *r);
int mapcache_util_mutex_release(mapcache_context *r);
 */



/**\defgroup imageio Image IO */
/** @{ */

/**
 * compression strategy to apply
 */
typedef enum {
    MAPCACHE_COMPRESSION_BEST, /**< best but slowest compression*/
    MAPCACHE_COMPRESSION_FAST, /**< fast compression*/
    MAPCACHE_COMPRESSION_DEFAULT /**< default compression*/
} mapcache_compression_type;

/**\interface mapcache_image_format
 * \brief an image format
 * \sa mapcache_image_format_jpeg
 * \sa mapcache_image_format_png
 */
struct mapcache_image_format {
    char *name; /**< the key by which this format will be referenced */
    char *extension; /**< the extension to use when saving a file with this format */
    char *mime_type;
    mapcache_buffer * (*write)(mapcache_context *ctx, mapcache_image *image, mapcache_image_format * format);
    /**< pointer to a function that returns a mapcache_buffer containing the given image encoded
     * in the specified format
     */

    mapcache_buffer* (*create_empty_image)(mapcache_context *ctx, mapcache_image_format *format,
          size_t width, size_t height, unsigned int color);
    apr_table_t *metadata;
};

/**\defgroup imageio_png PNG Image IO
 * \ingroup imageio */
/** @{ */

/**\class mapcache_image_format_png
 * \brief PNG image format
 * \extends mapcache_image_format
 * \sa mapcache_image_format_png_q
 */
struct mapcache_image_format_png {
    mapcache_image_format format;
    mapcache_compression_type compression_level; /**< PNG compression level to apply */
};

struct mapcache_image_format_mixed {
   mapcache_image_format format;
   mapcache_image_format *transparent;
   mapcache_image_format *opaque;
};


mapcache_image_format* mapcache_imageio_create_mixed_format(apr_pool_t *pool,
      char *name, mapcache_image_format *transparent, mapcache_image_format *opaque);

/**\class mapcache_image_format_png_q
 * \brief Quantized PNG format
 * \extends mapcache_image_format_png
 */
struct mapcache_image_format_png_q {
    mapcache_image_format_png format;
    int ncolors; /**< number of colors used in quantization, 2-256 */
};

/**
 * @param r
 * @param buffer
 * @return
 */
mapcache_image* _mapcache_imageio_png_decode(mapcache_context *ctx, mapcache_buffer *buffer);

void mapcache_image_create_empty(mapcache_context *ctx, mapcache_cfg *cfg);
/**
 * @param r
 * @param buffer
 * @return
 */
void _mapcache_imageio_png_decode_to_image(mapcache_context *ctx, mapcache_buffer *buffer,
      mapcache_image *image);


/**
 * \brief create a format capable of creating RGBA png
 * \memberof mapcache_image_format_png
 * @param pool
 * @param name
 * @param compression the ZLIB compression to apply
 * @return
 */
mapcache_image_format* mapcache_imageio_create_png_format(apr_pool_t *pool, char *name, mapcache_compression_type compression);

/**
 * \brief create a format capable of creating quantized png
 * \memberof mapcache_image_format_png_q
 * @param pool
 * @param name
 * @param compression the ZLIB compression to apply
 * @param ncolors the number of colors to quantize with
 * @return
 */
mapcache_image_format* mapcache_imageio_create_png_q_format(apr_pool_t *pool, char *name, mapcache_compression_type compression, int ncolors);

/** @} */

/**\defgroup imageio_jpg JPEG Image IO
 * \ingroup imageio */
/** @{ */

/**\class mapcache_image_format_jpeg
 * \brief JPEG image format
 * \extends mapcache_image_format
 */
struct mapcache_image_format_jpeg {
    mapcache_image_format format;
    int quality; /**< JPEG quality, 1-100 */
};

mapcache_image_format* mapcache_imageio_create_jpeg_format(apr_pool_t *pool, char *name, int quality);

/**
 * @param r
 * @param buffer
 * @return
 */
mapcache_image* _mapcache_imageio_jpeg_decode(mapcache_context *ctx, mapcache_buffer *buffer);

/**
 * @param r
 * @param buffer
 * @return
 */
void _mapcache_imageio_jpeg_decode_to_image(mapcache_context *ctx, mapcache_buffer *buffer,
      mapcache_image *image);

/** @} */

/**
 * \brief lookup the first few bytes of a buffer to check for a known image format
 */
mapcache_image_format_type mapcache_imageio_header_sniff(mapcache_context *ctx, mapcache_buffer *buffer);

/**
 * \brief checks if the given buffer is a recognized image format
 */
int mapcache_imageio_is_valid_format(mapcache_context *ctx, mapcache_buffer *buffer);


/**
 * decodes given buffer
 */
mapcache_image* mapcache_imageio_decode(mapcache_context *ctx, mapcache_buffer *buffer);

/**
 * decodes given buffer to an allocated image
 */
void mapcache_imageio_decode_to_image(mapcache_context *ctx, mapcache_buffer *buffer, mapcache_image *image);


/** @} */

typedef struct {
   double start;
   double end;
   double resolution;
} mapcache_interval;

typedef enum {
   MAPCACHE_DIMENSION_VALUES,
   MAPCACHE_DIMENSION_REGEX,
   MAPCACHE_DIMENSION_INTERVALS,
   MAPCACHE_DIMENSION_TIME
} mapcache_dimension_type;

struct mapcache_dimension {
   mapcache_dimension_type type;
   char *name;
   char *unit;
   apr_table_t *metadata;
   char *default_value;
   
   /**
    * \brief validate the given value
    * 
    * \param value is updated in case the given value is correct but has to be represented otherwise,
    * e.g. to round off a value
    * \returns MAPCACHE_SUCCESS if the given value is correct for the current dimension
    * \returns MAPCACHE_FAILURE if not
    */
   int (*validate)(mapcache_context *context, mapcache_dimension *dimension, char **value);
   
   /**
    * \brief returns a list of values that are authorized for this dimension
    *
    * \returns a list of character strings that will be included in the capabilities <dimension> element
    */
   const char** (*print_ogc_formatted_values)(mapcache_context *context, mapcache_dimension *dimension);
   
   /**
    * \brief parse the value given in the configuration
    */
   void (*configuration_parse_xml)(mapcache_context *context, mapcache_dimension *dim, ezxml_t node);
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
   void (*configuration_parse_json)(mapcache_context *context, mapcache_dimension *dim, cJSON *node);
#endif
};

struct mapcache_dimension_values {
   mapcache_dimension dimension;
   int nvalues;
   char **values;
   int case_sensitive;
};

struct mapcache_dimension_regex {
   mapcache_dimension dimension;
   char *regex_string;
#ifdef USE_PCRE
   pcre *pcregex;
#else
   regex_t *regex;
#endif
};

struct mapcache_dimension_intervals {
   mapcache_dimension dimension;
   int nintervals;
   mapcache_interval *intervals;
};

struct mapcache_dimension_time {
   mapcache_dimension dimension;
   int nintervals;
   mapcache_interval *intervals;
};

mapcache_dimension* mapcache_dimension_values_create(apr_pool_t *pool);
mapcache_dimension* mapcache_dimension_regex_create(apr_pool_t *pool);
mapcache_dimension* mapcache_dimension_intervals_create(apr_pool_t *pool);
mapcache_dimension* mapcache_dimension_time_create(apr_pool_t *pool);


int mapcache_is_axis_inverted(const char *srs);

#endif /* MAPCACHE_H_ */
/* vim: ai ts=3 sts=3 et sw=3
*/

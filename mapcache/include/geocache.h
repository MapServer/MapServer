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

/*! \file geocache.h
    \brief global function and structure declarations
 */


#ifndef GEOCACHE_H_
#define GEOCACHE_H_

#include <apr_tables.h>
#include <apr_hash.h>
#include <apr_global_mutex.h>
#include "util.h"
#include "ezxml.h"
#include "cJSON.h"
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

#define GEOCACHE_SUCCESS 0
#define GEOCACHE_FAILURE 1
#define GEOCACHE_TRUE 1
#define GEOCACHE_FALSE 0
#define GEOCACHE_TILESET_WRONG_SIZE 2
#define GEOCACHE_TILESET_WRONG_RESOLUTION 3
#define GEOCACHE_TILESET_WRONG_EXTENT 4
#define GEOCACHE_CACHE_MISS 5
#define GEOCACHE_FILE_LOCKED 6

#define GEOCACHE_VERSION "0.5-dev"
#define GEOCACHE_USERAGENT "mod-geocache/"GEOCACHE_VERSION

#define GEOCACHE_LOCKFILE_PREFIX "_gc_lock"



typedef struct geocache_image_format geocache_image_format;
typedef struct geocache_image_format_mixed geocache_image_format_mixed;
typedef struct geocache_image_format_png geocache_image_format_png;
typedef struct geocache_image_format_png_q geocache_image_format_png_q;
typedef struct geocache_image_format_jpeg geocache_image_format_jpeg;
typedef struct geocache_cfg geocache_cfg;
typedef struct geocache_tileset geocache_tileset;
typedef struct geocache_cache geocache_cache;
typedef struct geocache_source geocache_source;
typedef struct geocache_buffer geocache_buffer;
typedef struct geocache_tile geocache_tile;
typedef struct geocache_metatile geocache_metatile;
typedef struct geocache_feature_info geocache_feature_info;
typedef struct geocache_request_get_feature_info geocache_request_get_feature_info;
typedef struct geocache_map geocache_map;
typedef struct geocache_proxied_response geocache_proxied_response;
typedef struct geocache_source_wms geocache_source_wms;
#if 0
typedef struct geocache_source_gdal geocache_source_gdal;
#endif
typedef struct geocache_cache_disk geocache_cache_disk;
typedef struct geocache_http geocache_http;
typedef struct geocache_request geocache_request;
typedef struct geocache_request_proxy geocache_request_proxy;
typedef struct geocache_request_get_capabilities geocache_request_get_capabilities;
typedef struct geocache_request_get_capabilities_demo geocache_request_get_capabilities_demo;
typedef struct geocache_request_get_capabilities_wms geocache_request_get_capabilities_wms;
typedef struct geocache_request_get_capabilities_wmts geocache_request_get_capabilities_wmts;
typedef struct geocache_forwarding_rule geocache_forwarding_rule;
typedef struct geocache_request_get_capabilities_tms geocache_request_get_capabilities_tms;
typedef struct geocache_request_get_capabilities_kml geocache_request_get_capabilities_kml;

typedef struct geocache_request_get_tile geocache_request_get_tile;
typedef struct geocache_request_get_map geocache_request_get_map;
typedef struct geocache_service geocache_service;
typedef struct geocache_service_wms geocache_service_wms;
typedef struct geocache_service_wmts geocache_service_wmts;
typedef struct geocache_service_gmaps geocache_service_gmaps;
typedef struct geocache_service_ve geocache_service_ve;
typedef struct geocache_service_tms geocache_service_tms;
typedef struct geocache_service_kml geocache_service_kml;
typedef struct geocache_service_demo geocache_service_demo;
typedef struct geocache_server_cfg geocache_server_cfg;
typedef struct geocache_image geocache_image;
typedef struct geocache_grid geocache_grid;
typedef struct geocache_grid_level geocache_grid_level;
typedef struct geocache_grid_link geocache_grid_link;
typedef struct geocache_context geocache_context;
typedef struct geocache_dimension geocache_dimension;
typedef struct geocache_dimension_time geocache_dimension_time;
typedef struct geocache_dimension_intervals geocache_dimension_intervals;
typedef struct geocache_dimension_values geocache_dimension_values;
typedef struct geocache_dimension_regex geocache_dimension_regex;

/** \defgroup utility Utility */
/** @{ */



geocache_image *geocache_error_image(geocache_context *ctx, int width, int height, char *msg);

/**
 * \interface geocache_context
 * \brief structure passed to most geocache functions to abstract common functions
 */
struct geocache_context {
    /**
     * \brief indicate that an error has happened
     * \memberof geocache_context
     * \param c
     * \param code the error code
     * \param message human readable message of what happened
     */
    void (*set_error)(geocache_context *ctx, int code, char *message, ...);

    /**
     * \brief query context to know if an error has occured
     * \memberof geocache_context
     */
    int (*get_error)(geocache_context * ctx);

    /**
     * \brief get human readable message for the error
     * \memberof geocache_context
     */
    char* (*get_error_message)(geocache_context * ctx);
    
    /**
     * \brief get human readable message for the error
     * \memberof geocache_context
     */
    void (*clear_errors)(geocache_context * ctx);


    /**
     * \brief log a message
     * \memberof geocache_context
     */
    void (*log)(geocache_context *ctx, geocache_log_level level, char *message, ...);

    /**
     * \brief aquire a lock shared between all instances of geocache.
     *
     * depending on the underlying backend, this lock should be effective between threads and/or processes
     * \memberof geocache_context
     * \param c
     * \param nonblocking should the call block while waiting for the lock. if set to 0, the function will
     *        return once the lock has been aquired. if set to 1, the function will return immediately, and
     *        the return value should be checked to verify if the lock was successfully aquired, or if
     *        another instance of geocache had already quired the lock
     * \returns GEOCACHE_SUCCESS if the lock was aquired
     * \returns GEOCACHE_FAILURE if there was an error aquiring the lock
     * \returns GEOCACHE_LOCKED if \param nonblocking was set to 1 and another instance has already aquired
     *          the lock
     */
    void (*global_lock_aquire)(geocache_context *ctx);

    /**
     * \brief release a previously aquired lock
     *
     * \memberof geocache_context
     * calling this function after an unsuccessful call to global_lock_aquire() or from a different thread
     * or process that has called global_lock_aquire() has undefined behavior
     */
    void (*global_lock_release)(geocache_context * ctx);

    const char* (*get_instance_id)(geocache_context * ctx);

    apr_pool_t *pool;
    char *_contenttype;
    char *_errmsg;
    int _errcode;
    geocache_cfg *config;
};

void geocache_context_init(geocache_context *ctx);

#define GC_CHECK_ERROR_RETURN(ctx) (if(((geocache_context*)ctx)->_errcode) return GEOCACHE_FAILURE;)
#define GC_CHECK_ERROR(ctx) if(((geocache_context*)ctx)->_errcode) return;
#define GC_HAS_ERROR(ctx) (((geocache_context*)ctx)->_errcode > 0)

/**
 * \brief autoexpanding buffer that allocates memory from a pool
 * \sa geocache_buffer_create()
 * \sa geocache_buffer_append()
 * 
 */
struct geocache_buffer {
    unsigned char* buf; /**< pointer to the actual data contained in buffer */
    size_t size; /**< number of bytes actually used in the buffer */
    size_t avail; /**< number of bytes allocated */
    apr_pool_t* pool; /**< apache pool to allocate from */
};

/* in buffer.c */
/**
 * \brief create and initialize a geocache_buffer
 * \memberof geocache_buffer
 * \param initialStorage the initial size that should be allocated in the buffer.
 *        defaults to #INITIAL_BUFFER_SIZE.
 * \param pool the pool from which to allocate memory.
 */
geocache_buffer *geocache_buffer_create(size_t initialStorage, apr_pool_t* pool);

/**
 * \brief append data
 * \memberof geocache_buffer
 * \param buffer
 * \param len the lenght of the data to append.
 * \param data the data to append
 */
int geocache_buffer_append(geocache_buffer *buffer, size_t len, void *data);

/** @} */

/** \defgroup source Sources */

/** @{ */

typedef enum {
    GEOCACHE_SOURCE_WMS,
    GEOCACHE_SOURCE_GDAL
} geocache_source_type;

/**\interface geocache_source
 * \brief a source of data that can return image data
 */
struct geocache_source {
    char *name; /**< the key this source can be referenced by */
    double data_extent[4]; /**< extent in which this source can produce data */
    geocache_source_type type;
    apr_table_t *metadata;

    apr_array_header_t *info_formats;
    /**
     * \brief get the data for the metatile
     *
     * sets the geocache_metatile::tile::data for the given tile
     */
    void (*render_map)(geocache_context *ctx, geocache_map *map);

    void (*query_info)(geocache_context *ctx, geocache_feature_info *fi);

    void (*configuration_parse_xml)(geocache_context *ctx, ezxml_t xml, geocache_source * source);
    void (*configuration_parse_json)(geocache_context *ctx, cJSON *node, geocache_source * source);
    void (*configuration_check)(geocache_context *ctx, geocache_source * source);
};

geocache_http* geocache_http_configuration_parse_xml(geocache_context *ctx,ezxml_t node);
geocache_http* geocache_http_configuration_parse_json(geocache_context *ctx,cJSON *node);
geocache_http* geocache_http_clone(geocache_context *ctx, geocache_http *orig);

struct geocache_http {
   char *url; /**< the base url to request */
   apr_table_t *headers; /**< additional headers to add to the http request, eg, Referer */
   /* TODO: authentication */
};

/**\class geocache_source_wms
 * \brief WMS geocache_source
 * \implements geocache_source
 */
struct geocache_source_wms {
    geocache_source source;
    apr_table_t *wms_default_params; /**< default WMS parameters (SERVICE,REQUEST,STYLES,VERSION) */
    apr_table_t *getmap_params; /**< WMS parameters specified in configuration */
    apr_table_t *getfeatureinfo_params; /**< WMS parameters specified in configuration */
    geocache_http *http;
};

#if 0
#ifdef USE_GDAL
/**\class geocache_source_gdal
 * \brief GDAL geocache_source
 * \implements geocache_source
 */
struct geocache_source_gdal {
    geocache_source source;
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
    GEOCACHE_CACHE_DISK
#ifdef USE_MEMCACHE
       ,GEOCACHE_CACHE_MEMCACHE
#endif
#ifdef USE_SQLITE
       ,GEOCACHE_CACHE_SQLITE
#endif
} geocache_cache_type;

/** \interface geocache_cache
 * \brief a place to cache a geocache_tile
 */
struct geocache_cache {
    char *name; /**< key this cache is referenced by */
    geocache_cache_type type;
    apr_table_t *metadata;
    
    /**
     * get tile content from cache
     * \returns GEOCACHE_SUCCESS if the data was correctly loaded from the disk
     * \returns GEOCACHE_FAILURE if the file exists but contains no data
     * \returns GEOCACHE_CACHE_MISS if the file does not exist on the disk
     * \memberof geocache_cache
     */
    int (*tile_get)(geocache_context *ctx, geocache_tile * tile);
    
    /**
     * delete tile from cache
     * 
     * \memberof geocache_cache
     */
    void (*tile_delete)(geocache_context *ctx, geocache_tile * tile);

    int (*tile_exists)(geocache_context *ctx, geocache_tile * tile);

    /**
     * set tile content to cache
     * \memberof geocache_cache
     */
    void (*tile_set)(geocache_context *ctx, geocache_tile * tile);

    void (*configuration_parse_xml)(geocache_context *ctx, ezxml_t xml, geocache_cache * cache);
    void (*configuration_parse_json)(geocache_context *ctx, cJSON *node, geocache_cache * cache);
    void (*configuration_post_config)(geocache_context *ctx, geocache_cache * cache, geocache_cfg *config);
};

/**\class geocache_cache_disk
 * \brief a geocache_cache on a filesytem
 * \implements geocache_cache
 */
struct geocache_cache_disk {
    geocache_cache cache;
    char *base_directory;
    char *filename_template;
    int symlink_blank;
};


#ifdef USE_SQLITE
/**\class geocache_cache_sqlite
 * \brief a geocache_cache on a filesytem
 * \implements geocache_cache
 */
typedef struct geocache_cache_sqlite geocache_cache_sqlite;
typedef struct geocache_cache_sqlite_stmt geocache_cache_sqlite_stmt;

struct geocache_cache_sqlite_stmt {
   char *sql;
};

struct geocache_cache_sqlite {
   geocache_cache cache;
   char *dbname_template;
   int hitstats;
   geocache_cache_sqlite_stmt create_stmt;
   geocache_cache_sqlite_stmt exists_stmt;
   geocache_cache_sqlite_stmt get_stmt;
   geocache_cache_sqlite_stmt hitstat_stmt;
   geocache_cache_sqlite_stmt set_stmt;
   geocache_cache_sqlite_stmt delete_stmt;
};

/**
 * \memberof geocache_cache_sqlite
 */
geocache_cache* geocache_cache_sqlite_create(geocache_context *ctx);
geocache_cache* geocache_cache_mbtiles_create(geocache_context *ctx);
#endif

#ifdef USE_MEMCACHE
typedef struct geocache_cache_memcache geocache_cache_memcache;
/**\class geocache_cache_memcache
 * \brief a geocache_cache on memcached servers
 * \implements geocache_cache
 */
struct geocache_cache_memcache {
    geocache_cache cache;
    apr_memcache_t *memcache;
};

/**
 * \memberof geocache_cache_memcache
 */
geocache_cache* geocache_cache_memcache_create(geocache_context *ctx);
#endif

/** @} */


typedef enum {
   GEOCACHE_REQUEST_UNKNOWN,
   GEOCACHE_REQUEST_GET_TILE,
   GEOCACHE_REQUEST_GET_MAP,
   GEOCACHE_REQUEST_GET_CAPABILITIES,
   GEOCACHE_REQUEST_GET_FEATUREINFO,
   GEOCACHE_REQUEST_PROXY
} geocache_request_type;

typedef enum {
   GEOCACHE_GETMAP_ERROR,
   GEOCACHE_GETMAP_ASSEMBLE,
   GEOCACHE_GETMAP_FORWARD
} geocache_getmap_strategy;

typedef enum {
   GEOCACHE_RESAMPLE_NEAREST,
   GEOCACHE_RESAMPLE_BILINEAR
} geocache_resample_mode;

/**
 * \brief a request sent by a client
 */

struct geocache_request {
   geocache_request_type type;
   geocache_service *service;
};

struct geocache_request_get_tile {
   geocache_request request;

   /**
    * a list of tiles requested by the client
    */
   geocache_tile **tiles;

   /**
    * the number of tiles requested by the client.
    * If more than one, and merging is enabled,
    * the supplied tiles will be merged together
    * before being returned to the client
    */
   int ntiles;
   geocache_image_format *format;
   
};

struct geocache_proxied_response {
   geocache_buffer *data;
   apr_table_t *headers;
};

struct geocache_map {
   geocache_tileset *tileset;
   geocache_grid_link *grid_link;
   apr_table_t *dimensions;
   geocache_buffer *data;
   int width, height;
   double extent[4];
   apr_time_t mtime; /**< last modification time */
   int expires; /**< time in seconds after which the tile should be rechecked for validity */
};

struct geocache_feature_info {
   geocache_map map;
   int i,j;
   char *format;
};

struct geocache_request_get_feature_info {
   geocache_request request;
   geocache_feature_info *fi;
};

struct geocache_request_get_map {
   geocache_request request;
   geocache_map **maps;
   int nmaps;
   geocache_getmap_strategy getmap_strategy;
   geocache_resample_mode resample_mode;
   geocache_image_format *getmap_format;
};

struct geocache_request_get_capabilities {
   geocache_request request;

   /**
    * the body of the capabilities
    */
   char *capabilities;

   /**
    * the mime type
    */
   char *mime_type;
};

struct geocache_request_get_capabilities_tms {
   geocache_request_get_capabilities request;
   geocache_tileset *tileset;
   geocache_grid_link *grid_link;
   char *version;
};

struct geocache_request_get_capabilities_kml {
   geocache_request_get_capabilities request;
   geocache_tile *tile;
};

struct geocache_request_get_capabilities_wms {
   geocache_request_get_capabilities request;
};

struct geocache_request_get_capabilities_wmts {
   geocache_request_get_capabilities request;
};

/**
 * the capabilities request for a specific service, to be able to create
 * demo pages specific to a given service
 */
struct geocache_request_get_capabilities_demo {
   geocache_request_get_capabilities request;
   geocache_service *service;
};

struct geocache_request_proxy {
   geocache_request request;
   geocache_http *http;
   apr_table_t *params;
   const char *pathinfo;
};

struct geocache_forwarding_rule {
   char *name;
   geocache_http *http;
   apr_array_header_t *match_params;  /* actually those are geocache_dimensions */
   int append_pathinfo;
};




/** \defgroup services Services*/
/** @{ */

#define GEOCACHE_SERVICES_COUNT 7

typedef enum {
    GEOCACHE_SERVICE_TMS=0, GEOCACHE_SERVICE_WMTS,
    GEOCACHE_SERVICE_DEMO, GEOCACHE_SERVICE_GMAPS, GEOCACHE_SERVICE_KML,
    GEOCACHE_SERVICE_VE, GEOCACHE_SERVICE_WMS
} geocache_service_type;

#define GEOCACHE_UNITS_COUNT 3
typedef enum {
    GEOCACHE_UNIT_METERS=0, GEOCACHE_UNIT_DEGREES, GEOCACHE_UNIT_FEET
} geocache_unit;

/* defined in util.c*/
extern const double geocache_meters_per_unit[GEOCACHE_UNITS_COUNT];

/** \interface geocache_service
 * \brief a standard service (eg WMS, TMS)
 */
struct geocache_service {
    char *name;
    geocache_service_type type;
    
    /**
     * the pathinfo prefix of the url that routes to this service
     * eg, for accessing a wms service on http://host/geocache/mywmsservice? ,
     * url_prefix would take the value "mywmsservice"
     */
    char *url_prefix;
    
    /**
     * \brief allocates and populates a geocache_request corresponding to the parameters received
     */
    void (*parse_request)(geocache_context *ctx, geocache_service *service, geocache_request **request, const char *path_info, apr_table_t *params, geocache_cfg * config);

    /**
     * \param request the received request (should be of type GEOCACHE_REQUEST_CAPABILITIES
     * \param url the full url at which the service is available
     */
    void (*create_capabilities_response)(geocache_context *ctx, geocache_request_get_capabilities *request, char *url, char *path_info, geocache_cfg *config);
    
    /**
     * parse advanced configuration options for the selected service
     */
    void (*configuration_parse_xml)(geocache_context *ctx, ezxml_t xml, geocache_service * service, geocache_cfg *config);
    void (*configuration_parse_json)(geocache_context *ctx, cJSON *node, geocache_service * service, geocache_cfg *config);
};

/**\class geocache_service_wms
 * \brief an OGC WMS service
 * \implements geocache_service
 */
struct geocache_service_wms {
    geocache_service service;
    apr_array_header_t *forwarding_rules;
    geocache_getmap_strategy getmap_strategy;
    geocache_resample_mode resample_mode;
    geocache_image_format *getmap_format;
};

/**\class geocache_service_kml
 * \brief a KML superoverlay service
 * \implements geocache_service
 */
struct geocache_service_kml {
    geocache_service service;
};

/**\class geocache_service_tms
 * \brief a TMS service
 * \implements geocache_service
 */
struct geocache_service_tms {
    geocache_service service;
    int reverse_y;
};

/**\class geocache_service_wmts
 * \brief a WMTS service
 * \implements geocache_service
 */
struct geocache_service_wmts {
    geocache_service service;
};

/**\class geocache_service_demo
 * \brief a demo service
 * \implements geocache_service
 */
struct geocache_service_demo {
    geocache_service service;

};

/**\class geocache_service_ve
 * \brief a virtualearth service
 * \implements geocache_service
 */
struct geocache_service_ve {
    geocache_service service;
};

/**
 * \brief create and initialize a geocache_service_wms
 * \memberof geocache_service_wms
 */
geocache_service* geocache_service_wms_create(geocache_context *ctx);

/**
 * \brief create and initialize a geocache_service_ve
 * \memberof geocache_service_ve
 */
geocache_service* geocache_service_ve_create(geocache_context *ctx);

/**
 * \brief create and initialize a geocache_service_gmaps
 * \memberof geocache_service_gmaps
 */
geocache_service* geocache_service_gmaps_create(geocache_context *ctx);

/**
 * \brief create and initialize a geocache_service_kml
 * \memberof geocache_service_kml
 */
geocache_service* geocache_service_kml_create(geocache_context *ctx);

/**
 * \brief create and initialize a geocache_service_tms
 * \memberof geocache_service_tms
 */
geocache_service* geocache_service_tms_create(geocache_context *ctx);

/**
 * \brief create and initialize a geocache_service_wmts
 * \memberof geocache_service_wtms
 */
geocache_service* geocache_service_wmts_create(geocache_context *ctx);

/**
 * \brief create and initialize a geocache_service_demo
 * \memberof geocache_service_demo
 */
geocache_service* geocache_service_demo_create(geocache_context *ctx);

/**
 * \brief return the request that corresponds to the given url
 */
void geocache_service_dispatch_request(geocache_context *ctx,
      geocache_request **request,
      char *pathinfo,
      apr_table_t *params,
      geocache_cfg *config);


/** @} */

/** \defgroup image Image Data Handling */

/** @{ */

typedef enum {
    GC_UNKNOWN, GC_PNG, GC_JPEG
} geocache_image_format_type;

/**\class geocache_image
 * \brief representation of an RGBA image
 * 
 * to access a pixel at position x,y, you should use the #GET_IMG_PIXEL macro
 */
struct geocache_image {
    unsigned char *data; /**< pointer to the beginning of image data, stored in rgba order */
    size_t w; /**< width of the image */
    size_t h; /**< height of the image */
    size_t stride; /**< stride of an image row */
};

/** \def GET_IMG_PIXEL
 * return the address of a pixel
 * \param y the row
 * \param x the column
 * \param img the geocache_image
 * \returns a pointer to the pixel
 */
#define GET_IMG_PIXEL(img,x,y) (&((img).data[(y)*(img).stride + (x)*4]))


/**
 * \brief initialize a new geocache_image
 */
geocache_image* geocache_image_create(geocache_context *ctx);

/**
 \brief merge a set of tiles into a single image
 \param tiles the list of tiles to merge
 \param ntiles the number of tiles in the list of tiles
 \param format the format to encode the resulting image
 \param ctx the context
 \returns a new tile with the merged image
 */
geocache_tile* geocache_image_merge_tiles(geocache_context *ctx, geocache_image_format *format, geocache_tile **tiles, int ntiles);

void geocache_image_copy_resampled_nearest(geocache_context *ctx, geocache_image *src, geocache_image *dst,
      double off_x, double off_y, double scale_x, double scale_y);
void geocache_image_copy_resampled_bilinear(geocache_context *ctx, geocache_image *src, geocache_image *dst,
      double off_x, double off_y, double scale_x, double scale_y);


/**
 * \brief merge two images
 * \param base the imae to merge onto
 * \param overlay the image to overlay onto
 * \param ctx the context
 * when finished, base will be modified and have overlay merged onto it
 */
void geocache_image_merge(geocache_context *ctx, geocache_image *base, geocache_image *overlay);

void geocache_image_copy_resampled(geocache_context *ctx, geocache_image *src, geocache_image *dst,
      int srcX, int srcY, int srcW, int srcH,
      int dstX, int dstY, int dstW, int dstH);

/**
 * \brief split the given metatile into tiles
 * \param mt the metatile to split
 * \param r the context
 */
void geocache_image_metatile_split(geocache_context *ctx, geocache_metatile *mt);

/**
 * \brief check if given image is composed of a unique color
 * \param image the geocache_image to process
 * \returns GEOCACHE_TRUE if the image contains a single color
 * \returns GEOCACHE_FALSE if the image has more than one color
 */
int geocache_image_blank_color(geocache_image* image);


/**
 * \brief check if image has some non opaque pixels
 */
int geocache_image_has_alpha(geocache_image *img);

/** @} */


/** \defgroup http HTTP Request handling*/
/** @{ */
void geocache_http_do_request(geocache_context *ctx, geocache_http *req, geocache_buffer *data, apr_table_t *headers);
void geocache_http_do_request_with_params(geocache_context *ctx, geocache_http *req, apr_table_t *params,
      geocache_buffer *data, apr_table_t *headers);
char* geocache_http_build_url(geocache_context *ctx, char *base, apr_table_t *params);
apr_table_t *geocache_http_parse_param_string(geocache_context *ctx, char *args);
/** @} */

/** \defgroup configuration Configuration*/

/** @{ */

struct geocache_server_cfg {
   apr_global_mutex_t *mutex;
   char *mutex_name;
   apr_hash_t *aliases; /**< list of geocache configurations aliased to a server uri */
};

/**
 * a configuration that will be served
 */
struct geocache_cfg {
    char *configFile; /**< the filename from which this configuration was loaded */

    /**
     * a list of services that will be responded to
     */
    geocache_service * services[GEOCACHE_SERVICES_COUNT];

    /**
     * hashtable containing configured geocache_source%s
     */
    apr_hash_t *sources;

    /**
     * hashtable containing configured geocache_cache%s
     */
    apr_hash_t *caches;

    /**
     * hashtable containing configured geocache_tileset%s
     */
    apr_hash_t *tilesets;

    /**
     * hashtable containing configured geocache_image_format%s
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
    geocache_image_format *default_image_format;

    /**
     * how should error messages be reported to the user
     */
    geocache_error_reporting reporting;

    /**
     * encoded empty (tranpsarent) image that will be returned to clients if cofigured
     * to return blank images upon error
     */
    geocache_buffer *empty_image;

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
void geocache_configuration_parse(geocache_context *ctx, const char *filename, geocache_cfg *config);
void geocache_configuration_post_config(geocache_context *ctx, geocache_cfg *config);
void geocache_configuration_parse_json(geocache_context *ctx, const char *filename, geocache_cfg *config);
void parse_keyvalues(geocache_context *ctx, cJSON *node, apr_table_t *tbl);
void geocache_configuration_parse_xml(geocache_context *ctx, const char *filename, geocache_cfg *config);
geocache_cfg* geocache_configuration_create(apr_pool_t *pool);
geocache_source* geocache_configuration_get_source(geocache_cfg *config, const char *key);
geocache_cache* geocache_configuration_get_cache(geocache_cfg *config, const char *key);
geocache_grid *geocache_configuration_get_grid(geocache_cfg *config, const char *key);
geocache_tileset* geocache_configuration_get_tileset(geocache_cfg *config, const char *key);
geocache_image_format *geocache_configuration_get_image_format(geocache_cfg *config, const char *key);
void geocache_configuration_add_image_format(geocache_cfg *config, geocache_image_format *format, const char * key);
void geocache_configuration_add_source(geocache_cfg *config, geocache_source *source, const char * key);
void geocache_configuration_add_grid(geocache_cfg *config, geocache_grid *grid, const char * key);
void geocache_configuration_add_tileset(geocache_cfg *config, geocache_tileset *tileset, const char * key);
void geocache_configuration_add_cache(geocache_cfg *config, geocache_cache *cache, const char * key);

/** @} */
/**
 * \memberof geocache_source
 */
void geocache_source_init(geocache_context *ctx, geocache_source *source);

/**
 * \memberof geocache_source_gdal
 */
geocache_source* geocache_source_gdal_create(geocache_context *ctx);

/**
 * \memberof geocache_source_wms
 */
geocache_source* geocache_source_wms_create(geocache_context *ctx);

/**
 * \memberof geocache_cache_disk
 */
geocache_cache* geocache_cache_disk_create(geocache_context *ctx);


/** \defgroup tileset Tilesets*/
/** @{ */

/**
 * \brief Tile
 * \sa geocache_metatile
 * \sa geocache_tileset::metasize_x geocache_tileset::metasize_x geocache_tileset::metabuffer
 */
struct geocache_tile {
    geocache_tileset *tileset; /**< the geocache_tileset that corresponds to the tile*/
    geocache_grid_link *grid_link;
    int x; /**< tile x index */
    int y; /**< tile y index */
    int z; /**< tile z index (zoom level) */
    /**
     * encoded image data for the tile.
     * \sa geocache_cache::tile_get()
     * \sa geocache_source::render_map()
     * \sa geocache_image_format
     */
    geocache_buffer *data;
    apr_time_t mtime; /**< last modification time */
    int expires; /**< time in seconds after which the tile should be rechecked for validity */
    
    apr_table_t *dimensions;
};

/**
 * \brief  MetaTile
 * \extends geocache_tile
 */
struct geocache_metatile {
   geocache_map map;
   int x,y,z;
   int metasize_x, metasize_y;
   int ntiles; /**< the number of geocache_metatile::tiles contained in this metatile */
   geocache_tile *tiles; /**< the list of geocache_tile s contained in this metatile */
   void *lock; /**< pointer to opaque structure set by the locking mechanism */
};


struct geocache_grid_level {
   double resolution;
   unsigned int maxx, maxy;
};

struct geocache_grid {
   char *name;
   int nlevels;
   char *srs;
   apr_array_header_t *srs_aliases;
   double extent[4];
   geocache_unit unit;
   int tile_sx, tile_sy; /**<width and height of a tile in pixels */
   geocache_grid_level **levels;
   apr_table_t *metadata;
};


struct geocache_grid_link {
   geocache_grid *grid;
   /**
    * precalculated limits for available each level: [minTileX, minTileY, maxTileX, maxTileY].
    *
    * a request is valid if x is in [minTileX, maxTileX[ and y in [minTileY,maxTileY]
    */
   double *restricted_extent;
   int **grid_limits;
};

/**\class geocache_tileset
 * \brief a set of tiles that can be requested by a client, created from a geocache_source
 *        stored by a geocache_cache in a geocache_format
 */
struct geocache_tileset {
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
     * size of the metatile that should be requested to the geocache_tileset::source
     */
    int metasize_x, metasize_y;

    /**
     * size of the gutter around the metatile that should be requested to the geocache_tileset::source
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
    geocache_cache *cache;

    /**
     * the source from which tiles should be requested
     */
    geocache_source *source;

    /**
     * the format to use when storing tiles coming from a metatile
     */
    geocache_image_format *format;

    /**
     * a list of parameters that can be forwarded from the client to the geocache_tileset::source
     */
    apr_array_header_t *dimensions;
    
    /**
     * image to be used as a watermark
     */
    geocache_image *watermark;

    /**
     * handle to the configuration this tileset belongs to
     */
    geocache_cfg *config;
   
    /**
     * should we service wms requests not aligned to a grid
     */
    int full_wms;

    apr_table_t *metadata;
};

void geocache_tileset_get_map_tiles(geocache_context *ctx, geocache_tileset *tileset,
      geocache_grid_link *grid_link,
      double *bbox, int width, int height,
      int *ntiles,
      geocache_tile ***tiles);

geocache_image* geocache_tileset_assemble_map_tiles(geocache_context *ctx, geocache_tileset *tileset,
      geocache_grid_link *grid_link,
      double *bbox, int width, int height,
      int ntiles,
      geocache_tile **tiles,
      geocache_resample_mode mode);

/**
 * compute x,y,z value given a bbox.
 * will return GEOCACHE_FAILURE
 * if the bbox does not correspond to the tileset's configuration
 */
int geocache_grid_get_cell(geocache_context *ctx, geocache_grid *grid, double *bbox,
      int *x, int *y, int *z);

/**
 * \brief verify the created tile respects configured constraints
 * @param tile
 * @param r
 * @return
 */
void geocache_tileset_tile_validate(geocache_context *ctx, geocache_tile *tile);

/**
 * compute level for a given resolution
 * 
 * computes the integer level for the given resolution. the input resolution will be set to the exact
 * value configured for the tileset, to compensate for rounding errors that could creep in if using
 * the resolution calculated from input parameters
 * 
 * \returns GEOCACHE_TILESET_WRONG_RESOLUTION if the given resolution is't configured
 * \returns GEOCACHE_SUCCESS if the level was found
 */
void geocache_tileset_get_level(geocache_context *ctx, geocache_tileset *tileset, double *resolution, int *level);

void geocache_grid_get_closest_level(geocache_context *ctx, geocache_grid *grid, double resolution, int *level);
void geocache_tileset_tile_get(geocache_context *ctx, geocache_tile *tile);

/**
 * \brief delete tile from cache
 * @param whole_metatile delete all the other tiles from the metatile to
 */
void geocache_tileset_tile_delete(geocache_context *ctx, geocache_tile *tile, int whole_metatile);

int geocache_grid_is_bbox_aligned(geocache_context *ctx, geocache_grid *grid, double *bbox);

/**
 * \brief create and initialize a tile for the given tileset and grid_link
 * @param tileset
 * @param grid_link
 * @param pool
 * @return
 */
geocache_tile* geocache_tileset_tile_create(apr_pool_t *pool, geocache_tileset *tileset, geocache_grid_link *grid_link);

/**
 * \brief create and initialize a map for the given tileset and grid_link
 * @param tileset
 * @param grid_link
 * @param pool
 * @return
 */
geocache_map* geocache_tileset_map_create(apr_pool_t *pool, geocache_tileset *tileset, geocache_grid_link *grid_link);


/**
 * \brief create and initialize a feature_info for the given tileset and grid_link
 */
geocache_feature_info* geocache_tileset_feature_info_create(apr_pool_t *pool, geocache_tileset *tileset,
      geocache_grid_link *grid_link);

/**
 * \brief create and initalize a tileset
 * @param pool
 * @return
 */
geocache_tileset* geocache_tileset_create(geocache_context *ctx);

void geocache_tileset_configuration_check(geocache_context *ctx, geocache_tileset *tileset);
void geocache_tileset_add_watermark(geocache_context *ctx, geocache_tileset *tileset, const char *filename);


/**
 * lock the tile
 */
void geocache_tileset_metatile_lock(geocache_context *ctx, geocache_metatile *tile);

geocache_metatile* geocache_tileset_metatile_get(geocache_context *ctx, geocache_tile *tile);

/**
 * unlock the tile
 */
void geocache_tileset_metatile_unlock(geocache_context *ctx, geocache_metatile *tile);

/**
 * check if there is a lock on the tile (the lock will have been set by another process/thread)
 * \returns GEOCACHE_TRUE if the tile is locked by another process/thread
 * \returns GEOCACHE_FALSE if there is no lock on the tile
 */
int geocache_tileset_metatile_lock_exists(geocache_context *ctx, geocache_metatile *tile);

/**
 * wait for the lock set on the tile (the lock will have been set by another process/thread)
 */
void geocache_tileset_metatile_lock_wait(geocache_context *ctx, geocache_metatile *tile);


/** @} */



geocache_tile *geocache_core_get_tile(geocache_context *ctx, geocache_request_get_tile *req_tile);

geocache_map *geocache_core_get_map(geocache_context *ctx, geocache_request_get_map *req_map);

geocache_feature_info *geocache_core_get_featureinfo(geocache_context *ctx, geocache_request_get_feature_info *req_fi);

geocache_proxied_response *geocache_core_proxy_request(geocache_context *ctx, geocache_request_proxy *req_proxy);


/* in grid.c */
geocache_grid* geocache_grid_create(apr_pool_t *pool);

const char* geocache_grid_get_crs(geocache_context *ctx, geocache_grid *grid);
const char* geocache_grid_get_srs(geocache_context *ctx, geocache_grid *grid);

void geocache_grid_get_extent(geocache_context *ctx, geocache_grid *grid,
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
void geocache_grid_get_xy(geocache_context *ctx, geocache_grid *grid, double dx, double dy, int z, int *x, int *y);

double geocache_grid_get_resolution(double *bbox, int sx, int sy);
double geocache_grid_get_horizontal_resolution(double *bbox, int width);
double geocache_grid_get_vertical_resolution(double *bbox, int height);

int geocache_grid_get_level(geocache_context *ctx, geocache_grid *grid, double *resolution, int *level);
void geocache_grid_compute_limits(const geocache_grid *grid, const double *extent, int **limits);

/* in util.c */
int geocache_util_extract_int_list(geocache_context *ctx, const char* args, const char *sep, int **numbers,
        int *numbers_count);
int geocache_util_extract_double_list(geocache_context *ctx, const char* args, const char *sep, double **numbers,
        int *numbers_count);
char *geocache_util_str_replace(apr_pool_t *pool, const char *string, const char *substr,
      const char *replacement );

/*
int geocache_util_mutex_aquire(geocache_context *r);
int geocache_util_mutex_release(geocache_context *r);
 */



/**\defgroup imageio Image IO */
/** @{ */

/**
 * compression strategy to apply
 */
typedef enum {
    GEOCACHE_COMPRESSION_BEST, /**< best but slowest compression*/
    GEOCACHE_COMPRESSION_FAST, /**< fast compression*/
    GEOCACHE_COMPRESSION_DEFAULT /**< default compression*/
} geocache_compression_type;

/**\interface geocache_image_format
 * \brief an image format
 * \sa geocache_image_format_jpeg
 * \sa geocache_image_format_png
 */
struct geocache_image_format {
    char *name; /**< the key by which this format will be referenced */
    char *extension; /**< the extension to use when saving a file with this format */
    char *mime_type;
    geocache_buffer * (*write)(geocache_context *ctx, geocache_image *image, geocache_image_format * format);
    /**< pointer to a function that returns a geocache_buffer containing the given image encoded
     * in the specified format
     */

    geocache_buffer* (*create_empty_image)(geocache_context *ctx, geocache_image_format *format,
          size_t width, size_t height, unsigned int color);
    apr_table_t *metadata;
};

/**\defgroup imageio_png PNG Image IO
 * \ingroup imageio */
/** @{ */

/**\class geocache_image_format_png
 * \brief PNG image format
 * \extends geocache_image_format
 * \sa geocache_image_format_png_q
 */
struct geocache_image_format_png {
    geocache_image_format format;
    geocache_compression_type compression_level; /**< PNG compression level to apply */
};

struct geocache_image_format_mixed {
   geocache_image_format format;
   geocache_image_format *transparent;
   geocache_image_format *opaque;
};


geocache_image_format* geocache_imageio_create_mixed_format(apr_pool_t *pool,
      char *name, geocache_image_format *transparent, geocache_image_format *opaque);

/**\class geocache_image_format_png_q
 * \brief Quantized PNG format
 * \extends geocache_image_format_png
 */
struct geocache_image_format_png_q {
    geocache_image_format_png format;
    int ncolors; /**< number of colors used in quantization, 2-256 */
};

/**
 * @param r
 * @param buffer
 * @return
 */
geocache_image* _geocache_imageio_png_decode(geocache_context *ctx, geocache_buffer *buffer);

void geocache_image_create_empty(geocache_context *ctx, geocache_cfg *cfg);
/**
 * @param r
 * @param buffer
 * @return
 */
void _geocache_imageio_png_decode_to_image(geocache_context *ctx, geocache_buffer *buffer,
      geocache_image *image);


/**
 * \brief create a format capable of creating RGBA png
 * \memberof geocache_image_format_png
 * @param pool
 * @param name
 * @param compression the ZLIB compression to apply
 * @return
 */
geocache_image_format* geocache_imageio_create_png_format(apr_pool_t *pool, char *name, geocache_compression_type compression);

/**
 * \brief create a format capable of creating quantized png
 * \memberof geocache_image_format_png_q
 * @param pool
 * @param name
 * @param compression the ZLIB compression to apply
 * @param ncolors the number of colors to quantize with
 * @return
 */
geocache_image_format* geocache_imageio_create_png_q_format(apr_pool_t *pool, char *name, geocache_compression_type compression, int ncolors);

/** @} */

/**\defgroup imageio_jpg JPEG Image IO
 * \ingroup imageio */
/** @{ */

/**\class geocache_image_format_jpeg
 * \brief JPEG image format
 * \extends geocache_image_format
 */
struct geocache_image_format_jpeg {
    geocache_image_format format;
    int quality; /**< JPEG quality, 1-100 */
};

geocache_image_format* geocache_imageio_create_jpeg_format(apr_pool_t *pool, char *name, int quality);

/**
 * @param r
 * @param buffer
 * @return
 */
geocache_image* _geocache_imageio_jpeg_decode(geocache_context *ctx, geocache_buffer *buffer);

/**
 * @param r
 * @param buffer
 * @return
 */
void _geocache_imageio_jpeg_decode_to_image(geocache_context *ctx, geocache_buffer *buffer,
      geocache_image *image);

/** @} */

/**
 * \brief lookup the first few bytes of a buffer to check for a known image format
 */
geocache_image_format_type geocache_imageio_header_sniff(geocache_context *ctx, geocache_buffer *buffer);

/**
 * \brief checks if the given buffer is a recognized image format
 */
int geocache_imageio_is_valid_format(geocache_context *ctx, geocache_buffer *buffer);


/**
 * decodes given buffer
 */
geocache_image* geocache_imageio_decode(geocache_context *ctx, geocache_buffer *buffer);

/**
 * decodes given buffer to an allocated image
 */
void geocache_imageio_decode_to_image(geocache_context *ctx, geocache_buffer *buffer, geocache_image *image);


/** @} */

typedef struct {
   double start;
   double end;
   double resolution;
} geocache_interval;

typedef enum {
   GEOCACHE_DIMENSION_VALUES,
   GEOCACHE_DIMENSION_REGEX,
   GEOCACHE_DIMENSION_INTERVALS,
   GEOCACHE_DIMENSION_TIME
} geocache_dimension_type;

struct geocache_dimension {
   geocache_dimension_type type;
   char *name;
   char *unit;
   apr_table_t *metadata;
   char *default_value;
   
   /**
    * \brief validate the given value
    * 
    * \param value is updated in case the given value is correct but has to be represented otherwise,
    * e.g. to round off a value
    * \returns GEOCACHE_SUCCESS if the given value is correct for the current dimension
    * \returns GEOCACHE_FAILURE if not
    */
   int (*validate)(geocache_context *context, geocache_dimension *dimension, char **value);
   
   /**
    * \brief returns a list of values that are authorized for this dimension
    *
    * \returns a list of character strings that will be included in the capabilities <dimension> element
    */
   const char** (*print_ogc_formatted_values)(geocache_context *context, geocache_dimension *dimension);
   
   /**
    * \brief parse the value given in the configuration
    */
   void (*configuration_parse_xml)(geocache_context *context, geocache_dimension *dim, ezxml_t node);
   void (*configuration_parse_json)(geocache_context *context, geocache_dimension *dim, cJSON *node);
};

struct geocache_dimension_values {
   geocache_dimension dimension;
   int nvalues;
   char **values;
   int case_sensitive;
};

struct geocache_dimension_regex {
   geocache_dimension dimension;
   char *regex_string;
#ifdef USE_PCRE
   pcre *pcregex;
#else
   regex_t *regex;
#endif
};

struct geocache_dimension_intervals {
   geocache_dimension dimension;
   int nintervals;
   geocache_interval *intervals;
};

struct geocache_dimension_time {
   geocache_dimension dimension;
   int nintervals;
   geocache_interval *intervals;
};

geocache_dimension* geocache_dimension_values_create(apr_pool_t *pool);
geocache_dimension* geocache_dimension_regex_create(apr_pool_t *pool);
geocache_dimension* geocache_dimension_intervals_create(apr_pool_t *pool);
geocache_dimension* geocache_dimension_time_create(apr_pool_t *pool);


int geocache_is_axis_inverted(const char *srs);

#endif /* GEOCACHE_H_ */
/* vim: ai ts=3 sts=3 et sw=3
*/

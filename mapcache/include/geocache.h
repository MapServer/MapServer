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
#include "errors.h"
#ifdef USE_GDAL
#include <gdal.h>
#include <cpl_conv.h>
#endif
#include <assert.h>
#include <apr_time.h>
#include <regex.h>

#define GEOCACHE_SUCCESS 0
#define GEOCACHE_FAILURE 1
#define GEOCACHE_TRUE 1
#define GEOCACHE_FALSE 0
#define GEOCACHE_TILESET_WRONG_SIZE 2
#define GEOCACHE_TILESET_WRONG_RESOLUTION 3
#define GEOCACHE_TILESET_WRONG_EXTENT 4
#define GEOCACHE_CACHE_MISS 5
#define GEOCACHE_FILE_LOCKED 6




typedef struct geocache_image_format geocache_image_format;
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
typedef struct geocache_source_wms geocache_source_wms;
typedef struct geocache_source_gdal geocache_source_gdal;
typedef struct geocache_cache_disk geocache_cache_disk;
typedef struct geocache_request geocache_request;
typedef struct geocache_request_get_capabilities geocache_request_get_capabilities;
typedef struct geocache_request_get_capabilities_wms geocache_request_get_capabilities_wms;
typedef struct geocache_request_get_capabilities_wmts geocache_request_get_capabilities_wmts;
typedef struct geocache_request_get_capabilities_tms geocache_request_get_capabilities_tms;

typedef struct geocache_request_get_tile geocache_request_get_tile;
typedef struct geocache_service geocache_service;
typedef struct geocache_service_wms geocache_service_wms;
typedef struct geocache_service_wmts geocache_service_wmts;
typedef struct geocache_service_tms geocache_service_tms;
typedef struct geocache_service_demo geocache_service_demo;
typedef struct geocache_server_cfg geocache_server_cfg;
typedef struct geocache_image geocache_image;
typedef struct geocache_grid geocache_grid;
typedef struct geocache_grid_level geocache_grid_level;
typedef struct geocache_grid_link geocache_grid_link;
typedef struct geocache_context geocache_context;
typedef struct geocache_dimension geocache_dimension;

/** \defgroup utility Utility */
/** @{ */

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
    /**
     * \brief get the data for the metatile
     *
     * sets the geocache_metatile::tile::data for the given tile
     */
    void (*render_metatile)(geocache_context *ctx, geocache_metatile * mt);
    void (*configuration_parse)(geocache_context *ctx, ezxml_t xml, geocache_source * source);
    void (*configuration_check)(geocache_context *ctx, geocache_source * source);
};

/**\class geocache_source_wms
 * \brief WMS geocache_source
 * \implements geocache_source
 */
struct geocache_source_wms {
    geocache_source source;
    char *url; /**< the base WMS url */
    apr_table_t *wms_default_params; /**< default WMS parameters (SERVICE,REQUEST,STYLES,VERSION) */
    apr_table_t *wms_params; /**< WMS parameters specified in configuration */
};

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


/** \defgroup cache Caches */

/** @{ */
typedef enum {
    GEOCACHE_CACHE_DISK
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

    int (*tile_exists)(geocache_context *ctx, geocache_tile * tile);

    /**
     * set tile content to cache
     * \memberof geocache_cache
     */
    void (*tile_set)(geocache_context *ctx, geocache_tile * tile);

    void (*configuration_parse)(geocache_context *ctx, ezxml_t xml, geocache_cache * cache);
    void (*configuration_check)(geocache_context *ctx, geocache_cache * cache);
};

/**\class geocache_cache_disk
 * \brief a geocache_cache on a filesytem
 * \implements geocache_cache
 */
struct geocache_cache_disk {
    geocache_cache cache;
    char *base_directory;
    int symlink_blank;
};

/** @} */


typedef enum {
   GEOCACHE_REQUEST_UNKNOWN,
   GEOCACHE_REQUEST_GET_TILE,
   GEOCACHE_REQUEST_GET_MAP,
   GEOCACHE_REQUEST_GET_CAPABILITIES,
} geocache_request_type;
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

struct geocache_request_get_capabilities_wms {
   geocache_request_get_capabilities request;
   char *version;
};

struct geocache_request_get_capabilities_wmts {
   geocache_request_get_capabilities request;
   char *version;
};



/** \defgroup services Services*/
/** @{ */

#define GEOCACHE_SERVICES_COUNT 4

typedef enum {
    GEOCACHE_SERVICE_WMS = 0, GEOCACHE_SERVICE_TMS, GEOCACHE_SERVICE_WMTS,
    GEOCACHE_SERVICE_DEMO
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
    void (*parse_request)(geocache_context *ctx, geocache_request **request, const char *path_info, apr_table_t *params, geocache_cfg * config);

    /**
     * \param request the received request (should be of type GEOCACHE_REQUEST_CAPABILITIES
     * \param url the full url at which the service is available
     */
    void (*create_capabilities_response)(geocache_context *ctx, geocache_request_get_capabilities *request, char *url, char *path_info, geocache_cfg *config);
};

/**\class geocache_service_wms
 * \brief an OGC WMS service
 * \implements geocache_service
 */
struct geocache_service_wms {
    geocache_service service;
};

/**\class geocache_service_tms
 * \brief a TMS service
 * \implements geocache_service
 */
struct geocache_service_tms {
    geocache_service service;
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

/**
 * \brief create and initialize a geocache_service_wms
 * \memberof geocache_service_wms
 */
geocache_service* geocache_service_wms_create(geocache_context *ctx);

/**
 * \brief create and initialize a geocache_service_tms
 * \memberof geocache_service_tms
 */
geocache_service* geocache_service_tms_create(geocache_context *ctx);

/**
 * \brief create and initialize a geocache_service_wtms
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

/**
 * \brief merge two images
 * \param base the imae to merge onto
 * \param overlay the image to overlay onto
 * \param ctx the context
 * when finished, base will be modified and have overlay merged onto it
 */
void geocache_image_merge(geocache_context *ctx, geocache_image *base, geocache_image *overlay);

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
/** @} */


/** \defgroup http HTTP Request handling*/
/** @{ */
void geocache_http_request_url(geocache_context *ctx, char *url, geocache_buffer *data);
void geocache_http_request_url_with_params(geocache_context *ctx, char *url, apr_table_t *params, geocache_buffer *data);
char* geocache_http_build_url(geocache_context *ctx, char *base, apr_table_t *params);
apr_table_t *geocache_http_parse_param_string(geocache_context *ctx, char *args);
/** @} */

/** \defgroup configuration Configuration*/

/** @{ */

struct geocache_server_cfg {
    apr_global_mutex_t *mutex;
    char *mutex_name;
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
     * the format to use when merging multiple tiles
     */
    geocache_image_format *merge_format;

    /**
      * how should error messages be reported to the user
      */
     geocache_error_reporting reporting;
     
     apr_table_t *metadata;
};

/**
 *
 * @param filename
 * @param config
 * @param pool
 * @return
 */
void geocache_configuration_parse(geocache_context *ctx, const char *filename, geocache_cfg *config);
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
     * \sa geocache_source::render_tile()
     * \sa geocache_source::render_metatile()
     * \sa geocache_image_format
     */
    geocache_buffer *data;
    void *lock; /**< pointer to opaque structure set by a geocache_cache to reference a lock on the tile
                \sa geocache_cache::tile_lock() */
    apr_time_t mtime; /**< last modification time */
    int expires; /**< time in seconds after which the tile should be rechecked for validity */
    
    apr_table_t *dimensions;
};

/**
 * \brief  MetaTile
 * \extends geocache_tile
 */
struct geocache_metatile {
    geocache_tile tile; /**< the geocache_tile that corresponds to this metatile */
    int sx; /**< metatile width */
    int sy; /**< metatile height */
    double bbox[4]; /**< the bounding box covered by this metatile */
    int ntiles; /**< the number of geocache_metatile::tiles contained in this metatile */
    geocache_tile *tiles; /**< the list of geocache_tile s contained in this metatile */
    geocache_image *imdata;
};


struct geocache_grid_level {
   double resolution;
   unsigned int maxx, maxy;
};

struct geocache_grid {
   char *name;
   int nlevels;
   char *srs;
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
     * the extent of the tileset that can be cached
     */
    double restricted_extent[4];

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
     */
    int expires;

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
     * the format to use when storing a tile coming from a source
     * if NULL, store what the source has produced without processing
     */
    geocache_image_format *cache_format;

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
    
    apr_table_t *metadata;
};

/**
 * \brief compute a tile's x,y and z value given a BBOX.
 * @param tile
 * @param bbox 
 * @param r
 * @return
 */
void geocache_tileset_tile_lookup(geocache_context *ctx, geocache_tile *tile, double *bbox);

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


void geocache_tileset_tile_get(geocache_context *ctx, geocache_tile *tile);

/**
 * \brief create and initialize a tile for the given tileset
 * @param tileset
 * @param pool
 * @return
 */
geocache_tile* geocache_tileset_tile_create(apr_pool_t *pool, geocache_tileset *tileset);

/**
 * \brief create and initalize a tileset
 * @param pool
 * @return
 */
geocache_tileset* geocache_tileset_create(geocache_context *ctx);

/**
 * \brief compute the bounding box of a tile
 * @param tile the input tile
 * @param bbox the bounding box to populate
 */
void geocache_tileset_tile_bbox(geocache_tile *tile, double *bbox);



/**
 * lock the tile
 */
void geocache_tileset_tile_lock(geocache_context *ctx, geocache_tile *tile);

/**
 * unlock the tile
 */
void geocache_tileset_tile_unlock(geocache_context *ctx, geocache_tile *tile);

/**
 * check if there is a lock on the tile (the lock will have been set by another process/thread)
 * \returns GEOCACHE_TRUE if the tile is locked by another process/thread
 * \returns GEOCACHE_FALSE if there is no lock on the tile
 */
int geocache_tileset_tile_lock_exists(geocache_context *ctx, geocache_tile *tile);

/**
 * wait for the lock set on the tile (the lock will have been set by another process/thread)
 */
void geocache_tileset_tile_lock_wait(geocache_context *ctx, geocache_tile *tile);


/** @} */


/* in grid.c */
geocache_grid* geocache_grid_create(apr_pool_t *pool);

const char* geocache_grid_get_crs(geocache_context *ctx, geocache_grid *grid);
const char* geocache_grid_get_srs(geocache_context *ctx, geocache_grid *grid);

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

double geocache_grid_get_resolution(geocache_grid *grid, double *bbox);
void geocache_grid_get_level(geocache_context *ctx, geocache_grid *grid, double *resolution, int *level);
void geocache_grid_compute_limits(const geocache_grid *grid, const double *extent, int **limits);

/* in util.c */
int geocache_util_extract_int_list(geocache_context *ctx, const char* args, const char sep, int **numbers,
        int *numbers_count);
int geocache_util_extract_double_list(geocache_context *ctx, const char* args, const char sep, double **numbers,
        int *numbers_count);

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
 * \brief check if image has some non opaque pixels
 */
int geocache_imageio_image_has_alpha(geocache_image *img);

/**
 * decodes given buffer
 */
geocache_image* geocache_imageio_decode(geocache_context *ctx, geocache_buffer *buffer);


/** @} */

struct geocache_dimension {
   char *name;
   char *unit;
   apr_table_t *metadata;
   char *default_value;
   int nvalues;
   char **values;
   int (*validate)(geocache_context *context, geocache_dimension *dimension, const char *value);
};

geocache_dimension* geocache_dimension_create(apr_pool_t *pool);

#endif /* GEOCACHE_H_ */
/* vim: ai ts=3 sts=3 et sw=3
*/

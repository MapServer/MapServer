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

#ifndef GEOCACHE_H_
#define GEOCACHE_H_

#include <apr_tables.h>
#include <apr_hash.h>
#include <apr_global_mutex.h>
#include <httpd.h>
#include <http_config.h>
#include <libxml/tree.h>
#include "util.h"
#include <assert.h>


#define GEOCACHE_SUCCESS 0
#define GEOCACHE_FAILURE 1
#define GEOCACHE_TRUE 1
#define GEOCACHE_FALSE 0
#define GEOCACHE_TILESET_WRONG_SIZE 2
#define GEOCACHE_TILESET_WRONG_RESOLUTION 3
#define GEOCACHE_TILESET_WRONG_EXTENT 4
#define GEOCACHE_CACHE_MISS 5
#define GEOCACHE_FILE_LOCKED 6

#define GEOCACHE_SERVICES_COUNT 2
typedef enum {GEOCACHE_SERVICE_WMS=0, GEOCACHE_SERVICE_TMS} geocache_service_type;

typedef enum {GEOCACHE_SOURCE_WMS} geocache_source_type;
typedef enum {GEOCACHE_IMAGE_FORMAT_UNKNOWN, GEOCACHE_IMAGE_FORMAT_PNG, GEOCACHE_IMAGE_FORMAT_JPEG} geocache_image_format_type;

typedef struct geocache_cfg geocache_cfg;
typedef struct geocache_tileset geocache_tileset;
typedef struct geocache_cache geocache_cache;
typedef struct geocache_source geocache_source;

extern module AP_MODULE_DECLARE_DATA geocache_module;

typedef struct {
   unsigned char* buf ;     /* buffer */
   size_t size ; /* bytes used */
   size_t avail ;  /* bytes allocated */
   apr_pool_t* pool; /*apache pool to allocate from */
} geocache_buffer;

typedef struct {
   geocache_tileset *tileset;
   int x,y,z;
   int sx,sy;
   geocache_buffer *data;
   void *lock;
} geocache_tile;

typedef struct {
   geocache_tile tile;
   double bbox[4];
   int ntiles;
   geocache_tile *tiles;
} geocache_metatile;


struct geocache_source{
   char *name;
   char *srs;
   double data_extent[4];
   geocache_image_format_type image_format;
   geocache_source_type type;
   int supports_metatiling;
   int (*render_tile)(geocache_tile *tile, request_rec *r);
   int (*render_metatile)(geocache_metatile *mt, request_rec *r);
   char* (*configuration_parse)(xmlNode *xml, geocache_source *source, apr_pool_t *pool);
   char* (*configuration_check)(geocache_source *source, apr_pool_t *pool);      
};



typedef struct {
  geocache_source source;
  char *url;
  apr_table_t *wms_default_params;
  apr_table_t *wms_params;
} geocache_wms_source;


typedef enum {GEOCACHE_CACHE_DISK} geocache_cache_type;

struct geocache_cache{
   char *name;
   geocache_cache_type type;
   int (*tile_get)(geocache_tile *tile, request_rec *r);
   int (*tile_set)(geocache_tile *tile, request_rec *r);
   char* (*configuration_parse)(xmlNode *xml, geocache_cache *cache, apr_pool_t *pool);
   char* (*configuration_check)(geocache_cache *cache, apr_pool_t *pool);  
   int (*tile_lock)(geocache_tile *tile, request_rec *r);
   int (*tile_unlock)(geocache_tile *tile, request_rec *r);
   int (*tile_lock_exists)(geocache_tile *tile, request_rec *r);
   int (*tile_lock_wait)(geocache_tile *tile, request_rec *r);
};

typedef struct {
   geocache_cache cache;
   char *base_directory;
} geocache_cache_disk;

typedef struct {
   geocache_tile **tiles;
   int ntiles;
} geocache_request;

typedef struct {
   geocache_service_type type;
   geocache_request* (*parse_request)(request_rec *r, apr_table_t *params, geocache_cfg *config);
} geocache_service;

typedef struct {
   geocache_service service;
} geocache_service_wms;

typedef struct {
   geocache_service service;
} geocache_service_tms;


struct geocache_tileset {
   char *name;
   double extent[4];
   char *srs;
   int tile_sx, tile_sy;
   int levels;
   double *resolutions;
   int metasize_x, metasize_y;
   int metabuffer;
   geocache_image_format_type format;
   geocache_cache *cache;
   geocache_source *source;
   apr_table_t *forwarded_params;
};

struct geocache_cfg {
   char *configFile;
   geocache_service* services[GEOCACHE_SERVICES_COUNT];
   apr_hash_t *sources;
   apr_hash_t *caches;
   apr_hash_t *tilesets;
};

typedef struct {
   apr_global_mutex_t *mutex;
} geocache_server_cfg;

typedef struct {
   unsigned char *data;
   size_t w,h;
   size_t stride;
} geocache_image;




/* in buffer.c */
geocache_buffer *geocache_buffer_create(size_t initialStorage, apr_pool_t* pool);
int geocache_buffer_append(geocache_buffer *buffer, size_t len, void *data);

/* in http.c */
int geocache_http_request_url(request_rec *r, char *url, geocache_buffer *data);
int geocache_http_request_url_with_params(request_rec *r, char *url, apr_table_t *params, geocache_buffer *data);
char* geocache_http_build_url(request_rec *r, char *base, apr_table_t *params);
apr_table_t *geocache_http_parse_param_string(request_rec *r, char *args);

/* functions exported in configuration.c */
char* geocache_configuration_parse(const char *filename, geocache_cfg *config, apr_pool_t *pool);
geocache_cfg* geocache_configuration_create(apr_pool_t *pool);
geocache_source* geocache_configuration_get_source(geocache_cfg *config, const char *key);
geocache_cache* geocache_configuration_get_cache(geocache_cfg *config, const char *key);
geocache_tileset* geocache_configuration_get_tileset(geocache_cfg *config, const char *key);
void geocache_configuration_add_source(geocache_cfg *config, geocache_source *source, const char * key);
void geocache_configuration_add_tileset(geocache_cfg *config, geocache_tileset *tileset, const char * key);
void geocache_configuration_add_cache(geocache_cfg *config, geocache_cache *cache, const char * key);

/* functions exported in source.c */
void geocache_source_init(geocache_source *source, apr_pool_t *pool);

/* functions exported in wms_source.c */
geocache_wms_source* geocache_source_wms_create(apr_pool_t *pool);

/* functions exported in disk_cache.c */
geocache_cache_disk* geocache_cache_disk_create(apr_pool_t *pool);


/* functions exported in tileset.c */
int geocache_tileset_tile_lookup(geocache_tile *tile, double *bbox, request_rec *r);
int geocache_tileset_tile_get(geocache_tile *tile, request_rec *r);
geocache_tile* geocache_tileset_tile_create(geocache_tileset *tileset, apr_pool_t *pool);
geocache_tileset* geocache_tileset_create(apr_pool_t *pool);
void geocache_tileset_tile_bbox(geocache_tile *tile, double *bbox);



/* in service.c */
geocache_service* geocache_service_wms_create(apr_pool_t *pool);
geocache_service* geocache_service_tms_create(apr_pool_t *pool);

/* in util.c */
int geocache_util_extract_int_list(char* args, const char sep, int **numbers,
      int *numbers_count, apr_pool_t *pool);
int geocache_util_extract_double_list(char* args, const char sep, double **numbers,
      int *numbers_count, apr_pool_t *pool);
int geocache_util_mutex_aquire(request_rec *r);
int geocache_util_mutex_release(request_rec *r);

typedef struct {
   geocache_buffer *buffer;
   unsigned char *ptr;
} _geocache_buffer_closure;

/* in image.c */
geocache_tile* geocache_image_merge_tiles(request_rec *r, geocache_tile **tiles, int ntiles);
int geocache_image_metatile_split(geocache_metatile *mt, request_rec *r);

/* in imageio.c */
geocache_buffer* geocache_imageio_encode(request_rec *r, geocache_image *img, geocache_image_format_type format);
geocache_image_format_type geocache_imageio_header_sniff(request_rec *r, geocache_buffer *buffer);
geocache_image* geocache_imageio_decode(request_rec *r, geocache_buffer *buffer);

#endif /* GEOCACHE_H_ */

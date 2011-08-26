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

#ifndef YATC_H_
#define YATC_H_

#include <apr_tables.h>
#include <apr_hash.h>
#include <apr_global_mutex.h>
#include <httpd.h>
#include <http_config.h>
#include <libxml/tree.h>
#include "util.h"
#include <assert.h>


#define YATC_SUCCESS 0
#define YATC_FAILURE 1
#define YATC_TILESET_WRONG_SIZE 2
#define YATC_TILESET_WRONG_RESOLUTION 3
#define YATC_TILESET_WRONG_EXTENT 4
#define YATC_CACHE_MISS 5
#define YATC_FILE_EXISTS 6

#define YATC_SERVICES_COUNT 2
typedef enum {YATC_SERVICE_WMS=0, YATC_SERVICE_TMS} yatc_service_type;

typedef enum {YATC_SOURCE_WMS} yatc_source_type;
typedef enum {YATC_IMAGE_FORMAT_UNSPECIFIED, YATC_IMAGE_FORMAT_PNG, YATC_IMAGE_FORMAT_JPEG} yatc_image_format_type;

typedef struct yatc_cfg yatc_cfg;
typedef struct yatc_tileset yatc_tileset;
typedef struct yatc_cache yatc_cache;
typedef struct yatc_source yatc_source;

module AP_MODULE_DECLARE_DATA yatc_module;

typedef struct {
   char* buf ;     /* buffer */
   size_t size ; /* bytes used */
   size_t avail ;  /* bytes allocated */
   apr_pool_t* pool; /*apache pool to allocate from */
} yatc_buffer;

typedef struct {
   yatc_tileset *tileset;
   int x,y,z;
   int sx,sy;
   yatc_buffer *data;
   void *lock;
} yatc_tile;

typedef struct {
   yatc_tile tile;
   double bbox[4];
   int ntiles;
   yatc_tile *tiles;
} yatc_metatile;


struct yatc_source{
   char *name;
   char *srs;
   double data_extent[4];
   yatc_image_format_type image_format;
   yatc_source_type type;
   int supports_metatiling;
   int (*render_tile)(yatc_tile *tile, request_rec *r);
   int (*render_metatile)(yatc_metatile *mt, request_rec *r);
   char* (*configuration_parse)(xmlNode *xml, yatc_source *source, apr_pool_t *pool);
   char* (*configuration_check)(yatc_source *source, apr_pool_t *pool);      
};



typedef struct {
  yatc_source source;
  char *url;
  apr_table_t *wms_default_params;
  apr_table_t *wms_params;
} yatc_wms_source;


typedef enum {YATC_CACHE_DISK} yatc_cache_type;

struct yatc_cache{
   char *name;
   yatc_cache_type type;
   int (*tile_get)(yatc_tile *tile, request_rec *r);
   int (*tile_set)(yatc_tile *tile, request_rec *r);
   char* (*configuration_parse)(xmlNode *xml, yatc_cache *cache, apr_pool_t *pool);
   char* (*configuration_check)(yatc_cache *cache, apr_pool_t *pool);  
   int (*tile_lock)(yatc_tile *tile, request_rec *r);
   int (*tile_unlock)(yatc_tile *tile, request_rec *r);
};

typedef struct {
   yatc_cache cache;
   char *base_directory;
} yatc_cache_disk;

typedef struct {
   yatc_tile **tiles;
   int ntiles;
} yatc_request;

typedef struct {
   yatc_service_type type;
   yatc_request* (*parse_request)(request_rec *r, apr_table_t *params, yatc_cfg *config);
} yatc_service;

typedef struct {
   yatc_service service;
} yatc_service_wms;

typedef struct {
   yatc_service service;
} yatc_service_tms;


struct yatc_tileset {
   char *name;
   double extent[4];
   char *srs;
   int tile_sx, tile_sy;
   int levels;
   double *resolutions;
   int metasize_x, metasize_y;
   int metabuffer;
   yatc_cache *cache;
   yatc_source *source;
   apr_table_t *forwarded_params;
};

struct yatc_cfg {
   char *configFile;
   yatc_service* services[YATC_SERVICES_COUNT];
   apr_hash_t *sources;
   apr_hash_t *caches;
   apr_hash_t *tilesets;
};

typedef struct {
   apr_global_mutex_t *mutex;
} yatc_server_cfg;





/* in buffer.c */
yatc_buffer *yatc_buffer_create(size_t initialStorage, apr_pool_t* pool);
int yatc_buffer_append(yatc_buffer *buffer, size_t len, void *data);

/* in http.c */
int yatc_http_request_url(request_rec *r, char *url, yatc_buffer *data);
int yatc_http_request_url_with_params(request_rec *r, char *url, apr_table_t *params, yatc_buffer *data);
char* yatc_http_build_url(request_rec *r, char *base, apr_table_t *params);
apr_table_t *yatc_http_parse_param_string(request_rec *r, char *args);

/* functions exported in configuration.c */
char* yatc_configuration_parse(const char *filename, yatc_cfg *config, apr_pool_t *pool);
yatc_cfg* yatc_configuration_create(apr_pool_t *pool);
yatc_source* yatc_configuration_get_source(yatc_cfg *config, const char *key);
yatc_cache* yatc_configuration_get_cache(yatc_cfg *config, const char *key);
yatc_tileset* yatc_configuration_get_tileset(yatc_cfg *config, const char *key);
void yatc_configuration_add_source(yatc_cfg *config, yatc_source *source, const char * key);
void yatc_configuration_add_tileset(yatc_cfg *config, yatc_tileset *tileset, const char * key);
void yatc_configuration_add_cache(yatc_cfg *config, yatc_cache *cache, const char * key);

/* functions exported in source.c */
void yatc_source_init(yatc_source *source, apr_pool_t *pool);

/* functions exported in wms_source.c */
yatc_wms_source* yatc_source_wms_create(apr_pool_t *pool);

/* functions exported in disk_cache.c */
yatc_cache_disk* yatc_cache_disk_create(apr_pool_t *pool);


/* functions exported in tileset.c */
int yatc_tileset_tile_lookup(yatc_tile *tile, double *bbox, request_rec *r);
int yatc_tileset_tile_get(yatc_tile *tile, request_rec *r);
int yatc_tileset_tile_render(yatc_tile *tile, request_rec *r);
yatc_tile* yatc_tileset_tile_create(yatc_tileset *tileset, apr_pool_t *pool);
yatc_tileset* yatc_tileset_create(apr_pool_t *pool);
void yatc_tileset_tile_bbox(yatc_tile *tile, double *bbox);



/* in service.c */
yatc_service* yatc_service_wms_create(apr_pool_t *pool);
yatc_service* yatc_service_tms_create(apr_pool_t *pool);

/* in util.c */
int yatc_util_extract_int_list(char* args, const char sep, int **numbers,
      int *numbers_count, apr_pool_t *pool);
int yatc_util_extract_double_list(char* args, const char sep, double **numbers,
      int *numbers_count, apr_pool_t *pool);

/* in image.c */
yatc_tile* yatc_image_merge_tiles(request_rec *r, yatc_tile **tiles, int ntiles);
int yatc_image_metatile_split(yatc_metatile *mt, request_rec *r);

#endif /* YATC_H_ */

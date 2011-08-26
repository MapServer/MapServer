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

#ifdef USE_MEMCACHE

#include "geocache.h"



/**
 * \brief return key for given tile
 * 
 * \param tile the tile to get the key from
 * \param key pointer to a char* that will contain the key
 * \param ctx
 * \private \memberof geocache_cache_memcache
 */
static void _geocache_cache_memcache_tile_key(geocache_context *ctx, geocache_tile *tile, char **path) {
   char *start;
   start = apr_pstrcat(ctx->pool,
         tile->tileset->name,"/",
         tile->grid_link->grid->name,
         NULL);
   if(tile->dimensions) {
      const apr_array_header_t *elts = apr_table_elts(tile->dimensions);
      int i = elts->nelts;
      while(i--) {
         apr_table_entry_t *entry = &(APR_ARRAY_IDX(elts,i,apr_table_entry_t));
         start = apr_pstrcat(ctx->pool,start,"/",entry->key,"/",entry->val,NULL);
      }
   }
   *path = apr_psprintf(ctx->pool,"%s/%02d/%03d/%03d/%03d/%03d/%03d/%03d.%s",
         start,
         tile->z,
         tile->x / 1000000,
         (tile->x / 1000) % 1000,
         tile->x % 1000,
         tile->y / 1000000,
         (tile->y / 1000) % 1000,
         tile->y % 1000,
         tile->tileset->format?tile->tileset->format->extension:"png");
   if(!*path) {
      ctx->set_error(ctx,500, "failed to allocate tile key");
   }
  
}

static int _geocache_cache_memcache_has_tile(geocache_context *ctx, geocache_tile *tile) {
   char *key, *tmpdata;
   int rv;
   size_t tmpdatasize;
   geocache_cache_memcache *cache = (geocache_cache_memcache*)tile->tileset->cache;
   _geocache_cache_memcache_tile_key(ctx, tile, &key);
   if(GC_HAS_ERROR(ctx)) {
      return GEOCACHE_FALSE;
   }
   rv = apr_memcache_getp(cache->memcache,ctx->pool,key,&tmpdata,&tmpdatasize,NULL);
   if(rv != APR_SUCCESS) {
      return GEOCACHE_FALSE;
   }
   if(tmpdatasize == 0) {
      return GEOCACHE_FALSE;
   }
   return GEOCACHE_TRUE;
}

static void _geocache_cache_memcache_delete(geocache_context *ctx, geocache_tile *tile) {
   char *key;
   int rv;
   char errmsg[120];
   geocache_cache_memcache *cache = (geocache_cache_memcache*)tile->tileset->cache;
   _geocache_cache_memcache_tile_key(ctx, tile, &key);
   GC_CHECK_ERROR(ctx);
   rv = apr_memcache_delete(cache->memcache,key,0);
   if(rv != APR_SUCCESS && rv!= APR_NOTFOUND) {
      int code = 500;
      ctx->set_error(ctx,code,"memcache: failed to delete key %s: %s", key, apr_strerror(rv,errmsg,120));
   }
}

/**
 * \brief get content of given tile
 * 
 * fills the geocache_tile::data of the given tile with content stored on the memcache server
 * \private \memberof geocache_cache_memcache
 * \sa geocache_cache::tile_get()
 */
static int _geocache_cache_memcache_get(geocache_context *ctx, geocache_tile *tile) {
   char *key;
   int rv;
   geocache_cache_memcache *cache = (geocache_cache_memcache*)tile->tileset->cache;
   _geocache_cache_memcache_tile_key(ctx, tile, &key);
   if(GC_HAS_ERROR(ctx)) {
      return GEOCACHE_FAILURE;
   }
   tile->data = geocache_buffer_create(0,ctx->pool);
   rv = apr_memcache_getp(cache->memcache,ctx->pool,key,(char**)&tile->data->buf,&tile->data->size,NULL);
   if(rv != APR_SUCCESS) {
      return GEOCACHE_CACHE_MISS;
   }
   if(tile->data->size == 0) {
      ctx->set_error(ctx,500,"memcache cache returned 0-length data for tile %d %d %d\n",tile->x,tile->y,tile->z);
      return GEOCACHE_FAILURE;
   }
   /* extract the tile modification time from the end of the data returned */
   memcpy(
         &tile->mtime,
         &(tile->data->buf[tile->data->size-sizeof(apr_time_t)]),
         sizeof(apr_time_t));
   tile->data->buf[tile->data->size+sizeof(apr_time_t)]='\0';
   tile->data->avail = tile->data->size;
   tile->data->size -= sizeof(apr_time_t);
   return GEOCACHE_SUCCESS;
}

/**
 * \brief push tile data to memcached
 * 
 * writes the content of geocache_tile::data to the configured memcached instance(s)
 * \returns GEOCACHE_FAILURE if there is no data to write, or if the tile isn't locked
 * \returns GEOCACHE_SUCCESS if the tile has been successfully written
 * \private \memberof geocache_cache_memcache
 * \sa geocache_cache::tile_set()
 */
static void _geocache_cache_memcache_set(geocache_context *ctx, geocache_tile *tile) {
   char *key;
   int rv;
   /* set expiration to one day if not configured */
   int expires = 86400;
   if(tile->tileset->auto_expire)
      expires = tile->tileset->auto_expire;
   geocache_cache_memcache *cache = (geocache_cache_memcache*)tile->tileset->cache;
   _geocache_cache_memcache_tile_key(ctx, tile, &key);
   GC_CHECK_ERROR(ctx);

   /* concatenate the current time to the end of the memcache data so we can extract it out
    * when we re-get the tile */
   char *data = calloc(1,tile->data->size+sizeof(apr_time_t));
   apr_time_t now = apr_time_now();
   apr_pool_cleanup_register(ctx->pool, data, (void*)free, apr_pool_cleanup_null);
   memcpy(data,tile->data->buf,tile->data->size);
   memcpy(&(data[tile->data->size]),&now,sizeof(apr_time_t));
   
   rv = apr_memcache_set(cache->memcache,key,data,tile->data->size+sizeof(apr_time_t),expires,0);
   if(rv != APR_SUCCESS) {
      ctx->set_error(ctx,500,"failed to store tile %d %d %d to memcache cache %s",
            tile->x,tile->y,tile->z,cache->cache.name);
      return;
   }
}

/**
 * \private \memberof geocache_cache_memcache
 */
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
static void _geocache_cache_memcache_configuration_parse_json(geocache_context *ctx, cJSON *node, geocache_cache *cache) {
   geocache_cache_memcache *dcache = (geocache_cache_memcache*)cache;
   cJSON *servers = cJSON_GetObjectItem(node,"servers");
   int n;
   if(!servers || !(n = cJSON_GetArraySize(servers))) {
      ctx->set_error(ctx,400,"memcache cache %s has no servers configured",cache->name);
      return;
   }
   if(APR_SUCCESS != apr_memcache_create(ctx->pool, n, 0, &dcache->memcache)) {
      ctx->set_error(ctx,400,"cache %s: failed to create memcache backend", cache->name);
      return;
   }
   while(n--) {
      cJSON *jserver = cJSON_GetArrayItem(servers,n);
      cJSON *jhostname = cJSON_GetObjectItem(jserver,"host");
      cJSON *jport = cJSON_GetObjectItem(jserver,"port");
      apr_memcache_server_t *server;
      apr_port_t port = 11211;
      char *host;
      if(!jhostname || !jhostname->valuestring || !strlen(jhostname->valuestring)) {
         ctx->set_error(ctx,400,"memcache cache %s has no hostname configured",cache->name);
         return;
      }
      host = apr_pstrdup(ctx->pool,jhostname->valuestring);
      if(jport && jport->valueint) {
         port = jport->valueint;
      }
      if(APR_SUCCESS != apr_memcache_server_create(ctx->pool,host,port,4,5,50,10000,&server)) {
         ctx->set_error(ctx,400,"cache %s: failed to create server %s:%d",cache->name,host,port);
         return;
      }
      if(APR_SUCCESS != apr_memcache_add_server(dcache->memcache,server)) {
         ctx->set_error(ctx,400,"cache %s: failed to add server %s:%d",cache->name,host,port);
         return;
      }
      if(APR_SUCCESS != apr_memcache_set(dcache->memcache,"geocache_test_key","geocache",8,0,0)) {
         ctx->set_error(ctx,400,"cache %s: failed to add test key to server %s:%d",cache->name,host,port);
         return;
      }
   }
}
#endif

/**
 * \private \memberof geocache_cache_memcache
 */
static void _geocache_cache_memcache_configuration_parse_xml(geocache_context *ctx, ezxml_t node, geocache_cache *cache) {
   ezxml_t cur_node;
   geocache_cache_memcache *dcache = (geocache_cache_memcache*)cache;
   int servercount = 0;
   for(cur_node = ezxml_child(node,"server"); cur_node; cur_node = cur_node->next) {
      servercount++;
   }
   if(!servercount) {
      ctx->set_error(ctx,400,"memcache cache %s has no <server>s configured",cache->name);
      return;
   }
   if(APR_SUCCESS != apr_memcache_create(ctx->pool, servercount, 0, &dcache->memcache)) {
      ctx->set_error(ctx,400,"cache %s: failed to create memcache backend", cache->name);
      return;
   }
   for(cur_node = ezxml_child(node,"server"); cur_node; cur_node = cur_node->next) {
      ezxml_t xhost = ezxml_child(cur_node,"host");
      ezxml_t xport = ezxml_child(cur_node,"port");
      const char *host;
      apr_memcache_server_t *server;
      apr_port_t port;
      if(!xhost || !xhost->txt || ! *xhost->txt) {
         ctx->set_error(ctx,400,"cache %s: <server> with no <host>",cache->name);
         return;
      } else {
         host = apr_pstrdup(ctx->pool,xhost->txt);
      }

      if(!xport || !xport->txt || ! *xport->txt) {
         ctx->set_error(ctx,400,"cache %s: <server> with no <port>", cache->name);
         return;
      } else {
         char *endptr;
         int iport = (int)strtol(xport->txt,&endptr,10);
         if(*endptr != 0) {
            ctx->set_error(ctx,400,"failed to parse value %s for memcache cache %s", xport->txt,cache->name);
            return;
         }
         port = iport;
      }
      if(APR_SUCCESS != apr_memcache_server_create(ctx->pool,host,port,4,5,50,10000,&server)) {
         ctx->set_error(ctx,400,"cache %s: failed to create server %s:%d",cache->name,host,port);
         return;
      }
      if(APR_SUCCESS != apr_memcache_add_server(dcache->memcache,server)) {
         ctx->set_error(ctx,400,"cache %s: failed to add server %s:%d",cache->name,host,port);
         return;
      }
      if(APR_SUCCESS != apr_memcache_set(dcache->memcache,"geocache_test_key","geocache",8,0,0)) {
         ctx->set_error(ctx,400,"cache %s: failed to add test key to server %s:%d",cache->name,host,port);
         return;
      }
   }
}
   
/**
 * \private \memberof geocache_cache_memcache
 */
static void _geocache_cache_memcache_configuration_post_config(geocache_context *ctx, geocache_cache *cache,
      geocache_cfg *cfg) {
   geocache_cache_memcache *dcache = (geocache_cache_memcache*)cache;
   if(!dcache->memcache || dcache->memcache->ntotal==0) {
      ctx->set_error(ctx,400,"cache %s has no servers configured",cache->name);
   }
}


/**
 * \brief creates and initializes a geocache_memcache_cache
 */
geocache_cache* geocache_cache_memcache_create(geocache_context *ctx) {
   geocache_cache_memcache *cache = apr_pcalloc(ctx->pool,sizeof(geocache_cache_memcache));
   if(!cache) {
      ctx->set_error(ctx, 500, "failed to allocate memcache cache");
      return NULL;
   }
   cache->cache.metadata = apr_table_make(ctx->pool,3);
   cache->cache.type = GEOCACHE_CACHE_MEMCACHE;
   cache->cache.tile_get = _geocache_cache_memcache_get;
   cache->cache.tile_exists = _geocache_cache_memcache_has_tile;
   cache->cache.tile_set = _geocache_cache_memcache_set;
   cache->cache.tile_delete = _geocache_cache_memcache_delete;
   cache->cache.configuration_post_config = _geocache_cache_memcache_configuration_post_config;
   cache->cache.configuration_parse_xml = _geocache_cache_memcache_configuration_parse_xml;
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
   cache->cache.configuration_parse_json = _geocache_cache_memcache_configuration_parse_json;
#endif
   return (geocache_cache*)cache;
}

#endif

/* vim: ai ts=3 sts=3 et sw=3
*/

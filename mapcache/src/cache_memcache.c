/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: memcache cache backend.
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
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
 *****************************************************************************/

#ifdef USE_MEMCACHE

#include "mapcache.h"



/**
 * \brief return key for given tile
 * 
 * \param tile the tile to get the key from
 * \param key pointer to a char* that will contain the key
 * \param ctx
 * \private \memberof mapcache_cache_memcache
 */
static void _mapcache_cache_memcache_tile_key(mapcache_context *ctx, mapcache_tile *tile, char **path) {
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
#ifdef DEBUG
   ctx->log(ctx,MAPCACHE_DEBUG,"memcache key is %s (length:%d)",
         *path,strlen(*path));
#endif
  
}

static int _mapcache_cache_memcache_has_tile(mapcache_context *ctx, mapcache_tile *tile) {
   char *key, *tmpdata;
   int rv;
   size_t tmpdatasize;
   mapcache_cache_memcache *cache = (mapcache_cache_memcache*)tile->tileset->cache;
   _mapcache_cache_memcache_tile_key(ctx, tile, &key);
   if(GC_HAS_ERROR(ctx)) {
      return MAPCACHE_FALSE;
   }
   rv = apr_memcache_getp(cache->memcache,ctx->pool,key,&tmpdata,&tmpdatasize,NULL);
   if(rv != APR_SUCCESS) {
      return MAPCACHE_FALSE;
   }
   if(tmpdatasize == 0) {
      return MAPCACHE_FALSE;
   }
   return MAPCACHE_TRUE;
}

static void _mapcache_cache_memcache_delete(mapcache_context *ctx, mapcache_tile *tile) {
   char *key;
   int rv;
   char errmsg[120];
   mapcache_cache_memcache *cache = (mapcache_cache_memcache*)tile->tileset->cache;
   _mapcache_cache_memcache_tile_key(ctx, tile, &key);
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
 * fills the mapcache_tile::data of the given tile with content stored on the memcache server
 * \private \memberof mapcache_cache_memcache
 * \sa mapcache_cache::tile_get()
 */
static int _mapcache_cache_memcache_get(mapcache_context *ctx, mapcache_tile *tile) {
   char *key;
   int rv;
   mapcache_cache_memcache *cache = (mapcache_cache_memcache*)tile->tileset->cache;
   _mapcache_cache_memcache_tile_key(ctx, tile, &key);
   if(GC_HAS_ERROR(ctx)) {
      return MAPCACHE_FAILURE;
   }
   tile->data = mapcache_buffer_create(0,ctx->pool);
   rv = apr_memcache_getp(cache->memcache,ctx->pool,key,(char**)&tile->data->buf,&tile->data->size,NULL);
   if(rv != APR_SUCCESS) {
      return MAPCACHE_CACHE_MISS;
   }
   if(tile->data->size == 0) {
      ctx->set_error(ctx,500,"memcache cache returned 0-length data for tile %d %d %d\n",tile->x,tile->y,tile->z);
      return MAPCACHE_FAILURE;
   }
   /* extract the tile modification time from the end of the data returned */
   memcpy(
         &tile->mtime,
         &(((char*)tile->data->buf)[tile->data->size-sizeof(apr_time_t)]),
         sizeof(apr_time_t));
   ((char*)tile->data->buf)[tile->data->size+sizeof(apr_time_t)]='\0';
   tile->data->avail = tile->data->size;
   tile->data->size -= sizeof(apr_time_t);
   return MAPCACHE_SUCCESS;
}

/**
 * \brief push tile data to memcached
 * 
 * writes the content of mapcache_tile::data to the configured memcached instance(s)
 * \returns MAPCACHE_FAILURE if there is no data to write, or if the tile isn't locked
 * \returns MAPCACHE_SUCCESS if the tile has been successfully written
 * \private \memberof mapcache_cache_memcache
 * \sa mapcache_cache::tile_set()
 */
static void _mapcache_cache_memcache_set(mapcache_context *ctx, mapcache_tile *tile) {
   char *key;
   int rv;
   /* set expiration to one day if not configured */
   int expires = 86400;
   if(tile->tileset->auto_expire)
      expires = tile->tileset->auto_expire;
   mapcache_cache_memcache *cache = (mapcache_cache_memcache*)tile->tileset->cache;
   _mapcache_cache_memcache_tile_key(ctx, tile, &key);
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
 * \private \memberof mapcache_cache_memcache
 */
static void _mapcache_cache_memcache_configuration_parse_xml(mapcache_context *ctx, ezxml_t node, mapcache_cache *cache, mapcache_cfg *config) {
   ezxml_t cur_node;
   mapcache_cache_memcache *dcache = (mapcache_cache_memcache*)cache;
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
      if(APR_SUCCESS != apr_memcache_set(dcache->memcache,"mapcache_test_key","mapcache",8,0,0)) {
         ctx->set_error(ctx,400,"cache %s: failed to add test key to server %s:%d",cache->name,host,port);
         return;
      }
   }
}
   
/**
 * \private \memberof mapcache_cache_memcache
 */
static void _mapcache_cache_memcache_configuration_post_config(mapcache_context *ctx, mapcache_cache *cache,
      mapcache_cfg *cfg) {
   mapcache_cache_memcache *dcache = (mapcache_cache_memcache*)cache;
   if(!dcache->memcache || dcache->memcache->ntotal==0) {
      ctx->set_error(ctx,400,"cache %s has no servers configured",cache->name);
   }
}


/**
 * \brief creates and initializes a mapcache_memcache_cache
 */
mapcache_cache* mapcache_cache_memcache_create(mapcache_context *ctx) {
   mapcache_cache_memcache *cache = apr_pcalloc(ctx->pool,sizeof(mapcache_cache_memcache));
   if(!cache) {
      ctx->set_error(ctx, 500, "failed to allocate memcache cache");
      return NULL;
   }
   cache->cache.metadata = apr_table_make(ctx->pool,3);
   cache->cache.type = MAPCACHE_CACHE_MEMCACHE;
   cache->cache.tile_get = _mapcache_cache_memcache_get;
   cache->cache.tile_exists = _mapcache_cache_memcache_has_tile;
   cache->cache.tile_set = _mapcache_cache_memcache_set;
   cache->cache.tile_delete = _mapcache_cache_memcache_delete;
   cache->cache.configuration_post_config = _mapcache_cache_memcache_configuration_post_config;
   cache->cache.configuration_parse_xml = _mapcache_cache_memcache_configuration_parse_xml;
   return (mapcache_cache*)cache;
}

#endif

/* vim: ai ts=3 sts=3 et sw=3
*/

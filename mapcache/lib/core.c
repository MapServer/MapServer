/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: high level functions called
 *           from the CGI or apache-module implementations
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

#include <apr_strings.h>
#include "mapcache.h"
#if APR_HAS_THREADS
#include "apu_version.h"

#if (APU_MAJOR_VERSION <= 1 && APU_MINOR_VERSION <= 3)
 #define USE_THREADPOOL 0
#else
 #define USE_THREADPOOL 1
#endif

/* use a thread pool if using 1.3.12 or higher apu */
#if !USE_THREADPOOL
#include <apr_thread_proc.h>
#else
#include <apr_thread_pool.h>
#endif
 
typedef struct {
   mapcache_tile *tile;
   mapcache_context *ctx;
   int launch;
} _thread_tile;

static void* APR_THREAD_FUNC _thread_get_tile(apr_thread_t *thread, void *data) {
   _thread_tile* t = (_thread_tile*)data;
   mapcache_tileset_tile_get(t->ctx, t->tile);
#if !USE_THREADPOOL
   apr_thread_exit(thread, APR_SUCCESS);
#endif
   return NULL;
}

#endif


mapcache_http_response *mapcache_http_response_create(apr_pool_t *pool) {
   mapcache_http_response *response = (mapcache_http_response*) apr_pcalloc(pool,
         sizeof(mapcache_http_response));
   /* make room for at least Expires, Cache-Control, and Content-Type */ 
   response->headers = apr_table_make(pool,3);
   response->code = 200;
   return response;
}

void mapcache_prefetch_tiles(mapcache_context *ctx, mapcache_tile **tiles, int ntiles) {

   apr_thread_t **threads;
   apr_threadattr_t *thread_attrs;
   int nthreads;
#if !APR_HAS_THREADS
   int i;
   for(i=0;i<ntiles;i++) {
      mapcache_tileset_tile_get(ctx, tiles[i]);
      GC_CHECK_ERROR(ctx);
   }
#else
   int i,rv;
   _thread_tile* thread_tiles;
   if(ntiles==1 || ctx->config->threaded_fetching == 0) {
   /* if threads disabled, or only fetching a single tile, don't launch a thread for the operation */
      for(i=0;i<ntiles;i++) {
         mapcache_tileset_tile_get(ctx, tiles[i]);
         GC_CHECK_ERROR(ctx);
      }
      return;
   }


   /* allocate a thread struct for each tile. Not all will be used */
   thread_tiles = (_thread_tile*)apr_pcalloc(ctx->pool,ntiles*sizeof(_thread_tile));
#if 1 || !USE_THREADPOOL
   /* use multiple threads, to fetch from multiple metatiles and/or multiple tilesets */
   apr_threadattr_create(&thread_attrs, ctx->pool);
   threads = (apr_thread_t**)apr_pcalloc(ctx->pool, ntiles*sizeof(apr_thread_t*));
   nthreads = 0;
   for(i=0;i<ntiles;i++) {
     int j;
      thread_tiles[i].tile = tiles[i];
      thread_tiles[i].launch = 1;
      j=i-1;
      /* 
       * we only launch one thread per metatile as in the unseeded case the threads
       * for a same metatile will lock while only a single thread launches the actual
       * rendering request
       */
      while(j>=0) {
         /* check that the given metatile hasn't been rendered yet */
         if(thread_tiles[j].launch &&
               (thread_tiles[i].tile->tileset == thread_tiles[j].tile->tileset) &&
               (thread_tiles[i].tile->x / thread_tiles[i].tile->tileset->metasize_x  == 
                  thread_tiles[j].tile->x / thread_tiles[j].tile->tileset->metasize_x)&&
               (thread_tiles[i].tile->y / thread_tiles[i].tile->tileset->metasize_y  == 
                  thread_tiles[j].tile->y / thread_tiles[j].tile->tileset->metasize_y)) {
            thread_tiles[i].launch = 0; /* this tile will not have a thread spawned for it */
            break;
         }
         j--;
      }
      if(thread_tiles[i].launch)
         thread_tiles[i].ctx = ctx->clone(ctx);
   }
   for(i=0;i<ntiles;i++) {
      if(!thread_tiles[i].launch) continue; /* skip tiles that have been marked */
      rv = apr_thread_create(&threads[i], thread_attrs, _thread_get_tile, (void*)&(thread_tiles[i]), thread_tiles[i].ctx->pool);
      if(rv != APR_SUCCESS) {
         ctx->set_error(ctx,500, "failed to create thread %d of %d\n",i,ntiles);
         break;
      }
      nthreads++;
   }

   /* wait for launched threads to finish */
   for(i=0;i<ntiles;i++) {
      if(!thread_tiles[i].launch) continue;
      apr_thread_join(&rv, threads[i]);
      if(rv != APR_SUCCESS) {
         ctx->set_error(ctx,500, "thread %d of %d failed on exit\n",i,ntiles);
      }
      if(GC_HAS_ERROR(thread_tiles[i].ctx)) {
         /* transfer error message from child thread to main context */
         ctx->set_error(ctx,thread_tiles[i].ctx->get_error(thread_tiles[i].ctx),
               thread_tiles[i].ctx->get_error_message(thread_tiles[i].ctx));
      }
   }
   for(i=0;i<ntiles;i++) {
      /* fetch the tiles that did not get a thread launched for them */
      if(thread_tiles[i].launch) continue;
      mapcache_tileset_tile_get(ctx, tiles[i]);
   }
#else
    /* experimental version using a threadpool, disabled for stability reasons */
   apr_thread_pool_t *thread_pool;
   apr_thread_pool_create(&thread_pool,2,ctx->config->download_threads,ctx->pool);
   for(i=0;i<ntiles;i++) {
      ctx->log(ctx,MAPCACHE_DEBUG,"starting thread for tile %s",tiles[i]->tileset->name);
      thread_tiles[i].tile = tiles[i];
      thread_tiles[i].ctx = ctx->clone(ctx);
      rv = apr_thread_pool_push(thread_pool,_thread_get_tile,(void*)&(thread_tiles[i]), 0,NULL);
      if(rv != APR_SUCCESS) {
         ctx->set_error(ctx,500, "failed to push thread %d of %d in thread pool\n",i,ntiles);
         break;
      }
   }
   GC_CHECK_ERROR(ctx);
   while(apr_thread_pool_tasks_run_count(thread_pool) != ntiles || apr_thread_pool_busy_count(thread_pool)>0)
      apr_sleep(10000);
   apr_thread_pool_destroy(thread_pool);
   for(i=0;i<ntiles;i++) {
      if(GC_HAS_ERROR(thread_tiles[i].ctx)) {
         ctx->set_error(ctx,thread_tiles[i].ctx->get_error(thread_tiles[i].ctx),
               thread_tiles[i].ctx->get_error_message(thread_tiles[i].ctx));
      }
   }
#endif
   
#endif

}

mapcache_http_response *mapcache_core_get_tile(mapcache_context *ctx, mapcache_request_get_tile *req_tile) {
  int expires = 0;
  mapcache_http_response *response;
   int i;
   char *timestr;
   mapcache_image *base=NULL,*overlay;
   mapcache_image_format *format = NULL;

#ifdef DEBUG
   if(req_tile->ntiles ==0) {
      ctx->set_error(ctx,500,"BUG: get_tile called with 0 tiles");
      return NULL;
   }
#endif
   expires = 0;
   response = mapcache_http_response_create(ctx->pool);
  

   mapcache_prefetch_tiles(ctx,req_tile->tiles,req_tile->ntiles);
   if(GC_HAS_ERROR(ctx))
      return NULL;

   /* this loop retrieves the tiles from the caches, and eventually decodes and merges them together
    * if multiple tiles were asked for */
   for(i=0;i<req_tile->ntiles;i++) {
      mapcache_tile *tile = req_tile->tiles[i];
      if(GC_HAS_ERROR(ctx))
         return NULL;
      if(i==0) {
         response->mtime = tile->mtime;
         expires = tile->expires;
         /* if we have multiple tiles to merge, decode the image data */
         if(req_tile->ntiles>1) {
            if(!tile->raw_image) {
               tile->raw_image = mapcache_imageio_decode(ctx, tile->encoded_data);
               if(!tile->raw_image) return NULL;
            }
            base = tile->raw_image;
         }
      } else {
         if(response->mtime < tile->mtime)
            response->mtime = tile->mtime;
         if(tile->expires < expires) {
            expires = tile->expires;
         }
         if(!tile->raw_image) {
            tile->raw_image = mapcache_imageio_decode(ctx, tile->encoded_data);
            if(!tile->raw_image) return NULL;
         }
         overlay = tile->raw_image;
         mapcache_image_merge(ctx, base, overlay);
         if(GC_HAS_ERROR(ctx)) {
            return NULL;
         }
      }
   }
   format = NULL;

   /* if we had more than one tile, we need to encode the raw image data into a mapcache_buffer */
   if(req_tile->ntiles > 1) {
      if(req_tile->ntiles>1) {
         format = req_tile->tiles[0]->tileset->format;
         if(req_tile->format) {
            format = req_tile->format;
         }
         if(!format) {
            format = ctx->config->default_image_format; /* this one is always defined */
         }
      }
      response->data = format->write(ctx, base, format);
      if(GC_HAS_ERROR(ctx)) {
         return NULL;
      }
   } else {
      response->data = req_tile->tiles[0]->encoded_data;
      format = req_tile->tiles[0]->tileset->format;
   }

   /* compute the content-type */
   if(format && format->mime_type) {
      apr_table_set(response->headers,"Content-Type",format->mime_type);
   } else {
      mapcache_image_format_type t = mapcache_imageio_header_sniff(ctx,response->data);
      if(t == GC_PNG)
         apr_table_set(response->headers,"Content-Type","image/png");
      else if(t == GC_JPEG)
         apr_table_set(response->headers,"Content-Type","image/jpeg");
   }

   /* compute expiry headers */
   if(expires) {
      apr_time_t now = apr_time_now();
      apr_time_t additional = apr_time_from_sec(expires);
      apr_time_t texpires = now + additional;
      apr_table_set(response->headers, "Cache-Control",apr_psprintf(ctx->pool, "max-age=%d", expires));
      timestr = apr_palloc(ctx->pool, APR_RFC822_DATE_LEN);
      apr_rfc822_date(timestr, texpires);
      apr_table_setn(response->headers, "Expires", timestr);
   }

   return response;
}

void mapcache_fetch_maps(mapcache_context *ctx, mapcache_map **maps, int nmaps, mapcache_resample_mode mode ) {
   mapcache_tile ***maptiles;
   int *nmaptiles;
   mapcache_tile **tiles;
   int ntiles = 0;
   int i;
   maptiles = apr_pcalloc(ctx->pool,nmaps*sizeof(mapcache_tile**));
   nmaptiles = apr_pcalloc(ctx->pool,nmaps*sizeof(int));
   for(i=0;i<nmaps;i++) {
      mapcache_tileset_get_map_tiles(ctx,maps[i]->tileset,maps[i]->grid_link,
            maps[i]->extent, maps[i]->width, maps[i]->height,
            &(nmaptiles[i]), &(maptiles[i]));
      ntiles += nmaptiles[i];
   }
   tiles = apr_pcalloc(ctx->pool,ntiles * sizeof(mapcache_tile*));
   ntiles = 0;
   for(i=0;i<nmaps;i++) {
      int j;
      for(j=0;j<nmaptiles[i];j++) {
         tiles[ntiles] = maptiles[i][j];
         tiles[ntiles]->dimensions = maps[i]->dimensions;
         ntiles++;
      }
   }
   mapcache_prefetch_tiles(ctx,tiles,ntiles);
   for(i=0;i<nmaps;i++) {
      int j;
      for(j=0;j<nmaptiles[i];j++) {
         /* update the map modification time if it is older than the tile mtime */
         if(maptiles[i][j]->mtime>maps[i]->mtime) maps[i]->mtime = maptiles[i][j]->mtime;

         /* set the map expiration delay to the tile expiration delay,
          * either if the map hasn't got an expiration delay yet
          * or if the tile expiration is shorter than the map expiration
          */
         if(!maps[i]->expires || maptiles[i][j]->expires<maps[i]->expires) maps[i]->expires =maptiles[i][j]->expires;
      }
      maps[i]->raw_image = mapcache_tileset_assemble_map_tiles(ctx,maps[i]->tileset,maps[i]->grid_link,
            maps[i]->extent, maps[i]->width, maps[i]->height,
            nmaptiles[i], maptiles[i],
            mode);
   }
}

mapcache_http_response *mapcache_core_get_map(mapcache_context *ctx, mapcache_request_get_map *req_map) {
  mapcache_image_format *format = NULL;
  mapcache_http_response *response;
  mapcache_map *basemap;
  int i;
   char *timestr;
#ifdef DEBUG
   if(req_map->nmaps ==0) {
      ctx->set_error(ctx,500,"BUG: get_map called with 0 maps");
      return NULL;
   }
#endif

   
   if(req_map->getmap_strategy == MAPCACHE_GETMAP_ERROR) {
      ctx->set_error(ctx, 404, "full wms support disabled");
      return NULL;
   }
   
   format = NULL;
   response = mapcache_http_response_create(ctx->pool);
   basemap = req_map->maps[0];

   
   if(req_map->getmap_strategy == MAPCACHE_GETMAP_ASSEMBLE) {
      mapcache_fetch_maps(ctx, req_map->maps, req_map->nmaps, req_map->resample_mode);
      if(GC_HAS_ERROR(ctx)) return NULL;
      for(i=1;i<req_map->nmaps;i++) {
         mapcache_map *overlaymap = req_map->maps[i];
         if(GC_HAS_ERROR(ctx)) return NULL;
         mapcache_image_merge(ctx,basemap->raw_image,overlaymap->raw_image);
         if(GC_HAS_ERROR(ctx)) return NULL;
         if(overlaymap->mtime > basemap->mtime) basemap->mtime = overlaymap->mtime;
         if(!basemap->expires || overlaymap->expires<basemap->expires) basemap->expires = overlaymap->expires;
      }
   } else /*if(ctx->config->getmap_strategy == MAPCACHE_GETMAP_FORWARD)*/ {
      int i;
      for(i=0;i<req_map->nmaps;i++) {
         if(!req_map->maps[i]->tileset->source) {
            ctx->set_error(ctx,404,"cannot forward request for tileset %s: no source configured",
                  req_map->maps[i]->tileset->name);
            return NULL;
         }
      }
      basemap->tileset->source->render_map(ctx, basemap);
      if(GC_HAS_ERROR(ctx)) return NULL;
      if(req_map->nmaps>1) {
         if(!basemap->raw_image) {
            basemap->raw_image = mapcache_imageio_decode(ctx,basemap->encoded_data);
            if(GC_HAS_ERROR(ctx)) return NULL;
         }
         for(i=1;i<req_map->nmaps;i++) {
            mapcache_map *overlaymap = req_map->maps[i];
            overlaymap->tileset->source->render_map(ctx, overlaymap);
            if(GC_HAS_ERROR(ctx)) return NULL;
            if(!overlaymap->raw_image) {
               overlaymap->raw_image = mapcache_imageio_decode(ctx,overlaymap->encoded_data);
               if(GC_HAS_ERROR(ctx)) return NULL;
            }
            if(GC_HAS_ERROR(ctx)) return NULL;
            mapcache_image_merge(ctx,basemap->raw_image,overlaymap->raw_image);
            if(GC_HAS_ERROR(ctx)) return NULL;
            if(!basemap->expires || overlaymap->expires<basemap->expires) basemap->expires = overlaymap->expires;
         }
      }
   }
   
   if(basemap->raw_image) {
      format = req_map->getmap_format; /* always defined, defaults to JPEG */
      response->data = format->write(ctx,basemap->raw_image,format);
      if(GC_HAS_ERROR(ctx)) {
         return NULL;
      }
   } else {
      /* this case happens when we have a forward strategy for a single tileset */
#ifdef DEBUG
      if(!basemap->encoded_data) {
         ctx->set_error(ctx,500,"###BUG### core_get_map failed with null encoded_data");
         return NULL;
      }
#endif
      response->data = basemap->encoded_data;
   }

   /* compute the content-type */
   if(format && format->mime_type) {
      apr_table_set(response->headers,"Content-Type",format->mime_type);
   } else {
      mapcache_image_format_type t = mapcache_imageio_header_sniff(ctx,response->data);
      if(t == GC_PNG)
         apr_table_set(response->headers,"Content-Type","image/png");
      else if(t == GC_JPEG)
         apr_table_set(response->headers,"Content-Type","image/jpeg");
   }

   /* compute expiry headers */
   if(basemap->expires) {
      apr_time_t now = apr_time_now();
      apr_time_t additional = apr_time_from_sec(basemap->expires);
      apr_time_t texpires = now + additional;
      apr_table_set(response->headers, "Cache-Control",
            apr_psprintf(ctx->pool, "max-age=%d", basemap->expires));
      timestr = apr_palloc(ctx->pool, APR_RFC822_DATE_LEN);
      apr_rfc822_date(timestr, texpires);
      apr_table_setn(response->headers, "Expires", timestr);
   }

   response->mtime = basemap->mtime;
   return response;
}

mapcache_http_response *mapcache_core_proxy_request(mapcache_context *ctx, mapcache_request_proxy *req_proxy) {
  mapcache_http *http;
   mapcache_http_response *response = mapcache_http_response_create(ctx->pool);
    response->data = mapcache_buffer_create(30000,ctx->pool);
    http = req_proxy->http;
    if(req_proxy->pathinfo) {
      http = mapcache_http_clone(ctx,http);
      if( (*(req_proxy->pathinfo)) == '/' ||
            http->url[strlen(http->url)-1] == '/')
         http->url = apr_pstrcat(ctx->pool,http->url,req_proxy->pathinfo,NULL);
      else
         http->url = apr_pstrcat(ctx->pool,http->url,"/",req_proxy->pathinfo,NULL);
    }
    mapcache_http_do_request_with_params(ctx,http,req_proxy->params,response->data,response->headers,&response->code);
    if(response->code !=0 && GC_HAS_ERROR(ctx)) {
       /* the http request was successful, but the server returned an error */
       ctx->clear_errors(ctx);
    }
    /*remove some headers that should not be sent back to the client*/
    apr_table_unset(response->headers,"Transfer-Encoding");
    apr_table_unset(response->headers,"Connection");
    return response;
}

mapcache_http_response *mapcache_core_get_featureinfo(mapcache_context *ctx,
      mapcache_request_get_feature_info *req_fi) {
   mapcache_feature_info *fi = req_fi->fi;
   mapcache_tileset *tileset = fi->map.tileset;
   if(!tileset->source) {
      ctx->set_error(ctx,404,"cannot query tileset %s: no source defined",tileset->name);
      return NULL;
   }
   if(tileset->source->info_formats) {
      int i;
      mapcache_http_response *response;
      for(i=0;i<tileset->source->info_formats->nelts;i++) {
         if(!strcmp(fi->format, APR_ARRAY_IDX(tileset->source->info_formats,i,char*))) {
            break;
         }
      }
      if(i == tileset->source->info_formats->nelts) {
         ctx->set_error(ctx,404, "unsupported feature info format %s",fi->format);
         return NULL;
      }
      tileset->source->query_info(ctx,fi);
      if(GC_HAS_ERROR(ctx)) return NULL;
      response = mapcache_http_response_create(ctx->pool);
      response->data = fi->data;
      apr_table_set(response->headers,"Content-Type",fi->format);
      return response;
   } else {
      ctx->set_error(ctx,404, "tileset %s does not support feature info requests");
      return NULL;
   }
}

mapcache_http_response* mapcache_core_get_capabilities(mapcache_context *ctx, mapcache_service *service,
      mapcache_request_get_capabilities *req_caps, char *url, char *path_info, mapcache_cfg *config) {
  mapcache_http_response *response;
   service->create_capabilities_response(ctx,req_caps,url,path_info,config);
   if(GC_HAS_ERROR(ctx)) {
      return NULL;
   }
   response = mapcache_http_response_create(ctx->pool);
   response->data = mapcache_buffer_create(0,ctx->pool);
   response->data->size = strlen(req_caps->capabilities);
   response->data->buf = req_caps->capabilities;
   response->data->avail = response->data->size;
   apr_table_set(response->headers,"Content-Type",req_caps->mime_type);
   return response;
}

mapcache_http_response* mapcache_core_respond_to_error(mapcache_context *ctx) {
  char *msg;
   //TODO: have the service format the error response
   mapcache_http_response *response = mapcache_http_response_create(ctx->pool);
   
   /* extract code and message from context */
   response->code = ctx->_errcode;
   if(!response->code) response->code = 500;

   msg = ctx->_errmsg;
   if(!msg) {
      msg = "an unspecified error has occured";
   }
   ctx->log(ctx,MAPCACHE_ERROR,msg);

  

   if(ctx->config && ctx->config->reporting == MAPCACHE_REPORT_MSG) {
      char *err_body = msg;
      apr_table_set(response->headers, "Content-Type", "text/plain");
      if(ctx->service && ctx->service->format_error) {
         ctx->service->format_error(ctx,ctx->service,msg,&err_body,response->headers);
      }
      /* manually populate the mapcache_buffer with the error message */
      response->data = mapcache_buffer_create(0,ctx->pool);
      response->data->size = strlen(err_body);
      response->data->buf = err_body;
      response->data->avail = response->data->size;
   } else if(ctx->config && ctx->config->reporting == MAPCACHE_REPORT_EMPTY_IMG) {
      response->data = ctx->config->empty_image;
      apr_table_set(response->headers, "Content-Type", ctx->config->default_image_format->mime_type);
      apr_table_set(response->headers, "X-Mapcache-Error", msg);
   } else if(ctx->config && ctx->config->reporting == MAPCACHE_REPORT_ERROR_IMG) {
      mapcache_image *errim = mapcache_error_image(ctx,256,256,msg);
      mapcache_buffer *buf = ctx->config->default_image_format->write(ctx,errim,ctx->config->default_image_format);
      response->data = buf;
      apr_table_set(response->headers, "Content-Type", ctx->config->default_image_format->mime_type);
      apr_table_set(response->headers, "X-Mapcache-Error", msg);
   }
   return response;

}

/* vim: ai ts=3 sts=3 et sw=3
*/

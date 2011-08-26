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

#include <apr_strings.h>
#include "geocache.h"

geocache_tile *geocache_core_get_tile(geocache_context *ctx, geocache_request_get_tile *req_tile) {
#ifdef DEBUG
   if(req_tile->ntiles ==0) {
      ctx->set_error(ctx,500,"BUG: get_tile called with 0 tiles");
      return NULL;
   }
#endif
   geocache_tile *rettile = NULL;
   geocache_image_format *format = NULL;
   if(req_tile->ntiles>1) {
      format = req_tile->tiles[0]->tileset->format;
      if(req_tile->format) {
         format = req_tile->format;
      }
      if(!format) {
         format = ctx->config->default_image_format; /* this one is always defined */
      }
   }
   int i;
   for(i=0;i<req_tile->ntiles;i++) {
      geocache_tile *tile = req_tile->tiles[i];
      geocache_tileset_tile_get(ctx, tile);
      if(GC_HAS_ERROR(ctx))
         return NULL;
   }
   if(req_tile->ntiles == 1) {
      rettile = req_tile->tiles[0];
   } else {
      rettile = (geocache_tile*)geocache_image_merge_tiles(ctx,format,req_tile->tiles,req_tile->ntiles);
      if(GC_HAS_ERROR(ctx))
         return NULL;
      rettile->tileset = req_tile->tiles[0]->tileset;
   }
   return rettile;
}



geocache_image* _core_get_single_map(geocache_context *ctx, geocache_map *map, geocache_resample_mode mode) {

   geocache_tile **maptiles;
   int i,nmaptiles;
   geocache_tileset_get_map_tiles(ctx,map->tileset,map->grid_link,
         map->extent, map->width, map->height,
         &nmaptiles, &maptiles);
   for(i=0;i<nmaptiles;i++) {
      geocache_tile *tile = maptiles[i];
      tile->dimensions = map->dimensions;
      geocache_tileset_tile_get(ctx, tile);
      if(GC_HAS_ERROR(ctx))
         return NULL;
      
      /* update the map modification time if it is older than the tile mtime */
      if(tile->mtime>map->mtime) map->mtime = tile->mtime;

      /* set the map expiration delay to the tile expiration delay,
       * either if the map hasn't got an expiration delay yet
       * or if the tile expiration is shorter than the map expiration
       */
      if(!map->expires || tile->expires<map->expires) map->expires = tile->expires;
   }
   geocache_image *getmapim = geocache_tileset_assemble_map_tiles(ctx,map->tileset,map->grid_link,
         map->extent, map->width, map->height,
         nmaptiles, maptiles,
         mode);
   return getmapim;
}

geocache_map *geocache_core_get_map(geocache_context *ctx, geocache_request_get_map *req_map) {
#ifdef DEBUG
   if(req_map->nmaps ==0) {
      ctx->set_error(ctx,500,"BUG: get_map called with 0 maps");
      return NULL;
   }
#endif

   geocache_image_format *format = req_map->getmap_format; /* always defined, defaults to JPEG */
   
   if(req_map->getmap_strategy == GEOCACHE_GETMAP_ERROR) {
      ctx->set_error(ctx, 404, "full wms support disabled");
      return NULL;
   } else if(req_map->getmap_strategy == GEOCACHE_GETMAP_ASSEMBLE) {
      int i;
      geocache_map *basemap = req_map->maps[0];
      geocache_image *baseim = _core_get_single_map(ctx,basemap,req_map->resample_mode);
      if(GC_HAS_ERROR(ctx)) return NULL;
      for(i=1;i<req_map->nmaps;i++) {
         geocache_map *overlaymap = req_map->maps[i];
         geocache_image *overlayim = _core_get_single_map(ctx,overlaymap,req_map->resample_mode); 
         if(GC_HAS_ERROR(ctx)) return NULL;
         geocache_image_merge(ctx,baseim,overlayim);
         if(GC_HAS_ERROR(ctx)) return NULL;
         if(overlaymap->mtime > basemap->mtime) basemap->mtime = overlaymap->mtime;
         if(!basemap->expires || overlaymap->expires<basemap->expires) basemap->expires = overlaymap->expires;

      }
      basemap->data = format->write(ctx,baseim,format);
      return basemap;
   } else /*if(ctx->config->getmap_strategy == GEOCACHE_GETMAP_FORWARD)*/ {
      int i;
      geocache_map *basemap = req_map->maps[0];
      basemap->tileset->source->render_map(ctx, basemap);
      if(GC_HAS_ERROR(ctx)) return NULL;
      if(req_map->nmaps>1) {
         geocache_image *baseim = geocache_imageio_decode(ctx,basemap->data);
         if(GC_HAS_ERROR(ctx)) return NULL;
         for(i=1;i<req_map->nmaps;i++) {
            geocache_map *overlaymap = req_map->maps[i];
            overlaymap->tileset->source->render_map(ctx, overlaymap);
            if(GC_HAS_ERROR(ctx)) return NULL;
            geocache_image *overlayim = geocache_imageio_decode(ctx,overlaymap->data); 
            if(GC_HAS_ERROR(ctx)) return NULL;
            geocache_image_merge(ctx,baseim,overlayim);
            if(GC_HAS_ERROR(ctx)) return NULL;
         }
         basemap->data = format->write(ctx,baseim,format);
      }
      return basemap;
   }
}
geocache_proxied_response *geocache_core_proxy_request(geocache_context *ctx, geocache_request_proxy *req_proxy) {
   geocache_proxied_response *response = (geocache_proxied_response*) apr_pcalloc(ctx->pool,
         sizeof(geocache_proxied_response));

    response->data = geocache_buffer_create(30000,ctx->pool);
    response->headers = apr_table_make(ctx->pool,1);
    geocache_http *http = req_proxy->http;
    if(req_proxy->pathinfo) {
      http = geocache_http_clone(ctx,http);
      http->url = apr_pstrcat(ctx->pool,http->url,req_proxy->pathinfo,NULL);
    }
    geocache_http_do_request_with_params(ctx,http,req_proxy->params,response->data,response->headers);
    return response;
}

geocache_feature_info *geocache_core_get_featureinfo(geocache_context *ctx,
      geocache_request_get_feature_info *req_fi) {
   geocache_feature_info *fi = req_fi->fi;
   geocache_tileset *tileset = fi->map.tileset;
   if(tileset->source->info_formats) {
      int i;
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
      return fi;
   } else {
      ctx->set_error(ctx,404, "tileset %s does not support feature info requests");
      return NULL;
   }
}


/* vim: ai ts=3 sts=3 et sw=3
*/

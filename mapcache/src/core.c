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

#include "geocache.h"

geocache_tile *geocache_core_get_tile(geocache_context *ctx, geocache_request_get_tile *req_tile) {
   geocache_tile *rettile = NULL;
   int i;
   for(i=0;i<req_tile->ntiles;i++) {
      geocache_tile *tile = req_tile->tiles[i];
      geocache_tileset_tile_get(ctx, tile);
      if(GC_HAS_ERROR(ctx))
         return NULL;
      if(req_tile->ntiles == 1) {
         rettile = req_tile->tiles[0];
      } else {
         rettile = (geocache_tile*)geocache_image_merge_tiles(ctx,ctx->config->merge_format,
               req_tile->tiles,req_tile->ntiles);
         if(GC_HAS_ERROR(ctx))
            return NULL;
         rettile->tileset = req_tile->tiles[0]->tileset;
      }
   }
   return rettile;
}

geocache_image* _core_get_single_map(geocache_context *ctx, geocache_map *map) {

   geocache_tile **maptiles;
   int i,nmaptiles;
   geocache_tileset_get_map_tiles(ctx,map->tileset,map->grid_link,
         map->extent, map->width, map->height,
         &nmaptiles, &maptiles);
   for(i=0;i<nmaptiles;i++) {
      geocache_tile *tile = maptiles[i];
      geocache_tileset_tile_get(ctx, tile);
      if(GC_HAS_ERROR(ctx))
         return NULL;
   }
   geocache_image *getmapim = geocache_tileset_assemble_map_tiles(ctx,map->tileset,map->grid_link,
         map->extent, map->width, map->height,
         nmaptiles, maptiles);
   return getmapim;
}

geocache_map *geocache_core_get_map(geocache_context *ctx, geocache_request_get_map *req_map) {
#ifdef DEBUG
   if(req_map->nmaps ==0) {
      ctx->set_error(ctx,500,"BUG: get_map called with 0 maps");
      return NULL;
   }
#endif

   if(ctx->config->getmap_strategy == GEOCACHE_GETMAP_ERROR) {
      ctx->set_error(ctx, 404, "full wms support disabled");
      return NULL;
   } else if(ctx->config->getmap_strategy == GEOCACHE_GETMAP_ASSEMBLE) {
#ifdef USE_CAIRO
      int i;
      geocache_map *basemap = req_map->maps[0];
      geocache_image *baseim = _core_get_single_map(ctx,basemap);
      if(GC_HAS_ERROR(ctx)) return NULL;
      for(i=1;i<req_map->nmaps;i++) {
         geocache_map *overlaymap = req_map->maps[i];
         geocache_image *overlayim = _core_get_single_map(ctx,overlaymap); 
         if(GC_HAS_ERROR(ctx)) return NULL;
         geocache_image_merge(ctx,baseim,overlayim);
         if(GC_HAS_ERROR(ctx)) return NULL;
      }

      basemap->data = basemap->tileset->format->write(ctx,baseim,basemap->tileset->format);
      return basemap;
#else
      ctx->set_error(ctx,501,"cairo not enabled: wms getmap handling by assembling tiles not configured in this build");
      return NULL;
#endif
   } else /*if(ctx->config->getmap_strategy == GEOCACHE_GETMAP_FORWARD)*/ {
      int i;
      geocache_map *basemap = req_map->maps[0];
      basemap->tileset->source->render_map(ctx, basemap);
      if(GC_HAS_ERROR(ctx)) return NULL;
      if(i>1) {
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
         basemap->data = basemap->tileset->format->write(ctx,baseim,basemap->tileset->format);
      }
      return basemap;
   }
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

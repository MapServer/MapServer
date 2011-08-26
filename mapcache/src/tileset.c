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
#include <apr_strings.h>
#include <math.h>

static double _geocache_tileset_get_resolution(geocache_tileset *tileset, double *bbox) {
   double rx = (bbox[2] - bbox[0]) / (double)tileset->grid->tile_sx;
   double ry = (bbox[3] - bbox[1]) / (double)tileset->grid->tile_sy;
   return GEOCACHE_MAX(rx,ry);
}

void geocache_tileset_get_level(geocache_context *ctx, geocache_tileset *tileset, double *resolution, int *level) {
   double max_diff = *resolution / (double)GEOCACHE_MAX(tileset->grid->tile_sx, tileset->grid->tile_sy);
   int i;
   for(i=0; i<tileset->grid->levels; i++) {
      if(fabs(tileset->grid->resolutions[i] - *resolution) < max_diff) {
         *resolution = tileset->grid->resolutions[i];
         *level = i;
         return;
      }
   }
   ctx->set_error(ctx, GEOCACHE_TILESET_ERROR, "tileset %s: failed lookup for resolution %f", tileset->name, *resolution);
}

void geocache_tileset_tile_validate(geocache_context *ctx, geocache_tile *tile) {
   if(tile->z < 0 || tile->z >= tile->tileset->grid->levels) {
      ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR,"invalid tile z level");
      return;
   }
   /* TODO: check restricted extent when implemented */
}

/*
 * update the tile by setting it's x,y,z value given a bbox.
 * will return GEOCACHE_TILESET_WRONG_RESOLUTION or GEOCACHE_TILESET_WRONG_EXTENT
 * if the bbox does not correspond to the tileset's configuration
 */
static void _geocache_tileset_tile_get_cell(geocache_context *ctx, geocache_tile *tile, double *bbox) {
   double res = _geocache_tileset_get_resolution(tile->tileset,bbox);
   geocache_tileset_get_level(ctx, tile->tileset, &res, &(tile->z));
   GC_CHECK_ERROR(ctx);
   /* TODO: strict mode
           if exact and self.extent_type == "strict" and not self.contains((minx, miny), res):
               raise TileCacheException("Lower left corner (%f, %f) is outside layer bounds %s. \nTo remove this condition, set extent_type=loose in your configuration."
                        % (minx, miny, self.bbox))
               return None
    */
   tile->x = (int)round((bbox[0] - tile->tileset->grid->extents[tile->z][0]) / (res * tile->tileset->grid->tile_sx));
   tile->y = (int)round((bbox[1] - tile->tileset->grid->extents[tile->z][1]) / (res * tile->tileset->grid->tile_sy));

   if((fabs(bbox[0] - (tile->x * res * tile->tileset->grid->tile_sx) - tile->tileset->grid->extents[tile->z][0] ) / res > 1) ||
         (fabs(bbox[1] - (tile->y * res * tile->tileset->grid->tile_sy) - tile->tileset->grid->extents[tile->z][1] ) / res > 1)) {
      ctx->set_error(ctx, GEOCACHE_TILESET_ERROR, "tileset %s: supplied bbox not aligned on configured grid",tile->tileset->name);
   }
}

void geocache_tileset_get_xy(geocache_context *ctx, geocache_tileset *tileset, double dx, double dy,
        int z, int *x, int *y) {
    double res = tileset->grid->resolutions[z];
    *x = (int)((dx - tileset->grid->extents[z][0]) / (res * tileset->grid->tile_sx));
    *y = (int)((dy - tileset->grid->extents[z][1]) / (res * tileset->grid->tile_sy));
}

/*
 * for each of the metatile's tiles, ask the underlying cache to lock it
 */
void _geocache_tileset_metatile_lock(geocache_context *ctx, geocache_metatile *mt) {
   int i;
   for(i=0; i<mt->ntiles; i++) {
      geocache_tile *tile = &(mt->tiles[i]);
      geocache_tileset_tile_lock(ctx, tile);
      if(GC_HAS_ERROR(ctx)) {
         /* undo successful locks */
         int j;
         for(j=0;j<i;j++) {
            tile = &(mt->tiles[j]);
            geocache_tileset_tile_unlock(ctx,tile);
         }
         return;
      }
   }
}

/*
 * for each of the metatile's tiles, ask the underlying cache to unlock it
 */
void _geocache_tileset_metatile_unlock(geocache_context *ctx, geocache_metatile *mt) {
   int i;
   for(i=0; i<mt->ntiles; i++) {
      geocache_tile *tile = &(mt->tiles[i]);
      geocache_tileset_tile_unlock(ctx, tile);
   }
}

/*
 * compute the metatile that should be rendered for the given tile
 */
static geocache_metatile* _geocache_tileset_metatile_get(geocache_context *ctx, geocache_tile *tile) {
   geocache_metatile *mt = (geocache_metatile*)apr_pcalloc(ctx->pool, sizeof(geocache_metatile));
   int i,j,blx,bly;
   double res = tile->tileset->grid->resolutions[tile->z];
   double gbuffer,gwidth,gheight;
   mt->tile.tileset = tile->tileset;
   mt->ntiles = mt->tile.tileset->metasize_x * mt->tile.tileset->metasize_y;
   mt->tiles = (geocache_tile*)apr_pcalloc(ctx->pool, mt->ntiles * sizeof(geocache_tile));
   mt->tile.sx =  mt->tile.tileset->metasize_x * tile->sx + 2 * mt->tile.tileset->metabuffer;
   mt->tile.sy =  mt->tile.tileset->metasize_y * tile->sy + 2 * mt->tile.tileset->metabuffer;
   mt->tile.z = tile->z;
   mt->tile.x = tile->x / mt->tile.tileset->metasize_x;
   if(tile->x < 0)
      mt->tile.x --;
   mt->tile.y = tile->y / mt->tile.tileset->metasize_y;
   if(tile->y < 0)
      mt->tile.y --;

   //tilesize   = self.actualSize()
   gbuffer = res * mt->tile.tileset->metabuffer;
   gwidth = res * mt->tile.tileset->metasize_x * tile->sx;
   gheight = res * mt->tile.tileset->metasize_y * tile->sy;
   mt->bbox[0] = mt->tile.tileset->grid->extents[tile->z][0] + mt->tile.x * gwidth - gbuffer;
   mt->bbox[1] = mt->tile.tileset->grid->extents[tile->z][1] + mt->tile.y * gheight - gbuffer;
   mt->bbox[2] = mt->bbox[0] + gwidth + 2 * gbuffer;
   mt->bbox[3] = mt->bbox[1] + gheight + 2 * gbuffer;

   blx = mt->tile.x * mt->tile.tileset->metasize_x;
   bly = mt->tile.y * mt->tile.tileset->metasize_y;
   for(i=0; i<mt->tile.tileset->metasize_x; i++) {
      for(j=0; j<mt->tile.tileset->metasize_y; j++) {
         geocache_tile *t = &(mt->tiles[i*mt->tile.tileset->metasize_x+j]);
         t->z = tile->z;
         t->x = blx + i;
         t->y = bly + j;
         t->tileset = tile->tileset;
         t->sx = tile->sx;
         t->sy = tile->sy;
      }
   }

   return mt;
}

/*
 * do the actual rendering and saving of a metatile:
 *  - query the datasource for the image data
 *  - split the resulting image along the metabuffer / metatiles
 *  - save each tile to cache
 */
void _geocache_tileset_render_metatile(geocache_context *ctx, geocache_metatile *mt) {
   int i;
   mt->tile.tileset->source->render_metatile(ctx, mt);
   GC_CHECK_ERROR(ctx);
   geocache_image_metatile_split(ctx, mt);
   GC_CHECK_ERROR(ctx);
   for(i=0;i<mt->ntiles;i++) {
      geocache_tile *tile = &(mt->tiles[i]);
      mt->tile.tileset->cache->tile_set(ctx, tile);
      GC_CHECK_ERROR(ctx);
   }
}

/*
 * compute the bounding box of a given tile
 */
void geocache_tileset_tile_bbox(geocache_tile *tile, double *bbox) {
   double res  = tile->tileset->grid->resolutions[tile->z];
   bbox[0] = tile->tileset->grid->extents[tile->z][0] + (res * tile->x * tile->sx);
   bbox[1] = tile->tileset->grid->extents[tile->z][1] + (res * tile->y * tile->sy);
   bbox[2] = tile->tileset->grid->extents[tile->z][0] + (res * (tile->x + 1) * tile->sx);
   bbox[3] = tile->tileset->grid->extents[tile->z][1] + (res * (tile->y + 1) * tile->sy);
}

/*
 * allocate and initialize a new tileset
 */
geocache_tileset* geocache_tileset_create(geocache_context *ctx) {
   geocache_tileset* tileset = (geocache_tileset*)apr_pcalloc(ctx->pool, sizeof(geocache_tileset));
   tileset->metasize_x = tileset->metasize_y = 1;
   tileset->metabuffer = 0;
   tileset->expires = 0;
   tileset->metadata = apr_table_make(ctx->pool,3);
   tileset->dimensions = NULL;
   tileset->format = NULL;
   tileset->grid = NULL;
   tileset->config = NULL;
   return tileset;
}

/*
 * allocate and initialize a tile for a given tileset
 */
geocache_tile* geocache_tileset_tile_create(apr_pool_t *pool, geocache_tileset *tileset) {
   geocache_tile *tile = (geocache_tile*)apr_pcalloc(pool, sizeof(geocache_tile));
   tile->tileset = tileset;
   tile->expires = tileset->expires;
   tile->sx = tileset->grid->tile_sx;
   tile->sy = tileset->grid->tile_sy;
   return tile;
}


void geocache_tileset_tile_lookup(geocache_context *ctx, geocache_tile *tile, double *bbox) {
   if(tile->sx != tile->tileset->grid->tile_sx || tile->sy != tile->tileset->grid->tile_sy) {
      ctx->set_error(ctx, GEOCACHE_TILESET_ERROR, "tileset %s: wrong size. found %dx%d instead of %dx%d",
            tile->tileset->name,tile->sx,tile->sy,tile->tileset->grid->tile_sx,tile->tileset->grid->tile_sy);
      return;
   }
   _geocache_tileset_tile_get_cell(ctx, tile,bbox);
}

/**
 * \brief return the image data for a given tile
 * this call uses a global (interprocess+interthread) mutex if the tile was not found
 * in the cache.
 * the processing here is:
 *  - if the tile is found in the cache, return it. done
 *  - if it isn't found:
 *    - aquire mutex
 *    - check if the tile isn't being rendered by another thread/process
 *      - if another thread is rendering, wait for it to finish and return it's data
 *      - otherwise, lock all the tiles corresponding to the request (a metatile has multiple tiles)
 *    - release mutex
 *    - call the source to render the metatile, and save the tiles to disk
 *    - aquire mutex
 *    - unlock the tiles we have rendered
 *    - release mutex
 *  
 */
void geocache_tileset_tile_get(geocache_context *ctx, geocache_tile *tile) {
   int isLocked,ret;
   geocache_metatile *mt=NULL;
   if(tile->sx != tile->tileset->grid->tile_sx || tile->sy != tile->tileset->grid->tile_sy) {
      ctx->set_error(ctx, GEOCACHE_TILESET_ERROR, 
            "tileset %s: asked for a %dx%d tile from a %dx%d tileset",tile->tileset->name,
            tile->sx, tile->sy, tile->tileset->grid->tile_sx, tile->tileset->grid->tile_sy);
      return;
   }
   ret = tile->tileset->cache->tile_get(ctx, tile);
   GC_CHECK_ERROR(ctx);

   if(ret == GEOCACHE_CACHE_MISS) {
      /* the tile does not exist, we must take action before re-asking for it */

      /*
       * is the tile already being rendered by another thread ?
       * the call is protected by the same mutex that sets the lock on the tile,
       * so we can assure that:
       * - if the lock does not exist, then this thread should do the rendering
       * - if the lock exists, we should wait for the other thread to finish
       */

      ctx->global_lock_aquire(ctx);
      GC_CHECK_ERROR(ctx);

      isLocked = geocache_tileset_tile_lock_exists(ctx, tile);
      if(isLocked == GEOCACHE_FALSE) {
         /* no other thread is doing the rendering, we aquire and lock a list of tiles to render */
         mt = _geocache_tileset_metatile_get(ctx, tile);
         _geocache_tileset_metatile_lock(ctx, mt);
      }
      ctx->global_lock_release(ctx);
      if(GC_HAS_ERROR(ctx))
         return;

      if(isLocked == GEOCACHE_TRUE) {
         /* another thread is rendering the tile, we should wait for it to finish */
#ifdef DEBUG
         ctx->log(ctx, GEOCACHE_DEBUG, "cache wait: tileset %s - tile %d %d %d",
               tile->tileset->name,tile->x, tile->y,tile->z);
#endif
         geocache_tileset_tile_lock_wait(ctx,tile);
         GC_CHECK_ERROR(ctx);
      } else {
         /* no other thread is doing the rendering, do it ourselves */
#ifdef DEBUG
         ctx->log(ctx, GEOCACHE_DEBUG, "cache miss: tileset %s - tile %d %d %d",
               tile->tileset->name,tile->x, tile->y,tile->z);
#endif
         /* this will query the source to create the tiles, and save them to the cache */
         _geocache_tileset_render_metatile(ctx, mt);
         
         /* remove the lockfiles */
         ctx->global_lock_aquire(ctx);
         _geocache_tileset_metatile_unlock(ctx,mt);
         ctx->global_lock_release(ctx);
         GC_CHECK_ERROR(ctx);
      }
      
      /* the previous step has successfully finished, we can now query the cache to return the tile content */
      ret = tile->tileset->cache->tile_get(ctx, tile);
      if(ret != GEOCACHE_SUCCESS) {
         ctx->set_error(ctx, GEOCACHE_TILESET_ERROR, "tileset %s: failed to re-get tile %d %d %d from cache after set", tile->tileset->name,tile->x,tile->y,tile->z);
      }
   }
}

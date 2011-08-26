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


void geocache_tileset_tile_validate(geocache_context *ctx, geocache_tile *tile) {
   if(tile->z < 0 || tile->z >= tile->grid_link->grid->nlevels) {
      ctx->set_error(ctx,404,"invalid tile z level");
      return;
   }
   int *limits = tile->grid_link->grid_limits[tile->z];
   if(tile->x<limits[0] || tile->x>=limits[2]) {
      ctx->set_error(ctx, 404, "tile x=%d not in [%d,%d[",
            tile->x,limits[0],limits[2]);
      return;
   }
   if(tile->y<limits[1] || tile->y>=limits[3]) {
      ctx->set_error(ctx, 404, "tile y=%d not in [%d,%d[",
            tile->y,limits[1],limits[3]);
      return;
   }
}


void geocache_tileset_get_map_tiles(geocache_context *ctx, geocache_tileset *tileset,
      geocache_grid_link *grid_link,
      double *bbox, int width, int height,
      int *ntiles,
      geocache_tile ***tiles) {
   double resolution;
   int level;
   resolution = geocache_grid_get_resolution(bbox, width, height);
   geocache_grid_get_closest_level(ctx,grid_link->grid,resolution,&level);
   int mx,my,Mx,My;
   int x,y;
   geocache_grid_get_xy(ctx,grid_link->grid,bbox[0],bbox[1],level,&mx,&my);
   geocache_grid_get_xy(ctx,grid_link->grid,bbox[2],bbox[3],level,&Mx,&My);
   *ntiles = (Mx-mx+1)*(My-my+1);
   if(*ntiles<=0) {
      ctx->set_error(ctx,500,"BUG: negative number of tiles");
      return;
   }
   int i=0;
   *tiles = (geocache_tile**)apr_pcalloc(ctx->pool, *ntiles*sizeof(geocache_tile*));
   for(x=mx;x<=Mx;x++) {
      for(y=my;y<=My;y++) {
         geocache_tile *tile = geocache_tileset_tile_create(ctx->pool,tileset, grid_link);
         tile->x = x;
         tile->y = y;
         tile->z = level;
         geocache_tileset_tile_validate(ctx,tile);
         if(GC_HAS_ERROR(ctx)) {
            //clear the error message
            ctx->clear_errors(ctx);
         } else {
            (*tiles)[i++]=tile;
         }
      }
   }
   *ntiles = i;
}

#ifdef USE_CAIRO

#include <cairo/cairo.h>
#include <math.h>

geocache_image* geocache_tileset_assemble_map_tiles(geocache_context *ctx, geocache_tileset *tileset,
      geocache_grid_link *grid_link,
      double *bbox, int width, int height,
      int ntiles,
      geocache_tile **tiles) {
   double resolution = geocache_grid_get_resolution(bbox, width, height);
   double tilebbox[4];
   geocache_image *image = geocache_image_create(ctx);
   image->w = width;
   image->h = height;
   image->stride = width*4;
   image->data = apr_pcalloc(ctx->pool,width*height*4*sizeof(unsigned char));
   cairo_surface_t* dstsurface= cairo_image_surface_create_for_data(image->data, CAIRO_FORMAT_ARGB32,
         width, height,image->stride);
   cairo_t *cr = cr = cairo_create (dstsurface);

   int i;
   for(i=0;i<ntiles;i++) {
      geocache_tile *tile = tiles[i];
      double tileresolution = tile->grid_link->grid->levels[tile->z]->resolution;
      geocache_tileset_tile_bbox(tile,tilebbox);
      geocache_image *im = geocache_imageio_decode(ctx,tile->data);
      cairo_surface_t* srcsurface= cairo_image_surface_create_for_data(im->data, CAIRO_FORMAT_ARGB32,
            im->w, im->h,im->stride);
      /*compute the pixel position of top left corner*/
      double dstminx = floor((tilebbox[0]-bbox[0])/resolution);
      double dstminy = floor((bbox[3]-tilebbox[3])/resolution);
      double f = tileresolution/resolution;
      double dstwidth = ceil(im->w*f);
      f = dstwidth/(double)im->w;
      cairo_save(cr);
      //cairo_clip(cr);
      cairo_translate (cr, dstminx,dstminy);
      cairo_scale  (cr, f, f);
      cairo_set_source_surface (cr, srcsurface, 0, 0);
      cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
      cairo_paint (cr);
      cairo_restore(cr);
      cairo_surface_destroy(srcsurface);
   }
   cairo_surface_destroy(dstsurface);
   cairo_destroy(cr);
   return image;
}

#endif

/*
 * compute the metatile that should be rendered for the given tile
 */
static geocache_metatile* _geocache_tileset_metatile_get(geocache_context *ctx, geocache_tile *tile) {
   geocache_metatile *mt = (geocache_metatile*)apr_pcalloc(ctx->pool, sizeof(geocache_metatile));
   int i,j,blx,bly;
   geocache_tileset *tileset = tile->tileset;
   geocache_grid *grid = tile->grid_link->grid;
   double res = grid->levels[tile->z]->resolution;
   double gbuffer,gwidth,gheight;
   mt->map.tileset = tileset;
   mt->map.grid_link = tile->grid_link;
   mt->ntiles = tileset->metasize_x * tileset->metasize_y;
   mt->tiles = (geocache_tile*)apr_pcalloc(ctx->pool, mt->ntiles * sizeof(geocache_tile));
   mt->map.width =  tileset->metasize_x * grid->tile_sx + 2 * tileset->metabuffer;
   mt->map.height =  tileset->metasize_y * grid->tile_sy + 2 * tileset->metabuffer;
   mt->map.dimensions = tile->dimensions;
   mt->z = tile->z;
   mt->x = tile->x / tileset->metasize_x;
   if(tile->x < 0)
      mt->x --;
   mt->y = tile->y / tileset->metasize_y;
   if(tile->y < 0)
      mt->y --;

   //tilesize   = self.actualSize()
   gbuffer = res * tileset->metabuffer;
   gwidth = res * tileset->metasize_x * grid->tile_sx;
   gheight = res * tileset->metasize_y * grid->tile_sy;
   mt->map.extent[0] = grid->extent[0] + mt->x * gwidth - gbuffer;
   mt->map.extent[1] = grid->extent[1] + mt->y * gheight - gbuffer;
   mt->map.extent[2] = mt->map.extent[0] + gwidth + 2 * gbuffer;
   mt->map.extent[3] = mt->map.extent[1] + gheight + 2 * gbuffer;

   blx = mt->x * tileset->metasize_x;
   bly = mt->y * tileset->metasize_y;
   for(i=0; i<tileset->metasize_x; i++) {
      for(j=0; j<tileset->metasize_y; j++) {
         geocache_tile *t = &(mt->tiles[i*tileset->metasize_x+j]);
         t->dimensions = tile->dimensions;
         t->grid_link = tile->grid_link;
         t->z = tile->z;
         t->x = blx + i;
         t->y = bly + j;
         t->tileset = tile->tileset;
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
   mt->map.tileset->source->render_map(ctx, &mt->map);
   GC_CHECK_ERROR(ctx);
   geocache_image_metatile_split(ctx, mt);
   GC_CHECK_ERROR(ctx);
   for(i=0;i<mt->ntiles;i++) {
      geocache_tile *tile = &(mt->tiles[i]);
      mt->map.tileset->cache->tile_set(ctx, tile);
      GC_CHECK_ERROR(ctx);
   }
}

/*
 * compute the bounding box of a given tile
 */
void geocache_tileset_tile_bbox(geocache_tile *tile, double *bbox) {
   double res  = tile->grid_link->grid->levels[tile->z]->resolution;
   bbox[0] = tile->grid_link->grid->extent[0] + (res * tile->x * tile->grid_link->grid->tile_sx);
   bbox[1] = tile->grid_link->grid->extent[1] + (res * tile->y * tile->grid_link->grid->tile_sy);
   bbox[2] = tile->grid_link->grid->extent[0] + (res * (tile->x + 1) * tile->grid_link->grid->tile_sx);
   bbox[3] = tile->grid_link->grid->extent[1] + (res * (tile->y + 1) * tile->grid_link->grid->tile_sy);
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
   tileset->grid_links = NULL;
   tileset->config = NULL;
   return tileset;
}

/*
 * allocate and initialize a tile for a given tileset
 */
geocache_tile* geocache_tileset_tile_create(apr_pool_t *pool, geocache_tileset *tileset, geocache_grid_link *grid_link) {
   geocache_tile *tile = (geocache_tile*)apr_pcalloc(pool, sizeof(geocache_tile));
   tile->tileset = tileset;
   tile->expires = tileset->expires;
   tile->grid_link = grid_link;
   if(tileset->dimensions) {
      int i;
      tile->dimensions = apr_table_make(pool,tileset->dimensions->nelts);
      for(i=0;i<tileset->dimensions->nelts;i++) {
         geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
         apr_table_set(tile->dimensions,dimension->name,dimension->default_value);
      }
   }
   return tile;
}

/*
 * allocate and initialize a map for a given tileset
 */
geocache_map* geocache_tileset_map_create(apr_pool_t *pool, geocache_tileset *tileset, geocache_grid_link *grid_link) {
   geocache_map *map = (geocache_map*)apr_pcalloc(pool, sizeof(geocache_map));
   map->tileset = tileset;
   map->grid_link = grid_link;
   if(tileset->dimensions) {
      int i;
      map->dimensions = apr_table_make(pool,tileset->dimensions->nelts);
      for(i=0;i<tileset->dimensions->nelts;i++) {
         geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
         apr_table_set(map->dimensions,dimension->name,dimension->default_value);
      }
   }
   return map;
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
      
      mt = _geocache_tileset_metatile_get(ctx, tile);

      isLocked = geocache_tileset_metatile_lock_exists(ctx, mt);
      if(isLocked == GEOCACHE_FALSE) {
         /* no other thread is doing the rendering, we aquire and lock the metatile */
         geocache_tileset_metatile_lock(ctx, mt);
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
         geocache_tileset_metatile_lock_wait(ctx,mt);
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
         geocache_tileset_metatile_unlock(ctx,mt);
         ctx->global_lock_release(ctx);
         GC_CHECK_ERROR(ctx);
      }
      
      /* the previous step has successfully finished, we can now query the cache to return the tile content */
      ret = tile->tileset->cache->tile_get(ctx, tile);
      if(ret != GEOCACHE_SUCCESS) {
         ctx->set_error(ctx, 500, "tileset %s: failed to re-get tile %d %d %d from cache after set", tile->tileset->name,tile->x,tile->y,tile->z);
      }
   }
}
/* vim: ai ts=3 sts=3 et sw=3
*/

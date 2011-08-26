/*
 * tileset.c
 *
 *  Created on: Oct 11, 2010
 *      Author: tom
 */

#include "geocache.h"
#include <apr_strings.h>
#include <http_log.h>
#include <math.h>

static double _geocache_tileset_get_resolution(geocache_tileset *tileset, double *bbox) {
   double rx = (bbox[2] - bbox[0]) / (double)tileset->tile_sx;
   double ry = (bbox[3] - bbox[1]) / (double)tileset->tile_sy;
   return GEOCACHE_MAX(rx,ry);
}

static int _geocache_tileset_get_level(geocache_tileset *tileset, double *resolution, int *level, request_rec *r) {
   double max_diff = *resolution / (double)GEOCACHE_MAX(tileset->tile_sx, tileset->tile_sy);
   int i;
   for(i=0; i<tileset->levels; i++) {
      if(fabs(tileset->resolutions[i] - *resolution) < max_diff) {
         *resolution = tileset->resolutions[i];
         *level = i;
         return GEOCACHE_SUCCESS;
      }
   }
   ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "tileset %s: failed lookup for resolution %f", tileset->name, *resolution);
   return GEOCACHE_TILESET_WRONG_RESOLUTION;
}

/*
 * update the tile by setting it's x,y,z value given a bbox.
 * will return GEOCACHE_TILESET_WRONG_RESOLUTION or GEOCACHE_TILESET_WRONG_EXTENT
 * if the bbox does not correspond to the tileset's configuration
 */
static int _geocache_tileset_tile_get_cell(geocache_tile *tile, double *bbox, request_rec *r) {
   int ret;
   double res = _geocache_tileset_get_resolution(tile->tileset,bbox);
   ret = _geocache_tileset_get_level(tile->tileset,&res,&(tile->z),r);
   if(ret != GEOCACHE_SUCCESS) return ret;
   /* TODO: strict mode
           if exact and self.extent_type == "strict" and not self.contains((minx, miny), res):
               raise TileCacheException("Lower left corner (%f, %f) is outside layer bounds %s. \nTo remove this condition, set extent_type=loose in your configuration."
                        % (minx, miny, self.bbox))
               return None
    */
   tile->x = (int)((bbox[0] - tile->tileset->extent[0]) / (res * tile->tileset->tile_sx));
   tile->y = (int)((bbox[1] - tile->tileset->extent[1]) / (res * tile->tileset->tile_sy));

   if((fabs(bbox[0] - (tile->x * res * tile->tileset->tile_sx) - tile->tileset->extent[0] ) / res > 1) ||
         (fabs(bbox[1] - (tile->y * res * tile->tileset->tile_sy) - tile->tileset->extent[1] ) / res > 1)) {
      ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "tileset %s: supplied bbox not aligned on configured grid",tile->tileset->name);
      return GEOCACHE_TILESET_WRONG_EXTENT;
   }
   return GEOCACHE_SUCCESS;
}

/*
 * for each of the metatile's tiles, ask the underlying cache to lock it
 */
void _geocache_tileset_metatile_lock(geocache_metatile *mt, request_rec *r) {
   int i;
   for(i=0; i<mt->ntiles; i++) {
      geocache_tile *tile = &(mt->tiles[i]);
      mt->tile.tileset->cache->tile_lock(tile,r);
   }
}

/*
 * for each of the metatile's tiles, ask the underlying cache to unlock it
 */
void _geocache_tileset_metatile_unlock(geocache_metatile *mt, request_rec *r) {
   int i;
   for(i=0; i<mt->ntiles; i++) {
      geocache_tile *tile = &(mt->tiles[i]);
      mt->tile.tileset->cache->tile_unlock(tile,r);
   }
}

/*
 * compute the metatile that should be rendered for the given tile
 */
static geocache_metatile* _geocache_tileset_metatile_get(geocache_tile *tile, request_rec *r) {
   geocache_metatile *mt = (geocache_metatile*)apr_pcalloc(r->pool, sizeof(geocache_metatile));
   int i,j,blx,bly;
   double res = tile->tileset->resolutions[tile->z];
   double gbuffer,gwidth,gheight;
   mt->tile.tileset = tile->tileset;
   mt->ntiles = mt->tile.tileset->metasize_x * mt->tile.tileset->metasize_y;
   mt->tiles = (geocache_tile*)apr_pcalloc(r->pool, mt->ntiles * sizeof(geocache_tile));
   mt->tile.sx =  mt->tile.tileset->metasize_x * tile->sx + 2 * mt->tile.tileset->metabuffer;
   mt->tile.sy =  mt->tile.tileset->metasize_y * tile->sy + 2 * mt->tile.tileset->metabuffer;
   mt->tile.z = tile->z;
   mt->tile.x = tile->x / mt->tile.tileset->metasize_x;
   mt->tile.y = tile->y / mt->tile.tileset->metasize_y;

   //tilesize   = self.actualSize()
   gbuffer = res * mt->tile.tileset->metabuffer;
   gwidth = res * mt->tile.tileset->metasize_x * tile->sx;
   gheight = res * mt->tile.tileset->metasize_y * tile->sy;
   mt->bbox[0] = mt->tile.tileset->extent[0] + mt->tile.x * gwidth - gbuffer;
   mt->bbox[1] = mt->tile.tileset->extent[1] + mt->tile.y * gheight - gbuffer;
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
int _geocache_tileset_render_metatile(geocache_metatile *mt, request_rec *r) {
   int i;
   int ret;
   ret = mt->tile.tileset->source->render_metatile(mt, r);
   if(ret != GEOCACHE_SUCCESS) { return ret; }
   ret = geocache_image_metatile_split(mt,r);
   if(ret != GEOCACHE_SUCCESS) { return ret; }
   for(i=0;i<mt->ntiles;i++) {
      geocache_tile *tile = &(mt->tiles[i]);
      ret = mt->tile.tileset->cache->tile_set(tile, r);
      if(ret != GEOCACHE_SUCCESS) { return ret; }
   }
   return GEOCACHE_SUCCESS;
}

/*
 * compute the bounding box of a given tile
 */
void geocache_tileset_tile_bbox(geocache_tile *tile, double *bbox) {
   double res  = tile->tileset->resolutions[tile->z];
   bbox[0] = tile->tileset->extent[0] + (res * tile->x * tile->sx);
   bbox[1] = tile->tileset->extent[1] + (res * tile->y * tile->sy);
   bbox[2] = tile->tileset->extent[0] + (res * (tile->x + 1) * tile->sx);
   bbox[3] = tile->tileset->extent[1] + (res * (tile->y + 1) * tile->sy);
}

/*
 * allocate and initialize a new tileset
 */
geocache_tileset* geocache_tileset_create(apr_pool_t *pool) {
   geocache_tileset* tileset = (geocache_tileset*)apr_pcalloc(pool, sizeof(geocache_tileset));
   tileset->metasize_x = tileset->metasize_y = 1;
   tileset->metabuffer = 0;
   tileset->tile_sx = tileset->tile_sy = 256;
   tileset->extent[0]=tileset->extent[1]=tileset->extent[2]=tileset->extent[3]=0;
   tileset->forwarded_params = apr_table_make(pool,1);
   tileset->format = GEOCACHE_IMAGE_FORMAT_PNG;
   return tileset;
}

/*
 * allocate and initialize a tile for a given tileset
 */
geocache_tile* geocache_tileset_tile_create(geocache_tileset *tileset, apr_pool_t *pool) {
   geocache_tile *tile = (geocache_tile*)apr_pcalloc(pool, sizeof(geocache_tile));
   tile->tileset = tileset;
   tile->sx = tileset->tile_sx;
   tile->sy = tileset->tile_sy;
   return tile;
}

/*
 * compute a tile's x,y and z value given a BBOX.
 */
int geocache_tileset_tile_lookup(geocache_tile *tile, double *bbox, request_rec *r) {
   if(tile->sx != tile->tileset->tile_sx || tile->sy != tile->tileset->tile_sy) {
      ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "tileset %s: wrong size. found %dx%d instead of %dx%d",
            tile->tileset->name,tile->sx,tile->sy,tile->tileset->tile_sx,tile->tileset->tile_sy);
      return GEOCACHE_TILESET_WRONG_SIZE;
   }
   return _geocache_tileset_tile_get_cell(tile,bbox,r);
}

/*
 * return the image data for a given tile
 * this call uses a global (interprocess+interthread) mutex if the tile was not found
 * in the cache.
 * the processing here is:
 *  - if the tile is found in the cache, return it. done
 *  - if it isn't found:
 *    * aquire mutex
 *    * check if the tile isn't being rendered by another thread/process
 *      - if another thread is rendering, wait for it to finish and return it's data
 *      - otherwise, lock all the tiles corresponding to the request (a metatile has multiple tiles)
 *    * release mutex
 *    * call the source to render the metatile, and save the tiles to disk
 *    * aquire mutex
 *    * unlock the tiles we have rendered
 *    * release mutex
 *  
 */
int geocache_tileset_tile_get(geocache_tile *tile, request_rec *r) {
   int ret;
   int isLocked;
   geocache_metatile *mt;
   if(tile->sx != tile->tileset->tile_sx || tile->sy != tile->tileset->tile_sy) {
      ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
            "tileset %s: asked for a %dx%d tile from a %dx%d tileset",tile->tileset->name,
            tile->sx, tile->sy, tile->tileset->tile_sx, tile->tileset->tile_sy);
      return GEOCACHE_FAILURE;
   }
   ret = tile->tileset->cache->tile_get(tile, r);
   if(ret == GEOCACHE_CACHE_MISS) {
      /* the tile does not exist, we must take action before re-asking for it */

      /*
       * is the tile already being rendered by another thread ?
       * the call is protected by the same mutex that sets the lock on the tile,
       * so we can assure that:
       * - if the lock does not exist, then this thread should do the rendering
       * - if the lock exists, we should wait for the other thread to finish
       */
      ret = geocache_util_mutex_aquire(r);
      if(ret != GEOCACHE_SUCCESS) return ret;

      isLocked = tile->tileset->cache->tile_lock_exists(tile,r);
      if(isLocked == GEOCACHE_FALSE) {
         /* no other thread is doing the rendering, we aquire and lock a list of tiles to render */
         mt = _geocache_tileset_metatile_get(tile,r);
         _geocache_tileset_metatile_lock(mt,r);
      }

      ret = geocache_util_mutex_release(r);
      if(ret != GEOCACHE_SUCCESS) return ret;

      if(isLocked == GEOCACHE_TRUE) {
         /* another thread is rendering the tile, we should wait for it to finish */
         ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,"cache wait: tileset %s - tile %d %d %d",
               tile->tileset->name,tile->x, tile->y,tile->z);
         tile->tileset->cache->tile_lock_wait(tile,r);
      } else {
         /* no other thread is doing the rendering, do it ourselves */
         ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,"cache miss: tileset %s - tile %d %d %d",
               tile->tileset->name,tile->x, tile->y,tile->z);
         
         /* this will query the source to create the tiles, and save them to the cache */
         ret = _geocache_tileset_render_metatile(mt,r);
         
         /* remove the lockfiles */
         geocache_util_mutex_aquire(r);
         _geocache_tileset_metatile_unlock(mt,r);
         geocache_util_mutex_release(r);

         if(ret != GEOCACHE_SUCCESS) return ret;
      }
      
      /* the previous step has successfully finished, we can now query the cache to return the tile content */
      ret = tile->tileset->cache->tile_get(tile, r);
      if(ret != GEOCACHE_SUCCESS) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
               "tileset %s: failed to re-get tile from cache after set", tile->tileset->name);
         return ret;
      }
      return GEOCACHE_SUCCESS;
   } else {
      return ret;
   }
}

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
#include <apr_file_info.h>
#include <apr_file_io.h>
#include <math.h>

void geocache_tileset_configuration_check(geocache_context *ctx, geocache_tileset *tileset) {
   
   /* check we have all we want */
   if(tileset->cache == NULL) {
      /* TODO: we should allow tilesets with no caches */
      ctx->set_error(ctx, 400, "tileset \"%s\" has no cache configured.", tileset->name);
      return;
   }

   if(apr_is_empty_array(tileset->grid_links)) {
      ctx->set_error(ctx, 400, "tileset \"%s\" has no grids configured", tileset->name);
      return;
   } 
#ifdef USE_PROJ
   /* not implemented yet, will be used to automatically calculate wgs84bbox with proj */
   else if(tileset->wgs84bbox[0]>=tileset->wgs84bbox[2]) {
      geocache_grid_link *sgrid = APR_ARRAY_IDX(tileset->grid_links,0,geocache_grid_link*);
      double *extent = sgrid->grid->extent;
      if(sgrid->restricted_extent) {
         extent = sgrid->restricted_extent;
      }
   }
#endif
   if(!tileset->format && tileset->source && tileset->source->type == GEOCACHE_SOURCE_GDAL) {
      ctx->set_error(ctx,400, "tileset \"%s\" references a gdal source. <format> tag is missing and mandatory in this case",
            tileset->name);
      return;
   }

   if(tileset->metabuffer < 0 || tileset->metasize_x < 1 || tileset->metasize_y < 1) {
      ctx->set_error(ctx,400,"tileset \"%s\" has invalid metasize %d,%d or metabuffer %d",
            tileset->name,tileset->metasize_x,tileset->metasize_y,tileset->metabuffer);
      return;
   }

   if(!tileset->format && (
         tileset->metasize_x != 1 ||
         tileset->metasize_y != 1 ||
         tileset->metabuffer != 0 ||
         tileset->watermark)) {
       if(tileset->watermark) {
          ctx->set_error(ctx,400,"tileset \"%s\" has no <format> configured, but it is needed for the watermark",
                   tileset->name);
          return;
       } else {
          ctx->set_error(ctx,400,"tileset \"%s\" has no <format> configured, but it is needed for metatiling",
                   tileset->name);
          return;
       }
   }
}

void geocache_tileset_add_watermark(geocache_context *ctx, geocache_tileset *tileset, const char *filename) {
   apr_file_t *f;
   apr_finfo_t finfo;
   int rv;
   if(apr_file_open(&f, filename, APR_FOPEN_READ|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
            APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
      ctx->set_error(ctx,500, "failed to open watermark image %s",filename);
      return;
   }
   rv = apr_file_info_get(&finfo, APR_FINFO_SIZE, f);
   if(!finfo.size) {
      ctx->set_error(ctx, 500, "watermark %s has no data",filename);
      return;
   }

   geocache_buffer *watermarkdata = geocache_buffer_create(finfo.size,ctx->pool);
   //manually add the data to our buffer
   apr_size_t size = finfo.size;
   apr_file_read(f,watermarkdata->buf,&size);
   watermarkdata->size = size;
   if(size != finfo.size) {
      ctx->set_error(ctx, 500,  "failed to copy watermark image data, got %d of %d bytes",(int)size, (int)finfo.size);
      return;
   }
   apr_file_close(f);
   tileset->watermark = geocache_imageio_decode(ctx,watermarkdata);
}

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

geocache_image* geocache_tileset_assemble_map_tiles(geocache_context *ctx, geocache_tileset *tileset,
      geocache_grid_link *grid_link,
      double *bbox, int width, int height,
      int ntiles,
      geocache_tile **tiles,
      geocache_resample_mode mode) {
   double hresolution = geocache_grid_get_horizontal_resolution(bbox, width);
   double vresolution = geocache_grid_get_vertical_resolution(bbox, height);
   double tilebbox[4];
   geocache_tile *toplefttile=NULL;
   int mx=INT_MAX,my=INT_MAX,Mx=INT_MIN,My=INT_MIN;
   int i;
   geocache_image *image = geocache_image_create(ctx);
   image->w = width;
   image->h = height;
   image->stride = width*4;
   image->data = calloc(1,width*height*4*sizeof(unsigned char));
   apr_pool_cleanup_register(ctx->pool, image->data, (void*)free, apr_pool_cleanup_null) ;
   if(ntiles == 0) {
      return image;
   }

   /* compute the number of tiles horizontally and vertically */
   for(i=0;i<ntiles;i++) {
      geocache_tile *tile = tiles[i];
      if(tile->x < mx) mx = tile->x;
      if(tile->y < my) my = tile->y;
      if(tile->x > Mx) Mx = tile->x;
      if(tile->y > My) My = tile->y;
   }
   /* create image that will contain the unscaled tiles data */
   geocache_image *srcimage = geocache_image_create(ctx);
   srcimage->w = (Mx-mx+1)*tiles[0]->grid_link->grid->tile_sx;
   srcimage->h = (My-my+1)*tiles[0]->grid_link->grid->tile_sy;
   srcimage->stride = srcimage->w*4;
   srcimage->data = calloc(1,srcimage->w*srcimage->h*4*sizeof(unsigned char));
   apr_pool_cleanup_register(ctx->pool, srcimage->data, (void*)free, apr_pool_cleanup_null) ;

   /* copy the tiles data into the src image */
   for(i=0;i<ntiles;i++) {
      geocache_tile *tile = tiles[i];
      if(tile->x == mx && tile->y == My) {
         toplefttile = tile;
      }
      int ox,oy; /* the offset from the start of the src image to the start of the tile */
      ox = (tile->x - mx) * tile->grid_link->grid->tile_sx;
      oy = (My - tile->y) * tile->grid_link->grid->tile_sy;
      geocache_image fakeimg;
      fakeimg.stride = srcimage->stride;
      fakeimg.data = &(srcimage->data[oy*srcimage->stride+ox*4]);
      geocache_imageio_decode_to_image(ctx,tile->data,&fakeimg);
   }

   assert(toplefttile);

   /* copy/scale the srcimage onto the destination image */
   double tileresolution = toplefttile->grid_link->grid->levels[toplefttile->z]->resolution;
   geocache_grid_get_extent(ctx,toplefttile->grid_link->grid,
         toplefttile->x, toplefttile->y, toplefttile->z, tilebbox);
   
   /*compute the pixel position of top left corner*/
   double dstminx = (tilebbox[0]-bbox[0])/hresolution;
   double dstminy = (bbox[3]-tilebbox[3])/vresolution;
   double hf = tileresolution/hresolution;
   double vf = tileresolution/vresolution;
   if(fabs(hf-1)<0.0001 && fabs(vf-1)<0.0001) {
      //use nearest resampling if we are at the resolution of the tiles
      geocache_image_copy_resampled_nearest(ctx,srcimage,image,dstminx,dstminy,hf,vf);
   } else {
      switch(mode) {
         case GEOCACHE_RESAMPLE_BILINEAR:
            geocache_image_copy_resampled_bilinear(ctx,srcimage,image,dstminx,dstminy,hf,vf);
            break;
         default:
            geocache_image_copy_resampled_nearest(ctx,srcimage,image,dstminx,dstminy,hf,vf);
            break;
      }
   }
   return image;
}

/*
 * compute the metatile that should be rendered for the given tile
 */
geocache_metatile* geocache_tileset_metatile_get(geocache_context *ctx, geocache_tile *tile) {
   geocache_metatile *mt = (geocache_metatile*)apr_pcalloc(ctx->pool, sizeof(geocache_metatile));
   int i,j,blx,bly;
   geocache_tileset *tileset = tile->tileset;
   geocache_grid *grid = tile->grid_link->grid;
   double res = grid->levels[tile->z]->resolution;
   double gbuffer,gwidth,gheight,fullgwidth,fullgheight;
   mt->map.tileset = tileset;
   mt->map.grid_link = tile->grid_link;
   mt->z = tile->z;
   mt->x = tile->x / tileset->metasize_x;
   if(tile->x < 0)
      mt->x --;
   mt->y = tile->y / tileset->metasize_y;
   if(tile->y < 0)
      mt->y --;
   blx = mt->x * tileset->metasize_x;
   bly = mt->y * tileset->metasize_y;
  
   /* adjust the size of the the metatile so it does not extend past the grid limits.
    * If we don't do this, we end up with cut labels on the edges of the tile grid
    */
   if(blx+tileset->metasize_x-1 >= grid->levels[tile->z]->maxx) {
      mt->metasize_x = grid->levels[tile->z]->maxx - blx;
   } else {
      mt->metasize_x = tileset->metasize_x;
   }
   if(bly+tileset->metasize_y-1 >= grid->levels[tile->z]->maxy) {
      mt->metasize_y = grid->levels[tile->z]->maxy - bly;
   } else {
      mt->metasize_y = tileset->metasize_y;
   }

   mt->ntiles = mt->metasize_x * mt->metasize_y;
   mt->tiles = (geocache_tile*)apr_pcalloc(ctx->pool, mt->ntiles * sizeof(geocache_tile));
   mt->map.width =  mt->metasize_x * grid->tile_sx + 2 * tileset->metabuffer;
   mt->map.height =  mt->metasize_y * grid->tile_sy + 2 * tileset->metabuffer;
   mt->map.dimensions = tile->dimensions;

   //tilesize   = self.actualSize()
   gbuffer = res * tileset->metabuffer;
   gwidth = res * mt->metasize_x * grid->tile_sx;
   gheight = res * mt->metasize_y * grid->tile_sy;
   fullgwidth = res * tileset->metasize_x * grid->tile_sx;
   fullgheight = res * tileset->metasize_y * grid->tile_sy;
   mt->map.extent[0] = grid->extent[0] + mt->x * fullgwidth - gbuffer;
   mt->map.extent[1] = grid->extent[1] + mt->y * fullgheight - gbuffer;
   mt->map.extent[2] = mt->map.extent[0] + gwidth + 2 * gbuffer;
   mt->map.extent[3] = mt->map.extent[1] + gheight + 2 * gbuffer;

   for(i=0; i<mt->metasize_x; i++) {
      for(j=0; j<mt->metasize_y; j++) {
         geocache_tile *t = &(mt->tiles[i*mt->metasize_y+j]);
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
#ifdef DEBUG
   if(!mt->map.tileset->source) {
      ctx->set_error(ctx,500,"###BUG### tileset_render_metatile called on tileset with no source");
      return;
   }
#endif
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
 * allocate and initialize a new tileset
 */
geocache_tileset* geocache_tileset_create(geocache_context *ctx) {
   geocache_tileset* tileset = (geocache_tileset*)apr_pcalloc(ctx->pool, sizeof(geocache_tileset));
   tileset->metasize_x = tileset->metasize_y = 1;
   tileset->metabuffer = 0;
   tileset->expires = 300; /*set a reasonable default to 5 mins */
   tileset->auto_expire = 0;
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
   if(tileset->auto_expire) {
      tile->expires = tileset->auto_expire;
   } else {
      tile->expires = tileset->expires;
   }
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

/*
 * allocate and initialize a feature_info for a given tileset
 */
geocache_feature_info* geocache_tileset_feature_info_create(apr_pool_t *pool, geocache_tileset *tileset,
      geocache_grid_link *grid_link) {
   geocache_feature_info *fi = (geocache_feature_info*)apr_pcalloc(pool, sizeof(geocache_feature_info));
   fi->map.tileset = tileset;
   fi->map.grid_link = grid_link;
   if(tileset->dimensions) {
      int i;
      fi->map.dimensions = apr_table_make(pool,tileset->dimensions->nelts);
      for(i=0;i<tileset->dimensions->nelts;i++) {
         geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
         apr_table_set(fi->map.dimensions,dimension->name,dimension->default_value);
      }
   }
   return fi;
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

   if(ret == GEOCACHE_SUCCESS && tile->tileset->auto_expire && tile->mtime && tile->tileset->source) {
      /* the cache is in auto-expire mode, and can return the tile modification date,
       * and there is a source configured so we can possibly update it,
       * so we check to see if it is stale */
      apr_time_t now = apr_time_now();
      apr_time_t stale = tile->mtime + apr_time_from_sec(tile->tileset->auto_expire);
      if(stale<now) {
         geocache_tileset_tile_delete(ctx,tile,GEOCACHE_TRUE);
         GC_CHECK_ERROR(ctx);
         ret = GEOCACHE_CACHE_MISS;
      }
   }

   if(ret == GEOCACHE_CACHE_MISS) {
      /* bail out straight away if the tileset has no source */
      if(!tile->tileset->source) {
         ctx->set_error(ctx,404,"tile not in cache, and no source configured for tileset %s",
               tile->tileset->name);
         return;
      }

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
      
      mt = geocache_tileset_metatile_get(ctx, tile);

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
         if(isLocked == GEOCACHE_TRUE) {
            ctx->set_error(ctx, 500, "tileset %s: unknown error (another thread/process failed to create the tile I was waiting for)",
                  tile->tileset->name);
         } else {
            /* shouldn't really happen, as the error ought to have been caught beforehand */
            ctx->set_error(ctx, 500, "tileset %s: failed to re-get tile %d %d %d from cache after set", tile->tileset->name,tile->x,tile->y,tile->z);
         }
      }
   }
   /* update the tile expiration time */
   if(tile->tileset->auto_expire && tile->mtime) {
      apr_time_t now = apr_time_now();
      apr_time_t expire_time = tile->mtime + apr_time_from_sec(tile->tileset->auto_expire);
      tile->expires = apr_time_sec(expire_time-now);
   }
}

void geocache_tileset_tile_delete(geocache_context *ctx, geocache_tile *tile, int whole_metatile) {
   int i;
   /*delete the tile itself*/
   tile->tileset->cache->tile_delete(ctx,tile);
   GC_CHECK_ERROR(ctx);

   if(whole_metatile) {
      geocache_metatile *mt = geocache_tileset_metatile_get(ctx, tile);
      for(i=0;i<mt->ntiles;i++) {
         geocache_tile *subtile = &mt->tiles[i];
         /* skip deleting the actual tile */
         if(subtile->x == tile->x && subtile->y == tile->y) continue;
         subtile->tileset->cache->tile_delete(ctx,subtile);
         /* silently pass failure if the tile was not found */
         if(ctx->get_error(ctx) == 404) {
            ctx->clear_errors(ctx);
         }
         GC_CHECK_ERROR(ctx);
      }
   }
}

/* vim: ai ts=3 sts=3 et sw=3
*/

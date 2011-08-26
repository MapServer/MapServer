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

void _geocache_image_merge(geocache_context *ctx, geocache_image *base, geocache_image *overlay) {
   int i,j;
   unsigned char *browptr, *orowptr, *bptr, *optr;
   if(base->w != overlay->w || base->h != overlay->h) {
      ctx->set_error(ctx, GEOCACHE_IMAGE_ERROR, "attempting to merge images with different sizes");
      return;
   }
   browptr = base->data;
   orowptr = overlay->data;
   for(i=0;i<base->h;i++) {
      bptr = browptr;
      optr = orowptr;
      for(j=0;j<base->w;j++) {
         if(optr[3]) { /* if overlay is not completely transparent */
            if(optr[3] == 255) {
               bptr[0]=optr[0];
               bptr[1]=optr[1];
               bptr[2]=optr[2];
               bptr[3]=optr[3];
            } else {
               unsigned int br = bptr[0];
               unsigned int bg = bptr[1];
               unsigned int bb = bptr[2];
               unsigned int ba = bptr[3];
               unsigned int or = optr[0];
               unsigned int og = optr[1];
               unsigned int ob = optr[2];
               unsigned int oa = optr[3];
               bptr[0] += (unsigned char)(((or - br)*oa) >> 8);
               bptr[1] += (unsigned char)(((og - bg)*oa) >> 8);
               bptr[2] += (unsigned char)(((ob - bb)*oa) >> 8);
               bptr[3] = (ba==255)?255:(unsigned char)((oa + ba) - ((oa * ba + 255) >> 8));                        
            }
         }
         bptr+=4;optr+=4;
      }
      browptr += base->stride;
      orowptr += overlay->stride;
   }
}

geocache_tile* geocache_image_merge_tiles(geocache_context *ctx, geocache_image_format *format, geocache_tile **tiles, int ntiles) {
   geocache_image *base,*overlay;
   int i;
   geocache_tile *tile = apr_pcalloc(ctx->pool,sizeof(geocache_tile));
   tile->mtime = tiles[0]->mtime;
   tile->expires = tiles[0]->expires;
   base = geocache_imageio_decode(ctx, tiles[0]->data);
   if(!base) return NULL;
   for(i=1; i<ntiles; i++) {
      overlay = geocache_imageio_decode(ctx, tiles[i]->data);
      if(!overlay) return NULL;
      if(tile->mtime < tiles[i]->mtime)
         tile->mtime = tiles[i]->mtime;
      if(tiles[i]->expires && ((tile->expires < tiles[i]->expires) || !tile->expires)) {
         tile->expires = tiles[i]->expires;
      }
      _geocache_image_merge(ctx, base, overlay);
      if(GC_HAS_ERROR(ctx)) {
         return NULL;
      }
   }

   tile->data = format->write(ctx, base, format);
   if(GC_HAS_ERROR(ctx)) {
      return NULL;
   }
   tile->sx = base->w;
   tile->sy = base->h;
   tile->tileset = tiles[0]->tileset;
   return tile;
}

void geocache_image_metatile_split(geocache_context *ctx, geocache_metatile *mt) {
   if(mt->tile.tileset->format) {
      /* the tileset has a format defined, we will use it to encode the data */
      geocache_image tileimg;
      geocache_image *metatile;
      int i,j;
      int sx,sy;
      tileimg.w = mt->tile.tileset->grid->tile_sx;
      tileimg.h = mt->tile.tileset->grid->tile_sy;
      metatile = geocache_imageio_decode(ctx, mt->tile.data);
      if(!metatile) {
         ctx->set_error(ctx, GEOCACHE_IMAGE_ERROR, "failed to load image data from metatile");
         return;
      }
      tileimg.stride = metatile->stride;
      for(i=0;i<mt->tile.tileset->metasize_x;i++) {
         for(j=0;j<mt->tile.tileset->metasize_y;j++) {
            sx = mt->tile.tileset->metabuffer + i * tileimg.w;
            sy = mt->tile.sy - (mt->tile.tileset->metabuffer + (j+1) * tileimg.w);
            tileimg.data = &(metatile->data[sy*metatile->stride + 4 * sx]);
            mt->tiles[i*mt->tile.tileset->metasize_x+j].data = mt->tile.tileset->format->write(ctx, &tileimg, mt->tile.tileset->format);
            GC_CHECK_ERROR(ctx);
         }
      }
   } else {
#ifdef DEBUG
      if(mt->tile.tileset->metasize_x != 1 ||
            mt->tile.tileset->metasize_y != 1 ||
            mt->tile.tileset->metabuffer != 0) {
         ctx->set_error(ctx, GEOCACHE_IMAGE_ERROR, "##### BUG ##### using a metatile with no format");
         return;
      }
      mt->tiles[0].data = mt->tile.data;
#endif
   }
}

int geocache_image_blank_color(geocache_image* image) {
   int* pixptr;
   int r,c;
   for(r=0;r<image->h;r++) {
      pixptr = (int*)(image->data + r * image->stride);
      for(c=0;c<image->w;c++) {
         if(*(pixptr++) != *((int*)image->data)) {
            return GEOCACHE_FALSE;
         }
      }
   }
   return GEOCACHE_TRUE;
}


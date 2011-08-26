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
#include <http_log.h>


int _geocache_image_merge(request_rec *r, geocache_image *base, geocache_image *overlay) {
   int i,j;
   unsigned char *browptr, *orowptr, *bptr, *optr;
   if(base->w != overlay->w || base->h != overlay->h) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "attempting to merge images with different sizes");
      return GEOCACHE_FAILURE;
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
   return GEOCACHE_SUCCESS;
}

geocache_tile* geocache_image_merge_tiles(request_rec *r, geocache_image_format *format, geocache_tile **tiles, int ntiles) {
   geocache_image *base,*overlay;
   int i;
   geocache_tile *tile = apr_pcalloc(r->pool,sizeof(geocache_tile));
   tile->mtime = tiles[0]->mtime;
   tile->expires = tiles[0]->expires;
   base = geocache_imageio_decode(r, tiles[0]->data);
   if(!base) return NULL;
   for(i=1; i<ntiles; i++) {
      overlay = geocache_imageio_decode(r, tiles[i]->data);
      if(tile->mtime < tiles[i]->mtime)
         tile->mtime = tiles[i]->mtime;
      if(tiles[i]->expires && ((tile->expires < tiles[i]->expires) || !tile->expires)) {
         tile->expires = tiles[i]->expires;
      }
      if(!overlay) return NULL;
      _geocache_image_merge(r, base, overlay);
   }

   tile->data = format->write(base, format, r);
   tile->sx = base->w;
   tile->sy = base->h;
   tile->tileset = tiles[0]->tileset;
   return tile;
}

int geocache_image_metatile_split(geocache_metatile *mt, request_rec *r) {
   if(mt->tile.tileset->format) {
      /* the tileset has a format defined, we will use it to encode the data */
      geocache_image tileimg;
      geocache_image *metatile;
      int i,j;
      int sx,sy;
      tileimg.w = mt->tile.tileset->tile_sx;
      tileimg.h = mt->tile.tileset->tile_sy;
      metatile = geocache_imageio_decode(r, mt->tile.data);
      if(!metatile) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to load image data from metatile");
         return GEOCACHE_FAILURE;
      }
      tileimg.stride = metatile->stride;
      if(!metatile) return GEOCACHE_FAILURE;
      for(i=0;i<mt->tile.tileset->metasize_x;i++) {
         for(j=0;j<mt->tile.tileset->metasize_y;j++) {
            sx = mt->tile.tileset->metabuffer + i * tileimg.w;
            sy = mt->tile.sy - (mt->tile.tileset->metabuffer + (j+1) * tileimg.w);
            tileimg.data = &(metatile->data[sy*metatile->stride + 4 * sx]);
            mt->tiles[i*mt->tile.tileset->metasize_x+j].data = mt->tile.tileset->format->write(&tileimg, mt->tile.tileset->format, r);
         }
      }
   } else {
#ifdef DEBUG
      if(mt->tile.tileset->metasize_x != 1 ||
            mt->tile.tileset->metasize_y != 1 ||
            mt->tile.tileset->metabuffer != 0) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "##### BUG ##### using a metatile with no format");
         return GEOCACHE_FAILURE;
      }
      mt->tiles[0].data = mt->tile.data;
#endif
   }
   return GEOCACHE_SUCCESS;
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


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

#include "yatc.h"
#include <cairo.h>

typedef struct {
   yatc_buffer *buffer;
   char *ptr;
} yatc_closure;



cairo_status_t _yatc_cairo_read_func(void *closure, unsigned char *data, unsigned int length) {
   yatc_closure *c = (yatc_closure*)closure;
   memcpy(data,c->ptr,length);
   c->ptr += length;
   return CAIRO_STATUS_SUCCESS;
}

cairo_status_t _yatc_cairo_write_func(void *closure, const unsigned char *data, unsigned int length) {
   yatc_buffer *c = (yatc_buffer*)closure;
   yatc_buffer_append(c,length,(void*)data);
   return CAIRO_STATUS_SUCCESS;
}

cairo_surface_t *_yatc_tile_create_cairo_surface(yatc_buffer *buffer) {
   yatc_closure cl;
   cairo_surface_t *surface = NULL;
   
   cl.buffer = buffer;
   cl.ptr = buffer->buf;
   surface = cairo_image_surface_create_from_png_stream(_yatc_cairo_read_func,(void*)&cl);
      
   return surface;
}

yatc_buffer* _yatc_buffer_create_from_cairo(apr_pool_t *pool, cairo_surface_t *surface) {
   yatc_buffer *buffer = yatc_buffer_create(5000,pool);
   cairo_surface_write_to_png_stream(surface,_yatc_cairo_write_func,(void*)buffer);
   return buffer;
}

yatc_tile* _yatc_tile_create_from_cairo(apr_pool_t *pool, cairo_surface_t *surface) {
   yatc_tile *tile = apr_pcalloc(pool,sizeof(yatc_tile));
   tile->sx = cairo_image_surface_get_width(surface);
   tile->sy = cairo_image_surface_get_height(surface);
   tile->data = _yatc_buffer_create_from_cairo(pool,surface);
   return tile;
}





yatc_tile* yatc_image_merge_tiles(request_rec *r, yatc_tile **tiles, int ntiles) {
   cairo_surface_t *base,*overlay;
   cairo_t *cr;
   int i;
   yatc_tile *tile;

   base = _yatc_tile_create_cairo_surface(tiles[0]->data);
   cr = cairo_create(base);
   for(i=1;i<ntiles;i++) {
      overlay = _yatc_tile_create_cairo_surface(tiles[i]->data);
      cairo_set_source_surface (cr,overlay,0,0);
      cairo_paint_with_alpha(cr,1.0);
      cairo_surface_destroy(overlay);
   }
   tile = _yatc_tile_create_from_cairo(r->pool,base);
   cairo_surface_destroy(base);
   cairo_destroy(cr);
   return tile;
}

int yatc_image_metatile_split(yatc_metatile *mt, request_rec *r) {
   cairo_surface_t *metatile;
   cairo_surface_t *tile;
   cairo_t *cr;
   int i,j;
   int w,h,sx,sy;
   w = mt->tile.tileset->tile_sx;
   h = mt->tile.tileset->tile_sy;
   metatile = _yatc_tile_create_cairo_surface(mt->tile.data);
   tile = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
   cr = cairo_create(tile);
   for(i=0;i<mt->tile.tileset->metasize_x;i++) {
      for(j=0;j<mt->tile.tileset->metasize_y;j++) {
         cairo_save (cr);
         cairo_set_source_rgba (cr, 0,0,0,0);
         cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
         cairo_paint (cr);
         cairo_restore (cr);

         sx = mt->tile.tileset->metabuffer + i * w;
         sy = mt->tile.sy - (mt->tile.tileset->metabuffer + (j+1) * w);
         cairo_set_source_surface (cr, metatile, -sx, -sy);
         cairo_rectangle (cr, 0,0, w, h);
         cairo_fill (cr);
         mt->tiles[i*mt->tile.tileset->metasize_x+j].data = _yatc_buffer_create_from_cairo(r->pool,tile);
      }
   }
   cairo_surface_destroy(tile);
   cairo_destroy(cr);
   cairo_surface_destroy(metatile);
   return YATC_SUCCESS;
}


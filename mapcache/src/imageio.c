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

#include "mapcache.h"
#include <png.h>
#include <jpeglib.h>

/**\addtogroup imageio*/
/** @{ */

int mapcache_imageio_is_valid_format(mapcache_context *ctx, mapcache_buffer *buffer) {
   mapcache_image_format_type t = mapcache_imageio_header_sniff(ctx,buffer);
   if(t==GC_PNG || t==GC_JPEG) {
      return MAPCACHE_TRUE;
   } else {
      return MAPCACHE_FALSE;
   }
}

mapcache_image_format_type mapcache_imageio_header_sniff(mapcache_context *ctx, mapcache_buffer *buffer) {
   if(!buffer) {
      return GC_UNKNOWN;
   }
   if(buffer->size >= 8 && png_sig_cmp((png_bytep)buffer->buf, 0, 8) == 0) {
      return GC_PNG;
   } else if(buffer->size >= 2 && ((unsigned char*)buffer->buf)[0] == 0xFF && ((unsigned char*)buffer->buf)[1] == 0xD8) {
      return GC_JPEG;
   } else {
      return GC_UNKNOWN;
   }
}



mapcache_image* mapcache_imageio_decode(mapcache_context *ctx, mapcache_buffer *buffer) {
   mapcache_image_format_type type = mapcache_imageio_header_sniff(ctx,buffer);
   if(type == GC_PNG) {
      return _mapcache_imageio_png_decode(ctx,buffer);
   } else if(type == GC_JPEG) {
      return _mapcache_imageio_jpeg_decode(ctx,buffer);
   } else {
      ctx->set_error(ctx, 500, "mapcache_imageio_decode: unrecognized image format");
      return NULL;
   }
}


void mapcache_image_create_empty(mapcache_context *ctx, mapcache_cfg *cfg) {
   unsigned int color=0;

   /* create a transparent image for PNG, and a white one for jpeg */
   if(!strstr(cfg->default_image_format->mime_type,"png")) {
      color = 0xffffffff;
   }
   cfg->empty_image = cfg->default_image_format->create_empty_image(ctx, cfg->default_image_format,
         256,256, color);
   GC_CHECK_ERROR(ctx);
}

void mapcache_imageio_decode_to_image(mapcache_context *ctx, mapcache_buffer *buffer,
      mapcache_image *image) {
   mapcache_image_format_type type = mapcache_imageio_header_sniff(ctx,buffer);
   if(type == GC_PNG) {
      _mapcache_imageio_png_decode_to_image(ctx,buffer,image);
   } else if(type == GC_JPEG) {
      _mapcache_imageio_jpeg_decode_to_image(ctx,buffer,image);
   } else {
      ctx->set_error(ctx, 500, "mapcache_imageio_decode: unrecognized image format");
   }
   return;
}

/** @} */

/* vim: ai ts=3 sts=3 et sw=3
*/

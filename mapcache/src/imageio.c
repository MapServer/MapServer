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
#include <png.h>
#include <jpeglib.h>

/**\addtogroup imageio*/
/** @{ */

int geocache_imageio_image_has_alpha(geocache_image *img) {
   size_t i,j;
   unsigned char *ptr, *rptr = img->data;
   for(i=0;i<img->h;i++) {     
      ptr = rptr;
      for(j=0;j<img->w;j++) {
         if(ptr[3]<(unsigned char)255)
            return 1;
         ptr += 4;
      }
      rptr += img->stride;
   }
   return 0;
}

int geocache_imageio_is_valid_format(geocache_context *ctx, geocache_buffer *buffer) {
   geocache_image_format_type t = geocache_imageio_header_sniff(ctx,buffer);
   if(t==GC_PNG || t==GC_JPEG) {
      return GEOCACHE_TRUE;
   } else {
      return GEOCACHE_FALSE;
   }
}

geocache_image_format_type geocache_imageio_header_sniff(geocache_context *ctx, geocache_buffer *buffer) {
   if(!buffer) {
      return GC_UNKNOWN;
   }
   if(buffer->size >= 8 && png_sig_cmp((png_bytep)buffer->buf, 0, 8) == 0) {
      return GC_PNG;
   } else if(buffer->size >= 2 && buffer->buf[0] == 0xFF && buffer->buf[1] == 0xD8) {
      return GC_JPEG;
   } else {
      return GC_UNKNOWN;
   }
}



geocache_image* geocache_imageio_decode(geocache_context *ctx, geocache_buffer *buffer) {
   geocache_image_format_type type = geocache_imageio_header_sniff(ctx,buffer);
   if(type == GC_PNG) {
      return _geocache_imageio_png_decode(ctx,buffer);
   } else if(type == GC_JPEG) {
      return _geocache_imageio_jpeg_decode(ctx,buffer);
   } else {
      ctx->set_error(ctx, GEOCACHE_IMAGE_ERROR, "geocache_imageio_decode: unrecognized image format");
      return NULL;
   }
}

/** @} */

/* vim: ai ts=3 sts=3 et sw=3
*/

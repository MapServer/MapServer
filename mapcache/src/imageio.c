/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: high level image format I/O
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

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

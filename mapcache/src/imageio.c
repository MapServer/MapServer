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
#include <png.h>

int _geocache_imageio_image_has_alpha(geocache_image *img) {
   int i,j;
   unsigned char *ptr, *rptr = img->data;
   for(i=0;i<img->h;i++) {
      rptr += img->stride;
      ptr = rptr;
      for(j=0;j<img->w;i++) {
         if(ptr[3]<255)
            return 1;
         ptr += 4;
      }
   }
   return 0;
}
void _geocache_imageio_png_write_func(png_structp png_ptr, png_bytep data, png_size_t length) {
   geocache_buffer_append((geocache_buffer*)png_get_io_ptr(png_ptr),length,data);
}

void _geocache_imageio_png_read_func(png_structp png_ptr, png_bytep data, png_size_t length) {
   _geocache_buffer_closure *b = (_geocache_buffer_closure*)png_get_io_ptr(png_ptr);
   memcpy(data,b->ptr,length);
   b->ptr += length;
}

void _geocache_imageio_png_flush_func(png_structp png_ptr) {
   // do nothing
}

geocache_image_format_type geocache_imageio_header_sniff(request_rec *r, geocache_buffer *buffer) {
   if(!buffer || buffer->size < 8) {
      return GEOCACHE_IMAGE_FORMAT_UNKNOWN;
   }
   if(png_sig_cmp((png_bytep)buffer->buf, 0, 8) == 0) {
      return GEOCACHE_IMAGE_FORMAT_PNG;
      /*   } else if(png_sig_cmp(buffer->buf, 0, 8) == 0) {
      return GEOCACHE_IMAGE_FORMAT_JPEG;*/
   } else {
      return GEOCACHE_IMAGE_FORMAT_UNKNOWN;
   }
}

geocache_image* geocache_imageio_png_decode(request_rec *r, geocache_buffer *buffer) {
   geocache_image *img;
   png_uint_32 row_bytes;
   int bit_depth,color_type,i;
   unsigned char **row_pointers;
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;
   _geocache_buffer_closure b;
   b.buffer = buffer;
   b.ptr = buffer->buf;

   
   /* could pass pointers to user-defined error handlers instead of NULLs: */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (!png_ptr) {
      ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r, "failed to allocate png_struct structure");
      return NULL;
   }

   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
   {
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r, "failed to allocate png_info structure");
      return NULL;
   }

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r, "failed to setjmp(png_jmpbuf(png_ptr))");
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      return NULL;
   }
   png_set_read_fn(png_ptr,&b,_geocache_imageio_png_read_func);
   
   png_read_info(png_ptr,info_ptr);
   img = apr_pcalloc(r->pool,sizeof(geocache_image));
   if(!png_get_IHDR(png_ptr, info_ptr, &img->w, &img->h,&bit_depth, &color_type,NULL,NULL,NULL)) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to read png header");
      return NULL;
   }
   
   img->data = apr_pcalloc(r->pool,img->w*img->h*4*sizeof(unsigned char));
   img->stride = img->w * 4;
   row_pointers = apr_pcalloc(r->pool,img->h * sizeof(unsigned char*));

   png_bytep rowptr = img->data;
   for(i=0;i<img->h;i++) {
      row_pointers[i] = rowptr;
      rowptr += img->stride;
   }


   if (color_type == PNG_COLOR_TYPE_PALETTE)
      /* expand palette images to RGB */
      png_set_expand(png_ptr);
   if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      /* expand low bit-depth grayscale to 8bits */
      png_set_expand(png_ptr);
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
      /* expand transparency chunks to full alpha */
      png_set_expand(png_ptr);
   if (bit_depth == 16)
      /* scale 16bits down to 8 */
      png_set_strip_16(png_ptr);
   if (color_type == PNG_COLOR_TYPE_GRAY ||
         color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      /* convert grayscale to rgba */
      png_set_gray_to_rgb(png_ptr);

   //png_set_bgr(png_ptr);
   if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
      png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);

   png_read_update_info(png_ptr, info_ptr);
   row_bytes = png_get_rowbytes(png_ptr, info_ptr);

   png_read_image(png_ptr, row_pointers);
   png_read_end(png_ptr,NULL);
   png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

   return img;
}


geocache_buffer* geocache_imageio_png_encode(request_rec *r, geocache_image *img) {
   png_infop info_ptr; 
   int color_type;
   size_t row;
   geocache_buffer *buffer = NULL;
   png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);

   if (!png_ptr) {
      ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r, "failed to allocate png_struct structure");
      return NULL;
   }

   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
   {
      png_destroy_write_struct(&png_ptr,
            (png_infopp)NULL);
      ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r, "failed to allocate png_info structure");
      return NULL;
   }

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r, "failed to setjmp(png_jmpbuf(png_ptr))");
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return NULL;
   }

   buffer = geocache_buffer_create(5000,r->pool);

   png_set_write_fn(png_ptr, buffer, _geocache_imageio_png_write_func, _geocache_imageio_png_flush_func);

   if(_geocache_imageio_image_has_alpha(img))
      color_type = PNG_COLOR_TYPE_RGB_ALPHA;
   else
      color_type = PNG_COLOR_TYPE_RGB;

   png_set_IHDR(png_ptr, info_ptr, img->w, img->h,
         8, color_type, PNG_INTERLACE_NONE,
         PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
   png_set_compression_level (png_ptr, Z_BEST_SPEED);

   png_write_info(png_ptr, info_ptr);

   png_bytep rowptr = img->data;
   for(row=0;row<img->h;row++) {
      png_write_row(png_ptr,rowptr);
      rowptr += img->stride;
   }
   png_write_end(png_ptr, info_ptr);
   png_destroy_write_struct(&png_ptr, &info_ptr);
   return buffer;
}

geocache_image* geocache_imageio_decode(request_rec *r, geocache_buffer *buffer) {
   if(geocache_imageio_header_sniff(r,buffer) != GEOCACHE_IMAGE_FORMAT_PNG) {
      return NULL;
   }
   return geocache_imageio_png_decode(r,buffer);
}

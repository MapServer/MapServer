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
#include <jpeglib.h>

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

typedef struct {
   struct jpeg_destination_mgr pub;
   unsigned char *data;
   geocache_buffer *buffer;
} geocache_jpeg_destination_mgr;


#define OUTPUT_BUF_SIZE 4096

static void _geocache_imageio_jpeg_init_source(j_decompress_ptr cinfo)
{
   /* nothing to do */
}

static int _geocache_imageio_jpeg_fill_input_buffer(j_decompress_ptr cinfo)
{
   static JOCTET mybuffer[4];

   /* The whole JPEG data is expected to reside in the supplied memory
    * buffer, so any request for more data beyond the given buffer size
    * is treated as an error.
    */
   /* Insert a fake EOI marker */
   mybuffer[0] = (JOCTET) 0xFF;
   mybuffer[1] = (JOCTET) JPEG_EOI;

   cinfo->src->next_input_byte = mybuffer;
   cinfo->src->bytes_in_buffer = 2;

   return TRUE;
}

static void _geocache_imageio_jpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
   struct jpeg_source_mgr * src = cinfo->src;

   /* Just a dumb implementation for now.  Could use fseek() except
    * it doesn't work on pipes.  Not clear that being smart is worth
    * any trouble anyway --- large skips are infrequent.
    */
   if (num_bytes > 0) {
      while (num_bytes > (long) src->bytes_in_buffer) {
         num_bytes -= (long) src->bytes_in_buffer;
         (void) (*src->fill_input_buffer) (cinfo);
         /* note we assume that fill_input_buffer will never return FALSE,
          * so suspension need not be handled.
          */
      }
      src->next_input_byte += (size_t) num_bytes;
      src->bytes_in_buffer -= (size_t) num_bytes;
   }
}

static void _geocache_imageio_jpeg_term_source(j_decompress_ptr cinfo)
{
}



int _geocache_imageio_jpeg_mem_src (j_decompress_ptr cinfo, unsigned char * inbuffer, unsigned long insize)
{
   struct jpeg_source_mgr * src;

   if (inbuffer == NULL || insize == 0) /* Treat empty input as fatal error */
      return GEOCACHE_FAILURE;

   /* The source object is made permanent so that a series of JPEG images
    * can be read from the same buffer by calling jpeg_mem_src only before
    * the first one.
    */
   if (cinfo->src == NULL) {   /* first time for this JPEG object? */
      cinfo->src = (struct jpeg_source_mgr *)
                        (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                              sizeof(struct jpeg_source_mgr));
   }

   src = cinfo->src;
   src->init_source = _geocache_imageio_jpeg_init_source;
   src->fill_input_buffer = _geocache_imageio_jpeg_fill_input_buffer;
   src->skip_input_data = _geocache_imageio_jpeg_skip_input_data;
   src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
   src->term_source = _geocache_imageio_jpeg_term_source;
   src->bytes_in_buffer = (size_t) insize;
   src->next_input_byte = (JOCTET *) inbuffer;
   return GEOCACHE_SUCCESS;
}



void _geocache_imageio_jpeg_init_destination (j_compress_ptr cinfo) {
   geocache_jpeg_destination_mgr *dest = (geocache_jpeg_destination_mgr*) cinfo->dest;

   /* Allocate the output buffer --- it will be released when done with image */
   dest->data = (unsigned char *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
         JPOOL_IMAGE,OUTPUT_BUF_SIZE * sizeof (unsigned char));

   dest->pub.next_output_byte = dest->data;
   dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

void _geocache_imageio_jpeg_buffer_term_destination (j_compress_ptr cinfo) {
   geocache_jpeg_destination_mgr *dest = (geocache_jpeg_destination_mgr*) cinfo->dest;
   geocache_buffer_append(dest->buffer,OUTPUT_BUF_SIZE-dest->pub.free_in_buffer, dest->data);
   dest->pub.next_output_byte = dest->data;
   dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}


int _geocache_imageio_jpeg_buffer_empty_output_buffer (j_compress_ptr cinfo) {
   geocache_jpeg_destination_mgr *dest = (geocache_jpeg_destination_mgr*) cinfo->dest;
   geocache_buffer_append(dest->buffer,OUTPUT_BUF_SIZE, dest->data);
   dest->pub.next_output_byte = dest->data;
   dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
   return TRUE;
}

geocache_buffer* _geocache_imageio_jpeg_encode(request_rec *r, geocache_image *img) {
   struct jpeg_compress_struct cinfo;
   struct jpeg_error_mgr jerr;
   int quality = 85;
   geocache_jpeg_destination_mgr *dest;
   JSAMPLE *rowdata; 
   unsigned int row;
   geocache_buffer *buffer = geocache_buffer_create(5000,r->pool);
   cinfo.err = jpeg_std_error(&jerr);
   jpeg_create_compress(&cinfo);

   cinfo.dest = (struct jpeg_destination_mgr *)
                                                                (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
                                                                      sizeof (geocache_jpeg_destination_mgr));
   ((geocache_jpeg_destination_mgr*)cinfo.dest)->pub.empty_output_buffer = _geocache_imageio_jpeg_buffer_empty_output_buffer;
   ((geocache_jpeg_destination_mgr*)cinfo.dest)->pub.term_destination = _geocache_imageio_jpeg_buffer_term_destination;
   ((geocache_jpeg_destination_mgr*)cinfo.dest)->buffer = buffer;

   dest = (geocache_jpeg_destination_mgr*) cinfo.dest;
   dest->pub.init_destination = _geocache_imageio_jpeg_init_destination;

   cinfo.image_width = img->w;
   cinfo.image_height = img->h;
   cinfo.input_components = 3;
   cinfo.in_color_space = JCS_RGB;
   jpeg_set_defaults(&cinfo);
   jpeg_set_quality(&cinfo, quality, TRUE);
   jpeg_start_compress(&cinfo, TRUE);

   rowdata = (JSAMPLE*)malloc(img->w*cinfo.input_components*sizeof(JSAMPLE));
   for(row=0;row<img->h;row++) {
      JSAMPLE *pixptr = rowdata;
      int col;
      unsigned char *r,*g,*b;
      r=&(img->data[0])+row*img->stride;
      g=&(img->data[1])+row*img->stride;
      b=&(img->data[2])+row*img->stride;
      for(col=0;col<img->w;col++) {
         *(pixptr++) = *r;
         *(pixptr++) = *g;
         *(pixptr++) = *b;
         r+=4;
         g+=4;
         b+=4;
      }
      (void) jpeg_write_scanlines(&cinfo, &rowdata, 1);
   }

   /* Step 6: Finish compression */

   jpeg_finish_compress(&cinfo);
   jpeg_destroy_compress(&cinfo);
   free(rowdata);
   return buffer;
}


geocache_image_format_type geocache_imageio_header_sniff(request_rec *r, geocache_buffer *buffer) {
   if(!buffer) {
      return GEOCACHE_IMAGE_FORMAT_UNKNOWN;
   }
   if(buffer->size >= 8 && png_sig_cmp((png_bytep)buffer->buf, 0, 8) == 0) {
      return GEOCACHE_IMAGE_FORMAT_PNG;
   } else if(buffer->size >= 2 && buffer->buf[0] == 0xFF && buffer->buf[1] == 0xD8) {
      return GEOCACHE_IMAGE_FORMAT_JPEG;
   } else {
      return GEOCACHE_IMAGE_FORMAT_UNKNOWN;
   }
}



geocache_image* _geocache_imageio_jpeg_decode(request_rec *r, geocache_buffer *buffer) {
   struct jpeg_decompress_struct cinfo = {NULL};
   struct jpeg_error_mgr jerr;
   jpeg_create_decompress(&cinfo);
   cinfo.err = jpeg_std_error(&jerr);
   geocache_image *img = apr_pcalloc(r->pool,sizeof(geocache_image));
   if (_geocache_imageio_jpeg_mem_src(&cinfo,buffer->buf, buffer->size) != GEOCACHE_SUCCESS){
      return NULL;
   }

   jpeg_read_header(&cinfo, TRUE);
   jpeg_start_decompress(&cinfo);
   img->w = cinfo.output_width;
   img->h = cinfo.output_height;
   int s = cinfo.output_components;
   img->data = apr_pcalloc(r->pool,img->w*img->h*4*sizeof(unsigned char));
   img->stride = img->w * 4;

   unsigned char *temp = apr_pcalloc(r->pool,img->w*s);
   while ((int)cinfo.output_scanline < img->h)
   {
      int i;
      unsigned char *rowptr = &img->data[cinfo.output_scanline * img->stride];
      unsigned char *tempptr = temp;
      jpeg_read_scanlines(&cinfo, &tempptr, 1);
      if (s == 1)
      {
         for (i = 0; i < img->w; i++)
         {
            *rowptr++ = *tempptr;
            *rowptr++ = *tempptr;
            *rowptr++ = *tempptr;
            *rowptr++ = 255;
            tempptr++;
         }
      }
      else if (s == 3)
      {
         for (i = 0; i < img->w; i++)
         {
            *rowptr++ = *tempptr++;
            *rowptr++ = *tempptr++;
            *rowptr++ = *tempptr++;
            *rowptr++ = 255;
         }
      }
      else
      {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "unsupported jpeg format");
         jpeg_destroy_decompress(&cinfo);
         return NULL;
      }
   }
   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);
   return img;

}

geocache_image* _geocache_imageio_png_decode(request_rec *r, geocache_buffer *buffer) {
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

geocache_buffer* _geocache_imageio_png_encode(request_rec *r, geocache_image *img) {
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

geocache_buffer* geocache_imageio_encode(request_rec *r, geocache_image *image, geocache_image_format_type format) {
   if(format == GEOCACHE_IMAGE_FORMAT_PNG) {
      return _geocache_imageio_png_encode(r,image);
   } else if(format == GEOCACHE_IMAGE_FORMAT_JPEG) {
      return _geocache_imageio_jpeg_encode(r,image);
   } else {
      return NULL;
   }
}

geocache_image* geocache_imageio_decode(request_rec *r, geocache_buffer *buffer) {
   geocache_image_format_type type = geocache_imageio_header_sniff(r,buffer);
   if(type == GEOCACHE_IMAGE_FORMAT_PNG) {
      return _geocache_imageio_png_decode(r,buffer);
   } else if(type == GEOCACHE_IMAGE_FORMAT_JPEG) {
      return _geocache_imageio_jpeg_decode(r,buffer);
   } else {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "geocache_imageio_decode: unrecognized image format");
      return NULL;
   }
}

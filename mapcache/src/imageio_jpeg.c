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
#include <jpeglib.h>

/**\addtogroup imageio_jpg */
/** @{ */

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

geocache_buffer* _geocache_imageio_jpeg_encode(geocache_context *ctx, geocache_image *img, geocache_image_format *format) {
   struct jpeg_compress_struct cinfo;
   struct jpeg_error_mgr jerr;
   geocache_jpeg_destination_mgr *dest;
   JSAMPLE *rowdata;
   unsigned int row;
   geocache_buffer *buffer = geocache_buffer_create(5000, ctx->pool);
   cinfo.err = jpeg_std_error(&jerr);
   jpeg_create_compress(&cinfo);

   cinfo.dest = (struct jpeg_destination_mgr *)(*cinfo.mem->alloc_small) (
         (j_common_ptr) &cinfo, JPOOL_PERMANENT,
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
   jpeg_set_quality(&cinfo, ((geocache_image_format_jpeg*)format)->quality, TRUE);
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

geocache_image* _geocache_imageio_jpeg_decode(geocache_context *r, geocache_buffer *buffer) {
   struct jpeg_decompress_struct cinfo = {NULL};
   struct jpeg_error_mgr jerr;
   jpeg_create_decompress(&cinfo);
   cinfo.err = jpeg_std_error(&jerr);
   geocache_image *img = geocache_image_create(r);
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
         r->set_error(r, 500, "unsupported jpeg format");
         jpeg_destroy_decompress(&cinfo);
         return NULL;
      }
   }
   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);
   return img;

}

static geocache_buffer* _geocache_imageio_jpg_create_empty(geocache_context *ctx, geocache_image_format *format,
      size_t width, size_t height, unsigned int color) {

   geocache_image_format_jpeg *f = (geocache_image_format_jpeg*)format;

   apr_pool_t *pool = NULL;
   if(apr_pool_create(&pool,ctx->pool) != APR_SUCCESS) {
      ctx->set_error(ctx,500,"png create empty: failed to create temp memory pool");
      return NULL;
   }
   geocache_image *empty = geocache_image_create(ctx);
   if(GC_HAS_ERROR(ctx)) {
      return NULL;
   }
   empty->data = (unsigned char*)apr_pcalloc(pool,width*height*4*sizeof(unsigned char)); 
   int i;
   for(i=0;i<width*height;i++) {
      ((unsigned int*)empty->data)[i] = color;
   }
   empty->w = width;
   empty->h = height;
   empty->stride = width * 4;

   geocache_buffer *buf = format->write(ctx,empty,format);
   apr_pool_destroy(pool);
   return buf;
}

geocache_image_format* geocache_imageio_create_jpeg_format(apr_pool_t *pool, char *name, int quality ) {
   geocache_image_format_jpeg *format = apr_pcalloc(pool, sizeof(geocache_image_format_jpeg));
   format->format.name = name;
   format->format.extension = apr_pstrdup(pool,"jpg");
   format->format.mime_type = apr_pstrdup(pool,"image/jpeg");
   format->format.metadata = apr_table_make(pool,3);
   format->format.create_empty_image = _geocache_imageio_jpg_create_empty;
   format->format.write = _geocache_imageio_jpeg_encode;
   format->quality = quality;
   return (geocache_image_format*)format;
}

/** @} */



/* vim: ai ts=3 sts=3 et sw=3
*/

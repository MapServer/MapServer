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
#ifdef USE_PIXMAN
#include <pixman.h>
#else
#include <math.h>
#endif

geocache_image* geocache_image_create(geocache_context *ctx) {
    geocache_image *img = (geocache_image*)apr_pcalloc(ctx->pool,sizeof(geocache_image));
    img->w= img->h= 0;
    img->data=NULL;
    return img;
}

int geocache_image_has_alpha(geocache_image *img) {
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

void geocache_image_merge(geocache_context *ctx, geocache_image *base, geocache_image *overlay) {
   int starti,startj;
   if(base->w < overlay->w || base->h < overlay->h) {
      ctx->set_error(ctx, 500, "attempting to merge an larger image onto another");
      return;
   }
   starti = (base->h - overlay->h)/2;
   startj = (base->w - overlay->w)/2;
#ifdef USE_PIXMAN
   pixman_image_t *si = pixman_image_create_bits(PIXMAN_a8r8g8b8,overlay->w,overlay->h,
         (uint32_t*)overlay->data,overlay->stride);
   pixman_image_t *bi = pixman_image_create_bits(PIXMAN_a8r8g8b8,base->w,base->h,
         (uint32_t*)base->data,base->stride);
   pixman_transform_t transform;
   pixman_transform_init_translate(&transform,
         pixman_int_to_fixed(-startj),
         pixman_int_to_fixed(-starti));
   pixman_image_set_filter(si,PIXMAN_FILTER_NEAREST, NULL, 0);
   pixman_image_set_transform (si, &transform);
   pixman_image_composite (PIXMAN_OP_OVER, si, si, bi,
                            0, 0, 0, 0, 0, 0, base->w,base->h);
   pixman_image_unref(si);
   pixman_image_unref(bi);
#else
   int i,j;
   unsigned char *browptr, *orowptr, *bptr, *optr;

   browptr = base->data + starti * base->stride + startj*4;
   orowptr = overlay->data;
   for(i=0;i<overlay->h;i++) {
      bptr = browptr;
      optr = orowptr;
      for(j=0;j<overlay->w;j++) {
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
#endif
}

#ifndef USE_PIXMAN
static inline void bilinear_pixel(geocache_image *img, double x, double y, unsigned char *dst) {
   int px,py;
   px = (int)x;
   py = (int)y;

   int px1 = (px==(img->w-1))?(px):(px+1);
   int py1 = (py==(img->h-1))?(py):(py+1);

   unsigned char *p1 = &img->data[py*img->stride+px*4];
   unsigned char *p2 = &img->data[py*img->stride+px1*4];
   unsigned char *p3 = &img->data[py1*img->stride+px*4];
   unsigned char *p4 = &img->data[py1*img->stride+px1*4];

   // Calculate the weights for each pixel
   float fx = x - px;
   float fy = y - py;
   float fx1 = 1.0f - fx;
   float fy1 = 1.0f - fy;

   int w1 = fx1 * fy1 * 256.0f;
   int w2 = fx  * fy1 * 256.0f;
   int w3 = fx1 * fy  * 256.0f;
   int w4 = fx  * fy  * 256.0f;

   // Calculate the weighted sum of pixels (for each color channel)
   dst[0] = (p1[0] * w1 + p2[0] * w2 + p3[0] * w3 + p4[0] * w4) >> 8;
   dst[1] = (p1[1] * w1 + p2[1] * w2 + p3[1] * w3 + p4[1] * w4) >> 8;
   dst[2] = (p1[2] * w1 + p2[2] * w2 + p3[2] * w3 + p4[2] * w4) >> 8;
   dst[3] = (p1[3] * w1 + p2[3] * w2 + p3[3] * w3 + p4[3] * w4) >> 8;
}
#endif

void geocache_image_copy_resampled_nearest(geocache_context *ctx, geocache_image *src, geocache_image *dst,
      double off_x, double off_y, double scale_x, double scale_y) {
#ifdef USE_PIXMAN
   pixman_image_t *si = pixman_image_create_bits(PIXMAN_a8r8g8b8,src->w,src->h,
         (uint32_t*)src->data,src->stride);
   pixman_image_t *bi = pixman_image_create_bits(PIXMAN_a8r8g8b8,dst->w,dst->h,
         (uint32_t*)dst->data,dst->stride);
   pixman_transform_t transform;
   pixman_transform_init_translate(&transform,pixman_double_to_fixed(-off_x),pixman_double_to_fixed(-off_y));
   pixman_transform_scale(&transform,NULL,pixman_double_to_fixed(1.0/scale_x),pixman_double_to_fixed(1.0/scale_y));
   pixman_image_set_transform (si, &transform);
   pixman_image_set_filter(si,PIXMAN_FILTER_NEAREST, NULL, 0);
   pixman_image_composite (PIXMAN_OP_SRC, si, NULL, bi,
                            0, 0, 0, 0, 0, 0, dst->w,dst->h);
   pixman_image_unref(si);
   pixman_image_unref(bi);
#else
   int dstx,dsty;
   unsigned char *dstrowptr = dst->data;
   for(dsty=0; dsty<dst->h; dsty++) {
      int *dstptr = (int*)dstrowptr;
      int srcy = round((dsty-off_y)/scale_y);
      if(srcy >= 0 && srcy < src->h) {
         for(dstx=0; dstx<dst->w;dstx++) {
            int srcx = round((dstx-off_x)/scale_x);
            if(srcx >= 0 && srcx < src->w) {
               *dstptr = *((int*)&(src->data[srcy*src->stride+srcx*4]));
            }
            dstptr ++;
         }
      }
      dstrowptr += dst->stride;
   }
#endif
} 


void geocache_image_copy_resampled_bilinear(geocache_context *ctx, geocache_image *src, geocache_image *dst,
      double off_x, double off_y, double scale_x, double scale_y) {
#ifdef USE_PIXMAN
   pixman_image_t *si = pixman_image_create_bits(PIXMAN_a8r8g8b8,src->w,src->h,
         (uint32_t*)src->data,src->stride);
   pixman_image_t *bi = pixman_image_create_bits(PIXMAN_a8r8g8b8,dst->w,dst->h,
         (uint32_t*)dst->data,dst->stride);
   pixman_transform_t transform;
   pixman_transform_init_translate(&transform,pixman_double_to_fixed(-off_x),pixman_double_to_fixed(-off_y));
   pixman_transform_scale(&transform,NULL,pixman_double_to_fixed(1.0/scale_x),pixman_double_to_fixed(1.0/scale_y));
   pixman_image_set_transform (si, &transform);
   pixman_image_set_filter(si,PIXMAN_FILTER_BILINEAR, NULL, 0);
   pixman_image_composite (PIXMAN_OP_SRC, si, NULL, bi,
                            0, 0, 0, 0, 0, 0, dst->w,dst->h);
   pixman_image_unref(si);
   pixman_image_unref(bi);
#else
   int dstx,dsty;
   unsigned char *dstrowptr = dst->data;
   for(dsty=0; dsty<dst->h; dsty++) {
      unsigned char *dstptr = dstrowptr;
      double srcy = (dsty-off_y)/scale_y;
      if(srcy >= 0 && srcy < src->h) {
         for(dstx=0; dstx<dst->w;dstx++) {
            double srcx = (dstx-off_x)/scale_x;
            if(srcx >= 0 && srcx < src->w) {
               bilinear_pixel(src,srcx,srcy,dstptr);
            }
            dstptr += 4;
         }
      }
      dstrowptr += dst->stride;
   }
#endif
} 

void geocache_image_metatile_split(geocache_context *ctx, geocache_metatile *mt) {
   if(mt->map.tileset->format) {
      /* the tileset has a format defined, we will use it to encode the data */
      geocache_image tileimg;
      geocache_image *metatile;
      int i,j;
      int sx,sy;
      tileimg.w = mt->map.grid_link->grid->tile_sx;
      tileimg.h = mt->map.grid_link->grid->tile_sy;
      metatile = geocache_imageio_decode(ctx, mt->map.data);
      if(!metatile) {
         ctx->set_error(ctx, 500, "failed to load image data from metatile");
         return;
      }
      tileimg.stride = metatile->stride;
      for(i=0;i<mt->metasize_x;i++) {
         for(j=0;j<mt->metasize_y;j++) {
            sx = mt->map.tileset->metabuffer + i * tileimg.w;
            sy = mt->map.height - (mt->map.tileset->metabuffer + (j+1) * tileimg.w);
            tileimg.data = &(metatile->data[sy*metatile->stride + 4 * sx]);
            if(mt->map.tileset->watermark) {
                geocache_image_merge(ctx,&tileimg,mt->map.tileset->watermark);
                GC_CHECK_ERROR(ctx);
            }
            mt->tiles[i*mt->metasize_y+j].data =
               mt->map.tileset->format->write(ctx, &tileimg, mt->map.tileset->format);
            GC_CHECK_ERROR(ctx);
         }
      }
   } else {
#ifdef DEBUG
      if(mt->map.tileset->metasize_x != 1 ||
            mt->map.tileset->metasize_y != 1 ||
            mt->map.tileset->metabuffer != 0) {
         ctx->set_error(ctx, 500, "##### BUG ##### using a metatile with no format");
         return;
      }
#endif
      mt->tiles[0].data = mt->map.data;
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

/* vim: ai ts=3 sts=3 et sw=3
*/

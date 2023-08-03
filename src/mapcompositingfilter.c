/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  RFC 113 Layer compositing
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2015 Regents of the University of Minnesota.
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
#include "mapserver.h"
#include <regex.h>
#define pixmove(rb,srcx,srcy,dstx,dsty) \
  memcpy(rb->data.rgba.pixels+dsty*rb->data.rgba.row_step+dstx*4,\
          rb->data.rgba.pixels+srcy*rb->data.rgba.row_step+srcx*4,\
          4)
#define pixerase(rb,x,y) memset(rb->data.rgba.pixels+y*rb->data.rgba.row_step+x*4,0,4)

void msApplyTranslationCompositingFilter(rasterBufferObj *rb, int xtrans, int ytrans) {
  int src_sx,src_sy,dst_sx,dst_sy;
  if((unsigned)abs(xtrans)>=rb->width || (unsigned)abs(ytrans)>=rb->height) {
    for(unsigned y = 0; y<rb->height; y++)
      for(unsigned x = 0; x<rb->width; x++)
        pixerase(rb,x,y);
  }
  if(xtrans == 0 && ytrans == 0)
    return;
  if(xtrans>=0) {
    if(ytrans>=0) {
      src_sx = rb->width - xtrans - 1;
      src_sy = rb->height - ytrans - 1;
      dst_sx = rb->width - 1;
      dst_sy = rb->height -1;
      for(int y = src_sy,dst_y= dst_sy;y>=0;y--,dst_y--) {
        for(int x = src_sx,dst_x= dst_sx;x>=0;x--,dst_x--) {
          pixmove(rb,x,y,dst_x,dst_y);
        }
      }
      for(int y=0;y<ytrans;y++)
        for(unsigned x=0;x<rb->width;x++)
          pixerase(rb,x,y);  
      for(unsigned y=ytrans;y<rb->height;y++)
        for(int x=0;x<xtrans;x++)
          pixerase(rb,x,y);
    } else {
      src_sx = rb->width - xtrans - 1;
      src_sy = - ytrans;
      dst_sx = rb->width - 1;
      dst_sy = 0;
      for(unsigned y = src_sy,dst_y= dst_sy;y<rb->height;y++,dst_y++) {
        for(int x = src_sx,dst_x= dst_sx;x>=0;x--,dst_x--) {
          pixmove(rb,x,y,dst_x,dst_y);
        }
      }
      for(unsigned y=0;y<rb->height+ytrans;y++)
        for(int x=0;x<xtrans;x++)
          pixerase(rb,x,y);  
      for(unsigned y=rb->height+ytrans;y<rb->height;y++)
        for(unsigned x=0;x<rb->width;x++)
          pixerase(rb,x,y);
    }
  } else {
    if(ytrans>=0) {
      src_sx = - xtrans;
      src_sy = rb->height - ytrans - 1;
      dst_sx = 0;
      dst_sy = rb->height -1;
      for(int y = src_sy,dst_y= dst_sy;y>=0;y--,dst_y--) {
        for(unsigned x = src_sx,dst_x= dst_sx;x<rb->width;x++,dst_x++) {
          pixmove(rb,x,y,dst_x,dst_y);
        }
      }
      for(int y=0;y<ytrans;y++)
        for(unsigned x=0;x<rb->width;x++)
          pixerase(rb,x,y);
      for(unsigned y=ytrans;y<rb->height;y++)
        for(unsigned x=rb->width+xtrans;x<rb->width;x++)
          pixerase(rb,x,y);
    } else {
      src_sx = - xtrans;
      src_sy = - ytrans;
      dst_sx = 0;
      dst_sy = 0;
      for(unsigned y = src_sy,dst_y= dst_sy;y<rb->height;y++,dst_y++) {
        for(unsigned x = src_sx,dst_x= dst_sx;x<rb->width;x++,dst_x++) {
          pixmove(rb,x,y,dst_x,dst_y);
        }
      }
      for(unsigned y=0;y<rb->height+ytrans;y++)
        for(unsigned x=rb->width+xtrans;x<rb->width;x++)
          pixerase(rb,x,y);
      for(unsigned y=rb->height+ytrans;y<rb->height;y++)
        for(unsigned x=0;x<rb->width;x++)
          pixerase(rb,x,y);
    }
  }
}

void msApplyBlackeningCompositingFilter(rasterBufferObj *rb) {
  unsigned char *r,*g,*b;
  for(unsigned row=0;row<rb->height;row++) {
    r = rb->data.rgba.r + row*rb->data.rgba.row_step;
    g = rb->data.rgba.g + row*rb->data.rgba.row_step;
    b = rb->data.rgba.b + row*rb->data.rgba.row_step;
    for(unsigned col=0;col<rb->width;col++) {
      *r = *g = *b = 0;
      r+=4;g+=4;b+=4;
    }    
  }
}

void msApplyWhiteningCompositingFilter(rasterBufferObj *rb) {
  unsigned char *r,*g,*b,*a;
  for(unsigned row=0;row<rb->height;row++) {
    r = rb->data.rgba.r + row*rb->data.rgba.row_step;
    g = rb->data.rgba.g + row*rb->data.rgba.row_step;
    b = rb->data.rgba.b + row*rb->data.rgba.row_step;
    a = rb->data.rgba.a + row*rb->data.rgba.row_step;
    for(unsigned col=0;col<rb->width;col++) {
      *r = *g = *b = *a;
      r+=4;g+=4;b+=4;a+=4;
    }    
  }
}

void msApplyGrayscaleCompositingFilter(rasterBufferObj *rb) {
  unsigned char *r,*g,*b;
  for(unsigned row=0;row<rb->height;row++) {
    r = rb->data.rgba.r + row*rb->data.rgba.row_step;
    g = rb->data.rgba.g + row*rb->data.rgba.row_step;
    b = rb->data.rgba.b + row*rb->data.rgba.row_step;
    for(unsigned col=0;col<rb->width;col++) {
      unsigned int mix = (unsigned int)*r + (unsigned int)*g + (unsigned int)*b;
      mix /=3;
      *r = *g = *b = (unsigned char)mix;
      r+=4;g+=4;b+=4;
    }    
  }
}

int msApplyCompositingFilter(mapObj *map, rasterBufferObj *rb, CompositingFilter *filter) {
  int rstatus;
  regex_t regex;
  regmatch_t pmatch[3];
  
  /* test for blurring filter */
  regcomp(&regex, "blur\\(([0-9]+)\\)", REG_EXTENDED);
  rstatus = regexec(&regex, filter->filter, 2, pmatch, 0);
  regfree(&regex);
  if(!rstatus) {
    char *rad = malloc(pmatch[1].rm_eo - pmatch[1].rm_so + 1);
    unsigned int irad;
    strncpy(rad,filter->filter+pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so);
    rad[pmatch[1].rm_eo - pmatch[1].rm_so]=0;
    //msDebug("got blur filter with radius %s\n",rad);
    irad = atoi(rad);
    free(rad);
    irad = MS_NINT(irad*map->resolution/map->defresolution);
    msApplyBlurringCompositingFilter(rb,irad);
    return MS_SUCCESS;
  }
  
  /* test for translation filter */
  regcomp(&regex, "translate\\((-?[0-9]+),(-?[0-9]+)\\)", REG_EXTENDED);
  rstatus = regexec(&regex, filter->filter, 3, pmatch, 0);
  regfree(&regex);
  if(!rstatus) {
    char *num;
    int xtrans,ytrans;
    num = malloc(pmatch[1].rm_eo - pmatch[1].rm_so + 1);
    strncpy(num,filter->filter+pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so);
    num[pmatch[1].rm_eo - pmatch[1].rm_so]=0;
    xtrans = atoi(num);
    free(num);
    num = malloc(pmatch[2].rm_eo - pmatch[2].rm_so + 1);
    strncpy(num,filter->filter+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
    num[pmatch[2].rm_eo - pmatch[2].rm_so]=0;
    ytrans = atoi(num);
    free(num);
    //msDebug("got translation filter of radius %d,%d\n",xtrans,ytrans);
    xtrans = MS_NINT(xtrans*map->resolution/map->defresolution);
    ytrans = MS_NINT(ytrans*map->resolution/map->defresolution);
    msApplyTranslationCompositingFilter(rb,xtrans,ytrans);
    return MS_SUCCESS;
  }
  
  /* test for grayscale filter */
  if(!strncmp(filter->filter,"grayscale()",strlen("grayscale()"))) {
    msApplyGrayscaleCompositingFilter(rb);
    return MS_SUCCESS;
  }
  if(!strncmp(filter->filter,"blacken()",strlen("blacken()"))) {
    msApplyBlackeningCompositingFilter(rb);
    return MS_SUCCESS;
  }
  if(!strncmp(filter->filter,"whiten()",strlen("whiten()"))) {
    msApplyWhiteningCompositingFilter(rb);
    return MS_SUCCESS;
  }
  
  msSetError(MS_MISCERR,"unknown compositing filter (%s)", "msApplyCompositingFilter()", filter->filter);
  return MS_FAILURE;
}

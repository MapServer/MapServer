/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Low level PNG/JPEG/GIF image io native functions
 * Author:   Thomas Bonfort (tbonfort)
 *
 ******************************************************************************
 * Copyright (c) 2009 Thomas Bonfort
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
 ****************************************************************************/

#include "mapserver.h"
#include "png.h"
#include "setjmp.h"
#include <assert.h>
#include "jpeglib.h"
#include <stdlib.h>

#ifdef USE_GIF
#include "gif_lib.h"
#endif



typedef struct _streamInfo {
  FILE *fp;
  bufferObj *buffer;
} streamInfo;

void png_write_data_to_stream(png_structp png_ptr, png_bytep data, png_size_t length)
{
  FILE *fp = ((streamInfo*)png_get_io_ptr(png_ptr))->fp;
  msIO_fwrite(data,length,1,fp);
}

void png_write_data_to_buffer(png_structp png_ptr, png_bytep data, png_size_t length)
{
  bufferObj *buffer = ((streamInfo*)png_get_io_ptr(png_ptr))->buffer;
  msBufferAppend(buffer,data,length);
}

void png_flush_data(png_structp png_ptr)
{
  /* do nothing */
}

typedef struct {
  struct jpeg_destination_mgr pub;
  unsigned char *data;
} ms_destination_mgr;

typedef struct {
  ms_destination_mgr mgr;
  FILE *stream;
} ms_stream_destination_mgr;

typedef struct {
  ms_destination_mgr mgr;
  bufferObj *buffer;
} ms_buffer_destination_mgr;

#define OUTPUT_BUF_SIZE 4096

void
jpeg_init_destination (j_compress_ptr cinfo)
{
  ms_destination_mgr *dest = (ms_destination_mgr*) cinfo->dest;

  /* Allocate the output buffer --- it will be released when done with image */
  dest->data = (unsigned char *)
               (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                   OUTPUT_BUF_SIZE * sizeof (unsigned char));

  dest->pub.next_output_byte = dest->data;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

void jpeg_stream_term_destination (j_compress_ptr cinfo)
{
  ms_stream_destination_mgr *dest = (ms_stream_destination_mgr*) cinfo->dest;
  msIO_fwrite(dest->mgr.data, OUTPUT_BUF_SIZE-dest->mgr.pub.free_in_buffer, 1, dest->stream);
  dest->mgr.pub.next_output_byte = dest->mgr.data;
  dest->mgr.pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

void jpeg_buffer_term_destination (j_compress_ptr cinfo)
{
  ms_buffer_destination_mgr *dest = (ms_buffer_destination_mgr*) cinfo->dest;
  msBufferAppend(dest->buffer, dest->mgr.data, OUTPUT_BUF_SIZE-dest->mgr.pub.free_in_buffer);
  dest->mgr.pub.next_output_byte = dest->mgr.data;
  dest->mgr.pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

int jpeg_stream_empty_output_buffer (j_compress_ptr cinfo)
{
  ms_stream_destination_mgr *dest = (ms_stream_destination_mgr*) cinfo->dest;
  msIO_fwrite(dest->mgr.data, OUTPUT_BUF_SIZE, 1, dest->stream);
  dest->mgr.pub.next_output_byte = dest->mgr.data;
  dest->mgr.pub.free_in_buffer = OUTPUT_BUF_SIZE;
  return TRUE;
}

int jpeg_buffer_empty_output_buffer (j_compress_ptr cinfo)
{
  ms_buffer_destination_mgr *dest = (ms_buffer_destination_mgr*) cinfo->dest;
  msBufferAppend(dest->buffer, dest->mgr.data, OUTPUT_BUF_SIZE);
  dest->mgr.pub.next_output_byte = dest->mgr.data;
  dest->mgr.pub.free_in_buffer = OUTPUT_BUF_SIZE;
  return TRUE;
}


int saveAsJPEG(mapObj *map /*not used*/, rasterBufferObj *rb, streamInfo *info,
               outputFormatObj *format)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  int quality = atoi(msGetOutputFormatOption( format, "QUALITY", "75"));
  ms_destination_mgr *dest;
  JSAMPLE *rowdata;
  unsigned int row;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  if (cinfo.dest == NULL) {
    if(info->fp) {
      cinfo.dest = (struct jpeg_destination_mgr *)
                   (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
                                              sizeof (ms_stream_destination_mgr));
      ((ms_stream_destination_mgr*)cinfo.dest)->mgr.pub.empty_output_buffer = jpeg_stream_empty_output_buffer;
      ((ms_stream_destination_mgr*)cinfo.dest)->mgr.pub.term_destination = jpeg_stream_term_destination;
      ((ms_stream_destination_mgr*)cinfo.dest)->stream = info->fp;
    } else {

      cinfo.dest = (struct jpeg_destination_mgr *)
                   (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
                                              sizeof (ms_buffer_destination_mgr));
      ((ms_buffer_destination_mgr*)cinfo.dest)->mgr.pub.empty_output_buffer = jpeg_buffer_empty_output_buffer;
      ((ms_buffer_destination_mgr*)cinfo.dest)->mgr.pub.term_destination = jpeg_buffer_term_destination;
      ((ms_buffer_destination_mgr*)cinfo.dest)->buffer = info->buffer;
    }
  }
  dest = (ms_destination_mgr*) cinfo.dest;
  dest->pub.init_destination = jpeg_init_destination;

  cinfo.image_width = rb->width;
  cinfo.image_height = rb->height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);
  jpeg_start_compress(&cinfo, TRUE);

  rowdata = (JSAMPLE*)malloc(rb->width*cinfo.input_components*sizeof(JSAMPLE));
  for(row=0; row<rb->height; row++) {
    JSAMPLE *pixptr = rowdata;
    int col;
    unsigned char *r,*g,*b;
    r=rb->data.rgba.r+row*rb->data.rgba.row_step;
    g=rb->data.rgba.g+row*rb->data.rgba.row_step;
    b=rb->data.rgba.b+row*rb->data.rgba.row_step;
    for(col=0; col<rb->width; col++) {
      *(pixptr++) = *r;
      *(pixptr++) = *g;
      *(pixptr++) = *b;
      r+=rb->data.rgba.pixel_step;
      g+=rb->data.rgba.pixel_step;
      b+=rb->data.rgba.pixel_step;
    }
    (void) jpeg_write_scanlines(&cinfo, &rowdata, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  free(rowdata);
  return MS_SUCCESS;
}

/*
 * sort a given list of rgba entries so that all the opaque pixels are at the end
 */
int remapPaletteForPNG(rasterBufferObj *rb, rgbPixel *rgb, unsigned char *a, int *num_a)
{
  int bot_idx, top_idx, x;
  int remap[256];
  unsigned int maxval = rb->data.palette.scaling_maxval;

  assert(rb->type == MS_BUFFER_BYTE_PALETTE);

  /*
  ** remap the palette colors so that all entries with
  ** the maximal alpha value (i.e., fully opaque) are at the end and can
  ** therefore be omitted from the tRNS chunk.  Note that the ordering of
  ** opaque entries is reversed from how they were previously arranged
  ** --not that this should matter to anyone.
  */

  for (top_idx = rb->data.palette.num_entries-1, bot_idx = x = 0;  x < rb->data.palette.num_entries;  ++x) {
    if (rb->data.palette.palette[x].a == maxval)
      remap[x] = top_idx--;
    else
      remap[x] = bot_idx++;
  }
  /* sanity check:  top and bottom indices should have just crossed paths */
  if (bot_idx != top_idx + 1) {
    msSetError(MS_MISCERR,"quantization sanity check failed","createPNGPalette()");
    return MS_FAILURE;
  }

  *num_a = bot_idx;

  for(x=0; x<rb->width*rb->height; x++)
    rb->data.palette.pixels[x] = remap[rb->data.palette.pixels[x]];


  for (x = 0; x < rb->data.palette.num_entries; ++x) {
    if(maxval == 255) {
      a[remap[x]] = rb->data.palette.palette[x].a;
      rgb[remap[x]].r = rb->data.palette.palette[x].r;
      rgb[remap[x]].g = rb->data.palette.palette[x].g;
      rgb[remap[x]].b = rb->data.palette.palette[x].b;
    } else {
      /* rescale palette */
      rgb[remap[x]].r = (rb->data.palette.palette[x].r * 255 + (maxval >> 1)) / maxval;
      rgb[remap[x]].g = (rb->data.palette.palette[x].g * 255 + (maxval >> 1)) / maxval;
      rgb[remap[x]].b = (rb->data.palette.palette[x].b * 255 + (maxval >> 1)) / maxval;
      a[remap[x]] = (rb->data.palette.palette[x].a * 255 + (maxval >> 1)) / maxval;
    }
    if(a[remap[x]] != 255) {
      /* un-premultiply pixels */
      double da = 255.0/a[remap[x]];
      rgb[remap[x]].r *=  da;
      rgb[remap[x]].g *=  da;
      rgb[remap[x]].b *=  da;
    }
  }

  return MS_SUCCESS;
}

int savePalettePNG(rasterBufferObj *rb, streamInfo *info, int compression)
{
  png_infop info_ptr;
  rgbPixel rgb[256];
  unsigned char a[256];
  int num_a;
  int row,sample_depth;
  png_structp png_ptr = png_create_write_struct(
                          PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);

  assert(rb->type == MS_BUFFER_BYTE_PALETTE);

  if (!png_ptr)
    return (MS_FAILURE);

  png_set_compression_level(png_ptr, compression);
  png_set_filter (png_ptr,0, PNG_FILTER_NONE);

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr,
                             (png_infopp)NULL);
    return (MS_FAILURE);
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return (MS_FAILURE);
  }
  if(info->fp)
    png_set_write_fn(png_ptr,info, png_write_data_to_stream, png_flush_data);
  else
    png_set_write_fn(png_ptr,info, png_write_data_to_buffer, png_flush_data);


  if (rb->data.palette.num_entries <= 2)
    sample_depth = 1;
  else if (rb->data.palette.num_entries <= 4)
    sample_depth = 2;
  else if (rb->data.palette.num_entries <= 16)
    sample_depth = 4;
  else
    sample_depth = 8;

  png_set_IHDR(png_ptr, info_ptr, rb->width, rb->height,
               sample_depth, PNG_COLOR_TYPE_PALETTE,
               0, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  remapPaletteForPNG(rb,rgb,a,&num_a);

  png_set_PLTE(png_ptr, info_ptr, (png_colorp)(rgb),rb->data.palette.num_entries);
  if(num_a)
    png_set_tRNS(png_ptr, info_ptr, a,num_a, NULL);

  png_write_info(png_ptr, info_ptr);
  png_set_packing(png_ptr);

  for(row=0; row<rb->height; row++) {
    unsigned char *rowptr = &(rb->data.palette.pixels[row*rb->width]);
    png_write_row(png_ptr, rowptr);
  }
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  return MS_SUCCESS;
}

int readPalette(const char *palette, rgbaPixel *entries, unsigned int *nEntries, int useAlpha)
{
  FILE *stream = NULL;
  char buffer[MS_BUFFER_LENGTH];
  int r,g,b,a;
  *nEntries = 0;

  stream = fopen(palette, "r");
  if(!stream) {
    msSetError(MS_IOERR, "Error opening palette file %s.", "readPalette()", palette);
    return MS_FAILURE;
  }

  while(fgets(buffer, MS_BUFFER_LENGTH, stream) && *nEntries<256) {
    /* while there are colors to load */
    if(buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\r')
      continue; /* skip comments and blank lines */
    if(!useAlpha) {
      if(3 != sscanf(buffer,"%d,%d,%d\n",&r,&g,&b)) {
        msSetError(MS_MISCERR,"failed to parse color %d r,g,b triplet in line \"%s\" from file %s","readPalette()",*nEntries+1,buffer,palette);
        return MS_FAILURE;
      }
    } else {
      if(4 != sscanf(buffer,"%d,%d,%d,%d\n",&r,&g,&b,&a)) {
        msSetError(MS_MISCERR,"failed to parse color %d r,g,b,a quadruplet in line \"%s\" from file %s","readPalette()",*nEntries+1,buffer,palette);
        return MS_FAILURE;
      }
    }
    if(useAlpha && a != 255) {
      double da = a/255.0;
      entries[*nEntries].r = r * da;
      entries[*nEntries].g = g * da;
      entries[*nEntries].b = b * da;
      entries[*nEntries].a = a;
    } else {
      entries[*nEntries].r = r;
      entries[*nEntries].g = g;
      entries[*nEntries].b = b;
      entries[*nEntries].a = 255;
    }
    (*nEntries)++;
  }
  fclose(stream);
  return MS_SUCCESS;
}

int saveAsPNG(mapObj *map,rasterBufferObj *rb, streamInfo *info, outputFormatObj *format)
{
  int force_pc256 = MS_FALSE;
  int force_palette = MS_FALSE;

  int ret = MS_FAILURE;

  const char *force_string,*zlib_compression;
  int compression = -1;

  zlib_compression = msGetOutputFormatOption( format, "COMPRESSION", NULL);
  if(zlib_compression && *zlib_compression) {
    char *endptr;
    compression = strtol(zlib_compression,&endptr,10);
    if(*endptr || compression<-1 || compression>9) {
      msSetError(MS_MISCERR,"failed to parse FORMATOPTION \"COMPRESSION=%s\", expecting integer from 0 to 9.","saveAsPNG()",zlib_compression);
      return MS_FAILURE;
    }
  }


  force_string = msGetOutputFormatOption( format, "QUANTIZE_FORCE", NULL );
  if( force_string && (strcasecmp(force_string,"on") == 0  || strcasecmp(force_string,"yes") == 0 || strcasecmp(force_string,"true") == 0) )
    force_pc256 = MS_TRUE;

  force_string = msGetOutputFormatOption( format, "PALETTE_FORCE", NULL );
  if( force_string && (strcasecmp(force_string,"on") == 0  || strcasecmp(force_string,"yes") == 0 || strcasecmp(force_string,"true") == 0) )
    force_palette = MS_TRUE;

  if(force_pc256 || force_palette) {
    rasterBufferObj qrb;
    rgbaPixel palette[256], paletteGiven[256];
    unsigned int numPaletteGivenEntries;
    memset(&qrb,0,sizeof(rasterBufferObj));
    qrb.type = MS_BUFFER_BYTE_PALETTE;
    qrb.width = rb->width;
    qrb.height = rb->height;
    qrb.data.palette.pixels = (unsigned char*)malloc(qrb.width*qrb.height*sizeof(unsigned char));
    qrb.data.palette.scaling_maxval = 255;
    if(force_pc256) {
      qrb.data.palette.palette = palette;
      qrb.data.palette.num_entries = atoi(msGetOutputFormatOption( format, "QUANTIZE_COLORS", "256"));
      ret = msQuantizeRasterBuffer(rb,&(qrb.data.palette.num_entries),qrb.data.palette.palette,
                                   NULL, 0,
                                   &qrb.data.palette.scaling_maxval);
    } else {
      int colorsWanted = atoi(msGetOutputFormatOption( format, "QUANTIZE_COLORS", "0"));
      const char *palettePath = msGetOutputFormatOption( format, "PALETTE", "palette.txt");
      char szPath[MS_MAXPATHLEN];
      if(map) {
        msBuildPath(szPath, map->mappath, palettePath);
        palettePath = szPath;
      }
      if(readPalette(palettePath,paletteGiven,&numPaletteGivenEntries,format->transparent) != MS_SUCCESS) {
        return MS_FAILURE;
      }

      if(numPaletteGivenEntries == 256 || colorsWanted == 0) {
        qrb.data.palette.palette = paletteGiven;
        qrb.data.palette.num_entries = numPaletteGivenEntries;
        ret = MS_SUCCESS;

        /* we have a full palette and don't want an additional quantization step */
      } else {
        /* quantize the image, and mix our colours in the resulting palette */
        qrb.data.palette.palette = palette;
        qrb.data.palette.num_entries = MS_MAX(colorsWanted,numPaletteGivenEntries);
        ret = msQuantizeRasterBuffer(rb,&(qrb.data.palette.num_entries),qrb.data.palette.palette,
                                     paletteGiven,numPaletteGivenEntries,
                                     &qrb.data.palette.scaling_maxval);
      }
    }
    if(ret != MS_FAILURE) {
      ret = msClassifyRasterBuffer(rb,&qrb);
      ret = savePalettePNG(&qrb,info,compression);
    }
    msFree(qrb.data.palette.pixels);
    return ret;
  } else if(rb->type == MS_BUFFER_BYTE_RGBA) {
    png_infop info_ptr;
    int color_type;
    int row;
    unsigned int *rowdata;
    png_structp png_ptr = png_create_write_struct(
                            PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);

    if (!png_ptr)
      return (MS_FAILURE);

    png_set_compression_level(png_ptr, compression);
    png_set_filter (png_ptr,0, PNG_FILTER_NONE);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
      png_destroy_write_struct(&png_ptr,
                               (png_infopp)NULL);
      return (MS_FAILURE);
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return (MS_FAILURE);
    }
    if(info->fp)
      png_set_write_fn(png_ptr,info, png_write_data_to_stream, png_flush_data);
    else
      png_set_write_fn(png_ptr,info, png_write_data_to_buffer, png_flush_data);

    if(rb->data.rgba.a)
      color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    else
      color_type = PNG_COLOR_TYPE_RGB;

    png_set_IHDR(png_ptr, info_ptr, rb->width, rb->height,
                 8, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    if(!rb->data.rgba.a && rb->data.rgba.pixel_step==4)
      png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

    rowdata = (unsigned int*)malloc(rb->width*sizeof(unsigned int));
    for(row=0; row<rb->height; row++) {
      unsigned int *pixptr = rowdata;
      int col;
      unsigned char *a,*r,*g,*b;
      r=rb->data.rgba.r+row*rb->data.rgba.row_step;
      g=rb->data.rgba.g+row*rb->data.rgba.row_step;
      b=rb->data.rgba.b+row*rb->data.rgba.row_step;
      if(rb->data.rgba.a) {
        a=rb->data.rgba.a+row*rb->data.rgba.row_step;
        for(col=0; col<rb->width; col++) {
          if(*a) {
            double da = *a/255.0;
            unsigned char *pix = (unsigned char*)pixptr;
            pix[0] = *r/da;
            pix[1] = *g/da;
            pix[2] = *b/da;
            pix[3] = *a;
          } else {
            *pixptr=0;
          }
          pixptr++;
          a+=rb->data.rgba.pixel_step;
          r+=rb->data.rgba.pixel_step;
          g+=rb->data.rgba.pixel_step;
          b+=rb->data.rgba.pixel_step;
        }
      } else {
        for(col=0; col<rb->width; col++) {
          unsigned char *pix = (unsigned char*)pixptr;
          pix[0] = *r;
          pix[1] = *g;
          pix[2] = *b;
          pixptr++;
          r+=rb->data.rgba.pixel_step;
          g+=rb->data.rgba.pixel_step;
          b+=rb->data.rgba.pixel_step;
        }
      }

      png_write_row(png_ptr,(png_bytep)rowdata);

    }
    png_write_end(png_ptr, info_ptr);
    free(rowdata);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return MS_SUCCESS;
  } else {
    msSetError(MS_MISCERR,"Unknown buffer type","saveAsPNG()");
    return MS_FAILURE;
  }
}

/* For platforms with incomplete ANSI defines. Fortunately,
   SEEK_SET is defined to be zero by the standard. */

#ifndef SEEK_SET
#define SEEK_SET 0
#endif /* SEEK_SET */



#ifdef USE_GD
/* #include <math.h> */
/* #include <string.h> */
/* #include <stdlib.h> */
#include "gd.h"

/* this is used for creating images in main memory */

typedef struct fileIOCtx {
  gdIOCtx ctx;
  FILE *f;
}
fileIOCtx;

static void msFreeFileCtx (gdIOCtx * ctx)
{
  free(ctx);
}

static int filePutbuf (gdIOCtx * ctx, const void *buf, int size)
{
  fileIOCtx *fctx;
  fctx = (fileIOCtx *) ctx;

  return fwrite (buf, 1, size, fctx->f);
}

static int fileGetbuf (gdIOCtx * ctx, void *buf, int size)
{
  fileIOCtx *fctx;
  fctx = (fileIOCtx *) ctx;

  return (msIO_fread (buf, 1, size, fctx->f));

}

static void filePutchar (gdIOCtx * ctx, int a)
{
  unsigned char b;
  fileIOCtx *fctx;
  fctx = (fileIOCtx *) ctx;

  b = a;

  putc (b, fctx->f);
}

static int fileGetchar (gdIOCtx * ctx)
{
  fileIOCtx *fctx;
  fctx = (fileIOCtx *) ctx;

  return getc (fctx->f);
}


static int fileSeek (struct gdIOCtx *ctx, const int pos)
{
  fileIOCtx *fctx;
  fctx = (fileIOCtx *) ctx;
  return (fseek (fctx->f, pos, SEEK_SET) == 0);
}

static long fileTell (struct gdIOCtx *ctx)
{
  fileIOCtx *fctx;
  fctx = (fileIOCtx *) ctx;

  return ftell (fctx->f);
}

/* return data as a dynamic pointer */
gdIOCtx *msNewGDFileCtx (FILE * f)
{
  fileIOCtx *ctx;

  ctx = (fileIOCtx *) malloc (sizeof (fileIOCtx));
  if (ctx == NULL) {
    return NULL;
  }

  ctx->f = f;

  ctx->ctx.getC = fileGetchar;
  ctx->ctx.putC = filePutchar;

  ctx->ctx.getBuf = fileGetbuf;
  ctx->ctx.putBuf = filePutbuf;

  ctx->ctx.tell = fileTell;
  ctx->ctx.seek = fileSeek;

  ctx->ctx.gd_free = msFreeFileCtx;

  return (gdIOCtx *) ctx;
}

static const unsigned char PNGsig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; /* 89 50 4E 47 0D 0A 1A 0A hex */

int msLoadGDRasterBufferFromFile(char *path, rasterBufferObj *rb)
{
  FILE *stream;
  char bytes[8];
  gdImagePtr img = NULL;
  stream = fopen(path,"rb");
  if(!stream) {
    msSetError(MS_MISCERR, "unable to open file %s for reading", "loadGDImg()", path);
    return MS_FAILURE;
  }
  if(1 != fread(bytes,8,1,stream)) { /* read some bytes to try and identify the file */
    msSetError(MS_MISCERR, "Unable to read header from image file %s", "msLoadGDRasterBufferFromFile()",path);
    return MS_FAILURE;
  }
  rewind(stream); /* reset the image for the readers */
  if(memcmp(bytes,"GIF8",4)==0) {
#ifdef USE_GD_GIF
    gdIOCtx *ctx;
    ctx = msNewGDFileCtx(stream);
    img = gdImageCreateFromGifCtx(ctx);
    ctx->gd_free(ctx);
#else
    msSetError(MS_MISCERR, "Unable to load GIF symbol.", "msLoadGDRasterBufferFromFile()");
    return MS_FAILURE;
#endif
  } else if (memcmp(bytes,PNGsig,8)==0) {
#ifdef USE_GD_PNG
    gdIOCtx *ctx;
    ctx = msNewGDFileCtx(stream);
    img = gdImageCreateFromPngCtx(ctx);
    ctx->gd_free(ctx);
#else
    msSetError(MS_MISCERR, "Unable to load PNG symbol.", "msLoadGDRasterBufferFromFile()");
    return MS_FAILURE;
#endif
  }

  fclose(stream);

  if(!img) {
    msSetError(MS_GDERR, NULL, "loadGDImg()");
    rb->type = MS_BUFFER_NONE;
    return MS_FAILURE;
  }
  if(gdImageTrueColor(img)) {
    int x,y;
    gdImagePtr pimg = gdImageCreate(gdImageSX(img),gdImageSY(img));
    gdImageColorAllocateAlpha(pimg,0,0,0,127);
    for(y = 0 ; y < gdImageSY(img); y++) {
      for(x = 0; x < gdImageSX(img); x++) {
        int pix = gdImageGetTrueColorPixel(img,x,y);
        if(gdTrueColorGetAlpha(pix) == 127) {
          gdImageSetPixel(pimg,x,y,0);
          pimg->transparent = 0;
        } else {
          gdImageSetPixel(pimg,x,y,gdImageColorResolveAlpha(pimg,gdTrueColorGetRed(pix),gdTrueColorGetGreen(pix),
                          gdTrueColorGetBlue(pix),gdTrueColorGetAlpha(pix)));
        }

      }
    }
    gdImageDestroy(img);
    img = pimg;
  }
  rb->type = MS_BUFFER_GD;
  rb->width = gdImageSX(img);
  rb->height = gdImageSY(img);
  rb->data.gd_img = img;
  return MS_SUCCESS;
}
#endif

int readPNG(char *path, rasterBufferObj *rb)
{
  png_uint_32 width,height;
  unsigned char *a,*r,*g,*b;
  int bit_depth,color_type,i;
  unsigned char **row_pointers;
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;

  FILE *stream = fopen(path,"rb");
  if(!stream)
    return MS_FAILURE;

  /* could pass pointers to user-defined error handlers instead of NULLs: */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fclose(stream);
    return MS_FAILURE;   /* out of memory */
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(stream);
    return MS_FAILURE;   /* out of memory */
  }

  if(setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
    fclose(stream);
    return MS_FAILURE;
  }

  png_init_io(png_ptr,stream);
  png_read_info(png_ptr,info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height,
               &bit_depth, &color_type,
               NULL,NULL,NULL);

  rb->width = width;
  rb->height = height;
  rb->type = MS_BUFFER_BYTE_RGBA;
  rb->data.rgba.pixels = (unsigned char*)malloc(width*height*4*sizeof(unsigned char));
  row_pointers = (unsigned char**)malloc(height*sizeof(unsigned char*));
  rb->data.rgba.pixel_step=4;
  rb->data.rgba.row_step = width*4;
  b = rb->data.rgba.b = &rb->data.rgba.pixels[0];
  g = rb->data.rgba.g = &rb->data.rgba.pixels[1];
  r = rb->data.rgba.r = &rb->data.rgba.pixels[2];
  a = rb->data.rgba.a = &rb->data.rgba.pixels[3];

  for(i=0; i<height; i++) {
    row_pointers[i] = &(rb->data.rgba.pixels[i*rb->data.rgba.row_step]);
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

  png_set_bgr(png_ptr);
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);

  png_read_update_info(png_ptr, info_ptr);
  assert(png_get_rowbytes(png_ptr, info_ptr) == rb->data.rgba.row_step);

  png_read_image(png_ptr, row_pointers);
  free(row_pointers);
  row_pointers=NULL;
  png_read_end(png_ptr,NULL);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  /*premultiply data*/
  for(i=0; i<width*height; i++) {
    if(*a < 255) {
      if(*a == 0) {
        *r=*g=*b=0;
      } else {
        *r = (*r * *a + 255) >> 8;
        *g = (*g * *a + 255) >> 8;
        *b = (*b * *a + 255) >> 8;
      }
    }
    a+=4;
    b+=4;
    g+=4;
    r+=4;
  }

  fclose(stream);
  return MS_SUCCESS;

}

#ifdef USE_GD
int saveGdImage(gdImagePtr ip, FILE *fp, outputFormatObj *format)
{
  gdIOCtx *ctx = NULL;

  if (fp && (fp == stdout))
    ctx = msIO_getGDIOCtx( fp );

  if(strcasecmp("ON", msGetOutputFormatOption(format, "INTERLACE", "ON")) == 0)
    gdImageInterlace(ip, 1);

  if(format->transparent)
    gdImageColorTransparent(ip, 0);

  if(strcasestr(format->driver, "/gif")) {
#ifdef USE_GD_GIF
    if (ctx)
      gdImageGifCtx(ip, ctx);
    else
      gdImageGif(ip, fp);
#else
    msSetError(MS_MISCERR, "GIF output is not available.", "saveImageGD()");
    return(MS_FAILURE);
#endif
  } else if(strcasestr(format->driver, "/png")) {
#ifdef USE_GD_PNG
    if (ctx)
      gdImagePngCtx(ip, ctx);
    else
      gdImagePng(ip, fp);
#else
    msSetError(MS_MISCERR, "PNG output is not available.", "saveImageGD()");
    return(MS_FAILURE);
#endif
  } else if(strcasestr(format->driver, "/jpeg")) {
#ifdef USE_GD_JPEG
    if (ctx)
      gdImageJpegCtx(ip, ctx, atoi(msGetOutputFormatOption( format, "QUALITY", "75")));
    else
      gdImageJpeg(ip, fp, atoi(msGetOutputFormatOption( format, "QUALITY", "75")));
#else
    msSetError(MS_MISCERR, "JPEG output is not available.", "saveImageGD()");
    return(MS_FAILURE);
#endif
  } else {
    msSetError(MS_MISCERR, "Unknown or unsupported format.", "saveImageGD()");
    return(MS_FAILURE);
  }
  msFree(ctx);

  return MS_SUCCESS;
}

int saveGdImageBuffer(gdImagePtr ip, bufferObj *buffer, outputFormatObj *format)
{
  gdIOCtx *ctx;
  int tmp_size;

  ctx = gdNewDynamicCtx (2048, NULL);

  if( format->imagemode == MS_IMAGEMODE_RGBA )
    gdImageSaveAlpha( ip, 1 );
  else if( format->imagemode == MS_IMAGEMODE_RGB )
    gdImageSaveAlpha( ip, 0 );

  if(strcasecmp("ON", msGetOutputFormatOption(format, "INTERLACE", "ON")) == 0)
    gdImageInterlace(ip, 1);

  if(format->transparent)
    gdImageColorTransparent(ip, 0);

  if(strcasestr(format->driver, "/gif")) {
#ifdef USE_GD_GIF
    gdImageGifCtx( ip, ctx );
#else
    msSetError(MS_MISCERR, "GIF output is not available.", "saveImageGD()");
    ctx->gd_free(ctx);
    return(MS_FAILURE);
#endif
  } else if(strcasestr(format->driver, "/png")) {
#ifdef USE_GD_PNG
    gdImagePngCtx(ip, ctx);
#else
    msSetError(MS_MISCERR, "PNG output is not available.", "saveImageGD()");
    ctx->gd_free(ctx);
    return(MS_FAILURE);
#endif
  } else if(strcasestr(format->driver, "/jpeg")) {
#ifdef USE_GD_JPEG
    gdImageJpegCtx(ip, ctx, atoi(msGetOutputFormatOption( format, "QUALITY", "75")));
#else
    msSetError(MS_MISCERR, "JPEG output is not available.", "saveImageGD()");
    ctx->gd_free(ctx);
    return(MS_FAILURE);
#endif
  } else {
    msSetError(MS_MISCERR, "Unknown or unsupported format.", "saveImageGD()");
    ctx->gd_free(ctx);
    return(MS_FAILURE);
  }

  /* gdDPExtractData expects a int*, but bufferObj::size is a size_t */
  /* so use a temp variable to hold it */
  buffer->data = gdDPExtractData (ctx, &tmp_size);
  buffer->size = tmp_size;

  ctx->gd_free(ctx);
  return MS_SUCCESS;
}
#endif

int msSaveRasterBuffer(mapObj *map, rasterBufferObj *rb, FILE *stream,
                       outputFormatObj *format)
{
#ifdef USE_GD
  if(rb->type == MS_BUFFER_GD) {
    return saveGdImage(rb->data.gd_img, stream, format);
  }
#else
  assert(rb->type != MS_BUFFER_GD);
#endif
  if(strcasestr(format->driver,"/png")) {
    streamInfo info;
    info.fp = stream;
    info.buffer = NULL;

    return saveAsPNG(map, rb,&info,format);
  } else if(strcasestr(format->driver,"/jpeg")) {
    streamInfo info;
    info.fp = stream;
    info.buffer=NULL;
    return saveAsJPEG(map, rb,&info,format);
  } else {
    msSetError(MS_MISCERR,"unsupported image format\n", "msSaveRasterBuffer()");
    return MS_FAILURE;
  }
}

int msSaveRasterBufferToBuffer(rasterBufferObj *data, bufferObj *buffer,
                               outputFormatObj *format)
{
#ifdef USE_GD
  if(data->type == MS_BUFFER_GD) {
    return saveGdImageBuffer(data->data.gd_img, buffer, format);
  }
#else
  assert(data->type != MS_BUFFER_GD);
#endif
  if(strcasestr(format->driver,"/png")) {
    streamInfo info;
    info.fp = NULL;
    info.buffer = buffer;
    return saveAsPNG(NULL, data,&info,format);
  } else if(strcasestr(format->driver,"/jpeg")) {
    streamInfo info;
    info.fp = NULL;
    info.buffer=buffer;
    return saveAsJPEG(NULL, data,&info,format);
  } else {
    msSetError(MS_MISCERR,"unsupported image format\n", "msSaveRasterBuffer()");
    return MS_FAILURE;
  }
}

#ifdef USE_GIF
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
static char const *gif_error_msg(int code)
#else
static char const *gif_error_msg()
#endif
{
  static char msg[80];

#if (!defined GIFLIB_MAJOR) || (GIFLIB_MAJOR < 5)
  int code = GifLastError();
#endif
  switch (code) {
    case E_GIF_ERR_OPEN_FAILED: /* should not see this */
      return "Failed to open given file";

    case E_GIF_ERR_WRITE_FAILED:
      return "Write failed";

    case E_GIF_ERR_HAS_SCRN_DSCR: /* should not see this */
      return "Screen descriptor already passed to giflib";

    case E_GIF_ERR_HAS_IMAG_DSCR: /* should not see this */
      return "Image descriptor already passed to giflib";

    case E_GIF_ERR_NO_COLOR_MAP: /* should not see this */
      return "Neither global nor local color map set";

    case E_GIF_ERR_DATA_TOO_BIG: /* should not see this */
      return "Too much pixel data passed to giflib";

    case E_GIF_ERR_NOT_ENOUGH_MEM:
      return "Out of memory";

    case E_GIF_ERR_DISK_IS_FULL:
      return "Disk is full";

    case E_GIF_ERR_CLOSE_FAILED: /* should not see this */
      return "File close failed";

    case E_GIF_ERR_NOT_WRITEABLE: /* should not see this */
      return "File not writable";

    case D_GIF_ERR_OPEN_FAILED:
      return "Failed to open file";

    case D_GIF_ERR_READ_FAILED:
      return "Failed to read from file";

    case D_GIF_ERR_NOT_GIF_FILE:
      return "File is not a GIF file";

    case D_GIF_ERR_NO_SCRN_DSCR:
      return "No screen descriptor detected - invalid file";

    case D_GIF_ERR_NO_IMAG_DSCR:
      return "No image descriptor detected - invalid file";

    case D_GIF_ERR_NO_COLOR_MAP:
      return "No global or local color map found";

    case D_GIF_ERR_WRONG_RECORD:
      return "Wrong record type detected - invalid file?";

    case D_GIF_ERR_DATA_TOO_BIG:
      return "Data in file too big for image";

    case D_GIF_ERR_NOT_ENOUGH_MEM:
      return "Out of memory";

    case D_GIF_ERR_CLOSE_FAILED:
      return "Close failed";

    case D_GIF_ERR_NOT_READABLE:
      return "File not opened for read";

    case D_GIF_ERR_IMAGE_DEFECT:
      return "Defective image";

    case D_GIF_ERR_EOF_TOO_SOON:
      return "Unexpected EOF - invalid file";

    default:
      sprintf(msg, "Unknown giflib error code %d", code);
      return msg;
  }
}


/* not fully implemented and tested */
/* missing: set the first pointer to a,r,g,b */
int readGIF(char *path, rasterBufferObj *rb)
{
  int i, j, codeSize, extCode, firstImageRead = MS_FALSE;
  unsigned char *r,*g,*b,*a;
  int transIdx = -1;
  GifFileType *image;
  GifPixelType *line;
  GifRecordType recordType;
  GifByteType *codeBlock, *extension;
  ColorMapObject *cmap;
  int interlacedOffsets[] = {0,4,2,1};
  int interlacedJumps[] = {8,8,4,2};
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
  int errcode;
#endif


  rb->type = MS_BUFFER_BYTE_RGBA;
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
  image =  DGifOpenFileName(path, &errcode);
  if (image == NULL) {
    msSetError(MS_MISCERR,"failed to load gif image: %s","readGIF()", gif_error_msg(errcode));
    return MS_FAILURE;
  }
#else
  image =  DGifOpenFileName(path);
  if (image == NULL) {
    msSetError(MS_MISCERR,"failed to load gif image: %s","readGIF()", gif_error_msg());
    return MS_FAILURE;
  }
#endif
  rb->width = image->SWidth;
  rb->height = image->SHeight;
  rb->data.rgba.row_step = rb->width * 4;
  rb->data.rgba.pixel_step = 4;
  rb->data.rgba.pixels = (unsigned char*)malloc(rb->width*rb->height*4*sizeof(unsigned char));
  b = rb->data.rgba.b = &rb->data.rgba.pixels[0];
  g = rb->data.rgba.g = &rb->data.rgba.pixels[1];
  r = rb->data.rgba.r = &rb->data.rgba.pixels[2];
  a = rb->data.rgba.a = &rb->data.rgba.pixels[3];

  cmap = (image->Image.ColorMap)?image->Image.ColorMap:image->SColorMap;
  for(i=0; i<rb->width*rb->height; i++) {
    *a = 255;
    *r = cmap->Colors[image->SBackGroundColor].Red;
    *g = cmap->Colors[image->SBackGroundColor].Green;
    *b = cmap->Colors[image->SBackGroundColor].Blue;
    a+=rb->data.rgba.pixel_step;
    r+=rb->data.rgba.pixel_step;
    g+=rb->data.rgba.pixel_step;
    b+=rb->data.rgba.pixel_step;
  }

  do {
    if (DGifGetRecordType(image, &recordType) == GIF_ERROR) {
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
      msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg(image->Error));
#else
      msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg());
#endif
      return MS_FAILURE;
    }

    switch (recordType) {
      case SCREEN_DESC_RECORD_TYPE:
        DGifGetScreenDesc(image);
        break;
      case IMAGE_DESC_RECORD_TYPE:
        if (DGifGetImageDesc(image) == GIF_ERROR) {
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
          msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg(image->Error));
#else
          msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg());
#endif
          return MS_FAILURE;
        }
        if (!firstImageRead) {

          int row = image->Image.Top;
          int col = image->Image.Left;
          int width = image->Image.Width;
          int height = image->Image.Height;

          /* sanity check: */
          if(col+width>rb->width || row+height>rb->height) {
            msSetError(MS_MISCERR,"corrupted gif image, img size exceeds screen size","readGIF()");
            return MS_FAILURE;
          }

          line = (GifPixelType *) malloc(width * sizeof(GifPixelType));
          if(image->Image.Interlace) {
            int count;
            for(count=0; count<4; count++) {
              for(i=row+interlacedOffsets[count]; i<row+height; i+=interlacedJumps[count]) {
                int offset = i * rb->data.rgba.row_step + col * rb->data.rgba.pixel_step;
                a = rb->data.rgba.a + offset;
                r = rb->data.rgba.r + offset;
                g = rb->data.rgba.g + offset;
                b = rb->data.rgba.b + offset;
                if (DGifGetLine(image, line, width) == GIF_ERROR) {
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
                  msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()",gif_error_msg(image->Error));
#else
                  msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()",gif_error_msg());
#endif
                  return MS_FAILURE;
                }

                for(j=0; j<width; j++) {
                  GifPixelType pix = line[j];
                  if(transIdx == -1 || pix != transIdx) {
                    *a = 255;
                    *r = cmap->Colors[pix].Red;
                    *g = cmap->Colors[pix].Green;
                    *b = cmap->Colors[pix].Blue;
                  } else {
                    *a = *r = *g = *b = 0;
                  }
                  a+=rb->data.rgba.pixel_step;
                  r+=rb->data.rgba.pixel_step;
                  g+=rb->data.rgba.pixel_step;
                  b+=rb->data.rgba.pixel_step;
                }
              }
            }
          } else {
            for (i = 0; i < height; i++) {
              int offset = i * rb->data.rgba.row_step + col * rb->data.rgba.pixel_step;
              a = rb->data.rgba.a + offset;
              r = rb->data.rgba.r + offset;
              g = rb->data.rgba.g + offset;
              b = rb->data.rgba.b + offset;
              if (DGifGetLine(image, line, width) == GIF_ERROR) {
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
                msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()",gif_error_msg(image->Error));
#else
                msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()",gif_error_msg());
#endif
                return MS_FAILURE;
              }
              for(j=0; j<width; j++) {
                GifPixelType pix = line[j];
                if(transIdx == -1 || pix != transIdx) {
                  *a = 255;
                  *r = cmap->Colors[pix].Red;
                  *g = cmap->Colors[pix].Green;
                  *b = cmap->Colors[pix].Blue;
                } else {
                  *a = *r = *g = *b = 0;
                }
                a+=rb->data.rgba.pixel_step;
                r+=rb->data.rgba.pixel_step;
                g+=rb->data.rgba.pixel_step;
                b+=rb->data.rgba.pixel_step;
              }
            }
          }
          free((char *) line);
          firstImageRead = MS_TRUE;
        } else {
          /* Skip all next images */
          if (DGifGetCode(image, &codeSize, &codeBlock) == GIF_ERROR) {
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
            msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg(image->Error));
#else
            msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg());
#endif
            return MS_FAILURE;
          }
          while (codeBlock != NULL) {
            if (DGifGetCodeNext(image, &codeBlock) == GIF_ERROR) {
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
              msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg(image->Error));
#else
              msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg());
#endif
              return MS_FAILURE;
            }
          }
        }
        break;
      case EXTENSION_RECORD_TYPE:
        /* skip all extension blocks */
        if (DGifGetExtension(image, &extCode, &extension) == GIF_ERROR) {
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
          msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg(image->Error));
#else
          msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg());
#endif
          return MS_FAILURE;
        }
        if(extCode == 249 && (extension[1] & 1))
          transIdx = extension[4];
        for (;;) {
          if (DGifGetExtensionNext(image, &extension) == GIF_ERROR) {
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
            msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg(image->Error));
#else
            msSetError(MS_MISCERR,"corrupted gif image?: %s","readGIF()", gif_error_msg());
#endif
            return MS_FAILURE;
          }
          if (extension == NULL)
            break;
          if(extension[1] & 1)
            transIdx = extension[4];
        }
        break;
      case UNDEFINED_RECORD_TYPE:
        break;
      case TERMINATE_RECORD_TYPE:
        break;
      default:
        break;
    }

  } while (recordType != TERMINATE_RECORD_TYPE);

  if (DGifCloseFile(image) == GIF_ERROR) {
#if defined GIFLIB_MAJOR && GIFLIB_MAJOR >= 5
    msSetError(MS_MISCERR,"failed to close gif after loading: %s","readGIF()", gif_error_msg(image->Error));
#else
    msSetError(MS_MISCERR,"failed to close gif after loading: %s","readGIF()", gif_error_msg());
#endif
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}
#endif

int msLoadMSRasterBufferFromFile(char *path, rasterBufferObj *rb)
{
  FILE *stream;
  unsigned char signature[8];
  int ret = MS_FAILURE;
  stream = fopen(path,"rb");
  if(!stream) {
    msSetError(MS_MISCERR, "unable to open file %s for reading", "msLoadMSRasterBufferFromFile()", path);
    return MS_FAILURE;
  }
  if(1 != fread(signature,8,1,stream)) {
    msSetError(MS_MISCERR, "Unable to read header from image file %s", "msLoadMSRasterBufferFromFile()",path);
    return MS_FAILURE;
  }
  fclose(stream);
  if(png_sig_cmp(signature,0,8) == 0) {
    ret = readPNG(path,rb);
  } else if (!strncmp((char*)signature,"GIF",3)) {
#ifdef USE_GIF
    ret = readGIF(path,rb);
#else
    msSetError(MS_MISCERR,"pixmap %s seems to be GIF formatted, but this server is compiled without GIF support","readImage()",path);
    return MS_FAILURE;
#endif
  } else {
    msSetError(MS_MISCERR,"unsupported pixmap format","readImage()");
    return MS_FAILURE;
  }
  return ret;
}

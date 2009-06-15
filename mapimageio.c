/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Low level PNG/JPEG image io functions independent of GD.
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

#include "png.h"
#include "setjmp.h"
#include "mapserver.h"

#include "jpeglib.h"

MS_CVSID("$Id$")

typedef struct _streamInfo {
    FILE *fp;
    bufferObj *buffer;
} streamInfo;

void png_write_data_to_stream(png_structp png_ptr, png_bytep data, png_size_t length) {
    FILE *fp = ((streamInfo*)png_get_io_ptr(png_ptr))->fp;
    fwrite(data,length,1,fp);
}

void png_write_data_to_buffer(png_structp png_ptr, png_bytep data, png_size_t length) {
    bufferObj *buffer = ((streamInfo*)png_get_io_ptr(png_ptr))->buffer;
    msBufferAppend(buffer,data,length);
}

void png_flush_data(png_structp png_ptr) {
 // do nothing
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
jpeg_init_destination (j_compress_ptr cinfo) {
    ms_destination_mgr *dest = (ms_destination_mgr*) cinfo->dest;

    /* Allocate the output buffer --- it will be released when done with image */
    dest->data = (unsigned char *)
        (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                OUTPUT_BUF_SIZE * sizeof (unsigned char));

    dest->pub.next_output_byte = dest->data;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

void jpeg_stream_term_destination (j_compress_ptr cinfo) {
    ms_stream_destination_mgr *dest = (ms_stream_destination_mgr*) cinfo->dest;
    fwrite(dest->mgr.data, OUTPUT_BUF_SIZE-dest->mgr.pub.free_in_buffer, 1, dest->stream);
    dest->mgr.pub.next_output_byte = dest->mgr.data;
    dest->mgr.pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

void jpeg_buffer_term_destination (j_compress_ptr cinfo) {
    ms_buffer_destination_mgr *dest = (ms_buffer_destination_mgr*) cinfo->dest;
    msBufferAppend(dest->buffer, dest->mgr.data, OUTPUT_BUF_SIZE-dest->mgr.pub.free_in_buffer);
    dest->mgr.pub.next_output_byte = dest->mgr.data;
    dest->mgr.pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

int jpeg_stream_empty_output_buffer (j_compress_ptr cinfo) {
    ms_stream_destination_mgr *dest = (ms_stream_destination_mgr*) cinfo->dest;
    fwrite(dest->mgr.data, OUTPUT_BUF_SIZE, 1, dest->stream);
    dest->mgr.pub.next_output_byte = dest->mgr.data;
    dest->mgr.pub.free_in_buffer = OUTPUT_BUF_SIZE;
    return TRUE;
}

int jpeg_buffer_empty_output_buffer (j_compress_ptr cinfo) {
    ms_buffer_destination_mgr *dest = (ms_buffer_destination_mgr*) cinfo->dest;
    msBufferAppend(dest->buffer, dest->mgr.data, OUTPUT_BUF_SIZE);
    dest->mgr.pub.next_output_byte = dest->mgr.data;
    dest->mgr.pub.free_in_buffer = OUTPUT_BUF_SIZE;
    return TRUE;
}


int saveAsJPEG(rasterBufferObj *data, streamInfo *info,int quality)  {
  	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
    ms_destination_mgr *dest;
    JSAMPLE *rowdata; 
    unsigned int row;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    if (cinfo.dest == NULL)
    {
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

    cinfo.image_width = data->width;
    cinfo.image_height = data->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    rowdata = (JSAMPLE*)malloc(data->width*cinfo.input_components*sizeof(JSAMPLE));
    for(row=0;row<data->height;row++) {
        JSAMPLE *pixptr = rowdata;
        int col;
        unsigned char *r,*g,*b;
        r=data->r+row*data->row_step;
        g=data->g+row*data->row_step;
        b=data->b+row*data->row_step;
        for(col=0;col<data->width;col++) {
            *(pixptr++) = *r;
            *(pixptr++) = *g;
            *(pixptr++) = *b;
            r+=data->pixel_step;
            g+=data->pixel_step;
            b+=data->pixel_step;
        }
        (void) jpeg_write_scanlines(&cinfo, &rowdata, 1);
    }

    /* Step 6: Finish compression */

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    free(rowdata);
    return MS_SUCCESS;
}

int saveAsPNG(rasterBufferObj *data, streamInfo *info) {
    png_infop info_ptr; 
    int color_type;
    int row;
    unsigned int *rowdata;
    png_structp png_ptr = png_create_write_struct(
            PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);
    if (!png_ptr)
        return (MS_FAILURE);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_write_struct(&png_ptr,
                (png_infopp)NULL);
        return (MS_FAILURE);
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return (MS_FAILURE);
    }
    if(info->fp) 
        png_set_write_fn(png_ptr,info, png_write_data_to_stream, png_flush_data);
    else
        png_set_write_fn(png_ptr,info, png_write_data_to_buffer, png_flush_data);

    if(data->a)
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    else
        color_type = PNG_COLOR_TYPE_RGB;

    png_set_IHDR(png_ptr, info_ptr, data->width, data->height,
            8, color_type, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    if(!data->a && data->pixel_step==4)
       png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

    rowdata = (unsigned int*)malloc(data->width*sizeof(unsigned int));
    for(row=0;row<data->height;row++) {
        unsigned int *pixptr = rowdata;
        int col;
        unsigned char *a,*r,*g,*b;
        r=data->r+row*data->row_step;
        g=data->g+row*data->row_step;
        b=data->b+row*data->row_step;
        if(data->a) {
            a=data->a+row*data->row_step;
            for(col=0;col<data->width;col++) {
                if(*a) {
                    unsigned char *pix = (unsigned char*)pixptr;
                    pix[0] = ((int)(*r) * 255) / *a;
                    pix[1] = ((int)(*g) * 255) / *a;
                    pix[2] = ((int)(*b) * 255) / *a;
                    pix[3] = *a;
                } else {
                    *pixptr=0;
                }
                pixptr++;
                a+=data->pixel_step;
                r+=data->pixel_step;
                g+=data->pixel_step;
                b+=data->pixel_step;
            }
        } else {
            for(col=0;col<data->width;col++) {
                unsigned char *pix = (unsigned char*)pixptr;
                pix[0] = *r;
                pix[1] = *g;
                pix[2] = *b;
                pixptr++;
                r+=data->pixel_step;
                g+=data->pixel_step;
                b+=data->pixel_step;
            }
        }

        png_write_row(png_ptr,(png_bytep)rowdata);

    }
    png_write_end(png_ptr, info_ptr);
    free(rowdata);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return MS_SUCCESS;
}

int msSaveRasterBuffer(rasterBufferObj *data, FILE *stream,
        outputFormatObj *format) {
    if(msCaseFindSubstring(format->driver,"/png")) {
        streamInfo info;
        info.fp = stream;
        info.buffer = NULL;
        return saveAsPNG(data,&info);
    } else if(msCaseFindSubstring(format->driver,"/jpeg")) {
        streamInfo info;
        info.fp = stream;
        info.buffer=NULL;
        return saveAsJPEG(data,&info,atoi(msGetOutputFormatOption( format, "QUALITY", "75")));
    } else {
        msSetError(MS_MISCERR,"unsupported image format\n", "msSaveRasterBuffer()");
        return MS_FAILURE;
    }
}

int msSaveRasterBufferToBuffer(rasterBufferObj *data, bufferObj *buffer,
        outputFormatObj *format) {
    if(msCaseFindSubstring(format->driver,"/png")) {
        streamInfo info;
        info.fp = NULL;
        info.buffer = buffer;
        return saveAsPNG(data,&info);
    } else if(msCaseFindSubstring(format->driver,"/jpeg")) {
        streamInfo info;
        info.fp = NULL;
        info.buffer=buffer;
        return saveAsJPEG(data,&info,atoi(msGetOutputFormatOption( format, "QUALITY", "75")));
    } else {
        msSetError(MS_MISCERR,"unsupported image format\n", "msSaveRasterBuffer()");
        return MS_FAILURE;
    }
}

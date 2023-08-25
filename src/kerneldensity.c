
/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  KernelDensity layer implementation and related functions.
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 2014 Regents of the University of Minnesota.
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
#include <float.h>

#include "gdal.h"

static
void gaussian_blur(float *values, int width, int height, int radius) {
  float *tmp = (float*)msSmallMalloc(width*height*sizeof(float));
  int length = radius*2+1;
  float *kernel = (float*)msSmallMalloc(length*sizeof(float));
  float sigma=radius/3.0;
	float a=1.0/ sqrt(2.0*M_PI*sigma*sigma);
	float den=2.0*sigma*sigma;
	int i,x,y;

	for (i=0; i<length; i++) {
	  float x=i - radius;
	  float v=a * exp(-(x*x) / den);
	  kernel[i]=v;
	}
	memset(tmp,0,width*height*sizeof(float));

	for(y=0; y<height; y++) {
		float* src_row=values + width*y;
		float* dst_row=tmp + width*y;

		for(x=radius; x<width-radius; x++) {
			float accum=0;
			for(i=0; i<length; i++) {
				accum+=src_row[x+i-radius] * kernel[i];
			}
			dst_row[x]=accum;
		}
	}

	for(x=0; x<width; x++) {
		float* src_col=tmp+x;
		float* dst_col=values+x;

		for(y=radius; y<height-radius; y++) {
			float accum=0;
			for (i=0; i<length; i++) {
				accum+=src_col[width*(y+i-radius)] * kernel[i];
			}
			dst_col[y*width]=accum;
		}
	}
  free(tmp);
  free(kernel);
}

void msKernelDensityProcessing(layerObj *layer, interpolationProcessingParams *interpParams) {
    const char *interpParamsProcessing = msLayerGetProcessingKey( layer, "KERNELDENSITY_RADIUS" );
    if(interpParamsProcessing) {
        interpParams->radius = atoi(interpParamsProcessing);
    } else {
        interpParams->radius = 10;
    }

    interpParamsProcessing = msLayerGetProcessingKey( layer, "KERNELDENSITY_COMPUTE_BORDERS" );
    if(interpParamsProcessing && strcasecmp(interpParamsProcessing,"OFF")) {
        interpParams->expand_searchrect = 1;
    } else {
        interpParams->expand_searchrect = 0;
    }

    interpParamsProcessing = msLayerGetProcessingKey( layer, "KERNELDENSITY_NORMALIZATION" );
    if(!interpParamsProcessing || !strcasecmp(interpParamsProcessing,"AUTO")) {
        interpParams->normalization_scale = 0.0;
    } else {
        interpParams->normalization_scale = atof(interpParamsProcessing);
        if(interpParams->normalization_scale != 0) {
            interpParams->normalization_scale = 1.0 / interpParams->normalization_scale;
        } else {
            interpParams->normalization_scale = 1.0;
        }
    }
}

void msKernelDensity(imageObj *image, float *values, int width, int height, int npoints,
                   interpolationProcessingParams *interpParams, unsigned char *iValues) {
    int i,j;
    float valmax=FLT_MIN, valmin=FLT_MAX;
    int radius = interpParams->radius;
    float normalization_scale = interpParams->normalization_scale;
    int expand_searchrect = interpParams->expand_searchrect;

    gaussian_blur(values, width, height, radius);

    if(normalization_scale == 0.0) {   /* auto normalization */
        for (j = radius; j < height-radius; j++) {
            for (i = radius; i < width-radius; i++) {
                float val = values[j*width + i];
                if(val >0 && val>valmax) {
                    valmax = val;
                }
                if(val>0 && val<valmin) {
                    valmin = val;
                }
            }
        }
    } else {
        valmin = 0;
        valmax = normalization_scale;
    }

    if(expand_searchrect) {
        for (j=0; j < image->height; j++) {
            for (i=0; i < image->width; i++) {
                float norm = (values[(j+radius)*width + i + radius] - valmin) / valmax;
                int v = 255 * norm;
                if (v<0) v=0;
                else if (v>255) v = 255;
                iValues[j*image->width + i] = v;
            }
        }
    } else {
        if(npoints > 0) {
            for (j=radius; j < image->height-radius; j++) {
                for (i=radius; i < image->width-radius; i++) {
                    float norm=(values[j*width + i] - valmin) / valmax;
                    int v=255 * norm;
                    if (v<0) v=0;
                    else if (v>255) v=255;
                    iValues[j*image->width + i]=v;
                }
            }
        }
    }
}

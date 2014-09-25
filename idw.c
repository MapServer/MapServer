/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  IDW layer interpolation.
 * Author:   Hermes L. Herrera Martinez and the MapServer team.
 * Thanks:   Thomas Bonfort
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
#define EPSILON 0.000000001
#include <time.h>

void idw(float *xyz, int width, int height, int npoints, processingParams *psz, unsigned char *iValues) {
    int i,j,index;
    int radius = psz->radius;
    float power = psz->power;
    for (j=0; j < height; j++) {
        for (i=0; i < width; i++) {
            double den = EPSILON, num = 0;
            for(index = 0; index < npoints*3; index += 3){
                double d = (xyz[index] - i)*(xyz[index] - i)+(xyz[index+1] - j)*(xyz[index+1] - j);
                if(radius*radius > d) {
                    double w = (double)1.0/(pow(d, power*0.5) + EPSILON);
                    num += w*xyz[index+2];
                    den += w;
                }
            }
            iValues[j*width + i] = num/den;
        }
    }
}

void idwProcessing(layerObj *layer, processingParams *psz) {
    const char *pszProcessing = msLayerGetProcessingKey( layer, "IDW_POWER" );
    if(pszProcessing) {
        psz->power = atof(pszProcessing);
        if(psz->power > 12){
            psz->power = 12;
        }
    } else{
        psz->power = 2.0;
    }

    pszProcessing = msLayerGetProcessingKey( layer, "IDW_RADIUS" );
    if(pszProcessing){
        psz->radius = atof(pszProcessing);
    } else {
        psz->radius = MAX(layer->map->width,layer->map->height);
    }

    pszProcessing = msLayerGetProcessingKey( layer, "IDW_COMPUTE_BORDERS" );
    if(pszProcessing && strcasecmp(pszProcessing,"OFF")){
        psz->expand_searchrect = 1;
    } else {
        psz->expand_searchrect = 0;
    }
}

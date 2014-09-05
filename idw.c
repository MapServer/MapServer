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
#ifdef USE_GDAL

#include "gdal.h"

#include <time.h>

void idw(float *xyz, float power, int idwRadius, int width, int height, int numFeatures, unsigned char *iValues) {
    int i,j,index;
    int * x = (int*) msSmallCalloc(numFeatures, sizeof(int));
    int * y = (int*) msSmallCalloc(numFeatures, sizeof(int));
    float*z = (float*) msSmallCalloc(numFeatures, sizeof(float));

    for(i=0, j=0; i < numFeatures*3; i += 3) {
        x[j] = (int)xyz[i];
        y[j] = (int)xyz[i+1];
        z[j++] = xyz[i+2];
    }
    for (j=0; j < height; j++) {
        for (i=0; i < width; i++) {
            double den = EPSILON, num = 0, value;
            for(index = 0; index < numFeatures; index++){
                double d = (x[index] - i)*(x[index] - i)+(y[index] - j)*(y[index] - j);
                if(idwRadius*idwRadius > d) {
                    double w = (double)1.0/(pow(d, power*0.5) + EPSILON);
                    num += w*z[index];
                    den += w;
                }
            }
            value = (double)num/(den);
            iValues[j*width + i] = value;
        }
    }
    free(x); free(y); free(z);
}


int msComputeIDWDataset(mapObj *map, imageObj *image, layerObj *idw_layer, void **hDSvoid, void **cleanup_ptr) {
    int status,layer_idx, i, nclasses=0, have_sample=0;
    rectObj searchrect;
    shapeObj shape;
    layerObj *layer;
    float * values;
    int radius = 10, im_width = image->width, im_height = image->height;
    int count,expand_searchrect=1;
    double power, idwRadius = im_width, invcellsize = 1.0 / map->cellsize, georadius=0;
    unsigned char *iValues;
    GDALDatasetH hDS;
    const char *pszProcessing;
    int *classgroup = NULL;

    assert(idw_layer->connectiontype == MS_IDW);
    *cleanup_ptr = NULL;

    if(!idw_layer->connection || !*idw_layer->connection) {
        msSetError(MS_MISCERR, "msComputeIDWDataset()", "IDW layer has no CONNECTION defined");
        return MS_FAILURE;
    }

    pszProcessing = msLayerGetProcessingKey( idw_layer, "IDW_POWER" );
    if(pszProcessing) {
        power = atof(pszProcessing);
        if(power > 12){
            power=12;
        }
    } else
        power = 2.0;

    pszProcessing = msLayerGetProcessingKey( idw_layer, "IDW_RADIUS" );
    if(pszProcessing)
        idwRadius = atof(pszProcessing);

    pszProcessing = msLayerGetProcessingKey( idw_layer, "IDW_COMPUTE_BORDERS" );
    if(pszProcessing && strcasecmp(pszProcessing,"OFF"))
        expand_searchrect = 1;
    else
        expand_searchrect = 0;

    layer_idx = msGetLayerIndex(map,idw_layer->connection);
    if(layer_idx == -1) {
        int nLayers, *aLayers;
        aLayers = msGetLayersIndexByGroup(map, idw_layer->connection, &nLayers);
        if(!aLayers || !nLayers) {
            msSetError(MS_MISCERR, "IDW layer (%s) references unknown layer (%s)", "msComputeIDWDataset()",
                       idw_layer->name,idw_layer->connection);
            return (MS_FAILURE);
        }
        for(i=0; i<nLayers; i++) {
            layer_idx = aLayers[i];
            layer = GET_LAYER(map, layer_idx);
            if(msScaleInBounds(map->scaledenom, layer->minscaledenom, layer->maxscaledenom))
                break;
        }
        free(aLayers);
        if(i == nLayers) {
            msSetError(MS_MISCERR, "IDW layer (%s) references no layer for current scale", "msComputeKernelDensityDataset()",
                       idw_layer->name);
            return (MS_FAILURE);
        }
    } else {
        layer = GET_LAYER(map, layer_idx);
    }
    /* open the linked layer */
    status = msLayerOpen(layer);
    if(status != MS_SUCCESS) return MS_FAILURE;

    status = msLayerWhichItems(layer, MS_FALSE, NULL);
    if(status != MS_SUCCESS) {
        msLayerClose(layer);
        return MS_FAILURE;
    }

    /* identify target shapes */
    if(layer->transform == MS_TRUE) {
        searchrect = map->extent;
        if(expand_searchrect) {
            georadius = radius * map->cellsize;
            searchrect.minx -= georadius;
            searchrect.miny -= georadius;
            searchrect.maxx += georadius;
            searchrect.maxy += georadius;
            im_width += 2 * radius;
            im_height += 2 * radius;
        }
    }
    else {
        searchrect.minx = searchrect.miny = 0;
        searchrect.maxx = map->width-1;
        searchrect.maxy = map->height-1;
    }

#ifdef USE_PROJ
    layer->project = msProjectionsDiffer(&(layer->projection), &(map->projection));
    if(layer->project)
        msProjectRect(&map->projection, &layer->projection, &searchrect); /* project the searchrect to source coords */
#endif

    status = msLayerWhichShapes(layer, searchrect, MS_FALSE);

    if(status == MS_DONE) {
        msLayerClose(layer);
        return MS_SUCCESS;
    } else if(status != MS_SUCCESS) {
        msLayerClose(layer);
        return MS_FAILURE;
    }

    values = (float*) msSmallCalloc(3*im_width * im_height, sizeof(float));
    if(layer->classgroup && layer->numclasses > 0)
        classgroup = msAllocateValidClassGroups(layer, &nclasses);

    msInitShape(&shape);
    count=0;
    while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
        int l,p,s,c;
        classObj *_class;
        double weight = 1.0;
#ifdef USE_PROJ
        if(layer->project)
            msProjectShape(&layer->projection, &map->projection, &shape);
#endif
        if(layer->numclasses > 0) {
            c = msShapeGetClass(layer, map, &shape, classgroup, nclasses);
            if(c == -1) {
                goto nextshape; /* no class matched, skip */
            }
            _class = layer->class[c];

            if(_class->status == MS_OFF){
                goto nextshape; /* no visible, skip */
            }

            for (s = 0; s < _class->numstyles; s++) {
                if (msScaleInBounds(map->scaledenom,
                                    _class->styles[s]->minscaledenom,
                                    _class->styles[s]->maxscaledenom)) {
                    if(_class->styles[s]->bindings[MS_STYLE_BINDING_SIZE].index != -1) {
                        weight = atof(shape.values[_class->styles[s]->bindings[MS_STYLE_BINDING_SIZE].index]);
                    } else {
                        weight = _class->styles[s]->size;
                    }
                    break;
                }
            }
            if(s == _class->numstyles) {
                /* no style in scale bounds */
                goto nextshape;
            }
        }

        for(l=0; l<shape.numlines; l++) {
            for(p=0; p<shape.line[l].numpoints; p++) {
                int x = MS_MAP2IMAGE_XCELL_IC(shape.line[l].point[p].x, map->extent.minx - georadius, invcellsize);
                int y = MS_MAP2IMAGE_YCELL_IC(shape.line[l].point[p].y, map->extent.maxy + georadius, invcellsize);
                if(x>=0 && y>=0 && x<im_width && y<im_height) {
                    have_sample = 1;
                    values[count++]=x;
                    values[count++]=y;
                    values[count++]=weight;
                }
            }
        }

nextshape:
        msFreeShape(&shape);
    }
    msLayerClose(layer);

    if(status == MS_DONE) {
        status = MS_SUCCESS;
    } else {
        status = MS_FAILURE;
    }
    /*of = fopen("results.txt","w");
    fprintf(of, "---------\n\t%d\n", 21);
    fclose(of);*/
    if(have_sample && expand_searchrect) {
        iValues = msSmallMalloc(image->width*image->height*sizeof(unsigned char));
    } else {
        iValues = msSmallCalloc(1,image->width*image->height*sizeof(unsigned char));
    }
    //idw interpolation
    if(have_sample){
        idw(values, power, idwRadius, image->width, image->height, count/3, iValues);
    }
    free(values);
    {
        char ds_string [1024];
        double adfGeoTransform[6];
        snprintf(ds_string,1024,"MEM:::DATAPOINTER=%p,PIXELS=%u,LINES=%u,BANDS=1,DATATYPE=Byte,PIXELOFFSET=1,LINEOFFSET=%u",
                 iValues,image->width,image->height,image->width);
        hDS = GDALOpenShared( ds_string, GA_ReadOnly );
        if(hDS==NULL) {
            msSetError(MS_MISCERR,"msComputeIDWDataset()","failed to create in-memory gdal dataset for interpolated data");
            status = MS_FAILURE;
            free(iValues);
        }
        adfGeoTransform[0] = map->extent.minx - map->cellsize * 0.5; /* top left x */
        adfGeoTransform[1] = map->cellsize;/* w-e pixel resolution */
        adfGeoTransform[2] = 0; /* 0 */
        adfGeoTransform[3] = map->extent.maxy + map->cellsize * 0.5;/* top left y */
        adfGeoTransform[4] =0; /* 0 */
        adfGeoTransform[5] = -map->cellsize;/* n-s pixel resolution (negative value) */
        GDALSetGeoTransform(hDS,adfGeoTransform);
        *hDSvoid = hDS;
        *cleanup_ptr = (void*)iValues;
    }
    return status;
}
#else


int msComputeIDWDataset(mapObj *map, imageObj *image, layerObj *layer, void **hDSvoid, void **cleanup_ptr) {
    msSetError(MS_MISCERR,"msComputeIDWDataset()", "IDW layers require GDAL support, however GDAL support is not compiled in this build");
    return MS_FAILURE;
}

#endif

int msCleanupIDWDataset(mapObj *map, imageObj *image, layerObj *layer, void *cleanup_ptr) {
    if(cleanup_ptr!=NULL){
        free(cleanup_ptr);
    }
    return MS_SUCCESS;
}

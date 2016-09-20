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
#ifdef USE_GDAL

#include "gdal.h"
#include "cpl_string.h"

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


int msComputeKernelDensityDataset(mapObj *map, imageObj *image, layerObj *kerneldensity_layer, void **hDSvoid, void **cleanup_ptr) {

  int status,layer_idx, i,j, nclasses=0, have_sample=0;
  rectObj searchrect;
  shapeObj shape;
  layerObj *layer;
  float *values = NULL;
  int radius = 10, im_width = image->width, im_height = image->height;
  int expand_searchrect=1;
  float normalization_scale=0.0;
  double invcellsize = 1.0 / map->cellsize, georadius=0;
  float valmax=FLT_MIN, valmin=FLT_MAX;
  unsigned char *iValues;
  GDALDatasetH hDS;
  const char *pszProcessing;
  int *classgroup = NULL;
  
  assert(kerneldensity_layer->connectiontype == MS_KERNELDENSITY);
  *cleanup_ptr = NULL;

  if(!kerneldensity_layer->connection || !*kerneldensity_layer->connection) {
    msSetError(MS_MISCERR, "msComputeKernelDensityDataset()", "KernelDensity layer has no CONNECTION defined");
    return MS_FAILURE;
  }
  pszProcessing = msLayerGetProcessingKey( kerneldensity_layer, "KERNELDENSITY_RADIUS" );
  if(pszProcessing)
    radius = atoi(pszProcessing);
  else
    radius = 10;

  pszProcessing = msLayerGetProcessingKey( kerneldensity_layer, "KERNELDENSITY_COMPUTE_BORDERS" );
  if(pszProcessing && strcasecmp(pszProcessing,"OFF"))
    expand_searchrect = 1;
  else
    expand_searchrect = 0;

  pszProcessing = msLayerGetProcessingKey( kerneldensity_layer, "KERNELDENSITY_NORMALIZATION" );
  if(!pszProcessing || !strcasecmp(pszProcessing,"AUTO"))
    normalization_scale = 0.0;
  else {
    normalization_scale = atof(pszProcessing);
    if(normalization_scale != 0) {
      normalization_scale = 1.0 / normalization_scale;
    } else {
      normalization_scale = 1.0;
    }
  }

  layer_idx = msGetLayerIndex(map,kerneldensity_layer->connection);
  if(layer_idx == -1) {
    int nLayers, *aLayers;
    aLayers = msGetLayersIndexByGroup(map, kerneldensity_layer->connection, &nLayers);
    if(!aLayers || !nLayers) {
      msSetError(MS_MISCERR, "KernelDensity layer (%s) references unknown layer (%s)", "msComputeKernelDensityDataset()",
                 kerneldensity_layer->name,kerneldensity_layer->connection);
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
      msSetError(MS_MISCERR, "KernelDensity layer (%s) references no layer for current scale", "msComputeKernelDensityDataset()",
                 kerneldensity_layer->name);
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
    /* nothing to do */
  if(status == MS_SUCCESS) { /* at least one sample may have overlapped */
    
    if(layer->classgroup && layer->numclasses > 0)
      classgroup = msAllocateValidClassGroups(layer, &nclasses);
    
    msInitShape(&shape);
    while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
      int l,p,s,c;
      double weight = 1.0;
      if(!values) /* defer allocation until we effectively have a feature */
        values = (float*) msSmallCalloc(im_width * im_height, sizeof(float));
#ifdef USE_PROJ
      if(layer->project)
        msProjectShape(&layer->projection, &map->projection, &shape);
#endif
      
      /* the weight for the sample is set to 1.0 by default. If the
       * layer has some classes defined, we will read the weight from
       * the class->style->size (which can be binded to an attribute)
       */
      if(layer->numclasses > 0) {
        c = msShapeGetClass(layer, map, &shape, classgroup, nclasses);
        if((c == -1) || (layer->class[c]->status == MS_OFF)) {
          goto nextshape; /* no class matched, skip */
        }
        for (s = 0; s < layer->class[c]->numstyles; s++) {
          if (msScaleInBounds(map->scaledenom,
                  layer->class[c]->styles[s]->minscaledenom,
                  layer->class[c]->styles[s]->maxscaledenom)) {
            if(layer->class[c]->styles[s]->bindings[MS_STYLE_BINDING_SIZE].index != -1) {
              weight = atof(shape.values[layer->class[c]->styles[s]->bindings[MS_STYLE_BINDING_SIZE].index]);
            } else {
              weight = layer->class[c]->styles[s]->size;
            }
            break;
          }
        }
        if(s == layer->class[c]->numstyles) {
          /* no style in scale bounds */
          goto nextshape;
        }
      }
      for(l=0; l<shape.numlines; l++) {
        for(p=0; p<shape.line[l].numpoints; p++) {
          int x = MS_MAP2IMAGE_XCELL_IC(shape.line[l].point[p].x, map->extent.minx - georadius, invcellsize);
          int y = MS_MAP2IMAGE_YCELL_IC(shape.line[l].point[p].y, map->extent.maxy + georadius, invcellsize);
          if(x>=0 && y>=0 && x<im_width && y<im_height) {
            float *value = values + y * im_width + x;
            (*value) += weight;
            have_sample = 1;
          }
        }
      }
      
      nextshape:
      msFreeShape(&shape);
    }
  } else if(status != MS_DONE) {
    msLayerClose(layer);
    return MS_FAILURE;
  }
  
  /* status == MS_DONE */
  msLayerClose(layer);
  status = MS_SUCCESS;


  if(have_sample) { /* no use applying the filtering kernel if we have no samples */
    gaussian_blur(values,im_width, im_height, radius);

    if(normalization_scale == 0.0) {   /* auto normalization */
      for (j=radius; j<im_height-radius; j++) {
        for (i=radius; i<im_width-radius; i++) {
          float val = values[j*im_width + i];
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
  }


  if(have_sample && expand_searchrect) {
    iValues = msSmallMalloc(image->width*image->height*sizeof(unsigned char));
    for (j=0; j<image->height; j++) {
      for (i=0; i<image->width; i++) {
        float norm=(values[(j+radius)*im_width + i + radius] - valmin) / valmax;
        int v=255 * norm;
        if (v<0) v=0;
        else if (v>255) v=255;
        iValues[j*image->width + i] = v;
      }
    }
  } else {
    iValues = msSmallCalloc(1,image->width*image->height*sizeof(unsigned char));
    if(have_sample) {
       for (j=radius; j<image->height-radius; j++) {
         for (i=radius; i<image->width-radius; i++) {
           float norm=(values[j*im_width + i] - valmin) / valmax;
           int v=255 * norm;
           if (v<0) v=0;
           else if (v>255) v=255;
           iValues[j*image->width + i]=v;
         }
       }
    }
  }

  free(values);
  
  {
    char pointer[64];
    char ds_string [1024];
    double adfGeoTransform[6];
    memset(pointer, 0, sizeof(pointer));
    CPLPrintPointer(pointer, iValues, sizeof(pointer));
    snprintf(ds_string,1024,"MEM:::DATAPOINTER=%s,PIXELS=%u,LINES=%u,BANDS=1,DATATYPE=Byte,PIXELOFFSET=1,LINEOFFSET=%u",
        pointer,image->width,image->height,image->width);
    hDS = GDALOpenShared( ds_string, GA_ReadOnly );
    if(hDS==NULL) {
      msSetError(MS_MISCERR,"msComputeKernelDensityDataset()","failed to create in-memory gdal dataset for interpolated data");
      status = MS_FAILURE;
      free(iValues);
      return status;
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


int msComputeKernelDensityDataset(mapObj *map, imageObj *image, layerObj *layer, void **hDSvoid, void **cleanup_ptr) {
    msSetError(MS_MISCERR,"msComputeKernelDensityDataset()", "KernelDensity layers require GDAL support, however GDAL support is not compiled in this build");
    return MS_FAILURE;
}

#endif

int msCleanupKernelDensityDataset(mapObj *map, imageObj *image, layerObj *layer, void *cleanup_ptr) {
  free(cleanup_ptr);
  return MS_SUCCESS;
}

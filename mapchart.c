/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Implementation of dynamic charting (MS-RFC-29)
 * Author:   Thomas Bonfort
 *
 ******************************************************************************
 * Copyright (c) 1996-2007 Regents of the University of Minnesota.
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
 ****************************************************************************/


#include "map.h"

MS_CVSID("$Id: $")

int msDrawBarChartGD(mapObj *map, layerObj *layer, shapeObj *shape, imageObj *image,
                     int width, int height, float maxVal, float minVal, int barWidth)
{
	
    pointObj center;
    float *values,shapeMaxVal=0,shapeMinVal=0,pixperval;
    int c,vertOrigin,horizStart,color;
	
    msDrawStartShape(map, layer, image, shape);
#ifdef USE_PROJ
    if(layer->project && msProjectionsDiffer(&(layer->projection), &(map->projection)))
        msProjectShape(&layer->projection, &map->projection, shape);
    else
        layer->project = MS_FALSE;
#endif
    /*center.x=(shape->bounds.minx+shape->bounds.maxx)/2.;
      center.y=(shape->bounds.miny+shape->bounds.maxy)/2.;*/
    msPolygonLabelPoint(shape, &center, 10);
    if(layer->transform == MS_TRUE) {
        if(!msPointInRect(&center, &map->extent)) return MS_SUCCESS; /* next point */
        center.x = MS_MAP2IMAGE_X(center.x, map->extent.minx, map->cellsize);
        center.y = MS_MAP2IMAGE_Y(center.y, map->extent.maxy, map->cellsize);
    } else
        msOffsetPointRelativeTo(&center, layer);
	
    if(msBindLayerToShape(layer, shape) != MS_SUCCESS)
        return MS_FAILURE; /* error message is set in msBindLayerToShape() */

    values=(float*)calloc(layer->numclasses,sizeof(float));
    for(c=0;c<layer->numclasses;c++)
    {
        values[c]=(layer->class[c]->styles[0]->size);
        if(values[c]>shapeMaxVal)
            shapeMaxVal=values[c];
        if(values[c]<shapeMinVal)
            shapeMinVal=values[c];
    }
    if(!maxVal)
        maxVal=shapeMaxVal;
    if(!minVal)
        minVal=shapeMinVal;
    pixperval=(float)height/(maxVal-minVal);
    vertOrigin=center.y-minVal*pixperval+height/2;
    horizStart=center.x-width/2;
    for(c=0;c<layer->numclasses;c++)
    {
        int barHeight;
        color = gdImageColorAllocate(image->img.gd, layer->class[c]->styles[0]->color.red,
                                     layer->class[c]->styles[0]->color.green,
                                     layer->class[c]->styles[0]->color.blue);
        if(values[c]>0)
        {
            barHeight=MS_MIN(values[c],maxVal)*pixperval;
            gdImageFilledRectangle(image->img.gd, horizStart,vertOrigin-barHeight, horizStart+barWidth-1 , vertOrigin,color);
        }
        else if(values[c]<0)
        {
            barHeight=MS_MIN(values[c],minVal)*pixperval;
            gdImageFilledRectangle(image->img.gd, horizStart, vertOrigin, horizStart+barWidth-1 , vertOrigin+barHeight,color);
        }
        horizStart+=barWidth;
    }
    free(values);

    return MS_SUCCESS;
}

int msDrawPieChartGD(mapObj *map, layerObj *layer, shapeObj *shape, imageObj *image,
                     int width, int height)
{
    int i,c,color;
    pointObj center;
    float *values;
    float dTotal=0.,start=0;
															
    msDrawStartShape(map, layer, image, shape);
#ifdef USE_PROJ
    if(layer->project && msProjectionsDiffer(&(layer->projection), &(map->projection)))
        msProjectShape(&layer->projection, &map->projection, shape);
    else
        layer->project = MS_FALSE;
#endif
    /*center.x=(shape->bounds.minx+shape->bounds.maxx)/2.;
      center.y=(shape->bounds.miny+shape->bounds.maxy)/2.;*/
    msPolygonLabelPoint(shape, &center, 10);
    if(layer->transform == MS_TRUE) {
        if(!msPointInRect(&center, &map->extent)) return MS_SUCCESS; /* next point */
        center.x = MS_MAP2IMAGE_X(center.x, map->extent.minx, map->cellsize);
        center.y = MS_MAP2IMAGE_Y(center.y, map->extent.maxy, map->cellsize);
    } else
        msOffsetPointRelativeTo(&center, layer);
	
    if(msBindLayerToShape(layer, shape) != MS_SUCCESS)
        return MS_FAILURE; /* error message is set in msBindLayerToShape() */

    values=(float*)calloc(layer->numclasses,sizeof(float));
    for(c=0;c<layer->numclasses;c++)
    {
        values[c]=(layer->class[c]->styles[0]->size);
        dTotal+=values[c];
    }
															 
    for(i=0; i < layer->numclasses; i++)
    {
        values[i]*=360.0/dTotal;
        color = gdImageColorAllocate(image->img.gd, layer->class[i]->styles[0]->color.red,
                                     layer->class[i]->styles[0]->color.green,
                                     layer->class[i]->styles[0]->color.blue);
        if(1)
        {
            gdImageSetAntiAliased(image->img.gd, color);
            gdImageFilledArc(image->img.gd, center.x, center.y, width, height, (int)start, (int)(start+values[i]), gdAntiAliased, gdPie);
            gdImageSetAntiAliased(image->img.gd, -1);
        }
        else
        {
            gdImageFilledArc(image->img.gd, center.x, center.y, width, height, (int)start, (int)(start+values[i]), color, gdPie);
        }
        start+=values[i];
    }
    free(values);
    return MS_SUCCESS;
}

int msDrawPieChartLayerGD(mapObj *map, layerObj *layer, imageObj *image, 
                          int width, int height)
{
    shapeObj    shape;
    int         status;
    /* step through the target shapes */
    msInitShape(&shape);
    while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
        msDrawPieChartGD(map, layer, &shape, image,width,height);
        msFreeShape(&shape);
    }
    return MS_SUCCESS;
}

int msDrawBarChartLayerGD(mapObj *map, layerObj *layer, imageObj *image, 
                          int width, int height)
{
    shapeObj    shape;
    int         status;
    const char *barMax=msLayerGetProcessingKey( layer,"CHART_BAR_MAXVAL" );
    const char *barMin=msLayerGetProcessingKey( layer,"CHART_BAR_MINVAL" );
    int barWidth;
    float barMaxVal=0,barMinVal=0;
	
    if(barMax)
        barMaxVal=atof(barMax);
    if(barMin)
        barMinVal=atof(barMin);
    barWidth=width/layer->numclasses;
    if(!barWidth)
    {
        msSetError(MS_MISCERR, "Specified width of chart too small to fit given number of classes", "msDrawBarChartLayerGD()");
        return MS_FAILURE;
    }
    /* step through the target shapes */
    msInitShape(&shape);
    while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
        msDrawBarChartGD(map, layer, &shape, image,width,height,barMaxVal,barMinVal,barWidth);
        msFreeShape(&shape);
    }
    return MS_SUCCESS;
}

int msDrawChartLayerGD(mapObj *map, layerObj *layer, imageObj *image)
{
	
    char        annotate=MS_TRUE;
    rectObj     searchrect;
    const char *chartType=msLayerGetProcessingKey( layer,"CHART_TYPE" );
    const char *chartSize=msLayerGetProcessingKey( layer,"CHART_SIZE" );
    int width,height,status;
    if(chartType==NULL)
        chartType="PIE";
    if(chartSize==NULL)
    {
        width=height=20;
    }
    else
    {
        switch(sscanf(chartSize,"%d %d",&width,&height))
        {
          case 2: break;
          case 1: height=width;break;
          default:
            msSetError(MS_MISCERR, "msDrawChartGD format error for processing arg \"CHART_SIZE\"", "msDrawChartGD()");
            return MS_FAILURE;
        }
    }
	
    annotate = msEvalContext(map, layer, layer->labelrequires);
    if(map->scale > 0) {
        if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale)) annotate = MS_FALSE;
        if((layer->labelminscale != -1) && (map->scale < layer->labelminscale)) annotate = MS_FALSE;
    }
  
    /* open this layer */
    status = msLayerOpen(layer);
    if(status != MS_SUCCESS) return MS_FAILURE;
  
    status = msLayerWhichItems(layer, MS_TRUE, annotate, NULL);
    if(status != MS_SUCCESS) {
        msLayerClose(layer);
        return MS_FAILURE;
    }
    /* identify target shapes */
    if(layer->transform == MS_TRUE)
        searchrect = map->extent;
    else {
        searchrect.minx = searchrect.miny = 0;
        searchrect.maxx = map->width-1;
        searchrect.maxy = map->height-1;
    }
  
#ifdef USE_PROJ
    if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
        msProjectRect(&map->projection, &layer->projection, &searchrect); /* project the searchrect to source coords */
#endif
    
    status = msLayerWhichShapes(layer, searchrect);
    if(status == MS_DONE) { /* no overlap */
        msLayerClose(layer);
        return MS_SUCCESS;
    } else if(status != MS_SUCCESS) {
        msLayerClose(layer);
        return MS_FAILURE;
    }
    if( strcasecmp(chartType,"PIE") == 0 )
    {
        msDrawPieChartLayerGD(map, layer, image,width,height);
    }
    else if( strcasecmp(chartType,"BAR") == 0 )
    {
        msDrawBarChartLayerGD(map, layer, image,width,height);
    }
    else
    {
        msSetError(MS_MISCERR, "msDrawChartLayerGD unknown \"CHART_TYPE\" processing key", "msDrawChartLayerGD()");
        msLayerClose(layer);
        return MS_FAILURE;
    }
  
    msLayerClose(layer);  
    return MS_SUCCESS;
}

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of dynamic charting (MS-RFC-29)
 * Author:   Thomas Bonfort ( thomas.bonfort[at]gmail.com )
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
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/


#include "mapserver.h"

MS_CVSID("$Id$")

#define MS_CHART_TYPE_PIE 1
#define MS_CHART_TYPE_BAR 2
#define MS_CHART_TYPE_VBAR 3

/*
** check if an object of width w and height h placed at point x,y can fit in an image of width mw and height mh
*/
#define MS_CHART_FITS(x,y,w,h,mw,mh) (((x)-(w)/2.>0.)&&((x)+(w)/2.<(mw))&&((y)-(h)/2.>0.)&&((y)+(h)/2.)<(mh))

/*
** find a point on a shape. check if it fits in image
** returns 
**  MS_SUCCESS and point coordinates in 'p' if chart fits in image
**  MS_FAILURE if no point could be found
*/
int findChartPoint(mapObj *map, shapeObj *shape, int width, int height, pointObj *center) {
    int middle,numpoints,idx,offset;
    double invcellsize = 1.0/map->cellsize; /*speed up MAP2IMAGE_X/Y_IC_DBL*/
    switch(shape->type) {
        case MS_SHAPE_POINT:
            center->x=MS_MAP2IMAGE_X_IC_DBL(shape->line[0].point[0].x, map->extent.minx, invcellsize);
            center->y=MS_MAP2IMAGE_Y_IC_DBL(shape->line[0].point[0].y, map->extent.maxy, invcellsize);
            
            if(MS_CHART_FITS(center->x,center->y,width,height,map->width,map->height))
                return MS_SUCCESS;
            else
                return MS_FAILURE;
            break;
        case MS_SHAPE_LINE:
            /*loop through line segments starting from middle (alternate between before and after middle point)
            **first segment that fits is chosen
            */
            middle=shape->line[0].numpoints/2; /*start with middle segment of line*/
            numpoints=shape->line[0].numpoints;
            for(offset=1;offset<=middle;offset++) {
                idx=middle+offset;
                if(idx<numpoints) {
                    center->x=(shape->line[0].point[idx-1].x+shape->line[0].point[idx].x)/2.;
                    center->y=(shape->line[0].point[idx-1].y+shape->line[0].point[idx].y)/2.;
                    center->x=MS_MAP2IMAGE_X_IC_DBL(center->x, map->extent.minx, invcellsize);
                    center->y=MS_MAP2IMAGE_Y_IC_DBL(center->y, map->extent.maxy, invcellsize);

                    if(MS_CHART_FITS(center->x,center->y,width,height,map->width,map->height))
                        return MS_SUCCESS;
                    
                    break;
                }
                idx=middle-offset;
                if(idx>=0) {
                    center->x=(shape->line[0].point[idx].x+shape->line[0].point[idx+1].x)/2;
                    center->y=(shape->line[0].point[idx].y+shape->line[0].point[idx+1].y)/2;
                    center->x=MS_MAP2IMAGE_X_IC_DBL(center->x, map->extent.minx, invcellsize);
                    center->y=MS_MAP2IMAGE_Y_IC_DBL(center->y, map->extent.maxy, invcellsize);
                    
                    if(MS_CHART_FITS(center->x,center->y,width,height,map->width,map->height))
                        return MS_SUCCESS;
                    break;
                }
            }
            return MS_FAILURE;
        break;
        case MS_SHAPE_POLYGON:
            msPolygonLabelPoint(shape, center, -1);
            center->x=MS_MAP2IMAGE_X_IC_DBL(center->x, map->extent.minx, invcellsize);
            center->y=MS_MAP2IMAGE_Y_IC_DBL(center->y, map->extent.maxy, invcellsize);
            
            if(MS_CHART_FITS(center->x,center->y,width,height,map->width,map->height))
                return MS_SUCCESS;
            else
                return MS_FAILURE;
            break;
        default:
            return MS_FAILURE;
    }
}

void drawRectangle(mapObj *map, imageObj *image, float mx, float my, float Mx, float My,
        styleObj *style) {
	shapeObj shape;
	lineObj line;
	pointObj point[5];
	line.point = point;
	line.numpoints = 5;
	shape.line = &line;
	shape.numlines = 1;
	
	point[0].x = point[4].x = point[3].x = mx;
	point[0].y = point[4].y = point[1].y = my;
	point[1].x = point[2].x = Mx;
	point[2].y = point[3].y = My;
	
	msDrawShadeSymbol(&map->symbolset,image,&shape,style,1.0);
}

int msDrawVBarChart(mapObj *map, imageObj *image, pointObj *center,
        float *values, styleObj **styles, int numvalues,            
        float barWidth)
{

    int c;
    float left,bottom,cur; /*shortcut to pixel boundaries of the chart*/
    float height = 0; 

    for(c=0;c<numvalues;c++)
    {
        height += values[c];
    }

    cur = bottom = center->y+height/2.;
    left = center->x-barWidth/2.;
     
    for(c=0;c<numvalues;c++)
    {
        drawRectangle(map, image, left, cur, left+barWidth, cur-values[c], styles[c]);
        cur -= values[c];
    }
    return MS_SUCCESS;
}


int msDrawBarChart(mapObj *map, imageObj *image, pointObj *center,
        float *values, styleObj **styles, int numvalues,            
        float width, float height, float *maxVal, float *minVal, float barWidth)
{

    float upperLimit,lowerLimit;
    float shapeMaxVal,shapeMinVal,pixperval;
    int c;
    float vertOrigin,vertOriginClipped,horizStart,y;
    float left,top,bottom; /*shortcut to pixel boundaries of the chart*/
    
    top=center->y-height/2.;
    bottom=center->y+height/2.;
    left=center->x-width/2.;

    shapeMaxVal=shapeMinVal=values[0];
    for(c=1;c<numvalues;c++)
    {
        if(maxVal==NULL || minVal==NULL) { /*compute bounds if not specified*/
            if(values[c]>shapeMaxVal)
                shapeMaxVal=values[c];
            if(values[c]<shapeMinVal)
                shapeMinVal=values[c];
        }
    }
    
    /*
     * use specified bounds if wanted
     * if not, always show the origin
     */
    upperLimit = (maxVal!=NULL)? *maxVal : MS_MAX(shapeMaxVal,0);
    lowerLimit = (minVal!=NULL)? *minVal : MS_MIN(shapeMinVal,0);
    if(upperLimit==lowerLimit) {
      /* treat the case where we would have an unspecified behavior */
      upperLimit+=0.5;
      lowerLimit-=0.5;
    }
    
    pixperval=(float)height/(upperLimit-lowerLimit);
    vertOrigin=bottom+lowerLimit*pixperval;
    vertOriginClipped=(vertOrigin<top) ? top : 
                        (vertOrigin>bottom) ? bottom : vertOrigin;
    horizStart=left;
    /*
    color = gdImageColorAllocate(image->img.gd, 0,0,0);
    gdImageRectangle(image->img.gd, left-1,top-1, center.x+width/2.+1,bottom+1,color);
    */
    for(c=0;c<numvalues;c++)
    {
        int barHeight=values[c]*pixperval;
        /*clip bars*/
        y=((vertOrigin-barHeight)<top) ? top : 
            (vertOrigin-barHeight>bottom) ? bottom : vertOrigin-barHeight;
        if(y!=vertOriginClipped) { /*don't draw bars of height == 0 (i.e. either values==0, or clipped)*/
            if(values[c]>0) 
                drawRectangle(map, image, horizStart, y, horizStart+barWidth-1, vertOriginClipped, styles[c]);
            else
                drawRectangle(map,image, horizStart, vertOriginClipped, horizStart+barWidth-1 , y, styles[c]);
        }
        horizStart+=barWidth;
    }
    return MS_SUCCESS;
}

int msDrawPieChart(mapObj *map, imageObj *image,
        pointObj *center, float diameter,
        float *values, styleObj **styles, int numvalues)
{
    int i;
    double dTotal=0.,start=0;

    for(i=0;i<numvalues;i++)
    {
        if(values[i]<0.) {
            msSetError(MS_MISCERR, "cannot draw pie charts for negative values", "msDrawPieChart()");
            return MS_FAILURE;
        }
        dTotal+=values[i];
    }

    for(i=0; i < numvalues; i++)
    {
        float angle = values[i];
        if(angle==0) continue; /*no need to draw. causes artifacts with outlines*/
        angle*=360.0/dTotal;
        msDrawPieSlice(&map->symbolset,image, center, styles[i], diameter/2., start, start+angle);
        
        start+=angle;
    }
    return MS_SUCCESS;
}

int getNextShape(mapObj *map, layerObj *layer, float *values, styleObj **styles, shapeObj *shape) {
    int status;
    int c;
    status = msLayerNextShape(layer, shape);
    if(status == MS_SUCCESS) {
#ifdef USE_PROJ
        if(layer->project && msProjectionsDiffer(&(layer->projection), &(map->projection)))
            msProjectShape(&layer->projection, &map->projection, shape);
        else
            layer->project = MS_FALSE;
#endif

        if(msBindLayerToShape(layer, shape, MS_FALSE) != MS_SUCCESS)
            return MS_FAILURE; /* error message is set in msBindLayerToShape() */

        for(c=0;c<layer->numclasses;c++)
        {
            values[c]=(layer->class[c]->styles[0]->size);
            styles[c]=layer->class[c]->styles[0];
        }
    }
    return status;
}

/* eventually add a class to the layer to get the diameter from an attribute */
int pieLayerProcessDynamicDiameter(layerObj *layer) {
    const char *chartRangeProcessingKey=NULL;
    char *attrib;
    float mindiameter=-1, maxdiameter, minvalue, maxvalue;
    classObj *newclass;
    styleObj *newstyle;
    const char *chartSizeProcessingKey=msLayerGetProcessingKey( layer,"CHART_SIZE" );
    if(chartSizeProcessingKey != NULL)
        return MS_FALSE;
    chartRangeProcessingKey=msLayerGetProcessingKey( layer,"CHART_SIZE_RANGE" );
    if(chartRangeProcessingKey==NULL)
        return MS_FALSE;
    attrib = msSmallMalloc(strlen(chartRangeProcessingKey)+1);
    switch(sscanf(chartRangeProcessingKey,"%s %f %f %f %f",attrib,
                &mindiameter,&maxdiameter,&minvalue,&maxvalue))
    {
        case 1: /*we only have the attribute*/
        case 5: /*we have the attribute and the four range values*/
            break;
        default:
            free(attrib);
            msSetError(MS_MISCERR, "Chart Layer format error for processing key \"CHART_RANGE\"", "msDrawChartLayer()");
            return MS_FAILURE;
    }
    /*create a new class in the layer containing the wanted attribute
     * as the SIZE of its first STYLE*/
    newclass=msGrowLayerClasses(layer);
    if(newclass==NULL) {
        free(attrib);
        return MS_FAILURE;
    }
    initClass(newclass);
    layer->numclasses++;

    /*create and attach a new styleObj to our temp class
     * and bind the wanted attribute to its SIZE
     */
    newstyle=msGrowClassStyles(newclass);
    if(newstyle==NULL) {
        free(attrib);
        return MS_FAILURE;
    }
    initStyle(newstyle);
    newclass->numstyles++;
    newclass->name=(char*)msStrdup("__MS_SIZE_ATTRIBUTE_");
    newstyle->bindings[MS_STYLE_BINDING_SIZE].item=msStrdup(attrib);
    newstyle->numbindings++;
    free(attrib);

    return MS_TRUE;

}


int msDrawPieChartLayer(mapObj *map, layerObj *layer, imageObj *image)
{
    shapeObj    shape;
    int         status=MS_SUCCESS;
    const char *chartRangeProcessingKey=NULL;
    const char *chartSizeProcessingKey=msLayerGetProcessingKey( layer,"CHART_SIZE" );
    float diameter, mindiameter=-1, maxdiameter, minvalue, maxvalue, exponent=0;
    float *values;
    styleObj **styles;
    pointObj center;
    int numvalues = layer->numclasses; /* the number of classes to represent in the graph */
    if(chartSizeProcessingKey==NULL)
    {
        chartRangeProcessingKey=msLayerGetProcessingKey( layer,"CHART_SIZE_RANGE" );
        if(chartRangeProcessingKey==NULL)
            diameter=20;
        else {
            sscanf(chartRangeProcessingKey,"%*s %f %f %f %f %f",
                &mindiameter,&maxdiameter,&minvalue,&maxvalue,&exponent);
        }
    }
    else
    {
        if(sscanf(chartSizeProcessingKey ,"%f",&diameter)!=1) {
            msSetError(MS_MISCERR, "msDrawChart format error for processing key \"CHART_SIZE\"", "msDrawPieChartLayer()");
            return MS_FAILURE;
        }
    }
    /* step through the target shapes */
    msInitShape(&shape);

    values=(float*)calloc(numvalues,sizeof(float));
    MS_CHECK_ALLOC(values, numvalues*sizeof(float), MS_FAILURE);
    styles = (styleObj**)malloc((numvalues)*sizeof(styleObj*));
    if (styles == NULL)
    {
        msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msDrawPieChartLayer()",
                   __FILE__, __LINE__, numvalues*sizeof(styleObj*)); 
        free(values);
        return MS_FAILURE;
    }

    if(chartRangeProcessingKey!=NULL) 
        numvalues--;
    while(MS_SUCCESS == getNextShape(map,layer,values,styles,&shape)) {
        msDrawStartShape(map, layer, image, &shape);
        if(chartRangeProcessingKey!=NULL) {
            diameter = values[numvalues];
            if(mindiameter>=0) {
                if(diameter<=minvalue)
                    diameter=mindiameter;
                else if(diameter>=maxvalue)
                    diameter=maxdiameter;
                else {
                    if (exponent <= 0)
                        diameter=MS_NINT(
                            mindiameter+
                            ((diameter-minvalue)/(maxvalue-minvalue))*
                            (maxdiameter-mindiameter)
                        );
                    else
                        diameter=MS_NINT(
                            mindiameter+
                            pow((diameter-minvalue)/(maxvalue-minvalue),1.0/exponent)*
                            (maxdiameter-mindiameter)
                        );
                }
            }
        }
        if(findChartPoint(map, &shape, diameter, diameter, &center) == MS_SUCCESS) {
            status = msDrawPieChart(map,image, &center, diameter,
                values,styles,numvalues);
        }
        msDrawEndShape(map,layer,image,&shape);
        msFreeShape(&shape);
    }
    free(values);
    free(styles);
    return status;
}

int msDrawVBarChartLayer(mapObj *map, layerObj *layer, imageObj *image)
{
    shapeObj    shape;
    int         status=MS_SUCCESS;
    const char *chartSizeProcessingKey=msLayerGetProcessingKey( layer,"CHART_SIZE" );
    const char *chartScaleProcessingKey=msLayerGetProcessingKey( layer,"CHART_SCALE" );
    float barWidth,scale=1.0;
    float *values;
    styleObj **styles;
    pointObj center;
    int numvalues = layer->numclasses; 
    if(chartSizeProcessingKey==NULL)
    {
        barWidth=20;
    }
    else
    {
        if(sscanf(chartSizeProcessingKey ,"%f",&barWidth) != 1) {
            msSetError(MS_MISCERR, "msDrawChart format error for processing key \"CHART_SIZE\"", "msDrawVBarChartLayer()");
            return MS_FAILURE;
        }
    }

    if(chartScaleProcessingKey){
        if(sscanf(chartScaleProcessingKey,"%f",&scale)!=1) {
            msSetError(MS_MISCERR, "Error reading value for processing key \"CHART_SCALE\"", "msDrawVBarChartLayer()");
            return MS_FAILURE;
        }
    }
    msInitShape(&shape);

    values=(float*)calloc(numvalues,sizeof(float));
    MS_CHECK_ALLOC(values, numvalues*sizeof(float), MS_FAILURE);
    styles = (styleObj**)malloc(numvalues*sizeof(styleObj*));
    if (styles == NULL)
    {
        msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msDrawVBarChartLayer()",
                   __FILE__, __LINE__, numvalues*sizeof(styleObj*)); 
        free(values);
        return MS_FAILURE;
    }

    while(MS_SUCCESS == getNextShape(map,layer,values,styles,&shape)) {
        int i;
        double h=0;
        for(i=0;i<numvalues;i++) {
            values[i]*=scale;
            h += values[i];
        }
        msDrawStartShape(map, layer, image, &shape);
        if(findChartPoint(map, &shape, barWidth,h, &center)==MS_SUCCESS) {
            status = msDrawVBarChart(map,image,
                &center,
                values, styles, numvalues,
                barWidth);
        }
        msDrawEndShape(map,layer,image,&shape);
        msFreeShape(&shape);
    }
    free(values);
    free(styles);
    return status;
}


int msDrawBarChartLayer(mapObj *map, layerObj *layer, imageObj *image)
{
    shapeObj    shape;
    int         status=MS_SUCCESS;
    const char *chartSizeProcessingKey=msLayerGetProcessingKey( layer,"CHART_SIZE" );
    const char *barMax=msLayerGetProcessingKey( layer,"CHART_BAR_MAXVAL" );
    const char *barMin=msLayerGetProcessingKey( layer,"CHART_BAR_MINVAL" );
    float width,height;
    float barWidth;
    float *values;
    styleObj **styles;
    pointObj center;
    float barMaxVal,barMinVal;
    int numvalues = layer->numclasses; 
    if(chartSizeProcessingKey==NULL)
    {
        width=height=20;
    }
    else
    {
        switch(sscanf(chartSizeProcessingKey ,"%f %f",&width,&height)) {
            case 2:
                break;
            case 1:
                height = width;
                break;
            default:
                msSetError(MS_MISCERR, "msDrawChart format error for processing key \"CHART_SIZE\"", "msDrawBarChartLayer()");
                return MS_FAILURE;
        }
    }

    if(barMax){
        if(sscanf(barMax,"%f",&barMaxVal)!=1) {
            msSetError(MS_MISCERR, "Error reading value for processing key \"CHART_BAR_MAXVAL\"", "msDrawBarChartLayer()");
            return MS_FAILURE;
        }
    }
    if(barMin) {
        if(sscanf(barMin,"%f",&barMinVal)!=1) {
            msSetError(MS_MISCERR, "Error reading value for processing key \"CHART_BAR_MINVAL\"", "msDrawBarChartLayer()");
            return MS_FAILURE;
        }
    }
    if(barMin && barMax && barMinVal>=barMaxVal) {
        msSetError(MS_MISCERR, "\"CHART_BAR_MINVAL\" must be less than \"CHART_BAR_MAXVAL\"", "msDrawBarChartLayer()");
        return MS_FAILURE;
    }
    barWidth=(float)width/(float)layer->numclasses;
    if(!barWidth)
    {
        msSetError(MS_MISCERR, "Specified width of chart too small to fit given number of classes", "msDrawBarChartLayer()");
        return MS_FAILURE;
    }

    msInitShape(&shape);

    values=(float*)calloc(numvalues,sizeof(float));
    MS_CHECK_ALLOC(values, numvalues*sizeof(float), MS_FAILURE);
    styles = (styleObj**)malloc(numvalues*sizeof(styleObj*));
    if (styles == NULL)
    {
        msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msDrawBarChartLayer()",
                   __FILE__, __LINE__, numvalues*sizeof(styleObj*)); 
        free(values);
        return MS_FAILURE;
    }

    while(MS_SUCCESS == getNextShape(map,layer,values,styles,&shape)) {
        msDrawStartShape(map, layer, image, &shape);
        if(findChartPoint(map, &shape, width,height, &center)==MS_SUCCESS) {
            status = msDrawBarChart(map,image,
                &center,
                values, styles, numvalues,
                width,height,
                (barMax!=NULL)?&barMaxVal:NULL,
                (barMin!=NULL)?&barMinVal:NULL,
                barWidth);
        }
        msDrawEndShape(map,layer,image,&shape);
        msFreeShape(&shape);
    }
    free(values);
    free(styles);
    return status;
}

/**
 * Generic function to render chart layers.
 */
int msDrawChartLayer(mapObj *map, layerObj *layer, imageObj *image)
{

    rectObj     searchrect;
    const char *chartTypeProcessingKey=msLayerGetProcessingKey( layer,"CHART_TYPE" );
    int chartType=MS_CHART_TYPE_PIE;
    int status = MS_FAILURE;
    
    if (image && map && layer)
    {
        if( !(MS_RENDERER_PLUGIN(image->format) )) {
            msSetError(MS_MISCERR, "chart drawing currently only supports GD and AGG renderers", "msDrawChartLayer()");
            return MS_FAILURE;
        }
        
        if( layer->numclasses < 2 ) {
            msSetError(MS_MISCERR,"chart drawing requires at least 2 classes in layer", "msDrawChartLayer()");
            return MS_FAILURE;
        }
    
        if(chartTypeProcessingKey!=NULL) {
            if( strcasecmp(chartTypeProcessingKey,"PIE") == 0 ) {
                chartType=MS_CHART_TYPE_PIE;
            }
            else if( strcasecmp(chartTypeProcessingKey,"BAR") == 0 ) {
                chartType=MS_CHART_TYPE_BAR;
            }
            else if( strcasecmp(chartTypeProcessingKey,"VBAR") == 0 ) {
                chartType=MS_CHART_TYPE_VBAR;
            }
            else {
                msSetError(MS_MISCERR,"unknown chart type for processing key \"CHART_TYPE\", must be one of \"PIE\" or \"BAR\"", "msDrawChartLayer()");
                return MS_FAILURE;
            }
        }
        if(chartType == MS_CHART_TYPE_PIE) {
            pieLayerProcessDynamicDiameter(layer);
        }
            
        /* open this layer */
        status = msLayerOpen(layer);
        if(status != MS_SUCCESS) return MS_FAILURE;
    
        status = msLayerWhichItems(layer, MS_FALSE, NULL);
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
        
        status = msLayerWhichShapes(layer, searchrect, MS_FALSE);
        if(status == MS_DONE) { /* no overlap */
            msLayerClose(layer);
            return MS_SUCCESS;
        } else if(status != MS_SUCCESS) {
            msLayerClose(layer);
            return MS_FAILURE;
        }
        switch(chartType) {
            case MS_CHART_TYPE_PIE:
                status = msDrawPieChartLayer(map, layer, image);
                break;
            case MS_CHART_TYPE_BAR:
                status = msDrawBarChartLayer(map, layer, image);
                break;
            case MS_CHART_TYPE_VBAR:
                status = msDrawVBarChartLayer(map, layer, image);
                break;
            default:
                return MS_FAILURE;/*shouldn't be here anyways*/
        }
    
        msLayerClose(layer);  
    }
    return status;
}

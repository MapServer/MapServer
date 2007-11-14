/******************************************************************************
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
            if( MS_RENDERER_GD(map->outputformat) ) {
                center->x=MS_MAP2IMAGE_X(shape->line[0].point[0].x, map->extent.minx, map->cellsize);
                center->y=MS_MAP2IMAGE_Y(shape->line[0].point[0].y, map->extent.maxy, map->cellsize);
            }
            #ifdef USE_AGG
            else if( MS_RENDERER_AGG(map->outputformat) ) {
                center->x=MS_MAP2IMAGE_X_IC_DBL(shape->line[0].point[0].x, map->extent.minx, invcellsize);
                center->y=MS_MAP2IMAGE_Y_IC_DBL(shape->line[0].point[0].y, map->extent.maxy, invcellsize);
            }
            #endif
            
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
                    if( MS_RENDERER_GD(map->outputformat) ) {
                        center->x=MS_MAP2IMAGE_X(center->x, map->extent.minx, map->cellsize);
                        center->y=MS_MAP2IMAGE_Y(center->y, map->extent.maxy, map->cellsize);
                    }
                    #ifdef USE_AGG
                    else if( MS_RENDERER_AGG(map->outputformat) ) {
                        center->x=MS_MAP2IMAGE_X_IC_DBL(center->x, map->extent.minx, invcellsize);
                        center->y=MS_MAP2IMAGE_Y_IC_DBL(center->y, map->extent.maxy, invcellsize);
                    }
                    #endif
                    if(MS_CHART_FITS(center->x,center->y,width,height,map->width,map->height))
                        return MS_SUCCESS;
                    
                    break;
                }
                idx=middle-offset;
                if(idx>=0) {
                    center->x=(shape->line[0].point[idx].x+shape->line[0].point[idx+1].x)/2;
                    center->y=(shape->line[0].point[idx].y+shape->line[0].point[idx+1].y)/2;
                    if( MS_RENDERER_GD(map->outputformat) ) {
                        center->x=MS_MAP2IMAGE_X(center->x, map->extent.minx, map->cellsize);
                        center->y=MS_MAP2IMAGE_Y(center->y, map->extent.maxy, map->cellsize);
                    }
                    #ifdef USE_AGG
                    else if( MS_RENDERER_AGG(map->outputformat) ) {
                        center->x=MS_MAP2IMAGE_X_IC_DBL(center->x, map->extent.minx, invcellsize);
                        center->y=MS_MAP2IMAGE_Y_IC_DBL(center->y, map->extent.maxy, invcellsize);
                    }
                    #endif
                    if(MS_CHART_FITS(center->x,center->y,width,height,map->width,map->height))
                        return MS_SUCCESS;
                    break;
                }
            }
            return MS_FAILURE;
        break;
        case MS_SHAPE_POLYGON:
            msPolygonLabelPoint(shape, center, -1);
            if( MS_RENDERER_GD(map->outputformat) ) {
                center->x=MS_MAP2IMAGE_X(center->x, map->extent.minx, map->cellsize);
                center->y=MS_MAP2IMAGE_Y(center->y, map->extent.maxy, map->cellsize);
            }
            #ifdef USE_AGG
            else if( MS_RENDERER_AGG(map->outputformat) ) {
                center->x=MS_MAP2IMAGE_X_IC_DBL(center->x, map->extent.minx, invcellsize);
                center->y=MS_MAP2IMAGE_Y_IC_DBL(center->y, map->extent.maxy, invcellsize);
            }
            #endif
            if(MS_CHART_FITS(center->x,center->y,width,height,map->width,map->height))
                return MS_SUCCESS;
            else
                return MS_FAILURE;
            break;
        default:
            return MS_FAILURE;
    }
}

int msDrawBarChart(mapObj *map, layerObj *layer, shapeObj *shape, imageObj *image,
                    int width, int height, float *maxVal, float *minVal, float barWidth)
{

    pointObj center;
    float upperLimit,lowerLimit;
    float *values,shapeMaxVal,shapeMinVal,pixperval;
    int c,color,outlinecolor,outlinewidth;
    float vertOrigin,vertOriginClipped,horizStart,y;
    float left,top,bottom; /*shortcut to pixel boundaries of the chart*/
    msDrawStartShape(map, layer, image, shape);
#ifdef USE_PROJ
    if(layer->project && msProjectionsDiffer(&(layer->projection), &(map->projection)))
        msProjectShape(&layer->projection, &map->projection, shape);
    else
        layer->project = MS_FALSE;
#endif
    if(layer->transform == MS_TRUE) {
      if(findChartPoint(map, shape, width, height, &center)==MS_FAILURE)
        return MS_SUCCESS; /*next shape*/
    } else {
        /* why would this ever be used? */
        msOffsetPointRelativeTo(&center, layer); 
    }
    
    top=center.y-height/2.;
    bottom=center.y+height/2.;
    left=center.x-width/2.;

    if(msBindLayerToShape(layer, shape) != MS_SUCCESS)
        return MS_FAILURE; /* error message is set in msBindLayerToShape() */

    values=(float*)calloc(layer->numclasses,sizeof(float));

    shapeMaxVal=shapeMinVal=0;
    for(c=0;c<layer->numclasses;c++)
    {
        values[c]=(layer->class[c]->styles[0]->size);
        if(maxVal==NULL || minVal==NULL) { /*compute bounds if not specified*/
            if(c==0)
                shapeMaxVal=shapeMinVal=values[0];
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
    
    pixperval=(float)height/(upperLimit-lowerLimit);
    vertOrigin=bottom+lowerLimit*pixperval;
    vertOriginClipped=(vertOrigin<top) ? top : 
                        (vertOrigin>bottom) ? bottom : vertOrigin;
    horizStart=left;
    /*
    color = gdImageColorAllocate(image->img.gd, 0,0,0);
    gdImageRectangle(image->img.gd, left-1,top-1, center.x+width/2.+1,bottom+1,color);
    */
    for(c=0;c<layer->numclasses;c++)
    {
        int barHeight=values[c]*pixperval;
        /*clip bars*/
        y=((vertOrigin-barHeight)<top) ? top : 
                    (vertOrigin-barHeight>bottom) ? bottom : vertOrigin-barHeight;
        if(y!=vertOriginClipped) { /*don't draw bars of height == 0 (i.e. either values==0, or clipped)*/
            if( MS_RENDERER_GD(map->outputformat) ) {
                color = gdImageColorResolve(image->img.gd, 
                        layer->class[c]->styles[0]->color.red,
                        layer->class[c]->styles[0]->color.green,
                        layer->class[c]->styles[0]->color.blue);
                outlinecolor=-1;outlinewidth=1;
                if(MS_VALID_COLOR(layer->class[c]->styles[0]->outlinecolor)) {
                    outlinecolor = gdImageColorResolve(image->img.gd, 
                            layer->class[c]->styles[0]->outlinecolor.red,
                            layer->class[c]->styles[0]->outlinecolor.green,
                            layer->class[c]->styles[0]->outlinecolor.blue);
                }
                if(layer->class[c]->styles[0]->width!=-1)
                    outlinewidth=layer->class[c]->styles[0]->width;
                if(values[c]>0) {
                    if(outlinecolor==-1) {
                        gdImageFilledRectangle(image->img.gd, horizStart,y, horizStart+barWidth-1 , vertOriginClipped,color);
                    } else {
                        gdImageFilledRectangle(image->img.gd, horizStart,y, horizStart+barWidth-1 , vertOriginClipped,outlinecolor);
                        gdImageFilledRectangle(image->img.gd, horizStart+outlinewidth,y+outlinewidth, horizStart+barWidth-1-outlinewidth , vertOriginClipped-outlinewidth,color);
                    }
                }
                else {
                    if(outlinecolor==-1) {
                        gdImageFilledRectangle(image->img.gd, horizStart, vertOriginClipped, horizStart+barWidth-1 , y,color);
                    } else {
                        gdImageFilledRectangle(image->img.gd, horizStart, vertOriginClipped, horizStart+barWidth-1 , y,outlinecolor);
                        gdImageFilledRectangle(image->img.gd, horizStart+outlinewidth, vertOriginClipped+outlinewidth, horizStart+barWidth-1-outlinewidth , y-outlinewidth,color);  
                    }
                }
                    
            }
            #ifdef USE_AGG
            else if( MS_RENDERER_AGG(map->outputformat) ) {
                int outline=0;
                if(MS_VALID_COLOR(layer->class[c]->styles[0]->outlinecolor))
                    outline=1; /*outlining is wierd if this isn't done*/
                msFilledRectangleAGG(image, layer->class[c]->styles[0], horizStart, y, horizStart+barWidth-outline , vertOriginClipped);
            }
            #endif       
        }
        horizStart+=barWidth;
    }
    free(values);

    return MS_SUCCESS;
}

int msDrawPieChart(mapObj *map, layerObj *layer, shapeObj *shape, 
        imageObj *image, int diameter,
        int range_class,
        float mindiameter,float maxdiameter,
        float minvalue,float maxvalue)
{
    int i,c,color,outlinecolor,outlinewidth;
    pointObj center;
    float *values;
    float dTotal=0.,start=0,center_x,center_y;
    msDrawStartShape(map, layer, image, shape);
#ifdef USE_PROJ
    if(layer->project && msProjectionsDiffer(&(layer->projection), &(map->projection)))
        msProjectShape(&layer->projection, &map->projection, shape);
    else
        layer->project = MS_FALSE;
#endif

    if(msBindLayerToShape(layer, shape) != MS_SUCCESS)
        return MS_FAILURE; /* error message is set in msBindLayerToShape() */
    
    /*check if dynamic diameter was wanted*/
    if(range_class>=0) {
        diameter=layer->class[range_class]->styles[0]->size;
        /*check if the diameter should be scaled according to specified bounds*/
        if(mindiameter>=0) {
            if(diameter<=minvalue)
                diameter=mindiameter;
            else if(diameter>=maxvalue)
                diameter=maxdiameter;
            else {
                diameter=MS_NINT(
                        mindiameter+
                        ((diameter-minvalue)/(maxvalue-minvalue))*
                        (maxdiameter-mindiameter)
                );
            }
        }
    }
    
    if(layer->transform == MS_TRUE) {
          if(findChartPoint(map, shape, diameter, diameter, &center)==MS_FAILURE)
            return MS_SUCCESS; /*next shape*/
    } else {
        /* why would this ever be used? */
        msOffsetPointRelativeTo(&center, layer); 
    }
    values=(float*)calloc(layer->numclasses,sizeof(float));
    for(c=0;c<layer->numclasses;c++)
    {
        if(c==range_class) continue; /*skip the class that gives the dynamic diameter*/
        values[c]=(layer->class[c]->styles[0]->size);
        if(values[c]<0.) {
            msSetError(MS_MISCERR, "cannot draw pie charts for negative values", "msDrawPieChart()");
            return MS_FAILURE;
        }
        dTotal+=values[c];
    }

    for(i=0; i < layer->numclasses; i++)
    {
        if(i==range_class) continue; /*skip the class that gives the dynamic diameter*/
        if(values[i]==0) continue; /*no need to draw. causes artifacts with outlines*/
        values[i]*=360.0/dTotal;
        if( MS_RENDERER_GD(map->outputformat) )
        {
            color = gdImageColorResolve(image->img.gd, layer->class[i]->styles[0]->color.red,
                                                layer->class[i]->styles[0]->color.green,
                                                layer->class[i]->styles[0]->color.blue);
            outlinecolor=-1;outlinewidth=1;
            if(MS_VALID_COLOR(layer->class[i]->styles[0]->outlinecolor)) {
                outlinecolor = gdImageColorResolve(image->img.gd, layer->class[i]->styles[0]->outlinecolor.red,
                        layer->class[i]->styles[0]->outlinecolor.green,
                        layer->class[i]->styles[0]->outlinecolor.blue);
            }
            if(layer->class[i]->styles[0]->width!=-1)
                outlinewidth=layer->class[i]->styles[0]->width;
            /* 
             * offset the center of the slice
             * NOTE: angles are anti-trigonometric
             * 
             */
            if(layer->class[i]->styles[0]->offsetx>0) {
                center_x=center.x+layer->class[i]->styles[0]->offsetx*cos(((-start-values[i]/2)*MS_PI/180.));
                center_y=center.y-layer->class[i]->styles[0]->offsetx*sin(((-start-values[i]/2)*MS_PI/180.));
            } else {
                center_x=center.x;
                center_y=center.y;
            }
            
            if(outlinecolor==-1) {
                gdImageFilledArc(image->img.gd, center_x, center_y, diameter, diameter, (int)start, (int)(start+values[i]), color, gdPie);               
            }
            else {
                gdImageFilledArc(image->img.gd, center_x, center_y, diameter, diameter, (int)start, (int)(start+values[i]), color, gdPie);
                gdImageSetThickness(image->img.gd, outlinewidth);
                gdImageFilledArc(image->img.gd, center_x, center_y, diameter, diameter, (int)start, (int)(start+values[i]), outlinecolor,gdNoFill|gdEdged);
                gdImageSetThickness(image->img.gd, 1);                              
            }
        }
        #ifdef USE_AGG
        else if( MS_RENDERER_AGG(map->outputformat) )
        {
            msPieSliceAGG(image, layer->class[i]->styles[0], center.x, center.y, diameter/2., start, start+values[i]);
        }
        #endif

        start+=values[i];
    }
    free(values);
    return MS_SUCCESS;
}

int msDrawPieChartLayer(mapObj *map, layerObj *layer, imageObj *image, 
                        int radius,
                        int range_class,
                        float mindiameter,float maxdiameter,
                        float minvalue,float maxvalue)
{
    shapeObj    shape;
    int         status=MS_SUCCESS;
    /* step through the target shapes */
    msInitShape(&shape);
    while((status==MS_SUCCESS)&&(msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
        status = msDrawPieChart(map, layer, &shape, image,radius,
                range_class,mindiameter,maxdiameter,minvalue,maxvalue);
        msFreeShape(&shape);
    }
    return status;
}

int msDrawBarChartLayer(mapObj *map, layerObj *layer, imageObj *image, 
                        int width, int height)
{
    shapeObj    shape;
    int         status=MS_SUCCESS;
    const char *barMax=msLayerGetProcessingKey( layer,"CHART_BAR_MAXVAL" );
    const char *barMin=msLayerGetProcessingKey( layer,"CHART_BAR_MINVAL" );
    float barWidth;
    float barMaxVal,barMinVal;

    if(barMax){
        if(sscanf(barMax,"%f",&barMaxVal)!=1) {
            msSetError(MS_MISCERR, "Error reading value for processing key \"CHART_BAR_MAXVAL\"", "msDrawBarChartLayerGD()");
            return MS_FAILURE;
        }
    }
    if(barMin) {
        if(sscanf(barMin,"%f",&barMinVal)!=1) {
            msSetError(MS_MISCERR, "Error reading value for processing key \"CHART_BAR_MINVAL\"", "msDrawBarChartLayerGD()");
            return MS_FAILURE;
        }
    }
    if(barMin && barMax && barMinVal>=barMaxVal) {
        msSetError(MS_MISCERR, "\"CHART_BAR_MINVAL\" must be less than \"CHART_BAR_MAXVAL\"", "msDrawBarChartLayerGD()");
        return MS_FAILURE;
    }
    barWidth=(float)width/(float)layer->numclasses;
    if(!barWidth)
    {
        msSetError(MS_MISCERR, "Specified width of chart too small to fit given number of classes", "msDrawBarChartLayerGD()");
        return MS_FAILURE;
    }
    /* step through the target shapes */
    msInitShape(&shape);
    while((status==MS_SUCCESS)&&(msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
        status = msDrawBarChart(map, layer, &shape, image,width,height,(barMax!=NULL)?&barMaxVal:NULL,(barMin!=NULL)?&barMinVal:NULL,barWidth);
        msFreeShape(&shape);
    }
    return status;
}

/**
 * Generic function to render chart layers.
 */
int msDrawChartLayer(mapObj *map, layerObj *layer, imageObj *image)
{

    char        annotate=MS_TRUE;
    rectObj     searchrect;
    const char *chartTypeProcessingKey=msLayerGetProcessingKey( layer,"CHART_TYPE" );
    const char *chartSizeProcessingKey=msLayerGetProcessingKey( layer,"CHART_SIZE" );
    const char *chartRangeProcessingKey=msLayerGetProcessingKey( layer,"CHART_SIZE_RANGE" );
    int chartType=MS_CHART_TYPE_PIE;
    int width,height;
    float mindiameter=-1.,maxdiameter=-1.,minvalue=-1.,maxvalue=-1.;
    int tmpclassindex=-1;
    int status = MS_FAILURE;
    
    if (image && map && layer)
    {
        if( !(MS_RENDERER_GD(image->format) || MS_RENDERER_AGG(image->format) )) {
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
            else {
                msSetError(MS_MISCERR,"unknown chart type for processing key \"CHART_TYPE\", must be one of \"PIE\" or \"BAR\"", "msDrawChartLayer()");
                return MS_FAILURE;
            }
        }
            
        if(chartSizeProcessingKey==NULL)
        {
            width=height=20;
        }
        else
        {
            switch(sscanf(chartSizeProcessingKey ,"%d %d",&width,&height))
            {
            case 2: 
                if(chartType==MS_CHART_TYPE_PIE) {
                    msSetError(MS_MISCERR,"only one size (radius) supported for processing key \"CHART_SIZE\" for pie chart layers", "msDrawChartLayer()");
                    return MS_FAILURE;
                }
                break;
            case 1: height=width;break;
            default:
                msSetError(MS_MISCERR, "msDrawChart format error for processing key \"CHART_SIZE\"", "msDrawChartLayer()");
                return MS_FAILURE;
            }
        }
        
        if(chartType==MS_CHART_TYPE_PIE && chartRangeProcessingKey) { /*if CHART_SIZE_RANGE was specified*/
            char *attrib;
            classObj *newclass;
            styleObj *newstyle;
            attrib = malloc(strlen(chartRangeProcessingKey)+1);
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
            
            /*remember the new class's index so that:
             * - we can delete the class when we've finished processing the layer
             * - we can tell the pie rendering functions where it should read the size value
             */
            tmpclassindex=layer->numclasses; 
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
            newclass->name=strdup("__MS_SIZE_ATTRIBUTE_");
            newstyle->bindings[MS_STYLE_BINDING_SIZE].item=strdup(attrib);
            newstyle->numbindings++;
            free(attrib);
        }
    
        annotate = msEvalContext(map, layer, layer->labelrequires);
        if(map->scaledenom > 0) {
            if((layer->labelmaxscaledenom != -1) && (map->scaledenom >= layer->labelmaxscaledenom)) annotate = MS_FALSE;
            if((layer->labelminscaledenom != -1) && (map->scaledenom < layer->labelminscaledenom)) annotate = MS_FALSE;
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
        switch(chartType) {
            case MS_CHART_TYPE_PIE:
                status = msDrawPieChartLayer(map, layer, image,width,
                        tmpclassindex,mindiameter,maxdiameter,minvalue,maxvalue);
                break;
            case MS_CHART_TYPE_BAR:
                status = msDrawBarChartLayer(map, layer, image,width,height);
                break;
            default:
                return MS_FAILURE;/*shouldn't be here anyways*/
        }
    
        msLayerClose(layer);  
    }
    /*
     * if we're using a dynamic size for pie layers, free the temporary class we used
     * so it doesn't appear in legends, etc...
     */
    if(tmpclassindex>=0) {
        classObj *c=msRemoveClass(layer,tmpclassindex);
        freeClass(c);
        msFree(c);
    }
    return status;
}

/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Legend generation.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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

MS_CVSID("$Id$")

#define PSF .8
#define VMARGIN 5 /* margin at top and bottom of legend graphic */
#define HMARGIN 5 /* margin at left and right of legend graphic */

/* 
 * generic function for drawing a legend icon. (added for bug #2348)
 * renderer specific drawing functions shouldn't be called directly, but through
 * this function
 */
int msDrawLegendIcon(mapObj *map, layerObj *lp, classObj *class, int width, int height, imageObj *img, int dstX, int dstY)
{
#ifdef USE_AGG
    if(MS_RENDERER_AGG(map->outputformat))
        return msDrawLegendIconAGG(map,lp,class,width,height,img,dstX,dstY);
    else
#endif
        /*default is to draw with GD*/
        return msDrawLegendIconGD(map,lp,class,width,height,img->img.gd,dstX,dstY);
}

imageObj *msCreateLegendIcon(mapObj* map, layerObj* lp, classObj* class, int width, int height)
{
  imageObj *image;
  outputFormatObj *format = NULL;
  int i = 0;

  if(!map->outputformat || (!MS_RENDERER_GD(map->outputformat) && !MS_RENDERER_AGG(map->outputformat) )) {
    msSetError(MS_GDERR, "Map outputformat must be set to a GD or AGG format!", "msCreateLegendIcon()");
    return(NULL);
  }

  /* ensure we have an image format representing the options for the legend */
  msApplyOutputFormat(&format, map->outputformat, map->legend.transparent, map->legend.interlace, MS_NOOVERRIDE);

  /* create image */
#ifdef USE_AGG
  if( MS_RENDERER_AGG(map->outputformat) )
      image = msImageCreateAGG(width, height, map->outputformat, map->web.imagepath, map->web.imageurl);        
  else
#endif
      image = msImageCreateGD(width, height, map->outputformat, map->web.imagepath, map->web.imageurl);

  /* drop this reference to output format */
  msApplyOutputFormat( &format, NULL, MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE );

  if(image == NULL) {
    msSetError(MS_GDERR, "Unable to initialize image.","msCreateLegendIcon()");
    return(NULL);
  }

  /* allocate the background color */
#ifdef USE_AGG
  if( MS_RENDERER_AGG(map->outputformat) )
      msImageInitAGG( image, &(map->legend.imagecolor));
  else
#endif
  msImageInitGD( image, &(map->legend.imagecolor));

  /* Call drawLegendIcon with destination (0, 0) */
  /* Return an empty image if lp==NULL || class=NULL  */
  /* (If class is NULL draw the legend for all classes. Modifications done */
  /* Fev 2004 by AY) */
  if (lp) {
    msClearLayerPenValues(lp); /* just in case the mapfile has already been processed */
    if (class) {
      msDrawLegendIcon(map, lp, class, width, height, image, 0, 0);
    } else {
      for (i=0; i<lp->numclasses; i++) {
        msDrawLegendIcon(map, lp, lp->class[i], width, height, image, 0, 0);
      }
    }
  }
#ifdef USE_AGG
  if( MS_RENDERER_AGG(map->outputformat) )
      msAlphaAGG2GD(image);
#endif
  return image;
}

/*
 * Calculates the optimal size for the legend. If the optional layerObj
 * argument is given, the legend size will be calculated for only that
 * layer. Otherwise, the legend size is calculated for all layers that
 * are not MS_OFF or of MS_LAYER_QUERY type.
 *     
 * Returns one of:
 *   MS_SUCCESS
 *   MS_FAILURE
 */
int msLegendCalcSize(mapObj *map, int scale_independent, int *size_x,
                     int *size_y, layerObj *psForLayer) {
    int i, j;
    int status, maxwidth=0, nLegendItems=0;
    char *transformedText; // Label text after applying wrapping,
                           // encoding if necessary
    layerObj *lp;  
    rectObj rect;
    
    /* Reset sizes */
    *size_x = 0;
    *size_y = 0;
    
    /* Enable scale-dependent calculations */
    if (!scale_independent) {
        map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
        status = msCalculateScale(map->extent, map->units, map->width,
                                  map->height, map->resolution, &map->scaledenom);
        if (status != MS_SUCCESS) return MS_FAILURE;
    }


    /*
     * step through all map classes, and for each one that will be displayed
     * calculate the label size
     */
    for (i=0; i<map->numlayers; i++) 
    {
        if (psForLayer)
        {
            lp = psForLayer;
            i = map->numlayers;
        }
        else
          lp = (GET_LAYER(map, map->layerorder[i]));

        if ((lp->status == MS_OFF && lp != psForLayer) || (lp->type == MS_LAYER_QUERY)) /* skip it */
            continue;
            
        if (!scale_independent && map->scaledenom > 0) {
            if ((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom))
                continue;
            if ((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom))
                continue;
        }
        
        for (j=lp->numclasses-1; j>=0; j--) 
        {
            if (!lp->class[j]->name) continue; /* skip it */
            
            /* skip the class if the classgroup is defined*/
            if (lp->classgroup && 
                (lp->class[j]->group == NULL || strcasecmp(lp->class[j]->group, lp->classgroup) != 0))
              continue;

            /* Verify class scale */
            if (!scale_independent && map->scaledenom > 0) {
                if (   (lp->class[j]->maxscaledenom > 0) 
                    && (map->scaledenom > lp->class[j]->maxscaledenom))
                    continue;
                    
                if (   (lp->class[j]->minscaledenom > 0)
                    && (map->scaledenom <= lp->class[j]->minscaledenom))
                    continue;
            }
            
            /*
             * apply encoding and line wrapping to the legend label if requested
             * this is done conditionnally as the text transformation function
             * does some memory allocations that can be avoided in most cases.
             * the transformed text must be freed once finished, this must be done
             * conditionnally by testing if the transformed text pointer is the
             * same as the class name pointer
             */
            if (map->legend.label.encoding || map->legend.label.wrap)
                transformedText = msTransformLabelText(&map->legend.label,
                                                       lp->class[j]->name);
            else
              transformedText = strdup(lp->class[j]->name);

            if (   transformedText == NULL
                || msGetLabelSize(transformedText, &map->legend.label, 
                                  &rect, &(map->fontset), 1.0, MS_FALSE) != 0)
            { /* something bad happened */
                if (transformedText)
                  msFree(transformedText);

                return MS_FAILURE;
            }

            msFree(transformedText);
            maxwidth = MS_MAX(maxwidth, MS_NINT(rect.maxx - rect.minx));
            *size_y += MS_MAX(MS_NINT(rect.maxy - rect.miny), map->legend.keysizey);
            nLegendItems++;
        }
    }

    /* Calculate the size of the legend: */
    /*   - account for the Y keyspacing */
    *size_y += (2*VMARGIN) + ((nLegendItems-1) * map->legend.keyspacingy);
    /*   - determine the legend width */
    *size_x = (2*HMARGIN) + maxwidth + map->legend.keyspacingx + map->legend.keysizex;
    
    if (*size_y <=0 ||  *size_x <=0)
       return MS_FAILURE;

    return MS_SUCCESS;
}

/*
** Creates a GD image of a legend for a specific map. msDrawLegend()
** respects the current scale, and classes without a name are not
** added to the legend.
**
** scale_independent is used for WMS GetLegendGraphic. It should be set to
** MS_FALSE in most cases. If it is set to MS_TRUE then the layers' minscale
** and maxscale are ignored and layers that are currently out of scale still
** show up in the legend.
*/
imageObj *msDrawLegend(mapObj *map, int scale_independent)
{
    gdImagePtr img; /* image data structure */
    int i,j; /* loop counters */
    pointObj pnt;
    int size_x, size_y=0;
    layerObj *lp;  
    rectObj rect;
    imageObj *image = NULL;
    outputFormatObj *format = NULL;
    struct legend_struct {
        int height;
        char *transformedText;
        layerObj *layer;
        classObj *theclass;
        struct legend_struct* pred;
    };
    typedef struct legend_struct legendlabel;
    legendlabel *head=NULL,*cur=NULL;

    

    if(msValidateContexts(map) != MS_SUCCESS) return NULL; /* make sure there are no recursive REQUIRES or LABELREQUIRES expressions */

    if(msLegendCalcSize(map, scale_independent, &size_x, &size_y, NULL) != MS_SUCCESS) return NULL;

    /*
     * step through all map classes, and for each one that will be displayed
     * keep a reference to its label size and text
     */
    for(i=0; i<map->numlayers; i++) {
        lp = (GET_LAYER(map, map->layerorder[i]));

        if((lp->status == MS_OFF) || (lp->type == MS_LAYER_QUERY)) /* skip it */
            continue;
        if(!scale_independent && map->scaledenom > 0) {
            if((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom)) continue;
            if((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom)) continue;
        }
        for(j=lp->numclasses-1;j>=0;j--) {

          /* skip the class if the classgroup is defined*/
          if (lp->classgroup && 
              (lp->class[j]->group == NULL || strcasecmp(lp->class[j]->group, lp->classgroup) != 0))
            continue;
            if(!lp->class[j]->name) continue; /* skip it */
            if(!scale_independent && map->scaledenom > 0) {  /* verify class scale here */
                if((lp->class[j]->maxscaledenom > 0) && (map->scaledenom > lp->class[j]->maxscaledenom))
                    continue;
                if((lp->class[j]->minscaledenom > 0) && (map->scaledenom <= lp->class[j]->minscaledenom))
                    continue;
            }
            cur=(legendlabel*)malloc(sizeof(legendlabel));
            
            /*
             * apply encoding and line wrapping to the legend label if requested
             * this is done conditionnally as the text transformation function
             * does some memory allocations that can be avoided in most cases.
             * the transformed text must be freed once finished, this must be done
             * conditionnally by testing if the transformed text pointer is the
             * same as the class name pointer
             */
            if(map->legend.label.encoding || map->legend.label.wrap)
                cur->transformedText= msTransformLabelText(&map->legend.label,lp->class[j]->name);
            else
                cur->transformedText=lp->class[j]->name;
            cur->theclass=lp->class[j];
            cur->layer=lp;
            cur->pred=head;
            head=cur;
            if(cur->transformedText==NULL||
                    msGetLabelSize(cur->transformedText, &map->legend.label, &rect, &(map->fontset), 1.0, MS_FALSE) != 0)
            { /* something bad happened, free allocated mem */
                while(cur) {
                    if(cur->transformedText!=cur->theclass->name)
                        free(cur->transformedText);
                    head=cur;
                    cur=cur->pred;
                    free(head);
                }
                return(NULL); 
            }
            cur->height = MS_MAX(MS_NINT(rect.maxy - rect.miny), map->legend.keysizey);
        }
    }

    /* ensure we have an image format representing the options for the legend. */
    msApplyOutputFormat(&format, map->outputformat, map->legend.transparent, map->legend.interlace, MS_NOOVERRIDE);

    /* initialize the legend image */
#ifdef USE_AGG
    if( MS_RENDERER_AGG(map->outputformat) )
        image = msImageCreateAGG(size_x, size_y, format, map->web.imagepath, map->web.imageurl);        
    else
#endif
        image = msImageCreateGD(size_x, size_y, format, map->web.imagepath, map->web.imageurl);

    /* drop this reference to output format */
    msApplyOutputFormat(&format, NULL, MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE);

    if (image)
        img = image->img.gd;
    else {
        msSetError(MS_GDERR, "Unable to initialize image.", "msDrawLegend()");
        return(NULL);
    }

    /* Set background */
#ifdef USE_AGG
    if( MS_RENDERER_AGG(map->outputformat) )
        msImageInitAGG( image, &(map->legend.imagecolor));
    else
#endif
        msImageInitGD(image, &(map->legend.imagecolor));


    msClearPenValues(map); /* just in case the mapfile has already been processed */
    pnt.y = VMARGIN;
    pnt.x = HMARGIN + map->legend.keysizex + map->legend.keyspacingx;

    while(cur) { /*cur initially points on the last legend item, i.e. the one that should be at the top*/
        int number_of_newlines=0, offset=0;

        /*set the scale factor so that scale dependant symbols are drawn in the legend with their default size*/
        if(cur->layer->sizeunits != MS_PIXELS)
            cur->layer->scalefactor = (msInchesPerUnit(cur->layer->sizeunits,0)/msInchesPerUnit(map->units,0)) / map->cellsize;

        if(msDrawLegendIcon(map, cur->layer, cur->theclass,  map->legend.keysizex,  map->legend.keysizey, image, HMARGIN, (int) pnt.y) != MS_SUCCESS)
            return NULL;
        
        /*
         * adjust the baseline for multiline truetype labels. the label point is the bottom left 
         * corner of the *first* line, which we do not know exactly as we only have the
         * bounding box of the whole label. current approach is to suppose that all lines
         * mostly have the same height, and offset the starting point by the mean hight of 
         * one line. This isn't perfect but still much better than the previous approach
         */
        
        if(map->legend.label.type==MS_TRUETYPE &&
                (number_of_newlines=msCountChars(cur->transformedText,'\n'))>0) {
            offset=cur->height/(number_of_newlines+1);
            pnt.y += offset;
        }
        else
            pnt.y +=  cur->height;
        /* TODO: 
         * note tbonfort: if this todo concerned treating the individual heights of the legend labels,
         * then this is now done
         * */

        msDrawLabel(image, pnt, cur->transformedText, &(map->legend.label), &map->fontset, 1.0);
        if(offset) {
            /* if we had multiple lines, adjust the current position so it points
             * to the bottom of the current label
             */
            pnt.y += cur->height-offset;
        }
        pnt.y += map->legend.keyspacingy; /* bump y for next label */
        
        /*free the transformed text if needed*/
        if(cur->transformedText!=cur->theclass->name)
            free(cur->transformedText);
        head=cur;
        cur=cur->pred;
        free(head);
    } /* next legend */


#ifdef USE_AGG
    if(MS_RENDERER_AGG(map->outputformat))
        msAlphaAGG2GD(image);
#endif
    return(image);
}

/* TODO */
int msEmbedLegend(mapObj *map, imageObj *img)
{
  int s,l;
  pointObj point;
  imageObj *image = NULL;

  s = msGetSymbolIndex(&(map->symbolset), "legend", MS_FALSE);
  if(s == -1) {
    if (msGrowSymbolSet(&map->symbolset) == NULL)
        return -1;
    s = map->symbolset.numsymbols;
    map->symbolset.numsymbols++;
    initSymbol(map->symbolset.symbol[s]);
  } else {
    if(map->symbolset.symbol[s]->img) 
      gdImageDestroy(map->symbolset.symbol[s]->img);
  }
#ifdef USE_AGG
  if(MS_RENDERER_AGG(map->outputformat))
      msAlphaGD2AGG(img);
#endif
  
  /* render the legend. */
  image = msDrawLegend(map, MS_FALSE);

  /* steal the gdImage and free the rest of the imageObj */
  map->symbolset.symbol[s]->img = image->img.gd; 
  image->img.gd = NULL;
  msFreeImage( image );


  if(!map->symbolset.symbol[s]->img) return(-1); /* something went wrong creating scalebar */

  map->symbolset.symbol[s]->type = MS_SYMBOL_PIXMAP; /* intialize a few things */
  map->symbolset.symbol[s]->name = strdup("legend");  
  map->symbolset.symbol[s]->sizex = map->symbolset.symbol[s]->img->sx;
  map->symbolset.symbol[s]->sizey = map->symbolset.symbol[s]->img->sy;

  /* I'm not too sure this test is sufficient ... NFW. */
  if(map->legend.transparent == MS_ON)
    gdImageColorTransparent(map->symbolset.symbol[s]->img, 0);

  switch(map->legend.position) {
  case(MS_LL):
    point.x = MS_NINT(map->symbolset.symbol[s]->img->sx/2.0);
    point.y = map->height - MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_LR):
    point.x = map->width - MS_NINT(map->symbolset.symbol[s]->img->sx/2.0);
    point.y = map->height - MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_LC):
    point.x = MS_NINT(map->width/2.0);
    point.y = map->height - MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_UR):
    point.x = map->width - MS_NINT(map->symbolset.symbol[s]->img->sx/2.0);
    point.y = MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_UL):
    point.x = MS_NINT(map->symbolset.symbol[s]->img->sx/2.0);
    point.y = MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_UC):
    point.x = MS_NINT(map->width/2.0);
    point.y = MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  }

  l = msGetLayerIndex(map, "__embed__legend");
  if(l == -1) {
    if (msGrowMapLayers(map) == NULL)
        return(-1);
    l = map->numlayers;
    map->numlayers++;
    if(initLayer((GET_LAYER(map, l)), map) == -1) return(-1);
    GET_LAYER(map, l)->name = strdup("__embed__legend");
    GET_LAYER(map, l)->type = MS_LAYER_ANNOTATION;

    if (msGrowLayerClasses( GET_LAYER(map, l) ) == NULL)
        return(-1);
    if(initClass(GET_LAYER(map, l)->class[0]) == -1) return(-1);
    GET_LAYER(map, l)->numclasses = 1; /* so we make sure to free it */
        
    /* update the layer order list with the layer's index. */
    map->layerorder[l] = l;
  }

  GET_LAYER(map, l)->status = MS_ON;

  if (msMaybeAllocateStyle(GET_LAYER(map, l)->class[0], 0)==MS_FAILURE) return MS_FAILURE;
  GET_LAYER(map, l)->class[0]->styles[0]->symbol = s;
  GET_LAYER(map, l)->class[0]->styles[0]->color.pen = -1;
  GET_LAYER(map, l)->class[0]->label.force = MS_TRUE;
  GET_LAYER(map, l)->class[0]->label.size = MS_MEDIUM; /* must set a size to have a valid label definition */
  GET_LAYER(map, l)->class[0]->label.priority = MS_MAX_LABEL_PRIORITY;

  if(map->legend.postlabelcache) /* add it directly to the image */
    msDrawMarkerSymbol(&map->symbolset, img, &point, GET_LAYER(map, l)->class[0]->styles[0], 1.0);
  else
    msAddLabel(map, l, 0, -1, -1, &point, NULL, " ", 1.0, NULL);

  /* Mark layer as deleted so that it doesn't interfere with html legends or */
  /* with saving maps */
  GET_LAYER(map, l)->status = MS_DELETE;

  return(0);
}




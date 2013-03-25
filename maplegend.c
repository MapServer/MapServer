/******************************************************************************
 * $Id$
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



#define PSF .8
#define VMARGIN 5 /* margin at top and bottom of legend graphic */
#define HMARGIN 5 /* margin at left and right of legend graphic */

/*
 * generic function for drawing a legend icon. (added for bug #2348)
 * renderer specific drawing functions shouldn't be called directly, but through
 * this function
 */
int msDrawLegendIcon(mapObj *map, layerObj *lp, classObj *theclass,
                     int width, int height, imageObj *image, int dstX, int dstY)
{
  int i, type, hasmarkersymbol;
  double offset;
  shapeObj box, zigzag;
  pointObj marker;
  char szPath[MS_MAXPATHLEN];
  styleObj outline_style;
  imageObj *image_draw = image;
  int originalopacity = lp->opacity;
  rendererVTableObj *renderer;
  outputFormatObj *transFormat = NULL, *altFormat=NULL;
  const char *alternativeFormatString = NULL;

  if(!MS_RENDERER_PLUGIN(image->format)) {
    msSetError(MS_MISCERR,"unsupported image format","msDrawLegendIcon()");
    return MS_FAILURE;
  }

  alternativeFormatString = msLayerGetProcessingKey(lp, "RENDERER");
  if (MS_RENDERER_PLUGIN(image_draw->format) && alternativeFormatString!=NULL &&
      (altFormat=  msSelectOutputFormat(map, alternativeFormatString))) {
    msInitializeRendererVTable(altFormat);

    image_draw = msImageCreate(image->width, image->height,
                               altFormat, image->imagepath, image->imageurl, map->resolution, map->defresolution, &map->imagecolor);
    renderer = MS_IMAGE_RENDERER(image_draw);
  } else {
    renderer = MS_IMAGE_RENDERER(image_draw);
    if (lp->opacity > 0 && lp->opacity < 100) {
      if (!renderer->supports_transparent_layers) {
        image_draw = msImageCreate(image->width, image->height,
                                   image->format, image->imagepath, image->imageurl, map->resolution, map->defresolution, NULL);
        if (!image_draw) {
          msSetError(MS_MISCERR, "Unable to initialize temporary transparent image.",
                     "msDrawLegendIcon()");
          return (MS_FAILURE);
        }
        /* set opacity to full, as the renderer should be rendering a fully opaque image */
        lp->opacity=100;
      }
    }
  }


  if(renderer->supports_clipping && MS_VALID_COLOR(map->legend.outlinecolor)) {
    /* keep GD specific code here for now as it supports clipping */
    rectObj clip;
    clip.maxx = dstX + width - 1;
    clip.maxy = dstY + height -1;
    clip.minx = dstX;
    clip.miny = dstY;
    renderer->setClip(image_draw,clip);
  }

  /* initialize the box used for polygons and for outlines */
  box.line = (lineObj *)msSmallMalloc(sizeof(lineObj));
  box.numlines = 1;
  box.line[0].point = (pointObj *)msSmallMalloc(sizeof(pointObj)*5);
  box.line[0].numpoints = 5;

  box.line[0].point[0].x = dstX + 0.5;
  box.line[0].point[0].y = dstY + 0.5;
  box.line[0].point[1].x = dstX + width - 0.5;
  box.line[0].point[1].y = dstY + 0.5;
  box.line[0].point[2].x = dstX + width - 0.5;
  box.line[0].point[2].y = dstY + height - 0.5;
  box.line[0].point[3].x = dstX + 0.5;
  box.line[0].point[3].y = dstY + height - 0.5;
  box.line[0].point[4].x = box.line[0].point[0].x;
  box.line[0].point[4].y = box.line[0].point[0].y;
  box.line[0].numpoints = 5;

  /* if the class has a keyimage, treat it as a point layer
   * (the keyimage will be treated there) */
  if(theclass->keyimage != NULL) {
    type = MS_LAYER_POINT;
  } else {
    /* some polygon layers may be better drawn using zigzag if there is no fill */
    type = lp->type;
    if(type == MS_LAYER_POLYGON) {
      type = MS_LAYER_LINE;
      for(i=0; i<theclass->numstyles; i++) {
        if(MS_VALID_COLOR(theclass->styles[i]->color)) { /* there is a fill */
          type = MS_LAYER_POLYGON;
          break;
        }
      }
    }
  }

  /*
  ** now draw the appropriate color/symbol/size combination
  */
  switch(type) {
    case MS_LAYER_ANNOTATION:
      marker.x = dstX + MS_NINT(width / 2.0);
      marker.y = dstY + MS_NINT(height / 2.0);
      hasmarkersymbol = 0;
      for(i=0; i<theclass->numstyles; i++) {
          if (theclass->styles[i]->symbol < map->symbolset.numsymbols && theclass->styles[i]->symbol > 0) {
             hasmarkersymbol = 1;
             break;
          }
      }
      if (hasmarkersymbol) {
        for(i=0; i<theclass->numstyles; i++)
          msDrawMarkerSymbol(&map->symbolset, image_draw, &marker, theclass->styles[i], lp->scalefactor);
      } else if (theclass->labels && theclass->numlabels > 0) {
        labelObj *label = theclass->labels[0]; /* use the first label definition */
        double lsize = label->size;
        double langle = label->angle;
        int lpos = label->position;
        int loffsetx = label->offsetx;
        int loffsety = label->offsety;

        label->offsetx = 0;
        label->offsety = 0;
        label->angle = 0;
        label->position = MS_CC;
        if (label->type == MS_TRUETYPE) label->size = height;
        msDrawLabel(map, image_draw, marker, (char*)"Az", label,1.0);

        label->size = lsize;
        label->position = lpos;
        label->angle = langle;
        label->offsetx = loffsetx;
        label->offsety = loffsety;
      }
      break;
    case MS_LAYER_POINT:
      marker.x = dstX + MS_NINT(width / 2.0);
      marker.y = dstY + MS_NINT(height / 2.0);
      if(theclass->keyimage != NULL) {
        int symbolNum;
        styleObj imgStyle;
        symbolObj *symbol=NULL;
        symbolNum = msAddImageSymbol(&(map->symbolset), msBuildPath(szPath, map->mappath, theclass->keyimage));
        if(symbolNum == -1) {
          msSetError(MS_GDERR, "Failed to open legend key image", "msCreateLegendIcon()");
          return(MS_FAILURE);
        }

        symbol = map->symbolset.symbol[symbolNum];

        initStyle(&imgStyle);
        /*set size so that symbol will be scaled properly #3296*/
        if (width/symbol->sizex < height/symbol->sizey)
          imgStyle.size = symbol->sizey*(width/symbol->sizex);
        else
          imgStyle.size = symbol->sizey*(height/symbol->sizey);

        if (imgStyle.size > imgStyle.maxsize)
          imgStyle.maxsize = imgStyle.size;

        imgStyle.symbol = symbolNum;
        msDrawMarkerSymbol(&map->symbolset,image_draw,&marker,&imgStyle,lp->scalefactor);
        /* TO DO: we may want to handle this differently depending on the relative size of the keyimage */
      } else {
        for(i=0; i<theclass->numstyles; i++)
          msDrawMarkerSymbol(&map->symbolset, image_draw, &marker, theclass->styles[i], lp->scalefactor);
      }
      break;
    case MS_LAYER_LINE:
      offset = 1;
      /* To set the offset, we only check the size/width parameter of the first style */
      if (theclass->numstyles > 0) {
        if (theclass->styles[0]->symbol > 0 && theclass->styles[0]->symbol < map->symbolset.numsymbols && 
              map->symbolset.symbol[theclass->styles[0]->symbol]->type != MS_SYMBOL_SIMPLE)
            offset = theclass->styles[0]->size/2;
        else
            offset = theclass->styles[0]->width/2;
      }
      zigzag.line = (lineObj *)msSmallMalloc(sizeof(lineObj));
      zigzag.numlines = 1;
      zigzag.line[0].point = (pointObj *)msSmallMalloc(sizeof(pointObj)*4);
      zigzag.line[0].numpoints = 4;

      zigzag.line[0].point[0].x = dstX + offset;
      zigzag.line[0].point[0].y = dstY + height - offset;
      zigzag.line[0].point[1].x = dstX + MS_NINT(width / 3.0) - 1;
      zigzag.line[0].point[1].y = dstY + offset;
      zigzag.line[0].point[2].x = dstX + MS_NINT(2.0 * width / 3.0) - 1;
      zigzag.line[0].point[2].y = dstY + height - offset;
      zigzag.line[0].point[3].x = dstX + width - offset;
      zigzag.line[0].point[3].y = dstY + offset;

      for(i=0; i<theclass->numstyles; i++)
        msDrawLineSymbol(&map->symbolset, image_draw, &zigzag, theclass->styles[i], lp->scalefactor);

      free(zigzag.line[0].point);
      free(zigzag.line);
      break;
    case MS_LAYER_CIRCLE:
    case MS_LAYER_RASTER:
    case MS_LAYER_CHART:
    case MS_LAYER_POLYGON:
      for(i=0; i<theclass->numstyles; i++)
        msDrawShadeSymbol(&map->symbolset, image_draw, &box, theclass->styles[i], lp->scalefactor);
      break;
    default:
      return MS_FAILURE;
      break;
  } /* end symbol drawing */

  /* handle an outline if necessary */
  if(MS_VALID_COLOR(map->legend.outlinecolor)) {
    initStyle(&outline_style);
    outline_style.color = map->legend.outlinecolor;
    msDrawLineSymbol(&map->symbolset, image_draw, &box, &outline_style, 1.0);
    /* reset clipping rectangle */
    if(renderer->supports_clipping)
      renderer->resetClip(image_draw);
  }

  if (altFormat) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image);
    rendererVTableObj *altrenderer = MS_IMAGE_RENDERER(image_draw);
    rasterBufferObj rb;
    memset(&rb,0,sizeof(rasterBufferObj));

    altrenderer->getRasterBufferHandle(image_draw,&rb);
    renderer->mergeRasterBuffer(image,&rb,lp->opacity*0.01,0,0,0,0,rb.width,rb.height);
    /*
     * hack to work around bug #3834: if we have use an alternate renderer, the symbolset may contain
     * symbols that reference it. We want to remove those references before the altFormat is destroyed
     * to avoid a segfault and/or a leak, and so the the main renderer doesn't pick the cache up thinking
     * it's for him.
     */
    for(i=0; i<map->symbolset.numsymbols; i++) {
      if (map->symbolset.symbol[i]!=NULL) {
        symbolObj *s = map->symbolset.symbol[i];
        if(s->renderer == altrenderer) {
          altrenderer->freeSymbol(s);
          s->renderer = NULL;
        }
      }
    }
    msFreeImage(image_draw);

  } else if(image != image_draw) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image_draw);
    rasterBufferObj rb;
    memset(&rb,0,sizeof(rasterBufferObj));

    lp->opacity = originalopacity;

    renderer->getRasterBufferHandle(image_draw,&rb);
    renderer->mergeRasterBuffer(image,&rb,lp->opacity*0.01,0,0,0,0,rb.width,rb.height);
    msFreeImage(image_draw);

    /* deref and possibly free temporary transparent output format.  */
    msApplyOutputFormat( &transFormat, NULL, MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE );

  }

  free(box.line[0].point);
  free(box.line);

  return MS_SUCCESS;
}


imageObj *msCreateLegendIcon(mapObj* map, layerObj* lp, classObj* class, int width, int height)
{
  imageObj *image;
  outputFormatObj *format = NULL;
  int i = 0;

  rendererVTableObj *renderer = MS_MAP_RENDERER(map);

  if( !renderer ) {
    msSetError(MS_MISCERR, "invalid map outputformat", "msCreateLegendIcon()");
    return(NULL);
  }

  /* ensure we have an image format representing the options for the legend */
  msApplyOutputFormat(&format, map->outputformat, map->legend.transparent, map->legend.interlace, MS_NOOVERRIDE);

  image = msImageCreate(width,height,format,map->web.imagepath, map->web.imageurl,
                        map->resolution, map->defresolution, &(map->legend.imagecolor));

  /* drop this reference to output format */
  msApplyOutputFormat( &format, NULL, MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE );

  if(image == NULL) {
    msSetError(MS_GDERR, "Unable to initialize image.","msCreateLegendIcon()");
    return(NULL);
  }

  /* Call drawLegendIcon with destination (0, 0) */
  /* Return an empty image if lp==NULL || class=NULL  */
  /* (If class is NULL draw the legend for all classes. Modifications done */
  /* Fev 2004 by AY) */
  if (lp) {
#ifdef USE_GD
    msClearLayerPenValues(lp); /* just in case the mapfile has already been processed */
#endif
    if (class) {
      msDrawLegendIcon(map, lp, class, width, height, image, 0, 0);
    } else {
      for (i=0; i<lp->numclasses; i++) {
        msDrawLegendIcon(map, lp, lp->class[i], width, height, image, 0, 0);
      }
    }
  }
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
int msLegendCalcSize(mapObj *map, int scale_independent, int *size_x, int *size_y,
                     int *layer_index, int num_layers)
{
  int i, j;
  int status, maxwidth=0, nLegendItems=0;
  char *text, *transformedText; /* legend text before/after applying wrapping, encoding if necessary */
  layerObj *lp;
  rectObj rect;
  int current_layers=0;

  /* reset sizes */
  *size_x = 0;
  *size_y = 0;

  /* enable scale-dependent calculations */
  if(!scale_independent) {
    map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
    status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scaledenom);
    if(status != MS_SUCCESS) return MS_FAILURE;
  }

  /*
   * step through all map classes, and for each one that will be displayed
   * calculate the label size
   */
  if (layer_index != NULL && num_layers >0)
    current_layers  = num_layers;
  else
    current_layers = map->numlayers;

  for(i=0; i< current_layers; i++) {

    if (layer_index != NULL && num_layers > 0)
      lp = GET_LAYER(map, layer_index[i]);
    else
      lp = (GET_LAYER(map, map->layerorder[i]));

    if((lp->status == MS_OFF && (layer_index == NULL || num_layers <= 0)) || (lp->type == MS_LAYER_QUERY)) /* skip it */
      continue;

    if(!scale_independent && map->scaledenom > 0) {
      if((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom)) continue;
      if((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom)) continue;
    }

    for(j=lp->numclasses-1; j>=0; j--) {
      text = lp->class[j]->title?lp->class[j]->title:lp->class[j]->name; /* point to the right legend text, title takes precedence */
      if(!text) continue; /* skip it */

      /* skip the class if the classgroup is defined */
      if(lp->classgroup && (lp->class[j]->group == NULL || strcasecmp(lp->class[j]->group, lp->classgroup) != 0))
        continue;

      /* verify class scale */
      if(!scale_independent && map->scaledenom > 0) {
        if((lp->class[j]->maxscaledenom > 0) && (map->scaledenom > lp->class[j]->maxscaledenom)) continue;
        if((lp->class[j]->minscaledenom > 0) && (map->scaledenom <= lp->class[j]->minscaledenom)) continue;
      }

      /*
       * apply encoding and line wrapping to the legend label if requested
       * this is done conditionally as the text transformation function
       * does some memory allocations that can be avoided in most cases.
       * the transformed text must be freed once finished, this must be done
       * conditionnally by testing if the transformed text pointer is the
       * same as the class name pointer
       */
      if(map->legend.label.encoding || map->legend.label.wrap)
        transformedText = msTransformLabelText(map,&map->legend.label, text);
      else
        transformedText = msStrdup(text);

      if(transformedText == NULL || msGetLabelSize(map, &map->legend.label, transformedText, map->legend.label.size, &rect, NULL) != MS_SUCCESS) { /* something bad happened */
        if(transformedText) msFree(transformedText);
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

  if(*size_y <=0 ||  *size_x <=0)
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
  int i,j; /* loop counters */
  pointObj pnt;
  int size_x, size_y=0;
  layerObj *lp;
  rectObj rect;
  imageObj *image = NULL;
  outputFormatObj *format = NULL;
  char *text;

  struct legend_struct {
    int height;
    char *transformedText;
    layerObj *layer;
    classObj *theclass;
    struct legend_struct* pred;
  };
  typedef struct legend_struct legendlabel;
  legendlabel *head=NULL,*cur=NULL;

  if(!MS_RENDERER_PLUGIN(map->outputformat)) {
    msSetError(MS_MISCERR,"unsupported output format","msDrawLegend()");
    return NULL;
  }
  if(msValidateContexts(map) != MS_SUCCESS) return NULL; /* make sure there are no recursive REQUIRES or LABELREQUIRES expressions */
  if(msLegendCalcSize(map, scale_independent, &size_x, &size_y, NULL, 0) != MS_SUCCESS) return NULL;

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

    if(!scale_independent && lp->maxscaledenom <=0 && lp->minscaledenom <=0) {
      if((lp->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > lp->maxgeowidth)) continue;
      if((lp->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < lp->mingeowidth)) continue;
    }

    for(j=lp->numclasses-1; j>=0; j--) {
      text = lp->class[j]->title?lp->class[j]->title:lp->class[j]->name; /* point to the right legend text, title takes precedence */
      if(!text) continue; /* skip it */

      /* skip the class if the classgroup is defined */
      if(lp->classgroup && (lp->class[j]->group == NULL || strcasecmp(lp->class[j]->group, lp->classgroup) != 0))
        continue;

      if(!scale_independent && map->scaledenom > 0) {  /* verify class scale here */
        if((lp->class[j]->maxscaledenom > 0) && (map->scaledenom > lp->class[j]->maxscaledenom)) continue;
        if((lp->class[j]->minscaledenom > 0) && (map->scaledenom <= lp->class[j]->minscaledenom)) continue;
      }

      cur = (legendlabel*) msSmallMalloc(sizeof(legendlabel));

      /*
       * apply encoding and line wrapping to the legend label if requested
       * this is done conditionnally as the text transformation function
       * does some memory allocations that can be avoided in most cases.
       * the transformed text must be freed once finished, this must be done
       * conditionally by testing if the transformed text pointer is the
       * same as the class name pointer
       */
      if(map->legend.label.encoding || map->legend.label.wrap)
        cur->transformedText = msTransformLabelText(map,&map->legend.label,text);
      else
        cur->transformedText = msStrdup(text); /* so we can always do msFree() when cleaning up */

      cur->theclass = lp->class[j];
      cur->layer = lp;
      cur->pred = head;
      head = cur;

      if(cur->transformedText==NULL ||
          msGetLabelSize(map, &map->legend.label, cur->transformedText, map->legend.label.size, &rect, NULL) != MS_SUCCESS) { /* something bad happened, free allocated mem */
        while(cur) {
          free(cur->transformedText);
          head = cur;
          cur = cur->pred;
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
  image = msImageCreate(size_x, size_y, format, map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution, &map->legend.imagecolor);
  if(!image) {
    msSetError(MS_MISCERR, "Unable to initialize image.", "msDrawLegend()");
    return NULL;
  }
  /* image = renderer->createImage(size_x,size_y,format,&(map->legend.imagecolor)); */

  /* drop this reference to output format */
  msApplyOutputFormat(&format, NULL, MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE);

#ifdef USE_GD
  msClearPenValues(map); /* just in case the mapfile has already been processed */
#endif
  pnt.y = VMARGIN;
  pnt.x = HMARGIN + map->legend.keysizex + map->legend.keyspacingx;

  while(cur) { /* cur initially points on the last legend item, i.e. the one that should be at the top */
    int number_of_newlines=0, offset=0;

    /* set the scale factor so that scale dependant symbols are drawn in the legend with their default size */
    if(cur->layer->sizeunits != MS_PIXELS) {
      map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
      cur->layer->scalefactor = (msInchesPerUnit(cur->layer->sizeunits,0)/msInchesPerUnit(map->units,0)) / map->cellsize;
    }
    if(msDrawLegendIcon(map, cur->layer, cur->theclass,  map->legend.keysizex,  map->legend.keysizey, image, HMARGIN, (int) pnt.y) != MS_SUCCESS)
      return NULL;

    /*
     * adjust the baseline for multiline truetype labels. the label point is the bottom left
     * corner of the *first* line, which we do not know exactly as we only have the
     * bounding box of the whole label. current approach is to suppose that all lines
     * mostly have the same height, and offset the starting point by the mean hight of
     * one line. This isn't perfect but still much better than the previous approach
     */
    if(map->legend.label.type==MS_TRUETYPE && (number_of_newlines=msCountChars(cur->transformedText,'\n')) > 0) {
      offset=cur->height/(number_of_newlines+1);
      pnt.y += offset;
    } else
      pnt.y += cur->height;

    /* TODO: note tbonfort: if this todo concerned treating the individual heights of the legend labels, then this is now done */

    msDrawLabel(map, image, pnt, cur->transformedText, &(map->legend.label), 1.0);
    if(offset) {
      /* if we had multiple lines, adjust the current position so it points to the bottom of the current label */
      pnt.y += cur->height-offset;
    }
    pnt.y += map->legend.keyspacingy; /* bump y for next label */

    /* clean up */
    free(cur->transformedText);
    head = cur;
    cur = cur->pred;
    free(head);
  } /* next legend */

  return(image);
}

/* TODO */
int msEmbedLegend(mapObj *map, imageObj *img)
{
  int s,l;
  pointObj point;
  imageObj *image = NULL;
  symbolObj *legendSymbol;

  rendererVTableObj *renderer;

  if(!MS_RENDERER_PLUGIN(map->outputformat) || !MS_MAP_RENDERER(map)->supports_pixel_buffer) {
    msSetError(MS_MISCERR, "unsupported output format", "msEmbedLegend()");
    return MS_FAILURE;
  }
  renderer = MS_MAP_RENDERER(map);

  s = msGetSymbolIndex(&(map->symbolset), "legend", MS_FALSE);
  if(s != -1)
    msRemoveSymbol(&(map->symbolset), s); /* solves some caching issues in AGG with long-running processes */

  if(msGrowSymbolSet(&map->symbolset) == NULL)
    return -1;
  s = map->symbolset.numsymbols;
  legendSymbol = map->symbolset.symbol[s];
  map->symbolset.numsymbols++;
  initSymbol(legendSymbol);

  /* render the legend. */
  image = msDrawLegend(map, MS_FALSE);
  if( image == NULL ) return -1;

  /* copy renderered legend image into symbol */
  legendSymbol->pixmap_buffer = calloc(1,sizeof(rasterBufferObj));
  MS_CHECK_ALLOC(legendSymbol->pixmap_buffer, sizeof(rasterBufferObj), MS_FAILURE);

  if(MS_SUCCESS != renderer->getRasterBufferCopy(image,legendSymbol->pixmap_buffer))
    return MS_FAILURE;
  legendSymbol->renderer = renderer;

  msFreeImage( image );

  if(!legendSymbol->pixmap_buffer) return(-1); /* something went wrong creating scalebar */

  legendSymbol->type = MS_SYMBOL_PIXMAP; /* intialize a few things */
  legendSymbol->name = msStrdup("legend");
  legendSymbol->sizex = legendSymbol->pixmap_buffer->width;
  legendSymbol->sizey = legendSymbol->pixmap_buffer->height;

  /* I'm not too sure this test is sufficient ... NFW. */
  /* if(map->legend.transparent == MS_ON) */
  /*  gdImageColorTransparent(legendSymbol->img_deprecated, 0); */

  switch(map->legend.position) {
    case(MS_LL):
      point.x = MS_NINT(legendSymbol->sizex/2.0);
      point.y = map->height - MS_NINT(legendSymbol->sizey/2.0);
      break;
    case(MS_LR):
      point.x = map->width - MS_NINT(legendSymbol->sizex/2.0);
      point.y = map->height - MS_NINT(legendSymbol->sizey/2.0);
      break;
    case(MS_LC):
      point.x = MS_NINT(map->width/2.0);
      point.y = map->height - MS_NINT(legendSymbol->sizey/2.0);
      break;
    case(MS_UR):
      point.x = map->width - MS_NINT(legendSymbol->sizex/2.0);
      point.y = MS_NINT(legendSymbol->sizey/2.0);
      break;
    case(MS_UL):
      point.x = MS_NINT(legendSymbol->sizex/2.0);
      point.y = MS_NINT(legendSymbol->sizey/2.0);
      break;
    case(MS_UC):
      point.x = MS_NINT(map->width/2.0);
      point.y = MS_NINT(legendSymbol->sizey/2.0);
      break;
  }

  l = msGetLayerIndex(map, "__embed__legend");
  if(l == -1) {
    if(msGrowMapLayers(map) == NULL)
      return(-1);
    l = map->numlayers;
    map->numlayers++;
    if(initLayer((GET_LAYER(map, l)), map) == -1) return(-1);
    GET_LAYER(map, l)->name = msStrdup("__embed__legend");
    GET_LAYER(map, l)->type = MS_LAYER_POINT;

    if(msGrowLayerClasses( GET_LAYER(map, l) ) == NULL)
      return(-1);
    if(initClass(GET_LAYER(map, l)->class[0]) == -1) return(-1);
    GET_LAYER(map, l)->numclasses = 1; /* so we make sure to free it */

    /* update the layer order list with the layer's index. */
    map->layerorder[l] = l;
  }

  GET_LAYER(map, l)->status = MS_ON;

  if(map->legend.postlabelcache) { /* add it directly to the image */
    if(msMaybeAllocateClassStyle(GET_LAYER(map, l)->class[0], 0)==MS_FAILURE) return MS_FAILURE;
    GET_LAYER(map, l)->class[0]->styles[0]->symbol = s;
    msDrawMarkerSymbol(&map->symbolset, img, &point, GET_LAYER(map, l)->class[0]->styles[0], 1.0);
  } else {
    if(!GET_LAYER(map, l)->class[0]->labels) {
      if(msGrowClassLabels(GET_LAYER(map, l)->class[0]) == NULL) return MS_FAILURE;
      initLabel(GET_LAYER(map, l)->class[0]->labels[0]);
      GET_LAYER(map, l)->class[0]->numlabels = 1;
      GET_LAYER(map, l)->class[0]->labels[0]->force = MS_TRUE;
      GET_LAYER(map, l)->class[0]->labels[0]->size = MS_MEDIUM; /* must set a size to have a valid label definition */
      GET_LAYER(map, l)->class[0]->labels[0]->priority = MS_MAX_LABEL_PRIORITY;
      GET_LAYER(map, l)->class[0]->labels[0]->annotext = NULL;
    }
    if(GET_LAYER(map, l)->class[0]->labels[0]->numstyles == 0) {
      if(msGrowLabelStyles(GET_LAYER(map,l)->class[0]->labels[0]) == NULL)
        return(MS_FAILURE);
      GET_LAYER(map,l)->class[0]->labels[0]->numstyles = 1;
      initStyle(GET_LAYER(map,l)->class[0]->labels[0]->styles[0]);
      GET_LAYER(map,l)->class[0]->labels[0]->styles[0]->_geomtransform.type = MS_GEOMTRANSFORM_LABELPOINT;
    }
    GET_LAYER(map,l)->class[0]->labels[0]->styles[0]->symbol = s;
    msAddLabel(map, GET_LAYER(map, l)->class[0]->labels[0], l, 0, NULL, &point, NULL, -1);
  }

  /* Mark layer as deleted so that it doesn't interfere with html legends or with saving maps */
  GET_LAYER(map, l)->status = MS_DELETE;

  return(0);
}

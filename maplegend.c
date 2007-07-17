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

#include "map.h"

MS_CVSID("$Id$")

#define PSF .8
#define VMARGIN 5 /* margin at top and bottom of legend graphic */
#define HMARGIN 5 /* margin at left and right of legend graphic */

int msDrawLegendIcon(mapObj *map, layerObj *lp, classObj *class, int width, int height, gdImagePtr img, int dstX, int dstY)
{
  int i, type;
  shapeObj box, zigzag;
  pointObj marker;
  char szPath[MS_MAXPATHLEN];
  imageObj *image = NULL;
  styleObj outline_style;

  /* if drawing an outline (below) we need to set clipping to keep symbols withing the outline */
  if(MS_VALID_COLOR(map->legend.outlinecolor))
    gdImageSetClip(img, dstX, dstY, dstX + width - 1, dstY + height - 1);

  /* initialize the box used for polygons and for outlines */
  box.line = (lineObj *)malloc(sizeof(lineObj));
  box.numlines = 1;
  box.line[0].point = (pointObj *)malloc(sizeof(pointObj)*5);
  box.line[0].numpoints = 5;

  box.line[0].point[0].x = dstX;
  box.line[0].point[0].y = dstY;
  box.line[0].point[1].x = dstX + width - 1;
  box.line[0].point[1].y = dstY;
  box.line[0].point[2].x = dstX + width - 1;
  box.line[0].point[2].y = dstY + height - 1;
  box.line[0].point[3].x = dstX;
  box.line[0].point[3].y = dstY + height - 1;
  box.line[0].point[4].x = box.line[0].point[0].x;
  box.line[0].point[4].y = box.line[0].point[0].y;
  box.line[0].numpoints = 5;
    
  /* if the class has a keyimage then load it, scale it and we're done */
  if(class->keyimage != NULL) {
    image = msImageLoadGD(msBuildPath(szPath, map->mappath, class->keyimage));
    if(!image) return(MS_FAILURE);

    /* TO DO: we may want to handle this differently depending on the relative size of the keyimage */
    gdImageCopyResampled(img, image->img.gd, dstX, dstY, 0, 0, width, height, image->img.gd->sx, image->img.gd->sy);
  } else {        
    /* some polygon layers may be better drawn using zigzag if there is no fill */
    type = lp->type;
    if(type == MS_LAYER_POLYGON) {
      type = MS_LAYER_LINE;
      for(i=0; i<class->numstyles; i++) {
       if(MS_VALID_COLOR(class->styles[i]->color)) { /* there is a fill */
	  type = MS_LAYER_POLYGON;
	  break;
        }
      }
    }

    /* 
    ** now draw the appropriate color/symbol/size combination 
    */     

    /* Bug 490 - switch alpha blending on for a layer that requires it */
    if(lp->opacity == MS_GD_ALPHA)
      gdImageAlphaBlending(img, 1);

    switch(type) {
    case MS_LAYER_ANNOTATION:
    case MS_LAYER_POINT:
      marker.x = dstX + MS_NINT(width / 2.0);
      marker.y = dstY + MS_NINT(height / 2.0);

      for(i=0; i<class->numstyles; i++)
        msDrawMarkerSymbolGD(&map->symbolset, img, &marker, class->styles[i], lp->scalefactor);          
      break;
    case MS_LAYER_LINE:
      zigzag.line = (lineObj *)malloc(sizeof(lineObj));
      zigzag.numlines = 1;
      zigzag.line[0].point = (pointObj *)malloc(sizeof(pointObj)*4);
      zigzag.line[0].numpoints = 4;

      zigzag.line[0].point[0].x = dstX;
      zigzag.line[0].point[0].y = dstY + height - 1;
      zigzag.line[0].point[1].x = dstX + MS_NINT(width / 3.0) - 1;
      zigzag.line[0].point[1].y = dstY;
      zigzag.line[0].point[2].x = dstX + MS_NINT(2.0 * width / 3.0) - 1;
      zigzag.line[0].point[2].y = dstY + height - 1;
      zigzag.line[0].point[3].x = dstX + width - 1;
      zigzag.line[0].point[3].y = dstY;
      zigzag.line[0].numpoints = 4;

      for(i=0; i<class->numstyles; i++)
        msDrawLineSymbolGD(&map->symbolset, img, &zigzag, class->styles[i], lp->scalefactor); 

      free(zigzag.line[0].point);
      free(zigzag.line);	
      break;
    case MS_LAYER_CIRCLE:
    case MS_LAYER_RASTER:
    case MS_LAYER_CHART:
    case MS_LAYER_POLYGON:
      for(i=0; i<class->numstyles; i++)     
        msDrawShadeSymbolGD(&map->symbolset, img, &box, class->styles[i], lp->scalefactor);
      break;
    default:
      return MS_FAILURE;
      break;
    } /* end symbol drawing */
  }   

  /* handle an outline if necessary */
  if(MS_VALID_COLOR(map->legend.outlinecolor)) {
    initStyle(&outline_style);
    outline_style.color = map->legend.outlinecolor;
    msDrawLineSymbolGD(&map->symbolset, img, &box, &outline_style, 1.0);    
    gdImageSetClip(img, 0, 0, img->sx - 1, img->sy - 1); /* undo any clipping settings */
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

  if(!map->outputformat || (!MS_RENDERER_GD(map->outputformat) && !MS_RENDERER_AGG(map->outputformat) )) {
    msSetError(MS_GDERR, "Map outputformat must be set to a GD format!", "msCreateLegendIcon()");
    return(NULL);
  }

  /* ensure we have an image format representing the options for the legend */
  msApplyOutputFormat(&format, map->outputformat, map->legend.transparent, map->legend.interlace, MS_NOOVERRIDE);

  /* create image */
  image = msImageCreateGD(width, height, map->outputformat, map->web.imagepath, map->web.imageurl);

  /* drop this reference to output format */
  msApplyOutputFormat( &format, NULL, MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE );

  if(image == NULL) {
    msSetError(MS_GDERR, "Unable to initialize image.","msCreateLegendIcon()");
    return(NULL);
  }

  /* allocate the background color */
  msImageInitGD( image, &(map->legend.imagecolor));

  /* Call drawLegendIcon with destination (0, 0) */
  /* Return an empty image if lp==NULL || class=NULL  */
  /* (If class is NULL draw the legend for all classes. Modifications done */
  /* Fev 2004 by AY) */
  if (lp) {
    msClearLayerPenValues(lp); /* just in case the mapfile has already been processed */
    if (class) {
      msDrawLegendIcon(map, lp, class, width, height, image->img.gd, 0, 0);
    } else {
      for (i=0; i<lp->numclasses; i++) {
        msDrawLegendIcon(map, lp, lp->class[i], width, height, image->img.gd, 0, 0);
      }
    }
  }

  return image;
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
  int status;

  gdImagePtr img; /* image data structure */
  int i,j; /* loop counters */
  pointObj pnt;
  int size_x, size_y;
  layerObj *lp;  
  int maxwidth=0, maxheight=0, n=0;
  int *heights;
  rectObj rect;
  imageObj *image = NULL;
  outputFormatObj *format = NULL;

  if (!scale_independent) {
    map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
    status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scale);
    if(status != MS_SUCCESS) return(NULL);
  }

  if(msValidateContexts(map) != MS_SUCCESS) return NULL; /* make sure there are no recursive REQUIRES or LABELREQUIRES expressions */

  /*
  ** allocate heights array
  */
  for(i=0; i<map->numlayers; i++) {
    lp = (GET_LAYER(map, map->layerorder[i]));

    if((lp->status == MS_OFF) || (lp->type == MS_LAYER_QUERY)) /* skip it */
      continue;

    for(j=0;j<lp->numclasses;j++) {
      if(!lp->class[j]->name) continue; /* skip it */
      n++;
    }
  }

  if((heights = (int *)malloc(sizeof(int)*n)) == NULL) {
    msSetError(MS_MEMERR, "Error allocating heights array.", "msDrawLegend()");
    return(NULL);
  }

  /*
  ** Calculate the optimal image size for the legend
  */
  n=0;
  for(i=0; i<map->numlayers; i++) { /* Need to find the longest legend label string */
    lp = (GET_LAYER(map, map->layerorder[i]));

    if((lp->status == MS_OFF) || (lp->type == MS_LAYER_QUERY)) /* skip it */
      continue;

    if(!scale_independent && map->scale > 0) {
      if((lp->maxscale > 0) && (map->scale > lp->maxscale)) continue;
      if((lp->minscale > 0) && (map->scale <= lp->minscale)) continue;
    }
 
    for(j=0;j<lp->numclasses;j++) {
      if(!lp->class[j]->name)
	continue; /* skip it */

      if(!scale_independent && map->scale > 0) {  /* verify class scale here */
	if((lp->class[j]->maxscale > 0) && (map->scale > lp->class[j]->maxscale))
	  continue;
	if((lp->class[j]->minscale > 0) && (map->scale <= lp->class[j]->minscale))
	  continue;
      }

      if(msGetLabelSize(lp->class[j]->name, &map->legend.label, &rect, &(map->fontset), 1.0, MS_FALSE) != 0)
	return(NULL); /* something bad happened */

      maxheight = MS_MAX(maxheight, MS_NINT(rect.maxy - rect.miny));
      maxwidth = MS_MAX(maxwidth, MS_NINT(rect.maxx - rect.minx));
      heights[n] = MS_NINT(rect.maxy - rect.miny);

      n++;
    }
  }

  size_x = (2*HMARGIN)+(maxwidth)+(map->legend.keyspacingx)+(map->legend.keysizex);
  size_y = (2*VMARGIN) + ((n-1)*map->legend.keyspacingy);
  for(i=0; i<n; i++) {
    heights[i] = MS_MAX(heights[i], maxheight);
    size_y += MS_MAX(heights[i], map->legend.keysizey);
  }

  /* ensure we have an image format representing the options for the legend. */
  msApplyOutputFormat(&format, map->outputformat, map->legend.transparent, map->legend.interlace, MS_NOOVERRIDE);

  /* initialize the legend image */
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
  if(image != NULL)
    msImageInitGD(image, &(map->legend.imagecolor));

  msClearPenValues(map); /* just in case the mapfile has already been processed */

  pnt.y = VMARGIN;
    
  /* for(i=0; i<map->numlayers; i++) { */
  for(i=map->numlayers-1; i>=0; i--) {
    lp = (GET_LAYER(map, map->layerorder[i])); /* for brevity */

    if((lp->numclasses == 0) || (lp->status == MS_OFF) || (lp->type == MS_LAYER_QUERY))
      continue; /* skip this layer */

    if(!scale_independent && map->scale > 0) {
      if((lp->maxscale > 0) && (map->scale > lp->maxscale))
	continue;
      if((lp->minscale > 0) && (map->scale <= lp->minscale))
	continue;

      /* Should we also consider lp->symbolscale? I don't think so. Showing the "standard" size makes the most sense. */
      if(lp->sizeunits != MS_PIXELS)
        lp->scalefactor = (msInchesPerUnit(lp->sizeunits,0)/msInchesPerUnit(map->units,0)) / map->cellsize;
    }

    for(j=0; j<lp->numclasses; j++) { /* always at least 1 class */

      if(!lp->class[j]->name) continue; /* skip it */
     
      if(!scale_independent && map->scale > 0) {  /* verify class scale here */
        if((lp->class[j]->maxscale > 0) && (map->scale > lp->class[j]->maxscale))
          continue;
        if((lp->class[j]->minscale > 0) && (map->scale <= lp->class[j]->minscale))
          continue;
      }
 
      pnt.x = HMARGIN + map->legend.keysizex + map->legend.keyspacingx;
      
      /* TODO */
      if(msDrawLegendIcon(map, lp, lp->class[j],  map->legend.keysizex,  map->legend.keysizey, image->img.gd, HMARGIN, (int) pnt.y) != MS_SUCCESS)
        return NULL;

      pnt.y += MS_MAX(map->legend.keysizey, maxheight);
      /* TODO */
      msDrawLabel(image, pnt, lp->class[j]->name, &(map->legend.label), &map->fontset, 1.0);

      pnt.y += map->legend.keyspacingy; /* bump y for next label */
	
    } /* next label */
  } /* next layer */

  free(heights);
  return(image);
}

/* TODO */
int msEmbedLegend(mapObj *map, gdImagePtr img)
{
  int s,l;
  pointObj point;
  imageObj *image = NULL;

  s = msGetSymbolIndex(&(map->symbolset), "legend", MS_FALSE);
  if(s == -1) {
    if (map->symbolset.symbol[map->symbolset.numsymbols]==NULL) {
      map->symbolset.symbol[map->symbolset.numsymbols]=(symbolObj*)malloc(sizeof(symbolObj));
      if (map->symbolset.symbol[map->symbolset.numsymbols]==NULL) {
        msSetError(MS_MEMERR, "Failed to allocate memory for a symbolObj", "msEmbedLegend()");
        return -1;
      }
    }
    s = map->symbolset.numsymbols;
    map->symbolset.numsymbols++;
    initSymbol(map->symbolset.symbol[s]);
  } else {
    if(map->symbolset.symbol[s]->img) 
      gdImageDestroy(map->symbolset.symbol[s]->img);
  }
  
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
    l = map->numlayers;
    map->numlayers++;
    GET_LAYER(map, l)=(layerObj*)malloc(sizeof(layerObj));
    if (GET_LAYER(map, l)==NULL) {
	msSetError(MS_MISCERR, "Failed to init new layerObj",
		"msEmbedLegend()");
	return(MS_FAILURE);
    }
    if(initLayer((GET_LAYER(map, l)), map) == -1) return(-1);
    GET_LAYER(map, l)->name = strdup("__embed__legend");
    GET_LAYER(map, l)->type = MS_LAYER_ANNOTATION;

    GET_LAYER(map, l)->class[0]=(classObj*)malloc(sizeof(classObj));
    if (GET_LAYER(map, l)->class[0]==NULL) {
	msSetError(MS_MISCERR, "Failed to init new classObj",
		"msEmbedLegend()");
	return(MS_FAILURE);
    }
    if(initClass(GET_LAYER(map, l)->class[0]) == -1) return(-1);
    GET_LAYER(map, l)->numclasses = 1; /* so we make sure to free it */
        
    /* update the layer order list with the layer's index. */
    map->layerorder[l] = l;
  }

  GET_LAYER(map, l)->status = MS_ON;

  if (msMaybeAllocateStyle(GET_LAYER(map, l)->class[0], 0)==MS_FAILURE) return MS_FAILURE;
  GET_LAYER(map, l)->class[0]->numstyles = 1;
  GET_LAYER(map, l)->class[0]->styles[0]->symbol = s;
  GET_LAYER(map, l)->class[0]->styles[0]->color.pen = -1;
  GET_LAYER(map, l)->class[0]->label.force = MS_TRUE;
  GET_LAYER(map, l)->class[0]->label.size = MS_MEDIUM; /* must set a size to have a valid label definition */
  GET_LAYER(map, l)->class[0]->label.priority = MS_MAX_LABEL_PRIORITY;

  if(map->legend.postlabelcache) /* add it directly to the image */
    msDrawMarkerSymbolGD(&map->symbolset, img, &point, GET_LAYER(map, l)->class[0]->styles[0], 1.0);
  else
    msAddLabel(map, l, 0, -1, -1, &point, NULL, " ", 1.0, NULL);

  /* Mark layer as deleted so that it doesn't interfere with html legends or */
  /* with saving maps */
  GET_LAYER(map, l)->status = MS_DELETE;

  return(0);
}




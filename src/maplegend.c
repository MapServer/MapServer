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

static int msDrawGradientSymbol(rendererVTableObj *renderer,
                                imageObj *image_draw, double x_center,
                                double y_center, int width, int height,
                                styleObj *style) {
  unsigned char *r, *g, *b, *a;
  symbolObj symbol;
  rasterBufferObj *rb;
  symbolStyleObj symbolStyle;
  int ret;

  initSymbol(&symbol);
  rb = (rasterBufferObj *)calloc(1, sizeof(rasterBufferObj));
  symbol.pixmap_buffer = rb;
  rb->type = MS_BUFFER_BYTE_RGBA;
  rb->width = width;
  rb->height = height;
  rb->data.rgba.row_step = rb->width * 4;
  rb->data.rgba.pixel_step = 4;
  rb->data.rgba.pixels = (unsigned char *)malloc(sizeof(unsigned char) *
                                                 rb->width * rb->height * 4);
  b = rb->data.rgba.b = &rb->data.rgba.pixels[0];
  g = rb->data.rgba.g = &rb->data.rgba.pixels[1];
  r = rb->data.rgba.r = &rb->data.rgba.pixels[2];
  a = rb->data.rgba.a = &rb->data.rgba.pixels[3];
  for (unsigned j = 0; j < rb->height; j++) {
    for (unsigned i = 0; i < rb->width; i++) {
      msValueToRange(style,
                     style->minvalue + (double)i / rb->width *
                                           (style->maxvalue - style->minvalue),
                     MS_COLORSPACE_RGB);
      b[4 * (j * rb->width + i)] = style->color.blue;
      g[4 * (j * rb->width + i)] = style->color.green;
      r[4 * (j * rb->width + i)] = style->color.red;
      a[4 * (j * rb->width + i)] = style->color.alpha;
    }
  }
  INIT_SYMBOL_STYLE(symbolStyle);
  ret = renderer->renderPixmapSymbol(image_draw, x_center, y_center, &symbol,
                                     &symbolStyle);
  msFreeSymbol(&symbol);
  return ret;
}

/*
 * generic function for drawing a legend icon. (added for bug #2348)
 * renderer specific drawing functions shouldn't be called directly, but through
 * this function
 */
int msDrawLegendIcon(mapObj *map, layerObj *lp, classObj *theclass, int width,
                     int height, imageObj *image, int dstX, int dstY,
                     int scale_independant, class_hittest *hittest) {
  int i, type, hasmarkersymbol, ret = MS_SUCCESS;
  double offset;
  double polygon_contraction =
      0.5; /* used to account for the width of a polygon's outline */
  shapeObj box, zigzag;
  lineObj box_line, zigzag_line;
  pointObj box_point[5], zigzag_point[4];
  pointObj marker;
  char szPath[MS_MAXPATHLEN];
  styleObj outline_style;
  imageObj *image_draw = image;
  rendererVTableObj *renderer;
  outputFormatObj *transFormat = NULL, *altFormat = NULL;
  const char *alternativeFormatString = NULL;

  if (!MS_RENDERER_PLUGIN(image->format)) {
    msSetError(MS_MISCERR, "unsupported image format", "msDrawLegendIcon()");
    return MS_FAILURE;
  }

  alternativeFormatString = msLayerGetProcessingKey(lp, "RENDERER");
  if (MS_RENDERER_PLUGIN(image_draw->format) &&
      alternativeFormatString != NULL &&
      (altFormat = msSelectOutputFormat(map, alternativeFormatString))) {
    msInitializeRendererVTable(altFormat);

    image_draw = msImageCreate(
        image->width, image->height, altFormat, image->imagepath,
        image->imageurl, map->resolution, map->defresolution, &map->imagecolor);
    image_draw->map = map;
    renderer = MS_IMAGE_RENDERER(image_draw);
  } else {
    renderer = MS_IMAGE_RENDERER(image_draw);
    if (lp->compositer && renderer->compositeRasterBuffer) {
      image_draw = msImageCreate(image->width, image->height, image->format,
                                 image->imagepath, image->imageurl,
                                 map->resolution, map->defresolution, NULL);
      if (!image_draw) {
        msSetError(MS_MISCERR,
                   "Unable to initialize temporary transparent image.",
                   "msDrawLegendIcon()");
        return (MS_FAILURE);
      }
      image_draw->map = map;
    }
  }

  if (renderer->supports_clipping && MS_VALID_COLOR(map->legend.outlinecolor)) {
    /* keep GD specific code here for now as it supports clipping */
    rectObj clip;
    clip.maxx = dstX + width - 1;
    clip.maxy = dstY + height - 1;
    clip.minx = dstX;
    clip.miny = dstY;
    renderer->setClip(image_draw, clip);
  }

  /* if the class has a keyimage, treat it as a point layer
   * (the keyimage will be treated there) */
  if (theclass->keyimage != NULL) {
    type = MS_LAYER_POINT;
  } else {
    /* some polygon layers may be better drawn using zigzag if there is no fill
     */
    type = lp->type;
    if (type == MS_LAYER_POLYGON) {
      type = MS_LAYER_LINE;
      for (i = 0; i < theclass->numstyles; i++) {
        if (MS_VALID_COLOR(theclass->styles[i]->color)) { /* there is a fill */
          type = MS_LAYER_POLYGON;
        }
        if (MS_VALID_COLOR(
                theclass->styles[i]->outlinecolor)) { /* there is an outline */
          polygon_contraction =
              MS_MAX(polygon_contraction, theclass->styles[i]->width / 2.0);
        }
      }
    }
  }

  /* initialize the box used for polygons and for outlines */
  msInitShape(&box);
  box.line = &box_line;
  box.numlines = 1;
  box.line[0].point = box_point;
  box.line[0].numpoints = 5;
  box.type = MS_SHAPE_POLYGON;

  box.line[0].point[0].x = dstX + polygon_contraction;
  box.line[0].point[0].y = dstY + polygon_contraction;
  box.line[0].point[1].x = dstX + width - polygon_contraction;
  box.line[0].point[1].y = dstY + polygon_contraction;
  box.line[0].point[2].x = dstX + width - polygon_contraction;
  box.line[0].point[2].y = dstY + height - polygon_contraction;
  box.line[0].point[3].x = dstX + polygon_contraction;
  box.line[0].point[3].y = dstY + height - polygon_contraction;
  box.line[0].point[4].x = box.line[0].point[0].x;
  box.line[0].point[4].y = box.line[0].point[0].y;

  /*
  ** now draw the appropriate color/symbol/size combination
  */

  /* Scalefactor will be infinity when SIZEUNITS is set in LAYER */
  if (lp->sizeunits != MS_PIXELS) {
    lp->scalefactor = 1.0;
  }

  switch (type) {
  case MS_LAYER_POINT:
    marker.x = dstX + MS_NINT(width / 2.0);
    marker.y = dstY + MS_NINT(height / 2.0);
    if (theclass->keyimage != NULL) {
      int symbolNum;
      styleObj imgStyle;
      symbolObj *symbol = NULL;
      symbolNum =
          msAddImageSymbol(&(map->symbolset), msBuildPath(szPath, map->mappath,
                                                          theclass->keyimage));
      if (symbolNum == -1) {
        msSetError(MS_IMGERR, "Failed to open legend key image",
                   "msCreateLegendIcon()");
        return (MS_FAILURE);
      }

      symbol = map->symbolset.symbol[symbolNum];

      initStyle(&imgStyle);
      /*set size so that symbol will be scaled properly #3296*/
      if (width / symbol->sizex < height / symbol->sizey)
        imgStyle.size = symbol->sizey * (width / symbol->sizex);
      else
        imgStyle.size = symbol->sizey * (height / symbol->sizey);

      if (imgStyle.size > imgStyle.maxsize)
        imgStyle.maxsize = imgStyle.size;

      imgStyle.symbol = symbolNum;
      ret = msDrawMarkerSymbol(map, image_draw, &marker, &imgStyle, 1.0);
      if (MS_UNLIKELY(ret == MS_FAILURE))
        goto legend_icon_cleanup;
      /* TO DO: we may want to handle this differently depending on the relative
       * size of the keyimage */
    } else {
      for (i = 0; i < theclass->numstyles; i++) {
        if (!scale_independant && map->scaledenom > 0) {
          styleObj *lp = theclass->styles[i];
          if ((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom))
            continue;
          if ((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom))
            continue;
        }
        if (hittest && hittest->stylehits[i].status == 0)
          continue;
        ret = msDrawMarkerSymbol(map, image_draw, &marker, theclass->styles[i],
                                 lp->scalefactor * image->resolutionfactor);
        if (MS_UNLIKELY(ret == MS_FAILURE))
          goto legend_icon_cleanup;
      }
    }
    break;
  case MS_LAYER_LINE:
    offset = 1;
    /* To set the offset, we only check the size/width parameter of the first
     * style */
    if (theclass->numstyles > 0) {
      if (theclass->styles[0]->symbol > 0 &&
          theclass->styles[0]->symbol < map->symbolset.numsymbols &&
          map->symbolset.symbol[theclass->styles[0]->symbol]->type !=
              MS_SYMBOL_SIMPLE)
        offset = theclass->styles[0]->size / 2;
      else
        offset = theclass->styles[0]->width / 2;
    }
    msInitShape(&zigzag);
    zigzag.line = &zigzag_line;
    zigzag.numlines = 1;
    zigzag.line[0].point = zigzag_point;
    zigzag.line[0].numpoints = 4;
    zigzag.type = MS_SHAPE_LINE;

    zigzag.line[0].point[0].x = dstX + offset;
    zigzag.line[0].point[0].y = dstY + height - offset;
    zigzag.line[0].point[1].x = dstX + MS_NINT(width / 3.0) - 1;
    zigzag.line[0].point[1].y = dstY + offset;
    zigzag.line[0].point[2].x = dstX + MS_NINT(2.0 * width / 3.0) - 1;
    zigzag.line[0].point[2].y = dstY + height - offset;
    zigzag.line[0].point[3].x = dstX + width - offset;
    zigzag.line[0].point[3].y = dstY + offset;

    for (i = 0; i < theclass->numstyles; i++) {
      if (!scale_independant && map->scaledenom > 0) {
        styleObj *lp = theclass->styles[i];
        if ((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom))
          continue;
        if ((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom))
          continue;
      }
      if (hittest && hittest->stylehits[i].status == 0)
        continue;
      if (theclass->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_NONE ||
          theclass->styles[i]->_geomtransform.type ==
              MS_GEOMTRANSFORM_LABELPOINT ||
          theclass->styles[i]->_geomtransform.type ==
              MS_GEOMTRANSFORM_LABELPOLY) {
        if (theclass->styles[i]->outlinewidth > 0) {
          /* Swap the style contents to render the outline first,
           * and then restore the style to render the interior of the line
           */
          msOutlineRenderingPrepareStyle(theclass->styles[i], map, lp, image);
          ret =
              msDrawLineSymbol(map, image_draw, &zigzag, theclass->styles[i],
                               lp->scalefactor * image_draw->resolutionfactor);
          msOutlineRenderingRestoreStyle(theclass->styles[i], map, lp, image);
          if (MS_UNLIKELY(ret == MS_FAILURE))
            goto legend_icon_cleanup;
        }
        ret = msDrawLineSymbol(map, image_draw, &zigzag, theclass->styles[i],
                               lp->scalefactor * image_draw->resolutionfactor);
        if (MS_UNLIKELY(ret == MS_FAILURE))
          goto legend_icon_cleanup;
      } else {
        if (theclass->styles[i]->outlinewidth > 0) {
          /* Swap the style contents to render the outline first,
           * and then restore the style to render the interior of the line
           */
          msOutlineRenderingPrepareStyle(theclass->styles[i], map, lp, image);
          ret = msDrawTransformedShape(
              map, image_draw, &zigzag, theclass->styles[i],
              lp->scalefactor * image_draw->resolutionfactor);
          msOutlineRenderingRestoreStyle(theclass->styles[i], map, lp, image);
          if (MS_UNLIKELY(ret == MS_FAILURE))
            goto legend_icon_cleanup;
        }
        ret = msDrawTransformedShape(
            map, image_draw, &zigzag, theclass->styles[i],
            lp->scalefactor * image_draw->resolutionfactor);
        if (MS_UNLIKELY(ret == MS_FAILURE))
          goto legend_icon_cleanup;
      }
    }

    break;
  case MS_LAYER_CIRCLE:
  case MS_LAYER_RASTER:
  case MS_LAYER_CHART:
  case MS_LAYER_POLYGON:
    for (i = 0; i < theclass->numstyles; i++) {
      if (!scale_independant && map->scaledenom > 0) {
        styleObj *lp = theclass->styles[i];
        if ((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom))
          continue;
        if ((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom))
          continue;
      }
      if (hittest && hittest->stylehits[i].status == 0)
        continue;
      if (theclass->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_NONE ||
          theclass->styles[i]->_geomtransform.type ==
              MS_GEOMTRANSFORM_LABELPOINT ||
          theclass->styles[i]->_geomtransform.type ==
              MS_GEOMTRANSFORM_LABELPOLY) {

        if (MS_VALID_COLOR(theclass->styles[i]->mincolor)) {
          ret = msDrawGradientSymbol(
              renderer, image_draw, dstX + width / 2., dstY + height / 2.0,
              width - 2 * polygon_contraction, height - 2 * polygon_contraction,
              theclass->styles[i]);
        } else {
          ret =
              msDrawShadeSymbol(map, image_draw, &box, theclass->styles[i],
                                lp->scalefactor * image_draw->resolutionfactor);
        }
        if (MS_UNLIKELY(ret == MS_FAILURE))
          goto legend_icon_cleanup;
      } else {
        ret = msDrawTransformedShape(map, image_draw, &box, theclass->styles[i],
                                     lp->scalefactor);
        if (MS_UNLIKELY(ret == MS_FAILURE))
          goto legend_icon_cleanup;
      }
    }
    break;
  default:
    return MS_FAILURE;
    break;
  } /* end symbol drawing */

  /* handle label styles */
  for (i = 0; i < theclass->numlabels; i++) {
    labelObj *l = theclass->labels[i];
    if (!scale_independant && map->scaledenom > 0) {
      if (msScaleInBounds(map->scaledenom, l->minscaledenom,
                          l->maxscaledenom)) {
        int j;
        for (j = 0; j < l->numstyles; j++) {
          styleObj *s = l->styles[j];
          marker.x = dstX + MS_NINT(width / 2.0);
          marker.y = dstY + MS_NINT(height / 2.0);
          if (s->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT) {
            ret = msDrawMarkerSymbol(map, image_draw, &marker, s,
                                     lp->scalefactor);
            if (MS_UNLIKELY(ret == MS_FAILURE))
              goto legend_icon_cleanup;
          }
        }
      }
    }
  }

  /* handle "pure" text layers, i.e. layers with no symbology */
  hasmarkersymbol = 0;
  if (theclass->numstyles == 0) {
    for (i = 0; i < theclass->numlabels; i++) {
      labelObj *l = theclass->labels[i];
      if (!scale_independant && map->scaledenom > 0) {
        if (msScaleInBounds(map->scaledenom, l->minscaledenom,
                            l->maxscaledenom)) {
          int j;
          for (j = 0; j < l->numstyles; j++) {
            styleObj *s = l->styles[j];
            if (s->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT) {
              hasmarkersymbol = 1;
            }
          }
        }
      }
    }
  } else {
    hasmarkersymbol = 1;
  }

  if (!hasmarkersymbol && theclass->numlabels > 0) {
    textSymbolObj ts;
    pointObj textstartpt;
    marker.x = dstX + MS_NINT(width / 2.0);
    marker.y = dstY + MS_NINT(height / 2.0);
    initTextSymbol(&ts);
    msPopulateTextSymbolForLabelAndString(
        &ts, theclass->labels[0], msStrdup("Az"),
        lp->scalefactor * image_draw->resolutionfactor,
        image_draw->resolutionfactor, duplicate_always);
    ts.label->size = height - 1;
    ts.rotation = 0;
    ret = msComputeTextPath(map, &ts);
    if (MS_UNLIKELY(ret == MS_FAILURE))
      goto legend_icon_cleanup;
    textstartpt = get_metrics(&marker, MS_CC, ts.textpath, 0, 0, 0, 0, NULL);
    ret = msDrawTextSymbol(map, image_draw, textstartpt, &ts);
    freeTextSymbol(&ts);
    if (MS_UNLIKELY(ret == MS_FAILURE))
      goto legend_icon_cleanup;
  }

  /* handle an outline if necessary */
  if (MS_VALID_COLOR(map->legend.outlinecolor)) {
    initStyle(&outline_style);
    outline_style.color = map->legend.outlinecolor;
    ret = msDrawLineSymbol(map, image_draw, &box, &outline_style,
                           image_draw->resolutionfactor);
    if (MS_UNLIKELY(ret == MS_FAILURE))
      goto legend_icon_cleanup;
    /* reset clipping rectangle */
    if (renderer->supports_clipping)
      renderer->resetClip(image_draw);
  }

  if (altFormat) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image);
    rendererVTableObj *altrenderer = MS_IMAGE_RENDERER(image_draw);
    rasterBufferObj rb;
    memset(&rb, 0, sizeof(rasterBufferObj));

    ret = altrenderer->getRasterBufferHandle(image_draw, &rb);
    if (MS_UNLIKELY(ret == MS_FAILURE))
      goto legend_icon_cleanup;
    ret = renderer->mergeRasterBuffer(
        image, &rb, ((lp->compositer) ? lp->compositer->opacity * 0.01 : 1.0),
        0, 0, 0, 0, rb.width, rb.height);
    if (MS_UNLIKELY(ret == MS_FAILURE))
      goto legend_icon_cleanup;
    /*
     * hack to work around bug #3834: if we have use an alternate renderer, the
     * symbolset may contain symbols that reference it. We want to remove those
     * references before the altFormat is destroyed to avoid a segfault and/or a
     * leak, and so the the main renderer doesn't pick the cache up thinking
     * it's for him.
     */
    for (i = 0; i < map->symbolset.numsymbols; i++) {
      if (map->symbolset.symbol[i] != NULL) {
        symbolObj *s = map->symbolset.symbol[i];
        if (s->renderer == altrenderer) {
          altrenderer->freeSymbol(s);
          s->renderer = NULL;
        }
      }
    }

  } else if (image != image_draw) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image_draw);
    rasterBufferObj rb;
    memset(&rb, 0, sizeof(rasterBufferObj));

    ret = renderer->getRasterBufferHandle(image_draw, &rb);
    if (MS_UNLIKELY(ret == MS_FAILURE))
      goto legend_icon_cleanup;
    ret = renderer->mergeRasterBuffer(
        image, &rb, ((lp->compositer) ? lp->compositer->opacity * 0.01 : 1.0),
        0, 0, 0, 0, rb.width, rb.height);
    if (MS_UNLIKELY(ret == MS_FAILURE))
      goto legend_icon_cleanup;

    /* deref and possibly free temporary transparent output format.  */
    msApplyOutputFormat(&transFormat, NULL, MS_NOOVERRIDE);
  }

legend_icon_cleanup:
  if (image != image_draw) {
    msFreeImage(image_draw);
  }
  return ret;
}

imageObj *msCreateLegendIcon(mapObj *map, layerObj *lp, classObj *class,
                             int width, int height, int scale_independant) {
  imageObj *image;
  outputFormatObj *format = NULL;

  rendererVTableObj *renderer = MS_MAP_RENDERER(map);

  if (!renderer) {
    msSetError(MS_MISCERR, "invalid map outputformat", "msCreateLegendIcon()");
    return (NULL);
  }

  /* ensure we have an image format representing the options for the legend */
  msApplyOutputFormat(&format, map->outputformat, map->legend.transparent);

  image = msImageCreate(width, height, format, map->web.imagepath,
                        map->web.imageurl, map->resolution, map->defresolution,
                        &(map->legend.imagecolor));

  /* drop this reference to output format */
  msApplyOutputFormat(&format, NULL, MS_NOOVERRIDE);

  if (image == NULL) {
    msSetError(MS_IMGERR, "Unable to initialize image.",
               "msCreateLegendIcon()");
    return (NULL);
  }
  image->map = map;

  /* Call drawLegendIcon with destination (0, 0) */
  /* Return an empty image if lp==NULL || class=NULL  */
  /* (If class is NULL draw the legend for all classes. Modifications done */
  /* Fev 2004 by AY) */
  if (lp) {
    if (class) {
      if (MS_UNLIKELY(MS_FAILURE ==
                      msDrawLegendIcon(map, lp, class, width, height, image, 0,
                                       0, scale_independant, NULL))) {
        msFreeImage(image);
        return NULL;
      }
    } else {
      for (int i = 0; i < lp->numclasses; i++) {
        if (MS_UNLIKELY(MS_FAILURE == msDrawLegendIcon(map, lp, lp->class[i],
                                                       width, height, image, 0,
                                                       0, scale_independant,
                                                       NULL))) {
          msFreeImage(image);
          return NULL;
        }
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
int msLegendCalcSize(mapObj *map, int scale_independent, int *size_x,
                     int *size_y, int *layer_index, int num_layers,
                     map_hittest *hittest, double resolutionfactor) {
  int i, j;
  int status, maxwidth = 0, nLegendItems = 0;
  char *text;
  layerObj *lp;
  rectObj rect;
  int current_layers = 0;

  /* reset sizes */
  *size_x = 0;
  *size_y = 0;

  /* enable scale-dependent calculations */
  if (!scale_independent) {
    map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
    status = msCalculateScale(map->extent, map->units, map->width, map->height,
                              map->resolution, &map->scaledenom);
    if (status != MS_SUCCESS)
      return MS_FAILURE;
  }

  /*
   * step through all map classes, and for each one that will be displayed
   * calculate the label size
   */
  if (layer_index != NULL && num_layers > 0)
    current_layers = num_layers;
  else
    current_layers = map->numlayers;

  for (i = 0; i < current_layers; i++) {
    int layerindex;
    if (layer_index != NULL && num_layers > 0)
      layerindex = layer_index[i];
    else
      layerindex = map->layerorder[i];

    lp = (GET_LAYER(map, layerindex));

    if ((lp->status == MS_OFF && (layer_index == NULL || num_layers <= 0)) ||
        (lp->type == MS_LAYER_QUERY)) /* skip it */
      continue;

    if (hittest && hittest->layerhits[layerindex].status == 0)
      continue;

    if (!scale_independent && map->scaledenom > 0) {
      if ((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom))
        continue;
      if ((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom))
        continue;
    }

    for (j = lp->numclasses - 1; j >= 0; j--) {
      textSymbolObj ts;
      text = lp->class[j]->title
                 ? lp->class[j]->title
                 : lp->class[j]->name; /* point to the right legend text, title
                                          takes precedence */
      if (!text)
        continue; /* skip it */

      /* skip the class if the classgroup is defined */
      if (lp->classgroup &&
          (lp->class[j] -> group == NULL || strcasecmp(lp->class[j] -> group,
                                                       lp -> classgroup) != 0))
        continue;

      /* verify class scale */
      if (!scale_independent && map->scaledenom > 0) {
        if ((lp->class[j] -> maxscaledenom > 0) &&
            (map->scaledenom > lp->class[j] -> maxscaledenom))
          continue;
        if ((lp->class[j] -> minscaledenom > 0) &&
            (map->scaledenom <= lp->class[j] -> minscaledenom))
          continue;
      }
      if (hittest && hittest->layerhits[layerindex].classhits[j].status == 0)
        continue;

      if (*text) {
        initTextSymbol(&ts);
        msPopulateTextSymbolForLabelAndString(&ts, &map->legend.label,
                                              msStrdup(text), resolutionfactor,
                                              resolutionfactor, 0);
        if (MS_UNLIKELY(MS_FAILURE == msGetTextSymbolSize(map, &ts, &rect))) {
          freeTextSymbol(&ts);
          return MS_FAILURE;
        }
        freeTextSymbol(&ts);

        maxwidth = MS_MAX(maxwidth, MS_NINT(rect.maxx - rect.minx));
        *size_y += MS_MAX(MS_NINT(rect.maxy - rect.miny), map->legend.keysizey);
      } else {
        *size_y += map->legend.keysizey;
      }
      nLegendItems++;
    }
  }

  /* Calculate the size of the legend: */
  /*   - account for the Y keyspacing */
  *size_y += (2 * VMARGIN) + ((nLegendItems - 1) * map->legend.keyspacingy);
  /*   - determine the legend width */
  *size_x =
      (2 * HMARGIN) + maxwidth + map->legend.keyspacingx + map->legend.keysizex;

  if (*size_y <= 0 || *size_x <= 0)
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
imageObj *msDrawLegend(mapObj *map, int scale_independent,
                       map_hittest *hittest) {
  int i, j, ret = MS_SUCCESS; /* loop counters */
  pointObj pnt;
  int size_x, size_y = 0;
  layerObj *lp;
  rectObj rect;
  imageObj *image = NULL;
  outputFormatObj *format = NULL;
  char *text;

  struct legend_struct {
    int height;
    textSymbolObj ts;
    int layerindex, classindex;
    struct legend_struct *pred;
  };
  typedef struct legend_struct legendlabel;
  legendlabel *head = NULL;

  if (!MS_RENDERER_PLUGIN(map->outputformat)) {
    msSetError(MS_MISCERR, "unsupported output format", "msDrawLegend()");
    return NULL;
  }
  if (msValidateContexts(map) != MS_SUCCESS)
    return NULL; /* make sure there are no recursive REQUIRES or LABELREQUIRES
                    expressions */
  if (msLegendCalcSize(map, scale_independent, &size_x, &size_y, NULL, 0,
                       hittest,
                       map->resolution / map->defresolution) != MS_SUCCESS)
    return NULL;

  /*
   * step through all map classes, and for each one that will be displayed
   * keep a reference to its label size and text
   */

  for (i = 0; i < map->numlayers; i++) {
    lp = (GET_LAYER(map, map->layerorder[i]));

    if ((lp->status == MS_OFF) || (lp->type == MS_LAYER_QUERY)) /* skip it */
      continue;

    if (hittest && hittest->layerhits[map->layerorder[i]].status == 0)
      continue;

    if (!scale_independent && map->scaledenom > 0) {
      if ((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom))
        continue;
      if ((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom))
        continue;
    }

    if (!scale_independent && lp->maxscaledenom <= 0 &&
        lp->minscaledenom <= 0) {
      if ((lp->maxgeowidth > 0) &&
          ((map->extent.maxx - map->extent.minx) > lp->maxgeowidth))
        continue;
      if ((lp->mingeowidth > 0) &&
          ((map->extent.maxx - map->extent.minx) < lp->mingeowidth))
        continue;
    }

    /* set the scale factor so that scale dependent symbols are drawn in the
     * legend with their default size */
    if (lp->sizeunits != MS_PIXELS) {
      map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
      lp->scalefactor =
          (msInchesPerUnit(lp->sizeunits, 0) / msInchesPerUnit(map->units, 0)) /
          map->cellsize;
    }

    for (j = lp->numclasses - 1; j >= 0; j--) {
      text = lp->class[j]->title
                 ? lp->class[j]->title
                 : lp->class[j]->name; /* point to the right legend text, title
                                          takes precedence */
      if (!text)
        continue; /* skip it */

      /* skip the class if the classgroup is defined */
      if (lp->classgroup &&
          (lp->class[j] -> group == NULL || strcasecmp(lp->class[j] -> group,
                                                       lp -> classgroup) != 0))
        continue;

      if (!scale_independent &&
          map->scaledenom > 0) { /* verify class scale here */
        if ((lp->class[j] -> maxscaledenom > 0) &&
            (map->scaledenom > lp->class[j] -> maxscaledenom))
          continue;
        if ((lp->class[j] -> minscaledenom > 0) &&
            (map->scaledenom <= lp->class[j] -> minscaledenom))
          continue;
      }

      if (hittest &&
          hittest->layerhits[map->layerorder[i]].classhits[j].status == 0) {
        continue;
      }

      legendlabel *cur = (legendlabel *)msSmallMalloc(sizeof(legendlabel));
      initTextSymbol(&cur->ts);
      if (*text) {
        msPopulateTextSymbolForLabelAndString(
            &cur->ts, &map->legend.label, msStrdup(text),
            map->resolution / map->defresolution,
            map->resolution / map->defresolution, 0);
        if (MS_UNLIKELY(MS_FAILURE == msComputeTextPath(map, &cur->ts))) {
          ret = MS_FAILURE;
          goto cleanup;
        }
        if (MS_UNLIKELY(MS_FAILURE ==
                        msGetTextSymbolSize(map, &cur->ts, &rect))) {
          ret = MS_FAILURE;
          goto cleanup;
        }
        cur->height =
            MS_MAX(MS_NINT(rect.maxy - rect.miny), map->legend.keysizey);
      } else {
        cur->height = map->legend.keysizey;
      }

      cur->classindex = j;
      cur->layerindex = map->layerorder[i];
      cur->pred = head;
      head = cur;
    }
  }

  /* ensure we have an image format representing the options for the legend. */
  msApplyOutputFormat(&format, map->outputformat, map->legend.transparent);

  /* initialize the legend image */
  image = msImageCreate(size_x, size_y, format, map->web.imagepath,
                        map->web.imageurl, map->resolution, map->defresolution,
                        &map->legend.imagecolor);
  if (!image) {
    msSetError(MS_MISCERR, "Unable to initialize image.", "msDrawLegend()");
    return NULL;
  }
  image->map = map;

  /* image =
   * renderer->createImage(size_x,size_y,format,&(map->legend.imagecolor)); */

  /* drop this reference to output format */
  msApplyOutputFormat(&format, NULL, MS_NOOVERRIDE);

  pnt.y = VMARGIN;
  pnt.x = HMARGIN + map->legend.keysizex + map->legend.keyspacingx;

  while (head) { /* head initially points on the last legend item, i.e. the one
                    that should be at the top */
    legendlabel *cur = head;
    class_hittest *ch = NULL;

    /* set the scale factor so that scale dependent symbols are drawn in the
     * legend with their default size */
    if (map->layers[cur->layerindex]->sizeunits != MS_PIXELS) {
      map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
      map->layers[cur->layerindex]->scalefactor =
          (msInchesPerUnit(map->layers[cur->layerindex]->sizeunits, 0) /
           msInchesPerUnit(map->units, 0)) /
          map->cellsize;
    }
    if (hittest) {
      ch = &hittest->layerhits[cur->layerindex].classhits[cur->classindex];
    }
    ret = msDrawLegendIcon(map, map->layers[cur->layerindex],
                           map->layers[cur->layerindex]->class[cur->classindex],
                           map -> legend.keysizex, map->legend.keysizey, image,
                           HMARGIN, (int)pnt.y, scale_independent, ch);
    if (MS_UNLIKELY(ret != MS_SUCCESS))
      goto cleanup;

    pnt.y += cur->height;

    if (cur->ts.annotext) {
      pointObj textPnt = pnt;
      textPnt.y -= cur->ts.textpath->bounds.bbox.maxy;
      textPnt.y += map->legend.label.offsety;
      textPnt.x += map->legend.label.offsetx;
      ret = msDrawTextSymbol(map, image, textPnt, &cur->ts);
      if (MS_UNLIKELY(ret == MS_FAILURE))
        goto cleanup;
      /* Coverity Scan is confused by label refcount, and wrongly believe we */
      /* might free &map->legend.label, so make it clear we won't */
      freeTextSymbolEx(&cur->ts, MS_FALSE);
      MS_REFCNT_DECR(cur->ts.label);
    }

    pnt.y += map->legend.keyspacingy; /* bump y for next label */

    /* clean up */
    head = cur->pred;
    free(cur);
  } /* next legend */

cleanup:
  while (head) {
    legendlabel *cur = head;
    /* Coverity Scan is confused by label refcount, and wrongly believe we */
    /* might free &map->legend.label, so make it clear we won't */
    freeTextSymbolEx(&cur->ts, MS_FALSE);
    MS_REFCNT_DECR(cur->ts.label);
    head = cur->pred;
    free(cur);
  }
  if (MS_UNLIKELY(ret != MS_SUCCESS)) {
    if (image)
      msFreeImage(image);
    return NULL;
  }
  return (image);
}

/* TODO */
int msEmbedLegend(mapObj *map, imageObj *img) {
  pointObj point;
  imageObj *image = NULL;
  symbolObj *legendSymbol;
  char *imageType = NULL;
  const char *const LEGEND_SYMBOL_NAME = "legend";

  rendererVTableObj *renderer;

  int symbolIdx =
      msGetSymbolIndex(&(map->symbolset), LEGEND_SYMBOL_NAME, MS_FALSE);
  if (symbolIdx != -1)
    msRemoveSymbol(&(map->symbolset),
                   symbolIdx); /* solves some caching issues in AGG with
                                  long-running processes */

  if (msGrowSymbolSet(&map->symbolset) == NULL)
    return -1;
  symbolIdx = map->symbolset.numsymbols;
  legendSymbol = map->symbolset.symbol[symbolIdx];
  map->symbolset.numsymbols++;
  initSymbol(legendSymbol);

  if (!MS_RENDERER_PLUGIN(map->outputformat) ||
      !MS_MAP_RENDERER(map)->supports_pixel_buffer) {
    imageType = msStrdup(map->imagetype); /* save format */
    if MS_DRIVER_CAIRO (map->outputformat)
      map->outputformat = msSelectOutputFormat(map, "cairopng");
    else
      map->outputformat = msSelectOutputFormat(map, "png");

    msInitializeRendererVTable(map->outputformat);
  }
  renderer = MS_MAP_RENDERER(map);

  /* render the legend. */
  image = msDrawLegend(map, MS_FALSE, NULL);
  if (image == NULL) {
    msFree(imageType);
    return MS_FAILURE;
  }

  if (imageType) {
    map->outputformat =
        msSelectOutputFormat(map, imageType); /* restore format */
    msFree(imageType);
  }

  /* copy renderered legend image into symbol */
  legendSymbol->pixmap_buffer = calloc(1, sizeof(rasterBufferObj));
  MS_CHECK_ALLOC(legendSymbol->pixmap_buffer, sizeof(rasterBufferObj),
                 MS_FAILURE);

  if (MS_SUCCESS !=
      renderer->getRasterBufferCopy(image, legendSymbol->pixmap_buffer))
    return MS_FAILURE;
  legendSymbol->renderer = renderer;

  msFreeImage(image);

  if (!legendSymbol->pixmap_buffer)
    return (MS_FAILURE); /* something went wrong creating scalebar */

  legendSymbol->type = MS_SYMBOL_PIXMAP; /* initialize a few things */
  legendSymbol->name = msStrdup(LEGEND_SYMBOL_NAME);
  legendSymbol->sizex = legendSymbol->pixmap_buffer->width;
  legendSymbol->sizey = legendSymbol->pixmap_buffer->height;

  /* I'm not too sure this test is sufficient ... NFW. */
  /* if(map->legend.transparent == MS_ON) */
  /*  gdImageColorTransparent(legendSymbol->img_deprecated, 0); */

  switch (map->legend.position) {
  case (MS_LL):
    point.x = MS_NINT(legendSymbol->sizex / 2.0);
    point.y = map->height - MS_NINT(legendSymbol->sizey / 2.0);
    break;
  case (MS_LR):
    point.x = map->width - MS_NINT(legendSymbol->sizex / 2.0);
    point.y = map->height - MS_NINT(legendSymbol->sizey / 2.0);
    break;
  case (MS_LC):
    point.x = MS_NINT(map->width / 2.0);
    point.y = map->height - MS_NINT(legendSymbol->sizey / 2.0);
    break;
  case (MS_UR):
    point.x = map->width - MS_NINT(legendSymbol->sizex / 2.0);
    point.y = MS_NINT(legendSymbol->sizey / 2.0);
    break;
  case (MS_UL):
    point.x = MS_NINT(legendSymbol->sizex / 2.0);
    point.y = MS_NINT(legendSymbol->sizey / 2.0);
    break;
  case (MS_UC):
    point.x = MS_NINT(map->width / 2.0);
    point.y = MS_NINT(legendSymbol->sizey / 2.0);
    break;
  }

  const char *const EMBED_LEGEND_LAYER_NAME = "__embed__legend";
  int layerIdx = msGetLayerIndex(map, EMBED_LEGEND_LAYER_NAME);
  if (layerIdx == -1) {
    if (msGrowMapLayers(map) == NULL)
      return (-1);
    layerIdx = map->numlayers;
    map->numlayers++;
    layerObj *layer = GET_LAYER(map, layerIdx);
    if (initLayer(layer, map) == -1)
      return (-1);
    layer->name = msStrdup(EMBED_LEGEND_LAYER_NAME);
    layer->type = MS_LAYER_POINT;

    if (msGrowLayerClasses(layer) == NULL)
      return (-1);
    if (initClass(layer->class[0]) == -1)
      return (-1);
    layer->numclasses = 1; /* so we make sure to free it */

    /* update the layer order list with the layer's index. */
    map->layerorder[layerIdx] = layerIdx;
  }

  layerObj *layer = GET_LAYER(map, layerIdx);

  layer->status = MS_ON;

  classObj *klass = layer->class[0];
  if (map->legend.postlabelcache) { /* add it directly to the image */
    if (MS_UNLIKELY(msMaybeAllocateClassStyle(klass, 0) == MS_FAILURE))
      return MS_FAILURE;
    klass->styles[0]->symbol = symbolIdx;
    if (MS_UNLIKELY(MS_FAILURE == msDrawMarkerSymbol(map, img, &point,
                                                     klass->styles[0], 1.0)))
      return MS_FAILURE;
  } else {
    if (!klass->labels) {
      if (msGrowClassLabels(klass) == NULL)
        return MS_FAILURE;
      labelObj *label = klass->labels[0];
      initLabel(label);
      klass->numlabels = 1;
      label->force = MS_TRUE;
      label->size =
          MS_MEDIUM; /* must set a size to have a valid label definition */
      label->priority = MS_MAX_LABEL_PRIORITY;
    }
    labelObj *label = klass->labels[0];
    if (label->numstyles == 0) {
      if (msGrowLabelStyles(label) == NULL)
        return (MS_FAILURE);
      label->numstyles = 1;
      initStyle(label->styles[0]);
      label->styles[0]->_geomtransform.type = MS_GEOMTRANSFORM_LABELPOINT;
    }
    label->styles[0]->symbol = symbolIdx;
    if (MS_UNLIKELY(MS_FAILURE == msAddLabel(map, img, label, layerIdx, 0, NULL,
                                             &point, -1, NULL)))
      return MS_FAILURE;
  }

  /* Mark layer as deleted so that it doesn't interfere with html legends or
   * with saving maps */
  layer->status = MS_DELETE;

  return MS_SUCCESS;
}

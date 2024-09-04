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

#define MS_CHART_TYPE_PIE 1
#define MS_CHART_TYPE_BAR 2
#define MS_CHART_TYPE_VBAR 3

/*
** check if an object of width w and height h placed at point x,y can fit in an
*image of width mw and height mh
*/
#define MS_CHART_FITS(x, y, w, h, mw, mh)                                      \
  (((x) - (w) / 2. > 0.) && ((x) + (w) / 2. < (mw)) &&                         \
   ((y) - (h) / 2. > 0.) && ((y) + (h) / 2.) < (mh))

/*
** find a point on a shape. check if it fits in image
** returns
**  MS_SUCCESS and point coordinates in 'p' if chart fits in image
**  MS_FAILURE if no point could be found
*/
int findChartPoint(mapObj *map, shapeObj *shape, int width, int height,
                   pointObj *center) {
  int middle, numpoints;
  double invcellsize = 1.0 / map->cellsize; /*speed up MAP2IMAGE_X/Y_IC_DBL*/
  switch (shape->type) {
  case MS_SHAPE_POINT:
    center->x = MS_MAP2IMAGE_X_IC_DBL(shape->line[0].point[0].x,
                                      map->extent.minx, invcellsize);
    center->y = MS_MAP2IMAGE_Y_IC_DBL(shape->line[0].point[0].y,
                                      map->extent.maxy, invcellsize);

    if (MS_CHART_FITS(center->x, center->y, width, height, map->width,
                      map->height))
      return MS_SUCCESS;
    else
      return MS_FAILURE;
    break;
  case MS_SHAPE_LINE:
    /*loop through line segments starting from middle (alternate between before
     *and after middle point) *first segment that fits is chosen
     */
    middle = shape->line[0].numpoints / 2; /*start with middle segment of line*/
    numpoints = shape->line[0].numpoints;
    if (1 <= middle) {
      int idx = middle + 1;
      if (idx < numpoints) {
        center->x =
            (shape->line[0].point[idx - 1].x + shape->line[0].point[idx].x) /
            2.;
        center->y =
            (shape->line[0].point[idx - 1].y + shape->line[0].point[idx].y) /
            2.;
        center->x =
            MS_MAP2IMAGE_X_IC_DBL(center->x, map->extent.minx, invcellsize);
        center->y =
            MS_MAP2IMAGE_Y_IC_DBL(center->y, map->extent.maxy, invcellsize);

        if (MS_CHART_FITS(center->x, center->y, width, height, map->width,
                          map->height))
          return MS_SUCCESS;

        return MS_FAILURE;
      }
      idx = middle - 1;
      center->x =
          (shape->line[0].point[idx].x + shape->line[0].point[idx + 1].x) / 2;
      center->y =
          (shape->line[0].point[idx].y + shape->line[0].point[idx + 1].y) / 2;
      center->x =
          MS_MAP2IMAGE_X_IC_DBL(center->x, map->extent.minx, invcellsize);
      center->y =
          MS_MAP2IMAGE_Y_IC_DBL(center->y, map->extent.maxy, invcellsize);

      if (MS_CHART_FITS(center->x, center->y, width, height, map->width,
                        map->height))
        return MS_SUCCESS;
      return MS_FAILURE;
    }
    return MS_FAILURE;
    break;
  case MS_SHAPE_POLYGON:
    msPolygonLabelPoint(shape, center, -1);
    center->x = MS_MAP2IMAGE_X_IC_DBL(center->x, map->extent.minx, invcellsize);
    center->y = MS_MAP2IMAGE_Y_IC_DBL(center->y, map->extent.maxy, invcellsize);

    if (MS_CHART_FITS(center->x, center->y, width, height, map->width,
                      map->height))
      return MS_SUCCESS;
    else
      return MS_FAILURE;
    break;
  default:
    return MS_FAILURE;
  }
}

int WARN_UNUSED drawRectangle(mapObj *map, imageObj *image, double mx,
                              double my, double Mx, double My,
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
  /* cppcheck-suppress unreadVariable */
  point[1].x = point[2].x = Mx;
  /* cppcheck-suppress unreadVariable */
  point[2].y = point[3].y = My;

  return msDrawShadeSymbol(map, image, &shape, style, 1.0);
}

int WARN_UNUSED msDrawVBarChart(mapObj *map, imageObj *image, pointObj *center,
                                double *values, styleObj **styles,
                                int numvalues, double barWidth) {

  int c;
  double left, cur; /*shortcut to pixel boundaries of the chart*/
  double height = 0;

  for (c = 0; c < numvalues; c++) {
    height += values[c];
  }

  cur = center->y + height / 2.;
  left = center->x - barWidth / 2.;

  for (c = 0; c < numvalues; c++) {
    if (MS_UNLIKELY(MS_FAILURE == drawRectangle(map, image, left, cur,
                                                left + barWidth,
                                                cur - values[c], styles[c])))
      return MS_FAILURE;
    cur -= values[c];
  }
  return MS_SUCCESS;
}

int msDrawBarChart(mapObj *map, imageObj *image, pointObj *center,
                   double *values, styleObj **styles, int numvalues,
                   double width, double height, double *maxVal, double *minVal,
                   double barWidth) {

  double upperLimit, lowerLimit;
  double shapeMaxVal, shapeMinVal, pixperval;
  int c;
  double vertOrigin, vertOriginClipped, horizStart, y;
  double left, top, bottom; /*shortcut to pixel boundaries of the chart*/

  top = center->y - height / 2.;
  bottom = center->y + height / 2.;
  left = center->x - width / 2.;

  shapeMaxVal = shapeMinVal = values[0];
  for (c = 1; c < numvalues; c++) {
    if (maxVal == NULL || minVal == NULL) { /*compute bounds if not specified*/
      if (values[c] > shapeMaxVal)
        shapeMaxVal = values[c];
      if (values[c] < shapeMinVal)
        shapeMinVal = values[c];
    }
  }

  /*
   * use specified bounds if wanted
   * if not, always show the origin
   */
  upperLimit = (maxVal != NULL) ? *maxVal : MS_MAX(shapeMaxVal, 0);
  lowerLimit = (minVal != NULL) ? *minVal : MS_MIN(shapeMinVal, 0);
  if (upperLimit == lowerLimit) {
    /* treat the case where we would have an unspecified behavior */
    upperLimit += 0.5;
    lowerLimit -= 0.5;
  }

  pixperval = height / (upperLimit - lowerLimit);
  vertOrigin = bottom + lowerLimit * pixperval;
  vertOriginClipped = (vertOrigin < top)      ? top
                      : (vertOrigin > bottom) ? bottom
                                              : vertOrigin;
  horizStart = left;

  for (c = 0; c < numvalues; c++) {
    double barHeight = values[c] * pixperval;
    /*clip bars*/
    y = ((vertOrigin - barHeight) < top)    ? top
        : (vertOrigin - barHeight > bottom) ? bottom
                                            : vertOrigin - barHeight;
    if (y != vertOriginClipped) { /*don't draw bars of height == 0 (i.e. either
                                     values==0, or clipped)*/
      if (values[c] > 0) {
        if (MS_UNLIKELY(MS_FAILURE == drawRectangle(map, image, horizStart, y,
                                                    horizStart + barWidth - 1,
                                                    vertOriginClipped,
                                                    styles[c])))
          return MS_FAILURE;
      } else {
        if (MS_UNLIKELY(MS_FAILURE ==
                        drawRectangle(map, image, horizStart, vertOriginClipped,
                                      horizStart + barWidth - 1, y, styles[c])))
          return MS_FAILURE;
      }
    }
    horizStart += barWidth;
  }
  return MS_SUCCESS;
}

int WARN_UNUSED msDrawPieChart(mapObj *map, imageObj *image, pointObj *center,
                               double diameter, double *values,
                               styleObj **styles, int numvalues) {
  int i;
  double dTotal = 0., start = 0;

  for (i = 0; i < numvalues; i++) {
    if (values[i] < 0.) {
      msSetError(MS_MISCERR, "cannot draw pie charts for negative values",
                 "msDrawPieChart()");
      return MS_FAILURE;
    }
    dTotal += values[i];
  }

  for (i = 0; i < numvalues; i++) {
    double angle = values[i];
    if (angle == 0)
      continue; /*no need to draw. causes artifacts with outlines*/
    angle *= 360.0 / dTotal;
    if (MS_UNLIKELY(MS_FAILURE == msDrawPieSlice(map, image, center, styles[i],
                                                 diameter / 2., start,
                                                 start + angle)))
      return MS_FAILURE;

    start += angle;
  }
  return MS_SUCCESS;
}

int getNextShape(mapObj *map, layerObj *layer, double *values, int *nvalues,
                 styleObj **styles, shapeObj *shape) {
  int status;
  int c;
  status = msLayerNextShape(layer, shape);
  if (status == MS_SUCCESS) {

    if (layer->project) {
      if (layer->reprojectorLayerToMap == NULL) {
        layer->reprojectorLayerToMap =
            msProjectCreateReprojector(&layer->projection, &map->projection);
        if (layer->reprojectorLayerToMap == NULL) {
          return MS_FAILURE;
        }
      }
      msProjectShapeEx(layer->reprojectorLayerToMap, shape);
    }

    if (msBindLayerToShape(layer, shape,
                           MS_DRAWMODE_FEATURES | MS_DRAWMODE_LABELS) !=
        MS_SUCCESS)
      return MS_FAILURE; /* error message is set in msBindLayerToShape() */

    *nvalues = 0;
    for (c = 0; c < layer->numclasses; c++) {
      if (msEvalExpression(layer, shape, &(layer->class[c] -> expression),
                           layer->classitemindex) == MS_TRUE) {
        values[*nvalues] = (layer->class[c] -> styles[0] -> size);
        styles[*nvalues] = layer->class[c]->styles[0];
        (*nvalues)++;
      }
    }
  }
  return status;
}

/* eventually add a class to the layer to get the diameter from an attribute */
int pieLayerProcessDynamicDiameter(layerObj *layer) {
  const char *chartRangeProcessingKey = NULL;
  char *attrib;
  double mindiameter = -1, maxdiameter, minvalue, maxvalue;
  classObj *newclass;
  styleObj *newstyle;
  const char *chartSizeProcessingKey =
      msLayerGetProcessingKey(layer, "CHART_SIZE");
  if (chartSizeProcessingKey != NULL)
    return MS_FALSE;
  chartRangeProcessingKey = msLayerGetProcessingKey(layer, "CHART_SIZE_RANGE");
  if (chartRangeProcessingKey == NULL)
    return MS_FALSE;
  attrib = msStrdup(chartRangeProcessingKey);
  char *space = strchr(attrib, ' ');
  if (space) {
    *space = '\0';
    if (sscanf(space + 1, "%lf %lf %lf %lf", &mindiameter, &maxdiameter,
               &minvalue, &maxvalue) != 4) {
      /*we don't have the attribute and the four range values*/
      free(attrib);
      msSetError(MS_MISCERR,
                 "Chart Layer format error for processing key \"CHART_RANGE\"",
                 "msDrawChartLayer()");
      return MS_FAILURE;
    }
  }

  /*create a new class in the layer containing the wanted attribute
   * as the SIZE of its first STYLE*/
  newclass = msGrowLayerClasses(layer);
  if (newclass == NULL) {
    free(attrib);
    return MS_FAILURE;
  }
  initClass(newclass);
  layer->numclasses++;

  /*create and attach a new styleObj to our temp class
   * and bind the wanted attribute to its SIZE
   */
  newstyle = msGrowClassStyles(newclass);
  if (newstyle == NULL) {
    free(attrib);
    return MS_FAILURE;
  }
  initStyle(newstyle);
  newclass->numstyles++;
  newclass->name = (char *)msStrdup("__MS_SIZE_ATTRIBUTE_");
  newstyle->bindings[MS_STYLE_BINDING_SIZE].item = msStrdup(attrib);
  newstyle->numbindings++;
  free(attrib);

  return MS_TRUE;
}

/* clean up the class added temporarily */
static void pieLayerCleanupDynamicDiameter(layerObj *layer) {
  if (layer->numclasses > 0 &&
      EQUALN(layer->class[layer->numclasses - 1] -> name,
             "__MS_SIZE_ATTRIBUTE_", 20)) {
    classObj *c = msRemoveClass(layer, layer->numclasses - 1);
    freeClass(c);
    msFree(c);
  }
}

int msDrawPieChartLayer(mapObj *map, layerObj *layer, imageObj *image) {
  shapeObj shape;
  int status = MS_SUCCESS;
  const char *chartRangeProcessingKey = NULL;
  const char *chartSizeProcessingKey =
      msLayerGetProcessingKey(layer, "CHART_SIZE");
  double diameter = 0, mindiameter = -1, maxdiameter = 0, minvalue = 0,
         maxvalue = 0, exponent = 0;
  double *values;
  styleObj **styles;
  pointObj center;
  int numvalues =
      layer->numclasses; /* the number of classes to represent in the graph */
  int numvalues_for_shape = 0;

  if (chartSizeProcessingKey == NULL) {
    chartRangeProcessingKey =
        msLayerGetProcessingKey(layer, "CHART_SIZE_RANGE");
    if (chartRangeProcessingKey == NULL)
      diameter = 20;
    else {
      const int nvalues =
          sscanf(chartRangeProcessingKey, "%*s %lf %lf %lf %lf %lf",
                 &mindiameter, &maxdiameter, &minvalue, &maxvalue, &exponent);
      if (nvalues != 4 && nvalues != 5) {
        msSetError(
            MS_MISCERR,
            "msDrawChart format error for processing key \"CHART_SIZE_RANGE\": "
            "itemname minsize maxsize minval maxval [exponent] is expected",
            "msDrawPieChartLayer()");
        return MS_FAILURE;
      }
    }
  } else {
    if (sscanf(chartSizeProcessingKey, "%lf", &diameter) != 1) {
      msSetError(MS_MISCERR,
                 "msDrawChart format error for processing key \"CHART_SIZE\"",
                 "msDrawPieChartLayer()");
      return MS_FAILURE;
    }
  }

  layer->project =
      msProjectionsDiffer(&(layer->projection), &(map->projection));

  /* step through the target shapes */
  msInitShape(&shape);

  values = (double *)calloc(numvalues, sizeof(double));
  MS_CHECK_ALLOC(values, numvalues * sizeof(double), MS_FAILURE);
  styles = (styleObj **)malloc((numvalues) * sizeof(styleObj *));
  if (styles == NULL) {
    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n",
               "msDrawPieChartLayer()", __FILE__, __LINE__,
               (unsigned int)(numvalues * sizeof(styleObj *)));
    free(values);
    return MS_FAILURE;
  }

  while (MS_SUCCESS == getNextShape(map, layer, values, &numvalues_for_shape,
                                    styles, &shape)) {
    if (chartRangeProcessingKey != NULL)
      numvalues_for_shape--;
    if (numvalues_for_shape == 0) {
      msFreeShape(&shape);
      continue;
    }
    msDrawStartShape(map, layer, image, &shape);
    if (chartRangeProcessingKey != NULL) {
      diameter = values[numvalues_for_shape];
      if (mindiameter >= 0) {
        if (diameter <= minvalue)
          diameter = mindiameter;
        else if (diameter >= maxvalue)
          diameter = maxdiameter;
        else {
          if (exponent <= 0)
            diameter = MS_NINT(mindiameter +
                               ((diameter - minvalue) / (maxvalue - minvalue)) *
                                   (maxdiameter - mindiameter));
          else
            diameter = MS_NINT(
                mindiameter + pow((diameter - minvalue) / (maxvalue - minvalue),
                                  1.0 / exponent) *
                                  (maxdiameter - mindiameter));
        }
      }
    }
    if (findChartPoint(map, &shape, diameter, diameter, &center) ==
        MS_SUCCESS) {
      status = msDrawPieChart(map, image, &center, diameter, values, styles,
                              numvalues_for_shape);
    }
    msDrawEndShape(map, layer, image, &shape);
    msFreeShape(&shape);
  }
  free(values);
  free(styles);
  return status;
}

int msDrawVBarChartLayer(mapObj *map, layerObj *layer, imageObj *image) {
  shapeObj shape;
  int status = MS_SUCCESS;
  const char *chartSizeProcessingKey =
      msLayerGetProcessingKey(layer, "CHART_SIZE");
  const char *chartScaleProcessingKey =
      msLayerGetProcessingKey(layer, "CHART_SCALE");
  double barWidth, scale = 1.0;
  double *values;
  styleObj **styles;
  pointObj center;
  int numvalues = layer->numclasses;
  int numvalues_for_shape;
  if (chartSizeProcessingKey == NULL) {
    barWidth = 20;
  } else {
    if (sscanf(chartSizeProcessingKey, "%lf", &barWidth) != 1) {
      msSetError(MS_MISCERR,
                 "msDrawChart format error for processing key \"CHART_SIZE\"",
                 "msDrawVBarChartLayer()");
      return MS_FAILURE;
    }
  }

  if (chartScaleProcessingKey) {
    if (sscanf(chartScaleProcessingKey, "%lf", &scale) != 1) {
      msSetError(MS_MISCERR,
                 "Error reading value for processing key \"CHART_SCALE\"",
                 "msDrawVBarChartLayer()");
      return MS_FAILURE;
    }
  }
  msInitShape(&shape);

  values = (double *)calloc(numvalues, sizeof(double));
  MS_CHECK_ALLOC(values, numvalues * sizeof(double), MS_FAILURE);
  styles = (styleObj **)malloc(numvalues * sizeof(styleObj *));
  if (styles == NULL) {
    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n",
               "msDrawVBarChartLayer()", __FILE__, __LINE__,
               (unsigned int)(numvalues * sizeof(styleObj *)));
    free(values);
    return MS_FAILURE;
  }

  while (MS_SUCCESS == getNextShape(map, layer, values, &numvalues_for_shape,
                                    styles, &shape)) {
    int i;
    double h = 0;
    if (numvalues_for_shape == 0) {
      continue;
    }
    for (i = 0; i < numvalues_for_shape; i++) {
      values[i] *= scale;
      h += values[i];
    }
    msDrawStartShape(map, layer, image, &shape);
    if (findChartPoint(map, &shape, barWidth, h, &center) == MS_SUCCESS) {
      status = msDrawVBarChart(map, image, &center, values, styles,
                               numvalues_for_shape, barWidth);
    }
    msDrawEndShape(map, layer, image, &shape);
    msFreeShape(&shape);
  }
  free(values);
  free(styles);
  return status;
}

int msDrawBarChartLayer(mapObj *map, layerObj *layer, imageObj *image) {
  shapeObj shape;
  int status = MS_SUCCESS;
  const char *chartSizeProcessingKey =
      msLayerGetProcessingKey(layer, "CHART_SIZE");
  const char *barMax = msLayerGetProcessingKey(layer, "CHART_BAR_MAXVAL");
  const char *barMin = msLayerGetProcessingKey(layer, "CHART_BAR_MINVAL");
  double width, height;
  double barWidth;
  double *values;
  styleObj **styles;
  pointObj center;
  double barMaxVal = 0.0, barMinVal = 0.0;
  int numvalues = layer->numclasses;
  int numvalues_for_shape;
  if (chartSizeProcessingKey == NULL) {
    width = height = 20;
  } else {
    const int ret = sscanf(chartSizeProcessingKey, "%lf %lf", &width, &height);
    if (ret == 1) {
      height = width;
    } else if (ret != 2) {
      msSetError(MS_MISCERR,
                 "msDrawChart format error for processing key \"CHART_SIZE\"",
                 "msDrawBarChartLayer()");
      return MS_FAILURE;
    }
  }

  if (barMax) {
    if (sscanf(barMax, "%lf", &barMaxVal) != 1) {
      msSetError(MS_MISCERR,
                 "Error reading value for processing key \"CHART_BAR_MAXVAL\"",
                 "msDrawBarChartLayer()");
      return MS_FAILURE;
    }
  }
  if (barMin) {
    if (sscanf(barMin, "%lf", &barMinVal) != 1) {
      msSetError(MS_MISCERR,
                 "Error reading value for processing key \"CHART_BAR_MINVAL\"",
                 "msDrawBarChartLayer()");
      return MS_FAILURE;
    }
  }
  if (barMin && barMax && barMinVal >= barMaxVal) {
    msSetError(MS_MISCERR,
               "\"CHART_BAR_MINVAL\" must be less than \"CHART_BAR_MAXVAL\"",
               "msDrawBarChartLayer()");
    return MS_FAILURE;
  }
  barWidth = (double)width / (double)layer->numclasses;
  if (!barWidth) {
    msSetError(
        MS_MISCERR,
        "Specified width of chart too small to fit given number of classes",
        "msDrawBarChartLayer()");
    return MS_FAILURE;
  }

  msInitShape(&shape);

  values = (double *)calloc(numvalues, sizeof(double));
  MS_CHECK_ALLOC(values, numvalues * sizeof(double), MS_FAILURE);
  styles = (styleObj **)malloc(numvalues * sizeof(styleObj *));
  if (styles == NULL) {
    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n",
               "msDrawBarChartLayer()", __FILE__, __LINE__,
               (unsigned int)(numvalues * sizeof(styleObj *)));
    free(values);
    return MS_FAILURE;
  }

  while (MS_SUCCESS == getNextShape(map, layer, values, &numvalues_for_shape,
                                    styles, &shape)) {
    if (numvalues_for_shape == 0)
      continue;
    msDrawStartShape(map, layer, image, &shape);
    if (findChartPoint(map, &shape, width, height, &center) == MS_SUCCESS) {
      status = msDrawBarChart(map, image, &center, values, styles,
                              numvalues_for_shape, width, height,
                              (barMax != NULL) ? &barMaxVal : NULL,
                              (barMin != NULL) ? &barMinVal : NULL, barWidth);
    }
    msDrawEndShape(map, layer, image, &shape);
    msFreeShape(&shape);
  }
  free(values);
  free(styles);
  return status;
}

/**
 * Generic function to render chart layers.
 */
int msDrawChartLayer(mapObj *map, layerObj *layer, imageObj *image) {

  rectObj searchrect;
  const char *chartTypeProcessingKey =
      msLayerGetProcessingKey(layer, "CHART_TYPE");
  int chartType = MS_CHART_TYPE_PIE;
  int status = MS_FAILURE;

  if (image && map) {
    if (!(MS_RENDERER_PLUGIN(image->format))) {
      msSetError(MS_MISCERR,
                 "chart drawing currently only supports GD and AGG renderers",
                 "msDrawChartLayer()");
      return MS_FAILURE;
    }

    if (chartTypeProcessingKey != NULL) {
      if (strcasecmp(chartTypeProcessingKey, "PIE") == 0) {
        chartType = MS_CHART_TYPE_PIE;
      } else if (strcasecmp(chartTypeProcessingKey, "BAR") == 0) {
        chartType = MS_CHART_TYPE_BAR;
      } else if (strcasecmp(chartTypeProcessingKey, "VBAR") == 0) {
        chartType = MS_CHART_TYPE_VBAR;
      } else {
        msSetError(MS_MISCERR,
                   "unknown chart type for processing key \"CHART_TYPE\", must "
                   "be one of \"PIE\" or \"BAR\"",
                   "msDrawChartLayer()");
        return MS_FAILURE;
      }
    }
    if (chartType == MS_CHART_TYPE_PIE) {
      pieLayerProcessDynamicDiameter(layer);
    }

    /* open this layer */
    status = msLayerOpen(layer);
    if (status != MS_SUCCESS)
      return MS_FAILURE;

    status = msLayerWhichItems(layer, MS_FALSE, NULL);
    if (status != MS_SUCCESS) {
      msLayerClose(layer);
      return MS_FAILURE;
    }
    /* identify target shapes */
    if (layer->transform == MS_TRUE)
      searchrect = map->extent;
    else {
      searchrect.minx = searchrect.miny = 0;
      searchrect.maxx = map->width - 1;
      searchrect.maxy = map->height - 1;
    }

    if ((map->projection.numargs > 0) && (layer->projection.numargs > 0))
      msProjectRect(&map->projection, &layer->projection,
                    &searchrect); /* project the searchrect to source coords */

    status = msLayerWhichShapes(layer, searchrect, MS_FALSE);
    if (status == MS_DONE) { /* no overlap */
      msLayerClose(layer);
      if (chartType == MS_CHART_TYPE_PIE)
        pieLayerCleanupDynamicDiameter(layer);
      return MS_SUCCESS;
    } else if (status != MS_SUCCESS) {
      msLayerClose(layer);
      if (chartType == MS_CHART_TYPE_PIE)
        pieLayerCleanupDynamicDiameter(layer);
      return MS_FAILURE;
    }
    switch (chartType) {
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
      return MS_FAILURE; /*shouldn't be here anyways*/
    }

    msLayerClose(layer);

    if (chartType == MS_CHART_TYPE_PIE)
      pieLayerCleanupDynamicDiameter(layer);
  }
  return status;
}

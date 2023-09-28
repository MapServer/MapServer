/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose: RFC48 implementation of geometry transformations for styling
 * Author:   Thomas Bonfort , Camptocamp (thomas.bonfort at camptocamp.com)
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
#include "mapthread.h"

extern int yyparse(parseObj *p);

void msStyleSetGeomTransform(styleObj *s, char *transform) {
  msFree(s->_geomtransform.string);
  if (!transform) {
    s->_geomtransform.type = MS_GEOMTRANSFORM_NONE;
    s->_geomtransform.string = NULL;
    return;
  }
  s->_geomtransform.string = msStrdup(transform);
  if (!strncasecmp("start", transform, 5)) {
    s->_geomtransform.type = MS_GEOMTRANSFORM_START;
  } else if (!strncasecmp("end", transform, 3)) {
    s->_geomtransform.type = MS_GEOMTRANSFORM_END;
  } else if (!strncasecmp("vertices", transform, 8)) {
    s->_geomtransform.type = MS_GEOMTRANSFORM_VERTICES;
  } else if (!strncasecmp("bbox", transform, 4)) {
    s->_geomtransform.type = MS_GEOMTRANSFORM_BBOX;
  } else if (!strncasecmp("labelpnt", transform, 8)) {
    s->_geomtransform.type = MS_GEOMTRANSFORM_LABELPOINT;
  } else if (!strncasecmp("labelpoly", transform, 9)) {
    s->_geomtransform.type = MS_GEOMTRANSFORM_LABELPOLY;
  } else if (!strncasecmp("labelcenter", transform, 11)) {
    s->_geomtransform.type = MS_GEOMTRANSFORM_LABELCENTER;
  } else if (!strncasecmp("centroid", transform, 8)) {
    s->_geomtransform.type = MS_GEOMTRANSFORM_CENTROID;
  } else {
    s->_geomtransform.type = MS_GEOMTRANSFORM_NONE;
    msSetError(MS_MISCERR, "unknown transform expression",
               "msStyleSetGeomTransform()");
    msFree(s->_geomtransform.string);
    s->_geomtransform.string = NULL;
  }
}

/*
 * return a copy of the geometry transform expression
 * returned char* must be freed by the caller
 */
char *msStyleGetGeomTransform(styleObj *s) {
  return msStrdup(s->_geomtransform.string);
}

double calcOrientation(pointObj *p1, pointObj *p2) {
  double theta;
  theta = atan2(p2->x - p1->x, p2->y - p1->y);
  return MS_RAD_TO_DEG * (theta - MS_PI2);
}

double calcMidAngle(pointObj *p1, pointObj *p2, pointObj *p3) {
  pointObj p1n;
  double dx12, dy12, dx23, dy23, l12, l23;

  /* We treat both segments as vector 1-2 and vector 2-3 and
   * compute their dx,dy and length
   */
  dx12 = p2->x - p1->x;
  dy12 = p2->y - p1->y;
  l12 = sqrt(dx12 * dx12 + dy12 * dy12);
  dx23 = p3->x - p2->x;
  dy23 = p3->y - p2->y;
  l23 = sqrt(dx23 * dx23 + dy23 * dy23);

  /* Normalize length of vector 1-2 to same as length of vector 2-3 */
  if (l12 > 0.0) {
    p1n.x = p2->x - dx12 * (l23 / l12);
    p1n.y = p2->y - dy12 * (l23 / l12);
  } else
    p1n = *p2; /* segment 1-2 is 0-length, use segment 2-3 for orientation */

  /* Return the orientation defined by the sum of the normalized vectors */
  return calcOrientation(&p1n, p3);
}

/*
 * RFC48 implementation:
 *  - transform the original shapeobj
 *  - use the styleObj to render the transformed shapeobj
 */
int msDrawTransformedShape(mapObj *map, imageObj *image, shapeObj *shape,
                           styleObj *style, double scalefactor) {
  int type = style->_geomtransform.type;
  int i, j, status = MS_SUCCESS;
  switch (type) {
  case MS_GEOMTRANSFORM_END: /*render point on last vertex only*/
    for (j = 0; j < shape->numlines; j++) {
      lineObj *line = &(shape->line[j]);
      pointObj *p = &(line->point[line->numpoints - 1]);
      if (p->x < 0 || p->x > image->width || p->y < 0 || p->y > image->height)
        continue;
      if (style->autoangle == MS_TRUE && line->numpoints > 1) {
        style->angle = calcOrientation(&(line->point[line->numpoints - 2]), p);
      }
      status = msDrawMarkerSymbol(map, image, p, style, scalefactor);
    }
    break;
  case MS_GEOMTRANSFORM_START: /*render point on first vertex only*/
    for (j = 0; j < shape->numlines; j++) {
      lineObj *line = &(shape->line[j]);
      pointObj *p = &(line->point[0]);
      /*skip if outside image*/
      if (p->x < 0 || p->x > image->width || p->y < 0 || p->y > image->height)
        continue;
      if (style->autoangle == MS_TRUE && line->numpoints > 1) {
        style->angle = calcOrientation(p, &(line->point[1]));
      }
      status = msDrawMarkerSymbol(map, image, p, style, scalefactor);
    }
    break;
  case MS_GEOMTRANSFORM_VERTICES:
    for (j = 0; j < shape->numlines; j++) {
      lineObj *line = &(shape->line[j]);
      for (i = 1; i < line->numpoints - 1; i++) {
        pointObj *p = &(line->point[i]);
        /*skip points outside image*/
        if (p->x < 0 || p->x > image->width || p->y < 0 || p->y > image->height)
          continue;
        if (style->autoangle == MS_TRUE) {
          style->angle = calcMidAngle(&(line->point[i - 1]), &(line->point[i]),
                                      &(line->point[i + 1]));
        }
        status = msDrawMarkerSymbol(map, image, p, style, scalefactor);
      }
    }
    break;
  case MS_GEOMTRANSFORM_BBOX: {
    shapeObj bbox;
    lineObj bbox_line;
    pointObj bbox_points[5];
    int padding = MS_MAX(style->width, style->size) +
                  3; /* so clipped shape does not extent into image */

    /*create a shapeObj representing the bounding box (clipped by the image
     * size)*/
    bbox.numlines = 1;
    bbox.line = &bbox_line;
    bbox.line->numpoints = 5;
    bbox.line->point = bbox_points;

    msComputeBounds(shape);
    bbox_points[0].x = bbox_points[4].x = bbox_points[1].x =
        (shape->bounds.minx < -padding) ? -padding : shape->bounds.minx;
    /* cppcheck-suppress unreadVariable */
    bbox_points[2].x = bbox_points[3].x =
        (shape->bounds.maxx > image->width + padding) ? image->width + padding
                                                      : shape->bounds.maxx;
    bbox_points[0].y = bbox_points[4].y = bbox_points[3].y =
        (shape->bounds.miny < -padding) ? -padding : shape->bounds.miny;
    /* cppcheck-suppress unreadVariable */
    bbox_points[1].y = bbox_points[2].y =
        (shape->bounds.maxy > image->height + padding) ? image->height + padding
                                                       : shape->bounds.maxy;
    status = msDrawShadeSymbol(map, image, &bbox, style, scalefactor);
  } break;
  case MS_GEOMTRANSFORM_CENTROID: {
    double unused; /*used by centroid function*/
    pointObj centroid;
    if (MS_SUCCESS ==
        msGetPolygonCentroid(shape, &centroid, &unused, &unused)) {
      status = msDrawMarkerSymbol(map, image, &centroid, style, scalefactor);
    }
  } break;
  case MS_GEOMTRANSFORM_EXPRESSION: {
    int status;
    shapeObj *tmpshp;
    parseObj p;

    p.shape = shape; /* set a few parser globals (hence the lock) */
    p.expr = &(style->_geomtransform);

    if (p.expr->tokens == NULL) { /* this could happen if drawing originates
                                     from legend code (#5193) */
      status = msTokenizeExpression(p.expr, NULL, NULL);
      if (status != MS_SUCCESS) {
        msSetError(MS_MISCERR, "Unable to tokenize expression.",
                   "msDrawTransformedShape()");
        return MS_FAILURE;
      }
    }

    p.expr->curtoken = p.expr->tokens; /* reset */
    p.type = MS_PARSE_TYPE_SHAPE;

    status = yyparse(&p);
    if (status != 0) {
      msSetError(MS_PARSEERR, "Failed to process shape expression: %s",
                 "msDrawTransformedShape", style->_geomtransform.string);
      return MS_FAILURE;
    }
    tmpshp = p.result.shpval;

    switch (tmpshp->type) {
    case MS_SHAPE_POINT:
    case MS_SHAPE_POLYGON:
      /* cppcheck-suppress unreadVariable */
      status = msDrawShadeSymbol(map, image, tmpshp, style, scalefactor);
      break;
    case MS_SHAPE_LINE:
      /* cppcheck-suppress unreadVariable */
      status = msDrawLineSymbol(map, image, tmpshp, style, scalefactor);
      break;
    }

    msFreeShape(tmpshp);
    msFree(tmpshp);
  } break;
  case MS_GEOMTRANSFORM_LABELPOINT:
  case MS_GEOMTRANSFORM_LABELPOLY:
    break;
  default:
    msSetError(MS_MISCERR, "unknown geomtransform", "msDrawTransformedShape()");
    return MS_FAILURE;
  }
  return status;
}

/*
 * RFC89 implementation:
 *  - transform directly the shapeobj
 */
int msGeomTransformShape(mapObj *map, layerObj *layer, shapeObj *shape) {
  int i;
  expressionObj *e = &layer->_geomtransform;

#ifdef USE_V8_MAPSCRIPT
  if (!map->v8context) {
    msV8CreateContext(map);
    if (!map->v8context) {
      msSetError(MS_V8ERR, "Unable to create v8 context.",
                 "msGeomTransformShape()");
      return MS_FAILURE;
    }
  }

  msV8ContextSetLayer(map, layer);
#endif

  switch (e->type) {
  case MS_GEOMTRANSFORM_EXPRESSION: {
    int status;
    shapeObj *tmpshp;
    parseObj p;

    p.shape = shape; /* set a few parser globals (hence the lock) */
    p.expr = e;
    p.expr->curtoken = p.expr->tokens; /* reset */
    p.type = MS_PARSE_TYPE_SHAPE;
    p.dblval = map->cellsize * (msInchesPerUnit(map->units, 0) /
                                msInchesPerUnit(layer->units, 0));
    p.dblval2 = 0;
    /* data_cellsize is only set with contour layer */
    if (layer->connectiontype == MS_CONTOUR) {
      const char *value =
          msLookupHashTable(&layer->metadata, "__data_cellsize__");
      if (value)
        p.dblval2 = atof(value);
    }

    status = yyparse(&p);
    if (status != 0) {
      msSetError(MS_PARSEERR, "Failed to process shape expression: %s",
                 "msGeomTransformShape()", e->string);
      return MS_FAILURE;
    }

    tmpshp = p.result.shpval;

    for (i = 0; i < shape->numlines; i++)
      free(shape->line[i].point);
    shape->numlines = 0;
    if (shape->line)
      free(shape->line);
    shape->line = NULL;
    shape->type = tmpshp->type; /* might have been a change (e.g. centerline) */

    for (i = 0; i < tmpshp->numlines; i++)
      msAddLine(shape, &(tmpshp->line[i])); /* copy each line */

    msFreeShape(tmpshp);
    msFree(tmpshp);
  } break;
  default:
    msSetError(MS_MISCERR, "unknown geomtransform", "msGeomTransformShape()");
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/*****************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Content Dependent Legend rendering support
 * Author:   Thomas Bonfort (tbonfort@terriscope.fr)
 *
 ******************************************************************************
 * Copyright (c) 1996-2013 Regents of the University of Minnesota.
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

void initStyleHitTests(styleObj *s, style_hittest *sh, int default_status) {
  (void)s;
  sh->status = default_status;
}

void initLabelHitTests(labelObj *l, label_hittest *lh, int default_status) {
  int i;
  lh->stylehits = msSmallCalloc(l->numstyles, sizeof(style_hittest));
  lh->status = default_status;
  for (i = 0; i < l->numstyles; i++) {
    initStyleHitTests(l->styles[i], &lh->stylehits[i], default_status);
  }
}

void initClassHitTests(classObj *c, class_hittest *ch, int default_status) {
  int i;
  ch->stylehits = msSmallCalloc(c->numstyles, sizeof(style_hittest));
  ch->labelhits = msSmallCalloc(c->numlabels, sizeof(label_hittest));
  ch->status = default_status;
  for (i = 0; i < c->numstyles; i++) {
    initStyleHitTests(c->styles[i], &ch->stylehits[i], default_status);
  }
  for (i = 0; i < c->numlabels; i++) {
    initLabelHitTests(c->labels[i], &ch->labelhits[i], default_status);
  }
}

void initLayerHitTests(layerObj *l, layer_hittest *lh) {
  int i, default_status;
  lh->classhits = msSmallCalloc(l->numclasses, sizeof(class_hittest));

  switch (l->type) {
  case MS_LAYER_POLYGON:
  case MS_LAYER_POINT:
  case MS_LAYER_LINE:
  case MS_LAYER_ANNOTATION:
    default_status = 0; /* needs testing */
    break;
  default:
    default_status = 1; /* no hittesting needed, use traditional mode */
    break;
  }
  lh->status = default_status;
  for (i = 0; i < l->numclasses; i++) {
    initClassHitTests(l->class[i], &lh -> classhits[i], default_status);
  }
}
void initMapHitTests(mapObj *map, map_hittest *mh) {
  int i;
  mh->layerhits = msSmallCalloc(map->numlayers, sizeof(layer_hittest));
  for (i = 0; i < map->numlayers; i++) {
    initLayerHitTests(GET_LAYER(map, i), &mh->layerhits[i]);
  }
}

void freeLabelHitTests(labelObj *l, label_hittest *lh) {
  (void)l;
  free(lh->stylehits);
}

void freeClassHitTests(classObj *c, class_hittest *ch) {
  int i;
  for (i = 0; i < c->numlabels; i++) {
    freeLabelHitTests(c->labels[i], &ch->labelhits[i]);
  }
  free(ch->stylehits);
  free(ch->labelhits);
}
void freeLayerHitTests(layerObj *l, layer_hittest *lh) {
  int i;
  for (i = 0; i < l->numclasses; i++) {
    freeClassHitTests(l->class[i], &lh -> classhits[i]);
  }
  free(lh->classhits);
}
void freeMapHitTests(mapObj *map, map_hittest *mh) {
  int i;
  for (i = 0; i < map->numlayers; i++) {
    freeLayerHitTests(GET_LAYER(map, i), &mh->layerhits[i]);
  }
  free(mh->layerhits);
}

int msHitTestShape(mapObj *map, layerObj *layer, shapeObj *shape, int drawmode,
                   class_hittest *hittest) {
  int i;
  classObj *cp = layer->class[shape->classindex];
  if (MS_DRAW_FEATURES(drawmode)) {
    for (i = 0; i < cp->numstyles; i++) {
      styleObj *sp = cp->styles[i];
      if (msScaleInBounds(map->scaledenom, sp->minscaledenom,
                          sp->maxscaledenom)) {
        hittest->stylehits[i].status = 1;
      }
    }
  }
  if (MS_DRAW_LABELS(drawmode)) {
    for (i = 0; i < cp->numlabels; i++) {
      labelObj *l = cp->labels[i];
      if (msGetLabelStatus(map, layer, shape, l) == MS_ON) {
        int s;
        hittest->labelhits[i].status = 1;
        for (s = 0; s < l->numstyles; s++) {
          hittest->labelhits[i].stylehits[s].status = 1;
        }
      }
    }
  }
  return MS_SUCCESS;
}

int msHitTestLayer(mapObj *map, layerObj *layer, layer_hittest *hittest) {
  int status;
#ifdef USE_GEOS
  shapeObj searchpoly;
#endif
  if (!msLayerIsVisible(map, layer)) {
    hittest->status = 0;
    return MS_SUCCESS;
  }
  if (layer->type == MS_LAYER_LINE || layer->type == MS_LAYER_POLYGON ||
      layer->type == MS_LAYER_POINT || layer->type == MS_LAYER_ANNOTATION) {
    int maxfeatures = msLayerGetMaxFeaturesToDraw(layer, NULL);
    int annotate = msEvalContext(map, layer, layer->labelrequires);
    shapeObj shape;
    int nclasses, featuresdrawn = 0;
    int *classgroup;
    rectObj searchrect;
    int minfeaturesize = -1;
    if (map->scaledenom > 0) {
      if ((layer->labelmaxscaledenom != -1) &&
          (map->scaledenom >= layer->labelmaxscaledenom))
        annotate = MS_FALSE;
      if ((layer->labelminscaledenom != -1) &&
          (map->scaledenom < layer->labelminscaledenom))
        annotate = MS_FALSE;
    }

    status = msLayerOpen(layer);
    if (status != MS_SUCCESS)
      return MS_FAILURE;

    /* build item list */
    status = msLayerWhichItems(layer, MS_FALSE, NULL);

    if (status != MS_SUCCESS) {
      msLayerClose(layer);
      return MS_FAILURE;
    }

    /* identify target shapes */
    if (layer->transform == MS_TRUE) {
      searchrect = map->extent;
      if ((map->projection.numargs > 0) && (layer->projection.numargs > 0))
        msProjectRect(
            &map->projection, &layer->projection,
            &searchrect); /* project the searchrect to source coords */
    } else {
      searchrect.minx = searchrect.miny = 0;
      searchrect.maxx = map->width - 1;
      searchrect.maxy = map->height - 1;
    }
#ifdef USE_GEOS
    msInitShape(&searchpoly);
    msRectToPolygon(searchrect, &searchpoly);
#endif

    status = msLayerWhichShapes(layer, searchrect, MS_FALSE);
    if (status == MS_DONE) { /* no overlap */
#ifdef USE_GEOS
      msFreeShape(&searchpoly);
#endif
      msLayerClose(layer);
      hittest->status = 0;
      return MS_SUCCESS;
    } else if (status != MS_SUCCESS) {
#ifdef USE_GEOS
      msFreeShape(&searchpoly);
#endif
      msLayerClose(layer);
      return MS_FAILURE;
    }

    /* step through the target shapes */
    msInitShape(&shape);

    nclasses = 0;
    classgroup = NULL;
    if (layer->classgroup && layer->numclasses > 0)
      classgroup = msAllocateValidClassGroups(layer, &nclasses);

    if (layer->minfeaturesize > 0)
      minfeaturesize = Pix2LayerGeoref(map, layer, layer->minfeaturesize);

    while ((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
      int drawmode = MS_DRAWMODE_FEATURES;
#ifdef USE_GEOS
      if (!msGEOSIntersects(&shape, &searchpoly)) {
        msFreeShape(&shape);
        continue;
      }
#else
      if (shape.type == MS_SHAPE_POLYGON) {
        msClipPolygonRect(&shape, map->extent);
      } else {
        msClipPolylineRect(&shape, map->extent);
      }
      if (shape.numlines == 0) {
        msFreeShape(&shape);
        continue;
      }
#endif
      /* Check if the shape size is ok to be drawn, we need to clip */
      if ((shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) &&
          (minfeaturesize > 0)) {
        msTransformShape(&shape, map->extent, map->cellsize, NULL);
        msComputeBounds(&shape);
        if (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE) {
          msFreeShape(&shape);
          continue;
        }
      }

      shape.classindex =
          msShapeGetClass(layer, map, &shape, classgroup, nclasses);
      if ((shape.classindex == -1) ||
          (layer->class[shape.classindex] -> status == MS_OFF)) {
        msFreeShape(&shape);
        continue;
      }
      hittest->classhits[shape.classindex].status = 1;
      hittest->status = 1;

      if (maxfeatures >= 0 && featuresdrawn >= maxfeatures) {
        msFreeShape(&shape);
        status = MS_DONE;
        break;
      }
      featuresdrawn++;

      if (annotate && layer->class[shape.classindex] -> numlabels > 0) {
        drawmode |= MS_DRAWMODE_LABELS;
      }

      status = msHitTestShape(
          map, layer, &shape, drawmode,
          &hittest->classhits[shape.classindex]); /* all styles  */
      msFreeShape(&shape);
    }

#ifdef USE_GEOS
    msFreeShape(&searchpoly);
#endif

    if (classgroup)
      msFree(classgroup);

    if (status != MS_DONE) {
      msLayerClose(layer);
      return MS_FAILURE;
    }
    msLayerClose(layer);
    return MS_SUCCESS;

  } else {
    /* we don't hittest these layers, skip as they already have been initialized
     */
    return MS_SUCCESS;
  }
}
int msHitTestMap(mapObj *map, map_hittest *hittest) {
  int i, status;
  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  status = msCalculateScale(map->extent, map->units, map->width, map->height,
                            map->resolution, &map->scaledenom);
  if (status != MS_SUCCESS) {
    return MS_FAILURE;
  }
  for (i = 0; i < map->numlayers; i++) {
    layerObj *lp = map->layers[i];
    status = msHitTestLayer(map, lp, &hittest->layerhits[i]);
    if (status != MS_SUCCESS) {
      return MS_FAILURE;
    }
  }
  return MS_SUCCESS;
}

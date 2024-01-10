/*****************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Content Dependant Legend rendering support
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

#ifndef HITTEST_H
#define HITTEST_H

typedef struct {
  int status;
} style_hittest;

typedef struct {
  style_hittest *stylehits;
  int status;
} label_hittest;

typedef struct {
  style_hittest *stylehits;
  label_hittest *labelhits;
  int status;
} class_hittest;

typedef struct {
  class_hittest *classhits;
  int status;
} layer_hittest;

typedef struct map_hittest {
  layer_hittest *layerhits;
} map_hittest;

int msHitTestMap(mapObj *map, map_hittest *hittest);
int msHitTestLayer(mapObj *map, layerObj *layer, layer_hittest *hittest);
void initStyleHitTests(styleObj *s, style_hittest *sh, int default_status);
void initLabelHitTests(labelObj *l, label_hittest *lh, int default_status);
void initClassHitTests(classObj *c, class_hittest *ch, int default_status);
void initLayerHitTests(layerObj *l, layer_hittest *lh);
void initMapHitTests(mapObj *map, map_hittest *mh);
void freeLabelHitTests(labelObj *l, label_hittest *lh);
void freeClassHitTests(classObj *c, class_hittest *ch);
void freeLayerHitTests(layerObj *l, layer_hittest *lh);
void freeMapHitTests(mapObj *map, map_hittest *mh);

#endif

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Labeling Implementation.
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
 ****************************************************************************/

/*
** maplabel.c: Routines to enable text drawing, BITMAP or TRUETYPE.
*/

#include <float.h>

#include "mapserver.h"
#include "fontcache.h"

#include "cpl_vsi.h"
#include "cpl_string.h"





void initTextPath(textPathObj *ts) {
  memset(ts,0,sizeof(*ts));
}

int WARN_UNUSED msLayoutTextSymbol(mapObj *map, textSymbolObj *ts, textPathObj *tgret);

#if defined(USE_EXTENDED_DEBUG) && 0
static void msDebugTextPath(textSymbolObj *ts) {
  int i;
  msDebug("text: %s\n",ts->annotext);
  if(ts->textpath) {
    for(i=0;i<ts->textpath->numglyphs; i++) {
      glyphObj *g = &ts->textpath->glyphs[i];
      msDebug("glyph %d: pos: %f %f\n",g->glyph->key.codepoint,g->pnt.x,g->pnt.y);
    }
  } else {
    msDebug("no glyphs\n");
  }
  msDebug("\n=========================================\n");
}
#endif

int msComputeTextPath(mapObj *map, textSymbolObj *ts) {
  textPathObj *tgret = msSmallMalloc(sizeof(textPathObj));
  assert(ts->annotext && *ts->annotext);
  initTextPath(tgret);
  ts->textpath = tgret;
  tgret->absolute = 0;
  tgret->glyph_size = ts->label->size * ts->scalefactor;
  tgret->glyph_size = MS_MAX(tgret->glyph_size, ts->label->minsize * ts->resolutionfactor);
  tgret->glyph_size = MS_NINT(MS_MIN(tgret->glyph_size, ts->label->maxsize * ts->resolutionfactor));
  tgret->line_height = ceil(tgret->glyph_size * 1.33);
  return msLayoutTextSymbol(map,ts,tgret);
}

void initTextSymbol(textSymbolObj *ts) {
  memset(ts,0,sizeof(*ts));
}

void freeTextPath(textPathObj *tp) {
  free(tp->glyphs);
  if(tp->bounds.poly) {
    free(tp->bounds.poly->point);
    free(tp->bounds.poly);
  }
}
void freeTextSymbol(textSymbolObj *ts) {
  if(ts->textpath) {
    freeTextPath(ts->textpath);
    free(ts->textpath);
  }
  if(ts->label->numstyles) {
    if(ts->style_bounds) {
      int i;
      for(i=0;i<ts->label->numstyles; i++) {
        if(ts->style_bounds[i]) {
          if(ts->style_bounds[i]->poly) {
            free(ts->style_bounds[i]->poly->point);
            free(ts->style_bounds[i]->poly);
          }
          free(ts->style_bounds[i]);
        }
      }
      free(ts->style_bounds);
    }
  }
  free(ts->annotext);
  if(freeLabel(ts->label) == MS_SUCCESS) {
    free(ts->label);
  }
}

void msCopyTextPath(textPathObj *dst, textPathObj *src) {
  int i;
  *dst = *src;
  if(src->bounds.poly) {
    dst->bounds.poly = msSmallMalloc(sizeof(lineObj));
    dst->bounds.poly->numpoints = src->bounds.poly->numpoints;
    dst->bounds.poly->point = msSmallMalloc(src->bounds.poly->numpoints * sizeof(pointObj));
    for(i=0; i<src->bounds.poly->numpoints; i++) {
      dst->bounds.poly->point[i] = src->bounds.poly->point[i];
    }
  } else {
    dst->bounds.poly = NULL;
  }
  if(dst->numglyphs > 0) {
    dst->glyphs = msSmallMalloc(dst->numglyphs * sizeof(glyphObj));
    for(i=0; i<dst->numglyphs; i++)
      dst->glyphs[i] = src->glyphs[i];
  }
}

void msCopyTextSymbol(textSymbolObj *dst, textSymbolObj *src) {
  *dst = *src;
  MS_REFCNT_INCR(src->label);
  dst->annotext = msStrdup(src->annotext);
  if(dst->textpath) {
    dst->textpath = msSmallMalloc(sizeof(textPathObj));
    msCopyTextPath(dst->textpath,src->textpath);
  }
  if(dst->style_bounds) {
    int i;
    dst->style_bounds = msSmallCalloc(src->label->numstyles, sizeof(label_bounds*));
    for(i=0; i<src->label->numstyles; i++) {
      if(src->style_bounds[i]) {
        dst->style_bounds[i] = msSmallMalloc(sizeof(label_bounds));
        copyLabelBounds(dst->style_bounds[i], src->style_bounds[i]);
      }
    }
  }
}

static int labelNeedsDeepCopy(labelObj *label) {
  int i;
  if(label->numbindings > 0) return MS_TRUE;
  for(i=0; i<label->numstyles; i++) {
    if(label->styles[i]->numbindings>0) {
      return MS_TRUE;
    }
  }
  return MS_FALSE;
}

void msPopulateTextSymbolForLabelAndString(textSymbolObj *ts, labelObj *l, char *string, double scalefactor, double resolutionfactor, label_cache_mode cache) {
  if(cache == duplicate_always) {
    ts->label = msSmallMalloc(sizeof(labelObj));
    initLabel(ts->label);
    msCopyLabel(ts->label,l);
  } else if(cache == duplicate_never) {
    ts->label = l;
    MS_REFCNT_INCR(l);
  } else if(cache == duplicate_if_needed && labelNeedsDeepCopy(l)) {
    ts->label = msSmallMalloc(sizeof(labelObj));
    initLabel(ts->label);
    msCopyLabel(ts->label,l);
  } else {
    ts->label = l;
    MS_REFCNT_INCR(l);
  }
  ts->resolutionfactor = resolutionfactor;
  ts->scalefactor = scalefactor;
  ts->annotext = string; /* we take the ownership of the annotation text */
  ts->rotation = l->angle * MS_DEG_TO_RAD;
}

int msAddLabelGroup(mapObj *map, imageObj *image, layerObj* layer, int classindex, shapeObj *shape, pointObj *point, double featuresize)
{
  int l,s, priority;
  labelCacheSlotObj *cacheslot;

  labelCacheMemberObj *cachePtr=NULL;
  layerObj *layerPtr=NULL;
  classObj *classPtr=NULL;
  int numtextsymbols = 0;
  textSymbolObj **textsymbols, *ts;
  int layerindex = layer->index;

  // We cannot use GET_LAYER here because in drawQuery the drawing may happen
  // on a temp layer only.
  layerPtr = layer;
  classPtr = layer->class[classindex];

  if(classPtr->numlabels == 0) return MS_SUCCESS; /* not an error just nothing to do */

  /* check that the label intersects the layer mask */
  if(layerPtr->mask) {
    int maskLayerIdx = msGetLayerIndex(map,layerPtr->mask);
    layerObj *maskLayer = GET_LAYER(map,maskLayerIdx);
    unsigned char *alphapixptr;
    if(maskLayer->maskimage && MS_IMAGE_RENDERER(maskLayer->maskimage)->supports_pixel_buffer) {
      rasterBufferObj rb;
      int x,y;
      memset(&rb,0,sizeof(rasterBufferObj));
      if(UNLIKELY(MS_FAILURE == MS_IMAGE_RENDERER(maskLayer->maskimage)->getRasterBufferHandle(maskLayer->maskimage,&rb))) {
        return MS_FAILURE;
      }
      x = MS_NINT(point->x);
      y = MS_NINT(point->y);
      /* Using label repeatdistance, we might have a point with x/y below 0. See #4764 */
      if (x >= 0 && x < rb.width && y >= 0 && y < rb.height) {
        assert(rb.type == MS_BUFFER_BYTE_RGBA);
        alphapixptr = rb.data.rgba.a+rb.data.rgba.row_step*y + rb.data.rgba.pixel_step*x;
        if(!*alphapixptr) {
          /* label point does not intersect mask */
          return MS_SUCCESS;
        }
      } else {
        return MS_SUCCESS; /* label point does not intersect image extent, we cannot know if it intersects
                             mask, so we discard it (#5237)*/
      }
    } else {
      msSetError(MS_MISCERR, "Layer (%s) references references a mask layer, but the selected renderer does not support them", "msAddLabelGroup()", layerPtr->name);
      return (MS_FAILURE);
    }
  }

  textsymbols = msSmallMalloc(classPtr->numlabels * sizeof(textSymbolObj*));

  for(l=0; l<classPtr->numlabels; l++) {
    labelObj *lbl = classPtr->labels[l];
    char *annotext;
    if(msGetLabelStatus(map,layerPtr,shape,lbl) == MS_OFF) {
      continue;
    }
    annotext = msShapeGetLabelAnnotation(layerPtr,shape,lbl);
    if(!annotext) {
      for(s=0;s<lbl->numstyles;s++) {
        if(lbl->styles[s]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT)
          break; /* we have a "symbol only label, so we shouldn't skip this label */
      }
      if(s == lbl->numstyles) {
        continue; /* no anno text, and no label symbols */
      }
    }
    ts = msSmallMalloc(sizeof(textSymbolObj));
    initTextSymbol(ts);
    msPopulateTextSymbolForLabelAndString(ts,lbl,annotext,layerPtr->scalefactor,image->resolutionfactor, 1);

    if(annotext && *annotext && lbl->autominfeaturesize && featuresize > 0) {
      if(UNLIKELY(MS_FAILURE == msComputeTextPath(map,ts))) {
        freeTextSymbol(ts);
        free(ts);
        return MS_FAILURE;
      }
      if(featuresize < (ts->textpath->bounds.bbox.maxx - ts->textpath->bounds.bbox.minx)) {
        /* feature is too big to be drawn, skip it */
        freeTextSymbol(ts);
        free(ts);
        continue;
      }
    }
    textsymbols[numtextsymbols] = ts;
    numtextsymbols++;
  }

  if(numtextsymbols == 0) {
    free(textsymbols);
    return MS_SUCCESS;
  }

  /* Validate label priority value and get ref on label cache for it */
  priority = classPtr->labels[0]->priority; /* take priority from the first label */
  if (priority < 1)
    priority = 1;
  else if (priority > MS_MAX_LABEL_PRIORITY)
    priority = MS_MAX_LABEL_PRIORITY;

  cacheslot = &(map->labelcache.slots[priority-1]);

  if(cacheslot->numlabels == cacheslot->cachesize) { /* just add it to the end */
    cacheslot->labels = (labelCacheMemberObj *) realloc(cacheslot->labels, sizeof(labelCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
    MS_CHECK_ALLOC(cacheslot->labels, sizeof(labelCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT), MS_FAILURE);
    cacheslot->cachesize += MS_LABELCACHEINCREMENT;
  }

  cachePtr = &(cacheslot->labels[cacheslot->numlabels]);

  cachePtr->layerindex = layerindex; /* so we can get back to this *raw* data if necessary */

  cachePtr->classindex = classindex;

  cachePtr->point = *point; /* the actual label point */

  cachePtr->leaderline = NULL;
  cachePtr->leaderbbox = NULL;

  cachePtr->markerid = -1;

  cachePtr->status = MS_FALSE;

  if(layerPtr->type == MS_LAYER_POINT && classPtr->numstyles > 0) {
    /* cache the marker placement, it's already on the map */
    /* TO DO: at the moment only checks the bottom style, perhaps should check all of them */
    /* #2347: after RFC-24 classPtr->styles could be NULL so we check it */
    double w, h;
    if(msGetMarkerSize(map, classPtr->styles[0], &w, &h, layerPtr->scalefactor) != MS_SUCCESS)
      return(MS_FAILURE);

    if(cacheslot->nummarkers == cacheslot->markercachesize) { /* just add it to the end */
      cacheslot->markers = (markerCacheMemberObj *) realloc(cacheslot->markers, sizeof(markerCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
      MS_CHECK_ALLOC(cacheslot->markers, sizeof(markerCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT), MS_FAILURE);
      cacheslot->markercachesize+=MS_LABELCACHEINCREMENT;
    }

    cacheslot->markers[cacheslot->nummarkers].bounds.minx = (point->x - .5 * w);
    cacheslot->markers[cacheslot->nummarkers].bounds.miny = (point->y - .5 * h);
    cacheslot->markers[cacheslot->nummarkers].bounds.maxx = cacheslot->markers[cacheslot->nummarkers].bounds.minx + (w-1);
    cacheslot->markers[cacheslot->nummarkers].bounds.maxy = cacheslot->markers[cacheslot->nummarkers].bounds.miny + (h-1);
    cacheslot->markers[cacheslot->nummarkers].id = cacheslot->numlabels;

    cachePtr->markerid = cacheslot->nummarkers;
    cacheslot->nummarkers++;
  }
  cachePtr->textsymbols = textsymbols;
  cachePtr->numtextsymbols = numtextsymbols;

  cacheslot->numlabels++;

  return(MS_SUCCESS);
}

int msAddLabel(mapObj *map, imageObj *image, labelObj *label, int layerindex, int classindex,
    shapeObj *shape, pointObj *point, double featuresize, textSymbolObj *ts)
{
  int i;
  labelCacheSlotObj *cacheslot;
  labelCacheMemberObj *cachePtr=NULL;
  char *annotext = NULL;
  layerObj *layerPtr;
  classObj *classPtr;

  layerPtr=GET_LAYER(map,layerindex);
  assert(layerPtr);

  assert(classindex < layerPtr->numclasses);
  classPtr = layerPtr->class[classindex];

  assert(label);

  if(ts)
    annotext = ts->annotext;
  else if(shape)
    annotext = msShapeGetLabelAnnotation(layerPtr,shape,label);

  if(!annotext) {
    /* check if we have a labelpnt style */
    for(i=0; i<label->numstyles; i++) {
      if(label->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT)
        break;
    }
    if(i==label->numstyles) {
      /* label has no text or marker symbols */
      if(ts) {
        freeTextSymbol(ts);
        free(ts);
      }
      return MS_SUCCESS;
    }
  }

  if(classPtr->leader) {
    if(ts && ts->textpath && ts->textpath->absolute) {
      msSetError(MS_MISCERR, "LEADERs are not supported on ANGLE FOLLOW labels", "msAddLabel()");
      return MS_FAILURE;
    }
  }
  /* check that the label intersects the layer mask */

  if (layerPtr->mask) {
    int maskLayerIdx = msGetLayerIndex(map, layerPtr->mask);
    layerObj *maskLayer = GET_LAYER(map, maskLayerIdx);
    unsigned char *alphapixptr;
    if (maskLayer->maskimage && MS_IMAGE_RENDERER(maskLayer->maskimage)->supports_pixel_buffer) {
      rasterBufferObj rb;
      memset(&rb, 0, sizeof (rasterBufferObj));
      if(UNLIKELY(MS_FAILURE == MS_IMAGE_RENDERER(maskLayer->maskimage)->getRasterBufferHandle(maskLayer->maskimage, &rb))) {
        return MS_FAILURE;
      }
      assert(rb.type == MS_BUFFER_BYTE_RGBA);
      if (point) {
        int x = MS_NINT(point->x);
        int y = MS_NINT(point->y);
        /* Using label repeatdistance, we might have a point with x/y below 0. See #4764 */
        if (x >= 0 && x < rb.width && y >= 0 && y < rb.height) {
          alphapixptr = rb.data.rgba.a+rb.data.rgba.row_step*y + rb.data.rgba.pixel_step*x;
          if(!*alphapixptr) {
            /* label point does not intersect mask */
            if(ts) {
              freeTextSymbol(ts);
              free(ts);
            }
            return MS_SUCCESS;
          }
        } else {
          return MS_SUCCESS; /* label point does not intersect image extent, we cannot know if it intersects
                                mask, so we discard it (#5237)*/
        }
      } else if (ts && ts->textpath) {
        int i = 0;
        for (i = 0; i < ts->textpath->numglyphs; i++) {
          int x = MS_NINT(ts->textpath->glyphs[i].pnt.x);
          int y = MS_NINT(ts->textpath->glyphs[i].pnt.y);
          if (x >= 0 && x < rb.width && y >= 0 && y < rb.height) {
            alphapixptr = rb.data.rgba.a + rb.data.rgba.row_step * y + rb.data.rgba.pixel_step*x;
            if (!*alphapixptr) {
              freeTextSymbol(ts);
              free(ts);
              return MS_SUCCESS;
            }
          } else {
            freeTextSymbol(ts);
            free(ts);
            return MS_SUCCESS; /* label point does not intersect image extent, we cannot know if it intersects
                                  mask, so we discard it (#5237)*/
          }
        }
      }
    } else {
      msSetError(MS_MISCERR, "Layer (%s) references references a mask layer, but the selected renderer does not support them", "msAddLabel()", layerPtr->name);
      return (MS_FAILURE);
    }
  }

  if(!ts) {
    ts = msSmallMalloc(sizeof(textSymbolObj));
    initTextSymbol(ts);
    msPopulateTextSymbolForLabelAndString(ts,label,annotext,layerPtr->scalefactor,image->resolutionfactor, 1);
  }

  if(annotext && label->autominfeaturesize && featuresize > 0) {
    if(!ts->textpath) {
      if(UNLIKELY(MS_FAILURE == msComputeTextPath(map,ts)))
        return MS_FAILURE;
    }
    if(featuresize > (ts->textpath->bounds.bbox.maxx - ts->textpath->bounds.bbox.minx)) {
      /* feature is too big to be drawn, skip it */
      freeTextSymbol(ts);
      free(ts);
      return MS_SUCCESS;
    }
  }



  /* Validate label priority value and get ref on label cache for it */
  if (label->priority < 1)
    label->priority = 1;
  else if (label->priority > MS_MAX_LABEL_PRIORITY)
    label->priority = MS_MAX_LABEL_PRIORITY;

  cacheslot = &(map->labelcache.slots[label->priority-1]);

  if(cacheslot->numlabels == cacheslot->cachesize) { /* just add it to the end */
    cacheslot->labels = (labelCacheMemberObj *) realloc(cacheslot->labels, sizeof(labelCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
    MS_CHECK_ALLOC(cacheslot->labels, sizeof(labelCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT), MS_FAILURE);
    cacheslot->cachesize += MS_LABELCACHEINCREMENT;
  }

  cachePtr = &(cacheslot->labels[cacheslot->numlabels]);

  cachePtr->layerindex = layerindex; /* so we can get back to this *raw* data if necessary */
  cachePtr->classindex = classindex;
#ifdef include_deprecated
  if(shape) {
    cachePtr->shapetype = shape->type;
  } else {
    cachePtr->shapetype = MS_SHAPE_POINT;
  }
#endif

  cachePtr->leaderline = NULL;
  cachePtr->leaderbbox = NULL;

  /* Store the label point or the label path (Bug #1620) */
  if ( point ) {
    cachePtr->point = *point; /* the actual label point */
  } else {
    assert(ts && ts->textpath && ts->textpath->absolute && ts->textpath->numglyphs>0);
    /* Use the middle point of the labelpath for mindistance calculations */
    cachePtr->point = ts->textpath->glyphs[ts->textpath->numglyphs/2].pnt;
  }

  /* copy the label */
  cachePtr->numtextsymbols = 1;
  cachePtr->textsymbols = (textSymbolObj **) msSmallMalloc(sizeof(textSymbolObj*));
  cachePtr->textsymbols[0] = ts;
  cachePtr->markerid = -1;

  cachePtr->status = MS_FALSE;

  if(layerPtr->type == MS_LAYER_POINT && classPtr->numstyles > 0) { /* cache the marker placement, it's already on the map */
    double w, h;

    if(cacheslot->nummarkers == cacheslot->markercachesize) { /* just add it to the end */
      cacheslot->markers = (markerCacheMemberObj *) realloc(cacheslot->markers, sizeof(markerCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
      MS_CHECK_ALLOC(cacheslot->markers, sizeof(markerCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT), MS_FAILURE);
      cacheslot->markercachesize+=MS_LABELCACHEINCREMENT;
    }

    i = cacheslot->nummarkers;

    /* TO DO: at the moment only checks the bottom style, perhaps should check all of them */
    /* #2347: after RFC-24 classPtr->styles could be NULL so we check it */
    if(classPtr->styles != NULL) {
      if(msGetMarkerSize(map, classPtr->styles[0], &w, &h, layerPtr->scalefactor) != MS_SUCCESS)
        return(MS_FAILURE);
      cacheslot->markers[cacheslot->nummarkers].bounds.minx = (point->x - .5 * w);
      cacheslot->markers[cacheslot->nummarkers].bounds.miny = (point->y - .5 * h);
      cacheslot->markers[cacheslot->nummarkers].bounds.maxx = cacheslot->markers[cacheslot->nummarkers].bounds.minx + (w-1);
      cacheslot->markers[cacheslot->nummarkers].bounds.maxy = cacheslot->markers[cacheslot->nummarkers].bounds.miny + (h-1);
      cacheslot->markers[i].id = cacheslot->numlabels;

      cachePtr->markerid = i;

      cacheslot->nummarkers++;
    }
  }

  cacheslot->numlabels++;

  return(MS_SUCCESS);
}

/*
** Is a label completely in the image, reserving a gutter (in pixels) inside
** image for no labels (effectively making image larger. The gutter can be
** negative in cases where a label has a buffer around it.
*/
static int labelInImage(int width, int height, lineObj *lpoly, rectObj *bounds, int gutter)
{
  int j;

  /* do a bbox test first */
  if(bounds->minx >= gutter &&
      bounds->miny >= gutter &&
      bounds->maxx < width-gutter &&
      bounds->maxy < height-gutter) {
    return MS_TRUE;
  }

  if(lpoly) {
    for(j=1; j<lpoly->numpoints; j++) {
      if(lpoly->point[j].x < gutter) return(MS_FALSE);
      if(lpoly->point[j].x >= width-gutter) return(MS_FALSE);
      if(lpoly->point[j].y < gutter) return(MS_FALSE);
      if(lpoly->point[j].y >= height-gutter) return(MS_FALSE);
    }
  } else {
    /* if no poly, then return false as the boundong box intersected */
    return MS_FALSE;
  }

  return(MS_TRUE);
}

void insertRenderedLabelMember(mapObj *map, labelCacheMemberObj *cachePtr) {
  if(map->labelcache.num_rendered_members == map->labelcache.num_allocated_rendered_members) {
    if(map->labelcache.num_rendered_members == 0) {
      map->labelcache.num_allocated_rendered_members = 50;
    } else {
      map->labelcache.num_allocated_rendered_members *= 2;
    }
    map->labelcache.rendered_text_symbols = msSmallRealloc(map->labelcache.rendered_text_symbols,
            map->labelcache.num_allocated_rendered_members * sizeof(labelCacheMemberObj*));
  }
  map->labelcache.rendered_text_symbols[map->labelcache.num_rendered_members++] = cachePtr;
}

static inline int testSegmentLabelBBoxIntersection(const rectObj *leaderbbox, const pointObj *lp1,
    const pointObj *lp2, const label_bounds *test) {
  if(msRectOverlap(leaderbbox, &test->bbox)) {
    if(test->poly) {
      int pp;
      for(pp=1; pp<test->poly->numpoints; pp++) {
        if(msIntersectSegments(
            &(test->poly->point[pp-1]),
            &(test->poly->point[pp]),
            lp1,lp2) ==  MS_TRUE) {
          return(MS_FALSE);
        }
      }
    } else {
      pointObj tp1,tp2;
      tp1.x = test->bbox.minx;
      tp1.y = test->bbox.miny;
      tp2.x = test->bbox.minx;
      tp2.y = test->bbox.maxy;
      if(msIntersectSegments(lp1,lp2,&tp1,&tp2))
        return MS_FALSE;
      tp2.x = test->bbox.maxx;
      tp2.y = test->bbox.miny;
      if(msIntersectSegments(lp1,lp2,&tp1,&tp2))
        return MS_FALSE;
      tp1.x = test->bbox.maxx;
      tp1.y = test->bbox.maxy;
      tp2.x = test->bbox.minx;
      tp2.y = test->bbox.maxy;
      if(msIntersectSegments(lp1,lp2,&tp1,&tp2))
        return MS_FALSE;
      tp2.x = test->bbox.maxx;
      tp2.y = test->bbox.miny;
      if(msIntersectSegments(lp1,lp2,&tp1,&tp2))
        return MS_FALSE;
    }
  }
  return MS_TRUE;
}

int msTestLabelCacheLeaderCollision(mapObj *map, pointObj *lp1, pointObj *lp2) {
  int p;
  rectObj leaderbbox;
  leaderbbox.minx = MS_MIN(lp1->x,lp2->x);
  leaderbbox.maxx = MS_MAX(lp1->x,lp2->x);
  leaderbbox.miny = MS_MIN(lp1->y,lp2->y);
  leaderbbox.maxy = MS_MAX(lp1->y,lp2->y);
  for(p=0; p<map->labelcache.num_rendered_members; p++) {
    labelCacheMemberObj *curCachePtr= map->labelcache.rendered_text_symbols[p];
    if(msRectOverlap(&leaderbbox, &(curCachePtr->bbox))) {
    /* leaderbbox interesects with the curCachePtr's global bbox */
      int t;
      for(t=0; t<curCachePtr->numtextsymbols; t++) {
        int s;
        textSymbolObj *ts = curCachePtr->textsymbols[t];
        /* check for intersect with textpath */
        if(ts->textpath && testSegmentLabelBBoxIntersection(&leaderbbox, lp1, lp2, &ts->textpath->bounds) == MS_FALSE) {
          return MS_FALSE;
        }
        /* check for intersect with label's labelpnt styles */
        if(ts->style_bounds) {
          for(s=0; s<ts->label->numstyles; s++) {
            if(ts->label->styles[s]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT) {
              if(testSegmentLabelBBoxIntersection(&leaderbbox, lp1,lp2, ts->style_bounds[s]) == MS_FALSE) {
                return MS_FALSE;
              }
            }
          }
        }
      }
      if(curCachePtr->leaderbbox) {
        if(msIntersectSegments(lp1,lp2,&(curCachePtr->leaderline->point[0]), &(curCachePtr->leaderline->point[1])) ==  MS_TRUE) {
          return MS_FALSE;
        }
      }
    }
  }
  return MS_TRUE;
}

/* msTestLabelCacheCollisions()
**
** Compares label bounds (in *bounds) against labels already drawn and markers from cache and
** returns MS_FALSE if it collides with another label, or collides with a marker.
**
** This function is used by the various msDrawLabelCacheXX() implementations.

int msTestLabelCacheCollisions(mapObj *map, labelCacheMemberObj *cachePtr, label_bounds *bounds,
              int current_priority, int current_label);
*/
int msTestLabelCacheCollisions(mapObj *map, labelCacheMemberObj *cachePtr, label_bounds *lb,
        int current_priority, int current_label)
{
  labelCacheObj *labelcache = &(map->labelcache);
  int i, p, ll;

  /*
   * Check against image bounds first
   */
  if(!cachePtr->textsymbols[0]->label->partials) {
    if(labelInImage(map->width, map->height, lb->poly, &lb->bbox, labelcache->gutter) == MS_FALSE) {
      return MS_FALSE;
    }
  }

  /* Compare against all rendered markers from this priority level and higher.
  ** Labels can overlap their own marker and markers from lower priority levels
  */
  for (p=current_priority; p < MS_MAX_LABEL_PRIORITY; p++) {
    labelCacheSlotObj *markerslot;
    markerslot = &(labelcache->slots[p]);

    for ( ll = 0; ll < markerslot->nummarkers; ll++ ) {
      if ( !(p == current_priority && current_label == markerslot->markers[ll].id ) ) {  /* labels can overlap their own marker */
        if ( intersectLabelPolygons(NULL, &markerslot->markers[ll].bounds, lb->poly, &lb->bbox ) == MS_TRUE ) {
          return MS_FALSE;
        }
      }
    }
  }

  for(p=0; p<labelcache->num_rendered_members; p++) {
    labelCacheMemberObj *curCachePtr= labelcache->rendered_text_symbols[p];
    if(msRectOverlap(&curCachePtr->bbox,&lb->bbox)) {
      for(i=0; i<curCachePtr->numtextsymbols; i++) {
        int j;
        textSymbolObj *ts = curCachePtr->textsymbols[i];
        if(ts->textpath && intersectLabelPolygons(ts->textpath->bounds.poly, &ts->textpath->bounds.bbox, lb->poly, &lb->bbox) == MS_TRUE ) {
          return MS_FALSE;
        }
        if(ts->style_bounds) {
          for(j=0;j<ts->label->numstyles;j++) {
            if(ts->style_bounds[j] && ts->label->styles[j]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT) {
              if(intersectLabelPolygons(ts->style_bounds[j]->poly, &ts->style_bounds[j]->bbox,
                  lb->poly, &lb->bbox)) {
                return MS_FALSE;
              }
            }
          }
        }
      }
    }
    if(curCachePtr->leaderline) {
      if(testSegmentLabelBBoxIntersection(curCachePtr->leaderbbox, &curCachePtr->leaderline->point[0],
          &curCachePtr->leaderline->point[1], lb) == MS_FALSE) {
        return MS_FALSE;
      }
    }
  }
  return MS_TRUE;
}

/* utility function to get the rect of a string outside of a rendering loop, i.e.
 without going through textpath layouts */
int msGetStringSize(mapObj *map, labelObj *label, int size, char *string, rectObj *r) {
  textSymbolObj ts;
  double lsize = label->size;
  initTextSymbol(&ts);
  label->size = size;
  msPopulateTextSymbolForLabelAndString(&ts,label,msStrdup(string),1,1,0);
  if(UNLIKELY(MS_FAILURE == msGetTextSymbolSize(map,&ts,r)))
    return MS_FAILURE;
  label->size = lsize;
  freeTextSymbol(&ts);
  return MS_SUCCESS;
}

int msInitFontSet(fontSetObj *fontset)
{
  fontset->filename = NULL;

  /* fontset->fonts = NULL; */
  initHashTable(&(fontset->fonts));

  fontset->numfonts = 0;
  fontset->map = NULL;
  return( 0 );
}

int msFreeFontSet(fontSetObj *fontset)
{
  if (fontset->filename)
    free(fontset->filename);
  fontset->filename = NULL;
  if (&(fontset->fonts))
    msFreeHashItems(&(fontset->fonts));
  /* fontset->fonts = NULL; */
  fontset->numfonts = 0;

  return( 0 );
}

int msLoadFontSet(fontSetObj *fontset, mapObj *map)
{
  VSILFILE *stream;
  const char* line;
  char alias[64], file1[MS_PATH_LENGTH], file2[MS_PATH_LENGTH];
  char *path;
  char szPath[MS_MAXPATHLEN];
  int i;
  int bFullPath = 0;

  if(fontset->numfonts != 0) /* already initialized */
    return(0);

  if(!fontset->filename)
    return(0);

  fontset->map = (mapObj *)map;

  path = msGetPath(fontset->filename);

  /* fontset->fonts = msCreateHashTable(); // create font hash */
  /* if(!fontset->fonts) { */
  /* msSetError(MS_HASHERR, "Error initializing font hash.", "msLoadFontSet()"); */
  /* return(-1); */
  /* } */

  stream = VSIFOpenL( msBuildPath(szPath, fontset->map->mappath, fontset->filename), "rb");
  if(!stream) {
    msSetError(MS_IOERR, "Error opening fontset %s.", "msLoadFontset()",
               fontset->filename);
    free(path);
    return(-1);
  }

  i = 0;
  while( (line = CPLReadLineL(stream)) != NULL ) { /* while there's something to load */

    if(line[0] == '#' || line[0] == '\n' || line[0] == '\r' || line[0] == ' ')
      continue; /* skip comments and blank lines */

    sscanf(line,"%s %s", alias,  file1);

    if (!(*file1) || !(*alias) || (strlen(file1) <= 0))
      continue;

    bFullPath = 0;
#if defined(_WIN32) && !defined(__CYGWIN__)
    if (file1[0] == '\\' || (strlen(file1) > 1 && (file1[1] == ':')))
      bFullPath = 1;
#else
    if(file1[0] == '/')
      bFullPath = 1;
#endif

    if(bFullPath) { /* already full path */
      msInsertHashTable(&(fontset->fonts), alias, file1);
    } else {
      snprintf(file2, sizeof(file2), "%s%s", path, file1);
      /* msInsertHashTable(fontset->fonts, alias, file2); */

      /*
      ** msBuildPath is use here, but if we have to save the fontset file
      ** the msBuildPath must be done everywhere the fonts are used and
      ** removed here.
      */
      msInsertHashTable(&(fontset->fonts), alias,
                        msBuildPath(szPath, fontset->map->mappath, file2));

    }

    i++;
  }

  fontset->numfonts = i;
  VSIFCloseL(stream); /* close the file */
  free(path);

  return(0);
}


int msGetTextSymbolSize(mapObj *map, textSymbolObj *ts, rectObj *r) {
  if(!ts->textpath) {
    if(UNLIKELY(MS_FAILURE == msComputeTextPath(map,ts)))
      return MS_FAILURE;
  }
  *r = ts->textpath->bounds.bbox;
  return MS_SUCCESS;
}
/*
** Note: All these routines assume a reference point at the LL corner of the text. GD's
** bitmapped fonts use UL and this is compensated for. Note the rect is relative to the
** LL corner of the text to be rendered, this is first line for TrueType fonts.
*/

#define MARKER_SLOP 2

pointObj get_metrics(pointObj *p, int position, textPathObj *tp, int ox, int oy, double rotation, int buffer, label_bounds *bounds)
{
  pointObj q = {0}; // initialize
  double x1=0, y1=0, x2=0, y2=0;
  double sin_a,cos_a;
  double w, h, x, y;

  w = tp->bounds.bbox.maxx - tp->bounds.bbox.minx;
  h = tp->bounds.bbox.maxy - tp->bounds.bbox.miny;

  switch(position) {
    case MS_UL:
      x1 = -w - ox;
      y1 = -oy;
      break;
    case MS_UC:
      x1 = -(w/2.0);
      y1 = -oy - MARKER_SLOP;
      break;
    case MS_UR:
      x1 = ox;
      y1 = -oy;
      break;
    case MS_CL:
      x1 = -w - ox - MARKER_SLOP;
      if(oy > 0 && tp->numlines == 1)
        y1 = oy;
      else
        y1 = (h/2.0);
      break;
    case MS_CC:
      x1 = -(w/2.0) + ox;
      y1 = (h/2.0) + oy;
      break;
    case MS_CR:
      x1 = ox + MARKER_SLOP;
      if(oy > 0 && tp->numlines == 1)
        y1 = oy;
      else
        y1 = (h/2.0);
      break;
    case MS_LL:
      x1 = -w - ox;
      y1 = h + oy;
      break;
    case MS_LC:
      x1 = -(w/2.0);
      y1 = h + oy + MARKER_SLOP;
      break;
    case MS_LR:
      x1 = ox;
      y1 = h + oy;
      break;
  }

  if(rotation) {
    sin_a = sin(rotation);
    cos_a = cos(rotation);

    x = x1 - tp->bounds.bbox.minx;
    y = tp->bounds.bbox.maxy - y1;
    q.x = p->x + (x * cos_a - (y) * sin_a);
    q.y = p->y - (x * sin_a + (y) * cos_a);

    if(bounds) {
      x2 = x1 - buffer; /* ll */
      y2 = y1 + buffer;
      bounds->poly->point[0].x = p->x + (x2 * cos_a - (-y2) * sin_a);
      bounds->poly->point[0].y = p->y - (x2 * sin_a + (-y2) * cos_a);

      x2 = x1 - buffer; /* ul */
      y2 = y1 - h - buffer;
      bounds->poly->point[1].x = p->x + (x2 * cos_a - (-y2) * sin_a);
      bounds->poly->point[1].y = p->y - (x2 * sin_a + (-y2) * cos_a);

      x2 = x1 + w + buffer; /* ur */
      y2 = y1 - h - buffer;
      bounds->poly->point[2].x = p->x + (x2 * cos_a - (-y2) * sin_a);
      bounds->poly->point[2].y = p->y - (x2 * sin_a + (-y2) * cos_a);

      x2 = x1 + w + buffer; /* lr */
      y2 = y1 + buffer;
      bounds->poly->point[3].x = p->x + (x2 * cos_a - (-y2) * sin_a);
      bounds->poly->point[3].y = p->y - (x2 * sin_a + (-y2) * cos_a);

      bounds->poly->point[4].x = bounds->poly->point[0].x;
      bounds->poly->point[4].y = bounds->poly->point[0].y;
      fastComputeBounds(bounds->poly,&bounds->bbox);
    }
  }
  else {
    q.x = p->x + x1 - tp->bounds.bbox.minx;
    q.y = p->y + y1 ;

    if(bounds) {
      /* no rotation, we only need to return a bbox */
      bounds->poly = NULL;


      bounds->bbox.minx = q.x - buffer;
      bounds->bbox.maxy = q.y + buffer + tp->bounds.bbox.maxy;
      bounds->bbox.maxx = q.x + w + buffer;
      bounds->bbox.miny = bounds->bbox.maxy - h - buffer*2;
    }
  }
  return(q);
}

int intersectTextSymbol(textSymbolObj *ts, label_bounds *lb) {
  if(ts->textpath && ts->textpath->absolute) {
    if(intersectLabelPolygons(lb->poly,&lb->bbox,ts->textpath->bounds.poly,&ts->textpath->bounds.bbox))
      return MS_TRUE;
  }
  if(ts->style_bounds) {
    int s;
    for(s=0; s<ts->label->numstyles; s++) {
      if(ts->style_bounds[s] && ts->label->styles[s]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT &&
          intersectLabelPolygons(lb->poly,&lb->bbox,ts->style_bounds[s]->poly, &ts->style_bounds[s]->bbox))
        return MS_TRUE;
    }
  }
  return MS_FALSE;
}

/*
** Variation on msIntersectPolygons. Label polygons aren't like shapefile polygons. They
** have no holes, and often do have overlapping parts (i.e. road symbols).
*/
int intersectLabelPolygons(lineObj *l1, rectObj *r1, lineObj *l2, rectObj *r2)
{
  int v1,v2;
  pointObj *point;
  lineObj *p1,*p2,sp1,sp2;
  pointObj pnts1[5],pnts2[5];


  /* STEP 0: check bounding boxes */
  if(!msRectOverlap(r1,r2)) { /* from alans@wunderground.com */
    return(MS_FALSE);
  }
  if(!l1 && !l2)
    return MS_TRUE;

  if(!l1) {
    p1 = &sp1;
    p1->numpoints = 5;
    p1->point = pnts1;
    pnts1[0].x = pnts1[1].x = pnts1[4].x = r1->minx;
    pnts1[2].x = pnts1[3].x = r1->maxx;
    pnts1[0].y = pnts1[3].y = pnts1[4].y = r1->miny;
    pnts1[1].y = pnts1[2].y = r1->maxy;
  } else {
    p1 = l1;
  }
  if(!l2) {
    p2 = &sp2;
    p2->numpoints = 5;
    p2->point = pnts2;
    pnts2[0].x = pnts2[1].x = pnts2[4].x = r2->minx;
    pnts2[2].x = pnts2[3].x = r2->maxx;
    pnts2[0].y = pnts2[3].y = pnts2[4].y = r2->miny;
    pnts2[1].y = pnts2[2].y = r2->maxy;
  } else {
    p2 = l2;
  }

  /* STEP 1: look for intersecting line segments */
  for(v1=1; v1<p1->numpoints; v1++)
    for(v2=1; v2<p2->numpoints; v2++)
      if(msIntersectSegments(&(p1->point[v1-1]), &(p1->point[v1]), &(p2->point[v2-1]), &(p2->point[v2])) ==  MS_TRUE) {
        return(MS_TRUE);
    }

  /* STEP 2: polygon one completely contains two (only need to check one point from each part) */
  point = &(p2->point[0]);
  if(msPointInPolygon(point, p1) == MS_TRUE) /* ok, the point is in a polygon */
    return(MS_TRUE);

  /* STEP 3: polygon two completely contains one (only need to check one point from each part) */
  point = &(p1->point[0]);
  if(msPointInPolygon(point, p2) == MS_TRUE) /* ok, the point is in a polygon */
    return(MS_TRUE);

  return(MS_FALSE);
}

/* For MapScript, exactly the same the msInsertStyle */
int msInsertLabelStyle(labelObj *label, styleObj *style, int nStyleIndex)
{
  int i;

  if (!style) {
    msSetError(MS_CHILDERR, "Can't insert a NULL Style", "msInsertLabelStyle()");
    return -1;
  }

  /* Ensure there is room for a new style */
  if (msGrowLabelStyles(label) == NULL) {
    return -1;
  }
  /* Catch attempt to insert past end of styles array */
  else if (nStyleIndex >= label->numstyles) {
    msSetError(MS_CHILDERR, "Cannot insert style beyond index %d", "insertLabelStyle()", label->numstyles-1);
    return -1;
  } else if (nStyleIndex < 0) { /* Insert at the end by default */
    label->styles[label->numstyles]=style;
    MS_REFCNT_INCR(style);
    label->numstyles++;
    return label->numstyles-1;
  } else if (nStyleIndex >= 0 && nStyleIndex < label->numstyles) {
    /* Move styles existing at the specified nStyleIndex or greater */
    /* to a higher nStyleIndex */
    for (i=label->numstyles-1; i>=nStyleIndex; i--) {
      label->styles[i+1] = label->styles[i];
    }
    label->styles[nStyleIndex]=style;
    MS_REFCNT_INCR(style);
    label->numstyles++;
    return nStyleIndex;
  } else {
    msSetError(MS_CHILDERR, "Invalid nStyleIndex", "insertLabelStyle()");
    return -1;
  }
}

/**
 * Move the style up inside the array of styles.
 */
int msMoveLabelStyleUp(labelObj *label, int nStyleIndex)
{
  styleObj *psTmpStyle = NULL;
  if (label && nStyleIndex < label->numstyles && nStyleIndex >0) {
    psTmpStyle = (styleObj *)malloc(sizeof(styleObj));
    initStyle(psTmpStyle);

    msCopyStyle(psTmpStyle, label->styles[nStyleIndex]);

    msCopyStyle(label->styles[nStyleIndex],
                label->styles[nStyleIndex-1]);

    msCopyStyle(label->styles[nStyleIndex-1], psTmpStyle);

    return(MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveLabelStyleUp()",
             nStyleIndex);
  return (MS_FAILURE);
}


/**
 * Move the style down inside the array of styles.
 */
int msMoveLabelStyleDown(labelObj *label, int nStyleIndex)
{
  styleObj *psTmpStyle = NULL;

  if (label && nStyleIndex < label->numstyles-1 && nStyleIndex >=0) {
    psTmpStyle = (styleObj *)malloc(sizeof(styleObj));
    initStyle(psTmpStyle);

    msCopyStyle(psTmpStyle, label->styles[nStyleIndex]);

    msCopyStyle(label->styles[nStyleIndex],
                label->styles[nStyleIndex+1]);

    msCopyStyle(label->styles[nStyleIndex+1], psTmpStyle);

    return(MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveLabelStyleDown()",
             nStyleIndex);
  return (MS_FAILURE);
}

/**
 * Delete the style identified by the index and shift
 * styles that follows the deleted style.
 */
int msDeleteLabelStyle(labelObj *label, int nStyleIndex)
{
  int i = 0;
  if (label && nStyleIndex < label->numstyles && nStyleIndex >=0) {
    if (freeStyle(label->styles[nStyleIndex]) == MS_SUCCESS)
      msFree(label->styles[nStyleIndex]);
    for (i=nStyleIndex; i< label->numstyles-1; i++) {
      label->styles[i] = label->styles[i+1];
    }
    label->styles[label->numstyles-1] = NULL;
    label->numstyles--;
    return(MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msDeleteLabelStyle()",
             nStyleIndex);
  return (MS_FAILURE);
}

styleObj *msRemoveLabelStyle(labelObj *label, int nStyleIndex)
{
  int i;
  styleObj *style;
  if (nStyleIndex < 0 || nStyleIndex >= label->numstyles) {
    msSetError(MS_CHILDERR, "Cannot remove style, invalid nStyleIndex %d", "removeLabelStyle()", nStyleIndex);
    return NULL;
  } else {
    style=label->styles[nStyleIndex];
    for (i=nStyleIndex; i<label->numstyles-1; i++) {
      label->styles[i]=label->styles[i+1];
    }
    label->styles[label->numstyles-1]=NULL;
    label->numstyles--;
    MS_REFCNT_DECR(style);
    return style;
  }
}

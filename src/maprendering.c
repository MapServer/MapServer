/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Rendering utility functions
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
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

#include <assert.h>

#include "mapserver.h"
#include "mapcopy.h"
#include "fontcache.h"

void computeSymbolStyle(symbolStyleObj *s, styleObj *src, symbolObj *symbol,
                        double scalefactor, double resolutionfactor) {
  double default_size;
  double target_size;
  double style_size;

  default_size = msSymbolGetDefaultSize(symbol);
  style_size = (src->size == -1) ? default_size : src->size;

  INIT_SYMBOL_STYLE(*s);
  if (symbol->type == MS_SYMBOL_PIXMAP) {
    s->color = s->outlinecolor = NULL;
  } else if (symbol->filled || symbol->type == MS_SYMBOL_TRUETYPE) {
    if (MS_VALID_COLOR(src->color))
      s->color = &src->color;
    if (MS_VALID_COLOR(src->outlinecolor))
      s->outlinecolor = &src->outlinecolor;
  } else {
    if (MS_VALID_COLOR(src->color))
      s->outlinecolor = &(src->color);
    else if (MS_VALID_COLOR(src->outlinecolor))
      s->outlinecolor = &(src->outlinecolor);
    s->color = NULL;
  }

  target_size = style_size * scalefactor;
  target_size = MS_MAX(target_size, src->minsize * resolutionfactor);
  target_size = MS_MIN(target_size, src->maxsize * resolutionfactor);
  s->scale = target_size / default_size;
  s->gap = src->gap * target_size / style_size;

  if (s->outlinecolor) {
    s->outlinewidth = src->width * scalefactor;
    s->outlinewidth = MS_MAX(s->outlinewidth, src->minwidth * resolutionfactor);
    s->outlinewidth = MS_MIN(s->outlinewidth, src->maxwidth * resolutionfactor);
  } else {
    s->outlinewidth = 0;
  }

  s->rotation = src->angle * MS_DEG_TO_RAD;
}

#define COMPARE_COLORS(a, b)                                                   \
  (((a).red == (b).red) && ((a).green == (b).green) &&                         \
   ((a).blue == (b).blue) && ((a).alpha == (b).alpha))

tileCacheObj *searchTileCache(imageObj *img, symbolObj *symbol,
                              symbolStyleObj *s, int width, int height) {
  tileCacheObj *cur = img->tilecache;
  while (cur != NULL) {
    if (cur->width == width && cur->height == height && cur->symbol == symbol &&
        cur->outlinewidth == s->outlinewidth && cur->rotation == s->rotation &&
        cur->scale == s->scale &&
        (!s->color || COMPARE_COLORS(cur->color, *s->color)) &&
        (!s->backgroundcolor ||
         COMPARE_COLORS(cur->backgroundcolor, *s->backgroundcolor)) &&
        (!s->outlinecolor ||
         COMPARE_COLORS(cur->outlinecolor, *s->outlinecolor)))
      return cur;
    cur = cur->next;
  }
  return NULL;
}

int preloadSymbol(symbolSetObj *symbolset, symbolObj *symbol,
                  rendererVTableObj *renderer) {
  (void)symbolset;
  switch (symbol->type) {
  case MS_SYMBOL_VECTOR:
  case MS_SYMBOL_ELLIPSE:
  case MS_SYMBOL_SIMPLE:
  case (MS_SYMBOL_TRUETYPE):
    break;
  case (MS_SYMBOL_SVG):
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
    return msPreloadSVGSymbol(symbol);
#else
    msSetError(MS_SYMERR, "SVG symbol support is not enabled.",
               "preloadSymbol()");
    return MS_FAILURE;
#endif
    break;
  case (MS_SYMBOL_PIXMAP): {
    if (!symbol->pixmap_buffer) {
      if (MS_SUCCESS != msPreloadImageSymbol(renderer, symbol))
        return MS_FAILURE;
    }
  } break;
  default:
    msSetError(MS_MISCERR, "unsupported symbol type %d", "preloadSymbol()",
               symbol->type);
    return MS_FAILURE;
  }
  return MS_SUCCESS;
}

/* add a cached tile to the current image's cache */
tileCacheObj *addTileCache(imageObj *img, imageObj *tile, symbolObj *symbol,
                           symbolStyleObj *style, int width, int height) {
  tileCacheObj *cachep;

  if (img->ntiles >=
      MS_IMAGECACHESIZE) { /* remove last element, size stays the same */
    cachep = img->tilecache;

    /*go to the before last cache object*/
    while (cachep->next && cachep->next->next)
      cachep = cachep->next;
    assert(cachep->next);

    /*free the last tile's data*/
    msFreeImage(cachep->next->image);

    /*reuse the last tile object*/
    /* make the cache point to the start of the list*/
    cachep->next->next = img->tilecache;
    /* point the global cache to the new first */
    img->tilecache = cachep->next;
    /* the before last cache is now last, so it has no successor*/
    cachep->next = NULL;

  } else {
    img->ntiles += 1;
    cachep = (tileCacheObj *)malloc(sizeof(tileCacheObj));
    MS_CHECK_ALLOC(cachep, sizeof(tileCacheObj), NULL);
    cachep->next = img->tilecache;
    img->tilecache = cachep;
  }

  cachep = img->tilecache;

  cachep->image = tile;
  cachep->outlinewidth = style->outlinewidth;
  cachep->scale = style->scale;
  cachep->rotation = style->rotation;
  if (style->color)
    MS_COPYCOLOR(&cachep->color, style->color);
  if (style->outlinecolor)
    MS_COPYCOLOR(&cachep->outlinecolor, style->outlinecolor);
  cachep->width = width;
  cachep->height = height;
  cachep->symbol = symbol;
  return (cachep);
}

/* helper function to center glyph on the desired point */
static int drawGlyphMarker(imageObj *img, face_element *face,
                           glyph_element *glyphc, double px, double py,
                           int size, double rotation, colorObj *clr,
                           colorObj *oclr, int olwidth) {
  double ox, oy;
  textPathObj path;
  glyphObj glyph;
  rendererVTableObj *renderer = img->format->vtable;
  if (!renderer->renderGlyphs)
    return MS_FAILURE;
  path.numglyphs = 1;
  glyph.glyph = glyphc;
  glyph.face = face;
  path.glyphs = &glyph;
  path.glyph_size = size;
  glyph.rot = rotation;
  ox = (glyphc->metrics.maxx + glyphc->metrics.minx) / 2.0;
  oy = (glyphc->metrics.maxy + glyphc->metrics.miny) / 2.0;
  if (rotation != 0) {
    double sina, cosa;
    double rox, roy;
    sina = sin(rotation);
    cosa = cos(rotation);
    rox = ox * cosa - oy * sina;
    roy = ox * sina + oy * cosa;
    px -= rox;
    py += roy;
    glyph.pnt.x = px;
    glyph.pnt.y = py;
  } else {
    glyph.pnt.x = px - ox;
    glyph.pnt.y = py + oy;
  }
  textSymbolObj ts;
  memset(&ts, 0, sizeof(ts));
  ts.textpath = &path;
  return renderer->renderGlyphs(img, &ts, clr, oclr, olwidth, MS_TRUE);
}

imageObj *getTile(imageObj *img, symbolObj *symbol, symbolStyleObj *s,
                  int width, int height, int seamlessmode) {
  tileCacheObj *tile;
  int status = MS_SUCCESS;
  rendererVTableObj *renderer = img->format->vtable;
  if (width == -1 || height == -1) {
    width = height = MS_MAX(symbol->sizex, symbol->sizey);
  }
  tile = searchTileCache(img, symbol, s, width, height);

  if (tile == NULL) {
    imageObj *tileimg;
    double p_x, p_y;
    tileimg = msImageCreate(width, height, img->format, NULL, NULL,
                            img->resolution, img->resolution, NULL);
    if (MS_UNLIKELY(!tileimg)) {
      return NULL;
    }
    if (!seamlessmode) {
      p_x = width / 2.0;
      p_y = height / 2.0;
      switch (symbol->type) {
      case (MS_SYMBOL_TRUETYPE): {
        unsigned int unicode;
        glyph_element *glyphc;
        face_element *face = msGetFontFace(symbol->font, &img->map->fontset);
        if (MS_UNLIKELY(!face)) {
          status = MS_FAILURE;
          break;
        }
        msUTF8ToUniChar(symbol->character, &unicode);
        unicode = msGetGlyphIndex(face, unicode);
        glyphc = msGetGlyphByIndex(face, MS_MAX(MS_NINT(s->scale), 1), unicode);
        if (MS_UNLIKELY(!glyphc)) {
          status = MS_FAILURE;
          break;
        }
        status = drawGlyphMarker(tileimg, face, glyphc, p_x, p_y, s->scale,
                                 s->rotation, s->color, s->outlinecolor,
                                 s->outlinewidth);
      } break;
      case (MS_SYMBOL_PIXMAP):
        status = msPreloadImageSymbol(renderer, symbol);
        if (MS_UNLIKELY(status == MS_FAILURE)) {
          break;
        }
        status = renderer->renderPixmapSymbol(tileimg, p_x, p_y, symbol, s);
        break;
      case (MS_SYMBOL_ELLIPSE):
        status = renderer->renderEllipseSymbol(tileimg, p_x, p_y, symbol, s);
        break;
      case (MS_SYMBOL_VECTOR):
        status = renderer->renderVectorSymbol(tileimg, p_x, p_y, symbol, s);
        break;

      case (MS_SYMBOL_SVG):
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
        status = msPreloadSVGSymbol(symbol);
        if (MS_LIKELY(status == MS_SUCCESS)) {
          if (renderer->supports_svg) {
            status = renderer->renderSVGSymbol(tileimg, p_x, p_y, symbol, s);
          } else {
            status = msRenderRasterizedSVGSymbol(tileimg, p_x, p_y, symbol, s);
          }
        }
#else
        msSetError(MS_SYMERR, "SVG symbol support is not enabled.",
                   "getTile()");
        status = MS_FAILURE;
#endif
        break;
      default:
        msSetError(MS_SYMERR, "Unknown symbol type %d", "getTile()",
                   symbol->type);
        status = MS_FAILURE;
        break;
      }
      if (MS_UNLIKELY(status == MS_FAILURE)) {
        msFreeImage(tileimg);
        return NULL;
      }
    } else {
      /*
       * in seamless mode, we render the the symbol 9 times on a 3x3 grid to
       * account for antialiasing blending from one tile to the next. We finally
       * keep the center tile
       */
      imageObj *tile3img =
          msImageCreate(width * 3, height * 3, img->format, NULL, NULL,
                        img->resolution, img->resolution, NULL);
      int i, j;
      rasterBufferObj tmpraster;
      for (i = 1; i <= 3; i++) {
        p_x = (i + 0.5) * width;
        for (j = 1; j <= 3; j++) {
          p_y = (j + 0.5) * height;
          switch (symbol->type) {
          case (MS_SYMBOL_TRUETYPE): {
            unsigned int unicode;
            glyph_element *glyphc;
            face_element *face =
                msGetFontFace(symbol->font, &img->map->fontset);
            if (MS_UNLIKELY(!face)) {
              status = MS_FAILURE;
              break;
            }
            msUTF8ToUniChar(symbol->character, &unicode);
            unicode = msGetGlyphIndex(face, unicode);
            glyphc =
                msGetGlyphByIndex(face, MS_MAX(MS_NINT(s->scale), 1), unicode);
            if (MS_UNLIKELY(!glyphc)) {
              status = MS_FAILURE;
              break;
            }
            status = drawGlyphMarker(tileimg, face, glyphc, p_x, p_y, s->scale,
                                     s->rotation, s->color, s->outlinecolor,
                                     s->outlinewidth);
          } break;
          case (MS_SYMBOL_PIXMAP):
            status = msPreloadImageSymbol(renderer, symbol);
            if (MS_UNLIKELY(status == MS_FAILURE)) {
              break;
            }
            status =
                renderer->renderPixmapSymbol(tile3img, p_x, p_y, symbol, s);
            break;
          case (MS_SYMBOL_ELLIPSE):
            status =
                renderer->renderEllipseSymbol(tile3img, p_x, p_y, symbol, s);
            break;
          case (MS_SYMBOL_VECTOR):
            status =
                renderer->renderVectorSymbol(tile3img, p_x, p_y, symbol, s);
            break;
          default:
            msSetError(MS_SYMERR,
                       "BUG: Seamless mode is only for vector symbols",
                       "getTile()");
            return NULL;
          }
          if (MS_UNLIKELY(status == MS_FAILURE)) {
            msFreeImage(tile3img);
            return NULL;
          }
        }
      }

      status = MS_IMAGE_RENDERER(tile3img)->getRasterBufferHandle(tile3img,
                                                                  &tmpraster);
      if (MS_UNLIKELY(status == MS_FAILURE)) {
        msFreeImage(tile3img);
        return NULL;
      }
      status = renderer->mergeRasterBuffer(tileimg, &tmpraster, 1.0, width,
                                           height, 0, 0, width, height);
      msFreeImage(tile3img);
    }
    if (MS_UNLIKELY(status == MS_FAILURE)) {
      msFreeImage(tileimg);
      return NULL;
    }
    tile = addTileCache(img, tileimg, symbol, s, width, height);
  }
  return tile->image;
}

int msImagePolylineMarkers(imageObj *image, shapeObj *p, symbolObj *symbol,
                           symbolStyleObj *style, double spacing,
                           double initialgap, int auto_angle) {
  rendererVTableObj *renderer = MS_IMAGE_RENDERER(image);
  int i, j;
  pointObj point;
  double original_rotation = style->rotation;
  double symbol_width, symbol_height;
  glyph_element *glyphc = NULL;
  face_element *face = NULL;
  int ret = MS_SUCCESS;
  if (symbol->type != MS_SYMBOL_TRUETYPE) {
    symbol_width = MS_MAX(1, symbol->sizex * style->scale);
    symbol_height = MS_MAX(1, symbol->sizey * style->scale);
  } else {
    unsigned int unicode;
    msUTF8ToUniChar(symbol->character, &unicode);
    face = msGetFontFace(symbol->font, &image->map->fontset);
    if (MS_UNLIKELY(!face))
      return MS_FAILURE;
    unicode = msGetGlyphIndex(face, unicode);
    glyphc = msGetGlyphByIndex(face, MS_MAX(MS_NINT(style->scale), 1), unicode);
    if (MS_UNLIKELY(!glyphc))
      return MS_FAILURE;
    symbol_width = glyphc->metrics.maxx - glyphc->metrics.minx;
    symbol_height = glyphc->metrics.maxy - glyphc->metrics.miny;
  }
  for (i = 0; i < p->numlines; i++) {
    int line_in = 0;
    double line_length = 0;
    double current_length;
    if (initialgap < 0) {
      current_length = spacing / 2.0; /* initial padding for each line */
    } else {
      current_length = initialgap; /* initial padding for each line */
    }
    for (j = 1; j < p->line[i].numpoints; j++) {
      double rx, ry, theta, length;
      length =
          sqrt((pow((p->line[i].point[j].x - p->line[i].point[j - 1].x), 2) +
                pow((p->line[i].point[j].y - p->line[i].point[j - 1].y), 2)));
      line_length += length;
      if (length == 0)
        continue;
      rx = (p->line[i].point[j].x - p->line[i].point[j - 1].x) / length;
      ry = (p->line[i].point[j].y - p->line[i].point[j - 1].y) / length;

      if (auto_angle) {
        theta = asin(ry);
        if (rx < 0) {
          theta += MS_PI;
        } else
          theta = -theta;
        style->rotation = original_rotation + theta;
      }
      while (current_length <= length) {

        point.x = p->line[i].point[j - 1].x + current_length * rx;
        point.y = p->line[i].point[j - 1].y + current_length * ry;
        if (symbol->anchorpoint_x != 0.5 || symbol->anchorpoint_y != 0.5) {
          double ox, oy;
          ox = (0.5 - symbol->anchorpoint_x) * symbol_width;
          oy = (0.5 - symbol->anchorpoint_y) * symbol_height;
          if (style->rotation != 0) {
            double sina, cosa;
            double rox, roy;
            sina = sin(-style->rotation);
            cosa = cos(-style->rotation);
            rox = ox * cosa - oy * sina;
            roy = ox * sina + oy * cosa;
            point.x += rox;
            point.y += roy;
          } else {
            point.x += ox;
            point.y += oy;
          }
        }

        /* if the point is not in the map extent, skip it. (POLYLINE_NO_CLIP) */
        if ((point.x < -(symbol_width) ||
             point.x > (image->width + symbol_width)) ||
            (point.y < -(symbol_height) ||
             point.y > (image->height + symbol_height))) {
          current_length += spacing;
          line_in = 1;
          continue;
        }

        switch (symbol->type) {
        case MS_SYMBOL_PIXMAP:
          ret = renderer->renderPixmapSymbol(image, point.x, point.y, symbol,
                                             style);
          break;
        case MS_SYMBOL_ELLIPSE:
          ret = renderer->renderEllipseSymbol(image, point.x, point.y, symbol,
                                              style);
          break;
        case MS_SYMBOL_VECTOR:
          ret = renderer->renderVectorSymbol(image, point.x, point.y, symbol,
                                             style);
          break;
        case MS_SYMBOL_TRUETYPE:
          ret = drawGlyphMarker(image, face, glyphc, point.x, point.y,
                                style->scale, style->rotation, style->color,
                                style->outlinecolor, style->outlinewidth);
          break;
        case (MS_SYMBOL_SVG):
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
          if (renderer->supports_svg) {
            ret = renderer->renderSVGSymbol(image, point.x, point.y, symbol,
                                            style);
          } else {
            ret = msRenderRasterizedSVGSymbol(image, point.x, point.y, symbol,
                                              style);
          }
#else
          msSetError(MS_SYMERR, "SVG symbol support is not enabled.",
                     "msImagePolylineMarkers()()");
          ret = MS_FAILURE;
#endif
          break;
        }
        if (ret != MS_SUCCESS)
          return ret;
        current_length += spacing;
        line_in = 1;
      }

      current_length -= length;
    }

    /*
     * if we couldn't place a symbol on the line and no initialgap was
     * specified,  add one now we don't add the symbol if the line is shorter
     * than the length of the symbol itself
     */
    if (initialgap < 0 && !line_in && line_length > symbol_width) {

      /* total lengths of beginnning and end of current segment */
      double before_length = 0, after_length = 0;

      /*optimize*/
      line_length /= 2.0;

      for (j = 1; j < p->line[i].numpoints; j++) {
        double length;
        length =
            sqrt((pow((p->line[i].point[j].x - p->line[i].point[j - 1].x), 2) +
                  pow((p->line[i].point[j].y - p->line[i].point[j - 1].y), 2)));
        after_length += length;
        if (after_length > line_length) {
          double rx, ry, theta;
          /* offset where the symbol should be drawn on the current
           * segment */
          double offset = line_length - before_length;

          rx = (p->line[i].point[j].x - p->line[i].point[j - 1].x) / length;
          ry = (p->line[i].point[j].y - p->line[i].point[j - 1].y) / length;
          if (auto_angle) {
            theta = asin(ry);
            if (rx < 0) {
              theta += MS_PI;
            } else
              theta = -theta;
            style->rotation = original_rotation + theta;
          }

          point.x = p->line[i].point[j - 1].x + offset * rx;
          point.y = p->line[i].point[j - 1].y + offset * ry;
          switch (symbol->type) {
          case MS_SYMBOL_PIXMAP:
            ret = renderer->renderPixmapSymbol(image, point.x, point.y, symbol,
                                               style);
            break;
          case MS_SYMBOL_ELLIPSE:
            ret = renderer->renderEllipseSymbol(image, point.x, point.y, symbol,
                                                style);
            break;
          case MS_SYMBOL_VECTOR:
            ret = renderer->renderVectorSymbol(image, point.x, point.y, symbol,
                                               style);
            break;
          case MS_SYMBOL_TRUETYPE:
            ret = drawGlyphMarker(image, face, glyphc, point.x, point.y,
                                  style->scale, style->rotation, style->color,
                                  style->outlinecolor, style->outlinewidth);
            break;
          case (MS_SYMBOL_SVG):
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
            if (renderer->supports_svg) {
              ret = renderer->renderSVGSymbol(image, point.x, point.y, symbol,
                                              style);
            } else {
              ret = msRenderRasterizedSVGSymbol(image, point.x, point.y, symbol,
                                                style);
            }
#else
            msSetError(MS_SYMERR, "SVG symbol support is not enabled.",
                       "msImagePolylineMarkers()()");
            ret = MS_FAILURE;
#endif
            break;
          }
          break; /* we have rendered the single marker for this line */
        }
        before_length += length;
      }
    }
  }
  return ret;
}

int msDrawLineSymbol(mapObj *map, imageObj *image, shapeObj *p, styleObj *style,
                     double scalefactor) {
  int status = MS_SUCCESS;
  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
      rendererVTableObj *renderer = image->format->vtable;
      symbolObj *symbol;
      shapeObj *offsetLine = p;
      int i;
      double width;
      double finalscalefactor;

      if (p->numlines == 0)
        return MS_SUCCESS;

      if (style->symbol >= map->symbolset.numsymbols || style->symbol < 0)
        return MS_SUCCESS; /* no such symbol, 0 is OK   */

      symbol = map->symbolset.symbol[style->symbol];
      /* store a reference to the renderer to be used for freeing */
      symbol->renderer = renderer;

      width = style->width * scalefactor;
      width = MS_MIN(width, style->maxwidth * image->resolutionfactor);
      width = MS_MAX(width, style->minwidth * image->resolutionfactor);
      if (style->width != 0) {
        finalscalefactor = width / style->width;
      } else {
        finalscalefactor = 1.0;
      }

      if (style->offsety == MS_STYLE_SINGLE_SIDED_OFFSET) {
        offsetLine = msOffsetPolyline(p, style->offsetx * finalscalefactor,
                                      MS_STYLE_SINGLE_SIDED_OFFSET);
      } else if (style->offsety == MS_STYLE_DOUBLE_SIDED_OFFSET) {
        offsetLine = msOffsetPolyline(p, style->offsetx * finalscalefactor,
                                      MS_STYLE_DOUBLE_SIDED_OFFSET);
      } else if (style->offsetx != 0 || style->offsety != 0) {
        offsetLine = msOffsetPolyline(p, style->offsetx * finalscalefactor,
                                      style->offsety * finalscalefactor);
      }
      if (style->symbol == 0 || (symbol->type == MS_SYMBOL_SIMPLE)) {
        strokeStyleObj s;
        s.linecap = style->linecap;
        s.linejoin = style->linejoin;
        s.linejoinmaxsize = style->linejoinmaxsize;
        s.antialiased = style->antialiased;
        s.width = width;
        s.patternlength = style->patternlength;
        for (i = 0; i < s.patternlength; i++)
          s.pattern[i] = style->pattern[i] * finalscalefactor;
        s.patternoffset = (style->initialgap <= 0)
                              ? 0
                              : (style->initialgap * finalscalefactor);

        if (MS_VALID_COLOR(style->color))
          s.color = &style->color;
        else if (MS_VALID_COLOR(style->outlinecolor))
          s.color = &style->outlinecolor;
        else {
          /* msSetError(MS_MISCERR,"no color defined for line
           * styling","msDrawLineSymbol()"); not really an error */
          status = MS_SUCCESS;
          goto line_cleanup;
        }
        status = renderer->renderLine(image, offsetLine, &s);
      } else {
        symbolStyleObj s;
        if (preloadSymbol(&map->symbolset, symbol, renderer) != MS_SUCCESS) {
          status = MS_FAILURE;
          goto line_cleanup;
        }

        INIT_SYMBOL_STYLE(s);
        computeSymbolStyle(&s, style, symbol, scalefactor,
                           image->resolutionfactor);
        s.style = style;

        /* compute a markerStyle and use it on the line */
        if (style->gap < 0) {
          /* special function that treats any other symbol used as a marker, not
           * a brush */
          status =
              msImagePolylineMarkers(image, offsetLine, symbol, &s, -s.gap,
                                     style->initialgap * finalscalefactor, 1);
        } else if (style->gap > 0) {
          status =
              msImagePolylineMarkers(image, offsetLine, symbol, &s, s.gap,
                                     style->initialgap * finalscalefactor, 0);
        } else {
          if (renderer->renderLineTiled != NULL) {
            int pw, ph;
            imageObj *tile = NULL;
            if (s.scale != 1) {
              pw = MS_NINT(symbol->sizex * s.scale);
              ph = MS_NINT(symbol->sizey * s.scale);
            } else {
              pw = symbol->sizex;
              ph = symbol->sizey;
            }
            if (pw < 1)
              pw = 1;
            if (ph < 1)
              ph = 1;
            tile = getTile(image, symbol, &s, pw, ph, 0);
            status = renderer->renderLineTiled(image, offsetLine, tile);
          } else {
            msSetError(MS_RENDERERERR,
                       "renderer does not support brushed lines",
                       "msDrawLineSymbol()");
            status = MS_FAILURE;
          }
        }
      }

    line_cleanup:
      if (offsetLine != p) {
        msFreeShape(offsetLine);
        msFree(offsetLine);
      }
      return status;
    } else if (MS_RENDERER_IMAGEMAP(image->format))
      msDrawLineSymbolIM(map, image, p, style, scalefactor);
    else {
      msSetError(MS_RENDERERERR, "unsupported renderer", "msDrawLineSymbol()");
      status = MS_FAILURE;
    }
  } else {
    status = MS_FAILURE;
  }
  return status;
}

int msDrawShadeSymbol(mapObj *map, imageObj *image, shapeObj *p,
                      styleObj *style, double scalefactor) {
  int ret = MS_SUCCESS;
  symbolObj *symbol;
  if (!p)
    return MS_SUCCESS;
  if (p->numlines <= 0)
    return MS_SUCCESS;

  if (style->symbol >= map->symbolset.numsymbols || style->symbol < 0)
    return MS_SUCCESS; /* no such symbol, 0 is OK */
  symbol = map->symbolset.symbol[style->symbol];

  /*
   * if only an outlinecolor was defined, and not a color,
   * switch to the line drawing function
   *
   * this behavior is kind of a mapfile hack, and must be
   * kept for backwards compatibility
   */
  if (symbol->type != MS_SYMBOL_PIXMAP && symbol->type != MS_SYMBOL_SVG) {
    if (!MS_VALID_COLOR(style->color)) {
      if (MS_VALID_COLOR(style->outlinecolor))
        return msDrawLineSymbol(map, image, p, style, scalefactor);
      else {
        /* just do nothing if no color has been set */
        return MS_SUCCESS;
      }
    }
  }
  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
      rendererVTableObj *renderer = image->format->vtable;
      shapeObj *offsetPolygon = NULL;
      /* store a reference to the renderer to be used for freeing */
      if (style->symbol)
        symbol->renderer = renderer;

      if (style->offsetx != 0 || style->offsety != 0) {
        if (style->offsety == MS_STYLE_SINGLE_SIDED_OFFSET) {
          offsetPolygon = msOffsetPolyline(p, style->offsetx * scalefactor,
                                           MS_STYLE_SINGLE_SIDED_OFFSET);
        } else if (style->offsety == MS_STYLE_DOUBLE_SIDED_OFFSET) {
          offsetPolygon = msOffsetPolyline(p, style->offsetx * scalefactor,
                                           MS_STYLE_DOUBLE_SIDED_OFFSET);
        } else {
          offsetPolygon = msOffsetPolyline(p, style->offsetx * scalefactor,
                                           style->offsety * scalefactor);
        }
      } else {
        offsetPolygon = p;
      }
      /* simple polygon drawing, without any specific symbol.
       * also draws an optional outline */
      if (style->symbol == 0 || symbol->type == MS_SYMBOL_SIMPLE) {
        ret = renderer->renderPolygon(image, offsetPolygon, &style->color);
        if (ret != MS_SUCCESS)
          goto cleanup;
        if (MS_VALID_COLOR(style->outlinecolor)) {
          strokeStyleObj s;
          INIT_STROKE_STYLE(s);
          s.color = &style->outlinecolor;
          s.color->alpha = style->color.alpha;
          s.width =
              (style->width == 0) ? scalefactor : style->width * scalefactor;
          s.width = MS_MIN(s.width, style->maxwidth);
          s.width = MS_MAX(s.width, style->minwidth);
          ret = renderer->renderLine(image, offsetPolygon, &s);
        }
        goto cleanup; /*finished plain polygon*/
      } else if (symbol->type == MS_SYMBOL_HATCH) {
        double width, spacing;
        double pattern[MS_MAXPATTERNLENGTH];
        int i;

        width = (style->width <= 0) ? scalefactor : style->width * scalefactor;
        width = MS_MIN(width, style->maxwidth * image->resolutionfactor);
        width = MS_MAX(width, style->minwidth * image->resolutionfactor);
        spacing = (style->size <= 0) ? scalefactor : style->size * scalefactor;
        spacing = MS_MIN(spacing, style->maxsize * image->resolutionfactor);
        spacing = MS_MAX(spacing, style->minsize * image->resolutionfactor);

        /* scale the pattern by the factor applied to the width */
        for (i = 0; i < style->patternlength; i++) {
          pattern[i] = style->pattern[i] * width / style->width;
        }

        ret = msHatchPolygon(image, offsetPolygon, spacing, width, pattern,
                             style->patternlength, style->angle, &style->color);
        goto cleanup;
      } else {
        symbolStyleObj s;
        int pw, ph;
        imageObj *tile;
        int seamless = 0;

        if (preloadSymbol(&map->symbolset, symbol, renderer) != MS_SUCCESS) {
          ret = MS_FAILURE;
          goto cleanup;
        }

        INIT_SYMBOL_STYLE(s);
        computeSymbolStyle(&s, style, symbol, scalefactor,
                           image->resolutionfactor);
        s.style = style;

        if (!s.color && !s.outlinecolor && symbol->type != MS_SYMBOL_PIXMAP &&
            symbol->type != MS_SYMBOL_SVG) {
          ret = MS_SUCCESS; /* nothing to do (colors are required except for
                               PIXMAP symbols */
          goto cleanup;
        }

        if (s.backgroundcolor) {
          ret =
              renderer->renderPolygon(image, offsetPolygon, s.backgroundcolor);
          if (ret != MS_SUCCESS)
            goto cleanup;
        }

        if (s.scale != 1) {
          if (s.gap > 0) {
            pw = MS_MAX(MS_NINT(s.gap), symbol->sizex * s.scale);
            ph = MS_MAX(MS_NINT(s.gap), symbol->sizey * s.scale);
          } else {
            pw = MS_NINT(symbol->sizex * s.scale);
            ph = MS_NINT(symbol->sizey * s.scale);
          }
        } else {
          if (s.gap > 0) {
            pw = MS_MAX(s.gap, symbol->sizex);
            ph = MS_MAX(s.gap, symbol->sizey);
          } else {
            pw = symbol->sizex;
            ph = symbol->sizey;
          }
        }
        if (pw < 1)
          pw = 1;
        if (ph < 1)
          ph = 1;

        /* if we're doing vector symbols with an antialiased pixel rendererer,
         * we want to enable seamless mode, i.e. comute a tile that accounts for
         * the blending of neighbouring tiles at the tile border
         */
        if (symbol->type == MS_SYMBOL_VECTOR && style->gap == 0 &&
            (image->format->renderer == MS_RENDER_WITH_AGG ||
             image->format->renderer == MS_RENDER_WITH_CAIRO_RASTER)) {
          seamless = 1;
        }
        tile = getTile(image, symbol, &s, pw, ph, seamless);
        ret = renderer->renderPolygonTiled(image, offsetPolygon, tile);
      }

    cleanup:
      if (offsetPolygon != p) {
        msFreeShape(offsetPolygon);
        msFree(offsetPolygon);
      }
      return ret;
    } else if (MS_RENDERER_IMAGEMAP(image->format))
      msDrawShadeSymbolIM(map, image, p, style, scalefactor);
  }
  return ret;
}

int msDrawMarkerSymbol(mapObj *map, imageObj *image, pointObj *p,
                       styleObj *style, double scalefactor) {
  int ret = MS_SUCCESS;
  if (!p)
    return MS_SUCCESS;
  if (style->symbol >= map->symbolset.numsymbols || style->symbol <= 0)
    return MS_SUCCESS; /* no such symbol, 0 is OK   */

  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
      rendererVTableObj *renderer = image->format->vtable;
      symbolStyleObj s;
      double p_x, p_y;
      symbolObj *symbol = map->symbolset.symbol[style->symbol];
      /* store a reference to the renderer to be used for freeing */
      symbol->renderer = renderer;
      if (preloadSymbol(&map->symbolset, symbol, renderer) != MS_SUCCESS) {
        return MS_FAILURE;
      }

      computeSymbolStyle(&s, style, symbol, scalefactor,
                         image->resolutionfactor);
      s.style = style;
      if (!s.color && !s.outlinecolor && symbol->type != MS_SYMBOL_PIXMAP &&
          symbol->type != MS_SYMBOL_SVG) {
        return MS_SUCCESS; // nothing to do if no color, except for pixmap
                           // symbols
      }
      if (s.scale == 0) {
        return MS_SUCCESS;
      }

      /* TODO: skip the drawing of the symbol if it's smaller than a pixel ?
      if (s.size < 1)
       return; // size too small
       */

      p_x = p->x;
      p_y = p->y;

      if (style->polaroffsetpixel != 0 || style->polaroffsetangle != 0) {
        double angle = style->polaroffsetangle * MS_DEG_TO_RAD;
        p_x += (style->polaroffsetpixel * cos(-angle)) * scalefactor;
        p_y += (style->polaroffsetpixel * sin(-angle)) * scalefactor;
      }

      p_x += style->offsetx * scalefactor;
      p_y += style->offsety * scalefactor;

      if (symbol->anchorpoint_x != 0.5 || symbol->anchorpoint_y != 0.5) {
        double sx, sy;
        double ox, oy;
        if (MS_UNLIKELY(MS_FAILURE ==
                        msGetMarkerSize(map, style, &sx, &sy, scalefactor))) {
          return MS_FAILURE;
        }
        ox = (0.5 - symbol->anchorpoint_x) * sx;
        oy = (0.5 - symbol->anchorpoint_y) * sy;
        if (s.rotation != 0) {
          double sina, cosa;
          double rox, roy;
          sina = sin(-s.rotation);
          cosa = cos(-s.rotation);
          rox = ox * cosa - oy * sina;
          roy = ox * sina + oy * cosa;
          p_x += rox;
          p_y += roy;
        } else {
          p_x += ox;
          p_y += oy;
        }
      }

      if (renderer->use_imagecache) {
        imageObj *tile = getTile(image, symbol, &s, -1, -1, 0);
        if (tile != NULL)
          return renderer->renderTile(image, tile, p_x, p_y);
        else {
          msSetError(MS_RENDERERERR, "problem creating cached tile",
                     "msDrawMarkerSymbol()");
          return MS_FAILURE;
        }
      }
      switch (symbol->type) {
      case (MS_SYMBOL_TRUETYPE): {
        unsigned int unicode;
        glyph_element *glyphc;
        face_element *face = msGetFontFace(symbol->font, &map->fontset);
        if (MS_UNLIKELY(!face))
          return MS_FAILURE;
        msUTF8ToUniChar(symbol->character, &unicode);
        unicode = msGetGlyphIndex(face, unicode);
        glyphc = msGetGlyphByIndex(face, MS_MAX(MS_NINT(s.scale), 1), unicode);
        if (MS_UNLIKELY(!glyphc))
          return MS_FAILURE;
        ret =
            drawGlyphMarker(image, face, glyphc, p_x, p_y, s.scale, s.rotation,
                            s.color, s.outlinecolor, s.outlinewidth);
      } break;
      case (MS_SYMBOL_PIXMAP): {
        assert(symbol->pixmap_buffer);
        ret = renderer->renderPixmapSymbol(image, p_x, p_y, symbol, &s);
      } break;
      case (MS_SYMBOL_ELLIPSE): {
        ret = renderer->renderEllipseSymbol(image, p_x, p_y, symbol, &s);
      } break;
      case (MS_SYMBOL_VECTOR): {
        ret = renderer->renderVectorSymbol(image, p_x, p_y, symbol, &s);
      } break;
      case (MS_SYMBOL_SVG): {
        if (renderer->supports_svg) {
          ret = renderer->renderSVGSymbol(image, p_x, p_y, symbol, &s);
        } else {
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
          ret = msRenderRasterizedSVGSymbol(image, p_x, p_y, symbol, &s);
#else
          msSetError(MS_SYMERR, "SVG symbol support is not enabled.",
                     "msDrawMarkerSymbol()");
          return MS_FAILURE;
#endif
        }
      } break;
      default:
        break;
      }
      return ret;
    } else if (MS_RENDERER_IMAGEMAP(image->format))
      msDrawMarkerSymbolIM(map, image, p, style, scalefactor);
  }
  return ret;
}

int msDrawLabelBounds(mapObj *map, imageObj *image, label_bounds *bnds,
                      styleObj *style, double scalefactor) {
  /*
   * helper function to draw label bounds, where we might have only a rectObj
   * and not a lineObj/shapeObj
   */
  shapeObj shape;
  shape.numlines = 1;
  if (bnds->poly) {
    shape.line = bnds->poly;
    return msDrawShadeSymbol(map, image, &shape, style, scalefactor);
  } else {
    pointObj pnts1[5];
    lineObj l;
    l.point = pnts1;
    l.numpoints = 5;
    pnts1[0].x = pnts1[1].x = pnts1[4].x = bnds->bbox.minx;
    pnts1[2].x = pnts1[3].x = bnds->bbox.maxx;
    pnts1[0].y = pnts1[3].y = pnts1[4].y = bnds->bbox.miny;
    pnts1[1].y = pnts1[2].y = bnds->bbox.maxy;
    (void)pnts1[0].x;
    (void)pnts1[0].y;
    (void)pnts1[1].x;
    (void)pnts1[1].y;
    (void)pnts1[2].x;
    (void)pnts1[2].y;
    (void)pnts1[3].x;
    (void)pnts1[3].y;
    (void)pnts1[4].x;
    (void)pnts1[4].y;
    shape.line = &l; // must return from this block
    return msDrawShadeSymbol(map, image, &shape, style, scalefactor);
  }
}

int msDrawTextSymbol(mapObj *map, imageObj *image, pointObj labelPnt,
                     textSymbolObj *ts) {
  (void)map;
  rendererVTableObj *renderer = image->format->vtable;
  colorObj *c = NULL, *oc = NULL;
  int ow;
  assert(ts->textpath);
  if (!renderer->renderGlyphs)
    return MS_FAILURE;

  if (!ts->textpath->absolute) {
    int g;
    double cosa, sina;
    double x = labelPnt.x;
    double y = labelPnt.y;
    if (ts->rotation != 0) {
      cosa = cos(ts->rotation);
      sina = sin(ts->rotation);
      for (g = 0; g < ts->textpath->numglyphs; g++) {
        double ox = ts->textpath->glyphs[g].pnt.x;
        double oy = ts->textpath->glyphs[g].pnt.y;
        ts->textpath->glyphs[g].pnt.x = ox * cosa + oy * sina + x;
        ts->textpath->glyphs[g].pnt.y = -ox * sina + oy * cosa + y;
        ts->textpath->glyphs[g].rot = ts->rotation;
      }
    } else {
      for (g = 0; g < ts->textpath->numglyphs; g++) {
        ts->textpath->glyphs[g].pnt.x += x;
        ts->textpath->glyphs[g].pnt.y += y;
      }
    }
  }

  if (MS_VALID_COLOR(ts->label->shadowcolor)) {
    textSymbolObj *ts_shadow;
    int g;
    double ox, oy;
    double cosa, sina;
    int ret;
    if (ts->rotation != 0) {
      cosa = cos(ts->rotation);
      sina = sin(ts->rotation);
      ox = ts->scalefactor *
           (cosa * ts->label->shadowsizex + sina * ts->label->shadowsizey);
      oy = ts->scalefactor *
           (-sina * ts->label->shadowsizex + cosa * ts->label->shadowsizey);
    } else {
      ox = ts->scalefactor * ts->label->shadowsizex;
      oy = ts->scalefactor * ts->label->shadowsizey;
    }

    ts_shadow = msSmallMalloc(sizeof(textSymbolObj));
    initTextSymbol(ts_shadow);
    msCopyTextSymbol(ts_shadow, ts);

    for (g = 0; g < ts_shadow->textpath->numglyphs; g++) {
      ts_shadow->textpath->glyphs[g].pnt.x += ox;
      ts_shadow->textpath->glyphs[g].pnt.y += oy;
    }

    ret = renderer->renderGlyphs(image, ts_shadow, &ts->label->shadowcolor,
                                 NULL, 0, MS_FALSE);
    freeTextSymbol(ts_shadow);
    msFree(ts_shadow);
    if (ret != MS_SUCCESS)
      return ret;
  }

  if (MS_VALID_COLOR(ts->label->color))
    c = &ts->label->color;
  if (MS_VALID_COLOR(ts->label->outlinecolor))
    oc = &ts->label->outlinecolor;
  ow = MS_NINT((double)ts->label->outlinewidth *
               ((double)ts->textpath->glyph_size / (double)ts->label->size));
  return renderer->renderGlyphs(image, ts, c, oc, ow, MS_FALSE);
}

/************************************************************************/
/*                          msCircleDrawLineSymbol                      */
/*                                                                      */
/************************************************************************/
int msCircleDrawLineSymbol(mapObj *map, imageObj *image, pointObj *p, double r,
                           styleObj *style, double scalefactor) {
  shapeObj *circle;
  int status;
  if (!image)
    return MS_FAILURE;
  circle = msRasterizeArc(p->x, p->y, r, 0, 360, 0);
  if (!circle)
    return MS_FAILURE;
  status = msDrawLineSymbol(map, image, circle, style, scalefactor);
  msFreeShape(circle);
  msFree(circle);
  return status;
}

int msCircleDrawShadeSymbol(mapObj *map, imageObj *image, pointObj *p, double r,
                            styleObj *style, double scalefactor) {
  shapeObj *circle;
  int status;
  if (!image)
    return MS_FAILURE;
  circle = msRasterizeArc(p->x, p->y, r, 0, 360, 0);
  if (!circle)
    return MS_FAILURE;
  status = msDrawShadeSymbol(map, image, circle, style, scalefactor);
  msFreeShape(circle);
  msFree(circle);
  return status;
}

int msDrawPieSlice(mapObj *map, imageObj *image, pointObj *p, styleObj *style,
                   double radius, double start, double end) {
  int status;
  shapeObj *circle;
  double center_x = p->x;
  double center_y = p->y;
  if (!image)
    return MS_FAILURE;
  if (style->offsetx > 0) {
    center_x += style->offsetx * cos(((-start - end) * MS_PI / 360.));
    center_y -= style->offsetx * sin(((-start - end) * MS_PI / 360.));
  }
  circle = msRasterizeArc(center_x, center_y, radius, start, end, 1);
  if (!circle)
    return MS_FAILURE;
  status = msDrawShadeSymbol(map, image, circle, style, 1.0);
  msFreeShape(circle);
  msFree(circle);
  return status;
}

/*
 * RFC 49 implementation
 * if an outlinewidth is used:
 *  - augment the style's width to account for the outline width
 *  - swap the style color and outlinecolor
 *  - draw the shape (the outline) in the first pass of the
 *    caching mechanism
 */

void msOutlineRenderingPrepareStyle(styleObj *pStyle, mapObj *map,
                                    layerObj *layer, imageObj *image) {
  colorObj tmp;

  if (pStyle->outlinewidth > 0) {
    /* adapt width (must take scalefactor into account) */
    pStyle->width += (pStyle->outlinewidth /
                      (layer->scalefactor / image->resolutionfactor)) *
                     2;
    pStyle->minwidth += pStyle->outlinewidth * 2;
    pStyle->maxwidth += pStyle->outlinewidth * 2;
    pStyle->size += (pStyle->outlinewidth / layer->scalefactor *
                     (map->resolution / map->defresolution));

    /*swap color and outlinecolor*/
    tmp = pStyle->color;
    pStyle->color = pStyle->outlinecolor;
    pStyle->outlinecolor = tmp;
  }
}

/*
 * RFC 49 implementation: switch back the styleobj to its
 * original state, so the line fill will be drawn in the
 * second pass of the caching mechanism
 */

void msOutlineRenderingRestoreStyle(styleObj *pStyle, mapObj *map,
                                    layerObj *layer, imageObj *image) {
  colorObj tmp;

  if (pStyle->outlinewidth > 0) {
    /* reset widths to original state */
    pStyle->width -= (pStyle->outlinewidth /
                      (layer->scalefactor / image->resolutionfactor)) *
                     2;
    pStyle->minwidth -= pStyle->outlinewidth * 2;
    pStyle->maxwidth -= pStyle->outlinewidth * 2;
    pStyle->size -= (pStyle->outlinewidth / layer->scalefactor *
                     (map->resolution / map->defresolution));

    /*reswap colors to original state*/
    tmp = pStyle->color;
    pStyle->color = pStyle->outlinecolor;
    pStyle->outlinecolor = tmp;
  }
}

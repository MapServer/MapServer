/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  AGG rendering and other AGG related functions.
 * Author:   Thomas Bonfort and the MapServer team.
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
 *****************************************************************************/

#include "mapserver.h"
#include "fontcache.h"
#include "mapagg.h"
#include <assert.h>
#include "renderers/agg/include/agg_color_rgba.h"
#include "renderers/agg/include/agg_pixfmt_rgba.h"
#include "renderers/agg/include/agg_renderer_base.h"
#include "renderers/agg/include/agg_renderer_scanline.h"
#include "renderers/agg/include/agg_math_stroke.h"
#include "renderers/agg/include/agg_scanline_p.h"
#include "renderers/agg/include/agg_scanline_u.h"
#include "renderers/agg/include/agg_rasterizer_scanline_aa.h"
#include "renderers/agg/include/agg_span_pattern_rgba.h"
#include "renderers/agg/include/agg_span_allocator.h"
#include "renderers/agg/include/agg_span_interpolator_linear.h"
#include "renderers/agg/include/agg_pattern_filters_rgba.h"
#include "renderers/agg/include/agg_image_accessors.h"
#include "renderers/agg/include/agg_conv_stroke.h"
#include "renderers/agg/include/agg_conv_dash.h"
#include "renderers/agg/include/agg_font_freetype.h"
#include "renderers/agg/include/agg_conv_contour.h"
#include "renderers/agg/include/agg_ellipse.h"
#include "renderers/agg/include/agg_gamma_functions.h"
#include "renderers/agg/include/agg_blur.h"

#include "renderers/agg/include/agg_rasterizer_outline_aa.h"
#include "renderers/agg/include/agg_renderer_outline_aa.h"
#include "renderers/agg/include/agg_renderer_outline_image.h"
#include "renderers/agg/include/agg_span_pattern_rgba.h"
#include "renderers/agg/include/agg_span_image_filter_rgba.h"
#include "renderers/agg/include/agg_glyph_raster_bin.h"
#include "renderers/agg/include/agg_renderer_raster_text.h"
#include "renderers/agg/include/agg_path_storage_integer.h"

#include "renderers/agg/include/agg_conv_clipper.h"

#include "cpl_conv.h"   // CPLGetConfigOption
#include "cpl_string.h" // CPLTestBool

#ifdef USE_PIXMAN
#include <pixman.h>
#endif

#include <limits>
#include <memory>
#include <new>
#include <vector>

typedef mapserver::order_bgra band_order;

#define AGG_LINESPACE 1.33

typedef mapserver::int8u band_type;
typedef mapserver::rgba8 color_type;
typedef mapserver::pixel32_type pixel_type;

typedef mapserver::blender_rgba_pre<color_type, band_order> blender_pre;
typedef mapserver::comp_op_adaptor_rgba_pre<color_type, band_order>
    compop_blender_pre;

typedef mapserver::pixfmt_alpha_blend_rgba<
    blender_pre, mapserver::rendering_buffer, pixel_type>
    pixel_format;
typedef mapserver::pixfmt_custom_blend_rgba<compop_blender_pre,
                                            mapserver::rendering_buffer>
    compop_pixel_format;
typedef mapserver::rendering_buffer rendering_buffer;
typedef mapserver::renderer_base<pixel_format> renderer_base;
typedef mapserver::renderer_base<compop_pixel_format> compop_renderer_base;
typedef mapserver::renderer_scanline_aa_solid<renderer_base> renderer_scanline;
typedef mapserver::renderer_scanline_bin_solid<renderer_base>
    renderer_scanline_aliased;
typedef mapserver::rasterizer_scanline_aa<> rasterizer_scanline;
typedef mapserver::font_engine_freetype_int16 font_engine_type;
typedef mapserver::font_cache_manager<font_engine_type> font_manager_type;
typedef mapserver::conv_curve<font_manager_type::path_adaptor_type>
    font_curve_type;
typedef mapserver::glyph_raster_bin<color_type> glyph_gen;

static const color_type AGG_NO_COLOR = color_type(0, 0, 0, 0);

#define aggColor(c)                                                            \
  mapserver::rgba8_pre((c)->red, (c)->green, (c)->blue, (c)->alpha)

class aggRendererCache {
public:
  font_engine_type m_feng;
  font_manager_type m_fman;
  aggRendererCache() : m_fman(m_feng) {}
};

class AGG2Renderer {
public:
  std::vector<band_type> buffer{};
  rendering_buffer m_rendering_buffer;
  pixel_format m_pixel_format;
  compop_pixel_format m_compop_pixel_format;
  renderer_base m_renderer_base;
  compop_renderer_base m_compop_renderer_base;
  renderer_scanline m_renderer_scanline;
  renderer_scanline_aliased m_renderer_scanline_aliased;
  rasterizer_scanline m_rasterizer_aa;
  rasterizer_scanline m_rasterizer_aa_gamma;
  mapserver::scanline_p8 sl_poly; /*packed scanlines, works faster when the area
    is larger than the perimeter, in number of pixels*/
  mapserver::scanline_u8 sl_line; /*unpacked scanlines, works faster if the area
    is roughly equal to the perimeter, in number of pixels*/
  bool use_alpha = false;
  std::unique_ptr<mapserver::conv_stroke<line_adaptor>> stroke{};
  std::unique_ptr<mapserver::conv_dash<line_adaptor>> dash{};
  std::unique_ptr<mapserver::conv_stroke<mapserver::conv_dash<line_adaptor>>>
      stroke_dash{};
  double default_gamma = 0.0;
  mapserver::gamma_linear gamma_function;
};

#define AGG_RENDERER(image) ((AGG2Renderer *)(image)->img.plugin)

template <class VertexSource>
static void applyCJC(VertexSource &stroke, int caps, int joins) {
  switch (joins) {
  case MS_CJC_ROUND:
    stroke.line_join(mapserver::round_join);
    break;
  case MS_CJC_MITER:
    stroke.line_join(mapserver::miter_join);
    break;
  case MS_CJC_BEVEL:
  case MS_CJC_NONE:
    stroke.line_join(mapserver::bevel_join);
    break;
  }
  switch (caps) {
  case MS_CJC_BUTT:
  case MS_CJC_NONE:
    stroke.line_cap(mapserver::butt_cap);
    break;
  case MS_CJC_ROUND:
    stroke.line_cap(mapserver::round_cap);
    break;
  case MS_CJC_SQUARE:
    stroke.line_cap(mapserver::square_cap);
    break;
  }
}

int agg2RenderLine(imageObj *img, shapeObj *p, strokeStyleObj *style) {

  AGG2Renderer *r = AGG_RENDERER(img);
  line_adaptor lines = line_adaptor(p);

  r->m_rasterizer_aa.reset();
  r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
  if (style->antialiased == MS_FALSE) {
    r->m_renderer_scanline_aliased.color(aggColor(style->color));
  } else {
    r->m_renderer_scanline.color(aggColor(style->color));
  }

  if (style->patternlength <= 0) {
    if (!r->stroke) {
      r->stroke.reset(new mapserver::conv_stroke<line_adaptor>(lines));
    } else {
      r->stroke->attach(lines);
    }
    r->stroke->width(style->width);
    if (style->width > 1) {
      applyCJC(*r->stroke, style->linecap, style->linejoin);
    } else {
      r->stroke->inner_join(mapserver::inner_bevel);
      r->stroke->line_join(mapserver::bevel_join);
    }
    r->m_rasterizer_aa.add_path(*r->stroke);
  } else {
    if (!r->dash) {
      r->dash.reset(new mapserver::conv_dash<line_adaptor>(lines));
    } else {
      r->dash->remove_all_dashes();
      r->dash->dash_start(0.0);
      r->dash->attach(lines);
    }
    if (!r->stroke_dash) {
      r->stroke_dash.reset(
          new mapserver::conv_stroke<mapserver::conv_dash<line_adaptor>>(
              *r->dash));
    } else {
      r->stroke_dash->attach(*r->dash);
    }
    int patt_length = 0;
    for (int i = 0; i < style->patternlength; i += 2) {
      if (i < style->patternlength - 1) {
        r->dash->add_dash(MS_MAX(1, MS_NINT(style->pattern[i])),
                          MS_MAX(1, MS_NINT(style->pattern[i + 1])));
        if (style->patternoffset) {
          patt_length += MS_MAX(1, MS_NINT(style->pattern[i])) +
                         MS_MAX(1, MS_NINT(style->pattern[i + 1]));
        }
      }
    }
    if (style->patternoffset > 0) {
      r->dash->dash_start(patt_length - style->patternoffset);
    }
    r->stroke_dash->width(style->width);
    if (style->width > 1) {
      applyCJC(*r->stroke_dash, style->linecap, style->linejoin);
    } else {
      r->stroke_dash->inner_join(mapserver::inner_bevel);
      r->stroke_dash->line_join(mapserver::bevel_join);
    }
    r->m_rasterizer_aa.add_path(*r->stroke_dash);
  }
  if (style->antialiased == MS_FALSE)
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line,
                                r->m_renderer_scanline_aliased);
  else
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line,
                                r->m_renderer_scanline);
  return MS_SUCCESS;
}

int agg2RenderLineTiled(imageObj *img, shapeObj *p, imageObj *tile) {

  mapserver::pattern_filter_bilinear_rgba8 fltr;
  typedef mapserver::line_image_pattern<
      mapserver::pattern_filter_bilinear_rgba8>
      pattern_type;
  typedef mapserver::renderer_outline_image<renderer_base, pattern_type>
      renderer_img_type;
  typedef mapserver::rasterizer_outline_aa<renderer_img_type,
                                           mapserver::line_coord_sat>
      rasterizer_img_type;
  pattern_type patt(fltr);

  AGG2Renderer *r = AGG_RENDERER(img);
  AGG2Renderer *tileRenderer = AGG_RENDERER(tile);

  line_adaptor lines(p);

  patt.create(tileRenderer->m_pixel_format);
  renderer_img_type ren_img(r->m_renderer_base, patt);
  rasterizer_img_type ras_img(ren_img);
  ras_img.add_path(lines);
  return MS_SUCCESS;
}

int agg2RenderPolygon(imageObj *img, shapeObj *p, colorObj *color) {
  AGG2Renderer *r = AGG_RENDERER(img);
  polygon_adaptor polygons(p);
  r->m_rasterizer_aa_gamma.reset();
  r->m_rasterizer_aa_gamma.filling_rule(mapserver::fill_even_odd);
  r->m_rasterizer_aa_gamma.add_path(polygons);
  r->m_renderer_scanline.color(aggColor(color));
  mapserver::render_scanlines(r->m_rasterizer_aa_gamma, r->sl_poly,
                              r->m_renderer_scanline);
  return MS_SUCCESS;
}

static inline double int26p6_to_dbl(int p) { return double(p) / 64.0; }

template <class PathStorage>
bool decompose_ft_outline(const FT_Outline &outline, bool flip_y,
                          const mapserver::trans_affine &mtx,
                          PathStorage &path) {
  double x1, y1, x2, y2, x3, y3;

  FT_Vector *point;
  FT_Vector *limit;
  char *tags;

  int n;     // index of contour in outline
  int first; // index of first point in contour
  char tag;  // current point's state

  first = 0;

  for (n = 0; n < outline.n_contours; n++) {
    int last; // index of last point in contour

    last = outline.contours[n];
    limit = outline.points + last;

    FT_Vector v_start = outline.points[first];

    FT_Vector v_control = v_start;

    point = outline.points + first;
    tags = outline.tags + first;
    tag = FT_CURVE_TAG(tags[0]);

    // A contour cannot start with a cubic control point!
    if (tag == FT_CURVE_TAG_CUBIC)
      return false;

    // check first point to determine origin
    if (tag == FT_CURVE_TAG_CONIC) {
      const FT_Vector v_last = outline.points[last];

      // first point is conic control.  Yes, this happens.
      if (FT_CURVE_TAG(outline.tags[last]) == FT_CURVE_TAG_ON) {
        // start at last point if it is on the curve
        v_start = v_last;
        limit--;
      } else {
        // if both first and last points are conic,
        // start at their middle and record its position
        // for closure
        v_start.x = (v_start.x + v_last.x) / 2;
        v_start.y = (v_start.y + v_last.y) / 2;
      }
      point--;
      tags--;
    }

    x1 = int26p6_to_dbl(v_start.x);
    y1 = int26p6_to_dbl(v_start.y);
    if (flip_y)
      y1 = -y1;
    mtx.transform(&x1, &y1);
    path.move_to(x1, y1);

    while (point < limit) {
      point++;
      tags++;

      tag = FT_CURVE_TAG(tags[0]);
      switch (tag) {
      case FT_CURVE_TAG_ON: // emit a single line_to
      {
        x1 = int26p6_to_dbl(point->x);
        y1 = int26p6_to_dbl(point->y);
        if (flip_y)
          y1 = -y1;
        mtx.transform(&x1, &y1);
        path.line_to(x1, y1);
        // path.line_to(conv(point->x), flip_y ? -conv(point->y) :
        // conv(point->y));
        continue;
      }

      case FT_CURVE_TAG_CONIC: // consume conic arcs
      {
        v_control.x = point->x;
        v_control.y = point->y;

      Do_Conic:
        if (point < limit) {
          FT_Vector vec;
          FT_Vector v_middle;

          point++;
          tags++;
          tag = FT_CURVE_TAG(tags[0]);

          vec.x = point->x;
          vec.y = point->y;

          if (tag == FT_CURVE_TAG_ON) {
            x1 = int26p6_to_dbl(v_control.x);
            y1 = int26p6_to_dbl(v_control.y);
            x2 = int26p6_to_dbl(vec.x);
            y2 = int26p6_to_dbl(vec.y);
            if (flip_y) {
              y1 = -y1;
              y2 = -y2;
            }
            mtx.transform(&x1, &y1);
            mtx.transform(&x2, &y2);
            path.curve3(x1, y1, x2, y2);
            continue;
          }

          if (tag != FT_CURVE_TAG_CONIC)
            return false;

          v_middle.x = (v_control.x + vec.x) / 2;
          v_middle.y = (v_control.y + vec.y) / 2;

          x1 = int26p6_to_dbl(v_control.x);
          y1 = int26p6_to_dbl(v_control.y);
          x2 = int26p6_to_dbl(v_middle.x);
          y2 = int26p6_to_dbl(v_middle.y);
          if (flip_y) {
            y1 = -y1;
            y2 = -y2;
          }
          mtx.transform(&x1, &y1);
          mtx.transform(&x2, &y2);
          path.curve3(x1, y1, x2, y2);

          // path.curve3(conv(v_control.x),
          //             flip_y ? -conv(v_control.y) : conv(v_control.y),
          //             conv(v_middle.x),
          //             flip_y ? -conv(v_middle.y) : conv(v_middle.y));

          v_control = vec;
          goto Do_Conic;
        }

        x1 = int26p6_to_dbl(v_control.x);
        y1 = int26p6_to_dbl(v_control.y);
        x2 = int26p6_to_dbl(v_start.x);
        y2 = int26p6_to_dbl(v_start.y);
        if (flip_y) {
          y1 = -y1;
          y2 = -y2;
        }
        mtx.transform(&x1, &y1);
        mtx.transform(&x2, &y2);
        path.curve3(x1, y1, x2, y2);

        // path.curve3(conv(v_control.x),
        //             flip_y ? -conv(v_control.y) : conv(v_control.y),
        //             conv(v_start.x),
        //             flip_y ? -conv(v_start.y) : conv(v_start.y));
        goto Close;
      }

      default: // FT_CURVE_TAG_CUBIC
      {
        FT_Vector vec1, vec2;

        if (point + 1 > limit || FT_CURVE_TAG(tags[1]) != FT_CURVE_TAG_CUBIC) {
          return false;
        }

        vec1.x = point[0].x;
        vec1.y = point[0].y;
        vec2.x = point[1].x;
        vec2.y = point[1].y;

        point += 2;
        tags += 2;

        if (point <= limit) {
          FT_Vector vec;

          vec.x = point->x;
          vec.y = point->y;

          x1 = int26p6_to_dbl(vec1.x);
          y1 = int26p6_to_dbl(vec1.y);
          x2 = int26p6_to_dbl(vec2.x);
          y2 = int26p6_to_dbl(vec2.y);
          x3 = int26p6_to_dbl(vec.x);
          y3 = int26p6_to_dbl(vec.y);
          if (flip_y) {
            y1 = -y1;
            y2 = -y2;
            y3 = -y3;
          }
          mtx.transform(&x1, &y1);
          mtx.transform(&x2, &y2);
          mtx.transform(&x3, &y3);
          path.curve4(x1, y1, x2, y2, x3, y3);

          // path.curve4(conv(vec1.x),
          //             flip_y ? -conv(vec1.y) : conv(vec1.y),
          //             conv(vec2.x),
          //             flip_y ? -conv(vec2.y) : conv(vec2.y),
          //             conv(vec.x),
          //             flip_y ? -conv(vec.y) : conv(vec.y));
          continue;
        }

        x1 = int26p6_to_dbl(vec1.x);
        y1 = int26p6_to_dbl(vec1.y);
        x2 = int26p6_to_dbl(vec2.x);
        y2 = int26p6_to_dbl(vec2.y);
        x3 = int26p6_to_dbl(v_start.x);
        y3 = int26p6_to_dbl(v_start.y);
        if (flip_y) {
          y1 = -y1;
          y2 = -y2;
          y3 = -y3;
        }
        mtx.transform(&x1, &y1);
        mtx.transform(&x2, &y2);
        mtx.transform(&x3, &y3);
        path.curve4(x1, y1, x2, y2, x3, y3);

        // path.curve4(conv(vec1.x),
        //             flip_y ? -conv(vec1.y) : conv(vec1.y),
        //             conv(vec2.x),
        //             flip_y ? -conv(vec2.y) : conv(vec2.y),
        //             conv(v_start.x),
        //             flip_y ? -conv(v_start.y) : conv(v_start.y));
        goto Close;
      }
      }
    }

    path.close_polygon();

  Close:
    first = last + 1;
  }

  return true;
}

int agg2RenderPolygonTiled(imageObj *img, shapeObj *p, imageObj *tile) {
  assert(img->format->renderer == tile->format->renderer);

  AGG2Renderer *r = AGG_RENDERER(img);
  AGG2Renderer *tileRenderer = AGG_RENDERER(tile);
  polygon_adaptor polygons(p);
  typedef mapserver::wrap_mode_repeat wrap_type;
  typedef mapserver::image_accessor_wrap<pixel_format, wrap_type, wrap_type>
      img_source_type;
  typedef mapserver::span_pattern_rgba<img_source_type> span_gen_type;
  mapserver::span_allocator<mapserver::rgba8> sa;
  r->m_rasterizer_aa.reset();
  r->m_rasterizer_aa.filling_rule(mapserver::fill_even_odd);
  img_source_type img_src(tileRenderer->m_pixel_format);
  span_gen_type sg(img_src, 0, 0);
  r->m_rasterizer_aa.add_path(polygons);
  mapserver::render_scanlines_aa(r->m_rasterizer_aa, r->sl_poly,
                                 r->m_renderer_base, sa, sg);
  return MS_SUCCESS;
}

int agg2RenderGlyphsPath(imageObj *img, const textSymbolObj *ts, colorObj *c,
                         colorObj *oc, int ow, int /*isMarker*/) {

  const textPathObj *tp = ts->textpath;
  mapserver::path_storage glyphs;
  mapserver::trans_affine trans;
  AGG2Renderer *r = AGG_RENDERER(img);
  r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
  for (int i = 0; i < tp->numglyphs; i++) {
    glyphObj *gl = tp->glyphs + i;
    trans.reset();
    trans.rotate(-gl->rot);
    trans.translate(gl->pnt.x, gl->pnt.y);
    outline_element *ol = msGetGlyphOutline(gl->face, gl->glyph);
    if (!ol) {
      return MS_FAILURE;
    }
    decompose_ft_outline(ol->outline, true, trans, glyphs);
  }
  mapserver::conv_curve<mapserver::path_storage> m_curves(glyphs);
  if (oc) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    mapserver::conv_contour<mapserver::conv_curve<mapserver::path_storage>> cc(
        m_curves);
    cc.width(ow + 1);
    r->m_rasterizer_aa.add_path(cc);
    r->m_renderer_scanline.color(aggColor(oc));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line,
                                r->m_renderer_scanline);
  }
  if (c) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    r->m_rasterizer_aa.add_path(m_curves);
    r->m_renderer_scanline.color(aggColor(c));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line,
                                r->m_renderer_scanline);
  }
  return MS_SUCCESS;
}

mapserver::path_storage imageVectorSymbol(symbolObj *symbol) {
  mapserver::path_storage path;
  int is_new = 1;

  for (int i = 0; i < symbol->numpoints; i++) {
    if ((symbol->points[i].x == -99) && (symbol->points[i].y == -99))
      is_new = 1;

    else {
      if (is_new) {
        path.move_to(symbol->points[i].x, symbol->points[i].y);
        is_new = 0;
      } else {
        path.line_to(symbol->points[i].x, symbol->points[i].y);
      }
    }
  }
  return path;
}

int agg2RenderVectorSymbol(imageObj *img, double x, double y, symbolObj *symbol,
                           symbolStyleObj *style) {
  AGG2Renderer *r = AGG_RENDERER(img);
  double ox = symbol->sizex * 0.5;
  double oy = symbol->sizey * 0.5;

  mapserver::path_storage path = imageVectorSymbol(symbol);
  mapserver::trans_affine mtx;
  mtx *= mapserver::trans_affine_translation(-ox, -oy);
  mtx *= mapserver::trans_affine_scaling(style->scale);
  mtx *= mapserver::trans_affine_rotation(-style->rotation);
  mtx *= mapserver::trans_affine_translation(x, y);
  path.transform(mtx);
  if (style->color) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_even_odd);
    r->m_rasterizer_aa.add_path(path);
    r->m_renderer_scanline.color(aggColor(style->color));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_poly,
                                r->m_renderer_scanline);
  }
  if (style->outlinecolor) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    r->m_renderer_scanline.color(aggColor(style->outlinecolor));
    mapserver::conv_stroke<mapserver::path_storage> stroke(path);
    stroke.width(style->outlinewidth);
    r->m_rasterizer_aa.add_path(stroke);
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_poly,
                                r->m_renderer_scanline);
  }
  return MS_SUCCESS;
}

int agg2RenderPixmapSymbol(imageObj *img, double x, double y, symbolObj *symbol,
                           symbolStyleObj *style) {
  AGG2Renderer *r = AGG_RENDERER(img);
  rasterBufferObj *pixmap = symbol->pixmap_buffer;
  assert(pixmap->type == MS_BUFFER_BYTE_RGBA);
  rendering_buffer b(pixmap->data.rgba.pixels, pixmap->width, pixmap->height,
                     pixmap->data.rgba.row_step);
  pixel_format pf(b);

  r->m_rasterizer_aa.reset();
  r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
  if ((style->rotation != 0 && style->rotation != MS_PI * 2.) ||
      style->scale != 1) {
    mapserver::trans_affine image_mtx;
    image_mtx *= mapserver::trans_affine_translation(-(pf.width() / 2.),
                                                     -(pf.height() / 2.));
    /*agg angles are antitrigonometric*/
    image_mtx *= mapserver::trans_affine_rotation(-style->rotation);
    image_mtx *= mapserver::trans_affine_scaling(style->scale);

    image_mtx *= mapserver::trans_affine_translation(x, y);
    image_mtx.invert();
    typedef mapserver::span_interpolator_linear<> interpolator_type;
    interpolator_type interpolator(image_mtx);
    mapserver::span_allocator<color_type> sa;

    // "hardcoded" bilinear filter
    //------------------------------------------
    typedef mapserver::span_image_filter_rgba_bilinear_clip<pixel_format,
                                                            interpolator_type>
        span_gen_type;
    span_gen_type sg(pf, mapserver::rgba(0, 0, 0, 0), interpolator);
    mapserver::path_storage pixmap_bbox;
    int ims_2 =
        MS_NINT(MS_MAX(pixmap->width, pixmap->height) * style->scale * 1.415) /
            2 +
        1;

    pixmap_bbox.move_to(x - ims_2, y - ims_2);
    pixmap_bbox.line_to(x + ims_2, y - ims_2);
    pixmap_bbox.line_to(x + ims_2, y + ims_2);
    pixmap_bbox.line_to(x - ims_2, y + ims_2);

    r->m_rasterizer_aa.add_path(pixmap_bbox);
    mapserver::render_scanlines_aa(r->m_rasterizer_aa, r->sl_poly,
                                   r->m_renderer_base, sa, sg);
  } else {
    // just copy the image at the correct location (we place the pixmap on
    // the nearest integer pixel to avoid blurring)
    r->m_renderer_base.blend_from(pf, 0, MS_NINT(x - pixmap->width / 2.),
                                  MS_NINT(y - pixmap->height / 2.));
  }
  return MS_SUCCESS;
}

int agg2RenderEllipseSymbol(imageObj *image, double x, double y,
                            symbolObj *symbol, symbolStyleObj *style) {
  AGG2Renderer *r = AGG_RENDERER(image);
  mapserver::path_storage path;
  mapserver::ellipse ellipse(x, y, symbol->sizex * style->scale / 2,
                             symbol->sizey * style->scale / 2);
  path.concat_path(ellipse);
  if (style->rotation != 0) {
    mapserver::trans_affine mtx;
    mtx *= mapserver::trans_affine_translation(-x, -y);
    /*agg angles are antitrigonometric*/
    mtx *= mapserver::trans_affine_rotation(-style->rotation);
    mtx *= mapserver::trans_affine_translation(x, y);
    path.transform(mtx);
  }

  if (style->color) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_even_odd);
    r->m_rasterizer_aa.add_path(path);
    r->m_renderer_scanline.color(aggColor(style->color));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line,
                                r->m_renderer_scanline);
  }
  if (style->outlinewidth) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    mapserver::conv_stroke<mapserver::path_storage> stroke(path);
    stroke.width(style->outlinewidth);
    r->m_rasterizer_aa.add_path(stroke);
    r->m_renderer_scanline.color(aggColor(style->outlinecolor));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_poly,
                                r->m_renderer_scanline);
  }
  return MS_SUCCESS;
}

int agg2RenderTile(imageObj * /*img*/, imageObj * /*tile*/, double /*x*/,
                   double /*y*/) {
  /*
  AGG2Renderer *imgRenderer = agg2GetRenderer(img);
  AGG2Renderer *tileRenderer = agg2GetRenderer(tile);
  */
  return MS_FAILURE;
}

int aggInitializeRasterBuffer(rasterBufferObj *rb, int width, int height,
                              int mode) {
  rb->type = MS_BUFFER_BYTE_RGBA;
  rb->data.rgba.pixel_step = 4;
  rb->data.rgba.row_step = rb->data.rgba.pixel_step * width;
  rb->width = width;
  rb->height = height;
  int nBytes = rb->data.rgba.row_step * height;
  rb->data.rgba.pixels = (band_type *)msSmallCalloc(nBytes, sizeof(band_type));
  rb->data.rgba.r = &(rb->data.rgba.pixels[band_order::R]);
  rb->data.rgba.g = &(rb->data.rgba.pixels[band_order::G]);
  rb->data.rgba.b = &(rb->data.rgba.pixels[band_order::B]);
  if (mode == MS_IMAGEMODE_RGBA) {
    rb->data.rgba.a = &(rb->data.rgba.pixels[band_order::A]);
  }
  return MS_SUCCESS;
}

int aggGetRasterBufferHandle(imageObj *img, rasterBufferObj *rb) {
  AGG2Renderer *r = AGG_RENDERER(img);
  rb->type = MS_BUFFER_BYTE_RGBA;
  rb->data.rgba.pixels = r->buffer.data();
  rb->data.rgba.row_step = r->m_rendering_buffer.stride();
  rb->data.rgba.pixel_step = 4;
  rb->width = r->m_rendering_buffer.width();
  rb->height = r->m_rendering_buffer.height();
  rb->data.rgba.r = &(r->buffer[band_order::R]);
  rb->data.rgba.g = &(r->buffer[band_order::G]);
  rb->data.rgba.b = &(r->buffer[band_order::B]);
  if (r->use_alpha)
    rb->data.rgba.a = &(r->buffer[band_order::A]);
  else
    rb->data.rgba.a = NULL;
  return MS_SUCCESS;
}

int aggGetRasterBufferCopy(imageObj *img, rasterBufferObj *rb) {
  AGG2Renderer *r = AGG_RENDERER(img);
  aggInitializeRasterBuffer(rb, img->width, img->height, MS_IMAGEMODE_RGBA);
  int nBytes = r->m_rendering_buffer.stride() * r->m_rendering_buffer.height();
  memcpy(rb->data.rgba.pixels, r->buffer.data(), nBytes);
  return MS_SUCCESS;
}

int agg2MergeRasterBuffer(imageObj *dest, rasterBufferObj *overlay,
                          double opacity, int srcX, int srcY, int dstX,
                          int dstY, int width, int height) {
  assert(overlay->type == MS_BUFFER_BYTE_RGBA);
  rendering_buffer b(overlay->data.rgba.pixels, overlay->width, overlay->height,
                     overlay->data.rgba.row_step);
  pixel_format pf(b);
  AGG2Renderer *r = AGG_RENDERER(dest);
  mapserver::rect_base<int> src_rect(srcX, srcY, srcX + width, srcY + height);
  r->m_renderer_base.blend_from(pf, &src_rect, dstX - srcX, dstY - srcY,
                                unsigned(opacity * 255));
  return MS_SUCCESS;
}

/* image i/o */
imageObj *agg2CreateImage(int width, int height, outputFormatObj *format,
                          colorObj *bg) {
  imageObj *image = NULL;
  if (format->imagemode != MS_IMAGEMODE_RGB &&
      format->imagemode != MS_IMAGEMODE_RGBA) {
    msSetError(MS_MISCERR,
               "AGG2 driver only supports RGB or RGBA pixel models.",
               "agg2CreateImage()");
    return image;
  }
  if (width > 0 && height > 0) {
    image = (imageObj *)calloc(1, sizeof(imageObj));
    MS_CHECK_ALLOC(image, sizeof(imageObj), NULL);
    AGG2Renderer *r = new AGG2Renderer();

    /* Compute size on 64bit and check that it is compatible of the platform
     * size_t */
    const AGG_INT64U bufSize64 =
        (AGG_INT64U)width * height * 4 * sizeof(band_type);
    if (bufSize64 > std::numeric_limits<size_t>::max() / 2) {
      msSetError(MS_MEMERR,
                 "%s: %d: Out of memory allocating " AGG_INT64U_FRMT
                 " bytes.\n",
                 "agg2CreateImage()", __FILE__, __LINE__, bufSize64);
      free(image);
      delete r;
      return NULL;
    }

    const size_t bufSize = (size_t)bufSize64;
    try {
      r->buffer.resize(bufSize / sizeof(band_type));
    } catch (const std::bad_alloc &) {
      msSetError(MS_MEMERR,
                 "%s: %d: Out of memory allocating " AGG_INT64U_FRMT
                 " bytes.\n",
                 "agg2CreateImage()", __FILE__, __LINE__, bufSize64);
      free(image);
      delete r;
      return NULL;
    }
    r->m_rendering_buffer.attach(r->buffer.data(), width, height, width * 4);
    r->m_pixel_format.attach(r->m_rendering_buffer);
    r->m_compop_pixel_format.attach(r->m_rendering_buffer);
    r->m_renderer_base.attach(r->m_pixel_format);
    r->m_compop_renderer_base.attach(r->m_compop_pixel_format);
    r->m_renderer_scanline.attach(r->m_renderer_base);
    r->m_renderer_scanline_aliased.attach(r->m_renderer_base);
    r->default_gamma = atof(msGetOutputFormatOption(format, "GAMMA", "0.75"));
    if (r->default_gamma <= 0.0 || r->default_gamma >= 1.0) {
      r->default_gamma = 0.75;
    }
    r->gamma_function.set(0, r->default_gamma);
    r->m_rasterizer_aa_gamma.gamma(r->gamma_function);
    if (bg && !format->transparent)
      r->m_renderer_base.clear(aggColor(bg));
    else
      r->m_renderer_base.clear(AGG_NO_COLOR);

    if (!bg || format->transparent || format->imagemode == MS_IMAGEMODE_RGBA) {
      r->use_alpha = true;
    } else {
      r->use_alpha = false;
    }
    image->img.plugin = (void *)r;
  } else {
    msSetError(MS_RENDERERERR, "Cannot create cairo image of size %dx%d.",
               "msImageCreateCairo()", width, height);
  }

  return image;
}

int agg2SaveImage(imageObj * /*img*/, mapObj * /*map*/, FILE * /*fp*/,
                  outputFormatObj * /*format*/) {

  return MS_FAILURE;
}

int agg2StartNewLayer(imageObj *img, mapObj * /*map*/, layerObj *layer) {
  AGG2Renderer *r = AGG_RENDERER(img);
  const char *sgamma = msLayerGetProcessingKey(layer, "GAMMA");
  double gamma;
  if (sgamma) {
    gamma = atof(sgamma);
    if (gamma <= 0 || gamma >= 1)
      gamma = 0.75;
  } else {
    gamma = r->default_gamma;
  }
  if (r->gamma_function.end() != gamma) {
    r->gamma_function.end(gamma);
    r->m_rasterizer_aa_gamma.gamma(r->gamma_function);
  }
  return MS_SUCCESS;
}

int agg2CloseNewLayer(imageObj * /*img*/, mapObj * /*map*/,
                      layerObj * /*layer*/) {
  return MS_SUCCESS;
}

int agg2FreeImage(imageObj *image) {
  AGG2Renderer *r = AGG_RENDERER(image);
  delete r;
  image->img.plugin = NULL;
  return MS_SUCCESS;
}

int agg2FreeSymbol(symbolObj * /*symbol*/) { return MS_SUCCESS; }

int agg2InitCache(void **vcache) {
  aggRendererCache *cache = new aggRendererCache();
  *vcache = (void *)cache;
  return MS_SUCCESS;
}

int agg2Cleanup(void *vcache) {
  aggRendererCache *cache = (aggRendererCache *)vcache;
  delete cache;
  return MS_SUCCESS;
}

// ------------------------------------------------------------------------
// Function to create a custom hatch symbol based on an arbitrary angle.
// ------------------------------------------------------------------------
static mapserver::path_storage createHatch(double ox, double oy, double rx,
                                           double ry, int sx, int sy,
                                           double angle, double step) {
  mapserver::path_storage path;
  // restrict the angle to [0 180[, i.e ]-pi/2,pi/2] in radians
  angle = fmod(angle, 360.0);
  if (angle < 0)
    angle += 360;
  if (angle >= 180)
    angle -= 180;

  // treat 2 easy cases which would cause divide by 0 in generic case
  if (angle == 0) {
    double y0 = step - fmod(oy - ry, step);
    if ((oy - ry) < 0) {
      y0 -= step;
    }
    for (double y = y0; y < sy; y += step) {
      path.move_to(0, y);
      path.line_to(sx, y);
    }
    return path;
  }
  if (angle == 90) {
    double x0 = step - fmod(ox - rx, step);
    if ((ox - rx) < 0) {
      x0 -= step;
    }
    for (double x = x0; x < sx; x += step) {
      path.move_to(x, 0);
      path.line_to(x, sy);
    }
    return path;
  }

  double theta = (90 - angle) * MS_DEG_TO_RAD; /* theta in ]-pi/2 , pi/2] */
  double ct = cos(theta);
  double st = sin(theta);
  double invct = 1.0 / ct;
  double invst = 1.0 / st;
  double
      r0; /* distance from first hatch line to the top-left (if angle in 0,pi/2)
               or bottom-left (if angle in -pi/2,0) corner of the hatch bbox */
  double rmax = sqrt((
      double)(sx * sx +
              sy *
                  sy)); /* distance to the furthest hatch we will have to create
TODO: this could be optimized for bounding boxes where width is very different
than height for certain hatch angles */
  double rref =
      rx * ct + ry * st; /* distance to the line passing through the refpoint,
                            origin is (0,0) of the imageObj */
  double
      rcorner; /* distance to the line passing through the topleft or bottomleft
                  corner of the hatch bbox (origin is (0,0) of imageObj) */

  /* calculate the distance from the refpoint to the top right of the path */
  if (angle < 90) {
    rcorner = ox * ct + oy * st;
    r0 = step - fmod(rcorner - rref, step);
    if (rcorner - rref < 0)
      r0 -= step;
  } else {
    rcorner = ox * ct + (oy + sy) * st;
    r0 = step - fmod(rcorner - rref, step);
    if (rcorner - rref < 0)
      r0 -= step;
    st = -st;
    invst = -invst;
  }

  // parametrize each line as r = x.cos(theta) + y.sin(theta)
  for (double r = r0; r < rmax; r += step) {
    int inter = 0;
    double x, y;
    double pt[4]; // array to store the coordinates of intersection of the line
                  // with the sides
    // in the general case there will only be two intersections
    // so pt[4] should be sufficient to store the coordinates of the
    // intersection, but we allocate pt[8] to treat the special and
    // rare/unfortunate case when the line is a perfect diagonal (and therfore
    // intersects all four sides) note that the order for testing is important
    // in this case so that the first two intersection points actually
    // correspond to the diagonal and not a degenerate line

    // test for intersection with each side

    y = r * invst;
    x = 0; // test for intersection with left of image
    if (y >= 0 && y <= sy) {
      pt[2 * inter] = x;
      pt[2 * inter + 1] = y;
      inter++;
    }
    x = sx;
    y = (r - sx * ct) * invst; // test for intersection with right of image
    if (y >= 0 && y <= sy) {
      pt[2 * inter] = x;
      pt[2 * inter + 1] = y;
      inter++;
    }
    if (inter < 2) {
      y = 0;
      x = r * invct; // test for intersection with top of image
      if (x >= 0 && x <= sx) {
        pt[2 * inter] = x;
        pt[2 * inter + 1] = y;
        inter++;
      }
    }
    if (inter < 2) {
      y = sy;
      x = (r - sy * st) * invct; // test for intersection with bottom of image
      if (x >= 0 && x <= sx) {
        pt[2 * inter] = x;
        pt[2 * inter + 1] = y;
        inter++;
      }
    }
    if (inter == 2 && (pt[0] != pt[2] || pt[1] != pt[3])) {
      // the line intersects with two sides of the image, it should therefore be
      // drawn
      if (angle < 90) {
        path.move_to(pt[0], pt[1]);
        path.line_to(pt[2], pt[3]);
      } else {
        path.move_to(pt[0], sy - pt[1]);
        path.line_to(pt[2], sy - pt[3]);
      }
    }
  }
  return path;
}

template <class VertexSource>
int renderPolygonHatches(imageObj *img, VertexSource &clipper,
                         colorObj *color) {
  if (img->format->renderer == MS_RENDER_WITH_AGG) {
    AGG2Renderer *r = AGG_RENDERER(img);
    r->m_rasterizer_aa_gamma.reset();
    r->m_rasterizer_aa_gamma.filling_rule(mapserver::fill_non_zero);
    r->m_rasterizer_aa_gamma.add_path(clipper);
    r->m_renderer_scanline.color(aggColor(color));
    mapserver::render_scanlines(r->m_rasterizer_aa_gamma, r->sl_poly,
                                r->m_renderer_scanline);
  } else {
    shapeObj shape;
    msInitShape(&shape);
    int allocated = 20;
    lineObj line;
    shape.line = &line;
    shape.numlines = 1;
    shape.line[0].point =
        (pointObj *)msSmallCalloc(allocated, sizeof(pointObj));
    shape.line[0].numpoints = 0;
    double x = 0, y = 0;
    unsigned int cmd;
    clipper.rewind(0);
    while ((cmd = clipper.vertex(&x, &y)) != mapserver::path_cmd_stop) {
      switch (cmd) {
      case mapserver::path_cmd_line_to:
        if (shape.line[0].numpoints == allocated) {
          allocated *= 2;
          shape.line[0].point = (pointObj *)msSmallRealloc(
              shape.line[0].point, allocated * sizeof(pointObj));
        }
        shape.line[0].point[shape.line[0].numpoints].x = x;
        shape.line[0].point[shape.line[0].numpoints].y = y;
        shape.line[0].numpoints++;
        break;
      case mapserver::path_cmd_move_to:
        shape.line[0].point[0].x = x;
        shape.line[0].point[0].y = y;
        shape.line[0].numpoints = 1;
        break;
      case mapserver::path_cmd_end_poly | mapserver::path_flags_close:
        if (shape.line[0].numpoints > 2) {
          if (MS_UNLIKELY(MS_FAILURE == MS_IMAGE_RENDERER(img)->renderPolygon(
                                            img, &shape, color))) {
            free(shape.line[0].point);
            return MS_FAILURE;
          }
        }
        break;
      default:
        assert(0); // WTF?
      }
    }
    free(shape.line[0].point);
  }
  return MS_SUCCESS;
}

int msHatchPolygon(imageObj *img, shapeObj *poly, double spacing, double width,
                   double *pattern, int patternlength, double angle,
                   colorObj *color) {
  assert(MS_RENDERER_PLUGIN(img->format));
  msComputeBounds(poly);

  /* amount we should expand the bounding box by */
  double exp = width * 0.7072;

  /* width and height of the bounding box we will be creating the hatch in */
  int pw = (int)(poly->bounds.maxx - poly->bounds.minx + exp * 2) + 1;
  int ph = (int)(poly->bounds.maxy - poly->bounds.miny + exp * 2) + 1;

  /* position of the top-left corner of the bounding box */
  double ox = poly->bounds.minx - exp;
  double oy = poly->bounds.miny - exp;

  // create a rectangular hatch of size pw,ph starting at 0,0
  // the created hatch is of the size of the shape's bounding box
  mapserver::path_storage hatch =
      createHatch(ox, oy, img->refpt.x, img->refpt.y, pw, ph, angle, spacing);
  if (hatch.total_vertices() <= 0)
    return MS_SUCCESS;

  // translate the hatch so it overlaps the current shape
  hatch.transform(mapserver::trans_affine_translation(ox, oy));

  polygon_adaptor polygons(poly);

  if (patternlength > 1) {
    // dash the color-hatch and render it clipped by the shape
    mapserver::conv_dash<mapserver::path_storage> dash(hatch);
    mapserver::conv_stroke<mapserver::conv_dash<mapserver::path_storage>>
        stroke(dash);
    for (int i = 0; i < patternlength; i += 2) {
      if (i < patternlength - 1) {
        dash.add_dash(pattern[i], pattern[i + 1]);
      }
    }
    stroke.width(width);
    stroke.line_cap(mapserver::butt_cap);
    mapserver::conv_clipper<
        polygon_adaptor,
        mapserver::conv_stroke<mapserver::conv_dash<mapserver::path_storage>>>
        clipper(polygons, stroke, mapserver::clipper_and);
    renderPolygonHatches(img, clipper, color);
  } else {
    // render the hatch clipped by the shape
    mapserver::conv_stroke<mapserver::path_storage> stroke(hatch);
    stroke.width(width);
    stroke.line_cap(mapserver::butt_cap);
    mapserver::conv_clipper<polygon_adaptor,
                            mapserver::conv_stroke<mapserver::path_storage>>
        clipper(polygons, stroke, mapserver::clipper_and);
    renderPolygonHatches(img, clipper, color);
  }

  // assert(prevCmd == mapserver::path_cmd_line_to);
  // delete lines;
  return MS_SUCCESS;
}

#ifdef USE_PIXMAN
static pixman_op_t ms2pixman_compop(CompositingOperation c) {
  switch (c) {
  case MS_COMPOP_CLEAR:
    return PIXMAN_OP_CLEAR;
  case MS_COMPOP_SRC:
    return PIXMAN_OP_SRC;
  case MS_COMPOP_DST:
    return PIXMAN_OP_DST;
  case MS_COMPOP_SRC_OVER:
    return PIXMAN_OP_OVER;
  case MS_COMPOP_DST_OVER:
    return PIXMAN_OP_OVER_REVERSE;
  case MS_COMPOP_SRC_IN:
    return PIXMAN_OP_IN;
  case MS_COMPOP_DST_IN:
    return PIXMAN_OP_IN_REVERSE;
  case MS_COMPOP_SRC_OUT:
    return PIXMAN_OP_OUT;
  case MS_COMPOP_DST_OUT:
    return PIXMAN_OP_OUT_REVERSE;
  case MS_COMPOP_SRC_ATOP:
    return PIXMAN_OP_ATOP;
  case MS_COMPOP_DST_ATOP:
    return PIXMAN_OP_ATOP_REVERSE;
  case MS_COMPOP_XOR:
    return PIXMAN_OP_XOR;
  case MS_COMPOP_PLUS:
    return PIXMAN_OP_ADD;
  case MS_COMPOP_MULTIPLY:
    return PIXMAN_OP_MULTIPLY;
  case MS_COMPOP_SCREEN:
    return PIXMAN_OP_SCREEN;
  case MS_COMPOP_OVERLAY:
    return PIXMAN_OP_OVERLAY;
  case MS_COMPOP_DARKEN:
    return PIXMAN_OP_DARKEN;
  case MS_COMPOP_LIGHTEN:
    return PIXMAN_OP_LIGHTEN;
  case MS_COMPOP_COLOR_DODGE:
    return PIXMAN_OP_COLOR_DODGE;
  case MS_COMPOP_COLOR_BURN:
    return PIXMAN_OP_COLOR_DODGE;
  case MS_COMPOP_HARD_LIGHT:
    return PIXMAN_OP_HARD_LIGHT;
  case MS_COMPOP_SOFT_LIGHT:
    return PIXMAN_OP_SOFT_LIGHT;
  case MS_COMPOP_DIFFERENCE:
    return PIXMAN_OP_DIFFERENCE;
  case MS_COMPOP_EXCLUSION:
    return PIXMAN_OP_EXCLUSION;
  case MS_COMPOP_HSL_HUE:
    return PIXMAN_OP_HSL_HUE;
  case MS_COMPOP_HSL_LUMINOSITY:
    return PIXMAN_OP_HSL_LUMINOSITY;
  case MS_COMPOP_HSL_SATURATION:
    return PIXMAN_OP_HSL_SATURATION;
  case MS_COMPOP_HSL_COLOR:
    return PIXMAN_OP_HSL_COLOR;
  case MS_COMPOP_INVERT:
  case MS_COMPOP_INVERT_RGB:
  case MS_COMPOP_MINUS:
  case MS_COMPOP_CONTRAST:
  default:
    return PIXMAN_OP_OVER;
  }
}
#endif

static mapserver::comp_op_e ms2agg_compop(CompositingOperation c) {
  switch (c) {
  case MS_COMPOP_CLEAR:
    return mapserver::comp_op_clear;
  case MS_COMPOP_SRC:
    return mapserver::comp_op_src;
  case MS_COMPOP_DST:
    return mapserver::comp_op_dst;
  case MS_COMPOP_SRC_OVER:
    return mapserver::comp_op_src_over;
  case MS_COMPOP_DST_OVER:
    return mapserver::comp_op_dst_over;
  case MS_COMPOP_SRC_IN:
    return mapserver::comp_op_src_in;
  case MS_COMPOP_DST_IN:
    return mapserver::comp_op_dst_in;
  case MS_COMPOP_SRC_OUT:
    return mapserver::comp_op_src_out;
  case MS_COMPOP_DST_OUT:
    return mapserver::comp_op_dst_out;
  case MS_COMPOP_SRC_ATOP:
    return mapserver::comp_op_src_atop;
  case MS_COMPOP_DST_ATOP:
    return mapserver::comp_op_dst_atop;
  case MS_COMPOP_XOR:
    return mapserver::comp_op_xor;
  case MS_COMPOP_PLUS:
    return mapserver::comp_op_plus;
  case MS_COMPOP_MINUS:
    return mapserver::comp_op_minus;
  case MS_COMPOP_MULTIPLY:
    return mapserver::comp_op_multiply;
  case MS_COMPOP_SCREEN:
    return mapserver::comp_op_screen;
  case MS_COMPOP_OVERLAY:
    return mapserver::comp_op_overlay;
  case MS_COMPOP_DARKEN:
    return mapserver::comp_op_darken;
  case MS_COMPOP_LIGHTEN:
    return mapserver::comp_op_lighten;
  case MS_COMPOP_COLOR_DODGE:
    return mapserver::comp_op_color_dodge;
  case MS_COMPOP_COLOR_BURN:
    return mapserver::comp_op_color_burn;
  case MS_COMPOP_HARD_LIGHT:
    return mapserver::comp_op_hard_light;
  case MS_COMPOP_SOFT_LIGHT:
    return mapserver::comp_op_soft_light;
  case MS_COMPOP_DIFFERENCE:
    return mapserver::comp_op_difference;
  case MS_COMPOP_EXCLUSION:
    return mapserver::comp_op_exclusion;
  case MS_COMPOP_CONTRAST:
    return mapserver::comp_op_contrast;
  case MS_COMPOP_INVERT:
    return mapserver::comp_op_invert;
  case MS_COMPOP_INVERT_RGB:
    return mapserver::comp_op_invert_rgb;
  case MS_COMPOP_HSL_HUE:
    return mapserver::comp_op_hsl_hue;
  case MS_COMPOP_HSL_LUMINOSITY:
    return mapserver::comp_op_hsl_luminosity;
  case MS_COMPOP_HSL_SATURATION:
    return mapserver::comp_op_hsl_saturation;
  case MS_COMPOP_HSL_COLOR:
    return mapserver::comp_op_hsl_color;
  default:
    return mapserver::comp_op_src_over;
  }
}

#ifdef USE_PIXMAN
static int aggCompositeRasterBufferPixman(imageObj *dest,
                                          rasterBufferObj *overlay,
                                          CompositingOperation comp,
                                          int opacity) {
  assert(overlay->type == MS_BUFFER_BYTE_RGBA);
  AGG2Renderer *r = AGG_RENDERER(dest);
  pixman_image_t *si = pixman_image_create_bits(
      PIXMAN_a8r8g8b8, overlay->width, overlay->height,
      (uint32_t *)overlay->data.rgba.pixels, overlay->data.rgba.row_step);
  pixman_image_t *bi = pixman_image_create_bits(
      PIXMAN_a8r8g8b8, dest->width, dest->height,
      reinterpret_cast<uint32_t *>(&(r->buffer[0])), dest->width * 4);
  pixman_image_t *alpha_mask_i = NULL, *alpha_mask_i_ptr;
  pixman_image_set_filter(si, PIXMAN_FILTER_NEAREST, NULL, 0);
  unsigned char *alpha_mask = NULL;
  if (opacity > 0) {
    if (opacity == 100) {
      alpha_mask_i_ptr = NULL;
    } else {
      unsigned char alpha = (unsigned char)(opacity * 2.55);
      if (!alpha_mask_i) {
        alpha_mask = (unsigned char *)msSmallMalloc(dest->width * dest->height);
        alpha_mask_i =
            pixman_image_create_bits(PIXMAN_a8, dest->width, dest->height,
                                     (uint32_t *)alpha_mask, dest->width);
      }
      memset(alpha_mask, alpha, dest->width * dest->height);
      alpha_mask_i_ptr = alpha_mask_i;
    }
    pixman_image_composite(ms2pixman_compop(comp), si, alpha_mask_i_ptr, bi, 0,
                           0, 0, 0, 0, 0, dest->width, dest->height);
  }
  pixman_image_unref(si);
  pixman_image_unref(bi);
  if (alpha_mask_i) {
    pixman_image_unref(alpha_mask_i);
    msFree(alpha_mask);
  }
  return MS_SUCCESS;
}
#endif

static int aggCompositeRasterBufferNoPixman(imageObj *dest,
                                            rasterBufferObj *overlay,
                                            CompositingOperation comp,
                                            int opacity) {
  assert(overlay->type == MS_BUFFER_BYTE_RGBA);
  AGG2Renderer *r = AGG_RENDERER(dest);
  rendering_buffer b(overlay->data.rgba.pixels, overlay->width, overlay->height,
                     overlay->data.rgba.row_step);
  pixel_format pf(b);
  mapserver::comp_op_e comp_op = ms2agg_compop(comp);
  if (comp_op == mapserver::comp_op_src_over) {
    r->m_renderer_base.blend_from(pf, 0, 0, 0, unsigned(opacity * 2.55));
  } else {
    compop_pixel_format pixf(r->m_rendering_buffer);
    compop_renderer_base ren(pixf);
    pixf.comp_op(comp_op);
    ren.blend_from(pf, 0, 0, 0, unsigned(opacity * 2.55));
  }
  return MS_SUCCESS;
}

void msApplyBlurringCompositingFilter(rasterBufferObj *rb,
                                      unsigned int radius) {
  rendering_buffer b(rb->data.rgba.pixels, rb->width, rb->height,
                     rb->data.rgba.row_step);
  pixel_format pf(b);
  mapserver::stack_blur_rgba32(pf, radius, radius);
}

int msPopulateRendererVTableAGG(rendererVTableObj *renderer) {
  renderer->compositeRasterBuffer = &aggCompositeRasterBufferNoPixman;
#ifdef USE_PIXMAN
  const char *pszUsePixman = CPLGetConfigOption("MS_USE_PIXMAN", "YES");
  if (CPLTestBool(pszUsePixman)) {
    renderer->compositeRasterBuffer = &aggCompositeRasterBufferPixman;
  }
#endif
  renderer->supports_pixel_buffer = 1;
  renderer->use_imagecache = 0;
  renderer->supports_clipping = 0;
  renderer->supports_svg = 0;
  renderer->default_transform_mode = MS_TRANSFORM_SIMPLIFY;
  agg2InitCache(&(MS_RENDERER_CACHE(renderer)));
  renderer->cleanup = agg2Cleanup;
  renderer->renderLine = &agg2RenderLine;

  renderer->renderPolygon = &agg2RenderPolygon;
  renderer->renderPolygonTiled = &agg2RenderPolygonTiled;
  renderer->renderLineTiled = &agg2RenderLineTiled;

  renderer->renderGlyphs = &agg2RenderGlyphsPath;

  renderer->renderVectorSymbol = &agg2RenderVectorSymbol;

  renderer->renderPixmapSymbol = &agg2RenderPixmapSymbol;

  renderer->renderEllipseSymbol = &agg2RenderEllipseSymbol;

  renderer->renderTile = &agg2RenderTile;

  renderer->getRasterBufferHandle = &aggGetRasterBufferHandle;
  renderer->getRasterBufferCopy = aggGetRasterBufferCopy;
  renderer->initializeRasterBuffer = aggInitializeRasterBuffer;

  renderer->mergeRasterBuffer = &agg2MergeRasterBuffer;
  renderer->loadImageFromFile = msLoadMSRasterBufferFromFile;
  renderer->createImage = &agg2CreateImage;
  renderer->saveImage = &agg2SaveImage;

  renderer->startLayer = &agg2StartNewLayer;
  renderer->endLayer = &agg2CloseNewLayer;

  renderer->freeImage = &agg2FreeImage;
  renderer->freeSymbol = &agg2FreeSymbol;

  return MS_SUCCESS;
}

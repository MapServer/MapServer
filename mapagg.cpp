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
#include "renderers/agg/include/agg_span_image_filter_rgba.h"
#include "renderers/agg/include/agg_pattern_filters_rgba.h"
#include "renderers/agg/include/agg_span_image_filter_rgb.h"
#include "renderers/agg/include/agg_image_accessors.h"
#include "renderers/agg/include/agg_conv_stroke.h"
#include "renderers/agg/include/agg_conv_dash.h"
#include "renderers/agg/include/agg_path_storage.h"
#include "renderers/agg/include/agg_font_freetype.h"
#include "renderers/agg/include/agg_conv_contour.h"
#include "renderers/agg/include/agg_ellipse.h"
#include "renderers/agg/include/agg_gamma_functions.h"

#include "renderers/agg/include/agg_rasterizer_outline_aa.h"
#include "renderers/agg/include/agg_renderer_outline_aa.h"
#include "renderers/agg/include/agg_renderer_outline_image.h"
#include "renderers/agg/include/agg_span_pattern_rgba.h"
#include "renderers/agg/include/agg_span_image_filter_rgba.h"
#include "renderers/agg/include/agg_glyph_raster_bin.h"
#include "renderers/agg/include/agg_renderer_raster_text.h"
#include "renderers/agg/include/agg_embedded_raster_fonts.h"

#include "renderers/agg/include/agg_conv_clipper.h"

#ifdef AGG_ALIASED_ENABLED
#include "renderers/agg/include/agg_renderer_primitives.h"
#include "renderers/agg/include/agg_rasterizer_outline.h"
#endif

typedef mapserver::order_bgra band_order;

#define AGG_LINESPACE 1.33

typedef mapserver::int8u band_type;
typedef mapserver::rgba8 color_type;
typedef mapserver::pixel32_type pixel_type;

typedef mapserver::blender_rgba_pre<color_type, band_order> blender_pre;

typedef mapserver::pixfmt_alpha_blend_rgba<blender_pre, mapserver::rendering_buffer, pixel_type> pixel_format;
typedef mapserver::rendering_buffer rendering_buffer;
typedef mapserver::renderer_base<pixel_format> renderer_base;
typedef mapserver::renderer_scanline_aa_solid<renderer_base> renderer_scanline;
typedef mapserver::rasterizer_scanline_aa<> rasterizer_scanline;
typedef mapserver::font_engine_freetype_int16 font_engine_type;
typedef mapserver::font_cache_manager<font_engine_type> font_manager_type;
typedef mapserver::conv_curve<font_manager_type::path_adaptor_type> font_curve_type;

#ifdef AGG_ALIASED_ENABLED
typedef mapserver::renderer_primitives<renderer_base> renderer_primitives;
typedef mapserver::rasterizer_outline<renderer_primitives> rasterizer_outline;
#endif
static color_type AGG_NO_COLOR = color_type(0, 0, 0, 0);

const mapserver::int8u* rasterfonts[]= {
  mapserver::mcs5x10_mono, /*gd tiny. gse5x7 is a bit less high than gd tiny*/
  mapserver::mcs5x11_mono, /*gd small*/
  mapserver::mcs6x11_mono, /*gd medium*/
  mapserver::mcs7x12_mono_low, /*gd large*/
  mapserver::mcs7x12_mono_high /*gd huge*/
};

fontMetrics rasterfont_sizes[] = {
  {5,mapserver::mcs5x10_mono[0]},
  {5,mapserver::mcs5x11_mono[0]},
  {6,mapserver::mcs6x11_mono[0]},
  {7,mapserver::mcs7x12_mono_low[0]},
  {7,mapserver::mcs7x12_mono_high[0]}
};

#define aggColor(c) mapserver::rgba8_pre(c->red, c->green, c->blue, c->alpha)

class aggRendererCache
{
public:
  font_engine_type m_feng;
  font_manager_type m_fman;
  aggRendererCache(): m_fman(m_feng) {}
};

class AGG2Renderer
{
public:

  AGG2Renderer()
#ifdef AGG_ALIASED_ENABLED
    :
    m_renderer_primitives(m_renderer_base),
    m_rasterizer_primitives(m_renderer_primitives)
#endif
  {}

  band_type* buffer;
  rendering_buffer m_rendering_buffer;
  pixel_format m_pixel_format;
  renderer_base m_renderer_base;
  renderer_scanline m_renderer_scanline;
#ifdef AGG_ALIASED_ENABLED
  renderer_primitives m_renderer_primitives;
  rasterizer_outline m_rasterizer_primitives;
#endif
  rasterizer_scanline m_rasterizer_aa;
  rasterizer_scanline m_rasterizer_aa_gamma;
  mapserver::scanline_p8 sl_poly; /*packed scanlines, works faster when the area is larger
    than the perimeter, in number of pixels*/
  mapserver::scanline_u8 sl_line; /*unpacked scanlines, works faster if the area is roughly
    equal to the perimeter, in number of pixels*/
  bool use_alpha;
};

#define AGG_RENDERER(image) ((AGG2Renderer*) (image)->img.plugin)

template<class VertexSource>
static void applyCJC(VertexSource &stroke, int caps, int joins)
{
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

inline int aggLoadFont(aggRendererCache *cache, char *font, double size)
{
  if(!cache->m_feng.name() || strcmp(cache->m_feng.name(),font)) {
    if (!cache->m_feng.load_font(font, 0, mapserver::glyph_ren_outline)) {
      msSetError(MS_TTFERR, "AGG error loading font (%s)", "aggLoadFont()", font);
      return MS_FAILURE;
    }
    if(!cache->m_feng.hinting())
      cache->m_feng.hinting(true);
    if(cache->m_feng.resolution() != 96)
      cache->m_feng.resolution(96);
    if(!cache->m_feng.flip_y())
      cache->m_feng.flip_y(true);
    cache->m_feng.height(size);
  } else {
    if(cache->m_feng.height()!=size)
      cache->m_feng.height(size);
  }
  return MS_SUCCESS;
}

int agg2RenderLine(imageObj *img, shapeObj *p, strokeStyleObj *style)
{

  AGG2Renderer *r = AGG_RENDERER(img);
  line_adaptor lines = line_adaptor(p);

#ifdef AGG_ALIASED_ENABLED
  r->m_rasterizer_primitives.reset();
  r->m_renderer_primitives.line_color(aggColor(style->color));
  r->m_rasterizer_primitives.add_path(lines);
  return MS_SUCCESS;
#endif

  r->m_rasterizer_aa.reset();
  r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
  r->m_renderer_scanline.color(aggColor(style->color));

  if (style->patternlength <= 0) {
    mapserver::conv_stroke<line_adaptor> stroke(lines);
    stroke.width(style->width);
    if(style->width>1) {
      applyCJC(stroke, style->linecap, style->linejoin);
    } else {
      stroke.inner_join(mapserver::inner_bevel);
      stroke.line_join(mapserver::bevel_join);
    }
    r->m_rasterizer_aa.add_path(stroke);
  } else {
    mapserver::conv_dash<line_adaptor> dash(lines);
    mapserver::conv_stroke<mapserver::conv_dash<line_adaptor> > stroke_dash(dash);
    int patt_length = 0;
    for (int i = 0; i < style->patternlength; i += 2) {
      if (i < style->patternlength - 1) {
        dash.add_dash(MS_MAX(1,MS_NINT(style->pattern[i])),
                      MS_MAX(1,MS_NINT(style->pattern[i + 1])));
        if(style->patternoffset) {
          patt_length += MS_MAX(1,MS_NINT(style->pattern[i])) +
                         MS_MAX(1,MS_NINT(style->pattern[i + 1]));
        }
      }
    }
    if(style->patternoffset > 0) {
      dash.dash_start(patt_length - style->patternoffset);
    }
    stroke_dash.width(style->width);
    if(style->width>1) {
      applyCJC(stroke_dash, style->linecap, style->linejoin);
    } else {
      stroke_dash.inner_join(mapserver::inner_bevel);
      stroke_dash.line_join(mapserver::bevel_join);
    }
    r->m_rasterizer_aa.add_path(stroke_dash);
  }
  mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
  return MS_SUCCESS;
}

int agg2RenderLineTiled(imageObj *img, shapeObj *p, imageObj * tile)
{

  mapserver::pattern_filter_bilinear_rgba8 fltr;
  typedef mapserver::line_image_pattern<mapserver::pattern_filter_bilinear_rgba8> pattern_type;
  typedef mapserver::renderer_outline_image<renderer_base, pattern_type> renderer_img_type;
  typedef mapserver::rasterizer_outline_aa<renderer_img_type, mapserver::line_coord_sat> rasterizer_img_type;
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

int agg2RenderPolygon(imageObj *img, shapeObj *p, colorObj * color)
{
  AGG2Renderer *r = AGG_RENDERER(img);
  polygon_adaptor polygons(p);
  r->m_rasterizer_aa_gamma.reset();
  r->m_rasterizer_aa_gamma.filling_rule(mapserver::fill_even_odd);
  r->m_rasterizer_aa_gamma.add_path(polygons);
  r->m_renderer_scanline.color(aggColor(color));
  mapserver::render_scanlines(r->m_rasterizer_aa_gamma, r->sl_poly, r->m_renderer_scanline);
  return MS_SUCCESS;
}

int agg2RenderPolygonTiled(imageObj *img, shapeObj *p, imageObj * tile)
{
  assert(img->format->renderer == tile->format->renderer);

  AGG2Renderer *r = AGG_RENDERER(img);
  AGG2Renderer *tileRenderer = AGG_RENDERER(tile);
  polygon_adaptor polygons(p);
  typedef mapserver::wrap_mode_repeat wrap_type;
  typedef mapserver::image_accessor_wrap<pixel_format,wrap_type,wrap_type> img_source_type;
  typedef mapserver::span_pattern_rgba<img_source_type> span_gen_type;
  mapserver::span_allocator<mapserver::rgba8> sa;
  r->m_rasterizer_aa.reset();
  r->m_rasterizer_aa.filling_rule(mapserver::fill_even_odd);
  img_source_type img_src(tileRenderer->m_pixel_format);
  span_gen_type sg(img_src, 0, 0);
  r->m_rasterizer_aa.add_path(polygons);
  mapserver::render_scanlines_aa(r->m_rasterizer_aa, r->sl_poly, r->m_renderer_base, sa , sg);
  return MS_SUCCESS;
}

int agg2RenderGlyphs(imageObj *img, double x, double y, labelStyleObj *style, char *text)
{
  AGG2Renderer *r = AGG_RENDERER(img);
  aggRendererCache *cache = (aggRendererCache*)MS_RENDERER_CACHE(MS_IMAGE_RENDERER(img));
  if(aggLoadFont(cache,style->fonts[0],style->size) == MS_FAILURE)
    return MS_FAILURE;
  r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);

  int curfontidx = 0;
  const mapserver::glyph_cache* glyph;
  int unicode;
  font_curve_type m_curves(cache->m_fman.path_adaptor());
  mapserver::trans_affine mtx;
  mtx *= mapserver::trans_affine_translation(-x, -y);
  /*agg angles are antitrigonometric*/
  mtx *= mapserver::trans_affine_rotation(-style->rotation);
  mtx *= mapserver::trans_affine_translation(x, y);

  double fx = x, fy = y;
  const char *utfptr = text;
  mapserver::path_storage glyphs;

  //first render all the glyphs to a path
  while (*utfptr) {
    if (*utfptr == '\r') {
      fx = x;
      utfptr++;
      continue;
    }
    if (*utfptr == '\n') {
      fx = x;
      fy += ceil(style->size * AGG_LINESPACE);
      utfptr++;
      continue;
    }
    utfptr += msUTF8ToUniChar(utfptr, &unicode);
    if(curfontidx != 0) {
      if(aggLoadFont(cache,style->fonts[0],style->size) == MS_FAILURE)
        return MS_FAILURE;
      curfontidx = 0;
    }

    glyph = cache->m_fman.glyph(unicode);

    if(!glyph || glyph->glyph_index == 0) {
      int i;
      for(i=1; i<style->numfonts; i++) {
        if(aggLoadFont(cache,style->fonts[i],style->size) == MS_FAILURE)
          return MS_FAILURE;
        curfontidx = i;
        glyph = cache->m_fman.glyph(unicode);
        if(glyph && glyph->glyph_index != 0) {
          break;
        }
      }
    }


    if (glyph) {
      //cache->m_fman.add_kerning(&fx, &fy);
      cache->m_fman.init_embedded_adaptors(glyph, fx, fy);
      mapserver::conv_transform<font_curve_type, mapserver::trans_affine> trans_c(m_curves, mtx);
      glyphs.concat_path(trans_c);
      fx += glyph->advance_x;
      fy += glyph->advance_y;
    }
  }

  if (style->outlinewidth) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    mapserver::conv_contour<mapserver::path_storage> cc(glyphs);
    cc.width(style->outlinewidth + 1);
    r->m_rasterizer_aa.add_path(cc);
    r->m_renderer_scanline.color(aggColor(style->outlinecolor));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
  }
  if (style->color) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    r->m_rasterizer_aa.add_path(glyphs);
    r->m_renderer_scanline.color(aggColor(style->color));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
  }

  return MS_SUCCESS;

}

int agg2RenderBitmapGlyphs(imageObj *img, double x, double y, labelStyleObj *style, char *text)
{
  typedef mapserver::glyph_raster_bin<color_type> glyph_gen;
  int size = MS_NINT(style->size);
  if(size<0 || size>4) {
    msSetError(MS_RENDERERERR,"invalid bitmap font size", "agg2RenderBitmapGlyphs()");
    return MS_FAILURE;
  }
  AGG2Renderer *r = AGG_RENDERER(img);
  glyph_gen glyph(0);
  mapserver::renderer_raster_htext_solid<renderer_base, glyph_gen> rt(r->m_renderer_base, glyph);
  glyph.font(rasterfonts[size]);
  int numlines=0;
  char **lines;
  /*masking out the out-of-range character codes*/
  int len;
  int cc_start = rasterfonts[size][2];
  int cc_end = cc_start + rasterfonts[size][3];
  if(msCountChars(text,'\n')) {
    if((lines = msStringSplit((const char*)text, '\n', &(numlines))) == NULL)
      return(-1);
  } else {
    lines = &text;
    numlines = 1;
  }
  y -= glyph.base_line();
  for(int n=0; n<numlines; n++) {
    len = strlen(lines[n]);
    for (int i = 0; i < len; i++)
      if (lines[n][i] < cc_start || lines[n][i] > cc_end)
        lines[n][i] = '.';
    if(style->outlinewidth > 0) {
      rt.color(aggColor(style->outlinecolor));
      for(int i=-1; i<=1; i++) {
        for(int j=-1; j<=1; j++) {
          if(i||j) {
            rt.render_text(x+i, y+j, lines[n], true);
          }
        }
      }
    }
    assert(style->color);
    rt.color(aggColor(style->color));
    rt.render_text(x, y, lines[n], true);
    y += glyph.height();
  }
  if(*lines != text)
    msFreeCharArray(lines, numlines);
  return MS_SUCCESS;
  return MS_SUCCESS;
}

int agg2RenderGlyphsLine(imageObj *img, labelPathObj *labelpath, labelStyleObj *style, char *text)
{
  AGG2Renderer *r = AGG_RENDERER(img);
  aggRendererCache *cache = (aggRendererCache*)MS_RENDERER_CACHE(MS_IMAGE_RENDERER(img));
  if(aggLoadFont(cache,style->fonts[0],style->size) == MS_FAILURE)
    return MS_FAILURE;
  r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);

  const mapserver::glyph_cache* glyph;
  int unicode;
  int curfontidx = 0;
  font_curve_type m_curves(cache->m_fman.path_adaptor());

  mapserver::path_storage glyphs;

  for (int i = 0; i < labelpath->path.numpoints; i++) {
    assert(text);
    mapserver::trans_affine mtx;
    mtx *= mapserver::trans_affine_translation(-labelpath->path.point[i].x,-labelpath->path.point[i].y);
    mtx *= mapserver::trans_affine_rotation(-labelpath->angles[i]);
    mtx *= mapserver::trans_affine_translation(labelpath->path.point[i].x,labelpath->path.point[i].y);
    text += msUTF8ToUniChar(text, &unicode);

    if(curfontidx != 0) {
      if(aggLoadFont(cache,style->fonts[0],style->size) == MS_FAILURE)
        return MS_FAILURE;
      curfontidx = 0;
    }

    glyph = cache->m_fman.glyph(unicode);

    if(!glyph || glyph->glyph_index == 0) {
      int i;
      for(i=1; i<style->numfonts; i++) {
        if(aggLoadFont(cache,style->fonts[i],style->size) == MS_FAILURE)
          return MS_FAILURE;
        curfontidx = i;
        glyph = cache->m_fman.glyph(unicode);
        if(glyph && glyph->glyph_index != 0) {
          break;
        }
      }
    }
    if (glyph) {
      cache->m_fman.init_embedded_adaptors(glyph, labelpath->path.point[i].x,labelpath->path.point[i].y);
      mapserver::conv_transform<font_curve_type, mapserver::trans_affine> trans_c(m_curves, mtx);
      glyphs.concat_path(trans_c);
    }
  }

  if (style->outlinewidth) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    mapserver::conv_contour<mapserver::path_storage> cc(glyphs);
    cc.width(style->outlinewidth + 1);
    r->m_rasterizer_aa.add_path(cc);
    r->m_renderer_scanline.color(aggColor(style->outlinecolor));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
  }
  if (style->color) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    r->m_rasterizer_aa.add_path(glyphs);
    r->m_renderer_scanline.color(aggColor(style->color));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
  }

  return MS_SUCCESS;
}

static mapserver::path_storage imageVectorSymbolAGG(symbolObj *symbol)
{
  mapserver::path_storage path;
  bool is_new=true;
  for(int i=0; i < symbol->numpoints; i++) {
    if((symbol->points[i].x == -99) && (symbol->points[i].y == -99)) { // (PENUP)
      is_new=true;
    } else {
      if(is_new) {
        path.move_to(symbol->points[i].x,symbol->points[i].y);
        is_new=false;
      } else {
        path.line_to(symbol->points[i].x,symbol->points[i].y);
      }
    }
  }
  return path;
}


int agg2RenderVectorSymbol(imageObj *img, double x, double y,
                           symbolObj *symbol, symbolStyleObj * style)
{
  AGG2Renderer *r = AGG_RENDERER(img);
  double ox = symbol->sizex * 0.5;
  double oy = symbol->sizey * 0.5;

  mapserver::path_storage path = imageVectorSymbolAGG(symbol);
  mapserver::trans_affine mtx;
  mtx *= mapserver::trans_affine_translation(-ox,-oy);
  mtx *= mapserver::trans_affine_scaling(style->scale);
  mtx *= mapserver::trans_affine_rotation(-style->rotation);
  mtx *= mapserver::trans_affine_translation(x, y);
  path.transform(mtx);
  if (style->color) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_even_odd);
    r->m_rasterizer_aa.add_path(path);
    r->m_renderer_scanline.color(aggColor(style->color));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_poly, r->m_renderer_scanline);
  }
  if(style->outlinecolor) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    r->m_renderer_scanline.color(aggColor(style->outlinecolor));
    mapserver::conv_stroke<mapserver::path_storage> stroke(path);
    stroke.width(style->outlinewidth);
    r->m_rasterizer_aa.add_path(stroke);
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_poly, r->m_renderer_scanline);
  }
  return MS_SUCCESS;
}

int agg2RenderPixmapSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj * style)
{
  AGG2Renderer *r = AGG_RENDERER(img);
  rasterBufferObj *pixmap = symbol->pixmap_buffer;
  assert(pixmap->type == MS_BUFFER_BYTE_RGBA);
  rendering_buffer b(pixmap->data.rgba.pixels,pixmap->width,pixmap->height,pixmap->data.rgba.row_step);
  pixel_format pf(b);

  r->m_rasterizer_aa.reset();
  r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
  if ( (style->rotation != 0 && style->rotation != MS_PI*2.)|| style->scale != 1) {
    mapserver::trans_affine image_mtx;
    image_mtx *= mapserver::trans_affine_translation(-(pf.width()/2.),-(pf.height()/2.));
    /*agg angles are antitrigonometric*/
    image_mtx *= mapserver::trans_affine_rotation(-style->rotation);
    image_mtx *= mapserver::trans_affine_scaling(style->scale);



    image_mtx *= mapserver::trans_affine_translation(x,y);
    image_mtx.invert();
    typedef mapserver::span_interpolator_linear<> interpolator_type;
    interpolator_type interpolator(image_mtx);
    mapserver::span_allocator<color_type> sa;

    // "hardcoded" bilinear filter
    //------------------------------------------
    typedef mapserver::span_image_filter_rgba_bilinear_clip<pixel_format, interpolator_type> span_gen_type;
    span_gen_type sg(pf, mapserver::rgba(0,0,0,0), interpolator);
    mapserver::path_storage pixmap_bbox;
    int ims_2 = MS_NINT(MS_MAX(pixmap->width,pixmap->height)*style->scale*1.415)/2+1;

    pixmap_bbox.move_to(x-ims_2,y-ims_2);
    pixmap_bbox.line_to(x+ims_2,y-ims_2);
    pixmap_bbox.line_to(x+ims_2,y+ims_2);
    pixmap_bbox.line_to(x-ims_2,y+ims_2);

    r->m_rasterizer_aa.add_path(pixmap_bbox);
    mapserver::render_scanlines_aa(r->m_rasterizer_aa, r->sl_poly, r->m_renderer_base, sa, sg);
  } else {
    //just copy the image at the correct location (we place the pixmap on
    //the nearest integer pixel to avoid blurring)
    r->m_renderer_base.blend_from(pf,0,MS_NINT(x-pixmap->width/2.),MS_NINT(y-pixmap->height/2.));
  }
  return MS_SUCCESS;
}

int agg2RenderEllipseSymbol(imageObj *image, double x, double y,
                            symbolObj *symbol, symbolStyleObj * style)
{
  AGG2Renderer *r = AGG_RENDERER(image);
  mapserver::path_storage path;
  mapserver::ellipse ellipse(x,y,symbol->sizex*style->scale/2,symbol->sizey*style->scale/2);
  path.concat_path(ellipse);
  if( style->rotation != 0) {
    mapserver::trans_affine mtx;
    mtx *= mapserver::trans_affine_translation(-x,-y);
    /*agg angles are antitrigonometric*/
    mtx *= mapserver::trans_affine_rotation(-style->rotation);
    mtx *= mapserver::trans_affine_translation(x,y);
    path.transform(mtx);
  }

  if(style->color) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_even_odd);
    r->m_rasterizer_aa.add_path(path);
    r->m_renderer_scanline.color(aggColor(style->color));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
  }
  if(style->outlinewidth) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    mapserver::conv_stroke<mapserver::path_storage> stroke(path);
    stroke.width(style->outlinewidth);
    r->m_rasterizer_aa.add_path(stroke);
    r->m_renderer_scanline.color(aggColor(style->outlinecolor));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_poly, r->m_renderer_scanline);
  }
  return MS_SUCCESS;
}

int agg2RenderTruetypeSymbol(imageObj *img, double x, double y,
                             symbolObj *symbol, symbolStyleObj * style)
{
  AGG2Renderer *r = AGG_RENDERER(img);
  aggRendererCache *cache = (aggRendererCache*)MS_RENDERER_CACHE(MS_IMAGE_RENDERER(img));
  if(aggLoadFont(cache,symbol->full_font_path,style->scale) == MS_FAILURE)
    return MS_FAILURE;

  int unicode;
  font_curve_type m_curves(cache->m_fman.path_adaptor());

  msUTF8ToUniChar(symbol->character, &unicode);
  const mapserver::glyph_cache* glyph = cache->m_fman.glyph(unicode);
  double ox = (glyph->bounds.x1 + glyph->bounds.x2) / 2.;
  double oy = (glyph->bounds.y1 + glyph->bounds.y2) / 2.;

  mapserver::trans_affine mtx = mapserver::trans_affine_translation(-ox, -oy);
  if(style->rotation)
    mtx *= mapserver::trans_affine_rotation(-style->rotation);
  mtx *= mapserver::trans_affine_translation(x, y);

  mapserver::path_storage glyphs;

  cache->m_fman.init_embedded_adaptors(glyph, 0,0);
  mapserver::conv_transform<font_curve_type, mapserver::trans_affine> trans_c(m_curves, mtx);
  glyphs.concat_path(trans_c);
  if (style->outlinecolor) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    mapserver::conv_contour<mapserver::path_storage> cc(glyphs);
    cc.auto_detect_orientation(true);
    cc.width(style->outlinewidth + 1);
    r->m_rasterizer_aa.add_path(cc);
    r->m_renderer_scanline.color(aggColor(style->outlinecolor));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
  }

  if (style->color) {
    r->m_rasterizer_aa.reset();
    r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
    r->m_rasterizer_aa.add_path(glyphs);
    r->m_renderer_scanline.color(aggColor(style->color));
    mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
  }
  return MS_SUCCESS;

}

int agg2RenderTile(imageObj *img, imageObj *tile, double x, double y)
{
  /*
  AGG2Renderer *imgRenderer = agg2GetRenderer(img);
  AGG2Renderer *tileRenderer = agg2GetRenderer(tile);
  */
  return MS_FAILURE;
}

int aggInitializeRasterBuffer(rasterBufferObj *rb, int width, int height, int mode)
{
  rb->type = MS_BUFFER_BYTE_RGBA;
  rb->data.rgba.pixel_step = 4;
  rb->data.rgba.row_step = rb->data.rgba.pixel_step * width;
  rb->width = width;
  rb->height = height;
  int nBytes = rb->data.rgba.row_step * height;
  rb->data.rgba.pixels = (band_type*)msSmallCalloc(nBytes,sizeof(band_type));
  rb->data.rgba.r = &(rb->data.rgba.pixels[band_order::R]);
  rb->data.rgba.g = &(rb->data.rgba.pixels[band_order::G]);
  rb->data.rgba.b = &(rb->data.rgba.pixels[band_order::B]);
  if(mode == MS_IMAGEMODE_RGBA) {
    rb->data.rgba.a = &(rb->data.rgba.pixels[band_order::A]);
  }
  return MS_SUCCESS;
}



int aggGetRasterBufferHandle(imageObj *img, rasterBufferObj * rb)
{
  AGG2Renderer *r = AGG_RENDERER(img);
  rb->type =MS_BUFFER_BYTE_RGBA;
  rb->data.rgba.pixels = r->buffer;
  rb->data.rgba.row_step = r->m_rendering_buffer.stride();
  rb->data.rgba.pixel_step = 4;
  rb->width = r->m_rendering_buffer.width();
  rb->height = r->m_rendering_buffer.height();
  rb->data.rgba.r = &(r->buffer[band_order::R]);
  rb->data.rgba.g = &(r->buffer[band_order::G]);
  rb->data.rgba.b = &(r->buffer[band_order::B]);
  if(r->use_alpha)
    rb->data.rgba.a = &(r->buffer[band_order::A]);
  else
    rb->data.rgba.a = NULL;
  return MS_SUCCESS;
}

int aggGetRasterBufferCopy(imageObj *img, rasterBufferObj *rb)
{
  AGG2Renderer *r = AGG_RENDERER(img);
  aggInitializeRasterBuffer(rb, img->width, img->height, MS_IMAGEMODE_RGBA);
  int nBytes = r->m_rendering_buffer.stride()*r->m_rendering_buffer.height();
  memcpy(rb->data.rgba.pixels,r->buffer, nBytes);
  return MS_SUCCESS;
}



int agg2MergeRasterBuffer(imageObj *dest, rasterBufferObj *overlay, double opacity, int srcX, int srcY,
                          int dstX, int dstY, int width, int height)
{
  assert(overlay->type == MS_BUFFER_BYTE_RGBA);
  rendering_buffer b(overlay->data.rgba.pixels, overlay->width, overlay->height, overlay->data.rgba.row_step);
  pixel_format pf(b);
  AGG2Renderer *r = AGG_RENDERER(dest);
  mapserver::rect_base<int> src_rect(srcX,srcY,srcX+width,srcY+height);
  r->m_renderer_base.blend_from(pf,&src_rect, dstX-srcX, dstY-srcY, unsigned(opacity * 255));
  return MS_SUCCESS;
}

/* image i/o */
imageObj *agg2CreateImage(int width, int height, outputFormatObj *format, colorObj * bg)
{
  imageObj *image = NULL;
  if (format->imagemode != MS_IMAGEMODE_RGB && format->imagemode != MS_IMAGEMODE_RGBA) {
    msSetError(MS_MISCERR,
               "AGG2 driver only supports RGB or RGBA pixel models.", "agg2CreateImage()");
    return image;
  }
  image = (imageObj *) calloc(1, sizeof (imageObj));
  MS_CHECK_ALLOC(image, sizeof (imageObj), NULL);
  AGG2Renderer *r = new AGG2Renderer();

  r->buffer = (band_type*)malloc(width * height * 4 * sizeof(band_type));
  if (r->buffer == NULL) {
    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "agg2CreateImage()",
               __FILE__, __LINE__, width * height * 4 * sizeof(band_type));
    free(image);
    return NULL;
  }
  r->m_rendering_buffer.attach(r->buffer, width, height, width * 4);
  r->m_pixel_format.attach(r->m_rendering_buffer);
  r->m_renderer_base.attach(r->m_pixel_format);
  r->m_renderer_scanline.attach(r->m_renderer_base);
  double gamma = atof(msGetOutputFormatOption( format, "GAMMA", "0.75" ));
  if(gamma > 0.0 && gamma < 1.0) {
    r->m_rasterizer_aa_gamma.gamma(mapserver::gamma_linear(0.0,gamma));
  }
  if( bg && !format->transparent )
    r->m_renderer_base.clear(aggColor(bg));
  else
    r->m_renderer_base.clear(AGG_NO_COLOR);

  if (!bg || format->transparent || format->imagemode == MS_IMAGEMODE_RGBA ) {
    r->use_alpha = true;
  } else {
    r->use_alpha = false;
  }
  image->img.plugin = (void*) r;

  return image;
}

int agg2SaveImage(imageObj *img, mapObj* map, FILE *fp, outputFormatObj * format)
{
  msSetError(MS_MISCERR, "AGG2 does not support direct image saving", "agg2SaveImage()");

  return MS_FAILURE;
}
/*...*/

/* helper functions */
int agg2GetTruetypeTextBBox(rendererVTableObj *renderer, char **fonts, int numfonts, double size, char *string,
                            rectObj *rect, double **advances,int bAdjustBaseline)
{

  aggRendererCache *cache = (aggRendererCache*)MS_RENDERER_CACHE(renderer);
  if(aggLoadFont(cache,fonts[0],size) == MS_FAILURE)
    return MS_FAILURE;
  int curfontidx = 0;

  int unicode, curGlyph = 1, numglyphs = 0;
  if (advances) {
    numglyphs = msGetNumGlyphs(string);
  }
  const mapserver::glyph_cache* glyph;
  string += msUTF8ToUniChar(string, &unicode);

  if(curfontidx != 0) {
    if(aggLoadFont(cache,fonts[0],size) == MS_FAILURE)
      return MS_FAILURE;
    curfontidx = 0;
  }
  glyph = cache->m_fman.glyph(unicode);
  if(!glyph || glyph->glyph_index == 0) {
    int i;
    for(i=1; i<numfonts; i++) {
      if(aggLoadFont(cache,fonts[i],size) == MS_FAILURE)
        return MS_FAILURE;
      curfontidx = i;
      glyph = cache->m_fman.glyph(unicode);
      if(glyph && glyph->glyph_index != 0) {
        break;
      }
    }
  }
  if (glyph) {
    rect->minx = glyph->bounds.x1;
    rect->maxx = glyph->bounds.x2;
    rect->miny = glyph->bounds.y1;
    rect->maxy = bAdjustBaseline?1:glyph->bounds.y2;
  } else
    return MS_FAILURE;
  if (advances) {
    *advances = (double*) malloc(numglyphs * sizeof (double));
    MS_CHECK_ALLOC(*advances, numglyphs * sizeof (double), MS_FAILURE);
    (*advances)[0] = glyph->advance_x;
  }
  double fx = glyph->advance_x, fy = glyph->advance_y;
  while (*string) {
    if (advances) {
      if (*string == '\r' || *string == '\n')
        (*advances)[curGlyph++] = -fx;
    }
    if (*string == '\r') {
      fx = 0;
      string++;
      continue;
    }
    if (*string == '\n') {
      fx = 0;
      fy += ceil(size * AGG_LINESPACE);
      string++;
      continue;
    }
    string += msUTF8ToUniChar(string, &unicode);
    if(curfontidx != 0) {
      if(aggLoadFont(cache,fonts[0],size) == MS_FAILURE)
        return MS_FAILURE;
      curfontidx = 0;
    }
    glyph = cache->m_fman.glyph(unicode);
    if(!glyph || glyph->glyph_index == 0) {
      int i;
      for(i=1; i<numfonts; i++) {
        if(aggLoadFont(cache,fonts[i],size) == MS_FAILURE)
          return MS_FAILURE;
        curfontidx = i;
        glyph = cache->m_fman.glyph(unicode);
        if(glyph && glyph->glyph_index != 0) {
          break;
        }
      }
    }
    if (glyph) {
      rect->minx = MS_MIN(rect->minx, fx+glyph->bounds.x1);
      rect->miny = MS_MIN(rect->miny, fy+glyph->bounds.y1);
      rect->maxx = MS_MAX(rect->maxx, fx+glyph->bounds.x2);
      rect->maxy = MS_MAX(rect->maxy, fy+(bAdjustBaseline?1:glyph->bounds.y2));

      fx += glyph->advance_x;
      fy += glyph->advance_y;
      if (advances) {
        (*advances)[curGlyph++] = glyph->advance_x;
      }
    }
  }
  return MS_SUCCESS;
}

int agg2StartNewLayer(imageObj *img, mapObj*map, layerObj *layer)
{
  return MS_SUCCESS;
}

int agg2CloseNewLayer(imageObj *img, mapObj *map, layerObj *layer)
{
  return MS_SUCCESS;
}

int agg2FreeImage(imageObj * image)
{
  AGG2Renderer *r = AGG_RENDERER(image);
  free(r->buffer);
  delete r;
  image->img.plugin = NULL;
  return MS_SUCCESS;
}

int agg2FreeSymbol(symbolObj * symbol)
{
  return MS_SUCCESS;
}

int agg2InitCache(void **vcache)
{
  aggRendererCache *cache = new aggRendererCache();
  *vcache = (void*)cache;
  return MS_SUCCESS;
}

int agg2Cleanup(void *vcache)
{
  aggRendererCache *cache = (aggRendererCache*)vcache;
  delete cache;
  return MS_SUCCESS;
}


// ------------------------------------------------------------------------
// Function to create a custom hatch symbol based on an arbitrary angle.
// ------------------------------------------------------------------------
static mapserver::path_storage createHatch(double ox, double oy,
    double rx, double ry,
    int sx, int sy, double angle, double step)
{
  mapserver::path_storage path;
  //restrict the angle to [0 180[, i.e ]-pi/2,pi/2] in radians
  angle = fmod(angle, 360.0);
  if(angle < 0) angle += 360;
  if(angle >= 180) angle -= 180;

  //treat 2 easy cases which would cause divide by 0 in generic case
  if(angle==0) {
    double y0 = step-fmod(oy-ry,step);
    if((oy - ry) < 0) {
      y0 -= step;
    }
    for(double y=y0; y<sy; y+=step) {
      path.move_to(0,y);
      path.line_to(sx,y);
    }
    return path;
  }
  if(angle==90) {
    double x0 = step-fmod(ox-rx,step);
    if((ox - rx) < 0) {
      x0 -= step;
    }
    for(double x=x0; x<sx; x+=step) {
      path.move_to(x,0);
      path.line_to(x,sy);
    }
    return path;
  }


  double theta = (90-angle)*MS_DEG_TO_RAD; /* theta in ]-pi/2 , pi/2] */
  double ct = cos(theta);
  double st = sin(theta);
  double invct = 1.0/ct;
  double invst = 1.0/st;
  double r0; /* distance from first hatch line to the top-left (if angle in  0,pi/2)
                  or bottom-left (if angle in -pi/2,0) corner of the hatch bbox */
  double rmax = sqrt((double)(sx*sx+sy*sy)); /* distance to the furthest hatch we will have to create
TODO: this could be optimized for bounding boxes where width is very different than height for
certain hatch angles */
  double rref= rx*ct + ry*st; /* distance to the line passing through the refpoint, origin is
                                   (0,0) of the imageObj */
  double rcorner; /* distance to the line passing through the topleft or bottomleft corner
                       of the hatch bbox (origin is (0,0) of imageObj) */

  /* calculate the distance from the refpoint to the top right of the path */
  if(angle < 90) {
    rcorner = ox*ct + oy*st;
    r0 = step - fmod(rcorner-rref,step);
    if(rcorner-rref<0) r0 -= step;
  } else {
    rcorner = ox*ct + (oy+sy)*st;
    r0 = step - fmod(rcorner-rref,step);
    if(rcorner-rref<0) r0 -= step;
    st = -st;
    invst = -invst;
  }


  //parametrize each line as r = x.cos(theta) + y.sin(theta)
  for(double r=r0; r<rmax; r+=step) {
    int inter=0;
    double x,y;
    double pt[4]; //array to store the coordinates of intersection of the line with the sides
    //in the general case there will only be two intersections
    //so pt[4] should be sufficient to store the coordinates of the intersection,
    //but we allocate pt[8] to treat the special and rare/unfortunate case when the
    //line is a perfect diagonal (and therfore intersects all four sides)
    //note that the order for testing is important in this case so that the first
    //two intersection points actually correspond to the diagonal and not a degenerate line

    //test for intersection with each side

    y=r*invst;
    x=0; // test for intersection with left of image
    if(y>=0&&y<=sy) {
      pt[2*inter]=x;
      pt[2*inter+1]=y;
      inter++;
    }
    x=sx;
    y=(r-sx*ct)*invst;// test for intersection with right of image
    if(y>=0&&y<=sy) {
      pt[2*inter]=x;
      pt[2*inter+1]=y;
      inter++;
    }
    if(inter<2) {
      y=0;
      x=r*invct;// test for intersection with top of image
      if(x>=0&&x<=sx) {
        pt[2*inter]=x;
        pt[2*inter+1]=y;
        inter++;
      }
    }
    if(inter<2) {
      y=sy;
      x=(r-sy*st)*invct;// test for intersection with bottom of image
      if(x>=0&&x<=sx) {
        pt[2*inter]=x;
        pt[2*inter+1]=y;
        inter++;
      }
    }
    if(inter==2 && (pt[0]!=pt[2] || pt[1]!=pt[3])) {
      //the line intersects with two sides of the image, it should therefore be drawn
      if(angle<90) {
        path.move_to(pt[0],pt[1]);
        path.line_to(pt[2],pt[3]);
      } else {
        path.move_to(pt[0],sy-pt[1]);
        path.line_to(pt[2],sy-pt[3]);
      }
    }
  }
  return path;
}

template<class VertexSource> void renderPolygonHatches(imageObj *img,VertexSource &clipper, colorObj *color)
{
  if(img->format->renderer == MS_RENDER_WITH_AGG) {
    AGG2Renderer *r = AGG_RENDERER(img);
    r->m_rasterizer_aa_gamma.reset();
    r->m_rasterizer_aa_gamma.filling_rule(mapserver::fill_non_zero);
    r->m_rasterizer_aa_gamma.add_path(clipper);
    r->m_renderer_scanline.color(aggColor(color));
    mapserver::render_scanlines(r->m_rasterizer_aa_gamma, r->sl_poly, r->m_renderer_scanline);
  } else {
    shapeObj shape;
    msInitShape(&shape);
    int allocated = 20;
    lineObj line;
    shape.line = &line;
    shape.numlines = 1;
    shape.line[0].point = (pointObj*)msSmallCalloc(allocated,sizeof(pointObj));
    shape.line[0].numpoints = 0;
    double x=0,y=0;
    unsigned int cmd;
    clipper.rewind(0);
    while((cmd = clipper.vertex(&x,&y)) != mapserver::path_cmd_stop) {
      switch(cmd) {
        case mapserver::path_cmd_line_to:
          if(shape.line[0].numpoints == allocated) {
            allocated *= 2;
            shape.line[0].point = (pointObj*)msSmallRealloc(shape.line[0].point, allocated*sizeof(pointObj));
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
        case mapserver::path_cmd_end_poly|mapserver::path_flags_close:
          if(shape.line[0].numpoints > 2) {
            MS_IMAGE_RENDERER(img)->renderPolygon(img,&shape,color);
          }
          break;
        default:
          assert(0); //WTF?
      }
    }
    free(shape.line[0].point);
  }
}

int msHatchPolygon(imageObj *img, shapeObj *poly, double spacing, double width, double *pattern, int patternlength, double angle, colorObj *color)
{
  assert(MS_RENDERER_PLUGIN(img->format));
  msComputeBounds(poly);

  /* amount we should expand the bounding box by */
  double exp = width * 0.7072;

  /* width and height of the bounding box we will be creating the hatch in */
  int pw=(int)(poly->bounds.maxx-poly->bounds.minx+exp*2)+1;
  int ph=(int)(poly->bounds.maxy-poly->bounds.miny+exp*2)+1;

  /* position of the top-left corner of the bounding box */
  double ox = poly->bounds.minx - exp;
  double oy = poly->bounds.miny - exp;

  //create a rectangular hatch of size pw,ph starting at 0,0
  //the created hatch is of the size of the shape's bounding box
  mapserver::path_storage hatch = createHatch(ox,oy,
                                  img->refpt.x,img->refpt.y,pw,ph,angle,spacing);
  if(hatch.total_vertices()<=0) return MS_SUCCESS;

  //translate the hatch so it overlaps the current shape
  hatch.transform(mapserver::trans_affine_translation(ox,oy));

  polygon_adaptor polygons(poly);



  if(patternlength>1) {
    //dash the hatch and render it clipped by the shape
    mapserver::conv_dash<mapserver::path_storage > dash(hatch);
    mapserver::conv_stroke<mapserver::conv_dash<mapserver::path_storage> > stroke(dash);
    for (int i=0; i<patternlength; i+=2) {
      if (i < patternlength-1) {
        dash.add_dash(pattern[i], pattern[i+1]);
      }
    }
    stroke.width(width);
    stroke.line_cap(mapserver::butt_cap);
    mapserver::conv_clipper<polygon_adaptor,mapserver::conv_stroke<mapserver::conv_dash<mapserver::path_storage> > > clipper(polygons,stroke, mapserver::clipper_and);
    renderPolygonHatches(img,clipper,color);
  } else {
    //render the hatch clipped by the shape
    mapserver::conv_stroke <mapserver::path_storage > stroke(hatch);
    stroke.width(width);
    stroke.line_cap(mapserver::butt_cap);
    mapserver::conv_clipper<polygon_adaptor,mapserver::conv_stroke<mapserver::path_storage> > clipper(polygons,stroke, mapserver::clipper_and);
    renderPolygonHatches(img,clipper,color);
  }


  //assert(prevCmd == mapserver::path_cmd_line_to);
  //delete lines;
  return MS_SUCCESS;
}


int msPopulateRendererVTableAGG(rendererVTableObj * renderer)
{
  renderer->supports_transparent_layers = 0;
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

  renderer->renderGlyphs = &agg2RenderGlyphs;
  renderer->renderGlyphsLine = &agg2RenderGlyphsLine;
  renderer->renderBitmapGlyphs = &agg2RenderBitmapGlyphs;

  renderer->renderVectorSymbol = &agg2RenderVectorSymbol;

  renderer->renderPixmapSymbol = &agg2RenderPixmapSymbol;

  renderer->renderEllipseSymbol = &agg2RenderEllipseSymbol;

  renderer->renderTruetypeSymbol = &agg2RenderTruetypeSymbol;

  renderer->renderTile = &agg2RenderTile;

  renderer->getRasterBufferHandle = &aggGetRasterBufferHandle;
  renderer->getRasterBufferCopy = aggGetRasterBufferCopy;
  renderer->initializeRasterBuffer = aggInitializeRasterBuffer;

  renderer->mergeRasterBuffer = &agg2MergeRasterBuffer;
  renderer->loadImageFromFile = msLoadMSRasterBufferFromFile;
  renderer->createImage = &agg2CreateImage;
  renderer->saveImage = &agg2SaveImage;

  renderer->getTruetypeTextBBox = &agg2GetTruetypeTextBBox;

  renderer->startLayer = &agg2StartNewLayer;
  renderer->endLayer = &agg2CloseNewLayer;

  renderer->freeImage = &agg2FreeImage;
  renderer->freeSymbol = &agg2FreeSymbol;
  renderer->cleanup = agg2Cleanup;

  renderer->supports_bitmap_fonts = 1;
  for(int i=0; i<5; i++) {
    renderer->bitmapFontMetrics[i] = &(rasterfont_sizes[i]);
  }

  return MS_SUCCESS;
}

/******************************************************************************
 * $id$
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
#include "renderers/agg/include/agg_color_rgba.h"
#include "renderers/agg/include/agg_pixfmt_rgba.h"
#include "renderers/agg/include/agg_renderer_base.h"
#include "renderers/agg/include/agg_renderer_scanline.h"
#include "renderers/agg/include/agg_math_stroke.h"
#include "renderers/agg/include/agg_scanline_p.h"
#include "renderers/agg/include/agg_scanline_u.h"
#include "renderers/agg/include/agg_scanline_bin.h"
#include "renderers/agg/include/agg_rasterizer_scanline_aa.h"
#include "renderers/agg/include/agg_conv_stroke.h"
#include "renderers/agg/include/agg_conv_dash.h"
#include "renderers/agg/include/agg_path_storage.h"
#include "renderers/agg/include/agg_font_freetype.h"
#include "renderers/agg/include/agg_conv_contour.h"

#ifdef CPL_MSB
typedef mapserver::order_argb band_order;
#else
typedef mapserver::order_bgra band_order;
#endif

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

static color_type AGG_NO_COLOR = color_type(0, 0, 0, 0);

inline static color_type aggColor(colorObj *c) {
   if (c && MS_VALID_COLOR(*c)) {
      return mapserver::rgba8_pre(c->red, c->green, c->blue, c->alpha);
   } else {
      return color_type(0, 0, 0, 0);
   }
}

class AGG2Renderer {
public:

   AGG2Renderer() : m_fman(m_feng) {
   }

   band_type* buffer;
   rendering_buffer m_rendering_buffer;
   pixel_format m_pixel_format;
   renderer_base m_renderer_base;
   renderer_scanline m_renderer_scanline;
   rasterizer_scanline m_rasterizer_aa;
   mapserver::scanline_p8 sl_poly; /*packed scanlines, works faster when the area is larger
    than the perimeter, in number of pixels*/
   mapserver::scanline_u8 sl_line; /*unpacked scanlines, works faster if the area is roughly
    equal to the perimeter, in number of pixels*/
   mapserver::scanline_bin m_sl_bin;
   font_engine_type m_feng;
   font_manager_type m_fman;
};

inline AGG2Renderer *agg2GetRenderer(imageObj *image) {
   return (AGG2Renderer*) image->img.plugin;
}

template<class VertexSource>
static void applyCJC(VertexSource &stroke, int caps, int joins) {
   switch (joins) {
      case MS_CJC_NONE:
      case MS_CJC_ROUND:
         stroke.line_join(mapserver::round_join);
         break;
      case MS_CJC_MITER:
         stroke.line_join(mapserver::miter_join);
         break;
      case MS_CJC_BEVEL:
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

void agg2RenderLine(imageObj *img, shapeObj *p, strokeStyleObj *style) {

   AGG2Renderer *r = agg2GetRenderer(img);
   line_adaptor lines = line_adaptor(p);
   r->m_rasterizer_aa.reset();
   r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
   r->m_renderer_scanline.color(aggColor(&(style->color)));

   if (style->patternlength <= 0) {
      mapserver::conv_stroke<line_adaptor> stroke(lines);
      stroke.width(style->width);
      applyCJC(stroke, style->linecap, style->linejoin);
      r->m_rasterizer_aa.add_path(stroke);
   } else {
      mapserver::conv_dash<line_adaptor> dash(lines);
      mapserver::conv_stroke<mapserver::conv_dash<line_adaptor> > stroke_dash(dash);
      for (int i = 0; i < style->patternlength; i += 2) {

         if (i < style->patternlength - 1) {

            dash.add_dash(MS_MAX(1,MS_NINT(style->pattern[i])),
                    MS_MAX(1,MS_NINT(style->pattern[i + 1])));
         }
         stroke_dash.width(style->width);
         applyCJC(stroke_dash, style->linecap, style->linejoin);
         r->m_rasterizer_aa.add_path(stroke_dash);
      }
   }
   mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
}

void agg2RenderLineTiled(imageObj *img, shapeObj *p, imageObj * tile) {

}

void agg2RenderPolygon(imageObj *img, shapeObj *p, colorObj * color) {
   AGG2Renderer *r = agg2GetRenderer(img);
   polygon_adaptor polygons(p);
   r->m_rasterizer_aa.reset();
   r->m_rasterizer_aa.filling_rule(mapserver::fill_even_odd);
   r->m_rasterizer_aa.add_path(polygons);
   r->m_renderer_scanline.color(aggColor(color));
   mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_poly, r->m_renderer_scanline);
}

void agg2RenderPolygonTiled(imageObj *img, shapeObj *p, imageObj * tile) {

}

void agg2RenderGlyphs(imageObj *img, double x, double y,
        labelStyleObj *style, char *text) {
   AGG2Renderer *r = agg2GetRenderer(img);
   if (!r->m_feng.load_font(style->font, 0, mapserver::glyph_ren_outline)) {
      msSetError(MS_TTFERR, "AGG error loading font (%s)", "agg2RenderGlyphs()", style->font);
   }
   r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);

   const mapserver::glyph_cache* glyph;
   int unicode;
   r->m_feng.hinting(true);
   r->m_feng.height(style->size);
   r->m_feng.resolution(96);
   r->m_feng.flip_y(true);
   font_curve_type m_curves(r->m_fman.path_adaptor());
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
      glyph = r->m_fman.glyph(unicode);
      ;
      if (glyph) {
         r->m_fman.init_embedded_adaptors(glyph, fx, fy);
         mapserver::conv_transform<font_curve_type, mapserver::trans_affine> trans_c(m_curves, mtx);
         glyphs.concat_path(trans_c);
         fx += glyph->advance_x;
         fy += glyph->advance_y;
      }
   }

   //use a smoother renderer for the shadow
   /*if (shadowcolor.a) {
      mapserver::trans_affine_translation tr(shdx, shdy);
      mapserver::conv_transform<mapserver::path_storage, mapserver::trans_affine> tglyphs(glyphs, tr);
      mapserver::line_profile_aa prof;
      prof.width(0.5);
      renderer_oaa ren_oaa(ren_base, prof);
      rasterizer_outline_aa rasterizer_smooth(ren_oaa);
      ren_oaa.color(shadowcolor);
      rasterizer_smooth.add_path(tglyphs);
   }*/
   if (style->outlinewidth && style->outlinecolor.alpha) {
      color_type outlinecolor = aggColor(&(style->outlinecolor));
      r->m_rasterizer_aa.reset();
      r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
      mapserver::conv_contour<mapserver::path_storage> cc(glyphs);
      cc.width(style->outlinewidth);
      r->m_rasterizer_aa.add_path(cc);
      r->m_renderer_scanline.color(outlinecolor);
      mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
   }
   if (style->color.alpha) {
      color_type color = aggColor(&(style->color));
      r->m_rasterizer_aa.reset();
      r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
      r->m_rasterizer_aa.add_path(glyphs);
      r->m_renderer_scanline.color(color);
      mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
   }

}

void agg2RenderGlyphsLine(imageObj *img, labelPathObj *labelpath,
        labelStyleObj *style, char *text) {

}

void agg2RenderVectorSymbol(imageObj *img, double x, double y,
        symbolObj *symbol, symbolStyleObj * style) {

}

void* agg2CreateVectorSymbolTile(int width, int height,
        symbolObj *symbol, symbolStyleObj * style) {
    return NULL;
}

void agg2RenderPixmapSymbol(imageObj *img, double x, double y,
        symbolObj *symbol, symbolStyleObj * style) {

}

void* agg2CreatePixmapSymbolTile(int width, int height,
        symbolObj *symbol, symbolStyleObj * style) {
    return NULL;
}

void agg2RenderEllipseSymbol(imageObj *image, double x, double y,
        symbolObj *symbol, symbolStyleObj * style) {

}

void* agg2CreateEllipseSymbolTile(int width, int height,
        symbolObj *symbol, symbolStyleObj * style) {
    return NULL;
}

void agg2RenderTruetypeSymbol(imageObj *img, double x, double y,
        symbolObj *symbol, symbolStyleObj * style) {
   AGG2Renderer *r = agg2GetRenderer(img);
   if (!r->m_feng.load_font(symbol->full_font_path, 0, mapserver::glyph_ren_outline)) {
      msSetError(MS_TTFERR, "AGG error loading font (%s)", "agg2RenderTruetypeSymbol()", symbol->full_font_path);
   }

   const mapserver::glyph_cache* glyph;
   int unicode;
   r->m_feng.hinting(true);
   r->m_feng.height(style->scale);
   r->m_feng.resolution(96);
   r->m_feng.flip_y(true);
   font_curve_type m_curves(r->m_fman.path_adaptor());
   double ox = 0, oy = 0;
      //we're rendering a marker symbol, it has to be centered
      //on the label point (the default is the lower left of the
      //text)
      //this block computes the offset between the lower left corner
      //of the character and it's center for the given rotaion. this
      //offset will then be used in the following mapserver::trans_affine_translation
      msUTF8ToUniChar(symbol->character, &unicode);
      glyph = r->m_fman.glyph(unicode);
      ox = glyph->bounds.x1 + (glyph->bounds.x2 - glyph->bounds.x1) / 2.;
      oy = glyph->bounds.y1 + (glyph->bounds.y2 - glyph->bounds.y1) / 2.;
      mapserver::trans_affine_rotation(-style->rotation).transform(&ox, &oy);
   mapserver::trans_affine mtx;
   mtx *= mapserver::trans_affine_translation(-x, -y);
   /*agg angles are antitrigonometric*/
   mtx *= mapserver::trans_affine_rotation(-style->rotation);
   mtx *= mapserver::trans_affine_translation(x - ox, y - oy);
   mapserver::path_storage glyphs;

         r->m_fman.init_embedded_adaptors(glyph, x, y);
         mapserver::conv_transform<font_curve_type, mapserver::trans_affine> trans_c(m_curves, mtx);
         glyphs.concat_path(trans_c);
   if (style->outlinecolor.alpha) {
      color_type outlinecolor = aggColor(&(style->outlinecolor));
      r->m_rasterizer_aa.reset();
      r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
      mapserver::conv_contour<mapserver::path_storage> cc(glyphs);
      cc.width(style->outlinewidth);
      r->m_rasterizer_aa.add_path(cc);
      r->m_renderer_scanline.color(outlinecolor);
      mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
   }

   if (style->color.alpha) {
      r->m_rasterizer_aa.reset();
      r->m_rasterizer_aa.filling_rule(mapserver::fill_non_zero);
      r->m_rasterizer_aa.add_path(glyphs);
      color_type color = aggColor(&(style->color));
      r->m_renderer_scanline.color(color);
      mapserver::render_scanlines(r->m_rasterizer_aa, r->sl_line, r->m_renderer_scanline);
   }

}

void* agg2CreateTruetypeSymbolTile(int width, int height,
        symbolObj *symbol, symbolStyleObj * style) {
    return NULL;
}

void agg2RenderTile(imageObj *img, imageObj *tile, double x, double y) {

}

void agg2GetRasterBuffer(imageObj *img, rasterBufferObj * rb) {
   AGG2Renderer *r = agg2GetRenderer(img);
   rb->pixelbuffer = r->buffer;
   rb->row_step = r->m_rendering_buffer.stride();
   rb->pixel_step = 4;
   rb->width = r->m_rendering_buffer.width();
   rb->height = r->m_rendering_buffer.height();
   rb->r = &(r->buffer[band_order::R]);
   rb->g = &(r->buffer[band_order::G]);
   rb->b = &(r->buffer[band_order::B]);
   if (img->format->imagemode == MS_IMAGEMODE_RGBA) {
      rb->a = &(r->buffer[band_order::A]);
   } else {

      rb->a = NULL;
   }
}

void agg2MergeRasterBuffer(imageObj *dest, rasterBufferObj *overlay, double opacity, int dstX, int dstY) {

}

/* image i/o */
imageObj * agg2CreateImage(int width, int height, outputFormatObj *format, colorObj * bg) {
   imageObj *image = NULL;
   if (format->imagemode != MS_IMAGEMODE_RGB && format->imagemode != MS_IMAGEMODE_RGBA) {
      msSetError(MS_MISCERR,
              "AGG2 driver only supports RGB or RGBA pixel models.", "agg2CreateImage()");
      return image;
   }
   image = (imageObj *) calloc(1, sizeof (imageObj));
   AGG2Renderer *r = new AGG2Renderer();

   r->buffer = new band_type[width * height * 4];
   r->m_rendering_buffer.attach(r->buffer, width, height, width * 4);
   r->m_pixel_format.attach(r->m_rendering_buffer);
   r->m_renderer_base.attach(r->m_pixel_format);
   r->m_renderer_scanline.attach(r->m_renderer_base);
   if (format->imagemode == MS_IMAGEMODE_RGBA) {
      r->m_renderer_base.clear(AGG_NO_COLOR);
   } else {
      r->m_renderer_base.clear(aggColor(bg));
   }
   image->img.plugin = (void*) r;

   return image;
}

int agg2SaveImage(imageObj *img, FILE *fp, outputFormatObj * format) {
   msSetError(MS_MISCERR, "AGG2 does not support direct image saving", "agg2SaveImage()");

   return MS_FAILURE;
}
/*...*/

/* helper functions */
int agg2GetTruetypeTextBBox(imageObj *img, char *font, double size, char *string,
        rectObj *rect, double **advances) {
   AGG2Renderer *r = agg2GetRenderer(img);
   if (!r->m_feng.load_font(font, 0, mapserver::glyph_ren_outline)) {
      msSetError(MS_TTFERR, "AGG error loading font (%s)", "agg2GetTruetypeTextBBox()", font);
      return MS_FAILURE;
   }
   r->m_feng.hinting(true);
   r->m_feng.height(size);
   r->m_feng.resolution(96);
   r->m_feng.flip_y(true);
   int unicode, curGlyph = 1, numglyphs;
   if (advances) {
      numglyphs = msGetNumGlyphs(string);
   }
   const mapserver::glyph_cache* glyph;
   string += msUTF8ToUniChar(string, &unicode);
   glyph = r->m_fman.glyph(unicode);
   if (glyph) {
      rect->minx = glyph->bounds.x1;
      rect->maxx = glyph->bounds.x2;
      rect->miny = glyph->bounds.y1;
      rect->maxy = glyph->bounds.y2;
   } else
      return MS_FAILURE;
   if (advances) {
      *advances = (double*) malloc(numglyphs * sizeof (double));
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
      glyph = r->m_fman.glyph(unicode);
      if (glyph) {
         rect->minx = MS_MIN(rect->minx, fx+glyph->bounds.x1);
         rect->miny = MS_MIN(rect->miny, fy+glyph->bounds.y1);
         rect->maxx = MS_MAX(rect->maxx, fx+glyph->bounds.x2);
         rect->maxy = MS_MAX(rect->maxy, fy+glyph->bounds.y2);

         fx += glyph->advance_x;
         fy += glyph->advance_y;
         if (advances) {
            (*advances)[curGlyph++] = glyph->advance_x;
         }
      }
   }
   return MS_SUCCESS;
}

void agg2StartNewLayer(imageObj *img, layerObj *layer, double opacity) {
}

void agg2CloseNewLayer(imageObj *img, layerObj *layer, double opacity) {
}

void agg2TransformShape(shapeObj *shape, rectObj extend, double cellsize) {

}

void agg2FreeImage(imageObj * image) {

   AGG2Renderer *r = agg2GetRenderer(image);
   delete[] r->buffer;
   delete r;
   image->img.plugin = NULL;
}

void agg2FreeTile(imageObj * tile) {

}

void agg2FreeSymbol(symbolObj * symbol) {

}

int msPopulateRendererVTableAGG(rendererVTableObj * renderer) {
   renderer->supports_transparent_layers = 0;
   renderer->supports_pixel_buffer = 1;
   renderer->supports_imagecache = 0;

   renderer->renderLine = &agg2RenderLine;

   renderer->renderPolygon = &agg2RenderPolygon;
   renderer->renderPolygonTiled = &agg2RenderPolygonTiled;
   renderer->renderLineTiled = &agg2RenderLineTiled;

   renderer->renderGlyphs = &agg2RenderGlyphs;

   renderer->renderGlyphsLine = &agg2RenderGlyphsLine;

   renderer->renderVectorSymbol = &agg2RenderVectorSymbol;

   renderer->createVectorSymbolTile = &agg2CreateVectorSymbolTile;

   renderer->renderPixmapSymbol = &agg2RenderPixmapSymbol;

   renderer->createPixmapSymbolTile = &agg2CreatePixmapSymbolTile;

   renderer->renderEllipseSymbol = &agg2RenderEllipseSymbol;

   renderer->createEllipseSymbolTile = &agg2CreateEllipseSymbolTile;

   renderer->renderTruetypeSymbol = &agg2RenderTruetypeSymbol;

   renderer->createTruetypeSymbolTile = &agg2CreateTruetypeSymbolTile;

   renderer->renderTile = &agg2RenderTile;

   renderer->getRasterBuffer = &agg2GetRasterBuffer;

   renderer->mergeRasterBuffer = &agg2MergeRasterBuffer;
   renderer->createImage = &agg2CreateImage;
   renderer->saveImage = &agg2SaveImage;

   renderer->getTruetypeTextBBox = &agg2GetTruetypeTextBBox;

   renderer->startNewLayer = &agg2StartNewLayer;
   renderer->closeNewLayer = &agg2CloseNewLayer;

   renderer->transformShape = &msTransformShapeAGG;
   renderer->freeImage = &agg2FreeImage;
   renderer->freeTile = &agg2FreeTile;
   renderer->freeSymbol = &agg2FreeSymbol;
   return MS_SUCCESS;
}

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Cairo Rendering functions
 * Author:   Thomas Bonfort
 *
 ******************************************************************************
 * Copyright (c) 1996-2009 Regents of the University of Minnesota.
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

#ifdef USE_CAIRO

#include <cairo.h>
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <cairo-pdf.h>
#include <cairo-svg.h>
#else
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>
#endif

#ifdef USE_SVG_CAIRO
#include <svg-cairo.h>
#else
#ifdef USE_RSVG
  #include <librsvg/rsvg.h>
  #ifndef LIBRSVG_CHECK_VERSION
    #include <librsvg/librsvg-features.h>
  #endif
  #ifndef RSVG_CAIRO_H
    #include <librsvg/rsvg-cairo.h>
  #endif
  #include <glib-object.h>
#endif
#endif

#ifdef USE_GDAL
#include <cpl_string.h>
#include <gdal.h>
#endif

# include <cairo-ft.h>
/*
#include <pango/pangocairo.h>
#include <glib.h>
*/
#include <string.h>
/*
#include <ft2build.h>
#include FT_FREETYPE_H
*/

typedef struct facecacheObj faceCacheObj;
struct facecacheObj {
  cairo_font_face_t *face;
  FT_Face ftface;
  char *path;
  faceCacheObj *next;
  cairo_user_data_key_t facekey;
};

int freeFaceCache(faceCacheObj *fc)
{
  /* printf("***\nface %s has %d cairo references\n***\n",fc->path,cairo_font_face_get_reference_count(fc->face)); */
  cairo_font_face_destroy(fc->face);
  FT_Done_Face(fc->ftface);
  free(fc->path);
  return MS_SUCCESS;
}

typedef struct {
  faceCacheObj *facecache;
  FT_Library library;
  /* dummy surface and context */
  unsigned char dummydata[4];
  cairo_surface_t *dummysurface;
  cairo_t *dummycr;
} cairoCacheData;

void initializeCache(void **vcache)
{
  cairoCacheData *cache = (cairoCacheData*)malloc(sizeof(cairoCacheData));
  *vcache = cache;

  cache->facecache = NULL;
  FT_Init_FreeType(&(cache->library));
  /* dummy surface and context */
  cache->dummysurface = cairo_image_surface_create_for_data(cache->dummydata, CAIRO_FORMAT_ARGB32, 1,1,4);
  cache->dummycr = cairo_create(cache->dummysurface);
}

int cleanupCairo(void *cache)
{
  cairoCacheData *ccache = (cairoCacheData*)cache;

  if(ccache->dummycr) {
    cairo_destroy(ccache->dummycr);
  }
  if(ccache->dummysurface) {
    cairo_surface_destroy(ccache->dummysurface);
  }
  if(ccache->facecache) {
    faceCacheObj *next,*cur;
    cur = ccache->facecache;
    do {
      next = cur->next;
      freeFaceCache(cur);
      free(cur);
      cur=next;
    } while(cur);
  }
  if(ccache->library)
    FT_Done_FreeType(ccache->library);
  free(ccache);
  return MS_SUCCESS;
}





typedef struct {
  cairo_surface_t *surface;
  cairo_t *cr;
  bufferObj *outputStream;
  int use_alpha;
} cairo_renderer;


#define CAIRO_RENDERER(im) ((cairo_renderer*)(im->img.plugin))


int freeImageCairo(imageObj *img)
{
  cairo_renderer *r = CAIRO_RENDERER(img);
  if(r) {
    cairo_destroy(r->cr);
    cairo_surface_finish(r->surface);
    cairo_surface_destroy(r->surface);
    if(r->outputStream) {
      msBufferFree(r->outputStream);
      free(r->outputStream);
    }
    free(r);
  }
  return MS_SUCCESS;
}


faceCacheObj *getFontFace(cairoCacheData *cache, const char *font)
{
  faceCacheObj *newface = NULL;
  faceCacheObj *cur=cache->facecache;
  while(cur) {
    if(!strcmp(cur->path,font))
      return cur;
    cur = cur->next;
  }
  newface = malloc(sizeof(faceCacheObj));

  if(FT_New_Face(cache->library, font, 0, &(newface->ftface))) {
    msSetError(MS_RENDERERERR,"Freetype failed to open font %s","getFontFace()",font);
    free(newface);
    return NULL;
  }

  /* Try to select charmap */
  if (!newface->ftface->charmap) {
    if( FT_Select_Charmap(newface->ftface, FT_ENCODING_MS_SYMBOL) )
       FT_Select_Charmap(newface->ftface, FT_ENCODING_APPLE_ROMAN );
  }

  newface->next = cache->facecache;
  cache->facecache = newface;
  newface->face = cairo_ft_font_face_create_for_ft_face(newface->ftface, 0);

  cairo_font_face_set_user_data (newface->face, &newface->facekey,
                                 &(newface->ftface), (cairo_destroy_func_t) NULL); // we call FT_Done_Face ourselves in freeFaceCache

  newface->path = msStrdup(font);
  return newface;
}

#define msCairoSetSourceColor(cr, c) cairo_set_source_rgba((cr),(c)->red/255.0,(c)->green/255.0,(c)->blue/255.0,(c)->alpha/255.0);

int renderLineCairo(imageObj *img, shapeObj *p, strokeStyleObj *stroke)
{
  int i,j;
  cairo_renderer *r = CAIRO_RENDERER(img);
  assert(stroke->color);
  cairo_new_path(r->cr);
  msCairoSetSourceColor(r->cr,stroke->color);
  for(i=0; i<p->numlines; i++) {
    lineObj *l = &(p->line[i]);
    if(l->numpoints == 0) continue;
    cairo_move_to(r->cr,l->point[0].x,l->point[0].y);
    for(j=1; j<l->numpoints; j++) {
      cairo_line_to(r->cr,l->point[j].x,l->point[j].y);
    }
  }
  if(stroke->patternlength>0) {
    cairo_set_dash(r->cr,stroke->pattern,stroke->patternlength,-stroke->patternoffset);
  }
  switch(stroke->linecap) {
    case MS_CJC_BUTT:
      cairo_set_line_cap(r->cr,CAIRO_LINE_CAP_BUTT);
      break;
    case MS_CJC_SQUARE:
      cairo_set_line_cap(r->cr,CAIRO_LINE_CAP_SQUARE);
      break;
    case MS_CJC_ROUND:
    case MS_CJC_NONE:
    default:
      cairo_set_line_cap(r->cr,CAIRO_LINE_CAP_ROUND);
  }
  cairo_set_line_width (r->cr, stroke->width);
  cairo_stroke (r->cr);
  if(stroke->patternlength>0) {
    cairo_set_dash(r->cr,stroke->pattern,0,0);
  }
  return MS_SUCCESS;
}

int renderPolygonCairo(imageObj *img, shapeObj *p, colorObj *c)
{
  cairo_renderer *r = CAIRO_RENDERER(img);
  int i,j;
  cairo_new_path(r->cr);
  cairo_set_fill_rule(r->cr,CAIRO_FILL_RULE_EVEN_ODD);
  msCairoSetSourceColor(r->cr,c);
  for(i=0; i<p->numlines; i++) {
    lineObj *l = &(p->line[i]);
    cairo_move_to(r->cr,l->point[0].x,l->point[0].y);
    for(j=1; j<l->numpoints; j++) {
      cairo_line_to(r->cr,l->point[j].x,l->point[j].y);
    }
    cairo_close_path(r->cr);
  }
  cairo_fill(r->cr);
  return MS_SUCCESS;
}

int renderPolygonTiledCairo(imageObj *img, shapeObj *p,  imageObj *tile)
{
  int i,j;
  cairo_renderer *r = CAIRO_RENDERER(img);
  cairo_renderer *tileRenderer = CAIRO_RENDERER(tile);
  cairo_pattern_t *pattern = cairo_pattern_create_for_surface(tileRenderer->surface);
  cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

  cairo_set_source(r->cr, pattern);
  for (i = 0; i < p->numlines; i++) {
    lineObj *l = &(p->line[i]);
    cairo_move_to(r->cr, l->point[0].x, l->point[0].y);
    for (j = 1; j < l->numpoints; j++) {
      cairo_line_to(r->cr, l->point[j].x, l->point[j].y);
    }
    /* cairo_close_path(r->cr); */
  }
  cairo_fill(r->cr);
  cairo_pattern_destroy(pattern);
  return MS_SUCCESS;
}

cairo_surface_t *createSurfaceFromBuffer(rasterBufferObj *b)
{
  assert(b->type == MS_BUFFER_BYTE_RGBA);
  return cairo_image_surface_create_for_data (b->data.rgba.pixels,
         CAIRO_FORMAT_ARGB32,
         b->width,
         b->height,
         b->data.rgba.row_step);
}


int renderPixmapSymbolCairo(imageObj *img, double x, double y,symbolObj *symbol,
                            symbolStyleObj *style)
{
  cairo_renderer *r = CAIRO_RENDERER(img);
  cairo_surface_t *im;
  rasterBufferObj *b = symbol->pixmap_buffer;
  assert(b);
  if(!symbol->renderer_cache) {
    symbol->renderer_cache = (void*)createSurfaceFromBuffer(b);
  }
  assert(symbol->renderer_cache);
  im=(cairo_surface_t*)symbol->renderer_cache;
  cairo_save(r->cr);
  if(style->rotation != 0 || style->scale != 1) {
    cairo_translate (r->cr, x,y);
    cairo_rotate (r->cr, -style->rotation);
    cairo_scale  (r->cr, style->scale,style->scale);
    cairo_translate (r->cr, -0.5*b->width, -0.5*b->height);
  } else {
    cairo_translate (r->cr, MS_NINT(x-0.5*b->width),MS_NINT(y-0.5*b->height));
  }
  cairo_set_source_surface (r->cr, im, 0, 0);
  cairo_paint (r->cr);
  cairo_restore(r->cr);
  return MS_SUCCESS;
}

int renderVectorSymbolCairo(imageObj *img, double x, double y, symbolObj *symbol,
                            symbolStyleObj *style)
{
  cairo_renderer *r = CAIRO_RENDERER(img);
  double ox=symbol->sizex*0.5,oy=symbol->sizey*0.5;
  int is_new = 1,i;
  cairo_new_path(r->cr);
  cairo_save(r->cr);
  cairo_translate(r->cr,x,y);
  cairo_scale(r->cr,style->scale,style->scale);
  cairo_rotate(r->cr,-style->rotation);
  cairo_translate(r->cr,-ox,-oy);
  for (i = 0; i < symbol->numpoints; i++) {
    if ((symbol->points[i].x == -99) && (symbol->points[i].y == -99)) { /* (PENUP) */
      is_new = 1;
    } else {
      if (is_new) {
        cairo_move_to(r->cr,symbol->points[i].x,symbol->points[i].y);
        is_new = 0;
      } else {
        cairo_line_to(r->cr,symbol->points[i].x,symbol->points[i].y);
      }
    }
  }
  cairo_restore(r->cr);
  if(style->color) {
    msCairoSetSourceColor(r->cr,style->color);
    cairo_fill_preserve(r->cr);
  }
  if(style->outlinewidth>0) {
    msCairoSetSourceColor(r->cr,style->outlinecolor);
    cairo_set_line_width (r->cr, style->outlinewidth);
    cairo_stroke_preserve(r->cr);
  }
  cairo_new_path(r->cr);
  return MS_SUCCESS;
}

#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
struct svg_symbol_cache {
  rasterBufferObj *pixmap_buffer;
#ifdef USE_RSVG
  RsvgHandle *svgc;
#else
  svg_cairo_t *svgc;
#endif
  double scale,rotation;
} ;
#endif

int renderSVGSymbolCairo(imageObj *img, double x, double y, symbolObj *symbol,
                         symbolStyleObj *style)
{
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
  struct svg_symbol_cache *cache;
  cairo_renderer *r = CAIRO_RENDERER(img);



  msPreloadSVGSymbol(symbol);
  assert(symbol->renderer_cache);
  cache = symbol->renderer_cache;

  cairo_save(r->cr);
  cairo_translate(r->cr,x,y);
  cairo_scale(r->cr,style->scale,style->scale);

  if (style->rotation != 0) {
    cairo_rotate(r->cr, -style->rotation);
    cairo_translate (r->cr, -(int)(symbol->sizex/2), -(int)(symbol->sizey/2));
  } else
    cairo_translate (r->cr, -(int)(symbol->sizex/2), -(int)(symbol->sizey/2));

#ifdef USE_SVG_CAIRO
  {
    svg_cairo_status_t status;
    status = svg_cairo_render(cache->svgc, r->cr);
    if(status != SVG_CAIRO_STATUS_SUCCESS) {
      cairo_restore(r->cr);
      return MS_FAILURE;
    }
  }
#else
  rsvg_handle_render_cairo(cache->svgc, r->cr);
#endif

  cairo_restore(r->cr);

  return MS_SUCCESS;


#else
  msSetError(MS_MISCERR, "SVG Symbols requested but is not built with libsvgcairo",
             "renderSVGSymbolCairo()");
  return MS_FAILURE;
#endif
}

int renderTruetypeSymbolCairo(imageObj *img, double x, double y, symbolObj *symbol,
                              symbolStyleObj *s)
{
  int unicode;
  cairo_glyph_t glyph;
  cairo_text_extents_t extents;

  cairo_matrix_t trans;
  double ox,oy;
  cairoCacheData *cache = MS_IMAGE_RENDERER_CACHE(img);
  cairo_renderer *r = CAIRO_RENDERER(img);
  faceCacheObj *face = getFontFace(cache,symbol->full_font_path);

  if(!face) return MS_FAILURE;

  cairo_save(r->cr);
  cairo_set_font_face(r->cr,face->face);
  cairo_set_font_size(r->cr,s->scale*96/72.0);


  msUTF8ToUniChar(symbol->character, &unicode);

  if (face->ftface->charmap &&
    face->ftface->charmap->encoding == FT_ENCODING_MS_SYMBOL)
    unicode |= 0xf000;

  glyph.index = FT_Get_Char_Index(face->ftface, unicode);
  glyph.x=0;
  glyph.y=0;
  cairo_glyph_extents(r->cr,&glyph,1,&extents);
  ox=extents.x_bearing+extents.width/2.;
  oy=extents.y_bearing+extents.height/2.;



  cairo_matrix_init_rotate(&trans,-s->rotation);

  cairo_matrix_transform_point(&trans,&ox,&oy);
  /* cairo_translate(cr,-extents.width/2,-extents.height/2); */

  cairo_translate(r->cr,x-ox,y-oy);
  cairo_rotate(r->cr, -s->rotation);

  cairo_glyph_path(r->cr,&glyph,1);

  if (s->outlinewidth) {
    msCairoSetSourceColor(r->cr, s->outlinecolor);
    cairo_set_line_width(r->cr, s->outlinewidth + 1);
    cairo_stroke_preserve(r->cr);
  }
  if(s->color) {
    msCairoSetSourceColor(r->cr, s->color);
    cairo_fill_preserve(r->cr);
  }
  cairo_new_path(r->cr);
  cairo_restore(r->cr);
  return MS_SUCCESS;
}

int renderTileCairo(imageObj *img, imageObj *tile, double x, double y)
{
  cairo_renderer *r = CAIRO_RENDERER(img);
  cairo_surface_t *im = CAIRO_RENDERER(tile)->surface;
  int w = cairo_image_surface_get_width (im);
  int h = cairo_image_surface_get_height (im);
  cairo_save(r->cr);
  cairo_translate(r->cr, MS_NINT(x-0.5 * w), MS_NINT(y -0.5 * h));
  cairo_set_source_surface(r->cr, im, 0, 0);
  cairo_pattern_set_filter (cairo_get_source (r->cr), CAIRO_FILTER_NEAREST);
  cairo_paint(r->cr);
  cairo_restore(r->cr);
  return MS_SUCCESS;
}

#define CAIROLINESPACE 1.33

int getTruetypeTextBBoxCairo(rendererVTableObj *renderer, char **fonts, int numfonts, double size,
                             char *text, rectObj *rect, double **advances, int bAdjustBaseline)
{
  cairoCacheData *cache = MS_RENDERER_CACHE(renderer);
  faceCacheObj* face = getFontFace(cache,fonts[0]);

  int curfontidx = 0;
  char *utfptr=text;
  int i,unicode;
  unsigned long previdx=0;
  faceCacheObj* prevface = face;
  int numglyphs = msGetNumGlyphs(text);
  cairo_glyph_t glyph;
  cairo_text_extents_t extents;
  double px=0,py=0;

  if(face == NULL) {
    return MS_FAILURE;
  }

  cairo_set_font_face(cache->dummycr,face->face);
  cairo_set_font_size(cache->dummycr,size*96/72.0);

  if(advances != NULL) {
    *advances = (double*)malloc(numglyphs*sizeof(double));
  }

  for(i=0; i<numglyphs; i++) {
    utfptr+=msUTF8ToUniChar(utfptr, &unicode);
    glyph.x=px;
    glyph.y=py;
    if(unicode=='\n') {
      py += ceil(size*CAIROLINESPACE);
      px = 0;
      previdx=0;
      continue;
    }

    if (curfontidx != 0) {
      face = getFontFace(cache, fonts[0]);
      cairo_set_font_face(cache->dummycr, face->face);
      curfontidx = 0;
    }

    if (face->ftface->charmap &&
      face->ftface->charmap->encoding == FT_ENCODING_MS_SYMBOL)
      unicode |= 0xf000;

    glyph.index = FT_Get_Char_Index(face->ftface, unicode);

    if (glyph.index == 0) {
      int j;
      for (j = 1; j < numfonts; j++) {
        curfontidx = j;
        face = getFontFace(cache, fonts[j]);
        glyph.index = FT_Get_Char_Index(face->ftface, unicode);
        if (glyph.index != 0) {
          cairo_set_font_face(cache->dummycr, face->face);
          break;
        }
      }
    }

    if( FT_HAS_KERNING((prevface->ftface)) && previdx ) {
      FT_Vector delta;
      FT_Get_Kerning( prevface->ftface, previdx, glyph.index, FT_KERNING_DEFAULT, &delta );
      px += delta.x / 64.;
    }
    cairo_glyph_extents(cache->dummycr,&glyph,1,&extents);

    if(i==0) {
      rect->minx = px+extents.x_bearing;
      rect->miny = py+extents.y_bearing;
      rect->maxx = px+extents.x_bearing+extents.width;
      rect->maxy = py+(bAdjustBaseline?1:(extents.y_bearing+extents.height));
    } else {
      rect->minx = MS_MIN(rect->minx,px+extents.x_bearing);
      rect->miny = MS_MIN(rect->miny,py+extents.y_bearing);
      rect->maxy = MS_MAX(rect->maxy,py+(bAdjustBaseline?1:(extents.y_bearing+extents.height)));
      rect->maxx = MS_MAX(rect->maxx,px+extents.x_bearing+extents.width);
    }
    if(advances!=NULL)
      (*advances)[i]=extents.x_advance;
    px += extents.x_advance;
    previdx=glyph.index;
    prevface = face;
  }
  return MS_SUCCESS;
}

int renderGlyphsCairo(imageObj *img,double x, double y, labelStyleObj *style, char *text)
{
  cairo_renderer *r = CAIRO_RENDERER(img);
  cairoCacheData *cache = MS_IMAGE_RENDERER_CACHE(img);
  faceCacheObj* face = getFontFace(cache,style->fonts[0]);

  int curfontidx = 0;
  char *utfptr=text;
  int i,unicode;
  unsigned long previdx=0;
  faceCacheObj* prevface = face;
  int numglyphs = msGetNumGlyphs(text);
  cairo_glyph_t glyph;
  cairo_text_extents_t extents;
  double px=0,py=0;

  if(face == NULL) {
    return MS_FAILURE;
  }

  cairo_set_font_face(r->cr,face->face);
  cairo_set_font_size(r->cr,style->size*96/72.0);

  cairo_save(r->cr);
  cairo_translate(r->cr,MS_NINT(x),MS_NINT(y));
  if(style->rotation != 0.0)
    cairo_rotate(r->cr, -style->rotation);

  for(i=0; i<numglyphs; i++) {
    utfptr+=msUTF8ToUniChar(utfptr, &unicode);
    glyph.x=px;
    glyph.y=py;
    if(unicode=='\n') {
      py += ceil(style->size*CAIROLINESPACE);
      px = 0;
      previdx=0;
      continue;
    }
    if(curfontidx != 0) {
      face = getFontFace(cache,style->fonts[0]);
      cairo_set_font_face(r->cr,face->face);
      curfontidx = 0;
    }

    if (face->ftface->charmap &&
      face->ftface->charmap->encoding == FT_ENCODING_MS_SYMBOL)
      unicode |= 0xf000;

    glyph.index = FT_Get_Char_Index(face->ftface, unicode);
    if(glyph.index == 0) {
      int j;
      for(j=1; j<style->numfonts; j++) {
        curfontidx = j;
        face = getFontFace(cache,style->fonts[j]);
        glyph.index = FT_Get_Char_Index(face->ftface, unicode);
        if(glyph.index != 0) {
          cairo_set_font_face(r->cr,face->face);
          break;
        }
      }
    }

    if(  FT_HAS_KERNING((prevface->ftface)) && previdx ) {
      FT_Vector delta;
      FT_Get_Kerning( prevface->ftface, previdx, glyph.index, FT_KERNING_DEFAULT, &delta );
      px += delta.x / 64.;
    }
    cairo_glyph_extents(r->cr,&glyph,1,&extents);
    cairo_glyph_path(r->cr,&glyph,1);
    px += extents.x_advance;
    previdx=glyph.index;
    prevface=face;
  }

  if (style->outlinewidth > 0) {
    cairo_save(r->cr);
    msCairoSetSourceColor(r->cr, style->outlinecolor);
    cairo_set_line_width(r->cr, style->outlinewidth + 1);
    cairo_stroke_preserve(r->cr);
    cairo_restore(r->cr);
  }
  if(style->color) {
    msCairoSetSourceColor(r->cr, style->color);
    cairo_fill(r->cr);
  }
  cairo_new_path(r->cr);
  cairo_restore(r->cr);
  return MS_SUCCESS;
}


cairo_status_t _stream_write_fn(void *b, const unsigned char *data, unsigned int length)
{
  msBufferAppend((bufferObj*)b,(void*)data,length);
  return CAIRO_STATUS_SUCCESS;
}

imageObj* createImageCairo(int width, int height, outputFormatObj *format,colorObj* bg)
{
  imageObj *image = NULL;
  cairo_renderer *r=NULL;
  if (format->imagemode != MS_IMAGEMODE_RGB && format->imagemode!= MS_IMAGEMODE_RGBA) {
    msSetError(MS_MISCERR,
               "Cairo driver only supports RGB or RGBA pixel models.","msImageCreateCairo()");
    return image;
  }
  if (width > 0 && height > 0) {
    image = (imageObj *) calloc(1, sizeof(imageObj));
    r = (cairo_renderer*)calloc(1,sizeof(cairo_renderer));
    if(!strcasecmp(format->driver,"cairo/pdf")) {
      r->outputStream = (bufferObj*)malloc(sizeof(bufferObj));
      msBufferInit(r->outputStream);
      r->surface = cairo_pdf_surface_create_for_stream(
                     _stream_write_fn,
                     r->outputStream,
                     width,height);
    } else if(!strcasecmp(format->driver,"cairo/svg")) {
      r->outputStream = (bufferObj*)malloc(sizeof(bufferObj));
      msBufferInit(r->outputStream);
      r->surface = cairo_svg_surface_create_for_stream(
                     _stream_write_fn,
                     r->outputStream,
                     width,height);
    } else if(!strcasecmp(format->driver,"cairo/winGDI") && format->device) {
#if CAIRO_HAS_WIN32_SURFACE
      r->outputStream = NULL;
      r->surface = cairo_win32_surface_create(format->device);
#else
      msSetError(MS_RENDERERERR, "Cannot create cairo image. Cairo was not compiled with support for the win32 backend.",
                 "msImageCreateCairo()");
#endif
    } else if(!strcasecmp(format->driver,"cairo/winGDIPrint") && format->device) {
#if CAIRO_HAS_WIN32_SURFACE
      r->outputStream = NULL;
      r->surface = cairo_win32_printing_surface_create(format->device);
#else
      msSetError(MS_RENDERERERR, "Cannot create cairo image. Cairo was not compiled with support for the win32 backend.",
                 "msImageCreateCairo()");
#endif
    } else {
      r->outputStream = NULL;
      r->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    }
    r->cr = cairo_create(r->surface);
    if(format->transparent || !bg || !MS_VALID_COLOR(*bg)) {
      r->use_alpha = 1;
      cairo_set_source_rgba (r->cr, 0,0,0,0);
    } else {
      r->use_alpha = 0;
      msCairoSetSourceColor(r->cr,bg);
    }
    cairo_save (r->cr);
    cairo_set_operator (r->cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (r->cr);
    cairo_restore (r->cr);

    cairo_set_line_cap (r->cr,CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(r->cr,CAIRO_LINE_JOIN_ROUND);
    image->img.plugin = (void*)r;
  } else {
    msSetError(MS_RENDERERERR, "Cannot create cairo image of size %dx%d.",
               "msImageCreateCairo()", width, height);
  }
  return image;
}

/* msSaveImagePostPDFProcessing() will call the GDAL PDF driver to add geospatial */
/* information to the regular PDF generated by cairo. This is only triggered if the */
/* GEO_ENCODING outputformat option is set (to ISO32000 or OGC_BP). Additionnal */
/* options can be provided by specifying outputformat options starting with */
/* METADATA_ITEM: prefix. For example METADATA_ITEM:PRODUCER=MapServer */
/* Those options are AUTHOR, CREATOR, CREATION_DATE, KEYWORDS, PRODUCER, SUBJECT, TITLE */
/* See http://gdal.org/frmt_pdf.html documentation. */

static void msTransformToGeospatialPDF(imageObj *img, mapObj *map, cairo_renderer *r)
{
  /* We need a GDAL 1.10 PDF driver at runtime, but as far as the C API is concerned, GDAL 1.9 is */
  /* largely sufficient. */
#if defined(USE_GDAL) && defined(GDAL_VERSION_NUM) && GDAL_VERSION_NUM >= 1900
  GDALDatasetH hDS = NULL;
  const char* pszGEO_ENCODING = NULL;
  GDALDriverH hPDFDriver = NULL;
  const char* pszVirtualIO = NULL;
  int bVirtualIO = FALSE;
  char* pszTmpFilename = NULL;
  VSILFILE* fp = NULL;

  if (map == NULL)
    return;

  pszGEO_ENCODING = msGetOutputFormatOption(img->format, "GEO_ENCODING", NULL);
  if (pszGEO_ENCODING == NULL)
    return;

  msGDALInitialize();

  hPDFDriver = GDALGetDriverByName("PDF");
  if (hPDFDriver == NULL)
    return;

  /* When compiled against libpoppler, the PDF driver is VirtualIO capable */
  /* but not, when it is compiled against libpodofo. */
  pszVirtualIO = GDALGetMetadataItem( hPDFDriver, GDAL_DCAP_VIRTUALIO, NULL );
  if (pszVirtualIO)
    bVirtualIO = CSLTestBoolean(pszVirtualIO);

  if (bVirtualIO)
    pszTmpFilename = msTmpFile(map, NULL, "/vsimem/mscairopdf/", "pdf");
  else
    pszTmpFilename = msTmpFile(map, map->mappath, NULL, "pdf");

  /* Copy content of outputStream buffer into file */
  fp = VSIFOpenL(pszTmpFilename, "wb");
  if (fp == NULL) {
    msFree(pszTmpFilename);
    return;
  }
  VSIFWriteL(r->outputStream->data, 1, r->outputStream->size, fp);
  VSIFCloseL(fp);
  fp = NULL;

  hDS = GDALOpen(pszTmpFilename, GA_Update);
  if ( hDS != NULL ) {
    char* pszWKT = msProjectionObj2OGCWKT( &(map->projection) );
    if( pszWKT != NULL ) {
      double adfGeoTransform[6];
      int i;

      /* Add user-specified options */
      for( i = 0; i < img->format->numformatoptions; i++ ) {
        const char* pszOption = img->format->formatoptions[i];
        if( strncasecmp(pszOption,"METADATA_ITEM:",14) == 0 ) {
          char* pszKey = NULL;
          const char* pszValue = CPLParseNameValue(pszOption + 14,
                                 &pszKey);
          if( pszKey != NULL ) {
            GDALSetMetadataItem(hDS, pszKey, pszValue, NULL);
            CPLFree(pszKey);
          }
        }
      }

      /* We need to rescale the geotransform because GDAL will not necessary */
      /* open the PDF with the DPI that was used to generate it */
      memcpy(adfGeoTransform, map->gt.geotransform, 6 * sizeof(double));
      adfGeoTransform[1] = adfGeoTransform[1] * map->width / GDALGetRasterXSize(hDS);
      adfGeoTransform[5] = adfGeoTransform[5] * map->height / GDALGetRasterYSize(hDS);
      GDALSetGeoTransform(hDS, adfGeoTransform);
      GDALSetProjection(hDS, pszWKT);

      msFree( pszWKT );
      pszWKT = NULL;

      CPLSetThreadLocalConfigOption("GDAL_PDF_GEO_ENCODING", pszGEO_ENCODING);

      GDALClose(hDS);
      hDS = NULL;

      CPLSetThreadLocalConfigOption("GDAL_PDF_GEO_ENCODING", NULL);

      /* We need to replace the buffer with the content of the GDAL file */
      fp = VSIFOpenL(pszTmpFilename, "rb");
      if( fp != NULL ) {
        int nFileSize;

        VSIFSeekL(fp, 0, SEEK_END);
        nFileSize = (int)VSIFTellL(fp);

        msBufferResize(r->outputStream, nFileSize);

        VSIFSeekL(fp, 0, SEEK_SET);
        r->outputStream->size = VSIFReadL(r->outputStream->data, 1, nFileSize, fp);

        VSIFCloseL(fp);
        fp = NULL;
      }
    }
  }

  if ( hDS != NULL )
    GDALClose(hDS);

  VSIUnlink(pszTmpFilename);

  msFree(pszTmpFilename);
#endif
}

int saveImageCairo(imageObj *img, mapObj *map, FILE *fp, outputFormatObj *format)
{
  cairo_renderer *r = CAIRO_RENDERER(img);
  if(!strcasecmp(img->format->driver,"cairo/pdf") || !strcasecmp(img->format->driver,"cairo/svg")) {
    cairo_surface_finish (r->surface);

    if (map != NULL && !strcasecmp(img->format->driver,"cairo/pdf"))
      msTransformToGeospatialPDF(img, map, r);

    msIO_fwrite(r->outputStream->data,r->outputStream->size,1,fp);
  } else {
    /* not supported */
  }
  return MS_SUCCESS;
}

unsigned char* saveImageBufferCairo(imageObj *img, int *size_ptr, outputFormatObj *format)
{
  cairo_renderer *r = CAIRO_RENDERER(img);
  unsigned char *data;
  assert(!strcasecmp(img->format->driver,"cairo/pdf") || !strcasecmp(img->format->driver,"cairo/svg"));
  cairo_surface_finish (r->surface);
  data = msSmallMalloc(r->outputStream->size);
  memcpy(data,r->outputStream->data,r->outputStream->size);
  *size_ptr = (int)r->outputStream->size;
  return data;
}

int renderEllipseSymbolCairo(imageObj *img, double x, double y, symbolObj *symbol,
                             symbolStyleObj *style)
{
  cairo_renderer *r = CAIRO_RENDERER(img);

  cairo_save(r->cr);
  cairo_set_line_cap(r->cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join(r->cr, CAIRO_LINE_JOIN_MITER);
  cairo_translate(r->cr,x,y);
  cairo_rotate(r->cr,-style->rotation);
  cairo_scale(r->cr,symbol->sizex*style->scale/2,symbol->sizey*style->scale/2);
  cairo_arc (r->cr, 0,0,1, 0, 2 * MS_PI);
  cairo_restore(r->cr);
  if(style->color) {
    msCairoSetSourceColor(r->cr, style->color);
    cairo_fill_preserve(r->cr);
  }
  if(style->outlinewidth > 0) {
    cairo_set_line_width (r->cr, style->outlinewidth);
    msCairoSetSourceColor(r->cr, style->outlinecolor);
    cairo_stroke_preserve(r->cr);
  }
  cairo_new_path(r->cr);
  return MS_SUCCESS;
}



int startLayerVectorCairo(imageObj *img, mapObj *map, layerObj *layer)
{
  if(layer->opacity<100) {
    cairo_renderer *r = CAIRO_RENDERER(img);
    cairo_push_group (r->cr);
  }
  return MS_SUCCESS;
}

int closeLayerVectorCairo(imageObj *img, mapObj *map, layerObj *layer)
{
  if(layer->opacity<100) {
    cairo_renderer *r = CAIRO_RENDERER(img);
    cairo_pop_group_to_source (r->cr);
    cairo_paint_with_alpha (r->cr, layer->opacity*0.01);
  }
  return MS_SUCCESS;
}

int startLayerRasterCairo(imageObj *img, mapObj *map, layerObj *layer)
{
  return MS_SUCCESS;
}

int closeLayerRasterCairo(imageObj *img, mapObj *map, layerObj *layer)
{
  return MS_SUCCESS;
}


int getRasterBufferHandleCairo(imageObj *img, rasterBufferObj *rb)
{
  unsigned char *pb;
  cairo_renderer *r = CAIRO_RENDERER(img);
  rb->type = MS_BUFFER_BYTE_RGBA;
  pb = cairo_image_surface_get_data(r->surface);
  rb->data.rgba.pixels = pb;
  rb->data.rgba.row_step = cairo_image_surface_get_stride(r->surface);
  rb->data.rgba.pixel_step=4;
  rb->width = cairo_image_surface_get_width(r->surface);
  rb->height = cairo_image_surface_get_height(r->surface);
  rb->data.rgba.r = &(pb[2]);
  rb->data.rgba.g = &(pb[1]);
  rb->data.rgba.b = &(pb[0]);
  if(r->use_alpha)
    rb->data.rgba.a = &(pb[3]);
  else
    rb->data.rgba.a = NULL;
  return MS_SUCCESS;
}

int getRasterBufferCopyCairo(imageObj *img, rasterBufferObj *rb)
{
  cairo_renderer *r = CAIRO_RENDERER(img);
  unsigned char *pb;
  rb->type = MS_BUFFER_BYTE_RGBA;
  rb->data.rgba.row_step = cairo_image_surface_get_stride(r->surface);
  rb->data.rgba.pixel_step=4;
  rb->width = cairo_image_surface_get_width(r->surface);
  rb->height = cairo_image_surface_get_height(r->surface);
  pb = (unsigned char*)malloc(rb->height * rb->data.rgba.row_step * sizeof(unsigned char));
  memcpy(pb,cairo_image_surface_get_data(r->surface),rb->height * rb->data.rgba.row_step);
  rb->data.rgba.pixels = pb;
  rb->data.rgba.r = &(pb[2]);
  rb->data.rgba.g = &(pb[1]);
  rb->data.rgba.b = &(pb[0]);
  if(r->use_alpha)
    rb->data.rgba.a = &(pb[3]);
  else
    rb->data.rgba.a = NULL;
  return MS_SUCCESS;
}



int mergeRasterBufferCairo(imageObj *img, rasterBufferObj *rb, double opacity,
                           int srcX, int srcY, int dstX, int dstY,
                           int width, int height)
{
  cairo_surface_t *src;
  cairo_renderer *r;
  /* not implemented for src,dst,width and height */
  if(rb->type != MS_BUFFER_BYTE_RGBA) {
    return MS_FAILURE;
  }
  r = CAIRO_RENDERER(img);

  src = cairo_image_surface_create_for_data(rb->data.rgba.pixels,CAIRO_FORMAT_ARGB32,
        rb->width,rb->height,
        rb->data.rgba.row_step);

  if(dstX||dstY||srcX||srcY||width!=img->width||height!=img->height) {
    cairo_set_source_surface (r->cr, src, dstX - srcX, dstY - srcY);
    cairo_rectangle (r->cr, dstX, dstY, width, height);
    cairo_fill (r->cr);
  } else {
    cairo_set_source_surface (r->cr,src,0,0);
    cairo_paint_with_alpha(r->cr,opacity);
  }
  cairo_surface_finish(src);
  cairo_surface_destroy(src);
  return MS_SUCCESS;
}


void freeSVGCache(symbolObj *s) {
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
      struct svg_symbol_cache *cache = s->renderer_cache;
      assert(cache->svgc);
#ifdef USE_SVG_CAIRO
      svg_cairo_destroy(cache->svgc);
#else
      rsvg_handle_close(cache->svgc, NULL);
  #if LIBRSVG_CHECK_VERSION(2,35,0)
      g_object_unref(cache->svgc);
  #else
      rsvg_handle_free(cache->svgc);
  #endif
#endif
      if(cache->pixmap_buffer) {
        msFreeRasterBuffer(cache->pixmap_buffer);
        free(cache->pixmap_buffer);
      }
      msFree(s->renderer_cache);
#endif
}

int freeSymbolCairo(symbolObj *s)
{
  if(!s->renderer_cache)
    return MS_SUCCESS;
  switch(s->type) {
    case MS_SYMBOL_VECTOR:
      cairo_path_destroy(s->renderer_cache);
      break;
    case MS_SYMBOL_PIXMAP:
      cairo_surface_destroy(s->renderer_cache);
      break;
    case MS_SYMBOL_SVG:
      freeSVGCache(s);
      break;
  }
  s->renderer_cache=NULL;
  return MS_SUCCESS;
}

int initializeRasterBufferCairo(rasterBufferObj *rb, int width, int height, int mode)
{
  rb->type = MS_BUFFER_BYTE_RGBA;
  rb->width = width;
  rb->height = height;
  rb->data.rgba.pixel_step = 4;
  rb->data.rgba.row_step = width * 4;
  rb->data.rgba.pixels = (unsigned char*)calloc(width*height*4,sizeof(unsigned char));
  rb->data.rgba.r = &(rb->data.rgba.pixels[2]);
  rb->data.rgba.g = &(rb->data.rgba.pixels[1]);
  rb->data.rgba.b = &(rb->data.rgba.pixels[0]);
  rb->data.rgba.a = &(rb->data.rgba.pixels[3]);
  return MS_SUCCESS;
}


int msPreloadSVGSymbol(symbolObj *symbol)
{
#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
  struct svg_symbol_cache *cache;

  if(!symbol->renderer_cache) {
    cache = msSmallCalloc(1,sizeof(struct svg_symbol_cache));
    symbol->renderer_free_func = &freeSVGCache;
  } else {
    cache = symbol->renderer_cache;
  }
  if(cache->svgc)
    return MS_SUCCESS;

#ifdef USE_SVG_CAIRO
  {
    unsigned int svg_width, svg_height;
    int status;
    status = svg_cairo_create(&cache->svgc);
    if (status) {
      msSetError(MS_RENDERERERR, "problem creating cairo svg", "msPreloadSVGSymbol()");
      return MS_FAILURE;
    }
    status = svg_cairo_parse(cache->svgc, symbol->full_pixmap_path);
    if (status) {
      msSetError(MS_RENDERERERR, "problem parsing svg symbol", "msPreloadSVGSymbol()");
      return MS_FAILURE;
    }
    svg_cairo_get_size (cache->svgc, &svg_width, &svg_height);
    if (svg_width == 0 || svg_height == 0) {
      msSetError(MS_RENDERERERR, "problem parsing svg symbol", "msPreloadSVGSymbol()");
      return MS_FAILURE;
    }

    symbol->sizex = svg_width;
    symbol->sizey = svg_height;
  }
#else
  {
    RsvgDimensionData dim;
    cache->svgc = rsvg_handle_new_from_file(symbol->full_pixmap_path,NULL);
    if(!cache->svgc) {
      msSetError(MS_RENDERERERR,"failed to load svg file %s", "msPreloadSVGSymbol()", symbol->full_pixmap_path);
      return MS_FAILURE;
    }
    rsvg_handle_get_dimensions_sub (cache->svgc, &dim, NULL);
    symbol->sizex = dim.width;
    symbol->sizey = dim.height;
  }
#endif

  symbol->renderer_cache = cache;

  return MS_SUCCESS;

#else
  msSetError(MS_MISCERR, "SVG Symbols requested but is not built with libsvgcairo",
             "msPreloadSVGSymbol()");
  return MS_FAILURE;
#endif
}



int msRenderRasterizedSVGSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style)
{

#if defined(USE_SVG_CAIRO) || defined(USE_RSVG)
  struct svg_symbol_cache *svg_cache;
  symbolStyleObj pixstyle;
  symbolObj pixsymbol;

  if(img->format->renderer == MS_RENDER_WITH_GD) {
    msSetError(MS_MISCERR, "GD renderer does not support SVG symbols", "msRenderRasterizedSVGSymbol()");
    return MS_FAILURE;
  }

  if(MS_SUCCESS != msPreloadSVGSymbol(symbol))
    return MS_FAILURE;
  svg_cache = (struct svg_symbol_cache*) symbol->renderer_cache;

  //already rendered at the right size and scale? return
  if(svg_cache->scale != style->scale || svg_cache->rotation != style->rotation) {
    cairo_t *cr;
    cairo_surface_t *surface;
    unsigned char *pb;
    int width, height, surface_w, surface_h;
    /* need to recompute the pixmap */
    if(svg_cache->pixmap_buffer) {
      msFreeRasterBuffer(svg_cache->pixmap_buffer);
    } else {
      svg_cache->pixmap_buffer = msSmallCalloc(1,sizeof(rasterBufferObj));
    }

    //increase pixmap size to accomodate scaling/rotation
    if (style->scale != 1.0) {
      width = surface_w = (symbol->sizex * style->scale + 0.5);
      height = surface_h = (symbol->sizey * style->scale + 0.5);
    } else {
      width = surface_w = symbol->sizex;
      height = surface_h = symbol->sizey;
    }
    if (style->rotation != 0) {
      surface_w = MS_NINT(width * 1.415);
      surface_h = MS_NINT(height * 1.415);
    }

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, surface_w, surface_h);
    cr = cairo_create(surface);

    if (style->rotation != 0) {
      cairo_translate(cr, surface_w / 2, surface_h / 2);
      cairo_rotate(cr, -style->rotation);
      cairo_translate(cr, -width / 2, -height / 2);
    }
    if (style->scale != 1.0) {
      cairo_scale(cr, style->scale, style->scale);
    }
#ifdef USE_SVG_CAIRO
    if(svg_cairo_render(svg_cache->svgc, cr) != SVG_CAIRO_STATUS_SUCCESS) {
      return MS_FAILURE;
    }
#else
  rsvg_handle_render_cairo(svg_cache->svgc, cr);
#endif
    pb = cairo_image_surface_get_data(surface);

    //set up raster
    initializeRasterBufferCairo(svg_cache->pixmap_buffer, surface_w, surface_h, 0);
    memcpy(svg_cache->pixmap_buffer->data.rgba.pixels, pb, surface_w * surface_h * 4 * sizeof (unsigned char));
    svg_cache->scale = style->scale;
    svg_cache->rotation = style->rotation;
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
  }
  assert(svg_cache->pixmap_buffer->height && svg_cache->pixmap_buffer->width);

  pixstyle = *style;
  pixstyle.rotation = 0.0;
  pixstyle.scale = 1.0;

  pixsymbol.pixmap_buffer = svg_cache->pixmap_buffer;
  pixsymbol.type = MS_SYMBOL_PIXMAP;

  MS_IMAGE_RENDERER(img)->renderPixmapSymbol(img,x,y,&pixsymbol,&pixstyle);
  MS_IMAGE_RENDERER(img)->freeSymbol(&pixsymbol);
  return MS_SUCCESS;
#else
  msSetError(MS_MISCERR, "SVG Symbols requested but MapServer is not built with libsvgcairo",
             "renderSVGSymbolCairo()");
  return MS_FAILURE;
#endif
}

void msCairoCleanup() {
   cairo_debug_reset_static_data();
}

#endif /*USE_CAIRO*/


int msPopulateRendererVTableCairoRaster( rendererVTableObj *renderer )
{
#ifdef USE_CAIRO
  renderer->supports_pixel_buffer=1;
  renderer->supports_transparent_layers = 0;
  renderer->supports_svg = 1;
  renderer->default_transform_mode = MS_TRANSFORM_SIMPLIFY;
  initializeCache(&MS_RENDERER_CACHE(renderer));
  renderer->startLayer = startLayerRasterCairo;
  renderer->endLayer = closeLayerRasterCairo;
  renderer->renderLineTiled = NULL;
  renderer->renderLine=&renderLineCairo;
  renderer->createImage=&createImageCairo;
  renderer->saveImage=&saveImageCairo;
  renderer->getRasterBufferHandle=&getRasterBufferHandleCairo;
  renderer->getRasterBufferCopy=&getRasterBufferCopyCairo;
  renderer->renderPolygon=&renderPolygonCairo;
  renderer->renderGlyphs=&renderGlyphsCairo;
  renderer->freeImage=&freeImageCairo;
  renderer->renderEllipseSymbol = &renderEllipseSymbolCairo;
  renderer->renderVectorSymbol = &renderVectorSymbolCairo;
  renderer->renderTruetypeSymbol = &renderTruetypeSymbolCairo;
  renderer->renderSVGSymbol = &renderSVGSymbolCairo;
  renderer->renderPixmapSymbol = &renderPixmapSymbolCairo;
  renderer->mergeRasterBuffer = &mergeRasterBufferCairo;
  renderer->getTruetypeTextBBox = &getTruetypeTextBBoxCairo;
  renderer->renderTile = &renderTileCairo;
  renderer->loadImageFromFile = &msLoadMSRasterBufferFromFile;
  renderer->renderPolygonTiled = &renderPolygonTiledCairo;
  renderer->freeSymbol = &freeSymbolCairo;
  renderer->cleanup = &cleanupCairo;
  return MS_SUCCESS;
#else
  msSetError(MS_MISCERR, "Cairo Driver requested but MapServer is not built in",
             "msPopulateRendererVTableCairoRaster()");
  return MS_FAILURE;
#endif
}

int populateRendererVTableCairoVector( rendererVTableObj *renderer )
{
#ifdef USE_CAIRO
  renderer->use_imagecache=0;
  renderer->supports_pixel_buffer=0;
  renderer->supports_transparent_layers = 1;
  renderer->supports_svg = 1;
  renderer->default_transform_mode = MS_TRANSFORM_SIMPLIFY;
  initializeCache(&MS_RENDERER_CACHE(renderer));
  renderer->startLayer = startLayerVectorCairo;
  renderer->endLayer = closeLayerVectorCairo;
  renderer->renderLine=&renderLineCairo;
  renderer->renderLineTiled = NULL;
  renderer->createImage=&createImageCairo;
  renderer->saveImage=&saveImageCairo;
  renderer->saveImageBuffer = &saveImageBufferCairo;
  renderer->getRasterBufferHandle=&getRasterBufferHandleCairo;
  renderer->renderPolygon=&renderPolygonCairo;
  renderer->renderGlyphs=&renderGlyphsCairo;
  renderer->freeImage=&freeImageCairo;
  renderer->renderEllipseSymbol = &renderEllipseSymbolCairo;
  renderer->renderVectorSymbol = &renderVectorSymbolCairo;
  renderer->renderTruetypeSymbol = &renderTruetypeSymbolCairo;
  renderer->renderSVGSymbol = &renderSVGSymbolCairo;
  renderer->renderPixmapSymbol = &renderPixmapSymbolCairo;
  renderer->loadImageFromFile = &msLoadMSRasterBufferFromFile;
  renderer->mergeRasterBuffer = &mergeRasterBufferCairo;
  renderer->initializeRasterBuffer = initializeRasterBufferCairo;
  renderer->getTruetypeTextBBox = &getTruetypeTextBBoxCairo;
  renderer->renderTile = &renderTileCairo;
  renderer->renderPolygonTiled = &renderPolygonTiledCairo;
  renderer->freeSymbol = &freeSymbolCairo;
  renderer->cleanup = &cleanupCairo;
  return MS_SUCCESS;
#else
  msSetError(MS_MISCERR, "Cairo Driver requested but MapServer is not built in",
             "msPopulateRendererVTableCairoRaster()");
  return MS_FAILURE;
#endif
}

int msPopulateRendererVTableCairoSVG( rendererVTableObj *renderer )
{
  return populateRendererVTableCairoVector(renderer);
}
int msPopulateRendererVTableCairoPDF( rendererVTableObj *renderer )
{
  return populateRendererVTableCairoVector(renderer);
}


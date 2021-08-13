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
#include <cairo-win32.h>
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

#include <cpl_string.h>
#include <gdal.h>

#include "fontcache.h"

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

typedef struct cairoFaceCache cairoFaceCache;
struct cairoFaceCache {
  cairo_font_face_t *face;
  FT_Face ftface;
  cairo_font_options_t *options;
  cairoFaceCache *next;
};

void freeCairoFaceCache(cairoFaceCache *fc) {
  cairo_font_face_destroy(fc->face);
  cairo_font_options_destroy(fc->options);
}

typedef struct {
  cairoFaceCache *cairofacecache;
  /* dummy surface and context */
  unsigned char dummydata[4];
  cairo_surface_t *dummysurface;
  cairo_t *dummycr;
} cairoCacheData;

void initializeCache(void **vcache)
{
  cairoCacheData *cache = (cairoCacheData*)malloc(sizeof(cairoCacheData));
  *vcache = cache;

  cache->cairofacecache = NULL;
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
  if(ccache->cairofacecache) {
    cairoFaceCache *next,*cur;
    cur = ccache->cairofacecache;
    do {
      next = cur->next;
      freeCairoFaceCache(cur);
      free(cur);
      cur=next;
    } while(cur);
  }

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

static cairoFaceCache* getCairoFontFace(cairoCacheData *cache, FT_Face ftface) {
  cairoFaceCache *cur = cache->cairofacecache;
  while(cur) {
    if(cur->ftface == ftface) return cur;
    cur = cur->next;
  }
  cur = msSmallMalloc(sizeof(cairoFaceCache));
  cur->next = cache->cairofacecache;
  cache->cairofacecache = cur;
  cur->ftface = ftface;
  cur->face = cairo_ft_font_face_create_for_ft_face(ftface, 0);
  cur->options = cairo_font_options_create();
  cairo_font_options_set_hint_style(cur->options,CAIRO_HINT_STYLE_NONE);
  return cur;
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

int renderGlyphs2Cairo(imageObj *img, const textSymbolObj *ts, colorObj *c, colorObj *oc, int ow, int isMarker) {
  const textPathObj *tp = ts->textpath;
  cairo_renderer *r = CAIRO_RENDERER(img);
  cairoCacheData *cache = MS_IMAGE_RENDERER_CACHE(img);
  cairoFaceCache *cairo_face = NULL;
  FT_Face prevface = NULL;
  int g;
  (void)isMarker;

  cairo_set_font_size(r->cr,MS_NINT(tp->glyph_size * 96.0/72.0));
  for(g=0;g<tp->numglyphs;g++) {
    glyphObj *gl = &tp->glyphs[g];
    cairo_glyph_t glyph;
    /* load the glyph's face into cairo, if not already present */
    if(gl->face->face != prevface) {
      cairo_face = getCairoFontFace(cache,gl->face->face);
      cairo_set_font_face(r->cr, cairo_face->face);
      cairo_set_font_options(r->cr,cairo_face->options);
      prevface = gl->face->face;
      cairo_set_font_size(r->cr,MS_NINT(tp->glyph_size * 96.0/72.0));
    }
    cairo_save(r->cr);
    cairo_translate(r->cr,gl->pnt.x,gl->pnt.y);
    if(gl->rot != 0.0)
      cairo_rotate(r->cr, -gl->rot);
    glyph.x = glyph.y = 0;
    glyph.index = gl->glyph->key.codepoint;
    cairo_glyph_path(r->cr,&glyph,1);
    cairo_restore(r->cr);
  }
  if (oc) {
    cairo_save(r->cr);
    msCairoSetSourceColor(r->cr, oc);
    cairo_set_line_width(r->cr, ow + 1);
    cairo_stroke_preserve(r->cr);
    cairo_restore(r->cr);
  }
  if(c) {
    msCairoSetSourceColor(r->cr, c);
    cairo_fill(r->cr);
  }
  cairo_new_path(r->cr);
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
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1,15,10)
      {
          const char* msPDFCreationDate = getenv("MS_PDF_CREATION_DATE");
          if( msPDFCreationDate )
          {
              cairo_pdf_surface_set_metadata (r->surface,
                                              CAIRO_PDF_METADATA_CREATE_DATE,
                                              msPDFCreationDate);
          }
      }
#endif
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
}

int saveImageCairo(imageObj *img, mapObj *map, FILE *fp, outputFormatObj *format_unused)
{
  (void)format_unused;
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

unsigned char* saveImageBufferCairo(imageObj *img, int *size_ptr, outputFormatObj *format_unused)
{
  (void)format_unused;
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
  (void)map;
  if(layer->compositer && layer->compositer->opacity<100) {
    cairo_renderer *r = CAIRO_RENDERER(img);
    cairo_push_group (r->cr);
  }
  return MS_SUCCESS;
}

int closeLayerVectorCairo(imageObj *img, mapObj *map, layerObj *layer)
{
  (void)map;
  if(layer->compositer && layer->compositer->opacity<100) {
    cairo_renderer *r = CAIRO_RENDERER(img);
    cairo_pop_group_to_source (r->cr);
    cairo_paint_with_alpha (r->cr, layer->compositer->opacity*0.01);
  }
  return MS_SUCCESS;
}

int startLayerRasterCairo(imageObj *img, mapObj *map, layerObj *layer)
{
  (void)img;
  (void)map;
  (void)layer;
  return MS_SUCCESS;
}

int closeLayerRasterCairo(imageObj *img, mapObj *map, layerObj *layer)
{
  (void)img;
  (void)map;
  (void)layer;
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

static cairo_operator_t ms2cairo_compop(CompositingOperation op) {
  switch(op) {
    case MS_COMPOP_CLEAR:
      return CAIRO_OPERATOR_CLEAR;
    case MS_COMPOP_SRC:
      return CAIRO_OPERATOR_SOURCE;
    case MS_COMPOP_DST:
      return CAIRO_OPERATOR_DEST;
    case MS_COMPOP_SRC_OVER:
      return CAIRO_OPERATOR_OVER;
    case MS_COMPOP_DST_OVER:
      return CAIRO_OPERATOR_DEST_OVER;
    case MS_COMPOP_SRC_IN:
      return CAIRO_OPERATOR_IN;
    case MS_COMPOP_DST_IN:
      return CAIRO_OPERATOR_DEST_IN;
    case MS_COMPOP_SRC_OUT:
      return CAIRO_OPERATOR_OUT;
    case MS_COMPOP_DST_OUT:
      return CAIRO_OPERATOR_DEST_OUT;
    case MS_COMPOP_SRC_ATOP:
      return CAIRO_OPERATOR_ATOP;
    case MS_COMPOP_DST_ATOP:
      return CAIRO_OPERATOR_DEST_ATOP;
    case MS_COMPOP_XOR:
      return CAIRO_OPERATOR_XOR;
    case MS_COMPOP_PLUS:
      return CAIRO_OPERATOR_ADD;
    case MS_COMPOP_MULTIPLY:
      return CAIRO_OPERATOR_MULTIPLY;
    case MS_COMPOP_SCREEN:
      return CAIRO_OPERATOR_SCREEN;
    case MS_COMPOP_OVERLAY:
      return CAIRO_OPERATOR_OVERLAY;
    case MS_COMPOP_DARKEN:
      return CAIRO_OPERATOR_DARKEN;
    case MS_COMPOP_LIGHTEN:
      return CAIRO_OPERATOR_LIGHTEN;
    case MS_COMPOP_COLOR_DODGE:
      return CAIRO_OPERATOR_COLOR_DODGE;
    case MS_COMPOP_COLOR_BURN:
      return CAIRO_OPERATOR_COLOR_BURN;
    case MS_COMPOP_HARD_LIGHT:
      return CAIRO_OPERATOR_HARD_LIGHT;
    case MS_COMPOP_SOFT_LIGHT:
      return CAIRO_OPERATOR_SOFT_LIGHT;
    case MS_COMPOP_DIFFERENCE:
      return CAIRO_OPERATOR_DIFFERENCE;
    case MS_COMPOP_EXCLUSION:
      return CAIRO_OPERATOR_EXCLUSION;
    case MS_COMPOP_INVERT:
    case MS_COMPOP_INVERT_RGB:
    case MS_COMPOP_MINUS:
    case MS_COMPOP_CONTRAST:
    default:
      return CAIRO_OPERATOR_OVER;
  }
}

int cairoCompositeRasterBuffer(imageObj *img, rasterBufferObj *rb, CompositingOperation comp, int opacity) {
  cairo_surface_t *src;
  cairo_renderer *r;
  if(rb->type != MS_BUFFER_BYTE_RGBA) {
    return MS_FAILURE;
  }
  r = CAIRO_RENDERER(img);

  src = cairo_image_surface_create_for_data(rb->data.rgba.pixels,CAIRO_FORMAT_ARGB32,
        rb->width,rb->height,
        rb->data.rgba.row_step);

  cairo_set_source_surface (r->cr,src,0,0);
  cairo_set_operator(r->cr, ms2cairo_compop(comp));
  cairo_paint_with_alpha(r->cr,opacity/100.0);
  cairo_surface_finish(src);
  cairo_surface_destroy(src);
  cairo_set_operator(r->cr,CAIRO_OPERATOR_OVER);
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
      if(!cache) return;
      assert(cache->svgc);
#ifdef USE_SVG_CAIRO
      svg_cairo_destroy(cache->svgc);
#else
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
      s->renderer_cache = NULL;
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
  (void)mode;
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
    cache->svgc = rsvg_handle_new_from_file(symbol->full_pixmap_path,NULL);
    if(!cache->svgc) {
      msSetError(MS_RENDERERERR,"failed to load svg file %s", "msPreloadSVGSymbol()", symbol->full_pixmap_path);
      return MS_FAILURE;
    }
#if LIBRSVG_CHECK_VERSION(2,46,0)
    /* rsvg_handle_get_dimensions_sub() is deprecated since librsvg 2.46 */
    /* It seems rsvg_handle_get_intrinsic_dimensions() is the best equivalent */
    /* when the <svg> root node includes a width and height attributes in pixels */
    gboolean has_width = FALSE;
    RsvgLength width = {0, RSVG_UNIT_PX};
    gboolean has_height = FALSE;
    RsvgLength height = {0, RSVG_UNIT_PX};
    rsvg_handle_get_intrinsic_dimensions(cache->svgc,
                                         &has_width, &width,
                                         &has_height, &height,
                                         NULL, NULL);
    if( has_width && width.unit == RSVG_UNIT_PX &&
        has_height && height.unit == RSVG_UNIT_PX )
    {
        symbol->sizex = width.length;
        symbol->sizey = height.length;
    }
    else
    {
        RsvgRectangle ink_rect = { 0, 0, 0, 0 };
        rsvg_handle_get_geometry_for_element (cache->svgc, NULL, &ink_rect, NULL, NULL);
        symbol->sizex = ink_rect.width;
        symbol->sizey = ink_rect.height;
    }
#else
    RsvgDimensionData dim;
    rsvg_handle_get_dimensions_sub (cache->svgc, &dim, NULL);
    symbol->sizex = dim.width;
    symbol->sizey = dim.height;
#endif
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
  int status;

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
      surface_w = surface_h = MS_NINT(MS_MAX(height, width) * 1.415);
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

  status = MS_IMAGE_RENDERER(img)->renderPixmapSymbol(img,x,y,&pixsymbol,&pixstyle);
  MS_IMAGE_RENDERER(img)->freeSymbol(&pixsymbol);
  return status;
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
  renderer->compositeRasterBuffer = cairoCompositeRasterBuffer;
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
  renderer->renderGlyphs=&renderGlyphs2Cairo;
  renderer->freeImage=&freeImageCairo;
  renderer->renderEllipseSymbol = &renderEllipseSymbolCairo;
  renderer->renderVectorSymbol = &renderVectorSymbolCairo;
  renderer->renderSVGSymbol = &renderSVGSymbolCairo;
  renderer->renderPixmapSymbol = &renderPixmapSymbolCairo;
  renderer->mergeRasterBuffer = &mergeRasterBufferCairo;
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
  renderer->compositeRasterBuffer = NULL;
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
  renderer->renderGlyphs=&renderGlyphs2Cairo;
  renderer->freeImage=&freeImageCairo;
  renderer->renderEllipseSymbol = &renderEllipseSymbolCairo;
  renderer->renderVectorSymbol = &renderVectorSymbolCairo;
  renderer->renderSVGSymbol = &renderSVGSymbolCairo;
  renderer->renderPixmapSymbol = &renderPixmapSymbolCairo;
  renderer->loadImageFromFile = &msLoadMSRasterBufferFromFile;
  renderer->mergeRasterBuffer = &mergeRasterBufferCairo;
  renderer->initializeRasterBuffer = initializeRasterBufferCairo;
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


/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  UTFGrid rendering functions (using AGG)
 * Author:   Francois Desjarlais
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
#include "maputfgrid.h"
#include "mapagg.h"
#include "renderers/agg/include/agg_rasterizer_scanline_aa.h"
#include "renderers/agg/include/agg_basics.h"
#include "renderers/agg/include/agg_renderer_scanline.h"
#include "renderers/agg/include/agg_scanline_bin.h"
#include "renderers/agg/include/agg_gamma_functions.h"
#include "renderers/agg/include/agg_conv_stroke.h"
#include "renderers/agg/include/agg_ellipse.h"

#include <memory>
#include <vector>

typedef mapserver::int32u band_type;
typedef mapserver::row_ptr_cache<band_type> rendering_buffer;
typedef pixfmt_utf<utfpix32, rendering_buffer> pixfmt_utf32;
typedef mapserver::renderer_base<pixfmt_utf32> renderer_base;
typedef mapserver::rasterizer_scanline_aa<> rasterizer_scanline;
typedef mapserver::renderer_scanline_bin_solid<renderer_base> renderer_scanline;

static const utfpix32 UTF_WATER = utfpix32(32);

#define utfitem(c) utfpix32(c)

namespace {

struct shapeData {
  std::string datavalues{};
  std::string itemvalue{};
  band_type utfvalue = 0;
  int serialid = 0;
};

/*
 * UTFGrid specific polygon adaptor.
 */
class polygon_adaptor_utf final : public polygon_adaptor {
public:
  polygon_adaptor_utf(shapeObj *shape, int utfres_in)
      : polygon_adaptor(shape), utfresolution(utfres_in) {}

  unsigned vertex(double *x, double *y) override {
    if (m_point < m_pend) {
      bool first = m_point == m_line->point;
      *x = m_point->x / utfresolution;
      *y = m_point->y / utfresolution;
      m_point++;
      return first ? mapserver::path_cmd_move_to : mapserver::path_cmd_line_to;
    }
    *x = *y = 0.0;
    if (!m_stop) {

      m_line++;
      if (m_line >= m_lend) {
        m_stop = true;
        return mapserver::path_cmd_end_poly;
      }

      m_point = m_line->point;
      m_pend = &(m_line->point[m_line->numpoints]);
      return mapserver::path_cmd_end_poly;
    }
    return mapserver::path_cmd_stop;
  }

private:
  int utfresolution;
};

/*
 * UTFGrid specific line adaptor.
 */
class line_adaptor_utf final : public line_adaptor {
public:
  line_adaptor_utf(shapeObj *shape, int utfres_in)
      : line_adaptor(shape), utfresolution(utfres_in) {}

  unsigned vertex(double *x, double *y) override {
    if (m_point < m_pend) {
      bool first = m_point == m_line->point;
      *x = m_point->x / utfresolution;
      *y = m_point->y / utfresolution;
      m_point++;
      return first ? mapserver::path_cmd_move_to : mapserver::path_cmd_line_to;
    }
    m_line++;
    *x = *y = 0.0;
    if (m_line >= m_lend)
      return mapserver::path_cmd_stop;

    m_point = m_line->point;
    m_pend = &(m_line->point[m_line->numpoints]);

    return vertex(x, y);
  }

private:
  int utfresolution;
};

class UTFGridRenderer {
public:
  std::vector<shapeData> table{};
  int utfresolution = 0;
  int layerwatch = 0;
  bool renderlayer = false;
  bool useutfitem = false;
  bool useutfdata = false;
  bool duplicates = false;
  band_type utfvalue = 0;
  layerObj *utflayer = nullptr;
  int width = 0;
  int height = 0;
  std::vector<band_type> buffer{};
  rendering_buffer m_rendering_buffer{};
  pixfmt_utf32 m_pixel_format{};
  renderer_base m_renderer_base{};
  rasterizer_scanline m_rasterizer{};
  renderer_scanline m_renderer_scanline{};
  mapserver::scanline_bin sl_utf{};
  std::unique_ptr<mapserver::conv_stroke<line_adaptor_utf>> stroke{};
};

} // namespace

#define UTFGRID_RENDERER(image)                                                \
  (static_cast<UTFGridRenderer *>((image)->img.plugin))

/*
 * Encode to avoid unavailable char in the JSON
 */
static unsigned int encodeForRendering(unsigned int toencode) {
  unsigned int encoded;
  encoded = toencode + 32;
  /* 34 => " */
  if (encoded >= 34) {
    encoded = encoded + 1;
  }
  /* 92 => \ */
  if (encoded >= 92) {
    encoded = encoded + 1;
  }
  return encoded;
}

/*
 * Decode value to have the initial one
 */
static unsigned int decodeRendered(unsigned int todecode) {
  unsigned int decoded;

  if (todecode >= 92)
    todecode--;

  if (todecode >= 34)
    todecode--;

  decoded = todecode - 32;

  return decoded;
}

/*
 * Add the shapeObj UTFDATA and UTFITEM to the lookup table.
 */
static band_type addToTable(UTFGridRenderer *r, shapeObj *p) {
  /* Looks for duplicates. */
  if (r->duplicates == false && r->useutfitem) {
    const auto itemvalue = p->values[r->utflayer->utfitemindex];
    for (const auto &tableItem : r->table) {
      if (tableItem.itemvalue == itemvalue) {
        /* Found a copy of the values in the table. */
        return tableItem.utfvalue;
      }
    }
  }

  /* Data added to the table */
  shapeData table;
  char *escaped = msEvalTextExpressionJSonEscape(&r->utflayer->utfdata, p);
  table.datavalues = escaped ? escaped : "";
  msFree(escaped);

  /* If UTFITEM is set in the mapfile we add its value to the table */
  if (r->useutfitem)
    table.itemvalue = p->values[r->utflayer->utfitemindex];

  table.serialid = static_cast<int>(r->table.size() + 1);

  /* Simple operation so we don't have unavailable char in the JSON */
  const auto utfvalue = encodeForRendering(table.serialid);
  table.utfvalue = utfvalue;

  r->table.emplace_back(std::move(table));

  return utfvalue;
}

/*
 * Use AGG to render any path.
 */
template <class vertex_source>
static int utfgridRenderPath(imageObj *img, vertex_source &path) {
  UTFGridRenderer *r = UTFGRID_RENDERER(img);
  r->m_rasterizer.reset();
  r->m_rasterizer.filling_rule(mapserver::fill_even_odd);
  r->m_rasterizer.add_path(path);
  r->m_renderer_scanline.color(utfitem(r->utfvalue));
  mapserver::render_scanlines(r->m_rasterizer, r->sl_utf,
                              r->m_renderer_scanline);
  return MS_SUCCESS;
}

/*
 * Initialize the renderer, create buffer, allocate memory.
 */
static imageObj *utfgridCreateImage(int width, int height,
                                    outputFormatObj *format,
                                    colorObj * /*bg*/) {
  auto r = std::unique_ptr<UTFGridRenderer>(new UTFGridRenderer);

  r->utfresolution =
      atof(msGetOutputFormatOption(format, "UTFRESOLUTION", "4"));
  if (r->utfresolution < 1) {
    msSetError(MS_MISCERR, "UTFRESOLUTION smaller that 1 in the mapfile.",
               "utfgridCreateImage()");
    return nullptr;
  }
  r->duplicates =
      EQUAL("true", msGetOutputFormatOption(format, "DUPLICATES", "true"));

  r->width = width / r->utfresolution;
  r->height = height / r->utfresolution;

  r->buffer.resize(static_cast<size_t>(r->width) * r->height);

  /* AGG specific operations */
  r->m_rendering_buffer.attach(&r->buffer[0], r->width, r->height, r->width);
  r->m_pixel_format.attach(r->m_rendering_buffer);
  r->m_renderer_base.attach(r->m_pixel_format);
  r->m_renderer_scanline.attach(r->m_renderer_base);
  r->m_renderer_base.clear(UTF_WATER);
  r->m_rasterizer.gamma(mapserver::gamma_none());

  imageObj *image = (imageObj *)msSmallCalloc(1, sizeof(imageObj));
  image->img.plugin = r.release();

  return image;
}

/*
 * Free all the memory used by the renderer.
 */
static int utfgridFreeImage(imageObj *img) {
  UTFGridRenderer *r = UTFGRID_RENDERER(img);
  delete r;

  img->img.plugin = NULL;

  return MS_SUCCESS;
}

/*
 * Update a character in the utfgrid.
 */
static int utfgridUpdateChar(imageObj *img, band_type oldChar,
                             band_type newChar) {
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  for (auto &ch : r->buffer) {
    if (ch == oldChar) {
      ch = newChar;
    }
  }

  return MS_SUCCESS;
}

/*
 * Remove unnecessary data that didn't made it to the final grid.
 */
static int utfgridCleanData(imageObj *img) {
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  std::vector<bool> usedChar;
  usedChar.resize(r->table.size());

  int itemFound = 0;

  for (const auto ch : r->buffer) {
    const auto rendered = decodeRendered(ch);
    if (rendered != 0 && !usedChar[rendered - 1]) {
      itemFound++;
      usedChar[rendered - 1] = true;
    }
  }

  std::vector<shapeData> updatedData;
  updatedData.reserve(itemFound);

  for (const auto &tableItem : r->table) {
    if (usedChar[decodeRendered(tableItem.utfvalue) - 1]) {
      updatedData.emplace_back(tableItem);
      auto &newData = updatedData.back();

      newData.serialid = static_cast<int>(updatedData.size());

      const band_type utfvalue = encodeForRendering(newData.serialid);

      utfgridUpdateChar(img, newData.utfvalue, utfvalue);
      newData.utfvalue = utfvalue;
    }
  }

  r->table = std::move(updatedData);

  return MS_SUCCESS;
}

/*
 * Print the renderer data as JSON.
 */
static int utfgridSaveImage(imageObj *img, mapObj * /*map*/, FILE *fp,
                            outputFormatObj * /*format*/) {
  utfgridCleanData(img);

  const UTFGridRenderer *renderer = UTFGRID_RENDERER(img);

  if (renderer->layerwatch > 1)
    return MS_FAILURE;

  const int imgheight = renderer->height;
  const int imgwidth = renderer->width;

  msIO_fprintf(fp, "{\"grid\":[");

  /* Print the buffer */
  std::vector<wchar_t> string;
  string.resize(imgwidth + 1);
  for (int row = 0; row < imgheight; row++) {

    /* Need a comma between each line but JSON must not start with a comma. */
    if (row != 0)
      msIO_fprintf(fp, ",");
    msIO_fprintf(fp, "\"");
    for (int col = 0; col < imgwidth; col++) {
      /* Get the data from buffer. */
      band_type pixelid = renderer->buffer[(row * imgwidth) + col];
      string[col] = pixelid;
    }

    /* Conversion to UTF-8 encoding */
#if defined(_WIN32) && !defined(__CYGWIN__)
    const char *encoding = "UCS-2LE";
#else
    const char *encoding = "UCS-4LE";
#endif

    char *utf8 = msConvertWideStringToUTF8(string.data(), encoding);
    msIO_fprintf(fp, "%s", utf8);
    msFree(utf8);
    msIO_fprintf(fp, "\"");
  }

  msIO_fprintf(fp, "],\"keys\":[\"\"");

  /* Print the specified key */
  for (const auto &table : renderer->table) {
    msIO_fprintf(fp, ",");

    if (renderer->useutfitem) {
      char *pszEscaped = msEscapeJSonString(table.itemvalue.c_str());
      msIO_fprintf(fp, "\"%s\"", pszEscaped);
      msFree(pszEscaped);
    }
    /* If no UTFITEM specified use the serial ID as the key */
    else
      msIO_fprintf(fp, "\"%i\"", table.serialid);
  }

  msIO_fprintf(fp, "],\"data\":{");

  /* Print the data */
  if (renderer->useutfdata) {
    bool first = true;
    for (const auto &table : renderer->table) {
      if (!first)
        msIO_fprintf(fp, ",");
      first = false;

      if (renderer->useutfitem) {
        char *pszEscaped = msEscapeJSonString(table.itemvalue.c_str());
        msIO_fprintf(fp, "\"%s\":", pszEscaped);
        msFree(pszEscaped);
      }
      /* If no UTFITEM specified use the serial ID as the key */
      else
        msIO_fprintf(fp, "\"%i\":", table.serialid);

      msIO_fprintf(fp, "%s", table.datavalues.c_str());
    }
  }
  msIO_fprintf(fp, "}}");

  return MS_SUCCESS;
}

/*
 * Starts a layer for UTFGrid renderer.
 */
static int utfgridStartLayer(imageObj *img, mapObj * /*map*/, layerObj *layer) {
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* Look if the layer uses the UTFGrid output format */
  if (layer->utfdata.string != 0) {
    r->useutfdata = true;
  }

  /* layerwatch is set to 1 on first layer treated. Doesn't allow multiple
   * layers. */
  if (!r->layerwatch) {
    r->layerwatch++;

    r->renderlayer = true;
    r->utflayer = layer;
    layer->refcount++;

    /* Verify if renderer needs to use UTFITEM */
    if (r->utflayer->utfitem)
      r->useutfitem = true;
  }
  /* If multiple layers, send error */
  else {
    r->layerwatch++;
    msSetError(MS_MISCERR,
               "MapServer does not support multiple UTFGrid layers rendering "
               "simultaneously.",
               "utfgridStartLayer()");
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/*
 * Tell renderer the layer is done.
 */
static int utfgridEndLayer(imageObj *img, mapObj * /*map*/, layerObj *layer) {
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* Look if the layer was rendered, if it was then turn off rendering. */
  if (r->renderlayer) {
    r->utflayer = NULL;
    layer->refcount--;
    r->renderlayer = false;
  }

  return MS_SUCCESS;
}

/*
 * Do the table operations on the shapes. Allow multiple types of data to be
 * rendered.
 */
static int utfgridStartShape(imageObj *img, shapeObj *shape) {
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  if (!r->renderlayer)
    return MS_FAILURE;

  /* Table operations */
  r->utfvalue = addToTable(r, shape);

  return MS_SUCCESS;
}

/*
 * Tells the renderer that the shape's rendering is done.
 */
static int utfgridEndShape(imageObj *img, shapeObj *) {
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  r->utfvalue = 0;
  return MS_SUCCESS;
}

/*
 * Function that renders polygons into UTFGrid.
 */
static int utfgridRenderPolygon(imageObj *img, shapeObj *polygonshape,
                                colorObj *) {
  const UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if (r->utfvalue == 0) {
    return MS_FAILURE;
  }

  /* Render the polygon */
  polygon_adaptor_utf polygons(polygonshape, r->utfresolution);
  utfgridRenderPath(img, polygons);

  return MS_SUCCESS;
}

/*
 * Function that renders lines into UTFGrid. Starts by looking if the line is a
 * polygon outline, draw it if it's not.
 */
static int utfgridRenderLine(imageObj *img, shapeObj *lineshape,
                             strokeStyleObj *linestyle) {
  UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if (r->utfvalue == 0) {
    return MS_SUCCESS;
    /* If we don't get a character to draw we just skip execution
     * instead of failing
     */
  }

  /* Render the line */
  line_adaptor_utf lines(lineshape, r->utfresolution);

  if (!r->stroke) {
    r->stroke.reset(new mapserver::conv_stroke<line_adaptor_utf>(lines));
  } else {
    r->stroke->attach(lines);
  }
  r->stroke->width(linestyle->width / r->utfresolution);
  utfgridRenderPath(img, *r->stroke);

  return MS_SUCCESS;
}

/*
 * Function that render vector type symbols into UTFGrid.
 */
static int utfgridRenderVectorSymbol(imageObj *img, double x, double y,
                                     symbolObj *symbol, symbolStyleObj *style) {
  const UTFGridRenderer *r = UTFGRID_RENDERER(img);
  double ox = symbol->sizex * 0.5;
  double oy = symbol->sizey * 0.5;

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if (r->utfvalue == 0) {
    return MS_FAILURE;
  }

  /* Pathing the symbol */
  mapserver::path_storage path = imageVectorSymbol(symbol);

  /* Transformation to the right size/scale. */
  mapserver::trans_affine mtx;
  mtx *= mapserver::trans_affine_translation(-ox, -oy);
  mtx *= mapserver::trans_affine_scaling(style->scale / r->utfresolution);
  mtx *= mapserver::trans_affine_rotation(-style->rotation);
  mtx *= mapserver::trans_affine_translation(x / r->utfresolution,
                                             y / r->utfresolution);
  path.transform(mtx);

  /* Rendering the symbol. */
  utfgridRenderPath(img, path);

  return MS_SUCCESS;
}

/*
 * Function that renders Pixmap type symbols into UTFGrid.
 */
static int utfgridRenderPixmapSymbol(imageObj *img, double x, double y,
                                     symbolObj *symbol, symbolStyleObj *style) {
  const UTFGridRenderer *r = UTFGRID_RENDERER(img);
  rasterBufferObj *pixmap = symbol->pixmap_buffer;

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if (r->utfvalue == 0) {
    return MS_FAILURE;
  }

  /* Pathing the symbol BBox */
  mapserver::path_storage pixmap_bbox;
  double w, h;
  w = pixmap->width * style->scale / (2.0);
  h = pixmap->height * style->scale / (2.0);
  pixmap_bbox.move_to((x - w) / r->utfresolution, (y - h) / r->utfresolution);
  pixmap_bbox.line_to((x + w) / r->utfresolution, (y - h) / r->utfresolution);
  pixmap_bbox.line_to((x + w) / r->utfresolution, (y + h) / r->utfresolution);
  pixmap_bbox.line_to((x - w) / r->utfresolution, (y + h) / r->utfresolution);

  /* Rendering the symbol */
  utfgridRenderPath(img, pixmap_bbox);

  return MS_SUCCESS;
}

/*
 * Function that render ellipse type symbols into UTFGrid.
 */
static int utfgridRenderEllipseSymbol(imageObj *img, double x, double y,
                                      symbolObj *symbol,
                                      symbolStyleObj *style) {
  const UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if (r->utfvalue == 0) {
    return MS_FAILURE;
  }

  /* Pathing the symbol. */
  mapserver::path_storage path;
  mapserver::ellipse ellipse(
      x / r->utfresolution, y / r->utfresolution,
      symbol->sizex * style->scale / 2 / r->utfresolution,
      symbol->sizey * style->scale / 2 / r->utfresolution);
  path.concat_path(ellipse);

  /* Rotation if necessary. */
  if (style->rotation != 0) {
    mapserver::trans_affine mtx;
    mtx *= mapserver::trans_affine_translation(-x / r->utfresolution,
                                               -y / r->utfresolution);
    mtx *= mapserver::trans_affine_rotation(-style->rotation);
    mtx *= mapserver::trans_affine_translation(x / r->utfresolution,
                                               y / r->utfresolution);
    path.transform(mtx);
  }

  /* Rendering the symbol. */
  utfgridRenderPath(img, path);

  return MS_SUCCESS;
}

static int utfgridRenderGlyphs(imageObj *img, const textSymbolObj *ts,
                               colorObj * /*c*/, colorObj * /*oc*/, int /*ow*/,
                               int isMarker) {

  const textPathObj *tp = ts->textpath;
  const UTFGridRenderer *r = UTFGRID_RENDERER(img);

  /* If it's not a marker then it's a label or other thing and we don't
   *  want to draw it on the map
   */
  if (!isMarker) {
    return MS_SUCCESS; // Stop the rendering with no errors
  }

  /* utfvalue is set to 0 if the shape isn't in the table. */
  if (r->utfvalue == 0) {
    return MS_SUCCESS; // Stop the rendering with no errors
  }

  /* Pathing the symbol BBox */
  mapserver::path_storage box;
  double size, x, y;

  size = tp->glyph_size;
  ;
  x = tp->glyphs->pnt.x;
  y = tp->glyphs->pnt.y;

  box.move_to((x) / r->utfresolution, (y) / r->utfresolution);
  box.line_to((x + size) / r->utfresolution, (y) / r->utfresolution);
  box.line_to((x + size) / r->utfresolution, (y - size) / r->utfresolution);
  box.line_to((x) / r->utfresolution, (y - size) / r->utfresolution);

  /* Rotation if necessary. */
  if (tp->glyphs->rot != 0) {
    mapserver::trans_affine mtx;
    mtx *= mapserver::trans_affine_translation(-x / r->utfresolution,
                                               -y / r->utfresolution);
    mtx *= mapserver::trans_affine_rotation(-tp->glyphs->rot);
    mtx *= mapserver::trans_affine_translation(x / r->utfresolution,
                                               y / r->utfresolution);
    box.transform(mtx);
  }

  /* Rendering the symbol */
  utfgridRenderPath(img, box);

  return MS_SUCCESS;
}

static int utfgridFreeSymbol(symbolObj *) { return MS_SUCCESS; }

/*
 * Add the necessary functions for UTFGrid to the renderer VTable.
 */
int msPopulateRendererVTableUTFGrid(rendererVTableObj *renderer) {
  renderer->default_transform_mode = MS_TRANSFORM_SIMPLIFY;

  renderer->createImage = &utfgridCreateImage;
  renderer->freeImage = &utfgridFreeImage;
  renderer->saveImage = &utfgridSaveImage;

  renderer->startLayer = &utfgridStartLayer;
  renderer->endLayer = &utfgridEndLayer;

  renderer->startShape = &utfgridStartShape;
  renderer->endShape = &utfgridEndShape;

  renderer->renderPolygon = &utfgridRenderPolygon;
  renderer->renderGlyphs = &utfgridRenderGlyphs;
  renderer->renderLine = &utfgridRenderLine;
  renderer->renderVectorSymbol = &utfgridRenderVectorSymbol;
  renderer->renderPixmapSymbol = &utfgridRenderPixmapSymbol;
  renderer->renderEllipseSymbol = &utfgridRenderEllipseSymbol;

  renderer->freeSymbol = &utfgridFreeSymbol;

  renderer->loadImageFromFile = msLoadMSRasterBufferFromFile;

  return MS_SUCCESS;
}

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Headers for mapkml.cpp Google Earth KML output
 * Author:   David Kana and the MapServer team
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

#ifdef USE_KML

#include "mapkmlrenderer.h"

#ifdef __cplusplus
extern "C" {
#endif

KmlRenderer *getKmlRenderer(imageObj *img) {
  return (KmlRenderer *)img->img.plugin;
}

imageObj *msCreateImageKml(int width, int height, outputFormatObj *format,
                           colorObj *bg) {
  imageObj *image = NULL;

  image = (imageObj *)malloc(sizeof(imageObj));
  MS_CHECK_ALLOC(image, sizeof(imageObj), NULL);
  memset(image, 0, sizeof(imageObj));

  KmlRenderer *ren = new KmlRenderer(width, height, format, bg);
  image->img.plugin = (void *)ren;

  return image;
}

int msSaveImageKml(imageObj *img, mapObj * /*map*/, FILE *fp,
                   outputFormatObj *format) {
  KmlRenderer *renderer = getKmlRenderer(img);
  return renderer->saveImage(img, fp, format);
}

int msRenderLineKml(imageObj *img, shapeObj *p, strokeStyleObj *style) {
  KmlRenderer *renderer = getKmlRenderer(img);
  renderer->renderLine(img, p, style);
  return MS_SUCCESS;
}

int msRenderPolygonKml(imageObj *img, shapeObj *p, colorObj *color) {
  KmlRenderer *renderer = getKmlRenderer(img);
  renderer->renderPolygon(img, p, color);
  return MS_SUCCESS;
}

int msRenderPolygonTiledKml(imageObj * /*img*/, shapeObj * /*p*/,
                            imageObj * /*tile*/) {
  /*KmlRenderer* renderer = getKmlRenderer(img);*/
  return MS_SUCCESS;
}

int msRenderLineTiledKml(imageObj * /*img*/, shapeObj * /*p*/,
                         imageObj * /*tile*/) {
  return MS_SUCCESS;
}

int msRenderGlyphsKml(imageObj *img, const textSymbolObj *ts, colorObj *c,
                      colorObj *oc, int ow, int /*isMarker*/) {
  KmlRenderer *renderer = getKmlRenderer(img);
  renderer->renderGlyphs(img, ts, c, oc, ow);
  return MS_SUCCESS;
}

int msRenderVectorSymbolKml(imageObj *img, double x, double y,
                            symbolObj *symbol, symbolStyleObj *style) {
  KmlRenderer *renderer = getKmlRenderer(img);
  renderer->renderVectorSymbol(img, x, y, symbol, style);
  return MS_SUCCESS;
}

int msRenderPixmapSymbolKml(imageObj *img, double x, double y,
                            symbolObj *symbol, symbolStyleObj *style) {
  KmlRenderer *renderer = getKmlRenderer(img);
  renderer->renderPixmapSymbol(img, x, y, symbol, style);
  return MS_SUCCESS;
}

int msRenderEllipseSymbolKml(imageObj *image, double x, double y,
                             symbolObj *symbol, symbolStyleObj *style) {
  KmlRenderer *renderer = getKmlRenderer(image);
  renderer->renderEllipseSymbol(image, x, y, symbol, style);
  return MS_SUCCESS;
}

int msRenderTruetypeSymbolKml(imageObj *image, double x, double y,
                              symbolObj *symbol, symbolStyleObj *style) {
  KmlRenderer *renderer = getKmlRenderer(image);
  renderer->renderTruetypeSymbol(image, x, y, symbol, style);
  return MS_SUCCESS;
}

int msRenderTileKml(imageObj * /*img*/, imageObj * /*tile*/, double /*x*/,
                    double /*y*/) {
  return MS_SUCCESS;
}

int msGetRasterBufferKml(imageObj * /*img*/, rasterBufferObj * /*rb*/) {
  return MS_FAILURE; // not supported for kml
}

int msGetTruetypeTextBBoxKml(rendererVTableObj * /*r*/, char ** /*fonts*/,
                             int /*numfonts*/, double size, char *string,
                             rectObj *rect, double **advances,
                             int /*bAdjustBaseline*/) {
  rect->minx = 0.0;
  rect->maxx = 0.0;
  rect->miny = 0.0;
  rect->maxy = 0.0;
  if (advances) {
    int numglyphs = msGetNumGlyphs(string);
    *advances = (double *)msSmallMalloc(numglyphs * sizeof(double));
    for (int i = 0; i < numglyphs; i++) {
      (*advances)[i] = size;
    }
  }
  return MS_SUCCESS;
}

int msStartNewLayerKml(imageObj *img, mapObj * /*map*/, layerObj *layer) {
  KmlRenderer *renderer = getKmlRenderer(img);
  return renderer->startNewLayer(img, layer);
}

int msCloseNewLayerKml(imageObj *img, mapObj * /*map*/, layerObj *layer) {
  KmlRenderer *renderer = getKmlRenderer(img);
  return renderer->closeNewLayer(img, layer);
}

int msFreeImageKml(imageObj *image) {
  KmlRenderer *renderer = getKmlRenderer(image);
  if (renderer) {
    delete renderer;
  }
  image->img.plugin = NULL;
  return MS_SUCCESS;
}

int msFreeSymbolKml(symbolObj * /*symbol*/) { return MS_SUCCESS; }

int msStartShapeKml(imageObj *img, shapeObj *shape) {
  KmlRenderer *renderer = getKmlRenderer(img);
  renderer->startShape(img, shape);
  return MS_SUCCESS;
}

int msEndShapeKml(imageObj *img, shapeObj *shape) {
  KmlRenderer *renderer = getKmlRenderer(img);
  renderer->endShape(img, shape);
  return MS_SUCCESS;
}

int msMergeRasterBufferKml(imageObj *dest, rasterBufferObj *overlay,
                           double /*opacity*/, int /*srcX*/, int /*srcY*/,
                           int /*dstX*/, int /*dstY*/, int /*width*/,
                           int /*height*/) {
  KmlRenderer *renderer = getKmlRenderer(dest);
  return renderer->mergeRasterBuffer(dest, overlay);
}

#ifdef __cplusplus
}
#endif

#endif /*USE_KML*/

int aggInitializeRasterBuffer(rasterBufferObj *rb, int width, int height,
                              int mode);

int msPopulateRendererVTableKML(rendererVTableObj *renderer) {
#ifdef USE_KML
  renderer->supports_pixel_buffer = 0;
  renderer->supports_clipping = 0;
  renderer->use_imagecache = 0;
  renderer->default_transform_mode = MS_TRANSFORM_NONE;

  renderer->startLayer = msStartNewLayerKml;
  renderer->endLayer = msCloseNewLayerKml;
  renderer->renderLine = &msRenderLineKml;
  renderer->createImage = &msCreateImageKml;
  renderer->saveImage = &msSaveImageKml;
  renderer->renderPolygon = &msRenderPolygonKml;
  renderer->renderGlyphs = &msRenderGlyphsKml;
  renderer->renderEllipseSymbol = &msRenderEllipseSymbolKml;
  renderer->renderVectorSymbol = &msRenderVectorSymbolKml;
  renderer->renderPixmapSymbol = &msRenderPixmapSymbolKml;
  renderer->mergeRasterBuffer = &msMergeRasterBufferKml;
  renderer->loadImageFromFile = msLoadMSRasterBufferFromFile;
  renderer->initializeRasterBuffer = aggInitializeRasterBuffer;
  renderer->renderTile = &msRenderTileKml;
  renderer->renderPolygonTiled = &msRenderPolygonTiledKml;
  renderer->renderLineTiled = NULL;
  renderer->freeSymbol = &msFreeSymbolKml;
  renderer->freeImage = &msFreeImageKml;
  renderer->mergeRasterBuffer = msMergeRasterBufferKml;
  renderer->compositeRasterBuffer = NULL;

  renderer->startShape = &msStartShapeKml;
  renderer->endShape = &msEndShapeKml;

  return MS_SUCCESS;
#else
  msSetError(MS_MISCERR, "KML Driver requested but is not built in",
             "msPopulateRendererVTableKML()");
  return MS_FAILURE;
#endif
}

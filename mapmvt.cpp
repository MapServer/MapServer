/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapBox Vector Tile rendering.
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2015 Regents of the University of Minnesota.
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

#ifdef USE_PBF
#include "vector_tile.pb-c.h"
#endif

int msPopulateRendererVTableMVT(rendererVTableObj * renderer) {
#ifdef USE_PBF
  renderer->compositeRasterBuffer = NULL;
  renderer->supports_pixel_buffer = 0;
  renderer->use_imagecache = 0;
  renderer->supports_clipping = 0;
  renderer->supports_svg = 0;
  renderer->default_transform_mode = MS_TRANSFORM_SIMPLIFY;
  renderer->cleanup = mvtCleanup;
  renderer->renderLine = &mvtRenderLine;

  renderer->renderPolygon = &mvtRenderPolygon;
  renderer->renderPolygonTiled = &mvtRenderPolygon;
  renderer->renderLineTiled = &mvtRenderLineTiled;

  renderer->renderGlyphs = &mvtRenderGlyphsPath;

  renderer->renderVectorSymbol = &mvtRenderVectorSymbol;

  renderer->renderPixmapSymbol = &mvtRenderPixmapSymbol;

  renderer->renderEllipseSymbol = &mvtRenderEllipseSymbol;

  renderer->renderTile = &mvtRenderTile;

  renderer->getRasterBufferHandle = NULL;
  renderer->getRasterBufferCopy = NULL;
  renderer->initializeRasterBuffer = NULL;

  renderer->mergeRasterBuffer = NULL;
  renderer->loadImageFromFile = NULL;
  renderer->createImage = &mvtCreateImage;
  renderer->saveImage = &mvtSaveImage;

  renderer->startLayer = &mvtStartNewLayer;
  renderer->endLayer = &mvtCloseNewLayer;

  renderer->freeImage = &mvtFreeImage;
  renderer->freeSymbol = &mvtFreeSymbol;
  renderer->cleanup = mvtCleanup;

  return MS_SUCCESS;
#else
  msSetError(MS_MISCERR, "Vector Tile Driver requested but support is not compiled in",
             "msPopulateRendererVTableMVT()");
  return MS_FAILURE;
#endif
}

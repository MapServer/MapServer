/******************************************************************************
 * $id: mapogl.cpp 7725 2011-04-09 15:56:58Z toby $
 *
 * Project:  MapServer
 * Purpose:  Various template processing functions.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2008 Regents of the University of Minnesota.
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

#include "mapserver.h"

#ifdef USE_OGL
#include <stdio.h>
#include <assert.h>
#include "mapoglrenderer.h"
#include "mapoglcontext.h"



imageObj* createImageObjOgl(OglRenderer* renderer)
{
  if (!renderer->isValid()) return NULL;
  imageObj* pNewImage = (imageObj*)calloc(1, sizeof(imageObj));
  if (!pNewImage)
    return pNewImage;
  pNewImage->img.plugin = (void *) renderer;
  return pNewImage;
}

OglRenderer* getOglRenderer(imageObj* img)
{
  return (OglRenderer*) img->img.plugin;
}

int msSaveImageOgl(imageObj *img, mapObj *map, FILE *fp, outputFormatObj *format)
{
  rasterBufferObj data;
  OglRenderer* renderer = getOglRenderer(img);
  renderer->readRasterBuffer(&data);
  return msSaveRasterBuffer(NULL,&data,fp,img->format );
}

int msDrawLineOgl(imageObj *img, shapeObj *p, strokeStyleObj *style)
{
  OglRenderer* renderer = getOglRenderer(img);
  renderer->renderPolyline(p, style->color, style->width, style->patternlength, style->pattern);
  return MS_SUCCESS;
}

int msDrawPolygonOgl(imageObj *img, shapeObj *p, colorObj *color)
{
  OglRenderer* renderer = getOglRenderer(img);
  renderer->renderPolygon(p, color, NULL, 0);
  return MS_SUCCESS;
}

//void msFreeTileOgl(imageObj *tile)
//{
//    delete (OglCache*)tile;
//}

int msDrawLineTiledOgl(imageObj *img, shapeObj *p, imageObj *tile)
{
  OglRenderer* renderer = getOglRenderer(img);
  renderer->renderPolylineTile(p, getOglRenderer(tile)->getTexture());
  return MS_SUCCESS;
}

int msDrawPolygonTiledOgl(imageObj *img, shapeObj *p, imageObj *tile)
{
  OglRenderer* renderer = getOglRenderer(img);
  renderer->renderPolygon(p, NULL, NULL, 0.0, getOglRenderer(tile)->getTexture());
  return MS_SUCCESS;
}

int msRenderPixmapOgl(imageObj *img, double x, double y,
                      symbolObj *symbol, symbolStyleObj *style)
{
  OglRenderer* renderer = getOglRenderer(img);
  renderer->renderPixmap(symbol, x, y, style->rotation, style->scale);
  return MS_SUCCESS;
}

int msRenderVectorSymbolOgl
(imageObj *img, double x, double y,
 symbolObj *symbol, symbolStyleObj *style)
{
  OglRenderer* renderer = getOglRenderer(img);
  renderer->renderVectorSymbol(x, y, symbol, style->scale, style->rotation, style->color, style->outlinecolor, style->outlinewidth);
  return MS_SUCCESS;
}

int msRenderTruetypeSymbolOgl(imageObj *img, double x, double y,
                              symbolObj *symbol, symbolStyleObj * style)
{
  OglRenderer* renderer = getOglRenderer(img);
  double size = style->scale*72.0;
  renderer->renderGlyphs(0, 0, style->color, style->outlinecolor, size, symbol->font, symbol->character, style->rotation, NULL, 0, 0);
  return MS_SUCCESS;
}

int msRenderTileOgl(imageObj *img, imageObj *tile, double x, double y)
{
  OglRenderer* renderer = getOglRenderer(img);
  renderer->renderTile(getOglRenderer(tile)->getTexture(), x, y, 0.0);
  return MS_SUCCESS;
}

int msGetTruetypeTextBBoxOgl(rendererVTableObj *renderer, char **fonts, int numfonts, double size, char *string, rectObj *rect, double **advances, int bAdjustBaseline)
{
  if (OglRenderer::getStringBBox(fonts[0], size, string, rect, advances)) {
    return MS_SUCCESS;
  } else {
    return MS_FAILURE;
  }
}

int msRenderGlyphsOgl(imageObj *img, double x, double y,
                      labelStyleObj *style, char *text)
{
  OglRenderer* renderer = getOglRenderer(img);
  renderer->renderGlyphs(x, y, style->color, style->outlinecolor, style->size, style->fonts[0], text, style->rotation, NULL, 0.0, 0.0);
  return MS_SUCCESS;
}

int msMergeRasterBufferOgl(imageObj *dest, rasterBufferObj *overlay, double opacity, int srcX, int srcY, int dstX, int dstY, int width, int height)
{
  OglRenderer* renderer = getOglRenderer(dest);
  renderer->drawRasterBuffer(overlay, opacity, srcX, srcY, dstX, dstY, width, height);
  return MS_SUCCESS;
}

imageObj* msImageCreateOgl(int width, int height, outputFormatObj *format, colorObj* bg)
{
  imageObj *pNewImage = NULL;

  if (format->imagemode != MS_IMAGEMODE_RGB && format->imagemode
      != MS_IMAGEMODE_RGBA) {
    msSetError(
      MS_OGLERR, "OpenGL driver only supports RGB or RGBA pixel models.",
      "msImageCreateOGL()");
    return NULL;
  }
  return createImageObjOgl(new OglRenderer(width, height, bg));
}

int msRenderEllipseOgl(imageObj *image, double x, double y,
                       symbolObj *symbol, symbolStyleObj *style)
{

  OglRenderer* renderer = getOglRenderer(image);
  renderer->renderEllipse(x, y, style->rotation, symbol->sizex, symbol->sizey, style->color, style->outlinecolor, style->outlinewidth);
  return MS_SUCCESS;
}

int msFreeImageOgl(imageObj *img)
{
  OglRenderer* renderer = getOglRenderer(img);
  if (renderer) {
    delete renderer;
  }
  img->img.plugin=NULL;
  return MS_SUCCESS;
}

int msStartLayerOgl(imageObj *img, mapObj *map, layerObj *layer)
{
  getOglRenderer(img)->setTransparency((double)layer->opacity/100);
  return MS_SUCCESS;
}

int msEndLayerOgl(imageObj *img, mapObj *map, layerObj *layer)
{
  getOglRenderer(img)->setTransparency(1.0);
  return MS_SUCCESS;
}

int msFreeSymbolOgl(symbolObj *s)
{
  return MS_SUCCESS;
}

int msGetRasterBufferCopyOgl(imageObj *img, rasterBufferObj *rb)
{
  getOglRenderer(img)->readRasterBuffer(rb);
  return MS_SUCCESS;
}

int msGetRasterBufferHandleOgl(imageObj *img, rasterBufferObj * rb)
{
  getOglRenderer(img)->readRasterBuffer(rb);
  return MS_SUCCESS;
}

int msInitializeRasterBufferOgl(rasterBufferObj *rb, int width, int height, int mode)
{
  OglRenderer::initializeRasterBuffer(rb, width, height, mode==MS_IMAGEMODE_RGBA);
  return MS_SUCCESS;
}

#endif /* USE_OGL */

int msPopulateRendererVTableOGL(rendererVTableObj *renderer)
{
#ifdef USE_OGL
  renderer->supports_transparent_layers = 1;
  renderer->supports_pixel_buffer = 1;
  renderer->supports_clipping = 0;
  renderer->use_imagecache = 0;
  renderer->supports_bitmap_fonts = 0;
  renderer->default_transform_mode = MS_TRANSFORM_SIMPLIFY;
  renderer->startLayer = msStartLayerOgl;
  renderer->endLayer = msEndLayerOgl;

  renderer->createImage=&msImageCreateOgl;
  renderer->saveImage=&msSaveImageOgl;


  renderer->renderLine=&msDrawLineOgl;
  renderer->renderPolygon=&msDrawPolygonOgl;
  renderer->renderGlyphs=&msRenderGlyphsOgl;
  renderer->renderEllipseSymbol = &msRenderEllipseOgl;
  renderer->renderVectorSymbol = &msRenderVectorSymbolOgl;
  renderer->renderPixmapSymbol = &msRenderPixmapOgl;
  renderer->renderTruetypeSymbol = &msRenderTruetypeSymbolOgl;

  renderer->renderTile = &msRenderTileOgl;
  renderer->renderPolygonTiled = &msDrawPolygonTiledOgl;
  renderer->renderLineTiled = &msDrawLineTiledOgl;

  renderer->getTruetypeTextBBox = &msGetTruetypeTextBBoxOgl;

  renderer->getRasterBufferHandle = msGetRasterBufferHandleOgl;
  renderer->getRasterBufferCopy = msGetRasterBufferCopyOgl;
  renderer->initializeRasterBuffer = msInitializeRasterBufferOgl;
  renderer->mergeRasterBuffer = msMergeRasterBufferOgl;
  renderer->loadImageFromFile = msLoadMSRasterBufferFromFile;

  renderer->freeSymbol = &msFreeSymbolOgl;
  renderer->freeImage=&msFreeImageOgl;

  return MS_SUCCESS;
#else
  msSetError(MS_MISCERR,"OGL driver requested but it is not compiled in this release",
             "msPopulateRendererVTableOGL()");
  return MS_FAILURE;
#endif

}


/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Dummy functions to implement when using a pluggable renderer
 * Author:   Thomas Bonfort, thomas.bonfort@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 2010, Thomas Bonfort
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

int renderLineDummy(imageObj *img, shapeObj *p, strokeStyleObj *style)
{
  (void)img;
  (void)p;
  (void)style;
  msSetError(MS_RENDERERERR,"renderLine not implemented","renderLine()");
  return MS_FAILURE;
}

int renderPolygonDummy(imageObj *img, shapeObj *p, colorObj *color)
{
  (void)img;
  (void)p;
  (void)color;
  msSetError(MS_RENDERERERR,"renderPolygon not implemented","renderPolygon()");
  return MS_FAILURE;
}

int renderPolygonTiledDummy(imageObj *img, shapeObj *p, imageObj *tile)
{
  (void)img;
  (void)p;
  (void)tile;
  msSetError(MS_RENDERERERR,"renderPolygonTiled not implemented","renderPolygonTiled()");
  return MS_FAILURE;
}

int renderLineTiledDummy(imageObj *img, shapeObj *p, imageObj *tile)
{
  (void)img;
  (void)p;
  (void)tile;
  msSetError(MS_RENDERERERR,"renderLineTiled not implemented","renderLineTiled()");
  return MS_FAILURE;
}

int renderRasterGlyphsDummy(imageObj *img, double x, double y, int fontIndex,
                            colorObj *color, char* text)
{
  (void)img;
  (void)x;
  (void)y;
  (void)fontIndex;
  (void)color;
  (void)text;
  msSetError(MS_RENDERERERR,"renderRasterGlyphs not implemented","renderRasterGlyphs()");
  return MS_FAILURE;
}

int renderGlyphsLineDummy(imageObj *img,labelPathObj *labelpath,
                          labelStyleObj *style, char *text)
{
  (void)img;
  (void)labelpath;
  (void)style;
  (void)text;
  msSetError(MS_RENDERERERR,"renderGlyphsLine not implemented","renderGlyphsLine()");
  return MS_FAILURE;
}

int renderVectorSymbolDummy(imageObj *img, double x, double y,
                            symbolObj *symbol, symbolStyleObj *style)
{
  (void)img;
  (void)x;
  (void)y;
  (void)symbol;
  (void)style;
  msSetError(MS_RENDERERERR,"renderVectorSymbol not implemented","renderVectorSymbol()");
  return MS_FAILURE;
}

void* createVectorSymbolTileDummy(int width, int height,
                                  symbolObj *symbol, symbolStyleObj *style)
{
  (void)width;
  (void)height;
  (void)symbol;
  (void)style;
  msSetError(MS_RENDERERERR,"createVectorSymbolTile not implemented","createVectorSymbolTile()");
  return NULL;
}

int renderPixmapSymbolDummy(imageObj *img, double x, double y,
                            symbolObj *symbol, symbolStyleObj *style)
{
  (void)img;
  (void)x;
  (void)y;
  (void)symbol;
  (void)style;
  msSetError(MS_RENDERERERR,"renderPixmapSymbol not implemented","renderPixmapSymbol()");
  return MS_FAILURE;
}

void* createPixmapSymbolTileDummy(int width, int height,
                                  symbolObj *symbol, symbolStyleObj *style)
{
  (void)width;
  (void)height;
  (void)symbol;
  (void)style;
  msSetError(MS_RENDERERERR,"createPixmapSymbolTile not implemented","createPixmapSymbolTile()");
  return NULL;
}

int renderEllipseSymbolDummy(imageObj *image, double x, double y,
                             symbolObj *symbol, symbolStyleObj *style)
{
  (void)image;
  (void)x;
  (void)y;
  (void)symbol;
  (void)style;
  msSetError(MS_RENDERERERR,"renderEllipseSymbol not implemented","renderEllipseSymbol()");
  return MS_FAILURE;
}

void* createEllipseSymbolTileDummy(int width, int height,
                                   symbolObj *symbol, symbolStyleObj *style)
{
  (void)width;
  (void)height;
  (void)symbol;
  (void)style;
  msSetError(MS_RENDERERERR,"createEllipseSymbolTile not implemented","createEllipseSymbolTile()");
  return NULL;
}

int renderTruetypeSymbolDummy(imageObj *img, double x, double y,
                              symbolObj *symbol, symbolStyleObj *style)
{
  (void)img;
  (void)x;
  (void)y;
  (void)symbol;
  (void)style;
  msSetError(MS_RENDERERERR,"renderTruetypeSymbol not implemented","renderTruetypeSymbol()");
  return MS_FAILURE;
}

void* createTruetypeSymbolTileDummy(int width, int height,
                                    symbolObj *symbol, symbolStyleObj *style)
{
  (void)width;
  (void)height;
  (void)symbol;
  (void)style;
  msSetError(MS_RENDERERERR,"createTruetypeSymbolTile not implemented","createTruetypeSymbolTile()");
  return NULL;
}

int renderTileDummy(imageObj *img, imageObj *tile, double x, double y)
{
  (void)img;
  (void)tile;
  (void)x;
  (void)y;
  msSetError(MS_RENDERERERR,"renderTile not implemented","renderTile()");
  return MS_FAILURE;
}

rasterBufferObj* loadImageFromFileDummy(char *path)
{
  (void)path;
  msSetError(MS_RENDERERERR,"loadImageFromFile not implemented","loadImageFromFile()");
  return NULL;
}


int getRasterBufferHandleDummy(imageObj *img, rasterBufferObj *rb)
{
  (void)img;
  (void)rb;
  msSetError(MS_RENDERERERR,"getRasterBufferHandle not implemented","getRasterBufferHandle()");
  return MS_FAILURE;
}

int getRasterBufferCopyDummy(imageObj *img, rasterBufferObj *rb)
{
  (void)img;
  (void)rb;
  msSetError(MS_RENDERERERR,"getRasterBufferCopy not implemented","getRasterBufferCopy()");
  return MS_FAILURE;
}

rasterBufferObj* createRasterBufferDummy(int width, int height)
{
  (void)width;
  (void)height;
  msSetError(MS_RENDERERERR,"createRasterBuffer not implemented","createRasterBuffer()");
  return NULL;
}

int mergeRasterBufferDummy(imageObj *dest, rasterBufferObj *overlay, double opacity, int srcX, int srcY, int dstX, int dstY, int width, int height)
{
  (void)dest;
  (void)overlay;
  (void)opacity;
  (void)srcX;
  (void)srcY;
  (void)dstX;
  (void)dstY;
  (void)width;
  (void)height;
  msSetError(MS_RENDERERERR,"mergeRasterBuffer not implemented","mergeRasterBuffer()");
  return MS_FAILURE;
}

int initializeRasterBufferDummy(rasterBufferObj *rb, int width, int height, int mode)
{
  (void)rb;
  (void)width;
  (void)height;
  (void)mode;
  msSetError(MS_RENDERERERR,"initializeRasterBuffer not implemented","initializeRasterBuffer()");
  return MS_FAILURE;
}



/* image i/o */
imageObj* createImageDummy(int width, int height, outputFormatObj *format, colorObj* bg)
{
  (void)width;
  (void)height;
  (void)format;
  (void)bg;
  msSetError(MS_RENDERERERR,"createImage not implemented","createImage()");
  return NULL;
}

int saveImageDummy(imageObj *img, mapObj *map, FILE *fp, outputFormatObj *format)
{
  (void)img;
  (void)map;
  (void)fp;
  (void)format;
  msSetError(MS_RENDERERERR,"saveImage not implemented","saveImage()");
  return MS_FAILURE;
}
/*...*/

/* helper functions */
int getTruetypeTextBBoxDummy(rendererVTableObj *renderer, char **fonts, int numfonts, double size,
                             char *string,rectObj *rect, double **advances, int bAdjustBaseline)
{
  (void)renderer;
  (void)fonts;
  (void)numfonts;
  (void)size;
  (void)string;
  (void)rect;
  (void)advances;
  (void)bAdjustBaseline;
  msSetError(MS_RENDERERERR,"getTruetypeTextBBox not implemented","getTruetypeTextBBox()");
  return MS_FAILURE;
}

int startLayerDummy(imageObj *img, mapObj *map, layerObj *layer)
{
  (void)img;
  (void)map;
  (void)layer;
  msSetError(MS_RENDERERERR,"startLayer not implemented","startLayer()");
  return MS_FAILURE;
}

int endLayerDummy(imageObj *img, mapObj *map, layerObj *layer)
{
  (void)img;
  (void)map;
  (void)layer;
  msSetError(MS_RENDERERERR,"endLayer not implemented","endLayer()");
  return MS_FAILURE;
}

int startShapeDummy(imageObj *img, shapeObj *shape)
{
  (void)img;
  (void)shape;
  msSetError(MS_RENDERERERR,"startShape not implemented","startShape()");
  return MS_FAILURE;
}

int endShapeDummy(imageObj *img, shapeObj *shape)
{
  (void)img;
  (void)shape;
  msSetError(MS_RENDERERERR,"endShape not implemented","endShape()");
  return MS_FAILURE;
}

int setClipDummy(imageObj *img, rectObj clipRect)
{
  (void)img;
  (void)clipRect;
  msSetError(MS_RENDERERERR,"setClip not implemented","setClip()");
  return MS_FAILURE;
}
int resetClipDummy(imageObj *img)
{
  (void)img;
  msSetError(MS_RENDERERERR,"resetClip not implemented","resetClip()");
  return MS_FAILURE;
}

int freeImageDummy(imageObj *image)
{
  (void)image;
  msSetError(MS_RENDERERERR,"freeImage not implemented","freeImage()");
  return MS_FAILURE;
}

int freeTileDummy(imageObj *tile)
{
  (void)tile;
  msSetError(MS_RENDERERERR,"freeTile not implemented","freeTile()");
  return MS_FAILURE;
}

int freeSymbolDummy(symbolObj *symbol)
{
  (void)symbol;
  msSetError(MS_RENDERERERR,"freeSymbol not implemented","freeSymbol()");
  return MS_FAILURE;
}

int cleanupDummy(void *renderer_data)
{
  (void)renderer_data;
  return MS_SUCCESS;
}

int msInitializeDummyRenderer(rendererVTableObj *renderer)
{

  renderer->use_imagecache = 0;
  renderer->supports_pixel_buffer = 0;
  renderer->compositeRasterBuffer = NULL;
  renderer->supports_clipping = 0;
  renderer->supports_svg = 0;
  renderer->renderer_data = NULL;
  renderer->transform_mode = MS_TRANSFORM_SIMPLIFY;
  renderer->startLayer = &startLayerDummy;
  renderer->endLayer = &endLayerDummy;
  renderer->renderLine=&renderLineDummy;
  renderer->renderLineTiled = NULL;
  renderer->createImage=&createImageDummy;
  renderer->saveImage=&saveImageDummy;
  renderer->getRasterBufferHandle=&getRasterBufferHandleDummy;
  renderer->getRasterBufferCopy=getRasterBufferCopyDummy;
  renderer->initializeRasterBuffer=initializeRasterBufferDummy;
  renderer->renderPolygon=&renderPolygonDummy;
  renderer->freeImage=&freeImageDummy;
  renderer->renderEllipseSymbol = &renderEllipseSymbolDummy;
  renderer->renderVectorSymbol = &renderVectorSymbolDummy;
  renderer->renderPixmapSymbol = &renderPixmapSymbolDummy;
  renderer->mergeRasterBuffer = &mergeRasterBufferDummy;
  renderer->renderTile = &renderTileDummy;
  renderer->renderPolygonTiled = &renderPolygonTiledDummy;
  renderer->freeSymbol = &freeSymbolDummy;
  renderer->cleanup = &cleanupDummy;
  renderer->startShape = NULL;
  renderer->endShape = NULL;
  renderer->renderGlyphs = NULL;
  return MS_SUCCESS;
}



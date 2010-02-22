/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  GD rendering functions (using renderer plugin API)
 * Author:   Stephen Lime, Thomas Bonfort
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
#include "mapthread.h"
#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

void startNewLayerGD(imageObj *img, double opacity) {
}

void closeNewLayerGD(imageObj *img, double opacity) { 
}

/*
** GD driver-specific image handling functions.
*/
imageObj *createImageGD(int width, int height, outputFormatObj *format, colorObj* bg) 
{
  imageObj *img = NULL;
  gdImagePtr ip;

  img = (imageObj *) calloc(1, sizeof (imageObj));

  /* we're only doing PC256 for the moment */
  ip = gdImageCreate(width, height);
  gdImageColorAllocate(ip, bg->red, bg->green, bg->blue); /* set the background color */

  img->img.plugin = (void *) ip;
  return img;
}

int saveImageGD(imageObj *img, FILE *fp, outputFormatObj *format) 
{
  gdImagePtr ip;

  if(!img || !fp) return NULL;
  ip = (gdImagePtr) img->img.plugin;

  if(strcasecmp("ON", msGetOutputFormatOption(format, "INTERLACE", "ON")) == 0)
    gdImageInterlace(ip, 1);

  if(format->transparent)
    gdImageColorTransparent(ip, 0);

  if(strcasecmp(format->driver, "gd2/gif") == 0)
    gdImageGif(ip, fp);
  else if(strcasecmp(format->driver, "gd2/png") == 0)
    gdImagePng(ip, fp);
  else if(strcasecmp(format->driver, "gd2/jpeg") == 0)
    gdImageJpeg(ip, fp, atoi(msGetOutputFormatOption( format, "QUALITY", "75")));
  else
    return MS_FAILURE; /* unsupported format */
  
  return MS_SUCCESS;
}

void freeImageGD(imageObj *img) 
{
  if(img->img.plugin) gdImageDestroy((gdImagePtr)img->img.plugin);
}

/*
** GD driver-specific rendering functions.
*/
void renderLineGD(imageObj *img, shapeObj *p, strokeStyleObj *stroke) {
}

void renderPolygonGD(imageObj *img, shapeObj *p, colorObj *c) {
}

void renderGlyphsLineGD(imageObj *img, labelPathObj *labelpath, labelStyleObj *style, char *text) {
}

void renderGlyphsGD(imageObj *img,double x, double y, labelStyleObj *style, char *text) {
}

void renderEllipseSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style) {
}

void renderVectorSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style) {
}

void renderTruetypeSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *s) {
}

void renderPixmapSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style) {
}

void renderTileGD(imageObj *img, imageObj *tile, double x, double y) {
}

void renderPolygonTiledGD(imageObj *img, shapeObj *p,  imageObj *tile) {
}

/*
** GD driver-specific raster buffer functions.
*/
void getRasterBufferGD(imageObj *img, rasterBufferObj *rb) {
}

void mergeRasterBufferGD(imageObj *img, rasterBufferObj *rb, double opacity, int dstX, int dstY) {
}

int getTruetypeTextBBoxGD(imageObj *img,char *font, double size, char *text, rectObj *rect, double **advances) {
  return MS_SUCCESS;
}

/*
** Misc. cleanup functions for GD driver.
*/
void freeTileGD(imageObj *tile) {
  freeImageGD(tile);
}

void freeSymbolGD(symbolObj *s) {
}

inline int populateRendererVTableGD( rendererVTableObj *renderer ) {
  renderer->supports_imagecache=0;
  renderer->supports_pixel_buffer=1;
  renderer->supports_transparent_layers = 0;
  renderer->startNewLayer = startNewLayerGD;
  renderer->closeNewLayer = closeNewLayerGD;
  renderer->renderLine = &renderLineGD;
  renderer->createImage = &createImageGD;
  renderer->saveImage = &saveImageGD;
  renderer->getRasterBuffer = &getRasterBufferGD;
  renderer->transformShape = &msTransformShapeToPixel;
  renderer->renderPolygon = &renderPolygonGD;
  renderer->renderGlyphsLine = &renderGlyphsLineGD;
  renderer->renderGlyphs = &renderGlyphsGD;
  renderer->freeImage = &freeImageGD;
  renderer->renderEllipseSymbol = &renderEllipseSymbolGD;
  renderer->renderVectorSymbol = &renderVectorSymbolGD;
  renderer->renderTruetypeSymbol = &renderTruetypeSymbolGD;
  renderer->renderPixmapSymbol = &renderPixmapSymbolGD;
  renderer->mergeRasterBuffer = &mergeRasterBufferGD;
  renderer->getTruetypeTextBBox = &getTruetypeTextBBoxGD;
  renderer->renderTile = &renderTileGD;
  renderer->renderPolygonTiled = &renderPolygonTiledGD;
  renderer->freeTile = &freeTileGD;
  renderer->freeSymbol = &freeSymbolGD;
  return MS_SUCCESS;
}

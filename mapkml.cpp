/******************************************************************************
 * 
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

#include "mapkml.h"

#ifdef __cplusplus
extern "C" {
#endif

KmlRenderer* getKmlRenderer(imageObj* img)
{
	return (KmlRenderer*) img->img.plugin;
}

imageObj* msCreateImageKml(int width, int height, outputFormatObj *format, colorObj* bg)
{
	imageObj *image = NULL;

	image = (imageObj*)malloc(sizeof(imageObj));
	memset(image, 0, sizeof(imageObj));

	KmlRenderer *ren = new KmlRenderer(width, height, bg);
	image->img.plugin = (void *) ren;

	return image;
}

int msSaveImageKml(imageObj *img, FILE *fp, outputFormatObj *format)
{
	KmlRenderer* renderer = getKmlRenderer(img);
	return renderer->saveImage(img, fp, format);
}

void msRenderLineKml(imageObj *img, shapeObj *p, strokeStyleObj *style)
{
	KmlRenderer* renderer = getKmlRenderer(img);
	renderer->renderLine(img, p, style);
}

void msRenderPolygonKml(imageObj *img, shapeObj *p, colorObj *color)
{
	KmlRenderer* renderer = getKmlRenderer(img);
	renderer->renderPolygon(img, p, color);
}

void msRenderPolygonTiledKml(imageObj *img, shapeObj *p,  imageObj *tile)
{
  /*KmlRenderer* renderer = getKmlRenderer(img);*/
}

void msRenderLineTiledKml(imageObj *img, shapeObj *p, imageObj *tile)
{
}

void msRenderGlyphsKml(imageObj *img, double x, double y,
            labelStyleObj *style, char *text)
{
	KmlRenderer* renderer = getKmlRenderer(img);
	renderer->renderGlyphs(img, x, y, style, text);
}

void msRenderGlyphsLineKml(imageObj *img,labelPathObj *labelpath,
      labelStyleObj *style, char *text)
{
}

void msRenderVectorSymbolKml(imageObj *img, double x, double y,
		symbolObj *symbol, symbolStyleObj *style)
{
    	KmlRenderer* renderer = getKmlRenderer(img);
	renderer->renderVectorSymbol(img, x, y, symbol, style);
}

void* msCreateVectorSymbolTileKml(int width, int height,
        symbolObj *symbol, symbolStyleObj *style)
{
	return NULL;
}

void msRenderPixmapSymbolKml(imageObj *img, double x, double y,
    	symbolObj *symbol, symbolStyleObj *style)
{
	KmlRenderer* renderer = getKmlRenderer(img);
	renderer->renderPixmapSymbol(img, x, y, symbol, style);
}

void* msCreatePixmapSymbolTileKml(int width, int height,
        symbolObj *symbol, symbolStyleObj *style)
{
	return NULL;
}

void msRenderEllipseSymbolKml(imageObj *image, double x, double y, 
		symbolObj *symbol, symbolStyleObj *style)
{
    KmlRenderer* renderer = getKmlRenderer(image);
    renderer->renderEllipseSymbol(image, x, y, symbol,
                                  style);
}

void* msCreateEllipseSymbolTileKml(int width, int height,
        symbolObj *symbol, symbolStyleObj *style)
{
	return NULL;
}

void msRenderTruetypeSymbolKml(imageObj *img, double x, double y,
        symbolObj *symbol, symbolStyleObj *style)
{
}

void* msCreateTruetypeSymbolTileKml(int width, int height,
        symbolObj *symbol, symbolStyleObj *style)
{
	return NULL;
}

void msRenderTileKml(imageObj *img, imageObj *tile, double x, double y)
{
}

void msGetRasterBufferKml(imageObj *img,rasterBufferObj *rb)
{
}

void msMergeRasterBufferKml(imageObj *dest, rasterBufferObj *overlay, double opacity, int dstX, int dstY)
{
}

int msGetTruetypeTextBBoxKml(imageObj *img,char *font, double size, char *string,
		rectObj *rect, double **advances)
{
	KmlRenderer* renderer = getKmlRenderer(img);
	return renderer->getTruetypeTextBBox(img, font, size, string, rect, advances);
}

void msStartNewLayerKml(imageObj *img, layerObj *layer, double opacity)
{
	KmlRenderer* renderer = getKmlRenderer(img);
	renderer->startNewLayer(img, layer);
}

void msCloseNewLayerKml(imageObj *img, layerObj *layer, double opacity)
{
	KmlRenderer* renderer = getKmlRenderer(img);
	renderer->closeNewLayer(img, layer);
}

void msTransformShapeKml(shapeObj *shape, rectObj extend, double cellsize)
{
}

void msFreeImageKml(imageObj *image)
{
	KmlRenderer* renderer = getKmlRenderer(image);
	if (renderer)
	{
		delete renderer;
	}
	image->img.plugin=NULL;
}

void msFreeTileKml(imageObj *tile)
{
}

void msFreeSymbolKml(symbolObj *symbol)
{
}

void msStartShapeKml(imageObj *img, shapeObj *shape)
{
	KmlRenderer* renderer = getKmlRenderer(img);
	renderer->startShape(img, shape);
}

void msEndShapeKml(imageObj *img, shapeObj *shape)
{
    KmlRenderer* renderer = getKmlRenderer(img);
    renderer->endShape(img, shape);
}

int  msDrawRasterLayerKml(mapObj *map, layerObj *layer, imageObj *img)
{
    KmlRenderer* renderer = getKmlRenderer(img);
    return renderer->renderRasterLayer(img);
    
}
#ifdef __cplusplus
}
#endif


#endif /*USE_KML*/

int msPopulateRendererVTableKML( rendererVTableObj *renderer )
{
#ifdef USE_KML

	renderer->supports_transparent_layers = 1;
	renderer->supports_pixel_buffer = 0;
	renderer->supports_imagecache = 0;

	renderer->startNewLayer = msStartNewLayerKml;
	renderer->closeNewLayer = msCloseNewLayerKml;
    renderer->renderLine=&msRenderLineKml;
    renderer->createImage=&msCreateImageKml;
    renderer->saveImage=&msSaveImageKml;
    renderer->transformShape=&msTransformShapeKml;
    renderer->renderPolygon=&msRenderPolygonKml;
    renderer->renderGlyphs=&msRenderGlyphsKml;
	renderer->renderGlyphsLine = &msRenderGlyphsLineKml;
    renderer->renderEllipseSymbol = &msRenderEllipseSymbolKml;
    renderer->renderVectorSymbol = &msRenderVectorSymbolKml;
    renderer->renderPixmapSymbol = &msRenderPixmapSymbolKml;
	renderer->renderTruetypeSymbol = &msRenderTruetypeSymbolKml;
	renderer->getRasterBuffer = &msGetRasterBufferKml;
    renderer->mergeRasterBuffer = &msMergeRasterBufferKml;
    renderer->getTruetypeTextBBox = &msGetTruetypeTextBBoxKml;
    renderer->createPixmapSymbolTile = &msCreatePixmapSymbolTileKml;
    renderer->createVectorSymbolTile = &msCreateVectorSymbolTileKml;
    renderer->createEllipseSymbolTile = &msCreateEllipseSymbolTileKml;
    renderer->createTruetypeSymbolTile = &msCreateTruetypeSymbolTileKml;
    renderer->renderTile = &msRenderTileKml;
    renderer->renderPolygonTiled = &msRenderPolygonTiledKml;
    renderer->renderLineTiled = &msRenderLineTiledKml;
    renderer->freeTile = &msFreeTileKml;
    renderer->freeSymbol = &msFreeSymbolKml;
    renderer->freeImage=&msFreeImageKml;

	renderer->startShape=&msStartShapeKml;
	renderer->endShape=&msEndShapeKml;

    return MS_SUCCESS;
#else
    msSetError(MS_MISCERR, "KML Driver requested but is not built in", 
            "msPopulateRendererVTableKML()");
    return MS_FAILURE;
#endif
}


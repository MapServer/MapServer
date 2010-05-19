#ifdef USE_KML

#ifndef MAPKML_H
#define MAPKML_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mapserver.h"

imageObj* msCreateImageKml(int width, int height, outputFormatObj *format, colorObj* bg);
int msSaveImageKml(imageObj *img, FILE *fp, outputFormatObj *format);
void msRenderLineKml(imageObj *img, shapeObj *p, strokeStyleObj *style);
void msRenderPolygonKml(imageObj *img, shapeObj *p, colorObj *color);
void msRenderPolygonTiledKml(imageObj *img, shapeObj *p,  imageObj *tile);
void msRenderLineTiledKml(imageObj *img, shapeObj *p, imageObj *tile);
void msRenderGlyphsKml(imageObj *img, double x, double y,
            labelStyleObj *style, char *text);

void msRenderGlyphsLineKml(imageObj *img,labelPathObj *labelpath,
      labelStyleObj *style, char *text);

void msRenderVectorSymbolKml(imageObj *img, double x, double y,
		symbolObj *symbol, symbolStyleObj *style);

void* msCreateVectorSymbolTileKml(int width, int height,
        symbolObj *symbol, symbolStyleObj *style);

void msRenderPixmapSymbolKml(imageObj *img, double x, double y,
    	symbolObj *symbol, symbolStyleObj *style);

void* msCreatePixmapSymbolTileKml(int width, int height,
        symbolObj *symbol, symbolStyleObj *style);

void msRenderEllipseSymbolKml(imageObj *image, double x, double y, 
		symbolObj *symbol, symbolStyleObj *style);

void* msCreateEllipseSymbolTileKml(int width, int height,
        symbolObj *symbol, symbolStyleObj *style);

void msRenderTruetypeSymbolKml(imageObj *img, double x, double y,
        symbolObj *symbol, symbolStyleObj *style);

void* msCreateTruetypeSymbolTileKml(int width, int height,
        symbolObj *symbol, symbolStyleObj *style);

void msRenderTileKml(imageObj *img, imageObj *tile, double x, double y);
void msGetRasterBufferKml(imageObj *img,rasterBufferObj *rb);

void msMergeRasterBufferKml(imageObj *dest, rasterBufferObj *overlay, double opacity, int dstX, int dstY);
int msGetTruetypeTextBBoxKml(imageObj *img,char *font, double size, char *string,
		rectObj *rect, double **advances);

  void msStartNewLayerKml(imageObj *img, layerObj *layer, double opacity);
  void msCloseNewLayerKml(imageObj *img, layerObj *layer, double opacity);

void msStartShapeKml(imageObj *img, shapeObj *shape);
void msEndShapeKml(imageObj *img, shapeObj *shape);

void msTransformShapeKml(shapeObj *shape, rectObj extend, double cellsize);
void msFreeImageKml(imageObj *image);
void msFreeTileKml(imageObj *tile);
void msFreeSymbolKml(symbolObj *symbol);

int  msDrawRasterLayerKml(mapObj *map, layerObj *layer, imageObj *img);

#ifdef __cplusplus
}
#endif

#endif /* MAPKML_H */

#endif /*USE_KML*/

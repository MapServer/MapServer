/*
 * mapogl.c
 *
 *  Created on: 12/01/2009
 *      Author: toby
 */

#include "mapserver.h"

#ifdef USE_OGL
#include <assert.h>
#include "mapoglrenderer.h"
#include "mapoglcontext.h"

OglRenderer* getOglRenderer(imageObj* img)
{
	return (OglRenderer*) img->img.plugin;
}

void msDrawLineOgl(imageObj *img, shapeObj *p, colorObj *c, double width,
		int patternlength, double* pattern)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->renderPolyline(p, c, width, patternlength, pattern);
}

void msDrawPolygonOgl(imageObj *img, shapeObj *p, colorObj *c, colorObj *oc,
		double outlineWidth)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->renderPolygon(p, c, oc, outlineWidth);
}

void msFreeTileOgl(void *tile)
{
	delete (OglCache*)tile;
}

void msDrawLineTiledOgl(imageObj *img, shapeObj *p, void* tile)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->renderPolylineTile(p, (OglCache*)tile);}

void msDrawPolygonTiledOgl(imageObj *img, shapeObj *p, colorObj *oc, double outlineWidth, void *tile)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->renderPolygon(p, NULL, oc, outlineWidth, (OglCache*)tile);
}

void msRenderPixmapOgl(imageObj *img, double x, double y, symbolObj *symbol,
		double scale, double angle)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->renderPixmap(symbol, x, y, angle, scale);
}

void msRenderVectorSymbolOgl(imageObj *img, double x, double y,
		symbolObj *symbol, double scale, double angle, colorObj *c,
		colorObj *oc, double ow)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->renderVectorSymbol(x, y, symbol, scale, angle, c, oc, ow);
}

void* msCreateTilePixmapOgl(symbolObj *symbol, double scale, double angle, colorObj *bc)
{
	double canvasWidth = symbol->sizex*scale;
	double canvasHeight = symbol->sizey*scale;
	OglTexture* renderer = new OglTexture(canvasWidth, canvasHeight, bc);
	renderer->renderPixmap(symbol, 0, 0, angle, scale);
	OglCache* tile = renderer->renderToTile();
	delete renderer;
	return tile;
}

void *msCreateTileEllipseOgl(double width, double height, double angle,
		colorObj *c, colorObj *bc, colorObj *oc, double ow)
{
	double canvasWidth = angle == 0.0 ? width : MS_MAX(height, width);
	double canvasHeight = angle == 0.0 ? height : MS_MAX(height, width);
	OglTexture* renderer = new OglTexture(canvasWidth, canvasHeight, bc);
	renderer->renderEllipse(ceil(canvasWidth/2), ceil(canvasHeight/2), angle, width, height, c, NULL, ow);
	OglCache* tile = renderer->renderToTile();
	delete renderer;
	return tile;
}

void* msCreateTileVectorOgl(symbolObj *symbol, double scale, double angle,
		colorObj *c, colorObj *bc, colorObj *oc, double ow)
{
	OglTexture* renderer = new OglTexture(symbol->sizex*scale, symbol->sizey*scale, bc);
	renderer->renderVectorSymbol(0,0, symbol, scale, angle, c, bc, ow);
	OglCache* tile = renderer->renderToTile();
	delete renderer;
	return tile;
}

void* msCreateTileTruetypeOgl(imageObj *img, char *text, char *font,
		double size, double angle, colorObj *c, colorObj *bc, colorObj *oc,
		double ow)
{
	OglTexture* renderer = new OglTexture(size, size, bc);
	renderer->renderGlyphs(0, 0, c, oc, size, font, text, angle, NULL, 0, 0);
	OglCache* tile = renderer->renderToTile();
	delete renderer;
	return tile;
}

void msRenderTileOgl(imageObj *img, void *tile, double x, double y, double angle)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->renderTile((OglCache*)tile, x, y, angle);
}

int msGetTruetypeTextBBoxOgl(imageObj *img, char *font, double size, char *string, rectObj *rect, double **advances)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->getStringBBox(font, size, string, rect, advances);
	return 0;
}

void msRenderGlyphsOgl(imageObj *img, double x, double y, colorObj *c,
		colorObj *outlinecolor, double size, char *font, char *thechars,
		double angle, colorObj *shadowcolor, double shdx, double shdy,
		int outlinewidth)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->renderGlyphs(x, y, c, outlinecolor, size, font, thechars, angle, shadowcolor, shdx, shdy);
}

void msMergeImagesOgl(imageObj *dest, imageObj *overlay, int opacity, int dstX,
		int dstY)
{
	//	cairo_renderer *rd = getCairoRenderer(dest);
	//	cairo_renderer *ro = getCairoRenderer(overlay);
	//	cairo_set_source_surface (rd->cr,ro->surface,dstX,dstY);
	//	cairo_paint_with_alpha(rd->cr,opacity*0.01);
}

imageObj* msImageCreateOgl(int width, int height, outputFormatObj *format, colorObj* bg)
{
	imageObj *pNewImage = NULL;

	if (format->imagemode != MS_IMAGEMODE_RGB && format->imagemode
			!= MS_IMAGEMODE_RGBA)
	{
		msSetError(
				MS_OGLERR, "OpenGL driver only supports RGB or RGBA pixel models.",
				"msImageCreateOGL()");
		return NULL;
	}

	pNewImage = (imageObj*)calloc(1, sizeof(imageObj));
	if (!pNewImage)
		return pNewImage;

	OglRenderer *ren = new OglRenderer(width, height, bg);
	pNewImage->img.plugin = (void *) ren;
	return pNewImage;
}

int msSaveImageOgl(imageObj *img, char *filename, outputFormatObj *format)
{
        imageObj* gdImg = msImageCreateGD(img->width, img->height, format, filename, NULL, 72, 72);

	OglRenderer* ogl = getOglRenderer(img);
	ogl->attach(gdImg);
	char *pFormatBuffer;
	char cGDFormat[128];
	int iReturn = 0;

	pFormatBuffer = format->driver;

	strcpy(cGDFormat, "gd/");
	strcat(cGDFormat, &(format->driver[4]));

	format->driver = &cGDFormat[0];

	iReturn = msSaveImageGD(gdImg, filename, format);

	format->driver = pFormatBuffer;

	return 1;
}

void msRenderEllipseOgl(imageObj *img, double x, double y, double width,
		double height, double angle, colorObj *color, colorObj *outlinecolor,
		double outlinewidth)
{
	OglRenderer* renderer = getOglRenderer(img);
	renderer->renderEllipse(x, y, angle, width, height, color, outlinecolor, outlinewidth);
}

void msFreeImageOgl(imageObj *img)
{
	OglRenderer* renderer = getOglRenderer(img);
	if (renderer)
	{
		delete renderer;
	}
	img->img.plugin=NULL;
}

void msStartNewLayerOgl(imageObj *img, double opacity)
{
	getOglRenderer(img)->setTransparency(opacity/100);
}

void msCloseNewLayerOgl(imageObj *img, double opacity)
{
	getOglRenderer(img)->setTransparency(1.0);
}

void msFreeSymbolOgl(symbolObj *s)
{
	
}
#endif /* USE_OGL */

int msPopulateRendererVTableOGL(rendererVTableObj *renderer) {
#ifdef USE_OGL
    	renderer->supports_transparent_layers = 1;
    	renderer->startNewLayer = msStartNewLayerOgl;
    	renderer->closeNewLayer = msCloseNewLayerOgl;
        renderer->renderLine=&msDrawLineOgl;
        renderer->createImage=&msImageCreateOgl;
        renderer->saveImage=&msSaveImageOgl;
        renderer->transformShape=&msTransformShapeAGG;
        renderer->renderPolygon=&msDrawPolygonOgl;
        renderer->renderGlyphs=&msRenderGlyphsOgl;
        renderer->renderEllipseSymbol = &msRenderEllipseOgl;
        renderer->renderVectorSymbol = &msRenderVectorSymbolOgl;
        renderer->renderPixmapSymbol = &msRenderPixmapOgl;
        renderer->mergeRasterBuffer = &msMergeImagesOgl;
        renderer->getTruetypeTextBBox = &msGetTruetypeTextBBoxOgl;
        renderer->createPixmapSymbolTile = &msCreateTilePixmapOgl;
        renderer->createVectorSymbolTile = &msCreateTileVectorOgl;
        renderer->createEllipseSymbolTile = &msCreateTileEllipseOgl;
        renderer->createTruetypeSymbolTile = &msCreateTileTruetypeOgl;
        renderer->renderTile = &msRenderTileOgl;
        renderer->renderPolygonTiled = &msDrawPolygonTiledOgl;
        renderer->renderLineTiled = &msDrawLineTiledOgl;
        renderer->freeTile = &msFreeTileOgl;
        renderer->freeSymbol = &msFreeSymbolOgl;
        renderer->freeImage=&msFreeImageOgl;
        return MS_SUCCESS;
    #else
        msSetError(MS_MISCERR,"OGL driver requested but it is not compiled in this release",
                "msPopulateRendererVTableOGL()");
        return MS_FAILURE;
#endif

}


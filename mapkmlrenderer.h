/******************************************************************************
 * 
 *
 * Project:  MapServer
 * Purpose:  Headers for mapkmlrenderer.cpp Google Earth KML output
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


#ifndef MAPKMLRENDERER_H
#define MAPKMLRENDERER_H

#if defined(USE_KML)

#include "mapserver.h"
#include "maplibxml2.h"


class KmlRenderer
{
protected:

	// map properties
	int				Width, Height;
	rectObj			MapExtent;
	double			MapCellsize;
	colorObj		BgColor;
	char			MapPath[MS_MAXPATHLEN];

	// xml nodes pointers
    xmlDocPtr	XmlDoc;
	xmlNodePtr	DocNode;
	xmlNodePtr	LayerNode;
	xmlNodePtr	GroundOverlayNode;

	xmlNodePtr	PlacemarkNode;
	xmlNodePtr	GeomNode;
	xmlNodePtr	DescriptionNode;

	shapeObj	*CurrentShape;
        shapeObj        *CurrentShapeDrawn;
	char		**Items;
	int			NumItems;
	int			DumpAttributes;

	// placemark symbology
	hashTableObj	*StyleHashTable;

	labelStyleObj	LabelStyle;
	strokeStyleObj	LineStyle;
	colorObj		PolygonColor;

	char			SymbolName[128];
	char			SymbolUrl[128];

	enum			{ NumSymbologyFlag = 4};
	char			SymbologyFlag[NumSymbologyFlag];

	enum			symbFlagsEnum { Label, Line, Polygon, Symbol };

	// internal output format containing cairo renderer used for rasterizing and rendering symbols
	outputFormatObj	*RasterizerOutputFormat;
	imageObj		*InternalImg;

	imageObj		*TempImg;

	int				FirstLayer;
        
        mapObj                  *map;
        layerObj                *currentLayer;
	// max number of features rendered in "vector KML" mode
	// default value 100
	// value can be specified in format option
	// OUTPUTFORMAT
	//  ...
	//	FORMATOPTION "kml_maxfeaturecount=512"
	// END	
	//
	// value can be specified also in layer metadata
	// LAYER
	// ...
	// METADATA
	//	kml_maxfeaturecount	"512"
	// END	
	// END
	int				MaxFeatureCount;

	// if FeatureCount is greater than MaxFeatureCount, layer is rasterized and
	// rendered as ground overlay
	// number of features in layer
	int				FeatureCount;

	// if true - features are rasterized
	int				VectorMode;

	// if true - features are written directly in kml
	int				RasterMode;

	int				AltitudeMode;
	int				Tessellate;
	int				Extrude;

	enum altitudeModeEnum { undefined, clampToGround, relativeToGround, absolute };

protected:

	imageObj* createInternalImage();
	xmlNodePtr createPlacemarkNode(xmlNodePtr parentNode, char *styleUrl);
	xmlNodePtr createGroundOverlayNode(char *imageHref);
	xmlNodePtr createDescriptionNode(shapeObj *shape);

	char* lookupSymbolUrl(imageObj *img, symbolObj *symbol, symbolStyleObj *style);

	void addCoordsNode(xmlNodePtr parentNode, pointObj *pts, int numPts);

	void setupRenderingParams(hashTableObj *layerMetadata);
	void addAddRenderingSpecifications(xmlNodePtr node);

	int checkProjection(projectionObj *projection);

	int createIconImage(char *fileName, symbolObj *symbol, symbolStyleObj *style);

	void renderSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style);

	//////////////////////////////////////////////////////////////////////////////

	void renderLineVector(imageObj *img, shapeObj *p, strokeStyleObj *style);
	void renderPolygonVector(imageObj *img, shapeObj *p, colorObj *color);
	void renderGlyphsVector(imageObj *img, double x, double y, labelStyleObj *style, char *text);

	char* lookupPlacemarkStyle();
	void flushPlacemark();
	xmlNodePtr getGeomParentNode(char *geomName);
        char* getLayerName(layerObj *layer);
        void processLayer(layerObj *layer);

public:

	KmlRenderer(int width, int height, colorObj* color = NULL);
	virtual ~KmlRenderer();

    imageObj* createImage(int width, int height, outputFormatObj *format, colorObj* bg);
    int saveImage(imageObj *img, FILE *fp, outputFormatObj *format);
    
	void startNewLayer(imageObj *img, layerObj *layer);
	void closeNewLayer(imageObj *img, layerObj *layer);

	void startShape(imageObj *img, shapeObj *shape);
	void endShape(imageObj *img, shapeObj *shape);

	void renderLine(imageObj *img, shapeObj *p, strokeStyleObj *style);
	void renderPolygon(imageObj *img, shapeObj *p, colorObj *color);

	void renderGlyphs(imageObj *img, double x, double y, labelStyleObj *style, char *text);

	// Symbols
	void renderPixmapSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style);
	void renderVectorSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style);
	void renderEllipseSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style);
	void renderTruetypeSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style);

	int getTruetypeTextBBox(imageObj *img,char *font, double size, char *string, rectObj *rect, double **advances);
};

#endif /* USE_KML */
#endif

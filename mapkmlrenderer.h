/******************************************************************************
 * $Id$
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

#include "mapserver-config.h"
#if defined(USE_KML)

#include "mapserver.h"
#include "maplibxml2.h"


class KmlRenderer
{
private:
  const char *pszLayerDescMetadata; /*if the kml_description is set*/
  char **papszLayerIncludeItems;
  int nIncludeItems;
  char **papszLayerExcludeItems;
  int nExcludeItems;
  char *pszLayerNameAttributeMetadata;

protected:

  // map properties
  int       Width, Height;
  rectObj     MapExtent;
  double      MapCellsize;
  colorObj    BgColor;
  char      MapPath[MS_MAXPATHLEN];

  // xml nodes pointers
  xmlDocPtr XmlDoc;
  xmlNodePtr  DocNode;
  xmlNodePtr  LayerNode;
  xmlNodePtr  GroundOverlayNode;

  xmlNodePtr  PlacemarkNode;
  xmlNodePtr  GeomNode;
  xmlNodePtr  DescriptionNode;

  int         CurrentShapeIndex;
  int         CurrentDrawnShapeIndex;
  char            *CurrentShapeName;
  char    **Items;
  int     NumItems;
  int     DumpAttributes;

  // placemark symbology
  hashTableObj  *StyleHashTable;

  labelStyleObj         LabelStyle;
  strokeStyleObj          *LineStyle;
  int                     numLineStyle;
  colorObj    PolygonColor;

  char      SymbolName[128];
  char      SymbolUrl[128];

  enum      { NumSymbologyFlag = 4};
  char      SymbologyFlag[NumSymbologyFlag];

  enum      symbFlagsEnum { Label, Line, Polygon, Symbol };

  int       FirstLayer;

  mapObj                  *map;
  layerObj                *currentLayer;

  int       AltitudeMode;
  int       Tessellate;
  int       Extrude;

  enum altitudeModeEnum { undefined, clampToGround, relativeToGround, absolute };
  /**True if elevation is taken from a feature attribute*/
  bool mElevationFromAttribute;
  /**Attribute index of elevation (or -1 if elevation is not attribute driven*/
  int mElevationAttributeIndex;
  double mCurrentElevationValue;


  outputFormatObj *aggFormat;

protected:

  imageObj* createInternalImage();
  xmlNodePtr createPlacemarkNode(xmlNodePtr parentNode, char *styleUrl);
  xmlNodePtr createGroundOverlayNode(xmlNodePtr parentNode, char *imageHref, layerObj *layer);
  xmlNodePtr createDescriptionNode(shapeObj *shape);

  char* lookupSymbolUrl(imageObj *img, symbolObj *symbol, symbolStyleObj *style);

  void addCoordsNode(xmlNodePtr parentNode, pointObj *pts, int numPts);

  void setupRenderingParams(hashTableObj *layerMetadata);
  void addAddRenderingSpecifications(xmlNodePtr node);

  int checkProjection(mapObj *map);

  int createIconImage(char *fileName, symbolObj *symbol, symbolStyleObj *style);

  void renderSymbol(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style);

  //////////////////////////////////////////////////////////////////////////////

  void renderLineVector(imageObj *img, shapeObj *p, strokeStyleObj *style);
  void renderPolygonVector(imageObj *img, shapeObj *p, colorObj *color);
  void renderGlyphsVector(imageObj *img, double x, double y, labelStyleObj *style, char *text);

  char* lookupPlacemarkStyle();
  void flushPlacemark();
  xmlNodePtr getGeomParentNode(const char *geomName);
  char* getLayerName(layerObj *layer);
  void processLayer(layerObj *layer, outputFormatObj *format);
  void addLineStyleToList(strokeStyleObj *style);
  const char *getAliasName(layerObj *lp, char *pszItemName, const char *namespaces);

public:

  KmlRenderer(int width, int height, outputFormatObj *format, colorObj* color = NULL);
  virtual ~KmlRenderer();

  imageObj* createImage(int width, int height, outputFormatObj *format, colorObj* bg);
  int saveImage(imageObj *img, FILE *fp, outputFormatObj *format);

  int startNewLayer(imageObj *img, layerObj *layer);
  int closeNewLayer(imageObj *img, layerObj *layer);

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

  int getTruetypeTextBBox(imageObj *img,char **fonts, int numfonts, double size, char *string, rectObj *rect, double **advances);
  int mergeRasterBuffer(imageObj *image, rasterBufferObj *rb);
};

#endif /* USE_KML */
#endif

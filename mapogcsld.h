/**********************************************************************
 * $Id$
 *
 * Name:     mapogcsld.h
 * Project:  MapServer
 * Language: C
 * Purpose:  OGC SLD implementation
 * Author:   Y. Assefa, DM Solutions Group (assefa@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2003, Y. Assefa, DM Solutions Group Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 * $Log$
 * Revision 1.4  2003/11/30 16:30:04  assefa
 * Support mulitple symbolisers in a Rule.
 *
 * Revision 1.3  2003/11/25 03:21:44  assefa
 * Add test support.
 * Add filter support.
 *
 * Revision 1.2  2003/11/07 21:35:07  assefa
 * Add PointSymbolizer.
 * Add External Graphic symbol support.
 *
 * Revision 1.1  2003/11/06 23:09:25  assefa
 * OGC SLD support.
 *
 *
 **********************************************************************/

#ifdef USE_OGR

#include "map.h"
/* There is a dependency to OGR for the MiniXML parser */
#include "cpl_minixml.h"


/* -------------------------------------------------------------------- */
/*      prototypes.                                                     */
/* -------------------------------------------------------------------- */
void msSLDApplySLDURL(mapObj *map, char *szURL);
void msSLDApplySLD(mapObj *map, char *psSLDXML);
layerObj  *msSLDParseSLD(mapObj *map, char *psSLDXML, int *pnLayers);
void msSLDParseNamedLayer(CPLXMLNode *psRoot, layerObj *layer);
void msSLDParseRule(CPLXMLNode *psRoot, layerObj *psLayer);
void msSLDParseStroke(CPLXMLNode *psStroke, styleObj *psStyle,
                      mapObj *map, int iColorParam);
void msSLDParsePolygonFill(CPLXMLNode *psFill, styleObj *psStyle,
                           mapObj *map);

void msSLDParseLineSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,  
                              int bNewClass);
void msSLDParsePolygonSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                                  int bNewClass);
void msSLDParsePointSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer, 
                               int bNewClass);
void msSLDParseTextSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                              int bOtherSymboliser);

void msSLDParseGraphicFillOrStroke(CPLXMLNode *psGraphicFill,
                                   char *pszDashValue,
                                   styleObj *psStyle, mapObj *map);
void msSLDParseExternalGraphic(CPLXMLNode *psExternalGraphic, styleObj *psStyle, 
                              mapObj *map);

int msSLDGetLineSymbol(mapObj *map);
int msSLDGetDashLineSymbol(mapObj *map, char *pszDashArray);
int msSLDGetMarkSymbol(mapObj *map, char *pszSymbolName, int bFilled,
                       char *pszDashValue);
int msSLDGetGraphicSymbol(mapObj *map, char *pszFileName);

void msSLDSetColorObject(char *psHexColor, colorObj *psColor);

void msSLDParseTextParams(CPLXMLNode *psRoot, layerObj *psLayer, classObj *psClass);
void ParseTextPointPlacement(CPLXMLNode *psRoot, classObj *psClass);
void ParseTextLinePlacement(CPLXMLNode *psRoot, classObj *psClass);

#endif

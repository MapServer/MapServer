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
void msSLDApplySLD(mapObj *map, const char *wmtver, char *psSLDXML);
layerObj  *msSLDParseSLD(mapObj *map, char *psSLDXML, int *pnLayers);
void msSLDParseNamedLayer(CPLXMLNode *psRoot, layerObj *layer);
void msSLDParseRule(CPLXMLNode *psRoot, layerObj *psLayer);
void msSLDParseStroke(CPLXMLNode *psStroke, styleObj *psStyle,
                      mapObj *map, int iColorParam);
void msSLDParsePolygonFill(CPLXMLNode *psFill, styleObj *psStyle,
                           mapObj *map);

void msSLDParseLineSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer);
void msSLDParsePolygonSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer);
void msSLDParseGraphicFillOrStroke(CPLXMLNode *psGraphicFill,
                                   char *pszDashValue,
                                   styleObj *psStyle, mapObj *map);
int msSLDGetLineSymbol(mapObj *map);
int msSLDGetDashLineSymbol(mapObj *map, char *pszDashArray);
int msSLDGetMarkSymbol(mapObj *map, char *pszSymbolName, int bFilled,
                       char *pszDashValue);

void msSLDSetColorObject(char *psHexColor, colorObj *psColor);
#endif

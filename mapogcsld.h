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
 * Revision 1.13  2005/05/01 15:15:47  sean
 * Previous commit mangled revision 1.12, now repaired (bug 1337)
 *
 * Revision 1.12  2005/05/01 15:05:09  sean
 * Moved prototypes for 3 functions outside the #ifdef USE_OGR block to allow builds without GDAL/OGR (bug 1337)
 *
 * Revision 1.11  2004/07/29 21:50:19  assefa
 * Use wfs_filter metedata when generating an SLD (Bug 782)
 *
 * Revision 1.10  2004/04/12 18:38:24  assefa
 * Add dll export support for windows.
 *
 * Revision 1.9  2004/02/06 02:23:01  assefa
 * Make sure that point symbolizers always initialize the color
 * parameter of the style.
 *
 * Revision 1.8  2004/01/05 21:17:53  assefa
 * ApplySLD and ApplySLDURL on a layer can now take a NamedLayer name as argument.
 *
 * Revision 1.7  2003/12/05 04:02:34  assefa
 * Add generation of SLD for points and text.
 *
 * Revision 1.6  2003/12/03 18:52:21  assefa
 * Add partly support for SLD generation.
 *
 * Revision 1.5  2003/12/01 16:10:13  assefa
 * Add #ifdef USE_OGR for sld functions available to mapserver.
 *
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

#include "map.h"

MS_DLL_EXPORT char *msSLDGenerateSLD(mapObj *map, int iLayer);
MS_DLL_EXPORT int msSLDApplySLDURL(mapObj *map, char *szURL, int iLayer,
                                   char *pszStyleLayerName);
MS_DLL_EXPORT int msSLDApplySLD(mapObj *map, char *psSLDXML, int iLayer, 
                                char *pszStyleLayerName);

#ifdef USE_OGR

/* There is a dependency to OGR for the MiniXML parser */
#include "cpl_minixml.h"


/* -------------------------------------------------------------------- */
/*      prototypes.                                                     */
/* -------------------------------------------------------------------- */
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
void msSLDParseRasterSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer);

void msSLDParseGraphicFillOrStroke(CPLXMLNode *psGraphicFill,
                                   char *pszDashValue,
                                   styleObj *psStyle, mapObj *map, int bPointLayer);
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

char *msSLDGenerateSLDLayer(layerObj *psLayer);

char *msSLDGetFilter(classObj *psClass, const char *pszWfsFilter);
char *msSLDGenerateLineSLD(styleObj *psStyle, layerObj *psLayer);
char *msSLDGeneratePolygonSLD(styleObj *psStyle, layerObj *psLayer);
char *msSLDGeneratePointSLD(styleObj *psStyle, layerObj *psLayer);
char *msSLDGenerateTextSLD(classObj *psClass, layerObj *psLayer);


#endif

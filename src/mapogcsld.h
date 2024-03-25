/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
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
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 ****************************************************************************/

#include "mapserver.h"

#ifdef __cplusplus
extern "C" {
#endif

MS_DLL_EXPORT char *msSLDGenerateSLD(mapObj *map, int iLayer,
                                     const char *pszVersion);
MS_DLL_EXPORT int msSLDApplySLDURL(mapObj *map, const char *szURL, int iLayer,
                                   const char *pszStyleLayerName,
                                   char **ppszLayerNames);
MS_DLL_EXPORT int msSLDApplySLD(mapObj *map, const char *psSLDXML, int iLayer,
                                const char *pszStyleLayerName,
                                char **ppszLayerNames);
int msSLDApplyFromFile(mapObj *map, layerObj *layer, const char *filename);

/* There is a dependency to OGR for the MiniXML parser */
#include "cpl_minixml.h"

enum objType { MS_OBJ_STYLE, MS_OBJ_LABEL };

/* -------------------------------------------------------------------- */
/*      prototypes.                                                     */
/* -------------------------------------------------------------------- */
layerObj *msSLDParseSLD(mapObj *map, const char *psSLDXML, int *pnLayers);
int msSLDParseNamedLayer(CPLXMLNode *psRoot, layerObj *layer);
int msSLDParseRule(CPLXMLNode *psRoot, layerObj *psLayer,
                   const char *pszUserStyleName);
int msSLDParseStroke(CPLXMLNode *psStroke, styleObj *psStyle, mapObj *map,
                     int iColorParam);
int msSLDParseOgcExpression(CPLXMLNode *psRoot, void *psObj, int binding,
                            enum objType objtype);
int msSLDParsePolygonFill(CPLXMLNode *psFill, styleObj *psStyle, mapObj *map);

int msSLDParseLineSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                             int bNewClass, const char *pszUserStyleName);
int msSLDParsePolygonSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                                int bNewClass, const char *pszUserStyleName);
int msSLDParsePointSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                              int bNewClass, const char *pszUserStyleName);
int msSLDParseTextSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                             int bOtherSymboliser,
                             const char *pszUserStyleName);
int msSLDParseRasterSymbolizer(CPLXMLNode *psRoot, layerObj *psLayer,
                               const char *pszUserStyleName);

int msSLDParseGraphicFillOrStroke(CPLXMLNode *psGraphicFill, char *pszDashValue,
                                  styleObj *psStyle, mapObj *map);
int msSLDParseExternalGraphic(CPLXMLNode *psExternalGraphic, styleObj *psStyle,
                              mapObj *map);

int msSLDGetLineSymbol(mapObj *map);
int msSLDGetDashLineSymbol(mapObj *map, char *pszDashArray);
int msSLDGetMarkSymbol(mapObj *map, char *pszSymbolName, int bFilled);
int msSLDGetGraphicSymbol(mapObj *map, char *pszFileName, char *extGraphicName,
                          int nGap);

int msSLDSetColorObject(char *psHexColor, colorObj *psColor);

int msSLDParseTextParams(CPLXMLNode *psRoot, layerObj *psLayer,
                         classObj *psClass);
int ParseTextPointPlacement(CPLXMLNode *psRoot, classObj *psClass);
int ParseTextLinePlacement(CPLXMLNode *psRoot, classObj *psClass);

char *msSLDGenerateSLDLayer(layerObj *psLayer, int nVersion);

char *msSLDGetFilter(classObj *psClass, const char *pszWfsFilter);
char *msSLDGenerateLineSLD(styleObj *psStyle, layerObj *psLayer, int nVersion);
char *msSLDGeneratePolygonSLD(styleObj *psStyle, layerObj *psLayer,
                              int nVersion);
char *msSLDGeneratePointSLD(styleObj *psStyle, layerObj *psLayer, int nVersion);
char *msSLDGenerateTextSLD(classObj *psClass, layerObj *psLayer, int nVersion);
FilterEncodingNode *BuildExpressionTree(char *pszExpression,
                                        FilterEncodingNode *psNode);

#ifdef __cplusplus
} /* extern C */
#endif

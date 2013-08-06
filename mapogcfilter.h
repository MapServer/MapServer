/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC Filter Encoding implementation
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

#ifndef MAPOGCFILTER_H
#define MAPOGCFILTER_H

#include "mapserver.h"

/*dont need ogr for these functikons*/
MS_DLL_EXPORT int FLTIsNumeric(char *pszValue);
MS_DLL_EXPORT int FLTApplyExpressionToLayer(layerObj *lp, char *pszExpression);
MS_DLL_EXPORT  char *FLTGetExpressionForValuesRanges(layerObj *lp, char *item, char *value,  int forcecharcter);

#ifdef USE_OGR

/* There is a dependency to OGR for the MiniXML parser */
#include "cpl_minixml.h"

#ifdef USE_LIBXML2

#include<libxml/parser.h>
#include<libxml/tree.h>
#endif


typedef struct {
  char *pszWildCard;
  char *pszSingleChar;
  char *pszEscapeChar;
  int  bCaseInsensitive;
} FEPropertyIsLike;

/* -------------------------------------------------------------------- */
/*      prototypes.                                                     */
/* -------------------------------------------------------------------- */
MS_DLL_EXPORT FilterEncodingNode *FLTParseFilterEncoding(char *szXMLString);
MS_DLL_EXPORT FilterEncodingNode *FLTCreateFilterEncodingNode(void);
MS_DLL_EXPORT int FLTApplyFilterToLayer(FilterEncodingNode *psNode, mapObj *map,
                                        int iLayerIndex);

MS_DLL_EXPORT int FLTLayerApplyCondSQLFilterToLayer(FilterEncodingNode *psNode, mapObj *map,
    int iLayerIndex);
MS_DLL_EXPORT int FLTLayerApplyPlainFilterToLayer(FilterEncodingNode *psNode, mapObj *map,
    int iLayerIndex);

MS_DLL_EXPORT void FLTFreeFilterEncodingNode(FilterEncodingNode *psFilterNode);

MS_DLL_EXPORT int FLTValidFilterNode(FilterEncodingNode *psFilterNode);
MS_DLL_EXPORT int FLTValidForBBoxFilter(FilterEncodingNode *psFilterNode);
MS_DLL_EXPORT int FLTNumberOfFilterType(FilterEncodingNode *psFilterNode,
                                        const char *szType);
MS_DLL_EXPORT int FLTIsBBoxFilter(FilterEncodingNode *psFilterNode);
MS_DLL_EXPORT int FLTIsPointFilter(FilterEncodingNode *psFilterNode);
MS_DLL_EXPORT int FLTIsLineFilter(FilterEncodingNode *psFilterNode);
MS_DLL_EXPORT int FLTIsPolygonFilter(FilterEncodingNode *psFilterNode);

MS_DLL_EXPORT int FLTValidForPropertyIsLikeFilter(FilterEncodingNode *psFilterNode);
MS_DLL_EXPORT char *FLTGetMapserverIsPropertyExpression(FilterEncodingNode *psFilterNode,
    layerObj *lp);
MS_DLL_EXPORT int FLTIsOnlyPropertyIsLike(FilterEncodingNode *psFilterNode);

MS_DLL_EXPORT void FLTInsertElementInNode(FilterEncodingNode *psFilterNode,
    CPLXMLNode *psXMLNode);
MS_DLL_EXPORT int FLTIsLogicalFilterType(char *pszValue);
MS_DLL_EXPORT int FLTIsBinaryComparisonFilterType(char *pszValue);
MS_DLL_EXPORT int FLTIsComparisonFilterType(char *pszValue);
MS_DLL_EXPORT int FLTIsFeatureIdFilterType(char *pszValue);
MS_DLL_EXPORT int FLTIsSpatialFilterType(char *pszValue);
MS_DLL_EXPORT int FLTIsSupportedFilterType(CPLXMLNode *psXMLNode);

MS_DLL_EXPORT char *FLTGetMapserverExpression(FilterEncodingNode *psFilterNode, layerObj *lp);
MS_DLL_EXPORT char *FLTGetNodeExpression(FilterEncodingNode *psFilterNode, layerObj *lp);
MS_DLL_EXPORT char *FLTGetBBOX(FilterEncodingNode *psFilterNode, rectObj *psRect);

MS_DLL_EXPORT shapeObj *FLTGetShape(FilterEncodingNode *psFilterNode, double *pdfDistance,
                                    int *pnUnit);

MS_DLL_EXPORT char *FLTGetLogicalComparisonExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp);
MS_DLL_EXPORT char *FLTGetBinaryComparisonExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp);
MS_DLL_EXPORT char *FLTGetIsBetweenComparisonExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp);
MS_DLL_EXPORT char *FLTGetIsLikeComparisonExpression(FilterEncodingNode *psFilterNode);
MS_DLL_EXPORT int FLTHasSpatialFilter(FilterEncodingNode *psFilterNode);


/*SQL expressions related functions.*/
MS_DLL_EXPORT int FLTApplySimpleSQLFilter(FilterEncodingNode *psNode, mapObj *map,
    int iLayerIndex);

MS_DLL_EXPORT char *FLTGetSQLExpression(FilterEncodingNode *psFilterNode,layerObj *lp);
MS_DLL_EXPORT char *FLTGetBinaryComparisonSQLExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp);
MS_DLL_EXPORT char *FLTGetIsBetweenComparisonSQLExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp);
MS_DLL_EXPORT char *FLTGetIsLikeComparisonSQLExpression(FilterEncodingNode *psFilterNode, layerObj *lp);

MS_DLL_EXPORT char *FLTGetLogicalComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
    layerObj *lp);
MS_DLL_EXPORT int FLTIsSimpleFilter(FilterEncodingNode *psFilterNode);

MS_DLL_EXPORT FilterEncodingNode *FLTCreateFeatureIdFilterEncoding(char *pszString);

MS_DLL_EXPORT int FLTParseEpsgString(char *pszEpsg, projectionObj *psProj);
MS_DLL_EXPORT int FLTParseGMLEnvelope(CPLXMLNode *psRoot, rectObj *psBbox, char **ppszSRS);
MS_DLL_EXPORT  int FLTParseGMLBox(CPLXMLNode *psBox, rectObj *psBbox, char **ppszSRS);

/*common-expressions*/
MS_DLL_EXPORT   char *FLTGetBinaryComparisonCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp);
MS_DLL_EXPORT  char *FLTGetCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp);
MS_DLL_EXPORT int FLTApplyFilterToLayerCommonExpression(mapObj *map, int iLayerIndex, char *pszExpression);


#ifdef USE_LIBXML2
MS_DLL_EXPORT xmlNodePtr FLTGetCapabilities(xmlNsPtr psNsParent, xmlNsPtr psNsOgc, int bTemporal);
#endif


void FLTPreParseFilterForAlias(FilterEncodingNode *psFilterNode,
                               mapObj *map, int i, const char *namespaces);

#endif


#endif

/**********************************************************************
 * $Id$
 *
 * Name:     mapogcfilter.h
 * Project:  MapServer
 * Language: C
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
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 ****************************************************************************/

#ifdef USE_OGR

#include "map.h"
/* There is a dependency to OGR for the MiniXML parser */
#include "cpl_minixml.h"



typedef struct
{
      char *pszWildCard;
      char *pszSingleChar;
      char *pszEscapeChar;
      int  bCaseInsensitive;
}FEPropertyIsLike;

/* -------------------------------------------------------------------- */
/*      prototypes.                                                     */
/* -------------------------------------------------------------------- */
FilterEncodingNode *FLTParseFilterEncoding(char *szXMLString);
FilterEncodingNode *FLTCreateFilterEncodingNode(void);
int FLTApplyFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                         int iLayerIndex, int bOnlySpatialFilter);

int FLTLayerApplyCondSQLFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                     int iLayerIndex, int bOnlySpatialFilter);
int FLTLayerApplyPlainFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                   int iLayerIndex, int bOnlySpatialFilter);

int FLTApplySpatialFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                 int iLayerIndex);

void FLTFreeFilterEncodingNode(FilterEncodingNode *psFilterNode);

int FLTValidFilterNode(FilterEncodingNode *psFilterNode);
int FLTValidForBBoxFilter(FilterEncodingNode *psFilterNode);
int FLTNumberOfFilterType(FilterEncodingNode *psFilterNode, 
                          const char *szType);
int FLTIsBBoxFilter(FilterEncodingNode *psFilterNode);
int FLTIsPointFilter(FilterEncodingNode *psFilterNode);
int FLTIsLineFilter(FilterEncodingNode *psFilterNode);
int FLTIsPolygonFilter(FilterEncodingNode *psFilterNode);

int FLTValidForPropertyIsLikeFilter(FilterEncodingNode *psFilterNode);
char *FLTGetMapserverIsPropertyExpression(FilterEncodingNode *psFilterNode);
int FLTIsOnlyPropertyIsLike(FilterEncodingNode *psFilterNode);

void FLTInsertElementInNode(FilterEncodingNode *psFilterNode,
                            CPLXMLNode *psXMLNode);
int FLTIsLogicalFilterType(char *pszValue);
int FLTIsBinaryComparisonFilterType(char *pszValue);
int FLTIsComparisonFilterType(char *pszValue);
int FLTIsSpatialFilterType(char *pszValue);
int FLTIsSupportedFilterType(CPLXMLNode *psXMLNode);

char *FLTGetMapserverExpression(FilterEncodingNode *psFilterNode);
char *FLTGetMapserverExpressionClassItem(FilterEncodingNode *psFilterNode);
char *FLTGetNodeExpression(FilterEncodingNode *psFilterNode);
char *FLTGetBBOX(FilterEncodingNode *psFilterNode, rectObj *psRect);

shapeObj *FLTGetShape(FilterEncodingNode *psFilterNode, double *pdfDistance,
                      int *pnUnit);

char *FLTGetLogicalComparisonExpresssion(FilterEncodingNode *psFilterNode);
char *FLTGetBinaryComparisonExpresssion(FilterEncodingNode *psFilterNode);
char *FLTGetIsBetweenComparisonExpresssion(FilterEncodingNode *psFilterNode);
char *FLTGetIsLikeComparisonExpression(FilterEncodingNode *psFilterNode);
int FLTHasSpatialFilter(FilterEncodingNode *psFilterNode);


/*SQL expressions related functions.*/
int FLTApplySimpleSQLFilter(FilterEncodingNode *psNode, mapObj *map, 
                          int iLayerIndex);

char *FLTGetSQLExpression(FilterEncodingNode *psFilterNode,int connectiontype);
char *FLTGetBinaryComparisonSQLExpresssion(FilterEncodingNode *psFilterNode);
char *FLTGetIsBetweenComparisonSQLExpresssion(FilterEncodingNode *psFilterNode);
char *FLTGetIsLikeComparisonSQLExpression(FilterEncodingNode *psFilterNode,
                                       int connectiontype);
char *FLTGetLogicalComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
                                            int connectiontype);
int FLTIsSimpleFilter(FilterEncodingNode *psFilterNode);


#endif

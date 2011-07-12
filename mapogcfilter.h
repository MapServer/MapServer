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
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 * $Log$
 * Revision 1.15  2006/04/08 03:39:15  frank
 * improve error propagation
 *
 * Revision 1.14  2005/10/29 02:03:43  jani
 * MS RFC 8: Pluggable External Feature Layer Providers (bug 1477).
 *
 * Revision 1.13  2005/10/28 01:09:42  jani
 * MS RFC 3: Layer vtable architecture (bug 1477)
 *
 * Revision 1.12  2005/07/20 13:55:34  assefa
 * Added support of case insensitive matChase attribute.
 *
 * Revision 1.11  2005/05/12 17:38:54  assefa
 * prototype for FLTGetShape changed : Now parsing unit value.
 *
 * Revision 1.10  2005/04/25 06:41:56  sdlime
 * Applied Bill's newest gradient patch, more concise in the mapfile and potential to use via MapScript.
 *
 * Revision 1.9  2005/03/29 22:53:14  assefa
 * Initial support to improve WFS filter performance for DB layers (Bug 1292).
 *
 * Revision 1.8  2005/01/28 06:16:54  sdlime
 * Applied patch to make function prototypes ANSI C compliant. Thanks to Petter Reinholdtsen. This fixes but 1181.
 *
 * Revision 1.7  2004/07/28 22:16:17  assefa
 * Add support for spatial filters inside an SLD. (Bug 782).
 *
 * Revision 1.6  2004/02/04 19:46:24  assefa
 * Add support for multiple spatial opertaors inside one filter.
 * Add support for opeartors DWithin and Intersect.
 *
 * Revision 1.5  2004/01/13 19:33:10  assefa
 * Correct in bug when builing expression for the IsLIke operator.
 *
 * Revision 1.4  2003/10/07 23:54:24  assefa
 * Additional Validation for propertyislike.
 *
 * Revision 1.3  2003/09/26 13:44:40  assefa
 * Add support for gml box with 2 <coord> elements.
 *
 * Revision 1.2  2003/09/23 14:34:34  assefa
 * ifdef's for OGR use.
 *
 * Revision 1.1  2003/09/10 19:54:27  assefa
 * Renamed from fileterencoding.c/h
 *
 * Revision 1.4  2003/09/10 03:54:09  assefa
 * Add partial support for BBox.
 * Add Node validating functions.
 *
 * Revision 1.3  2003/09/02 22:59:06  assefa
 * Add classitem extrcat function for IsLike filter.
 *
 * Revision 1.2  2003/08/26 02:18:09  assefa
 * Add PropertyIsBetween and PropertyIsLike.
 *
 * Revision 1.1  2003/08/13 21:54:32  assefa
 * Initial revision.
 *
 *
 **********************************************************************/

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

char *FLTGetSQLExpression(FilterEncodingNode *psFilterNode,layerObj *lp);
char *FLTGetBinaryComparisonSQLExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp);
char *FLTGetIsBetweenComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,layerObj *lp);
char *FLTGetIsLikeComparisonSQLExpression(FilterEncodingNode *psFilterNode, layerObj *lp);
char *FLTGetLogicalComparisonSQLExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp);
int FLTIsSimpleFilter(FilterEncodingNode *psFilterNode);


#endif

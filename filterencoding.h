/**********************************************************************
 * $Id$
 *
 * Name:     filterencoding.h
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

#include "map.h"
/* There is a dependency to GDAL/OGR for the MiniXML parser */
#include "cpl_minixml.h"


typedef enum 
{
    FILTER_NODE_TYPE_UNDEFINED = -1,
    FILTER_NODE_TYPE_LOGICAL = 0,
    FILTER_NODE_TYPE_SPATIAL = 1,
    FILTER_NODE_TYPE_COMPARISON = 2,
    FILTER_NODE_TYPE_PROPERTYNAME = 3,
    FILTER_NODE_TYPE_BBOX = 4,
    FILTER_NODE_TYPE_LITERAL = 5,
    FILTER_NODE_TYPE_BOUNDARY = 6
} FilterNodeType;


typedef struct _FilterNode
{
    FilterNodeType      eType;
    char                *pszValue;
    void                *pOther;

    struct _FilterNode  *psLeftNode;
    struct _FilterNode  *psRightNode;

      
}FilterEncodingNode;


typedef struct
{
    char *pszWildCard;
    char *pszSingleChar;
    char *pszEscapeChar;
}FEPropertyIsLike;

/* -------------------------------------------------------------------- */
/*      prototypes.                                                     */
/* -------------------------------------------------------------------- */
FilterEncodingNode *FLTParseFilterEncoding(char *szXMLString);
FilterEncodingNode *FLTCreateFilterEncodingNode();

void FLTFreeFilterEncodingNode(FilterEncodingNode *psFilterNode);
int FLTValidFilterNode(FilterEncodingNode *psFilterNode);

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

char *FLTGetLogicalComparisonExpresssion(FilterEncodingNode *psFilterNode);
char *FLTGetBinaryComparisonExpresssion(FilterEncodingNode *psFilterNode);
char *FLTGetIsBetweenComparisonExpresssion(FilterEncodingNode *psFilterNode);
char *FLTGetIsLikeComparisonExpresssion(FilterEncodingNode *psFilterNode);

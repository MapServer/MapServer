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
FilterEncodingNode *PaserFilterEncoding(char *szXMLString);
FilterEncodingNode *CreateFilerEncodingNode();
void FreeFilterEncodingNode(FilterEncodingNode *psFilterNode);

void InsertElementInNode(FilterEncodingNode *psFilterNode,
                         CPLXMLNode *psXMLNode);
int IsLogicalFilterType(char *pszValue);
int IsBinaryComparisonFilterType(char *pszValue);
int IsComparisonFilterType(char *pszValue);
int IsSpatialFilterType(char *pszValue);
int IsSupportedFilterType(CPLXMLNode *psXMLNode);

char *GetMapserverExpression(FilterEncodingNode *psFilterNode);
char *GetNodeExpression(FilterEncodingNode *psFilterNode);
char *GetLogicalComparisonExpresssion(FilterEncodingNode *psFilterNode);
char *GetBinaryComparisonExpresssion(FilterEncodingNode *psFilterNode);
char *GetIsBetweenComparisonExpresssion(FilterEncodingNode *psFilterNode);
char *GetIsLikeComparisonExpresssion(FilterEncodingNode *psFilterNode);

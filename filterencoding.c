/**********************************************************************
 * $Id$
 *
 * Name:     filterencoding.c
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
 * Revision 1.1  2003/08/13 21:54:32  assefa
 * Initial revision.
 *
 *
 **********************************************************************/

#include "filterencoding.h"


/************************************************************************/
/*            FilterNode *PaserFilterEncoding(char *szXMLString)        */
/*                                                                      */
/*      Parses an Filter Encoding XML string and creates a              */
/*      FilterEncodingNodes corresponding to the string.                */
/*      Returns a pointer to the first node or NULL if                  */
/*      unsuccessfull.                                                  */
/*      Calling function should use FreeFilterEncodingNode function     */
/*      to free memeory.                                                */
/************************************************************************/
FilterEncodingNode *PaserFilterEncoding(char *szXMLString)
{
    CPLXMLNode *psRoot, *psChild, *psFilter = NULL;
    FilterEncodingNode *psFilterNode = NULL;

    if (szXMLString == NULL || strlen(szXMLString) <= 0 ||
        (strstr(szXMLString, "<Filter>") == NULL))
      return NULL;

    psRoot = CPLParseXMLString(szXMLString);
    if( psRoot == NULL)
       return NULL;

/* -------------------------------------------------------------------- */
/*      get the root element (Filter).                                  */
/* -------------------------------------------------------------------- */
    psChild = psRoot;
    psFilter = NULL;
    while( psChild != NULL )
    {
        if (psChild->eType == CXT_Element &&
            EQUAL(psChild->pszValue,"Filter"))
        {
            psFilter = psChild;
            break;
        }
        else
          psChild = psChild->psNext;
    }

    if (!psFilter)
      return NULL;

    psChild = psFilter->psChild;
    if (psChild && IsSupportedFilterType(psChild))
    {
        psFilterNode = CreateFilerEncodingNode();
        
        InsertElementInNode(psFilterNode, psChild);
    }

    return psFilterNode;
}

/************************************************************************/
/*                          FreeFilterEncodingNode                      */
/*                                                                      */
/*      recursive freeing of Filer Encoding nodes.                      */
/************************************************************************/
void FreeFilterEncodingNode(FilterEncodingNode *psFilterNode)
{
    if (psFilterNode)
    {
        if (psFilterNode->psLeftNode)
        {
            FreeFilterEncodingNode(psFilterNode->psLeftNode);
            psFilterNode->psLeftNode = NULL;
        }
        if (psFilterNode->psRightNode)
        {
            FreeFilterEncodingNode(psFilterNode->psRightNode);
            psFilterNode->psRightNode = NULL;
        }

        free(psFilterNode);
    }
}


/************************************************************************/
/*                         CreateFilerEncodingNode                      */
/*                                                                      */
/*      return a FilerEncoding node.                                    */
/************************************************************************/
FilterEncodingNode *CreateFilerEncodingNode()
{
    FilterEncodingNode *psFilterNode = NULL;

    psFilterNode = 
      (FilterEncodingNode *)malloc(sizeof (FilterEncodingNode));
    psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
    psFilterNode->pszValue = NULL;
    psFilterNode->psLeftNode = NULL;
    psFilterNode->psRightNode = NULL;

    return psFilterNode;
}


/************************************************************************/
/*                           InsertElementInNode                        */
/*                                                                      */
/*      Utility function to parse an XML node and transfter the         */
/*      contennts into the Filer Encoding node structure.               */
/************************************************************************/
void InsertElementInNode(FilterEncodingNode *psFilterNode,
                         CPLXMLNode *psXMLNode)
{
    int nStrLength = 0;

    if (psFilterNode && psXMLNode)
    {
        psFilterNode->pszValue = strdup(psXMLNode->pszValue);
        psFilterNode->psLeftNode = NULL;
        psFilterNode->psRightNode = NULL;
        
/* -------------------------------------------------------------------- */
/*      Logical filter. AND, OR and NOT are supported. Example of       */
/*      filer using logical filters :                                   */
/*      <Filter>                                                        */
/*        <And>                                                         */
/*          <PropertyIsGreaterThan>                                     */
/*            <PropertyName>Person/Age</PropertyName>                   */
/*            <Literal>50</Literal>                                     */
/*          </PropertyIsGreaterThan>                                    */
/*          <PropertyIsEqualTo>                                         */
/*             <PropertyName>Person/Address/City</PropertyName>         */
/*             <Literal>Toronto</Literal>                               */
/*          </PropertyIsEqualTo>                                        */
/*        </And>                                                        */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
        if (IsLogicalFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_LOGICAL;
            if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
                strcasecmp(psFilterNode->pszValue, "OR") == 0)
            {
                if (psXMLNode->psChild && psXMLNode->psChild->psNext)
                {
                    psFilterNode->psLeftNode = CreateFilerEncodingNode();
                    InsertElementInNode(psFilterNode->psLeftNode, 
                                        psXMLNode->psChild);
                    psFilterNode->psRightNode = CreateFilerEncodingNode();
                    InsertElementInNode(psFilterNode->psRightNode, 
                                        psXMLNode->psChild->psNext);
                }
            }
            else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0)
            {
                if (psXMLNode->psChild)
                {
                    psFilterNode->psLeftNode = CreateFilerEncodingNode();
                    InsertElementInNode(psFilterNode->psLeftNode, 
                                        psXMLNode->psChild); 
                }
            }                           
        }//end if is logical
/* -------------------------------------------------------------------- */
/*      Spatial Filter. Only BBox is supported. Eg of Filer using       */
/*      BBOX :                                                          */
/*      <Filter>                                                        */
/*       <BBOX>                                                         */
/*        <PropertyName>Geometry</PropertyName>                         */
/*        <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326”>*/
/*          <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*        </gml:Box>                                                    */
/*       </BBOX>                                                        */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
        else if (IsSpatialFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_SPATIAL;
            if (psXMLNode->psChild && psXMLNode->psChild->psNext &&
                strcasecmp(psXMLNode->psChild->pszValue, "PropertyName") == 0 &&
                strcasecmp(psXMLNode->psChild->psNext->pszValue,"gml:Box") == 0)
                
            {
                psFilterNode->psLeftNode = CreateFilerEncodingNode();

                psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                psFilterNode->psLeftNode->pszValue = 
                  psXMLNode->psChild->pszValue;

                if (psXMLNode->psChild->psNext->psChild)
                {
                    psFilterNode->psRightNode = CreateFilerEncodingNode();

                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BBOX;
                
                    psFilterNode->psRightNode->pszValue = 
                      psXMLNode->psChild->psNext->psChild->pszValue;
                }
            }
        }//end of is spatial
/* -------------------------------------------------------------------- */
/*      Comparison Filter                                               */
/* -------------------------------------------------------------------- */
        else if (IsComparisonFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_COMPARISON;
/* -------------------------------------------------------------------- */
/*      binary comaparison types. Example :                             */
/*                                                                      */
/*      <Filter>                                                        */
/*        <PropertyIsEqualTo>                                           */
/*          <PropertyName>SomeProperty</PropertyName>                   */
/*          <Literal>100</Literal>                                      */
/*        </PropertyIsEqualTo>                                          */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
            if (IsBinaryComparisonFilterType(psXMLNode->pszValue))
            {
                if (psXMLNode->psChild && psXMLNode->psChild->psNext &&
                    strcasecmp(psXMLNode->psChild->pszValue, "PropertyName") == 0 &&
                    strcasecmp(psXMLNode->psChild->psNext->pszValue,"Literal") == 0)
                {
                    psFilterNode->psLeftNode = CreateFilerEncodingNode();

                    psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                    if ( psXMLNode->psChild->psChild)
                      psFilterNode->psLeftNode->pszValue = 
                        strdup(psXMLNode->psChild->psChild->pszValue);

                    psFilterNode->psRightNode = CreateFilerEncodingNode();

                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
                    if (psXMLNode->psChild->psNext->psChild)
                      psFilterNode->psRightNode->pszValue = 
                        strdup(psXMLNode->psChild->psNext->psChild->pszValue);
                }
            }
/* -------------------------------------------------------------------- */
/*      PropertyIsBetween filter : extract property name and boudary    */
/*      values. The boundary  values are stored in the right            */
/*      node. The values are separated by a semi-column (;)             */
/*      Eg of Filter :                                                  */
/*      <PropertyIsBetween>                                             */
/*         <PropertyName>DEPTH</PropertyName>                           */
/*         <LowerBoundary>400</LowerBoundary>                           */
/*         <UpperBoundary>800</UpperBoundary>                           */
/*      </PropertyIsBetween>                                            */
/* -------------------------------------------------------------------- */
            else if (strcasecmp(psXMLNode->pszValue, "PropertyIsBetween") == 0)
            {
                if (psXMLNode->psChild &&
                    strcasecmp(psXMLNode->psChild->pszValue, 
                               "PropertyName") == 0 &&
                    psXMLNode->psChild->psNext &&  
                      strcasecmp(psXMLNode->psChild->psNext->pszValue, 
                               "LowerBoundary") == 0 &&
                    psXMLNode->psChild->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->pszValue, 
                                "UpperBoundary") == 0)
                {
                    psFilterNode->psLeftNode = CreateFilerEncodingNode();
                    psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                    if (psXMLNode->psChild->psChild)
                      psFilterNode->psLeftNode->pszValue = 
                        strdup(psXMLNode->psChild->psChild->pszValue);

                    psFilterNode->psRightNode = CreateFilerEncodingNode();
                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BOUNDARY;
                    if (psXMLNode->psChild->psNext->psChild &&
                        psXMLNode->psChild->psNext->psNext->psChild)
                    {
                        nStrLength = 
                          strlen(psXMLNode->psChild->psNext->psChild->pszValue);
                        nStrLength +=
                          strlen(psXMLNode->psChild->psNext->psNext->
                                 psChild->pszValue);

                        nStrLength += 2; //adding a ; between bounary values
                         psFilterNode->psRightNode->pszValue = 
                           (char *)malloc(sizeof(char)*(nStrLength));
                         strcpy( psFilterNode->psRightNode->pszValue,
                                 psXMLNode->psChild->psNext->psChild->pszValue);
                         strcat(psFilterNode->psRightNode->pszValue, ";");
                         strcat(psFilterNode->psRightNode->pszValue,
                                psXMLNode->psChild->psNext->psNext->psChild->pszValue); 
                         psFilterNode->psRightNode->pszValue[nStrLength-1]='\0';
                    }
                         
                  
                }
            }//end of PropertyIsBetween 
/* -------------------------------------------------------------------- */
/*      PropertyIsLike                                                  */
/* -------------------------------------------------------------------- */
            else if (strcasecmp(psXMLNode->pszValue, "PropertyIsLike") == 0)
            {
                //TODO
            }
        }
            
    }
}

 
/************************************************************************/
/*            int IsLogicalFilterType((char *pszValue)                  */
/*                                                                      */
/*      return TRUE if the value of the node is of logical filter       */
/*       encoding type.                                                 */
/************************************************************************/
int IsLogicalFilterType(char *pszValue)
{
    if (pszValue)
    {
        if (strcasecmp(pszValue, "AND") == 0 ||
            strcasecmp(pszValue, "OR") == 0 ||
             strcasecmp(pszValue, "NOT") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*         int IsBinaryComparisonFilterType(char *pszValue)             */
/*                                                                      */
/*      Binary comparison filter type.                                  */
/************************************************************************/
int IsBinaryComparisonFilterType(char *pszValue)
{
    if (pszValue)
    {
         if (strcasecmp(pszValue, "PropertyIsEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsNotEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsLessThan") == 0 ||  
             strcasecmp(pszValue, "PropertyIsGreaterThan") == 0 ||  
             strcasecmp(pszValue, "PropertyIsLessThanOrEqulaTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsGreaterThanOrEqulaTo") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*            int IsComparisonFilterType(char *pszValue)                */
/*                                                                      */
/*      return TRUE if the value of the node is of comparison filter    */
/*      encoding type.                                                  */
/************************************************************************/
int IsComparisonFilterType(char *pszValue)
{
    if (pszValue)
    {
         if (IsBinaryComparisonFilterType(pszValue) ||  
             strcasecmp(pszValue, "PropertyIsLike") == 0 ||  
             strcasecmp(pszValue, "PropertyIsBetween") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}


/************************************************************************/
/*            int IsSpatialFilterType(char *pszValue)                   */
/*                                                                      */
/*      return TRUE if the value of the node is of spatial filter       */
/*      encoding type.                                                  */
/************************************************************************/
int IsSpatialFilterType(char *pszValue)
{
    if (pszValue)
    {
        if ( strcasecmp(pszValue, "BBOX") == 0)
          return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*           int IsSupportedFilterType(CPLXMLNode *psXMLNode)           */
/*                                                                      */
/*      Verfify if the value of the node is one of the supported        */
/*      filter type.                                                    */
/************************************************************************/
int IsSupportedFilterType(CPLXMLNode *psXMLNode)
{
    if (psXMLNode)
    {
        if (IsLogicalFilterType(psXMLNode->pszValue) ||
            IsSpatialFilterType(psXMLNode->pszValue) ||
            IsComparisonFilterType(psXMLNode->pszValue))
          return MS_TRUE;
    }

    return MS_FALSE;
}
            
            

/************************************************************************/
/*                          GetMapserverExpression                      */
/*                                                                      */
/*      Return a mapserver expression base on the Filer encoding nodes. */
/************************************************************************/
char *GetMapserverExpression(FilterEncodingNode *psFilterNode)
{
    char *pszExpression = NULL;

    if (!psFilterNode)
      return NULL;

    if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON)
    {
        if ( psFilterNode->psLeftNode && psFilterNode->psRightNode)
        {
            if (IsBinaryComparisonFilterType(psFilterNode->pszValue))
            {
                pszExpression = GetBinaryComparisonExpresssion(psFilterNode);
            }
        }
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL)
    {
        if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
            strcasecmp(psFilterNode->pszValue, "OR") == 0)
        {
            pszExpression = GetLogicalComparisonExpresssion(psFilterNode);
        }
    }
            
    return pszExpression;
}
  
/************************************************************************/
/*                            GetNodeExpression                         */
/*                                                                      */
/*      Return the expresion for a specific node.                       */
/************************************************************************/
char *GetNodeExpression(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode)
      return NULL;

    if (IsLogicalFilterType(psFilterNode->pszValue))
      return GetLogicalComparisonExpresssion(psFilterNode);
    else if (IsBinaryComparisonFilterType(psFilterNode->pszValue))
      return GetBinaryComparisonExpresssion(psFilterNode);
    else
      return NULL;
}


/************************************************************************/
/*                     GetLogicalComparisonExpresssion                  */
/*                                                                      */
/*      Return the expression for logical comparison expression.        */
/************************************************************************/
char *GetLogicalComparisonExpresssion(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];
    char *pszTmp = NULL;
    szBuffer[0] = '\0';

    if (!psFilterNode || !IsLogicalFilterType(psFilterNode->pszValue))
      return NULL;

    if (psFilterNode->psLeftNode && psFilterNode->psRightNode)
    {
        strcat(szBuffer, " (");
        pszTmp = GetNodeExpression(psFilterNode->psLeftNode);
        if (!pszTmp)
          return NULL;

        strcat(szBuffer, pszTmp);
        strcat(szBuffer, " ");
        strcat(szBuffer, psFilterNode->pszValue);
        strcat(szBuffer, " ");
        pszTmp = GetNodeExpression(psFilterNode->psRightNode);
        if (!pszTmp)
          return NULL;
        strcat(szBuffer, pszTmp);
        strcat(szBuffer, ") ");
    }

    return strdup(szBuffer);
    
}

/************************************************************************/
/*                      GetBinaryComparisonExpresssion                  */
/*                                                                      */
/*      Return the expression for a binary comparison filter node.      */
/************************************************************************/
char *GetBinaryComparisonExpresssion(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];

    szBuffer[0] = '\0';
    if (!psFilterNode || !IsBinaryComparisonFilterType(psFilterNode->pszValue))
      return NULL;

    strcat(szBuffer, " (");
    //attribute
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);
    strcat(szBuffer, " ");
    
    //logical operator
    if (strcasecmp(psFilterNode->pszValue, 
                   "PropertyIsEqualTo") == 0)
      strcat(szBuffer, "=");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsNotEqualTo") == 0)
      strcat(szBuffer, "!="); //TODO verify that != is supported
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThan") == 0)
      strcat(szBuffer, "<");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThan") == 0)
      strcat(szBuffer, ">");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThanOrEqulaTo") == 0)
      strcat(szBuffer, "<=");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThanOrEqulaTo") == 0)
      strcat(szBuffer, ">=");
    
    strcat(szBuffer, " ");
    
    //value
    strcat(szBuffer, psFilterNode->psRightNode->pszValue);
    strcat(szBuffer, ") ");
    
    return strdup(szBuffer);
}

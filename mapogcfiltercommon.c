/**********************************************************************
 * $Id: mapogcfilter.c 10508 2010-09-17 20:52:33Z assefa $
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

#ifdef USE_OGR
#include "cpl_minixml.h"
#endif

#include "mapogcfilter.h"
#include "mapserver.h"
#include "mapowscommon.h"

MS_CVSID("$Id: $")

#ifdef USE_OGR

static int FTLParseEpsgString(char *pszEpsg, projectionObj *psProj)
{
    int nStatus = MS_FALSE;
    int nTokens = 0;
    char **tokens = NULL;
    int nEpsgTmp=0;

#ifdef USE_PROJ
    if (pszEpsg && psProj)
    {
        nTokens = 0;
        tokens = msStringSplit(pszEpsg,'#', &nTokens);
        if (tokens && nTokens == 2)
        {
            char szTmp[32];
            sprintf(szTmp, "init=epsg:%s",tokens[1]);
            msInitProjection(psProj);
            if (msLoadProjectionString(psProj, szTmp) == 0)
              nStatus = MS_TRUE;
        }
        else if (tokens &&  nTokens == 1)
        {
            if (tokens)
              msFreeCharArray(tokens, nTokens);
            nTokens = 0;

            tokens = msStringSplit(pszEpsg,':', &nTokens);
            nEpsgTmp = -1;
            if (tokens &&  nTokens == 1)
            {
                nEpsgTmp = atoi(tokens[0]);
                
            }
            else if (tokens &&  nTokens == 2)
            {
                nEpsgTmp = atoi(tokens[1]);
            }
            if (nEpsgTmp > 0)
            {
                char szTmp[32];
                sprintf(szTmp, "init=epsg:%d",nEpsgTmp);
                msInitProjection(psProj);
                if (msLoadProjectionString(psProj, szTmp) == 0)
                  nStatus = MS_TRUE;
            }
        }
        if (tokens)
          msFreeCharArray(tokens, nTokens);
    }
#endif
    return nStatus;
}

char *FLTGetIsBetweenComparisonCommonExpresssion(FilterEncodingNode *psFilterNode,
                                                 layerObj *lp)
{
    const size_t bufferSize = 1024;
    char szBuffer[1024];
    char **aszBounds = NULL;
    int nBounds = 0;
    int bString=0;
    char *pszExpression=NULL;

    if (!psFilterNode ||
        !(strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0))
      return NULL;

    if (psFilterNode->psLeftNode == NULL || psFilterNode->psRightNode == NULL )
      return NULL;

/* -------------------------------------------------------------------- */
/*      Get the bounds value which are stored like boundmin;boundmax    */
/* -------------------------------------------------------------------- */
    aszBounds = msStringSplit(psFilterNode->psRightNode->pszValue, ';', &nBounds);
    if (nBounds != 2)
    {
        if (aszBounds)
          msFreeCharArray(aszBounds, nBounds);
        return NULL;
    }
/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (aszBounds[0])
    {
        snprintf(szBuffer,  bufferSize, "%s_type",  psFilterNode->psLeftNode->pszValue);
        if (msOWSLookupMetadata(&(lp->metadata), "OFG", szBuffer) != NULL &&
            (strcasecmp(msOWSLookupMetadata(&(lp->metadata), "G", szBuffer), "Character") == 0))
          bString = 1;
        else if (FLTIsNumeric(aszBounds[0]) == MS_FALSE)    
          bString = 1;
    }
    if (!bString)
    {
        if (aszBounds[1])
        {
            if (FLTIsNumeric(aszBounds[1]) == MS_FALSE)    
              bString = 1;  
        }
    }
        

/* -------------------------------------------------------------------- */
/*      build expresssion.                                              */
/* -------------------------------------------------------------------- */
    /* attribute */
    if (bString)
      sprintf(szBuffer, "%s", " (\"[");
    else
      sprintf(szBuffer, "%s", " ([");
    pszExpression = msStringConcatenate(pszExpression, szBuffer);
    pszExpression = msStringConcatenate(pszExpression, psFilterNode->psLeftNode->pszValue);

    if (bString)
      sprintf(szBuffer, "%s", "]\" ");
    else
      sprintf(szBuffer, "%s", "] ");
    pszExpression = msStringConcatenate(pszExpression, szBuffer);    
    
    sprintf(szBuffer, "%s", " >= ");
    pszExpression = msStringConcatenate(pszExpression, szBuffer); 

    if (bString)
    {
        sprintf(szBuffer,"%s", "\"");
        pszExpression = msStringConcatenate(pszExpression, szBuffer); 
    }

    snprintf(szBuffer, bufferSize, "%s", aszBounds[0]);
    pszExpression = msStringConcatenate(pszExpression, szBuffer); 
    if (bString)
    {
        sprintf(szBuffer, "%s", "\"");
        pszExpression = msStringConcatenate(pszExpression, szBuffer); 
    }

    sprintf(szBuffer, "%s", " AND ");
    pszExpression = msStringConcatenate(pszExpression, szBuffer); 

    if (bString)
      sprintf(szBuffer, "%s", " \"[");
    else
      sprintf(szBuffer, "%s", " ["); 
    pszExpression = msStringConcatenate(pszExpression, szBuffer); 
     
    /* attribute */
    pszExpression = msStringConcatenate(pszExpression, psFilterNode->psLeftNode->pszValue);
    
    if (bString)
      sprintf(szBuffer, "%s", "]\" ");
    else
      sprintf(szBuffer, "%s", "] ");
    pszExpression = msStringConcatenate(pszExpression, szBuffer); 

    sprintf(szBuffer, "%s", " <= ");
    pszExpression = msStringConcatenate(pszExpression, szBuffer); 
    if (bString)
    {
        sprintf(szBuffer,"%s", "\"");
        pszExpression = msStringConcatenate(pszExpression, szBuffer); 
    }
    snprintf(szBuffer, bufferSize, "%s", aszBounds[1]);
    pszExpression = msStringConcatenate(pszExpression, szBuffer); 

    if (bString)
    {
        sprintf(szBuffer,"\"");
        pszExpression = msStringConcatenate(pszExpression, szBuffer); 
    }
    sprintf(szBuffer, "%s", ")");     
    pszExpression = msStringConcatenate(pszExpression, szBuffer); 
    
    if (aszBounds)
      msFreeCharArray(aszBounds, nBounds);

    return pszExpression;
}

char *FLTGetBinaryComparisonCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
     char szTmp[1024];
     char *pszExpression = NULL;
     int bString;

     if (psFilterNode == NULL)
      return NULL;

/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (psFilterNode->psRightNode->pszValue)
    {
        snprintf(szTmp, sizeof(szTmp), "%s_type",  psFilterNode->psLeftNode->pszValue);
        if (msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp) != NULL &&
            (strcasecmp(msOWSLookupMetadata(&(lp->metadata), "G", szTmp), "Character") == 0))
          bString = 1;
        else if (FLTIsNumeric(psFilterNode->psRightNode->pszValue) == MS_FALSE)    
          bString = 1;
    }

    /* specical case to be able to have empty strings in the expression. */
    /*propertyislike is always treated as string*/
    if (psFilterNode->psRightNode->pszValue == NULL || 
        strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
      bString = 1;
    
    
      
    /* attribute */
    if (bString)
      sprintf(szTmp, "%s", " (\"[");
    else
      sprintf(szTmp,  "%s"," ([");
    pszExpression = msStringConcatenate(pszExpression, szTmp);
    pszExpression = msStringConcatenate(pszExpression, psFilterNode->psLeftNode->pszValue);
    if (bString)
      sprintf(szTmp,  "%s","]\" ");
    else
      sprintf(szTmp,  "%s", "] "); 
    pszExpression = msStringConcatenate(pszExpression, szTmp);

    if (strcasecmp(psFilterNode->pszValue, 
                   "PropertyIsEqualTo") == 0)
    {
        /*case insensitive set ? */
        if (psFilterNode->psRightNode->pOther && 
            (*(int *)psFilterNode->psRightNode->pOther) == 1)
        {
          sprintf(szTmp,  "%s", "=="); /*TODO what should this be ??*/
        }
        else
          sprintf(szTmp,  "%s", "==");
    
    }
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsNotEqualTo") == 0)
      sprintf(szTmp,  "%s", " != "); 
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThan") == 0)
      sprintf(szTmp,  "%s", " < ");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThan") == 0)
      sprintf(szTmp,  "%s", " > ");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThanOrEqualTo") == 0)
      sprintf(szTmp,  "%s", " <= ");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThanOrEqualTo") == 0)
      sprintf(szTmp,  "%s", " >= ");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLike") == 0)
      sprintf(szTmp,  "%s", " ~* ");
    
    pszExpression = msStringConcatenate(pszExpression, szTmp);

    /* value */
    if (bString)
    {
        sprintf(szTmp,  "%s", "\"");
        pszExpression = msStringConcatenate(pszExpression, szTmp);
    }

    if (psFilterNode->psRightNode->pszValue)
      pszExpression = msStringConcatenate(pszExpression, psFilterNode->psRightNode->pszValue);

    if (bString)
    {
        sprintf(szTmp,  "%s", "\"");
        pszExpression = msStringConcatenate(pszExpression, szTmp);
    }

     sprintf(szTmp,  "%s", ")");
     pszExpression = msStringConcatenate(pszExpression, szTmp);

    return pszExpression;
}



char *FLTGetLogicalComparisonCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
    char *pszExpression = NULL;
    char *pszTmp = NULL;
    char szBuffer[256];

    if (!psFilterNode || !FLTIsLogicalFilterType(psFilterNode->pszValue))
      return NULL;

/* -------------------------------------------------------------------- */
/*      OR and AND                                                      */
/* -------------------------------------------------------------------- */
    if (psFilterNode->psLeftNode && psFilterNode->psRightNode)
    {
        pszTmp = FLTGetCommonExpression(psFilterNode->psLeftNode, lp);
        if (!pszTmp)
          return NULL;

        sprintf(szBuffer, "%s", " (");
        pszExpression = msStringConcatenate(pszExpression, szBuffer);
        
        pszExpression = msStringConcatenate(pszExpression, pszTmp);
        msFree(pszTmp);
        
        sprintf(szBuffer, "%s", " ");
        pszExpression = msStringConcatenate(pszExpression, szBuffer);

        pszExpression = msStringConcatenate(pszExpression, psFilterNode->pszValue);
        sprintf(szBuffer, "%s", " ");


        pszTmp = FLTGetCommonExpression(psFilterNode->psRightNode, lp);
        if (!pszTmp)
          return NULL;

        pszExpression = msStringConcatenate(pszExpression, pszTmp);
        msFree(pszTmp);

        sprintf(szBuffer, "%s", ") ");
        pszExpression = msStringConcatenate(pszExpression, szBuffer);
    }
/* -------------------------------------------------------------------- */
/*      NOT                                                             */
/* -------------------------------------------------------------------- */
    else if (psFilterNode->psLeftNode && 
             strcasecmp(psFilterNode->pszValue, "NOT") == 0)
    {
        pszTmp = FLTGetCommonExpression(psFilterNode->psLeftNode, lp);
        if (!pszTmp)
          return NULL;

        sprintf(szBuffer, "%s", " (NOT ");
        pszExpression = msStringConcatenate(pszExpression, szBuffer);

        pszExpression = msStringConcatenate(pszExpression, pszTmp);
        msFree(pszTmp);

        sprintf(szBuffer, "%s", ") ");
        pszExpression = msStringConcatenate(pszExpression, szBuffer);
        
    }

    
    return pszExpression;
}


char *FLTGetSpatialComparisonCommonExpression(FilterEncodingNode *psNode, layerObj *lp)
{
    char *pszExpression = NULL;
    shapeObj *psQueryShape = NULL;
    double dfDistance = -1;
    int nUnit = -1;
    char *pszWktText = NULL;
    char szBuffer[256];
    char *pszTmp=NULL;
    projectionObj sProjTmp;
    char *pszEPSG= NULL;
    rectObj sQueryRect;
    shapeObj *psTmpShape=NULL, *psBufferShape=NULL;
    int bBBoxQuery = 0;
    
    if (psNode == NULL || lp == NULL)
      return NULL;

    if (psNode->eType != FILTER_NODE_TYPE_SPATIAL)
      return NULL;

    /* get the shape*/
    /*BBOX case: replace it with NOT DISJOINT.*/
    if(FLTIsBBoxFilter(psNode))
    {
        pszEPSG = FLTGetBBOX(psNode, &sQueryRect);
        /*this should be removed and bbox should parse and strore the srs properly, 
          using now legacy code*/
        if (pszEPSG)
          psNode->pszSRS = msStrdup(pszEPSG);
        
        psTmpShape = (shapeObj *)malloc(sizeof(shapeObj));
        msInitShape(psTmpShape);
        msRectToPolygon(sQueryRect, psTmpShape);
        bBBoxQuery = 1;

    }
    else
    {
        /*other geos type operations*/

        /*project shape to layer projection. If the proj is not part of the filter query,
          assume that the cooredinates are in the map projection*/
      
        psQueryShape = FLTGetShape(psNode, &dfDistance, &nUnit);
    
        if ((strcasecmp(psNode->pszValue, "DWithin") == 0 ||
             strcasecmp(psNode->pszValue, "Beyond") == 0 ) &&
            dfDistance > 0)
        {
            if (nUnit >=0 && nUnit != lp->map->units)
              dfDistance *= msInchesPerUnit(nUnit,0)/msInchesPerUnit(lp->map->units,0);

            psBufferShape = msGEOSBuffer(psQueryShape, dfDistance);
        }
        if (psBufferShape)
          psTmpShape = psBufferShape;
        else
          psTmpShape = psQueryShape;
    }

    if (psTmpShape)
    {
        if( lp->projection.numargs > 0)
        {
            if (psNode->pszSRS && FTLParseEpsgString(psNode->pszSRS, &sProjTmp))
              msProjectShape(&sProjTmp, &lp->projection, psTmpShape);
            else if (lp->map->projection.numargs > 0)
              msProjectShape(&lp->map->projection, &lp->projection, psTmpShape);
        }
        if (bBBoxQuery)
          sprintf(szBuffer, "%s", " (NOT ([shape] ");
        else
          sprintf(szBuffer, "%s", " ([shape] ");

        pszExpression = msStringConcatenate(pszExpression, szBuffer);


        if (strcasecmp(psNode->pszValue, "intersect") == 0)
          pszTmp = msStrdup(psNode->pszValue);
        else
          pszTmp = msStrdup(psNode->pszValue);
        msStringToLower(pszTmp);
        if (bBBoxQuery)
          sprintf(szBuffer, " %s ", "disjoint");
        else
           sprintf(szBuffer, " %s ", pszTmp);

        pszExpression = msStringConcatenate(pszExpression, szBuffer);
        msFree(pszTmp);

        pszWktText = msGEOSShapeToWKT(psTmpShape);
        sprintf(szBuffer, "%s", " fromText('");
        pszExpression = msStringConcatenate(pszExpression, szBuffer);
        pszExpression = msStringConcatenate(pszExpression, pszWktText);
        sprintf(szBuffer, "%s", "')");
        pszExpression = msStringConcatenate(pszExpression, szBuffer);
        
    }
    if (psBufferShape)
      msFreeShape(psBufferShape);

    
    sprintf(szBuffer, "%s", ")");
    pszExpression = msStringConcatenate(pszExpression, szBuffer);

     if (bBBoxQuery)
     {
          sprintf(szBuffer, "%s", ")");
          pszExpression = msStringConcatenate(pszExpression, szBuffer);
     }
    return pszExpression;
}

char *FLTGetFeatureIdCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
    
    char *pszExpression = NULL;
    int nTokens = 0, i=0, bString=0;
    char **tokens = NULL;
    const char *pszAttribute=NULL;

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)
    if (psFilterNode->pszValue)
    {
        pszAttribute = msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
        if (pszAttribute)
        {
            tokens = msStringSplit(psFilterNode->pszValue,',', &nTokens);
            if (tokens && nTokens > 0)
            {
                for (i=0; i<nTokens; i++)
                {   
                    char *pszTmp = NULL;
                    int bufferSize = 0;	

                    if (i == 0)
                    {
                        if(FLTIsNumeric(tokens[0]) == MS_FALSE) 
                          bString = 1;
                    }
                    
                   
                    if (bString)
                    {   
                        bufferSize = 11+strlen(tokens[i])+strlen(pszAttribute)+1;
                        pszTmp = (char *)malloc(bufferSize);
                        snprintf(pszTmp, bufferSize, "(\"[%s]\" ==\"%s\")" , pszAttribute, tokens[i]);
                    }
                    else
                    {   
                        bufferSize = 7+strlen(tokens[i])+strlen(pszAttribute)+1;
                        pszTmp = (char *)malloc(bufferSize);
                        snprintf(pszTmp, bufferSize, "([%s] == %s)" , pszAttribute, tokens[i]);
                    }

                    if (pszExpression != NULL)
                      pszExpression = msStringConcatenate(pszExpression, " OR ");
                    else
                      pszExpression = msStringConcatenate(pszExpression, "(");
                    pszExpression = msStringConcatenate(pszExpression, pszTmp);
                    msFree(pszTmp);
                }

                msFreeCharArray(tokens, nTokens);
            }
        }   
        /*opening and closing brackets are needed for mapserver expressions*/
        if (pszExpression)
          pszExpression = msStringConcatenate(pszExpression, ")");
    }
#endif

    return pszExpression;
}

char *FLTGetCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
    char *pszExpression = NULL;

    if (!psFilterNode)
      return NULL;

    if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON)
    {
        if ( psFilterNode->psLeftNode && psFilterNode->psRightNode)
        {
            if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue) ||
                strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
              pszExpression = FLTGetBinaryComparisonCommonExpression(psFilterNode, lp);

            else if (strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0)
              pszExpression = FLTGetIsBetweenComparisonCommonExpresssion(psFilterNode, lp);
        }
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL)
      pszExpression = FLTGetLogicalComparisonCommonExpression(psFilterNode, lp);

    else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL)
      pszExpression = FLTGetSpatialComparisonCommonExpression(psFilterNode, lp);

    else if (psFilterNode->eType ==  FILTER_NODE_TYPE_FEATUREID)
      pszExpression = FLTGetFeatureIdCommonExpression(psFilterNode, lp);

    return pszExpression;
}


int FLTApplyFilterToLayerCommonExpression(mapObj *map, int iLayerIndex, char *pszExpression)
{
    int retval;

    msFreeQuery(&(map->query));
    msInitQuery(&(map->query));
    
    map->query.type = MS_QUERY_BY_FILTER;

    map->query.filter = (expressionObj *) malloc(sizeof(expressionObj));
    initExpression( map->query.filter);
    map->query.filter->string = msStrdup(pszExpression);
    map->query.filter->type = 2000;
    map->query.layer = iLayerIndex;

    /*TODO: if there is a bbox in the node, get it and set the map extent*/
    map->query.rect = map->extent;

    retval = msQueryByFilter(map);
    
    return retval;
}

#endif

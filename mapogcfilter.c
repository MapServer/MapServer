/**********************************************************************
 * $Id$
 *
 * Name:     mapogcfilter.c
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
 * Revision 1.44  2005/02/21 20:37:55  assefa
 * Initialize buffer properly (Bug 1252).
 *
 * Revision 1.43  2005/02/18 03:06:46  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.42  2005/01/28 06:16:54  sdlime
 * Applied patch to make function prototypes ANSI C compliant. Thanks to Petter Reinholdtsen. This fixes but 1181.
 *
 * Revision 1.41  2005/01/06 00:33:49  assefa
 * bug 1143 : missing call to msInitShape.
 *
 * Revision 1.40  2004/12/07 15:31:51  assefa
 * Change the output of the expression when using a wild card for
 * PropertyIsLike (Bug 1107).
 *
 * Revision 1.39  2004/11/22 14:56:53  dan
 * Added missing argument to msEvalContext()
 *
 * Revision 1.38  2004/10/28 23:25:18  assefa
 * Return exception when manadatory elements are missing from the filter (Bug 935)
 *
 * Revision 1.37  2004/10/25 20:03:27  assefa
 * Do not use layer tolerance for msQueryByShape (Bug 768).
 *
 * Revision 1.36  2004/10/25 17:55:16  assefa
 * Correct bug when testing for string values in filter ProperyIsBetween (Bug 464).
 *
 * Revision 1.35  2004/10/22 15:39:18  assefa
 * Correct PropertyIsLike bug (Bug 987).
 *
 * Revision 1.34  2004/10/21 04:30:56  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 * Revision 1.33  2004/10/09 18:22:41  sean
 * towards resolving bug 339, have implemented a mutex acquiring wrapper for
 * the loadExpressionString function.  the new msLoadExpressionString should be
 * used everywhere outside of the mapfile loading phase, and the previous
 * loadExpressionString function should be used within the mapfile loading
 * phase.
 *
 * Revision 1.32  2004/09/28 15:50:14  assefa
 * Correct bug related to gml bbox (Bug 913).
 *
 * Revision 1.31  2004/09/17 16:31:38  assefa
 * Correct bug with spatial filters Dwithin and Intersect.
 *
 * Revision 1.30  2004/08/17 17:42:04  assefa
 * Correct bug in FLTGetQueryResultsForNode.
 *
 * Revision 1.29  2004/07/29 21:50:19  assefa
 * Use wfs_filter metedata when generating an SLD (Bug 782)
 *
 * Revision 1.28  2004/07/28 22:16:16  assefa
 * Add support for spatial filters inside an SLD. (Bug 782).
 *
 * Revision 1.27  2004/05/11 12:57:29  assefa
 * Correct bug when testing if an attribute value is a string.
 *
 * Revision 1.26  2004/02/26 14:48:03  frank
 * Avoid warnings by pre-include cpl_minixml.h before ogr_api.h is included.
 *
 * Revision 1.25  2004/02/20 22:18:29  assefa
 * Make sure that arrays are  sorted at th end of the And/Or functions.
 *
 * Revision 1.24  2004/02/20 00:58:08  assefa
 * The layer->template was not set in some cases.
 *
 * Revision 1.23  2004/02/11 00:06:51  assefa
 * Correct Bug in FLTArraysNot when passed array was empty.
 *
 * Revision 1.22  2004/02/10 23:46:39  assefa
 * Coorect typo error.
 *
 * Revision 1.21  2004/02/10 23:44:00  assefa
 * Set layer template before doing a query for the NOT operator.
 * Correct a bug when doing an OR on 2 arrays.
 *
 * Revision 1.20  2004/02/05 20:01:58  assefa
 * Set layer tolernace when using query by shape.
 *
 * Revision 1.19  2004/02/05 19:19:20  assefa
 * strip names spaces ogc and gml from the xml string.
 *
 * Revision 1.18  2004/02/04 21:20:36  assefa
 * Add Intersect in the list of supported operators.
 *
 * Revision 1.17  2004/02/04 19:58:00  assefa
 * Remove unused variables.
 *
 * Revision 1.16  2004/02/04 19:46:24  assefa
 * Add support for multiple spatial opertaors inside one filter.
 * Add support for opeartors DWithin and Intersect.
 *
 * Revision 1.15  2004/01/13 19:33:10  assefa
 * Correct in bug when builing expression for the IsLIke operator.
 *
 * Revision 1.14  2004/01/05 21:16:26  assefa
 * Correct bug with PropertyIsLike and a BBOX filters.
 *
 * Revision 1.13  2003/10/22 13:19:51  assefa
 * Change delimiter from single quote to double quote when buidging expressions.
 *
 * Revision 1.12  2003/10/07 23:54:24  assefa
 * Additional Validation for propertyislike.
 *
 * Revision 1.11  2003/09/30 15:58:16  assefa
 * IsBetween filter can have a <literal> node before value.
 *
 * Revision 1.10  2003/09/29 20:40:56  assefa
 * Remove brackets when It is a logical expression.
 *
 * Revision 1.9  2003/09/29 18:24:27  assefa
 * Add test if value is valid in FLTGetMapserverExpressionClassItem.
 *
 * Revision 1.8  2003/09/29 14:19:06  assefa
 * Correct srsname extraction from the Box element.
 *
 * Revision 1.7  2003/09/29 12:52:45  assefa
 * Correct string comparison for BBox.
 *
 * Revision 1.6  2003/09/29 01:00:14  assefa
 * Look for string Filter instead of <Filter since namespaces can be used.
 *
 * Revision 1.5  2003/09/26 13:44:40  assefa
 * Add support for gml box with 2 <coord> elements.
 *
 * Revision 1.4  2003/09/23 14:34:34  assefa
 * ifdef's for OGR use.
 *
 * Revision 1.3  2003/09/22 22:53:20  assefa
 * Add ifdef USE_OGR where the MiniMXL Parser is used.
 *
 * Revision 1.2  2003/09/19 21:55:54  assefa
 * Strip namespaces.
 *
 * Revision 1.1  2003/09/10 19:54:27  assefa
 * Renamed from fileterencoding.c/h
 *
 * Revision 1.5  2003/09/10 13:30:15  assefa
 * Correct test in IsBetween filter.
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
#include "cpl_minixml.h"
#endif

#include "mapogcfilter.h"
#include "map.h"

MS_CVSID("$Id$")

#ifdef USE_OGR

static int compare_ints( const void * a, const void * b)
{
    return (*(int*)a) - (*(int*)b);
}

int FLTogrConvertGeometry(OGRGeometryH hGeometry, shapeObj *psShape,
                          OGRwkbGeometryType nType) 
{
    return msOGRGeometryToShape(hGeometry, psShape, nType);
}

int FLTShapeFromGMLTree(CPLXMLNode *psTree, shapeObj *psShape)
{
    if (psTree && psShape)
    {
        CPLXMLNode *psNext = psTree->psNext;
        OGRGeometryH hGeometry = NULL;

        psTree->psNext = NULL;
        hGeometry = OGR_G_CreateFromGMLTree(psTree );
        psTree->psNext = psNext;

        if (hGeometry)
        {
            FLTogrConvertGeometry(hGeometry, psShape, 
                                  OGR_G_GetGeometryType(hGeometry));
        }
        return MS_TRUE;
    }

    return MS_FALSE;
}
/************************************************************************/
/*                        FLTGetQueryResultsForNode                     */
/*                                                                      */
/*      Return an array of shape id's selected after the filter has     */
/*      been applied.                                                   */
/*      Assuming here that the node is not a logical node but           */
/*      spatial or comparaison.                                         */
/************************************************************************/
int *FLTGetQueryResultsForNode(FilterEncodingNode *psNode, mapObj *map, 
                               int iLayerIndex, int *pnResults,
                               int bOnlySpatialFilter)
{
    char *szExpression = NULL;
    int bIsBBoxFilter =0, nEpsgTmp = 0, i=0;
    char *szEPSG = NULL;
    rectObj sQueryRect = map->extent;
    layerObj *lp = NULL;
    char *szClassItem = NULL;
    char **tokens = NULL;
    int nTokens = 0;
    projectionObj sProjTmp;
    int *panResults = NULL;
    int bPointQuery = 0, bShapeQuery=0;
    shapeObj *psQueryShape = NULL;
    double dfDistance = -1;
    double dfCurrentTolerance = 0;

    if (!psNode || !map || iLayerIndex < 0 ||
        iLayerIndex > map->numlayers-1)
      return NULL;
    
    if (!bOnlySpatialFilter)
      szExpression = FLTGetMapserverExpression(psNode);

    bIsBBoxFilter = FLTIsBBoxFilter(psNode);
    if (bIsBBoxFilter)
      szEPSG = FLTGetBBOX(psNode, &sQueryRect);
    else if ((bPointQuery = FLTIsPointFilter(psNode)))
    {
        psQueryShape = FLTGetShape(psNode, &dfDistance);
    }
    else if (FLTIsLineFilter(psNode) || FLTIsPolygonFilter(psNode))
    {
        bShapeQuery = 1;
        psQueryShape = FLTGetShape(psNode, NULL);
    }

    if (!szExpression && !szEPSG && !bIsBBoxFilter 
        && !bPointQuery && !bShapeQuery && (bOnlySpatialFilter == MS_FALSE))
      return NULL;

    lp = &(map->layers[iLayerIndex]);

    if (szExpression)
    {
        szClassItem = FLTGetMapserverExpressionClassItem(psNode);
                
        initClass(&(lp->class[0]));

        lp->class[0].type = lp->type;
        lp->numclasses = 1;
        msLoadExpressionString(&lp->class[0].expression, 
                                  szExpression);
/* -------------------------------------------------------------------- */
/*      classitems are necessary for filter type PropertyIsLike         */
/* -------------------------------------------------------------------- */
        if (szClassItem)
        {
            if (lp->classitem)
              free (lp->classitem);
            lp->classitem = strdup(szClassItem);

/* ==================================================================== */
/*      If there is a case where PorprtyIsLike is combined with an      */
/*      Or, then we need to create a second class with the              */
/*      PrpertyIsEqual expression. Note that the first expression       */
/*      returned does not include the IslikePropery.                    */
/* ==================================================================== */
            if (!FLTIsOnlyPropertyIsLike(psNode))
            {
                szExpression = 
                  FLTGetMapserverIsPropertyExpression(psNode);
                if (szExpression)
                {
                    initClass(&(lp->class[1]));
                    
                    lp->class[1].type = lp->type;
                    lp->numclasses++;
                    msLoadExpressionString(&lp->class[1].expression, 
                                         szExpression);
                    if (!lp->class[1].template)
                      lp->class[1].template = strdup("ttt.html");
                }
            }
        }

        if (!lp->class[0].template)
          lp->class[0].template = strdup("ttt.html");
/* -------------------------------------------------------------------- */
/*      Need to free the template so the all shapes's will not end      */
/*      up being queried. The query is dependent on the class           */
/*      expression.                                                     */
/* -------------------------------------------------------------------- */
        if (lp->template)
        {
            free(lp->template);
            lp->template = NULL;
        }
    }
    else if (!bOnlySpatialFilter)
    {
        /* if there are no expression (so no template set in the classes, */
        /* make sure that the layer is queryable by setting the template  */
        /* parameter */
        if (!lp->template)
          lp->template = strdup("ttt.html");
    }
/* -------------------------------------------------------------------- */
/*      Use the epsg code to reproject the values from the QueryRect    */
/*      to the map projection.                                          */
/*      The srs should be a string like                                 */
/*      srsName="http://www.opengis.net/gml/srs/epsg.xml#4326".         */
/*      We will just extract the value after the # and assume that      */
/*      It corresponds to the epsg code on the system. This syntax      */
/*      is the one descibed in the GML specification.                   */
/*                                                                      */
/*       There is also several servers requesting the box with the      */
/*      srsName using the following syntax : <Box                       */
/*      srsName="EPSG:42304">. So we also support this syntax.          */
/*                                                                      */
/* -------------------------------------------------------------------- */
    if(szEPSG && map->projection.numargs > 0)
    {
#ifdef USE_PROJ
        nTokens = 0;
        tokens = split(szEPSG,'#', &nTokens);
        if (tokens && nTokens == 2)
        {
            char szTmp[32];
            sprintf(szTmp, "init=epsg:%s",tokens[1]);
            msInitProjection(&sProjTmp);
            if (msLoadProjectionString(&sProjTmp, szTmp) == 0)
              msProjectRect(&map->projection, &sProjTmp, &sQueryRect);
        }
        else if (tokens &&  nTokens == 1)
        {
            if (tokens)
              msFreeCharArray(tokens, nTokens);
            nTokens = 0;

            tokens = split(szEPSG,':', &nTokens);
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
                msInitProjection(&sProjTmp);
                if (msLoadProjectionString(&sProjTmp, szTmp) == 0)
                  msProjectRect(&map->projection, &sProjTmp, &sQueryRect);
            }
        }
        if (tokens)
          msFreeCharArray(tokens, nTokens);
#endif
    }


    if (szExpression || bIsBBoxFilter || 
        (bOnlySpatialFilter && !bIsBBoxFilter && !bPointQuery && !bShapeQuery))
      msQueryByRect(map, lp->index, sQueryRect);
    else if (bPointQuery && psQueryShape && psQueryShape->numlines > 0
             && psQueryShape->line[0].numpoints > 0 && dfDistance >=0)
      msQueryByPoint(map, lp->index, MS_MULTIPLE, 
                     psQueryShape->line[0].point[0], dfDistance);
    else if (bShapeQuery && psQueryShape && psQueryShape->numlines > 0
             && psQueryShape->line[0].numpoints > 0)
    {
        /* disable any tolerance value already set for the layerm (Bug 768) */
        dfCurrentTolerance = lp->tolerance;
        lp->tolerance = 0;
        if (dfDistance > 0)
        {
            lp->tolerance = dfDistance;
        }
        msQueryByShape(map, lp->index,  psQueryShape);
      
        lp->tolerance = dfCurrentTolerance;
    }

    if (szExpression)
      free(szExpression);

    if (lp && lp->resultcache && lp->resultcache->numresults > 0)
    {
        panResults = (int *)malloc(sizeof(int)*lp->resultcache->numresults);
        for (i=0; i<lp->resultcache->numresults; i++)
          panResults[i] = lp->resultcache->results[i].shapeindex;

        qsort(panResults, lp->resultcache->numresults, 
              sizeof(int), compare_ints);

        *pnResults = lp->resultcache->numresults;
        
    }

    return panResults;

}
 
int FLTGML2Shape_XMLNode(CPLXMLNode *psNode, shapeObj *psShp)
{
    lineObj line={0,NULL};
    CPLXMLNode *psCoordinates = NULL;
    char *pszTmpCoord = NULL;
    char **szCoords = NULL;
    int nCoords = 0;


    if (!psNode || !psShp)
      return MS_FALSE;

    
    if( strcasecmp(psNode->pszValue,"PointType") == 0
        || strcasecmp(psNode->pszValue,"Point") == 0)
    {
        psCoordinates = CPLGetXMLNode(psNode, "coordinates");
        
        if (psCoordinates && psCoordinates->psChild && 
            psCoordinates->psChild->pszValue)
        {
            pszTmpCoord = psCoordinates->psChild->pszValue;
            szCoords = split(pszTmpCoord, ',', &nCoords);
            if (szCoords && nCoords >= 2)
            {
                line.numpoints = 1;
                line.point = (pointObj *)malloc(sizeof(pointObj));
                line.point[0].x = atof(szCoords[0]);
                line.point[0].y =  atof(szCoords[1]);
                line.point[0].m = 0;

                psShp->type = MS_SHAPE_POINT;

                msAddLine(psShp, &line);
                free(line.point);

                return MS_TRUE;
            }
        }
    }
    
    return MS_FALSE;
}



static int addResult(resultCacheObj *cache, int classindex, int shapeindex, int tileindex)
{
  int i;

  if(cache->numresults == cache->cachesize) { /* just add it to the end */
    if(cache->cachesize == 0)
      cache->results = (resultCacheMemberObj *) malloc(sizeof(resultCacheMemberObj)*MS_RESULTCACHEINCREMENT);
    else
      cache->results = (resultCacheMemberObj *) realloc(cache->results, sizeof(resultCacheMemberObj)*(cache->cachesize+MS_RESULTCACHEINCREMENT));
    if(!cache->results) {
      msSetError(MS_MEMERR, "Realloc() error.", "addResult()");
      return(MS_FAILURE);
    }   
    cache->cachesize += MS_RESULTCACHEINCREMENT;
  }

  i = cache->numresults;

  cache->results[i].classindex = classindex;
  cache->results[i].tileindex = tileindex;
  cache->results[i].shapeindex = shapeindex;
  cache->numresults++;

  return(MS_SUCCESS);
}
 


/************************************************************************/
/*                         FLTAddToLayerResultCache                     */
/*                                                                      */
/*      Utility function to add shaped to the layer result              */
/*      cache. This function is used with the WFS GetFeature            */
/*      interface and the cache will be used later on to output the     */
/*      result into gml (the only important thing here is the shape     */
/*      id).                                                            */
/************************************************************************/
void FLTAddToLayerResultCache(int *anValues, int nSize, mapObj *map, 
                              int iLayerIndex)
{
    layerObj *psLayer = NULL;
    int i = 0, status = MS_FALSE;
    shapeObj shape;
    int nClassIndex = -1;
    char        annotate=MS_TRUE;

    if (!anValues || nSize <= 0 || !map || iLayerIndex < 0 ||
        iLayerIndex > map->numlayers-1)
      return;

    psLayer = &(map->layers[iLayerIndex]);
    if (psLayer->resultcache)
    {
        if (psLayer->resultcache->results)
          free (psLayer->resultcache->results);
        free(psLayer->resultcache);
    }

    psLayer->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj));

    psLayer->resultcache->results = NULL;
    psLayer->resultcache->numresults =  0;
    psLayer->resultcache->cachesize = 0;
    psLayer->resultcache->bounds.minx = -1;
    psLayer->resultcache->bounds.miny = -1;
    psLayer->resultcache->bounds.maxx = -1;
    psLayer->resultcache->bounds.maxy = -1;

    status = msLayerOpen(psLayer);
    if (status != MS_SUCCESS) 
      return;
    annotate = msEvalContext(map, psLayer, psLayer->labelrequires);
    if(map->scale > 0) 
    {
        if((psLayer->labelmaxscale != -1) && (map->scale >= psLayer->labelmaxscale)) 
          annotate = MS_FALSE;
        if((psLayer->labelminscale != -1) && (map->scale < psLayer->labelminscale)) 
          annotate = MS_FALSE;
    }
    status = msLayerWhichItems(psLayer, MS_TRUE, annotate, NULL);
    if (status != MS_SUCCESS) 
      return;

    for (i=0; i<nSize; i++)
    {
        msInitShape(&shape);
        status = msLayerGetShape(psLayer, &shape, -1, anValues[i]);
        if (status != MS_SUCCESS)
          nClassIndex = -1;
        else
          nClassIndex = msShapeGetClass(psLayer, &shape, map->scale);
        
        addResult(psLayer->resultcache, nClassIndex, anValues[i], -1);

        if(psLayer->resultcache->numresults == 1)
          psLayer->resultcache->bounds = shape.bounds;
        else
          msMergeRect(&(psLayer->resultcache->bounds), &shape.bounds);
    }


}

/************************************************************************/
/*                               FTLIsInArray                           */
/*                                                                      */
/*      Verify if a certain value is inside an ordered array.           */
/************************************************************************/
int FTLIsInArray(int *panArray, int nSize, int nValue)
{
    int i = 0;
    if (panArray && nSize > 0)
    {
        for (i=0; i<nSize; i++)
        {
            if (panArray[i] == nValue)
              return MS_TRUE;
            if (panArray[i] > nValue)
              return MS_FALSE;
        }
    }

    return MS_FALSE;
}




/************************************************************************/
/*                               FLTArraysNot                           */
/*                                                                      */
/*      Return the list of all shape id's in a layer that are not in    */
/*      the array.                                                      */
/************************************************************************/
int *FLTArraysNot(int *panArray, int nSize, mapObj *map,
                  int iLayerIndex, int *pnResult)
{
    layerObj *psLayer = NULL;
    int *panResults = NULL;
    int *panTmp = NULL;
    int i = 0, iResult = 0;
    
    if (!map || iLayerIndex < 0 || iLayerIndex > map->numlayers-1)
      return NULL;

     psLayer = &(map->layers[iLayerIndex]);
     if (psLayer->template == NULL)
       psLayer->template = strdup("ttt.html");

     msQueryByRect(map, psLayer->index, map->extent);

     free(psLayer->template);
     psLayer->template = NULL;

     if (!psLayer->resultcache || psLayer->resultcache->numresults <= 0)
       return NULL;

     panResults = (int *)malloc(sizeof(int)*psLayer->resultcache->numresults);
     
     panTmp = (int *)malloc(sizeof(int)*psLayer->resultcache->numresults);
     for (i=0; i<psLayer->resultcache->numresults; i++)
       panTmp[i] = psLayer->resultcache->results[i].shapeindex;
     qsort(panTmp, psLayer->resultcache->numresults, 
           sizeof(int), compare_ints);
     
     
     iResult = 0;
      for (i=0; i<psLayer->resultcache->numresults; i++)
      {
          if (!FTLIsInArray(panArray, nSize, panTmp[i]) || 
              panArray == NULL)
            panResults[iResult++] = 
              psLayer->resultcache->results[i].shapeindex;
      }

      free(panTmp);

      if (iResult > 0)
      {
          panResults = (int *)realloc(panResults, sizeof(int)*iResult);
          qsort(panResults, iResult, sizeof(int), compare_ints);
          *pnResult = iResult;
      }

      return panResults;
}
 
/************************************************************************/
/*                               FLTArraysOr                            */
/*                                                                      */
/*      Utility function to do an OR on 2 arrays.                       */
/************************************************************************/
int *FLTArraysOr(int *aFirstArray, int nSizeFirst, 
                 int *aSecondArray, int nSizeSecond,
                 int *pnResult)
{
    int nResultSize = 0;
    int *panResults = 0;
    int iResult = 0;
    int i, j;

    if (aFirstArray == NULL && aSecondArray == NULL)
      return NULL;

    if (aFirstArray == NULL || aSecondArray == NULL)
    {
        if (aFirstArray && nSizeFirst > 0)
        {
            panResults = (int *)malloc(sizeof(int)*nSizeFirst);
            for (i=0; i<nSizeFirst; i++)
              panResults[i] = aFirstArray[i];
            if (pnResult)
              *pnResult = nSizeFirst;

            return panResults;
        }
        else if (aSecondArray && nSizeSecond)
        {
            panResults = (int *)malloc(sizeof(int)*nSizeSecond);
            for (i=0; i<nSizeSecond; i++)
              panResults[i] = aSecondArray[i];
            if (pnResult)
              *pnResult = nSizeSecond;

            return panResults;
        }
    }
            
    if (aFirstArray && aSecondArray && nSizeFirst > 0 && 
        nSizeSecond > 0)
    {
        nResultSize = nSizeFirst + nSizeSecond;

        panResults = (int *)malloc(sizeof(int)*nResultSize);
        iResult= 0;

        if (nSizeFirst < nSizeSecond)
        {
            for (i=0; i<nSizeFirst; i++)
              panResults[iResult++] = aFirstArray[i];

            for (i=0; i<nSizeSecond; i++)
            {
                for (j=0; j<nSizeFirst; j++)
                {
                    if (aSecondArray[i] ==  aFirstArray[j])
                      break;
                    if (aSecondArray[i] < aFirstArray[j])
                    {
                        panResults[iResult++] = aSecondArray[i];
                        break;
                    }
                }
                if (j == nSizeFirst)
                    panResults[iResult++] = aSecondArray[i];
            }
        }
        else
        {       
            for (i=0; i<nSizeSecond; i++)
              panResults[iResult++] = aSecondArray[i];

            for (i=0; i<nSizeFirst; i++)
            {
                for (j=0; j<nSizeSecond; j++)
                {
                    if (aFirstArray[i] ==  aSecondArray[j])
                      break;
                    if (aFirstArray[i] < aSecondArray[j])
                    {
                        panResults[iResult++] = aFirstArray[i];
                        break;
                    }
                }
                if (j == nSizeSecond)
                    panResults[iResult++] = aFirstArray[i];
            }
        }
          
        if (iResult > 0)
        {
            panResults = (int *)realloc(panResults, sizeof(int)*iResult);
            qsort(panResults, iResult, sizeof(int), compare_ints);
            *pnResult = iResult;
            return panResults;
        }

    }

    return NULL;
}

/************************************************************************/
/*                               FLTArraysAnd                           */
/*                                                                      */
/*      Utility function to do an AND on 2 arrays.                      */
/************************************************************************/
int *FLTArraysAnd(int *aFirstArray, int nSizeFirst, 
                  int *aSecondArray, int nSizeSecond,
                  int *pnResult)
{
    int *panResults = NULL;
    int nResultSize =0;
    int iResult = 0;
    int i, j;

    /* assuming arrays are sorted. */
    if (aFirstArray && aSecondArray && nSizeFirst > 0 && 
        nSizeSecond > 0)
    {
        if (nSizeFirst < nSizeSecond)
          nResultSize = nSizeFirst;
        else
          nResultSize = nSizeSecond;
 
        panResults = (int *)malloc(sizeof(int)*nResultSize);
        iResult= 0;

        if (nSizeFirst > nSizeSecond)
        {
            for (i=0; i<nSizeFirst; i++)
            {
                for (j=0; j<nSizeSecond; j++)
                {
                    if (aFirstArray[i] == aSecondArray[j])
                    {
                        panResults[iResult++] = aFirstArray[i];
                        break;
                    }
                    /* if (aFirstArray[i] < aSecondArray[j]) */
                    /* break; */
                }
            }
        }
        else
        {
             for (i=0; i<nSizeSecond; i++)
             {
                 for (j=0; j<nSizeFirst; j++)
                 {
                     if (aSecondArray[i] == aFirstArray[j])
                     {
                         panResults[iResult++] = aSecondArray[i];
                         break;
                     }
                     /* if (aSecondArray[i] < aFirstArray[j]) */
                     /* break; */
                }
            }
        }
        
        if (iResult > 0)
        {
            panResults = (int *)realloc(panResults, sizeof(int)*iResult);
            qsort(panResults, iResult, sizeof(int), compare_ints);
            *pnResult = iResult;
            return panResults;
        }

    }

    return NULL;
}



/************************************************************************/
/*                            FLTGetQueryResults                        */
/*                                                                      */
/*      Return an arry of shpe id's after a filetr node was applied     */
/*      on a layer.                                                     */
/************************************************************************/
int *FLTGetQueryResults(FilterEncodingNode *psNode, mapObj *map, 
                        int iLayerIndex, int *pnResults,
                        int bOnlySpatialFilter)
{
    int *panResults = NULL, *panLeftResults=NULL, *panRightResults=NULL;
    int nLeftResult=0, nRightResult=0, nResults = 0;

    if (psNode->eType == FILTER_NODE_TYPE_LOGICAL)
    {
        if (psNode->psLeftNode)
          panLeftResults =  FLTGetQueryResults(psNode->psLeftNode, map, 
                                               iLayerIndex, &nLeftResult,
                                               bOnlySpatialFilter);

       if (psNode->psRightNode)
          panRightResults =  FLTGetQueryResults(psNode->psRightNode, map,
                                               iLayerIndex, &nRightResult,
                                                bOnlySpatialFilter);

        if (psNode->pszValue && strcasecmp(psNode->pszValue, "AND") == 0)
          panResults = FLTArraysAnd(panLeftResults, nLeftResult, 
                                  panRightResults, nRightResult, &nResults);
        else if (psNode->pszValue && strcasecmp(psNode->pszValue, "OR") == 0)
          panResults = FLTArraysOr(panLeftResults, nLeftResult, 
                                 panRightResults, nRightResult, &nResults);

        else if (psNode->pszValue && strcasecmp(psNode->pszValue, "NOT") == 0)
          panResults = FLTArraysNot(panLeftResults, nLeftResult, map, 
                                   iLayerIndex, &nResults);
    }
    else
    {
        panResults = FLTGetQueryResultsForNode(psNode, map, iLayerIndex, 
                                               &nResults, bOnlySpatialFilter);
    }

    if (pnResults)
      *pnResults = nResults;

    return panResults;
}

int FLTApplySpatialFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                 int iLayerIndex)
{
    return FLTApplyFilterToLayer(psNode, map, iLayerIndex, MS_TRUE);
}

/************************************************************************/
/*                          FLTApplyFilterToLayer                       */
/*                                                                      */
/*      Use the filter encoding node to create mapserver expressions    */
/*      and apply it to the layer.                                      */
/************************************************************************/
int FLTApplyFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                          int iLayerIndex, int bOnlySpatialFilter)
{
    int *panResults = NULL;
    int nResults = 0;
    layerObj *psLayer = NULL;

/* -------------------------------------------------------------------- */
/*      If it is a "simple" (meaning only one bbox and/or one           */
/*      propertyislike, just call FLTGetQueryResultsForNode (builds     */
/*      the appropriate expression and does the query).                 */
/*       Else, we have to do a recursive query (use each node to do     */
/*      a query and the use th operators (and, not, or) to merge        */
/*      results.                                                        */
/* -------------------------------------------------------------------- */

    
    /*    if (FLTValidForBBoxFilter(psNode) && 
        FLTValidForPropertyIsLikeFilter(psNode))
    {
        panResults= FLTGetQueryResultsForNode(psNode, map, iLayerIndex,
                                            &nResults);
    }
    
    else
    {
    */
    panResults = FLTGetQueryResults(psNode, map, iLayerIndex,
                                    &nResults, bOnlySpatialFilter);
    if (panResults)
      FLTAddToLayerResultCache(panResults, nResults, map, iLayerIndex);
    /* clear the cache if the results is NULL to make sure there aren't */
    /* any left over from intermediate queries. */
    else 
    {
        psLayer = &(map->layers[iLayerIndex]);
            
        if (psLayer && psLayer->resultcache)
        {
            if (psLayer->resultcache->results)
              free (psLayer->resultcache->results);
            free(psLayer->resultcache);
            
            psLayer->resultcache = NULL;
        }
    }
    

    if (panResults)
      free(panResults);

    return MS_SUCCESS;
}



/************************************************************************/
/*            FilterNode *FLTPaserFilterEncoding(char *szXMLString)     */
/*                                                                      */
/*      Parses an Filter Encoding XML string and creates a              */
/*      FilterEncodingNodes corresponding to the string.                */
/*      Returns a pointer to the first node or NULL if                  */
/*      unsuccessfull.                                                  */
/*      Calling function should use FreeFilterEncodingNode function     */
/*      to free memeory.                                                */
/************************************************************************/
FilterEncodingNode *FLTParseFilterEncoding(char *szXMLString)
{
    CPLXMLNode *psRoot = NULL, *psChild=NULL, *psFilter=NULL;
    CPLXMLNode  *psFilterStart = NULL;
    FilterEncodingNode *psFilterNode = NULL;
    
    if (szXMLString == NULL || strlen(szXMLString) <= 0 ||
        (strstr(szXMLString, "Filter") == NULL))
      return NULL;

    psRoot = CPLParseXMLString(szXMLString);
    
    if( psRoot == NULL)
       return NULL;

    /* strip namespaces */
    CPLStripXMLNamespace(psRoot, "ogc", 1); 
    CPLStripXMLNamespace(psRoot, "gml", 1); 

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
    psFilterStart = NULL;
    while (psChild)
    { 
        if (FLTIsSupportedFilterType(psChild))
        {
            psFilterStart = psChild;
            psChild = NULL;
        }
        else
          psChild = psChild->psNext;
    }
              
    if (psFilterStart && FLTIsSupportedFilterType(psFilterStart))
    {
        psFilterNode = FLTCreateFilterEncodingNode();
        
        FLTInsertElementInNode(psFilterNode, psFilterStart);
    }



/* -------------------------------------------------------------------- */
/*      validate the node tree to make sure that all the nodes are valid.*/
/* -------------------------------------------------------------------- */
    if (!FLTValidFilterNode(psFilterNode))
      return NULL;

/* -------------------------------------------------------------------- */
/*      validate for the BBox case :                                    */
/*       - only one BBox filter is supported                            */
/*       - a Bbox is only acceptable with an AND logical operator       */
/* -------------------------------------------------------------------- */
    /* if (!FLTValidForBBoxFilter(psFilterNode)) */
    /* return NULL; */

/* -------------------------------------------------------------------- */
/*      validate for the PropertyIsLike case :                          */
/*       - only one PropertyIsLike filter is supported                  */
/* -------------------------------------------------------------------- */
    /* if (!FLTValidForPropertyIsLikeFilter(psFilterNode)) */
    /* return NULL; */



    return psFilterNode;
}


/************************************************************************/
/*      int FLTValidFilterNode(FilterEncodingNode *psFilterNode)        */
/*                                                                      */
/*      Validate that all the nodes are filled properly. We could       */
/*      have parts of the nodes that are correct and part which         */
/*      could be incorrect if the filter string sent is corrupted       */
/*      (eg missing a value :<PropertyName><PropertyName>)              */
/************************************************************************/
int FLTValidFilterNode(FilterEncodingNode *psFilterNode)
{
    int  bReturn = 0;

    if (!psFilterNode)
      return 0;

    if (psFilterNode->eType == FILTER_NODE_TYPE_UNDEFINED)
      return 0;

    if (psFilterNode->psLeftNode)
    {
        bReturn = FLTValidFilterNode(psFilterNode->psLeftNode);
        if (bReturn == 0)
          return 0;
        else if (psFilterNode->psRightNode)
          return FLTValidFilterNode(psFilterNode->psRightNode);
    }

    return 1;
}
/************************************************************************/
/*                          FLTFreeFilterEncodingNode                   */
/*                                                                      */
/*      recursive freeing of Filer Encoding nodes.                      */
/************************************************************************/
void FLTFreeFilterEncodingNode(FilterEncodingNode *psFilterNode)
{
    if (psFilterNode)
    {
        if (psFilterNode->psLeftNode)
        {
            FLTFreeFilterEncodingNode(psFilterNode->psLeftNode);
            psFilterNode->psLeftNode = NULL;
        }
        if (psFilterNode->psRightNode)
        {
            FLTFreeFilterEncodingNode(psFilterNode->psRightNode);
            psFilterNode->psRightNode = NULL;
        }

        free(psFilterNode);
        /* TODO free pOther point for some operators. */
    }
}


/************************************************************************/
/*                         FLTCreateFilterEncodingNode                  */
/*                                                                      */
/*      return a FilerEncoding node.                                    */
/************************************************************************/
FilterEncodingNode *FLTCreateFilterEncodingNode(void)
{
    FilterEncodingNode *psFilterNode = NULL;

    psFilterNode = 
      (FilterEncodingNode *)malloc(sizeof (FilterEncodingNode));
    psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
    psFilterNode->pszValue = NULL;
    psFilterNode->pOther = NULL;
    psFilterNode->psLeftNode = NULL;
    psFilterNode->psRightNode = NULL;

    return psFilterNode;
}


/************************************************************************/
/*                           FLTInsertElementInNode                     */
/*                                                                      */
/*      Utility function to parse an XML node and transfter the         */
/*      contennts into the Filer Encoding node structure.               */
/************************************************************************/
void FLTInsertElementInNode(FilterEncodingNode *psFilterNode,
                            CPLXMLNode *psXMLNode)
{
    int nStrLength = 0;
    char *pszTmp = NULL;

    if (psFilterNode && psXMLNode && psXMLNode->pszValue)
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
        if (FLTIsLogicalFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_LOGICAL;
            if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
                strcasecmp(psFilterNode->pszValue, "OR") == 0)
            {
                if (psXMLNode->psChild && psXMLNode->psChild->psNext)
                {
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                    FLTInsertElementInNode(psFilterNode->psLeftNode, 
                                        psXMLNode->psChild);
                    psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                    FLTInsertElementInNode(psFilterNode->psRightNode, 
                                        psXMLNode->psChild->psNext);
                }
            }
            else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0)
            {
                if (psXMLNode->psChild)
                {
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                    FLTInsertElementInNode(psFilterNode->psLeftNode, 
                                        psXMLNode->psChild); 
                }
            } 
            else
              psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
        }/* end if is logical */
/* -------------------------------------------------------------------- */
/*      Spatial Filter.                                                 */
/*      BBOX :                                                          */
/*      <Filter>                                                        */
/*       <BBOX>                                                         */
/*        <PropertyName>Geometry</PropertyName>                         */
/*        <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326”>*/
/*          <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*        </gml:Box>                                                    */
/*       </BBOX>                                                        */
/*      </Filter>                                                       */
/*                                                                      */
/*       DWithin                                                        */
/*                                                                      */
/*      <xsd:element name="DWithin"                                     */
/*      type="ogc:DistanceBufferType"                                   */
/*      substitutionGroup="ogc:spatialOps"/>                            */
/*                                                                      */
/*      <xsd:complexType name="DistanceBufferType">                     */
/*         <xsd:complexContent>                                         */
/*            <xsd:extension base="ogc:SpatialOpsType">                 */
/*               <xsd:sequence>                                         */
/*                  <xsd:element ref="ogc:PropertyName"/>               */
/*                  <xsd:element ref="gml:_Geometry"/>                  */
/*                  <xsd:element name="Distance" type="ogc:DistanceType"/>*/
/*               </xsd:sequence>                                        */
/*            </xsd:extension>                                          */
/*         </xsd:complexContent>                                        */
/*      </xsd:complexType>                                              */
/*                                                                      */
/*                                                                      */
/*       <Filter>                                                       */
/*       <DWithin>                                                      */
/*        <PropertyName>Geometry</PropertyName>                         */
/*        <gml:Point>                                                   */
/*          <gml:coordinates>13.0983,31.5899</gml:coordinates>          */
/*        </gml:Point>                                                  */
/*        <Distance units="url#m">10</Distance>                         */
/*       </DWithin>                                                     */
/*      </Filter>                                                       */
/*                                                                      */
/*       Intersect                                                      */
/*                                                                      */
/*       type="ogc:BinarySpatialOpType" substitutionGroup="ogc:spatialOps"/>*/
/*      <xsd:element name="Intersects"                                  */
/*      type="ogc:BinarySpatialOpType"                                  */
/*      substitutionGroup="ogc:spatialOps"/>                            */
/*                                                                      */
/*      <xsd:complexType name="BinarySpatialOpType">                    */
/*      <xsd:complexContent>                                            */
/*      <xsd:extension base="ogc:SpatialOpsType">                       */
/*      <xsd:sequence>                                                  */
/*      <xsd:element ref="ogc:PropertyName"/>                           */
/*      <xsd:choice>                                                    */
/*      <xsd:element ref="gml:_Geometry"/>                              */
/*      <xsd:element ref="gml:Box"/>                                    */
/*      </xsd:sequence>                                                 */
/*      </xsd:extension>                                                */
/*      </xsd:complexContent>                                           */
/*      </xsd:complexType>                                              */
/* -------------------------------------------------------------------- */
        else if (FLTIsSpatialFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_SPATIAL;

            if (strcasecmp(psXMLNode->pszValue, "BBOX") == 0)
            {
                char *pszSRS = NULL;
                CPLXMLNode *psPropertyName = NULL;
                CPLXMLNode *psBox = NULL;
                CPLXMLNode *psCoordinates = NULL;
                CPLXMLNode *psCoord1 = NULL, *psCoord2 = NULL;
                CPLXMLNode *psX = NULL, *psY = NULL;
                char **szCoords=NULL, **szMin=NULL, **szMax = NULL;
                char  *szCoords1=NULL, *szCoords2 = NULL;
                int nCoords = 0;
                char *pszTmpCoord = NULL;
                int bCoordinatesValid = 0;

                psPropertyName = CPLGetXMLNode(psXMLNode, "PropertyName");
                psBox = CPLGetXMLNode(psXMLNode, "Box");
                if (!psBox)
                  psBox = CPLGetXMLNode(psXMLNode, "BoxType");

                if (psBox)
                {
                    pszSRS = (char *)CPLGetXMLValue(psBox, "srsName", NULL);
            
                    psCoordinates = CPLGetXMLNode(psBox, "coordinates");
                    if (psCoordinates && psCoordinates->psChild && 
                        psCoordinates->psChild->pszValue)
                    {
                        pszTmpCoord = psCoordinates->psChild->pszValue;
                        szCoords = split(pszTmpCoord, ' ', &nCoords);
                        if (szCoords && nCoords == 2)
                        {
                            szCoords1 = strdup(szCoords[0]);
                            szCoords2 = strdup(szCoords[1]);
                            szMin = split(szCoords1, ',', &nCoords);
                            if (szMin && nCoords == 2)
                              szMax = split(szCoords2, ',', &nCoords);
                            if (szMax && nCoords == 2)
                              bCoordinatesValid =1;

                            free(szCoords1);        
                            free(szCoords2);
                        }
                    }
                    else
                    {
                        psCoord1 = CPLGetXMLNode(psBox, "coord");
                        if (psCoord1 && psCoord1->psNext && 
                            psCoord1->psNext->pszValue && 
                            strcmp(psCoord1->psNext->pszValue, "coord") ==0)
                        {
                            szMin = (char **)malloc(sizeof(char *)*2);
                            szMax = (char **)malloc(sizeof(char *)*2);
                            psCoord2 = psCoord1->psNext;
                            psX =  CPLGetXMLNode(psCoord1, "X");
                            psY =  CPLGetXMLNode(psCoord1, "Y");
                            if (psX && psY && psX->psChild && psY->psChild &&
                                psX->psChild->pszValue && psY->psChild->pszValue)
                            {
                                szMin[0] = psX->psChild->pszValue;
                                szMin[1] = psY->psChild->pszValue;

                                psX =  CPLGetXMLNode(psCoord2, "X");
                                psY =  CPLGetXMLNode(psCoord2, "Y");
                                if (psX && psY && psX->psChild && psY->psChild &&
                                    psX->psChild->pszValue && psY->psChild->pszValue)
                                {
                                    szMax[0] = psX->psChild->pszValue;
                                    szMax[1] = psY->psChild->pszValue;
                                    bCoordinatesValid = 1;
                                }
                            }
                        }
                    
                    }
                }
            
                if (psPropertyName == NULL || !bCoordinatesValid)
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;

                if (psPropertyName && bCoordinatesValid)
                {
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                    /* not really using the property name anywhere ??  */
                    /* Is is always Geometry ?  */
                    if (psPropertyName->psChild && 
                        psPropertyName->psChild->pszValue)
                    {
                        psFilterNode->psLeftNode->eType = 
                          FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = 
                          strdup(psPropertyName->psChild->pszValue);
                    }
                
                    /* srs and coordinates */
                    psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BBOX;
                    /* srs might be empty */
                    if (pszSRS)
                      psFilterNode->psRightNode->pszValue = strdup(pszSRS);

                    psFilterNode->psRightNode->pOther =     
                      (rectObj *)malloc(sizeof(rectObj));
                    ((rectObj *)psFilterNode->psRightNode->pOther)->minx = 
                      atof(szMin[0]);
                    ((rectObj *)psFilterNode->psRightNode->pOther)->miny = 
                      atof(szMin[1]);

                    ((rectObj *)psFilterNode->psRightNode->pOther)->maxx = 
                      atof(szMax[0]);
                    ((rectObj *)psFilterNode->psRightNode->pOther)->maxy = 
                      atof(szMax[1]);

                    free(szMin);
                    free(szMax);
                }
            }
            else if (strcasecmp(psXMLNode->pszValue, "DWithin") == 0)
            {
                shapeObj *psShape = NULL;
                int bPoint = 0, bLine = 0, bPolygon = 0;

                
                CPLXMLNode *psGMLElement = NULL, *psDistance=NULL;


                psGMLElement = CPLGetXMLNode(psXMLNode, "Point");
                if (!psGMLElement)
                  psGMLElement =  CPLGetXMLNode(psXMLNode, "PointType");
                if (psGMLElement)
                  bPoint =1;
                else
                {
                    psGMLElement= CPLGetXMLNode(psXMLNode, "Polygon");
                    if (psGMLElement)
                      bPolygon = 1;
                    else
                    {
                        psGMLElement= CPLGetXMLNode(psXMLNode, "LineString");
                        if (psGMLElement)
                          bLine = 1;
                    }		
                }

                psDistance = CPLGetXMLNode(psXMLNode, "Distance");
                if (psGMLElement && psDistance && psDistance->psChild &&  
                    psDistance->psChild->psNext && psDistance->psChild->psNext->pszValue)
                {
                    psShape = (shapeObj *)malloc(sizeof(shapeObj));
                    msInitShape(psShape);
                    if (FLTShapeFromGMLTree(psGMLElement, psShape))
                      /* if (FLTGML2Shape_XMLNode(psPoint, psShape)) */
                    {
                        psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                        /* not really using the property name anywhere ?? Is is always */
                        /* Geometry ?  */
                    
                        psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = strdup("Geometry");

                        psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                        if (bPoint)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POINT;
                        else if (bLine)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_LINE;
                        else if (bPolygon)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POLYGON;
                        psFilterNode->psRightNode->pOther = (shapeObj *)psShape;
                        psFilterNode->psRightNode->pszValue = 
                          strdup(psDistance->psChild->psNext->pszValue);
                    }
                }
                else
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
            }
            else if (strcasecmp(psXMLNode->pszValue, "Intersect") == 0)
            {
                shapeObj *psShape = NULL;
                int  bLine = 0, bPolygon = 0;

                
                CPLXMLNode *psGMLElement = NULL;


                psGMLElement = CPLGetXMLNode(psXMLNode, "Polygon");
                if (psGMLElement)
                  bPolygon = 1;
                else
                {
                    psGMLElement= CPLGetXMLNode(psXMLNode, "LineString");
                    if (psGMLElement)
                      bLine = 1;
                }		
                

                if (psGMLElement)
                {
                    psShape = (shapeObj *)malloc(sizeof(shapeObj));
                    msInitShape(psShape);
                    if (FLTShapeFromGMLTree(psGMLElement, psShape))
                      /* if (FLTGML2Shape_XMLNode(psPoint, psShape)) */
                    {
                        psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                        /* not really using the property name anywhere ?? Is is always */
                        /* Geometry ?  */
                    
                        psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = strdup("Geometry");

                        psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                        if (bLine)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_LINE;
                        else if (bPolygon)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POLYGON;
                        psFilterNode->psRightNode->pOther = (shapeObj *)psShape;

                    }
                }
                else
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
            }
            
                
        }/* end of is spatial */


/* -------------------------------------------------------------------- */
/*      Comparison Filter                                               */
/* -------------------------------------------------------------------- */
        else if (FLTIsComparisonFilterType(psXMLNode->pszValue))
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
            if (FLTIsBinaryComparisonFilterType(psXMLNode->pszValue))
            {
                if (psXMLNode->psChild && psXMLNode->psChild->psNext &&
                    psXMLNode->psChild->pszValue &&
                    psXMLNode->psChild->psNext->pszValue &&
                    strcasecmp(psXMLNode->psChild->pszValue, "PropertyName") == 0 &&
                    strcasecmp(psXMLNode->psChild->psNext->pszValue,"Literal") == 0)
                {
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();

                    if (psXMLNode->psChild->psChild &&
                        psXMLNode->psChild->psChild->pszValue &&
                        strlen(psXMLNode->psChild->psChild->pszValue) > 0)
                    {
                        psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = 
                          strdup(psXMLNode->psChild->psChild->pszValue);
                    }

                    psFilterNode->psRightNode = FLTCreateFilterEncodingNode();

                  
                    /* special case where the user puts an empty value */
                    /* for the Literal so it can end up as an empty  */
                    /* string query in the expression */
                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
                    if (psXMLNode->psChild->psNext->psChild &&
                        psXMLNode->psChild->psNext->psChild->pszValue &&
                        strlen(psXMLNode->psChild->psNext->psChild->pszValue) > 0)
                    {
                        psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
                        psFilterNode->psRightNode->pszValue = 
                          strdup(psXMLNode->psChild->psNext->psChild->pszValue);
                    }
                    else
                      psFilterNode->psRightNode->pszValue = NULL;
                }
                else
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
            }
/* -------------------------------------------------------------------- */
/*      PropertyIsBetween filter : extract property name and boudary    */
/*      values. The boundary  values are stored in the right            */
/*      node. The values are separated by a semi-column (;)             */
/*      Eg of Filter :                                                  */
/*      <PropertyIsBetween>                                             */
/*         <PropertyName>DEPTH</PropertyName>                           */
/*         <LowerBoundary><Literal>400</Literal></LowerBoundary>        */
/*         <UpperBoundary><Literal>800</Literal></UpperBoundary>        */
/*      </PropertyIsBetween>                                            */
/*                                                                      */
/*      Or                                                              */
/*      <PropertyIsBetween>                                             */
/*         <PropertyName>DEPTH</PropertyName>                           */
/*         <LowerBoundary>400</LowerBoundary>                           */
/*         <UpperBoundary>800</UpperBoundary>                           */
/*      </PropertyIsBetween>                                            */
/* -------------------------------------------------------------------- */
            else if (strcasecmp(psXMLNode->pszValue, "PropertyIsBetween") == 0)
            {
                CPLXMLNode *psUpperNode = NULL, *psLowerNode = NULL;
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
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                    
                    if (psXMLNode->psChild->psChild && 
                        psXMLNode->psChild->psChild->pszValue)
                    {
                        psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = 
                          strdup(psXMLNode->psChild->psChild->pszValue);
                    }

                    psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                    if (psXMLNode->psChild->psNext->psChild &&
                        psXMLNode->psChild->psNext->psNext->psChild &&
                        psXMLNode->psChild->psNext->psChild->pszValue &&
                        psXMLNode->psChild->psNext->psNext->psChild->pszValue)
                    {
                        /* check if the <Literals> is there */
                        psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BOUNDARY;
                        if (psXMLNode->psChild->psNext->psChild->psChild)
                          psLowerNode = psXMLNode->psChild->psNext->psChild->psChild;
                        else
                          psLowerNode = psXMLNode->psChild->psNext->psChild;

                        if (psXMLNode->psChild->psNext->psNext->psChild->psChild)
                          psUpperNode = psXMLNode->psChild->psNext->psNext->psChild->psChild;
                        else
                          psUpperNode = psXMLNode->psChild->psNext->psNext->psChild;

                        nStrLength = 
                          strlen(psLowerNode->pszValue);
                        nStrLength +=
                          strlen(psUpperNode->pszValue);

                        nStrLength += 2; /* adding a ; between bounary values */
                         psFilterNode->psRightNode->pszValue = 
                           (char *)malloc(sizeof(char)*(nStrLength));
                         strcpy( psFilterNode->psRightNode->pszValue,
                                psLowerNode->pszValue);
                         strcat(psFilterNode->psRightNode->pszValue, ";");
                         strcat(psFilterNode->psRightNode->pszValue,
                                psUpperNode->pszValue); 
                         psFilterNode->psRightNode->pszValue[nStrLength-1]='\0';
                    }
                         
                  
                }
                else
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;

            }/* end of PropertyIsBetween  */
/* -------------------------------------------------------------------- */
/*      PropertyIsLike                                                  */
/*                                                                      */
/*      <Filter>                                                        */
/*      <PropertyIsLike wildCard="*" singleChar="#" escape="!">         */
/*      <PropertyName>LAST_NAME</PropertyName>                          */
/*      <Literal>JOHN*</Literal>                                        */
/*      </PropertyIsLike>                                               */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
            else if (strcasecmp(psXMLNode->pszValue, "PropertyIsLike") == 0)
            {
                if (psXMLNode->psChild &&  
                    strcasecmp(psXMLNode->psChild->pszValue, "wildCard") == 0 &&
                    psXMLNode->psChild->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->pszValue, "singleChar") == 0 &&
                    psXMLNode->psChild->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->pszValue, "escape") == 0 &&
                    psXMLNode->psChild->psNext->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->psNext->pszValue, "PropertyName") == 0 &&
                    psXMLNode->psChild->psNext->psNext->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->psNext->psNext->pszValue, "Literal") == 0)
                {
/* -------------------------------------------------------------------- */
/*      Get the wildCard, the singleChar and the escapeChar used.       */
/* -------------------------------------------------------------------- */
                    psFilterNode->pOther = (FEPropertyIsLike *)malloc(sizeof(FEPropertyIsLike));
                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "wildCard", "");
                    if (pszTmp)
                      ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard = 
                        strdup(pszTmp);
                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "singleChar", "");
                    if (pszTmp)
                      ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar = 
                        strdup(pszTmp);
                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "escape", "");
                    if (pszTmp)
                      ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar = 
                        strdup(pszTmp);
/* -------------------------------------------------------------------- */
/*      Create left and right node for the attribute and the value.     */
/* -------------------------------------------------------------------- */
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();

                    
                    
                    if (psXMLNode->psChild->psNext->psNext->psNext->psChild &&
                        psXMLNode->psChild->psNext->psNext->psNext->psChild->pszValue)
                    {
                        psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = 
                          strdup(psXMLNode->psChild->psNext->psNext->psNext->psChild->pszValue);
                    }

                    psFilterNode->psRightNode = FLTCreateFilterEncodingNode();

                    if (psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild &&
                        psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild->pszValue)
                    {
                        psFilterNode->psRightNode->pszValue = 
                          strdup(psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild->pszValue);        
                        psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
                    }
                }
                else
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
                  
            }
        }
            
    }
}

 
/************************************************************************/
/*            int FLTIsLogicalFilterType((char *pszValue)                  */
/*                                                                      */
/*      return TRUE if the value of the node is of logical filter       */
/*       encoding type.                                                 */
/************************************************************************/
int FLTIsLogicalFilterType(char *pszValue)
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
/*         int FLTIsBinaryComparisonFilterType(char *pszValue)             */
/*                                                                      */
/*      Binary comparison filter type.                                  */
/************************************************************************/
int FLTIsBinaryComparisonFilterType(char *pszValue)
{
    if (pszValue)
    {
         if (strcasecmp(pszValue, "PropertyIsEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsNotEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsLessThan") == 0 ||  
             strcasecmp(pszValue, "PropertyIsGreaterThan") == 0 ||  
             strcasecmp(pszValue, "PropertyIsLessThanOrEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsGreaterThanOrEqualTo") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*            int FLTIsComparisonFilterType(char *pszValue)                */
/*                                                                      */
/*      return TRUE if the value of the node is of comparison filter    */
/*      encoding type.                                                  */
/************************************************************************/
int FLTIsComparisonFilterType(char *pszValue)
{
    if (pszValue)
    {
         if (FLTIsBinaryComparisonFilterType(pszValue) ||  
             strcasecmp(pszValue, "PropertyIsLike") == 0 ||  
             strcasecmp(pszValue, "PropertyIsBetween") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}


/************************************************************************/
/*            int FLTIsSpatialFilterType(char *pszValue)                   */
/*                                                                      */
/*      return TRUE if the value of the node is of spatial filter       */
/*      encoding type.                                                  */
/************************************************************************/
int FLTIsSpatialFilterType(char *pszValue)
{
    if (pszValue)
    {
        if ( strcasecmp(pszValue, "BBOX") == 0 ||
             strcasecmp(pszValue, "DWithin") == 0 ||
             strcasecmp(pszValue, "Intersect") == 0)
          return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*           int FLTIsSupportedFilterType(CPLXMLNode *psXMLNode)           */
/*                                                                      */
/*      Verfify if the value of the node is one of the supported        */
/*      filter type.                                                    */
/************************************************************************/
int FLTIsSupportedFilterType(CPLXMLNode *psXMLNode)
{
    if (psXMLNode)
    {
        if (FLTIsLogicalFilterType(psXMLNode->pszValue) ||
            FLTIsSpatialFilterType(psXMLNode->pszValue) ||
            FLTIsComparisonFilterType(psXMLNode->pszValue))
          return MS_TRUE;
    }

    return MS_FALSE;
}
 
/************************************************************************/
/*                          FLTNumberOfFilterType                       */
/*                                                                      */
/*      Loop trhough the nodes and return the number of nodes of        */
/*      specified value.                                                */
/************************************************************************/
int FLTNumberOfFilterType(FilterEncodingNode *psFilterNode, const char *szType)
{
    int nCount = 0;
    int nLeftNode=0 , nRightNode = 0;

    if (!psFilterNode || !szType || !psFilterNode->pszValue)
      return 0;

    if (strcasecmp(psFilterNode->pszValue, (char*)szType) == 0)
      nCount++;

    if (psFilterNode->psLeftNode)
      nLeftNode = FLTNumberOfFilterType(psFilterNode->psLeftNode, szType);

    nCount += nLeftNode;

    if (psFilterNode->psRightNode)
      nRightNode = FLTNumberOfFilterType(psFilterNode->psRightNode, szType);
    nCount += nRightNode;
   
    return nCount;
}
    

/************************************************************************/
/*                      FLTValidForPropertyIsLikeFilter                 */
/*                                                                      */
/*      Valdate if there is only one ProperyIsLike Filter.              */
/************************************************************************/
int FLTValidForPropertyIsLikeFilter(FilterEncodingNode *psFilterNode)
{
    int nCount = 0;
   
    if (!psFilterNode)
      return 1;

    nCount = FLTNumberOfFilterType(psFilterNode, "PropertyIsLike");

    if (nCount > 1)
      return 0;
    else if (nCount == 0)
      return 1;
    
    /* nCount ==1  */
    if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
      return 1;

    /*    if (strcasecmp(psFilterNode->pszValue, "OR") == 0)
    {
        if (strcasecmp(psFilterNode->psLeftNode->pszValue, "PropertyIsLike") ==0 ||
            strcasecmp(psFilterNode->psRightNode->pszValue, "PropertyIsLike") ==0)
          return 1;
    }
    */
    return 1;
}

int FLTIsPropertyIsLikeFilter(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode || !psFilterNode->pszValue)
      return 0;
    
    if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
      return 1;

    if (strcasecmp(psFilterNode->pszValue, "OR") == 0)
    {
      if (strcasecmp(psFilterNode->psLeftNode->pszValue, "PropertyIsLike") ==0 ||
          strcasecmp(psFilterNode->psRightNode->pszValue, "PropertyIsLike") ==0)
        return 1;
    }

    return 0;
}       
 

/************************************************************************/
/*                         FLTIsOnlyPropertyIsLike                      */
/*                                                                      */
/*      Check if the only expression expression in the node tree is     */
/*      of type PropertyIsLIke. 2 cases :                               */
/*        - one node with PropertyIsLike                                */
/*        - or PropertyIsLike combined with BBOX                        */
/************************************************************************/
int FLTIsOnlyPropertyIsLike(FilterEncodingNode *psFilterNode)
{
    if (psFilterNode && psFilterNode->pszValue)
    {
        
        if (strcmp(psFilterNode->pszValue, "PropertyIsLike") ==0)
          return 1;
        else if (FLTNumberOfFilterType(psFilterNode, "PropertyIsLike") == 1 &&
                 FLTNumberOfFilterType(psFilterNode, "BBOX") == 1)
          return 1;
    }
    return 0;
}

char *FLTGetMapserverIsPropertyExpression(FilterEncodingNode *psFilterNode)
{
    char *pszExpression = NULL;

    if (psFilterNode && psFilterNode->pszValue && 
         strcmp(psFilterNode->pszValue, "PropertyIsLike") ==0)
      return FLTGetMapserverExpression(psFilterNode);
    else
    {
        if (psFilterNode->psLeftNode)
          pszExpression = 
            FLTGetMapserverIsPropertyExpression(psFilterNode->psLeftNode);
        if (!pszExpression && psFilterNode->psRightNode)
          pszExpression = 
            FLTGetMapserverIsPropertyExpression(psFilterNode->psRightNode);
    }

    return pszExpression;
}


/************************************************************************/
/*                          FLTValidForBBoxFilter                       */
/*                                                                      */
/*      Validate if there is only one BBOX filter node. Here is waht    */
/*      is supported (is valid) :                                       */
/*        - one node which is a BBOX                                    */
/*        - a logical AND with a valid BBOX                             */
/*                                                                      */
/*      eg 1: <Filter>                                                  */
/*            <BBOX>                                                    */
/*              <PropertyName>Geometry</PropertyName>                   */
/*              <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326”>*/
/*                <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*              </gml:Box>                                              */
/*            </BBOX>                                                   */
/*          </Filter>                                                   */
/*                                                                      */
/*      eg 2 :<Filter>                                                  */
/*              <AND>                                                   */
/*               <BBOX>                                                 */
/*                <PropertyName>Geometry</PropertyName>                  */
/*                <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326”>*/
/*                  <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*                </gml:Box>                                            */
/*               </BBOX>                                                */
/*               <PropertyIsEqualTo>                                    */
/*               <PropertyName>SomeProperty</PropertyName>              */
/*                <Literal>100</Literal>                                */
/*              </PropertyIsEqualTo>                                    */
/*             </AND>                                                   */
/*           </Filter>                                                  */
/*                                                                      */
/************************************************************************/
int FLTValidForBBoxFilter(FilterEncodingNode *psFilterNode)
{
    int nCount = 0;
   
    if (!psFilterNode || !psFilterNode->pszValue)
      return 1;

    nCount = FLTNumberOfFilterType(psFilterNode, "BBOX");

    if (nCount > 1)
      return 0;
    else if (nCount == 0)
      return 1;

    /* nCount ==1  */
    if (strcasecmp(psFilterNode->pszValue, "BBOX") == 0)
      return 1;

    if (strcasecmp(psFilterNode->pszValue, "AND") == 0)
    {
      if (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") ==0 ||
          strcasecmp(psFilterNode->psRightNode->pszValue, "BBOX") ==0)
        return 1;
    }

    return 0;
}    

int FLTIsLineFilter(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode || !psFilterNode->pszValue)
      return 0;

    if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL && 
        psFilterNode->psRightNode &&  
        psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_GEOMETRY_LINE)
      return 1;

    return 0;
}
  
int FLTIsPolygonFilter(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode || !psFilterNode->pszValue)
      return 0;

    if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL && 
        psFilterNode->psRightNode &&  
        psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_GEOMETRY_POLYGON)
      return 1;

    return 0;
}
    
int FLTIsPointFilter(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode || !psFilterNode->pszValue)
      return 0;

    if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL && 
        psFilterNode->psRightNode &&  
        psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_GEOMETRY_POINT)
      return 1;

    return 0;
}

int FLTIsBBoxFilter(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode || !psFilterNode->pszValue)
      return 0;
    
    if (strcasecmp(psFilterNode->pszValue, "BBOX") == 0)
      return 1;

    /*    if (strcasecmp(psFilterNode->pszValue, "AND") == 0)
    {
      if (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") ==0 ||
          strcasecmp(psFilterNode->psRightNode->pszValue, "BBOX") ==0)
        return 1;
    }
    */
    return 0;
}       

shapeObj *FLTGetShape(FilterEncodingNode *psFilterNode, double *pdfDistance)
{
    FilterEncodingNode *psNode = psFilterNode;
    if (psNode)
    {
        if (psNode->eType == FILTER_NODE_TYPE_SPATIAL && psNode->psRightNode)
          psNode = psNode->psRightNode;

        if (psNode->eType == FILTER_NODE_TYPE_GEOMETRY_POINT ||
            psNode->eType == FILTER_NODE_TYPE_GEOMETRY_LINE ||
            psNode->eType == FILTER_NODE_TYPE_GEOMETRY_POLYGON)
        {
            if (psNode->pszValue && pdfDistance)
              *pdfDistance = atof(psNode->pszValue);

            return (shapeObj *)psNode->pOther;
        }
    }
    return NULL;   
}

/************************************************************************/
/*                                FLTGetBBOX                            */
/*                                                                      */
/*      Loop through the nodes are return the coordinates of the        */
/*      first bbox node found. The retrun value is the epsg code of     */
/*      the bbox.                                                       */
/************************************************************************/
char *FLTGetBBOX(FilterEncodingNode *psFilterNode, rectObj *psRect)
{
    char *pszReturn = NULL;
    
    if (!psFilterNode || !psRect)
      return NULL;

    if (strcasecmp(psFilterNode->pszValue, "BBOX") == 0)
    {
        if (psFilterNode->psRightNode && psFilterNode->psRightNode->pOther)
        {
            psRect->minx = ((rectObj *)psFilterNode->psRightNode->pOther)->minx;
            psRect->miny = ((rectObj *)psFilterNode->psRightNode->pOther)->miny;
            psRect->maxx = ((rectObj *)psFilterNode->psRightNode->pOther)->maxx;
            psRect->maxy = ((rectObj *)psFilterNode->psRightNode->pOther)->maxy;
            
            return psFilterNode->psRightNode->pszValue;
            
        }
    }
    else
    {
        pszReturn = FLTGetBBOX(psFilterNode->psLeftNode, psRect);
        if (pszReturn)
          return pszReturn;
        else
          return  FLTGetBBOX(psFilterNode->psRightNode, psRect);
    }

    return pszReturn;
}
/************************************************************************/
/*                     GetMapserverExpressionClassItem                  */
/*                                                                      */
/*      Check if one of the node is a PropertyIsLike filter node. If    */
/*      that is the case return the value that will be used as a        */
/*      ClassItem in mapserver.                                         */
/*      NOTE : only the first one is used.                              */
/************************************************************************/
char *FLTGetMapserverExpressionClassItem(FilterEncodingNode *psFilterNode)
{
    char *pszReturn = NULL;

    if (!psFilterNode)
      return NULL;

    if (psFilterNode->pszValue && 
        strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
    {
        if (psFilterNode->psLeftNode)
          return psFilterNode->psLeftNode->pszValue;
    }
    else
    {
        pszReturn = FLTGetMapserverExpressionClassItem(psFilterNode->psLeftNode);
        if (pszReturn)
          return pszReturn;
        else
          return  FLTGetMapserverExpressionClassItem(psFilterNode->psRightNode);
    }

    return pszReturn;
}
/************************************************************************/
/*                          GetMapserverExpression                      */
/*                                                                      */
/*      Return a mapserver expression base on the Filer encoding nodes. */
/************************************************************************/
char *FLTGetMapserverExpression(FilterEncodingNode *psFilterNode)
{
    char *pszExpression = NULL;

    if (!psFilterNode)
      return NULL;

    if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON)
    {
        if ( psFilterNode->psLeftNode && psFilterNode->psRightNode)
        {
            if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
            {
                pszExpression = FLTGetBinaryComparisonExpresssion(psFilterNode);
            }
            else if (strcasecmp(psFilterNode->pszValue, 
                                "PropertyIsBetween") == 0)
            {
                 pszExpression = FLTGetIsBetweenComparisonExpresssion(psFilterNode);
            }
            else if (strcasecmp(psFilterNode->pszValue, 
                                "PropertyIsLike") == 0)
            {
                 pszExpression = FLTGetIsLikeComparisonExpression(psFilterNode);
            }
        }
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL)
    {
        if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
            strcasecmp(psFilterNode->pszValue, "OR") == 0)
        {
            pszExpression = FLTGetLogicalComparisonExpresssion(psFilterNode);
        }
        else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0)
        {       
            pszExpression = FLTGetLogicalComparisonExpresssion(psFilterNode);
        }
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL)
    {
        /* TODO */
    }
            
    return pszExpression;
}
  
/************************************************************************/
/*                            FLTGetNodeExpression                      */
/*                                                                      */
/*      Return the expresion for a specific node.                       */
/************************************************************************/
char *FLTGetNodeExpression(FilterEncodingNode *psFilterNode)
{
    char *pszExpression = NULL;
    if (!psFilterNode)
      return NULL;

    if (FLTIsLogicalFilterType(psFilterNode->pszValue))
      pszExpression = FLTGetLogicalComparisonExpresssion(psFilterNode);
    else if (FLTIsComparisonFilterType(psFilterNode->pszValue))
    {
        if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
          pszExpression = FLTGetBinaryComparisonExpresssion(psFilterNode);
        else if (strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0)
          pszExpression = FLTGetIsBetweenComparisonExpresssion(psFilterNode);
        else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
          pszExpression = FLTGetIsLikeComparisonExpression(psFilterNode);
    }

    return pszExpression;
}


/************************************************************************/
/*                     FLTGetLogicalComparisonExpresssion               */
/*                                                                      */
/*      Return the expression for logical comparison expression.        */
/************************************************************************/
char *FLTGetLogicalComparisonExpresssion(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];
    char *pszTmp = NULL;
    szBuffer[0] = '\0';

    if (!psFilterNode || !FLTIsLogicalFilterType(psFilterNode->pszValue))
      return NULL;

    
/* ==================================================================== */
/*      special case for BBOX node.                                     */
/* ==================================================================== */
    if (psFilterNode->psLeftNode && psFilterNode->psRightNode &&
        ((strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") == 0) ||
         (strcasecmp(psFilterNode->psRightNode->pszValue, "BBOX") == 0) ||
         (strcasecmp(psFilterNode->psLeftNode->pszValue, "DWithin") == 0) ||
         (strcasecmp(psFilterNode->psRightNode->pszValue, "DWithin") == 0) ||
         (strcasecmp(psFilterNode->psLeftNode->pszValue, "Intersect") == 0) ||
         (strcasecmp(psFilterNode->psRightNode->pszValue, "Intersect") == 0)))
    {
        strcat(szBuffer, " (");
        if (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") != 0 &&
            strcasecmp(psFilterNode->psLeftNode->pszValue, "DWithin") != 0 &&
            strcasecmp(psFilterNode->psLeftNode->pszValue, "Intersect") != 0)
          pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode);
        else
          pszTmp = FLTGetNodeExpression(psFilterNode->psRightNode);

        if (!pszTmp)
          return NULL;
        if (strcasecmp(psFilterNode->psLeftNode->pszValue, "PropertyIsLike") == 0 ||
            strcasecmp(psFilterNode->psRightNode->pszValue, "PropertyIsLike") == 0)
          sprintf(szBuffer, "%s", pszTmp);
        else
           sprintf(szBuffer, "(%s)", pszTmp);
        

        return strdup(szBuffer);
    }

/* ==================================================================== */
/*      special case for PropertyIsLike.                                */
/* ==================================================================== */
    if (psFilterNode->psLeftNode && psFilterNode->psRightNode &&
        ((strcasecmp(psFilterNode->psLeftNode->pszValue, "PropertyIsLike") == 0) ||
         (strcasecmp(psFilterNode->psRightNode->pszValue, "PropertyIsLike") == 0)))
    {
        if (strcasecmp(psFilterNode->psLeftNode->pszValue, "PropertyIsLike") != 0)
          pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode);
        else
          pszTmp = FLTGetNodeExpression(psFilterNode->psRightNode);

        if (!pszTmp)
          return NULL;
        strcat(szBuffer, pszTmp);

        return strdup(szBuffer);
    }
/* -------------------------------------------------------------------- */
/*      OR and AND                                                      */
/* -------------------------------------------------------------------- */
    if (psFilterNode->psLeftNode && psFilterNode->psRightNode)
    {
        strcat(szBuffer, " (");
        pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode);
        if (!pszTmp)
          return NULL;

        strcat(szBuffer, pszTmp);
        strcat(szBuffer, " ");
        strcat(szBuffer, psFilterNode->pszValue);
        strcat(szBuffer, " ");
        pszTmp = FLTGetNodeExpression(psFilterNode->psRightNode);
        if (!pszTmp)
          return NULL;
        strcat(szBuffer, pszTmp);
        strcat(szBuffer, ") ");
    }
/* -------------------------------------------------------------------- */
/*      NOT                                                             */
/* -------------------------------------------------------------------- */
    else if (psFilterNode->psLeftNode && 
             strcasecmp(psFilterNode->pszValue, "NOT") == 0)
    {
        strcat(szBuffer, " (NOT ");
        pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode);
        if (!pszTmp)
          return NULL;
        strcat(szBuffer, pszTmp);
        strcat(szBuffer, ") ");
    }
    else
      return NULL;
    
    return strdup(szBuffer);
    
}

    
    
/************************************************************************/
/*                      FLTGetBinaryComparisonExpresssion               */
/*                                                                      */
/*      Return the expression for a binary comparison filter node.      */
/************************************************************************/
char *FLTGetBinaryComparisonExpresssion(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];
    int i=0, bString=0, nLenght = 0;

    szBuffer[0] = '\0';
    if (!psFilterNode || !FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
      return NULL;

/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (psFilterNode->psRightNode->pszValue)
    {
        nLenght = strlen(psFilterNode->psRightNode->pszValue);
        for (i=0; i<nLenght; i++)
        {
            if (!isdigit(psFilterNode->psRightNode->pszValue[i]) &&
                psFilterNode->psRightNode->pszValue[i] != '.')
            {
                /* if (psFilterNode->psRightNode->pszValue[i] < '0' || */
                /* psFilterNode->psRightNode->pszValue[i] > '9') */
                bString = 1;
                break;
            }
        }
    }
    
    /* specical case to be able to have empty strings in the expression. */
    if (psFilterNode->psRightNode->pszValue == NULL)
      bString = 1;
      

    if (bString)
      strcat(szBuffer, " (\"[");
    else
      strcat(szBuffer, " ([");
    /* attribute */

    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);
    if (bString)
      strcat(szBuffer, "]\" ");
    else
      strcat(szBuffer, "] "); 
    

    /* logical operator */
    if (strcasecmp(psFilterNode->pszValue, 
                   "PropertyIsEqualTo") == 0)
      strcat(szBuffer, "=");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsNotEqualTo") == 0)
      strcat(szBuffer, "!="); 
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThan") == 0)
      strcat(szBuffer, "<");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThan") == 0)
      strcat(szBuffer, ">");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThanOrEqualTo") == 0)
      strcat(szBuffer, "<=");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThanOrEqualTo") == 0)
      strcat(szBuffer, ">=");
    
    strcat(szBuffer, " ");
    
    /* value */
    if (bString)
      strcat(szBuffer, "\"");
    
    if (psFilterNode->psRightNode->pszValue)
      strcat(szBuffer, psFilterNode->psRightNode->pszValue);

    if (bString)
      strcat(szBuffer, "\"");
    strcat(szBuffer, ") ");

    return strdup(szBuffer);
}


/************************************************************************/
/*                    FLTGetIsBetweenComparisonExpresssion              */
/*                                                                      */
/*      Build expresssion for IsBteween Filter.                         */
/************************************************************************/
char *FLTGetIsBetweenComparisonExpresssion(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];
    char **aszBounds = NULL;
    int nBounds = 0;
    int i=0, bString=0, nLenght = 0;


    szBuffer[0] = '\0';
    if (!psFilterNode ||
        !(strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0))
      return NULL;

    if (!psFilterNode->psLeftNode || !psFilterNode->psRightNode )
      return NULL;

/* -------------------------------------------------------------------- */
/*      Get the bounds value which are stored like boundmin;boundmax    */
/* -------------------------------------------------------------------- */
    aszBounds = split(psFilterNode->psRightNode->pszValue, ';', &nBounds);
    if (nBounds != 2)
      return NULL;
/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (aszBounds[0])
    {
        nLenght = strlen(aszBounds[0]);
        for (i=0; i<nLenght; i++)
        {
            if (!isdigit(aszBounds[0][i]) && aszBounds[0][i] != '.')
            {
                bString = 1;
                break;
            }
        }
    }
    if (!bString)
    {
        if (aszBounds[1])
        {
            nLenght = strlen(aszBounds[1]);
            for (i=0; i<nLenght; i++)
            {
                if (!isdigit(aszBounds[1][i]) && aszBounds[1][i] != '.')
                {
                    bString = 1;
                    break;
                }
            }
        }
    }
        

/* -------------------------------------------------------------------- */
/*      build expresssion.                                              */
/* -------------------------------------------------------------------- */
    if (bString)
      strcat(szBuffer, " (\"[");
    else
      strcat(szBuffer, " ([");

    /* attribute */
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);

    if (bString)
      strcat(szBuffer, "]\" ");
    else
      strcat(szBuffer, "] ");
        
    
    strcat(szBuffer, " >= ");
    if (bString)
      strcat(szBuffer,"\"");
    strcat(szBuffer, aszBounds[0]);
    if (bString)
      strcat(szBuffer,"\"");

    strcat(szBuffer, " AND ");

    if (bString)
      strcat(szBuffer, " \"[");
    else
      strcat(szBuffer, " ["); 

    /* attribute */
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);
    
    if (bString)
      strcat(szBuffer, "]\" ");
    else
      strcat(szBuffer, "] ");
    
    strcat(szBuffer, " <= ");
    if (bString)
      strcat(szBuffer,"\"");
    strcat(szBuffer, aszBounds[1]);
    if (bString)
      strcat(szBuffer,"\"");
    strcat(szBuffer, ")");
     
    
    return strdup(szBuffer);
}
    
/************************************************************************/
/*                      FLTGetIsLikeComparisonExpression               */
/*                                                                      */
/*      Build expression for IsLike filter.                             */
/************************************************************************/
char *FLTGetIsLikeComparisonExpression(FilterEncodingNode *psFilterNode)
{
    char szBuffer[512];
    char *pszValue = NULL;
    
    char *pszWild = NULL;
    char *pszSingle = NULL;
    char *pszEscape = NULL;
    
    int nLength=0, i=0, iBuffer = 0;

    if (!psFilterNode || !psFilterNode->pOther || !psFilterNode->psLeftNode ||
        !psFilterNode->psRightNode || !psFilterNode->psRightNode->pszValue)
      return NULL;

    pszWild = ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard;
    pszSingle = ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar;
    pszEscape = ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar;

    if (!pszWild || strlen(pszWild) == 0 ||
        !pszSingle || strlen(pszSingle) == 0 || 
        !pszEscape || strlen(pszEscape) == 0)
      return NULL;

 
/* -------------------------------------------------------------------- */
/*      Use only the right node (which is the value), to build the      */
/*      regular expresssion. The left node will be used to build the    */
/*      classitem.                                                      */
/* -------------------------------------------------------------------- */
    szBuffer[0] = '/';
    szBuffer[1] = '\0';
    //szBuffer[1] = '^';
    pszValue = psFilterNode->psRightNode->pszValue;
    nLength = strlen(pszValue);
    iBuffer = 1;
    for (i=0; i<nLength; i++)
    {
        if (pszValue[i] != pszWild[0] && 
            pszValue[i] != pszSingle[0] &&
            pszValue[i] != pszEscape[0])
        {
            szBuffer[iBuffer] = pszValue[i];
            iBuffer++;
            szBuffer[iBuffer] = '\0';
        }
        else if  (pszValue[i] == pszSingle[0])
        {
             szBuffer[iBuffer] = '.';
             iBuffer++;
             szBuffer[iBuffer] = '\0';
        }
        else if  (pszValue[i] == pszEscape[0])
        {
            szBuffer[iBuffer] = '\\';
            iBuffer++;
            szBuffer[iBuffer] = '\0';
            /*if (i<nLength-1)
            {
                szBuffer[iBuffer] = pszValue[i+1];
                iBuffer++;
                szBuffer[iBuffer] = '\0';
            }
            */
        }
        else if (pszValue[i] == pszWild[0])
        {
            /* strcat(szBuffer, "[0-9,a-z,A-Z,\\s]*"); */
            /* iBuffer+=17; */
            strcat(szBuffer, ".*");
            iBuffer+=2;
            szBuffer[iBuffer] = '\0';
        }
    }   
    szBuffer[iBuffer] = '/';
    szBuffer[iBuffer+1] = '\0';
    
        
    return strdup(szBuffer);
}


/************************************************************************/
/*                           FTLHasSpatialFilter                        */
/*                                                                      */
/*      Utility function to see if a spatial filter is included in      */
/*      the node.                                                       */
/************************************************************************/
int FTLHasSpatialFilter(FilterEncodingNode *psNode)
{
    int bResult = MS_FALSE;

    if (!psNode)
      return MS_FALSE;

    if (psNode->eType == FILTER_NODE_TYPE_LOGICAL)
    {
        if (psNode->psLeftNode)
          bResult = FTLHasSpatialFilter(psNode->psLeftNode);

        if (bResult)
          return MS_TRUE;

        if (psNode->psRightNode)
           bResult = FTLHasSpatialFilter(psNode->psRightNode);

        if (bResult)
          return MS_TRUE;
    }
    else if (FLTIsBBoxFilter(psNode) || FLTIsPointFilter(psNode) ||
             FLTIsLineFilter(psNode) || FLTIsPolygonFilter(psNode))
      return MS_TRUE;


    return MS_FALSE;
}


#endif

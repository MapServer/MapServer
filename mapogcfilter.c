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


#define _GNU_SOURCE

#ifdef USE_OGR
#include "cpl_minixml.h"
#include "cpl_string.h"
#endif

#include "mapogcfilter.h"
#include "mapserver.h"
#include "mapowscommon.h"




int FLTIsNumeric(char *pszValue)
{
  if (pszValue != NULL && *pszValue != '\0' && !isspace(*pszValue)) {
    /*the regex seems to have a problem on windows when mapserver is built using
      PHP regex*/
#if defined(_WIN32) && !defined(__CYGWIN__)
    int i = 0, nLength=0, bString=0;

    nLength = strlen(pszValue);
    for (i=0; i<nLength; i++) {
      if (i == 0) {
        if (!isdigit(pszValue[i]) &&  pszValue[i] != '-') {
          bString = 1;
          break;
        }
      } else if (!isdigit(pszValue[i]) &&  pszValue[i] != '.') {
        bString = 1;
        break;
      }
    }
    if (!bString)
      return MS_TRUE;
#else
    char * p;
    strtod (pszValue, &p);
    if (*p == '\0') return MS_TRUE;
#endif
  }

  return MS_FALSE;
}

/*
** Apply an expression to the layer's filter element.
**
*/
int FLTApplyExpressionToLayer(layerObj *lp, char *pszExpression)
{
  char *pszFinalExpression=NULL, *pszBuffer = NULL;
  /*char *escapedTextString=NULL;*/
  int bConcatWhere=0, bHasAWhere=0;

  if (lp && pszExpression) {
    if (lp->connectiontype == MS_POSTGIS || lp->connectiontype ==  MS_ORACLESPATIAL ||
        lp->connectiontype == MS_SDE || lp->connectiontype == MS_PLUGIN) {
      pszFinalExpression = msStrdup("(");
      pszFinalExpression = msStringConcatenate(pszFinalExpression, pszExpression);
      pszFinalExpression = msStringConcatenate(pszFinalExpression, ")");
    } else if (lp->connectiontype == MS_OGR) {
      pszFinalExpression = msStrdup(pszExpression);
      if (lp->filter.type != MS_EXPRESSION) {
        bConcatWhere = 1;
      } else {
        if (lp->filter.string && EQUALN(lp->filter.string,"WHERE ",6)) {
          bHasAWhere = 1;
          bConcatWhere =1;
        }
      }

    } else
      pszFinalExpression = msStrdup(pszExpression);

    if (bConcatWhere)
      pszBuffer = msStringConcatenate(pszBuffer, "WHERE ");
    /* if the filter is set and it's an expression type, concatenate it with
                this filter. If not just free it */
    if (lp->filter.string && lp->filter.type == MS_EXPRESSION) {
      pszBuffer = msStringConcatenate(pszBuffer, "((");
      if (bHasAWhere)
        pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string+6);
      else
        pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string);
      pszBuffer = msStringConcatenate(pszBuffer, ") and ");
    } else if (lp->filter.string)
      freeExpression(&lp->filter);

    pszBuffer = msStringConcatenate(pszBuffer, pszFinalExpression);

    if(lp->filter.string && lp->filter.type == MS_EXPRESSION)
      pszBuffer = msStringConcatenate(pszBuffer, ")");

    /*assuming that expression was properly escaped
          escapedTextString = msStringEscape(pszBuffer);
          msLoadExpressionString(&lp->filter,
                                 (char*)CPLSPrintf("%s", escapedTextString));
          msFree(escapedTextString);
    */
    msLoadExpressionString(&lp->filter, pszBuffer);


    msFree(pszFinalExpression);

    if (pszBuffer)
      msFree(pszBuffer);

    return MS_TRUE;
  }

  return MS_FALSE;
}

char *FLTGetExpressionForValuesRanges(layerObj *lp, char *item, char *value,  int forcecharcter)
{
  int bIscharacter, bSqlLayer;
  char *pszExpression = NULL, *pszEscapedStr=NULL, *pszTmpExpression=NULL;
  char **paszElements = NULL, **papszRangeElements=NULL;
  int numelements,i,nrangeelements;
  /* double minval, maxval; */
  if (lp && item && value) {
    if (lp->connectiontype == MS_POSTGIS || lp->connectiontype ==  MS_ORACLESPATIAL ||
        lp->connectiontype == MS_SDE || lp->connectiontype == MS_PLUGIN)
      bSqlLayer = MS_TRUE;
    else
      bSqlLayer = MS_FALSE;

    if (strstr(value, "/") == NULL) {
      /*value(s)*/
      paszElements = msStringSplit (value, ',', &numelements);
      if (paszElements && numelements > 0) {
        if (forcecharcter)
          bIscharacter = MS_TRUE;
        bIscharacter= !FLTIsNumeric(paszElements[0]);

        pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
        for (i=0; i<numelements; i++) {
          pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
          if (bSqlLayer)
            pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
          else {
            if (bIscharacter)
              pszTmpExpression = msStringConcatenate(pszTmpExpression, "\"");
            pszTmpExpression = msStringConcatenate(pszTmpExpression, "[");
            pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
            pszTmpExpression = msStringConcatenate(pszTmpExpression, "]");
            if (bIscharacter)
              pszTmpExpression = msStringConcatenate(pszTmpExpression, "\"");
          }
          if (bIscharacter) {
            if (bSqlLayer)
              pszTmpExpression = msStringConcatenate(pszTmpExpression, " = '");
            else
              pszTmpExpression = msStringConcatenate(pszTmpExpression, " = \"");
          } else
            pszTmpExpression = msStringConcatenate(pszTmpExpression, " = ");

          pszEscapedStr = msLayerEscapeSQLParam(lp, paszElements[i]);
          pszTmpExpression = msStringConcatenate(pszTmpExpression, pszEscapedStr);

          if (bIscharacter) {
            if (bSqlLayer)
              pszTmpExpression = msStringConcatenate(pszTmpExpression, "'");
            else
              pszTmpExpression = msStringConcatenate(pszTmpExpression, "\"");
          }
          pszTmpExpression = msStringConcatenate(pszTmpExpression, ")");

          msFree(pszEscapedStr);
          pszEscapedStr=NULL;

          if (pszExpression != NULL)
            pszExpression = msStringConcatenate(pszExpression, " OR ");

          pszExpression =  msStringConcatenate(pszExpression, pszTmpExpression);

          msFree(pszTmpExpression);
          pszTmpExpression = NULL;
        }
        pszExpression = msStringConcatenate(pszExpression, ")");
        msFreeCharArray(paszElements, numelements);
      }
    } else {
      /*range(s)*/
      paszElements = msStringSplit (value, ',', &numelements);
      if (paszElements && numelements > 0) {
        pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
        for (i=0; i<numelements; i++) {
          papszRangeElements = msStringSplit (paszElements[i], '/', &nrangeelements);
          if (papszRangeElements && nrangeelements > 0) {
            pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
            if (nrangeelements == 2 || nrangeelements == 3) {
              /*
              minval = atof(papszRangeElements[0]);
              maxval = atof(papszRangeElements[1]);
              */
              if (bSqlLayer)
                pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
              else {
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "[");
                pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "]");
              }

              pszTmpExpression = msStringConcatenate(pszTmpExpression, " >= ");

              pszEscapedStr = msLayerEscapeSQLParam(lp, papszRangeElements[0]);
              pszTmpExpression = msStringConcatenate(pszTmpExpression, pszEscapedStr);
              msFree(pszEscapedStr);
              pszEscapedStr=NULL;

              pszTmpExpression = msStringConcatenate(pszTmpExpression, " AND ");

              if (bSqlLayer)
                pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
              else {
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "[");
                pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "]");
              }

              pszTmpExpression = msStringConcatenate(pszTmpExpression, " <= ");

              pszEscapedStr = msLayerEscapeSQLParam(lp, papszRangeElements[1]);
              pszTmpExpression = msStringConcatenate(pszTmpExpression, pszEscapedStr);
              msFree(pszEscapedStr);
              pszEscapedStr=NULL;

              pszTmpExpression = msStringConcatenate(pszTmpExpression, ")");
            } else if (nrangeelements == 1) {
              pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
              if (bSqlLayer)
                pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
              else {
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "[");
                pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "]");
              }

              pszTmpExpression = msStringConcatenate(pszTmpExpression, " = ");

              pszEscapedStr = msLayerEscapeSQLParam(lp, papszRangeElements[0]);
              pszTmpExpression = msStringConcatenate(pszTmpExpression, pszEscapedStr);
              msFree(pszEscapedStr);
              pszEscapedStr=NULL;

              pszTmpExpression = msStringConcatenate(pszTmpExpression, ")");
            }

            if (pszExpression != NULL)
              pszExpression = msStringConcatenate(pszExpression, " OR ");

            pszExpression =  msStringConcatenate(pszExpression, pszTmpExpression);
            msFree(pszTmpExpression);
            pszTmpExpression = NULL;

            msFreeCharArray(papszRangeElements, nrangeelements);
          }
        }
        pszExpression = msStringConcatenate(pszExpression, ")");
        msFreeCharArray(paszElements, numelements);
      }
    }
  }
  return pszExpression;
}

#ifdef USE_OGR


int FLTogrConvertGeometry(OGRGeometryH hGeometry, shapeObj *psShape,
                          OGRwkbGeometryType nType)
{
  return msOGRGeometryToShape(hGeometry, psShape, nType);
}

int FLTShapeFromGMLTree(CPLXMLNode *psTree, shapeObj *psShape , char **ppszSRS)
{
  char *pszSRS = NULL;
  if (psTree && psShape) {
    CPLXMLNode *psNext = psTree->psNext;
    OGRGeometryH hGeometry = NULL;

    psTree->psNext = NULL;
    hGeometry = OGR_G_CreateFromGMLTree(psTree );
    psTree->psNext = psNext;

    if (hGeometry) {
      OGRwkbGeometryType nType;
      nType = OGR_G_GetGeometryType(hGeometry);
      if (nType == wkbPolygon25D || nType == wkbMultiPolygon25D)
        nType = wkbPolygon;
      else if (nType == wkbLineString25D || nType == wkbMultiLineString25D)
        nType = wkbLineString;
      else if (nType == wkbPoint25D  || nType ==  wkbMultiPoint25D)
        nType = wkbPoint;
      FLTogrConvertGeometry(hGeometry, psShape, nType);

      OGR_G_DestroyGeometry(hGeometry);
    }

    pszSRS = (char *)CPLGetXMLValue(psTree, "srsName", NULL);
    if (ppszSRS && pszSRS)
      *ppszSRS = msStrdup(pszSRS);

    return MS_TRUE;
  }

  return MS_FALSE;
}

int FLTGetGeosOperator(char *pszValue)
{
  if (!pszValue)
    return -1;

  if (strcasecmp(pszValue, "Equals") == 0)
    return MS_GEOS_EQUALS;
  else if (strcasecmp(pszValue, "Intersect") == 0 ||
           strcasecmp(pszValue, "Intersects") == 0)
    return MS_GEOS_INTERSECTS;
  else if (strcasecmp(pszValue, "Disjoint") == 0)
    return MS_GEOS_DISJOINT;
  else if (strcasecmp(pszValue, "Touches") == 0)
    return MS_GEOS_TOUCHES;
  else if (strcasecmp(pszValue, "Crosses") == 0)
    return MS_GEOS_CROSSES;
  else if (strcasecmp(pszValue, "Within") == 0)
    return MS_GEOS_WITHIN;
  else if (strcasecmp(pszValue, "Contains") == 0)
    return MS_GEOS_CONTAINS;
  else if (strcasecmp(pszValue, "Overlaps") == 0)
    return MS_GEOS_OVERLAPS;
  else if (strcasecmp(pszValue, "Beyond") == 0)
    return MS_GEOS_BEYOND;
  else if (strcasecmp(pszValue, "DWithin") == 0)
    return MS_GEOS_DWITHIN;

  return -1;
}

int FLTIsGeosNode(char *pszValue)
{
  if (FLTGetGeosOperator(pszValue) == -1)
    return MS_FALSE;

  return MS_TRUE;
}

int FLTParseEpsgString(char *pszEpsg, projectionObj *psProj)
{
  int nStatus = MS_FALSE;
  int nTokens = 0;
  char **tokens = NULL;
  int nEpsgTmp=0;
  size_t bufferSize = 0;

#ifdef USE_PROJ
  if (pszEpsg && psProj) {
    nTokens = 0;
    tokens = msStringSplit(pszEpsg,'#', &nTokens);
    if (tokens && nTokens == 2) {
      char *szTmp;
      bufferSize = 10+strlen(tokens[1])+1;
      szTmp = (char *)malloc(bufferSize);
      snprintf(szTmp, bufferSize, "init=epsg:%s", tokens[1]);
      msInitProjection(psProj);
      if (msLoadProjectionString(psProj, szTmp) == 0)
        nStatus = MS_TRUE;
      free(szTmp);
    } else if (tokens &&  nTokens == 1) {
      if (tokens)
        msFreeCharArray(tokens, nTokens);
      nTokens = 0;

      tokens = msStringSplit(pszEpsg,':', &nTokens);
      nEpsgTmp = -1;
      if (tokens &&  nTokens == 1) {
        nEpsgTmp = atoi(tokens[0]);

      } else if (tokens &&  nTokens == 2) {
        nEpsgTmp = atoi(tokens[1]);
      }
      if (nEpsgTmp > 0) {
        char szTmp[32];
        snprintf(szTmp, sizeof(szTmp), "init=epsg:%d",nEpsgTmp);
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



int FLTGML2Shape_XMLNode(CPLXMLNode *psNode, shapeObj *psShp)
{
  lineObj line= {0,NULL};
  CPLXMLNode *psCoordinates = NULL;
  char *pszTmpCoord = NULL;
  char **szCoords = NULL;
  int nCoords = 0;


  if (!psNode || !psShp)
    return MS_FALSE;


  if( strcasecmp(psNode->pszValue,"PointType") == 0
      || strcasecmp(psNode->pszValue,"Point") == 0) {
    psCoordinates = CPLGetXMLNode(psNode, "coordinates");

    if (psCoordinates && psCoordinates->psChild &&
        psCoordinates->psChild->pszValue) {
      pszTmpCoord = psCoordinates->psChild->pszValue;
      szCoords = msStringSplit(pszTmpCoord, ',', &nCoords);
      if (szCoords && nCoords >= 2) {
        line.numpoints = 1;
        line.point = (pointObj *)malloc(sizeof(pointObj));
        line.point[0].x = atof(szCoords[0]);
        line.point[0].y =  atof(szCoords[1]);
#ifdef USE_POINT_Z_M
        line.point[0].m = 0;
#endif

        psShp->type = MS_SHAPE_POINT;

        msAddLine(psShp, &line);
        free(line.point);

        return MS_TRUE;
      }
    }
  }

  return MS_FALSE;
}




/************************************************************************/
/*                        FLTIsSimpleFilterNoSpatial                    */
/*                                                                      */
/*      Filter encoding with only attribute queries                     */
/************************************************************************/
int FLTIsSimpleFilterNoSpatial(FilterEncodingNode *psNode)
{
  if (FLTIsSimpleFilter(psNode) &&
      FLTNumberOfFilterType(psNode, "BBOX") == 0)
    return MS_TRUE;

  return MS_FALSE;
}




/************************************************************************/
/*                      FLTApplySimpleSQLFilter()                       */
/************************************************************************/

int FLTApplySimpleSQLFilter(FilterEncodingNode *psNode, mapObj *map,
                            int iLayerIndex)
{
  layerObj *lp = NULL;
  char *szExpression = NULL;
  rectObj sQueryRect = map->extent;
  char *szEPSG = NULL;
  char **tokens = NULL;
  int nTokens = 0, nEpsgTmp = 0;
  projectionObj sProjTmp;
  char *pszBuffer = NULL;
  int bConcatWhere = 0;
  int bHasAWhere =0;
  char *pszTmp = NULL, *pszTmp2 = NULL;
  size_t bufferSize = 0;
  char *tmpfilename = NULL;

  lp = (GET_LAYER(map, iLayerIndex));

  /* if there is a bbox use it */
  szEPSG = FLTGetBBOX(psNode, &sQueryRect);
  if(szEPSG && map->projection.numargs > 0) {
#ifdef USE_PROJ
    nTokens = 0;
    tokens = msStringSplit(szEPSG,'#', &nTokens);
    if (tokens && nTokens == 2) {
      char *szTmp;
      bufferSize = 10+strlen(tokens[1])+1;
      szTmp = (char *)malloc(bufferSize);
      snprintf(szTmp, bufferSize, "init=epsg:%s",tokens[1]);
      msInitProjection(&sProjTmp);
      if (msLoadProjectionString(&sProjTmp, szTmp) == 0)
        msProjectRect(&sProjTmp, &map->projection,  &sQueryRect);
      free(szTmp);
    } else if (tokens &&  nTokens == 1) {
      if (tokens)
        msFreeCharArray(tokens, nTokens);
      nTokens = 0;

      tokens = msStringSplit(szEPSG,':', &nTokens);
      nEpsgTmp = -1;
      if (tokens &&  nTokens == 1) {
        nEpsgTmp = atoi(tokens[0]);

      } else if (tokens &&  nTokens == 2) {
        nEpsgTmp = atoi(tokens[1]);
      }
      if (nEpsgTmp > 0) {
        char szTmp[32];
        snprintf(szTmp, sizeof(szTmp), "init=epsg:%d",nEpsgTmp);
        msInitProjection(&sProjTmp);
        if (msLoadProjectionString(&sProjTmp, szTmp) == 0)
          msProjectRect(&sProjTmp, &map->projection,  &sQueryRect);
        msFreeProjection(&sProjTmp);
      }
    }
    if (tokens)
      msFreeCharArray(tokens, nTokens);
#endif
  }

  /* make sure that the layer can be queried*/
  if (!lp->template)
    lp->template = msStrdup("ttt.html");

  /* if there is no class, create at least one, so that query by rect
     would work*/
  if (lp->numclasses == 0) {
    if (msGrowLayerClasses(lp) == NULL)
      return MS_FAILURE;
    initClass(lp->class[0]);
  }

  bConcatWhere = 0;
  bHasAWhere = 0;
  if (lp->connectiontype == MS_POSTGIS || lp->connectiontype ==  MS_ORACLESPATIAL ||
      lp->connectiontype == MS_SDE || lp->connectiontype == MS_PLUGIN) {
    szExpression = FLTGetSQLExpression(psNode, lp);
    if (szExpression) {
      pszTmp = msStrdup("(");
      pszTmp = msStringConcatenate(pszTmp, szExpression);
      pszTmp = msStringConcatenate(pszTmp, ")");
      msFree(szExpression);
      szExpression = pszTmp;
    }
  }
  /* concatenates the WHERE clause for OGR layers. This only applies if
     the expression was empty or not of an expression string. If there
     is an sql type expression, it is assumed to have the WHERE clause.
     If it is an expression and does not have a WHERE it is assumed to be a mapserver
     type expression*/
  else if (lp->connectiontype == MS_OGR) {
    if (lp->filter.type != MS_EXPRESSION) {
      szExpression = FLTGetSQLExpression(psNode, lp);
      bConcatWhere = 1;
    } else {
      if (lp->filter.string && EQUALN(lp->filter.string,"WHERE ",6)) {
        szExpression = FLTGetSQLExpression(psNode, lp);
        bHasAWhere = 1;
        bConcatWhere =1;
      } else {
        szExpression = FLTGetCommonExpression(psNode, lp);
      }
    }
  } else {
    szExpression = FLTGetCommonExpression(psNode, lp);

  }


  if (szExpression) {

    if (bConcatWhere)
      pszBuffer = msStringConcatenate(pszBuffer, "WHERE ");

    /* if the filter is set and it's an expression type, concatenate it with
                 this filter. If not just free it */
    if (lp->filter.string && lp->filter.type == MS_EXPRESSION) {
      pszBuffer = msStringConcatenate(pszBuffer, "((");
      if (bHasAWhere)
        pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string+6);
      else
        pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string);
      pszBuffer = msStringConcatenate(pszBuffer, ") and ");
    } else if (lp->filter.string)
      freeExpression(&lp->filter);


    pszBuffer = msStringConcatenate(pszBuffer, szExpression);

    if(lp->filter.string && lp->filter.type == MS_EXPRESSION)
      pszBuffer = msStringConcatenate(pszBuffer, ")");

    msLoadExpressionString(&lp->filter, pszBuffer);
    free(szExpression);
  }

  if (pszBuffer)
    free(pszBuffer);

  map->query.type = MS_QUERY_BY_RECT;
  map->query.mode = MS_QUERY_MULTIPLE;
  map->query.layer = lp->index;
  map->query.rect = sQueryRect;

  if(map->debug == MS_DEBUGLEVEL_VVV) {
    tmpfilename = msTmpFile(map, map->mappath, NULL, "_filter.map");
    if (tmpfilename == NULL) {
      tmpfilename = msTmpFile(map, NULL, NULL, "_filter.map" );
    }
    if (tmpfilename) {
      msSaveMap(map,tmpfilename);
      msDebug("FLTApplySimpleSQLFilter(): Map file after Filter was applied %s", tmpfilename);
      msFree(tmpfilename);
    }
  }

  /*for oracle connection, if we have a simple filter with no spatial constraints
    we should set the connection function to NONE to have a better performance
    (#2725)*/

  if (lp->connectiontype ==  MS_ORACLESPATIAL && FLTIsSimpleFilterNoSpatial(psNode)) {
    if (strcasestr(lp->data, "USING") == 0)
      lp->data = msStringConcatenate(lp->data, " USING NONE");
    else if (strcasestr(lp->data, "NONE") == 0) {
      /*if one of the functions is used, just replace it with NONE*/
      if (strcasestr(lp->data, "FILTER"))
        lp->data = msCaseReplaceSubstring(lp->data, "FILTER", "NONE");
      else if (strcasestr(lp->data, "GEOMRELATE"))
        lp->data = msCaseReplaceSubstring(lp->data, "GEOMRELATE", "NONE");
      else if (strcasestr(lp->data, "RELATE"))
        lp->data = msCaseReplaceSubstring(lp->data, "RELATE", "NONE");
      else if (strcasestr(lp->data, "VERSION")) {
        /*should add NONE just before the VERSION. Cases are:
          DATA "ORA_GEOMETRY FROM data USING VERSION 10g
          DATA "ORA_GEOMETRY FROM data  USING UNIQUE FID VERSION 10g"
         */
        pszTmp = (char *)strcasestr(lp->data, "VERSION");
        pszTmp2 = msStringConcatenate(pszTmp2, " NONE ");
        pszTmp2 = msStringConcatenate(pszTmp2, pszTmp);

        lp->data = msCaseReplaceSubstring(lp->data, pszTmp, pszTmp2);

        msFree(pszTmp2);

      } else if (strcasestr(lp->data, "SRID")) {
        lp->data = msStringConcatenate(lp->data, " NONE");
      }
    }
  }

  return msQueryByRect(map);

  /* return MS_SUCCESS; */
}





/************************************************************************/
/*                            FLTIsSimpleFilter                         */
/*                                                                      */
/*      Filter encoding with only attribute queries and only one bbox.  */
/************************************************************************/
int FLTIsSimpleFilter(FilterEncodingNode *psNode)
{
  if (FLTValidForBBoxFilter(psNode)) {
    if (FLTNumberOfFilterType(psNode, "DWithin") == 0 &&
        FLTNumberOfFilterType(psNode, "Intersect") == 0 &&
        FLTNumberOfFilterType(psNode, "Intersects") == 0 &&
        FLTNumberOfFilterType(psNode, "Equals") == 0 &&
        FLTNumberOfFilterType(psNode, "Disjoint") == 0 &&
        FLTNumberOfFilterType(psNode, "Touches") == 0 &&
        FLTNumberOfFilterType(psNode, "Crosses") == 0 &&
        FLTNumberOfFilterType(psNode, "Within") == 0 &&
        FLTNumberOfFilterType(psNode, "Contains") == 0 &&
        FLTNumberOfFilterType(psNode, "Overlaps") == 0 &&
        FLTNumberOfFilterType(psNode, "Beyond") == 0)
      return TRUE;
  }

  return FALSE;
}



/************************************************************************/
/*                          FLTApplyFilterToLayer                       */
/*                                                                      */
/*      Use the filter encoding node to create mapserver expressions    */
/*      and apply it to the layer.                                      */
/************************************************************************/
int FLTApplyFilterToLayer(FilterEncodingNode *psNode, mapObj *map,
                          int iLayerIndex)
{
  layerObj *layer = GET_LAYER(map, iLayerIndex);

  if ( ! layer->vtable) {
    int rv =  msInitializeVirtualTable(layer);
    if (rv != MS_SUCCESS)
      return rv;
  }
  return layer->vtable->LayerApplyFilterToLayer(psNode, map,  iLayerIndex);

}

/************************************************************************/
/*               FLTLayerApplyCondSQLFilterToLayer                       */
/*                                                                      */
/* Helper function for layer virtual table architecture                 */
/************************************************************************/
int FLTLayerApplyCondSQLFilterToLayer(FilterEncodingNode *psNode, mapObj *map,
                                      int iLayerIndex)
{
  /* ==================================================================== */
  /*      Check here to see if it is a simple filter and if that is       */
  /*      the case, we are going to use the FILTER element on             */
  /*      the layer.                                                      */
  /* ==================================================================== */
  if (FLTIsSimpleFilter(psNode)) {
    return FLTApplySimpleSQLFilter(psNode, map, iLayerIndex);
  }

  return FLTLayerApplyPlainFilterToLayer(psNode, map, iLayerIndex);
}

/************************************************************************/
/*                   FLTLayerApplyPlainFilterToLayer                    */
/*                                                                      */
/* Helper function for layer virtual table architecture                 */
/************************************************************************/
int FLTLayerApplyPlainFilterToLayer(FilterEncodingNode *psNode, mapObj *map,
                                    int iLayerIndex)
{
  char *pszExpression  =NULL;
  int status =MS_FALSE;

  pszExpression = FLTGetCommonExpression(psNode,  GET_LAYER(map, iLayerIndex));
  if (pszExpression) {
    status = FLTApplyFilterToLayerCommonExpression(map, iLayerIndex, pszExpression);
    msFree(pszExpression);
  }

  return status;
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

  /* strip namespaces. We srtip all name spaces (#1350)*/
  CPLStripXMLNamespace(psRoot, NULL, 1);

  /* -------------------------------------------------------------------- */
  /*      get the root element (Filter).                                  */
  /* -------------------------------------------------------------------- */
  psChild = psRoot;
  psFilter = NULL;

  while( psChild != NULL ) {
    if (psChild->eType == CXT_Element &&
        EQUAL(psChild->pszValue,"Filter")) {
      psFilter = psChild;
      break;
    } else
      psChild = psChild->psNext;
  }

  if (!psFilter)
    return NULL;

  psChild = psFilter->psChild;
  psFilterStart = NULL;
  while (psChild) {
    if (FLTIsSupportedFilterType(psChild)) {
      psFilterStart = psChild;
      psChild = NULL;
    } else
      psChild = psChild->psNext;
  }

  if (psFilterStart && FLTIsSupportedFilterType(psFilterStart)) {
    psFilterNode = FLTCreateFilterEncodingNode();

    FLTInsertElementInNode(psFilterNode, psFilterStart);
  }

  CPLDestroyXMLNode( psRoot );

  /* -------------------------------------------------------------------- */
  /*      validate the node tree to make sure that all the nodes are valid.*/
  /* -------------------------------------------------------------------- */
  if (!FLTValidFilterNode(psFilterNode))
    return NULL;


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

  if (psFilterNode->psLeftNode) {
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
  if (psFilterNode) {
    if (psFilterNode->psLeftNode) {
      FLTFreeFilterEncodingNode(psFilterNode->psLeftNode);
      psFilterNode->psLeftNode = NULL;
    }
    if (psFilterNode->psRightNode) {
      FLTFreeFilterEncodingNode(psFilterNode->psRightNode);
      psFilterNode->psRightNode = NULL;
    }

    if (psFilterNode->pszSRS)
      free( psFilterNode->pszSRS);

    if( psFilterNode->pOther ) {
      if (psFilterNode->pszValue != NULL &&
          strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0) {
        if( ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard )
          free( ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard );
        if( ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar )
          free( ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar );
        if( ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar )
          free( ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar );
      } else if (psFilterNode->eType == FILTER_NODE_TYPE_GEOMETRY_POINT ||
                 psFilterNode->eType == FILTER_NODE_TYPE_GEOMETRY_LINE ||
                 psFilterNode->eType == FILTER_NODE_TYPE_GEOMETRY_POLYGON) {
        msFreeShape((shapeObj *)(psFilterNode->pOther));
      }
      /* else */
      /* TODO free pOther special fields */
      free( psFilterNode->pOther );
    }

    /* Cannot free pszValue before, 'cause we are testing it above */
    if( psFilterNode->pszValue )
      free( psFilterNode->pszValue );

    free(psFilterNode);
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
  psFilterNode->pszSRS = NULL;
  psFilterNode->psLeftNode = NULL;
  psFilterNode->psRightNode = NULL;

  return psFilterNode;
}

FilterEncodingNode *FLTCreateBinaryCompFilterEncodingNode(void)
{
  FilterEncodingNode *psFilterNode = NULL;

  psFilterNode = FLTCreateFilterEncodingNode();
  /* used to store case sensitivity flag. Default is 0 meaning the
     comparing is case snetitive */
  psFilterNode->pOther = (int *)malloc(sizeof(int));
  (*(int *)(psFilterNode->pOther)) = 0;

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
  FilterEncodingNode *psCurFilNode= NULL;
  CPLXMLNode *psCurXMLNode = NULL;
  CPLXMLNode *psTmpNode = NULL;
  CPLXMLNode *psFeatureIdNode = NULL;
  char *pszFeatureId=NULL, *pszFeatureIdList=NULL;
  char **tokens = NULL;
  int nTokens = 0;

  if (psFilterNode && psXMLNode && psXMLNode->pszValue) {
    psFilterNode->pszValue = msStrdup(psXMLNode->pszValue);
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
    if (FLTIsLogicalFilterType(psXMLNode->pszValue)) {
      psFilterNode->eType = FILTER_NODE_TYPE_LOGICAL;
      if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
          strcasecmp(psFilterNode->pszValue, "OR") == 0) {
        if (psXMLNode->psChild && psXMLNode->psChild->psNext) {
          /*2 operators */
          if (psXMLNode->psChild->psNext->psNext == NULL) {
            psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
            FLTInsertElementInNode(psFilterNode->psLeftNode,
                                   psXMLNode->psChild);
            psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
            FLTInsertElementInNode(psFilterNode->psRightNode,
                                   psXMLNode->psChild->psNext);
          } else {
            psCurXMLNode =  psXMLNode->psChild;
            psCurFilNode = psFilterNode;
            while(psCurXMLNode) {
              if (psCurXMLNode->psNext && psCurXMLNode->psNext->psNext) {
                psCurFilNode->psLeftNode =
                FLTCreateFilterEncodingNode();
                FLTInsertElementInNode(psCurFilNode->psLeftNode,
                                       psCurXMLNode);
                psCurFilNode->psRightNode =
                FLTCreateFilterEncodingNode();
                psCurFilNode->psRightNode->eType =
                FILTER_NODE_TYPE_LOGICAL;
                psCurFilNode->psRightNode->pszValue =
                msStrdup(psFilterNode->pszValue);

                psCurFilNode = psCurFilNode->psRightNode;
                psCurXMLNode = psCurXMLNode->psNext;
              } else if (psCurXMLNode->psNext) { /*last 2 operators*/
                psCurFilNode->psLeftNode =
                FLTCreateFilterEncodingNode();
                FLTInsertElementInNode(psCurFilNode->psLeftNode,
                                       psCurXMLNode);

                psCurFilNode->psRightNode =
                FLTCreateFilterEncodingNode();
                FLTInsertElementInNode(psCurFilNode->psRightNode,
                                       psCurXMLNode->psNext);
                psCurXMLNode = psCurXMLNode->psNext->psNext;

              }
            }
          }
        }
      } else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0) {
        if (psXMLNode->psChild) {
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
          FLTInsertElementInNode(psFilterNode->psLeftNode,
                                 psXMLNode->psChild);
        }
      } else
        psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
    }/* end if is logical */
    /* -------------------------------------------------------------------- */
    /*      Spatial Filter.                                                 */
    /*      BBOX :                                                          */
    /*      <Filter>                                                        */
    /*       <BBOX>                                                         */
    /*        <PropertyName>Geometry</PropertyName>                         */
    /*        <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326">*/
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
    else if (FLTIsSpatialFilterType(psXMLNode->pszValue)) {
      psFilterNode->eType = FILTER_NODE_TYPE_SPATIAL;

      if (strcasecmp(psXMLNode->pszValue, "BBOX") == 0) {
        char *pszSRS = NULL;
        CPLXMLNode *psPropertyName = NULL;
        CPLXMLNode *psBox = NULL, *psEnvelope=NULL;
        rectObj sBox;

        int bCoordinatesValid = 0;

        psPropertyName = CPLGetXMLNode(psXMLNode, "PropertyName");
        psBox = CPLGetXMLNode(psXMLNode, "Box");
        if (!psBox)
          psBox = CPLGetXMLNode(psXMLNode, "BoxType");

        /*FE 1.0 used box FE1.1 uses envelop*/
        if (psBox)
          bCoordinatesValid = FLTParseGMLBox(psBox, &sBox, &pszSRS);
        else if ((psEnvelope = CPLGetXMLNode(psXMLNode, "Envelope")))
          bCoordinatesValid = FLTParseGMLEnvelope(psEnvelope, &sBox, &pszSRS);

        if (psPropertyName == NULL || !bCoordinatesValid)
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;

        if (psPropertyName && bCoordinatesValid) {
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
          /* not really using the property name anywhere ??  */
          /* Is is always Geometry ?  */
          if (psPropertyName->psChild &&
              psPropertyName->psChild->pszValue) {
            psFilterNode->psLeftNode->eType =
            FILTER_NODE_TYPE_PROPERTYNAME;
            psFilterNode->psLeftNode->pszValue =
            msStrdup(psPropertyName->psChild->pszValue);
          }

          /* srs and coordinates */
          psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BBOX;
          /* srs might be empty */
          if (pszSRS)
            psFilterNode->psRightNode->pszValue = pszSRS;

          psFilterNode->psRightNode->pOther =
          (rectObj *)msSmallMalloc(sizeof(rectObj));
          ((rectObj *)psFilterNode->psRightNode->pOther)->minx = sBox.minx;
          ((rectObj *)psFilterNode->psRightNode->pOther)->miny = sBox.miny;
          ((rectObj *)psFilterNode->psRightNode->pOther)->maxx = sBox.maxx;
          ((rectObj *)psFilterNode->psRightNode->pOther)->maxy =  sBox.maxy;
        }
      } else if (strcasecmp(psXMLNode->pszValue, "DWithin") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Beyond") == 0)

      {
        shapeObj *psShape = NULL;
        int bPoint = 0, bLine = 0, bPolygon = 0;
        char *pszUnits = NULL;
        char *pszSRS = NULL;


        CPLXMLNode *psGMLElement = NULL, *psDistance=NULL;


        psGMLElement = CPLGetXMLNode(psXMLNode, "Point");
        if (!psGMLElement)
          psGMLElement =  CPLGetXMLNode(psXMLNode, "PointType");
        if (psGMLElement)
          bPoint =1;
        else {
          psGMLElement= CPLGetXMLNode(psXMLNode, "Polygon");
          if (psGMLElement)
            bPolygon = 1;
          else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "MultiPolygon")))
            bPolygon = 1;
          else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "MultiSurface")))
            bPolygon = 1;
          else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "Box")))
            bPolygon = 1;
          else {
            psGMLElement= CPLGetXMLNode(psXMLNode, "LineString");
            if (psGMLElement)
              bLine = 1;
          }
        }

        psDistance = CPLGetXMLNode(psXMLNode, "Distance");
        if (psGMLElement && psDistance && psDistance->psChild &&
            psDistance->psChild->psNext && psDistance->psChild->psNext->pszValue) {
          pszUnits = (char *)CPLGetXMLValue(psDistance, "units", NULL);
          psShape = (shapeObj *)msSmallMalloc(sizeof(shapeObj));
          msInitShape(psShape);
          if (FLTShapeFromGMLTree(psGMLElement, psShape, &pszSRS))
            /* if (FLTGML2Shape_XMLNode(psPoint, psShape)) */
          {
            /*set the srs if available*/
            if (pszSRS)
              psFilterNode->pszSRS = pszSRS;

            psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
            /* not really using the property name anywhere ?? Is is always */
            /* Geometry ?  */

            psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
            psFilterNode->psLeftNode->pszValue = msStrdup("Geometry");

            psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
            if (bPoint)
              psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POINT;
            else if (bLine)
              psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_LINE;
            else if (bPolygon)
              psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POLYGON;
            psFilterNode->psRightNode->pOther = (shapeObj *)psShape;
            /*the value will be distance;units*/
            psFilterNode->psRightNode->pszValue =
            msStrdup(psDistance->psChild->psNext->pszValue);
            if (pszUnits) {
              psFilterNode->psRightNode->pszValue= msStringConcatenate(psFilterNode->psRightNode->pszValue, ";");
              psFilterNode->psRightNode->pszValue= msStringConcatenate(psFilterNode->psRightNode->pszValue, pszUnits);
            }
          }
        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
      } else if (strcasecmp(psXMLNode->pszValue, "Intersect") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Intersects") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Equals") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Disjoint") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Touches") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Crosses") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Within") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Contains") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Overlaps") == 0) {
        shapeObj *psShape = NULL;
        int  bLine = 0, bPolygon = 0, bPoint=0;
        char *pszSRS = NULL;


        CPLXMLNode *psGMLElement = NULL;


        psGMLElement = CPLGetXMLNode(psXMLNode, "Polygon");
        if (psGMLElement)
          bPolygon = 1;
        else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "MultiPolygon")))
          bPolygon = 1;
        else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "MultiSurface")))
          bPolygon = 1;
        else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "Box")))
          bPolygon = 1;
        else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "LineString"))) {
          if (psGMLElement)
            bLine = 1;
        }

        else {
          psGMLElement = CPLGetXMLNode(psXMLNode, "Point");
          if (!psGMLElement)
            psGMLElement =  CPLGetXMLNode(psXMLNode, "PointType");
          if (psGMLElement)
            bPoint =1;
        }

        if (psGMLElement) {
          psShape = (shapeObj *)msSmallMalloc(sizeof(shapeObj));
          msInitShape(psShape);
          if (FLTShapeFromGMLTree(psGMLElement, psShape, &pszSRS))
            /* if (FLTGML2Shape_XMLNode(psPoint, psShape)) */
          {
            /*set the srs if available*/
            if (pszSRS)
              psFilterNode->pszSRS = pszSRS;

            psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
            /* not really using the property name anywhere ?? Is is always */
            /* Geometry ?  */

            psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
            psFilterNode->psLeftNode->pszValue = msStrdup("Geometry");

            psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
            if (bPoint)
              psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POINT;
            if (bLine)
              psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_LINE;
            else if (bPolygon)
              psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POLYGON;
            psFilterNode->psRightNode->pOther = (shapeObj *)psShape;

          }
        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
      }


    }/* end of is spatial */


    /* -------------------------------------------------------------------- */
    /*      Comparison Filter                                               */
    /* -------------------------------------------------------------------- */
    else if (FLTIsComparisonFilterType(psXMLNode->pszValue)) {
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
      if (FLTIsBinaryComparisonFilterType(psXMLNode->pszValue)) {
        psTmpNode = CPLSearchXMLNode(psXMLNode,  "PropertyName");
        if (psTmpNode &&  psTmpNode->psChild &&
            psTmpNode->psChild->pszValue &&
            strlen(psTmpNode->psChild->pszValue) > 0) {
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
          psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
          psFilterNode->psLeftNode->pszValue =
            msStrdup(psTmpNode->psChild->pszValue);

          psTmpNode = CPLSearchXMLNode(psXMLNode,  "Literal");
          if (psTmpNode) {
            psFilterNode->psRightNode = FLTCreateBinaryCompFilterEncodingNode();
            psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;

            if (psTmpNode &&
                psTmpNode->psChild &&
                psTmpNode->psChild->pszValue &&
                strlen(psTmpNode->psChild->pszValue) > 0) {

              psFilterNode->psRightNode->pszValue =
                msStrdup(psTmpNode->psChild->pszValue);

              /*check if the matchCase attribute is set*/
              if (psXMLNode->psChild &&
                  psXMLNode->psChild->eType == CXT_Attribute  &&
                  psXMLNode->psChild->pszValue &&
                  strcasecmp(psXMLNode->psChild->pszValue, "matchCase") == 0 &&
                  psXMLNode->psChild->psChild &&
                  psXMLNode->psChild->psChild->pszValue &&
                  strcasecmp( psXMLNode->psChild->psChild->pszValue, "false") == 0) {
                (*(int *)psFilterNode->psRightNode->pOther) = 1;
              }

            }
            /* special case where the user puts an empty value */
            /* for the Literal so it can end up as an empty  */
            /* string query in the expression */
            else
              psFilterNode->psRightNode->pszValue = NULL;
          }
        }
        if (psFilterNode->psLeftNode == NULL || psFilterNode->psRightNode == NULL)
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
      else if (strcasecmp(psXMLNode->pszValue, "PropertyIsBetween") == 0) {
        CPLXMLNode *psUpperNode = NULL, *psLowerNode = NULL;
        if (psXMLNode->psChild &&
            strcasecmp(psXMLNode->psChild->pszValue,
                       "PropertyName") == 0 &&
            psXMLNode->psChild->psNext &&
            strcasecmp(psXMLNode->psChild->psNext->pszValue,
                       "LowerBoundary") == 0 &&
            psXMLNode->psChild->psNext->psNext &&
            strcasecmp(psXMLNode->psChild->psNext->psNext->pszValue,
                       "UpperBoundary") == 0) {
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();

          if (psXMLNode->psChild->psChild &&
              psXMLNode->psChild->psChild->pszValue) {
            psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
            psFilterNode->psLeftNode->pszValue =
              msStrdup(psXMLNode->psChild->psChild->pszValue);
          }

          psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
          if (psXMLNode->psChild->psNext->psChild &&
              psXMLNode->psChild->psNext->psNext->psChild &&
              psXMLNode->psChild->psNext->psChild->pszValue &&
              psXMLNode->psChild->psNext->psNext->psChild->pszValue) {
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
            strlcat(psFilterNode->psRightNode->pszValue, ";", nStrLength);
            strlcat(psFilterNode->psRightNode->pszValue,
                    psUpperNode->pszValue, nStrLength);
          }


        } else
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
      else if (strcasecmp(psXMLNode->pszValue, "PropertyIsLike") == 0) {
        if (CPLSearchXMLNode(psXMLNode,  "Literal") &&
            CPLSearchXMLNode(psXMLNode,  "PropertyName") &&
            CPLGetXMLValue(psXMLNode, "wildCard", "") &&
            CPLGetXMLValue(psXMLNode, "singleChar", "") &&
            (CPLGetXMLValue(psXMLNode, "escape", "") ||
             CPLGetXMLValue(psXMLNode, "escapeChar", "")))
          /*
            psXMLNode->psChild &&
            strcasecmp(psXMLNode->psChild->pszValue, "wildCard") == 0 &&
            psXMLNode->psChild->psNext &&
            strcasecmp(psXMLNode->psChild->psNext->pszValue, "singleChar") == 0 &&
            psXMLNode->psChild->psNext->psNext &&
            strcasecmp(psXMLNode->psChild->psNext->psNext->pszValue, "escape") == 0 &&
            psXMLNode->psChild->psNext->psNext->psNext &&
            strcasecmp(psXMLNode->psChild->psNext->psNext->psNext->pszValue, "PropertyName") == 0 &&
            psXMLNode->psChild->psNext->psNext->psNext->psNext &&
            strcasecmp(psXMLNode->psChild->psNext->psNext->psNext->psNext->pszValue, "Literal") == 0)
          */
        {
          /* -------------------------------------------------------------------- */
          /*      Get the wildCard, the singleChar and the escapeChar used.       */
          /* -------------------------------------------------------------------- */
          psFilterNode->pOther = (FEPropertyIsLike *)malloc(sizeof(FEPropertyIsLike));
          /*default is case sensitive*/
          ((FEPropertyIsLike *)psFilterNode->pOther)->bCaseInsensitive = 0;

          pszTmp = (char *)CPLGetXMLValue(psXMLNode, "wildCard", "");
          if (pszTmp)
            ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard =
              msStrdup(pszTmp);
          pszTmp = (char *)CPLGetXMLValue(psXMLNode, "singleChar", "");
          if (pszTmp)
            ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar =
              msStrdup(pszTmp);
          pszTmp = (char *)CPLGetXMLValue(psXMLNode, "escape", "");
          if (pszTmp && strlen(pszTmp)>0)
            ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar =
              msStrdup(pszTmp);
          else {
            pszTmp = (char *)CPLGetXMLValue(psXMLNode, "escapeChar", "");
            if (pszTmp)
              ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar =
                msStrdup(pszTmp);
          }
          pszTmp = (char *)CPLGetXMLValue(psXMLNode, "matchCase", "");
          if (pszTmp && strlen(pszTmp) > 0 &&
              strcasecmp(pszTmp, "false") == 0) {
            ((FEPropertyIsLike *)psFilterNode->pOther)->bCaseInsensitive =1;
          }
          /* -------------------------------------------------------------------- */
          /*      Create left and right node for the attribute and the value.     */
          /* -------------------------------------------------------------------- */
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();

          psTmpNode = CPLSearchXMLNode(psXMLNode,  "PropertyName");
          if (psTmpNode &&  psTmpNode->psChild &&
              psTmpNode->psChild->pszValue &&
              strlen(psTmpNode->psChild->pszValue) > 0)

          {
            /*
            if (psXMLNode->psChild->psNext->psNext->psNext->psChild &&
            psXMLNode->psChild->psNext->psNext->psNext->psChild->pszValue)
            {
            psFilterNode->psLeftNode->pszValue =
              msStrdup(psXMLNode->psChild->psNext->psNext->psNext->psChild->pszValue);
            */
            psFilterNode->psLeftNode->pszValue =
              msStrdup(psTmpNode->psChild->pszValue);
            psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;

          }

          psFilterNode->psRightNode = FLTCreateFilterEncodingNode();


          psTmpNode = CPLSearchXMLNode(psXMLNode,  "Literal");
          if (psTmpNode &&  psTmpNode->psChild &&
              psTmpNode->psChild->pszValue &&
              strlen(psTmpNode->psChild->pszValue) > 0) {
            /*
            if (psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild &&
            psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild->pszValue)
            {

            psFilterNode->psRightNode->pszValue =
              msStrdup(psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild->pszValue);
            */
            psFilterNode->psRightNode->pszValue =
              msStrdup(psTmpNode->psChild->pszValue);

            psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
          }
        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;

      }
    }
    /* -------------------------------------------------------------------- */
    /*      FeatureId Filter                                                */
    /*                                                                      */
    /*      <ogc:Filter>                                                    */
    /*      <ogc:FeatureId fid="INWATERA_1M.1013"/>                         */
    /*      <ogc:FeatureId fid="INWATERA_1M.10"/>                           */
    /*      <ogc:FeatureId fid="INWATERA_1M.13"/>                           */
    /*      <ogc:FeatureId fid="INWATERA_1M.140"/>                          */
    /*      <ogc:FeatureId fid="INWATERA_1M.5001"/>                         */
    /*      <ogc:FeatureId fid="INWATERA_1M.2001"/>                         */
    /*      </ogc:Filter>                                                   */
    /*                                                                      */
    /*                                                                      */
    /*      Note that for FES1.1.0 the featureid has been depricated in     */
    /*      favor of GmlObjectId                                            */
    /*      <GmlObjectId gml:id="TREESA_1M.1234"/>                          */
    /* -------------------------------------------------------------------- */
    else if (FLTIsFeatureIdFilterType(psXMLNode->pszValue)) {
      psFilterNode->eType = FILTER_NODE_TYPE_FEATUREID;
      pszFeatureId = (char *)CPLGetXMLValue(psXMLNode, "fid", NULL);
      /*for FE 1.1.0 GmlObjectId */
      if (pszFeatureId == NULL)
        pszFeatureId = (char *)CPLGetXMLValue(psXMLNode, "id", NULL);
      pszFeatureIdList = NULL;

      psFeatureIdNode = psXMLNode;
      while (psFeatureIdNode) {
        pszFeatureId = (char *)CPLGetXMLValue(psFeatureIdNode, "fid", NULL);
        if (!pszFeatureId)
          pszFeatureId = (char *)CPLGetXMLValue(psFeatureIdNode, "id", NULL);

        if (pszFeatureId) {
          if (pszFeatureIdList)
            pszFeatureIdList = msStringConcatenate(pszFeatureIdList, ",");

          /*typname could be part of the value : INWATERA_1M.1234*/
          tokens = msStringSplit(pszFeatureId,'.', &nTokens);
          if (tokens && nTokens == 2)
            pszFeatureIdList = msStringConcatenate(pszFeatureIdList, tokens[1]);
          else
            pszFeatureIdList = msStringConcatenate(pszFeatureIdList, pszFeatureId);

          if (tokens)
            msFreeCharArray(tokens, nTokens);
        }
        psFeatureIdNode = psFeatureIdNode->psNext;
      }

      if (pszFeatureIdList) {
        psFilterNode->pszValue =  msStrdup(pszFeatureIdList);
        msFree(pszFeatureIdList);
      } else
        psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
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
  if (pszValue) {
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
  if (pszValue) {
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
  if (pszValue) {
    if (FLTIsBinaryComparisonFilterType(pszValue) ||
        strcasecmp(pszValue, "PropertyIsLike") == 0 ||
        strcasecmp(pszValue, "PropertyIsBetween") == 0)
      return MS_TRUE;
  }

  return MS_FALSE;
}

/************************************************************************/
/*            int FLTIsFeatureIdFilterType(char *pszValue)              */
/*                                                                      */
/*      return TRUE if the value of the node is of featureid filter     */
/*      encoding type.                                                  */
/************************************************************************/
int FLTIsFeatureIdFilterType(char *pszValue)
{
  if (pszValue && (strcasecmp(pszValue, "FeatureId") == 0 ||
                   strcasecmp(pszValue, "GmlObjectId") == 0))

    return MS_TRUE;

  return MS_FALSE;
}

/************************************************************************/
/*            int FLTIsSpatialFilterType(char *pszValue)                */
/*                                                                      */
/*      return TRUE if the value of the node is of spatial filter       */
/*      encoding type.                                                  */
/************************************************************************/
int FLTIsSpatialFilterType(char *pszValue)
{
  if (pszValue) {
    if ( strcasecmp(pszValue, "BBOX") == 0 ||
         strcasecmp(pszValue, "DWithin") == 0 ||
         strcasecmp(pszValue, "Intersect") == 0 ||
         strcasecmp(pszValue, "Intersects") == 0 ||
         strcasecmp(pszValue, "Equals") == 0 ||
         strcasecmp(pszValue, "Disjoint") == 0 ||
         strcasecmp(pszValue, "Touches") == 0 ||
         strcasecmp(pszValue, "Crosses") == 0 ||
         strcasecmp(pszValue, "Within") == 0 ||
         strcasecmp(pszValue, "Contains") == 0 ||
         strcasecmp(pszValue, "Overlaps") == 0 ||
         strcasecmp(pszValue, "Beyond") == 0)
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
  if (psXMLNode) {
    if (FLTIsLogicalFilterType(psXMLNode->pszValue) ||
        FLTIsSpatialFilterType(psXMLNode->pszValue) ||
        FLTIsComparisonFilterType(psXMLNode->pszValue) ||
        FLTIsFeatureIdFilterType(psXMLNode->pszValue))
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
/*              <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326">*/
/*                <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*              </gml:Box>                                              */
/*            </BBOX>                                                   */
/*          </Filter>                                                   */
/*                                                                      */
/*      eg 2 :<Filter>                                                  */
/*              <AND>                                                   */
/*               <BBOX>                                                 */
/*                <PropertyName>Geometry</PropertyName>                  */
/*                <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326">*/
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

  if (strcasecmp(psFilterNode->pszValue, "AND") == 0) {
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

shapeObj *FLTGetShape(FilterEncodingNode *psFilterNode, double *pdfDistance,
                      int *pnUnit)
{
  char **tokens = NULL;
  int nTokens = 0;
  FilterEncodingNode *psNode = psFilterNode;
  char *szUnitStr = NULL;
  char *szUnit = NULL;

  if (psNode) {
    if (psNode->eType == FILTER_NODE_TYPE_SPATIAL && psNode->psRightNode)
      psNode = psNode->psRightNode;

    if (psNode->eType == FILTER_NODE_TYPE_GEOMETRY_POINT ||
        psNode->eType == FILTER_NODE_TYPE_GEOMETRY_LINE ||
        psNode->eType == FILTER_NODE_TYPE_GEOMETRY_POLYGON) {

      if (psNode->pszValue && pdfDistance) {
        /*
        sytnax expected is "distance;unit" or just "distance"
        if unit is there syntax is "URI#unit" (eg http://..../#m)
        or just "unit"
        */
        tokens = msStringSplit(psNode->pszValue,';', &nTokens);
        if (tokens && nTokens >= 1) {
          *pdfDistance = atof(tokens[0]);

          if (nTokens == 2 && pnUnit) {
            szUnitStr = msStrdup(tokens[1]);
            msFreeCharArray(tokens, nTokens);
            nTokens = 0;
            tokens = msStringSplit(szUnitStr,'#', &nTokens);
            msFree(szUnitStr);
            if (tokens && nTokens >= 1) {
              if (nTokens ==1)
                szUnit = tokens[0];
              else
                szUnit = tokens[1];

              if (strcasecmp(szUnit,"m") == 0 ||
                  strcasecmp(szUnit,"meters") == 0 )
                *pnUnit = MS_METERS;
              else if (strcasecmp(szUnit,"km") == 0 ||
                       strcasecmp(szUnit,"kilometers") == 0)
                *pnUnit = MS_KILOMETERS;
              else if (strcasecmp(szUnit,"NM") == 0 ||
                       strcasecmp(szUnit,"nauticalmiles") == 0)
                *pnUnit = MS_NAUTICALMILES;
              else if (strcasecmp(szUnit,"mi") == 0 ||
                       strcasecmp(szUnit,"miles") == 0)
                *pnUnit = MS_MILES;
              else if (strcasecmp(szUnit,"in") == 0 ||
                       strcasecmp(szUnit,"inches") == 0)
                *pnUnit = MS_INCHES;
              else if (strcasecmp(szUnit,"ft") == 0 ||
                       strcasecmp(szUnit,"feet") == 0)
                *pnUnit = MS_FEET;
              else if (strcasecmp(szUnit,"deg") == 0 ||
                       strcasecmp(szUnit,"dd") == 0)
                *pnUnit = MS_DD;
              else if (strcasecmp(szUnit,"px") == 0)
                *pnUnit = MS_PIXELS;

              msFreeCharArray(tokens, nTokens);
            }
          }
        }

      }

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

  if (psFilterNode->pszValue && strcasecmp(psFilterNode->pszValue, "BBOX") == 0) {
    if (psFilterNode->psRightNode && psFilterNode->psRightNode->pOther) {
      psRect->minx = ((rectObj *)psFilterNode->psRightNode->pOther)->minx;
      psRect->miny = ((rectObj *)psFilterNode->psRightNode->pOther)->miny;
      psRect->maxx = ((rectObj *)psFilterNode->psRightNode->pOther)->maxx;
      psRect->maxy = ((rectObj *)psFilterNode->psRightNode->pOther)->maxy;

      return psFilterNode->psRightNode->pszValue;

    }
  } else {
    pszReturn = FLTGetBBOX(psFilterNode->psLeftNode, psRect);
    if (pszReturn)
      return pszReturn;
    else
      return  FLTGetBBOX(psFilterNode->psRightNode, psRect);
  }

  return pszReturn;
}

/************************************************************************/
/*                          GetMapserverExpression                      */
/*                                                                      */
/*      Return a mapserver expression base on the Filer encoding nodes. */
/************************************************************************/
char *FLTGetMapserverExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  char *pszExpression = NULL;
  const char *pszAttribute = NULL;
  char szTmp[256];
  char **tokens = NULL;
  int nTokens = 0, i=0,bString=0;
  char *pszTmp;

  if (!psFilterNode)
    return NULL;

  if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON) {
    if ( psFilterNode->psLeftNode && psFilterNode->psRightNode) {
      if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue)) {
        pszExpression = FLTGetBinaryComparisonExpresssion(psFilterNode, lp);
      } else if (strcasecmp(psFilterNode->pszValue,
                            "PropertyIsBetween") == 0) {
        pszExpression = FLTGetIsBetweenComparisonExpresssion(psFilterNode, lp);
      } else if (strcasecmp(psFilterNode->pszValue,
                            "PropertyIsLike") == 0) {
        pszExpression = FLTGetIsLikeComparisonExpression(psFilterNode);
      }
    }
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL) {
    if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
        strcasecmp(psFilterNode->pszValue, "OR") == 0) {
      pszExpression = FLTGetLogicalComparisonExpresssion(psFilterNode, lp);
    } else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0) {
      pszExpression = FLTGetLogicalComparisonExpresssion(psFilterNode, lp);
    }
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL) {
    /* TODO */
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_FEATUREID) {
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)
    if (psFilterNode->pszValue) {
      pszAttribute = msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
      if (pszAttribute) {
        tokens = msStringSplit(psFilterNode->pszValue,',', &nTokens);
        if (tokens && nTokens > 0) {
          for (i=0; i<nTokens; i++) {
            if (i == 0) {
              pszTmp = tokens[0];
              if(FLTIsNumeric(pszTmp) == MS_FALSE)
                bString = 1;
            }
            if (bString)
              snprintf(szTmp, sizeof(szTmp), "('[%s]' = '%s')" , pszAttribute, tokens[i]);
            else
              snprintf(szTmp, sizeof(szTmp), "([%s] = %s)" , pszAttribute, tokens[i]);

            if (pszExpression != NULL)
              pszExpression = msStringConcatenate(pszExpression, " OR ");
            else
              pszExpression = msStringConcatenate(pszExpression, "(");
            pszExpression = msStringConcatenate(pszExpression, szTmp);
          }

          msFreeCharArray(tokens, nTokens);
        }
      }
      /*opening and closing brackets are needed for mapserver expressions*/
      if (pszExpression)
        pszExpression = msStringConcatenate(pszExpression, ")");
    }
#else
    msSetError(MS_MISCERR, "OWS support is not available.",
               "FLTGetMapserverExpression()");
    return(MS_FAILURE);
#endif

  }
  return pszExpression;
}


/************************************************************************/
/*                           FLTGetSQLExpression                        */
/*                                                                      */
/*      Build SQL expressions from the mapserver nodes.                 */
/************************************************************************/
char *FLTGetSQLExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  char *pszExpression = NULL;
  const char *pszAttribute = NULL;
  char szTmp[256];
  char **tokens = NULL;
  int nTokens = 0, i=0, bString=0;

  if (psFilterNode == NULL || lp == NULL)
    return NULL;

  if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON) {
    if ( psFilterNode->psLeftNode && psFilterNode->psRightNode) {
      if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue)) {
        pszExpression =
          FLTGetBinaryComparisonSQLExpresssion(psFilterNode, lp);
      } else if (strcasecmp(psFilterNode->pszValue,
                            "PropertyIsBetween") == 0) {
        pszExpression =
          FLTGetIsBetweenComparisonSQLExpresssion(psFilterNode, lp);
      } else if (strcasecmp(psFilterNode->pszValue,
                            "PropertyIsLike") == 0) {
        pszExpression =
          FLTGetIsLikeComparisonSQLExpression(psFilterNode, lp);

      }
    }
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL) {
    if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
        strcasecmp(psFilterNode->pszValue, "OR") == 0) {
      pszExpression =
        FLTGetLogicalComparisonSQLExpresssion(psFilterNode, lp);

    } else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0) {
      pszExpression =
        FLTGetLogicalComparisonSQLExpresssion(psFilterNode, lp);

    }
  }

  else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL) {
    /* TODO */
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_FEATUREID) {
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)
    if (psFilterNode->pszValue) {
      pszAttribute = msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
      if (pszAttribute) {
        tokens = msStringSplit(psFilterNode->pszValue,',', &nTokens);
        bString = 0;
        if (tokens && nTokens > 0) {
          for (i=0; i<nTokens; i++) {
            char *pszEscapedStr = NULL;
            if (strlen(tokens[i]) <= 0)
              continue;

            if (FLTIsNumeric((tokens[i])) == MS_FALSE)
              bString = 1;

            pszEscapedStr = msLayerEscapeSQLParam(lp, tokens[i]);
            if (bString)
              snprintf(szTmp, sizeof(szTmp), "(%s = '%s')" , pszAttribute, pszEscapedStr);
            else
              snprintf(szTmp, sizeof(szTmp), "(%s = %s)" , pszAttribute, pszEscapedStr);

            msFree(pszEscapedStr);
            pszEscapedStr=NULL;

            if (pszExpression != NULL)
              pszExpression = msStringConcatenate(pszExpression, " OR ");
            else
              /*opening and closing brackets*/
              pszExpression = msStringConcatenate(pszExpression, "(");

            pszExpression = msStringConcatenate(pszExpression, szTmp);
          }

          msFreeCharArray(tokens, nTokens);
        }
      }
      /*opening and closing brackets*/
      if (pszExpression)
        pszExpression = msStringConcatenate(pszExpression, ")");
    }
#else
    msSetError(MS_MISCERR, "OWS support is not available.",
               "FLTGetSQLExpression()");
    return(MS_FAILURE);
#endif

  }

  return pszExpression;
}

/************************************************************************/
/*                            FLTGetNodeExpression                      */
/*                                                                      */
/*      Return the expresion for a specific node.                       */
/************************************************************************/
char *FLTGetNodeExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  char *pszExpression = NULL;
  if (!psFilterNode)
    return NULL;

  if (FLTIsLogicalFilterType(psFilterNode->pszValue))
    pszExpression = FLTGetLogicalComparisonExpresssion(psFilterNode, lp);
  else if (FLTIsComparisonFilterType(psFilterNode->pszValue)) {
    if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
      pszExpression = FLTGetBinaryComparisonExpresssion(psFilterNode, lp);
    else if (strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0)
      pszExpression = FLTGetIsBetweenComparisonExpresssion(psFilterNode, lp);
    else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
      pszExpression = FLTGetIsLikeComparisonExpression(psFilterNode);
  }

  return pszExpression;
}


/************************************************************************/
/*                     FLTGetLogicalComparisonSQLExpresssion            */
/*                                                                      */
/*      Return the expression for logical comparison expression.        */
/************************************************************************/
char *FLTGetLogicalComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
    layerObj *lp)
{
  char *pszBuffer = NULL;
  char *pszTmp = NULL;
  int nTmp = 0;

  if (lp == NULL)
    return NULL;

  /* ==================================================================== */
  /*      special case for BBOX node.                                     */
  /* ==================================================================== */
  if (psFilterNode->psLeftNode && psFilterNode->psRightNode &&
      ((strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") == 0) ||
       (strcasecmp(psFilterNode->psRightNode->pszValue, "BBOX") == 0))) {
    if (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") != 0)
      pszTmp = FLTGetSQLExpression(psFilterNode->psLeftNode, lp);
    else
      pszTmp = FLTGetSQLExpression(psFilterNode->psRightNode, lp);

    if (!pszTmp)
      return NULL;

    pszBuffer = (char *)malloc(sizeof(char) * (strlen(pszTmp) + 1));
    sprintf(pszBuffer, "%s", pszTmp);
  }

  /* -------------------------------------------------------------------- */
  /*      OR and AND                                                      */
  /* -------------------------------------------------------------------- */
  else if (psFilterNode->psLeftNode && psFilterNode->psRightNode) {
    pszTmp = FLTGetSQLExpression(psFilterNode->psLeftNode, lp);
    if (!pszTmp)
      return NULL;

    pszBuffer = (char *)malloc(sizeof(char) *
                               (strlen(pszTmp) +
                                strlen(psFilterNode->pszValue) + 5));
    pszBuffer[0] = '\0';
    strcat(pszBuffer, " (");
    strcat(pszBuffer, pszTmp);
    strcat(pszBuffer, " ");
    strcat(pszBuffer, psFilterNode->pszValue);
    strcat(pszBuffer, " ");

    free( pszTmp );

    nTmp = strlen(pszBuffer);
    pszTmp = FLTGetSQLExpression(psFilterNode->psRightNode, lp);
    if (!pszTmp)
      return NULL;

    pszBuffer = (char *)realloc(pszBuffer,
                                sizeof(char) * (strlen(pszTmp) + nTmp +3));
    strcat(pszBuffer, pszTmp);
    strcat(pszBuffer, ") ");
  }
  /* -------------------------------------------------------------------- */
  /*      NOT                                                             */
  /* -------------------------------------------------------------------- */
  else if (psFilterNode->psLeftNode &&
           strcasecmp(psFilterNode->pszValue, "NOT") == 0) {
    pszTmp = FLTGetSQLExpression(psFilterNode->psLeftNode, lp);
    if (!pszTmp)
      return NULL;

    pszBuffer = (char *)malloc(sizeof(char) * (strlen(pszTmp) +  9));
    pszBuffer[0] = '\0';

    strcat(pszBuffer, " (NOT ");
    strcat(pszBuffer, pszTmp);
    strcat(pszBuffer, ") ");
  } else
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      Cleanup.                                                        */
  /* -------------------------------------------------------------------- */
  if( pszTmp != NULL )
    free( pszTmp );
  return pszBuffer;

}

/************************************************************************/
/*                     FLTGetLogicalComparisonExpresssion               */
/*                                                                      */
/*      Return the expression for logical comparison expression.        */
/************************************************************************/
char *FLTGetLogicalComparisonExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  char *pszTmp = NULL;
  char *pszBuffer = NULL;
  int nTmp = 0;

  if (!psFilterNode || !FLTIsLogicalFilterType(psFilterNode->pszValue))
    return NULL;


  /* ==================================================================== */
  /*      special case for BBOX node.                                     */
  /* ==================================================================== */
  if (psFilterNode->psLeftNode && psFilterNode->psRightNode &&
      (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") == 0 ||
       strcasecmp(psFilterNode->psRightNode->pszValue, "BBOX") == 0 ||
       FLTIsGeosNode(psFilterNode->psLeftNode->pszValue) ||
       FLTIsGeosNode(psFilterNode->psRightNode->pszValue)))


  {

    /*strcat(szBuffer, " (");*/
    if (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") != 0 &&
        strcasecmp(psFilterNode->psLeftNode->pszValue, "DWithin") != 0 &&
        FLTIsGeosNode(psFilterNode->psLeftNode->pszValue) == MS_FALSE)
      pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode, lp);
    else
      pszTmp = FLTGetNodeExpression(psFilterNode->psRightNode, lp);

    if (!pszTmp)
      return NULL;

    pszBuffer = (char *)malloc(sizeof(char) * (strlen(pszTmp) + 3));
    pszBuffer[0] = '\0';
    /*
    if (strcasecmp(psFilterNode->psLeftNode->pszValue, "PropertyIsLike") == 0 ||
        strcasecmp(psFilterNode->psRightNode->pszValue, "PropertyIsLike") == 0)
      sprintf(pszBuffer, "%s", pszTmp);
    else
    */
    sprintf(pszBuffer, "(%s)", pszTmp);

    free(pszTmp);

    return pszBuffer;
  }


  /* -------------------------------------------------------------------- */
  /*      OR and AND                                                      */
  /* -------------------------------------------------------------------- */
  if (psFilterNode->psLeftNode && psFilterNode->psRightNode) {
    pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode, lp);
    if (!pszTmp)
      return NULL;

    pszBuffer = (char *)malloc(sizeof(char) *
                               (strlen(pszTmp) + strlen(psFilterNode->pszValue) + 5));
    pszBuffer[0] = '\0';
    strcat(pszBuffer, " (");

    strcat(pszBuffer, pszTmp);
    strcat(pszBuffer, " ");
    strcat(pszBuffer, psFilterNode->pszValue);
    strcat(pszBuffer, " ");
    free(pszTmp);

    pszTmp = FLTGetNodeExpression(psFilterNode->psRightNode, lp);
    if (!pszTmp)
      return NULL;

    nTmp = strlen(pszBuffer);
    pszBuffer = (char *)realloc(pszBuffer,
                                sizeof(char) * (strlen(pszTmp) + nTmp +3));

    strcat(pszBuffer, pszTmp);
    strcat(pszBuffer, ") ");
    free(pszTmp);
  }
  /* -------------------------------------------------------------------- */
  /*      NOT                                                             */
  /* -------------------------------------------------------------------- */
  else if (psFilterNode->psLeftNode &&
           strcasecmp(psFilterNode->pszValue, "NOT") == 0) {
    pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode, lp);
    if (!pszTmp)
      return NULL;

    pszBuffer = (char *)malloc(sizeof(char) *
                               (strlen(pszTmp) +  9));
    pszBuffer[0] = '\0';
    strcat(pszBuffer, " (NOT ");
    strcat(pszBuffer, pszTmp);
    strcat(pszBuffer, ") ");

    free(pszTmp);
  } else
    return NULL;

  return pszBuffer;

}



/************************************************************************/
/*                      FLTGetBinaryComparisonExpresssion               */
/*                                                                      */
/*      Return the expression for a binary comparison filter node.      */
/************************************************************************/
char *FLTGetBinaryComparisonExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  int bString=0;
  char szTmp[256];

  szBuffer[0] = '\0';
  if (!psFilterNode || !FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      check if the value is a numeric value or alphanumeric. If it    */
  /*      is alphanumeric, add quotes around attribute and values.        */
  /* -------------------------------------------------------------------- */
  bString = 0;
  if (psFilterNode->psRightNode->pszValue) {
    const char* pszOFGType;
    snprintf(szTmp, sizeof(szTmp), "%s_type",  psFilterNode->psLeftNode->pszValue);
    pszOFGType = msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp);
    if (pszOFGType!= NULL && strcasecmp(pszOFGType, "Character") == 0)
      bString = 1;
    else if (FLTIsNumeric(psFilterNode->psRightNode->pszValue) == MS_FALSE)
      bString = 1;
  }

  /* specical case to be able to have empty strings in the expression. */
  if (psFilterNode->psRightNode->pszValue == NULL)
    bString = 1;


  if (bString)
    strlcat(szBuffer, " (\"[", bufferSize);
  else
    strlcat(szBuffer, " ([", bufferSize);
  /* attribute */

  strlcat(szBuffer, psFilterNode->psLeftNode->pszValue, bufferSize);
  if (bString)
    strlcat(szBuffer, "]\" ", bufferSize);
  else
    strlcat(szBuffer, "] ", bufferSize);


  /* logical operator */
  if (strcasecmp(psFilterNode->pszValue,
                 "PropertyIsEqualTo") == 0) {
    /*case insensitive set ? */
    if (psFilterNode->psRightNode->pOther &&
        (*(int *)psFilterNode->psRightNode->pOther) == 1) {
      strlcat(szBuffer, "IEQ", bufferSize);
    } else
      strlcat(szBuffer, "=", bufferSize);
  } else if (strcasecmp(psFilterNode->pszValue,
                        "PropertyIsNotEqualTo") == 0)
    strlcat(szBuffer, "!=", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsLessThan") == 0)
    strlcat(szBuffer, "<", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsGreaterThan") == 0)
    strlcat(szBuffer, ">", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsLessThanOrEqualTo") == 0)
    strlcat(szBuffer, "<=", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsGreaterThanOrEqualTo") == 0)
    strlcat(szBuffer, ">=", bufferSize);

  strlcat(szBuffer, " ", bufferSize);

  /* value */
  if (bString)
    strlcat(szBuffer, "\"", bufferSize);

  if (psFilterNode->psRightNode->pszValue)
    strlcat(szBuffer, psFilterNode->psRightNode->pszValue, bufferSize);

  if (bString)
    strlcat(szBuffer, "\"", bufferSize);

  strlcat(szBuffer, ") ", bufferSize);

  return msStrdup(szBuffer);
}




/************************************************************************/
/*                      FLTGetBinaryComparisonSQLExpresssion            */
/*                                                                      */
/*      Return the expression for a binary comparison filter node.      */
/************************************************************************/
char *FLTGetBinaryComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
    layerObj *lp)
{
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  int bString=0;
  char szTmp[256];
  char* pszEscapedStr = NULL;

  szBuffer[0] = '\0';
  if (!psFilterNode || !
      FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      check if the value is a numeric value or alphanumeric. If it    */
  /*      is alphanumeric, add quotes around attribute and values.        */
  /* -------------------------------------------------------------------- */
  bString = 0;
  if (psFilterNode->psRightNode->pszValue) {
    const char* pszOFGType;
    snprintf(szTmp, sizeof(szTmp), "%s_type",  psFilterNode->psLeftNode->pszValue);
    pszOFGType = msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp);
    if (pszOFGType!= NULL && strcasecmp(pszOFGType, "Character") == 0)
      bString = 1;

    else if (FLTIsNumeric(psFilterNode->psRightNode->pszValue) == MS_FALSE)
      bString = 1;
  }

  /* specical case to be able to have empty strings in the expression. */
  if (psFilterNode->psRightNode->pszValue == NULL)
    bString = 1;


  /*opening bracket*/
  strlcat(szBuffer, " (", bufferSize);

  pszEscapedStr = msLayerEscapePropertyName(lp, psFilterNode->psLeftNode->pszValue);


  /* attribute */
  /*case insensitive set ? */
  if (bString &&
      strcasecmp(psFilterNode->pszValue,
                 "PropertyIsEqualTo") == 0 &&
      psFilterNode->psRightNode->pOther &&
      (*(int *)psFilterNode->psRightNode->pOther) == 1) {
    snprintf(szTmp, sizeof(szTmp), "lower(%s) ",  pszEscapedStr);
    strlcat(szBuffer, szTmp, bufferSize);
  } else
    strlcat(szBuffer, pszEscapedStr, bufferSize);

  msFree(pszEscapedStr);
  pszEscapedStr = NULL;


  /* logical operator */
  if (strcasecmp(psFilterNode->pszValue,
                 "PropertyIsEqualTo") == 0)
    strlcat(szBuffer, "=", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsNotEqualTo") == 0)
    strlcat(szBuffer, "<>", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsLessThan") == 0)
    strlcat(szBuffer, "<", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsGreaterThan") == 0)
    strlcat(szBuffer, ">", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsLessThanOrEqualTo") == 0)
    strlcat(szBuffer, "<=", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsGreaterThanOrEqualTo") == 0)
    strlcat(szBuffer, ">=", bufferSize);

  strlcat(szBuffer, " ", bufferSize);

  /* value */

  if (bString &&
      psFilterNode->psRightNode->pszValue &&
      strcasecmp(psFilterNode->pszValue,
                 "PropertyIsEqualTo") == 0 &&
      psFilterNode->psRightNode->pOther &&
      (*(int *)psFilterNode->psRightNode->pOther) == 1) {
    char* pszEscapedStr;
    pszEscapedStr = msLayerEscapeSQLParam(lp, psFilterNode->psRightNode->pszValue);
    snprintf(szTmp, sizeof(szTmp), "lower('%s') ", pszEscapedStr);
    msFree(pszEscapedStr);
    strlcat(szBuffer, szTmp, bufferSize);
  } else {
    if (bString)
      strlcat(szBuffer, "'", bufferSize);

    if (psFilterNode->psRightNode->pszValue) {
      if (bString) {
        char* pszEscapedStr;
        pszEscapedStr = msLayerEscapeSQLParam(lp, psFilterNode->psRightNode->pszValue);
        strlcat(szBuffer, pszEscapedStr, bufferSize);
        msFree(pszEscapedStr);
        pszEscapedStr=NULL;
      } else
        strlcat(szBuffer, psFilterNode->psRightNode->pszValue, bufferSize);
    }

    if (bString)
      strlcat(szBuffer, "'", bufferSize);

  }
  /*closing bracket*/
  strlcat(szBuffer, ") ", bufferSize);

  return msStrdup(szBuffer);
}


/************************************************************************/
/*                    FLTGetIsBetweenComparisonSQLExpresssion           */
/*                                                                      */
/*      Build an SQL expresssion for IsBteween Filter.                  */
/************************************************************************/
char *FLTGetIsBetweenComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
    layerObj *lp)
{
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  char **aszBounds = NULL;
  int nBounds = 0;
  int bString=0;
  char szTmp[256];
  char* pszEscapedStr;

  szBuffer[0] = '\0';
  if (!psFilterNode ||
      !(strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0))
    return NULL;

  if (!psFilterNode->psLeftNode || !psFilterNode->psRightNode )
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      Get the bounds value which are stored like boundmin;boundmax    */
  /* -------------------------------------------------------------------- */
  aszBounds = msStringSplit(psFilterNode->psRightNode->pszValue, ';', &nBounds);
  if (nBounds != 2)
    return NULL;
  /* -------------------------------------------------------------------- */
  /*      check if the value is a numeric value or alphanumeric. If it    */
  /*      is alphanumeric, add quotes around attribute and values.        */
  /* -------------------------------------------------------------------- */
  bString = 0;
  if (aszBounds[0]) {
    const char* pszOFGType;
    snprintf(szTmp, sizeof(szTmp), "%s_type",  psFilterNode->psLeftNode->pszValue);
    pszOFGType = msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp);
    if (pszOFGType!= NULL && strcasecmp(pszOFGType, "Character") == 0)
      bString = 1;
    else if (FLTIsNumeric(aszBounds[0]) == MS_FALSE)
      bString = 1;
  }
  if (!bString) {
    if (aszBounds[1]) {
      if (FLTIsNumeric(aszBounds[1]) == MS_FALSE)
        bString = 1;
    }
  }


  /* -------------------------------------------------------------------- */
  /*      build expresssion.                                              */
  /* -------------------------------------------------------------------- */
  /*opening paranthesis */
  strlcat(szBuffer, " (", bufferSize);

  /* attribute */
  pszEscapedStr = msLayerEscapePropertyName(lp, psFilterNode->psLeftNode->pszValue);

  strlcat(szBuffer, pszEscapedStr, bufferSize);
  msFree(pszEscapedStr);
  pszEscapedStr = NULL;

  /*between*/
  strlcat(szBuffer, " BETWEEN ", bufferSize);

  /*bound 1*/
  if (bString)
    strlcat(szBuffer,"'", bufferSize);
  pszEscapedStr = msLayerEscapeSQLParam( lp, aszBounds[0]);
  strlcat(szBuffer, pszEscapedStr, bufferSize);
  msFree(pszEscapedStr);
  pszEscapedStr=NULL;

  if (bString)
    strlcat(szBuffer,"'", bufferSize);

  strlcat(szBuffer, " AND ", bufferSize);

  /*bound 2*/
  if (bString)
    strlcat(szBuffer, "'", bufferSize);
  pszEscapedStr = msLayerEscapeSQLParam( lp, aszBounds[1]);
  strlcat(szBuffer, pszEscapedStr, bufferSize);
  msFree(pszEscapedStr);
  pszEscapedStr=NULL;

  if (bString)
    strlcat(szBuffer,"'", bufferSize);

  /*closing paranthesis*/
  strlcat(szBuffer, ")", bufferSize);


  return msStrdup(szBuffer);
}

/************************************************************************/
/*                    FLTGetIsBetweenComparisonExpresssion              */
/*                                                                      */
/*      Build expresssion for IsBteween Filter.                         */
/************************************************************************/
char *FLTGetIsBetweenComparisonExpresssion(FilterEncodingNode *psFilterNode,
    layerObj *lp)
{
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  char **aszBounds = NULL;
  int nBounds = 0;
  int bString=0;
  char szTmp[256];


  szBuffer[0] = '\0';
  if (!psFilterNode ||
      !(strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0))
    return NULL;

  if (!psFilterNode->psLeftNode || !psFilterNode->psRightNode )
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      Get the bounds value which are stored like boundmin;boundmax    */
  /* -------------------------------------------------------------------- */
  aszBounds = msStringSplit(psFilterNode->psRightNode->pszValue, ';', &nBounds);
  if (nBounds != 2) {
    msFreeCharArray(aszBounds, nBounds);
    return NULL;
  }
  /* -------------------------------------------------------------------- */
  /*      check if the value is a numeric value or alphanumeric. If it    */
  /*      is alphanumeric, add quotes around attribute and values.        */
  /* -------------------------------------------------------------------- */
  bString = 0;
  if (aszBounds[0]) {
    const char* pszOFGType;
    snprintf(szTmp, sizeof(szTmp), "%s_type",  psFilterNode->psLeftNode->pszValue);
    pszOFGType = msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp);
    if (pszOFGType!= NULL && strcasecmp(pszOFGType, "Character") == 0)
      bString = 1;
    else if (FLTIsNumeric(aszBounds[0]) == MS_FALSE)
      bString = 1;
  }
  if (!bString) {
    if (aszBounds[1]) {
      if (FLTIsNumeric(aszBounds[1]) == MS_FALSE)
        bString = 1;
    }
  }


  /* -------------------------------------------------------------------- */
  /*      build expresssion.                                              */
  /* -------------------------------------------------------------------- */
  if (bString)
    strlcat(szBuffer, " (\"[", bufferSize);
  else
    strlcat(szBuffer, " ([", bufferSize);

  /* attribute */
  strlcat(szBuffer, psFilterNode->psLeftNode->pszValue, bufferSize);

  if (bString)
    strlcat(szBuffer, "]\" ", bufferSize);
  else
    strlcat(szBuffer, "] ", bufferSize);


  strlcat(szBuffer, " >= ", bufferSize);
  if (bString)
    strlcat(szBuffer,"\"", bufferSize);
  strlcat(szBuffer, aszBounds[0], bufferSize);
  if (bString)
    strlcat(szBuffer,"\"", bufferSize);

  strlcat(szBuffer, " AND ", bufferSize);

  if (bString)
    strlcat(szBuffer, " \"[", bufferSize);
  else
    strlcat(szBuffer, " [", bufferSize);

  /* attribute */
  strlcat(szBuffer, psFilterNode->psLeftNode->pszValue, bufferSize);

  if (bString)
    strlcat(szBuffer, "]\" ", bufferSize);
  else
    strlcat(szBuffer, "] ", bufferSize);

  strlcat(szBuffer, " <= ", bufferSize);
  if (bString)
    strlcat(szBuffer,"\"", bufferSize);
  strlcat(szBuffer, aszBounds[1], bufferSize);
  if (bString)
    strlcat(szBuffer,"\"", bufferSize);
  strlcat(szBuffer, ")", bufferSize);

  msFreeCharArray(aszBounds, nBounds);

  return msStrdup(szBuffer);
}

/************************************************************************/
/*                      FLTGetIsLikeComparisonExpression               */
/*                                                                      */
/*      Build expression for IsLike filter.                             */
/************************************************************************/
char *FLTGetIsLikeComparisonExpression(FilterEncodingNode *psFilterNode)
{
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  char szTmp[256];
  char *pszValue = NULL;

  char *pszWild = NULL;
  char *pszSingle = NULL;
  char *pszEscape = NULL;
  int  bCaseInsensitive = 0;

  int nLength=0, i=0, iTmp=0;


  if (!psFilterNode || !psFilterNode->pOther || !psFilterNode->psLeftNode ||
      !psFilterNode->psRightNode || !psFilterNode->psRightNode->pszValue)
    return NULL;

  pszWild = ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard;
  pszSingle = ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar;
  pszEscape = ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar;
  bCaseInsensitive = ((FEPropertyIsLike *)psFilterNode->pOther)->bCaseInsensitive;

  if (!pszWild || strlen(pszWild) == 0 ||
      !pszSingle || strlen(pszSingle) == 0 ||
      !pszEscape || strlen(pszEscape) == 0)
    return NULL;


  /* -------------------------------------------------------------------- */
  /*      Use operand with regular expressions.                           */
  /* -------------------------------------------------------------------- */
  szBuffer[0] = '\0';
  sprintf(szTmp, "%s", " (\"[");
  szTmp[4] = '\0';

  strlcat(szBuffer, szTmp, bufferSize);

  /* attribute */
  strlcat(szBuffer, psFilterNode->psLeftNode->pszValue, bufferSize);
  szBuffer[strlen(szBuffer)] = '\0';

  /*#3521 */
  if(bCaseInsensitive == 1)
    sprintf(szTmp, "%s", "]\" ~* /");
  else
    sprintf(szTmp, "%s", "]\" =~ /");
  szTmp[7] = '\0';
  strlcat(szBuffer, szTmp, bufferSize);
  szBuffer[strlen(szBuffer)] = '\0';


  pszValue = psFilterNode->psRightNode->pszValue;
  nLength = strlen(pszValue);

  iTmp =0;
  if (nLength > 0 && pszValue[0] != pszWild[0] &&
      pszValue[0] != pszSingle[0] &&
      pszValue[0] != pszEscape[0]) {
    szTmp[iTmp]= '^';
    iTmp++;
  }
  for (i=0; i<nLength; i++) {
    if (pszValue[i] != pszWild[0] &&
        pszValue[i] != pszSingle[0] &&
        pszValue[i] != pszEscape[0]) {
      szTmp[iTmp] = pszValue[i];
      iTmp++;
      szTmp[iTmp] = '\0';
    } else if  (pszValue[i] == pszSingle[0]) {
      szTmp[iTmp] = '.';
      iTmp++;
      szTmp[iTmp] = '\0';
    } else if  (pszValue[i] == pszEscape[0]) {
      szTmp[iTmp] = '\\';
      iTmp++;
      szTmp[iTmp] = '\0';

    } else if (pszValue[i] == pszWild[0]) {
      /* strcat(szBuffer, "[0-9,a-z,A-Z,\\s]*"); */
      /* iBuffer+=17; */
      szTmp[iTmp++] = '.';
      szTmp[iTmp++] = '*';
      szTmp[iTmp] = '\0';
    }
  }
  szTmp[iTmp] = '/';
  szTmp[++iTmp] = '\0';

  strlcat(szBuffer, szTmp, bufferSize);
  strlcat(szBuffer, ")", bufferSize);
  return msStrdup(szBuffer);
}

/************************************************************************/
/*                      FLTGetIsLikeComparisonSQLExpression             */
/*                                                                      */
/*      Build an sql expression for IsLike filter.                      */
/************************************************************************/
char *FLTGetIsLikeComparisonSQLExpression(FilterEncodingNode *psFilterNode,
    layerObj *lp)
{
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  char *pszValue = NULL;

  char *pszWild = NULL;
  char *pszSingle = NULL;
  char *pszEscape = NULL;
  char szTmp[4];

  int nLength=0, i=0, j=0;
  int  bCaseInsensitive = 0;

  char *pszEscapedStr = NULL;

  if (!psFilterNode || !psFilterNode->pOther || !psFilterNode->psLeftNode ||
      !psFilterNode->psRightNode || !psFilterNode->psRightNode->pszValue)
    return NULL;

  pszWild = ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard;
  pszSingle = ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar;
  pszEscape = ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar;
  bCaseInsensitive = ((FEPropertyIsLike *)psFilterNode->pOther)->bCaseInsensitive;

  if (!pszWild || strlen(pszWild) == 0 ||
      !pszSingle || strlen(pszSingle) == 0 ||
      !pszEscape || strlen(pszEscape) == 0)
    return NULL;

  if (pszEscape[0] == '\'') {
    /* This might be valid, but the risk of SQL injection is too high */
    /* and the below code is not ready for that */
    /* Someone who does this has clearly suspect intentions ! */
    msSetError(MS_MISCERR, "Single quote character is not allowed as an escaping character.",
               "FLTGetIsLikeComparisonSQLExpression()");
    return NULL;
  }


  szBuffer[0] = '\0';
  /*opening bracket*/
  strlcat(szBuffer, " (", bufferSize);

  /* attribute name */
  pszEscapedStr = msLayerEscapePropertyName(lp, psFilterNode->psLeftNode->pszValue);

  strlcat(szBuffer, pszEscapedStr, bufferSize);
  msFree(pszEscapedStr);
  pszEscapedStr = NULL;

  if (lp->connectiontype == MS_POSTGIS) {
    if (bCaseInsensitive == 1)
      strlcat(szBuffer, "::text ilike '", bufferSize);
    else
      strlcat(szBuffer, "::text like '", bufferSize);
  } else
    strlcat(szBuffer, " like '", bufferSize);

  pszValue = psFilterNode->psRightNode->pszValue;
  nLength = strlen(pszValue);

  pszEscapedStr = (char*) msSmallMalloc( 3 * nLength + 1);

  for (i=0, j=0; i<nLength; i++) {
    char c = pszValue[i];
    if (c != pszWild[0] &&
        c != pszSingle[0] &&
        c != pszEscape[0]) {
      if (c == '\'') {
        pszEscapedStr[j++] = '\'';
        pszEscapedStr[j++] = '\'';
      } else if (c == '\\') {
        pszEscapedStr[j++] = '\\';
        pszEscapedStr[j++] = '\\';
      } else
        pszEscapedStr[j++] = c;
    } else if  (c == pszSingle[0]) {
      pszEscapedStr[j++] = '_';
    } else if  (c == pszEscape[0]) {
      pszEscapedStr[j++] = pszEscape[0];
      if (i+1<nLength) {
        char nextC = pszValue[i+1];
        i++;
        if (nextC == '\'') {
          pszEscapedStr[j++] = '\'';
          pszEscapedStr[j++] = '\'';
        } else
          pszEscapedStr[j++] = nextC;
      }
    } else if (c == pszWild[0]) {
      pszEscapedStr[j++] = '%';
    }
  }
  pszEscapedStr[j++] = 0;
  strlcat(szBuffer, pszEscapedStr, bufferSize);
  msFree(pszEscapedStr);

  strlcat(szBuffer, "'", bufferSize);
  if (lp->connectiontype != MS_OGR) {
    strlcat(szBuffer, " escape '", bufferSize);
    szTmp[0] = pszEscape[0];
    if (pszEscape[0] == '\\') {
      szTmp[1] = '\\';
      szTmp[2] = '\'';
      szTmp[3] = '\0';
    } else {
      szTmp[1] = '\'';
      szTmp[2] = '\0';
    }

    strlcat(szBuffer,  szTmp, bufferSize);
  }
  strlcat(szBuffer,  ") ", bufferSize);

  return msStrdup(szBuffer);
}

/************************************************************************/
/*                           FLTHasSpatialFilter                        */
/*                                                                      */
/*      Utility function to see if a spatial filter is included in      */
/*      the node.                                                       */
/************************************************************************/
int FLTHasSpatialFilter(FilterEncodingNode *psNode)
{
  int bResult = MS_FALSE;

  if (!psNode)
    return MS_FALSE;

  if (psNode->eType == FILTER_NODE_TYPE_LOGICAL) {
    if (psNode->psLeftNode)
      bResult = FLTHasSpatialFilter(psNode->psLeftNode);

    if (bResult)
      return MS_TRUE;

    if (psNode->psRightNode)
      bResult = FLTHasSpatialFilter(psNode->psRightNode);

    if (bResult)
      return MS_TRUE;
  } else if (FLTIsBBoxFilter(psNode) || FLTIsPointFilter(psNode) ||
             FLTIsLineFilter(psNode) || FLTIsPolygonFilter(psNode))
    return MS_TRUE;


  return MS_FALSE;
}


/************************************************************************/
/*                     FLTCreateFeatureIdFilterEncoding                 */
/*                                                                      */
/*      Utility function to create a filter node of FeatureId type.     */
/************************************************************************/
FilterEncodingNode *FLTCreateFeatureIdFilterEncoding(char *pszString)
{
  FilterEncodingNode *psFilterNode = NULL;
  char **tokens = NULL;
  int nTokens = 0;

  if (pszString) {
    psFilterNode = FLTCreateFilterEncodingNode();
    psFilterNode->eType = FILTER_NODE_TYPE_FEATUREID;
    /*split if tyname is included in the string*/
    tokens = msStringSplit(pszString,'.', &nTokens);
    if (tokens && nTokens == 2)
      psFilterNode->pszValue = msStrdup(tokens[1]);
    else
      psFilterNode->pszValue =  msStrdup(pszString);

    if (tokens)
      msFreeCharArray(tokens, nTokens);
    return psFilterNode;
  }
  return NULL;
}


/************************************************************************/
/*                              FLTParseGMLBox                          */
/*                                                                      */
/*      Parse gml box. Used for FE 1.0                                  */
/************************************************************************/
int FLTParseGMLBox(CPLXMLNode *psBox, rectObj *psBbox, char **ppszSRS)
{
  int bCoordinatesValid = 0;
  CPLXMLNode *psCoordinates = NULL, *psCoordChild=NULL;
  CPLXMLNode *psCoord1 = NULL, *psCoord2 = NULL;
  CPLXMLNode *psX = NULL, *psY = NULL;
  char **papszCoords=NULL, **papszMin=NULL, **papszMax = NULL;
  int nCoords = 0, nCoordsMin = 0, nCoordsMax = 0;
  char *pszTmpCoord = NULL;
  const char *pszSRS = NULL;
  const char *pszTS = NULL;
  const char *pszCS = NULL;
  double minx = 0.0, miny = 0.0, maxx = 0.0, maxy = 0.0;

  if (psBox) {
    pszSRS = CPLGetXMLValue(psBox, "srsName", NULL);
    if (ppszSRS && pszSRS)
      *ppszSRS = msStrdup(pszSRS);

    psCoordinates = CPLGetXMLNode(psBox, "coordinates");
    if (!psCoordinates)
      return 0;
    pszTS = CPLGetXMLValue(psCoordinates, "ts", NULL);
    pszCS = CPLGetXMLValue(psCoordinates, "cs", NULL);

    psCoordChild =  psCoordinates->psChild;
    while (psCoordinates && psCoordChild && psCoordChild->eType != CXT_Text) {
      psCoordChild = psCoordChild->psNext;
    }
    if (psCoordChild && psCoordChild->pszValue) {
      pszTmpCoord = psCoordChild->pszValue;
      if (pszTS)
        papszCoords = msStringSplit(pszTmpCoord, pszTS[0], &nCoords);
      else
        papszCoords = msStringSplit(pszTmpCoord, ' ', &nCoords);
      if (papszCoords && nCoords == 2) {
        if (pszCS)
          papszMin = msStringSplit(papszCoords[0], pszCS[0], &nCoordsMin);
        else
          papszMin = msStringSplit(papszCoords[0], ',', &nCoordsMin);
        if (papszMin && nCoordsMin == 2) {
          if (pszCS)
            papszMax = msStringSplit(papszCoords[1], pszCS[0], &nCoordsMax);
          else
            papszMax = msStringSplit(papszCoords[1], ',', &nCoordsMax);
        }
        if (papszMax && nCoordsMax == 2) {
          bCoordinatesValid =1;
          minx =  atof(papszMin[0]);
          miny =  atof(papszMin[1]);
          maxx =  atof(papszMax[0]);
          maxy =  atof(papszMax[1]);
        }

        msFreeCharArray(papszMin, nCoordsMin);
        msFreeCharArray(papszMax, nCoordsMax);
      }

      msFreeCharArray(papszCoords, nCoords);
    } else {
      psCoord1 = CPLGetXMLNode(psBox, "coord");
      if (psCoord1 && psCoord1->psNext &&
          psCoord1->psNext->pszValue &&
          strcmp(psCoord1->psNext->pszValue, "coord") ==0) {
        psCoord2 = psCoord1->psNext;
        psX =  CPLGetXMLNode(psCoord1, "X");
        psY =  CPLGetXMLNode(psCoord1, "Y");
        if (psX && psY && psX->psChild && psY->psChild &&
            psX->psChild->pszValue && psY->psChild->pszValue) {
          minx = atof(psX->psChild->pszValue);
          miny = atof(psY->psChild->pszValue);

          psX =  CPLGetXMLNode(psCoord2, "X");
          psY =  CPLGetXMLNode(psCoord2, "Y");
          if (psX && psY && psX->psChild && psY->psChild &&
              psX->psChild->pszValue && psY->psChild->pszValue) {
            maxx = atof(psX->psChild->pszValue);
            maxy = atof(psY->psChild->pszValue);
            bCoordinatesValid = 1;
          }
        }
      }

    }
  }

  if (bCoordinatesValid) {
    psBbox->minx =  minx;
    psBbox->miny =  miny;

    psBbox->maxx =  maxx;
    psBbox->maxy =  maxy;
  }

  return bCoordinatesValid;
}
/************************************************************************/
/*                           FLTParseGMLEnvelope                        */
/*                                                                      */
/*      Utility function to parse a gml:Enevelop (used for SOS and FE1.1)*/
/************************************************************************/
int FLTParseGMLEnvelope(CPLXMLNode *psRoot, rectObj *psBbox, char **ppszSRS)
{
  CPLXMLNode *psChild=NULL;
  CPLXMLNode *psUpperCorner=NULL, *psLowerCorner=NULL;
  char *pszLowerCorner=NULL, *pszUpperCorner=NULL;
  int bValid = 0;
  char **tokens;
  int n;

  if (psRoot && psBbox && psRoot->eType == CXT_Element &&
      EQUAL(psRoot->pszValue,"Envelope")) {
    /*Get the srs if available*/
    if (ppszSRS) {
      psChild = psRoot->psChild;
      while (psChild != NULL) {
        if (psChild->eType == CXT_Attribute && psChild->pszValue &&
            EQUAL(psChild->pszValue, "srsName") && psChild->psChild &&
            psChild->psChild->pszValue) {
          *ppszSRS = msStrdup(psChild->psChild->pszValue);
          break;
        }
        psChild = psChild->psNext;
      }
    }
    psLowerCorner = CPLSearchXMLNode(psRoot, "lowerCorner");
    psUpperCorner = CPLSearchXMLNode(psRoot, "upperCorner");

    if (psLowerCorner && psUpperCorner && EQUAL(psLowerCorner->pszValue,"lowerCorner") &&
        EQUAL(psUpperCorner->pszValue,"upperCorner")) {
      /*get the values*/
      psChild = psLowerCorner->psChild;
      while (psChild != NULL) {
        if (psChild->eType != CXT_Text)
          psChild = psChild->psNext;
        else
          break;
      }
      if (psChild && psChild->eType == CXT_Text)
        pszLowerCorner = psChild->pszValue;

      psChild = psUpperCorner->psChild;
      while (psChild != NULL) {
        if (psChild->eType != CXT_Text)
          psChild = psChild->psNext;
        else
          break;
      }
      if (psChild && psChild->eType == CXT_Text)
        pszUpperCorner = psChild->pszValue;

      if (pszLowerCorner && pszUpperCorner) {
        tokens = msStringSplit(pszLowerCorner, ' ', &n);
        if (tokens && n >= 2) {
          psBbox->minx = atof(tokens[0]);
          psBbox->miny = atof(tokens[1]);

          msFreeCharArray(tokens, n);

          tokens = msStringSplit(pszUpperCorner, ' ', &n);
          if (tokens && n >= 2) {
            psBbox->maxx = atof(tokens[0]);
            psBbox->maxy = atof(tokens[1]);
            msFreeCharArray(tokens, n);

            bValid = 1;
          }
        }
      }
    }
  }
  if (bValid && ppszSRS && *ppszSRS) {
    projectionObj sProjTmp;
    msInitProjection(&sProjTmp);
    if (msLoadProjectionStringEPSG(&sProjTmp, *ppszSRS) == 0) {
      msAxisNormalizePoints( &sProjTmp, 1, &psBbox->minx, &psBbox->miny);
      msAxisNormalizePoints( &sProjTmp, 1, &psBbox->maxx, &psBbox->maxy);
    }
  }
  return bValid;
}


static void FLTReplacePropertyName(FilterEncodingNode *psFilterNode,
                                   const char *pszOldName, char *pszNewName)
{
  if (psFilterNode && pszOldName && pszNewName) {
    if (psFilterNode->eType == FILTER_NODE_TYPE_PROPERTYNAME) {
      if (psFilterNode->pszValue &&
          strcasecmp(psFilterNode->pszValue, pszOldName) == 0) {
        msFree(psFilterNode->pszValue);
        psFilterNode->pszValue = msStrdup(pszNewName);
      }
    }
    if (psFilterNode->psLeftNode)
      FLTReplacePropertyName(psFilterNode->psLeftNode, pszOldName,
                             pszNewName);
    if (psFilterNode->psRightNode)
      FLTReplacePropertyName(psFilterNode->psRightNode, pszOldName,
                             pszNewName);
  }
}


static void FLTStripNameSpacesFromPropertyName(FilterEncodingNode *psFilterNode)
{
  char **tokens=NULL;
  int n=0;

  if (psFilterNode) {
    if (psFilterNode->eType == FILTER_NODE_TYPE_PROPERTYNAME) {
      if (psFilterNode->pszValue &&
          strstr(psFilterNode->pszValue, ":")) {
        tokens = msStringSplit(psFilterNode->pszValue, ':', &n);
        if (tokens && n==2) {
          msFree(psFilterNode->pszValue);
          psFilterNode->pszValue = msStrdup(tokens[1]);
        }
        if (tokens && n>0)
          msFreeCharArray(tokens, n);
      }
    }
    if (psFilterNode->psLeftNode)
      FLTStripNameSpacesFromPropertyName(psFilterNode->psLeftNode);
    if (psFilterNode->psRightNode)
      FLTStripNameSpacesFromPropertyName(psFilterNode->psRightNode);
  }

}

/************************************************************************/
/*                        FLTPreParseFilterForAlias                     */
/*                                                                      */
/*      Utility function to replace aliased' attributes with their      */
/*      real name.                                                      */
/************************************************************************/
void FLTPreParseFilterForAlias(FilterEncodingNode *psFilterNode,
                               mapObj *map, int i, const char *namespaces)
{
  layerObj *lp=NULL;
  char szTmp[256];
  const char *pszFullName = NULL;
  int layerWasOpened =  MS_FALSE;

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

  if (psFilterNode && map && i>=0 && i<map->numlayers) {
    /*strip name speces befor hand*/
    FLTStripNameSpacesFromPropertyName(psFilterNode);

    lp = GET_LAYER(map, i);
    layerWasOpened = msLayerIsOpen(lp);
    if (msLayerOpen(lp) == MS_SUCCESS && msLayerGetItems(lp) == MS_SUCCESS) {
      for(i=0; i<lp->numitems; i++) {
        if (!lp->items[i] || strlen(lp->items[i]) <= 0)
          continue;
        snprintf(szTmp, sizeof(szTmp), "%s_alias", lp->items[i]);
        pszFullName = msOWSLookupMetadata(&(lp->metadata), namespaces, szTmp);
        if (pszFullName) {
          FLTReplacePropertyName(psFilterNode, pszFullName,
                                 lp->items[i]);
        }
      }
      if (!layerWasOpened) /* do not close the layer if it has been opened somewhere else (paging?) */
        msLayerClose(lp);
    }
  }
#else
  msSetError(MS_MISCERR, "OWS support is not available.",
             "FLTPreParseFilterForAlias()");

#endif
}


#ifdef USE_LIBXML2

xmlNodePtr FLTGetCapabilities(xmlNsPtr psNsParent, xmlNsPtr psNsOgc, int bTemporal)
{
  xmlNodePtr psRootNode = NULL, psNode = NULL, psSubNode = NULL, psSubSubNode = NULL;

  psRootNode = xmlNewNode(psNsParent, BAD_CAST "Filter_Capabilities");

  psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Spatial_Capabilities", NULL);

  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "GeometryOperands", NULL);
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand", BAD_CAST "gml:Point");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand", BAD_CAST "gml:LineString");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand", BAD_CAST "gml:Polygon");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand", BAD_CAST "gml:Envelope");

  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "SpatialOperators", NULL);
#ifdef USE_GEOS
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Equals");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Disjoint");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Touches");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Within");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Overlaps");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Crosses");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Intersects");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Contains");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "DWithin");
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Beyond");
#endif
  psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "BBOX");

  if (bTemporal) {
    psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Temporal_Capabilities", NULL);
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "TemporalOperands", NULL);
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "TemporalOperand", BAD_CAST "gml:TimePeriod");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "TemporalOperand", BAD_CAST "gml:TimeInstant");

    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "TemporalOperators", NULL);
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "TemporalOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "TM_Equals");
  }
  psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Scalar_Capabilities", NULL);
  xmlNewChild(psNode, psNsOgc, BAD_CAST "LogicalOperators", NULL);
  psNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperators", NULL);
  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "LessThan");
  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "GreaterThan");
  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "LessThanEqualTo");
  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "GreaterThanEqualTo");
  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "EqualTo");
  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "NotEqualTo");
  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "Like");
  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "Between");

  psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Id_Capabilities", NULL);
  xmlNewChild(psNode, psNsOgc, BAD_CAST "EID", NULL);
  xmlNewChild(psNode, psNsOgc, BAD_CAST "FID", NULL);
  return psRootNode;
}
#endif
#endif

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

#ifdef USE_OGR
#include "cpl_minixml.h"
#endif

#include "mapogcfilter.h"
#include "mapserver.h"
#include "mapows.h"
#include "mapowscommon.h"

#ifdef USE_OGR

char *FLTGetIsLikeComparisonCommonExpression(FilterEncodingNode *psFilterNode)
{
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  char szTmp[256];
  char *pszValue = NULL;

  const char *pszWild = NULL;
  const char *pszSingle = NULL;
  const char *pszEscape = NULL;
  int  bCaseInsensitive = 0;
  FEPropertyIsLike* propIsLike;

  int nLength=0, i=0, iTmp=0;

  if (!psFilterNode || !psFilterNode->pOther || !psFilterNode->psLeftNode || !psFilterNode->psRightNode || !psFilterNode->psRightNode->pszValue)
    return NULL;

  propIsLike = (FEPropertyIsLike *)psFilterNode->pOther;
  pszWild = propIsLike->pszWildCard;
  pszSingle = propIsLike->pszSingleChar;
  pszEscape = propIsLike->pszEscapeChar;
  bCaseInsensitive = propIsLike->bCaseInsensitive;

  if (!pszWild || strlen(pszWild) == 0 || !pszSingle || strlen(pszSingle) == 0 || !pszEscape || strlen(pszEscape) == 0)
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      Use operand with regular expressions.                           */
  /* -------------------------------------------------------------------- */
  szBuffer[0] = '\0';
  sprintf(szTmp, "%s", "(\"[");
  szTmp[4] = '\0';

  strlcat(szBuffer, szTmp, bufferSize);

  /* attribute */
  strlcat(szBuffer, psFilterNode->psLeftNode->pszValue, bufferSize);
  szBuffer[strlen(szBuffer)] = '\0';

  /* #3521 */
  if (bCaseInsensitive == 1)
    sprintf(szTmp, "%s", "]\" ~* \"");
  else
    sprintf(szTmp, "%s", "]\" ~ \"");
  szTmp[7] = '\0';
  strlcat(szBuffer, szTmp, bufferSize);
  szBuffer[strlen(szBuffer)] = '\0';

  pszValue = psFilterNode->psRightNode->pszValue;
  nLength = strlen(pszValue);
  if( 1 + 2 * nLength + 1 + 1 >= sizeof(szTmp) )
      return NULL;

  iTmp =0;
  if (nLength > 0 && pszValue[0] != pszWild[0] && pszValue[0] != pszSingle[0] && pszValue[0] != pszEscape[0]) {
    szTmp[iTmp]= '^';
    iTmp++;
  }
  for (i=0; i<nLength; i++) {
    if (pszValue[i] != pszWild[0] && pszValue[i] != pszSingle[0] && pszValue[i] != pszEscape[0]) {
      szTmp[iTmp] = pszValue[i];
      iTmp++;
      szTmp[iTmp] = '\0';
    } else if (pszValue[i] == pszSingle[0]) {
      szTmp[iTmp] = '.';
      iTmp++;
      szTmp[iTmp] = '\0';
    } else if (pszValue[i] == pszEscape[0]) {
      szTmp[iTmp] = '\\';
      iTmp++;
      szTmp[iTmp] = '\0';
    } else if (pszValue[i] == pszWild[0]) {
      szTmp[iTmp++] = '.';
      szTmp[iTmp++] = '*';
      szTmp[iTmp] = '\0';
    }
  }
  szTmp[iTmp] = '"';
  szTmp[++iTmp] = '\0';

  strlcat(szBuffer, szTmp, bufferSize);
  strlcat(szBuffer, ")", bufferSize);
  return msStrdup(szBuffer);
}

char *FLTGetIsBetweenComparisonCommonExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  char **aszBounds = NULL;
  int nBounds = 0;
  int bString=0;
  int bDateTime = 0;
  char *pszExpression=NULL, *pszTmpEscaped;

  if (!psFilterNode || !(strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0))
    return NULL;

  if (psFilterNode->psLeftNode == NULL || psFilterNode->psRightNode == NULL )
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
    const char* pszType;
    snprintf(szBuffer,  bufferSize, "%s_type",  psFilterNode->psLeftNode->pszValue);
    pszType = msOWSLookupMetadata(&(lp->metadata), "OFG", szBuffer);
    if (pszType != NULL && (strcasecmp(pszType, "Character") == 0))
      bString = 1;
    else if (pszType != NULL && (strcasecmp(pszType, "Date") == 0))
      bDateTime = 1;
    else if (FLTIsNumeric(aszBounds[0]) == MS_FALSE)
      bString = 1;
  }
  if (!bString && !bDateTime) {
    if (aszBounds[1]) {
      if (FLTIsNumeric(aszBounds[1]) == MS_FALSE)
        bString = 1;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      build expresssion.                                              */
  /* -------------------------------------------------------------------- */
  /* attribute */
  if (bString)
    sprintf(szBuffer, "%s", "(\"[");
  else
    sprintf(szBuffer, "%s", "([");
  pszExpression = msStringConcatenate(pszExpression, szBuffer);
  
  pszExpression = msStringConcatenate(pszExpression, psFilterNode->psLeftNode->pszValue);

  if (bString)
    sprintf(szBuffer, "%s", "]\" ");
  else
    sprintf(szBuffer, "%s", "] ");
  pszExpression = msStringConcatenate(pszExpression, szBuffer);

  sprintf(szBuffer, "%s", " >= ");
  pszExpression = msStringConcatenate(pszExpression, szBuffer);

  if (bString) {
    pszExpression = msStringConcatenate(pszExpression, "\"");
  }
  else if (bDateTime) {
    pszExpression = msStringConcatenate(pszExpression, "`");
  }

  pszTmpEscaped = msStringEscape(aszBounds[0]);
  snprintf(szBuffer, bufferSize, "%s", pszTmpEscaped);
  if(pszTmpEscaped != aszBounds[0] ) msFree(pszTmpEscaped);
  pszExpression = msStringConcatenate(pszExpression, szBuffer);
  if (bString) {
    pszExpression = msStringConcatenate(pszExpression, "\"");
  }
  else if (bDateTime) {
    pszExpression = msStringConcatenate(pszExpression, "`");
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
  if (bString) {
    pszExpression = msStringConcatenate(pszExpression, "\"");
  }
  else if (bDateTime) {
    pszExpression = msStringConcatenate(pszExpression, "`");
  }
  pszTmpEscaped = msStringEscape(aszBounds[1]);
  snprintf(szBuffer, bufferSize, "%s", pszTmpEscaped);
  if (pszTmpEscaped != aszBounds[1]) msFree(pszTmpEscaped);
  pszExpression = msStringConcatenate(pszExpression, szBuffer);

  if (bString) {
    pszExpression = msStringConcatenate(pszExpression, "\"");
  }
  else if (bDateTime) {
    pszExpression = msStringConcatenate(pszExpression, "`");
  }
  sprintf(szBuffer, "%s", ")");
  pszExpression = msStringConcatenate(pszExpression, szBuffer);

  msFreeCharArray(aszBounds, nBounds);

  return pszExpression;
}

char *FLTGetBinaryComparisonCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  char szTmp[1024];
  char *pszExpression = NULL, *pszTmpEscaped;
  int bString;
  int bDateTime;

  if (psFilterNode == NULL)
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      check if the value is a numeric value or alphanumeric. If it    */
  /*      is alphanumeric, add quotes around attribute and values.        */
  /* -------------------------------------------------------------------- */
  bString = 0;
  bDateTime = 0;
  if (psFilterNode->psRightNode->pszValue) {
    const char* pszType;
    snprintf(szTmp, sizeof(szTmp), "%s_type",  psFilterNode->psLeftNode->pszValue);
    pszType = msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp);
    if (pszType!= NULL && (strcasecmp(pszType, "Character") == 0))
      bString = 1;
    else if (pszType!= NULL && (strcasecmp(pszType, "Date") == 0))
      bDateTime = 1;
    else if (FLTIsNumeric(psFilterNode->psRightNode->pszValue) == MS_FALSE)
      bString = 1;
  }

  /* specical case to be able to have empty strings in the expression. */
  /* propertyislike is always treated as string */
  if (psFilterNode->psRightNode->pszValue == NULL || strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
    bString = 1;

  /* attribute */
  if (bString)
    sprintf(szTmp, "%s", "(\"[");
  else
    sprintf(szTmp,  "%s","([");
  pszExpression = msStringConcatenate(pszExpression, szTmp);
  pszExpression = msStringConcatenate(pszExpression, psFilterNode->psLeftNode->pszValue);
  
  if (bString)
    sprintf(szTmp,  "%s","]\" ");
  else
    sprintf(szTmp,  "%s", "] ");
  pszExpression = msStringConcatenate(pszExpression, szTmp);

  if (strcasecmp(psFilterNode->pszValue, "PropertyIsEqualTo") == 0) {
    /* case insensitive set ? */
    if (psFilterNode->psRightNode->pOther && (*(int *)psFilterNode->psRightNode->pOther) == 1)
      sprintf(szTmp,  "%s", "=*");
    else
      sprintf(szTmp,  "%s", "=");
  } else if (strcasecmp(psFilterNode->pszValue, "PropertyIsNotEqualTo") == 0)
    sprintf(szTmp,  "%s", "!=");
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLessThan") == 0)
    sprintf(szTmp,  "%s", "<");
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsGreaterThan") == 0)
    sprintf(szTmp,  "%s", ">");
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLessThanOrEqualTo") == 0)
    sprintf(szTmp,  "%s", "<=");
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsGreaterThanOrEqualTo") == 0)
    sprintf(szTmp,  "%s", ">=");
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
    sprintf(szTmp,  "%s", "~");

  pszExpression = msStringConcatenate(pszExpression, szTmp);
  pszExpression = msStringConcatenate(pszExpression, " ");

  /* value */
  if (bString) {
    sprintf(szTmp,  "%s", "\"");
    pszExpression = msStringConcatenate(pszExpression, szTmp);
  }
  else if (bDateTime) {
    sprintf(szTmp,  "%s", "`");
    pszExpression = msStringConcatenate(pszExpression, szTmp);
  }

  if (psFilterNode->psRightNode->pszValue) {
    pszTmpEscaped = msStringEscape(psFilterNode->psRightNode->pszValue);
    pszExpression = msStringConcatenate(pszExpression, pszTmpEscaped);
    if(pszTmpEscaped != psFilterNode->psRightNode->pszValue ) msFree(pszTmpEscaped);
  }

  if (bString) {
    sprintf(szTmp,  "%s", "\"");
    pszExpression = msStringConcatenate(pszExpression, szTmp);
  }
  else if (bDateTime) {
    sprintf(szTmp,  "%s", "`");
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

  if (!psFilterNode || !FLTIsLogicalFilterType(psFilterNode->pszValue))
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      OR and AND                                                      */
  /* -------------------------------------------------------------------- */
  if (psFilterNode->psLeftNode && psFilterNode->psRightNode) {
    pszTmp = FLTGetCommonExpression(psFilterNode->psLeftNode, lp);
    if (!pszTmp)
      return NULL;

    pszExpression = msStringConcatenate(pszExpression, "(");

    pszExpression = msStringConcatenate(pszExpression, pszTmp);
    msFree(pszTmp);

    pszExpression = msStringConcatenate(pszExpression, " ");

    pszExpression = msStringConcatenate(pszExpression, psFilterNode->pszValue);

    pszExpression = msStringConcatenate(pszExpression, " ");

    pszTmp = FLTGetCommonExpression(psFilterNode->psRightNode, lp);
    if (!pszTmp) {
      msFree(pszExpression);
      return NULL;
    }

    pszExpression = msStringConcatenate(pszExpression, pszTmp);
    msFree(pszTmp);

    pszExpression = msStringConcatenate(pszExpression, ")");
  }
  /* -------------------------------------------------------------------- */
  /*      NOT                                                             */
  /* -------------------------------------------------------------------- */
  else if (psFilterNode->psLeftNode && strcasecmp(psFilterNode->pszValue, "NOT") == 0) {
    pszTmp = FLTGetCommonExpression(psFilterNode->psLeftNode, lp);
    if (!pszTmp)
      return NULL;

    pszExpression = msStringConcatenate(pszExpression, "(NOT ");

    pszExpression = msStringConcatenate(pszExpression, pszTmp);
    msFree(pszTmp);

    pszExpression = msStringConcatenate(pszExpression, ")");
  }

  return pszExpression;
}

char *FLTGetSpatialComparisonCommonExpression(FilterEncodingNode *psNode, layerObj *lp)
{
  char *pszExpression = NULL;
  shapeObj *psQueryShape = NULL;
  double dfDistance = -1;
  int nUnit = -1, nLayerUnit = -1;
  char *pszWktText = NULL;
  char szBuffer[256];
  char *pszTmp=NULL;
  projectionObj sProjTmp;
  rectObj sQueryRect;
  shapeObj *psTmpShape=NULL;
  int bBBoxQuery = 0;
  int bAlreadyReprojected = 0;

  if (psNode == NULL || lp == NULL)
    return NULL;

  if (psNode->eType != FILTER_NODE_TYPE_SPATIAL)
    return NULL;

  /* get the shape */
  if (FLTIsBBoxFilter(psNode)) {
    char szPolygon[512];
    FLTGetBBOX(psNode, &sQueryRect);

    snprintf(szPolygon, sizeof(szPolygon),
             "POLYGON((%.18f %.18f,%.18f %.18f,%.18f %.18f,%.18f %.18f,%.18f %.18f))",
             sQueryRect.minx, sQueryRect.miny,
             sQueryRect.minx, sQueryRect.maxy,
             sQueryRect.maxx, sQueryRect.maxy,
             sQueryRect.maxx, sQueryRect.miny,
             sQueryRect.minx, sQueryRect.miny);

    psTmpShape = msShapeFromWKT(szPolygon);

    /* 
    ** This is a horrible hack to deal with world-extent requests and
    ** reprojection. msProjectRect() detects if reprojection from longlat to 
    ** projected SRS, and in that case it transforms the bbox to -1e-15,-1e-15,1e15,1e15
    ** to ensure that all features are returned.
    **
    ** Make wfs_200_cite_filter_bbox_world.xml and wfs_200_cite_postgis_bbox_world.xml pass
    */
    if (fabs(sQueryRect.minx - -180.0) < 1e-5 &&
        fabs(sQueryRect.miny - -90.0) < 1e-5 &&
        fabs(sQueryRect.maxx - 180.0) < 1e-5 &&
        fabs(sQueryRect.maxy - 90.0) < 1e-5)
    {
      if (lp->projection.numargs > 0) {
        if (psNode->pszSRS)
          msInitProjection(&sProjTmp);
        if (psNode->pszSRS) {
          /* Use the non EPSG variant since axis swapping is done in FLTDoAxisSwappingIfNecessary */
          if (msLoadProjectionString(&sProjTmp, psNode->pszSRS) == 0) {
            msProjectRect(&sProjTmp, &lp->projection, &sQueryRect);
          }
        } else if (lp->map->projection.numargs > 0)
          msProjectRect(&lp->map->projection, &lp->projection, &sQueryRect);
        if (psNode->pszSRS)
          msFreeProjection(&sProjTmp);
      }
      if (sQueryRect.minx <= -1e14) {
        msFreeShape(psTmpShape);
        msFree(psTmpShape);
        psTmpShape = (shapeObj*) msSmallMalloc(sizeof(shapeObj));
        msInitShape(psTmpShape);
        msRectToPolygon(sQueryRect, psTmpShape);
        bAlreadyReprojected = 1;
      }
    }

    bBBoxQuery = 1;
  } else {
    /* other geos type operations */

    /* project shape to layer projection. If the proj is not part of the filter query,
      assume that the cooredinates are in the map projection */

    psQueryShape = FLTGetShape(psNode, &dfDistance, &nUnit);

    if ((strcasecmp(psNode->pszValue, "DWithin") == 0 || strcasecmp(psNode->pszValue, "Beyond") == 0 ) && dfDistance > 0) {
      nLayerUnit = lp->units;
      if(nLayerUnit == -1) nLayerUnit = GetMapserverUnitUsingProj(&lp->projection);
      if(nLayerUnit == -1) nLayerUnit = lp->map->units;
      if(nLayerUnit == -1) nLayerUnit = GetMapserverUnitUsingProj(&lp->map->projection);

      if (nUnit >= 0 && nUnit != nLayerUnit)
        dfDistance *= msInchesPerUnit(nUnit,0)/msInchesPerUnit(nLayerUnit,0); /* target is layer units */
    }

    psTmpShape = psQueryShape;
  }

  if (psTmpShape) {

    /*
    ** target is layer projection
    */
    if (!bAlreadyReprojected && lp->projection.numargs > 0) {
      if (psNode->pszSRS)
        msInitProjection(&sProjTmp);
      if (psNode->pszSRS) {
        /* Use the non EPSG variant since axis swapping is done in FLTDoAxisSwappingIfNecessary */
        if (msLoadProjectionString(&sProjTmp, psNode->pszSRS) == 0) {
          msProjectShape(&sProjTmp, &lp->projection, psTmpShape);
        }
      } else if (lp->map->projection.numargs > 0)
        msProjectShape(&lp->map->projection, &lp->projection, psTmpShape);
      if (psNode->pszSRS)
        msFreeProjection(&sProjTmp);
    }

    /* function name */
    if (bBBoxQuery) {
      sprintf(szBuffer, "%s", "intersects");
    } else {
      if (strncasecmp(psNode->pszValue, "intersect", 9) == 0)
        sprintf(szBuffer, "%s", "intersects");
      else {
        pszTmp = msStrdup(psNode->pszValue);
        msStringToLower(pszTmp);
        sprintf(szBuffer, "%s", pszTmp);
        msFree(pszTmp);
      }
    }
    pszExpression = msStringConcatenate(pszExpression, szBuffer);
    pszExpression = msStringConcatenate(pszExpression, "(");

    /* geometry binding */
    sprintf(szBuffer, "%s", "[shape]");
    pszExpression = msStringConcatenate(pszExpression, szBuffer);
    pszExpression = msStringConcatenate(pszExpression, ",");

    /* filter geometry */
    pszWktText = msGEOSShapeToWKT(psTmpShape);
    sprintf(szBuffer, "%s", "fromText('");
    pszExpression = msStringConcatenate(pszExpression, szBuffer);
    pszExpression = msStringConcatenate(pszExpression, pszWktText);
    sprintf(szBuffer, "%s", "')");
    pszExpression = msStringConcatenate(pszExpression, szBuffer);
    msGEOSFreeWKT(pszWktText);

    /* (optional) beyond/dwithin distance, always 0.0 since we apply the distance as a buffer earlier */
    if ((strcasecmp(psNode->pszValue, "DWithin") == 0 || strcasecmp(psNode->pszValue, "Beyond") == 0)) {
      // pszExpression = msStringConcatenate(pszExpression, ",0.0");
      sprintf(szBuffer, ",%g", dfDistance);
      pszExpression = msStringConcatenate(pszExpression, szBuffer);      
    }

    /* terminate the function */
    pszExpression = msStringConcatenate(pszExpression, ") = TRUE");
  }

  /*
  ** Cleanup
  */
  if (bBBoxQuery) {
    msFreeShape(psTmpShape);
    msFree(psTmpShape);
  }

  return pszExpression;
}

char *FLTGetFeatureIdCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  char *pszExpression = NULL;
  int nTokens = 0, i=0, bString=0;
  char **tokens = NULL;
  const char *pszAttribute=NULL;

#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) || defined(USE_SOS_SVR)
  if (psFilterNode->pszValue) {
    pszAttribute = msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
    if (pszAttribute) {
      tokens = msStringSplit(psFilterNode->pszValue,',', &nTokens);
      if (tokens && nTokens > 0) {
        for (i=0; i<nTokens; i++) {
          char *pszTmp = NULL;
          int bufferSize = 0;
          const char* pszId = tokens[i];
          const char* pszDot = strchr(pszId, '.');
          if( pszDot )
            pszId = pszDot + 1;

          if (i == 0) {
            if(FLTIsNumeric(pszId) == MS_FALSE)
              bString = 1;
          }

          if (bString) {
            bufferSize = 11+strlen(pszId)+strlen(pszAttribute)+1;
            pszTmp = (char *)msSmallMalloc(bufferSize);
            snprintf(pszTmp, bufferSize, "(\"[%s]\" ==\"%s\")" , pszAttribute, pszId);
          } else {
            bufferSize = 8+strlen(pszId)+strlen(pszAttribute)+1;
            pszTmp = (char *)msSmallMalloc(bufferSize);
            snprintf(pszTmp, bufferSize, "([%s] == %s)" , pszAttribute, pszId);
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

    /* opening and closing brackets are needed for mapserver expressions */
    if (pszExpression)
      pszExpression = msStringConcatenate(pszExpression, ")");
  }
#endif

  return pszExpression;
}

char* FLTGetTimeExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  char* pszExpression = NULL;
  const char* pszTimeField;
  const char* pszTimeValue;

  if (psFilterNode == NULL || lp == NULL)
    return NULL;

  if (psFilterNode->eType != FILTER_NODE_TYPE_TEMPORAL)
    return NULL;

  pszTimeValue = FLTGetDuring(psFilterNode, &pszTimeField);
  if (pszTimeField && pszTimeValue) {
    expressionObj old_filter;
    msInitExpression(&old_filter);
    msCopyExpression(&old_filter, &lp->filter); /* save existing filter */
    msFreeExpression(&lp->filter);
    if (msLayerSetTimeFilter(lp, pszTimeValue, pszTimeField) == MS_TRUE) {
      pszExpression = msStrdup(lp->filter.string);
    }
    msCopyExpression(&lp->filter, &old_filter); /* restore old filter */
    msFreeExpression(&old_filter);
  }
  return pszExpression;
}

char *FLTGetCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  char *pszExpression = NULL;

  if (!psFilterNode)
    return NULL;

  if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON) {
    if ( psFilterNode->psLeftNode && psFilterNode->psRightNode) {
      if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
        pszExpression = FLTGetBinaryComparisonCommonExpression(psFilterNode, lp);
      else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
        pszExpression = FLTGetIsLikeComparisonCommonExpression(psFilterNode);
      else if (strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0)
        pszExpression = FLTGetIsBetweenComparisonCommonExpresssion(psFilterNode, lp);
    }
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL) {
    pszExpression = FLTGetLogicalComparisonCommonExpression(psFilterNode, lp);
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL) {
    pszExpression = FLTGetSpatialComparisonCommonExpression(psFilterNode, lp);
  } else if (psFilterNode->eType ==  FILTER_NODE_TYPE_FEATUREID) {
    pszExpression = FLTGetFeatureIdCommonExpression(psFilterNode, lp);
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_TEMPORAL) {
    pszExpression = FLTGetTimeExpression(psFilterNode, lp);
  }

  return pszExpression;
}

int FLTApplyFilterToLayerCommonExpression(mapObj *map, int iLayerIndex, const char *pszExpression)
{
  return FLTApplyFilterToLayerCommonExpressionWithRect(map, iLayerIndex, pszExpression, map->extent);
}

/* rect must be in map->projection */
int FLTApplyFilterToLayerCommonExpressionWithRect(mapObj *map, int iLayerIndex, const char *pszExpression, rectObj rect)
{
  int retval;
  int save_startindex;
  int save_maxfeatures;
  int save_only_cache_result_count;

  save_startindex = map->query.startindex;
  save_maxfeatures = map->query.maxfeatures;
  save_only_cache_result_count = map->query.only_cache_result_count;
  msInitQuery(&(map->query));
  map->query.startindex = save_startindex;
  map->query.maxfeatures = save_maxfeatures;
  map->query.only_cache_result_count = save_only_cache_result_count;

  map->query.mode = MS_QUERY_MULTIPLE;
  map->query.layer = iLayerIndex;

  map->query.rect = rect;

  if( pszExpression )
  {
    map->query.type = MS_QUERY_BY_FILTER;
    msInitExpression(&map->query.filter);
    map->query.filter.string = msStrdup(pszExpression);
    map->query.filter.type = MS_EXPRESSION; /* a logical expression */

    retval = msQueryByFilter(map);
  }
  else
  {
    map->query.type = MS_QUERY_BY_RECT;
    retval = msQueryByRect(map);
  }

  return retval;
}

#endif

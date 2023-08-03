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

#include "mapogcfilter.h"
#include "mapserver.h"
#include "mapows.h"
#include "mapowscommon.h"
#include "cpl_minixml.h"

#include <string>

static std::string FLTGetIsLikeComparisonCommonExpression(FilterEncodingNode *psFilterNode)
{
  /* From http://pubs.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap09.html#tag_09_04 */
  /* also add double quote because we are within a string */
  const char* pszRegexSpecialCharsAndDoubleQuote = "\\^${[().*+?|\"";

  if (!psFilterNode || !psFilterNode->pOther || !psFilterNode->psLeftNode || !psFilterNode->psRightNode || !psFilterNode->psRightNode->pszValue)
    return std::string();

  const FEPropertyIsLike* propIsLike = (const FEPropertyIsLike *)psFilterNode->pOther;
  const char* pszWild = propIsLike->pszWildCard;
  const char* pszSingle = propIsLike->pszSingleChar;
  const char* pszEscape = propIsLike->pszEscapeChar;
  const bool bCaseInsensitive = propIsLike->bCaseInsensitive != 0;

  if (!pszWild || strlen(pszWild) == 0 || !pszSingle || strlen(pszSingle) == 0 || !pszEscape || strlen(pszEscape) == 0)
    return std::string();

  /* -------------------------------------------------------------------- */
  /*      Use operand with regular expressions.                           */
  /* -------------------------------------------------------------------- */
  std::string expr("(\"[");

  /* attribute */
  expr += psFilterNode->psLeftNode->pszValue;

  /* #3521 */
  if (bCaseInsensitive )
    expr += "]\" ~* \"";
  else
    expr += "]\" ~ \"";

  const char* pszValue = psFilterNode->psRightNode->pszValue;
  const size_t nLength = strlen(pszValue);

  if (nLength > 0) {
    expr += '^';
  }
  for (size_t i=0; i<nLength; i++) {
    if (pszValue[i] == pszSingle[0]) {
      expr += '.';
    /* The Filter escape character is supposed to only escape the single, wildcard and escape character */
    } else if (pszValue[i] == pszEscape[0] && (
                    pszValue[i+1] == pszSingle[0] ||
                    pszValue[i+1] == pszWild[0] ||
                    pszValue[i+1] == pszEscape[0])) {
      if( pszValue[i+1] == '\\' )
      {
          /* Tricky case: \ must be escaped ncce in the regular expression context
             so that the regexp matches it as an ordinary character.
             But as \ is also the escape character for MapServer string, we
             must escape it again. */
          expr += "\\" "\\" "\\" "\\";
      }
      else
      {
        /* If the escaped character is itself a regular expression special character */
        /* we need to regular-expression-escape-it ! */
        if( strchr(pszRegexSpecialCharsAndDoubleQuote, pszValue[i+1]) )
        {
            expr += '\\';
        }
        expr += pszValue[i+1];
      }
      i++;
    } else if (pszValue[i] == pszWild[0]) {
      expr += ".*";
    }
    /* Escape regular expressions special characters and double quote */
    else if (strchr(pszRegexSpecialCharsAndDoubleQuote, pszValue[i]))
    {
      if( pszValue[i] == '\\' )
      {
          /* See above explantation */
          expr += "\\" "\\" "\\" "\\";
      }
      else
      {
        expr += '\\';
        expr += pszValue[i];
      }
    }
    else {
      expr += pszValue[i];
    }
  }
  if (nLength > 0) {
    expr += '$';
  }
  expr += "\")";
  return expr;
}

static std::string FLTGetIsBetweenComparisonCommonExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  if (psFilterNode->psLeftNode == NULL || psFilterNode->psRightNode == NULL )
    return std::string();

  /* -------------------------------------------------------------------- */
  /*      Get the bounds value which are stored like boundmin;boundmax    */
  /* -------------------------------------------------------------------- */
  const auto bounds = msStringSplit(psFilterNode->psRightNode->pszValue, ';');
  if (bounds.size() != 2) {
    return std::string();
  }

  /* -------------------------------------------------------------------- */
  /*      check if the value is a numeric value or alphanumeric. If it    */
  /*      is alphanumeric, add quotes around attribute and values.        */
  /* -------------------------------------------------------------------- */
  bool bString = false;
  bool bDateTime = false;

  const char* pszType = msOWSLookupMetadata(&(lp->metadata), "OFG",
    (std::string(psFilterNode->psLeftNode->pszValue)+ "_type").c_str());
  if (pszType != NULL && (strcasecmp(pszType, "Character") == 0))
    bString = true;
  else if (pszType != NULL && (strcasecmp(pszType, "Date") == 0))
    bDateTime = true;
  else if (FLTIsNumeric(bounds[0].c_str()) == MS_FALSE)
    bString = true;

  if (!bString && !bDateTime) {
      if (FLTIsNumeric(bounds[1].c_str()) == MS_FALSE)
        bString = true;
  }

  std::string expr;
  /* -------------------------------------------------------------------- */
  /*      build expresssion.                                              */
  /* -------------------------------------------------------------------- */
  /* attribute */
  if (bString)
    expr += "(\"[";
  else
    expr += "([";

  expr += psFilterNode->psLeftNode->pszValue;

  if (bString)
    expr += "]\" ";
  else
    expr += "] ";

  expr += " >= ";

  if (bString) {
    expr += '\"';
  }
  else if (bDateTime) {
    expr += '`';
  }

  expr += msStdStringEscape(bounds[0].c_str());

  if (bString) {
    expr += '\"';
  }
  else if (bDateTime) {
    expr += '`';
  }

  expr += " AND ";

  if (bString)
    expr += " \"[";
  else
    expr += " [";

  /* attribute */
  expr += psFilterNode->psLeftNode->pszValue;

  if (bString)
    expr += "]\" ";
  else
    expr += "] ";

  expr += " <= ";

  if (bString) {
    expr += '\"';
  }
  else if (bDateTime) {
    expr += '`';
  }

  expr += msStdStringEscape(bounds[1].c_str());

  if (bString) {
    expr += '\"';
  }
  else if (bDateTime) {
    expr += '`';
  }
  expr += ')';

  return expr;
}

static
std::string FLTGetBinaryComparisonCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  /* -------------------------------------------------------------------- */
  /*      check if the value is a numeric value or alphanumeric. If it    */
  /*      is alphanumeric, add quotes around attribute and values.        */
  /* -------------------------------------------------------------------- */
  bool bString = false;
  bool bDateTime = false;
  if (psFilterNode->psRightNode->pszValue) {
    const char* pszType = msOWSLookupMetadata(&(lp->metadata), "OFG",
        (std::string(psFilterNode->psLeftNode->pszValue) + "_type").c_str());
    if (pszType!= NULL && (strcasecmp(pszType, "Character") == 0))
      bString = true;
    else if (pszType!= NULL && (strcasecmp(pszType, "Date") == 0))
      bDateTime = true;
    else if (FLTIsNumeric(psFilterNode->psRightNode->pszValue) == MS_FALSE)
      bString = true;
  }

  /* specical case to be able to have empty strings in the expression. */
  /* propertyislike is always treated as string */
  if (psFilterNode->psRightNode->pszValue == NULL || strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
    bString = true;

  /* attribute */
  std::string expr;
  if (bString)
    expr = "(\"[";
  else
    expr = "([";
  expr += psFilterNode->psLeftNode->pszValue;
  
  if (bString)
    expr += "]\" ";
  else
    expr += "] ";

  if (strcasecmp(psFilterNode->pszValue, "PropertyIsEqualTo") == 0) {
    /* case insensitive set ? */
    if (psFilterNode->psRightNode->pOther && (*(int *)psFilterNode->psRightNode->pOther) == 1)
      expr += "=*";
    else
      expr += "=";
  } else if (strcasecmp(psFilterNode->pszValue, "PropertyIsNotEqualTo") == 0)
    expr += "!=";
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLessThan") == 0)
    expr += "<";
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsGreaterThan") == 0)
    expr += ">";
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLessThanOrEqualTo") == 0)
    expr += "<=";
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsGreaterThanOrEqualTo") == 0)
    expr += ">=";
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
    expr += "~";
  expr += ' ';

  /* value */
  if (bString) {
    expr += "\"";
  }
  else if (bDateTime) {
    expr += "`";
  }

  if (psFilterNode->psRightNode->pszValue) {
    expr += msStdStringEscape(psFilterNode->psRightNode->pszValue);
  }

  if (bString) {
    expr += "\"";
  }
  else if (bDateTime) {
    expr += "`";
  }

  expr += ")";

  return expr;
}

static
std::string FLTGetLogicalComparisonCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  std::string expr;
  /* -------------------------------------------------------------------- */
  /*      OR and AND                                                      */
  /* -------------------------------------------------------------------- */
  if (psFilterNode->psLeftNode && psFilterNode->psRightNode) {
    char* pszTmp = FLTGetCommonExpression(psFilterNode->psLeftNode, lp);
    if (!pszTmp)
      return std::string();

    expr = '(';
    expr += pszTmp;
    msFree(pszTmp);
    expr += ' ';
    expr += psFilterNode->pszValue;
    expr += ' ';

    pszTmp = FLTGetCommonExpression(psFilterNode->psRightNode, lp);
    if (!pszTmp) {
      return std::string();
    }

    expr += pszTmp;
    msFree(pszTmp);
    expr += ')';
  }
  /* -------------------------------------------------------------------- */
  /*      NOT                                                             */
  /* -------------------------------------------------------------------- */
  else if (psFilterNode->psLeftNode && strcasecmp(psFilterNode->pszValue, "NOT") == 0) {
    char* pszTmp = FLTGetCommonExpression(psFilterNode->psLeftNode, lp);
    if (!pszTmp)
      return std::string();

    expr = "(NOT ";
    expr += pszTmp;
    msFree(pszTmp);
    expr += ')';
  }

  return expr;
}

static
std::string FLTGetSpatialComparisonCommonExpression(FilterEncodingNode *psNode, layerObj *lp)
{
  std::string expr;
  double dfDistance = -1;
  shapeObj *psTmpShape=NULL;
  bool bBBoxQuery = false;
  bool bAlreadyReprojected = false;

  if (lp == NULL)
    return std::string();

  /* get the shape */
  if (FLTIsBBoxFilter(psNode)) {
    rectObj sQueryRect;
    FLTGetBBOX(psNode, &sQueryRect);

    char szPolygon[512];
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
        projectionObj sProjTmp;
        if (psNode->pszSRS) {
          msInitProjection(&sProjTmp);
          msProjectionInheritContextFrom(&sProjTmp, &lp->projection);
        }
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
        bAlreadyReprojected = true;
      }
    }

    bBBoxQuery = true;
  } else {
    /* other geos type operations */

    /* project shape to layer projection. If the proj is not part of the filter query,
      assume that the cooredinates are in the map projection */

    int nUnit = -1;
    shapeObj* psQueryShape = FLTGetShape(psNode, &dfDistance, &nUnit);

    if ((strcasecmp(psNode->pszValue, "DWithin") == 0 || strcasecmp(psNode->pszValue, "Beyond") == 0 ) && dfDistance > 0) {
      int nLayerUnit = lp->units;
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
      projectionObj sProjTmp;
      if (psNode->pszSRS) {
        msInitProjection(&sProjTmp);
        msProjectionInheritContextFrom(&sProjTmp, &lp->projection);
      }
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
      expr = "intersects";
    } else {
      if (strncasecmp(psNode->pszValue, "intersect", 9) == 0)
        expr = "intersects";
      else {
        expr = msStringToLower(std::string(psNode->pszValue));
      }
    }
    /* geometry binding */
    expr += "([shape],fromText('";

    /* filter geometry */
    char* pszWktText = msGEOSShapeToWKT(psTmpShape);
    expr += pszWktText ? pszWktText : "Cannot translate shape to WKT";
    expr += "')";
    msGEOSFreeWKT(pszWktText);

    /* (optional) beyond/dwithin distance, always 0.0 since we apply the distance as a buffer earlier */
    if ((strcasecmp(psNode->pszValue, "DWithin") == 0 || strcasecmp(psNode->pszValue, "Beyond") == 0)) {
      char szBuffer[256];
      sprintf(szBuffer, ",%g", dfDistance);
      expr += szBuffer;
    }

    /* terminate the function */
    expr += ") = TRUE";
  }

  /*
  ** Cleanup
  */
  if (bBBoxQuery) {
    msFreeShape(psTmpShape);
    msFree(psTmpShape);
  }

  return expr;
}

static std::string FLTGetFeatureIdCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  std::string expr;

#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) || defined(USE_SOS_SVR)
  if (psFilterNode->pszValue) {
    const char* pszAttribute = msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
    if (pszAttribute) {
      const auto tokens = msStringSplit(psFilterNode->pszValue,',');
      if (!tokens.empty()) {
        for (size_t i=0; i < tokens.size(); i++) {
          const char* pszId = tokens[i].c_str();
          const char* pszDot = strrchr(pszId, '.');
          if( pszDot )
            pszId = pszDot + 1;

          bool bString = false;
          if (i == 0) {
            if(FLTIsNumeric(pszId) == MS_FALSE)
              bString = true;
          }

         if (!expr.empty())
            expr += " OR ";
          else
            expr = '(';

          if (bString) {
            expr += "(\"[";
            expr += pszAttribute;
            expr += "]\" == \"";
            expr += pszId;
            expr += "\")";
          } else {
            expr += "([";
            expr += pszAttribute;
            expr += "] == ";
            expr += pszId;
            expr += ")";
          }
        }
      }
    }

    /* opening and closing brackets are needed for mapserver expressions */
    if (!expr.empty())
      expr += ')';
  }
#endif

  return expr;
}

std::string FLTGetTimeExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  if (lp == NULL)
    return std::string();

  std::string expr;
  const char* pszTimeField = nullptr;
  const char* pszTimeValue = FLTGetDuring(psFilterNode, &pszTimeField);
  if (pszTimeField && pszTimeValue) {
    expressionObj old_filter;
    msInitExpression(&old_filter);
    msCopyExpression(&old_filter, &lp->filter); /* save existing filter */
    msFreeExpression(&lp->filter);
    if (msLayerSetTimeFilter(lp, pszTimeValue, pszTimeField) == MS_TRUE &&
        lp->filter.string) {
      expr = lp->filter.string;
    }
    msCopyExpression(&lp->filter, &old_filter); /* restore old filter */
    msFreeExpression(&old_filter);
  }
  return expr;
}

char *FLTGetCommonExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
  char *pszExpression = NULL;

  if (!psFilterNode)
    return NULL;

  if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON) {
    if ( psFilterNode->psLeftNode && psFilterNode->psRightNode) {
      if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
        pszExpression = msStrdup(FLTGetBinaryComparisonCommonExpression(psFilterNode, lp).c_str());
      else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
        pszExpression = msStrdup(FLTGetIsLikeComparisonCommonExpression(psFilterNode).c_str());
      else if (strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0)
        pszExpression = msStrdup(FLTGetIsBetweenComparisonCommonExpresssion(psFilterNode, lp).c_str());
    }
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL) {
    pszExpression = msStrdup(FLTGetLogicalComparisonCommonExpression(psFilterNode, lp).c_str());
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL) {
    pszExpression = msStrdup(FLTGetSpatialComparisonCommonExpression(psFilterNode, lp).c_str());
  } else if (psFilterNode->eType ==  FILTER_NODE_TYPE_FEATUREID) {
    pszExpression = msStrdup(FLTGetFeatureIdCommonExpression(psFilterNode, lp).c_str());
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_TEMPORAL) {
    pszExpression = msStrdup(FLTGetTimeExpression(psFilterNode, lp).c_str());
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

  const int save_startindex = map->query.startindex;
  const int save_maxfeatures = map->query.maxfeatures;
  const int save_only_cache_result_count = map->query.only_cache_result_count;
  const int save_cache_shapes = map->query.cache_shapes;
  const int save_max_cached_shape_count = map->query.max_cached_shape_count;
  const int save_max_cached_shape_ram_amount = map->query.max_cached_shape_ram_amount;
  msInitQuery(&(map->query));
  map->query.startindex = save_startindex;
  map->query.maxfeatures = save_maxfeatures;
  map->query.only_cache_result_count = save_only_cache_result_count;
  map->query.cache_shapes = save_cache_shapes;
  map->query.max_cached_shape_count = save_max_cached_shape_count;
  map->query.max_cached_shape_ram_amount = save_max_cached_shape_ram_amount;

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

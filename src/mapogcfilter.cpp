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

#include "mapserver-config.h"

#include "cpl_minixml.h"
#include "cpl_string.h"

#include "mapogcfilter.h"
#include "mapserver.h"
#include "mapowscommon.h"
#include "maptime.h"
#include "mapows.h"
#include <ctype.h>

#if 0
static int FLTHasUniqueTopLevelDuringFilter(FilterEncodingNode *psFilterNode);
#endif

#if !(defined(_WIN32) && !defined(__CYGWIN__))
static inline void IGUR_double(double ignored) {
  (void)ignored;
} /* Ignore GCC Unused Result */
#endif

int FLTIsNumeric(const char *pszValue) {
  if (pszValue != NULL && *pszValue != '\0' && !isspace(*pszValue)) {
    /*the regex seems to have a problem on windows when mapserver is built using
      PHP regex*/
#if defined(_WIN32) && !defined(__CYGWIN__)
    int i = 0, nLength = 0, bString = 0;

    nLength = strlen(pszValue);
    for (i = 0; i < nLength; i++) {
      if (i == 0) {
        if (!isdigit(pszValue[i]) && pszValue[i] != '-') {
          bString = 1;
          break;
        }
      } else if (!isdigit(pszValue[i]) && pszValue[i] != '.') {
        bString = 1;
        break;
      }
    }
    if (!bString)
      return MS_TRUE;
#else
    char *p;
    IGUR_double(strtod(pszValue, &p));
    if (p != pszValue && *p == '\0')
      return MS_TRUE;
#endif
  }

  return MS_FALSE;
}

/*
** Apply an expression to the layer's filter element.
**
*/
int FLTApplyExpressionToLayer(layerObj *lp, const char *pszExpression) {
  char *pszFinalExpression = NULL, *pszBuffer = NULL;
  /*char *escapedTextString=NULL;*/
  int bConcatWhere = 0, bHasAWhere = 0;

  if (lp && pszExpression) {
    if (lp->connectiontype == MS_POSTGIS ||
        lp->connectiontype == MS_ORACLESPATIAL ||
        lp->connectiontype == MS_PLUGIN) {
      pszFinalExpression = msStrdup("(");
      pszFinalExpression =
          msStringConcatenate(pszFinalExpression, pszExpression);
      pszFinalExpression = msStringConcatenate(pszFinalExpression, ")");
    } else if (lp->connectiontype == MS_OGR) {
      pszFinalExpression = msStrdup(pszExpression);
      if (lp->filter.type != MS_EXPRESSION) {
        bConcatWhere = 1;
      } else {
        if (lp->filter.string && EQUALN(lp->filter.string, "WHERE ", 6)) {
          bHasAWhere = 1;
          bConcatWhere = 1;
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
        pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string + 6);
      else
        pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string);
      pszBuffer = msStringConcatenate(pszBuffer, ") and ");
    } else if (lp->filter.string)
      msFreeExpression(&lp->filter);

    pszBuffer = msStringConcatenate(pszBuffer, pszFinalExpression);

    if (lp->filter.string && lp->filter.type == MS_EXPRESSION)
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

char *FLTGetExpressionForValuesRanges(layerObj *lp, const char *item,
                                      const char *value, int forcecharcter) {
  int bIscharacter;
  char *pszExpression = NULL, *pszTmpExpression = NULL;
  char **paszElements = NULL, **papszRangeElements = NULL;
  int numelements, i, nrangeelements;

  /* double minval, maxval; */
  if (lp && item && value) {
    if (strstr(value, "/") == NULL) {
      /*value(s)*/
      paszElements = msStringSplit(value, ',', &numelements);
      if (paszElements && numelements > 0) {
        if (forcecharcter)
          bIscharacter = MS_TRUE;
        else
          bIscharacter = !FLTIsNumeric(paszElements[0]);

        pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
        for (i = 0; i < numelements; i++) {
          pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
          {
            if (bIscharacter)
              pszTmpExpression = msStringConcatenate(pszTmpExpression, "\"");
            pszTmpExpression = msStringConcatenate(pszTmpExpression, "[");
            pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
            pszTmpExpression = msStringConcatenate(pszTmpExpression, "]");
            if (bIscharacter)
              pszTmpExpression = msStringConcatenate(pszTmpExpression, "\"");
          }
          if (bIscharacter) {
            pszTmpExpression = msStringConcatenate(pszTmpExpression, " = \"");
          } else
            pszTmpExpression = msStringConcatenate(pszTmpExpression, " = ");

          {
            char *pszEscapedStr = msLayerEscapeSQLParam(lp, paszElements[i]);
            pszTmpExpression =
                msStringConcatenate(pszTmpExpression, pszEscapedStr);
            msFree(pszEscapedStr);
          }

          if (bIscharacter) {
            pszTmpExpression = msStringConcatenate(pszTmpExpression, "\"");
          }
          pszTmpExpression = msStringConcatenate(pszTmpExpression, ")");

          if (pszExpression != NULL)
            pszExpression = msStringConcatenate(pszExpression, " OR ");

          pszExpression = msStringConcatenate(pszExpression, pszTmpExpression);

          msFree(pszTmpExpression);
          pszTmpExpression = NULL;
        }
        pszExpression = msStringConcatenate(pszExpression, ")");
      }
      msFreeCharArray(paszElements, numelements);
    } else {
      /*range(s)*/
      paszElements = msStringSplit(value, ',', &numelements);
      if (paszElements && numelements > 0) {
        pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
        for (i = 0; i < numelements; i++) {
          papszRangeElements =
              msStringSplit(paszElements[i], '/', &nrangeelements);
          if (papszRangeElements && nrangeelements > 0) {
            pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
            if (nrangeelements == 2 || nrangeelements == 3) {
              /*
              minval = atof(papszRangeElements[0]);
              maxval = atof(papszRangeElements[1]);
              */
              {
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "[");
                pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "]");
              }

              pszTmpExpression = msStringConcatenate(pszTmpExpression, " >= ");

              {
                char *pszEscapedStr =
                    msLayerEscapeSQLParam(lp, papszRangeElements[0]);
                pszTmpExpression =
                    msStringConcatenate(pszTmpExpression, pszEscapedStr);
                msFree(pszEscapedStr);
              }

              pszTmpExpression = msStringConcatenate(pszTmpExpression, " AND ");

              {
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "[");
                pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "]");
              }

              pszTmpExpression = msStringConcatenate(pszTmpExpression, " <= ");

              {
                char *pszEscapedStr =
                    msLayerEscapeSQLParam(lp, papszRangeElements[1]);
                pszTmpExpression =
                    msStringConcatenate(pszTmpExpression, pszEscapedStr);
                msFree(pszEscapedStr);
              }

              pszTmpExpression = msStringConcatenate(pszTmpExpression, ")");
            } else if (nrangeelements == 1) {
              pszTmpExpression = msStringConcatenate(pszTmpExpression, "(");
              {
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "[");
                pszTmpExpression = msStringConcatenate(pszTmpExpression, item);
                pszTmpExpression = msStringConcatenate(pszTmpExpression, "]");
              }

              pszTmpExpression = msStringConcatenate(pszTmpExpression, " = ");

              {
                char *pszEscapedStr =
                    msLayerEscapeSQLParam(lp, papszRangeElements[0]);
                pszTmpExpression =
                    msStringConcatenate(pszTmpExpression, pszEscapedStr);
                msFree(pszEscapedStr);
              }

              pszTmpExpression = msStringConcatenate(pszTmpExpression, ")");
            }

            if (pszExpression != NULL)
              pszExpression = msStringConcatenate(pszExpression, " OR ");

            pszExpression =
                msStringConcatenate(pszExpression, pszTmpExpression);
            msFree(pszTmpExpression);
            pszTmpExpression = NULL;
          }
          msFreeCharArray(papszRangeElements, nrangeelements);
        }
        pszExpression = msStringConcatenate(pszExpression, ")");
      }
      msFreeCharArray(paszElements, numelements);
    }
  }
  msFree(pszTmpExpression);
  return pszExpression;
}

static int FLTShapeFromGMLTree(CPLXMLNode *psTree, shapeObj *psShape,
                               char **ppszSRS) {
  const char *pszSRS = NULL;
  if (psTree && psShape) {
    CPLXMLNode *psNext = psTree->psNext;
    OGRGeometryH hGeometry = NULL;

    psTree->psNext = NULL;
    hGeometry = OGR_G_CreateFromGMLTree(psTree);
    psTree->psNext = psNext;

    if (hGeometry) {
      OGRwkbGeometryType nType;
      nType = OGR_G_GetGeometryType(hGeometry);
      if (nType == wkbPolygon25D || nType == wkbMultiPolygon25D)
        nType = wkbPolygon;
      else if (nType == wkbLineString25D || nType == wkbMultiLineString25D)
        nType = wkbLineString;
      else if (nType == wkbPoint25D || nType == wkbMultiPoint25D)
        nType = wkbPoint;
      msOGRGeometryToShape(hGeometry, psShape, nType);

      OGR_G_DestroyGeometry(hGeometry);

      pszSRS = CPLGetXMLValue(psTree, "srsName", NULL);
      if (ppszSRS && pszSRS)
        *ppszSRS = msStrdup(pszSRS);

      return MS_TRUE;
    }
  }

  return MS_FALSE;
}

/************************************************************************/
/*                            FLTSplitFilters                           */
/*                                                                      */
/*    Split filters separated by parentheses into an array of strings.  */
/************************************************************************/
char **FLTSplitFilters(const char *pszStr, int *pnTokens) {
  const char *pszTokenBegin;
  char **papszRet = NULL;
  int nTokens = 0;
  char chStringQuote = '\0';
  int nXMLIndent = 0;
  int bInBracket = FALSE;

  if (*pszStr != '(') {
    *pnTokens = 0;
    return NULL;
  }
  pszStr++;
  pszTokenBegin = pszStr;
  while (*pszStr != '\0') {
    /* Ignore any character until end of quoted string */
    if (chStringQuote != '\0') {
      if (*pszStr == chStringQuote)
        chStringQuote = 0;
    }
    /* Detect begin of quoted string only for an XML attribute, i.e. between <
       and > */
    else if (bInBracket && (*pszStr == '\'' || *pszStr == '"')) {
      chStringQuote = *pszStr;
    }
    /* Begin of XML element */
    else if (*pszStr == '<') {
      bInBracket = TRUE;
      if (pszStr[1] == '/')
        nXMLIndent--;
      else if (pszStr[1] != '!')
        nXMLIndent++;
    }
    /* <something /> case */
    else if (*pszStr == '/' && pszStr[1] == '>') {
      bInBracket = FALSE;
      nXMLIndent--;
      pszStr++;
    }
    /* End of XML element */
    else if (*pszStr == '>') {
      bInBracket = FALSE;
    }
    /* Only detect and of filter when XML indentation goes back to zero */
    else if (nXMLIndent == 0 && *pszStr == ')') {
      papszRet =
          (char **)msSmallRealloc(papszRet, sizeof(char *) * (nTokens + 1));
      papszRet[nTokens] = msStrdup(pszTokenBegin);
      papszRet[nTokens][pszStr - pszTokenBegin] = '\0';
      nTokens++;
      if (pszStr[1] != '(') {
        break;
      }
      pszStr++;
      pszTokenBegin = pszStr + 1;
    }
    pszStr++;
  }
  *pnTokens = nTokens;
  return papszRet;
}

/************************************************************************/
/*                            FLTIsSimpleFilter                         */
/*                                                                      */
/*      Filter encoding with only attribute queries and only one bbox.  */
/************************************************************************/
int FLTIsSimpleFilter(FilterEncodingNode *psNode) {
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
                          int iLayerIndex) {
  layerObj *layer = GET_LAYER(map, iLayerIndex);

  if (!layer->vtable) {
    int rv = msInitializeVirtualTable(layer);
    if (rv != MS_SUCCESS)
      return rv;
  }
  if (!layer->vtable)
    return MS_FAILURE;
  return layer->vtable->LayerApplyFilterToLayer(psNode, map, iLayerIndex);
}

/************************************************************************/
/*               FLTLayerApplyCondSQLFilterToLayer                       */
/*                                                                      */
/* Helper function for layer virtual table architecture                 */
/************************************************************************/
int FLTLayerApplyCondSQLFilterToLayer(FilterEncodingNode *psNode, mapObj *map,
                                      int iLayerIndex) {
  return FLTLayerApplyPlainFilterToLayer(psNode, map, iLayerIndex);
}

/************************************************************************/
/*                           FLTGetTopBBOX                              */
/*                                                                      */
/* Return the "top" BBOX if there's a unique one.                       */
/************************************************************************/
static int FLTGetTopBBOXInternal(FilterEncodingNode *psNode,
                                 FilterEncodingNode **ppsTopBBOX,
                                 int *pnCount) {
  if (psNode->pszValue && strcasecmp(psNode->pszValue, "BBOX") == 0) {
    (*pnCount)++;
    if (*pnCount == 1) {
      *ppsTopBBOX = psNode;
      return TRUE;
    }
    *ppsTopBBOX = NULL;
    return FALSE;
  } else if (psNode->pszValue && strcasecmp(psNode->pszValue, "AND") == 0) {
    return FLTGetTopBBOXInternal(psNode->psLeftNode, ppsTopBBOX, pnCount) &&
           FLTGetTopBBOXInternal(psNode->psRightNode, ppsTopBBOX, pnCount);
  } else {
    return TRUE;
  }
}

static FilterEncodingNode *FLTGetTopBBOX(FilterEncodingNode *psNode) {
  int nCount = 0;
  FilterEncodingNode *psTopBBOX = NULL;
  FLTGetTopBBOXInternal(psNode, &psTopBBOX, &nCount);
  return psTopBBOX;
}

/************************************************************************/
/*                   FLTLayerSetInvalidRectIfSupported                  */
/*                                                                      */
/*  This function will set in *rect a very huge extent if the layer     */
/*  wfs_use_default_extent_for_getfeature metadata item is set to false */
/*  and the layer supports such degenerate rectangle, as a hint that    */
/*  they should not issue a spatial filter.                             */
/************************************************************************/

int FLTLayerSetInvalidRectIfSupported(layerObj *lp, rectObj *rect,
                                      const char *metadata_namespaces) {
  const char *pszUseDefaultExtent =
      msOWSLookupMetadata(&(lp->metadata), metadata_namespaces,
                          "use_default_extent_for_getfeature");
  if (pszUseDefaultExtent && !CSLTestBoolean(pszUseDefaultExtent) &&
      (lp->connectiontype == MS_OGR || lp->connectiontype == MS_POSTGIS ||
       ((lp->connectiontype == MS_PLUGIN) &&
        (strstr(lp->plugin_library, "msplugin_mssql2008") != NULL)))) {
    const rectObj rectInvalid = MS_INIT_INVALID_RECT;
    *rect = rectInvalid;
    return MS_TRUE;
  }
  return MS_FALSE;
}

/************************************************************************/
/*                   FLTLayerApplyPlainFilterToLayer                    */
/*                                                                      */
/* Helper function for layer virtual table architecture                 */
/************************************************************************/
int FLTLayerApplyPlainFilterToLayer(FilterEncodingNode *psNode, mapObj *map,
                                    int iLayerIndex) {
  char *pszExpression = NULL;
  int status = MS_FALSE;
  layerObj *lp = GET_LAYER(map, iLayerIndex);

  pszExpression = FLTGetCommonExpression(psNode, lp);
  if (pszExpression) {
    FilterEncodingNode *psTopBBOX;
    rectObj rect = map->extent;

    FLTLayerSetInvalidRectIfSupported(lp, &rect, "OF");

    psTopBBOX = FLTGetTopBBOX(psNode);
    if (psTopBBOX) {
      int can_remove_expression = MS_TRUE;
      const char *pszEPSG = FLTGetBBOX(psNode, &rect);
      if (pszEPSG && map->projection.numargs > 0) {
        projectionObj sProjTmp;
        msInitProjection(&sProjTmp);
        msProjectionInheritContextFrom(&sProjTmp, &map->projection);
        /* Use the non EPSG variant since axis swapping is done in
         * FLTDoAxisSwappingIfNecessary */
        if (msLoadProjectionString(&sProjTmp, pszEPSG) == 0) {
          rectObj oldRect = rect;
          msProjectRect(&sProjTmp, &map->projection, &rect);
          /* If reprojection is involved, do not remove the expression */
          if (rect.minx != oldRect.minx || rect.miny != oldRect.miny ||
              rect.maxx != oldRect.maxx || rect.maxy != oldRect.maxy) {
            can_remove_expression = MS_FALSE;
          }
        }
        msFreeProjection(&sProjTmp);
      }

      /* Small optimization: if the query is just a BBOX, then do a */
      /* msQueryByRect() */
      if (psTopBBOX == psNode && can_remove_expression) {
        msFree(pszExpression);
        pszExpression = NULL;
      }
    }

    if (map->debug == MS_DEBUGLEVEL_VVV) {
      if (pszExpression)
        msDebug("FLTLayerApplyPlainFilterToLayer(): %s, "
                "rect=%.15g,%.15g,%.15g,%.15g\n",
                pszExpression, rect.minx, rect.miny, rect.maxx, rect.maxy);
      else
        msDebug(
            "FLTLayerApplyPlainFilterToLayer(): rect=%.15g,%.15g,%.15g,%.15g\n",
            rect.minx, rect.miny, rect.maxx, rect.maxy);
    }

    status = FLTApplyFilterToLayerCommonExpressionWithRect(map, iLayerIndex,
                                                           pszExpression, rect);
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
/*      unsuccessful.                                                  */
/*      Calling function should use FreeFilterEncodingNode function     */
/*      to free memory.                                                */
/************************************************************************/
FilterEncodingNode *FLTParseFilterEncoding(const char *szXMLString) {
  CPLXMLNode *psRoot = NULL, *psChild = NULL, *psFilter = NULL;
  FilterEncodingNode *psFilterNode = NULL;

  if (szXMLString == NULL || strlen(szXMLString) == 0 ||
      (strstr(szXMLString, "Filter") == NULL))
    return NULL;

  psRoot = CPLParseXMLString(szXMLString);

  if (psRoot == NULL)
    return NULL;

  /* strip namespaces. We srtip all name spaces (#1350)*/
  CPLStripXMLNamespace(psRoot, NULL, 1);

  /* -------------------------------------------------------------------- */
  /*      get the root element (Filter).                                  */
  /* -------------------------------------------------------------------- */
  psFilter = CPLGetXMLNode(psRoot, "=Filter");
  if (!psFilter) {
    CPLDestroyXMLNode(psRoot);
    return NULL;
  }

  psChild = psFilter->psChild;
  while (psChild) {
    if (FLTIsSupportedFilterType(psChild)) {
      psFilterNode = FLTCreateFilterEncodingNode();
      FLTInsertElementInNode(psFilterNode, psChild);
      break;
    } else
      psChild = psChild->psNext;
  }

  CPLDestroyXMLNode(psRoot);

  /* -------------------------------------------------------------------- */
  /*      validate the node tree to make sure that all the nodes are valid.*/
  /* -------------------------------------------------------------------- */
  if (!FLTValidFilterNode(psFilterNode)) {
    FLTFreeFilterEncodingNode(psFilterNode);
    return NULL;
  }

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
int FLTValidFilterNode(FilterEncodingNode *psFilterNode) {
  if (!psFilterNode)
    return 0;

  if (psFilterNode->eType == FILTER_NODE_TYPE_UNDEFINED)
    return 0;

  if (psFilterNode->psLeftNode) {
    const int bReturn = FLTValidFilterNode(psFilterNode->psLeftNode);
    if (bReturn == 0)
      return 0;
    else if (psFilterNode->psRightNode)
      return FLTValidFilterNode(psFilterNode->psRightNode);
  }

  return 1;
}

/************************************************************************/
/*                       FLTIsGeometryFilterNodeType                    */
/************************************************************************/

static int FLTIsGeometryFilterNodeType(int eType) {
  return (eType == FILTER_NODE_TYPE_GEOMETRY_POINT ||
          eType == FILTER_NODE_TYPE_GEOMETRY_LINE ||
          eType == FILTER_NODE_TYPE_GEOMETRY_POLYGON);
}

/************************************************************************/
/*                          FLTFreeFilterEncodingNode                   */
/*                                                                      */
/*      recursive freeing of Filter Encoding nodes.                      */
/************************************************************************/
void FLTFreeFilterEncodingNode(FilterEncodingNode *psFilterNode) {
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
      free(psFilterNode->pszSRS);

    if (psFilterNode->pOther) {
      if (psFilterNode->pszValue != NULL &&
          strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0) {
        FEPropertyIsLike *propIsLike = (FEPropertyIsLike *)psFilterNode->pOther;
        if (propIsLike->pszWildCard)
          free(propIsLike->pszWildCard);
        if (propIsLike->pszSingleChar)
          free(propIsLike->pszSingleChar);
        if (propIsLike->pszEscapeChar)
          free(propIsLike->pszEscapeChar);
      } else if (FLTIsGeometryFilterNodeType(psFilterNode->eType)) {
        msFreeShape((shapeObj *)(psFilterNode->pOther));
      }
      /* else */
      /* TODO free pOther special fields */
      free(psFilterNode->pOther);
    }

    /* Cannot free pszValue before, 'cause we are testing it above */
    if (psFilterNode->pszValue)
      free(psFilterNode->pszValue);

    free(psFilterNode);
  }
}

/************************************************************************/
/*                         FLTCreateFilterEncodingNode                  */
/*                                                                      */
/*      return a FilterEncoding node.                                    */
/************************************************************************/
FilterEncodingNode *FLTCreateFilterEncodingNode(void) {
  FilterEncodingNode *psFilterNode = NULL;

  psFilterNode = (FilterEncodingNode *)malloc(sizeof(FilterEncodingNode));
  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
  psFilterNode->pszValue = NULL;
  psFilterNode->pOther = NULL;
  psFilterNode->pszSRS = NULL;
  psFilterNode->psLeftNode = NULL;
  psFilterNode->psRightNode = NULL;

  return psFilterNode;
}

FilterEncodingNode *FLTCreateBinaryCompFilterEncodingNode(void) {
  FilterEncodingNode *psFilterNode = NULL;

  psFilterNode = FLTCreateFilterEncodingNode();
  /* used to store case sensitivity flag. Default is 0 meaning the
     comparing is case sensititive */
  psFilterNode->pOther = (int *)malloc(sizeof(int));
  (*(int *)(psFilterNode->pOther)) = 0;

  return psFilterNode;
}

/************************************************************************/
/*                           FLTFindGeometryNode                        */
/*                                                                      */
/************************************************************************/

static CPLXMLNode *FLTFindGeometryNode(CPLXMLNode *psXMLNode, int *pbPoint,
                                       int *pbLine, int *pbPolygon) {
  CPLXMLNode *psGMLElement = NULL;

  psGMLElement = CPLGetXMLNode(psXMLNode, "Point");
  if (!psGMLElement)
    psGMLElement = CPLGetXMLNode(psXMLNode, "PointType");
  if (psGMLElement)
    *pbPoint = 1;
  else {
    psGMLElement = CPLGetXMLNode(psXMLNode, "Polygon");
    if (psGMLElement)
      *pbPolygon = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "MultiPolygon")))
      *pbPolygon = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "Surface")))
      *pbPolygon = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "MultiSurface")))
      *pbPolygon = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "Box")))
      *pbPolygon = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "Envelope")))
      *pbPolygon = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "LineString")))
      *pbLine = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "MultiLineString")))
      *pbLine = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "Curve")))
      *pbLine = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "MultiCurve")))
      *pbLine = 1;
    else if ((psGMLElement = CPLGetXMLNode(psXMLNode, "MultiPoint")))
      *pbPoint = 1;
  }
  return psGMLElement;
}

/************************************************************************/
/*                           FLTGetPropertyName                         */
/************************************************************************/
static const char *FLTGetPropertyName(CPLXMLNode *psXMLNode) {
  const char *pszPropertyName;

  pszPropertyName = CPLGetXMLValue(psXMLNode, "PropertyName", NULL);
  if (pszPropertyName == NULL) /* FE 2.0 ? */
    pszPropertyName = CPLGetXMLValue(psXMLNode, "ValueReference", NULL);
  return pszPropertyName;
}

/************************************************************************/
/*                          FLTGetFirstChildNode                        */
/************************************************************************/
static CPLXMLNode *FLTGetFirstChildNode(CPLXMLNode *psXMLNode) {
  if (psXMLNode == NULL)
    return NULL;
  psXMLNode = psXMLNode->psChild;
  while (psXMLNode != NULL) {
    if (psXMLNode->eType == CXT_Element)
      return psXMLNode;
    psXMLNode = psXMLNode->psNext;
  }
  return NULL;
}

/************************************************************************/
/*                        FLTGetNextSibblingNode                        */
/************************************************************************/
static CPLXMLNode *FLTGetNextSibblingNode(CPLXMLNode *psXMLNode) {
  if (psXMLNode == NULL)
    return NULL;
  psXMLNode = psXMLNode->psNext;
  while (psXMLNode != NULL) {
    if (psXMLNode->eType == CXT_Element)
      return psXMLNode;
    psXMLNode = psXMLNode->psNext;
  }
  return NULL;
}

/************************************************************************/
/*                           FLTInsertElementInNode                     */
/*                                                                      */
/*      Utility function to parse an XML node and transfer the          */
/*      contents into the Filter Encoding node structure.               */
/************************************************************************/
void FLTInsertElementInNode(FilterEncodingNode *psFilterNode,
                            CPLXMLNode *psXMLNode) {
  char *pszTmp = NULL;
  FilterEncodingNode *psCurFilNode = NULL;
  CPLXMLNode *psCurXMLNode = NULL;
  CPLXMLNode *psTmpNode = NULL;
  CPLXMLNode *psFeatureIdNode = NULL;
  const char *pszFeatureId = NULL;
  char *pszFeatureIdList = NULL;

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
        CPLXMLNode *psFirstNode = FLTGetFirstChildNode(psXMLNode);
        CPLXMLNode *psSecondNode = FLTGetNextSibblingNode(psFirstNode);
        if (psFirstNode && psSecondNode) {
          /*2 operators */
          CPLXMLNode *psNextNode = FLTGetNextSibblingNode(psSecondNode);
          if (psNextNode == NULL) {
            psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
            FLTInsertElementInNode(psFilterNode->psLeftNode, psFirstNode);
            psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
            FLTInsertElementInNode(psFilterNode->psRightNode, psSecondNode);
          } else {
            psCurXMLNode = psFirstNode;
            psCurFilNode = psFilterNode;
            while (psCurXMLNode) {
              psNextNode = FLTGetNextSibblingNode(psCurXMLNode);
              if (FLTGetNextSibblingNode(psNextNode)) {
                psCurFilNode->psLeftNode = FLTCreateFilterEncodingNode();
                FLTInsertElementInNode(psCurFilNode->psLeftNode, psCurXMLNode);
                psCurFilNode->psRightNode = FLTCreateFilterEncodingNode();
                psCurFilNode->psRightNode->eType = FILTER_NODE_TYPE_LOGICAL;
                psCurFilNode->psRightNode->pszValue =
                    msStrdup(psFilterNode->pszValue);

                psCurFilNode = psCurFilNode->psRightNode;
                psCurXMLNode = psNextNode;
              } else { /*last 2 operators*/
                psCurFilNode->psLeftNode = FLTCreateFilterEncodingNode();
                FLTInsertElementInNode(psCurFilNode->psLeftNode, psCurXMLNode);

                psCurFilNode->psRightNode = FLTCreateFilterEncodingNode();
                FLTInsertElementInNode(psCurFilNode->psRightNode, psNextNode);
                break;
              }
            }
          }
        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
      } else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0) {
        CPLXMLNode *psFirstNode = FLTGetFirstChildNode(psXMLNode);
        if (psFirstNode) {
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
          FLTInsertElementInNode(psFilterNode->psLeftNode, psFirstNode);
        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
      } else
        psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
    } /* end if is logical */
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
    /*       type="ogc:BinarySpatialOpType"
       substitutionGroup="ogc:spatialOps"/>*/
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
        const char *pszPropertyName = NULL;
        CPLXMLNode *psBox = NULL, *psEnvelope = NULL;
        rectObj sBox = {0, 0, 0, 0};

        int bCoordinatesValid = 0;

        pszPropertyName = FLTGetPropertyName(psXMLNode);
        psBox = CPLGetXMLNode(psXMLNode, "Box");
        if (!psBox)
          psBox = CPLGetXMLNode(psXMLNode, "BoxType");

        /*FE 1.0 used box FE1.1 uses envelop*/
        if (psBox)
          bCoordinatesValid = FLTParseGMLBox(psBox, &sBox, &pszSRS);
        else if ((psEnvelope = CPLGetXMLNode(psXMLNode, "Envelope")))
          bCoordinatesValid = FLTParseGMLEnvelope(psEnvelope, &sBox, &pszSRS);

        if (bCoordinatesValid) {
          /*set the srs if available*/
          if (pszSRS)
            psFilterNode->pszSRS = pszSRS;

          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
          psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
          /* PropertyName is optional since FE 1.1.0, in which case */
          /* the BBOX must apply to all geometry fields. As we support */
          /* currently only one geometry field, this doesn't make much */
          /* difference to further processing. */
          if (pszPropertyName != NULL) {
            psFilterNode->psLeftNode->pszValue = msStrdup(pszPropertyName);
          }

          /* coordinates */
          psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BBOX;
          psFilterNode->psRightNode->pOther =
              (rectObj *)msSmallMalloc(sizeof(rectObj));
          ((rectObj *)psFilterNode->psRightNode->pOther)->minx = sBox.minx;
          ((rectObj *)psFilterNode->psRightNode->pOther)->miny = sBox.miny;
          ((rectObj *)psFilterNode->psRightNode->pOther)->maxx = sBox.maxx;
          ((rectObj *)psFilterNode->psRightNode->pOther)->maxy = sBox.maxy;
        } else {
          msFree(pszSRS);
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
        }
      } else if (strcasecmp(psXMLNode->pszValue, "DWithin") == 0 ||
                 strcasecmp(psXMLNode->pszValue, "Beyond") == 0)

      {
        shapeObj *psShape = NULL;
        int bPoint = 0, bLine = 0, bPolygon = 0;
        const char *pszUnits = NULL;
        const char *pszDistance = NULL;
        const char *pszPropertyName;
        char *pszSRS = NULL;

        CPLXMLNode *psGMLElement = NULL, *psDistance = NULL;

        pszPropertyName = FLTGetPropertyName(psXMLNode);

        psGMLElement =
            FLTFindGeometryNode(psXMLNode, &bPoint, &bLine, &bPolygon);

        psDistance = CPLGetXMLNode(psXMLNode, "Distance");
        if (psDistance != NULL)
          pszDistance = CPLGetXMLValue(psDistance, NULL, NULL);
        if (pszPropertyName != NULL && psGMLElement && psDistance != NULL) {
          pszUnits = CPLGetXMLValue(psDistance, "units", NULL);
          if (pszUnits == NULL) /* FE 2.0 */
            pszUnits = CPLGetXMLValue(psDistance, "uom", NULL);
          psShape = (shapeObj *)msSmallMalloc(sizeof(shapeObj));
          msInitShape(psShape);
          if (FLTShapeFromGMLTree(psGMLElement, psShape, &pszSRS)) {
            /*set the srs if available*/
            if (pszSRS)
              psFilterNode->pszSRS = pszSRS;

            psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
            psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
            psFilterNode->psLeftNode->pszValue = msStrdup(pszPropertyName);

            psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
            if (bPoint)
              psFilterNode->psRightNode->eType =
                  FILTER_NODE_TYPE_GEOMETRY_POINT;
            else if (bLine)
              psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_LINE;
            else if (bPolygon)
              psFilterNode->psRightNode->eType =
                  FILTER_NODE_TYPE_GEOMETRY_POLYGON;
            psFilterNode->psRightNode->pOther = (shapeObj *)psShape;
            /*the value will be distance;units*/
            psFilterNode->psRightNode->pszValue = msStrdup(pszDistance);
            if (pszUnits) {
              psFilterNode->psRightNode->pszValue =
                  msStringConcatenate(psFilterNode->psRightNode->pszValue, ";");
              psFilterNode->psRightNode->pszValue = msStringConcatenate(
                  psFilterNode->psRightNode->pszValue, pszUnits);
            }
          } else {
            free(psShape);
            msFree(pszSRS);
            psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
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
        int bLine = 0, bPolygon = 0, bPoint = 0;
        char *pszSRS = NULL;
        const char *pszPropertyName;

        CPLXMLNode *psGMLElement = NULL;

        pszPropertyName = FLTGetPropertyName(psXMLNode);

        psGMLElement =
            FLTFindGeometryNode(psXMLNode, &bPoint, &bLine, &bPolygon);

        if (pszPropertyName != NULL && psGMLElement) {
          psShape = (shapeObj *)msSmallMalloc(sizeof(shapeObj));
          msInitShape(psShape);
          if (FLTShapeFromGMLTree(psGMLElement, psShape, &pszSRS)) {
            /*set the srs if available*/
            if (pszSRS)
              psFilterNode->pszSRS = pszSRS;

            psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
            psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
            psFilterNode->psLeftNode->pszValue = msStrdup(pszPropertyName);

            psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
            if (bPoint)
              psFilterNode->psRightNode->eType =
                  FILTER_NODE_TYPE_GEOMETRY_POINT;
            else if (bLine)
              psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_LINE;
            else if (bPolygon)
              psFilterNode->psRightNode->eType =
                  FILTER_NODE_TYPE_GEOMETRY_POLYGON;
            psFilterNode->psRightNode->pOther = (shapeObj *)psShape;

          } else {
            free(psShape);
            msFree(pszSRS);
            psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
          }
        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
      }

    } /* end of is spatial */

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
        const char *pszPropertyName = FLTGetPropertyName(psXMLNode);
        if (pszPropertyName != NULL) {

          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
          psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
          psFilterNode->psLeftNode->pszValue = msStrdup(pszPropertyName);

          psTmpNode = CPLSearchXMLNode(psXMLNode, "Literal");
          if (psTmpNode) {
            const char *pszLiteral = CPLGetXMLValue(psTmpNode, NULL, NULL);

            psFilterNode->psRightNode = FLTCreateBinaryCompFilterEncodingNode();
            psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;

            if (pszLiteral != NULL) {
              const char *pszMatchCase;

              psFilterNode->psRightNode->pszValue = msStrdup(pszLiteral);

              pszMatchCase = CPLGetXMLValue(psXMLNode, "matchCase", NULL);

              /*check if the matchCase attribute is set*/
              if (pszMatchCase != NULL &&
                  strcasecmp(pszMatchCase, "false") == 0) {
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
        if (psFilterNode->psLeftNode == NULL ||
            psFilterNode->psRightNode == NULL)
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
      }

      /* -------------------------------------------------------------------- */
      /*      PropertyIsBetween filter : extract property name and boundary */
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
        const char *pszPropertyName = FLTGetPropertyName(psXMLNode);
        CPLXMLNode *psLowerBoundary = CPLGetXMLNode(psXMLNode, "LowerBoundary");
        CPLXMLNode *psUpperBoundary = CPLGetXMLNode(psXMLNode, "UpperBoundary");
        const char *pszLowerNode = NULL;
        const char *pszUpperNode = NULL;
        if (psLowerBoundary != NULL) {
          /* check if the <Literal> is there */
          if (CPLGetXMLNode(psLowerBoundary, "Literal") != NULL)
            pszLowerNode = CPLGetXMLValue(psLowerBoundary, "Literal", NULL);
          else
            pszLowerNode = CPLGetXMLValue(psLowerBoundary, NULL, NULL);
        }
        if (psUpperBoundary != NULL) {
          if (CPLGetXMLNode(psUpperBoundary, "Literal") != NULL)
            pszUpperNode = CPLGetXMLValue(psUpperBoundary, "Literal", NULL);
          else
            pszUpperNode = CPLGetXMLValue(psUpperBoundary, NULL, NULL);
        }
        if (pszPropertyName != NULL && pszLowerNode != NULL &&
            pszUpperNode != NULL) {
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();

          psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
          psFilterNode->psLeftNode->pszValue = msStrdup(pszPropertyName);

          psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BOUNDARY;

          /* adding a ; between boundary values */
          const int nStrLength =
              strlen(pszLowerNode) + strlen(pszUpperNode) + 2;

          psFilterNode->psRightNode->pszValue =
              (char *)malloc(sizeof(char) * (nStrLength));
          strcpy(psFilterNode->psRightNode->pszValue, pszLowerNode);
          strlcat(psFilterNode->psRightNode->pszValue, ";", nStrLength);
          strlcat(psFilterNode->psRightNode->pszValue, pszUpperNode,
                  nStrLength);

        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;

      } /* end of PropertyIsBetween  */
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
        const char *pszPropertyName = FLTGetPropertyName(psXMLNode);
        const char *pszLiteral = CPLGetXMLValue(psXMLNode, "Literal", NULL);
        const char *pszWildCard = CPLGetXMLValue(psXMLNode, "wildCard", NULL);
        const char *pszSingleChar =
            CPLGetXMLValue(psXMLNode, "singleChar", NULL);
        const char *pszEscapeChar = CPLGetXMLValue(psXMLNode, "escape", NULL);
        if (pszEscapeChar == NULL)
          pszEscapeChar = CPLGetXMLValue(psXMLNode, "escapeChar", NULL);
        if (pszPropertyName != NULL && pszLiteral != NULL &&
            pszWildCard != NULL && pszSingleChar != NULL &&
            pszEscapeChar != NULL) {
          FEPropertyIsLike *propIsLike;

          propIsLike = (FEPropertyIsLike *)malloc(sizeof(FEPropertyIsLike));

          psFilterNode->pOther = propIsLike;
          propIsLike->bCaseInsensitive = 0;
          propIsLike->pszWildCard = msStrdup(pszWildCard);
          propIsLike->pszSingleChar = msStrdup(pszSingleChar);
          propIsLike->pszEscapeChar = msStrdup(pszEscapeChar);

          pszTmp = (char *)CPLGetXMLValue(psXMLNode, "matchCase", NULL);
          if (pszTmp && strcasecmp(pszTmp, "false") == 0) {
            propIsLike->bCaseInsensitive = 1;
          }
          /* --------------------------------------------------------------------
           */
          /*      Create left and right node for the attribute and the value. */
          /* --------------------------------------------------------------------
           */
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();

          psFilterNode->psLeftNode->pszValue = msStrdup(pszPropertyName);
          psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;

          psFilterNode->psRightNode = FLTCreateFilterEncodingNode();

          psFilterNode->psRightNode->pszValue = msStrdup(pszLiteral);

          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;

      }

      else if (strcasecmp(psXMLNode->pszValue, "PropertyIsNull") == 0) {
        const char *pszPropertyName = FLTGetPropertyName(psXMLNode);
        if (pszPropertyName != NULL) {
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
          psFilterNode->psLeftNode->pszValue = msStrdup(pszPropertyName);
          psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
      }

      else if (strcasecmp(psXMLNode->pszValue, "PropertyIsNil") == 0) {
        const char *pszPropertyName = FLTGetPropertyName(psXMLNode);
        if (pszPropertyName != NULL) {
          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
          psFilterNode->psLeftNode->pszValue = msStrdup(pszPropertyName);
          psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
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
    /*      Note that for FES1.1.0 the featureid has been deprecated in     */
    /*      favor of GmlObjectId                                            */
    /*      <GmlObjectId gml:id="TREESA_1M.1234"/>                          */
    /*                                                                      */
    /*      And in FES 2.0, in favor of <fes:ResourceId rid="foo.1234"/>    */
    /* -------------------------------------------------------------------- */
    else if (FLTIsFeatureIdFilterType(psXMLNode->pszValue)) {
      psFilterNode->eType = FILTER_NODE_TYPE_FEATUREID;
      pszFeatureIdList = NULL;

      psFeatureIdNode = psXMLNode;
      while (psFeatureIdNode) {
        pszFeatureId = CPLGetXMLValue(psFeatureIdNode, "fid", NULL);
        if (!pszFeatureId)
          pszFeatureId = CPLGetXMLValue(psFeatureIdNode, "id", NULL);
        if (!pszFeatureId)
          pszFeatureId = CPLGetXMLValue(psFeatureIdNode, "rid", NULL);

        if (pszFeatureId) {
          if (pszFeatureIdList)
            pszFeatureIdList = msStringConcatenate(pszFeatureIdList, ",");

          pszFeatureIdList =
              msStringConcatenate(pszFeatureIdList, pszFeatureId);
        }
        psFeatureIdNode = psFeatureIdNode->psNext;
      }

      if (pszFeatureIdList) {
        msFree(psFilterNode->pszValue);
        psFilterNode->pszValue = msStrdup(pszFeatureIdList);
        msFree(pszFeatureIdList);
      } else
        psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
    }

    /* -------------------------------------------------------------------- */
    /*      Temporal Filter.                                                */
    /*
    <fes:During>
    <fes:ValueReference>gml:TimeInstant</fes:ValueReference>
    <gml:TimePeriod gml:id="TP1">
    <gml:begin>
    <gml:TimeInstant gml:id="TI1">
    <gml:timePosition>2005-05-17T00:00:00Z</gml:timePosition>
    </gml:TimeInstant>
    </gml:begin>
    <gml:end>
    <gml:TimeInstant gml:id="TI2">
    <gml:timePosition>2005-05-23T00:00:00Z</gml:timePosition>
    </gml:TimeInstant>
    </gml:end>
    </gml:TimePeriod>
    </fes:During>
    */
    /* -------------------------------------------------------------------- */
    else if (FLTIsTemporalFilterType(psXMLNode->pszValue)) {
      psFilterNode->eType = FILTER_NODE_TYPE_TEMPORAL;

      if (strcasecmp(psXMLNode->pszValue, "During") == 0) {
        const char *pszPropertyName = NULL;
        const char *pszBeginTime;
        const char *pszEndTime;

        pszPropertyName = FLTGetPropertyName(psXMLNode);
        pszBeginTime = CPLGetXMLValue(
            psXMLNode, "TimePeriod.begin.TimeInstant.timePosition", NULL);
        if (pszBeginTime == NULL)
          pszBeginTime =
              CPLGetXMLValue(psXMLNode, "TimePeriod.beginPosition", NULL);
        pszEndTime = CPLGetXMLValue(
            psXMLNode, "TimePeriod.end.TimeInstant.timePosition", NULL);
        if (pszEndTime == NULL)
          pszEndTime =
              CPLGetXMLValue(psXMLNode, "TimePeriod.endPosition", NULL);

        if (pszPropertyName && pszBeginTime && pszEndTime &&
            strchr(pszBeginTime, '\'') == NULL &&
            strchr(pszBeginTime, '\\') == NULL &&
            strchr(pszEndTime, '\'') == NULL &&
            strchr(pszEndTime, '\\') == NULL &&
            msTimeGetResolution(pszBeginTime) >= 0 &&
            msTimeGetResolution(pszEndTime) >= 0) {

          psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
          psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
          psFilterNode->psLeftNode->pszValue = msStrdup(pszPropertyName);

          psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_TIME_PERIOD;
          const size_t nSize = strlen(pszBeginTime) + strlen(pszEndTime) + 2;
          psFilterNode->psRightNode->pszValue =
              static_cast<char *>(msSmallMalloc(nSize));
          snprintf(psFilterNode->psRightNode->pszValue, nSize, "%s/%s",
                   pszBeginTime, pszEndTime);
        } else
          psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
      } else {
        psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
      }

    } /* end of is temporal */
  }
}

/************************************************************************/
/*            int FLTIsLogicalFilterType((char *pszValue)                  */
/*                                                                      */
/*      return TRUE if the value of the node is of logical filter       */
/*       encoding type.                                                 */
/************************************************************************/
int FLTIsLogicalFilterType(const char *pszValue) {
  if (pszValue) {
    if (strcasecmp(pszValue, "AND") == 0 || strcasecmp(pszValue, "OR") == 0 ||
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
int FLTIsBinaryComparisonFilterType(const char *pszValue) {
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
int FLTIsComparisonFilterType(const char *pszValue) {
  if (pszValue) {
    if (FLTIsBinaryComparisonFilterType(pszValue) ||
        strcasecmp(pszValue, "PropertyIsLike") == 0 ||
        strcasecmp(pszValue, "PropertyIsBetween") == 0 ||
        strcasecmp(pszValue, "PropertyIsNull") == 0 ||
        strcasecmp(pszValue, "PropertyIsNil") == 0)
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
int FLTIsFeatureIdFilterType(const char *pszValue) {
  if (pszValue && (strcasecmp(pszValue, "FeatureId") == 0 ||
                   strcasecmp(pszValue, "GmlObjectId") == 0 ||
                   strcasecmp(pszValue, "ResourceId") == 0))

    return MS_TRUE;

  return MS_FALSE;
}

/************************************************************************/
/*            int FLTIsSpatialFilterType(char *pszValue)                */
/*                                                                      */
/*      return TRUE if the value of the node is of spatial filter       */
/*      encoding type.                                                  */
/************************************************************************/
int FLTIsSpatialFilterType(const char *pszValue) {
  if (pszValue) {
    if (strcasecmp(pszValue, "BBOX") == 0 ||
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
/*            int FLTIsTemportalFilterType(char *pszValue)              */
/*                                                                      */
/*      return TRUE if the value of the node is of temporal filter      */
/*      encoding type.                                                  */
/************************************************************************/
int FLTIsTemporalFilterType(const char *pszValue) {
  if (pszValue) {
    if (strcasecmp(pszValue, "During") == 0)
      return MS_TRUE;
  }

  return MS_FALSE;
}

/************************************************************************/
/*           int FLTIsSupportedFilterType(CPLXMLNode *psXMLNode)           */
/*                                                                      */
/*      Verify if the value of the node is one of the supported        */
/*      filter type.                                                    */
/************************************************************************/
int FLTIsSupportedFilterType(CPLXMLNode *psXMLNode) {
  if (psXMLNode) {
    if (FLTIsLogicalFilterType(psXMLNode->pszValue) ||
        FLTIsSpatialFilterType(psXMLNode->pszValue) ||
        FLTIsComparisonFilterType(psXMLNode->pszValue) ||
        FLTIsFeatureIdFilterType(psXMLNode->pszValue) ||
        FLTIsTemporalFilterType(psXMLNode->pszValue))
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
int FLTNumberOfFilterType(FilterEncodingNode *psFilterNode,
                          const char *szType) {
  int nCount = 0;
  int nLeftNode = 0, nRightNode = 0;

  if (!psFilterNode || !szType || !psFilterNode->pszValue)
    return 0;

  if (strcasecmp(psFilterNode->pszValue, (char *)szType) == 0)
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
/*      Validate if there is only one BBOX filter node. Here is what    */
/*      is supported (is valid) :                                       */
/*        - one node which is a BBOX                                    */
/*        - a logical AND with a valid BBOX                             */
/*                                                                      */
/*      eg 1: <Filter>                                                  */
/*            <BBOX>                                                    */
/*              <PropertyName>Geometry</PropertyName>                   */
/*              <gml:Box
 * srsName="http://www.opengis.net/gml/srs/epsg.xml#4326">*/
/*                <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*              </gml:Box>                                              */
/*            </BBOX>                                                   */
/*          </Filter>                                                   */
/*                                                                      */
/*      eg 2 :<Filter>                                                  */
/*              <AND>                                                   */
/*               <BBOX>                                                 */
/*                <PropertyName>Geometry</PropertyName>                  */
/*                <gml:Box
 * srsName="http://www.opengis.net/gml/srs/epsg.xml#4326">*/
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
int FLTValidForBBoxFilter(FilterEncodingNode *psFilterNode) {
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
    return FLTValidForBBoxFilter(psFilterNode->psLeftNode) &&
           FLTValidForBBoxFilter(psFilterNode->psRightNode);
  }

  return 0;
}

#if 0
static int FLTHasUniqueTopLevelDuringFilter(FilterEncodingNode *psFilterNode)
{
  int nCount = 0;

  if (!psFilterNode || !psFilterNode->pszValue)
    return 1;

  nCount = FLTNumberOfFilterType(psFilterNode, "During");

  if (nCount > 1)
    return 0;
  else if (nCount == 0)
    return 1;

  /* nCount ==1  */
  if (strcasecmp(psFilterNode->pszValue, "During") == 0)
    return 1;

  if (strcasecmp(psFilterNode->pszValue, "AND") == 0) {
    return FLTHasUniqueTopLevelDuringFilter(psFilterNode->psLeftNode) &&
           FLTHasUniqueTopLevelDuringFilter(psFilterNode->psRightNode);
  }

  return 0;
}
#endif

int FLTIsLineFilter(FilterEncodingNode *psFilterNode) {
  if (!psFilterNode || !psFilterNode->pszValue)
    return 0;

  if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL &&
      psFilterNode->psRightNode &&
      psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_GEOMETRY_LINE)
    return 1;

  return 0;
}

int FLTIsPolygonFilter(FilterEncodingNode *psFilterNode) {
  if (!psFilterNode || !psFilterNode->pszValue)
    return 0;

  if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL &&
      psFilterNode->psRightNode &&
      psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_GEOMETRY_POLYGON)
    return 1;

  return 0;
}

int FLTIsPointFilter(FilterEncodingNode *psFilterNode) {
  if (!psFilterNode || !psFilterNode->pszValue)
    return 0;

  if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL &&
      psFilterNode->psRightNode &&
      psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_GEOMETRY_POINT)
    return 1;

  return 0;
}

int FLTIsBBoxFilter(FilterEncodingNode *psFilterNode) {
  if (!psFilterNode || !psFilterNode->pszValue)
    return 0;

  if (strcasecmp(psFilterNode->pszValue, "BBOX") == 0)
    return 1;

  return 0;
}

shapeObj *FLTGetShape(FilterEncodingNode *psFilterNode, double *pdfDistance,
                      int *pnUnit) {
  char **tokens = NULL;
  int nTokens = 0;
  FilterEncodingNode *psNode = psFilterNode;
  char *szUnitStr = NULL;
  char *szUnit = NULL;

  if (psNode) {
    if (psNode->eType == FILTER_NODE_TYPE_SPATIAL && psNode->psRightNode)
      psNode = psNode->psRightNode;

    if (FLTIsGeometryFilterNodeType(psNode->eType)) {

      if (psNode->pszValue && pdfDistance) {
        /*
        syntax expected is "distance;unit" or just "distance"
        if unit is there syntax is "URI#unit" (eg http://..../#m)
        or just "unit"
        */
        tokens = msStringSplit(psNode->pszValue, ';', &nTokens);
        if (tokens && nTokens >= 1) {
          *pdfDistance = atof(tokens[0]);

          if (nTokens == 2 && pnUnit) {
            szUnitStr = msStrdup(tokens[1]);
            msFreeCharArray(tokens, nTokens);
            nTokens = 0;
            tokens = msStringSplit(szUnitStr, '#', &nTokens);
            msFree(szUnitStr);
            if (tokens && nTokens >= 1) {
              if (nTokens == 1)
                szUnit = tokens[0];
              else
                szUnit = tokens[1];

              if (strcasecmp(szUnit, "m") == 0 ||
                  strcasecmp(szUnit, "meters") == 0)
                *pnUnit = MS_METERS;
              else if (strcasecmp(szUnit, "km") == 0 ||
                       strcasecmp(szUnit, "kilometers") == 0)
                *pnUnit = MS_KILOMETERS;
              else if (strcasecmp(szUnit, "NM") == 0 ||
                       strcasecmp(szUnit, "nauticalmiles") == 0)
                *pnUnit = MS_NAUTICALMILES;
              else if (strcasecmp(szUnit, "mi") == 0 ||
                       strcasecmp(szUnit, "miles") == 0)
                *pnUnit = MS_MILES;
              else if (strcasecmp(szUnit, "in") == 0 ||
                       strcasecmp(szUnit, "inches") == 0)
                *pnUnit = MS_INCHES;
              else if (strcasecmp(szUnit, "ft") == 0 ||
                       strcasecmp(szUnit, "feet") == 0)
                *pnUnit = MS_FEET;
              else if (strcasecmp(szUnit, "deg") == 0 ||
                       strcasecmp(szUnit, "dd") == 0)
                *pnUnit = MS_DD;
              else if (strcasecmp(szUnit, "px") == 0)
                *pnUnit = MS_PIXELS;
            }
          }
        }
        msFreeCharArray(tokens, nTokens);
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
/*      first bbox node found. The return value is the epsg code of     */
/*      the bbox.                                                       */
/************************************************************************/
const char *FLTGetBBOX(FilterEncodingNode *psFilterNode, rectObj *psRect) {
  const char *pszReturn = NULL;

  if (!psFilterNode || !psRect)
    return NULL;

  if (psFilterNode->pszValue &&
      strcasecmp(psFilterNode->pszValue, "BBOX") == 0) {
    if (psFilterNode->psRightNode && psFilterNode->psRightNode->pOther) {
      rectObj *pRect = (rectObj *)psFilterNode->psRightNode->pOther;
      psRect->minx = pRect->minx;
      psRect->miny = pRect->miny;
      psRect->maxx = pRect->maxx;
      psRect->maxy = pRect->maxy;

      return psFilterNode->pszSRS;
    }
  } else {
    pszReturn = FLTGetBBOX(psFilterNode->psLeftNode, psRect);
    if (pszReturn)
      return pszReturn;
    else
      return FLTGetBBOX(psFilterNode->psRightNode, psRect);
  }

  return pszReturn;
}

const char *FLTGetDuring(FilterEncodingNode *psFilterNode,
                         const char **ppszTimeField) {
  const char *pszReturn = NULL;

  if (!psFilterNode || !ppszTimeField)
    return NULL;

  if (psFilterNode->pszValue &&
      strcasecmp(psFilterNode->pszValue, "During") == 0) {
    *ppszTimeField = psFilterNode->psLeftNode->pszValue;
    return psFilterNode->psRightNode->pszValue;
  } else {
    pszReturn = FLTGetDuring(psFilterNode->psLeftNode, ppszTimeField);
    if (pszReturn)
      return pszReturn;
    else
      return FLTGetDuring(psFilterNode->psRightNode, ppszTimeField);
  }

  return pszReturn;
}

/************************************************************************/
/*                           FLTGetSQLExpression                        */
/*                                                                      */
/*      Build SQL expressions from the mapserver nodes.                 */
/************************************************************************/
char *FLTGetSQLExpression(FilterEncodingNode *psFilterNode, layerObj *lp) {
  char *pszExpression = NULL;

  if (psFilterNode == NULL || lp == NULL)
    return NULL;

  if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON) {
    if (psFilterNode->psLeftNode && psFilterNode->psRightNode) {
      if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue)) {
        pszExpression = FLTGetBinaryComparisonSQLExpresssion(psFilterNode, lp);
      } else if (strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0) {
        pszExpression =
            FLTGetIsBetweenComparisonSQLExpresssion(psFilterNode, lp);
      } else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0) {
        pszExpression = FLTGetIsLikeComparisonSQLExpression(psFilterNode, lp);
      }
    }
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL) {
    if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
        strcasecmp(psFilterNode->pszValue, "OR") == 0) {
      pszExpression = FLTGetLogicalComparisonSQLExpresssion(psFilterNode, lp);

    } else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0) {
      pszExpression = FLTGetLogicalComparisonSQLExpresssion(psFilterNode, lp);
    }
  }

  else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL) {
    /* TODO */
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_FEATUREID) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)
    if (psFilterNode->pszValue) {
      const char *pszAttribute =
          msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
      if (pszAttribute) {
        int nTokens = 0;
        char **tokens = msStringSplit(psFilterNode->pszValue, ',', &nTokens);
        int bString = 0;
        if (tokens && nTokens > 0) {
          for (int i = 0; i < nTokens; i++) {
            char *pszEscapedStr = NULL;
            const char *pszId = tokens[i];
            const char *pszDot = strchr(pszId, '.');
            if (pszDot)
              pszId = pszDot + 1;

            if (strlen(pszId) <= 0)
              continue;

            if (FLTIsNumeric(pszId) == MS_FALSE)
              bString = 1;

            pszEscapedStr = msLayerEscapeSQLParam(lp, pszId);
            char szTmp[256];
            if (bString) {
              if (lp->connectiontype == MS_OGR ||
                  lp->connectiontype == MS_POSTGIS)
                snprintf(szTmp, sizeof(szTmp),
                         "(CAST(%s AS CHARACTER(255)) = '%s')", pszAttribute,
                         pszEscapedStr);
              else
                snprintf(szTmp, sizeof(szTmp), "(%s = '%s')", pszAttribute,
                         pszEscapedStr);
            } else
              snprintf(szTmp, sizeof(szTmp), "(%s = %s)", pszAttribute,
                       pszEscapedStr);

            msFree(pszEscapedStr);
            pszEscapedStr = NULL;

            if (pszExpression != NULL)
              pszExpression = msStringConcatenate(pszExpression, " OR ");
            else
              /*opening and closing brackets*/
              pszExpression = msStringConcatenate(pszExpression, "(");

            pszExpression = msStringConcatenate(pszExpression, szTmp);
          }
        }
        msFreeCharArray(tokens, nTokens);
      }
      /*opening and closing brackets*/
      if (pszExpression)
        pszExpression = msStringConcatenate(pszExpression, ")");
    }
#else
    msSetError(MS_MISCERR, "OWS support is not available.",
               "FLTGetSQLExpression()");
    return NULL;
#endif

  } else if (lp->connectiontype != MS_OGR &&
             psFilterNode->eType == FILTER_NODE_TYPE_TEMPORAL)
    pszExpression = msStrdup(FLTGetTimeExpression(psFilterNode, lp).c_str());

  return pszExpression;
}

/************************************************************************/
/*                     FLTGetLogicalComparisonSQLExpresssion            */
/*                                                                      */
/*      Return the expression for logical comparison expression.        */
/************************************************************************/
char *FLTGetLogicalComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
                                            layerObj *lp) {
  char *pszBuffer = NULL;
  char *pszTmp = NULL;

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

    const size_t nSize = strlen(pszTmp) + 1;
    pszBuffer = (char *)malloc(nSize);
    snprintf(pszBuffer, nSize, "%s", pszTmp);
  }

  /* ==================================================================== */
  /*      special case for temporal filter node (OGR layer only)          */
  /* ==================================================================== */
  else if (lp->connectiontype == MS_OGR && psFilterNode->psLeftNode &&
           psFilterNode->psRightNode &&
           (psFilterNode->psLeftNode->eType == FILTER_NODE_TYPE_TEMPORAL ||
            psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_TEMPORAL)) {
    if (psFilterNode->psLeftNode->eType != FILTER_NODE_TYPE_TEMPORAL)
      pszTmp = FLTGetSQLExpression(psFilterNode->psLeftNode, lp);
    else
      pszTmp = FLTGetSQLExpression(psFilterNode->psRightNode, lp);

    if (!pszTmp)
      return NULL;

    const size_t nSize = strlen(pszTmp) + 1;
    pszBuffer = (char *)malloc(nSize);
    snprintf(pszBuffer, nSize, "%s", pszTmp);
  }

  /* -------------------------------------------------------------------- */
  /*      OR and AND                                                      */
  /* -------------------------------------------------------------------- */
  else if (psFilterNode->psLeftNode && psFilterNode->psRightNode) {
    pszTmp = FLTGetSQLExpression(psFilterNode->psLeftNode, lp);
    if (!pszTmp)
      return NULL;

    pszBuffer = (char *)malloc(
        sizeof(char) * (strlen(pszTmp) + strlen(psFilterNode->pszValue) + 5));
    pszBuffer[0] = '\0';
    strcat(pszBuffer, " (");
    strcat(pszBuffer, pszTmp);
    strcat(pszBuffer, " ");
    strcat(pszBuffer, psFilterNode->pszValue);
    strcat(pszBuffer, " ");

    free(pszTmp);

    const size_t nTmp = strlen(pszBuffer);
    pszTmp = FLTGetSQLExpression(psFilterNode->psRightNode, lp);
    if (!pszTmp) {
      free(pszBuffer);
      return NULL;
    }

    pszBuffer = (char *)msSmallRealloc(
        pszBuffer, sizeof(char) * (strlen(pszTmp) + nTmp + 3));
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

    pszBuffer = (char *)malloc(sizeof(char) * (strlen(pszTmp) + 9));
    pszBuffer[0] = '\0';

    strcat(pszBuffer, " (NOT ");
    strcat(pszBuffer, pszTmp);
    strcat(pszBuffer, ") ");
  } else
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      Cleanup.                                                        */
  /* -------------------------------------------------------------------- */
  if (pszTmp != NULL)
    free(pszTmp);
  return pszBuffer;
}

/************************************************************************/
/*                      FLTGetBinaryComparisonSQLExpresssion            */
/*                                                                      */
/*      Return the expression for a binary comparison filter node.      */
/************************************************************************/
char *FLTGetBinaryComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
                                           layerObj *lp) {
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  int bString = 0;
  char szTmp[256];
  char *pszEscapedStr = NULL;

  szBuffer[0] = '\0';
  if (!psFilterNode || !FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      check if the value is a numeric value or alphanumeric. If it    */
  /*      is alphanumeric, add quotes around attribute and values.        */
  /* -------------------------------------------------------------------- */
  bString = 0;
  if (psFilterNode->psRightNode->pszValue) {
    const char *pszOFGType;
    snprintf(szTmp, sizeof(szTmp), "%s_type",
             psFilterNode->psLeftNode->pszValue);
    pszOFGType = msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp);
    if (pszOFGType != NULL && strcasecmp(pszOFGType, "Character") == 0)
      bString = 1;

    else if (FLTIsNumeric(psFilterNode->psRightNode->pszValue) == MS_FALSE)
      bString = 1;
  }

  /* special case to be able to have empty strings in the expression. */
  if (psFilterNode->psRightNode->pszValue == NULL)
    bString = 1;

  /*opening bracket*/
  strlcat(szBuffer, " (", bufferSize);

  pszEscapedStr =
      msLayerEscapePropertyName(lp, psFilterNode->psLeftNode->pszValue);

  /* attribute */
  /*case insensitive set ? */
  if (bString && strcasecmp(psFilterNode->pszValue, "PropertyIsEqualTo") == 0 &&
      psFilterNode->psRightNode->pOther &&
      (*(int *)psFilterNode->psRightNode->pOther) == 1) {
    snprintf(szTmp, sizeof(szTmp), "lower(%s) ", pszEscapedStr);
    strlcat(szBuffer, szTmp, bufferSize);
  } else
    strlcat(szBuffer, pszEscapedStr, bufferSize);

  msFree(pszEscapedStr);
  pszEscapedStr = NULL;

  /* logical operator */
  if (strcasecmp(psFilterNode->pszValue, "PropertyIsEqualTo") == 0)
    strlcat(szBuffer, "=", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsNotEqualTo") == 0)
    strlcat(szBuffer, "<>", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLessThan") == 0)
    strlcat(szBuffer, "<", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsGreaterThan") == 0)
    strlcat(szBuffer, ">", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLessThanOrEqualTo") ==
           0)
    strlcat(szBuffer, "<=", bufferSize);
  else if (strcasecmp(psFilterNode->pszValue,
                      "PropertyIsGreaterThanOrEqualTo") == 0)
    strlcat(szBuffer, ">=", bufferSize);

  strlcat(szBuffer, " ", bufferSize);

  /* value */

  if (bString && psFilterNode->psRightNode->pszValue &&
      strcasecmp(psFilterNode->pszValue, "PropertyIsEqualTo") == 0 &&
      psFilterNode->psRightNode->pOther &&
      (*(int *)psFilterNode->psRightNode->pOther) == 1) {
    char *pszEscapedStr;
    pszEscapedStr =
        msLayerEscapeSQLParam(lp, psFilterNode->psRightNode->pszValue);
    snprintf(szTmp, sizeof(szTmp), "lower('%s') ", pszEscapedStr);
    msFree(pszEscapedStr);
    strlcat(szBuffer, szTmp, bufferSize);
  } else {
    if (bString)
      strlcat(szBuffer, "'", bufferSize);

    if (psFilterNode->psRightNode->pszValue) {
      if (bString) {
        char *pszEscapedStr;
        pszEscapedStr =
            msLayerEscapeSQLParam(lp, psFilterNode->psRightNode->pszValue);
        strlcat(szBuffer, pszEscapedStr, bufferSize);
        msFree(pszEscapedStr);
        pszEscapedStr = NULL;
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
/*      Build an SQL expression for IsBteween Filter.                  */
/************************************************************************/
char *FLTGetIsBetweenComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
                                              layerObj *lp) {
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  char **aszBounds = NULL;
  int nBounds = 0;
  int bString = 0;
  char szTmp[256];
  char *pszEscapedStr;

  szBuffer[0] = '\0';
  if (!psFilterNode ||
      !(strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0))
    return NULL;

  if (!psFilterNode->psLeftNode || !psFilterNode->psRightNode)
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
    const char *pszOFGType;
    snprintf(szTmp, sizeof(szTmp), "%s_type",
             psFilterNode->psLeftNode->pszValue);
    pszOFGType = msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp);
    if (pszOFGType != NULL && strcasecmp(pszOFGType, "Character") == 0)
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
  /*      build expression.                                              */
  /* -------------------------------------------------------------------- */
  /*opening parenthesis */
  strlcat(szBuffer, " (", bufferSize);

  /* attribute */
  pszEscapedStr =
      msLayerEscapePropertyName(lp, psFilterNode->psLeftNode->pszValue);

  strlcat(szBuffer, pszEscapedStr, bufferSize);
  msFree(pszEscapedStr);
  pszEscapedStr = NULL;

  /*between*/
  strlcat(szBuffer, " BETWEEN ", bufferSize);

  /*bound 1*/
  if (bString)
    strlcat(szBuffer, "'", bufferSize);
  pszEscapedStr = msLayerEscapeSQLParam(lp, aszBounds[0]);
  strlcat(szBuffer, pszEscapedStr, bufferSize);
  msFree(pszEscapedStr);
  pszEscapedStr = NULL;

  if (bString)
    strlcat(szBuffer, "'", bufferSize);

  strlcat(szBuffer, " AND ", bufferSize);

  /*bound 2*/
  if (bString)
    strlcat(szBuffer, "'", bufferSize);
  pszEscapedStr = msLayerEscapeSQLParam(lp, aszBounds[1]);
  strlcat(szBuffer, pszEscapedStr, bufferSize);
  msFree(pszEscapedStr);
  pszEscapedStr = NULL;

  if (bString)
    strlcat(szBuffer, "'", bufferSize);

  /*closing parenthesis*/
  strlcat(szBuffer, ")", bufferSize);

  msFreeCharArray(aszBounds, nBounds);

  return msStrdup(szBuffer);
}

/************************************************************************/
/*                      FLTGetIsLikeComparisonSQLExpression             */
/*                                                                      */
/*      Build an sql expression for IsLike filter.                      */
/************************************************************************/
char *FLTGetIsLikeComparisonSQLExpression(FilterEncodingNode *psFilterNode,
                                          layerObj *lp) {
  const size_t bufferSize = 1024;
  char szBuffer[1024];
  char *pszValue = NULL;

  const char *pszWild = NULL;
  const char *pszSingle = NULL;
  const char *pszEscape = NULL;
  char szTmp[4];

  int nLength = 0, i = 0, j = 0;
  int bCaseInsensitive = 0;

  char *pszEscapedStr = NULL;
  FEPropertyIsLike *propIsLike;

  if (!psFilterNode || !psFilterNode->pOther || !psFilterNode->psLeftNode ||
      !psFilterNode->psRightNode || !psFilterNode->psRightNode->pszValue)
    return NULL;

  propIsLike = (FEPropertyIsLike *)psFilterNode->pOther;
  pszWild = propIsLike->pszWildCard;
  pszSingle = propIsLike->pszSingleChar;
  pszEscape = propIsLike->pszEscapeChar;
  bCaseInsensitive = propIsLike->bCaseInsensitive;

  if (!pszWild || strlen(pszWild) == 0 || !pszSingle ||
      strlen(pszSingle) == 0 || !pszEscape || strlen(pszEscape) == 0)
    return NULL;

  if (pszEscape[0] == '\'') {
    /* This might be valid, but the risk of SQL injection is too high */
    /* and the below code is not ready for that */
    /* Someone who does this has clearly suspect intentions ! */
    msSetError(
        MS_MISCERR,
        "Single quote character is not allowed as an escaping character.",
        "FLTGetIsLikeComparisonSQLExpression()");
    return NULL;
  }

  szBuffer[0] = '\0';
  /*opening bracket*/
  strlcat(szBuffer, " (", bufferSize);

  /* attribute name */
  pszEscapedStr =
      msLayerEscapePropertyName(lp, psFilterNode->psLeftNode->pszValue);

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

  pszEscapedStr = (char *)msSmallMalloc(3 * nLength + 1);

  for (i = 0, j = 0; i < nLength; i++) {
    char c = pszValue[i];
    if (c != pszWild[0] && c != pszSingle[0] && c != pszEscape[0]) {
      if (c == '\'') {
        pszEscapedStr[j++] = '\'';
        pszEscapedStr[j++] = '\'';
      } else if (c == '\\') {
        pszEscapedStr[j++] = '\\';
        pszEscapedStr[j++] = '\\';
      } else
        pszEscapedStr[j++] = c;
    } else if (c == pszSingle[0]) {
      pszEscapedStr[j++] = '_';
    } else if (c == pszEscape[0]) {
      pszEscapedStr[j++] = pszEscape[0];
      if (i + 1 < nLength) {
        char nextC = pszValue[i + 1];
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
    if (lp->connectiontype == MS_POSTGIS && pszEscape[0] == '\\')
      strlcat(szBuffer, " escape E'", bufferSize);
    else
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

    strlcat(szBuffer, szTmp, bufferSize);
  }
  strlcat(szBuffer, ") ", bufferSize);

  return msStrdup(szBuffer);
}

/************************************************************************/
/*                           FLTHasSpatialFilter                        */
/*                                                                      */
/*      Utility function to see if a spatial filter is included in      */
/*      the node.                                                       */
/************************************************************************/
int FLTHasSpatialFilter(FilterEncodingNode *psNode) {
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
FilterEncodingNode *FLTCreateFeatureIdFilterEncoding(const char *pszString) {
  FilterEncodingNode *psFilterNode = NULL;

  if (pszString) {
    psFilterNode = FLTCreateFilterEncodingNode();
    psFilterNode->eType = FILTER_NODE_TYPE_FEATUREID;
    psFilterNode->pszValue = msStrdup(pszString);
    return psFilterNode;
  }
  return NULL;
}

/************************************************************************/
/*                              FLTParseGMLBox                          */
/*                                                                      */
/*      Parse gml box. Used for FE 1.0                                  */
/************************************************************************/
int FLTParseGMLBox(CPLXMLNode *psBox, rectObj *psBbox, char **ppszSRS) {
  int bCoordinatesValid = 0;
  CPLXMLNode *psCoordinates = NULL;
  CPLXMLNode *psCoord1 = NULL, *psCoord2 = NULL;
  char **papszCoords = NULL, **papszMin = NULL, **papszMax = NULL;
  int nCoords = 0, nCoordsMin = 0, nCoordsMax = 0;
  const char *pszTmpCoord = NULL;
  const char *pszSRS = NULL;
  const char *pszTS = NULL;
  const char *pszCS = NULL;
  double minx = 0.0, miny = 0.0, maxx = 0.0, maxy = 0.0;

  if (psBox) {
    pszSRS = CPLGetXMLValue(psBox, "srsName", NULL);
    if (ppszSRS && pszSRS)
      *ppszSRS = msStrdup(pszSRS);

    psCoordinates = CPLGetXMLNode(psBox, "coordinates");
    pszTS = CPLGetXMLValue(psCoordinates, "ts", NULL);
    if (pszTS == NULL)
      pszTS = " ";
    pszCS = CPLGetXMLValue(psCoordinates, "cs", NULL);
    if (pszCS == NULL)
      pszCS = ",";
    pszTmpCoord = CPLGetXMLValue(psCoordinates, NULL, NULL);

    if (pszTmpCoord) {
      papszCoords = msStringSplit(pszTmpCoord, pszTS[0], &nCoords);
      if (papszCoords && nCoords == 2) {
        papszMin = msStringSplit(papszCoords[0], pszCS[0], &nCoordsMin);
        if (papszMin && nCoordsMin == 2) {
          papszMax = msStringSplit(papszCoords[1], pszCS[0], &nCoordsMax);
        }
        if (papszMax && nCoordsMax == 2) {
          bCoordinatesValid = 1;
          minx = atof(papszMin[0]);
          miny = atof(papszMin[1]);
          maxx = atof(papszMax[0]);
          maxy = atof(papszMax[1]);
        }

        msFreeCharArray(papszMin, nCoordsMin);
        msFreeCharArray(papszMax, nCoordsMax);
      }

      msFreeCharArray(papszCoords, nCoords);
    } else {
      psCoord1 = CPLGetXMLNode(psBox, "coord");
      psCoord2 = FLTGetNextSibblingNode(psCoord1);
      if (psCoord1 && psCoord2 && strcmp(psCoord2->pszValue, "coord") == 0) {
        const char *pszX = CPLGetXMLValue(psCoord1, "X", NULL);
        const char *pszY = CPLGetXMLValue(psCoord1, "Y", NULL);
        if (pszX && pszY) {
          minx = atof(pszX);
          miny = atof(pszY);

          pszX = CPLGetXMLValue(psCoord2, "X", NULL);
          pszY = CPLGetXMLValue(psCoord2, "Y", NULL);
          if (pszX && pszY) {
            maxx = atof(pszX);
            maxy = atof(pszY);
            bCoordinatesValid = 1;
          }
        }
      }
    }
  }

  if (bCoordinatesValid) {
    psBbox->minx = minx;
    psBbox->miny = miny;

    psBbox->maxx = maxx;
    psBbox->maxy = maxy;
  }

  return bCoordinatesValid;
}
/************************************************************************/
/*                           FLTParseGMLEnvelope                        */
/*                                                                      */
/*      Utility function to parse a gml:Envelope (used for SOS and FE1.1)*/
/************************************************************************/
int FLTParseGMLEnvelope(CPLXMLNode *psRoot, rectObj *psBbox, char **ppszSRS) {
  CPLXMLNode *psUpperCorner = NULL, *psLowerCorner = NULL;
  const char *pszLowerCorner = NULL, *pszUpperCorner = NULL;
  int bValid = 0;
  char **tokens;
  int n;

  if (psRoot && psBbox && psRoot->eType == CXT_Element &&
      EQUAL(psRoot->pszValue, "Envelope")) {
    /*Get the srs if available*/
    if (ppszSRS) {
      const char *pszSRS = CPLGetXMLValue(psRoot, "srsName", NULL);
      if (pszSRS != NULL)
        *ppszSRS = msStrdup(pszSRS);
    }
    psLowerCorner = CPLSearchXMLNode(psRoot, "lowerCorner");
    psUpperCorner = CPLSearchXMLNode(psRoot, "upperCorner");

    if (psLowerCorner && psUpperCorner) {
      pszLowerCorner = CPLGetXMLValue(psLowerCorner, NULL, NULL);
      pszUpperCorner = CPLGetXMLValue(psUpperCorner, NULL, NULL);

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
            bValid = 1;
          }
        }
        msFreeCharArray(tokens, n);
      }
    }
  }

  return bValid;
}

/************************************************************************/
/*                        FLTNeedSRSSwapping                            */
/************************************************************************/

static int FLTNeedSRSSwapping(mapObj *map, const char *pszSRS) {
  int bNeedSwapping = MS_FALSE;
  projectionObj sProjTmp;
  msInitProjection(&sProjTmp);
  if (map) {
    msProjectionInheritContextFrom(&sProjTmp, &map->projection);
  }
  if (msLoadProjectionStringEPSG(&sProjTmp, pszSRS) == 0) {
    bNeedSwapping = msIsAxisInvertedProj(&sProjTmp);
  }
  msFreeProjection(&sProjTmp);
  return bNeedSwapping;
}

/************************************************************************/
/*                      FLTDoAxisSwappingIfNecessary                    */
/*                                                                      */
/*      Explore all geometries and BBOX to do axis swapping when the    */
/*      SRS requires it. If no explicit SRS is attached to the geometry */
/*      the bDefaultSRSNeedsAxisSwapping is taken into account. The     */
/*      caller will have to determine its value from a more general     */
/*      context.                                                        */
/************************************************************************/
void FLTDoAxisSwappingIfNecessary(mapObj *map, FilterEncodingNode *psFilterNode,
                                  int bDefaultSRSNeedsAxisSwapping) {
  if (psFilterNode == NULL)
    return;

  if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL &&
      psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_BBOX) {
    rectObj *rect = (rectObj *)psFilterNode->psRightNode->pOther;
    const char *pszSRS = psFilterNode->pszSRS;
    if ((pszSRS != NULL && FLTNeedSRSSwapping(map, pszSRS)) ||
        (pszSRS == NULL && bDefaultSRSNeedsAxisSwapping)) {
      double tmp;

      tmp = rect->minx;
      rect->minx = rect->miny;
      rect->miny = tmp;

      tmp = rect->maxx;
      rect->maxx = rect->maxy;
      rect->maxy = tmp;
    }
  } else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL &&
             FLTIsGeometryFilterNodeType(psFilterNode->psRightNode->eType)) {
    shapeObj *shape = (shapeObj *)(psFilterNode->psRightNode->pOther);
    const char *pszSRS = psFilterNode->pszSRS;
    if ((pszSRS != NULL && FLTNeedSRSSwapping(map, pszSRS)) ||
        (pszSRS == NULL && bDefaultSRSNeedsAxisSwapping)) {
      msAxisSwapShape(shape);
    }
  } else {
    FLTDoAxisSwappingIfNecessary(map, psFilterNode->psLeftNode,
                                 bDefaultSRSNeedsAxisSwapping);
    FLTDoAxisSwappingIfNecessary(map, psFilterNode->psRightNode,
                                 bDefaultSRSNeedsAxisSwapping);
  }
}

static void FLTReplacePropertyName(FilterEncodingNode *psFilterNode,
                                   const char *pszOldName,
                                   const char *pszNewName) {
  if (psFilterNode && pszOldName && pszNewName) {
    if (psFilterNode->eType == FILTER_NODE_TYPE_PROPERTYNAME) {
      if (psFilterNode->pszValue &&
          strcasecmp(psFilterNode->pszValue, pszOldName) == 0) {
        msFree(psFilterNode->pszValue);
        psFilterNode->pszValue = msStrdup(pszNewName);
      }
    }
    if (psFilterNode->psLeftNode)
      FLTReplacePropertyName(psFilterNode->psLeftNode, pszOldName, pszNewName);
    if (psFilterNode->psRightNode)
      FLTReplacePropertyName(psFilterNode->psRightNode, pszOldName, pszNewName);
  }
}

static int FLTIsGMLDefaultProperty(const char *pszName) {
  return (strcmp(pszName, "gml:name") == 0 ||
          strcmp(pszName, "gml:description") == 0 ||
          strcmp(pszName, "gml:descriptionReference") == 0 ||
          strcmp(pszName, "gml:identifier") == 0 ||
          strcmp(pszName, "gml:boundedBy") == 0 ||
          strcmp(pszName, "@gml:id") == 0);
}

static void
FLTStripNameSpacesFromPropertyName(FilterEncodingNode *psFilterNode) {
  char **tokens = NULL;
  int n = 0;

  if (psFilterNode) {

    if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON &&
        psFilterNode->psLeftNode != NULL &&
        psFilterNode->psLeftNode->eType == FILTER_NODE_TYPE_PROPERTYNAME &&
        FLTIsGMLDefaultProperty(psFilterNode->psLeftNode->pszValue)) {
      return;
    }

    if (psFilterNode->eType == FILTER_NODE_TYPE_PROPERTYNAME) {
      if (psFilterNode->pszValue && strstr(psFilterNode->pszValue, ":")) {
        tokens = msStringSplit(psFilterNode->pszValue, ':', &n);
        if (tokens && n == 2) {
          msFree(psFilterNode->pszValue);
          psFilterNode->pszValue = msStrdup(tokens[1]);
        }
        msFreeCharArray(tokens, n);
      }
    }
    if (psFilterNode->psLeftNode)
      FLTStripNameSpacesFromPropertyName(psFilterNode->psLeftNode);
    if (psFilterNode->psRightNode)
      FLTStripNameSpacesFromPropertyName(psFilterNode->psRightNode);
  }
}

static void FLTRemoveGroupName(FilterEncodingNode *psFilterNode,
                               gmlGroupListObj *groupList) {
  int i;

  if (psFilterNode) {

    if (psFilterNode->eType == FILTER_NODE_TYPE_PROPERTYNAME) {
      if (psFilterNode->pszValue != NULL) {
        const char *pszPropertyName = psFilterNode->pszValue;
        const char *pszSlash = strchr(pszPropertyName, '/');
        if (pszSlash != NULL) {
          const char *pszColon = strchr(pszPropertyName, ':');
          if (pszColon != NULL && pszColon < pszSlash)
            pszPropertyName = pszColon + 1;
          for (i = 0; i < groupList->numgroups; i++) {
            const char *pszGroupName = groupList->groups[i].name;
            size_t nGroupNameLen = strlen(pszGroupName);
            if (strncasecmp(pszPropertyName, pszGroupName, nGroupNameLen) ==
                    0 &&
                pszPropertyName[nGroupNameLen] == '/') {
              char *pszTmp;
              pszPropertyName = pszPropertyName + nGroupNameLen + 1;
              pszColon = strchr(pszPropertyName, ':');
              if (pszColon != NULL)
                pszPropertyName = pszColon + 1;
              pszTmp = msStrdup(pszPropertyName);
              msFree(psFilterNode->pszValue);
              psFilterNode->pszValue = pszTmp;
              break;
            }
          }
        }
      }
    }

    if (psFilterNode->psLeftNode)
      FLTRemoveGroupName(psFilterNode->psLeftNode, groupList);
    if (psFilterNode->psRightNode)
      FLTRemoveGroupName(psFilterNode->psRightNode, groupList);
  }
}

/************************************************************************/
/*                    FLTPreParseFilterForAliasAndGroup                 */
/*                                                                      */
/*      Utility function to replace aliased' and grouped attributes     */
/*      with their internal name.                                       */
/************************************************************************/
void FLTPreParseFilterForAliasAndGroup(FilterEncodingNode *psFilterNode,
                                       mapObj *map, int i,
                                       const char *namespaces) {
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR)
  if (psFilterNode && map && i >= 0 && i < map->numlayers) {
    /*strip name spaces before hand*/
    FLTStripNameSpacesFromPropertyName(psFilterNode);

    layerObj *lp = GET_LAYER(map, i);
    int layerWasOpened = msLayerIsOpen(lp);
    if (msLayerOpen(lp) == MS_SUCCESS && msLayerGetItems(lp) == MS_SUCCESS) {

      /* Remove group names from property names if using groupname/itemname
       * syntax */
      gmlGroupListObj *groupList = msGMLGetGroups(lp, namespaces);
      if (groupList && groupList->numgroups > 0)
        FLTRemoveGroupName(psFilterNode, groupList);
      msGMLFreeGroups(groupList);

      for (i = 0; i < lp->numitems; i++) {
        if (!lp->items[i] || strlen(lp->items[i]) <= 0)
          continue;
        char szTmp[256];
        snprintf(szTmp, sizeof(szTmp), "%s_alias", lp->items[i]);
        const char *pszFullName =
            msOWSLookupMetadata(&(lp->metadata), namespaces, szTmp);
        if (pszFullName) {
          FLTReplacePropertyName(psFilterNode, pszFullName, lp->items[i]);
        }
      }
      if (!layerWasOpened) /* do not close the layer if it has been opened
                              somewhere else (paging?) */
        msLayerClose(lp);
    }
  }
#else
  msSetError(MS_MISCERR, "OWS support is not available.",
             "FLTPreParseFilterForAlias()");

#endif
}

/************************************************************************/
/*                        FLTCheckFeatureIdFilters                      */
/*                                                                      */
/*      Check that FeatureId filters match features in the active       */
/*      layer.                                                          */
/************************************************************************/
int FLTCheckFeatureIdFilters(FilterEncodingNode *psFilterNode, mapObj *map,
                             int i) {
  int status = MS_SUCCESS;

  if (psFilterNode->eType == FILTER_NODE_TYPE_FEATUREID) {
    char **tokens;
    int nTokens = 0;
    layerObj *lp;
    int j;

    lp = GET_LAYER(map, i);
    tokens = msStringSplit(psFilterNode->pszValue, ',', &nTokens);
    for (j = 0; j < nTokens; j++) {
      const char *pszId = tokens[j];
      const char *pszDot = strrchr(pszId, '.');
      if (pszDot) {
        if (static_cast<size_t>(pszDot - pszId) != strlen(lp->name) ||
            strncasecmp(pszId, lp->name, strlen(lp->name)) != 0) {
          msSetError(MS_MISCERR,
                     "Feature id %s not consistent with feature type name %s.",
                     "FLTPreParseFilterForAlias()", pszId, lp->name);
          status = MS_FAILURE;
          break;
        }
      }
    }
    msFreeCharArray(tokens, nTokens);
  }

  if (psFilterNode->psLeftNode) {
    status = FLTCheckFeatureIdFilters(psFilterNode->psLeftNode, map, i);
    if (status == MS_SUCCESS) {
      if (psFilterNode->psRightNode)
        status = FLTCheckFeatureIdFilters(psFilterNode->psRightNode, map, i);
    }
  }
  return status;
}

/************************************************************************/
/*                        FLTCheckInvalidOperand                        */
/*                                                                      */
/*      Check that the operand of a comparison operator is valid        */
/*      Currently only detects use of boundedBy in a binary comparison  */
/************************************************************************/
int FLTCheckInvalidOperand(FilterEncodingNode *psFilterNode) {
  int status = MS_SUCCESS;

  if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON &&
      psFilterNode->psLeftNode != NULL &&
      psFilterNode->psLeftNode->eType == FILTER_NODE_TYPE_PROPERTYNAME) {
    if (strcmp(psFilterNode->psLeftNode->pszValue, "gml:boundedBy") == 0 &&
        strcmp(psFilterNode->pszValue, "PropertyIsNull") != 0 &&
        strcmp(psFilterNode->pszValue, "PropertyIsNil") != 0) {
      msSetError(MS_MISCERR, "Operand '%s' is invalid in comparison.",
                 "FLTCheckInvalidOperand()",
                 psFilterNode->psLeftNode->pszValue);
      return MS_FAILURE;
    }
  }
  if (psFilterNode->psLeftNode) {
    status = FLTCheckInvalidOperand(psFilterNode->psLeftNode);
    if (status == MS_SUCCESS) {
      if (psFilterNode->psRightNode)
        status = FLTCheckInvalidOperand(psFilterNode->psRightNode);
    }
  }
  return status;
}

/************************************************************************/
/*                       FLTProcessPropertyIsNull                       */
/*                                                                      */
/*      HACK for PropertyIsNull processing. PostGIS & Spatialite only   */
/*      for now.                                                        */
/************************************************************************/
int FLTProcessPropertyIsNull(FilterEncodingNode *psFilterNode, mapObj *map,
                             int i) {
  int status = MS_SUCCESS;

  if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON &&
      psFilterNode->psLeftNode != NULL &&
      psFilterNode->psLeftNode->eType == FILTER_NODE_TYPE_PROPERTYNAME &&
      strcmp(psFilterNode->pszValue, "PropertyIsNull") == 0 &&
      !FLTIsGMLDefaultProperty(psFilterNode->psLeftNode->pszValue)) {
    layerObj *lp;
    int layerWasOpened;

    lp = GET_LAYER(map, i);
    layerWasOpened = msLayerIsOpen(lp);

    /* Horrible HACK to compensate for the lack of null testing in MapServer */
    if (lp->connectiontype == MS_POSTGIS ||
        (lp->connectiontype == MS_OGR && msOGRSupportsIsNull(lp))) {
      msFree(psFilterNode->pszValue);
      psFilterNode->pszValue = msStrdup("PropertyIsEqualTo");
      psFilterNode->psRightNode = FLTCreateBinaryCompFilterEncodingNode();
      psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
      psFilterNode->psRightNode->pszValue = msStrdup("_MAPSERVER_NULL_");
    }

    if (!layerWasOpened) /* do not close the layer if it has been opened
                            somewhere else (paging?) */
      msLayerClose(lp);
  }

  if (psFilterNode->psLeftNode) {
    status = FLTProcessPropertyIsNull(psFilterNode->psLeftNode, map, i);
    if (status == MS_SUCCESS) {
      if (psFilterNode->psRightNode)
        status = FLTProcessPropertyIsNull(psFilterNode->psRightNode, map, i);
    }
  }
  return status;
}

/************************************************************************/
/*                        FLTCheckInvalidProperty                       */
/*                                                                      */
/*      Check that property names are known                             */
/************************************************************************/
int FLTCheckInvalidProperty(FilterEncodingNode *psFilterNode, mapObj *map,
                            int i) {
  int status = MS_SUCCESS;

  if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON &&
      psFilterNode->psLeftNode != NULL &&
      psFilterNode->psLeftNode->eType == FILTER_NODE_TYPE_PROPERTYNAME) {
    layerObj *lp;
    int layerWasOpened;
    int bFound = MS_FALSE;

    if ((strcmp(psFilterNode->pszValue, "PropertyIsNull") == 0 ||
         strcmp(psFilterNode->pszValue, "PropertyIsNil") == 0) &&
        FLTIsGMLDefaultProperty(psFilterNode->psLeftNode->pszValue)) {
      return MS_SUCCESS;
    }

    lp = GET_LAYER(map, i);
    layerWasOpened = msLayerIsOpen(lp);
    if ((layerWasOpened || msLayerOpen(lp) == MS_SUCCESS) &&
        msLayerGetItems(lp) == MS_SUCCESS) {
      int i;
      gmlItemListObj *items = msGMLGetItems(lp, "G");
      for (i = 0; i < items->numitems; i++) {
        if (!items->items[i].name || strlen(items->items[i].name) == 0 ||
            !items->items[i].visible)
          continue;
        if (strcasecmp(items->items[i].name,
                       psFilterNode->psLeftNode->pszValue) == 0) {
          bFound = MS_TRUE;
          break;
        }
      }
      msGMLFreeItems(items);
    }

    if (!layerWasOpened) /* do not close the layer if it has been opened
                            somewhere else (paging?) */
      msLayerClose(lp);

    if (!bFound) {
      msSetError(MS_MISCERR, "Property '%s' is unknown.",
                 "FLTCheckInvalidProperty()",
                 psFilterNode->psLeftNode->pszValue);
      return MS_FAILURE;
    }
  }

  if (psFilterNode->psLeftNode) {
    status = FLTCheckInvalidProperty(psFilterNode->psLeftNode, map, i);
    if (status == MS_SUCCESS) {
      if (psFilterNode->psRightNode)
        status = FLTCheckInvalidProperty(psFilterNode->psRightNode, map, i);
    }
  }
  return status;
}

/************************************************************************/
/*                           FLTSimplify                                */
/*                                                                      */
/*      Simplify the expression by removing parts that evaluate to      */
/*      constants.                                                      */
/*      The passed psFilterNode is potentially consumed by the function */
/*      and replaced by the returned value.                             */
/*      If the function returns NULL, *pnEvaluation = MS_FALSE means    */
/*      that  the filter evaluates to FALSE, or MS_TRUE that it         */
/*      evaluates to TRUE                                               */
/************************************************************************/
FilterEncodingNode *FLTSimplify(FilterEncodingNode *psFilterNode,
                                int *pnEvaluation) {
  *pnEvaluation = -1;

  /* There are no nullable or nillable property in WFS currently */
  /* except gml:name or gml:description that are null */
  if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON &&
      (strcmp(psFilterNode->pszValue, "PropertyIsNull") == 0 ||
       strcmp(psFilterNode->pszValue, "PropertyIsNil") == 0) &&
      psFilterNode->psLeftNode != NULL &&
      psFilterNode->psLeftNode->eType == FILTER_NODE_TYPE_PROPERTYNAME) {
    if (strcmp(psFilterNode->pszValue, "PropertyIsNull") == 0 &&
        FLTIsGMLDefaultProperty(psFilterNode->psLeftNode->pszValue) &&
        strcmp(psFilterNode->psLeftNode->pszValue, "@gml:id") != 0 &&
        strcmp(psFilterNode->psLeftNode->pszValue, "gml:boundedBy") != 0)
      *pnEvaluation = MS_TRUE;
    else
      *pnEvaluation = MS_FALSE;
    FLTFreeFilterEncodingNode(psFilterNode);
    return NULL;
  }

  if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL &&
      strcasecmp(psFilterNode->pszValue, "NOT") == 0 &&
      psFilterNode->psLeftNode != NULL) {
    int nEvaluation;
    psFilterNode->psLeftNode =
        FLTSimplify(psFilterNode->psLeftNode, &nEvaluation);
    if (psFilterNode->psLeftNode == NULL) {
      *pnEvaluation = 1 - nEvaluation;
      FLTFreeFilterEncodingNode(psFilterNode);
      return NULL;
    }
  }

  if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL &&
      (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
       strcasecmp(psFilterNode->pszValue, "OR") == 0) &&
      psFilterNode->psLeftNode != NULL && psFilterNode->psRightNode != NULL) {
    FilterEncodingNode *psOtherNode;
    int nEvaluation;
    int nExpectedValForFastExit;
    psFilterNode->psLeftNode =
        FLTSimplify(psFilterNode->psLeftNode, &nEvaluation);

    if (strcasecmp(psFilterNode->pszValue, "AND") == 0)
      nExpectedValForFastExit = MS_FALSE;
    else
      nExpectedValForFastExit = MS_TRUE;

    if (psFilterNode->psLeftNode == NULL) {
      if (nEvaluation == nExpectedValForFastExit) {
        *pnEvaluation = nEvaluation;
        FLTFreeFilterEncodingNode(psFilterNode);
        return NULL;
      }
      psOtherNode = psFilterNode->psRightNode;
      psFilterNode->psRightNode = NULL;
      FLTFreeFilterEncodingNode(psFilterNode);
      return FLTSimplify(psOtherNode, pnEvaluation);
    }

    psFilterNode->psRightNode =
        FLTSimplify(psFilterNode->psRightNode, &nEvaluation);
    if (psFilterNode->psRightNode == NULL) {
      if (nEvaluation == nExpectedValForFastExit) {
        *pnEvaluation = nEvaluation;
        FLTFreeFilterEncodingNode(psFilterNode);
        return NULL;
      }
      psOtherNode = psFilterNode->psLeftNode;
      psFilterNode->psLeftNode = NULL;
      FLTFreeFilterEncodingNode(psFilterNode);
      return FLTSimplify(psOtherNode, pnEvaluation);
    }
  }

  return psFilterNode;
}

#ifdef USE_LIBXML2

xmlNodePtr FLTGetCapabilities(xmlNsPtr psNsParent, xmlNsPtr psNsOgc,
                              int bTemporal) {
  xmlNodePtr psRootNode = NULL, psNode = NULL, psSubNode = NULL,
             psSubSubNode = NULL;

  psRootNode = xmlNewNode(psNsParent, BAD_CAST "Filter_Capabilities");

  psNode =
      xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Spatial_Capabilities", NULL);

  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "GeometryOperands", NULL);
  xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand",
              BAD_CAST "gml:Point");
  xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand",
              BAD_CAST "gml:LineString");
  xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand",
              BAD_CAST "gml:Polygon");
  xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand",
              BAD_CAST "gml:Envelope");

  psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "SpatialOperators", NULL);
#ifdef USE_GEOS
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Equals");
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Disjoint");
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Touches");
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Within");
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Overlaps");
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Crosses");
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Intersects");
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Contains");
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "DWithin");
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Beyond");
#endif
  psSubSubNode =
      xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
  xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "BBOX");

  if (bTemporal) {
    psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Temporal_Capabilities",
                         NULL);
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "TemporalOperands", NULL);
    xmlNewChild(psSubNode, psNsOgc, BAD_CAST "TemporalOperand",
                BAD_CAST "gml:TimePeriod");
    xmlNewChild(psSubNode, psNsOgc, BAD_CAST "TemporalOperand",
                BAD_CAST "gml:TimeInstant");

    psSubNode =
        xmlNewChild(psNode, psNsOgc, BAD_CAST "TemporalOperators", NULL);
    psSubSubNode =
        xmlNewChild(psSubNode, psNsOgc, BAD_CAST "TemporalOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "TM_Equals");
  }
  psNode =
      xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Scalar_Capabilities", NULL);
  xmlNewChild(psNode, psNsOgc, BAD_CAST "LogicalOperators", NULL);
  psNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperators", NULL);
  xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator",
              BAD_CAST "LessThan");
  xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator",
              BAD_CAST "GreaterThan");
  xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator",
              BAD_CAST "LessThanEqualTo");
  xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator",
              BAD_CAST "GreaterThanEqualTo");
  xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator",
              BAD_CAST "EqualTo");
  xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator",
              BAD_CAST "NotEqualTo");
  xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "Like");
  xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator",
              BAD_CAST "Between");

  psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Id_Capabilities", NULL);
  xmlNewChild(psNode, psNsOgc, BAD_CAST "EID", NULL);
  xmlNewChild(psNode, psNsOgc, BAD_CAST "FID", NULL);
  return psRootNode;
}
#endif

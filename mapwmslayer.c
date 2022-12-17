/*****************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of WMS CONNECTIONTYPE - client to WMS servers
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 *****************************************************************************
 * Copyright (c) 2001-2004, Daniel Morissette, DM Solutions Group Inc
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
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include "mapserver.h"
#include "maperror.h"
#include "mapogcsld.h"
#include "mapows.h"

#include <time.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#include <stdio.h>
#endif

#include "cpl_vsi.h"

/**********************************************************************
 *                          msInitWmsParamsObj()
 *
 **********************************************************************/
int msInitWmsParamsObj(wmsParamsObj *wmsparams)
{
  wmsparams->onlineresource = NULL;
  wmsparams->params = msCreateHashTable();
  wmsparams->numparams=0;
  wmsparams->httpcookiedata = NULL;

  return MS_SUCCESS;
}

/**********************************************************************
 *                          msFreeWmsParamsObj()
 *
 * Frees the contents of the object, but not the object itself.
 **********************************************************************/
void msFreeWmsParamsObj(wmsParamsObj *wmsparams)
{
  msFree(wmsparams->onlineresource);
  wmsparams->onlineresource = NULL;

  msFreeHashTable(wmsparams->params);
  wmsparams->params = NULL;

  msFree(wmsparams->httpcookiedata);

  wmsparams->numparams=0;
}

/**********************************************************************
 *                          msSetWMSParamString()
 *
 **********************************************************************/

#ifdef USE_WMS_LYR
static int msSetWMSParamString(wmsParamsObj *psWMSParams,
                               const char *name, const char * value,
                               int urlencode, int nVersion)
{
  if (urlencode) {
    char *pszTmp;

    /*
     *  Special case handling for characters the WMS specification
     *  says should not be encoded, when they occur in certain
     *  parameters.
     *
     *  Note: WMS 1.3 removes SRS and FORMAT from the set of
     *        exceptional cases, but renames SRS as CRS in any case.
     */
    if( strcmp(name,"LAYERS") == 0 ||
        strcmp(name,"STYLES") == 0 ||
        strcmp(name,"BBOX") == 0 ) {
      pszTmp = msEncodeUrlExcept(value,',');
    } else if ( strcmp(name,"SRS") == 0 ) {
      pszTmp = msEncodeUrlExcept(value,':');
    } else if ( nVersion < OWS_1_3_0 && strcmp(name,"FORMAT") == 0 ) {
      pszTmp = msEncodeUrlExcept(value,'/');
    } else {
      pszTmp = msEncodeUrl(value);
    }

    msInsertHashTable(psWMSParams->params, name, pszTmp);
    msFree(pszTmp);
  } else {
    msInsertHashTable(psWMSParams->params, name, value);
  }
  psWMSParams->numparams++;

  return MS_SUCCESS;
}
#endif /* def USE_WMS_LYR */

/**********************************************************************
 *                          msSetWMSParamInt()
 *
 **********************************************************************/

#ifdef USE_WMS_LYR
static int msSetWMSParamInt(wmsParamsObj *wmsparams,
                            const char *name, int value)
{
  char szBuf[100];

  snprintf(szBuf, sizeof(szBuf), "%d", value);
  msInsertHashTable(wmsparams->params, name, szBuf);
  wmsparams->numparams++;

  return MS_SUCCESS;
}
#endif /* def USE_WMS_LYR */

/**********************************************************************
 *                          msBuildWMSParamsUrl()
 *
 **********************************************************************/
static char *msBuildURLFromWMSParams(wmsParamsObj *wmsparams)
{
  const char *key, *value;
  size_t bufferSize = 0;
  int nLen;
  char *pszURL;

  /* Compute size required for URL buffer
   */
  nLen = strlen(wmsparams->onlineresource) + 3;

  key = msFirstKeyFromHashTable(wmsparams->params);
  while (key != NULL) {
    value = msLookupHashTable(wmsparams->params, key);
    nLen += strlen(key) + strlen(value) + 2;

    key = msNextKeyFromHashTable(wmsparams->params, key);
  }

  bufferSize = nLen+1;
  pszURL = (char*)msSmallMalloc(bufferSize);

  /* Start with the onlineresource value and append trailing '?' or '&'
   * if missing.
   */
  strcpy(pszURL, wmsparams->onlineresource);
  if (strchr(pszURL, '?') == NULL)
    strcat(pszURL, "?");
  else {
    char *c;
    c = pszURL+strlen(pszURL)-1;
    if (*c != '?' && *c != '&')
      strcpy(c+1, "&");
  }

  /* Now add all the parameters
   */
  nLen = strlen(pszURL);
  key = msFirstKeyFromHashTable(wmsparams->params);
  while (key != NULL) {
    value = msLookupHashTable(wmsparams->params, key);
    snprintf(pszURL+nLen, bufferSize-nLen, "%s=%s&", key, value);
    nLen += strlen(key) + strlen(value) + 2;
    key = msNextKeyFromHashTable(wmsparams->params, key);
  }

  /* Get rid of trailing '&'*/
  pszURL[nLen-1] = '\0';


  return pszURL;
}




#ifdef USE_WMS_LYR
/**********************************************************************
 *                          msBuildWMSLayerURLBase()
 *
 * Build the base of a GetMap or GetFeatureInfo URL using metadata.
 * The parameters to set are:
 *   VERSION
 *   LAYERS
 *   FORMAT
 *   TRANSPARENT
 *   STYLES
 *   QUERY_LAYERS (for queriable layers only)
 *
 * Returns a reference to a newly allocated string that should be freed
 * by the caller.
 **********************************************************************/
static int msBuildWMSLayerURLBase(mapObj *map, layerObj *lp,
                                  wmsParamsObj *psWMSParams, int nRequestType)
{
  const char *pszOnlineResource, *pszVersion, *pszName, *pszFormat;
  const char *pszFormatList, *pszStyle, /* *pszStyleList,*/ *pszTime;
  const char *pszBgColor, *pszTransparent;
  const char *pszSLD=NULL, *pszStyleSLDBody=NULL, *pszVersionKeyword=NULL;
  const char *pszSLDBody=NULL, *pszSLDURL = NULL;
  char *pszSLDGenerated = NULL;
  int nVersion=OWS_VERSION_NOTSET;

  /* If lp->connection is not set then use wms_onlineresource metadata */
  pszOnlineResource = lp->connection;
  if (pszOnlineResource == NULL)
    pszOnlineResource = msOWSLookupMetadata(&(lp->metadata),
                                            "MO", "onlineresource");

  pszVersion =        msOWSLookupMetadata(&(lp->metadata), "MO", "server_version");
  pszName =           msOWSLookupMetadata(&(lp->metadata), "MO", "name");
  pszFormat =         msOWSLookupMetadata(&(lp->metadata), "MO", "format");
  pszFormatList =     msOWSLookupMetadata(&(lp->metadata), "MO", "formatlist");
  pszStyle =          msOWSLookupMetadata(&(lp->metadata), "MO", "style");
  /*pszStyleList =      msOWSLookupMetadata(&(lp->metadata), "MO", "stylelist");*/
  pszTime =           msOWSLookupMetadata(&(lp->metadata), "MO", "time");
  pszSLDBody =        msOWSLookupMetadata(&(lp->metadata), "MO", "sld_body");
  pszSLDURL =         msOWSLookupMetadata(&(lp->metadata), "MO", "sld_url");
  pszBgColor =        msOWSLookupMetadata(&(lp->metadata), "MO", "bgcolor");
  pszTransparent =    msOWSLookupMetadata(&(lp->metadata), "MO", "transparent");

  if (pszOnlineResource==NULL || pszVersion==NULL || pszName==NULL) {
    msSetError(MS_WMSCONNERR,
               "One of wms_onlineresource, wms_server_version, wms_name "
               "metadata is missing in layer %s.  "
               "Please either provide a valid CONNECTION URL, or provide "
               "those values in the layer's metadata.\n",
               "msBuildWMSLayerURLBase()", lp->name);
    return MS_FAILURE;
  }

  psWMSParams->onlineresource = msStrdup(pszOnlineResource);

  if (strncmp(pszVersion, "1.0.7", 5) < 0)
    pszVersionKeyword = "WMTVER";
  else
    pszVersionKeyword = "VERSION";

  nVersion = msOWSParseVersionString(pszVersion);
  /* WMS 1.0.8 is really just 1.1.0 */
  if (nVersion == OWS_1_0_8) nVersion = OWS_1_1_0;

  msSetWMSParamString(psWMSParams, pszVersionKeyword, pszVersion, MS_FALSE, nVersion);
  msSetWMSParamString(psWMSParams, "SERVICE", "WMS",     MS_FALSE, nVersion);
  msSetWMSParamString(psWMSParams, "LAYERS",  pszName,   MS_TRUE, nVersion);

  if (pszFormat==NULL && pszFormatList==NULL) {
    msSetError(MS_WMSCONNERR,
               "At least wms_format or wms_formatlist is required for "
               "layer %s.  "
               "Please either provide a valid CONNECTION URL, or provide "
               "those values in the layer's metadata.\n",
               "msBuildWMSLayerURLBase()", lp->name);
    return MS_FAILURE;
  }

  if (pszFormat != NULL) {
    msSetWMSParamString(psWMSParams, "FORMAT",  pszFormat, MS_TRUE, nVersion);
  } else {
    /* Look for the first format in list that matches */
    char **papszTok;
    int i, n;
    papszTok = msStringSplit(pszFormatList, ',', &n);

    for(i=0; pszFormat==NULL && i<n; i++) {
      if (0
#if defined USE_PNG
          || strcasecmp(papszTok[i], "PNG")
          || strcasecmp(papszTok[i], "image/png")
#endif
#if defined USE_JPEG
          || strcasecmp(papszTok[i], "JPEG")
          || strcasecmp(papszTok[i], "image/jpeg")
#endif
         ) {
        pszFormat = papszTok[i];
      }
    }

    if (pszFormat) {
      msSetWMSParamString(psWMSParams, "FORMAT",  pszFormat, MS_TRUE, nVersion);
      msFreeCharArray(papszTok, n);
    } else {
      msSetError(MS_WMSCONNERR,
                 "Could not find a format that matches supported input "
                 "formats in wms_formatlist metdata in layer %s.  "
                 "Please either provide a valid CONNECTION URL, or "
                 "provide the required layer metadata.\n",
                 "msBuildWMSLayerURLBase()", lp->name);
      msFreeCharArray(papszTok, n);
      return MS_FAILURE;
    }
  }

  if (pszStyle==NULL) {
    /* When no style is selected, use "" which is a valid default. */
    pszStyle = "";
  } else {
    /* Was a wms_style_..._sld URL provided? */
    char szBuf[100];
    snprintf(szBuf, sizeof(szBuf), "style_%.80s_sld", pszStyle);
    pszSLD = msOWSLookupMetadata(&(lp->metadata), "MO", szBuf);
    snprintf(szBuf, sizeof(szBuf), "style_%.80s_sld_body", pszStyle);
    pszStyleSLDBody = msOWSLookupMetadata(&(lp->metadata), "MO", szBuf);

    if (pszSLD || pszStyleSLDBody) {
      /* SLD URL is set.  If this defn. came from a map context then */
      /* the style name may just be an internal name: "Style{%d}" if */
      /* that's the case then we should not pass this name via the URL */
      if (strncmp(pszStyle, "Style{", 6) == 0)
        pszStyle = "";
    }
  }

  /*  set STYLE parameter no matter what, even if it's empty (i.e. "STYLES=")
   *  GetLegendGraphic doesn't support multiple styles and is named STYLE
   */
  if (nRequestType == WMS_GETLEGENDGRAPHIC) {
    msSetWMSParamString(psWMSParams, "STYLE", pszStyle, MS_TRUE, nVersion);
  } else {
    msSetWMSParamString(psWMSParams, "STYLES", pszStyle, MS_TRUE, nVersion);
  }

  if (pszSLD != NULL) {
    /* Only SLD is set */
    msSetWMSParamString(psWMSParams, "SLD",    pszSLD,   MS_TRUE, nVersion);
  } else if (pszStyleSLDBody != NULL) {
    /* SLDBODY are set */
    msSetWMSParamString(psWMSParams, "SLD_BODY", pszStyleSLDBody, MS_TRUE, nVersion);
  }

  if (msIsLayerQueryable(lp)) {
    msSetWMSParamString(psWMSParams, "QUERY_LAYERS", pszName, MS_TRUE, nVersion);
  }
  if (pszTime && strlen(pszTime) > 0) {
    msSetWMSParamString(psWMSParams, "TIME",   pszTime,  MS_TRUE, nVersion);
  }

  /* if  the metadata wms_sld_body is set to AUTO, we generate
   * the sld based on classes found in the map file and send
   * it in the URL. If diffrent from AUTO, we are assuming that
   * it is a valid sld.
   */
  if (pszSLDBody) {
    if (strcasecmp(pszSLDBody, "AUTO") == 0) {
      if (pszVersion && strncmp(pszVersion, "1.3.0", 5) == 0)
        pszSLDGenerated = msSLDGenerateSLD(map, lp->index, "1.1.0");
      else
        pszSLDGenerated = msSLDGenerateSLD(map, lp->index, NULL);

      if (pszSLDGenerated) {
        msSetWMSParamString(psWMSParams, "SLD_BODY",
                            pszSLDGenerated, MS_TRUE, nVersion);
        free(pszSLDGenerated);
      }
    } else {
      msSetWMSParamString(psWMSParams, "SLD_BODY", pszSLDBody, MS_TRUE, nVersion);
    }

  }

  if (pszSLDURL) {
    msSetWMSParamString(psWMSParams, "SLD", pszSLDURL, MS_TRUE, nVersion);
  }

  if (pszBgColor) {
    msSetWMSParamString(psWMSParams, "BGCOLOR", pszBgColor, MS_TRUE, nVersion);
  }

  if (pszTransparent) {
    msSetWMSParamString(psWMSParams, "TRANSPARENT", pszTransparent, MS_TRUE, nVersion);
  } else {
    msSetWMSParamString(psWMSParams, "TRANSPARENT", "TRUE", MS_TRUE, nVersion);
  }

  return MS_SUCCESS;
}

#endif /* USE_WMS_LYR */


/**********************************************************************
 *                          msBuildWMSLayerURL()
 *
 * Build a GetMap or GetFeatureInfo URL.
 *
 * Returns a reference to a newly allocated string that should be freed
 * by the caller.
 **********************************************************************/

static int
msBuildWMSLayerURL(mapObj *map, layerObj *lp, int nRequestType,
                   int nClickX, int nClickY, int nFeatureCount,
                   const char *pszInfoFormat, rectObj *bbox_ret,
                   int *width_ret, int *height_ret,
                   wmsParamsObj *psWMSParams)
{
#ifdef USE_WMS_LYR
  char *pszEPSG = NULL;
  const char *pszVersion, *pszRequestParam, *pszExceptionsParam,
        *pszSrsParamName="SRS", *pszLayer=NULL, *pszQueryLayers=NULL,
        *pszUseStrictAxisOrder;
  rectObj bbox;
  int bbox_width = map->width, bbox_height = map->height;
  int nVersion=OWS_VERSION_NOTSET;
  int bUseStrictAxisOrder = MS_FALSE; /* this is the assumption up to 1.1.0 */
  int bFlipAxisOrder = MS_FALSE;
  const char *pszTmp;
  int bIsEssential = MS_FALSE;

  if (lp->connectiontype != MS_WMS) {
    msSetError(MS_WMSCONNERR, "Call supported only for CONNECTIONTYPE WMS",
               "msBuildWMSLayerURL()");
    return MS_FAILURE;
  }

  /* ------------------------------------------------------------------
   * Find out request version
   * ------------------------------------------------------------------ */
  if (lp->connection == NULL ||
      ((pszVersion = strstr(lp->connection, "VERSION=")) == NULL &&
       (pszVersion = strstr(lp->connection, "version=")) == NULL &&
       (pszVersion = strstr(lp->connection, "WMTVER=")) == NULL &&
       (pszVersion = strstr(lp->connection, "wmtver=")) == NULL ) ) {
    /* CONNECTION missing or seems incomplete... try to build from metadata */
    if (msBuildWMSLayerURLBase(map, lp, psWMSParams, nRequestType) != MS_SUCCESS)
      return MS_FAILURE;  /* An error already produced. */

    /* If we received MS_SUCCESS then version must have been set */
    pszVersion = msLookupHashTable(psWMSParams->params, "VERSION");
    if (pszVersion ==NULL)
      pszVersion = msLookupHashTable(psWMSParams->params, "WMTVER");

    nVersion = msOWSParseVersionString(pszVersion);
  } else {
    /* CONNECTION string seems complete, start with that. */
    char *pszDelimiter;
    psWMSParams->onlineresource = msStrdup(lp->connection);

    /* Fetch version info */
    pszVersion = strchr(pszVersion, '=')+1;
    pszDelimiter = strchr(pszVersion, '&');
    if (pszDelimiter != NULL)
      *pszDelimiter = '\0';
    nVersion = msOWSParseVersionString(pszVersion);
    if (pszDelimiter != NULL)
      *pszDelimiter = '&';
  }

  switch (nVersion) {
    case OWS_1_0_8:
      nVersion = OWS_1_1_0;    /* 1.0.8 == 1.1.0 */
      break;
    case OWS_1_0_0:
    case OWS_1_0_1:
    case OWS_1_0_7:
    case OWS_1_1_0:
    case OWS_1_1_1:
      /* All is good, this is a supported version. */
      break;
    case OWS_1_3_0:
      /* 1.3.0 introduces a few changes... */
      pszSrsParamName = "CRS";
      bUseStrictAxisOrder = MS_TRUE; /* this is the assumption for 1.3.0 */
      break;
    default:
      /* Not a supported version */
      msSetError(MS_WMSCONNERR, "MapServer supports only WMS 1.0.0 to 1.3.0 (please verify the VERSION parameter in the connection string).", "msBuildWMSLayerURL()");
      return MS_FAILURE;
  }

  /* ------------------------------------------------------------------
   * For GetFeatureInfo requests, make sure QUERY_LAYERS is included
   * ------------------------------------------------------------------ */
  if  (nRequestType == WMS_GETFEATUREINFO &&
       strstr(psWMSParams->onlineresource, "QUERY_LAYERS=") == NULL &&
       strstr(psWMSParams->onlineresource, "query_layers=") == NULL &&
       msLookupHashTable(psWMSParams->params, "QUERY_LAYERS") == NULL) {
    pszQueryLayers = msOWSLookupMetadata(&(lp->metadata), "MO", "name");

    if (pszQueryLayers == NULL) {
      msSetError(MS_WMSCONNERR, "wms_name not set or WMS Connection String must contain the QUERY_LAYERS parameter to support GetFeatureInfo requests (with name in uppercase).", "msBuildWMSLayerURL()");
      return MS_FAILURE;
    }
  }

  /* ------------------------------------------------------------------
   * For GetLegendGraphic requests, make sure LAYER is included
   * ------------------------------------------------------------------ */
  if  (nRequestType == WMS_GETLEGENDGRAPHIC &&
       strstr(psWMSParams->onlineresource, "LAYER=") == NULL &&
       strstr(psWMSParams->onlineresource, "layer=") == NULL &&
       msLookupHashTable(psWMSParams->params, "LAYER") == NULL) {
    pszLayer = msOWSLookupMetadata(&(lp->metadata), "MO", "name");

    if (pszLayer == NULL) {
      msSetError(MS_WMSCONNERR, "wms_name not set or WMS Connection String must contain the LAYER parameter to support GetLegendGraphic requests (with name in uppercase).", "msBuildWMSLayerURL()");
      return MS_FAILURE;
    }
  }

  /* ------------------------------------------------------------------
   * Figure the SRS we'll use for the request.
   * - Fetch the map SRS (if it's EPSG)
   * - Check if map SRS is listed in layer wms_srs metadata
   * - If map SRS is valid for this layer then use it
   * - Otherwise request layer in its default SRS and we'll reproject later
   * ------------------------------------------------------------------ */
  msOWSGetEPSGProj(&(map->projection),NULL, NULL, MS_TRUE, &pszEPSG);
  if ( pszEPSG &&
      (strncasecmp(pszEPSG, "EPSG:", 5) == 0 ||
       strncasecmp(pszEPSG, "AUTO:", 5) == 0) ) {
    const char *pszFound;
    char *pszLyrEPSG;
    int nLen;
    char *pszPtr = NULL;

    /* If it's an AUTO projection then keep only id and strip off  */
    /* the parameters for now (we'll restore them at the end) */
    if (strncasecmp(pszEPSG, "AUTO:", 5) == 0) {
      if ((pszPtr = strchr(pszEPSG, ',')))
        *pszPtr = '\0';
    }

    nLen = strlen(pszEPSG);

    msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "MO", MS_FALSE, &pszLyrEPSG);

    if (pszLyrEPSG == NULL ||
        (pszFound = strstr(pszLyrEPSG, pszEPSG)) == NULL ||
        ! ((*(pszFound+nLen) == '\0') || isspace(*(pszFound+nLen))) ) {
      /* Not found in Layer's list of SRS (including projection object) */
      free(pszEPSG);
      pszEPSG = NULL;
    }
    msFree(pszLyrEPSG);
    if (pszEPSG && pszPtr)
      *pszPtr = ',';  /* Restore full AUTO:... definition */
  }

  if (pszEPSG == NULL) {
      msOWSGetEPSGProj(&(lp->projection), &(lp->metadata),"MO", MS_TRUE, &pszEPSG);
      if( pszEPSG == NULL || (strncasecmp(pszEPSG, "EPSG:", 5) != 0 && strncasecmp(pszEPSG, "AUTO:", 5) != 0) )  {
        msSetError(MS_WMSCONNERR, "Layer must have an EPSG or AUTO projection code (in its PROJECTION object or wms_srs metadata)", "msBuildWMSLayerURL()");
        msFree(pszEPSG);
        return MS_FAILURE;
      }
  }

  /* ------------------------------------------------------------------
   * For an AUTO projection, set the Units,lon0,lat0 if not already set
   * ------------------------------------------------------------------ */
  if (strncasecmp(pszEPSG, "AUTO:", 5) == 0 &&
      strchr(pszEPSG, ',') == NULL) {
    pointObj oPoint;
    char *pszNewEPSG;

    /* Use center of map view for lon0,lat0 */
    oPoint.x = (map->extent.minx + map->extent.maxx)/2.0;
    oPoint.y = (map->extent.miny + map->extent.maxy)/2.0;
    msProjectPoint(&(map->projection), &(map->latlon), &oPoint);

    pszNewEPSG = (char*)msSmallMalloc(101*sizeof(char));

    snprintf(pszNewEPSG, 100, "%s,9001,%.16g,%.16g",
             pszEPSG, oPoint.x, oPoint.y);
    pszNewEPSG[100]='\0';
    free(pszEPSG);
    pszEPSG=pszNewEPSG;
  }

  /*
   * Work out whether we'll be wanting to flip the axis order for the request
   */
  pszUseStrictAxisOrder = msOWSLookupMetadata(&(lp->metadata), "MO", "strict_axis_order");
  if (pszUseStrictAxisOrder != NULL) {
    if (strncasecmp(pszUseStrictAxisOrder, "1", 1) == 0 ||
        strncasecmp(pszUseStrictAxisOrder, "true", 4) == 0) {
      bUseStrictAxisOrder = MS_TRUE;
    } else if (strncasecmp(pszUseStrictAxisOrder, "0", 1) == 0 ||
        strncasecmp(pszUseStrictAxisOrder, "false", 5) == 0) {
      bUseStrictAxisOrder = MS_FALSE;
    }
  }
  if (bUseStrictAxisOrder == MS_TRUE && pszEPSG &&
      strncasecmp(pszEPSG, "EPSG:", 5) == 0 &&
      msIsAxisInverted(atoi(pszEPSG + 5))) {
    bFlipAxisOrder = MS_TRUE;
  }

  /* ------------------------------------------------------------------
   * Set layer SRS.
   * ------------------------------------------------------------------ */
  /* No need to set lp->proj if it's already set to the right EPSG code */
  {
    char* pszEPSGCodeFromLayer = NULL;
    msOWSGetEPSGProj(&(lp->projection), NULL, "MO", MS_TRUE, &pszEPSGCodeFromLayer);
    if (pszEPSGCodeFromLayer == NULL || strcasecmp(pszEPSG, pszEPSGCodeFromLayer) != 0) {
      char *ows_srs = NULL;
      msOWSGetEPSGProj(NULL, &(lp->metadata), "MO", MS_FALSE, &ows_srs);
      /* no need to set lp->proj if it is already set and there is only
      one item in the _srs metadata for this layer - we will assume
      the projection block matches the _srs metadata (the search for ' '
      in ows_srs is a test to see if there are multiple EPSG: codes) */
      if( lp->projection.numargs == 0 || ows_srs == NULL || (strchr(ows_srs,' ') != NULL) ) {
        if (strncasecmp(pszEPSG, "EPSG:", 5) == 0) {
          char szProj[20];
          snprintf(szProj, sizeof(szProj), "init=epsg:%s", pszEPSG+5);
          if (msLoadProjectionString(&(lp->projection), szProj) != 0) {
            msFree(pszEPSGCodeFromLayer);
            msFree(ows_srs);
            return MS_FAILURE;
          }
        } else {
          if (msLoadProjectionString(&(lp->projection), pszEPSG) != 0) {
            msFree(pszEPSGCodeFromLayer);
            msFree(ows_srs);
            return MS_FAILURE;
          }
        }
      }
      msFree(ows_srs);
    }
    msFree(pszEPSGCodeFromLayer);
  }

  /* ------------------------------------------------------------------
   * Adjust for MapServer EXTENT being center of pixel and WMS BBOX being
   * edge of pixel (#2843).
   * ------------------------------------------------------------------ */
  bbox = map->extent;

  bbox.minx -= map->cellsize * 0.5;
  bbox.maxx += map->cellsize * 0.5;
  bbox.miny -= map->cellsize * 0.5;
  bbox.maxy += map->cellsize * 0.5;

  /* -------------------------------------------------------------------- */
  /*      Reproject if needed.                                            */
  /* -------------------------------------------------------------------- */
  if (msProjectionsDiffer(&(map->projection), &(lp->projection))) {
    msProjectRect(&(map->projection), &(lp->projection), &bbox);

    /* -------------------------------------------------------------------- */
    /*      Sometimes our remote WMS only accepts square pixel              */
    /*      requests.  If this is the case adjust adjust the number of      */
    /*      pixels or lines in the request so that the pixels are           */
    /*      square.                                                         */
    /* -------------------------------------------------------------------- */
    {
      const char *nonsquare_ok =
        msOWSLookupMetadata(&(lp->metadata),
                            "MO", "nonsquare_ok");

      /* assume nonsquare_ok is false */
      if( nonsquare_ok != NULL
          && (strcasecmp(nonsquare_ok,"no") == 0
              || strcasecmp(nonsquare_ok,"false") == 0) ) {
        double cellsize_x = (bbox.maxx-bbox.minx) / bbox_width;
        double cellsize_y = (bbox.maxy-bbox.miny) / bbox_height;

        if( cellsize_x < cellsize_y * 0.999999 ) {
          int new_bbox_height =
            ceil((cellsize_y/cellsize_x) * bbox_height);

          if (lp->debug)
            msDebug("NONSQUARE_OK=%s, adjusted HEIGHT from %d to %d to equalize cellsize at %g.\n",
                    nonsquare_ok,
                    bbox_height, new_bbox_height, cellsize_x );
          bbox_height = new_bbox_height;
        } else if( cellsize_y < cellsize_x * 0.999999 ) {
          int new_bbox_width =
            ceil((cellsize_x/cellsize_y) * bbox_width);

          if (lp->debug)
            msDebug("NONSQUARE_OK=%s, adjusted WIDTH from %d to %d to equalize cellsize at %g.\n",
                    nonsquare_ok,
                    bbox_width, new_bbox_width, cellsize_y );
          bbox_width = new_bbox_width;
        } else {
          if (lp->debug)
            msDebug("NONSQUARE_OK=%s, but cellsize was already square - no change.\n",
                    nonsquare_ok );
        }
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      If the layer has predefined extents, and a predefined           */
  /*      projection that matches the request projection, then            */
  /*      consider restricting the BBOX to match the limits.              */
  /* -------------------------------------------------------------------- */
  if( bbox_width != 0 ) {
    char *ows_srs;
    rectObj  layer_rect;

    msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "MO", MS_FALSE, &ows_srs);

    if( ows_srs && strchr(ows_srs,' ') == NULL
        && msOWSGetLayerExtent( map, lp, "MO", &layer_rect) == MS_SUCCESS ) {
      /* fulloverlap */
      if( msRectContained( &bbox, &layer_rect ) ) {
        /* no changes */
      }

      /* no overlap */
      else if( !msRectOverlap( &layer_rect, &bbox ) ) {
        bbox_width = 0;
        bbox_height = 0;
      }

      else {
        double cellsize_x = (bbox.maxx-bbox.minx) / bbox_width;
        double cellsize_y = (bbox.maxy-bbox.miny) / bbox_height;
        double cellsize = MS_MIN(cellsize_x,cellsize_y);

        msRectIntersect( &bbox, &layer_rect );

        bbox_width = round((bbox.maxx - bbox.minx) / cellsize);
        bbox_height = round((bbox.maxy - bbox.miny) / cellsize);

        /* Force going through the resampler if we're going to receive a clipped BBOX (#4931) */
        if(msLayerGetProcessingKey(lp, "RESAMPLE") == NULL) {
          msLayerSetProcessingKey(lp, "RESAMPLE", "nearest");
        }
      }
    }
    msFree(ows_srs);
  }

  /* -------------------------------------------------------------------- */
  /*      Potentially return the bbox.                                    */
  /* -------------------------------------------------------------------- */
  if (bbox_ret != NULL)
    *bbox_ret = bbox;

  if( width_ret != NULL )
    *width_ret = bbox_width;

  if( height_ret != NULL )
    *height_ret = bbox_height;

  /* ------------------------------------------------------------------
   * Build the request URL.
   * At this point we set only the following parameters for GetMap:
   *   REQUEST
   *   SRS (or CRS)
   *   BBOX
   *
   * And for GetFeatureInfo:
   *   X (I for 1.3.0)
   *   Y (J for 1.3.0)
   *   INFO_FORMAT
   *   FEATURE_COUNT (only if nFeatureCount > 0)
   *
   * The connection string should contain all other required params,
   * including:
   *   VERSION
   *   LAYERS
   *   FORMAT
   *   TRANSPARENT
   *   STYLES
   *   QUERY_LAYERS (for queryable layers only)
   * ------------------------------------------------------------------ */

  /* ------------------------------------------------------------------
   * Sometimes a requested layer is essential for the map, so if the
   * request fails or an error is delivered, the map has not to be drawn
   * ------------------------------------------------------------------ */
  if ((pszTmp = msOWSLookupMetadata(&(lp->metadata),
                                    "MO", "essential")) != NULL) {
    if( strcasecmp(pszTmp,"true") == 0
        || strcasecmp(pszTmp,"on") == 0
        || strcasecmp(pszTmp,"yes") == 0 )
      bIsEssential = MS_TRUE;
    else
      bIsEssential = atoi(pszTmp);       
  }

  if (nRequestType == WMS_GETFEATUREINFO) {
    char szBuf[100] = "";

    if (nVersion >= OWS_1_0_7)
      pszRequestParam = "GetFeatureInfo";
    else
      pszRequestParam = "feature_info";

    if (nVersion >= OWS_1_3_0)
      pszExceptionsParam = "XML";
    else if (nVersion >= OWS_1_1_0) /* 1.1.0 to 1.1.0 */
      pszExceptionsParam = "application/vnd.ogc.se_xml";
    else if (nVersion > OWS_1_0_0)  /* 1.0.1 to 1.0.7 */
      pszExceptionsParam = "SE_XML";
    else
      pszExceptionsParam = "WMS_XML";

    msSetWMSParamString(psWMSParams, "REQUEST", pszRequestParam, MS_FALSE, nVersion);
    msSetWMSParamInt(   psWMSParams, "WIDTH",   bbox_width);
    msSetWMSParamInt(   psWMSParams, "HEIGHT",  bbox_height);

    msSetWMSParamString(psWMSParams, pszSrsParamName, pszEPSG, MS_FALSE, nVersion);

    if (bFlipAxisOrder == MS_TRUE) {
      snprintf(szBuf, sizeof(szBuf), "%.15g,%.15g,%.15g,%.15g",
               bbox.miny, bbox.minx, bbox.maxy, bbox.maxx);
    } else {
      snprintf(szBuf, sizeof(szBuf), "%.15g,%.15g,%.15g,%.15g",
               bbox.minx, bbox.miny, bbox.maxx, bbox.maxy);
    }
    msSetWMSParamString(psWMSParams, "BBOX",    szBuf, MS_TRUE, nVersion);

    if (nVersion >= OWS_1_3_0) {
      msSetWMSParamInt(   psWMSParams, "I",       nClickX);
      msSetWMSParamInt(   psWMSParams, "J",       nClickY);
    } else {
      msSetWMSParamInt(   psWMSParams, "X",       nClickX);
      msSetWMSParamInt(   psWMSParams, "Y",       nClickY);
    }

    msSetWMSParamString(psWMSParams, "EXCEPTIONS", pszExceptionsParam, MS_FALSE, nVersion);
    msSetWMSParamString(psWMSParams, "INFO_FORMAT", pszInfoFormat, MS_TRUE, nVersion);

    if (pszQueryLayers) { /* not set in CONNECTION string */
      msSetWMSParamString(psWMSParams, "QUERY_LAYERS", pszQueryLayers, MS_FALSE, nVersion);
    }

    /* If FEATURE_COUNT <= 0 then don't pass this parameter */
    /* The spec states that FEATURE_COUNT must be greater than zero */
    /* and if not passed then the behavior is up to the server */
    if (nFeatureCount > 0) {
      msSetWMSParamInt(psWMSParams, "FEATURE_COUNT", nFeatureCount);
    }

  } else if (nRequestType == WMS_GETLEGENDGRAPHIC) {
    if(map->extent.maxx > map->extent.minx && map->width > 0 && map->height > 0) {
      char szBuf[20] = "";
      double scaledenom;
      msCalculateScale(map->extent, map->units, map->width, map->height,
                     map->resolution, &scaledenom);
      snprintf(szBuf, 20, "%g",scaledenom);
      msSetWMSParamString(psWMSParams, "SCALE", szBuf, MS_FALSE, nVersion);
    }
    pszRequestParam = "GetLegendGraphic";

    pszExceptionsParam = msOWSLookupMetadata(&(lp->metadata),
                         "MO", "exceptions_format");
    if (pszExceptionsParam == NULL) {
      if (nVersion >= OWS_1_1_0 && nVersion < OWS_1_3_0)
        pszExceptionsParam = "application/vnd.ogc.se_inimage";
      else
        pszExceptionsParam = "INIMAGE";
    }

    if (pszLayer) { /* not set in CONNECTION string */
      msSetWMSParamString(psWMSParams, "LAYER", pszLayer, MS_FALSE, nVersion);
    }

    msSetWMSParamString(psWMSParams, "REQUEST", pszRequestParam, MS_FALSE, nVersion);
    msSetWMSParamString(psWMSParams, pszSrsParamName, pszEPSG, MS_FALSE, nVersion);

    if (nVersion >= OWS_1_3_0) {
      msSetWMSParamString(psWMSParams, "SLD_VERSION", "1.1.0", MS_FALSE, nVersion);
    }

  } else { /* if (nRequestType == WMS_GETMAP) */
    char szBuf[100] = "";

    if (nVersion >= OWS_1_0_7)
      pszRequestParam = "GetMap";
    else
      pszRequestParam = "map";

    pszExceptionsParam = msOWSLookupMetadata(&(lp->metadata),
                         "MO", "exceptions_format");

    if (!bIsEssential) {
      if (pszExceptionsParam == NULL) {
        if (nVersion >= OWS_1_1_0 && nVersion < OWS_1_3_0)
          pszExceptionsParam = "application/vnd.ogc.se_inimage";
        else
          pszExceptionsParam = "INIMAGE";
      }
    } else {
      /* if layer is essential, do not emit EXCEPTIONS parameter (defaults to XML) */
      pszExceptionsParam = NULL;
    }

    msSetWMSParamString(psWMSParams, "REQUEST", pszRequestParam, MS_FALSE, nVersion);
    msSetWMSParamInt(   psWMSParams, "WIDTH",   bbox_width);
    msSetWMSParamInt(   psWMSParams, "HEIGHT",  bbox_height);
    msSetWMSParamString(psWMSParams, pszSrsParamName, pszEPSG, MS_FALSE, nVersion);

    if (bFlipAxisOrder == MS_TRUE) {
      snprintf(szBuf, sizeof(szBuf), "%.15g,%.15g,%.15g,%.15g",
               bbox.miny, bbox.minx, bbox.maxy, bbox.maxx);
    } else {
      snprintf(szBuf, sizeof(szBuf), "%.15g,%.15g,%.15g,%.15g",
               bbox.minx, bbox.miny, bbox.maxx, bbox.maxy);
    }
    msSetWMSParamString(psWMSParams, "BBOX",    szBuf, MS_TRUE, nVersion);
    if( pszExceptionsParam ) {
      msSetWMSParamString(psWMSParams, "EXCEPTIONS",  pszExceptionsParam, MS_FALSE, nVersion);
    }
  }

  free(pszEPSG);

  return MS_SUCCESS;

#else
  /* ------------------------------------------------------------------
   * WMS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WMSCONNERR, "WMS CLIENT CONNECTION support is not available.",
             "msBuildWMSLayerURL()");
  return MS_FAILURE;

#endif /* USE_WMS_LYR */

}


/**********************************************************************
 *                          msWMSGetFeatureInfoURL()
 *
 * Build a GetFeatureInfo URL for this layer.
 *
 * Returns a reference to a newly allocated string that should be freed
 * by the caller.
 **********************************************************************/
char *msWMSGetFeatureInfoURL(mapObj *map, layerObj *lp,
                             int nClickX, int nClickY, int nFeatureCount,
                             const char *pszInfoFormat)
{
  wmsParamsObj sThisWMSParams;
  char *pszURL;

  msInitWmsParamsObj(&sThisWMSParams);

  if (msBuildWMSLayerURL(map, lp, WMS_GETFEATUREINFO,
                         nClickX, nClickY, nFeatureCount,
                         pszInfoFormat, NULL, NULL, NULL,
                         &sThisWMSParams)!= MS_SUCCESS) {
    return NULL;
  }

  pszURL = msBuildURLFromWMSParams(&sThisWMSParams);
  msFreeWmsParamsObj(&sThisWMSParams);

  return pszURL;
}

/**********************************************************************
 *                          msPrepareWMSLayerRequest()
 *
 **********************************************************************/

int msPrepareWMSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             int nRequestType, enum MS_CONNECTION_TYPE lastconnectiontype,
                             wmsParamsObj *psLastWMSParams,
                             int nClickX, int nClickY, int nFeatureCount, const char *pszInfoFormat,
                             httpRequestObj *pasReqInfo, int *numRequests)
{
#ifdef USE_WMS_LYR
  char *pszURL = NULL, *pszHTTPCookieData = NULL;
  const char *pszTmp;
  rectObj bbox = { 0 };
  int bbox_width = 0, bbox_height = 0;
  int nTimeout, bOkToMerge, bForceSeparateRequest, bCacheToDisk;
  wmsParamsObj sThisWMSParams;

  if (lp->connectiontype != MS_WMS)
    return MS_FAILURE;

  msInitWmsParamsObj(&sThisWMSParams);

  /* ------------------------------------------------------------------
   * Build the request URL, this will also set layer projection and
   * compute BBOX in that projection.
   * ------------------------------------------------------------------ */


  if (nRequestType == WMS_GETMAP &&
      ( msBuildWMSLayerURL(map, lp, WMS_GETMAP,
                           0, 0, 0, NULL, &bbox, &bbox_width, &bbox_height,
                           &sThisWMSParams) != MS_SUCCESS) ) {
    /* an error was already reported. */
    msFreeWmsParamsObj(&sThisWMSParams);
    return MS_FAILURE;
  }

  else if (nRequestType == WMS_GETFEATUREINFO &&
           msBuildWMSLayerURL(map, lp, WMS_GETFEATUREINFO,
                              nClickX, nClickY, nFeatureCount, pszInfoFormat,
                              NULL, NULL, NULL,
                              &sThisWMSParams) != MS_SUCCESS ) {
    /* an error was already reported. */
    msFreeWmsParamsObj(&sThisWMSParams);
    return MS_FAILURE;
  } else if (nRequestType == WMS_GETLEGENDGRAPHIC &&
             msBuildWMSLayerURL(map, lp, WMS_GETLEGENDGRAPHIC,
                                0, 0, 0, NULL,
                                NULL, NULL, NULL,
                                &sThisWMSParams) != MS_SUCCESS ) {
    /* an error was already reported. */
    msFreeWmsParamsObj(&sThisWMSParams);
    return MS_FAILURE;
  }


  /* ------------------------------------------------------------------
   * Check if the request is empty, perhaps due to reprojection problems
   * or wms_extents restrictions.
   * ------------------------------------------------------------------ */
  if ((nRequestType == WMS_GETMAP) && (bbox_width == 0 || bbox_height == 0) ) {
    msFreeWmsParamsObj(&sThisWMSParams);
    return MS_SUCCESS;  /* No overlap. */
  }

  /* ------------------------------------------------------------------
   * Check if layer overlaps current view window (using wms_latlonboundingbox)
   * ------------------------------------------------------------------ */
  if ((pszTmp = msOWSLookupMetadata(&(lp->metadata),
                                    "MO", "latlonboundingbox")) != NULL) {
    char **tokens;
    int n;
    rectObj ext;

    tokens = msStringSplit(pszTmp, ' ', &n);
    if (tokens==NULL || n != 4) {
      msSetError(MS_WMSCONNERR, "Wrong number of arguments for 'wms_latlonboundingbox' metadata.",
                 "msDrawWMSLayer()");
      msFreeWmsParamsObj(&sThisWMSParams);
      return MS_FAILURE;
    }

    ext.minx = atof(tokens[0]);
    ext.miny = atof(tokens[1]);
    ext.maxx = atof(tokens[2]);
    ext.maxy = atof(tokens[3]);

    msFreeCharArray(tokens, n);

    /* Reproject latlonboundingbox to the selected SRS for the layer and */
    /* check if it overlaps the bbox that we calculated for the request */

    msProjectRect(&(map->latlon), &(lp->projection), &ext);
    if (!msRectOverlap(&bbox, &ext)) {
      /* No overlap... nothing to do */

      msFreeWmsParamsObj(&sThisWMSParams);
      return MS_SUCCESS;  /* No overlap. */
    }
  }

  /* ------------------------------------------------------------------
   * check to see if a the metadata wms_connectiontimeout is set. If it is
   * the case we will use it, else we use the default which is 30 seconds.
   * First check the metadata in the layer object and then in the map object.
   * ------------------------------------------------------------------ */
  nTimeout = 30;  /* Default is 30 seconds  */
  if ((pszTmp = msOWSLookupMetadata2(&(lp->metadata), &(map->web.metadata),
                                    "MO", "connectiontimeout")) != NULL) {
    nTimeout = atoi(pszTmp);
  }

  /* ------------------------------------------------------------------
   * Check if we want to use in memory images instead of writing to disk.
   * ------------------------------------------------------------------ */
  if ((pszTmp = msOWSLookupMetadata(&(lp->metadata),
                                    "MO", "cache_to_disk")) != NULL) {
    if( strcasecmp(pszTmp,"true") == 0
        || strcasecmp(pszTmp,"on") == 0
        || strcasecmp(pszTmp,"yes") == 0 )
      bCacheToDisk = MS_TRUE;
    else
      bCacheToDisk = atoi(pszTmp);
  } else
    bCacheToDisk = MS_FALSE;

  if( bCacheToDisk ) {
    /* We'll store the remote server's response to a tmp file. */
    if (map->web.imagepath == NULL || strlen(map->web.imagepath) == 0) {
      msSetError(MS_WMSERR,
                 "WEB.IMAGEPATH must be set to use WMS client connections.",
                 "msPrepareWMSLayerRequest()");
      return MS_FAILURE;
    }
  }

  /* ------------------------------------------------------------------
   * Check if layer can be merged with previous WMS layer requests
   * Metadata wms_force_separate_request can be set to 1 to prevent this
   * this layer from being combined with any other layer.
   * ------------------------------------------------------------------ */
  bForceSeparateRequest = MS_FALSE;
  if ((pszTmp = msOWSLookupMetadata(&(lp->metadata),
                                    "MO", "force_separate_request")) != NULL) {
    bForceSeparateRequest = atoi(pszTmp);
  }
  bOkToMerge = MS_FALSE;
  if (!bForceSeparateRequest &&
      lastconnectiontype == MS_WMS &&
      psLastWMSParams != NULL &&
      sThisWMSParams.numparams == psLastWMSParams->numparams &&
      strcmp(sThisWMSParams.onlineresource,
             psLastWMSParams->onlineresource) == 0) {
    const char *key, *value1, *value2;
    bOkToMerge = MS_TRUE;

    key = msFirstKeyFromHashTable(sThisWMSParams.params);
    while (key != NULL && bOkToMerge == MS_TRUE) {
      /* Skip parameters whose values can be different */
      if (!(strcmp(key, "LAYERS") == 0 ||
            strcmp(key, "QUERY_LAYERS") == 0 ||
            strcmp(key, "STYLES") == 0) ) {
        value1 = msLookupHashTable(psLastWMSParams->params, key);
        value2 = msLookupHashTable(sThisWMSParams.params, key);

        if (value1==NULL || value2==NULL ||
            strcmp(value1, value2) != 0) {
          bOkToMerge = MS_FALSE;
          break;
        }
      }
      key = msNextKeyFromHashTable(sThisWMSParams.params, key);
    }
  }

  /*------------------------------------------------------------------
   * Check to see if there's a HTTP Cookie to forward
   * If Cookie differ between the two connection, it's NOT OK to merge
   * the connection
   * ------------------------------------------------------------------ */
  if ((pszTmp = msOWSLookupMetadata(&(lp->metadata),
                                    "MO", "http_cookie")) != NULL) {
    if(strcasecmp(pszTmp, "forward") == 0) {
      pszTmp= msLookupHashTable(&(map->web.metadata),"http_cookie_data");
      if(pszTmp != NULL) {
        pszHTTPCookieData = msStrdup(pszTmp);
      }
    } else {
      pszHTTPCookieData = msStrdup(pszTmp);
    }
  } else if ((pszTmp = msOWSLookupMetadata(&(map->web.metadata),
                       "MO", "http_cookie")) != NULL) {
    if(strcasecmp(pszTmp, "forward") == 0) {
      pszTmp= msLookupHashTable(&(map->web.metadata),"http_cookie_data");
      if(pszTmp != NULL) {
        pszHTTPCookieData = msStrdup(pszTmp);
      }
    } else {
      pszHTTPCookieData = msStrdup(pszTmp);
    }
  }

  if(bOkToMerge && pszHTTPCookieData != sThisWMSParams.httpcookiedata) {
    if(pszHTTPCookieData == NULL || sThisWMSParams.httpcookiedata == NULL) {
      bOkToMerge = MS_FALSE;
    }
    if(strcmp(pszHTTPCookieData, sThisWMSParams.httpcookiedata) != 0) {
      bOkToMerge = MS_FALSE;
    }
  }

  if (bOkToMerge) {
    /* Merge both requests into sThisWMSParams
     */
    const char *value1, *value2;
    char *keys[] = {"LAYERS", "QUERY_LAYERS", "STYLES"};
    int i;

    for(i=0; i<3; i++) {
      value1 = msLookupHashTable(psLastWMSParams->params, keys[i]);
      value2 = msLookupHashTable(sThisWMSParams.params, keys[i]);
      if (value1 && value2) {
        char *pszBuf;
        int nLen;

        nLen = strlen(value1) + strlen(value2) +2;
        pszBuf = malloc(nLen);
        MS_CHECK_ALLOC(pszBuf, nLen, MS_FAILURE);

        snprintf(pszBuf, nLen, "%s,%s", value1, value2);
        /* TODO should really send the server request version here */
        msSetWMSParamString(&sThisWMSParams, keys[i], pszBuf,MS_FALSE, OWS_VERSION_NOTSET);

        /* This key existed already, we don't want it counted twice */
        sThisWMSParams.numparams--;

        msFree(pszBuf);
      }
    }
  }


  /* ------------------------------------------------------------------
   * Build new request URL
   * ------------------------------------------------------------------ */
  pszURL = msBuildURLFromWMSParams(&sThisWMSParams);

  if (bOkToMerge && (*numRequests)>0) {
    /* ------------------------------------------------------------------
     * Update the last request in the array:  (*numRequests)-1
     * ------------------------------------------------------------------ */
    msFree(pasReqInfo[(*numRequests)-1].pszGetUrl);
    pasReqInfo[(*numRequests)-1].pszGetUrl = pszURL;
    pszURL = NULL;
    pasReqInfo[(*numRequests)-1].debug |= lp->debug;
    if (nTimeout > pasReqInfo[(*numRequests)-1].nTimeout)
      pasReqInfo[(*numRequests)-1].nTimeout = nTimeout;
  } else {
    /* ------------------------------------------------------------------
     * Add a request to the array (already preallocated)
     * ------------------------------------------------------------------ */
    pasReqInfo[(*numRequests)].nLayerId = nLayerId;
    pasReqInfo[(*numRequests)].pszGetUrl = pszURL;

    pszURL = NULL;
    pasReqInfo[(*numRequests)].pszHTTPCookieData = pszHTTPCookieData;
    pszHTTPCookieData = NULL;
    if( bCacheToDisk ) {
      pasReqInfo[(*numRequests)].pszOutputFile =
        msTmpFile(map, map->mappath, NULL, "wms.tmp");
    } else
      pasReqInfo[(*numRequests)].pszOutputFile = NULL;
    pasReqInfo[(*numRequests)].nStatus = 0;
    pasReqInfo[(*numRequests)].nTimeout = nTimeout;
    pasReqInfo[(*numRequests)].bbox   = bbox;
    pasReqInfo[(*numRequests)].width  = bbox_width;
    pasReqInfo[(*numRequests)].height = bbox_height;
    pasReqInfo[(*numRequests)].debug = lp->debug;

    if (msHTTPAuthProxySetup(&(map->web.metadata), &(lp->metadata),
                             pasReqInfo, *numRequests, map, "MO") != MS_SUCCESS)
      return MS_FAILURE;

    (*numRequests)++;
  }


  /* ------------------------------------------------------------------
   * Replace contents of psLastWMSParams with sThisWMSParams
   * unless bForceSeparateRequest is set in which case we make it empty
   * ------------------------------------------------------------------ */
  if (psLastWMSParams) {
    msFreeWmsParamsObj(psLastWMSParams);
    if (!bForceSeparateRequest)
      *psLastWMSParams = sThisWMSParams;
    else
      msInitWmsParamsObj(psLastWMSParams);
  } else {
    /* Can't copy it, so we just free it */
    msFreeWmsParamsObj(&sThisWMSParams);
  }

  return MS_SUCCESS;

#else
  /* ------------------------------------------------------------------
   * WMS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WMSCONNERR, "WMS CLIENT CONNECTION support is not available.",
             "msDrawWMSLayer()");
  return(MS_FAILURE);

#endif /* USE_WMS_LYR */

}

/**********************************************************************
 *                          msDrawWMSLayerLow()
 *
 **********************************************************************/

int msDrawWMSLayerLow(int nLayerId, httpRequestObj *pasReqInfo,
                      int numRequests, mapObj *map, layerObj *lp, imageObj *img)
{
#ifdef USE_WMS_LYR
  int status = MS_SUCCESS;
  int iReq = -1;
  char szPath[MS_MAXPATHLEN];
  int currenttype;
  int currentconnectiontype;
  int numclasses;
  char *mem_filename = NULL;
  const char *pszTmp;
  int bIsEssential = MS_FALSE;

  /* ------------------------------------------------------------------
   * Sometimes a requested layer is essential for the map, so if the
   * request fails or an error is delivered, the map has not to be drawn
   * ------------------------------------------------------------------ */
  if ((pszTmp = msOWSLookupMetadata(&(lp->metadata),
                                    "MO", "essential")) != NULL) {
    if( strcasecmp(pszTmp,"true") == 0
        || strcasecmp(pszTmp,"on") == 0
        || strcasecmp(pszTmp,"yes") == 0 )
      bIsEssential = MS_TRUE;
    else
      bIsEssential = atoi(pszTmp);      
  }

  /* ------------------------------------------------------------------
   * Find the request info for this layer in the array, based on nLayerId
   * ------------------------------------------------------------------ */
  for(iReq=0; iReq<numRequests; iReq++) {
    if (pasReqInfo[iReq].nLayerId == nLayerId)
      break;
  }

  if (iReq == numRequests) {
    /* This layer was skipped or was included in a multi-layers
     * request ... nothing to do.
     */
    return MS_SUCCESS;
  }

  if ( !MS_HTTP_SUCCESS( pasReqInfo[iReq].nStatus ) ) {
    /* ====================================================================
          Failed downloading layer... we log an error but we still return
          SUCCESS here so that the layer is only skipped instead of aborting
          the whole draw map.
          If the layer is essential the map is not to be drawn.
     ==================================================================== */
    msSetError(MS_WMSERR,
               "WMS GetMap request failed for layer '%s' (Status %d: %s).",
               "msDrawWMSLayerLow()",
               (lp->name?lp->name:"(null)"),
               pasReqInfo[iReq].nStatus, pasReqInfo[iReq].pszErrBuf );

    if (!bIsEssential)
      return MS_SUCCESS;
    else
      return MS_FAILURE;
  }

  /* ------------------------------------------------------------------
   * Check the Content-Type of the response to see if we got an exception,
   * if yes then try to parse it and pass the info to msSetError().
   * We log an error but we still return SUCCESS here so that the layer
   * is only skipped instead of aborting the whole draw map.
   * If the layer is essential the map is not to be drawn.
   * ------------------------------------------------------------------ */
  if (pasReqInfo[iReq].pszContentType &&
      (strcmp(pasReqInfo[iReq].pszContentType, "text/xml") == 0 ||
       strcmp(pasReqInfo[iReq].pszContentType, "application/vnd.ogc.se_xml") == 0)) {
    FILE *fp;
    char szBuf[MS_BUFFER_LENGTH];

    if( pasReqInfo[iReq].pszOutputFile ) {
      fp = fopen(pasReqInfo[iReq].pszOutputFile, "r");
      if (fp) {
        /* TODO: For now we'll only read the first chunk and return it
         * via msSetError()... we should really try to parse the XML
         * and extract the exception code/message though
         */
        size_t nSize;

        nSize = fread(szBuf, sizeof(char), MS_BUFFER_LENGTH-1, fp);
        if (nSize < MS_BUFFER_LENGTH)
          szBuf[nSize] = '\0';
        else {
          strlcpy(szBuf, "(!!!)", sizeof(szBuf)); /* This should never happen */
        }

        fclose(fp);

        /* We're done with the remote server's response... delete it. */
        if (!lp->debug)
          unlink(pasReqInfo[iReq].pszOutputFile);
      } else {
        strlcpy(szBuf, "(Failed to open exception response)", sizeof(szBuf));
      }
    } else {
      strlcpy( szBuf, pasReqInfo[iReq].result_data, MS_BUFFER_LENGTH );
    }

    if (lp->debug)
      msDebug("WMS GetMap request got XML exception for layer '%s': %s.",
              (lp->name?lp->name:"(null)"), szBuf );

    msSetError(MS_WMSERR,
               "WMS GetMap request got XML exception for layer '%s': %s.",
               "msDrawWMSLayerLow()",
               (lp->name?lp->name:"(null)"), szBuf );

    if (!bIsEssential)
      return MS_SUCCESS;
    else
      return MS_FAILURE;
  }

  /* ------------------------------------------------------------------
   * If the output was written to a memory buffer, then we will need
   * to attach a "VSI" name to this buffer.
   * ------------------------------------------------------------------ */
  if( pasReqInfo[iReq].pszOutputFile == NULL ) {
    mem_filename = msTmpFile(map, NULL, "/vsimem/msout/", "img.tmp" );

    VSIFCloseL(
      VSIFileFromMemBuffer( mem_filename,
                            (GByte*) pasReqInfo[iReq].result_data,
                            (vsi_l_offset) pasReqInfo[iReq].result_size,
                            FALSE ) );
  }

  /* ------------------------------------------------------------------
   * Prepare layer for drawing, reprojecting the image received from the
   * server if needed...
   * ------------------------------------------------------------------ */
  /* keep the current type that will be restored at the end of this  */
  /* function. */
  currenttype = lp->type;
  currentconnectiontype = lp->connectiontype;
  lp->type = MS_LAYER_RASTER;
  lp->connectiontype = MS_SHAPEFILE;

  /* set the classes to 0 so that It won't do client side */
  /* classification if an sld was set. */
  numclasses = lp->numclasses;

  /* ensure the file connection is closed right away after the layer */
  /* is rendered */
  msLayerSetProcessingKey( lp, "CLOSE_CONNECTION", "NORMAL");

  if (msOWSLookupMetadata(&(lp->metadata), "MO", "sld_body") ||
      msOWSLookupMetadata(&(lp->metadata), "MO", "sld_url"))
    lp->numclasses = 0;

  if (lp->data) free(lp->data);
  if( mem_filename != NULL )
    lp->data = mem_filename;
  else
    lp->data =  msStrdup(pasReqInfo[iReq].pszOutputFile);

  /* #3138 If PROCESSING "RESAMPLE=..." is set we cannot use the simple case */
  if (!msProjectionsDiffer(&(map->projection), &(lp->projection)) &&
      (msLayerGetProcessingKey(lp, "RESAMPLE") == NULL) ) {
    /* The simple case... no reprojection needed... render layer directly. */
    lp->transform = MS_FALSE;
    /* if (msDrawRasterLayerLow(map, lp, img) != 0) */
    /* status = MS_FAILURE; */
    if (msDrawLayer(map, lp, img) != 0)
      status = MS_FAILURE;
  } else {
    VSILFILE *fp;
    char *wldfile;
    /* OK, we have to resample the raster to map projection... */
    lp->transform = MS_TRUE;
    msLayerSetProcessingKey( lp, "LOAD_WHOLE_IMAGE", "YES" );

    /* Create a world file with raster extents */
    /* One line per value, in this order: cx, 0, 0, cy, ulx, uly */
    wldfile = msBuildPath(szPath, lp->map->mappath, lp->data);
    if (wldfile && (strlen(wldfile)>=3))
      strcpy(wldfile+strlen(wldfile)-3, "wld");
    if (wldfile && (fp = VSIFOpenL(wldfile, "wt")) != NULL) {
      double dfCellSizeX = MS_OWS_CELLSIZE(pasReqInfo[iReq].bbox.minx,
                                           pasReqInfo[iReq].bbox.maxx,
                                           pasReqInfo[iReq].width);
      double dfCellSizeY = MS_OWS_CELLSIZE(pasReqInfo[iReq].bbox.maxy,
                                           pasReqInfo[iReq].bbox.miny,
                                           pasReqInfo[iReq].height);
      char world_text[5000];

      sprintf( world_text, "%.12f\n0\n0\n%.12f\n%.12f\n%.12f\n",
               dfCellSizeX,
               dfCellSizeY,
               pasReqInfo[iReq].bbox.minx + dfCellSizeX * 0.5,
               pasReqInfo[iReq].bbox.maxy + dfCellSizeY * 0.5 );

      VSIFWriteL( world_text, 1, strlen(world_text), fp );
      VSIFCloseL( fp );

      /* GDAL should be called to reproject automatically. */
      if (msDrawLayer(map, lp, img) != 0)
        status = MS_FAILURE;

      if (!lp->debug || mem_filename != NULL)
        VSIUnlink( wldfile );
    } else {
      msSetError(MS_WMSCONNERR,
                 "Unable to create wld file for WMS slide.",
                 "msDrawWMSLayer()");
      status = MS_FAILURE;
    }

  }

  /* We're done with the remote server's response... delete it. */
  if (!lp->debug || mem_filename != NULL)
    VSIUnlink(lp->data);

  /* restore prveious type */
  lp->type = currenttype;
  lp->connectiontype = currentconnectiontype;

  /* restore previous numclasses */
  lp->numclasses = numclasses;

  free(lp->data);
  lp->data = NULL;

  return status;

#else
  /* ------------------------------------------------------------------
   * WMS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WMSCONNERR, "WMS CLIENT CONNECTION support is not available.",
             "msDrawWMSLayer()");
  return(MS_FAILURE);

#endif /* USE_WMS_LYR */

}


int msWMSLayerExecuteRequest(mapObj *map, int nOWSLayers, int nClickX, int nClickY,
                             int nFeatureCount, const char *pszInfoFormat, int type)
{
#ifdef USE_WMS_LYR

  msIOContext *context;

  httpRequestObj *pasReqInfo;
  wmsParamsObj sLastWMSParams;
  int i, numReq = 0;

  pasReqInfo = (httpRequestObj *)msSmallMalloc((nOWSLayers+1)*sizeof(httpRequestObj));
  msHTTPInitRequestObj(pasReqInfo, nOWSLayers+1);
  msInitWmsParamsObj(&sLastWMSParams);

  /* Generate the http request */
  for (i=0; i<map->numlayers; i++) {
    if (GET_LAYER(map,map->layerorder[i])->status == MS_ON) {
      if (type == WMS_GETFEATUREINFO ) {
        if( msPrepareWMSLayerRequest(map->layerorder[i], map, GET_LAYER(map,map->layerorder[i]),
                                   WMS_GETFEATUREINFO,
                                   MS_WMS, &sLastWMSParams,
                                   nClickX, nClickY, nFeatureCount, pszInfoFormat,
                                   pasReqInfo, &numReq) == MS_FAILURE) {
          msFreeWmsParamsObj(&sLastWMSParams);
          msFree(pasReqInfo);
          return MS_FAILURE;
        }
      } else if (msPrepareWMSLayerRequest(map->layerorder[i], map, GET_LAYER(map,map->layerorder[i]),
                                          WMS_GETLEGENDGRAPHIC,
                                          MS_WMS, &sLastWMSParams,
                                          0, 0, 0, NULL,
                                          pasReqInfo, &numReq) == MS_FAILURE) {
        msFreeWmsParamsObj(&sLastWMSParams);
        msFree(pasReqInfo);
        return MS_FAILURE;
      }
    }
  }

  if (msOWSExecuteRequests(pasReqInfo, numReq, map, MS_FALSE) == MS_FAILURE) {
    msHTTPFreeRequestObj(pasReqInfo, numReq);
    msFree(pasReqInfo);
    msFreeWmsParamsObj(&sLastWMSParams);
    return MS_FAILURE;
  }

  context = msIO_getHandler( stdout );
  if( context == NULL ) {
    msHTTPFreeRequestObj(pasReqInfo, numReq);
    msFree(pasReqInfo);
    msFreeWmsParamsObj(&sLastWMSParams);
    return MS_FAILURE;
  }

  msIO_printf("Content-Type: %s%c%c",pasReqInfo[0].pszContentType, 10,10);

  if( pasReqInfo[0].pszOutputFile ) {
    FILE *fp;
    char szBuf[MS_BUFFER_LENGTH];

    fp = fopen(pasReqInfo[0].pszOutputFile, "r");
    if (fp) {
      while(1) {
        size_t nSize;
        nSize = fread(szBuf, sizeof(char), MS_BUFFER_LENGTH-1, fp);
        if (nSize > 0)
          msIO_contextWrite( context,
                             szBuf,
                             nSize);
        if (nSize != MS_BUFFER_LENGTH-1)
          break;
      }
      fclose(fp);
      if (!map->debug)
        unlink(pasReqInfo[0].pszOutputFile);
    } else {
      msSetError(MS_IOERR, "'%s'.",
                 "msWMSLayerExecuteRequest()", pasReqInfo[0].pszOutputFile);
      return MS_FAILURE;
    }
  } else {
    msIO_contextWrite( context,
                       pasReqInfo[0].result_data,
                       pasReqInfo[0].result_size );
  }

  msHTTPFreeRequestObj(pasReqInfo, numReq);
  msFree(pasReqInfo);
  msFreeWmsParamsObj(&sLastWMSParams);

  return MS_SUCCESS;
#else
  /* ------------------------------------------------------------------
   * WMS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WMSCONNERR, "WMS CLIENT CONNECTION support is not available.",
             "msWMSLayerExecuteRequest()");
  return(MS_FAILURE);

#endif /* USE_WMS_LYR */

}

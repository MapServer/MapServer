/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of WMS CONNECTIONTYPE - client to WMS servers
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2001-2004, Daniel Morissette, DM Solutions Group Inc
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
 * Revision 1.64  2004/04/05 21:57:06  assefa
 * Fix bug in msWMSGetFeatureInfoURL not passing a wmsparams structure to the
 * msBuildWMSLayerURL.
 *
 * Revision 1.63  2004/03/30 15:44:20  dan
 * Fixed leak of pszURL when merging multiple WMS layers
 *
 * Revision 1.62  2004/03/30 14:55:44  dan
 * Added wms_force_separate_request metadata to prevent a WMS layer from
 * being included in multi-layer request.
 *
 * Revision 1.61  2004/03/30 00:12:28  dan
 * Added ability to combine multiple WMS connections to the same server
 * into a single request when the layers are adjacent and compatible.(bug 116)
 *
 * Revision 1.60  2004/03/25 21:59:23  sean
 * Undoing accidental changes to mapwmslayer.c
 *
 * Revision 1.58  2004/02/13 15:28:56  assefa
 * Use wms_sld_url to send sld request.
 *
 * Revision 1.57  2004/02/11 20:45:38  assefa
 * When generating the SLD, generate only for the layer and not the whole map.
 *
 * Revision 1.56  2004/02/02 20:47:39  assefa
 * Use of wms_sld_body url.
 * Make sure that the layer type is preserved after a msDrawWMSLayerLow call.
 *
 * Revision 1.55  2004/01/08 20:44:21  assefa
 * Add #ifdef for windows to be able to use snprintf.
 *
 * Revision 1.54  2003/11/03 15:45:29  assefa
 * Do not delete the temporary file comming from the server if we are in debug
 * mode. (DEBUG ON on the layer)
 *
 * ...
 *
 * Revision 1.1  2001/08/14 21:26:54  dan
 * Initial revision - only loads and draws WMS CONNECTION URL for now.
 *
 **********************************************************************/

#include "map.h"
#include "maperror.h"
#include "mapogcsld.h"

#include <time.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#include <stdio.h>
#endif

#define WMS_V_1_0_0  100
#define WMS_V_1_0_7  107
#define WMS_V_1_1_0  110


/**********************************************************************
 *                          msInitWmsParamsObj()
 *
 **********************************************************************/
int msInitWmsParamsObj(wmsParamsObj *wmsparams) 
{
    wmsparams->onlineresource = NULL;
    wmsparams->params = msCreateHashTable();
    wmsparams->numparams=0;

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

    wmsparams->numparams=0;
}

/**********************************************************************
 *                          msSetWMSParamString()
 *
 **********************************************************************/
static int msSetWMSParamString(wmsParamsObj *psWMSParams, 
                               const char *name, const char * value,
                               int urlencode) 
{
    if (urlencode)
    {
        char *pszTmp;
        pszTmp = msEncodeUrl(value);
        msInsertHashTable(psWMSParams->params, name, pszTmp);
        msFree(pszTmp);
    }
    else
    {
        msInsertHashTable(psWMSParams->params, name, value);
    }
    psWMSParams->numparams++;

    return MS_SUCCESS;
}

/**********************************************************************
 *                          msSetWMSParamInt()
 *
 **********************************************************************/
static int msSetWMSParamInt(wmsParamsObj *wmsparams, 
                            const char *name, int value) 
{
    char szBuf[100];

    snprintf(szBuf, 100, "%d", value);
    msInsertHashTable(wmsparams->params, name, szBuf);
    wmsparams->numparams++;

    return MS_SUCCESS;
}

/**********************************************************************
 *                          msBuildWMSParamsUrl()
 *
 **********************************************************************/
static char *msBuildURLFromWMSParams(wmsParamsObj *wmsparams) 
{
    const char *key, *value;
    int nLen;
    char *pszURL;

    /* Compute size required for URL buffer 
     */
    nLen = strlen(wmsparams->onlineresource) + 3;

    key = msFirstKeyFromHashTable(wmsparams->params);
    while (key != NULL)
    {
        value = msLookupHashTable(wmsparams->params, key);
        nLen += strlen(key) + strlen(value) + 2;

        key = msNextKeyFromHashTable(wmsparams->params, key);
    }

    pszURL = (char*)malloc((nLen+1)*sizeof(char*));

    /* Start with the onlineresource value and append trailing '?' or '&' 
     * if missing.
     */
    strcpy(pszURL, wmsparams->onlineresource);
    if (strchr(pszURL, '?') == NULL)
        strcat(pszURL, "?");
    else
    {
        char *c;
        c = pszURL+strlen(pszURL)-1;
        if (*c != '?' && *c != '&')
            strcpy(c+1, "&");
    }

    /* Now add all the parameters 
     */
    nLen = strlen(pszURL);
    key = msFirstKeyFromHashTable(wmsparams->params);
    while (key != NULL)
    {
        value = msLookupHashTable(wmsparams->params, key);
        sprintf(pszURL+nLen, "%s=%s&", key, value);
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
                                  wmsParamsObj *psWMSParams) 
{
    const char *pszOnlineResource, *pszVersion, *pszName, *pszFormat;
    const char *pszFormatList, *pszStyle, *pszStyleList, *pszTime;
    const char *pszSLD=NULL, *pszVersionKeyword=NULL;
    const char *pszSLDBody=NULL;
    char *pszSLDGenerated = NULL, *pszSLDURL = NULL;

    /* If lp->connection is not set then use wms_onlineresource metadata */
    pszOnlineResource = lp->connection;
    if (pszOnlineResource == NULL) 
      pszOnlineResource = msLookupHashTable(lp->metadata,"wms_onlineresource");

    pszVersion =        msLookupHashTable(lp->metadata, "wms_server_version");
    pszName =           msLookupHashTable(lp->metadata, "wms_name");
    pszFormat =         msLookupHashTable(lp->metadata, "wms_format");
    pszFormatList =     msLookupHashTable(lp->metadata, "wms_formatlist");
    pszStyle =          msLookupHashTable(lp->metadata, "wms_style");
    pszStyleList =      msLookupHashTable(lp->metadata, "wms_stylelist");
    pszTime =           msLookupHashTable(lp->metadata, "wms_time");
    pszSLDBody =        msLookupHashTable(lp->metadata, "wms_sld_body");
    pszSLDURL =         msLookupHashTable(lp->metadata, "wms_sld_url");

    if (pszOnlineResource==NULL || pszVersion==NULL || pszName==NULL)
    {
        msSetError(MS_WMSCONNERR, 
                   "One of wms_onlineresource, wms_server_version, wms_name "
                   "metadata is missing in layer %s.  "
                   "Please either provide a valid CONNECTION URL, or provide "
                   "those values in the layer's metadata.\n", 
                   "msBuildWMSLayerURLBase()", lp->name);
        return MS_FAILURE;
    }

    psWMSParams->onlineresource = strdup(pszOnlineResource);

    if (strncmp(pszVersion, "1.0.7", 5) < 0)
        pszVersionKeyword = "WMTVER";
    else
        pszVersionKeyword = "VERSION";

    msSetWMSParamString(psWMSParams, pszVersionKeyword, pszVersion, MS_FALSE);
    msSetWMSParamString(psWMSParams, "SERVICE", "WMS",     MS_FALSE);
    msSetWMSParamString(psWMSParams, "LAYERS",  pszName,   MS_TRUE);
    msSetWMSParamString(psWMSParams, "TRANSPARENT", "TRUE",MS_FALSE);


    if (pszFormat==NULL && pszFormatList==NULL)
    {
        msSetError(MS_WMSCONNERR, 
                   "At least wms_format or wms_formatlist is required for "
                   "layer %s.  "
                   "Please either provide a valid CONNECTION URL, or provide "
                   "those values in the layer's metadata.\n", 
                   "msBuildWMSLayerURLBase()", lp->name);
        return MS_FAILURE;
    }

    if (pszFormat != NULL)
    {
        msSetWMSParamString(psWMSParams, "FORMAT",  pszFormat, MS_TRUE);
    }
    else
    {
        /* Look for the first format in list that matches */
        char **papszTok;
        int i, n;
        papszTok = split(pszFormatList, ',', &n);

        for(i=0; pszFormat==NULL && i<n; i++)
        {
            if (0 
#ifdef USE_GD_GIF
                || strcasecmp(papszTok[i], "GIF")
                || strcasecmp(papszTok[i], "image/gif")
#endif
#ifdef USE_GD_PNG
                || strcasecmp(papszTok[i], "PNG")
                || strcasecmp(papszTok[i], "image/png")
#endif
#ifdef USE_GD_JPEG
                || strcasecmp(papszTok[i], "JPEG")
                || strcasecmp(papszTok[i], "image/jpeg")
#endif
#ifdef USE_GD_WBMP
                || strcasecmp(papszTok[i], "WBMP")
                || strcasecmp(papszTok[i], "image/wbmp")
#endif
                )
            {
                pszFormat = papszTok[i];
            }
        }

        if (pszFormat)
        {
            msSetWMSParamString(psWMSParams, "FORMAT",  pszFormat, MS_TRUE);
            msFreeCharArray(papszTok, n);
        }
        else
        {
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


    if (pszStyle==NULL)
    {
        // When no style is selected, use "" which is a valid default.
        pszStyle = "";
    }
    else
    {
        // Was a wms_style_..._sld URL provided?
        char szBuf[100];
        sprintf(szBuf, "wms_style_%.80s_sld", pszStyle);
        pszSLD = msLookupHashTable(lp->metadata, szBuf);

        if (pszSLD)
        {
            // SLD URL is set.  If this defn. came from a map context then
            // the style name may just be an internal name: "Style{%d}" if
            // that's the case then we should not pass this name via the URL
            if (strncmp(pszStyle, "Style{", 6) == 0)
                pszStyle = "";
        }
    }

    if (pszSLD == NULL)
    {
        // STYLES is mandatory if SLD not set
        msSetWMSParamString(psWMSParams, "STYLES", pszStyle, MS_TRUE);
    }
    else if (strlen(pszStyle) > 0)
    {
        // Both STYLES and SLD are set
        msSetWMSParamString(psWMSParams, "STYLES", pszStyle, MS_TRUE);
        msSetWMSParamString(psWMSParams, "SLD",    pszSLD,   MS_TRUE);
    }
    else
    {
        // Only SLD is set
        msSetWMSParamString(psWMSParams, "SLD",    pszSLD,   MS_TRUE);
    }

    if (msIsLayerQueryable(lp))
    {
        msSetWMSParamString(psWMSParams, "QUERY_LAYERS", pszName, MS_TRUE);
    }
    if (pszTime && strlen(pszTime) > 0)
    {
        msSetWMSParamString(psWMSParams, "TIME",   pszTime,  MS_TRUE);
    }

    /* if  the metadata wms_sld_body is set to AUTO, we generate
     * the sld based on classes found in the map file and send
     * it in the URL. If diffrent from AUTO, we are assuming that
     * it is a valid sld.
     */
    if (pszSLDBody)
    {
        if (strcasecmp(pszSLDBody, "AUTO") == 0)
        {
            pszSLDGenerated = msSLDGenerateSLD(map, lp->index);
            if (pszSLDGenerated)
            {
                msSetWMSParamString(psWMSParams, "SLD_BODY", 
                                    pszSLDGenerated, MS_TRUE);
                free(pszSLDGenerated);
            }
        }
        else
        {
            msSetWMSParamString(psWMSParams, "SLD_BODY", pszSLDBody, MS_TRUE);
        }

    }
    
    if (pszSLDURL)
    {
        msSetWMSParamString(psWMSParams, "SLD", pszSLDURL, MS_TRUE);
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
#define WMS_GETMAP         1
#define WMS_GETFEATUREINFO 2

int msBuildWMSLayerURL(mapObj *map, layerObj *lp, int nRequestType,
                       int nClickX, int nClickY, int nFeatureCount,
                       const char *pszInfoFormat, rectObj *bbox_ret,
                       wmsParamsObj *psWMSParams)
{
#ifdef USE_WMS_LYR
    char *pszEPSG = NULL;
    const char *pszVersion, *pszTmp, *pszRequestParam, *pszExceptionsParam;
    rectObj bbox;
    int nVersion;
    
    if (lp->connectiontype != MS_WMS)
    {
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
         (pszVersion = strstr(lp->connection, "wmtver=")) == NULL ) )
    {
        // CONNECTION missing or seems incomplete... try to build from metadata
        if (msBuildWMSLayerURLBase(map, lp, psWMSParams) != MS_SUCCESS)
            return MS_FAILURE;  // An error already produced.

        // If we received MS_SUCCESS then version must have been set
        pszVersion = msLookupHashTable(psWMSParams->params, "VERSION");
        if (pszVersion ==NULL)
            pszVersion = msLookupHashTable(psWMSParams->params, "WMTVER");

    }
    else
    {
        // CONNECTION string seems complete, start with that.
        psWMSParams->onlineresource = strdup(lp->connection);
        pszVersion = strchr(pszVersion, '=')+1;
    }

    if (strncmp(pszVersion, "1.0.8", 5) >= 0)    /* 1.0.8 == 1.1.0 */
        nVersion = WMS_V_1_1_0;
    else if (strncmp(pszVersion, "1.0.7", 5) >= 0)
        nVersion = WMS_V_1_0_7;
    else if (strncmp(pszVersion, "1.0.0", 5) >= 0)
        nVersion = WMS_V_1_0_0;
    else
    {
        msSetError(MS_WMSCONNERR, "MapServer supports only WMS 1.0.0 to 1.1.0 (please verify the VERSION parameter in the connection string).", "msBuildWMSLayerURL()");
        return MS_FAILURE;
    }


/* ------------------------------------------------------------------
 * For GetFeatureInfo requests, make sure QUERY_LAYERS is included
 * ------------------------------------------------------------------ */
    if  (nRequestType == WMS_GETFEATUREINFO &&
         strstr(psWMSParams->onlineresource, "QUERY_LAYERS=") == NULL &&
         strstr(psWMSParams->onlineresource, "query_layers=") == NULL &&
         msLookupHashTable(psWMSParams->params, "QUERY_LAYERS") == NULL)
    {
        msSetError(MS_WMSCONNERR, "WMS Connection String must contain the QUERY_LAYERS parameter to support GetFeatureInfo requests (with name in uppercase).", "msBuildWMSLayerURL()");
        return MS_FAILURE;
    }



/* ------------------------------------------------------------------
 * Figure the SRS we'll use for the request.
 * - Fetch the map SRS (if it's EPSG)
 * - Check if map SRS is listed in layer wms_srs metadata
 * - If map SRS is valid for this layer then use it
 * - Otherwise request layer in its default SRS and we'll reproject later
 * ------------------------------------------------------------------ */
    if ((pszEPSG = (char*)msGetEPSGProj(&(map->projection), 
                                           NULL, MS_TRUE)) != NULL &&
        (pszEPSG = strdup(pszEPSG)) != NULL &&
        (strncasecmp(pszEPSG, "EPSG:", 5) == 0 ||
         strncasecmp(pszEPSG, "AUTO:", 5) == 0) )
    {
        const char *pszLyrEPSG, *pszFound;
        int nLen;
        char *pszPtr = NULL;

        // If it's an AUTO projection then keep only id and strip off 
        // the parameters for now (we'll restore them at the end)
        if (strncasecmp(pszEPSG, "AUTO:", 5) == 0)
        {
            if ((pszPtr = strchr(pszEPSG, ',')))
                *pszPtr = '\0';
        }

        nLen = strlen(pszEPSG);

        pszLyrEPSG = msGetEPSGProj(&(lp->projection), lp->metadata, MS_FALSE);

        if (pszLyrEPSG == NULL ||
            (pszFound = strstr(pszLyrEPSG, pszEPSG)) == NULL ||
            ! ((*(pszFound+nLen) == '\0') || isspace(*(pszFound+nLen))) )
        {
            // Not found in Layer's list of SRS (including projection object)
            free(pszEPSG);
            pszEPSG = NULL;
        }
        if (pszEPSG && pszPtr)
            *pszPtr = ',';  // Restore full AUTO:... definition
    }

    if (pszEPSG == NULL &&
        ((pszEPSG = (char*)msGetEPSGProj(&(lp->projection), 
                                            lp->metadata, MS_TRUE)) == NULL ||
         (pszEPSG = strdup(pszEPSG)) == NULL ||
         (strncasecmp(pszEPSG, "EPSG:", 5) != 0 &&
          strncasecmp(pszEPSG, "AUTO:", 5) != 0 ) ) )
    {
        msSetError(MS_WMSCONNERR, "Layer must have an EPSG or AUTO projection code (in its PROJECTION object or wms_srs metadata)", "msBuildWMSLayerURL()");
        if (pszEPSG) free(pszEPSG);
        return MS_FAILURE;
    }

/* ------------------------------------------------------------------
 * For an AUTO projection, set the Units,lon0,lat0 if not already set
 * ------------------------------------------------------------------ */
    if (strncasecmp(pszEPSG, "AUTO:", 5) == 0 &&
        strchr(pszEPSG, ',') == NULL)
    {
        pointObj oPoint;
        char *pszNewEPSG;

        // Use center of map view for lon0,lat0
        oPoint.x = (map->extent.minx + map->extent.maxx)/2.0;
        oPoint.y = (map->extent.miny + map->extent.maxy)/2.0;
        msProjectPoint(&(map->projection), &(map->latlon), &oPoint);

        pszNewEPSG = (char*)malloc(101*sizeof(char));

        snprintf(pszNewEPSG, 100, "%s,9001,%.16g,%.16g", 
                 pszEPSG, oPoint.x, oPoint.y);
        pszNewEPSG[100]='\0';
        free(pszEPSG);
        pszEPSG=pszNewEPSG;
    }

/* ------------------------------------------------------------------
 * Set layer SRS and reproject map extents to the layer's SRS
 * ------------------------------------------------------------------ */
    // No need to set lp->proj if it's already set to the right EPSG code
    if ((pszTmp = msGetEPSGProj(&(lp->projection), NULL, MS_TRUE)) == NULL ||
        strcasecmp(pszEPSG, pszTmp) != 0)
    {
        if (strncasecmp(pszEPSG, "EPSG:", 5) == 0)
        {
            char szProj[20];
            sprintf(szProj, "init=epsg:%s", pszEPSG+5);
            if (msLoadProjectionString(&(lp->projection), szProj) != 0)
                return MS_FAILURE;
        }
        else
        {
            if (msLoadProjectionString(&(lp->projection), pszEPSG) != 0)
                return MS_FAILURE;
        }
    }

    bbox = map->extent;
    if (msProjectionsDiffer(&(map->projection), &(lp->projection)))
    {
        msProjectRect(&(map->projection), &(lp->projection), &bbox);
    }

    if (bbox_ret != NULL)
        *bbox_ret = bbox;

/* ------------------------------------------------------------------
 * Build the request URL.
 * At this point we set only the following parameters for GetMap:
 *   REQUEST
 *   SRS
 *   BBOX
 *
 * And for GetFeatureInfo:
 *   X
 *   Y
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
 *   QUERY_LAYERS (for queriable layers only)
 * ------------------------------------------------------------------ */

    if (nRequestType == WMS_GETFEATUREINFO)
    {
        char szBuf[100] = "";

        if (nVersion >= WMS_V_1_0_7)
            pszRequestParam = "GetFeatureInfo";
        else
            pszRequestParam = "feature_info";

        if (nVersion >= WMS_V_1_1_0)
            pszExceptionsParam = "application/vnd.ogc.se_xml";
        else if (nVersion > WMS_V_1_1_0)  /* 1.0.1 to 1.0.7 */
            pszExceptionsParam = "SE_XML";
        else
            pszExceptionsParam = "WMS_XML";

        msSetWMSParamString(psWMSParams, "REQUEST", pszRequestParam, MS_FALSE);
        msSetWMSParamInt(   psWMSParams, "WIDTH",   map->width);
        msSetWMSParamInt(   psWMSParams, "HEIGHT",  map->height);
        msSetWMSParamString(psWMSParams, "SRS",     pszEPSG, MS_FALSE);

        snprintf(szBuf, 100, "%f,%f,%f,%f", 
                 bbox.minx, bbox.miny, bbox.maxx, bbox.maxy);
        msSetWMSParamString(psWMSParams, "BBOX",    szBuf, MS_TRUE);
 
        msSetWMSParamInt(   psWMSParams, "X",       nClickX);
        msSetWMSParamInt(   psWMSParams, "Y",       nClickY);
 
        msSetWMSParamString(psWMSParams, "EXCEPTIONS", pszExceptionsParam, MS_FALSE);
        msSetWMSParamString(psWMSParams, "INFO_FORMAT",pszInfoFormat, MS_TRUE);

        // If FEATURE_COUNT <= 0 then don't pass this parameter
        // The spec states that FEATURE_COUNT must be greater than zero
        // and if not passed then the behavior is up to the server
        if (nFeatureCount > 0)
        {
            msSetWMSParamInt(psWMSParams, "FEATURECOUNT", nFeatureCount);
        }

    }
    else /* if (nRequestType == WMS_GETMAP) */
    {
        char szBuf[100] = "";

        if (nVersion >= WMS_V_1_0_7)
            pszRequestParam = "GetMap";
        else
            pszRequestParam = "map";

        if (nVersion >= WMS_V_1_1_0)
            pszExceptionsParam = "application/vnd.ogc.se_inimage";
        else
            pszExceptionsParam = "INIMAGE";

        msSetWMSParamString(psWMSParams, "REQUEST", pszRequestParam, MS_FALSE);
        msSetWMSParamInt(   psWMSParams, "WIDTH",   map->width);
        msSetWMSParamInt(   psWMSParams, "HEIGHT",  map->height);
        msSetWMSParamString(psWMSParams, "SRS",     pszEPSG, MS_FALSE);

        snprintf(szBuf, 100, "%f,%f,%f,%f", 
                 bbox.minx, bbox.miny, bbox.maxx, bbox.maxy);
        msSetWMSParamString(psWMSParams, "BBOX",    szBuf, MS_TRUE);
        msSetWMSParamString(psWMSParams, "EXCEPTIONS",  pszExceptionsParam, MS_FALSE);
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
                           pszInfoFormat, NULL, &sThisWMSParams)!= MS_SUCCESS)
    {
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
                             enum MS_CONNECTION_TYPE lastconnectiontype,
                             wmsParamsObj *psLastWMSParams,
                             httpRequestObj *pasReqInfo, int *numRequests) 
{
#ifdef USE_WMS_LYR
    char *pszURL = NULL;
    const char *pszTmp;
    rectObj bbox;
    int nTimeout, bOkToMerge, bForceSeparateRequest;
    wmsParamsObj sThisWMSParams;

    if (lp->connectiontype != MS_WMS)
        return MS_FAILURE;

    msInitWmsParamsObj(&sThisWMSParams);

/* ------------------------------------------------------------------
 * Build the request URL, this will also set layer projection and
 * compute BBOX in that projection.
 * ------------------------------------------------------------------ */
    if ( msBuildWMSLayerURL(map, lp, WMS_GETMAP,
                            0, 0, 0, NULL, &bbox, 
                            &sThisWMSParams) != MS_SUCCESS)
    {
        /* an error was already reported. */
        msFreeWmsParamsObj(&sThisWMSParams);
        return MS_FAILURE;
    }

/* ------------------------------------------------------------------
 * Check if layer overlaps current view window (using wms_latlonboundingbox)
 * ------------------------------------------------------------------ */
    if ((pszTmp = msLookupHashTable(lp->metadata, 
                                    "wms_latlonboundingbox")) != NULL)
    {
        char **tokens;
        int n;
        rectObj ext;

        tokens = split(pszTmp, ' ', &n);
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

        // Reproject latlonboundingbox to the selected SRS for the layer and
        // check if it overlaps the bbox that we calculated for the request

        msProjectRect(&(map->latlon), &(lp->projection), &ext);
        if (!msRectOverlap(&bbox, &ext))
        {
            // No overlap... nothing to do

            msFreeWmsParamsObj(&sThisWMSParams);
            return MS_SUCCESS;  // No overlap.
        }
    }

/* ------------------------------------------------------------------
 * check to see if a the metadata wms_connectiontimeout is set. If it is 
 * the case we will use it, else we use the default which is 30 seconds.
 * First check the metadata in the layer object and then in the map object.
 * ------------------------------------------------------------------ */
    nTimeout = 30;  // Default is 30 seconds 
    if ((pszTmp = msLookupHashTable(lp->metadata, 
                                    "wms_connectiontimeout")) != NULL)
    {
        nTimeout = atoi(pszTmp); 
    }
    else if ((pszTmp = msLookupHashTable(map->web.metadata, 
                                         "wms_connectiontimeout")) != NULL)
    {
        nTimeout = atoi(pszTmp);
    }

/* ------------------------------------------------------------------
 * Check if layer can be merged with previous WMS layer requests
 * Metadata wms_force_separate_request can be set to 1 to prevent this
 * this layer from being combined with any other layer.
 * ------------------------------------------------------------------ */
    bForceSeparateRequest = MS_FALSE;
    if ((pszTmp = msLookupHashTable(lp->metadata, 
                                    "wms_force_separate_request")) != NULL)
    {
        bForceSeparateRequest = atoi(pszTmp); 
    }
    bOkToMerge = MS_FALSE;
    if (!bForceSeparateRequest &&
        lastconnectiontype == MS_WMS &&
        psLastWMSParams != NULL &&
        sThisWMSParams.numparams == psLastWMSParams->numparams &&
        strcmp(sThisWMSParams.onlineresource, 
               psLastWMSParams->onlineresource) == 0)
    {
        const char *key, *value1, *value2;
        bOkToMerge = MS_TRUE;

        key = msFirstKeyFromHashTable(sThisWMSParams.params);
        while (key != NULL && bOkToMerge == MS_TRUE)
        {
            // Skip parameters whose values can be different
            if (!(strcmp(key, "LAYERS") == 0 ||
                  strcmp(key, "QUERY_LAYERS") == 0 ||
                  strcmp(key, "STYLES") == 0) )
            {
                value1 = msLookupHashTable(psLastWMSParams->params, key);
                value2 = msLookupHashTable(sThisWMSParams.params, key);

                if (value1==NULL || value2==NULL ||
                    strcmp(value1, value2) != 0)
                {
                    bOkToMerge = MS_FALSE;
                    break;
                }
            }
            key = msNextKeyFromHashTable(sThisWMSParams.params, key);
        }
    }

    if (bOkToMerge)
    {
        /* Merge both requests into sThisWMSParams
         */
        const char *value1, *value2;
        char *keys[] = {"LAYERS", "QUERY_LAYERS", "STYLES"};
        int i;

        for(i=0; i<3; i++)
        {
            value1 = msLookupHashTable(psLastWMSParams->params, keys[i]);
            value2 = msLookupHashTable(sThisWMSParams.params, keys[i]);
            if (value1 && value2)
            {
                char *pszBuf;
                int nLen;

                nLen = strlen(value1) + strlen(value2) +2;
                pszBuf = malloc(nLen * sizeof(char));
                if (pszBuf == NULL)
                {
                    msSetError(MS_MEMERR, NULL, "msPrepareWMSLayerRequest()");
                    return MS_FAILURE;
                }

                sprintf(pszBuf, "%s,%s", value1, value2);
                msSetWMSParamString(&sThisWMSParams, keys[i], pszBuf,MS_FALSE);

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


    if (bOkToMerge && (*numRequests)>0)
    {
/* ------------------------------------------------------------------
 * Update the last request in the array:  (*numRequests)-1
 * ------------------------------------------------------------------ */
        msFree(pasReqInfo[(*numRequests)-1].pszGetUrl);
        pasReqInfo[(*numRequests)-1].pszGetUrl = pszURL;
        pszURL = NULL;
        pasReqInfo[(*numRequests)-1].debug |= lp->debug;
        if (nTimeout > pasReqInfo[(*numRequests)-1].nTimeout)
            pasReqInfo[(*numRequests)-1].nTimeout = nTimeout;
    }
    else
    {
/* ------------------------------------------------------------------
 * Add a request to the array (already preallocated)
 * ------------------------------------------------------------------ */
        pasReqInfo[(*numRequests)].nLayerId = nLayerId;
        pasReqInfo[(*numRequests)].pszGetUrl = pszURL;
        pszURL = NULL;
        // We'll store the remote server's response to a tmp file.
        pasReqInfo[(*numRequests)].pszOutputFile =msTmpFile(map->web.imagepath,
                                                            "img.tmp");
        pasReqInfo[(*numRequests)].nStatus = 0;
        pasReqInfo[(*numRequests)].nTimeout = nTimeout;
        pasReqInfo[(*numRequests)].bbox = bbox;
        pasReqInfo[(*numRequests)].debug = lp->debug;

        (*numRequests)++;
    }


/* ------------------------------------------------------------------
 * Replace contents of psLastWMSParams with sThisWMSParams 
 * unless bForceSeparateRequest is set in which case we make it empty
 * ------------------------------------------------------------------ */
    if (psLastWMSParams)
    {
        msFreeWmsParamsObj(psLastWMSParams);
        if (!bForceSeparateRequest)
            *psLastWMSParams = sThisWMSParams;
        else
            msInitWmsParamsObj(psLastWMSParams);
    }
    else
    {
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
    int numclasses;

/* ------------------------------------------------------------------
 * Find the request info for this layer in the array, based on nLayerId
 * ------------------------------------------------------------------ */
    for(iReq=0; iReq<numRequests; iReq++)
    {
        if (pasReqInfo[iReq].nLayerId == nLayerId)
            break;
    }

    if (iReq == numRequests)
    {
        /* This layer was skipped or was included in a multi-layers 
         * request ... nothing to do.
         */
        return MS_SUCCESS;  
    }

    if ( !MS_HTTP_SUCCESS( pasReqInfo[iReq].nStatus ) )
    {
/* ==================================================================== 
      Failed downloading layer... we log an error but we still return 
      SUCCESS here so that the layer is only skipped intead of aborting
      the whole draw map.
 ==================================================================== */
        msSetError(MS_WMSERR, 
                   "WMS GetMap request failed for layer '%s' (Status %d: %s).",
                   "msDrawWMSLayerLow()", 
                   (lp->name?lp->name:"(null)"), 
                   pasReqInfo[iReq].nStatus, pasReqInfo[iReq].pszErrBuf );

        return MS_SUCCESS;
    }

/* ------------------------------------------------------------------
 * Prepare layer for drawing, reprojecting the image received from the
 * server if needed...
 * ------------------------------------------------------------------ */
    //keep the current type that will be restored at the end of this 
    //function.
    currenttype = lp->type;
    lp->type = MS_LAYER_RASTER;

    //set the classes to 0 so that It won't do client side
    //classification if an sld was set.
    numclasses = lp->numclasses;
    if (msLookupHashTable(lp->metadata, "wms_sld_body") ||
        msLookupHashTable(lp->metadata, "wms_sld_url"))        
      lp->numclasses = 0;

    if (lp->data) free(lp->data);
    lp->data =  strdup(pasReqInfo[iReq].pszOutputFile);

    if (!msProjectionsDiffer(&(map->projection), &(lp->projection)))
    {
        // The simple case... no reprojection needed... render layer directly.
        lp->transform = MS_FALSE;
        if (msDrawRasterLayerLow(map, lp, img) != 0)
            status = MS_FAILURE;
    }
    else
    {
        FILE *fp;
        char *wldfile;
        // OK, we have to resample the raster to map projection...
        lp->transform = MS_TRUE;

        // Create a world file with raster extents
        // One line per value, in this order: cx, 0, 0, cy, ulx, uly
        wldfile = msBuildPath(szPath, lp->map->mappath, lp->data);
        if (wldfile)    
            strcpy(wldfile+strlen(wldfile)-3, "wld");
        if (wldfile && (fp = fopen(wldfile, "wt")) != NULL)
        {
            fprintf(fp, "%f\n", MS_CELLSIZE(pasReqInfo[iReq].bbox.minx,
                                            pasReqInfo[iReq].bbox.maxx, 
                                            map->width));
            fprintf(fp, "0\n");
            fprintf(fp, "0\n");
            fprintf(fp, "%f\n", MS_CELLSIZE(pasReqInfo[iReq].bbox.maxy,
                                            pasReqInfo[iReq].bbox.miny, 
                                            map->height));
            fprintf(fp, "%f\n", pasReqInfo[iReq].bbox.minx);
            fprintf(fp, "%f\n", pasReqInfo[iReq].bbox.maxy);
            fclose(fp);

            // GDAL should be called to reproject automatically.
            if (msDrawRasterLayerLow(map, lp, img) != 0)
                status = MS_FAILURE;

            unlink(wldfile);
        }
        else
        {
            msSetError(MS_WMSCONNERR, 
                       "Unable to create wld file for WMS slide.", 
                       "msDrawWMSLayer()");
            status = MS_FAILURE;
        }

    } 

    // We're done with the remote server's response... delete it.
    if (!lp->debug)
      unlink(lp->data);

    //restore prveious type
    lp->type = currenttype;

    //restore previous numclasses
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



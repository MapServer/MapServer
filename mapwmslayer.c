/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of WMS CONNECTIONTYPE - client to WMS servers
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2001-2002, Daniel Morissette, DM Solutions Group Inc
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
 * Revision 1.31  2002/10/09 02:29:03  dan
 * Initial implementation of WFS client support.
 *
 * Revision 1.30  2002/09/20 03:44:07  sdlime
 * Swapped map_path for mappath for consistency.
 *
 * Revision 1.29  2002/09/17 13:08:30  julien
 * Remove all chdir() function and replace them with the new msBuildPath function.
 * This have been done to make MapServer thread safe. (Bug 152)
 *
 * Revision 1.28  2002/07/08 03:46:42  dan
 * Finished changes to download WMS layers in parallel when drawing map
 *
 * Revision 1.27  2002/06/26 03:10:43  dan
 * Modified msGetImages() in preparation for support of multiple requests
 * in parrallel
 *
 * Revision 1.26  2002/06/21 18:33:15  frank
 * added support for IMAGEMODE INT16 and FLOAT
 *
 * Revision 1.25  2002/06/11 13:54:08  frank
 * avoid warning
 *
 * Revision 1.24  2002/05/14 14:07:32  assefa
 * Use of ImageObj to be able to output Vector/Raster beside GD.
 *
 * Revision 1.23  2002/03/13 23:45:22  sdlime
 * Added projection support to the GML output code. Re-shuffled the code 
 * to extract the EPSG values for a layer or map into mapproject.c.
 *
 * Revision 1.22  2002/02/08 21:07:27  sacha
 * Added template support to WMS.
 *
 * Revision 1.21  2002/02/01 00:08:36  sacha
 * Move msTmpFile function from mapwmslayer.c to maputil.c
 *
 * Revision 1.20  2002/01/24 18:31:14  dan
 * Use REQUEST=map instead of REQUEST=Map for WMS 1.0.0 requests.
 *
 * Revision 1.19  2002/01/22 23:00:04  dan
 * Added -DENABLE_STDERR_DEBUG in --enable-debug config option to
 * enable/disable msDebug() output to stderr.  Default is disabled.
 *
 * ...
 *
 * Revision 1.1  2001/08/14 21:26:54  dan
 * Initial revision - only loads and draws WMS CONNECTION URL for now.
 *
 **********************************************************************/

#include "map.h"
#include "maperror.h"

#include <time.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#endif

#define WMS_V_1_0_0  100
#define WMS_V_1_0_7  107
#define WMS_V_1_1_0  110


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

char *msBuildWMSLayerURL(mapObj *map, layerObj *lp, int nRequestType,
                         int nClickX, int nClickY, int nFeatureCount,
                         const char *pszInfoFormat, rectObj *bbox_ret) 
{
#ifdef USE_WMS_LYR
    char *pszURL = NULL, *pszEPSG = NULL;
    const char *pszTmp, *pszRequestParam, *pszExceptionsParam;
    rectObj bbox;
    int nVersion;
    
    if (lp->connectiontype != MS_WMS || lp->connection == NULL)
    {
        msSetError(MS_WMSCONNERR, "Call supported only for CONNECTIONTYPE WMS",
                   "msBuildWMSLayerURL()");
        return NULL;
    }

/* ------------------------------------------------------------------
 * Find out request version
 * ------------------------------------------------------------------ */
    if ((pszTmp = strstr(lp->connection, "VERSION=")) == NULL &&
        (pszTmp = strstr(lp->connection, "version=")) == NULL &&
        (pszTmp = strstr(lp->connection, "WMTVER=")) == NULL &&
        (pszTmp = strstr(lp->connection, "wmtver=")) == NULL )
    {
        msSetError(MS_WMSCONNERR, "WMS Connection String must contain the VERSION or WMTVER parameter (with name in uppercase).", "msBuildWMSLayerURL()");
        return NULL;
    }
    pszTmp = strchr(pszTmp, '=')+1;
    if (strncmp(pszTmp, "1.0.8", 5) >= 0)    /* 1.0.8 == 1.1.0 */
        nVersion = WMS_V_1_1_0;
    else if (strncmp(pszTmp, "1.0.7", 5) >= 0)
        nVersion = WMS_V_1_0_7;
    else if (strncmp(pszTmp, "1.0.0", 5) >= 0)
        nVersion = WMS_V_1_0_0;
    else
    {
        msSetError(MS_WMSCONNERR, "MapServer supports only WMS 1.0.0 to 1.1.0 (please verify the VERSION parameter in the connection string).", "msBuildWMSLayerURL()");
        return NULL;
    }


/* ------------------------------------------------------------------
 * For GetFeatureInfo requests, make sure QUERY_LAYERS is included
 * ------------------------------------------------------------------ */
    if  (nRequestType == WMS_GETFEATUREINFO &&
         strstr(lp->connection, "QUERY_LAYERS=") == NULL &&
         strstr(lp->connection, "query_layers=") == NULL )
    {
        msSetError(MS_WMSCONNERR, "WMS Connection String must contain the QUERY_LAYERS parameter to support GetFeatureInfo requests (with name in uppercase).", "msBuildWMSLayerURL()");
        return NULL;
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
        strncasecmp(pszEPSG, "EPSG:", 5) == 0)
    {
        const char *pszLyrEPSG, *pszFound;
        int nLen;

        nLen = strlen(pszEPSG);

        pszLyrEPSG = msGetEPSGProj(&(lp->projection), lp->metadata, MS_FALSE);

        if (pszLyrEPSG == NULL ||
            (pszFound = strstr(pszLyrEPSG, pszEPSG)) == NULL ||
            ! isspace(*(pszFound+nLen)) )
        {
            // Not found in Layer's list of SRS (including projection object)
            free(pszEPSG);
            pszEPSG = NULL;
        }
    }

    if (pszEPSG == NULL &&
        ((pszEPSG = (char*)msGetEPSGProj(&(lp->projection), 
                                            lp->metadata, MS_TRUE)) == NULL ||
         (pszEPSG = strdup(pszEPSG)) == NULL ||
         strncasecmp(pszEPSG, "EPSG:", 5) != 0) )
    {
        msSetError(MS_WMSCONNERR, "Layer must have an EPSG projection code (in its PROJECTION object or wms_srs metadata)", "msBuildWMSLayerURL()");
        if (pszEPSG) free(pszEPSG);
        return NULL;
    }

/* ------------------------------------------------------------------
 * Set layer SRS and reproject map extents to the layer's SRS
 * ------------------------------------------------------------------ */
    // No need to set lp->proj if it's already set to the right EPSG code
    if ((pszTmp = msGetEPSGProj(&(lp->projection), NULL, MS_TRUE)) == NULL ||
        strcasecmp(pszEPSG, pszTmp) != 0)
    {
        char szProj[20];
        sprintf(szProj, "init=epsg:%s", pszEPSG+5);
        if (msLoadProjectionString(&(lp->projection), szProj) != 0)
            return NULL;
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
    // Make sure we have a big enough buffer for the URL
    if(!(pszURL = (char *)malloc((strlen(lp->connection)+256)*sizeof(char)))) 
    {
        msSetError(MS_MEMERR, NULL, "msBuildWMSLayerURL()");
        return NULL;
    }

    // __TODO__ We have to urlencode each value... especially the BBOX values
    // because if they end up in exponent format (123e+06) the + will be seen
    // as a space by the remote server.

    if (nRequestType == WMS_GETFEATUREINFO)
    {
        char szFeatureCount[30] = "";

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

        // If FEATURE_COUNT <= 0 then don't pass this parameter
        // The spec states that FEATURE_COUNT must be greater than zero
        // and if not passed hten the behavior is up to the server
        if (nFeatureCount > 0)
        {
            sprintf(szFeatureCount, "&FEATURE_COUNT=%d", nFeatureCount);
        }

        sprintf(pszURL, 
                "%s&REQUEST=%s&WIDTH=%d&HEIGHT=%d&SRS=%s&BBOX=%f,%f,%f,%f"
                "&EXCEPTIONS=%s&X=%d&Y=%d&INFO_FORMAT=%s%s",
                lp->connection, pszRequestParam, map->width, map->height, 
                pszEPSG, bbox.minx, bbox.miny, bbox.maxx, bbox.maxy,
                pszExceptionsParam,
                nClickX, nClickY, pszInfoFormat, szFeatureCount);
    }
    else /* if (nRequestType == WMS_GETMAP) */
    {
        if (nVersion >= WMS_V_1_0_7)
            pszRequestParam = "GetMap";
        else
            pszRequestParam = "map";

        if (nVersion >= WMS_V_1_1_0)
            pszExceptionsParam = "application/vnd.ogc.se_inimage";
        else
            pszExceptionsParam = "INIMAGE";

        sprintf(pszURL, 
                "%s&REQUEST=%s&WIDTH=%d&HEIGHT=%d&SRS=%s&BBOX=%f,%f,%f,%f"
                "&EXCEPTIONS=%s",
                lp->connection, pszRequestParam, map->width, map->height, 
                pszEPSG, bbox.minx, bbox.miny, bbox.maxx, bbox.maxy,
                pszExceptionsParam);
    }

    free(pszEPSG);

    return pszURL;

#else
/* ------------------------------------------------------------------
 * WMS CONNECTION Support not included...
 * ------------------------------------------------------------------ */
  msSetError(MS_WMSCONNERR, "WMS CLIENT CONNECTION support is not available.", 
             "msBuildWMSLayerURL()");
  return NULL;

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
    return msBuildWMSLayerURL(map, lp, WMS_GETFEATUREINFO,
                              nClickX, nClickY, nFeatureCount,
                              pszInfoFormat, NULL);
}


/**********************************************************************
 *                          msPrepareWMSLayerRequest()
 *
 **********************************************************************/

int msPrepareWMSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             httpRequestObj *pasReqInfo, int *numRequests) 
{
#ifdef USE_WMS_LYR
    char *pszURL = NULL;
    const char *pszTmp;
    rectObj bbox;
    int nTimeout;

    if (lp->connectiontype != MS_WMS || lp->connection == NULL)
        return MS_FAILURE;

    if((lp->status != MS_ON) && (lp->status != MS_DEFAULT))
        return MS_SUCCESS;

  if(map->scale > 0) {
    // layer scale boundaries should be checked first
    if((lp->maxscale > 0) && (map->scale > lp->maxscale))
      return(MS_SUCCESS);
    if((lp->minscale > 0) && (map->scale <= lp->minscale))
      return(MS_SUCCESS);
  }

/* ------------------------------------------------------------------
 * Build the request URL, this will also set layer projection and
 * compute BBOX in that projection.
 * ------------------------------------------------------------------ */
    if ((pszURL = msBuildWMSLayerURL(map, lp, WMS_GETMAP,
                                     0, 0, 0, NULL, &bbox)) == NULL)
    {
        /* an error was already reported. */
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
            free(pszURL);
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
            free(pszURL);

            return MS_SUCCESS;  // No overlap.
        }
    }

/* ------------------------------------------------------------------
 * check to see if a the metadata wms_connectiontimeout is set. If it is 
 * the case we will use it, else we use the default which is 30 seconds.
 * First check the metadata in the layer object and then in the map object.
 * ------------------------------------------------------------------ */
    nTimeout = 30000;  // Default is 30 seconds (internal value in ms)
    if ((pszTmp = msLookupHashTable(lp->metadata, 
                                    "wms_connectiontimeout")) != NULL)
    {
        nTimeout = atoi(pszTmp)*1000;  // Convert from seconds to milliseconds
    }
    else if ((pszTmp = msLookupHashTable(map->web.metadata, 
                                         "wms_connectiontimeout")) != NULL)
    {
        nTimeout = atoi(pszTmp)*1000;
    }

/* ------------------------------------------------------------------
 * Add a request to the array (already preallocated)
 * ------------------------------------------------------------------ */
    pasReqInfo[(*numRequests)].nLayerId = nLayerId;
    pasReqInfo[(*numRequests)].request = NULL;
    pasReqInfo[(*numRequests)].pszGetUrl = pszURL;
    pszURL = NULL;
    // We'll store the remote server's response to a tmp file.
    pasReqInfo[(*numRequests)].pszOutputFile = msTmpFile(map->web.imagepath, 
                                                         "img.tmp");
    pasReqInfo[(*numRequests)].nStatus = 0;
    pasReqInfo[(*numRequests)].nTimeout = nTimeout;
    pasReqInfo[(*numRequests)].bbox = bbox;

    (*numRequests)++;

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

/* ------------------------------------------------------------------
 * Find the request info for this layer in the array, based on nLayerId
 * ------------------------------------------------------------------ */
    for(iReq=0; iReq<numRequests; iReq++)
    {
        if (pasReqInfo[iReq].nLayerId == nLayerId)
            break;
    }

    if (iReq == numRequests)
        return MS_SUCCESS;  // This layer was skipped... nothing to do.

    if (pasReqInfo[iReq].nStatus != 200)
    {
/* ==================================================================== 
      Failed downloading layer... we still return SUCCESS here so that 
      the layer is only skipped intead of aborting the whole draw map.
 ==================================================================== */
        return MS_SUCCESS;
    }

/* ------------------------------------------------------------------
 * Prepare layer for drawing, reprojecting the image received from the
 * server if needed...
 * ------------------------------------------------------------------ */
    lp->type = MS_LAYER_RASTER;

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
            free(wldfile);
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
    unlink(lp->data);

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



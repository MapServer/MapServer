/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of WFS CONNECTIONTYPE - client to WFS servers
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2002, Daniel Morissette, DM Solutions Group Inc
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
 * Revision 1.1  2002/10/09 02:29:03  dan
 * Initial implementation of WFS client support.
 *
 **********************************************************************/

#include "map.h"
#include "maperror.h"
#include "mapows.h"

#define WFS_V_1_0_0  100


/**********************************************************************
 *                          msBuildWFSLayerURL()
 *
 * Build a WFS GetFeature URL.  
 *
 * Returns a reference to a newly allocated string that should be freed 
 * by the caller.
 **********************************************************************/
char *msBuildWFSLayerURL(mapObj *map, layerObj *lp, rectObj *bbox_ret) 
{
#ifdef USE_WFS_LYR
    char *pszURL = NULL;
    const char *pszTmp;
    rectObj bbox;
    int nVersion;
    
    if (lp->connectiontype != MS_WFS || lp->connection == NULL)
    {
        msSetError(MS_WFSCONNERR, "Call supported only for CONNECTIONTYPE WFS",
                   "msBuildWFSLayerURL()");
        return NULL;
    }

/* ------------------------------------------------------------------
 * Find out request version
 * ------------------------------------------------------------------ */
    if ((pszTmp = strstr(lp->connection, "VERSION=")) == NULL &&
        (pszTmp = strstr(lp->connection, "version=")) == NULL )
    {
        msSetError(MS_WFSCONNERR, "WFS Connection String must contain the VERSION parameter (with name in uppercase).", "msBuildWFSLayerURL()");
        return NULL;
    }
    pszTmp = strchr(pszTmp, '=')+1;
    if (strncmp(pszTmp, "1.0.0", 5) >= 0)
        nVersion = WFS_V_1_0_0;
    else
    {
        msSetError(MS_WFSCONNERR, "MapServer supports only WFS 1.0.0 (please verify the VERSION parameter in the connection string).", "msBuildWFSLayerURL()");
        return NULL;
    }

/* ------------------------------------------------------------------
 * Figure the SRS we'll use for the request.
 * - Fetch the map SRS (if it's EPSG)
 * - Check if map SRS is listed in layer wfs_srs metadata
 * - If map SRS is valid for this layer then use it
 * - Otherwise request layer in its default SRS and we'll reproject later
 * ------------------------------------------------------------------ */

// __TODO__ WFS servers support only one SRS... need to decide how we'll
// handle this and document it well.
// It's likely that we'll simply reproject the BBOX to teh layer's projection.



/* ------------------------------------------------------------------
 * Set layer SRS and reproject map extents to the layer's SRS
 * ------------------------------------------------------------------ */
#ifdef __TODO__
    // No need to set lp->proj if it's already set to the right EPSG code
    if ((pszTmp = msGetEPSGProj(&(lp->projection), NULL, MS_TRUE)) == NULL ||
        strcasecmp(pszEPSG, pszTmp) != 0)
    {
        char szProj[20];
        sprintf(szProj, "init=epsg:%s", pszEPSG+5);
        if (msLoadProjectionString(&(lp->projection), szProj) != 0)
            return NULL;
    }
#endif

    bbox = map->extent;
    if (msProjectionsDiffer(&(map->projection), &(lp->projection)))
    {
        msProjectRect(&(map->projection), &(lp->projection), &bbox);
    }

    if (bbox_ret != NULL)
        *bbox_ret = bbox;

/* ------------------------------------------------------------------
 * Build the request URL.
 * At this point we set only the following parameters for GetFeature:
 *   REQUEST
 *   BBOX
 *
 * The connection string should contain all other required params, 
 * including:
 *   VERSION
 *   SERVICE
 *   TYPENAME
 * ------------------------------------------------------------------ */
    // Make sure we have a big enough buffer for the URL
    if(!(pszURL = (char *)malloc((strlen(lp->connection)+256)*sizeof(char)))) 
    {
        msSetError(MS_MEMERR, NULL, "msBuildWFSLayerURL()");
        return NULL;
    }

    // __TODO__ We have to urlencode each value... especially the BBOX values
    // because if they end up in exponent format (123e+06) the + will be seen
    // as a space by the remote server.

    sprintf(pszURL, 
            "%s&REQUEST=GetFeature&BBOX=%f,%f,%f,%f",
            lp->connection, bbox.minx, bbox.miny, bbox.maxx, bbox.maxy);

    return pszURL;

#else
/* ------------------------------------------------------------------
 * WFS CONNECTION Support not included...
 * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.", 
             "msBuildWFSLayerURL()");
  return NULL;

#endif /* USE_WFS_LYR */

}


/**********************************************************************
 *                          msPrepareWFSLayerRequest()
 *
 **********************************************************************/

int msPrepareWFSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             httpRequestObj *pasReqInfo, int *numRequests) 
{
#ifdef USE_WFS_LYR
    char *pszURL = NULL;
    const char *pszTmp;
    rectObj bbox;
    int nTimeout;

    if (lp->connectiontype != MS_WFS || lp->connection == NULL)
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
    if ((pszURL = msBuildWFSLayerURL(map, lp, &bbox)) == NULL)
    {
        /* an error was already reported. */
        return MS_FAILURE;
    }

/* ------------------------------------------------------------------
 * Check if layer overlaps current view window (using wfs_latlonboundingbox)
 * ------------------------------------------------------------------ */
    if ((pszTmp = msLookupHashTable(lp->metadata, 
                                    "wfs_latlonboundingbox")) != NULL)
    {
        char **tokens;
        int n;
        rectObj ext;

        tokens = split(pszTmp, ' ', &n);
        if (tokens==NULL || n != 4) {
            msSetError(MS_WFSCONNERR, "Wrong number of arguments for 'wfs_latlonboundingbox' metadata.",
                       "msDrawWFSLayer()");
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
 * check to see if a the metadata wfs_connectiontimeout is set. If it is 
 * the case we will use it, else we use the default which is 30 seconds.
 * First check the metadata in the layer object and then in the map object.
 * ------------------------------------------------------------------ */
    nTimeout = 30000;  // Default is 30 seconds (internal value in ms)
    if ((pszTmp = msLookupHashTable(lp->metadata, 
                                    "wfs_connectiontimeout")) != NULL)
    {
        nTimeout = atoi(pszTmp)*1000;  // Convert from seconds to milliseconds
    }
    else if ((pszTmp = msLookupHashTable(map->web.metadata, 
                                         "wfs_connectiontimeout")) != NULL)
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
                                                         "gml.tmp");
    pasReqInfo[(*numRequests)].nStatus = 0;
    pasReqInfo[(*numRequests)].nTimeout = nTimeout;
    pasReqInfo[(*numRequests)].bbox = bbox;

    (*numRequests)++;

    return MS_SUCCESS;

#else
/* ------------------------------------------------------------------
 * WFS CONNECTION Support not included...
 * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.", 
             "msDrawWFSLayer()");
  return(MS_FAILURE);

#endif /* USE_WFS_LYR */

}


/**********************************************************************
 *                          msDrawWFSLayerLow()
 *
 **********************************************************************/

int msDrawWFSLayerLow(int nLayerId, httpRequestObj *pasReqInfo, 
                      int numRequests, mapObj *map, layerObj *lp, imageObj *img) 
{
#ifdef USE_WFS_LYR
    int status = MS_SUCCESS;
    int iReq = -1;
    char szPath[MS_MAXPATHLEN];
    char *pszOldConnection = NULL;

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
 * Prepare layer for drawing, set as an OGR connection to the GML data file
 * after saving the original WFS connection string to restore it at the end.
 * ------------------------------------------------------------------ */
    pszOldConnection = lp->connection;
    lp->connectiontype = MS_OGR;
    lp->connection = strdup(pasReqInfo[iReq].pszOutputFile);

    // Render as a regular OGR layer
    status = msDrawLayer(map, lp, img);

    // Restore original WFS CONNECITON value
    free(lp->connection);
    lp->connection = pszOldConnection;

    // We're done with the remote server's response... delete it.
    // __TODO__ We should cache the responses for reuse
    //unlink(pasReqInfo[iReq].pszOutputFile);

    return status;

#else
/* ------------------------------------------------------------------
 * WFS CONNECTION Support not included...
 * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.", 
             "msDrawWFSLayer()");
  return(MS_FAILURE);

#endif /* USE_WFS_LYR */

}



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
 * Revision 1.11  2003/02/19 14:19:02  frank
 * cleanup warnings
 *
 * Revision 1.10  2002/12/19 07:35:53  dan
 * Call msWFSLayerWhichShapes() inside open() to force downloading layer and
 * enable msOGRLayerGetItems() to work for queries.  i.e.WFS queries work now.
 *
 * Revision 1.9  2002/12/19 06:30:59  dan
 * Enable caching WMS/WFS request using tmp filename built from URL
 *
 * Revision 1.8  2002/12/19 05:17:09  dan
 * Report WFS exceptions, and do not fail on WFS requests returning 0 features
 *
 * Revision 1.7  2002/12/17 21:33:54  dan
 * Enable following redirections with libcurl (requires libcurl 7.10.1+)
 *
 * Revision 1.6  2002/12/17 05:30:17  dan
 * Fixed HTTP timeout value (in secs, not msecs) for WMS/WFS requests
 *
 * Revision 1.5  2002/12/17 04:48:25  dan
 * Accept version 0.0.14 in addition to 1.0.0
 *
 * Revision 1.4  2002/12/16 20:35:00  dan
 * Flush libwww and use libcurl instead for HTTP requests in WMS/WFS client
 *
 * Revision 1.3  2002/12/13 01:32:53  dan
 * OOOpps.. lp->wfslayerinfo was not set in msWFSLayerOpen()
 *
 * Revision 1.2  2002/12/13 00:57:31  dan
 * Modified WFS implementation to behave more as a real vector data source
 *
 * Revision 1.1  2002/10/09 02:29:03  dan
 * Initial implementation of WFS client support.
 *
 **********************************************************************/

#include "map.h"
#include "maperror.h"
#include "mapows.h"

#define WFS_V_0_0_14  14
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
    if (strncmp(pszTmp, "0.0.14", 6) == 0 || strncmp(pszTmp, "1.0.0", 5) >= 0 )
        nVersion = WFS_V_1_0_0;
    else
    {
        msSetError(MS_WFSCONNERR, "MapServer supports only WFS 1.0.0 or 0.0.14 (please verify the VERSION parameter in the connection string).", "msBuildWFSLayerURL()");
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


typedef struct ms_wfs_layer_info_t
{
    char        *pszGMLFilename;
    rectObj     rect;                     /* set by WhichShapes */
    char        *pszGetUrl;
    int         nStatus;           /* HTTP status */
    int         bLayerOpened;      /* False until msWFSLayerOpen() is called*/
} msWFSLayerInfo;

#ifdef USE_WFS_LYR

/**********************************************************************
 *                          msAllocWFSLayerInfo()
 *
 **********************************************************************/
static msWFSLayerInfo *msAllocWFSLayerInfo()
{
    msWFSLayerInfo *psInfo;

    psInfo = (msWFSLayerInfo*)calloc(1,sizeof(msWFSLayerInfo));
    if (psInfo)
    {
        psInfo->pszGMLFilename = NULL;
        psInfo->rect.minx = psInfo->rect.maxx = 0;
        psInfo->rect.miny = psInfo->rect.maxy = 0;
        psInfo->pszGetUrl = NULL;
        psInfo->nStatus = 0;
        psInfo->bLayerOpened = MS_FALSE;
    }
    else
    {
        msSetError(MS_MEMERR, NULL, "msAllocWFSLayerInfo()");
    }

    return psInfo;
}

/**********************************************************************
 *                          msFreeWFSLayerInfo()
 *
 **********************************************************************/
static void msFreeWFSLayerInfo(msWFSLayerInfo *psInfo)
{
    if (psInfo)
    {
        if (psInfo->pszGMLFilename)
            free(psInfo->pszGMLFilename);
        if (psInfo->pszGetUrl)
            free(psInfo->pszGetUrl);

        free(psInfo);
    }
}

#endif /* USE_WFS_LYR */

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
    int nStatus = MS_SUCCESS;
    msWFSLayerInfo *psInfo = NULL;

    if (lp->connectiontype != MS_WFS || lp->connection == NULL)
        return MS_FAILURE;

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
 * check to see if a the metadata wfs_connectiontimeout is set. If it is 
 * the case we will use it, else we use the default which is 30 seconds.
 * First check the metadata in the layer object and then in the map object.
 * ------------------------------------------------------------------ */
    nTimeout = 30;  // Default is 30 seconds 
    if ((pszTmp = msLookupHashTable(lp->metadata, 
                                    "wfs_connectiontimeout")) != NULL)
    {
        nTimeout = atoi(pszTmp);
    }
    else if ((pszTmp = msLookupHashTable(map->web.metadata, 
                                         "wfs_connectiontimeout")) != NULL)
    {
        nTimeout = atoi(pszTmp);
    }

/* ------------------------------------------------------------------
 * If nLayerId == -1 then we need to figure it
 * ------------------------------------------------------------------ */
    if (nLayerId == -1)
    {
        int iLayer;
        for(iLayer=0; iLayer < map->numlayers; iLayer++)
        {
            if (&(map->layers[iLayer]) == lp)
            {
                nLayerId = iLayer;
                break;
            }
        }
    }

/* ------------------------------------------------------------------
 * Add a request to the array (already preallocated)
 * ------------------------------------------------------------------ */
    pasReqInfo[(*numRequests)].nLayerId = nLayerId;
    pasReqInfo[(*numRequests)].pszGetUrl = pszURL;
    pszURL = NULL;
    // We'll store the remote server's response to a tmp file.
    pasReqInfo[(*numRequests)].pszOutputFile =  
        msOWSBuildURLFilename(map->web.imagepath, 
                              pasReqInfo[(*numRequests)].pszGetUrl,".tmp.gml");
    pasReqInfo[(*numRequests)].nStatus = 0;
    pasReqInfo[(*numRequests)].nTimeout = nTimeout;
    pasReqInfo[(*numRequests)].bbox = bbox;

/* ------------------------------------------------------------------
 * Pre-Open the layer now, (i.e. alloc and fill msWFSLayerInfo inside 
 * layer obj).  Layer will be ready for use when the main mapserver 
 * code calls msLayerOpen().
 * ------------------------------------------------------------------ */
    if (lp->wfslayerinfo != NULL)
    {
        psInfo =(msWFSLayerInfo*)(lp->wfslayerinfo);
    }
    else
    {
        lp->wfslayerinfo = psInfo = msAllocWFSLayerInfo();
    }

    if (psInfo->pszGMLFilename) 
        free(psInfo->pszGMLFilename);
    psInfo->pszGMLFilename=strdup(pasReqInfo[(*numRequests)].pszOutputFile);
 
    psInfo->rect = pasReqInfo[(*numRequests)].bbox;

    if (psInfo->pszGetUrl) 
        free(psInfo->pszGetUrl);
    psInfo->pszGetUrl = strdup(pasReqInfo[(*numRequests)].pszGetUrl);

    psInfo->nStatus = 0;

    (*numRequests)++;

    return nStatus;

#else
/* ------------------------------------------------------------------
 * WFS CONNECTION Support not included...
 * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.", 
             "msPrepareWFSLayerRequest");
  return(MS_FAILURE);

#endif /* USE_WFS_LYR */

}

/**********************************************************************
 *                          msWFSUpdateRequestInfo()
 *
 * This function is called after a WFS request has been completed so that
 * we can copy request result information from the httpRequestObj to the
 * msWFSLayerInfo struct.  Things to copy here are the HTTP status, exceptions
 * information, mime type, etc.
 **********************************************************************/
void msWFSUpdateRequestInfo(layerObj *lp, httpRequestObj *pasReqInfo)
{
    if (lp->wfslayerinfo)
    {
        msWFSLayerInfo *psInfo = NULL;

        psInfo =(msWFSLayerInfo*)(lp->wfslayerinfo);

        // Copy request results infos to msWFSLayerInfo struct
        // For now there is only nStatus, but we should eventually add
        // mime type and WFS exceptions information.
        psInfo->nStatus = pasReqInfo->nStatus;
    }
}

/**********************************************************************
 *                          msWFSLayerOpen()
 *
 * WFS layers are just a special case of OGR connection.  Only the open/close
 * methods differ since they have to download and maintain GML files in cache
 * but the rest is mapped directly to OGR function calls in maplayer.c
 *
 **********************************************************************/

int msWFSLayerOpen(layerObj *lp, 
                   const char *pszGMLFilename, rectObj *defaultBBOX) 
{
#ifdef USE_WFS_LYR
    int status = MS_SUCCESS;
    msWFSLayerInfo *psInfo = NULL;

    if (lp->wfslayerinfo != NULL)
    {
        psInfo =(msWFSLayerInfo*)lp->wfslayerinfo;

        // Layer already opened.  If explicit filename requested then check
        // that file was already opened with the same filename.
        // If no explicit filename requested then we'll try to reuse the
        // previously opened layer... this will happen in a msDrawMap() call.
        if (pszGMLFilename == NULL ||
            (psInfo->pszGMLFilename && pszGMLFilename && 
             strcmp(psInfo->pszGMLFilename, pszGMLFilename) == 0) )
        {
            return MS_SUCCESS;  // Nothing to do... layer is already opened
        }
        else
        {
            // Hmmm... should we produce a fatal error?
            // For now we'll just close the layer and reopen it.
            msDebug("msWFSLayerOpen(): Layer already opened (%s)\n",
                    lp->name?lp->name:"(null)" );
            msWFSLayerClose(lp);
        }
    }

/* ------------------------------------------------------------------
 * Alloc and fill msWFSLayerInfo inside layer obj
 * ------------------------------------------------------------------ */
    lp->wfslayerinfo = psInfo = msAllocWFSLayerInfo();

    if (pszGMLFilename)
        psInfo->pszGMLFilename = strdup(pszGMLFilename);
    else
        psInfo->pszGMLFilename = msTmpFile(lp->map->web.imagepath, 
                                           "tmp.gml");

    if (defaultBBOX)
    {
        // __TODO__ If new bbox differs from current one then we should
        // invalidate current GML file in cache
        psInfo->rect = *defaultBBOX;
    }
    else
    {
        // Use map bbox by default
        psInfo->rect = lp->map->extent;
    }

    // We will call whichshapes() now and force downloading layer right
    // away.  This saves from having to call DescribeFeatureType and
    // parsing the response (being lazy I guess) and anyway given the
    // way we work with layers right now the bbox is unlikely to change
    // between now and the time whichshapes() would have been called by
    // the MapServer core.
    if (msWFSLayerWhichShapes(lp, psInfo->rect) == MS_FAILURE)
        status = MS_FAILURE;
    
    psInfo->bLayerOpened = MS_TRUE;

    return status;
#else
/* ------------------------------------------------------------------
 * WFS CONNECTION Support not included...
 * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.", 
             "msWFSLayerOpen()");
  return(MS_FAILURE);

#endif /* USE_WFS_LYR */
}

/**********************************************************************
 *                          msWFSLayerInitItemInfo()
 *
 **********************************************************************/

int msWFSLayerInitItemInfo(layerObj *layer)
{
    // Nothing to do here.  OGR will do its own initialization when it
    // opens the actual file.
    // Note that we didn't implement our own msWFSLayerFreeItemInfo()
    // so that the OGR one gets called.
    return MS_SUCCESS;
}

/**********************************************************************
 *                          msWFSLayerGetItems()
 *
 **********************************************************************/

int msWFSLayerGetItems(layerObj *layer)
{
    // For now this method simply lets OGR parse the GML and figure the 
    // schema itself.
    // It could also be implemented to call DescribeFeatureType for
    // this layer, but we don't need to do it so why waste resources?

    return msOGRLayerGetItems(layer);
}

/**********************************************************************
 *                          msWFSLayerWhichShapes()
 *
 **********************************************************************/

int msWFSLayerWhichShapes(layerObj *lp, rectObj rect)
{
#ifdef USE_WFS_LYR
    msWFSLayerInfo *psInfo;
    int status = MS_SUCCESS;
    const char *pszTmp;
    FILE *fp;

    psInfo =(msWFSLayerInfo*)lp->wfslayerinfo;

    if (psInfo == NULL)
    {
        msSetError(MS_WFSCONNERR, "Assertion failed: WFS layer not opened!!!", 
                   "msWFSLayerWhichShapes()");
        return(MS_FAILURE);
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
            msSetError(MS_WFSCONNERR, "Wrong number of values in 'wfs_latlonboundingbox' metadata.",
                       "msWFSLayerWhichShapes()");
            return MS_FAILURE;
        }

        ext.minx = atof(tokens[0]);
        ext.miny = atof(tokens[1]);
        ext.maxx = atof(tokens[2]);
        ext.maxy = atof(tokens[3]);

        msFreeCharArray(tokens, n);

        // Reproject latlonboundingbox to the selected SRS for the layer and
        // check if it overlaps the bbox that we calculated for the request

        msProjectRect(&(lp->map->latlon), &(lp->projection), &ext);
        if (!msRectOverlap(&rect, &ext))
        {
            // No overlap... nothing to do
            return MS_DONE;  // No overlap.
        }
    }


 /* ------------------------------------------------------------------
  * __TODO__ If new bbox differs from current one then we should
  * invalidate current GML file in cache
  * ------------------------------------------------------------------ */
    psInfo->rect = rect;


/* ------------------------------------------------------------------
 * If file not downloaded yet then do it now.
 * ------------------------------------------------------------------ */
    if (psInfo->nStatus == 0)
    {
        httpRequestObj asReqInfo[2];
        int numReq = 0;

        msHTTPInitRequestObj(asReqInfo, 2);

        if ( msPrepareWFSLayerRequest(-1, lp->map, lp,
                                      asReqInfo, &numReq) == MS_FAILURE  ||
             msOWSExecuteRequests(asReqInfo, numReq, 
                                  lp->map, MS_TRUE) == MS_FAILURE )
        {
            // Delete tmp file... we don't want it to stick around.
            unlink(asReqInfo[0].pszOutputFile);
            return MS_FAILURE;
        }

        // Cleanup
        msHTTPFreeRequestObj(asReqInfo, numReq);

    }

    if ( !MS_HTTP_SUCCESS( psInfo->nStatus ) )
    {
        // Delete tmp file... we don't want it to stick around.
        unlink(psInfo->pszGMLFilename);

        msSetError(MS_WFSCONNERR, 
                   "Got HTTP status %d downloading WFS layer %s", 
                   "msWFSLayerWhichShapes()",
                   psInfo->nStatus, lp->name?lp->name:"(null)");
        return(MS_FAILURE);
    }

/* ------------------------------------------------------------------
 * Check that file is really GML... it could be an exception, or just junk.
 * ------------------------------------------------------------------ */
    if ((fp = fopen(psInfo->pszGMLFilename, "r")) != NULL)
    {
        char szHeader[2000];
        int  nBytes = 0;

        nBytes = fread( szHeader, 1, sizeof(szHeader)-1, fp );
        fclose(fp);

        if (nBytes < 0)
            nBytes = 0;
        szHeader[nBytes] = '\0';

        if ( nBytes == 0 )
        {
            msSetError(MS_WFSCONNERR, 
                       "WFS request produced no oputput for layer %s.",
                       "msWFSLayerWhichShapes()",
                       lp->name?lp->name:"(null)");
            return(MS_FAILURE);

        }
        if ( strstr(szHeader, "<WFS_Exception>") ||
             strstr(szHeader, "<ServiceExceptionReport>") )
        {
            msOWSProcessException(lp, psInfo->pszGMLFilename,
                                  MS_WFSCONNERR, "msWFSLayerWhichShapes()" );
            return MS_FAILURE;
        }
        else if ( strstr(szHeader,"opengis.net/gml") &&
                  strstr(szHeader,"featureMember>") == NULL )
        {
            // This looks like valid GML, but contains 0 features.
            return MS_DONE;
        }
        else if ( strstr(szHeader,"opengis.net/gml") == NULL ||
                  strstr(szHeader,"featureMember>") == NULL )
        {
            // This is probably just junk.
            msSetError(MS_WFSCONNERR, 
                       "WFS request produced unexpected output (junk?) for layer %s.",
                       "msWFSLayerWhichShapes()",
                       lp->name?lp->name:"(null)");
            return(MS_FAILURE);
        }
        
        // If we got this far, it must be a valid GML dataset... keep going
    }


/* ------------------------------------------------------------------
 * Open GML file using OGR.
 * ------------------------------------------------------------------ */
    if ((status = msOGRLayerOpen(lp, psInfo->pszGMLFilename)) != MS_SUCCESS)
        return status;
    
    status = msOGRLayerWhichShapes(lp, rect);

    return status;
#else
/* ------------------------------------------------------------------
 * WFS CONNECTION Support not included...
 * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.", 
             "msWFSLayerWhichShapes()");
  return(MS_FAILURE);

#endif /* USE_WFS_LYR */
}



/**********************************************************************
 *                          msWFSLayerClose()
 *
 **********************************************************************/

int msWFSLayerClose(layerObj *lp)
{
#ifdef USE_WFS_LYR

/* ------------------------------------------------------------------
 * Cleanup OGR connection
 * ------------------------------------------------------------------ */
    if (lp->ogrlayerinfo)
        msOGRLayerClose(lp);

/* ------------------------------------------------------------------
 * Cleanup WFS connection info.
 * __TODO__ For now we flush everything, but we should try to cache some stuff
 * ------------------------------------------------------------------ */
    // __TODO__ unlink()  .gml file and OGR's schema file if they exist
    // unlink(

    msFreeWFSLayerInfo(lp->wfslayerinfo);
    lp->wfslayerinfo = NULL;

    return MS_SUCCESS;

#else
/* ------------------------------------------------------------------
 * WFS CONNECTION Support not included...
 * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.", 
             "msWFSLayerClose()");
  return(MS_FAILURE);

#endif /* USE_WFS_LYR */
}



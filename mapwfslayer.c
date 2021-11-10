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

#include "mapserver.h"
#include "maperror.h"
#include "mapows.h"
#include "mapproject.h"

#include <time.h>
#include <assert.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#endif



#define WFS_V_0_0_14  14
#define WFS_V_1_0_0  100


/*====================================================================
 *  Private (static) functions
 *====================================================================*/

#ifdef USE_WFS_LYR

/************************************************************************/
/*                           msBuildRequestParms                        */
/*                                                                      */
/*      Build the params object based on the metadata                   */
/*      information. This object will be used when building the Get     */
/*      and Post requsests.                                             */
/*      Note : Verify the connection string to extract some values      */
/*      for backward compatiblity. (It is though depricated).           */
/*      This will also set layer projection and compute BBOX in that    */
/*      projection.                                                     */
/*                                                                      */
/************************************************************************/
static wfsParamsObj *msBuildRequestParams(mapObj *map, layerObj *lp,
    rectObj *bbox_ret)
{
  wfsParamsObj *psParams = NULL;
  rectObj bbox;
  const char *pszTmp;
  char *pszVersion, *pszTypeName;

  if (!map || !lp || !bbox_ret)
    return NULL;

  if (lp->connection == NULL)
    return NULL;

  psParams = msWFSCreateParamsObj();
  pszTmp = msOWSLookupMetadata(&(lp->metadata), "FO", "version");
  if (pszTmp)
    psParams->pszVersion = msStrdup(pszTmp);
  else {
    pszTmp = strstr(lp->connection, "VERSION=");
    if (!pszTmp)
      pszTmp = strstr(lp->connection, "version=");
    if (pszTmp) {
      pszVersion = strchr(pszTmp, '=')+1;
      if (strncmp(pszVersion, "0.0.14", 6) == 0)
        psParams->pszVersion = msStrdup("0.0.14");
      else if (strncmp(pszVersion, "1.0.0", 5) == 0)
        psParams->pszVersion = msStrdup("1.0.0");
    }
  }

  /*the service is always set to WFS : see bug 1302 */
  psParams->pszService = msStrdup("WFS");

  /*
  pszTmp = msOWSLookupMetadata(&(lp->metadata), "FO", "service");
  if (pszTmp)
    psParams->pszService = msStrdup(pszTmp);
  else
  {
      pszTmp = strstr(lp->connection, "SERVICE=");
      if (!pszTmp)
        pszTmp = strstr(lp->connection, "service=");
      if (pszTmp)
      {
          pszService = strchr(pszTmp, '=')+1;
          if (strncmp(pszService, "WFS", 3) == 0)
            psParams->pszService = msStrdup("WFS");
      }
  }
  */

  pszTmp = msOWSLookupMetadata(&(lp->metadata), "FO", "geometryname");
  if (pszTmp)
    psParams->pszGeometryName = msStrdup(pszTmp);

  pszTmp = msOWSLookupMetadata(&(lp->metadata), "FO", "typename");
  if (pszTmp)
    psParams->pszTypeName = msStrdup(pszTmp);
  else {
    pszTmp = strstr(lp->connection, "TYPENAME=");
    if (!pszTmp)
      pszTmp = strstr(lp->connection, "typename=");
    if (pszTmp) {
      pszTypeName = strchr(pszTmp, '=')+1;
      if (pszTypeName) {
        const int nLength = strlen(pszTypeName);
        if (nLength > 0) {
          int i=0;
          for (; i<nLength; i++) {
            if (pszTypeName[i] == '&')
              break;
          }

          if (i<nLength) {
            char *pszTypeNameTmp = NULL;
            pszTypeNameTmp = msStrdup(pszTypeName);
            pszTypeNameTmp[i] = '\0';
            psParams->pszTypeName = msStrdup(pszTypeNameTmp);
            free(pszTypeNameTmp);
          } else
            psParams->pszTypeName = msStrdup(pszTypeName);
        }
      }
    }
  }

  pszTmp = msOWSLookupMetadata(&(lp->metadata), "FO", "filter");
  if (pszTmp && strlen(pszTmp) > 0) {
    if (strstr(pszTmp, "<Filter>") !=NULL || strstr(pszTmp, "<ogc:Filter") != NULL)
      psParams->pszFilter = msStrdup(pszTmp);
    else {
      psParams->pszFilter = msStringConcatenate(psParams->pszFilter, "<ogc:Filter xmlns:ogc=\"http://www.opengis.net/ogc\">");
      psParams->pszFilter = msStringConcatenate(psParams->pszFilter, (char*)pszTmp);
      psParams->pszFilter = msStringConcatenate(psParams->pszFilter, "</ogc:Filter>");
    }
  }

  pszTmp = msOWSLookupMetadata(&(lp->metadata), "FO", "maxfeatures");
  if (pszTmp)
    psParams->nMaxFeatures = atoi(pszTmp);

  /* Request is always GetFeature; */
  psParams->pszRequest = msStrdup("GetFeature");


  /* ------------------------------------------------------------------
   * Figure the SRS we'll use for the request.
   * - Fetch the map SRS (if it's EPSG)
   * - Check if map SRS is listed in layer wfs_srs metadata
   * - If map SRS is valid for this layer then use it
   * - Otherwise request layer in its default SRS and we'll reproject later
   * ------------------------------------------------------------------ */

  /* __TODO__ WFS servers support only one SRS... need to decide how we'll */
  /* handle this and document it well. */
  /* It's likely that we'll simply reproject the BBOX to teh layer's projection. */

  /* ------------------------------------------------------------------
   * Set layer SRS and reproject map extents to the layer's SRS
   * ------------------------------------------------------------------ */
#ifdef __TODO__
  /* No need to set lp->proj if it's already set to the right EPSG code */
  if ((pszTmp = msGetEPSGProj(&(lp->projection), NULL, MS_TRUE)) == NULL ||
      strcasecmp(pszEPSG, pszTmp) != 0) {
    char szProj[20];
    snprintf(szProj, sizeof(szProj), "init=epsg:%s", pszEPSG+5);
    if (msLoadProjectionString(&(lp->projection), szProj) != 0)
      return NULL;
  }
#endif

  bbox = map->extent;
  if (msProjectionsDiffer(&(map->projection), &(lp->projection))) {
    msProjectRect(&(map->projection), &(lp->projection), &bbox);
  }

  *bbox_ret = bbox;

  return psParams;

}

/**********************************************************************
 *                          msBuildWFSLayerPostRequest()
 *
 * Build a WFS GetFeature xml document for a Post Request.
 *
 * Returns a reference to a newly allocated string that should be freed
 * by the caller.
 **********************************************************************/
static char *msBuildWFSLayerPostRequest(rectObj *bbox, wfsParamsObj *psParams)
{
  char *pszPostReq = NULL;
  char *pszFilter = NULL;
  char *pszGeometryName = "Geometry";
  size_t bufferSize = 0;

  if (psParams->pszVersion == NULL ||
      (strncmp(psParams->pszVersion, "0.0.14", 6) != 0 &&
       strncmp(psParams->pszVersion, "1.0.0", 5)) != 0) {
    msSetError(MS_WFSCONNERR, "MapServer supports only WFS 1.0.0 or 0.0.14 (please verify the version metadata wfs_version).", "msBuildWFSLayerPostRequest()");
    return NULL;
  }



  if (psParams->pszTypeName == NULL) {
    msSetError(MS_WFSCONNERR, "Metadata wfs_typename must be set in the layer", "msBuildWFSLayerPostRequest()");
    return NULL;
  }


  if (psParams->pszGeometryName) {
    pszGeometryName = psParams->pszGeometryName;
  }

  if (psParams->pszFilter)
    pszFilter = psParams->pszFilter;
  else {
    bufferSize = 500;
    pszFilter = (char *)msSmallMalloc(bufferSize);
    snprintf(pszFilter, bufferSize, "<ogc:Filter>\n"
             "<ogc:BBOX>\n"
             "<ogc:PropertyName>%s</ogc:PropertyName>\n"
             "<gml:Box>\n"
             "<gml:coordinates>%f,%f %f,%f</gml:coordinates>\n"
             "</gml:Box>\n"
             "</ogc:BBOX>\n"
             "</ogc:Filter>", pszGeometryName, bbox->minx, bbox->miny, bbox->maxx, bbox->maxy);
  }

  bufferSize = strlen(pszFilter)+strlen(psParams->pszTypeName)+500;
  pszPostReq = (char *)msSmallMalloc(bufferSize);
  if (psParams->nMaxFeatures > 0)
    snprintf(pszPostReq, bufferSize, "<?xml version=\"1.0\" ?>\n"
             "<wfs:GetFeature\n"
             "service=\"WFS\"\n"
             "version=\"1.0.0\"\n"
             "maxFeatures=\"%d\"\n"
             "outputFormat=\"GML2\"\n"
			 "xmlns:wfs=\"http://www.opengis.net/wfs\" xmlns:ogc=\"http://www.opengis.net/ogc\" xsi:schemaLocation=\"http://www.opengis.net/wfs http://schemas.opengis.net/wfs/1.0.0/wfs.xsd\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:gml=\"http://www.opengis.net/gml\">\n"
             "<wfs:Query typeName=\"%s\">\n"
             "%s"
             "</wfs:Query>\n"
             "</wfs:GetFeature>\n", psParams->nMaxFeatures, psParams->pszTypeName, pszFilter);
  else
    snprintf(pszPostReq, bufferSize, "<?xml version=\"1.0\" ?>\n"
             "<wfs:GetFeature\n"
             "service=\"WFS\"\n"
             "version=\"1.0.0\"\n"
             "outputFormat=\"GML2\"\n"
             "xmlns:wfs=\"http://www.opengis.net/wfs\" xmlns:ogc=\"http://www.opengis.net/ogc\" xsi:schemaLocation=\"http://www.opengis.net/wfs http://schemas.opengis.net/wfs/1.0.0/wfs.xsd\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:gml=\"http://www.opengis.net/gml\">\n"
             "<wfs:Query typeName=\"%s\">\n"
             "%s"
             "</wfs:Query>\n"
             "</wfs:GetFeature>\n", psParams->pszTypeName, pszFilter);
  if (psParams->pszFilter == NULL)
    free(pszFilter);


  return pszPostReq;
}

/**********************************************************************
 *                          msBuildWFSLayerGetURL()
 *
 * Build a WFS GetFeature URL for a Get Request.
 *
 * Returns a reference to a newly allocated string that should be freed
 * by the caller.
 **********************************************************************/
static char *msBuildWFSLayerGetURL(layerObj *lp, rectObj *bbox,
                                   wfsParamsObj *psParams)
{
  char *pszURL = NULL, *pszOnlineResource=NULL;
  const char *pszTmp;
  char *pszVersion, *pszService, *pszTypename = NULL;
  int bVersionInConnection = 0;
  int bTypenameInConnection = 0;
  size_t bufferSize = 0;

  if (lp->connectiontype != MS_WFS || lp->connection == NULL) {
    msSetError(MS_WFSCONNERR, "Call supported only for CONNECTIONTYPE WFS",
               "msBuildWFSLayerGetURL()");
    return NULL;
  }

  /* -------------------------------------------------------------------- */
  /*      Find out request version. Look first for the wfs_version        */
  /*      metedata. If not available try to find out if the CONNECTION    */
  /*      string contains the version. This last test is done for         */
  /*      backward compatiblity but is depericated.                       */
  /* -------------------------------------------------------------------- */
  pszVersion = psParams->pszVersion;
  if (!pszVersion) {
    if ((pszTmp = strstr(lp->connection, "VERSION=")) == NULL &&
        (pszTmp = strstr(lp->connection, "version=")) == NULL ) {
      msSetError(MS_WFSCONNERR, "Metadata wfs_version must be set in the layer", "msBuildWFSLayerGetURL()");
      return NULL;
    }
    pszVersion = strchr(pszTmp, '=')+1;
    bVersionInConnection = 1;
  }


  if (strncmp(pszVersion, "0.0.14", 6) != 0 &&
      strncmp(pszVersion, "1.0.0", 5) != 0 &&
      strncmp(pszVersion, "1.1", 3) != 0) {
    msSetError(MS_WFSCONNERR,
	       "MapServer supports only WFS 1.1.0, 1.0.0 or 0.0.14 (please verify the version metadata wfs_version).",
	       "msBuildWFSLayerGetURL()");
    return NULL;
  }

  /* -------------------------------------------------------------------- */
  /*      Find out the service. It is always set to WFS in function       */
  /*      msBuildRequestParms  (check Bug 1302 for details).              */
  /* -------------------------------------------------------------------- */
  pszService = psParams->pszService;


  /* -------------------------------------------------------------------- */
  /*      Find out the typename. Look first for the wfs_tyename           */
  /*      metadata. If not available try to find out if the CONNECTION    */
  /*      string contains it. This last test is done for                  */
  /*      backward compatiblity but is depericated.                       */
  /* -------------------------------------------------------------------- */
  pszTypename = psParams->pszTypeName;
  if (!pszTypename) {
    if ((pszTmp = strstr(lp->connection, "TYPENAME=")) == NULL &&
        (pszTmp = strstr(lp->connection, "typename=")) == NULL ) {
      msSetError(MS_WFSCONNERR, "Metadata wfs_typename must be set in the layer", "msBuildWFSLayerGetURL()");
      return NULL;
    }
    bTypenameInConnection = 1;
  }


  /* --------------------------------------------------------------------
   * Build the request URL.
   * At this point we set only the following parameters for GetFeature:
   *   REQUEST
   *   BBOX
   *   VERSION
   *   SERVICE
   *   TYPENAME
   *   FILTER
   *   MAXFEATURES
   *
   * For backward compatiblity the user could also have in the connection
   * string the following parameters (but it is depricated):
   *   VERSION
   *   SERVICE
   *   TYPENAME
   * -------------------------------------------------------------------- */
  /* Make sure we have a big enough buffer for the URL */
  bufferSize = strlen(lp->connection)+1024;
  pszURL = (char *)malloc(bufferSize);
  MS_CHECK_ALLOC(pszURL, bufferSize, NULL);

  /* __TODO__ We have to urlencode each value... especially the BBOX values */
  /* because if they end up in exponent format (123e+06) the + will be seen */
  /* as a space by the remote server. */

  /* -------------------------------------------------------------------- */
  /*      build the URL,                                                  */
  /* -------------------------------------------------------------------- */
  /* make sure connection ends with "&" or "?" */
  pszOnlineResource = msOWSTerminateOnlineResource(lp->connection);
  snprintf(pszURL, bufferSize, "%s", pszOnlineResource);
  msFree(pszOnlineResource);

  /* REQUEST */
  snprintf(pszURL + strlen(pszURL), bufferSize-strlen(pszURL),  "&REQUEST=GetFeature");

  /* VERSION */
  if (!bVersionInConnection)
    snprintf(pszURL + strlen(pszURL), bufferSize-strlen(pszURL),  "&VERSION=%s", pszVersion);

  /* SERVICE */
  snprintf(pszURL + strlen(pszURL), bufferSize-strlen(pszURL),  "&SERVICE=%s", pszService);

  /* TYPENAME */
  if (!bTypenameInConnection)
    snprintf(pszURL + strlen(pszURL), bufferSize-strlen(pszURL),  "&TYPENAME=%s", pszTypename);

  /* -------------------------------------------------------------------- */
  /*      If the filter parameter is given in the wfs_filter metadata,    */
  /*      we use it and do not send the BBOX paramter as they are         */
  /*      mutually exclusive.                                             */
  /* -------------------------------------------------------------------- */
  if (psParams->pszFilter) {
    char *encoded_filter = msEncodeUrl(psParams->pszFilter);
    snprintf(pszURL + strlen(pszURL), bufferSize-strlen(pszURL), "&FILTER=%s",encoded_filter);
    free(encoded_filter);
  } else {
	  /*
	   * take care about the axis order for WFS 1.1
	   */
	  char *projUrn;
	  char *projEpsg;
	  projUrn = msOWSGetProjURN(&(lp->projection), &(lp->metadata), "FO", 1);
	  msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FO", 1, &projEpsg);

	  /*
	   * WFS 1.1 supports including the SRS in the BBOX parameter, should
	   * respect axis order in the BBOX and has a separate SRSNAME parameter for
	   * the desired result SRS.
	   * WFS 1.0 is always easting, northing, doesn't include the SRS as part of
	   * the BBOX parameter and has no SRSNAME parameter: if we don't have a
	   * URN then fallback to WFS 1.0 style */
	  if ((strncmp(pszVersion, "1.1", 3) == 0) && projUrn) {
		 if (projEpsg && (strncmp(projEpsg, "EPSG:", 5) == 0) &&
				 msIsAxisInverted(atoi(projEpsg + 5))) {
			 snprintf(pszURL + strlen(pszURL), bufferSize - strlen(pszURL),
					 "&BBOX=%.15g,%.15g,%.15g,%.15g,%s&SRSNAME=%s",
					 bbox->miny, bbox->minx, bbox->maxy, bbox->maxx,
					 projUrn, projUrn);
		 } else {
			 snprintf(pszURL + strlen(pszURL), bufferSize - strlen(pszURL),
					 "&BBOX=%.15g,%.15g,%.15g,%.15g,%s&SRSNAME=%s",
					 bbox->minx, bbox->miny, bbox->maxx, bbox->maxy,
					 projUrn, projUrn);
		 }
	  } else {
		  snprintf(pszURL + strlen(pszURL), bufferSize - strlen(pszURL),
				  "&BBOX=%.15g,%.15g,%.15g,%.15g",
				  bbox->minx, bbox->miny, bbox->maxx, bbox->maxy);
	  }

	  msFree(projUrn);
	  msFree(projEpsg);
  }

  if (psParams->nMaxFeatures > 0)
    snprintf(pszURL + strlen(pszURL), bufferSize-strlen(pszURL),
             "&MAXFEATURES=%d", psParams->nMaxFeatures);

  return pszURL;

}

/**********************************************************************
 *                          msWFSLayerInfo
 *
 **********************************************************************/
typedef struct ms_wfs_layer_info_t {
  char        *pszGMLFilename;
  rectObj     rect;                     /* set by WhichShapes */
  char        *pszGetUrl;
  int         nStatus;           /* HTTP status */
  int         bLayerHasValidGML;  /* False until msWFSLayerWhichShapes() is called and determines the result GML is valid with features*/
} msWFSLayerInfo;


/**********************************************************************
 *                          msAllocWFSLayerInfo()
 *
 **********************************************************************/
static msWFSLayerInfo *msAllocWFSLayerInfo(void)
{
  msWFSLayerInfo *psInfo;

  psInfo = (msWFSLayerInfo*)calloc(1,sizeof(msWFSLayerInfo));
  MS_CHECK_ALLOC(psInfo, sizeof(msWFSLayerInfo), NULL);

  psInfo->pszGMLFilename = NULL;
  psInfo->rect.minx = psInfo->rect.maxx = 0;
  psInfo->rect.miny = psInfo->rect.maxy = 0;
  psInfo->pszGetUrl = NULL;
  psInfo->nStatus = 0;

  return psInfo;
}

/**********************************************************************
 *                          msFreeWFSLayerInfo()
 *
 **********************************************************************/
static void msFreeWFSLayerInfo(msWFSLayerInfo *psInfo)
{
  if (psInfo) {
    if (psInfo->pszGMLFilename)
      free(psInfo->pszGMLFilename);
    if (psInfo->pszGetUrl)
      free(psInfo->pszGetUrl);

    free(psInfo);
  }
}

#endif /* USE_WFS_LYR */

/*====================================================================
 *  Public functions
 *====================================================================*/

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
  int bPostRequest = 0;
  wfsParamsObj *psParams = NULL;
  char *pszHTTPCookieData = NULL;


  if (lp->connectiontype != MS_WFS || lp->connection == NULL)
    return MS_FAILURE;

  /* ------------------------------------------------------------------
   * Build a params object that will be used by to build the request,
    this will also set layer projection and compute BBOX in that projection.
   * ------------------------------------------------------------------ */
  psParams = msBuildRequestParams(map, lp, &bbox);
  if (!psParams)
    return MS_FAILURE;

  /* -------------------------------------------------------------------- */
  /*      Depending on the metadata wfs_request_method, build a Get or    */
  /*      a Post URL.                                                     */
  /*    If it is a Get request the URL would contain all the parameters in*/
  /*      the string;                                                     */
  /*      If it is a Post request, the URL will only contain the          */
  /*      connection string comming from the layer.                       */
  /* -------------------------------------------------------------------- */
  if ((pszTmp = msOWSLookupMetadata(&(lp->metadata),
                                    "FO", "request_method")) != NULL) {
    if (strncmp(pszTmp, "GET", 3) ==0) {
      pszURL = msBuildWFSLayerGetURL(lp, &bbox, psParams);
      if (!pszURL) {
        /* an error was already reported. */
        return MS_FAILURE;
      }
    }
  }
  /* else it is a post request and just get the connection string */
  if (!pszURL) {
    bPostRequest = 1;
    pszURL = msStrdup(lp->connection);
  }

  /* ------------------------------------------------------------------
   * check to see if a the metadata wfs_connectiontimeout is set. If it is
   * the case we will use it, else we use the default which is 30 seconds.
   * First check the metadata in the layer object and then in the map object.
   * ------------------------------------------------------------------ */
  nTimeout = 30;  /* Default is 30 seconds  */
  if ((pszTmp = msOWSLookupMetadata2(&(lp->metadata), &(map->web.metadata),
                                    "FO", "connectiontimeout")) != NULL) {
    nTimeout = atoi(pszTmp);
  }

  /*------------------------------------------------------------------
   * Check to see if there's a HTTP Cookie to forward
   * If Cookie differ between the two connection, it's NOT OK to merge
   * the connection
   * ------------------------------------------------------------------ */
  if ((pszTmp = msOWSLookupMetadata(&(lp->metadata),
                                    "FO", "http_cookie")) != NULL) {
    if(strcasecmp(pszTmp, "forward") == 0) {
      pszTmp= msLookupHashTable(&(map->web.metadata),"http_cookie_data");
      if(pszTmp != NULL) {
        pszHTTPCookieData = msStrdup(pszTmp);
      }
    } else {
      pszHTTPCookieData = msStrdup(pszTmp);
    }
  } else if ((pszTmp = msOWSLookupMetadata(&(map->web.metadata),
                       "FO", "http_cookie")) != NULL) {
    if(strcasecmp(pszTmp, "forward") == 0) {
      pszTmp= msLookupHashTable(&(map->web.metadata),"http_cookie_data");
      if(pszTmp != NULL) {
        pszHTTPCookieData = msStrdup(pszTmp);
      }
    } else {
      pszHTTPCookieData = msStrdup(pszTmp);
    }
  }

  /* ------------------------------------------------------------------
   * If nLayerId == -1 then we need to figure it
   * ------------------------------------------------------------------ */
  if (nLayerId == -1) {
    int iLayer;
    for(iLayer=0; iLayer < map->numlayers; iLayer++) {
      if (GET_LAYER(map, iLayer) == lp) {
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

  if (bPostRequest) {
    pasReqInfo[(*numRequests)].pszPostRequest =
      msBuildWFSLayerPostRequest(&bbox, psParams);
    pasReqInfo[(*numRequests)].pszPostContentType =
      msStrdup("text/xml");
  }


  /* We'll store the remote server's response to a tmp file. */
  pasReqInfo[(*numRequests)].pszOutputFile = msTmpFile(map, map->mappath, NULL, "tmp.gml");

  /* TODO: Implement Caching of GML responses. There was an older caching
   * method, but it suffered from a race condition. See #3137.
   */

  pasReqInfo[(*numRequests)].pszHTTPCookieData = pszHTTPCookieData;
  pszHTTPCookieData = NULL;
  pasReqInfo[(*numRequests)].nStatus = 0;
  pasReqInfo[(*numRequests)].nTimeout = nTimeout;
  pasReqInfo[(*numRequests)].bbox = bbox;
  pasReqInfo[(*numRequests)].debug = lp->debug;

  if (msHTTPAuthProxySetup(&(map->web.metadata), &(lp->metadata),
                           pasReqInfo, *numRequests, map, "FO") != MS_SUCCESS) {
    msWFSFreeParamsObj(psParams);
    return MS_FAILURE;
  }
  
  /* ------------------------------------------------------------------
   * Pre-Open the layer now, (i.e. alloc and fill msWFSLayerInfo inside
   * layer obj).  Layer will be ready for use when the main mapserver
   * code calls msLayerOpen().
   * ------------------------------------------------------------------ */
  if (lp->wfslayerinfo != NULL) {
    psInfo =(msWFSLayerInfo*)(lp->wfslayerinfo);
  } else {
    lp->wfslayerinfo = psInfo = msAllocWFSLayerInfo();
  }

  if (psInfo->pszGMLFilename)
    free(psInfo->pszGMLFilename);
  psInfo->pszGMLFilename=msStrdup(pasReqInfo[(*numRequests)].pszOutputFile);

  psInfo->rect = pasReqInfo[(*numRequests)].bbox;

  if (psInfo->pszGetUrl)
    free(psInfo->pszGetUrl);
  psInfo->pszGetUrl = msStrdup(pasReqInfo[(*numRequests)].pszGetUrl);

  psInfo->nStatus = 0;

  (*numRequests)++;

  msWFSFreeParamsObj(psParams);

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
#ifdef USE_WFS_LYR
  if (lp->wfslayerinfo) {
    msWFSLayerInfo *psInfo = NULL;

    psInfo =(msWFSLayerInfo*)(lp->wfslayerinfo);

    /* Copy request results infos to msWFSLayerInfo struct */
    /* For now there is only nStatus, but we should eventually add */
    /* mime type and WFS exceptions information. */
    psInfo->nStatus = pasReqInfo->nStatus;
  }
#else
  /* ------------------------------------------------------------------
   * WFS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.",
             "msWFSUpdateRequestInfo()");
#endif /* USE_WFS_LYR */
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

  if ( msCheckParentPointer(lp->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  if (lp->wfslayerinfo != NULL) {
    psInfo =(msWFSLayerInfo*)lp->wfslayerinfo;

    /* Layer already opened.  If explicit filename requested then check */
    /* that file was already opened with the same filename. */
    /* If no explicit filename requested then we'll try to reuse the */
    /* previously opened layer... this will happen in a msDrawMap() call. */
    if (pszGMLFilename == NULL ||
        (psInfo->pszGMLFilename &&
         strcmp(psInfo->pszGMLFilename, pszGMLFilename) == 0) ) {
      if (lp->layerinfo == NULL) {
        if (msWFSLayerWhichShapes(lp, psInfo->rect, MS_FALSE) == MS_FAILURE) /* no access to context (draw vs. query) here, although I doubt it matters... */
          return MS_FAILURE;
      }
      return MS_SUCCESS;  /* Nothing to do... layer is already opened */
    } else {
      /* Hmmm... should we produce a fatal error? */
      /* For now we'll just close the layer and reopen it. */
      if (lp->debug)
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
    psInfo->pszGMLFilename = msStrdup(pszGMLFilename);
  else {
    psInfo->pszGMLFilename = msTmpFile(lp->map,
                                       lp->map->mappath,
                                       NULL,
                                       "tmp.gml");
  }

  if (defaultBBOX) {
    /* __TODO__ If new bbox differs from current one then we should */
    /* invalidate current GML file in cache */
    psInfo->rect = *defaultBBOX;
  } else {
    /* Use map bbox by default */
    psInfo->rect = lp->map->extent;
  }

  /* We will call whichshapes() now and force downloading layer right */
  /* away.  This saves from having to call DescribeFeatureType and */
  /* parsing the response (being lazy I guess) and anyway given the */
  /* way we work with layers right now the bbox is unlikely to change */
  /* between now and the time whichshapes() would have been called by */
  /* the MapServer core. */

  if((lp->map->projection.numargs > 0) && (lp->projection.numargs > 0))
    msProjectRect(&lp->map->projection, &lp->projection, &psInfo->rect); /* project the searchrect to source coords */

  if (msWFSLayerWhichShapes(lp, psInfo->rect, MS_FALSE) == MS_FAILURE)  /* no access to context (draw vs. query) here, although I doubt it matters... */
    status = MS_FAILURE;


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
 *                          msWFSLayerOpenVT()
 *
 * Overloaded version of msWFSLayerOpen for virtual table architecture
 **********************************************************************/
int
msWFSLayerOpenVT(layerObj *lp)
{
  return msWFSLayerOpen(lp, NULL, NULL);
}

/**********************************************************************
 *                          msWFSLayerIsOpen()
 *
 * Returns MS_TRUE if layer is already open, MS_FALSE otherwise.
 *
 **********************************************************************/

int msWFSLayerIsOpen(layerObj *lp)
{
#ifdef USE_WFS_LYR
  if (lp->wfslayerinfo != NULL)
    return MS_TRUE;

  return MS_FALSE;
#else
  /* ------------------------------------------------------------------
   * WFS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.",
             "msWFSLayerIsOpen()");
  return(MS_FALSE);

#endif /* USE_WFS_LYR */
}

/**********************************************************************
 *                          msWFSLayerInitItemInfo()
 *
 **********************************************************************/

int msWFSLayerInitItemInfo(layerObj *layer)
{
  (void)layer;
  /* Nothing to do here.  OGR will do its own initialization when it */
  /* opens the actual file. */
  /* Note that we didn't implement our own msWFSLayerFreeItemInfo() */
  /* so that the OGR one gets called. */
  return MS_SUCCESS;
}

/**********************************************************************
 *                          msWFSLayerGetShape()
 *
 **********************************************************************/
int msWFSLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
#ifdef USE_WFS_LYR
  msWFSLayerInfo* psInfo = NULL;

  if(layer != NULL && layer->wfslayerinfo != NULL)
    psInfo = (msWFSLayerInfo*)layer->wfslayerinfo;
  else {
    msSetError(MS_WFSERR, "Layer is not opened.", "msWFSLayerGetShape()");
    return MS_FAILURE;
  }

  if(psInfo->bLayerHasValidGML)
    return msOGRLayerGetShape(layer, shape, record);
  else {
    /* Layer is successful, but there is no data to process */
    msFreeShape(shape);
    shape->type = MS_SHAPE_NULL;
    return MS_FAILURE;
  }
#else
  /* ------------------------------------------------------------------
   * WFS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.",
             "msWFSLayerGetShape()");
  return(MS_FAILURE);
#endif /* USE_WFS_LYR */

}

/**********************************************************************
 *                          msWFSLayerGetNextShape()
 *
 **********************************************************************/
int msWFSLayerNextShape(layerObj *layer, shapeObj *shape)
{
#ifdef USE_WFS_LYR
  msWFSLayerInfo* psInfo = NULL;

  if(layer != NULL && layer->wfslayerinfo != NULL)
    psInfo = (msWFSLayerInfo*)layer->wfslayerinfo;
  else {
    msSetError(MS_WFSERR, "Layer is not opened.", "msWFSLayerNextShape()");
    return MS_FAILURE;
  }

  if(psInfo->bLayerHasValidGML)
    return msOGRLayerNextShape(layer, shape);
  else {
    /* Layer is successful, but there is no data to process */
    msFreeShape(shape);
    shape->type = MS_SHAPE_NULL;
    return MS_FAILURE;
  }
#else
  /* ------------------------------------------------------------------
   * WFS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.",
             "msWFSLayerNextShape()");
  return(MS_FAILURE);
#endif /* USE_WFS_LYR */

}

/**********************************************************************
 *                          msWFSLayerGetExtent()
 *
 **********************************************************************/
int msWFSLayerGetExtent(layerObj *layer, rectObj *extent)
{
#ifdef USE_WFS_LYR
  msWFSLayerInfo* psInfo = NULL;

  if(layer != NULL && layer->wfslayerinfo != NULL)
    psInfo = (msWFSLayerInfo*)layer->wfslayerinfo;
  else {
    msSetError(MS_WFSERR, "Layer is not opened.", "msWFSLayerGetExtent()");
    return MS_FAILURE;
  }

  if(psInfo->bLayerHasValidGML)
    return msOGRLayerGetExtent(layer, extent);
  else {
    /* Layer is successful, but there is no data to process */
    msSetError(MS_WFSERR, "Unable to get extents for this layer.", "msWFSLayerGetExtent()");
    return MS_FAILURE;
  }
#else
  /* ------------------------------------------------------------------
   * WFS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.",
             "msWFSLayerGetExtent()");
  return(MS_FAILURE);
#endif /* USE_WFS_LYR */

}

/**********************************************************************
 *                          msWFSLayerGetItems()
 *
 **********************************************************************/

int msWFSLayerGetItems(layerObj *layer)
{
#ifdef USE_WFS_LYR
  /* For now this method simply lets OGR parse the GML and figure the  */
  /* schema itself. */
  /* It could also be implemented to call DescribeFeatureType for */
  /* this layer, but we don't need to do it so why waste resources? */

  msWFSLayerInfo* psInfo = NULL;

  if(layer != NULL && layer->wfslayerinfo != NULL)
    psInfo = (msWFSLayerInfo*)layer->wfslayerinfo;
  else {
    msSetError(MS_WFSERR, "Layer is not opened.", "msWFSLayerGetItems()");
    return MS_FAILURE;
  }

  if(psInfo->bLayerHasValidGML)
    return msOGRLayerGetItems(layer);
  else {
    /* Layer is successful, but there is no data to process */
    layer->numitems = 0;
    layer->items = NULL;
    return MS_SUCCESS;
  }
#else
  /* ------------------------------------------------------------------
   * WFS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.",
             "msWFSLayerGetItems()");
  return(MS_FAILURE);
#endif /* USE_WFS_LYR */

}

/**********************************************************************
 *                          msWFSLayerWhichShapes()
 *
 **********************************************************************/

int msWFSLayerWhichShapes(layerObj *lp, rectObj rect, int isQuery)
{
#ifdef USE_WFS_LYR
  msWFSLayerInfo *psInfo;
  int status = MS_SUCCESS;
  const char *pszTmp;
  FILE *fp;

  if ( msCheckParentPointer(lp->map,"map")==MS_FAILURE )
    return MS_FAILURE;


  psInfo =(msWFSLayerInfo*)lp->wfslayerinfo;

  if (psInfo == NULL) {
    msSetError(MS_WFSCONNERR, "Assertion failed: WFS layer not opened!!!",
               "msWFSLayerWhichShapes()");
    return(MS_FAILURE);
  }

  /* ------------------------------------------------------------------
   * Check if layer overlaps current view window (using wfs_latlonboundingbox)
   * ------------------------------------------------------------------ */
  if ((pszTmp = msOWSLookupMetadata(&(lp->metadata),
                                    "FO", "latlonboundingbox")) != NULL) {
    char **tokens;
    int n;
    rectObj ext;

    tokens = msStringSplit(pszTmp, ' ', &n);
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

    /* Reproject latlonboundingbox to the selected SRS for the layer and */
    /* check if it overlaps the bbox that we calculated for the request */

    msProjectRect(&(lp->map->latlon), &(lp->projection), &ext);
    if (!msRectOverlap(&rect, &ext)) {
      /* No overlap... nothing to do. If layer was never opened, go open it.*/
      if (lp->layerinfo)
        return MS_DONE;  /* No overlap. */
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
  if (psInfo->nStatus == 0) {
    httpRequestObj asReqInfo[2];
    int numReq = 0;

    msHTTPInitRequestObj(asReqInfo, 2);

    if ( msPrepareWFSLayerRequest(-1, lp->map, lp,
                                  asReqInfo, &numReq) == MS_FAILURE  ||
         msOWSExecuteRequests(asReqInfo, numReq,
                              lp->map, MS_TRUE) == MS_FAILURE ) {
      /* Delete tmp file... we don't want it to stick around. */
      unlink(asReqInfo[0].pszOutputFile);
      return MS_FAILURE;
    }

    /* Cleanup */
    msHTTPFreeRequestObj(asReqInfo, numReq);

  }

  if ( !MS_HTTP_SUCCESS( psInfo->nStatus ) ) {
    /* Delete tmp file... we don't want it to stick around. */
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
  if ((fp = fopen(psInfo->pszGMLFilename, "r")) != NULL) {
    char szHeader[2000];
    int  nBytes = 0;

    nBytes = fread( szHeader, 1, sizeof(szHeader)-1, fp );
    fclose(fp);

    if (nBytes < 0)
      nBytes = 0;
    szHeader[nBytes] = '\0';

    if ( nBytes == 0 ) {
      msSetError(MS_WFSCONNERR,
                 "WFS request produced no oputput for layer %s.",
                 "msWFSLayerWhichShapes()",
                 lp->name?lp->name:"(null)");
      return(MS_FAILURE);

    }
    if ( strstr(szHeader, "<WFS_Exception>") ||
         strstr(szHeader, "<ServiceExceptionReport>") ) {
      msOWSProcessException(lp, psInfo->pszGMLFilename,
                            MS_WFSCONNERR, "msWFSLayerWhichShapes()" );
      return MS_FAILURE;
    } else if ( strstr(szHeader,"opengis.net/gml") &&
                strstr(szHeader,"featureMember>") == NULL ) {
      /* This looks like valid GML, but contains 0 features. */
      return MS_DONE;
    } else if ( strstr(szHeader,"opengis.net/gml") == NULL ||
                strstr(szHeader,"featureMember>") == NULL ) {
      /* This is probably just junk. */
      msSetError(MS_WFSCONNERR,
                 "WFS request produced unexpected output (junk?) for layer %s.",
                 "msWFSLayerWhichShapes()",
                 lp->name?lp->name:"(null)");
      return(MS_FAILURE);
    }

    /* If we got this far, it must be a valid GML dataset... keep going */
  }


  /* ------------------------------------------------------------------
   * Open GML file using OGR.
   * ------------------------------------------------------------------ */
  if ((status = msOGRLayerOpen(lp, psInfo->pszGMLFilename)) != MS_SUCCESS)
    return status;

  status = msOGRLayerWhichShapes(lp, rect, isQuery);

  /* Mark that the OGR Layer is valid */
  psInfo->bLayerHasValidGML = MS_TRUE;

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
  if (lp->layerinfo)
    msOGRLayerClose(lp);

  /* ------------------------------------------------------------------
   * Cleanup WFS connection info.
   * __TODO__ For now we flush everything, but we should try to cache some stuff
   * ------------------------------------------------------------------ */
  /* __TODO__ unlink()  .gml file and OGR's schema file if they exist */
  /* unlink( */

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

/**********************************************************************
 *                          msWFSExecuteGetFeature()
 * Returns the temporary gml file name. User shpuld free the return string.
 **********************************************************************/
char *msWFSExecuteGetFeature(layerObj *lp)
{
#ifdef USE_WFS_LYR
  char *gmltmpfile = NULL;
  msWFSLayerInfo *psInfo = NULL;

  if (lp == NULL || lp->connectiontype != MS_WFS)
    return NULL;

  msWFSLayerOpen(lp, NULL, NULL);
  psInfo =(msWFSLayerInfo*)lp->wfslayerinfo;
  if (psInfo &&  psInfo->pszGMLFilename)
    gmltmpfile = msStrdup(psInfo->pszGMLFilename);
  msWFSLayerClose(lp);

  return gmltmpfile;

#else
  /* ------------------------------------------------------------------
   * WFS CONNECTION Support not included...
   * ------------------------------------------------------------------ */
  msSetError(MS_WFSCONNERR, "WFS CLIENT CONNECTION support is not available.",
             "msExecuteWFSGetFeature()");
  return NULL;

#endif /* USE_WFS_LYR */

}

int
msWFSLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerInitItemInfo = msWFSLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msOGRLayerFreeItemInfo; /* yes, OGR */
  layer->vtable->LayerOpen = msWFSLayerOpenVT;
  layer->vtable->LayerIsOpen = msWFSLayerIsOpen;
  layer->vtable->LayerWhichShapes = msWFSLayerWhichShapes;
  layer->vtable->LayerNextShape = msWFSLayerNextShape;
  /* layer->vtable->LayerResultsGetShape = msWFSLayerResultGetShape; */
  /* layer->vtable->LayerGetShapeCount, use default */
  layer->vtable->LayerGetShape = msWFSLayerGetShape;
  layer->vtable->LayerClose = msWFSLayerClose;
  layer->vtable->LayerGetItems = msWFSLayerGetItems;
  layer->vtable->LayerGetExtent = msWFSLayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  /* layer->vtable->LayerCloseConnection, use default */
  layer->vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */
  /* layer->vtable->LayerGetAutoProjection, use defaut*/

  return MS_SUCCESS;
}

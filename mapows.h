/**********************************************************************
 * $Id$
 *
 * mapows.h - OGC Web Services (WMS, WFS, WCS) support function definitions
 *
 **********************************************************************
 * $Log$
 * Revision 1.32  2004/05/03 03:45:42  dan
 * Include map= param in default onlineresource of GetCapabilties if it
 * was explicitly set in QUERY_STRING (bug 643)
 *
 * Revision 1.31  2004/04/19 22:08:39  sdlime
 * Added msOWSGetEPSGProj() to mapows.h/.c and updated the original from mapproject.c to use Dan's namespaces.
 *
 * Revision 1.30  2004/04/14 07:31:40  dan
 * Removed msOWSGetMetadata(), replaced by msOWSLookupMetadata()
 *
 * Revision 1.29  2004/04/14 05:14:54  dan
 * Added ability to pass a default value to msOWSPrintMetadataList() (bug 616)
 *
 * Revision 1.28  2004/04/14 04:54:30  dan
 * Created msOWSLookupMetadata() and added namespaces lookup in all
 * msOWSPrint*Metadata() functions. Also pass namespaces=NULL everywhere
 * that calls those functions for now to avoid breaking something just
 * before the release. (bug 615, 568)
 *
 * Revision 1.27  2004/04/12 18:38:24  assefa
 * Add dll export support for windows.
 *
 * Revision 1.26  2004/03/30 00:12:28  dan
 * Added ability to combine multiple WMS connections to the same server
 * into a single request when the layers are adjacent and compatible.(bug 116)
 *
 * Revision 1.25  2004/03/29 18:34:25  assefa
 * Windows compilation problem : gettimeofday and timval struct (Bug 602)
 *
 * Revision 1.24  2004/03/29 14:41:55  dan
 * Use CURL's internal timer instead of custom gettimeofday() calls for
 * timing WMS/WFS requests
 *
 * Revision 1.23  2004/03/18 23:11:12  dan
 * Added detailed reporting (using msDebug) of layer rendering times
 *
 * Revision 1.22  2004/03/11 22:45:39  dan
 * Added pszPostContentType in httpRequestObj instead of using hardcoded
 * text/html mime type for all post requests.
 *
 * Revision 1.21  2004/02/24 06:20:37  sdlime
 * Added msOWSGetMetadata() function.
 *
 * Revision 1.20  2004/02/05 06:05:54  sdlime
 * Added first WCS prototype to mapows.h
 *
 * Revision 1.19  2003/10/30 22:37:01  assefa
 * Add function msWFSExecuteGetFeature on a wfs layer.
 *
 * Revision 1.18  2003/10/06 13:03:19  assefa
 * Use of namespace. Correct execption return.
 *
 * Revision 1.17  2003/09/19 21:54:19  assefa
 * Add support fot the Post request.
 *
 * Revision 1.16  2003/04/09 07:13:49  dan
 * Added GetContext (custom) request in WMS interface.
 * Added missing gml: namespace in 0.1.7 context output.
 *
 * Revision 1.15  2003/03/26 20:24:38  dan
 * Do not call msDebug() unless debug flag is turned on
 *
 * Revision 1.14  2003/01/10 06:39:06  sdlime
 * Moved msEncodeHTMLEntities() and msDecodeHTMLEntities() from mapows.c 
 * to mapstring.c so they can be used a bit more freely.
 *
 * ...
 *
 * Revision 1.1  2002/09/03 03:19:51  dan
 * Set the bases for WFS Server support + moved some WMS/WFS stuff to mapows.c
 *
 **********************************************************************/

#ifndef MAPOWS_H
#define MAPOWS_H

#include <time.h>

/*====================================================================
 *   maphttp.c
 *====================================================================*/

#define MS_HTTP_SUCCESS(status)  (status == 200 || status == 242)

typedef struct http_request_info
{
    int         nLayerId;
    char      * pszGetUrl;
    char      * pszOutputFile;
    int         nTimeout;
    rectObj     bbox;
    int         nStatus;       /* 200=success, value < 0 if request failed */
    char      * pszContentType;
    char      * pszErrBuf;     /* Buffer where curl can write errors */
    char        *pszPostRequest;     /* post request content (NULL for GET) */
    char        *pszPostContentType; /* post request MIME type */

    /* For debugging/profiling */
    int         debug;         /* Debug mode?  MS_TRUE/MS_FALSE */

    /* Private members */
    void      * curl_handle;   /* CURLM * handle */
    FILE      * fp;            /* FILE * used during download */
} httpRequestObj;

typedef  struct
{
  char *pszVersion;
  char *pszRequest;
  char *pszService;
  char *pszTypeName;
  char *pszFilter;
  int nMaxFeatures;
  char *pszBbox; //only used with a Get Request
  char *pszOutputFormat; //only used with DescibeFeatureType

} wfsParamsObj;


/* wmsParamsObj
 *
 * Used to preprocess WMS request parameters and combine layers that can
 * be comined in a GetMap request.
 */
typedef  struct
{
  char        *onlineresource;
  hashTableObj params;
  int          numparams;
} wmsParamsObj;

int msHTTPInit();
void msHTTPCleanup();

void msHTTPInitRequestObj(httpRequestObj *pasReqInfo, int numRequests);
void msHTTPFreeRequestObj(httpRequestObj *pasReqInfo, int numRequests);
int  msHTTPExecuteRequests(httpRequestObj *pasReqInfo, int numRequests,
                           int bCheckLocalCache);
int  msHTTPGetFile(const char *pszGetUrl, const char *pszOutputFile, 
                   int *pnHTTPStatus, int nTimeout, int bCheckLocalCache,
                   int bDebug);


/*====================================================================
 *   mapows.c
 *====================================================================*/
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR)

MS_DLL_EXPORT int msOWSDispatch(mapObj *map, cgiRequestObj *request);
MS_DLL_EXPORT int msOWSMakeAllLayersUnique(mapObj *map);
MS_DLL_EXPORT char *msOWSGetOnlineResource(mapObj *map, const char *metadata_name, cgiRequestObj *req);
MS_DLL_EXPORT const char *msOWSGetSchemasLocation(mapObj *map);

// OWS_NOERR and OWS_WARN passed as action_if_not_found to printMetadata()
#define OWS_NOERR   0
#define OWS_WARN    1

// OWS_WMS and OWS_WFS used for functions that differ in behavior between
// WMS and WFS services (e.g. msOWSPrintLatLonBoundingBox())
#define OWS_WMS     1
#define OWS_WFS     2

MS_DLL_EXPORT const char * msOWSLookupMetadata(hashTableObj metadata, 
                                    const char *namespaces, const char *name);
MS_DLL_EXPORT int msOWSPrintMetadata(FILE *stream, hashTableObj metadata, 
                       const char *namespaces, const char *name, 
                       int action_if_not_found, const char *format, 
                       const char *default_value);
int msOWSPrintEncodeMetadata(FILE *stream, hashTableObj metadata, 
                             const char *namespaces, const char *name, 
                             int action_if_not_found, 
                             const char *format, const char *default_value) ;
int msOWSPrintGroupMetadata(FILE *stream, mapObj *map, char* pszGroupName, 
                            const char *namespaces, const char *name, 
                            int action_if_not_found, 
                            const char *format, const char *default_value);
int msOWSPrintParam(FILE *stream, const char *name, const char *value, 
                    int action_if_not_found, const char *format, 
                    const char *default_value);
int msOWSPrintMetadataList(FILE *stream, hashTableObj metadata, 
                           const char *namespaces, const char *name, 
                           const char *startTag, 
                           const char *endTag, const char *itemFormat,
                           const char *default_value);
void msOWSPrintLatLonBoundingBox(FILE *stream, const char *tabspace, 
                                 rectObj *extent, projectionObj *srcproj,
                                 int nService);
void msOWSPrintBoundingBox(FILE *stream, const char *tabspace, 
                           rectObj *extent, 
                           projectionObj *srcproj,
                           hashTableObj metadata );
void msOWSPrintContactInfo( FILE *stream, const char *tabspace, 
                           const char *wmtver, hashTableObj metadata );
int msOWSGetLayerExtent(mapObj *map, layerObj *lp, rectObj *ext);
int msOWSExecuteRequests(httpRequestObj *pasReqInfo, int numRequests,
                         mapObj *map, int bCheckLocalCache);
void msOWSProcessException(layerObj *lp, const char *pszFname, 
                           int nErrorCode, const char *pszFuncName);
char *msOWSBuildURLFilename(const char *pszPath, const char *pszURL, 
                            const char *pszExt);
const char *msOWSGetEPSGProj(projectionObj *proj, hashTableObj metadata, const char *namespaces, int bReturnOnlyFirstOne);
#endif

/*====================================================================
 *   mapgml.c
 *====================================================================*/
int msGMLWriteQuery(mapObj *map, char *filename);

#ifdef USE_WFS_SVR
int msGMLWriteWFSQuery(mapObj *map, FILE *stream, int maxfeatures, char *);
#endif


/*====================================================================
 *   mapwms.c
 *====================================================================*/
int msWMSDispatch(mapObj *map, cgiRequestObj *req); 


/*====================================================================
 *   mapwmslayer.c
 *====================================================================*/

int msInitWmsParamsObj(wmsParamsObj *wmsparams);
void msFreeWmsParamsObj(wmsParamsObj *wmsparams);

int msPrepareWMSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             enum MS_CONNECTION_TYPE lastconnectiontype,
                             wmsParamsObj *psLastWMSParams,
                             httpRequestObj *pasReqInfo, int *numRequests);
int msDrawWMSLayerLow(int nLayerId, httpRequestObj *pasReqInfo, 
                      int numRequests, mapObj *map, layerObj *lp, 
                      imageObj *img);
MS_DLL_EXPORT char *msWMSGetFeatureInfoURL(mapObj *map, layerObj *lp,
                             int nClickX, int nClickY, int nFeatureCount,
                             const char *pszInfoFormat); 


/*====================================================================
 *   mapwfs.c
 *====================================================================*/
int msWFSDispatch(mapObj *map, cgiRequestObj *requestobj);
void msWFSParseRequest(cgiRequestObj *, wfsParamsObj *);
wfsParamsObj *msWFSCreateParamsObj();
void msWFSFreeParamsObj(wfsParamsObj *wfsparams);

#ifdef USE_WFS_SVR
const char *msWFSGetGeomElementName(mapObj *map, layerObj *lp);
#endif


/*====================================================================
 *   mapwfslayer.c
 *====================================================================*/

int msPrepareWFSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             httpRequestObj *pasReqInfo, int *numRequests);
void msWFSUpdateRequestInfo(layerObj *lp, httpRequestObj *pasReqInfo);
int msWFSLayerOpen(layerObj *lp, 
                   const char *pszGMLFilename, rectObj *defaultBBOX);
int msWFSLayerInitItemInfo(layerObj *layer);
int msWFSLayerGetItems(layerObj *layer);
int msWFSLayerWhichShapes(layerObj *layer, rectObj rect);
int msWFSLayerClose(layerObj *lp);
MS_DLL_EXPORT char *msWFSExecuteGetFeature(layerObj *lp);

/*====================================================================
 *   mapcontext.c
 *====================================================================*/

MS_DLL_EXPORT int msWriteMapContext(mapObj *map, FILE *stream);
MS_DLL_EXPORT int msSaveMapContext(mapObj *map, char *filename);
MS_DLL_EXPORT int msLoadMapContext(mapObj *map, char *filename);


/*====================================================================
 *   mapwcs.c
 *====================================================================*/

int msWCSDispatch(mapObj *map, cgiRequestObj *requestobj); // only 1 public function

#endif /* MAPOWS_H */


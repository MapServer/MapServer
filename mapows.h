/**********************************************************************
 * $Id$
 *
 * mapows.h - OGC Web Services (WMS, WFS, WCS) support function definitions
 *
 **********************************************************************
 * $Log$
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
#include <sys/time.h>

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

int msOWSDispatch(mapObj *map, cgiRequestObj *request);
const char *msOWSGetMetadata(hashTableObj metadata, ...);
int msOWSMakeAllLayersUnique(mapObj *map);
char *msOWSGetOnlineResource(mapObj *map, const char *metadata_name);
const char *msOWSGetSchemasLocation(mapObj *map);

// OWS_NOERR and OWS_WARN passed as action_if_not_found to printMetadata()
#define OWS_NOERR   0
#define OWS_WARN    1

// OWS_WMS and OWS_WFS used for functions that differ in behavior between
// WMS and WFS services (e.g. msOWSPrintLatLonBoundingBox())
#define OWS_WMS     1
#define OWS_WFS     2

int msOWSPrintMetadata(FILE *stream, hashTableObj metadata, const char *name, 
                       int action_if_not_found, const char *format, 
                       const char *default_value);
int msOWSPrintEncodeMetadata(FILE *stream, hashTableObj metadata, 
                             const char *name, int action_if_not_found, 
                             const char *format, const char *default_value) ;
int msOWSPrintGroupMetadata(FILE *stream, mapObj *map, char* pszGroupName, 
                            const char *name, int action_if_not_found, 
                            const char *format, const char *default_value);
int msOWSPrintParam(FILE *stream, const char *name, const char *value, 
                    int action_if_not_found, const char *format, 
                    const char *default_value);
int msOWSPrintMetadataList(FILE *stream, hashTableObj metadata, 
                           const char *name, const char *startTag, 
                           const char *endTag, const char *itemFormat);
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
int msWMSDispatch(mapObj *map, char **names, char **values, int numentries); 


/*====================================================================
 *   mapwmslayer.c
 *====================================================================*/

int msPrepareWMSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             httpRequestObj *pasReqInfo, int *numRequests);
int msDrawWMSLayerLow(int nLayerId, httpRequestObj *pasReqInfo, 
                      int numRequests, mapObj *map, layerObj *lp, 
                      imageObj *img);
char *msWMSGetFeatureInfoURL(mapObj *map, layerObj *lp,
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
char *msWFSExecuteGetFeature(layerObj *lp);

/*====================================================================
 *   mapcontext.c
 *====================================================================*/

int msWriteMapContext(mapObj *map, FILE *stream);
int msSaveMapContext(mapObj *map, char *filename);
int msLoadMapContext(mapObj *map, char *filename);


/*====================================================================
 *   mapwcs.c
 *====================================================================*/

int msWCSDispatch(mapObj *map, cgiRequestObj *requestobj); // only 1 public function

#endif /* MAPOWS_H */


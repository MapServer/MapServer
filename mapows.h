/**********************************************************************
 * $Id$
 *
 * mapows.h - OGC Web Services (WMS, WFS) support function definitions
 *
 **********************************************************************
 * $Log$
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
 * Moved msEncodeHTMLEntities() and msDecodeHTMLEntities() from mapows.c to mapstring.c so they can be used a bit more freely.
 *
 * Revision 1.13  2002/12/19 06:30:59  dan
 * Enable caching WMS/WFS request using tmp filename built from URL
 *
 * Revision 1.12  2002/12/19 05:17:09  dan
 * Report WFS exceptions, and do not fail on WFS requests returning 0 features
 *
 * Revision 1.11  2002/12/18 16:45:49  dan
 * Fixed WFS capabilities to validate against schema
 *
 * Revision 1.10  2002/12/17 21:33:54  dan
 * Enable following redirections with libcurl (requires libcurl 7.10.1+)
 *
 * Revision 1.9  2002/12/16 20:35:00  dan
 * Flush libwww and use libcurl instead for HTTP requests in WMS/WFS client
 *
 * Revision 1.8  2002/12/13 00:57:31  dan
 * Modified WFS implementation to behave more as a real vector data source
 *
 * Revision 1.7  2002/11/20 21:22:32  dan
 * Added msOWSGetSchemasLocation() for use by both WFS and WMS Map Context
 *
 * Revision 1.6  2002/11/20 17:17:21  julien
 * Support version 0.1.2 of MapContext
 * Remove warning from tags
 * Encode and decode all url
 *
 * Revision 1.5  2002/10/28 20:31:20  dan
 * New support for WMS Map Context (from Julien)
 *
 * Revision 1.2  2002/10/22 20:03:57  julien
 * Add the mapcontext support
 *
 * Revision 1.4  2002/10/09 02:29:03  dan
 * Initial implementation of WFS client support.
 *
 * Revision 1.3  2002/10/08 02:40:08  dan
 * Added WFS DescribeFeatureType
 *
 * Revision 1.2  2002/10/04 21:29:41  dan
 * WFS: Added GetCapabilities and basic GetFeature (still some work to do)
 *
 * Revision 1.1  2002/09/03 03:19:51  dan
 * Set the bases for WFS Server support + moved some WMS/WFS stuff to mapows.c
 *
 **********************************************************************/

#ifndef MAPOWS_H
#define MAPOWS_H

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
    int         debug;         /* Debug mode?  MS_TRUE/MS_FALSE */
    char        *pszPostRequest; /*  post request content */

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

}wfsParamsObj;

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
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR)

int msOWSDispatch(mapObj *map, cgiRequestObj *request);
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


/*====================================================================
 *   mapcontext.c
 *====================================================================*/

int msWriteMapContext(mapObj *map, FILE *stream);
int msSaveMapContext(mapObj *map, char *filename);
int msLoadMapContext(mapObj *map, char *filename);

#endif /* MAPOWS_H */


/*****************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC Web Services (WMS, WFS, WCS) support function definitions
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/


#ifndef MAPOWS_H
#define MAPOWS_H

#include <time.h>

/* This is the URL to the official OGC Schema Repository. We use it by 
 * default for OGC services unless the ows_schemas_lcoation web metadata 
 * is set in the mapfile.
 */
#define OWS_DEFAULT_SCHEMAS_LOCATION   "http://schemas.opengis.net"

/*====================================================================
 *   maphttp.c
 *====================================================================*/

#define MS_HTTP_SUCCESS(status)  (status == 200 || status == 242)

enum MS_HTTP_PROXY_TYPE
{
    MS_HTTP,
    MS_SOCKS5
};

enum MS_HTTP_AUTH_TYPE
{
    MS_BASIC,
    MS_DIGEST,
    MS_NTLM,
    MS_ANY,
    MS_ANYSAFE
};

typedef struct http_request_info
{
    int     nLayerId;
    char    *pszGetUrl;
    char    *pszOutputFile;
    int     nTimeout;
    rectObj bbox;
    int     nStatus;            /* 200=success, value < 0 if request failed */
    char    *pszContentType;    /* Content-Type of the response */
    char    *pszErrBuf;         /* Buffer where curl can write errors */
    char    *pszPostRequest;    /* post request content (NULL for GET) */
    char    *pszPostContentType;/* post request MIME type */
    char    *pszUserAgent;      /* User-Agent, auto-generated if not set */
    char    *pszHTTPCookieData; /* HTTP Cookie data */
    
    char    *pszProxyAddress;   /* The address (IP or hostname) of proxy svr */
    long     nProxyPort;        /* The proxy's port                          */
    enum MS_HTTP_PROXY_TYPE eProxyType; /* The type of proxy                 */
    enum MS_HTTP_AUTH_TYPE  eProxyAuthType; /* Auth method against proxy     */
    char    *pszProxyUsername;  /* Proxy authentication username             */
    char    *pszProxyPassword;  /* Proxy authentication password             */
    
    enum MS_HTTP_AUTH_TYPE eHttpAuthType; /* HTTP Authentication type        */
    char    *pszHttpUsername;   /* HTTP Authentication username              */
    char    *pszHttpPassword;   /* HTTP Authentication password              */

    /* For debugging/profiling */
    int         debug;         /* Debug mode?  MS_TRUE/MS_FALSE */

    /* Private members */
    void      * curl_handle;   /* CURLM * handle */
    FILE      * fp;            /* FILE * used during download */
} httpRequestObj;

typedef struct
{
  char *pszVersion;
  char *pszUpdateSequence;
  char *pszRequest;
  char *pszService;
  char *pszTypeName;
  char *pszFilter;
  int nMaxFeatures;
  char *pszBbox; /* only used with a Get Request */
  char *pszOutputFormat; /* only used with DescibeFeatureType */
  char *pszFeatureId;
  char *pszSrs;
  char *pszResultType;
  char *pszPropertyName;
} wfsParamsObj;

/*
 * sosParamsObj
 * Used to preprocess SOS request parameters
 *
 */

typedef struct {
  char *pszVersion;
  char *pszAcceptVersions;
  char *pszUpdateSequence;
  char *pszRequest;
  char *pszService;
  char *pszOutputFormat;
  char *pszSensorId;
  char *pszProcedure;
  char *pszOffering; 
  char *pszObservedProperty;
  char *pszEventTime;
  char *pszResult;
  char *pszResponseFormat;
  char *pszResultModel;
  char *pszResponseMode;
  char *pszBBox;
  char *pszFeatureOfInterest;
  char *pszSrsName;
} sosParamsObj;

/* wmsParamsObj
 *
 * Used to preprocess WMS request parameters and combine layers that can
 * be comined in a GetMap request.
 */
typedef  struct
{
  char        *onlineresource;
  hashTableObj *params;
  int          numparams;
  char         *httpcookiedata;
} wmsParamsObj;

int msHTTPInit(void);
void msHTTPCleanup(void);

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
MS_DLL_EXPORT int msOWSDispatch(mapObj *map, cgiRequestObj *request, int force_ows_mode);

MS_DLL_EXPORT const char * msOWSLookupMetadata(hashTableObj *metadata, 
                                    const char *namespaces, const char *name);
MS_DLL_EXPORT const char * msOWSLookupMetadata2(hashTableObj *pri,
                                                hashTableObj *sec,
                                                const char *namespaces,
                                                const char *name);

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR) || defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

MS_DLL_EXPORT int msOWSMakeAllLayersUnique(mapObj *map);
MS_DLL_EXPORT int msOWSNegotiateVersion(int requested_version, int supported_versions[], int num_supported_versions);
MS_DLL_EXPORT char *msOWSTerminateOnlineResource(const char *src_url);
MS_DLL_EXPORT char *msOWSGetOnlineResource(mapObj *map, const char *namespaces, const char *metadata_name, cgiRequestObj *req);
MS_DLL_EXPORT const char *msOWSGetSchemasLocation(mapObj *map);
MS_DLL_EXPORT const char *msOWSGetLanguage(mapObj *map, const char *context);

/* Constants for OWS Service version numbers */
#define OWS_0_1_2   0x000102
#define OWS_0_1_4   0x000104
#define OWS_0_1_6   0x000106
#define OWS_0_1_7   0x000107
#define OWS_1_0_0   0x010000
#define OWS_1_0_1   0x010001
#define OWS_1_0_6   0x010006
#define OWS_1_0_7   0x010007
#define OWS_1_0_8   0x010008
#define OWS_1_1_0   0x010100
#define OWS_1_1_1   0x010101
#define OWS_1_3_0   0x010300
#define OWS_VERSION_MAXLEN   20  /* Buffer size for msOWSGetVersionString() */
#define OWS_VERSION_NOTSET    -1
#define OWS_VERSION_BADFORMAT -2

MS_DLL_EXPORT int msOWSParseVersionString(const char *pszVersion);
MS_DLL_EXPORT const char *msOWSGetVersionString(int nVersion, char *pszBuffer);


/* OWS_NOERR and OWS_WARN passed as action_if_not_found to printMetadata() */
#define OWS_NOERR   0
#define OWS_WARN    1

/* OWS_WMS and OWS_WFS used for functions that differ in behavior between */
/* WMS and WFS services (e.g. msOWSPrintLatLonBoundingBox()) */
#define OWS_WMS     1
#define OWS_WFS     2

MS_DLL_EXPORT int msOWSPrintMetadata(FILE *stream, hashTableObj *metadata, 
                       const char *namespaces, const char *name, 
                       int action_if_not_found, const char *format, 
                       const char *default_value);
int msOWSPrintEncodeMetadata(FILE *stream, hashTableObj *metadata, 
                             const char *namespaces, const char *name, 
                             int action_if_not_found, 
                             const char *format, const char *default_value) ;
char *msOWSGetEncodeMetadata(hashTableObj *metadata, 
                             const char *namespaces, const char *name, 
                             const char *default_value);

int msOWSPrintValidateMetadata(FILE *stream, hashTableObj *metadata, 
                               const char *namespaces, const char *name, 
                               int action_if_not_found, 
                               const char *format, const char *default_value);
int msOWSPrintGroupMetadata(FILE *stream, mapObj *map, char* pszGroupName, 
                            const char *namespaces, const char *name, 
                            int action_if_not_found, 
                            const char *format, const char *default_value);
int msOWSPrintURLType(FILE *stream, hashTableObj *metadata, 
                      const char *namespaces, const char *name, 
                      int action_if_not_found, const char *tag_format, 
                      const char *tag_name, const char *type_format, 
                      const char *width_format, const char *height_format, 
                      const char *urlfrmt_format, const char *href_format,
                      int type_is_mandatory, int width_is_mandatory, 
                      int height_is_mandatory, int format_is_mandatory, 
                      int href_is_mandatory, const char *default_type, 
                      const char *default_width, const char *default_height, 
                      const char *default_urlfrmt, const char *default_href, 
                      const char *tabspace);
int msOWSPrintParam(FILE *stream, const char *name, const char *value, 
                    int action_if_not_found, const char *format, 
                    const char *default_value);
int msOWSPrintEncodeParam(FILE *stream, const char *name, const char *value, 
                          int action_if_not_found, const char *format, 
                          const char *default_value);
int msOWSPrintMetadataList(FILE *stream, hashTableObj *metadata, 
                           const char *namespaces, const char *name, 
                           const char *startTag, 
                           const char *endTag, const char *itemFormat,
                           const char *default_value);
int msOWSPrintEncodeMetadataList(FILE *stream, hashTableObj *metadata, 
                                 const char *namespaces, const char *name, 
                                 const char *startTag, 
                                 const char *endTag, const char *itemFormat,
                                 const char *default_value);
int msOWSPrintEncodeParamList(FILE *stream, const char *name, 
                              const char *value, int action_if_not_found, 
                              char delimiter, const char *startTag, 
                              const char *endTag, const char *format, 
                              const char *default_value);
void msOWSPrintLatLonBoundingBox(FILE *stream, const char *tabspace, 
                                 rectObj *extent, projectionObj *srcproj,
                                 projectionObj *wfsproj, int nService);
void msOWSPrintEX_GeographicBoundingBox(FILE *stream, const char *tabspace, 
                                        rectObj *extent, projectionObj *srcproj);

void msOWSPrintBoundingBox(FILE *stream, const char *tabspace, 
                           rectObj *extent, 
                           projectionObj *srcproj,
                           hashTableObj *metadata,
                           const char *namespaces,
                           int wms_version);
void msOWSPrintContactInfo( FILE *stream, const char *tabspace, 
                            int nVersion, hashTableObj *metadata,
                            const char *namespaces  );
int msOWSGetLayerExtent(mapObj *map, layerObj *lp, const char *namespaces, rectObj *ext);
int msOWSExecuteRequests(httpRequestObj *pasReqInfo, int numRequests,
                         mapObj *map, int bCheckLocalCache);
void msOWSProcessException(layerObj *lp, const char *pszFname, 
                           int nErrorCode, const char *pszFuncName);
char *msOWSBuildURLFilename(const char *pszPath, const char *pszURL, 
                            const char *pszExt);
const char *msOWSGetEPSGProj(projectionObj *proj, hashTableObj *metadata, const char *namespaces, int bReturnOnlyFirstOne);
char *msOWSGetProjURN(projectionObj *proj, hashTableObj *metadata, const char *namespaces, int bReturnOnlyFirstOne);

void msOWSGetDimensionInfo(layerObj *layer, const char *pszDimension, 
                           const char **pszDimUserValue, 
                           const char **pszDimUnits, 
                           const char **pszDimDefault, 
                           const char **pszDimNearValue, 
                           const char **pszDimUnitSymbol, 
                           const char **pszDimMultiValue);

int msOWSNegotiateUpdateSequence(const char *requested_updateSequence, const char *updatesequence);

#endif

/*====================================================================
 *   mapgml.c
 *====================================================================*/
#define OWS_GML2 0 /* Supported GML formats */
#define OWS_GML3 1

#define OWS_WFS_FEATURE_COLLECTION_NAME "msFeatureCollection"
#define OWS_GML_DEFAULT_GEOMETRY_NAME "msGeometry"
#define OWS_GML_OCCUR_UNBOUNDED -1

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR)

/* TODO, there must be a better way to generalize these lists of objects... */

typedef struct {
  char *name;     /* name of the item */
  char *alias;    /* is the item aliased for presentation? (NULL if not) */
  char *type;     /* raw type for this item (NULL for a "string") (TODO: should this be a lookup table instead?) */
#ifndef __cplusplus 
  char *template;  /* presentation string for this item, needs to be a complete XML tag */
#else
  char *_template;  /* presentation string for this item, needs to be a complete XML tag */
#endif
  int encode;     /* should the value be HTML encoded? Default is MS_TRUE */
  int visible;    /* should this item be output, default is MS_FALSE */  
} gmlItemObj;

typedef struct {
  gmlItemObj *items;
  int numitems;
} gmlItemListObj;

typedef struct {
  char *name;     /* name of the constant */
  char *type;     /* raw type for this item (NULL for a "string") */
  char *value;    /* output value for this constant (output will look like: <name>value</name>) */
} gmlConstantObj;

typedef struct {
  gmlConstantObj *constants;
  int numconstants;
} gmlConstantListObj;

typedef struct {
  char *name;     /* name of the geometry (type of GML property) */
  char *type;     /* raw type for these geometries (point|multipoint|line|multiline|polygon|multipolygon */
  int occurmin, occurmax;   /* number of occurances (default 0,1) */
} gmlGeometryObj;

typedef struct {
  gmlGeometryObj *geometries;
  int numgeometries;
} gmlGeometryListObj;

typedef struct {
  char *name;     /* name of the group */
  char **items;   /* list of items in the group */
  int numitems;   /* number of items */
  char *type;     /* name of the complex type */
} gmlGroupObj;

typedef struct {
  gmlGroupObj *groups;
  int numgroups;
} gmlGroupListObj;

typedef struct {
	char *prefix;
	char *uri;
	char *schemalocation;
} gmlNamespaceObj;

typedef struct {
  gmlNamespaceObj *namespaces;
  int numnamespaces;
} gmlNamespaceListObj;

MS_DLL_EXPORT int msItemInGroups(char *name, gmlGroupListObj *groupList);
MS_DLL_EXPORT gmlItemListObj *msGMLGetItems(layerObj *layer, const char *metadata_namespaces);
MS_DLL_EXPORT void msGMLFreeItems(gmlItemListObj *itemList);
MS_DLL_EXPORT gmlConstantListObj *msGMLGetConstants(layerObj *layer, const char *metadata_namespaces);
MS_DLL_EXPORT void msGMLFreeConstants(gmlConstantListObj *constantList);
MS_DLL_EXPORT gmlGeometryListObj *msGMLGetGeometries(layerObj *layer, const char *metadata_namespaces);
MS_DLL_EXPORT void msGMLFreeGeometries(gmlGeometryListObj *geometryList);
MS_DLL_EXPORT gmlGroupListObj *msGMLGetGroups(layerObj *layer, const char *metadata_namespaces);
MS_DLL_EXPORT void msGMLFreeGroups(gmlGroupListObj *groupList);
MS_DLL_EXPORT gmlNamespaceListObj *msGMLGetNamespaces(webObj *web, const char *metadata_namespaces);
MS_DLL_EXPORT void msGMLFreeNamespaces(gmlNamespaceListObj *namespaceList);
#endif

/* export to fix bug 851 */
MS_DLL_EXPORT int msGMLWriteQuery(mapObj *map, char *filename, const char *namespaces);


#ifdef USE_WFS_SVR
MS_DLL_EXPORT int msGMLWriteWFSQuery(mapObj *map, FILE *stream, int maxfeatures, char *wfs_namespace, 
                                     int outputformat,  int bUseGetShape);
#endif


/*====================================================================
 *   mapwms.c
 *====================================================================*/
int msWMSDispatch(mapObj *map, cgiRequestObj *req); 
MS_DLL_EXPORT int msWMSLoadGetMapParams(mapObj *map, int nVersion, char **names, 
                                        char **values, int numentries, char *wms_exception_format);


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

/* Supported DescribeFeature formats */
#define OWS_DEFAULT_SCHEMA 0 /* basically a GML 2.1 schema */
#define OWS_SFE_SCHEMA 1 /* GML for simple feature exchange (formerly GML3L0) */

int msWFSDispatch(mapObj *map, cgiRequestObj *requestobj);
void msWFSParseRequest(cgiRequestObj *, wfsParamsObj *);
wfsParamsObj *msWFSCreateParamsObj(void);
void msWFSFreeParamsObj(wfsParamsObj *wfsparams);
int msWFSIsLayerSupported(layerObj *lp);
int msWFSException(mapObj *map, const char *locator, const char *code, 
                   const char *version);

#ifdef USE_WFS_SVR
const char *msWFSGetGeomElementName(mapObj *map, layerObj *lp);

int msWFSException11(mapObj *map, const char *locator, 
                     const char *exceptionCode, const char *version);
int msWFSGetCapabilities11(mapObj *map, wfsParamsObj *wfsparams, 
                           cgiRequestObj *req); 

#endif

/*====================================================================
 *   mapwfslayer.c
 *====================================================================*/

int msPrepareWFSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             httpRequestObj *pasReqInfo, int *numRequests);
void msWFSUpdateRequestInfo(layerObj *lp, httpRequestObj *pasReqInfo);
int msWFSLayerOpen(layerObj *lp, 
                   const char *pszGMLFilename, rectObj *defaultBBOX);
int msWFSLayerIsOpen(layerObj *lp); 
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
MS_DLL_EXPORT int msLoadMapContext(mapObj *map, char *filename, int unique_layer_names);
MS_DLL_EXPORT int msLoadMapContextURL(mapObj *map, char *urlfilename, int unique_layer_names);


/*====================================================================
 *   mapwcs.c
 *====================================================================*/

int msWCSDispatch(mapObj *map, cgiRequestObj *requestobj); /* only 1 public function */



/*====================================================================
 *   mapogsos.c
 *====================================================================*/

int msSOSDispatch(mapObj *map, cgiRequestObj *requestobj); /* only 1 public function */


#endif /* MAPOWS_H */


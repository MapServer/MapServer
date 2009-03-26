/**********************************************************************
 * $Id$
 *
 * mapows.h - OGC Web Services (WMS, WFS, WCS) support function definitions
 *
 **********************************************************************
 * $Log$
 * Revision 1.78  2006/08/28 21:16:21  assefa
 * add flag USE_SOS_SVR as part of the flags enabling ows utility functions
 * to be defined.
 *
 * Revision 1.77  2006/08/23 18:28:00  dan
 * Use OWS_DEFAULT_SCHEMAS_LOCATION #define instead of hardcoded string (bug 1873)
 *
 * Revision 1.76  2006/08/22 04:45:06  sdlime
 * Fixed a bug that did not allow for seperate metadata namespaces to be used for WMS vs. WFS for GML transformations (e.g. WFS_GEOMETRIES, WMS_GEOMETRIES).
 *
 * Revision 1.75  2006/08/14 18:47:28  sdlime
 * Some more implementation of external application schema support.
 *
 * Revision 1.74  2006/08/11 18:56:40  sdlime
 * Initial versions of namespace read/free functions.
 *
 * Revision 1.73  2006/08/11 18:40:20  sdlime
 * Created gmlNamespaceObj structures.
 *
 * Revision 1.72  2006/06/06 14:21:01  frank
 * ensure msOWSDispatch() always available
 *
 * Revision 1.71  2006/04/17 19:06:49  dan
 * Set User-Agent in HTTP headers of client WMS/WFS connections (bug 1749)
 *
 * Revision 1.70  2006/03/14 04:08:34  assefa
 * Add disptach call to SOS service.
 *
 * Revision 1.69  2006/02/14 03:39:30  julien
 * Update to MapContext 1.1.0, add dimensions support in context bug 1581
 *
 * Revision 1.68  2006/02/07 17:40:23  sdlime
 * Reverting to Dan's original solution for the template member.
 *
 * Revision 1.67  2006/02/07 16:13:37  sdlime
 * Renamed gmlItemObj template to tmplate to avoid c++ compile errors.
 *
 * Revision 1.66  2006/02/07 13:45:32  dan
 * Use _template instead of template in gmlItemObj in C++
 *
 * Revision 1.65  2006/02/06 19:50:41  sdlime
 * Added ability to define a item template for GML output, best for use with application schema. A templated attribute is: 1) not queryable and 2) not output in the server produced schema. The layer namespace and item value can be accessed via the template: e.g. gml_area_template '<$namespace:area>$value</$namespace:area>
 *
 * Revision 1.64  2006/01/05 16:25:10  assefa
 * Correct mapscript windows build problem when flag USE_WMS_SVR was
 * not set (Bug 1529)
 *
 * Revision 1.63  2005/10/25 20:29:52  sdlime
 * Completed work to add constants to GML output. For example gml_constants 'aConstant'  gml_aConstant_value 'this is a constant', which results in output like <aConstant>this is a constant</aConstant>. Constants can appear in groups and can havespecific types (default is string). Constants are NOT queryable so their use should be limited untilsome extensions to wfs 1.1 appear that will allow us to mark certain elements as queryable or not in capabilities output.
 *
 * Revision 1.62  2005/10/24 23:53:47  sdlime
 * A few comment fixes.
 *
 * Revision 1.61  2005/10/24 23:49:55  sdlime
 * Added gmlConstantObj and gmlConstantListObj to mapows.h. These create a mechanism to output constant values as part of GML outout (and schema creation) without defining new attributes in the source data.
 *
 * Revision 1.60  2005/10/12 15:34:27  sdlime
 * Updated the gmlGroupObj to allow you to set the group type. This impacts schema location. If not set the complex type written by the schema generator is 'groupnameType' and via metadata you can override that (e.g. gml_groupname_type 'MynameType').
 *
 * Revision 1.59  2005/06/02 20:32:01  sdlime
 * Changed metadata reference from ...getfeature_collection to ...feature_collection.
 *
 * Revision 1.58  2005/06/02 20:25:04  sdlime
 * Updated WFS output to not use wfs:FeatureCollection as the main container for GML3 output. A default container of msFeatureCollection is provided or the user may define one explicitly.
 *
 * Revision 1.57  2005/05/31 18:24:49  sdlime
 * Updated GML3 writer to use the new gmlGeometryListObj. This allows you to package geometries from WFS in a pretty flexible manner. Will port GML2 writer once testing on GML3 code is complete.
 *
 * Revision 1.56  2005/05/31 05:49:31  sdlime
 * Added geometry metadata processing functions to mapgml.c.
 *
 * Revision 1.55  2005/05/31 05:21:17  sdlime
 * Added couple structures for managing GML/WFS geometry types.
 *
 * Revision 1.54  2005/05/24 18:52:45  julien
 * Bug 1149: From WMS 1.1.1, SRS are given in individual tags.
 *
 * Revision 1.53  2005/05/23 17:41:37  sdlime
 * Added constants for the 2 types of schemas WFS servers will support. A default schema that references GML 2.1 geometries, and another for schemas complient with the GML profile for simple feature exchange document recently published (formerly known as GML3L0).
 *
 * Revision 1.52  2005/05/23 17:31:27  sdlime
 * Move GML metadata structures and functions prototypes to mapows.h since they will be needed by mapwfs.c as well.
 *
 * Revision 1.51  2005/05/13 17:23:34  dan
 * First pass at properly handling XML exceptions from CONNECTIONTYPE WMS
 * layers. Still needs some work. (bug 1246)
 *
 * Revision 1.50  2005/04/21 21:10:38  sdlime
 * Adjusted WFS support to allow for a new output format (GML3).
 *
 * Revision 1.49  2005/02/18 03:06:46  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
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

/* This is the URL to the official OGC Schema Repository. We use it by 
 * default for OGC services unless the ows_schemas_lcoation web metadata 
 * is set in the mapfile.
 */
#define OWS_DEFAULT_SCHEMAS_LOCATION   "http://schemas.opengis.net"

/*====================================================================
 *   maphttp.c
 *====================================================================*/

#define MS_HTTP_SUCCESS(status)  (status == 200 || status == 242)

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
  char *pszBbox; /* only used with a Get Request */
  char *pszOutputFormat; /* only used with DescibeFeatureType */

} wfsParamsObj;


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
MS_DLL_EXPORT int msOWSDispatch(mapObj *map, cgiRequestObj *request);

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

MS_DLL_EXPORT int msOWSMakeAllLayersUnique(mapObj *map);
MS_DLL_EXPORT char *msOWSTerminateOnlineResource(const char *src_url);
MS_DLL_EXPORT char *msOWSGetOnlineResource(mapObj *map, const char *namespaces, const char *metadata_name, cgiRequestObj *req);
MS_DLL_EXPORT const char *msOWSGetSchemasLocation(mapObj *map);

/* Constants for OWS Service version numbers */
#define OWS_0_1_2   0x000102
#define OWS_0_1_4   0x000104
#define OWS_0_1_6   0x000106
#define OWS_0_1_7   0x000107
#define OWS_1_0_0   0x010000
#define OWS_1_0_6   0x010006
#define OWS_1_0_7   0x010007
#define OWS_1_0_8   0x010008
#define OWS_1_1_0   0x010100
#define OWS_1_1_1   0x010101
#define OWS_VERSION_MAXLEN   20  /* Buffer size for msOWSGetVersionString() */

MS_DLL_EXPORT int msOWSParseVersionString(const char *pszVersion);
MS_DLL_EXPORT const char *msOWSGetVersionString(int nVersion, char *pszBuffer);


/* OWS_NOERR and OWS_WARN passed as action_if_not_found to printMetadata() */
#define OWS_NOERR   0
#define OWS_WARN    1

/* OWS_WMS and OWS_WFS used for functions that differ in behavior between */
/* WMS and WFS services (e.g. msOWSPrintLatLonBoundingBox()) */
#define OWS_WMS     1
#define OWS_WFS     2

MS_DLL_EXPORT const char * msOWSLookupMetadata(hashTableObj *metadata, 
                                    const char *namespaces, const char *name);
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
                                 int nService);
void msOWSPrintBoundingBox(FILE *stream, const char *tabspace, 
                           rectObj *extent, 
                           projectionObj *srcproj,
                           hashTableObj *metadata,
                           const char *namespaces);
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

void msOWSGetDimensionInfo(layerObj *layer, const char *pszDimension, 
                           const char **pszDimUserValue, 
                           const char **pszDimUnits, 
                           const char **pszDimDefault, 
                           const char **pszDimNearValue, 
                           const char **pszDimUnitSymbol, 
                           const char **pszDimMultiValue);

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
MS_DLL_EXPORT int msGMLWriteWFSQuery(mapObj *map, FILE *stream, int maxfeatures, char *wfs_namespace, int outputformat);
#endif


/*====================================================================
 *   mapwms.c
 *====================================================================*/
int msWMSDispatch(mapObj *map, cgiRequestObj *req); 
MS_DLL_EXPORT int msWMSLoadGetMapParams(mapObj *map, int nVersion,
                          char **names, char **values, int numentries);


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


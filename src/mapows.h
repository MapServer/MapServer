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

#include "maphttp.h"
#include <time.h>

/* This is the URL to the official OGC Schema Repository. We use it by
 * default for OGC services unless the ows_schemas_lcoation web metadata
 * is set in the mapfile.
 */
#define OWS_DEFAULT_SCHEMAS_LOCATION "http://schemas.opengis.net"

#if defined USE_LIBXML2 && defined USE_WFS_SVR
#include <libxml/tree.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*====================================================================
 *   mapows.c
 *====================================================================*/

typedef struct {
  char *pszVersion;
  char *pszUpdateSequence;
  char *pszRequest;
  char *pszService;
  char *pszTypeName;
  char *pszFilter;
  char *pszFilterLanguage;
  char *pszGeometryName;
  int nMaxFeatures;
  char *pszBbox; /* only used with a Get Request */
  char *pszOutputFormat;
  char *pszFeatureId;
  char *pszSrs;
  char *pszResultType;
  char *pszPropertyName;
  int nStartIndex;
  char *pszAcceptVersions;
  char *pszSections;
  char *pszSortBy;         /* Not implemented yet */
  char *pszLanguage;       /* Inspire extension */
  char *pszValueReference; /* For GetValueReference */
  char *pszStoredQueryId;  /* For DescribeStoredQueries */
  int countGetFeatureById; /* Number of
                              urn:ogc:def:query:OGC-WFS::GetFeatureById
                              GetFeature requests */
  int bHasPostStoredQuery; /* TRUE if a XML GetFeature StoredQuery is present */
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
typedef struct {
  char *onlineresource;
  hashTableObj *params;
  int numparams;
  char *httpcookiedata;
} wmsParamsObj;

/* metadataParamsObj: Represent a metadata specific request with its enabled
 * layers */
typedef struct {
  char *pszRequest;
  char *pszLayer;
  char *pszOutputSchema;
} metadataParamsObj;

/* owsRequestObj: Represent a OWS specific request with its enabled layers */
typedef struct {
  int numlayers;
  int numwmslayerargs;
  int *enabled_layers;
  int *layerwmsfilterindex;

  char *service;
  char *version;
  char *request;
  void *document; /* xmlDocPtr or CPLXMLNode* */
} owsRequestObj;

MS_DLL_EXPORT int msOWSDispatch(mapObj *map, cgiRequestObj *request,
                                int ows_mode);

MS_DLL_EXPORT const char *msOWSLookupMetadata(hashTableObj *metadata,
                                              const char *namespaces,
                                              const char *name);
MS_DLL_EXPORT const char *
msOWSLookupMetadataWithLanguage(hashTableObj *metadata, const char *namespaces,
                                const char *name,
                                const char *validated_language);
MS_DLL_EXPORT const char *msOWSLookupMetadata2(hashTableObj *pri,
                                               hashTableObj *sec,
                                               const char *namespaces,
                                               const char *name);

MS_DLL_EXPORT int
msUpdateGMLFieldMetadata(layerObj *layer, const char *field_name,
                         const char *gml_type, const char *gml_width,
                         const char *gml_precision, const short nullable);

void msOWSInitRequestObj(owsRequestObj *ows_request);
void msOWSClearRequestObj(owsRequestObj *ows_request);

MS_DLL_EXPORT int msOWSRequestIsEnabled(mapObj *map, layerObj *layer,
                                        const char *namespaces,
                                        const char *name, int check_all_layers);
MS_DLL_EXPORT void msOWSRequestLayersEnabled(mapObj *map,
                                             const char *namespaces,
                                             const char *request,
                                             owsRequestObj *request_layers);
MS_DLL_EXPORT int msOWSParseRequestMetadata(const char *metadata,
                                            const char *request, int *disabled);

/* Constants for OWS Service version numbers */
#define OWS_0_1_2 0x000102
#define OWS_0_1_4 0x000104
#define OWS_0_1_6 0x000106
#define OWS_0_1_7 0x000107
#define OWS_1_0_0 0x010000
#define OWS_1_0_1 0x010001
#define OWS_1_0_6 0x010006
#define OWS_1_0_7 0x010007
#define OWS_1_0_8 0x010008
#define OWS_1_1_0 0x010100
#define OWS_1_1_1 0x010101
#define OWS_1_1_2 0x010102
#define OWS_1_3_0 0x010300
#define OWS_2_0_0 0x020000
#define OWS_2_0_1 0x020001
#define OWS_VERSION_MAXLEN 20 /* Buffer size for msOWSGetVersionString() */
#define OWS_VERSION_NOTSET -1
#define OWS_VERSION_BADFORMAT -2

MS_DLL_EXPORT int msOWSParseVersionString(const char *pszVersion);
MS_DLL_EXPORT const char *msOWSGetVersionString(int nVersion, char *pszBuffer);

MS_DLL_EXPORT const char *msOWSGetLanguage(mapObj *map, const char *context);
MS_DLL_EXPORT char *msOWSGetOnlineResource(mapObj *map, const char *namespaces,
                                           const char *metadata_name,
                                           cgiRequestObj *req);
MS_DLL_EXPORT const char *msOWSGetSchemasLocation(mapObj *map);
MS_DLL_EXPORT char *msOWSTerminateOnlineResource(const char *src_url);

void msOWSGetEPSGProj(projectionObj *proj, hashTableObj *metadata,
                      const char *namespaces, int bReturnOnlyFirstOne,
                      char **epsgProj);
void msOWSProjectToWGS84(projectionObj *srcproj, rectObj *ext);

#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR) || defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

MS_DLL_EXPORT int msOWSMakeAllLayersUnique(mapObj *map);
MS_DLL_EXPORT int msOWSNegotiateVersion(int requested_version,
                                        const int supported_versions[],
                                        int num_supported_versions);
MS_DLL_EXPORT char *msOWSGetOnlineResource2(mapObj *map, const char *namespaces,
                                            const char *metadata_name,
                                            cgiRequestObj *req,
                                            const char *validated_language);
MS_DLL_EXPORT const char *msOWSGetInspireSchemasLocation(mapObj *map);
MS_DLL_EXPORT char **msOWSGetLanguageList(mapObj *map, const char *namespaces,
                                          int *numitems);
MS_DLL_EXPORT char *msOWSGetLanguageFromList(mapObj *map,
                                             const char *namespaces,
                                             const char *requested_language);
MS_DLL_EXPORT char *msOWSLanguageNegotiation(mapObj *map,
                                             const char *namespaces,
                                             char **accept_languages,
                                             int num_accept_languages);

/* OWS_NOERR and OWS_WARN passed as action_if_not_found to printMetadata() */
#define OWS_NOERR 0
#define OWS_WARN 1

/* OWS_WMS and OWS_WFS used for functions that differ in behavior between */
/* WMS and WFS services (e.g. msOWSPrintLatLonBoundingBox()) */
typedef enum { OWS_WMS = 1, OWS_WFS = 2, OWS_WCS = 3 } OWSServiceType;

MS_DLL_EXPORT int msOWSPrintInspireCommonExtendedCapabilities(
    FILE *stream, mapObj *map, const char *namespaces, int action_if_not_found,
    const char *tag_name, const char *tag_ns, const char *validated_language,
    const OWSServiceType service);
int msOWSPrintInspireCommonMetadata(FILE *stream, mapObj *map,
                                    const char *namespaces,
                                    int action_if_not_found,
                                    const OWSServiceType service);
int msOWSPrintInspireCommonLanguages(FILE *stream, mapObj *map,
                                     const char *namespaces,
                                     int action_if_not_found,
                                     const char *validated_language);

MS_DLL_EXPORT int msOWSPrintMetadata(FILE *stream, hashTableObj *metadata,
                                     const char *namespaces, const char *name,
                                     int action_if_not_found,
                                     const char *format,
                                     const char *default_value);
int msOWSPrintEncodeMetadata(FILE *stream, hashTableObj *metadata,
                             const char *namespaces, const char *name,
                             int action_if_not_found, const char *format,
                             const char *default_value);
int msOWSPrintEncodeMetadata2(FILE *stream, hashTableObj *metadata,
                              const char *namespaces, const char *name,
                              int action_if_not_found, const char *format,
                              const char *default_value,
                              const char *validated_language);
char *msOWSGetEncodeMetadata(hashTableObj *metadata, const char *namespaces,
                             const char *name, const char *default_value);

int msOWSPrintValidateMetadata(FILE *stream, hashTableObj *metadata,
                               const char *namespaces, const char *name,
                               int action_if_not_found, const char *format,
                               const char *default_value);
int msOWSPrintGroupMetadata(FILE *stream, mapObj *map, char *pszGroupName,
                            const char *namespaces, const char *name,
                            int action_if_not_found, const char *format,
                            const char *default_value);
int msOWSPrintGroupMetadata2(FILE *stream, mapObj *map, char *pszGroupName,
                             const char *namespaces, const char *name,
                             int action_if_not_found, const char *format,
                             const char *default_value,
                             const char *validated_language);
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
                           const char *startTag, const char *endTag,
                           const char *itemFormat, const char *default_value);
int msOWSPrintEncodeMetadataList(FILE *stream, hashTableObj *metadata,
                                 const char *namespaces, const char *name,
                                 const char *startTag, const char *endTag,
                                 const char *itemFormat,
                                 const char *default_value);
int msOWSPrintEncodeParamList(FILE *stream, const char *name, const char *value,
                              int action_if_not_found, char delimiter,
                              const char *startTag, const char *endTag,
                              const char *format, const char *default_value);
void msOWSPrintLatLonBoundingBox(FILE *stream, const char *tabspace,
                                 rectObj *extent, projectionObj *srcproj,
                                 projectionObj *wfsproj,
                                 OWSServiceType nService);
void msOWSPrintEX_GeographicBoundingBox(FILE *stream, const char *tabspace,
                                        rectObj *extent,
                                        projectionObj *srcproj);

void msOWSPrintBoundingBox(FILE *stream, const char *tabspace, rectObj *extent,
                           projectionObj *srcproj, hashTableObj *layer_meta,
                           hashTableObj *map_meta, const char *namespaces,
                           int wms_version);
void msOWSPrintContactInfo(FILE *stream, const char *tabspace, int nVersion,
                           hashTableObj *metadata, const char *namespaces);
int msOWSGetLayerExtent(mapObj *map, layerObj *lp, const char *namespaces,
                        rectObj *ext);

int msOWSExecuteRequests(httpRequestObj *pasReqInfo, int numRequests,
                         mapObj *map, int bCheckLocalCache);

void msOWSProcessException(layerObj *lp, const char *pszFname, int nErrorCode,
                           const char *pszFuncName);
char *msOWSBuildURLFilename(const char *pszPath, const char *pszURL,
                            const char *pszExt);
char *msOWSGetProjURN(projectionObj *proj, hashTableObj *metadata,
                      const char *namespaces, int bReturnOnlyFirstOne);
char *msOWSGetProjURI(projectionObj *proj, hashTableObj *metadata,
                      const char *namespaces, int bReturnOnlyFirstOne);

void msOWSGetDimensionInfo(layerObj *layer, const char *pszDimension,
                           const char **pszDimUserValue,
                           const char **pszDimUnits, const char **pszDimDefault,
                           const char **pszDimNearValue,
                           const char **pszDimUnitSymbol,
                           const char **pszDimMultiValue);

int msOWSNegotiateUpdateSequence(const char *requested_updateSequence,
                                 const char *updatesequence);

outputFormatObj *msOwsIsOutputFormatValid(mapObj *map, const char *format,
                                          hashTableObj *metadata,
                                          const char *namespaces,
                                          const char *name);
#endif /* #if any wxs service enabled */

/*====================================================================
 *   mapgml.c
 *====================================================================*/

typedef enum {
  OWS_GML2, /* 2.1.2 */
  OWS_GML3, /* 3.1.1 */
  OWS_GML32 /* 3.2.1 */
} OWSGMLVersion;

#define OWS_WFS_FEATURE_COLLECTION_NAME "msFeatureCollection"
#define OWS_GML_DEFAULT_GEOMETRY_NAME "msGeometry"
#define OWS_GML_OCCUR_UNBOUNDED -1

/* TODO, there must be a better way to generalize these lists of objects... */

typedef struct {
  char *name;  /* name of the item */
  char *alias; /* is the item aliased for presentation? (NULL if not) */
  char *type; /* raw type for this item (NULL for a "string") (TODO: should this
                 be a lookup table instead?) */
#ifndef __cplusplus
  char *template; /* presentation string for this item, needs to be a complete
                     XML tag */
#else
  char *_template; /* presentation string for this item, needs to be a complete
                      XML tag */
#endif
  int encode;    /* should the value be HTML encoded? Default is MS_TRUE */
  int visible;   /* should this item be output, default is MS_FALSE */
  int width;     /* field width, zero if unknown */
  int precision; /* field precision (decimal places), zero if unknown or N/A */
  int outputByDefault; /* whether this should be output in a GetFeature without
                          PropertyName. MS_TRUE by default, unless
                          gml_default_items is specified and the item name is
                          not in it */
  int minOccurs; /* 0 by default. Can be set to 1 by specifying item name in
                    gml_mandatory_items */
} gmlItemObj;

typedef struct {
  gmlItemObj *items;
  int numitems;
} gmlItemListObj;

typedef struct {
  char *name;  /* name of the constant */
  char *type;  /* raw type for this item (NULL for a "string") */
  char *value; /* output value for this constant (output will look like:
                  <name>value</name>) */
} gmlConstantObj;

typedef struct {
  gmlConstantObj *constants;
  int numconstants;
} gmlConstantListObj;

typedef struct {
  char *name;             /* name of the geometry (type of GML property) */
  char *type;             /* raw type for these geometries
                             (point|multipoint|line|multiline|polygon|multipolygon */
  int occurmin, occurmax; /* number of occurances (default 0,1) */
} gmlGeometryObj;

typedef struct {
  gmlGeometryObj *geometries;
  int numgeometries;
} gmlGeometryListObj;

typedef struct {
  char *name;   /* name of the group */
  char **items; /* list of items in the group */
  int numitems; /* number of items */
  char *type;   /* name of the complex type */
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

MS_DLL_EXPORT gmlItemListObj *msGMLGetItems(layerObj *layer,
                                            const char *metadata_namespaces);
MS_DLL_EXPORT void msGMLFreeItems(gmlItemListObj *itemList);

#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR)

MS_DLL_EXPORT int msItemInGroups(const char *name, gmlGroupListObj *groupList);
MS_DLL_EXPORT gmlConstantListObj *
msGMLGetConstants(layerObj *layer, const char *metadata_namespaces);
MS_DLL_EXPORT void msGMLFreeConstants(gmlConstantListObj *constantList);
MS_DLL_EXPORT gmlGeometryListObj *
msGMLGetGeometries(layerObj *layer, const char *metadata_namespaces,
                   int bWithDefaultGeom);
MS_DLL_EXPORT void msGMLFreeGeometries(gmlGeometryListObj *geometryList);
MS_DLL_EXPORT gmlNamespaceListObj *
msGMLGetNamespaces(webObj *web, const char *metadata_namespaces);
MS_DLL_EXPORT void msGMLFreeNamespaces(gmlNamespaceListObj *namespaceList);
#endif

MS_DLL_EXPORT gmlGroupListObj *msGMLGetGroups(layerObj *layer,
                                              const char *metadata_namespaces);
MS_DLL_EXPORT void msGMLFreeGroups(gmlGroupListObj *groupList);

/* export to fix bug 851 */
MS_DLL_EXPORT int msGMLWriteQuery(mapObj *map, char *filename,
                                  const char *namespaces);

#ifdef USE_WFS_SVR

void msGMLWriteWFSBounds(mapObj *map, FILE *stream, const char *tab,
                         OWSGMLVersion outputformat, int nWFSVersion,
                         int bUseURN);

MS_DLL_EXPORT int msGMLWriteWFSQuery(mapObj *map, FILE *stream,
                                     const char *wfs_namespace,
                                     OWSGMLVersion outputformat,
                                     int nWFSVersion, int bUseURN,
                                     int bGetPropertyValueRequest);
#endif

/*====================================================================
 *   mapwms.c
 *====================================================================*/
int msWMSDispatch(mapObj *map, cgiRequestObj *req, owsRequestObj *ows_request,
                  int force_wms_mode);
MS_DLL_EXPORT int msWMSLoadGetMapParams(mapObj *map, int nVersion, char **names,
                                        char **values, int numentries,
                                        const char *wms_exception_format,
                                        const char *wms_request,
                                        owsRequestObj *ows_request);

/*====================================================================
 *   mapwmslayer.c
 *====================================================================*/

#define WMS_GETMAP 1
#define WMS_GETFEATUREINFO 2
#define WMS_GETLEGENDGRAPHIC 3

int msInitWmsParamsObj(wmsParamsObj *wmsparams);
void msFreeWmsParamsObj(wmsParamsObj *wmsparams);

int msPrepareWMSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             int nRequestType,
                             enum MS_CONNECTION_TYPE lastconnectiontype,
                             wmsParamsObj *psLastWMSParams, int nClickX,
                             int nClickY, int nFeatureCount,
                             const char *pszInfoFormat,
                             httpRequestObj *pasReqInfo, int *numRequests);
int msDrawWMSLayerLow(int nLayerId, httpRequestObj *pasReqInfo, int numRequests,
                      mapObj *map, layerObj *lp, imageObj *img);
MS_DLL_EXPORT char *msWMSGetFeatureInfoURL(mapObj *map, layerObj *lp,
                                           int nClickX, int nClickY,
                                           int nFeatureCount,
                                           const char *pszInfoFormat);
int msWMSLayerExecuteRequest(mapObj *map, int nOWSLayers, int nClickX,
                             int nClickY, int nFeatureCount,
                             const char *pszInfoFormat, int type);

/*====================================================================
 *   mapmetadata.c
 *====================================================================*/

metadataParamsObj *msMetadataCreateParamsObj(void);
void msMetadataFreeParamsObj(metadataParamsObj *metadataparams);
int msMetadataDispatch(mapObj *map, cgiRequestObj *requestobj);
void msMetadataSetGetMetadataURL(layerObj *lp, const char *url);

/*====================================================================
 *   mapwfs.c
 *====================================================================*/

MS_DLL_EXPORT int msWFSDispatch(mapObj *map, cgiRequestObj *requestobj,
                                owsRequestObj *ows_request, int force_wfs_mode);
wfsParamsObj *msWFSCreateParamsObj(void);
int msWFSHandleUpdateSequence(mapObj *map, wfsParamsObj *wfsparams,
                              const char *pszFunction);
void msWFSFreeParamsObj(wfsParamsObj *wfsparams);
int msIsLayerSupportedForWFSOrOAPIF(layerObj *lp);
int msWFSException(mapObj *map, const char *locator, const char *code,
                   const char *version);

#ifdef USE_WFS_SVR
const char *msWFSGetGeomElementName(mapObj *map, layerObj *lp);

int msWFSException11(mapObj *map, const char *locator,
                     const char *exceptionCode, const char *version);
int msWFSGetCapabilities11(mapObj *map, wfsParamsObj *wfsparams,
                           cgiRequestObj *req, owsRequestObj *ows_request);
#ifdef USE_LIBXML2
xmlNodePtr msWFSDumpLayer11(mapObj *map, layerObj *lp, xmlNsPtr psNsOws,
                            int nWFSVersion, const char *validate_language,
                            char *script_url);
#endif
char *msWFSGetOutputFormatList(mapObj *map, layerObj *layer, int nWFSVersion);

int msWFSException20(mapObj *map, const char *locator,
                     const char *exceptionCode);
int msWFSGetCapabilities20(mapObj *map, wfsParamsObj *params,
                           cgiRequestObj *req, owsRequestObj *ows_request);
int msWFSListStoredQueries20(mapObj *map, owsRequestObj *ows_request);
int msWFSDescribeStoredQueries20(mapObj *map, wfsParamsObj *params,
                                 owsRequestObj *ows_request);
char *msWFSGetResolvedStoredQuery20(mapObj *map, wfsParamsObj *wfsparams,
                                    const char *id, hashTableObj *hashTable);

#endif

/*====================================================================
 *   mapwfslayer.c
 *====================================================================*/

int msPrepareWFSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             httpRequestObj *pasReqInfo, int *numRequests);
void msWFSUpdateRequestInfo(layerObj *lp, httpRequestObj *pasReqInfo);
int msWFSLayerOpen(layerObj *lp, const char *pszGMLFilename,
                   rectObj *defaultBBOX);
int msWFSLayerIsOpen(layerObj *lp);
int msWFSLayerInitItemInfo(layerObj *layer);
int msWFSLayerGetItems(layerObj *layer);
int msWFSLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery);
int msWFSLayerClose(layerObj *lp);
MS_DLL_EXPORT char *msWFSExecuteGetFeature(layerObj *lp);

/*====================================================================
 *   mapcontext.c
 *====================================================================*/

MS_DLL_EXPORT int msWriteMapContext(mapObj *map, FILE *stream);
MS_DLL_EXPORT int msSaveMapContext(mapObj *map, char *filename);
MS_DLL_EXPORT int msLoadMapContext(mapObj *map, const char *filename,
                                   int unique_layer_names);
MS_DLL_EXPORT int msLoadMapContextURL(mapObj *map, char *urlfilename,
                                      int unique_layer_names);

/*====================================================================
 *   mapwcs.c
 *====================================================================*/

int msWCSDispatch(mapObj *map, cgiRequestObj *requestobj,
                  owsRequestObj *ows_request); /* only 1 public function */

/*====================================================================
 *   mapogsos.c
 *====================================================================*/

int msSOSDispatch(mapObj *map, cgiRequestObj *requestobj,
                  owsRequestObj *ows_request); /* only 1 public function */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MAPOWS_H */

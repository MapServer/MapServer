/**********************************************************************
 * $Id$
 *
 * mapows.h - OGC Web Services (WMS, WFS) support function definitions
 *
 **********************************************************************
 * $Log$
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
 *   mapows.c
 *====================================================================*/
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR)

int msOWSDispatch(mapObj *map, char **names, char **values, int numentries); 
int msOWSMakeAllLayersUnique(mapObj *map);
char *msOWSGetOnlineResource(mapObj *map, const char *metadata_name);

// OWS_NOERR and OWS_WARN passed as action_if_not_found to printMetadata()
#define OWS_NOERR   0
#define OWS_WARN    1

int msOWSPrintMetadata(hashTableObj metadata, const char *name, 
                       int action_if_not_found, const char *format, 
                       const char *default_value);
int msOWSPrintGroupMetadata(mapObj *map, char* pszGroupName, const char *name, 
                            int action_if_not_found, const char *format, 
                            const char *default_value);
int msOWSPrintParam(const char *name, const char *value, 
                    int action_if_not_found, const char *format, 
                    const char *default_value);
int msOWSPrintMetadataList(hashTableObj metadata, const char *name, 
                           const char *startTag, const char *endTag,
                           const char *itemFormat);
void msOWSPrintLatLonBoundingBox(const char *tabspace, 
                                 rectObj *extent, projectionObj *srcproj);
void msOWSPrintBoundingBox(const char *tabspace, 
                           rectObj *extent, 
                           projectionObj *srcproj,
                           hashTableObj metadata );
char *msEncodeHTMLEntities(const char *string);
int msOWSGetLayerExtent(mapObj *map, layerObj *lp, rectObj *ext);

#endif

/*====================================================================
 *   mapgml.c
 *====================================================================*/
int msGMLWriteQuery(mapObj *map, char *filename);
int msGMLWriteWFSQuery(mapObj *map, FILE *stream);


/*====================================================================
 *   mapwms.c
 *====================================================================*/
int msWMSDispatch(mapObj *map, char **names, char **values, int numentries); 



/*====================================================================
 *   mapwmslayer.c
 *====================================================================*/

typedef struct http_request_info
{
    int         nLayerId;
    void      * request;  /* HTRequest * */
    char      * pszGetUrl;
    char      * pszOutputFile;
    int         nStatus;
    int         nTimeout;
    rectObj     bbox;
} httpRequestObj;

void msFreeRequestObj(httpRequestObj *pasReqInfo, int numRequests);
int msPrepareWMSLayerRequest(int nLayerId, mapObj *map, layerObj *lp,
                             httpRequestObj *pasReqInfo, int *numRequests);
int msWMSExecuteRequests(httpRequestObj *pasReqInfo, int numRequests);
int msDrawWMSLayerLow(int nLayerId, httpRequestObj *pasReqInfo, 
                      int numRequests, mapObj *map, layerObj *lp, 
                      imageObj *img);

char *msWMSGetFeatureInfoURL(mapObj *map, layerObj *lp,
                             int nClickX, int nClickY, int nFeatureCount,
                             const char *pszInfoFormat); 


/*====================================================================
 *   mapwfs.c
 *====================================================================*/
int msWFSDispatch(mapObj *map, char **names, char **values, int numentries); 



/*====================================================================
 *   mapwfslayer.c
 *====================================================================*/


#endif /* MAPOWS_H */


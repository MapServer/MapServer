/**********************************************************************
 * $Id$
 *
 * mapows.h - OGC Web Services (WMS, WFS) support function definitions
 *
 **********************************************************************
 * $Log$
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

#endif

/*====================================================================
 *   mapgml.c
 *====================================================================*/
int msGMLWriteQuery(mapObj *map, char *filename);


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


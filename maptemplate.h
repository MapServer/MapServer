#ifndef MAPTEMPLATE_H
#define MAPTEMPLATE_H

/*!
 * $Id$
*/

#include "map.h"

#define IDSIZE 128
#define TEMPLATE_TYPE(s)  (((strncmp("http://", s, 7) == 0) || (strncmp("https://", s, 8) == 0) || (strncmp("ftp://", s, 6)) == 0)  ? MS_URL : MS_FILE)
#define MAXZOOM 25
#define MINZOOM -25

enum modes {
     BROWSE,
     ZOOMIN,
     ZOOMOUT,
     MAP,
     LEGEND,
     REFERENCE,
     SCALEBAR,
     COORDINATE,
     PROCESSING,
     QUERY,
     QUERYMAP,
     NQUERY,
     NQUERYMAP,
     ITEMQUERY,
     ITEMQUERYMAP,
     ITEMNQUERY,
     ITEMNQUERYMAP,
     FEATUREQUERY,
     FEATUREQUERYMAP,
     FEATURENQUERY,
     FEATURENQUERYMAP,
     ITEMFEATUREQUERY,
     ITEMFEATUREQUERYMAP,ITEMFEATURENQUERY,ITEMFEATURENQUERYMAP,INDEXQUERY,INDEXQUERYMAP};

/*! \srtuct mapservObj
 *  \brief Global structur used by template and mapserv
 * 
 * This structur was created to seperate template functionality
 * from the main mapserv file. Instead of moving all template
 * related functions in a new file (maptemplate.c) and change
 * their signatures to pass all global variables, we created this
 * structure with all global variables needed by template.
*/
typedef struct
{
   /// should the query and/or map be saved
   int SaveMap, SaveQuery;

   char **ParamNames;
   char **ParamValues;
   int NumParams;

   mapObj *Map;

   char *Layers[MS_MAXLAYERS];
   
   /// number of layers specfied by a use
   int NumLayers;

   layerObj *ResultLayer;

   shapeObj SelectShape, ResultShape;

   rectObj RawExt;

   pointObj MapPnt;

   /// default for browsing
   double fZoom, Zoom;
   
   /// whether zooming in or out, default is pan or 0
   int ZoomDirection; 

   /// can be BROWSE, QUERY, etc.
   int Mode; 
   
   /// big enough for time + pid
   char Id[IDSIZE]; 

   /* 
    ** variables for multiple query results processing 
    */
   int RN; /// overall result number
   int LRN; /// result number within a layer
   int NL; /// total number of layers with results
   int NR; /// total number or results
   int NLR; /// number of results in a layer
} mapservObj;

/*! \fn msAllocMapServObj
 * Allocate memory for all variables in strusture
 * and initiate default values
*/
mapservObj* msAllocMapServObj();

/*! \fn msFreeMapServObj
 * free all variables in structure
*/
void msFreeMapServObj(mapservObj* msObj);

// For Mapserv.c
int isOn(mapservObj* msObj, char *name, char *group);

int msReturnPage(mapservObj* msObj, char* , int);
int msReturnURL(mapservObj* msObj, char*, int);
int msReturnQuery(mapservObj* msObj);

#endif

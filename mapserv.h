#ifndef MAPSERV_H
#define MAPSERV_H

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#endif

#include <ctype.h>
#include <time.h>

#include "map.h"

#include "cgiutil.h"

/*
** Defines
*/
#define MAXIMGSIZE 1024
#define MAXCLICK 2*MAXIMGSIZE
#define MINCLICK -MAXCLICK
#define MAXZOOM 25
#define MINZOOM -25
#define NUMEXP "[-]?(([0-9]+)|([0-9]*[.][0-9]+)([eE][-+]?[0-9]+)?)"
#define EXTENT_PADDING .05
#define IDSIZE 128

extern int enter_string;

/*
** Macros
*/
#define TEMPLATE_TYPE(s)  (((strncmp("http://", s, 7) == 0) || (strncmp("https://", s, 8) == 0) || (strncmp("ftp://", s, 6)) == 0)  ? MS_URL : MS_FILE)

/*
** Enumerated types (PROCESSING is a EGIS mode only), keep the query modes in sequence and at the end of the enumeration
*/
int numModes = 27;
enum modes {BROWSE,ZOOMIN,ZOOMOUT,MAP,LEGEND,REFERENCE,SCALEBAR,COORDINATE,PROCESSING,QUERY,QUERYMAP,NQUERY,NQUERYMAP,ITEMQUERY,ITEMQUERYMAP,ITEMNQUERY,ITEMNQUERYMAP,FEATUREQUERY,FEATUREQUERYMAP,FEATURENQUERY,FEATURENQUERYMAP,ITEMFEATUREQUERY,ITEMFEATUREQUERYMAP,ITEMFEATURENQUERY,ITEMFEATURENQUERYMAP,INDEXQUERY,INDEXQUERYMAP};
static char *modeStrings[27] = {"BROWSE","ZOOMIN","ZOOMOUT","MAP","LEGEND","REFERENCE","SCALEBAR","COORDINATE","PROCESSING","QUERY","QUERYMAP","NQUERY","NQUERYMAP","ITEMQUERY","ITEMQUERYMAP","ITEMNQUERY","ITEMNQUERYMAP","FEATUREQUERY","FEATUREQUERYMAP","FEATURENQUERY","FEATURENQUERYMAP","ITEMFEATUREQUERY","ITEMFEATUREQUERYMAP","ITEMFEATURENQUERY","ITEMFEATURENQUERYMAP","INDEXQUERY","INDEXQUERYMAP"};

enum coordSources {NONE, FROMIMGPNT, FROMIMGBOX, FROMIMGSHAPE, FROMREFPNT, FROMUSERPNT, FROMUSERBOX, FROMUSERSHAPE, FROMBUF, FROMSCALE};

/*
** Global variables
*/
int UseShapes=MS_FALSE; // are results of a query to be used in calculating an extent of some sort
int SearchMap=MS_FALSE; // apply pan/zoom BEFORE doing the query (e.g. query the output image rather than the input image)
int SaveMap=MS_FALSE, SaveQuery=MS_FALSE; // should the query and/or map be saved 

char **ParamNames=NULL;
char **ParamValues=NULL;
int NumParams=0;

mapObj *Map=NULL;

char *Layers[MS_MAXLAYERS];
int NumLayers=0; /* number of layers specfied by a user */

char *QueryFile=NULL;
char *QueryLayer=NULL, *SelectLayer=NULL;
int QueryLayerIndex=-1, SelectLayerIndex=-1;
char *Item=NULL, *Value=NULL;

layerObj *ResultLayer=NULL;

shapeObj SelectShape, ResultShape;

int ShapeIndex=-1, TileIndex=-1;

rectObj RawExt={-1.0,-1.0,-1.0,-1.0};

int ImgRows=-1, ImgCols=-1;
rectObj ImgExt={-1.0,-1.0,-1.0,-1.0}; /* Existing image's mapextent */
rectObj ImgBox={-1.0,-1.0,-1.0,-1.0};
pointObj MapPnt={-1.0, -1.0};
pointObj RefPnt={-1.0, -1.0};
pointObj ImgPnt={-1.0, -1.0};

int QueryCoordSource=NONE, CoordSource=NONE;

double Buffer=0, Scale=0; /* used to create a map extent around a point */
double fZoom=1, Zoom=1; /* default for browsing */
int ZoomDirection=0; /* whether zooming in or out, default is pan or 0 */
int ZoomSize=0; /* zoom absolute magnitude (i.e. > 0) */

int Mode=BROWSE; /* can be BROWSE, QUERY, etc. */
char Id[IDSIZE]="\0"; /* big enough for time + pid */

/* 
** variables for multiple query results processing 
*/
int RN=0; /* overall result number */
int LRN=0; /* result number within a layer */
int NL=0; /* total number of layers with results */
int NR=0; /* total number or results */
int NLR=0; /* number of results in a layer */

#endif /* MAPSERV_H */

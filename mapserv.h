#ifndef MAPSERV_H
#define MAPSERV_H

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#endif

#include <ctype.h>
#include <time.h>

#include "maptemplate.h"

#include "cgiutil.h"
/*
** Defines
*/
#define NUMEXP "[-]?(([0-9]+)|([0-9]*[.][0-9]+)([eE][-+]?[0-9]+)?)"
#define EXTENT_PADDING .05


extern int enter_string;

/*
** Macros
*/
#define TEMPLATE_TYPE(s)  (((strncmp("http://", s, 7) == 0) || (strncmp("https://", s, 8) == 0) || (strncmp("ftp://", s, 6)) == 0)  ? MS_URL : MS_FILE)

/*
** Enumerated types (PROCESSING is a EGIS mode only), keep the query modes in sequence and at the end of the enumeration
*/
int numModes = 27;

static char *modeStrings[27] = {"BROWSE","ZOOMIN","ZOOMOUT","MAP","LEGEND","REFERENCE","SCALEBAR","COORDINATE","PROCESSING","QUERY","QUERYMAP","NQUERY","NQUERYMAP","ITEMQUERY","ITEMQUERYMAP","ITEMNQUERY","ITEMNQUERYMAP","FEATUREQUERY","FEATUREQUERYMAP","FEATURENQUERY","FEATURENQUERYMAP","ITEMFEATUREQUERY","ITEMFEATUREQUERYMAP","ITEMFEATURENQUERY","ITEMFEATURENQUERYMAP","INDEXQUERY","INDEXQUERYMAP"};

enum coordSources {NONE, FROMIMGPNT, FROMIMGBOX, FROMIMGSHAPE, FROMREFPNT, FROMUSERPNT, FROMUSERBOX, FROMUSERSHAPE, FROMBUF, FROMSCALE};

/*
** Global variables
*/
int UseShapes=MS_FALSE; // are results of a query to be used in calculating an extent of some sort
int SearchMap=MS_FALSE; // apply pan/zoom BEFORE doing the query (e.g. query the output image rather than the input image)

char *QueryFile=NULL;
char *QueryLayer=NULL, *SelectLayer=NULL;
int QueryLayerIndex=-1, SelectLayerIndex=-1;
char *Item=NULL, *Value=NULL;

int ShapeIndex=-1, TileIndex=-1;

int ImgRows=-1, ImgCols=-1;
rectObj ImgExt={-1.0,-1.0,-1.0,-1.0}; /* Existing image's mapextent */
rectObj ImgBox={-1.0,-1.0,-1.0,-1.0};

pointObj RefPnt={-1.0, -1.0};
pointObj ImgPnt={-1.0, -1.0};

int QueryCoordSource=NONE, CoordSource=NONE;

double Buffer=0, Scale=0; /* used to create a map extent around a point */

int ZoomSize=0; /* zoom absolute magnitude (i.e. > 0) */


#endif /* MAPSERV_H */

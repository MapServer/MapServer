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
** Enumerated types, keep the query modes in sequence and at the end of the enumeration (mode enumeration is in maptemplate.h).
*/
int numModes = 26;
static char *modeStrings[26] = {"BROWSE","ZOOMIN","ZOOMOUT","MAP","LEGEND","REFERENCE","SCALEBAR","COORDINATE",
                                "QUERY","QUERYMAP","NQUERY","NQUERYMAP",
			        "ITEMQUERY","ITEMQUERYMAP","ITEMNQUERY","ITEMNQUERYMAP",
				"FEATUREQUERY","FEATUREQUERYMAP","FEATURENQUERY","FEATURENQUERYMAP",
				"ITEMFEATUREQUERY","ITEMFEATUREQUERYMAP","ITEMFEATURENQUERY","ITEMFEATURENQUERYMAP",
				"INDEXQUERY","INDEXQUERYMAP"};

/*
** Global variables
*/
int SearchMap=MS_FALSE; // apply pan/zoom BEFORE doing the query (e.g. query the output image rather than the input image)

char *QueryFile=NULL;
char *QueryLayer=NULL, *SelectLayer=NULL;
int QueryLayerIndex=-1, SelectLayerIndex=-1;

char *QueryItem=NULL, *QueryString=NULL;

int ShapeIndex=-1, TileIndex=-1;

int QueryCoordSource=NONE;

int ZoomSize=0; /* zoom absolute magnitude (i.e. > 0) */

#endif /* MAPSERV_H */

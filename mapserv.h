/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Declarations supporting mapserv.c.
 * Author:   Steve Lime and the MapServer team.
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

#ifndef MAPSERV_H
#define MAPSERV_H

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#endif

#include <ctype.h>
#include <time.h>

#include "maptemplate.h"
#include "maptile.h"

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
int numModes = 29;
static char *modeStrings[29] = {"BROWSE","ZOOMIN","ZOOMOUT","MAP","LEGEND","LEGENDICON","REFERENCE","SCALEBAR","COORDINATE",
                                "QUERY","QUERYMAP","NQUERY","NQUERYMAP",
			        "ITEMQUERY","ITEMQUERYMAP","ITEMNQUERY","ITEMNQUERYMAP",
				"FEATUREQUERY","FEATUREQUERYMAP","FEATURENQUERY","FEATURENQUERYMAP",
				"ITEMFEATUREQUERY","ITEMFEATUREQUERYMAP","ITEMFEATURENQUERY","ITEMFEATURENQUERYMAP",
				"INDEXQUERY","INDEXQUERYMAP","TILE","OWS"};

/*
** Global variables
*/
int SearchMap=MS_FALSE; /* apply pan/zoom BEFORE doing the query (e.g. query the output image rather than the input image) */

char *QueryFile=NULL;
char *QueryLayer=NULL, *SelectLayer=NULL;
int QueryLayerIndex=-1, SelectLayerIndex=-1;

char *QueryItem=NULL, *QueryString=NULL;

int ShapeIndex=-1, TileIndex=-1;

int QueryCoordSource=NONE;

int ZoomSize=0; /* zoom absolute magnitude (i.e. > 0) */

#endif /* MAPSERV_H */

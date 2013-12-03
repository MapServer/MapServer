/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  PostGIS CONNECTIONTYPE support.
 * Author:   Paul Ramsey <pramsey@cleverelephant.ca>
 *           Dave Blasby <dblasby@gmail.com>
 *
 ******************************************************************************
 * Copyright (c) 2010 Paul Ramsey
 * Copyright (c) 2002 Refractions Research
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

#ifdef USE_POSTGIS

#include "libpq-fe.h"

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 2
#endif

/* HEX = 16 or BASE64 = 64*/
#define TRANSFER_ENCODING 16

/* Substitution token for box hackery */
#define BOXTOKEN "!BOX!"
#define BOXTOKENLENGTH 5

/*
** msPostGISLayerInfo
**
** Specific information needed for managing this layer.
*/
typedef struct {
  char        *sql;        /* SQL query to send to database */
  PGconn      *pgconn;     /* Connection to database */
  long        rownum;      /* What row is the next to be read (for random access) */
  PGresult    *pgresult;   /* For fetching rows from the database */
  char        *uid;        /* Name of user-specified unique identifier, if set */
  char        *srid;       /* Name of user-specified SRID: zero-length => calculate; non-zero => use this value! */
  char        *geomcolumn; /* Specified geometry column, eg "THEGEOM from thetable" */
  char        *fromsource; /* Specified record source, ed "thegeom from THETABLE" or "thegeom from (SELECT..) AS FOO" */
  int         endian;      /* Endianness of the mapserver host */
  int         version;     /* PostGIS version of the database */
  int         paging;      /* Driver handling of pagination, enabled by default */
  int         force2d;     /* Pass geometry through ST_Force2D */
}
msPostGISLayerInfo;


/*
** Utility structure for handling the WKB returned by the database while
** reading.
*/
typedef struct {
  char *wkb; /* Pointer to front of WKB */
  char *ptr; /* Pointer to current write point */
  size_t size; /* Size of allocated space */
  int *typemap; /* Look-up array to valid OGC types */
} wkbObj;

/*
** Utility structure used when building up stroked lines while
** handling curved feature types.
*/
typedef struct {
  pointObj *data; /* Re-sizeable point buffer */
  int npoints;  /* How many points are we currently storing */
  int maxpoints; /* How big is our point buffer */
} pointArrayObj;

/*
** All the WKB type numbers from the OGC
*/
typedef enum {
  WKB_POINT=1,
  WKB_LINESTRING=2,
  WKB_POLYGON=3,
  WKB_MULTIPOINT=4,
  WKB_MULTILINESTRING=5,
  WKB_MULTIPOLYGON=6,
  WKB_GEOMETRYCOLLECTION=7,
  WKB_CIRCULARSTRING=8,
  WKB_COMPOUNDCURVE=9,
  WKB_CURVEPOLYGON=10,
  WKB_MULTICURVE=11,
  WKB_MULTISURFACE=12
} wkb_typenum;

/*
** See below.
*/
#define WKB_TYPE_COUNT 16

/*
** Map the WKB type numbers returned by PostGIS < 2.0 to the
** valid OGC numbers
*/
static int wkb_postgis15[WKB_TYPE_COUNT] = {
  0,
  WKB_POINT,
  WKB_LINESTRING,
  WKB_POLYGON,
  WKB_MULTIPOINT,
  WKB_MULTILINESTRING,
  WKB_MULTIPOLYGON,
  WKB_GEOMETRYCOLLECTION,
  WKB_CIRCULARSTRING,
  WKB_COMPOUNDCURVE,
  0,0,0,
  WKB_CURVEPOLYGON,
  WKB_MULTICURVE,
  WKB_MULTISURFACE
};

/*
** Map the WKB type numbers returned by PostGIS >= 2.0 to the
** valid OGC numbers
*/
static int wkb_postgis20[WKB_TYPE_COUNT] = {
  0,
  WKB_POINT,
  WKB_LINESTRING,
  WKB_POLYGON,
  WKB_MULTIPOINT,
  WKB_MULTILINESTRING,
  WKB_MULTIPOLYGON,
  WKB_GEOMETRYCOLLECTION,
  WKB_CIRCULARSTRING,
  WKB_COMPOUNDCURVE,
  WKB_CURVEPOLYGON,
  WKB_MULTICURVE,
  WKB_MULTISURFACE,
  0,0,0
};


/*
** Prototypes
*/
void msPostGISFreeLayerInfo(layerObj *layer);
msPostGISLayerInfo *msPostGISCreateLayerInfo(void);
char *msPostGISBuildSQL(layerObj *layer, rectObj *rect, long *uid);
int msPostGISParseData(layerObj *layer);
int arcStrokeCircularString(wkbObj *w, double segment_angle, lineObj *line);
int wkbConvGeometryToShape(wkbObj *w, shapeObj *shape);
pointArrayObj* pointArrayNew(int maxpoints);
void pointArrayFree(pointArrayObj *d);

#endif /* USE_POSTGIS */


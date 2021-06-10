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

/*
** Some theory of operation:
**
** Build SQL from DATA statement and LAYER state. SQL is always of the form:
**
**    SELECT [this, that, other], geometry, uid
**      FROM [table|(subquery) as sub]
**      WHERE [box] AND [filter]
**
** So the geometry always resides at layer->numitems and the uid always
** resides at layer->numitems + 1
**
** Geometry is requested as Hex encoded WKB. The endian is always requested
** as the client endianness.
**
** msPostGISLayerWhichShapes creates SQL based on DATA and LAYER state,
** executes it, and places the un-read PGresult handle in the layerinfo->pgresult,
** setting the layerinfo->rownum to 0.
**
** msPostGISNextShape reads a row, increments layerinfo->rownum, and returns
** MS_SUCCESS, until rownum reaches ntuples, and it returns MS_DONE instead.
**
*/

/* GNU needs this for strcasestr */
#define _GNU_SOURCE
/* required for MSVC */
#define _USE_MATH_DEFINES

#include <assert.h>
#include <string.h>
#include <math.h>
#include "mapserver.h"
#include "maptime.h"
#include "mappostgis.h"
#include "mapows.h"

#define FP_EPSILON 1e-12
#define FP_EQ(a, b) (fabs((a)-(b)) < FP_EPSILON)
#define FP_LEFT -1
#define FP_RIGHT 1
#define FP_COLINEAR 0

#define SEGMENT_ANGLE 10.0
#define SEGMENT_MINPOINTS 10

#define WKBZOFFSET_NONISO 0x80000000
#define WKBMOFFSET_NONISO 0x40000000

#define HAS_Z   0x1
#define HAS_M   0x2

#if TRANSFER_ENCODING == 256
#define RESULTSET_TYPE 1
#else
#define RESULTSET_TYPE 0
#endif

#ifdef USE_POSTGIS



/*
** msPostGISCloseConnection()
**
** Handler registered witih msConnPoolRegister so that Mapserver
** can clean up open connections during a shutdown.
*/
void msPostGISCloseConnection(void *pgconn)
{
  PQfinish((PGconn*)pgconn);
}

/*
** msPostGISCreateLayerInfo()
*/
msPostGISLayerInfo *msPostGISCreateLayerInfo(void)
{
  msPostGISLayerInfo *layerinfo = msSmallMalloc(sizeof(msPostGISLayerInfo));
  layerinfo->sql = NULL;
  layerinfo->srid = NULL;
  layerinfo->uid = NULL;
  layerinfo->pgconn = NULL;
  layerinfo->pgresult = NULL;
  layerinfo->geomcolumn = NULL;
  layerinfo->fromsource = NULL;
  layerinfo->endian = 0;
  layerinfo->rownum = 0;
  layerinfo->version = 0;
  layerinfo->paging = MS_TRUE;
#ifdef USE_POINT_Z_M
  layerinfo->force2d = MS_FALSE;
#else
  layerinfo->force2d = MS_TRUE;
#endif
  return layerinfo;
}

/*
** msPostGISFreeLayerInfo()
*/
void msPostGISFreeLayerInfo(layerObj *layer)
{
  msPostGISLayerInfo *layerinfo = NULL;
  layerinfo = (msPostGISLayerInfo*)layer->layerinfo;
  if ( layerinfo->sql ) free(layerinfo->sql);
  if ( layerinfo->uid ) free(layerinfo->uid);
  if ( layerinfo->srid ) free(layerinfo->srid);
  if ( layerinfo->geomcolumn ) free(layerinfo->geomcolumn);
  if ( layerinfo->fromsource ) free(layerinfo->fromsource);
  if ( layerinfo->pgresult ) PQclear(layerinfo->pgresult);
  if ( layerinfo->pgconn ) msConnPoolRelease(layer, layerinfo->pgconn);
  free(layerinfo);
  layer->layerinfo = NULL;
}


/*
** postgresqlNoticeHandler()
**
** Propagate messages from the database to the Mapserver log,
** set in PQsetNoticeProcessor during layer open.
*/
void postresqlNoticeHandler(void *arg, const char *message)
{
  layerObj *lp;
  lp = (layerObj*)arg;

  if (lp->debug) {
    msDebug("%s\n", message);
  }
}


/*
** Expandable pointObj array. The lineObj unfortunately
** is not useful for this purpose, so we have this one.
*/
pointArrayObj*
pointArrayNew(int maxpoints)
{
  pointArrayObj *d = msSmallMalloc(sizeof(pointArrayObj));
  if ( maxpoints < 1 ) maxpoints = 1; /* Avoid a degenerate case */
  d->maxpoints = maxpoints;
  d->data = msSmallMalloc(maxpoints * sizeof(pointObj));
  d->npoints = 0;
  return d;
}

/*
** Utility function to creal up the pointArrayObj
*/
void
pointArrayFree(pointArrayObj *d)
{
  if ( ! d ) return;
  if ( d->data ) free(d->data);
  free(d);
}

/*
** Add a pointObj to the pointObjArray, allocating
** extra storage space if we've used up our existing
** buffer.
*/
static int
pointArrayAddPoint(pointArrayObj *d, const pointObj *p)
{
  if ( !p || !d ) return MS_FAILURE;
  /* Avoid overwriting memory buffer */
  if ( d->maxpoints - d->npoints == 0 ) {
    d->maxpoints *= 2;
    d->data = realloc(d->data, d->maxpoints * sizeof(pointObj));
  }
  d->data[d->npoints] = *p;
  d->npoints++;
  return MS_SUCCESS;
}

/*
** Pass an input type number through the PostGIS version
** type map array to handle the pre-2.0 incorrect WKB types
*/

static int
wkbTypeMap(wkbObj *w, int type, int* pnZMFlag)
{
  *pnZMFlag = 0;
  /* PostGIS >= 2 : ISO SQL/MM style Z types ? */
  if( type >= 1000 && type < 2000 )
  {
    type -= 1000;
    *pnZMFlag = HAS_Z;
  }
  /* PostGIS >= 2 : ISO SQL/MM style M types ? */
  else if( type >= 2000 && type < 3000 )
  {
    type -= 2000;
    *pnZMFlag = HAS_M;
  }
  /* PostGIS >= 2 : ISO SQL/MM style ZM types ? */
  else if( type >= 3000 && type < 4000 )
  {
    type -= 3000;
    *pnZMFlag = HAS_Z | HAS_M;
  }
  /* PostGIS 1.X EWKB : Extended WKB Z or ZM ? */
  else if( (type & WKBZOFFSET_NONISO) != 0 )
  {
    if( (type & WKBMOFFSET_NONISO) != 0 )
        *pnZMFlag = HAS_Z | HAS_M;
    else
        *pnZMFlag = HAS_Z;
    type &= 0x00FFFFFF;
  }
  /* PostGIS 1.X EWKB: Extended WKB M ? */
  else if( (type & WKBMOFFSET_NONISO) != 0 )
  {
    *pnZMFlag = HAS_M;
    type &= 0x00FFFFFF;
  }
  if ( type >= 0 && type < WKB_TYPE_COUNT )
    return w->typemap[type];
  else
    return 0;
}

/*
** Read the WKB type number from a wkbObj without
** advancing the read pointer.
*/
static int
wkbType(wkbObj *w, int* pnZMFlag)
{
  int t;
  memcpy(&t, (w->ptr + 1), sizeof(int));
  return wkbTypeMap(w,t, pnZMFlag);
}

/*
** Read the type number of the first element of a
** collection without advancing the read pointer.
*/
static int
wkbCollectionSubType(wkbObj *w, int* pnZMFlag)
{
  int t;
  memcpy(&t, (w->ptr + 1 + 4 + 4 + 1), sizeof(int));
  return wkbTypeMap(w,t, pnZMFlag);
}

/*
** Read one byte from the WKB and advance the read pointer
*/
static char
wkbReadChar(wkbObj *w)
{
  char c;
  memcpy(&c, w->ptr, sizeof(char));
  w->ptr += sizeof(char);
  return c;
}

/*
** Read one integer from the WKB and advance the read pointer.
** We assume the endianess of the WKB is the same as this machine.
*/
static inline int
wkbReadInt(wkbObj *w)
{
  int i;
  memcpy(&i, w->ptr, sizeof(int));
  w->ptr += sizeof(int);
  return i;
}

/*
** Read one pointObj (two doubles) from the WKB and advance the read pointer.
** We assume the endianess of the WKB is the same as this machine.
*/
static inline void
wkbReadPointP(wkbObj *w, pointObj *p, int nZMFlag)
{
  memcpy(&(p->x), w->ptr, sizeof(double));
  w->ptr += sizeof(double);
  memcpy(&(p->y), w->ptr, sizeof(double));
  w->ptr += sizeof(double);
  if( nZMFlag & HAS_Z )
  {
#ifdef USE_POINT_Z_M
      memcpy(&(p->z), w->ptr, sizeof(double));
#endif
      w->ptr += sizeof(double);
  }
#ifdef USE_POINT_Z_M
  else
  {
      p->z = 0;
  }
#endif
  if( nZMFlag & HAS_M )
  {
#ifdef USE_POINT_Z_M
      memcpy(&(p->m), w->ptr, sizeof(double));
#endif
      w->ptr += sizeof(double);
  }
#ifdef USE_POINT_Z_M
  else
  {
      p->m = 0;
  }
#endif
}

/*
** Read one pointObj (two doubles) from the WKB and advance the read pointer.
** We assume the endianess of the WKB is the same as this machine.
*/
static inline pointObj
wkbReadPoint(wkbObj *w, int nZMFlag)
{
  pointObj p;
  wkbReadPointP(w, &p, nZMFlag);
  return p;
}

/*
** Read a "point array" and return an allocated lineObj.
** A point array is a WKB fragment that starts with a
** point count, which is followed by that number of doubles * 2.
** Linestrings, circular strings, polygon rings, all show this
** form.
*/
static void
wkbReadLine(wkbObj *w, lineObj *line, int nZMFlag)
{
  int i;
  pointObj p;
  int npoints = wkbReadInt(w);

  line->numpoints = npoints;
  line->point = msSmallMalloc(npoints * sizeof(pointObj));
  for ( i = 0; i < npoints; i++ ) {
    wkbReadPointP(w, &p, nZMFlag);
    line->point[i] = p;
  }
}

/*
** Advance the read pointer past a geometry without returning any
** values. Used for skipping un-drawable elements in a collection.
*/
static void
wkbSkipGeometry(wkbObj *w)
{
  int type, npoints, nrings, ngeoms, i;
  int nZMFlag;
  int nCoordDim;
  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w), &nZMFlag);
  nCoordDim = 2 + (((nZMFlag & HAS_Z) != 0) ? 1 : 0) + (((nZMFlag & HAS_M) != 0) ? 1 : 0);
  switch(type) {
    case WKB_POINT:
      w->ptr += nCoordDim * sizeof(double);
      break;
    case WKB_CIRCULARSTRING:
    case WKB_LINESTRING:
      npoints = wkbReadInt(w);
      w->ptr += npoints * nCoordDim * sizeof(double);
      break;
    case WKB_POLYGON:
      nrings = wkbReadInt(w);
      for ( i = 0; i < nrings; i++ ) {
        npoints = wkbReadInt(w);
        w->ptr += npoints * nCoordDim * sizeof(double);
      }
      break;
    case WKB_MULTIPOINT:
    case WKB_MULTILINESTRING:
    case WKB_MULTIPOLYGON:
    case WKB_GEOMETRYCOLLECTION:
    case WKB_COMPOUNDCURVE:
    case WKB_CURVEPOLYGON:
    case WKB_MULTICURVE:
    case WKB_MULTISURFACE:
      ngeoms = wkbReadInt(w);
      for ( i = 0; i < ngeoms; i++ ) {
        wkbSkipGeometry(w);
      }
  }
}

/*
** Convert a WKB point to a shapeObj, advancing the read pointer as we go.
*/
static int
wkbConvPointToShape(wkbObj *w, shapeObj *shape)
{
  int type;
  lineObj line;
  int nZMFlag;

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w),&nZMFlag);

  if( type != WKB_POINT ) return MS_FAILURE;

  if( ! (shape->type == MS_SHAPE_POINT) ) return MS_FAILURE;
  line.numpoints = 1;
  line.point = msSmallMalloc(sizeof(pointObj));
  line.point[0] = wkbReadPoint(w, nZMFlag);
  msAddLineDirectly(shape, &line);
  return MS_SUCCESS;
}

/*
** Convert a WKB line string to a shapeObj, advancing the read pointer as we go.
*/
static int
wkbConvLineStringToShape(wkbObj *w, shapeObj *shape)
{
  int type;
  lineObj line;
  int nZMFlag;

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w), &nZMFlag);

  if( type != WKB_LINESTRING ) return MS_FAILURE;

  wkbReadLine(w,&line, nZMFlag);
  msAddLineDirectly(shape, &line);

  return MS_SUCCESS;
}

/*
** Convert a WKB polygon to a shapeObj, advancing the read pointer as we go.
*/
static int
wkbConvPolygonToShape(wkbObj *w, shapeObj *shape)
{
  int type;
  int i, nrings;
  lineObj line;
  int nZMFlag;

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w), &nZMFlag);

  if( type != WKB_POLYGON ) return MS_FAILURE;

  /* How many rings? */
  nrings = wkbReadInt(w);

  /* Add each ring to the shape */
  for( i = 0; i < nrings; i++ ) {
    wkbReadLine(w,&line, nZMFlag);
    msAddLineDirectly(shape, &line);
  }

  return MS_SUCCESS;
}

/*
** Convert a WKB curve polygon to a shapeObj, advancing the read pointer as we go.
** The arc portions of the rings will be stroked to linestrings as they
** are read by the underlying circular string handling.
*/
static int
wkbConvCurvePolygonToShape(wkbObj *w, shapeObj *shape)
{
  int type, i, ncomponents;
  int failures = 0;
  int was_poly = ( shape->type == MS_SHAPE_POLYGON );
  int nZMFlag;

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w), &nZMFlag);
  ncomponents = wkbReadInt(w);

  if( type != WKB_CURVEPOLYGON ) return MS_FAILURE;

  /* Lower the allowed dimensionality so we can
  *  catch the linear ring components */
  shape->type = MS_SHAPE_LINE;

  for ( i = 0; i < ncomponents; i++ ) {
    if ( wkbConvGeometryToShape(w, shape) == MS_FAILURE ) {
      wkbSkipGeometry(w);
      failures++;
    }
  }

  /* Go back to expected dimensionality */
  if ( was_poly) shape->type = MS_SHAPE_POLYGON;

  if ( failures == ncomponents )
    return MS_FAILURE;
  else
    return MS_SUCCESS;
}

/*
** Convert a WKB circular string to a shapeObj, advancing the read pointer as we go.
** Arcs will be stroked to linestrings.
*/
static int
wkbConvCircularStringToShape(wkbObj *w, shapeObj *shape)
{
  int type;
  int nZMFlag;
  lineObj line = {0, NULL};

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w), &nZMFlag);

  if( type != WKB_CIRCULARSTRING ) return MS_FAILURE;

  /* Stroke the string into a point array */
  if ( arcStrokeCircularString(w, SEGMENT_ANGLE, &line, nZMFlag) == MS_FAILURE ) {
    if(line.point) free(line.point);
    return MS_FAILURE;
  }

  /* Fill in the lineObj */
  if ( line.numpoints > 0 ) {
    msAddLine(shape, &line);
    if(line.point) free(line.point);
  }

  return MS_SUCCESS;
}

/*
** Compound curves need special handling. First we load
** each component of the curve on the a lineObj in a shape.
** Then we merge those lineObjs into a single lineObj. This
** allows compound curves to serve as closed rings in
** curve polygons.
*/
static int
wkbConvCompoundCurveToShape(wkbObj *w, shapeObj *shape)
{
  int npoints = 0;
  int type, ncomponents, i, j;
  lineObj line;
  shapeObj shapebuf;
  int nZMFlag;

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w), &nZMFlag);

  /* Init our shape buffer */
  msInitShape(&shapebuf);

  if( type != WKB_COMPOUNDCURVE ) return MS_FAILURE;

  /* How many components in the compound curve? */
  ncomponents = wkbReadInt(w);

  /* We'll load each component onto a line in a shape */
  for( i = 0; i < ncomponents; i++ )
    wkbConvGeometryToShape(w, &shapebuf);

  /* Do nothing on empty */
  if ( shapebuf.numlines == 0 )
    return MS_FAILURE;

  /* Count the total number of points */
  for( i = 0; i < shapebuf.numlines; i++ )
    npoints += shapebuf.line[i].numpoints;

  /* Do nothing on empty */
  if ( npoints == 0 )
    return MS_FAILURE;

  line.numpoints = npoints;
  line.point = msSmallMalloc(sizeof(pointObj) * npoints);

  /* Copy in the points */
  npoints = 0;
  for ( i = 0; i < shapebuf.numlines; i++ ) {
    for ( j = 0; j < shapebuf.line[i].numpoints; j++ ) {
      /* Don't add a start point that duplicates an endpoint */
      if( j == 0 && i > 0 &&
          memcmp(&(line.point[npoints - 1]),&(shapebuf.line[i].point[j]),sizeof(pointObj)) == 0 ) {
        continue;
      }
      line.point[npoints++] = shapebuf.line[i].point[j];
    }
  }
  line.numpoints = npoints;

  /* Clean up */
  msFreeShape(&shapebuf);

  /* Fill in the lineObj */
  msAddLineDirectly(shape, &line);

  return MS_SUCCESS;
}

/*
** Convert a WKB collection string to a shapeObj, advancing the read pointer as we go.
** Many WKB types (MultiPoint, MultiLineString, MultiPolygon, MultiSurface,
** MultiCurve, GeometryCollection) can be treated identically as collections
** (they start with endian, type number and count of sub-elements, then provide the
** subelements as WKB) so are handled with this one function.
*/
static int
wkbConvCollectionToShape(wkbObj *w, shapeObj *shape)
{
  int i, ncomponents;
  int failures = 0;
  int nZMFlag;

  /*endian = */wkbReadChar(w);
  /*type = */wkbTypeMap(w,wkbReadInt(w), &nZMFlag);
  ncomponents = wkbReadInt(w);

  /*
  * If we can draw any portion of the collection, we will,
  * but if all the components fail, we will draw nothing.
  */
  for ( i = 0; i < ncomponents; i++ ) {
    if ( wkbConvGeometryToShape(w, shape) == MS_FAILURE ) {
      wkbSkipGeometry(w);
      failures++;
    }
  }
  if ( failures == ncomponents || ncomponents == 0)
    return MS_FAILURE;
  else
    return MS_SUCCESS;
}

/*
** Generic handler to switch to the appropriate function for the WKB type.
** Note that we also handle switching here to avoid processing shapes
** we will be unable to draw. Example: we can't draw point features as
** a MS_SHAPE_LINE layer, so if the type is WKB_POINT and the layer is
** MS_SHAPE_LINE, we exit before converting.
*/
int
wkbConvGeometryToShape(wkbObj *w, shapeObj *shape)
{
  int nZMFlag;
  int wkbtype = wkbType(w, &nZMFlag); /* Peak at the type number */

  switch(wkbtype) {
      /* Recurse into anonymous collections */
    case WKB_GEOMETRYCOLLECTION:
      return wkbConvCollectionToShape(w, shape);
      /* Handle area types */
    case WKB_POLYGON:
      return wkbConvPolygonToShape(w, shape);
    case WKB_MULTIPOLYGON:
      return wkbConvCollectionToShape(w, shape);
    case WKB_CURVEPOLYGON:
      return wkbConvCurvePolygonToShape(w, shape);
    case WKB_MULTISURFACE:
      return wkbConvCollectionToShape(w, shape);
  }

  /* We can't convert any of the following types into polygons */
  if ( shape->type == MS_SHAPE_POLYGON ) return MS_FAILURE;

  /* Handle linear types */
  switch(wkbtype) {
    case WKB_LINESTRING:
      return wkbConvLineStringToShape(w, shape);
    case WKB_CIRCULARSTRING:
      return wkbConvCircularStringToShape(w, shape);
    case WKB_COMPOUNDCURVE:
      return wkbConvCompoundCurveToShape(w, shape);
    case WKB_MULTILINESTRING:
      return wkbConvCollectionToShape(w, shape);
    case WKB_MULTICURVE:
      return wkbConvCollectionToShape(w, shape);
  }

  /* We can't convert any of the following types into lines */
  if ( shape->type == MS_SHAPE_LINE ) return MS_FAILURE;

  /* Handle point types */
  switch(wkbtype) {
    case WKB_POINT:
      return wkbConvPointToShape(w, shape);
    case WKB_MULTIPOINT:
      return wkbConvCollectionToShape(w, shape);
  }

  /* This is a WKB type we don't know about! */
  return MS_FAILURE;
}


/*
** What side of p1->p2 is q on?
*/
static inline int
arcSegmentSide(const pointObj *p1, const pointObj *p2, const pointObj *q)
{
  double side = ( (q->x - p1->x) * (p2->y - p1->y) - (p2->x - p1->x) * (q->y - p1->y) );
  if ( FP_EQ(side,0.0) ) {
    return FP_COLINEAR;
  } else {
    if ( side < 0.0 )
      return FP_LEFT;
    else
      return FP_RIGHT;
  }
}

/*
** Calculate the center of the circle defined by three points.
** Using matrix approach from http://mathforum.org/library/drmath/view/55239.html
*/
int
arcCircleCenter(const pointObj *p1, const pointObj *p2, const pointObj *p3, pointObj *center, double *radius)
{
  pointObj c = {0,0,0,0}; // initialize
  double dx21, dy21, dx31, dy31, h21, h31, d, r;

  /* Circle is closed, so p2 must be opposite p1 & p3. */
  if ((fabs(p1->x - p3->x) < FP_EPSILON) && (fabs(p1->y-p3->y) < FP_EPSILON)) {
    c.x = p1->x + (p2->x - p1->x) / 2.0;
    c.y = p1->y + (p2->y - p1->y) / 2.0;
    r = sqrt(pow(c.x - p1->x, 2.0) + pow(c.y - p1->y, 2.0));
  }
  /* There is no circle here, the points are actually co-linear */
  else if ( arcSegmentSide(p1, p3, p2) == FP_COLINEAR ) {
    return MS_FAILURE;
  }
  /* Calculate the center and radius. */
  else {

    /* Radius */
    dx21 = p2->x - p1->x;
    dy21 = p2->y - p1->y;
    dx31 = p3->x - p1->x;
    dy31 = p3->y - p1->y;

    h21 = pow(dx21, 2.0) + pow(dy21, 2.0);
    h31 = pow(dx31, 2.0) + pow(dy31, 2.0);

    /* 2 * |Cross product|, d<0 means clockwise and d>0 counterclockwise sweeping angle */
    d = 2 * (dx21 * dy31 - dx31 * dy21);

    c.x = p1->x + (h21 * dy31 - h31 * dy21) / d;
    c.y = p1->y - (h21 * dx31 - h31 * dx21) / d;
    r = sqrt(pow(c.x - p1->x, 2) + pow(c.y - p1->y, 2));
  }

  if ( radius ) *radius = r;
  if ( center ) *center = c;

  return MS_SUCCESS;
}

/*
** Write a stroked version of the circle defined by three points into a
** point buffer. The segment_angle (degrees) is the coverage of each stroke segment,
** and depending on whether this is the first arc in a circularstring,
** you might want to include_first
*/
int
arcStrokeCircle(const pointObj *p1, const pointObj *p2, const pointObj *p3,
                double segment_angle, int include_first, pointArrayObj *pa)
{
  pointObj center; /* Center of our circular arc */
  double radius; /* Radius of our circular arc */
  double sweep_angle_r; /* Total angular size of our circular arc in radians */
  double segment_angle_r; /* Segment angle in radians */
  double a1, /*a2,*/ a3; /* Angles represented by p1, p2, p3 relative to center */
  int side = arcSegmentSide(p1, p3, p2); /* What side of p1,p3 is the middle point? */
  int num_edges; /* How many edges we will be generating */
  double current_angle_r; /* What angle are we generating now (radians)? */
  int i; /* Counter */
  pointObj p; /* Temporary point */
  int is_closed = MS_FALSE;

  /* We need to know if we're dealing with a circle early */
  if ( FP_EQ(p1->x, p3->x) && FP_EQ(p1->y, p3->y) )
    is_closed = MS_TRUE;

  /* Check if the "arc" is actually straight */
  if ( ! is_closed && side == FP_COLINEAR ) {
    /* We just need to write in the end points */
    if ( include_first )
      pointArrayAddPoint(pa, p1);
    pointArrayAddPoint(pa, p3);
    return MS_SUCCESS;
  }

  /* We should always be able to find the center of a non-linear arc */
  if ( arcCircleCenter(p1, p2, p3, &center, &radius) == MS_FAILURE )
    return MS_FAILURE;

  /* Calculate the angles that our three points represent */
  a1 = atan2(p1->y - center.y, p1->x - center.x);
  /* UNUSED
  a2 = atan2(p2->y - center.y, p2->x - center.x);
   */
  a3 = atan2(p3->y - center.y, p3->x - center.x);
  segment_angle_r = M_PI * segment_angle / 180.0;

  /* Closed-circle case, we sweep the whole circle! */
  if ( is_closed ) {
    sweep_angle_r = 2.0 * M_PI;
  }
  /* Clockwise sweep direction */
  else if ( side == FP_LEFT ) {
    if ( a3 > a1 ) /* Wrapping past 180? */
      sweep_angle_r = a1 + (2.0 * M_PI - a3);
    else
      sweep_angle_r = a1 - a3;
  }
  /* Counter-clockwise sweep direction */
  else if ( side == FP_RIGHT ) {
    if ( a3 > a1 ) /* Wrapping past 180? */
      sweep_angle_r = a3 - a1;
    else
      sweep_angle_r = a3 + (2.0 * M_PI - a1);
  } else
    sweep_angle_r = 0.0;

  /* We don't have enough resolution, let's invert our strategy. */
  if ( (sweep_angle_r / segment_angle_r) < SEGMENT_MINPOINTS ) {
    segment_angle_r = sweep_angle_r / (SEGMENT_MINPOINTS + 1);
  }

  /* We don't have enough resolution to stroke this arc,
  *  so just join the start to the end. */
  if ( sweep_angle_r < segment_angle_r ) {
    if ( include_first )
      pointArrayAddPoint(pa, p1);
    pointArrayAddPoint(pa, p3);
    return MS_SUCCESS;
  }

  /* How many edges to generate (we add the final edge
  *  by sticking on the last point */
  num_edges = floor(sweep_angle_r / fabs(segment_angle_r));

  /* Go backwards (negative angular steps) if we are stroking clockwise */
  if ( side == FP_LEFT )
    segment_angle_r *= -1;

  /* What point should we start with? */
  if( include_first ) {
    current_angle_r = a1;
  } else {
    current_angle_r = a1 + segment_angle_r;
    num_edges--;
  }

  /* For each edge, increment or decrement by our segment angle */
  for( i = 0; i < num_edges; i++ ) {
    if (segment_angle_r > 0.0 && current_angle_r > M_PI)
      current_angle_r -= 2*M_PI;
    if (segment_angle_r < 0.0 && current_angle_r < -1*M_PI)
      current_angle_r -= 2*M_PI;
    p.x = center.x + radius*cos(current_angle_r);
    p.y = center.y + radius*sin(current_angle_r);
    pointArrayAddPoint(pa, &p);
    current_angle_r += segment_angle_r;
  }

  /* Add the last point */
  pointArrayAddPoint(pa, p3);
  return MS_SUCCESS;
}

/*
** This function does not actually take WKB as input, it takes the
** WKB starting from the numpoints integer. Each three-point edge
** is stroked into a linestring and appended into the lineObj
** argument.
*/
int
arcStrokeCircularString(wkbObj *w, double segment_angle, lineObj *line, int nZMFlag)
{
  pointObj p1, p2, p3;
  int npoints, nedges;
  int edge = 0;
  pointArrayObj *pa;

  if ( ! w || ! line ) return MS_FAILURE;

  npoints = wkbReadInt(w);
  nedges = npoints / 2;

  /* All CircularStrings have an odd number of points */
  if ( npoints < 3 || npoints % 2 != 1 )
    return MS_FAILURE;

  /* Make a large guess at how much space we'll need */
  pa = pointArrayNew(nedges * 180 / segment_angle);

  wkbReadPointP(w,&p3,nZMFlag);

  /* Fill out the point array with stroked arcs */
  while( edge < nedges ) {
    p1 = p3;
    wkbReadPointP(w,&p2,nZMFlag);
    wkbReadPointP(w,&p3,nZMFlag);
    if ( arcStrokeCircle(&p1, &p2, &p3, segment_angle, edge ? 0 : 1, pa) == MS_FAILURE ) {
      pointArrayFree(pa);
      return MS_FAILURE;
    }
    edge++;
  }

  /* Copy the point array into the line */
  line->numpoints = pa->npoints;
  line->point = msSmallMalloc(line->numpoints * sizeof(pointObj));
  memcpy(line->point, pa->data, line->numpoints * sizeof(pointObj));

  /* Clean up */
  pointArrayFree(pa);

  return MS_SUCCESS;
}


/*
** For LAYER types that are not the usual ones (charts,
** annotations, etc) we will convert to a shape type
** that "makes sense" given the WKB input. We do this
** by peaking at the type number of the first collection
** sub-element.
*/
static int
msPostGISFindBestType(wkbObj *w, shapeObj *shape)
{
  int wkbtype;
  int nZMFlag;

  /* What kind of geometry is this? */
  wkbtype = wkbType(w, &nZMFlag);

  /* Generic collection, we need to look a little deeper. */
  if ( wkbtype == WKB_GEOMETRYCOLLECTION )
    wkbtype = wkbCollectionSubType(w, &nZMFlag);

  switch ( wkbtype ) {
    case WKB_POLYGON:
    case WKB_CURVEPOLYGON:
    case WKB_MULTIPOLYGON:
      shape->type = MS_SHAPE_POLYGON;
      break;
    case WKB_LINESTRING:
    case WKB_CIRCULARSTRING:
    case WKB_COMPOUNDCURVE:
    case WKB_MULTICURVE:
    case WKB_MULTILINESTRING:
      shape->type = MS_SHAPE_LINE;
      break;
    case WKB_POINT:
    case WKB_MULTIPOINT:
      shape->type = MS_SHAPE_POINT;
      break;
    default:
      return MS_FAILURE;
  }

  return wkbConvGeometryToShape(w, shape);
}

/*
** Recent versions of PgSQL provide the version as an int in a
** simple call to the connection handle. For earlier ones we have
** to parse the version string into a usable number.
*/
static int
msPostGISRetrievePgVersion(PGconn *pgconn)
{
#ifndef POSTGIS_HAS_SERVER_VERSION
  int pgVersion = 0;
  char *strVersion = NULL;
  char *strParts[3] = { NULL, NULL, NULL };
  int i = 0, j = 0, len = 0;
  int factor = 10000;

  if (pgconn == NULL) {
    msSetError(MS_QUERYERR, "Layer does not have a postgis connection.", "msPostGISRetrievePgVersion()");
    return MS_FAILURE;
  }

  if (! PQparameterStatus(pgconn, "server_version") )
    return MS_FAILURE;

  strVersion = msStrdup(PQparameterStatus(pgconn, "server_version"));
  if( ! strVersion )
    return MS_FAILURE;

  strParts[j] = strVersion;
  j++;
  len = strlen(strVersion);
  for( i = 0; i < len; i++ ) {
    if( strVersion[i] == '.' ) {
      strVersion[i] = '\0';

      if( j < 3 ) {
        strParts[j] = strVersion + i + 1;
        j++;
      } else {
        free(strVersion);
        msSetError(MS_QUERYERR, "Too many parts in version string.", "msPostGISRetrievePgVersion()");
        return MS_FAILURE;
      }
    }
  }

  for( j = 0; j < 3 && strParts[j]; j++ ) {
    pgVersion += factor * atoi(strParts[j]);
    factor = factor / 100;
  }
  free(strVersion);
  return pgVersion;
#else
  return PQserverVersion(pgconn);
#endif
}

/*
** Get the PostGIS version number from the database as integer.
** Versions are multiplied out as with PgSQL: 1.5.2 -> 10502, 2.0.0 -> 20000.
*/
static int
msPostGISRetrieveVersion(PGconn *pgconn)
{
  static char* sql = "SELECT postgis_version()";
  int version = 0;
  size_t strSize;
  char *strVersion = NULL;
  char *ptr;
  char *strParts[3] = { NULL, NULL, NULL };
  int i = 0, j = 0;
  int factor = 10000;
  PGresult *pgresult = NULL;

  if ( ! pgconn ) {
    msSetError(MS_QUERYERR, "No open connection.", "msPostGISRetrieveVersion()");
    return MS_FAILURE;
  }

  pgresult = PQexecParams(pgconn, sql,0, NULL, NULL, NULL, NULL, 0);

  if ( !pgresult || PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
    msDebug("Error executing SQL: (%s) in msPostGISRetrieveVersion()", sql);
    msSetError(MS_QUERYERR, "Error executing SQL. check server logs.", "msPostGISRetrieveVersion()");
    return MS_FAILURE;
  }

  if (PQgetisnull(pgresult, 0, 0)) {
    PQclear(pgresult);
    msSetError(MS_QUERYERR,"Null result returned.","msPostGISRetrieveVersion()");
    return MS_FAILURE;
  }

  strSize = PQgetlength(pgresult, 0, 0) + 1;
  strVersion = (char*)msSmallMalloc(strSize);
  strlcpy(strVersion, PQgetvalue(pgresult, 0, 0), strSize);
  PQclear(pgresult);

  ptr = strVersion;
  strParts[j++] = strVersion;
  while( *ptr != '\0' && j < 3 ) {
    if ( *ptr == '.' ) {
      *ptr = '\0';
      strParts[j++] = ptr + 1;
    }
    if ( *ptr == ' ' ) {
      *ptr = '\0';
      break;
    }
    ptr++;
  }

  for( i = 0; i < j; i++ ) {
    version += factor * atoi(strParts[i]);
    factor = factor / 100;
  }
  free(strVersion);

  return version;
}

/*
** msPostGISRetrievePK()
**
** Find out that the primary key is for this layer.
** The layerinfo->fromsource must already be populated and
** must not be a subquery.
*/
static int
msPostGISRetrievePK(layerObj *layer)
{
  PGresult *pgresult = NULL;
  char *sql = 0;
  size_t size;
  msPostGISLayerInfo *layerinfo = 0;
  int length;
  int pgVersion;

  if (layer->debug) {
    msDebug("msPostGISRetrievePK called.\n");
  }

  layerinfo = (msPostGISLayerInfo *) layer->layerinfo;


  if (layerinfo->pgconn == NULL) {
    msSetError(MS_QUERYERR, "Layer does not have a postgis connection.", "msPostGISRetrievePK()");
    return MS_FAILURE;
  }
  pgVersion = msPostGISRetrievePgVersion(layerinfo->pgconn);

  if (pgVersion < 70000) {
    if (layer->debug) {
      msDebug("msPostGISRetrievePK(): Major version below 7.\n");
    }
    return MS_FAILURE;
  }
  if (pgVersion < 70200) {
    if (layer->debug) {
      msDebug("msPostGISRetrievePK(): Version below 7.2.\n");
    }
    return MS_FAILURE;
  }
  if (pgVersion < 70300) {
    /*
    ** PostgreSQL v7.2 has a different representation of primary keys that
    ** later versions.  This currently does not explicitly exclude
    ** multicolumn primary keys.
    */
    static char *v72sql = "select b.attname from pg_class as a, pg_attribute as b, (select oid from pg_class where relname = '%s') as c, pg_index as d where d.indexrelid = a.oid and d.indrelid = c.oid and d.indisprimary and b.attrelid = a.oid and a.relnatts = 1";
    sql = msSmallMalloc(strlen(layerinfo->fromsource) + strlen(v72sql) + 1);
    sprintf(sql, v72sql, layerinfo->fromsource);
  } else {
    /* Attempt to separate fromsource into schema.table */
    char *pos_sep;
    char *schema = NULL;
    char *table = NULL;
    pos_sep = strstr(layerinfo->fromsource, ".");
    if (pos_sep) {
      length = strlen(layerinfo->fromsource) - strlen(pos_sep) + 1;
      schema = (char*)msSmallMalloc(length);
      strlcpy(schema, layerinfo->fromsource, length);

      length = strlen(pos_sep) + 1;
      table = (char*)msSmallMalloc(length);
      strlcpy(table, pos_sep + 1, length);

      if (layer->debug) {
        msDebug("msPostGISRetrievePK(): Found schema %s, table %s.\n", schema, table);
      }
    }
    /*
    ** PostgreSQL v7.3 and later treat primary keys as constraints.
    ** We only support single column primary keys, so multicolumn
    ** pks are explicitly excluded from the query.
    */
    if (schema && table) {
      static char *v73sql = "select attname from pg_attribute, pg_constraint, pg_class, pg_namespace where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = '%s' and pg_class.relnamespace = pg_namespace.oid and pg_namespace.nspname = '%s' and pg_constraint.conkey[2] is null";
      sql = msSmallMalloc(strlen(schema) + strlen(table) + strlen(v73sql) + 1);
      sprintf(sql, v73sql, table, schema);
    } else {
      static char *v73sql = "select attname from pg_attribute, pg_constraint, pg_class where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = '%s' and pg_table_is_visible(pg_class.oid) and pg_constraint.conkey[2] is null";
      sql = msSmallMalloc(strlen(layerinfo->fromsource) + strlen(v73sql) + 1);
      sprintf(sql, v73sql, layerinfo->fromsource);
    }
    free(table);
    free(schema);
  }

  if (layer->debug > 1) {
    msDebug("msPostGISRetrievePK: %s\n", sql);
  }

  layerinfo = (msPostGISLayerInfo *) layer->layerinfo;

  if (layerinfo->pgconn == NULL) {
    msSetError(MS_QUERYERR, "Layer does not have a postgis connection.", "msPostGISRetrievePK()");
    free(sql);
    return MS_FAILURE;
  }

  pgresult = PQexecParams(layerinfo->pgconn, sql, 0, NULL, NULL, NULL, NULL, 0);
  if ( !pgresult || PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
    static char *tmp1 = "Error executing SQL: ";
    char *tmp2 = NULL;
    size_t size2;

    size2 = sizeof(char)*(strlen(tmp1) + strlen(sql) + 1);
    tmp2 = (char*)msSmallMalloc(size2);
    strlcpy(tmp2, tmp1, size2);
    strlcat(tmp2, sql, size2);
    msSetError(MS_QUERYERR, "%s", "msPostGISRetrievePK()", tmp2);
    free(tmp2);
    free(sql);
    return MS_FAILURE;
  }

  if (PQntuples(pgresult) < 1) {
    if (layer->debug) {
      msDebug("msPostGISRetrievePK: No results found.\n");
    }
    PQclear(pgresult);
    free(sql);
    return MS_FAILURE;
  }
  if (PQntuples(pgresult) > 1) {
    if (layer->debug) {
      msDebug("msPostGISRetrievePK: Multiple results found.\n");
    }
    PQclear(pgresult);
    free(sql);
    return MS_FAILURE;
  }

  if (PQgetisnull(pgresult, 0, 0)) {
    if (layer->debug) {
      msDebug("msPostGISRetrievePK: Null result returned.\n");
    }
    PQclear(pgresult);
    free(sql);
    return MS_FAILURE;
  }

  size = PQgetlength(pgresult, 0, 0) + 1;
  layerinfo->uid = (char*)msSmallMalloc(size);
  strlcpy(layerinfo->uid, PQgetvalue(pgresult, 0, 0), size);

  PQclear(pgresult);
  free(sql);
  return MS_SUCCESS;
}


/*
** msPostGISParseData()
**
** Parse the DATA string for geometry column name, table name,
** unique id column, srid, and SQL string.
*/
int msPostGISParseData(layerObj *layer)
{
  char *pos_opt, *pos_scn, *tmp, *pos_srid, *pos_uid, *pos_geom, *data;
  char *pos_use_1st, *pos_use_2nd;
  int slength, dsize;
  msPostGISLayerInfo *layerinfo;

  assert(layer != NULL);
  assert(layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo*)(layer->layerinfo);

  if (layer->debug) {
    msDebug("msPostGISParseData called.\n");
  }

  if (!layer->data) {
    msSetError(MS_QUERYERR, "Missing DATA clause. DATA statement must contain 'geometry_column from table_name' or 'geometry_column from (sub-query) as sub'.", "msPostGISParseData()");
    return MS_FAILURE;
  }
  dsize = strlen ( layer->data ) + 1;
  data = (char*)msSmallMalloc(dsize);
  strlcpy ( data, layer->data, dsize );
  for ( tmp = data; *tmp; tmp++ )
    if ( strchr ( "\t\n\r", *tmp ) ) *tmp = ' ';

  /*
  ** Clean up any existing strings first, as we will be populating these fields.
  */
  if( layerinfo->srid ) {
    free(layerinfo->srid);
    layerinfo->srid = NULL;
  }
  if( layerinfo->uid ) {
    free(layerinfo->uid);
    layerinfo->uid = NULL;
  }
  if( layerinfo->geomcolumn ) {
    free(layerinfo->geomcolumn);
    layerinfo->geomcolumn = NULL;
  }
  if( layerinfo->fromsource ) {
    free(layerinfo->fromsource);
    layerinfo->fromsource = NULL;
  }

  /*
  ** Look for the optional ' using ' clauses.
  */
  pos_srid = pos_uid = NULL;
  pos_use_1st = pos_use_2nd = NULL;
  tmp = strcasestr(data, " using ");
  while ( tmp )
  {
    pos_use_1st = pos_use_2nd;
    pos_use_2nd = tmp + 1;
    tmp = strcasestr(tmp+1, " using ");
  };

  /*
  ** What clause appear after 2nd 'using', if set?
  */
  if ( pos_use_2nd )
  {
    for ( tmp = pos_use_2nd + 5; *tmp == ' '; tmp++ );
    if ( strncasecmp ( tmp, "unique ", 7 ) == 0 )
      for ( pos_uid = tmp + 7; *pos_uid == ' '; pos_uid++ );
    if ( strncasecmp ( tmp, "srid=", 5 ) == 0 ) pos_srid = tmp + 5;
  };

  /*
  ** What clause appear after 1st 'using', if set?
  */
  if ( pos_use_1st )
  {
    for ( tmp = pos_use_1st + 5; *tmp == ' '; tmp++ );
    if ( strncasecmp ( tmp, "unique ", 7 ) == 0 )
    {
      if ( pos_uid )
      {
        free ( data );
        msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable. Too many 'USING UNIQUE' found! %s", "msPostGISParseData()", layer->data);
        return MS_FAILURE;
      };
      for ( pos_uid = tmp + 7; *pos_uid == ' '; pos_uid++ );
    };
    if ( strncasecmp ( tmp, "srid=", 5 ) == 0 )
    {
      if ( pos_srid )
      {
        free ( data );
        msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable. Too many 'USING SRID' found! %s", "msPostGISParseData()", layer->data);
        return MS_FAILURE;
      };
      pos_srid = tmp + 5;
    };
  };

  /*
  ** Look for the optional ' using unique ID' string first.
  */
  if (pos_uid) {
    /* Find the end of this case 'using unique ftab_id using srid=33' */
    tmp = strstr(pos_uid, " ");
    /* Find the end of this case 'using srid=33 using unique ftab_id' */
    if (!tmp) {
      tmp = pos_uid + strlen(pos_uid);
    }
    layerinfo->uid = (char*) msSmallMalloc((tmp - pos_uid) + 1);
    strlcpy(layerinfo->uid, pos_uid, (tmp - pos_uid) + 1);
    msStringTrim(layerinfo->uid);
  }

  /*
  ** Look for the optional ' using srid=333 ' string next.
  */
  if (!pos_srid) {
    layerinfo->srid = (char*) msSmallMalloc(1);
    (layerinfo->srid)[0] = '\0'; /* no SRID, so return just null terminator*/
  } else {
    slength = strspn(pos_srid, "-0123456789");
    if (!slength) {
      free ( data );
      msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable. You specified 'USING SRID' but didn't have any numbers! %s", "msPostGISParseData()", layer->data);
      return MS_FAILURE;
    } else {
      layerinfo->srid = (char*) msSmallMalloc(slength + 1);
      strlcpy(layerinfo->srid, pos_srid, slength+1);
      msStringTrim(layerinfo->srid);
    }
  }

  /*
  ** This is a little hack so the rest of the code works.
  ** pos_opt should point to the start of the optional blocks.
  **
  ** If they are both set, return the smaller one.
  ** If pos_use_1st set, then it smaller.
  */
  if (pos_use_1st) {
    pos_opt = pos_use_1st;
  }
  /* If one or none is set, return the larger one. */
  else {
    pos_opt = pos_use_2nd;
  }
  /* No pos_opt? Move it to the end of the string. */
  if (!pos_opt) {
    pos_opt = data + strlen(data);
  }
  /* Back after the last non-space character. */
  while( ( pos_opt > data ) && ( *(pos_opt-1) == ' ' ) )
  {
    --pos_opt;
  }

  /*
  ** Scan for the 'geometry from table' or 'geometry from () as foo' clause.
  */

  /* Find the first non-white character to start from */
  for ( pos_geom = data; *pos_geom == ' '; pos_geom++ );

  /* Find the end of the geom column name */
  pos_scn = strcasestr(data, " from ");
  if (!pos_scn) {
    free ( data );
    msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable. Must contain 'geometry from table' or 'geometry from (subselect) as foo'. %s", "msPostGISParseData()", layer->data);
    return MS_FAILURE;
  }

  /* Copy the geometry column name */
  layerinfo->geomcolumn = (char*) msSmallMalloc((pos_scn - pos_geom) + 1);
  strlcpy(layerinfo->geomcolumn, pos_geom, (pos_scn - pos_geom) + 1);
  msStringTrim(layerinfo->geomcolumn);

  /* Copy the table name or sub-select clause */
  for ( pos_scn += 6; *pos_scn == ' '; pos_scn++ );
  if ( pos_opt - pos_scn < 1 )
  {
    free ( data );
    msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable.  Must contain 'geometry from table' or 'geometry from (subselect) as foo'. %s", "msPostGISParseData()", layer->data);
    return MS_FAILURE;
  };
  layerinfo->fromsource = (char*) msSmallMalloc(pos_opt - pos_scn + 1);
  strlcpy(layerinfo->fromsource, ( layer->data - data ) + pos_scn, pos_opt - pos_scn + 1);
  msStringTrim(layerinfo->fromsource);

  /* Something is wrong, our goemetry column and table references are not there. */
  if (strlen(layerinfo->fromsource) < 1 || strlen(layerinfo->geomcolumn) < 1) {
    free ( data );
    msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable.  Must contain 'geometry from table' or 'geometry from (subselect) as foo'. %s", "msPostGISParseData()", layer->data);
    return MS_FAILURE;
  }

  /*
  ** We didn't find a ' using unique ' in the DATA string so try and find a
  ** primary key on the table.
  */
  if ( ! (layerinfo->uid) ) {
    if ( strstr(layerinfo->fromsource, " ") ) {
      free ( data );
      msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable.  You must specify 'using unique' when supplying a subselect in the data definition.", "msPostGISParseData()");
      return MS_FAILURE;
    }
    if ( msPostGISRetrievePK(layer) != MS_SUCCESS ) {
      /* No user specified unique id so we will use the PostgreSQL oid */
      /* TODO: Deprecate this, oids are deprecated in PostgreSQL */
      layerinfo->uid = msStrdup("oid");
    }
  }

  if (layer->debug) {
    msDebug("msPostGISParseData: unique_column=%s, srid=%s, geom_column_name=%s, table_name=%s\n", layerinfo->uid, layerinfo->srid, layerinfo->geomcolumn, layerinfo->fromsource);
  }
  free ( data );
  return MS_SUCCESS;
}



/*
** Decode a hex character.
*/
static unsigned char msPostGISHexDecodeChar[256] = {
  /* not Hex characters */
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  /* 0-9 */
  0,1,2,3,4,5,6,7,8,9,
  /* not Hex characters */
  64,64,64,64,64,64,64,
  /* A-F */
  10,11,12,13,14,15,
  /* not Hex characters */
  64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,
  /* a-f */
  10,11,12,13,14,15,
  64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  /* not Hex characters (upper 128 characters) */
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
};

/*
** Decode hex string "src" (null terminated)
** into "dest" (not null terminated).
** Returns length of decoded array or 0 on failure.
*/
int msPostGISHexDecode(unsigned char *dest, const char *src, int srclen)
{

  if (src && *src && (srclen % 2 == 0) ) {

    unsigned char *p = dest;
    int i;

    for ( i=0; i<srclen; i+=2 ) {
      register unsigned char b1=0, b2=0;
      register unsigned char c1 = src[i];
      register unsigned char c2 = src[i + 1];

      b1 = msPostGISHexDecodeChar[c1];
      b2 = msPostGISHexDecodeChar[c2];

      *p++ = (b1 << 4) | b2;

    }
    return(p-dest);
  }
  return 0;
}

/*
** Decode a base64 character.
*/
static unsigned char msPostGISBase64DecodeChar[256] = {
  /* not Base64 characters */
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,
  /*  +  */
  62,
  /* not Base64 characters */
  64,64,64,
  /*  /  */
  63,
  /* 0-9 */
  52,53,54,55,56,57,58,59,60,61,
  /* not Base64 characters */
  64,64,64,64,64,64,64,
  /* A-Z */
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
  /* not Base64 characters */
  64,64,64,64,64,64,
  /* a-z */
  26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
  /* not Base64 characters */
  64,64,64,64,64,
  /* not Base64 characters (upper 128 characters) */
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
};

/*
** Decode base64 string "src" (null terminated)
** into "dest" (not null terminated).
** Returns length of decoded array or 0 on failure.
*/
int msPostGISBase64Decode(unsigned char *dest, const char *src, int srclen)
{

  if (src && *src) {

    unsigned char *p = dest;
    int i, j, k;
    unsigned char *buf = calloc(srclen + 1, sizeof(unsigned char));

    /* Drop illegal chars first */
    for (i=0, j=0; src[i]; i++) {
      unsigned char c = src[i];
      if ( (msPostGISBase64DecodeChar[c] != 64) || (c == '=') ) {
        buf[j++] = c;
      }
    }

    for (k=0; k<j; k+=4) {
      register unsigned char c1='A', c2='A', c3='A', c4='A';
      register unsigned char b1=0, b2=0, b3=0, b4=0;

      c1 = buf[k];

      if (k+1<j) {
        c2 = buf[k+1];
      }
      if (k+2<j) {
        c3 = buf[k+2];
      }
      if (k+3<j) {
        c4 = buf[k+3];
      }

      b1 = msPostGISBase64DecodeChar[c1];
      b2 = msPostGISBase64DecodeChar[c2];
      b3 = msPostGISBase64DecodeChar[c3];
      b4 = msPostGISBase64DecodeChar[c4];

      *p++=((b1<<2)|(b2>>4) );
      if (c3 != '=') {
        *p++=(((b2&0xf)<<4)|(b3>>2) );
      }
      if (c4 != '=') {
        *p++=(((b3&0x3)<<6)|b4 );
      }
    }
    free(buf);
    return(p-dest);
  }
  return 0;
}

/*
** msPostGISBuildSQLBox()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLBox(layerObj *layer, rectObj *rect, char *strSRID)
{

  char *strBox = NULL;
  size_t sz;

  if (layer->debug) {
    msDebug("msPostGISBuildSQLBox called.\n");
  }

  if ( strSRID ) {
    static char *strBoxTemplate = "ST_GeomFromText('POLYGON((%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g))',%s)";
    /* 10 doubles + 1 integer + template characters */
    sz = 10 * 22 + strlen(strSRID) + strlen(strBoxTemplate);
    strBox = (char*)msSmallMalloc(sz+1); /* add space for terminating NULL */
    if ( sz <= snprintf(strBox, sz, strBoxTemplate,
                        rect->minx, rect->miny,
                        rect->minx, rect->maxy,
                        rect->maxx, rect->maxy,
                        rect->maxx, rect->miny,
                        rect->minx, rect->miny,
                        strSRID)) {
      msSetError(MS_MISCERR,"Bounding box digits truncated.","msPostGISBuildSQLBox");
      return NULL;
    }
  } else {
    static char *strBoxTemplate = "ST_GeomFromText('POLYGON((%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g))')";
    /* 10 doubles + template characters */
    sz = 10 * 22 + strlen(strBoxTemplate);
    strBox = (char*)msSmallMalloc(sz+1); /* add space for terminating NULL */
    if ( sz <= snprintf(strBox, sz, strBoxTemplate,
                        rect->minx, rect->miny,
                        rect->minx, rect->maxy,
                        rect->maxx, rect->maxy,
                        rect->maxx, rect->miny,
                        rect->minx, rect->miny) ) {
      msSetError(MS_MISCERR,"Bounding box digits truncated.","msPostGISBuildSQLBox");
      return NULL;
    }
  }

  return strBox;

}


/*
** msPostGISBuildSQLItems()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLItems(layerObj *layer)
{

  char *strEndian = NULL;
  char *strGeom = NULL;
  char *strItems = NULL;
  msPostGISLayerInfo *layerinfo = NULL;

  if (layer->debug) {
    msDebug("msPostGISBuildSQLItems called.\n");
  }

  assert( layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

  if ( ! layerinfo->geomcolumn ) {
    msSetError(MS_MISCERR, "layerinfo->geomcolumn is not initialized.", "msPostGISBuildSQLItems()");
    return NULL;
  }

  /*
  ** Get the server to transform the geometry into our
  ** native endian before transmitting it to us..
  */
  if (layerinfo->endian == LITTLE_ENDIAN) {
    strEndian = "NDR";
  } else {
    strEndian = "XDR";
  }

  {
    /*
    ** We transfer the geometry from server to client as a
    ** hex or base64 encoded WKB byte-array. We will have to decode this
    ** data once we get it. Forcing to 2D (via the AsBinary function
    ** which includes a 2D force in it) removes ordinates we don't
    ** need, saving transfer and encode/decode time.
    */
    char *force2d = "";
#if TRANSFER_ENCODING == 64
    const char *strGeomTemplate = "encode(ST_AsBinary(%s(\"%s\"),'%s'),'base64') as geom,\"%s\"";
#elif TRANSFER_ENCODING == 256
    const char *strGeomTemplate = "ST_AsBinary(%s(\"%s\"),'%s') as geom,\"%s\"::text";
#else
    const char *strGeomTemplate = "encode(ST_AsBinary(%s(\"%s\"),'%s'),'hex') as geom,\"%s\"";
#endif
    if( layerinfo->force2d ) {
      if( layerinfo->version >= 20100 )
        force2d = "ST_Force2D";
      else
        force2d = "ST_Force_2D";
    }
    else if( layerinfo->version < 20000 )
    {
        /* Use AsEWKB() to get 3D */
#if TRANSFER_ENCODING == 64
        strGeomTemplate = "encode(AsEWKB(%s(\"%s\"),'%s'),'base64') as geom,\"%s\"";
#elif TRANSFER_ENCODING == 256
        strGeomTemplate = "AsEWKB(%s(\"%s\"),'%s') as geom,\"%s\"::text";
#else
        strGeomTemplate = "encode(AsEWKB(%s(\"%s\"),'%s'),'hex') as geom,\"%s\"";
#endif
    }
    strGeom = (char*)msSmallMalloc(strlen(strGeomTemplate) + strlen(force2d) + strlen(strEndian) + strlen(layerinfo->geomcolumn) + strlen(layerinfo->uid) + 1);
    sprintf(strGeom, strGeomTemplate, force2d, layerinfo->geomcolumn, strEndian, layerinfo->uid);
  }

  if( layer->debug > 1 ) {
    msDebug("msPostGISBuildSQLItems: %d items requested.\n",layer->numitems);
  }

  /*
  ** Not requesting items? We just need geometry and unique id.
  */
  if (layer->numitems == 0) {
    strItems = msStrdup(strGeom);
  }
  /*
  ** Build SQL to pull all the items.
  */
  else {
    int length = strlen(strGeom) + 2;
    int t;
    for ( t = 0; t < layer->numitems; t++ ) {
      length += strlen(layer->items[t]) + 3; /* itemname + "", */
#if TRANSFER_ENCODING == 256
      length +=6; /*add a ::text*/
#endif
    }
    strItems = (char*)msSmallMalloc(length);
    strItems[0] = '\0';
    for ( t = 0; t < layer->numitems; t++ ) {
      strlcat(strItems, "\"", length);
      strlcat(strItems, layer->items[t], length);
#if TRANSFER_ENCODING == 256
      strlcat(strItems, "\"::text,", length);
#else
      strlcat(strItems, "\",", length);
#endif
    }
    strlcat(strItems, strGeom, length);
  }

  free(strGeom);
  return strItems;
}


/*
** msPostGISFindTableName()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISFindTableName(char* fromsource)
{
  char *f_table_name = NULL;
  char *pos = strstr(fromsource, " ");
 
  if ( ! pos ) {
    /* target table is one word */
    f_table_name = msStrdup(fromsource);
  } else {
    /* target table is hiding in sub-select clause */
    pos = strcasestr(fromsource, " from ");
    if ( pos ) {
      char *pos_paren;
      char *pos_space;
      pos += 6; /* should be start of table name */
      pos_paren = strstr(pos, ")"); /* first ) after table name */
      pos_space = strstr(pos, " "); /* first space after table name */
      if ( pos_space < pos_paren ) {
        /* found space first */
        f_table_name = (char*)msSmallMalloc(pos_space - pos + 1);
        strlcpy(f_table_name, pos, pos_space - pos+1);
      } else {
        /* found ) first */
        f_table_name = (char*)msSmallMalloc(pos_paren - pos + 1);
        strlcpy(f_table_name, pos, pos_paren - pos+1);
      }
    }
  }
  return f_table_name;
}


/*
** msPostGISBuildSQLSRID()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLSRID(layerObj *layer)
{

  char *strSRID = NULL;
  msPostGISLayerInfo *layerinfo = NULL;

  if (layer->debug) {
    msDebug("msPostGISBuildSQLSRID called.\n");
  }

  assert( layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

  /* An SRID was already provided in the DATA line. */
  if ( layerinfo->srid && (strlen(layerinfo->srid) > 0) ) {
    strSRID = msStrdup(layerinfo->srid);
    if( layer->debug > 1 ) {
      msDebug("msPostGISBuildSQLSRID: SRID provided (%s)\n", strSRID);
    }
  }
  /*
  ** No SRID in data line, so extract target table from the 'fromsource'.
  ** Either of form "thetable" (one word) or "(select ... from thetable)"
  ** or "(select ... from thetable where ...)".
  */
  else {
    char *f_table_name = msPostGISFindTableName(layerinfo->fromsource);
    char *strSRIDTemplate = "find_srid('','%s','%s')";
    if( layer->debug > 1 ) {
      msDebug("msPostGISBuildSQLSRID: Building find_srid line.\n");
    }

    if (!f_table_name)
      return NULL;  /* should not happen */

    strSRID = msSmallMalloc(strlen(strSRIDTemplate) + strlen(f_table_name) + strlen(layerinfo->geomcolumn) + 1);
    sprintf(strSRID, strSRIDTemplate, f_table_name, layerinfo->geomcolumn);
    free(f_table_name);
  }
  return strSRID;
}


/*
** msPostGISReplaceBoxToken()
**
** Convert a fromsource data statement into something usable by replacing the !BOX! token.
**
** Returns malloc'ed char* that must be freed by caller.
*/
static char *msPostGISReplaceBoxToken(layerObj *layer, rectObj *rect, const char *fromsource)
{
  char *result = NULL;

  if ( strstr(fromsource, BOXTOKEN) && rect ) {
    char *strBox = NULL;
    char *strSRID = NULL;

    /* We see to set the SRID on the box, but to what SRID? */
    strSRID = msPostGISBuildSQLSRID(layer);
    if ( ! strSRID ) {
      return NULL;
    }

    /* Create a suitable SQL string from the rectangle and SRID. */
    strBox = msPostGISBuildSQLBox(layer, rect, strSRID);
    if ( ! strBox ) {
      msSetError(MS_MISCERR, "Unable to build box SQL.", "msPostGISReplaceBoxToken()");
      if (strSRID) free(strSRID);
      return NULL;
    }

    /* Do the substitution. */
    while ( strstr(fromsource, BOXTOKEN) ) {
      char    *start, *end;
      char    *oldresult = result;
      size_t buffer_size = 0;
      start = strstr(fromsource, BOXTOKEN);
      end = start + BOXTOKENLENGTH;

      buffer_size = (start - fromsource) + strlen(strBox) + strlen(end) +1;
      result = (char*)msSmallMalloc(buffer_size);

      strlcpy(result, fromsource, start - fromsource +1);
      strlcpy(result + (start - fromsource), strBox, buffer_size-(start - fromsource));
      strlcat(result, end, buffer_size);

      fromsource = result;
      if (oldresult != NULL)
        free(oldresult);
    }

    if (strSRID) free(strSRID);
    if (strBox) free(strBox);
  } else {
    result = msStrdup(fromsource);
  }
  return result;

}

/*
** msPostGISBuildSQLFrom()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLFrom(layerObj *layer, rectObj *rect)
{
  char *strFrom = 0;
  msPostGISLayerInfo *layerinfo;

  if (layer->debug) {
    msDebug("msPostGISBuildSQLFrom called.\n");
  }

  assert( layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

  if ( ! layerinfo->fromsource ) {
    msSetError(MS_MISCERR, "Layerinfo->fromsource is not initialized.", "msPostGISBuildSQLFrom()");
    return NULL;
  }

  /*
  ** If there's a '!BOX!' in our source we need to substitute the
  ** current rectangle for it...
  */
  strFrom = msPostGISReplaceBoxToken(layer, rect, layerinfo->fromsource);

  return strFrom;
}

/*
** msPostGISBuildSQLWhere()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLWhere(layerObj *layer, rectObj *rect, long *uid, rectObj *rectInOtherSRID, int otherSRID)
{
  char *strRect = 0;
  char *strFilter1=0, *strFilter2=0;
  char *strUid = 0;
  char *strWhere = 0;
  char *strOrderBy = 0;
  char *strLimit = 0;
  char *strOffset = 0;
  size_t strRectLength = 0;
  size_t strFilterLength1=0, strFilterLength2=0;
  size_t strUidLength = 0;
  size_t strOrderByLength = 0;
  size_t strLimitLength = 0;
  size_t strOffsetLength = 0;
  size_t bufferSize = 0;
  int insert_and = 0;
  msPostGISLayerInfo *layerinfo;

  if (layer->debug) {
    msDebug("msPostGISBuildSQLWhere called.\n");
  }

  assert( layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

  if ( ! layerinfo->fromsource ) {
    msSetError(MS_MISCERR, "Layerinfo->fromsource is not initialized.", "msPostGISBuildSQLWhere()");
    return NULL;
  }

  /* Populate strLimit, if necessary. */
  if ( layerinfo->paging && layer->maxfeatures >= 0 ) {
    static char *strLimitTemplate = " limit %d";
    strLimit = msSmallMalloc(strlen(strLimitTemplate) + 12);
    sprintf(strLimit, strLimitTemplate, layer->maxfeatures);
    strLimitLength = strlen(strLimit);
  }

  /* Populate strOffset, if necessary. */
  if ( layerinfo->paging && layer->startindex > 0 ) {
    static char *strOffsetTemplate = " offset %d";
    strOffset = msSmallMalloc(strlen(strOffsetTemplate) + 12);
    sprintf(strOffset, strOffsetTemplate, layer->startindex-1);
    strOffsetLength = strlen(strOffset);
  }

  /* Populate strRect, if necessary. */
  if ( rect && layerinfo->geomcolumn ) {
    char *strBox = 0;
    char *strSRID = 0;
    size_t strBoxLength = 0;

    /* We see to set the SRID on the box, but to what SRID? */
    strSRID = msPostGISBuildSQLSRID(layer);
    if ( ! strSRID ) {
      free( strLimit );
      free( strOffset );
      return NULL;
    }

    strBox = msPostGISBuildSQLBox(layer, rect, strSRID);
    if ( strBox ) {
      strBoxLength = strlen(strBox);
    } else {
      msSetError(MS_MISCERR, "Unable to build box SQL.", "msPostGISBuildSQLWhere()");
      free( strLimit );
      free( strOffset );
      msFree(strSRID);
      return NULL;
    }

    if( strstr(strSRID, "find_srid(") == NULL )
    {
        // If the SRID is known, we can safely use ST_Intersects()
        // otherwise if find_srid() would return 0, ST_Intersects() would not
        // work at all, which breaks the msautotest/query/query_postgis.map
        // tests, releated to bdry_counpy2 layer that has no SRID
        static const char *strRectTemplate = "ST_Intersects(\"%s\", %s)";
        strRect = (char*)msSmallMalloc(strlen(strRectTemplate) + strBoxLength + strlen(layerinfo->geomcolumn) +1 );
        sprintf(strRect, strRectTemplate, layerinfo->geomcolumn, strBox);
    }
    else
    {
        static const char *strRectTemplate = "\"%s\" && %s";
        strRect = (char*)msSmallMalloc(strlen(strRectTemplate) + strBoxLength + strlen(layerinfo->geomcolumn) +1 );
        sprintf(strRect, strRectTemplate, layerinfo->geomcolumn, strBox);
    }
    strRectLength = strlen(strRect);
    free(strBox);
    msFree(strSRID);

    /* Combine with other rectangle  expressed in another SRS */
    /* (generally equivalent to the above in current code paths) */
    if( rectInOtherSRID != NULL && otherSRID > 0 )
    {
      char* strRectOtherSRID;
      static const char *strRectOtherSRIDTemplate = "ST_Intersects(ST_Transform(%s,%d),%s)";
      char szSRID[32];
      char* strTmp = NULL;

      sprintf(szSRID, "%d", otherSRID);
 
      strBox = msPostGISBuildSQLBox(layer, rectInOtherSRID, szSRID);
      if ( strBox ) {
        strBoxLength = strlen(strBox);
      } else {
        msSetError(MS_MISCERR, "Unable to build box SQL.", "msPostGISBuildSQLWhere()");
        free( strLimit );
        free( strOffset );
        return NULL;
      }

      strRectOtherSRID = (char*)msSmallMalloc(strlen(strRectOtherSRIDTemplate) + strBoxLength + strlen(layerinfo->geomcolumn) +1 );
      sprintf(strRectOtherSRID, strRectOtherSRIDTemplate, layerinfo->geomcolumn, otherSRID, strBox);
      free(strBox);

      strTmp = msStringConcatenate(strTmp, "((");
      strTmp = msStringConcatenate(strTmp, strRect);
      strTmp = msStringConcatenate(strTmp, ") AND ");
      strTmp = msStringConcatenate(strTmp, strRectOtherSRID);
      strTmp = msStringConcatenate(strTmp, ")");

      msFree(strRect);
      msFree(strRectOtherSRID);
      strRect = strTmp;
      strRectLength = strlen(strRect);
    }
    else if( rectInOtherSRID != NULL && otherSRID < 0 )
    {
      char* strSRID;
      char* strRectOtherSRID;
      static const char *strRectOtherSRIDTemplate = "ST_Intersects(%s,%s)";
      char* strTmp = NULL;

      strSRID = msPostGISBuildSQLSRID(layer);
      if ( ! strSRID ) {
        free( strLimit );
        free( strOffset );
        return NULL;
      }
      strBox = msPostGISBuildSQLBox(layer, rectInOtherSRID, strSRID);
      msFree(strSRID);
      if ( strBox ) {
        strBoxLength = strlen(strBox);
      } else {
        msSetError(MS_MISCERR, "Unable to build box SQL.", "msPostGISBuildSQLWhere()");
        free( strLimit );
        free( strOffset );
        return NULL;
      }

      strRectOtherSRID = (char*)msSmallMalloc(strlen(strRectOtherSRIDTemplate) + strBoxLength + strlen(layerinfo->geomcolumn) +1 );
      sprintf(strRectOtherSRID, strRectOtherSRIDTemplate, layerinfo->geomcolumn, strBox);
      free(strBox);

      strTmp = msStringConcatenate(strTmp, "((");
      strTmp = msStringConcatenate(strTmp, strRect);
      strTmp = msStringConcatenate(strTmp, ") AND ");
      strTmp = msStringConcatenate(strTmp, strRectOtherSRID);
      strTmp = msStringConcatenate(strTmp, ")");

      msFree(strRect);
      msFree(strRectOtherSRID);
      strRect = strTmp;
      strRectLength = strlen(strRect);
    }
  }

  /* Handle a translated filter (RFC91). */
  if ( layer->filter.native_string ) { 
    static char *strFilterTemplate = "(%s)";
    strFilter1 = (char *) msSmallMalloc(strlen(strFilterTemplate) + strlen(layer->filter.native_string)+1);
    sprintf(strFilter1, strFilterTemplate, layer->filter.native_string);
    strFilterLength1 = strlen(strFilter1);
  }

  /* Handle a native filter set as a PROCESSING option (#5001). */
  if( msLayerGetProcessingKey(layer, "NATIVE_FILTER") != NULL ) {
    static char *strFilterTemplate = "(%s)";
    char *native_filter = msLayerGetProcessingKey(layer, "NATIVE_FILTER");
    strFilter2 = (char *) msSmallMalloc(strlen(strFilterTemplate) + strlen(native_filter)+1);
    sprintf(strFilter2, strFilterTemplate, native_filter);
    strFilterLength2 = strlen(strFilter2);
  }

  /* Populate strUid, if necessary. */
  if ( uid ) {
    static char *strUidTemplate = "\"%s\" = %ld";
    strUid = (char*)msSmallMalloc(strlen(strUidTemplate) + strlen(layerinfo->uid) + 64);
    sprintf(strUid, strUidTemplate, layerinfo->uid, *uid);
    strUidLength = strlen(strUid);
  }

  /* Populate strOrderBy, if necessary */
  if( layer->sortBy.nProperties > 0 ) {
    char* pszTmp = msLayerBuildSQLOrderBy(layer);
    strOrderBy = msStringConcatenate(strOrderBy, " ORDER BY ");
    strOrderBy = msStringConcatenate(strOrderBy, pszTmp);
    msFree(pszTmp);
    strOrderByLength = strlen(strOrderBy);
  }

  bufferSize = strRectLength + 5 + (strFilterLength1 + 5) + (strFilterLength2 + 5) + strUidLength
               + strLimitLength + strOffsetLength + strOrderByLength + 1;
  strWhere = (char*)msSmallMalloc(bufferSize);
  *strWhere = '\0';
  if ( strRect ) {
    strlcat(strWhere, strRect, bufferSize);
    insert_and++;
    free(strRect);
  }
  if ( strFilter1 ) {
    if ( insert_and ) {
      strlcat(strWhere, " and ", bufferSize);
    }
    strlcat(strWhere, strFilter1, bufferSize);
    free(strFilter1);
    insert_and++;
  }
  if ( strFilter2 ) {
    if ( insert_and ) {
      strlcat(strWhere, " and ", bufferSize);
    }
    strlcat(strWhere, strFilter2, bufferSize);
    free(strFilter2);
    insert_and++;
  }
  if ( strUid ) {
    if ( insert_and ) {
      strlcat(strWhere, " and ", bufferSize);
    }
    strlcat(strWhere, strUid, bufferSize);
    free(strUid);
    insert_and++;
  }

  if ( strOrderBy ) {
    strlcat(strWhere, strOrderBy, bufferSize);
    free(strOrderBy);
  }

  if ( strLimit ) {
    strlcat(strWhere, strLimit, bufferSize);
    free(strLimit);
  }
  if ( strOffset ) {
    strlcat(strWhere, strOffset, bufferSize);
    free(strOffset);
  }

  return strWhere;
}

/*
** msPostGISBuildSQL()
**
** rect is the search rectangle in layer SRS. It can be set to NULL
** uid can be set to NULL
** rectInOtherSRID is an additional rectangle potentially in another SRS. It can be set to NULL.
** Only used if rect != NULL
** otherSRID is the SRID of the additional rectangle. It can be set to -1 if
** rectInOtherSRID is in the SRID of the layer.
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQL(layerObj *layer, rectObj *rect, long *uid, rectObj *rectInOtherSRID, int otherSRID)
{

  msPostGISLayerInfo *layerinfo = 0;
  char *strFrom = 0;
  char *strItems = 0;
  char *strWhere = 0;
  char *strSQL = 0;
  static char *strSQLTemplate0 = "select %s from %s where %s";
  static char *strSQLTemplate1 = "select %s from %s%s";
  char *strSQLTemplate = 0;

  if (layer->debug) {
    msDebug("msPostGISBuildSQL called.\n");
  }

  assert( layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

  strItems = msPostGISBuildSQLItems(layer);
  if ( ! strItems ) {
    msSetError(MS_MISCERR, "Failed to build SQL items.", "msPostGISBuildSQL()");
    return NULL;
  }

  strFrom = msPostGISBuildSQLFrom(layer, rect);

  if ( ! strFrom ) {
    msSetError(MS_MISCERR, "Failed to build SQL 'from'.", "msPostGISBuildSQL()");
    return NULL;
  }

  /* If there's BOX hackery going on, we don't want to append a box index test at
     the end of the query, the user is going to be responsible for making things
     work with their hackery. */
  if ( strstr(layerinfo->fromsource, BOXTOKEN) )
    strWhere = msPostGISBuildSQLWhere(layer, NULL, uid, rectInOtherSRID, otherSRID);
  else
    strWhere = msPostGISBuildSQLWhere(layer, rect, uid, rectInOtherSRID, otherSRID);

  if ( ! strWhere ) {
    msSetError(MS_MISCERR, "Failed to build SQL 'where'.", "msPostGISBuildSQL()");
    return NULL;
  }

  strSQLTemplate = strlen(strWhere) ? strSQLTemplate0 : strSQLTemplate1;

  strSQL = msSmallMalloc(strlen(strSQLTemplate) + strlen(strFrom) + strlen(strItems) + strlen(strWhere) + 1);
  sprintf(strSQL, strSQLTemplate, strItems, strFrom, strWhere);

  free(strItems);
  free(strFrom);
  free(strWhere);

  return strSQL;
}

#define wkbstaticsize 4096
int msPostGISReadShape(layerObj *layer, shapeObj *shape)
{

  char *wkbstr = NULL;
  unsigned char wkbstatic[wkbstaticsize];
  unsigned char *wkb = NULL;
  wkbObj w;
  msPostGISLayerInfo *layerinfo = NULL;
  int result = 0;
  int wkbstrlen = 0;

  if (layer->debug) {
    msDebug("msPostGISReadShape called.\n");
  }

  assert(layer->layerinfo != NULL);
  layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

  /* Retrieve the geometry. */
  wkbstr = (char*)PQgetvalue(layerinfo->pgresult, layerinfo->rownum, layer->numitems );
  wkbstrlen = PQgetlength(layerinfo->pgresult, layerinfo->rownum, layer->numitems);

  if ( ! wkbstr ) {
    msSetError(MS_QUERYERR, "WKB returned is null!", "msPostGISReadShape()");
    return MS_FAILURE;
  }

  if(wkbstrlen > wkbstaticsize) {
    wkb = calloc(wkbstrlen, sizeof(char));
  } else {
    wkb = wkbstatic;
  }
#if TRANSFER_ENCODING == 64
  result = msPostGISBase64Decode(wkb, wkbstr, wkbstrlen - 1);
  w.size = (wkbstrlen - 1)/2;
#elif TRANSFER_ENCODING == 256
  result = 1;
  memcpy(wkb, wkbstr, wkbstrlen);
  w.size = wkbstrlen;
#else
  result = msPostGISHexDecode(wkb, wkbstr, wkbstrlen);
  w.size = (wkbstrlen - 1)/2;
#endif

  if( ! result ) {
    if(wkb!=wkbstatic) free(wkb);
    return MS_FAILURE;
  }

  /* Initialize our wkbObj */
  w.wkb = (char*)wkb;
  w.ptr = w.wkb;

  /* Set the type map according to what version of PostGIS we are dealing with */
  if( layerinfo->version >= 20000 ) /* PostGIS 2.0+ */
    w.typemap = wkb_postgis20;
  else
  {
    w.typemap = wkb_postgis15;
    if( layerinfo->force2d == MS_FALSE )
    {
        /* Is there SRID ? Skip it */
        if( w.size >= 9 && (w.ptr[4] & 0x20) != 0 )
        {
            w.ptr[5] = w.ptr[1];
            w.ptr[6] = w.ptr[2];
            w.ptr[7] = w.ptr[3];
            w.ptr[8] = w.ptr[4] & ~(0x20);
            w.ptr[4] = 1;
            w.ptr += 4;
            w.size -= 4;
        }
    }
  }

  switch (layer->type) {

    case MS_LAYER_POINT:
      shape->type = MS_SHAPE_POINT;
      result = wkbConvGeometryToShape(&w, shape);
      break;

    case MS_LAYER_LINE:
      shape->type = MS_SHAPE_LINE;
      result = wkbConvGeometryToShape(&w, shape);
      break;

    case MS_LAYER_POLYGON:
      shape->type = MS_SHAPE_POLYGON;
      result = wkbConvGeometryToShape(&w, shape);
      break;

    case MS_LAYER_QUERY:
    case MS_LAYER_CHART:
      result = msPostGISFindBestType(&w, shape);
      break;

    case MS_LAYER_RASTER:
      msDebug("Ignoring MS_LAYER_RASTER in msPostGISReadShape.\n");
      break;

    case MS_LAYER_CIRCLE:
      msDebug("Ignoring MS_LAYER_RASTER in msPostGISReadShape.\n");
      break;

    default:
      msDebug("Unsupported layer type in msPostGISReadShape()!\n");
      break;
  }

  /* All done with WKB geometry, free it! */
  if(wkb!=wkbstatic) free(wkb);

  if (result != MS_FAILURE) {
    int t;
    long uid;
    char *tmp;
    /* Found a drawable shape, so now retreive the attributes. */

    shape->values = (char**) msSmallMalloc(sizeof(char*) * layer->numitems);
    for ( t = 0; t < layer->numitems; t++) {
      int size = PQgetlength(layerinfo->pgresult, layerinfo->rownum, t);
      char *val = (char*)PQgetvalue(layerinfo->pgresult, layerinfo->rownum, t);
      int isnull = PQgetisnull(layerinfo->pgresult, layerinfo->rownum, t);
      if ( isnull ) {
        shape->values[t] = msStrdup("");
      } else {
        shape->values[t] = (char*) msSmallMalloc(size + 1);
        memcpy(shape->values[t], val, size);
        shape->values[t][size] = '\0'; /* null terminate it */
        msStringTrimBlanks(shape->values[t]);
      }
      if( layer->debug > 4 ) {
        msDebug("msPostGISReadShape: PQgetlength = %d\n", size);
      }
      if( layer->debug > 1 ) {
        msDebug("msPostGISReadShape: [%s] \"%s\"\n", layer->items[t], shape->values[t]);
      }
    }

    /* t is the geometry, t+1 is the uid */
    tmp = PQgetvalue(layerinfo->pgresult, layerinfo->rownum, t + 1);
    if( tmp ) {
      uid = strtol( tmp, NULL, 10 );
    } else {
      uid = 0;
    }
    if( layer->debug > 4 ) {
      msDebug("msPostGISReadShape: Setting shape->index = %ld\n", uid);
      msDebug("msPostGISReadShape: Setting shape->resultindex = %ld\n", layerinfo->rownum);
    }
    shape->index = uid;
    shape->resultindex = layerinfo->rownum;

    if( layer->debug > 2 ) {
      msDebug("msPostGISReadShape: [index] %ld\n",  shape->index);
    }

    shape->numvalues = layer->numitems;

    msComputeBounds(shape);
  } else {
     shape->type = MS_SHAPE_NULL;
  }

  if( layer->debug > 2 ) {
    char *tmp = msShapeToWKT(shape);
    msDebug("msPostGISReadShape: [shape] %s\n", tmp);
    free(tmp);
  }

  return MS_SUCCESS;
}

#endif /* USE_POSTGIS */


/*
** msPostGISLayerOpen()
**
** Registered vtable->LayerOpen function.
*/
int msPostGISLayerOpen(layerObj *layer)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo  *layerinfo;
  int order_test = 1;
  const char* force2d_processing;

  assert(layer != NULL);

  if (layer->debug) {
    msDebug("msPostGISLayerOpen called: %s\n", layer->data);
  }

  if (layer->layerinfo) {
    if (layer->debug) {
      msDebug("msPostGISLayerOpen: Layer is already open!\n");
    }
    return MS_SUCCESS;  /* already open */
  }

  if (!layer->data) {
    msSetError(MS_QUERYERR, "Nothing specified in DATA statement.", "msPostGISLayerOpen()");
    return MS_FAILURE;
  }

  /*
  ** Initialize the layerinfo
  **/
  layerinfo = msPostGISCreateLayerInfo();

  if (((char*) &order_test)[0] == 1) {
    layerinfo->endian = LITTLE_ENDIAN;
  } else {
    layerinfo->endian = BIG_ENDIAN;
  }

  /*
  ** Get a database connection from the pool.
  */
  layerinfo->pgconn = (PGconn *) msConnPoolRequest(layer);

  /* No connection in the pool, so set one up. */
  if (!layerinfo->pgconn) {
    char *conn_decrypted;
    if (layer->debug) {
      msDebug("msPostGISLayerOpen: No connection in pool, creating a fresh one.\n");
    }

    if (!layer->connection) {
      msSetError(MS_MISCERR, "Missing CONNECTION keyword.", "msPostGISLayerOpen()");
      free(layerinfo);
      return MS_FAILURE;
    }

    /*
    ** Decrypt any encrypted token in connection string and attempt to connect.
    */
    conn_decrypted = msDecryptStringTokens(layer->map, layer->connection);
    if (conn_decrypted == NULL) {
      free(layerinfo);
      return MS_FAILURE;  /* An error should already have been produced */
    }
    layerinfo->pgconn = PQconnectdb(conn_decrypted);
    msFree(conn_decrypted);
    conn_decrypted = NULL;

    /*
    ** Connection failed, return error message with passwords ***ed out.
    */
    if (!layerinfo->pgconn || PQstatus(layerinfo->pgconn) == CONNECTION_BAD) {
      char *index, *maskeddata;
      if (layer->debug)
        msDebug("msPostGISLayerOpen: Connection failure.\n");

      maskeddata = msStrdup(layer->connection);
      index = strstr(maskeddata, "password=");
      if (index != NULL) {
        index = (char*)(index + 9);
        while (*index != '\0' && *index != ' ') {
          *index = '*';
          index++;
        }
      }

      msDebug( "Database connection failed (%s) with connect string '%s'\nIs the database running? Is it allowing connections? Does the specified user exist? Is the password valid? Is the database on the standard port? in msPostGISLayerOpen()", PQerrorMessage(layerinfo->pgconn), maskeddata);
      msSetError(MS_QUERYERR, "Database connection failed. Check server logs for more details.Is the database running? Is it allowing connections? Does the specified user exist? Is the password valid? Is the database on the standard port?", "msPostGISLayerOpen()");

      if(layerinfo->pgconn) PQfinish(layerinfo->pgconn);
      free(maskeddata);
      free(layerinfo);
      return MS_FAILURE;
    }

    /* Register to receive notifications from the database. */
    PQsetNoticeProcessor(layerinfo->pgconn, postresqlNoticeHandler, (void *) layer);

    /* Save this connection in the pool for later. */
    msConnPoolRegister(layer, layerinfo->pgconn, msPostGISCloseConnection);
  } else {
    /* Connection in the pool should be tested to see if backend is alive. */
    if( PQstatus(layerinfo->pgconn) != CONNECTION_OK ) {
      /* Uh oh, bad connection. Can we reset it? */
      PQreset(layerinfo->pgconn);
      if( PQstatus(layerinfo->pgconn) != CONNECTION_OK ) {
        /* Nope, time to bail out. */
        msSetError(MS_QUERYERR, "PostgreSQL database connection. Check server logs for more details", "msPostGISLayerOpen()");
        msDebug( "PostgreSQL database connection gone bad (%s) in msPostGISLayerOpen()", PQerrorMessage(layerinfo->pgconn));
        free(layerinfo);
        /* FIXME: we should also release the connection from the pool in this case, but it is stale...
         * for the time being we do not release it so it can never be used again. If this happens multiple
         * times there will be a leak... */
        return MS_FAILURE;
      }
    }
  }

  /* Get the PostGIS version number from the database */
  layerinfo->version = msPostGISRetrieveVersion(layerinfo->pgconn);
  if( layerinfo->version == MS_FAILURE ) {
    msConnPoolRelease(layer, layerinfo->pgconn);
    free(layerinfo);
    return MS_FAILURE;
  }
  if (layer->debug)
    msDebug("msPostGISLayerOpen: Got PostGIS version %d.\n", layerinfo->version);

  force2d_processing = msLayerGetProcessingKey( layer, "FORCE2D" );
  if(force2d_processing && !strcasecmp(force2d_processing,"no")) {
    layerinfo->force2d = MS_FALSE;
  }
  else if(force2d_processing && !strcasecmp(force2d_processing,"yes")) {
    layerinfo->force2d = MS_TRUE;
  }
  if (layer->debug)
    msDebug("msPostGISLayerOpen: Forcing 2D geometries: %s.\n", (layerinfo->force2d)?"yes":"no");

  /* Save the layerinfo in the layerObj. */
  layer->layerinfo = (void*)layerinfo;

  return MS_SUCCESS;
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISLayerOpen()");
  return MS_FAILURE;
#endif
}

/*
** msPostGISLayerClose()
**
** Registered vtable->LayerClose function.
*/
int msPostGISLayerClose(layerObj *layer)
{
#ifdef USE_POSTGIS

  if (layer->debug) {
    msDebug("msPostGISLayerClose called: %s\n", layer->data);
  }

  if( layer->layerinfo ) {
    msPostGISFreeLayerInfo(layer);
  }

  return MS_SUCCESS;
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISLayerClose()");
  return MS_FAILURE;
#endif
}


/*
** msPostGISLayerIsOpen()
**
** Registered vtable->LayerIsOpen function.
*/
int msPostGISLayerIsOpen(layerObj *layer)
{
#ifdef USE_POSTGIS

  if (layer->debug) {
    msDebug("msPostGISLayerIsOpen called.\n");
  }

  if (layer->layerinfo)
    return MS_TRUE;
  else
    return MS_FALSE;
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISLayerIsOpen()");
  return MS_FAILURE;
#endif
}


/*
** msPostGISLayerFreeItemInfo()
**
** Registered vtable->LayerFreeItemInfo function.
*/
void msPostGISLayerFreeItemInfo(layerObj *layer)
{
#ifdef USE_POSTGIS
  if (layer->debug) {
    msDebug("msPostGISLayerFreeItemInfo called.\n");
  }

  if (layer->iteminfo) {
    free(layer->iteminfo);
  }
  layer->iteminfo = NULL;
#endif
}

/*
** msPostGISLayerInitItemInfo()
**
** Registered vtable->LayerInitItemInfo function.
** Our iteminfo is list of indexes from 1..numitems.
*/
int msPostGISLayerInitItemInfo(layerObj *layer)
{
#ifdef USE_POSTGIS
  int i;
  int *itemindexes ;

  if (layer->debug) {
    msDebug("msPostGISLayerInitItemInfo called.\n");
  }

  if (layer->numitems == 0) {
    return MS_SUCCESS;
  }

  if (layer->iteminfo) {
    free(layer->iteminfo);
  }

  layer->iteminfo = msSmallMalloc(sizeof(int) * layer->numitems);
  if (!layer->iteminfo) {
    msSetError(MS_MEMERR, "Out of memory.", "msPostGISLayerInitItemInfo()");
    return MS_FAILURE;
  }

  itemindexes = (int*)layer->iteminfo;
  for (i = 0; i < layer->numitems; i++) {
    itemindexes[i] = i; /* Last item is always the geometry. The rest are non-geometry. */
  }

  return MS_SUCCESS;
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISLayerInitItemInfo()");
  return MS_FAILURE;
#endif
}

#ifdef USE_POSTGIS
static const char** buildBindValues(layerObj *layer, int* p_num_bind_values)
{
  /* try to get the first bind value */
  char bind_key[20];
  const char* bind_value = msLookupHashTable(&layer->bindvals, "1");
  int num_bind_values = 0;
  const char** layer_bind_values = (const char**)(bind_value ? msSmallMalloc(sizeof(const char*) * 1000) : NULL);
  while(bind_value != NULL && num_bind_values < 1000) {
    /* put the bind value on the stack */
    layer_bind_values[num_bind_values] = bind_value;
    /* increment the counter */
    num_bind_values++;
    /* create a new lookup key */
    sprintf(bind_key, "%d", num_bind_values+1);
    /* get the bind_value */
    bind_value = msLookupHashTable(&layer->bindvals, bind_key);
  }
  *p_num_bind_values = num_bind_values;
  return layer_bind_values;
}

static void freeBindValues(const char** layer_bind_values)
{
  free((void*)layer_bind_values);
}

static PGresult* runPQexecParamsWithBindSubstitution(layerObj *layer, const char* strSQL, int binary)
{
  PGresult *pgresult = NULL;
  msPostGISLayerInfo* layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

  int num_bind_values = 0;
  const char** layer_bind_values = buildBindValues(layer, &num_bind_values);

  if(num_bind_values > 0) {
    pgresult = PQexecParams(layerinfo->pgconn, strSQL, num_bind_values, NULL, layer_bind_values, NULL, NULL, binary);
  } else {
    pgresult = PQexecParams(layerinfo->pgconn, strSQL, 0, NULL, NULL, NULL, NULL, binary);
  }

  /* free bind values */
  freeBindValues(layer_bind_values);

  return pgresult;
}
#endif

/*
** msPostGISLayerWhichShapes()
**
** Registered vtable->LayerWhichShapes function.
*/
int msPostGISLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo *layerinfo = NULL;
  char *strSQL = NULL;
  PGresult *pgresult = NULL;

  assert(layer != NULL);
  assert(layer->layerinfo != NULL);

  if (layer->debug) {
    msDebug("msPostGISLayerWhichShapes called.\n");
  }

  /* Fill out layerinfo with our current DATA state. */
  if ( msPostGISParseData(layer) != MS_SUCCESS) {
    return MS_FAILURE;
  }

  /*
  ** This comes *after* parsedata, because parsedata fills in
  ** layer->layerinfo.
  */
  layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

  /* Build a SQL query based on our current state. */
  strSQL = msPostGISBuildSQL(layer, &rect, NULL, NULL, -1);
  if ( ! strSQL ) {
    msSetError(MS_QUERYERR, "Failed to build query SQL.", "msPostGISLayerWhichShapes()");
    return MS_FAILURE;
  }

  if (layer->debug) {
    msDebug("msPostGISLayerWhichShapes query: %s\n", strSQL);
  }

  pgresult = runPQexecParamsWithBindSubstitution(layer, strSQL, RESULTSET_TYPE);

  if ( layer->debug > 1 ) {
    msDebug("msPostGISLayerWhichShapes query status: %s (%d)\n", PQresStatus(PQresultStatus(pgresult)), PQresultStatus(pgresult));
  }

  /* Something went wrong. */
  if (!pgresult || PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
    msDebug("msPostGISLayerWhichShapes(): Error (%s) executing query: %s\n", PQerrorMessage(layerinfo->pgconn), strSQL);
    msSetError(MS_QUERYERR, "Error executing query. Check server logs","msPostGISLayerWhichShapes()");
    free(strSQL);
    if (pgresult) {
      PQclear(pgresult);
    }
    return MS_FAILURE;
  }

  if ( layer->debug ) {
    msDebug("msPostGISLayerWhichShapes got %d records in result.\n", PQntuples(pgresult));
  }

  /* Clean any existing pgresult before storing current one. */
  if(layerinfo->pgresult) PQclear(layerinfo->pgresult);
  layerinfo->pgresult = pgresult;

  /* Clean any existing SQL before storing current. */
  if(layerinfo->sql) free(layerinfo->sql);
  layerinfo->sql = strSQL;

  layerinfo->rownum = 0;

  return MS_SUCCESS;
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISLayerWhichShapes()");
  return MS_FAILURE;
#endif
}

/*
** msPostGISLayerNextShape()
**
** Registered vtable->LayerNextShape function.
*/
int msPostGISLayerNextShape(layerObj *layer, shapeObj *shape)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo  *layerinfo;

  if (layer->debug) {
    msDebug("msPostGISLayerNextShape called.\n");
  }

  assert(layer != NULL);
  assert(layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

  shape->type = MS_SHAPE_NULL;

  /*
  ** Roll through pgresult until we hit non-null shape (usually right away).
  */
  while (shape->type == MS_SHAPE_NULL) {
    if (layerinfo->rownum < PQntuples(layerinfo->pgresult)) {
      /* Retrieve this shape, cursor access mode. */
      msPostGISReadShape(layer, shape);
      if( shape->type != MS_SHAPE_NULL ) {
        (layerinfo->rownum)++; /* move to next shape */
        return MS_SUCCESS;
      } else {
        (layerinfo->rownum)++; /* move to next shape */
      }
    } else {
      return MS_DONE;
    }
  }

  /* Found nothing, clean up and exit. */
  msFreeShape(shape);

  return MS_FAILURE;
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISLayerNextShape()");
  return MS_FAILURE;
#endif
}

/*
** msPostGISLayerGetShape()
**
 */
int msPostGISLayerGetShapeCount(layerObj *layer, rectObj rect, projectionObj *rectProjection)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo *layerinfo = NULL;
  char *strSQL = NULL;
  char *strSQLCount = NULL;
  PGresult *pgresult = NULL;
  int nCount = 0;
  int rectSRID = -1;
  rectObj searchrectInLayerProj = rect;

  assert(layer != NULL);
  assert(layer->layerinfo != NULL);

  if (layer->debug) {
    msDebug("msPostGISLayerGetShapeCount called.\n");
  }


  // Special processing if the specified projection for the rect is different from the layer projection
  // We want to issue a WHERE that includes
  // ((the_geom && rect_reprojected_in_layer_SRID) AND NOT ST_Disjoint(ST_Transform(the_geom, rect_SRID), rect))
  if( rectProjection != NULL && layer->project &&
      msProjectionsDiffer(&(layer->projection), rectProjection) )
  {
    // If we cannot guess the EPSG code of the rectProjection, we cannot
    // use ST_Transform, so fallback on slow implementation
    if( rectProjection->numargs < 1 ||
        strncasecmp(rectProjection->args[0], "init=epsg:", strlen("init=epsg:")) != 0 )
    {
      if (layer->debug) {
        msDebug("msPostGISLayerGetShapeCount(): cannot find EPSG code of rectProjection. Falling back on client-side feature count.\n");
      }
      return LayerDefaultGetShapeCount(layer, rect, rectProjection);
    }

    // Reproject the passed rect into the layer projection and get
    // the SRID from the rectProjection
    msProjectRect(rectProjection, &(layer->projection), &searchrectInLayerProj); /* project the searchrect to source coords */
    rectSRID = atoi(rectProjection->args[0] + strlen("init=epsg:"));
  }

  msLayerTranslateFilter(layer, &layer->filter, layer->filteritem);

  /* Fill out layerinfo with our current DATA state. */
  if ( msPostGISParseData(layer) != MS_SUCCESS) {
    return -1;
  }

  /*
  ** This comes *after* parsedata, because parsedata fills in
  ** layer->layerinfo.
  */
  layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

  /* Build a SQL query based on our current state. */
  strSQL = msPostGISBuildSQL(layer, &searchrectInLayerProj, NULL,
                             &rect, rectSRID);
  if ( ! strSQL ) {
    msSetError(MS_QUERYERR, "Failed to build query SQL.", "msPostGISLayerGetShapeCount()");
    return -1;
  }

  strSQLCount = NULL;
  strSQLCount = msStringConcatenate(strSQLCount, "SELECT COUNT(*) FROM (");
  strSQLCount = msStringConcatenate(strSQLCount, strSQL);
  strSQLCount = msStringConcatenate(strSQLCount, ") msQuery");

  msFree(strSQL);

  if (layer->debug) {
    msDebug("msPostGISLayerGetShapeCount query: %s\n", strSQLCount);
  }

  pgresult = runPQexecParamsWithBindSubstitution(layer, strSQLCount, 0);

  if ( layer->debug > 1 ) {
    msDebug("msPostGISLayerWhichShapes query status: %s (%d)\n",
            PQresStatus(PQresultStatus(pgresult)), PQresultStatus(pgresult));
  }

  /* Something went wrong. */
  if (!pgresult || PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
    msDebug("msPostGISLayerGetShapeCount(): Error (%s) executing query: %s. "
            "Falling back to client-side evaluation\n",
            PQerrorMessage(layerinfo->pgconn), strSQLCount);
    msFree(strSQLCount);
    if (pgresult) {
      PQclear(pgresult);
    }
    return LayerDefaultGetShapeCount(layer, rect, rectProjection);
  }

  msFree(strSQLCount);
  nCount = atoi(PQgetvalue(pgresult, 0, 0 ));

  if ( layer->debug ) {
    msDebug("msPostGISLayerWhichShapes return: %d.\n", nCount);
  }
  PQclear(pgresult);

  return nCount;
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISLayerGetShapeCount()");
  return -1;
#endif
}


/*
** msPostGISLayerGetShape()
**
** Registered vtable->LayerGetShape function. For pulling from a prepared and
** undisposed result set.
*/
int msPostGISLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
#ifdef USE_POSTGIS

  PGresult *pgresult = NULL;
  msPostGISLayerInfo *layerinfo = NULL;

  long shapeindex = record->shapeindex;
  int resultindex = record->resultindex;

  assert(layer != NULL);
  assert(layer->layerinfo != NULL);

  if (layer->debug) {
    msDebug("msPostGISLayerGetShape called for record = %i\n", resultindex);
  }

  /* If resultindex is set, fetch the shape from the resultcache, otherwise fetch it from the DB  */
  if (resultindex >= 0) {
    int status;

    layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

    /* Check the validity of the open result. */
    pgresult = layerinfo->pgresult;
    if ( ! pgresult ) {
      msSetError( MS_MISCERR,
                  "PostgreSQL result set is null.",
                  "msPostGISLayerGetShape()");
      return MS_FAILURE;
    }
    status = PQresultStatus(pgresult);
    if ( layer->debug > 1 ) {
      msDebug("msPostGISLayerGetShape query status: %s (%d)\n", PQresStatus(status), status);
    }
    if( ! ( status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) ) {
      msSetError( MS_MISCERR,
                  "PostgreSQL result set is not ready.",
                  "msPostGISLayerGetShape()");
      return MS_FAILURE;
    }

    /* Check the validity of the requested record number. */
    if( resultindex >= PQntuples(pgresult) ) {
      msDebug("msPostGISLayerGetShape got request for (%d) but only has %d tuples.\n", resultindex, PQntuples(pgresult));
      msSetError( MS_MISCERR,
                  "Got request larger than result set.",
                  "msPostGISLayerGetShape()");
      return MS_FAILURE;
    }

    layerinfo->rownum = resultindex; /* Only return one result. */

    /* We don't know the shape type until we read the geometry. */
    shape->type = MS_SHAPE_NULL;

    /* Return the shape, cursor access mode. */
    msPostGISReadShape(layer, shape);

    return (shape->type == MS_SHAPE_NULL) ? MS_FAILURE : MS_SUCCESS;
  } else { /* no resultindex, fetch the shape from the DB */
    int num_tuples;
    char *strSQL = 0;

    /* Fill out layerinfo with our current DATA state. */
    if ( msPostGISParseData(layer) != MS_SUCCESS) {
      return MS_FAILURE;
    }

    /*
    ** This comes *after* parsedata, because parsedata fills in
    ** layer->layerinfo.
    */
    layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

    /* Build a SQL query based on our current state. */
    strSQL = msPostGISBuildSQL(layer, NULL, &shapeindex, NULL, -1);
    if ( ! strSQL ) {
      msSetError(MS_QUERYERR, "Failed to build query SQL.", "msPostGISLayerGetShape()");
      return MS_FAILURE;
    }

    if (layer->debug) {
      msDebug("msPostGISLayerGetShape query: %s\n", strSQL);
    }

    pgresult = runPQexecParamsWithBindSubstitution(layer, strSQL, RESULTSET_TYPE);

    /* Something went wrong. */
    if ( (!pgresult) || (PQresultStatus(pgresult) != PGRES_TUPLES_OK) ) {
      msDebug("msPostGISLayerGetShape(): Error (%s) executing SQL: %s\n", PQerrorMessage(layerinfo->pgconn), strSQL );
      msSetError(MS_QUERYERR, "Error executing SQL. Check server logs.","msPostGISLayerGetShape()");

      if (pgresult) {
        PQclear(pgresult);
      }
      free(strSQL);

      return MS_FAILURE;
    }

    /* Clean any existing pgresult before storing current one. */
    if(layerinfo->pgresult) PQclear(layerinfo->pgresult);
    layerinfo->pgresult = pgresult;

    /* Clean any existing SQL before storing current. */
    if(layerinfo->sql) free(layerinfo->sql);
    layerinfo->sql = strSQL;

    layerinfo->rownum = 0; /* Only return one result. */

    /* We don't know the shape type until we read the geometry. */
    shape->type = MS_SHAPE_NULL;

    num_tuples = PQntuples(pgresult);
    if (layer->debug) {
      msDebug("msPostGISLayerGetShape number of records: %d\n", num_tuples);
    }

    if (num_tuples > 0) {
      /* Get shape in random access mode. */
      msPostGISReadShape(layer, shape);
    }

    return (shape->type == MS_SHAPE_NULL) ? MS_FAILURE : ( (num_tuples > 0) ? MS_SUCCESS : MS_DONE );
  }
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISLayerGetShape()");
  return MS_FAILURE;
#endif
}

/**********************************************************************
 *                     msPostGISPassThroughFieldDefinitions()
 *
 * Pass the field definitions through to the layer metadata in the
 * "gml_[item]_{type,width,precision}" set of metadata items for
 * defining fields.
 **********************************************************************/

/* These are the OIDs for some builtin types, as returned by PQftype(). */
/* They were copied from pg_type.h in src/include/catalog/pg_type.h */

#ifndef BOOLOID
#define BOOLOID                 16
#define BYTEAOID                17
#define CHAROID                 18
#define NAMEOID                 19
#define INT8OID                 20
#define INT2OID                 21
#define INT2VECTOROID           22
#define INT4OID                 23
#define REGPROCOID              24
#define TEXTOID                 25
#define OIDOID                  26
#define TIDOID                  27
#define XIDOID                  28
#define CIDOID                  29
#define OIDVECTOROID            30
#define FLOAT4OID               700
#define FLOAT8OID               701
#define INT4ARRAYOID            1007
#define TEXTARRAYOID            1009
#define BPCHARARRAYOID          1014
#define VARCHARARRAYOID         1015
#define FLOAT4ARRAYOID          1021
#define FLOAT8ARRAYOID          1022
#define BPCHAROID   1042
#define VARCHAROID    1043
#define DATEOID     1082
#define TIMEOID     1083
#define TIMETZOID     1266
#define TIMESTAMPOID          1114
#define TIMESTAMPTZOID          1184
#define NUMERICOID              1700
#endif

#ifdef USE_POSTGIS
static void
msPostGISPassThroughFieldDefinitions( layerObj *layer,
                                      PGresult *pgresult )

{
  int i, numitems = PQnfields(pgresult);
  msPostGISLayerInfo *layerinfo = layer->layerinfo;

  for(i=0; i<numitems; i++) {
    int oid, fmod;
    const char *gml_type = "Character";
    const char *item = PQfname(pgresult,i);
    char md_item_name[256];
    char gml_width[32], gml_precision[32];

    gml_width[0] = '\0';
    gml_precision[0] = '\0';

    /* skip geometry column */
    if( strcmp(item, layerinfo->geomcolumn) == 0 )
      continue;

    oid = PQftype(pgresult,i);
    fmod = PQfmod(pgresult,i);

    if( (oid == BPCHAROID || oid == VARCHAROID) && fmod >= 4 ) {
      sprintf( gml_width, "%d", fmod-4 );

    } else if( oid == BOOLOID ) {
      gml_type = "Integer";
      sprintf( gml_width, "%d", 1 );

    } else if( oid == INT2OID ) {
      gml_type = "Integer";
      sprintf( gml_width, "%d", 5 );

    } else if( oid == INT4OID ) {
      gml_type = "Integer";

    } else if( oid == INT8OID ) {
      gml_type = "Long";

    } else if( oid == FLOAT4OID || oid == FLOAT8OID ) {
      gml_type = "Real";

    } else if( oid == NUMERICOID ) {
      gml_type = "Real";

      if( fmod >= 4 && ((fmod - 4) & 0xFFFF) == 0 ) {
        gml_type = "Integer";
        sprintf( gml_width, "%d", (fmod - 4) >> 16 );
      } else if( fmod >= 4 ) {
        sprintf( gml_width, "%d", (fmod - 4) >> 16 );
        sprintf( gml_precision, "%d", ((fmod-4) & 0xFFFF) );
      }
    } else if( oid == DATEOID ) {
      gml_type = "Date";
    } else if( oid == TIMEOID || oid == TIMETZOID ) {
      gml_type = "Time";
    } else if( oid == TIMESTAMPOID || oid == TIMESTAMPTZOID ) {
      gml_type = "DateTime";
    }

    snprintf( md_item_name, sizeof(md_item_name), "gml_%s_type", item );
    if( msOWSLookupMetadata(&(layer->metadata), "G", "type") == NULL )
      msInsertHashTable(&(layer->metadata), md_item_name, gml_type );

    snprintf( md_item_name, sizeof(md_item_name), "gml_%s_width", item );
    if( strlen(gml_width) > 0
        && msOWSLookupMetadata(&(layer->metadata), "G", "width") == NULL )
      msInsertHashTable(&(layer->metadata), md_item_name, gml_width );

    snprintf( md_item_name, sizeof(md_item_name), "gml_%s_precision",item );
    if( strlen(gml_precision) > 0
        && msOWSLookupMetadata(&(layer->metadata), "G", "precision")==NULL )
      msInsertHashTable(&(layer->metadata), md_item_name, gml_precision );
  }
}
#endif /* defined(USE_POSTGIS) */

/*
** msPostGISLayerGetItems()
**
** Registered vtable->LayerGetItems function. Query the database for
** column information about the requested layer. Rather than look in
** system tables, we just run a zero-cost query and read out of the
** result header.
*/
int msPostGISLayerGetItems(layerObj *layer)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo *layerinfo = NULL;
  static char *strSQLTemplate = "select * from %s where false limit 0";
  PGresult *pgresult = NULL;
  char *col = NULL;
  char *sql = NULL;
  char *strFrom = NULL;
  char found_geom = 0;
  const char *value;
  int t, item_num;
  rectObj rect;

  /* A useless rectangle for our useless query */
  rect.minx = rect.miny = rect.maxx = rect.maxy = 0.0;

  assert(layer != NULL);
  assert(layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

  assert(layerinfo->pgconn);

  if (layer->debug) {
    msDebug("msPostGISLayerGetItems called.\n");
  }

  /* Fill out layerinfo with our current DATA state. */
  if ( msPostGISParseData(layer) != MS_SUCCESS) {
    return MS_FAILURE;
  }

  layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

  /* This allocates a fresh string, so remember to free it... */
  strFrom = msPostGISReplaceBoxToken(layer, &rect, layerinfo->fromsource);

  /*
  ** Both the "table" and "(select ...) as sub" cases can be handled with the
  ** same SQL.
  */
  sql = (char*) msSmallMalloc(strlen(strSQLTemplate) + strlen(strFrom));
  sprintf(sql, strSQLTemplate, strFrom);
  free(strFrom);

  if (layer->debug) {
    msDebug("msPostGISLayerGetItems executing SQL: %s\n", sql);
  }

  pgresult = runPQexecParamsWithBindSubstitution(layer, sql, 0);

  if ( (!pgresult) || (PQresultStatus(pgresult) != PGRES_TUPLES_OK) ) {
    msDebug("msPostGISLayerGetItems(): Error (%s) executing SQL: %s\n", PQerrorMessage(layerinfo->pgconn), sql);
    msSetError(MS_QUERYERR, "Error executing SQL. Check server logs","msPostGISLayerGetItems()");
    if (pgresult) {
      PQclear(pgresult);
    }
    free(sql);
    return MS_FAILURE;
  }

  free(sql);

  layer->numitems = PQnfields(pgresult) - 1; /* dont include the geometry column (last entry)*/
  layer->items = msSmallMalloc(sizeof(char*) * (layer->numitems + 1)); /* +1 in case there is a problem finding geometry column */

  found_geom = 0; /* havent found the geom field */
  item_num = 0;

  for (t = 0; t < PQnfields(pgresult); t++) {
    col = PQfname(pgresult, t);
    if ( strcmp(col, layerinfo->geomcolumn) != 0 ) {
      /* this isnt the geometry column */
      layer->items[item_num] = msStrdup(col);
      item_num++;
    } else {
      found_geom = 1;
    }
  }

  /*
  ** consider populating the field definitions in metadata.
  */
  if((value = msOWSLookupMetadata(&(layer->metadata), "G", "types")) != NULL
      && strcasecmp(value,"auto") == 0 )
    msPostGISPassThroughFieldDefinitions( layer, pgresult );

  /*
  ** Cleanup
  */
  PQclear(pgresult);

  if (!found_geom) {
    msSetError(MS_QUERYERR, "Tried to find the geometry column in the database, but couldn't find it.  Is it mis-capitalized? '%s'", "msPostGISLayerGetItems()", layerinfo->geomcolumn);
    return MS_FAILURE;
  }

  return msPostGISLayerInitItemInfo(layer);
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISLayerGetItems()");
  return MS_FAILURE;
#endif
}

/*
** msPostGISLayerGetExtent()
**
** Registered vtable->LayerGetExtent function. Query the database for
** the extent of the requested layer.
*/
int msPostGISLayerGetExtent(layerObj *layer, rectObj *extent)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo *layerinfo = NULL;
  char *strSQL = NULL;
  char *strFilter1 = 0, *strFilter2 = 0;
  char *f_table_name;
  static char *sqlExtentTemplate = "SELECT ST_Extent(%s) FROM %s";
  size_t buffer_len;
  size_t strFilterLength1 = 0, strFilterLength2 = 0;
  PGresult *pgresult = NULL;
  
  if (layer->debug) {
    msDebug("msPostGISLayerGetExtent called.\n");
  }

  assert( layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

  if ( msPostGISParseData(layer) != MS_SUCCESS) {
    return MS_FAILURE;
  }

  /* if we have !BOX! substitution then we use just the table name */
  if ( strstr(layerinfo->fromsource, BOXTOKEN) )
    f_table_name = msPostGISFindTableName(layerinfo->fromsource);
  else
    f_table_name = msStrdup(layerinfo->fromsource);
  
  if ( !f_table_name ) {
    msSetError(MS_MISCERR, "Failed to get table name.", "msPostGISLayerGetExtent()");
    return MS_FAILURE;
  }

  /* Handle a translated filter (RFC91). */
  if (layer->filter.native_string) {
      static char *strFilterTemplate = "(%s)";
      strFilter1 = (char *)msSmallMalloc(strlen(strFilterTemplate) + strlen(layer->filter.native_string) + 1);
      sprintf(strFilter1, strFilterTemplate, layer->filter.native_string);
      strFilterLength1 = strlen(strFilter1) + 7;
  }

  /* Handle a native filter set as a PROCESSING option (#5001). */
  if (msLayerGetProcessingKey(layer, "NATIVE_FILTER") != NULL) {
      static char *strFilterTemplate = "(%s)";
      char *native_filter = msLayerGetProcessingKey(layer, "NATIVE_FILTER");
      strFilter2 = (char *)msSmallMalloc(strlen(strFilterTemplate) + strlen(native_filter) + 1);
      sprintf(strFilter2, strFilterTemplate, native_filter);
      strFilterLength2 = strlen(strFilter2) + 7;
  }

  buffer_len = strlen(layerinfo->geomcolumn) + strlen(f_table_name) + strlen(sqlExtentTemplate)
      + strFilterLength1 + strFilterLength2;
  strSQL = (char*)msSmallMalloc(buffer_len+1); /* add space for terminating NULL */
  snprintf(strSQL, buffer_len, sqlExtentTemplate, layerinfo->geomcolumn, f_table_name);  
  msFree(f_table_name);

  if (strFilter1) {
      strlcat(strSQL, " where ", buffer_len);
      strlcat(strSQL, strFilter1, buffer_len);
      msFree(strFilter1);
      if (strFilter2) {
          strlcat(strSQL, " and ", buffer_len);
          strlcat(strSQL, strFilter2, buffer_len);
          msFree(strFilter2);
      }
  }
  else if (strFilter2) {
      strlcat(strSQL, " where ", buffer_len);
      strlcat(strSQL, strFilter2, buffer_len);
      msFree(strFilter2);
  }

  if (layer->debug) {
    msDebug("msPostGISLayerGetExtent executing SQL: %s\n", strSQL);
  }

  /* executing the query */
  pgresult = runPQexecParamsWithBindSubstitution(layer, strSQL, 0);

  msFree(strSQL);

  if ( (!pgresult) || (PQresultStatus(pgresult) != PGRES_TUPLES_OK) ) {
    msDebug("Error executing SQL: (%s) in msPostGISLayerGetExtent()", PQerrorMessage(layerinfo->pgconn));
    msSetError(MS_MISCERR, "Error executing SQL. Check server logs.","msPostGISLayerGetExtent()");
    if (pgresult)
      PQclear(pgresult);

    return MS_FAILURE;
  }

  /* process results */
  if (PQntuples(pgresult) < 1) {
    msSetError(MS_MISCERR, "msPostGISLayerGetExtent: No results found.", 
        "msPostGISLayerGetExtent()");
    PQclear(pgresult);
    return MS_FAILURE;
  }
  
  if (PQgetisnull(pgresult, 0, 0)) {
    msSetError(MS_MISCERR, "msPostGISLayerGetExtent: Null result returned.", 
        "msPostGISLayerGetExtent()");
    PQclear(pgresult);
    return MS_FAILURE;
  }

  if (sscanf(PQgetvalue(pgresult, 0, 0), "BOX(%lf %lf,%lf %lf)", 
         &extent->minx, &extent->miny, &extent->maxx, &extent->maxy) != 4) {
    msSetError(MS_MISCERR, "Failed to process result data.", "msPostGISLayerGetExtent()");
    PQclear(pgresult);
    return MS_FAILURE;
  }

  /* cleanup */
  PQclear(pgresult);

  return MS_SUCCESS;
#else
  msSetError( MS_MISCERR, "PostGIS support is not available.", "msPostGISLayerGetExtent()");
  return MS_FAILURE;
#endif
}

/*
** msPostGISLayerGetNumFeatures()
**
** Registered vtable->LayerGetNumFeatures function. Query the database for
** the feature count of the requested layer.
*/
int msPostGISLayerGetNumFeatures(layerObj *layer)
{
#ifdef USE_POSTGIS
    msPostGISLayerInfo *layerinfo = NULL;
    char *strSQL = NULL;
    char *strFilter1 = 0, *strFilter2 = 0;
    char *f_table_name;
    static char *sqlNumFeaturesTemplate = "SELECT count(*) FROM %s";
    size_t buffer_len;
    size_t strFilterLength1 = 0, strFilterLength2 = 0;
    PGresult *pgresult = NULL;
    int result;
    char *tmp;

    if (layer->debug) {
        msDebug("msPostGISLayerGetNumFeatures called.\n");
    }

    assert(layer->layerinfo != NULL);

    layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

    if (msPostGISParseData(layer) != MS_SUCCESS) {
        return -1;
    }

    /* if we have !BOX! substitution then we use just the table name */
    if (strstr(layerinfo->fromsource, BOXTOKEN))
        f_table_name = msPostGISFindTableName(layerinfo->fromsource);
    else
        f_table_name = msStrdup(layerinfo->fromsource);

    if (!f_table_name) {
        msSetError(MS_MISCERR, "Failed to get table name.", "msPostGISLayerGetExtent()");
        return -1;
    }

    /* Handle a translated filter (RFC91). */
    if (layer->filter.native_string) {
        static char *strFilterTemplate = "(%s)";
        strFilter1 = (char *)msSmallMalloc(strlen(strFilterTemplate) + strlen(layer->filter.native_string) + 1);
        sprintf(strFilter1, strFilterTemplate, layer->filter.native_string);
        strFilterLength1 = strlen(strFilter1) + 7;
    }

    /* Handle a native filter set as a PROCESSING option (#5001). */
    if (msLayerGetProcessingKey(layer, "NATIVE_FILTER") != NULL) {
        static char *strFilterTemplate = "(%s)";
        char *native_filter = msLayerGetProcessingKey(layer, "NATIVE_FILTER");
        strFilter2 = (char *)msSmallMalloc(strlen(strFilterTemplate) + strlen(native_filter) + 1);
        sprintf(strFilter2, strFilterTemplate, native_filter);
        strFilterLength2 = strlen(strFilter2) + 7;
    }

    buffer_len = strlen(f_table_name) + strlen(sqlNumFeaturesTemplate)
        + strFilterLength1 + strFilterLength2;
    strSQL = (char*)msSmallMalloc(buffer_len + 1); /* add space for terminating NULL */
    snprintf(strSQL, buffer_len, sqlNumFeaturesTemplate, f_table_name);
    msFree(f_table_name);

    if (strFilter1) {
        strlcat(strSQL, " where ", buffer_len);
        strlcat(strSQL, strFilter1, buffer_len);
        msFree(strFilter1);
        if (strFilter2) {
            strlcat(strSQL, " and ", buffer_len);
            strlcat(strSQL, strFilter2, buffer_len);
            msFree(strFilter2);
        }
    }
    else if (strFilter2) {
        strlcat(strSQL, " where ", buffer_len);
        strlcat(strSQL, strFilter2, buffer_len);
        msFree(strFilter2);
    }

    if (layer->debug) {
        msDebug("msPostGISLayerGetNumFeatures executing SQL: %s\n", strSQL);
    }

    /* executing the query */

    pgresult = runPQexecParamsWithBindSubstitution(layer, strSQL, 0);

    msFree(strSQL);

    if ((!pgresult) || (PQresultStatus(pgresult) != PGRES_TUPLES_OK)) {
        msDebug("Error executing SQL: (%s) in msPostGISLayerGetNumFeatures()", PQerrorMessage(layerinfo->pgconn));
        msSetError(MS_MISCERR, "Error executing SQL. Check server logs.", "msPostGISLayerGetNumFeatures()");
        if (pgresult)
            PQclear(pgresult);

        return -1;
    }

    /* process results */
    if (PQntuples(pgresult) < 1) {
        msSetError(MS_MISCERR, "msPostGISLayerGetNumFeatures: No results found.",
            "msPostGISLayerGetNumFeatures()");
        PQclear(pgresult);
        return -1;
    }

    if (PQgetisnull(pgresult, 0, 0)) {
        msSetError(MS_MISCERR, "msPostGISLayerGetNumFeatures: Null result returned.",
            "msPostGISLayerGetNumFeatures()");
        PQclear(pgresult);
        return -1;
    }

    tmp = PQgetvalue(pgresult, 0, 0);
    if (tmp) {
        result = strtol(tmp, NULL, 10);
    }
    else {
        result = 0;
    }

    /* cleanup */
    PQclear(pgresult);

    return result;
#else
    msSetError(MS_MISCERR, "PostGIS support is not available.", "msPostGISLayerGetNumFeatures()");
    return -1;
#endif
}

/*
 * make sure that the timestring is complete and acceptable
 * to the date_trunc function :
 * - if the resolution is year (2004) or month (2004-01),
 * a complete string for time would be 2004-01-01
 * - if the resolluion is hour or minute (2004-01-01 15), a
 * complete time is 2004-01-01 15:00:00
 */
int postgresTimeStampForTimeString(const char *timestring, char *dest, size_t destsize)
{
  int nlength = strlen(timestring);
  int timeresolution = msTimeGetResolution(timestring);
  int bNoDate = (*timestring == 'T');
  if (timeresolution < 0)
    return MS_FALSE;
 
  switch(timeresolution) {
    case TIME_RESOLUTION_YEAR:
      if (timestring[nlength-1] != '-') {
        snprintf(dest, destsize,"date '%s-01-01'",timestring);
      } else {
        snprintf(dest, destsize,"date '%s01-01'",timestring);
      }
      break;
    case TIME_RESOLUTION_MONTH:
      if (timestring[nlength-1] != '-') {
        snprintf(dest, destsize,"date '%s-01'",timestring);
      } else {
        snprintf(dest, destsize,"date '%s01'",timestring);
      }
      break;
    case TIME_RESOLUTION_DAY:
      snprintf(dest, destsize,"date '%s'",timestring);
      break;
    case TIME_RESOLUTION_HOUR:
      if (timestring[nlength-1] != ':') {
        if(bNoDate)
          snprintf(dest, destsize,"time '%s:00:00'", timestring);
        else
          snprintf(dest, destsize,"timestamp '%s:00:00'", timestring);
      } else {
        if(bNoDate)
          snprintf(dest, destsize,"time '%s00:00'", timestring);
        else
          snprintf(dest, destsize,"timestamp '%s00:00'", timestring);
      }
      break;
    case TIME_RESOLUTION_MINUTE:
      if (timestring[nlength-1] != ':') {
        if(bNoDate)
          snprintf(dest, destsize,"time '%s:00'", timestring);
        else
          snprintf(dest, destsize,"timestamp '%s:00'", timestring);
      } else {
        if(bNoDate)
          snprintf(dest, destsize,"time '%s00'", timestring);
        else
          snprintf(dest, destsize,"timestamp '%s00'", timestring);
      }
      break;
    case TIME_RESOLUTION_SECOND:
      if(bNoDate)
         snprintf(dest, destsize,"time '%s'", timestring);
      else
         snprintf(dest, destsize,"timestamp '%s'", timestring);
      break;
    default:
      return MS_FAILURE;
  }
  return MS_SUCCESS;

}

/*
 * create a postgresql where clause for the given timestring, taking into account
 * the resolution (e.g. second, day, month...) of the given timestring
 * we apply the date_trunc function on the given timestring and not on the time
 * column in order for postgres to take advantage of an eventual index on the
 * time column
 *
 * the generated sql is
 *
 * (
 *    timecol >= date_trunc(timestring,resolution)
 *      and
 *    timecol < date_trunc(timestring,resolution) + interval '1 resolution'
 * )
 */
int createPostgresTimeCompareEquals(const char *timestring, char *dest, size_t destsize)
{
  int timeresolution = msTimeGetResolution(timestring);
  char timeStamp[100];
  char *interval;
  if (timeresolution < 0) return MS_FALSE;

  postgresTimeStampForTimeString(timestring,timeStamp,100);

  switch(timeresolution) {
    case TIME_RESOLUTION_YEAR:
      interval = "year";
      break;
    case TIME_RESOLUTION_MONTH:
      interval = "month";
      break;
    case TIME_RESOLUTION_DAY:
      interval = "day";
      break;
    case TIME_RESOLUTION_HOUR:
      interval = "hour";
      break;
    case TIME_RESOLUTION_MINUTE:
      interval = "minute";
      break;
    case TIME_RESOLUTION_SECOND:
      interval = "second";
      break;
    default:
      return MS_FAILURE;
  }
  snprintf(dest, destsize, " between date_trunc('%s',%s) and date_trunc('%s',%s) + interval '1 %s' - interval '1 second'", interval, timeStamp, interval, timeStamp, interval);
  return MS_SUCCESS;
}

int createPostgresTimeCompareGreaterThan(const char *timestring, char *dest, size_t destsize)
{
  int timeresolution = msTimeGetResolution(timestring);
  char timestamp[100];
  char *interval;
  if (timeresolution < 0) return MS_FALSE;
 
  postgresTimeStampForTimeString(timestring,timestamp,100);

  switch(timeresolution) {
  case TIME_RESOLUTION_YEAR:
    interval = "year";
    break;
  case TIME_RESOLUTION_MONTH:
    interval = "month";
    break;
  case TIME_RESOLUTION_DAY:
    interval = "day";
    break;
  case TIME_RESOLUTION_HOUR:
    interval = "hour";
    break;
  case TIME_RESOLUTION_MINUTE:
    interval = "minute";
    break;
  case TIME_RESOLUTION_SECOND:
    interval = "second";
    break;
  default:
    return MS_FAILURE;
  }

  snprintf(dest, destsize,"date_trunc('%s',%s)", interval, timestamp);
  return MS_SUCCESS;
}

/*
 * create a postgresql where clause for the range given by the two input timestring,
 * taking into account the resolution (e.g. second, day, month...) of each of the
 * given timestrings (both timestrings can have different resolutions, although I don't
 * know if that's a valid TIME range
 * we apply the date_trunc function on the given timestrings and not on the time
 * column in order for postgres to take advantage of an eventual index on the
 * time column
 *
 * the generated sql is
 *
 * (
 *    timecol >= date_trunc(mintimestring,minresolution)
 *      and
 *    timecol < date_trunc(maxtimestring,maxresolution) + interval '1 maxresolution'
 * )
 */
int createPostgresTimeCompareLessThan(const char *timestring, char *dest, size_t destsize)
{
  int timeresolution = msTimeGetResolution(timestring);
  char timestamp[100];
  char *interval;
  if (timeresolution < 0)return MS_FALSE;

  postgresTimeStampForTimeString(timestring,timestamp,100);

  switch(timeresolution) {
    case TIME_RESOLUTION_YEAR:
      interval = "year";
      break;
    case TIME_RESOLUTION_MONTH:
      interval = "month";
      break;
    case TIME_RESOLUTION_DAY:
      interval = "day";
      break;
    case TIME_RESOLUTION_HOUR:
      interval = "hour";
      break;
    case TIME_RESOLUTION_MINUTE:
      interval = "minute";
      break;
    case TIME_RESOLUTION_SECOND:
      interval = "second";
      break;
    default:
      return MS_FAILURE;
  }
  snprintf(dest, destsize,"(date_trunc('%s',%s) + interval '1 %s' - interval '1 second')", interval, timestamp, interval);
  return MS_SUCCESS;
}

char *msPostGISEscapeSQLParam(layerObj *layer, const char *pszString)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo *layerinfo = NULL;
  int nError;
  size_t nSrcLen;
  char* pszEscapedStr =NULL;

  if (layer && pszString) {
    if(!msPostGISLayerIsOpen(layer))
      msPostGISLayerOpen(layer);

    assert(layer->layerinfo != NULL);

    layerinfo = (msPostGISLayerInfo *) layer->layerinfo;
    nSrcLen = strlen(pszString);
    pszEscapedStr = (char*) msSmallMalloc( 2 * nSrcLen + 1);
    PQescapeStringConn (layerinfo->pgconn, pszEscapedStr, pszString, nSrcLen, &nError);
    if (nError != 0) {
      free(pszEscapedStr);
      pszEscapedStr = NULL;
    }
  }
  return pszEscapedStr;
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISEscapeSQLParam()");
  return NULL;
#endif
}

void msPostGISEnablePaging(layerObj *layer, int value)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo *layerinfo = NULL;

  if (layer->debug) {
    msDebug("msPostGISEnablePaging called.\n");
  }

  if(!msPostGISLayerIsOpen(layer))
    msPostGISLayerOpen(layer);

  assert( layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo *)layer->layerinfo;
  layerinfo->paging = value;

#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISEnablePaging()");
#endif
  return;
}

int msPostGISGetPaging(layerObj *layer)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo *layerinfo = NULL;

  if (layer->debug) {
    msDebug("msPostGISGetPaging called.\n");
  }

  if(!msPostGISLayerIsOpen(layer))
    return MS_TRUE;

  assert( layer->layerinfo != NULL);

  layerinfo = (msPostGISLayerInfo *)layer->layerinfo;
  return layerinfo->paging;
#else
  msSetError( MS_MISCERR,
              "PostGIS support is not available.",
              "msPostGISEnablePaging()");
  return MS_FAILURE;
#endif
}

/*
** Look ahead to find the next node of a specific type.
*/
tokenListNodeObjPtr findNextTokenByType(tokenListNodeObjPtr node, int type) {
  while (node != NULL) {
    if(node->token == type) return node;
    node = node->next; /* keep looking */
  }

  return NULL; /* not found */
}

/*
** msPostGISLayerTranslateFilter()
**
** Registered vtable->LayerTranslateFilter function.
*/
int msPostGISLayerTranslateFilter(layerObj *layer, expressionObj *filter, char *filteritem)
{
#ifdef USE_POSTGIS
  tokenListNodeObjPtr node = NULL;

  char *snippet = NULL;
  char *native_string = NULL;
  char *strtmpl = NULL;
  char *stresc = NULL;

  int comparisonToken = -1;
  int bindingToken = -1;

  msPostGISLayerInfo *layerinfo = layer->layerinfo;

  if(!filter->string) return MS_SUCCESS; /* not an error, just nothing to do */

  // fprintf(stderr, "input: %s, %s, %d\n", filter->string, filteritem, filter->type);

  /*
  ** FILTERs use MapServer syntax *only* (#5001).
  */
  if(filter->type == MS_STRING && filter->string && filteritem) { /* item/value pair - add escaping */

    stresc = msLayerEscapePropertyName(layer, filteritem);
    if(filter->flags & MS_EXP_INSENSITIVE) {
      native_string = msStringConcatenate(native_string, "upper(");
      native_string = msStringConcatenate(native_string, stresc);
      native_string = msStringConcatenate(native_string, "::text) = upper(");
    } else {
      native_string = msStringConcatenate(native_string, stresc);
      native_string = msStringConcatenate(native_string, "::text = ");
    }
    msFree(stresc);

    strtmpl = "'%s'";  /* don't have a type for the righthand literal so assume it's a string and we quote */
    stresc = msPostGISEscapeSQLParam(layer, filter->string);    
    snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(stresc));
    sprintf(snippet, strtmpl, stresc);
    native_string = msStringConcatenate(native_string, snippet);
    msFree(snippet);
    msFree(stresc);

    if(filter->flags & MS_EXP_INSENSITIVE) native_string = msStringConcatenate(native_string, ")");
  } else if(filter->type == MS_REGEX && filter->string && filteritem) { /* item/regex pair  - add escaping */

    stresc = msLayerEscapePropertyName(layer, filteritem);
    native_string = msStringConcatenate(native_string, stresc);
    if(filter->flags & MS_EXP_INSENSITIVE) {
      native_string = msStringConcatenate(native_string, "::text ~* ");
    } else {
      native_string = msStringConcatenate(native_string, "::text ~ ");
    }
    msFree(stresc);

    strtmpl = "'%s'";
    stresc = msPostGISEscapeSQLParam(layer, filter->string);
    snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(stresc));
    sprintf(snippet, strtmpl, stresc);
    native_string = msStringConcatenate(native_string, snippet);
    msFree(snippet);
    msFree(stresc);
  } else if(filter->type == MS_EXPRESSION) {
    int ieq_expected = MS_FALSE;

    if(msPostGISParseData(layer) != MS_SUCCESS) return MS_FAILURE;

    if(layer->debug >= 2) msDebug("msPostGISLayerTranslateFilter. String: %s.\n", filter->string);
    if(!filter->tokens) return MS_SUCCESS;
    if(layer->debug >= 2) msDebug("msPostGISLayerTranslateFilter. There are tokens to process... \n");

    node = filter->tokens;
    while (node != NULL) {

      /*
      ** Do any token caching/tracking here, easier to have it in one place.
      */
      if(node->token == MS_TOKEN_BINDING_TIME) {
        bindingToken = node->token;
      } else if(node->token == MS_TOKEN_COMPARISON_EQ ||
         node->token == MS_TOKEN_COMPARISON_IEQ ||
         node->token == MS_TOKEN_COMPARISON_NE ||
         node->token == MS_TOKEN_COMPARISON_GT || node->token == MS_TOKEN_COMPARISON_GE ||
         node->token == MS_TOKEN_COMPARISON_LT || node->token == MS_TOKEN_COMPARISON_LE ||
         node->token == MS_TOKEN_COMPARISON_IN) {
        comparisonToken = node->token;
      }

      switch(node->token) {

        /* literal tokens */
        case MS_TOKEN_LITERAL_BOOLEAN:
          if(node->tokenval.dblval == MS_TRUE)
            native_string = msStringConcatenate(native_string, "TRUE");
          else
            native_string = msStringConcatenate(native_string, "FALSE");
          break;
        case MS_TOKEN_LITERAL_NUMBER:
          snippet = (char *) msSmallMalloc(32);
          if( node->tokenval.dblval >= INT_MIN &&
              node->tokenval.dblval <= INT_MAX &&
              node->tokenval.dblval == (int)node->tokenval.dblval )
              sprintf(snippet, "%d", (int)node->tokenval.dblval);
          else
              sprintf(snippet, "%.18g", node->tokenval.dblval);
          native_string = msStringConcatenate(native_string, snippet);
          msFree(snippet);
          break;
        case MS_TOKEN_LITERAL_STRING:

          if(comparisonToken == MS_TOKEN_COMPARISON_IN) { /* issue 5490 */
            char **strings=NULL;
            int i, nstrings=0;

	    strings = msStringSplit(node->tokenval.strval, ',', &nstrings);
            if(nstrings > 0) {
	      native_string = msStringConcatenate(native_string, "(");
              for(i=0; i<nstrings; i++) {
                if(i != 0) native_string = msStringConcatenate(native_string, ",");
                strtmpl = "'%s'";
		stresc = msPostGISEscapeSQLParam(layer, strings[i]);
		snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(stresc));
		sprintf(snippet, strtmpl, stresc);
		native_string = msStringConcatenate(native_string, snippet);
		msFree(snippet);
		msFree(stresc);
              }
              native_string = msStringConcatenate(native_string, ")");
            }

            msFreeCharArray(strings, nstrings);
          } else {
            if(comparisonToken == MS_TOKEN_COMPARISON_IEQ)
                strtmpl = "lower('%s')";
            else
                strtmpl = "'%s'";
            stresc = msPostGISEscapeSQLParam(layer, node->tokenval.strval);
            snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(stresc));
            sprintf(snippet, strtmpl, stresc);
            native_string = msStringConcatenate(native_string, snippet);
            msFree(snippet);
            msFree(stresc);
          }

          break;
        case MS_TOKEN_LITERAL_TIME: {
	  snippet = (char *) msSmallMalloc(512);

          if(comparisonToken == MS_TOKEN_COMPARISON_EQ) { // TODO: support !=
            createPostgresTimeCompareEquals(node->tokensrc, snippet, 512);
          } else if(comparisonToken == MS_TOKEN_COMPARISON_GT || comparisonToken == MS_TOKEN_COMPARISON_GE) {
            createPostgresTimeCompareGreaterThan(node->tokensrc, snippet, 512);
          } else if(comparisonToken == MS_TOKEN_COMPARISON_LT || comparisonToken == MS_TOKEN_COMPARISON_LE) {
            createPostgresTimeCompareLessThan(node->tokensrc, snippet, 512);
          } else {
            goto cleanup;
          }

          comparisonToken = -1; bindingToken = -1; /* reset */
          native_string = msStringConcatenate(native_string, snippet);
          msFree(snippet);
          break;
  	}
        case MS_TOKEN_LITERAL_SHAPE:
        {
          char* wkt = msShapeToWKT(node->tokenval.shpval);
          native_string = msStringConcatenate(native_string, "ST_GeomFromText('");
          native_string = msStringConcatenate(native_string, wkt);
          msFree(wkt);
          native_string = msStringConcatenate(native_string, "'");
          if(layerinfo->srid && strcmp(layerinfo->srid, "") != 0) {
            native_string = msStringConcatenate(native_string, ",");
            native_string = msStringConcatenate(native_string, layerinfo->srid);
          }
          native_string = msStringConcatenate(native_string, ")");
          break;
        }

	/* data binding tokens */
        case MS_TOKEN_BINDING_TIME:
        case MS_TOKEN_BINDING_DOUBLE:
        case MS_TOKEN_BINDING_INTEGER:
        case MS_TOKEN_BINDING_STRING:
          if (node->token == MS_TOKEN_BINDING_STRING && node->next->token == MS_TOKEN_COMPARISON_IEQ ) {
            strtmpl = "lower(%s::text)";
            ieq_expected = MS_TRUE;
          }
          else if(node->token == MS_TOKEN_BINDING_STRING || node->next->token == MS_TOKEN_COMPARISON_RE || node->next->token == MS_TOKEN_COMPARISON_IRE)
            strtmpl = "%s::text"; /* explicit cast necessary for certain operators */
          else
            strtmpl = "%s";
          stresc = msLayerEscapePropertyName(layer, node->tokenval.bindval.item);
          snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(stresc));
          sprintf(snippet, strtmpl, stresc);
          native_string = msStringConcatenate(native_string, snippet);
          msFree(snippet);
          msFree(stresc);       
          break;
        case MS_TOKEN_BINDING_SHAPE:
          native_string = msStringConcatenate(native_string, layerinfo->geomcolumn);
          break;
        case MS_TOKEN_BINDING_MAP_CELLSIZE:
          strtmpl = "%lf";
          snippet = (char *) msSmallMalloc(strlen(strtmpl) + 16);
          sprintf(snippet, strtmpl, layer->map->cellsize);
          native_string = msStringConcatenate(native_string, snippet);
          msFree(snippet);
	  break;

	/* spatial comparison tokens */
        case MS_TOKEN_COMPARISON_INTERSECTS:
        case MS_TOKEN_COMPARISON_DISJOINT:
        case MS_TOKEN_COMPARISON_TOUCHES:
        case MS_TOKEN_COMPARISON_OVERLAPS:
        case MS_TOKEN_COMPARISON_CROSSES:
        case MS_TOKEN_COMPARISON_WITHIN:
        case MS_TOKEN_COMPARISON_CONTAINS:
        case MS_TOKEN_COMPARISON_EQUALS:
        case MS_TOKEN_COMPARISON_DWITHIN:
          if(node->next->token != '(') goto cleanup;
          native_string = msStringConcatenate(native_string, "st_");
          native_string = msStringConcatenate(native_string, msExpressionTokenToString(node->token));
          break;

	/* functions */
        case MS_TOKEN_FUNCTION_LENGTH:
        case MS_TOKEN_FUNCTION_AREA:
        case MS_TOKEN_FUNCTION_BUFFER:
        case MS_TOKEN_FUNCTION_DIFFERENCE:
          native_string = msStringConcatenate(native_string, "st_");
          native_string = msStringConcatenate(native_string, msExpressionTokenToString(node->token));
          break;

	case MS_TOKEN_COMPARISON_IEQ:
            if( ieq_expected )
            {
                native_string = msStringConcatenate(native_string, "=");
                ieq_expected = MS_FALSE;
            }
            else
            {
                goto cleanup;
            }
            break;

	/* unsupported tokens */
        case MS_TOKEN_COMPARISON_BEYOND:
	case MS_TOKEN_FUNCTION_TOSTRING:
	case MS_TOKEN_FUNCTION_ROUND:
	case MS_TOKEN_FUNCTION_SIMPLIFY:
        case MS_TOKEN_FUNCTION_SIMPLIFYPT:        
        case MS_TOKEN_FUNCTION_GENERALIZE:
          goto cleanup;
          break;

        default:
          /* by default accept the general token to string conversion */

          if(node->token == MS_TOKEN_COMPARISON_EQ && node->next != NULL && node->next->token == MS_TOKEN_LITERAL_TIME) break; /* skip, handled with the next token */
          if(bindingToken == MS_TOKEN_BINDING_TIME && (node->token == MS_TOKEN_COMPARISON_EQ || node->token == MS_TOKEN_COMPARISON_NE)) break; /* skip, handled elsewhere */
          if(node->token == MS_TOKEN_COMPARISON_EQ && node->next != NULL && node->next->token == MS_TOKEN_LITERAL_STRING &&
             strcmp(node->next->tokenval.strval, "_MAPSERVER_NULL_") == 0 )
          {
              native_string = msStringConcatenate(native_string, " IS NULL");
              node = node->next;
              break;
          }

          native_string = msStringConcatenate(native_string, msExpressionTokenToString(node->token));
          break;
        }

      node = node->next;
    }
  }

  filter->native_string = msStrdup(native_string);    
  msFree(native_string);

  // fprintf(stderr, "output: %s\n", filter->native_string); 

  return MS_SUCCESS;

cleanup:
  msSetError(MS_MISCERR, "Translation to native SQL failed.", "msPostGISLayerTranslateFilter()");
  msFree(native_string);
  return MS_FAILURE;
#else
  msSetError(MS_MISCERR, "PostGIS support is not available.", "msPostGISLayerTranslateFilter()");
  return MS_FAILURE;
#endif
}

int msPostGISLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerTranslateFilter = msPostGISLayerTranslateFilter;

  layer->vtable->LayerInitItemInfo = msPostGISLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msPostGISLayerFreeItemInfo;
  layer->vtable->LayerOpen = msPostGISLayerOpen;
  layer->vtable->LayerIsOpen = msPostGISLayerIsOpen;
  layer->vtable->LayerWhichShapes = msPostGISLayerWhichShapes;
  layer->vtable->LayerNextShape = msPostGISLayerNextShape;
  layer->vtable->LayerGetShape = msPostGISLayerGetShape;
  layer->vtable->LayerGetShapeCount = msPostGISLayerGetShapeCount;
  layer->vtable->LayerClose = msPostGISLayerClose;
  layer->vtable->LayerGetItems = msPostGISLayerGetItems;
  layer->vtable->LayerGetExtent = msPostGISLayerGetExtent;
  layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;
  /* layer->vtable->LayerGetAutoStyle, not supported for this layer */
  /* layer->vtable->LayerCloseConnection = msPostGISLayerClose; */
  // layer->vtable->LayerSetTimeFilter = msPostGISLayerSetTimeFilter;
  layer->vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  layer->vtable->LayerGetNumFeatures = msPostGISLayerGetNumFeatures;

  /* layer->vtable->LayerGetAutoProjection, use defaut*/

  layer->vtable->LayerEscapeSQLParam = msPostGISEscapeSQLParam;
  layer->vtable->LayerEnablePaging = msPostGISEnablePaging;
  layer->vtable->LayerGetPaging = msPostGISGetPaging;

  return MS_SUCCESS;
}

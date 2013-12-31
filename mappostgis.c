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

#define FP_EPSILON 1e-12
#define FP_EQ(a, b) (fabs((a)-(b)) < FP_EPSILON)
#define FP_LEFT -1
#define FP_RIGHT 1
#define FP_COLINEAR 0

#define SEGMENT_ANGLE 10.0
#define SEGMENT_MINPOINTS 10

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
wkbTypeMap(wkbObj *w, int type)
{
  if ( type < WKB_TYPE_COUNT )
    return w->typemap[type];
  else
    return 0;
}

/*
** Read the WKB type number from a wkbObj without
** advancing the read pointer.
*/
static int
wkbType(wkbObj *w)
{
  int t;
  memcpy(&t, (w->ptr + 1), sizeof(int));
  return wkbTypeMap(w,t);
}

/*
** Read the type number of the first element of a
** collection without advancing the read pointer.
*/
static int
wkbCollectionSubType(wkbObj *w)
{
  int t;
  memcpy(&t, (w->ptr + 1 + 4 + 4 + 1), sizeof(int));
  return wkbTypeMap(w,t);
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
** Read one double from the WKB and advance the read pointer.
** We assume the endianess of the WKB is the same as this machine.
*/
static inline double
wkbReadDouble(wkbObj *w)
{
  double d;
  memcpy(&d, w->ptr, sizeof(double));
  w->ptr += sizeof(double);
  return d;
}

/*
** Read one pointObj (two doubles) from the WKB and advance the read pointer.
** We assume the endianess of the WKB is the same as this machine.
*/
static inline void
wkbReadPointP(wkbObj *w, pointObj *p)
{
  memcpy(&(p->x), w->ptr, sizeof(double));
  w->ptr += sizeof(double);
  memcpy(&(p->y), w->ptr, sizeof(double));
  w->ptr += sizeof(double);
}

/*
** Read one pointObj (two doubles) from the WKB and advance the read pointer.
** We assume the endianess of the WKB is the same as this machine.
*/
static inline pointObj
wkbReadPoint(wkbObj *w)
{
  pointObj p;
  wkbReadPointP(w, &p);
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
wkbReadLine(wkbObj *w, lineObj *line)
{
  int i;
  pointObj p;
  int npoints = wkbReadInt(w);

  line->numpoints = npoints;
  line->point = msSmallMalloc(npoints * sizeof(pointObj));
  for ( i = 0; i < npoints; i++ ) {
    wkbReadPointP(w, &p);
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
  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w));
  switch(type) {
    case WKB_POINT:
      w->ptr += 2 * sizeof(double);
      break;
    case WKB_CIRCULARSTRING:
    case WKB_LINESTRING:
      npoints = wkbReadInt(w);
      w->ptr += npoints * 2 * sizeof(double);
      break;
    case WKB_POLYGON:
      nrings = wkbReadInt(w);
      for ( i = 0; i < nrings; i++ ) {
        npoints = wkbReadInt(w);
        w->ptr += npoints * 2 * sizeof(double);
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

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w));

  if( type != WKB_POINT ) return MS_FAILURE;

  if( ! (shape->type == MS_SHAPE_POINT) ) return MS_FAILURE;
  line.numpoints = 1;
  line.point = msSmallMalloc(sizeof(pointObj));
  line.point[0] = wkbReadPoint(w);
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

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w));

  if( type != WKB_LINESTRING ) return MS_FAILURE;

  wkbReadLine(w,&line);
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

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w));

  if( type != WKB_POLYGON ) return MS_FAILURE;

  /* How many rings? */
  nrings = wkbReadInt(w);

  /* Add each ring to the shape */
  for( i = 0; i < nrings; i++ ) {
    wkbReadLine(w,&line);
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

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w));
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
  lineObj line = {0, NULL};

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w));

  if( type != WKB_CIRCULARSTRING ) return MS_FAILURE;

  /* Stroke the string into a point array */
  if ( arcStrokeCircularString(w, SEGMENT_ANGLE, &line) == MS_FAILURE ) {
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
  lineObj *line;
  shapeObj shapebuf;

  /*endian = */wkbReadChar(w);
  type = wkbTypeMap(w,wkbReadInt(w));

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

  /* Allocate space for the new line */
  line = msSmallMalloc(sizeof(lineObj));
  line->numpoints = npoints;
  line->point = msSmallMalloc(sizeof(pointObj) * npoints);

  /* Copy in the points */
  npoints = 0;
  for ( i = 0; i < shapebuf.numlines; i++ ) {
    for ( j = 0; j < shapebuf.line[i].numpoints; j++ ) {
      /* Don't add a start point that duplicates an endpoint */
      if( j == 0 && i > 0 &&
          memcmp(&(line->point[npoints - 1]),&(shapebuf.line[i].point[j]),sizeof(pointObj)) == 0 ) {
        continue;
      }
      line->point[npoints++] = shapebuf.line[i].point[j];
    }
  }
  line->numpoints = npoints;

  /* Clean up */
  msFreeShape(&shapebuf);

  /* Fill in the lineObj */
  msAddLineDirectly(shape, line);

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

  /*endian = */wkbReadChar(w);
  /*type = */wkbTypeMap(w,wkbReadInt(w));
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
  if ( failures == ncomponents )
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
  int wkbtype = wkbType(w); /* Peak at the type number */

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
** Calculate determinant of a 3x3 matrix. Handy for
** the circle center calculation.
*/
static inline double
arcDeterminant3x3(double *m)
{
  /* This had better be a 3x3 matrix or we'll fall to bits */
  return m[0] * ( m[4] * m[8] - m[7] * m[5] ) -
         m[3] * ( m[1] * m[8] - m[7] * m[2] ) +
         m[6] * ( m[1] * m[5] - m[4] * m[2] );
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
  pointObj c;
  double r;

  /* Components of the matrices. */
  double x1sq = p1->x * p1->x;
  double x2sq = p2->x * p2->x;
  double x3sq = p3->x * p3->x;
  double y1sq = p1->y * p1->y;
  double y2sq = p2->y * p2->y;
  double y3sq = p3->y * p3->y;
  double matrix_num_x[9];
  double matrix_num_y[9];
  double matrix_denom[9];

  /* Intialize matrix_num_x */
  matrix_num_x[0] = x1sq+y1sq;
  matrix_num_x[1] = p1->y;
  matrix_num_x[2] = 1.0;
  matrix_num_x[3] = x2sq+y2sq;
  matrix_num_x[4] = p2->y;
  matrix_num_x[5] = 1.0;
  matrix_num_x[6] = x3sq+y3sq;
  matrix_num_x[7] = p3->y;
  matrix_num_x[8] = 1.0;

  /* Intialize matrix_num_y */
  matrix_num_y[0] = p1->x;
  matrix_num_y[1] = x1sq+y1sq;
  matrix_num_y[2] = 1.0;
  matrix_num_y[3] = p2->x;
  matrix_num_y[4] = x2sq+y2sq;
  matrix_num_y[5] = 1.0;
  matrix_num_y[6] = p3->x;
  matrix_num_y[7] = x3sq+y3sq;
  matrix_num_y[8] = 1.0;

  /* Intialize matrix_denom */
  matrix_denom[0] = p1->x;
  matrix_denom[1] = p1->y;
  matrix_denom[2] = 1.0;
  matrix_denom[3] = p2->x;
  matrix_denom[4] = p2->y;
  matrix_denom[5] = 1.0;
  matrix_denom[6] = p3->x;
  matrix_denom[7] = p3->y;
  matrix_denom[8] = 1.0;

  /* Circle is closed, so p2 must be opposite p1 & p3. */
  if ( FP_EQ(p1->x,p3->x) && FP_EQ(p1->y,p3->y) ) {
    c.x = (p1->x + p2->x) / 2.0;
    c.y = (p1->y + p2->y) / 2.0;
    r = sqrt( (p1->x - p2->x) * (p1->x - p2->x) + (p1->y - p2->y) * (p1->y - p2->y) ) / 2.0;
  }
  /* There is no circle here, the points are actually co-linear */
  else if ( arcSegmentSide(p1, p3, p2) == FP_COLINEAR ) {
    return MS_FAILURE;
  }
  /* Calculate the center and radius. */
  else {
    double denom = 2.0 * arcDeterminant3x3(matrix_denom);
    /* Center components */
    c.x = arcDeterminant3x3(matrix_num_x) / denom;
    c.y = arcDeterminant3x3(matrix_num_y) / denom;

    /* Radius */
    r = sqrt((p1->x-c.x) * (p1->x-c.x) + (p1->y-c.y) * (p1->y-c.y));
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
arcStrokeCircularString(wkbObj *w, double segment_angle, lineObj *line)
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

  wkbReadPointP(w,&p3);

  /* Fill out the point array with stroked arcs */
  while( edge < nedges ) {
    p1 = p3;
    wkbReadPointP(w,&p2);
    wkbReadPointP(w,&p3);
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

  /* What kind of geometry is this? */
  wkbtype = wkbType(w);

  /* Generic collection, we need to look a little deeper. */
  if ( wkbtype == WKB_GEOMETRYCOLLECTION )
    wkbtype = wkbCollectionSubType(w);

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
    msSetError(MS_QUERYERR, "Error executing SQL: %s", "msPostGISRetrieveVersion()", sql);
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
  while( ptr != '\0' && j < 3 ) {
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
  char *pos_sep;
  char *schema = NULL;
  char *table = NULL;

  if (layer->debug) {
    msDebug("msPostGISRetrievePK called.\n");
  }

  layerinfo = (msPostGISLayerInfo *) layer->layerinfo;

  /* Attempt to separate fromsource into schema.table */
  pos_sep = strstr(layerinfo->fromsource, ".");
  if (pos_sep) {
    length = strlen(layerinfo->fromsource) - strlen(pos_sep) + 1;
    schema = (char*)msSmallMalloc(length);
    strlcpy(schema, layerinfo->fromsource, length);

    length = strlen(pos_sep);
    table = (char*)msSmallMalloc(length);
    strlcpy(table, pos_sep + 1, length);

    if (layer->debug) {
      msDebug("msPostGISRetrievePK(): Found schema %s, table %s.\n", schema, table);
    }
  }

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
    sql = msSmallMalloc(strlen(layerinfo->fromsource) + strlen(v72sql));
    sprintf(sql, v72sql, layerinfo->fromsource);
  } else {
    /*
    ** PostgreSQL v7.3 and later treat primary keys as constraints.
    ** We only support single column primary keys, so multicolumn
    ** pks are explicitly excluded from the query.
    */
    if (schema && table) {
      static char *v73sql = "select attname from pg_attribute, pg_constraint, pg_class, pg_namespace where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = '%s' and pg_class.relnamespace = pg_namespace.oid and pg_namespace.nspname = '%s' and pg_constraint.conkey[2] is null";
      sql = msSmallMalloc(strlen(schema) + strlen(table) + strlen(v73sql));
      sprintf(sql, v73sql, table, schema);
      free(table);
      free(schema);
    } else {
      static char *v73sql = "select attname from pg_attribute, pg_constraint, pg_class where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = '%s' and pg_table_is_visible(pg_class.oid) and pg_constraint.conkey[2] is null";
      sql = msSmallMalloc(strlen(layerinfo->fromsource) + strlen(v73sql));
      sprintf(sql, v73sql, layerinfo->fromsource);
    }
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
    msSetError(MS_QUERYERR, tmp2, "msPostGISRetrievePK()");
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
  int slength;
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
  data = layer->data;

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
  ** Look for the optional ' using unique ID' string first.
  */
  pos_uid = strcasestr(data, " using unique ");
  if (pos_uid) {
    /* Find the end of this case 'using unique ftab_id using srid=33' */
    tmp = strstr(pos_uid + 14, " ");
    /* Find the end of this case 'using srid=33 using unique ftab_id' */
    if (!tmp) {
      tmp = pos_uid + strlen(pos_uid);
    }
    layerinfo->uid = (char*) msSmallMalloc((tmp - (pos_uid + 14)) + 1);
    strlcpy(layerinfo->uid, pos_uid + 14, tmp - (pos_uid + 14)+1);
    msStringTrim(layerinfo->uid);
  }

  /*
  ** Look for the optional ' using srid=333 ' string next.
  */
  pos_srid = strcasestr(data, " using srid=");
  if (!pos_srid) {
    layerinfo->srid = (char*) msSmallMalloc(1);
    (layerinfo->srid)[0] = '\0'; /* no SRID, so return just null terminator*/
  } else {
    slength = strspn(pos_srid + 12, "-0123456789");
    if (!slength) {
      msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable. You specified 'USING SRID' but didnt have any numbers! %s", "msPostGISParseData()", data);
      return MS_FAILURE;
    } else {
      layerinfo->srid = (char*) msSmallMalloc(slength + 1);
      strlcpy(layerinfo->srid, pos_srid + 12, slength+1);
      msStringTrim(layerinfo->srid);
    }
  }

  /*
  ** This is a little hack so the rest of the code works.
  ** pos_opt should point to the start of the optional blocks.
  **
  ** If they are both set, return the smaller one.
  */
  if (pos_srid && pos_uid) {
    pos_opt = (pos_srid > pos_uid) ? pos_uid : pos_srid;
  }
  /* If one or none is set, return the larger one. */
  else {
    pos_opt = (pos_srid > pos_uid) ? pos_srid : pos_uid;
  }
  /* No pos_opt? Move it to the end of the string. */
  if (!pos_opt) {
    pos_opt = data + strlen(data);
  }

  /*
  ** Scan for the 'geometry from table' or 'geometry from () as foo' clause.
  */

  /* Find the first non-white character to start from */
  pos_geom = data;
  while( *pos_geom == ' ' || *pos_geom == '\t' || *pos_geom == '\n' || *pos_geom == '\r' )
    pos_geom++;

  /* Find the end of the geom column name */
  pos_scn = strcasestr(data, " from ");
  if (!pos_scn) {
    msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable. Must contain 'geometry from table' or 'geometry from (subselect) as foo'. %s", "msPostGISParseData()", data);
    return MS_FAILURE;
  }

  /* Copy the geometry column name */
  layerinfo->geomcolumn = (char*) msSmallMalloc((pos_scn - pos_geom) + 1);
  strlcpy(layerinfo->geomcolumn, pos_geom, pos_scn - pos_geom+1);
  msStringTrim(layerinfo->geomcolumn);

  /* Copy the table name or sub-select clause */
  layerinfo->fromsource = (char*) msSmallMalloc((pos_opt - (pos_scn + 6)) + 1);
  strlcpy(layerinfo->fromsource, pos_scn + 6, pos_opt - (pos_scn + 6)+1);
  msStringTrim(layerinfo->fromsource);

  /* Something is wrong, our goemetry column and table references are not there. */
  if (strlen(layerinfo->fromsource) < 1 || strlen(layerinfo->geomcolumn) < 1) {
    msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable.  Must contain 'geometry from table' or 'geometry from (subselect) as foo'. %s", "msPostGISParseData()", data);
    return MS_FAILURE;
  }

  /*
  ** We didn't find a ' using unique ' in the DATA string so try and find a
  ** primary key on the table.
  */
  if ( ! (layerinfo->uid) ) {
    if ( strstr(layerinfo->fromsource, " ") ) {
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
#if TRANSFER_ENCODING == 64
    static char *strGeomTemplate = "encode(ST_AsBinary(ST_Force_2D(\"%s\"),'%s'),'base64') as geom,\"%s\"";
#else
    static char *strGeomTemplate = "encode(ST_AsBinary(ST_Force_2D(\"%s\"),'%s'),'hex') as geom,\"%s\"";
#endif
    strGeom = (char*)msSmallMalloc(strlen(strGeomTemplate) + strlen(strEndian) + strlen(layerinfo->geomcolumn) + strlen(layerinfo->uid));
    sprintf(strGeom, strGeomTemplate, layerinfo->geomcolumn, strEndian, layerinfo->uid);
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
    }
    strItems = (char*)msSmallMalloc(length);
    strItems[0] = '\0';
    for ( t = 0; t < layer->numitems; t++ ) {
      strlcat(strItems, "\"", length);
      strlcat(strItems, layer->items[t], length);
      strlcat(strItems, "\",", length);
    }
    strlcat(strItems, strGeom, length);
  }

  free(strGeom);
  return strItems;
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
    char *f_table_name;
    char *strSRIDTemplate = "find_srid('','%s','%s')";
    char *pos = strstr(layerinfo->fromsource, " ");
    if( layer->debug > 1 ) {
      msDebug("msPostGISBuildSQLSRID: Building find_srid line.\n", strSRID);
    }

    if ( ! pos ) {
      /* target table is one word */
      f_table_name = msStrdup(layerinfo->fromsource);
      if( layer->debug > 1 ) {
        msDebug("msPostGISBuildSQLSRID: Found table (%s)\n", f_table_name);
      }
    } else {
      /* target table is hiding in sub-select clause */
      pos = strcasestr(layerinfo->fromsource, " from ");
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
      } else {
        /* should not happen */
        return NULL;
      }
    }
    strSRID = msSmallMalloc(strlen(strSRIDTemplate) + strlen(f_table_name) + strlen(layerinfo->geomcolumn));
    sprintf(strSRID, strSRIDTemplate, f_table_name, layerinfo->geomcolumn);
    if ( f_table_name ) free(f_table_name);
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
char *msPostGISBuildSQLWhere(layerObj *layer, rectObj *rect, long *uid)
{
  char *strRect = 0;
  char *strFilter = 0;
  char *strUid = 0;
  char *strWhere = 0;
  char *strLimit = 0;
  char *strOffset = 0;
  size_t strRectLength = 0;
  size_t strFilterLength = 0;
  size_t strUidLength = 0;
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
    static char *strRectTemplate = "%s && %s";

    /* We see to set the SRID on the box, but to what SRID? */
    strSRID = msPostGISBuildSQLSRID(layer);
    if ( ! strSRID ) {
      return NULL;
    }

    strBox = msPostGISBuildSQLBox(layer, rect, strSRID);
    if ( strBox ) {
      strBoxLength = strlen(strBox);
    } else {
      msSetError(MS_MISCERR, "Unable to build box SQL.", "msPostGISBuildSQLWhere()");
      return NULL;
    }

    strRect = (char*)msSmallMalloc(strlen(strRectTemplate) + strBoxLength + strlen(layerinfo->geomcolumn));
    sprintf(strRect, strRectTemplate, layerinfo->geomcolumn, strBox);
    strRectLength = strlen(strRect);
    if (strBox) free(strBox);
    if (strSRID) free(strSRID);
  }

  /* Populate strFilter, if necessary. */
  if ( layer->filter.string ) {
    static char *strFilterTemplate = "(%s)";
    strFilter = (char*)msSmallMalloc(strlen(strFilterTemplate) + strlen(layer->filter.string));
    sprintf(strFilter, strFilterTemplate, layer->filter.string);
    strFilterLength = strlen(strFilter);
  }

  /* Populate strUid, if necessary. */
  if ( uid ) {
    static char *strUidTemplate = "\"%s\" = %ld";
    strUid = (char*)msSmallMalloc(strlen(strUidTemplate) + strlen(layerinfo->uid) + 64);
    sprintf(strUid, strUidTemplate, layerinfo->uid, *uid);
    strUidLength = strlen(strUid);
  }

  bufferSize = strRectLength + 5 + strFilterLength + 5 + strUidLength
               + strLimitLength + strOffsetLength;
  strWhere = (char*)msSmallMalloc(bufferSize);
  *strWhere = '\0';
  if ( strRect ) {
    strlcat(strWhere, strRect, bufferSize);
    insert_and++;
    free(strRect);
  }
  if ( strFilter ) {
    if ( insert_and ) {
      strlcat(strWhere, " and ", bufferSize);
    }
    strlcat(strWhere, strFilter, bufferSize);
    free(strFilter);
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
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQL(layerObj *layer, rectObj *rect, long *uid)
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
    strWhere = msPostGISBuildSQLWhere(layer, NULL, uid);
  else
    strWhere = msPostGISBuildSQLWhere(layer, rect, uid);

  if ( ! strWhere ) {
    msSetError(MS_MISCERR, "Failed to build SQL 'where'.", "msPostGISBuildSQL()");
    return NULL;
  }

  strSQLTemplate = strlen(strWhere) ? strSQLTemplate0 : strSQLTemplate1;

  strSQL = msSmallMalloc(strlen(strSQLTemplate) + strlen(strFrom) + strlen(strItems) + strlen(strWhere));
  sprintf(strSQL, strSQLTemplate, strItems, strFrom, strWhere);
  if (strItems) free(strItems);
  if (strFrom) free(strFrom);
  if (strWhere) free(strWhere);

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
    msSetError(MS_QUERYERR, "Base64 WKB returned is null!", "msPostGISReadShape()");
    return MS_FAILURE;
  }

  if(wkbstrlen > wkbstaticsize) {
    wkb = calloc(wkbstrlen, sizeof(char));
  } else {
    wkb = wkbstatic;
  }
#if TRANSFER_ENCODING == 64
  result = msPostGISBase64Decode(wkb, wkbstr, wkbstrlen - 1);
#else
  result = msPostGISHexDecode(wkb, wkbstr, wkbstrlen);
#endif

  if( ! result ) {
    if(wkb!=wkbstatic) free(wkb);
    return MS_FAILURE;
  }

  /* Initialize our wkbObj */
  w.wkb = (char*)wkb;
  w.ptr = w.wkb;
  w.size = (wkbstrlen - 1)/2;

  /* Set the type map according to what version of PostGIS we are dealing with */
  if( layerinfo->version >= 20000 ) /* PostGIS 2.0+ */
    w.typemap = wkb_postgis20;
  else
    w.typemap = wkb_postgis15;

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

    case MS_LAYER_ANNOTATION:
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
      msDebug("msPostGISReadShape: Setting shape->index = %d\n", uid);
      msDebug("msPostGISReadShape: Setting shape->resultindex = %d\n", layerinfo->rownum);
    }
    shape->index = uid;
    shape->resultindex = layerinfo->rownum;

    if( layer->debug > 2 ) {
      msDebug("msPostGISReadShape: [index] %d\n",  shape->index);
    }

    shape->numvalues = layer->numitems;

    msComputeBounds(shape);
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
      return MS_FAILURE;
    }

    /*
    ** Decrypt any encrypted token in connection string and attempt to connect.
    */
    conn_decrypted = msDecryptStringTokens(layer->map, layer->connection);
    if (conn_decrypted == NULL) {
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

      msSetError(MS_QUERYERR, "Database connection failed (%s) with connect string '%s'\nIs the database running? Is it allowing connections? Does the specified user exist? Is the password valid? Is the database on the standard port?", "msPostGISLayerOpen()", PQerrorMessage(layerinfo->pgconn), maskeddata);

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
        msSetError(MS_QUERYERR, "PostgreSQL database connection gone bad (%s)", "msPostGISLayerOpen()", PQerrorMessage(layerinfo->pgconn));
        return MS_FAILURE;
      }

    }
  }

  /* Get the PostGIS version number from the database */
  layerinfo->version = msPostGISRetrieveVersion(layerinfo->pgconn);
  if( layerinfo->version == MS_FAILURE ) return MS_FAILURE;
  if (layer->debug)
    msDebug("msPostGISLayerOpen: Got PostGIS version %d.\n", layerinfo->version);

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
  char** layer_bind_values = (char**)msSmallMalloc(sizeof(char*) * 1000);
  char* bind_value;
  char* bind_key = (char*)msSmallMalloc(3);

  int num_bind_values = 0;

  /* try to get the first bind value */
  bind_value = msLookupHashTable(&layer->bindvals, "1");
  while(bind_value != NULL) {
    /* put the bind value on the stack */
    layer_bind_values[num_bind_values] = bind_value;
    /* increment the counter */
    num_bind_values++;
    /* create a new lookup key */
    sprintf(bind_key, "%d", num_bind_values+1);
    /* get the bind_value */
    bind_value = msLookupHashTable(&layer->bindvals, bind_key);
  }

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
  strSQL = msPostGISBuildSQL(layer, &rect, NULL);
  if ( ! strSQL ) {
    msSetError(MS_QUERYERR, "Failed to build query SQL.", "msPostGISLayerWhichShapes()");
    return MS_FAILURE;
  }

  if (layer->debug) {
    msDebug("msPostGISLayerWhichShapes query: %s\n", strSQL);
  }

  if(num_bind_values > 0) {
    pgresult = PQexecParams(layerinfo->pgconn, strSQL, num_bind_values, NULL, (const char**)layer_bind_values, NULL, NULL, 1);
  } else {
    pgresult = PQexecParams(layerinfo->pgconn, strSQL,0, NULL, NULL, NULL, NULL, 0);
  }

  /* free bind values */
  free(bind_key);
  free(layer_bind_values);

  if ( layer->debug > 1 ) {
    msDebug("msPostGISLayerWhichShapes query status: %s (%d)\n", PQresStatus(PQresultStatus(pgresult)), PQresultStatus(pgresult));
  }

  /* Something went wrong. */
  if (!pgresult || PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
    if ( layer->debug ) {
      msDebug("Error (%s) executing query: %s", "msPostGISLayerWhichShapes()\n", PQerrorMessage(layerinfo->pgconn), strSQL);
    }
    msSetError(MS_QUERYERR, "Error executing query: %s ", "msPostGISLayerWhichShapes()", PQerrorMessage(layerinfo->pgconn));
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
    strSQL = msPostGISBuildSQL(layer, 0, &shapeindex);
    if ( ! strSQL ) {
      msSetError(MS_QUERYERR, "Failed to build query SQL.", "msPostGISLayerGetShape()");
      return MS_FAILURE;
    }

    if (layer->debug) {
      msDebug("msPostGISLayerGetShape query: %s\n", strSQL);
    }

    pgresult = PQexecParams(layerinfo->pgconn, strSQL,0, NULL, NULL, NULL, NULL, 0);

    /* Something went wrong. */
    if ( (!pgresult) || (PQresultStatus(pgresult) != PGRES_TUPLES_OK) ) {
      if ( layer->debug ) {
        msDebug("Error (%s) executing SQL: %s", "msPostGISLayerGetShape()\n", PQerrorMessage(layerinfo->pgconn), strSQL );
      }
      msSetError(MS_QUERYERR, "Error executing SQL: %s", "msPostGISLayerGetShape()", PQerrorMessage(layerinfo->pgconn));

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

    } else if( oid == INT4OID || oid == INT8OID ) {
      gml_type = "Integer";

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
    } else if( oid == DATEOID
               || oid == TIMESTAMPOID || oid == TIMESTAMPTZOID ) {
      gml_type = "Date";
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

  pgresult = PQexecParams(layerinfo->pgconn, sql,0, NULL, NULL, NULL, NULL, 0);

  if ( (!pgresult) || (PQresultStatus(pgresult) != PGRES_TUPLES_OK) ) {
    if ( layer->debug ) {
      msDebug("Error (%s) executing SQL: %s", "msPostGISLayerGetItems()\n", PQerrorMessage(layerinfo->pgconn), sql);
    }
    msSetError(MS_QUERYERR, "Error executing SQL: %s", "msPostGISLayerGetItems()", PQerrorMessage(layerinfo->pgconn));
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
int createPostgresTimeCompareSimple(const char *timecol, const char *timestring, char *dest, size_t destsize)
{
  int timeresolution = msTimeGetResolution(timestring);
  char timeStamp[100];
  char *interval;
  if (timeresolution < 0)
    return MS_FALSE;
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
  snprintf(dest, destsize,"(%s >= date_trunc('%s',%s) and %s < date_trunc('%s',%s) + interval '1 %s')",
           timecol, interval, timeStamp, timecol, interval, timeStamp, interval);
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
int createPostgresTimeCompareRange(const char *timecol, const char *mintime, const char *maxtime,
                                   char *dest, size_t destsize)
{
  int mintimeresolution = msTimeGetResolution(mintime);
  int maxtimeresolution = msTimeGetResolution(maxtime);
  char minTimeStamp[100];
  char maxTimeStamp[100];
  char *minTimeInterval,*maxTimeInterval;
  if (mintimeresolution < 0 || maxtimeresolution < 0)
    return MS_FALSE;
  postgresTimeStampForTimeString(mintime,minTimeStamp,100);
  postgresTimeStampForTimeString(maxtime,maxTimeStamp,100);

  switch(maxtimeresolution) {
    case TIME_RESOLUTION_YEAR:
      maxTimeInterval = "year";
      break;
    case TIME_RESOLUTION_MONTH:
      maxTimeInterval = "month";
      break;
    case TIME_RESOLUTION_DAY:
      maxTimeInterval = "day";
      break;
    case TIME_RESOLUTION_HOUR:
      maxTimeInterval = "hour";
      break;
    case TIME_RESOLUTION_MINUTE:
      maxTimeInterval = "minute";
      break;
    case TIME_RESOLUTION_SECOND:
      maxTimeInterval = "second";
      break;
    default:
      return MS_FAILURE;
  }
  switch(mintimeresolution) {
    case TIME_RESOLUTION_YEAR:
      minTimeInterval = "year";
      break;
    case TIME_RESOLUTION_MONTH:
      minTimeInterval = "month";
      break;
    case TIME_RESOLUTION_DAY:
      minTimeInterval = "day";
      break;
    case TIME_RESOLUTION_HOUR:
      minTimeInterval = "hour";
      break;
    case TIME_RESOLUTION_MINUTE:
      minTimeInterval = "minute";
      break;
    case TIME_RESOLUTION_SECOND:
      minTimeInterval = "second";
      break;
    default:
      return MS_FAILURE;
  }
  snprintf(dest, destsize,"(%s >= date_trunc('%s',%s) and %s < date_trunc('%s',%s) + interval '1 %s')",
           timecol, minTimeInterval, minTimeStamp,
           timecol, maxTimeInterval, maxTimeStamp, maxTimeInterval);
  return MS_SUCCESS;
}

int msPostGISLayerSetTimeFilter(layerObj *lp, const char *timestring, const char *timefield)
{
  char **atimes, **aranges = NULL;
  int numtimes=0,i=0,numranges=0;
  size_t buffer_size = 512;
  char buffer[512], bufferTmp[512];

  buffer[0] = '\0';
  bufferTmp[0] = '\0';

  if (!lp || !timestring || !timefield)
    return MS_FALSE;

  if( strchr(timestring,'\'') || strchr(timestring, '\\') ) {
     msSetError(MS_MISCERR, "Invalid time filter.", "msPostGISLayerSetTimeFilter()");
     return MS_FALSE;
  }

  /* discrete time */
  if (strstr(timestring, ",") == NULL &&
      strstr(timestring, "/") == NULL) { /* discrete time */
    createPostgresTimeCompareSimple(timefield, timestring, buffer, buffer_size);
  } else {

    /* multiple times, or ranges */
    atimes = msStringSplit (timestring, ',', &numtimes);
    if (atimes == NULL || numtimes < 1)
      return MS_FALSE;

    strlcat(buffer, "(", buffer_size);
    for(i=0; i<numtimes; i++) {
      if(i!=0) {
        strlcat(buffer, " OR ", buffer_size);
      }
      strlcat(buffer, "(", buffer_size);
      aranges = msStringSplit(atimes[i],  '/', &numranges);
      if(!aranges) return MS_FALSE;
      if(numranges == 1) {
        /* we don't have range, just a simple time */
        createPostgresTimeCompareSimple(timefield, atimes[i], bufferTmp, buffer_size);
        strlcat(buffer, bufferTmp, buffer_size);
      } else if(numranges == 2) {
        /* we have a range */
        createPostgresTimeCompareRange(timefield, aranges[0], aranges[1], bufferTmp, buffer_size);
        strlcat(buffer, bufferTmp, buffer_size);
      } else {
        return MS_FALSE;
      }
      msFreeCharArray(aranges, numranges);
      strlcat(buffer, ")", buffer_size);
    }
    strlcat(buffer, ")", buffer_size);
    msFreeCharArray(atimes, numtimes);
  }
  if(!*buffer) {
    return MS_FALSE;
  }
  if(lp->filteritem) free(lp->filteritem);
  lp->filteritem = msStrdup(timefield);
  if (&lp->filter) {
    /* if the filter is set and it's a string type, concatenate it with
       the time. If not just free it */
    if (lp->filter.type == MS_EXPRESSION) {
      snprintf(bufferTmp, buffer_size, "(%s) and %s", lp->filter.string, buffer);
      loadExpressionString(&lp->filter, bufferTmp);
    } else {
      freeExpression(&lp->filter);
      loadExpressionString(&lp->filter, buffer);
    }
  }


  return MS_TRUE;
}

char *msPostGISEscapeSQLParam(layerObj *layer, const char *pszString)
{
#ifdef USE_POSTGIS
  msPostGISLayerInfo *layerinfo = NULL;
  int nError;
  size_t nSrcLen;
  char* pszEscapedStr =NULL;

  if (layer && pszString && strlen(pszString) > 0) {
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

int msPostGISLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerInitItemInfo = msPostGISLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msPostGISLayerFreeItemInfo;
  layer->vtable->LayerOpen = msPostGISLayerOpen;
  layer->vtable->LayerIsOpen = msPostGISLayerIsOpen;
  layer->vtable->LayerWhichShapes = msPostGISLayerWhichShapes;
  layer->vtable->LayerNextShape = msPostGISLayerNextShape;
  layer->vtable->LayerGetShape = msPostGISLayerGetShape;
  layer->vtable->LayerClose = msPostGISLayerClose;
  layer->vtable->LayerGetItems = msPostGISLayerGetItems;
  /* layer->vtable->LayerGetExtent = msPostGISLayerGetExtent; */
  layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;
  /* layer->vtable->LayerGetAutoStyle, not supported for this layer */
  /* layer->vtable->LayerCloseConnection = msPostGISLayerClose; */
  layer->vtable->LayerSetTimeFilter = msPostGISLayerSetTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */

  /* layer->vtable->LayerGetAutoProjection, use defaut*/

  layer->vtable->LayerEscapeSQLParam = msPostGISEscapeSQLParam;
  layer->vtable->LayerEnablePaging = msPostGISEnablePaging;
  layer->vtable->LayerGetPaging = msPostGISGetPaging;

  return MS_SUCCESS;
}

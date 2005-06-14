/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  MapServer-GEOS integration.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2004 Regents of the University of Minnesota.
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.11  2005/06/14 04:28:44  sdlime
 * Fixed GEOS to shapeObj for multipolgon geometries.
 *
 * Revision 1.10  2005/05/19 05:57:08  sdlime
 * More interface clean up, added msGEOSFreeGeometry and updated geometry2shape code to so the new shapeObj contains a reference to the geometry used to create it.
 *
 * Revision 1.9  2005/05/17 13:59:57  sdlime
 * More GEOS interface improvements (global GeometryFactory).
 *
 * Revision 1.8  2005/05/11 07:10:21  sdlime
 * Finished the rest of the wrapper funtions for initial GEOS support which basically ammounts to buffer and convex hull creation from MapScript at the moment. There are likely a number of memory leaks associated with the implementation at the moment.
 *
 * Revision 1.7  2005/02/23 05:16:50  sdlime
 * Added GEOS=>shape conversion for GEOS_MULTILINE geometries.
 *
 * Revision 1.6  2005/02/23 04:52:33  sdlime
 * Added GEOS=>shape conversion for GEOS_MULTIPOINT geometries.
 *
 * Revision 1.5  2005/02/23 04:40:17  sdlime
 * Added wrapper for creating convex hulls to GEOS support. Added to MapScript as well.
 *
 * Revision 1.4  2005/02/22 18:33:37  dan
 * Fixes to build without USE_GEOS
 *
 * Revision 1.3  2005/02/22 07:40:27  sdlime
 * A bunch of updates to GEOS integration. Can move many primatives between
 * MapServer and GEOS, still need to do collections (e.g. multi-point/line/polygon).
 * Added buffer method to mapscript (mapscript/shape.i).
 *
 * Revision 1.2  2004/10/28 02:23:35  frank
 * added standard header
 *
 */

#include "map.h"

#ifdef USE_GEOS
#include <string>
#include <iostream>
#include <fstream>

#include "geos.h"

MS_CVSID("$Id$")

using namespace geos;

/*
** Global variables: only the read-only GeometryFactory.
*/
GeometryFactory *gf=NULL;

static void msGEOSCreateGeometryFactory() 
{
  if(!gf) 
    gf = new GeometryFactory();
}

/*
** Translation functions
*/
static Geometry *msGEOSShape2Geometry_point(pointObj *point)
{
  try {
    Coordinate *c = new Coordinate(point->x, point->y);
    Geometry *g = gf->createPoint(*c);
    delete c;
    return g;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSShape2Geometry_point()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static Geometry *msGEOSShape2Geometry_multipoint(lineObj *multipoint)
{
  try {
    int i;
    vector<Geometry *> *parts=NULL;

    parts = new vector<Geometry *>(multipoint->numpoints);
    for(i=0; i<multipoint->numpoints; i++)
      (*parts)[i] = msGEOSShape2Geometry_point(&(multipoint->point[i]));

    Geometry *g = gf->createMultiPoint(parts);

    return g;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSShape2Geometry_multiline()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static Geometry *msGEOSShape2Geometry_line(lineObj *line)
{
  try {
    int i;
    Coordinate c;

    DefaultCoordinateSequence *coords = new DefaultCoordinateSequence(line->numpoints);

    c.z = DoubleNotANumber; /* same for all points in the line (TODO: support z) */
    for(i=0; i<line->numpoints; i++) {
      c.x = line->point[i].x;
      c.y = line->point[i].y;
      coords->setAt(c, i);
    }

    Geometry *g = gf->createLineString(coords);

    return g;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSShape2Geometry_line()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static Geometry *msGEOSShape2Geometry_multiline(shapeObj *multiline)
{
  try {
    int i;
    vector<Geometry *> *parts=NULL;

    parts = new vector<Geometry *>(multiline->numlines);
    for(i=0; i<multiline->numlines; i++)
      (*parts)[i] = msGEOSShape2Geometry_line(&(multiline->line[i]));

    Geometry *g = gf->createMultiLineString(parts);

    return g;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSShape2Geometry_multiline()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static Geometry *msGEOSShape2Geometry_simplepolygon(shapeObj *shape, int r, int *outerList)
{
  try {
    int i, j, k;
    Coordinate c;
    LinearRing *outerRing, *innerRing;
    vector<Geometry *> *innerRings=NULL;
    int numInnerRings=0, *innerList;
    
    /* build the outer ring */
    DefaultCoordinateSequence *coords = new DefaultCoordinateSequence(shape->line[r].numpoints);
    
    c.z = DoubleNotANumber; /* same for all points in the line (TODO: support z) */
    for(i=0; i<shape->line[r].numpoints; i++) {
      c.x = shape->line[r].point[i].x;
      c.y = shape->line[r].point[i].y;
      coords->setAt(c, i);
    }
    
    outerRing = (LinearRing *) gf->createLinearRing(coords);
    
    /* build the inner rings */
    innerList = msGetInnerList(shape, r, outerList);    
    for(j=0; j<shape->numlines; j++)
      if(innerList[j] == MS_TRUE) numInnerRings++;

    /* printf("\tnumber of inner rings=%d\n", numInnerRings); */

    if(numInnerRings > 0) {
      k = 0; /* inner ring counter */
      innerRings = new vector<Geometry *>(numInnerRings);
      for(j=0; j<shape->numlines; j++) {
	if(innerList[j] == MS_FALSE) continue;
	
	DefaultCoordinateSequence *coords = new DefaultCoordinateSequence(shape->line[j].numpoints);
	
	c.z = DoubleNotANumber; /* same for all points in the line (TODO: support z) */
	for(i=0; i<shape->line[j].numpoints; i++) {
	  c.x = shape->line[j].point[i].x;
	  c.y = shape->line[j].point[i].y;
	  coords->setAt(c, i);
	}

	innerRing = (LinearRing *) gf->createLinearRing(coords);
	(*innerRings)[k] = innerRing;
	k++;
      }
    }

    Geometry *g = gf->createPolygon(outerRing, innerRings);

    free(innerList); /* clean up */
   
    return g;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSShape2Geometry_simplepolygon()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static Geometry *msGEOSShape2Geometry_polygon(shapeObj *shape)
{
  try {
    int i, j;
    vector<Geometry *> *parts=NULL;
    int *outerList, numOuterRings=0, lastOuterRing=0;
    Geometry *g=NULL;

    outerList = msGetOuterList(shape);
    for(i=0; i<shape->numlines; i++) {
      if(outerList[i] == MS_TRUE) {
	numOuterRings++;
	lastOuterRing = i; /* save for the simple case */
      }
    }

    /* printf("number of outer rings=%d\n", numOuterRings); */

    if(numOuterRings == 1) { 
      g = msGEOSShape2Geometry_simplepolygon(shape, lastOuterRing, outerList);
    } else { /* a true multipolygon */
      parts = new vector<Geometry *>(numOuterRings);

      j = 0; /* part counter */
      for(i=0; i<shape->numlines; i++) {
	if(outerList[i] == MS_FALSE) continue;
	/* printf("working on ring, id=%d (part %d)\n", i, j); */
	(*parts)[j] = msGEOSShape2Geometry_simplepolygon(shape, i, outerList); /* TODO: account for NULL return values */
	j++;
      }

      g = gf->createMultiPolygon(parts);
    }

    free(outerList);
    return g;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSShape2Geometry_line()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

Geometry *msGEOSShape2Geometry(shapeObj *shape)
{
  if(!shape)
    return NULL; /* a NULL shape generates a NULL geometry */

  /* if there is not an instance of a GeometeryFactory, create one */
  if(!gf) 
    msGEOSCreateGeometryFactory();

  switch(shape->type) {
  case MS_SHAPE_POINT:
    if(shape->numlines == 0 || shape->line[0].numpoints == 0) /* not enough info for a point */
      return NULL;

    if(shape->line[0].numpoints == 1) /* simple point */
      return msGEOSShape2Geometry_point(&(shape->line[0].point[0]));
    else /* multi-point */
      return msGEOSShape2Geometry_multipoint(&(shape->line[0]));
    break;
  case MS_SHAPE_LINE:
    if(shape->numlines == 0 || shape->line[0].numpoints < 2) /* not enough info for a line */
      return NULL;

    if(shape->numlines == 1) /* simple line */
      return msGEOSShape2Geometry_line(&(shape->line[0]));
    else /* multi-line */
      return msGEOSShape2Geometry_multiline(shape);
    break;
  case MS_SHAPE_POLYGON:
    if(shape->numlines == 0 || shape->line[0].numpoints < 4) /* not enough info for a polygon (first=last) */
      return NULL;

    return msGEOSShape2Geometry_polygon(shape); /* simple and multipolygon cases are addressed */
    break;
  default:
    break;
  }

  return NULL; /* should not get here */
}

static shapeObj *msGEOSGeometry2Shape_point(Geometry *g)
{
  try {
    const Coordinate *c = g->getCoordinate();
    shapeObj *shape=NULL;
    
    shape = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(shape);
    
    shape->type = MS_SHAPE_POINT;
    shape->line = (lineObj *) malloc(sizeof(lineObj));
    shape->numlines = 1;
    shape->line[0].point = (pointObj *) malloc(sizeof(pointObj));
    shape->line[0].numpoints = 1;
    shape->geometry = g;

    shape->line[0].point[0].x = c->x;
    shape->line[0].point[0].y = c->y;  
    /* shape->line[0].point[0].z = c->z; */

    return shape;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSGeometry2Shape_point()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static shapeObj *msGEOSGeometry2Shape_multipoint(Geometry *g)
{
  try {
    int i;
    int numPoints = g->getNumPoints();
    CoordinateSequence *coords = g->getCoordinates(); /* would be nice to have read-only access */
    Coordinate c;
    shapeObj *shape=NULL;

    shape = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(shape);

    shape->type = MS_SHAPE_POINT;
    shape->line = (lineObj *) malloc(sizeof(lineObj));
    shape->numlines = 1;
    shape->line[0].point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
    shape->line[0].numpoints = numPoints;
    shape->geometry = g;

    for(i=0; i<numPoints; i++) {
      c = coords->getAt(i);

      shape->line[0].point[i].x = c.x;
      shape->line[0].point[i].y = c.y;
      /* shape->line[0].point[i].z = c.z; */
    }

    delete coords;
    return shape;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSGeometry2Shape_multipoint()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static shapeObj *msGEOSGeometry2Shape_line(LineString *g)
{
  shapeObj *shape=NULL;

  try {
    int i;
    int numPoints = g->getNumPoints();
    const CoordinateSequence *coords = g->getCoordinatesRO(); /* a pointer to coordinates, do not delete */
    Coordinate c;

    shape = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(shape);

    shape->type = MS_SHAPE_LINE;
    shape->line = (lineObj *) malloc(sizeof(lineObj));
    shape->numlines = 1;
    shape->line[0].point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
    shape->line[0].numpoints = numPoints;
    shape->geometry = g;

    for(i=0; i<numPoints; i++) {
      c = coords->getAt(i);

      shape->line[0].point[i].x = c.x;
      shape->line[0].point[i].y = c.y;
      /* shape->line[0].point[i].z = c.z; */
    }

    return shape;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSGeometry2Shape_line()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static shapeObj *msGEOSGeometry2Shape_multiline(MultiLineString *g)
{
  try {
    int i, j;
    int numPoints, numLines = g->getNumGeometries();
    const CoordinateSequence *coords;
    const LineString *lineString;
    Coordinate c;

    shapeObj *shape=NULL;
    lineObj line;

    shape = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(shape);

    shape->type = MS_SHAPE_LINE;
    shape->line = (lineObj *) malloc(sizeof(lineObj)*numLines);
    shape->numlines = numLines;
    shape->geometry = g;

    for(j=0; j<numLines; j++) {
      lineString = (LineString *) g->getGeometryN(j);
      coords = lineString->getCoordinatesRO();
      numPoints = lineString->getNumPoints();

      line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
      line.numpoints = numPoints;

      for(i=0; i<numPoints; i++) {
	c = coords->getAt(i);
	
	line.point[i].x = c.x;
	line.point[i].y = c.y;
	/* line.point[i].z = c.z; */	
      }
      msAddLine(shape, &line);
      free(line.point);
    }

    return shape;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSGeometry2Shape_multiline()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static shapeObj *msGEOSGeometry2Shape_polygon(Polygon *g)
{
  try {
    shapeObj *shape=NULL;
    lineObj line;
    int numPoints, numRings;
    int i, j;

    Coordinate c;
    const CoordinateSequence *coords;
    const LineString *ring;

    shape = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(shape);
    shape->type = MS_SHAPE_POLYGON;
    shape->geometry = g;

    /* exterior ring */
    ring = g->getExteriorRing();
    coords = ring->getCoordinatesRO();
    numPoints = ring->getNumPoints();

    line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
    line.numpoints = numPoints;

    for(i=0; i<numPoints; i++) {
      c = coords->getAt(i);
      
      line.point[i].x = c.x;
      line.point[i].y = c.y;
      /* line.point[i].z = c.z; */
    }
    msAddLine(shape, &line);
    free(line.point);

    /* interior rings */
    numRings = g->getNumInteriorRing();
    for(j=0; j<numRings; j++) {
      ring = g->getInteriorRingN(j);
      coords = ring->getCoordinatesRO();
      numPoints = ring->getNumPoints();

      line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
      line.numpoints = numPoints;

      for(i=0; i<numPoints; i++) {
	c = coords->getAt(i);
	
	line.point[i].x = c.x;
	line.point[i].y = c.y;
	/* line.point[i].z = 0;
	   line.point[i].m = 0; */
      }
      msAddLine(shape, &line);
      free(line.point);
    }

    return shape;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSGeometry2Shape_polygon()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

static shapeObj *msGEOSGeometry2Shape_multipolygon(MultiPolygon *g)
{
  try {
    int i, j, k;
    shapeObj *shape=NULL;
    lineObj line;
    int numPoints, numRings, numPolygons=g->getNumGeometries();

    Coordinate c;
    const CoordinateSequence *coords;
    const LineString *ring;
    const Polygon *polygon;

    shape = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(shape);

    shape->type = MS_SHAPE_POLYGON;
    shape->geometry = g;

    for(k=0; k<numPolygons; k++) { /* for each polygon */
      polygon = (Polygon *) g->getGeometryN(k);

      /* exterior ring */
      ring = polygon->getExteriorRing();
      coords = ring->getCoordinatesRO();
      numPoints = ring->getNumPoints();

      line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
      line.numpoints = numPoints;

      for(i=0; i<numPoints; i++) {
	c = coords->getAt(i);

	line.point[i].x = c.x;
	line.point[i].y = c.y;
	/* line.point[i].z = c.z; */
      }
      msAddLine(shape, &line);
      free(line.point);

      /* interior rings */
      numRings = polygon->getNumInteriorRing();
      for(j=0; j<numRings; j++) {
	ring = polygon->getInteriorRingN(j);
	coords = ring->getCoordinatesRO();
	numPoints = ring->getNumPoints();

	line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
	line.numpoints = numPoints;

	for(i=0; i<numPoints; i++) {
	  c = coords->getAt(i);

	  line.point[i].x = c.x;
	  line.point[i].y = c.y;
	  /* line.point[i].z = 0;
	     line.point[i].m = 0; */
	}
	msAddLine(shape, &line);
	free(line.point);
      }
    } /* next polygon */

    return shape;
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSGeometry2Shape_multipolygon()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}

shapeObj *msGEOSGeometry2Shape(Geometry *g)
{
  if(!g) 
    return NULL; /* a NULL geometry generates a NULL shape */

  try {
    switch(g->getGeometryTypeId()) {
    case GEOS_POINT:
      return msGEOSGeometry2Shape_point(g);
      break;
    case GEOS_MULTIPOINT:
      return msGEOSGeometry2Shape_multipoint(g);
      break;
    case GEOS_LINESTRING:
      return msGEOSGeometry2Shape_line((LineString *)g);
      break;
    case GEOS_MULTILINESTRING:
      return msGEOSGeometry2Shape_multiline((MultiLineString *)g);
      break;
    case GEOS_POLYGON:
      return msGEOSGeometry2Shape_polygon((Polygon *)g);
      break;
    case GEOS_MULTIPOLYGON:
      return msGEOSGeometry2Shape_multipolygon((MultiPolygon *)g);
      break;
    default:
      msSetError(MS_GEOSERR, "Unsupported GEOS geometry type (%d).", "msGEOSGeometry2Shape()", g->getGeometryTypeId());
      return NULL;
    }
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSGeometry2Shape()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
}
#endif

/*
** Maintenence functions exposed to MapServer/MapScript.
*/

void msGEOSFreeGeometry(shapeObj *shape)
{
#ifdef USE_GEOS
  if(!shape || !shape->geometry || !gf) 
    return;

  try {
    Geometry *g = (Geometry *) shape->geometry;
    gf->destroyGeometry(g);
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSDeleteGeometry()", (char *) ge->toString().c_str());
    delete ge;
    return;
  } catch (...) {
    return;
  }
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSFreeGeometry()");
  return;
#endif
}

/*
** Analytical functions exposed to MapServer/MapScript.
*/

shapeObj *msGEOSBuffer(shapeObj *shape, double width)
{
#ifdef USE_GEOS
  if(!shape) 
    return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = msGEOSShape2Geometry(shape);
  Geometry *g = (Geometry *) shape->geometry;
  if(!g) 
    return NULL;

  try {
      Geometry *bg = g->buffer(width);
      return msGEOSGeometry2Shape(bg);
  } catch (GEOSException *ge) {     
    msSetError(MS_GEOSERR, "%s", "msGEOSBuffer()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSBuffer()");
  return NULL;
#endif
}

shapeObj *msGEOSConvexHull(shapeObj *shape)
{
#ifdef USE_GEOS
  if(!shape)
    return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = msGEOSShape2Geometry(shape);
  Geometry *g = (Geometry *) shape->geometry;
  if(!g)
    return NULL;

  try {
    Geometry *bg = g->convexHull();
    return msGEOSGeometry2Shape(bg);
  } catch (GEOSException *ge) {
    msSetError(MS_GEOSERR, "%s", "msGEOSConvexHull()", (char *) ge->toString().c_str());
    delete ge;
    return NULL;
  } catch (...) {
    return NULL;
  }
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSConvexHull()");
  return NULL;
#endif
}

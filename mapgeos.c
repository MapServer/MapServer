/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapServer-GEOS integration.
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
 *****************************************************************************/

#include "mapserver.h"
#include "mapgraph.h"

#ifdef USE_GEOS

// To avoid accidental use of non reentrant GEOS API.
// (check only effective in GEOS >= 3.5)
#define GEOS_USE_ONLY_R_API

#include <geos_c.h>

/*
** Error handling...
*/
static void msGEOSError(const char *format, ...)
{
  va_list args;

  va_start (args, format);
  msSetError(MS_GEOSERR, format, "msGEOSError()", args); /* just pass along to MapServer error handling */
  va_end(args);

  return;
}

static void msGEOSNotice(const char *fmt, ...)
{
  return; /* do nothing with notices at this point */
}

#ifndef USE_THREAD
static GEOSContextHandle_t geos_handle;

static inline GEOSContextHandle_t msGetGeosContextHandle()
{
  return geos_handle;
}

#else
#include "mapthread.h"
typedef struct geos_thread_info {
  struct geos_thread_info *next;
  void*             thread_id;
  GEOSContextHandle_t        geos_handle;
} geos_thread_info_t;

static geos_thread_info_t *geos_list = NULL;

static GEOSContextHandle_t msGetGeosContextHandle()
{
  geos_thread_info_t *link;
  GEOSContextHandle_t ret_obj;
  void*        thread_id;

  msAcquireLock( TLOCK_GEOS );

  thread_id = msGetThreadId();

  /* find link for this thread */

  for( link = geos_list;
       link != NULL && link->thread_id != thread_id
       && link->next != NULL && link->next->thread_id != thread_id;
       link = link->next ) {}

  /* If the target thread link is already at the head of the list were ok */
  if( geos_list != NULL && geos_list->thread_id == thread_id ) {
  }

  /* We don't have one ... initialize one. */
  else if( link == NULL || link->next == NULL ) {
    geos_thread_info_t *new_link;
    new_link = (geos_thread_info_t *) malloc(sizeof(geos_thread_info_t));
    new_link->next = geos_list;
    new_link->thread_id = thread_id;
    new_link->geos_handle = initGEOS_r(msGEOSNotice, msGEOSError);

    geos_list = new_link;
  }

  /* If the link is not already at the head of the list, promote it */
  else if( link != NULL && link->next != NULL ) {
    geos_thread_info_t *target = link->next;

    link->next = link->next->next;
    target->next = geos_list;
    geos_list = target;
  }

  ret_obj = geos_list->geos_handle;

  msReleaseLock( TLOCK_GEOS );

  return ret_obj;
}
#endif

/*
** Setup/Cleanup wrappers
*/
void msGEOSSetup()
{
#ifndef USE_THREAD
  geos_handle = initGEOS_r(msGEOSNotice, msGEOSError);
#endif
}

void msGEOSCleanup()
{
#ifndef USE_THREAD
  finishGEOS_r(geos_handle);
  geos_handle = NULL;
#else
  geos_thread_info_t *link;
  msAcquireLock( TLOCK_GEOS );
  for( link = geos_list; link != NULL;) {
    geos_thread_info_t *cur = link;
    link = link->next;
    finishGEOS_r(cur->geos_handle);
    free(cur);
  }
  geos_list = NULL;
  msReleaseLock( TLOCK_GEOS );
#endif
}

/*
** Translation functions
*/
static GEOSGeom msGEOSShape2Geometry_point(pointObj *point)
{
  GEOSCoordSeq coords;
  GEOSGeom g;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!point) return NULL;

  coords = GEOSCoordSeq_create_r(handle,1, 2); /* todo handle z's */
  if(!coords) return NULL;

  GEOSCoordSeq_setX_r(handle,coords, 0, point->x);
  GEOSCoordSeq_setY_r(handle,coords, 0, point->y);
  /* GEOSCoordSeq_setY(coords, 0, point->z); */

  g = GEOSGeom_createPoint_r(handle,coords); /* g owns the coordinate in coords */

  return g;
}

static GEOSGeom msGEOSShape2Geometry_multipoint(lineObj *multipoint)
{
  int i;
  GEOSGeom g;
  GEOSGeom *points;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!multipoint) return NULL;

  points = malloc(multipoint->numpoints*sizeof(GEOSGeom));
  if(!points) return NULL;

  for(i=0; i<multipoint->numpoints; i++)
    points[i] = msGEOSShape2Geometry_point(&(multipoint->point[i]));

  g = GEOSGeom_createCollection_r(handle,GEOS_MULTIPOINT, points, multipoint->numpoints);

  free(points);

  return g;
}

static GEOSGeom msGEOSShape2Geometry_line(lineObj *line)
{
  int i;
  GEOSGeom g;
  GEOSCoordSeq coords;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!line) return NULL;

  coords = GEOSCoordSeq_create_r(handle,line->numpoints, 2); /* todo handle z's */
  if(!coords) return NULL;

  for(i=0; i<line->numpoints; i++) {
    GEOSCoordSeq_setX_r(handle,coords, i, line->point[i].x);
    GEOSCoordSeq_setY_r(handle,coords, i, line->point[i].y);
    /* GEOSCoordSeq_setZ(coords, i, line->point[i].z); */
  }

  g = GEOSGeom_createLineString_r(handle,coords); /* g owns the coordinates in coords */

  return g;
}

static GEOSGeom msGEOSShape2Geometry_multiline(shapeObj *multiline)
{
  int i;
  GEOSGeom g;
  GEOSGeom *lines;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!multiline) return NULL;

  lines = malloc(multiline->numlines*sizeof(GEOSGeom));
  if(!lines) return NULL;

  for(i=0; i<multiline->numlines; i++)
    lines[i] = msGEOSShape2Geometry_line(&(multiline->line[i]));

  g = GEOSGeom_createCollection_r(handle,GEOS_MULTILINESTRING, lines, multiline->numlines);

  free(lines);

  return g;
}

static GEOSGeom msGEOSShape2Geometry_simplepolygon(shapeObj *shape, int r, int *outerList)
{
  int i, j, k;
  GEOSCoordSeq coords;
  GEOSGeom g;
  GEOSGeom outerRing;
  GEOSGeom *innerRings=NULL;
  int numInnerRings=0, *innerList;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape || !outerList) return NULL;

  /* build the outer shell */
  coords = GEOSCoordSeq_create_r(handle,shape->line[r].numpoints, 2); /* todo handle z's */
  if(!coords) return NULL;

  for(i=0; i<shape->line[r].numpoints; i++) {
    GEOSCoordSeq_setX_r(handle,coords, i, shape->line[r].point[i].x);
    GEOSCoordSeq_setY_r(handle,coords, i, shape->line[r].point[i].y);
    /* GEOSCoordSeq_setZ(coords, i, shape->line[r].point[i].z); */
  }

  outerRing = GEOSGeom_createLinearRing_r(handle,coords); /* outerRing owns the coordinates in coords */

  /* build the holes */
  innerList = msGetInnerList(shape, r, outerList);
  for(j=0; j<shape->numlines; j++)
    if(innerList[j] == MS_TRUE) numInnerRings++;

  if(numInnerRings > 0) {
    k = 0; /* inner ring counter */

    innerRings = msSmallMalloc(numInnerRings*sizeof(GEOSGeom));

    for(j=0; j<shape->numlines; j++) {
      if(innerList[j] == MS_FALSE) continue;

      coords = GEOSCoordSeq_create_r(handle,shape->line[j].numpoints, 2); /* todo handle z's */
      if(!coords) {
        free(innerRings);
        free(innerList);
        return NULL; /* todo, this will leak memory (shell + allocated holes) */
      }

      for(i=0; i<shape->line[j].numpoints; i++) {
        GEOSCoordSeq_setX_r(handle,coords, i, shape->line[j].point[i].x);
        GEOSCoordSeq_setY_r(handle,coords, i, shape->line[j].point[i].y);
        /* GEOSCoordSeq_setZ(coords, i, shape->line[j].point[i].z); */
      }

      innerRings[k] = GEOSGeom_createLinearRing_r(handle,coords); /* innerRings[k] owns the coordinates in coords */
      k++;
    }
  }

  g = GEOSGeom_createPolygon_r(handle,outerRing, innerRings, numInnerRings);

  free(innerList); /* clean up */
  free(innerRings); /* clean up */

  return g;
}

static GEOSGeom msGEOSShape2Geometry_polygon(shapeObj *shape)
{
  int i, j;
  GEOSGeom *polygons;
  int *outerList, numOuterRings=0, lastOuterRing=0;
  GEOSGeom g;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  outerList = msGetOuterList(shape);
  for(i=0; i<shape->numlines; i++) {
    if(outerList[i] == MS_TRUE) {
      numOuterRings++;
      lastOuterRing = i; /* save for the simple case */
    }
  }

  if(numOuterRings == 1) {
    g = msGEOSShape2Geometry_simplepolygon(shape, lastOuterRing, outerList);
  } else { /* a true multipolygon */
    polygons = msSmallMalloc(numOuterRings*sizeof(GEOSGeom));

    j = 0; /* part counter */
    for(i=0; i<shape->numlines; i++) {
      if(outerList[i] == MS_FALSE) continue;
      polygons[j] = msGEOSShape2Geometry_simplepolygon(shape, i, outerList); /* TODO: account for NULL return values */
      j++;
    }

    g = GEOSGeom_createCollection_r(handle,GEOS_MULTIPOLYGON, polygons, numOuterRings);
    free(polygons);
  }

  free(outerList);
  return g;
}

GEOSGeom msGEOSShape2Geometry(shapeObj *shape)
{
  if(!shape)
    return NULL; /* a NULL shape generates a NULL geometry */

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

static shapeObj *msGEOSGeometry2Shape_point(GEOSGeom g)
{
  GEOSCoordSeq coords;
  shapeObj *shape=NULL;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!g) return NULL;

  shape = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(shape);

  shape->type = MS_SHAPE_POINT;
  shape->line = (lineObj *) malloc(sizeof(lineObj));
  shape->numlines = 1;
  shape->line[0].point = (pointObj *) malloc(sizeof(pointObj));
  shape->line[0].numpoints = 1;
  shape->geometry = (GEOSGeom) g;

  coords = (GEOSCoordSeq) GEOSGeom_getCoordSeq_r(handle,g);

  GEOSCoordSeq_getX_r(handle,coords, 0, &(shape->line[0].point[0].x));
  GEOSCoordSeq_getY_r(handle,coords, 0, &(shape->line[0].point[0].y));
  /* GEOSCoordSeq_getZ(coords, 0, &(shape->line[0].point[0].z)); */

  shape->bounds.minx = shape->bounds.maxx = shape->line[0].point[0].x;
  shape->bounds.miny = shape->bounds.maxy = shape->line[0].point[0].y;

  return shape;
}

static shapeObj *msGEOSGeometry2Shape_multipoint(GEOSGeom g)
{
  int i;
  int numPoints;
  GEOSCoordSeq coords;
  GEOSGeom point;

  shapeObj *shape=NULL;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!g) return NULL;
  numPoints = GEOSGetNumGeometries_r(handle,g); /* each geometry has 1 point */

  shape = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(shape);

  shape->type = MS_SHAPE_POINT;
  shape->line = (lineObj *) malloc(sizeof(lineObj));
  shape->numlines = 1;
  shape->line[0].point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
  shape->line[0].numpoints = numPoints;
  shape->geometry = (GEOSGeom) g;

  for(i=0; i<numPoints; i++) {
    point = (GEOSGeom) GEOSGetGeometryN_r(handle,g, i);
    coords = (GEOSCoordSeq) GEOSGeom_getCoordSeq_r(handle,point);

    GEOSCoordSeq_getX_r(handle,coords, 0, &(shape->line[0].point[i].x));
    GEOSCoordSeq_getY_r(handle,coords, 0, &(shape->line[0].point[i].y));
    /* GEOSCoordSeq_getZ(coords, 0, &(shape->line[0].point[i].z)); */
  }

  msComputeBounds(shape);

  return shape;
}

static shapeObj *msGEOSGeometry2Shape_line(GEOSGeom g)
{
  shapeObj *shape=NULL;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  int i;
  int numPoints;
  GEOSCoordSeq coords;

  if(!g) return NULL;
  numPoints = GEOSGetNumCoordinates_r(handle,g);
  coords = (GEOSCoordSeq) GEOSGeom_getCoordSeq_r(handle,g);

  shape = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(shape);

  shape->type = MS_SHAPE_LINE;
  shape->line = (lineObj *) malloc(sizeof(lineObj));
  shape->numlines = 1;
  shape->line[0].point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
  shape->line[0].numpoints = numPoints;
  shape->geometry = (GEOSGeom) g;

  for(i=0; i<numPoints; i++) {
    GEOSCoordSeq_getX_r(handle,coords, i, &(shape->line[0].point[i].x));
    GEOSCoordSeq_getY_r(handle,coords, i, &(shape->line[0].point[i].y));
    /* GEOSCoordSeq_getZ(coords, i, &(shape->line[0].point[i].z)); */
  }

  msComputeBounds(shape);

  return shape;
}

static void msGEOSGeometry2Shape_multiline_part(GEOSContextHandle_t handle, GEOSGeom part, shapeObj *shape) {
  int i;
  int type, numGeometries, numPoints;
  GEOSCoordSeq coords;
  lineObj line;
  
  type = GEOSGeomTypeId_r(handle,part); 
  if (type == GEOS_MULTILINESTRING) {
    numGeometries = GEOSGetNumGeometries_r(handle,part);

    for(i=0; i<numGeometries; i++) {
      GEOSGeom subPart = (GEOSGeom) GEOSGetGeometryN_r(handle, part, i);
      msGEOSGeometry2Shape_multiline_part(handle, subPart, shape);
    }
  } else {
    numPoints = GEOSGetNumCoordinates_r(handle,part);
    coords = (GEOSCoordSeq) GEOSGeom_getCoordSeq_r(handle,part);
    line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
    line.numpoints = numPoints;

    for(i=0; i<numPoints; i++) {
      GEOSCoordSeq_getX_r(handle,coords, i, &(line.point[i].x));
      GEOSCoordSeq_getY_r(handle,coords, i, &(line.point[i].y));
      /* GEOSCoordSeq_getZ(coords, i, &(line.point[i].z)); */
    }

    msAddLineDirectly(shape, &line);
  }
}

static shapeObj *msGEOSGeometry2Shape_multiline(GEOSGeom g)
{
  int i;
  int numGeometries;
  GEOSGeom part;

  shapeObj *shape=NULL;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!g) return NULL;
  numGeometries = GEOSGetNumGeometries_r(handle,g);

  shape = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(shape);

  shape->type = MS_SHAPE_LINE;
  shape->geometry = (GEOSGeom) g;

  for(i=0; i<numGeometries; i++) {
    part = (GEOSGeom) GEOSGetGeometryN_r(handle, g, i);
    msGEOSGeometry2Shape_multiline_part(handle, part, shape);
  }

  msComputeBounds(shape);

  return shape;
}

static shapeObj *msGEOSGeometry2Shape_polygon(GEOSGeom g)
{
  shapeObj *shape=NULL;
  lineObj line;
  int numPoints, numRings;
  int i, j;

  GEOSCoordSeq coords;
  GEOSGeom ring;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!g) return NULL;

  shape = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(shape);
  shape->type = MS_SHAPE_POLYGON;
  shape->geometry = (GEOSGeom) g;

  /* exterior ring */
  ring = (GEOSGeom) GEOSGetExteriorRing_r(handle,g);
  numPoints = GEOSGetNumCoordinates_r(handle,ring);
  coords = (GEOSCoordSeq) GEOSGeom_getCoordSeq_r(handle,ring);

  line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
  line.numpoints = numPoints;

  for(i=0; i<numPoints; i++) {
    GEOSCoordSeq_getX_r(handle,coords, i, &(line.point[i].x));
    GEOSCoordSeq_getY_r(handle,coords, i, &(line.point[i].y));
    /* GEOSCoordSeq_getZ(coords, i, &(line.point[i].z)); */
  }
  msAddLineDirectly(shape, &line);

  /* interior rings */
  numRings = GEOSGetNumInteriorRings_r(handle,g);
  for(j=0; j<numRings; j++) {
    ring = (GEOSGeom) GEOSGetInteriorRingN_r(handle,g, j);
    if(GEOSisRing_r(handle,ring) != 1) continue; /* skip it */

    numPoints = GEOSGetNumCoordinates_r(handle,ring);
    coords = (GEOSCoordSeq) GEOSGeom_getCoordSeq_r(handle,ring);

    line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
    line.numpoints = numPoints;

    for(i=0; i<numPoints; i++) {
      GEOSCoordSeq_getX_r(handle,coords, i, &(line.point[i].x));
      GEOSCoordSeq_getY_r(handle,coords, i, &(line.point[i].y));
      /* GEOSCoordSeq_getZ(coords, i, &(line.point[i].z)); */
    }
    msAddLineDirectly(shape, &line);
  }

  msComputeBounds(shape);

  return shape;
}

static shapeObj *msGEOSGeometry2Shape_multipolygon(GEOSGeom g)
{
  int i, j, k;
  shapeObj *shape=NULL;
  lineObj line;
  int numPoints, numRings, numPolygons;

  GEOSCoordSeq coords;
  GEOSGeom polygon, ring;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!g) return NULL;
  numPolygons = GEOSGetNumGeometries_r(handle,g);

  shape = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(shape);
  shape->type = MS_SHAPE_POLYGON;
  shape->geometry = (GEOSGeom) g;

  for(k=0; k<numPolygons; k++) { /* for each polygon */
    polygon = (GEOSGeom) GEOSGetGeometryN_r(handle,g, k);

    /* exterior ring */
    ring = (GEOSGeom) GEOSGetExteriorRing_r(handle,polygon);
    numPoints = GEOSGetNumCoordinates_r(handle,ring);
    coords = (GEOSCoordSeq) GEOSGeom_getCoordSeq_r(handle,ring);

    line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
    line.numpoints = numPoints;

    for(i=0; i<numPoints; i++) {
      GEOSCoordSeq_getX_r(handle,coords, i, &(line.point[i].x));
      GEOSCoordSeq_getY_r(handle,coords, i, &(line.point[i].y));
      /* GEOSCoordSeq_getZ(coords, i, &(line.point[i].z)); */
    }
    msAddLineDirectly(shape, &line);

    /* interior rings */
    numRings = GEOSGetNumInteriorRings_r(handle,polygon);

    for(j=0; j<numRings; j++) {
      ring = (GEOSGeom) GEOSGetInteriorRingN_r(handle,polygon, j);
      if(GEOSisRing_r(handle,ring) != 1) continue; /* skip it */

      numPoints = GEOSGetNumCoordinates_r(handle,ring);
      coords = (GEOSCoordSeq) GEOSGeom_getCoordSeq_r(handle,ring);

      line.point = (pointObj *) malloc(sizeof(pointObj)*numPoints);
      line.numpoints = numPoints;

      for(i=0; i<numPoints; i++) {
        GEOSCoordSeq_getX_r(handle,coords, i, &(line.point[i].x));
        GEOSCoordSeq_getY_r(handle,coords, i, &(line.point[i].y));
        /* GEOSCoordSeq_getZ(coords, i, &(line.point[i].z)); */
      }
      msAddLineDirectly(shape, &line);
    }
  } /* next polygon */

  msComputeBounds(shape);

  return shape;
}

shapeObj *msGEOSGeometry2Shape(GEOSGeom g)
{
  int type;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!g)
    return NULL; /* a NULL geometry generates a NULL shape */

  type = GEOSGeomTypeId_r(handle,g);
  switch(type) {
    case GEOS_POINT:
      return msGEOSGeometry2Shape_point(g);
      break;
    case GEOS_MULTIPOINT:
      return msGEOSGeometry2Shape_multipoint(g);
      break;
    case GEOS_LINESTRING:
      return msGEOSGeometry2Shape_line(g);
      break;
    case GEOS_MULTILINESTRING:
      return msGEOSGeometry2Shape_multiline(g);
      break;
    case GEOS_POLYGON:
      return msGEOSGeometry2Shape_polygon(g);
      break;
    case GEOS_MULTIPOLYGON:
      return msGEOSGeometry2Shape_multipolygon(g);
      break;
    case GEOS_GEOMETRYCOLLECTION:
      if (!GEOSisEmpty_r(handle,g))
      {
        int i, j, numGeoms;
        shapeObj* shape;

        numGeoms = GEOSGetNumGeometries_r(handle,g);

        shape = (shapeObj *) malloc(sizeof(shapeObj));
        msInitShape(shape);
        shape->type = MS_SHAPE_LINE;
        shape->geometry = (GEOSGeom) g;
        
        numGeoms = GEOSGetNumGeometries_r(handle,g);
        for(i = 0; i < numGeoms; i++) { /* for each geometry */
           shapeObj* shape2 = msGEOSGeometry2Shape((GEOSGeom)GEOSGetGeometryN_r(handle,g, i));
           if (shape2) {
              for (j = 0; j < shape2->numlines; j++)
                 msAddLineDirectly(shape, &shape2->line[j]);
              shape2->numlines = 0;
              shape2->geometry = NULL; /* not owned */
              msFreeShape(shape2);
           }
        }
        msComputeBounds(shape);
        return shape;
      }
      break;
    default:
      msSetError(MS_GEOSERR, "Unsupported GEOS geometry type (%d).", "msGEOSGeometry2Shape()", type);
  }
  return NULL;
}
#endif

/*
** Maintenence functions exposed to MapServer/MapScript.
*/

void msGEOSFreeGeometry(shapeObj *shape)
{
#ifdef USE_GEOS
  GEOSGeom g=NULL;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape || !shape->geometry)
    return;

  g = (GEOSGeom) shape->geometry;
  GEOSGeom_destroy_r(handle,g);
  shape->geometry = NULL;
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSFreeGEOSGeom()");
  return;
#endif
}

/*
** WKT input and output functions
*/
shapeObj *msGEOSShapeFromWKT(const char *wkt)
{
#ifdef USE_GEOS
  GEOSGeom g;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!wkt)
    return NULL;

  g = GEOSGeomFromWKT_r(handle,wkt);
  if(!g) {
    msSetError(MS_GEOSERR, "Error reading WKT geometry \"%s\".", "msGEOSShapeFromWKT()", wkt);
    return NULL;
  } else {
    return msGEOSGeometry2Shape(g);
  }

#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSShapeFromWKT()");
  return NULL;
#endif
}

/* Return should be freed with msGEOSFreeWKT */
char *msGEOSShapeToWKT(shapeObj *shape)
{
#ifdef USE_GEOS
  GEOSGeom g;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape)
    return NULL;

  /* if we have a geometry, we should update it*/
  msGEOSFreeGeometry(shape);

  shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);
  g = (GEOSGeom) shape->geometry;
  if(!g) return NULL;

  return GEOSGeomToWKT_r(handle,g);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSShapeToWKT()");
  return NULL;
#endif
}

void msGEOSFreeWKT(char* pszGEOSWKT)
{
#ifdef USE_GEOS
  GEOSContextHandle_t handle = msGetGeosContextHandle();
#if GEOS_VERSION_MAJOR > 3 || (GEOS_VERSION_MAJOR == 3 && GEOS_VERSION_MINOR >= 2)
  GEOSFree_r(handle,pszGEOSWKT);
#endif
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSFreeWKT()");
#endif
}

shapeObj *msGEOSOffsetCurve(shapeObj *p, double offset) {
#if defined USE_GEOS && (GEOS_VERSION_MAJOR > 3 || (GEOS_VERSION_MAJOR == 3 && GEOS_VERSION_MINOR >= 3))
  int typeChanged = 0;
  GEOSGeom g1, g2 = NULL;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!p) 
    return NULL;

  /*
   * GEOSOffsetCurve_r() uses BufferBuilder.bufferLineSingleSided(), which
   * works with lines, naturally. In order to allow offsets for a MapServer
   * polygonObj, it has to be processed as line and afterwards reverted.
   */
  if(p->type == MS_SHAPE_POLYGON) {
    p->type = MS_SHAPE_LINE;
    typeChanged = 1;
    msGEOSFreeGeometry(p);
  }

  if(typeChanged || !p->geometry)
    p->geometry = (GEOSGeom) msGEOSShape2Geometry(p);

  g1 = (GEOSGeom) p->geometry;
  if(!g1) return NULL;
  
  if (GEOSGeomTypeId_r(handle,g1) == GEOS_MULTILINESTRING)
  {
    GEOSGeom *lines = malloc(p->numlines*sizeof(GEOSGeom));
    if (!lines) return NULL;
    for(int i=0; i<p->numlines; i++)
    {
      lines[i] = GEOSOffsetCurve_r(handle, GEOSGetGeometryN_r(handle,g1,i),
                                   offset, 4, GEOSBUF_JOIN_MITRE, fabs(offset*1.5));
    }
    g2 = GEOSGeom_createCollection_r(handle,GEOS_MULTILINESTRING, lines, p->numlines);
    free(lines);
  }
  else
  {
    g2 = GEOSOffsetCurve_r(handle,g1, offset, 4, GEOSBUF_JOIN_MITRE, fabs(offset*1.5));
  }

  /*
   * Undo change of geometry type. We won't re-create the geos gemotry here,
   * it's up to each geos function to create it.
   */
  if(typeChanged) {
    msGEOSFreeGeometry(p);
    p->type = MS_SHAPE_POLYGON;
  }

  if (g2)
    return msGEOSGeometry2Shape(g2);

  return NULL;
#else
  msSetError(MS_GEOSERR, "GEOS Offset Curve support is not available.", "msGEOSingleSidedBuffer()");
  return NULL;
#endif
}

/*
** Analytical functions exposed to MapServer/MapScript.
*/

shapeObj *msGEOSBuffer(shapeObj *shape, double width)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape)
    return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);

  g1 = (GEOSGeom) shape->geometry;
  if(!g1) return NULL;

  g2 = GEOSBuffer_r(handle, g1, width, 30);
  return msGEOSGeometry2Shape(g2);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSBuffer()");
  return NULL;
#endif
}

shapeObj *msGEOSSimplify(shapeObj *shape, double tolerance)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape)
    return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);

  g1 = (GEOSGeom) shape->geometry;
  if(!g1) return NULL;

  g2 = GEOSSimplify_r(handle, g1, tolerance);
  return msGEOSGeometry2Shape(g2);
#else
  msSetError(MS_GEOSERR, "GEOS Simplify support is not available.", "msGEOSSimplify()");
  return NULL;
#endif
}

shapeObj *msGEOSTopologyPreservingSimplify(shapeObj *shape, double tolerance)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape)
    return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);

  g1 = (GEOSGeom) shape->geometry;
  if(!g1) return NULL;

  g2 = GEOSTopologyPreserveSimplify_r(handle, g1, tolerance);
  return msGEOSGeometry2Shape(g2);
#else
  msSetError(MS_GEOSERR, "GEOS Simplify support is not available.", "msGEOSTopologyPreservingSimplify()");
  return NULL;
#endif
}

shapeObj *msGEOSConvexHull(shapeObj *shape)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape) return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);
  g1 = (GEOSGeom) shape->geometry;
  if(!g1) return NULL;

  g2 = GEOSConvexHull_r(handle, g1);
  return msGEOSGeometry2Shape(g2);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSConvexHull()");
  return NULL;
#endif
}

shapeObj *msGEOSBoundary(shapeObj *shape)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape) return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);
  g1 = (GEOSGeom) shape->geometry;
  if(!g1) return NULL;

  g2 = GEOSBoundary_r(handle, g1);
  return msGEOSGeometry2Shape(g2);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSBoundary()");
  return NULL;
#endif
}

pointObj *msGEOSGetCentroid(shapeObj *shape)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  GEOSCoordSeq coords;
  pointObj *point;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape) return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);
  g1 = (GEOSGeom) shape->geometry;
  if(!g1) return NULL;

  g2 = GEOSGetCentroid_r(handle, g1);
  if (!g2) return NULL;

  point = (pointObj *) malloc(sizeof(pointObj));

  coords = (GEOSCoordSeq) GEOSGeom_getCoordSeq_r(handle,g2);

  GEOSCoordSeq_getX_r(handle,coords, 0, &(point->x));
  GEOSCoordSeq_getY_r(handle,coords, 0, &(point->y));
  /* GEOSCoordSeq_getZ(coords, 0, &(point->z)); */

  GEOSGeom_destroy_r(handle, g2);

  return point;
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSGetCentroid()");
  return NULL;
#endif
}

shapeObj *msGEOSUnion(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2, g3;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return NULL;

  if(!shape1->geometry) /* if no geometry for the shape then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = (GEOSGeom) shape1->geometry;
  if(!g1) return NULL;

  if(!shape2->geometry) /* if no geometry for the shape then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = (GEOSGeom) shape2->geometry;
  if(!g2) return NULL;

  g3 = GEOSUnion_r(handle, g1, g2);
  return msGEOSGeometry2Shape(g3);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSUnion()");
  return NULL;
#endif
}

shapeObj *msGEOSIntersection(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2, g3;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return NULL;

  if(!shape1->geometry) /* if no geometry for the shape then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = (GEOSGeom) shape1->geometry;
  if(!g1) return NULL;

  if(!shape2->geometry) /* if no geometry for the shape then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = (GEOSGeom) shape2->geometry;
  if(!g2) return NULL;

  g3 = GEOSIntersection_r(handle, g1, g2);
  return msGEOSGeometry2Shape(g3);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSIntersection()");
  return NULL;
#endif
}

shapeObj *msGEOSDifference(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2, g3;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return NULL;

  if(!shape1->geometry) /* if no geometry for the shape then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = (GEOSGeom) shape1->geometry;
  if(!g1) return NULL;

  if(!shape2->geometry) /* if no geometry for the shape then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = (GEOSGeom) shape2->geometry;
  if(!g2) return NULL;

  g3 = GEOSDifference_r(handle, g1, g2);
  return msGEOSGeometry2Shape(g3);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSDifference()");
  return NULL;
#endif
}

shapeObj *msGEOSSymDifference(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2, g3;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return NULL;

  if(!shape1->geometry) /* if no geometry for the shape then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = (GEOSGeom) shape1->geometry;
  if(!g1) return NULL;

  if(!shape2->geometry) /* if no geometry for the shape then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = (GEOSGeom) shape2->geometry;
  if(!g2) return NULL;

  g3 = GEOSSymDifference_r(handle, g1, g2);
  return msGEOSGeometry2Shape(g3);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSSymDifference()");
  return NULL;
#endif
}

shapeObj *msGEOSLineMerge(shapeObj *shape)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape) return NULL;
  if(shape->type != MS_SHAPE_LINE) return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);
  g1 = (GEOSGeom) shape->geometry;
  if(!g1) return NULL;

  g2 = GEOSLineMerge_r(handle, g1);
  return msGEOSGeometry2Shape(g2);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSLineMerge()");
  return NULL;
#endif
}

shapeObj *msGEOSDelaunayTriangulation(shapeObj *shape, double tolerance, int onlyEdges)
{
  GEOSGeom g1, g2;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape) return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);
  g1 = (GEOSGeom) shape->geometry;
  if(!g1) return NULL;

  g2 = GEOSDelaunayTriangulation_r(handle, g1, tolerance, onlyEdges);
  return msGEOSGeometry2Shape(g2);
}

shapeObj *msGEOSVoronoiDiagram(shapeObj *shape, double tolerance, int onlyEdges)
{
#if defined(USE_GEOS) && GEOS_VERSION_MAJOR >= 3 && GEOS_VERSION_MINOR >= 5
  GEOSGeom g1, g2;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape) return NULL;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);
  g1 = (GEOSGeom) shape->geometry;
  if(!g1) return NULL;

  g2 = GEOSVoronoiDiagram_r(handle, g1, NULL, tolerance, onlyEdges);
  return msGEOSGeometry2Shape(g2);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available or GEOS version is not 3.5 or higher.", "msGEOSVoronoiDiagram()");
  return NULL;
#endif
}

#define COMPARE_POINTS(a,b) (((a).x!=(b).x || (a).y!=(b).y)?MS_FALSE:MS_TRUE)

static int keepEdge(lineObj *segment, shapeObj *polygon)
{
  int i,j;

  if(msIntersectPointPolygon(&segment->point[0], polygon) != MS_TRUE) return MS_FALSE;
  if(msIntersectPointPolygon(&segment->point[1], polygon) != MS_TRUE) return MS_FALSE;

  for(i=0; i<polygon->numlines; i++)
    for(j=1; j<polygon->line[i].numpoints; j++)
      if(msIntersectSegments(&(segment->point[0]), &(segment->point[1]), &(polygon->line[i].point[j-1]), &(polygon->line[i].point[j])) ==  MS_TRUE)
    	return(MS_FALSE);

  return(MS_TRUE);
}

// returns the index of the node, we use z to store a count of points at the same coordinate
static int buildNodes(multipointObj *nodes, pointObj *point) 
{
  int i;

  for(i=0; i<nodes->numpoints; i++) {
    if(COMPARE_POINTS(nodes->point[i], *point)) { // found it
      nodes->point[i].z++;
      return i;
    }
  }

  // not found, add it
  nodes->point[i].x = point->x;
  nodes->point[i].y = point->y;
  nodes->point[i].z = 1;
  nodes->numpoints++;

  return i;
}

shapeObj *msGEOSSkeletonize(shapeObj *shape, int depth)
{
#if defined(USE_GEOS) && GEOS_VERSION_MAJOR >= 3 && GEOS_VERSION_MINOR >= 5 
  int i, j;
  shapeObj *shape2, *shape3;

  graphObj *graph;
  multipointObj nodes;
  int src, dest;

  int *path=NULL, *tmp_path=NULL; // array of node indexes
  int path_size, tmp_path_size;
  double path_dist, tmp_path_dist, max_path_dist=-1;

  if(!shape) return NULL;
  if(shape->type != MS_SHAPE_POLYGON) return NULL;

  shape2 = msGEOSVoronoiDiagram(shape, 0.0, MS_TRUE);
  if(!shape2) return NULL;

  // process the edges and build a graph representation
  nodes.point = (pointObj *) malloc(shape2->numlines*sizeof(pointObj)*2);
  nodes.numpoints = 0;
  if(!nodes.point) {
    msFreeShape(shape2);
    return NULL;
  }

  graph = msCreateGraph(shape2->numlines*2);
  if(!graph) {
    msFreeShape(shape2);
    msFree(nodes.point);
    return NULL;
  }

  for(i=0; i<shape2->numlines; i++) {
    if(keepEdge(&shape2->line[i], shape) == MS_TRUE) {
      src = buildNodes(&nodes, &shape2->line[i].point[0]);
      dest = buildNodes(&nodes, &shape2->line[i].point[1]);
      msGraphAddEdge(graph, src, dest, msDistancePointToPoint(&shape2->line[i].point[0], &shape2->line[i].point[1]));
    }
  }
  msFreeShape(shape2); // done with voronoi geometry

  // step through edge nodes (z=1)
  for(i=0; i<nodes.numpoints; i++) {
    if(nodes.point[i].z != 1) continue; // skip

    if(!path) { // first, keep this path
      path = msGraphGetLongestShortestPath(graph, i, &path_size, &path_dist);
      max_path_dist = path_dist;
    } else {
      if(i == path[path_size-1]) continue; // skip, graph is bi-directional so it can't be any longer 
      tmp_path = msGraphGetLongestShortestPath(graph, i, &tmp_path_size, &tmp_path_dist);
      if(tmp_path_dist > max_path_dist) { // longer, keep this path
        free(path);
        path = tmp_path;
        path_size = tmp_path_size;
        path_dist = tmp_path_dist;
        max_path_dist = tmp_path_dist;
      } else { // skip path
        free(tmp_path);
      }
    }
  }

  msFreeGraph(graph); // done with graph

  // transform the path into a shape
  if(path) {
    lineObj line;
    line.point = (pointObj *) malloc(path_size*sizeof(pointObj));
    if(!line.point) {
      // clean up path and nodes
      return NULL;
    }
    line.numpoints = path_size;

    shape2 = (shapeObj *) malloc(sizeof(shapeObj));
    if(!shape2) {
      // clean up path, nodes and line.point
      return NULL;
    }
    msInitShape(shape2);
    shape2->type = MS_SHAPE_LINE;
    
    for(i=0; i<path_size; i++) {
      line.point[i].x = nodes.point[path[i]].x;
      line.point[i].y = nodes.point[path[i]].y;
    }
    msAddLineDirectly(shape2, &line);
  } else {
    free(nodes.point);
    return NULL;
  }

  free(path);
  free(nodes.point);
  return shape2;
#else
  msSetError(MS_GEOSERR, "GEOS support is not available or GEOS version is not 3.5 or higher.", "msGEOSSkeletonize()");
  return NULL;
#endif
}

/*
** Binary predicates exposed to MapServer/MapScript
*/

/*
** Does shape1 contain shape2, returns MS_TRUE/MS_FALSE or -1 for an error.
*/
int msGEOSContains(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return -1;

  if(!shape1->geometry) /* if no geometry for shape1 then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = shape1->geometry;
  if(!g1) return -1;

  if(!shape2->geometry) /* if no geometry for shape2 then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = shape2->geometry;
  if(!g2) return -1;

  result = GEOSContains_r(handle,g1, g2);
  return ((result==2) ? -1 : result);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSContains()");
  return -1;
#endif
}

/*
** Does shape1 overlap shape2, returns MS_TRUE/MS_FALSE or -1 for an error.
*/
int msGEOSOverlaps(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return -1;

  if(!shape1->geometry) /* if no geometry for shape1 then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = shape1->geometry;
  if(!g1) return -1;

  if(!shape2->geometry) /* if no geometry for shape2 then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = shape2->geometry;
  if(!g2) return -1;

  result = GEOSOverlaps_r(handle,g1, g2);
  return ((result==2) ? -1 : result);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSOverlaps()");
  return -1;
#endif
}

/*
** Is shape1 within shape2, returns MS_TRUE/MS_FALSE or -1 for an error.
*/
int msGEOSWithin(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return -1;

  if(!shape1->geometry) /* if no geometry for shape1 then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = shape1->geometry;
  if(!g1) return -1;

  if(!shape2->geometry) /* if no geometry for shape2 then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = shape2->geometry;
  if(!g2) return -1;

  result = GEOSWithin_r(handle,g1, g2);
  return ((result==2) ? -1 : result);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSWithin()");
  return -1;
#endif
}

/*
** Does shape1 cross shape2, returns MS_TRUE/MS_FALSE or -1 for an error.
*/
int msGEOSCrosses(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return -1;

  if(!shape1->geometry) /* if no geometry for shape1 then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = shape1->geometry;
  if(!g1) return -1;

  if(!shape2->geometry) /* if no geometry for shape2 then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = shape2->geometry;
  if(!g2) return -1;

  result = GEOSCrosses_r(handle,g1, g2);
  return ((result==2) ? -1 : result);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSCrosses()");
  return -1;
#endif
}

/*
** Does shape1 intersect shape2, returns MS_TRUE/MS_FALSE or -1 for an error.
*/
int msGEOSIntersects(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return -1;

  if(!shape1->geometry) /* if no geometry for shape1 then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = (GEOSGeom) shape1->geometry;
  if(!g1) return -1;

  if(!shape2->geometry) /* if no geometry for shape2 then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = (GEOSGeom) shape2->geometry;
  if(!g2) return -1;

  result = GEOSIntersects_r(handle,g1, g2);
  return ((result==2) ? -1 : result);
#else
  if(!shape1 || !shape2)
    return -1;

  switch(shape1->type) { /* todo: deal with point shapes */
    case(MS_SHAPE_LINE):
      switch(shape2->type) {
        case(MS_SHAPE_LINE):
          return msIntersectPolylines(shape1, shape2);
        case(MS_SHAPE_POLYGON):
          return msIntersectPolylinePolygon(shape1, shape2);
      }
      break;
    case(MS_SHAPE_POLYGON):
      switch(shape2->type) {
        case(MS_SHAPE_LINE):
          return msIntersectPolylinePolygon(shape2, shape1);
        case(MS_SHAPE_POLYGON):
          return msIntersectPolygons(shape1, shape2);
      }
      break;
  }

  return -1;
#endif
}

/*
** Does shape1 touch shape2, returns MS_TRUE/MS_FALSE or -1 for an error.
*/
int msGEOSTouches(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return -1;

  if(!shape1->geometry) /* if no geometry for shape1 then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = (GEOSGeom) shape1->geometry;
  if(!g1) return -1;

  if(!shape2->geometry) /* if no geometry for shape2 then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = (GEOSGeom) shape2->geometry;
  if(!g2) return -1;

  result = GEOSTouches_r(handle,g1, g2);
  return ((result==2) ? -1 : result);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSTouches()");
  return -1;
#endif
}

/*
** Does shape1 equal shape2, returns MS_TRUE/MS_FALSE or -1 for an error.
*/
int msGEOSEquals(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return -1;

  if(!shape1->geometry) /* if no geometry for shape1 then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = (GEOSGeom) shape1->geometry;
  if(!g1) return -1;

  if(!shape2->geometry) /* if no geometry for shape2 then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = (GEOSGeom) shape2->geometry;
  if(!g2) return -1;

  result = GEOSEquals_r(handle,g1, g2);
  return ((result==2) ? -1 : result);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSEquals()");
  return -1;
#endif
}

/*
** Are shape1 and shape2 disjoint, returns MS_TRUE/MS_FALSE or -1 for an error.
*/
int msGEOSDisjoint(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return -1;

  if(!shape1->geometry) /* if no geometry for shape1 then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = (GEOSGeom) shape1->geometry;
  if(!g1) return -1;

  if(!shape2->geometry) /* if no geometry for shape2 then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = (GEOSGeom) shape2->geometry;
  if(!g2) return -1;

  result = GEOSDisjoint_r(handle,g1, g2);
  return ((result==2) ? -1 : result);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSDisjoint()");
  return -1;
#endif
}

/*
** Useful misc. functions that return -1 on failure.
*/
double msGEOSArea(shapeObj *shape)
{
#if defined(USE_GEOS) && defined(GEOS_CAPI_VERSION_MAJOR) && defined(GEOS_CAPI_VERSION_MINOR) && (GEOS_CAPI_VERSION_MAJOR > 1 || GEOS_CAPI_VERSION_MINOR >= 1)
  GEOSGeom g;
  double area;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape) return -1;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);
  g = (GEOSGeom) shape->geometry;
  if(!g) return -1;

  result = GEOSArea_r(handle,g, &area);
  return  ((result==0) ? -1 : area);
#elif defined(USE_GEOS)
  msSetError(MS_GEOSERR, "GEOS support enabled, but old version lacks GEOSArea().", "msGEOSArea()");
  return -1;
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSArea()");
  return -1;
#endif
}

double msGEOSLength(shapeObj *shape)
{
#if defined(USE_GEOS) && defined(GEOS_CAPI_VERSION_MAJOR) && defined(GEOS_CAPI_VERSION_MINOR) && (GEOS_CAPI_VERSION_MAJOR > 1 || GEOS_CAPI_VERSION_MINOR >= 1)

  GEOSGeom g;
  double length;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape) return -1;

  if(!shape->geometry) /* if no geometry for the shape then build one */
    shape->geometry = (GEOSGeom) msGEOSShape2Geometry(shape);
  g = (GEOSGeom) shape->geometry;
  if(!g) return -1;

  result = GEOSLength_r(handle,g, &length);
  return  ((result==0) ? -1 : length);
#elif defined(USE_GEOS)
  msSetError(MS_GEOSERR, "GEOS support enabled, but old version lacks GEOSLength().", "msGEOSLength()");
  return -1;
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msGEOSLength()");
  return -1;
#endif
}

double msGEOSDistance(shapeObj *shape1, shapeObj *shape2)
{
#ifdef USE_GEOS
  GEOSGeom g1, g2;
  double distance;
  int result;
  GEOSContextHandle_t handle = msGetGeosContextHandle();

  if(!shape1 || !shape2)
    return -1;

  if(!shape1->geometry) /* if no geometry for shape1 then build one */
    shape1->geometry = (GEOSGeom) msGEOSShape2Geometry(shape1);
  g1 = (GEOSGeom) shape1->geometry;
  if(!g1) return -1;

  if(!shape2->geometry) /* if no geometry for shape2 then build one */
    shape2->geometry = (GEOSGeom) msGEOSShape2Geometry(shape2);
  g2 = (GEOSGeom) shape2->geometry;
  if(!g2) return -1;

  result = GEOSDistance_r(handle,g1, g2, &distance);
  return ((result==0) ? -1 : distance);
#else
  return msDistanceShapeToShape(shape1, shape2); /* fall back on brute force method (for MapScript) */
#endif
}

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Various geospatial search operations.
 * Author:   Steve Lime and the MapServer team.
 *
 * Notes: For information on point in polygon function please see:
 *
 *   http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
 *
 * The appropriate copyright notice accompanies the funtion definition.
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

#include "mapserver.h"



#define LASTVERT(v,n)  ((v) == 0 ? n-2 : v-1)
#define NEXTVERT(v,n)  ((v) == n-2 ? 0 : v+1)

/*
** Returns MS_TRUE if rectangles a and b overlap
*/
int msRectOverlap(const rectObj *a, const rectObj *b)
{
  if(a->minx > b->maxx) return(MS_FALSE);
  if(a->maxx < b->minx) return(MS_FALSE);
  if(a->miny > b->maxy) return(MS_FALSE);
  if(a->maxy < b->miny) return(MS_FALSE);
  return(MS_TRUE);
}

/*
** Computes the intersection of two rectangles, updating the first
** to be only the intersection of the two.  Returns MS_FALSE if
** the intersection is empty.
*/
int msRectIntersect( rectObj *a, const rectObj *b )
{
  if( a->maxx > b->maxx )
    a->maxx = b->maxx;
  if( a->minx < b->minx )
    a->minx = b->minx;
  if( a->maxy > b->maxy )
    a->maxy = b->maxy;
  if( a->miny < b->miny )
    a->miny = b->miny;

  if( a->maxx < a->minx || b->maxx < b->minx )
    return MS_FALSE;
  else
    return MS_TRUE;
}

/*
** Returns MS_TRUE if rectangle a is contained in rectangle b
*/
int msRectContained(const rectObj *a, const rectObj *b)
{
  if(a->minx >= b->minx && a->maxx <= b->maxx)
    if(a->miny >= b->miny && a->maxy <= b->maxy)
      return(MS_TRUE);
  return(MS_FALSE);
}

/*
** Merges rect b into rect a. Rect a changes, b does not.
*/
void msMergeRect(rectObj *a, rectObj *b)
{
  a->minx = MS_MIN(a->minx, b->minx);
  a->maxx = MS_MAX(a->maxx, b->maxx);
  a->miny = MS_MIN(a->miny, b->miny);
  a->maxy = MS_MAX(a->maxy, b->maxy);
}

int msPointInRect(const pointObj *p, const rectObj *rect)
{
  if(p->x < rect->minx) return(MS_FALSE);
  if(p->x > rect->maxx) return(MS_FALSE);
  if(p->y < rect->miny) return(MS_FALSE);
  if(p->y > rect->maxy) return(MS_FALSE);
  return(MS_TRUE);
}

int msPolygonDirection(lineObj *c)
{
  double mx, my, area;
  int i, v=0, lv, nv;

  /* first find lowest, rightmost point of polygon */
  mx = c->point[0].x;
  my = c->point[0].y;

  for(i=0; i<c->numpoints-1; i++) {
    if((c->point[i].y < my) || ((c->point[i].y == my) && (c->point[i].x > mx))) {
      v = i;
      mx = c->point[i].x;
      my = c->point[i].y;
    }
  }

  lv = LASTVERT(v,c->numpoints);
  nv = NEXTVERT(v,c->numpoints);

  area = c->point[lv].x*c->point[v].y - c->point[lv].y*c->point[v].x + c->point[lv].y*c->point[nv].x - c->point[lv].x*c->point[nv].y + c->point[v].x*c->point[nv].y - c->point[nv].x*c->point[v].y;
  if(area > 0)
    return(1); /* counter clockwise orientation */
  else if(area < 0) /* clockwise orientation */
    return(-1);
  else
    return(0); /* shouldn't happen unless the polygon is self intersecting */
}

/*
** Copyright (c) 1970-2003, Wm. Randolph Franklin
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
** associated documentation files (the "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
** following conditions:
**
** 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
**    following disclaimers.
** 2. Redistributions in binary form must reproduce the above copyright notice in the documentation and/or
**    other materials provided with the distribution.
** 3. The name of W. Randolph Franklin may not be used to endorse or promote products derived from this
**    Software without specific prior written permission.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
** LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
** NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
int msPointInPolygon(pointObj *p, lineObj *c)
{
  int i, j, status = MS_FALSE;

  for (i = 0, j = c->numpoints-1; i < c->numpoints; j = i++) {
    if ((((c->point[i].y<=p->y) && (p->y<c->point[j].y)) || ((c->point[j].y<=p->y) && (p->y<c->point[i].y))) && (p->x < (c->point[j].x - c->point[i].x) * (p->y - c->point[i].y) / (c->point[j].y - c->point[i].y) + c->point[i].x))
      status = !status;
  }
  return status;
}

/*
** Note: the following functions are brute force implementations. Some fancy
** computational geometry algorithms would speed things up nicely in many
** cases. In due time... -SDL-
*/

int msIntersectSegments(const pointObj *a, const pointObj *b, const pointObj *c, const pointObj *d)   /* from comp.graphics.alogorithms FAQ */
{

  double r, s;
  double denominator, numerator;

  numerator = ((a->y-c->y)*(d->x-c->x) - (a->x-c->x)*(d->y-c->y));
  denominator = ((b->x-a->x)*(d->y-c->y) - (b->y-a->y)*(d->x-c->x));

  if((denominator == 0) && (numerator == 0)) { /* lines are coincident, intersection is a line segement if it exists */
    if(a->y == c->y) { /* coincident horizontally, check x's */
      if(((a->x >= MS_MIN(c->x,d->x)) && (a->x <= MS_MAX(c->x,d->x))) || ((b->x >= MS_MIN(c->x,d->x)) && (b->x <= MS_MAX(c->x,d->x))))
        return(MS_TRUE);
      else
        return(MS_FALSE);
    } else { /* test for y's will work fine for remaining cases */
      if(((a->y >= MS_MIN(c->y,d->y)) && (a->y <= MS_MAX(c->y,d->y))) || ((b->y >= MS_MIN(c->y,d->y)) && (b->y <= MS_MAX(c->y,d->y))))
        return(MS_TRUE);
      else
        return(MS_FALSE);
    }
  }

  if(denominator == 0) /* lines are parallel, can't intersect */
    return(MS_FALSE);

  r = numerator/denominator;

  if((r<0) || (r>1))
    return(MS_FALSE); /* no intersection */

  numerator = ((a->y-c->y)*(b->x-a->x) - (a->x-c->x)*(b->y-a->y));
  s = numerator/denominator;

  if((s<0) || (s>1))
    return(MS_FALSE); /* no intersection */

  return(MS_TRUE);
}

/*
** Instead of using ring orientation we count the number of parts the
** point falls in. If odd the point is in the polygon, if 0 or even
** then the point is in a hole or completely outside.
*/
int msIntersectPointPolygon(pointObj *point, shapeObj *poly)
{
  int i;
  int status=MS_FALSE;

  for(i=0; i<poly->numlines; i++) {
    if(msPointInPolygon(point, &poly->line[i]) == MS_TRUE) /* ok, the point is in a line */
      status = !status;
  }

  return(status);
}

int msIntersectMultipointPolygon(shapeObj *multipoint, shapeObj *poly)
{
  int i,j;

  /* The change to loop through all the lines has been made for ticket
   * #2443 but is no more needed since ticket #2762. PostGIS now put all
   * points into a single line.  */
  for(i=0; i<multipoint->numlines; i++ ) {
    lineObj points = multipoint->line[i];
    for(j=0; j<points.numpoints; j++) {
      if(msIntersectPointPolygon(&(points.point[j]), poly) == MS_TRUE)
        return(MS_TRUE);
    }
  }

  return(MS_FALSE);
}

int msIntersectPolylines(shapeObj *line1, shapeObj *line2)
{
  int c1,v1,c2,v2;

  for(c1=0; c1<line1->numlines; c1++)
    for(v1=1; v1<line1->line[c1].numpoints; v1++)
      for(c2=0; c2<line2->numlines; c2++)
        for(v2=1; v2<line2->line[c2].numpoints; v2++)
          if(msIntersectSegments(&(line1->line[c1].point[v1-1]), &(line1->line[c1].point[v1]),
                                 &(line2->line[c2].point[v2-1]), &(line2->line[c2].point[v2])) ==  MS_TRUE)
            return(MS_TRUE);

  return(MS_FALSE);
}

int msIntersectPolylinePolygon(shapeObj *line, shapeObj *poly)
{
  int i;

  /* STEP 1: polygon might competely contain the polyline or one of it's parts (only need to check one point from each part) */
  for(i=0; i<line->numlines; i++) {
    if(msIntersectPointPolygon(&(line->line[i].point[0]), poly) == MS_TRUE) /* this considers holes and multiple parts */
      return(MS_TRUE);
  }

  /* STEP 2: look for intersecting line segments */
  if (msIntersectPolylines(line, poly) == MS_TRUE)
    return (MS_TRUE);

  return(MS_FALSE);
}

int msIntersectPolygons(shapeObj *p1, shapeObj *p2)
{
  int i;

  /* STEP 1: polygon 1 completely contains 2 (only need to check one point from each part) */
  for(i=0; i<p2->numlines; i++) {
    if(msIntersectPointPolygon(&(p2->line[i].point[0]), p1) == MS_TRUE) /* this considers holes and multiple parts */
      return(MS_TRUE);
  }

  /* STEP 2: polygon 2 completely contains 1 (only need to check one point from each part) */
  for(i=0; i<p1->numlines; i++) {
    if(msIntersectPointPolygon(&(p1->line[i].point[0]), p2) == MS_TRUE) /* this considers holes and multiple parts */
      return(MS_TRUE);
  }

  /* STEP 3: look for intersecting line segments */
  if (msIntersectPolylines(p1, p2) == MS_TRUE)
    return(MS_TRUE);

  /*
  ** At this point we know there are are no intersections between edges. There may be other tests necessary
  ** but I haven't run into any cases that require them.
  */

  return(MS_FALSE);
}


/*
** Distance computations
*/

double msDistancePointToPoint(pointObj *a, pointObj *b)
{
  double d;

  d = sqrt(msSquareDistancePointToPoint(a, b));

  return(d);
}

/*
** Quickly compute the square of the distance; avoids expensive sqrt() call on each invocation
*/
double msSquareDistancePointToPoint(pointObj *a, pointObj *b)
{
  double dx, dy;

  dx = a->x - b->x;
  dy = a->y - b->y;

  return(dx*dx + dy*dy);
}

double msDistancePointToSegment(pointObj *p, pointObj *a, pointObj *b)
{
  return (sqrt(msSquareDistancePointToSegment(p, a, b)));
}

double msSquareDistancePointToSegment(pointObj *p, pointObj *a, pointObj *b)
{
  double l_squared; /* squared length of line ab */
  double r,s;

  l_squared = msSquareDistancePointToPoint(a,b);

  if(l_squared == 0.0) /* a = b */
    return(msSquareDistancePointToPoint(a,p));

  r = ((a->y - p->y)*(a->y - b->y) - (a->x - p->x)*(b->x - a->x))/(l_squared);

  if(r > 1) /* perpendicular projection of P is on the forward extention of AB */
    return(MS_MIN(msSquareDistancePointToPoint(p, b),msSquareDistancePointToPoint(p, a)));
  if(r < 0) /* perpendicular projection of P is on the backward extention of AB */
    return(MS_MIN(msSquareDistancePointToPoint(p, b),msSquareDistancePointToPoint(p, a)));

  s = ((a->y - p->y)*(b->x - a->x) - (a->x - p->x)*(b->y - a->y))/l_squared;

  return(fabs(s*s*l_squared));
}

#define SMALL_NUMBER 0.00000001
#define dot(u,v) ((u).x *(v).x + (u).y *(v).y) /* vector dot product */
#define norm(v) sqrt(dot(v,v))

#define slope(a,b) (((a)->y - (b)->y)/((a)->x - (b)->x))

/* Segment to segment distance code is a modified version of that found at:  */
/*  */
/* http://www.geometryalgorithms.com/Archive/algorithm_0106/algorithm_0106.htm */
/*  */
/* Copyright 2001, softSurfer (www.softsurfer.com) */
/* This code may be freely used and modified for any purpose */
/* providing that this copyright notice is included with it. */
/* SoftSurfer makes no warranty for this code, and cannot be held */
/* liable for any real or imagined damage resulting from its use. */
/* Users of this code must verify correctness for their application. */

double msDistanceSegmentToSegment(pointObj *pa, pointObj *pb, pointObj *pc, pointObj *pd)
{
  vectorObj u, v, w;

  /* check for strictly parallel segments first */
  /* if(((pa->x == pb->x) && (pc->x == pd->x)) || (slope(pa,pb) == slope(pc,pd))) { // vertical (infinite slope) || otherwise parallel */
  /* D = msDistancePointToSegment(pa, pc, pd); */
  /* D = MS_MIN(D, msDistancePointToSegment(pb, pc, pd)); */
  /* D = MS_MIN(D, msDistancePointToSegment(pc, pa, pb)); */
  /* return(MS_MIN(D, msDistancePointToSegment(pd, pa, pb))); */
  /* } */

  u.x = pb->x - pa->x; /* u = pb - pa */
  u.y = pb->y - pa->y;
  v.x = pd->x - pc->x; /* v = pd - pc  */
  v.y = pd->y - pc->y;
  w.x = pa->x - pc->x; /* w = pa - pc */
  w.y = pa->y - pc->y;

  const double a = dot(u,u);
  const double b = dot(u,v);
  const double c = dot(v,v);
  const double d = dot(u,w);
  const double e = dot(v,w);

  const double D = a*c - b*b;
  /* N=numerator, D=demoninator */
  double sN = 0;
  double sD = D;
  double tD = D;
  double tN = D;

  /* compute the line parameters of the two closest points */
  if(D < SMALL_NUMBER) { /* lines are parallel or almost parallel */
    sD = 1.0;
    tN = e;
    tD = c;
  } else { /* get the closest points on the infinite lines */
    sN = b*e - c*d;
    tN = a*e - b*d;
    if(sN < 0) {
      sN = 0.0;
      tN = e;
      tD = c;
    } else if(sN > sD) {
      sN = sD;
      tN = e + b;
      tD = c;
    }
  }

  if(tN < 0) {
    tN = 0.0;
    if(-d < 0) {
      /* sN = 0.0 */
    }
    else if(-d > a)
      sN = sD;
    else {
      sN = -d;
      sD = a;
    }
  } else if(tN > tD) {
    tN = tD;
    if((-d + b) < 0) {
      /* sN = 0.0 */
    }
    else if((-d + b) > a)
      sN = sD;
    else {
      sN = (-d + b);
      sD = a;
    }
  }

  /* finally do the division to get sc and tc */
  const double sc = sN/sD;
  const double tc = tN/tD;

  vectorObj dP;
  dP.x = w.x + (sc*u.x) - (tc*v.x);
  dP.y = w.y + (sc*u.y) - (tc*v.y);

  return(norm(dP));
}

double msDistancePointToShape(pointObj *point, shapeObj *shape)
{
  double d;

  d = msSquareDistancePointToShape(point, shape);

  return(sqrt(d));
}

/*
** As msDistancePointToShape; avoid expensive sqrt calls
*/
double msSquareDistancePointToShape(pointObj *point, shapeObj *shape)
{
  int i, j;
  double dist, minDist=-1;

  switch(shape->type) {
    case(MS_SHAPE_POINT):
      for(j=0; j<shape->numlines; j++) {
        for(i=0; i<shape->line[j].numpoints; i++) {
          dist = msSquareDistancePointToPoint(point, &(shape->line[j].point[i]));
          if((dist < minDist) || (minDist < 0)) minDist = dist;
        }
      }
      break;
    case(MS_SHAPE_LINE):
      for(j=0; j<shape->numlines; j++) {
        for(i=1; i<shape->line[j].numpoints; i++) {
          dist = msSquareDistancePointToSegment(point, &(shape->line[j].point[i-1]), &(shape->line[j].point[i]));
          if((dist < minDist) || (minDist < 0)) minDist = dist;
        }
      }
      break;
    case(MS_SHAPE_POLYGON):
      if(msIntersectPointPolygon(point, shape))
        minDist = 0; /* point is IN the shape */
      else { /* treat shape just like a line */
        for(j=0; j<shape->numlines; j++) {
          for(i=1; i<shape->line[j].numpoints; i++) {
            dist = msSquareDistancePointToSegment(point, &(shape->line[j].point[i-1]), &(shape->line[j].point[i]));
            if((dist < minDist) || (minDist < 0)) minDist = dist;
          }
        }
      }
      break;
    default:
      break;
  }

  return(minDist);
}

double msDistanceShapeToShape(shapeObj *shape1, shapeObj *shape2)
{
  int i,j,k,l;
  double dist, minDist=-1;

  switch(shape1->type) {
    case(MS_SHAPE_POINT): /* shape1 */
      for(i=0; i<shape1->numlines; i++) {
        for(j=0; j<shape1->line[i].numpoints; j++) {
          dist = msSquareDistancePointToShape(&(shape1->line[i].point[j]), shape2);
          if((dist < minDist) || (minDist < 0))
            minDist = dist;
        }
      }
      minDist = sqrt(minDist);
      break;
    case(MS_SHAPE_LINE): /* shape1 */
      switch(shape2->type) {
        case(MS_SHAPE_POINT):
          for(i=0; i<shape2->numlines; i++) {
            for(j=0; j<shape2->line[i].numpoints; j++) {
              dist = msSquareDistancePointToShape(&(shape2->line[i].point[j]), shape1);
              if((dist < minDist) || (minDist < 0))
                minDist = dist;
            }
          }
          minDist = sqrt(minDist);
          break;
        case(MS_SHAPE_LINE):
          for(i=0; i<shape1->numlines; i++) {
            for(j=1; j<shape1->line[i].numpoints; j++) {
              for(k=0; k<shape2->numlines; k++) {
                for(l=1; l<shape2->line[k].numpoints; l++) {
                  /* check intersection (i.e. dist=0) */
                  if(msIntersectSegments(&(shape1->line[i].point[j-1]), &(shape1->line[i].point[j]), &(shape2->line[k].point[l-1]), &(shape2->line[k].point[l])) == MS_TRUE)
                    return(0);

                  /* no intersection, compute distance */
                  dist = msDistanceSegmentToSegment(&(shape1->line[i].point[j-1]), &(shape1->line[i].point[j]), &(shape2->line[k].point[l-1]), &(shape2->line[k].point[l]));
                  if((dist < minDist) || (minDist < 0))
                    minDist = dist;
                }
              }
            }
          }
          break;
        case(MS_SHAPE_POLYGON):
          /* shape2 (the polygon) could contain shape1 or one of it's parts       */
          for(i=0; i<shape1->numlines; i++) {
            if(msIntersectPointPolygon(&(shape1->line[0].point[0]), shape2) == MS_TRUE) /* this considers holes and multiple parts */
              return(0);
          }

          /* check segment intersection and, if necessary, distance between segments */
          for(i=0; i<shape1->numlines; i++) {
            for(j=1; j<shape1->line[i].numpoints; j++) {
              for(k=0; k<shape2->numlines; k++) {
                for(l=1; l<shape2->line[k].numpoints; l++) {
                  /* check intersection (i.e. dist=0) */
                  if(msIntersectSegments(&(shape1->line[i].point[j-1]), &(shape1->line[i].point[j]), &(shape2->line[k].point[l-1]), &(shape2->line[k].point[l])) == MS_TRUE)
                    return(0);

                  /* no intersection, compute distance */
                  dist = msDistanceSegmentToSegment(&(shape1->line[i].point[j-1]), &(shape1->line[i].point[j]), &(shape2->line[k].point[l-1]), &(shape2->line[k].point[l]));
                  if((dist < minDist) || (minDist < 0))
                    minDist = dist;
                }
              }
            }
          }
          break;
      }
      break;
    case(MS_SHAPE_POLYGON): /* shape1 */
      switch(shape2->type) {
        case(MS_SHAPE_POINT):
          for(i=0; i<shape2->numlines; i++) {
            for(j=0; j<shape2->line[i].numpoints; j++) {
              dist = msSquareDistancePointToShape(&(shape2->line[i].point[j]), shape1);
              if((dist < minDist) || (minDist < 0))
                minDist = dist;
            }
          }
          minDist = sqrt(minDist);
          break;
        case(MS_SHAPE_LINE):
          /* shape1 (the polygon) could contain shape2 or one of it's parts       */
          for(i=0; i<shape2->numlines; i++) {
            if(msIntersectPointPolygon(&(shape2->line[i].point[0]), shape1) == MS_TRUE) /* this considers holes and multiple parts */
              return(0);
          }

          /* check segment intersection and, if necessary, distance between segments */
          for(i=0; i<shape1->numlines; i++) {
            for(j=1; j<shape1->line[i].numpoints; j++) {
              for(k=0; k<shape2->numlines; k++) {
                for(l=1; l<shape2->line[k].numpoints; l++) {
                  /* check intersection (i.e. dist=0) */
                  if(msIntersectSegments(&(shape1->line[i].point[j-1]), &(shape1->line[i].point[j]), &(shape2->line[k].point[l-1]), &(shape2->line[k].point[l])) == MS_TRUE)
                    return(0);

                  /* no intersection, compute distance */
                  dist = msDistanceSegmentToSegment(&(shape1->line[i].point[j-1]), &(shape1->line[i].point[j]), &(shape2->line[k].point[l-1]), &(shape2->line[k].point[l]));
                  if((dist < minDist) || (minDist < 0))
                    minDist = dist;
                }
              }
            }
          }
          break;
        case(MS_SHAPE_POLYGON):
          /* shape1 completely contains shape2 (only need to check one point from each part) */
          for(i=0; i<shape2->numlines; i++) {
            if(msIntersectPointPolygon(&(shape2->line[i].point[0]), shape1) == MS_TRUE) /* this considers holes and multiple parts */
              return(0);
          }

          /* shape2 completely contains shape1 (only need to check one point from each part) */
          for(i=0; i<shape1->numlines; i++) {
            if(msIntersectPointPolygon(&(shape1->line[i].point[0]), shape2) == MS_TRUE) /* this considers holes and multiple parts */
              return(0);
          }

          /* check segment intersection and, if necessary, distance between segments */
          for(i=0; i<shape1->numlines; i++) {
            for(j=1; j<shape1->line[i].numpoints; j++) {
              for(k=0; k<shape2->numlines; k++) {
                for(l=1; l<shape2->line[k].numpoints; l++) {
                  /* check intersection (i.e. dist=0) */
                  if(msIntersectSegments(&(shape1->line[i].point[j-1]), &(shape1->line[i].point[j]), &(shape2->line[k].point[l-1]), &(shape2->line[k].point[l])) == MS_TRUE)
                    return(0);

                  /* no intersection, compute distance */
                  dist = msDistanceSegmentToSegment(&(shape1->line[i].point[j-1]), &(shape1->line[i].point[j]), &(shape2->line[k].point[l-1]), &(shape2->line[k].point[l]));
                  if((dist < minDist) || (minDist < 0))
                    minDist = dist;
                }
              }
            }
          }
          break;
      }
      break;
  }

  return(minDist);
}

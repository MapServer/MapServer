#include "map.h"

#define LASTVERT(v,n)  ((v) == 0 ? n-2 : v-1)
#define NEXTVERT(v,n)  ((v) == n-2 ? 0 : v+1)

/*
** Returns MS_TRUE if rectangles a and b overlap
*/
int msRectOverlap(rectObj *a, rectObj *b)
{
  if(a->minx > b->maxx) return(MS_FALSE);
  if(a->maxx < b->minx) return(MS_FALSE);
  if(a->miny > b->maxy) return(MS_FALSE);
  if(a->maxy < b->miny) return(MS_FALSE);
  return(MS_TRUE);
}

/*
** Returns MS_TRUE if rectangle a is contained in rectangle b
*/
int msRectContained(rectObj *a, rectObj *b)
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

int msPointInRect(pointObj *p, rectObj *rect)
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
  else 
    if(area < 0) /* clockwise orientation */
      return(-1);
    else
      return(0); /* shouldn't happen unless the polygon is self intersecting */
}

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

int msIntersectSegments(pointObj *a, pointObj *b, pointObj *c, pointObj *d) { /* from comp.graphics.alogorithms FAQ */

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
int msIntersectPointPolygon(pointObj *point, shapeObj *poly) {
  int i;
  int status=MS_FALSE;

  for(i=0; i<poly->numlines; i++) {
    if(msPointInPolygon(point, &poly->line[i]) == MS_TRUE) /* ok, the point is in a line */
      status = !status;
  }

  return(status);  
}

int msIntersectMultipointPolygon(multipointObj *points, shapeObj *poly) {
  int i;
  
  for(i=0; i<points->numpoints; i++) {
    if(msIntersectPointPolygon(&(points->point[i]), poly) == MS_TRUE)
      return(MS_TRUE);
  }
    
  return(MS_FALSE);
}

int msIntersectPolylines(shapeObj *line1, shapeObj *line2) {
  int c1,v1,c2,v2;

  for(c1=0; c1<line1->numlines; c1++)
    for(v1=1; v1<line1->line[c1].numpoints; v1++)
      for(c2=0; c2<line2->numlines; c2++)
	for(v2=1; v2<line2->line[c2].numpoints; v2++)
	  if(msIntersectSegments(&(line1->line[c1].point[v1-1]), &(line1->line[c1].point[v1]), &(line2->line[c2].point[v2-1]), &(line2->line[c2].point[v2])) ==  MS_TRUE)
	    return(MS_TRUE);

  return(MS_FALSE);
}

int msIntersectPolylinePolygon(shapeObj *line, shapeObj *poly) {
  int c1,v1,c2,v2;

  /* STEP 1: look for intersecting line segments */
  for(c1=0; c1<line->numlines; c1++)
    for(v1=1; v1<line->line[c1].numpoints; v1++)
      for(c2=0; c2<poly->numlines; c2++)
	for(v2=1; v2<poly->line[c2].numpoints; v2++)
	  if(msIntersectSegments(&(line->line[c1].point[v1-1]), &(line->line[c1].point[v1]), &(poly->line[c2].point[v2-1]), &(poly->line[c2].point[v2])) ==  MS_TRUE)
	    return(MS_TRUE);

  /* STEP 2: polygon might competely contain the polyline or one of it's parts (only need to check one point from each part) */
  for(c1=0; c1<line->numlines; c1++) {
    if(msIntersectPointPolygon(&(line->line[0].point[0]), poly) == MS_TRUE) // this considers holes and multiple parts
      return(MS_TRUE);
  }

  return(MS_FALSE);
}

int msIntersectPolygons(shapeObj *p1, shapeObj *p2) {
  int c1,v1,c2,v2;

  /* STEP 1: polygon 1 completely contains 2 (only need to check one point from each part) */
  for(c2=0; c2<p2->numlines; c2++) {
    if(msIntersectPointPolygon(&(p2->line[c2].point[0]), p1) == MS_TRUE) // this considers holes and multiple parts
      return(MS_TRUE);
  }

  /* STEP 2: polygon 2 completely contains 1 (only need to check one point from each part) */
  for(c1=0; c1<p1->numlines; c1++) {
    if(msIntersectPointPolygon(&(p1->line[c1].point[0]), p2) == MS_TRUE) // this considers holes and multiple parts
      return(MS_TRUE);
  }

  /* STEP 3: look for intersecting line segments */
  for(c1=0; c1<p1->numlines; c1++)
    for(v1=1; v1<p1->line[c1].numpoints; v1++)
      for(c2=0; c2<p2->numlines; c2++)
	for(v2=1; v2<p2->line[c2].numpoints; v2++)
	  if(msIntersectSegments(&(p1->line[c1].point[v1-1]), &(p1->line[c1].point[v1]), &(p2->line[c2].point[v2-1]), &(p2->line[c2].point[v2])) ==  MS_TRUE)	    
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
  double dx, dy;
  
  dx = a->x - b->x;
  dy = a->y - b->y;
  d = sqrt(dx*dx + dy*dy);
  return(d);
}

double msDistancePointToSegment(pointObj *p, pointObj *a, pointObj *b)
{
  double l; /* length of line ab */
  double r,s;

  l = msDistancePointToPoint(a,b);

  if(l == 0.0) // a = b
    return( msDistancePointToPoint(a,p));

  r = ((a->y - p->y)*(a->y - b->y) - (a->x - p->x)*(b->x - a->x))/(l*l);

  if(r > 1) /* perpendicular projection of P is on the forward extention of AB */
    return(MS_MIN(msDistancePointToPoint(p, b),msDistancePointToPoint(p, a)));
  if(r < 0) /* perpendicular projection of P is on the backward extention of AB */
    return(MS_MIN(msDistancePointToPoint(p, b),msDistancePointToPoint(p, a)));

  s = ((a->y - p->y)*(b->x - a->x) - (a->x - p->x)*(b->y - a->y))/(l*l);

  return(fabs(s*l));
}

double msDistanceSegmentToSegment(pointObj *a, pointObj *b, pointObj *c, pointObj *d) 
{
  return(-1);
}

double msDistancePointToShape(pointObj *point, shapeObj *shape)
{
  int i, j;
  double dist, minDist=-1;

  switch(shape->type) {
  case(MS_SHAPE_POINT):    
    for(j=0;j<shape->numlines;j++) {
      for(i=0; i<shape->line[j].numpoints; i++) {
        dist = msDistancePointToPoint(point, &(shape->line[j].point[i]));
        if((dist < minDist) || (minDist < 0)) minDist = dist;
      }
    }
    break;
  case(MS_SHAPE_LINE):
    for(j=0;j<shape->numlines;j++) {
      for(i=1; i<shape->line[j].numpoints; i++) {
        dist = msDistancePointToSegment(point, &(shape->line[j].point[i-1]), &(shape->line[j].point[i]));
        if((dist < minDist) || (minDist < 0)) minDist = dist;
      }
    }
    break;
  case(MS_SHAPE_POLYGON):
    if(msIntersectPointPolygon(point, shape))
      minDist = 0; // point is IN the shape
    else { // treat shape just like a line
      for(j=0;j<shape->numlines;j++) {
        for(i=1; i<shape->line[j].numpoints; i++) {
          dist = msDistancePointToSegment(point, &(shape->line[j].point[i-1]), &(shape->line[j].point[i]));
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

double msDistanceShapeToShape(pointObj *point, shapeObj *shape)
{
  return(-1);
}

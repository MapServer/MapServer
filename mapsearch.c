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
** Length of the line segment defined by points a and b.
*/
double msDistanceBetweenPoints(pointObj *a, pointObj *b)
{
  double d;
  double dx, dy;
  
  dx = a->x - b->x;
  dy = a->y - b->y;
  d = sqrt(dx*dx + dy*dy);
  return(d);
}

double msDistanceFromPointToLine(pointObj *p, pointObj *a, pointObj *b)
{
  double l; /* length of line ab */
  double r,s;

  l = msDistanceBetweenPoints(a,b);

  if(l == 0.0) // a = b
    return( msDistanceBetweenPoints(a,p));

  r = ((a->y - p->y)*(a->y - b->y) - (a->x - p->x)*(b->x - a->x))/(l*l);
  s = ((a->y - p->y)*(b->x - a->x) - (a->x - p->x)*(b->y - a->y))/(l*l);

  if(r > 1) /* perpendicular projection of P is on the forward extention of AB */
    return(MS_MIN(msDistanceBetweenPoints(p, b),msDistanceBetweenPoints(p, a)));
  if(r < 0) /* perpendicular projection of P is on the backward extention of AB */
    return(MS_MIN(msDistanceBetweenPoints(p, b),msDistanceBetweenPoints(p, a)));

  return(fabs(s*l));
}

/*
** Finds the minimum distance to a group of points.
*/
double msDistanceFromPointToMultipoint(pointObj *p, multipointObj *points)
{
  int i;
  double d, mind=-1;

  for(i=0; i<points->numpoints; i++) {
    d = msDistanceBetweenPoints(p, &(points->point[i]));
    if((d < mind) || (mind < 0))
      mind = d;
  }

  return(mind);
}

/*
** Finds minimum distance from a point to a polyline. Computed from the
** closest line segment in the polyline.
*/
double msDistanceFromPointToPolyline(pointObj *p, shapeObj *polyline)
{
  int i,j;
  double d, mind=-1;

  for(j=0;j<polyline->numlines;j++) {
    for(i=1; i<polyline->line[j].numpoints; i++) {
      d = msDistanceFromPointToLine(p, &(polyline->line[j].point[i-1]), &(polyline->line[j].point[i]));
      if((d < mind) || (mind < 0))
	mind = d;
    }
  }
  return(mind);
}

/*
** Finds minimum distance from a point to a polygon. If the point is IN the
** polygon the distance will be zero. Otherwise the distance will be to the
** closest line segment in the polygon.
*/
double msDistanceFromPointToPolygon(pointObj *p, shapeObj *poly)
{
  int i;
  int status=MS_FALSE;
  
  /* first see if the point is IN the polygon */
  for(i=0; i<poly->numlines; i++) {
    if(msPointInPolygon(p, &poly->line[i]) == MS_TRUE) { /* ok, the point is in a line */
      status = !status;
    }
  }

  if(status == MS_FALSE) /* i.e. we're not IN the polygon, need a distance */
    return(msDistanceFromPointToPolyline(p, poly));
  else    
    return(0);
}

/*
** Note: the following functions are brute force implementations. Some fancy
** computational geometry algorithms would speed things up nicely in many
** cases. In due time... -SDL-
*/

int intersectLines(pointObj a, pointObj b, pointObj c, pointObj d) { /* from comp.graphics.alogorithms FAQ */

  double r, s;
  double denominator, numerator;

  numerator = ((a.y-c.y)*(d.x-c.x) - (a.x-c.x)*(d.y-c.y));  
  denominator = ((b.x-a.x)*(d.y-c.y) - (b.y-a.y)*(d.x-c.x));  

  if((denominator == 0) && (numerator == 0)) { /* lines are coincident, intersection is a line segement if it exists */
    if(a.y == c.y) { /* coincident horizontally, check x's */
      if(((a.x >= MS_MIN(c.x,d.x)) && (a.x <= MS_MAX(c.x,d.x))) || ((b.x >= MS_MIN(c.x,d.x)) && (b.x <= MS_MAX(c.x,d.x))))
	return(MS_TRUE);
      else
	return(MS_FALSE);
    } else { /* test for y's will work fine for remaining cases */
      if(((a.y >= MS_MIN(c.y,d.y)) && (a.y <= MS_MAX(c.y,d.y))) || ((b.y >= MS_MIN(c.y,d.y)) && (b.y <= MS_MAX(c.y,d.y))))
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

  numerator = ((a.y-c.y)*(b.x-a.x) - (a.x-c.x)*(b.y-a.y));
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
	  if(intersectLines(line1->line[c1].point[v1-1], line1->line[c1].point[v1], line2->line[c2].point[v2-1], line2->line[c2].point[v2]) ==  MS_TRUE)
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
	  if(intersectLines(line->line[c1].point[v1-1], line->line[c1].point[v1], poly->line[c2].point[v2-1], poly->line[c2].point[v2]) ==  MS_TRUE)
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

  /* STEP 1: look for intersecting line segments */
  for(c1=0; c1<p1->numlines; c1++)
    for(v1=1; v1<p1->line[c1].numpoints; v1++)
      for(c2=0; c2<p2->numlines; c2++)
	for(v2=1; v2<p2->line[c2].numpoints; v2++)
	  if(intersectLines(p1->line[c1].point[v1-1], p1->line[c1].point[v1], p2->line[c2].point[v2-1], p2->line[c2].point[v2]) ==  MS_TRUE)	    
	    return(MS_TRUE);

  /*
  ** At this point we know there are are no intersections between edges. However, one polygon might
  ** contain portions of the other. There may be a case that the following two steps don't catch, but
  ** I haven't found one yet.
  */

  /* STEP 2: polygon 1 completely contains 2 (only need to check one point from each part) */
  for(c2=0; c2<p2->numlines; c2++) {
    if(msIntersectPointPolygon(&(p2->line[c2].point[0]), p1) == MS_TRUE) // this considers holes and multiple parts
      return(MS_TRUE);
  }

  /* STEP 3: polygon 2 completely contains 1 (only need to check one point from each part) */
  for(c1=0; c1<p1->numlines; c1++) {
    if(msIntersectPointPolygon(&(p1->line[c1].point[0]), p2) == MS_TRUE) // this considers holes and multiple parts
      return(MS_TRUE);
  }

  return(MS_FALSE);
}

#ifdef USE_PROJ
char *msWhichShapesProj(shapefileObj *shp, rectObj window, projectionObj *in, projectionObj *out)
{
  int i;
  rectObj shape_rect, search_rect;
  char *status=NULL;
  char *filename;

  search_rect = window;
  if((in->numargs > 0) && (out->numargs > 0))
    msProjectRect(out->proj, in->proj, &search_rect); /* project the search_rect to shapefile coords */

  if(msRectContained(&shp->bounds, &search_rect) == MS_TRUE) {
    status = msAllocBitArray(shp->numshapes);
    if(!status) {
      msSetError(MS_MEMERR, NULL, "msWhichShapes()");
      return(NULL);
    }
    for(i=0;i<shp->numshapes;i++) 
      msSetBit(status, i, 1);
  } else {
    if((filename = (char *)malloc(strlen(shp->source)+strlen(MS_INDEX_EXTENSION)+1)) == NULL) {
      msSetError(MS_MEMERR, NULL, "msWhichShapes()");    
      return(NULL);
    }
    sprintf(filename, "%s%s", shp->source, MS_INDEX_EXTENSION);
    
    status = msSearchDiskTree(filename, search_rect);
    free(filename);

    if(!status) { /* no index */
      status = msAllocBitArray(shp->numshapes);
      if(!status) {
	msSetError(MS_MEMERR, NULL, "msWhichShapes()");       
	return(NULL);
      }
      
      for(i=0;i<shp->numshapes;i++) { /* for each shape */
	if(!SHPReadBounds(shp->hSHP, i, &shape_rect))
	  if(msRectOverlap(&shape_rect, &search_rect) == MS_TRUE)
	    msSetBit(status, i, 1);
      }
    } else { /* there was an index */

      /* 
      ** We need to refine the status array for potenial matches against
      ** actual shape boundaries. The index search doesn't have enough 
      ** information to do this itself.
      */
      for(i=0;i<shp->numshapes;i++) { /* for each shape */
        if(msGetBit(status, i)) {
	  if(!SHPReadBounds(shp->hSHP, i, &shape_rect))
	    if(msRectOverlap(&shape_rect, &search_rect) != MS_TRUE)
	      msSetBit(status, i, 0);
        }
      }

    } 
  }
 
  return(status); /* success */
}
#endif

char *msWhichShapes(shapefileObj *shp, rectObj window)
{
  int i;
  rectObj shape_rect, search_rect;
  char *status=NULL;
  char *filename;

  search_rect = window;
  
  if(msRectContained(&shp->bounds, &search_rect) == MS_TRUE) {
    status = msAllocBitArray(shp->numshapes);
    if(!status) {
      msSetError(MS_MEMERR, NULL, "msWhichShapes()");
      return(NULL);
    }
    for(i=0;i<shp->numshapes;i++) msSetBit(status, i, 1);
  } else {
    if((filename = (char *)malloc(strlen(shp->source)+strlen(MS_INDEX_EXTENSION)+1)) == NULL) {
      msSetError(MS_MEMERR, NULL, "msWhichShapes()");    
      return(NULL);
    }
    sprintf(filename, "%s%s", shp->source, MS_INDEX_EXTENSION);
    
    status = msSearchDiskTree(filename, search_rect);
    free(filename);    

    if(!status) { /* no index */

      status = msAllocBitArray(shp->numshapes);
      if(!status) {
	msSetError(MS_MEMERR, NULL, "msWhichShapes()");       
	return(NULL);
      }
      
      for(i=0;i<shp->numshapes;i++) { /* for each shape */
	if(!SHPReadBounds(shp->hSHP, i, &shape_rect))
	  if(msRectOverlap(&shape_rect, &search_rect) == MS_TRUE)
	    msSetBit(status, i, 1);
      }

    } else { /* there was an index */

      /* 
      ** We need to refine the status array for potenial matches against
      ** actual shape boundaries. The index search doesn't have enough 
      ** information to do this itself.
      */
      for(i=0;i<shp->numshapes;i++) { /* for each shape */
        if(msGetBit(status, i)) {
	  if(!SHPReadBounds(shp->hSHP, i, &shape_rect))
	    if(msRectOverlap(&shape_rect, &search_rect) != MS_TRUE)
	      msSetBit(status, i, 0);
        }
      }

    } 
  }

  return(status); /* success */
}

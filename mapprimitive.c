#include "map.h"
#include "mapprimitive.h"

typedef enum {CLIP_LEFT, CLIP_MIDDLE, CLIP_RIGHT} CLIP_STATE;

#define CLIP_CHECK(min, a, max) ((a) < (min) ? CLIP_LEFT : ((a) > (max) ? CLIP_RIGHT : CLIP_MIDDLE));
#define ROUND(a)       ( (a) + 0.5 )
#define SWAP( a, b, t) ( (t) = (a), (a) = (b), (b) = (t) )
#define EDGE_CHECK( x0, x, x1) ((x) < MS_MIN( (x0), (x1)) ? CLIP_LEFT : ((x) > MS_MAX( (x0), (x1)) ? CLIP_RIGHT : CLIP_MIDDLE ))

# define INFINITY	(1.0e+30)
# define NEARZERO	(1.0e-30)	/* 1/INFINITY */

void msPrintShape(shapeObj *p) 
{
  int i,j;

  msDebug("Shape contains %d parts.\n",  p->numlines);
  for (i=0; i<p->numlines; i++) {
    msDebug("\tPart %d contains %d points.\n", i, p->line[i].numpoints);
    for (j=0; j<p->line[i].numpoints; j++) {
      msDebug("\t\t%d: (%f, %f)\n", j, p->line[i].point[j].x, p->line[i].point[j].y);
    }
  }
}

void msInitShape(shapeObj *shape)
{
  // spatial component
  shape->line = NULL;
  shape->numlines = 0;
  shape->type = MS_SHAPE_NULL;
  shape->bounds.minx = shape->bounds.miny = -1;
  shape->bounds.maxx = shape->bounds.maxy = -1;
  
  // attribute component
  shape->values = NULL;
  shape->numvalues = 0;

  // annotation component
  shape->text = NULL;

  // bookkeeping component
  shape->classindex = 0; // default class
  shape->tileindex = shape->index = -1;
}

int msCopyShape(shapeObj *from, shapeObj *to) {
  int i;

  if(!from || !to) return(-1);

  for(i=0; i<from->numlines; i++)
    msAddLine(to, &(from->line[i])); // copy each line

  to->type = from->type;

  to->bounds.minx = from->bounds.minx;
  to->bounds.miny = from->bounds.miny;
  to->bounds.maxx = from->bounds.maxx;
  to->bounds.maxy = from->bounds.maxy;

  if(from->text) to->text = strdup(from->text);

  to->classindex = from->classindex;
  to->index = from->index;
  to->tileindex = from->tileindex;

  if(from->values) {    
    to->values = (char **)malloc(sizeof(char *)*from->numvalues);
    for(i=0; i<from->numvalues; i++)
      to->values[i] = strdup(from->values[i]);
    to->numvalues = from->numvalues;
  }

  return(0);
}

void msFreeShape(shapeObj *shape)
{
  int c;

  for (c= 0; c < shape->numlines; c++)
    free(shape->line[c].point);
  free(shape->line);

  if(shape->values) msFreeCharArray(shape->values, shape->numvalues);
  if(shape->text) free(shape->text);
  
  msInitShape(shape); // now reset
}

void msComputeBounds(shapeObj *shape)
{
  int i, j;

  if(shape->numlines <= 0) return;
  if(shape->line[0].numpoints <= 0) return;

  shape->bounds.minx = shape->bounds.maxx = shape->line[0].point[0].x;
  shape->bounds.miny = shape->bounds.maxy = shape->line[0].point[0].y;
    
  for( i=0; i<shape->numlines; i++ ) {
    for( j=0; j<shape->line[i].numpoints; j++ ) {
      shape->bounds.minx = MS_MIN(shape->bounds.minx, shape->line[i].point[j].x);
      shape->bounds.maxx = MS_MAX(shape->bounds.maxx, shape->line[i].point[j].x);
      shape->bounds.miny = MS_MIN(shape->bounds.miny, shape->line[i].point[j].y);
      shape->bounds.maxy = MS_MAX(shape->bounds.maxy, shape->line[i].point[j].y);
    }
  }
}

int msAddLine(shapeObj *p, lineObj *new_line)
{
  int c, v;
  lineObj *extended_line;

  /* Create an extended line array */
  if((extended_line = (lineObj *)malloc((p->numlines + 1) * sizeof(lineObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "msAddLine()");
    return(-1);
  }

  /* Copy the old line into the extended line array */
  for (c= 0; c < p->numlines; c++)
    extended_line[c]= p->line[c];

  /* Copy the new line onto the end of the extended line array */
  c= p->numlines;
  extended_line[c].numpoints = new_line->numpoints;  
  if((extended_line[c].point = (pointObj *)malloc(new_line->numpoints * sizeof(pointObj))) == NULL) {
    msSetError(MS_MEMERR, NULL, "msAddLine()");
    return(-1);
  }

  for (v= 0; v < new_line->numpoints; v++)
    extended_line[c].point[v]= new_line->point[v];

  /* Dispose of the old line */
  if(p->line) free(p->line);

  /* Update the polygon information */
  p->numlines++;
  p->line = extended_line;

  return(0);
}

/*
** Converts a rect array to a shapeObj structure. Note order is CW assuming y origin
** is in the lower left corner (normal cartesian coordinate system). Also polygon is
** is closed (i.e. first=last). This conforms to the shapefile specification. For image
** coordinate systems (i.e. GD) this is back-ass-ward, which is fine cause the function
** that calculates direction assumes min y = lower left, this way it'll still work. Drawing
** functions are independent of direction. Orientation problems can cause some nasty bugs.
*/
void msRectToPolygon(rectObj rect, shapeObj *poly)
{
  lineObj line={0,NULL};

  line.point = (pointObj *)malloc(sizeof(pointObj)*5);
  
  line.point[0].x = rect.minx;
  line.point[0].y = rect.miny;
  line.point[1].x = rect.minx;
  line.point[1].y = rect.maxy;
  line.point[2].x = rect.maxx;
  line.point[2].y = rect.maxy;
  line.point[3].x = rect.maxx;
  line.point[3].y = rect.miny;
  line.point[4].x = line.point[0].x;
  line.point[4].y = line.point[0].y;
  
  line.numpoints = 5;
  
  msAddLine(poly, &line);
  poly->type = MS_SHAPE_POLYGON;
  poly->bounds = rect;
  free(line.point);
}

/*
** Private implementation of the Sutherland-Cohen algorithm. Taken in part
** from "Getting Graphic: Programming Fundamentals in C and C++" by Mark Finlay
** and John Petritis. (pages 179-182)
*/
static int clipLine(double *x1, double *y1, double *x2, double *y2, rectObj rect)
{
  double x, y;
  double slope;
  CLIP_STATE check1, check2;

  if(*x1 < rect.minx && *x2 < rect.minx)
    return(MS_FALSE);
  if(*x1 > rect.maxx && *x2 > rect.maxx)
    return(MS_FALSE);

  check1 = CLIP_CHECK(rect.minx, *x1, rect.maxx);
  check2 = CLIP_CHECK(rect.minx, *x2, rect.maxx);
  if(check1 == CLIP_LEFT || check2 == CLIP_LEFT) {
    slope = (*y2 - *y1)/(*x2 - *x1);
    y = *y1 + (rect.minx - *x1)*slope;
    if(check1 == CLIP_LEFT) {
      *x1 = rect.minx;
      *y1 = y;
    } else {
      *x2 = rect.minx;
      *y2 = y;
    }
  }
  if(check1 == CLIP_RIGHT || check2 == CLIP_RIGHT) {
    slope = (*y2 - *y1)/(*x2 - *x1);
    y = *y1 + (rect.maxx - *x1)*slope;
    if(check1 == CLIP_RIGHT) {
      *x1 = rect.maxx;
      *y1 = y;
    } else {
      *x2 = rect.maxx;
      *y2 = y;
    }
  }

  if(*y1 < rect.miny && *y2 < rect.miny)
    return(MS_FALSE);
  if(*y1 > rect.maxy && *y2 > rect.maxy)
    return(MS_FALSE);

  check1 = CLIP_CHECK(rect.miny, *y1, rect.maxy);
  check2 = CLIP_CHECK(rect.miny, *y2, rect.maxy);
  if(check1 == CLIP_LEFT || check2 == CLIP_LEFT) {
    slope = (*x2 - *x1)/(*y2 - *y1);
    x = *x1 + (rect.miny - *y1)*slope;
    if(check1 == CLIP_LEFT) {
      *x1 = x;
      *y1 = rect.miny;
    } else {
      *x2 = x;
      *y2 = rect.miny;
    }
  }
  if(check1 == CLIP_RIGHT || check2 == CLIP_RIGHT) {
    slope = (*x2 - *x1)/(*y2 - *y1);
    x = *x1 + (rect.maxy - *y1)*slope;
    if(check1 == CLIP_RIGHT) {
      *x1 = x;
      *y1 = rect.maxy;
    } else {
      *x2 = x;
      *y2 = rect.maxy;
    }
  }

  return(MS_TRUE);
}

/*
** Routine for clipping a polyline, stored in a shapeObj struct, to a
** rectangle. Uses clipLine() function to create a new shapeObj. 
*/
void msClipPolylineRect(shapeObj *shape, rectObj rect)
{
  int i,j;
  lineObj line={0,NULL};
  double x1, x2, y1, y2;
  shapeObj tmp={0,NULL};

  if(shape->numlines == 0) /* nothing to clip */
    return;

  for(i=0; i<shape->numlines; i++) {

    line.point = (pointObj *)malloc(sizeof(pointObj)*shape->line[i].numpoints);
    line.numpoints = 0;

    x1 = shape->line[i].point[0].x;
    y1 = shape->line[i].point[0].y;
    for(j=1; j<shape->line[i].numpoints; j++) {
      x2 = shape->line[i].point[j].x;
      y2 = shape->line[i].point[j].y;

      if(clipLine(&x1,&y1,&x2,&y2,rect) == MS_TRUE) {
	if(line.numpoints == 0) { /* first segment, add both points */
	  line.point[0].x = x1;
	  line.point[0].y = y1;
	  line.point[1].x = x2;
	  line.point[1].y = y2;
	  line.numpoints = 2;
	} else { /* add just the last point */
	  line.point[line.numpoints].x = x2;
	  line.point[line.numpoints].y = y2;
	  line.numpoints++;	  
	}

	if((x2 != shape->line[i].point[j].x) || (y2 != shape->line[i].point[j].y)) {
	  msAddLine(&tmp, &line);
	  line.numpoints = 0; /* new line */
	}
      }

      x1 = shape->line[i].point[j].x;
      y1 = shape->line[i].point[j].y;
    }

    if(line.numpoints > 0)
      msAddLine(&tmp, &line);
    free(line.point);
    line.numpoints = 0; /* new line */
  }
  
  for (i=0; i<shape->numlines; i++) free(shape->line[i].point);
  free(shape->line);

  shape->line = tmp.line;
  shape->numlines = tmp.numlines;
}

/*
** Slightly modified version of the Liang-Barsky polygon clipping algorithm
*/
void msClipPolygonRect(shapeObj *shape, rectObj rect)
{
  int i, j;
  double deltax, deltay, xin,xout,  yin,yout;
  double tinx,tiny,  toutx,touty,  tin1, tin2,  tout;
  double x1,y1, x2,y2;

  shapeObj tmp;
  lineObj line={0,NULL};

  msInitShape(&tmp);
  
  if(shape->numlines == 0) /* nothing to clip */
    return;
   
  for(j=0; j<shape->numlines; j++) {

    line.point = (pointObj *)malloc(sizeof(pointObj)*2*shape->line[j].numpoints+1); /* worst case scenario, +1 allows us to duplicate the 1st and last point */
    line.numpoints = 0;

    for (i = 0; i < shape->line[j].numpoints-1; i++) {
      
      x1 = shape->line[j].point[i].x;
      y1 = shape->line[j].point[i].y;
      x2 = shape->line[j].point[i+1].x;
      y2 = shape->line[j].point[i+1].y;
      
      deltax = x2-x1;
      if (deltax == 0) { /* bump off of the vertical */
	deltax = (x1 > rect.minx) ? -NEARZERO : NEARZERO ;
      }
      deltay = y2-y1;
      if (deltay == 0) { /* bump off of the horizontal */
	deltay = (y1 > rect.miny) ? -NEARZERO : NEARZERO ;
      }
      
      if (deltax > 0) {		/*  points to right */
	xin = rect.minx;
	xout = rect.maxx;
      }
      else {
	xin = rect.maxx;
	xout = rect.minx;
      }
      if (deltay > 0) {		/*  points up */
	yin = rect.miny;
	yout = rect.maxy;
      }
      else {
	yin = rect.maxy;
	yout = rect.miny;
      }
      
      tinx = (xin - x1)/deltax;
      tiny = (yin - y1)/deltay;
      
      if (tinx < tiny) {	/* hits x first */
	tin1 = tinx;
	tin2 = tiny;
      } else {			/* hits y first */
	tin1 = tiny;
	tin2 = tinx;
      }
      
      if (1 >= tin1) {
	if (0 < tin1) {
	  line.point[line.numpoints].x = xin;
	  line.point[line.numpoints].y = yin;
	  line.numpoints++;
	}
	if (1 >= tin2) {
	  toutx = (xout - x1)/deltax;
	  touty = (yout - y1)/deltay;
	  
	  tout = (toutx < touty) ? toutx : touty ;
	  
	  if (0 < tin2 || 0 < tout) {
	    if (tin2 <= tout) {
	      if (0 < tin2) {
		if (tinx > tiny) {
		  line.point[line.numpoints].x = xin;
		  line.point[line.numpoints].y = y1 + tinx*deltay;
		  line.numpoints++;
		} else {
		  line.point[line.numpoints].x = x1 + tiny*deltax;
		  line.point[line.numpoints].y = yin;
		  line.numpoints++;
		}
	      }
	      if (1 > tout) {
		if (toutx < touty) {
		  line.point[line.numpoints].x = xout;
		  line.point[line.numpoints].y = y1 + toutx*deltay;
		  line.numpoints++;
		} else {
		  line.point[line.numpoints].x = x1 + touty*deltax;
		  line.point[line.numpoints].y = yout;
		  line.numpoints++;
		}
	      }	else {
		line.point[line.numpoints].x = x2;
		line.point[line.numpoints].y = y2;
		line.numpoints++;
	      }
	    } else {
	      if (tinx > tiny) {
		line.point[line.numpoints].x = xin;
		line.point[line.numpoints].y = yout;
		line.numpoints++;
	      } else {
		line.point[line.numpoints].x = xout;
		line.point[line.numpoints].y = yin;
		line.numpoints++;
	      }
	    }
	  }
	}
      }
    }

    if(line.numpoints > 0) {
      line.point[line.numpoints].x = line.point[0].x; // force closure
      line.point[line.numpoints].y = line.point[0].y;
      line.numpoints++;
      msAddLine(&tmp, &line);
    }

    free(line.point);
  } /* next line */
  
  for (i=0; i<shape->numlines; i++) free(shape->line[i].point);
  free(shape->line);

  shape->line = tmp.line;
  shape->numlines = tmp.numlines;

  return;
}

/*
** Converts from map coordinates to image coordinates
*/
void msTransformShapeToPixel(shapeObj *shape, rectObj extent, double cellsize)
{
  int i,j,k; /* loop counters */


  if(shape->numlines == 0) return; // nothing to transform

  if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON) { // remove co-linear vertices
  
    for(i=0; i<shape->numlines; i++) { // for each part
      
      shape->line[i].point[0].x = MS_MAP2IMAGE_X(shape->line[i].point[0].x, extent.minx, cellsize);
      shape->line[i].point[0].y = MS_MAP2IMAGE_Y(shape->line[i].point[0].y, extent.maxy, cellsize);
      
      for(j=1, k=1; j < shape->line[i].numpoints; j++ ) {
	
	shape->line[i].point[k].x = MS_MAP2IMAGE_X(shape->line[i].point[j].x, extent.minx, cellsize);
	shape->line[i].point[k].y = MS_MAP2IMAGE_Y(shape->line[i].point[j].y, extent.maxy, cellsize);

	if(k == 1) {
	  if((shape->line[i].point[0].x != shape->line[i].point[1].x) || (shape->line[i].point[0].y != shape->line[i].point[1].y))
	    k++;
	} else {
	  if((shape->line[i].point[k-1].x != shape->line[i].point[k].x) || (shape->line[i].point[k-1].y != shape->line[i].point[k].y)) {
	    if(((shape->line[i].point[k-2].y - shape->line[i].point[k-1].y)*(shape->line[i].point[k-1].x - shape->line[i].point[k].x)) == ((shape->line[i].point[k-2].x - shape->line[i].point[k-1].x)*(shape->line[i].point[k-1].y - shape->line[i].point[k].y))) {	    
	      shape->line[i].point[k-1].x = shape->line[i].point[k].x;
	      shape->line[i].point[k-1].y = shape->line[i].point[k].y;	
	    } else {
	      k++;
	    }
	  }
	}
      }
      shape->line[i].numpoints = k; // save actual number kept
    }
  } else { // points or untyped shapes
    for(i=0; i<shape->numlines; i++) { // for each part
      for(j=1; j < shape->line[i].numpoints; j++ ) {
	shape->line[i].point[j].x = MS_MAP2IMAGE_X(shape->line[i].point[j].x, extent.minx, cellsize);
	shape->line[i].point[j].y = MS_MAP2IMAGE_Y(shape->line[i].point[j].y, extent.maxy, cellsize);
	  }
	}
  }
}
/*
** Converts from map coordinates to image coordinates
*/
void msTransformPixelToShape(shapeObj *shape, rectObj extent, double cellsize)
{
	int i,j; /* loop counters */

	if(shape->numlines == 0) return; // nothing to transform

	if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON)  // remove co-linear vertices
	{
		for(i=0; i<shape->numlines; i++)  // for each part
		{ 
			for(j=0; j < shape->line[i].numpoints; j++ ) 
			{
				shape->line[i].point[j].x = MS_IMAGE2MAP_X(shape->line[i].point[j].x, extent.minx, cellsize);
				shape->line[i].point[j].y = MS_IMAGE2MAP_Y(shape->line[i].point[j].y, extent.maxy, cellsize);
			}
		}
	}
	else  // points or untyped shapes
	{
		for(i=0; i<shape->numlines; i++)  // for each part
		{
			for(j=1; j < shape->line[i].numpoints; j++ ) 
			{
				shape->line[i].point[j].x = MS_IMAGE2MAP_X(shape->line[i].point[j].x, extent.minx, cellsize);
				shape->line[i].point[j].y = MS_IMAGE2MAP_Y(shape->line[i].point[j].y, extent.maxy, cellsize);
			}
		}
	}

	return;
}

void msImageScanline(gdImagePtr im, int x1, int x2, int y, int c)
{
  int x;

  // TO DO: This fix (adding if/then) was to address circumstances in the polygon fill code
  // where x2 < x1. There may be something wrong in that code, but at any rate this is safer.

  if(x1 < x2)
    for(x=x1; x<=x2; x++) gdImageSetPixel(im, x, y, c);
  else
    for(x=x2; x<=x1; x++) gdImageSetPixel(im, x, y, c);
}

/*
** Not a generic intersection test, we KNOW the lines aren't parallel or coincident. To be used with the next
** buffering code only. See code in mapsearch.c for a boolean test for intersection.
*/
static pointObj generateLineIntersection(pointObj a, pointObj b, pointObj c, pointObj d) 
{
  pointObj p;
  double r;
  double denominator, numerator;

  if(b.x == c.x && b.y == c.y) return(b);

  numerator = ((a.y-c.y)*(d.x-c.x) - (a.x-c.x)*(d.y-c.y));  
  denominator = ((b.x-a.x)*(d.y-c.y) - (b.y-a.y)*(d.x-c.x));  

  r = numerator/denominator;

  p.x = MS_NINT(a.x + r*(b.x-a.x));
  p.y = MS_NINT(a.y + r*(b.y-a.y));

  return(p);
}

void bufferPolyline(shapeObj *p, shapeObj *op, int w)
{
  int i, j;
  pointObj a;
  lineObj inside, outside;
  double angle;
  double dx, dy;  

  for (i = 0; i < p->numlines; i++) {

    inside.point = (pointObj *)malloc(sizeof(pointObj)*p->line[i].numpoints);
    outside.point = (pointObj *)malloc(sizeof(pointObj)*p->line[i].numpoints);
    inside.numpoints = outside.numpoints = p->line[i].numpoints;    

    angle = asin(MS_ABS(p->line[i].point[1].x - p->line[i].point[0].x)/sqrt((pow((p->line[i].point[1].x - p->line[i].point[0].x),2) + pow((p->line[i].point[1].y - p->line[i].point[0].y),2))));
    if(p->line[i].point[0].x < p->line[i].point[1].x)
      dy = sin(angle) * (w/2);
    else
      dy = -sin(angle) * (w/2);
    if(p->line[i].point[0].y < p->line[i].point[1].y)
      dx = -cos(angle) * (w/2);
    else
      dx = cos(angle) * (w/2);

    inside.point[0].x = p->line[i].point[0].x + dx;
    inside.point[1].x = p->line[i].point[1].x + dx;
    inside.point[0].y = p->line[i].point[0].y + dy;
    inside.point[1].y = p->line[i].point[1].y + dy;
    
    outside.point[0].x = p->line[i].point[0].x - dx;
    outside.point[1].x = p->line[i].point[1].x - dx;
    outside.point[0].y = p->line[i].point[0].y - dy;
    outside.point[1].y = p->line[i].point[1].y - dy;

    for(j=2; j<p->line[i].numpoints; j++) {

      angle = asin(MS_ABS(p->line[i].point[j].x - p->line[i].point[j-1].x)/sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2))));
      if(p->line[i].point[j-1].x < p->line[i].point[j].x)
	dy = sin(angle) * (w/2);
      else
	dy = -sin(angle) * (w/2);
      if(p->line[i].point[j-1].y < p->line[i].point[j].y)
	dx = -cos(angle) * (w/2);
      else
        dx = cos(angle) * (w/2);

      a.x = p->line[i].point[j-1].x + dx;
      inside.point[j].x = p->line[i].point[j].x + dx;
      a.y = p->line[i].point[j-1].y + dy;
      inside.point[j].y = p->line[i].point[j].y + dy;
      inside.point[j-1] = generateLineIntersection(inside.point[j-2], inside.point[j-1], a, inside.point[j]);      

      a.x = p->line[i].point[j-1].x - dx;
      outside.point[j].x = p->line[i].point[j].x - dx;
      a.y = p->line[i].point[j-1].y - dy;
      outside.point[j].y = p->line[i].point[j].y - dy;
      outside.point[j-1] = generateLineIntersection(outside.point[j-2], outside.point[j-1], a, outside.point[j]);
    }

    // need a touch of code if 1st point equals last point in p (find intersection)

    msAddLine(op, &inside);
    msAddLine(op, &outside);

    free(inside.point);
    free(outside.point);
  }
  
  return;
}

void msImageFilledCircle(gdImagePtr im, pointObj *p, int r, int c)
{
  int y;
  int ymin, ymax, xmin, xmax;
  double dx, dy;

  ymin = MS_MAX((p->y - r), 0);
  ymax = MS_MIN((p->y + r), (gdImageSY(im)-1));

  for(y=ymin; y<=ymax; y++) {
    dy = MS_ABS(p->y - y);
    dx = sqrt((r*r) - (dy*dy));

    xmin = MS_MAX((p->x - dx), 0);
    xmax = MS_MIN((p->x + dx), (gdImageSX(im)-1));

    msImageScanline(im, xmin, xmax, y, c);
  }

  return;
}

void msImageFilledPolygon(gdImagePtr im, shapeObj *p, int c)
{
  float *slope;
  pointObj *point1, *point2, *testpoint1, *testpoint2;
  int i, j, k, l, m, nfound, *xintersect, temp, sign;
  int x, y, ymin, ymax, *horiz, wrong_order;
  int n;

  if(p->numlines == 0) return;

#if 0
  if( c & 0xFF000000 )
	gdImageAlphaBlending( im, 1 );
#endif
  
  /* calculate the total number of vertices */
  n=0;
  for(i=0; i<p->numlines; i++)
    n += p->line[i].numpoints;

  /* Allocate slope and horizontal detection arrays */
  slope = (float *)calloc(n, sizeof(float));
  horiz = (int *)calloc(n, sizeof(int));
  
  /* Since at most only one intersection is added per edge, there can only be at most n intersections per scanline */
  xintersect = (int *)calloc(n, sizeof(int));
  
  /* Find the min and max Y */
  ymin = p->line[0].point[0].y;
  ymax = ymin;
  
  for(l=0,j=0; j<p->numlines; j++) {
    point1 = &( p->line[j].point[p->line[j].numpoints-1] );
    for(i=0; i < p->line[j].numpoints; i++,l++) {
      point2 = &( p->line[j].point[i] );
      if(point1->y == point2->y) {
	horiz[l] = 1;
	slope[l] = 0.0;
      } else {
	horiz[l] = 0;
	slope[l] = (float) (point2->x - point1->x) / (point2->y - point1->y);
      }
      ymin = MS_MIN(ymin, point1->y);
      ymax = MS_MAX(ymax, point2->y);
      point1 = point2;
    }
  }  

  for(y = ymin; y <= ymax; y++) { /* for each scanline */

    nfound = 0;
    for(j=0, l=0; j<p->numlines; j++) { /* for each line, l is overall point counter */

      m = l; /* m is offset from begining of all vertices */
      point1 = &( p->line[j].point[p->line[j].numpoints-1] );
      for(i=0; i < p->line[j].numpoints; i++, l++) {
	point2 = &( p->line[j].point[i] );
	if(EDGE_CHECK(point1->y, y, point2->y) == CLIP_MIDDLE) {
	  
	  if(horiz[l]) /* First, is this edge horizontal ? */
	    continue;

	  /* Did we intersect the first point point ? */
	  if(y == point1->y) {
	    /* Yes, must find first non-horizontal edge */
	    k = i-1;
	    if(k < 0) k = p->line[j].numpoints-1;
	    while(horiz[m+k]) {
	      k--;
	      if(k < 0) k = p->line[j].numpoints-1;
	    }
	    /* Now perform sign test */
	    if(k > 0)
	      testpoint1 = &( p->line[j].point[k-1] );
	    else
	      testpoint1 = &( p->line[j].point[p->line[j].numpoints-1] );
	    testpoint2 = &( p->line[j].point[k] );
	    sign = (testpoint2->y - testpoint1->y) *
	      (point2->y - point1->y);
	    if(sign < 0)
	      xintersect[nfound++] = point1->x;
	    /* All done for point matching case */
	  } else {  
	    /* Not at the first point,
	       find the intersection*/
	    x = ROUND(point1->x + (y - point1->y)*slope[l]);
	    xintersect[nfound++] = x;
	  }
	}                 /* End of checking this edge */
	
	point1 = point2;  /* Go on to next edge */
      }
    } /* Finished this scanline, draw all of the spans */
    
    /* First, sort the intersections */
    do {
      wrong_order = 0;
      for(i=0; i < nfound-1; i++) {
	if(xintersect[i] > xintersect[i+1]) {
	  wrong_order = 1;
	  SWAP(xintersect[i], xintersect[i+1], temp);
	}
      }
    } while(wrong_order);
    
    /* Great, now we can draw the spans */
    for(i=0; i < nfound; i += 2)
      msImageScanline(im, xintersect[i], xintersect[i+1], y, c);
  } /* End of scanline loop */
  
  /* Finally, draw all of the horizontal edges */
  for(j=0, l=0; j<p->numlines; j++) {
    point1 = &( p->line[j].point[p->line[j].numpoints - 1] );
    for(i=0; i<p->line[j].numpoints; i++, l++) {
      point2 = &( p->line[j].point[i] );
      if(horiz[l])
	msImageScanline(im, point1->x, point2->x, point2->y, c);
      point1 = point2;
    }
  }
  
#if 0
   gdImageAlphaBlending( im, 0 );
#endif

  free(slope);
  free(horiz);
  free(xintersect);
}

static double length(pointObj a, pointObj b)
{
  double d;

  d = sqrt((pow((a.x-b.x),2) + pow((a.y-b.y),2)));
  return(d);
}

// Currently unused.
#ifdef notdef
static int get_centroid(shapeObj *p, pointObj *lp, double *miny, double *maxy)
{
  int i,j;
  double cent_weight_x=0.0, cent_weight_y=0.0;
  double len, total_len=0;

  *miny = *maxy = p->line[0].point[0].y;
  for(i=0; i<p->numlines; i++) {
    for(j=1; j<p->line[i].numpoints; j++) {
      *miny = MS_MIN(*miny, p->line[i].point[j].y);
      *maxy = MS_MAX(*maxy, p->line[i].point[j].y);
      len = length(p->line[i].point[j-1], p->line[i].point[j]);
      cent_weight_x += len * ((p->line[i].point[j-1].x + p->line[i].point[j].x)/2);
      cent_weight_y += len * ((p->line[i].point[j-1].y + p->line[i].point[j].y)/2);
      total_len += len;
    }
  }

  if(total_len == 0)
    return(-1);

  lp->x = cent_weight_x / total_len;
  lp->y = cent_weight_y / total_len;
  
  return(0);
}
#endif

static void get_bbox(shapeObj *poly, double *minx, double *miny, double *maxx, double *maxy) {
  int i, j;

  *minx = *maxx = poly->line[0].point[0].x;
  *miny = *maxy = poly->line[0].point[0].y;
  for(i=0; i<poly->numlines; i++) {
    for(j=1; j<poly->line[i].numpoints; j++) {
      *minx = MS_MIN(*minx, poly->line[i].point[j].x);
      *maxx = MS_MAX(*maxx, poly->line[i].point[j].x);
      *miny = MS_MIN(*miny, poly->line[i].point[j].y);
      *maxy = MS_MAX(*maxy, poly->line[i].point[j].y);
    }
  }

  return;
}

#define NUM_SCANLINES 5

/*
** Find a label point in a polygon.
*/
int msPolygonLabelPoint(shapeObj *p, pointObj *lp, int min_dimension)
{
  double slope;
  pointObj *point1=NULL, *point2=NULL;
  int i, j, k, nfound;
  double x, y, *xintersect, temp;
  double hi_y, lo_y;
  int wrong_order, n;
  double len, max_len=0;
  double skip, minx, maxx, maxy, miny;

  get_bbox(p, &minx, &miny, &maxx, &maxy);

  if(min_dimension != -1)
    if(MS_MIN(maxx-minx,maxy-miny) < min_dimension) return(MS_FAILURE);

  //if(get_centroid(p, lp, &miny, &maxy) == -1) return(MS_FAILURE);
  lp->x = (maxx+minx)/2.0;
  lp->y = (maxy+miny)/2.0;

  if(msIntersectPointPolygon(lp, p) == MS_TRUE) return(MS_SUCCESS);

  /* do it the hard way - scanline */

  skip = (maxy - miny)/NUM_SCANLINES;

  n=0;
  for(j=0; j<p->numlines; j++) /* count total number of points */
    n += p->line[j].numpoints;
  xintersect = (double *)calloc(n, sizeof(double));

  for(k=1; k<=NUM_SCANLINES; k++) { /* sample the shape in the y direction */
    
    y = maxy - k*skip; 

    /* need to find a y that won't intersect any vertices exactly */  
    hi_y = y - 1; /* first initializing lo_y, hi_y to be any 2 pnts on either side of lp->y */
    lo_y = y + 1;
    for(j=0; j<p->numlines; j++) {
      if((lo_y < y) && (hi_y >= y)) 
	break; /* already initialized */
      for(i=0; i < p->line[j].numpoints; i++) {
	if((lo_y < y) && (hi_y >= y)) 
	  break; /* already initialized */
	if(p->line[j].point[i].y < y)
	  lo_y = p->line[j].point[i].y;
	if(p->line[j].point[i].y >= y)
	  hi_y = p->line[j].point[i].y;
      }
    }

    n=0;
    for(j=0; j<p->numlines; j++) {
      for(i=0; i < p->line[j].numpoints; i++) {
	if((p->line[j].point[i].y < y) && ((y - p->line[j].point[i].y) < (y - lo_y)))
	  lo_y = p->line[j].point[i].y;
	if((p->line[j].point[i].y >= y) && ((p->line[j].point[i].y - y) < (hi_y - y)))
	  hi_y = p->line[j].point[i].y;
      }      
    }

    if(lo_y == hi_y) 
      return (MS_FAILURE);
    else  
      y = (hi_y + lo_y)/2.0;    
    
    nfound = 0;
    for(j=0; j<p->numlines; j++) { /* for each line */
      
      point1 = &( p->line[j].point[p->line[j].numpoints-1] );
      for(i=0; i < p->line[j].numpoints; i++) {
	point2 = &( p->line[j].point[i] );
	
	if(EDGE_CHECK(point1->y, y, point2->y) == CLIP_MIDDLE) {
	  
	  if(point1->y == point2->y)
	    continue; /* ignore horizontal edges */
	  else	  
	    slope = (point2->x - point1->x) / (point2->y - point1->y);
	  
	  x = point1->x + (y - point1->y)*slope;
	  xintersect[nfound++] = x;
	} /* End of checking this edge */
	
	point1 = point2;  /* Go on to next edge */
      }
    } /* Finished the scanline */
    
    /* First, sort the intersections */
    do {
      wrong_order = 0;
      for(i=0; i < nfound-1; i++) {
	if(xintersect[i] > xintersect[i+1]) {
	  wrong_order = 1;
	  SWAP(xintersect[i], xintersect[i+1], temp);
	}
      }
    } while(wrong_order);
    
    /* Great, now find longest span */
    point1->y = point2->y = y;
    for(i=0; i < nfound; i += 2) {
      point1->x = xintersect[i];
      point2->x = xintersect[i+1];
      len = length(*point1, *point2);
      if(len > max_len) {
	max_len = len;
	lp->x = (point1->x + point2->x)/2;
	lp->y = y;
      }
    }
  }

  free(xintersect);

  if(max_len > 0)
    return(MS_SUCCESS);
  else
    return(MS_FAILURE);
}

/*
** Find center of longest segment in polyline p. The polyline must have been converted
** to image coordinates before calling this function.
*/
int msPolylineLabelPoint(shapeObj *p, pointObj *lp, int min_length, double *angle, double *length)
{
  double segment_length, line_length, total_length, max_segment_length, max_line_length;
  int segment_index, line_index, temp_segment_index;
  int i, j;
  double theta;
  
  temp_segment_index = segment_index = line_index = 0;

  total_length = 0;
  max_line_length = 0;
  for(i=0; i<p->numlines; i++) {

    line_length = 0;
    max_segment_length = 0;
    for(j=1;j<p->line[i].numpoints;j++) {
      segment_length = sqrt((pow((p->line[i].point[j].x-p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y-p->line[i].point[j-1].y),2)));
      line_length += segment_length;
      if(segment_length > max_segment_length) {
	max_segment_length = segment_length;
	temp_segment_index = j;
      }
    }

    total_length += line_length;

    if(line_length > max_line_length) {
      max_line_length = line_length;
      line_index = i;
      segment_index = temp_segment_index;
    }
  }

  if(segment_index == 0) /* must have a degenerate line, skip it */
    return(MS_FAILURE);

  if((min_length != -1) && (total_length < min_length)) /* too short to label */
    return(MS_FAILURE);

  // ok, now we know which line and which segment within that line
  i = line_index;
  j = segment_index;

  *length = total_length;

  lp->x = (p->line[i].point[j].x + p->line[i].point[j-1].x)/2.0;
  lp->y = (p->line[i].point[j].y + p->line[i].point[j-1].y)/2.0;
 
  theta = asin(MS_ABS(p->line[i].point[j].x - p->line[i].point[j-1].x)/sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2))));

  if(p->line[i].point[j-1].x < p->line[i].point[j].x) { /* i.e. to the left */
    if(p->line[i].point[j-1].y < p->line[i].point[j].y) /* i.e. below */
      *angle = -(90.0 - MS_RAD_TO_DEG*theta);
    else
      *angle = (90.0 - MS_RAD_TO_DEG*theta);      
  } else {
    if(p->line[i].point[j-1].y < p->line[i].point[j].y) /* i.e. below */
      *angle = (90.0 - MS_RAD_TO_DEG*theta);
    else
      *angle = -(90.0 - MS_RAD_TO_DEG*theta);      
  }

  return(MS_SUCCESS);
}

static gdPoint generateGDLineIntersection(gdPoint a, gdPoint b, gdPoint c, gdPoint d) 
{
  gdPoint p;
  double r;
  double denominator, numerator;

  if(b.x == c.x && b.y == c.y) return(b);

  numerator = ((a.y-c.y)*(d.x-c.x) - (a.x-c.x)*(d.y-c.y));  
  denominator = ((b.x-a.x)*(d.y-c.y) - (b.y-a.y)*(d.x-c.x));  

  r = numerator/denominator;

  p.x = MS_NINT(a.x + r*(b.x-a.x));
  p.y = MS_NINT(a.y + r*(b.y-a.y));

  return(p);
}

static double getAngleFromDxDy(double dx, double dy) 
{
  double angle;
  if (!dx) {
    if (dy > 0) 
      angle = MS_PI2;
    else
      angle = MS_3PI2;
  } else { 
    angle = atan(dy/dx);
    if (dx < 0) 
      angle += MS_PI;
    else if (dx > 0 && dy < 0) 
      angle += MS_2PI;
  }
  return(angle);
}

static void imageFilledSegment(gdImagePtr im, double x, double y, double sz, double angle_from, double angle_to, int c) 
{
  int j=0;
  double angle, dx, dy;
  static gdPoint points[38];
  double angle_dif;

  sz -= 0.1;
  if      (sz < 4)  angle_dif = MS_PI2;
  else if (sz < 12) angle_dif = MS_PI/5;
  else if (sz < 20) angle_dif = MS_PI2/7;
  else              angle_dif = MS_PI2/10;
  
  angle = angle_from;
  while(angle < angle_to) {
    dx = cos(angle)*sz;
    dy = sin(angle)*sz;
    points[j].x = MS_NINT(x + dx);
    points[j].y = MS_NINT(y + dy);
    angle += angle_dif;
    j++;
  }
  
  if (j) {
    dx = cos(angle_to)*sz;
    dy = sin(angle_to)*sz;
    points[j].x = MS_NINT(x + dx);
    points[j].y = MS_NINT(y + dy);
    j++;
    points[j].x = MS_NINT(x);
    points[j].y = MS_NINT(y);
    j++;
    points[j].x = points[0].x;
    points[j].y = points[0].y;
    j++;
  
    gdImagePolygon(im, points, j, c);
    gdImageFilledPolygon(im, points, j, c);
  }
}

/* ------------------------------------------------------------------------------- */
/*    Draw a cartographic line symbol of the specified size, color, join and cap   */
/* ------------------------------------------------------------------------------- */
static void RenderCartoLine(gdImagePtr img, int gox, double *acoord, double *bcoord, double da, double db, double angle_n, double size, int c, symbolObj *symbol, double styleCoef, double *styleSize, int *styleIndex, int *styleVis, gdPoint *points, double *da_px, double *db_px, int offseta, int offsetb) 
{
  double an, bn, da_pxn, db_pxn, d_size, a, b;
  double d_step_coef, da_pxn_coef, db_pxn_coef, da_px_coef, db_px_coef;
  int db_line, s, first, tan1;

  /* Steps for one pixel */
  *da_px = da/MS_ABS(db);
  *db_px = MS_SGN(db);
  if (gox) { 
    da_pxn = *db_px*sin(angle_n);
    db_pxn = *db_px*cos(angle_n);
  } else {
    da_pxn = *db_px*cos(angle_n);
    db_pxn = *db_px*sin(angle_n);
  }

  if (!da || !db || (size == 1)) {
    d_step_coef = 1;
  } else {
    d_step_coef = 0.5;
    size -= d_step_coef;
  }
  da *= 1/d_step_coef;
  db *= 1/d_step_coef;

  da_px_coef = *da_px*d_step_coef;
  db_px_coef = *db_px*d_step_coef;
  da_pxn_coef = da_pxn*d_step_coef;
  db_pxn_coef = db_pxn*d_step_coef;
  
  if(offseta != 0 || offsetb != 0) {
    if (gox) { 
      if(offseta == -99) { // old-style offset (version 3.3 and earlier)
        *acoord += MS_SGN(db)*MS_ABS(da_pxn)*offsetb;
        *bcoord -= MS_SGN(da)*MS_ABS(db_pxn)*offsetb;
      } else { // normal offset (eg. drop shadow)
        *acoord += offseta;
        *bcoord += offsetb;
      }
    } else {
      if(offsetb == -99) { // old-style offset (version 3.3 and earlier)
        *acoord -= MS_SGN(db)*MS_ABS(da_pxn)*offseta;
        *bcoord += MS_SGN(da)*MS_ABS(db_pxn)*offseta;
      } else { // normal offset (eg. drop shadow)
        *acoord += offseta;
        *bcoord += offsetb;
      }
    }
  }
  
  a = *acoord;
  b = *bcoord; 

  /* Tan 1 correction */
  tan1 = 0;
  if (size > 1 && MS_ABS(MS_ABS(da/db) - 1) < 0.4) {
    size -= 0.5;
    tan1 = 1;
  }
  
  /* Style counting unit */
  d_size = sqrt(pow(*da_px,2)+pow(*db_px,2))*d_step_coef;
  
  /* Line rendering */
  first = 1;
  for (db_line = -1*d_step_coef; db_line++ < MS_ABS(db); b += db_px_coef) {
  
    /* Set the start of the first pixel */
    an = a - da_pxn*(size-1)/2;
    bn = b - db_pxn*(size-1)/2;
  
    /* Tan 1 correction */
    if (tan1) {
      an -= da_pxn*d_step_coef;
      bn += db_pxn*d_step_coef;
    }
    
    /* Save corners */
    if (first) {
      if (gox) {
        if (MS_SGN(db) > 0) {
          points[0].x = MS_NINT(bn);
          points[0].y = MS_NINT(an);
          points[1].x = MS_NINT(bn+db_pxn*size);
          points[1].y = MS_NINT(an+da_pxn*size);
        } else {
          points[1].x = MS_NINT(bn);
          points[1].y = MS_NINT(an);
          points[0].x = MS_NINT(bn+db_pxn*size);
          points[0].y = MS_NINT(an+da_pxn*size);
        }
      } else {
        if (MS_SGN(db) > 0) {
          points[0].x = MS_NINT(an);
          points[0].y = MS_NINT(bn);
          points[1].x = MS_NINT(an+da_pxn*size);
          points[1].y = MS_NINT(bn+db_pxn*size);
        } else {
          points[1].x = MS_NINT(an);
          points[1].y = MS_NINT(bn);
          points[0].x = MS_NINT(an+da_pxn*size);
          points[0].y = MS_NINT(bn+db_pxn*size);
        }
      }
    }
    first = 0;

    /* Style counting */
    if (symbol->stylelength > 0) {
      if (*styleIndex == symbol->stylelength) 
        *styleIndex = 0;
      *styleSize -= d_size;    
      if (*styleSize < 0) {
        *styleSize = symbol->style[*styleIndex]*styleCoef;
        *styleSize -= d_size;    
        (*styleIndex)++;
        *styleVis = *styleVis?0:1; 
      }
      
      /* Skip spaces */
      if (! *styleVis) {
        a += da_px_coef;
        continue;
      }
    } else {
      *styleVis = 1;
    }  

    /* Walk by one pixel steps */ 
    for (s = 1; s <= size/d_step_coef; s++) {

      if (gox) gdImageSetPixel(img, MS_NINT(bn), MS_NINT(an), c);
          else gdImageSetPixel(img, MS_NINT(an), MS_NINT(bn), c);

      an += da_pxn_coef;
      bn += db_pxn_coef;
          
    } 
    a += da_px_coef;

  }                   

  /* Set the start of the first pixel */
  an = a - da_pxn*(size-1)/2;
  bn = b - db_pxn*(size-1)/2;

  /* Tan 1 correction */
  if (tan1) {
    an -= da_pxn*d_step_coef;
    bn += db_pxn*d_step_coef;
  }
  
  /* Save next corners */
  if (gox) {
    if (MS_SGN(db) > 0) {
      points[3].x = MS_NINT(bn);
      points[3].y = MS_NINT(an);
      points[2].x = MS_NINT(bn+db_pxn*size);
      points[2].y = MS_NINT(an+da_pxn*size);
    } else {
      points[2].x = MS_NINT(bn);
      points[2].y = MS_NINT(an);
      points[3].x = MS_NINT(bn+db_pxn*size);
      points[3].y = MS_NINT(an+da_pxn*size);
    }
  } else {
    if (MS_SGN(db) > 0) {
      points[3].x = MS_NINT(an);
      points[3].y = MS_NINT(bn);
      points[2].x = MS_NINT(an+da_pxn*size);
      points[2].y = MS_NINT(bn+db_pxn*size);
    } else {
      points[2].x = MS_NINT(an);
      points[2].y = MS_NINT(bn);
      points[3].x = MS_NINT(an+da_pxn*size);
      points[3].y = MS_NINT(bn+db_pxn*size);
    }
  }
  
}

/* ------------------------------------------------------------------------------- */
/*    Draw a cartographic line symbol of the specified size, color, join and cap   */
/* ------------------------------------------------------------------------------- */
void msImageCartographicPolyline(gdImagePtr img, shapeObj *p, styleObj *style, symbolObj *symbol, int c, double size, double scalefactor)
{
  int i, j, k;
  double x, y, dx, dy, s, maxs, angle, angle_n, last_angle=0, angle_left_limit, angle_from, angle_to;

  int styleIndex, styleVis, last_styleVis=0;
  double styleSize, styleCoef, dy_px, dx_px, size_2;

//  char buffer[256];

  gdPoint points[4], last_points[4];
  gdPoint cap_join_points[6];

  /* Style settings */
  if (symbol->stylelength > 0) {   
    styleVis = 1;
    styleIndex = -1;

//    if (symbol->stylesize)
//      styleCoef = size/symbol->stylesize;
//    else
//      styleCoef = 1;
    styleCoef = size/style->size;
    styleSize = -1;
  }

  /* Draw lines */
  for (i = 0; i < p->numlines; i++) {
    for(j=1; j<p->line[i].numpoints; j++) {
      
      x = p->line[i].point[j-1].x;
      y = p->line[i].point[j-1].y;
      
      /* dx, dy */
      dx = p->line[i].point[j].x - x;
      dy = p->line[i].point[j].y - y;
      if (!dx && !dy) continue;
      
      /* A line angle */
      angle = getAngleFromDxDy(dx, dy);

      /* Normal */
      angle_n = angle - MS_PI2;
      
      /* Steps for one pixel */
      if ((MS_ABS(dx) > MS_ABS(dy)) || (dx==dy)) {
        RenderCartoLine(img, 1, &y, &x, dy, dx, angle_n, size, c, symbol, styleCoef, &styleSize, &styleIndex, &styleVis, points, &dy_px, &dx_px, style->offsety, style->offsetx);      
      } else {
        RenderCartoLine(img, 0, &x, &y, dx, dy, angle_n, size, c, symbol, styleCoef, &styleSize, &styleIndex, &styleVis, points, &dx_px, &dy_px, style->offsetx, style->offsety);      
      }

//      gdImageSetPixel(img, MS_NINT(points[0].x), MS_NINT(points[0].y), 2);

//      gdImageLine(img, p->line[i].point[j-1].x, p->line[i].point[j-1].y, p->line[i].point[j].x, p->line[i].point[j].y, 3);  

//sprintf(buffer, "LastVis: %d, angle: %g", last_styleVis, angle);
//gdImageString(img, gdFontSmall, 10, 10, buffer, 1);

      /* Jointype */
//      if ((j > 1) && (angle != last_angle) && (last_styleVis)) {
      if ((size > 3) && (j > 1) && (angle != last_angle)) {

        angle_left_limit = last_angle - MS_PI;

        switch (symbol->linejoin) {
          /* Miter */
          case MS_CJC_NONE:
            /* do nothing */  
          break;
          /* Round */
          case MS_CJC_ROUND:
            /* The current line is on the left site of the last line */ 
            if ((angle_left_limit > 0 && angle_left_limit < angle && angle < last_angle) ||
                (angle_left_limit < 0 && ((0 <= angle && angle < last_angle) || (angle_left_limit+MS_2PI < angle)))) {
              angle_from = angle_n  - MS_PI;  
              angle_to = last_angle - MS_PI2 - MS_PI;
            /* The current line is on the right site of the last line */ 
            } else {
              angle_from = last_angle - MS_PI2;  
              angle_to = angle_n;
            }
            if (angle_from > angle_to)
              angle_from -= MS_2PI;

            angle_from -= MS_DEG_TO_RAD*2;
            angle_to   += MS_DEG_TO_RAD*2;
            
            imageFilledSegment(img, x, y, (size-1)/2, angle_from, angle_to, c);
          break;

          /* Miter */
          case MS_CJC_MITER:
          /* Bevel */
          case MS_CJC_BEVEL:

            /* The current line is on the left site of the last line */ 
            if ((angle_left_limit > 0 && angle_left_limit < angle && angle < last_angle) ||
                (angle_left_limit < 0 && ((0 <= angle && angle < last_angle) || (angle_left_limit+MS_2PI < angle)))) {
              cap_join_points[0].x = MS_NINT(last_points[3].x); 
              cap_join_points[0].y = MS_NINT(last_points[3].y); 
              cap_join_points[2].x = MS_NINT(points[0].x); 
              cap_join_points[2].y = MS_NINT(points[0].y); 
              if ((angle == last_angle + MS_PI) || (angle == last_angle - MS_PI)) {
                cap_join_points[1].x = MS_NINT(x)+dx_px*size*symbol->linejoinmaxsize;
                cap_join_points[1].y = MS_NINT(y)+dy_px*size*symbol->linejoinmaxsize;
              } else { 
                cap_join_points[1] = generateGDLineIntersection(last_points[0], last_points[3], points[0], points[3]);
              } 
            /* The current line is on the right site of the last line */ 
            } else {
              cap_join_points[0].x = MS_NINT(last_points[2].x); 
              cap_join_points[0].y = MS_NINT(last_points[2].y); 
              cap_join_points[2].x = MS_NINT(points[1].x); 
              cap_join_points[2].y = MS_NINT(points[1].y); 
              if ((angle == last_angle + MS_PI) || (angle == last_angle - MS_PI)) {
                cap_join_points[1].x = MS_NINT(x)-dx_px*size*symbol->linejoinmaxsize;
                cap_join_points[1].y = MS_NINT(y)-dy_px*size*symbol->linejoinmaxsize;
              } else { 
                cap_join_points[1] = generateGDLineIntersection(last_points[1], last_points[2], points[1], points[2]); 
              }
            }
            /* Check the max join size */ 
            dx = cap_join_points[1].x - x; 
            dy = cap_join_points[1].y - y;
            s = sqrt(pow(dx,2)+pow(dy,2));
            
            if (symbol->linejoin == MS_CJC_MITER) {
              /* Miter */
              maxs = size*symbol->linejoinmaxsize;
              if (s > maxs) {
                s = s/maxs;
                dx = dx/s; 
                dy = dy/s;
                cap_join_points[1].x = MS_NINT(x + dx); 
                cap_join_points[1].y = MS_NINT(y + dy); 
              }
              s = s?s:1;
              cap_join_points[3].x = MS_NINT(x - dx/s); 
              cap_join_points[3].y = MS_NINT(y - dy/s); 
              gdImageFilledPolygon(img, cap_join_points, 4, c);
            /* Bevel */
            } else {
              s = s?s:1;
              cap_join_points[1] = cap_join_points[2]; 
              cap_join_points[2].x = MS_NINT(x - dx/s); 
              cap_join_points[2].y = MS_NINT(y - dy/s); 
              gdImageFilledPolygon(img, cap_join_points, 3, c);
            }
          break;
        }
      }

      /* Captype */
      if (symbol->linecap != MS_CJC_BUTT && size > 3 && ((j == 1) || (j == (p->line[i].numpoints-1)))) { 
        size_2 = size/sqrt(pow(dx_px,2)+pow(dy_px,2))/2;
        switch (symbol->linecap) {
          /* Butt */
          case MS_CJC_BUTT:
            /* do nothing */
          break;
          /* Round */
          case MS_CJC_ROUND:
            /* First point */            
            if (j == 1) { 
              dx = points[0].x - points[1].x;
              dy = points[0].y - points[1].y;
              angle_from = angle + MS_PI2;
              angle_from -= angle_from > MS_2PI?MS_2PI:0;
              angle_to   = angle_from + MS_PI;
              s = sqrt(pow(dx,2) + pow(dy,2))/2;
              imageFilledSegment(img, points[0].x+dx_px-dx/2.0, points[0].y+dy_px-dy/2.0, s, angle_from, angle_to, c);
            } 
            /* Last point */            
            if (j == (p->line[i].numpoints-1)) {
              dx = points[2].x - points[3].x;
              dy = points[2].y - points[3].y;
              angle_from = angle - MS_PI2;
              angle_from += angle_from < 0?MS_2PI:0;
              angle_to   = angle_from + MS_PI;
              s = sqrt(pow(dx,2) + pow(dy,2))/2;
              imageFilledSegment(img, points[2].x-dx_px-dx/2.0, points[2].y-dy_px-dy/2.0, s, angle_from, angle_to, c);
            }
          break;
          /* Square */
          case MS_CJC_SQUARE:
            /* First point */            
            if (j == 1) { 
              cap_join_points[0].x = MS_NINT(points[0].x+dx_px); 
              cap_join_points[0].y = MS_NINT(points[0].y+dy_px); 
              cap_join_points[1].x = MS_NINT(points[1].x+dx_px); 
              cap_join_points[1].y = MS_NINT(points[1].y+dy_px); 
              cap_join_points[2].x = MS_NINT(points[1].x - size_2*dx_px); 
              cap_join_points[2].y = MS_NINT(points[1].y - size_2*dy_px); 
              cap_join_points[3].x = MS_NINT(points[0].x - size_2*dx_px); 
              cap_join_points[3].y = MS_NINT(points[0].y - size_2*dy_px);
              gdImageFilledPolygon(img, cap_join_points, 4, c);
            } 
            /* Last point */            
            if (j == (p->line[i].numpoints-1)) {
              cap_join_points[0].x = MS_NINT(points[2].x-dx_px); 
              cap_join_points[0].y = MS_NINT(points[2].y-dy_px); 
              cap_join_points[1].x = MS_NINT(points[3].x-dx_px); 
              cap_join_points[1].y = MS_NINT(points[3].y-dy_px); 
              cap_join_points[2].x = MS_NINT(points[3].x + size_2*dx_px); 
              cap_join_points[2].y = MS_NINT(points[3].y + size_2*dy_px); 
              cap_join_points[3].x = MS_NINT(points[2].x + size_2*dx_px); 
              cap_join_points[3].y = MS_NINT(points[2].y + size_2*dy_px);
              gdImageFilledPolygon(img, cap_join_points, 4, c);
            } 
          break;
          /* Triangle */
          case MS_CJC_TRIANGLE:
            /* First point */            
            if (j == 1) { 
              cap_join_points[0].x = MS_NINT(points[0].x+dx_px); 
              cap_join_points[0].y = MS_NINT(points[0].y+dy_px); 
              cap_join_points[1].x = MS_NINT(points[1].x+dx_px); 
              cap_join_points[1].y = MS_NINT(points[1].y+dy_px); 
              cap_join_points[2].x = MS_NINT((points[0].x+points[1].x)/2 - size_2*dx_px); 
              cap_join_points[2].y = MS_NINT((points[0].y+points[1].y)/2 - size_2*dy_px); 
              gdImageFilledPolygon(img, cap_join_points, 3, c);
            } 
            /* Last point */            
            if (j == (p->line[i].numpoints-1)) {
              cap_join_points[0].x = MS_NINT(points[2].x-dx_px); 
              cap_join_points[0].y = MS_NINT(points[2].y-dy_px); 
              cap_join_points[1].x = MS_NINT(points[3].x-dx_px); 
              cap_join_points[1].y = MS_NINT(points[3].y-dy_px); 
              cap_join_points[2].x = MS_NINT((points[2].x+points[3].x)/2 + size_2*dx_px); 
              cap_join_points[2].y = MS_NINT((points[2].y+points[3].y)/2 + size_2*dy_px); 
              gdImageFilledPolygon(img, cap_join_points, 3, c);
            } 
          break;
        }
      }  
      
/*      gdImageLine(img, points[0].x, points[0].y, points[1].x, points[1].y, 3);  
      gdImageLine(img, points[1].x, points[1].y, points[2].x, points[2].y, 3);  
      gdImageLine(img, points[2].x, points[2].y, points[3].x, points[3].y, 3);  
*/
      /* Copy the last point and angle */
      for (k=0; k<4; k++) {
        last_points[k] = points[k];
//        gdImageSetPixel(img, MS_NINT(points[k].x), MS_NINT(points[k].y), 1); 
      }
      last_angle = angle;
      last_styleVis = styleVis;
    
    }

  }
}

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

  fprintf(stdout, "Shape contains %d parts.\n",  p->numlines);
  for (i=0; i<p->numlines; i++) {
    fprintf(stdout, "\tPart %d contains %d points.\n", i, p->line[i].numpoints);
    for (j=0; j<p->line[i].numpoints; j++) {
      fprintf(stdout, "\t\t%d: (%f, %f)\n", j, p->line[i].point[j].x, p->line[i].point[j].y);
    }
  }
}

void msInitShape(shapeObj *shape)
{
  // spatial component
  shape->line = NULL;
  shape->numlines = 0;
  shape->type = MS_NULL;
  shape->bounds.minx = shape->bounds.miny = -1;
  shape->bounds.maxx = shape->bounds.maxy = -1;
  
  // attribute component ...to be added soon

  // annotation component
  shape->text = NULL;

  // bookkeeping component
  shape->classindex = 0; // default class
  shape->queryindex = -1;
}

void msFreeShape(shapeObj *shape)
{
  int c;

  for (c= 0; c < shape->numlines; c++)
    free(shape->line[c].point);
  free(shape->line);

  free(shape->text);
  
  shape->line = NULL;
  shape->numlines= 0;
  shape->text = NULL;
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
  free(p->line);

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
void msRect2Polygon(rectObj rect, shapeObj *poly)
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
  poly->type = MS_POLYGON;
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
void msClipPolylineRect(shapeObj *in, rectObj rect, shapeObj *out)
{
  int i,j;
  lineObj line={0,NULL};
  double x1, x2, y1, y2;
  shapeObj tmp={0,NULL};

  if(in->numlines == 0) /* nothing to clip */
    return;

  for(i=0; i<in->numlines; i++) {

    line.point = (pointObj *)malloc(sizeof(pointObj)*in->line[i].numpoints);
    line.numpoints = 0;

    x1 = in->line[i].point[0].x;
    y1 = in->line[i].point[0].y;
    for(j=1; j<in->line[i].numpoints; j++) {
      x2 = in->line[i].point[j].x;
      y2 = in->line[i].point[j].y;

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

	if((x2 != in->line[i].point[j].x) || (y2 != in->line[i].point[j].y)) {
	  msAddLine(&tmp, &line);
	  line.numpoints = 0; /* new line */
	}
      }

      x1 = in->line[i].point[j].x;
      y1 = in->line[i].point[j].y;
    }

    if(line.numpoints > 0)
      msAddLine(&tmp, &line);
    free(line.point);
    line.numpoints = 0; /* new line */
  }
  
  if(in == out)
    msFreeShape(in);
  out->line = tmp.line;
  out->numlines = tmp.numlines;
}

/*
** Slightly modified version of the Liang-Barsky polygon clipping algorithm
*/
void msClipPolygonRect(shapeObj *in, rectObj rect, shapeObj *out)
{
  int i, j;
  double deltax, deltay, xin,xout,  yin,yout;
  double tinx,tiny,  toutx,touty,  tin1, tin2,  tout;
  double x1,y1, x2,y2;

  shapeObj tmp={0,NULL,{-1,-1,-1,-1},MS_NULL};
  lineObj line={0,NULL};
  
  if(in->numlines == 0) /* nothing to clip */
    return;
   
  for(j=0; j<in->numlines; j++) {

    line.point = (pointObj *)malloc(sizeof(pointObj)*2*in->line[j].numpoints+1); /* worst case scenario, +1 allows us to duplicate the 1st and last point */
    line.numpoints = 0;

    for (i = 0; i < in->line[j].numpoints-1; i++) {
      
      x1 = in->line[j].point[i].x;
      y1 = in->line[j].point[i].y;
      x2 = in->line[j].point[i+1].x;
      y2 = in->line[j].point[i+1].y;
      
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
  
  if(in == out)
    msFreeShape(in);
  out->line = tmp.line;
  out->numlines = tmp.numlines;

  return;
}

/*
** Converts from map coordinates to image coordinates
*/
void msTransformPolygon(rectObj extent, double cellsize, shapeObj *p)
{
  int i,j,k; /* loop counters */

  if(p->numlines == 0)
    return; /* nothing to do */  
  
  for(i=0; i<p->numlines; i++) { /* for each line */
    
    p->line[i].point[0].x = MS_NINT((p->line[i].point[0].x - extent.minx)/cellsize);
    p->line[i].point[0].y = MS_NINT((extent.maxy - p->line[i].point[0].y)/cellsize);
      
    for(j=1, k=1; j < p->line[i].numpoints; j++ ) {
      
      p->line[i].point[k].x = MS_NINT((p->line[i].point[j].x - extent.minx)/cellsize); 
      p->line[i].point[k].y = MS_NINT((extent.maxy - p->line[i].point[j].y)/cellsize);
      
      if(k == 1) {
	if((p->line[i].point[0].x != p->line[i].point[1].x) || (p->line[i].point[0].y != p->line[i].point[1].y))
	  k++;
      } else {
	if((p->line[i].point[k-1].x != p->line[i].point[k].x) || (p->line[i].point[k-1].y != p->line[i].point[k].y)) {
	  if(((p->line[i].point[k-2].y - p->line[i].point[k-1].y)*(p->line[i].point[k-1].x - p->line[i].point[k].x)) == ((p->line[i].point[k-2].x - p->line[i].point[k-1].x)*(p->line[i].point[k-1].y - p->line[i].point[k].y))) {	    
	    p->line[i].point[k-1].x = p->line[i].point[k].x;
	    p->line[i].point[k-1].y = p->line[i].point[k].y;	
	  } else {
	    k++;
	  }
	}
      }
    }
    p->line[i].numpoints = k; /* save actual number kept */
  }

  return;
}

void msImageScanline(gdImagePtr im, int x1, int x2, int y, int c)
{
  int x;

  for(x=x1; x<=x2; x++)
    gdImageSetPixel(im, x, y, c);
}

void msImagePolyline(gdImagePtr im, shapeObj *p, int c)
{
  int i, j;
  
  for (i = 0; i < p->numlines; i++) {
    for(j=1; j<p->line[i].numpoints; j++)
      gdImageLine(im, (int)p->line[i].point[j-1].x, (int)p->line[i].point[j-1].y, (int)p->line[i].point[j].x, (int)p->line[i].point[j].y, c);
  }
}

void msImagePolylineOffset(gdImagePtr im, shapeObj *p, int offset, int c)
{
  int i, j;
  double dx, dy;
  int ox=0, oy=0;
  
  if(offset == 0) {
    msImagePolyline(im, p, c);
    return;
  }

  for (i = 0; i < p->numlines; i++) {
    for(j=1; j<p->line[i].numpoints; j++) {
      dx = abs(p->line[i].point[j-1].x - p->line[i].point[j].x);
      dy = abs(p->line[i].point[j-1].y - p->line[i].point[j].y);
      if(dx<=dy)
	ox=offset;
      else
	oy=offset;
      gdImageLine(im, (int)p->line[i].point[j-1].x+ox, (int)p->line[i].point[j-1].y+oy, (int)p->line[i].point[j].x+ox, (int)p->line[i].point[j].y+oy, c);
      ox = oy = 0;
    }
  }
}

void msImageFilledPolygon(gdImagePtr im, shapeObj *p, int c)
{
  float *slope;
  pointObj point1, point2, testpoint1, testpoint2;
  int i, j, k, l, m, nfound, *xintersect, temp, sign;
  int x, y, ymin, ymax, *horiz, wrong_order;
  int n;

  if(p->numlines == 0)
    return;

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
    point1 = p->line[j].point[p->line[j].numpoints-1];
    for(i=0; i < p->line[j].numpoints; i++,l++) {
      point2 = p->line[j].point[i];
      if(point1.y == point2.y) {
	horiz[l] = 1;
	slope[l] = 0.0;
      } else {
	horiz[l] = 0;
	slope[l] = (float) (point2.x - point1.x) / (point2.y - point1.y);
      }
      ymin = MS_MIN(ymin, point1.y);
      ymax = MS_MAX(ymax, point2.y);
      point1 = point2;
    }
  }  

  for(y = ymin; y <= ymax; y++) { /* for each scanline */

    nfound = 0;
    for(j=0, l=0; j<p->numlines; j++) { /* for each line, l is overall point counter */

      m = l; /* m is offset from begining of all vertices */
      point1 = p->line[j].point[p->line[j].numpoints-1];
      for(i=0; i < p->line[j].numpoints; i++, l++) {
	point2 = p->line[j].point[i];
	if(EDGE_CHECK(point1.y, y, point2.y) == CLIP_MIDDLE) {
	  
	  if(horiz[l]) /* First, is this edge horizontal ? */
	    continue;

	  /* Did we intersect the first point point ? */
	  if(y == point1.y) {
	    /* Yes, must find first non-horizontal edge */
	    k = i-1;
	    if(k < 0) k = p->line[j].numpoints-1;
	    while(horiz[m+k]) {
	      k--;
	      if(k < 0) k = p->line[j].numpoints-1;
	    }
	    /* Now perform sign test */
	    if(k > 0)
	      testpoint1 = p->line[j].point[k-1];
	    else
	      testpoint1 = p->line[j].point[p->line[j].numpoints-1];
	    testpoint2 = p->line[j].point[k];
	    sign = (testpoint2.y - testpoint1.y) *
	      (point2.y - point1.y);
	    if(sign < 0)
	      xintersect[nfound++] = point1.x;
	    /* All done for point matching case */
	  } else {  
	    /* Not at the first point,
	       find the intersection*/
	    x = ROUND(point1.x + (y - point1.y)*slope[l]);
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
    point1 = p->line[j].point[p->line[j].numpoints - 1];
    for(i=0; i<p->line[j].numpoints; i++, l++) {
      point2 = p->line[j].point[i];
      if(horiz[l])
	msImageScanline(im, point1.x, point2.x, point2.y, c);
      point1 = point2;
    }
  }
  
  free(slope);
  free(horiz);
  free(xintersect);

} /* End of msImageFilledPolygon */

static double length(pointObj a, pointObj b)
{
  double d;

  d = sqrt((pow((a.x-b.x),2) + pow((a.y-b.y),2)));
  return(d);
}

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
  pointObj point1, point2;
  int i, j, k, nfound;
  double x, y, *xintersect, temp;
  double hi_y, lo_y;
  int wrong_order, n;
  double len, max_len=0;
  double skip, minx, maxx, maxy, miny;

  get_bbox(p, &minx, &miny, &maxx, &maxy);

  if(min_dimension != -1)
    if(MS_MIN(maxx-minx,maxy-miny) < min_dimension) return(-1);

  lp->x = (maxx+minx)/2.0;
  lp->y = (maxy+miny)/2.0;

  //if(get_centroid(p, lp, &miny, &maxy) == -1) return(-1);

  if(msIntersectPointPolygon(lp, p) == MS_TRUE) /* cool, done */
    return(0);

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
      return (-1);
    else  
      y = (hi_y + lo_y)/2.0;    
    
    nfound = 0;
    for(j=0; j<p->numlines; j++) { /* for each line */
      
      point1 = p->line[j].point[p->line[j].numpoints-1];
      for(i=0; i < p->line[j].numpoints; i++) {
	point2 = p->line[j].point[i];
	
	if(EDGE_CHECK(point1.y, y, point2.y) == CLIP_MIDDLE) {
	  
	  if(point1.y == point2.y)
	    continue; /* ignore horizontal edges */
	  else	  
	    slope = (point2.x - point1.x) / (point2.y - point1.y);
	  
	  x = point1.x + (y - point1.y)*slope;
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
    point1.y = point2.y = y;
    for(i=0; i < nfound; i += 2) {
      point1.x = xintersect[i];
      point2.x = xintersect[i+1];
      len = length(point1, point2);
      if(len > max_len) {
	max_len = len;
	lp->x = (point1.x + point2.x)/2;
	lp->y = y;
      }
    }
  }

  free(xintersect);

  if(max_len > 0)
    return(0);
  else
    return(-1);
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
    return(-1);

  if((min_length != -1) && (total_length < min_length)) /* too short to label */
    return(-1);

  // ok, now we know which line and which segment within that line
  i = line_index;
  j = segment_index;

  *length = total_length;

  lp->x = (p->line[i].point[j].x + p->line[i].point[j-1].x)/2.0;
  lp->y = (p->line[i].point[j].y + p->line[i].point[j-1].y)/2.0;
 
  theta = asin(MS_ABS(p->line[i].point[j].x - p->line[i].point[j-1].x)/sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2))));

  if(p->line[i].point[j-1].x < p->line[i].point[j].x) { /* i.e. to the left */
    if(p->line[i].point[j-1].y < p->line[i].point[j].y) /* i.e. below */
      *angle = -MS_DEG_TO_RAD*(90 - MS_RAD_TO_DEG*theta);
    else
      *angle = MS_DEG_TO_RAD*(90 - MS_RAD_TO_DEG*theta);      
  } else {
    if(p->line[i].point[j-1].y < p->line[i].point[j].y) /* i.e. below */
      *angle = MS_DEG_TO_RAD*(90 - MS_RAD_TO_DEG*theta);
    else
      *angle = -MS_DEG_TO_RAD*(90 - MS_RAD_TO_DEG*theta);      
  }

  return(0);
}

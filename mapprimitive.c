/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Implementations for rectObj, pointObj, lineObj, shapeObj, etc.
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.76  2006/09/01 18:03:19  umberto
 * Removed USE_GEOS defines, bug #1890
 *
 * Revision 1.75  2006/08/17 04:32:16  sdlime
 * Disable path following labels unless GD 2.0.29 or greater is available.
 *
 * Revision 1.74  2006/06/27 13:58:19  frank
 * Place shapeObj->geometry assignment into #ifdef USE_GEOS.
 *
 * Revision 1.73  2006/06/27 06:32:09  sdlime
 * Fixed msCopyShape to initialize the geometry property of the destination shape to NULL.
 *
 * Revision 1.72  2006/05/18 22:20:37  sdlime
 * Fixed offseting code percentage of 1.0 maps to width-1 or height-1.
 *
 * Revision 1.71  2006/04/28 03:13:02  sdlime
 * Fixed a few issues with relative coordinates. Added support for all nine relative positions. (bug 1547)
 *
 * Revision 1.70  2006/04/27 04:05:17  sdlime
 * Initial support for relative coordinates. (bug 1547)
 *
 * Revision 1.69  2006/04/26 13:42:53  frank
 * temporarily block out MS_PERCENTAGES use till it is defined.
 *
 * Revision 1.68  2006/04/26 03:25:47  sdlime
 * Applied most recent patch for curved labels. (bug 1620)
 *
 * Revision 1.67  2006/03/22 23:31:20  sdlime
 * Applied latest patch for curved labels. (bug 1620)
 *
 * Revision 1.66  2006/03/02 06:43:51  sdlime
 * Applied latest patch for curved labels. (bug 1620)
 *
 * Revision 1.65  2006/02/24 05:53:49  sdlime
 * Applied another round of patches for bug 1620.
 *
 * Revision 1.64  2006/02/18 21:14:09  hobu
 * INFINITY is already defined on in the math headers on osx.
 * Don't redefine it if it is already there.
 *
 * Revision 1.63  2006/02/18 20:59:13  sdlime
 * Initial code for curved labels. (bug 1620)
 *
 * Revision 1.62  2006/02/17 03:05:23  sdlime
 * Slightly more efficient version (no modulus operator) of the routine to test if a ring is an outer ring. (bug 1648)
 *
 * Revision 1.61  2006/02/16 07:51:31  sdlime
 * Fixed a flaw in routine that computes outer ring list. In certain cases it could miss an outer ring with holes in certain places. (bug 1648)
 *
 * Revision 1.60  2005/11/01 05:35:50  frank
 * added preliminary implementation of OGR based WKT translation, still untested
 *
 * Revision 1.59  2005/10/31 04:58:14  sdlime
 * Added a check in msFreeShape() to make sure the incoming shapeObj is not NULL.
 *
 * Revision 1.58  2005/10/30 05:05:07  sdlime
 * Initial support for WKT via GEOS. The reader is only integrated via the map file reader, with MapScript, CGI and URL support following ASAP. (bug 1466)
 *
 * Revision 1.57  2005/10/20 19:37:03  frank
 * added msAddPointToLine
 *
 * Revision 1.56  2005/10/18 03:20:44  frank
 * fixed use of memmove in msShapeDeleteLine()
 *
 * Revision 1.55  2005/10/18 03:10:45  frank
 * added msShapeDeleteLine
 *
 * Revision 1.54  2005/07/27 18:21:58  frank
 * bug 1432: optimized msAddLine() to use realloc()
 *
 * Revision 1.53  2005/06/14 16:03:34  dan
 * Updated copyright date to 2005
 *
 * Revision 1.52  2005/05/19 05:57:08  sdlime
 * Added GEOS geometry clean up code to msFreeShape...
 *
 * Revision 1.51  2005/04/21 15:09:28  julien
 * Bug1244: Replace USE_SHAPE_Z_M by USE_POINT_Z_M
 *
 * Revision 1.50  2005/04/14 15:17:14  julien
 * Bug 1244: Remove Z and M from point by default to gain performance.
 *
 * Revision 1.49  2005/03/25 05:42:58  frank
 * added msAddLineDirectly(), msAddLine() uses memcpy()
 *
 * Revision 1.48  2005/03/24 22:27:20  frank
 * optimized msTransformShapeToPixel - avoid division
 *
 * Revision 1.47  2005/03/24 17:50:04  frank
 * optimized msClipPoly{gon/line}Rect for all-inside case
 *
 * Revision 1.46  2005/02/22 07:40:27  sdlime
 * A bunch of updates to GEOS integration. Can move many primatives between MapServer and GEOS, still need to do collections (e.g. multi-point/line/polygon). Added buffer method to mapscript (mapscript/shape.i).
 *
 * Revision 1.45  2005/02/18 03:06:46  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.44  2005/02/13 22:16:06  dan
 * Use double as second arg to pow() (bug 1235)
 *
 * Revision 1.43  2004/12/14 21:30:43  sdlime
 * Moved functions to build lists of inner and outer rings to mapprimitive.c from mapgml.c. They are needed to covert between MapServer polygons and GEOS gemometries (bug 771).
 *
 * Revision 1.42  2004/11/12 20:23:16  frank
 * include z and m in formatted pointObj, fmt is const!
 *
 * Revision 1.41  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"
#include "mapprimitive.h"
#include <assert.h>

MS_CVSID("$Id$")

typedef enum {CLIP_LEFT, CLIP_MIDDLE, CLIP_RIGHT} CLIP_STATE;

#define CLIP_CHECK(min, a, max) ((a) < (min) ? CLIP_LEFT : ((a) > (max) ? CLIP_RIGHT : CLIP_MIDDLE));
#define ROUND(a)       ( (a) + 0.5 )
#define SWAP( a, b, t) ( (t) = (a), (a) = (b), (b) = (t) )
#define EDGE_CHECK( x0, x, x1) ((x) < MS_MIN( (x0), (x1)) ? CLIP_LEFT : ((x) > MS_MAX( (x0), (x1)) ? CLIP_RIGHT : CLIP_MIDDLE ))

#ifndef INFINITY
#define INFINITY	(1.0e+30)
#endif
#define NEARZERO	(1.0e-30)	/* 1/INFINITY */

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

shapeObj *msShapeFromWKT(const char *string)
{
#ifdef USE_GEOS
  return msGEOSShapeFromWKT(string);
#elif defined(USE_OGR)
  return msOGRShapeFromWKT(string);
#else
  msSetError(MS_MISCERR, "WKT support is not available, please compile MapServer with GEOS or OGR support.", "msShapeFromWKT()");
  return NULL;
#endif
}

char *msShapeToWKT(shapeObj *shape)
{
#ifdef USE_GEOS
  return msGEOSShapeToWKT(shape);
#elif defined(USE_OGR)
  return msOGRShapeToWKT(shape);
#else
  msSetError(MS_MISCERR, "WKT support is not available, please compile MapServer with GEOS or OGR support.", "msShapeToWKT()");
  return NULL;
#endif
}

void msInitShape(shapeObj *shape)
{
  /* spatial component */
  shape->line = NULL;
  shape->numlines = 0;
  shape->type = MS_SHAPE_NULL;
  shape->bounds.minx = shape->bounds.miny = -1;
  shape->bounds.maxx = shape->bounds.maxy = -1;
  
  /* attribute component */
  shape->values = NULL;
  shape->numvalues = 0;

  shape->geometry = NULL;

  /* annotation component */
  shape->text = NULL;

  /* bookkeeping component */
  shape->classindex = 0; /* default class */
  shape->tileindex = shape->index = -1;
}

int msCopyShape(shapeObj *from, shapeObj *to) {
  int i;

  if(!from || !to) return(-1);

  for(i=0; i<from->numlines; i++)
    msAddLine(to, &(from->line[i])); /* copy each line */

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

  to->geometry = NULL; /* GEOS code will build automatically if necessary */

  return(0);
}

void msFreeShape(shapeObj *shape)
{
  int c;

  if(!shape) return; /* for safety */

  for (c= 0; c < shape->numlines; c++)
    free(shape->line[c].point);
  free(shape->line);

  if(shape->values) msFreeCharArray(shape->values, shape->numvalues);
  if(shape->text) free(shape->text);
  
#ifdef USE_GEOS
  msGEOSFreeGeometry(shape);
#endif

  msInitShape(shape); /* now reset */
}

void msFreeLabelPathObj(labelPathObj *path)
{
  msFreeShape(&(path->bounds));
  msFree(path->path.point);
  msFree(path->angles);
  msFree(path);
}

void msShapeDeleteLine( shapeObj *shape, int line )

{
    if( line < 0 || line >= shape->numlines )
    {
        assert( 0 );
        return;
    }

    free( shape->line[line].point );
    if( line < shape->numlines - 1 )
    {
        memmove( shape->line + line, 
                 shape->line + line + 1, 
                 sizeof(lineObj) * (shape->numlines - line - 1) );
    }
    shape->numlines--;
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

/* checks to see if ring r is an outer ring of shape */
static int isOuterRing(shapeObj *shape, int r) 
{
  int i, status=MS_TRUE; 

  if(shape->numlines == 1) return(MS_TRUE);

  for(i=0; i<shape->numlines; i++) {
    if(i == r) continue;
    if(msPointInPolygon(&(shape->line[r].point[0]), &(shape->line[i])) == MS_TRUE)
      status = !status;
  }

  return(status);
}

/*
** Returns a list of outer rings for shape (the list has one entry for each ring, 
** MS_TRUE for outer rings).
*/
int *msGetOuterList(shapeObj *shape) 
{
  int i;
  int *list;

  list = (int *)malloc(sizeof(int)*shape->numlines);
  if(!list) return(NULL);

  for(i=0; i<shape->numlines; i++)
    list[i] = isOuterRing(shape, i);

  return(list);
}

/*
** Returns a list of inner rings for ring r in shape (given a list of outer rings).
*/
int *msGetInnerList(shapeObj *shape, int r, int *outerlist) 
{
  int i;
  int *list;

  list = (int *)malloc(sizeof(int)*shape->numlines);
  if(!list) return(NULL);

  for(i=0; i<shape->numlines; i++) { /* test all rings against the ring */

    if(outerlist[i] == MS_TRUE) { /* ring is an outer and can't be an inner */
      list[i] = MS_FALSE;
      continue;
    }

    list[i] = msPointInPolygon(&(shape->line[i].point[0]), &(shape->line[r]));
  }

  return(list);
}

/*
** Add point to a line object.
**
** Note that reallocating the point array larger for each point can
** be pretty inefficient, so use this function sparingly.  Mostly 
** geometries creators should create their own working lineObj and 
** then call msAddLine() to add it to a shape.
*/

int msAddPointToLine(lineObj *line, pointObj *point )
{
    line->numpoints += 1;
    
    line->point = (pointObj *) 
        realloc(line->point, sizeof(pointObj) * line->numpoints);

    line->point[line->numpoints-1] = *point;

    return MS_SUCCESS;
}

int msAddLine(shapeObj *p, lineObj *new_line)
{
    lineObj lineCopy;

    lineCopy.numpoints = new_line->numpoints;
    lineCopy.point = (pointObj *) malloc(new_line->numpoints*sizeof(pointObj));
    if( lineCopy.point == NULL )
    {
        msSetError(MS_MEMERR, NULL, "msAddLine()");
        return(MS_FAILURE);
    }
    
    memcpy( lineCopy.point, new_line->point, 
            sizeof(pointObj) * new_line->numpoints );

    return msAddLineDirectly( p, &lineCopy );
}

/*
** Same as msAddLine(), except that this version "seizes" the points
** array from the passed in line and uses it instead of copying it.  
*/
int msAddLineDirectly(shapeObj *p, lineObj *new_line)
{
  int c;

  if( p->numlines == 0 )
      p->line = (lineObj *) malloc(sizeof(lineObj));
  else
      p->line = (lineObj *) realloc(p->line, (p->numlines+1)*sizeof(lineObj));
  
  /* Copy the new line onto the end of the extended line array */
  c= p->numlines;
  p->line[c].numpoints = new_line->numpoints;  
  p->line[c].point = new_line->point;

  /* strip points reference off the passed in lineObj */
  new_line->point = NULL;
  new_line->numpoints = 0;

  /* Update the polygon information */
  p->numlines++;

  return(MS_SUCCESS);
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
  if(poly->numlines == 1) { /* poly was empty to begin with */
    poly->type = MS_SHAPE_POLYGON;
    poly->bounds = rect;
  } else
    msMergeRect(&poly->bounds, &rect);
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
  shapeObj tmp;

  memset( &tmp, 0, sizeof(shapeObj) );

  if(shape->numlines == 0) /* nothing to clip */
    return;

  /*
  ** Don't do any clip processing of shapes completely within the
  ** clip rectangle based on a comparison of bounds.   We could do 
  ** something similar for completely outside, but that rarely occurs
  ** since the spatial query at the layer read level has generally already
  ** discarded all shapes completely outside the rect.
  */
  if( shape->bounds.maxx <= rect.maxx
      && shape->bounds.minx >= rect.minx
      && shape->bounds.maxy <= rect.maxy
      && shape->bounds.miny >= rect.miny )
  {
      return;
  }

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

  /*
  ** Don't do any clip processing of shapes completely within the
  ** clip rectangle based on a comparison of bounds.   We could do 
  ** something similar for completely outside, but that rarely occurs
  ** since the spatial query at the layer read level has generally already
  ** discarded all shapes completely outside the rect.
  */
  if( shape->bounds.maxx <= rect.maxx
      && shape->bounds.minx >= rect.minx
      && shape->bounds.maxy <= rect.maxy
      && shape->bounds.miny >= rect.miny )
  {
      return;
  }

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
      line.point[line.numpoints].x = line.point[0].x; /* force closure */
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
** offsets a point relative to an image position
*/
void msOffsetPointRelativeTo(pointObj *point, layerObj *layer)
{
  double x=0, y=0;

  if(layer->transform == MS_TRUE) return; /* nothing to do */

  if(layer->units == MS_PERCENTAGES) {
    point->x *= (layer->map->width-1);
    point->y *= (layer->map->height-1);
  }

  if(layer->transform == MS_FALSE || layer->transform == MS_UL) return; /* done */

  switch(layer->transform) {
  case MS_UC:
    x = (layer->map->width-1)/2;
    y = 0;
    break;
  case MS_UR:
    x = layer->map->width-1;
    y = 0;
    break;
  case MS_CL:
    x = 0;
    y = layer->map->height/2;
    break;
  case MS_CC:
    x = layer->map->width/2;
    y = layer->map->height/2;
    break;
  case MS_CR:
    x = layer->map->width-1;
    y = layer->map->height/2;
    break;
  case MS_LL:
    x = 0;
    y = layer->map->height-1;
    break;
  case MS_LC:
    x = layer->map->width/2;
    y = layer->map->height-1;
    break;
  case MS_LR:
    x = layer->map->width-1;
    y = layer->map->height-1;
    break;
  }

  point->x += x;
  point->y += y;

  return;
}

/*
** offsets a shape relative to an image position
*/
void msOffsetShapeRelativeTo(shapeObj *shape, layerObj *layer) 
{
  int i, j;
  double x=0, y=0;

  if(layer->transform == MS_TRUE) return; /* nothing to do */

  if(layer->units == MS_PERCENTAGES) {
    for (i=0; i<shape->numlines; i++) {
      for (j=0; j<shape->line[i].numpoints; j++) {
        shape->line[i].point[j].x *= (layer->map->width-1);
        shape->line[i].point[j].y *= (layer->map->height-1);
      }
    }
  }

  if(layer->transform == MS_FALSE || layer->transform == MS_UL) return; /* done */

  switch(layer->transform) {
  case MS_UC:
    x = (layer->map->width-1)/2;
    y = 0;
    break;
  case MS_UR:
    x = layer->map->width-1;
    y = 0;
    break;
  case MS_CL:
    x = 0;
    y = layer->map->height/2;
    break;
  case MS_CC:
    x = layer->map->width/2;
    y = layer->map->height/2;
    break;
  case MS_CR:
    x = layer->map->width-1;
    y = layer->map->height/2;
    break;
  case MS_LL:
    x = 0;
    y = layer->map->height-1;
    break;
  case MS_LC:
    x = layer->map->width/2;
    y = layer->map->height-1;
    break;
  case MS_LR:
    x = layer->map->width-1;
    y = layer->map->height-1;
    break;
  }

  for (i=0; i<shape->numlines; i++) {
    for (j=0; j<shape->line[i].numpoints; j++) {
      shape->line[i].point[j].x += x;
      shape->line[i].point[j].y += y;
    }
  }

  return;
}

/*
** converts from map coordinates to image coordinates
*/
void msTransformShapeToPixel(shapeObj *shape, rectObj extent, double cellsize)
{
  int i,j,k; /* loop counters */
  double inv_cs = 1.0 / cellsize; /* invert and multiply much faster */

  if(shape->numlines == 0) return; /* nothing to transform */

  if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON) { /* remove co-linear vertices */
  
    for(i=0; i<shape->numlines; i++) { /* for each part */
      
      shape->line[i].point[0].x = MS_MAP2IMAGE_X_IC(shape->line[i].point[0].x, extent.minx, inv_cs);
      shape->line[i].point[0].y = MS_MAP2IMAGE_Y_IC(shape->line[i].point[0].y, extent.maxy, inv_cs);
      
      for(j=1, k=1; j < shape->line[i].numpoints; j++ ) {
	
	shape->line[i].point[k].x = MS_MAP2IMAGE_X_IC(shape->line[i].point[j].x, extent.minx, inv_cs);
	shape->line[i].point[k].y = MS_MAP2IMAGE_Y_IC(shape->line[i].point[j].y, extent.maxy, inv_cs);

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
      shape->line[i].numpoints = k; /* save actual number kept */
    }
  } else { /* points or untyped shapes */
    for(i=0; i<shape->numlines; i++) { /* for each part */
      for(j=1; j < shape->line[i].numpoints; j++ ) {
	shape->line[i].point[j].x = MS_MAP2IMAGE_X_IC(shape->line[i].point[j].x, extent.minx, inv_cs);
	shape->line[i].point[j].y = MS_MAP2IMAGE_Y_IC(shape->line[i].point[j].y, extent.maxy, inv_cs);
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

	if(shape->numlines == 0) return; /* nothing to transform */

	if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON)  /* remove co-linear vertices */
	{
		for(i=0; i<shape->numlines; i++)  /* for each part */
		{ 
			for(j=0; j < shape->line[i].numpoints; j++ ) 
			{
				shape->line[i].point[j].x = MS_IMAGE2MAP_X(shape->line[i].point[j].x, extent.minx, cellsize);
				shape->line[i].point[j].y = MS_IMAGE2MAP_Y(shape->line[i].point[j].y, extent.maxy, cellsize);
			}
		}
	}
	else  /* points or untyped shapes */
	{
		for(i=0; i<shape->numlines; i++)  /* for each part */
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

    angle = asin(MS_ABS(p->line[i].point[1].x - p->line[i].point[0].x)/sqrt((pow((p->line[i].point[1].x - p->line[i].point[0].x),2.0) + pow((p->line[i].point[1].y - p->line[i].point[0].y),2.0))));
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

      angle = asin(MS_ABS(p->line[i].point[j].x - p->line[i].point[j-1].x)/sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2.0) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2.0))));
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

    /* need a touch of code if 1st point equals last point in p (find intersection) */

    msAddLine(op, &inside);
    msAddLine(op, &outside);

    free(inside.point);
    free(outside.point);
  }
  
  return;
}

/* Currently unused. */
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

  /* if(get_centroid(p, lp, &miny, &maxy) == -1) return(MS_FAILURE); */
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
    for(i=0; i < nfound; i += 2) {
      len = fabs(xintersect[i] - xintersect[i+1]);
      if(len > max_len) {
	max_len = len;
	lp->x = (xintersect[i] + xintersect[i+1])/2;
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
      segment_length = sqrt((pow((p->line[i].point[j].x-p->line[i].point[j-1].x),2.0) + pow((p->line[i].point[j].y-p->line[i].point[j-1].y),2.0)));
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

  /* ok, now we know which line and which segment within that line */
  i = line_index;
  j = segment_index;

  *length = total_length;

  lp->x = (p->line[i].point[j].x + p->line[i].point[j-1].x)/2.0;
  lp->y = (p->line[i].point[j].y + p->line[i].point[j-1].y)/2.0;
 
  theta = asin(MS_ABS(p->line[i].point[j].x - p->line[i].point[j-1].x)/sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2.0) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2.0))));

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

/*
 * Calculate a series of label points for each character in the label for a
 * given polyline.  The resultant series of points is stored in *labelpath.
 * Note that the points and bounds are allocated in this function.  The
 * polyline must be converted to image coordinates before calling this
 * function.
 */
labelPathObj* msPolylineLabelPath(shapeObj *p, int min_length, fontSetObj *fontset, char *string, labelObj *label, double scalefactor, int *status)
{
  double line_length, max_line_length, segment_length, total_length, distance_along_segment;
  double fwd_line_length, rev_line_length, text_length, text_start_length;
  int segment_index, line_index;
  
  int i,j,k, inc, final_j;
  double direction;
  rectObj bbox;
  lineObj bounds;
  double *offsets;
  double size, t;
  double cx, cy; /* centre of a character, x & y values. */
  
  double theta;
  double dx, dy, w, cos_t, sin_t;

  double **segment_lengths = NULL;
  labelPathObj *labelpath = NULL;
  char *font = NULL;

  /* Line smoothing kernel */
  double kernel[] = {0.1,0.2,2,0.2,0.1}; /*{1.5, 2, 15, 2, 1.5};*/
  double kernel_normal = 2.6; /* Must be sum of kernel elements */

  double letterspacing = 1.25;

  offsets = NULL;
  
  /* Assume success */
  *status = MS_SUCCESS;

#ifndef GD_HAS_FTEX_XSHOW
  goto FAILURE; /* we don't have a current enough version of GD, fall back to ANGLE AUTO */
#else
  /* Skip the label and use the normal algorithm if it has fewer than 2 characters */
  if ( strlen(string) < 2 ) {
    goto FAILURE;
  }
  
  segment_index = line_index = 0;
  total_length = max_line_length = 0.0;
  
  /* Determine longest line */
  segment_lengths = (double**)malloc(sizeof(double*) * p->numlines);
  for ( i = 0; i < p->numlines; i++ ) {
    
    segment_lengths[i] = (double*)malloc(sizeof(double) * p->line[i].numpoints);    
    line_length = 0;
    
    for ( j = 1; j < p->line[i].numpoints; j++ ) {
      segment_length = sqrt( pow((p->line[i].point[j].x - p->line[i].point[j-1].x), 2.0) +
                             pow((p->line[i].point[j].y - p->line[i].point[j-1].y), 2.0) );
      line_length += segment_length;
      segment_lengths[i][j-1] = segment_length;
      
    }

    total_length += line_length;
    
    if ( line_length > max_line_length ) {
      line_index = i;
      max_line_length = line_length;
    }
  }

  i = line_index;
  
  if ( ((min_length != -1) && (total_length < min_length)) ) {
    /* Too short */ 
    *status = MS_FAILURE;
    goto FAILURE;
  }

  if ( p->line[i].numpoints < 2 ) {
    /* Degenerate */
    *status = MS_FAILURE;
    goto FAILURE;
  }

  if ( p->line[i].numpoints == 2 ) {
    /* We can just use the regular algorithm to save some cycles */
    goto FAILURE;
  }
  
  /* Determine the total length of the text */
  if ( msGetLabelSizeEx(string, label, &bbox, fontset, scalefactor, MS_FALSE, &offsets) == MS_FAILURE ) {
    *status = MS_FAILURE;
    goto FAILURE;
  }

  size = label->size*scalefactor;
  size = MS_MAX(size, label->minsize);
  size = MS_MIN(size, label->maxsize);

  font = msLookupHashTable(&(fontset->fonts), label->font);
  if(!font) {
    if(label->font) 
      msSetError(MS_TTFERR, "Requested font (%s) not found.", "msPolylineLabelPath()", label->font);
    else
      msSetError(MS_TTFERR, "Requested font (NULL) not found.", "msPolylineLabelPath()");
    *status = MS_FAILURE;
    goto FAILURE;
  }
  
  text_length = letterspacing * (bbox.maxx - bbox.minx);

  /* If the text length is way longer than the line, skip adding the
     label if it isn't forced (long extrapolated labels tend to be
     ugly) */
  if ( text_length > 1.5 * max_line_length && label->force == MS_FALSE ) {
    *status = MS_FAILURE;
    goto FAILURE;
  }
  
  /* Allocate the labelpath */
  labelpath = (labelPathObj*)malloc(sizeof(labelPathObj));
  labelpath->path.numpoints = strlen(string);
  labelpath->path.point = (pointObj*)calloc(labelpath->path.numpoints,sizeof(pointObj));
  labelpath->angles = (double*)malloc(sizeof(double) * (labelpath->path.numpoints));
  msInitShape(&(labelpath->bounds));

  /* The bounds will have two points for each character plus an endpoint:
     the UL corners of each bbox will be tied together and the LL corners
     will be tied together. */
  bounds.numpoints = 2*strlen(string) + 1;
  bounds.point = (pointObj*)malloc(sizeof(pointObj) * bounds.numpoints);
                                   
  /* The points start at (max_line_length - text_length) / 2 in order to be centred */
  text_start_length = (max_line_length - text_length) / 2.0;
  
  /* The text is longer than the line: extrapolate the first and last segments */
  if ( text_start_length < 0.0 ) {
    
    j = 0;
    final_j = p->line[i].numpoints - 1;
    fwd_line_length = rev_line_length = 0;
    
  } else {

    /* Proceed until we've traversed text_start_length in distance */
    fwd_line_length = 0;
    j = 0;
    while ( fwd_line_length < text_start_length )
      fwd_line_length += segment_lengths[i][j++];

    j--;

    /* Determine the last segment */ 
    rev_line_length = 0;
    final_j = p->line[i].numpoints - 1;
    while ( rev_line_length < text_start_length ) {
      rev_line_length += segment_lengths[i][final_j - 1];
      final_j--;
    }
    final_j++;

  }

  if ( final_j == 0 )
    final_j = 1;
  
  /* Determine if the line is mostly left to right or right to left */
  direction = 0;
  k = j;
  while ( k < final_j ) {
    direction += p->line[i].point[k+1].x - p->line[i].point[k].x;
    k++;
  }
  
  if ( direction > 0 ) {

    /* j is already correct */
    inc = 1;

    /* Length of the segment containing the starting point */
    segment_length = segment_lengths[i][j];
    /* Determine how far along the segment we need to go */
    t = 1 - (fwd_line_length - text_start_length) / segment_length;
    
  } else {

    j = final_j;
    inc = -1;

    /* Length of the segment containing the starting point */
    segment_length = segment_lengths[i][j-1];
    t = 1 - (rev_line_length - text_start_length) / segment_length;
    
  }
    
  distance_along_segment = t * segment_length; /* Starting point */
  
  theta = 0;
  k = 0;
  w = 0;
  while ( k < labelpath->path.numpoints ) {
    int m;
    double x,y;
      
    x = t * (p->line[i].point[j+inc].x - p->line[i].point[j].x) + p->line[i].point[j].x;
    y = t * (p->line[i].point[j+inc].y - p->line[i].point[j].y) + p->line[i].point[j].y;

    /* Average this label point with its neighbors according to the
       smoothing kernel */
    
    if ( k == 0 ) {
      labelpath->path.point[k].x += (kernel[0] + kernel[1]) * x;      
      labelpath->path.point[k].y += (kernel[0] + kernel[1]) * y;      
      
    } else if ( k == 1 ) {
      labelpath->path.point[k].x += kernel[0] * x;
      labelpath->path.point[k].y += kernel[0] * y;
      
    } else if ( k == labelpath->path.numpoints - 2 ) {
      labelpath->path.point[k].x += kernel[4] * x;
      labelpath->path.point[k].y += kernel[4] * y;
      
    } else if ( k == labelpath->path.numpoints - 1 ) {
      labelpath->path.point[k].x += (kernel[3] + kernel[4]) * x;
      labelpath->path.point[k].y += (kernel[3] + kernel[4]) * y;
      }
      
    for (m = 0; m < 5; m++) {
      if ( m + k - 2 < 0 || m + k - 2 > labelpath->path.numpoints - 1 )
        continue;
      
      labelpath->path.point[k+m-2].x += kernel[m]*x;
      labelpath->path.point[k+m-2].y += kernel[m]*y;
    }
    
    w = letterspacing*offsets[k];
    
    /* Add the character's width to the distance along the line */
    distance_along_segment += w;

    /* If we still have segments left and we've past the current
       segment, move to the next one */
    
    if ( inc == 1 && j < p->line[i].numpoints - 2 ) {
      
      while ( j < p->line[i].numpoints - 2 && distance_along_segment > segment_lengths[i][j] ) {
        distance_along_segment -= segment_lengths[i][j];
        
        /* Move to next segment */
        j += inc;
      }

      segment_length = segment_lengths[i][j];
      
      
    } else if ( inc == -1 && j > 1 ) {

      while ( j > 1 && distance_along_segment > segment_lengths[i][j-1] ) {
        distance_along_segment -= segment_lengths[i][j-1];
        
        /* Move to next segment */
        j += inc;
      }
      
      segment_length = segment_lengths[i][j-1];
      
    }
    
    /* Recalculate interpolation parameter */
    t = distance_along_segment / segment_length;
    
    k++;
    
  }
  
  /* Pre-calc the character's centre y value.  Used for rotation adjustment.  */
  cy = -size / 2.0;
    
  labelpath->path.point[0].x /= kernel_normal;
  labelpath->path.point[0].y /= kernel_normal;
  
  /* Average the points and calculate each angle */
  for (k = 1; k <= labelpath->path.numpoints; k++) {

    if ( k < labelpath->path.numpoints ) {  
      labelpath->path.point[k].x /= kernel_normal;
      labelpath->path.point[k].y /= kernel_normal;
      dx = labelpath->path.point[k].x - labelpath->path.point[k-1].x;
      dy = labelpath->path.point[k].y - labelpath->path.point[k-1].y;
    } else {
      /* Handle the last character */
      dx = t * (p->line[i].point[j+inc].x - p->line[i].point[j].x) + p->line[i].point[j].x - labelpath->path.point[k-1].x;
      dy = t * (p->line[i].point[j+inc].y - p->line[i].point[j].y) + p->line[i].point[j].y - labelpath->path.point[k-1].y;
    }

    theta = -atan2(dy,dx);

    /* If the difference between subsequent angles is > 80% of 180deg
       bail because the line likely overlaps itself. */
    if ( k > 2 && abs(theta - labelpath->angles[k-2]) > 0.4 * MS_PI ) {
      *status = MS_FAILURE;
      goto FAILURE;
    }
      
    /* msDebug("s: %c (x,y): (%0.2f,%0.2f) t: %0.2f\n", string[k-1], labelpath->path.point[k-1].x, labelpath->path.point[k-1].y, theta); */

    labelpath->angles[k-1] = theta;
  
    /* Move the previous point so that when the character is rotated and
       placed it is centred on the line */
    cos_t = cos(theta);
    sin_t = sin(theta);
  
    w = letterspacing*offsets[k-1];

    cx = 0; /* Center the character vertically only */

    dx = - (cx * cos_t + cy * sin_t);
    dy = - (cy * cos_t - cx * sin_t);
  
    labelpath->path.point[k-1].x += dx;
    labelpath->path.point[k-1].y += dy;

    /* Calculate the bounds */
    bbox.minx = 0;
    bbox.maxx = w;
    bbox.maxy = 0;
    bbox.miny = -size;

    /* Add the label buffer to the bounds */
    bbox.maxx += label->buffer;
    bbox.maxy += label->buffer;
    bbox.minx -= label->buffer;
    bbox.miny -= label->buffer;
  
    if ( k < labelpath->path.numpoints ) {
      /* Transform the bbox too.  We take the UL and LL corners and rotate
         then translate them. */
      bounds.point[k-1].x = (bbox.minx * cos_t + bbox.maxy * sin_t) + labelpath->path.point[k-1].x;
      bounds.point[k-1].y = (bbox.maxy * cos_t - bbox.minx * sin_t) + labelpath->path.point[k-1].y;

      /* Start at end and work towards the half way point */
      bounds.point[bounds.numpoints - k - 1].x = (bbox.minx * cos_t + bbox.miny * sin_t) + labelpath->path.point[k-1].x;
      bounds.point[bounds.numpoints - k - 1].y = (bbox.miny * cos_t - bbox.minx * sin_t) + labelpath->path.point[k-1].y;

    } else {
    
      /* This is the last character in the string so we take the UR and LR
         corners of the bbox */
      bounds.point[k-1].x = (bbox.maxx * cos_t + bbox.maxy * sin_t) + labelpath->path.point[k-1].x;
      bounds.point[k-1].y = (bbox.maxy * cos_t - bbox.maxx * sin_t) + labelpath->path.point[k-1].y;
  
      bounds.point[bounds.numpoints - k - 1].x = (bbox.maxx * cos_t + bbox.miny * sin_t) + labelpath->path.point[k-1].x;
      bounds.point[bounds.numpoints - k - 1].y = (bbox.miny * cos_t - bbox.maxx * sin_t) + labelpath->path.point[k-1].y;
    }

  }

  /* Close the bounds */
  bounds.point[bounds.numpoints - 1].x = bounds.point[0].x;
  bounds.point[bounds.numpoints - 1].y = bounds.point[0].y;
  
  /* Convert the bounds to a shape and store them in the labelpath */
  if ( msAddLineDirectly(&(labelpath->bounds), &bounds) == MS_FAILURE ) {
    *status = MS_FAILURE;
    goto FAILURE;
  }
  
  msComputeBounds(&(labelpath->bounds));

  if ( segment_lengths ) {
    for ( i = 0; i < p->numlines; i++ )
      free(segment_lengths[i]);
    free(segment_lengths);
  }
  
  free(offsets);
  
  return labelpath;
#endif

FAILURE:
  if ( segment_lengths ) {
    for ( i = 0; i < p->numlines; i++ )
      free(segment_lengths[i]);
    free(segment_lengths);
  }
    
  if ( offsets )
    free(offsets);
  
  if ( labelpath ) {
    msFreeLabelPathObj(labelpath);
    labelpath = NULL;
  }
    
  return NULL;
}

/* ===========================================================================
   Pretty printing of primitive objects
   ======================================================================== */

void msRectToFormattedString(rectObj *rect, char *format, char *buffer, 
                             int buffer_length) 
{
    snprintf(buffer, buffer_length, format,
             rect->minx, rect->miny, rect->maxx, rect->maxy);
}

void msPointToFormattedString(pointObj *point, const char *format, 
                              char *buffer, int buffer_length) 
{
#ifdef USE_POINT_Z_M
    snprintf(buffer, buffer_length, format, point->x, point->y, point->z, point->m);
#else
    snprintf(buffer, buffer_length, format, point->x, point->y);
#endif
}


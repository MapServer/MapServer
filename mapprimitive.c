/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementations for rectObj, pointObj, lineObj, shapeObj, etc.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2008 Regents of the University of Minnesota.
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
#include "mapprimitive.h"
#include <assert.h>
#include <locale.h>



typedef enum {CLIP_LEFT, CLIP_MIDDLE, CLIP_RIGHT} CLIP_STATE;

#define CLIP_CHECK(min, a, max) ((a) < (min) ? CLIP_LEFT : ((a) > (max) ? CLIP_RIGHT : CLIP_MIDDLE));
#define ROUND(a)       ( (a) + 0.5 )
#define SWAP( a, b, t) ( (t) = (a), (a) = (b), (b) = (t) )
#define EDGE_CHECK( x0, x, x1) ((x) < MS_MIN( (x0), (x1)) ? CLIP_LEFT : ((x) > MS_MAX( (x0), (x1)) ? CLIP_RIGHT : CLIP_MIDDLE ))

#ifndef INFINITY
#define INFINITY (1.0e+30)
#endif
#define NEARZERO (1.0e-30) /* 1/INFINITY */

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
  char* pszGEOSStr;
  char* pszStr;
  pszGEOSStr = msGEOSShapeToWKT(shape);
  pszStr = (pszGEOSStr) ? msStrdup(pszGEOSStr) : NULL;
  msGEOSFreeWKT(pszGEOSStr);
  return pszStr;
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
  shape->renderer_cache = NULL;

  /* annotation component */
  shape->text = NULL;

  /* bookkeeping component */
  shape->classindex = 0; /* default class */
  shape->tileindex = shape->index = shape->resultindex = -1;

  shape->scratch = MS_FALSE; /* not a temporary/scratch shape */
}

int msCopyShape(shapeObj *from, shapeObj *to)
{
  int i;

  if(!from || !to) return(-1);

  for(i=0; i<from->numlines; i++)
    msAddLine(to, &(from->line[i])); /* copy each line */

  to->type = from->type;

  to->bounds.minx = from->bounds.minx;
  to->bounds.miny = from->bounds.miny;
  to->bounds.maxx = from->bounds.maxx;
  to->bounds.maxy = from->bounds.maxy;

  if(from->text) to->text = msStrdup(from->text);

  to->classindex = from->classindex;
  to->index = from->index;
  to->tileindex = from->tileindex;
  to->resultindex = from->resultindex;

  if(from->values) {
    to->values = (char **)msSmallMalloc(sizeof(char *)*from->numvalues);
    for(i=0; i<from->numvalues; i++)
      to->values[i] = msStrdup(from->values[i]);
    to->numvalues = from->numvalues;
  }

  to->geometry = NULL; /* GEOS code will build automatically if necessary */
  to->scratch = from->scratch;

  return(0);
}

void msFreeShape(shapeObj *shape)
{
  int c;

  if(!shape) return; /* for safety */

  for (c= 0; c < shape->numlines; c++)
    free(shape->line[c].point);

  if (shape->line) free(shape->line);
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
  if( line < 0 || line >= shape->numlines ) {
    assert( 0 );
    return;
  }

  free( shape->line[line].point );
  if( line < shape->numlines - 1 ) {
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
  for(i=0; i<shape->numlines; i++) {
    if(shape->line[i].numpoints > 0) {
      shape->bounds.minx = shape->bounds.maxx = shape->line[i].point[0].x;
      shape->bounds.miny = shape->bounds.maxy = shape->line[i].point[0].y;
      break;
    }
  }
  if(i == shape->numlines) return;

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
int msIsOuterRing(shapeObj *shape, int r)
{
  int i, status=MS_TRUE;
  int result1, result2;

  if(shape->numlines == 1) return(MS_TRUE);

  for(i=0; i<shape->numlines; i++) {
    if(i == r) continue;

    /*
    ** We have to test 2, or perhaps 3 points on the shape against the ring because
    ** it is possible that at most one point could touch the ring and the function
    ** msPointInPolygon() is indeterminite in that case. (bug #2434)
    */
    result1 = msPointInPolygon(&(shape->line[r].point[0]), &(shape->line[i]));
    result2 = msPointInPolygon(&(shape->line[r].point[1]), &(shape->line[i]));
    if(result1 == result2) { /* same result twice, neither point was on the edge */
      if(result1 == MS_TRUE) status = !status;
    } else { /* one of the first 2 points were on the edge of the ring, the next one isn't */
      if(msPointInPolygon(&(shape->line[r].point[2]), &(shape->line[i])) == MS_TRUE)
        status = !status;
    }

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
  MS_CHECK_ALLOC(list, sizeof(int)*shape->numlines, NULL);

  for(i=0; i<shape->numlines; i++)
    list[i] = msIsOuterRing(shape, i);

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
  MS_CHECK_ALLOC(list, sizeof(int)*shape->numlines, NULL);

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

  line->point = (pointObj *) msSmallRealloc(line->point, sizeof(pointObj) * line->numpoints);
  line->point[line->numpoints-1] = *point;

  return MS_SUCCESS;
}

int msAddLine(shapeObj *p, lineObj *new_line)
{
  lineObj lineCopy;

  lineCopy.numpoints = new_line->numpoints;
  lineCopy.point = (pointObj *) malloc(new_line->numpoints*sizeof(pointObj));
  MS_CHECK_ALLOC(lineCopy.point, new_line->numpoints*sizeof(pointObj), MS_FAILURE);

  memcpy( lineCopy.point, new_line->point, sizeof(pointObj) * new_line->numpoints );

  return msAddLineDirectly( p, &lineCopy );
}

/*
** Same as msAddLine(), except that this version "seizes" the points
** array from the passed in line and uses it instead of copying it.
*/
int msAddLineDirectly(shapeObj *p, lineObj *new_line)
{
  int c;

  if( p->numlines == 0 ) {
    p->line = (lineObj *) malloc(sizeof(lineObj));
    MS_CHECK_ALLOC(p->line, sizeof(lineObj), MS_FAILURE);
  } else {
    p->line = (lineObj *) realloc(p->line, (p->numlines+1)*sizeof(lineObj));
    MS_CHECK_ALLOC(p->line, (p->numlines+1)*sizeof(lineObj), MS_FAILURE);
  }

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
  lineObj line= {0,NULL};

  line.point = (pointObj *)msSmallMalloc(sizeof(pointObj)*5);

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
** Private implementation of the Sutherland-Cohen algorithm. Inspired by
** "Getting Graphic: Programming Fundamentals in C and C++" by Mark Finlay
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
  lineObj line= {0,NULL};
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
      && shape->bounds.miny >= rect.miny ) {
    return;
  }

  for(i=0; i<shape->numlines; i++) {

    line.point = (pointObj *)msSmallMalloc(sizeof(pointObj)*shape->line[i].numpoints);
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

    if(line.numpoints > 0) {
      msAddLineDirectly(&tmp, &line);
    } else {
      free(line.point);
      line.numpoints = 0; /* new line */
    }
  }

  for (i=0; i<shape->numlines; i++) free(shape->line[i].point);
  free(shape->line);

  shape->line = tmp.line;
  shape->numlines = tmp.numlines;
  msComputeBounds(shape);
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
  lineObj line= {0,NULL};

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
      && shape->bounds.miny >= rect.miny ) {
    return;
  }

  for(j=0; j<shape->numlines; j++) {

    line.point = (pointObj *)msSmallMalloc(sizeof(pointObj)*2*shape->line[j].numpoints+1); /* worst case scenario, +1 allows us to duplicate the 1st and last point */
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

      if (deltax > 0) { /*  points to right */
        xin = rect.minx;
        xout = rect.maxx;
      } else {
        xin = rect.maxx;
        xout = rect.minx;
      }
      if (deltay > 0) { /*  points up */
        yin = rect.miny;
        yout = rect.maxy;
      } else {
        yin = rect.maxy;
        yout = rect.miny;
      }

      tinx = (xin - x1)/deltax;
      tiny = (yin - y1)/deltay;

      if (tinx < tiny) { /* hits x first */
        tin1 = tinx;
        tin2 = tiny;
      } else {            /* hits y first */
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
              } else {
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
      msAddLineDirectly(&tmp, &line);
    } else {
      free(line.point);
    }
  } /* next line */

  for (i=0; i<shape->numlines; i++) free(shape->line[i].point);
  free(shape->line);

  shape->line = tmp.line;
  shape->numlines = tmp.numlines;
  msComputeBounds(shape);

  return;
}

/*
** offsets a point relative to an image position
*/
void msOffsetPointRelativeTo(pointObj *point, layerObj *layer)
{
  double x=0, y=0;
  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return;


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
  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return;


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

void msTransformShapeSimplify(shapeObj *shape, rectObj extent, double cellsize)
{
  int i,j,k,beforelast; /* loop counters */
  double dx,dy;
  pointObj *point;
  double inv_cs = 1.0 / cellsize; /* invert and multiply much faster */
  int ok = 0;
  if(shape->numlines == 0) return; /* nothing to transform */

  if(shape->type == MS_SHAPE_LINE) {
    /*
     * loop through the shape's lines, and do naive simplification
     * to discard the points that are too close to one another.
     * currently the threshold is to discard points if they fall
     * less than a pixel away from their predecessor.
     * the simplified line is guaranteed to contain at
     * least its first and last point
     */
    for(i=0; i<shape->numlines; i++) { /* for each part */
      if(shape->line[i].numpoints<2) {
        shape->line[i].numpoints=0;
        continue; /*skip degenerate lines*/
      }
      point=shape->line[i].point;
      /*always keep first point*/
      point[0].x = MS_MAP2IMAGE_X_IC_DBL(point[0].x, extent.minx, inv_cs);
      point[0].y = MS_MAP2IMAGE_Y_IC_DBL(point[0].y, extent.maxy, inv_cs);
      beforelast=shape->line[i].numpoints-1;
      for(j=1,k=1; j < beforelast; j++ ) { /*loop from second point to first-before-last point*/
        point[k].x = MS_MAP2IMAGE_X_IC_DBL(point[j].x, extent.minx, inv_cs);
        point[k].y = MS_MAP2IMAGE_Y_IC_DBL(point[j].y, extent.maxy, inv_cs);
        dx=(point[k].x-point[k-1].x);
        dy=(point[k].y-point[k-1].y);
        if(dx*dx+dy*dy>1)
          k++;
      }
      /* try to keep last point */
      point[k].x = MS_MAP2IMAGE_X_IC_DBL(point[j].x, extent.minx, inv_cs);
      point[k].y = MS_MAP2IMAGE_Y_IC_DBL(point[j].y, extent.maxy, inv_cs);
      /* discard last point if equal to the one before it */
      if(point[k].x!=point[k-1].x || point[k].y!=point[k-1].y) {
        shape->line[i].numpoints=k+1;
      } else {
        shape->line[i].numpoints=k;
      }
      /* skip degenerate line once more */
      if(shape->line[i].numpoints<2) {
        shape->line[i].numpoints=0;
      } else {
        ok = 1; /* we have at least one line with more than two points */
      }
    }
  } else if(shape->type == MS_SHAPE_POLYGON) {
    /*
     * loop through the shape's lines, and do naive simplification
     * to discard the points that are too close to one another.
     * currently the threshold is to discard points if they fall
     * less than a pixel away from their predecessor.
     * the simplified polygon is guaranteed to contain at
     * least its first, second and last point
     */
    for(i=0; i<shape->numlines; i++) { /* for each part */
      if(shape->line[i].numpoints<4) {
        shape->line[i].numpoints=0;
        continue; /*skip degenerate lines*/
      }
      point=shape->line[i].point;
      /*always keep first and second point*/
      point[0].x = MS_MAP2IMAGE_X_IC_DBL(point[0].x, extent.minx, inv_cs);
      point[0].y = MS_MAP2IMAGE_Y_IC_DBL(point[0].y, extent.maxy, inv_cs);
      point[1].x = MS_MAP2IMAGE_X_IC_DBL(point[1].x, extent.minx, inv_cs);
      point[1].y = MS_MAP2IMAGE_Y_IC_DBL(point[1].y, extent.maxy, inv_cs);
      beforelast=shape->line[i].numpoints-2;
      for(j=2,k=2; j < beforelast; j++ ) { /*loop from second point to second-before-last point*/
        point[k].x = MS_MAP2IMAGE_X_IC_DBL(point[j].x, extent.minx, inv_cs);
        point[k].y = MS_MAP2IMAGE_Y_IC_DBL(point[j].y, extent.maxy, inv_cs);
        dx=(point[k].x-point[k-1].x);
        dy=(point[k].y-point[k-1].y);
        if(dx*dx+dy*dy>1)
          k++;
      }
      /*always keep last two points (the last point is the repetition of the
       * first one */
      point[k].x = MS_MAP2IMAGE_X_IC_DBL(point[j].x, extent.minx, inv_cs);
      point[k].y = MS_MAP2IMAGE_Y_IC_DBL(point[j].y, extent.maxy, inv_cs);
      point[k+1].x = MS_MAP2IMAGE_X_IC_DBL(point[j+1].x, extent.minx, inv_cs);
      point[k+1].y = MS_MAP2IMAGE_Y_IC_DBL(point[j+1].y, extent.maxy, inv_cs);
      shape->line[i].numpoints = k+2;
      ok = 1;
    }
  } else { /* only for untyped shapes, as point layers don't go through this function */
    for(i=0; i<shape->numlines; i++) {
      point=shape->line[i].point;
      for(j=0; j<shape->line[i].numpoints; j++) {
        point[j].x = MS_MAP2IMAGE_X_IC_DBL(point[j].x, extent.minx, inv_cs);
        point[j].y = MS_MAP2IMAGE_Y_IC_DBL(point[j].y, extent.maxy, inv_cs);
      }
    }
    ok = 1;
  }
  if(!ok) {
    for(i=0; i<shape->numlines; i++) {
      free(shape->line[i].point);
    }
    shape->numlines = 0 ;
  }
}

/**
 * Generic function to transorm the shape coordinates to output coordinates
 */
void  msTransformShape(shapeObj *shape, rectObj extent, double cellsize, imageObj *image)
{
  if (image != NULL && MS_RENDERER_PLUGIN(image->format)) {
    rendererVTableObj *renderer = MS_IMAGE_RENDERER(image);
    if(renderer->transform_mode == MS_TRANSFORM_SNAPTOGRID) {
      msTransformShapeToPixelSnapToGrid(shape, extent, cellsize, renderer->approximation_scale);
    } else if(renderer->transform_mode == MS_TRANSFORM_SIMPLIFY) {
      msTransformShapeSimplify(shape, extent, cellsize);
    } else if(renderer->transform_mode == MS_TRANSFORM_ROUND) {
      msTransformShapeToPixelRound(shape, extent, cellsize);
    } else if(renderer->transform_mode == MS_TRANSFORM_FULLRESOLUTION) {
      msTransformShapeToPixelDoublePrecision(shape,extent,cellsize);
    } else if(renderer->transform_mode == MS_TRANSFORM_NONE) {
      /* nothing to do */
      return;
    }
    /* unknown, do nothing */
    return;
  }
  msTransformShapeToPixelRound(shape, extent, cellsize);
}

void msTransformShapeToPixelSnapToGrid(shapeObj *shape, rectObj extent, double cellsize, double grid_resolution)
{
  int i,j,k; /* loop counters */
  double inv_cs;
  if(shape->numlines == 0) return;
  inv_cs = 1.0 / cellsize; /* invert and multiply much faster */


  if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON) { /* remove duplicate vertices */
    for(i=0; i<shape->numlines; i++) { /* for each part */
      int snap = 1;
      double x0,y0,x1,y1,x2,y2;
      /*do a quick heuristic: will we risk having a degenerate shape*/
      if(shape->type == MS_SHAPE_LINE) {
        /*a line is degenerate if it has a single pixel. we check that the first and last pixel are different*/
        x0 = MS_MAP2IMAGE_X_IC_SNAP(shape->line[i].point[0].x, extent.minx, inv_cs, grid_resolution);
        y0 = MS_MAP2IMAGE_Y_IC_SNAP(shape->line[i].point[0].y, extent.maxy, inv_cs, grid_resolution);
        x1 = MS_MAP2IMAGE_X_IC_SNAP(shape->line[i].point[shape->line[i].numpoints-1].x, extent.minx, inv_cs, grid_resolution);
        y1 = MS_MAP2IMAGE_Y_IC_SNAP(shape->line[i].point[shape->line[i].numpoints-1].y, extent.maxy, inv_cs, grid_resolution);
        if(x0 == x1 && y0 == y1) {
          snap = 0;
        }
      } else if(shape->type == MS_SHAPE_POLYGON) {
        x0 = MS_MAP2IMAGE_X_IC_SNAP(shape->line[i].point[0].x, extent.minx, inv_cs, grid_resolution);
        y0 = MS_MAP2IMAGE_Y_IC_SNAP(shape->line[i].point[0].y, extent.maxy, inv_cs, grid_resolution);
        x1 = MS_MAP2IMAGE_X_IC_SNAP(shape->line[i].point[shape->line[i].numpoints/3].x, extent.minx, inv_cs, grid_resolution);
        y1 = MS_MAP2IMAGE_Y_IC_SNAP(shape->line[i].point[shape->line[i].numpoints/3].y, extent.maxy, inv_cs, grid_resolution);
        x2 = MS_MAP2IMAGE_X_IC_SNAP(shape->line[i].point[shape->line[i].numpoints/3*2].x, extent.minx, inv_cs, grid_resolution);
        y2 = MS_MAP2IMAGE_Y_IC_SNAP(shape->line[i].point[shape->line[i].numpoints/3*2].y, extent.maxy, inv_cs, grid_resolution);
        if((x0 == x1 && y0 == y1) ||
            (x0 == x2 && y0 == y2) ||
            (x1 == x2 && y1 == y2)) {
          snap = 0;
        }
      }
      if(snap) {
        shape->line[i].point[0].x = x0;
        shape->line[i].point[0].y = y0;
        for(j=1, k=1; j < shape->line[i].numpoints; j++ ) {
          shape->line[i].point[k].x = MS_MAP2IMAGE_X_IC_SNAP(shape->line[i].point[j].x, extent.minx, inv_cs, grid_resolution);
          shape->line[i].point[k].y = MS_MAP2IMAGE_Y_IC_SNAP(shape->line[i].point[j].y, extent.maxy, inv_cs, grid_resolution);
          if(shape->line[i].point[k].x!=shape->line[i].point[k-1].x || shape->line[i].point[k].y!=shape->line[i].point[k-1].y)
            k++;
        }
        shape->line[i].numpoints=k;
      } else {
        if(shape->type == MS_SHAPE_LINE) {
          shape->line[i].point[0].x = MS_MAP2IMAGE_X_IC_DBL(shape->line[i].point[0].x, extent.minx, inv_cs);
          shape->line[i].point[0].y = MS_MAP2IMAGE_Y_IC_DBL(shape->line[i].point[0].y, extent.maxy, inv_cs);
          shape->line[i].point[1].x = MS_MAP2IMAGE_X_IC_DBL(shape->line[i].point[shape->line[i].numpoints-1].x, extent.minx, inv_cs);
          shape->line[i].point[1].y = MS_MAP2IMAGE_Y_IC_DBL(shape->line[i].point[shape->line[i].numpoints-1].y, extent.maxy, inv_cs);
          shape->line[i].numpoints = 2;
        } else {
          for(j=0; j < shape->line[i].numpoints; j++ ) {
            shape->line[i].point[j].x = MS_MAP2IMAGE_X_IC_DBL(shape->line[i].point[j].x, extent.minx, inv_cs);
            shape->line[i].point[j].y = MS_MAP2IMAGE_Y_IC_DBL(shape->line[i].point[j].y, extent.maxy, inv_cs);
          }
        }
      }
    }
  } else { /* points or untyped shapes */
    for(i=0; i<shape->numlines; i++) { /* for each part */
      for(j=1; j < shape->line[i].numpoints; j++ ) {
        shape->line[i].point[j].x = MS_MAP2IMAGE_X_IC_DBL(shape->line[i].point[j].x, extent.minx, inv_cs);
        shape->line[i].point[j].y = MS_MAP2IMAGE_Y_IC_DBL(shape->line[i].point[j].y, extent.maxy, inv_cs);
      }
    }
  }

}

void msTransformShapeToPixelRound(shapeObj *shape, rectObj extent, double cellsize)
{
  int i,j,k; /* loop counters */
  double inv_cs;
  if(shape->numlines == 0) return;
  inv_cs = 1.0 / cellsize; /* invert and multiply much faster */
  if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON) { /* remove duplicate vertices */
    for(i=0; i<shape->numlines; i++) { /* for each part */
      shape->line[i].point[0].x = MS_MAP2IMAGE_X_IC(shape->line[i].point[0].x, extent.minx, inv_cs);;
      shape->line[i].point[0].y = MS_MAP2IMAGE_Y_IC(shape->line[i].point[0].y, extent.maxy, inv_cs);
      for(j=1, k=1; j < shape->line[i].numpoints; j++ ) {
        shape->line[i].point[k].x = MS_MAP2IMAGE_X_IC(shape->line[i].point[j].x, extent.minx, inv_cs);
        shape->line[i].point[k].y = MS_MAP2IMAGE_Y_IC(shape->line[i].point[j].y, extent.maxy, inv_cs);
        if(shape->line[i].point[k].x!=shape->line[i].point[k-1].x || shape->line[i].point[k].y!=shape->line[i].point[k-1].y)
          k++;
      }
      shape->line[i].numpoints=k;
    }
  } else { /* points or untyped shapes */
    for(i=0; i<shape->numlines; i++) { /* for each part */
      for(j=0; j < shape->line[i].numpoints; j++ ) {
        shape->line[i].point[j].x = MS_MAP2IMAGE_X_IC(shape->line[i].point[j].x, extent.minx, inv_cs);
        shape->line[i].point[j].y = MS_MAP2IMAGE_Y_IC(shape->line[i].point[j].y, extent.maxy, inv_cs);
      }
    }
  }

}

void msTransformShapeToPixelDoublePrecision(shapeObj *shape, rectObj extent, double cellsize)
{
  int i,j; /* loop counters */
  double inv_cs = 1.0 / cellsize; /* invert and multiply much faster */
  for(i=0; i<shape->numlines; i++) {
    for(j=0; j<shape->line[i].numpoints; j++) {
      shape->line[i].point[j].x = MS_MAP2IMAGE_X_IC_DBL(shape->line[i].point[j].x, extent.minx, inv_cs);
      shape->line[i].point[j].y = MS_MAP2IMAGE_Y_IC_DBL(shape->line[i].point[j].y, extent.maxy, inv_cs);
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

  if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON) { /* remove co-linear vertices */

    for(i=0; i<shape->numlines; i++) { /* for each part */
      for(j=0; j < shape->line[i].numpoints; j++ ) {
        shape->line[i].point[j].x = MS_IMAGE2MAP_X(shape->line[i].point[j].x, extent.minx, cellsize);
        shape->line[i].point[j].y = MS_IMAGE2MAP_Y(shape->line[i].point[j].y, extent.maxy, cellsize);
      }
    }
  } else { /* points or untyped shapes */

    for(i=0; i<shape->numlines; i++) { /* for each part */
      for(j=1; j < shape->line[i].numpoints; j++ ) {
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

    inside.point = (pointObj *)msSmallMalloc(sizeof(pointObj)*p->line[i].numpoints);
    outside.point = (pointObj *)msSmallMalloc(sizeof(pointObj)*p->line[i].numpoints);
    inside.numpoints = outside.numpoints = p->line[i].numpoints;

    angle = asin(MS_ABS(p->line[i].point[1].x - p->line[i].point[0].x)/sqrt((((p->line[i].point[1].x - p->line[i].point[0].x)*(p->line[i].point[1].x - p->line[i].point[0].x)) + ((p->line[i].point[1].y - p->line[i].point[0].y)*(p->line[i].point[1].y - p->line[i].point[0].y)))));
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

      angle = asin(MS_ABS(p->line[i].point[j].x - p->line[i].point[j-1].x)/sqrt((((p->line[i].point[j].x - p->line[i].point[j-1].x)*(p->line[i].point[j].x - p->line[i].point[j-1].x)) + ((p->line[i].point[j].y - p->line[i].point[j-1].y)*(p->line[i].point[j].y - p->line[i].point[j-1].y)))));
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

static double getRingArea(lineObj *ring)
{
  int i;
  double s=0;

  for(i=0; i<ring->numpoints-1; i++)
    s += (ring->point[i].x*ring->point[i+1].y - ring->point[i+1].x*ring->point[i].y);

  return (MS_ABS(s/2));
}

double msGetPolygonArea(shapeObj *p)
{
  int i;
  double area=0;

  for(i=0; i<p->numlines; i++) {
    if(msIsOuterRing(p, i))
      area += getRingArea(&(p->line[i]));
    else
      area -= getRingArea(&(p->line[i])); /* hole */
  }

  return area;
}

/*
** Computes the center of gravity for a polygon based on it's largest outer ring only.
*/
static int getPolygonCenterOfGravity(shapeObj *p, pointObj *lp)
{
  int i, j;
  double area=0;
  double sx=0, sy=0, tsx, tsy, s; /* sums */
  double a;

  double largestArea=0;

  for(i=0; i<p->numlines; i++) {
    tsx = tsy = s = 0; /* reset the ring sums */
    for(j=0; j<p->line[i].numpoints-1; j++) {
      a = p->line[i].point[j].x*p->line[i].point[j+1].y - p->line[i].point[j+1].x*p->line[i].point[j].y;
      s += a;
      tsx += (p->line[i].point[j].x + p->line[i].point[j+1].x)*a;
      tsy += (p->line[i].point[j].y + p->line[i].point[j+1].y)*a;
    }
    area = MS_ABS(s/2);

    if(area > largestArea) {
      largestArea = area;
      sx = s>0?tsx:-tsx;
      sy = s>0?tsy:-tsy;
    }
  }

  lp->x = sx/(6*largestArea);
  lp->y = sy/(6*largestArea);

  return MS_SUCCESS;
}

int msGetPolygonCentroid(shapeObj *p, pointObj *lp, double *miny, double *maxy)
{
  int i,j;
  double cent_weight_x=0.0, cent_weight_y=0.0;
  double len, total_len=0;

  *miny = *maxy = p->line[0].point[0].y;
  for(i=0; i<p->numlines; i++) {
    for(j=1; j<p->line[i].numpoints; j++) {
      *miny = MS_MIN(*miny, p->line[i].point[j].y);
      *maxy = MS_MAX(*maxy, p->line[i].point[j].y);
      len = msDistancePointToPoint(&(p->line[i].point[j-1]), &(p->line[i].point[j]));
      cent_weight_x += len * ((p->line[i].point[j-1].x + p->line[i].point[j].x)/2);
      cent_weight_y += len * ((p->line[i].point[j-1].y + p->line[i].point[j].y)/2);
      total_len += len;
    }
  }

  if(total_len == 0)
    return(MS_FAILURE);

  lp->x = cent_weight_x / total_len;
  lp->y = cent_weight_y / total_len;

  return(MS_SUCCESS);
}

/*
** Find a label point in a polygon.
*/
int msPolygonLabelPoint(shapeObj *p, pointObj *lp, double min_dimension)
{
  double slope;
  pointObj *point1=NULL, *point2=NULL, cp;
  int i, j, nfound;
  double x, y, *intersect, temp;
  double min, max;
  int wrong_order, n;
  double len, max_len=0;
  double minx, maxx, maxy, miny;

  int method = 2;

  msComputeBounds(p);
  minx = p->bounds.minx;
  miny = p->bounds.miny;
  maxx = p->bounds.maxx;
  maxy = p->bounds.maxy;

  if(min_dimension > 0)
    if(MS_MIN(maxx-minx,maxy-miny) < min_dimension) return(MS_FAILURE);

  cp.x = (maxx+minx)/2.0;
  cp.y = (maxy+miny)/2.0;

  switch (method) {
    case 0: /* MBR */
      lp->x = cp.x;
      lp->y = cp.y;
      break;
    case 1: /* centroid */
      if(msGetPolygonCentroid(p, lp, &miny, &maxy) != MS_SUCCESS) return(MS_FAILURE);
      break;
    case 2: /* center of gravity */
      if(getPolygonCenterOfGravity(p, lp) != MS_SUCCESS) return(MS_FAILURE);
      break;
  }

  if(msIntersectPointPolygon(lp, p) == MS_TRUE) {
    double dist, min_dist=-1;

    /* compute a distance to the polygon */
    for(j=0; j<p->numlines; j++) {
      for(i=1; i<p->line[j].numpoints; i++) {
        dist = msSquareDistancePointToSegment(lp, &(p->line[j].point[i-1]), &(p->line[j].point[i]));
        if((dist < min_dist) || (min_dist < 0)) min_dist = dist;
      }
    }
    min_dist = sqrt(min_dist);

    if(min_dist > .1*MS_MAX(maxx-minx, maxy-miny))
      return(MS_SUCCESS); /* point is not too close to the edge */
  }

  /* printf("label: %s\n", p->text);
     printf("    bbox: %g %g %g %g\n",minx, miny, maxx, maxy);
     printf("    center: %g %g\n", cp.x, cp.y);
     printf("    center of gravity: %g %g\n", lp->x, lp->y);
     printf("    dx: %g, dy: %g\n", lp->x-cp.x, lp->y-cp.y);
     printf("    distance to parent shape: %g\n", min_dist);
     return MS_SUCCESS; */

  n=0;
  for(j=0; j<p->numlines; j++) /* count total number of points */
    n += p->line[j].numpoints;
  intersect = (double *) calloc(n, sizeof(double));
  MS_CHECK_ALLOC(intersect, n*sizeof(double), MS_FAILURE);


  if(MS_ABS((int)lp->x - (int)cp.x) > MS_ABS((int)lp->y - (int)cp.y)) { /* center horizontally, fix y */

    y = lp->y;

    /* need to find a y that won't intersect any vertices exactly */
    max = y - 1; /* first initializing min, max to be any 2 pnts on either side of y */
    min = y + 1;
    for(j=0; j<p->numlines; j++) {
      if((min < y) && (max >= y))  break;
      for(i=0; i < p->line[j].numpoints; i++) {
        if((min < y) && (max >= y))  break;
        if(p->line[j].point[i].y < y)
          min = p->line[j].point[i].y;
        if(p->line[j].point[i].y >= y)
          max = p->line[j].point[i].y;
      }
    }

    n=0;
    for(j=0; j<p->numlines; j++) {
      for(i=0; i < p->line[j].numpoints; i++) {
        if((p->line[j].point[i].y < y) && ((y - p->line[j].point[i].y) < (y - min)))
          min = p->line[j].point[i].y;
        if((p->line[j].point[i].y >= y) && ((p->line[j].point[i].y - y) < (max - y)))
          max = p->line[j].point[i].y;
      }
    }

    if(min == max)
      return (MS_FAILURE);
    else
      y = (max + min)/2.0;

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
          intersect[nfound++] = x;
        } /* end checking this edge */

        point1 = point2; /* next edge */
      }
    } /* finished line */

    /* sort the intersections */
    do {
      wrong_order = 0;
      for(i=0; i < nfound-1; i++) {
        if(intersect[i] > intersect[i+1]) {
          wrong_order = 1;
          SWAP(intersect[i], intersect[i+1], temp);
        }
      }
    } while(wrong_order);

    /* find longest span */
    for(i=0; i < nfound; i += 2) {
      len = fabs(intersect[i] - intersect[i+1]);
      if(len > max_len) {
        max_len = len;
        lp->x = (intersect[i] + intersect[i+1])/2;
        /* lp->y = y; */
      }
    }
  } else { /* center vertically, fix x */
    x = lp->x;

    /* need to find a x that won't intersect any vertices exactly */
    max = x - 1; /* first initializing min, max to be any 2 pnts on either side of x */
    min = x + 1;
    for(j=0; j<p->numlines; j++) {
      if((min < x) && (max >= x))  break;
      for(i=0; i < p->line[j].numpoints; i++) {
        if((min < x) && (max >= x))  break;
        if(p->line[j].point[i].x < x)
          min = p->line[j].point[i].x;
        if(p->line[j].point[i].x >= x)
          max = p->line[j].point[i].x;
      }
    }

    n=0;
    for(j=0; j<p->numlines; j++) {
      for(i=0; i < p->line[j].numpoints; i++) {
        if((p->line[j].point[i].x < x) && ((x - p->line[j].point[i].x) < (x - min)))
          min = p->line[j].point[i].x;
        if((p->line[j].point[i].x >= x) && ((p->line[j].point[i].x - x) < (max - x)))
          max = p->line[j].point[i].x;
      }
    }

    if(min == max)
      return (MS_FAILURE);
    else
      x = (max + min)/2.0;

    nfound = 0;
    for(j=0; j<p->numlines; j++) { /* for each line */

      point1 = &( p->line[j].point[p->line[j].numpoints-1] );
      for(i=0; i < p->line[j].numpoints; i++) {
        point2 = &( p->line[j].point[i] );

        if(EDGE_CHECK(point1->x, x, point2->x) == CLIP_MIDDLE) {

          if(point1->x == point2->x)
            continue; /* ignore vertical edges */
          else if(point1->y == point2->y)
            y = point1->y; /* for a horizontal edge we know y */
          else {
            slope = (point2->x - point1->x) / (point2->y - point1->y);
            y = (x - point1->x)/slope + point1->y;
          }

          intersect[nfound++] = y;
        } /* end checking this edge */

        point1 = point2; /* next edge */
      }
    } /* finished line */

    /* sort the intersections */
    do {
      wrong_order = 0;
      for(i=0; i < nfound-1; i++) {
        if(intersect[i] > intersect[i+1]) {
          wrong_order = 1;
          SWAP(intersect[i], intersect[i+1], temp);
        }
      }
    } while(wrong_order);

    /* find longest span */
    for(i=0; i < nfound; i += 2) {
      len = fabs(intersect[i] - intersect[i+1]);
      if(len > max_len) {
        max_len = len;
        lp->y = (intersect[i] + intersect[i+1])/2;
        /* lp->x = x; */
      }
    }
  }

  free(intersect);

  if(max_len > 0)
    return(MS_SUCCESS);
  else
    return(MS_FAILURE);
}

/* Compute all the lineString/segment lengths and determine the longest lineString of a multiLineString
 * shape: in paramater, the multiLineString to compute.
 * segment_lengths: out parameter, the segment lengths of all lineString.
 * line_lengths: out parameter, the lineString lengths of the multiLineString.
 * max_line_index: out parameter, the index of the longest lineString of the multiLineString.
 * max_line_length: out parameter, the length of the longest lineString of the multiLineString.
 * total_length: out parameter, the total length of the MultiLineString
 * segment_index: out parameter, the index of the longest lineString of the multiLineString.
*/
void msPolylineComputeLineSegments(shapeObj *shape, double ***segment_lengths, double **line_lengths, int *max_line_index, double *max_line_length, int *segment_index, double *total_length)
{
  int i, j, temp_segment_index;
  double segment_length, max_segment_length;

  (*segment_lengths) = (double **) msSmallMalloc(sizeof(double *) * shape->numlines);
  (*line_lengths) = (double *) msSmallMalloc(sizeof(double) * shape->numlines);

  temp_segment_index = *segment_index = *max_line_index = 0;

  *total_length = 0;
  *max_line_length = 0;
  for(i=0; i<shape->numlines; i++) {

    (*segment_lengths)[i] = (double*) msSmallMalloc(sizeof(double) * shape->line[i].numpoints);

    (*line_lengths)[i] = 0;
    max_segment_length = 0;
    for(j=1; j<shape->line[i].numpoints; j++) {
      segment_length = sqrt((((shape->line[i].point[j].x-shape->line[i].point[j-1].x)*(shape->line[i].point[j].x-shape->line[i].point[j-1].x)) + ((shape->line[i].point[j].y-shape->line[i].point[j-1].y)*(shape->line[i].point[j].y-shape->line[i].point[j-1].y))));
      (*line_lengths)[i] += segment_length;
      (*segment_lengths)[i][j-1] = segment_length;
      if(segment_length > max_segment_length) {
        max_segment_length = segment_length;
        temp_segment_index = j;
      }
    }

    *total_length += (*line_lengths)[i];

    if((*line_lengths)[i] > *max_line_length) {
      *max_line_length = (*line_lengths)[i];
      *max_line_index = i;
      *segment_index = temp_segment_index;
    }
  }
}

/*
** If no repeatdistance, find center of longest segment in polyline p. The polyline must have been converted
** to image coordinates before calling this function.
*/
pointObj** msPolylineLabelPoint(shapeObj *p, int min_length, int repeat_distance, double ***angles, double ***lengths, int *numpoints, int anglemode)
{
  return msPolylineLabelPointExtended(p, min_length, repeat_distance, angles, lengths, numpoints, NULL, 0, anglemode);
}

pointObj** msPolylineLabelPointExtended(shapeObj *p, int min_length, int repeat_distance, double ***angles, double ***lengths, int *numpoints, int *regularLines, int numlines, int anglemode)
{
  double total_length, max_line_length;
  int i,j, max_line_index, segment_index, labelpoints_index, labelpoints_size;
  double** segment_lengths;
  double* line_lengths;
  pointObj** labelpoints;

  labelpoints_index = 0;
  labelpoints_size = p->numlines; /* minimal array size */
  *numpoints = 0;

  labelpoints = (pointObj **) msSmallMalloc(sizeof(pointObj *) * labelpoints_size);
  (*angles) = (double **) msSmallMalloc(sizeof(double *) * labelpoints_size);
  (*lengths) = (double **) msSmallMalloc(sizeof(double *) * labelpoints_size);

  msPolylineComputeLineSegments(p, &segment_lengths, &line_lengths, &max_line_index, &max_line_length, &segment_index, &total_length);

  if (repeat_distance > 0) {
    for(i=0; i<p->numlines; i++)
      if (numlines > 0) {
        for (j=0; j<numlines; j++)
          if (regularLines[j] == i) {
            msPolylineLabelPointLineString(p, min_length, repeat_distance, angles, lengths, segment_lengths, i, line_lengths[i], total_length, segment_index, &labelpoints_index, &labelpoints_size, &labelpoints, anglemode);
            break;
          }
      } else {
        msPolylineLabelPointLineString(p, min_length, repeat_distance, angles, lengths, segment_lengths, i, line_lengths[i], total_length, segment_index, &labelpoints_index, &labelpoints_size, &labelpoints, anglemode);
      }
  } else
    msPolylineLabelPointLineString(p, min_length, repeat_distance, angles, lengths, segment_lengths, max_line_index, max_line_length, total_length, segment_index, &labelpoints_index, &labelpoints_size, &labelpoints, anglemode);

  *numpoints = labelpoints_index;

  /* freeing memory: allocated by msPolylineComputeLineSegments */
  if ( segment_lengths ) {
    for ( i = 0; i < p->numlines; i++ )
      free(segment_lengths[i]);
    free(segment_lengths);
  }

  free(line_lengths);

  return labelpoints;
}

void msPolylineLabelPointLineString(shapeObj *p, int min_length, int repeat_distance, double ***angles, double ***lengths, double** segment_lengths,
                                    int line_index, double line_length, double total_length, int segment_index, int* labelpoints_index, int* labelpoints_size, pointObj ***labelpoints, int anglemode)
{
  int i, j, k, l, n, index, point_repeat;
  double t, tmp_length, theta, fwd_length, point_distance;
  double center_point_position, left_point_position, right_point_position, point_position;

  tmp_length = total_length;
  if (repeat_distance > 0)
    tmp_length = line_length;

  if((min_length != -1) && (tmp_length < min_length)) /* too short to label */
    return;

  i = line_index;

  if(p->line[i].numpoints < 2)
    return;
  point_distance = 0;
  point_repeat = 1;
  left_point_position = right_point_position = center_point_position = line_length / 2.0;

  if (repeat_distance > 0) {
    point_repeat = line_length / repeat_distance;

    if (point_repeat > 1) {
      if (point_repeat % 2 == 0)
        point_repeat -= 1;
      point_distance = (line_length / point_repeat); /* buffer allowed per point */

      /* initial point position */
      left_point_position -= ((point_repeat-1)/2 * point_distance);
      right_point_position += ((point_repeat-1)/2 * point_distance);

      point_repeat = (point_repeat-1)/2+1;
    } else
      point_repeat = 1;
  }

  for (l=0; l < point_repeat; ++l) {
    if (l == point_repeat-1) { /* last point to place is always the center point */
      point_position = center_point_position;
      n = 1;
    } else {
      point_position = right_point_position;
      n = 0;
    }

    do {
      if (*labelpoints_index == *labelpoints_size) {
        *labelpoints_size *= 2;
        (*labelpoints) = (pointObj **) msSmallRealloc(*labelpoints,sizeof(pointObj *) * (*labelpoints_size));
        (*angles) = (double **) msSmallRealloc(*angles,sizeof(double *) * (*labelpoints_size));
        (*lengths) = (double **) msSmallRealloc(*lengths,sizeof(double *) * (*labelpoints_size));
      }

      index = (*labelpoints_index)++;
      (*labelpoints)[index] = (pointObj *) msSmallMalloc(sizeof(pointObj));
      (*angles)[index] = (double *) msSmallMalloc(sizeof(double));
      (*lengths)[index] = (double *) msSmallMalloc(sizeof(double));

      if (repeat_distance > 0)
        *(*lengths)[index] = line_length;
      else
        *(*lengths)[index] = total_length;

      /* if there is only 1 label to place... put it in the middle of the current segment (as old behavior) */
      if ( ((anglemode == MS_AUTO) || (anglemode == MS_AUTO2)) &&
           (point_repeat == 1) ) {
        j = segment_index;
        (*labelpoints)[index]->x = (p->line[i].point[j].x + p->line[i].point[j-1].x)/2.0;
        (*labelpoints)[index]->y = (p->line[i].point[j].y + p->line[i].point[j-1].y)/2.0;
      } else {
        j=0;
        fwd_length = 0;
        while (fwd_length < point_position) {
          fwd_length += segment_lengths[i][j++];
        }

        k = j-1;

        t = 1 - (fwd_length - point_position) / segment_lengths[i][j-1];
        (*labelpoints)[index]->x = t * (p->line[i].point[k+1].x - p->line[i].point[k].x) + p->line[i].point[k].x;
        (*labelpoints)[index]->y = t * (p->line[i].point[k+1].y - p->line[i].point[k].y) + p->line[i].point[k].y;
      }

      if(anglemode != MS_NONE) {
        theta = atan2(p->line[i].point[j].x - p->line[i].point[j-1].x, p->line[i].point[j].y - p->line[i].point[j-1].y);
        if(anglemode == MS_AUTO2) {
          *(*angles)[index] = (MS_RAD_TO_DEG*theta) - 90;
        } else { /* AUTO, FOLLOW */
          if(p->line[i].point[j-1].x < p->line[i].point[j].x) { /* i.e. to the left */
            *(*angles)[index] = (MS_RAD_TO_DEG*theta) - 90;
          } else {
            *(*angles)[index] = (MS_RAD_TO_DEG*theta) + 90;
          }
        }
      }

      point_position = left_point_position;
      n++;
    } while (n<2); /* we place the right point then the left point. */

    right_point_position -= point_distance;
    left_point_position += point_distance;
  }

  return;
}

/* Calculate the labelpath for each line if repeatdistance is enabled, else the labelpath of the longest line segment */
labelPathObj** msPolylineLabelPath(mapObj *map, imageObj *img,shapeObj *p, int min_length, fontSetObj *fontset, char *string, labelObj *label, double scalefactor, int *numpaths,
                                   int** regular_lines, int* num_regular_lines)
{
  double max_line_length, total_length;
  double **segment_lengths, *line_lengths;
  int i, segment_index, max_line_index, labelpaths_index, labelpaths_size, regular_lines_index, regular_lines_size;
  labelPathObj** labelpaths;

  labelpaths_index = 0;
  labelpaths_size = p->numlines; /* minimal array size */
  regular_lines_index = 0;
  regular_lines_size = 1;
  *numpaths = 0;
  segment_index = max_line_index = 0;
  total_length = max_line_length = 0.0;

  if(!string) return NULL;


  labelpaths = (labelPathObj **) msSmallMalloc(sizeof(labelPathObj *) * labelpaths_size);
  (*regular_lines) = (int *) msSmallMalloc(sizeof(int) * regular_lines_size);

  msPolylineComputeLineSegments(p, &segment_lengths, &line_lengths, &max_line_index, &max_line_length, &segment_index, &total_length);

  if(label->repeatdistance > 0)
    for(i=0; i<p->numlines; i++) {
      msPolylineLabelPathLineString(map,img, p,min_length, fontset, string, label, scalefactor, i, segment_lengths, line_lengths[i], total_length,
                                    &labelpaths_index, &labelpaths_size, &labelpaths, regular_lines, &regular_lines_index, &regular_lines_size);
    }
  else
    msPolylineLabelPathLineString(map, img, p,min_length, fontset, string, label, scalefactor, max_line_index, segment_lengths, line_lengths[max_line_index], total_length,
                                  &labelpaths_index, &labelpaths_size, &labelpaths, regular_lines, &regular_lines_index, &regular_lines_size);

  /* freeing memory: allocated by msPolylineComputeLineSegments */
  if ( segment_lengths ) {
    for ( i = 0; i < p->numlines; i++ )
      free(segment_lengths[i]);
    free(segment_lengths);
  }

  free(line_lengths);

  /* set the number of paths in the array */
  *numpaths = labelpaths_index;
  *num_regular_lines = regular_lines_index;
  return labelpaths;
}

/*
 * Calculate a series of label points for each character in the label for a
 * given polyline. The resultant series of points is stored in *labelpath,
 * which is added to the labelpaths array. Note that the points and bounds
 * are allocated in this function.  The polyline must be converted to image
 * coordinates before calling this function.
 */
void msPolylineLabelPathLineString(mapObj *map, imageObj *img, shapeObj *p, int min_length, fontSetObj *fontset, char *string, labelObj *label, double scalefactor, int line_index,
                                   double** segment_lengths, double line_length, double total_length, int* labelpaths_index, int* labelpaths_size, labelPathObj*** labelpaths,
                                   int** regular_lines, int *regular_lines_index, int* regular_lines_size)
{
  double distance_along_segment;
  double segment_length, fwd_line_length, rev_line_length, text_length, text_start_length, label_buffer, text_end_length;
  double right_label_position, left_label_position, center_label_position;
  int numchars;

  int i,j,k,l,n, inc, final_j, label_repeat;
  double direction;
  rectObj bbox;
  lineObj bounds;
  double *offsets;
  double size, t, tmp_length;
  double cx, cy; /* centre of a character, x & y values. */

  double theta;
  double dx, dy, w, cos_t, sin_t;

  labelPathObj *labelpath = NULL;

  /* Line smoothing kernel */
  double kernel[] = {0.1, 0.2, 2, 0.2, 0.1}; /* {1.5, 2, 15, 2, 1.5}; */
  double kernel_normal = 2.6; /* Must be sum of kernel elements */
  int kernel_size = 5;

  double letterspacing = 1.05;
  /* As per RFC 60, if label->maxoverlapangle == 0 then fall back on pre-6.0 behavior
     which was to use maxoverlapangle = 0.4*MS_PI ( 40% of 180 degrees ) */
  double maxoverlapangle = 0.4 * MS_PI;

  offsets = NULL;

  tmp_length = total_length;
  if (label->repeatdistance > 0)
    tmp_length = line_length;

  numchars = msGetNumGlyphs(string);

  /* skip the label and use the normal algorithm if it has fewer than 2 characters */
  if(numchars < 2)
    goto ANGLEFOLLOW_FAILURE;

  i = line_index;

  if (((min_length != -1) && (tmp_length < min_length))) { /* too short */
    goto FAILURE;
  }

  if(p->line[i].numpoints < 2) { /* degenerate */
    goto FAILURE;
  }

  if(p->line[i].numpoints == 2) /* use the regular angled text algorithm */
    goto ANGLEFOLLOW_FAILURE;

  size = label->size*scalefactor;
  size = MS_MAX(size, label->minsize*img->resolutionfactor);
  size = MS_MIN(size, label->maxsize*img->resolutionfactor);

  /* determine the total length of the text */
  if (msGetLabelSize(map,label,string,size,&bbox,&offsets) != MS_SUCCESS) {
    goto FAILURE;
  }


  scalefactor = size / label->size;

  text_length = letterspacing * (bbox.maxx - bbox.minx);

  /*
  ** if the text length is way longer than the line, skip adding the
  ** label if it isn't forced (long extrapolated labels tend to be ugly)
  */
  if ( text_length > 1.5 * line_length && label->force == MS_FALSE ) {
    goto FAILURE;
  }

  /* We compute the number of labels we can repeat in the line */
  text_end_length = 0;
  left_label_position = right_label_position = center_label_position = (line_length - text_length) / 2.0;
  label_repeat = (line_length / (text_length + label->repeatdistance));
  label_buffer = (line_length / label_repeat); /* buffer allowed per label */
  if (label->repeatdistance > 0 && label_repeat > 1) {
    if (label_repeat % 2 == 0) {
      label_repeat -= 1;
      label_buffer = (line_length / label_repeat);
    }
    /* text_start_length = (label_buffer / 2) - (text_length / 2); */

    /* initial point position */
    left_label_position -= ((label_repeat-1)/2 * label_buffer);
    right_label_position += ((label_repeat-1)/2 * label_buffer);

    label_repeat = (label_repeat-1)/2+1;
  } else {
    label_repeat = 1;
    center_label_position = (line_length - text_length) / 2.0;
  }

  if(label->maxoverlapangle >=0)
    maxoverlapangle = label->maxoverlapangle * MS_DEG_TO_RAD; /* radian */

  for (l=0; l < label_repeat; l++) {
    if (l == label_repeat-1) { /* last label to place is always the center label */
      text_start_length = center_label_position;
      n = 1;
    } else {
      text_start_length = right_label_position;
      n = 0;
    }

    do {
      /* allocate the labelpath */
      labelpath = (labelPathObj *) msSmallMalloc(sizeof(labelPathObj));
      labelpath->path.numpoints = numchars;
      labelpath->path.point = (pointObj *) msSmallCalloc(labelpath->path.numpoints, sizeof(pointObj));
      labelpath->angles = (double *) msSmallMalloc(sizeof(double) * (labelpath->path.numpoints));
      msInitShape(&(labelpath->bounds));


      /*
      ** The bounds will have two points for each character plus an endpoint:
      ** the UL corners of each bbox will be tied together and the LL corners
      ** will be tied together.
      */
      bounds.numpoints = 2*numchars + 1;
      bounds.point = (pointObj *) msSmallMalloc(sizeof(pointObj) * bounds.numpoints);

      /* the points start at (line_length - text_length) / 2 in order to be centred */
      /* text_start_length = (line_length - text_length) / 2.0; */

      /* the text is longer than the line: extrapolate the first and last segments */
      if(text_start_length < 0.0) {
        j = 0;
        final_j = p->line[i].numpoints - 1;
        fwd_line_length = rev_line_length = 0;
      } else {
        /* proceed until we've traversed text_start_length in distance */
        fwd_line_length = 0;
        j = 0;
        while ( fwd_line_length < text_start_length )
          fwd_line_length += segment_lengths[i][j++];

        j--;

        /* determine the last segment */
        rev_line_length = 0;
        final_j = p->line[i].numpoints - 1;

        text_end_length = line_length - (text_start_length + text_length);
        while(rev_line_length < text_end_length) {
          rev_line_length += segment_lengths[i][final_j - 1];
          final_j--;
        }
        final_j++;
      }

      if(final_j == 0)
        final_j = 1;

      /* determine if the line is mostly left to right or right to left,
         see bug 1620 discussion by Steve Woodbridge */
      direction = 0;
      k = j;
      while (k < final_j) {
        direction += p->line[i].point[k+1].x - p->line[i].point[k].x;
        k++;
      }

      if(direction > 0) {
        inc = 1; /* j is already correct */

        /* length of the segment containing the starting point */
        segment_length = segment_lengths[i][j];

        /* determine how far along the segment we need to go */
        if(text_start_length < 0.0)
          t = text_start_length / segment_length;
        else
          t = 1 - (fwd_line_length - text_start_length) / segment_length;
      } else {
        j = final_j;
        inc = -1;

        /* length of the segment containing the starting point */
        segment_length = segment_lengths[i][j-1];
        if(text_start_length < 0.0)
          t = text_start_length / segment_length;
        else
          t = 1 - (rev_line_length - text_end_length) / segment_length;
      }

      distance_along_segment = t * segment_length; /* starting point */

      theta = 0;
      k = 0;
      w = 0;
      while ( k < labelpath->path.numpoints ) {
        int m;
        double x,y;

        x = t * (p->line[i].point[j+inc].x - p->line[i].point[j].x) + p->line[i].point[j].x;
        y = t * (p->line[i].point[j+inc].y - p->line[i].point[j].y) + p->line[i].point[j].y;

        /*
        ** This used to be a series of if-then-else's, but that fails for short (length < 4)
        ** labels. There may be small speed-ups possible here. (bug 1921)
        **
        ** average this label point with its neighbors according to the smoothing kernel
        */
        if ( k == 0) {
          labelpath->path.point[k].x += (kernel[0] + kernel[1]) * x;
          labelpath->path.point[k].y += (kernel[0] + kernel[1]) * y;
        }
        if ( k == 1) {
          labelpath->path.point[k].x += kernel[0] * x;
          labelpath->path.point[k].y += kernel[0] * y;
        }
        if ( k == labelpath->path.numpoints - 2) {
          labelpath->path.point[k].x += kernel[4] * x;
          labelpath->path.point[k].y += kernel[4] * y;
        }
        if ( k == labelpath->path.numpoints - 1) {
          labelpath->path.point[k].x += (kernel[3] + kernel[4]) * x;
          labelpath->path.point[k].y += (kernel[3] + kernel[4]) * y;
        }

        for(m = 0; m < kernel_size; m++) {
          if(m + k - 2 < 0 || m + k - 2 > labelpath->path.numpoints - 1)
            continue;
          labelpath->path.point[k+m-2].x += kernel[m]*x;
          labelpath->path.point[k+m-2].y += kernel[m]*y;
        }

        w = letterspacing*offsets[k];

        /* add the character's width to the distance along the line */
        distance_along_segment += w;

        /* if we still have segments left and we've past the current segment, move to the next one */
        if(inc == 1 && j < p->line[i].numpoints - 2) {

          while ( j < p->line[i].numpoints - 2 && distance_along_segment > segment_lengths[i][j] ) {
            distance_along_segment -= segment_lengths[i][j];
            j += inc; /* move to next segment */
          }

          segment_length = segment_lengths[i][j];

        } else if( inc == -1 && j > 1 ) {

          while ( j > 1 && distance_along_segment > segment_lengths[i][j-1] ) {
            distance_along_segment -= segment_lengths[i][j-1];
            j += inc; /* move to next segment */
          }

          segment_length = segment_lengths[i][j-1];
        }

        /* Recalculate interpolation parameter */
        t = distance_along_segment / segment_length;

        k++;
      }

      /* pre-calc the character's centre y value.  Used for rotation adjustment. */
      cy = -size / 2.0;

      labelpath->path.point[0].x /= kernel_normal;
      labelpath->path.point[0].y /= kernel_normal;

      /* Average the points and calculate each angle */
      for (k = 1; k <= labelpath->path.numpoints; k++) {
        double anglediff;
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

        if ( maxoverlapangle > 0 && k > 1) {
          /* If the difference between the last char angle and the current one
             is greater than the MAXOVERLAPANGLE value (set at 80% of 180deg by default)
             , bail the label */
          anglediff = fabs(theta - labelpath->angles[k-2]);
          anglediff = MS_MIN(anglediff, MS_2PI - anglediff);
          if(anglediff > maxoverlapangle ) {
            goto LABEL_FAILURE;
          }
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
        goto LABEL_FAILURE;
      }

      msComputeBounds(&(labelpath->bounds));

      goto LABEL_END;

LABEL_FAILURE:

      if (bounds.point)
        free(bounds.point);
      bounds.point = NULL;
      bounds.numpoints = 0;

      if ( labelpath ) {
        msFreeLabelPathObj(labelpath);
        labelpath = NULL;
      }

LABEL_END:
      if (labelpath) {
        if (*labelpaths_index == *labelpaths_size) {
          *labelpaths_size *= 2;
          (*labelpaths) = (labelPathObj **) msSmallRealloc(*labelpaths,sizeof(labelPathObj *) * (*labelpaths_size));
        }
        (*labelpaths)[(*labelpaths_index)++] = labelpath;
      }

      text_start_length = left_label_position;
      n++;
    } while (n<2);

    right_label_position -= label_buffer;
    left_label_position += label_buffer;

  }

  goto END; /* normal exit */

ANGLEFOLLOW_FAILURE: /* Angle follow failure: add the line index in the arrays */
  if (*regular_lines_index == *regular_lines_size) {
    *regular_lines_size *= 2;
    (*regular_lines) = (int*) msSmallRealloc(*regular_lines,sizeof(int) * (*regular_lines_size));
  }
  (*regular_lines)[(*regular_lines_index)++] = line_index;

FAILURE: /* Global failure */

END:
  if ( offsets )
    free(offsets);

  return;
}

/* ===========================================================================
   Pretty printing of primitive objects
   ======================================================================== */

void msRectToFormattedString(rectObj *rect, char *format, char *buffer, int buffer_length)
{
  snprintf(buffer, buffer_length, format, rect->minx, rect->miny, rect->maxx, rect->maxy);
}

void msPointToFormattedString(pointObj *point, const char *format, char *buffer, int buffer_length)
{
#ifdef USE_POINT_Z_M
  snprintf(buffer, buffer_length, format, point->x, point->y, point->z, point->m);
#else
  snprintf(buffer, buffer_length, format, point->x, point->y);
#endif
}

/* Returns true if a shape contains only degenerate parts */
int msIsDegenerateShape(shapeObj *shape)
{
  int i;
  int non_degenerate_parts = 0;
  for(i=0; i<shape->numlines; i++) { /* e.g. part */

    /* skip degenerate parts, really should only happen with pixel output */
    if((shape->type == MS_SHAPE_LINE && shape->line[i].numpoints < 2) ||
        (shape->type == MS_SHAPE_POLYGON && shape->line[i].numpoints < 3))
      continue;

    non_degenerate_parts++;
  }
  return( non_degenerate_parts == 0 );
}


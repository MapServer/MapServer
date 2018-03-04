/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  projectionObj / PROJ.4 interface.
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
 ****************************************************************************/

#include "mapserver.h"
#include "mapproject.h"
#include "mapthread.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mapaxisorder.h"



#ifdef USE_PROJ
static int msTestNeedWrap( pointObj pt1, pointObj pt2, pointObj pt2_geo,
                           projectionObj *src_proj,
                           projectionObj *dst_proj );
#endif


/************************************************************************/
/*                           int msIsAxisInverted                       */
/*      Check to see if we should invert the axis.                       */
/*                                                                      */
/************************************************************************/
int msIsAxisInverted(int epsg_code)
{
  const unsigned int row = epsg_code / 8;
  const unsigned char index = epsg_code % 8;

  /*check the static table*/
  if ((row < sizeof(axisOrientationEpsgCodes)) && (axisOrientationEpsgCodes[row] & (1 << index)))
    return MS_TRUE;
  else
    return MS_FALSE;
}

/************************************************************************/
/*                           msProjectPoint()                           */
/************************************************************************/
int msProjectPoint(projectionObj *in, projectionObj *out, pointObj *point)
{
#ifdef USE_PROJ
  projUV p;
  int  error;

  if( in && in->gt.need_geotransform ) {
    double x_out, y_out;

    x_out = in->gt.geotransform[0]
            + in->gt.geotransform[1] * point->x
            + in->gt.geotransform[2] * point->y;
    y_out = in->gt.geotransform[3]
            + in->gt.geotransform[4] * point->x
            + in->gt.geotransform[5] * point->y;

    point->x = x_out;
    point->y = y_out;
  }

  /* -------------------------------------------------------------------- */
  /*      If the source and destination are simple and equal, then do     */
  /*      nothing.                                                        */
  /* -------------------------------------------------------------------- */
  if( in && in->numargs == 1 && out && out->numargs == 1
      && strcmp(in->args[0],out->args[0]) == 0 ) {
    /* do nothing, no transformation required */
  }

  /* -------------------------------------------------------------------- */
  /*      If we have a fully defined input coordinate system and          */
  /*      output coordinate system, then we will use pj_transform.        */
  /* -------------------------------------------------------------------- */
  else if( in && in->proj && out && out->proj ) {
    double  z = 0.0;

    if( pj_is_latlong(in->proj) ) {
      point->x *= DEG_TO_RAD;
      point->y *= DEG_TO_RAD;
    }

#if PJ_VERSION < 480
    msAcquireLock( TLOCK_PROJ );
#endif
    error = pj_transform( in->proj, out->proj, 1, 0,
                          &(point->x), &(point->y), &z );
#if PJ_VERSION < 480
    msReleaseLock( TLOCK_PROJ );
#endif

    if( error || point->x == HUGE_VAL || point->y == HUGE_VAL ) {
//      msSetError(MS_PROJERR,"proj says: %s","msProjectPoint()",pj_strerrno(error));
      return MS_FAILURE;
    }

    if( pj_is_latlong(out->proj) ) {
      point->x *= RAD_TO_DEG;
      point->y *= RAD_TO_DEG;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Otherwise we fallback to using pj_fwd() or pj_inv() and         */
  /*      assuming that the NULL projectionObj is supposed to be          */
  /*      lat/long in the same datum as the other projectionObj.  This    */
  /*      is essentially a backwards compatibility mode.                  */
  /* -------------------------------------------------------------------- */
  else {
    /* nothing to do if the other coordinate system is also lat/long */
    if( in == NULL && (out == NULL || pj_is_latlong(out->proj) ))
      return MS_SUCCESS;
    if( out == NULL && in != NULL && pj_is_latlong(in->proj) )
      return MS_SUCCESS;

    p.u = point->x;
    p.v = point->y;

    if(in==NULL || in->proj==NULL) { /* input coordinates are lat/lon */
      p.u *= DEG_TO_RAD; /* convert to radians */
      p.v *= DEG_TO_RAD;
      p = pj_fwd(p, out->proj);
    } else {
      if(out==NULL || out->proj==NULL) { /* output coordinates are lat/lon */
        p = pj_inv(p, in->proj);
        p.u *= RAD_TO_DEG; /* convert to decimal degrees */
        p.v *= RAD_TO_DEG;
      } else { /* need to go from one projection to another */
        p = pj_inv(p, in->proj);
        p = pj_fwd(p, out->proj);
      }
    }

    if( p.u == HUGE_VAL || p.v == HUGE_VAL )
      return MS_FAILURE;

    point->x = p.u;
    point->y = p.v;
  }

  if( out && out->gt.need_geotransform ) {
    double x_out, y_out;

    x_out = out->gt.invgeotransform[0]
            + out->gt.invgeotransform[1] * point->x
            + out->gt.invgeotransform[2] * point->y;
    y_out = out->gt.invgeotransform[3]
            + out->gt.invgeotransform[4] * point->x
            + out->gt.invgeotransform[5] * point->y;

    point->x = x_out;
    point->y = y_out;
  }

  return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectPoint()");
  return(MS_FAILURE);
#endif
}

/************************************************************************/
/*                         msProjectGrowRect()                          */
/************************************************************************/
#ifdef USE_PROJ
static void msProjectGrowRect(projectionObj *in, projectionObj *out,
                              rectObj *prj_rect, int *rect_initialized,
                              pointObj *prj_point, int *failure )

{
  if( msProjectPoint(in, out, prj_point) == MS_SUCCESS ) {
    if( *rect_initialized ) {
      prj_rect->miny = MS_MIN(prj_rect->miny, prj_point->y);
      prj_rect->maxy = MS_MAX(prj_rect->maxy, prj_point->y);
      prj_rect->minx = MS_MIN(prj_rect->minx, prj_point->x);
      prj_rect->maxx = MS_MAX(prj_rect->maxx, prj_point->x);
    } else {
      prj_rect->minx = prj_rect->maxx = prj_point->x;
      prj_rect->miny = prj_rect->maxy = prj_point->y;
      *rect_initialized = MS_TRUE;
    }
  } else
    (*failure)++;
}
#endif /* def USE_PROJ */

/************************************************************************/
/*                          msProjectSegment()                          */
/*                                                                      */
/*      Interpolate along a line segment for which one end              */
/*      reprojects and the other end does not.  Finds the horizon.      */
/************************************************************************/
#ifdef USE_PROJ
static int msProjectSegment( projectionObj *in, projectionObj *out,
                             pointObj *start, pointObj *end )

{
  pointObj testPoint, subStart, subEnd;

  /* -------------------------------------------------------------------- */
  /*      Without loss of generality we assume the first point            */
  /*      reprojects, and the second doesn't.  If that is not the case    */
  /*      then re-call with the points reversed.                          */
  /* -------------------------------------------------------------------- */
  testPoint = *start;
  if( msProjectPoint( in, out, &testPoint ) == MS_FAILURE ) {
    testPoint = *end;
    if( msProjectPoint( in, out, &testPoint ) == MS_FAILURE )
      return MS_FAILURE;
    else
      return msProjectSegment( in, out, end, start );
  }

  /* -------------------------------------------------------------------- */
  /*      We will apply a binary search till we are within out            */
  /*      tolerance.                                                      */
  /* -------------------------------------------------------------------- */
  subStart = *start;
  subEnd = *end;

#define TOLERANCE 0.01

  while( fabs(subStart.x - subEnd.x)
         + fabs(subStart.y - subEnd.y) > TOLERANCE ) {
    pointObj midPoint;

    midPoint.x = (subStart.x + subEnd.x) * 0.5;
    midPoint.y = (subStart.y + subEnd.y) * 0.5;

    testPoint = midPoint;

    if( msProjectPoint( in, out, &testPoint ) == MS_FAILURE ) {
      if (midPoint.x == subEnd.x && midPoint.y == subEnd.y)
        return MS_FAILURE; /* break infinite loop */

      subEnd = midPoint;
    }
    else {
      if (midPoint.x == subStart.x && midPoint.y == subStart.y)
        return MS_FAILURE; /* break infinite loop */

      subStart = midPoint;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Now reproject the end points and return.                        */
  /* -------------------------------------------------------------------- */
  *end = subStart;

  if( msProjectPoint( in, out, end ) == MS_FAILURE
      || msProjectPoint( in, out, start ) == MS_FAILURE )
    return MS_FAILURE;
  else
    return MS_SUCCESS;
}
#endif

/************************************************************************/
/*                         msProjectShapeLine()                         */
/*                                                                      */
/*      Reproject a single linestring, potentially splitting into       */
/*      more than one line string if there are over the horizon         */
/*      portions.                                                       */
/*                                                                      */
/*      For polygons, no splitting takes place, but over the horizon    */
/*      points are clipped, and one segment is run from the fall        */
/*      over the horizon point to the come back over the horizon point. */
/************************************************************************/

#ifdef USE_PROJ
static int
msProjectShapeLine(projectionObj *in, projectionObj *out,
                   shapeObj *shape, int line_index)

{
  int i;
  pointObj  lastPoint, thisPoint, wrkPoint;
  lineObj *line = shape->line + line_index;
  lineObj *line_out = line;
  int valid_flag = 0; /* 1=true, -1=false, 0=unknown */
  int numpoints_in = line->numpoints;
  int line_alloc = numpoints_in;
  int wrap_test;

#ifdef USE_PROJ_FASTPATHS
#define MAXEXTENT 20037508.34
#define M_PIby360 .0087266462599716479
#define MAXEXTENTby180 111319.4907777777777777777
#define p_x line->point[i].x
#define p_y line->point[i].y
  if(in->wellknownprojection == wkp_lonlat && out->wellknownprojection == wkp_gmerc) {
    for( i = line->numpoints-1; i >= 0; i-- ) {
      p_x *= MAXEXTENTby180;
      p_y = log(tan((90 + p_y) * M_PIby360)) * MS_RAD_TO_DEG;
      p_y *= MAXEXTENTby180;
      if (p_x > MAXEXTENT) p_x = MAXEXTENT;
      if (p_x < -MAXEXTENT) p_x = -MAXEXTENT;
      if (p_y > MAXEXTENT) p_y = MAXEXTENT;
      if (p_y < -MAXEXTENT) p_y = -MAXEXTENT;
    }
    return MS_SUCCESS;
  }
  if(in->wellknownprojection == wkp_gmerc && out->wellknownprojection == wkp_lonlat) {
    for( i = line->numpoints-1; i >= 0; i-- ) {
      if (p_x > MAXEXTENT) p_x = MAXEXTENT;
      else if (p_x < -MAXEXTENT) p_x = -MAXEXTENT;
      if (p_y > MAXEXTENT) p_y = MAXEXTENT;
      else if (p_y < -MAXEXTENT) p_y = -MAXEXTENT;
      p_x = (p_x / MAXEXTENT) * 180;
      p_y = (p_y / MAXEXTENT) * 180;
      p_y = MS_RAD_TO_DEG * (2 * atan(exp(p_y * MS_DEG_TO_RAD)) - MS_PI2);
    }
    msComputeBounds( shape ); /* fixes bug 1586 */
    return MS_SUCCESS;
  }
#undef p_x
#undef p_y
#endif



  wrap_test = out != NULL && out->proj != NULL && pj_is_latlong(out->proj)
              && !pj_is_latlong(in->proj);

  line->numpoints = 0;

  memset( &lastPoint, 0, sizeof(lastPoint) );

  /* -------------------------------------------------------------------- */
  /*      Loop over all input points in linestring.                       */
  /* -------------------------------------------------------------------- */
  for( i=0; i < numpoints_in; i++ ) {
    int ms_err;
    wrkPoint = thisPoint = line->point[i];

    ms_err = msProjectPoint(in, out, &wrkPoint );

    /* -------------------------------------------------------------------- */
    /*      Apply wrap logic.                                               */
    /* -------------------------------------------------------------------- */
    if( wrap_test && i > 0 && ms_err != MS_FAILURE ) {
      double dist;
      pointObj pt1Geo;

      if( line_out->numpoints > 0 )
        pt1Geo = line_out->point[line_out->numpoints-1];
      else
        pt1Geo = wrkPoint; /* this is a cop out */

      dist = wrkPoint.x - pt1Geo.x;
      if( fabs(dist) > 180.0
          && msTestNeedWrap( thisPoint, lastPoint,
                             pt1Geo, in, out ) ) {
        if( dist > 0.0 )
          wrkPoint.x -= 360.0;
        else if( dist < 0.0 )
          wrkPoint.x += 360.0;
      }
    }

    /* -------------------------------------------------------------------- */
    /*      Put result into output line with appropriate logic for          */
    /*      failure breaking lines, etc.                                    */
    /* -------------------------------------------------------------------- */
    if( ms_err == MS_FAILURE ) {
      /* We have started out invalid */
      if( i == 0 ) {
        valid_flag = -1;
      }

      /* valid data has ended, we need to work out the horizon */
      else if( valid_flag == 1 ) {
        pointObj startPoint, endPoint;

        startPoint = lastPoint;
        endPoint = thisPoint;
        if( msProjectSegment( in, out, &startPoint, &endPoint )
            == MS_SUCCESS ) {
          line_out->point[line_out->numpoints++] = endPoint;
        }
        valid_flag = -1;
      }

      /* Still invalid ... */
      else if( valid_flag == -1 ) {
        /* do nothing */
      }
    }

    else {
      /* starting out valid. */
      if( i == 0 ) {
        line_out->point[line_out->numpoints++] = wrkPoint;
        valid_flag = 1;
      }

      /* Still valid, nothing special */
      else if( valid_flag == 1 ) {
        line_out->point[line_out->numpoints++] = wrkPoint;
      }

      /* we have come over the horizon, figure out where, start newline*/
      else {
        pointObj startPoint, endPoint;

        startPoint = lastPoint;
        endPoint = thisPoint;
        if( msProjectSegment( in, out, &endPoint, &startPoint )
            == MS_SUCCESS ) {
          lineObj newLine;

          /* force pre-allocation of lots of points room */
          if( line_out->numpoints > 0
              && shape->type == MS_SHAPE_LINE ) {
            newLine.numpoints = numpoints_in - i + 1;
            newLine.point = line->point;
            msAddLine( shape, &newLine );

            /* new line is now lineout, but start without any points */
            line_out = shape->line + shape->numlines-1;

            line_out->numpoints = 0;

            /* the shape->line array is realloc, refetch "line" */
            line = shape->line + line_index;
          } else if( line_out == line
                     && line->numpoints >= i-2 ) {
            newLine.numpoints = numpoints_in;
            newLine.point = line->point;
            msAddLine( shape, &newLine );

            line = shape->line + line_index;

            line_out = shape->line + shape->numlines-1;
            line_out->numpoints = line->numpoints;
            line->numpoints = 0;

            /*
             * Now realloc this array large enough to hold all
             * the points we could possibly need to add.
             */
            line_alloc = line_alloc * 2;

            line_out->point = (pointObj *)
                              realloc(line_out->point,
                                      sizeof(pointObj) * line_alloc);
          }

          line_out->point[line_out->numpoints++] = startPoint;
        }
        line_out->point[line_out->numpoints++] = wrkPoint;
        valid_flag = 1;
      }
    }

    lastPoint = thisPoint;
  }

  /* -------------------------------------------------------------------- */
  /*      Make sure that polygons are closed, even if the trip over       */
  /*      the horizon left them unclosed.                                 */
  /* -------------------------------------------------------------------- */
  if( shape->type == MS_SHAPE_POLYGON
      && line_out->numpoints > 2
      && (line_out->point[0].x != line_out->point[line_out->numpoints-1].x
          || line_out->point[0].y != line_out->point[line_out->numpoints-1].y) ) {
    /* make a copy because msAddPointToLine can realloc the array */
    pointObj sFirstPoint = line_out->point[0];
    msAddPointToLine( line_out, &sFirstPoint );
  }

  return(MS_SUCCESS);
}
#endif

/************************************************************************/
/*                           msProjectShape()                           */
/************************************************************************/
int msProjectShape(projectionObj *in, projectionObj *out, shapeObj *shape)
{
#ifdef USE_PROJ
  int i;
#ifdef USE_PROJ_FASTPATHS
  int j;

#define p_x shape->line[i].point[j].x
#define p_y shape->line[i].point[j].y
  if(in->wellknownprojection == wkp_lonlat && out->wellknownprojection == wkp_gmerc) {
    for( i = shape->numlines-1; i >= 0; i-- ) {
      for( j = shape->line[i].numpoints-1; j >= 0; j-- ) {
        p_x *= MAXEXTENTby180;
        p_y = log(tan((90 + p_y) * M_PIby360)) * MS_RAD_TO_DEG;
        p_y *= MAXEXTENTby180;
        if (p_x > MAXEXTENT) p_x = MAXEXTENT;
        else if (p_x < -MAXEXTENT) p_x = -MAXEXTENT;
        if (p_y > MAXEXTENT) p_y = MAXEXTENT;
        else if (p_y < -MAXEXTENT) p_y = -MAXEXTENT;
      }
    }
    msComputeBounds( shape ); /* fixes bug 1586 */
    return MS_SUCCESS;
  }
  if(in->wellknownprojection == wkp_gmerc && out->wellknownprojection == wkp_lonlat) {
    for( i = shape->numlines-1; i >= 0; i-- ) {
      for( j = shape->line[i].numpoints-1; j >= 0; j-- ) {
        if (p_x > MAXEXTENT) p_x = MAXEXTENT;
        else if (p_x < -MAXEXTENT) p_x = -MAXEXTENT;
        if (p_y > MAXEXTENT) p_y = MAXEXTENT;
        else if (p_y < -MAXEXTENT) p_y = -MAXEXTENT;
        p_x = (p_x / MAXEXTENT) * 180;
        p_y = (p_y / MAXEXTENT) * 180;
        p_y = MS_RAD_TO_DEG * (2 * atan(exp(p_y * MS_DEG_TO_RAD)) - MS_PI2);
      }
    }
    msComputeBounds( shape ); /* fixes bug 1586 */
    return MS_SUCCESS;
  }
#undef p_x
#undef p_y
#endif



  for( i = shape->numlines-1; i >= 0; i-- ) {
    if( shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON ) {
      if( msProjectShapeLine( in, out, shape, i ) == MS_FAILURE )
        msShapeDeleteLine( shape, i );
    } else if( msProjectLine(in, out, shape->line+i ) == MS_FAILURE ) {
      msShapeDeleteLine( shape, i );
    }
  }

  if( shape->numlines == 0 ) {
    msFreeShape( shape );
    return MS_FAILURE;
  } else {
    msComputeBounds( shape ); /* fixes bug 1586 */
    return(MS_SUCCESS);
  }
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectShape()");
  return(MS_FAILURE);
#endif
}

/************************************************************************/
/*                           msProjectLine()                            */
/*                                                                      */
/*      This function is now normally only used for point data.         */
/*      msProjectShapeLine() is used for lines and polygons and has     */
/*      lots of logic to handle horizon crossing.                       */
/************************************************************************/

int msProjectLine(projectionObj *in, projectionObj *out, lineObj *line)
{
#ifdef USE_PROJ
  int i, be_careful = 1;

  if( be_careful )
    be_careful = out->proj != NULL && pj_is_latlong(out->proj)
                 && !pj_is_latlong(in->proj);

  if( be_careful ) {
    pointObj  startPoint, thisPoint; /* locations in projected space */

    startPoint = line->point[0];

    for(i=0; i<line->numpoints; i++) {
      double  dist;

      thisPoint = line->point[i];

      /*
      ** Read comments before msTestNeedWrap() to better understand
      ** this dateline wrapping logic.
      */
      msProjectPoint(in, out, &(line->point[i]));
      if( i > 0 ) {
        dist = line->point[i].x - line->point[0].x;
        if( fabs(dist) > 180.0 ) {
          if( msTestNeedWrap( thisPoint, startPoint,
                              line->point[0], in, out ) ) {
            if( dist > 0.0 ) {
              line->point[i].x -= 360.0;
            } else if( dist < 0.0 ) {
              line->point[i].x += 360.0;
            }
          }
        }

      }
    }
  } else {
    for(i=0; i<line->numpoints; i++) {
      if( msProjectPoint(in, out, &(line->point[i])) == MS_FAILURE )
        return MS_FAILURE;
    }
  }

  return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectLine()");
  return(MS_FAILURE);
#endif
}

/************************************************************************/
/*                           msProjectRectGrid()                        */
/************************************************************************/

#define NUMBER_OF_SAMPLE_POINTS 100

int msProjectRectGrid(projectionObj *in, projectionObj *out, rectObj *rect)
{
#ifdef USE_PROJ
  pointObj prj_point;
  rectObj prj_rect;
  int   rect_initialized = MS_FALSE, failure=0;
  int     ix, iy;

  double dx, dy;
  double x, y;

  dx = (rect->maxx - rect->minx)/NUMBER_OF_SAMPLE_POINTS;
  dy = (rect->maxy - rect->miny)/NUMBER_OF_SAMPLE_POINTS;

  /* first ensure the top left corner is processed, even if the rect
     turns out to be degenerate. */

  prj_point.x = rect->minx;
  prj_point.y = rect->miny;
#ifdef USE_POINT_Z_M
  prj_point.z = 0.0;
  prj_point.m = 0.0;
#endif /* USE_POINT_Z_M */

  msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                    &failure);

  failure = 0;
  for(ix = 0; ix <= NUMBER_OF_SAMPLE_POINTS; ix++ ) {
    x = rect->minx + ix * dx;

    for(iy = 0; iy <= NUMBER_OF_SAMPLE_POINTS; iy++ ) {
      y = rect->miny + iy * dy;

      prj_point.x = x;
      prj_point.y = y;
      msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                        &failure);
    }
  }

  if( !rect_initialized ) {
    prj_rect.minx = 0;
    prj_rect.maxx = 0;
    prj_rect.miny = 0;
    prj_rect.maxy = 0;

    msSetError(MS_PROJERR, "All points failed to reproject.", "msProjectRect()");
  } else {
    msDebug( "msProjectRect(): some points failed to reproject, doing internal sampling.\n" );
  }

  rect->minx = prj_rect.minx;
  rect->miny = prj_rect.miny;
  rect->maxx = prj_rect.maxx;
  rect->maxy = prj_rect.maxy;

  if( !rect_initialized )
    return MS_FAILURE;
  else
    return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectRect()");
  return(MS_FAILURE);
#endif
}

/************************************************************************/
/*                    msProjectRectTraditionalEdge()                    */
/************************************************************************/
#ifdef notdef
static int
msProjectRectTraditionalEdge(projectionObj *in, projectionObj *out,
                             rectObj *rect)
{
#ifdef USE_PROJ
  pointObj prj_point;
  rectObj prj_rect;
  int   rect_initialized = MS_FALSE, failure=0;
  int     ix, iy;

  double dx, dy;
  double x, y;

  dx = (rect->maxx - rect->minx)/NUMBER_OF_SAMPLE_POINTS;
  dy = (rect->maxy - rect->miny)/NUMBER_OF_SAMPLE_POINTS;

  /* first ensure the top left corner is processed, even if the rect
     turns out to be degenerate. */

  prj_point.x = rect->minx;
  prj_point.y = rect->miny;
#ifdef USE_POINT_Z_M
  prj_point.z = 0.0;
  prj_point.m = 0.0;
#endif /* USE_POINT_Z_M */

  msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                    &failure);

  /* sample along top and bottom */
  if(dx > 0) {
    for(ix = 0; ix <= NUMBER_OF_SAMPLE_POINTS; ix++ ) {
      x = rect->minx + ix * dx;

      prj_point.x = x;
      prj_point.y = rect->miny;
      msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                        &failure);

      prj_point.x = x;
      prj_point.y = rect->maxy;
      msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                        &failure);
    }
  }

  /* sample along left and right */
  if(dy > 0) {
    for(iy = 0; iy <= NUMBER_OF_SAMPLE_POINTS; iy++ ) {
      y = rect->miny + iy * dy;

      prj_point.y = y;
      prj_point.x = rect->minx;
      msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                        &failure);

      prj_point.x = rect->maxx;
      prj_point.y = y;
      msProjectGrowRect(in,out,&prj_rect,&rect_initialized,&prj_point,
                        &failure);
    }
  }

  /*
  ** If there have been any failures around the edges, then we had better
  ** try and fill in the interior to get a close bounds.
  */
  if( failure > 0 )
    return msProjectRectGrid( in, out, rect );

  rect->minx = prj_rect.minx;
  rect->miny = prj_rect.miny;
  rect->maxx = prj_rect.maxx;
  rect->maxy = prj_rect.maxy;

  if( !rect_initialized )
    return MS_FAILURE;
  else
    return(MS_SUCCESS);
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectRect()");
  return(MS_FAILURE);
#endif
}
#endif /* def notdef */

/************************************************************************/
/*                       msProjectRectAsPolygon()                       */
/************************************************************************/

static int
msProjectRectAsPolygon(projectionObj *in, projectionObj *out,
                       rectObj *rect)
{
#ifdef USE_PROJ
  shapeObj polygonObj;
  lineObj  ring;
  /*  pointObj ringPoints[NUMBER_OF_SAMPLE_POINTS*4+4]; */
  pointObj *ringPoints;
  int     ix, iy;

  double dx, dy;

  /* If projecting from longlat to projected */
  /* This hack was introduced for WFS 2.0 compliance testing, but is far */
  /* from being perfect */
  if( out && !pj_is_latlong(out->proj) && in && pj_is_latlong(in->proj) &&
      fabs(rect->minx - -180.0) < 1e-5 && fabs(rect->miny - -90.0) < 1e-5 &&
      fabs(rect->maxx - 180.0) < 1e-5 && fabs(rect->maxy - 90.0) < 1e-5) {
    pointObj pointTest;
    pointTest.x = -180;
    pointTest.y = 85;
    msProjectPoint(in, out, &pointTest);
    /* Detect if we are reprojecting from EPSG:4326 to EPSG:3857 */
    /* and if so use more plausible bounds to avoid issues with computed */
    /* resolution for WCS */
    if (fabs(pointTest.x - -20037508.3427892) < 1e-5 && fabs(pointTest.y - 19971868.8804086) )
    {
        rect->minx = -20037508.3427892;
        rect->miny = -20037508.3427892;
        rect->maxx = 20037508.3427892;
        rect->maxy = 20037508.3427892;
    }
    else
    {
        rect->minx = -1e15;
        rect->miny = -1e15;
        rect->maxx = 1e15;
        rect->maxy = 1e15;
    }
    return MS_SUCCESS;
  }

  /* -------------------------------------------------------------------- */
  /*      Build polygon as steps around the source rectangle.             */
  /* -------------------------------------------------------------------- */
  dx = (rect->maxx - rect->minx)/NUMBER_OF_SAMPLE_POINTS;
  dy = (rect->maxy - rect->miny)/NUMBER_OF_SAMPLE_POINTS;

  if(dx==0 && dy==0) {
    pointObj foo;
    msDebug( "msProjectRect(): Warning: degenerate rect {%f,%f,%f,%f}\n",rect->minx,rect->miny,rect->minx,rect->miny );
    foo.x = rect->minx;
    foo.y = rect->miny;
    msProjectPoint(in,out,&foo);
    rect->minx=rect->maxx=foo.x;
    rect->miny=rect->maxy=foo.y;
    return MS_SUCCESS;
  }
  
  ringPoints = (pointObj*) calloc(sizeof(pointObj),NUMBER_OF_SAMPLE_POINTS*4+4);
  ring.point = ringPoints;
  ring.numpoints = 0;

  msInitShape( &polygonObj );
  polygonObj.type = MS_SHAPE_POLYGON;

  /* sample along top */
  if(dx != 0) {
    for(ix = 0; ix <= NUMBER_OF_SAMPLE_POINTS; ix++ ) {
      ringPoints[ring.numpoints].x = rect->minx + ix * dx;
      ringPoints[ring.numpoints++].y = rect->miny;
    }
  }

  /* sample on along right side */
  if(dy != 0) {
    for(iy = 1; iy <= NUMBER_OF_SAMPLE_POINTS; iy++ ) {
      ringPoints[ring.numpoints].x = rect->maxx;
      ringPoints[ring.numpoints++].y = rect->miny + iy * dy;
    }
  }

  /* sample along bottom */
  if(dx != 0) {
    for(ix = NUMBER_OF_SAMPLE_POINTS-1; ix >= 0; ix-- ) {
      ringPoints[ring.numpoints].x = rect->minx + ix * dx;
      ringPoints[ring.numpoints++].y = rect->maxy;
    }
  }

  /* sample on along left side */
  if(dy != 0) {
    for(iy = NUMBER_OF_SAMPLE_POINTS-1; iy >= 0; iy-- ) {
      ringPoints[ring.numpoints].x = rect->minx;
      ringPoints[ring.numpoints++].y = rect->miny + iy * dy;
    }
  }

  msAddLineDirectly( &polygonObj, &ring );

#ifdef notdef
  FILE *wkt = fopen("/tmp/www-before.wkt","w");
  char *tmp = msShapeToWKT(&polygonObj);
  fprintf(wkt,"%s\n", tmp);
  free(tmp);
  fclose(wkt);
#endif

  /* -------------------------------------------------------------------- */
  /*      Attempt to reproject.                                           */
  /* -------------------------------------------------------------------- */
  msProjectShapeLine( in, out, &polygonObj, 0 );

  /* If no points reprojected, try a grid sampling */
  if( polygonObj.numlines == 0 || polygonObj.line[0].numpoints == 0 ) {
    msFreeShape( &polygonObj );
    return msProjectRectGrid( in, out, rect );
  }

#ifdef notdef
  wkt = fopen("/tmp/www-after.wkt","w");
  tmp = msShapeToWKT(&polygonObj);
  fprintf(wkt,"%s\n", tmp);
  free(tmp);
  fclose(wkt);
#endif

  /* -------------------------------------------------------------------- */
  /*      Collect bounds.                                                 */
  /* -------------------------------------------------------------------- */
  rect->minx = rect->maxx = polygonObj.line[0].point[0].x;
  rect->miny = rect->maxy = polygonObj.line[0].point[0].y;

  for( ix = 1; ix < polygonObj.line[0].numpoints; ix++ ) {
    pointObj  *pnt = polygonObj.line[0].point + ix;

    rect->minx = MS_MIN(rect->minx,pnt->x);
    rect->maxx = MS_MAX(rect->maxx,pnt->x);
    rect->miny = MS_MIN(rect->miny,pnt->y);
    rect->maxy = MS_MAX(rect->maxy,pnt->y);
  }

  msFreeShape( &polygonObj );

  /* -------------------------------------------------------------------- */
  /*      Special case to handle reprojection from "more than the         */
  /*      whole world" projected coordinates that sometimes produce a     */
  /*      region greater than 360 degrees wide due to various wrapping    */
  /*      logic.                                                          */
  /* -------------------------------------------------------------------- */
  if( out && pj_is_latlong(out->proj) && in && !pj_is_latlong(in->proj)
      && rect->maxx - rect->minx > 360.0 && !out->gt.need_geotransform ) {
    rect->maxx = 180;
    rect->minx = -180;
  }

  return MS_SUCCESS;
#else
  msSetError(MS_PROJERR, "Projection support is not available.", "msProjectRect()");
  return(MS_FAILURE);
#endif
}

/************************************************************************/
/*                        msProjectHasLonWrap()                         */
/************************************************************************/

int msProjectHasLonWrap(projectionObj *in, double* pdfLonWrap)
{
    int i;
    if( pdfLonWrap )
        *pdfLonWrap = 0;
#if USE_PROJ
    if( !pj_is_latlong(in->proj) )
        return MS_FALSE;
#endif
    for( i = 0; i < in->numargs; i++ )
    {
        if( strncmp(in->args[i], "lon_wrap=",
                    strlen("lon_wrap=")) == 0 )
        {
            if( pdfLonWrap )
                *pdfLonWrap = atof(in->args[i] + strlen("lon_wrap="));
            return MS_TRUE;
        }
    }
    return MS_FALSE;
}

/************************************************************************/
/*                           msProjectRect()                            */
/************************************************************************/

int msProjectRect(projectionObj *in, projectionObj *out, rectObj *rect)
{
#ifdef notdef
  return msProjectRectTraditionalEdge( in, out, rect );
#else
  char *over = "+over";
  int ret;
  int bFreeInOver = MS_FALSE;
  int bFreeOutOver = MS_FALSE;
  projectionObj in_over,out_over,*inp,*outp;
  double dfLonWrap = 0.0;

#if USE_PROJ
  /* Detect projecting from north polar stereographic to longlat */
  if( in && !in->gt.need_geotransform &&
      out && !out->gt.need_geotransform &&
      !pj_is_latlong(in->proj) && pj_is_latlong(out->proj) )
  {
      pointObj p;
      p.x = 0.0;
      p.y = 0.0;
      if( msProjectPoint(in, out, &p) == MS_SUCCESS &&
          fabs(p.y - 90) < 1e-8 )
      {
        /* Is the pole in the rectangle ? */
        if( 0 >= rect->minx && 0 >= rect->miny &&
            0 <= rect->maxx && 0 <= rect->maxy )
        {
            if( msProjectRectAsPolygon(in, out, rect ) == MS_SUCCESS )
            {
                rect->minx = -180.0;
                rect->maxx = 180.0;
                rect->maxy = 90.0;
                return MS_SUCCESS;
            }
        }
        /* Are we sure the dateline is not enclosed ? */
        else if( rect->maxy < 0 || rect->maxx < 0 || rect->minx > 0 )
        {
            return msProjectRectAsPolygon(in, out, rect );
        }
      }
  }
#endif

  if(in && msProjectHasLonWrap(in, &dfLonWrap) && dfLonWrap == 180.0) {
    inp = in;
    outp = out;
    if( rect->maxx > 180.0 ) {
      rect->minx = -180.0;
      rect->maxx = 180.0;
    }
  }
  /* 
   * Issue #4892: When projecting a rectangle we do not want proj to wrap resulting
   * coordinates around the dateline, as in practice a requested bounding box of
   * -22.000.000, -YYY, 22.000.000, YYY should be projected as
   * -190,-YYY,190,YYY rather than 170,-YYY,-170,YYY as would happen when wrapping (and
   *  vice-versa when projecting from lonlat to metric).
   *  To enforce this, we clone the input projections and add the "+over" proj 
   *  parameter in order to disable dateline wrapping.
   */ 
  else {
    if(out) {
      bFreeOutOver = MS_TRUE;
      msInitProjection(&out_over);
      msCopyProjectionExtended(&out_over,out,&over,1);
      outp = &out_over;
    } else {
      outp = out;
    }
    if(in) {
      bFreeInOver = MS_TRUE;
      msInitProjection(&in_over);
      msCopyProjectionExtended(&in_over,in,&over,1);
      inp = &in_over;
    } else {
      inp = in;
    }
  }
  ret = msProjectRectAsPolygon(inp,outp, rect );
  if(bFreeInOver)
    msFreeProjection(&in_over);
  if(bFreeOutOver)
    msFreeProjection(&out_over);
  return ret;
#endif
}

#ifdef USE_PROJ
static int msProjectSortString(const void* firstelt, const void* secondelt)
{
    char* firststr = *(char**)firstelt;
    char* secondstr = *(char**)secondelt;
    return strcmp(firststr, secondstr);
}

/************************************************************************/
/*                        msGetProjectNormalized()                      */
/************************************************************************/

static projectionObj* msGetProjectNormalized( const projectionObj* p )
{
  int i;
  char* pszNewProj4Def;
  projectionObj* pnew;

  pnew = (projectionObj*)msSmallMalloc(sizeof(projectionObj));
  msInitProjection(pnew);
  msCopyProjection(pnew, (projectionObj*)p);

  if(p->proj == NULL )
      return pnew;

  /* Normalize definition so that msProjectDiffers() works better */
  pszNewProj4Def = pj_get_def( p->proj, 0 );
  msFreeCharArray(pnew->args, pnew->numargs);
  pnew->args = msStringSplit(pszNewProj4Def,'+', &pnew->numargs);
  for(i = 0; i < pnew->numargs; i++)
  {
      /* Remove trailing space */
      if( strlen(pnew->args[i]) > 0 && pnew->args[i][strlen(pnew->args[i])-1] == ' ' )
          pnew->args[i][strlen(pnew->args[i])-1] = '\0';
      /* Remove spurious no_defs or init= */
      if( strcmp(pnew->args[i], "no_defs") == 0 ||
          strncmp(pnew->args[i], "init=", 5) == 0 )
      {
          if( i < pnew->numargs - 1 )
          {
              msFree(pnew->args[i]);
              memmove(pnew->args + i, pnew->args + i + 1,
                      sizeof(char*) * (pnew->numargs - 1 -i ));
          }
          else 
          {
              msFree(pnew->args[i]);
          }
          pnew->numargs --;
          i --;
          continue;
      }
  }
  /* Sort the strings so they can be compared */
  qsort(pnew->args, pnew->numargs, sizeof(char*), msProjectSortString);
  /*{
      fprintf(stderr, "'%s' =\n", pszNewProj4Def);
      for(i = 0; i < p->numargs; i++)
          fprintf(stderr, "'%s' ", p->args[i]);
      fprintf(stderr, "\n");
  }*/
  pj_dalloc(pszNewProj4Def);
  
  return pnew;
}
#endif /* USE_PROJ */

/************************************************************************/
/*                        msProjectionsDiffer()                         */
/************************************************************************/

/*
** Compare two projections, and return MS_TRUE if they differ.
**
** For now this is implemented by exact comparison of the projection
** arguments, but eventually this should call a PROJ.4 function with
** more awareness of the issues.
**
** NOTE: MS_FALSE is returned if either of the projection objects
** has no arguments, since reprojection won't work anyways.
*/

static int msProjectionsDifferInternal( projectionObj *proj1, projectionObj *proj2 )

{
  int   i;

  if( proj1->numargs == 0 || proj2->numargs == 0 )
    return MS_FALSE;

  if( proj1->numargs != proj2->numargs )
    return MS_TRUE;

  /* This test should be more rigerous. */
  if( proj1->gt.need_geotransform
      || proj2->gt.need_geotransform )
    return MS_TRUE;

  for( i = 0; i < proj1->numargs; i++ ) {
    if( strcmp(proj1->args[i],proj2->args[i]) != 0 )
      return MS_TRUE;
  }

  return MS_FALSE;
}

int msProjectionsDiffer( projectionObj *proj1, projectionObj *proj2 )
{
#ifdef USE_PROJ
    int ret;

    ret = msProjectionsDifferInternal(proj1, proj2); 
    if( ret &&
        /* to speed up things, do normalization only if one proj is */
        /* likely of the form init=epsg:XXX and the other proj=XXX datum=YYY... */
        ( (proj1->numargs == 1 && proj2->numargs > 1) ||
          (proj1->numargs > 1 && proj2->numargs == 1) ) )
    {
        projectionObj* p1normalized;
        projectionObj* p2normalized;

        p1normalized = msGetProjectNormalized( proj1 );
        p2normalized = msGetProjectNormalized( proj2 );
        ret = msProjectionsDifferInternal(p1normalized, p2normalized);
        msFreeProjection(p1normalized);
        msFree(p1normalized);
        msFreeProjection(p2normalized);
        msFree(p2normalized);
    }
    return ret;
#else
    return msProjectionsDifferInternal(proj1, proj2);
#endif
}

/************************************************************************/
/*                           msTestNeedWrap()                           */
/************************************************************************/
/*

Frank Warmerdam, Nov, 2001.

See Also:

http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=15

Proposal:

Modify msProjectLine() so that it "dateline wraps" objects when necessary
in order to preserve their shape when reprojecting to lat/long.  This
will be accomplished by:

1) As each vertex is reprojected, compare the X distance between that
   vertex and the previous vertex.  If it is less than 180 then proceed to
   the next vertex without any special logic, otherwise:

2) Reproject the center point of the line segment from the last vertex to
   the current vertex into lat/long.  Does it's longitude lie between the
   longitudes of the start and end point.  If yes, return to step 1) for
   the next vertex ... everything is fine.

3) We have determined that this line segment is suffering from 360 degree
   wrap to keep in the -180 to +180 range.  Now add or subtract 360 degrees
   as determined from the original sign of the distances.

This is similar to the code there now (though disabled in CVS); however,
it will ensure that big boxes will remain big, and not get dateline wrapped
because of the extra test in step 2).  However step 2 is invoked only very
rarely so this process takes little more than the normal process.  In fact,
if we were sufficiently concerned about performance we could do a test on the
shape MBR in lat/long space, and if the width is less than 180 we know we never
need to even do test 1).

What doesn't this resolve:

This ensures that individual lines are kept in the proper shape when
reprojected to geographic space.  However, it does not:

 o Ensure that all rings of a polygon will get transformed to the same "side"
   of the world.  Depending on starting points of the different rings it is
   entirely possible for one ring to end up in the -180 area and another ring
   from the same polygon to end up in the +180 area.  We might possibly be
   able to achieve this though, by treating the multi-ring polygon as a whole
   and testing the first point of each additional ring against the last
   vertex of the previous ring (or any previous vertex for that matter).

 o It does not address the need to cut up lines and polygons into distinct
   chunks to preserve the correct semantics.  Really a polygon that
   spaces the dateline in a -180 to 180 view should get split into two
   polygons.  We haven't addressed that, though if it were to be addressed,
   it could be done as a followon and distinct step from what we are doing
   above.  In fact this sort of improvement (split polygons based on dateline
   or view window) should be done for all lat/long shapes regardless of
   whether they are being reprojected from another projection.

 o It does not address issues related to viewing rectangles that go outside
   the -180 to 180 longitude range.  For instance, it is entirely reasonable
   to want a 160 to 200 longitude view to see an area on the dateline clearly.
   Currently shapes in the -180 to -160 range which should be displayed in the
   180 to 200 portion of that view will not be because there is no recogition
   that they belong there.


*/

#ifdef USE_PROJ
static int msTestNeedWrap( pointObj pt1, pointObj pt2, pointObj pt2_geo,
                           projectionObj *in,
                           projectionObj *out )

{
  pointObj  middle;

  middle.x = (pt1.x + pt2.x) * 0.5;
  middle.y = (pt1.y + pt2.y) * 0.5;

  if( msProjectPoint( in, out, &pt1 ) == MS_FAILURE
      || msProjectPoint( in, out, &pt2 ) == MS_FAILURE
      || msProjectPoint( in, out, &middle ) == MS_FAILURE )
    return 0;

  /*
   * If the last point was moved, then we are considered due for a
   * move to.
   */
  if( fabs(pt2_geo.x-pt2.x) > 180.0 )
    return 1;

  /*
   * Otherwise, test to see if the middle point transforms
   * to be between the end points. If yes, no wrapping is needed.
   * Otherwise wrapping is needed.
   */
  if( (middle.x < pt1.x && middle.x < pt2_geo.x)
      || (middle.x > pt1.x && middle.x > pt2_geo.x) )
    return 1;
  else
    return 0;
}
#endif /* def USE_PROJ */

/************************************************************************/
/*                            msProjFinder()                            */
/************************************************************************/
#ifdef USE_PROJ
static char *ms_proj_lib = NULL;
static char *last_filename = NULL;

static const char *msProjFinder( const char *filename)

{
  if( last_filename != NULL )
    free( last_filename );

  if( filename == NULL )
    return NULL;

  if( ms_proj_lib == NULL )
    return filename;

  last_filename = (char *) malloc(strlen(filename)+strlen(ms_proj_lib)+2);
  sprintf( last_filename, "%s/%s", ms_proj_lib, filename );

  return last_filename;
}
#endif /* def USE_PROJ */

/************************************************************************/
/*                       msProjLibInitFromEnv()                         */
/************************************************************************/
void msProjLibInitFromEnv()
{
  const char *val;

  if( (val=getenv( "PROJ_LIB" )) != NULL ) {
    msSetPROJ_LIB(val, NULL);
  }
}

/************************************************************************/
/*                           msSetPROJ_LIB()                            */
/************************************************************************/
void msSetPROJ_LIB( const char *proj_lib, const char *pszRelToPath )

{
#ifdef USE_PROJ
  static int finder_installed = 0;
  char *extended_path = NULL;

  /* Handle relative path if applicable */
  if( proj_lib && pszRelToPath
      && proj_lib[0] != '/'
      && proj_lib[0] != '\\'
      && !(proj_lib[0] != '\0' && proj_lib[1] == ':') ) {
    struct stat stat_buf;
    extended_path = (char*) msSmallMalloc(strlen(pszRelToPath)
                                          + strlen(proj_lib) + 10);
    sprintf( extended_path, "%s/%s", pszRelToPath, proj_lib );

#ifndef S_ISDIR
#  define S_ISDIR(x) ((x) & S_IFDIR)
#endif

    if( stat( extended_path, &stat_buf ) == 0
        && S_ISDIR(stat_buf.st_mode) )
      proj_lib = extended_path;
  }


  msAcquireLock( TLOCK_PROJ );

  if( finder_installed == 0 && proj_lib != NULL) {
    finder_installed = 1;
    pj_set_finder( msProjFinder );
  }

  if (proj_lib == NULL) pj_set_finder(NULL);

  if( ms_proj_lib != NULL ) {
    free( ms_proj_lib );
    ms_proj_lib = NULL;
  }

  if( last_filename != NULL ) {
    free( last_filename );
    last_filename = NULL;
  }

  if( proj_lib != NULL )
    ms_proj_lib = msStrdup( proj_lib );

  msReleaseLock( TLOCK_PROJ );

  if ( extended_path )
    msFree( extended_path );
#endif
}

/************************************************************************/
/*                       msGetProjectionString()                        */
/*                                                                      */
/*      Return the projection string.                                   */
/************************************************************************/

char *msGetProjectionString(projectionObj *proj)
{
  char        *pszProjString = NULL;
  int         i = 0, nLen = 0;

  if (proj) {
    /* -------------------------------------------------------------------- */
    /*      Alloc buffer large enough to hold the whole projection defn     */
    /* -------------------------------------------------------------------- */
    for (i=0; i<proj->numargs; i++) {
      if (proj->args[i])
        nLen += (strlen(proj->args[i]) + 2);
    }

    pszProjString = (char*)malloc(sizeof(char) * nLen+1);
    pszProjString[0] = '\0';

    /* -------------------------------------------------------------------- */
    /*      Plug each arg into the string with a '+' prefix                 */
    /* -------------------------------------------------------------------- */
    for (i=0; i<proj->numargs; i++) {
      if (!proj->args[i] || strlen(proj->args[i]) == 0)
        continue;
      if (pszProjString[0] == '\0') {
        /* no space at beginning of line */
        if (proj->args[i][0] != '+')
          strcat(pszProjString, "+");
      } else {
        if (proj->args[i][0] != '+')
          strcat(pszProjString, " +");
        else
          strcat(pszProjString, " ");
      }
      strcat(pszProjString, proj->args[i]);
    }
  }

  return pszProjString;
}

/************************************************************************/
/*                       msIsAxisInvertedProj()                         */
/*                                                                      */
/*      Return MS_TRUE is the proj object has epgsaxis=ne               */
/************************************************************************/

int msIsAxisInvertedProj( projectionObj *proj )
{
  int i;
  const char *axis = NULL;

  for( i = 0; i < proj->numargs; i++ ) {
    if( strstr(proj->args[i],"epsgaxis=") != NULL ) {
      axis = strstr(proj->args[i],"=") + 1;
      break;
    }
  }

  if( axis == NULL )
    return MS_FALSE;

  if( strcasecmp(axis,"en") == 0 )
    return MS_FALSE;

  if( strcasecmp(axis,"ne") != 0 ) {
    msDebug( "msIsAxisInvertedProj(): odd +epsgaxis= value: '%s'.",
             axis );
    return MS_FALSE;
  }
  
  return MS_TRUE;
}

/************************************************************************/
/*                       msAxisNormalizePoints()                        */
/*                                                                      */
/*      Convert the passed points to "easting, northing" axis           */
/*      orientation if they are not already.                            */
/************************************************************************/

void msAxisNormalizePoints( projectionObj *proj, int count,
                            double *x, double *y )

{
  int i;
  if( !msIsAxisInvertedProj(proj ) )
      return;

  /* Switch axes */
  for( i = 0; i < count; i++ ) {
    double tmp;

    tmp = x[i];
    x[i] = y[i];
    y[i] = tmp;
  }
}



/************************************************************************/
/*                             msAxisSwapShape                          */
/*                                                                      */
/*      Utility function to swap x and y coordiatesn Use for now for    */
/*      WFS 1.1.x                                                       */
/************************************************************************/
void msAxisSwapShape(shapeObj *shape)
{
  double tmp;
  int i,j;

  if (shape) {
    for(i=0; i<shape->numlines; i++) {
      for( j=0; j<shape->line[i].numpoints; j++ ) {
        tmp = shape->line[i].point[j].x;
        shape->line[i].point[j].x = shape->line[i].point[j].y;
        shape->line[i].point[j].y = tmp;
      }
    }

    /*swap bounds*/
    tmp = shape->bounds.minx;
    shape->bounds.minx = shape->bounds.miny;
    shape->bounds.miny = tmp;

    tmp = shape->bounds.maxx;
    shape->bounds.maxx = shape->bounds.maxy;
    shape->bounds.maxy = tmp;
  }
}

/************************************************************************/
/*                      msAxisDenormalizePoints()                       */
/*                                                                      */
/*      Convert points from easting,northing orientation to the         */
/*      preferred epsg orientation of this projectionObj.               */
/************************************************************************/

void msAxisDenormalizePoints( projectionObj *proj, int count,
                              double *x, double *y )

{
  /* For how this is essentially identical to normalizing */
  msAxisNormalizePoints( proj, count, x, y );
}

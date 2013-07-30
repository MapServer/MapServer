/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Rendering utility functions
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
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
#include "renderers/agg/include/agg_arc.h"
#include "renderers/agg/include/agg_basics.h"



shapeObj *msRasterizeArc(double x0, double y0, double radius, double startAngle, double endAngle, int isSlice)
{
  static int allocated_size=100;
  shapeObj *shape = (shapeObj*)calloc(1,sizeof(shapeObj));
  MS_CHECK_ALLOC(shape, sizeof(shapeObj), NULL);
  mapserver::arc arc ( x0, y0,radius,radius, startAngle*MS_DEG_TO_RAD, endAngle*MS_DEG_TO_RAD,true );
  arc.approximation_scale ( 1 );
  arc.rewind(1);
  msInitShape(shape);

  lineObj *line = (lineObj*)calloc(1,sizeof(lineObj));
  if (!line) {
    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msRasterizeArc()" ,
               __FILE__, __LINE__, (unsigned int)sizeof(lineObj));
    free(shape);
    return NULL;
  }
  shape->line = line;
  shape->numlines = 1;
  line->point = (pointObj*)calloc(allocated_size,sizeof(pointObj));
  if (!line->point) {
    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msRasterizeArc()" ,
               __FILE__, __LINE__, (unsigned int)(allocated_size*sizeof(pointObj)));
    free(line);
    free(shape);
    return NULL;
  }

  line->numpoints = 0;

  double x,y;

  //first segment from center to first point of arc
  if(isSlice) {
    line->point[0].x = x0;
    line->point[0].y = y0;
    line->numpoints = 1;
  }
  while(arc.vertex(&x,&y) != mapserver::path_cmd_stop) {
    if(line->numpoints == allocated_size) {
      allocated_size *= 2;
      line->point = (pointObj*)realloc(line->point, allocated_size * sizeof(pointObj));
      if (!line->point) {
        msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msRasterizeArc()" ,
                   __FILE__, __LINE__, (unsigned int)(allocated_size * sizeof(pointObj)));
        free(line);
        free(shape);
        return NULL;
      }
    }
    line->point[line->numpoints].x = x;
    line->point[line->numpoints].y = y;
    line->numpoints++;
  }

  //make sure the shape is closed
  if(line->point[line->numpoints-1].x != line->point[0].x ||
      line->point[line->numpoints-1].y != line->point[0].y) {
    if(line->numpoints == allocated_size) {
      allocated_size *= 2;
      line->point = (pointObj*)msSmallRealloc(line->point, allocated_size * sizeof(pointObj));
    }
    line->point[line->numpoints].x = line->point[0].x;
    line->point[line->numpoints].y = line->point[0].y;
    line->numpoints++;
  }

  return shape;
}




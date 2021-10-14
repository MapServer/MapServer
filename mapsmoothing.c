/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  RFC94 Shape smoothing
 * Author:   Alan Boudreault (aboudreault@mapgears.com)
 * Author:   Daniel Morissette (dmorissette@mapgears.com)
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

#define FP_EPSILON 1e-12
#define FP_EQ(a, b) (fabs((a)-(b)) < FP_EPSILON)

/* Internal Use, represent a line window (points) */
typedef struct {
  int pos; /* current point position in line */
  int size;
  int index; /* index of the current point in points array */
  lineObj *line;
  int lineIsRing; /* closed ring? */  
  pointObj **points;
} lineWindow;

static void initLineWindow(lineWindow *lw, lineObj *line, int size)
{ 
  lw->pos = -1;
  lw->lineIsRing = MS_FALSE;
  lw->size = size;
  lw->line = line;
  lw->index = floor(lw->size/2); /* index of current position in points array */
  lw->points = (pointObj**)msSmallMalloc(sizeof(pointObj*)*size);
  
  if ( (line->numpoints >= 2) &&
       ((FP_EQ(line->point[0].x,
               line->point[line->numpoints-1].x)) &&
        (FP_EQ(line->point[0].y,
               line->point[line->numpoints-1].y))) ) {
    lw->lineIsRing = 1; 
  }
}

static void freeLineWindow(lineWindow *lw)
{  
  free(lw->points);
}

static int nextLineWindow(lineWindow *lw)
{
  int i;

  if (++lw->pos >= lw->line->numpoints)
    return MS_DONE;

  lw->points[lw->index] = &lw->line->point[lw->pos];

  for (i=0;i<lw->index;++i) {
    int r, l;
    r = lw->pos-(i+1);
    l = lw->pos+(i+1);

    /* adjust values */
    if ((r < 0) && lw->lineIsRing)
      r = lw->line->numpoints-(i+2);
    if ((l >= lw->line->numpoints) && lw->lineIsRing)
      l = 1+(l-lw->line->numpoints);
    
    /* return if the window in not valid.. */
    if (r<0 || l>=lw->line->numpoints)
      return MS_FALSE;
    
    lw->points[lw->index-(i+1)] = &lw->line->point[r];
    lw->points[lw->index+(i+1)] = &lw->line->point[l];
  }

  return MS_TRUE;
}

/* Calculates the distance ratio between the total distance of a path and the distance
   between the first and last point (to detect a loop). */
static double computePathDistanceRatio(pointObj **points, int len)
{
  double sum;
  int i;

  for (sum=0,i=1;i<len;++i) {
    sum += msDistancePointToPoint(points[i-1], points[i]);
  }

  return sum/msDistancePointToPoint(points[0], points[len-1]);
}

/* Pre-Processing of a shape. It modifies the shape by adding intermediate
   points where a loop is detected to improve the smoothing result. */
static int processShapePathDistance(shapeObj *shape, int force)
{
  shapeObj initialShape, *newShape;
  int i;
  
  /* initial shape to process */
  msInitShape(&initialShape);    
  msCopyShape(shape, &initialShape);

  newShape = shape; /* we modify the shape object directly */
  shape = &initialShape;
  
  /* Clean our shape object */
  for (i= 0; i < newShape->numlines; i++)
    free(newShape->line[i].point);
  newShape->numlines = 0;
  if (newShape->line) free(newShape->line);
  
  for (i=0;i<shape->numlines;++i) {
    const int windowSize = 5;
    int res;
    lineWindow lw;
    lineObj line = {0, NULL};

    initLineWindow(&lw, &shape->line[i], windowSize);
    msAddLine(newShape, &line);

    while ((res = nextLineWindow(&lw)) != MS_DONE) {
      double ratio = 0;
      pointObj point = {0,0,0,0}; // initialize

      if (lw.lineIsRing && lw.pos==lw.line->numpoints-1) {
        point = newShape->line[i].point[0];
        msAddPointToLine(&newShape->line[i],
                         &point);
        continue;
      }

      if (res == MS_FALSE) { /* invalid window */
        msAddPointToLine(&newShape->line[i],
                         lw.points[lw.index]);
        continue;
      }

      if (!force)
        ratio = computePathDistanceRatio(lw.points, windowSize);
                      
      if (force || (ratio > 1.3)) {
        point.x = (lw.line->point[lw.pos].x + lw.points[lw.index-1]->x)/2;
        point.y = (lw.line->point[lw.pos].y + lw.points[lw.index-1]->y)/2;
        msAddPointToLine(&newShape->line[i],
                         &point);
      }
      
      point = lw.line->point[lw.pos];
      msAddPointToLine(&newShape->line[i],
                       &point);
      
      if (force || (ratio > 1.3)) {
        point.x = (lw.line->point[lw.pos].x + lw.points[lw.index+1]->x)/2;
        point.y = (lw.line->point[lw.pos].y + lw.points[lw.index+1]->y)/2;
        msAddPointToLine(&newShape->line[i],
                         &point);
      }
      
    }
       
    freeLineWindow(&lw);
  }
    
  msFreeShape(shape);
  
  return MS_SUCCESS;
}

shapeObj* msSmoothShapeSIA(shapeObj *shape, int ss, int si, char *preprocessing)
{
  int i, j;
  pointObj *p;
  double *coeff;
  shapeObj *newShape;

  newShape = (shapeObj *) msSmallMalloc(sizeof (shapeObj));
  msInitShape(newShape);
  newShape->type = shape->type; // preserve the type

  if (ss < 3)
    ss = 3;
  
  if (si < 1)
    si = 1;

  /* Apply preprocessing */
  if (preprocessing)
  {
    if (strcasecmp(preprocessing, "all") == 0)
      processShapePathDistance(shape, MS_TRUE);
    else if (strcasecmp(preprocessing, "angle") == 0)
      processShapePathDistance(shape, MS_FALSE);
  }
  
  p = (pointObj *) msSmallMalloc(ss*sizeof(pointObj));
  coeff = (double *) msSmallMalloc(ss*sizeof (double));        
  
  for (i=0;i<si;++i) {
    shapeObj initialShape;
    
    if (si > 1 && i>0) {
      msInitShape(&initialShape);    
      msCopyShape(shape, &initialShape);
      
      /* Clean our shape object */
      for (j=0; j < newShape->numlines; ++j)
        free(newShape->line[j].point);
      newShape->numlines = 0;
      if (newShape->line) {
        free(newShape->line);
        newShape->line = NULL;
      }

      shape = &initialShape;
    }
    
    for (j=0;j<shape->numlines;++j) {
      int k, ws, res;      
      lineObj newLine = {0,NULL};
      lineWindow lw;

      /* determine if we can use the ss for this line */
      ws = ss;      
      if (ws >= shape->line[j].numpoints) {
        ws = shape->line[j].numpoints-1;
      }

      if (ws%2==0)
        ws-=1;  

      initLineWindow(&lw, &shape->line[j], ws);
      msAddLine(newShape, &newLine);

      coeff[lw.index] = 1;
      for (k=0;k<lw.index;++k) {
        coeff[lw.index+(k+1)] = coeff[lw.index-k]/2;
        coeff[lw.index-(k+1)] = coeff[lw.index+k]/2;    
      }
      
      
      while ((res = nextLineWindow(&lw)) != MS_DONE) {
        double sum_x=0, sum_y=0, sum = 0;
        pointObj point = {0,0,0,0}; // initialize
        int k = 0;

        if (res == MS_FALSE) { /* invalid window */
          msAddPointToLine(&newShape->line[j],
                           lw.points[lw.index]);
          continue;
        }

        /* Apply Coefficient */
        p[lw.index] = *lw.points[lw.index];
        for (k=0; k<lw.index; ++k) {
          p[lw.index-(k+1)] = *lw.points[lw.index-(k+1)];
          p[lw.index-(k+1)].x *= coeff[lw.index-(k+1)];
          p[lw.index-(k+1)].y *= coeff[lw.index-(k+1)];
          p[lw.index+(k+1)] = *lw.points[lw.index+(k+1)];
          p[lw.index+(k+1)].x *= coeff[lw.index+(k+1)];
          p[lw.index+(k+1)].y *= coeff[lw.index+(k+1)];
        }
        
        for (k=0; k<lw.size; ++k) {
          sum += coeff[k];
          sum_x += p[k].x;
          sum_y += p[k].y;          
        }
      
        point.x = sum_x/sum;
        point.y = sum_y/sum;
        msAddPointToLine(&newShape->line[j],
                         &point);
      }

      freeLineWindow(&lw);      
    }
    
    if (i>0) {
      msFreeShape(shape);
      shape = newShape;
    }
    
  }
  
  free(p);
  free(coeff);

  return newShape;
}

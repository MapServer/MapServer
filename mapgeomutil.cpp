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
#include "mapagg.h"
#include "renderers/agg/include/agg_arc.h"
#include "renderers/agg/include/agg_basics.h"
#include "renderers/agg/include/agg_path_storage.h"
#include "renderers/agg/include/agg_conv_clipper.h"
#include "renderers/agg/include/agg_conv_stroke.h"



shapeObj *msRasterizeArc(double x0, double y0, double radius, double startAngle, double endAngle, int isSlice) {
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
                   __FILE__, __LINE__, sizeof(lineObj));       
        free(shape);
        return NULL; 
    }
    shape->line = line;
    shape->numlines = 1;
    line->point = (pointObj*)calloc(allocated_size,sizeof(pointObj));
    if (!line->point) {
        msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msRasterizeArc()" ,
                       __FILE__, __LINE__, allocated_size*sizeof(pointObj));       
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
                           __FILE__, __LINE__, allocated_size * sizeof(pointObj));       
                free(line);
                free(shape);
                return NULL; 
            }
        }
        line->point[line->numpoints].x = x;
        line->point[line->numpoints].y = y;
        line->numpoints++;
    }
    
    //make sure the shape is closed if we're doing a full circle
    if(!isSlice && !(endAngle-startAngle)%360) {
        if(line->point[line->numpoints-1].x != line->point[0].x ||
           line->point[line->numpoints-1].y != line->point[0].y) {
            if(line->numpoints == allocated_size) {
                allocated_size *= 2;
                line->point = (pointObj*)realloc(line->point, allocated_size * sizeof(pointObj));
                if (!line->point) {
                    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msRasterizeArc()" ,  
                               __FILE__, __LINE__, allocated_size * sizeof(pointObj));       
                    free(line);
                    free(shape);
                    return NULL; 
                }
            }
            line->point[line->numpoints].x = line->point[0].x;
            line->point[line->numpoints].y = line->point[0].y;
            line->numpoints++;
        }
        
    }
    if(isSlice) {
        if(line->numpoints == allocated_size) {
            allocated_size *= 2;
            line->point = (pointObj*)realloc(line->point, allocated_size * sizeof(pointObj));
            if (!line->point) {
                msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "msRasterizeArc()" ,
                           __FILE__, __LINE__, allocated_size * sizeof(pointObj));       
                free(line);
                free(shape);
                return NULL; 
            }
        }
        line->point[line->numpoints].x = x0;
        line->point[line->numpoints].y = y0;
        line->numpoints++;
    }
    return shape;
}

// ------------------------------------------------------------------------ 
// Function to create a custom hatch symbol based on an arbitrary angle. 
// ------------------------------------------------------------------------
static mapserver::path_storage createHatch(int sx, int sy, double angle, double step)
{
    mapserver::path_storage path;
    //path.start_new_path();
    //restrict the angle to [0 180[
    angle = fmod(angle, 360.0);
    if(angle < 0) angle += 360;
    if(angle >= 180) angle -= 180;

    //treat 2 easy cases which would cause divide by 0 in generic case
    if(angle==0) {
        for(double y=step/2.0;y<sy;y+=step) {
            path.move_to(0,y);
            path.line_to(sx,y);
        }
        return path;
    }
    if(angle==90) {
        for(double x=step/2.0;x<sx;x+=step) {
            path.move_to(x,0);
            path.line_to(x,sy);
        }
        return path;
    }


    double theta = (90-angle)*MS_DEG_TO_RAD;
    double ct = cos(theta);
    double st = sin(theta);
    double rmax = sqrt((double)sx*sx+sy*sy);

    //parametrize each line as r = x.cos(theta) + y.sin(theta)
    for(double r=(angle<90)?step/2.:-rmax;r<rmax;r+=step) {
        int inter=0;
        double x,y;
        double pt[8]; //array to store the coordinates of intersection of the line with the sides
        //in the general case there will only be two intersections
        //so pt[4] should be sufficient to store the coordinates of the intersection,
        //but we allocate pt[8] to treat the special and rare/unfortunate case when the
        //line is a perfect diagonal (and therfore intersects all four sides)
        //note that the order for testing is important in this case so that the first
        //two intersection points actually correspond to the diagonal and not a degenerate line
        
        //test for intersection with each side

        y=r/st;x=0; // test for intersection with top of image
        if(y>=0&&y<=sy) {
            pt[2*inter]=x;pt[2*inter+1]=y;
            inter++;
        }     
        x=sx;y=(r-sx*ct)/st;// test for intersection with bottom of image
        if(y>=0&&y<=sy) {
            pt[2*inter]=x;pt[2*inter+1]=y;
            inter++;
        }
        y=0;x=r/ct;// test for intersection with left of image
        if(x>=0&&x<=sx) {
            pt[2*inter]=x;pt[2*inter+1]=y;
            inter++;
        }
        y=sy;x=(r-sy*st)/ct;// test for intersection with right of image
        if(x>=0&&x<=sx) {
            pt[2*inter]=x;pt[2*inter+1]=y;
            inter++;
        }
        if(inter==2 && (pt[0]!=pt[2] || pt[1]!=pt[3])) { 
            //the line intersects with two sides of the image, it should therefore be drawn
            path.move_to(pt[0],pt[1]);
            path.line_to(pt[2],pt[3]);
        }
    }
    return path;
}

int msHatchPolygon(imageObj *img, shapeObj *poly, double spacing, double width, double angle, colorObj *color) {
   assert(MS_RENDERER_PLUGIN(img->format));
   mapserver::path_storage lines = createHatch(img->width, img->height, angle, spacing);
   polygon_adaptor polygons(poly);
   shapeObj shape;
   msInitShape(&shape);
   int allocated = 20;
   lineObj line;
   shape.line = &line;
   shape.numlines = 1;
   shape.line[0].point = (pointObj*)msSmallCalloc(allocated,sizeof(pointObj));
   shape.line[0].numpoints = 0;
   mapserver::conv_stroke<mapserver::path_storage> stroke(lines);
   stroke.width(width);
   stroke.line_cap(mapserver::butt_cap);
   //mapserver::conv_clipper<mapserver::path_storage,polygon_adaptor> clipper(*lines,polygons, mapserver::clipper_and);
   //mapserver::conv_clipper<polygon_adaptor,mapserver::path_storage> clipper(polygons,lines, mapserver::clipper_and);
   mapserver::conv_clipper<polygon_adaptor,mapserver::conv_stroke<mapserver::path_storage> > clipper(polygons,stroke, mapserver::clipper_and); 
   clipper.rewind(0);
   
   double x=0,y=0;
   unsigned int cmd, prevCmd=-1;
   while((cmd = clipper.vertex(&x,&y)) != mapserver::path_cmd_stop) {
      switch(cmd) {
      case mapserver::path_cmd_line_to:
         if(shape.line[0].numpoints == allocated) {
            allocated *= 2;
            shape.line[0].point = (pointObj*)msSmallRealloc(shape.line[0].point, allocated*sizeof(pointObj));
         }
         shape.line[0].point[shape.line[0].numpoints].x = x;
         shape.line[0].point[shape.line[0].numpoints].y = y;
         shape.line[0].numpoints++;
         break;
      case mapserver::path_cmd_move_to:
         assert(shape.line[0].numpoints <= 1 || prevCmd == mapserver::path_cmd_line_to);
         if(shape.line[0].numpoints > 2) {
            MS_IMAGE_RENDERER(img)->renderPolygon(img,&shape,color);
         }
         shape.line[0].point[0].x = x;
         shape.line[0].point[0].y = y;
         shape.line[0].numpoints = 1;
         break;
      default:
         assert(0); //WTF?
      }
      prevCmd = cmd;
   }
   free(shape.line[0].point);
   //assert(prevCmd == mapserver::path_cmd_line_to);
   //delete lines;
   return MS_SUCCESS;
}



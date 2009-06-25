/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose: RFC48 implementation of geometry transformations for styling
 * Author:   Thomas Bonfort , Camptocamp (thomas.bonfort at camptocamp.com)
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

void msStyleSetGeomTransform(styleObj *s, char *transform) {
  msFree(s->_geomtransformexpression);
  s->_geomtransformexpression = strdup(transform);
  if(!strncasecmp("start",transform,5)) {
    s->_geomtransform = MS_GEOMTRANSFORM_START;
  }
  else if(!strncasecmp("end",transform,3)) {
    s->_geomtransform = MS_GEOMTRANSFORM_END;
  }
  else if(!strncasecmp("vertices",transform,8)) {
    s->_geomtransform = MS_GEOMTRANSFORM_VERTICES;
  }
  else if(!strncasecmp("bbox",transform,4)) {
    s->_geomtransform = MS_GEOMTRANSFORM_BBOX;
  }
  else if(!strncasecmp("centroid",transform,8)) {
    s->_geomtransform = MS_GEOMTRANSFORM_CENTROID;
  }
  else {
    s->_geomtransform = MS_GEOMTRANSFORM_NONE;
    msSetError(MS_MISCERR,"unknown transform expression","msStyleSetGeomTransform()");
    msFree(s->_geomtransformexpression);
    s->_geomtransformexpression = NULL;
  }
}


/*
 * return a copy of the geometry transform expression
 * returned char* must be freed by the caller
 */
char *msStyleGetGeomTransform(styleObj *s) {
  if(s->_geomtransformexpression==NULL)
    return NULL;
  return strdup(s->_geomtransformexpression);
}


double calcOrientation(pointObj *p1, pointObj *p2) {
  double theta;
  theta = atan2(p2->x - p1->x,p2->y - p1->y);
  return MS_RAD_TO_DEG*(theta-MS_PI2);
}

double calcMidAngle(pointObj *p1, pointObj *p2, pointObj *p3) {
  double theta1,theta2;
  theta1 = atan2(p1->x-p2->x,p1->y-p2->y);
  if(theta1<0) theta1 += MS_2PI;
  theta2 = atan2(p3->x-p2->x,p3->y-p2->y);
  if(theta2<0) theta2 += MS_2PI;
  return MS_RAD_TO_DEG*((theta1+theta2)/2.0);
}

/*
 * RFC48 implementation:
 *  - transform the original shapeobj
 *  - use the styleObj to render the transformed shapeobj
 */
int msDrawTransformedShape(mapObj *map, symbolSetObj *symbolset, imageObj *image, shapeObj *shape, styleObj *style, double scalefactor){
  int type = style->_geomtransform;
  int i,j;
  switch(type) {
    case MS_GEOMTRANSFORM_END: /*render point on last vertex only*/
      for(j=0;j<shape->numlines;j++) {
        lineObj *line = &(shape->line[j]);
        pointObj *p = &(line->point[line->numpoints-1]);
        if(p->x<0||p->x>image->width||p->y<0||p->y>image->height)
            continue;
        if(style->autoangle==MS_TRUE && line->numpoints>1) {
          style->angle = calcOrientation(&(line->point[line->numpoints-2]),p);
          if(symbolset->symbol[style->symbol]->type==MS_SYMBOL_VECTOR)
            style->angle = - style->angle;
        }
        msDrawMarkerSymbol(symbolset,image,p,style,scalefactor);
      }
      break;
    case MS_GEOMTRANSFORM_START: /*render point on first vertex only*/
      for(j=0;j<shape->numlines;j++) {
        lineObj *line = &(shape->line[j]);
        pointObj *p = &(line->point[0]);
        /*skip if outside image*/
        if(p->x<0||p->x>image->width||p->y<0||p->y>image->height)
            continue;
        if(style->autoangle==MS_TRUE && line->numpoints>1) {
          style->angle = calcOrientation(p,&(line->point[1]));
          if(symbolset->symbol[style->symbol]->type==MS_SYMBOL_VECTOR)
            style->angle = - style->angle;
        }
        msDrawMarkerSymbol(symbolset,image,p,style,scalefactor);
      }
      break;
    case MS_GEOMTRANSFORM_VERTICES:
      for(j=0;j<shape->numlines;j++) {
        lineObj *line = &(shape->line[j]);
        for(i=1;i<line->numpoints-1;i++) {
          pointObj *p = &(line->point[i]);
          /*skip points outside image*/
          if(p->x<0||p->x>image->width||p->y<0||p->y>image->height)
            continue;
          if(style->autoangle==MS_TRUE) {
            style->angle = calcMidAngle(&(line->point[i-1]),&(line->point[i]),&(line->point[i+1]));
            if(symbolset->symbol[style->symbol]->type==MS_SYMBOL_VECTOR)
              style->angle = - style->angle;
          }
          msDrawMarkerSymbol(symbolset,image,p,style,scalefactor);
        }
      }
      break;
    case MS_GEOMTRANSFORM_BBOX: 
      {
        shapeObj bbox;
        lineObj bbox_line;
        pointObj bbox_points[5];
        int padding = MS_MAX(style->width,style->size)+3; //so clipped shape does not extent into image
        
        /*create a shapeObj representing the bounding box (clipped by the image size)*/
        bbox.numlines = 1;
        bbox.line = &bbox_line;
        bbox.line->numpoints = 5;
        bbox.line->point = bbox_points;
 
        msComputeBounds(shape);
        bbox_points[0].x=bbox_points[4].x=bbox_points[1].x = 
            (shape->bounds.minx < -padding) ? -padding : shape->bounds.minx;
        bbox_points[2].x=bbox_points[3].x = 
            (shape->bounds.maxx > image->width+padding) ? image->width+padding : shape->bounds.maxx;
        bbox_points[0].y=bbox_points[4].y=bbox_points[3].y = 
            (shape->bounds.miny < -padding) ? -padding : shape->bounds.miny;
        bbox_points[1].y=bbox_points[2].y =
            (shape->bounds.maxy > image->height+padding) ? image->height+padding : shape->bounds.maxy;
  	    msDrawShadeSymbol(symbolset, image, &bbox, style, scalefactor);
      }
      break;
    case MS_GEOMTRANSFORM_CENTROID:
      {
        double unused; /*used by centroid function*/
        pointObj centroid;
        if(MS_SUCCESS == msGetPolygonCentroid(shape,&centroid,&unused,&unused)){
          msDrawMarkerSymbol(symbolset,image,&centroid,style,scalefactor);
        }
      }
    default:
     msSetError(MS_MISCERR, "unknown geomtransform", "msDrawTransformedShape()");
     return MS_FAILURE;
  }
  return MS_SUCCESS;
}

/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript pointObj extensions
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             
   ===========================================================================
   Copyright (c) 1996-2001 Regents of the University of Minnesota.
   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
 
   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.
 
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ===========================================================================
*/

%extend pointObj 
{
  
    pointObj(double x=0.0, double y=0.0, double m=2e-38) 
    {
        pointObj *p;
        p = (pointObj *)malloc(sizeof(pointObj));
        if (!p) return NULL;
        p->x = x;
        p->y = y;
        if (m > 1e-38)
            p->m = m;
        return p;
    }

    ~pointObj() 
    {
        free(self);
    }

    int project(projectionObj *in, projectionObj *out) 
    {
        return msProjectPoint(in, out, self);
    }	

    int draw(mapObj *map, layerObj *layer, imageObj *image, int classindex, 
             char *text) 
    {
        return msDrawPoint(map, layer, self, image, classindex, text);
    }

    double distanceToPoint(pointObj *point) 
    {
        return msDistancePointToPoint(self, point);
    }

    double distanceToSegment(pointObj *a, pointObj *b) 
    {
        return msDistancePointToSegment(self, a, b);
    }

    double distanceToShape(shapeObj *shape) 
    {
        return msDistancePointToShape(self, shape);
    }

    int setXY(double x, double y, double m=2e-38) 
    {
        self->x = x;
        self->y = y;
        if (m > 1e-38)
        self->m = m;
        return MS_SUCCESS;
    }

}


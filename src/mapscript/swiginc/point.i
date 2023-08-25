/* ===========================================================================
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
    /** 
    * Create new instance. Easting, northing, and measure arguments are optional.
    * Java pointObj constructors are in java/javaextend.i
    * See https://github.com/mapserver/mapserver/issues/1106
    */
#ifndef SWIGJAVA
    pointObj(double x=0.0, double y=0.0, double z=0.0, double m=-2e38) 
    {
        pointObj *p;
        p = (pointObj *)calloc(1,sizeof(pointObj));
        if (!p) return NULL;
        p->x = x;
        p->y = y;
        p->z = z;
        p->m = m;
        return p;
    }
#endif

    ~pointObj() 
    {
        free(self);
    }

    /// Reproject point from proj_in to proj_out. Transformation is done in place.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int project(projectionObj *projin, projectionObj *projout) 
    {
        return msProjectPoint(projin, projout, self);
    }

    /// Reproject point given a reprojection object. Transformation is done in place.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int project(reprojectionObj *reprojector)
    {
        return msProjectPointEx(reprojector, self);
    }

    /// Draw the point using the styles defined by the classindex class of layer and 
    /// labelled with string text. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int draw(mapObj *map, layerObj *layer, imageObj *image, int classindex, 
             char *text) 
    {
        return msDrawPoint(map, layer, self, image, classindex, text);
    }

    /// Returns the distance to point.
    double distanceToPoint(pointObj *point) 
    {
        return msDistancePointToPoint(self, point);
    }

    /// Returns the minimum distance to a hypothetical line segment connecting point1 
    /// and point2.
    double distanceToSegment(pointObj *a, pointObj *b) 
    {
        return msDistancePointToSegment(self, a, b);
    }

    /// Returns the minimum distance to shape.
    double distanceToShape(shapeObj *shape) 
    {
        return msDistancePointToShape(self, shape);
    }

    /**
    * Set spatial coordinate and, optionally, measure values simultaneously. 
    * The measure will be set only if the value of m is greater than the ESRI measure 
    * no-data value of -1e38. 
    * Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    */
    int setXY(double x, double y, double m=-2e38) 
    {
        self->x = x;
        self->y = y;
        self->z = 0.0;
        self->m = m;
        return MS_SUCCESS;
    }

    /**
    * Set spatial coordinate and, optionally, measure values simultaneously. 
    * The measure will be set only if the value of m is greater than the ESRI measure 
    * no-data value of -1e38. 
    * Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    */
    int setXYZ(double x, double y, double z, double m=-2e38) 
    {
        self->x = x;
        self->y = y;
        self->z = z;
        self->m = m;
        return MS_SUCCESS;
    }

    /**
    * Set spatial coordinate and, optionally, measure values simultaneously.
    * The measure will be set only if the value of m is greater than the ESRI measure
    * no-data value of -1e38.
    * Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    */
    int setXYZM(double x, double y, double z, double m) 
    {
        self->x = x;
        self->y = y;
        self->z = z;
        self->m = m;
        return MS_SUCCESS;
    }

    %newobject toString;
    /**
    Return a string formatted like: ``{ 'x': %f , 'y': %f, 'z': %f }``
    with the coordinate values substituted appropriately. Python users can get the same effect 
    via the pointObj  __str__ method:
    
    >>> p = mapscript.pointObj(1, 1)
    >>> str(p)
    { 'x': 1.000000 , 'y': 1.000000, 'z': 1.000000 }
    */
    char *toString() 
    {
        char buffer[256];
        const char *fmt;

        if( self->m < -1e38 ){
            fmt = "{ 'x': %.16g, 'y': %.16g, 'z': %.16g }";
        }
        else {
            fmt = "{ 'x': %.16g, 'y': %.16g, 'z': %.16g, 'm': %.16g }";
        }

msPointToFormattedString(self, fmt, (char *) &buffer, 256);
    return msStrdup(buffer);
    }

    %newobject toShape;
    /// Convert to a new :class:`shapeObj`
    shapeObj *toShape() 
    {
      shapeObj *shape;

      shape = (shapeObj *) malloc(sizeof(shapeObj));
      msInitShape(shape);
    
      shape->type = MS_SHAPE_POINT;
      shape->line = (lineObj *) malloc(sizeof(lineObj));
      shape->numlines = 1;
      shape->line[0].point = (pointObj *) malloc(sizeof(pointObj));
      shape->line[0].numpoints = 1;

      shape->line[0].point[0].x = self->x;
      shape->line[0].point[0].y = self->y;
      shape->line[0].point[0].z = self->z;
      shape->line[0].point[0].m = self->m;

      return shape;
    }
}

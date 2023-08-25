/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript rectObj extensions
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

%extend rectObj {

    /**
    * Create new instance. The four easting and northing arguments are optional and 
    * default to -1.0. Note the new optional fifth argument which allows creation 
    * of rectangles in image (pixel/line) units which are also tested for validity.
    */
    rectObj(double minx=-1.0, double miny=-1.0, 
            double maxx=-1.0, double maxy=-1.0,
            int imageunits=MS_FALSE) 
    {
        rectObj *rect;
    
        if (imageunits == MS_FALSE)
        { 
            if (minx > maxx || miny > maxy)
            {
                msSetError(MS_RECTERR,
                    "{ 'minx': %f , 'miny': %f , 'maxx': %f , 'maxy': %f }",
                    "rectObj()", minx, miny, maxx, maxy);
                return NULL;
            }
        }
        else 
        {
            if (minx > maxx || maxy > miny) 
            {
                msSetError(MS_RECTERR,
                    "image (pixel/line) units { 'minx': %f , 'miny': %f , 'maxx': %f , 'maxy': %f }",
                    "rectObj()", minx, miny, maxx, maxy);
                return NULL;
            }
        }
    
        rect = (rectObj *)calloc(1, sizeof(rectObj));
        if (!rect)
            return(NULL);
    
        rect->minx = minx;
        rect->miny = miny;
        rect->maxx = maxx;
        rect->maxy = maxy;

        return(rect);
    }

    ~rectObj() {
        free(self);
    }

    /// Reproject rectangle from proj_in to proj_out. Transformation is done in place. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int project(projectionObj *projin, projectionObj *projout) {
        return msProjectRect(projin, projout, self);
    }

    /// Reproject rectangle given a reprojection object. Transformation is done in place.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int project(reprojectionObj *reprojector)
    {
        return msProjectRectAsPolygon(reprojector, self);
    }

    /// Adjust the rect to fit the width and height. Returns cellsize of rect.
    double fit(int width, int height) {
        return  msAdjustExtent(self, width, height);
    } 

    /// Draw rectangle into img using style defined by the classindex class of layer. 
    /// The rectangle is labeled with the string text. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int draw(mapObj *map, layerObj *layer, imageObj *image, 
             int classindex, char *text) 
    {
        shapeObj shape;
        int ret;

        msInitShape(&shape);
        msRectToPolygon(*self, &shape);
        shape.classindex = classindex;
        if(text && layer->class[classindex]->numlabels > 0) {
          shape.text = msStrdup(text);
        }
        
        ret = msDrawShape(map, layer, &shape, image, -1, MS_DRAWMODE_FEATURES|MS_DRAWMODE_LABELS);

        msFreeShape(&shape);
    
        return ret;
    }
    
    %newobject getCenter;
    /// Return the center point of the rectangle.
    pointObj *getCenter() 
    {
        pointObj *center;
        center = (pointObj *)calloc(1, sizeof(pointObj));
        if (!center) {
            msSetError(2, "Failed to allocate memory for point", "getCenter()");
            return NULL;
        }
        center->x = (self->minx + self->maxx)/2;
        center->y = (self->miny + self->maxy)/2;
        
        return center;
    }

    %newobject toPolygon;
    /// Convert to a polygon of five vertices.
    shapeObj *toPolygon() 
    {
        lineObj line = {0,NULL};
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = MS_SHAPE_POLYGON;
  
        line.point = (pointObj *)malloc(sizeof(pointObj)*5);
        line.point[0].x = self->minx;
        line.point[0].y = self->miny;
        line.point[1].x = self->minx;
        line.point[1].y = self->maxy;
        line.point[2].x = self->maxx;
        line.point[2].y = self->maxy;
        line.point[3].x = self->maxx;
        line.point[3].y = self->miny;
        line.point[4].x = line.point[0].x;
        line.point[4].y = line.point[0].y;
  
        line.numpoints = 5;
  
        msAddLine(shape, &line);
        msComputeBounds(shape);
        
        free(line.point);

        return shape;
    }

    %newobject toString;
    /**
    Return a string formatted like: ``{ 'minx': %f , 'miny': %f , 'maxx': %f , 'maxy': %f }``
    with the bounding values substituted appropriately. Python users can get the same effect 
    via the rectObj __str__ method:
    
    >>> r = mapscript.rectObj(0, 0, 1, 1)
    >>> str(r)
    { 'minx': 0 , 'miny': 0 , 'maxx': 1 , 'maxy': 1 }
    */
    char *toString()
    {
        char buffer[256];
        char fmt[]="{ 'minx': %.16g , 'miny': %.16g , 'maxx': %.16g , 'maxy': %.16g }";
        msRectToFormattedString(self, (char *) &fmt, (char *) &buffer, 256);
        return msStrdup(buffer);
    }
}

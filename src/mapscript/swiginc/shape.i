/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript shapeObj extensions
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

%extend shapeObj 
{
    /**
    * Return a new shapeObj of the specified type. See the type attribute. 
    * No attribute values created by default. 
    * initValues should be explicitly called to create the required number of values. 
    * Each feature of a layer's data is a shapeObj. Each part of the shape is a 
    * closed :class:`lineObj`
    */
    shapeObj(int type=MS_SHAPE_NULL) 
    {
        shapeObj *shape;

    shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;

        msInitShape(shape);
        if(type >= 0) shape->type = type;
        
        return shape;
    }

    ~shapeObj() 
    {
        msFreeShape(self);
        free(self);		
    }

    %newobject fromWKT;
    /// Returns a new shapeObj based on a well-known text representation of a geometry. 
    /// Requires GEOS support. Returns NULL/undef on failure.
    static shapeObj *fromWKT(char *wkt)
    {
    shapeObj *shape;
        
        if(!wkt) return NULL;

        shape = msShapeFromWKT(wkt);
    if(!shape) return NULL;

    return shape;
    }

    /// Reproject shape from proj_in to proj_out. Transformation is done in place. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int project(projectionObj *projin, projectionObj *projout) 
    {
        return msProjectShape(projin, projout, self);
    }

    /// Reproject shape given a reprojection object. Transformation is done in place.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int project(reprojectionObj *reprojector)
    {
        return msProjectShapeEx(reprojector, self);
    }

    /// Returns a reference to part at index. Reference is valid only during 
    /// the life of the shapeObj.
    lineObj *get(int i) {
        if (i<0 || i>=self->numlines)
            return NULL;
        else
            return &(self->line[i]);
    }

    /// Add line (i.e. a part) to the shape. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int add(lineObj *line) {
        return msAddLine(self, line);
    }

    /// Draws the individual shape using layer. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int draw(mapObj *map, layerObj *layer, imageObj *image) {
        return msDrawShape(map, layer, self, image, -1, MS_DRAWMODE_FEATURES|MS_DRAWMODE_LABELS);
    }

    /// Must be called to calculate new bounding box after new parts have been added.
    /// TODO: should return int and set msSetError.
    void setBounds() 
    {    
        msComputeBounds(self);
        return;
    }

#if defined (SWIGJAVA) || defined (SWIGPHP)
    %newobject cloneShape;
    shapeObj *cloneShape()
#else
    %newobject clone;
    /**
    Return an independent copy of the shape.

    .. note::

        In the Java & PHP modules this method is named ``cloneShape``.    
    */    
    shapeObj *clone()
#endif
    {
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = self->type;
        msCopyShape(self, shape);
        return shape;
    }

    /// Copy the shape to shape_copy. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int copy(shapeObj *dest) 
    {
        return(msCopyShape(self, dest));
    }

    %newobject toWKT;
    /// Returns the well - known text representation of a shapeObj.
    /// Requires GEOS support.Returns NULL / undefined on failure.
    char *toWKT()
    {
        return msShapeToWKT(self);
    }

    %newobject buffer;
    /// Returns a new buffered shapeObj based on the supplied distance 
    /// (given in the coordinates of the existing shapeObj). 
    /// Requires GEOS support. Returns NULL/undefined on failure.
    shapeObj *buffer(double width) { return msGEOSBuffer(self, width); }

    %newobject simplify;
    /// Given a tolerance, returns a simplified shape object or NULL on error. 
    /// Requires GEOS support (>=3.0).
    shapeObj *simplify(double tolerance) { return msGEOSSimplify(self, tolerance); }

    %newobject topologyPreservingSimplify;
    /// Given a tolerance, returns a simplified shape object or NULL on error. 
    /// Requires GEOS support (>=3.0).
    shapeObj *topologyPreservingSimplify(double tolerance) { return msGEOSTopologyPreservingSimplify(self, tolerance); }

    %newobject convexHull;
    /// Returns the convex hull of the existing shape. 
    /// Requires GEOS support. Returns NULL/undef on failure.
    shapeObj *convexHull() { return msGEOSConvexHull(self); }

    %newobject boundary;
    /// Returns the boundary of the existing shape. 
    /// Requires GEOS support. Returns NULL/undef on failure.
    shapeObj *boundary() { return msGEOSBoundary(self); }

    %newobject getCentroid;
    /// Returns the centroid for the existing shape. 
    /// Requires GEOS support. Returns NULL/undef on failure.
    pointObj *getCentroid() { return msGEOSGetCentroid(self); }

    %newobject Union;
    /// Returns the union of the existing and supplied shape. 
    /// Shapes must be of the same type. 
    /// Requires GEOS support. Returns NULL/undef on failure.
    shapeObj *Union(shapeObj *shape) { return msGEOSUnion(self, shape); }

    %newobject intersection;
    /// Returns the computed intersection of the supplied and existing shape. 
    /// Requires GEOS support. Returns NULL/undef on failure.
    shapeObj *intersection(shapeObj *shape) { return msGEOSIntersection(self, shape); }

    %newobject difference;
    /// Returns the computed difference of the supplied and existing shape. 
    /// Requires GEOS support. Returns NULL/undef on failure.
    shapeObj *difference(shapeObj *shape) { return msGEOSDifference(self, shape); }

    %newobject symDifference;
    /// Returns the computed symmetric difference of the supplied and existing shape. 
    /// Requires GEOS support. Returns NULL/undef on failure.
    shapeObj *symDifference(shapeObj *shape) { return msGEOSSymDifference(self, shape); }

    /// Returns :data:`MS_TRUE` if shape2 is entirely inside the shape, returns -1 on error and :data:`MS_FALSE` otherwise.
    /// Requires GEOS support.
    int contains(shapeObj *shape) { return msGEOSContains(self, shape); }

    /// Returns :data:`MS_TRUE` if shape2 overlaps shape, returns -1 on error and :data:`MS_FALSE` otherwise.
    /// Requires GEOS support.
    int overlaps(shapeObj *shape) { return msGEOSOverlaps(self, shape); }

    /// Returns :data:`MS_TRUE` if shape is entirely within shape2, returns -1 on error and :data:`MS_FALSE` otherwise.
    /// Requires GEOS support.
    int within(shapeObj *shape) { return msGEOSWithin(self, shape); }

    /// Returns :data:`MS_TRUE` if shape2 crosses the shape, returns -1 on error and :data:`MS_FALSE` otherwise.
    /// Requires GEOS support.
    int crosses(shapeObj *shape) { return msGEOSCrosses(self, shape); }

    /// Returns :data:`MS_TRUE` if shape and shape2 intersect, and :data:`MS_FALSE` otherwise.
    /// Does not require GEOS support but will use GEOS functions if available.
    int intersects(shapeObj *shape) { return msGEOSIntersects(self, shape); } /* if GEOS is not present an alternative computation is provided, see mapgeos.c */

    /// Returns :data:`MS_TRUE` if shape and shape2 touch, returns -1 on error and :data:`MS_FALSE` otherwise.
    /// Requires GEOS support.
    int touches(shapeObj *shape) { return msGEOSTouches(self, shape); }

    /// Returns :data:`MS_TRUE` if shape and shape2 are equal (geometry only), returns -1 on error and :data:`MS_FALSE` otherwise.
    /// Requires GEOS support.
    int equals(shapeObj *shape) { return msGEOSEquals(self, shape); }

    /// Returns :data:`MS_TRUE` if shape and shape2 are disjoint, returns -1 on error and :data:`MS_FALSE` otherwise.
    /// Requires GEOS support.
    int disjoint(shapeObj *shape) { return msGEOSDisjoint(self, shape); }

    /// Returns the area of the shape (if applicable). Requires GEOS support. 
    double getArea() { return msGEOSArea(self); }

    /// Returns the length (or perimeter) of the shape. Requires GEOS support. 
    double getLength() { return msGEOSLength(self); }

    /// Return the shape attribute at index i.
    char *getValue(int i) 
    {
        if (i >= 0 && i < self->numvalues && self->values)
            return (self->values[i]);
        else
            return NULL;
    }

    /// Returns :data:`MS_TRUE` if the point is inside the shape, :data:`MS_FALSE` otherwise
    int contains(pointObj *point) 
    {
        if (self->type == MS_SHAPE_POLYGON)
            return msIntersectPointPolygon(point, self);
        
        return -1;
    }

    /// Returns the distance to point.
    double distanceToPoint(pointObj *point) 
    {
        return msDistancePointToShape(point, self); /* should there be a GEOS version of this? */
    }

    /// Returns the minimum distance to shape.
    double distanceToShape(shapeObj *shape) 
    {
        return msGEOSDistance(self, shape); /* note this calls msDistanceShapeToShape() if GEOS support is not present */
    }

    %newobject getLabelPoint;
    /// Given a shape, return a point object suitable for labelling it. 
    pointObj *getLabelPoint()
    {
        pointObj *point = (pointObj *)calloc(1, sizeof(pointObj));
        if (point == NULL) {
            msSetError(MS_MEMERR, "Failed to allocate memory for point", "labelPoint()");
            return NULL;
        }

        if(self->type == MS_SHAPE_POLYGON && msPolygonLabelPoint(self, point, -1) == MS_SUCCESS)
            return point;

        free(point);
        return NULL;
    }

    /// Set the shape value at index i to value.
    int setValue(int i, char *value)
    {
        if (!self->values || !value)
        {
            msSetError(MS_SHPERR, "Can't set value", "setValue()");
            return MS_FAILURE;
        }
        if (i >= 0 && i < self->numvalues)
        {
            msFree(self->values[i]);
            self->values[i] = msStrdup(value);
            if (!self->values[i])
            {
                return MS_FAILURE;
            }
            else return MS_SUCCESS;
        }
        else
        {
            msSetError(MS_SHPERR, "Invalid value index", 
                                  "setValue()");
            return MS_FAILURE;
        }
    }

    /// Allocates memory for the requested number of values.
    void initValues(int numvalues)
    {
        int i;
        
        if(self->values) msFreeCharArray(self->values, self->numvalues);
        self->values = NULL;
        self->numvalues = 0;
        
        /* Allocate memory for the values */
        if (numvalues > 0) {
            if ((self->values = (char **)malloc(sizeof(char *)*numvalues)) == NULL) {
                msSetError(MS_MEMERR, "Failed to allocate memory for values", "shapeObj()");
                return;
            } else {
                for (i=0; i<numvalues; i++) self->values[i] = msStrdup("");
            }
            self->numvalues = numvalues;
        }
    }
}

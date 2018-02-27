/* ===========================================================================
   $Id$
 
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
    static shapeObj *fromWKT(char *wkt)
    {
	shapeObj *shape;
        
        if(!wkt) return NULL;

        shape = msShapeFromWKT(wkt);
	if(!shape) return NULL;

	return shape;
    }

    int project(projectionObj *projin, projectionObj *projout) 
    {
        return msProjectShape(projin, projout, self);
    }

    lineObj *get(int i) {
        if (i<0 || i>=self->numlines)
            return NULL;
        else
            return &(self->line[i]);
    }

    int add(lineObj *line) {
        return msAddLine(self, line);
    }

    int draw(mapObj *map, layerObj *layer, imageObj *image) {
        return msDrawShape(map, layer, self, image, -1, MS_DRAWMODE_FEATURES|MS_DRAWMODE_LABELS);
    }

    void setBounds() 
    {    
        msComputeBounds(self);
        return;
    }

#ifdef SWIGJAVA
    %newobject cloneShape;
    shapeObj *cloneShape()
#else
    %newobject clone;
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

    int copy(shapeObj *dest) 
    {
        return(msCopyShape(self, dest));
    }

    %newobject toWKT;
    char *toWKT()
    {
        return msShapeToWKT(self);
    }

    %newobject buffer;
    shapeObj *buffer(double width) { return msGEOSBuffer(self, width); }

    %newobject simplify;
    shapeObj *simplify(double tolerance) { return msGEOSSimplify(self, tolerance); }

    %newobject topologyPreservingSimplify;
    shapeObj *topologyPreservingSimplify(double tolerance) { return msGEOSTopologyPreservingSimplify(self, tolerance); }

    %newobject convexHull;
    shapeObj *convexHull() { return msGEOSConvexHull(self); }

    %newobject boundary;
    shapeObj *boundary() { return msGEOSBoundary(self); }

    %newobject getCentroid;
    pointObj *getCentroid() { return msGEOSGetCentroid(self); }

    %newobject Union;
    shapeObj *Union(shapeObj *shape) { return msGEOSUnion(self, shape); }

    %newobject intersection;
    shapeObj *intersection(shapeObj *shape) { return msGEOSIntersection(self, shape); }

    %newobject difference;
    shapeObj *difference(shapeObj *shape) { return msGEOSDifference(self, shape); }

    %newobject symDifference;
    shapeObj *symDifference(shapeObj *shape) { return msGEOSSymDifference(self, shape); }

    int contains(shapeObj *shape) { return msGEOSContains(self, shape); }
    int overlaps(shapeObj *shape) { return msGEOSOverlaps(self, shape); }
    int within(shapeObj *shape) { return msGEOSWithin(self, shape); }
    int crosses(shapeObj *shape) { return msGEOSCrosses(self, shape); }
    int intersects(shapeObj *shape) { return msGEOSIntersects(self, shape); } /* if GEOS is not present an alternative computation is provided, see mapgeos.c */
    int touches(shapeObj *shape) { return msGEOSTouches(self, shape); }
    int equals(shapeObj *shape) { return msGEOSEquals(self, shape); }
    int disjoint(shapeObj *shape) { return msGEOSDisjoint(self, shape); }

    double getArea() { return msGEOSArea(self); }
    double getLength() { return msGEOSLength(self); }

    char *getValue(int i) 
    {
        if (i >= 0 && i < self->numvalues && self->values)
            return (self->values[i]);
        else
            return NULL;
    }

    int contains(pointObj *point) 
    {
        if (self->type == MS_SHAPE_POLYGON)
            return msIntersectPointPolygon(point, self);
        
        return -1;
    }

    double distanceToPoint(pointObj *point) 
    {
        return msDistancePointToShape(point, self); /* should there be a GEOS version of this? */
    }

    double distanceToShape(shapeObj *shape) 
    {
	return msGEOSDistance(self, shape); /* note this calls msDistanceShapeToShape() if GEOS support is not present */
    }

    %newobject getLabelPoint;
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

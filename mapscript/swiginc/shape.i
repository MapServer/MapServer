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
        int i;
        shapeObj *shape;

	shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;

        msInitShape(shape);
        if(type >= 0) shape->type = type;

        /* Allocate memory for 4 values */
        if ((shape->values = (char **)malloc(sizeof(char *)*4)) == NULL) {
            msSetError(MS_MEMERR, "Failed to allocate memory for values", "shapeObj()");
            return NULL;
        } else {
            for (i=0; i<4; i++) shape->values[i] = strdup("");
        }
        shape->numvalues = 4;
        
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
        int i;

        if(!wkt) return NULL;

        shape = msShapeFromWKT(wkt);
	if(!shape) return NULL;

        /* Allocate memory for 4 values */
        if ((shape->values = (char **)malloc(sizeof(char *)*4)) == NULL) {
            msSetError(MS_MEMERR, "Failed to allocate memory for values", "fromWKT()");
            return NULL;
        } else {
            for (i=0; i<4; i++) shape->values[i] = strdup("");
        }
        shape->numvalues = 4;

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
        return msDrawShape(map, layer, self, image, -1);
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
    shapeObj *buffer(int width)
    {
       return msGEOSBuffer(self, width);
    }

    %newobject convexHull;
    shapeObj *convexHull()
    {
       return msGEOSConvexHull(self);
    }

    int contains(shapeObj *shape)
    {
       return msGEOSContains(self, shape);
    }

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
        return msDistancePointToShape(point, self);
    }

    double distanceToShape(shapeObj *shape) 
    {
        return msDistanceShapeToShape(self, shape);
    }

    int intersects(shapeObj *shape) 
    {
        switch(self->type) {
            case(MS_SHAPE_LINE):
                switch(shape->type) {
                    case(MS_SHAPE_LINE):
	                    return msIntersectPolylines(self, shape);
                    case(MS_SHAPE_POLYGON):
	                    return msIntersectPolylinePolygon(self, shape);
                }
                break;
            case(MS_SHAPE_POLYGON):
                switch(shape->type) {
                    case(MS_SHAPE_LINE):
	                    return msIntersectPolylinePolygon(shape, self);
                    case(MS_SHAPE_POLYGON):
	                    return msIntersectPolygons(self, shape);
                }
                break;
            }

        return -1;
    }
   
    int setValue(int i, char *value)
    {
        if (!self->values || !value)
        {
            msSetError(MS_SHPERR, "Can't set value", "setValue()");
            return MS_FAILURE;
        }
        if (i >= 0 && i < 4)
        {
            msFree(self->values[i]);
            self->values[i] = strdup(value);
            if (!self->values[i])
            {
                return MS_FAILURE;
            }
            else return MS_SUCCESS;
        }
        else
        {
            msSetError(MS_SHPERR, "Invalid index, only 4 possible values", 
                                  "setValue()");
            return MS_FAILURE;
        }
    }
}


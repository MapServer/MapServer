/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript lineObj extensions
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

%extend lineObj 
{

    /// A :class:`lineObj` is composed of one or more :class:`pointObj` instances
    lineObj() 
    {
        lineObj *line;

        line = (lineObj *)malloc(sizeof(lineObj));
        if (!line)
            return(NULL);

        line->numpoints=0;
        line->point=NULL;

        return line;
    }

    ~lineObj() 
    {
        free(self->point);
        free(self);		
    }

    /// Transform line in place from proj_in to proj_out.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int project(projectionObj *projin, projectionObj *projout) 
    {
        return msProjectLine(projin, projout, self);
    }

    /// Reproject line given a reprojection object. Transformation is done in place.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int project(reprojectionObj *reprojector)
    {
        return msProjectLineEx(reprojector, self);
    }

    /// Return reference to point at index *i*.
    pointObj *get(int i) 
    {
        if (i<0 || i>=self->numpoints)
            return NULL;
        else
            return &(self->point[i]);
    }

    /// Add point *p* to the line.
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int add(pointObj *p)
    {
        if (self->numpoints == 0) { /* new */	
        self->point = (pointObj *)malloc(sizeof(pointObj));
        if (!self->point)
            return MS_FAILURE;
        } else { /* extend array */
            self->point = (pointObj *)realloc(self->point, 
                                      sizeof(pointObj)*(self->numpoints+1));
        if (!self->point)
            return MS_FAILURE;
        }

        self->point[self->numpoints].x = p->x;
        self->point[self->numpoints].y = p->y;
        self->point[self->numpoints].z = p->z;
        self->point[self->numpoints].m = p->m;
        self->numpoints++;

        return MS_SUCCESS;
    }

    /// Set the point at index *i* to point *p*. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int set(int i, pointObj *p)
    {
        if (i<0 || i>=self->numpoints)
            return MS_FAILURE;

        self->point[i].x = p->x;
        self->point[i].y = p->y;
        self->point[i].z = p->z;
        self->point[i].m = p->m;
        return MS_SUCCESS;
    }

}

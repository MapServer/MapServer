/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript shapefileObj extensions
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

%extend shapefileObj 
{

    shapefileObj(char *filename, int type=-1) 
    {    
        shapefileObj *shapefile;
        int status;

        shapefile = (shapefileObj *)malloc(sizeof(shapefileObj));
        if (!shapefile)
            return NULL;

        if (type == -1)
            status = msShapefileOpen(shapefile, "rb", filename, MS_TRUE);
        else if (type == -2)
            status = msShapefileOpen(shapefile, "rb+", filename, MS_TRUE);
        else
            status = msShapefileCreate(shapefile, filename, type);

        if (status == -1) {
            msShapefileClose(shapefile);
            free(shapefile);
            return NULL;
        }
 
        return(shapefile);
    }

    ~shapefileObj() 
    {
        msShapefileClose(self);
        free(self);  
    }

    int get(int i, shapeObj *shape) 
    {
        if (i<0 || i>=self->numshapes)
            return MS_FAILURE;

        msFreeShape(shape); /* frees all lines and points before re-filling */
        msSHPReadShape(self->hSHP, i, shape);

        return MS_SUCCESS;
    }

    %newobject getShape;
    shapeObj *getShape(int i)
    {
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = self->type;
        msSHPReadShape(self->hSHP, i, shape);
        return shape;

    }

    int getPoint(int i, pointObj *point) 
    {
        if (i<0 || i>=self->numshapes)
            return MS_FAILURE;

        msSHPReadPoint(self->hSHP, i, point);
        return MS_SUCCESS;
    }

    int getTransformed(mapObj *map, int i, shapeObj *shape) 
    {
        if (i<0 || i>=self->numshapes)
            return MS_FAILURE;

        msFreeShape(shape); /* frees all lines and points before re-filling */
        msSHPReadShape(self->hSHP, i, shape);
        msTransformShapeToPixel(shape, map->extent, map->cellsize);

        return MS_SUCCESS;
    }

    void getExtent(int i, rectObj *rect) 
    {
        msSHPReadBounds(self->hSHP, i, rect);
    }

    int add(shapeObj *shape) 
    {
        /* Trap NULL or empty shapes -- bug 1201 */
        if (!shape) 
        {
            msSetError(MS_SHPERR, "Can't add NULL shape", "shapefileObj::add");
            return MS_FAILURE;
        }
        else if (!shape->line)
        {
            msSetError(MS_SHPERR, "Can't add empty shape", "shapefileObj::add");
            return MS_FAILURE;
        }

        return msSHPWriteShape(self->hSHP, shape);	
    }	

    int addPoint(pointObj *point) 
    {    
        return msSHPWritePoint(self->hSHP, point);	
    }
    
    DBFInfo *getDBF() {
    	return self->hDBF;
    }
    
}


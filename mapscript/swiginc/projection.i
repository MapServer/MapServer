/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript projectionObj extensions
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

%extend projectionObj 
{

    /// Create new instance of projectionObj. Input parameter proj4 is a 
    /// PROJ definition string such as 'init=EPSG:4269'
    projectionObj(char *proj4) 
    {
        int status;
        projectionObj *proj=NULL;

        proj = (projectionObj *)malloc(sizeof(projectionObj));
        if (!proj) return NULL;
        msInitProjection(proj);

        status = msLoadProjectionString(proj, proj4);
        if (status == -1) {
            msFreeProjection(proj);
            free(proj);
            return NULL;
        }

        return proj;
    }

    ~projectionObj() 
    {
        msFreeProjection(self);
        free(self);
    }

    /// Update the projectionObj using an OGC WKT definition
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int setWKTProjection(char* wkt) {
        /* no debug output here */
        return msOGCWKT2ProjectionObj(wkt, self, MS_FALSE);
    }

    /// Returns the units of a projection object. Returns -1 on error.
    int getUnits() {
      return GetMapserverUnitUsingProj(self);
    }
}

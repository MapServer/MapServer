/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript webObj extensions
   Author:   Steve Lime 
             Umberto Nicoletti unicoletti@prometeo.it
             
   ===========================================================================
   Copyright (c) 1996-2020 Regents of the University of Minnesota.
   
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

%extend webObj 
{
    /**
    * Instances of webObj are always are always embedded inside the :class:`mapObj`. 
    * Has no other existence than as an attribute of a :class:`mapObj`. Serves as a container for various run-time 
    * web application definitions like temporary file paths, template paths, etc.
    * See https://github.com/mapserver/mapserver/issues/1798
    */
    webObj() 
    {
        webObj *web;
        web = (webObj *) malloc(sizeof(webObj));
        initWeb(web);
        return web;
    }

    ~webObj() 
    {
        if (!self) return;
        freeWeb(self);
        free(self);
    }

    /// Update a web object from a string snippet. Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int updateFromString(char *snippet)
    {
        return msUpdateWebFromString(self, snippet, MS_FALSE);
    }

    %newobject convertToString;
    /// Output the WEB object as a Mapfile string. Provides the inverse option for updateFromString.
    char* convertToString()
    {
        return msWriteWebToString(self);
    }
}

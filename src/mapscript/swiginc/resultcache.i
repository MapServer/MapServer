/* ===========================================================================   
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript resultCacheObj extensions
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

%extend resultCacheObj
{
    /// Returns the result at index i, like :meth:`layerObj.getResult`,
    /// or ``NULL`` if index is outside the range of results.
    resultObj *getResult(int i)
    {
        if (i >= 0 && i < self->numresults) {
            return &self->results[i];
        }
        return NULL;
    }

}


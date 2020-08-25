/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript errorObj extensions
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


/* A few things necessary for automatically wrapped functions */
%newobject msGetErrorString;

%extend errorObj
{
    /// Create a new instance
    errorObj() 
    {    
        return msGetErrorObj();
    }

    ~errorObj() {}

    /// Returns the next error in the stack or NULL if the end has been reached
    errorObj *next() 
    {
        errorObj *ep;

        if (self == NULL || self->next == NULL) return NULL;

        ep = msGetErrorObj();
        while (ep != self) {
            /* We reached end of list of active errorObj and 
               didn't find the errorObj... this is bad! */
            if (ep->next == NULL) return NULL;
            ep = ep->next;
        }
    
        return ep->next;
    }
}


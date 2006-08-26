/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript styleObj extensions
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

   See Bugzilla issue 548 about work on styleObj and classObj
*/

%extend styleObj {

    styleObj(classObj *parent_class=NULL) 
    {
    
        styleObj *style;
        int result;
        
        if (!parent_class) 
        { 
            style = (styleObj *) malloc(sizeof(styleObj));
            if (!style) return NULL;
            result = initStyle(style);
            if (result == MS_SUCCESS) 
            {
                style->isachild = MS_FALSE;
                return style;
            }
            else 
            {
                msSetError(MS_MISCERR, "Failed to initialize styleObj",
                                       "styleObj()");
                return NULL;
            }
        }
        else 
        {
            if (parent_class->numstyles == MS_MAXSTYLES) 
            {
                msSetError(MS_CHILDERR, "Exceeded max number of styles: %d",
                           "styleObj()", MS_MAXSTYLES);
                return NULL;
            }
            parent_class->numstyles++;
            return &(parent_class->styles[parent_class->numstyles-1]);
        }
    }
   
    ~styleObj() 
    {
        if (self->isachild == MS_FALSE) 
            free(self);
    }

#ifdef SWIGJAVA
    %newobject cloneStyle;
    styleObj *cloneStyle() 
#else
    %newobject clone;
    styleObj *clone() 
#endif
    {
        styleObj *style;

        style = (styleObj *) malloc(sizeof(styleObj));
        if (!style)
        {
            msSetError(MS_MEMERR,
                "Could not allocate memory for new styleObj instance",
                "clone()");
            return NULL;
        }
        if (initStyle(style) == -1)
        {
            msSetError(MS_MEMERR, "Failed to initialize Style",
                                  "clone()");
            return NULL;
        }

        if (msCopyStyle(style, self) != MS_SUCCESS)
        {
            free(style);
            return NULL;
        }
        
        style->isachild = MS_FALSE;
        return style;
    }
    
    int setSymbolByName(mapObj *map, char* symbolname) 
    {
        self->symbol = msGetSymbolIndex(&map->symbolset, symbolname, MS_TRUE);
        if (self->symbolname) free((char*)self->symbolname);
        if (symbolname) self->symbolname = strdup(symbolname);
        else self->symbolname = 0;
        return self->symbol;
	}
}

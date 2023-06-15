/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript styleObj extensions
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             
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

// See https://github.com/mapserver/mapserver/issues/548

%extend styleObj {

    /// Returns new default :class:`styleObj` instance. The ``parent_class`` is optional. 
    styleObj(classObj *parent_class=NULL) 
    {
    
        styleObj *style = NULL;
        
        if (parent_class!=NULL) {
            if ((style = msGrowClassStyles(parent_class)) == NULL)
                return NULL;

            if ( initStyle(style) == MS_FAILURE ) {
                msSetError(MS_MISCERR, "Failed to init new styleObj instance",
                                       "initStyle()");
            }
            parent_class->numstyles++;
            MS_REFCNT_INCR(style);
        }
        else {
            style = (styleObj *) malloc(sizeof(styleObj));
            if (!style) { 
                msSetError(MS_MEMERR, "Failed to allocate memory for new styleObj instance",
                                       "styleObj()");
                return NULL;
            }
            if ( initStyle(style) == MS_FAILURE ) {
                msSetError(MS_MISCERR, "Failed to init new styleObj instance",
                                       "initStyle()");
                msFree(style);
                return NULL;
            }
        }
        return style;
    }
   
    ~styleObj() 
    {
        if (self) { 
            if ( freeStyle(self) == MS_SUCCESS ) {
                        free(self);
                self=NULL;
            }
        }
    }

    /// Update a style from a string snippet. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int updateFromString(char *snippet)
    {
        return msUpdateStyleFromString(self, snippet);
    }

    %newobject convertToString;
    /// Saves the object to a string. Provides the inverse option for :func:`styleObj.updateFromString` 
    char* convertToString()
    {
        return msWriteStyleToString(self);
    }

#if defined (SWIGJAVA) || defined (SWIGPHP)
    %newobject cloneStyle;
    styleObj *cloneStyle() 
#else
    %newobject clone;
    /**
    Return an independent copy of the style with no parent class.

    .. note::

        In the Java & PHP modules this method is named ``cloneStyle``.    
    */ 
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
        
        return style;
    }

    /// Setting the symbol of the :class:`styleObj` given the reference of the map 
    /// object and the symbol name. 
    int setSymbolByName(mapObj *map, char* symbolname) 
    {
        self->symbol = msGetSymbolIndex(&map->symbolset, symbolname, MS_TRUE);
        if (self->symbolname) free((char*)self->symbolname);
        if (symbolname) self->symbolname = msStrdup(symbolname);
        else self->symbolname = 0;
        return self->symbol;
    }

    /// Remove the attribute binding for a specified style property. 
    int removeBinding(int binding) 
    {
        if(binding < 0 || binding >= MS_STYLE_BINDING_LENGTH) return MS_FAILURE;

        if(self->bindings[binding].item) {
            free(self->bindings[binding].item);
            self->bindings[binding].item = NULL;
            self->bindings[binding].index = -1;
            self->numbindings--;
        }

        return MS_SUCCESS;
    }

    /// Remove the attribute binding for a specified style property. 
    int setBinding(int binding, const char *item) 
    {
        if(!item) return MS_FAILURE;
        if(binding < 0 || binding >= MS_STYLE_BINDING_LENGTH) return MS_FAILURE;

        if(self->bindings[binding].item) {
            free(self->bindings[binding].item);
            self->bindings[binding].item = NULL;
            self->bindings[binding].index = -1;
            self->numbindings--;
        }

        self->bindings[binding].item = msStrdup(item);
        self->numbindings++;

        return MS_SUCCESS;
    }

    /// Get the attribute binding for a specified style property. 
    /// Returns NULL if there is no binding for this property. 
    const char *getBinding(int binding) 
    {
        if(binding < 0 || binding >= MS_STYLE_BINDING_LENGTH) return NULL;

        return self->bindings[binding].item;
    }

    /// Get the :ref:`GEOMTRANSFORM <mapfile-style-geomtransform>` for the style. 
    char *getGeomTransform() 
    {
        return self->_geomtransform.string;
    }

    /// Set the :ref:`GEOMTRANSFORM <mapfile-style-geomtransform>` for the style. 
    void setGeomTransform(char *transform) 
    {
        msStyleSetGeomTransform(self, transform);
    }
}

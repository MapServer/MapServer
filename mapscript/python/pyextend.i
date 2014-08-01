/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Python-specific extensions to MapScript objects
 * Author:   Sean Gillies, sgillies@frii.com
 *
 ******************************************************************************
 *
 * Python-specific mapscript code has been moved into this 
 * SWIG interface file to improve the readibility of the main
 * interface file.  The main mapscript.i file includes this
 * file when SWIGPYTHON is defined (via 'swig -python ...').
 *
 *****************************************************************************/

/* fromstring: Factory for mapfile objects */

%pythoncode {
def fromstring(data, mappath=None):
    """Creates map objects from mapfile strings.

    Parameters
    ==========
    data : string
        Mapfile in a string.
    mappath : string
        Optional root map path, enabling relative paths in mapfile.

    Example
    =======
    >>> mo = fromstring("MAP\nNAME 'test'\nEND")
    >>> mo.name
    'test'
    """
    import re
    if re.search("^\s*MAP", data, re.I): 
        return msLoadMapFromString(data, mappath)
    elif re.search("^\s*LAYER", data, re.I):
        ob = layerObj()
        ob.updateFromString(data)
        return ob
    elif re.search("^\s*CLASS", data, re.I):
        ob = classObj()
        ob.updateFromString(data)
        return ob
    elif re.search("^\s*STYLE", data, re.I):
        ob = styleObj()
        ob.updateFromString(data)
        return ob
    else:
        raise ValueError, "No map, layer, class, or style found. Can not load from provided string"
}

/* ===========================================================================
   Python rectObj extensions
   ======================================================================== */

%extend pointObj {

%pythoncode {

    def __str__(self):
        return self.toString()

}
    
}


/* ===========================================================================
   Python rectObj extensions
   ======================================================================== */
   
%extend rectObj {

%pythoncode {

    def __str__(self):
        return self.toString()
        
    def __contains__(self, item):
        item_type = str(type(item))
        if item_type == "<class 'mapscript.pointObj'>":
            if item.x >= self.minx and item.x <= self.maxx \
            and item.y >= self.miny and item.y <= self.maxy:
                return True
            else:
                return False
        else:
            raise TypeError, \
                '__contains__ does not yet handle %s' % (item_type)
        
}

}

/******************************************************************************
 * Extensions to mapObj
 *****************************************************************************/

%extend mapObj {
  
    /* getLayerOrder() extension returns the map layerorder as a native
     sequence */

    PyObject *getLayerOrder() {
        int i;
        PyObject *order;
        order = PyTuple_New(self->numlayers);
        for (i = 0; i < self->numlayers; i++) {
            PyTuple_SetItem(order,i,PyInt_FromLong((long)self->layerorder[i]));
        }
        return order;
    } 

    int setLayerOrder(PyObject *order) {
        int i, size;
        size = PyTuple_Size(order);
        for (i = 0; i < size; i++) {
            self->layerorder[i] = (int)PyInt_AsLong(PyTuple_GetItem(order, i));
        }
        return MS_SUCCESS;
    }
    
    PyObject* getSize()
    {
        PyObject* output ;
        output = PyTuple_New(2);
        PyTuple_SetItem(output,0,PyInt_FromLong((long)self->width));
        PyTuple_SetItem(output,1,PyInt_FromLong((long)self->height));
        return output;
    }    
%pythoncode {

    def get_height(self):
        return self.getSize()[1] # <-- second member is the height
    def get_width(self):
        return self.getSize()[0] # <-- first member is the width
    def set_height(self, value):
        return self.setSize(self.getSize()[0], value)
    def set_width(self, value):
        return self.setSize(value, self.getSize()[1])
    width = property(get_width, set_width)
    height = property(get_height, set_height)
    
}    
    
}

/****************************************************************************
 * Support for bridging Python file-like objects and GD through IOCtx
 ***************************************************************************/

/******************************************************************************
 * Extensions to imageObj
 *****************************************************************************/
    
%extend imageObj {

    /* New imageObj constructor taking an optional PyFile-ish object
     * argument.
     *
     * Furthermore, we are defaulting width and height so that in case
     * anyone wants to swig mapscript with the -keyword option, they can
     * omit width and height.  Work done as part of Bugzilla issue 550. */

    imageObj(PyObject *arg1=Py_None, PyObject *arg2=Py_None, 
             PyObject *input_format=Py_None, PyObject *input_resolution=Py_None, PyObject *input_defresolution=Py_None)
    {
#ifdef FORCE_BROKEN_GD_CODE
        imageObj *image=NULL;
        outputFormatObj *format=NULL;
        int width;
        int height;
        double resolution, defresolution;
        PyObject *pybytes;
        rendererVTableObj *renderer = NULL;
        rasterBufferObj *rb = NULL;
      
        unsigned char PNGsig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
        unsigned char JPEGsig[3] = {255, 216, 255};

        resolution = defresolution = MS_DEFAULT_RESOLUTION;

        if ((PyInt_Check(arg1) && PyInt_Check(arg2)) || PyString_Check(arg1))
        {
            if (input_format == Py_None) {
                format = msCreateDefaultOutputFormat(NULL, "GD/GIF", "gdgif");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/PNG", "gdpng");

                if (format)
                  msInitializeRendererVTable(format);
            }
            else if (PyString_Check(input_format)) {
                format = msCreateDefaultOutputFormat(NULL, 
                                                     PyString_AsString(input_format),
                                                     NULL);
            }
            else {
                if ((SWIG_ConvertPtr(input_format, (void **) &format,
                                     SWIGTYPE_p_outputFormatObj,
                                     SWIG_POINTER_EXCEPTION | 0 )) == -1) 
                {
                    msSetError(MS_IMGERR, "Can't convert format pointer",
                               "imageObj()");
                    return NULL;
                }
            }
        
            if (format == NULL) {
                msSetError(MS_IMGERR, "Could not create output format",
                           "imageObj()");
                return NULL;
            }
        }

        if (PyFloat_Check(input_resolution))
            resolution = PyFloat_AsDouble(input_resolution);
        if (PyFloat_Check(input_defresolution))
            defresolution = PyFloat_AsDouble(input_defresolution);

        if (PyInt_Check(arg1) && PyInt_Check(arg2)) 
        {
            /* Create from width, height, format/driver */
            width = (int) PyInt_AsLong(arg1);
            height = (int) PyInt_AsLong(arg2);

            image = msImageCreate(width, height, format, NULL, NULL, resolution, defresolution, NULL);
            return image;
        }
        
        /* Is arg1 a filename? */
        else if (PyString_Check(arg1)) 
        {
            renderer = format->vtable;
            rb = (rasterBufferObj*)calloc(1,sizeof(rasterBufferObj));

            if (!rb) {
                msSetError(MS_MEMERR, NULL, "imageObj()");
                return NULL;
            }

            if ( (renderer->loadImageFromFile(PyString_AsString(arg1), rb)) == MS_FAILURE)
                return NULL;

            image = msImageCreate(rb->width, rb->height, format, NULL, NULL, 
                                  resolution, defresolution, NULL);
            renderer->mergeRasterBuffer(image, rb, 1.0, 0, 0, 0, 0, rb->width, rb->height);

            msFreeRasterBuffer(rb);
            free(rb);

            return image;
        }
        
        /* Is a file-like object */
        else if (arg1 != Py_None)
        {

            if (PyObject_HasAttrString(arg1, "seek"))
            {
                /* Detect image format */
                pybytes = PyObject_CallMethod(arg1, "read", "i", 8);
                PyObject_CallMethod(arg1, "seek", "i", 0);
            
                if (memcmp(PyString_AsString(pybytes),"GIF8",4)==0) 
                {
%#ifdef USE_GD_GIF
                    image = createImageObjFromPyFile(arg1, "GD/GIF");
%#else
                    msSetError(MS_MISCERR, "Unable to load GIF image.",
                               "imageObj()");
%#endif
                }
                else if (memcmp(PyString_AsString(pybytes),PNGsig,8)==0) 
                {
%#ifdef USE_GD_PNG
                    image = createImageObjFromPyFile(arg1, "GD/PNG");
%#else
                    msSetError(MS_MISCERR, "Unable to load PNG image.",
                               "imageObj()");
%#endif
                }
                else if (memcmp(PyString_AsString(pybytes),JPEGsig,3)==0) 
                {
%#ifdef USE_GD_JPEG
                    image = createImageObjFromPyFile(arg1, "GD/JPEG");
%#else
                    msSetError(MS_MISCERR, "Unable to load JPEG image.", 
                               "imageObj()");
%#endif
                }
                else
                {
                    msSetError(MS_MISCERR, "Failed to detect image format.  Likely cause is invalid image or improper filemode.  On windows, Python files should be opened in 'rb' mode.", "imageObj()");
                }

                return image;
            
            }
            else /* such as a url handle */
            {
                /* If there is no seek method, we absolutely must
                   have a driver name */
                if (!PyString_Check(arg2))
                {
                    msSetError(MS_MISCERR, "A driver name absolutely must accompany file objects which do not have a seek() method", "imageObj()");
                    return NULL;
                }    
                return (imageObj *) createImageObjFromPyFile(arg1, 
                        PyString_AsString(arg2));
            }
        }
        else 
        {
            msSetError(MS_IMGERR, "Failed to create image", 
                       "imageObj()");
            return NULL;
        }
#else
         msSetError(MS_IMGERR, "imageObj() is severely broken and should not be used","imageObj()");
         return NULL;
#endif
    }
  
    /* ======================================================================
       write()

       Write image data to an open Python file or file-like object.
       Overrides extension method in mapscript/swiginc/image.i.
       Intended to replace saveToString.
    ====================================================================== */
    int write( PyObject *file=Py_None )
    {
        unsigned char *imgbuffer=NULL;
        int imgsize;
        PyObject *noerr;
        int retval=MS_FAILURE;
        rendererVTableObj *renderer = NULL;

        /* Return immediately if image driver is not GD */
        if ( !MS_RENDERER_PLUGIN(self->format) )
        {
            msSetError(MS_IMGERR, "Writing of %s format not implemented",
                       "imageObj::write", self->format->driver);
            return MS_FAILURE;
        }

        if (file == Py_None) /* write to stdout */
            retval = msSaveImage(NULL, self, NULL);

        else if (PyFile_Check(file)) /* a Python (C) file */
        {
            renderer = self->format->vtable;
            /* FIXME? as an improvement, pass a map argument instead of the NULL (see #4216) */
            retval = renderer->saveImage(self, NULL, PyFile_AsFile(file), self->format);
        }
        else /* presume a Python file-like object */
        {
            imgbuffer = msSaveImageBuffer(self, &imgsize,
                                          self->format);
            if (imgsize == 0)
            {
                msSetError(MS_IMGERR, "failed to get image buffer", "write()");
                return MS_FAILURE;
            }
                
            noerr = PyObject_CallMethod(file, "write", "s#", imgbuffer,
                                        imgsize);
            free(imgbuffer);
            if (noerr == NULL)
                return MS_FAILURE;
            else
                Py_DECREF(noerr);
            retval = MS_SUCCESS;
        }

        return retval;
    }

    /* Deprecated */  
    PyObject *saveToString() {
        int size=0;
        unsigned char *imgbytes;
        PyObject *imgstring; 

        imgbytes = msSaveImageBuffer(self, &size, self->format);
        if (size == 0)
        {
            msSetError(MS_IMGERR, "failed to get image buffer", "saveToString()");
            return NULL;
        }
        imgstring = PyString_FromStringAndSize((const char*) imgbytes, size); 
        free(imgbytes);
        return imgstring;
    }

}


/******************************************************************************
 * Extensions to styleObj
 *****************************************************************************/

%extend styleObj {

    void pattern_set(int nListSize, double* pListValues)
    {
        if( nListSize < 2 )
        {
            msSetError(MS_SYMERR, "Not enough pattern elements. A minimum of 2 are required", "pattern_set()");
            return;
        }
        if( nListSize > MS_MAXPATTERNLENGTH )
        {
            msSetError(MS_MISCERR, "Too many elements", "pattern_set()");
            return;
        }
        memcpy( self->pattern, pListValues, sizeof(double) * nListSize);
        self->patternlength = nListSize;
    }

    void pattern_get(double** argout, int* pnListSize)
    {
        *pnListSize = self->patternlength;
        *argout = (double*) malloc(sizeof(double) * *pnListSize);
        memcpy( *argout, self->pattern, sizeof(double) * *pnListSize);
    }

    void patternlength_set2(int patternlength)
    {
        msSetError(MS_MISCERR, "pattern is read-only", "patternlength_set()");
    }

%pythoncode {

    __swig_setmethods__["patternlength"] = _mapscript.styleObj_patternlength_set2
    __swig_getmethods__["patternlength"] = _mapscript.styleObj_patternlength_get
    if _newclass:patternlength = _swig_property(_mapscript.styleObj_patternlength_get, _mapscript.styleObj_patternlength_set2)

    __swig_setmethods__["pattern"] = _mapscript.styleObj_pattern_set
    __swig_getmethods__["pattern"] = _mapscript.styleObj_pattern_get
    if _newclass:pattern = _swig_property(_mapscript.styleObj_pattern_get, _mapscript.styleObj_pattern_set)
}

}

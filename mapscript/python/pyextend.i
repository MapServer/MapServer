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

/* ===========================================================================
   Python rectObj extensions
   ======================================================================== */

%extend pointObj {

    %newobject __str__;
    char *__str__() {
        char buffer[256];
        char fmt[]="{ 'x': %f , 'y': %f }";
        msPointToFormattedString(self, (char *) &fmt, (char *) &buffer, 256);
        return strdup(buffer);
    }
    
}


/* ===========================================================================
   Python rectObj extensions
   ======================================================================== */
   
%extend rectObj {

    %newobject __str__;
    char *__str__() {
        char buffer[256];
        char fmt[]="{ 'minx': %f , 'miny': %f , 'maxx': %f , 'maxy': %f }";
        msRectToFormattedString(self, (char *) &fmt, (char *) &buffer, 256);
        return strdup(buffer);
    }
   
// Extensions in Python
%pythoncode {

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
  
    // getLayerOrder() extension returns the map layerorder as a native
    // sequence

    PyObject *getLayerOrder() {
        int i;
        PyObject *order;
        order = PyTuple_New(self->numlayers);
        for (i = 0; i < self->numlayers; i++) {
            PyTuple_SetItem(order,i,PyInt_FromLong((long)self->layerorder[i]));
        }
        return order;
    } 

    // setLayerOrder() extension
    
    int setLayerOrder(PyObject *order) {
        int i, size;
        size = PyTuple_Size(order);
        for (i = 0; i < size; i++) {
            self->layerorder[i] = (int)PyInt_AsLong(PyTuple_GetItem(order, i));
        }
        return MS_SUCCESS;
    }
    
    // getSize() extension
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

    imageObj(int width, int height, const char *driver=NULL,
             PyObject *file=Py_None, mapObj *map=NULL)
    {
        imageObj *image=NULL;
        outputFormatObj *format;

        if (file == Py_None) {
            if (driver) {
                format = msCreateDefaultOutputFormat(NULL, driver);
            }
            else {
                format = msCreateDefaultOutputFormat(NULL, "GD/GIF");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/PNG");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/JPEG");
                if (format == NULL)
                    format = msCreateDefaultOutputFormat(NULL, "GD/WBMP");
            }
            if (format == NULL) {
                msSetError(MS_IMGERR, "Could not create output format %s",
                           "imageObj()", driver);
                return NULL;
            }
            image = msImageCreate(width, height, format, NULL, NULL, map);
            return image;
        }
        // Is file a filename?
        else if (PyString_Check(file)) {
            return (imageObj *) msImageLoadGD((char *) PyString_AsString(file));
        }
        // Is a file-like object
        else if (file && driver) {
            return (imageObj *) createImageObjFromPyFile(width, height, file, driver);
        }
        else {
            msSetError(MS_IMGERR, "Failed to create image (%s, %s)", "imageObj()", file, driver);
            return NULL;
        }
    }
  
    /* ======================================================================
       write()

       Write image data to an open Python file or file-like object.
       Overrides extension method in mapscript/swiginc/image.i.
       Intended to replace saveToString.
    ====================================================================== */
    int write( PyObject *file=Py_None )
    {
        FILE *stream;
        gdIOCtx *ctx;
        unsigned char *imgbuffer;
        int imgsize;
        PyObject *noerr;
        int retval=MS_FAILURE;
       
        /* Return immediately if image driver is not GD */
        if ( !MS_DRIVER_GD(self->format) )
        {
            msSetError(MS_IMGERR, "Writing of %s format not implemented",
                       "imageObj::write", self->format->driver);
            return MS_FAILURE;
        }

        if (file == Py_None) /* write to stdout */
        {
            //if ( msIO_needBinaryStdout() == MS_FAILURE )
            //    return MS_FAILURE;
            ctx = gdNewFileCtx(stdout);
            retval = msSaveImageGDCtx(self->img.gd, ctx, self->format);
            ctx->gd_free(ctx);
        }
        else if (PyFile_Check(file)) /* a Python (C) file */
        {
            stream = PyFile_AsFile(file);
            ctx = gdNewFileCtx(stream);
            retval = msSaveImageGDCtx(self->img.gd, ctx, self->format);
            ctx->gd_free(ctx);
        }
        else /* presume a Python file-like object */
        {
            imgbuffer = msSaveImageBufferGD(self->img.gd, &imgsize,
                                            self->format);
            if (imgsize == 0)
            {
                msSetError(MS_IMGERR, "failed to get image buffer", "write()");
                return MS_FAILURE;
            }
                
            noerr = PyObject_CallMethod(file, "write", "s#", imgbuffer,
                                        imgsize);
            gdFree(imgbuffer);
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

        imgbytes = msSaveImageBufferGD(self->img.gd, &size, self->format);
        if (size == 0)
        {
            msSetError(MS_IMGERR, "failed to get image buffer", "saveToString()");
            return NULL;
        }
        imgstring = PyString_FromStringAndSize(imgbytes, size); 
        gdFree(imgbytes);
        return imgstring;
    }

}



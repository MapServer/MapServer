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

    char *__str__() {
        char buffer[256];
        char fmt[]="{ 'minx': %f , 'miny': %f , 'maxx': %f }";
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

/* ===========================================================================
   Python mapserver container iterator class
   ======================================================================== */
   
%pythoncode {

class LineIterator:
    
    def __init__(self, line):
        self.current = 0
        self.line = line
        self.limit = line.numpoints - 1
        
    def __iter__(self):
        return self

    def next(self):
        if self.current <= self.limit:
            p = None
            try:
                p = self.line.getPoint(self.current)
            except AttributeError:
                p = self.line.get(self.current)
            self.current = self.current + 1
            return p
        else:
            raise StopIteration
        
}


/* ===========================================================================
   Python lineObj extensions
   ======================================================================== */
   
%extend lineObj 
{

%pythoncode {

    def __iter__(self):
        return LineIterator(self)
    
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
   
    PyObject *saveToString() {
        unsigned char *imgbytes;
        int size;
        PyObject *imgstring; 

%#if GD2_VERS > 1
        if (self->format->imagemode == MS_IMAGEMODE_RGBA) {
            gdImageSaveAlpha(self->img.gd, 1);
        }
        else if (self->format->imagemode == MS_IMAGEMODE_RGB) {
            gdImageSaveAlpha(self->img.gd, 0);
        }
%#endif 

        if (strcasecmp("ON", 
            msGetOutputFormatOption(self->format, "INTERLACE", "ON" )) == 0) {
            
            gdImageInterlace(self->img.gd, 1);
        }

        if (self->format->transparent) {
            gdImageColorTransparent(self->img.gd, 0);
        }

        if (strcasecmp(self->format->driver, "gd/gif") == 0) {
%#ifdef USE_GD_GIF
            imgbytes = gdImageGifPtr(self->img.gd, &size);
%#else
            msSetError(MS_IMGERR, "GIF output is not available.", 
                       "saveToString()");
            return NULL;
%#endif
        } else if (strcasecmp(self->format->driver, "gd/png") == 0) {
%#ifdef USE_GD_PNG
            imgbytes = gdImagePngPtr(self->img.gd, &size);
%#else
            msSetError(MS_IMGERR, "PNG output is not available.", 
                       "saveToString()");
            return NULL;
%#endif
        } else if (strcasecmp(self->format->driver, "gd/jpeg") == 0) {
%#ifdef USE_GD_JPEG
            imgbytes = gdImageJpegPtr(self->img.gd, &size, 
                atoi(msGetOutputFormatOption(self->format, "QUALITY", "75" )));
%#else
            msSetError(MS_IMGERR, "JPEG output is not available.", 
                       "saveToString()");
            return NULL;
%#endif
        } else if (strcasecmp(self->format->driver, "gd/wbmp") == 0) {
%#ifdef USE_GD_WBMP
            imgbytes = gdImageWBMPPtr(self->img.gd, &size, 1);
%#else
            msSetError(MS_IMGERR, "WBMP output is not available.", 
                      "saveToString()");
            return NULL;
%#endif
        } else {
            msSetError(MS_IMGERR, "Unknown output image type driver: %s.", 
                       "saveToString()", self->format->driver );
            return NULL;
        } 
        
        imgstring = PyString_FromStringAndSize(imgbytes, size); 
        gdFree(imgbytes);
        return imgstring;
    }

}

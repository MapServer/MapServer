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

    // get a list of font alias/filename tuples from the map fontset

    PyObject *getFonts() {
        int i;
        PyObject *PyFonts, *PyTemp;
        struct hashObj *tp=NULL;
        
        PyFonts = PyList_New(0);

        for (i=0; i<MS_HASHSIZE; i++) {
            tp = self->fontset.fonts[i];
            if (tp != NULL) {
                PyTemp = PyTuple_New(2);
                PyTuple_SetItem(PyTemp, 0, PyString_FromString(tp->key));
                PyTuple_SetItem(PyTemp, 1, PyString_FromString(tp->data));
                PyList_Append(PyFonts, PyTemp);
            }
        }
        return PyFonts;
    }
            
}

/******************************************************************************
 * Extensions to imageObj
 *****************************************************************************/

%extend imageObj {

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
            return(MS_FAILURE);
%#endif
        } else if (strcasecmp(self->format->driver, "gd/png") == 0) {
%#ifdef USE_GD_PNG
            imgbytes = gdImagePngPtr(self->img.gd, &size);
%#else
            msSetError(MS_IMGERR, "PNG output is not available.", 
                       "saveToString()");
            return(MS_FAILURE);
%#endif
        } else if (strcasecmp(self->format->driver, "gd/jpeg") == 0) {
%#ifdef USE_GD_JPEG
            imgbytes = gdImageJpegPtr(self->img.gd, &size, 
                atoi(msGetOutputFormatOption(self->format, "QUALITY", "75" )));
%#else
            msSetError(MS_IMGERR, "JPEG output is not available.", 
                       "saveToString()");
            return(MS_FAILURE);
%#endif
        } else if (strcasecmp(self->format->driver, "gd/wbmp") == 0) {
%#ifdef USE_GD_WBMP
            imgbytes = gdImageWBMPPtr(self->img.gd, &size, 1);
%#else
            msSetError(MS_IMGERR, "WBMP output is not available.", 
                      "saveToString()");
            return(MS_FAILURE);
%#endif
        } else {
            msSetError(MS_IMGERR, "Unknown output image type driver: %s.", 
                       "saveToString()", self->format->driver );
            return(MS_FAILURE);
        } 
        
        imgstring = PyString_FromStringAndSize(imgbytes, size); 
        gdFree(imgbytes);
        return imgstring;
    }

}

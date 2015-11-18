/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript imageObj extensions
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
   =========================================================================*/

%extend imageObj {
   
    /* imageObj constructor now takes filename as an optional argument.
     * If the target language is Python, we ignore this constructor and
     * instead use the one in python/pymodule.i. */
#ifndef SWIGPYTHON
    imageObj(int width, int height, outputFormatObj *input_format=NULL,
             const char *file=NULL,
             double resolution=MS_DEFAULT_RESOLUTION, double defresolution=MS_DEFAULT_RESOLUTION)
    {
        imageObj *image=NULL;
        outputFormatObj *format;
        rendererVTableObj *renderer = NULL;
        rasterBufferObj *rb = NULL;

        if (input_format) {
            format = input_format;
        }
        else {
            format = msCreateDefaultOutputFormat(NULL, "AGG/PNG", "aggpng");
            if (format)
                msInitializeRendererVTable(format);
        }
        if (format == NULL) {
            msSetError(MS_IMGERR, "Could not create output format",
                       "imageObj()");
            return NULL;
        }

        if (file) {
            
            renderer = format->vtable;
            rb = (rasterBufferObj*) malloc(sizeof(rasterBufferObj));
            if (!rb) {
                msSetError(MS_MEMERR, NULL, "imageObj()");
                return NULL;
            }
            if ( (renderer->loadImageFromFile((char *)file, rb)) == MS_FAILURE)
                return NULL;

            image = msImageCreate(rb->width, rb->height, format, NULL, NULL, 
                                  resolution, defresolution, NULL);
            renderer->mergeRasterBuffer(image, rb, 1.0, 0, 0, 0, 0, rb->width, rb->height);

            msFreeRasterBuffer(rb);
            free(rb);

            return image;
        }

        image = msImageCreate(width, height, format, NULL, NULL, resolution, defresolution, NULL);
        return image;
    }
#endif

    ~imageObj() 
    {
        msFreeImage(self);    
    }

    /* saveGeo - see Bugzilla issue 549 */ 
    void save(char *filename, mapObj *map=NULL) 
    {
        msSaveImage(map, self, filename );
    }

    /* ======================================================================
       write()

       Write image data to an open file handle.  Intended to replace
       saveToString.  See python/pyextend.i for the Python specific
       version of this method.
    ====================================================================== */
#ifndef SWIGPYTHON
    int write( FILE *file=NULL )
    {
        int retval=MS_FAILURE;
        rendererVTableObj *renderer = NULL;

        if (MS_RENDERER_PLUGIN(self->format))
        {
            if (file)
            {
                renderer = self->format->vtable;
                /* FIXME? as an improvement, pass a map argument instead of the NULL (see #4216) */
                retval = renderer->saveImage(self, NULL, file, self->format);
            }
            else
            {
                retval = msSaveImage(NULL, self, NULL);
            }
        }
        else
        {
            msSetError(MS_IMGERR, "Writing of %s format not implemented",
                       "imageObj::write");
        }

        return retval;
    }
#endif

    /*
    -------------------------------------------------------------------------
    getBytes returns a gdBuffer structure (defined in mapscript.i) which must
    be typemapped to an object appropriate to the target language.  This
    typemap must also msFree the data member of the gdBuffer.  See the type-
    maps in java/javamodule.i and python/pymodule.i for examples.

    contributed by Jerry Pisk, jerry.pisk@gmail.com
    -------------------------------------------------------------------------
    */
    
    gdBuffer getBytes() 
    {
        gdBuffer buffer;
        
        buffer.owns_data = MS_TRUE;
        
        buffer.data = msSaveImageBuffer(self, &buffer.size, self->format);
            
        if( buffer.data == NULL || buffer.size == 0 )
        {
            buffer.data = NULL;
            msSetError(MS_MISCERR, "Failed to get image buffer", "getBytes");
            return buffer;
        }

        return buffer;
    }

    int getSize() {
        gdBuffer buffer;
	int size=0;
        
        buffer.data = msSaveImageBuffer(self, &buffer.size, self->format);
	size = buffer.size;
            
        if( buffer.data == NULL || buffer.size == 0 ) {
            buffer.data = NULL;
            msSetError(MS_MISCERR, "Failed to get image buffer size", "getSize");
        }
	free(buffer.data);
        return size;
    }
}


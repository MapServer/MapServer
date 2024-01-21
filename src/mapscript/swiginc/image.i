/* ===========================================================================
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

  /**
  Create a new :class:`imageObj` instance. If *filename* is specified, an imageObj
  is created from the file and any specified *width*, *height*, and *format* parameters 
  will be overridden by values of the image in *filename*.  Otherwise, if *format* is specified (as an :class:`outputFormatObj`) an imageObj is created
  using that format. If *filename* is not specified, then *width* and *height* should be specified. 
  The default resolution is currently 72 and defined by :data:`MS_DEFAULT_RESOLUTION` - this setting is 
  not available in MapScript. 
  */
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
            format = msCreateDefaultOutputFormat(NULL, "AGG/PNG", "aggpng", NULL);
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
            if ( (renderer->loadImageFromFile((char *)file, rb)) == MS_FAILURE) {
                msFreeRasterBuffer(rb);
                free(rb);
                return NULL;
            }
            image = msImageCreate(rb->width, rb->height, format, NULL, NULL,
                                  resolution, defresolution, NULL);
            if (! image) {
                msFreeRasterBuffer(rb);
                free(rb);
                return NULL;
            }

            if(renderer->mergeRasterBuffer(image, rb, 1.0, 0, 0, 0, 0, rb->width, rb->height) != MS_SUCCESS) {
                msFreeImage(image);
                image = NULL;
            }

            msFreeRasterBuffer(rb);
            free(rb);

            return image;
        }

        image = msImageCreate(width, height, format, NULL, NULL, resolution, defresolution, NULL);
        return image;
    }

    ~imageObj()
    {
        msFreeImage(self);
    }

    /// Save image to filename. The optional map parameter must be specified if 
    /// saving GeoTIFF images.
    void save(char *filename, mapObj *map=NULL)
    {
        msSaveImage(map, self, filename );
    }

#ifndef SWIGPYTHON
    /**
    Write image data to an open file handle. Intended to replace
    saveToString.  See ``python/pyextend.i`` for the Python specific
    version of this method.
    */
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
                       "imageObj::write",
                       self->format->name);
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

    /// Returns the image contents as a binary buffer. The exact form of this buffer will 
    /// vary by MapScript language (e.g. a string in Python, byte[] array in Java and C#, unhandled in Perl)
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

    /**
    Returns the size of the binary buffer representing the image buffer

    .. note:: 

        The getSize method is inefficient as it does a call to getBytes and 
        then computes the size of the byte array. The byte array is then immediately discarded. 
        In most cases it is more efficient to call getBytes directly.
    */
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

    /**
    Pastes another imageObj on top of this imageObj.
    If optional dstx,dsty are provided then they define the position where the
    image should be copied (dstx,dsty = top-left corner position).
    */
    int pasteImage(imageObj *imageSrc, double opacity=1.0, int dstx=0, int dsty=0)
    {
        rendererVTableObj *renderer = NULL;
        rasterBufferObj rb;

        if (!MS_RENDERER_PLUGIN(self->format)) {
            msSetError(MS_IMGERR, "PasteImage function should only be used with renderer plugin drivers.",
                       "imageObj::pasteImage");
            return MS_FAILURE;
        }

        memset(&rb,0,sizeof(rasterBufferObj));

        renderer = self->format->vtable;
        if(MS_SUCCESS != renderer->getRasterBufferHandle(imageSrc, &rb)) {
            msSetError(MS_IMGERR, "PasteImage failed to extract rasterbuffer handle",
                       "imageObj::pasteImage");
            return MS_FAILURE;
        }

        if(MS_SUCCESS != renderer->mergeRasterBuffer(self, &rb, opacity, 0, 0, dstx, dsty, rb.width, rb.height)) {
            msSetError(MS_IMGERR, "PasteImage failed to merge raster buffer",
                       "imageObj::pasteImage");
            return MS_FAILURE;
        }

        return MS_SUCCESS;

    }

    /**
    Writes the image to temp directory.
    Returns the image URL.
    */
    char *saveWebImage()
    {
        char *imageFile = NULL;
        char *imageFilename = NULL;
        char path[MS_MAXPATHLEN];
        char *imageUrlFull = NULL;

        imageFilename = msTmpFilename(self->format->extension);
        imageFile = msBuildPath(path, self->imagepath, imageFilename);

        if (msSaveImage(NULL, self, imageFile) != MS_SUCCESS) {
            msSetError(MS_IMGERR, "Failed writing image to %s",
                       "imageObj::saveWebImage",
                       imageFile);
            msFree(imageFilename);
            return NULL;
        }

        imageUrlFull = msStrdup(msBuildPath(path, self->imageurl, imageFilename));
        msFree(imageFile);        
        msFree(imageFilename);
        return imageUrlFull;
    }
}

/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript symbolObj extensions
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

/// See also https://github.com/mapserver/mapserver/issues/579

%extend symbolObj 
{

    /// Create new default :class:`symbolObj` named ``symbolname``. If ``imagefile`` is specified, then the symbol 
    /// will be of type :data:`MS_SYMBOL_PIXMAP`. 
    symbolObj(char *symbolname, const char *imagefile=NULL) 
    {
        symbolObj *symbol;
        symbol = (symbolObj *) malloc(sizeof(symbolObj));
        initSymbol(symbol);
        symbol->name = msStrdup(symbolname);
        if (imagefile) {
            msLoadImageSymbol(symbol, imagefile);
        }
        return symbol;
    }

    ~symbolObj() 
    {
        if (self) {
            if (msFreeSymbol(self)==MS_SUCCESS) {
                free(self);
                self=NULL;
            }
        }
    }

    /// Sets the ``imagefile`` path for a :data:`MS_SYMBOL_PIXMAP`. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int setImagepath(const char *imagefile) {
        return msLoadImageSymbol(self, imagefile);
    }

    /// Sets the symbol points from the points of line. Returns the updated number of points. 
    int setPoints(lineObj *line) {
        int i;
    self->sizex = 0;
    self->sizey = 0;
        for (i=0; i<line->numpoints; i++) {
            MS_COPYPOINT(&(self->points[i]), &(line->point[i]));
        self->sizex = MS_MAX(self->sizex, self->points[i].x);
        self->sizey = MS_MAX(self->sizey, self->points[i].y);
        }
        self->numpoints = line->numpoints;
        return self->numpoints;
    }

    %newobject getPoints;
    /// Returns the symbol points as a :class:`lineObj`. 
    lineObj *getPoints() 
    {
        int i;
        lineObj *line;
        line = (lineObj *) malloc(sizeof(lineObj));
        line->point = (pointObj *) malloc(sizeof(pointObj)*(self->numpoints));
        for (i=0; i<self->numpoints; i++) {
            line->point[i].x = self->points[i].x;
            line->point[i].y = self->points[i].y;
        }
        line->numpoints = self->numpoints;
        return line;
    }

    %newobject getImage;
    /// Returns a pixmap symbol's imagery as an :class:`imageObj`. 
    imageObj *getImage(outputFormatObj *input_format)
    {
        imageObj *image = NULL;
        outputFormatObj *format = NULL;
        rendererVTableObj *renderer = NULL;

        if (input_format)
        {
            format = input_format;
        }
        else 
        {
            format = msCreateDefaultOutputFormat(NULL, "AGG/PNG", "aggpng", NULL);
            if (format)
                msInitializeRendererVTable(format);
        }
        
        if (format == NULL) 
        {
            msSetError(MS_IMGERR, "Could not create output format",
                       "getImage()");
            return NULL;
        }

        renderer = format->vtable;
        msPreloadImageSymbol(renderer, self);
        if (self->pixmap_buffer) 
        {
            image = msImageCreate(self->pixmap_buffer->width, self->pixmap_buffer->height, format, NULL, NULL,
                                  MS_DEFAULT_RESOLUTION, MS_DEFAULT_RESOLUTION, NULL);
            if(!image) {
              msSetError(MS_IMGERR, "Could not create image",
                       "getImage()");
              return NULL;
            }
            if(MS_SUCCESS != renderer->mergeRasterBuffer(image, self->pixmap_buffer, 1.0, 0, 0, 0, 0,
                                        self->pixmap_buffer->width, self->pixmap_buffer->height)) {
              msSetError(MS_IMGERR, "Could not merge symbol image",
                       "getImage()");
              msFreeImage(image);
              return NULL;
            }
        }

        return image;
    }

    /// Set a pixmap symbol's imagery from image. 
    /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int setImage(imageObj *image)
    {
        rendererVTableObj *renderer = NULL;
        
        renderer = image->format->vtable;
        
        if (self->pixmap_buffer) {
            msFreeRasterBuffer(self->pixmap_buffer);
            free(self->pixmap_buffer);
        }
        
        self->pixmap_buffer = (rasterBufferObj*)malloc(sizeof(rasterBufferObj));
        if (!self->pixmap_buffer) {
            msSetError(MS_MEMERR, NULL, "setImage()");
            return MS_FAILURE;
        }
        self->type = MS_SYMBOL_PIXMAP;
        return renderer->getRasterBufferCopy(image, self->pixmap_buffer);
    }
}

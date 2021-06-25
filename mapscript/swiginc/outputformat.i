/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript outputFormatObj extensions
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

%extend outputFormatObj 
{
    /// Create new instance. If name is not provided, the value of driver is used as a name.
    outputFormatObj(const char *driver, char *name=NULL) 
    {
        outputFormatObj *format;

        format = msCreateDefaultOutputFormat(NULL, driver, name, NULL);

        /* in the case of unsupported formats, msCreateDefaultOutputFormat
           should return NULL */
        if (!format)
        {
            msSetError(MS_MISCERR, "Unsupported format driver: %s",
                       "outputFormatObj()", driver);
            return NULL;
        }
        
        msInitializeRendererVTable(format);

        MS_REFCNT_INIT(format);
        format->inmapfile = MS_TRUE;

        return format;
    }

    ~outputFormatObj() 
    {
        msFreeOutputFormat( self );
    }

    /**
    * Set file extension for output format such as  'png' or 'jpg'. 
    * Method could probably be deprecated since the extension attribute is mutable.
    * Not in Java extension.
    */
#ifndef SWIGJAVA
    void setExtension( const char *extension ) 
    {
        msFree( self->extension );
        self->extension = msStrdup(extension);
    }

    /**
    * Set mimetype for output format such as ``image/png`` or ``image/jpeg``.
    * Method could probably be deprecated since the mimetype attribute is mutable.
    * Not in Java extension
    */
    void setMimetype( const char *mimetype ) 
    {
        msFree( self->mimetype );
        self->mimetype = msStrdup(mimetype);
    }
#endif

    /// Set the format option at ``key`` to ``value``. Format options are mostly driver specific.
    void setOption( const char *key, const char *value ) 
    {
        msSetOutputFormatOption( self, key, value );
    }

    /// Checks some internal consistency issues, and returns :data:`MS_TRUE` 
    /// if things are OK and :data:`MS_FALSE` if there are problems. 
    /// Some problems are fixed up internally. May produce debug output if issues encountered.
    int validate() 
    {
        return msOutputFormatValidate(self, MS_FALSE );
    }

    %newobject getOption;
    /// Return the format option at ``key`` or ``defaultvalue`` 
    /// if key is not a valid hash index.
    char *getOption(const char *key, const char *value="") 
    {
        return msStrdup(msGetOutputFormatOption(self, key, value));
    }

    %newobject getOptionAt;
    /**
    * Returns the option at ``idx`` or NULL if the index is beyond the array bounds. 
    * The option is returned as the original KEY=VALUE string. 
    * The number of available options can be obtained via :attr:`outputFormatObj.numformatoptions`
    */
    char* getOptionAt(int i) {
       if( i >= 0 && i < self->numformatoptions ) {
          return msStrdup(self->formatoptions[i]);
       }
       return NULL;
    }

    /// Set the device property of the output format
    void attachDevice( void *device ) 
    {
        self->device = device;
    }
    
}

/* ===========================================================================
   $Id$
 
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
    
    outputFormatObj(const char *driver, char *name=NULL) 
    {
        outputFormatObj *format;

        format = msCreateDefaultOutputFormat(NULL, driver);

        /* in the case of unsupported formats, msCreateDefaultOutputFormat
           should return NULL */
        if (!format)
        {
            msSetError(MS_MISCERR, "Unsupported format driver: %s",
                       "outputFormatObj()", driver);
            return NULL;
        }
        
        /* Else, continue */
        format->refcount++;
	format->inmapfile = MS_TRUE;

        if (name != NULL)
        {
            free(format->name);
            format->name = strdup(name);
        }
        return format;
    }

    ~outputFormatObj() 
    {
        if ( --self->refcount < 1 )
            msFreeOutputFormat( self );
    }

#ifndef SWIGJAVA
    void setExtension( const char *extension ) 
    {
        msFree( self->extension );
        self->extension = strdup(extension);
    }

    void setMimetype( const char *mimetype ) 
    {
        msFree( self->mimetype );
        self->mimetype = strdup(mimetype);
    }
#endif

    void setOption( const char *key, const char *value ) 
    {
        msSetOutputFormatOption( self, key, value );
    }

    int validate() 
    {
       	return msOutputFormatValidate( self );
    }

    %newobject getOption;
    char *getOption(const char *key, const char *value="") 
    {
        const char *retval;
        retval = msGetOutputFormatOption(self, key, value);
        return strdup(retval);
    }
    
}



/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for manipulating OGC request stuff via
             mapscript.
   Author:   Sean Gillies, sgillies@frii.com
             
   ===========================================================================
   Copyright (c) 2004 Sean Gillies
   Copyright (c) 2006 Frank Warmerdam
   Copyright (c) 1996-2007 Regents of the University of Minnesota.
       
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


%{
/**
The following functions are used to simulate the CGI environment for a request
and return default values when a environment key is passed in
*/
static char *msGetEnvURL( const char *key, void *thread_context )
{
    if( strcmp(key,"REQUEST_METHOD") == 0 )
        return "GET";

    if( strcmp(key,"QUERY_STRING") == 0 )
        return (char *) thread_context;

    return NULL;
}

static char *msPostEnvURL(const char *key, void *thread_context)
{
    if (strcmp(key, "REQUEST_METHOD") == 0)
        return "POST";

    if (strcmp(key, "QUERY_STRING") == 0)
        return (char *)thread_context;

    return NULL;
}
%}

%rename(OWSRequest) cgiRequestObj;

// include has to be placed here
%include "../../cgiutil.h"

/* Class for programming OWS services - SG */

%extend cgiRequestObj {

    /**
    Not associated with other mapscript classes. Serves as a message intermediary 
    between an application and MapServer's OWS capabilities. Using it permits creation 
    of lightweight WMS services.
    */
    cgiRequestObj()
    {
        cgiRequestObj *request;

        request = msAllocCgiObj();
        if (!request) {
            msSetError(MS_CGIERR, "Failed to initialize object","OWSRequest()");
            return NULL;
        }
        
        return request;
    }

    ~cgiRequestObj()
    {
        msFreeCgiObj(self);
    }

    /**
    Initializes the OWSRequest object from the cgi environment variables 
    ``REQUEST_METHOD``, ``QUERY_STRING`` and ``HTTP_COOKIE``. 
    Returns the number of name/value pairs collected. 
    Warning: most errors will result in a process exit!
    */
    int loadParams()
    {
        self->NumParams = loadParams( self, NULL, NULL, 0, NULL);
        return self->NumParams;
    }

    /**
    Initializes the OWSRequest object from the provided URL which is 
    treated like a ``QUERY_STRING``. 
    Note that ``REQUEST_METHOD=GET`` and no post data is assumed in this case. 
    */
    int loadParamsFromURL( const char *url )
    {
        self->NumParams = loadParams( self, msGetEnvURL, NULL, 0, (void*)url );
        return self->NumParams;
    }

    /**
    Initializes the OWSRequest object with POST data, along with a the provided URL which is
    treated like a ``QUERY_STRING``.
    Note that ``REQUEST_METHOD=POST`` and the caller is responsible for setting the correct
    content type e.g. ``req.contenttype = "application/xml"``
    */
    int loadParamsFromPost( char *postData, const char *url)
    {
        self->NumParams = loadParams( self, msPostEnvURL, msStrdup(postData), (ms_uint32) strlen(postData), (void*)url);
        return self->NumParams;
    }

    /**
    Set a request parameter. For example:
    
    request.setParameter('REQUEST', 'GetMap')
    request.setParameter('BBOX', '-107.0,40.0,-106.0,41.0')
    */
    void setParameter(char *name, char *value) 
    {
        int i;
        
        if (self->NumParams == MS_DEFAULT_CGI_PARAMS) {
            msSetError(MS_CHILDERR, "Maximum number of items, %d, has been reached", "setItem()", MS_DEFAULT_CGI_PARAMS);
        }
        
        for (i=0; i<self->NumParams; i++) {
            if (strcasecmp(self->ParamNames[i], name) == 0) {
                free(self->ParamValues[i]);
                self->ParamValues[i] = msStrdup(value);
                break;
            }
        }
        if (i == self->NumParams) {
            self->ParamNames[self->NumParams] = msStrdup(name);
            self->ParamValues[self->NumParams] = msStrdup(value);
            self->NumParams++;
        }
    }

    /**
    Add a request parameter, even if the parameter key was previously set. 
    This is useful when multiple parameters with the same key are required. 
    For example: 
    request.addParameter('SIZE', 'x(100)')
    request.addParameter('SIZE', 'y(100)')
    */
    void addParameter(char *name, char *value)
    {
        if (self->NumParams == MS_DEFAULT_CGI_PARAMS) {
            msSetError(MS_CHILDERR, "Maximum number of items, %d, has been reached", "addParameter()", MS_DEFAULT_CGI_PARAMS);
        }
        self->ParamNames[self->NumParams] = msStrdup(name);
        self->ParamValues[self->NumParams] = msStrdup(value);
        self->NumParams++;
    }

    /// Return the name of the parameter at ``index`` in the request's 
    /// array of parameter names.
    char *getName(int index) 
    {
        if (index < 0 || index >= self->NumParams) {
            msSetError(MS_CHILDERR, "Invalid index, valid range is [0, %d]", "getName()", self->NumParams-1);
            return NULL;
        }
        return self->ParamNames[index];
    }

    /// Return the value of the parameter at ``index`` in the request's 
    /// array of parameter values.
    char *getValue(int index) 
    {
        if (index < 0 || index >= self->NumParams) {
            msSetError(MS_CHILDERR, "Invalid index, valid range is [0, %d]", "getValue()", self->NumParams-1);
            return NULL;
        }
        return self->ParamValues[index];
    }

    /// Return the value associated with the parameter ``name``
    char *getValueByName(const char *name) 
    {
        int i;
        for (i=0; i<self->NumParams; i++) {
            if (strcasecmp(self->ParamNames[i], name) == 0) {
                return self->ParamValues[i];
            }
        }
        return NULL;
    }

}

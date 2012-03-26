/* ===========================================================================
   $Id$
 
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
static char *msGetEnvURL( const char *key, void *thread_context )
{
    if( strcmp(key,"REQUEST_METHOD") == 0 )
        return "GET";

    if( strcmp(key,"QUERY_STRING") == 0 )
        return (char *) thread_context;

    return NULL;
}
%}

%rename(OWSRequest) cgiRequestObj;

%include "../../cgiutil.h"

/* Class for programming OWS services - SG */

%extend cgiRequestObj {

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

    int loadParams()
    {
	self->NumParams = loadParams( self, NULL, NULL, 0, NULL);
	return self->NumParams;
    }

    int loadParamsFromURL( const char *url )
    {
	self->NumParams = loadParams( self, msGetEnvURL, NULL, 0, (void*)url );
	return self->NumParams;
    }

    void setParameter(char *name, char *value) 
    {
        int i;
        
        if (self->NumParams == MS_DEFAULT_CGI_PARAMS) {
            msSetError(MS_CHILDERR, "Maximum number of items, %d, has been reached", "setItem()", MS_DEFAULT_CGI_PARAMS);
        }
        
        for (i=0; i<self->NumParams; i++) {
            if (strcasecmp(self->ParamNames[i], name) == 0) {
                free(self->ParamValues[i]);
                self->ParamValues[i] = strdup(value);
                break;
            }
        }
        if (i == self->NumParams) {
            self->ParamNames[self->NumParams] = strdup(name);
            self->ParamValues[self->NumParams] = strdup(value);
            self->NumParams++;
        }
    }
    
    void addParameter(char *name, char *value)
    {
        if (self->NumParams == MS_DEFAULT_CGI_PARAMS) {
            msSetError(MS_CHILDERR, "Maximum number of items, %d, has been reached", "addParameter()", MS_DEFAULT_CGI_PARAMS);
        }
        self->ParamNames[self->NumParams] = strdup(name);
        self->ParamValues[self->NumParams] = strdup(value);
        self->NumParams++;
    }

    char *getName(int index) 
    {
        if (index < 0 || index >= self->NumParams) {
            msSetError(MS_CHILDERR, "Invalid index, valid range is [0, %d]", "getName()", self->NumParams-1);
            return NULL;
        }
        return self->ParamNames[index];
    }

    char *getValue(int index) 
    {
        if (index < 0 || index >= self->NumParams) {
            msSetError(MS_CHILDERR, "Invalid index, valid range is [0, %d]", "getValue()", self->NumParams-1);
            return NULL;
        }
        return self->ParamValues[index];
    }

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


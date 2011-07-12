/* $Id$ */

%rename(OWSRequest) cgiRequestObj;

%include "../../cgiutil.h"

/* Class for programming OWS services - SG */

%extend cgiRequestObj {

#if defined(SWIGJAVA) || defined(SWIGCSHARP)
    cgiRequestObj()
#else
    cgiRequestObj(void)
#endif
    {
        cgiRequestObj *request;
        
        request = msAllocCgiObj();
        if (!request) {
            msSetError(MS_CGIERR, "Failed to initialize object","OWSRequest()");
            return NULL;
        }
        
        request->ParamNames = (char **) malloc(MS_MAX_CGI_PARAMS*sizeof(char*));
        request->ParamValues = (char **) malloc(MS_MAX_CGI_PARAMS*sizeof(char*));
        if (request->ParamNames==NULL || request->ParamValues==NULL) {
	        msSetError(MS_MEMERR, NULL, "OWSRequest()");
            return NULL;
        }
        return request;
    }

#if defined(SWIGJAVA) || defined(SWIGCSHARP)
    ~cgiRequestObj()
#else
    ~cgiRequestObj(void)
#endif
    {
        msFreeCharArray(self->ParamNames, self->NumParams);
        msFreeCharArray(self->ParamValues, self->NumParams);
        free(self);
    }

    int loadParams()
    {
	self->NumParams = loadParams( self );
	return self->NumParams;
    }

    void setParameter(char *name, char *value) 
    {
        int i;
        
        if (self->NumParams == MS_MAX_CGI_PARAMS) {
            msSetError(MS_CHILDERR, "Maximum number of items, %d, has been reached", "setItem()", MS_MAX_CGI_PARAMS);
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


#include "map.h"
#include "maperror.h"

typedef struct {
  // TODO: union?
} rangeObj; 

typedef struct {
  char *version;				// 1.0.0 for now
  char *request;				// GetCapabilities|DescribeCoverage|GetCoverage
  char *service;				// MUST be WCS
  char *section;				// of capabilities document: Service|Capability|ContentMetadata
  char **coverages;				// NULL terminated list of coverages (in the case of a GetCoverage there will only be 1)
  char *crs;					// request coordinate reference system
  char *response_crs;			// response coordinate reference system
  double bbox[6];				// subset bounding box (3D)
  char **times;
  long width, height, depth;	// image dimensions
  double resx, resy, resz;		// resolution
  char *format;
  char *exceptions;				// exception MIME type, (default application=vnd.ogc.se_xml?)
} wcsParamsObj;

static int msWCSException(mapObj *map, const char *version) 
{
  return MS_FAILURE;
}

static wcsParamsObj *msWCSCreateParams()
{
  wcsParamsObj *params;
  
  params = (wcsParamsObj *) calloc(1, sizeof(wcsParamsObj));
  if(params) { // initialize a few things
   
  }
  
  return params;
}

static void msWCSFreeParams(wcsParamsObj *params)
{
    if(params) {
	  // TODO
      if(params->version) free(params->version);
	  if(params->request) free(params->request);
	  if(params->service) free(params->service);
      if(params->section) free(params->section);
	  if(params->crs) free(params->crs);
      if(params->response_crs) free(params->crs);      
      if(params->format) free(params->format);
	  if(params->exceptions) free(params->exceptions);
	}
}

static void msWCSParseRequest(cgiRequestObj *request, wcsParamsObj *params)
{
  int i;

  if(!request || !params) // nothing to do
	return;
	
  if(request->NumParams > 0) {
    for(i=0; i<request->NumParams; i++) {
       if(strcasecmp(request->ParamNames[i], "VERSION") == 0)
	     params->version = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "REQUEST") == 0)
		 params->request = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "SERVICE") == 0)
		 params->service = strdup(request->ParamValues[i]);
       // and so on...
    }
  }

  // we are not dealing with an XML encoded request at this point
  return;
}

static int msWCSGetCapabilties(mapObj *map, wcsParamsObj *params)
{
  return MS_FALSE;
}

static int msWCSDescribeCoverage(mapObj *map, wcsParamsObj *params)
{
  return MS_FALSE;
}

static int msWCSGetCoverage(mapObj *map, wcsParamsObj *params)
{
  return MS_FALSE;
}

// Entry point for WCS requests
int msWCSDispatch(mapObj *map, cgiRequestObj *request)
{
#ifdef USE_WCS_SVR
  wcsParamsObj *params;
  
  // populate the service parameters
  params = msWCSCreateParams();
  msWCSParseRequest(request, params);

  // If SERVICE is specified then it MUST be "WCS"
  if(params->ervice != NULL && strcasecmp(params->service, "WFS") != 0)
    return MS_DONE;

  // If SERVICE, VERSION and REQUEST not included then noat a WCS request
  if(!params->service && !params->version && !params->request)
	return MS_DONE;

  return(MS_DONE); // no WCS support committed yet
#else
  msSetError(MS_WCSERR, "WCS server support is not available.", "msWCSDispatch()");
  return(MS_FAILURE);
#endif
}

#include "map.h"
#include "maperror.h"

typedef struct {
  // TODO: union?
} rangeObj; 

typedef struct {
  char *version;		// 1.0.0 for now
  char *request;		// GetCapabilities|DescribeCoverage|GetCoverage
  char *service;		// MUST be WCS
  char *section;		// of capabilities document: Service|Capability|ContentMetadata
  char **coverages;		// NULL terminated list of coverages (in the case of a GetCoverage there will only be 1)
  char *crs;			// request coordinate reference system
  char *response_crs;		// response coordinate reference system
  double bbox[6];		// subset bounding box (3D), although we'll only use 2D at best
  char **times;
  long width, height, depth;	// image dimensions
  double resx, resy, resz;      // resolution
  char *format;
  char *exceptions;		// exception MIME type, (default application=vnd.ogc.se_xml?)
} wcsParamsObj;

static int msWCSException(mapObj *map, const char *version) 
{
  printf("Content-type: text/xml%c%c",10,10);

  printf("<ServiceExceptionReport\n");
  printf("xmlns=\"http://www.opengis.net/ogc\" ");
  printf("xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
  printf("xsi:schemaLocation=\"http://www.opengis.net/ogc http://ogc.dmsolutions.ca/wfs/1.0/OGC-exception.xsd\">\n");
  printf("  <ServiceException>\n");
  msWriteErrorXML(stdout);
  printf("  </ServiceException>\n");
  printf("</ServiceExceptionReport>\n");

  return MS_FAILURE;
}

static wcsParamsObj *msWCSCreateParams()
{
  wcsParamsObj *params;

  fprintf(stderr, "in msWCSCreateParams()\n");
  
  params = (wcsParamsObj *) calloc(1, sizeof(wcsParamsObj));
  if(params) { // initialize a few things to default values
    params->version = strdup("1.0.0");
  }
  
  return params;
}

static void msWCSFreeParams(wcsParamsObj *params)
{
  fprintf(stderr, "in msWCSFreeParams()\n");

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

  fprintf(stderr, "in msWCSParseRequest()\n");	

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

  fprintf(stderr, "version=%s\n", params->version);

  // we are not dealing with an XML encoded request at this point
  return;
}

static int msWCSGetCapabilities_Service(mapObj *map, wcsParamsObj *params)
{
  printf("Service section goes here...\n");
  return MS_SUCCESS;
}

static int msWCSGetCapabilities_Capability(mapObj *map, wcsParamsObj *params)
{
  printf("Capability section(s) go here...\n");
  return MS_SUCCESS;
}

static int msWCSGetCapabilities_ContentMetadata(mapObj *map, wcsParamsObj *params)
{
  printf("ContentMetadata section(s) go here...\n");
  return MS_SUCCESS;
}

static int msWCSGetCapabilities(mapObj *map, wcsParamsObj *params)
{
  printf("Content-type: text/xml%c%c",10,10);

  // print common capability elements
  printf("capabilties start here...\n");

  // print the various capability sections
  if(!params->service || strcasecmp(params->service, "Service"))
    msWCSGetCapabilities_Service(map, params);

  if(!params->service || strcasecmp(params->service, "Capability"))
    msWCSGetCapabilities_Capability(map, params);

  if(!params->service || strcasecmp(params->service, "ContentMetadata"))
    msWCSGetCapabilities_ContentMetadata(map, params);

  printf("capabilties end here...\n");

  return MS_SUCCESS;
}

static int msWCSDescribeCoverage(mapObj *map, wcsParamsObj *params)
{
  return MS_SUCCESS;
}

static int msWCSGetCoverage(mapObj *map, wcsParamsObj *params)
{
  return MS_SUCCESS;
}

// Entry point for WCS requests
int msWCSDispatch(mapObj *map, cgiRequestObj *request)
{
#ifdef USE_WCS_SVR
  wcsParamsObj *params;
  
  fprintf(stderr, "in mwWCSDispatch()\n");

  // populate the service parameters
  params = msWCSCreateParams();
  msWCSParseRequest(request, params);

  // If SERVICE is specified then it MUST be "WCS"
  if(params->service && strcasecmp(params->service, "WCS") != 0)
    return MS_DONE;

  // If SERVICE and REQUEST not included then not a WCS request
  if(!params->service && !params->request)
    return MS_DONE;

  /*
  ** Start dispatching requests
  */
  if(strcasecmp(params->request, "GetCapabilities") == 0)    
    return msWCSGetCapabilities(map, params); // TODO, how do we free params

  /*
  ** Clean-up
  */
  msWCSFreeParams(params);

  return MS_SUCCESS;
#else
  msSetError(MS_WCSERR, "WCS server support is not available.", "msWCSDispatch()");
  return MS_FAILURE;
#endif
}

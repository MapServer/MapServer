#include "map.h"
#include "maperror.h"
#include "gdal.h"
#include "mapthread.h"
#include "cpl_string.h" // GDAL string handling

#ifdef USE_WCS_SVR

typedef struct {
  // TODO: union?
} rangeObj; 

/*
** Structure to hold metadata taken from the image or image tile index
*/
typedef struct {
  const char *srs;
  rectObj extent, llextent;
  double geotransform[6];
  int xsize, ysize;
  double xresolution, yresolution;
  int bandcount; 
  int imagemode;
} coverageMetadataObj;

typedef struct {
  char *version;		// 1.0.0 for now
  char *request;		// GetCapabilities|DescribeCoverage|GetCoverage
  char *service;		// MUST be WCS
  char *section;		// of capabilities document: /WCS_Capabilities/Service|/WCS_Capabilities/Capability|/WCS_Capabilities/ContentMetadata
  char **coverages;		// NULL terminated list of coverages (in the case of a GetCoverage there will only be 1)
  char *crs;			// request coordinate reference system
  char *response_crs;	// response coordinate reference system
  rectObj bbox;		    // subset bounding box (3D), although we'll only use 2D
  char **times;
  long width, height, depth;	// image dimensions
  double resx, resy, resz;      // resolution
  char *format;
  char *exceptions;		// exception MIME type, (default application=vnd.ogc.se_xml)
} wcsParamsObj;

// functions defined later in this source file
static int msWCSGetCoverageMetadata( layerObj *layer, coverageMetadataObj *cm );

static int msWCSException(const char *version) 
{
  // printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10);
  printf("Content-type: text/xml%c%c",10,10);

  // TODO: see WCS specific exception schema (Appendix 6)

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

static void msWCSPrintRequestCapability(const char *version, const char *request_tag, const char *script_url)
{
  printf("    <%s>\n", request_tag);

  printf("      <DCPType>\n");
  printf("        <HTTP>\n");
  printf("          <Get onlineResource=\"%s\" />\n", script_url);
  printf("        </HTTP>\n");
  printf("      </DCPType>\n");
  printf("      <DCPType>\n");
  printf("        <HTTP>\n");
  printf("          <Post onlineResource=\"%s\" />\n", script_url);
  printf("        </HTTP>\n");
  printf("      </DCPType>\n");

  printf("    </%s>\n", request_tag);
}

static wcsParamsObj *msWCSCreateParams()
{
  wcsParamsObj *params;
 
  params = (wcsParamsObj *) calloc(1, sizeof(wcsParamsObj));
  if(params) { // initialize a few things to default values
    params->version = strdup("1.0.0");
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

static int msWCSIsLayerSupported(layerObj *layer)
{
    // only raster layers, with 'DUMP TRUE' explicitly defined, are elligible to be served via WCS, WMS rasters are not ok
    if(layer->dump && (layer->type == MS_LAYER_RASTER) && layer->connectiontype != MS_WMS) return MS_TRUE;

    return MS_FALSE;
}


static int msWCSParseRequest(cgiRequestObj *request, wcsParamsObj *params, mapObj *map)
{
  int i;

  if(!request || !params) // nothing to do
	return MS_SUCCESS;

  if(request->NumParams > 0) {
    for(i=0; i<request->NumParams; i++) {
    
       if(strcasecmp(request->ParamNames[i], "VERSION") == 0)
	     params->version = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "REQUEST") == 0)
	     params->request = strdup(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "SERVICE") == 0)
	     params->service = strdup(request->ParamValues[i]);
	   else if(strcasecmp(request->ParamNames[i], "SECTION") == 0)
	     params->section = strdup(request->ParamValues[i]); // TODO: validate value here

       // GetCoverage parameters.
       else if(strcasecmp(request->ParamNames[i], "BBOX") == 0) {
         char **tokens;
         int n;
          
         tokens = split(request->ParamValues[i], ',', &n);
         if(tokens==NULL || n != 4) {
           msSetError(MS_WMSERR, "Wrong number of arguments for BBOX.", "msWMSLoadGetMapParams()");
           return msWCSException(params->version);
         }
         params->bbox.minx = atof(tokens[0]);
         params->bbox.miny = atof(tokens[1]);
         params->bbox.maxx = atof(tokens[2]);
         params->bbox.maxy = atof(tokens[3]);
           
         msFreeCharArray(tokens, n);
       } else if(strcasecmp(request->ParamNames[i], "RESX") == 0)
         params->resx = atof(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "RESY") == 0)
         params->resy = atof(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "WIDTH") == 0)
         params->width = atoi(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "HEIGHT") == 0)
         params->height = atoi(request->ParamValues[i]);
       else if(strcasecmp(request->ParamNames[i], "COVERAGE") == 0)
         params->coverages = CSLAddString(params->coverages, request->ParamValues[i]);

       // and so on...
    }
  }

  // we are not dealing with an XML encoded request at this point
  return MS_SUCCESS;
}

static int msWCSGetCapabilities_Service(mapObj *map, wcsParamsObj *params)
{
  // start the Service section, only need the full start tag if this is the only section requested
  if(!params->section) 
    printf("<Service>\n");
  else
    printf("<Service\n"
           "   version=\"%s\" \n"
           "   updateSequence=\"0\" \n"
           "   xmlns=\"http://www.opengis.net/wcs\" \n"
           "   xmlns:ogc=\"http://www.opengis.net/ogc\" \n"
           "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
           "   xsi:schemaLocation=\"http://schemas.opengis.net/wcs/%s/wcsCapabilities.xsd\">\n", params->version, params->version);

  // TODO: add MetadataLink (optional)
  
  msOWSPrintMetadata(stdout, map->web.metadata, "CO", "description", OWS_NOERR, "  <Description>%s</Description>\n", NULL);
  msOWSPrintMetadata(stdout, map->web.metadata, "CO", "name", OWS_NOERR, "  <Name>%s</Name>\n", "MapServer WCS");
  msOWSPrintMetadata(stdout, map->web.metadata, "CO", "label", OWS_WARN, "  <Label>%s</Label>\n", NULL);

  // we are not supporting the optional keyword type, at least not yet
  msOWSPrintMetadataList(stdout, map->web.metadata, "CO", "keywordlist", "  <Keywords>\n", "  </Keywords>\n", "    <Keyword>%s</Keyword>\n", NULL);

  // TODO: add ResponsibleParty (optional)

  msOWSPrintMetadata(stdout, map->web.metadata, "CO", "fees", OWS_NOERR, "  <Fees>%s</Fees>\n", "NONE");
  msOWSPrintMetadataList(stdout, map->web.metadata, "CO", "accessconstraints", "  <AccessConstraints>\n", "  </AccessConstraints>\n", "    %s\n", "NONE");

  // done
  printf("</Service>\n");

  return MS_SUCCESS;
}

static int msWCSGetCapabilities_Capability(mapObj *map, wcsParamsObj *params)
{
  char *script_url=NULL, *script_url_encoded;

   // we need this server's onlineresource for the request section
  if((script_url=msOWSGetOnlineResource(map, "wcs_onlineresource")) == NULL || (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL) {
    return msWCSException(params->version);
  }

  // start the Capabilty section, only need the full start tag if this is the only section requested
  if(!params->section) 
    printf("<Capability>\n");
  else
    printf("<Capability\n"
           "   version=\"%s\" \n"
           "   updateSequence=\"0\" \n"
           "   xmlns=\"http://www.opengis.net/wcs\" \n"
           "   xmlns:ogc=\"http://www.opengis.net/ogc\" \n"
           "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
           "   xsi:schemaLocation=\"http://schemas.opengis.net/wcs/%s/wcsCapabilities.xsd\">\n", params->version, params->version);

  // describe the types of requests the server can handle
  printf("  <Request>\n");

  msWCSPrintRequestCapability(params->version, "GetCapabilities", script_url_encoded);
  msWCSPrintRequestCapability(params->version, "DescribeCoverage", script_url_encoded);
  msWCSPrintRequestCapability(params->version, "GetCoverage", script_url_encoded);
 
  printf("  </Request>\n");

  // describe the exception formats the server can produce
  printf("  <Exception>\n");
  printf("    <Format>application/vnd.ogc.se_xml</Format>\n");
  printf("  </Exception>\n");

  // describe any vendor specific capabilities
   printf("  <VendorSpecificCapabilities />\n"); // none yet

  // done
  printf("</Capability>\n");

  return MS_SUCCESS;
}

static int msWCSGetCapabilities_CoverageOfferingBrief(layerObj *layer, wcsParamsObj *params) 
{
  coverageMetadataObj cm;
  int status;

  if(!msWCSIsLayerSupported(layer)) return MS_SUCCESS; // not an error, this layer cannot be served via WCS

  status = msWCSGetCoverageMetadata(layer, &cm);
  if(status != MS_SUCCESS) return MS_FAILURE;
  
  // start the CoverageOfferingBrief section
  printf("  <CoverageOfferingBrief>\n"); // is this tag right (I hate schemas without ANY examples)

  // TODO: add MetadataLink (optional)
  
  msOWSPrintMetadata(stdout, layer->metadata, "CO", "description", OWS_NOERR, "  <Description>%s</Description>\n", NULL);
  msOWSPrintMetadata(stdout, layer->metadata, "CO", "name", OWS_NOERR, "  <Name>%s</Name>\n", layer->name);

  msOWSPrintMetadata(stdout, layer->metadata, "CO", "label", OWS_WARN, "  <Label>%s</Label>\n", NULL);

  // TODO: add elevation and temporal ranges to LatLonEnvelope (optional)
  printf("    <LatLonEnvelope srsName=\"WGS84 (DD)\">\n");
  printf("      <gml:pos>%g %g</gml:pos>\n", cm.llextent.minx, cm.llextent.miny); // TODO: don't know if this is right
  printf("      <gml:pos>%g %g</gml:pos>\n", cm.llextent.maxx, cm.llextent.maxy);
  printf("    </LatLonEnvelope>\n");

  // we are not supporting the optional keyword type, at least not yet
  msOWSPrintMetadataList(stdout, layer->metadata, "CO", "keywordlist", "  <Keywords>\n", "  </Keywords>\n", "    <Keyword>%s</Keyword>\n", NULL);

  // done
  printf("  </CoverageOfferingBrief>\n");

  return MS_SUCCESS;
}

static int msWCSGetCapabilities_ContentMetadata(mapObj *map, wcsParamsObj *params)
{
  int i;

  // start the ContentMetadata section, only need the full start tag if this is the only section requested
  // TODO: add Xlink attributes for other sources of this information 
  if(!params->section)
    printf("<ContentMetadata>\n");
  else
    printf("<ContentMetadata\n"
           "   version=\"%s\" \n"
           "   updateSequence=\"0\" \n"
           "   xmlns=\"http://www.opengis.net/wcs\" \n"
           "   xmlns:ogc=\"http://www.opengis.net/ogc\" \n"
           "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
           "   xsi:schemaLocation=\"http://schemas.opengis.net/wcs/%s/wcsCapabilities.xsd\">\n", params->version, params->version);

  for(i=0; i<map->numlayers; i++)
    msWCSGetCapabilities_CoverageOfferingBrief(&(map->layers[i]), params);

  // done
  printf("</ContentMetadata>\n");

  return MS_SUCCESS;
}

static int msWCSGetCapabilities(mapObj *map, wcsParamsObj *params)
{
  // printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10);
  printf("Content-type: text/xml%c%c",10,10);

  // print common capability elements 
  msOWSPrintMetadata(stdout, map->web.metadata, NULL, "wcs_encoding", OWS_NOERR, "<?xml version='1.0' encoding=\"%s\" ?>\n", "ISO-8859-1");

  if(!params->section) printf("<WCS_Capabilities\n"
                              "   version=\"%s\" \n"
                              "   updateSequence=\"0\" \n"
                              "   xmlns=\"http://www.opengis.net/wcs\" \n"
                              "   xmlns:ogc=\"http://www.opengis.net/ogc\" \n"
                              "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                              "   xsi:schemaLocation=\"http://schemas.opengis.net/wcs/%s/wcsCapabilities.xsd\">\n", params->version, params->version);
         
  // print the various capability sections
  if(!params->section || strcasecmp(params->section, "/WCS_Capabilities/Service") == 0)
    msWCSGetCapabilities_Service(map, params);

  if(!params->section || strcasecmp(params->section, "/WCS_Capabilities/Capability")  == 0)
    msWCSGetCapabilities_Capability(map, params);

  if(!params->section || strcasecmp(params->section, "/WCS_Capabilities/ContentMetadata")  == 0)
    msWCSGetCapabilities_ContentMetadata(map, params);

  // done
  if(!params->section) printf("</WCS_Capabilities>\n");

  // clean up
  // msWCSFreeParams(params);

  return MS_SUCCESS;
}

/* Example RectifiedGrid from GML 3.0 spec 

<gml:RectifiedGrid dimension="2"> 
    <gml:limits> 
      <gml:GridEnvelope> 
        <gml:low>1 1</gml:low> 
        <gml:high>3 3</gml:high> 
      </gml:GridEnvelope> 
    </gml:limits> 
    <gml:axisName>u</gml:axisName> 
    <gml:axisName>v</gml:axisName> 
    <gml:origin> 
      <gml:Point gml:id="palindrome"> 
        <gml:coordinates>1.2,3.3,2.1</gml:coordinates> 
      </gml:Point> 
    </gml:origin> 
    <gml:offsetVector>1,2,3</gml:offsetVector> 
    <gml:offsetVector >2,1,0</gml:offsetVector> 
  </gml:RectifiedGrid>   
*/

static int msWCSDescribeCoverage_AxisDescription(layerObj *layer, char *id)
{
  const char *value;
  char tag[100]; // should be plenty of space
  
  printf("        <AxisDescription>\n"); // TODO: there are some optional attributes
  
  // TODO: add MetadataLink (optional)
  
  snprintf(tag, 100, "%s_description", id);
  msOWSPrintMetadata(stdout, layer->metadata, "CO", tag, OWS_NOERR, "          <Description>%s</Description>\n", NULL);
  snprintf(tag, 100, "%s_name", id);
  msOWSPrintMetadata(stdout, layer->metadata, "CO", tag, OWS_WARN, "          <Name>%s</Name>\n", NULL);
 
  snprintf(tag, 100, "%s_label", id);
  msOWSPrintMetadata(stdout, layer->metadata, "CO", tag, OWS_WARN, "          <Label>%s</Label>\n", NULL);
  
  // Values
  printf("          <Values>\n"); // TODO: there are some optional attributes
  
  // single values
  snprintf(tag, 100, "%s_values", id);
  if(msOWSLookupMetadata(layer->metadata, "CO", tag))
    msOWSPrintMetadataList(stdout, layer->metadata, "CO", tag, NULL, NULL, "            <SingleValue>%s</SingleValue>\n", NULL); // TODO: there are some optional attributes
  
  // intervals, only one per axis for now
  snprintf(tag, 100, "%s_interval", id);
  if((value = msOWSLookupMetadata(layer->metadata, "CO", tag)) != NULL) {
    char **tokens;
    int numtokens;

     tokens = split(value, ',', &numtokens);
     if(tokens && numtokens > 0) {
       printf("          <Interval>\n"); // TODO: there are some optional attributes
       if(numtokens >= 1) printf("          <Min>%s</Min>\n", tokens[0]);
       if(numtokens >= 2) printf("          <Max>%s</Max>\n", tokens[1]);
       if(numtokens >= 3) printf("          <Res>%s</Res>\n", tokens[2]);
       printf("          </Interval>\n"); 
     }
  }
  
  printf("          </Values>\n");
  
  // TODO: add NullValues (optional)
  
  printf("        </AxisDescription>\n");
  
  return MS_SUCCESS;
}

static int msWCSDescribeCoverage_CoverageOffering(layerObj *layer, wcsParamsObj *params) 
{
  static char *value; 
  coverageMetadataObj cm;
  int status;

  if(!msWCSIsLayerSupported(layer)) return MS_SUCCESS; // not an error, this layer cannot be served via WCS

  status = msWCSGetCoverageMetadata(layer, &cm);
  if(status != MS_SUCCESS) return MS_FAILURE;
  
  // start the Coverage section
  printf("  <CoverageOffering>\n");

  // TODO: add MetadataLink (optional)
  
  msOWSPrintMetadata(stdout, layer->metadata, "CO", "description", OWS_NOERR, "  <Description>%s</Description>\n", NULL);
  msOWSPrintMetadata(stdout, layer->metadata, "CO", "name", OWS_NOERR, "  <Name>%s</Name>\n", layer->name);

  msOWSPrintMetadata(stdout, layer->metadata, "CO", "label", OWS_WARN, "  <Label>%s</Label>\n", NULL);

  // TODO: add elevation and temporal ranges to LatLonEnvelope (optional)
  printf("    <LatLonEnvelope srsName=\"WGS84 (DD)\">\n");
  printf("      <gml:pos>%g %g</gml:pos>\n", cm.llextent.minx, cm.llextent.miny);
  printf("      <gml:pos>%g %g</gml:pos>\n", cm.llextent.maxx, cm.llextent.maxy);
  printf("    </LatLonEnvelope>\n");

  // we are not supporting the optional keyword type, at least not yet
  msOWSPrintMetadataList(stdout, layer->metadata, "CO", "keywordlist", "  <Keywords>\n", "  </Keywords>\n", "    <Keyword>%s</Keyword>\n", NULL);

  // DomainSet: starting simple, just a spatial domain (gml:envelope) and optionally a temporal domain
  printf("    <DomainSet>\n");

  // SpatialDomain
  printf("      <SpatialDomain>\n");
  
  // envelope in lat/lon
  printf("        <gml:Envelope srsName=\"WGS84 (DD)\">\n");
  printf("          <gml:pos>%g %g</gml:pos>\n", cm.llextent.minx, cm.llextent.miny);
  printf("          <gml:pos>%g %g</gml:pos>\n", cm.llextent.maxx, cm.llextent.maxy);
  printf("        </gml:Envelope>\n");
  
  // envelope in the native srs
  if((value = msOWSGetEPSGProj(&(layer->projection), layer->metadata, "CO", MS_TRUE)) != NULL)    
    printf("        <gml:Envelope srsName=\"%s\">\n", value);
  else if((value = msOWSGetEPSGProj(&(layer->map->projection), layer->map->web.metadata, "CO", MS_TRUE)) != NULL)
    printf("        <gml:Envelope srsName=\"%s\">\n", value);
  else 
    printf("        <!-- NativeCRSs ERROR: missing required information, no SRSs defined -->\n");
  printf("          <gml:pos>%g %g</gml:pos>\n", cm.extent.minx, cm.extent.miny);
  printf("          <gml:pos>%g %g</gml:pos>\n", cm.extent.maxx, cm.extent.maxy);
  printf("        </gml:Envelope>\n");

  printf("      </SpatialDomain>\n");

  // TemporalDomain

  // TODO: figure out when a temporal domain is valid, for example only tiled rasters support time as a domain, plus we need a timeitem
  if(msOWSLookupMetadata(layer->metadata, "CO", "timeposition") || msOWSLookupMetadata(layer->metadata, "CO", "timeperiod")) {
    printf("      <TemporalDomain>\n");

    // TimePosition (should support a value AUTO, then we could mine positions from the timeitem)
    msOWSPrintMetadataList(stdout, layer->metadata, "CO", "timeposition", NULL, NULL, "        <gml:timePosition>%s</gml:timePosition>\n", NULL);    

    // TODO:  add TimePeriod (only one per layer) 

    printf("      </TemporalDomain>\n");
  }
  
  printf("    </DomainSet>\n");
  
  // rangeSet
  printf("    <RangeSet>\n");
  printf("      <RangeSet>\n"); // TODO: there are some optional attributes

  // TODO: add MetadataLink (optional)
  
  msOWSPrintMetadata(stdout, layer->metadata, "CO", "rangeset_description", OWS_NOERR, "        <Description>%s</Description>\n", NULL);
  msOWSPrintMetadata(stdout, layer->metadata, "CO", "rangeset_name", OWS_WARN, "        <Name>%s</Name>\n", NULL);

  msOWSPrintMetadata(stdout, layer->metadata, "CO", "rangeset_label", OWS_WARN, "        <Label>%s</Label>\n", NULL);
  
  // compound range sets
  if((value = msOWSLookupMetadata(layer->metadata, "C", "rangeset_axes")) != NULL) {
    char **tokens;
    int numtokens;

     tokens = split(value, ',', &numtokens);
     if(tokens && numtokens > 0) {
       int i;
       for(i=0; i<numtokens; i++)
         msWCSDescribeCoverage_AxisDescription(layer, tokens[i]);
     }
  }
  
  printf("      </RangeSet>\n");
  printf("    </RangeSet>\n");

  // SupportedCRSs
  printf("    <SupportedCRSs>\n");
  
  // RequestResposeCRSs: check the layer metadata/projection, and then the map metadata/projection if necessary (should never get to the error message)
  if((value = msOWSGetEPSGProj(&(layer->projection), layer->metadata, "CO", MS_FALSE)) != NULL)    
    printf("      <RequestResposeCRSs>%s</RequestResposeCRSs>\n", value);
  else if((value = msOWSGetEPSGProj(&(layer->map->projection), layer->map->web.metadata, "CO", MS_FALSE)) != NULL)
    printf("      <RequestResposeCRSs>%s</RequestResposeCRSs>\n", value);
  else 
    printf("      <!-- RequestResposeCRSs ERROR: missing required information, no SRSs defined -->\n");
  
  // NativeCRSs (only one in our case)
  if((value = msOWSGetEPSGProj(&(layer->projection), layer->metadata, "CO", MS_TRUE)) != NULL)    
    printf("      <NativeCRSs>%s</NativeCRSs>\n", value);
  else if((value = msOWSGetEPSGProj(&(layer->map->projection), layer->map->web.metadata, "CO", MS_TRUE)) != NULL)
    printf("      <NativeCRSs>%s</NativeCRSs>\n", value);
  else 
    printf("      <!-- NativeCRSs ERROR: missing required information, no SRSs defined -->\n");
  
  printf("    </SupportedCRSs>\n");
  
  
  // SupportedFormats
  printf("    <SupportedFormats>\n");
  msOWSPrintMetadata(stdout, layer->metadata, "CO", "formats", OWS_NOERR, "      <Formats>%s</Formats>\n", NULL);
  msOWSPrintMetadata(stdout, layer->metadata, "CO", "nativeformat", OWS_NOERR, "      <NativeFormat>%s</NativeFormat>\n", NULL);
  printf("    </SupportedFormats>\n");
  
  // TODO: add SupportedInterpolations (optional)

  // done
  printf("  </CoverageOffering>\n");

  return MS_SUCCESS;
}

static int msWCSDescribeCoverage(mapObj *map, wcsParamsObj *params)
{
  int i,j;
  
  // printf("Content-type: application/vnd.ogc.se_xml%c%c",10,10);
  printf("Content-type: text/xml%c%c",10,10);

  // print common capability elements 
  msOWSPrintMetadata(stdout, map->web.metadata, NULL, "wcs_encoding", OWS_NOERR, "<?xml version='1.0' encoding=\"%s\" ?>\n", "ISO-8859-1");
  
  // start the DescribeCoverage section
  printf("<CoverageDescription\n"
         "   version=\"%s\" \n"
         "   updateSequence=\"0\" \n"
         "   xmlns=\"http://www.opengis.net/wcs\" \n"
         "   xmlns:ogc=\"http://www.opengis.net/ogc\" \n"
         "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
         "   xsi:schemaLocation=\"http://schemas.opengis.net/wcs/%s/describeCoverage.xsd\">\n", params->version, params->version);
  
  if(params->coverages) { // use the list
    j = 0;
    while(params->coverages[j]) {
      i = msGetLayerIndex(map, params->coverages[j]);
      if(i == -1) continue; // skip this layer, could/should generate an error
      msWCSDescribeCoverage_CoverageOffering(&(map->layers[i]), params);
      j++;
    }
  } else { // return all layers
    for(i=0; i<map->numlayers; i++)
      msWCSDescribeCoverage_CoverageOffering(&(map->layers[i]), params);
  }
  
  // done
  printf("</CoverageDescription>\n");
  
  return MS_SUCCESS;
}

static int msWCSGetCoverage(mapObj *map, wcsParamsObj *params)
{
    imageObj   *image;
    layerObj   *layer;
    int         status, i;

    // Did we get a TIME value (support only a single value for now)
    
    /*
    ** Time support is ONLY valid with layer-based TILEINDEXes. Basically we end up setting the filter and filteritem
    ** values in the appropriate layer.
    */
    
    // Are there any non-spatio/temporal ranges to do subsetting on (e.g. bands)
    
    /*
    ** Note that we'll support filtering based on band, pixel values and potentially on arbitrary TILEINDEX attributes.
    ** The xxxxx_item will tell us which of the 3 cases we are dealing with. Here we are setting TILEINDEX filter and/or
    ** layer processing values.
    */

    // Did we get BBOX values?  

    if( fabs((params->bbox.maxx - params->bbox.minx)) < 0.000000000001 
        || fabs(params->bbox.maxy - params->bbox.miny) < 0.000000000001 )
    {
        msSetError( MS_WMSERR, 
                    "BBOX missing or specifies an empty region.", 
                    "msWCSGetCoverage()" );
        return msWCSException(params->version);
    }
    
    // Compute width/height from bbox and cellsize. 

    if( (params->resx == 0.0 || params->resy == 0.0)
        && params->width != 0 && params->height != 0 )
    {
        params->resx = (params->bbox.maxx - params->bbox.minx) / params->width;
        params->resy = (params->bbox.maxy - params->bbox.miny) / params->height;
    }
    
    // Compute cellsize/res from bbox and raster size.

    if( (params->width == 0 || params->height == 0)
        && params->resx != 0 && params->resy != 0 )
    {
        params->width = (int) ((params->bbox.maxx - params->bbox.minx) / params->resx + 0.5);
        params->height = (int) ((params->bbox.maxy - params->bbox.miny) / params->resy + 0.5);
    }

    // Are we still underspecified? 

    if( params->width == 0 || params->height == 0 
        || params->resx == 0.0 || params->resy == 0.0 )
    {
        msSetError( MS_WMSERR, 
                    "A nonzero RESX/RESY or WIDTH/HEIGHT is required but neither was provided.", 
                    "msWCSGetCoverage()" );
        return msWCSException(params->version);
    }
    
    // Apply region and size to map object. 

    map->width = params->width;
    map->height = params->height;
    map->extent = params->bbox;
 
    map->cellsize = params->resx; // pick on, MapServer only supports square cells

    // Should we compute a new extent, ala WMS?
    if( fabs(params->resx/params->resy - 1.0) > 0.001 )
    {
        msSetError( MS_WMSERR, 
                    "RESX and RESY don't match.  This is currently not a supported option for MapServer WCS.", 
                    "msWCSGetCoverage()" );
        return msWCSException(params->version);
    }
                        
    // Find the layer we are working with. 
    layer = NULL;
    for( i = 0; i < map->numlayers; i++ )
    {
        if( EQUAL(map->layers[i].name,params->coverages[0]) )
        {
            layer = map->layers + i;
            break;
        }
    }

    if( layer == NULL )
    {
        msSetError( MS_WMSERR, 
                    "COVERAGE=%s not found, not in supported layer list.", 
                    "msWCSGetCoverage()", params->coverages[0] );
        return msWCSException(params->version);
    }

    // Select format into map object.

    /* for now we use the default output format defined in the .map file */

    // Create the image object. 
    if(!map->outputformat) {
        msSetError(MS_GDERR, "Map outputformat not set!", 
                   "msWCSGetCoverage()");
        return msWCSException(params->version);
    }
    else if( MS_RENDERER_GD(map->outputformat) )
    {
        image = msImageCreateGD(map->width, map->height, map->outputformat, 
				map->web.imagepath, map->web.imageurl);        
        if( image != NULL ) msImageInitGD( image, &map->imagecolor );
    }
    else if( MS_RENDERER_RAWDATA(map->outputformat) )
    {
        image = msImageCreate(map->width, map->height, map->outputformat,
                              map->web.imagepath, map->web.imageurl);
    }
    else
    {
        msSetError(MS_GDERR, "Map outputformat not supported for WCS!", 
                   "msWCSGetCoverage()");
        return msWCSException(params->version);
    }

    // Actually produce the "grid".
    status = msDrawRasterLayerLow( map, layer, image );
    if( status != MS_SUCCESS )
    {
        return status;
    }
    
    // Emit back to client.
    printf("Content-type: %s%c%c", MS_IMAGE_MIME_TYPE(map->outputformat), 10,10);
    status = msSaveImage(map, image, NULL);

    // Cleanup
    msFreeImage(image);

    return status;
}
#endif /* def USE_WCS_SVR */

// Entry point for WCS requests
int msWCSDispatch(mapObj *map, cgiRequestObj *request)
{
#ifdef USE_WCS_SVR
  wcsParamsObj *params;
  
  // populate the service parameters
  params = msWCSCreateParams();
  if( msWCSParseRequest(request, params, map) == MS_FAILURE )
      return MS_FAILURE;

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
    return msWCSGetCapabilities(map, params);
  else if(strcasecmp(params->request, "DescribeCoverage") == 0)    
    return msWCSDescribeCoverage(map, params);
  else if(strcasecmp(params->request, "GetCoverage") == 0)    
    return msWCSGetCoverage(map, params);

  return MS_DONE; // not a WCS request, let MapServer take it
#else
  msSetError(MS_WCSERR, "WCS server support is not available.", "msWCSDispatch()");
  return MS_FAILURE;
#endif
}

/************************************************************************/
/*                      msWCSGetCoverageMetadata()                      */
/************************************************************************/

#ifdef USE_WCS_SVR
static int msWCSGetCoverageMetadata( layerObj *layer, coverageMetadataObj *cm )
{
  // get information that is "data" independent
  if((cm->srs = msOWSGetEPSGProj(&(layer->projection), layer->metadata, "CO", MS_TRUE)) == NULL) {
    if((cm->srs = msOWSGetEPSGProj(&(layer->map->projection), layer->map->web.metadata, "CO", MS_TRUE)) == NULL) {
      msSetError(MS_WCSERR, "Unable to determine the SRS for this layer, no projection defined and no metadata available.", "msWCSGetCoverageMetadata()");
      return MS_FAILURE;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      If we have "virtual dataset" metadata on the layer, then use    */
  /*      that in preference to inspecting the file(s).                   */
  /* -------------------------------------------------------------------- */
  if( msOWSLookupMetadata(layer->metadata, "CO", "extent") != NULL ) {
    const char *value;

    // get extent
    cm->extent.minx = 0.0;
    cm->extent.maxx = 0.0;
    cm->extent.miny = 0.0;
    cm->extent.maxy = 0.0;
    if( msOWSGetLayerExtent( layer->map, layer, &cm->extent ) == MS_FAILURE )
      return MS_FAILURE;
    
    // get resolution
    cm->xresolution = 0.0;
    cm->yresolution = 0.0;
    if( (value = msOWSLookupMetadata(layer->metadata, "CO", "resolution")) != NULL ) {
      char **tokens;
      int n;
            
      tokens = split(value, ' ', &n);
      if( tokens == NULL || n != 2 ) {
        msSetError( MS_WCSERR, "Wrong number of arguments for wcs|ows_resolution metadata.", "msWCSGetCoverageMetadata()");
        msFreeCharArray( tokens, n );
        return MS_FAILURE;
      }
      cm->xresolution = atof(tokens[0]);
      cm->yresolution = atof(tokens[1]);
      msFreeCharArray( tokens, n );
    }

    // get Size (in pixels and lines)
    cm->xsize = 0;
    cm->ysize = 0;
    if( (value=msOWSLookupMetadata(layer->metadata, "CO", "size")) != NULL ) {
      char **tokens;
      int n;
            
      tokens = split(value, ' ', &n);
      if( tokens == NULL || n != 2 ) {
        msSetError( MS_WCSERR, "Wrong number of arguments for wcs|ows_size metadata.", "msWCSGetCoverageDomain()");
        msFreeCharArray( tokens, n );
        return MS_FAILURE;
      }
      cm->xsize = atoi(tokens[0]);
      cm->ysize = atoi(tokens[1]);
      msFreeCharArray( tokens, n );
    }

    // try to compute raster size
    if( cm->xsize == 0 && cm->ysize == 0 && cm->xresolution != 0.0 && cm->yresolution != 0.0 && cm->extent.minx != cm->extent.maxx && cm->extent.miny != cm->extent.maxy ) {
      cm->xsize = (int) ((cm->extent.maxx - cm->extent.minx) / cm->xresolution + 0.5);
      cm->ysize = (int) fabs((cm->extent.maxy - cm->extent.miny) / cm->yresolution + 0.5);
    }

    // try to compute raster resolution
    if( (cm->xresolution == 0.0 || cm->yresolution == 0.0) && cm->xsize != 0 && cm->ysize != 0 ) {
      cm->xresolution = (cm->extent.maxx - cm->extent.minx) / cm->xsize;
      cm->yresolution = (cm->extent.maxy - cm->extent.miny) / cm->ysize;
    }

    // do we have information to do anything
    if( cm->xresolution == 0.0 || cm->yresolution == 0.0 || cm->xsize == 0 || cm->ysize == 0 ) {
      msSetError( MS_WCSERR, "Failed to collect extent and resolution for WCS coverage from metadata.  Need value wcs|ows_resolution or wcs|ows_size values.", "msWCSGetCoverageMetadata()");
      return MS_FAILURE;
    }
        
    // compute geotransform
    cm->geotransform[0] = cm->extent.minx;
    cm->geotransform[1] = cm->xresolution;
    cm->geotransform[2] = 0.0;
    cm->geotransform[3] = cm->extent.maxy;
    cm->geotransform[4] = 0.0;
    cm->geotransform[5] = -fabs(cm->yresolution);

    // get bands count, or assume 1 if not found
    cm->bandcount = 1;
    if( (value=msOWSLookupMetadata(layer->metadata, "CO", "bandcount")) != NULL) {
      cm->bandcount = atoi(value);
    }

    // get bands type, or assume float if not found
    cm->imagemode = MS_IMAGEMODE_FLOAT32;
    if( (value=msOWSLookupMetadata(layer->metadata, "CO", "imagemode")) != NULL ) {
      if( EQUAL(value,"INT16") )
        cm->imagemode = MS_IMAGEMODE_INT16;
      else if( EQUAL(value,"FLOAT32") )
        cm->imagemode = MS_IMAGEMODE_FLOAT32;
      else if( EQUAL(value,"BYTE") )
        cm->imagemode = MS_IMAGEMODE_BYTE;
      else {
        msSetError( MS_WCSERR, "Content of wcs|ows_imagemode (%s) not recognised.  Should be one of PC256 (byte), INT16 or FLOAT32.", "msWCSGetCoverageMetadata()", value );
        return MS_FAILURE;
      }
    }
  } else if( layer->data == NULL ) { // no virtual metadata, not ok unless we're talking 1 image, hopefully we can fix that
    msSetError( MS_WMSERR, "RASTER Layer with no DATA statement and no WCS virtual dataset metadata.  Tileindexed raster layers not supported for WCS without virtual dataset metadata (cm->extent, wcs_res, wcs_size).", "msWCSGetCoverageDomain()" );
    return MS_FAILURE;
  } else { // work from the file (e.g. DATA)
    GDALDatasetH hDS;
    GDALRasterBandH hBand;
    char szPath[MS_MAXPATHLEN];
  
    msGDALInitialize();

    msAcquireLock( TLOCK_GDAL );
    hDS = GDALOpen(msBuildPath3(szPath,  layer->map->mappath, layer->map->shapepath, layer->data) , GA_ReadOnly );
    if( hDS == NULL ) {
      msReleaseLock( TLOCK_GDAL );
      msSetError( MS_IOERR, "%s", "msWCSGetCoverageMetadata()", CPLGetLastErrorMsg() );
      return MS_FAILURE;
    }


    msGetGDALGeoTransform( hDS, layer->map, layer, cm->geotransform );

    cm->xsize = GDALGetRasterXSize( hDS );
    cm->ysize = GDALGetRasterYSize( hDS );

    cm->extent.minx = cm->geotransform[0];
    cm->extent.maxx = cm->geotransform[0] + cm->geotransform[1] * cm->xsize + cm->geotransform[2] * cm->ysize;
    cm->extent.miny = cm->geotransform[3] + cm->geotransform[4] * cm->xsize + cm->geotransform[5] * cm->ysize;
    cm->extent.maxy = cm->geotransform[3];
    
    cm->bandcount = GDALGetRasterCount( hDS );
        
    if( cm->bandcount == 0 ) {
      msReleaseLock( TLOCK_GDAL );
      msSetError( MS_WMSERR, "Raster file %s has no raster bands.  This cannot be used in a layer.", "msWCSGetCoverageMetadata()", layer->data );
      return MS_FAILURE;
    }

    hBand = GDALGetRasterBand( hDS, 1 );
    switch( GDALGetRasterDataType( hBand ) ) {
    case GDT_Byte:
      cm->imagemode = MS_IMAGEMODE_BYTE;
      break;
    case GDT_Int16:
      cm->imagemode = MS_IMAGEMODE_INT16;
      break;
    default:
      cm->imagemode = MS_IMAGEMODE_FLOAT32;
      break;
    }

    GDALClose( hDS );
    msReleaseLock( TLOCK_GDAL );
  }
 
  // we must have the bounding box in lat/lon (WGS84 DD), is that default lat/lon transformation
  cm->llextent = cm->extent;
  if (layer->projection.numargs > 0 && !pj_is_latlong(layer->projection.proj)) // check the layer projection
    msProjectRect(&(layer->projection), NULL, &(cm->llextent));
  else if (layer->map->projection.numargs > 0 && !pj_is_latlong(layer->map->projection.proj)) // check the map projection
    msProjectRect(&(layer->map->projection), NULL, &(cm->llextent));
  else { // projection was specified in the metadata only (EPSG:... only at the moment) 
    projectionObj proj;
    char projstring[32];

    msInitProjection(&proj); // or bad things happen

    sprintf(projstring, "init=epsg:%.20s", cm->srs+5);
    if (msLoadProjectionString(&proj, projstring) != 0) return MS_FAILURE;
    msProjectRect(&proj, NULL, &(cm->llextent));
  }

  return MS_SUCCESS;
}
#endif

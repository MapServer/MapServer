#include "map.h"
#include "maperror.h"
#include "mapgml.h"
#include "gdfonts.h"

/*
** msIsLayerQueryable()  returns MS_TRUE/MS_FALSE
*/
int msIsLayerQueryable(layerObj *lp)
{
  int i, is_queryable = MS_FALSE;

  for(i=0; i<lp->numclasses; i++) {
    if(lp->class[i].template) {
      is_queryable = MS_TRUE;
      break;
    }
  }

  return is_queryable;
}


#ifdef USE_WMS

/*
** msWMSException()
**
** Report current MapServer error in requested format.
*/
static char *wms_exception_format=NULL;

int msWMSException(mapObj *map, const char *wmtversion) 
{
  // Establish default exception format depending on VERSION
  if (wms_exception_format == NULL) 
  {
      if (wmtversion && strcasecmp(wmtversion, "1.0.0") <= 0)
          wms_exception_format = "INIMAGE";  // WMS 1.0.0
      else
          wms_exception_format = "SE_XML";   // WMS 1.0.1 and later
  }

  if (strcasecmp(wms_exception_format, "INIMAGE") == 0 ||
      strcasecmp(wms_exception_format, "BLANK") == 0 )
  {
    gdFontPtr font = gdFontSmall;
    gdImagePtr img=NULL;
    int width=400, height=300, color, imagetype=MS_GIF, transparent=MS_FALSE;

    if (map) {
      width = map->width;
      height = map->height;
      imagetype = map->imagetype;
      transparent = map->transparent;
    }
    img = gdImageCreate(width, height);
    color = gdImageColorAllocate(img, 255,255,255);  // BG color
    color = gdImageColorAllocate(img, 0,0,0);        // Text color

    if (strcasecmp(wms_exception_format, "BLANK") != 0) {
      char errormsg[256];
      sprintf(errormsg, "%s: %s", ms_error.routine, ms_error.message);
      gdImageString(img, font, 5, height/2, errormsg, color);
    }

    printf("Content-type: image/%s%c%c", MS_IMAGE_MIME_TYPE(imagetype), 10,10);
    msSaveImage(img, NULL, imagetype, transparent, 0, -1);
    gdImageDestroy(img);

  }
  else if (strcasecmp(wms_exception_format, "WMS_XML") == 0) // Only in V1.0.0 
  {
    printf("Content-type: text/xml%c%c",10,10);
    printf("<WMTException version=\"1.0.0\">\n");
    msWriteError(stdout);
    printf("</WMTException>\n");
  }
  else  // SE_XML ... the default with V1.0.1 and later
  {
    printf("Content-type: text/xml%c%c",10,10);

    printf("<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\n");
    printf("<!DOCTYPE ServiceExceptionReport SYSTEM \"http://www.digitalearth.gov/wmt/xml/exception_1_0_1.dtd\">\n");

    printf("<ServiceExceptionReport version=\"1.0.1\">\n");
    printf("<ServiceException>\n");
    msWriteError(stdout);
    printf("</ServiceException>\n");
    printf("</ServiceExceptionReport>\n");
  }

  return MS_FAILURE; // so that we can call 'return msWMSException();' anywhere
}

/*
**
*/
int msWMSLoadGetMapParams(mapObj *map, const char *wmtver,
                          char **names, char **values, int numentries)
{
  int i;

  // Some of the getMap parameters are actually required depending on the 
  // request, but for now we assume all are optional and the map file 
  // defaults will apply.
  for(i=0; map && i<numentries; i++)
  {
    // getMap parameters
    if (strcasecmp(names[i], "LAYERS") == 0) 
    {
      char **layers;
      int numlayers, j, k;

      layers = split(values[i], ',', &numlayers);
      if (layers==NULL || numlayers < 1) {
        msSetError(MS_MISCERR, "At least one layer name required in LAYERS.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, wmtver);
      }

      for(j=0; j<map->numlayers; j++)
      {
        // Keep only layers with status=DEFAULT by default
        if (map->layers[j].status != MS_DEFAULT)
           map->layers[j].status = MS_OFF;

        for(k=0; k<numlayers; k++)
        {
          if (strcasecmp(map->layers[j].name, layers[k]) == 0)
            map->layers[j].status = MS_DEFAULT;
        }
      }

      msFreeCharArray(layers, numlayers);
    }
    else if (strcasecmp(names[i], "STYLES") == 0) {
      // __TODO__
    }
    else if (strcasecmp(names[i], "SRS") == 0) {
      char **tokens;
      int n;
      // SRS is in format "EPSG:..." or "AUTO:..."
      // For now we support only "EPSG:<epsg_code>" format
      tokens = split(values[i], ':', &n);
      if (tokens==NULL || n != 2) {
        msSetError(MS_MISCERR, "Wrong number of arguments for SRS.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, wmtver);
      }

      if (strcasecmp(tokens[0], "EPSG") == 0) {
        char buffer[20];
        tokens[1][10] = '\0'; // Just in case
        sprintf(buffer, "+init=epsg:%s", tokens[1]);

        if (msLoadProjectionString(&(map->projection), buffer) != 0)
          return msWMSException(map, wmtver);
      }
      else {
        msSetError(MS_MISCERR, 
                   "Unsupported SRS namespace (only EPSG currently supported).",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, wmtver);
      }
      msFreeCharArray(tokens, n);
    }
    else if (strcasecmp(names[i], "BBOX") == 0) {
      char **tokens;
      int n;
      tokens = split(values[i], ',', &n);
      if (tokens==NULL || n != 4) {
        msSetError(MS_MISCERR, "Wrong number of arguments for BBOX.",
                   "msWMSLoadGetMapParams()");
        return msWMSException(map, wmtver);
      }
      map->extent.minx = atof(tokens[0]);
      map->extent.miny = atof(tokens[1]);
      map->extent.maxx = atof(tokens[2]);
      map->extent.maxy = atof(tokens[3]);
      msFreeCharArray(tokens, n);
    }
    else if (strcasecmp(names[i], "WIDTH") == 0) {
      map->width = atoi(values[i]);
    }
    else if (strcasecmp(names[i], "HEIGHT") == 0) {
      map->height = atoi(values[i]);
    }
    else if (strcasecmp(names[i], "FORMAT") == 0) {
      if (strcasecmp(values[i], "GIF") == 0)
        map->imagetype = MS_GIF;
      else if (strcasecmp(values[i], "JPEG") == 0)
        map->imagetype = MS_JPEG;
      else if (strcasecmp(values[i], "PNG") == 0)
        map->imagetype = MS_PNG;
      else if (strcasecmp(values[i], "WBMP") == 0)
        map->imagetype = MS_WBMP;
      else {
        msSetError(MS_IMGERR, 
                   "Unsupported output format.", "msWMSLoadGetMapParams()");
        return msWMSException(map, wmtver);
      }
    }
    else if (strcasecmp(names[i], "TRANSPARENT") == 0) {
      map->transparent = (strcasecmp(values[i], "TRUE") == 0);
    }
    else if (strcasecmp(names[i], "BGCOLOR") == 0) {
      long c;
      c = strtol(values[i], NULL, 16);
      map->imagecolor.red = (c/0x10000)&0xff;
      map->imagecolor.green = (c/0x100)&0xff;
      map->imagecolor.blue = c&0xff;
    }
  }

  return MS_SUCCESS;
}


/*
**
*/
static void printRequestCap(const char *wmtver, const char *request, 
                         const char *script_url, const char *formats)
{
  printf("    <%s>\n", request);
  printf("      <Format>%s</Format>\n", formats);
  printf("      <DCPType>\n");
  printf("        <HTTP>\n");

  if (strcasecmp(wmtver, "1.0.0") == 0) {
    printf("          <Get onlineResource=\"%s\" />\n", script_url);
    printf("          <Post onlineResource=\"%s\" />\n", script_url);
  } else {
    printf("          <Get><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/></Get>\n", script_url);
    printf("          <Post><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/></Post>\n", script_url);
  }

  printf("        </HTTP>\n");
  printf("      </DCPType>\n");
  printf("    </%s>\n", request);
}

/*
** msWMSGetEPSGProj()
**
** Extract projection code for this layer/map.  First look for an EPSG code
** in projectionObj, if not found then look for "wms_proj" metadata... 
** and if not found then return NULL.
*/
const char *msWMSGetEPSGProj(projectionObj *proj, hashTableObj metadata)
{
  static char epsgCode[20] ="";
  static char *value;

  if (proj && proj->numargs > 0 && 
      (value = strstr(proj->args[0], "init=epsg:")) != NULL &&
      strlen(value) < 20)
  {
    sprintf(epsgCode, "EPSG:%s", value+10);
    return epsgCode;
  }
  else if ((value = msLookupHashTable(metadata, "wms_proj")) != NULL)
  {
    return value;  // Value should be in format "EPSG:n" or "AUTO:..."
  }

  return NULL;
}

/*
**
*/
void msWMSPrintLatLonBoundingBox(const char *tabspace, 
                                 rectObj *extent, projectionObj *srcproj) 
{
  rectObj ll_ext;
  ll_ext = *extent;

  if (srcproj->numargs > 0 && !pj_is_latlong(srcproj->proj)) {
    msProjectRect(srcproj, NULL, &ll_ext);
  }

  printf("%s<LatLonBoundingBox minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\" />\n", 
         tabspace, ll_ext.minx, ll_ext.miny, ll_ext.maxx, ll_ext.maxy);
}

/*
** msWMSGetLayerExtent()
**
** Try to establish layer extent, first looking for "extent" metadata, and
** if not found then open layer to read extent.
**
** __TODO__ Replace metadata with EXTENT param in layerObj???
*/
int msWMSGetLayerExtent(mapObj *map, layerObj *lp, rectObj *ext)
{
  static char *value;

  if ((value = msLookupHashTable(lp->metadata, "extent")) != NULL)
  {
    char **tokens;
    int n;

    tokens = split(value, ' ', &n);
    if (tokens==NULL || n != 4) {
      msSetError(MS_MISCERR, "Wrong number of arguments for EXTENT metadata.",
                 "msWMSGetLayerExtent()");
      return MS_FAILURE;
    }
    ext->minx = atof(tokens[0]);
    ext->miny = atof(tokens[1]);
    ext->maxx = atof(tokens[2]);
    ext->maxy = atof(tokens[3]);

    msFreeCharArray(tokens, n);
  }
  else if (lp->type == MS_LAYER_RASTER)
  {
    // __TODO__ We need getExtent() for rasters... use metadata for now.
    return MS_FAILURE; 
  }
  else
  {
    if (msLayerOpen(lp, map->shapepath) == MS_SUCCESS) {
      int status;
      status = msLayerGetExtent(lp, ext);
      msLayerClose(lp);
      return status;
    }
  }

  return MS_FAILURE;
}


/*
** msWMSCapabilities()
*/
int msWMSCapabilities(mapObj *map, const char *wmtver) 
{
  int i;
  const char *value=NULL;
  const char *dtd_url = NULL;
  char *script_url=NULL;
  rectObj ext;

  layerObj *lp=NULL;

  // Decide which version we're going to return.  Either 1.0.0 or 1.0.7
  if (wmtver && strcasecmp(wmtver, "1.0.7") < 0) {
    wmtver = "1.0.0";
    dtd_url = "http://www.digitalearth.gov/wmt/xml/capabilities_1_0_0.dtd";
  }
  else {
    wmtver = "1.0.7";
    dtd_url = "http://www.digitalearth.gov/wmt/xml/capabilities_1_0_7.dtd";
  }

  // We need this script's URL, including hostname.
  // Default to use the value of the "onlineresource" metadata, and if not
  // set then build it: "http://$(SERVER_NAME):$(SERVER_PORT)$(SCRIPT_NAME)"
  if ((value = msLookupHashTable(map->web.metadata, "onlineresource"))) {
    script_url = strdup(value);
  }
  else {
    const char *hostname, *port, *script;
    hostname = getenv("SERVER_NAME");
    port = getenv("SERVER_PORT");
    script = getenv("SCRIPT_NAME");

    if (hostname && port && script) {
      script_url = (char*)malloc(sizeof(char)*(strlen(hostname)+strlen(port)+strlen(script)+10));
      if (script_url) sprintf(script_url, "http://%s:%s%s", hostname, port, script);
    }
    else {
      msSetError(MS_CGIERR, "Impossible to establish server URL.  Please set \"onlineresource\" metadata.", "msWMSCapabilities()");
      return msWMSException(map, wmtver);
    }
  }
  if (script_url == NULL) {
      msSetError(MS_MEMERR, NULL, "msWMSCapabilities");
      return msWMSException(map, wmtver);
  }


  printf("Content-type: text/xml%c%c",10,10);

  printf("<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\n");
  printf("<!DOCTYPE WMT_MS_Capabilities SYSTEM \"%s\"\n", dtd_url);
  printf(" [\n");

  // some mapserver specific declarations will go here
  printf(" <!ELEMENT VendorSpecificCapabilities EMPTY>\n");

  printf(" ]>  <!-- end of DOCTYPE declaration -->\n\n");

  printf("<WMT_MS_Capabilities version=\"%s\" updateSequence=\"0\">\n",wmtver);

  // WMS definition
  printf("<Service> <!-- a service IS a MapServer mapfile -->\n");
  printf("  <Name>GetMap</Name> <!-- WMT defined -->\n");

  // the majority of this section is dependent on appropriately named metadata in the WEB object
  if(map->web.metadata && (value = msLookupHashTable(map->web.metadata, "title"))) printf("  <Title>%s</Title>\n", value);
  if(map->web.metadata && (value = msLookupHashTable(map->web.metadata, "abstract"))) printf("  <Abstract>%s</Abstract>\n", value);
  if (strcasecmp(wmtver, "1.0.0") == 0)
    printf("  <OnlineResource>%s</OnlineResource>\n", script_url);
  else
    printf("  <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/>\n", script_url);
  if(map->web.metadata && (value = msLookupHashTable(map->web.metadata, "keywordlist"))) {
    char **keywords;
    int numkeywords;

    keywords = split(value, ',', &numkeywords);
    if(keywords && numkeywords > 0) {
      printf("  <KeywordList>\n");
      for(i=0; i<numkeywords; i++) printf("    <Keyword>%s</Keyword\n", keywords[i]);
      printf("  </KeywordList>\n");
      msFreeCharArray(keywords, numkeywords);
    }    
  }
  
  // contact information is a required element in 1.0.7+
  if (strcasecmp(wmtver, "1.0.0") != 0) {
    printf("  <ContactInformation>\n");
    if(map->web.metadata && msLookupHashTable(map->web.metadata, "ContactPerson") && msLookupHashTable(map->web.metadata, "ContactOrganization")) {
      printf("    <ContactPersonPrimary>\n");
      printf("      <ContactPerson>%s</ContactPerson>\n", msLookupHashTable(map->web.metadata, "ContactPerson"));
      printf("      <ContactOrganization>%s</ContactOrganization>\n", msLookupHashTable(map->web.metadata, "ContactOrganization"));
      printf("    </ContactPersonPrimary>\n");
    }
    if(map->web.metadata && (value = msLookupHashTable(map->web.metadata, "contactposition"))) printf("    <ContactPosition>%s</ContactPosition>\n", value);
    if(map->web.metadata && msLookupHashTable(map->web.metadata, "addresstype") && 
       msLookupHashTable(map->web.metadata, "address") && msLookupHashTable(map->web.metadata, "city") &&
       msLookupHashTable(map->web.metadata, "stateorprovince") && msLookupHashTable(map->web.metadata, "postcode") &&
       msLookupHashTable(map->web.metadata, "country")) {
      printf("    <ContactAddress>\n");
      printf("      <AddressType>%s</AddressType>\n", msLookupHashTable(map->web.metadata, "addresstype"));
      printf("      <Address>%s</Address>\n", msLookupHashTable(map->web.metadata, "address"));
      printf("      <City>%s</City>\n", msLookupHashTable(map->web.metadata, "City"));
      printf("      <StateOrProvince>%s</StateOrProvince>\n", msLookupHashTable(map->web.metadata, "StateOrProvince"));
      printf("      <PostCode>%s</PostCode>\n", msLookupHashTable(map->web.metadata, "PostCode"));
      printf("      <Country>%s</County>\n", msLookupHashTable(map->web.metadata, "Country"));
      printf("    </ContactAddress>\n");
    }
    if(map->web.metadata && (value = msLookupHashTable(map->web.metadata, "ContactVoiceTelephone"))) printf("    <ContactVoiceTelephone>%s</ContactVoiceTelephone>\n", value);
    if(map->web.metadata && (value = msLookupHashTable(map->web.metadata, "ContactFacsimileTelephone"))) printf("    <ContactFacsimileTelephone>%s</ContactFacsimileTelephone>\n", value);
    if(map->web.metadata && (value = msLookupHashTable(map->web.metadata, "ContactElectronicMailAddress"))) printf("    <ContactElectronicMailAddress>%s</ContactElectronicMailAddress>\n", value);
    printf("  </ContactInformation>\n");
  }

  if(map->web.metadata && (value = msLookupHashTable(map->web.metadata, "accessconstraints"))) printf("  <AccessConstraints>%s</AccessConstraints>\n", value);
  if(map->web.metadata && (value = msLookupHashTable(map->web.metadata, "fees"))) printf("  <Fees>%s</Fees>\n", value);

  printf("</Service>\n\n");

  // WMS capabilities definitions (note: will probably need something even more portable than SCRIPT_NAME in non-cgi environments)
  printf("<Capability>\n");
  printf("  <Request>\n");

  printRequestCap(wmtver, "Map", script_url, 
#ifdef USE_GD_GIF
                      "<GIF />"
#endif
#ifdef USE_GD_JPEG
                      "<JPEG />"
#endif
#ifdef USE_GD_PNG
                      "<PNG />"
#endif
#ifdef USE_GD_WBMP
                      "<WBMP />"
#endif
                      );
  printRequestCap(wmtver, "Capabilities", script_url, "<WMS_XML />");
  printRequestCap(wmtver, "FeatureInfo", script_url, "<MIME />");

  printf("  </Request>\n");

  printf("  <Exception>\n");
  printf("    <Format><BLANK /><INIMAGE /><WMS_XML /></Format>\n");
  printf("  </Exception>\n");


  printf("  <VendorSpecificCapabilities />\n"); // nothing yet

  // Top-level layer with map extents and SRS, encloses all map layers
  printf("  <Layer>\n"); 
  if((value = msLookupHashTable(map->web.metadata, "title")) || (value == map->name)) printf("    <Title>%s</Title>\n", value);
  if((value = msLookupHashTable(map->web.metadata, "wms_all_proj"))) printf("    <SRS>%s</SRS>\n", value);

  msWMSPrintLatLonBoundingBox("    ", &(map->extent), &(map->projection));
  if((value = msWMSGetEPSGProj(&(map->projection), map->web.metadata))) printf("    <BoundingBox SRS=\"%s\" minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\" />\n", value, map->extent.minx, map->extent.miny, map->extent.maxx, map->extent.maxy);

  for(i=0; i<map->numlayers; i++) {
    lp = &(map->layers[i]);

    printf("    <Layer queryable=\"%d\">\n", msIsLayerQueryable(lp));
    printf("      <Name>%s</Name>\n", lp->name);

    // the majority of this section is dependent on appropriately named metadata in the LAYER object
    if((value = msLookupHashTable(lp->metadata, "title")) || (value = lp->name)) printf("      <Title>%s</Title>\n", value);
    if((value = msLookupHashTable(lp->metadata, "abstract"))) printf("      <Abstract>%s</Abstract>\n", value);
    if((value = msLookupHashTable(lp->metadata, "keywordlist"))) {
      char **keywords;
      int numkeywords;
      
      keywords = split(value, ',', &numkeywords);
      if(keywords && numkeywords > 0) {
	printf("      <KeywordList>\n");
	for(i=0; i<numkeywords; i++) printf("        <Keyword>%s</Keyword\n", keywords[i]);
	printf("      </KeywordList>\n");
	msFreeCharArray(keywords, numkeywords);
      }    
    }

    if((value = msLookupHashTable(lp->metadata, "wms_all_proj"))) printf("      <SRS>%s</SRS>\n", value);
    if (msWMSGetLayerExtent(map, lp, &ext) == MS_SUCCESS)
    {
      if(lp->projection.numargs > 0) {
	msWMSPrintLatLonBoundingBox("      ", &(ext), &(lp->projection));
	if((value = msWMSGetEPSGProj(&(lp->projection), lp->metadata))) printf("      <BoundingBox SRS=\"%s\" minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\" />\n", value, ext.minx, ext.miny, ext.maxx, ext.maxy);
      } else {
	msWMSPrintLatLonBoundingBox("      ", &(ext), &(map->projection));
	if((value = msWMSGetEPSGProj(&(map->projection), map->web.metadata))) printf("      <BoundingBox SRS=\"%s\" minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\" />\n", value, ext.minx, ext.miny, ext.maxx, ext.maxy);
      }
    }
    
    printf("    </Layer>\n");
  }

  printf("  </Layer>\n");

  printf("</Capability>\n");
  printf("</WMT_MS_Capabilities>\n");

  free(script_url);

  return(MS_SUCCESS);
}

/*
** msWMSGetMap()
*/
int msWMSGetMap(mapObj *map, const char *wmtver) 
{
  gdImagePtr img;

  // __TODO__ msDrawMap() will try to adjust the extent of the map
  // to match the width/height image ratio.
  // The spec states that this should not happen so that we can deliver
  // maps to devices with non-square pixels.

  img = msDrawMap(map);
  if (img == NULL)
      return msWMSException(map, wmtver);

  printf("Content-type: image/%s%c%c", MS_IMAGE_MIME_TYPE(map->imagetype), 10,10);
  if (msSaveImage(img, NULL, map->imagetype, map->transparent, 
                  map->interlace, map->imagequality) != MS_SUCCESS)
      return msWMSException(map, wmtver);

  gdImageDestroy(img);

  return(MS_SUCCESS);
}

/*
** msWMSFeatureInfo()
*/
int msWMSFeatureInfo(mapObj *map, const char *wmtver, 
                     char **names, char **values, int numentries) 
{
  int i, feature_count=1, numlayers_found=0;
  pointObj point = {-1.0, -1.0};
  const char *info_format="MIME";

  for(i=0; map && i<numentries; i++)
  {
    if (strcasecmp(names[i], "QUERY_LAYERS") == 0) 
    {
      char **layers;
      int numlayers, j, k;

      layers = split(values[i], ',', &numlayers);
      if (layers==NULL || numlayers < 1) {
        msSetError(MS_MISCERR, 
                   "At least one layer name required in QUERY_LAYERS.",
                   "msWMSFeatureInfo()");
        return msWMSException(map, wmtver);
      }

      for(j=0; j<map->numlayers; j++)
      {
        // Force all layers OFF by default
         map->layers[j].status = MS_OFF;

        for(k=0; k<numlayers; k++)
        {
            if (strcasecmp(map->layers[j].name, layers[k]) == 0) {
            map->layers[j].status = MS_ON;
            numlayers_found++;
          }
        }
      }

      msFreeCharArray(layers, numlayers);
    }
    else if (strcasecmp(names[i], "INFO_FORMAT") == 0) 
    {
        info_format = values[i];
    }
    else if (strcasecmp(names[i], "FEATURE_COUNT") == 0) 
    {
        feature_count = atoi(values[i]);
    }
    else if (strcasecmp(names[i], "X") == 0) 
    {
        point.x = atof(values[i]);
    }
    else if (strcasecmp(names[i], "Y") == 0) 
    {
        point.y = atof(values[i]);
    }
  }

  if (numlayers_found == 0) {
      msSetError(MS_MISCERR, 
                 "Required QUERY_LAYERS parameter missing for getFeatureInfo.",
                 "msWMSFeatureInfo()");
      return msWMSException(map, wmtver);
  }

  if (point.x == -1.0 || point.y == -1.0) {
      msSetError(MS_MISCERR, 
                 "Required X/Y parameters missing for getFeatureInfo.",
                 "msWMSFeatureInfo()");
      return msWMSException(map, wmtver);
  }

  // Perform the actual query

  // __TODO__ For now we let MapServer adjust map extent, but the spec
  // says that we should not adjust the extent to fit image aspect ratio
  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);

  point.x = map->extent.minx + map->cellsize*point.x;
  point.y = map->extent.maxy - map->cellsize*point.y;

  if(msQueryByPoint(map, -1, (feature_count==1?MS_SINGLE:MS_MULTIPLE), 
                    point, 0) != MS_SUCCESS) 
  {
    if (ms_error.code != MS_NOTFOUND)
      return msWMSException(map, wmtver);
  }

  // Generate response
  if (strcasecmp(info_format, "MIME") == 0) {

    // MIME response... we're free to use any valid MIME type
    int numresults = 0;

    printf("Content-type: text/plain%c%c", 10,10);
    printf("GetFeatureInfo results:\n");

    for(i=0; i<map->numlayers && numresults<feature_count; i++)
    {
      int j, k;
      layerObj *lp;
      lp = &(map->layers[i]);

      if (lp->status != MS_ON || 
          lp->resultcache==NULL || lp->resultcache->numresults == 0) 
        continue;

      if (msLayerOpen(lp, map->shapepath) != MS_SUCCESS ||
          msLayerGetItems(lp) != MS_SUCCESS)
        return msWMSException(map, wmtver);

      printf("\nLayer '%s'\n", lp->name);

      for(j=0; j<lp->resultcache->numresults && numresults<feature_count; j++)
      {
        shapeObj shape;
        msInitShape(&shape);
        if (msLayerGetShape(lp, &shape, lp->resultcache->results[j].tileindex,
                            lp->resultcache->results[j].shapeindex) != MS_SUCCESS)
          return msWMSException(map, wmtver);

        printf("  Feature %ld: \n", lp->resultcache->results[j].shapeindex);
        
        for(k=0; k<lp->numitems; k++)
        {
            printf("    %s = '%s'\n", lp->items[k], shape.values[k]);
        }

        msFreeShape(&shape);
        numresults++;
      }

      msLayerClose(lp);
    }

    if (numresults == 0) printf("\n  Search returned no results.\n");

  } else if (strcasecmp(info_format, "GML") == 0) {
    int numresults=0;

    printf("Content-type: text/xml%c%c", 10,10);
    
    msGMLStart(stdout, "2.0", map->name, NULL, NULL); // fix the NULL values, 2.0 is the GML version

    for(i=0; i<map->numlayers && numresults<feature_count; i++) {
      int j;
      layerObj *lp;
      lp = &(map->layers[i]);

      if(lp->status != MS_ON || lp->resultcache==NULL || lp->resultcache->numresults == 0)
        continue;

      if(msLayerOpen(lp, map->shapepath) != MS_SUCCESS || msLayerGetItems(lp) != MS_SUCCESS)
        return msWMSException(map, wmtver);

      for(j=0; j<lp->resultcache->numresults && numresults<feature_count; j++) {
        shapeObj shape;
        msInitShape(&shape);

        if(msLayerGetShape(lp, &shape, lp->resultcache->results[j].tileindex, lp->resultcache->results[j].shapeindex) != MS_SUCCESS)
          return msWMSException(map, wmtver);

	if(msGMLWriteShape(stdout, &shape, NULL, NULL) != MS_SUCCESS)
	  return msWMSException(map, wmtver);

        msFreeShape(&shape);
        numresults++;
      }

      msLayerClose(lp);
    }

    if (numresults == 0) printf("<!-- Search returned no results. -->\n");

    msGMLFinish(stdout, map->name);

  } else {
    msSetError(MS_MISCERR, "Unsupported INFO_FORMAT value.", "msWMSFeatureInfo()");
    return msWMSException(map, wmtver);
  }

  return(MS_SUCCESS);
}

#endif /* USE_WMS */

/*
** msWMSDispatch() is the entry point for WMS requests.
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid WMS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msWMSDispatch(mapObj *map, char **names, char **values, int numentries) 
{
#ifdef USE_WMS
  int i, status;
  static char *wmtver = NULL, *request=NULL, *service=NULL;

  /*
  ** Process Params common to all requests
  */
  // VERSION (WMTVER in 1.0.0) and REQUEST must be present in a valid request
  for(i=0; i<numentries; i++)
  {
    if (strcasecmp(names[i], "WMTVER") == 0 ||
        strcasecmp(names[i], "VERSION") == 0) {
      wmtver = values[i];
    }
    else if (strcasecmp(names[i], "REQUEST") == 0) {
      request = values[i];
    }
    else if (strcasecmp(names[i], "EXCEPTIONS") == 0) {
      wms_exception_format = values[i];
    }
    else if (strcasecmp(names[i], "SERVICE") == 0) {
      service = values[i];
    }
  }

  /*
  ** Dispatch request... we should probably do some validation on VERSION here
  ** vs the versions we actually support.
  */
  if ((service == NULL || strcasecmp(service, "WMS") == 0) &&
      request && (strcasecmp(request, "capabilities") == 0 ||
                  strcasecmp(request, "GetCapabilities") == 0) ) 
  {
      if (!wmtver) 
          wmtver = "1.0.7";  // VERSION is optional with getCapabilities only
      return msWMSCapabilities(map, wmtver);
  }

  // VERSION *and* REQUEST required by both getMap and getFeatureInfo
  if (wmtver==NULL || request==NULL) return MS_DONE; // Not a WMS request

  // getMap parameters are used by both getMap and getFeatureInfo
  status = msWMSLoadGetMapParams(map, wmtver, names, values, numentries);
  if (status != MS_SUCCESS) return status;

  if (strcasecmp(request, "map") == 0 ||
      strcasecmp(request, "GetMap") == 0) {
      return msWMSGetMap(map, wmtver);
  }
  else if (strcasecmp(request, "feature_info") == 0 ||
           strcasecmp(request, "GetFeatureInfo") == 0) {
      return msWMSFeatureInfo(map, wmtver, names, values, numentries);
  }

  // Hummmm... incomplete or unsupported WMS request
  msSetError(MS_MISCERR, "Incomplete or unsupported WMS request",
             "msWMSDispatch()");
  return msWMSException(map, wmtver);

#else

  msSetError(MS_MISCERR, "WMS support is not available.", "msWMSDispatch()");
  return(MS_FAILURE);

#endif
}


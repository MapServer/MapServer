/**********************************************************************
 * $Id$
 *
 * mapows.c - OGC Web Services (WMS, WFS) support functions
 *
 **********************************************************************
 * $Log$
 * Revision 1.23  2004/03/30 00:04:49  dan
 * Cleaned up changelog
 *
 * Revision 1.22  2004/02/26 18:38:08  frank
 * fixed last change
 *
 * Revision 1.21  2004/02/26 16:13:24  frank
 * Correct msOWSGetLayerExtent() error message.
 *
 * Revision 1.20  2004/02/26 16:08:49  frank
 * Added check for wcs_extent.
 *
 * Revision 1.19  2004/02/24 06:20:37  sdlime
 * Added msOWSGetMetadata() function.
 *
 * Revision 1.18  2004/02/05 04:40:01  sdlime
 * Added WCS to the OWS request broker function. The WCS request handler 
 * just returns MS_DONE for now.
 *
 * Revision 1.17  2003/09/19 21:54:19  assefa
 * Add support fot the Post request.
 *
 * Revision 1.16  2003/07/29 20:24:18  dan
 * Fixed problem with invalid BoundingBox tag in WMS capabilities (bug 34)
 *
 * Revision 1.15  2003/02/05 04:40:10  sdlime
 * Removed shapepath as an argument from msLayerOpen and msSHPOpenFile. The 
 * shapefile opening routine now expects just a filename. So, you must use 
 * msBuildPath or msBuildPath3 to create a full qualified filename. Relatively
 * simple change, but required lots of changes. Demo still works...
 *
 * Revision 1.14  2003/01/10 06:39:06  sdlime
 * Moved msEncodeHTMLEntities() and msDecodeHTMLEntities() from mapows.c to
 * mapstring.c so they can be used a bit more freely.
 *
 * Revision 1.13  2002/12/20 03:43:03  frank
 * ensure this builds without libcurl
 *
 * Revision 1.12  2002/12/19 06:30:59  dan
 * Enable caching WMS/WFS request using tmp filename built from URL
 *
 * Revision 1.11  2002/12/19 05:17:09  dan
 * Report WFS exceptions, and do not fail on WFS requests returning 0 features
 *
 * Revision 1.10  2002/12/18 16:45:49  dan
 * Fixed WFS capabilities to validate against schema
 *
 * Revision 1.9  2002/12/13 00:57:31  dan
 * Modified WFS implementation to behave more as a real vector data source
 *
 * Revision 1.8  2002/11/20 21:22:32  dan
 * Added msOWSGetSchemasLocation() for use by both WFS and WMS Map Context
 *
 * Revision 1.7  2002/11/20 17:17:21  julien
 * Support version 0.1.2 of MapContext
 * Remove warning from tags
 * Encode and decode all url
 *
 * Revision 1.6  2002/11/19 02:27:04  dan
 * Fixed unterminated buffer in msEncodeHTMLEntities()
 *
 * Revision 1.5  2002/11/07 21:16:45  julien
 * Fix warning in ContactInfo
 *
 * Revision 1.4  2002/11/07 15:46:45  julien
 * Print ContactInfo just if necessary
 *
 * Revision 1.3  2002/10/28 20:31:20  dan
 * New support for WMS Map Context (from Julien)
 *
 * Revision 1.2  2002/10/22 20:03:57  julien
 * Add the mapcontext support
 *
 * Revision 1.2  2002/10/04 21:29:41  dan
 * WFS: Added GetCapabilities and basic GetFeature (still some work to do)
 *
 * Revision 1.1  2002/09/03 03:19:51  dan
 * Set the bases for WFS Server support + moved some WMS/WFS stuff to mapows.c
 *
 **********************************************************************/

#include "map.h"

#include <ctype.h> /* isalnum() */
#include <stdarg.h> 

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR)

/*
** msOWSDispatch() is the entry point for any OWS request (WMS, WFS, ...)
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid OWS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msOWSDispatch(mapObj *map, cgiRequestObj *request)
{
    int status = MS_DONE;

    if (!request)
      return status;

#ifdef USE_WMS_SVR
    if ((status = msWMSDispatch(map, request->ParamNames, request->ParamValues,
                                request->NumParams)) != MS_DONE )
        return status;
#endif
#ifdef USE_WFS_SVR
    if ((status = msWFSDispatch(map, request)) != MS_DONE )
        return status;
#endif
#ifdef USE_WCS_SVR
    if ((status = msWCSDispatch(map, request)) != MS_DONE )
        return status;
#endif

    return MS_DONE;  /* Not a WMS/WFS request... let MapServer handle it */
}

const char *msOWSGetMetadata(hashTableObj metadata, ...) 
{
  va_list arglist;
  char *arg;
  const char *value;

  va_start(arglist, metadata);
  while((arg = va_arg(arglist, char *)) != NULL) {
    if((value = msLookupHashTable(metadata, (char*) arg))) {
      va_end(arglist);
      return(value);
    }
  }

  // didn't find the right metadata
  va_end(arglist);
  return(NULL);
}

/*
** msRenameLayer()
*/
static int msRenameLayer(layerObj *lp, int count)
{
    char *newname;
    newname = (char*)malloc((strlen(lp->name)+5)*sizeof(char));
    if (!newname) 
    {
        msSetError(MS_MEMERR, NULL, "msRenameLayer()");
        return MS_FAILURE;
    }
    sprintf(newname, "%s_%2.2d", lp->name, count);
    free(lp->name);
    lp->name = newname;
    
    return MS_SUCCESS;
}

/*
** msOWSMakeAllLayersUnique()
*/
int msOWSMakeAllLayersUnique(mapObj *map)
{
  int i, j;

  // Make sure all layers in the map file have valid and unique names
  for(i=0; i<map->numlayers; i++)
  {
      int count=1;
      for(j=i+1; j<map->numlayers; j++)
      {
          if (map->layers[i].name == NULL || map->layers[j].name == NULL)
          {
              msSetError(MS_MISCERR, 
                         "At least one layer is missing a name in map file.", 
                         "msOWSMakeAllLayersUnique()");
              return MS_FAILURE;
          }
          if (strcasecmp(map->layers[i].name, map->layers[j].name) == 0 &&
              msRenameLayer(&(map->layers[j]), ++count) != MS_SUCCESS)
          {
              return MS_FAILURE;
          }
      }

      // Don't forget to rename the first layer if duplicates were found
      if (count > 1 && msRenameLayer(&(map->layers[i]), 1) != MS_SUCCESS)
      {
          return MS_FAILURE;
      }
  }
  return MS_SUCCESS;
}


/*
** msOWSGetOnlineResource()
**
** Return the online resource for this service.  First try to lookup 
** specified metadata, and if not found then try to build the URL ourselves.
**
** Returns a newly allocated string that should be freed by the caller or
** NULL in case of error.
*/
char * msOWSGetOnlineResource(mapObj *map, const char *metadata_name)
{
    const char *value;
    char *online_resource = NULL;

    // We need this script's URL, including hostname.
    // Default to use the value of the "onlineresource" metadata, and if not
    // set then build it: "http://$(SERVER_NAME):$(SERVER_PORT)$(SCRIPT_NAME)?"
    if ((value = msLookupHashTable(map->web.metadata, (char*)metadata_name))) 
    {
        online_resource = (char*) malloc(strlen(value)+2);

        // Append trailing '?' or '&' if missing.
        strcpy(online_resource, value);
        if (strchr(online_resource, '?') == NULL)
            strcat(online_resource, "?");
        else
        {
            char *c;
            c = online_resource+strlen(online_resource)-1;
            if (*c != '?' && *c != '&')
                strcpy(c+1, "&");
        }
    }
    else 
    {
        const char *hostname, *port, *script, *protocol="http";
        hostname = getenv("SERVER_NAME");
        port = getenv("SERVER_PORT");
        script = getenv("SCRIPT_NAME");

        // HTTPS is set by Apache to "on" in an HTTPS server ... if not set
        // then check SERVER_PORT: 443 is the default https port.
        if ( ((value=getenv("HTTPS")) && strcasecmp(value, "on") == 0) ||
             ((value=getenv("SERVER_PORT")) && atoi(value) == 443) )
        {
            protocol = "https";
        }

        if (hostname && port && script) {
            online_resource = (char*)malloc(sizeof(char)*(strlen(hostname)+strlen(port)+strlen(script)+10));
            if (online_resource) 
                sprintf(online_resource, "%s://%s:%s%s?", protocol, hostname, port, script);
        }
        else 
        {
            msSetError(MS_CGIERR, "Impossible to establish server URL.  Please set \"%s\" metadata.", "msWMSCapabilities()", metadata_name);
            return NULL;
        }
    }

    if (online_resource == NULL) 
    {
        msSetError(MS_MEMERR, NULL, "msWMSCapabilities");
        return NULL;
    }

    return online_resource;
}


/* msOWSGetSchemasLocation()
**
** schemas location is the root of the web tree where all WFS-related 
** schemas can be found on this server.  These URLs must exist in order 
** to validate xml.
**
** Use value of "ows_schemas_location" metadata, if not set then
** return ".." as a default
*/
const char *msOWSGetSchemasLocation(mapObj *map)
{
    const char *schemas_location;

    schemas_location = msLookupHashTable(map->web.metadata, 
                                         "ows_schemas_location");
    if (schemas_location == NULL)
        schemas_location = "..";

    return schemas_location;
}



/*
** msOWSPrintMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not 
** found then one of a number of predefined actions will be taken. 
** If a default value is provided and metadata is absent then the 
** default will be used.
*/

int msOWSPrintMetadata(FILE *stream, hashTableObj metadata, const char *name, 
                       int action_if_not_found, const char *format, 
                       const char *default_value) 
{
    const char *value;
    int status = MS_NOERR;

    if((value = msLookupHashTable(metadata, (char*)name)))
    { 
        fprintf(stream, format, value);
    }
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            fprintf(stream, "<!-- WARNING: Mandatory metadata '%s' was missing in this context. -->\n", name);
            status = action_if_not_found;
        }

        if (default_value)
            fprintf(stream, format, default_value);
    }

    return status;
}

/*
** msOWSPrintEncodeMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not 
** found then one of a number of predefined actions will be taken. 
** If a default value is provided and metadata is absent then the 
** default will be used.
** Also encode the value with msEncodeHTMLEntities.
*/

int msOWSPrintEncodeMetadata(FILE *stream, hashTableObj metadata, 
                             const char *name, int action_if_not_found, 
                             const char *format, const char *default_value) 
{
    const char *value;
    char * pszEncodedValue=NULL;
    int status = MS_NOERR;

    if((value = msLookupHashTable(metadata, (char*)name)))
    {
        pszEncodedValue = msEncodeHTMLEntities(value);
        fprintf(stream, format, pszEncodedValue);
        free(pszEncodedValue);
    }
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            fprintf(stream, "<!-- WARNING: Mandatory metadata '%s' was missing in this context. -->\n", name);
            status = action_if_not_found;
        }

        if (default_value)
        {
            pszEncodedValue = msEncodeHTMLEntities(default_value);
            fprintf(stream, format, default_value);
            free(pszEncodedValue);
        }
    }

    return status;
}

/*
** msOWSPrintGroupMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not 
** found then one of a number of predefined actions will be taken. 
** If a default value is provided and metadata is absent then the 
** default will be used.
*/

int msOWSPrintGroupMetadata(FILE *stream, mapObj *map, char* pszGroupName, 
                            const char *name, int action_if_not_found, 
                            const char *format, const char *default_value) 
{
    const char *value;
    int status = MS_NOERR;
    int i;

    for (i=0; i<map->numlayers; i++)
    {
       if (map->layers[i].group && (strcmp(map->layers[i].group, pszGroupName) == 0) && map->layers[i].metadata)
       {
         if((value = msLookupHashTable(map->layers[i].metadata, (char*)name)))
         { 
            fprintf(stream, format, value);
            return status;
         }
       }
    }

    if (action_if_not_found == OWS_WARN)
    {
       fprintf(stream, "<!-- WARNING: Mandatory metadata '%s' was missing in this context. -->\n", name);
       status = action_if_not_found;
    }

    if (default_value)
       fprintf(stream, format, default_value);
   
    return status;
}

/* msOWSPrintParam()
**
** Same as printMetadata() but applied to mapfile parameters.
**/
int msOWSPrintParam(FILE *stream, const char *name, const char *value, 
                    int action_if_not_found, const char *format, 
                    const char *default_value) 
{
    int status = MS_NOERR;

    if(value && strlen(value) > 0)
    { 
        fprintf(stream, format, value);
    }
    else
    {
        if (action_if_not_found == OWS_WARN)
        {
            fprintf(stream, "<!-- WARNING: Mandatory mapfile parameter '%s' was missing in this context. -->\n", name);
            status = action_if_not_found;
        }

        if (default_value)
            fprintf(stream, format, default_value);
    }

    return status;
}

/* msOWSPrintMetadataList()
**
** Prints comma-separated lists metadata.  (e.g. keywordList)
**/
int msOWSPrintMetadataList(FILE *stream, hashTableObj metadata, 
                           const char *name, const char *startTag, 
                           const char *endTag, const char *itemFormat) 
{
    const char *value;
    if((value = msLookupHashTable(metadata, (char*)name))) 
    {
      char **keywords;
      int numkeywords;
      
      keywords = split(value, ',', &numkeywords);
      if(keywords && numkeywords > 0) {
        int kw;
	fprintf(stream, "%s", startTag);
	for(kw=0; kw<numkeywords; kw++) 
            fprintf(stream, itemFormat, keywords[kw]);
	fprintf(stream, "%s", endTag);
	msFreeCharArray(keywords, numkeywords);
      }
      return MS_TRUE;
    }
    return MS_FALSE;
}

/*
** msOWSPrintLatLonBoundingBox()
**
** Print a LatLonBoundingBox tag for WMS, or LatLongBoundingBox for WFS
** ... yes, the tag name differs between WMS and WFS, yuck!
**
*/
void msOWSPrintLatLonBoundingBox(FILE *stream, const char *tabspace, 
                                 rectObj *extent, projectionObj *srcproj,
                                 int nService)
{
  const char *pszTag = "LatLonBoundingBox";  /* The default for WMS */
  rectObj ll_ext;

  ll_ext = *extent;

  if (srcproj->numargs > 0 && !pj_is_latlong(srcproj->proj)) {
    msProjectRect(srcproj, NULL, &ll_ext);
  }

  if (nService == OWS_WFS)
      pszTag = "LatLongBoundingBox";

  fprintf(stream, "%s<%s minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\" />\n", 
         tabspace, pszTag, ll_ext.minx, ll_ext.miny, ll_ext.maxx, ll_ext.maxy);
}

/*
** Emit a bounding box if we can find projection information.
*/
void msOWSPrintBoundingBox(FILE *stream, const char *tabspace, 
                           rectObj *extent, 
                           projectionObj *srcproj,
                           hashTableObj metadata ) 
{
    const char	*value, *resx, *resy;

    /* Look for EPSG code in PROJECTION block only.  "wms_srs" metadata cannot be
     * used to establish the native projection of a layer for BoundingBox purposes.
     */
    value = msGetEPSGProj(srcproj, NULL, MS_TRUE);
    
    if( value != NULL )
    {
        fprintf(stream, "%s<BoundingBox SRS=\"%s\"\n"
               "%s            minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\"",
               tabspace, value, 
               tabspace, extent->minx, extent->miny, 
               extent->maxx, extent->maxy);

        if( ((resx = msLookupHashTable( metadata, "wms_resx" )) != NULL ||
             (resx = msLookupHashTable( metadata, "wfs_resx" )) != NULL )
            && ((resy = msLookupHashTable( metadata, "wms_resy" )) != NULL ||
                (resy = msLookupHashTable( metadata, "wfs_resy" )) != NULL ) )
            fprintf( stream, "\n%s            resx=\"%s\" resy=\"%s\"",
                    tabspace, resx, resy );
 
        fprintf( stream, " />\n" );
    }

}


/*
** Print the contact information
*/
void msOWSPrintContactInfo( FILE *stream, const char *tabspace, 
                           const char *wmtver, hashTableObj metadata )
{
  int bEnableContact = 0;

  // contact information is a required element in 1.0.7 but the
  // sub-elements such as ContactPersonPrimary, etc. are not!
  // In 1.1.0, ContactInformation becomes optional.
  if (strcasecmp(wmtver, "1.0.0") > 0) 
  {
    if(msLookupHashTable(metadata, "wms_contactperson") ||
       msLookupHashTable(metadata, "wms_contactorganization")) 
    {
      if(bEnableContact == 0)
      {
          fprintf(stream, "%s<ContactInformation>\n", tabspace);
          bEnableContact = 1;
      }

      // ContactPersonPrimary is optional, but when present then all its 
      // sub-elements are mandatory
      fprintf(stream, "%s  <ContactPersonPrimary>\n", tabspace);
      msOWSPrintMetadata(stream, metadata, "wms_contactperson", 
                  OWS_WARN, "      <ContactPerson>%s</ContactPerson>\n", NULL);
      msOWSPrintMetadata(stream, metadata, "wms_contactorganization", 
             OWS_WARN, "      <ContactOrganization>%s</ContactOrganization>\n",
             NULL);
      fprintf(stream, "%s  </ContactPersonPrimary>\n", tabspace);
    }

    if(bEnableContact == 0)
    {
        if(msOWSPrintMetadata(stream, metadata, "wms_contactposition", 
                           OWS_NOERR, 
     "    <ContactInformation>\n      <ContactPosition>%s</ContactPosition>\n",
                              NULL) != 0)
            bEnableContact = 1;
    }
    else
    {
        msOWSPrintMetadata(stream, metadata, "wms_contactposition", 
                    OWS_NOERR, "      <ContactPosition>%s</ContactPosition>\n",
                           NULL);
    }

    if(msLookupHashTable( metadata, "wms_addresstype" ) || 
       msLookupHashTable( metadata, "wms_address" ) || 
       msLookupHashTable( metadata, "wms_city" ) ||
       msLookupHashTable( metadata, "wms_stateorprovince" ) || 
       msLookupHashTable( metadata, "wms_postcode" ) ||
       msLookupHashTable( metadata, "wms_country" )) 
    {
      if(bEnableContact == 0)
      {
          fprintf(stream, "%s<ContactInformation>\n", tabspace);
          bEnableContact = 1;
      }

      // ContactAdress is optional, but when present then all its 
      // sub-elements are mandatory
      fprintf(stream, "%s  <ContactAddress>\n", tabspace);
      msOWSPrintMetadata(stream, metadata, "wms_addresstype", OWS_WARN,
                    "        <AddressType>%s</AddressType>\n", NULL);
      msOWSPrintMetadata(stream, metadata, "wms_address", OWS_WARN,
                    "        <Address>%s</Address>\n", NULL);
      msOWSPrintMetadata(stream, metadata, "wms_city", OWS_WARN,
                    "        <City>%s</City>\n", NULL);
      msOWSPrintMetadata(stream, metadata, "wms_stateorprovince", 
           OWS_WARN,"        <StateOrProvince>%s</StateOrProvince>\n", NULL);
      msOWSPrintMetadata(stream, metadata, "wms_postcode", OWS_WARN,
                    "        <PostCode>%s</PostCode>\n", NULL);
      msOWSPrintMetadata(stream, metadata, "wms_country", OWS_WARN,
                    "        <Country>%s</Country>\n", NULL);
      fprintf(stream, "%s  </ContactAddress>\n", tabspace);
    }


    if(bEnableContact == 0)
    {
        if(msOWSPrintMetadata(stream, metadata, "wms_contactvoicetelephone", 
                           OWS_NOERR,
                           "    <ContactInformation>\n      <ContactVoiceTelephone>%s</ContactVoiceTelephone>\n",
                              NULL) != 0)
            bEnableContact = 1;
    }
    else
    {
        msOWSPrintMetadata(stream, metadata, "wms_contactvoicetelephone", 
                           OWS_NOERR,
                   "      <ContactVoiceTelephone>%s</ContactVoiceTelephone>\n",
                           NULL);
    }

    if(bEnableContact == 0)
    {
        if(msOWSPrintMetadata(stream, metadata, 
                           "wms_contactfacsimiletelephone", OWS_NOERR,
                              "    <ContactInformation>\n     <ContactFacsimileTelephone>%s</ContactFacsimileTelephone>\n", NULL) != 0)
            bEnableContact = 1;
    }
    else
    {
        msOWSPrintMetadata(stream, metadata, 
                           "wms_contactfacsimiletelephone", OWS_NOERR,
                           "      <ContactFacsimileTelephone>%s</ContactFacsimileTelephone>\n", NULL);
    }

    if(bEnableContact == 0)
    {
        if(msOWSPrintMetadata(stream, metadata, 
                           "wms_contactelectronicmailaddress", OWS_NOERR,
                              "    <ContactInformation>\n     <ContactElectronicMailAddress>%s</ContactElectronicMailAddress>\n", NULL) != 0)
            bEnableContact = 1;
    }
    else
    {
        msOWSPrintMetadata(stream, metadata, 
                           "wms_contactelectronicmailaddress", OWS_NOERR,
                           "  <ContactElectronicMailAddress>%s</ContactElectronicMailAddress>\n", NULL);
    }


    if(bEnableContact == 1)
    {
        fprintf(stream, "%s</ContactInformation>\n", tabspace);
    }
  }
}

/*
** msOWSGetLayerExtent()
**
** Try to establish layer extent, first looking for "extent" metadata, and
** if not found then open layer to read extent.
**
** __TODO__ Replace metadata with EXTENT param in layerObj???
*/
int msOWSGetLayerExtent(mapObj *map, layerObj *lp, rectObj *ext)
{
  static char *value;

  if ((value = msLookupHashTable(lp->metadata, "wms_extent")) != NULL ||
      (value = msLookupHashTable(lp->metadata, "wfs_extent")) != NULL ||
      (value = msLookupHashTable(lp->metadata, "wcs_extent")) != NULL )
  {
    char **tokens;
    int n;

    tokens = split(value, ' ', &n);
    if (tokens==NULL || n != 4) {
      msSetError(MS_WMSERR, "Wrong number of arguments for EXTENT metadata.",
                 "msOWSGetLayerExtent()");
      return MS_FAILURE;
    }
    ext->minx = atof(tokens[0]);
    ext->miny = atof(tokens[1]);
    ext->maxx = atof(tokens[2]);
    ext->maxy = atof(tokens[3]);

    msFreeCharArray(tokens, n);
    return MS_SUCCESS;
  }
  else if (lp->type == MS_LAYER_RASTER)
  {
    // __TODO__ We need getExtent() for rasters... use metadata for now.
    return MS_FAILURE; 
  }
  else
  {
    if (msLayerOpen(lp) == MS_SUCCESS) {
      int status;
      status = msLayerGetExtent(lp, ext);
      msLayerClose(lp);
      return status;
    }
  }

  return MS_FAILURE;
}


/**********************************************************************
 *                          msOWSExecuteRequests()
 *
 * Execute a number of WFS/WMS HTTP requests in parallel, and then 
 * update layerObj information with the result of the requests.
 **********************************************************************/
int msOWSExecuteRequests(httpRequestObj *pasReqInfo, int numRequests,
                         mapObj *map, int bCheckLocalCache)
{
    int nStatus, iReq;

    // Execute requests
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
    nStatus = msHTTPExecuteRequests(pasReqInfo, numRequests, bCheckLocalCache);
#else
    msSetError(MS_WMSERR, "msOWSExecuteRequests() called apparently without libcurl configured, msHTTPExecuteRequests() not available.",
               "msOWSExecuteRequests()");
    return MS_FAILURE;
#endif

    // Scan list of layers and call the handler for each layer type to
    // pass them the request results.
    for(iReq=0; iReq<numRequests; iReq++)
    {
        if (pasReqInfo[iReq].nLayerId >= 0 && 
            pasReqInfo[iReq].nLayerId < map->numlayers)
        {
            layerObj *lp;

            lp = &(map->layers[pasReqInfo[iReq].nLayerId]);

            if (lp->connectiontype == MS_WFS)
                msWFSUpdateRequestInfo(lp, &(pasReqInfo[iReq]));
        }
    }

    return nStatus;
}

/**********************************************************************
 *                          msOWSProcessException()
 *
 **********************************************************************/
void msOWSProcessException(layerObj *lp, const char *pszFname, 
                           int nErrorCode, const char *pszFuncName)
{
    FILE *fp;

    if ((fp = fopen(pszFname, "r")) != NULL)
    {
        char *pszBuf=NULL;
        int   nBufSize=0;
        char *pszStart, *pszEnd;

        fseek(fp, 0, SEEK_END);
        nBufSize = ftell(fp);
        rewind(fp);
        pszBuf = (char*)malloc((nBufSize+1)*sizeof(char));
        if (pszBuf == NULL)
        {
            msSetError(MS_MEMERR, NULL, "msOWSProcessException()");
            fclose(fp);
            return;
        }

        if (fread(pszBuf, 1, nBufSize, fp) != nBufSize)
        {
            msSetError(MS_IOERR, NULL, "msOWSProcessException()");
            free(pszBuf);
            fclose(fp);
            return;
        }

        pszBuf[nBufSize] = '\0';


        // OK, got the data in the buffer.  Look for the <Message> tags
        if ((strstr(pszBuf, "<WFS_Exception>") &&            // WFS style
             (pszStart = strstr(pszBuf, "<Message>")) &&
             (pszEnd = strstr(pszStart, "</Message>")) ) ||
            (strstr(pszBuf, "<ServiceExceptionReport>") &&   // WMS style
             (pszStart = strstr(pszBuf, "<ServiceException>")) &&
             (pszEnd = strstr(pszStart, "</ServiceException>")) ))
        {
            pszStart = strchr(pszStart, '>')+1;
            *pszEnd = '\0';
            msSetError(nErrorCode, "Got Remote Server Exception for layer %s: %s",
                       pszFuncName, lp->name?lp->name:"(null)", pszStart);
        }
        else
        {
            msSetError(MS_WFSCONNERR, "Unable to parse Remote Server Exception Message for layer %s.",
                       pszFuncName, lp->name?lp->name:"(null)");
        }

        free(pszBuf);
        fclose(fp);
    }
}

/**********************************************************************
 *                          msOWSBuildURLFilename()
 *
 * Build a unique filename for this URL to use in caching remote server 
 * requests.  Slashes and illegal characters will be turned into '_'
 *
 * Returns a newly allocated buffer that should be freed by the caller or
 * NULL in case of error.
 **********************************************************************/
char *msOWSBuildURLFilename(const char *pszPath, const char *pszURL, 
                            const char *pszExt)
{
    char *pszBuf, *pszPtr;
    int  i, nBufLen;

    nBufLen = strlen(pszURL) + strlen(pszExt) +1;
    if (pszPath)
        nBufLen += (strlen(pszPath)+1);

    pszBuf = (char*)malloc((nBufLen+1)*sizeof(char));
    if (pszBuf == NULL)
    {
        msSetError(MS_MEMERR, NULL, "msOWSBuildURLFilename()");
        return NULL;
    }
    pszBuf[0] = '\0';

    if (pszPath)
    {
#ifdef _WIN32
        sprintf(pszBuf, "%s\\", pszPath);
#else
        sprintf(pszBuf, "%s/", pszPath);
#endif
    }

    pszPtr = pszBuf + strlen(pszBuf);

    for(i=0; pszURL[i] != '\0'; i++)
    {
        if (isalnum(pszURL[i]))
            *pszPtr = pszURL[i];
        else
            *pszPtr = '_';
        pszPtr++;
    }
    
    strcpy(pszPtr, pszExt);

    return pszBuf;
}


#endif /* USE_WMS_SVR || USE_WFS_SVR */




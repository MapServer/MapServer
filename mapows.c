/**********************************************************************
 * $Id$
 *
 * mapows.c - OGC Web Services (WMS, WFS) support functions
 *
 **********************************************************************
 * $Log$
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

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR)

/*
** msOWSDispatch() is the entry point for any OWS request (WMS, WFS, ...)
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid OWS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msOWSDispatch(mapObj *map, char **names, char **values, int numentries)
{
    int status = MS_DONE;

#ifdef USE_WMS_SVR
    if ((status = msWMSDispatch(map, names, values, numentries)) != MS_DONE )
        return status;
#endif
#ifdef USE_WFS_SVR
    if ((status = msWFSDispatch(map, names, values, numentries)) != MS_DONE )
        return status;
#endif

    return MS_DONE;  /* Not a WMS/WFS request... let MapServer handle it */
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
**
*/
void msOWSPrintLatLonBoundingBox(FILE *stream, const char *tabspace, 
                                 rectObj *extent, projectionObj *srcproj)
{
  rectObj ll_ext;
  ll_ext = *extent;

  if (srcproj->numargs > 0 && !pj_is_latlong(srcproj->proj)) {
    msProjectRect(srcproj, NULL, &ll_ext);
  }

  fprintf(stream, "%s<LatLonBoundingBox minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\" />\n", 
         tabspace, ll_ext.minx, ll_ext.miny, ll_ext.maxx, ll_ext.maxy);
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

    value = msGetEPSGProj(srcproj, metadata, MS_TRUE);
    
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


/* msEncodeHTMLEntities()
**
** Return a copy of string after replacing some problematic chars with their
** HTML entity equivalents.
**
** The replacements performed are:
**  '&' -> "&amp;", '"' -> "&quot;", '<' -> "&lt;" and '>' -> "&gt;"
**/
char *msEncodeHTMLEntities(const char *string) 
{
    int buflen, i;
    char *newstring;
    const char *c;

    // Start with 100 extra chars for replacements... 
    // should be good enough for most cases
    buflen = strlen(string) + 100;
    newstring = (char*)malloc(buflen+1*sizeof(char*));
    if (newstring == NULL)
    {
        msSetError(MS_MEMERR, NULL, "msEncodeHTMLEntities()");
        return NULL;
    }

    for(i=0, c=string; *c != '\0'; c++)
    {
        // Need to realloc buffer?
        if (i+6 > buflen)
        {
            // If we had to realloc then this string must contain several
            // entities... so let's go with twice the previous buffer size
            buflen *= 2;
            newstring = (char*)realloc(newstring, buflen+1*sizeof(char*));
            if (newstring == NULL)
            {
                msSetError(MS_MEMERR, NULL, "msEncodeHTMLEntities()");
                return NULL;
            }
        }

        switch(*c)
        {
          case '&':
            strcpy(newstring+i, "&amp;");
            i += 5;
            break;
          case '"':
            strcpy(newstring+i, "&quot;");
            i += 6;
            break;
          case '<':
            strcpy(newstring+i, "&lt;");
            i += 4;
            break;
          case '>':
            strcpy(newstring+i, "&gt;");
            i += 4;
            break;
          default:
            newstring[i++] = *c;
        }
    }

    newstring[i++] = '\0';

    return newstring;
}


/* msDecodeHTMLEntities()
**
** Modify the string to replace encoded characters by their true value
**
** The replacements performed are:
**  "&amp;" -> '&', "&quot;" -> '"', "&lt;" -> '<' and "&gt;" -> '>'
**/
void msDecodeHTMLEntities(const char *string) 
{
    char *pszAmp=NULL, *pszSemiColon=NULL, *pszReplace=NULL, *pszEnd=NULL;
    char *pszBuffer=NULL;

    if(string == NULL)
        return;
    else
        pszBuffer = (char*)string;

    pszReplace = (char*) malloc(sizeof(char) * strlen(pszBuffer));
    pszEnd = (char*) malloc(sizeof(char) * strlen(pszBuffer));

    while((pszAmp = strchr(pszBuffer, '&')) != NULL)
    {
        // Get the &...;
        strcpy(pszReplace, pszAmp);
        pszSemiColon = strchr(pszReplace, ';');
        if(pszSemiColon == NULL)
            break;
        else
            pszSemiColon++;
        pszReplace[pszSemiColon-pszReplace] = '\0';

        // Get everything after the &...;
        strcpy(pszEnd, pszSemiColon+1);

        // Replace the &...;
        if(strcasecmp(pszReplace, "&amp;") == 0)
        {
            pszBuffer[pszAmp - pszBuffer] = '&';
            pszBuffer[pszAmp - pszBuffer + 1] = '\0';
            strcat(pszBuffer, pszEnd);
        }
        else if(strcasecmp(pszReplace, "&quot;") == 0)
        {
            pszBuffer[pszAmp - pszBuffer] = '"';
            pszBuffer[pszAmp - pszBuffer + 1] = '\0';
            strcat(pszBuffer, pszEnd);
        }
        else if(strcasecmp(pszReplace, "&lt;") == 0)
        {
            pszBuffer[pszAmp - pszBuffer] = '<';
            pszBuffer[pszAmp - pszBuffer + 1] = '\0';
            strcat(pszBuffer, pszEnd);
        }
        else if(strcasecmp(pszReplace, "&gt;") == 0)
        {
            pszBuffer[pszAmp - pszBuffer] = '>';
            pszBuffer[pszAmp - pszBuffer + 1] = '\0';
            strcat(pszBuffer, pszEnd);
        }

        pszBuffer = pszAmp + 1;
    }

    free(pszReplace);
    free(pszEnd);

    return;
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
      (value = msLookupHashTable(lp->metadata, "wfs_extent")) != NULL )
  {
    char **tokens;
    int n;

    tokens = split(value, ' ', &n);
    if (tokens==NULL || n != 4) {
      msSetError(MS_WMSERR, "Wrong number of arguments for EXTENT metadata.",
                 "msWMSGetLayerExtent()");
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
    if (msLayerOpen(lp, map->shapepath) == MS_SUCCESS) {
      int status;
      status = msLayerGetExtent(lp, ext);
      msLayerClose(lp);
      return status;
    }
  }

  return MS_FAILURE;
}



#endif /* USE_WMS_SVR || USE_WFS_SVR */




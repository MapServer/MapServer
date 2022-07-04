/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC Web Services (WMS, WFS) support functions
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "mapserver.h"
#include "maptime.h"
#include "maptemplate.h"
#include "mapows.h"

#if defined(USE_LIBXML2)
#include "maplibxml2.h"
#else
#include "cpl_minixml.h"
#include "cpl_error.h"
#endif
#include "mapowscommon.h"

#include <ctype.h> /* isalnum() */
#include <stdarg.h>
#include <assert.h>



/*
** msOWSInitRequestObj() initializes an owsRequestObj; i.e: sets
** all internal pointers to NULL.
*/
void msOWSInitRequestObj(owsRequestObj *ows_request)
{
  ows_request->numlayers = 0;
  ows_request->numwmslayerargs = 0;
  ows_request->enabled_layers = NULL;
  ows_request->layerwmsfilterindex = NULL;

  ows_request->service = NULL;
  ows_request->version = NULL;
  ows_request->request = NULL;
  ows_request->document = NULL;
}

/*
** msOWSClearRequestObj() releases all resources associated with an
** owsRequestObj.
*/
void msOWSClearRequestObj(owsRequestObj *ows_request)
{
  msFree(ows_request->enabled_layers);
  msFree(ows_request->layerwmsfilterindex);
  msFree(ows_request->service);
  msFree(ows_request->version);
  msFree(ows_request->request);
  if(ows_request->document) {
#if defined(USE_LIBXML2)
    xmlFreeDoc(ows_request->document);
    xmlCleanupParser();
#else
    CPLDestroyXMLNode(ows_request->document);
#endif
  }
}

#if defined(USE_LIBXML2) && LIBXML_VERSION < 20900
static int bExternalEntityAsked = MS_FALSE;
static xmlParserInputPtr  dummyEntityLoader(const char * URL, 
                                           const char * ID, 
                                           xmlParserCtxtPtr context )
{
    bExternalEntityAsked = MS_TRUE;
    return NULL;
}
#endif

/*
** msOWSPreParseRequest() parses a cgiRequestObj either with GET/KVP
** or with POST/XML. Only SERVICE, VERSION (or WMTVER) and REQUEST are
** being determined, all WxS (or SOS) specific parameters are parsed
** within the according handler.
** The results are saved within an owsRequestObj. If the request was
** transmitted with POST/XML, either the document (if compiled with
** libxml2) or the root CPLXMLNode is saved to the ows_request->document
** field.
** Returns MS_SUCCESS upon success, MS_FAILURE if severe errors occurred
** or MS_DONE, if the service could not be determined.
*/
static int msOWSPreParseRequest(cgiRequestObj *request,
                                owsRequestObj *ows_request)
{
  /* decide if KVP or XML */
  if (request->type == MS_GET_REQUEST || (request->type == MS_POST_REQUEST 
    && request->contenttype && strncmp(request->contenttype, "application/x-www-form-urlencoded", strlen("application/x-www-form-urlencoded")) == 0)) {
    int i;
    /* parse KVP parameters service, version and request */
    for (i = 0; i < request->NumParams; ++i) {
      if (ows_request->service == NULL &&
          EQUAL(request->ParamNames[i], "SERVICE")) {
        ows_request->service = msStrdup(request->ParamValues[i]);
      } else if (ows_request->version == NULL &&
                 (EQUAL(request->ParamNames[i], "VERSION")
                 || EQUAL(request->ParamNames[i], "WMTVER"))) { /* for WMS */
        ows_request->version = msStrdup(request->ParamValues[i]);
      } else if (ows_request->request == NULL &&
                 EQUAL(request->ParamNames[i], "REQUEST")) {
        ows_request->request = msStrdup(request->ParamValues[i]);
      }

      /* stop if we have all necessary parameters */
      if(ows_request->service && ows_request->version && ows_request->request) {
        break;
      }
    }
  } else if (request->type == MS_POST_REQUEST) {
#if defined(USE_LIBXML2)
    xmlNodePtr root = NULL;
#if LIBXML_VERSION < 20900
    xmlExternalEntityLoader oldExternalEntityLoader;
#endif
#else
    CPLXMLNode *temp;
#endif
    if (!request->postrequest || !strlen(request->postrequest)) {
      msSetError(MS_OWSERR, "POST request is empty.",
                 "msOWSPreParseRequest()");
      return MS_FAILURE;
    }
#if defined(USE_LIBXML2)
#if LIBXML_VERSION < 20900
    oldExternalEntityLoader = xmlGetExternalEntityLoader();
    /* to avoid  XML External Entity vulnerability with libxml2 < 2.9 */
    xmlSetExternalEntityLoader (dummyEntityLoader); 
    bExternalEntityAsked = MS_FALSE;
#endif
    /* parse to DOM-Structure with libxml2 and get the root element */
    ows_request->document = xmlParseMemory(request->postrequest,
                                           strlen(request->postrequest));
#if LIBXML_VERSION < 20900
    xmlSetExternalEntityLoader (oldExternalEntityLoader); 
    if( bExternalEntityAsked )
    {
        msSetError(MS_OWSERR, "XML parsing error: %s",
                 "msOWSPreParseRequest()", "External entity fetch");
        return MS_FAILURE;
    }
#endif
    if (ows_request->document == NULL
        || (root = xmlDocGetRootElement(ows_request->document)) == NULL) {
      xmlErrorPtr error = xmlGetLastError();
      msSetError(MS_OWSERR, "XML parsing error: %s",
                 "msOWSPreParseRequest()", error->message);
      return MS_FAILURE;
    }

    /* Get service, version and request from root */
    xmlChar* serviceTmp = xmlGetProp(root, BAD_CAST "service");
    if (serviceTmp != NULL) {
        ows_request->service = msStrdup((char *) serviceTmp);
        xmlFree(serviceTmp);
    }

    xmlChar* versionTmp = xmlGetProp(root, BAD_CAST "version");
    if (versionTmp != NULL) {
        ows_request->version = msStrdup((char *) versionTmp);
        xmlFree(versionTmp);
    }

    ows_request->request = msStrdup((char *) root->name);

#else
    /* parse with CPLXML */
    ows_request->document = CPLParseXMLString(request->postrequest);
    if (ows_request->document == NULL) {
      msSetError(MS_OWSERR, "XML parsing error: %s",
                 "msOWSPreParseRequest()", CPLGetLastErrorMsg());
      return MS_FAILURE;
    }

    /* remove all namespaces */
    CPLStripXMLNamespace(ows_request->document, NULL, 1);
    for (temp = ows_request->document;
         temp != NULL;
         temp = temp->psNext) {

      if (temp->eType == CXT_Element) {
        const char *service, *version;
        ows_request->request = msStrdup(temp->pszValue);

        if ((service = CPLGetXMLValue(temp, "service", NULL)) != NULL) {
          ows_request->service = msStrdup(service);
        }
        if ((version = CPLGetXMLValue(temp, "version", NULL)) != NULL) {
          ows_request->version = msStrdup(version);
        }
        continue;
      }
    }
#endif /* defined(USE_LIBXML2) */
  } else {
    msSetError(MS_OWSERR, "Unknown request method. Use either GET or POST.",
               "msOWSPreParseRequest()");
    return MS_FAILURE;
  }

  /* certain WMS requests do not require the service parameter */
  /* see: http://trac.osgeo.org/mapserver/ticket/2531          */
  if (ows_request->service == NULL
      && ows_request->request != NULL) {
    if (EQUAL(ows_request->request, "GetMap")
        || EQUAL(ows_request->request, "GetFeatureInfo")) {
      ows_request->service = msStrdup("WMS");
    } else { /* service could not be determined */
      return MS_DONE;
    }
  }

  return MS_SUCCESS;
}

/*
** msOWSDispatch() is the entry point for any OWS request (WMS, WFS, ...)
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If force_ows_mode is true then an exception will be produced if the
**   request is not recognized as a valid request.
** - If force_ows_mode is false and this does not appear to be a valid OWS
**   request then MS_DONE is returned and MapServer is expected to process
**   this as a regular MapServer (traditional CGI) request.
*/
int msOWSDispatch(mapObj *map, cgiRequestObj *request, int ows_mode)
{
  int status = MS_DONE, force_ows_mode = 0;
  owsRequestObj ows_request;

  if (!request) {
    return status;
  }

  force_ows_mode = (ows_mode == OWS || ows_mode == WFS);

  msOWSInitRequestObj(&ows_request);
  switch(msOWSPreParseRequest(request, &ows_request)) {
    case MS_FAILURE: /* a severe error occurred */
      return MS_FAILURE;
    case MS_DONE:
      /* OWS Service could not be determined              */
      /* continue for now                                 */
      status = MS_DONE;
  }

  if (ows_request.service == NULL) {
#ifdef USE_LIBXML2
    if (ows_request.request && EQUAL(ows_request.request, "GetMetadata")) {
      status = msMetadataDispatch(map, request);
      msOWSClearRequestObj(&ows_request);
      return status;
    }
#endif
#ifdef USE_WFS_SVR
    if( msOWSLookupMetadata(&(map->web.metadata), "FO", "cite_wfs2") != NULL ) {
      status = msWFSException(map, "service", MS_OWS_ERROR_MISSING_PARAMETER_VALUE, NULL );
    }
    else
#endif

    /* exit if service is not set */
    if(force_ows_mode) {
      msSetError( MS_MISCERR,
                  "OWS Common exception: exceptionCode=MissingParameterValue, locator=SERVICE, ExceptionText=SERVICE parameter missing.",
                  "msOWSDispatch()");
      status = MS_FAILURE;
    } else {
      status = MS_DONE;
    }
  } else if (EQUAL(ows_request.service, "WMS")) {
#ifdef USE_WMS_SVR
    status = msWMSDispatch(map, request, &ows_request, MS_FALSE);
#else
    msSetError( MS_WMSERR,
                "SERVICE=WMS requested, but WMS support not configured in MapServer.",
                "msOWSDispatch()" );
#endif
  } else if (EQUAL(ows_request.service, "WFS")) {
#ifdef USE_WFS_SVR
    status = msWFSDispatch(map, request, &ows_request, (ows_mode == WFS));
#else
    msSetError( MS_WFSERR,
                "SERVICE=WFS requested, but WFS support not configured in MapServer.",
                "msOWSDispatch()" );
#endif
  } else if (EQUAL(ows_request.service, "WCS")) {
#ifdef USE_WCS_SVR
    status = msWCSDispatch(map, request, &ows_request);
#else
    msSetError( MS_WCSERR,
                "SERVICE=WCS requested, but WCS support not configured in MapServer.",
                "msOWSDispatch()" );
#endif
  } else if (EQUAL(ows_request.service, "SOS")) {
#ifdef USE_SOS_SVR
    status = msSOSDispatch(map, request, &ows_request);
#else
    msSetError( MS_SOSERR,
                "SERVICE=SOS requested, but SOS support not configured in MapServer.",
                "msOWSDispatch()" );
#endif
  } else if(force_ows_mode) {
    msSetError( MS_MISCERR,
                "OWS Common exception: exceptionCode=InvalidParameterValue, locator=SERVICE, ExceptionText=SERVICE parameter value invalid.",
                "msOWSDispatch()");
    status = MS_FAILURE;
  }

  msOWSClearRequestObj(&ows_request);
  return status;
}

/*
** msOWSIpParse()
**
** Parse the IP address or range into a binary array.
** Supports ipv4 and ipv6 addresses
** Ranges can be specified using the CIDR notation (ie: 192.100.100.0/24)
**
** Returns the parsed of the IP (4 or 16).
*/
int msOWSIpParse(const char* ip, unsigned char* ip1, unsigned char* mask)
{
  int len = 0, masklen, seps;
  
  if (msCountChars((char*)ip, '.') == 3) {
    /* ipv4 */
    unsigned char* val = ip1;
    len = 1;
    masklen = 32;
    *val = 0;
    while (*ip) {
      if (*ip >= '0' && *ip <= '9')
        (*val) = 10 * (*val) + (*ip - '0');
      else if (*ip == '.') {
        ++val;
        *val = 0;
        ++len;
      }
      else if (*ip == '/')
      {
        masklen = atoi(ip+1);
        if (masklen > 32)
          masklen = 32;
        break;
      }
      else 
        break;
      ++ip;
    }
    if (len != 4)
      return 0;
    /* write mask */
    if (mask) {
      memset(mask, 0, len);
      val = mask;
      while (masklen) {
        if (masklen >= 8) {
          *val = 0xff;
          masklen -= 8;
        }
        else {
          *val = - ((unsigned char)pow(2, 8 - masklen));
          break;
        }
        ++val;
      }
    }
  }
  else if ((seps = msCountChars((char*)ip, ':')) > 1 && seps < 8) {
    /* ipv6 */
    unsigned short* val = (unsigned short*)ip1;
    len = 2;
    masklen = 128;
    *val = 0;
    while (*ip) {
      if (*ip >= '0' && *ip <= '9')
        (*val) = 16 * (*val) + (*ip - '0');
      else if (*ip >= 'a' && *ip <= 'f')
        (*val) = 16 * (*val) + (*ip - 'a' + 10);
      else if (*ip >= 'A' && *ip <= 'F')
        (*val) = 16 * (*val) + (*ip - 'A' + 10);
      else if (*ip == ':') {
        ++ip;
        ++val;
        len += 2;
        *val = 0;
        if (*ip == ':') {
          /* insert 0 values */
          while (seps <= 7) {
            ++val;
            len += 2;
            *val = 0;
            ++seps;
          }
        }
        else
          continue;
      }
      else if (*ip == '/')
      {
        masklen = atoi(ip+1);
        if (masklen > 128)
          masklen = 128;
        break;
      }
      else
        break;
      ++ip;
    }
    if (len != 16)
      return 0;
    /* write mask */
    if (mask) {
      memset(mask, 0, len);
      val = (unsigned short*)mask;
      while (masklen) {
        if (masklen >= 16) {
          *val = 0xffff;
          masklen -= 16;
        }
        else {
          *val = - ((unsigned short)pow(2, 16 - masklen));
          break;
        }
        ++val;
      }
    }
  }

  return len;
}

/*
** msOWSIpInList()
**
** Check if an ip is in a space separated list of IP addresses/ranges.
** Supports ipv4 and ipv6 addresses
** Ranges can be specified using the CIDR notation (ie: 192.100.100.0/24)
**
** Returns MS_TRUE if the IP is found.
*/
int msOWSIpInList(const char *ip_list, const char* ip)
{
  int i, j, numips, iplen;
  unsigned char ip1[16];
  unsigned char ip2[16];
  unsigned char mask[16];
  char** ips;

  /* Parse input IP */
  iplen = msOWSIpParse(ip, (unsigned char*)&ip1, NULL);
  if (iplen != 4 && iplen != 16) /* ipv4 or ipv6 */
    return MS_FALSE;

  ips = msStringSplit(ip_list, ' ', &numips);
  if (ips) {
    for (i = 0; i < numips; i++) {
      if (msOWSIpParse(ips[i], (unsigned char*)&ip2, (unsigned char*)&mask) == iplen)
      {
        for (j = 0; j < iplen; j++) {
          if ((ip1[j] & mask[j]) != (ip2[j] & mask[j]))
            break;
        }
        if (j == iplen) {
          msFreeCharArray(ips, numips);
          return MS_TRUE; /* match found */
        }
      }
    }
    msFreeCharArray(ips, numips);
  }

  return MS_FALSE;
}

/*
** msOWSIpDisabled()
**
** Check if an ip is in a list specified in the metadata section.
**
** Returns MS_TRUE if the IP is found.
*/
int msOWSIpInMetadata(const char *ip_list, const char* ip)
{
  FILE *stream;
  char buffer[MS_BUFFER_LENGTH];
  int found = MS_FALSE;
  
  if (strncasecmp(ip_list, "file:", 5) == 0) {
    stream = fopen(ip_list + 5, "r");
    if(stream) {
      found = MS_FALSE;
      while(fgets(buffer, MS_BUFFER_LENGTH, stream)) {
        if(msOWSIpInList(buffer, ip)) {
          found = MS_TRUE;
          break;
        }
      }
      fclose(stream);
    }  
  }
  else {
    if(msOWSIpInList(ip_list, ip))
      found = MS_TRUE;
  }  
  return found;
}

/*
** msOWSIpDisabled()
**
** Check if the layers are enabled or disabled by IP list.
**
** 'namespaces' is a string with a letter for each namespace to lookup
** in the order they should be looked up. e.g. "MO" to lookup wms_ and ows_
** If namespaces is NULL then this function just does a regular metadata
** lookup.
**
** Returns the disabled flag.
*/
int msOWSIpDisabled(hashTableObj *metadata, const char *namespaces, const char* ip)
{
  const char *ip_list;
  int disabled = MS_FALSE;

  if (!ip)
    return MS_FALSE; /* no endpoint ip */

  ip_list = msOWSLookupMetadata(metadata, namespaces, "allowed_ip_list");
  if (!ip_list)
    ip_list = msOWSLookupMetadata(metadata, "O", "allowed_ip_list");

  if (ip_list) {
    disabled = MS_TRUE;
    if (msOWSIpInMetadata(ip_list, ip))
      disabled = MS_FALSE;
  }

  ip_list = msOWSLookupMetadata(metadata, namespaces, "denied_ip_list");
  if (!ip_list)
    ip_list = msOWSLookupMetadata(metadata, "O", "denied_ip_list");

  if (ip_list && msOWSIpInMetadata(ip_list, ip))
    disabled = MS_TRUE;

  return disabled;
}

/*
** msOWSRequestIsEnabled()
**
** Check if a layer is visible for a specific OWS request.
**
** 'namespaces' is a string with a letter for each namespace to lookup in
** the order they should be looked up. e.g. "MO" to lookup wms_ and ows_ If
** namespaces is NULL then this function just does a regular metadata
** lookup. If check_all_layers is set to MS_TRUE, the function will check
** all layers to see if the request is enable. (returns as soon as one is found) */
int msOWSRequestIsEnabled(mapObj *map, layerObj *layer,
                          const char *namespaces, const char *request, int check_all_layers)
{
  int disabled = MS_FALSE; /* explicitly disabled flag */
  const char *enable_request;
  const char *remote_ip;

  if (request == NULL)
    return MS_FALSE;

  remote_ip = getenv("REMOTE_ADDR");

  /* First, we check in the layer metadata */
  if (layer && check_all_layers == MS_FALSE) {
    enable_request = msOWSLookupMetadata(&layer->metadata, namespaces, "enable_request");
    if (msOWSParseRequestMetadata(enable_request, request, &disabled))
      return MS_TRUE;
    if (disabled) return MS_FALSE;

    enable_request = msOWSLookupMetadata(&layer->metadata, "O", "enable_request");
    if (msOWSParseRequestMetadata(enable_request, request, &disabled))
      return MS_TRUE;
    if (disabled) return MS_FALSE;

    if (msOWSIpDisabled(&layer->metadata, namespaces, remote_ip))
      return MS_FALSE;
  }

  if (map && (check_all_layers == MS_FALSE || map->numlayers == 0)) {
    /* then we check in the map metadata */
    enable_request = msOWSLookupMetadata(&map->web.metadata, namespaces, "enable_request");
    if (msOWSParseRequestMetadata(enable_request, request, &disabled))
      return MS_TRUE;
    if (disabled) return MS_FALSE;

    enable_request = msOWSLookupMetadata(&map->web.metadata, "O", "enable_request");
    if (msOWSParseRequestMetadata(enable_request, request, &disabled))
      return MS_TRUE;
    if (disabled) return MS_FALSE;

    if (msOWSIpDisabled(&map->web.metadata, namespaces, remote_ip))
      return MS_FALSE;
  }

  if (map && check_all_layers == MS_TRUE) {
    int i, globally_enabled = MS_FALSE;
    enable_request = msOWSLookupMetadata(&map->web.metadata, namespaces, "enable_request");
    globally_enabled = msOWSParseRequestMetadata(enable_request, request, &disabled);

    if (!globally_enabled && !disabled) {
      enable_request = msOWSLookupMetadata(&map->web.metadata, "O", "enable_request");
      globally_enabled = msOWSParseRequestMetadata(enable_request, request, &disabled);
    }

    if (globally_enabled && msOWSIpDisabled(&map->web.metadata, namespaces, remote_ip))
      globally_enabled = MS_FALSE;

    /* Check all layers */
    for(i=0; i<map->numlayers; i++) {
      int result = MS_FALSE;
      layerObj *lp;
      lp = (GET_LAYER(map, i));

      enable_request = msOWSLookupMetadata(&lp->metadata, namespaces, "enable_request");
      result = msOWSParseRequestMetadata(enable_request, request, &disabled);
      if (!result && disabled) continue; /* if the request has been explicitly set to disabled, continue */

      if (!result && !disabled) { /* if the request has not been found in the wms metadata, */
        /* check the ows namespace  */

        enable_request = msOWSLookupMetadata(&lp->metadata, "O", "enable_request");
        result = msOWSParseRequestMetadata(enable_request, request, &disabled);
        if (!result && disabled) continue;
      }

      if (msOWSIpDisabled(&lp->metadata, namespaces, remote_ip))
        continue;

      if (result || (!disabled && globally_enabled))
        return MS_TRUE;
    }

    if (!disabled && globally_enabled)
        return MS_TRUE;
  }

  return MS_FALSE;
}

/*
** msOWSRequestLayersEnabled()
**
** Check if the layers are visible for a specific OWS request.
**
** 'namespaces' is a string with a letter for each namespace to lookup
** in the order they should be looked up. e.g. "MO" to lookup wms_ and ows_
** If namespaces is NULL then this function just does a regular metadata
** lookup.
**
** Generates an array of the layer ids enabled.
*/
void msOWSRequestLayersEnabled(mapObj *map, const char *namespaces,
                               const char *request, owsRequestObj *ows_request)
{
  int disabled = MS_FALSE; /* explicitly disabled flag */
  int globally_enabled = MS_FALSE;
  const char *enable_request;
  const char *remote_ip;

  if (ows_request->numlayers > 0)
    msFree(ows_request->enabled_layers);

  ows_request->numlayers = 0;
  ows_request->enabled_layers = NULL;

  if (request == NULL || (map == NULL) || (map->numlayers <= 0))
    return;

  remote_ip = getenv("REMOTE_ADDR");

  enable_request = msOWSLookupMetadata(&map->web.metadata, namespaces, "enable_request");
  globally_enabled = msOWSParseRequestMetadata(enable_request, request, &disabled);

  if (!globally_enabled && !disabled) {
    enable_request = msOWSLookupMetadata(&map->web.metadata, "O", "enable_request");
    globally_enabled = msOWSParseRequestMetadata(enable_request, request, &disabled);
  }

  if (globally_enabled && msOWSIpDisabled(&map->web.metadata, namespaces, remote_ip))
      globally_enabled = MS_FALSE;

  if (map->numlayers) {
    int i, layers_size = map->numlayers; /* for most of cases, this will be relatively small */

    ows_request->enabled_layers = (int*)msSmallMalloc(sizeof(int)*layers_size);

    for(i=0; i<map->numlayers; i++) {
      int result = MS_FALSE;
      layerObj *lp;
      lp = (GET_LAYER(map, i));

      enable_request = msOWSLookupMetadata(&lp->metadata, namespaces, "enable_request");
      result = msOWSParseRequestMetadata(enable_request, request, &disabled);
      if (!result && disabled) continue; /* if the request has been explicitly set to disabled, continue */

      if (!result && !disabled) { /* if the request has not been found in the wms metadata, */
        /* check the ows namespace  */

        enable_request = msOWSLookupMetadata(&lp->metadata, "O", "enable_request");
        result = msOWSParseRequestMetadata(enable_request, request, &disabled);
        if (!result && disabled) continue;
      }

      if (msOWSIpDisabled(&lp->metadata, namespaces, remote_ip))
          continue;

      if (result || (!disabled && globally_enabled)) {
        ows_request->enabled_layers[ows_request->numlayers] = lp->index;
        ows_request->numlayers++;
      }
    }

    if (ows_request->numlayers == 0) {
      msFree(ows_request->enabled_layers);
      ows_request->enabled_layers = NULL;
    }
  }
}

/* msOWSParseRequestMetadata
 *
 * This function parse a enable_request metadata string and check if the
 * given request is present and enabled.
 */
int msOWSParseRequestMetadata(const char *metadata, const char *request, int *disabled)
{
  char requestBuffer[32];
  int wordFlag = MS_FALSE;
  int disableFlag = MS_FALSE;
  int allFlag = MS_FALSE;
  char *bufferPtr, *ptr = NULL;

  *disabled = MS_FALSE;

  if (metadata == NULL)
    return MS_FALSE;

  ptr = (char*)metadata;
  const size_t len = strlen(ptr);
  requestBuffer[0] = '\0';
  bufferPtr = requestBuffer;

  for (size_t i=0; i<=len; ++i,++ptr) {

    if (!wordFlag && isspace(*ptr))
      continue;

    wordFlag = MS_TRUE;

    if (*ptr == '!') {
      disableFlag = MS_TRUE;
      continue;
    } else if ( (*ptr == ' ') || (*ptr != '\0' && ptr[1] == '\0')) { /* end of word */
      if (ptr[1] == '\0' && *ptr != ' ') {
        *bufferPtr = *ptr;
        ++bufferPtr;
      }

      *bufferPtr = '\0';
      if (strcasecmp(request, requestBuffer) == 0) {
        *disabled =  MS_TRUE; /* explicitly found, will stop the process in msOWSRequestIsEnabled() */
        return (disableFlag ? MS_FALSE:MS_TRUE);
      } else {
        if (strcmp("*", requestBuffer) == 0) { /* check if we read the all flag */
          if (disableFlag)
            *disabled =  MS_TRUE;
          allFlag = disableFlag ? MS_FALSE:MS_TRUE;
        }
        /* reset flags */
        wordFlag = MS_FALSE;
        disableFlag = MS_FALSE;
        bufferPtr = requestBuffer;
      }
    } else {
      *bufferPtr = *ptr;
      ++bufferPtr;
    }
  }

  return allFlag;
}

/*
** msOWSGetPrefixFromNamespace()
**
** Return the metadata name prefix from a character identifying the OWS
** namespace.
*/
static
const char* msOWSGetPrefixFromNamespace(char chNamespace)
{
    // Return should be a 3 character string, otherwise breaks assumption
    // in msOWSLookupMetadata()
    switch( chNamespace )
    {
        case 'O': return "ows";
        case 'A': return "oga"; /* oga_... (OGC Geospatial API) */
        case 'M': return "wms";
        case 'F': return "wfs";
        case 'C': return "wcs";
        case 'G': return "gml";
        case 'S': return "sos";
        default:
          /* We should never get here unless an invalid code (typo) is */
          /* present in the code, but since this happened before... */
          msSetError(MS_WMSERR,
                     "Unsupported metadata namespace code (%c).",
                     "msOWSGetPrefixFromNamespace()", chNamespace );
          assert(MS_FALSE);
          return NULL;
    }
}

/*
** msOWSLookupMetadata()
**
** Attempts to lookup a given metadata name in multiple OWS namespaces.
**
** 'namespaces' is a string with a letter for each namespace to lookup
** in the order they should be looked up. e.g. "MO" to lookup wms_ and ows_
** If namespaces is NULL then this function just does a regular metadata
** lookup.
*/
const char *msOWSLookupMetadata(hashTableObj *metadata,
                                const char *namespaces, const char *name)
{
  const char *value = NULL;

  if (namespaces == NULL) {
    value = msLookupHashTable(metadata, (char*)name);
  } else {
    char buf[100] = "ows_";

    strlcpy(buf+4, name, 96);

    while (value == NULL && *namespaces != '\0') {
      const char* prefix = msOWSGetPrefixFromNamespace(*namespaces);
      if( prefix == NULL )
          return NULL;
      assert(strlen(prefix) == 3);
      memcpy(buf, prefix, 3);
      value = msLookupHashTable(metadata, buf);
      namespaces++;
    }
  }

  return value;
}


/*
** msOWSLookupMetadataWithLanguage()
**
** Attempts to lookup a given metadata name in multiple OWS namespaces
** for a specific language.
*/
const char *msOWSLookupMetadataWithLanguage(hashTableObj *metadata,
    const char *namespaces, const char *name, const char* validated_language)
{
  const char *value = NULL;

  if ( name && validated_language && validated_language[0] ) {
    size_t bufferSize = strlen(name)+strlen(validated_language)+2;
    char *name2 = (char *) msSmallMalloc( bufferSize );
    snprintf(name2, bufferSize, "%s.%s", name, validated_language);
    value = msOWSLookupMetadata(metadata, namespaces, name2);
    free(name2);
  }

  if ( name && !value ) {
    value = msOWSLookupMetadata(metadata, namespaces, name);
  }


  return value;
}

/*
** msOWSLookupMetadata2()
**
** Attempts to lookup a given metadata name in multiple hashTables, and
** in multiple OWS namespaces within each. First searches the primary
** table and if no result is found, attempts the search using the
** secondary (fallback) table.
**
** 'namespaces' is a string with a letter for each namespace to lookup
** in the order they should be looked up. e.g. "MO" to lookup wms_ and ows_
** If namespaces is NULL then this function just does a regular metadata
** lookup.
*/
const char *msOWSLookupMetadata2(hashTableObj *pri,
                                 hashTableObj *sec,
                                 const char *namespaces,
                                 const char *name)
{
  const char *result;

  if ((result = msOWSLookupMetadata(pri, namespaces, name)) == NULL) {
    /* Try the secondary table */
    result = msOWSLookupMetadata(sec, namespaces, name);
  }

  return result;
}


/* msOWSParseVersionString()
**
** Parse a version string in the format "a.b.c" or "a.b" and return an
** integer in the format 0x0a0b0c suitable for regular integer comparisons.
**
** Returns one of OWS_VERSION_NOTSET or OWS_VERSION_BADFORMAT if version
** could not be parsed.
*/
int msOWSParseVersionString(const char *pszVersion)
{
  char **digits = NULL;
  int numDigits = 0;

  if (pszVersion) {
    int nVersion = 0;
    digits = msStringSplit(pszVersion, '.', &numDigits);
    if (digits == NULL || numDigits < 2 || numDigits > 3) {
      msSetError(MS_OWSERR,
                 "Invalid version (%s). Version must be in the "
                 "format 'x.y' or 'x.y.z'",
                 "msOWSParseVersionString()", pszVersion);
      if (digits)
        msFreeCharArray(digits, numDigits);
      return OWS_VERSION_BADFORMAT;
    }

    nVersion = atoi(digits[0])*0x010000;
    nVersion += atoi(digits[1])*0x0100;
    if (numDigits > 2)
      nVersion += atoi(digits[2]);

    msFreeCharArray(digits, numDigits);

    return nVersion;
  }

  return OWS_VERSION_NOTSET;
}

/* msOWSGetVersionString()
**
** Returns a OWS version string in the format a.b.c from the integer
** version value passed as argument (0x0a0b0c)
**
** Fills in the pszBuffer and returns a reference to it. Recommended buffer
** size is OWS_VERSION_MAXLEN chars.
*/
const char *msOWSGetVersionString(int nVersion, char *pszBuffer)
{

  if (pszBuffer)
    snprintf(pszBuffer, OWS_VERSION_MAXLEN-1, "%d.%d.%d",
             (nVersion/0x10000)%0x100, (nVersion/0x100)%0x100, nVersion%0x100);

  return pszBuffer;
}


/*
** msOWSGetEPSGProj()
**
** Extract projection code for this layer/map.
**
** First look for a xxx_srs metadata. If not found then look for an EPSG
** code in projectionObj, and if not found then return NULL.
**
** If bReturnOnlyFirstOne=TRUE and metadata contains multiple EPSG codes
** then only the first one (which is assumed to be the layer's default
** projection) is returned.
*/
void msOWSGetEPSGProj(projectionObj *proj, hashTableObj *metadata, const char *namespaces, int bReturnOnlyFirstOne, char **epsgCode)
{
  const char *value;
  *epsgCode = NULL;

  /* metadata value should already be in format "EPSG:n" or "AUTO:..." */
  if (metadata && ((value = msOWSLookupMetadata(metadata, namespaces, "srs")) != NULL)) {
    const char *space_ptr;
    if (!bReturnOnlyFirstOne || (space_ptr = strchr(value,' ')) == NULL) {
      *epsgCode = msStrdup(value);
      return;
    }
    

    *epsgCode = msSmallMalloc((space_ptr - value + 1)*sizeof(char));
    /* caller requested only first projection code, copy up to the first space character*/
    strlcpy(*epsgCode, value, space_ptr - value + 1) ;
    return;
  } else if (proj && proj->numargs > 0 && (value = strstr(proj->args[0], "init=epsg:")) != NULL) {
    *epsgCode = msSmallMalloc((strlen("EPSG:")+strlen(value+10)+1)*sizeof(char));
    sprintf(*epsgCode, "EPSG:%s", value+10);
    return;
  } else if (proj && proj->numargs > 0 && (value = strstr(proj->args[0], "init=crs:")) != NULL) {
    *epsgCode = msSmallMalloc((strlen("CRS:")+strlen(value+9)+1)*sizeof(char));
    sprintf(*epsgCode, "CRS:%s", value+9);
    return;
  } else if (proj && proj->numargs > 0 && (strncasecmp(proj->args[0], "AUTO:", 5) == 0 ||
             strncasecmp(proj->args[0], "AUTO2:", 6) == 0)) {
    *epsgCode = msStrdup(proj->args[0]);
    return;
  }
}

/*
** msOWSProjectToWGS84()
**
** Reprojects the extent to WGS84.
**
*/
void msOWSProjectToWGS84(projectionObj *srcproj, rectObj *ext)
{
  if (srcproj->proj && !msProjIsGeographicCRS(srcproj)) {
    projectionObj wgs84;
    msInitProjection(&wgs84);
    msProjectionInheritContextFrom(&wgs84, srcproj);
    msLoadProjectionString(&wgs84, "+proj=longlat +ellps=WGS84 +datum=WGS84");
    msProjectRect(srcproj, &wgs84, ext);
    msFreeProjection(&wgs84);
  }
}

/* msOWSGetLanguage()
**
** returns the language via MAP/WEB/METADATA/ows_language
**
** Use value of "ows_language" metadata, if not set then
** return "undefined" as a default
*/
const char *msOWSGetLanguage(mapObj *map, const char *context)
{
  const char *language;

  /* if this is an exception, MapServer always returns Exception
     messages in en-US
  */
  if (strcmp(context,"exception") == 0) {
    language = MS_ERROR_LANGUAGE;
  }
  /* if not, fetch language from mapfile metadata */
  else {
    language = msLookupHashTable(&(map->web.metadata), "ows_language");

    if (language == NULL) {
      language = "undefined";
    }
  }
  return language;
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

  schemas_location = msLookupHashTable(&(map->web.metadata),
                                       "ows_schemas_location");
  if (schemas_location == NULL)
    schemas_location = OWS_DEFAULT_SCHEMAS_LOCATION;

  return schemas_location;
}

/*
** msOWSGetExpandedMetadataKey()
*/
static
char* msOWSGetExpandedMetadataKey(const char *namespaces, const char *metadata_name)
{
  char* pszRet = msStringConcatenate(NULL, "");
  for( int i = 0; namespaces[i] != '\0'; ++i )
  {
      if( i > 0 )
          pszRet = msStringConcatenate(pszRet, " or ");
      pszRet = msStringConcatenate(pszRet, "\"");
      pszRet = msStringConcatenate(pszRet, msOWSGetPrefixFromNamespace(namespaces[i]));
      pszRet = msStringConcatenate(pszRet, "_");
      pszRet = msStringConcatenate(pszRet, metadata_name);
      pszRet = msStringConcatenate(pszRet, "\"");
  }
  return pszRet;
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
char * msOWSGetOnlineResource(mapObj *map, const char *namespaces, const char *metadata_name,
                              cgiRequestObj *req)
{
  const char *value;
  char *online_resource = NULL;

  /* We need this script's URL, including hostname. */
  /* Default to use the value of the "onlineresource" metadata, and if not */
  /* set then build it: "http://$(SERVER_NAME):$(SERVER_PORT)$(SCRIPT_NAME)?" */
  /* (+append the map=... param if it was explicitly passed in QUERY_STRING) */
  /*  */
  if ((value = msOWSLookupMetadata(&(map->web.metadata), namespaces, metadata_name))) {
    online_resource = msOWSTerminateOnlineResource(value);
  } else {
    if ((online_resource = msBuildOnlineResource(map, req)) == NULL) {
      char* pszExpandedMetadataKey = msOWSGetExpandedMetadataKey(namespaces, metadata_name);
      msSetError(MS_CGIERR, "Please set %s metadata.", "msOWSGetOnlineResource()", pszExpandedMetadataKey);
      msFree(pszExpandedMetadataKey);
      return NULL;
    }
  }

  return online_resource;
}

/*
** msOWSTerminateOnlineResource()
**
** Append trailing "?" or "&" to an onlineresource URL if it doesn't have
** one already. The returned string is then ready to append GET parameters
** to it.
**
** Returns a newly allocated string that should be freed by the caller or
** NULL in case of error.
*/
char * msOWSTerminateOnlineResource(const char *src_url)
{
  char *online_resource = NULL;
  size_t buffer_size = 0;

  if (src_url == NULL)
    return NULL;

  buffer_size = strlen(src_url)+2;
  online_resource = (char*) malloc(buffer_size);

  if (online_resource == NULL) {
    msSetError(MS_MEMERR, NULL, "msOWSTerminateOnlineResource()");
    return NULL;
  }

  strlcpy(online_resource, src_url, buffer_size);

  /* Append trailing '?' or '&' if missing. */
  if (strchr(online_resource, '?') == NULL)
    strlcat(online_resource, "?", buffer_size);
  else {
    char *c;
    c = online_resource+strlen(online_resource)-1;
    if (*c != '?' && *c != '&')
      strlcpy(c+1, "&", buffer_size-strlen(online_resource));
  }

  return online_resource;
}

/************************************************************************/
/*                         msUpdateGMLFieldMetadata                     */
/*                                                                      */
/*      Updates a fields GML metadata if it has not already             */
/*      been set. Nullable is not implemented for all drivers           */
/*      and can be set to 0 if unknown                                  */
/************************************************************************/
int msUpdateGMLFieldMetadata(layerObj *layer, const char *field_name, const char *gml_type,
                             const char *gml_width, const char *gml_precision, const short nullable)
{

    char md_item_name[256];

    snprintf( md_item_name, sizeof(md_item_name), "gml_%s_type", field_name );
    if( msLookupHashTable(&(layer->metadata), md_item_name) == NULL )
    msInsertHashTable(&(layer->metadata), md_item_name, gml_type );

    snprintf( md_item_name, sizeof(md_item_name), "gml_%s_width", field_name );
    if( strlen(gml_width) > 0
        && msLookupHashTable(&(layer->metadata), md_item_name) == NULL )
    msInsertHashTable(&(layer->metadata), md_item_name, gml_width );

    snprintf( md_item_name, sizeof(md_item_name), "gml_%s_precision", field_name );
    if( strlen(gml_precision) > 0
        && msLookupHashTable(&(layer->metadata), md_item_name) == NULL )
    msInsertHashTable(&(layer->metadata), md_item_name, gml_precision );

    snprintf( md_item_name, sizeof(md_item_name), "gml_%s_nillable", field_name );
    if( nullable > 0 
        && msLookupHashTable(&(layer->metadata), md_item_name) == NULL )
    msInsertHashTable(&(layer->metadata), md_item_name, "true" );

    return MS_TRUE;
}

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR) || defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

/*
** msRenameLayer()
*/
static int msRenameLayer(layerObj *lp, int count)
{
  char *newname;
  newname = (char*)malloc((strlen(lp->name)+5)*sizeof(char));
  if (!newname) {
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

  /* Make sure all layers in the map file have valid and unique names */
  for(i=0; i<map->numlayers; i++) {
    int count=1;
    for(j=i+1; j<map->numlayers; j++) {
      if (GET_LAYER(map, i)->name == NULL || GET_LAYER(map, j)->name == NULL) {
        continue;
      }
      if (strcasecmp(GET_LAYER(map, i)->name, GET_LAYER(map, j)->name) == 0 &&
          msRenameLayer((GET_LAYER(map, j)), ++count) != MS_SUCCESS) {
        return MS_FAILURE;
      }
    }

    /* Don't forget to rename the first layer if duplicates were found */
    if (count > 1 && msRenameLayer((GET_LAYER(map, i)), 1) != MS_SUCCESS) {
      return MS_FAILURE;
    }
  }
  return MS_SUCCESS;
}

/*
** msOWSNegotiateVersion()
**
** returns the most suitable version an OWS is to support given a client
** version parameter.
**
** supported_versions must be ordered from highest to lowest
**
** Returns a version integer of the supported version
**
*/

int msOWSNegotiateVersion(int requested_version, const int supported_versions[], int num_supported_versions)
{
  int i;

  /* if version is not set return highest version */
  if (! requested_version)
    return supported_versions[0];

  /* if the requested version is lower than the lowest version return the lowest version  */
  if (requested_version < supported_versions[num_supported_versions-1])
    return supported_versions[num_supported_versions-1];

  /* return the first entry that's lower than or equal to the requested version */
  for (i = 0; i < num_supported_versions; i++) {
    if (supported_versions[i] <= requested_version)
      return supported_versions[i];
  }

  return requested_version;
}

/*
** msOWSGetOnlineResource()
**
** Return the online resource for this service and add language parameter.
**
** Returns a newly allocated string that should be freed by the caller or
** NULL in case of error.
*/
char * msOWSGetOnlineResource2(mapObj *map, const char *namespaces, const char *metadata_name,
                               cgiRequestObj *req, const char* validated_language)
{
  char *online_resource = msOWSGetOnlineResource(map, namespaces, metadata_name, req);

  if ( online_resource && validated_language && validated_language[0] ) {
    /* online_resource is already terminated, so we can simply add language=...& */
    /* but first we need to make sure that online_resource has enough capacity */
    online_resource = (char *)msSmallRealloc(online_resource, strlen(online_resource) + strlen(validated_language) +  11);
    strcat(online_resource, "language=");
    strcat(online_resource, validated_language);
    strcat(online_resource, "&");
  }

  return online_resource;
}

/* msOWSGetInspireSchemasLocation()
**
** schemas location is the root of the web tree where all Inspire-related
** schemas can be found on this server.  These URLs must exist in order
** to validate xml.
**
** Use value of "inspire_schemas_location" metadata
*/
const char *msOWSGetInspireSchemasLocation(mapObj *map)
{
  const char *schemas_location;

  schemas_location = msLookupHashTable(&(map->web.metadata),
                                       "inspire_schemas_location");
  if (schemas_location == NULL)
    schemas_location = "http://inspire.ec.europa.eu/schemas";

  return schemas_location;
}

/* msOWSGetLanguageList
**
** Returns the list of languages that this service supports
**
** Use value of "languages" metadata (comma-separated list), or NULL if not set
**
** Returns a malloced char** of length numitems which must be freed
** by the caller, or NULL (with numitems = 0)
*/
char **msOWSGetLanguageList(mapObj *map, const char *namespaces, int *numitems)
{

  const char *languages = NULL;

  languages = msOWSLookupMetadata(&(map->web.metadata), namespaces, "languages");
  if (languages && strlen(languages) > 0) {
    return msStringSplit(languages, ',', numitems);
  } else {
    *numitems = 0;
    return NULL;
  }
}

/* msOWSGetLanguageFromList
**
** Returns a language according to the language requested by the client
**
** If the requested language is in the languages metadata then use it,
** otherwise ignore it and use the defaul language, which is the first entry in
** the languages metadata list. Calling with a NULL requested_langauge
** therefore returns this default language. If the language metadata list is
** not defined then the language is set to NULL.
**
** Returns a malloced char* which must be freed by the caller, or NULL
*/
char *msOWSGetLanguageFromList(mapObj *map, const char *namespaces, const char *requested_language)
{
  int num_items = 0;
  char **languages = msOWSGetLanguageList(map, namespaces, &num_items);
  char *language = NULL;

  if( languages && num_items > 0 ) {
    if ( !requested_language || !msStringInArray( requested_language, languages, num_items) ) {
      language = msStrdup(languages[0]);
    } else {
      language = msStrdup(requested_language);
    }
  }
  msFreeCharArray(languages, num_items);

  return language;
}


/* msOWSLanguageNegotiation
**
** Returns a language according to the accepted languages requested by the client
**
** Returns a malloced char* which must be freed by the caller, or NULL
*/
char *msOWSLanguageNegotiation(mapObj *map, const char *namespaces, char **accept_languages, int num_accept_languages)
{
  int num_languages = 0;
  char **languages = NULL;
  char *result_language = NULL;

  languages = msOWSGetLanguageList(map, namespaces, &num_languages);

  if (languages && num_languages > 0) {
    int i;
    for (i = 0; i < num_accept_languages; ++i) {
      const char *accept_language = accept_languages[i];

      /* '*' means any language */
      if (EQUAL(accept_language, "*")) {
        result_language = msStrdup(languages[0]);
        break;
      } else if (msStringInArray(accept_language, languages, num_languages)) {
        result_language = msStrdup(accept_language);
        break;
      }
    }

    if (result_language == NULL) {
      result_language = msStrdup(languages[0]);
    }
  }

  msFreeCharArray(languages, num_languages);
  return result_language;
}


/* msOWSPrintInspireCommonExtendedCapabilities
**
** Output INSPIRE common extended capabilities items to stream
** The currently supported items are metadata and languages
**
** tag_name is the name (including ns prefix) of the tag to include the whole
** extended capabilities block in
**
** service is currently included for future compatibility when differing
** extended capabilities elements are included for different service types
**
** Returns a status code; MS_NOERR if all ok, action_if_not_found otherwise
*/
int msOWSPrintInspireCommonExtendedCapabilities(FILE *stream, mapObj *map, const char *namespaces,
    int action_if_not_found, const char *tag_name, const char* tag_ns,
    const char *validated_language, const OWSServiceType service)
{

  int metadataStatus = 0;
  int languageStatus = 0;

  if( tag_ns )
    msIO_fprintf(stream, "  <%s %s>\n", tag_name, tag_ns);
  else
    msIO_fprintf(stream, "  <%s>\n", tag_name);

  metadataStatus = msOWSPrintInspireCommonMetadata(stream, map, namespaces, action_if_not_found, service);
  languageStatus = msOWSPrintInspireCommonLanguages(stream, map, namespaces, action_if_not_found, validated_language);

  msIO_fprintf(stream, "  </%s>\n", tag_name);

  return (metadataStatus != MS_NOERR) ? metadataStatus : languageStatus;
}

/* msOWSPrintInspireCommonMetadata
**
** Output INSPIRE common metadata items to a stream
**
** Returns a status code; MS_NOERR if all OK, action_if_not_found otherwise
*/
int msOWSPrintInspireCommonMetadata(FILE *stream, mapObj *map, const char *namespaces,
                                    int action_if_not_found, const OWSServiceType service)
{

  int status = MS_NOERR;
  const char *inspire_capabilities = NULL;

  inspire_capabilities = msOWSLookupMetadata(&(map->web.metadata), namespaces, "inspire_capabilities");

  if(!inspire_capabilities) {
    if (OWS_WARN == action_if_not_found) {
      msIO_fprintf(stream, "<!-- WARNING: missing metadata entry for 'inspire_capabilities', one of 'embed' and 'url' must be supplied. -->\n");
    }
    return action_if_not_found;
  }
  if (strcasecmp("url",inspire_capabilities) == 0) {
    if ( msOWSLookupMetadata(&(map->web.metadata), namespaces, "inspire_metadataurl_href") != NULL ) {
      msIO_fprintf(stream, "    <inspire_common:MetadataUrl xsi:type=\"inspire_common:resourceLocatorType\">\n");
      msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "inspire_metadataurl_href", OWS_WARN, "      <inspire_common:URL>%s</inspire_common:URL>\n", "");
      msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "inspire_metadataurl_format", OWS_WARN, "      <inspire_common:MediaType>%s</inspire_common:MediaType>\n", "");
      msIO_fprintf(stream, "    </inspire_common:MetadataUrl>\n");
    } else {
      status = action_if_not_found;
      if (OWS_WARN == action_if_not_found) {
        char* pszExpandedMetadataKey = msOWSGetExpandedMetadataKey(namespaces, "inspire_metadataurl_href");
        msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata %s was missing in this context. -->\n", pszExpandedMetadataKey);
        msFree(pszExpandedMetadataKey);
      }
    }
  } else if (strcasecmp("embed",inspire_capabilities) == 0) {
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "inspire_resourcelocator", OWS_WARN, "    <inspire_common:ResourceLocator>\n      <inspire_common:URL>%s</inspire_common:URL>\n    </inspire_common:ResourceLocator>\n", NULL);
    msIO_fprintf(stream,"    <inspire_common:ResourceType>service</inspire_common:ResourceType>\n");
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "inspire_temporal_reference", OWS_WARN, "    <inspire_common:TemporalReference>\n      <inspire_common:DateOfLastRevision>%s</inspire_common:DateOfLastRevision>\n    </inspire_common:TemporalReference>\n", "");
    msIO_fprintf(stream, "    <inspire_common:Conformity>\n");
    msIO_fprintf(stream, "      <inspire_common:Specification>\n");
    msIO_fprintf(stream, "        <inspire_common:Title>-</inspire_common:Title>\n");
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "inspire_temporal_reference", OWS_NOERR, "        <inspire_common:DateOfLastRevision>%s</inspire_common:DateOfLastRevision>\n", "");
    msIO_fprintf(stream, "      </inspire_common:Specification>\n");
    msIO_fprintf(stream, "      <inspire_common:Degree>notEvaluated</inspire_common:Degree>\n");
    msIO_fprintf(stream, "    </inspire_common:Conformity>\n");
    msIO_fprintf(stream, "    <inspire_common:MetadataPointOfContact>\n");
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "inspire_mpoc_name", OWS_WARN, "      <inspire_common:OrganisationName>%s</inspire_common:OrganisationName>\n", "");
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "inspire_mpoc_email", OWS_WARN, "      <inspire_common:EmailAddress>%s</inspire_common:EmailAddress>\n", "");
    msIO_fprintf(stream, "    </inspire_common:MetadataPointOfContact>\n");
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "inspire_metadatadate", OWS_WARN, "      <inspire_common:MetadataDate>%s</inspire_common:MetadataDate>\n", "");
    if( service == OWS_WFS || service == OWS_WCS )
        msIO_fprintf(stream,"    <inspire_common:SpatialDataServiceType>download</inspire_common:SpatialDataServiceType>\n");
    else
        msIO_fprintf(stream,"    <inspire_common:SpatialDataServiceType>view</inspire_common:SpatialDataServiceType>\n");
    msOWSPrintEncodeMetadata(stream, &(map->web.metadata), namespaces, "inspire_keyword", OWS_WARN, "    <inspire_common:MandatoryKeyword>\n      <inspire_common:KeywordValue>%s</inspire_common:KeywordValue>\n    </inspire_common:MandatoryKeyword>\n", "");
  } else {
    status = action_if_not_found;
    if (OWS_WARN == action_if_not_found) {
      msIO_fprintf(stream, "<!-- WARNING: invalid value '%s' for 'inspire_capabilities', only 'embed' and 'url' are supported. -->\n", inspire_capabilities);
    }
  }

  return status;
}

/* msOWSPrintInspireCommonLanguages
**
** Output INSPIRE supported languages block to stream
**
** Returns a status code; MS_NOERR if all OK; action_if_not_found otherwise
*/
int msOWSPrintInspireCommonLanguages(FILE *stream, mapObj *map, const char *namespaces,
                                     int action_if_not_found, const char *validated_language)
{
  char *buffer = NULL; /* temp variable for malloced strings that will need freeing */
  int status = MS_NOERR;

  char *default_language = msOWSGetLanguageFromList(map, namespaces, NULL);

  if(validated_language && validated_language[0] && default_language) {
    msIO_fprintf(stream, "    <inspire_common:SupportedLanguages>\n");
    msIO_fprintf(stream, "      <inspire_common:DefaultLanguage><inspire_common:Language>%s"
                 "</inspire_common:Language></inspire_common:DefaultLanguage>\n",
                 buffer = msEncodeHTMLEntities(default_language));
    msFree(buffer);
    
    /* append _exclude to our default_language*/
    default_language = msSmallRealloc(default_language,strlen(default_language)+strlen("_exclude")+1);
    strcat(default_language,"_exclude");

    msOWSPrintEncodeMetadataList(stream, &(map->web.metadata), namespaces, "languages", NULL, NULL,
                                 "      <inspire_common:SupportedLanguage><inspire_common:Language>%s"
                                 "</inspire_common:Language></inspire_common:SupportedLanguage>\n", default_language);
    msIO_fprintf(stream, "    </inspire_common:SupportedLanguages>\n");
    msIO_fprintf(stream, "    <inspire_common:ResponseLanguage><inspire_common:Language>%s"
                 "</inspire_common:Language></inspire_common:ResponseLanguage>\n", validated_language);
  } else {
    status = action_if_not_found;
    if (OWS_WARN == action_if_not_found) {
      char* pszExpandedMetadataKey = msOWSGetExpandedMetadataKey(namespaces, "languages");
      msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata %s was missing in this context. -->\n", pszExpandedMetadataKey);
      msFree(pszExpandedMetadataKey);
    }
  }

  msFree(default_language);

  return status;
}

/*
** msOWSPrintMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not
** found then one of a number of predefined actions will be taken.
** If a default value is provided and metadata is absent then the
** default will be used.
*/

int msOWSPrintMetadata(FILE *stream, hashTableObj *metadata,
                       const char *namespaces, const char *name,
                       int action_if_not_found, const char *format,
                       const char *default_value)
{
  const char *value = NULL;
  int status = MS_NOERR;

  if((value = msOWSLookupMetadata(metadata, namespaces, name)) != NULL) {
    msIO_fprintf(stream, format, value);
  } else {
    if (action_if_not_found == OWS_WARN) {
      char* pszExpandedMetadataKey = msOWSGetExpandedMetadataKey(namespaces, name);
      msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata %s was missing in this context. -->\n", pszExpandedMetadataKey);
      msFree(pszExpandedMetadataKey);
      status = action_if_not_found;
    }

    if (default_value)
      msIO_fprintf(stream, format, default_value);
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

int msOWSPrintEncodeMetadata(FILE *stream, hashTableObj *metadata,
                             const char *namespaces, const char *name,
                             int action_if_not_found,
                             const char *format, const char *default_value)
{
  return msOWSPrintEncodeMetadata2(stream, metadata, namespaces, name, action_if_not_found, format, default_value, NULL);
}


/*
** msOWSPrintEncodeMetadata2()
**
** Attempt to output a capability item in the requested language.
** Fallback using no language parameter.
*/
int msOWSPrintEncodeMetadata2(FILE *stream, hashTableObj *metadata,
                              const char *namespaces, const char *name,
                              int action_if_not_found,
                              const char *format, const char *default_value,
                              const char *validated_language)
{
  const char *value;
  char * pszEncodedValue=NULL;
  int status = MS_NOERR;

  if((value = msOWSLookupMetadataWithLanguage(metadata, namespaces, name, validated_language))) {
    pszEncodedValue = msEncodeHTMLEntities(value);
    msIO_fprintf(stream, format, pszEncodedValue);
    free(pszEncodedValue);
  } else {
    if (action_if_not_found == OWS_WARN) {
      char* pszExpandedName = msStringConcatenate(NULL, name);
      if( validated_language && validated_language[0] )
      {
          pszExpandedName = msStringConcatenate(pszExpandedName, ".");
          pszExpandedName = msStringConcatenate(pszExpandedName, validated_language);
      }
      char* pszExpandedMetadataKey = msOWSGetExpandedMetadataKey(namespaces, pszExpandedName);
      msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata %s was missing in this context. -->\n", pszExpandedMetadataKey);
      msFree(pszExpandedName);
      msFree(pszExpandedMetadataKey);
      status = action_if_not_found;
    }

    if (default_value) {
      pszEncodedValue = msEncodeHTMLEntities(default_value);
      msIO_fprintf(stream, format, default_value);
      free(pszEncodedValue);
    }
  }

  return status;
}


/*
** msOWSGetEncodeMetadata()
**
** Equivalent to msOWSPrintEncodeMetadata. Returns en encoded value of the
** metadata or the default value.
** Caller should free the returned string.
*/
char *msOWSGetEncodeMetadata(hashTableObj *metadata,
                             const char *namespaces, const char *name,
                             const char *default_value)
{
  const char *value;
  char * pszEncodedValue=NULL;
  if((value = msOWSLookupMetadata(metadata, namespaces, name)))
    pszEncodedValue = msEncodeHTMLEntities(value);
  else if (default_value)
    pszEncodedValue = msEncodeHTMLEntities(default_value);

  return pszEncodedValue;
}


/*
** msOWSPrintValidateMetadata()
**
** Attempt to output a capability item.  If corresponding metadata is not
** found then one of a number of predefined actions will be taken.
** If a default value is provided and metadata is absent then the
** default will be used.
** Also validate the value with msIsXMLTagValid.
*/

int msOWSPrintValidateMetadata(FILE *stream, hashTableObj *metadata,
                               const char *namespaces, const char *name,
                               int action_if_not_found,
                               const char *format, const char *default_value)
{
  const char *value;
  int status = MS_NOERR;

  if((value = msOWSLookupMetadata(metadata, namespaces, name))) {
    if(msIsXMLTagValid(value) == MS_FALSE)
      msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid in a "
                   "XML tag context. -->\n", value);
    msIO_fprintf(stream, format, value);
  } else {
    if (action_if_not_found == OWS_WARN) {
      char* pszExpandedMetadataKey = msOWSGetExpandedMetadataKey(namespaces, name);
      msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata %s was missing in this context. -->\n", pszExpandedMetadataKey);
      msFree(pszExpandedMetadataKey);
      status = action_if_not_found;
    }

    if (default_value) {
      if(msIsXMLTagValid(default_value) == MS_FALSE)
        msIO_fprintf(stream, "<!-- WARNING: The value '%s' is not valid "
                     "in a XML tag context. -->\n", default_value);
      msIO_fprintf(stream, format, default_value);
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
                            const char *namespaces, const char *name,
                            int action_if_not_found,
                            const char *format, const char *default_value)
{
  return msOWSPrintGroupMetadata2(stream, map, pszGroupName, namespaces, name, action_if_not_found, format, default_value, NULL);
}

/*
** msOWSPrintGroupMetadata2()
**
** Attempt to output a capability item in the requested language.
** Fallback using no language parameter.
*/
int msOWSPrintGroupMetadata2(FILE *stream, mapObj *map, char* pszGroupName,
                             const char *namespaces, const char *name,
                             int action_if_not_found,
                             const char *format, const char *default_value,
                             const char *validated_language)
{
  const char *value;
  char *encoded;
  int status = MS_NOERR;
  int i;

  for (i=0; i<map->numlayers; i++) {
    if (GET_LAYER(map, i)->group && (strcmp(GET_LAYER(map, i)->group, pszGroupName) == 0) && &(GET_LAYER(map, i)->metadata)) {
      if((value = msOWSLookupMetadataWithLanguage(&(GET_LAYER(map, i)->metadata), namespaces, name, validated_language))) {
        encoded = msEncodeHTMLEntities(value);
        msIO_fprintf(stream, format, encoded);
        msFree(encoded);
        return status;
      }
    }
  }

  if (action_if_not_found == OWS_WARN) {
    char* pszExpandedMetadataKey = msOWSGetExpandedMetadataKey(namespaces, name);
    msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata %s was missing in this context. -->\n", pszExpandedMetadataKey);
    msFree(pszExpandedMetadataKey);
    status = action_if_not_found;
  }

  if (default_value) {
    encoded = msEncodeHTMLEntities(default_value);
    msIO_fprintf(stream, format, encoded);
    msFree(encoded);
  }

  return status;
}

/* msOWSPrintURLType()
**
** Attempt to output a URL item in capabilties.  If corresponding metadata
** is not found then one of a number of predefined actions will be taken.
** Since it's a capability item, five metadata will be used to populate the
** XML elements.
**
** The 'name' argument is the basename of the metadata items relating to this
** URL type and the suffixes _type, _width, _height, _format and _href will
** be appended to the name in the metadata search.
** e.g. passing name=metadataurl will result in the following medata entries
** being used:
**    ows_metadataurl_type
**    ows_metadataurl_format
**    ows_metadataurl_href
**    ... (width and height are unused for metadata)
**
** As for all the msOWSPrint*() functions, the namespace argument specifies
** which prefix (ows_, wms_, wcs_, etc.) is used for the metadata names above.
**
** Then the final string will be built from
** the tag_name and the five metadata. The template is:
** <tag_name%type%width%height%format>%href</tag_name>
**
** For example the width format will usually be " width=\"%s\"".
** An extern format will be "> <Format>%s</Format"
**
** Another template template may be used, but it needs to contains 5 %s,
** otherwise leave it to NULL. If tag_format is used then you don't need the
** tag_name and the tabspace.
**
** Note that all values will be HTML-encoded.
**/
int msOWSPrintURLType(FILE *stream, hashTableObj *metadata,
                      const char *namespaces, const char *name,
                      int action_if_not_found, const char *tag_format,
                      const char *tag_name, const char *type_format,
                      const char *width_format, const char *height_format,
                      const char *urlfrmt_format, const char *href_format,
                      int type_is_mandatory, int width_is_mandatory,
                      int height_is_mandatory, int format_is_mandatory,
                      int href_is_mandatory, const char *default_type,
                      const char *default_width, const char *default_height,
                      const char *default_urlfrmt, const char *default_href,
                      const char *tabspace)
{
  const char *value;
  char *metadata_name;
  char *encoded;
  int status = MS_NOERR;
  char *type=NULL, *width=NULL, *height=NULL, *urlfrmt=NULL, *href=NULL;

  const size_t buffer_size = strlen(name)+10;
  metadata_name = (char*)malloc(buffer_size);

  /* Get type */
  if(type_format != NULL) {
    snprintf(metadata_name, buffer_size, "%s_type", name);
    value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
    if(value != NULL) {
      encoded = msEncodeHTMLEntities(value);
      const size_t buffer_size_tmp = strlen(type_format)+strlen(encoded)+1;
      type = (char*)malloc(buffer_size_tmp);
      snprintf(type, buffer_size_tmp, type_format, encoded);
      msFree(encoded);
    }
  }

  /* Get width */
  if(width_format != NULL) {
    snprintf(metadata_name, buffer_size, "%s_width", name);
    value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
    if(value != NULL) {
      encoded = msEncodeHTMLEntities(value);
      const size_t buffer_size_tmp = strlen(width_format)+strlen(encoded)+1;
      width = (char*)malloc(buffer_size_tmp);
      snprintf(width, buffer_size_tmp, width_format, encoded);
      msFree(encoded);
    }
  }

  /* Get height */
  if(height_format != NULL) {
    snprintf(metadata_name, buffer_size, "%s_height", name);
    value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
    if(value != NULL) {
      encoded = msEncodeHTMLEntities(value);
      const size_t buffer_size_tmp = strlen(height_format)+strlen(encoded)+1;
      height = (char*)malloc(buffer_size_tmp);
      snprintf(height, buffer_size_tmp, height_format, encoded);
      msFree(encoded);
    }
  }

  /* Get format */
  if(urlfrmt_format != NULL) {
    snprintf(metadata_name, buffer_size, "%s_format", name);
    value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
    if(value != NULL) {
      encoded = msEncodeHTMLEntities(value);
      const size_t buffer_size_tmp = strlen(urlfrmt_format)+strlen(encoded)+1;
      urlfrmt = (char*)malloc(buffer_size_tmp);
      snprintf(urlfrmt, buffer_size_tmp, urlfrmt_format, encoded);
      msFree(encoded);
    }
  }

  /* Get href */
  if(href_format != NULL) {
    snprintf(metadata_name, buffer_size, "%s_href", name);
    value = msOWSLookupMetadata(metadata, namespaces, metadata_name);
    if(value != NULL) {
      encoded = msEncodeHTMLEntities(value);
      const size_t buffer_size_tmp = strlen(href_format)+strlen(encoded)+1;
      href = (char*)malloc(buffer_size_tmp);
      snprintf(href, buffer_size_tmp, href_format, encoded);
      msFree(encoded);
    }
  }

  msFree(metadata_name);

  if(type || width || height || urlfrmt || href ||
      (!metadata && (default_type || default_width || default_height ||
                     default_urlfrmt || default_href))) {
    if((!type && type_is_mandatory) || (!width && width_is_mandatory) ||
        (!height && height_is_mandatory) ||
        (!urlfrmt && format_is_mandatory) || (!href && href_is_mandatory)) {
      msIO_fprintf(stream, "<!-- WARNING: Some mandatory elements for '%s' are missing in this context. -->\n", tag_name);
      if (action_if_not_found == OWS_WARN) {
        char* pszExpandedMetadataKey = msOWSGetExpandedMetadataKey(namespaces, name);
        msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata %s was missing in this context. -->\n", pszExpandedMetadataKey);
        msFree(pszExpandedMetadataKey);
        status = action_if_not_found;
      }
    } else {
      if(!type && type_format && default_type) {
        const size_t buffer_size_tmp = strlen(type_format) + strlen(default_type) + 2;
        type = (char*) malloc(buffer_size_tmp);
        snprintf(type, buffer_size_tmp, type_format, default_type);
      } else if(!type)
        type = msStrdup("");
      if(!width && width_format && default_width) {
        const size_t buffer_size_tmp = strlen(width_format) + strlen(default_width) + 2;
        width = (char*) malloc(buffer_size_tmp);
        snprintf(width, buffer_size_tmp, width_format, default_width);
      } else if(!width)
        width = msStrdup("");
      if(!height && height_format && default_height) {
        const size_t buffer_size_tmp =  strlen(height_format) + strlen(default_height) + 2;
        height = (char*) malloc(buffer_size_tmp);
        snprintf(height, buffer_size_tmp, height_format, default_height);
      } else if(!height)
        height = msStrdup("");
      if(!urlfrmt && urlfrmt_format && default_urlfrmt) {
        const size_t buffer_size_tmp = strlen(urlfrmt_format) + strlen(default_urlfrmt) + 2;
        urlfrmt = (char*) malloc(buffer_size_tmp);
        snprintf(urlfrmt, buffer_size_tmp, urlfrmt_format, default_urlfrmt);
      } else if(!urlfrmt)
        urlfrmt = msStrdup("");
      if(!href && href_format && default_href) {
        const size_t buffer_size_tmp = strlen(href_format) + strlen(default_href) + 2;
        href = (char*) malloc(buffer_size_tmp);
        snprintf(href, buffer_size_tmp, href_format, default_href);
      } else if(!href)
        href = msStrdup("");

      if(tag_format == NULL)
        msIO_fprintf(stream, "%s<%s%s%s%s%s>%s</%s>\n", tabspace,
                     tag_name, type, width, height, urlfrmt, href,
                     tag_name);
      else
        msIO_fprintf(stream, tag_format,
                     type, width, height, urlfrmt, href);
    }

    msFree(type);
    msFree(width);
    msFree(height);
    msFree(urlfrmt);
    msFree(href);
  } else {
    if (action_if_not_found == OWS_WARN) {
      char* pszExpandedMetadataKey = msOWSGetExpandedMetadataKey(namespaces, name);
      msIO_fprintf(stream, "<!-- WARNING: Mandatory metadata %s was missing in this context. -->\n", pszExpandedMetadataKey);
      msFree(pszExpandedMetadataKey);
      status = action_if_not_found;
    }
  }

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

  if(value && strlen(value) > 0) {
    msIO_fprintf(stream, format, value);
  } else {
    if (action_if_not_found == OWS_WARN) {
      msIO_fprintf(stream, "<!-- WARNING: Mandatory mapfile parameter '%s' was missing in this context. -->\n", name);
      status = action_if_not_found;
    }

    if (default_value)
      msIO_fprintf(stream, format, default_value);
  }

  return status;
}

/* msOWSPrintEncodeParam()
**
** Same as printEncodeMetadata() but applied to mapfile parameters.
**/
int msOWSPrintEncodeParam(FILE *stream, const char *name, const char *value,
                          int action_if_not_found, const char *format,
                          const char *default_value)
{
  int status = MS_NOERR;
  char *encode;

  if(value && strlen(value) > 0) {
    encode = msEncodeHTMLEntities(value);
    msIO_fprintf(stream, format, encode);
    msFree(encode);
  } else {
    if (action_if_not_found == OWS_WARN) {
      msIO_fprintf(stream, "<!-- WARNING: Mandatory mapfile parameter '%s' was missing in this context. -->\n", name);
      status = action_if_not_found;
    }

    if (default_value) {
      encode = msEncodeHTMLEntities(default_value);
      msIO_fprintf(stream, format, encode);
      msFree(encode);
    }
  }

  return status;
}

/* msOWSPrintMetadataList()
**
** Prints comma-separated lists metadata.  (e.g. keywordList)
** default_value serves 2 purposes if specified:
** - won't be printed if part of MetadataList (default_value == key"_exclude")
**  (exclusion)
** - will be printed if MetadataList is empty (fallback)
**/
int msOWSPrintMetadataList(FILE *stream, hashTableObj *metadata,
                           const char *namespaces, const char *name,
                           const char *startTag,
                           const char *endTag, const char *itemFormat,
                           const char *default_value)
{
  const char *value;

  value = msOWSLookupMetadata(metadata, namespaces, name);

  if(value == NULL) {
    value = default_value;
    default_value = NULL;
  }

  if(value != NULL) {
    char **keywords;
    int numkeywords;

    keywords = msStringSplit(value, ',', &numkeywords);
    if(keywords && numkeywords > 0) {
      int kw;
      if(startTag) msIO_fprintf(stream, "%s", startTag);
      for(kw=0; kw<numkeywords; kw++) {
        if (default_value != NULL
            && strncasecmp(keywords[kw],default_value,strlen(keywords[kw])) == 0
            && strncasecmp("_exclude",default_value+strlen(default_value)-8,8) == 0)
          continue;

        msIO_fprintf(stream, itemFormat, keywords[kw]);
      }
      if(endTag) msIO_fprintf(stream, "%s", endTag);
    }
    msFreeCharArray(keywords, numkeywords);
    return MS_TRUE;
  }
  return MS_FALSE;
}

/* msOWSPrintEncodeMetadataList()
**
** Prints comma-separated lists metadata.  (e.g. keywordList)
** This will print HTML encoded values.
** default_value serves 2 purposes if specified:
** - won't be printed if part of MetadataList (default_value == key"_exclude")
**  (exclusion)
** - will be printed if MetadataList is empty (fallback)
**/
int msOWSPrintEncodeMetadataList(FILE *stream, hashTableObj *metadata,
                                 const char *namespaces, const char *name,
                                 const char *startTag,
                                 const char *endTag, const char *itemFormat,
                                 const char *default_value)
{
  const char *value;
  char *encoded;
  size_t default_value_len = 0;

  value = msOWSLookupMetadata(metadata, namespaces, name);

  if(value == NULL) {
    value = default_value;
    default_value = NULL;
  }
  if( default_value )
      default_value_len = strlen(default_value);

  if(value != NULL) {
    char **keywords;
    int numkeywords;

    keywords = msStringSplit(value, ',', &numkeywords);
    if(keywords && numkeywords > 0) {
      int kw;
      if(startTag) msIO_fprintf(stream, "%s", startTag);
      for(kw=0; kw<numkeywords; kw++) {
        if (default_value != NULL
            && default_value_len > 8
            && strncasecmp(keywords[kw],default_value,strlen(keywords[kw])) == 0
            && strncasecmp("_exclude",default_value+default_value_len-8,8) == 0)
          continue;

        encoded = msEncodeHTMLEntities(keywords[kw]);
        msIO_fprintf(stream, itemFormat, encoded);
        msFree(encoded);
      }
      if(endTag) msIO_fprintf(stream, "%s", endTag);
    }
    msFreeCharArray(keywords, numkeywords);
    return MS_TRUE;
  }
  return MS_FALSE;
}

/* msOWSPrintEncodeParamList()
**
** Same as msOWSPrintEncodeMetadataList() but applied to mapfile parameters.
**/
int msOWSPrintEncodeParamList(FILE *stream, const char *name,
                              const char *value, int action_if_not_found,
                              char delimiter, const char *startTag,
                              const char *endTag, const char *format,
                              const char *default_value)
{
  int status = MS_NOERR;
  char *encoded;
  char **items = NULL;
  int numitems = 0, i;

  if(value && strlen(value) > 0)
    items = msStringSplit(value, delimiter, &numitems);
  else {
    if (action_if_not_found == OWS_WARN) {
      msIO_fprintf(stream, "<!-- WARNING: Mandatory mapfile parameter '%s' was missing in this context. -->\n", name);
      status = action_if_not_found;
    }

    if (default_value)
      items = msStringSplit(default_value, delimiter, &numitems);
  }

  if(items && numitems > 0) {
    if(startTag) msIO_fprintf(stream, "%s", startTag);
    for(i=0; i<numitems; i++) {
      encoded = msEncodeHTMLEntities(items[i]);
      msIO_fprintf(stream, format, encoded);
      msFree(encoded);
    }
    if(endTag) msIO_fprintf(stream, "%s", endTag);
  }
  msFreeCharArray(items, numitems);

  return status;
}


/*
** msOWSPrintEX_GeographicBoundingBox()
**
** Print a EX_GeographicBoundingBox tag for WMS1.3.0
**
*/
void msOWSPrintEX_GeographicBoundingBox(FILE *stream, const char *tabspace,
                                        rectObj *extent, projectionObj *srcproj)

{
  const char *pszTag = "EX_GeographicBoundingBox";  /* The default for WMS */
  rectObj ext;

  ext = *extent;

  /* always project to lat long */
  msOWSProjectToWGS84(srcproj, &ext);

  msIO_fprintf(stream, "%s<%s>\n", tabspace, pszTag);
  msIO_fprintf(stream, "%s    <westBoundLongitude>%.6f</westBoundLongitude>\n", tabspace, ext.minx);
  msIO_fprintf(stream, "%s    <eastBoundLongitude>%.6f</eastBoundLongitude>\n", tabspace, ext.maxx);
  msIO_fprintf(stream, "%s    <southBoundLatitude>%.6f</southBoundLatitude>\n", tabspace, ext.miny);
  msIO_fprintf(stream, "%s    <northBoundLatitude>%.6f</northBoundLatitude>\n", tabspace, ext.maxy);
  msIO_fprintf(stream, "%s</%s>\n", tabspace, pszTag);

  /* msIO_fprintf(stream, "%s<%s minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\" />\n",
                  tabspace, pszTag, ext.minx, ext.miny, ext.maxx, ext.maxy); */
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
                                 projectionObj *wfsproj, OWSServiceType nService)
{
  const char *pszTag = "LatLonBoundingBox";  /* The default for WMS */
  rectObj ext;

  ext = *extent;

  if (nService == OWS_WMS) { /* always project to lat long */
    msOWSProjectToWGS84(srcproj, &ext);
  } else if (nService == OWS_WFS) { /* called from wfs 1.0.0 only: project to map srs, if set */
    pszTag = "LatLongBoundingBox";
    if (wfsproj) {
      if (msProjectionsDiffer(srcproj, wfsproj) == MS_TRUE)
        msProjectRect(srcproj, wfsproj, &ext);
    }
  }

  msIO_fprintf(stream, "%s<%s minx=\"%.6f\" miny=\"%.6f\" maxx=\"%.6f\" maxy=\"%.6f\" />\n",
               tabspace, pszTag, ext.minx, ext.miny, ext.maxx, ext.maxy);
}

/*
** Emit a bounding box if we can find projection information.
** If <namespaces>_bbox_extended is not set, emit a single bounding box
** using the layer's native SRS (ignoring any <namespaces>_srs metadata).
**
** If <namespaces>_bbox_extended is set to true, emit a bounding box
** for every projection listed in the <namespaces>_srs list.
** Check the map level metadata for both _bbox_extended and _srs,
** if there is no such metadata at the layer level.
** (These settings make more sense at the global/map level anyways)
*/
void msOWSPrintBoundingBox(FILE *stream, const char *tabspace,
                           rectObj *extent,
                           projectionObj *srcproj,
                           hashTableObj *layer_meta,
                           hashTableObj *map_meta,
                           const char *namespaces,
                           int wms_version)
{
  const char    *value, *resx, *resy, *wms_bbox_extended;
  char *encoded, *encoded_resx, *encoded_resy, *epsg_str;
  char **epsgs;
  int i, num_epsgs;
  projectionObj proj;
  rectObj ext;

  wms_bbox_extended = msOWSLookupMetadata2(layer_meta, map_meta, namespaces, "bbox_extended");
  if( wms_bbox_extended && strncasecmp(wms_bbox_extended, "true", 5) == 0 ) {
    /* get a list of all projections from the metadata
       try the layer metadata first, otherwise use the map's */
    if( msOWSLookupMetadata(layer_meta, namespaces, "srs") ) {
      msOWSGetEPSGProj(srcproj, layer_meta, namespaces, MS_FALSE, &epsg_str);
    } else {
      msOWSGetEPSGProj(srcproj, map_meta, namespaces, MS_FALSE, &epsg_str);
    }
    epsgs = msStringSplit(epsg_str, ' ', &num_epsgs);
    msFree(epsg_str);
  } else {
    /* Look for EPSG code in PROJECTION block only.  "wms_srs" metadata cannot be
     * used to establish the native projection of a layer for BoundingBox purposes.
     */
    epsgs = (char **) msSmallMalloc(sizeof(char *));
    num_epsgs = 1;
    msOWSGetEPSGProj(srcproj, layer_meta, namespaces, MS_TRUE, &(epsgs[0]));
  }

  for( i = 0; i < num_epsgs; i++) {
    value = epsgs[i];
    if( value && *value) {
      memcpy(&ext, extent, sizeof(rectObj));

      /* reproject the extents for each SRS's bounding box */
      msInitProjection(&proj);
      msProjectionInheritContextFrom(&proj, srcproj);
      if (msLoadProjectionStringEPSG(&proj, (char *)value) == 0) {
        if (msProjectionsDiffer(srcproj, &proj) == MS_TRUE) {
          msProjectRect(srcproj, &proj, &ext);
        }
        /*for wms 1.3.0 we need to make sure that we present the BBOX with
          a reversed axes for some espg codes*/
        if (wms_version >= OWS_1_3_0 && strncasecmp(value, "EPSG:", 5) == 0) {
          msAxisNormalizePoints( &proj, 1, &(ext.minx), &(ext.miny) );
          msAxisNormalizePoints( &proj, 1, &(ext.maxx), &(ext.maxy) );
        }
      }

      encoded = msEncodeHTMLEntities(value);
      if (msProjIsGeographicCRS(&proj) )
        msIO_fprintf(stream, "%s<BoundingBox %s=\"%s\"\n"
                     "%s            minx=\"%.6f\" miny=\"%.6f\" maxx=\"%.6f\" maxy=\"%.6f\"",
                     tabspace,
                     (wms_version >= OWS_1_3_0) ? "CRS" : "SRS",
                     encoded,
                     tabspace, ext.minx, ext.miny,
                     ext.maxx, ext.maxy);
      else
        msIO_fprintf(stream, "%s<BoundingBox %s=\"%s\"\n"
                     "%s            minx=\"%g\" miny=\"%g\" maxx=\"%g\" maxy=\"%g\"",
                     tabspace,
                     (wms_version >= OWS_1_3_0) ? "CRS" : "SRS",
                     encoded,
                     tabspace, ext.minx, ext.miny,
                     ext.maxx, ext.maxy);

      msFree(encoded);
      msFreeProjection( &proj );

      if( (resx = msOWSLookupMetadata2( layer_meta, map_meta, "MFO", "resx" )) != NULL &&
          (resy = msOWSLookupMetadata2( layer_meta, map_meta, "MFO", "resy" )) != NULL ) {
        encoded_resx = msEncodeHTMLEntities(resx);
        encoded_resy = msEncodeHTMLEntities(resy);
        msIO_fprintf( stream, "\n%s            resx=\"%s\" resy=\"%s\"",
                      tabspace, encoded_resx, encoded_resy );
        msFree(encoded_resx);
        msFree(encoded_resy);
      }

      msIO_fprintf( stream, " />\n" );
    }
  }
  msFreeCharArray(epsgs, num_epsgs);
}


/*
** Print the contact information
*/
void msOWSPrintContactInfo( FILE *stream, const char *tabspace,
                            int nVersion, hashTableObj *metadata,
                            const char *namespaces )
{
  /* contact information is a required element in 1.0.7 but the */
  /* sub-elements such as ContactPersonPrimary, etc. are not! */
  /* In 1.1.0, ContactInformation becomes optional. */
  if (nVersion > OWS_1_0_0) {
    msIO_fprintf(stream, "%s<ContactInformation>\n", tabspace);

    /* ContactPersonPrimary is optional, but when present then all its  */
    /* sub-elements are mandatory */

    if(msOWSLookupMetadata(metadata, namespaces, "contactperson") ||
        msOWSLookupMetadata(metadata, namespaces, "contactorganization")) {
      msIO_fprintf(stream, "%s  <ContactPersonPrimary>\n", tabspace);

      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "contactperson",
                               OWS_WARN, "      <ContactPerson>%s</ContactPerson>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "contactorganization",
                               OWS_WARN, "      <ContactOrganization>%s</ContactOrganization>\n",
                               NULL);
      msIO_fprintf(stream, "%s  </ContactPersonPrimary>\n", tabspace);
    }

    if(msOWSLookupMetadata(metadata, namespaces, "contactposition")) {
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "contactposition",
                               OWS_NOERR, "      <ContactPosition>%s</ContactPosition>\n",
                               NULL);
    }

    /* ContactAdress is optional, but when present then all its  */
    /* sub-elements are mandatory */
    if(msOWSLookupMetadata( metadata, namespaces, "addresstype" ) ||
        msOWSLookupMetadata( metadata, namespaces, "address" ) ||
        msOWSLookupMetadata( metadata, namespaces, "city" ) ||
        msOWSLookupMetadata( metadata, namespaces, "stateorprovince" ) ||
        msOWSLookupMetadata( metadata, namespaces, "postcode" ) ||
        msOWSLookupMetadata( metadata, namespaces, "country" )) {
      msIO_fprintf(stream, "%s  <ContactAddress>\n", tabspace);

      msOWSPrintEncodeMetadata(stream, metadata, namespaces,"addresstype", OWS_WARN,
                               "        <AddressType>%s</AddressType>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "address", OWS_WARN,
                               "        <Address>%s</Address>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "city", OWS_WARN,
                               "        <City>%s</City>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "stateorprovince",
                               OWS_WARN,"        <StateOrProvince>%s</StateOrProvince>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "postcode", OWS_WARN,
                               "        <PostCode>%s</PostCode>\n", NULL);
      msOWSPrintEncodeMetadata(stream, metadata, namespaces, "country", OWS_WARN,
                               "        <Country>%s</Country>\n", NULL);
      msIO_fprintf(stream, "%s  </ContactAddress>\n", tabspace);
    }

    if(msOWSLookupMetadata(metadata, namespaces, "contactvoicetelephone")) {
      msOWSPrintEncodeMetadata(stream, metadata, namespaces,
                               "contactvoicetelephone", OWS_NOERR,
                               "      <ContactVoiceTelephone>%s</ContactVoiceTelephone>\n",
                               NULL);
    }

    if(msOWSLookupMetadata(metadata, namespaces, "contactfacsimiletelephone")) {
      msOWSPrintEncodeMetadata(stream, metadata,
                               namespaces, "contactfacsimiletelephone", OWS_NOERR,
                               "      <ContactFacsimileTelephone>%s</ContactFacsimileTelephone>\n",
                               NULL);
    }

    if(msOWSLookupMetadata(metadata, namespaces, "contactelectronicmailaddress")) {
      msOWSPrintEncodeMetadata(stream, metadata,
                               namespaces, "contactelectronicmailaddress", OWS_NOERR,
                               "  <ContactElectronicMailAddress>%s</ContactElectronicMailAddress>\n",
                               NULL);
    }
    msIO_fprintf(stream, "%s</ContactInformation>\n", tabspace);
  }
}

/*
** msOWSGetLayerExtent()
**
** Try to establish layer extent, first looking for "ows_extent" metadata, and
** if not found then call msLayerGetExtent() which will lookup the
** layer->extent member, and if not found will open layer to read extent.
**
*/
int msOWSGetLayerExtent(mapObj *map, layerObj *lp, const char *namespaces, rectObj *ext)
{
  (void)map;
  const char *value;

  if ((value = msOWSLookupMetadata(&(lp->metadata), namespaces, "extent")) != NULL) {
    char **tokens;
    int n;

    tokens = msStringSplit(value, ' ', &n);
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
  } else {
    return msLayerGetExtent(lp, ext);
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

  /* Execute requests */
#if defined(USE_CURL)
  nStatus = msHTTPExecuteRequests(pasReqInfo, numRequests, bCheckLocalCache);
#else
  msSetError(MS_WMSERR, "msOWSExecuteRequests() called apparently without libcurl configured, msHTTPExecuteRequests() not available.",
             "msOWSExecuteRequests()");
  return MS_FAILURE;
#endif

  /* Scan list of layers and call the handler for each layer type to */
  /* pass them the request results. */
  for(iReq=0; iReq<numRequests; iReq++) {
    if (pasReqInfo[iReq].nLayerId >= 0 &&
        pasReqInfo[iReq].nLayerId < map->numlayers) {
      layerObj *lp;

      lp = GET_LAYER(map, pasReqInfo[iReq].nLayerId);

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

  if ((fp = fopen(pszFname, "r")) != NULL) {
    char *pszBuf=NULL;
    int   nBufSize=0;
    char *pszStart, *pszEnd;

    fseek(fp, 0, SEEK_END);
    nBufSize = ftell(fp);
    if(nBufSize < 0) {
      msSetError(MS_IOERR, NULL, "msOWSProcessException()");
      fclose(fp);
      return;
    }
    rewind(fp);
    pszBuf = (char*)malloc((nBufSize+1)*sizeof(char));
    if (pszBuf == NULL) {
      msSetError(MS_MEMERR, NULL, "msOWSProcessException()");
      fclose(fp);
      return;
    }

    if ((int) fread(pszBuf, 1, nBufSize, fp) != nBufSize) {
      msSetError(MS_IOERR, NULL, "msOWSProcessException()");
      free(pszBuf);
      fclose(fp);
      return;
    }

    pszBuf[nBufSize] = '\0';


    /* OK, got the data in the buffer.  Look for the <Message> tags */
    if ((strstr(pszBuf, "<WFS_Exception>") &&            /* WFS style */
         (pszStart = strstr(pszBuf, "<Message>")) &&
         (pszEnd = strstr(pszStart, "</Message>")) ) ||
        (strstr(pszBuf, "<ServiceExceptionReport>") &&   /* WMS style */
         (pszStart = strstr(pszBuf, "<ServiceException>")) &&
         (pszEnd = strstr(pszStart, "</ServiceException>")) )) {
      pszStart = strchr(pszStart, '>')+1;
      *pszEnd = '\0';
      msSetError(nErrorCode, "Got Remote Server Exception for layer %s: %s",
                 pszFuncName, lp->name?lp->name:"(null)", pszStart);
    } else {
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
  int  i;
  size_t nBufLen = 0;


  nBufLen = strlen(pszURL) + strlen(pszExt) +2;
  if (pszPath)
    nBufLen += (strlen(pszPath)+1);

  pszBuf = (char*)malloc(nBufLen);
  if (pszBuf == NULL) {
    msSetError(MS_MEMERR, NULL, "msOWSBuildURLFilename()");
    return NULL;
  }
  pszBuf[0] = '\0';

  if (pszPath) {
#ifdef _WIN32
    if (pszPath[strlen(pszPath) -1] != '/' &&
        pszPath[strlen(pszPath) -1] != '\\')
      snprintf(pszBuf, nBufLen, "%s\\", pszPath);
    else
      snprintf(pszBuf, nBufLen, "%s", pszPath);
#else
    if (pszPath[strlen(pszPath) -1] != '/')
      snprintf(pszBuf, nBufLen, "%s/", pszPath);
    else
      snprintf(pszBuf, nBufLen, "%s", pszPath);
#endif
  }

  pszPtr = pszBuf + strlen(pszBuf);

  for(i=0; pszURL[i] != '\0'; i++) {
    if (isalnum(pszURL[i]))
      *pszPtr = pszURL[i];
    else
      *pszPtr = '_';
    pszPtr++;
  }

  strlcpy(pszPtr, pszExt, nBufLen);

  return pszBuf;
}

/*
** msOWSGetProjURN()
**
** Fetch an OGC URN for this layer or map.  Similar to msOWSGetEPSGProj()
** but returns the result in the form "urn:ogc:def:crs:EPSG::27700".
** The returned buffer is dynamically allocated, and must be freed by the
** caller.
*/
char *msOWSGetProjURN(projectionObj *proj, hashTableObj *metadata, const char *namespaces, int bReturnOnlyFirstOne)
{
  char *result;
  char **tokens;
  int numtokens, i;
  char *oldStyle = NULL;
  
  msOWSGetEPSGProj( proj, metadata, namespaces, bReturnOnlyFirstOne, &oldStyle );

  if( oldStyle == NULL ||
      strncmp(oldStyle,"CRS:",4) == 0 ||
      strncmp(oldStyle,"AUTO:",5) == 0 ||
      strncmp(oldStyle,"AUTO2:",6) == 0 ) {
    msFree(oldStyle);
    return NULL;
  }

  result = msStrdup("");

  tokens = msStringSplit(oldStyle, ' ', &numtokens);
  msFree(oldStyle);
  for(i=0; tokens != NULL && i<numtokens; i++) {
    char urn[100];
    char* colon = strchr(tokens[i], ':');

    if( colon != NULL && strchr(colon + 1, ':') == NULL ) {
      *colon = 0;
      snprintf( urn, sizeof(urn), "urn:ogc:def:crs:%s::%s", tokens[i], colon + 1 );
    }
    else if( strcasecmp(tokens[i],"imageCRS") == 0 )
      snprintf( urn, sizeof(urn), "urn:ogc:def:crs:OGC::imageCRS" );
    else if( strncmp(tokens[i],"urn:ogc:def:crs:",16) == 0 ) {
      strlcpy( urn, tokens[i], sizeof(urn));
    } else {
      strlcpy( urn, "", sizeof(urn));
    }

    if( strlen(urn) > 0 ) {
      const size_t bufferSize = strlen(result)+strlen(urn)+2;
      result = (char *) msSmallRealloc(result, bufferSize);

      if( strlen(result) > 0 )
        strlcat( result, " ", bufferSize);
      strlcat( result, urn , bufferSize);
    } else {
      msDebug( "msOWSGetProjURN(): Failed to process SRS '%s', ignored.",
               tokens[i] );
    }
  }

  msFreeCharArray(tokens, numtokens);

  if( strlen(result) == 0 ) {
    msFree( result );
    return NULL;
  } else
    return result;
}

/*
** msOWSGetProjURI()
**
** Fetch an OGC URI for this layer or map.  Similar to msOWSGetEPSGProj()
** but returns the result in the form "http://www.opengis.net/def/crs/EPSG/0/27700".
** The returned buffer is dynamically allocated, and must be freed by the
** caller.
*/
char *msOWSGetProjURI(projectionObj *proj, hashTableObj *metadata, const char *namespaces, int bReturnOnlyFirstOne)
{
  char *result;
  char **tokens;
  int numtokens, i;
  char *oldStyle = NULL;
  
  msOWSGetEPSGProj( proj, metadata, namespaces, bReturnOnlyFirstOne, &oldStyle);

  if( oldStyle == NULL || !EQUALN(oldStyle,"EPSG:",5) ) {
    msFree(oldStyle); // avoid leak
    return NULL;
  }

  result = msStrdup("");

  tokens = msStringSplit(oldStyle, ' ', &numtokens);
  msFree(oldStyle);
  for(i=0; tokens != NULL && i<numtokens; i++) {
    char urn[100];

    if( strncmp(tokens[i],"EPSG:",5) == 0 )
      snprintf( urn, sizeof(urn), "http://www.opengis.net/def/crs/EPSG/0/%s", tokens[i]+5 );
    else if( strcasecmp(tokens[i],"imageCRS") == 0 )
      snprintf( urn, sizeof(urn), "http://www.opengis.net/def/crs/OGC/0/imageCRS" );
    else if( strncmp(tokens[i],"http://www.opengis.net/def/crs/",16) == 0 )
      snprintf( urn, sizeof(urn), "%s", tokens[i] );
    else
      strlcpy( urn, "", sizeof(urn) );

    if( strlen(urn) > 0 ) {
      result = (char *) msSmallRealloc(result,strlen(result)+strlen(urn)+2);

      if( strlen(result) > 0 )
        strcat( result, " " );
      strcat( result, urn );
    } else {
      msDebug( "msOWSGetProjURI(): Failed to process SRS '%s', ignored.",
               tokens[i] );
    }
  }

  msFreeCharArray(tokens, numtokens);

  if( strlen(result) == 0 ) {
    msFree( result );
    return NULL;
  } else
    return result;
}


/*
** msOWSGetDimensionInfo()
**
** Extract dimension information from a layer's metadata
**
** Before 4.9, only the time dimension was support. With the addition of
** Web Map Context 1.1.0, we need to support every dimension types.
** This function get the dimension information from special metadata in
** the layer, but can also return default values for the time dimension.
**
*/
void msOWSGetDimensionInfo(layerObj *layer, const char *pszDimension,
                           const char **papszDimUserValue,
                           const char **papszDimUnits,
                           const char **papszDimDefault,
                           const char **papszDimNearValue,
                           const char **papszDimUnitSymbol,
                           const char **papszDimMultiValue)
{
  char *pszDimensionItem;
  size_t bufferSize = 0;

  if(pszDimension == NULL || layer == NULL)
    return;

  bufferSize = strlen(pszDimension)+50;
  pszDimensionItem = (char*)malloc(bufferSize);

  /* units (mandatory in map context) */
  if(papszDimUnits != NULL) {
    snprintf(pszDimensionItem, bufferSize, "dimension_%s_units",          pszDimension);
    *papszDimUnits = msOWSLookupMetadata(&(layer->metadata), "MO",
                                         pszDimensionItem);
  }
  /* unitSymbol (mandatory in map context) */
  if(papszDimUnitSymbol != NULL) {
    snprintf(pszDimensionItem, bufferSize, "dimension_%s_unitsymbol",     pszDimension);
    *papszDimUnitSymbol = msOWSLookupMetadata(&(layer->metadata), "MO",
                          pszDimensionItem);
  }
  /* userValue (mandatory in map context) */
  if(papszDimUserValue != NULL) {
    snprintf(pszDimensionItem, bufferSize, "dimension_%s_uservalue",      pszDimension);
    *papszDimUserValue = msOWSLookupMetadata(&(layer->metadata), "MO",
                         pszDimensionItem);
  }
  /* default */
  if(papszDimDefault != NULL) {
    snprintf(pszDimensionItem, bufferSize, "dimension_%s_default",        pszDimension);
    *papszDimDefault = msOWSLookupMetadata(&(layer->metadata), "MO",
                                           pszDimensionItem);
  }
  /* multipleValues */
  if(papszDimMultiValue != NULL) {
    snprintf(pszDimensionItem, bufferSize, "dimension_%s_multiplevalues", pszDimension);
    *papszDimMultiValue = msOWSLookupMetadata(&(layer->metadata), "MO",
                          pszDimensionItem);
  }
  /* nearestValue */
  if(papszDimNearValue != NULL) {
    snprintf(pszDimensionItem, bufferSize, "dimension_%s_nearestvalue",   pszDimension);
    *papszDimNearValue = msOWSLookupMetadata(&(layer->metadata), "MO",
                         pszDimensionItem);
  }

  /* Use default time value if necessary */
  if(strcasecmp(pszDimension, "time") == 0) {
    if(papszDimUserValue != NULL && *papszDimUserValue == NULL)
      *papszDimUserValue = msOWSLookupMetadata(&(layer->metadata),
                           "MO", "time");
    if(papszDimDefault != NULL && *papszDimDefault == NULL)
      *papszDimDefault = msOWSLookupMetadata(&(layer->metadata),
                                             "MO", "timedefault");
    if(papszDimUnits != NULL && *papszDimUnits == NULL)
      *papszDimUnits = "ISO8601";
    if(papszDimUnitSymbol != NULL && *papszDimUnitSymbol == NULL)
      *papszDimUnitSymbol = "t";
    if(papszDimNearValue != NULL && *papszDimNearValue == NULL)
      *papszDimNearValue = "0";
  }

  free(pszDimensionItem);

  return;
}

/**
 * msOWSNegotiateUpdateSequence()
 *
 * returns the updateSequence value for an OWS
 *
 * @param requested_updatesequence the updatesequence passed by the client
 * @param updatesequence the updatesequence set by the server
 *
 * @return result of comparison (-1, 0, 1)
 * -1: lower / higher OR values not set by client or server
 *  1: higher / lower
 *  0: equal
 */

int msOWSNegotiateUpdateSequence(const char *requested_updatesequence, const char *updatesequence)
{
  int i;
  int valtype1 = 1; /* default datatype for updatesequence passed by client */
  int valtype2 = 1; /* default datatype for updatesequence set by server */
  struct tm tm_requested_updatesequence, tm_updatesequence;

  /* if not specified by client, or set by server,
     server responds with latest Capabilities XML */
  if (! requested_updatesequence || ! updatesequence)
    return -1;

  /* test to see if server value is an integer (1), string (2) or timestamp (3) */
  if (msStringIsInteger(updatesequence) == MS_FAILURE)
    valtype1 = 2;

  if (valtype1 == 2) { /* test if timestamp */
    msTimeInit(&tm_updatesequence);
    if (msParseTime(updatesequence, &tm_updatesequence) == MS_TRUE)
      valtype1 = 3;
    msResetErrorList();
  }

  /* test to see if client value is an integer (1), string (2) or timestamp (3) */
  if (msStringIsInteger(requested_updatesequence) == MS_FAILURE)
    valtype2 = 2;

  if (valtype2 == 2) { /* test if timestamp */
    msTimeInit(&tm_requested_updatesequence);
    if (msParseTime(requested_updatesequence, &tm_requested_updatesequence) == MS_TRUE)
      valtype2 = 3;
    msResetErrorList();
  }

  /* if the datatypes do not match, do not compare, */
  if (valtype1 != valtype2)
    return -1;

  if (valtype1 == 1) { /* integer */
    if (atoi(requested_updatesequence) < atoi(updatesequence))
      return -1;

    if (atoi(requested_updatesequence) > atoi(updatesequence))
      return 1;

    if (atoi(requested_updatesequence) == atoi(updatesequence))
      return 0;
  }

  if (valtype1 == 2) /* string */
    return strcasecmp(requested_updatesequence, updatesequence);

  if (valtype1 == 3) { /* timestamp */
    /* compare timestamps */
    i = msDateCompare(&tm_requested_updatesequence, &tm_updatesequence) +
        msTimeCompare(&tm_requested_updatesequence, &tm_updatesequence);
    return i;
  }

  /* return default -1 */
  return -1;
}


/************************************************************************/
/*                         msOwsIsOutputFormatValid                     */
/*                                                                      */
/*      Utlity function to parse a comma separated list in a            */
/*      metedata object and select and outputformat.                    */
/************************************************************************/
outputFormatObj* msOwsIsOutputFormatValid(mapObj *map, const char *format,
    hashTableObj *metadata,
    const char *namespaces, const char *name)
{
  char **tokens=NULL;
  int i,n;
  outputFormatObj *psFormat = NULL;
  const char * format_list=NULL;

  if (map && format && metadata && namespaces && name) {
    msApplyDefaultOutputFormats(map);
    format_list = msOWSLookupMetadata(metadata, namespaces, name);
    n = 0;
    if ( format_list)
      tokens = msStringSplit(format_list,  ',', &n);

    if (tokens && n > 0) {
      for (i=0; i<n; i++) {
        int iFormat = msGetOutputFormatIndex( map, tokens[i]);
        const char *mimetype;
        if( iFormat == -1 )
          continue;

        mimetype =  map->outputformatlist[iFormat]->mimetype;

        msStringTrim(tokens[i]);
        if (strcasecmp(tokens[i], format) == 0)
          break;
        if (mimetype && strcasecmp(mimetype, format) == 0)
          break;
      }
      if (i < n)
        psFormat = msSelectOutputFormat( map, format);
    }
    if(tokens)
      msFreeCharArray(tokens, n);
  }

  return psFormat;
}

#endif /* defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR) || defined(USE_WMS_LYR) || defined(USE_WFS_LYR) */

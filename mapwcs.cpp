/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OpenGIS Web Coverage Server (WCS) Implementation.
 * Author:   Steve Lime and the MapServer team.
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
 *****************************************************************************/

#include "mapserver.h"
#include "maperror.h"
#include "mapthread.h"
#include "mapows.h"
#include <assert.h>

#include <string>

#if defined(USE_WCS_SVR)

#include "mapwcs.h"

#include "maptime.h"
#include <time.h>

#include "gdal.h"
#include "cpl_string.h" /* GDAL string handling */

/************************************************************************/
/*                    msWCSValidateRangeSetParam()                      */
/************************************************************************/
static int msWCSValidateRangeSetParam(layerObj *lp, char *name, const char *value)
{
  char **allowed_ri_values;
  char **client_ri_values;
  int  allowed_count, client_count;
  int  i_client, i, all_match = 1;
  char *tmpname = NULL;
  const char *ri_values_list;

  if( name == NULL )
    return MS_FAILURE;

  /* Fetch the available values list for the rangeset item and tokenize */
  tmpname = (char *)msSmallMalloc(sizeof(char)*strlen(name) + 10);
  sprintf(tmpname,"%s_values", name);
  ri_values_list = msOWSLookupMetadata(&(lp->metadata), "CO", tmpname);
  msFree( tmpname );

  if (ri_values_list == NULL)
    return MS_FAILURE;

  allowed_ri_values = msStringSplit( ri_values_list, ',', &allowed_count);

  /* Parse the client value list into tokens. */
  client_ri_values = msStringSplit( value, ',', &client_count );

  /* test each client value against the allowed list. */

  for( i_client = 0; all_match && i_client < client_count; i_client++ ) {
    for( i = 0;
         i < allowed_count
         && strcasecmp(client_ri_values[i_client],
                       allowed_ri_values[i]) != 0;
         i++ ) {}

    if( i == allowed_count )
      all_match = 0;
  }

  msFreeCharArray(allowed_ri_values, allowed_count );
  msFreeCharArray(client_ri_values, client_count );

  if (all_match == 0)
    return MS_FAILURE;
  else
    return MS_SUCCESS;
}

/************************************************************************/
/*                    msWCSConvertRangeSetToString()                    */
/************************************************************************/
static char *msWCSConvertRangeSetToString(const char *value)
{
  char **tokens;
  int numtokens;
  double min, max, res;
  double val;
  char buf1[128], *buf2=NULL;

  if(strchr(value, '/')) { /* value is min/max/res */
    tokens = msStringSplit(value, '/', &numtokens);
    if(tokens==NULL || numtokens != 3) {
      msFreeCharArray(tokens, numtokens);
      return NULL; /* not a set of equally spaced intervals */
    }

    min = atof(tokens[0]);
    max = atof(tokens[1]);
    res = atof(tokens[2]);
    msFreeCharArray(tokens, numtokens);

    for(val=min; val<=max; val+=res) {
      if(val == min)
        snprintf(buf1, sizeof(buf1), "%g", val);
      else
        snprintf(buf1, sizeof(buf1), ",%g", val);
      buf2 = msStringConcatenate(buf2, buf1);
    }

    return buf2;
  } else
    return msStrdup(value);
}

/************************************************************************/
/*                           msWCSException()                           */
/************************************************************************/
int msWCSException(mapObj *map, const char *code, const char *locator,
                   const char *version )
{
  char *pszEncodedVal = NULL;
  char version_string[OWS_VERSION_MAXLEN];

  if( version == NULL )
    version = "1.0.0";

#if defined(USE_LIBXML2)
  if( msOWSParseVersionString(version) >= OWS_2_0_0 )
    return msWCSException20( map, code, locator, msOWSGetVersionString(msOWSParseVersionString(version), version_string) );
#endif

  if( msOWSParseVersionString(version) >= OWS_1_1_0 )
    return msWCSException11( map, code, locator, msOWSGetVersionString(msOWSParseVersionString(version), version_string) );

  msIO_setHeader("Content-Type","application/vnd.ogc.se_xml; charset=UTF-8");
  msIO_sendHeaders();

  /* msIO_printf("Content-Type: text/xml%c%c",10,10); */

  msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" ?>\n");

  msIO_printf("<ServiceExceptionReport version=\"1.2.0\"\n");
  msIO_printf("xmlns=\"http://www.opengis.net/ogc\" ");
  msIO_printf("xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
  pszEncodedVal = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
  msIO_printf("xsi:schemaLocation=\"http://www.opengis.net/ogc %s/wcs/1.0.0/OGC-exception.xsd\">\n",
              pszEncodedVal);
  msFree(pszEncodedVal);
  msIO_printf("  <ServiceException");
  if (code) {
    msIO_printf(" code=\"%s\"", code);
  }
  if (locator) {
    msIO_printf(" locator=\"%s\"", locator);
  }
  msIO_printf(">");
  msWriteErrorXML(stdout);
  msIO_printf("  </ServiceException>\n");
  msIO_printf("</ServiceExceptionReport>\n");

  msResetErrorList();

  return MS_FAILURE;
}

/************************************************************************/
/*                    msWCSPrintRequestCapability()                     */
/************************************************************************/

static void msWCSPrintRequestCapability(const char *request_tag, const char *script_url)
{
  msIO_printf("    <%s>\n", request_tag);

  msIO_printf("      <DCPType>\n");
  msIO_printf("        <HTTP>\n");
  msIO_printf("          <Get><OnlineResource xlink:type=\"simple\" xlink:href=\"%s\" /></Get>\n", script_url);
  msIO_printf("        </HTTP>\n");
  msIO_printf("      </DCPType>\n");
  msIO_printf("      <DCPType>\n");
  msIO_printf("        <HTTP>\n");
  msIO_printf("          <Post><OnlineResource xlink:type=\"simple\" xlink:href=\"%s\" /></Post>\n", script_url);
  msIO_printf("        </HTTP>\n");
  msIO_printf("      </DCPType>\n");

  msIO_printf("    </%s>\n", request_tag);
}

/************************************************************************/
/*                         msWCSCreateParams()                          */
/************************************************************************/
static wcsParamsObj *msWCSCreateParams()
{
  wcsParamsObj *params;

  params = (wcsParamsObj *) calloc(1, sizeof(wcsParamsObj));
  MS_CHECK_ALLOC(params, sizeof(wcsParamsObj), NULL);

  return params;
}

/************************************************************************/
/*                          msWCSFreeParams()                           */
/************************************************************************/
void msWCSFreeParams(wcsParamsObj *params)
{
  if(params) {
    /* TODO */
    if(params->version) free(params->version);
    if(params->updatesequence) free(params->updatesequence);
    if(params->request) free(params->request);
    if(params->service) free(params->service);
    if(params->section) free(params->section);
    if(params->crs) free(params->crs);
    if(params->response_crs) free(params->response_crs);
    if(params->format) free(params->format);
    if(params->exceptions) free(params->exceptions);
    if(params->time) free(params->time);
    if(params->interpolation) free(params->interpolation);
    CSLDestroy( params->coverages );
  }
}

/************************************************************************/
/*                       msWCSIsLayerSupported()                        */
/************************************************************************/

int msWCSIsLayerSupported(layerObj *layer)
{
  /* only raster layers, are elligible to be served via WCS, WMS rasters are not ok */
  if((layer->type == MS_LAYER_RASTER) && layer->connectiontype != MS_WMS && layer->name != NULL) return MS_TRUE;

  return MS_FALSE;
}

/************************************************************************/
/*                      msWCSGetRequestParameter()                      */
/*                                                                      */
/************************************************************************/

const char *msWCSGetRequestParameter(cgiRequestObj *request, const char *name)
{
  int i;

  if(!request || !name) /* nothing to do */
    return NULL;

  if(request->NumParams > 0) {
    for(i=0; i<request->NumParams; i++) {
      if(strcasecmp(request->ParamNames[i], name) == 0)
        return request->ParamValues[i];
    }
  }

  return NULL;
}

/************************************************************************/
/*                  msWCSSetDefaultBandsRangeSetInfo()                  */
/************************************************************************/

void msWCSSetDefaultBandsRangeSetInfo( wcsParamsObj *params,
                                       coverageMetadataObj *cm,
                                       layerObj *lp )
{
  (void)params;

  /* This function will provide default rangeset information for the "special" */
  /* "bands" rangeset if it appears in the axes list but has no specifics provided */
  /* in the metadata.   */

  const char *value;
  char *bandlist;
  size_t bufferSize = 0;
  int  i;

  /* Does this item exist in the axes list?  */

  value = msOWSLookupMetadata(&(lp->metadata), "CO", "rangeset_axes");
  if( value == NULL )
    return;

  value = strstr(value,"bands");
  if( value == NULL || (value[5] != '\0' && value[5] != ' ') )
    return;

  /* Are there any w*s_bands_ metadata already? If so, skip out. */
  if( msOWSLookupMetadata(&(lp->metadata), "CO", "bands_description") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_name") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_label") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_values") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_values_semantic") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_values_type") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_rangeitem") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_semantic") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_refsys") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_refsyslabel") != NULL
      || msOWSLookupMetadata(&(lp->metadata), "CO", "bands_interval") != NULL )
    return;

  /* OK, we have decided to fill in the information. */

  msInsertHashTable( &(lp->metadata), "wcs_bands_name", "bands" );
  msInsertHashTable( &(lp->metadata), "wcs_bands_label", "Bands/Channels/Samples" );
  msInsertHashTable( &(lp->metadata), "wcs_bands_rangeitem", "_bands" ); /* ? */

  bufferSize = cm->bandcount*30+30;
  bandlist = (char *) msSmallMalloc(bufferSize);
  strcpy( bandlist, "1" );
  for( i = 1; i < cm->bandcount; i++ )
    snprintf( bandlist+strlen(bandlist), bufferSize-strlen(bandlist), ",%d", i+1 );

  msInsertHashTable( &(lp->metadata), "wcs_bands_values", bandlist );
  free( bandlist );
}

/************************************************************************/
/*                         msWCSParseRequest()                          */
/************************************************************************/

static int msWCSParseRequest(cgiRequestObj *request, wcsParamsObj *params, mapObj *map)
{
  int i, n;
  char **tokens;

  if(!request || !params) /* nothing to do */
    return MS_SUCCESS;

  /* -------------------------------------------------------------------- */
  /*      Check if this appears to be an XML POST WCS request.            */
  /* -------------------------------------------------------------------- */

  msDebug("msWCSParseRequest(): request is %s.\n", (request->type == MS_POST_REQUEST)?"POST":"KVP");

  if( request->type == MS_POST_REQUEST
      && request->postrequest ) {
#if defined(USE_LIBXML2)
    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL, child = NULL;
    char *tmp = NULL;

    /* parse to DOM-Structure and get root element */
    if((doc = xmlParseMemory(request->postrequest, strlen(request->postrequest)))
        == NULL) {
      xmlErrorPtr error = xmlGetLastError();
      msSetError(MS_WCSERR, "XML parsing error: %s",
                 "msWCSParseRequest()", error->message);
      return MS_FAILURE;
    }
    root = xmlDocGetRootElement(doc);

    /* Get service, version and request from root */
    params->request = msStrdup((char *) root->name);
    if ((tmp = (char *) xmlGetProp(root, BAD_CAST "service")) != NULL)
      params->service = tmp;
    if ((tmp = (char *) xmlGetProp(root, BAD_CAST "version")) != NULL)
      params->version = tmp;

    /* search first level children, either CoverageID,  */
    for (child = root->children; child != NULL; child = child->next) {
      if (EQUAL((char *)child->name, "AcceptVersions")) {
        /* will be overridden to 1.1.1 anyway */
      } else if (EQUAL((char *) child->name, "UpdateSequence")) {
        params->updatesequence = (char *)xmlNodeGetContent(child);
      } else if (EQUAL((char *) child->name, "Sections")) {
        xmlNodePtr sectionNode = NULL;
        /* concatenate all sections by ',' */
        for(sectionNode = child->children; sectionNode != NULL; sectionNode = sectionNode->next) {
          char *content;
          if(!EQUAL((char *)sectionNode->name, "Section"))
            continue;
          content = (char *)xmlNodeGetContent(sectionNode);
          if(!params->section) {
            params->section = content;
          } else {
            params->section = msStringConcatenate(params->section, ",");
            params->section = msStringConcatenate(params->section, content);
            xmlFree(content);
          }
        }
      } else if(EQUAL((char *) child->name, "AcceptFormats")) {
        /* TODO: implement */
      } else if(EQUAL((char *) child->name, "Identifier")) {
        char *content = (char *)xmlNodeGetContent(child);
        params->coverages = CSLAddString(params->coverages, content);
        xmlFree(content);
      } else if(EQUAL((char *) child->name, "DomainSubset")) {
        xmlNodePtr tmpNode = NULL;
        for(tmpNode = child->children; tmpNode != NULL; tmpNode = tmpNode->next) {
          if(EQUAL((char *) tmpNode->name, "BoundingBox")) {
            xmlNodePtr cornerNode = NULL;
            params->crs = (char *)xmlGetProp(tmpNode, BAD_CAST "crs");
            if( strncasecmp(params->crs,"urn:ogc:def:crs:",16) == 0
                && strncasecmp(params->crs+strlen(params->crs)-8,"imageCRS",8)==0)
              strcpy( params->crs, "imageCRS" );
            for(cornerNode = tmpNode->children; cornerNode != NULL; cornerNode = cornerNode->next) {
              if(EQUAL((char *) cornerNode->name, "LowerCorner")) {
                char *value = (char *)xmlNodeGetContent(cornerNode);
                tokens = msStringSplit(value, ' ', &n);
                if(tokens==NULL || n < 2) {
                  msSetError(MS_WCSERR, "Wrong number of arguments for LowerCorner",
                             "msWCSParseRequest()");
                  return msWCSException(map, "InvalidParameterValue", "LowerCorner",
                                        params->version );
                }
                params->bbox.minx = atof(tokens[0]);
                params->bbox.miny = atof(tokens[1]);
                msFreeCharArray(tokens, n);
                xmlFree(value);
              }
              if(EQUAL((char *) cornerNode->name, "UpperCorner")) {
                char *value = (char *)xmlNodeGetContent(cornerNode);
                tokens = msStringSplit(value, ' ', &n);
                if(tokens==NULL || n < 2) {
                  msSetError(MS_WCSERR, "Wrong number of arguments for UpperCorner",
                             "msWCSParseRequest()");
                  return msWCSException(map, "InvalidParameterValue", "UpperCorner",
                                        params->version );
                }
                params->bbox.maxx = atof(tokens[0]);
                params->bbox.maxy = atof(tokens[1]);
                msFreeCharArray(tokens, n);
                xmlFree(value);
              }
            }
          }
        }
      } else if(EQUAL((char *) child->name, "RangeSubset")) {
        /* TODO: not implemented in mapserver WCS 1.1? */
      } else if(EQUAL((char *) child->name, "Output")) {
        xmlNodePtr tmpNode = NULL;
        params->format = (char *)xmlGetProp(child, BAD_CAST "format");
        for(tmpNode = child->children; tmpNode != NULL; tmpNode = tmpNode->next) {
          if(EQUAL((char *) tmpNode->name, "GridCRS")) {
            xmlNodePtr crsNode = NULL;
            for(crsNode = tmpNode->children; crsNode != NULL; crsNode = crsNode->next) {
              if(EQUAL((char *) crsNode->name, "GridBaseCRS")) {
                params->response_crs = (char *) xmlNodeGetContent(crsNode);
              } else if (EQUAL((char *) crsNode->name, "GridOrigin")) {
                char *value = (char *)xmlNodeGetContent(crsNode);
                tokens = msStringSplit(value, ' ', &n);
                if(tokens==NULL || n < 2) {
                  msSetError(MS_WCSERR, "Wrong number of arguments for GridOrigin",
                             "msWCSParseRequest()");
                  return msWCSException(map, "InvalidParameterValue", "GridOffsets",
                                        params->version );
                }
                params->originx = atof(tokens[0]);
                params->originy = atof(tokens[1]);
                msFreeCharArray(tokens, n);
                xmlFree(value);
              } else if (EQUAL((char *) crsNode->name, "GridOffsets")) {
                char *value = (char *)xmlNodeGetContent(crsNode);
                tokens = msStringSplit(value, ' ', &n);
                if(tokens==NULL || n < 2) {
                  msSetError(MS_WCSERR, "Wrong number of arguments for GridOffsets",
                             "msWCSParseRequest()");
                  return msWCSException(map, "InvalidParameterValue", "GridOffsets",
                                        params->version );
                }
                /* take absolute values to convert to positive RESX/RESY style
                WCS 1.0 behavior.  *but* this does break some possibilities! */
                params->resx = fabs(atof(tokens[0]));
                params->resy = fabs(atof(tokens[1]));
                msFreeCharArray(tokens, n);
                xmlFree(value);
              }
            }
          }
        }
      }
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return MS_SUCCESS;
#else /* defined(USE_LIBXML2) */
    msSetError(MS_WCSERR, "To enable POST requests, MapServer has to "
               "be compiled with libxml2.", "msWCSParseRequest()");
    return MS_FAILURE;
#endif /* defined(USE_LIBXML2) */
  }

  /* -------------------------------------------------------------------- */
  /*      Extract WCS KVP Parameters.                                     */
  /* -------------------------------------------------------------------- */
  if(request->NumParams > 0) {
    for(i=0; i<request->NumParams; i++) {

      if(strcasecmp(request->ParamNames[i], "VERSION") == 0)
        params->version = msStrdup(request->ParamValues[i]);
      if(strcasecmp(request->ParamNames[i], "UPDATESEQUENCE") == 0)
        params->updatesequence = msStrdup(request->ParamValues[i]);
      else if(strcasecmp(request->ParamNames[i], "REQUEST") == 0)
        params->request = msStrdup(request->ParamValues[i]);
      else if(strcasecmp(request->ParamNames[i], "INTERPOLATION") == 0)
        params->interpolation = msStrdup(request->ParamValues[i]);
      else if(strcasecmp(request->ParamNames[i], "SERVICE") == 0)
        params->service = msStrdup(request->ParamValues[i]);
      else if(strcasecmp(request->ParamNames[i], "SECTION") == 0) /* 1.0 */
        params->section = msStrdup(request->ParamValues[i]); /* TODO: validate value here */
      else if(strcasecmp(request->ParamNames[i], "SECTIONS") == 0) /* 1.1 */
        params->section = msStrdup(request->ParamValues[i]); /* TODO: validate value here */

      /* GetCoverage parameters. */
      else if(strcasecmp(request->ParamNames[i], "BBOX") == 0) {
        tokens = msStringSplit(request->ParamValues[i], ',', &n);
        if(tokens==NULL || n != 4) {
          msSetError(MS_WCSERR, "Wrong number of arguments for BBOX.", "msWCSParseRequest()");
          return msWCSException(map, "InvalidParameterValue", "bbox",
                                params->version );
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
      else if(strcasecmp(request->ParamNames[i], "TIME") == 0)
        params->time = msStrdup(request->ParamValues[i]);
      else if(strcasecmp(request->ParamNames[i], "FORMAT") == 0)
        params->format = msStrdup(request->ParamValues[i]);
      else if(strcasecmp(request->ParamNames[i], "CRS") == 0)
        params->crs = msStrdup(request->ParamValues[i]);
      else if(strcasecmp(request->ParamNames[i], "RESPONSE_CRS") == 0)
        params->response_crs = msStrdup(request->ParamValues[i]);

      /* WCS 1.1 DescribeCoverage and GetCoverage ... */
      else if(strcasecmp(request->ParamNames[i], "IDENTIFIER") == 0
              || strcasecmp(request->ParamNames[i], "IDENTIFIERS") == 0 ) {
        msDebug("msWCSParseRequest(): Whole String: %s\n", request->ParamValues[i]);
        params->coverages = CSLAddString(params->coverages, request->ParamValues[i]);
      }
      /* WCS 1.1 style BOUNDINGBOX */
      else if(strcasecmp(request->ParamNames[i], "BOUNDINGBOX") == 0) {
        tokens = msStringSplit(request->ParamValues[i], ',', &n);
        if(tokens==NULL || n < 5) {
          msSetError(MS_WCSERR, "Wrong number of arguments for BOUNDINGBOX.", "msWCSParseRequest()");
          return msWCSException(map, "InvalidParameterValue", "boundingbox",
                                params->version );
        }

        /* NOTE: WCS 1.1 boundingbox is center of pixel oriented, not edge
           like in WCS 1.0.  So bbox semantics are wonky till this is fixed
           later in the GetCoverage processing. */
        params->bbox.minx = atof(tokens[0]);
        params->bbox.miny = atof(tokens[1]);
        params->bbox.maxx = atof(tokens[2]);
        params->bbox.maxy = atof(tokens[3]);

        params->crs = msStrdup(tokens[4]);
        msFreeCharArray(tokens, n);
        /* normalize imageCRS urns to simply "imageCRS" */
        if( strncasecmp(params->crs,"urn:ogc:def:crs:",16) == 0
            && strncasecmp(params->crs+strlen(params->crs)-8,"imageCRS",8)==0)
          strcpy( params->crs, "imageCRS" );
      } else if(strcasecmp(request->ParamNames[i], "GridOffsets") == 0) {
        tokens = msStringSplit(request->ParamValues[i], ',', &n);
        if(tokens==NULL || n < 2) {
          msSetError(MS_WCSERR, "Wrong number of arguments for GridOffsets",
                     "msWCSParseRequest()");
          return msWCSException(map, "InvalidParameterValue", "GridOffsets",
                                params->version );
        }
        /* take absolute values to convert to positive RESX/RESY style
           WCS 1.0 behavior.  *but* this does break some possibilities! */
        params->resx = fabs(atof(tokens[0]));
        params->resy = fabs(atof(tokens[1]));
        msFreeCharArray(tokens, n);
      } else if(strcasecmp(request->ParamNames[i], "GridOrigin") == 0) {
        tokens = msStringSplit(request->ParamValues[i], ',', &n);
        if(tokens==NULL || n < 2) {
          msSetError(MS_WCSERR, "Wrong number of arguments for GridOrigin",
                     "msWCSParseRequest()");
          return msWCSException(map, "InvalidParameterValue", "GridOffsets",
                                params->version );
        }
        params->originx = atof(tokens[0]);
        params->originy = atof(tokens[1]);
        msFreeCharArray(tokens, n);
      }

      /* and so on... */
    }
  }
  /* we are not dealing with an XML encoded request at this point */
  return MS_SUCCESS;
}

/************************************************************************/
/*           msWCSGetCapabilities_Service_ResponsibleParty()            */
/************************************************************************/

static void msWCSGetCapabilities_Service_ResponsibleParty(mapObj *map)
{
  int bEnableTelephone=MS_FALSE, bEnableAddress=MS_FALSE, bEnableOnlineResource=MS_FALSE;

  /* the WCS-specific way */
  if(msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_individualname") ||
      msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_organizationname")) {

    msIO_printf("<responsibleParty>\n");
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_individualname", OWS_NOERR, "    <individualName>%s</individualName>\n", NULL);
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_organizationname", OWS_NOERR, "    <organisationName>%s</organisationName>\n", NULL);
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_positionname", OWS_NOERR, "    <positionName>%s</positionName>\n", NULL);

    if(msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_phone_voice") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_phone_facsimile")) bEnableTelephone = MS_TRUE;

    if(msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_deliverypoint") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_city") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_administrativearea") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_postalcode") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_country") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_address_electronicmailaddress")) bEnableAddress = MS_TRUE;

    if(msOWSLookupMetadata(&(map->web.metadata), "CO", "responsibleparty_onlineresource")) bEnableOnlineResource = MS_TRUE;

    if(bEnableTelephone || bEnableAddress || bEnableOnlineResource) {
      msIO_printf("  <contactInfo>\n");
      if(bEnableTelephone) {
        msIO_printf("    <phone>\n");
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_phone_voice", OWS_NOERR, "    <voice>%s</voice>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_phone_facsimile", OWS_NOERR, "    <facsimile>%s</facsimile>\n", NULL);
        msIO_printf("    </phone>\n");
      }
      if(bEnableAddress) {
        msIO_printf("    <address>\n");
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_deliverypoint", OWS_NOERR, "    <deliveryPoint>%s</deliveryPoint>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_city", OWS_NOERR, "    <city>%s</city>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_administrativearea", OWS_NOERR, "    <administrativeArea>%s</administrativeArea>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_postalcode", OWS_NOERR, "    <postalCode>%s</postalCode>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_country", OWS_NOERR, "    <country>%s</country>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_address_electronicmailaddress", OWS_NOERR, "    <electronicMailAddress>%s</electronicMailAddress>\n", NULL);
        msIO_printf("    </address>\n");
      }
      msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "responsibleparty_onlineresource", OWS_NOERR, "    <onlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n", NULL);
      msIO_printf("  </contactInfo>\n");
    }

    msIO_printf("</responsibleParty>\n");

  } else if(msOWSLookupMetadata(&(map->web.metadata), "CO", "contactperson") ||
            msOWSLookupMetadata(&(map->web.metadata), "CO", "contactorganization")) { /* leverage WMS contact information */

    msIO_printf("<responsibleParty>\n");
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "contactperson", OWS_NOERR, "    <individualName>%s</individualName>\n", NULL);
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "contactorganization", OWS_NOERR, "    <organisationName>%s</organisationName>\n", NULL);
    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "contactposition", OWS_NOERR, "    <positionName>%s</positionName>\n", NULL);

    if(msOWSLookupMetadata(&(map->web.metadata), "CO", "contactvoicetelephone") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "contactfacsimiletelephone")) bEnableTelephone = MS_TRUE;

    if(msOWSLookupMetadata(&(map->web.metadata), "CO", "address") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "city") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "stateorprovince") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "postcode") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "country") ||
        msOWSLookupMetadata(&(map->web.metadata), "CO", "contactelectronicmailaddress")) bEnableAddress = MS_TRUE;

    if(msOWSLookupMetadata(&(map->web.metadata), "CO", "service_onlineresource")) bEnableOnlineResource = MS_TRUE;

    if(bEnableTelephone || bEnableAddress || bEnableOnlineResource) {
      msIO_printf("  <contactInfo>\n");
      if(bEnableTelephone) {
        msIO_printf("    <phone>\n");
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "contactvoicetelephone", OWS_NOERR, "    <voice>%s</voice>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "contactfacsimiletelephone", OWS_NOERR, "    <facsimile>%s</facsimile>\n", NULL);
        msIO_printf("    </phone>\n");
      }
      if(bEnableAddress) {
        msIO_printf("    <address>\n");
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "address", OWS_NOERR, "    <deliveryPoint>%s</deliveryPoint>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "city", OWS_NOERR, "    <city>%s</city>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "stateorprovince", OWS_NOERR, "    <administrativeArea>%s</administrativeArea>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "postcode", OWS_NOERR, "    <postalCode>%s</postalCode>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "country", OWS_NOERR, "    <country>%s</country>\n", NULL);
        msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "contactelectronicmailaddress", OWS_NOERR, "    <electronicMailAddress>%s</electronicMailAddress>\n", NULL);
        msIO_printf("    </address>\n");
      }
      msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "service_onlineresource", OWS_NOERR, "    <onlineResource xlink:type=\"simple\" xlink:href=\"%s\"/>\n", NULL);
      msIO_printf("  </contactInfo>\n");
    }
    msIO_printf("</responsibleParty>\n");
  }

  return;
}

/************************************************************************/
/*                    msWCSGetCapabilities_Service()                    */
/************************************************************************/

static int msWCSGetCapabilities_Service(mapObj *map, wcsParamsObj *params)
{
  /* start the Service section, only need the full start tag if this is the only section requested */
  if(!params->section || (params->section && strcasecmp(params->section, "/") == 0))
    msIO_printf("<Service>\n");
  else
    msIO_printf("<Service\n"
                "   version=\"%s\" \n"
                "   updateSequence=\"%s\" \n"
                "   xmlns=\"http://www.opengis.net/wcs\" \n"
                "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n"
                "   xmlns:gml=\"http://www.opengis.net/gml\" \n"
                "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/wcsCapabilities.xsd\">\n", params->version, params->updatesequence, msOWSGetSchemasLocation(map), params->version);

  /* optional metadataLink */
  msOWSPrintURLType(stdout, &(map->web.metadata), "CO", "metadatalink",
                    OWS_NOERR,
                    "  <metadataLink%s%s%s%s xlink:type=\"simple\"%s/>",
                    NULL, " metadataType=\"%s\"", NULL, NULL, NULL,
                    " xlink:href=\"%s\"", MS_FALSE, MS_FALSE, MS_FALSE,
                    MS_FALSE, MS_TRUE, "other", NULL, NULL, NULL, NULL, NULL);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "description", OWS_NOERR, "  <description>%s</description>\n", NULL);
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "name", OWS_NOERR, "  <name>%s</name>\n", "MapServer WCS");
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "label", OWS_WARN, "  <label>%s</label>\n", NULL);

  /* we are not supporting the optional keyword type, at least not yet */
  msOWSPrintEncodeMetadataList(stdout, &(map->web.metadata), "CO", "keywordlist", "  <keywords>\n", "  </keywords>\n", "    <keyword>%s</keyword>\n", NULL);

  msWCSGetCapabilities_Service_ResponsibleParty(map);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "CO", "fees", OWS_NOERR, "  <fees>%s</fees>\n", "NONE");
  msOWSPrintEncodeMetadataList(stdout, &(map->web.metadata), "CO", "accessconstraints", "  <accessConstraints>\n", "  </accessConstraints>\n", "    %s\n", "NONE");

  /* done */
  msIO_printf("</Service>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*                  msWCSGetCapabilities_Capability()                   */
/************************************************************************/

static int msWCSGetCapabilities_Capability(mapObj *map, wcsParamsObj *params, cgiRequestObj *req)
{
  char *script_url=NULL, *script_url_encoded=NULL;

  /* we need this server's onlineresource for the request section */
  if((script_url=msOWSGetOnlineResource(map, "CO", "onlineresource", req)) == NULL || (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL) {
    free(script_url);
    free(script_url_encoded);
    return msWCSException(map, NULL, NULL, params->version );
  }

  /* start the Capabilty section, only need the full start tag if this is the only section requested */
  if(!params->section || (params->section && strcasecmp(params->section, "/") == 0))
    msIO_printf("<Capability>\n");
  else
    msIO_printf("<Capability\n"
                "   version=\"%s\" \n"
                "   updateSequence=\"%s\" \n"
                "   xmlns=\"http://www.opengis.net/wcs\" \n"
                "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n"
                "   xmlns:gml=\"http://www.opengis.net/gml\" \n"
                "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/wcsCapabilities.xsd\">\n", params->version, params->updatesequence, msOWSGetSchemasLocation(map), params->version);

  /* describe the types of requests the server can handle */
  msIO_printf("  <Request>\n");

  msWCSPrintRequestCapability("GetCapabilities", script_url_encoded);
  if (msOWSRequestIsEnabled(map, NULL, "C", "DescribeCoverage", MS_FALSE))
    msWCSPrintRequestCapability("DescribeCoverage", script_url_encoded);
  if (msOWSRequestIsEnabled(map, NULL, "C", "GetCoverage", MS_FALSE))
    msWCSPrintRequestCapability("GetCoverage", script_url_encoded);

  msIO_printf("  </Request>\n");

  /* describe the exception formats the server can produce */
  msIO_printf("  <Exception>\n");
  msIO_printf("    <Format>application/vnd.ogc.se_xml</Format>\n");
  msIO_printf("  </Exception>\n");

  /* describe any vendor specific capabilities */
  /* msIO_printf("  <VendorSpecificCapabilities />\n"); */ /* none yet */

  /* done */
  msIO_printf("</Capability>\n");

  free(script_url);
  free(script_url_encoded);

  return MS_SUCCESS;
}

/************************************************************************/
/*                    msWCSPrintMetadataLink()                          */
/************************************************************************/

static void  msWCSPrintMetadataLink(layerObj *layer, const char *script_url_encoded)
{
  const char* list = msOWSLookupMetadata(&(layer->metadata), "CO", "metadatalink_list");
  if( list ) {
    int ntokens = 0;
    char** tokens = msStringSplit(list, ' ', &ntokens);
    for( int i = 0; i < ntokens; i++ )
    {
      std::string key("metadatalink_");
      key += tokens[i];
      msOWSPrintURLType(stdout, &(layer->metadata), "CO", key.c_str(),
                        OWS_NOERR,
                        "  <metadataLink%s%s%s%s xlink:type=\"simple\"%s/>",
                        NULL, " metadataType=\"%s\"", NULL, NULL, NULL,
                        " xlink:href=\"%s\"", MS_FALSE, MS_FALSE, MS_FALSE,
                        MS_FALSE, MS_TRUE, "other", NULL, NULL, NULL, NULL, NULL);
    }
    msFreeCharArray(tokens, ntokens);
    return;
  }

  /* optional metadataLink */
  if (! msOWSLookupMetadata(&(layer->metadata), "CO", "metadatalink_href"))
    msMetadataSetGetMetadataURL(layer, script_url_encoded);

  msOWSPrintURLType(stdout, &(layer->metadata), "CO", "metadatalink",
                    OWS_NOERR,
                    "  <metadataLink%s%s%s%s xlink:type=\"simple\"%s/>",
                    NULL, " metadataType=\"%s\"", NULL, NULL, NULL,
                    " xlink:href=\"%s\"", MS_FALSE, MS_FALSE, MS_FALSE,
                    MS_FALSE, MS_TRUE, "other", NULL, NULL, NULL, NULL, NULL);

}

/************************************************************************/
/*             msWCSGetCapabilities_CoverageOfferingBrief()             */
/************************************************************************/

static int msWCSGetCapabilities_CoverageOfferingBrief(layerObj *layer, const char *script_url_encoded)
{
  coverageMetadataObj cm;
  int status;

  if((layer->status == MS_DELETE) || !msWCSIsLayerSupported(layer)) return MS_SUCCESS; /* not an error, this layer cannot be served via WCS */

  status = msWCSGetCoverageMetadata(layer, &cm);
  if(status != MS_SUCCESS) return MS_FAILURE;

  /* start the CoverageOfferingBrief section */
  msIO_printf("  <CoverageOfferingBrief>\n"); /* is this tag right? (I hate schemas without ANY examples) */

  msWCSPrintMetadataLink(layer, script_url_encoded);

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "description", OWS_NOERR, "    <description>%s</description>\n", NULL);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "name", OWS_NOERR, "    <name>%s</name>\n", layer->name);

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "label", OWS_WARN, "    <label>%s</label>\n", NULL);

  /* TODO: add elevation ranges to lonLatEnvelope (optional) */
  msIO_printf("    <lonLatEnvelope srsName=\"urn:ogc:def:crs:OGC:1.3:CRS84\">\n");
  msIO_printf("      <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.minx, cm.llextent.miny); /* TODO: don't know if this is right */
  msIO_printf("      <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.maxx, cm.llextent.maxy);

  msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "CO", "timeposition", NULL, NULL, "      <gml:timePosition>%s</gml:timePosition>\n", NULL);

  msIO_printf("    </lonLatEnvelope>\n");

  /* we are not supporting the optional keyword type, at least not yet */
  msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "CO", "keywordlist", "  <keywords>\n", "  </keywords>\n", "    <keyword>%s</keyword>\n", NULL);

  /* done */
  msIO_printf("  </CoverageOfferingBrief>\n");
  
  msWCSFreeCoverageMetadata(&cm);

  return MS_SUCCESS;
}

/************************************************************************/
/*                msWCSGetCapabilities_ContentMetadata()                */
/************************************************************************/

static int msWCSGetCapabilities_ContentMetadata(mapObj *map, wcsParamsObj *params, owsRequestObj *ows_request, cgiRequestObj *req)
{
  int i;
  char *script_url_encoded=NULL;

  {
      char* pszTmp = msOWSGetOnlineResource(map, "CO", "onlineresource", req);
      script_url_encoded = msEncodeHTMLEntities(pszTmp);
      msFree(pszTmp);
  }

  /* start the ContentMetadata section, only need the full start tag if this is the only section requested */
  /* TODO: add Xlink attributes for other sources of this information  */
  if(!params->section || (params->section && strcasecmp(params->section, "/") == 0))
    msIO_printf("<ContentMetadata>\n");
  else
    msIO_printf("<ContentMetadata\n"
                "   version=\"%s\" \n"
                "   updateSequence=\"%s\" \n"
                "   xmlns=\"http://www.opengis.net/wcs\" \n"
                "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n"
                "   xmlns:gml=\"http://www.opengis.net/gml\" \n"
                "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/wcsCapabilities.xsd\">\n", params->version, params->updatesequence, msOWSGetSchemasLocation(map), params->version);

  if(ows_request->numlayers == 0) {
    msIO_printf("  <!-- WARNING: No WCS layers are enabled. Check wcs/ows_enable_request settings. -->\n");
  } else {
    for(i=0; i<map->numlayers; i++) {
      if (!msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers))
        continue;

      if(msWCSGetCapabilities_CoverageOfferingBrief((GET_LAYER(map, i)), script_url_encoded) != MS_SUCCESS ) {
        msIO_printf("  <!-- WARNING: There was a problem with one of layers. See server log for details. -->\n");
      }
    }
  }

  msFree(script_url_encoded);

  /* done */
  msIO_printf("</ContentMetadata>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*                        msWCSGetCapabilities()                        */
/************************************************************************/

static int msWCSGetCapabilities(mapObj *map, wcsParamsObj *params, cgiRequestObj *req, owsRequestObj *ows_request)
{
  char tmpString[OWS_VERSION_MAXLEN];
  int i, tmpInt = 0;
  int wcsSupportedVersions[] = {OWS_1_1_2, OWS_1_1_1, OWS_1_1_0, OWS_1_0_0};
  int wcsNumSupportedVersions = 4;
  const char *updatesequence=NULL;

  /* check version is valid */
  tmpInt = msOWSParseVersionString(params->version);
  if (tmpInt == OWS_VERSION_BADFORMAT) {
    return msWCSException(map, "InvalidParameterValue",
                          "version", "1.0.0 ");
  }

  /* negotiate version */
  tmpInt = msOWSNegotiateVersion(tmpInt, wcsSupportedVersions, wcsNumSupportedVersions);

  /* set result as string and carry on */
  free(params->version);
  params->version = msStrdup(msOWSGetVersionString(tmpInt, tmpString));

  /* -------------------------------------------------------------------- */
  /*      1.1.x is sufficiently different we have a whole case for        */
  /*      it.  The remainder of this function is for 1.0.0.               */
  /* -------------------------------------------------------------------- */
  if( strncmp(params->version,"1.1",3) == 0 )
    return msWCSGetCapabilities11( map, params, req, ows_request);

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "CO", "updatesequence");

  if (params->updatesequence != NULL) {
    i = msOWSNegotiateUpdateSequence(params->updatesequence, updatesequence);
    if (i == 0) { /* current */
      msSetError(MS_WCSERR, "UPDATESEQUENCE parameter (%s) is equal to server (%s)", "msWCSGetCapabilities()", params->updatesequence, updatesequence);
      return msWCSException(map, "CurrentUpdateSequence",
                            "updatesequence", params->version );
    }
    if (i > 0) { /* invalid */
      msSetError(MS_WCSERR, "UPDATESEQUENCE parameter (%s) is higher than server (%s)", "msWCSGetCapabilities()", params->updatesequence, updatesequence);
      return msWCSException(map, "InvalidUpdateSequence",
                            "updatesequence", params->version );
    }
  }

  else { /* set default updatesequence */
    if(!updatesequence)
      updatesequence = "0";
    params->updatesequence = msStrdup(updatesequence);
  }

  /* if a bum section param is passed, throw exception */
  if (params->section &&
      strcasecmp(params->section, "/WCS_Capabilities/Service") != 0 &&
      strcasecmp(params->section, "/WCS_Capabilities/Capability") != 0 &&
      strcasecmp(params->section, "/WCS_Capabilities/ContentMetadata") != 0 &&
      strcasecmp(params->section, "/") != 0) {
    msIO_setHeader("Content-Type","application/vnd.ogc.se_xml; charset=UTF-8");
    msIO_sendHeaders();
    msSetError( MS_WCSERR,
                "Invalid SECTION parameter \"%s\"",
                "msWCSGetCapabilities()", params->section);
    return msWCSException(map, "InvalidParameterValue", "section",
                          params->version );
  }

  else {
    msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
    msIO_sendHeaders();

    /* print common capability elements  */
    /* TODO: DocType? */

    if (!updatesequence)
      updatesequence = "0";

    msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\n");

    if(!params->section || (params->section && strcasecmp(params->section, "/")  == 0)) msIO_printf("<WCS_Capabilities\n"
          "   version=\"%s\" \n"
          "   updateSequence=\"%s\" \n"
          "   xmlns=\"http://www.opengis.net/wcs\" \n"
          "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n"
          "   xmlns:gml=\"http://www.opengis.net/gml\" \n"
          "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
          "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/wcsCapabilities.xsd\">\n", params->version, updatesequence, msOWSGetSchemasLocation(map), params->version);

    /* print the various capability sections */
    if(!params->section || strcasecmp(params->section, "/WCS_Capabilities/Service") == 0)
      msWCSGetCapabilities_Service(map, params);

    if(!params->section || strcasecmp(params->section, "/WCS_Capabilities/Capability")  == 0)
      msWCSGetCapabilities_Capability(map, params, req);

    if(!params->section || strcasecmp(params->section, "/WCS_Capabilities/ContentMetadata")  == 0)
      msWCSGetCapabilities_ContentMetadata(map, params, ows_request, req);

    if(params->section && strcasecmp(params->section, "/")  == 0) {
      msWCSGetCapabilities_Service(map, params);
      msWCSGetCapabilities_Capability(map, params, req);
      msWCSGetCapabilities_ContentMetadata(map, params, ows_request, req);
    }

    /* done */
    if(!params->section || (params->section && strcasecmp(params->section, "/")  == 0)) msIO_printf("</WCS_Capabilities>\n");
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*               msWCSDescribeCoverage_AxisDescription()                */
/************************************************************************/

static int msWCSDescribeCoverage_AxisDescription(layerObj *layer, char *name)
{
  const char *value;
  char tag[100]; /* should be plenty of space */

  msIO_printf("        <axisDescription>\n");
  msIO_printf("          <AxisDescription");
  snprintf(tag, sizeof(tag), "%s_semantic",  name); /* optional attributes follow (should escape?) */
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", tag, OWS_NOERR, " semantic=\"%s\"", NULL);
  snprintf(tag, sizeof(tag), "%s_refsys", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", tag, OWS_NOERR, " refSys=\"%s\"", NULL);
  snprintf(tag, sizeof(tag), "%s_refsyslabel", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", tag, OWS_NOERR, " refSysLabel=\"%s\"", NULL);
  msIO_printf(">\n");

  /* TODO: add metadataLink (optional) */

  snprintf(tag, sizeof(tag), "%s_description", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", tag, OWS_NOERR, "            <description>%s</description>\n", NULL);
  /* snprintf(tag, sizeof(tag), "%s_name", name); */
  /* msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", tag, OWS_WARN, "            <name>%s</name>\n", NULL); */
  msIO_printf("            <name>%s</name>\n", name);

  snprintf(tag, sizeof(tag), "%s_label", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", tag, OWS_WARN, "            <label>%s</label>\n", NULL);

  /* Values */
  msIO_printf("            <values");
  snprintf(tag, sizeof(tag), "%s_values_semantic", name); /* optional attributes follow (should escape?) */
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", tag, OWS_NOERR, " semantic=\"%s\"", NULL);
  snprintf(tag, sizeof(tag), "%s_values_type", name);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", tag, OWS_NOERR, " type=\"%s\"", NULL);
  msIO_printf(">\n");

  /* single values, we do not support optional type and semantic attributes */
  snprintf(tag, sizeof(tag), "%s_values", name);
  if(msOWSLookupMetadata(&(layer->metadata), "CO", tag))
    msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "CO", tag, NULL, NULL, "              <singleValue>%s</singleValue>\n", NULL);

  /* intervals, only one per axis for now, we do not support optional type, atomic and semantic attributes */
  snprintf(tag, sizeof(tag), "%s_interval", name);
  if((value = msOWSLookupMetadata(&(layer->metadata), "CO", tag)) != NULL) {
    char **tokens;
    int numtokens;

    tokens = msStringSplit(value, '/', &numtokens);
    if(tokens && numtokens > 0) {
      msIO_printf("            <interval>\n");
      if(numtokens >= 1) msIO_printf("            <min>%s</min>\n", tokens[0]); /* TODO: handle closure */
      if(numtokens >= 2) msIO_printf("            <max>%s</max>\n", tokens[1]);
      if(numtokens >= 3) msIO_printf("            <res>%s</res>\n", tokens[2]);
      msIO_printf("            </interval>\n");
    }
  }

  /* TODO: add default (optional) */

  msIO_printf("            </values>\n");

  msIO_printf("          </AxisDescription>\n");
  msIO_printf("        </axisDescription>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*               msWCSDescribeCoverage_CoverageOffering()               */
/************************************************************************/

static int msWCSDescribeCoverage_CoverageOffering(layerObj *layer, wcsParamsObj *params, char *script_url_encoded)
{
  char **tokens;
  int numtokens;
  const char *value;
  char *epsg_buf, *encoded_format;
  coverageMetadataObj cm;
  int i, status;

  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  if(!msWCSIsLayerSupported(layer)) return MS_SUCCESS; /* not an error, this layer cannot be served via WCS */


  status = msWCSGetCoverageMetadata(layer, &cm);
  if(status != MS_SUCCESS) return MS_FAILURE;

  /* fill in bands rangeset info, if required.  */
  msWCSSetDefaultBandsRangeSetInfo( params, &cm, layer );

  /* start the Coverage section */
  msIO_printf("  <CoverageOffering>\n");

  msWCSPrintMetadataLink(layer, script_url_encoded);

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "description", OWS_NOERR, "  <description>%s</description>\n", NULL);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "name", OWS_NOERR, "  <name>%s</name>\n", layer->name);

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "label", OWS_WARN, "  <label>%s</label>\n", NULL);

  /* TODO: add elevation ranges to lonLatEnvelope (optional) */
  msIO_printf("    <lonLatEnvelope srsName=\"urn:ogc:def:crs:OGC:1.3:CRS84\">\n");
  msIO_printf("      <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.minx, cm.llextent.miny);
  msIO_printf("      <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.maxx, cm.llextent.maxy);

  msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "CO", "timeposition", NULL, NULL, "      <gml:timePosition>%s</gml:timePosition>\n", NULL);

  msIO_printf("    </lonLatEnvelope>\n");

  /* we are not supporting the optional keyword type, at least not yet */
  msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "CO", "keywordlist", "  <keywords>\n", "  </keywords>\n", "    <keyword>%s</keyword>\n", NULL);

  /* DomainSet: starting simple, just a spatial domain (gml:envelope) and optionally a temporal domain */
  msIO_printf("    <domainSet>\n");

  /* SpatialDomain */
  msIO_printf("      <spatialDomain>\n");

  /* envelope in lat/lon */
  msIO_printf("        <gml:Envelope srsName=\"EPSG:4326\">\n");
  msIO_printf("          <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.minx, cm.llextent.miny);
  msIO_printf("          <gml:pos>%.15g %.15g</gml:pos>\n", cm.llextent.maxx, cm.llextent.maxy);
  msIO_printf("        </gml:Envelope>\n");

  /* envelope in the native srs */
  msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "CO", MS_TRUE, &epsg_buf);
  if(!epsg_buf) {
    msOWSGetEPSGProj(&(layer->map->projection), &(layer->map->web.metadata), "CO", MS_TRUE, &epsg_buf);
  }
  if(epsg_buf) {
    msIO_printf("        <gml:Envelope srsName=\"%s\">\n", epsg_buf);
    msFree(epsg_buf);
  } else {
    msIO_printf("        <!-- NativeCRSs ERROR: missing required information, no SRSs defined -->\n");
  }
  msIO_printf("          <gml:pos>%.15g %.15g</gml:pos>\n", cm.extent.minx, cm.extent.miny);
  msIO_printf("          <gml:pos>%.15g %.15g</gml:pos>\n", cm.extent.maxx, cm.extent.maxy);
  msIO_printf("        </gml:Envelope>\n");

  /* gml:rectifiedGrid */
  msIO_printf("        <gml:RectifiedGrid dimension=\"2\">\n");
  msIO_printf("          <gml:limits>\n");
  msIO_printf("            <gml:GridEnvelope>\n");
  msIO_printf("              <gml:low>0 0</gml:low>\n");
  msIO_printf("              <gml:high>%d %d</gml:high>\n", cm.xsize-1, cm.ysize-1);
  msIO_printf("            </gml:GridEnvelope>\n");
  msIO_printf("          </gml:limits>\n");
  msIO_printf("          <gml:axisName>x</gml:axisName>\n");
  msIO_printf("          <gml:axisName>y</gml:axisName>\n");
  msIO_printf("          <gml:origin>\n");
  msIO_printf("            <gml:pos>%.15g %.15g</gml:pos>\n", cm.geotransform[0], cm.geotransform[3]);
  msIO_printf("          </gml:origin>\n");
  msIO_printf("          <gml:offsetVector>%.15g %.15g</gml:offsetVector>\n", cm.geotransform[1], cm.geotransform[2]); /* offset vector in X direction */
  msIO_printf("          <gml:offsetVector>%.15g %.15g</gml:offsetVector>\n", cm.geotransform[4], cm.geotransform[5]); /* offset vector in Y direction */
  msIO_printf("        </gml:RectifiedGrid>\n");

  msIO_printf("      </spatialDomain>\n");
  
  msWCSFreeCoverageMetadata(&cm);

  /* TemporalDomain */

  /* TODO: figure out when a temporal domain is valid, for example only tiled rasters support time as a domain, plus we need a timeitem */
  if(msOWSLookupMetadata(&(layer->metadata), "CO", "timeposition") || msOWSLookupMetadata(&(layer->metadata), "CO", "timeperiod")) {
    msIO_printf("      <temporalDomain>\n");

    /* TimePosition (should support a value AUTO, then we could mine positions from the timeitem) */
    msOWSPrintEncodeMetadataList(stdout, &(layer->metadata), "CO", "timeposition", NULL, NULL, "        <gml:timePosition>%s</gml:timePosition>\n", NULL);

    /* TODO:  add TimePeriod (only one per layer)  */

    msIO_printf("      </temporalDomain>\n");
  }

  msIO_printf("    </domainSet>\n");

  /* rangeSet */
  msIO_printf("    <rangeSet>\n");
  msIO_printf("      <RangeSet>\n"); /* TODO: there are some optional attributes */

  /* TODO: add metadataLink (optional) */

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "rangeset_description", OWS_NOERR, "        <description>%s</description>\n", NULL);
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "rangeset_name", OWS_WARN, "        <name>%s</name>\n", NULL);

  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "rangeset_label", OWS_WARN, "        <label>%s</label>\n", NULL);

  /* compound range sets */
  if((value = msOWSLookupMetadata(&(layer->metadata), "CO", "rangeset_axes")) != NULL) {
    tokens = msStringSplit(value, ',', &numtokens);
    if(tokens && numtokens > 0) {
      for(i=0; i<numtokens; i++)
        msWCSDescribeCoverage_AxisDescription(layer, tokens[i]);
      msFreeCharArray(tokens, numtokens);
    }
  }

  if((value = msOWSLookupMetadata(&(layer->metadata), "CO", "rangeset_nullvalue")) != NULL) {
    msIO_printf("        <nullValues>\n");
    msIO_printf("          <singleValue>%s</singleValue>\n", value);
    msIO_printf("        </nullValues>\n");
  }

  msIO_printf("      </RangeSet>\n");
  msIO_printf("    </rangeSet>\n");

  /* supportedCRSs */
  msIO_printf("    <supportedCRSs>\n");

  /* requestResposeCRSs: check the layer metadata/projection, and then the map metadata/projection if necessary (should never get to the error message) */
  msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "CO", MS_FALSE, &epsg_buf);
  if(!epsg_buf) {
    msOWSGetEPSGProj(&(layer->map->projection), &(layer->map->web.metadata), "CO", MS_FALSE, &epsg_buf);
  }
  if(epsg_buf) {
    tokens = msStringSplit(epsg_buf, ' ', &numtokens);
    if(tokens && numtokens > 0) {
      for(i=0; i<numtokens; i++)
        msIO_printf("      <requestResponseCRSs>%s</requestResponseCRSs>\n", tokens[i]);
      msFreeCharArray(tokens, numtokens);
    }
    msFree(epsg_buf);
  } else {
    msIO_printf("      <!-- requestResponseCRSs ERROR: missing required information, no SRSs defined -->\n");
  }

  /* nativeCRSs (only one in our case) */
  msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "CO", MS_TRUE, &epsg_buf);
  if(!epsg_buf) {
    msOWSGetEPSGProj(&(layer->map->projection), &(layer->map->web.metadata), "CO", MS_TRUE, &epsg_buf);
  }
  if(epsg_buf) {
    msIO_printf("      <nativeCRSs>%s</nativeCRSs>\n", epsg_buf);
    msFree(epsg_buf);
  } else {
    msIO_printf("      <!-- nativeCRSs ERROR: missing required information, no SRSs defined -->\n");
  }

  msIO_printf("    </supportedCRSs>\n");


  /* supportedFormats */
  msIO_printf("    <supportedFormats");
  msOWSPrintEncodeMetadata(stdout, &(layer->metadata), "CO", "nativeformat", OWS_NOERR, " nativeFormat=\"%s\"", NULL);
  msIO_printf(">\n");

  if( (encoded_format = msOWSGetEncodeMetadata( &(layer->metadata), "CO", "formats",
                                       "GTiff" )) != NULL ) {
    tokens = msStringSplit(encoded_format, ' ', &numtokens);
    if(tokens && numtokens > 0) {
      for(i=0; i<numtokens; i++)
        msIO_printf("      <formats>%s</formats>\n", tokens[i]);
      msFreeCharArray(tokens, numtokens);
    }
    msFree(encoded_format);
  }
  msIO_printf("    </supportedFormats>\n");

  msIO_printf("    <supportedInterpolations default=\"nearest neighbor\">\n");
  msIO_printf("      <interpolationMethod>nearest neighbor</interpolationMethod>\n" );
  msIO_printf("      <interpolationMethod>bilinear</interpolationMethod>\n" );
  /*  msIO_printf("      <interpolationMethod>bicubic</interpolationMethod>\n" ); */
  msIO_printf("    </supportedInterpolations>\n");


  /* done */
  msIO_printf("  </CoverageOffering>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*                       msWCSDescribeCoverage()                        */
/************************************************************************/

static int msWCSDescribeCoverage(mapObj *map, wcsParamsObj *params, owsRequestObj *ows_request, cgiRequestObj *req)
{
  int i = 0,j = 0, k = 0;
  const char *updatesequence=NULL;
  char **coverages=NULL;
  int numcoverages=0;

  char *coverageName=NULL;
  char *script_url_encoded=NULL;

  /* -------------------------------------------------------------------- */
  /*      1.1.x is sufficiently different we have a whole case for        */
  /*      it.  The remainder of this function is for 1.0.0.               */
  /* -------------------------------------------------------------------- */
  if( strncmp(params->version,"1.1",3) == 0 )
    return msWCSDescribeCoverage11( map, params, ows_request);

  /* -------------------------------------------------------------------- */
  /*      Process 1.0.0...                                                */
  /* -------------------------------------------------------------------- */

  if(params->coverages) { /* use the list, but validate it first */
    for(j=0; params->coverages[j]; j++) {
      coverages = msStringSplit(params->coverages[j], ',', &numcoverages);
      for(k=0; k<numcoverages; k++) {

        for(i=0; i<map->numlayers; i++) {
          coverageName = msOWSGetEncodeMetadata(&(GET_LAYER(map, i)->metadata), "CO", "name", GET_LAYER(map, i)->name);
          if( coverageName != NULL && EQUAL(coverageName, coverages[k]) &&
              (msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers)) ) {
            msFree(coverageName);
            break;
          }
          msFree(coverageName);
        }

        /* i = msGetLayerIndex(map, coverages[k]); */
        if(i == map->numlayers) { /* coverage not found */
          msSetError( MS_WCSERR, "COVERAGE %s cannot be opened / does not exist. A layer might be disabled for \
this request. Check wcs/ows_enable_request settings.", "msWCSDescribeCoverage()", coverages[k]);
          return msWCSException(map, "CoverageNotDefined", "coverage", params->version );
        }
      } /* next coverage */
      msFreeCharArray(coverages,numcoverages);
    }
  }

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "CO", "updatesequence");
  if (!updatesequence)
    updatesequence = "0";

  /* printf("Content-Type: application/vnd.ogc.se_xml%c%c",10,10); */
  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  /* print common capability elements  */
  msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" ?>\n");

  /* start the DescribeCoverage section */
  msIO_printf("<CoverageDescription\n"
              "   version=\"%s\" \n"
              "   updateSequence=\"%s\" \n"
              "   xmlns=\"http://www.opengis.net/wcs\" \n"
              "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n"
              "   xmlns:gml=\"http://www.opengis.net/gml\" \n"
              "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
              "   xsi:schemaLocation=\"http://www.opengis.net/wcs %s/wcs/%s/describeCoverage.xsd\">\n", params->version, updatesequence, msOWSGetSchemasLocation(map), params->version);

  {
    char* pszTmp = msOWSGetOnlineResource(map, "CO", "onlineresource", req);
    script_url_encoded = msEncodeHTMLEntities(pszTmp);
    msFree(pszTmp);
  }

  if(params->coverages) { /* use the list */
    for( j = 0; params->coverages[j]; j++ ) {
      coverages = msStringSplit(params->coverages[j], ',', &numcoverages);
      for(k=0; k<numcoverages; k++) {
        for(i=0; i<map->numlayers; i++) {
          coverageName = msOWSGetEncodeMetadata(&(GET_LAYER(map, i)->metadata), "CO", "name", GET_LAYER(map, i)->name);
          if( coverageName != NULL && EQUAL(coverageName, coverages[k]) ) {
            msFree(coverageName);
            break;
          }
          msFree(coverageName);
        }
        msWCSDescribeCoverage_CoverageOffering((GET_LAYER(map, i)), params, script_url_encoded);
      }
      msFreeCharArray(coverages,numcoverages);
    }
  } else { /* return all layers */
    for(i=0; i<map->numlayers; i++) {
      if (!msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers))
        continue;

      msWCSDescribeCoverage_CoverageOffering((GET_LAYER(map, i)), params, script_url_encoded);
    }
  }

  msFree(script_url_encoded);

  /* done */
  msIO_printf("</CoverageDescription>\n");

  return MS_SUCCESS;
}

/************************************************************************/
/*                       msWCSGetCoverageBands10()                      */
/************************************************************************/

static int msWCSGetCoverageBands10( mapObj *map, cgiRequestObj *request,
                                    wcsParamsObj *params, layerObj *lp,
                                    char **p_bandlist )

{
  const char *value = NULL;
  int i;

  /* Are there any non-spatio/temporal ranges to do subsetting on (e.g. bands) */
  value = msOWSLookupMetadata(&(lp->metadata), "CO", "rangeset_axes"); /* this will get all the compound range sets */
  if(value) {
    char **tokens;
    int numtokens;
    char tag[100];
    const char *rangeitem;

    tokens = msStringSplit(value, ',', &numtokens);

    for(i=0; i<numtokens; i++) {
      if((value = msWCSGetRequestParameter(request, tokens[i])) == NULL) continue; /* next rangeset parameter */

      /* ok, a parameter has been passed which matches a token in wcs_rangeset_axes */
      if(msWCSValidateRangeSetParam(lp, tokens[i], value) != MS_SUCCESS) {
        int ret;
        msSetError( MS_WCSERR, "Error specifying \"%s\" parameter value(s).", "msWCSGetCoverage()", tokens[i]);
        ret = msWCSException(map, "InvalidParameterValue", tokens[i], params->version );
        msFreeCharArray(tokens, numtokens);
        return ret;
      }

      /* xxxxx_rangeitem tells us how to subset */
      snprintf(tag, sizeof(tag), "%s_rangeitem", tokens[i]);
      if((rangeitem = msOWSLookupMetadata(&(lp->metadata), "CO", tag)) == NULL) {
        msSetError( MS_WCSERR, "Missing required metadata element \"%s\", unable to process %s=%s.", "msWCSGetCoverage()", tag, tokens[i], value);
        msFreeCharArray(tokens, numtokens);
        return msWCSException(map, NULL, NULL, params->version);
      }

      if(strcasecmp(rangeitem, "_bands") == 0) { /* special case, subset bands */
        *p_bandlist = msWCSConvertRangeSetToString(value);

        if(!*p_bandlist) {
          msSetError( MS_WCSERR, "Error specifying \"%s\" parameter value(s).", "msWCSGetCoverage()", tokens[i]);
          msFreeCharArray(tokens, numtokens);
          return msWCSException(map, NULL, NULL, params->version );
        }
      } else if(strcasecmp(rangeitem, "_pixels") == 0) { /* special case, subset pixels */
        msFreeCharArray(tokens, numtokens);
        msSetError( MS_WCSERR, "Arbitrary range sets based on pixel values are not yet supported.", "msWCSGetCoverage()" );
        return msWCSException(map, NULL, NULL, params->version);
      } else {
        msFreeCharArray(tokens, numtokens);
        msSetError( MS_WCSERR, "Arbitrary range sets based on tile (i.e. image) attributes are not yet supported.", "msWCSGetCoverage()" );
        return msWCSException(map, NULL, NULL, params->version );
      }
    }
    /* clean-up */
    msFreeCharArray(tokens, numtokens);
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                   msWCSGetCoverage_ImageCRSSetup()                   */
/*                                                                      */
/*      The request was in imageCRS - update the map projection to      */
/*      map the native projection of the layer, and reset the           */
/*      bounding box to match the projected bounds corresponding to     */
/*      the imageCRS request.                                           */
/************************************************************************/

static int msWCSGetCoverage_ImageCRSSetup(
  mapObj *map, wcsParamsObj *params,
  coverageMetadataObj *cm, layerObj *layer )

{
  /* -------------------------------------------------------------------- */
  /*      Load map with the layer (coverage) coordinate system.  We       */
  /*      really need a set projectionObj from projectionObj function!    */
  /* -------------------------------------------------------------------- */
  char *layer_proj = msGetProjectionString( &(layer->projection) );

  if (msLoadProjectionString(&(map->projection), layer_proj) != 0) {
    msFree(layer_proj);
    return msWCSException( map, NULL, NULL, params->version );
  }

  free( layer_proj );
  layer_proj = NULL;

  /* -------------------------------------------------------------------- */
  /*      Reset bounding box.                                             */
  /* -------------------------------------------------------------------- */
  if( params->bbox.maxx != params->bbox.minx ) {
    rectObj orig_bbox = params->bbox;

    params->bbox.minx =
      cm->geotransform[0]
      + orig_bbox.minx * cm->geotransform[1]
      + orig_bbox.miny * cm->geotransform[2];
    params->bbox.maxy =
      cm->geotransform[3]
      + orig_bbox.minx * cm->geotransform[4]
      + orig_bbox.miny * cm->geotransform[5];
    params->bbox.maxx =
      cm->geotransform[0]
      + (orig_bbox.maxx+1) * cm->geotransform[1]
      + (orig_bbox.maxy+1) * cm->geotransform[2];
    params->bbox.miny =
      cm->geotransform[3]
      + (orig_bbox.maxx+1) * cm->geotransform[4]
      + (orig_bbox.maxy+1) * cm->geotransform[5];

    /* WCS 1.1 boundbox is center of pixel oriented. */
    if( strncasecmp(params->version,"1.1",3) == 0 ) {
      params->bbox.minx += cm->geotransform[1]/2 + cm->geotransform[2]/2;
      params->bbox.maxx -= cm->geotransform[1]/2 + cm->geotransform[2]/2;
      params->bbox.maxy += cm->geotransform[4]/2 + cm->geotransform[5]/2;
      params->bbox.miny -= cm->geotransform[4]/2 + cm->geotransform[5]/2;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Reset resolution.                                               */
  /* -------------------------------------------------------------------- */
  if( params->resx != 0.0 ) {
    params->resx = cm->geotransform[1] * params->resx;
    params->resy = fabs(cm->geotransform[5] * params->resy);
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                    msWCSApplyLayerCreationOptions()                  */
/************************************************************************/

void msWCSApplyLayerCreationOptions(layerObj* lp,
                                    outputFormatObj* format,
                                    const char* bandlist)

{
    const char* pszKey;
    char szKeyBeginning[256];
    size_t nKeyBeginningLength;
    int nBands = 0;
    char** papszBandNumbers = msStringSplit(bandlist, ' ', &nBands);

    snprintf(szKeyBeginning, sizeof(szKeyBeginning),
             "wcs_outputformat_%s_creationoption_", format->name);
    nKeyBeginningLength = strlen(szKeyBeginning);

    pszKey = msFirstKeyFromHashTable( &(lp->metadata) );
    for( ; pszKey != NULL;
           pszKey = msNextKeyFromHashTable( &(lp->metadata), pszKey) )
    {
        if( strncmp(pszKey, szKeyBeginning, nKeyBeginningLength) == 0 )
        {
            const char* pszValue = msLookupHashTable( &(lp->metadata), pszKey);
            const char* pszGDALKey = pszKey + nKeyBeginningLength;
            if( EQUALN(pszGDALKey, "BAND_", strlen("BAND_")) )
            {
                /* Remap BAND specific creation option to the real output
                 * band number, given the band subset of the request */
                int nKeyOriBandNumber = atoi(pszGDALKey + strlen("BAND_"));
                int nTargetBandNumber = -1;
                int i;
                for(i = 0; i < nBands; i++ )
                {
                    if( nKeyOriBandNumber == atoi(papszBandNumbers[i]) )
                    {
                        nTargetBandNumber = i + 1;
                        break;
                    }
                }
                if( nTargetBandNumber > 0 )
                {
                    char szModKey[256];
                    const char* pszAfterBand =
                        strchr(pszGDALKey + strlen("BAND_"), '_');
                    if( pszAfterBand != NULL )
                    {
                        snprintf(szModKey, sizeof(szModKey),
                                 "BAND_%d%s",
                                 nTargetBandNumber,
                                 pszAfterBand);
                        if( lp->debug >= MS_DEBUGLEVEL_VVV ) {
                            msDebug("Setting GDAL %s=%s creation option\n",
                                    szModKey, pszValue);
                        }
                        msSetOutputFormatOption(format, szModKey, pszValue);
                    }
                }
            }
            else
            {
                if( lp->debug >= MS_DEBUGLEVEL_VVV ) {
                    msDebug("Setting GDAL %s=%s creation option\n",
                            pszGDALKey, pszValue);
                }
                msSetOutputFormatOption(format, pszGDALKey, pszValue);
            }
        }
    }

    msFreeCharArray( papszBandNumbers, nBands );
}

/************************************************************************/
/*               msWCSApplyDatasetMetadataAsCreationOptions()           */
/************************************************************************/

void msWCSApplyDatasetMetadataAsCreationOptions(layerObj* lp,
                                                outputFormatObj* format,
                                                const char* bandlist,
                                                void* hDSIn)
{
    /* Requires GDAL 2.3 in practice. */
    /* Automatic forwarding of input dataset metadata if it is GRIB and the */
    /* output is GRIB as well, and wcs_outputformat_GRIB_creationoption* are */
    /* not defined. */
    GDALDatasetH hDS = (GDALDatasetH)hDSIn;
    if( hDS && GDALGetDatasetDriver(hDS) &&
        EQUAL(GDALGetDriverShortName(GDALGetDatasetDriver(hDS)), "GRIB") &&
        EQUAL(format->driver, "GDAL/GRIB") )
    {
        const char* pszKey;
        char szKeyBeginning[256];
        size_t nKeyBeginningLength;
        int bWCSMetadataFound = MS_FALSE;

        snprintf(szKeyBeginning, sizeof(szKeyBeginning),
                "wcs_outputformat_%s_creationoption_", format->name);
        nKeyBeginningLength = strlen(szKeyBeginning);

        for( pszKey = msFirstKeyFromHashTable( &(lp->metadata) );
             pszKey != NULL;
             pszKey = msNextKeyFromHashTable( &(lp->metadata), pszKey) )
        {
            if( strncmp(pszKey, szKeyBeginning, nKeyBeginningLength) == 0 )
            {
                bWCSMetadataFound = MS_TRUE;
                break;
            }
        }
        if( !bWCSMetadataFound )
        {
            int nBands = 0;
            char** papszBandNumbers = msStringSplit(bandlist, ' ', &nBands);
            int i;
            for(i = 0; i < nBands; i++ )
            {
                int nSrcBand = atoi(papszBandNumbers[i]);
                int nDstBand = i + 1;
                GDALRasterBandH hBand = GDALGetRasterBand(hDS, nSrcBand);
                if( hBand )
                {
                    char** papszMD = GDALGetMetadata(hBand, NULL);
                    const char* pszMDI = CSLFetchNameValue(papszMD, "GRIB_IDS");
                    // Make sure it is a GRIB2 band
                    if( pszMDI )
                    {
                        char szKey[256];
                        sprintf(szKey, "BAND_%d_IDS", nDstBand);
                        msSetOutputFormatOption(format, szKey, pszMDI);

                        sprintf(szKey, "BAND_%d_DISCIPLINE", nDstBand);
                        msSetOutputFormatOption(format, szKey, 
                                CSLFetchNameValue(papszMD, "DISCIPLINE"));

                        sprintf(szKey, "BAND_%d_PDS_PDTN", nDstBand);
                        msSetOutputFormatOption(format, szKey, 
                                CSLFetchNameValue(papszMD, "GRIB_PDS_PDTN"));

                        sprintf(szKey, "BAND_%d_PDS_TEMPLATE_NUMBERS",
                                nDstBand);
                        msSetOutputFormatOption(format, szKey, 
                                CSLFetchNameValue(papszMD,
                                                  "GRIB_PDS_TEMPLATE_NUMBERS"));
                    }
                }
            }
            msFreeCharArray( papszBandNumbers, nBands );
        }
    }
}

/************************************************************************/
/*                          msWCSGetCoverage()                          */
/************************************************************************/

static int msWCSGetCoverage(mapObj *map, cgiRequestObj *request,
                            wcsParamsObj *params, owsRequestObj *ows_request)
{
  imageObj   *image;
  layerObj   *lp;
  int         status, i;
  const char *value;
  outputFormatObj *format;
  char *bandlist=NULL;
  size_t bufferSize = 0;
  char numbands[12]; /* should be large enough to hold the number of bands in the bandlist */
  coverageMetadataObj cm;
  rectObj reqextent;
  rectObj covextent;
  rasterBufferObj rb;
  int doDrawRasterLayerDraw = MS_TRUE;
  GDALDatasetH hDS = NULL;

  char *coverageName;

  /* make sure all required parameters are available (at least the easy ones) */
  if(!params->crs) {
    msSetError( MS_WCSERR, "Required parameter CRS was not supplied.", "msWCSGetCoverage()");
    return msWCSException(map, "MissingParameterValue", "crs", params->version);
  }

  if(!params->time && !params->bbox.minx && !params->bbox.miny
      && !params->bbox.maxx && !params->bbox.maxy) {
    msSetError(MS_WCSERR, "One of BBOX or TIME is required", "msWCSGetCoverage()");
    return msWCSException(map, "MissingParameterValue", "bbox/time", params->version);
  }

  if( params->coverages == NULL || params->coverages[0] == NULL ) {
    msSetError( MS_WCSERR,
                "Required parameter COVERAGE was not supplied.",
                "msWCSGetCoverage()");
    return msWCSException(map, "MissingParameterValue", "coverage", params->version);
  }

  /* For WCS 1.1, we need to normalize the axis order of the BBOX and
     resolution values some coordinate systems (eg. EPSG geographic) */
  if( strncasecmp(params->version,"1.0",3) != 0
      && params->crs != NULL
      && strncasecmp(params->crs,"urn:",4) == 0 ) {
    projectionObj proj;

    msInitProjection( &proj );
    msProjectionInheritContextFrom(&proj, &(map->projection));
    if( msLoadProjectionString( &proj, (char *) params->crs ) == 0 ) {
      msAxisNormalizePoints( &proj, 1,
                             &(params->bbox.minx),
                             &(params->bbox.miny) );
      msAxisNormalizePoints( &proj, 1,
                             &(params->bbox.maxx),
                             &(params->bbox.maxy) );
      msAxisNormalizePoints( &proj, 1,
                             &(params->resx),
                             &(params->resy) );
      msAxisNormalizePoints( &proj, 1,
                             &(params->originx),
                             &(params->originy) );
    } else
      msResetErrorList();
    msFreeProjection( &proj );
  }

  /* find the layer we are working with */
  lp = NULL;
  for(i=0; i<map->numlayers; i++) {
    coverageName = msOWSGetEncodeMetadata(&(GET_LAYER(map, i)->metadata), "CO", "name", GET_LAYER(map, i)->name);
    if( coverageName != NULL && EQUAL(coverageName, params->coverages[0]) &&
        (msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers)) ) {
      lp = GET_LAYER(map, i);
      free( coverageName );
      break;
    }
    free( coverageName );
  }

  if(lp == NULL) {
    msSetError( MS_WCSERR, "COVERAGE=%s not found, not in supported layer list. A layer might be disabled for \
this request. Check wcs/ows_enable_request settings.", "msWCSGetCoverage()", params->coverages[0] );
    return msWCSException(map, "InvalidParameterValue", "coverage", params->version);
  }

  /* make sure the layer is on */
  lp->status = MS_ON;
  
  /* If the layer has no projection set, set it to the map's projection (#4079) */
  if(lp->projection.numargs <=0) {
    char *map_original_srs = msGetProjectionString(&(map->projection));
    if (msLoadProjectionString(&(lp->projection), map_original_srs) != 0) {
      msSetError( MS_WCSERR, "Error when setting map projection to a layer with no projection", "msWCSGetCoverage()" );
      free(map_original_srs);
      return msWCSException(map, NULL, NULL, params->version);
    }
    free(map_original_srs);
  }

  /* we need the coverage metadata, since things like numbands may not be available otherwise */
  status = msWCSGetCoverageMetadata(lp, &cm);
  if(status != MS_SUCCESS) return MS_FAILURE;

  /* fill in bands rangeset info, if required.  */
  msWCSSetDefaultBandsRangeSetInfo(params, &cm, lp);

  /* handle the response CRS, that is, set the map object projection */
  if(params->response_crs || params->crs ) {
    int iUnits;
    const char *crs_to_use = params->response_crs;

    if( crs_to_use == NULL )
      crs_to_use = params->crs;

    if (strncasecmp(crs_to_use, "EPSG:", 5) == 0 || strncasecmp(crs_to_use,"urn:ogc:def:crs:",16) == 0 ) {
      if (msLoadProjectionString(&(map->projection), (char *) crs_to_use) != 0)
        return msWCSException( map, NULL, NULL,params->version);
    } else if( strcasecmp(crs_to_use,"imageCRS") == 0 ) {
      /* use layer native CRS, and rework bounding box accordingly */
      if( msWCSGetCoverage_ImageCRSSetup( map, params, &cm, lp ) != MS_SUCCESS ) {
        msWCSFreeCoverageMetadata(&cm);
        return MS_FAILURE;
      }
    } else {  /* should we support WMS style AUTO: projections? (not for now) */
      msSetError(MS_WCSERR, "Unsupported SRS namespace (only EPSG currently supported).", "msWCSGetCoverage()");
      return msWCSException(map, "InvalidParameterValue", "srs", params->version);
    }

    iUnits = GetMapserverUnitUsingProj(&(map->projection));
    if (iUnits != -1)
      map->units = static_cast<MS_UNITS>(iUnits);
  }

  /* did we get a TIME value (support only a single value for now) */
  if(params->time) {
    int tli;
    layerObj *tlp=NULL;

    /* need to handle NOW case */

    /* check format of TIME parameter */
    if(strchr(params->time, ',')) {
      msWCSFreeCoverageMetadata(&cm);
      msSetError( MS_WCSERR, "Temporal lists are not supported, only individual values.", "msWCSGetCoverage()" );
      return msWCSException(map, "InvalidParameterValue", "time", params->version);
    }
    if(strchr(params->time, '/')) {
      msWCSFreeCoverageMetadata(&cm);
      msSetError( MS_WCSERR, "Temporal ranges are not supported, only individual values.", "msWCSGetCoverage()" );
      return msWCSException(map, "InvalidParameterValue", "time", params->version);
    }

    /* TODO: will need to expand this check if a time period is supported */
    value = msOWSLookupMetadata(&(lp->metadata), "CO", "timeposition");
    if(!value) {
      msWCSFreeCoverageMetadata(&cm);
      msSetError( MS_WCSERR, "The coverage does not support temporal subsetting.", "msWCSGetCoverage()" );
      return msWCSException(map, "InvalidParameterValue", "time", params->version );
    }

    /* check if timestamp is covered by the wcs_timeposition definition */
    if (msValidateTimeValue(params->time, value) == MS_FALSE) {
      msWCSFreeCoverageMetadata(&cm);
      msSetError( MS_WCSERR, "The coverage does not have a time position of %s.", "msWCSGetCoverage()", params->time );
      return msWCSException(map, "InvalidParameterValue", "time", params->version);
    }

    /* make sure layer is tiled appropriately */
    if(!lp->tileindex) {
      msWCSFreeCoverageMetadata(&cm);
      msSetError( MS_WCSERR, "Underlying layer is not tiled, unable to do temporal subsetting.", "msWCSGetCoverage()" );
      return msWCSException(map, NULL, NULL, params->version);
    }
    tli = msGetLayerIndex(map, lp->tileindex);
    if(tli == -1) {
      msWCSFreeCoverageMetadata(&cm);
      msSetError( MS_WCSERR, "Underlying layer does not use appropriate tiling mechanism.", "msWCSGetCoverage()" );
      return msWCSException(map, NULL, NULL, params->version);
    }

    tlp = (GET_LAYER(map, tli));

    /* make sure there is enough information to filter */
    value = msOWSLookupMetadata(&(lp->metadata), "CO", "timeitem");
    if(!tlp->filteritem && !value) {
      msWCSFreeCoverageMetadata(&cm);
      msSetError( MS_WCSERR, "Not enough information available to filter.", "msWCSGetCoverage()" );
      return msWCSException(map, NULL, NULL, params->version);
    }

    /* override filteritem if specified in metadata */
    if(value) {
      if(tlp->filteritem) free(tlp->filteritem);
      tlp->filteritem = msStrdup(value);
    }

    /* finally set the filter */
    msFreeExpression(&tlp->filter);
    msLayerSetTimeFilter(tlp, params->time, value);
  }

  if( strncasecmp(params->version,"1.0",3) == 0 )
    status = msWCSGetCoverageBands10( map, request, params, lp, &bandlist );
  else
    status = msWCSGetCoverageBands11( map, request, params, lp, &bandlist );
  if( status != MS_SUCCESS ) {
    msWCSFreeCoverageMetadata(&cm);
    return status;
  }

  /* did we get BBOX values? if not use the exent stored in the coverageMetadataObj */
  if( fabs((params->bbox.maxx - params->bbox.minx)) < 0.000000000001  || fabs(params->bbox.maxy - params->bbox.miny) < 0.000000000001 ) {
    params->bbox = cm.extent;

    /* WCS 1.1 boundbox is center of pixel oriented. */
    if( strncasecmp(params->version,"1.1",3) == 0 ) {
      params->bbox.minx += cm.geotransform[1]/2 + cm.geotransform[2]/2;
      params->bbox.maxx -= cm.geotransform[1]/2 + cm.geotransform[2]/2;
      params->bbox.maxy += cm.geotransform[4]/2 + cm.geotransform[5]/2;
      params->bbox.miny -= cm.geotransform[4]/2 + cm.geotransform[5]/2;
    }
  }

  /* WCS 1.1+ GridOrigin is effectively resetting the minx/maxy
     BOUNDINGBOX values, so apply that here */
  if( params->originx != 0.0 || params->originy != 0.0 ) {
    assert( strncasecmp(params->version,"1.0",3) != 0 ); /* should always be 1.0 in this logic. */
    params->bbox.minx = params->originx;
    params->bbox.maxy = params->originy;
  }

  /* if necessary, project the BBOX to the map->projection */
  if(params->response_crs && params->crs) {
    projectionObj tmp_proj;

    msInitProjection(&tmp_proj);
    msProjectionInheritContextFrom(&tmp_proj, &(map->projection));
    if (msLoadProjectionString(&tmp_proj, (char *) params->crs) != 0) {
      msWCSFreeCoverageMetadata(&cm);
      return msWCSException( map, NULL, NULL, params->version);
    }
    msProjectRect(&tmp_proj, &map->projection, &(params->bbox));
    msFreeProjection(&tmp_proj);
  }

  /* in WCS 1.1 the default is full resolution */
  if( strncasecmp(params->version,"1.1",3) == 0 && params->resx == 0.0 && params->resy == 0.0 ) {
    params->resx = cm.geotransform[1];
    params->resy = fabs(cm.geotransform[5]);
  }

  /* compute width/height from BBOX and cellsize.  */
  if( (params->resx == 0.0 || params->resy == 0.0) && params->width != 0 && params->height != 0 ) {
    assert( strncasecmp(params->version,"1.0",3) == 0 ); /* should always be 1.0 in this logic. */
    params->resx = (params->bbox.maxx -params->bbox.minx) / params->width;
    params->resy = (params->bbox.maxy -params->bbox.miny) / params->height;
  }

  /* compute cellsize/res from bbox and raster size. */
  if( (params->width == 0 || params->height == 0) && params->resx != 0 && params->resy != 0 ) {

    /* WCS 1.0 boundbox is edge of pixel oriented. */
    if( strncasecmp(params->version,"1.0",3) == 0 ) {
      params->width = (int) ((params->bbox.maxx - params->bbox.minx) / params->resx + 0.5);
      params->height = (int) ((params->bbox.maxy - params->bbox.miny) / params->resy + 0.5);
    } else {
      params->width = (int) ((params->bbox.maxx - params->bbox.minx) / params->resx + 1.000001);
      params->height = (int) ((params->bbox.maxy - params->bbox.miny) / params->resy + 1.000001);

      /* recompute bounding box so we get exactly the origin and
         resolution requested. */
      params->bbox.maxx = params->bbox.minx + (params->width-1) * params->resx;
      params->bbox.miny = params->bbox.maxy - (params->height-1) * params->resy;
    }
  }

  /* are we still underspecified?  */
  if( (params->width == 0 || params->height == 0) && (params->resx == 0.0 || params->resy == 0.0 )) {
    msWCSFreeCoverageMetadata(&cm);
    msSetError( MS_WCSERR, "A non-zero RESX/RESY or WIDTH/HEIGHT is required but neither was provided.", "msWCSGetCoverage()" );
    return msWCSException(map, "MissingParameterValue", "width/height/resx/resy", params->version);
  }

  map->cellsize = params->resx;

  /* Do we need to force special handling?  */
  if( fabs(params->resx/params->resy - 1.0) > 0.001 ) {
    map->gt.need_geotransform = MS_TRUE;
    if( map->debug ) msDebug( "RESX and RESY don't match.  Using geotransform/resample.\n");
  }

  /* Do we have a specified interpolation method */
  if( params->interpolation != NULL ) {
    if( strncasecmp(params->interpolation,"NEAREST",7) == 0 )
      msLayerSetProcessingKey(lp, "RESAMPLE", "NEAREST");
    else if( strcasecmp(params->interpolation,"BILINEAR") == 0 )
      msLayerSetProcessingKey(lp, "RESAMPLE", "BILINEAR");
    else if( strcasecmp(params->interpolation,"AVERAGE") == 0 )
      msLayerSetProcessingKey(lp, "RESAMPLE", "AVERAGE");
    else {
      msWCSFreeCoverageMetadata(&cm);
      msSetError( MS_WCSERR, "INTERPOLATION=%s specifies an unsupported interpolation method.", "msWCSGetCoverage()", params->interpolation );
      return msWCSException(map, "InvalidParameterValue", "interpolation", params->version);
    }
  }

  /* apply region and size to map object.  */
  map->width = params->width;
  map->height = params->height;

  /* Are we exceeding the MAXSIZE limit on result size? */
  if(map->width > map->maxsize || map->height > map->maxsize ) {
    msWCSFreeCoverageMetadata(&cm);
    msSetError(MS_WCSERR, "Raster size out of range, width and height of resulting coverage must be no more than MAXSIZE=%d.", "msWCSGetCoverage()", map->maxsize);

    return msWCSException(map, "InvalidParameterValue",
                          "width/height", params->version);
  }

  /* adjust OWS BBOX to MapServer's pixel model */
  if( strncasecmp(params->version,"1.0",3) == 0 ) {
    params->bbox.minx += params->resx*0.5;
    params->bbox.miny += params->resy*0.5;
    params->bbox.maxx -= params->resx*0.5;
    params->bbox.maxy -= params->resy*0.5;
  }

  map->extent = params->bbox;

  map->cellsize = params->resx; /* pick one, MapServer only supports square cells (what about msAdjustExtent here!) */

  msMapComputeGeotransform(map);

  /* Do we need to fake out stuff for rotated support? */
  if( map->gt.need_geotransform )
    msMapSetFakedExtent( map );

  map->projection.gt = map->gt;

  /* check for overlap */

  /* get extent of bbox passed, and reproject */
  reqextent.minx = map->extent.minx;
  reqextent.miny = map->extent.miny;
  reqextent.maxx = map->extent.maxx;
  reqextent.maxy = map->extent.maxy;

  /* reproject incoming bbox */
  msProjectRect(&map->projection, &lp->projection, &(reqextent));

  /* get extent of layer */
  covextent.minx = cm.extent.minx;
  covextent.miny = cm.extent.miny;
  covextent.maxx = cm.extent.maxx;
  covextent.maxy = cm.extent.maxy;

  if(msRectOverlap(&reqextent, &covextent) == MS_FALSE) {
    msWCSFreeCoverageMetadata(&cm);
    msSetError(MS_WCSERR, "Requested BBOX (%.15g,%.15g,%.15g,%.15g) is outside requested coverage BBOX (%.15g,%.15g,%.15g,%.15g)",
               "msWCSGetCoverage()",
               reqextent.minx, reqextent.miny, reqextent.maxx, reqextent.maxy,
               covextent.minx, covextent.miny, covextent.maxx, covextent.maxy);
    return msWCSException(map, "NoApplicableCode", "bbox", params->version);
  }

  /* check and make sure there is a format, and that it's valid (TODO: make sure in the layer metadata) */
  if(!params->format) {
    msWCSFreeCoverageMetadata(&cm);
    msSetError( MS_WCSERR,  "Missing required FORMAT parameter.", "msWCSGetCoverage()" );
    return msWCSException(map, "MissingParameterValue", "format", params->version);
  }
  msApplyDefaultOutputFormats(map);
  if(msGetOutputFormatIndex(map,params->format) == -1) {
    msWCSFreeCoverageMetadata(&cm);
    msSetError( MS_WCSERR,  "Unrecognized value for the FORMAT parameter.", "msWCSGetCoverage()" );
    return msWCSException(map, "InvalidParameterValue", "format",
                          params->version );
  }

  /* create a temporary outputformat (we likely will need to tweak parts) */
  format = msCloneOutputFormat(msSelectOutputFormat(map,params->format));
  msApplyOutputFormat(&(map->outputformat), format, MS_NOOVERRIDE);

  if(!bandlist) { /* build a bandlist (default is ALL bands) */
    bufferSize = cm.bandcount*30+30;
    bandlist = (char *) msSmallMalloc(bufferSize);
    strcpy(bandlist, "1");
    for(i = 1; i < cm.bandcount; i++)
      snprintf(bandlist+strlen(bandlist), bufferSize-strlen(bandlist), ",%d", i+1);
  }

  /* apply nullvalue to the output format object if we have it */
  if((value = msOWSLookupMetadata(&(lp->metadata), "CO", "rangeset_nullvalue")) != NULL) {
    msSetOutputFormatOption( map->outputformat, "NULLVALUE", value );
  }

  msLayerSetProcessingKey(lp, "BANDS", bandlist);
  snprintf(numbands, sizeof(numbands), "%d", msCountChars(bandlist, ',')+1);
  msSetOutputFormatOption(map->outputformat, "BAND_COUNT", numbands);

  msWCSApplyLayerCreationOptions(lp, map->outputformat, bandlist);

  if( lp->tileindex == NULL && lp->data != NULL &&
      strlen(lp->data) > 0 &&
      lp->connectiontype != MS_KERNELDENSITY )
  {
      if( msDrawRasterLayerLowCheckIfMustDraw(map, lp) )
      {
          char* decrypted_path = NULL;
          char szPath[MS_MAXPATHLEN];
          hDS = (GDALDatasetH)msDrawRasterLayerLowOpenDataset(
                                    map, lp, lp->data, szPath, &decrypted_path);
          msFree(decrypted_path);
          if( hDS )
            msWCSApplyDatasetMetadataAsCreationOptions(lp, map->outputformat, bandlist, hDS);
      }
      else
      {
          doDrawRasterLayerDraw = MS_FALSE;
      }
  }

  free( bandlist );

  if(lp->mask) {
    int maskLayerIdx = msGetLayerIndex(map,lp->mask);
    layerObj *maskLayer;
    outputFormatObj *altFormat;
    if(maskLayerIdx == -1) {
      msWCSFreeCoverageMetadata(&cm);
      msSetError(MS_MISCERR, "Layer (%s) references unknown mask layer (%s)", "msDrawLayer()",
                 lp->name,lp->mask);
      msDrawRasterLayerLowCloseDataset(lp, hDS);
      return msWCSException(map, NULL, NULL, params->version );
    }
    maskLayer = GET_LAYER(map, maskLayerIdx);
    if(!maskLayer->maskimage) {
      int i,retcode;
      int origstatus, origlabelcache;
      char *origImageType = msStrdup(map->imagetype);
      altFormat =  msSelectOutputFormat(map, "png24");
      msInitializeRendererVTable(altFormat);
      /* TODO: check the png24 format hasn't been tampered with, i.e. it's agg */
      maskLayer->maskimage= msImageCreate(map->width, map->height, altFormat,
                                          map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution, NULL);
      if (!maskLayer->maskimage) {
        msWCSFreeCoverageMetadata(&cm);
        msSetError(MS_MISCERR, "Unable to initialize mask image.", "msDrawLayer()");
        msFree(origImageType);
        msDrawRasterLayerLowCloseDataset(lp, hDS);
        return msWCSException(map, NULL, NULL, params->version );
      }

      /*
       * force the masked layer to status on, and turn off the labelcache so that
       * eventual labels are added to the temporary image instead of being added
       * to the labelcache
       */
      origstatus = maskLayer->status;
      origlabelcache = maskLayer->labelcache;
      maskLayer->status = MS_ON;
      maskLayer->labelcache = MS_OFF;

      /* draw the mask layer in the temporary image */
      retcode = msDrawLayer(map, maskLayer, maskLayer->maskimage);
      maskLayer->status = origstatus;
      maskLayer->labelcache = origlabelcache;
      if(retcode != MS_SUCCESS) {
        msWCSFreeCoverageMetadata(&cm);
        /* set the imagetype from the original outputformat back (it was removed by msSelectOutputFormat() */
        msFree(map->imagetype);
        map->imagetype = origImageType;
        msDrawRasterLayerLowCloseDataset(lp, hDS);
        return msWCSException(map, NULL, NULL, params->version );
      }
      /*
       * hack to work around bug #3834: if we have use an alternate renderer, the symbolset may contain
       * symbols that reference it. We want to remove those references before the altFormat is destroyed
       * to avoid a segfault and/or a leak, and so the the main renderer doesn't pick the cache up thinking
       * it's for him.
       */
      for(i=0; i<map->symbolset.numsymbols; i++) {
        if (map->symbolset.symbol[i]!=NULL) {
          symbolObj *s = map->symbolset.symbol[i];
          if(s->renderer == MS_IMAGE_RENDERER(maskLayer->maskimage)) {
            MS_IMAGE_RENDERER(maskLayer->maskimage)->freeSymbol(s);
            s->renderer = NULL;
          }
        }
      }
      /* set the imagetype from the original outputformat back (it was removed by msSelectOutputFormat() */
      msFree(map->imagetype);
      map->imagetype = origImageType;
      
    }
  }
  
  /* create the image object  */
  if(!map->outputformat) {
    msWCSFreeCoverageMetadata(&cm);
    msSetError(MS_WCSERR, "The map outputformat is missing!", "msWCSGetCoverage()");
    msDrawRasterLayerLowCloseDataset(lp, hDS);
    return msWCSException(map, NULL, NULL, params->version );
  } else if( MS_RENDERER_RAWDATA(map->outputformat) || MS_RENDERER_PLUGIN(map->outputformat) ) {
    image = msImageCreate(map->width, map->height, map->outputformat, map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution, NULL);
  } else {
    msWCSFreeCoverageMetadata(&cm);
    msSetError(MS_WCSERR, "Map outputformat not supported for WCS!", "msWCSGetCoverage()");
    msDrawRasterLayerLowCloseDataset(lp, hDS);
    return msWCSException(map, NULL, NULL, params->version );
  }

  if( image == NULL ) {
    msWCSFreeCoverageMetadata(&cm);
    msDrawRasterLayerLowCloseDataset(lp, hDS);
    return msWCSException(map, NULL, NULL, params->version );
  }
  if( MS_RENDERER_RAWDATA(map->outputformat) ) {
    if( doDrawRasterLayerDraw ) {
      status = msDrawRasterLayerLowWithDataset( map, lp, image, NULL, hDS );
    } else {
      status = MS_SUCCESS;
    }
  } else {
    status = MS_IMAGE_RENDERER(image)->getRasterBufferHandle(image,&rb);
    if(MS_UNLIKELY(status == MS_FAILURE)) {
      msWCSFreeCoverageMetadata(&cm);
      msDrawRasterLayerLowCloseDataset(lp, hDS);
      return MS_FAILURE;
    }

    /* Actually produce the "grid". */
    if( doDrawRasterLayerDraw ) {
      status = msDrawRasterLayerLowWithDataset( map, lp, image, &rb, hDS );
    } else {
      status = MS_SUCCESS;
    }
  }
  msDrawRasterLayerLowCloseDataset(lp, hDS);

  if( status != MS_SUCCESS ) {
    msWCSFreeCoverageMetadata(&cm);
    msFreeImage(image);
    return msWCSException(map, NULL, NULL, params->version );
  }


  if( strncmp(params->version, "1.1",3) == 0 ) {
    msWCSReturnCoverage11( params, map, image );
  } else { /* WCS 1.0.0 - just return the binary data with a content type */
    const char *fo_filename;

    /* Do we have a predefined filename? */
    fo_filename = msGetOutputFormatOption( format, "FILENAME", NULL );
    if( fo_filename )
      msIO_setHeader("Content-Disposition","attachment; filename=%s",
                     fo_filename );

    /* Emit back to client. */
    msOutputFormatResolveFromImage( map, image );
    msIO_setHeader("Content-Type","%s",MS_IMAGE_MIME_TYPE(map->outputformat));
    msIO_sendHeaders();
    status = msSaveImage(map, image, NULL);

    if( status != MS_SUCCESS ) {
      /* unfortunately, the image content type will have already been sent
         but that is hard for us to avoid.  The main error that could happen
         here is a misconfigured tmp directory or running out of space. */
      msWCSFreeCoverageMetadata(&cm);
      return msWCSException(map, NULL, NULL, params->version );
    }
  }

  /* Cleanup */
  msFreeImage(image);
  msApplyOutputFormat(&(map->outputformat), NULL, MS_NOOVERRIDE);
  /* msFreeOutputFormat(format); */

  msWCSFreeCoverageMetadata(&cm);
  return status;
}
#endif /* def USE_WCS_SVR */

/************************************************************************/
/*                           msWCSDispatch()                            */
/*                                                                      */
/*      Entry point for WCS requests                                    */
/************************************************************************/

int msWCSDispatch(mapObj *map, cgiRequestObj *request, owsRequestObj *ows_request)
{
#if defined(USE_WCS_SVR)
  wcs20ParamsObj *params20 = NULL;
  int status, retVal, operation;

  /* If SERVICE is not set or not WCS exit gracefully. */
  if (ows_request->service == NULL
      || !EQUAL(ows_request->service, "WCS")) {
    return MS_DONE;
  }

  /* If no REQUEST is set, exit with an error */
  if (ows_request->request == NULL) {
    /* The request has to be set. */
    msSetError(MS_WCSERR, "Missing REQUEST parameter",
               "msWCSDispatch()");
    return msWCSException(map, "MissingParameterValue", "request",
                          ows_request->version );
  }

  if (EQUAL(ows_request->request, "GetCapabilities")) {
    operation = MS_WCS_GET_CAPABILITIES;
  } else if (EQUAL(ows_request->request, "DescribeCoverage")) {
    operation = MS_WCS_DESCRIBE_COVERAGE;
  } else if (EQUAL(ows_request->request, "GetCoverage")) {
    operation = MS_WCS_GET_COVERAGE;
  } else {
    msSetError(MS_WCSERR, "Invalid REQUEST parameter \"%s\"",
               "msWCSDispatch()", ows_request->request);
    return msWCSException(map, "InvalidParameterValue", "request",
                          ows_request->version);
  }

  /* Check the number of enabled layers for the REQUEST */
  msOWSRequestLayersEnabled(map, "C", ows_request->request, ows_request);
  if (ows_request->numlayers == 0) {
    int caps_globally_enabled = MS_FALSE, disabled = MS_FALSE;
    const char *enable_request;
    if(operation == MS_WCS_GET_CAPABILITIES) {
      enable_request = msOWSLookupMetadata(&map->web.metadata, "OC", "enable_request");
      caps_globally_enabled = msOWSParseRequestMetadata(enable_request, "GetCapabilities", &disabled);
    }

    if(caps_globally_enabled == MS_FALSE) {
      msSetError(MS_WCSERR, "WCS request not enabled. Check "
                 "wcs/ows_enable_request settings.",
                 "msWCSDispatch()");
      return msWCSException(map, "InvalidParameterValue", "request",
                            ows_request->version );
    }
  }

  /* Check the VERSION parameter */
  if (ows_request->version == NULL) {
    /* If the VERSION parameter is not set, it is either */
    /* an error (Describe and GetCoverage), or it has to */
    /* be determined (GetCapabilities). To determine the */
    /* version, the request has to be fully parsed to    */
    /* obtain the ACCEPTVERSIONS parameter. If this is   */
    /* present also, set version to "2.0.1".             */

    if (operation == MS_WCS_GET_CAPABILITIES) {
      /* Parse it as if it was a WCS 2.0 request */
      wcs20ParamsObjPtr params_tmp = msWCSCreateParamsObj20();
      status = msWCSParseRequest20(map, request, ows_request, params_tmp);
      if (status == MS_FAILURE) {
        msWCSFreeParamsObj20(params_tmp);
        return msWCSException(map, "InvalidParameterValue",
                              "request", "2.0.1");
      }

      /* VERSION negotiation */
      if (params_tmp->accept_versions != NULL) {
        /* choose highest acceptable */
        int i, highest_version = 0;
        char version_string[OWS_VERSION_MAXLEN];
        for(i = 0; params_tmp->accept_versions[i] != NULL; ++i) {
          int version = msOWSParseVersionString(params_tmp->accept_versions[i]);
          if (version == OWS_VERSION_BADFORMAT) {
            msWCSFreeParamsObj20(params_tmp);
            return msWCSException(map, "InvalidParameterValue",
                                  "version", NULL );
          }
          if(version > highest_version) {
            highest_version = version;
          }
        }
        msOWSGetVersionString(highest_version, version_string);
        params_tmp->version = msStrdup(version_string);
        ows_request->version = msStrdup(version_string);
      } else {
        /* set to highest acceptable */
        params_tmp->version = msStrdup("2.0.1");
        ows_request->version = msStrdup("2.0.1");
      }

      /* check if we can keep the params object */
      if (EQUAL(params_tmp->version, "2.0.1")) {
        params20 = params_tmp;
      } else {
        msWCSFreeParamsObj20(params_tmp);
      }
    } else { /* operation != GetCapabilities */
      /* VERSION is mandatory in other requests */
      msSetError(MS_WCSERR, "VERSION parameter not set.",
                 "msWCSDispatch()");
      return msWCSException(map, "InvalidParameterValue", "version",
                            NULL );
    }
  } else {
    /* Parse the VERSION parameter */
    int requested_version = msOWSParseVersionString(ows_request->version);
    if (requested_version == OWS_VERSION_BADFORMAT) {
      /* Return an error if the VERSION is */
      /* in an unsupported format.         */
      return msWCSException(map, "InvalidParameterValue",
                            "version", NULL );
    }

    if (operation == MS_WCS_GET_CAPABILITIES) {
      /* In case of GetCapabilities, make  */
      char version_string[OWS_VERSION_MAXLEN];
      int version, supported_versions[] =
      {OWS_2_0_1, OWS_2_0_0, OWS_1_1_2, OWS_1_1_1, OWS_1_1_0, OWS_1_0_0};
      version = msOWSNegotiateVersion(requested_version,
                                      supported_versions,
                                      sizeof(supported_versions)/sizeof(int));
      msOWSGetVersionString(version, version_string);
      msFree(ows_request->version);
      ows_request->version = msStrdup(version_string);
    }
  }

  /* VERSION specific request handler */
  if (strcmp(ows_request->version, "1.0.0") == 0
      || strcmp(ows_request->version, "1.1.0") == 0
      || strcmp(ows_request->version, "1.1.1") == 0
      || strcmp(ows_request->version, "1.1.2") == 0) {
    auto paramsTmp = msWCSCreateParams();
    status = msWCSParseRequest(request, paramsTmp, map);
    if (status == MS_FAILURE) {
      msWCSFreeParams(paramsTmp);
      free(paramsTmp);
      return msWCSException(map, "InvalidParameterValue",
                            "request", "2.0");
    }

    retVal = MS_FAILURE;
    if (operation == MS_WCS_GET_CAPABILITIES) {
      retVal = msWCSGetCapabilities(map, paramsTmp, request, ows_request);
    } else if (operation == MS_WCS_DESCRIBE_COVERAGE) {
      retVal = msWCSDescribeCoverage(map, paramsTmp, ows_request, request);
    } else if (operation == MS_WCS_GET_COVERAGE) {
      retVal = msWCSGetCoverage(map, request, paramsTmp, ows_request);
    }
    msWCSFreeParams(paramsTmp);
    free(paramsTmp);
    return retVal;
  } else if (strcmp(ows_request->version, "2.0.0") == 0
             || strcmp(ows_request->version, "2.0.1") == 0) {
#if defined(USE_LIBXML2)
    int i;

    if (params20 == NULL) {
      params20 = msWCSCreateParamsObj20();
      status = msWCSParseRequest20(map, request, ows_request, params20);
      if (status == MS_FAILURE) {
        msWCSFreeParamsObj20(params20);
        return msWCSException(map, "InvalidParameterValue",
                              "request", "2.0.1");
      }
      else if (status == MS_DONE) {
        /* MS_DONE means, that the exception has already been written to the IO 
          buffer. 
        */
        msWCSFreeParamsObj20(params20);
        return MS_FAILURE;
      }
    }

    /* check if all layer names are valid NCNames */
    for(i = 0; i < map->numlayers; ++i) {
      if(!msWCSIsLayerSupported(map->layers[i]))
        continue;

      /* Check if each layers name is a valid NCName. */
      if (msEvalRegex("^[a-zA-z_][a-zA-Z0-9_.-]*$" , map->layers[i]->name) == MS_FALSE) {
        msSetError(MS_WCSERR, "Layer name '%s' is not a valid NCName.",
                   "msWCSDispatch()", map->layers[i]->name);
        msWCSFreeParamsObj20(params20);
        return msWCSException(map, "mapserv", "Internal", "2.0.1");
      }
    }

    /* Call operation specific functions */
    if (operation == MS_WCS_GET_CAPABILITIES) {
      retVal = msWCSGetCapabilities20(map, request, params20, ows_request);
    } else if (operation == MS_WCS_DESCRIBE_COVERAGE) {
      retVal = msWCSDescribeCoverage20(map, params20, ows_request);
    } else if (operation == MS_WCS_GET_COVERAGE) {
      retVal = msWCSGetCoverage20(map, request, params20, ows_request);
    } else {
      msSetError(MS_WCSERR, "Invalid request '%s'.",
                 "msWCSDispatch20()", ows_request->request);
      retVal = msWCSException20(map, "InvalidParameterValue",
                                "request", "2.0.1");
    }
    /* clean up */
    msWCSFreeParamsObj20(params20);
    return retVal;
#else /* def USE_LIBXML2 */
    msSetError(MS_WCSERR, "WCS 2.0 needs mapserver to be compiled with libxml2.",
               "msWCSDispatch()");
    return msWCSException(map, "mapserv", "NoApplicableCode", "2.0.1");
#endif /* def USE_LIBXML2 */
  } else { /* unsupported version */
    msSetError(MS_WCSERR, "WCS Server does not support VERSION %s.",
               "msWCSDispatch()", ows_request->version);
    return msWCSException(map, "InvalidParameterValue",
                          "version", ows_request->version);
  }

#else
  msSetError(MS_WCSERR, "WCS server support is not available.", "msWCSDispatch()");
  return MS_FAILURE;
#endif
}

/************************************************************************/
/*                      msWCSGetCoverageMetadata()                      */
/************************************************************************/

#ifdef USE_WCS_SVR

void msWCSFreeCoverageMetadata(coverageMetadataObj *cm) {
  msFree(cm->srs_epsg);
}

int msWCSGetCoverageMetadata( layerObj *layer, coverageMetadataObj *cm )
{
  char  *srs_urn = NULL;
  int i = 0;
  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  /* -------------------------------------------------------------------- */
  /*      Get the SRS in WCS 1.0 format (eg. EPSG:n)                      */
  /* -------------------------------------------------------------------- */
  msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "CO", MS_TRUE, &(cm->srs_epsg));
  if(!cm->srs_epsg) {
    msOWSGetEPSGProj(&(layer->map->projection), &(layer->map->web.metadata), "CO", MS_TRUE, &(cm->srs_epsg));
    if(!cm->srs_epsg) {
      msSetError(MS_WCSERR, "Unable to determine the SRS for this layer, no projection defined and no metadata available.", "msWCSGetCoverageMetadata()");
      return MS_FAILURE;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Get the SRS in urn format.                                      */
  /* -------------------------------------------------------------------- */
  if((srs_urn = msOWSGetProjURN(&(layer->projection), &(layer->metadata),
                                "CO", MS_TRUE)) == NULL) {
    srs_urn = msOWSGetProjURN(&(layer->map->projection),
                              &(layer->map->web.metadata),
                              "CO", MS_TRUE);
  }

  if( srs_urn != NULL ) {
    if( strlen(srs_urn) > sizeof(cm->srs_urn) - 1 ) {
      msSetError(MS_WCSERR, "SRS URN too long!",
                 "msWCSGetCoverageMetadata()");
      return MS_FAILURE;
    }

    strcpy( cm->srs_urn, srs_urn );
    msFree( srs_urn );
  } else
    cm->srs_urn[0] = '\0';

  /* -------------------------------------------------------------------- */
  /*      If we have "virtual dataset" metadata on the layer, then use    */
  /*      that in preference to inspecting the file(s).                   */
  /*      We require extent and either size or resolution.                */
  /* -------------------------------------------------------------------- */
  if( msOWSLookupMetadata(&(layer->metadata), "CO", "extent") != NULL
      && (msOWSLookupMetadata(&(layer->metadata), "CO", "resolution") != NULL
          || msOWSLookupMetadata(&(layer->metadata), "CO", "size") != NULL) ) {
    const char *value;

    /* get extent */
    cm->extent.minx = 0.0;
    cm->extent.maxx = 0.0;
    cm->extent.miny = 0.0;
    cm->extent.maxy = 0.0;
    if( msOWSGetLayerExtent( layer->map, layer, "CO", &cm->extent ) == MS_FAILURE )
      return MS_FAILURE;

    /* get resolution */
    cm->xresolution = 0.0;
    cm->yresolution = 0.0;
    if( (value = msOWSLookupMetadata(&(layer->metadata), "CO", "resolution")) != NULL ) {
      char **tokens;
      int n;

      tokens = msStringSplit(value, ' ', &n);
      if( tokens == NULL || n != 2 ) {
        msSetError( MS_WCSERR, "Wrong number of arguments for wcs|ows_resolution metadata.", "msWCSGetCoverageMetadata()");
        msFreeCharArray( tokens, n );
        return MS_FAILURE;
      }
      cm->xresolution = atof(tokens[0]);
      cm->yresolution = atof(tokens[1]);
      msFreeCharArray( tokens, n );
    }

    /* get Size (in pixels and lines) */
    cm->xsize = 0;
    cm->ysize = 0;
    if( (value=msOWSLookupMetadata(&(layer->metadata), "CO", "size")) != NULL ) {
      char **tokens;
      int n;

      tokens = msStringSplit(value, ' ', &n);
      if( tokens == NULL || n != 2 ) {
        msSetError( MS_WCSERR, "Wrong number of arguments for wcs|ows_size metadata.", "msWCSGetCoverageDomain()");
        msFreeCharArray( tokens, n );
        return MS_FAILURE;
      }
      cm->xsize = atoi(tokens[0]);
      cm->ysize = atoi(tokens[1]);
      msFreeCharArray( tokens, n );
    }

    /* try to compute raster size */
    if( cm->xsize == 0 && cm->ysize == 0 && cm->xresolution != 0.0 && cm->yresolution != 0.0 && cm->extent.minx != cm->extent.maxx && cm->extent.miny != cm->extent.maxy ) {
      cm->xsize = (int) ((cm->extent.maxx - cm->extent.minx) / cm->xresolution + 0.5);
      cm->ysize = (int) fabs((cm->extent.maxy - cm->extent.miny) / cm->yresolution + 0.5);
    }

    /* try to compute raster resolution */
    if( (cm->xresolution == 0.0 || cm->yresolution == 0.0) && cm->xsize != 0 && cm->ysize != 0 ) {
      cm->xresolution = (cm->extent.maxx - cm->extent.minx) / cm->xsize;
      cm->yresolution = (cm->extent.maxy - cm->extent.miny) / cm->ysize;
    }

    /* do we have information to do anything */
    if( cm->xresolution == 0.0 || cm->yresolution == 0.0 || cm->xsize == 0 || cm->ysize == 0 ) {
      msSetError( MS_WCSERR, "Failed to collect extent and resolution for WCS coverage from metadata for layer '%s'.  Need value wcs|ows_resolution or wcs|ows_size values.", "msWCSGetCoverageMetadata()", layer->name );
      return MS_FAILURE;
    }

    /* compute geotransform */
    cm->geotransform[0] = cm->extent.minx;
    cm->geotransform[1] = cm->xresolution;
    cm->geotransform[2] = 0.0;
    cm->geotransform[3] = cm->extent.maxy;
    cm->geotransform[4] = 0.0;
    cm->geotransform[5] = -fabs(cm->yresolution);

    /* get bands count, or assume 1 if not found */
    cm->bandcount = 1;
    if( (value=msOWSLookupMetadata(&(layer->metadata), "CO", "bandcount")) != NULL) {
      cm->bandcount = atoi(value);
    }

    /* get bands type, or assume float if not found */
    cm->imagemode = MS_IMAGEMODE_FLOAT32;
    if( (value=msOWSLookupMetadata(&(layer->metadata), "CO", "imagemode")) != NULL ) {
      if( EQUAL(value,"INT16") )
        cm->imagemode = MS_IMAGEMODE_INT16;
      else if( EQUAL(value,"FLOAT32") )
        cm->imagemode = MS_IMAGEMODE_FLOAT32;
      else if( EQUAL(value,"BYTE") )
        cm->imagemode = MS_IMAGEMODE_BYTE;
      else {
        msSetError( MS_WCSERR, "Content of wcs|ows_imagemode (%s) not recognised.  Should be one of BYTE, INT16 or FLOAT32.", "msWCSGetCoverageMetadata()", value );
        return MS_FAILURE;
      }
    }
    /* set color interpretation to undefined */
    /* TODO: find better solution */
    for(i = 0; i < 10; ++i) {
      cm->bandinterpretation[i] = GDALGetColorInterpretationName(GCI_Undefined);
    }
  } else if( layer->data == NULL ) { /* no virtual metadata, not ok unless we're talking 1 image, hopefully we can fix that */
    msSetError( MS_WCSERR, "RASTER Layer with no DATA statement and no WCS virtual dataset metadata.  Tileindexed raster layers not supported for WCS without virtual dataset metadata (cm->extent, wcs_res, wcs_size).", "msWCSGetCoverageDomain()" );
    return MS_FAILURE;
  } else { /* work from the file (e.g. DATA) */
    GDALDatasetH hDS;
    GDALRasterBandH hBand;
    char szPath[MS_MAXPATHLEN];
    char *decrypted_path;

    msGDALInitialize();

    msTryBuildPath3(szPath,  layer->map->mappath, layer->map->shapepath, layer->data);
    decrypted_path = msDecryptStringTokens( layer->map, szPath );
    if( !decrypted_path )
      return MS_FAILURE;

    msAcquireLock( TLOCK_GDAL );
    {
        char** connectionoptions = msGetStringListFromHashTable(&(layer->connectionoptions));
        hDS = GDALOpenEx( decrypted_path, GDAL_OF_RASTER, NULL,
                          (const char* const*)connectionoptions, NULL);
        CSLDestroy(connectionoptions);
    }
    if( hDS == NULL ) {
      const char *cpl_error_msg = CPLGetLastErrorMsg();

      /* we wish to avoid reporting decrypted paths */
      if( cpl_error_msg != NULL
          && strstr(cpl_error_msg,decrypted_path) != NULL
          && strcmp(decrypted_path,szPath) != 0 )
        cpl_error_msg = NULL;

      if( cpl_error_msg == NULL )
        cpl_error_msg = "";

      msReleaseLock( TLOCK_GDAL );

      msSetError( MS_IOERR, "%s", "msWCSGetCoverageMetadata()",
                  cpl_error_msg );

      msFree( decrypted_path );
      return MS_FAILURE;
    }
    msFree( decrypted_path );

    msGetGDALGeoTransform( hDS, layer->map, layer, cm->geotransform );

    cm->xsize = GDALGetRasterXSize( hDS );
    cm->ysize = GDALGetRasterYSize( hDS );

    cm->extent.minx = cm->geotransform[0];
    cm->extent.maxx = cm->geotransform[0] + cm->geotransform[1] * cm->xsize + cm->geotransform[2] * cm->ysize;
    cm->extent.miny = cm->geotransform[3] + cm->geotransform[4] * cm->xsize + cm->geotransform[5] * cm->ysize;
    cm->extent.maxy = cm->geotransform[3];

    cm->xresolution = cm->geotransform[1];
    cm->yresolution = cm->geotransform[5];

    /* TODO: need to set resolution */

    cm->bandcount = GDALGetRasterCount( hDS );

    if( cm->bandcount == 0 ) {
      msReleaseLock( TLOCK_GDAL );
      msSetError( MS_WCSERR, "Raster file %s has no raster bands.  This cannot be used in a layer.", "msWCSGetCoverageMetadata()", layer->data );
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

    /* color interpretation */
    for(i = 1; i <= 10 && i <= cm->bandcount; ++i) {
      GDALColorInterp colorInterp;
      hBand = GDALGetRasterBand( hDS, i );
      colorInterp = GDALGetRasterColorInterpretation(hBand);
      cm->bandinterpretation[i-1] = GDALGetColorInterpretationName(colorInterp);
    }

    GDALClose( hDS );
    msReleaseLock( TLOCK_GDAL );
  }

  /* we must have the bounding box in lat/lon [WGS84(DD)/EPSG:4326] */
  cm->llextent = cm->extent;

  /* Already in latlong .. use directly. */
  if( layer->projection.proj != NULL && msProjIsGeographicCRS(&(layer->projection))) {
    /* no change */
  }

  else if (layer->projection.numargs > 0 && !msProjIsGeographicCRS(&(layer->projection))) /* check the layer projection */
    msProjectRect(&(layer->projection), NULL, &(cm->llextent));

  else if (layer->map->projection.numargs > 0 && !msProjIsGeographicCRS(&(layer->map->projection))) /* check the map projection */
    msProjectRect(&(layer->map->projection), NULL, &(cm->llextent));

  else { /* projection was specified in the metadata only (EPSG:... only at the moment)  */
    projectionObj proj;
    char projstring[32];

    msInitProjection(&proj); /* or bad things happen */
    msProjectionInheritContextFrom(&proj, &(layer->map->projection));

    snprintf(projstring, sizeof(projstring), "init=epsg:%.20s", cm->srs_epsg+5);
    if (msLoadProjectionString(&proj, projstring) != 0) return MS_FAILURE;
    msProjectRect(&proj, NULL, &(cm->llextent));
  }

  return MS_SUCCESS;
}
#endif /* def USE_WCS_SVR */


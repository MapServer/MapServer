/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OpenGIS Web Coverage Server (WCS) 1.1.0 Implementation.  This
 *           file holds some WCS 1.1.0 specific functions but other parts
 *           are still implemented in mapwcs.c.
 * Author:   Frank Warmerdam and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 2007, Frank Warmerdam
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

#include <assert.h>
#include "mapserver.h"
#include "maperror.h"
#include "mapthread.h"
#include "mapows.h"
#include "mapwcs.h"
#include "mapgdal.h"

#if defined(USE_WCS_SVR)
#include "mapwcs.h"
#include "gdal.h"
#include "cpl_string.h" /* GDAL string handling */
#endif

#if defined(USE_LIBXML2)
#include "maplibxml2.h"
#endif

#include <string>

#if defined(USE_WCS_SVR) && defined(USE_LIBXML2)
/*
** msWCSException11()
**
** Report current MapServer error in XML exception format.
** Wrapper function around msOWSCommonExceptionReport. Merely
** passes WCS specific info.
**
*/

int msWCSException11(mapObj *map, const char *exceptionCode,
                     const char *locator, const char *version) {
  int size = 0;
  char *errorString = NULL;
  char *schemasLocation = NULL;

  xmlDocPtr psDoc = NULL;
  xmlNodePtr psRootNode = NULL;
  xmlNsPtr psNsOws = NULL;
  xmlChar *buffer = NULL;

  psNsOws =
      xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");

  errorString = msGetErrorString("\n");
  schemasLocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = msOWSCommonExceptionReport(
      psNsOws, OWS_1_1_0, schemasLocation, version,
      msOWSGetLanguage(map, "exception"), exceptionCode, locator, errorString);

  xmlDocSetRootElement(psDoc, psRootNode);

  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/ows/1.1",
           BAD_CAST "ows");

  msIO_setHeader("Content-Type", "text/xml; charset=UTF-8");
  msIO_sendHeaders();

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, "UTF-8", 1);

  msIO_printf("%s", buffer);

  /*free buffer and the document */
  free(errorString);
  free(schemasLocation);
  xmlFree(buffer);
  xmlFreeDoc(psDoc);
  xmlFreeNs(psNsOws);

  /* clear error since we have already reported it */
  msResetErrorList();

  return MS_FAILURE;
}

/************************************************************************/
/*                       msWCSGetFormatsList11()                        */
/*                                                                      */
/*      Fetch back a comma delimited formats list for the past layer    */
/*      if one is supplied, otherwise for all formats supported by      */
/*      the server.  Formats should be identified by mime type.         */
/************************************************************************/

static char *msWCSGetFormatsList11(mapObj *map, layerObj *layer)

{
  char *format_list = msStrdup("");
  char **tokens = NULL, **formats = NULL;
  int i, numtokens = 0, numformats;
  char *value;

  msApplyDefaultOutputFormats(map);

  /* -------------------------------------------------------------------- */
  /*      Parse from layer metadata.                                      */
  /* -------------------------------------------------------------------- */
  if (layer != NULL &&
      (value = msOWSGetEncodeMetadata(&(layer->metadata), "CO", "formats",
                                      "GTiff")) != NULL) {
    tokens = msStringSplit(value, ' ', &numtokens);
    msFree(value);
  }

  /* -------------------------------------------------------------------- */
  /*      Parse from map.web metadata.                                    */
  /* -------------------------------------------------------------------- */
  else if ((value = msOWSGetEncodeMetadata(&(map->web.metadata), "CO",
                                           "formats", NULL)) != NULL) {
    tokens = msStringSplit(value, ' ', &numtokens);
    msFree(value);
  }

  /* -------------------------------------------------------------------- */
  /*      Or generate from all configured raster output formats that      */
  /*      look plausible.                                                 */
  /* -------------------------------------------------------------------- */
  else {
    tokens = (char **)calloc(map->numoutputformats, sizeof(char *));
    for (i = 0; i < map->numoutputformats; i++) {
      switch (map->outputformatlist[i]->renderer) {
        /* seeminly normal raster format */
      case MS_RENDER_WITH_AGG:
      case MS_RENDER_WITH_SKIA:
      case MS_RENDER_WITH_RAWDATA:
        tokens[numtokens++] = msStrdup(map->outputformatlist[i]->name);
        break;

        /* rest of formats aren't really WCS compatible */
      default:
        break;
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Convert outputFormatObj names into mime types and remove        */
  /*      duplicates.                                                     */
  /* -------------------------------------------------------------------- */
  numformats = 0;
  formats = (char **)calloc(numtokens, sizeof(char *));

  for (i = 0; i < numtokens; i++) {
    int format_i, j;
    const char *mimetype;

    for (format_i = 0; format_i < map->numoutputformats; format_i++) {
      if (strcasecmp(map->outputformatlist[format_i]->name, tokens[i]) == 0)
        break;
    }

    if (format_i == map->numoutputformats) {
      msDebug("Failed to find outputformat info on format '%s', ignore.\n",
              tokens[i]);
      continue;
    }

    mimetype = map->outputformatlist[format_i]->mimetype;
    if (mimetype == NULL || strlen(mimetype) == 0) {
      msDebug("No mimetime for format '%s', ignoring.\n", tokens[i]);
      continue;
    }

    for (j = 0; j < numformats; j++) {
      if (strcasecmp(mimetype, formats[j]) == 0)
        break;
    }

    if (j < numformats) {
      msDebug("Format '%s' ignored since mimetype '%s' duplicates another "
              "outputFormatObj.\n",
              tokens[i], mimetype);
      continue;
    }

    formats[numformats++] = msStrdup(mimetype);
  }

  msFreeCharArray(tokens, numtokens);

  /* -------------------------------------------------------------------- */
  /*      Turn mimetype list into comma delimited form for easy use       */
  /*      with xml functions.                                             */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < numformats; i++) {
    if (i > 0) {
      format_list = msStringConcatenate(format_list, (char *)",");
    }
    format_list = msStringConcatenate(format_list, formats[i]);
  }
  msFreeCharArray(formats, numformats);

  return format_list;
}

/************************************************************************/
/*                   msWCS_11_20_PrintMetadataLink()                    */
/************************************************************************/

static void msWCS_11_20_PrintMetadataLink(layerObj *layer,
                                          const std::string &radix,
                                          xmlDocPtr doc,
                                          xmlNodePtr psCSummary) {
  const char *value =
      msOWSLookupMetadata(&(layer->metadata), "CO", (radix + "_href").c_str());
  if (value) {
    xmlNsPtr psOwsNs =
        xmlSearchNs(doc, xmlDocGetRootElement(doc), BAD_CAST "ows");
    xmlNodePtr psMetadata =
        xmlNewChild(psCSummary, psOwsNs, BAD_CAST "Metadata", NULL);
    xmlNsPtr psXlinkNs =
        xmlSearchNs(doc, xmlDocGetRootElement(doc), BAD_CAST "xlink");
    const char *metadatalink_type = msOWSLookupMetadata(
        &(layer->metadata), "CO", (radix + "_type").c_str());
    const char *metadatalink_format = msOWSLookupMetadata(
        &(layer->metadata), "CO", (radix + "_format").c_str());

    xmlNewNsProp(psMetadata, psXlinkNs, BAD_CAST "type", BAD_CAST "simple");
    xmlNewNsProp(psMetadata, psXlinkNs, BAD_CAST "href", BAD_CAST value);
    if (metadatalink_type != NULL) {
      xmlNewProp(psMetadata, BAD_CAST "about", BAD_CAST metadatalink_type);
    }
    if (metadatalink_format != NULL) {
      xmlNewNsProp(psMetadata, psXlinkNs, BAD_CAST "role",
                   BAD_CAST metadatalink_format);
    }
  }
}

/************************************************************************/
/*                   msWCS_11_20_PrintMetadataLinks()                   */
/************************************************************************/

void msWCS_11_20_PrintMetadataLinks(layerObj *layer, xmlDocPtr doc,
                                    xmlNodePtr psCSummary) {
  const char *list =
      msOWSLookupMetadata(&(layer->metadata), "CO", "metadatalink_list");
  if (list) {
    int ntokens = 0;
    char **tokens = msStringSplit(list, ' ', &ntokens);
    for (int i = 0; i < ntokens; i++) {
      std::string key("metadatalink_");
      key += tokens[i];
      msWCS_11_20_PrintMetadataLink(layer, key, doc, psCSummary);
    }
    msFreeCharArray(tokens, ntokens);
    return;
  }

  msWCS_11_20_PrintMetadataLink(layer, "metadatalink", doc, psCSummary);
}

/************************************************************************/
/*                msWCSGetCapabilities11_CoverageSummary()              */
/*                                                                      */
/*      Generate a WCS 1.1 CoverageSummary.                             */
/************************************************************************/

static int msWCSGetCapabilities11_CoverageSummary(mapObj *map, xmlDocPtr doc,
                                                  xmlNodePtr psContents,
                                                  layerObj *layer)

{
  coverageMetadataObj cm;
  int status;
  const char *value;
  char *owned_value;
  char *format_list;
  xmlNodePtr psCSummary;
  xmlNsPtr psOwsNs = xmlSearchNs(doc, psContents, BAD_CAST "ows");
  int i = 0;

  status = msWCSGetCoverageMetadata(layer, &cm);
  if (status != MS_SUCCESS)
    return MS_FAILURE;

  psCSummary = xmlNewChild(psContents, NULL, BAD_CAST "CoverageSummary", NULL);

  /* -------------------------------------------------------------------- */
  /*      Title (from description)                                        */
  /* -------------------------------------------------------------------- */
  value = msOWSLookupMetadata(&(layer->metadata), "CO", "description");
  if (value == NULL)
    value = msOWSLookupMetadata(&(layer->metadata), "CO", "title");
  if (value == NULL)
    value = layer->name;
  xmlNewChild(psCSummary, psOwsNs, BAD_CAST "Title", BAD_CAST value);

  /* -------------------------------------------------------------------- */
  /*      Abstract                                                        */
  /* -------------------------------------------------------------------- */
  value = msOWSLookupMetadata(&(layer->metadata), "CO", "abstract");
  xmlNewChild(psCSummary, psOwsNs, BAD_CAST "Abstract", BAD_CAST value);

  /* -------------------------------------------------------------------- */
  /*      Keywords                                                        */
  /* -------------------------------------------------------------------- */
  value = msOWSLookupMetadata(&(layer->metadata), "CO", "keywordlist");

  if (value) {
    xmlNodePtr psNode;

    psNode = xmlNewChild(psCSummary, psOwsNs, BAD_CAST "Keywords", NULL);

    int n = 0;
    char **tokens = msStringSplit(value, ',', &n);
    if (tokens && n > 0) {
      for (i = 0; i < n; i++) {
        xmlNewChild(psNode, NULL, BAD_CAST "Keyword", BAD_CAST tokens[i]);
      }
    }
    msFreeCharArray(tokens, n);
  }

  /* -------------------------------------------------------------------- */
  /*      Metadata Link                                                   */
  /* -------------------------------------------------------------------- */
  msWCS_11_20_PrintMetadataLinks(layer, doc, psCSummary);

  /* -------------------------------------------------------------------- */
  /*      WGS84 bounding box.                                             */
  /* -------------------------------------------------------------------- */
  xmlAddChild(psCSummary, msOWSCommonWGS84BoundingBox(
                              psOwsNs, 2, cm.llextent.minx, cm.llextent.miny,
                              cm.llextent.maxx, cm.llextent.maxy));

  /* -------------------------------------------------------------------- */
  /*      Supported CRSes.                                                */
  /* -------------------------------------------------------------------- */
  if ((owned_value = msOWSGetProjURN(&(layer->projection), &(layer->metadata),
                                     "CO", MS_FALSE)) != NULL) {
    /* ok */
  } else if ((owned_value = msOWSGetProjURN(&(layer->map->projection),
                                            &(layer->map->web.metadata), "CO",
                                            MS_FALSE)) != NULL) {
    /* ok */
  } else
    msDebug("mapwcs.c: missing required information, no SRSs defined.\n");

  if (owned_value != NULL && strlen(owned_value) > 0)
    msLibXml2GenerateList(psCSummary, NULL, "SupportedCRS", owned_value, ' ');

  msFree(owned_value);

  /* -------------------------------------------------------------------- */
  /*      SupportedFormats                                                */
  /* -------------------------------------------------------------------- */
  format_list = msWCSGetFormatsList11(map, layer);

  if (strlen(format_list) > 0)
    msLibXml2GenerateList(psCSummary, NULL, "SupportedFormat", format_list,
                          ',');

  msFree(format_list);
  msWCSFreeCoverageMetadata(&cm);

  /* -------------------------------------------------------------------- */
  /*      Identifier (layer name)                                         */
  /* -------------------------------------------------------------------- */
  xmlNewChild(psCSummary, NULL, BAD_CAST "Identifier", BAD_CAST layer->name);

  return MS_SUCCESS;
}

/************************************************************************/
/*                       msWCSGetCapabilities11()                       */
/************************************************************************/
int msWCSGetCapabilities11(mapObj *map, wcsParamsObj *params,
                           cgiRequestObj *req, owsRequestObj *ows_request) {
  xmlDocPtr psDoc = NULL; /* document pointer */
  xmlNodePtr psRootNode, psMainNode, psNode;
  char *identifier_list = NULL, *format_list = NULL;
  const char *updatesequence = NULL;
  xmlNsPtr psOwsNs, psXLinkNs;
  char *schemaLocation = NULL;
  char *xsi_schemaLocation = NULL;
  char *script_url = NULL, *script_url_encoded = NULL;

  xmlChar *buffer = NULL;
  int size = 0, i;
  msIOContext *context = NULL;

  int ows_version = OWS_1_1_0;

  /* -------------------------------------------------------------------- */
  /*      Handle updatesequence                                           */
  /* -------------------------------------------------------------------- */

  updatesequence =
      msOWSLookupMetadata(&(map->web.metadata), "CO", "updatesequence");

  if (params->updatesequence != NULL) {
    i = msOWSNegotiateUpdateSequence(params->updatesequence, updatesequence);
    if (i == 0) { /* current */
      msSetError(
          MS_WCSERR, "UPDATESEQUENCE parameter (%s) is equal to server (%s)",
          "msWCSGetCapabilities11()", params->updatesequence, updatesequence);
      return msWCSException11(map, "CurrentUpdateSequence", "updatesequence",
                              params->version);
    }
    if (i > 0) { /* invalid */
      msSetError(
          MS_WCSERR, "UPDATESEQUENCE parameter (%s) is higher than server (%s)",
          "msWCSGetCapabilities11()", params->updatesequence, updatesequence);
      return msWCSException11(map, "InvalidUpdateSequence", "updatesequence",
                              params->version);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Build list of layer identifiers available.                      */
  /* -------------------------------------------------------------------- */
  identifier_list = msStrdup("");
  for (i = 0; i < map->numlayers; i++) {
    layerObj *layer = map->layers[i];
    int new_length;

    if (!msWCSIsLayerSupported(layer))
      continue;

    new_length = strlen(identifier_list) + strlen(layer->name) + 2;
    identifier_list = (char *)realloc(identifier_list, new_length);

    if (strlen(identifier_list) > 0)
      strcat(identifier_list, ",");
    strcat(identifier_list, layer->name);
  }

  /* -------------------------------------------------------------------- */
  /*      Create document.                                                */
  /* -------------------------------------------------------------------- */
  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = xmlNewNode(NULL, BAD_CAST "Capabilities");

  xmlDocSetRootElement(psDoc, psRootNode);

  /* -------------------------------------------------------------------- */
  /*      Name spaces                                                     */
  /* -------------------------------------------------------------------- */
  xmlSetNs(
      psRootNode,
      xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/wcs/1.1", NULL));
  psOwsNs = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_110_NAMESPACE_URI,
                     BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);
  psXLinkNs =
      xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI,
               BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI,
           BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_URI,
           BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_PREFIX);

  /*xmlNewProp(psRootNode, BAD_CAST " */
  xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST params->version);

  updatesequence =
      msOWSLookupMetadata(&(map->web.metadata), "CO", "updatesequence");

  if (updatesequence)
    xmlNewProp(psRootNode, BAD_CAST "updateSequence", BAD_CAST updatesequence);

  schemaLocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
  xsi_schemaLocation = msStrdup("http://www.opengis.net/wcs/1.1");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemaLocation);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation,
                                           "/wcs/1.1/wcsGetCapabilities.xsd ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation,
                                           MS_OWSCOMMON_OWS_110_NAMESPACE_URI);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemaLocation);
  xsi_schemaLocation =
      msStringConcatenate(xsi_schemaLocation, "/ows/1.1.0/owsAll.xsd");
  xmlNewNsProp(psRootNode, NULL, BAD_CAST "xsi:schemaLocation",
               BAD_CAST xsi_schemaLocation);
  msFree(schemaLocation);
  msFree(xsi_schemaLocation);

  /* -------------------------------------------------------------------- */
  /*      Service metadata.                                               */
  /* -------------------------------------------------------------------- */
  if (params->section == NULL || strstr(params->section, "All") != NULL ||
      strstr(params->section, "ServiceIdentification") != NULL) {
    xmlAddChild(psRootNode,
                msOWSCommonServiceIdentification(
                    psOwsNs, map, "OGC WCS", "2.0.1,1.1.1,1.0.0", "CO", NULL));
  }

  /*service provider*/
  if (params->section == NULL || strstr(params->section, "All") != NULL ||
      strstr(params->section, "ServiceProvider") != NULL) {
    xmlAddChild(psRootNode, msOWSCommonServiceProvider(psOwsNs, psXLinkNs, map,
                                                       "CO", NULL));
  }

  /* -------------------------------------------------------------------- */
  /*      Operations metadata.                                            */
  /* -------------------------------------------------------------------- */
  /*operation metadata */
  if ((script_url = msOWSGetOnlineResource(map, "CO", "onlineresource", req)) ==
          NULL ||
      (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL) {
    msSetError(MS_WCSERR, "Server URL not found", "msWCSGetCapabilities11()");
    return msWCSException11(map, "NoApplicableCode", "mapserv",
                            params->version);
  }
  free(script_url);

  if (params->section == NULL || strstr(params->section, "All") != NULL ||
      strstr(params->section, "OperationsMetadata") != NULL) {
    psMainNode =
        xmlAddChild(psRootNode, msOWSCommonOperationsMetadata(psOwsNs));

    /* -------------------------------------------------------------------- */
    /*      GetCapabilities - add Sections and AcceptVersions?              */
    /* -------------------------------------------------------------------- */
    psNode = msOWSCommonOperationsMetadataOperation(
        psOwsNs, psXLinkNs, "GetCapabilities", OWS_METHOD_GETPOST,
        script_url_encoded);

    xmlAddChild(psMainNode, psNode);
    xmlAddChild(psNode,
                msOWSCommonOperationsMetadataDomainType(
                    ows_version, psOwsNs, "Parameter", "service", "WCS"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                            ows_version, psOwsNs, "Parameter", "version",
                            (char *)params->version));

    /* -------------------------------------------------------------------- */
    /*      DescribeCoverage                                                */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "C", "DescribeCoverage", MS_FALSE)) {
      psNode = msOWSCommonOperationsMetadataOperation(
          psOwsNs, psXLinkNs, "DescribeCoverage", OWS_METHOD_GETPOST,
          script_url_encoded);

      xmlAddChild(psMainNode, psNode);
      xmlAddChild(psNode,
                  msOWSCommonOperationsMetadataDomainType(
                      ows_version, psOwsNs, "Parameter", "service", "WCS"));
      xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                              ows_version, psOwsNs, "Parameter", "version",
                              (char *)params->version));
      xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                              ows_version, psOwsNs, "Parameter", "identifiers",
                              identifier_list));
    }

    /* -------------------------------------------------------------------- */
    /*      GetCoverage                                                     */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "C", "GetCoverage", MS_FALSE)) {

      psNode = msOWSCommonOperationsMetadataOperation(
          psOwsNs, psXLinkNs, "GetCoverage", OWS_METHOD_GETPOST,
          script_url_encoded);

      format_list = msWCSGetFormatsList11(map, NULL);

      xmlAddChild(psMainNode, psNode);
      xmlAddChild(psNode,
                  msOWSCommonOperationsMetadataDomainType(
                      ows_version, psOwsNs, "Parameter", "service", "WCS"));
      xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                              ows_version, psOwsNs, "Parameter", "version",
                              (char *)params->version));
      xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                              ows_version, psOwsNs, "Parameter", "Identifier",
                              identifier_list));
      xmlAddChild(psNode,
                  msOWSCommonOperationsMetadataDomainType(
                      ows_version, psOwsNs, "Parameter", "InterpolationType",
                      "NEAREST_NEIGHBOUR,BILINEAR"));
      xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                              ows_version, psOwsNs, "Parameter", "format",
                              format_list));
      xmlAddChild(psNode,
                  msOWSCommonOperationsMetadataDomainType(
                      ows_version, psOwsNs, "Parameter", "store", "false"));
      xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                              ows_version, psOwsNs, "Parameter", "GridBaseCRS",
                              "urn:ogc:def:crs:epsg::4326"));

      msFree(format_list);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Contents section.                                               */
  /* -------------------------------------------------------------------- */
  if (params->section == NULL || strstr(params->section, "All") != NULL ||
      strstr(params->section, "Contents") != NULL) {
    psMainNode = xmlNewChild(psRootNode, NULL, BAD_CAST "Contents", NULL);

    if (ows_request->numlayers == 0) {
      xmlAddChild(psMainNode,
                  xmlNewComment(BAD_CAST
                                "WARNING: No WCS layers are enabled. "
                                "Check wcs/ows_enable_request settings."));
    } else {
      for (i = 0; i < map->numlayers; i++) {
        layerObj *layer = map->layers[i];
        int status;

        if (!msWCSIsLayerSupported(layer))
          continue;

        if (!msIntegerInArray(layer->index, ows_request->enabled_layers,
                              ows_request->numlayers))
          continue;

        status = msWCSGetCapabilities11_CoverageSummary(map, psDoc, psMainNode,
                                                        layer);
        if (status != MS_SUCCESS) {
          msFree(identifier_list);
          return MS_FAILURE;
        }
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Write out the document.                                         */
  /* -------------------------------------------------------------------- */

  if (msIO_needBinaryStdout() == MS_FAILURE)
    return MS_FAILURE;

  msIO_setHeader("Content-Type", "text/xml; charset=UTF-8");
  msIO_sendHeaders();

  context = msIO_getHandler(stdout);

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, "UTF-8", 1);
  msIO_contextWrite(context, buffer, size);
  xmlFree(buffer);

  /*free buffer and the document */
  /*xmlFree(buffer);*/
  xmlFreeDoc(psDoc);

  xmlCleanupParser();

  /* clean up */
  free(script_url_encoded);
  free(identifier_list);

  return (MS_SUCCESS);
}

/************************************************************************/
/*            msWCSDescribeCoverage_CoverageDescription11()             */
/************************************************************************/

static int msWCSDescribeCoverage_CoverageDescription11(layerObj *layer,
                                                       wcsParamsObj *params,
                                                       xmlNodePtr psRootNode,
                                                       xmlNsPtr psOwsNs)

{
  int status;
  coverageMetadataObj cm;
  xmlNodePtr psCD, psDomain, psSD, psGridCRS;
  const char *value;

  /* -------------------------------------------------------------------- */
  /*      Verify layer is processable.                                    */
  /* -------------------------------------------------------------------- */
  if (msCheckParentPointer(layer->map, "map") == MS_FAILURE)
    return MS_FAILURE;

  if (!msWCSIsLayerSupported(layer))
    return MS_SUCCESS;

  /* -------------------------------------------------------------------- */
  /*      Setup coverage metadata.                                        */
  /* -------------------------------------------------------------------- */
  status = msWCSGetCoverageMetadata(layer, &cm);
  if (status != MS_SUCCESS)
    return status;

  /* fill in bands rangeset info, if required.  */
  msWCSSetDefaultBandsRangeSetInfo(params, &cm, layer);

  /* -------------------------------------------------------------------- */
  /*      Create CoverageDescription node.                                */
  /* -------------------------------------------------------------------- */
  psCD = xmlNewChild(psRootNode, NULL, BAD_CAST "CoverageDescription", NULL);

  /* -------------------------------------------------------------------- */
  /*      Title (from description)                                        */
  /* -------------------------------------------------------------------- */
  value = msOWSLookupMetadata(&(layer->metadata), "CO", "description");
  if (value == NULL)
    value = layer->name;
  xmlNewChild(psCD, psOwsNs, BAD_CAST "Title", BAD_CAST value);

  /* -------------------------------------------------------------------- */
  /*      Abstract                                                        */
  /* -------------------------------------------------------------------- */
  value = msOWSLookupMetadata(&(layer->metadata), "CO", "abstract");
  xmlNewChild(psCD, psOwsNs, BAD_CAST "Abstract", BAD_CAST value);

  /* -------------------------------------------------------------------- */
  /*      Keywords                                                        */
  /* -------------------------------------------------------------------- */
  value = msOWSLookupMetadata(&(layer->metadata), "CO", "keywordlist");

  if (value)
    msLibXml2GenerateList(xmlNewChild(psCD, psOwsNs, BAD_CAST "Keywords", NULL),
                          NULL, "Keyword", value, ',');

  /* -------------------------------------------------------------------- */
  /*      Identifier (layer name)                                         */
  /* -------------------------------------------------------------------- */
  xmlNewChild(psCD, NULL, BAD_CAST "Identifier", BAD_CAST layer->name);

  /* -------------------------------------------------------------------- */
  /*      Domain                                                          */
  /* -------------------------------------------------------------------- */
  psDomain = xmlNewChild(psCD, NULL, BAD_CAST "Domain", NULL);

  /* -------------------------------------------------------------------- */
  /*      SpatialDomain                                                   */
  /* -------------------------------------------------------------------- */
  psSD = xmlNewChild(psDomain, NULL, BAD_CAST "SpatialDomain", NULL);

  /* -------------------------------------------------------------------- */
  /*      imageCRS bounding box.                                          */
  /* -------------------------------------------------------------------- */
  xmlAddChild(psSD,
              msOWSCommonBoundingBox(psOwsNs, "urn:ogc:def:crs:OGC::imageCRS",
                                     2, 0, 0, cm.xsize - 1, cm.ysize - 1));

  /* -------------------------------------------------------------------- */
  /*      native CRS bounding box.                                        */
  /* -------------------------------------------------------------------- */
  xmlAddChild(psSD, msOWSCommonBoundingBox(psOwsNs, cm.srs_urn, 2,
                                           cm.extent.minx, cm.extent.miny,
                                           cm.extent.maxx, cm.extent.maxy));

  /* -------------------------------------------------------------------- */
  /*      WGS84 bounding box.                                             */
  /* -------------------------------------------------------------------- */
  xmlAddChild(psSD, msOWSCommonWGS84BoundingBox(
                        psOwsNs, 2, cm.llextent.minx, cm.llextent.miny,
                        cm.llextent.maxx, cm.llextent.maxy));

  /* -------------------------------------------------------------------- */
  /*      GridCRS                                                         */
  /* -------------------------------------------------------------------- */
  {
    char format_buf[500];
    projectionObj proj;
    double x0 =
        cm.geotransform[0] + cm.geotransform[1] / 2 + cm.geotransform[2] / 2;
    double y0 =
        cm.geotransform[3] + cm.geotransform[4] / 2 + cm.geotransform[5] / 2;
    double resx = cm.geotransform[1];
    double resy = cm.geotransform[5];

    msInitProjection(&proj);
    msProjectionInheritContextFrom(&proj, &(layer->projection));
    if (msLoadProjectionString(&proj, cm.srs_urn) == 0) {
      msAxisNormalizePoints(&proj, 1, &x0, &y0);
      msAxisNormalizePoints(&proj, 1, &resx, &resy);
    }
    msFreeProjection(&proj);

    psGridCRS = xmlNewChild(psSD, NULL, BAD_CAST "GridCRS", NULL);

    xmlNewChild(psGridCRS, NULL, BAD_CAST "GridBaseCRS", BAD_CAST cm.srs_urn);
    xmlNewChild(psGridCRS, NULL, BAD_CAST "GridType",
                BAD_CAST "urn:ogc:def:method:WCS:1.1:2dSimpleGrid");

    snprintf(format_buf, sizeof(format_buf), "%.15g %.15g", x0, y0);
    xmlNewChild(psGridCRS, NULL, BAD_CAST "GridOrigin", BAD_CAST format_buf);

    snprintf(format_buf, sizeof(format_buf), "%.15g %.15g", resx, resy);
    xmlNewChild(psGridCRS, NULL, BAD_CAST "GridOffsets", BAD_CAST format_buf);

    xmlNewChild(psGridCRS, NULL, BAD_CAST "GridCS",
                BAD_CAST "urn:ogc:def:cs:OGC:0.0:Grid2dSquareCS");
  }

#ifdef notdef
  /* TemporalDomain */

  /* TODO: figure out when a temporal domain is valid, for example only tiled
   * rasters support time as a domain, plus we need a timeitem */
  if (msOWSLookupMetadata(&(layer->metadata), "CO", "timeposition") ||
      msOWSLookupMetadata(&(layer->metadata), "CO", "timeperiod")) {
    msIO_printf("      <temporalDomain>\n");

    /* TimePosition (should support a value AUTO, then we could mine positions
     * from the timeitem) */
    msOWSPrintEncodeMetadataList(
        stdout, &(layer->metadata), "CO", "timeposition", NULL, NULL,
        "        <gml:timePosition>%s</gml:timePosition>\n", NULL);

    /* TODO:  add TimePeriod (only one per layer)  */

    msIO_printf("      </temporalDomain>\n");
  }

  msIO_printf("    </domainSet>\n");
#endif

  /* -------------------------------------------------------------------- */
  /*      Range                                                           */
  /* -------------------------------------------------------------------- */
  {
    xmlNodePtr psField, psInterpMethods, psAxis;
    char *value;

    psField = xmlNewChild(xmlNewChild(psCD, NULL, BAD_CAST "Range", NULL), NULL,
                          BAD_CAST "Field", NULL);

    value = msOWSGetEncodeMetadata(&(layer->metadata), "CO", "rangeset_label",
                                   NULL);
    if (value)
      xmlNewChild(psField, psOwsNs, BAD_CAST "Title", BAD_CAST value);
    msFree(value);

    /* ows:Abstract? TODO */

    value = msOWSGetEncodeMetadata(&(layer->metadata), "CO", "rangeset_name",
                                   "raster");
    xmlNewChild(psField, NULL, BAD_CAST "Identifier", BAD_CAST value);
    msFree(value);

    xmlNewChild(xmlNewChild(psField, NULL, BAD_CAST "Definition", NULL),
                psOwsNs, BAD_CAST "AnyValue", NULL);

    /* NullValue */
    value = msOWSGetEncodeMetadata(&(layer->metadata), "CO",
                                   "rangeset_nullvalue", NULL);
    if (value)
      xmlNewChild(psField, NULL, BAD_CAST "NullValue", BAD_CAST value);
    msFree(value);

    /* InterpolationMethods */
    psInterpMethods =
        xmlNewChild(psField, NULL, BAD_CAST "InterpolationMethods", NULL);

    xmlNewChild(psInterpMethods, NULL, BAD_CAST "InterpolationMethod",
                BAD_CAST "bilinear");
    xmlNewChild(psInterpMethods, NULL, BAD_CAST "Default",
                BAD_CAST "nearest neighbor");

    /* -------------------------------------------------------------------- */
    /*      Bands axis.                                                     */
    /* -------------------------------------------------------------------- */
    {
      xmlNodePtr psKeys;
      int iBand;

      value = msOWSGetEncodeMetadata(&(layer->metadata), "CO", "bands_name",
                                     "bands");
      psAxis = xmlNewChild(psField, NULL, BAD_CAST "Axis", NULL);
      xmlNewProp(psAxis, BAD_CAST "identifier", BAD_CAST value);
      msFree(value);

      psKeys = xmlNewChild(psAxis, NULL, BAD_CAST "AvailableKeys", NULL);

      for (iBand = 0; iBand < cm.bandcount; iBand++) {
        char szBandName[32];

        snprintf(szBandName, sizeof(szBandName), "%d", iBand + 1);
        xmlNewChild(psKeys, NULL, BAD_CAST "Key", BAD_CAST szBandName);
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      SupportedCRS                                                    */
  /* -------------------------------------------------------------------- */
  {
    char *owned_value;

    if ((owned_value = msOWSGetProjURN(&(layer->projection), &(layer->metadata),
                                       "CO", MS_FALSE)) != NULL) {
      /* ok */
    } else if ((owned_value = msOWSGetProjURN(&(layer->map->projection),
                                              &(layer->map->web.metadata), "CO",
                                              MS_FALSE)) != NULL) {
      /* ok */
    } else
      msDebug("mapwcs.c: missing required information, no SRSs defined.\n");

    if (owned_value != NULL && strlen(owned_value) > 0)
      msLibXml2GenerateList(psCD, NULL, "SupportedCRS", owned_value, ' ');

    msFree(owned_value);
  }

  /* -------------------------------------------------------------------- */
  /*      SupportedFormats                                                */
  /* -------------------------------------------------------------------- */
  {
    char *format_list;

    format_list = msWCSGetFormatsList11(layer->map, layer);

    if (strlen(format_list) > 0)
      msLibXml2GenerateList(psCD, NULL, "SupportedFormat", format_list, ',');

    msFree(format_list);
  }
  msWCSFreeCoverageMetadata(&cm);

  return MS_SUCCESS;
}

/************************************************************************/
/*                      msWCSDescribeCoverage11()                       */
/************************************************************************/

int msWCSDescribeCoverage11(mapObj *map, wcsParamsObj *params,
                            owsRequestObj *ows_request) {
  xmlDocPtr psDoc = NULL; /* document pointer */
  xmlNodePtr psRootNode;
  xmlNsPtr psOwsNs;
  char *schemaLocation = NULL;
  char *xsi_schemaLocation = NULL;

  int i, j;

  /* -------------------------------------------------------------------- */
  /*      We will actually get the coverages list as a single item in     */
  /*      a string list with that item having the comma delimited         */
  /*      coverage names.  Split it up now, and assign back in place      */
  /*      of the old coverages list.                                      */
  /* -------------------------------------------------------------------- */
  if (CSLCount(params->coverages) == 1) {
    char **old_coverages = params->coverages;
    params->coverages =
        CSLTokenizeStringComplex(old_coverages[0], ",", FALSE, FALSE);
    CSLDestroy(old_coverages);
  }

  /* -------------------------------------------------------------------- */
  /*      Validate that the requested coverages exist as named layers.    */
  /* -------------------------------------------------------------------- */
  if (params->coverages) { /* use the list */
    for (j = 0; params->coverages[j]; j++) {
      i = msGetLayerIndex(map, params->coverages[j]);
      if ((i == -1) || (!msIntegerInArray(GET_LAYER(map, i)->index,
                                          ows_request->enabled_layers,
                                          ows_request->numlayers))) {
        msSetError(MS_WCSERR, "COVERAGE %s cannot be opened / does not exist",
                   "msWCSDescribeCoverage()", params->coverages[j]);
        return msWCSException11(map, "CoverageNotDefined", "coverage",
                                params->version);
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Create document.                                                */
  /* -------------------------------------------------------------------- */
  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = xmlNewNode(NULL, BAD_CAST "CoverageDescriptions");

  xmlDocSetRootElement(psDoc, psRootNode);

  /* -------------------------------------------------------------------- */
  /*      Name spaces                                                     */
  /* -------------------------------------------------------------------- */
  xmlSetNs(
      psRootNode,
      xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/wcs/1.1", NULL));
  psOwsNs = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_110_NAMESPACE_URI,
                     BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI,
           BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI,
           BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_URI,
           BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_PREFIX);

  schemaLocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
  xsi_schemaLocation = msStrdup("http://www.opengis.net/wcs/1.1");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemaLocation);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation,
                                           "/wcs/1.1/wcsDescribeCoverage.xsd ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation,
                                           MS_OWSCOMMON_OWS_110_NAMESPACE_URI);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemaLocation);
  xsi_schemaLocation =
      msStringConcatenate(xsi_schemaLocation, "/ows/1.1.0/owsAll.xsd");
  xmlNewNsProp(psRootNode, NULL, BAD_CAST "xsi:schemaLocation",
               BAD_CAST xsi_schemaLocation);
  msFree(schemaLocation);
  msFree(xsi_schemaLocation);

  /* -------------------------------------------------------------------- */
  /*      Generate a CoverageDescription for each requested coverage.     */
  /* -------------------------------------------------------------------- */

  if (params->coverages) { /* use the list */
    for (j = 0; params->coverages[j]; j++) {
      i = msGetLayerIndex(map, params->coverages[j]);
      msWCSDescribeCoverage_CoverageDescription11((GET_LAYER(map, i)), params,
                                                  psRootNode, psOwsNs);
    }
  } else { /* return all layers */
    for (i = 0; i < map->numlayers; i++) {

      if (!msIntegerInArray(GET_LAYER(map, i)->index,
                            ows_request->enabled_layers,
                            ows_request->numlayers))
        continue;

      msWCSDescribeCoverage_CoverageDescription11((GET_LAYER(map, i)), params,
                                                  psRootNode, psOwsNs);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Write out the document.                                         */
  /* -------------------------------------------------------------------- */
  {
    xmlChar *buffer = NULL;
    int size = 0;
    msIOContext *context = NULL;

    if (msIO_needBinaryStdout() == MS_FAILURE)
      return MS_FAILURE;

    msIO_setHeader("Content-Type", "text/xml; charset=UTF-8");
    msIO_sendHeaders();

    context = msIO_getHandler(stdout);

    xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, "UTF-8", 1);
    msIO_contextWrite(context, buffer, size);
    xmlFree(buffer);
  }

  /* -------------------------------------------------------------------- */
  /*      Cleanup                                                         */
  /* -------------------------------------------------------------------- */
  xmlFreeDoc(psDoc);
  xmlCleanupParser();

  return MS_SUCCESS;
}

#endif /* defined(USE_WCS_SVR) && defined(USE_LIBXML2) */

/************************************************************************/
/*                      msWCSGetCoverageBands11()                       */
/*                                                                      */
/*      We expect input to be of the form:                              */
/*      RangeSubset=raster:interpolation[bands[1]].                     */
/*                                                                      */
/*      RangeSet=raster:[bands[1,2]]                                    */
/*       or                                                             */
/*      RangeSet=raster:bilinear                                        */
/*                                                                      */
/*      This function tries to return a bandlist if found, and will     */
/*      also push an INTERPOLATION keyword into the parameters list     */
/*      if found in the RangeSubset.                                    */
/************************************************************************/

#if defined(USE_WCS_SVR)
int msWCSGetCoverageBands11(mapObj *map, cgiRequestObj *request,
                            wcsParamsObj *params, layerObj *lp,
                            char **p_bandlist)

{
  char *rangesubset, *field_id;
  const char *axis_id;
  int i;

  /* -------------------------------------------------------------------- */
  /*      Fetch the RangeSubset from the parameters, skip building a      */
  /*      bands list if not found.                                        */
  /* -------------------------------------------------------------------- */
  const char *value = msWCSGetRequestParameter(request, "RangeSubset");
  if (value == NULL)
    return MS_SUCCESS;

  rangesubset = msStrdup(value);

  /* -------------------------------------------------------------------- */
  /*      What is the <Field identifier=...> (rangeset_name)?             */
  /* -------------------------------------------------------------------- */
  value = msOWSLookupMetadata(&(lp->metadata), "CO", "rangeset_name");
  if (value == NULL)
    value = "raster";
  field_id = msStrdup(value);

  /* -------------------------------------------------------------------- */
  /*      What is the <Axis identifier=...> (bands_name)?                 */
  /* -------------------------------------------------------------------- */
  axis_id = msOWSLookupMetadata(&(lp->metadata), "CO", "bands_name");
  if (axis_id == NULL)
    axis_id = "bands";

  /* -------------------------------------------------------------------- */
  /*      Parse out the field identifier from the request and verify.     */
  /* -------------------------------------------------------------------- */
  value = rangesubset + strlen(field_id);

  if (strcasecmp(rangesubset, field_id) == 0) {
    free(rangesubset);
    free(field_id);
    return MS_SUCCESS; /* we only got field ... default options */
  }

  if (strlen(rangesubset) <= strlen(field_id) + 1 ||
      strncasecmp(rangesubset, field_id, strlen(field_id)) != 0 ||
      (*value != '[' && *value != ':')) {
    msSetError(
        MS_WCSERR,
        "RangeSubset field name malformed, expected '%s', got RangeSubset=%s",
        "msWCSGetCoverageBands11()", field_id, rangesubset);
    free(rangesubset);
    free(field_id);
    return msWCSException11(map, "NoApplicableCode", "mapserv",
                            params->version);
  }

  free(field_id);
  field_id = NULL;

  /* -------------------------------------------------------------------- */
  /*      Parse out the interpolation, if found.                          */
  /* -------------------------------------------------------------------- */
  if (*value == ':') {
    assert(params->interpolation == NULL);
    params->interpolation = msStrdup(value + 1);
    for (i = 0; params->interpolation[i] != '\0'; i++) {
      if (params->interpolation[i] == '[') {
        params->interpolation[i] = '\0';
        break;
      }
    }

    value += strlen(params->interpolation) + 1;
  }

  /* -------------------------------------------------------------------- */
  /*      Parse out the axis name, and verify.                            */
  /* -------------------------------------------------------------------- */
  if (*value != '[') {
    free(rangesubset);
    return MS_SUCCESS;
  }

  value++;

  if (strlen(value) <= strlen(axis_id) + 1 ||
      strncasecmp(value, axis_id, strlen(axis_id)) != 0 ||
      value[strlen(axis_id)] != '[') {
    msSetError(
        MS_WCSERR,
        "RangeSubset axis name malformed, expected '%s', got RangeSubset=%s",
        "msWCSGetCoverageBands11()", axis_id, rangesubset);
    free(rangesubset);
    return msWCSException11(map, "NoApplicableCode", "mapserv",
                            params->version);
  }

  /* -------------------------------------------------------------------- */
  /*      Parse the band list.  Basically assuming the band list is       */
  /*      everything from here to a close ';'.                            */
  /* -------------------------------------------------------------------- */
  value += strlen(axis_id) + 1;

  *p_bandlist = msStrdup(value);

  for (i = 0; (*p_bandlist)[i] != '\0'; i++) {
    if ((*p_bandlist)[i] == '[') {
      (*p_bandlist)[i] = '\0';
      break;
    }
  }
  free(rangesubset);
  return MS_SUCCESS;
}
#endif

/************************************************************************/
/*                       msWCSReturnCoverage11()                        */
/*                                                                      */
/*      Return a render image as a coverage to the caller with WCS      */
/*      1.1 "mime" wrapping.                                            */
/************************************************************************/

#if defined(USE_WCS_SVR)
int msWCSReturnCoverage11(wcsParamsObj *params, mapObj *map, imageObj *image) {
  int status, i;
  char *filename = NULL;
  char *base_dir = NULL;
  const char *fo_filename;

  fo_filename = msGetOutputFormatOption(image->format, "FILENAME", NULL);

  /* -------------------------------------------------------------------- */
  /*      Fetch the driver we will be using and check if it supports      */
  /*      VSIL IO.                                                        */
  /* -------------------------------------------------------------------- */
  if (EQUALN(image->format->driver, "GDAL/", 5)) {
    GDALDriverH hDriver;
    const char *pszExtension = image->format->extension;

    msAcquireLock(TLOCK_GDAL);
    hDriver = GDALGetDriverByName(image->format->driver + 5);
    if (hDriver == NULL) {
      msReleaseLock(TLOCK_GDAL);
      msSetError(MS_MISCERR, "Failed to find %s driver.",
                 "msWCSReturnCoverage11()", image->format->driver + 5);
      return msWCSException11(map, "NoApplicableCode", "mapserv",
                              params->version);
    }

    if (pszExtension == NULL)
      pszExtension = "img.tmp";

    if (msGDALDriverSupportsVirtualIOOutput(hDriver)) {
      base_dir = msTmpFile(map, map->mappath, "/vsimem/wcsout", NULL);
      if (fo_filename)
        filename = msStrdup(CPLFormFilename(base_dir, fo_filename, NULL));
      else
        filename = msStrdup(CPLFormFilename(base_dir, "out", pszExtension));

      /*            CleanVSIDir( "/vsimem/wcsout" ); */

      msReleaseLock(TLOCK_GDAL);
      status = msSaveImage(map, image, filename);
      if (status != MS_SUCCESS) {
        msFree(filename);
        msSetError(MS_MISCERR, "msSaveImage() failed",
                   "msWCSReturnCoverage11()");
        return msWCSException11(map, "NoApplicableCode", "mapserv",
                                params->version);
      }
    }
    msReleaseLock(TLOCK_GDAL);
  }

  /* -------------------------------------------------------------------- */
  /*      Output stock header.                                            */
  /* -------------------------------------------------------------------- */
  msIO_setHeader("Content-Type", "multipart/mixed; boundary=wcs");
  msIO_sendHeaders();
  msIO_printf("\r\n--wcs\r\n"
              "Content-Type: text/xml; charset=UTF-8\r\n"
              "Content-ID: wcs.xml\r\n\r\n"
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              "<Coverages\n"
              "     xmlns=\"http://www.opengis.net/wcs/1.1\"\n"
              "     xmlns:ows=\"http://www.opengis.net/ows/1.1\"\n"
              "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
              "     xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
              "     xsi:schemaLocation=\"http://www.opengis.net/ows/1.1 "
              "../owsCoverages.xsd\">\n"
              "  <Coverage>\n");

  /* -------------------------------------------------------------------- */
  /*      If we weren't able to write data under /vsimem, then we just    */
  /*      output a single "stock" filename.                               */
  /* -------------------------------------------------------------------- */
  if (filename == NULL) {
    msOutputFormatResolveFromImage(map, image);
    msIO_fprintf(stdout,
                 "    <ows:Reference xlink:href=\"cid:coverage/wcs.%s\"/>\n"
                 "  </Coverage>\n"
                 "</Coverages>\n"
                 "\r\n--wcs\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Description: coverage data\r\n"
                 "Content-Transfer-Encoding: binary\r\n"
                 "Content-ID: coverage/wcs.%s\r\n"
                 "Content-Disposition: INLINE\r\n\r\n",
                 MS_IMAGE_EXTENSION(map->outputformat),
                 MS_IMAGE_MIME_TYPE(map->outputformat),
                 MS_IMAGE_EXTENSION(map->outputformat));

    status = msSaveImage(map, image, NULL);
    if (status != MS_SUCCESS) {
      msSetError(MS_MISCERR, "msSaveImage() failed", "msWCSReturnCoverage11()");
      return msWCSException11(map, "NoApplicableCode", "mapserv",
                              params->version);
    }

    msIO_fprintf(stdout, "\r\n--wcs--\r\n");
    return MS_SUCCESS;
  }

  /* -------------------------------------------------------------------- */
  /*      When potentially listing multiple files, we take great care     */
  /*      to identify the "primary" file and list it first.  In fact      */
  /*      it is the only file listed in the coverages document.           */
  /* -------------------------------------------------------------------- */
  {
    char **all_files = CPLReadDir(base_dir);
    int count = CSLCount(all_files);

    if (msIO_needBinaryStdout() == MS_FAILURE)
      return MS_FAILURE;

    msAcquireLock(TLOCK_GDAL);
    for (i = count - 1; i >= 0; i--) {
      const char *this_file = all_files[i];

      if (EQUAL(this_file, ".") || EQUAL(this_file, "..")) {
        all_files = CSLRemoveStrings(all_files, i, 1, NULL);
        continue;
      }

      if (i > 0 && EQUAL(this_file, CPLGetFilename(filename))) {
        all_files = CSLRemoveStrings(all_files, i, 1, NULL);
        all_files = CSLInsertString(all_files, 0, CPLGetFilename(filename));
        i++;
      }
    }

    msIO_fprintf(stdout,
                 "    <ows:Reference xlink:href=\"cid:coverage/%s\"/>\n"
                 "  </Coverage>\n"
                 "</Coverages>\n",
                 CPLGetFilename(filename));

    /* -------------------------------------------------------------------- */
    /*      Dump all the files in the memory directory as mime sections.    */
    /* -------------------------------------------------------------------- */
    count = CSLCount(all_files);

    for (i = 0; i < count; i++) {
      const char *mimetype = NULL;
      VSILFILE *fp;
      unsigned char block[4000];
      int bytes_read;

      if (i == 0)
        mimetype = MS_IMAGE_MIME_TYPE(map->outputformat);

      if (mimetype == NULL)
        mimetype = "application/octet-stream";

      msIO_fprintf(stdout,
                   "\r\n--wcs\r\n"
                   "Content-Type: %s\r\n"
                   "Content-Description: coverage data\r\n"
                   "Content-Transfer-Encoding: binary\r\n"
                   "Content-ID: coverage/%s\r\n"
                   "Content-Disposition: INLINE\r\n\r\n",
                   mimetype, all_files[i]);

      fp = VSIFOpenL(CPLFormFilename(base_dir, all_files[i], NULL), "rb");
      if (fp == NULL) {
        msReleaseLock(TLOCK_GDAL);
        msSetError(MS_MISCERR, "Failed to open %s for streaming to stdout.",
                   "msWCSReturnCoverage11()", all_files[i]);
        return MS_FAILURE;
      }

      while ((bytes_read = VSIFReadL(block, 1, sizeof(block), fp)) > 0)
        msIO_fwrite(block, 1, bytes_read, stdout);

      VSIFCloseL(fp);

      VSIUnlink(CPLFormFilename(base_dir, all_files[i], NULL));
    }

    msFree(base_dir);
    msFree(filename);
    CSLDestroy(all_files);
    msReleaseLock(TLOCK_GDAL);

    msIO_fprintf(stdout, "\r\n--wcs--\r\n");
    return MS_SUCCESS;
  }
}
#endif /* defined(USE_WCS_SVR) && defined(USE_LIBXML2) */

/************************************************************************/
/* ==================================================================== */
/*  If we don't have libxml2 but WCS SVR was selected, then         */
/*      report WCS 1.1 requests as unsupported.                         */
/* ==================================================================== */
/************************************************************************/

#if defined(USE_WCS_SVR) && !defined(USE_LIBXML2)

#include "mapwcs.h"

/* ==================================================================== */

int msWCSDescribeCoverage11(mapObj *map, wcsParamsObj *params,
                            owsRequestObj *ows_request) {
  msSetError(MS_WCSERR,
             "WCS 1.1 request made, but mapserver requires libxml2 for WCS 1.1 "
             "services and this is not configured.",
             "msWCSDescribeCoverage11()");
  return msWCSException11(map, "NoApplicableCode", "mapserv", params->version);
}

/* ==================================================================== */

int msWCSGetCapabilities11(mapObj *map, wcsParamsObj *params,
                           cgiRequestObj *req, owsRequestObj *ows_request) {
  msSetError(MS_WCSERR,
             "WCS 1.1 request made, but mapserver requires libxml2 for WCS 1.1 "
             "services and this is not configured.",
             "msWCSGetCapabilities11()");

  return msWCSException11(map, "NoApplicableCode", "mapserv", params->version);
}

int msWCSException11(mapObj *map, const char *exceptionCode,
                     const char *locator, const char *version) {
  /* fallback to reporting using 1.0 style exceptions. */
  return msWCSException(map, exceptionCode, locator, "1.0.0");
}

#endif /* defined(USE_WCS_SVR) && !defined(USE_LIBXML2) */

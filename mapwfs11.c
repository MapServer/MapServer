/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC WFS 1.1.0 implementation. This file holds some WFS 1.1.0
 *           specific functions but other parts are still implemented in mapwfs.c.
 * Author:   Y. Assefa, DM Solutions Group (assefa@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2008, Y. Assefa, DM Solutions Group Inc
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 ****************************************************************************/

#include "mapserver.h"
#include "mapows.h"



#if defined(USE_WFS_SVR) && defined(USE_LIBXML2)
#include "maplibxml2.h"
#include "mapowscommon.h"
#include "mapogcfilter.h"

/************************************************************************/
/*                          msWFSException11()                          */
/************************************************************************/

int msWFSException11(mapObj *map, const char *locator,
                     const char *exceptionCode, const char *version)
{
  int size = 0;
  char *errorString     = NULL;
  char *errorMessage    = NULL;
  char *schemasLocation = NULL;
  const char *encoding;

  xmlDocPtr  psDoc      = NULL;
  xmlNodePtr psRootNode = NULL;
  xmlNsPtr   psNsOws    = NULL;
  xmlChar *buffer       = NULL;

  if (version == NULL)
    version = "1.1.0";

  psNsOws = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/ows", BAD_CAST "ows");

  encoding = msOWSLookupMetadata(&(map->web.metadata), "FO", "encoding");

  errorString = msGetErrorString("\n");
  errorMessage = msEncodeHTMLEntities(errorString);
  schemasLocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = msOWSCommonExceptionReport(psNsOws, OWS_1_0_0, schemasLocation, version, msOWSGetLanguage(map, "exception"), exceptionCode, locator, errorMessage);

  xmlDocSetRootElement(psDoc, psRootNode);

  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/ows", BAD_CAST "ows");

  if (encoding)
    msIO_setHeader("Content-type","text/xml; charset=%s", encoding);
  else
    msIO_setHeader("Content-type","text/xml");
  msIO_sendHeaders();

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, (encoding ? encoding : "ISO-8859-1"), 1);

  msIO_printf("%s", buffer);

  /*free buffer and the document */
  free(errorString);
  free(errorMessage);
  free(schemasLocation);
  xmlFree(buffer);
  xmlFreeDoc(psDoc);
  xmlFreeNs(psNsOws);

  /* clear error since we have already reported it */
  msResetErrorList();

  return MS_FAILURE;
}


/************************************************************************/
/*                             msWFSDumpLayer11                         */
/************************************************************************/
static xmlNodePtr msWFSDumpLayer11(mapObj *map, layerObj *lp, xmlNsPtr psNsOws)
{
  rectObj ext;

  xmlNodePtr psRootNode, psNode;
  const char *value    = NULL;
  const char *encoding = NULL;
  char *encoded=NULL;
  char *valueToFree;
  char **tokens;
  int n=0,i=0;

  encoding = msOWSLookupMetadata(&(map->web.metadata), "FO", "encoding");
  if (!encoding)
    encoding = "ISO-8859-1";

  psRootNode = xmlNewNode(NULL, BAD_CAST "FeatureType");

  /* add namespace to layer name */
  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_prefix");
  if(value) {
    n = strlen(value)+strlen(lp->name)+1+1;
    valueToFree = (char *) msSmallMalloc(sizeof(char*)*n);
    snprintf(valueToFree, n, "%s%s%s", (value ? value : ""), (value ? ":" : ""), lp->name);

    /*if there is an encoding using it on some of the items*/
    psNode = msOWSCommonxmlNewChildEncoded(psRootNode, NULL, "Name", valueToFree, encoding);
    msFree(valueToFree);
  } else {
    psNode = msOWSCommonxmlNewChildEncoded(psRootNode, NULL, "Name", lp->name, encoding);
  }

  if (lp->name && strlen(lp->name) > 0 &&
      (msIsXMLTagValid(lp->name) == MS_FALSE || isdigit(lp->name[0])))
    xmlAddSibling(psNode,
                  xmlNewComment(BAD_CAST "WARNING: The layer name '%s' might contain spaces or "
                                "invalid characters or may start with a number. This could lead to potential problems"));

  value = msOWSLookupMetadata(&(lp->metadata), "FO", "title");
  if (!value)
    value =(const char*)lp->name;

  psNode = msOWSCommonxmlNewChildEncoded(psRootNode, NULL, "Title", value, encoding);


  value = msOWSLookupMetadata(&(lp->metadata), "FO", "abstract");
  if (value)
    psNode = msOWSCommonxmlNewChildEncoded(psRootNode, NULL, "Abstract", value, encoding);



  value = msOWSLookupMetadata(&(lp->metadata), "FO", "keywordlist");

  if (value) {
    if (encoding)
      encoded = msGetEncodedString(value, encoding);
    else
      encoded = msGetEncodedString(value, "ISO-8859-1");

    msLibXml2GenerateList(
      xmlNewChild(psRootNode, psNsOws, BAD_CAST "Keywords", NULL),
      NULL, "Keyword", encoded, ',' );
    msFree(encoded);
  }
  /*support DefaultSRS and OtherSRS*/
  valueToFree = msOWSGetProjURN(&(map->projection),&(map->web.metadata),"FO",MS_FALSE);
  if (!valueToFree)
    valueToFree = msOWSGetProjURN(&(lp->projection), &(lp->metadata), "FO", MS_FALSE);

  if (valueToFree) {
    tokens = msStringSplit(valueToFree, ' ', &n);
    if (tokens && n > 0) {
      psNode = xmlNewChild(psRootNode, NULL, BAD_CAST "DefaultSRS", BAD_CAST tokens[0]);
      for (i=1; i<n; i++)
        psNode = xmlNewChild(psRootNode, NULL, BAD_CAST "OtherSRS", BAD_CAST tokens[i]);

      msFreeCharArray(tokens, n);
    }
  } else
    xmlAddSibling(psNode,
                  xmlNewComment(BAD_CAST "WARNING: Mandatory mapfile parameter: (at least one of) MAP.PROJECTION, LAYER.PROJECTION or wfs/ows_srs metadata was missing in this context."));

  free(valueToFree);
  valueToFree = NULL;

  /*TODO: adevertize only gml3?*/
  psNode = xmlNewNode(NULL, BAD_CAST "OutputFormats");
  xmlAddChild(psRootNode, psNode);

  {
    char *formats_list = msWFSGetOutputFormatList( map, lp, "1.1.0" );
    int iformat, n;
    char **tokens;

    n = 0;
    tokens = msStringSplit(formats_list, ',', &n);

    for( iformat = 0; iformat < n; iformat++ )
      xmlNewChild(psNode, NULL, BAD_CAST "Format",
                  BAD_CAST tokens[iformat] );
    msFree( formats_list );
    msFreeCharArray( tokens, n );
  }

  /*bbox*/
  if (msOWSGetLayerExtent(map, lp, "FO", &ext) == MS_SUCCESS) {
    /*convert to latlong*/
    if (lp->projection.numargs > 0) {
      if (!pj_is_latlong(&lp->projection.proj))
        msProjectRect(&lp->projection, NULL, &ext);
    } else if (map->projection.numargs > 0 && !pj_is_latlong(&map->projection.proj))
      msProjectRect(&map->projection, NULL, &ext);

    xmlAddChild(psRootNode,
                msOWSCommonWGS84BoundingBox( psNsOws, 2,
                    ext.minx, ext.miny,
                    ext.maxx, ext.maxy));
  } else {
    xmlNewChild(psRootNode, psNsOws, BAD_CAST "WGS84BoundingBox", NULL);
    xmlAddSibling(psNode,
                  xmlNewComment(BAD_CAST "WARNING: Optional WGS84BoundingBox could not be established for this layer.  Consider setting the EXTENT in the LAYER object, or wfs_extent metadata. Also check that your data exists in the DATA statement"));
  }

  value = msOWSLookupMetadata(&(lp->metadata), "FO", "metadataurl_href");

  if (value) {
    psNode = xmlNewChild(psRootNode, NULL, BAD_CAST "MetadataURL", BAD_CAST value);

    value = msOWSLookupMetadata(&(lp->metadata), "FO", "metadataurl_format");

    if (!value)
      value = msStrdup("text/html"); /* default */

    xmlNewProp(psNode, BAD_CAST "format", BAD_CAST value);

    value = msOWSLookupMetadata(&(lp->metadata), "FO", "metadataurl_type");

    if (!value)
      value = msStrdup("FGDC"); /* default */

    xmlNewProp(psNode, BAD_CAST "type", BAD_CAST value);
  }

  return psRootNode;
}

/************************************************************************/
/*                          msWFSGetCapabilities11                      */
/*                                                                      */
/*      Return the capabilities document for wfs 1.1.0                  */
/************************************************************************/
int msWFSGetCapabilities11(mapObj *map, wfsParamsObj *params,
                           cgiRequestObj *req, owsRequestObj *ows_request)
{
  xmlDocPtr psDoc = NULL;       /* document pointer */
  xmlNodePtr psRootNode, psMainNode, psNode, psFtNode;
  const char *updatesequence=NULL;
  xmlNsPtr psNsOws, psNsXLink, psNsOgc;
  char *schemalocation = NULL;
  char *xsi_schemaLocation = NULL;
  const char *user_namespace_prefix = NULL;
  const char *user_namespace_uri = NULL;
  gmlNamespaceListObj *namespaceList=NULL; /* for external application schema support */

  char *script_url=NULL, *script_url_encoded=NULL, *formats_list;
  const char *value = NULL;
  const char *encoding;

  xmlChar *buffer = NULL;
  int size = 0, i;
  msIOContext *context = NULL;

  int ows_version = OWS_1_0_0;

  /* -------------------------------------------------------------------- */
  /*      Handle updatesequence                                           */
  /* -------------------------------------------------------------------- */

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "FO", "updatesequence");

  encoding = msOWSLookupMetadata(&(map->web.metadata), "FO", "encoding");

  if (params->pszUpdateSequence != NULL) {
    i = msOWSNegotiateUpdateSequence(params->pszUpdateSequence, updatesequence);
    if (i == 0) { /* current */
      msSetError(MS_WFSERR, "UPDATESEQUENCE parameter (%s) is equal to server (%s)", "msWFSGetCapabilities11()", params->pszUpdateSequence, updatesequence);
      return msWFSException11(map, "updatesequence", "CurrentUpdateSequence", params->pszVersion);
    }
    if (i > 0) { /* invalid */
      msSetError(MS_WFSERR, "UPDATESEQUENCE parameter (%s) is higher than server (%s)", "msWFSGetCapabilities11()", params->pszUpdateSequence, updatesequence);
      return msWFSException11(map, "updatesequence", "InvalidUpdateSequence", params->pszVersion);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Create document.                                                */
  /* -------------------------------------------------------------------- */
  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = xmlNewNode(NULL, BAD_CAST "WFS_Capabilities");

  xmlDocSetRootElement(psDoc, psRootNode);

  /* -------------------------------------------------------------------- */
  /*      Name spaces                                                     */
  /* -------------------------------------------------------------------- */
  /*default name space*/
  xmlNewProp(psRootNode, BAD_CAST "xmlns", BAD_CAST "http://www.opengis.net/wfs");

  xmlSetNs(psRootNode, xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml"));
  xmlSetNs(psRootNode, xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/wfs", BAD_CAST "wfs"));

  psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);
  psNsXLink = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_PREFIX );

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_uri");
  if(value) user_namespace_uri = value;

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_prefix");
  if(value) user_namespace_prefix = value;
  if(user_namespace_prefix != NULL && msIsXMLTagValid(user_namespace_prefix) == MS_FALSE)
    msIO_printf("<!-- WARNING: The value '%s' is not valid XML namespace. -->\n", user_namespace_prefix);
  else
    xmlNewNs(psRootNode, BAD_CAST user_namespace_uri, BAD_CAST user_namespace_prefix);

  /* any additional namespaces */
  namespaceList = msGMLGetNamespaces(&(map->web), "G");
  for(i=0; i<namespaceList->numnamespaces; i++) {
    if(namespaceList->namespaces[i].uri) {
      xmlNewNs(psRootNode, BAD_CAST namespaceList->namespaces[i].uri, BAD_CAST namespaceList->namespaces[i].prefix);
    }
  }

  xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST params->pszVersion );

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "FO", "updatesequence");

  if (updatesequence)
    xmlNewProp(psRootNode, BAD_CAST "updateSequence", BAD_CAST updatesequence);

  /*schema*/
  schemalocation = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );
  xsi_schemaLocation = msStrdup("http://www.opengis.net/wfs");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemalocation);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/wfs/1.1.0/wfs.xsd");
  xmlNewNsProp(psRootNode, NULL, BAD_CAST "xsi:schemaLocation", BAD_CAST xsi_schemaLocation);

  /* -------------------------------------------------------------------- */
  /*      Service metadata.                                               */
  /* -------------------------------------------------------------------- */

  xmlAddChild(psRootNode,
                          msOWSCommonServiceIdentification(psNsOws, map, "OGC WFS", params->pszVersion, "FO"));

  /*service provider*/
  xmlAddChild(psRootNode, msOWSCommonServiceProvider(
                            psNsOws, psNsXLink, map, "FO"));

  /*operation metadata */
  if ((script_url=msOWSGetOnlineResource(map, "FO", "onlineresource", req)) == NULL
      || (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL) {
    msSetError(MS_WFSERR, "Server URL not found", "msWFSGetCapabilities11()");
    return msWFSException11(map, "mapserv", "NoApplicableCode", params->pszVersion);
  }

  /* -------------------------------------------------------------------- */
  /*      Operations metadata.                                            */
  /* -------------------------------------------------------------------- */
  psMainNode= xmlAddChild(psRootNode,msOWSCommonOperationsMetadata(psNsOws));

  /* -------------------------------------------------------------------- */
  /*      GetCapabilities                                                 */
  /* -------------------------------------------------------------------- */
  psNode = xmlAddChild(psMainNode,
                       msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"GetCapabilities",
                           OWS_METHOD_GETPOST, script_url_encoded));

  xmlAddChild(psMainNode, psNode);
  xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                ows_version, psNsOws, "Parameter", "service", "WFS"));
  /*accept version*/
  xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
              "Parameter", "AcceptVersions",
              "1.0.0,1.1.0"));
  /*format*/
  xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
              "Parameter", "AcceptFormats",
              "text/xml"));


  /* -------------------------------------------------------------------- */
  /*      DescribeFeatureType                                             */
  /* -------------------------------------------------------------------- */
  if (msOWSRequestIsEnabled(map, NULL, "F", "DescribeFeatureType", MS_TRUE)) {
    psNode = xmlAddChild(psMainNode,
                         msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"DescribeFeatureType",
                             OWS_METHOD_GETPOST, script_url_encoded));
    xmlAddChild(psMainNode, psNode);

    /*output format*/
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                "Parameter", "outputFormat",
                "XMLSCHEMA,text/xml; subtype=gml/2.1.2,text/xml; subtype=gml/3.1.1"));
  }

  /* -------------------------------------------------------------------- */
  /*      GetFeature                                                      */
  /* -------------------------------------------------------------------- */
  if (msOWSRequestIsEnabled(map, NULL, "F", "GetFeature", MS_TRUE)) {

    psNode = xmlAddChild(psMainNode,
                         msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"GetFeature",
                             OWS_METHOD_GETPOST, script_url_encoded));
    xmlAddChild(psMainNode, psNode);

    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                "Parameter", "resultType",
                "results,hits"));

    formats_list = msWFSGetOutputFormatList( map, NULL, "1.1.0" );
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                "Parameter", "outputFormat",
                formats_list));
    msFree( formats_list );

    value = msOWSLookupMetadata(&(map->web.metadata), "FO", "maxfeatures");

    if (value) {
      xmlAddChild(psMainNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                  "Constraint", "DefaultMaxFeatures",
                  (char *)value));
    }
  }

  /* -------------------------------------------------------------------- */
  /*      FeatureTypeList                                                 */
  /* -------------------------------------------------------------------- */

  psFtNode = xmlNewNode(NULL, BAD_CAST "FeatureTypeList");
  xmlAddChild(psRootNode, psFtNode);
  psNode = xmlNewChild(psFtNode, NULL, BAD_CAST "Operations", NULL);
  xmlNewChild(psNode, NULL, BAD_CAST "Operation", BAD_CAST "Query");

  for(i=0; i<map->numlayers; i++) {
    layerObj *lp;
    lp = GET_LAYER(map, i);

    if (!msIntegerInArray(lp->index, ows_request->enabled_layers, ows_request->numlayers))
      continue;

    /* List only vector layers in which DUMP=TRUE */
    if (msWFSIsLayerSupported(lp))
      xmlAddChild(psFtNode, msWFSDumpLayer11(map, lp, psNsOws));
  }





  /* -------------------------------------------------------------------- */
  /*      Filter capabilities.                                            */
  /* -------------------------------------------------------------------- */

  psNsOgc = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_PREFIX);
  xmlAddChild(psRootNode, FLTGetCapabilities(psNsOgc, psNsOgc, MS_FALSE));
  /* -------------------------------------------------------------------- */
  /*      Write out the document.                                         */
  /* -------------------------------------------------------------------- */

  if( msIO_needBinaryStdout() == MS_FAILURE )
    return MS_FAILURE;

  if (encoding)
    msIO_setHeader("Content-type","text/xml; charset=%s", encoding);
  else
    msIO_setHeader("Content-type","text/xml");
  msIO_sendHeaders();

  context = msIO_getHandler(stdout);

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, (encoding ? encoding : "ISO-8859-1"), 1);
  msIO_contextWrite(context, buffer, size);
  xmlFree(buffer);

  /*free buffer and the document */
  /*xmlFree(buffer);*/
  xmlFreeDoc(psDoc);
  xmlFreeNs(psNsOgc);

  free(script_url);
  free(script_url_encoded);
  free(xsi_schemaLocation);
  free(schemalocation);

  xmlCleanupParser();

  return(MS_SUCCESS);
}


#endif /*defined(USE_WFS_SVR) && defined(USE_LIBXML2)*/

#if defined(USE_WFS_SVR) && !defined(USE_LIBXML2)



int msWFSGetCapabilities11(mapObj *map, wfsParamsObj *params,
                           cgiRequestObj *req, owsRequestObj *ows_request)

{
  msSetError( MS_WFSERR,
              "WFS 1.1 request made, but mapserver requires libxml2 for WFS 1.1 services and this is not configured.",
              "msWFSGetCapabilities11()", "NoApplicableCode" );

  return msWFSException11(map, "mapserv", "NoApplicableCode", params->pszVersion);
}

int msWFSException11(mapObj *map, const char *locator, const char *exceptionCode, const char *version)
{
  /* fallback to reporting using 1.0 style exceptions. */
  return msWFSException( map, locator, exceptionCode, "1.0.0" );
}

#endif /* defined(USE_WFS_SVR) && !defined(USE_LIBXML2) */

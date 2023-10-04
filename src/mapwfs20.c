/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC WFS 2.0.0 implementation. This file holds some WFS 2.0.0
 *           specific functions but other parts are still implemented in
 *mapwfs.c. Author:   Even Rouault
 *
 **********************************************************************
 * Copyright (c) 2013, Even Rouault
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

#define MS_OWS_11_NAMESPACE_PREFIX MS_OWSCOMMON_OWS_NAMESPACE_PREFIX
#define MS_OWS_11_NAMESPACE_URI MS_OWSCOMMON_OWS_110_NAMESPACE_URI

#define URN_GET_FEATURE_BY_ID "urn:ogc:def:query:OGC-WFS::GetFeatureById"

#define GET_FEATURE_BY_ID                                                      \
  "<StoredQueryDescription id=\"" URN_GET_FEATURE_BY_ID "\">"                  \
  "<Title>Get feature by identifier</Title>"                                   \
  "<Abstract>Returns the single feature whose value is equal to the "          \
  "specified value of the ID argument</Abstract>"                              \
  "<Parameter name=\"ID\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" "      \
  "type=\"xs:string\"/>"                                                       \
  "<QueryExpressionText isPrivate=\"true\" "                                   \
  "language=\"urn:ogc:def:queryLanguage:OGC-WFS::WFS_QueryExpression\" "       \
  "returnFeatureTypes=\"\">"                                                   \
  "<Query xmlns:fes=\"http://www.opengis.net/fes/2.0\" typeNames=\"?\">"       \
  "<fes:Filter><fes:ResourceId rid=\"${ID}\"/></fes:Filter>"                   \
  "</Query>"                                                                   \
  "</QueryExpressionText>"                                                     \
  "</StoredQueryDescription>"

/************************************************************************/
/*                          msWFSException20()                          */
/************************************************************************/

int msWFSException20(mapObj *map, const char *locator,
                     const char *exceptionCode) {
  int size = 0;
  char *errorString = NULL;

  xmlDocPtr psDoc = NULL;
  xmlNodePtr psRootNode = NULL;
  xmlNsPtr psNsOws = NULL;
  xmlChar *buffer = NULL;
  const char *status = NULL;

  psNsOws = xmlNewNs(NULL, BAD_CAST MS_OWS_11_NAMESPACE_URI,
                     BAD_CAST MS_OWS_11_NAMESPACE_PREFIX);

  errorString = msGetErrorString("\n");

  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = msOWSCommonExceptionReport(
      psNsOws, OWS_1_1_0, msOWSGetSchemasLocation(map), "2.0.0",
      msOWSGetLanguage(map, "exception"), exceptionCode, locator, errorString);

  xmlDocSetRootElement(psDoc, psRootNode);

  xmlNewNs(psRootNode, BAD_CAST MS_OWS_11_NAMESPACE_URI,
           BAD_CAST MS_OWS_11_NAMESPACE_PREFIX);

  /* Table D.2 OGC 09-025r1 - For CITE compliance */
  if (EQUAL(exceptionCode, MS_OWS_ERROR_OPERATION_NOT_SUPPORTED) ||
      EQUAL(exceptionCode, MS_OWS_ERROR_OPTION_NOT_SUPPORTED)) {
    status = "400 Not Implemented";
  } else if (EQUAL(exceptionCode, MS_OWS_ERROR_MISSING_PARAMETER_VALUE) ||
             EQUAL(exceptionCode, MS_OWS_ERROR_INVALID_PARAMETER_VALUE) ||
             EQUAL(exceptionCode, MS_OWS_ERROR_VERSION_NEGOTIATION_FAILED) ||
             EQUAL(exceptionCode, MS_OWS_ERROR_INVALID_UPDATE_SEQUENCE)) {
    status = "400 Bad request";
  } else if (EQUAL(exceptionCode, MS_WFS_ERROR_OPERATION_PROCESSING_FAILED)) {
    status = "403 Server processing failed";
  } else if (EQUAL(exceptionCode, MS_OWS_ERROR_NOT_FOUND)) {
    status = "404 Not Found";
  } else if (EQUAL(exceptionCode, MS_OWS_ERROR_NO_APPLICABLE_CODE)) {
    status = "400 Internal Server Error";
  }

  if (status != NULL) {
    msIO_setHeader("Status", "%s", status);
  }
  msIO_setHeader("Content-Type", "text/xml; charset=UTF-8");
  msIO_sendHeaders();

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, "UTF-8", 1);

  msIO_printf("%s", buffer);

  /*free buffer and the document */
  free(errorString);
  xmlFree(buffer);
  xmlFreeDoc(psDoc);
  xmlFreeNs(psNsOws);

  /* clear error since we have already reported it */
  msResetErrorList();

  return MS_FAILURE;
}

/************************************************************************/
/*                          msWFSIncludeSection                         */
/************************************************************************/
static int msWFSIncludeSection(wfsParamsObj *params, const char *pszSection) {
  int i;
  if (params->pszSections == NULL)
    return TRUE;
  for (i = 0; params->pszSections[i] != '\0'; i++) {
    if (strncasecmp(params->pszSections + i, "All", strlen("All")) == 0)
      return TRUE;
    if (strncasecmp(params->pszSections + i, pszSection, strlen(pszSection)) ==
        0)
      return TRUE;
  }
  return FALSE;
}

/************************************************************************/
/*                       msWFSConstraintDefaultValue                    */
/************************************************************************/

static xmlNodePtr msWFSConstraintDefaultValue(xmlNsPtr psNs, xmlNsPtr psNsOws,
                                              const char *name,
                                              const char *value) {
  xmlNodePtr psRootNode = NULL;

  psRootNode = xmlNewNode(psNs, BAD_CAST "Constraint");

  xmlNewProp(psRootNode, BAD_CAST "name", BAD_CAST name);

  xmlNewChild(psRootNode, psNsOws, BAD_CAST "NoValues", NULL);
  xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "DefaultValue", BAD_CAST value);

  return psRootNode;
}

/************************************************************************/
/*                              msWFSOperator                           */
/************************************************************************/

static xmlNodePtr msWFSOperator(xmlNsPtr psNsFES, const char *pszOperatorType,
                                const char *pszOperator) {
  xmlNodePtr psNode = xmlNewNode(psNsFES, BAD_CAST pszOperatorType);
  xmlNewProp(psNode, BAD_CAST "name", BAD_CAST pszOperator);
  return psNode;
}

/************************************************************************/
/*                       msWFS20FilterCapabilities                      */
/************************************************************************/

static xmlNodePtr msWFS20FilterCapabilities(xmlNsPtr psNsFES, xmlNsPtr psNsOws,
                                            int bImplementsSorting) {
  xmlNodePtr psRootNode = NULL, psNode = NULL, psSubNode = NULL;

  psRootNode = xmlNewNode(psNsFES, BAD_CAST "Filter_Capabilities");

  psNode = xmlNewChild(psRootNode, psNsFES, BAD_CAST "Conformance", NULL);

  xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws,
                                                  "ImplementsQuery", "TRUE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(
                          psNsFES, psNsOws, "ImplementsAdHocQuery", "TRUE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(
                          psNsFES, psNsOws, "ImplementsFunctions", "FALSE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(
                          psNsFES, psNsOws, "ImplementsResourceId", "TRUE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws,
                                                  "ImplementsMinStandardFilter",
                                                  "TRUE"));
  xmlAddChild(psNode,
              msWFSConstraintDefaultValue(psNsFES, psNsOws,
                                          "ImplementsStandardFilter", "TRUE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws,
                                                  "ImplementsMinSpatialFilter",
                                                  "TRUE"));
  xmlAddChild(psNode,
              msWFSConstraintDefaultValue(psNsFES, psNsOws,
                                          "ImplementsSpatialFilter", "FALSE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws,
                                                  "ImplementsMinTemporalFilter",
                                                  "TRUE"));
  xmlAddChild(psNode,
              msWFSConstraintDefaultValue(psNsFES, psNsOws,
                                          "ImplementsTemporalFilter", "FALSE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(
                          psNsFES, psNsOws, "ImplementsVersionNav", "FALSE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(
                          psNsFES, psNsOws, "ImplementsSorting",
                          (bImplementsSorting) ? "TRUE" : "FALSE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws,
                                                  "ImplementsExtendedOperators",
                                                  "FALSE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(
                          psNsFES, psNsOws, "ImplementsMinimumXPath", "TRUE"));
  xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws,
                                                  "ImplementsSchemaElementFunc",
                                                  "FALSE"));

  psNode = xmlNewChild(psRootNode, psNsFES, BAD_CAST "Id_Capabilities", NULL);
  psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "ResourceIdentifier", NULL);
  xmlNewProp(psSubNode, BAD_CAST "name", BAD_CAST "fes:ResourceId");

  psNode =
      xmlNewChild(psRootNode, psNsFES, BAD_CAST "Scalar_Capabilities", NULL);
  xmlNewChild(psNode, psNsFES, BAD_CAST "LogicalOperators", NULL);
  psNode = xmlNewChild(psNode, psNsFES, BAD_CAST "ComparisonOperators", NULL);
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator",
                                    "PropertyIsEqualTo"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator",
                                    "PropertyIsNotEqualTo"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator",
                                    "PropertyIsLessThan"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator",
                                    "PropertyIsGreaterThan"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator",
                                    "PropertyIsLessThanOrEqualTo"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator",
                                    "PropertyIsGreaterThanOrEqualTo"));
  xmlAddChild(psNode,
              msWFSOperator(psNsFES, "ComparisonOperator", "PropertyIsLike"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator",
                                    "PropertyIsBetween"));
  /* Missing: PropertyIsNull, PropertyIsNil */

  psNode =
      xmlNewChild(psRootNode, psNsFES, BAD_CAST "Spatial_Capabilities", NULL);

  psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "GeometryOperands", NULL);
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:Point"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiPoint"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:LineString"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiLineString"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:Curve"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiCurve"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:Polygon"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiPolygon"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:Surface"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiSurface"));
  /* xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand",
   * "gml:MultiGeometry")); */
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:Box"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "GeometryOperand", "gml:Envelope"));

  psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "SpatialOperators", NULL);
#ifdef USE_GEOS
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Equals"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Disjoint"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Touches"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Within"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Overlaps"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Crosses"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "SpatialOperator", "Intersects"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Contains"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "DWithin"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Beyond"));
#endif
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "BBOX"));

  psNode =
      xmlNewChild(psRootNode, psNsFES, BAD_CAST "Temporal_Capabilities", NULL);
  psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "TemporalOperands", NULL);
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "TemporalOperand", "gml:TimePeriod"));
  xmlAddChild(psSubNode,
              msWFSOperator(psNsFES, "TemporalOperand", "gml:TimeInstant"));

  psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "TemporalOperators", NULL);
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "TemporalOperator", "During"));

  return psRootNode;
}

/************************************************************************/
/*                          msXMLStripIndentation                       */
/************************************************************************/

static void msXMLStripIndentation(char *ptr) {
  /* Remove spaces between > and < to get properly indented result */
  char *afterLastClosingBracket = NULL;
  if (*ptr == ' ')
    afterLastClosingBracket = ptr;
  while (*ptr != '\0') {
    if (*ptr == '<' && afterLastClosingBracket != NULL) {
      memmove(afterLastClosingBracket, ptr, strlen(ptr) + 1);
      ptr = afterLastClosingBracket;
    } else if (*ptr == '>') {
      afterLastClosingBracket = ptr + 1;
    } else if (*ptr != ' ' && *ptr != '\n')
      afterLastClosingBracket = NULL;
    ptr++;
  }
}

/************************************************************************/
/*                          msWFSAddInspireDSID                         */
/************************************************************************/

static void msWFSAddInspireDSID(mapObj *map, xmlNsPtr psNsInspireDls,
                                xmlNsPtr psNsInspireCommon,
                                xmlNodePtr pDlsExtendedCapabilities) {
  const char *dsid_code =
      msOWSLookupMetadata(&(map->web.metadata), "FO", "inspire_dsid_code");
  const char *dsid_ns =
      msOWSLookupMetadata(&(map->web.metadata), "FO", "inspire_dsid_ns");
  if (dsid_code == NULL) {
    xmlAddChild(
        pDlsExtendedCapabilities,
        xmlNewComment(
            BAD_CAST
            "WARNING: Required metadata \"inspire_dsid_code\" missing"));
  } else {
    int ntokensCode = 0, ntokensNS = 0;
    char **tokensCode;
    char **tokensNS = NULL;
    int i;

    tokensCode = msStringSplit(dsid_code, ',', &ntokensCode);
    if (dsid_ns != NULL)
      tokensNS =
          msStringSplitComplex(dsid_ns, ",", &ntokensNS, MS_ALLOWEMPTYTOKENS);
    if (ntokensNS > 0 && ntokensNS != ntokensCode) {
      xmlAddChild(
          pDlsExtendedCapabilities,
          xmlNewComment(
              BAD_CAST
              "WARNING: \"inspire_dsid_code\" and \"inspire_dsid_ns\" have not "
              "the same number of elements. Ignoring inspire_dsid_ns"));
      msFreeCharArray(tokensNS, ntokensNS);
      tokensNS = NULL;
      ntokensNS = 0;
    }
    for (i = 0; i < ntokensCode; i++) {
      xmlNodePtr pSDSI =
          xmlNewNode(psNsInspireDls, BAD_CAST "SpatialDataSetIdentifier");
      xmlAddChild(pDlsExtendedCapabilities, pSDSI);
      xmlNewTextChild(pSDSI, psNsInspireCommon, BAD_CAST "Code",
                      BAD_CAST tokensCode[i]);
      if (ntokensNS > 0 && tokensNS[i][0] != '\0')
        xmlNewTextChild(pSDSI, psNsInspireCommon, BAD_CAST "Namespace",
                        BAD_CAST tokensNS[i]);
    }
    msFreeCharArray(tokensCode, ntokensCode);
    if (tokensNS)
      msFreeCharArray(tokensNS, ntokensNS);
  }
}

/************************************************************************/
/*                          msWFSGetCapabilities20                      */
/*                                                                      */
/*      Return the capabilities document for WFS 2.0.0                  */
/************************************************************************/
int msWFSGetCapabilities20(mapObj *map, wfsParamsObj *params,
                           cgiRequestObj *req, owsRequestObj *ows_request) {
  xmlDocPtr psDoc = NULL; /* document pointer */
  xmlNodePtr psRootNode, psMainNode, psNode, psFtNode = NULL;
  const char *updatesequence = NULL;
  xmlNsPtr psNsOws, psNsXLink;
  xmlNsPtr psNsFES = NULL;
  xmlNsPtr psNsInspireCommon = NULL;
  xmlNsPtr psNsInspireDls = NULL;
  xmlDocPtr pInspireTmpDoc = NULL;
  char *xsi_schemaLocation = NULL;
  const char *user_namespace_prefix = NULL;
  const char *user_namespace_uri = NULL;
  gmlNamespaceListObj *namespaceList =
      NULL; /* for external application schema support */

  char *script_url = NULL, *formats_list;
  const char *value = NULL;

  xmlChar *buffer = NULL;
  int size = 0, i;
  msIOContext *context = NULL;

  int ows_version = OWS_1_1_0;
  int ret;

  char *validated_language;
  int bImplementsSorting = MS_FALSE;

  /* -------------------------------------------------------------------- */
  /*      Handle updatesequence                                           */
  /* -------------------------------------------------------------------- */
  ret = msWFSHandleUpdateSequence(map, params, "msWFSGetCapabilities20()");
  if (ret != MS_SUCCESS)
    return ret;

  validated_language = msOWSGetLanguageFromList(map, "FO", params->pszLanguage);

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
  xmlNewProp(psRootNode, BAD_CAST "xmlns",
             BAD_CAST MS_OWSCOMMON_WFS_20_NAMESPACE_URI);

  xmlSetNs(psRootNode,
           xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_GML_32_NAMESPACE_URI,
                    BAD_CAST MS_OWSCOMMON_GML_NAMESPACE_PREFIX));
  xmlSetNs(psRootNode,
           xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_WFS_20_NAMESPACE_URI,
                    BAD_CAST MS_OWSCOMMON_WFS_NAMESPACE_PREFIX));

  psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_110_NAMESPACE_URI,
                     BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);
  psNsXLink =
      xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI,
               BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI,
           BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_FES_20_NAMESPACE_URI,
           BAD_CAST MS_OWSCOMMON_FES_20_NAMESPACE_PREFIX);

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_uri");
  if (value)
    user_namespace_uri = value;
  else
    user_namespace_uri = MS_DEFAULT_NAMESPACE_URI;

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_prefix");
  if (value)
    user_namespace_prefix = value;
  else
    user_namespace_prefix = MS_DEFAULT_NAMESPACE_PREFIX;

  if (user_namespace_prefix != NULL &&
      msIsXMLTagValid(user_namespace_prefix) == MS_FALSE)
    msIO_printf(
        "<!-- WARNING: The value '%s' is not valid XML namespace. -->\n",
        user_namespace_prefix);
  else
    xmlNewNs(psRootNode, BAD_CAST user_namespace_uri,
             BAD_CAST user_namespace_prefix);

  /* any additional namespaces */
  namespaceList = msGMLGetNamespaces(&(map->web), "G");
  for (i = 0; i < namespaceList->numnamespaces; i++) {
    if (namespaceList->namespaces[i].uri) {
      xmlNewNs(psRootNode, BAD_CAST namespaceList->namespaces[i].uri,
               BAD_CAST namespaceList->namespaces[i].prefix);
    }
  }
  msGMLFreeNamespaces(namespaceList);

  if (msOWSLookupMetadata(&(map->web.metadata), "FO", "inspire_capabilities")) {
    psNsInspireCommon =
        xmlNewNs(psRootNode, BAD_CAST MS_INSPIRE_COMMON_NAMESPACE_URI,
                 BAD_CAST MS_INSPIRE_COMMON_NAMESPACE_PREFIX);
    psNsInspireDls = xmlNewNs(psRootNode, BAD_CAST MS_INSPIRE_DLS_NAMESPACE_URI,
                              BAD_CAST MS_INSPIRE_DLS_NAMESPACE_PREFIX);
  }

  xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST params->pszVersion);

  updatesequence =
      msOWSLookupMetadata(&(map->web.metadata), "FO", "updatesequence");

  if (updatesequence)
    xmlNewProp(psRootNode, BAD_CAST "updateSequence", BAD_CAST updatesequence);

  /*schema*/
  xsi_schemaLocation = msStrdup(MS_OWSCOMMON_WFS_20_NAMESPACE_URI);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation =
      msStringConcatenate(xsi_schemaLocation, msOWSGetSchemasLocation(map));
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation,
                                           MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION);

  if (psNsInspireDls != NULL) {
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
    xsi_schemaLocation =
        msStringConcatenate(xsi_schemaLocation, MS_INSPIRE_DLS_NAMESPACE_URI);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
    xsi_schemaLocation = msStringConcatenate(
        xsi_schemaLocation, msOWSGetInspireSchemasLocation(map));
    xsi_schemaLocation =
        msStringConcatenate(xsi_schemaLocation, MS_INSPIRE_DLS_SCHEMA_LOCATION);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation,
                                             MS_INSPIRE_COMMON_NAMESPACE_URI);
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
    xsi_schemaLocation = msStringConcatenate(
        xsi_schemaLocation, msOWSGetInspireSchemasLocation(map));
    xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation,
                                             MS_INSPIRE_COMMON_SCHEMA_LOCATION);
  }

  xmlNewNsProp(psRootNode, NULL, BAD_CAST "xsi:schemaLocation",
               BAD_CAST xsi_schemaLocation);
  free(xsi_schemaLocation);

  /* -------------------------------------------------------------------- */
  /*      Service metadata.                                               */
  /* -------------------------------------------------------------------- */

  /* TODO? : also add 1.1.0 and 1.0.0 as extra <ows:ServiceTypeVersion>. The OWS
   */
  /* schema would suggest to do so, and also the example */
  /* http://schemas.opengis.net/wfs/2.0/examples/GetCapabilities/GetCapabilities_Res_01.xml
   */
  /* and Deegree too, but GeoServer also only lists the current version. */
  if (msWFSIncludeSection(params, "ServiceIdentification")) {
    xmlAddChild(psRootNode, msOWSCommonServiceIdentification(
                                psNsOws, map, "WFS", params->pszVersion, "FO",
                                validated_language));
  }

  /*service provider*/
  if (msWFSIncludeSection(params, "ServiceProvider")) {
    xmlAddChild(psRootNode,
                msOWSCommonServiceProvider(psNsOws, psNsXLink, map, "FO",
                                           validated_language));
  }

  /*operation metadata */
  if (msWFSIncludeSection(params, "OperationsMetadata")) {
    if ((script_url = msOWSGetOnlineResource2(map, "FO", "onlineresource", req,
                                              validated_language)) == NULL) {
      msSetError(MS_WFSERR, "Server URL not found", "msWFSGetCapabilities20()");
      return msWFSException11(map, "mapserv", MS_OWS_ERROR_NO_APPLICABLE_CODE,
                              params->pszVersion);
    }

    /* -------------------------------------------------------------------- */
    /*      Operations metadata.                                            */
    /* -------------------------------------------------------------------- */
    psMainNode =
        xmlAddChild(psRootNode, msOWSCommonOperationsMetadata(psNsOws));

    /* -------------------------------------------------------------------- */
    /*      GetCapabilities                                                 */
    /* -------------------------------------------------------------------- */
    psNode = xmlAddChild(psMainNode, msOWSCommonOperationsMetadataOperation(
                                         psNsOws, psNsXLink, "GetCapabilities",
                                         OWS_METHOD_GETPOST, script_url));

    xmlAddChild(psMainNode, psNode);

    /*accept version*/
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                            ows_version, psNsOws, "Parameter", "AcceptVersions",
                            "2.0.0,1.1.0,1.0.0"));
    /*format*/
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                            ows_version, psNsOws, "Parameter", "AcceptFormats",
                            "text/xml"));
    /*sections*/
    xmlAddChild(psNode,
                msOWSCommonOperationsMetadataDomainType(
                    ows_version, psNsOws, "Parameter", "Sections",
                    "ServiceIdentification,ServiceProvider,OperationsMetadata,"
                    "FeatureTypeList,Filter_Capabilities"));

    /* -------------------------------------------------------------------- */
    /*      DescribeFeatureType                                             */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "DescribeFeatureType", MS_TRUE)) {
      psNode =
          xmlAddChild(psMainNode, msOWSCommonOperationsMetadataOperation(
                                      psNsOws, psNsXLink, "DescribeFeatureType",
                                      OWS_METHOD_GETPOST, script_url));
      xmlAddChild(psMainNode, psNode);

      /*output format*/
      xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                              ows_version, psNsOws, "Parameter", "outputFormat",
                              "application/gml+xml; version=3.2,"
                              "text/xml; subtype=gml/3.2.1,"
                              "text/xml; subtype=gml/3.1.1,"
                              "text/xml; subtype=gml/2.1.2"));
    }

    /* -------------------------------------------------------------------- */
    /*      GetFeature                                                      */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "GetFeature", MS_TRUE)) {

      psNode = xmlAddChild(psMainNode, msOWSCommonOperationsMetadataOperation(
                                           psNsOws, psNsXLink, "GetFeature",
                                           OWS_METHOD_GETPOST, script_url));
      xmlAddChild(psMainNode, psNode);

      formats_list = msWFSGetOutputFormatList(map, NULL, OWS_2_0_0);
      xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                              ows_version, psNsOws, "Parameter", "outputFormat",
                              formats_list));
      msFree(formats_list);
    }

    /* -------------------------------------------------------------------- */
    /*      GetPropertyValue                                                */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "GetPropertyValue", MS_TRUE)) {

      psNode =
          xmlAddChild(psMainNode, msOWSCommonOperationsMetadataOperation(
                                      psNsOws, psNsXLink, "GetPropertyValue",
                                      OWS_METHOD_GETPOST, script_url));
      xmlAddChild(psMainNode, psNode);

      /* Only advertize built-in GML formats for GetPropertyValue. Not sure */
      /* it makes sense to advertize OGR formats. */
      xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(
                              ows_version, psNsOws, "Parameter", "outputFormat",
                              "application/gml+xml; version=3.2,"
                              "text/xml; subtype=gml/3.2.1,"
                              "text/xml; subtype=gml/3.1.1,"
                              "text/xml; subtype=gml/2.1.2"));
    }

    /* -------------------------------------------------------------------- */
    /*      ListStoredQueries                                               */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "ListStoredQueries", MS_TRUE)) {

      psNode =
          xmlAddChild(psMainNode, msOWSCommonOperationsMetadataOperation(
                                      psNsOws, psNsXLink, "ListStoredQueries",
                                      OWS_METHOD_GETPOST, script_url));
      xmlAddChild(psMainNode, psNode);
    }

    /* -------------------------------------------------------------------- */
    /*      DescribeStoredQueries                                           */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "DescribeStoredQueries",
                              MS_TRUE)) {

      psNode = xmlAddChild(psMainNode,
                           msOWSCommonOperationsMetadataOperation(
                               psNsOws, psNsXLink, "DescribeStoredQueries",
                               OWS_METHOD_GETPOST, script_url));
      xmlAddChild(psMainNode, psNode);
    }

    xmlAddChild(psMainNode, msOWSCommonOperationsMetadataDomainType(
                                ows_version, psNsOws, "Parameter", "version",
                                "2.0.0,1.1.0,1.0.0"));

    /* Conformance declaration */
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(psNsOws, psNsOws,
                                            "ImplementsBasicWFS", "TRUE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(
                    psNsOws, psNsOws, "ImplementsTransactionalWFS", "FALSE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(psNsOws, psNsOws,
                                            "ImplementsLockingWFS", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws,
                                                        "KVPEncoding", "TRUE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws,
                                                        "XMLEncoding", "TRUE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(
                                psNsOws, psNsOws, "SOAPEncoding", "FALSE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(psNsOws, psNsOws,
                                            "ImplementsInheritance", "FALSE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(
                    psNsOws, psNsOws, "ImplementsRemoteResolve", "FALSE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(psNsOws, psNsOws,
                                            "ImplementsResultPaging", "TRUE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(
                    psNsOws, psNsOws, "ImplementsStandardJoins", "FALSE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(psNsOws, psNsOws,
                                            "ImplementsSpatialJoins", "FALSE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(
                    psNsOws, psNsOws, "ImplementsTemporalJoins", "FALSE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(
                    psNsOws, psNsOws, "ImplementsFeatureVersioning", "FALSE"));
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(psNsOws, psNsOws,
                                            "ManageStoredQueries", "FALSE"));

    /* Capacity declaration */
    xmlAddChild(psMainNode,
                msWFSConstraintDefaultValue(
                    psNsOws, psNsOws, "PagingIsTransactionSafe", "FALSE"));

    value = msOWSLookupMetadata(&(map->web.metadata), "FO", "maxfeatures");
    if (value) {
      xmlAddChild(psMainNode, msWFSConstraintDefaultValue(
                                  psNsOws, psNsOws, "CountDefault", value));
    }

    xmlAddChild(psMainNode,
                msOWSCommonOperationsMetadataDomainType(
                    ows_version, psNsOws, "Constraint", "QueryExpressions",
                    "wfs:Query,wfs:StoredQuery"));

    /* Add Inspire Download Services extended capabilities */
    if (psNsInspireDls != NULL) {
      msIOContext *old_context;
      msIOContext *new_context;
      msIOBuffer *buffer;
      xmlNodePtr pRoot;
      xmlNodePtr pOWSExtendedCapabilities;
      xmlNodePtr pDlsExtendedCapabilities;
      xmlNodePtr pChild;

      old_context = msIO_pushStdoutToBufferAndGetOldContext();
      msOWSPrintInspireCommonExtendedCapabilities(
          stdout, map, "FO", OWS_WARN, "foo",
          "xmlns:" MS_INSPIRE_COMMON_NAMESPACE_PREFIX
          "=\"" MS_INSPIRE_COMMON_NAMESPACE_URI "\" "
          "xmlns:" MS_INSPIRE_DLS_NAMESPACE_PREFIX
          "=\"" MS_INSPIRE_DLS_NAMESPACE_URI "\" "
          "xmlns:xsi=\"" MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI "\"",
          validated_language, OWS_WFS);

      new_context = msIO_getHandler(stdout);
      buffer = (msIOBuffer *)new_context->cbData;

      /* Remove spaces between > and < to get properly indented result */
      msXMLStripIndentation((char *)buffer->data);

      pInspireTmpDoc = xmlParseDoc((const xmlChar *)buffer->data);
      pRoot = xmlDocGetRootElement(pInspireTmpDoc);
      xmlReconciliateNs(psDoc, pRoot);

      pOWSExtendedCapabilities =
          xmlNewNode(psNsOws, BAD_CAST "ExtendedCapabilities");
      xmlAddChild(psMainNode, pOWSExtendedCapabilities);

      pDlsExtendedCapabilities =
          xmlNewNode(psNsInspireDls, BAD_CAST "ExtendedCapabilities");
      xmlAddChild(pOWSExtendedCapabilities, pDlsExtendedCapabilities);

      pChild = pRoot->children;
      while (pChild != NULL) {
        xmlNodePtr pNext = pChild->next;
        xmlUnlinkNode(pChild);
        xmlAddChild(pDlsExtendedCapabilities, pChild);
        pChild = pNext;
      }

      msWFSAddInspireDSID(map, psNsInspireDls, psNsInspireCommon,
                          pDlsExtendedCapabilities);

      msIO_restoreOldStdoutContext(old_context);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      FeatureTypeList                                                 */
  /* -------------------------------------------------------------------- */
  if (msWFSIncludeSection(params, "FeatureTypeList")) {
    psFtNode = xmlNewNode(NULL, BAD_CAST "FeatureTypeList");
    xmlAddChild(psRootNode, psFtNode);
  }

  for (i = 0; i < map->numlayers; i++) {
    layerObj *lp;
    lp = GET_LAYER(map, i);

    if (!msIntegerInArray(lp->index, ows_request->enabled_layers,
                          ows_request->numlayers))
      continue;

    if (msWFSIsLayerSupported(lp)) {
      if (psFtNode != NULL) {
        xmlAddChild(psFtNode, msWFSDumpLayer11(map, lp, psNsOws, OWS_2_0_0,
                                               validated_language, script_url));
      }

      /* As soon as at least one layer supports sorting, advertize sorting */
      if (msLayerSupportsSorting(lp))
        bImplementsSorting = MS_TRUE;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Filter capabilities.                                            */
  /* -------------------------------------------------------------------- */
  if (msWFSIncludeSection(params, "Filter_Capabilities")) {
    psNsFES = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_FES_20_NAMESPACE_URI,
                       BAD_CAST MS_OWSCOMMON_FES_20_NAMESPACE_PREFIX);
    xmlAddChild(psRootNode, msWFS20FilterCapabilities(psNsFES, psNsOws,
                                                      bImplementsSorting));
  }

  /* -------------------------------------------------------------------- */
  /*      Write out the document.                                         */
  /* -------------------------------------------------------------------- */

  if (msIO_needBinaryStdout() == MS_FAILURE)
    return MS_FAILURE;

  msIO_setHeader("Content-Type", "text/xml; charset=UTF-8");
  msIO_sendHeaders();

  context = msIO_getHandler(stdout);

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, ("UTF-8"), 1);
  msIO_contextWrite(context, buffer, size);
  xmlFree(buffer);

  /*free buffer and the document */
  /*xmlFree(buffer);*/
  xmlFreeDoc(psDoc);
  if (psNsFES != NULL)
    xmlFreeNs(psNsFES);
  if (pInspireTmpDoc != NULL)
    xmlFreeDoc(pInspireTmpDoc);

  free(script_url);
  msFree(validated_language);

  xmlCleanupParser();

  return (MS_SUCCESS);
}

/************************************************************************/
/*                       msWFSGetStoredQueries                          */
/*                                                                      */
/* Result must be freed with msFreeCharArray()                          */
/************************************************************************/

static char **msWFSGetStoredQueries(mapObj *map, int *pn) {
  const char *value;
  char **tokens;
  int i, n;

  value = msOWSLookupMetadata(&(map->web.metadata), "F", "storedqueries");
  if (value != NULL) {
    tokens = msStringSplit(value, ',', &n);
    for (i = 0; i < n; i++) {
      if (strcasecmp(tokens[i], URN_GET_FEATURE_BY_ID) == 0)
        break;
    }
    /* Add mandatory GetFeatureById stored query if not found */
    if (i == n) {
      tokens = (char **)realloc(tokens, (n + 1) * sizeof(char *));
      memmove(tokens + 1, tokens, n * sizeof(char *));
      tokens[0] = msStrdup(URN_GET_FEATURE_BY_ID);
      n++;
    }
  } else {
    tokens = (char **)malloc(sizeof(char *));
    tokens[0] = msStrdup(URN_GET_FEATURE_BY_ID);
    n = 1;
  }
  *pn = n;
  return tokens;
}

/************************************************************************/
/*                       msWFSGetStoredQuery                            */
/*                                                                      */
/* Result must be freed with msFree()                                   */
/************************************************************************/

static char *msWFSGetStoredQuery(mapObj *map, const char *pszURN) {
  const char *value;
  char szKey[256];

  snprintf(szKey, sizeof(szKey), "%s_inlinedef", pszURN);
  value = msOWSLookupMetadata(&(map->web.metadata), "F", szKey);
  if (value != NULL)
    return msStrdup(value);

  snprintf(szKey, sizeof(szKey), "%s_filedef", pszURN);
  value = msOWSLookupMetadata(&(map->web.metadata), "F", szKey);
  if (value != NULL) {
    FILE *f = fopen(value, "rb");
    if (f != NULL) {
      char *pszBuffer;
      int nread;
      long length;

      fseek(f, 0, SEEK_END);
      length = ftell(f);
      if (length < 0 || length > 1000000) {
        msSetError(MS_WFSERR, "%s: too big (%ld bytes > 1000000)",
                   "msWFSGetStoredQuery()", value, length);
        fclose(f);
      } else {
        fseek(f, 0, SEEK_SET);
        pszBuffer = (char *)malloc((int)length + 1);
        if (pszBuffer == NULL) {
          msSetError(MS_WFSERR, "Cannot allocate %d bytes to read %s",
                     "msWFSGetStoredQuery()", (int)length + 1, value);
          fclose(f);
        } else {
          nread = (int)fread(pszBuffer, 1, length, f);
          fclose(f);
          if (nread == length) {
            pszBuffer[nread] = '\0';
            return pszBuffer;
          }
          msSetError(MS_WFSERR, "Could only read %d bytes / %d of %s",
                     "msWFSGetStoredQuery()", nread, (int)length, value);
          msFree(pszBuffer);
        }
      }
    } else {
      msSetError(MS_WFSERR, "Cannot open %s", "msWFSGetStoredQuery()", value);
    }
  }

  if (strcasecmp(pszURN, URN_GET_FEATURE_BY_ID) == 0)
    return msStrdup(GET_FEATURE_BY_ID);
  return NULL;
}

/************************************************************************/
/*                     msWFSGetResolvedStoredQuery20                    */
/*                                                                      */
/* Result must be freed with msFree()                                   */
/************************************************************************/

char *msWFSGetResolvedStoredQuery20(mapObj *map, wfsParamsObj *wfsparams,
                                    const char *id, hashTableObj *hashTable) {
  char *storedQuery;
  xmlDocPtr psStoredQueryDoc;
  xmlNodePtr psStoredQueryRoot, pChild;

  storedQuery = msWFSGetStoredQuery(map, id);
  if (storedQuery == NULL) {
    msSetError(MS_WFSERR, "Unknown stored query id: %s",
               "msWFSGetResolvedStoredQuery20()", id);
    msWFSException(map, "storedqueryid", MS_OWS_ERROR_INVALID_PARAMETER_VALUE,
                   wfsparams->pszVersion);
    return NULL;
  }

  psStoredQueryDoc = xmlParseDoc((const xmlChar *)storedQuery);
  if (psStoredQueryDoc == NULL) {
    msSetError(MS_WFSERR, "Definition for stored query '%s' is invalid",
               "msWFSGetResolvedStoredQuery20()", id);
    msWFSException(map, "mapserv", MS_OWS_ERROR_NO_APPLICABLE_CODE,
                   wfsparams->pszVersion);
    msFree(storedQuery);
    return NULL;
  }

  psStoredQueryRoot = xmlDocGetRootElement(psStoredQueryDoc);

  /* Check that all parameters are provided */
  pChild = psStoredQueryRoot->children;
  while (pChild != NULL) {
    if (pChild->type == XML_ELEMENT_NODE &&
        strcmp((const char *)pChild->name, "Parameter") == 0) {
      xmlChar *parameterName = xmlGetProp(pChild, BAD_CAST "name");
      if (parameterName != NULL) {
        char szTmp[256];
        const char *value =
            msLookupHashTable(hashTable, (const char *)parameterName);
        if (value == NULL) {
          msSetError(MS_WFSERR, "Stored query '%s' requires parameter '%s'",
                     "msWFSParseRequest()", id, (const char *)parameterName);
          msWFSException(map, (const char *)parameterName,
                         MS_OWS_ERROR_MISSING_PARAMETER_VALUE,
                         wfsparams->pszVersion);
          msFree(storedQuery);
          xmlFree(parameterName);
          xmlFreeDoc(psStoredQueryDoc);
          return NULL;
        }

        snprintf(szTmp, sizeof(szTmp), "${%s}", (const char *)parameterName);
        storedQuery = msReplaceSubstring(storedQuery, szTmp, value);
      }
      xmlFree(parameterName);
    }
    pChild = pChild->next;
  }

  xmlFreeDoc(psStoredQueryDoc);

  return storedQuery;
}

/************************************************************************/
/*                       msWFSListStoredQueries20                       */
/************************************************************************/

int msWFSListStoredQueries20(mapObj *map, owsRequestObj *ows_request) {
  xmlDocPtr psDoc;
  xmlChar *buffer = NULL;
  int size = 0;
  msIOContext *context = NULL;
  xmlNodePtr psRootNode;
  char *xsi_schemaLocation = NULL;
  int i, j;
  int nStoredQueries = 0;
  char **storedQueries = NULL;

  xmlDocPtr psStoredQueryDoc;
  xmlNodePtr psStoredQueryRoot;

  /* -------------------------------------------------------------------- */
  /*      Create document.                                                */
  /* -------------------------------------------------------------------- */
  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = xmlNewNode(NULL, BAD_CAST "ListStoredQueriesResponse");

  xmlDocSetRootElement(psDoc, psRootNode);

  /* -------------------------------------------------------------------- */
  /*      Name spaces                                                     */
  /* -------------------------------------------------------------------- */

  /*default name space*/
  xmlNewProp(psRootNode, BAD_CAST "xmlns",
             BAD_CAST MS_OWSCOMMON_WFS_20_NAMESPACE_URI);

  xmlSetNs(psRootNode,
           xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_WFS_20_NAMESPACE_URI,
                    BAD_CAST MS_OWSCOMMON_WFS_NAMESPACE_PREFIX));

  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI,
           BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);

  /*schema*/
  xsi_schemaLocation = msStrdup(MS_OWSCOMMON_WFS_20_NAMESPACE_URI);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation =
      msStringConcatenate(xsi_schemaLocation, msOWSGetSchemasLocation(map));
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation,
                                           MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION);

  xmlNewNsProp(psRootNode, NULL, BAD_CAST "xsi:schemaLocation",
               BAD_CAST xsi_schemaLocation);
  free(xsi_schemaLocation);

  /* -------------------------------------------------------------------- */
  /*      Add queries                                                     */
  /* -------------------------------------------------------------------- */

  storedQueries = msWFSGetStoredQueries(map, &nStoredQueries);
  for (i = 0; i < nStoredQueries; i++) {
    char *query = msWFSGetStoredQuery(map, storedQueries[i]);
    if (query != NULL) {
      xmlNodePtr pChild;
      xmlNodePtr psStoredQuery;

      psStoredQueryDoc = xmlParseDoc((const xmlChar *)query);
      if (psStoredQueryDoc == NULL) {
        char szMsg[256];
        msFree(query);
        snprintf(szMsg, sizeof(szMsg),
                 "WARNING: Definition for stored query %s is invalid",
                 storedQueries[i]);
        xmlAddChild(psRootNode, xmlNewComment(BAD_CAST szMsg));
        continue;
      }

      psStoredQueryRoot = xmlDocGetRootElement(psStoredQueryDoc);

      psStoredQuery = xmlNewNode(NULL, BAD_CAST "StoredQuery");
      xmlNewProp(psStoredQuery, BAD_CAST "id", BAD_CAST storedQueries[i]);
      xmlAddChild(psRootNode, psStoredQuery);

      pChild = psStoredQueryRoot->children;
      while (pChild != NULL) {
        xmlNodePtr pNext = pChild->next;
        if (pChild->type == XML_ELEMENT_NODE &&
            strcmp((const char *)pChild->name, "Title") == 0) {
          xmlUnlinkNode(pChild);
          xmlAddChild(psStoredQuery, pChild);
        } else if (pChild->type == XML_ELEMENT_NODE &&
                   strcmp((const char *)pChild->name, "QueryExpressionText") ==
                       0) {
          xmlNodePtr psReturnFeatureType;

          if (strcasecmp(storedQueries[i], URN_GET_FEATURE_BY_ID) == 0) {
            for (j = 0; j < map->numlayers; j++) {
              layerObj *lp;
              const char *user_namespace_prefix = MS_DEFAULT_NAMESPACE_PREFIX;
              const char *user_namespace_uri = MS_DEFAULT_NAMESPACE_URI;
              const char *value;
              char szValue[256];

              lp = GET_LAYER(map, j);

              if (!msIntegerInArray(lp->index, ows_request->enabled_layers,
                                    ows_request->numlayers) ||
                  !msWFSIsLayerSupported(lp))
                continue;

              value = msOWSLookupMetadata(&(map->web.metadata), "FO",
                                          "namespace_uri");
              if (value)
                user_namespace_uri = value;

              value = msOWSLookupMetadata(&(map->web.metadata), "FO",
                                          "namespace_prefix");
              if (value)
                user_namespace_prefix = value;

              psReturnFeatureType =
                  xmlNewNode(NULL, BAD_CAST "ReturnFeatureType");
              xmlNewNs(psReturnFeatureType, BAD_CAST user_namespace_uri,
                       BAD_CAST user_namespace_prefix);

              xmlAddChild(psStoredQuery, psReturnFeatureType);
              snprintf(szValue, sizeof(szValue), "%s:%s", user_namespace_prefix,
                       lp->name);
              xmlAddChild(psReturnFeatureType, xmlNewText(BAD_CAST szValue));
            }
          } else {
            xmlChar *returnFeatureTypes =
                xmlGetProp(pChild, BAD_CAST "returnFeatureTypes");
            if (returnFeatureTypes != NULL &&
                strlen((const char *)returnFeatureTypes) > 0) {
              int ntypes;
              char **types =
                  msStringSplit((const char *)returnFeatureTypes, ' ', &ntypes);
              for (j = 0; j < ntypes; j++) {
                psReturnFeatureType =
                    xmlNewNode(NULL, BAD_CAST "ReturnFeatureType");
                xmlAddChild(psStoredQuery, psReturnFeatureType);
                xmlAddChild(psReturnFeatureType, xmlNewText(BAD_CAST types[j]));
              }
              msFreeCharArray(types, ntypes);
            } else {
              psReturnFeatureType =
                  xmlNewNode(NULL, BAD_CAST "ReturnFeatureType");
              xmlAddChild(psStoredQuery, psReturnFeatureType);
              xmlAddChild(
                  psReturnFeatureType,
                  xmlNewComment(BAD_CAST "WARNING: Missing return type"));
            }
            xmlFree(returnFeatureTypes);
          }
        }
        pChild = pNext;
      }

      xmlReconciliateNs(psDoc, psStoredQuery);
      xmlFreeDoc(psStoredQueryDoc);
      msFree(query);
    } else {
      char szMsg[256];
      snprintf(szMsg, sizeof(szMsg),
               "WARNING: Definition for stored query %s missing",
               storedQueries[i]);
      xmlAddChild(psRootNode, xmlNewComment(BAD_CAST szMsg));
    }
  }
  msFreeCharArray(storedQueries, nStoredQueries);

  /* -------------------------------------------------------------------- */
  /*      Write out the document.                                         */
  /* -------------------------------------------------------------------- */

  if (msIO_needBinaryStdout() == MS_FAILURE)
    return MS_FAILURE;

  msIO_setHeader("Content-Type", "text/xml; charset=UTF-8");
  msIO_sendHeaders();

  context = msIO_getHandler(stdout);

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, ("UTF-8"), 1);
  msIO_contextWrite(context, buffer, size);
  xmlFree(buffer);

  /*free buffer and the document */
  /*xmlFree(buffer);*/
  xmlFreeDoc(psDoc);

  xmlCleanupParser();

  return (MS_SUCCESS);
}

/************************************************************************/
/*                     msWFSDescribeStoredQueries20                     */
/************************************************************************/

int msWFSDescribeStoredQueries20(mapObj *map, wfsParamsObj *params,
                                 owsRequestObj *ows_request) {
  xmlDocPtr psDoc;
  xmlChar *buffer = NULL;
  int size = 0;
  msIOContext *context = NULL;
  xmlNodePtr psRootNode;
  char *xsi_schemaLocation = NULL;
  int i, j;
  int nStoredQueries = 0;
  char **storedQueries = NULL;

  xmlDocPtr psStoredQueryDoc;
  xmlNodePtr psStoredQueryRoot;

  if (params->pszStoredQueryId != NULL) {
    storedQueries =
        msStringSplit(params->pszStoredQueryId, ',', &nStoredQueries);
    for (i = 0; i < nStoredQueries; i++) {
      char *query = msWFSGetStoredQuery(map, storedQueries[i]);
      if (query == NULL) {
        msSetError(MS_WFSERR, "Unknown stored query id: %s",
                   "msWFSDescribeStoredQueries20()", storedQueries[i]);
        msFreeCharArray(storedQueries, nStoredQueries);
        return msWFSException(map, "storedqueryid",
                              MS_OWS_ERROR_INVALID_PARAMETER_VALUE,
                              params->pszVersion);
      }
      msFree(query);
    }
  } else {
    storedQueries = msWFSGetStoredQueries(map, &nStoredQueries);
  }

  /* -------------------------------------------------------------------- */
  /*      Create document.                                                */
  /* -------------------------------------------------------------------- */
  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = xmlNewNode(NULL, BAD_CAST "DescribeStoredQueriesResponse");

  xmlDocSetRootElement(psDoc, psRootNode);

  /* -------------------------------------------------------------------- */
  /*      Name spaces                                                     */
  /* -------------------------------------------------------------------- */

  /*default name space*/
  xmlNewProp(psRootNode, BAD_CAST "xmlns",
             BAD_CAST MS_OWSCOMMON_WFS_20_NAMESPACE_URI);

  xmlSetNs(psRootNode,
           xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_WFS_20_NAMESPACE_URI,
                    BAD_CAST MS_OWSCOMMON_WFS_NAMESPACE_PREFIX));

  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI,
           BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);

  /*schema*/
  xsi_schemaLocation = msStrdup(MS_OWSCOMMON_WFS_20_NAMESPACE_URI);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation =
      msStringConcatenate(xsi_schemaLocation, msOWSGetSchemasLocation(map));
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation,
                                           MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION);

  xmlNewNsProp(psRootNode, NULL, BAD_CAST "xsi:schemaLocation",
               BAD_CAST xsi_schemaLocation);
  free(xsi_schemaLocation);

  /* -------------------------------------------------------------------- */
  /*      Add queries                                                     */
  /* -------------------------------------------------------------------- */

  for (i = 0; i < nStoredQueries; i++) {
    char *query = msWFSGetStoredQuery(map, storedQueries[i]);
    if (query != NULL) {
      xmlNodePtr pChild;
      xmlNs *ns;
      xmlNodePtr psStoredQuery;

      psStoredQueryDoc = xmlParseDoc((const xmlChar *)query);
      if (psStoredQueryDoc == NULL) {
        char szMsg[256];
        msFree(query);
        snprintf(szMsg, sizeof(szMsg),
                 "WARNING: Definition for stored query %s is invalid",
                 storedQueries[i]);
        xmlAddChild(psRootNode, xmlNewComment(BAD_CAST szMsg));
        continue;
      }

      psStoredQueryRoot = xmlDocGetRootElement(psStoredQueryDoc);

      psStoredQuery = xmlNewNode(NULL, BAD_CAST "StoredQueryDescription");
      xmlNewProp(psStoredQuery, BAD_CAST "id", BAD_CAST storedQueries[i]);
      xmlAddChild(psRootNode, psStoredQuery);

      ns = psStoredQueryRoot->nsDef;
      while (ns != NULL) {
        xmlNewNs(psStoredQuery, BAD_CAST ns->href, BAD_CAST ns->prefix);
        ns = ns->next;
      }

      pChild = psStoredQueryRoot->children;
      while (pChild != NULL) {
        xmlNodePtr pNext = pChild->next;

        if (pChild->type == XML_ELEMENT_NODE &&
            strcmp((const char *)pChild->name, "QueryExpressionText") == 0) {
          if (strcasecmp(storedQueries[i], URN_GET_FEATURE_BY_ID) == 0) {
            char **arrayNsPrefix =
                (char **)malloc(sizeof(char *) * map->numlayers);
            char **arrayNsUri =
                (char **)malloc(sizeof(char *) * map->numlayers);
            int arraysize = 0;
            int k;
            char *returnFeatureTypes = NULL;
            xmlNodePtr psQueryExpressionText;

            psQueryExpressionText =
                xmlNewNode(NULL, BAD_CAST "QueryExpressionText");
            xmlAddChild(psStoredQuery, psQueryExpressionText);
            xmlNewProp(psQueryExpressionText, BAD_CAST "isPrivate",
                       BAD_CAST "true");
            xmlNewProp(
                psQueryExpressionText, BAD_CAST "language",
                BAD_CAST
                "urn:ogc:def:queryLanguage:OGC-WFS::WFS_QueryExpression");

            for (j = 0; j < map->numlayers; j++) {
              layerObj *lp;
              const char *user_namespace_prefix = MS_DEFAULT_NAMESPACE_PREFIX;
              const char *user_namespace_uri = MS_DEFAULT_NAMESPACE_URI;
              const char *value;
              char szValue[256];

              lp = GET_LAYER(map, j);

              if (!msIntegerInArray(lp->index, ows_request->enabled_layers,
                                    ows_request->numlayers) ||
                  !msWFSIsLayerSupported(lp))
                continue;

              value = msOWSLookupMetadata(&(map->web.metadata), "FO",
                                          "namespace_uri");
              if (value)
                user_namespace_uri = value;

              value = msOWSLookupMetadata(&(map->web.metadata), "FO",
                                          "namespace_prefix");
              if (value)
                user_namespace_prefix = value;

              for (k = 0; k < arraysize; k++) {
                if (strcmp(arrayNsPrefix[k], user_namespace_prefix) == 0)
                  break;
              }
              if (k == arraysize) {
                arrayNsPrefix[arraysize] = msStrdup(user_namespace_prefix);
                arrayNsUri[arraysize] = msStrdup(user_namespace_uri);
                arraysize++;

                xmlNewNs(psQueryExpressionText, BAD_CAST user_namespace_uri,
                         BAD_CAST user_namespace_prefix);
              }

              if (returnFeatureTypes != NULL)
                returnFeatureTypes =
                    msStringConcatenate(returnFeatureTypes, " ");
              snprintf(szValue, sizeof(szValue), "%s:%s", user_namespace_prefix,
                       lp->name);
              returnFeatureTypes =
                  msStringConcatenate(returnFeatureTypes, szValue);
            }

            xmlNewProp(psQueryExpressionText, BAD_CAST "returnFeatureTypes",
                       BAD_CAST returnFeatureTypes);

            msFree(returnFeatureTypes);
            msFreeCharArray(arrayNsPrefix, arraysize);
            msFreeCharArray(arrayNsUri, arraysize);
          } else {
            xmlChar *isPrivate = xmlGetProp(pChild, BAD_CAST "isPrivate");
            if (isPrivate != NULL &&
                strcmp((const char *)isPrivate, "true") == 0) {
              xmlNodePtr pSubChild = xmlFirstElementChild(pChild);
              xmlUnlinkNode(pSubChild);
              xmlFreeNode(pSubChild);
            }
            xmlUnlinkNode(pChild);
            xmlAddChild(psStoredQuery, pChild);
            msFree(isPrivate);
          }
        } else {
          xmlUnlinkNode(pChild);
          xmlAddChild(psStoredQuery, pChild);
        }
        pChild = pNext;
      }

      xmlReconciliateNs(psDoc, psStoredQuery);
      xmlFreeDoc(psStoredQueryDoc);
      msFree(query);
    } else {
      char szMsg[256];
      snprintf(szMsg, sizeof(szMsg),
               "WARNING: Definition for stored query %s missing",
               storedQueries[i]);
      xmlAddChild(psRootNode, xmlNewComment(BAD_CAST szMsg));
    }
  }
  msFreeCharArray(storedQueries, nStoredQueries);

  /* -------------------------------------------------------------------- */
  /*      Write out the document.                                         */
  /* -------------------------------------------------------------------- */

  if (msIO_needBinaryStdout() == MS_FAILURE)
    return MS_FAILURE;

  msIO_setHeader("Content-Type", "text/xml; charset=UTF-8");
  msIO_sendHeaders();

  context = msIO_getHandler(stdout);

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, ("UTF-8"), 1);
  msIO_contextWrite(context, buffer, size);
  xmlFree(buffer);

  /*free buffer and the document */
  /*xmlFree(buffer);*/
  xmlFreeDoc(psDoc);

  xmlCleanupParser();

  return (MS_SUCCESS);
}

#endif /* defined(USE_WFS_SVR) && defined(USE_LIBXML2) */

#if defined(USE_WFS_SVR) && !defined(USE_LIBXML2)

int msWFSException20(mapObj *map, const char *locator,
                     const char *exceptionCode) {
  /* fallback to reporting using 1.0 style exceptions. */
  return msWFSException(map, locator, exceptionCode, "1.0.0");
}

int msWFSGetCapabilities20(mapObj *map, wfsParamsObj *params,
                           cgiRequestObj *req, owsRequestObj *ows_request)

{
  msSetError(MS_WFSERR,
             "WFS 2.0 request made, but mapserver requires libxml2 for WFS 2.0 "
             "services and this is not configured.",
             "msWFSGetCapabilities20()");

  return msWFSException11(map, "mapserv", MS_OWS_ERROR_NO_APPLICABLE_CODE,
                          params->pszVersion);
}

int msWFSListStoredQueries20(mapObj *map, wfsParamsObj *params,
                             cgiRequestObj *req, owsRequestObj *ows_request) {
  msSetError(MS_WFSERR,
             "WFS 2.0 request made, but mapserver requires libxml2 for WFS 2.0 "
             "services and this is not configured.",
             "msWFSListStoredQueries20()");

  return msWFSException11(map, "mapserv", MS_OWS_ERROR_NO_APPLICABLE_CODE,
                          params->pszVersion);
}

int msWFSDescribeStoredQueries20(mapObj *map, wfsParamsObj *params,
                                 cgiRequestObj *req,
                                 owsRequestObj *ows_request) {
  msSetError(MS_WFSERR,
             "WFS 2.0 request made, but mapserver requires libxml2 for WFS 2.0 "
             "services and this is not configured.",
             "msWFSDescribeStoredQueries20()");

  return msWFSException11(map, "mapserv", MS_OWS_ERROR_NO_APPLICABLE_CODE,
                          params->pszVersion);
}

char *msWFSGetResolvedStoredQuery20(mapObj *map, wfsParamsObj *params,
                                    const char *id, hashTableObj *hashTable) {
  msSetError(MS_WFSERR,
             "WFS 2.0 request made, but mapserver requires libxml2 for WFS 2.0 "
             "services and this is not configured.",
             "msWFSGetResolvedStoredQuery20()");

  msWFSException11(map, "mapserv", MS_OWS_ERROR_NO_APPLICABLE_CODE,
                   params->pszVersion);
  return NULL;
}

#endif /* defined(USE_WFS_SVR) && !defined(USE_LIBXML2) */

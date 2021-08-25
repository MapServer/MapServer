/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC OWS Common Implementation for use by MapServer OGC code
 *           versions:
 *           1.0.0 (OGC Document 05-008c1)
 *           1.1.0 (OGC document 06-121r3)
 *
 * Author:   Tom Kralidis (tomkralidis@gmail.com)
 *
 ******************************************************************************
 * Copyright (c) 2006, Tom Kralidis
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
#include "mapows.h"

#ifdef USE_LIBXML2

#include<libxml/parser.h>
#include<libxml/tree.h>

#include "mapowscommon.h"
#include "maplibxml2.h"





/**
 * msOWSCommonServiceIdentification()
 *
 * returns an object of ServiceIdentification as per:
 *
 * 1.0.0 subclause 7.4.3
 * 1.1.1 subclause 7.4.4
 *
 * @param map mapObj used to fetch WEB/METADATA
 * @param servicetype the OWS type
 * @param supported_versions the supported version(s) of the OWS
 *
 * @return psRootNode xmlNodePtr of XML construct
 *
 */

xmlNodePtr msOWSCommonServiceIdentification(xmlNsPtr psNsOws, mapObj *map,
                                            const char *servicetype,
                                            const char *supported_versions,
                                            const char *namespaces,
                                            const char *validated_language)
{
  const char *value    = NULL;

  xmlNodePtr   psRootNode = NULL;
  xmlNodePtr   psNode     = NULL;

  if (_validateNamespace(psNsOws) == MS_FAILURE)
    psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  /* create element name */
  psRootNode = xmlNewNode(psNsOws, BAD_CAST "ServiceIdentification");

  /* add child elements */

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "title", validated_language);

  psNode = xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "Title", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_title\" missing for ows:Title"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "abstract", validated_language);

  psNode = xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "Abstract", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_abstract\" was missing for ows:Abstract"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "keywordlist", validated_language);

  if (value) {
    psNode = xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "Keywords", NULL);
    msLibXml2GenerateList(psNode, psNsOws, "Keyword", value, ',');
  }

  else {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_keywordlist\" was missing for ows:KeywordList"));
  }

  psNode = xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "ServiceType", BAD_CAST servicetype);

  xmlNewProp(psNode, BAD_CAST "codeSpace", BAD_CAST MS_OWSCOMMON_OGC_CODESPACE);

  msLibXml2GenerateList(psRootNode, psNsOws, "ServiceTypeVersion", supported_versions, ',');

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "fees", validated_language);

  psNode = xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "Fees", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_fees\" was missing for ows:Fees"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "accessconstraints", validated_language);

  psNode = xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "AccessConstraints", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_accessconstraints\" was missing for ows:AccessConstraints"));
  }

  return psRootNode;
}

/**
 * msOWSCommonServiceProvider()
 *
 * returns an object of ServiceProvider as per:
 *
 *
 * 1.0.0 subclause 7.4.4
 * 1.1.0 subclause 7.4.5
 *
 * @param map mapObj to fetch MAP/WEB/METADATA
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonServiceProvider(xmlNsPtr psNsOws, xmlNsPtr psNsXLink,
                                      mapObj *map, const char *namespaces,
                                      const char *validated_language)
{
  const char *value = NULL;

  xmlNodePtr   psNode          = NULL;
  xmlNodePtr   psRootNode      = NULL;
  xmlNodePtr   psSubNode       = NULL;
  xmlNodePtr   psSubSubNode    = NULL;
  xmlNodePtr   psSubSubSubNode = NULL;

  if (_validateNamespace(psNsOws) == MS_FAILURE)
    psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  psRootNode = xmlNewNode(psNsOws, BAD_CAST "ServiceProvider");

  /* add child elements */

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "contactorganization", validated_language);

  psNode = xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "ProviderName", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Mandatory metadata \"ows_contactorganization\" was missing for ows:ProviderName"));
  }

  psNode = xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "ProviderSite", NULL);

  xmlNewNsProp(psNode, psNsXLink, BAD_CAST "type", BAD_CAST "simple");

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "service_onlineresource", validated_language);

  xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_service_onlineresource\" was missing for ows:ProviderSite/@xlink:href"));
  }

  psNode = xmlNewTextChild(psRootNode, psNsOws, BAD_CAST "ServiceContact", NULL);

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "contactperson", validated_language);

  psSubNode = xmlNewTextChild(psNode, psNsOws, BAD_CAST "IndividualName", BAD_CAST  value);

  if (!value) {
    xmlAddSibling(psSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactperson\" was missing for ows:IndividualName"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "contactposition", validated_language);

  psSubNode = xmlNewTextChild(psNode, psNsOws, BAD_CAST "PositionName", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactposition\" was missing for ows:PositionName"));
  }

  psSubNode = xmlNewTextChild(psNode, psNsOws, BAD_CAST "ContactInfo", NULL);

  psSubSubNode = xmlNewTextChild(psSubNode, psNsOws, BAD_CAST "Phone", NULL);

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "contactvoicetelephone", validated_language);

  psSubSubSubNode = xmlNewTextChild(psSubSubNode, psNsOws, BAD_CAST "Voice", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactvoicetelephone\" was missing for ows:Voice"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "contactfacsimiletelephone", validated_language);

  psSubSubSubNode = xmlNewTextChild(psSubSubNode, psNsOws, BAD_CAST "Facsimile", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactfacsimiletelephone\" was missing for ows:Facsimile"));
  }

  psSubSubNode = xmlNewTextChild(psSubNode, psNsOws, BAD_CAST "Address", NULL);

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "address", validated_language);

  psSubSubSubNode = xmlNewTextChild(psSubSubNode, psNsOws, BAD_CAST "DeliveryPoint", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_address\" was missing for ows:DeliveryPoint"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "city", validated_language);

  psSubSubSubNode = xmlNewTextChild(psSubSubNode, psNsOws, BAD_CAST "City", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_city\" was missing for ows:City"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "stateorprovince", validated_language);

  psSubSubSubNode = xmlNewTextChild(psSubSubNode, psNsOws, BAD_CAST "AdministrativeArea", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_stateorprovince\" was missing for ows:AdministrativeArea"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "postcode", validated_language);

  psSubSubSubNode = xmlNewTextChild(psSubSubNode, psNsOws, BAD_CAST "PostalCode", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_postcode\" was missing for ows:PostalCode"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "country", validated_language);

  psSubSubSubNode = xmlNewTextChild(psSubSubNode, psNsOws, BAD_CAST "Country", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_country\" was missing for ows:Country"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "contactelectronicmailaddress", validated_language);

  psSubSubSubNode = xmlNewTextChild(psSubSubNode, psNsOws, BAD_CAST "ElectronicMailAddress", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactelectronicmailaddress\" was missing for ows:ElectronicMailAddress"));
  }

  psSubSubNode = xmlNewTextChild(psSubNode, psNsOws, BAD_CAST "OnlineResource", NULL);

  xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "type", BAD_CAST "simple");

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "service_onlineresource", validated_language);

  xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "href", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_service_onlineresource\" was missing for ows:OnlineResource/@xlink:href"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "hoursofservice", validated_language);

  psSubSubNode = xmlNewTextChild(psSubNode, psNsOws, BAD_CAST "HoursOfService", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_hoursofservice\" was missing for ows:HoursOfService"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "contactinstructions", validated_language);

  psSubSubNode = xmlNewTextChild(psSubNode, psNsOws, BAD_CAST "ContactInstructions", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactinstructions\" was missing for ows:ContactInstructions"));
  }

  value = msOWSLookupMetadataWithLanguage(&(map->web.metadata), namespaces, "role", validated_language);

  psSubNode = xmlNewTextChild(psNode, psNsOws, BAD_CAST "Role", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_role\" was missing for ows:Role"));
  }

  return psRootNode;

}

/**
 * msOWSCommonOperationsMetadata()
 *
 * returns the root element of OperationsMetadata as per:
 *
 * 1.0.0 subclause 7.4.5
 * 1.1.0 subclause 7.4.6
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonOperationsMetadata(xmlNsPtr psNsOws)
{
  xmlNodePtr psRootNode = NULL;

  if (_validateNamespace(psNsOws) == MS_FAILURE)
    psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  psRootNode = xmlNewNode(psNsOws, BAD_CAST "OperationsMetadata");
  return psRootNode;
}

/**
 * msOWSCommonOperationsMetadataOperation()
 *
 * returns an Operation element of OperationsMetadata as per:
 *
 * 1.0.0 subclause 7.4.5
 * 1.1.0 subclause 7.4.6
 *
 * @param name name of the Operation
 * @param method HTTP method: OWS_METHOD_GET, OWS_METHOD_POST or OWS_METHOD_GETPOST)
 * @param url online resource URL
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 */

xmlNodePtr msOWSCommonOperationsMetadataOperation(xmlNsPtr psNsOws, xmlNsPtr psXLinkNs, const char *name, int method, const char *url)
{
  xmlNodePtr psRootNode      = NULL;
  xmlNodePtr psNode          = NULL;
  xmlNodePtr psSubNode       = NULL;
  xmlNodePtr psSubSubNode    = NULL;

  if (_validateNamespace(psNsOws) == MS_FAILURE)
    psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);


  psRootNode = xmlNewNode(psNsOws, BAD_CAST "Operation");

  xmlNewProp(psRootNode, BAD_CAST "name", BAD_CAST name);

  psNode = xmlNewChild(psRootNode, psNsOws, BAD_CAST "DCP", NULL);

  psSubNode = xmlNewChild(psNode, psNsOws, BAD_CAST "HTTP", NULL);

  if (method  == OWS_METHOD_GET || method == OWS_METHOD_GETPOST ) {
    psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "Get", NULL);
    xmlNewNsProp(psSubSubNode, psXLinkNs, BAD_CAST "type", BAD_CAST "simple");
    xmlNewNsProp(psSubSubNode, psXLinkNs, BAD_CAST "href", BAD_CAST url);
  }

  if (method == OWS_METHOD_POST || method == OWS_METHOD_GETPOST ) {
    psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "Post", NULL);
    xmlNewNsProp(psSubSubNode, psXLinkNs, BAD_CAST "type", BAD_CAST "simple");
    xmlNewNsProp(psSubSubNode, psXLinkNs, BAD_CAST "href", BAD_CAST url);
  }

  return psRootNode;
}

/**
 * msOWSCommonOperationsMetadataDomainType()
 *
 * returns a Parameter or Constraint element (which are of type ows:DomainType)
 * of OperationsMetadata as per:
 *
 * 1.0.0 subclause 7.4.5
 * 1.1.0 subclause 7.4.6
 *
 * @param version the integerized x.y.z version of OWS Common to use
 * @param elname name of the element (Parameter | Constraint)
 * @param name name of the Parameter
 * @param values list of values (comma separated list) or NULL if none
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonOperationsMetadataDomainType(int version, xmlNsPtr psNsOws, const char *elname, const char *name, const char *values)
{
  xmlNodePtr psRootNode = NULL;
  xmlNodePtr psNode = NULL;

  if (_validateNamespace(psNsOws) == MS_FAILURE)
    psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  psRootNode = xmlNewNode(psNsOws, BAD_CAST elname);

  xmlNewProp(psRootNode, BAD_CAST "name", BAD_CAST name);

  if (version == OWS_1_0_0) {
    msLibXml2GenerateList(psRootNode, psNsOws, "Value", values, ',');
  }
  if (version == OWS_1_1_0 || version == OWS_2_0_0) {
    psNode = xmlNewChild(psRootNode, psNsOws, BAD_CAST "AllowedValues", NULL);
    msLibXml2GenerateList(psNode, psNsOws, "Value", values, ',');
  }

  return psRootNode;
}

/**
 * msOWSCommonExceptionReport()
 *
 * returns an object of ExceptionReport as per clause 8
 *
 * @param ows_version the version of OWS Common to use
 * @param schemas_location URL to OGC Schemas Location base
 * @param version the version of the calling specification
 * @param language ISO3166 code of language
 * @param exceptionCode a code from the calling specification's list of exceptions, or from OWS Common
 * @param locator where the exception was encountered (i.e. "layers" keyword)
 * @param ExceptionText the actual error message
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonExceptionReport(xmlNsPtr psNsOws, int ows_version, const char *schemas_location, const char *version, const char *language, const char *exceptionCode, const char *locator, const char *ExceptionText)
{
  char *xsi_schemaLocation = NULL;
  char szVersionBuf[OWS_VERSION_MAXLEN];

  xmlNsPtr     psNsXsi     = NULL;
  xmlNodePtr   psRootNode  = NULL;
  xmlNodePtr   psMainNode  = NULL;

  psRootNode = xmlNewNode(psNsOws, BAD_CAST "ExceptionReport");

  psNsXsi = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);

  /* add attributes to root element */
  xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST version);

  if (ows_version == OWS_1_0_0) {
    xmlNewProp(psRootNode, BAD_CAST "language", BAD_CAST language);
  }
  if (ows_version == OWS_1_1_0) {
    xmlNewProp(psRootNode, BAD_CAST "xml:lang", BAD_CAST language);
  }

  xsi_schemaLocation = msStrdup((char *)psNsOws->href);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, (char *)schemas_location);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/ows/");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, (char *)msOWSGetVersionString(ows_version, szVersionBuf));
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/owsExceptionReport.xsd");

  /* add namespace'd attributes to root element */
  xmlNewNsProp(psRootNode, psNsXsi, BAD_CAST "schemaLocation", BAD_CAST xsi_schemaLocation);

  /* add child element */
  psMainNode = xmlNewChild(psRootNode, NULL, BAD_CAST "Exception", NULL);

  /* add attributes to child */
  xmlNewProp(psMainNode, BAD_CAST "exceptionCode", BAD_CAST exceptionCode);

  if (locator != NULL) {
    xmlNewProp(psMainNode, BAD_CAST "locator", BAD_CAST locator);
  }

  if (ExceptionText != NULL) {
    xmlNewTextChild(psMainNode, NULL, BAD_CAST "ExceptionText", BAD_CAST ExceptionText);
  }

  free(xsi_schemaLocation);
  return psRootNode;
}

/**
 * msOWSCommonBoundingBox()
 *
 * returns an object of BoundingBox as per subclause 10.2.1
 *
 * If necessary (ie. an EPSG URN GCS such as 4326) the tuple axes will be
 * reoriented to match the EPSG coordinate system expectations.
 *
 * @param psNsOws OWS namespace object
 * @param crs the CRS / EPSG code
 * @param dimensions number of dimensions of the coordinates
 * @param minx minx
 * @param miny miny
 * @param maxx maxx
 * @param maxy maxy
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 */

xmlNodePtr msOWSCommonBoundingBox(xmlNsPtr psNsOws, const char *crs, int dimensions, double minx, double miny, double maxx, double maxy)
{
  char LowerCorner[100];
  char UpperCorner[100];
  char dim_string[100];
  xmlNodePtr psRootNode = NULL;

  /* Do we need to reorient tuple axes? */
  if(crs && strstr(crs, "imageCRS") == NULL) {
    projectionObj proj;

    msInitProjection( &proj );
    if( msLoadProjectionString( &proj, (char *) crs ) == 0 ) {
      msAxisNormalizePoints( &proj, 1, &minx, &miny );
      msAxisNormalizePoints( &proj, 1, &maxx, &maxy );
    }
    msFreeProjection( &proj );
  }


  if (_validateNamespace(psNsOws) == MS_FAILURE)
    psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  /* create element name */
  psRootNode = xmlNewNode(psNsOws, BAD_CAST "BoundingBox");

  /* add attributes to the root element */
  xmlNewProp(psRootNode, BAD_CAST "crs", BAD_CAST crs);

  snprintf( dim_string, sizeof(dim_string), "%d", dimensions );
  xmlNewProp(psRootNode, BAD_CAST "dimensions", BAD_CAST dim_string);

  snprintf(LowerCorner, sizeof(LowerCorner), "%.15g %.15g", minx, miny);
  snprintf(UpperCorner, sizeof(UpperCorner), "%.15g %.15g", maxx, maxy);

  /* add child elements */
  xmlNewChild(psRootNode, psNsOws,BAD_CAST "LowerCorner",BAD_CAST LowerCorner);
  xmlNewChild(psRootNode, psNsOws,BAD_CAST "UpperCorner",BAD_CAST UpperCorner);

  return psRootNode;
}

/**
 * msOWSCommonWGS84BoundingBox()
 *
 * returns an object of WGS84BoundingBox as per subclause 10.2.2
 *
 * @param psNsOws OWS namespace object
 * @param dimensions number of dimensions of the coordinates
 * @param minx minx
 * @param miny miny
 * @param maxx maxx
 * @param maxy maxy
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 */

xmlNodePtr msOWSCommonWGS84BoundingBox(xmlNsPtr psNsOws, int dimensions, double minx, double miny, double maxx, double maxy)
{
  char LowerCorner[100];
  char UpperCorner[100];
  char dim_string[100];

  xmlNodePtr psRootNode = NULL;

  if (_validateNamespace(psNsOws) == MS_FAILURE)
    psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  /* create element name */
  psRootNode = xmlNewNode(psNsOws, BAD_CAST "WGS84BoundingBox");

  snprintf( dim_string, sizeof(dim_string), "%d", dimensions );
  xmlNewProp(psRootNode, BAD_CAST "dimensions", BAD_CAST dim_string);

  snprintf(LowerCorner, sizeof(LowerCorner), "%.6f %.6f", minx, miny);
  snprintf(UpperCorner, sizeof(UpperCorner), "%.6f %.6f", maxx, maxy);

  /* add child elements */
  xmlNewChild(psRootNode, psNsOws,BAD_CAST "LowerCorner",BAD_CAST LowerCorner);
  xmlNewChild(psRootNode, psNsOws,BAD_CAST "UpperCorner",BAD_CAST UpperCorner);

  return psRootNode;
}

/**
 * _validateNamespace()
 *
 * validates the namespace passed to this module's functions
 *
 * @param psNsOws namespace object
 *
 * @return MS_SUCCESS or MS_FAILURE
 *
 */

int _validateNamespace(xmlNsPtr psNsOws)
{
  char namespace_prefix[10];
  snprintf(namespace_prefix, sizeof(namespace_prefix), "%s", psNsOws->prefix);
  if (strcmp(namespace_prefix, MS_OWSCOMMON_OWS_NAMESPACE_PREFIX) == 0)
    return MS_SUCCESS;
  else
    return MS_FAILURE;
}

/*
 * Valid an xml string against an XML schema
 * Inpired from: http://xml.developpez.com/sources/?page=validation#validate_XSD_CppCLI_2
 * taken from tinyows.org
 */
int msOWSSchemaValidation(const char* xml_schema, const char* xml)
{
  xmlSchemaPtr schema;
  xmlSchemaParserCtxtPtr ctxt;
  xmlSchemaValidCtxtPtr validctxt;
  int ret;
  xmlDocPtr doc;

  if (!xml_schema || !xml)
    return MS_FAILURE;

  xmlInitParser();
  schema = NULL;
  ret = -1;

  /* To valide WFS 2.0 requests, we might need to explicitely import */
  /* GML and FES 2.0 */
  if( strlen(xml_schema) > strlen(MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION) &&
      strcmp(xml_schema + strlen(xml_schema) -
            strlen(MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION), MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION) == 0 )
  {
    char szInMemSchema[2048];
    char szBaseLocation[256];

    strncpy(szBaseLocation, xml_schema, strlen(xml_schema) - strlen(MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION));
    szBaseLocation[strlen(xml_schema) - strlen(MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION)] = '\0';

    strcpy(szInMemSchema, "<schema elementFormDefault=\"qualified\" version=\"1.0.0\" "
                          "xmlns=\"http://www.w3.org/2001/XMLSchema\">\n");

    sprintf(szInMemSchema + strlen(szInMemSchema),
            "<import namespace=\"%s\" schemaLocation=\"%s\" />\n",
            MS_OWSCOMMON_WFS_20_NAMESPACE_URI, xml_schema);

    if( strstr(xml, MS_OWSCOMMON_FES_20_NAMESPACE_URI) != NULL )
    {
        sprintf(szInMemSchema + strlen(szInMemSchema),
                "<import namespace=\"%s\" schemaLocation=\"%s%s\" />\n",
                MS_OWSCOMMON_FES_20_NAMESPACE_URI, szBaseLocation, MS_OWSCOMMON_FES_20_SCHEMA_LOCATION);
    }

    if( strstr(xml, MS_OWSCOMMON_GML_32_NAMESPACE_URI) != NULL )
    {
        sprintf(szInMemSchema + strlen(szInMemSchema),
                "<import namespace=\"%s\" schemaLocation=\"%s%s\" />\n",
                MS_OWSCOMMON_GML_32_NAMESPACE_URI, szBaseLocation, MS_OWSCOMMON_GML_321_SCHEMA_LOCATION);
    }
    else if( strstr(xml, MS_OWSCOMMON_GML_NAMESPACE_URI) != NULL )
    {
        if( strstr(xml, MS_OWSCOMMON_GML_212_SCHEMA_LOCATION) != NULL )
        {
            sprintf(szInMemSchema + strlen(szInMemSchema),
                    "<import namespace=\"%s\" schemaLocation=\"%s%s\" />\n",
                    MS_OWSCOMMON_GML_NAMESPACE_URI, szBaseLocation, MS_OWSCOMMON_GML_212_SCHEMA_LOCATION);
        }
        else if( strstr(xml, MS_OWSCOMMON_GML_311_SCHEMA_LOCATION) != NULL )
        {
            sprintf(szInMemSchema + strlen(szInMemSchema),
                    "<import namespace=\"%s\" schemaLocation=\"%s%s\" />\n",
                    MS_OWSCOMMON_GML_NAMESPACE_URI, szBaseLocation, MS_OWSCOMMON_GML_311_SCHEMA_LOCATION);
        }
    }

    strcat(szInMemSchema, "</schema>\n");
    /*fprintf(stderr, "%s\n", szInMemSchema);*/

    ctxt = xmlSchemaNewMemParserCtxt(szInMemSchema, strlen(szInMemSchema));
  }
  else
  {
    /* Open XML Schema File */
    ctxt = xmlSchemaNewParserCtxt(xml_schema);
  }

  /*
  xmlSchemaSetParserErrors(ctxt,
                           (xmlSchemaValidityErrorFunc) libxml2_callback,
                           (xmlSchemaValidityWarningFunc) libxml2_callback, stderr);
  */

  schema = xmlSchemaParse(ctxt);
  xmlSchemaFreeParserCtxt(ctxt);

  /* If XML Schema hasn't been rightly loaded */
  if (schema == NULL) {
    xmlSchemaCleanupTypes();
    xmlMemoryDump();
    xmlCleanupParser();
    return ret;
  }

  doc = xmlParseDoc((xmlChar *)xml);

  if (doc != NULL) {
    /* Loading XML Schema content */
    validctxt = xmlSchemaNewValidCtxt(schema);
    /*
    xmlSchemaSetValidErrors(validctxt,
                            (xmlSchemaValidityErrorFunc) libxml2_callback,
                            (xmlSchemaValidityWarningFunc) libxml2_callback, stderr);
    */
    /* validation */
    ret = xmlSchemaValidateDoc(validctxt, doc);
    xmlSchemaFreeValidCtxt(validctxt);
  }

  xmlSchemaFree(schema);
  xmlFreeDoc(doc);
  xmlCleanupParser();

  return ret;
}

#endif /* defined(USE_LIBXML2) */


/**
 * msOWSCommonNegotiateVersion()
 *
 * returns a supported version as per subclause 7.3.2
 *
 * @param requested_version the version passed by the client
 * @param supported_versions an array of supported versions
 * @param num_supported_versions size of supported_versions
 *
 * @return supported version integer, or -1 on error
 *
 */

int msOWSCommonNegotiateVersion(int requested_version, const int supported_versions[], int num_supported_versions)
{
  int i;

  /* if version is not set return error */
  if (! requested_version)
    return -1;

  /* return the first entry that's equal to the requested version */
  for (i = 0; i < num_supported_versions; i++) {
    if (supported_versions[i] == requested_version)
      return supported_versions[i];
  }

  /* no match; calling code should throw an exception */
  return -1;
}

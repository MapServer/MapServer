/******************************************************************************
 *
 * Name:     mapowscommon.c
 * Project:  MapServer
 * Purpose:  OGC OWS Common Implementation for use by MapServer OGC code
 *           
 * Author:   Tom Kralidis (tomkralidis@hotmail.com)
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

#ifdef USE_SOS_SVR

#include<libxml/parser.h>
#include<libxml/tree.h>

#include "mapowscommon.h"

MS_CVSID("$Id$")

/**
 * msOWSCommonServiceIdentification()
 *
 * returns an object of ServiceIdentification as per subclause 7.4.3
 *
 * @param map mapObj used to fetch WEB/METADATA
 * @param servicetype the OWS type
 * @param version the version of the OWS
 *
 * @return psRootNode xmlNodePtr of XML construct
 *
 */

xmlNodePtr msOWSCommonServiceIdentification(mapObj *map, const char *servicetype, const char *version) {
  const char *value    = NULL;

  xmlNsPtr     psNs       = NULL;
  xmlNodePtr   psRootNode = NULL;
  xmlNodePtr   psNode     = NULL;
  xmlNodePtr   psSubNode  = NULL;

  psNs = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  /* create element name */
  psRootNode = xmlNewNode(xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX), BAD_CAST "ServiceIdentification");

  /* add child elements */

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "title");

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "Title", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_title\" missing for ows:Title"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "abstract");

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "Abstract", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_abstract\" was missing for ows:Abstract"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "keywordlist");

  if (value) {
    char **tokens = NULL;
    int n = 0;
    int i = 0;

    psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "Keywords", NULL);

    tokens = msStringSplit(value, ',', &n);
    if (tokens && n > 0) {
      for (i=0; i<n; i++) {
        psSubNode = xmlNewChild(psNode, NULL, BAD_CAST "Keyword", BAD_CAST tokens[i]);
        xmlSetNs(psSubNode, psNs);
      }
      msFreeCharArray(tokens, n);
    }
  }

  else {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_keywordlist\" was missing for ows:KeywordList"));
  }

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "ServiceType", BAD_CAST servicetype);
  
  xmlNewProp(psNode, BAD_CAST "codeSpace", BAD_CAST MS_OWSCOMMON_OGC_CODESPACE);

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "ServiceTypeVersion", BAD_CAST version);

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "fees");

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "Fees", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_fees\" was missing for ows:Fees"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "accessconstraints");

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "AccessConstraints", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_accessconstraints\" was missing for ows:AccessConstraints"));
  }

  return psRootNode;
}

/**
 * msOWSCommonServiceProvider()
 *
 * returns an object of ServiceProvider as per subclause 7.4.4
 *
 * @param map mapObj to fetch MAP/WEB/METADATA
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonServiceProvider(mapObj *map) {
  const char *value = NULL;

  xmlNsPtr     psNsXLink       = NULL;
  xmlNsPtr     psNsOws         = NULL;
  xmlNodePtr   psNode          = NULL;
  xmlNodePtr   psRootNode      = NULL;
  xmlNodePtr   psSubNode       = NULL;
  xmlNodePtr   psSubSubNode    = NULL;
  xmlNodePtr   psSubSubSubNode = NULL;

  psNsXLink = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);

  psNsOws = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  psRootNode = xmlNewNode(psNsOws, BAD_CAST "ServiceProvider");

  /* add child elements */

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "contactorganization");

  psNode = xmlNewChild(psRootNode, psNsOws, BAD_CAST "ProviderName", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Mandatory metadata \"ows_contactorganization\" was missing for ows:ProviderName"));
  }

  psNode = xmlNewChild(psRootNode, psNsOws, BAD_CAST "ProviderSite", NULL);

  xmlNewNsProp(psNode, psNsXLink, BAD_CAST "type", BAD_CAST "simple");

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "service_onlineresource");

  xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_service_onlineresource\" was missing for ows:ProviderSite/@xlink:href"));
  }

  psNode = xmlNewChild(psRootNode, psNsOws, BAD_CAST "ServiceContact", NULL);

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "contactperson");

  psSubNode = xmlNewChild(psNode, psNsOws, BAD_CAST "IndividualName", BAD_CAST  value);

  if (!value) {
    xmlAddSibling(psSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactperson\" was missing for ows:IndividualName"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "contactposition");

  psSubNode = xmlNewChild(psNode, psNsOws, BAD_CAST "PositionName", BAD_CAST value); 

  if (!value) {
    xmlAddSibling(psSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactposition\" was missing for ows:PositionName"));
  }

  psSubNode = xmlNewChild(psNode, psNsOws, BAD_CAST "ContactInfo", NULL);

  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "Phone", NULL);

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "contactvoicetelephone");

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "Voice", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactvoicetelephone\" was missing for ows:Voice"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "contactfacsimiletelephone");

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "Facsimile", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactfacsimiletelephone\" was missing for ows:Facsimile"));
  }

  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "Address", NULL);

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "address");

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "DeliveryPoint", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_address\" was missing for ows:DeliveryPoint"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "city");

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "City", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_city\" was missing for ows:City"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "stateorprovince");

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "AdministrativeArea", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_stateorprovince\" was missing for ows:AdministrativeArea"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "postcode");

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "PostalCode", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_postcode\" was missing for ows:PostalCode"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "country");

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "Country", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_country\" was missing for ows:Country"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "contactelectronicmailaddress");

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "ElectronicMailAddress", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactelectronicmailaddress\" was missing for ows:ElectronicMailAddress"));
  }

  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "OnlineResource", NULL);

  xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "type", BAD_CAST "simple");

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "service_onlineresource");

  xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "href", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_service_onlineresource\" was missing for ows:OnlineResource/@xlink:href"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "hoursofservice");

  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "HoursOfService", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_hoursofservice\" was missing for ows:HoursOfService"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "contactinstructions");

  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "ContactInstructions", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_contactinstructions\" was missing for ows:ContactInstructions"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "O", "role");

  psSubNode = xmlNewChild(psNode, psNsOws, BAD_CAST "Role", BAD_CAST value);

  if (!value) {
    xmlAddSibling(psSubNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"ows_role\" was missing for ows:Role"));
  }

  return psRootNode;

}

/**
 * msOWSCommonOperationsMetadata()
 *
 * returns the root element of OperationsMetadata as per subclause 7.4.5
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonOperationsMetadata() {
  xmlNsPtr psNs         = NULL;
  xmlNodePtr psRootNode = NULL;

  psNs = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);
  psRootNode = xmlNewNode(psNs, BAD_CAST "OperationsMetadata");
  return psRootNode;
}

/**
 * msOWSCommonOperationsMetadataOperation()
 *
 * returns an Operation element of OperationsMetadata as per subclause 7.4.5
 *
 * @param name name of the Operation
 * @param method HTTP method (1 for GET only, 2 for POST only, 3 for GET and POST)
 * @param url online resource URL
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonOperationsMetadataOperation(char *name, int method, char *url) {
  xmlNsPtr   psNs            = NULL;
  xmlNsPtr   psNsXLink       = NULL;
  xmlNodePtr psRootNode      = NULL;
  xmlNodePtr psNode          = NULL;
  xmlNodePtr psSubNode       = NULL;
  xmlNodePtr psSubSubNode    = NULL;

  psNs = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  psNsXLink = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);

  psRootNode = xmlNewNode(psNs, BAD_CAST "Operation");

  xmlNewProp(psRootNode, BAD_CAST "name", BAD_CAST name);

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "DCP", NULL);

  psSubNode = xmlNewChild(psNode, psNs, BAD_CAST "HTTP", NULL);

  if (method  == 1 || method == 3) {
    psSubSubNode = xmlNewChild(psSubNode, psNs, BAD_CAST "Get", NULL);
    xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "type", BAD_CAST "simple");
    xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "href", BAD_CAST url);
  }

  if (method == 2 || method == 3) {
    psSubSubNode = xmlNewChild(psSubNode, psNs, BAD_CAST "Post", NULL);
    xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "type", BAD_CAST "simple");
    xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "href", BAD_CAST url);
  }

  return psRootNode;
}

/**
 * msOWSCommonOperationsMetadataDomainType()
 *
 * returns a Parameter or Constraint element (which are of type ows:DomainType)
 * of OperationsMetadata as per subclause 7.4.5
 *
 * @param elname name of the element (Parameter | Constraint)
 * @param name name of the Parameter
 * @param values list of values (comma seperated list) or NULL if none
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonOperationsMetadataDomainType(char *elname, char *name, char *values) {
  xmlNsPtr   psNs       = NULL;
  xmlNodePtr psRootNode = NULL;
  xmlNodePtr psNode     = NULL; 

  psNs = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  psRootNode = xmlNewNode(psNs, BAD_CAST elname);

  xmlNewProp(psRootNode, BAD_CAST "name", BAD_CAST name);

  if (values != NULL) {
    char **tokens = NULL;
    int n = 0;
    int i = 0;
    tokens = msStringSplit(values, ',', &n);
    if (tokens && n > 0) {
      for (i=0; i<n; i++) { 
        psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "Value", BAD_CAST tokens[i]);
      }
      msFreeCharArray(tokens, n);
    }
  }
  return psRootNode;
}

/**
 * msOWSCommonExceptionReport()
 *
 * returns an object of ExceptionReport as per clause 8
 *
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

xmlNodePtr msOWSCommonExceptionReport(const char *schemas_location, const char *version, const char *language, const char *exceptionCode, const char *locator, const char *ExceptionText) {
  char *xsi_schemaLocation = NULL;

  xmlNsPtr     psNsXsi     = NULL;
  xmlNsPtr     psNsOws     = NULL;
  xmlNodePtr   psRootNode  = NULL;
  xmlNodePtr   psMainNode  = NULL;
  xmlNodePtr   psNode      = NULL;

  psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  psRootNode = xmlNewNode(psNsOws, BAD_CAST "ExceptionReport");

  psNsXsi = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);

  xmlSetNs(psRootNode,  psNsOws);

  /* add attributes to root element */
  xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST version);

  if (language !=  "undefined") {
    xmlNewProp(psRootNode, BAD_CAST "language", BAD_CAST language);
  }

  xsi_schemaLocation = strdup(MS_OWSCOMMON_OWS_NAMESPACE_URI);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, (char *)schemas_location);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/ows/1.0.0/owsExceptionReport.xsd");

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
    psNode = xmlNewChild(psMainNode, NULL, BAD_CAST "ExceptionText", BAD_CAST ExceptionText);
  }

  xmlFreeNs(psNsOws);
  return psRootNode;
}

/**
 * msOWSCommonBoundingBox()
 *
 * returns an object of BoundingBox as per subclause 10.2.1
 *
 * @param crs the CRS / EPSG code
 * @param dimensions number of dimensions of the coordinates
 * @param minx minx
 * @param miny miny
 * @param maxx maxx
 * @param maxy maxy
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonBoundingBox(const char *crs, int dimensions, double minx, double miny, double maxx, double maxy) {
  char LowerCorner[100];
  char UpperCorner[100];

  xmlNsPtr   psNs       = NULL;
  xmlNodePtr psRootNode = NULL;
  xmlNodePtr psNode     = NULL;

  psNs = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  /* create element name */
  psRootNode = xmlNewNode(psNs, BAD_CAST "BoundingBox");

  /* add attributes to the root element */
  xmlNewProp(psRootNode, BAD_CAST "crs", BAD_CAST crs);

  xmlNewProp(psRootNode, BAD_CAST "dimensions", BAD_CAST dimensions);

  sprintf(LowerCorner, "%g %g", minx, miny);
  sprintf(UpperCorner, "%g %g", maxx, maxy);

  /* add child elements */
  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "LowerCorner", BAD_CAST LowerCorner);
  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "UpperCorner", BAD_CAST UpperCorner);

  return psRootNode;
}

/**
 * msOWSCommonWGS84BoundingBox()
 *
 * returns an object of BoundingBox as per subclause 10.2.2
 *
 * @param dimensions number of dimensions of the coordinates
 * @param minx minx
 * @param miny miny
 * @param maxx maxx
 * @param maxy maxy
 *
 * @return psRootNode xmlNodePtr pointer of XML construct
 *
 */

xmlNodePtr msOWSCommonWGS84BoundingBox(int dimensions, double minx, double miny, double maxx, double maxy) {
  char LowerCorner[100];
  char UpperCorner[100];

  xmlNsPtr   psNs       = NULL;
  xmlNodePtr psRootNode = NULL;
  xmlNodePtr psNode     = NULL;

  psNs = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  /* create element name */
  psRootNode = xmlNewNode(psNs, BAD_CAST "WGS84BoundingBox");

  xmlNewProp(psRootNode, BAD_CAST "dimensions", BAD_CAST dimensions);

  sprintf(LowerCorner, "%g %g", minx, miny);
  sprintf(UpperCorner, "%g %g", maxx, maxy);

  /* add child elements */
  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "LowerCorner", BAD_CAST LowerCorner);
  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "UpperCorner", BAD_CAST UpperCorner);

  return psRootNode;
}

#endif /* defined(USE_SOS_SVR) */


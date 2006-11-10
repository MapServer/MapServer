/******************************************************************************
 *
 * Name:     mapowscommon.c
 * Project:  MapServer
 * Purpose:  OGC OWS Common Implementation for use by MapServer OGC code
 *           
 * Author:   Tom Kralidis (tomkralidis@hotmail.com)
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
 *
 ******************************************************************************
 *
 * $Log$
 * Revision 1.2  2006/11/10 18:38:36  frank
 * Added big ifdef USE_SOS_SVR for now.
 *
 * Revision 1.1  2006/11/10 01:37:44  tkralidi
 * Initial implementation (bug 1954)
 *
 *
 ******************************************************************************/

#include "map.h"

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
  const char *keywords = NULL;

  xmlNsPtr     psNs       = NULL;
  xmlNodePtr   psRootNode = NULL;
  xmlNodePtr   psNode     = NULL;
  xmlNodePtr   psSubNode  = NULL;

  psNs = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);

  /* create element name */
  psRootNode = xmlNewNode(xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX), BAD_CAST "ServiceIdentification");

  /* add child elements */
  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "Title", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "title"));
  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "Abstract", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "abstract"));

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "Keywords", NULL);

  keywords = msOWSLookupMetadata(&(map->web.metadata), "O", "keywordlist");

  if (keywords) {
    char **tokens = NULL;
    int n = 0;
    int i = 0;
    tokens = split(keywords, ',', &n);
    if (tokens && n > 0) {
      for (i=0; i<n; i++) {
        psSubNode = xmlNewChild(psNode, NULL, BAD_CAST "Keyword", BAD_CAST tokens[i]);
        xmlSetNs(psSubNode, psNs);
      }
      msFreeCharArray(tokens, n);
    }
  }

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "ServiceType", BAD_CAST servicetype);
  
  xmlNewProp(psNode, BAD_CAST "codeSpace", BAD_CAST MS_OWSCOMMON_OGC_CODESPACE);

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "ServiceTypeVersion", BAD_CAST version);

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "Fees", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "fees"));

  psNode = xmlNewChild(psRootNode, psNs, BAD_CAST "AccessConstraints", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "accessconstraints"));

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
  psNode = xmlNewChild(psRootNode, psNsOws, BAD_CAST "ProviderName", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "contactorganization"));

  psNode = xmlNewChild(psRootNode, psNsOws, BAD_CAST "ProviderSite", NULL);

  xmlNewNsProp(psNode, psNsXLink, BAD_CAST "type", BAD_CAST "simple");

  xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "service_onlineresource"));

  psNode = xmlNewChild(psRootNode, psNsOws, BAD_CAST "ServiceContact", NULL);

  psSubNode = xmlNewChild(psNode, psNsOws, BAD_CAST "IndividualName", BAD_CAST  msOWSLookupMetadata(&(map->web.metadata), "O", "contactperson"));

  psSubNode = xmlNewChild(psNode, psNsOws, BAD_CAST "PositionName", BAD_CAST  msOWSLookupMetadata(&(map->web.metadata), "O", "contactposition"));

  psSubNode = xmlNewChild(psNode, psNsOws, BAD_CAST "ContactInfo", NULL);

  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "Phone", NULL);

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "Voice", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "contactvoicetelephone"));

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "Facsimile", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "contactfacsimiletelephone"));

  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "Address", NULL);

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "DeliveryPoint", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "address"));

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "City", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "city"));

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "AdministrativeArea", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "stateorprovince"));

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "PostalCode", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "postcode"));

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "Country", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "country"));

  psSubSubSubNode = xmlNewChild(psSubSubNode, psNsOws, BAD_CAST "ElectronicMailAddress", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "contactelectronicmailaddress"));
  
  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "OnlineResource", NULL);

  xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "type", BAD_CAST "simple");

  xmlNewNsProp(psSubSubNode, psNsXLink, BAD_CAST "href", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "service_onlineresource"));

  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "HoursOfService", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "hoursofservice"));

  psSubSubNode = xmlNewChild(psSubNode, psNsOws, BAD_CAST "ContactInstructions", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "contactinstructions"));

  psSubNode = xmlNewChild(psNode, psNsOws, BAD_CAST "Role", BAD_CAST msOWSLookupMetadata(&(map->web.metadata), "O", "role"));

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
  xsi_schemaLocation = strcatalloc(xsi_schemaLocation, " ");
  xsi_schemaLocation = strcatalloc(xsi_schemaLocation, (char *)schemas_location);
  xsi_schemaLocation = strcatalloc(xsi_schemaLocation, "/ows/1.0.0/owsExceptionReport.xsd");

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


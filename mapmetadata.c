/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Metadata implementation
 * Author:   Tom Kralidis (tomkralidis@gmail.com)
 *
 **********************************************************************
 * Copyright (c) 2017, Tom Kralidis
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
#include "mapowscommon.h"
#include "maplibxml2.h"


/************************************************************************/
/*                   _msMetadataGetCharacterString                      */
/*                                                                      */
/*      Create a gmd:name/gmd:CharacterString element pattern           */
/************************************************************************/

xmlNodePtr _msMetadataGetCharacterString(xmlNsPtr namespace, char *name, char *value) {
  xmlNsPtr psNsGco = NULL;
  xmlNodePtr psNode = NULL;

  psNsGco = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd", BAD_CAST "gco");

  psNode = xmlNewNode(namespace, BAD_CAST name);

  if (!value)
    xmlNewNsProp(psNode, psNsGco, BAD_CAST "nilReason", BAD_CAST "missing");
  else
    xmlNewChild(psNode, psNsGco, BAD_CAST "CharacterString", BAD_CAST value);
  return psNode;

}


/************************************************************************/
/*                   _msMetadataGetURL                                  */
/*                                                                      */
/*      Create a gmd:name/gmd:URL element pattern                       */
/************************************************************************/

xmlNodePtr _msMetadataGetURL(xmlNsPtr namespace, char *name, char *value) {
  xmlNsPtr psNsGco = NULL;
  xmlNodePtr psNode = NULL;

  psNsGco = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd", BAD_CAST "gco");

  psNode = xmlNewNode(namespace, BAD_CAST name);

  if (!value)
    xmlNewNsProp(psNode, psNsGco, BAD_CAST "nilReason", BAD_CAST "missing");
  else
    xmlNewChild(psNode, namespace, BAD_CAST "URL", BAD_CAST value);
  return psNode;

}

/************************************************************************/
/*                   _msMetadataGetOnline                               */
/*                                                                      */
/*               Create a gmd:onLine element pattern                    */
/************************************************************************/

xmlNodePtr _msMetadataGetOnline(xmlNsPtr namespace, layerObj *layer, char *service, char *format, char *desc, char *url_in) {

  int status;
  char *url = NULL;
  char buffer[32];
  char *epsg_str;

  xmlNodePtr psNode = NULL;
  xmlNodePtr psORNode = NULL;

  rectObj rect;

  psNode = xmlNewNode(namespace, BAD_CAST "onLine");
  psORNode = xmlNewChild(psNode, namespace, BAD_CAST "CI_OnlineResource", NULL);

  url = msStrdup(url_in);

  if (strcasecmp(service, "M") == 0) {
    url = msStringConcatenate(url, msEncodeHTMLEntities("service=WMS&version=1.3.0&request=GetMap&width=500&height=300&styles=&layers="));
    url = msStringConcatenate(url, msEncodeHTMLEntities(layer->name));
    url = msStringConcatenate(url, msEncodeHTMLEntities("&format="));
    url = msStringConcatenate(url, msEncodeHTMLEntities(format));
    url = msStringConcatenate(url, msEncodeHTMLEntities("&crs="));
    msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "MFCSGO", MS_TRUE, &epsg_str);
    url = msStringConcatenate(url, msEncodeHTMLEntities(epsg_str));

    status = msLayerGetExtent(layer, &rect);

    if (status == 0) {
      url = msStringConcatenate(url, msEncodeHTMLEntities("&bbox="));
      sprintf(buffer, "%f", rect.miny);
      url = msStringConcatenate(url, buffer);
      url = msStringConcatenate(url, ",");
      sprintf(buffer, "%f", rect.minx);
      url = msStringConcatenate(url, buffer);
      url = msStringConcatenate(url, ",");
      sprintf(buffer, "%f", rect.maxy);
      url = msStringConcatenate(url, buffer);
      url = msStringConcatenate(url, ",");
      sprintf(buffer, "%f", rect.maxx);
      url = msStringConcatenate(url, buffer);
    }
  }
  else if (strcasecmp(service, "F") == 0) {
    url = msStringConcatenate(url, msEncodeHTMLEntities("service=WFS&version=1.1.0&request=GetFeature&typename="));
    url = msStringConcatenate(url, msEncodeHTMLEntities(layer->name));
    url = msStringConcatenate(url, msEncodeHTMLEntities("&outputformat="));
    url = msStringConcatenate(url, msEncodeHTMLEntities(format));
  }
  else if (strcasecmp(service, "C") == 0) {
    url = msStringConcatenate(url, msEncodeHTMLEntities("service=WCS&version=2.0.1&request=GetCoverage&coverageid="));
    url = msStringConcatenate(url, msEncodeHTMLEntities(layer->name));
    url = msStringConcatenate(url, msEncodeHTMLEntities("&format="));
    url = msStringConcatenate(url, msEncodeHTMLEntities(format));
  }

  xmlAddChild(psORNode, _msMetadataGetURL(namespace, "linkage", url));

  xmlAddChild(psORNode, _msMetadataGetCharacterString(namespace, "protocol", "WWW:DOWNLOAD-1.0-http--download"));
  xmlAddChild(psORNode, _msMetadataGetCharacterString(namespace, "name", layer->name));

  xmlAddChild(psORNode, _msMetadataGetCharacterString(namespace, "description", desc));

  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetInteger                              */
/*                                                                      */
/*      Create a gmd:name/gmd:Integer element pattern                   */
/************************************************************************/

xmlNodePtr _msMetadataGetInteger(xmlNsPtr namespace, char *name, int value) {
  char buffer[8];
  xmlNsPtr psNsGco = NULL;
  xmlNodePtr psNode = NULL;

  sprintf(buffer, "%d", value);

  psNsGco = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gco", BAD_CAST "gco");

  psNode = xmlNewNode(namespace, BAD_CAST name);

  if (!value)
    xmlNewNsProp(psNode, psNsGco, BAD_CAST "nilReason", BAD_CAST "missing");
  else
    xmlNewChild(psNode, psNsGco, BAD_CAST "Integer", BAD_CAST buffer);
  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetDecimal                              */
/*                                                                      */
/*      Create a gmd:name/gmd:Decimal element pattern                   */
/************************************************************************/

xmlNodePtr _msMetadataGetDecimal(xmlNsPtr namespace, char *name, double value) {
  char buffer[32];
  xmlNsPtr psNsGco = NULL;
  xmlNodePtr psNode = NULL;

  sprintf(buffer, "%f", value);

  psNsGco = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gco", BAD_CAST "gco");

  psNode = xmlNewNode(namespace, BAD_CAST name);

  if (!value)
    xmlNewNsProp(psNode, psNsGco, BAD_CAST "nilReason", BAD_CAST "missing");
  else
    xmlNewChild(psNode, psNsGco, BAD_CAST "Decimal", BAD_CAST buffer);
  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetCodeList                             */
/*                                                                      */
/*      Create a gmd:name/gmd:* code list element pattern               */
/************************************************************************/

xmlNodePtr _msMetadataGetCodeList(xmlNsPtr namespace, char *parent_element, char *name, char *value) {
  char *codelist = NULL;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psCodeNode = NULL;

  codelist = msStrdup("http://www.isotc211.org/2005/resources/Codelist/gmxCodelists.xml#");
  codelist = msStringConcatenate(codelist, name);

  psNode = xmlNewNode(namespace, BAD_CAST parent_element);
  psCodeNode = xmlNewChild(psNode, namespace, BAD_CAST name, BAD_CAST value);
  xmlNewProp(psCodeNode, BAD_CAST "codeSpace", BAD_CAST "ISOTC211/19115");
  xmlNewProp(psCodeNode, BAD_CAST "codeList", BAD_CAST codelist);
  xmlNewProp(psCodeNode, BAD_CAST "codeListValue", BAD_CAST value);
  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetDate                                 */
/*                                                                      */
/*      Create a gmd:date or gmd:dateStamp element pattern              */
/************************************************************************/

xmlNodePtr _msMetadataGetDate(xmlNsPtr namespace, char *parent_element, char *date_type, char *value) {
  xmlNsPtr psNsGco = NULL;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psNode2 = NULL;

  psNsGco = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd", BAD_CAST "gco");

  psNode = xmlNewNode(namespace, BAD_CAST parent_element);

  if (date_type == NULL) {  /* it's a gmd:dateStamp */
      xmlNewChild(psNode, psNsGco, BAD_CAST "Date", BAD_CAST value);
      return psNode;
  }

  psNode2 = xmlNewChild(psNode, namespace, BAD_CAST "date", NULL);
  xmlNewChild(psNode2, psNsGco, BAD_CAST "Date", BAD_CAST value);
  xmlAddChild(psNode, _msMetadataGetCodeList(namespace, "dateType", "CI_DateTypeCode", date_type));

  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetGMLTimePeriod                        */
/*                                                                      */
/*      Create a gml:TimePeriod element pattern                         */
/************************************************************************/

xmlNodePtr _msMetadataGetGMLTimePeriod(char **temporal)
{
  xmlNsPtr psNsGml = NULL;
  xmlNodePtr psNode = NULL;

  psNsGml = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml");
  psNode = xmlNewNode(psNsGml, BAD_CAST "TimePeriod");
  xmlNewNsProp(psNode, psNsGml, BAD_CAST "id", BAD_CAST "T001");
  xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml");
  xmlSetNs(psNode, psNsGml);
  xmlNewChild(psNode, psNsGml, BAD_CAST "beginPosition", BAD_CAST temporal[0]);
  xmlNewChild(psNode, psNsGml, BAD_CAST "endPosition", BAD_CAST temporal[1]);

  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetExtent                               */
/*                                                                      */
/*            Create a gmd:extent element pattern                       */
/************************************************************************/

xmlNodePtr _msMetadataGetExtent(xmlNsPtr namespace, layerObj *layer)
{
  int n;
  int status;
  char *value = NULL;
  char **temporal = NULL;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psEXNode = NULL;
  xmlNodePtr psGNode = NULL;
  xmlNodePtr psGNode2 = NULL;
  xmlNodePtr psTNode = NULL;
  xmlNodePtr psTNode2 = NULL;
  xmlNodePtr psENode = NULL;

  rectObj rect;

  psNode = xmlNewNode(namespace, BAD_CAST "extent");
  psEXNode = xmlNewChild(psNode, namespace, BAD_CAST "EX_Extent", NULL);

  /* scan for geospatial extent */
  status = msLayerGetExtent(layer, &rect); 

  if (status == 0) {
    /* always project to lat long */
    msOWSProjectToWGS84(&layer->projection, &rect);

    psGNode = xmlNewChild(psEXNode, namespace, BAD_CAST "geographicElement", NULL);
    psGNode2 = xmlNewChild(psGNode, namespace, BAD_CAST "EX_GeographicBoundingBox", NULL);
    xmlAddChild(psGNode2, _msMetadataGetDecimal(namespace, "westBoundLongitude", rect.minx));
    xmlAddChild(psGNode2, _msMetadataGetDecimal(namespace, "eastBoundLongitude", rect.maxx));
    xmlAddChild(psGNode2, _msMetadataGetDecimal(namespace, "southBoundLatitude", rect.miny));
    xmlAddChild(psGNode2, _msMetadataGetDecimal(namespace, "northBoundLatitude", rect.maxy));
  }

  /* scan for temporal extent */
  value = (char *)msOWSLookupMetadata(&(layer->metadata), "MO", "timeextent");
  if (value) { /* WMS */
    temporal = msStringSplit(value, '/', &n);
  }
  else { /* WCS */
    value = (char *)msOWSLookupMetadata(&(layer->metadata), "CO", "timeposition");
    if (value) {
      /* split extent */
      temporal = msStringSplit(value, ',', &n);
    }
  }
  if (!value) { /* SOS */
      value = (char *)msOWSLookupMetadata(&(layer->metadata), "SO", "offering_timeextent");
      if (value)
        temporal = msStringSplit(value, '/', &n);
  }
  if (value) { 
    if (temporal && n > 0) {
      psTNode = xmlNewChild(psEXNode, namespace, BAD_CAST "temporalElement", NULL);
      psTNode2 = xmlNewChild(psTNode, namespace, BAD_CAST "EX_TemporalExtent", NULL);
      psENode = xmlNewChild(psTNode2, namespace, BAD_CAST "extent", NULL);
      xmlAddChild(psENode, _msMetadataGetGMLTimePeriod(temporal));

      msFreeCharArray(temporal, n);
    }
  }

  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetReferenceSystemInfo                  */
/*                                                                      */
/*      Create a gmd:referenceSystemInfo element pattern                */
/************************************************************************/

xmlNodePtr _msMetadataGetReferenceSystemInfo(xmlNsPtr namespace, layerObj *layer)
{
  char *epsg_str;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psRSNode = NULL;
  xmlNodePtr psRSINode = NULL;
  xmlNodePtr psRSINode2 = NULL;

  psNode = xmlNewNode(namespace, BAD_CAST "referenceSystemInfo");
  psRSNode = xmlNewChild(psNode, namespace, BAD_CAST "MD_ReferenceSystem", NULL);
  psRSINode = xmlNewChild(psRSNode, namespace, BAD_CAST "referenceSystemIdentifier", NULL);
  psRSINode2 = xmlNewChild(psRSINode, namespace, BAD_CAST "RS_Identifier", NULL);

  msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "MFCSGO", MS_TRUE, &epsg_str);
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(namespace, "code", epsg_str));
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(namespace, "codeSpace", "http://www.epsg-registry.org"));
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(namespace, "version", "6.14"));

  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetContact                              */
/*                                                                      */
/*      Create a gmd:contact element pattern                            */
/************************************************************************/

xmlNodePtr _msMetadataGetContact(xmlNsPtr namespace, char *contact_element, mapObj *map)
{
  char *value = NULL;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psCNode = NULL;
  xmlNodePtr psCINode = NULL;
  xmlNodePtr psCINode2 = NULL;
  xmlNodePtr psPhoneNode = NULL;
  xmlNodePtr psCIPhoneNode = NULL;
  xmlNodePtr psAddressNode = NULL;
  xmlNodePtr psCIAddressNode = NULL;
  xmlNodePtr psORNode = NULL;
  xmlNodePtr psORNode2 = NULL;

  psNode = xmlNewNode(namespace, BAD_CAST contact_element);
  psCNode = xmlNewChild(psNode, namespace, BAD_CAST "CI_ResponsibleParty", NULL);
  xmlNewProp(psCNode, BAD_CAST "id", BAD_CAST contact_element);

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "contactperson");
  if (value)
    xmlAddChild(psCNode, _msMetadataGetCharacterString(namespace, "individualName", value));

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "contactorganization");
  if (value)
    xmlAddChild(psCNode, _msMetadataGetCharacterString(namespace, "organisationName", value));

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "contactposition");
  if (value)
    xmlAddChild(psCNode, _msMetadataGetCharacterString(namespace, "positionName", value));

  psCINode = xmlNewChild(psCNode, namespace, BAD_CAST "contactInfo", NULL);
  psCINode2 = xmlNewChild(psCINode, namespace, BAD_CAST "CI_Contact", NULL);
  psPhoneNode = xmlNewChild(psCINode2, namespace, BAD_CAST "phone", NULL);
  psCIPhoneNode = xmlNewChild(psPhoneNode, namespace, BAD_CAST "CI_Telephone", NULL);

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "contactvoicetelephone");
  if (value)
    xmlAddChild(psCIPhoneNode, _msMetadataGetCharacterString(namespace, "voice", value));

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "contactfacsimiletelephone");
  if (value)
    xmlAddChild(psCIPhoneNode, _msMetadataGetCharacterString(namespace, "facsimile", value));

  psAddressNode = xmlNewChild(psCINode2, namespace, BAD_CAST "address", NULL);
  psCIAddressNode = xmlNewChild(psAddressNode, namespace, BAD_CAST "CI_Address", NULL);

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "address");
  if (value)
    xmlAddChild(psCIAddressNode, _msMetadataGetCharacterString(namespace, "deliveryPoint", value));

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "city");
  if (value)
    xmlAddChild(psCIAddressNode, _msMetadataGetCharacterString(namespace, "city", value));

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "stateorprovince");
  if (value)
    xmlAddChild(psCIAddressNode, _msMetadataGetCharacterString(namespace, "administrativeArea", value));

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "postcode");
  if (value)
    xmlAddChild(psCIAddressNode, _msMetadataGetCharacterString(namespace, "postalCode", value));

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "country");
  if (value)
    xmlAddChild(psCIAddressNode, _msMetadataGetCharacterString(namespace, "country", value));

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "contactelectronicmailaddress");
  if (value)
    xmlAddChild(psCIAddressNode, _msMetadataGetCharacterString(namespace, "electronicMailAddress", value));

  psORNode = xmlNewChild(psCINode2, namespace, BAD_CAST "onlineResource", NULL);
  psORNode2 = xmlNewChild(psORNode, namespace, BAD_CAST "CI_OnlineResource", NULL);

  value = (char *)msOWSLookupMetadata(&(map->web.metadata), "MCFO", "onlineresource");
  if (value)
    xmlAddChild(psORNode2, _msMetadataGetURL(namespace, "linkage", value));

  xmlAddChild(psCNode, _msMetadataGetCodeList(namespace, "role", "CI_RoleCode", "pointOfContact"));

  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetIdentificationInfo                   */
/*                                                                      */
/*      Create a gmd:identificationInfo element pattern                 */
/************************************************************************/

xmlNodePtr _msMetadataGetIdentificationInfo(xmlNsPtr namespace, mapObj *map, layerObj *layer)
{
  int i = 0;
  int n;
  char *value;
  char **tokens = NULL;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psDINode = NULL;
  xmlNodePtr psCNode = NULL;
  xmlNodePtr psCINode = NULL;
  xmlNodePtr psDNode = NULL;
  xmlNodePtr psKWNode = NULL;
  xmlNodePtr psMDKNode = NULL;

  psNode = xmlNewNode(namespace, BAD_CAST "identificationInfo");

  psDINode = xmlNewChild(psNode, namespace, BAD_CAST "MD_DataIdentification", NULL);
  xmlNewProp(psDINode, BAD_CAST "id", BAD_CAST layer->name);
  psCNode = xmlNewChild(psDINode, namespace, BAD_CAST "citation", NULL);
  psCINode = xmlNewChild(psCNode, namespace, BAD_CAST "CI_Citation", NULL);

  value = (char *)msOWSLookupMetadata(&(layer->metadata), "MCFGO", "title");
  if (!value)
      value = (char *)msOWSLookupMetadata(&(layer->metadata), "S", "offering_name");
  xmlAddChild(psCINode, _msMetadataGetCharacterString(namespace, "title", value));

  psDNode = xmlNewChild(psCINode, namespace, BAD_CAST "date", NULL);

  xmlAddChild(psDNode, _msMetadataGetDate(namespace, "CI_Date", "publication", "2011"));

  value = (char *)msOWSLookupMetadata(&(layer->metadata), "MCFGO", "abstract");
  if (!value)
      value = (char *)msOWSLookupMetadata(&(layer->metadata), "S", "offering_description");
  xmlAddChild(psDINode, _msMetadataGetCharacterString(namespace, "abstract", value));

  value = (char *)msOWSLookupMetadata(&(layer->metadata), "MCFSGO", "keywordlist");

  if (value) {
    psKWNode = xmlNewChild(psDINode, namespace, BAD_CAST "descriptiveKeywords", NULL);
    psMDKNode = xmlNewChild(psKWNode, namespace, BAD_CAST "MD_Keywords", NULL);

    tokens = msStringSplit(value, ',', &n);
    if (tokens && n > 0) {
      for (i=0; i<n; i++) {
        xmlAddChild(psMDKNode, _msMetadataGetCharacterString(namespace, "keyword", tokens[i]));
      }
      msFreeCharArray(tokens, n);
    }
  }

  xmlAddChild(psDINode, _msMetadataGetCharacterString(namespace, "language", (char *)msOWSGetLanguage(map, "exception")));
  xmlAddChild(psDINode, _msMetadataGetExtent(namespace, layer));

  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetSpatialRepresentationInfo            */
/*                                                                      */
/*      Create a gmd:spatialRepresentationInfo element pattern          */
/************************************************************************/

xmlNodePtr _msMetadataGetSpatialRepresentationInfo(xmlNsPtr namespace, layerObj *layer)
{
  char *value;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psSRNode = NULL;
  xmlNodePtr psGONode = NULL;
  xmlNodePtr psGONode2 = NULL;

  psNode = xmlNewNode(namespace, BAD_CAST "spatialRepresentationInfo");

  if (layer->type == MS_LAYER_RASTER) {
    psSRNode = xmlNewChild(psNode, namespace, BAD_CAST "MD_GridSpatialRepresentation", NULL);
    xmlAddChild(psSRNode, _msMetadataGetInteger(namespace, "numberOfDimensions", 2));
    xmlAddChild(psSRNode, _msMetadataGetCodeList(namespace, "cellGeometry", "MD_CellGeometryCode", "area"));
  }
  else {
    psSRNode = xmlNewChild(psNode, namespace, BAD_CAST "MD_VectorSpatialRepresentation", NULL);
    xmlAddChild(psSRNode, _msMetadataGetCodeList(namespace, "topologyLevel", "MD_TopologyLevelCode", "geometryOnly"));
    psGONode = xmlNewChild(psSRNode, namespace, BAD_CAST "geometricObjects", NULL);
    psGONode2 = xmlNewChild(psGONode, namespace, BAD_CAST "MD_GeometricObjects", NULL);
    if (layer->type == MS_LAYER_POINT)
      value = "point";
    else if (layer->type == MS_LAYER_LINE)
      value = "curve";
    else if (layer->type == MS_LAYER_POLYGON)
      value = "surface";
    else
      value = "complex";
    xmlAddChild(psGONode2, _msMetadataGetCodeList(namespace, "geometricObjectType", "MD_GeometricObjectTypeCode", value));
    // TODO: find way to get feature count in a fast way
    /* xmlAddChild(psGONode2, _msMetadataGetInteger(namespace, "geometricObjectCount", msLayerGetNumFeatures(layer))); */
  }

  return psNode;
}


/************************************************************************/
/*                   _msMetadataGetDistributionInfo                     */
/*                                                                      */
/*      Create a gmd:identificationInfo element pattern                 */
/************************************************************************/

xmlNodePtr _msMetadataGetDistributionInfo(xmlNsPtr namespace, mapObj *map, layerObj *layer, cgiRequestObj *cgi_request)
{
  char *url = NULL;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psMDNode = NULL;
  xmlNodePtr psTONode = NULL;
  xmlNodePtr psDTONode = NULL;
  xmlNodePtr psDNode = NULL;
  xmlNodePtr psDNode2 = NULL;

  psNode = xmlNewNode(namespace, BAD_CAST "distributionInfo");
  psMDNode = xmlNewChild(psNode, namespace, BAD_CAST "MD_Distribution", NULL);

  url = msEncodeHTMLEntities(msOWSGetOnlineResource(map, "MFCSGO", "onlineresource", cgi_request));

  /* gmd:distributor */
  psDNode = xmlNewChild(psMDNode, namespace, BAD_CAST "distributor", NULL);
  psDNode2 = xmlNewChild(psDNode, namespace, BAD_CAST "MD_Distributor", NULL);
  xmlAddChild(psDNode2, _msMetadataGetContact(namespace, "distributorContact", map));

  /* gmd:transferOptions */
  psTONode = xmlNewChild(psMDNode, namespace, BAD_CAST "transferOptions", NULL);
  psDTONode = xmlNewChild(psTONode, namespace, BAD_CAST "MD_DigitalTransferOptions", NULL);
  xmlAddChild(psDTONode, _msMetadataGetCharacterString(namespace, "unitsOfDistribution", "KB"));

  /* links */

  /* WMS */
  xmlAddChild(psDTONode, _msMetadataGetOnline(namespace, layer, "M", "image/png", "PNG Format", url));

  xmlAddChild(psDTONode, _msMetadataGetOnline(namespace, layer, "M", "image/jpeg", "JPEG Format", url));
  xmlAddChild(psDTONode, _msMetadataGetOnline(namespace, layer, "M", "image/gif", "GIF Format", url));

  /* WCS */
  if (layer->type == MS_LAYER_RASTER) {
    xmlAddChild(psDTONode, _msMetadataGetOnline(namespace, layer, "C", "image/tiff", "GeoTIFF Format", url));
  }
  /* WFS */
  else {
    xmlAddChild(psDTONode, _msMetadataGetOnline(namespace, layer, "F", "GML2", "GML2 Format", url));
    xmlAddChild(psDTONode, _msMetadataGetOnline(namespace, layer, "F", "GML3", "GML3 Format", url));
  }

  return psNode;
}

/************************************************************************/
/*                   msMetadataGetExceptionReport                       */
/*                                                                      */
/*              Generate an OWS Common Exception Report                 */
/************************************************************************/

xmlNodePtr msMetadataGetExceptionReport(mapObj *map, char *code, char *locator, char *message)
{
  char *schemas_location = NULL;

  xmlNsPtr psNsOws = NULL;
  xmlNodePtr psRootNode = NULL;

  schemas_location = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
  psNsOws = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");

  psRootNode = msOWSCommonExceptionReport(psNsOws,
                                          OWS_1_1_0,
                                          schemas_location,
                                          "1.1.0",
                                          msOWSGetLanguage(map, "exception"),
                                          code, locator, message);

  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");

  return psRootNode;

}

/************************************************************************/
/*                   msMetadataGetLayerMetadata                         */
/*                                                                      */
/*      Generate an ISO 19139:2007 representation of layer metadata     */
/************************************************************************/

xmlNodePtr msMetadataGetLayerMetadata(mapObj *map, metadataParamsObj *paramsObj, cgiRequestObj *cgi_request, owsRequestObj *ows_request)
{
  int i;
  int layer_found = MS_FALSE;

  xmlNodePtr psRootNode = NULL;
  xmlNsPtr psNsGmd = NULL;
  xmlNsPtr psNsXsi = NULL;

  layerObj *layer = NULL;

  psNsXsi = xmlNewNs(NULL, BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "xsi");

  /* Check that layer requested exists in mapfile */ 
  for (i=0; i<map->numlayers; i++) {
    if(strcasecmp(GET_LAYER(map, i)->name, paramsObj->pszLayer) == 0) {
        layer_found = MS_TRUE;
        layer = GET_LAYER(map, i);
        break;
    }
  }

  if (layer_found == MS_FALSE) {
    psRootNode = msMetadataGetExceptionReport(map, "InvalidParameterValue", "layer", "Layer not found");
  }

  /* Check that outputschema is valid */
  else if (paramsObj->pszOutputSchema && strcasecmp(paramsObj->pszOutputSchema, "http://www.isotc211.org/2005/gmd") != 0) {
    psRootNode = msMetadataGetExceptionReport(map, "InvalidParameterValue", "outputschema", "OUTPUTSCHEMA must be \"http://www.isotc211.org/2005/gmd\"");
  }

  else {
    /* root element */
    psNsGmd = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd", BAD_CAST "gmd");
    psRootNode = xmlNewNode(NULL, BAD_CAST "MD_Metadata");
    xmlNewNs(psRootNode, BAD_CAST "http://www.isotc211.org/2005/gmd", BAD_CAST "gmd");
    xmlNewNs(psRootNode, BAD_CAST "http://www.isotc211.org/2005/gco", BAD_CAST "gco");
    xmlNewNs(psRootNode, BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "xsi");
    xmlSetNs(psRootNode, psNsGmd);

    xmlNewNsProp(psRootNode, psNsXsi, BAD_CAST "schemaLocation", BAD_CAST "http://www.isotc211.org/2005/gmd http://www.isotc211.org/2005/gmd/gmd.xsd");

    /* gmd:identifier */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "fileIdentifier", layer->name));

    /* gmd:language */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "language", (char *)msOWSGetLanguage(map, "exception")));

    /* gmd:hierarchyLevel */
    xmlAddChild(psRootNode, _msMetadataGetCodeList(psNsGmd, "hierarchyLevel", "MD_ScopeCode", "dataset"));

    /* gmd:contact */
    xmlAddChild(psRootNode, _msMetadataGetContact(psNsGmd, "contact", map));

    /* gmd:dateStamp */
    /* TODO: nil for now, find way to derive this automagically */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "dateStamp", NULL));

    /* gmd:metadataStandardName */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "metadataStandardName", "ISO 19115:2003 - Geographic information - Metadata"));

    /* gmd:metadataStandardVersion */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "metadataStandardVersion", "ISO 19115:2003"));

    /* gmd:spatialRepresentationInfo */
    xmlAddChild(psRootNode, _msMetadataGetSpatialRepresentationInfo(psNsGmd, layer));

    /* gmd:referenceSystemInfo */
    xmlAddChild(psRootNode, _msMetadataGetReferenceSystemInfo(psNsGmd, layer));

    /* gmd:identificationInfo */
    xmlAddChild(psRootNode, _msMetadataGetIdentificationInfo(psNsGmd, map, layer));

    /* gmd:distributionInfo */
    xmlAddChild(psRootNode, _msMetadataGetDistributionInfo(psNsGmd, map, layer, cgi_request));
  }

  return psRootNode;
}

/************************************************************************/
/*                  msMetadataDispatch                                  */
/*                                                                      */
/* Entry point for metadata requests.                                   */
/*                                                                      */
/* - If this is a valid request then it is processed and MS_SUCCESS     */
/*   is returned on success, or MS_FAILURE on failure.                  */ 
/*                                                                      */
/* - If this does not appear to be a valid WFS request then MS_DONE     */
/*   is returned and MapServer is expected to process this as a regular */
/*   MapServer request.                                                 */
/************************************************************************/

int msMetadataDispatch(mapObj *map, cgiRequestObj *cgi_request, owsRequestObj *ows_request)
{
  int i;
  int buffersize;
  int status = MS_SUCCESS;
  xmlNodePtr psRootNode = NULL;
  xmlDocPtr xml_document;
  xmlChar *xml_buffer;
  metadataParamsObj *paramsObj;
  layerObj *layer = NULL;

  /* Populate the Params object based on the request */

  paramsObj = msMetadataCreateParamsObj();

  xml_document = xmlNewDoc(BAD_CAST "1.0");
  xmlNewNs(NULL, BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "xsi");

  if (msMetadataParseRequest(map, cgi_request, ows_request, paramsObj) == MS_FAILURE) {
    psRootNode = msMetadataGetExceptionReport(map, "InvalidRequest", "layer", "Request parsing failed");
    status = MS_FAILURE;
  }

  /* if layer= is not specified, */
  if (paramsObj->pszLayer==NULL || strlen(paramsObj->pszLayer)<=0) {
    psRootNode = msMetadataGetExceptionReport(map, "MissingParameterValue", "layer", "Missing layer parameter");
    status = MS_FAILURE;
  }

  if (status == MS_SUCCESS) { /* Start dispatching request */
    /* Check that layer requested exists in mapfile */
    for (i=0; i<map->numlayers; i++) {
      if(strcasecmp(GET_LAYER(map, i)->name, paramsObj->pszLayer) == 0) {
        layer = GET_LAYER(map, i);
        break;
      }
    }
    if (layer != NULL && msOWSLookupMetadata(&(layer->metadata), "MFCO", "metadataurl_href")) {
      msIO_setHeader("Status", "301 Moved Permanently");
      msIO_setHeader("Location", "%s", msOWSLookupMetadata(&(layer->metadata), "MFCO", "metadataurl_href"));
      msIO_sendHeaders();
    }
    else {
      psRootNode = msMetadataGetLayerMetadata(map, paramsObj, cgi_request, ows_request);
    }
  }

  if (psRootNode != NULL) {
    xmlDocSetRootElement(xml_document, psRootNode);

    msIO_setHeader("Content-type", "text/xml");
    msIO_sendHeaders();

    xmlDocDumpFormatMemory(xml_document, &xml_buffer, &buffersize, 1);
    msIO_printf("%s", (char *) xml_buffer);

    xmlFree(xml_buffer);
    xmlFreeDoc(xml_document);
    msMetadataFreeParamsObj(paramsObj);
    free(paramsObj);
    paramsObj = NULL;
  }

  return status;
}

/************************************************************************/
/*                           msMetadataCreateParamsObj                  */
/*                                                                      */
/*   Create a parameter object, initialize it.                          */
/*   The caller should free the object using msMetadataFreeParamsObj.   */
/************************************************************************/

metadataParamsObj *msMetadataCreateParamsObj()
{
  metadataParamsObj *paramsObj = (metadataParamsObj *)calloc(1, sizeof(metadataParamsObj));
  MS_CHECK_ALLOC(paramsObj, sizeof(metadataParamsObj), NULL);

  paramsObj->pszLayer = NULL;
  paramsObj->pszOutputSchema = NULL;
  return paramsObj;
}


/************************************************************************/
/*                            msMetadataFreeParmsObj                    */
/*                                                                      */
/*      Free params object.                                             */
/************************************************************************/

void msMetadataFreeParamsObj(metadataParamsObj *metadataparams)
{
  if (metadataparams) {
    free(metadataparams->pszRequest);
    free(metadataparams->pszLayer);
    free(metadataparams->pszOutputSchema);
  }
}


/************************************************************************/
/*                            msMetadataParseRequest                    */
/*                                                                      */
/*      Parse request into the params object.                           */
/************************************************************************/

int msMetadataParseRequest(mapObj *map, cgiRequestObj *request, owsRequestObj *ows_request,
                      metadataParamsObj *metadataparams)
{
  int i = 0;

  if (!request || !metadataparams)
    return MS_FAILURE;

  if (request->NumParams > 0) {
    for(i=0; i<request->NumParams; i++) {
      if (request->ParamNames[i] && request->ParamValues[i]) {
        if (strcasecmp(request->ParamNames[i], "LAYER") == 0)
          metadataparams->pszLayer = msStrdup(request->ParamValues[i]);
        if (strcasecmp(request->ParamNames[i], "OUTPUTSCHEMA") == 0)
          metadataparams->pszOutputSchema = msStrdup(request->ParamValues[i]);
      }
    }
  }
  return MS_SUCCESS;
}

/************************************************************************/
/*                            msMetadataSetGetMetadataURL               */
/*                                                                      */
/*      Parse request into the params object.                           */
/************************************************************************/

void msMetadataSetGetMetadataURL(layerObj *lp, const char *url)
{
  char *pszMetadataURL=NULL;

  pszMetadataURL = msStrdup(url);
  msDecodeHTMLEntities(pszMetadataURL);
  pszMetadataURL = msStringConcatenate(pszMetadataURL, "request=GetMetadata&layer=");
  pszMetadataURL = msStringConcatenate(pszMetadataURL, lp->name);

  msInsertHashTable(&(lp->metadata), "ows_metadataurl_href", pszMetadataURL);
  msInsertHashTable(&(lp->metadata), "ows_metadataurl_type", "ISOTC211/19115");
  msInsertHashTable(&(lp->metadata), "ows_metadataurl_format", "text/xml");
  msInsertHashTable(&(lp->metadata), "ows_metadatalink_href", pszMetadataURL);
  msInsertHashTable(&(lp->metadata), "ows_metadatalink_type", "ISOTC211/19115");
  msInsertHashTable(&(lp->metadata), "ows_metadatalink_format", "text/xml");
  msFree(pszMetadataURL);
}


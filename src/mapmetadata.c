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

#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR) ||    \
    defined(USE_SOS_SVR) || defined(USE_WMS_LYR) || defined(USE_WFS_LYR)

#ifdef USE_LIBXML2

static int msMetadataParseRequest(cgiRequestObj *request,
                                  metadataParamsObj *metadataparams);

/************************************************************************/
/*                   _msMetadataGetCharacterString                      */
/*                                                                      */
/*      Create a gmd:name/gmd:CharacterString element pattern           */
/************************************************************************/

static xmlNodePtr _msMetadataGetCharacterString(xmlNsPtr namespace,
                                                const char *name,
                                                const char *value,
                                                xmlNsPtr *ppsNsGco) {

  if (*ppsNsGco == NULL)
    *ppsNsGco = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd",
                         BAD_CAST "gco");

  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST name);

  if (!value)
    xmlNewNsProp(psNode, *ppsNsGco, BAD_CAST "nilReason", BAD_CAST "missing");
  else
    xmlNewChild(psNode, *ppsNsGco, BAD_CAST "CharacterString", BAD_CAST value);
  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetURL                                  */
/*                                                                      */
/*      Create a gmd:name/gmd:URL element pattern                       */
/************************************************************************/

static xmlNodePtr _msMetadataGetURL(xmlNsPtr namespace, const char *name,
                                    const char *value, xmlNsPtr *ppsNsGco) {

  if (*ppsNsGco == NULL)
    *ppsNsGco = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd",
                         BAD_CAST "gco");

  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST name);

  if (!value)
    xmlNewNsProp(psNode, *ppsNsGco, BAD_CAST "nilReason", BAD_CAST "missing");
  else
    xmlNewChild(psNode, namespace, BAD_CAST "URL", BAD_CAST value);
  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetOnline                               */
/*                                                                      */
/*               Create a gmd:onLine element pattern                    */
/************************************************************************/

static char *_stringConcatenateAndEncodeHTML(char *str, const char *appendStr) {
  char *pszTmp = msEncodeHTMLEntities(appendStr);
  str = msStringConcatenate(str, pszTmp);
  msFree(pszTmp);
  return str;
}

static xmlNodePtr _msMetadataGetOnline(xmlNsPtr namespace, layerObj *layer,
                                       const char *service, const char *format,
                                       const char *desc, const char *url_in,
                                       xmlNsPtr *ppsNsGco) {

  int status;
  const char *link_protocol = "unknown protocol";

  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST "onLine");
  xmlNodePtr psORNode =
      xmlNewChild(psNode, namespace, BAD_CAST "CI_OnlineResource", NULL);

  char *url = msStrdup(url_in);

  if (strcasecmp(service, "M") == 0) {
    url = _stringConcatenateAndEncodeHTML(
        url, "service=WMS&version=1.3.0&request=GetMap&width=500&height=300&"
             "styles=&layers=");
    url = _stringConcatenateAndEncodeHTML(url, layer->name);
    url = _stringConcatenateAndEncodeHTML(url, "&format=");
    url = _stringConcatenateAndEncodeHTML(url, format);
    url = _stringConcatenateAndEncodeHTML(url, "&crs=");
    {
      char *epsg_str = NULL;
      msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "MFCSGO",
                       MS_TRUE, &epsg_str);
      url = _stringConcatenateAndEncodeHTML(url, epsg_str);
      msFree(epsg_str);
    }
    link_protocol = "WWW:DOWNLOAD-1.0-http-get-map";

    rectObj rect;
    status = msLayerGetExtent(layer, &rect);

    if (status == 0) {
      char buffer[32];
      url = _stringConcatenateAndEncodeHTML(url, "&bbox=");
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
  } else if (strcasecmp(service, "F") == 0) {
    link_protocol = "WWW:DOWNLOAD-1.0-http--download";
    url = _stringConcatenateAndEncodeHTML(
        url, "service=WFS&version=1.1.0&request=GetFeature&typename=");
    url = _stringConcatenateAndEncodeHTML(url, layer->name);
    url = _stringConcatenateAndEncodeHTML(url, "&outputformat=");
    url = _stringConcatenateAndEncodeHTML(url, format);
  } else if (strcasecmp(service, "C") == 0) {
    link_protocol = "WWW:DOWNLOAD-1.0-http--download";
    url = _stringConcatenateAndEncodeHTML(
        url, "service=WCS&version=2.0.1&request=GetCoverage&coverageid=");
    url = _stringConcatenateAndEncodeHTML(url, layer->name);
    url = _stringConcatenateAndEncodeHTML(url, "&format=");
    url = _stringConcatenateAndEncodeHTML(url, format);
  }

  xmlAddChild(psORNode, _msMetadataGetURL(namespace, "linkage", url, ppsNsGco));
  msFree(url);

  xmlAddChild(psORNode, _msMetadataGetCharacterString(namespace, "protocol",
                                                      link_protocol, ppsNsGco));
  xmlAddChild(psORNode, _msMetadataGetCharacterString(namespace, "name",
                                                      layer->name, ppsNsGco));

  xmlAddChild(psORNode, _msMetadataGetCharacterString(namespace, "description",
                                                      desc, ppsNsGco));

  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetInteger                              */
/*                                                                      */
/*      Create a gmd:name/gmd:Integer element pattern                   */
/************************************************************************/

static xmlNodePtr _msMetadataGetInteger(xmlNsPtr namespace, const char *name,
                                        int value, xmlNsPtr *ppsNsGco) {
  char buffer[8];
  sprintf(buffer, "%d", value);

  if (*ppsNsGco == NULL)
    *ppsNsGco = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd",
                         BAD_CAST "gco");

  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST name);

  if (!value)
    xmlNewNsProp(psNode, *ppsNsGco, BAD_CAST "nilReason", BAD_CAST "missing");
  else
    xmlNewChild(psNode, *ppsNsGco, BAD_CAST "Integer", BAD_CAST buffer);
  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetDecimal                              */
/*                                                                      */
/*      Create a gmd:name/gmd:Decimal element pattern                   */
/************************************************************************/

static xmlNodePtr _msMetadataGetDecimal(xmlNsPtr namespace, const char *name,
                                        double value, xmlNsPtr *ppsNsGco) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.6f", value);

  if (*ppsNsGco == NULL)
    *ppsNsGco = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd",
                         BAD_CAST "gco");

  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST name);

  if (!value)
    xmlNewNsProp(psNode, *ppsNsGco, BAD_CAST "nilReason", BAD_CAST "missing");
  else
    xmlNewChild(psNode, *ppsNsGco, BAD_CAST "Decimal", BAD_CAST buffer);
  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetCodeList                             */
/*                                                                      */
/*      Create a gmd:name/gmd:* code list element pattern               */
/************************************************************************/

xmlNodePtr _msMetadataGetCodeList(xmlNsPtr namespace,
                                  const char *parent_element, const char *name,
                                  const char *value) {
  char *codelist = NULL;
  codelist = msStrdup(
      "http://www.isotc211.org/2005/resources/Codelist/gmxCodelists.xml#");
  codelist = msStringConcatenate(codelist, name);

  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST parent_element);
  xmlNodePtr psCodeNode =
      xmlNewChild(psNode, namespace, BAD_CAST name, BAD_CAST value);
  xmlNewProp(psCodeNode, BAD_CAST "codeSpace", BAD_CAST "ISOTC211/19115");
  xmlNewProp(psCodeNode, BAD_CAST "codeList", BAD_CAST codelist);
  xmlNewProp(psCodeNode, BAD_CAST "codeListValue", BAD_CAST value);
  msFree(codelist);
  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetGMLTimePeriod                        */
/*                                                                      */
/*      Create a gml:TimePeriod element pattern                         */
/************************************************************************/

static xmlNodePtr _msMetadataGetGMLTimePeriod(char **temporal) {
  xmlNsPtr psNsGml =
      xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml");
  xmlNodePtr psNode = xmlNewNode(psNsGml, BAD_CAST "TimePeriod");
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

static xmlNodePtr _msMetadataGetExtent(xmlNsPtr namespace, layerObj *layer,
                                       xmlNsPtr *ppsNsGco) {
  int n = 0;
  int status;
  char **temporal = NULL;

  rectObj rect;

  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST "extent");
  xmlNodePtr psEXNode =
      xmlNewChild(psNode, namespace, BAD_CAST "EX_Extent", NULL);

  /* scan for geospatial extent */
  status = msLayerGetExtent(layer, &rect);

  if (status == 0) {
    /* always project to lat long */
    msOWSProjectToWGS84(&layer->projection, &rect);

    xmlNodePtr psGNode =
        xmlNewChild(psEXNode, namespace, BAD_CAST "geographicElement", NULL);
    xmlNodePtr psGNode2 = xmlNewChild(
        psGNode, namespace, BAD_CAST "EX_GeographicBoundingBox", NULL);
    xmlAddChild(psGNode2, _msMetadataGetDecimal(namespace, "westBoundLongitude",
                                                rect.minx, ppsNsGco));
    xmlAddChild(psGNode2, _msMetadataGetDecimal(namespace, "eastBoundLongitude",
                                                rect.maxx, ppsNsGco));
    xmlAddChild(psGNode2, _msMetadataGetDecimal(namespace, "southBoundLatitude",
                                                rect.miny, ppsNsGco));
    xmlAddChild(psGNode2, _msMetadataGetDecimal(namespace, "northBoundLatitude",
                                                rect.maxy, ppsNsGco));
  }

  /* scan for temporal extent */
  const char *value =
      msOWSLookupMetadata(&(layer->metadata), "MO", "timeextent");
  if (value) { /* WMS */
    temporal = msStringSplit(value, '/', &n);
  } else { /* WCS */
    value = msOWSLookupMetadata(&(layer->metadata), "CO", "timeposition");
    if (value) {
      /* split extent */
      temporal = msStringSplit(value, ',', &n);
    }
  }
  if (!value) { /* SOS */
    value =
        msOWSLookupMetadata(&(layer->metadata), "SO", "offering_timeextent");
    if (value)
      temporal = msStringSplit(value, '/', &n);
  }
  if (value) {
    if (temporal && n > 0) {
      xmlNodePtr psTNode =
          xmlNewChild(psEXNode, namespace, BAD_CAST "temporalElement", NULL);
      xmlNodePtr psTNode2 =
          xmlNewChild(psTNode, namespace, BAD_CAST "EX_TemporalExtent", NULL);
      xmlNodePtr psENode =
          xmlNewChild(psTNode2, namespace, BAD_CAST "extent", NULL);
      xmlAddChild(psENode, _msMetadataGetGMLTimePeriod(temporal));
    }
  }
  msFreeCharArray(temporal, n);

  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetReferenceSystemInfo                  */
/*                                                                      */
/*      Create a gmd:referenceSystemInfo element pattern                */
/************************************************************************/

static xmlNodePtr _msMetadataGetReferenceSystemInfo(xmlNsPtr namespace,
                                                    layerObj *layer,
                                                    xmlNsPtr *ppsNsGco) {
  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST "referenceSystemInfo");
  xmlNodePtr psRSNode =
      xmlNewChild(psNode, namespace, BAD_CAST "MD_ReferenceSystem", NULL);
  xmlNodePtr psRSINode = xmlNewChild(
      psRSNode, namespace, BAD_CAST "referenceSystemIdentifier", NULL);
  xmlNodePtr psRSINode2 =
      xmlNewChild(psRSINode, namespace, BAD_CAST "RS_Identifier", NULL);

  char *epsg_str = NULL;
  msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "MFCSGO", MS_TRUE,
                   &epsg_str);
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(namespace, "code",
                                                        epsg_str, ppsNsGco));
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(
                              namespace, "codeSpace",
                              "http://www.epsg-registry.org", ppsNsGco));
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(namespace, "version",
                                                        "6.14", ppsNsGco));
  msFree(epsg_str);

  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetContact                              */
/*                                                                      */
/*      Create a gmd:contact element pattern                            */
/************************************************************************/

xmlNodePtr _msMetadataGetContact(xmlNsPtr namespace, char *contact_element,
                                 mapObj *map, xmlNsPtr *ppsNsGco) {
  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST contact_element);
  xmlNodePtr psCNode =
      xmlNewChild(psNode, namespace, BAD_CAST "CI_ResponsibleParty", NULL);
  xmlNewProp(psCNode, BAD_CAST "id", BAD_CAST contact_element);

  const char *value =
      msOWSLookupMetadata(&(map->web.metadata), "MCFO", "contactperson");
  if (value)
    xmlAddChild(psCNode, _msMetadataGetCharacterString(
                             namespace, "individualName", value, ppsNsGco));

  value =
      msOWSLookupMetadata(&(map->web.metadata), "MCFO", "contactorganization");
  if (value)
    xmlAddChild(psCNode, _msMetadataGetCharacterString(
                             namespace, "organisationName", value, ppsNsGco));

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO", "contactposition");
  if (value)
    xmlAddChild(psCNode, _msMetadataGetCharacterString(
                             namespace, "positionName", value, ppsNsGco));

  xmlNodePtr psCINode =
      xmlNewChild(psCNode, namespace, BAD_CAST "contactInfo", NULL);
  xmlNodePtr psCINode2 =
      xmlNewChild(psCINode, namespace, BAD_CAST "CI_Contact", NULL);
  xmlNodePtr psPhoneNode =
      xmlNewChild(psCINode2, namespace, BAD_CAST "phone", NULL);
  xmlNodePtr psCIPhoneNode =
      xmlNewChild(psPhoneNode, namespace, BAD_CAST "CI_Telephone", NULL);

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO",
                              "contactvoicetelephone");
  if (value)
    xmlAddChild(psCIPhoneNode, _msMetadataGetCharacterString(namespace, "voice",
                                                             value, ppsNsGco));

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO",
                              "contactfacsimiletelephone");
  if (value)
    xmlAddChild(psCIPhoneNode, _msMetadataGetCharacterString(
                                   namespace, "facsimile", value, ppsNsGco));

  xmlNodePtr psAddressNode =
      xmlNewChild(psCINode2, namespace, BAD_CAST "address", NULL);
  xmlNodePtr psCIAddressNode =
      xmlNewChild(psAddressNode, namespace, BAD_CAST "CI_Address", NULL);

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO", "address");
  if (value)
    xmlAddChild(psCIAddressNode,
                _msMetadataGetCharacterString(namespace, "deliveryPoint", value,
                                              ppsNsGco));

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO", "city");
  if (value)
    xmlAddChild(psCIAddressNode, _msMetadataGetCharacterString(
                                     namespace, "city", value, ppsNsGco));

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO", "stateorprovince");
  if (value)
    xmlAddChild(psCIAddressNode,
                _msMetadataGetCharacterString(namespace, "administrativeArea",
                                              value, ppsNsGco));

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO", "postcode");
  if (value)
    xmlAddChild(psCIAddressNode, _msMetadataGetCharacterString(
                                     namespace, "postalCode", value, ppsNsGco));

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO", "country");
  if (value)
    xmlAddChild(psCIAddressNode, _msMetadataGetCharacterString(
                                     namespace, "country", value, ppsNsGco));

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO",
                              "contactelectronicmailaddress");
  if (value)
    xmlAddChild(psCIAddressNode,
                _msMetadataGetCharacterString(
                    namespace, "electronicMailAddress", value, ppsNsGco));

  value = msOWSLookupMetadata(&(map->web.metadata), "MCFO", "onlineresource");
  if (value) {
    xmlNodePtr psORNode =
        xmlNewChild(psCINode2, namespace, BAD_CAST "onlineResource", NULL);
    xmlNodePtr psORNode2 =
        xmlNewChild(psORNode, namespace, BAD_CAST "CI_OnlineResource", NULL);
    xmlAddChild(psORNode2,
                _msMetadataGetURL(namespace, "linkage", value, ppsNsGco));
  }

  xmlAddChild(psCNode, _msMetadataGetCodeList(namespace, "role", "CI_RoleCode",
                                              "pointOfContact"));

  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetIdentificationInfo                   */
/*                                                                      */
/*      Create a gmd:identificationInfo element pattern                 */
/************************************************************************/

static xmlNodePtr _msMetadataGetIdentificationInfo(xmlNsPtr namespace,
                                                   mapObj *map, layerObj *layer,
                                                   xmlNsPtr *ppsNsGco) {
  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST "identificationInfo");

  xmlNodePtr psDINode =
      xmlNewChild(psNode, namespace, BAD_CAST "MD_DataIdentification", NULL);
  xmlNewProp(psDINode, BAD_CAST "id", BAD_CAST layer->name);
  xmlNodePtr psCNode =
      xmlNewChild(psDINode, namespace, BAD_CAST "citation", NULL);
  xmlNodePtr psCINode =
      xmlNewChild(psCNode, namespace, BAD_CAST "CI_Citation", NULL);

  const char *value = msOWSLookupMetadata(&(layer->metadata), "MCFGO", "title");
  if (!value)
    value = msOWSLookupMetadata(&(layer->metadata), "S", "offering_name");
  xmlAddChild(psCINode, _msMetadataGetCharacterString(namespace, "title", value,
                                                      ppsNsGco));

  xmlNodePtr psDNode = xmlNewChild(psCINode, namespace, BAD_CAST "date", NULL);
  xmlNewNsProp(psDNode, *ppsNsGco, BAD_CAST "nilReason", BAD_CAST "missing");

  value = msOWSLookupMetadata(&(layer->metadata), "MCFGO", "attribution_title");
  if (value) {
    xmlAddChild(psCINode,
                _msMetadataGetCharacterString(namespace, "otherCitationDetails",
                                              value, ppsNsGco));
  }

  value = msOWSLookupMetadata(&(layer->metadata), "MCFGO", "abstract");
  if (!value)
    value =
        msOWSLookupMetadata(&(layer->metadata), "S", "offering_description");
  xmlAddChild(psDINode, _msMetadataGetCharacterString(namespace, "abstract",
                                                      value, ppsNsGco));

  value = msOWSLookupMetadata(&(layer->metadata), "MCFSGO", "keywordlist");

  if (value) {
    xmlNodePtr psKWNode =
        xmlNewChild(psDINode, namespace, BAD_CAST "descriptiveKeywords", NULL);
    xmlNodePtr psMDKNode =
        xmlNewChild(psKWNode, namespace, BAD_CAST "MD_Keywords", NULL);

    int n = 0;
    char **tokens = msStringSplit(value, ',', &n);
    if (tokens && n > 0) {
      for (int i = 0; i < n; i++) {
        xmlAddChild(psMDKNode, _msMetadataGetCharacterString(
                                   namespace, "keyword", tokens[i], ppsNsGco));
      }
    }
    msFreeCharArray(tokens, n);
  }

  xmlAddChild(psDINode,
              _msMetadataGetCharacterString(
                  namespace, "language",
                  (char *)msOWSGetLanguage(map, "exception"), ppsNsGco));
  xmlAddChild(psDINode, _msMetadataGetExtent(namespace, layer, ppsNsGco));

  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetSpatialRepresentationInfo            */
/*                                                                      */
/*      Create a gmd:spatialRepresentationInfo element pattern          */
/************************************************************************/

static xmlNodePtr _msMetadataGetSpatialRepresentationInfo(xmlNsPtr namespace,
                                                          layerObj *layer,
                                                          xmlNsPtr *ppsNsGco) {
  xmlNodePtr psNode =
      xmlNewNode(namespace, BAD_CAST "spatialRepresentationInfo");

  if (layer->type == MS_LAYER_RASTER) {
    xmlNodePtr psSRNode = xmlNewChild(
        psNode, namespace, BAD_CAST "MD_GridSpatialRepresentation", NULL);
    xmlAddChild(psSRNode, _msMetadataGetInteger(namespace, "numberOfDimensions",
                                                2, ppsNsGco));
    xmlAddChild(psSRNode,
                _msMetadataGetCodeList(namespace, "cellGeometry",
                                       "MD_CellGeometryCode", "area"));
  } else {
    xmlNodePtr psSRNode = xmlNewChild(
        psNode, namespace, BAD_CAST "MD_VectorSpatialRepresentation", NULL);
    xmlAddChild(psSRNode,
                _msMetadataGetCodeList(namespace, "topologyLevel",
                                       "MD_TopologyLevelCode", "geometryOnly"));
    xmlNodePtr psGONode =
        xmlNewChild(psSRNode, namespace, BAD_CAST "geometricObjects", NULL);
    xmlNodePtr psGONode2 =
        xmlNewChild(psGONode, namespace, BAD_CAST "MD_GeometricObjects", NULL);
    const char *value;
    if (layer->type == MS_LAYER_POINT)
      value = "point";
    else if (layer->type == MS_LAYER_LINE)
      value = "curve";
    else if (layer->type == MS_LAYER_POLYGON)
      value = "surface";
    else
      value = "complex";
    xmlAddChild(psGONode2,
                _msMetadataGetCodeList(namespace, "geometricObjectType",
                                       "MD_GeometricObjectTypeCode", value));
    // TODO: find way to get feature count in a fast way
    /* xmlAddChild(psGONode2, _msMetadataGetInteger(namespace,
     * "geometricObjectCount", msLayerGetNumFeatures(layer))); */
  }

  return psNode;
}

/************************************************************************/
/*                   _msMetadataGetDistributionInfo                     */
/*                                                                      */
/*      Create a gmd:identificationInfo element pattern                 */
/************************************************************************/

static xmlNodePtr _msMetadataGetDistributionInfo(xmlNsPtr namespace,
                                                 mapObj *map, layerObj *layer,
                                                 cgiRequestObj *cgi_request,
                                                 xmlNsPtr *ppsNsGco) {
  char *url = NULL;

  xmlNodePtr psNode = xmlNewNode(namespace, BAD_CAST "distributionInfo");
  xmlNodePtr psMDNode =
      xmlNewChild(psNode, namespace, BAD_CAST "MD_Distribution", NULL);

  {
    char *pszTmp =
        msOWSGetOnlineResource(map, "MFCSGO", "onlineresource", cgi_request);
    url = msEncodeHTMLEntities(pszTmp);
    msFree(pszTmp);
  }

  /* gmd:distributor */
  xmlNodePtr psDNode =
      xmlNewChild(psMDNode, namespace, BAD_CAST "distributor", NULL);
  xmlNodePtr psDNode2 =
      xmlNewChild(psDNode, namespace, BAD_CAST "MD_Distributor", NULL);
  xmlAddChild(psDNode2, _msMetadataGetContact(namespace, "distributorContact",
                                              map, ppsNsGco));

  /* gmd:transferOptions */
  xmlNodePtr psTONode =
      xmlNewChild(psMDNode, namespace, BAD_CAST "transferOptions", NULL);
  xmlNodePtr psDTONode = xmlNewChild(
      psTONode, namespace, BAD_CAST "MD_DigitalTransferOptions", NULL);
  xmlAddChild(psDTONode, _msMetadataGetCharacterString(
                             namespace, "unitsOfDistribution", "KB", ppsNsGco));

  /* links */

  /* WMS */
  xmlAddChild(psDTONode,
              _msMetadataGetOnline(namespace, layer, "M", "image/png",
                                   "PNG Format", url, ppsNsGco));
  xmlAddChild(psDTONode,
              _msMetadataGetOnline(namespace, layer, "M", "image/jpeg",
                                   "JPEG Format", url, ppsNsGco));

  /* WCS */
  if (layer->type == MS_LAYER_RASTER) {
    xmlAddChild(psDTONode,
                _msMetadataGetOnline(namespace, layer, "C", "image/tiff",
                                     "GeoTIFF Format", url, ppsNsGco));
  }
  /* WFS */
  else {
    xmlAddChild(psDTONode, _msMetadataGetOnline(namespace, layer, "F", "GML2",
                                                "GML2 Format", url, ppsNsGco));
    xmlAddChild(psDTONode, _msMetadataGetOnline(namespace, layer, "F", "GML3",
                                                "GML3 Format", url, ppsNsGco));
  }

  msFree(url);

  return psNode;
}

/************************************************************************/
/*                   msMetadataGetExceptionReport                       */
/*                                                                      */
/*              Generate an OWS Common Exception Report                 */
/************************************************************************/

static xmlNodePtr msMetadataGetExceptionReport(mapObj *map, char *code,
                                               char *locator, char *message,
                                               xmlNsPtr *ppsNsOws) {
  char *schemas_location = NULL;

  xmlNodePtr psRootNode = NULL;

  schemas_location = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
  *ppsNsOws =
      xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");

  psRootNode = msOWSCommonExceptionReport(
      *ppsNsOws, OWS_1_1_0, schemas_location, "1.1.0",
      msOWSGetLanguage(map, "exception"), code, locator, message);
  msFree(schemas_location);

  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/ows/1.1",
           BAD_CAST "ows");

  return psRootNode;
}

/************************************************************************/
/*                   msMetadataGetLayerMetadata                         */
/*                                                                      */
/*      Generate an ISO 19139-1:2019 representation of layer metadata   */
/************************************************************************/

static xmlNodePtr
msMetadataGetLayerMetadata(mapObj *map, metadataParamsObj *paramsObj,
                           cgiRequestObj *cgi_request, xmlNsPtr *ppsNsOws,
                           xmlNsPtr *ppsNsXsi, xmlNsPtr *ppsNsGmd,
                           xmlNsPtr *ppsNsGco) {
  int i;
  int layer_found = MS_FALSE;

  xmlNodePtr psRootNode = NULL;

  layerObj *layer = NULL;

  /* Check that layer requested exists in mapfile */
  for (i = 0; i < map->numlayers; i++) {
    if (strcasecmp(GET_LAYER(map, i)->name, paramsObj->pszLayer) == 0) {
      layer_found = MS_TRUE;
      layer = GET_LAYER(map, i);
      // when checking a layer with clustering msLayerGetExtent does not have
      // access to the source layer, so remove clustering first
      if (layer->cluster.region) {
        layer->cluster.region = NULL;
      }
      break;
    }
  }

  if (layer_found == MS_FALSE) {
    psRootNode = msMetadataGetExceptionReport(
        map, "InvalidParameterValue", "layer", "Layer not found", ppsNsOws);
  }

  /* Check that outputschema is valid */
  else if (paramsObj->pszOutputSchema &&
           strcasecmp(paramsObj->pszOutputSchema,
                      "http://www.isotc211.org/2005/gmd") != 0) {
    psRootNode = msMetadataGetExceptionReport(
        map, "InvalidParameterValue", "outputschema",
        "OUTPUTSCHEMA must be \"http://www.isotc211.org/2005/gmd\"", ppsNsOws);
  }

  else {
    *ppsNsXsi =
        xmlNewNs(NULL, BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",
                 BAD_CAST "xsi");
    *ppsNsGmd = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd",
                         BAD_CAST "gmd");
    /* root element */
    psRootNode = xmlNewNode(NULL, BAD_CAST "MD_Metadata");
    xmlNewNs(psRootNode, BAD_CAST "http://www.isotc211.org/2005/gmd",
             BAD_CAST "gmd");
    xmlNewNs(psRootNode, BAD_CAST "http://www.isotc211.org/2005/gco",
             BAD_CAST "gco");
    xmlNewNs(psRootNode, BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",
             BAD_CAST "xsi");
    xmlSetNs(psRootNode, *ppsNsGmd);

    xmlNewNsProp(psRootNode, *ppsNsXsi, BAD_CAST "schemaLocation",
                 BAD_CAST "http://www.isotc211.org/2005/gmd "
                          "http://www.isotc211.org/2005/gmd/gmd.xsd");

    /* gmd:identifier */
    xmlAddChild(psRootNode,
                _msMetadataGetCharacterString(*ppsNsGmd, "fileIdentifier",
                                              layer->name, ppsNsGco));

    /* gmd:language */
    xmlAddChild(psRootNode,
                _msMetadataGetCharacterString(
                    *ppsNsGmd, "language",
                    (char *)msOWSGetLanguage(map, "exception"), ppsNsGco));

    /* gmd:hierarchyLevel */
    xmlAddChild(psRootNode, _msMetadataGetCodeList(*ppsNsGmd, "hierarchyLevel",
                                                   "MD_ScopeCode", "dataset"));

    /* gmd:contact */
    xmlAddChild(psRootNode,
                _msMetadataGetContact(*ppsNsGmd, "contact", map, ppsNsGco));

    /* gmd:dateStamp */
    /* TODO: nil for now, find way to derive this automagically */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(
                                *ppsNsGmd, "dateStamp", NULL, ppsNsGco));

    /* gmd:metadataStandardName */
    xmlAddChild(psRootNode,
                _msMetadataGetCharacterString(
                    *ppsNsGmd, "metadataStandardName",
                    "ISO 19115:2003 - Geographic information - Metadata",
                    ppsNsGco));

    /* gmd:metadataStandardVersion */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(
                                *ppsNsGmd, "metadataStandardVersion",
                                "ISO 19115:2003", ppsNsGco));

    /* gmd:spatialRepresentationInfo */
    xmlAddChild(psRootNode, _msMetadataGetSpatialRepresentationInfo(
                                *ppsNsGmd, layer, ppsNsGco));

    /* gmd:referenceSystemInfo */
    xmlAddChild(psRootNode,
                _msMetadataGetReferenceSystemInfo(*ppsNsGmd, layer, ppsNsGco));

    /* gmd:identificationInfo */
    xmlAddChild(psRootNode, _msMetadataGetIdentificationInfo(*ppsNsGmd, map,
                                                             layer, ppsNsGco));

    /* gmd:distributionInfo */
    xmlAddChild(psRootNode, _msMetadataGetDistributionInfo(
                                *ppsNsGmd, map, layer, cgi_request, ppsNsGco));
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

int msMetadataDispatch(mapObj *map, cgiRequestObj *cgi_request) {
  int i;
  int status = MS_SUCCESS;
  xmlNodePtr psRootNode = NULL;
  xmlDocPtr xml_document;
  metadataParamsObj *paramsObj;
  layerObj *layer = NULL;
  xmlNsPtr psNsOws = NULL;
  xmlNsPtr psNsXsi = NULL;
  xmlNsPtr psNsGmd = NULL;
  xmlNsPtr psNsGco = NULL;

  /* Populate the Params object based on the request */

  paramsObj = msMetadataCreateParamsObj();

  xml_document = xmlNewDoc(BAD_CAST "1.0");

  if (msMetadataParseRequest(cgi_request, paramsObj) == MS_FAILURE) {
    psRootNode = msMetadataGetExceptionReport(
        map, "InvalidRequest", "layer", "Request parsing failed", &psNsOws);
    status = MS_FAILURE;
  }

  /* if layer= is not specified, */
  if (paramsObj->pszLayer == NULL || strlen(paramsObj->pszLayer) == 0) {
    psRootNode =
        msMetadataGetExceptionReport(map, "MissingParameterValue", "layer",
                                     "Missing layer parameter", &psNsOws);
    status = MS_FAILURE;
  }

  if (status == MS_SUCCESS) { /* Start dispatching request */
    /* Check that layer requested exists in mapfile */
    for (i = 0; i < map->numlayers; i++) {
      if (strcasecmp(GET_LAYER(map, i)->name, paramsObj->pszLayer) == 0) {
        layer = GET_LAYER(map, i);
        break;
      }
    }
    if (layer != NULL &&
        msOWSLookupMetadata(&(layer->metadata), "MFCO", "metadataurl_href")) {
      msIO_setHeader("Status", "301 Moved Permanently");
      msIO_setHeader(
          "Location", "%s",
          msOWSLookupMetadata(&(layer->metadata), "MFCO", "metadataurl_href"));
      msIO_sendHeaders();
    } else {
      psRootNode = msMetadataGetLayerMetadata(
          map, paramsObj, cgi_request, &psNsOws, &psNsXsi, &psNsGmd, &psNsGco);
    }
  }

  if (psRootNode != NULL) {
    xmlChar *xml_buffer = NULL;
    int buffersize = 0;
    xmlDocSetRootElement(xml_document, psRootNode);

    msIO_setHeader("Content-type", "text/xml; charset=UTF-8");
    msIO_sendHeaders();

    msIOContext *context = NULL;

    context = msIO_getHandler(stdout);

    xmlDocDumpFormatMemoryEnc(xml_document, &xml_buffer, &buffersize, "UTF-8",
                              1);
    msIO_contextWrite(context, xml_buffer, buffersize);

    xmlFree(xml_buffer);
  }
  xmlFreeDoc(xml_document);
  if (psNsOws)
    xmlFreeNs(psNsOws);
  if (psNsXsi)
    xmlFreeNs(psNsXsi);
  if (psNsGmd)
    xmlFreeNs(psNsGmd);
  if (psNsGco)
    xmlFreeNs(psNsGco);

  msMetadataFreeParamsObj(paramsObj);

  return status;
}

#endif /* USE_LIBXML2 */

/************************************************************************/
/*                           msMetadataCreateParamsObj                  */
/*                                                                      */
/*   Create a parameter object, initialize it.                          */
/*   The caller should free the object using msMetadataFreeParamsObj.   */
/************************************************************************/

metadataParamsObj *msMetadataCreateParamsObj() {
  metadataParamsObj *paramsObj =
      (metadataParamsObj *)calloc(1, sizeof(metadataParamsObj));
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

void msMetadataFreeParamsObj(metadataParamsObj *metadataparams) {
  if (metadataparams) {
    free(metadataparams->pszRequest);
    free(metadataparams->pszLayer);
    free(metadataparams->pszOutputSchema);
    free(metadataparams);
  }
}

/************************************************************************/
/*                            msMetadataParseRequest                    */
/*                                                                      */
/*      Parse request into the params object.                           */
/************************************************************************/

static int msMetadataParseRequest(cgiRequestObj *request,
                                  metadataParamsObj *metadataparams) {
  if (!request || !metadataparams)
    return MS_FAILURE;

  if (request->NumParams > 0) {
    for (int i = 0; i < request->NumParams; i++) {
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

void msMetadataSetGetMetadataURL(layerObj *lp, const char *url) {
  char *pszMetadataURL = NULL;

  pszMetadataURL = msStrdup(url);
  msDecodeHTMLEntities(pszMetadataURL);
  pszMetadataURL =
      msStringConcatenate(pszMetadataURL, "request=GetMetadata&layer=");
  pszMetadataURL = msStringConcatenate(pszMetadataURL, lp->name);

  msInsertHashTable(&(lp->metadata), "ows_metadataurl_href", pszMetadataURL);
  msInsertHashTable(&(lp->metadata), "ows_metadataurl_type", "TC211");
  msInsertHashTable(&(lp->metadata), "ows_metadataurl_format", "text/xml");
  msInsertHashTable(&(lp->metadata), "ows_metadatalink_href", pszMetadataURL);
  msInsertHashTable(&(lp->metadata), "ows_metadatalink_type", "TC211");
  msInsertHashTable(&(lp->metadata), "ows_metadatalink_format", "text/xml");
  msFree(pszMetadataURL);
}

#endif

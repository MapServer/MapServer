/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Metadata implementation
 * Author:   Tom Kralidis (tomkralidis@hotmail.com)
 *
 **********************************************************************
 * Copyright (c) 2012, Tom Kralidis
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
  /* TODO handle non geographic extents with gmd:EX_BoundingPolygon*/
  status = msLayerGetExtent(layer, &rect);

  if (status == 0) {
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
  char *value;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psRSNode = NULL;
  xmlNodePtr psRSINode = NULL;
  xmlNodePtr psRSINode2 = NULL;

  psNode = xmlNewNode(namespace, BAD_CAST "referenceSystemInfo");
  psRSNode = xmlNewChild(psNode, namespace, BAD_CAST "MD_ReferenceSystem", NULL);
  psRSINode = xmlNewChild(psRSNode, namespace, BAD_CAST "referenceSystemIdentifier", NULL);
  psRSINode2 = xmlNewChild(psRSINode, namespace, BAD_CAST "RS_Identifier", NULL);

  value = (char *)msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "MFCSGO", MS_TRUE);
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(namespace, "code", value));
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(namespace, "codeSpace", "http://www.epsg-registry.org"));
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(namespace, "version", "6.14"));

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

  value = (char *)msOWSLookupMetadata(&(layer->metadata), "MCFSGO", "title");
  xmlAddChild(psCINode, _msMetadataGetCharacterString(namespace, "title", value));

  psDNode = xmlNewChild(psCINode, namespace, BAD_CAST "date", NULL);

  xmlAddChild(psDNode, _msMetadataGetDate(namespace, "CI_Date", "publication", "2011"));

  value = (char *)msOWSLookupMetadata(&(layer->metadata), "MCFSGO", "abstract");
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
    xmlAddChild(psGONode2, _msMetadataGetInteger(namespace, "geometricObjectCount", msLayerGetNumFeatures(layer)));
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
  int status = 0;
  char buffer[32];
  char *url = NULL;
  char *desc = NULL;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psMDNode = NULL;
  xmlNodePtr psTONode = NULL;
  xmlNodePtr psDTONode = NULL;
  xmlNodePtr psOLNode = NULL;
  xmlNodePtr psORNode = NULL;
  rectObj rect;

  psNode = xmlNewNode(namespace, BAD_CAST "distributionInfo");
  psMDNode = xmlNewChild(psNode, namespace, BAD_CAST "MD_Distribution", NULL);

  /* TODO gmd:distributor */

  /* gmd:transferOptions */
  psTONode = xmlNewChild(psMDNode, namespace, BAD_CAST "transferOptions", NULL);
  psDTONode = xmlNewChild(psTONode, namespace, BAD_CAST "MD_DigitalTransferOptions", NULL);
  xmlAddChild(psDTONode, _msMetadataGetCharacterString(namespace, "unitsOfDistribution", "KB"));

  /* links */

  /* WMS */
  psOLNode = xmlNewChild(psDTONode, namespace, BAD_CAST "onLine", NULL);
  psORNode = xmlNewChild(psOLNode, namespace, BAD_CAST "CI_OnlineResource", NULL);

  url = msEncodeHTMLEntities(msOWSGetOnlineResource(map, "MFCSGO", "onlineresource", cgi_request));

  url = msStringConcatenate(url, msEncodeHTMLEntities("service=WMS&version=1.1.1&request=GetMap&width=500&height=300&format=image/png&styles=&layers="));
  url = msStringConcatenate(url, msEncodeHTMLEntities(layer->name));
  url = msStringConcatenate(url, msEncodeHTMLEntities("&srs="));
  url = msStringConcatenate(url, msEncodeHTMLEntities(msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "MFCSGO", MS_TRUE)));

  status = msLayerGetExtent(layer, &rect);

  if (status == 0) {
      url = msStringConcatenate(url, msEncodeHTMLEntities("&bbox="));
      sprintf(buffer, "%f", rect.minx);
      url = msStringConcatenate(url, buffer);
      url = msStringConcatenate(url, ",");
      sprintf(buffer, "%f", rect.miny);
      url = msStringConcatenate(url, buffer);
      url = msStringConcatenate(url, ",");
      sprintf(buffer, "%f", rect.maxx);
      url = msStringConcatenate(url, buffer);
      url = msStringConcatenate(url, ",");
      sprintf(buffer, "%f", rect.maxy);
      url = msStringConcatenate(url, buffer);
  }

  xmlAddChild(psORNode, _msMetadataGetURL(namespace, "linkage", url));

  /* add layers=, bbox=, srs=, styles=, */

  xmlAddChild(psORNode, _msMetadataGetCharacterString(namespace, "protocol", "WWW:DOWNLOAD-1.0-http--download"));
  xmlAddChild(psORNode, _msMetadataGetCharacterString(namespace, "name", layer->name));

  desc = (char *)msOWSLookupMetadata(&(layer->metadata), "MFCSGO", "title");
  desc = msStringConcatenate(desc, " (PNG Format)");

  if (desc)
    xmlAddChild(psORNode, _msMetadataGetCharacterString(namespace, "description", desc));

  return psNode;
}


/************************************************************************/
/*                   msMetadataGetServiceMetadata                       */
/*                                                                      */
/*      Generate an ISO 19119:2005 representation of service metadata   */
/************************************************************************/

int msMetadataGetServiceMetadata(mapObj *map, cgiRequestObj *cgi_request)
{
  msIO_setHeader("Content-type","text/xml");
  msIO_sendHeaders();
  msIO_printf("<service/>");
  return MS_SUCCESS;
}


/************************************************************************/
/*                   msMetadataGetLayerMetadata                         */
/*                                                                      */
/*      Generate an ISO 19139:2007 representation of service metadata   */
/************************************************************************/

int msMetadataGetLayerMetadata(mapObj *map, metadataParamsObj *paramsObj, cgiRequestObj *cgi_request, owsRequestObj *ows_request)
{
  int i;
  int buffersize;
  int layer_found = MS_FALSE;
  int result = MS_SUCCESS;

  char *schemas_location = NULL;

  xmlNodePtr psRootNode = NULL;
  xmlDocPtr xml_document;
  xmlChar *xml_buffer;
  xmlNsPtr psNsOws = NULL;
  xmlNsPtr psNsGmd = NULL;
  xmlNsPtr psNsXsi = NULL;

  layerObj *layer = NULL;

  msIO_setHeader("Content-type","text/xml");
  msIO_sendHeaders();

  xml_document = xmlNewDoc(BAD_CAST "1.0");
  schemas_location = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

  psNsOws = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");
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
    psRootNode = msOWSCommonExceptionReport(psNsOws,
                                            OWS_1_1_0,
                                            schemas_location,
                                            "1.1.0",
                                            msOWSGetLanguage(map, "exception"),
                                            "InvalidParameterValue",
                                            "layer",
                                            "Layer not found");
    xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");
    result = MS_FAILURE;
  }

  /* Check that outputschema is valid */
  else if (paramsObj->pszOutputSchema && strcasecmp(paramsObj->pszOutputSchema, "http://www.isotc211.org/2005/gmd") != 0) {
    psRootNode = msOWSCommonExceptionReport(psNsOws,
                                            OWS_1_1_0,
                                            schemas_location,
                                            "1.1.0",
                                            msOWSGetLanguage(map, "exception"),
                                            "InvalidParameterValue",
                                            "outputschema",
                                            "OUTPUTSCHEMA must be \"http://www.isotc211.org/2005/gmd\"");
    xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");
    result = MS_FAILURE;
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
    /* TODO: nil for now */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "contact", NULL));

    /* gmd:dateStamp */
    /* TODO: nil for now, find way to derive this automagically */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "dateStamp", NULL));

    /* gmd:metadataStandardName */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "metadataStandardName", "ISO 19115"));

    /* gmd:metadataStandardVersion */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "metadataStandardVersion", "2003"));

    /* gmd:spatialRepresentationInfo */
    xmlAddChild(psRootNode, _msMetadataGetSpatialRepresentationInfo(psNsGmd, layer));

    /* gmd:referenceSystemInfo */
    xmlAddChild(psRootNode, _msMetadataGetReferenceSystemInfo(psNsGmd, layer));

    /* gmd:identificationInfo */
    xmlAddChild(psRootNode, _msMetadataGetIdentificationInfo(psNsGmd, map, layer));

    /* gmd:distributionInfo */
    xmlAddChild(psRootNode, _msMetadataGetDistributionInfo(psNsGmd, map, layer, cgi_request));

    result = MS_SUCCESS;
  }
    
  xmlDocSetRootElement(xml_document, psRootNode);
  xmlDocDumpFormatMemory(xml_document, &xml_buffer, &buffersize, 1);
  msIO_printf((char *) xml_buffer);

  xmlFree(xml_buffer);
  xmlFreeDoc(xml_document);

  return result;
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
  int returnvalue = MS_DONE;
  int svr = MS_FALSE;

  metadataParamsObj *paramsObj;

  /*
  ** Populate the Params object based on the request
  */

  paramsObj = msMetadataCreateParamsObj();

  if (msMetadataParseRequest(map, cgi_request, ows_request, paramsObj) == MS_FAILURE)
    return msWFSException(map, "request", "InvalidRequest", NULL);

  /* if layer= is not specified, */
  if (paramsObj->pszLayer==NULL || strlen(paramsObj->pszLayer)<=0) {
    svr = MS_TRUE;
  }

  /*
  ** Start dispatching requests
  */
  if (svr == MS_TRUE)
    returnvalue = msMetadataGetServiceMetadata(map, cgi_request);
  else
    returnvalue = msMetadataGetLayerMetadata(map, paramsObj, cgi_request, ows_request);

  msMetadataFreeParamsObj(paramsObj);
  free(paramsObj);
  paramsObj = NULL;
  return returnvalue;
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

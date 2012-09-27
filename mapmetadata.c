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
  xmlNewChild(psNode, psNsGco, BAD_CAST "CharacterString", BAD_CAST value);
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
  xmlNewChild(psNode2, NULL, BAD_CAST "Date", BAD_CAST value);
  xmlAddChild(psNode, _msMetadataGetCodeList(namespace, "dateType", "CI_DateTypeCode", date_type));

  return psNode;
}

xmlNodePtr _msMetadataGetReferenceSystemInfo(xmlNsPtr namespace, layerObj *layer)
{
  char *value;
  xmlNodePtr psNode = NULL;
  xmlNodePtr psRSINode = NULL;
  xmlNodePtr psRSINode2 = NULL;

  psNode = xmlNewNode(namespace, BAD_CAST "MD_ReferenceSystem");
  psRSINode = xmlNewChild(psNode, namespace, BAD_CAST "referenceSystemIdentifier", NULL);
  psRSINode2 = xmlNewChild(psRSINode, namespace, BAD_CAST "RS_Identifier", NULL);

  value = (char *)msOWSGetEPSGProj(&(layer->projection), &(layer->metadata), "MO", MS_TRUE);
  xmlAddChild(psRSINode2, _msMetadataGetCharacterString(namespace, "code", value));


  return psNode;
}

int msMetadataGetServiceMetadata(mapObj *map, metadataParamsObj *paramsObj, owsRequestObj *ows_request)
{
  return MS_SUCCESS;
}

int msMetadataGetLayerMetadata(mapObj *map, metadataParamsObj *paramsObj, owsRequestObj *ows_request)
{
  int i;
  int buffersize;
  int layer_found = MS_FALSE;
  int result = MS_SUCCESS;

  char *schemas_location = NULL;

  xmlNodePtr psRootNode = NULL;
  xmlNodePtr psRSNode = NULL;
  xmlDocPtr xml_document;
  xmlChar *xml_buffer;
  xmlNsPtr psNsOws = NULL;
  xmlNsPtr psNsGmd = NULL;

  layerObj *layer = NULL;

  msIO_setHeader("Content-type","text/xml");
  msIO_sendHeaders();

  xml_document = xmlNewDoc(BAD_CAST "1.0");
  schemas_location = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

  for (i=0; i<map->numlayers; i++) {
    if(strcasecmp(GET_LAYER(map, i)->name, paramsObj->pszLayer) == 0) {
        layer_found = MS_TRUE;
        layer = GET_LAYER(map, i);
        break;
    }
  }
 
  if (layer_found == MS_FALSE) {
    psNsOws = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");
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

  else {
    /* root element */
    psNsGmd = xmlNewNs(NULL, BAD_CAST "http://www.isotc211.org/2005/gmd", BAD_CAST "gmd");
    psRootNode = xmlNewNode(NULL, BAD_CAST "MD_Metadata");
    xmlNewNs(psRootNode, BAD_CAST "http://www.isotc211.org/2005/gmd", BAD_CAST "gmd");
    xmlNewNs(psRootNode, BAD_CAST "http://www.isotc211.org/2005/gco", BAD_CAST "gco");
    xmlSetNs(psRootNode, psNsGmd);

    /* gmd:identifier */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "identifier", layer->name));

    /* gmd:language */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "language", (char *)msOWSGetLanguage(map, "exception")));

    /* gmd:hierarchyLevel */
    xmlAddChild(psRootNode, _msMetadataGetCodeList(psNsGmd, "hierarchyLevel", "MD_ScopeCode", "dataset"));

    /* gmd:contact */
    xmlNewChild(psRootNode, psNsGmd, BAD_CAST "contact", NULL);

    /* gmd:dateStamp */
    xmlAddChild(psRootNode, _msMetadataGetDate(psNsGmd, "dateStamp", NULL, "2011"));

    /* gmd:metadataStandardName */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "metadataStandardName", "ISO 19115"));
    /* gmd:metadataStandardVersion */
    xmlAddChild(psRootNode, _msMetadataGetCharacterString(psNsGmd, "metadataStandardVersion", "2003"));

    /* gmd:referenceSystemInfo */
    psRSNode = xmlNewChild(psRootNode, psNsGmd, BAD_CAST "referenceSystemInfo", NULL);
    xmlAddChild(psRSNode, _msMetadataGetReferenceSystemInfo(psNsGmd, layer));

    /* gmd:identificationInfo */
    xmlAddChild(psRootNode, _msMetadataGetDate(psNsGmd, "CI_Date", "publication", "2011"));
  

    /* gmd:distributionInfo */

    /* gmd:dataQualityInfo */

    result = MS_SUCCESS;
  }
    
  xmlDocSetRootElement(xml_document, psRootNode);
  xmlDocDumpFormatMemory(xml_document, &xml_buffer, &buffersize, 1);
  msIO_printf((char *) xml_buffer);

  xmlFree(xml_buffer);
  xmlFreeDoc(xml_document);

  return result;
}

/*
** msMetadataDispatch() is the entry point for metadata requests.
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid WFS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msMetadataDispatch(mapObj *map, cgiRequestObj *requestobj, owsRequestObj *ows_request)
{
  int returnvalue = MS_DONE;
  int svr = MS_FALSE;

  metadataParamsObj *paramsObj;

  /*
  ** Populate the Params object based on the request
  */

  paramsObj = msMetadataCreateParamsObj();

  if (msMetadataParseRequest(map, requestobj, ows_request, paramsObj) == MS_FAILURE)
    return msWFSException(map, "request", "InvalidRequest", NULL);

  /* if layer= is not specified, */
  if (paramsObj->pszLayer==NULL || strlen(paramsObj->pszLayer)<=0) {
    svr = MS_TRUE;
  }

  /*
  ** Start dispatching requests
  */
  if (svr == MS_TRUE)
    returnvalue = msMetadataGetServiceMetadata(map, paramsObj, ows_request);
  else
    returnvalue = msMetadataGetLayerMetadata(map, paramsObj, ows_request);

  msMetadataFreeParamsObj(paramsObj);
  free(paramsObj);
  paramsObj = NULL;
  return returnvalue;

}

/************************************************************************/
/*                           msMetadataCreateParamsObj                  */
/*                                                                      */
/*      Create a parameter object, initialize it.                       */
/*      The caller should free the object using msMetadataFreeParamsObj.*/
/************************************************************************/
metadataParamsObj *msMetadataCreateParamsObj()
{
  metadataParamsObj *paramsObj = (metadataParamsObj *)calloc(1, sizeof(metadataParamsObj));
  MS_CHECK_ALLOC(paramsObj, sizeof(metadataParamsObj), NULL);

  paramsObj->pszLayer = NULL;
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
      }
    }
  }


  return MS_SUCCESS;
}

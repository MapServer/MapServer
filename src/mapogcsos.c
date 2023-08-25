/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC SOS implementation
 * Author:   Y. Assefa, DM Solutions Group (assefa@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2006, Y. Assefa, DM Solutions Group Inc
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
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#define _GNU_SOURCE

#include "mapserver.h"
#include "mapows.h"

#if defined(USE_SOS_SVR) && defined(USE_LIBXML2)

#include "maperror.h"
#include "mapthread.h"
#include "mapows.h"
#include "maptime.h"
#include "mapgml.h"
#include "mapogcfilter.h"

#include "mapowscommon.h"
#include "maplibxml2.h"

#include "libxml/parser.h"
#include "libxml/tree.h"
#include "libxml/xpath.h"
#include "libxml/xpathInternals.h"

const char *pszSOSVersion                = "1.0.0";
const char *pszSOSNamespaceUri           = "http://www.opengis.net/sos/1.0";
const char *pszSOSNamespacePrefix        = "sos";
const char *pszOMNamespaceUri            = "http://www.opengis.net/om/1.0";
const char *pszOMNamespacePrefix         = "om";
const char *pszSOSDescribeSensorMimeType = "text/xml; subtype=\"sensorML/1.0.0\"";
const char *pszSOSGetObservationMimeType = "text/xml; subtype=\"om/1.0.0\"";

typedef struct {
  char *pszProcedure;
  xmlNodePtr psResultNode;
} SOSProcedureNode;

int msSOSParseRequest(mapObj *map, cgiRequestObj *request, sosParamsObj *sosparams);
void msSOSFreeParamsObj(sosParamsObj *sosparams);

/*
** msSOSException()
**
** Report current MapServer error in XML exception format.
** Wrapper function around msOWSCommonExceptionReport. Merely
** passes SOS specific info.
**
*/

static int msSOSException(mapObj *map, char *locator, char *exceptionCode)
{
  int size = 0;
  char *errorString     = NULL;
  char *schemasLocation = NULL;

  xmlDocPtr  psDoc      = NULL;
  xmlNodePtr psRootNode = NULL;
  xmlNsPtr   psNsOws    = NULL;
  xmlChar *buffer       = NULL;

  psNsOws = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");

  errorString = msGetErrorString("\n");
  schemasLocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = msOWSCommonExceptionReport(psNsOws, OWS_1_1_0, schemasLocation, pszSOSVersion, msOWSGetLanguage(map, "exception"), exceptionCode, locator, errorString);

  xmlDocSetRootElement(psDoc, psRootNode);

  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");

  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, ("UTF-8"), 1);

  msIO_printf("%s", buffer);

  /*free buffer and the document */
  free(errorString);
  free(schemasLocation);
  xmlFree(buffer);
  xmlFreeDoc(psDoc);
  xmlFreeNs(psNsOws);

  /*
  ** The typical pattern is to call msSOSException() right after
  ** msSetError().  In order to prevent mapserv.c from re-reporting this
  ** error at a higher level, we mark it as reported here. #3571
  */
  {
    errorObj *err = msGetErrorObj();
    if( err != NULL && err->code != MS_NOERR )
      err->isreported = MS_TRUE;
  }

  return MS_FAILURE;
}

static int _IsInList(char **papsProcedures, int nDistinctProcedures, char *pszProcedure)
{
  int i = 0;
  if (papsProcedures && nDistinctProcedures > 0 && pszProcedure) {
    for (i=0; i<nDistinctProcedures; i++) {
      if (papsProcedures[i] && strcmp(papsProcedures[i], pszProcedure) == 0)
        return 1;
    }
  }
  return 0;
}

/************************************************************************/
/*                           msSOSValidateFilter                        */
/*                                                                      */
/*      Look if the filter's property names have an equivalent          */
/*      layre's attribute.                                              */
/************************************************************************/
static int msSOSValidateFilter(FilterEncodingNode *psFilterNode,
                               layerObj *lp)
{
  int i=0, bFound =0;
  /* assuming here that the layer is opened*/
  if (psFilterNode && lp) {
    if (psFilterNode->eType == FILTER_NODE_TYPE_PROPERTYNAME) {
      for (i=0; i<lp->numitems; i++) {
        if (strcasecmp(lp->items[i], psFilterNode->pszValue) == 0) {
          bFound = 1;
          break;
        }
      }
      if (!bFound)
        return MS_FALSE;
    }
    if (psFilterNode->psLeftNode && psFilterNode->eType != FILTER_NODE_TYPE_SPATIAL) {
      if (msSOSValidateFilter(psFilterNode->psLeftNode, lp) == MS_FALSE)
        return MS_FALSE;
    }
    if (psFilterNode->psRightNode && psFilterNode->eType != FILTER_NODE_TYPE_SPATIAL) {
      if (msSOSValidateFilter(psFilterNode->psRightNode, lp) == MS_FALSE)
        return MS_FALSE;
    }
  }

  return MS_TRUE;
}



/************************************************************************/
/*                        msSOSAddMetadataChildNode                     */
/*                                                                      */
/*      Utility function to add a metadata node.                        */
/************************************************************************/
void msSOSAddMetadataChildNode(xmlNodePtr psParent, const char *psNodeName,
                               xmlNsPtr psNs, hashTableObj *metadata,
                               const char *psNamespaces,
                               const char *psMetadataName,
                               const char *psDefaultValue)
{
  xmlNodePtr psNode = NULL;
  char *psValue = NULL;

  if (psParent && psNodeName) {
    psValue = msOWSGetEncodeMetadata(metadata, psNamespaces, psMetadataName,
                                     psDefaultValue);
    if (psValue) {
      psNode = xmlNewChild(psParent, NULL, BAD_CAST psNodeName, BAD_CAST psValue);
      if (psNs)
        xmlSetNs(psNode,  psNs);
      free(psValue);
    }
  }
}


/************************************************************************/
/*                      msSOSGetFirstLayerForOffering                   */
/*                                                                      */
/*      return the first layer for the offering.                        */
/************************************************************************/
layerObj *msSOSGetFirstLayerForOffering(mapObj *map, const char *pszOffering,
                                        const char *pszProperty)
{
  layerObj *lp = NULL;
  const char *pszTmp = NULL;
  int i = 0;

  if (pszOffering && map) {
    for (i=0; i<map->numlayers; i++) {
      pszTmp =
        msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "offering_id");
      if (pszTmp && (strcasecmp(pszTmp, pszOffering) == 0)) {
        if (pszProperty) {
          pszTmp =
            msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S",
                                "observedproperty_id");
          if (pszTmp && (strcasecmp(pszTmp, pszProperty) == 0)) {
            lp = (GET_LAYER(map, i));
            break;
          }
        } else {
          lp = (GET_LAYER(map, i));
          break;
        }
      }
    }
  }
  return lp;
}

xmlNodePtr msSOSAddTimeNode(xmlNsPtr psNs, xmlNsPtr psNsGml, char *pszStart, char *pszEnd)
{
  xmlNodePtr psNode=NULL;

  char *timeel= NULL;

  if (strcmp((char *)psNs->prefix,"sos") == 0)
    timeel = "time";
  if (strcmp((char *)psNs->prefix,"om") == 0)
    timeel = "samplingTime";
  else
    timeel = "time";

  psNode = xmlNewNode(psNs, BAD_CAST timeel);
  xmlAddChild(psNode, msGML3TimePeriod(psNsGml, pszStart, pszEnd));
  return psNode;
}

void msSOSAddPropertyNode(xmlNsPtr psNsSwe, xmlNsPtr psNsXLink, xmlNodePtr psParent, layerObj *lp, xmlNsPtr psNsGml, char *pszCompositePhenomenonId)
{
  const char *pszValue = NULL;
  char *pszTmpVal = NULL, *pszFullName = NULL;
  xmlNodePtr psCompNode, psNode;
  int i, j=0;
  char szTmp[256];
  const char *pszComponentBase = "urn:ogc:def:property:";

  if (psParent && lp) {
    psNode = xmlNewChild(psParent, NULL, BAD_CAST "observedProperty", NULL);
    psCompNode = xmlNewChild(psNode, psNsSwe, BAD_CAST "CompositePhenomenon", NULL);
    pszValue = msOWSLookupMetadata(&(lp->metadata), "S",
                                   "observedproperty_id");
    pszTmpVal = msStrdup(pszValue);

    if (pszCompositePhenomenonId != NULL) { /* unique value needs to be constructed */
      pszTmpVal = msStringConcatenate(pszTmpVal, "_");
      pszTmpVal = msStringConcatenate(pszTmpVal, pszCompositePhenomenonId);
    }

    if (pszTmpVal) { /*should always be true */
      xmlNewNsProp(psCompNode, psNsGml, BAD_CAST "id", BAD_CAST pszTmpVal);
      msFree(pszTmpVal);
    }

    pszValue = msOWSLookupMetadata(&(lp->metadata), "S",
                                   "observedproperty_name");
    if (pszValue)
      (void) xmlNewTextChild(psCompNode, psNsGml,
                           BAD_CAST "name", BAD_CAST pszValue);

    /* add components */

    /*assuming that the layer is opened and msLayerGetItems called*/
    for(i=0; i<lp->numitems; i++) {
      pszValue = msOWSLookupMetadata(&(lp->metadata), "S", "observedproperty_authority");

      if (pszValue)
        pszTmpVal = msStrdup(pszValue);
      else
        pszTmpVal = msStrdup("OGC-SWE");

      pszFullName = msStrdup(pszComponentBase);

      pszFullName = msStringConcatenate(pszFullName, pszTmpVal);

      free(pszTmpVal);

      pszFullName = msStringConcatenate(pszFullName, ":");

      pszValue = msOWSLookupMetadata(&(lp->metadata), "S", "observedproperty_version");

      if (pszValue)
        pszTmpVal = msStrdup(pszValue);
      else
        pszTmpVal = msStrdup("1");

      pszFullName = msStringConcatenate(pszFullName, pszTmpVal);

      free(pszTmpVal);

      pszFullName = msStringConcatenate(pszFullName, ":");

      snprintf(szTmp, sizeof(szTmp), "%s_alias", lp->items[i]);
      pszValue = msOWSLookupMetadata(&(lp->metadata), "S", szTmp);

      if (pszValue)
        pszTmpVal = msStrdup(pszValue);
      else
        pszTmpVal = msStrdup(lp->items[i]);

      pszFullName = msStringConcatenate(pszFullName, pszTmpVal);

      psNode = xmlNewChild(psCompNode, psNsSwe, BAD_CAST "component", NULL);

      xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST pszFullName);
      free(pszFullName);
      free(pszTmpVal);
      j++;
    }
    pszTmpVal = msIntToString(j);
    xmlNewNsProp(psCompNode, NULL, BAD_CAST "dimension", BAD_CAST pszTmpVal);
    free(pszTmpVal);
  }
}

/************************************************************************/
/*                           msSOSAddGeometryNode                       */
/*                                                                      */
/*      Outout gml 2 gemptry nodes based on a shape. All logic comes    */
/*      from gmlWriteGeometry_GML2. Should be merged at one point if    */
/*      possible.                                                       */
/************************************************************************/
void  msSOSAddGeometryNode(xmlNsPtr psNsGml, xmlNsPtr psNsMs, xmlNodePtr psParent, mapObj *map, layerObj *lp, shapeObj *psShape,
                           const char *pszEpsg_in)
{
  char *pszTmp = NULL;
  int i,j = 0;
  xmlNodePtr psPointNode, psNode, psLineNode, psPolygonNode;
  int *panOuterList = NULL, *panInnerList = NULL;
  const char *pszEpsg = pszEpsg_in;
  char *pszEpsg_buf = NULL;
  

  if (psParent && psShape) {
    if (msProjectionsDiffer(&map->projection, &lp->projection) == MS_TRUE) {
      reprojectionObj* reprojector = msLayerGetReprojectorToMap(lp, map);
      if( reprojector )
      {
        msProjectShapeEx(reprojector, psShape);
      }
      msOWSGetEPSGProj(&(map->projection), &(lp->metadata), "SO", MS_TRUE, &pszEpsg_buf);
      pszEpsg = pszEpsg_buf;
    }
    switch(psShape->type) {
      case(MS_SHAPE_POINT):
        psNode = xmlNewChild(psParent, NULL, BAD_CAST "msGeometry", NULL);
        xmlSetNs(psNode, psNsMs);
        if (psShape->line[0].numpoints > 1) {
          psPointNode = xmlNewChild(psNode, NULL, BAD_CAST "MultiPoint", NULL);
          xmlSetNs(psPointNode, psNsGml);

          if (pszEpsg)
            xmlNewProp(psPointNode, BAD_CAST "srsName", BAD_CAST pszEpsg);
        } else
          psPointNode= psNode;

        /*add all points */
        for(i=0; i<psShape->line[0].numpoints; i++) {
          psNode = xmlAddChild(psPointNode, msGML3Point(psNsGml, pszEpsg, NULL, psShape->line[0].point[i].x, psShape->line[0].point[i].y));
        }
        break;

      case(MS_SHAPE_LINE):
        psNode = xmlNewChild(psParent, NULL, BAD_CAST "msGeometry", NULL);
        xmlSetNs(psNode,xmlNewNs(psNode, NULL,  NULL));
        if (psShape->numlines > 1) {
          psLineNode = xmlNewChild(psNode, NULL, BAD_CAST "MultiLineString", NULL);
          xmlSetNs(psLineNode,xmlNewNs(psLineNode,
                                       BAD_CAST "http://www.opengis.net/gml",
                                       BAD_CAST "gml"));

          if (pszEpsg)
            xmlNewProp(psLineNode, BAD_CAST "srsName", BAD_CAST pszEpsg);
        } else
          psLineNode= psNode;

        for(i=0; i<psShape->numlines; i++) {
          if (psShape->numlines > 1) {
            psNode = xmlNewChild(psLineNode, NULL, BAD_CAST "lineStringMember", NULL);
            xmlSetNs(psNode,xmlNewNs(psNode,
                                     BAD_CAST "http://www.opengis.net/gml",
                                     BAD_CAST "gml"));
            psNode = xmlNewChild(psNode, NULL, BAD_CAST "LineString", NULL);
            xmlSetNs(psNode,xmlNewNs(psNode,
                                     BAD_CAST "http://www.opengis.net/gml",
                                     BAD_CAST "gml"));
          } else {
            psNode = xmlNewChild(psLineNode, NULL, BAD_CAST "LineString", NULL);
            xmlSetNs(psNode,xmlNewNs(psNode,
                                     BAD_CAST "http://www.opengis.net/gml",
                                     BAD_CAST "gml"));
          }
          if (pszEpsg)
            xmlNewProp(psNode, BAD_CAST "srsName", BAD_CAST pszEpsg);

          pszTmp = NULL;
          for(j=0; j<psShape->line[i].numpoints; j++) {
            char *doubleTmp = msDoubleToString(psShape->line[i].point[j].x, MS_TRUE);
            pszTmp = msStringConcatenate(pszTmp, doubleTmp);
            pszTmp = msStringConcatenate(pszTmp, ",");
            free(doubleTmp);
            doubleTmp = msDoubleToString(psShape->line[i].point[j].y, MS_TRUE);
            pszTmp = msStringConcatenate(pszTmp, doubleTmp);
            pszTmp = msStringConcatenate(pszTmp, ",");
            free(doubleTmp);
          }
          psNode = xmlNewChild(psNode, NULL, BAD_CAST "coordinates", BAD_CAST pszTmp);
          xmlSetNs(psNode,xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml"));
          free(pszTmp);
        }

        break;

      case(MS_SHAPE_POLYGON):
        psNode = xmlNewChild(psParent, NULL, BAD_CAST "msGeometry", NULL);
        xmlSetNs(psNode,xmlNewNs(psNode, NULL,  NULL));
        if (psShape->numlines > 1) {
          psPolygonNode = xmlNewChild(psNode, NULL, BAD_CAST "MultiPolygon", NULL);
          xmlSetNs(psPolygonNode,
                   xmlNewNs(psPolygonNode, BAD_CAST "http://www.opengis.net/gml",
                            BAD_CAST "gml"));

          if (pszEpsg)
            xmlNewProp(psPolygonNode, BAD_CAST "srsName", BAD_CAST pszEpsg);
        } else
          psPolygonNode= psNode;

        panOuterList = msGetOuterList(psShape);

        for(i=0; i<psShape->numlines; i++) {
          if(panOuterList[i] != MS_TRUE)
            continue;

          panInnerList = msGetInnerList(psShape, i, panOuterList);

          if (psShape->numlines > 1) {
            psNode = xmlNewChild(psPolygonNode, NULL, BAD_CAST "polygonMember", NULL);
            xmlSetNs(psNode,xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml",
                                     BAD_CAST "gml"));
            psNode = xmlNewChild(psNode, NULL, BAD_CAST "Polygon", NULL);
            xmlSetNs(psNode,xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml",
                                     BAD_CAST "gml"));
          } else {
            psNode = xmlNewChild(psPolygonNode, NULL, BAD_CAST "Polygon", NULL);
            xmlSetNs(psNode,xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml",
                                     BAD_CAST "gml"));
          }
          if (pszEpsg)
            xmlNewProp(psNode, BAD_CAST "srsName", BAD_CAST pszEpsg);

          psNode = xmlNewChild(psNode, NULL, BAD_CAST "outerBoundaryIs", NULL);
          xmlSetNs(psNode,xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml",
                                   BAD_CAST "gml"));
          psNode = xmlNewChild(psNode, NULL, BAD_CAST "LinearRing", NULL);
          xmlSetNs(psNode,xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml",
                                   BAD_CAST "gml"));

          pszTmp = NULL;
          for(j=0; j<psShape->line[i].numpoints; j++) {
            char *doubleTmp;
            doubleTmp = msDoubleToString(psShape->line[i].point[j].x, MS_TRUE);
            pszTmp = msStringConcatenate(pszTmp, doubleTmp);
            pszTmp = msStringConcatenate(pszTmp, ",");
            free(doubleTmp);
            doubleTmp = msDoubleToString(psShape->line[i].point[j].y, MS_TRUE);
            pszTmp = msStringConcatenate(pszTmp, doubleTmp);
            pszTmp = msStringConcatenate(pszTmp, " ");
            free(doubleTmp);
          }
          psNode = xmlNewChild(psNode, NULL, BAD_CAST "coordinates", BAD_CAST pszTmp);
          xmlSetNs(psNode,xmlNewNs(psNode,
                                   BAD_CAST "http://www.opengis.net/gml",
                                   BAD_CAST "gml"));
          free(pszTmp);

          if (panInnerList)
            free(panInnerList);
        }

        if (panOuterList)
          free(panOuterList);

        break;

      default:
        break;
    }

  }
  msFree(pszEpsg_buf);

}

/************************************************************************/
/*          void msSOSAddDataBlockDefinition(xmlNodePtr psParent,       */
/*      layerObj *lp)                                                   */
/*                                                                      */
/*      Add a databalock used for GetObservation request.               */
/************************************************************************/
void msSOSAddDataBlockDefinition(xmlNsPtr psNsSwe, xmlNodePtr psParent, layerObj *lp)
{
  xmlNodePtr psNode, psRecordNode, psCompNode, psSubNode, psEncNode;
  const char *pszDefinition = NULL, *pszUom=NULL, *pszValue=NULL, *pszName=NULL;
  char szTmp[100];
  int i=0;
  char *pszTokenValue = NULL;
  char *pszBlockValue = NULL;
  const char *pszBlockSep=NULL, *pszTokenSep=NULL;

  if (psParent) {
    psNode = xmlNewChild(psParent, NULL, BAD_CAST "DataBlockDefinition", NULL);
    xmlSetNs(psNode, psNsSwe);

    /* -------------------------------------------------------------------- */
    /*      Add components.                                                 */
    /* -------------------------------------------------------------------- */
    psCompNode = xmlNewChild(psNode, NULL, BAD_CAST "components", NULL);
    psEncNode = xmlNewChild(psNode, NULL, BAD_CAST "encoding", NULL);
    psRecordNode = xmlNewChild(psCompNode, NULL, BAD_CAST "DataRecord", NULL);

    /*always add a time field if timeitem is defined*/
    if (msOWSLookupMetadata(&(lp->metadata), "SO", "timeitem")) {
      psNode = xmlNewChild(psRecordNode, NULL, BAD_CAST "field", NULL);
      xmlNewNsProp(psNode, NULL, BAD_CAST "name", BAD_CAST "time");
      psNode = xmlNewChild(psNode, NULL, BAD_CAST "Time", NULL);
      xmlNewNsProp(psNode, NULL, BAD_CAST "definition", BAD_CAST "urn:ogc:phenomenon:time:iso8601");
    }
    /*add all other fields*/
    /*assuming that the layer is open */
    for(i=0; i<lp->numitems; i++) {
      snprintf(szTmp, sizeof(szTmp), "%s_alias", lp->items[i]);
      pszValue = msOWSLookupMetadata(&(lp->metadata), "S", szTmp);
      if (pszValue) {
        psNode = xmlNewChild(psRecordNode, NULL, BAD_CAST "field", NULL);

        /* check if there is an alias/full name used */
        snprintf(szTmp, sizeof(szTmp), "%s_alias", lp->items[i]);
        pszName = msOWSLookupMetadata(&(lp->metadata), "S", szTmp);
        if (!pszName)
          pszName = lp->items[i];

        xmlNewNsProp(psNode, NULL, BAD_CAST "name", BAD_CAST pszName);

        psNode = xmlNewChild(psNode, NULL, BAD_CAST "Quantity", NULL);

        /* get definition and uom */
        snprintf(szTmp, sizeof(szTmp), "%s_definition", lp->items[i]);
        pszDefinition =  msOWSLookupMetadata(&(lp->metadata), "S", szTmp);

        if (pszDefinition == NULL)
          pszDefinition = "urn:ogc:object:definition";

        xmlNewNsProp(psNode, NULL, BAD_CAST "definition", BAD_CAST pszDefinition);

        snprintf(szTmp, sizeof(szTmp), "%s_uom", lp->items[i]);
        pszUom =  msOWSLookupMetadata(&(lp->metadata), "S", szTmp);

        if (pszUom == NULL)
          pszUom = "urn:ogc:object:uom";

        psNode = xmlNewChild(psNode, NULL, BAD_CAST "uom", NULL);
        xmlNewNsProp(psNode, NULL, BAD_CAST "code", BAD_CAST pszUom);

      }
    }

    /* -------------------------------------------------------------------- */
    /*      Add encoding block.                                             */
    /* -------------------------------------------------------------------- */
    pszBlockSep = msOWSLookupMetadata(&(lp->map->web.metadata), "S",
                                      "encoding_blockSeparator");
    pszTokenSep = msOWSLookupMetadata(&(lp->map->web.metadata), "S",
                                      "encoding_tokenSeparator");

    psSubNode = xmlNewChild(psEncNode, NULL, BAD_CAST "TextBlock", NULL);

    if (pszTokenSep)
      pszTokenValue = msStringConcatenate(pszTokenValue, (char *)pszTokenSep);
    else
      pszTokenValue = msStringConcatenate(pszTokenValue, ",");

    xmlNewNsProp(psSubNode, NULL, BAD_CAST "tokenSeparator", BAD_CAST pszTokenValue);

    if (pszBlockSep)
      pszBlockValue = msStringConcatenate(pszBlockValue, (char *)pszBlockSep);
    else
      pszBlockValue = msStringConcatenate(pszBlockValue, "\n");

    xmlNewNsProp(psSubNode, NULL, BAD_CAST "blockSeparator", BAD_CAST pszBlockValue);

    xmlNewNsProp(psSubNode, NULL, BAD_CAST "decimalSeparator", BAD_CAST ".");

    msFree(pszTokenValue);
    msFree(pszBlockValue);
  }
}

/************************************************************************/
/*                            msSOSAddMemberNode                        */
/*                                                                      */
/*      Add a memeber node corresponding to a feature.                  */
/*      Assuming that the layer is opened and msLayerGetItems is        */
/*      called on it.                                                   */
/************************************************************************/
void msSOSAddMemberNode(xmlNsPtr psNsGml, xmlNsPtr psNsOm, xmlNsPtr psNsSwe, xmlNsPtr psNsXLink, xmlNsPtr psNsMs, xmlNodePtr psParent, mapObj *map, layerObj *lp,
                        int iFeatureId, const char *script_url, const char *opLayerName)
{
  xmlNodePtr psObsNode, psNode, psLayerNode = NULL;
  const char *pszValue = NULL;
  char *pszEpsg = NULL;
  int status,i,j;
  shapeObj sShape;
  char szTmp[256];
  layerObj *lpfirst = NULL;
  const char *pszTimeField = NULL;
  char *pszTmp = NULL;
  char *pszOid = NULL;
  char *pszTime = NULL;
  char *pszValueShape = NULL;
  const char *pszFeatureId  = NULL;

  if (psParent) {
    msInitShape(&sShape);

    status = msLayerGetShape(lp, &sShape, &(lp->resultcache->results[iFeatureId]));
    if(status != MS_SUCCESS) {
      xmlFreeNs(psNsOm);
      return;
    }

    psNode = xmlNewChild(psParent, NULL, BAD_CAST "member", NULL);

    psObsNode = xmlNewChild(psNode, NULL, BAD_CAST "Observation", BAD_CAST pszValue);

    pszFeatureId = msOWSLookupMetadata(&(lp->metadata), "OSG", "featureid");

    if(pszFeatureId && msLayerGetItems(lp) == MS_SUCCESS) {
      /* find the featureid amongst the items for this layer */
      for(j=0; j<lp->numitems; j++) {
        if(strcasecmp(lp->items[j], pszFeatureId) == 0) { /* found it  */
          break;
        }
      }
      if (j<lp->numitems) {
        pszOid = msStringConcatenate(pszOid, "o_");
        pszOid = msStringConcatenate(pszOid, sShape.values[j]);
        xmlNewNsProp(psObsNode, psNsGml, BAD_CAST "id", BAD_CAST pszOid);
      }
    }

    /* order of elements is time, location, procedure, observedproperty
     featureofinterest, result */

    /* time*/
    pszTimeField =  msOWSLookupMetadata(&(lp->metadata), "SO",
                                        "timeitem");
    if (pszTimeField && sShape.values) {
      for(i=0; i<lp->numitems; i++) {
        if (strcasecmp(lp->items[i], pszTimeField) == 0) {
          if (sShape.values[i] && strlen(sShape.values[i]) > 0) {
            pszTime = msStringConcatenate(pszTime, sShape.values[i]);
            psNode = xmlNewChild(psObsNode, psNsOm, BAD_CAST "samplingTime", NULL);
            xmlAddChild(psNode, msGML3TimeInstant(psNsGml, pszTime));
            msFree(pszTime);
          }
          break;
        }
      }
    }
    /*TODO add location*/

    /*procedure*/
    /* if a procedure_item is defined, we should extract the value for the
       attributes. If not dump what is found in the procedure metadata bug 2054*/
    if ((pszValue =  msOWSLookupMetadata(&(lp->metadata), "S", "procedure_item"))) {
      lpfirst = msSOSGetFirstLayerForOffering(map,
                                              msOWSLookupMetadata(&(lp->metadata), "S",
                                                  "offering_id"),
                                              msOWSLookupMetadata(&(lp->metadata), "S",
                                                  "observedproperty_id"));

      if (lp != lpfirst)
        status = msLayerOpen(lpfirst);
      if (status == MS_SUCCESS && msLayerGetItems(lpfirst) == MS_SUCCESS) {
        for(i=0; i<lpfirst->numitems; i++) {
          if (strcasecmp(lpfirst->items[i], pszValue) == 0) {
            break;
          }
        }
        if (i < lpfirst->numitems && sShape.values) {
          snprintf(szTmp, sizeof(szTmp), "%s", "urn:ogc:def:procedure:");
          pszTmp = msStringConcatenate(pszTmp, szTmp);
          pszValueShape = msEncodeHTMLEntities(sShape.values[i]);
          pszTmp = msStringConcatenate(pszTmp, pszValueShape);

          psNode =  xmlNewChild(psObsNode, NULL, BAD_CAST "procedure", NULL);
          xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST pszTmp);
          msFree(pszTmp);
          pszTmp = NULL;
          msFree(pszValueShape);
        }
        /*else should we generate a warning !*/
        if (lp != lpfirst)
          msLayerClose(lpfirst);
      }

    } else if ((pszValue = msOWSLookupMetadata(&(lp->metadata), "S", "procedure"))) {
      if (! msOWSLookupMetadata(&(lp->metadata), "S", "procedure_item"))
        xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"sos_procedure_item\" missing for sos:procedure.  If you have more than 1 procedures, sos:procedure will output them incorrectly."));

      snprintf(szTmp, sizeof(szTmp), "%s", "urn:ogc:def:procedure:");
      pszTmp = msStringConcatenate(pszTmp, szTmp);
      pszTmp = msStringConcatenate(pszTmp, (char *)pszValue);

      psNode =  xmlNewChild(psObsNode, NULL, BAD_CAST "procedure", NULL);
      xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST pszTmp);
      msFree(pszTmp);
      pszTmp = NULL;
    }

    /*observed property*/
    pszValue = msOWSLookupMetadata(&(lp->metadata), "S",
                                   "observedproperty_id");
    if (pszValue)
      msSOSAddPropertyNode(psNsSwe, psNsXLink, psObsNode, lp, psNsGml, pszOid);
    msFree(pszOid);
    pszOid = NULL;

    /*TODO add featureofinterest*/

    pszTmp = msStringConcatenate(pszTmp, (char *) script_url);
    pszTmp = msStringConcatenate(pszTmp, "service=WFS&version=1.1.0&request=DescribeFeatureType&typename=");
    pszTmp = msStringConcatenate(pszTmp, (char *) opLayerName);

    psNode =  xmlNewChild(psObsNode, psNsOm, BAD_CAST "featureOfInterest", NULL);
    xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST pszTmp);

    msFree(pszTmp);
    pszTmp=NULL;

    /* add result : gml:featureMember of all selected elements */
    psNode = xmlNewChild(psObsNode, NULL, BAD_CAST "result", NULL);

    /*TODO should we add soemwhere the units of the value :
      <om:result uom="units.xml#cm">29.00</om:result> */


    if(msProjectionsDiffer(&(lp->projection), &(map->projection)))
    {
      reprojectionObj* reprojector = msLayerGetReprojectorToMap(lp, map);
      if( reprojector )
      {
        msProjectShapeEx(reprojector, &sShape);
      }
    }

    psNode = xmlNewChild(psNode, psNsGml, BAD_CAST "featureMember", NULL);
    /* xmlSetNs(psNode,xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml")); */

    /*TODO : add namespaces like wfs " ms and a url to mapserve ? */
    psLayerNode = xmlNewChild(psNode, psNsMs, BAD_CAST lp->name, NULL);

    /* fetch gml:id */

    pszFeatureId = msOWSLookupMetadata(&(lp->metadata), "OSG", "featureid");

    if(pszFeatureId && msLayerOpen(lp) == MS_SUCCESS && msLayerGetItems(lp) == MS_SUCCESS)

      xmlSetNs(psLayerNode,psNsMs);

    /*bbox*/

    msOWSGetEPSGProj(&(map->projection), &(lp->metadata), "SO", MS_TRUE, &pszEpsg);
    if (!pszEpsg)
      msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "SO", MS_TRUE, &pszEpsg);

    if (msProjectionsDiffer(&map->projection, &lp->projection) == MS_TRUE)
      msProjectRect(&lp->projection, &map->projection, &sShape.bounds);

    psNode = xmlAddChild(psLayerNode, msGML3BoundedBy(psNsGml, sShape.bounds.minx, sShape.bounds.miny, sShape.bounds.maxx, sShape.bounds.maxy, pszEpsg));

    /*geometry*/
    msSOSAddGeometryNode(psNsGml, psNsMs, psLayerNode, map, lp, &sShape, pszEpsg);
    msFree(pszEpsg);

    /*attributes */
    /* TODO only output attributes where there is a sos_%s_alias (to be discussed)*/
    /* the first layer is the one that has to have all the metadata defined */
    lpfirst = msSOSGetFirstLayerForOffering(map,
                                            msOWSLookupMetadata(&(lp->metadata), "S",
                                                "offering_id"),
                                            msOWSLookupMetadata(&(lp->metadata), "S",
                                                "observedproperty_id"));

    if (lpfirst && msLayerOpen(lpfirst) == MS_SUCCESS &&
        msLayerGetItems(lpfirst) == MS_SUCCESS) {
      for(i=0; i<lpfirst->numitems; i++) {
        snprintf(szTmp, sizeof(szTmp), "%s_alias", lpfirst->items[i]);
        pszValue = msOWSLookupMetadata(&(lpfirst->metadata), "S", szTmp);
        if (pszValue && sShape.values) {
          for (j=0; j<lp->numitems; j++) {
            if (strcasecmp(lpfirst->items[i],  lpfirst->items[j]) == 0) {
              /*if there is an alias used, use it to output the
                parameter name : eg "sos_AMMON_DIS_alias" "Amonia"  */
              snprintf(szTmp, sizeof(szTmp), "%s_alias", lpfirst->items[i]);
              pszValue = msOWSLookupMetadata(&(lpfirst->metadata), "S", szTmp);
              pszValueShape = msEncodeHTMLEntities(sShape.values[j]);

              if (pszValue) {
                pszTmp = msEncodeHTMLEntities(pszValue);
                psNode = xmlNewChild(psLayerNode, psNsMs, BAD_CAST pszValue,
                                     BAD_CAST pszValueShape);
                free(pszTmp);
              } else {
                pszTmp = msEncodeHTMLEntities(lpfirst->items[i]);
                psNode = xmlNewChild(psLayerNode, psNsMs,
                                     BAD_CAST lpfirst->items[i],
                                     BAD_CAST pszValueShape);
                free(pszTmp);
              }

              free(pszValueShape);
              xmlSetNs(psNode,psNsMs);
              break;
            }
          }
        }
      }
      if (lp->index != lpfirst->index)
        msLayerClose(lpfirst);
    }
    msFreeShape(&sShape);
  }
}

/************************************************************************/
/*                           msSOSReturnMemberResult                    */
/*                                                                      */
/*      Add a result string to the result node (used for                */
/*      GetObservation using "observation" as the response output.      */
/*      Assuming here that the layer is already opened.                 */
/************************************************************************/
char* msSOSReturnMemberResult(layerObj *lp, int iFeatureId, char **ppszProcedure)
{
  char *pszFinalValue = NULL;
  shapeObj sShape;
  int i, j, status;
  layerObj *lpfirst;
  const char *pszTimeField = NULL, *pszValue=NULL, *pszProcedureField=NULL;
  char *pszValueShape = NULL;
  char szTmp[100];
  const char *pszSep=NULL;

  msInitShape(&sShape);
  status = msLayerGetShape(lp, &sShape, &(lp->resultcache->results[iFeatureId]));
  if(status != MS_SUCCESS)
    return NULL;

  pszTimeField =  msOWSLookupMetadata(&(lp->metadata), "SO", "timeitem");
  if (pszTimeField && sShape.values) {
    for(i=0; i<lp->numitems; i++) {
      if (strcasecmp(lp->items[i], pszTimeField) == 0) {
        pszFinalValue = msStringConcatenate(pszFinalValue,  sShape.values[i]);
        break;
      }
    }
  }
  if (ppszProcedure && sShape.values) {
    pszProcedureField = msOWSLookupMetadata(&(lp->metadata), "S", "procedure_item");
    for(i=0; i<lp->numitems; i++) {
      if (strcasecmp(lp->items[i], pszProcedureField) == 0) {
        (*ppszProcedure) = msStrdup( sShape.values[i]);
        break;
      }
    }
  }

  /* the first layer is the one that has to have all the metadata defined */
  lpfirst = msSOSGetFirstLayerForOffering(lp->map,
                                          msOWSLookupMetadata(&(lp->metadata), "S",
                                              "offering_id"),
                                          msOWSLookupMetadata(&(lp->metadata), "S",
                                              "observedproperty_id"));


  if (sShape.values &&
      (lp == lpfirst || (lpfirst && msLayerOpen(lpfirst) == MS_SUCCESS &&
                        msLayerGetItems(lpfirst) == MS_SUCCESS))) {
    pszSep = msOWSLookupMetadata(&(lp->map->web.metadata), "S",
                                 "encoding_tokenSeparator");
    for(i=0; i<lpfirst->numitems; i++) {
      snprintf(szTmp, sizeof(szTmp), "%s_alias", lpfirst->items[i]);
      pszValue = msOWSLookupMetadata(&(lpfirst->metadata), "S", szTmp);
      if (pszValue) {
        for (j=0; j<lp->numitems; j++) {
          if (strcasecmp(lpfirst->items[i],  lpfirst->items[j]) == 0) {
            pszValueShape = msEncodeHTMLEntities(sShape.values[j]);


            if (pszFinalValue) {
              if (pszSep)
                pszFinalValue = msStringConcatenate(pszFinalValue, (char *)pszSep);
              else
                pszFinalValue = msStringConcatenate(pszFinalValue, ",");
            }
            pszFinalValue = msStringConcatenate(pszFinalValue,  pszValueShape);

            msFree(pszValueShape);
          }
        }
      }
    }
  }
  msFreeShape(&sShape);
  return pszFinalValue;
}

/************************************************************************/
/*                      msSOSAddMemberNodeObservation                   */
/*                                                                      */
/*      Add a member node used for getObservation request using         */
/*      Observation as the result format.                               */
/************************************************************************/
static
xmlNodePtr msSOSAddMemberNodeObservation(xmlNsPtr psNsGml, xmlNsPtr psNsOm, xmlNsPtr psNsSwe, xmlNsPtr psNsXLink, xmlNodePtr psParent, mapObj *map, layerObj *lp, const char *pszProcedure)
{
  char *pszTmp = NULL;
  xmlNodePtr psNode=NULL, psObsNode=NULL, psMemberNode=NULL;
  layerObj *lpfirst;
  const char *value = NULL;

  /*always featch the first layer that has the same offering id and observered propery.
   This allows to only define all the attributes and components on the first layer if the user
  wants to present several mapserver layers as the same offering.*/
  lpfirst = msSOSGetFirstLayerForOffering(map,
                                          msOWSLookupMetadata(&(lp->metadata), "S",
                                              "offering_id"),
                                          msOWSLookupMetadata(&(lp->metadata), "S",
                                              "observedproperty_id"));
  if (psParent) {
    psMemberNode = xmlNewChild(psParent, NULL, BAD_CAST "member", NULL);
    psObsNode = xmlNewChild(psMemberNode, NULL, BAD_CAST "Observation", NULL);

    /*time*/
    /* ??TODO : sampling time is a manadatory element but uses a non mandatory metadata sos_offering_timeextent */
    value = msOWSLookupMetadata(&(lp->metadata), "S", "offering_timeextent");
    if (value) {
      char **tokens;
      int n;
      char *pszEndTime = NULL;
      tokens = msStringSplit(value, '/', &n);
      if (tokens==NULL || (n != 1 && n!=2)) {
        msSetError(MS_SOSERR, "Wrong number of arguments for sos_offering_timeextent.",
                   "msSOSGetObservation()");
        msSOSException(map, "sos_offering_timeextent", "InvalidParameterValue");
        return NULL;
      }

      if (n == 2) /* end time is empty. It is going to be set as "now*/
        pszEndTime = tokens[1];

      psNode = xmlAddChild(psObsNode, msSOSAddTimeNode(psNsOm, psNsGml, tokens[0], pszEndTime));
      msFreeCharArray(tokens, n);

    }

    /* procedure */
    if (pszProcedure) { /*this should always be true since procedure is a manadtory element*/
      if (! msOWSLookupMetadata(&(lp->metadata), "S", "procedure_item") && msOWSLookupMetadata(&(lp->metadata), "S", "procedure"))
        xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"sos_procedure_item\" missing for sos:procedure.  If you have more than 1 procedures, sos:procedure will output them incorrectly."));

      pszTmp = msStringConcatenate(pszTmp, "urn:ogc:def:procedure:");
      pszTmp = msStringConcatenate(pszTmp, (char *)pszProcedure);
      psNode =  xmlNewChild(psObsNode, NULL, BAD_CAST "procedure", NULL);
      /* xmlNewNsProp(psNode, xmlNewNs(NULL, BAD_CAST "http://www.w3.org/1999/xlink", BAD_CAST "xlink"), BAD_CAST "href", BAD_CAST pszTmp); */
      xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST pszTmp);
      msFree(pszTmp);
      pszTmp = NULL;
    }

    /*observed propery and components*/
    if (lp != lpfirst &&
        msLayerOpen(lpfirst) == MS_SUCCESS && msLayerGetItems(lpfirst) == MS_SUCCESS) {
      msSOSAddPropertyNode(psNsSwe, psNsXLink, psObsNode, lpfirst, psNsGml, NULL);
      msLayerClose(lpfirst);
    } else
      msSOSAddPropertyNode(psNsSwe, psNsXLink, psObsNode, lpfirst, psNsGml, NULL);

    /* result definition*/
    psNode = xmlNewChild(psObsNode, NULL, BAD_CAST "resultDefinition", NULL);
    msSOSAddDataBlockDefinition(psNsSwe, psNode, lpfirst);

  }

  return psObsNode;
}

/************************************************************************/
/*                            msSOSParseTimeGML                         */
/*                                                                      */
/*      Utility function to convert a gml time value to a               */
/*      string. Supported gml times are :                               */
/*                                                                      */
/*       -  <gml:TimePeriod>                                            */
/*           <gml:beginPosition>2005-09-01T11:54:32</gml:beginPosition> */
/*            <gml:endPosition>2005-09-02T14:54:32</gml:endPosition>    */
/*          </gml:TimePeriod>                                           */
/*        This will be converted to startime/endtime                    */
/*                                                                      */
/*       - <gml:TimeInstant>                                            */
/*           <gml:timePosition>2003-02-13T12:28-08:00</gml:timePosition>*/
/*           </gml:TimeInstant>                                         */
/*       This will retunr the timevalue as a string.                    */
/*                                                                      */
/*       The caller of the function should free the return value.       */
/************************************************************************/
char *msSOSParseTimeGML(char *pszGmlTime)
{
  char *pszReturn = NULL, *pszBegin = NULL, *pszEnd = NULL;
  CPLXMLNode *psRoot=NULL, *psChild=NULL;
  CPLXMLNode *psTime=NULL, *psBegin=NULL, *psEnd=NULL;
  struct tm tm_struct;

  if (pszGmlTime) {
    psRoot = CPLParseXMLString(pszGmlTime);
    if(!psRoot)
      return NULL;
    CPLStripXMLNamespace(psRoot, "gml", 1);

    if (psRoot->eType == CXT_Element &&
        (EQUAL(psRoot->pszValue,"TimePeriod") ||
         EQUAL(psRoot->pszValue,"TimeInstant"))) {
      if (EQUAL(psRoot->pszValue,"TimeInstant")) {
        psChild = psRoot->psChild;
        if (psChild && EQUAL(psChild->pszValue,"timePosition")) {
          psTime = psChild->psNext;
          if (psTime && psTime->pszValue && psTime->eType == CXT_Text) {
            if (msParseTime(psTime->pszValue, &tm_struct) == MS_TRUE)
              pszReturn = msStrdup(psTime->pszValue);
          }
        }
      } else {
        psBegin = psRoot->psChild;
        if (psBegin)
          psEnd = psBegin->psNext;

        if (psBegin && EQUAL(psBegin->pszValue, "beginPosition") &&
            psEnd && EQUAL(psEnd->pszValue, "endPosition")) {
          if (psBegin->psChild && psBegin->psChild->pszValue &&
              psBegin->psChild->eType == CXT_Text)
            pszBegin = msStrdup( psBegin->psChild->pszValue);

          if (psEnd->psChild && psEnd->psChild->pszValue &&
              psEnd->psChild->eType == CXT_Text)
            pszEnd = msStrdup(psEnd->psChild->pszValue);

          if (pszBegin && pszEnd) {
            if (msParseTime(pszBegin, &tm_struct) == MS_TRUE &&
                msParseTime(pszEnd, &tm_struct) == MS_TRUE) {
              pszReturn = msStrdup(pszBegin);
              pszReturn = msStringConcatenate(pszReturn, "/");
              pszReturn = msStringConcatenate(pszReturn, pszEnd);
            }
          }
          msFree(pszBegin);
          msFree(pszEnd);
        }
      }
    }
  }
  CPLDestroyXMLNode(psRoot);
  return pszReturn;
}


/************************************************************************/
/*                           msSOSGetCapabilities                       */
/*                                                                      */
/*      getCapabilities request handler.                                */
/************************************************************************/
int msSOSGetCapabilities(mapObj *map, sosParamsObj *sosparams, cgiRequestObj *req, owsRequestObj *ows_request)
{
  xmlDocPtr psDoc = NULL;       /* document pointer */
  xmlNodePtr psRootNode, psMainNode, psNode;
  xmlNodePtr psOfferingNode;

  char *schemalocation = NULL;
  char *xsi_schemaLocation = NULL;
  char *script_url=NULL;
  const char *updatesequence=NULL;

  int i,j,k;
  layerObj *lp = NULL, *lpTmp = NULL;
  const char *value = NULL;
  char *pszTmp = NULL;
  char *pszProcedure = NULL;
  char szTmp[256];

  /* array of offering */
  char **papsOfferings = NULL;
  int nOfferings  =0, nCurrentOff = -1;
  int nProperties = 0;
  char **papszProperties = NULL;

  int iItemPosition = -1;
  shapeObj sShape;
  int status;

  /* for each layer it indicates the indice to be used in papsOfferings
     (to associate it with the offering) */
  int *panOfferingLayers = NULL;

  char **papsProcedures = NULL;
  int nDistinctProcedures =0;

  xmlNsPtr psNsGml = NULL;
  xmlNsPtr psNsSos = NULL;
  xmlNsPtr psNsOws = NULL;
  xmlNsPtr psNsOgc = NULL;
  xmlNsPtr psNsXLink = NULL;
  xmlNsPtr psNsSwe = NULL;

  xmlChar *buffer = NULL;
  int size = 0;
  msIOContext *context = NULL;

  int ows_version = OWS_1_1_0;

  int sosSupportedVersions[] = {OWS_1_0_0};
  int sosNumSupportedVersions = 1;

  /* acceptversions: do OWS Common style of version negotiation */
  if (sosparams->pszAcceptVersions) {
    char **tokens;
    int i, j, k=-1;

    tokens = msStringSplit(sosparams->pszAcceptVersions, ',', &j);

    for (i=0; i<j; i++) {
      int iVersion = 0;

      iVersion = msOWSParseVersionString(tokens[i]);

      if (iVersion == -1) {
        msSetError(MS_SOSERR, "Invalid version format : %s.", "msSOSGetCapabilities()", tokens[i]);
        msFreeCharArray(tokens, j);
        return msSOSException(map, "acceptversions", "VersionNegotiationFailed");
      }

      /* negotiate version */
      k = msOWSCommonNegotiateVersion(iVersion, sosSupportedVersions, sosNumSupportedVersions);

      if (k != -1)
        break;
    }
    msFreeCharArray(tokens, j);

    if(k == -1) {
      msSetError(MS_SOSERR, "ACCEPTVERSIONS list (%s) does not match supported versions (%s)", "msSOSGetCapabilities()", sosparams->pszAcceptVersions, pszSOSVersion);
      return msSOSException(map, "acceptversions", "VersionNegotiationFailed");
    }
  }

  /* updateSequence */
  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "SO", "updatesequence");

  if (sosparams->pszUpdateSequence != NULL) {
    i = msOWSNegotiateUpdateSequence(sosparams->pszUpdateSequence, updatesequence);
    if (i == 0) { /* current */
      msSetError(MS_SOSERR, "UPDATESEQUENCE parameter (%s) is equal to server (%s)", "msSOSGetCapabilities()", sosparams->pszUpdateSequence, updatesequence);
      return msSOSException(map, "updatesequence", "CurrentUpdateSequence");
    }
    if (i > 0) { /* invalid */
      msSetError(MS_SOSERR, "UPDATESEQUENCE parameter (%s) is higher than server (%s)", "msSOSGetCapabilities()", sosparams->pszUpdateSequence, updatesequence);
      return msSOSException(map, "updatesequence", "InvalidUpdateSequence");
    }
  }

  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = xmlNewNode(NULL, BAD_CAST "Capabilities");

  xmlDocSetRootElement(psDoc, psRootNode);

  psNsGml = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml");
  psNsSos = xmlNewNs(NULL, BAD_CAST pszSOSNamespaceUri, BAD_CAST pszSOSNamespacePrefix);
  psNsOgc = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_PREFIX);
  psNsSwe = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/swe/1.0.1", BAD_CAST "swe");

  /* name spaces */
  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml"));
  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/om/1.0", BAD_CAST "om"));

  psNsOws = xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows");
  xmlSetNs(psRootNode, psNsOws );

  xmlSetNs(psRootNode,xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/swe/1.0.1", BAD_CAST "swe"));

  psNsXLink = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);
  xmlSetNs(psRootNode, psNsXLink );

  xmlSetNs(psRootNode,xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX));
  xmlSetNs(psRootNode, xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OGC_NAMESPACE_PREFIX));
  xmlSetNs(psRootNode, xmlNewNs(psRootNode, BAD_CAST pszSOSNamespaceUri, BAD_CAST pszSOSNamespacePrefix));

  /*version fixed for now*/
  xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST pszSOSVersion);

  value = msOWSLookupMetadata(&(map->web.metadata), "SO", "updatesequence");

  if (value)
    xmlNewProp(psRootNode, BAD_CAST "updateSequence", BAD_CAST value);

  /*schema fixed*/
  schemalocation = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );
  xsi_schemaLocation = msStrdup(pszSOSNamespaceUri);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemalocation);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/sos/");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, (char *)pszSOSVersion);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/sosGetCapabilities.xsd");
  xmlNewNsProp(psRootNode, NULL, BAD_CAST "xsi:schemaLocation", BAD_CAST xsi_schemaLocation);

  /*service identification*/
  xmlAddChild(psRootNode, msOWSCommonServiceIdentification(psNsOws, map, "SOS", pszSOSVersion, "SO", NULL));

  /*service provider*/
  xmlAddChild(psRootNode, msOWSCommonServiceProvider(psNsOws, psNsXLink, map, "SO", NULL));

  /*operation metadata */

  if ((script_url=msOWSGetOnlineResource(map, "SO", "onlineresource", req)) == NULL)
  {
    free(xsi_schemaLocation);
    free(schemalocation);
    return msSOSException(map, "NoApplicableCode", "NoApplicableCode");
  }

  psMainNode = xmlAddChild(psRootNode, msOWSCommonOperationsMetadata(psNsOws));

  psNode     = xmlAddChild(psMainNode, msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"GetCapabilities", OWS_METHOD_GETPOST, (char *) script_url));
  xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "service", "SOS"));
  xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "version", (char *)pszSOSVersion));

  if (msOWSRequestIsEnabled(map, NULL, "S", "DescribeSensor", MS_TRUE)) {
    psNode     = xmlAddChild(psMainNode, msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"DescribeSensor", OWS_METHOD_GETPOST, (char *) script_url));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "service", "SOS"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "version", (char *)pszSOSVersion));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "sensorid", "urn:ogc:object:procedure"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "outputFormat", (char *)pszSOSDescribeSensorMimeType));
  }

  if (msOWSRequestIsEnabled(map, NULL, "S", "DescribeObservationType", MS_TRUE)) {
    psNode     = xmlAddChild(psMainNode, msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"DescribeObservationType", OWS_METHOD_GETPOST, (char *) script_url));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "service", "SOS"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "version", (char *)pszSOSVersion));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "observedproperty", "urn:ogc:object:observedproperty"));
  }

  if (msOWSRequestIsEnabled(map, NULL, "S", "GetObservation", MS_TRUE)) {
    psNode     = xmlAddChild(psMainNode, msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"GetObservation", OWS_METHOD_GETPOST, (char *) script_url));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "service", "SOS"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "version", (char *)pszSOSVersion));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "offering", "urn:ogc:object:offering"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "observedproperty", "urn:ogc:object:observedproperty"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "eventtime", "sos:time"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "procedure", "urn:ogc:object:sensor"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "featureofinterest", "gml:location"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "result", "ogc:Filter"));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "responseFormat", (char *)pszSOSGetObservationMimeType));
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Parameter", "resultModel", "Observation,Measurement"));
  }

  value = msOWSLookupMetadata(&(map->web.metadata), "SO", "maxfeatures");

  if (value) {
    psNode  = xmlAddChild(psMainNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,"Constraint", "DefaultMaxFeatures", (char *)value));
  }

  /*<ogc:Filter_Capabilities> */
  xmlAddChild(psRootNode, FLTGetCapabilities(psNsSos, psNsOgc, MS_TRUE));

  /*Offerings */
  psNode = xmlNewChild(psRootNode, NULL, BAD_CAST "Contents", NULL);
  psMainNode = xmlNewChild(psNode, NULL, BAD_CAST "ObservationOfferingList", NULL);

  /*go through the layers and check for metadata sos_offering_id.
    One or more layers could have the same offering id. In that case they
    are adverized as the same offering. The first layer that has*/

  if (map->numlayers) {
    papsOfferings = (char **)malloc(sizeof(char *)*map->numlayers);
    panOfferingLayers = (int *)malloc(sizeof(int)*map->numlayers);
    for (i=0; i<map->numlayers; i++)
      panOfferingLayers[i] = -1;

    for (i=0; i<map->numlayers; i++) {
      lp = (GET_LAYER(map, i));

      if (lp->status == MS_DELETE)
        continue;

      value = msOWSLookupMetadata(&(lp->metadata), "S", "offering_id");
      if (value && (msIntegerInArray(lp->index, ows_request->enabled_layers, ows_request->numlayers))) {
        nCurrentOff = -1;
        for (j=0; j<nOfferings; j++) {
          if (strcasecmp(value, papsOfferings[j]) == 0) {
            nCurrentOff = j;
            break;
          }
        }
        if (nCurrentOff >= 0) /* existing offering */
          panOfferingLayers[i] = nCurrentOff;
        else { /*new offering */
          papsOfferings[nOfferings] = msStrdup(value);
          panOfferingLayers[i] = nOfferings;
          nOfferings++;
        }
      }
    }

    if (nOfferings > 0) {
      for (i=0; i<nOfferings; i++) {
        psOfferingNode =
          xmlNewChild(psMainNode, NULL,BAD_CAST "ObservationOffering", NULL);
        xmlNewNsProp(psOfferingNode, psNsGml,
                     BAD_CAST "id", BAD_CAST papsOfferings[i]);
        for (j=0; j<map->numlayers; j++) {
          if (panOfferingLayers[j] == i) /*first layer of the offering */
            break;
        }

        lp = (GET_LAYER(map, j));

        /*description*/
        value = msOWSLookupMetadata(&(lp->metadata), "S",
                                    "offering_description");
        if (value)
          psNode = xmlNewChild(psOfferingNode, psNsGml, BAD_CAST "description", BAD_CAST value);
        else
          xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"sos_offering_description\" missing for gml:description"));

        /*name*/
        value = msOWSLookupMetadata(&(lp->metadata), "S", "offering_name");
        if (value)
          psNode = xmlNewChild(psOfferingNode, psNsGml, BAD_CAST "name", BAD_CAST value);
        else
          xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"sos_offering_name\" missing for gml:name"));

        /* srsName */
        value = msOWSLookupMetadata(&(map->web.metadata), "SO", "srs");

        if (value)
          msLibXml2GenerateList(psOfferingNode, psNsGml, "srsName", value, ' ');
        else
          xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Required metadata \"sos_srs\" missing for gml:srsName"));

        /*bounding box */
        /*TODO : if sos_offering_extent does not exist compute extents
                 Check also what happen if epsg not present */
        value = msOWSLookupMetadata(&(lp->metadata), "S", "offering_extent");
        if (value) {
          char **tokens,*pszLayerEPSG;
          int n;
          tokens = msStringSplit(value, ',', &n);
          if (tokens==NULL || n != 4) {
            msSetError(MS_SOSERR, "Wrong number of arguments for sos_offering_extent.",
                       "msSOSGetCapabilities()");
            return msSOSException(map, "sos_offering_extent", "InvalidParameterValue");
          }
          msOWSGetEPSGProj(&(lp->projection),&(lp->metadata), "SO", MS_TRUE,&pszLayerEPSG);
          if (pszLayerEPSG) {
            psNode = xmlAddChild(psOfferingNode, msGML3BoundedBy(psNsGml, atof(tokens[0]), atof(tokens[1]), atof(tokens[2]), atof(tokens[3]), pszLayerEPSG));
            msFree(pszLayerEPSG);
          }
          msFreeCharArray(tokens, n);

        }

        /* intended application */

        value = msOWSLookupMetadata(&(lp->metadata), "S", "offering_intendedapplication");

        if (value)
          psNode = xmlNewChild(psOfferingNode, psNsSos, BAD_CAST "intendedApplication", BAD_CAST value);
        else
          xmlAddSibling(psNode, xmlNewComment(BAD_CAST "WARNING: Optional metadata \"sos_offering_intendedapplication\" missing for sos:intendedApplication"));

        /*time*/
        value = msOWSLookupMetadata(&(lp->metadata), "S",
                                    "offering_timeextent");
        if (value) {
          char **tokens;
          int n;
          char *pszEndTime = NULL;
          tokens = msStringSplit(value, '/', &n);
          if (tokens==NULL || (n != 1 && n!=2)) {
            msSetError(MS_SOSERR, "Wrong number of arguments for sos_offering_timeextent.",
                       "msSOSGetCapabilities()");
            return msSOSException(map, "sos_offering_timeextent", "InvalidParameterValue");
          }

          if (n == 2) /* end time is empty. It is going to be set as "now*/
            pszEndTime = tokens[1];

          psNode = xmlAddChild(psOfferingNode, msSOSAddTimeNode(psNsSos, psNsGml, tokens[0], pszEndTime));
          msFreeCharArray(tokens, n);

        }

        /*procedure : output all procedure links for the offering */
        for (j=0; j<map->numlayers; j++) {
          if (panOfferingLayers[j] == i) {
            value = msOWSLookupMetadata(&(GET_LAYER(map, j)->metadata), "S",
                                        "procedure");
            if (value && strlen(value) > 0) {
              /*value could be a list of procedure*/
              char **tokens;
              int n = 0;
              tokens = msStringSplit(value, ' ', &n);
              for (k=0; k<n; k++) {
                /*TODO review the urn output */
                snprintf(szTmp, sizeof(szTmp), "%s", "urn:ogc:def:procedure:");
                pszTmp = msStringConcatenate(pszTmp, szTmp);
                pszTmp = msStringConcatenate(pszTmp, tokens[k]);

                psNode =
                  xmlNewChild(psOfferingNode, NULL, BAD_CAST "procedure", NULL);
                /* xmlNewNsProp(psNode, xmlNewNs(NULL, BAD_CAST "http://www.w3.org/1999/xlink", BAD_CAST "xlink"), BAD_CAST "href", BAD_CAST pszTmp); */
                xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST pszTmp);
                msFree(pszTmp);
                pszTmp = NULL;
              }
              msFreeCharArray(tokens, n);
            } else if ((value = msOWSLookupMetadata(&(GET_LAYER(map,j)->metadata), "S",
                                                    "procedure_item"))) {
              /* if a procedure_item is used, it means that the procedure
                 (or sensor) need to be extracted from the data. Thus we need to
                 query the layer and get the values from each feature */


              lpTmp = GET_LAYER(map,j);
              if (lpTmp->template == NULL)
                lpTmp->template = msStrdup("ttt");

              map->query.type = MS_QUERY_BY_RECT;
              map->query.mode = MS_QUERY_MULTIPLE;
              map->query.layer = j;
              map->query.rect = map->extent;
              msQueryByRect(map);

              /*check if the attribute specified in the procedure_item is available
                on the layer*/
              iItemPosition = -1;
              if (msLayerGetItems(lpTmp) == MS_SUCCESS &&
                  lpTmp->resultcache && lpTmp->resultcache->numresults > 0) {
                for(k=0; k<lpTmp->numitems; k++) {
                  if (strcasecmp(lpTmp->items[k], value) == 0) {
                    iItemPosition = k;
                    break;
                  }
                }
                if (iItemPosition == -1) {
                  msSetError(MS_SOSERR, "procedure_item %s could not be found on the layer %s",
                             "msSOSGetCapabilities()", value, lpTmp->name);
                  return msSOSException(map, "mapserv", "NoApplicableCode");
                }

                /*for each selected feature, grab the value of the prodedire_item*/
                /* do not duplicate sensor ids if they are the same */

                /*keep list of distinct procedures*/
                papsProcedures =
                  (char **)malloc(sizeof(char *) * lpTmp->resultcache->numresults);
                nDistinctProcedures = 0;
                for(k=0; k<lpTmp->resultcache->numresults; k++)
                  papsProcedures[k] = NULL;


                for(k=0; k<lpTmp->resultcache->numresults; k++) {
                  msInitShape(&sShape);
                  status = msLayerGetShape(lp, &sShape, &(lpTmp->resultcache->results[k]));
                  if(status != MS_SUCCESS)
                    continue;

                  if (sShape.values[iItemPosition]) {
                    pszProcedure = msStringConcatenate(pszProcedure, sShape.values[iItemPosition]);
                    if (!_IsInList(papsProcedures, nDistinctProcedures, pszProcedure)) {
                      papsProcedures[nDistinctProcedures] = msStrdup(pszProcedure);
                      nDistinctProcedures++;
                      snprintf(szTmp, sizeof(szTmp), "%s", "urn:ogc:def:procedure:");
                      pszTmp = msStringConcatenate(pszTmp, szTmp);
                      pszTmp = msStringConcatenate(pszTmp, pszProcedure);

                      psNode =
                        xmlNewChild(psOfferingNode, NULL, BAD_CAST "procedure", NULL);
                      xmlNewNsProp(psNode,
                                   xmlNewNs(NULL, BAD_CAST "http://www.w3.org/1999/xlink",
                                            BAD_CAST "xlink"), BAD_CAST "href", BAD_CAST pszTmp);
                      msFree(pszTmp);
                      pszTmp = NULL;

                    }
                    msFree(pszProcedure);
                    pszProcedure = NULL;
                  }
                }
                for(k=0; k<lpTmp->resultcache->numresults; k++)
                  if (papsProcedures[k] != NULL)
                    msFree(papsProcedures[k]);
                msFree(papsProcedures);

              } else {
                msSetError(MS_SOSERR, "Invalid procedure %s", "msSOSGetCapabilities()", value);
                return msSOSException(map, "procedure", "InvalidParameterValue");
              }
            } else {
              msSetError(MS_SOSERR, "Mandatory metadata procedure_item could not be found on the layer %s",
                         "msSOSGetCapabilities()", GET_LAYER(map,j)->name);
              return msSOSException(map, "mapserv", "NoApplicableCode");
            }

          }
        }
        /*observed property */
        /* observed property are equivalent to layers. We can group
           sevaral layers using the same sos_observedproperty_id. The
           components are the attributes. Components are exposed
           using the metadata sos_%s_aliasl */

        nProperties = 0;
        papszProperties =
          (char **)malloc(sizeof(char *)*map->numlayers);

        for (j=0; j<map->numlayers; j++) {
          if (panOfferingLayers[j] == i) {
            if ((value =
                   msOWSLookupMetadata(&(GET_LAYER(map, j)->metadata), "S",
                                       "observedproperty_id"))) {
              for (k=0; k<nProperties; k++) {
                if (strcasecmp(value, papszProperties[k]) == 0)
                  break;
              }
              if (k == nProperties) { /*not found*/
                papszProperties[nProperties] = msStrdup(value);
                nProperties++;
                lpTmp = GET_LAYER(map, j);
                if (msLayerOpen(lpTmp) == MS_SUCCESS && msLayerGetItems(lpTmp) == MS_SUCCESS) {
                  msSOSAddPropertyNode(psNsSwe, psNsXLink, psOfferingNode,
                                       lpTmp, psNsGml, NULL);
                  msLayerClose(lpTmp);
                }
              }
            }
          }
        }
        for (j=0; j<nProperties; j++)
          free(papszProperties[j]);
        free(papszProperties);

        psNode = xmlNewChild(psOfferingNode, NULL, BAD_CAST "featureOfInterest", NULL);
        xmlNewNsProp(psNode, psNsXLink, BAD_CAST "href", BAD_CAST "urn:ogc:def:feature:OGC-SWE:3:transient");

        psNode = xmlNewChild(psOfferingNode, NULL, BAD_CAST "responseFormat",
                             BAD_CAST pszSOSGetObservationMimeType);
        psNode = xmlNewChild(psOfferingNode, NULL, BAD_CAST "resultModel",
                             BAD_CAST "om:Observation");
        psNode = xmlNewChild(psOfferingNode, NULL, BAD_CAST "resultModel",
                             BAD_CAST "om:Measurement");
        psNode = xmlNewChild(psOfferingNode, NULL, BAD_CAST "responseMode",
                             BAD_CAST "inline");

      }/*end of offerings*/
    }

    if (papsOfferings && nOfferings > 0) {
      for (i=0; i<nOfferings; i++)
        msFree(papsOfferings[i]);
    }
    msFree(papsOfferings);
    if(panOfferingLayers)
      msFree(panOfferingLayers);

  }/* end of offerings */

  if ( msIO_needBinaryStdout() == MS_FAILURE )
    return MS_FAILURE;

  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  /*TODO* : check the encoding validity. Internally libxml2 uses UTF-8
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                           "SO", "encoding", OWS_NOERR,
                           "<?xml version='1.0' encoding=\"%s\" standalone=\"no\" ?>\n",
                           "ISO-8859-1");
  */
  /*xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, (encoding ? encoding : "ISO-8859-1"), 1);*/

  /* xmlDocDump crashs withe the prebuild windows binaries distibutes at the libxml site???.
   It works with locally build binaries*/

  context = msIO_getHandler(stdout);

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, ("UTF-8"), 1);
  msIO_contextWrite(context, buffer, size);
  xmlFree(buffer);

  /*free buffer and the document */
  /*xmlFree(buffer);*/
  xmlFreeDoc(psDoc);
  xmlFreeNs(psNsGml);
  xmlFreeNs(psNsSos);
  xmlFreeNs(psNsOgc);
  xmlFreeNs(psNsSwe);

  free(xsi_schemaLocation);
  free(schemalocation);
  msFree(script_url);

  /*
   *Free the global variables that may
   *have been allocated by the parser.
   */
  xmlCleanupParser();


  return(MS_SUCCESS);


  /*
  nSize = sizeof(workbuffer);
  nSize = nSize-1;*/ /* the last character for the '\0' */

  /*
  if (size > nSize)
  {
       iIndice = 0;
       while ((iIndice + nSize) <= size)
       {
           snprintf(workbuffer, (sizeof(workbuffer)-1), "%s", buffer+iIndice );
           workbuffer[sizeof(workbuffer)-1] = '\0';
           msIO_printf("%s", workbuffer);

           iIndice +=nSize;
       }
       if (iIndice < size)
       {
            sprintf(workbuffer, "%s", buffer+iIndice );
            msIO_printf("%s", workbuffer);
       }
   }
  else
  {
      msIO_printf("%s", buffer);
  }
  */
}

/************************************************************************/
/*                           msSOSGetObservation                        */
/*                                                                      */
/*      GetObservation request handler                                  */
/************************************************************************/
int msSOSGetObservation(mapObj *map, sosParamsObj *sosparams, cgiRequestObj *req, owsRequestObj *ows_request)
{
  char *schemalocation = NULL;
  char *xsi_schemaLocation = NULL;
  const char *pszTmp = NULL, *pszTmp2 = NULL;
  const char *user_namespace_uri = "http://mapserver.gis.umn.edu/mapserver";
  const char *user_namespace_prefix = "ms";
  char *script_url=NULL;
  int i, j, k, bLayerFound = 0;
  layerObj *lp = NULL, *lpfirst = NULL;
  const char *pszTimeExtent=NULL, *pszTimeField=NULL, *pszValue=NULL;
  FilterEncodingNode *psFilterNode = NULL;
  rectObj sBbox;

  xmlDocPtr psDoc = NULL;
  xmlNodePtr psRootNode,  psNode;
  char **tokens=NULL, **tokens1;
  int n=0, n1=0;
  xmlNsPtr psNsGml = NULL;
  xmlNsPtr psNsOm = NULL;
  xmlNsPtr psNsSwe = NULL;
  xmlNsPtr psNsXLink = NULL;
  xmlNsPtr psNsSos = NULL;
  xmlNsPtr psNsMs = NULL;
  const char *opLayerName = NULL;
  char *pszBuffer = NULL;
  const char *pszProcedureItem = NULL;
  int bSpatialDB = 0;
  xmlChar *buffer = NULL;
  int size = 0;
  msIOContext *context = NULL;
  xmlNodePtr psObservationNode = NULL, psResultNode=NULL;
  const char *pszProcedure = NULL;
  const char *pszBlockSep=NULL;
  int nDiffrentProc = 0;
  SOSProcedureNode *paDiffrentProc = NULL;
  int iItemPosition, status;
  shapeObj sShape;

  sBbox = map->extent;

  /* establish local namespace */
  pszTmp = msOWSLookupMetadata(&(map->web.metadata), "SFO", "namespace_uri");

  if(pszTmp) user_namespace_uri = pszTmp;

  pszTmp = msOWSLookupMetadata(&(map->web.metadata), "SFO", "namespace_prefix");
  if(pszTmp) user_namespace_prefix = pszTmp;

  /* validates mandatory request elements */
  if (!sosparams->pszOffering) {
    msSetError(MS_SOSERR, "Missing OFFERING parameter.", "msSOSGetObservation()");
    return msSOSException(map, "offering", "MissingParameterValue");
  }

  if (!sosparams->pszObservedProperty) {
    msSetError(MS_SOSERR, "Missing OBSERVEDPROPERTY parameter.", "msSOSGetObservation()");
    return msSOSException(map, "observedproperty", "MissingParameterValue");
  }

  if (!sosparams->pszResponseFormat) {
    msSetError(MS_SOSERR, "Missing RESPONSEFORMAT parameter.", "msSOSGetObservation()");
    return msSOSException(map, "responseformat", "MissingParameterValue");
  }

  if (strcasecmp(sosparams->pszResponseFormat, pszSOSGetObservationMimeType) != 0) {
    msSetError(MS_SOSERR, "Invalid RESPONSEFORMAT parameter %s.  Allowable values are: %s", "msSOSGetObservation()", sosparams->pszResponseFormat, pszSOSGetObservationMimeType);
    return msSOSException(map, "responseformat", "InvalidParameterValue");
  }

  if (sosparams->pszResponseMode && strcasecmp(sosparams->pszResponseMode, "inline") != 0) {
    msSetError(MS_SOSERR, "Invalid RESPONSEMODE parameter %s.  Allowable values are: \"inline\"", "msSOSGetObservation()", sosparams->pszResponseMode);
    return msSOSException(map, "responsemode", "InvalidParameterValue");
  }

  /*validate if offering exists*/
  for (i=0; i<map->numlayers; i++) {
    pszTmp = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "offering_id");
    if (pszTmp && (strcasecmp(pszTmp, sosparams->pszOffering) == 0) &&
        (msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers)))
      break;
  }

  if (i==map->numlayers) {
    msSetError(MS_SOSERR, "Offering %s not found. A layer might be disabled for \
this request. Check sos/ows_enable_request settings.", "msSOSGetObservation()", sosparams->pszOffering);
    return msSOSException(map, "offering", "InvalidParameterValue");
  }

  /*validate if observed property exist*/
  /* Allow more the 1 oberved property comma separated (specs is unclear on it). If we
     do it, we need to see if other parameters like result (filter encoding)
     should be given for each property too) */

  bLayerFound = 0;
  tokens = msStringSplit(sosparams->pszObservedProperty, ',', &n);

  for (i=0; i<map->numlayers; i++) {
    pszTmp = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "offering_id");
    pszTmp2 = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "observedproperty_id");

    GET_LAYER(map, i)->status = MS_OFF;

    if (pszTmp && pszTmp2) {
      if (strcasecmp(pszTmp, sosparams->pszOffering) == 0) {
        if (tokens && n > 0) {
          for (j=0; j<n; j++) {
            if(strcasecmp(pszTmp2, tokens[j]) == 0) {
              GET_LAYER(map, i)->status = MS_ON;
              /* opLayerName = msStrdup(GET_LAYER(map, i)->name); */
              opLayerName = GET_LAYER(map, i)->name;
              /* Force setting a template to enable query. */
              if (!GET_LAYER(map, i)->template)
                GET_LAYER(map, i)->template = msStrdup("ttt.html");
              bLayerFound = 1;
              break;
            }
          }
        }
      }
    }
  }
  if (tokens && n > 0)
    msFreeCharArray(tokens, n);

  if (bLayerFound == 0) {
    msSetError(MS_SOSERR, "ObservedProperty %s not found.", "msSOSGetObservation()", sosparams->pszObservedProperty);
    return msSOSException(map, "observedproperty", "InvalidParameterValue");
  }

  /* apply procedure : could be a comma separated list.
     set status to on those layers that have the sos_procedure metadata
     equals to this parameter. Note that the layer should already have it's status at ON
     by the  offering,observedproperty filter done above */

  if (sosparams->pszProcedure) {
    bLayerFound = 0;
    tokens = msStringSplit(sosparams->pszProcedure, ',', &n);

    if (tokens && n > 0) {
      for (i=0; i<map->numlayers; i++) {
        if(GET_LAYER(map, i)->status == MS_ON) {
          pszValue =  msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "procedure");

          if (pszValue) {
            /* the procedure metadata can be a list "sensor1 sensor2..."*/
            tokens1 = msStringSplit(pszValue, ' ', &n1);

            for (j=0; j<n; j++) {
              for (k=0; k<n1; k++) {
                if (strcasecmp(tokens1[k], tokens[j]) == 0) { /* found */
                  bLayerFound = 1;
                  break;
                }
              }
              if (k<n1)
                break;
            }
            if (j == n) /*not found*/
              GET_LAYER(map, i)->status = MS_OFF;

            if (tokens1)
              msFreeCharArray(tokens1, n1);

            if (bLayerFound == 0) {
              msSetError(MS_SOSERR, "Procedure %s not found.", "msSOSGetObservation()", sosparams->pszProcedure);
              msFreeCharArray(tokens, n);
              return msSOSException(map, "procedure", "InvalidParameterValue");
            }
          }

          /* if there is a procedure_item defined on the layer, we will */
          /* use it to set the filter parameter of the layer */

          if ((GET_LAYER(map, i)->status == MS_ON) && (pszProcedureItem =  msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "procedure_item"))) {

            lp = GET_LAYER(map, i);

            /* HACK BEGIN */

            if (msOWSLookupMetadata(&(GET_LAYER(map,i)->metadata), "S", "procedure") == NULL) {
              /* if sos_procedure_item is used, and sos_procedure is not, it means that */
              /* the procedure or sensor) need to be extracted from the data. Thus we */
              /* need to query the layer and get the values from each feature */

              if (lp->template == NULL)
                lp->template = msStrdup("ttt");

              map->query.type = MS_QUERY_BY_RECT;
              map->query.mode = MS_QUERY_MULTIPLE;
              map->query.layer = i;
              map->query.rect = map->extent;
              msQueryByRect(map);

              /*check if the attribute specified in the procedure_item is available */
              /*on the layer*/
              iItemPosition = -1;
              if (msLayerGetItems(lp) == MS_SUCCESS && lp->resultcache && lp->resultcache->numresults > 0) {
                for(k=0; k<lp->numitems; k++) {
                  if (strcasecmp(lp->items[k], pszProcedureItem) == 0) {
                    iItemPosition = k;
                    break;
                  }
                }
                if (iItemPosition == -1) {
                  msSetError(MS_SOSERR, "sos_procedure_item %s could not be found on the layer %s",
                             "msSOSGetCapabilities()", pszProcedureItem, lp->name);
                  return msSOSException(map, "mapserv", "NoApplicableCode");
                }

                /*for each selected feature, grab the value of the procedure_item*/
                bLayerFound = 0;

                for(k=0; k<lp->resultcache->numresults; k++) {
                  msInitShape(&sShape);
                  status = msLayerGetShape(lp, &sShape, &(lp->resultcache->results[k]));
                  if(status != MS_SUCCESS)
                    continue;

                  if (sShape.values[iItemPosition]) {
                    tokens = msStringSplit(sosparams->pszProcedure, ',', &n);
                    for (j=0; j<n; j++) {
                      if (strcasecmp(sShape.values[iItemPosition], tokens[j]) == 0) { /* found */
                        bLayerFound = 1;
                        break;
                      }
                    }
                  }

                  if (bLayerFound)
                    break;
                }

                if (bLayerFound == 0) {
                  msSetError(MS_SOSERR, "Invalid procedure value %s", "msSOSGetCapabilities()", sosparams->pszProcedure);
                  return msSOSException(map, "procedure", "InvalidParameterValue");
                }
              }
            }
            /* HACK END */

            pszBuffer = NULL;
            if (lp->filter.string && strlen(lp->filter.string) > 0)
              msFreeExpression(&lp->filter);

            /*The filter should reflect the underlying db*/
            /*for ogr add a where clause */
            bSpatialDB = 0;
            if (lp->connectiontype == MS_POSTGIS ||  lp->connectiontype == MS_ORACLESPATIAL || lp->connectiontype == MS_OGR)
              bSpatialDB = 1;


            if (bSpatialDB) {
              if (lp->connectiontype != MS_OGR)
                pszBuffer = msStringConcatenate(pszBuffer, "(");
              else
                pszBuffer = msStringConcatenate(pszBuffer, "WHERE ");
            } else
              pszBuffer = msStringConcatenate(pszBuffer, "(");

            for (j=0; j<n; j++) {
              if (j > 0)
                pszBuffer = msStringConcatenate(pszBuffer, " OR ");

              pszBuffer = msStringConcatenate(pszBuffer, "(");

              if (!bSpatialDB) {
                pszBuffer = msStringConcatenate(pszBuffer, "'[");
                pszBuffer = msStringConcatenate(pszBuffer, (char *)pszProcedureItem);
              } else {
                char* pszEscapedStr = msLayerEscapePropertyName(lp, (char *)pszProcedureItem);
                pszBuffer = msStringConcatenate(pszBuffer, pszEscapedStr);
                msFree(pszEscapedStr);
              }

              if (!bSpatialDB)
                pszBuffer = msStringConcatenate(pszBuffer, "]'");

              pszBuffer = msStringConcatenate(pszBuffer, " = '");
              {
                char* pszEscapedStr = msLayerEscapeSQLParam(lp, tokens[j]);
                pszBuffer = msStringConcatenate(pszBuffer,  pszEscapedStr);
                msFree(pszEscapedStr);
              }
              pszBuffer = msStringConcatenate(pszBuffer,  "')");
            }

            if (!bSpatialDB || lp->connectiontype != MS_OGR)
              pszBuffer = msStringConcatenate(pszBuffer, ")");

            msLoadExpressionString(&lp->filter, pszBuffer);
            if (pszBuffer)
              msFree(pszBuffer);
          }
        }
      }
      msFreeCharArray(tokens, n);
    }
  }

  /* -------------------------------------------------------------------- */
  /*      supports 2 types of gml:Time : TimePeriod and TimeInstant :     */
  /*      - <gml:TimePeriod>                                              */
  /*          <gml:beginPosition>2005-09-01T11:54:32</gml:beginPosition>  */
  /*         <gml:endPosition>2005-09-02T14:54:32</gml:endPosition>       */
  /*       </gml:TimePeriod>                                              */
  /*                                                                      */
  /*      - <gml:TimeInstant>                                             */
  /*           <gml:timePosition>2003-02-13T12:28-08:00</gml:timePosition>*/
  /*         </gml:TimeInstant>                                           */
  /*                                                                      */
  /*       The user can specify mutilple times separated by commas.       */
  /*                                                                      */
  /*       The gml will be parsed and trasformed into a sting tah         */
  /*      looks like timestart/timeend,...                                */
  /* -------------------------------------------------------------------- */

  /*apply time filter if available */
  if (sosparams->pszEventTime) {
    char **apszTimes = NULL;
    int numtimes = 0;
    char *pszTimeString = NULL, *pszTmp = NULL;

    /* Because SOS has specific TemporalOperator which extends FES 1.1, the time filter  */
    /* passed is different than what mapogcfilter (per 1.0.0) supports.                  */
    /*                                                                                   */
    /* Because, in XML POST mode, we traverse directly to gml:TimePeriod|gml:TimeInstant */
    /* this is passed directly to mapogcfilter.                                          */
    /* for GET requests, we strip the parent element before passing                      */

    pszTmp = msStrdup(sosparams->pszEventTime);

    pszTmp = msCaseReplaceSubstring(pszTmp, "<ogc:TM_Equals>", "");
    pszTmp = msCaseReplaceSubstring(pszTmp, "<TM_Equals>", "");
    pszTmp = msCaseReplaceSubstring(pszTmp, "</ogc:TM_Equals>", "");
    pszTmp = msCaseReplaceSubstring(pszTmp, "</TM_Equals>", "");

    apszTimes = msStringSplit (pszTmp, ',', &numtimes);
    msFree(pszTmp);

    if (numtimes >=1) {
      for (i=0; i<numtimes; i++) {
        pszTmp = msSOSParseTimeGML(apszTimes[i]);
        if (pszTmp) {
          if (pszTimeString)
            pszTimeString = msStringConcatenate(pszTimeString, ",");
          pszTimeString = msStringConcatenate(pszTimeString, pszTmp);
          msFree(pszTmp);
        }
      }
      msFreeCharArray(apszTimes, numtimes);
    }
    if (!pszTimeString) {
      msSetError(MS_SOSERR, "Invalid time value given for the eventTime parameter %s", "msSOSGetObservation()", sosparams->pszEventTime);
      return msSOSException(map, "eventtime", "InvalidParameterValue");
    }
    for (i=0; i<map->numlayers; i++) {
      if (GET_LAYER(map, i)->status == MS_ON) {
        /* the sos_offering_timeextent should be used for time validation*/
        /*TODO : too documented  ?*/
        lpfirst = msSOSGetFirstLayerForOffering(map, msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "offering_id"), NULL);
        if (lpfirst)
          pszTimeExtent = msOWSLookupMetadata(&lpfirst->metadata, "S", "offering_timeextent");

        pszTimeField =  msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "SO", "timeitem");

        if (pszTimeField) {
          /*validate only if time extent is set.*/
          if (pszTimeExtent) {
            if (msValidateTimeValue(pszTimeString, pszTimeExtent) == MS_TRUE)
              msLayerSetTimeFilter((GET_LAYER(map, i)), pszTimeString, pszTimeField);
            else {
              /*we should turn the layer off since the eventTime is not in the time extent*/
              GET_LAYER(map, i)->status = MS_OFF;
            }
          } else
            msLayerSetTimeFilter((GET_LAYER(map, i)), pszTimeString, pszTimeField);
        }
      }
    }
    if (pszTimeString)
      msFree(pszTimeString);
  }

  /*bbox*/
  /* this is a gml feature
  <gml:Envelope xmlns:gml="http://www.opengis.net/gml">
  <gml:lowerCorner srsName="EPSG:4326">-66 43</gml:lowerCorner>
  <upperCorner srsName="EPSG:4326">-62 45</upperCorner>
  </gml:Envelope>
  */

  if (sosparams->pszFeatureOfInterest) {
    int bValid = 0;
    CPLXMLNode *psRoot=NULL;
    char *pszSRS = NULL;

    psRoot = CPLParseXMLString(sosparams->pszFeatureOfInterest);
    if(!psRoot) {
      msSetError(MS_SOSERR, "Invalid gml:Envelope value given for featureOfInterest.", "msSOSGetObservation()");
      return msSOSException(map, "featureofinterest", "InvalidParameterValue");
    }

    CPLStripXMLNamespace(psRoot, "gml", 1);
    bValid = FLTParseGMLEnvelope(psRoot, &sBbox, &pszSRS);

    /* TODO we should reproject the bbox to the map projection if there is an srs defined */
    if (!bValid) {
      msSetError(MS_SOSERR, "Invalid gml:Envelope value given for featureOfInterest %s.", "msSOSGetObservation()", sosparams->pszEventTime);
      return msSOSException(map, "featureofinterest", "InvalidParameterValue");
    }
    map->extent.minx = sBbox.minx;
    map->extent.miny = sBbox.miny;
    map->extent.maxx = sBbox.maxx;
    map->extent.maxy = sBbox.maxy;

    CPLDestroyXMLNode(psRoot);
    msFree(pszSRS);
  }

  if (sosparams->pszSrsName) { /* validate against MAP.WEB.METADATA.sos_srs */
    int iUnits = -1;
    char **tokens = NULL;
    const char *pszSRSList = NULL;
    int n = 0;
    int bFound = 0;
    int k;
    char srsbuffer[100];
    projectionObj po;

    pszSRSList = msOWSLookupMetadata(&(map->web.metadata), "SO", "srs");

    if (pszSRSList) {
      tokens = msStringSplit(pszSRSList, ' ', &n);

      if (tokens && n > 0) {
        for (k=0; k<n; k++) {
          if (strncasecmp(tokens[k], "EPSG:", strlen("EPSG:")) == 0 &&
              strcasecmp(sosparams->pszSrsName, tokens[k]) == 0) { /* match */
            bFound = 1;

            /* project MAP.EXTENT to this SRS */
            msInitProjection(&po);
            msProjectionInheritContextFrom(&po, &map->projection);

            snprintf(srsbuffer, sizeof(srsbuffer), "+init=epsg:%.20s", sosparams->pszSrsName+strlen("EPSG:"));

            if (msLoadProjectionString(&po, srsbuffer) != 0) {
              msSetError(MS_SOSERR, "Could not set output projection to \"%s\"", "msSOSGetObservation()", sosparams->pszSrsName);
              return msSOSException(map, "mapserv", "NoApplicableCode");
            }

            if (msProjectionsDiffer(&map->projection, &po) == MS_TRUE) {
              msProjectRect(&map->projection, &po, &map->extent);
              sBbox = map->extent;
            }

            /* set map->projection to this SRS */
            if (msLoadProjectionString(&(map->projection), srsbuffer) != 0) {
              msSetError(MS_SOSERR, "Could not set output projection to \"%s\"", "msSOSGetObservation()", sosparams->pszSrsName);
              return msSOSException(map, "mapserv", "NoApplicableCode");
            }

            iUnits = GetMapserverUnitUsingProj(&(map->projection));
            if (iUnits != -1)
              map->units = iUnits;

            msFreeProjection(&po);
            break;
          }
        }
        msFreeCharArray(tokens, n);
      }
      if (bFound == 0) {
        msSetError(MS_SOSERR, "srsName value \"%s\" unsupported / invalid", "msSOSGetObservation()", sosparams->pszSrsName);
        return msSOSException(map, "srsName", "InvalidParameterValue");
      }
    } else {
      msSetError(MS_SOSERR, "MAP.WEB.METADATA.sos_srs not set", "msSOSGetObservation()");
      return msSOSException(map, "mapserv", "NoApplicableCode");
    }
  }

  /* apply filter */
  if (sosparams->pszResult) {
    psFilterNode = FLTParseFilterEncoding(sosparams->pszResult);

    if (!psFilterNode) {
      msSetError(MS_SOSERR, "Invalid or Unsupported RESULT in GetObservation: %s", "msSOSGetObservation()", sosparams->pszResult);
      return msSOSException(map, "result", "InvalidParameterValue");
    }
    /* apply the filter to all layers that are on */
    for (i=0; i<map->numlayers; i++) {
      lp = GET_LAYER(map, i);
      if (lp->status == MS_ON) {
        /* preparse parser so that alias for fields can be used */
        FLTPreParseFilterForAliasAndGroup(psFilterNode, map, i, "S");
        /* validate that the property names used are valid
          (there is a corresponding layer attribute) */
        if (msLayerOpen(lp) == MS_SUCCESS && msLayerGetItems(lp) == MS_SUCCESS) {
          if (msSOSValidateFilter(psFilterNode, lp)== MS_FALSE) {
            msSetError(MS_SOSERR, "Invalid component name in RESULT statement", "msSOSGetObservation()");
            return msSOSException(map, "result", "InvalidParameterValue");
          }
          msLayerClose(lp);
        }
        FLTApplyFilterToLayer(psFilterNode, map, i);
      }
    }

    FLTFreeFilterEncodingNode(psFilterNode);
  }



  /* this is just a fall back if bbox is enetered. The bbox parameter is not supported
     by the sos specs */
  if (sosparams->pszBBox && !sosparams->pszFeatureOfInterest) {
    char **tokens;
    int n;

    tokens = msStringSplit(sosparams->pszBBox, ',', &n);
    if (tokens==NULL || n != 4) {
      msSetError(MS_SOSERR, "Wrong number of arguments for bounding box.", "msSOSGetObservation()");
      return msSOSException(map, "bbox", "InvalidParameterValue");
    }
    sBbox.minx = atof(tokens[0]);
    sBbox.miny = atof(tokens[1]);
    sBbox.maxx = atof(tokens[2]);
    sBbox.maxy = atof(tokens[3]);
    msFreeCharArray(tokens, n);
  }

  /* do the query if the filter encoding (pszResult) is not part of the request.
     If pszResult is available, the query on the layers will be done when the filter
     is parsed*/
  if (!sosparams->pszResult) {
    map->query.type = MS_QUERY_BY_RECT;
    map->query.mode = MS_QUERY_MULTIPLE;
    map->query.layer = -1;
    map->query.rect = sBbox;
    msQueryByRect(map);
  }

  /*get the first layers of the offering*/
  for (i=0; i<map->numlayers; i++) {
    pszTmp = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "offering_id");
    if (pszTmp && (strcasecmp(pszTmp, sosparams->pszOffering) == 0)) {
      lp = (GET_LAYER(map, i));
      break;
    }
  }

  /* build xml return tree*/

  psNsSos = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/sos/1.0", BAD_CAST "sos");
  psNsGml = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml");
  psNsOm = xmlNewNs(NULL, BAD_CAST pszOMNamespaceUri, BAD_CAST pszOMNamespacePrefix);
  psNsSwe = xmlNewNs(NULL, BAD_CAST "http://www.opengis.net/swe/1.0.1", BAD_CAST "swe");
  psNsXLink = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);
  psNsMs = xmlNewNs(NULL, BAD_CAST user_namespace_uri, BAD_CAST user_namespace_prefix);

  psDoc = xmlNewDoc(BAD_CAST "1.0");
  psRootNode = xmlNewNode(NULL, BAD_CAST "ObservationCollection");
  xmlDocSetRootElement(psDoc, psRootNode);

  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml"));
  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/ows/1.1", BAD_CAST "ows"));
  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/swe/1.0.1", BAD_CAST "swe"));
  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX));
  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX));
  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST pszSOSNamespaceUri, BAD_CAST pszSOSNamespacePrefix));
  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST user_namespace_uri, BAD_CAST user_namespace_prefix));
  xmlSetNs(psRootNode,  xmlNewNs(psRootNode, BAD_CAST "http://www.opengis.net/om/1.0", BAD_CAST "om"));

  xmlNewNsProp(psRootNode, psNsGml, BAD_CAST "id", BAD_CAST sosparams->pszOffering);

  schemalocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

  if ((script_url=msOWSGetOnlineResource(map, "SO", "onlineresource", req)) == NULL)
  {
    free(schemalocation);
    return msSOSException(map, "NoApplicableCode", "NoApplicableCode");
  }

  xsi_schemaLocation = msStrdup("http://www.opengis.net/om/1.0 ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemalocation);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/om/1.0.0/om.xsd ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, (char *) user_namespace_uri);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, (char *) script_url);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "service=WFS&version=1.1.0&request=DescribeFeatureType&typename=");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, (char *) opLayerName);
  xmlNewNsProp(psRootNode, NULL, BAD_CAST "xsi:schemaLocation", BAD_CAST xsi_schemaLocation);

  /* description */
  pszTmp = msOWSLookupMetadata(&(lp->metadata), "S", "offering_description");
  if (pszTmp) {
    psNode = xmlNewChild(psRootNode, NULL, BAD_CAST "description", BAD_CAST pszTmp);
    xmlSetNs(psNode, xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml"));
  }

  /* name */
  pszTmp = msOWSLookupMetadata(&(lp->metadata), "S", "offering_name");
  if (pszTmp) {
    psNode = xmlNewChild(psRootNode, NULL, BAD_CAST "name", BAD_CAST pszTmp);
    xmlSetNs(psNode, xmlNewNs(psNode, BAD_CAST "http://www.opengis.net/gml", BAD_CAST "gml"));
  }

  /* extent */
  pszTmp = msOWSLookupMetadata(&(lp->metadata), "S", "offering_extent");

  if (pszTmp) {
    char **tokens;
    char *pszMapEpsg;
    int n;
    rectObj envelope;

    msOWSGetEPSGProj(&(map->projection), &(lp->metadata), "SO", MS_TRUE, &pszMapEpsg);

    tokens = msStringSplit(pszTmp, ',', &n);
    if (tokens==NULL || n != 4) {
      msSetError(MS_SOSERR, "Wrong number of arguments for sos_offering_extent.",
                 "msSOSGetCapabilities()");
      msFree(script_url);
      return msSOSException(map, "sos_offering_extent", "InvalidParameterValue");
    }

    envelope.minx = atof(tokens[0]);
    envelope.miny = atof(tokens[1]);
    envelope.maxx = atof(tokens[2]);
    envelope.maxy = atof(tokens[3]);

    if (map && msProjectionsDiffer(&map->projection, &lp->projection) == MS_TRUE) {
      if (msProjectRect(&lp->projection, &map->projection, &envelope) == MS_FAILURE) {
        msSetError(MS_SOSERR, "Coordinates transformation failed.  Raised in msProjectRect() of file %s line %d", "msSOSGetCapabilities()", __FILE__, __LINE__);
        msFree(script_url);
        return msSOSException(map, "sos_offering_extent", "InvalidParameterValue");
      }
    }

    psNode = xmlAddChild(psRootNode, msGML3BoundedBy(psNsGml, envelope.minx, envelope.miny, envelope.maxx, envelope.maxy, pszMapEpsg));
    msFreeCharArray(tokens, n);
    msFree(pszMapEpsg);
  }

  /* time
  pszTmp = msOWSLookupMetadata(&(lp->metadata), "S","offering_timeextent");
  if (pszTmp)
  {
      char **tokens;
      int n;
      char *pszEndTime = NULL;
      tokens = msStringSplit(pszTmp, '/', &n);
      if (tokens==NULL || (n != 1 && n!=2)) {
          msSetError(MS_SOSERR, "Wrong number of arguments for sos_offering_timeextent.",
                     "msSOSGetCapabilities()");
          return msSOSException(map, "sos_offering_timeextent", "InvalidParameterValue");
      }
  */
  /*    if (n == 2) */ /* end time is empty. It is going to be set as "now" */
  /*      pszEndTime = tokens[1];

        psNode = xmlAddChild(psRootNode, msSOSAddTimeNode(xmlNewNs(NULL, BAD_CAST pszOMNamespaceUri, BAD_CAST pszOMNamespacePrefix), tokens[0], pszEndTime));
      psNode = xmlAddChild(psRootNode, msSOSAddTimeNode(psNsOm, psNsGml, tokens[0], pszEndTime));
      msFreeCharArray(tokens, n);

  }
  */

  if (sosparams->pszResultModel && strcasecmp(sosparams->pszResultModel, "om:Measurement") != 0 &&
      strcasecmp(sosparams->pszResultModel, "om:Observation") != 0) {
    msSetError(MS_SOSERR, "resultModel should be om:Measurement or om:Observation", "msSOSGetObservation()");
    free(xsi_schemaLocation);
    free(schemalocation);
    msFree(script_url);
    xmlFreeNs(psNsSos);
    xmlFreeNs(psNsGml);
    xmlFreeNs(psNsOm);
    xmlFreeNs(psNsSwe);
    xmlFreeNs(psNsXLink);
    xmlFreeNs(psNsMs);
    xmlFreeDoc(psDoc);
    return msSOSException(map, "resultModel", "InvalidParameterValue");
  }

  else {
    /* output result members */
    for (i=0; i<map->numlayers; i++) {
      if (GET_LAYER(map, i)->resultcache && GET_LAYER(map, i)->resultcache->numresults > 0) {
        msLayerGetItems((GET_LAYER(map, i)));
        pszTmp = msOWSLookupMetadata(&(map->web.metadata), "SO", "maxfeatures");
        if (pszTmp != NULL)
          n1 = atoi(pszTmp);
        else
          n1 = 0;

        if (sosparams->pszResultModel == NULL || strcasecmp(sosparams->pszResultModel, "om:Measurement") == 0) {
          for(j=0; j<GET_LAYER(map, i)->resultcache->numresults; j++) {
            msSOSAddMemberNode(psNsGml, psNsOm, psNsSwe, psNsXLink, psNsMs, psRootNode, map, (GET_LAYER(map, i)), j, script_url, opLayerName);
            if (j == n1-1)
              break;
          }
        } else { /*assuming here that pszResultModel = observation */
          /*layer does not define a procedure_item: this means one procedure per
            layer defined using sos_procedure)*/
          if (msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "procedure_item") == NULL) {
            pszProcedure = msOWSLookupMetadata(&(lp->metadata), "S", "procedure");
            psObservationNode = msSOSAddMemberNodeObservation(psNsGml, psNsOm, psNsSwe, psNsXLink, psRootNode, map, (GET_LAYER(map, i)),
                                pszProcedure);
            /*add a result node*/
            psResultNode = xmlNewChild(psObservationNode, NULL, BAD_CAST "result", NULL);
            for(j=0; j<GET_LAYER(map, i)->resultcache->numresults; j++) {
              /*add a block separator*/
              if (j > 0) {
                pszBlockSep = msOWSLookupMetadata(&(map->web.metadata), "S",
                                                  "encoding_blockSeparator");
                if (pszBlockSep)
                  xmlNodeAddContent(psResultNode, BAD_CAST pszBlockSep);
                else
                  xmlNodeAddContent(psResultNode, BAD_CAST "\n");
              }
              {
                char* pszResult = msSOSReturnMemberResult((GET_LAYER(map, i)), j, NULL);
                if (pszResult) {
                  xmlNodeAddContent(psResultNode, BAD_CAST pszResult);
                  msFree(pszResult);
                }
              }
            }
          }
          /*this is the case where procedure_item is used. Needs more management since
            the same data on a layer contains different procedures (procedures are
            one of the fields of the record)*/
          else {

            for(j=0; j<GET_LAYER(map, i)->resultcache->numresults; j++) {
              char* pszProcedureValue = NULL;
              char* pszResult = msSOSReturnMemberResult((GET_LAYER(map, i)), j, &pszProcedureValue);
              if (!pszProcedureValue || !pszResult)
              {
                msFree(pszProcedureValue);
                msFree(pszResult);
                continue;
              }
              for (k=0; k<nDiffrentProc; k++) {
                if (strcasecmp(paDiffrentProc[k].pszProcedure, pszProcedureValue) == 0) {
                  pszBlockSep = msOWSLookupMetadata(&(map->web.metadata), "S",
                                                    "encoding_blockSeparator");
                  if (pszBlockSep)
                    xmlNodeAddContent(paDiffrentProc[k].psResultNode, BAD_CAST pszBlockSep);
                  else
                    xmlNodeAddContent(paDiffrentProc[k].psResultNode, BAD_CAST "\n");
                  xmlNodeAddContent(paDiffrentProc[k].psResultNode, BAD_CAST pszResult);
                  break;
                }
              }
              if (k == nDiffrentProc) { /*a new procedure*/
                nDiffrentProc++;
                if (paDiffrentProc == NULL)
                  paDiffrentProc = (SOSProcedureNode *)malloc(sizeof(SOSProcedureNode));
                else
                  paDiffrentProc = (SOSProcedureNode *)realloc(paDiffrentProc,
                                   sizeof(SOSProcedureNode)
                                   *nDiffrentProc);

                paDiffrentProc[nDiffrentProc-1].pszProcedure = msStrdup(pszProcedureValue);
                psObservationNode = msSOSAddMemberNodeObservation(psNsGml, psNsOm, psNsSwe, psNsXLink, psRootNode, map,
                                    (GET_LAYER(map, i)),
                                    pszProcedureValue);

                paDiffrentProc[nDiffrentProc-1].psResultNode =
                  xmlNewChild(psObservationNode, NULL, BAD_CAST "result", NULL);

                xmlNodeAddContent(paDiffrentProc[nDiffrentProc-1].psResultNode, BAD_CAST pszResult);
              }
              msFree(pszProcedureValue);
              msFree(pszResult);
            }
            if (paDiffrentProc) {
              for (k=0; k<nDiffrentProc; k++)
                msFree(paDiffrentProc[k].pszProcedure);
              free(paDiffrentProc);
            }
          }
        }
      }
    }
  }

  /* output results */
  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  context = msIO_getHandler(stdout);
  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, ("UTF-8"), 1);
  msIO_contextWrite(context, buffer, size);
  free(schemalocation);
  free(xsi_schemaLocation);
  xmlFreeNs(psNsSos);
  xmlFreeNs(psNsGml);
  xmlFreeNs(psNsOm);
  xmlFreeNs(psNsSwe);
  xmlFreeNs(psNsXLink);
  xmlFreeNs(psNsMs);
  xmlFree(buffer);
  msFree(script_url);

  /*free  document */
  xmlFreeDoc(psDoc);
  /*
  *Free the global variables that may
  *have been allocated by the parser.
  */
  xmlCleanupParser();


  return(MS_SUCCESS);
}



/************************************************************************/
/*                           msSOSDescribeSensor                        */
/*                                                                      */
/*      Describe sensor request handler.                               */
/************************************************************************/
int msSOSDescribeSensor(mapObj *map, sosParamsObj *sosparams, owsRequestObj *ows_request)
{
  char *pszEncodedUrl = NULL;
  const char *pszId = NULL, *pszUrl = NULL;
  int i = 0, j=0, k=0;
  layerObj *lp = NULL;
  int iItemPosition = -1;
  shapeObj sShape;
  int status;
  char *tmpstr = NULL, *pszTmp = NULL, *pszProcedureURI = NULL, *pszProcedureId = NULL;

  if (!sosparams->pszOutputFormat) {
    msSetError(MS_SOSERR, "Missing mandatory parameter outputFormat.", "msSOSDescribeSensor()");
    return msSOSException(map, "outputformat", "MissingParameterValue");
  }

  if (strcasecmp(sosparams->pszOutputFormat, pszSOSDescribeSensorMimeType) != 0) {
    msSetError(MS_SOSERR, "Invalid outputformat parameter %s.  Allowable values are: %s", "msSOSDescribeSensor()", sosparams->pszOutputFormat, pszSOSDescribeSensorMimeType);
    return msSOSException(map, "outputformat", "InvalidParameterValue");
  }

  if (!sosparams->pszProcedure) {
    msSetError(MS_SOSERR, "Missing mandatory parameter procedure", "msSOSDescribeSensor()");
    return msSOSException(map, "procedure", "MissingParameterValue");
  }

  for (i=0; i<map->numlayers; i++) {
    lp = GET_LAYER(map, i);
    pszId = msOWSLookupMetadata(&(lp->metadata), "S", "procedure");
    if (pszId && strlen(pszId) > 0) {
      /*procedure could be a list*/
      char **tokens = NULL;
      int n=0;
      int bFound = 0;
      tokens = msStringSplit(pszId, ' ', &n);
      for (k=0; k<n; k++) {
        if (tokens[k] && strlen(tokens[k]) > 0) {
          pszProcedureURI = msStrdup("urn:ogc:def:procedure:");
          pszProcedureURI = msStringConcatenate(pszProcedureURI, tokens[k]);
          if ( (pszProcedureURI && strcasecmp(pszProcedureURI, sosparams->pszProcedure) == 0) &&
               (msIntegerInArray(lp->index, ows_request->enabled_layers, ows_request->numlayers)) ) {
            bFound = 1;
            pszProcedureId = msStrdup(tokens[k]);
            msFree(pszProcedureURI);
            break;
          }
          msFree(pszProcedureURI);
        }
      }
      msFreeCharArray(tokens, n);
      if (bFound) {
        pszUrl = msOWSLookupMetadata(&(lp->metadata), "S", "describesensor_url");
        if (pszUrl) {
          pszTmp = msStrdup(pszUrl);

          /* %procedure% is the hardcoded variable name to use
             within sos_describesensor_url */
          tmpstr = (char *)malloc(sizeof(char)*strlen("procedure") + 3);
          sprintf(tmpstr,"%%%s%%", "procedure");
          if (strcasestr(pszUrl, tmpstr) != NULL)
            pszTmp = msCaseReplaceSubstring(pszTmp, tmpstr, pszProcedureId);
          msFree(tmpstr);

          pszEncodedUrl = msEncodeHTMLEntities(pszTmp);
          msIO_printf("Location: %s\n\n", pszEncodedUrl);
          msFree(pszTmp);
          msFree(pszEncodedUrl);
          msFree(pszProcedureId);
          return(MS_SUCCESS);
        } else {
          msSetError(MS_SOSERR, "Missing mandatory metadata sos_describesensor_url on layer %s", "msSOSDescribeSensor()", lp->name);
          return msSOSException(map, "sos_describesensor_url", "MissingParameterValue");
        }
      }
    } else if ((pszId = msOWSLookupMetadata(&(lp->metadata), "S", "procedure_item"))) {
      iItemPosition = -1;
      if (msLayerOpen(lp) == MS_SUCCESS && msLayerGetItems(lp) == MS_SUCCESS) {
        for(j=0; j<lp->numitems; j++) {
          if (strcasecmp(lp->items[j], pszId) == 0) {
            iItemPosition = j;
            break;
          }
        }
        msLayerClose(lp);
      }
      if (iItemPosition >=0) {
        if (lp->template == NULL)
          lp->template = msStrdup("ttt");

        map->query.type = MS_QUERY_BY_RECT;
        map->query.mode = MS_QUERY_MULTIPLE;
        map->query.layer = i;
        map->query.rect = map->extent;
        msQueryByRect(map);

        msLayerGetItems(lp);

        if (lp->resultcache && lp->resultcache->numresults > 0) {
          for(j=0; j<lp->resultcache->numresults; j++) {
            msInitShape(&sShape);
            status = msLayerGetShape(lp, &sShape, &(lp->resultcache->results[j]));
            if(status != MS_SUCCESS)
              continue;

            if (sShape.values[iItemPosition]) {
              pszProcedureURI = msStrdup("urn:ogc:def:procedure:");
              pszProcedureURI = msStringConcatenate(pszProcedureURI, sShape.values[iItemPosition]);
              if (strcasecmp(pszProcedureURI, sosparams->pszProcedure) == 0) {
                pszUrl = msOWSLookupMetadata(&(lp->metadata), "S", "describesensor_url");
                pszProcedureId = msStrdup(sShape.values[iItemPosition]);
                if (pszUrl) {
                  pszTmp = msStrdup(pszUrl);

                  /* %procedure% is the hardcoded variable names to use
                     within sos_describesensor_url */
                  tmpstr = (char *)malloc(sizeof(char)*strlen("procedure") + 3);
                  sprintf(tmpstr,"%%%s%%", "procedure");
                  if (strcasestr(pszUrl, tmpstr) != NULL)
                    pszTmp = msCaseReplaceSubstring(pszTmp, tmpstr, pszProcedureId);
                  msFree(tmpstr);

                  pszEncodedUrl = msEncodeHTMLEntities(pszTmp);
                  msIO_printf("Location: %s\n\n", pszEncodedUrl);
                  msFree(pszTmp);
                  return(MS_SUCCESS);
                } else {
                  msSetError(MS_SOSERR, "Missing mandatory metadata sos_describesensor_url on layer %s", "msSOSDescribeSensor()", lp->name);
                  return msSOSException(map, "mapserv", "NoApplicableCode");
                }
              }
            }
          }
        }
      }
    }
  }
  msSetError(MS_SOSERR, "procedure %s not found.", "msSOSDescribeSensor()", sosparams->pszProcedure);
  return msSOSException(map, "procedure", "InvalidParameterValue");
}

/************************************************************************/
/*                           msSOSDescribeObservationType               */
/*                                                                      */
/*      DescribeObserrvationType request handler                        */
/************************************************************************/
int msSOSDescribeObservationType(mapObj *map, sosParamsObj *sosparams, cgiRequestObj *req, owsRequestObj *ows_request)
{
  int i, j, n = 0, bLayerFound = 0;
  char **tokens = NULL;
  char *script_url=NULL;
  const char *pszTmp = NULL;
  char *pszTmp2=NULL;
  const char *opLayerName = NULL;

  if (!sosparams->pszObservedProperty) {
    msSetError(MS_SOSERR, "Missing mandatory parameter observedproperty", "msSOSDescribeObservationType()");
    return msSOSException(map, "observedproperty", "MissingParameterValue");
  }

  tokens = msStringSplit(sosparams->pszObservedProperty, ',', &n);

  for (i=0; i<map->numlayers; i++) {
    if (!msIntegerInArray(GET_LAYER(map, i)->index, ows_request->enabled_layers, ows_request->numlayers))
      continue;
    pszTmp = msOWSLookupMetadata(&(GET_LAYER(map, i)->metadata), "S", "observedproperty_id");
    if (pszTmp) {
      if (strcasecmp(pszTmp, sosparams->pszObservedProperty) == 0) {
        if (tokens && n > 0) {
          for (j=0; j<n; j++) {
            if(strcasecmp(pszTmp, tokens[j]) == 0) {
              opLayerName = GET_LAYER(map, i)->name;
              bLayerFound = 1;
              break;
            }
          }
        }
      }
    }
  }
  if (tokens && n > 0)
    msFreeCharArray(tokens, n);

  if (bLayerFound == 0) {
    msSetError(MS_SOSERR, "ObservedProperty %s not found.", "msSOSGetObservation()", sosparams->pszObservedProperty);
    return msSOSException(map, "observedproperty", "InvalidParameterValue");
  }

  if ((script_url=msOWSGetOnlineResource(map, "SO", "onlineresource", req)) == NULL)
    return msSOSException(map, "NoApplicableCode", "NoApplicableCode");

  pszTmp2 = msStringConcatenate(pszTmp2, (char *) script_url);
  pszTmp2 = msStringConcatenate(pszTmp2, "service=WFS&version=1.1.0&request=DescribeFeatureType&typename=");
  pszTmp2 = msStringConcatenate(pszTmp2, (char *) opLayerName);

  msIO_printf("Location: %s\n\n", pszTmp2);
  msFree(pszTmp2);
  msFree(script_url);
  return(MS_SUCCESS);
}


#endif /* defined(USE_WCS_SVR) && defined(USE_LIBXML2) */

/*
** msSOSDispatch() is the entry point for SOS requests.
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
*/
int msSOSDispatch(mapObj *map, cgiRequestObj *req, owsRequestObj *ows_request)
{
#if defined(USE_SOS_SVR) && defined(USE_LIBXML2)
  int returnvalue = MS_DONE;
  sosParamsObj *paramsObj = (sosParamsObj *)calloc(1, sizeof(sosParamsObj));

  if (msSOSParseRequest(map, req, paramsObj) == MS_FAILURE) {
    msSOSFreeParamsObj(paramsObj);
    free(paramsObj);
    return MS_FAILURE;
  }

  /* SERVICE must be specified and be SOS */
  if (paramsObj->pszService && strcasecmp(paramsObj->pszService, "SOS") == 0) { /* this is an SOS request */
    if (!paramsObj->pszRequest) {
      msSetError(MS_SOSERR, "Missing REQUEST Parameter", "msSOSDispatch()");
      msSOSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return msSOSException(map, "request", "MissingParameterValue");
    }

    msOWSRequestLayersEnabled(map, "S", paramsObj->pszRequest, ows_request);
    if (ows_request->numlayers == 0) {
      msSetError(MS_SOSERR, "SOS request not enabled. Check sos/ows_enable_request settings.", "msSOSDispatch()");
      msSOSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return msSOSException(map, "request", "InvalidParameterValue");
    }

    if (strcasecmp(paramsObj->pszRequest, "GetCapabilities") == 0) {
      returnvalue = msSOSGetCapabilities(map, paramsObj, req, ows_request);
      msSOSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
    }

    else if (strcasecmp(paramsObj->pszRequest, "DescribeSensor") == 0 ||
             strcasecmp(paramsObj->pszRequest, "GetObservation") == 0 ||
             strcasecmp(paramsObj->pszRequest, "DescribeObservationType") == 0
            ) {
      /* check version */
      if (!paramsObj->pszVersion) {
        msSetError(MS_SOSERR, "Missing VERSION parameter", "msSOSDispatch()");
        msSOSFreeParamsObj(paramsObj);
        free(paramsObj);
        paramsObj = NULL;
        return msSOSException(map, "version", "MissingParameterValue");
      }

      if (msOWSParseVersionString(paramsObj->pszVersion) != OWS_1_0_0) {
        msSetError(MS_SOSERR, "VERSION %s not supported.  Supported versions are: %s.", "msSOSDispatch()", paramsObj->pszVersion, pszSOSVersion);
        msSOSFreeParamsObj(paramsObj);
        free(paramsObj);
        paramsObj = NULL;
        return msSOSException(map, "version", "InvalidParameterValue");
      }

      if (strcasecmp(paramsObj->pszRequest, "DescribeSensor") == 0)
        returnvalue = msSOSDescribeSensor(map, paramsObj, ows_request);

      else if (strcasecmp(paramsObj->pszRequest, "GetObservation") == 0)
        returnvalue = msSOSGetObservation(map, paramsObj, req, ows_request);

      else if (strcasecmp(paramsObj->pszRequest, "DescribeObservationType") == 0)
        returnvalue = msSOSDescribeObservationType(map, paramsObj, req, ows_request);

      msSOSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
    } else {
      msSetError(MS_SOSERR, "Invalid REQUEST parameter: %s", "msSOSDispatch()", paramsObj->pszRequest);
      msSOSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return msSOSException(map, "request", "InvalidParameterValue");
    }
  } else {
    msSOSFreeParamsObj(paramsObj);
    free(paramsObj);
    return MS_DONE;  /* Not an SOS request */
  }
#else
  msSetError(MS_SOSERR, "SOS support is not available.", "msSOSDispatch()");
  return(MS_FAILURE);
#endif
}

#if defined(USE_SOS_SVR) && defined(USE_LIBXML2)

int msSOSParseRequest(mapObj *map, cgiRequestObj *request, sosParamsObj *sosparams)
{
  int i;
  xmlDocPtr doc;
  xmlXPathContextPtr context;
  xmlNodeSetPtr nodeset;
  xmlXPathObjectPtr psXPathTmp;
  char *pszTmp = NULL;

  if (request->NumParams) { /* this is a GET request */
    for(i=0; i<request->NumParams; i++) {
      if (strcasecmp(request->ParamNames[i], "SERVICE") == 0)
        sosparams->pszService = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "VERSION") == 0)
        sosparams->pszVersion = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "ACCEPTVERSIONS") == 0)
        sosparams->pszAcceptVersions = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "REQUEST") == 0)
        sosparams->pszRequest = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "UPDATESEQUENCE") == 0)
        sosparams->pszUpdateSequence = msStrdup(request->ParamValues[i]);
      else  if (strcasecmp(request->ParamNames[i], "SENSORID") == 0)
        sosparams->pszSensorId = msStrdup(request->ParamValues[i]);
      else  if (strcasecmp(request->ParamNames[i], "PROCEDURE") == 0)
        sosparams->pszProcedure = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "OUTPUTFORMAT") == 0)
        sosparams->pszOutputFormat = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "OFFERING") == 0)
        sosparams->pszOffering = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "OBSERVEDPROPERTY") == 0)
        sosparams->pszObservedProperty = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "EVENTTIME") == 0)
        sosparams->pszEventTime = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "RESULT") == 0)
        sosparams->pszResult = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "RESULTMODEL") == 0)
        sosparams->pszResultModel = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "RESPONSEFORMAT") == 0)
        sosparams->pszResponseFormat = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "RESPONSEMODE") == 0)
        sosparams->pszResponseMode = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "BBOX") == 0)
        sosparams->pszBBox = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "SRSNAME") == 0)
        sosparams->pszSrsName = msStrdup(request->ParamValues[i]);
      else if (strcasecmp(request->ParamNames[i], "FEATUREOFINTEREST") == 0)
        sosparams->pszFeatureOfInterest = msStrdup(request->ParamValues[i]);
    }
  }

  if (request->postrequest) { /* this a POST request */
    /* load document */
    doc = xmlParseDoc((xmlChar *)request->postrequest);
    if (doc == NULL ) {
      msSetError(MS_SOSERR, "Invalid POST request.  XML is not well-formed", "msSOSParseRequest()");
      return msSOSException(map, "request", "InvalidRequest");
    }

    /* load context */
    context = xmlXPathNewContext(doc);
    if (context == NULL) {
      msSetError(MS_SOSERR, "Could not create context (xmlXPathNewContext)", "msSOSParseRequest()");
      return msSOSException(map, "request", "NoApplicableCode");
    }

    /* register namespaces */
    if(xmlXPathRegisterNs(context, (xmlChar *)"sos", (xmlChar *)"http://www.opengis.net/sos/1.0") != 0 ||
        xmlXPathRegisterNs(context, (xmlChar *)"ows", (xmlChar *)"http://www.opengis.net/ows/1.1") != 0 ||
        xmlXPathRegisterNs(context, (xmlChar *)"ogc", (xmlChar *)"http://www.opengis.net/ogc") != 0 ||
        xmlXPathRegisterNs(context, (xmlChar *)"gml", (xmlChar *)"http://www.opengis.net/gml") != 0) {
      msSetError(MS_SOSERR, "Could not register namespaces (xmlXPathRegisterNs)", "msSOSParseRequest()");
      return msSOSException(map, "request", "NoApplicableCode");
    }

    /* check for service */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/*/@service");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszService = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for updateSequence*/
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/*/@updateSequence");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszUpdateSequence = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for version */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/*/ows:AcceptVersions/ows:Version|/*/@version");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszVersion = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for request */

    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetCapabilities");

    if (psXPathTmp)
      sosparams->pszRequest = msStrdup("GetCapabilities");

    xmlXPathFreeObject(psXPathTmp);

    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:DescribeSensor");

    if (psXPathTmp)
      sosparams->pszRequest = msStrdup("DescribeSensor");

    xmlXPathFreeObject(psXPathTmp);

    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation");

    if (psXPathTmp)
      sosparams->pszRequest = msStrdup("GetObservation");

    xmlXPathFreeObject(psXPathTmp);

    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:DescribeObservationType");

    if (psXPathTmp)
      sosparams->pszRequest = msStrdup("DescribeObservationType");

    xmlXPathFreeObject(psXPathTmp);

    /* check for outputformat */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:DescribeSensor/@outputFormat");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszOutputFormat = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for Procedure */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:DescribeSensor/sos:procedure");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszProcedure = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for offering */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/sos:offering");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszOffering = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for observedproperty */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/sos:observedProperty");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszObservedProperty = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for procedure */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/sos:procedure");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszProcedure = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for responseFormat */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/sos:responseFormat");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszResponseFormat = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for resultModel */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/sos:resultModel");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszResultModel = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for responseMode */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/sos:responseMode");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszResponseMode = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for srsName */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/@srsName");

    if (psXPathTmp) {
      nodeset = psXPathTmp->nodesetval;
      sosparams->pszSrsName = (char *)xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for result (chunk of XML) */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/sos:result/child::*");

    if (psXPathTmp) {
      sosparams->pszResult = msLibXml2GetXPathTree(doc, psXPathTmp);
      pszTmp = msStringConcatenate(pszTmp, "<ogc:Filter>");
      pszTmp = msStringConcatenate(pszTmp, sosparams->pszResult);
      pszTmp = msStringConcatenate(pszTmp, "</ogc:Filter>");
      msFree(sosparams->pszResult);
      sosparams->pszResult = msStrdup(pszTmp);
      msFree(pszTmp);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for featureOfInterest (chunk of XML) */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/sos:featureOfInterest/ogc:BBOX/gml:Envelope");

    if (psXPathTmp) {
      sosparams->pszFeatureOfInterest = (char *)msLibXml2GetXPathTree(doc, psXPathTmp);
    }

    xmlXPathFreeObject(psXPathTmp);

    /* check for eventTime (chunk of XML) */
    psXPathTmp = msLibXml2GetXPath(doc, context, (xmlChar *)"/sos:GetObservation/sos:eventTime/*/gml:TimeInstant|/sos:GetObservation/sos:eventTime/*/gml:TimePeriod");

    if (psXPathTmp) {
      sosparams->pszEventTime = (char *)msLibXml2GetXPathTree(doc, psXPathTmp);
    }

    xmlXPathFreeObject(psXPathTmp);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    xmlCleanupParser();
  }
  return MS_SUCCESS;
}

void msSOSFreeParamsObj(sosParamsObj *sosparams)
{
  if (sosparams) {
    if (sosparams->pszService)
      free(sosparams->pszService);
    if (sosparams->pszVersion)
      free(sosparams->pszVersion);
    if (sosparams->pszAcceptVersions)
      free(sosparams->pszAcceptVersions);
    if (sosparams->pszUpdateSequence)
      free(sosparams->pszUpdateSequence);
    if (sosparams->pszRequest)
      free(sosparams->pszRequest);
    if (sosparams->pszOutputFormat)
      free(sosparams->pszOutputFormat);
    if (sosparams->pszSensorId)
      free(sosparams->pszSensorId);
    if (sosparams->pszProcedure)
      free(sosparams->pszProcedure);
    if (sosparams->pszOffering)
      free(sosparams->pszOffering);
    if (sosparams->pszObservedProperty)
      free(sosparams->pszObservedProperty);
    if (sosparams->pszEventTime)
      free(sosparams->pszEventTime);
    if (sosparams->pszResult)
      free(sosparams->pszResult);
    if (sosparams->pszResponseFormat)
      free(sosparams->pszResponseFormat);
    if (sosparams->pszResultModel)
      free(sosparams->pszResultModel);
    if (sosparams->pszResponseMode)
      free(sosparams->pszResponseMode);
    if (sosparams->pszSrsName)
      free(sosparams->pszSrsName);
    if (sosparams->pszFeatureOfInterest)
      free(sosparams->pszFeatureOfInterest);
  }
}
#endif /* defined(USE_SOS_SVR) && defined(USE_LIBXML2) */

/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC WFS 2.0.0 implementation. This file holds some WFS 2.0.0
 *           specific functions but other parts are still implemented in mapwfs.c.
 * Author:   Even Rouault
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

#define MS_OWS_11_NAMESPACE_PREFIX       MS_OWSCOMMON_OWS_NAMESPACE_PREFIX
#define MS_OWS_11_NAMESPACE_URI          MS_OWSCOMMON_OWS_110_NAMESPACE_URI

/************************************************************************/
/*                          msWFSException20()                          */
/************************************************************************/

int msWFSException20(mapObj *map, const char *locator,
                     const char *exceptionCode)
{
  int size = 0;
  char *errorString     = NULL;
  char *errorMessage    = NULL;
  char *schemasLocation = NULL;

  xmlDocPtr  psDoc      = NULL;
  xmlNodePtr psRootNode = NULL;
  xmlNsPtr   psNsOws    = NULL;
  xmlChar *buffer       = NULL;

  psNsOws = xmlNewNs(NULL, BAD_CAST MS_OWS_11_NAMESPACE_URI, BAD_CAST MS_OWS_11_NAMESPACE_PREFIX);

  errorString = msGetErrorString("\n");
  errorMessage = msEncodeHTMLEntities(errorString);
  schemasLocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));

  psDoc = xmlNewDoc(BAD_CAST "1.0");

  psRootNode = msOWSCommonExceptionReport(psNsOws, OWS_1_1_0, schemasLocation, "2.0.0", msOWSGetLanguage(map, "exception"), exceptionCode, locator, errorMessage);

  xmlDocSetRootElement(psDoc, psRootNode);

  xmlNewNs(psRootNode, BAD_CAST MS_OWS_11_NAMESPACE_URI, BAD_CAST MS_OWS_11_NAMESPACE_PREFIX);

  /* FIXME: should we emit a HTTP status code? */
  /* msIO_setHeader("Status", "%s", "400 Bad request"); */
  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, "UTF-8", 1);

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
/*                          msWFSIncludeSection                         */
/************************************************************************/
static int msWFSIncludeSection(wfsParamsObj *params, const char* pszSection)
{
    int i;
    if( params->pszSections == NULL )
        return TRUE;
    for(i = 0; params->pszSections[i] != '\0'; i++ )
    {
        if( strncasecmp( params->pszSections + i, "All", strlen("All")) == 0 )
            return TRUE;
        if( strncasecmp( params->pszSections + i, pszSection, strlen(pszSection)) == 0 )
            return TRUE;
    }
    return FALSE;
}

/************************************************************************/
/*                       msWFSAddGlobalSRSNameParam                     */
/************************************************************************/

static void msWFSAddGlobalSRSNameParam(xmlNodePtr psMainNode,
                                       xmlNsPtr psNsOws,
                                       mapObj* map)
{
}

/************************************************************************/
/*                       msWFSConstraintDefaultValue                    */
/************************************************************************/

static
xmlNodePtr msWFSConstraintDefaultValue(xmlNsPtr psNs, xmlNsPtr psNsOws, const char *name, const char* value)
{
  xmlNodePtr psRootNode = NULL;

  psRootNode = xmlNewNode(psNs, BAD_CAST "Constraint");

  xmlNewProp(psRootNode, BAD_CAST "name", BAD_CAST name);

  xmlNewChild(psRootNode, psNsOws, BAD_CAST "NoValues", NULL );
  xmlNewChild(psRootNode, psNsOws, BAD_CAST "DefaultValue", BAD_CAST value);

  return psRootNode;
}

/************************************************************************/
/*                              msWFSOperator                           */
/************************************************************************/

static
xmlNodePtr msWFSOperator(xmlNsPtr psNsFES, const char* pszOperatorType, const char* pszOperator)
{
    xmlNodePtr psNode = xmlNewNode(psNsFES, BAD_CAST pszOperatorType);
    xmlNewProp(psNode, BAD_CAST "name", BAD_CAST pszOperator);
    return psNode;
}

/************************************************************************/
/*                       msWFS20FilterCapabilities                      */
/************************************************************************/

static
xmlNodePtr msWFS20FilterCapabilities(xmlNsPtr psNsFES, xmlNsPtr psNsOws)
{
  xmlNodePtr psRootNode = NULL, psNode = NULL, psSubNode = NULL;

  psRootNode = xmlNewNode(psNsFES, BAD_CAST "Filter_Capabilities");
  
  psNode = xmlNewChild(psRootNode, psNsFES, BAD_CAST "Conformance", NULL);
  
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsQuery", "TRUE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsAdHocQuery", "TRUE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsFunctions", "FALSE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsResourceId", "TRUE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsMinStandardFilter", "TRUE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsStandardFilter", "FALSE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsMinSpatialFilter", "TRUE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsSpatialFilter", "FALSE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsMinTemporalFilter", "TRUE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsTemporalFilter", "FALSE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsVersionNav", "FALSE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsSorting", "TRUE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsExtendedOperators", "FALSE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsMinimumXPath", "TRUE"));
    xmlAddChild(psNode, msWFSConstraintDefaultValue(psNsFES, psNsOws, "ImplementsSchemaElementFunc", "FALSE"));

  psNode = xmlNewChild(psRootNode, psNsFES, BAD_CAST "Id_Capabilities", NULL);
    psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "ResourceIdentifier", NULL );
    xmlNewProp(psSubNode, BAD_CAST "name", BAD_CAST "fes:ResourceId");

  psNode = xmlNewChild(psRootNode, psNsFES, BAD_CAST "Scalar_Capabilities", NULL);
  xmlNewChild(psNode, psNsFES, BAD_CAST "LogicalOperators", NULL);
  psNode = xmlNewChild(psNode, psNsFES, BAD_CAST "ComparisonOperators", NULL);
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator", "PropertyIsEqualTo"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator", "PropertyIsNotEqualTo"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator", "PropertyIsLessThan"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator", "PropertyIsGreaterThan"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator", "PropertyIsLessThanOrEqualTo"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator", "PropertyIsGreaterThanOrEqualTo"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator", "PropertyIsLike"));
  xmlAddChild(psNode, msWFSOperator(psNsFES, "ComparisonOperator", "PropertyIsBetween"));
  /* Missing: PropertyIsNull, PropertyIsNil */

  psNode = xmlNewChild(psRootNode, psNsFES, BAD_CAST "Spatial_Capabilities", NULL);

  psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "GeometryOperands", NULL);
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:Point"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiPoint"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:LineString"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiLineString"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:Curve"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiCurve"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:Polygon"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiPolygon"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:Surface"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiSurface"));
  /* xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:MultiGeometry")); */
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:Box"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "GeometryOperand", "gml:Envelope"));

  psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "SpatialOperators", NULL);
#ifdef USE_GEOS
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Equals"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Disjoint"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Touches"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Within"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Overlaps"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Crosses"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Intersects"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Contains"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "DWithin"));
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "Beyond"));
#endif
  xmlAddChild(psSubNode, msWFSOperator(psNsFES, "SpatialOperator", "BBOX"));

  psNode = xmlNewChild(psRootNode, psNsFES, BAD_CAST "Temporal_Capabilities", NULL);
    psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "TemporalOperands", NULL);
    xmlAddChild(psSubNode, msWFSOperator(psNsFES, "TemporalOperand", "gml:TimePeriod"));
    xmlAddChild(psSubNode, msWFSOperator(psNsFES, "TemporalOperand", "gml:TimeInstant"));

    psSubNode = xmlNewChild(psNode, psNsFES, BAD_CAST "TemporalOperators", NULL);
    xmlAddChild(psSubNode, msWFSOperator(psNsFES, "TemporalOperator", "During"));

  return psRootNode;
}

/************************************************************************/
/*                          msWFSGetCapabilities20                      */
/*                                                                      */
/*      Return the capabilities document for WFS 2.0.0                  */
/************************************************************************/
int msWFSGetCapabilities20(mapObj *map, wfsParamsObj *params,
                           cgiRequestObj *req, owsRequestObj *ows_request)
{
  xmlDocPtr psDoc = NULL;       /* document pointer */
  xmlNodePtr psRootNode, psMainNode, psNode, psFtNode;
  const char *updatesequence=NULL;
  xmlNsPtr psNsOws, psNsXLink;
  xmlNsPtr psNsFES = NULL;
  char *schemalocation = NULL;
  char *xsi_schemaLocation = NULL;
  const char *user_namespace_prefix = NULL;
  const char *user_namespace_uri = NULL;
  gmlNamespaceListObj *namespaceList=NULL; /* for external application schema support */

  char *script_url=NULL, *formats_list;
  const char *value = NULL;

  xmlChar *buffer = NULL;
  int size = 0, i;
  msIOContext *context = NULL;

  int ows_version = OWS_1_1_0;
  int ret;

  /* -------------------------------------------------------------------- */
  /*      Handle updatesequence                                           */
  /* -------------------------------------------------------------------- */
  ret = msWFSHandleUpdateSequence(map, params, "msWFSGetCapabilities20()");
  if( ret != MS_SUCCESS )
      return ret;

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
  xmlNewProp(psRootNode, BAD_CAST "xmlns", BAD_CAST MS_OWSCOMMON_WFS_20_NAMESPACE_URI);

  xmlSetNs(psRootNode, xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_GML_32_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_GML_NAMESPACE_PREFIX));
  xmlSetNs(psRootNode, xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_WFS_20_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_WFS_NAMESPACE_PREFIX));

  psNsOws = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_OWS_110_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_OWS_NAMESPACE_PREFIX);
  psNsXLink = xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX);
  xmlNewNs(psRootNode, BAD_CAST MS_OWSCOMMON_FES_20_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_FES_20_NAMESPACE_PREFIX );

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
  msGMLFreeNamespaces(namespaceList);
  

  xmlNewProp(psRootNode, BAD_CAST "version", BAD_CAST params->pszVersion );

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "FO", "updatesequence");

  if (updatesequence)
    xmlNewProp(psRootNode, BAD_CAST "updateSequence", BAD_CAST updatesequence);

  /*schema*/
  schemalocation = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );
  xsi_schemaLocation = msStrdup(MS_OWSCOMMON_WFS_20_NAMESPACE_URI);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, " ");
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, schemalocation);
  xsi_schemaLocation = msStringConcatenate(xsi_schemaLocation, "/wfs/2.0/wfs.xsd");
  xmlNewNsProp(psRootNode, NULL, BAD_CAST "xsi:schemaLocation", BAD_CAST xsi_schemaLocation);

  /* -------------------------------------------------------------------- */
  /*      Service metadata.                                               */
  /* -------------------------------------------------------------------- */

  /* TODO? : also add 1.1.0 and 1.0.0 as extra <ows:ServiceTypeVersion>. The OWS */
  /* schema would suggest to do so, and also the example */
  /* http://schemas.opengis.net/wfs/2.0/examples/GetCapabilities/GetCapabilities_Res_01.xml */
  /* and Deegree too, but GeoServer also only lists the current version. */
  if( msWFSIncludeSection(params, "ServiceIdentification") )
  {
    xmlAddChild(psRootNode,
                msOWSCommonServiceIdentification(psNsOws, map, "WFS", params->pszVersion, "FO"));
  }

  /*service provider*/
  if( msWFSIncludeSection(params, "ServiceProvider") )
  {
    xmlAddChild(psRootNode, msOWSCommonServiceProvider(
                            psNsOws, psNsXLink, map, "FO"));
  }

  /*operation metadata */
  if( msWFSIncludeSection(params, "OperationsMetadata") )
  {
    if ((script_url=msOWSGetOnlineResource(map, "FO", "onlineresource", req)) == NULL) {
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
                            OWS_METHOD_GETPOST, script_url));

    xmlAddChild(psMainNode, psNode);

    /*accept version*/
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                "Parameter", "AcceptVersions",
                "2.0.0,1.1.0,1.0.0"));
    /*format*/
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                "Parameter", "AcceptFormats",
                "text/xml"));
    /*sections*/
    xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                "Parameter", "Sections",
                "ServiceIdentification,ServiceProvider,OperationsMetadata,FeatureTypeList,Filter_Capabilities"));

    /* -------------------------------------------------------------------- */
    /*      DescribeFeatureType                                             */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "DescribeFeatureType", MS_TRUE)) {
        psNode = xmlAddChild(psMainNode,
                            msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"DescribeFeatureType",
                                OWS_METHOD_GETPOST, script_url));
        xmlAddChild(psMainNode, psNode);

        /*output format*/
        xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                    "Parameter", "outputFormat",
                    "application/gml+xml; version=3.2,"
                    "text/xml; subtype=gml/3.2.1,"
                    "text/xml; subtype=gml/3.1.1,"
                    "text/xml; subtype=gml/2.1.2"));
    }

    /* -------------------------------------------------------------------- */
    /*      GetFeature                                                      */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "GetFeature", MS_TRUE)) {

        psNode = xmlAddChild(psMainNode,
                            msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"GetFeature",
                                OWS_METHOD_GETPOST, script_url));
        xmlAddChild(psMainNode, psNode);

        formats_list = msWFSGetOutputFormatList( map, NULL, OWS_2_0_0 );
        xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                    "Parameter", "outputFormat",
                    formats_list));
        msFree( formats_list );
    }

    /* -------------------------------------------------------------------- */
    /*      GetPropertyValue                                                */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "GetPropertyValue", MS_TRUE)) {

        psNode = xmlAddChild(psMainNode,
                            msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"GetPropertyValue",
                                OWS_METHOD_GETPOST, script_url));
        xmlAddChild(psMainNode, psNode);

        formats_list = msWFSGetOutputFormatList( map, NULL, OWS_2_0_0 );
        xmlAddChild(psNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                    "Parameter", "outputFormat",
                    formats_list));
        msFree( formats_list );
    }

    /* -------------------------------------------------------------------- */
    /*      ListStoredQueries                                               */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "ListStoredQueries", MS_TRUE)) {

        psNode = xmlAddChild(psMainNode,
                            msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"ListStoredQueries",
                                OWS_METHOD_GETPOST, script_url));
        xmlAddChild(psMainNode, psNode);
    }

    /* -------------------------------------------------------------------- */
    /*      DescribeStoredQueries                                           */
    /* -------------------------------------------------------------------- */
    if (msOWSRequestIsEnabled(map, NULL, "F", "DescribeStoredQueries", MS_TRUE)) {

        psNode = xmlAddChild(psMainNode,
                            msOWSCommonOperationsMetadataOperation(psNsOws,psNsXLink,"DescribeStoredQueries",
                                OWS_METHOD_GETPOST, script_url));
        xmlAddChild(psMainNode, psNode);
    }

    xmlAddChild(psMainNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                "Parameter", "version",
                "2.0.0,1.1.0,1.0.0"));

    msWFSAddGlobalSRSNameParam(psMainNode, psNsOws, map);

    /* Conformance declaration */
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsBasicWFS", "TRUE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsTransactionalWFS", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsLockingWFS", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "KVPEncoding", "TRUE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "XMLEncoding", "TRUE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "SOAPEncoding", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsInheritance", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsRemoteResolve", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsResultPaging", "TRUE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsStandardJoins", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsSpatialJoins", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsTemporalJoins", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ImplementsFeatureVersioning", "FALSE"));
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "ManageStoredQueries", "FALSE"));

    /* Capacity declaration */
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "PagingIsTransactionSafe", "FALSE"));

    value = msOWSLookupMetadata(&(map->web.metadata), "FO", "maxfeatures");
    if (value) {
    xmlAddChild(psMainNode, msWFSConstraintDefaultValue(psNsOws, psNsOws, "CountDefault", value));
    }

    xmlAddChild(psMainNode, msOWSCommonOperationsMetadataDomainType(ows_version, psNsOws,
                "Constraint", "QueryExpressions","wfs:Query,wfs:StoredQuery"));
  }

  /* -------------------------------------------------------------------- */
  /*      FeatureTypeList                                                 */
  /* -------------------------------------------------------------------- */
  if( msWFSIncludeSection(params, "FeatureTypeList") )
  {
    psFtNode = xmlNewNode(NULL, BAD_CAST "FeatureTypeList");
    xmlAddChild(psRootNode, psFtNode);

    for(i=0; i<map->numlayers; i++) {
        layerObj *lp;
        lp = GET_LAYER(map, i);

        if (!msIntegerInArray(lp->index, ows_request->enabled_layers, ows_request->numlayers))
        continue;

        /* List only vector layers in which DUMP=TRUE */
        if (msWFSIsLayerSupported(lp))
          xmlAddChild(psFtNode, msWFSDumpLayer11(map, lp, psNsOws, OWS_2_0_0));
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Filter capabilities.                                            */
  /* -------------------------------------------------------------------- */
  if( msWFSIncludeSection(params, "Filter_Capabilities") )
  {
    psNsFES = xmlNewNs(NULL, BAD_CAST MS_OWSCOMMON_FES_20_NAMESPACE_URI, BAD_CAST MS_OWSCOMMON_FES_20_NAMESPACE_PREFIX);
    xmlAddChild(psRootNode, msWFS20FilterCapabilities(psNsFES, psNsOws));
  }

  /* -------------------------------------------------------------------- */
  /*      Write out the document.                                         */
  /* -------------------------------------------------------------------- */

  if( msIO_needBinaryStdout() == MS_FAILURE )
    return MS_FAILURE;

  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  context = msIO_getHandler(stdout);

  xmlDocDumpFormatMemoryEnc(psDoc, &buffer, &size, ("UTF-8"), 1);
  msIO_contextWrite(context, buffer, size);
  xmlFree(buffer);

  /*free buffer and the document */
  /*xmlFree(buffer);*/
  xmlFreeDoc(psDoc);
  if( psNsFES != NULL )
      xmlFreeNs(psNsFES);

  free(script_url);
  free(xsi_schemaLocation);
  free(schemalocation);

  xmlCleanupParser();

  return(MS_SUCCESS);
}

#endif /* defined(USE_WFS_SVR) && defined(USE_LIBXML2) */

#if defined(USE_WFS_SVR) && !defined(USE_LIBXML2)

int msWFSException20(mapObj *map, const char *locator,
                     const char *exceptionCode)
{
  /* fallback to reporting using 1.0 style exceptions. */
  return msWFSException( map, locator, exceptionCode, "1.0.0" );
}

int msWFSGetCapabilities20(mapObj *map, wfsParamsObj *params,
                           cgiRequestObj *req, owsRequestObj *ows_request)

{
  msSetError( MS_WFSERR,
              "WFS 2.0 request made, but mapserver requires libxml2 for WFS 2.0 services and this is not configured.",
              "msWFSGetCapabilities11()", "NoApplicableCode" );

  return msWFSException11(map, "mapserv", "NoApplicableCode", params->pszVersion);
}


#endif /* defined(USE_WFS_SVR) && !defined(USE_LIBXML2) */

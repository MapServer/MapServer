/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC OWS Common Implementation include file
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

#ifndef MAPOWSCOMMON_H
#define MAPOWSCOMMON_H

#ifdef USE_LIBXML2

#include <libxml/parser.h>
#include <libxml/tree.h>

#endif

/* W3C namespaces */

#define MS_OWSCOMMON_W3C_XLINK_NAMESPACE_URI "http://www.w3.org/1999/xlink"
#define MS_OWSCOMMON_W3C_XLINK_NAMESPACE_PREFIX "xlink"

#define MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI                                     \
  "http://www.w3.org/2001/XMLSchema-instance"
#define MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX "xsi"

#define MS_OWSCOMMON_W3C_XS_NAMESPACE_URI "http://www.w3.org/2001/XMLSchema"
#define MS_OWSCOMMON_W3C_XS_NAMESPACE_PREFIX "xs"

/* OGC namespaces */

#define MS_OWSCOMMON_OGC_NAMESPACE_URI "http://www.opengis.net/ogc"
#define MS_OWSCOMMON_OGC_NAMESPACE_PREFIX "ogc"

#define MS_OWSCOMMON_OWS_NAMESPACE_URI "http://www.opengis.net/ows"
#define MS_OWSCOMMON_OWS_NAMESPACE_PREFIX "ows"

#define MS_OWSCOMMON_OWS_110_NAMESPACE_URI "http://www.opengis.net/ows/1.1"

#define MS_OWSCOMMON_OWS_20_NAMESPACE_URI "http://www.opengis.net/ows/2.0"
#define MS_OWSCOMMON_OWS_20_SCHEMAS_LOCATION "/ows/2.0/owsAll.xsd"

/* OGC URNs */

#define MS_OWSCOMMON_URN_OGC_CRS_4326 "urn:opengis:def:crs:OGC:2:84"

/* default OGC Schemas Location */

#define MS_OWSCOMMON_SCHEMAS_LOCATION "http://schemas.opengis.net"

/* OGC codespace */

#define MS_OWSCOMMON_OGC_CODESPACE "OGC"

/* WCS namespaces */

#define MS_OWSCOMMON_WCS_20_NAMESPACE_URI "http://www.opengis.net/wcs/2.0"
#define MS_OWSCOMMON_WCS_20_SCHEMAS_LOCATION "/wcs/2.0/wcsAll.xsd"
#define MS_OWSCOMMON_WCS_NAMESPACE_PREFIX "wcs"

/* GML namespaces */

#define MS_OWSCOMMON_GML_NAMESPACE_URI "http://www.opengis.net/gml"
#define MS_OWSCOMMON_GML_NAMESPACE_PREFIX "gml"

#define MS_OWSCOMMON_GML_32_NAMESPACE_URI "http://www.opengis.net/gml/3.2"

#define MS_OWSCOMMON_GML_212_SCHEMA_LOCATION "/gml/2.1.2/feature.xsd"
#define MS_OWSCOMMON_GML_311_SCHEMA_LOCATION "/gml/3.1.1/base/gml.xsd"
#define MS_OWSCOMMON_GML_321_SCHEMA_LOCATION "/gml/3.2.1/gml.xsd"

/* WFS namespaces */

#define MS_OWSCOMMON_WFS_NAMESPACE_PREFIX "wfs"
#define MS_OWSCOMMON_WFS_NAMESPACE_URI "http://www.opengis.net/wfs"
#define MS_OWSCOMMON_WFS_20_NAMESPACE_URI "http://www.opengis.net/wfs/2.0"

#define MS_OWSCOMMON_WFS_10_SCHEMA_LOCATION "/wfs/1.0.0/WFS-basic.xsd"
#define MS_OWSCOMMON_WFS_11_SCHEMA_LOCATION "/wfs/1.1.0/wfs.xsd"
#define MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION "/wfs/2.0/wfs.xsd"

/* FES namespaces */

#define MS_OWSCOMMON_FES_20_NAMESPACE_PREFIX "fes"
#define MS_OWSCOMMON_FES_20_NAMESPACE_URI "http://www.opengis.net/fes/2.0"

#define MS_OWSCOMMON_FES_20_SCHEMA_LOCATION "/filter/2.0/filterAll.xsd"

/* GMLCov namespaces */

#define MS_OWSCOMMON_GMLCOV_10_NAMESPACE_URI "http://www.opengis.net/gmlcov/1.0"
#define MS_OWSCOMMON_GMLCOV_NAMESPACE_PREFIX "gmlcov"

/* SWE namespaces */
#define MS_OWSCOMMON_SWE_20_NAMESPACE_URI "http://www.opengis.net/swe/2.0"
#define MS_OWSCOMMON_SWE_NAMESPACE_PREFIX "swe"

/* Inspire namespaces */

#define MS_INSPIRE_COMMON_NAMESPACE_URI                                        \
  "http://inspire.ec.europa.eu/schemas/common/1.0"
#define MS_INSPIRE_COMMON_NAMESPACE_PREFIX "inspire_common"
#define MS_INSPIRE_COMMON_SCHEMA_LOCATION "/common/1.0/common.xsd"

#define MS_INSPIRE_VS_NAMESPACE_URI                                            \
  "http://inspire.ec.europa.eu/schemas/inspire_vs/1.0"
#define MS_INSPIRE_VS_NAMESPACE_PREFIX "inspire_vs"
#define MS_INSPIRE_VS_SCHEMA_LOCATION "/inspire_vs/1.0/inspire_vs.xsd"

#define MS_INSPIRE_DLS_NAMESPACE_URI                                           \
  "http://inspire.ec.europa.eu/schemas/inspire_dls/1.0"
#define MS_INSPIRE_DLS_NAMESPACE_PREFIX "inspire_dls"
#define MS_INSPIRE_DLS_SCHEMA_LOCATION "/inspire_dls/1.0/inspire_dls.xsd"

/* MapServer namespaces */
#define MS_DEFAULT_NAMESPACE_PREFIX "ms"
#define MS_DEFAULT_NAMESPACE_URI "http://mapserver.gis.umn.edu/mapserver"

/* OWS errors */

/* OWS 1.1.0 Table 25 */
#define MS_OWS_ERROR_OPERATION_NOT_SUPPORTED "OperationNotSupported"
#define MS_OWS_ERROR_MISSING_PARAMETER_VALUE "MissingParameterValue"
#define MS_OWS_ERROR_INVALID_PARAMETER_VALUE "InvalidParameterValue"
#define MS_OWS_ERROR_VERSION_NEGOTIATION_FAILED "VersionNegotiationFailed"
#define MS_OWS_ERROR_INVALID_UPDATE_SEQUENCE "InvalidUpdateSequence"
#define MS_OWS_ERROR_OPTION_NOT_SUPPORTED "OptionNotSupported"
#define MS_OWS_ERROR_NO_APPLICABLE_CODE "NoApplicableCode"

#define MS_OWS_ERROR_NOT_FOUND "NotFound"

#define MS_WFS_ERROR_OPERATION_PROCESSING_FAILED "OperationProcessingFailed"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_LIBXML2

/* function prototypes */

xmlNodePtr msOWSCommonServiceIdentification(xmlNsPtr psNsOws, mapObj *map,
                                            const char *servicetype,
                                            const char *version,
                                            const char *namespaces,
                                            const char *validated_language);

xmlNodePtr msOWSCommonServiceProvider(xmlNsPtr psNsOws, xmlNsPtr psXLinkNs,
                                      mapObj *map, const char *namespaces,
                                      const char *validated_language);

xmlNodePtr msOWSCommonOperationsMetadata(xmlNsPtr psNsOws);

#define OWS_METHOD_GET 1
#define OWS_METHOD_POST 2
#define OWS_METHOD_GETPOST 3

xmlNodePtr msOWSCommonOperationsMetadataOperation(xmlNsPtr psNsOws,
                                                  xmlNsPtr psXLinkNs,
                                                  const char *name, int method,
                                                  const char *url);

xmlNodePtr msOWSCommonOperationsMetadataDomainType(int version,
                                                   xmlNsPtr psNsOws,
                                                   const char *elname,
                                                   const char *name,
                                                   const char *values);

xmlNodePtr msOWSCommonExceptionReport(xmlNsPtr psNsOws, int ows_version,
                                      const char *schemas_location,
                                      const char *version, const char *language,
                                      const char *exceptionCode,
                                      const char *locator,
                                      const char *ExceptionText);

xmlNodePtr msOWSCommonBoundingBox(xmlNsPtr psNsOws, const char *crs,
                                  int dimensions, double minx, double miny,
                                  double maxx, double maxy);

xmlNodePtr msOWSCommonWGS84BoundingBox(xmlNsPtr psNsOws, int dimensions,
                                       double minx, double miny, double maxx,
                                       double maxy);

int _validateNamespace(xmlNsPtr psNsOws);

int msOWSSchemaValidation(const char *xml_schema, const char *xml);

#endif /* defined(USE_LIBXML2) */

int msOWSCommonNegotiateVersion(int requested_version,
                                const int supported_versions[],
                                int num_supported_versions);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* MAPOWSCOMMON_H */

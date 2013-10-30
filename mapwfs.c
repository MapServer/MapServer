/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  WFS server implementation
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2002, Daniel Morissette, DM Solutions Group Inc
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



#if defined(USE_WFS_SVR)

/* There is a dependency to GDAL/OGR for the GML driver and MiniXML parser */
#include "cpl_minixml.h"
#include "cpl_conv.h"

#include "mapogcfilter.h"
#include "mapowscommon.h"
#include "maptemplate.h"

#if defined(USE_LIBXML2)
#include "maplibxml2.h"
#endif

/* Must be sorted from more recent to older one */
static const int wfsSupportedVersions[] = {OWS_2_0_0, OWS_1_1_0, OWS_1_0_0};
static const char* const wfsSupportedVersionsStr[] = { "2.0.0", "1.1.0", "1.0.0" };
static const int wfsNumSupportedVersions =
    (int)(sizeof(wfsSupportedVersions)/sizeof(wfsSupportedVersions[0]));
static const char* const wfsUnsupportedOperations[] = { "GetFeatureWithLock",
                                                  "LockFeature",
                                                  "Transaction",
                                                  "CreateStoredQuery",
                                                  "DropStoredQuery" };
static const int wfsNumUnsupportedOperations =
    (int)(sizeof(wfsUnsupportedOperations)/sizeof(wfsUnsupportedOperations[0]));

#define WFS_LATEST_VERSION  wfsSupportedVersionsStr[0]

#define WFS_DEFAULT_NAMESPACE_PREFIX "ms"
#define WFS_DEFAULT_NAMESPACE_URI    "http://mapserver.gis.umn.edu/mapserver"

/* Supported DescribeFeature formats */
typedef enum
{
    OWS_DEFAULT_SCHEMA, /* basically a GML 2.1 schema */
    OWS_SFE_SCHEMA, /* GML for simple feature exchange (formerly GML3L0) */
    OWS_GML32_SFE_SCHEMA /* GML 3.2 Simple Features Level 0 */
} WFSSchemaVersion;

/*
** msWFSGetIndexUnsupportedOperation()
**
** Return index of pszOp in wfsUnsupportedOperations, or -1 otherwise
*/

static int msWFSGetIndexUnsupportedOperation(const char* pszOp)
{
    int i;
    for(i = 0; i < wfsNumUnsupportedOperations; i++ )
    {
        if( strcasecmp(wfsUnsupportedOperations[i], pszOp) == 0 )
            return i;
    }
    return -1;
}


static
const char *msWFSGetDefaultVersion(mapObj *map);

/*
** msWFSException()
**
** Report current MapServer error in XML exception format.
*/

static
int msWFSExceptionInternal(mapObj *map, const char *locator, const char *code,
                           const char *version, int locatorShouldBeNull )
{
  char *schemalocation = NULL;
  /* In WFS, exceptions are always XML.
  */

  if( version == NULL )
    version = msWFSGetDefaultVersion(map);

  if( msOWSParseVersionString(version) >= OWS_2_0_0 )
    return msWFSException20( map, (locatorShouldBeNull) ? NULL : locator, code );
  if( msOWSParseVersionString(version) >= OWS_1_1_0 )
    return msWFSException11( map, (locatorShouldBeNull) ? NULL : locator, code, version );

  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" ?>\n");

  msIO_printf("<ServiceExceptionReport ");
  msIO_printf("version=\"1.2.0\" ");
  msIO_printf("xmlns=\"http://www.opengis.net/ogc\" ");
  msIO_printf("xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
  schemalocation = msEncodeHTMLEntities(msOWSGetSchemasLocation(map));
  msIO_printf("xsi:schemaLocation=\"http://www.opengis.net/ogc %s/wfs/1.0.0/OGC-exception.xsd\">\n", schemalocation);
  free(schemalocation);
  msIO_printf("  <ServiceException code=\"%s\" locator=\"%s\">\n", code, locator);
  /* Optional <Locator> element currently unused. */
  /* msIO_printf("    <Message>\n"); */
  msWriteErrorXML(stdout);
  /* msIO_printf("    </Message>\n"); */
  msIO_printf("  </ServiceException>\n");
  msIO_printf("</ServiceExceptionReport>\n");

  return MS_FAILURE; /* so we can call 'return msWFSException();' anywhere */
}

int msWFSException(mapObj *map, const char *locator, const char *code,
                   const char *version )
{
    return msWFSExceptionInternal(map, locator, code, version, FALSE);
}


/*
** msWFSExceptionNoLocator()
**
** Report current MapServer error in XML exception format.
** For WFS >= 1.1.0, the locator will be ignored. It will be just used
** for legacy WFS 1.0 exceptions.
*/

static
int msWFSExceptionNoLocator(mapObj *map, const char *locator, const char *code,
                   const char *version )
{
    return msWFSExceptionInternal(map, locator, code, version, TRUE);
}

/*
** Helper function to build a list of output formats.
**
** Given a layer it will return all formats valid for that layer, otherwise
** all formats permitted on layers in the map are returned.  The string
** returned should be freed by the caller.
*/

char *msWFSGetOutputFormatList(mapObj *map, layerObj *layer, int nWFSVersion )
{
  int i, got_map_list = 0;
  static const int out_list_size = 20000;
  char *out_list = (char*) msSmallCalloc(1,out_list_size);

  if( nWFSVersion == OWS_1_0_0 )
      strcpy(out_list,"GML2");
  else if( nWFSVersion == OWS_1_1_0 )
      strcpy(out_list,"text/xml; subtype=gml/3.1.1");
  else
      strcpy(out_list,"application/gml+xml; version=3.2,"
                      "text/xml; subtype=gml/3.2.1,"
                      "text/xml; subtype=gml/3.1.1,"
                      "text/xml; subtype=gml/2.1.2");

  for( i = 0; i < map->numlayers; i++ ) {
    const char *format_list;
    layerObj *lp;
    int j, n;
    char **tokens;

    lp = GET_LAYER(map, i);
    if( layer != NULL && layer != lp )
      continue;

    format_list = msOWSLookupMetadata(&(lp->metadata),
                                      "F","getfeature_formatlist");

    if( format_list == NULL && !got_map_list ) {
      format_list = msOWSLookupMetadata(&(map->web.metadata),
                                        "F","getfeature_formatlist");
      got_map_list = 1;
    }

    if( format_list == NULL )
      continue;

    n = 0;
    tokens = msStringSplit(format_list, ',', &n);

    for( j = 0; j < n; j++ ) {
      int iformat;
      const char *fname, *hit;
      outputFormatObj *format_obj;

      msStringTrim( tokens[j] );
      iformat = msGetOutputFormatIndex(map,tokens[j]);
      if( iformat < 0 )
        continue;

      format_obj = map->outputformatlist[iformat];

      fname = format_obj->name;
      if( nWFSVersion >= OWS_1_1_0
          && format_obj->mimetype != NULL )
        fname = format_obj->mimetype;

      hit = strstr(out_list,fname);
      if( hit != NULL
          && (hit[strlen(fname)] == '\0' || hit[strlen(fname)] == ','))
        continue;

      if( strlen(out_list) + strlen(fname)+3 < out_list_size ) {
        strcat( out_list, "," );
        strcat( out_list, fname );
      } else
        break;
    }

    msFreeCharArray( tokens, n );
  }

  return out_list;
}

/*
**
*/
static void msWFSPrintRequestCap(const char *wmtver, const char *request,
                                 const char *script_url,
                                 const char *format_tag,
                                 const char *formats_list)
{
  msIO_printf("    <%s>\n", request);

  /* We expect to receive a NULL-terminated args list of formats */
  if (format_tag != NULL) {
    int i, n;
    char **tokens;

    n = 0;
    tokens = msStringSplit(formats_list, ',', &n);

    msIO_printf("      <%s>\n", format_tag);

    for( i = 0; i < n; i++ ) {
      msIO_printf("        <%s/>\n", tokens[i] );
    }

    msFreeCharArray( tokens, n );

    msIO_printf("      </%s>\n", format_tag);
  }

  msIO_printf("      <DCPType>\n");
  msIO_printf("        <HTTP>\n");
  msIO_printf("          <Get onlineResource=\"%s\" />\n", script_url);
  msIO_printf("        </HTTP>\n");
  msIO_printf("      </DCPType>\n");
  msIO_printf("      <DCPType>\n");
  msIO_printf("        <HTTP>\n");
  msIO_printf("          <Post onlineResource=\"%s\" />\n", script_url);
  msIO_printf("        </HTTP>\n");
  msIO_printf("      </DCPType>\n");


  msIO_printf("    </%s>\n", request);
}


/* msWFSLocateSRSInList()
**
** Utility function to check if a space separated list contains  the one passed in argument.
**  The list comes normaly from ows_srs metadata, and is expected to use the simple EPSG notation
** (EPSG:4326 ESPG:42304 ...). The srs comes from the query string and can either
** be of simple EPSG format or using gc:def:crs:EPSG:xxx format
*/
int msWFSLocateSRSInList(const char *pszList, const char *srs)
{
  int nTokens,i;
  char **tokens = NULL;
  int bFound = MS_FALSE;
  char epsg_string[100];
  const char *code;

  if (!pszList || !srs)
    return MS_FALSE;

  if (strncasecmp(srs, "EPSG:",5) == 0)
    code = srs+5;
  else if (strncasecmp(srs, "urn:ogc:def:crs:EPSG:",21) == 0) {
    if (srs[21] == ':')
      code = srs+21;
    else
      code = srs+20;

    while( *code != ':' && *code != '\0')
      code++;
    if( *code == ':' )
      code++;
  } else if (strncasecmp(srs, "urn:EPSG:geographicCRS:",23) == 0)
    code = srs + 23;
  else
    return MS_FALSE;

  snprintf( epsg_string, sizeof(epsg_string), "EPSG:%s", code );

  tokens = msStringSplit(pszList, ' ', &nTokens );
  if (tokens && nTokens > 0) {
    for (i=0; i<nTokens; i++) {
      if (strcasecmp(tokens[i],  epsg_string) == 0) {
        bFound = MS_TRUE;
        break;
      }
    }
    msFreeCharArray(tokens, nTokens);
  }

  return bFound;
}

/* msWFSGetFeatureApplySRS()
**
** Utility function called from msWFSGetFeature. It is assumed that at this point
** all queried layers are turned ON
*/
static int msWFSGetFeatureApplySRS(mapObj *map, const char *srs, int nWFSVersion)
{
  const char *pszLayerSRS=NULL;
  const char *pszMapSRS=NULL;
  char *pszOutputSRS=NULL;
  layerObj *lp;
  int i;

  /*validation of SRS
    - wfs 1.0 does not have an srsname parameter so all queried layers should be advertized using the
      same srs. For wfs 1.1.0 an srsName can be passed, we should validate that It is valid for all
      queries layers
  */
  pszMapSRS = msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FO", MS_TRUE);
  if(pszMapSRS && nWFSVersion >  OWS_1_0_0)
    msLoadProjectionStringEPSG(&(map->projection), pszMapSRS);

  if (srs == NULL || nWFSVersion == OWS_1_0_0) {
    for (i=0; i<map->numlayers; i++) {
      lp = GET_LAYER(map, i);
      if (lp->status != MS_ON)
        continue;

      if (pszMapSRS)
        pszLayerSRS = pszMapSRS;
      else
        pszLayerSRS = msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FO", MS_TRUE);

      if (pszLayerSRS == NULL) {
        msSetError(MS_WFSERR,
                   "Server config error: SRS must be set at least at the map or at the layer level.",
                   "msWFSGetFeature()");
        if (pszOutputSRS)
          msFree(pszOutputSRS);
        return MS_FAILURE;
      }
      if (pszOutputSRS == NULL)
        pszOutputSRS = msStrdup(pszLayerSRS);
      else if (strcasecmp(pszLayerSRS,pszOutputSRS) != 0) {
        msSetError(MS_WFSERR,
                   "Invalid GetFeature Request: All TYPENAMES in a single GetFeature request must have been advertized in the same SRS.  Please check the capabilities and reformulate your request.",
                   "msWFSGetFeature()");
        if (pszOutputSRS)
          msFree(pszOutputSRS);
        return MS_FAILURE;
      }

    }
  } else { /*srs is given so it should be valid for all layers*/
    /*get all the srs defined at the map level and check them aginst the srsName passed
      as argument*/
    pszMapSRS = msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FO", MS_FALSE);
    if (pszMapSRS) {
      if (!msWFSLocateSRSInList(pszMapSRS, srs)) {
        msSetError(MS_WFSERR,
                   "Invalid GetFeature Request:Invalid SRS.  Please check the capabilities and reformulate your request.",
                   "msWFSGetFeature()");
        return MS_FAILURE;
      }
      pszOutputSRS = msStrdup(srs);
    } else {
      for (i=0; i<map->numlayers; i++) {
        lp = GET_LAYER(map, i);
        if (lp->status != MS_ON)
          continue;

        pszLayerSRS = msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FO", MS_FALSE);
        if (!pszLayerSRS) {
          msSetError(MS_WFSERR,
                     "Server config error: SRS must be set at least at the map or at the layer level.",
                     "msWFSGetFeature()");
          return MS_FAILURE;
        }
        if (!msWFSLocateSRSInList(pszLayerSRS, srs)) {
          msSetError(MS_WFSERR,
                     "Invalid GetFeature Request:Invalid SRS.  Please check the capabilities and reformulate your request.",
                     "msWFSGetFeature()");
          return MS_FAILURE;
        }
      }
      pszOutputSRS = msStrdup(srs);
    }
  }

  if (pszOutputSRS && nWFSVersion >= OWS_1_1_0) {
    projectionObj sProjTmp;
    int nTmp=0;

    msInitProjection(&sProjTmp);
    nTmp = msLoadProjectionStringEPSG(&(sProjTmp), pszOutputSRS);
    if (nTmp == 0) {
      msProjectRect(&(map->projection), &(sProjTmp), &map->extent);
    }
    msFreeProjection(&(sProjTmp));
    /*check if the srs passed is valid. Assuming that it is an EPSG:xxx format,
      Or urn:ogc:def:crs:EPSG:xxx format. */
    if (strncasecmp(pszOutputSRS, "EPSG:", 5) == 0 ||
        strncasecmp(pszOutputSRS, "urn:ogc:def:crs:EPSG:",21) == 0) {
      /*we load the projection sting in the map and possibly
        set the axis order*/
      msFreeProjection(&map->projection);
      msLoadProjectionStringEPSG(&(map->projection), pszOutputSRS);
    } else if (strncasecmp(pszOutputSRS, "urn:EPSG:geographicCRS:",23) == 0) {
      char epsg_string[100];
      const char *code;


      code = pszOutputSRS + 23;

      snprintf( epsg_string, sizeof(epsg_string), "EPSG:%s", code );

      /*we load the projection sting in the map and possibly
        set the axis order*/
      /*reproject the map extent from current projection to output projection*/

      msFreeProjection(&map->projection);
      msLoadProjectionStringEPSG(&(map->projection), epsg_string);
    }
  }
  /* Set map output projection to which output features should be reprojected */
  else if (pszOutputSRS && strncasecmp(pszOutputSRS, "EPSG:", 5) == 0) {
    int nTmp =0;
    projectionObj sProjTmp;

    /*reproject the map extent from current projection to output projection*/
    msInitProjection(&sProjTmp);
    if (nWFSVersion >= OWS_1_1_0)
      nTmp = msLoadProjectionStringEPSG(&(sProjTmp), pszOutputSRS);
    else
      nTmp = msLoadProjectionString(&(sProjTmp), pszOutputSRS);

    if (nTmp == 0)
      msProjectRect(&(map->projection), &(sProjTmp), &map->extent);
    msFreeProjection(&(sProjTmp));

    msFreeProjection(&map->projection);
    msInitProjection(&map->projection);

    if (nWFSVersion >= OWS_1_1_0)
      nTmp = msLoadProjectionStringEPSG(&(map->projection), pszOutputSRS);
    else
      nTmp = msLoadProjectionString(&(map->projection), pszOutputSRS);

    if (nTmp != 0) {
      msSetError(MS_WFSERR, "msLoadProjectionString() failed", "msWFSGetFeature()");
      return MS_FAILURE;
    }
  }


  if (pszOutputSRS)
    msFree(pszOutputSRS);
  return MS_SUCCESS;
}


/* msWFSIsLayerSupported()
**
** Returns true (1) is this layer meets the requirements to be served as
** a WFS feature type.
*/
int msWFSIsLayerSupported(layerObj *lp)
{
  /* In order to be supported, lp->type must be specified, even for
  ** layers that are OGR, SDE, SDO, etc connections.
  */
  if ((lp->type == MS_LAYER_POINT ||
       lp->type == MS_LAYER_LINE ||
       lp->type == MS_LAYER_POLYGON ) &&
      lp->connectiontype != MS_WMS &&
      lp->connectiontype != MS_GRATICULE) {
    return 1; /* true */
  }

  return 0; /* false */
}

/*
** msWFSDumpLayer()
*/
int msWFSDumpLayer(mapObj *map, layerObj *lp)
{
  rectObj ext;
  const char *pszWfsSrs = NULL;
  projectionObj poWfs;

  msIO_printf("    <FeatureType>\n");

  if (lp->name && strlen(lp->name) > 0 &&
      (msIsXMLTagValid(lp->name) == MS_FALSE || isdigit(lp->name[0])))
    msIO_fprintf(stdout, "<!-- WARNING: The layer name '%s' might contain spaces or "
                 "invalid characters or may start with a number. This could lead to potential problems. -->\n",lp->name);

  msOWSPrintEncodeParam(stdout, "LAYER.NAME", lp->name, OWS_WARN,
                        "        <Name>%s</Name>\n", NULL);

  msOWSPrintEncodeMetadata(stdout, &(lp->metadata), "FO", "title",
                           OWS_WARN, "        <Title>%s</Title>\n", lp->name);

  msOWSPrintEncodeMetadata(stdout, &(lp->metadata), "FO", "abstract",
                           OWS_NOERR, "        <Abstract>%s</Abstract>\n", NULL);

  msOWSPrintEncodeMetadataList(stdout, &(lp->metadata), "FO",
                               "keywordlist",
                               "        <Keywords>\n",
                               "        </Keywords>\n",
                               "          %s\n", NULL);

  /* In WFS, every layer must have exactly one SRS and there is none at */
  /* the top level contrary to WMS */
  /*  */
  /* So here is the way we'll deal with SRS: */
  /* 1- If a top-level map projection (or wfs_srs metadata) is set then */
  /* all layers are advertized in the map's projection and they will */
  /* be reprojected on the fly in the GetFeature request. */
  /* 2- If there is no top-level map projection (or wfs_srs metadata) then */
  /* each layer is advertized in its own projection as defined in the */
  /* layer's projection object or wfs_srs metadata. */
  /*  */
  if (msOWSGetEPSGProj(&(map->projection),&(map->web.metadata),"FO",MS_TRUE) != NULL) {
    /* Map has a SRS.  Use it for all layers. */
    pszWfsSrs = msOWSGetEPSGProj(&(map->projection),&(map->web.metadata), "FO", MS_TRUE);
  } else {
    /* Map has no SRS.  Use layer SRS or produce a warning. */
    pszWfsSrs = msOWSGetEPSGProj(&(lp->projection),&(lp->metadata), "FO", MS_TRUE);
  }

  msOWSPrintEncodeParam(stdout, "(at least one of) MAP.PROJECTION, LAYER.PROJECTION or wfs_srs metadata",
                        pszWfsSrs, OWS_WARN, "        <SRS>%s</SRS>\n", NULL);

  /* If layer has no proj set then use map->proj for bounding box. */
  if (msOWSGetLayerExtent(map, lp, "FO", &ext) == MS_SUCCESS) {
    msInitProjection(&poWfs);
    if (pszWfsSrs != NULL)
      msLoadProjectionString(&(poWfs), pszWfsSrs);

    if(lp->projection.numargs > 0) {
      msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext),
                                  &(lp->projection), &(poWfs), OWS_WFS);
    } else {
      msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext),
                                  &(map->projection), &(poWfs), OWS_WFS);
    }
    msFreeProjection(&poWfs);
  } else {
    msIO_printf("<!-- WARNING: Optional LatLongBoundingBox could not be established for this layer.  Consider setting the EXTENT in the LAYER object, or wfs_extent metadata. Also check that your data exists in the DATA statement -->\n");
  }

  msOWSPrintURLType(stdout, &(lp->metadata), "FO", "metadataurl",
                    OWS_NOERR, NULL, "MetadataURL", " type=\"%s\"",
                    NULL, NULL, " format=\"%s\"", "%s",
                    MS_TRUE, MS_FALSE, MS_FALSE, MS_TRUE, MS_TRUE,
                    NULL, NULL, NULL, NULL, NULL, "        ");

  if (msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid") == NULL) {
    msIO_fprintf(stdout, "<!-- WARNING: Required Feature Id attribute (fid) not specified for this feature type. Make sure you set one of wfs_featureid, ows_featureid or gml_featureid metadata. -->\n");
  }

  msIO_printf("    </FeatureType>\n");

  return MS_SUCCESS;
}

/*
** msWFSHandleUpdateSequence()
*/
int msWFSHandleUpdateSequence(mapObj *map, wfsParamsObj *params, const char* pszFunction)
{
  /* -------------------------------------------------------------------- */
  /*      Handle updatesequence                                           */
  /* -------------------------------------------------------------------- */

  const char* updatesequence = msOWSLookupMetadata(&(map->web.metadata), "FO", "updatesequence");

  if (params->pszUpdateSequence != NULL) {
    int i = msOWSNegotiateUpdateSequence(params->pszUpdateSequence, updatesequence);
    if (i == 0) { /* current */
      msSetError(MS_WFSERR, "UPDATESEQUENCE parameter (%s) is equal to server (%s)", pszFunction, params->pszUpdateSequence, updatesequence);
      /* FIXME? : according to table 7 of OWS 1.1, we should return a service */
      /* metadata document with only “version” and “updateSequence” parameters */
      return msWFSException(map, "updatesequence", "CurrentUpdateSequence", params->pszVersion);
    }
    if (i > 0) { /* invalid */
      msSetError(MS_WFSERR, "UPDATESEQUENCE parameter (%s) is higher than server (%s)", pszFunction, params->pszUpdateSequence, updatesequence);
      /* locator must be NULL. See Table 25 of OWS 1.1 */
      return msWFSExceptionNoLocator(map, "updatesequence", "InvalidUpdateSequence", params->pszVersion);
    }
  }
  
  return MS_SUCCESS;
}


/*
** msWFSGetCapabilitiesNegotiateVersion()
*/
static int msWFSGetCapabilitiesNegotiateVersion(mapObj *map, wfsParamsObj *wfsparams)
{
  int iVersion = -1;
  char tmpString[OWS_VERSION_MAXLEN];

  /* acceptversions: do OWS Common style of version negotiation */
  if (wfsparams->pszAcceptVersions && strlen(wfsparams->pszAcceptVersions) > 0) {
    char **tokens;
    int i, j;

    tokens = msStringSplit(wfsparams->pszAcceptVersions, ',', &j);
    for (i=0; i<j; i++) {
      iVersion = msOWSParseVersionString(tokens[i]);

      if (iVersion < 0) {
        msSetError(MS_WFSERR, "Invalid version format : %s.", "msWFSGetCapabilities()", tokens[i]);
        msFreeCharArray(tokens, j);
        return msWFSException(map, "acceptversions", "InvalidParameterValue",NULL);
      }

      /* negotiate version */
      iVersion = msOWSCommonNegotiateVersion(iVersion, wfsSupportedVersions, wfsNumSupportedVersions);
      if (iVersion != -1)
        break;
    }
    msFreeCharArray(tokens, j);
    if(iVersion == -1) {
      msSetError(MS_WFSERR, "ACCEPTVERSIONS list (%s) does not match supported versions", "msWFSGetCapabilities()", wfsparams->pszAcceptVersions);
      /* locator must be NULL. See Table 25 of OWS 1.1 */
      return msWFSExceptionNoLocator(map, "acceptversions", "VersionNegotiationFailed",NULL);
    }
  } else {
    /* negotiate version */
    int tmpInt;
    iVersion = msOWSParseVersionString(wfsparams->pszVersion);
    if( iVersion < 0 )
    {
        return msWFSException(map, "version", "InvalidParameterValue", NULL);
    }
    tmpInt = msOWSCommonNegotiateVersion(iVersion, wfsSupportedVersions, wfsNumSupportedVersions);
    /* Old style negociation : paragraph D.11 of OWS 1.1.0 spec */
    if( tmpInt < 0 )
    {
        int i;
        for( i = 0 ; i < wfsNumSupportedVersions; i++ )
        {
            if( iVersion >= wfsSupportedVersions[i] )
            {
                iVersion = wfsSupportedVersions[i];
                break;
            }
        }
        if( i == wfsNumSupportedVersions )
            iVersion = wfsSupportedVersions[wfsNumSupportedVersions - 1];
    }
  }

  /* set result as string and carry on */
  if (wfsparams->pszVersion)
    msFree(wfsparams->pszVersion);
  wfsparams->pszVersion = msStrdup(msOWSGetVersionString(iVersion, tmpString));

  return MS_SUCCESS;
}


/*
** msWFSGetCapabilities()
*/
int msWFSGetCapabilities(mapObj *map, wfsParamsObj *wfsparams, cgiRequestObj *req, owsRequestObj *ows_request)
{
  char *script_url=NULL, *script_url_encoded;
  const char *updatesequence=NULL;
  const char *wmtver=NULL;
  char *formats_list;
  int ret;
  int iVersion;
  int i=0;
  
  ret = msWFSGetCapabilitiesNegotiateVersion(map, wfsparams);
  if( ret != MS_SUCCESS )
      return ret;

  iVersion = msOWSParseVersionString(wfsparams->pszVersion);
  if( iVersion == OWS_2_0_0 )
    return msWFSGetCapabilities20( map, wfsparams, req, ows_request);
  if( iVersion == OWS_1_1_0 )
    return msWFSGetCapabilities11( map, wfsparams, req, ows_request);

  /* Decide which version we're going to return... only 1.0.0 for now */
  wmtver = "1.0.0";

  /* We need this server's onlineresource. */
  if ((script_url=msOWSGetOnlineResource(map, "FO", "onlineresource", req)) == NULL ||
      (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL) {
    msSetError(MS_WFSERR, "Server URL not found", "msWFSGetCapabilities()");
    return msWFSException(map, "mapserv", "NoApplicableCode", wmtver);
  }
  free(script_url);
  script_url = NULL;

  ret = msWFSHandleUpdateSequence(map, wfsparams, "msWFSGetCapabilities()");
  if( ret != MS_SUCCESS )
  {
      free(script_url_encoded);
      return ret;
  }

  msIO_setHeader("Content-Type","text/xml; charset=UTF-8");
  msIO_sendHeaders();

  msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" ?>\n");

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "FO", "updatesequence");
  msIO_printf("<WFS_Capabilities \n"
              "   version=\"%s\" \n"
              "   updateSequence=\"%s\" \n"
              "   xmlns=\"http://www.opengis.net/wfs\" \n"
              "   xmlns:ogc=\"http://www.opengis.net/ogc\" \n"
              "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
              "   xsi:schemaLocation=\"http://www.opengis.net/wfs %s/wfs/%s/WFS-capabilities.xsd\">\n",
              wmtver, updatesequence ? updatesequence : "0",
              msOWSGetSchemasLocation(map), wmtver);

  /* Report MapServer Version Information */
  msIO_printf("\n<!-- %s -->\n\n", msGetVersion());

  /*
  ** SERVICE definition
  */
  msIO_printf("<Service>\n");
  msIO_printf("  <Name>MapServer WFS</Name>\n");

  /* the majority of this section is dependent on appropriately named metadata in the WEB object */
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "FO", "title",
                           OWS_WARN, "  <Title>%s</Title>\n", map->name);
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "FO", "abstract",
                           OWS_NOERR, "  <Abstract>%s</Abstract>\n", NULL);

  msOWSPrintEncodeMetadataList(stdout, &(map->web.metadata), "FO",
                               "keywordlist",
                               "  <Keywords>\n", "  </Keywords>\n",
                               "    %s\n", NULL);

  /* Service/onlineresource */
  /* Defaults to same as request onlineresource if wfs_service_onlineresource  */
  /* is not set. */
  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata),
                           "FO", "service_onlineresource", OWS_NOERR,
                           "  <OnlineResource>%s</OnlineResource>\n",
                           script_url_encoded);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "FO", "fees",
                           OWS_NOERR, "  <Fees>%s</Fees>\n", NULL);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "FO",
                           "accessconstraints", OWS_NOERR,
                           "  <AccessConstraints>%s</AccessConstraints>\n",
                           NULL);

  msIO_printf("</Service>\n\n");

  /*
  ** CAPABILITY definitions: list of supported requests
  */

  msIO_printf("<Capability>\n");

  msIO_printf("  <Request>\n");
  msWFSPrintRequestCap(wmtver, "GetCapabilities", script_url_encoded,
                       NULL, NULL);
  /* msWFSPrintRequestCap(wmtver, "DescribeFeatureType", script_url_encoded, "SchemaDescriptionLanguage", "XMLSCHEMA", "SFE_XMLSCHEMA", NULL); */
  /* msWFSPrintRequestCap(wmtver, "GetFeature", script_url_encoded, "ResultFormat", "GML2", "GML3", NULL); */

  /* don't advertise the GML3 or GML for SFE support */
  if (msOWSRequestIsEnabled(map, NULL, "F", "DescribeFeatureType", MS_TRUE))
    msWFSPrintRequestCap(wmtver, "DescribeFeatureType", script_url_encoded, "SchemaDescriptionLanguage", "XMLSCHEMA" );

  if (msOWSRequestIsEnabled(map, NULL, "F", "GetFeature", MS_TRUE)) {
    formats_list = msWFSGetOutputFormatList( map, NULL, OWS_1_0_0 );
    msWFSPrintRequestCap(wmtver, "GetFeature", script_url_encoded, "ResultFormat", formats_list );
    msFree( formats_list );
  }

  msIO_printf("  </Request>\n");
  msIO_printf("</Capability>\n\n");

  /*
  ** FeatureTypeList: layers
  */

  msIO_printf("<FeatureTypeList>\n");

  /* Operations supported... set default at top-level, and more operations */
  /* can be added inside each layer... for MapServer only query is supported */
  msIO_printf("  <Operations>\n");
  msIO_printf("    <Query/>\n");
  msIO_printf("  </Operations>\n");

  for(i=0; i<map->numlayers; i++) {
    layerObj *lp;
    lp = GET_LAYER(map, i);

    if (lp->status == MS_DELETE)
      continue;

    if (!msIntegerInArray(lp->index, ows_request->enabled_layers, ows_request->numlayers))
      continue;

    if (msWFSIsLayerSupported(lp)) {
      msWFSDumpLayer(map, lp);
    }
  }

  msIO_printf("</FeatureTypeList>\n\n");


  /*
  ** OGC Filter Capabilities ... for now we support only BBOX
  */

  msIO_printf("<ogc:Filter_Capabilities>\n");
  msIO_printf("  <ogc:Spatial_Capabilities>\n");
  msIO_printf("    <ogc:Spatial_Operators>\n");
#ifdef USE_GEOS
  msIO_printf("      <ogc:Equals/>\n");
  msIO_printf("      <ogc:Disjoint/>\n");
  msIO_printf("      <ogc:Touches/>\n");
  msIO_printf("      <ogc:Within/>\n");
  msIO_printf("      <ogc:Overlaps/>\n");
  msIO_printf("      <ogc:Crosses/>\n");
  msIO_printf("      <ogc:Intersect/>\n");
  msIO_printf("      <ogc:Contains/>\n");
  msIO_printf("      <ogc:DWithin/>\n");
#endif
  msIO_printf("      <ogc:BBOX/>\n");
  msIO_printf("    </ogc:Spatial_Operators>\n");
  msIO_printf("  </ogc:Spatial_Capabilities>\n");

  msIO_printf("  <ogc:Scalar_Capabilities>\n");
  msIO_printf("    <ogc:Logical_Operators />\n");
  msIO_printf("    <ogc:Comparison_Operators>\n");
  msIO_printf("      <ogc:Simple_Comparisons />\n");
  msIO_printf("      <ogc:Like />\n");
  msIO_printf("      <ogc:Between />\n");
  msIO_printf("    </ogc:Comparison_Operators>\n");
  msIO_printf("  </ogc:Scalar_Capabilities>\n");

  msIO_printf("</ogc:Filter_Capabilities>\n\n");

  /*
  ** Done!
  */
  msIO_printf("</WFS_Capabilities>\n");

  free(script_url_encoded);

  return MS_SUCCESS;
}

/*
** Helper functions for producing XML schema.
*/

static const char *msWFSGetGeometryType(const char *type, OWSGMLVersion outputformat)
{
  if(!type) return "???undefined???";

  if(strcasecmp(type, "point") == 0) {
    switch(outputformat) {
      case OWS_GML2:
      case OWS_GML3:
      case OWS_GML32:
        return "PointPropertyType";
    }
  } else if(strcasecmp(type, "multipoint") == 0) {
    switch(outputformat) {
      case OWS_GML2:
      case OWS_GML3:
      case OWS_GML32:
        return "MultiPointPropertyType";
    }
  } else if(strcasecmp(type, "line") == 0) {
    switch(outputformat) {
      case OWS_GML2:
        return "LineStringPropertyType";
      case OWS_GML3:
      case OWS_GML32:
        return "CurvePropertyType";
    }
  } else if(strcasecmp(type, "multiline") == 0) {
    switch(outputformat) {
      case OWS_GML2:
        return "MultiLineStringPropertyType";
      case OWS_GML3:
      case OWS_GML32:
        return "MultiCurvePropertyType";
    }
  } else if(strcasecmp(type, "polygon") == 0) {
    switch(outputformat) {
      case OWS_GML2:
        return "PolygonPropertyType";
      case OWS_GML3:
      case OWS_GML32:
        return "SurfacePropertyType";
    }
  } else if(strcasecmp(type, "multipolygon") == 0) {
    switch(outputformat) {
      case OWS_GML2:
        return "MultiPolygonPropertyType";
      case OWS_GML3:
      case OWS_GML32:
        return "MultiSurfacePropertyType";
    }
  }

  return "???unknown???";
}

static void msWFSWriteGeometryElement(FILE *stream, gmlGeometryListObj *geometryList, OWSGMLVersion outputformat, const char *tab)
{
  int i;
  gmlGeometryObj *geometry=NULL;

  if(!stream || !tab) return;
  if(geometryList && geometryList->numgeometries == 1 && strcasecmp(geometryList->geometries[0].name, "none") == 0) return;

  if(!geometryList || geometryList->numgeometries == 0) { /* write a default container */
    msIO_fprintf(stream, "%s<element name=\"%s\" type=\"gml:GeometryPropertyType\" minOccurs=\"0\" maxOccurs=\"1\"/>\n", tab, OWS_GML_DEFAULT_GEOMETRY_NAME);
    return;
  } else {
    if(geometryList->numgeometries == 1) {
      geometry = &(geometryList->geometries[0]);
      msIO_fprintf(stream, "%s<element name=\"%s\" type=\"gml:%s\" minOccurs=\"%d\"", tab, geometry->name, msWFSGetGeometryType(geometry->type, outputformat), geometry->occurmin);
      if(geometry->occurmax == OWS_GML_OCCUR_UNBOUNDED)
        msIO_fprintf(stream, " maxOccurs=\"unbounded\"/>\n");
      else
        msIO_fprintf(stream, " maxOccurs=\"%d\"/>\n", geometry->occurmax);
    } else {
      msIO_fprintf(stream, "%s<choice>\n", tab);
      for(i=0; i<geometryList->numgeometries; i++) {
        geometry = &(geometryList->geometries[i]);

        msIO_fprintf(stream, "  %s<element name=\"%s\" type=\"gml:%s\" minOccurs=\"%d\"", tab, geometry->name, msWFSGetGeometryType(geometry->type, outputformat), geometry->occurmin);
        if(geometry->occurmax == OWS_GML_OCCUR_UNBOUNDED)
          msIO_fprintf(stream, " maxOccurs=\"unbounded\"/>\n");
        else
          msIO_fprintf(stream, " maxOccurs=\"%d\"/>\n", geometry->occurmax);
      }
      msIO_fprintf(stream, "%s</choice>\n", tab);
    }
  }

  return;
}

static void msWFSWriteItemElement(FILE *stream, gmlItemObj *item, const char *tab)
{
  char *element_name;
  char *element_type = "string";

  if(!stream || !item || !tab) return;
  if(!item->visible) return; /* not exposing this attribute */
  if(item->template) return; /* can't adequately deal with templated items yet */

  if(item->alias) /* TODO: what about name spaces embedded in the alias? */
    element_name = item->alias;
  else
    element_name = item->name;

  if(item->type)
  {
    /* Map from MapServer types to XSD types */
    if( strcasecmp(item->type,"Integer") == 0 )
      element_type = "integer";
    else if( EQUAL(item->type,"Real") ||
             EQUAL(item->type,"double") /* just in case someone provided the xsd type directly */ )
      element_type = "double";
    else if( EQUAL(item->type,"Character") )
      element_type = "string";
    else if( EQUAL(item->type,"Date") )
      element_type = "date";
    else if( EQUAL(item->type,"Boolean") )
      element_type = "boolean";
  }

  msIO_fprintf(stream, "%s<element name=\"%s\" type=\"%s\"/>\n", tab, element_name, element_type);

  return;
}

static void msWFSWriteConstantElement(FILE *stream, gmlConstantObj *constant, const char *tab)
{
  char *element_type = "string";

  if(!stream || !constant || !tab) return;

  if(constant->type)
    element_type = constant->type;

  msIO_fprintf(stream, "%s<element name=\"%s\" type=\"%s\"/>\n", tab, constant->name, element_type);

  return;
}

static void msWFSWriteGroupElement(FILE *stream, gmlGroupObj *group, const char *tab, const char *namespace)
{
  if(group->type)
    msIO_fprintf(stream, "%s<element name=\"%s\" type=\"%s:%s\"/>\n", tab, group->name, namespace, group->type);
  else
    msIO_fprintf(stream, "%s<element name=\"%s\" type=\"%s:%sType\"/>\n", tab, group->name, namespace, group->name);

  return;
}

static void msWFSWriteGroupElementType(FILE *stream, gmlGroupObj *group, gmlItemListObj *itemList, gmlConstantListObj *constantList, const char *tab)
{
  int i, j;
  char *element_tab;

  gmlItemObj *item=NULL;
  gmlConstantObj *constant=NULL;

  /* setup the element tab */
  element_tab = (char *) malloc(sizeof(char)*strlen(tab)+5);
  MS_CHECK_ALLOC_NO_RET(element_tab, sizeof(char)*strlen(tab)+5);
  sprintf(element_tab, "%s    ", tab);

  if(group->type)
    msIO_fprintf(stream, "%s<complexType name=\"%s\">\n", tab, group->type);
  else
    msIO_fprintf(stream, "%s<complexType name=\"%sType\">\n", tab, group->name);

  msIO_fprintf(stream, "%s  <sequence>\n", tab);

  /* now the items/constants (e.g. elements) in the group */
  for(i=0; i<group->numitems; i++) {
    for(j=0; j<constantList->numconstants; j++) { /* find the right gmlConstantObj */
      constant = &(constantList->constants[j]);
      if(strcasecmp(constant->name, group->items[i]) == 0) {
        msWFSWriteConstantElement(stream, constant, element_tab);
        break;
      }
    }
    if(j != constantList->numconstants) continue; /* found this item */
    for(j=0; j<itemList->numitems; j++) { /* find the right gmlItemObj */
      item = &(itemList->items[j]);
      if(strcasecmp(item->name, group->items[i]) == 0) {
        msWFSWriteItemElement(stream, item, element_tab);
        break;
      }
    }
  }

  msIO_fprintf(stream, "%s  </sequence>\n", tab);
  msIO_fprintf(stream, "%s</complexType>\n", tab);

  return;
}

static const char* msWFSGetGMLSchemaLocation(OWSGMLVersion outputformat)
{
    switch( outputformat )
    {
        case OWS_GML2:
            return MS_OWSCOMMON_GML_212_SCHEMA_LOCATION;
        case OWS_GML3:
            return MS_OWSCOMMON_GML_311_SCHEMA_LOCATION;
        case OWS_GML32:
            return MS_OWSCOMMON_GML_321_SCHEMA_LOCATION;
    }
    return "/unknown.xsd";
}

static const char* msWFSGetGMLNamespaceURI(WFSSchemaVersion outputformat)
{
    switch( outputformat )
    {
        case OWS_DEFAULT_SCHEMA:
            return MS_OWSCOMMON_GML_NAMESPACE_URI;
        case OWS_SFE_SCHEMA:
            return MS_OWSCOMMON_GML_NAMESPACE_URI;
        case OWS_GML32_SFE_SCHEMA:
            return MS_OWSCOMMON_GML_32_NAMESPACE_URI;
    }
    return "http://unknown";
}

static const char* msWFSGetGMLNamespaceURIFromGMLVersion(WFSSchemaVersion outputformat)
{
    switch( outputformat )
    {
        case OWS_GML2:
            return MS_OWSCOMMON_GML_NAMESPACE_URI;
        case OWS_GML3:
            return MS_OWSCOMMON_GML_NAMESPACE_URI;
        case OWS_GML32:
            return MS_OWSCOMMON_GML_32_NAMESPACE_URI;
    }
    return "http://unknown";
}

static OWSGMLVersion msWFSGetGMLVersionFromSchemaVersion(WFSSchemaVersion outputformat)
{
    switch( outputformat )
    {
        case OWS_DEFAULT_SCHEMA:
            return OWS_GML2;
        case OWS_SFE_SCHEMA:
            return OWS_GML3;
        case OWS_GML32_SFE_SCHEMA:
            return OWS_GML32;
    }
    return OWS_GML2;
}

static void msWFS_NS_printf(const char* prefix, const char* uri)
{
    if( prefix == NULL )
        msIO_printf("   xmlns=\"%s\"\n", uri);
    else
        msIO_printf("   xmlns:%s=\"%s\"\n", prefix, uri);
}

static void msWFSPrintAdditionalNamespaces(const gmlNamespaceListObj *namespaceList)
{
    int i;
    /* any additional namespaces */
    for(i=0; i<namespaceList->numnamespaces; i++) {
      if(namespaceList->namespaces[i].uri) {
        char *uri_encoded=NULL;

        uri_encoded = msEncodeHTMLEntities(namespaceList->namespaces[i].uri);
        msWFS_NS_printf(namespaceList->namespaces[i].prefix, uri_encoded);
        msFree(uri_encoded);
      }
    }
}

/*
** msWFSDescribeFeatureType()
*/
static
int msWFSDescribeFeatureType(mapObj *map, wfsParamsObj *paramsObj, owsRequestObj *ows_request,
                             int nWFSVersion)
{
  int i, numlayers=0;
  char **layers = NULL;
  char **tokens;
  int n=0;

  const char *value;
  const char *user_namespace_prefix = WFS_DEFAULT_NAMESPACE_PREFIX;
  const char *user_namespace_uri = WFS_DEFAULT_NAMESPACE_URI;
  char *user_namespace_uri_encoded = NULL;
  const char *collection_name = OWS_WFS_FEATURE_COLLECTION_NAME;
  char *encoded_name = NULL, *encoded;

  WFSSchemaVersion outputformat = OWS_DEFAULT_SCHEMA; /* default output is GML 2.1 compliant schema*/

  gmlNamespaceListObj *namespaceList=NULL; /* for external application schema support */
  char *mimetype = NULL;

  if(paramsObj->pszTypeName && numlayers == 0) {
    /* Parse comma-delimited list of type names (layers) */
    /*  */
    /* __TODO__ Need to handle type grouping, e.g. "(l1,l2),l3,l4" */
    /*  */
    layers = msStringSplit(paramsObj->pszTypeName, ',', &numlayers);
    if (numlayers > 0) {
      /* strip namespace if there is one :ex TYPENAME=cdf:Other */
      tokens = msStringSplit(layers[0], ':', &n);
      if (tokens && n==2 && msGetLayerIndex(map, layers[0]) < 0) {
        msFreeCharArray(tokens, n);
        tokens = NULL;
        for (i=0; i<numlayers; i++) {
          tokens = msStringSplit(layers[i], ':', &n);
          if (tokens && n==2) {
            free(layers[i]);
            layers[i] = msStrdup(tokens[1]);
          }
          if (tokens)
            msFreeCharArray(tokens, n);
          tokens = NULL;
        }
      }
      if (tokens)
        msFreeCharArray(tokens, n);
    }
  }


  if (paramsObj->pszOutputFormat) {
    if(strcasecmp(paramsObj->pszOutputFormat, "XMLSCHEMA") == 0 ||
        strstr(paramsObj->pszOutputFormat, "gml/2")!= NULL) {
      mimetype = msEncodeHTMLEntities("text/xml; subtype=gml/2.1.2");
      outputformat = OWS_DEFAULT_SCHEMA;
    } else if(strcasecmp(paramsObj->pszOutputFormat, "SFE_XMLSCHEMA") == 0 ||
              strstr(paramsObj->pszOutputFormat, "gml/3.1")!= NULL) {
      mimetype = msEncodeHTMLEntities("text/xml; subtype=gml/3.1.1");
      outputformat = OWS_SFE_SCHEMA;
    } else if(strstr(paramsObj->pszOutputFormat, "gml/3.2")!= NULL ||
              strstr(paramsObj->pszOutputFormat, "application/gml+xml; version=3.2")!= NULL) {
      mimetype = msEncodeHTMLEntities("application/gml+xml; version=3.2");
      outputformat = OWS_GML32_SFE_SCHEMA;
    } else {
      msSetError(MS_WFSERR, "Unsupported DescribeFeatureType outputFormat (%s).", "msWFSDescribeFeatureType()", paramsObj->pszOutputFormat);
      return msWFSException(map, "outputformat", "InvalidParameterValue", paramsObj->pszVersion);
    }
  }
  /* If no outputFormat explicitely asked, use a sensible default for the WFS version */
  else {
    switch( nWFSVersion )
    {
        case OWS_1_0_0:
        default:
            mimetype = msEncodeHTMLEntities("text/xml"); /* ERO: why not "text/xml; subtype=gml/2.1.2" ? */
            break;

        case OWS_1_1_0:
            mimetype = msEncodeHTMLEntities("text/xml; subtype=gml/3.1.1");
            outputformat = OWS_SFE_SCHEMA;
            break;

        case OWS_2_0_0:
            mimetype = msEncodeHTMLEntities("application/gml+xml; version=3.2");
            outputformat = OWS_GML32_SFE_SCHEMA;
            break;
     }
  }

  /* Validate layers */
  if (numlayers > 0) {
    for (i=0; i<numlayers; i++) {
      int index = msGetLayerIndex(map, layers[i]);
      if ( (index < 0) || (!msIntegerInArray(GET_LAYER(map, index)->index, ows_request->enabled_layers, ows_request->numlayers)) ) {
        msSetError(MS_WFSERR, "Invalid typename (%s). A layer might be disabled for \
this request. Check wfs/ows_enable_request settings.", "msWFSDescribeFeatureType()", layers[i]);/* paramsObj->pszTypeName); */
        return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
      }
    }
  }

  /*
  ** retrieve any necessary external namespace/schema configuration information
  */
  namespaceList = msGMLGetNamespaces(&(map->web), "G");
  if (namespaceList == NULL) {
    msSetError(MS_MISCERR, "Unable to populate namespace list", "msWFSDescribeFeatureType()");
    return MS_FAILURE;
  }

  /*
  ** DescribeFeatureType response
  */

  msIO_setHeader("Content-Type","%s; charset=UTF-8",mimetype);
  msIO_sendHeaders();

  if (mimetype)
    msFree(mimetype);

  msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" ?>\n");

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_uri");
  if(value) user_namespace_uri = value;
  user_namespace_uri_encoded = msEncodeHTMLEntities(user_namespace_uri);

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_prefix");
  if(value) user_namespace_prefix = value;
  if(user_namespace_prefix != NULL && msIsXMLTagValid(user_namespace_prefix) == MS_FALSE)
    msIO_printf("<!-- WARNING: The value '%s' is not valid XML namespace. -->\n", user_namespace_prefix);

  msIO_printf("<schema\n"
              "   targetNamespace=\"%s\" \n"
              "   xmlns:%s=\"%s\" \n",
              user_namespace_uri_encoded, user_namespace_prefix,  user_namespace_uri_encoded);
  if( nWFSVersion < OWS_2_0_0 )
    msWFS_NS_printf(MS_OWSCOMMON_OGC_NAMESPACE_PREFIX, MS_OWSCOMMON_OGC_NAMESPACE_URI);
  msWFS_NS_printf("xsd", MS_OWSCOMMON_W3C_XS_NAMESPACE_URI);
  msWFS_NS_printf(NULL, MS_OWSCOMMON_W3C_XS_NAMESPACE_URI);
  msWFS_NS_printf(MS_OWSCOMMON_GML_NAMESPACE_PREFIX,
                  msWFSGetGMLNamespaceURI(outputformat) );

  msWFSPrintAdditionalNamespaces(namespaceList);

  msIO_printf("   elementFormDefault=\"qualified\" version=\"0.1\" >\n");

  encoded = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );

  msIO_printf("\n  <import namespace=\"%s\"\n"
              "          schemaLocation=\"%s%s\" />\n",
              msWFSGetGMLNamespaceURI(outputformat),
              encoded,
              msWFSGetGMLSchemaLocation(msWFSGetGMLVersionFromSchemaVersion(outputformat)));

  msFree(encoded);

  /* any additional namespace includes */
  for(i=0; i<namespaceList->numnamespaces; i++) {
    if(namespaceList->namespaces[i].uri && namespaceList->namespaces[i].schemalocation) {
      char *schema_location_encoded=NULL, *uri_encoded=NULL;

      uri_encoded = msEncodeHTMLEntities(namespaceList->namespaces[i].uri);
      schema_location_encoded = msEncodeHTMLEntities(namespaceList->namespaces[i].schemalocation);
      msIO_printf("\n  <import namespace=\"%s\"\n schemaLocation=\"%s\" />\n", uri_encoded, schema_location_encoded);
      msFree(uri_encoded);
      msFree(schema_location_encoded);
    }
  }

  /* output definition for the default feature container, can't use wfs:FeatureCollection with GML3:
     kept here so that the behaviour with wfs1.0 and gml3 output is preserved.
     We can use the wfs:FeatureCollection for wfs1.1*/
  if(outputformat == OWS_SFE_SCHEMA && nWFSVersion == OWS_1_0_0 ) {
    value = msOWSLookupMetadata(&(map->web.metadata), "FO", "feature_collection");
    if(value) collection_name = value;

    msIO_printf("  <element name=\"%s\" type=\"%s:%sType\" substitutionGroup=\"gml:_FeatureCollection\"/>\n", collection_name, user_namespace_prefix, collection_name);
    msIO_printf("  <complexType name=\"%sType\">\n", collection_name);
    msIO_printf("    <complexContent>\n");
    msIO_printf("      <extension base=\"gml:AbstractFeatureCollectionType\">\n");
    msIO_printf("        <attribute name=\"version\" type=\"string\" use=\"required\" fixed=\"1.0.0\"/>\n");
    msIO_printf("      </extension>\n");
    msIO_printf("    </complexContent>\n");
    msIO_printf("  </complexType>\n");
  }

  /*
  ** loop through layers
  */
  for(i=0; i<map->numlayers; i++)  {
    layerObj *lp;
    int j, bFound = 0;

    lp = GET_LAYER(map, i);

    for (j=0; j<numlayers && !bFound; j++) {
      if ( lp->name && strcasecmp(lp->name, layers[j]) == 0)
        bFound = 1;
    }

    if ((numlayers == 0 || bFound) && msWFSIsLayerSupported(lp) &&
        (msIntegerInArray(lp->index, ows_request->enabled_layers, ows_request->numlayers)) ) {

      /*
      ** OK, describe this layer IF you can open it and retrieve items
      */
      if (msLayerOpen(lp) == MS_SUCCESS) {
        if (msLayerGetItems(lp) == MS_SUCCESS) {
          int k;
          gmlGroupListObj *groupList=NULL;
          gmlItemListObj *itemList=NULL;
          gmlConstantListObj *constantList=NULL;
          gmlGeometryListObj *geometryList=NULL;
          gmlItemObj *item=NULL;
          gmlConstantObj *constant=NULL;

          const char *layer_namespace_prefix;
          const char *substitution_group;
          char *encoded_type=NULL;

          itemList = msGMLGetItems(lp, "G"); /* GML-related metadata */
          constantList = msGMLGetConstants(lp, "G");
          groupList = msGMLGetGroups(lp, "G");
          geometryList = msGMLGetGeometries(lp, "GFO");
          if (itemList == NULL || constantList == NULL || groupList == NULL || geometryList == NULL) {
            msSetError(MS_MISCERR, "Unable to populate item and group metadata structures", "msWFSDescribeFeatureType()");
            return MS_FAILURE;
          }

          value = msOWSLookupMetadata(&(lp->metadata), "OFG", "namespace_prefix");
          if(value)
            layer_namespace_prefix = value;
          else
            layer_namespace_prefix = user_namespace_prefix;

          /* value = msOWSLookupMetadata(&(lp->metadata), "OFG", "layername"); */
          encoded_name = msEncodeHTMLEntities( lp->name );
          value = msOWSLookupMetadata(&(lp->metadata), "OFG", "layer_type");
          if(value) {
            encoded_type = msEncodeHTMLEntities(value);
          } else {
            size_t sz = strlen(encoded_name) + strlen("Type") + 1;
            encoded_type = (char*) msSmallMalloc(sz);
            strlcpy(encoded_type, encoded_name, sz);
            strlcat(encoded_type, "Type", sz);
          }

          switch(outputformat)
          {
            case OWS_DEFAULT_SCHEMA: /* default GML 2.1.x schema */
            case OWS_SFE_SCHEMA: /* reference GML 3.1.1 schema */
            default:
                substitution_group = "gml:_Feature";
                break;

            case OWS_GML32_SFE_SCHEMA:  /* reference GML 3.2.1 schema */
                substitution_group = "gml:AbstractFeature";
                break;
          }

          msIO_printf("\n"
                      "  <element name=\"%s\" \n"
                      "           type=\"%s:%s\" \n"
                      "           substitutionGroup=\"%s\" />\n\n",
                      encoded_name, layer_namespace_prefix, encoded_type,
                      substitution_group);
          msFree(encoded_type);

          if(strcmp(layer_namespace_prefix, user_namespace_prefix) != 0)
            continue; /* the rest is defined in an external schema */

          msIO_printf("  <complexType name=\"%sType\">\n", encoded_name);
          msIO_printf("    <complexContent>\n");
          msIO_printf("      <extension base=\"gml:AbstractFeatureType\">\n");
          msIO_printf("        <sequence>\n");

          /* write the geometry schema element(s) */
          msWFSWriteGeometryElement(stdout, geometryList, outputformat, "          ");

          /* write the constant-based schema elements */
          for(k=0; k<constantList->numconstants; k++) {
            constant = &(constantList->constants[k]);
            if(msItemInGroups(constant->name, groupList) == MS_FALSE)
              msWFSWriteConstantElement(stdout, constant, "          ");
          }

          /* write the item-based schema elements */
          for(k=0; k<itemList->numitems; k++) {
            item = &(itemList->items[k]);
            if(msItemInGroups(item->name, groupList) == MS_FALSE)
              msWFSWriteItemElement(stdout, item, "          ");
          }

          for(k=0; k<groupList->numgroups; k++)
            msWFSWriteGroupElement(stdout, &(groupList->groups[k]), "          ", user_namespace_prefix);

          msIO_printf("        </sequence>\n");
          msIO_printf("      </extension>\n");
          msIO_printf("    </complexContent>\n");
          msIO_printf("  </complexType>\n");

          /* any group types */
          for(k=0; k<groupList->numgroups; k++)
            msWFSWriteGroupElementType(stdout, &(groupList->groups[k]), itemList, constantList, "  ");

          msGMLFreeItems(itemList);
          msGMLFreeConstants(constantList);
          msGMLFreeGroups(groupList);
          msGMLFreeGeometries(geometryList);
        }

        msLayerClose(lp);
      } else {
        msIO_printf("\n\n<!-- ERROR: Failed opening layer %s -->\n\n", encoded_name);
      }

    }
  }

  /*
  ** Done!
  */
  msIO_printf("\n</schema>\n");

  msFree(encoded_name);
  msFree(user_namespace_uri_encoded);

  if(layers)
    msFreeCharArray(layers, numlayers);

  msGMLFreeNamespaces(namespaceList);

  return MS_SUCCESS;
}

/*
** msWFSGetFeature_GMLPreamble()
**
** Generate the GML preamble up to the first feature for the builtin
** WFS GML support.
*/

typedef struct {
  const char *user_namespace_prefix;
  const char *user_namespace_uri;
  char       *user_namespace_uri_encoded;
  const char *collection_name;
  const char *typename;
  char       *script_url, *script_url_encoded;
  const char *output_mime_type;
  const char *output_schema_format;
} WFSGMLInfo;


static void msWFSGetFeature_PrintBasePrevNextURI(WFSGMLInfo *gmlinfo,
                                                 wfsParamsObj *paramsObj,
                                                 const char* encoded_version,
                                                 const char* encoded_typename,
                                                 const char* encoded_mime_type)
{
    msIO_printf("%s", gmlinfo->script_url_encoded);
    msIO_printf("SERVICE=WFS&amp;VERSION=");
    msIO_printf("%s", encoded_version);
    msIO_printf("&amp;REQUEST=GetFeature");
    msIO_printf("&amp;TYPENAMES=");
    msIO_printf("%s", encoded_typename);
    msIO_printf("&amp;OUTPUTFORMAT=");
    msIO_printf("%s", encoded_mime_type);
    if( paramsObj->pszBbox != NULL )
        msIO_printf("&amp;BBOX=%s", paramsObj->pszBbox);
    if( paramsObj->pszSrs != NULL )
        msIO_printf("&amp;SRSNAME=%s", paramsObj->pszSrs);
    if( paramsObj->pszFilter != NULL )
        msIO_printf("&amp;FILTER=%s", paramsObj->pszFilter);
    if( paramsObj->pszPropertyName != NULL)
        msIO_printf("&amp;PROPERTYNAME=%s", paramsObj->pszPropertyName);
    if( paramsObj->pszSortBy != NULL )
        msIO_printf("&amp;SORTBY=%s", paramsObj->pszSortBy);
    if( paramsObj->nMaxFeatures >= 0 )
        msIO_printf("&amp;COUNT=%d", paramsObj->nMaxFeatures);
}

static void msWFSGetFeature_GetTimeStamp(char* timestring, size_t timestringlen)
{
    struct tm *now;
    time_t tim=time(NULL);

    now=localtime(&tim);

    snprintf(timestring, timestringlen, "%d-%02d-%02dT%02d:%02d:%02d",
                now->tm_year+1900, now->tm_mon+1, now->tm_mday,
                now->tm_hour, now->tm_min, now->tm_sec);
}

static int msWFSGetFeature_GMLPreamble( mapObj *map,
                                        cgiRequestObj *req,
                                        WFSGMLInfo *gmlinfo,
                                        wfsParamsObj *paramsObj,
                                        OWSGMLVersion outputformat,
                                        int iResultTypeHits,
                                        int iNumberOfFeatures,
                                        int nMatchingFeatures,
                                        int maxfeatures,
                                        int nWFSVersion )

{
  const char *value;
  char       *encoded_version, *encoded_typename, *encoded_schema;
  gmlNamespaceListObj *namespaceList=NULL; /* for external application schema support */
  char timestring[100];
  timestring[0] = '\0';

  namespaceList = msGMLGetNamespaces(&(map->web), "G");
  if (namespaceList == NULL) {
    msSetError(MS_MISCERR, "Unable to populate namespace list", "msWFSGetFeature_GMLPreamble()");
    return MS_FAILURE;
  }

  /*
  ** Establish script_url.
  */

  if ((gmlinfo->script_url=msOWSGetOnlineResource(map,"FO","onlineresource",req)) ==NULL ||
      (gmlinfo->script_url_encoded = msEncodeHTMLEntities(gmlinfo->script_url)) == NULL) {
    msSetError(MS_WFSERR, "Server URL not found", "msWFSGetFeature()");
    return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
  }

  /*
  ** Write encoding.
  */
  msIO_printf("<?xml version='1.0' encoding=\"UTF-8\" ?>\n");

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_uri");
  if(value) gmlinfo->user_namespace_uri = value;
  gmlinfo->user_namespace_uri_encoded =
    msEncodeHTMLEntities(gmlinfo->user_namespace_uri);

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_prefix");
  if(value) gmlinfo->user_namespace_prefix = value;

  if(gmlinfo->user_namespace_prefix != NULL && msIsXMLTagValid(gmlinfo->user_namespace_prefix) == MS_FALSE)
    msIO_printf("<!-- WARNING: The value '%s' is not valid XML namespace. -->\n", gmlinfo->user_namespace_prefix);

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "feature_collection");
  if(value) gmlinfo->collection_name = value;

  encoded_version = msEncodeHTMLEntities( paramsObj->pszVersion );
  encoded_typename = msEncodeHTMLEntities( gmlinfo->typename );
  encoded_schema = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );

  if( nWFSVersion >= OWS_2_0_0) {
    int nNextStartIndex;
    char* tmp;
    char* encoded_mime_type;
    int bAbleToPrintPreviousOrNext = MS_TRUE;

    tmp = msEncodeUrl( gmlinfo->output_mime_type );
    encoded_mime_type = msEncodeHTMLEntities( tmp );
    msFree(tmp);

    msWFSGetFeature_GetTimeStamp(timestring, sizeof(timestring));

    msIO_printf("<wfs:FeatureCollection\n");

    msWFS_NS_printf(gmlinfo->user_namespace_prefix,
                    gmlinfo->user_namespace_uri_encoded);
    msWFS_NS_printf(MS_OWSCOMMON_GML_NAMESPACE_PREFIX,
                    msWFSGetGMLNamespaceURIFromGMLVersion(outputformat));
    msWFS_NS_printf(MS_OWSCOMMON_WFS_NAMESPACE_PREFIX,
                    MS_OWSCOMMON_WFS_20_NAMESPACE_URI);
    msWFS_NS_printf(MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX,
                    MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI);
    msWFSPrintAdditionalNamespaces(namespaceList);

    msIO_printf("   xsi:schemaLocation=\"%s %sSERVICE=WFS&amp;VERSION=%s&amp;REQUEST=DescribeFeatureType&amp;TYPENAME=%s&amp;OUTPUTFORMAT=%s "
                                        "%s %s%s "
                                        "%s %s%s\"\n",
                gmlinfo->user_namespace_uri_encoded,
                gmlinfo->script_url_encoded, encoded_version,
                encoded_typename,
                gmlinfo->output_schema_format,

                MS_OWSCOMMON_WFS_20_NAMESPACE_URI,
                encoded_schema,
                MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION,

                msWFSGetGMLNamespaceURIFromGMLVersion(outputformat),
                encoded_schema,
                msWFSGetGMLSchemaLocation(outputformat));

    msIO_printf("   timeStamp=\"%s\" numberMatched=\"", timestring);
    if( nMatchingFeatures < 0 )
    {
        /* If we don't know the exact number, at least return something */
        /* equivalent to what we would return with WFS 1.1, otherwise */
        /* resultType=hits would not return anything usefull to the client. */
        if( iResultTypeHits == MS_TRUE )
            msIO_printf("%d", iNumberOfFeatures);
        else
            msIO_printf("unknown");
    }
    else
        msIO_printf("%d", nMatchingFeatures);
    msIO_printf("\" numberReturned=\"%d\"",
                (iResultTypeHits == 1) ? 0 : iNumberOfFeatures);

    /* TODO: in case of a multi-layer GetFeature POST, it is difficult to build a */
    /* valid GET KVM GetFeature when some options are specified. So for now just */
    /* avoid those problematic cases */
    if( req->postrequest != NULL && strchr(encoded_typename, ',') == NULL &&
        (paramsObj->pszFilter != NULL || paramsObj->pszPropertyName != NULL ||
         paramsObj->pszSortBy != NULL) )
    {
        bAbleToPrintPreviousOrNext = MS_FALSE;
    }

    if( bAbleToPrintPreviousOrNext )
    {
        if( maxfeatures > 0 && 
            iResultTypeHits != 1 && paramsObj->nStartIndex > 0 &&
            (nMatchingFeatures < 0 || paramsObj->nStartIndex < nMatchingFeatures) )
        {
            int nPrevStartIndex;

            msIO_printf("\n");
            msIO_printf("   previous=\"");
            msWFSGetFeature_PrintBasePrevNextURI(gmlinfo, paramsObj,
                                                encoded_version,
                                                encoded_typename,
                                                encoded_mime_type);

            nPrevStartIndex = paramsObj->nStartIndex - maxfeatures;
            if( nPrevStartIndex > 0 )
                msIO_printf("&amp;STARTINDEX=%d", nPrevStartIndex);
            msIO_printf("\"");
        }

        if( iResultTypeHits != 1 && paramsObj->nStartIndex >= 0 )
            nNextStartIndex = paramsObj->nStartIndex;
        else
            nNextStartIndex = 0;
        if( maxfeatures > 0 && nMatchingFeatures > 0 &&
            (iResultTypeHits == 1 || iNumberOfFeatures + nNextStartIndex < nMatchingFeatures) )
        {
            msIO_printf("\n");
            msIO_printf("   next=\"");
            msWFSGetFeature_PrintBasePrevNextURI(gmlinfo, paramsObj,
                                                encoded_version,
                                                encoded_typename,
                                                encoded_mime_type);

            if( iResultTypeHits != 1 )
                nNextStartIndex += iNumberOfFeatures;

            if( nNextStartIndex > 0 )
                msIO_printf("&amp;STARTINDEX=%d", nNextStartIndex);
            msIO_printf("\"");
        }
    }

    msIO_printf(">\n");

    msFree(encoded_mime_type);
  }
  /*
  ** GML 2.x
  */
  else if(outputformat == OWS_GML2) { /* use a wfs:FeatureCollection */
    msIO_printf("<wfs:FeatureCollection\n");
    msWFS_NS_printf(gmlinfo->user_namespace_prefix,
                    gmlinfo->user_namespace_uri_encoded);
    msWFS_NS_printf(MS_OWSCOMMON_WFS_NAMESPACE_PREFIX,
                    MS_OWSCOMMON_WFS_NAMESPACE_URI);
    msWFS_NS_printf(MS_OWSCOMMON_GML_NAMESPACE_PREFIX,
                    MS_OWSCOMMON_GML_NAMESPACE_URI);
    msWFS_NS_printf(MS_OWSCOMMON_OGC_NAMESPACE_PREFIX,
                    MS_OWSCOMMON_OGC_NAMESPACE_URI);
    msWFS_NS_printf(MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX,
                    MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI);
    msWFSPrintAdditionalNamespaces(namespaceList);

    /* FIXME ? : the schemaLocation will be only valid for WFS 1.0.0 */
    msIO_printf("   xsi:schemaLocation=\"http://www.opengis.net/wfs %s/wfs/%s/WFS-basic.xsd \n"
                "                       %s %sSERVICE=WFS&amp;VERSION=%s&amp;REQUEST=DescribeFeatureType&amp;TYPENAME=%s&amp;OUTPUTFORMAT=%s\">\n",
                encoded_schema, encoded_version,
                gmlinfo->user_namespace_uri_encoded,
                gmlinfo->script_url_encoded, encoded_version,
                encoded_typename, gmlinfo->output_schema_format);
  }

  /*
  ** GML 3
  */
  else if( outputformat == OWS_GML3 ) {
    if( nWFSVersion == OWS_1_1_0 ) {
      msIO_printf("<wfs:FeatureCollection\n");
      msWFS_NS_printf(gmlinfo->user_namespace_prefix,
                      gmlinfo->user_namespace_uri_encoded);
      msWFS_NS_printf(MS_OWSCOMMON_GML_NAMESPACE_PREFIX,
                      MS_OWSCOMMON_GML_NAMESPACE_URI);
      msWFS_NS_printf(MS_OWSCOMMON_WFS_NAMESPACE_PREFIX,
                      MS_OWSCOMMON_WFS_NAMESPACE_URI);
      msWFS_NS_printf(MS_OWSCOMMON_OGC_NAMESPACE_PREFIX,
                      MS_OWSCOMMON_OGC_NAMESPACE_URI);
      msWFS_NS_printf(MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX,
                      MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI);
    } else {
      msIO_printf("<%s:%s\n"
                  "   version=\"1.0.0\"\n",
                  gmlinfo->user_namespace_prefix,
                  gmlinfo->collection_name);
      msWFS_NS_printf(gmlinfo->user_namespace_prefix,
                      gmlinfo->user_namespace_uri_encoded);
      msWFS_NS_printf(MS_OWSCOMMON_GML_NAMESPACE_PREFIX,
                      MS_OWSCOMMON_GML_NAMESPACE_URI);
      msWFS_NS_printf(MS_OWSCOMMON_OGC_NAMESPACE_PREFIX,
                      MS_OWSCOMMON_OGC_NAMESPACE_URI);
      msWFS_NS_printf(MS_OWSCOMMON_W3C_XSI_NAMESPACE_PREFIX,
                      MS_OWSCOMMON_W3C_XSI_NAMESPACE_URI);
    }

    msWFSPrintAdditionalNamespaces(namespaceList);

    msIO_printf("   xsi:schemaLocation=\"%s %sSERVICE=WFS&amp;VERSION=%s&amp;REQUEST=DescribeFeatureType&amp;TYPENAME=%s&amp;OUTPUTFORMAT=%s",
                gmlinfo->user_namespace_uri_encoded,
                gmlinfo->script_url_encoded, encoded_version,
                encoded_typename, gmlinfo->output_schema_format);

    if( nWFSVersion == OWS_1_1_0 ) {

      msIO_printf("  %s %s%s",
                  MS_OWSCOMMON_WFS_NAMESPACE_URI,
                  encoded_schema,
                  MS_OWSCOMMON_WFS_11_SCHEMA_LOCATION);

      if (iResultTypeHits == 1) {
        msWFSGetFeature_GetTimeStamp(timestring, sizeof(timestring));

        msIO_printf("\" timeStamp=\"%s\" numberOfFeatures=\"%d",
                    timestring, iNumberOfFeatures);
      }
    }
    msIO_printf("\">\n");
  }

  msFree(encoded_version);
  msFree(encoded_schema);
  msFree(encoded_typename);

  msGMLFreeNamespaces(namespaceList);

  return MS_SUCCESS;
}

/*
** msWFSGetFeature_GMLPostfix()
**
** Generate the GML file tail closing the collection and cleanup a bit.
*/

static int msWFSGetFeature_GMLPostfix( mapObj *map,
                                       cgiRequestObj *req,
                                       WFSGMLInfo *gmlinfo,
                                       wfsParamsObj *paramsObj,
                                       OWSGMLVersion outputformat,
                                       int maxfeatures,
                                       int iResultTypeHits,
                                       int iNumberOfFeatures,
                                       int nWFSVersion )

{
  if (((iNumberOfFeatures==0) || (maxfeatures == 0)) && iResultTypeHits == 0) {

    if( nWFSVersion < OWS_2_0_0 )
    {
      msIO_printf("   <gml:boundedBy>\n");
      if(outputformat == OWS_GML3 || outputformat == OWS_GML32 )
        msIO_printf("      <gml:Null>missing</gml:Null>\n");
      else
        msIO_printf("      <gml:null>missing</gml:null>\n");
      msIO_printf("   </gml:boundedBy>\n");
    }
  }

  if(outputformat == OWS_GML2)
    msIO_printf("</wfs:FeatureCollection>\n\n");
  else {
    if( nWFSVersion >= OWS_1_1_0 )
      msIO_printf("</wfs:FeatureCollection>\n\n");
    else
      msIO_printf("</%s:%s>\n\n", gmlinfo->user_namespace_prefix, gmlinfo->collection_name);
  }

  free(gmlinfo->script_url);
  free(gmlinfo->script_url_encoded);
  msFree(gmlinfo->user_namespace_uri_encoded);

  return MS_SUCCESS;
}

/*
** msWFSBuildParamList()
*/
static void msWFSBuildParamList(char** ppszStrList, const char* pszValue,
                                const char* pszSep)
{
    if (*ppszStrList == NULL)
        *ppszStrList = msStrdup(pszValue);
    else
    {
        char* pszTmp = msStrdup(*ppszStrList);
        *ppszStrList =
            (char *)msSmallRealloc(*ppszStrList,
                                    sizeof(char)*
                                    (strlen(pszTmp)+
                                     strlen(pszSep)+
                                     strlen(pszValue)+1));

        sprintf(*ppszStrList,"%s%s%s",pszTmp,pszSep,pszValue);
        free(pszTmp);
    }
}

/*
** msWFSRetrieveFeatures()
*/
static int msWFSRetrieveFeatures(mapObj* map,
                              const wfsParamsObj *paramsObj,
                              const WFSGMLInfo* gmlinfo,
                              int bFilterSet,
                              const char* pszFilter,
                              int bBBOXSet,
                              const char* pszBBOXSRS,
                              rectObj bbox,
                              int bFeatureIdSet,
                              char **layers,
                              int numlayers,
                              int nWFSVersion)
{
  int i, j;

#ifdef USE_OGR
  if (bFilterSet && pszFilter && strlen(pszFilter) > 0) {
    char **tokens = NULL;
    int nFilters;
    FilterEncodingNode *psNode = NULL;
    int iLayerIndex =1;
    char **paszFilter = NULL;
    errorObj   *ms_error;

    /* -------------------------------------------------------------------- */
    /*      Validate the parameters. When a FILTER parameter is given,      */
    /*      It needs the TYPENAME parameter for the layers. Also Filter     */
    /*      is Mutually exclusive with FEATUREID and BBOX (see wfs specs    */
    /*      1.0 section 13.7.3 on GetFeature)                               */
    /*                                                                      */
    /* -------------------------------------------------------------------- */

    /* WFS 2.0 */
    if (nWFSVersion >= OWS_2_0_0 &&
        paramsObj->pszFilterLanguage != NULL &&
        strcasecmp(paramsObj->pszFilterLanguage, "urn:ogc:def:query Language:OGC-FES:Filter") != 0) {
      msSetError(MS_WFSERR,
                 "Unhandled value for FILTER_LANGUAGE parameter. Only \"urn:ogc:def:query Language:OGC-FES:Filter\" accepted.",
                 "msWFSGetFeature()");
      return msWFSException(map, "filter_language", "InvalidParameterValue", paramsObj->pszVersion);
    }

    if (gmlinfo->typename == NULL || strlen(gmlinfo->typename) <= 0 || layers == NULL || numlayers <= 0) {
      msSetError(MS_WFSERR,
                 "Required %s parameter missing for GetFeature with a FILTER parameter.",
                 "msWFSGetFeature()",
                 (nWFSVersion >= OWS_2_0_0 ) ? "TYPENAMES": "TYPENAME" );
      return msWFSException(map, (nWFSVersion >= OWS_2_0_0 ) ? "typenames": "typename",
                            "MissingParameterValue", paramsObj->pszVersion);
    }

    if (bBBOXSet) {
      msSetError(MS_WFSERR,
                 "BBOX parameter and FILTER parameter are mutually exclusive in GetFeature.",
                 "msWFSGetFeature()");
      return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
    }

    if (bFeatureIdSet) {
      msSetError(MS_WFSERR,
                 "%s parameter and FILTER parameter are mutually exclusive in GetFeature.",
                 "msWFSGetFeature()",
                 (nWFSVersion < OWS_2_0_0 ) ? "FEATUREID" : "RESOURCEID" );
      return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
    }

    /* -------------------------------------------------------------------- */
    /*      Parse the Filter parameter. If there are several Filter         */
    /*      parameters, each Filter is inside a parantheses. Eg :           */
    /*      FILTER=(<Filter><Within><PropertyName>                          */
    /*      INWATERA_1M/WKB_GEOM|INWATERA_1M/WKB_GEOM                       */
    /*      <PropertyName><gml:Box><gml:coordinates>10,10 20,20</gml:coordinates>*/
    /*      </gml:Box></Within></Filter>)(<Filter><Within><PropertyName>    */
    /*      INWATERA_1M/WKB_GEOM<PropertyName><gml:Box><gml:coordinates>10,10*/
    /*      20,20</gml:coordinates></gml:Box></Within></Filter>)            */
    /* -------------------------------------------------------------------- */
    nFilters = 0;
    if (strlen(pszFilter) > 0 && pszFilter[0] == '(') {
      tokens = msStringSplit(pszFilter+1, '(', &nFilters);

      if (tokens &&  nFilters > 0 && numlayers == nFilters) {
        paszFilter = (char **)msSmallMalloc(sizeof(char *)*nFilters);
        for (i=0; i<nFilters; i++)
          paszFilter[i] = msStrdup(tokens[i]);
      }

      if (tokens &&  nFilters > 0 ) {
        msFreeCharArray(tokens, nFilters);
      }
    } else if (numlayers == 1) {
      nFilters=1;
      paszFilter = (char **)msSmallMalloc(sizeof(char *)*nFilters);
      paszFilter[0] = msStrdup(pszFilter);
    }

    if (numlayers != nFilters) {
      msSetError(MS_WFSERR, "Wrong number of filter elements, one filter must be specified for each feature type listed in the %s parameter.",
                 "msWFSGetFeature()",
                 (nWFSVersion >= OWS_2_0_0 ) ? "TYPENAMES": "TYPENAME" );
      return msWFSException(map, "filter", "InvalidParameterValue", paramsObj->pszVersion);
    }
    /* -------------------------------------------------------------------- */
    /*      run through the filters and build the class expressions.        */
    /*      TODO: items may have namespace prefixes, or reference aliases,  */
    /*      or groups. Need to be a bit more sophisticated here.            */
    /* -------------------------------------------------------------------- */
    for (i=0; i<nFilters; i++) {
      iLayerIndex = msGetLayerIndex(map, layers[i]);
      if (iLayerIndex < 0) {
        msSetError(MS_WFSERR, "Invalid Typename in GetFeature : %s. A layer might be disabled for \
this request. Check wfs/ows_enable_request settings.", "msWFSGetFeature()", layers[i]);
        return msWFSException(map,
                              (nWFSVersion >= OWS_2_0_0 ) ? "typenames": "typename",
                              "InvalidParameterValue", paramsObj->pszVersion);
      }
      psNode = FLTParseFilterEncoding(paszFilter[i]);

      if (!psNode) {
        msSetError(MS_WFSERR,
                   "Invalid or Unsupported FILTER in GetFeature : %s",
                   "msWFSGetFeature()", pszFilter);
        return msWFSException(map, "filter", "InvalidParameterValue", paramsObj->pszVersion);
      }

      /*preparse the filter for gml aliases*/
      FLTPreParseFilterForAlias(psNode, map, iLayerIndex, "G");

      if (msWFSGetFeatureApplySRS(map, paramsObj->pszSrs, nWFSVersion) == MS_FAILURE)
        return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);

      /* run filter.  If no results are found, do not throw exception */
      /* this is a null result */
      if( FLTApplyFilterToLayer(psNode, map, iLayerIndex) != MS_SUCCESS ) {
        ms_error = msGetErrorObj();

        if(ms_error->code != MS_NOTFOUND) {
          msSetError(MS_WFSERR, "FLTApplyFilterToLayer() failed", "msWFSGetFeature()");
          return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
        }
      }

      FLTFreeFilterEncodingNode( psNode );
      psNode = NULL;
    }

    msFreeCharArray(paszFilter, nFilters);
  }/* end if filter set */


  if (bFeatureIdSet) {
    char **tokens = NULL, **tokens1=NULL ;
    int nTokens = 0, j=0, nTokens1=0, k=0;
    FilterEncodingNode *psNode = NULL;
    char **aFIDLayers = NULL;
    char **aFIDValues = NULL;
    int iFIDLayers = 0;

    /* Keep only selected layers, set to OFF by default. */
    for(j=0; j<map->numlayers; j++) {
      layerObj *lp;
      lp = GET_LAYER(map, j);
      lp->status = MS_OFF;
    }
    /*featureid can be a list INWATERA_1M.1234, INWATERA_1M.1235
    We will keep all the feature id from the same layer together
    so that an OR would be applied if several of them are present
    */
    tokens = msStringSplit(paramsObj->pszFeatureId, ',', &nTokens);
    iFIDLayers = 0;
    if (tokens && nTokens >=1) {
      aFIDLayers = (char **)msSmallMalloc(sizeof(char *)*nTokens);
      aFIDValues = (char **)msSmallMalloc(sizeof(char *)*nTokens);
      for (j=0; j<nTokens; j++) {
        aFIDLayers[j] = NULL;
        aFIDValues[j] = NULL;
      }
      for (j=0; j<nTokens; j++) {
        tokens1 = msStringSplit(tokens[j], '.', &nTokens1);
        if (tokens1 && nTokens1 == 2) {
          for (k=0; k<iFIDLayers; k++) {
            if (strcasecmp(aFIDLayers[k], tokens1[0]) == 0)
              break;
          }
          if (k == iFIDLayers) {
            aFIDLayers[iFIDLayers] = msStrdup(tokens1[0]);
            iFIDLayers++;
          }
          if (aFIDValues[k] != NULL)
            aFIDValues[k] = msStringConcatenate(aFIDValues[k], ",");
          aFIDValues[k] = msStringConcatenate( aFIDValues[k], tokens1[1]);
        } else {
          msSetError(MS_WFSERR,
                     "Invalid FeatureId in GetFeature. Expecting layername.value : %s",
                     "msWFSGetFeature()", tokens[j]);
          if (tokens)
            msFreeCharArray(tokens, nTokens);
          if (tokens1)
            msFreeCharArray(tokens1, nTokens1);
          return msWFSException(map, "featureid", "InvalidParameterValue", paramsObj->pszVersion);
        }
        if (tokens1)
          msFreeCharArray(tokens1, nTokens1);
      }
    }
    if (tokens)
      msFreeCharArray(tokens, nTokens);

    /*turn on the layers and make sure projections are set properly*/
    for (j=0; j< iFIDLayers; j++) {
      for (k=0; k<map->numlayers; k++) {
        layerObj *lp;
        lp = GET_LAYER(map, k);
        if (msWFSIsLayerSupported(lp) && lp->name &&
            strcasecmp(lp->name, aFIDLayers[j]) == 0) {
          lp->status = MS_ON;
        }
        if (msWFSGetFeatureApplySRS(map, paramsObj->pszSrs, nWFSVersion) == MS_FAILURE)
          return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
      }
    }

    for (j=0; j< iFIDLayers; j++) {
      for (k=0; k<map->numlayers; k++) {
        layerObj *lp;
        lp = GET_LAYER(map, k);
        if (msWFSIsLayerSupported(lp) && lp->name &&
            strcasecmp(lp->name, aFIDLayers[j]) == 0) {
          lp->status = MS_ON;
          if (lp->template == NULL) {
            /* Force setting a template to enable query. */
            lp->template = msStrdup("ttt.html");
          }
          psNode = FLTCreateFeatureIdFilterEncoding(aFIDValues[j]);
          
          if( FLTApplyFilterToLayer(psNode, map, lp->index) != MS_SUCCESS ) {
            msSetError(MS_WFSERR, "FLTApplyFilterToLayer() failed", "msWFSGetFeature");
            return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
          }

          FLTFreeFilterEncodingNode( psNode );
          psNode = NULL;
          break;
        }
      }
      if (k == map->numlayers) { /*layer not found*/
        msSetError(MS_WFSERR,
                   "Invalid typename given with FeatureId in GetFeature : %s. A layer might be disabled for \
this request. Check wfs/ows_enable_request settings.", "msWFSGetFeature()",
                   aFIDLayers[j]);

        if (aFIDLayers && aFIDValues) {
          for (j=0; j<iFIDLayers; j++) {
            msFree(aFIDLayers[j]);
            msFree(aFIDValues[j]);
          }
          msFree(aFIDLayers);
          msFree(aFIDValues);
        }
        return msWFSException(map, "featureid", "InvalidParameterValue", paramsObj->pszVersion);
      }
    }

    if (aFIDLayers && aFIDValues) {
      for (j=0; j<iFIDLayers; j++) {
        msFree(aFIDLayers[j]);
        msFree(aFIDValues[j]);
      }
      msFree(aFIDLayers);
      msFree(aFIDValues);
    }
  }
#endif /* USE_OGR */

  /* Apply the requested SRS */
  if (msWFSGetFeatureApplySRS(map, paramsObj->pszSrs, nWFSVersion) == MS_FAILURE)
    return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
  
  /*
  ** Perform Query (only BBOX for now)
  */
  /* __TODO__ Using a rectangle query may not be the most efficient way */
  /* to do things here. */
  if (!bFilterSet && !bFeatureIdSet) {

    if (!bBBOXSet) {
      const char *pszMapSRS=NULL, *pszLayerSRS=NULL;
      bbox = map->extent;
      map->query.type = MS_QUERY_BY_RECT; /* setup the query */
      map->query.mode = MS_QUERY_MULTIPLE;

      /*if srsName was given for wfs 1.1.0, It is at this point loaded into the
        map object and should be used*/
      if(!paramsObj->pszSrs)
        pszMapSRS = msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FO", MS_TRUE);
      for(j=0; j<map->numlayers; j++) {
        layerObj *lp;
        rectObj ext;
        int status;
        lp = GET_LAYER(map, j);
        if (lp->status == MS_ON) {
          if (msOWSGetLayerExtent(map, lp, "FO", &ext) == MS_SUCCESS) {

            /* For a single point layer, to avoid numerical precision issues */
            /* when reprojection is involved */
            ext.minx -= 1e-5;
            ext.miny -= 1e-5;
            ext.maxx += 1e-5;
            ext.maxy += 1e-5;

            if (pszMapSRS != NULL && strncmp(pszMapSRS, "EPSG:", 5) == 0) {

              if( nWFSVersion >= OWS_1_1_0 )
                status = msLoadProjectionStringEPSG(&(map->projection), pszMapSRS);
              else
                status = msLoadProjectionString(&(map->projection), pszMapSRS);

              if (status != 0) {
                msSetError(MS_WFSERR, "msLoadProjectionString() failed: %s",
                           "msWFSGetFeature()", pszMapSRS);
                return msWFSException(map, "mapserv", "NoApplicableCode",
                                      paramsObj->pszVersion);
              }

            }

            /*make sure that the layer projectsion is loaded.
              It could come from a ows/wfs_srs metadata*/
            if (lp->projection.numargs == 0) {
              pszLayerSRS = msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FO", MS_TRUE);
              if (pszLayerSRS) {
                if (strncmp(pszLayerSRS, "EPSG:", 5) == 0) {
                  if( nWFSVersion >= OWS_1_1_0 )
                    msLoadProjectionStringEPSG(&(lp->projection), pszLayerSRS);
                  else
                    msLoadProjectionString(&(lp->projection), pszLayerSRS);
                }
              }
            }

            if (msProjectionsDiffer(&map->projection, &lp->projection) == MS_TRUE) {
              msProjectRect(&lp->projection, &map->projection, &(ext));
            }
            bbox = ext;
          }
          map->query.rect = bbox;
          map->query.layer = j;
          if(msQueryByRect(map) != MS_SUCCESS) {
            errorObj   *ms_error;
            ms_error = msGetErrorObj();

            if(ms_error->code != MS_NOTFOUND) {
              msSetError(MS_WFSERR, "ms_error->code not found", "msWFSGetFeature()");
              return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
            }
          }
        }
      }
    } else {

      char* sBBoxSrs = NULL;

      if( pszBBOXSRS != NULL )
          sBBoxSrs = msStrdup(pszBBOXSRS);

      /* On WFS 2.0.0, if the BBOX has no explicit SRS, use the map SRS */
      /* Should likely be used for WFS 1.1.0 too, but don't want to cause */
      /* issue at that point, since it can influence axis ordering */
      if( nWFSVersion >= OWS_2_0_0 && sBBoxSrs == NULL )
      {
          const char* srsConst;
          projectionObj sProjTmp;

          msInitProjection(&sProjTmp);
          srsConst = msOWSGetEPSGProj(&sProjTmp,&(map->web.metadata),"FO",MS_TRUE);
          if( srsConst != NULL )
              sBBoxSrs = msStrdup(srsConst);
          msFreeProjection(&sProjTmp);
      }

      if (sBBoxSrs) {
        int status;
        projectionObj sProjTmp;

        msInitProjection(&sProjTmp);
        /*do the axis order for now. It is unclear if the bbox are expected
          ro respect the axis oder defined in the projectsion #3296*/

        if(nWFSVersion >= OWS_1_1_0) {
          if ((status=msLoadProjectionStringEPSG(&sProjTmp, sBBoxSrs)) == 0) {
            msAxisNormalizePoints( &sProjTmp, 1, &bbox.minx, &bbox.miny );
            msAxisNormalizePoints( &sProjTmp, 1, &bbox.maxx, &bbox.maxy );
          }
        } else
          status = msLoadProjectionString(&sProjTmp, sBBoxSrs);

        if (status == 0 &&  map->projection.numargs > 0)
          msProjectRect(&sProjTmp, &map->projection, &bbox);
        msFreeProjection(&sProjTmp);

        msFree(sBBoxSrs);
        sBBoxSrs = NULL;
      }
      map->query.type = MS_QUERY_BY_RECT; /* setup the query */
      map->query.mode = MS_QUERY_MULTIPLE;
      map->query.rect = bbox;

      if(msQueryByRect(map) != MS_SUCCESS) {
        errorObj   *ms_error;
        ms_error = msGetErrorObj();

        if(ms_error->code != MS_NOTFOUND) {
          msSetError(MS_WFSERR, "ms_error->code not found", "msWFSGetFeature()");
          return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
        }
      }
    }
  }

  return MS_SUCCESS;
}


/*
** msWFSGetFeature()
*/
static
int msWFSGetFeature(mapObj *map, wfsParamsObj *paramsObj, cgiRequestObj *req,
                    owsRequestObj *ows_request, int nWFSVersion)
/* const char *wmtver, char **names, char **values, int numentries) */
{
  int   j, status;
  int   maxfeatures=-1,startindex=-1;
  rectObj     bbox;

  char **layers = NULL;
  int numlayers = 0;

  char   *pszFilter = NULL;
  int bFilterSet = 0;
  int bBBOXSet = 0;
  char *sBBoxSrs = NULL;
  int bFeatureIdSet = 0;

  const char *tmpmaxfeatures = NULL;
  WFSGMLInfo gmlinfo;

  OWSGMLVersion outputformat = OWS_GML2; /* default output is GML 2.1 */
  outputFormatObj *psFormat = NULL;

  int iNumberOfFeatures = 0;
  int iResultTypeHits = 0;
  int nMatchingFeatures = -1;

  char **papszPropertyName = NULL;
  int nPropertyNames = 0;
  int nQueriedLayers=0;
  layerObj *lpQueried=NULL;

  /*use msLayerGetShape instead of msLayerResultsGetShape of complex filter #3305
  int bComplexFilter = MS_FALSE;
  */

  /* Initialize gml options */
  memset( &gmlinfo, 0, sizeof(gmlinfo) );
  gmlinfo.user_namespace_prefix = WFS_DEFAULT_NAMESPACE_PREFIX;
  gmlinfo.user_namespace_uri = WFS_DEFAULT_NAMESPACE_URI;
  gmlinfo.collection_name = OWS_WFS_FEATURE_COLLECTION_NAME;
  gmlinfo.typename = "";
  gmlinfo.output_mime_type = "text/xml";
  gmlinfo.output_schema_format = "XMLSCHEMA";

  /* Default filter is map extents */
  bbox = map->extent;

  /* Read CGI parameters */
  /*  */
  /* __TODO__ Need to support XML encoded requests */
  /*  */

  if (paramsObj->pszResultType != NULL) {
    if (strcasecmp(paramsObj->pszResultType, "hits") == 0)
      iResultTypeHits = 1;
  }

  /* typename is mandatory unless featureid is specfied. We do not
     support featureid */
  if (paramsObj->pszTypeName==NULL && paramsObj->pszFeatureId == NULL) {
    msSetError(MS_WFSERR,
               "Incomplete WFS request: %s parameter missing",
               "msWFSGetFeature()",
               (nWFSVersion >= OWS_2_0_0 ) ? "TYPENAMES": "TYPENAME" );
    return msWFSException(map, (nWFSVersion >= OWS_2_0_0 ) ? "typenames": "typename",
                          "MissingParameterValue", paramsObj->pszVersion);
  }

  if(paramsObj->pszTypeName) {
    int j, k, y,z;

    char **tokens;
    int n=0, i=0;
    char szTmp[256];
    const char *pszFullName = NULL;

    /* keep a ref for layer use. */
    gmlinfo.typename = paramsObj->pszTypeName;

    /* Parse comma-delimited list of type names (layers) */
    /*  */
    /* __TODO__ Need to handle type grouping, e.g. "(l1,l2),l3,l4" */
    /*  */
    layers = msStringSplit(gmlinfo.typename, ',', &numlayers);

    /* ==================================================================== */
    /*      TODO: check if the typename contains namespaces (ex cdf:Other), */
    /*      If that is the case extract only the layer name.                */
    /* ==================================================================== */

    if (layers==NULL || numlayers < 1) {
      msSetError(MS_WFSERR, "At least one type name required in %s parameter.", "msWFSGetFeature()",
                 (nWFSVersion >= OWS_2_0_0 ) ? "TYPENAMES": "TYPENAME" );
      return msWFSException(map, (nWFSVersion >= OWS_2_0_0 ) ? "typenames": "typename" ,
                            "InvalidParameterValue", paramsObj->pszVersion);
    }

    tokens = msStringSplit(layers[0], ':', &n);
    if (tokens && n==2 && msGetLayerIndex(map, layers[0]) < 0) {
      msFreeCharArray(tokens, n);
      for (i=0; i<numlayers; i++) {
        tokens = msStringSplit(layers[i], ':', &n);
        if (tokens && n==2) {
          free(layers[i]);
          layers[i] = msStrdup(tokens[1]);
        }
        if (tokens)
          msFreeCharArray(tokens, n);
      }
    } else
      msFreeCharArray(tokens, n);



    /* Keep only selected layers, set to OFF by default. */
    for(j=0; j<map->numlayers; j++) {
      layerObj *lp;

      lp = GET_LAYER(map, j);

      /* Keep only selected layers, set to OFF by default. */
      lp->status = MS_OFF;
    }

    for (k=0; k<numlayers; k++) {
      int bLayerFound = MS_FALSE;

      for (j=0; j<map->numlayers; j++) {
        layerObj *lp;
        char   *pszPropertyName = NULL;

        lp = GET_LAYER(map, j);

        if (msWFSIsLayerSupported(lp) && lp->name && (strcasecmp(lp->name, layers[k]) == 0) &&
            (msIntegerInArray(lp->index, ows_request->enabled_layers, ows_request->numlayers)) ) {
          bLayerFound = MS_TRUE;

          lp->status = MS_ON;
          if (lp->template == NULL) {
            /* Force setting a template to enable query. */
            lp->template = msStrdup("ttt.html");
          }

          /* set the gml_include_items METADATA */
          if (paramsObj->pszPropertyName) {
            pszPropertyName = paramsObj->pszPropertyName;

            /*we parse the propertyname parameter only once*/
            if (papszPropertyName == NULL) {
              if (strlen(pszPropertyName) > 0 && (pszPropertyName[0] == '(' || numlayers == 1)) {
                if (numlayers == 1 && pszPropertyName[0] != '(') {
                  /* Accept PROPERTYNAME without () when there is a single TYPENAME */
                  char* pszTmpPropertyName = msSmallMalloc(1+strlen(pszPropertyName)+1+1);
                  sprintf(pszTmpPropertyName, "(%s)", pszPropertyName);
                  tokens = msStringSplit(pszTmpPropertyName+1, '(', &nPropertyNames);
                  free(pszTmpPropertyName);
                } else
                  tokens = msStringSplit(pszPropertyName+1, '(', &nPropertyNames);

                /*expecting to always have a list of property names equal to
                  the number of layers(typename)*/
                if (nPropertyNames != numlayers) {
                  if (tokens)
                    msFreeCharArray(tokens, nPropertyNames);
                  msSetError(MS_WFSERR,
                             "Optional PROPERTYNAME parameter. A list of properties may be specified for each type name. Example TYPENAME=name1&name2&PROPERTYNAME=(prop1,prop2)(prop1)",
                             "msWFSGetFeature()");
                  return msWFSException(map, "PROPERTYNAME", "InvalidParameterValue", paramsObj->pszVersion);
                }

                papszPropertyName = (char **)msSmallMalloc(sizeof(char *)*nPropertyNames);
                for (i=0; i<nPropertyNames; i++) {
                  if (strlen(tokens[i]) > 0) {
                    /*trim namespaces. PROPERTYNAME=(ns:prop1,ns:prop2)(prop1)*/
                    if (strstr(tokens[i], ":")) {
                      char **tokens1, **tokens2;
                      int n1=0,n2=0,l=0;
                      char *pszTmp = NULL;

                      tokens1 = msStringSplit(tokens[i], ',', &n1);
                      for (l=0; l<n1; l++) {
                        if (pszTmp!=NULL)
                          pszTmp = msStringConcatenate(pszTmp,",");

                        if (strstr(tokens1[l],":")) {
                          tokens2 = msStringSplit(tokens1[l], ':', &n2);
                          if (tokens2 && n2==2)
                            pszTmp = msStringConcatenate(pszTmp, tokens2[1]);
                          else
                            pszTmp = msStringConcatenate(pszTmp,tokens1[l]);
                          if (tokens2 && n2>0)
                            msFreeCharArray(tokens2, n2);
                        } else
                          pszTmp = msStringConcatenate(pszTmp,tokens1[l]);
                      }
                      papszPropertyName[i] = msStrdup(pszTmp);
                      msFree(pszTmp);
                      if (tokens1 && n1>0)
                        msFreeCharArray(tokens1, n1);
                    } else
                      papszPropertyName[i] = msStrdup(tokens[i]);
                    /* remove trailing ) */
                    papszPropertyName[i][strlen(papszPropertyName[i])-1] = '\0';
                  } else
                    papszPropertyName[i] = NULL; /*should return an error*/
                }
                if (tokens)
                  msFreeCharArray(tokens, nPropertyNames);
              } else {
                msSetError(MS_WFSERR,
                           "Optional PROPERTYNAME parameter. A list of properties may be specified for each type name. Example TYPENAME=name1&name2&PROPERTYNAME=(prop1,prop2)(prop1)",
                           "msWFSGetFeature()");
                return msWFSException(map, "PROPERTYNAME", "InvalidParameterValue", paramsObj->pszVersion);
              }
            }
            /*do an alias replace for the current layer*/
            if (papszPropertyName && msLayerOpen(lp) == MS_SUCCESS && msLayerGetItems(lp) == MS_SUCCESS) {
              for(z=0; z<lp->numitems; z++) {
                if (!lp->items[z] || strlen(lp->items[z]) <= 0)
                  continue;
                snprintf(szTmp, sizeof(szTmp), "%s_alias", lp->items[z]);
                pszFullName = msOWSLookupMetadata(&(lp->metadata), "G", szTmp);
                if (pszFullName)
                  papszPropertyName[k] = msReplaceSubstring(papszPropertyName[k], pszFullName, lp->items[z]);

              }

              /*validate that the property names passed are part of the items list*/
              tokens = msStringSplit(papszPropertyName[k], ',', &n);
              for (y=0; y<n; y++) {
                if (tokens[y] && strlen(tokens[y]) > 0) {
                  if (strcasecmp(tokens[y], "*") == 0 ||
                      strcasecmp(tokens[y], "!") == 0)
                    continue;
                  for(z=0; z<lp->numitems; z++) {
                    if (strcasecmp(tokens[y], lp->items[z]) == 0)
                      break;
                  }
                  /*we need to check of the property name is the geometry name; In that case it
                    is a valid property name*/
                  if (msOWSLookupMetadata(&(lp->metadata), "OFG", "geometries") != NULL)
                    snprintf(szTmp, sizeof(szTmp), "%s", msOWSLookupMetadata(&(lp->metadata), "OFG", "geometries"));
                  else
                    snprintf(szTmp, sizeof(szTmp), OWS_GML_DEFAULT_GEOMETRY_NAME);
                  if (z == lp->numitems && strcasecmp(tokens[y], szTmp) != 0) {
                    msSetError(MS_WFSERR,
                               "Invalid PROPERTYNAME %s",  "msWFSGetFeature()", tokens[y]);
                    msFreeCharArray(tokens, n);
                    return msWFSException(map, "PROPERTYNAME", "InvalidParameterValue", paramsObj->pszVersion);
                  }
                }
              }
              if (tokens && n > 0)
                msFreeCharArray(tokens, n);
              msLayerClose(lp);
            }


            if (papszPropertyName) {
              if (strlen(papszPropertyName[k]) > 0) {
                if (strcasecmp(papszPropertyName[k], "*") == 0) {
                  msInsertHashTable(&(lp->metadata), "GML_INCLUDE_ITEMS", "all");
                }
                /*this character is only used internally and allows postrequest to
                  have a proper property name parsing. It means do not affect what was set
                in the map file, It is set necessary when a wfs post request is used with
                several query elements, with some having property names and some not*/
                else if (strcasecmp(papszPropertyName[k], "!") == 0) {
                } else {
                  msInsertHashTable(&(lp->metadata), "GML_INCLUDE_ITEMS", papszPropertyName[k]);

                  /* exclude geometry if it was not asked for */
                  if (msOWSLookupMetadata(&(lp->metadata), "OFG", "geometries") != NULL)
                    snprintf(szTmp, sizeof(szTmp), "%s", msOWSLookupMetadata(&(lp->metadata), "OFG", "geometries"));
                  else
                    snprintf(szTmp, sizeof(szTmp), OWS_GML_DEFAULT_GEOMETRY_NAME);

                  if (strstr(papszPropertyName[k], szTmp) == NULL)
                    msInsertHashTable(&(lp->metadata), "GML_GEOMETRIES", "none");
                }
              } else /*empty string*/
                msInsertHashTable(&(lp->metadata), "GML_GEOMETRIES", "none");

            }
          }
        }
      }

      if (!bLayerFound) {
        /* Requested layer name was not in capabilities:
         * either it just doesn't exist
         */
        msSetError(MS_WFSERR,
                   "TYPENAME '%s' doesn't exist in this server.  Please check the capabilities and reformulate your request.",
                   "msWFSGetFeature()", layers[k]);
        return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
      }
    }
  }


  if (papszPropertyName && nPropertyNames > 0) {
    msFreeCharArray(papszPropertyName, nPropertyNames);
  }

  /* Validate outputformat */
  if (paramsObj->pszOutputFormat) {
    /* We support GML2, GML3, GML3.2 for now. */
    if(strcasecmp(paramsObj->pszOutputFormat, "GML2") == 0 ||
       strcasecmp(paramsObj->pszOutputFormat, "text/xml; subtype=gml/2.1.2") == 0) {
      outputformat = OWS_GML2;
      gmlinfo.output_schema_format = "XMLSCHEMA";
      gmlinfo.output_mime_type = "text/xml; subtype=gml/2.1.2";
    } else if(strcasecmp(paramsObj->pszOutputFormat, "GML3") == 0 ||
              strcasecmp(paramsObj->pszOutputFormat, "text/xml; subtype=gml/3.1.1") == 0) {
      outputformat = OWS_GML3;
      gmlinfo.output_schema_format = "SFE_XMLSCHEMA";
      gmlinfo.output_mime_type = "text/xml; subtype=gml/3.1.1";
    } else if(strcasecmp(paramsObj->pszOutputFormat, "GML32") == 0 ||
              strcasecmp(paramsObj->pszOutputFormat, "text/xml; subtype=gml/3.2.1") == 0 ||
              strcasecmp(paramsObj->pszOutputFormat, "application/gml+xml; version=3.2") == 0) {
      outputformat = OWS_GML32;
      gmlinfo.output_schema_format = "application%2Fgml%2Bxml%3B%20version%3D3.2";
      gmlinfo.output_mime_type = "text/xml; subtype=gml/3.2.1";
    } else {
      const char *format_list;
      hashTableObj *md;

      /* validate selected format against all selected layers. */
      for(j=0; j < map->numlayers; j++) {
        layerObj *lp;

        lp = GET_LAYER(map, j);

        if( lp->status != MS_ON )
          continue;

        md = &(lp->metadata);
        format_list = msOWSLookupMetadata(md, "F","getfeature_formatlist");
        if( format_list == NULL ) {
          md = &(map->web.metadata);
          format_list = msOWSLookupMetadata(md, "F","getfeature_formatlist");
        }
        if (format_list) {
          psFormat = msOwsIsOutputFormatValid(
                       map, paramsObj->pszOutputFormat, md,
                       "F", "getfeature_formatlist");
        }
        if (psFormat == NULL) {
          msSetError(MS_WFSERR,
                     "'%s' is not a permitted output format for layer '%s', review wfs_getfeature_formatlist setting.",
                     "msWFSGetFeature()",
                     paramsObj->pszOutputFormat,
                     lp->name );
          return msWFSException(map, "outputformat",
                                "InvalidParameterValue",
                                paramsObj->pszVersion );
        }

        if( psFormat->imagemode != MS_IMAGEMODE_FEATURE ) {
          msSetError(MS_WFSERR,
                     "OUTPUTFORMAT '%s' does not have IMAGEMODE FEATURE, and is not permitted for WFS output.",
                     "msWFSGetFeature()",
                     paramsObj->pszOutputFormat );
          return msWFSException( map, "outputformat",
                                 "InvalidParameterValue",
                                 paramsObj->pszVersion );
        }
      }
    }

    /* If OUTPUTFORMAT not set, default to gml */
  } else {
    /*set the output format using the version if available*/
    if( nWFSVersion == OWS_1_1_0 ) {
      outputformat = OWS_GML3;
      gmlinfo.output_schema_format = "text/xml;%20subtype=gml/3.1.1";
      gmlinfo.output_mime_type = "text/xml; subtype=gml/3.1.1";
    }
    else if( nWFSVersion >= OWS_2_0_0 ) {
      outputformat = OWS_GML32;
      gmlinfo.output_schema_format = "application%2Fgml%2Bxml%3B%20version%3D3.2";
      gmlinfo.output_mime_type = "text/xml; subtype=gml/3.2.1";
    }
  }

  if( nWFSVersion == OWS_1_0_0 ) {
    gmlinfo.output_mime_type = "text/xml";
  }

  /* else if (strcasecmp(names[i], "PROPERTYNAME") == 0) */
  /* { */
  /*  */
  /* } */

  tmpmaxfeatures = msOWSLookupMetadata(&(map->web.metadata), "FO", "maxfeatures");
  if (tmpmaxfeatures)
    maxfeatures = atoi(tmpmaxfeatures);
  if (paramsObj->nMaxFeatures >= 0) {
    if (maxfeatures < 0 || (maxfeatures > 0 && paramsObj->nMaxFeatures < maxfeatures))
      maxfeatures = paramsObj->nMaxFeatures;
  }

  nQueriedLayers=0;
  for(j=0; j<map->numlayers; j++) {
    layerObj *lp;
    lp = GET_LAYER(map, j);
    if (lp->status == MS_ON) {
      /* No reason to handle tolerances for WFS GetFeature */
      lp->tolerance = 0;
      lpQueried = GET_LAYER(map, j);
      nQueriedLayers++;
    }
  }

  /* WFS convention is that STARTINDEX=0 means the first feature, whereas */
  /* in MapServer internals we used another interpretation with STARTINDEX=1 */
  /* being the first index. This was before that the SWG WFS clarified things */
  /* hence the mess. Ultimately we should likely fix our internals to avoid */
  /* possible confusion */
  if (paramsObj->nStartIndex >= 0) {
    startindex = 1 + paramsObj->nStartIndex;
    map->query.startindex = startindex;    
  } 


  /* maxfeatures set */
  if (maxfeatures >= 0) {
    for(j=0; j<map->numlayers; j++) {
      layerObj *lp;
      lp = GET_LAYER(map, j);
      if (lp->status == MS_ON) {
        /*over-ride the value only if it is unset or wfs maxfeatures is
          lower that what is currently set*/
        if (lp->maxfeatures <=0 || (lp->maxfeatures > 0 && maxfeatures < lp->maxfeatures))
          lp->maxfeatures = maxfeatures;
      }
    }
    map->query.maxfeatures = maxfeatures;
  }

  /* startindex set */
  if (startindex > 0 && nQueriedLayers > 1) {
    for(j=0; j<map->numlayers; j++) {
      layerObj *lp;
      lp = GET_LAYER(map, j);
      if (lp->status == MS_ON) {
        msLayerEnablePaging(lp, MS_FALSE);
      }
    }
  } else if (startindex > 0 && lpQueried) {
    lpQueried->startindex = startindex;    
  }

  if (paramsObj->pszFilter) {
    bFilterSet = 1;
    pszFilter = paramsObj->pszFilter;
  }
  if (paramsObj->pszBbox) {
    char **tokens;
    int n;


    tokens = msStringSplit(paramsObj->pszBbox, ',', &n);
    if (tokens==NULL || (n != 4 && n !=5)) {
      msSetError(MS_WFSERR, "Wrong number of arguments for BBOX.", "msWFSGetFeature()");
      return msWFSException(map, "bbox", "InvalidParameterValue", paramsObj->pszVersion);
    }
    bbox.minx = atof(tokens[0]);
    bbox.miny = atof(tokens[1]);
    bbox.maxx = atof(tokens[2]);
    bbox.maxy = atof(tokens[3]);
    /*5th aregument is assumed to be projection*/
    if (n == 5)
      sBBoxSrs = msStrdup(tokens[4]);

    msFreeCharArray(tokens, n);
    bBBOXSet = 1;
    /* Note: BBOX SRS is implicit, it is the SRS of the selected */
    /* feature types, see pszOutputSRS in TYPENAMES above. */
  }
  if (paramsObj->pszFeatureId) {
    bFeatureIdSet = 1;
  }
  
  if( iResultTypeHits == 1 )
  {
      map->query.only_cache_result_count = MS_TRUE;
  }


  status = msWFSRetrieveFeatures(map,
                           paramsObj,
                           &gmlinfo,
                           bFilterSet,
                           pszFilter,
                           bBBOXSet,
                           sBBoxSrs,
                           bbox,
                           bFeatureIdSet,
                           layers,
                           numlayers,
                           nWFSVersion);
  if( status != MS_SUCCESS )
  {
      if(layers)
        msFreeCharArray(layers, numlayers);
      msFree(sBBoxSrs);
      return status;
  }

  /* Count total number of features retrieved */
  for(j=0; j<map->numlayers; j++) {
    if (GET_LAYER(map, j)->resultcache && GET_LAYER(map, j)->resultcache->numresults > 0) {
      iNumberOfFeatures += GET_LAYER(map, j)->resultcache->numresults;
    }
  }
  
  /* ----------------------------------------- */
  /* Now compute nMatchingFeatures for WFS 2.0 */
  /* ----------------------------------------- */
  
  if( nWFSVersion >= OWS_2_0_0 )
  {
    /* If no features have been retrived and there was no feature count limit, then */
    /* it is obvious. */
    if( iNumberOfFeatures == 0 && maxfeatures < 0 )
    {
        nMatchingFeatures = 0;
    }

    /* In the case we had limited the query by a maximum number of features, and */
    /* that the query has returned less features than the limit, or if there was */
    /* no limit in fature count, we are able to determine the number of matching */
    /* features */
    else if( iNumberOfFeatures > 0 && (maxfeatures < 0 || iNumberOfFeatures < maxfeatures) )
    {
        if( paramsObj->nStartIndex >= 0 )
            nMatchingFeatures = iNumberOfFeatures + paramsObj->nStartIndex;
        else
            nMatchingFeatures = iNumberOfFeatures;
    }

    /* Otherwise big hammer ! */
    else
    {
        /* The WFS 2.0 spec is not really clear on numberMatched behaviour */
        /* and there are interesting logic in server implementations. */
        /* */
        /* Deegree (http://deegree3-demo.deegree.org:80/inspire-workspace/services/wfs?) */
        /* will return numberMatched="19005" on the cp:CadastralParcel layer for a */
        /* resultType=hits request, whereas the server limitation is set to 15000 */
        /* But on a regular GetFeature (without count specified), it will return */
        /* numberMatched="15000" (and numberReturned="15000")) */
        /* */
        /* GeoServer 2.4.1 will also ignore its server limitation for a resultType=hits */
        /* but when using a regular GetFeature (without count specified), il will return */
        /* numberMatched=numberReturned=total_number_of_features_without_taking_into_account_server_limit */
        /* but the number of actually returned features will be truncated to the server */
        /* limit, likely a bug */
        /* */
        /* If wfs_compute_number_matched is set to TRUE, we will issue a full */
        /* count of potentially matching features, ignoring any client or server */
        /* side limits. */

        const char* pszComputeNumberMatched =
                msOWSLookupMetadata(&(map->web.metadata), "F",
                                                "compute_number_matched");
        if( pszComputeNumberMatched != NULL &&
            strcasecmp(pszComputeNumberMatched, "true") == 0 )
        {
            resultCacheObj** saveResultCache = (resultCacheObj** )
                msSmallMalloc( map->numlayers * sizeof(resultCacheObj*));

            /* Save the result cache that contains the features that we want to */
            /* emit in the response */
            for(j=0; j<map->numlayers; j++) {
                layerObj* lp = GET_LAYER(map, j);
                saveResultCache[j] = lp->resultcache;
                lp->resultcache = NULL;

                /* Resent layer paging */
                lp->maxfeatures = -1;
                lp->startindex = -1;
            }

            /* Re-run the query but with no limit */
            map->query.maxfeatures = -1;
            map->query.startindex = -1;
            map->query.only_cache_result_count = MS_TRUE;

            /* TODO: use a to-be-defined virtual layer method to get the feature */
            /* count without iterating over the features */
            msWFSRetrieveFeatures(map,
                                paramsObj,
                                &gmlinfo,
                                bFilterSet,
                                pszFilter,
                                bBBOXSet,
                                sBBoxSrs,
                                bbox,
                                bFeatureIdSet,
                                layers,
                                numlayers,
                                nWFSVersion);

            /* Count number of matched features and restore the original result cache */
            nMatchingFeatures = 0;
            for(j=0; j<map->numlayers; j++) {
                layerObj* lp = GET_LAYER(map, j);
                if (lp->resultcache && lp->resultcache->numresults > 0) {
                    nMatchingFeatures += lp->resultcache->numresults;
                }
                if(lp->resultcache) {
                    if(lp->resultcache->results) free(lp->resultcache->results);
                    free(lp->resultcache);
                    lp->resultcache = NULL;
                }
                lp->resultcache = saveResultCache[j];
            }
            msFree(saveResultCache);
        }
    }
  }

  if(layers)
    msFreeCharArray(layers, numlayers);

  /*
  ** GML Header generation.
  */

  status = MS_SUCCESS;

  if( psFormat == NULL ) {
    msIO_setHeader("Content-Type","%s; charset=UTF-8", gmlinfo.output_mime_type);
    msIO_sendHeaders();

    status = msWFSGetFeature_GMLPreamble( map, req, &gmlinfo, paramsObj,
                                 outputformat,
                                 iResultTypeHits,
                                 iNumberOfFeatures,
                                 nMatchingFeatures,
                                 maxfeatures,
                                 nWFSVersion );
    if(status != MS_SUCCESS) {
      return MS_FAILURE;
    }
  }

  /* handle case of maxfeatures = 0 */
  /*internally use a start index that start with 0 as the first index*/
  if( psFormat == NULL ) {
    if(maxfeatures != 0 && iResultTypeHits == 0)
    {
      /* Would make sense for WFS 1.1.0 too ! See #3576 */
      int bUseURN = (nWFSVersion == OWS_2_0_0);
      const char* useurn = msOWSLookupMetadata(&(map->web.metadata), "F", "return_srs_as_urn");
      if (useurn && strcasecmp(useurn, "true") == 0)
        bUseURN = 1;
      else if (useurn && strcasecmp(useurn, "false") == 0)
        bUseURN = 0;

      status = msGMLWriteWFSQuery(map, stdout,
                                  (char *) gmlinfo.user_namespace_prefix,
                                  outputformat,
                                  nWFSVersion,
                                  bUseURN);
    }
  } else {
    mapservObj *mapserv = msAllocMapServObj();

    /* Setup dummy mapserv object */
    mapserv->sendheaders = MS_TRUE;
    mapserv->map = map;
    msFreeCgiObj(mapserv->request);
    mapserv->request = req;
    map->querymap.status = MS_FALSE;

    status = msReturnTemplateQuery( mapserv, psFormat->name, NULL );

    mapserv->request = NULL;
    mapserv->map = NULL;

    msFreeMapServObj( mapserv );

    if( status != MS_SUCCESS ) {
      return msWFSException(map, "mapserv", "NoApplicableCode",
                            paramsObj->pszVersion );
    }
  }

  if( psFormat == NULL && status == MS_SUCCESS ) {
    msWFSGetFeature_GMLPostfix( map, req, &gmlinfo, paramsObj,
                                outputformat,
                                maxfeatures, iResultTypeHits, iNumberOfFeatures,
                                nWFSVersion );
  }

  /*
  ** Done! Now a bit of clean-up.
  */

  return status;
}


#endif /* USE_WFS_SVR */


/*
** msWFSDispatch() is the entry point for WFS requests.
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid WFS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msWFSDispatch(mapObj *map, cgiRequestObj *requestobj, owsRequestObj *ows_request, int force_wfs_mode)
{
#ifdef USE_WFS_SVR
  int status;
  int returnvalue = MS_DONE;
  int nWFSVersion;
  char* validated_language;

  /* static char *wmtver = NULL, *request=NULL, *service=NULL; */
  wfsParamsObj *paramsObj;
  /*
  ** Populate the Params object based on the request
  */
  paramsObj = msWFSCreateParamsObj();
  /* TODO : store also parameters that are inside the map object */
  /* into the paramsObj.  */
  if (msWFSParseRequest(map, requestobj, ows_request, paramsObj, force_wfs_mode) == MS_FAILURE)
    return msWFSException(map, "request", "InvalidRequest", NULL);

  validated_language = msOWSGetLanguageFromList(map, "FO", paramsObj->pszLanguage);
  if (validated_language != NULL) {
    msMapSetLanguageSpecificConnection(map, validated_language);
  }
  msFree(validated_language);

  if (force_wfs_mode) {
    /*request is always required*/
    if (paramsObj->pszRequest==NULL || strlen(paramsObj->pszRequest)<=0) {
      msSetError(MS_WFSERR,
                 "Incomplete WFS request: REQUEST parameter missing",
                 "msWFSDispatch()");
      returnvalue = msWFSException(map, "request", "MissingParameterValue", paramsObj->pszVersion);
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
    }

    /*version:
      wfs 1.0 and 1.1.0 POST request: optional
      wfs 1.0 and 1.1.0 GET request: optional for getcapabilities and required for describefeature and getfeature
     */
    if (paramsObj->pszVersion == NULL && requestobj && requestobj->postrequest == NULL &&
        strcasecmp(paramsObj->pszRequest, "GetCapabilities") != 0) {
      msSetError(MS_WFSERR,
                 "Invalid WFS request: VERSION parameter missing",
                 "msWFSDispatch()");
      returnvalue = msWFSException(map, "version", "MissingParameterValue", paramsObj->pszVersion);
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
    }

    if (paramsObj->pszVersion == NULL || strlen(paramsObj->pszVersion) <=0)
      paramsObj->pszVersion = msStrdup(msWFSGetDefaultVersion(map));

    /*service
      wfs 1.0 and 1.1.0 GET: required and should be set to WFS
      wfs 1.0 POST: required
      wfs 1.1.1 POST: optional
    */
    if ((paramsObj->pszService == NULL || strlen(paramsObj->pszService) == 0) &&
        ((requestobj && requestobj->postrequest == NULL) || strcasecmp(paramsObj->pszVersion,"1.0") ==0)) {
      msSetError(MS_WFSERR,
                 "Invalid WFS request: Missing SERVICE parameter",
                 "msWFSDispatch()");
      returnvalue = msWFSException(map, "service", "MissingParameterValue", paramsObj->pszVersion);
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
    }

    if (paramsObj->pszService == NULL || strlen(paramsObj->pszService) == 0)
      paramsObj->pszService = msStrdup("WFS");

    if (paramsObj->pszService!=NULL && strcasecmp(paramsObj->pszService, "WFS") != 0) {
      msSetError(MS_WFSERR,
                 "Invalid WFS request: SERVICE parameter must be set to WFS",
                 "msWFSDispatch()");
      returnvalue = msWFSException(map, "service", "InvalidParameterValue", paramsObj->pszVersion);
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
    }

  }
  /* If SERVICE is specified then it MUST be "WFS" */
  if (paramsObj->pszService != NULL &&
      strcasecmp(paramsObj->pszService, "WFS") != 0) {
    msWFSFreeParamsObj(paramsObj);
    free(paramsObj);
    paramsObj = NULL;
    return MS_DONE;  /* Not a WFS request */
  }

  /* If SERVICE, VERSION and REQUEST not included than this isn't a WFS req*/
  if (paramsObj->pszService == NULL && paramsObj->pszVersion==NULL &&
      paramsObj->pszRequest==NULL) {
    msWFSFreeParamsObj(paramsObj);
    free(paramsObj);
    paramsObj = NULL;
    return MS_DONE;  /* Not a WFS request */
  }

  /* VERSION *and* REQUEST *and* SERVICE required by all WFS requests including
   * GetCapabilities.
   */
  if (paramsObj->pszVersion==NULL || strlen(paramsObj->pszVersion)<=0) {
    msSetError(MS_WFSERR,
               "Incomplete WFS request: VERSION parameter missing",
               "msWFSDispatch()");

    returnvalue = msWFSException11(map, "version", "MissingParameterValue", msWFSGetDefaultVersion(map));
    msWFSFreeParamsObj(paramsObj);
    free(paramsObj);
    paramsObj = NULL;
    return returnvalue;
  }

  if (paramsObj->pszRequest==NULL || strlen(paramsObj->pszRequest)<=0) {
    msSetError(MS_WFSERR,
               "Incomplete WFS request: REQUEST parameter missing",
               "msWFSDispatch()");
    returnvalue = msWFSException(map, "request", "MissingParameterValue", paramsObj->pszVersion);
    msWFSFreeParamsObj(paramsObj);
    free(paramsObj);
    paramsObj = NULL;
    return returnvalue;
  }

  if (paramsObj->pszService==NULL || strlen(paramsObj->pszService)<=0) {
    msSetError(MS_WFSERR,
               "Incomplete WFS request: SERVICE parameter missing",
               "msWFSDispatch()");

    returnvalue = msWFSException(map, "service", "MissingParameterValue", paramsObj->pszVersion);
    msWFSFreeParamsObj(paramsObj);
    free(paramsObj);
    paramsObj = NULL;
    return returnvalue;
  }

  if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS) {
    msSetError(MS_WFSERR, "msOWSMakeAllLayersUnique() failed", "msWFSDispatch()");
    returnvalue = msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
    msWFSFreeParamsObj(paramsObj);
    free(paramsObj);
    paramsObj = NULL;
    return returnvalue;
  }
  /*
  ** Start dispatching requests
  */
  if (strcasecmp(paramsObj->pszRequest, "GetCapabilities") == 0 ) {
    msOWSRequestLayersEnabled(map, "F", paramsObj->pszRequest, ows_request);
    if (ows_request->numlayers == 0) {
      msSetError(MS_WFSERR, "WFS request not enabled. Check wfs/ows_enable_request settings.", "msWFSDispatch()");
      returnvalue = msWFSException(map, "request", "InvalidParameterValue", paramsObj->pszVersion);
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
    }

    returnvalue = msWFSGetCapabilities(map, paramsObj, requestobj, ows_request);
    msWFSFreeParamsObj(paramsObj);
    free(paramsObj);
    paramsObj = NULL;
    return returnvalue;
  }

  /*
  ** Validate VERSION against the versions that we support... we don't do this
  ** for GetCapabilities in order to allow version negociation.
  */
  if (strcmp(paramsObj->pszVersion, "1.0.0") != 0 &&
      strcmp(paramsObj->pszVersion, "1.1.0") != 0 &&
      strcmp(paramsObj->pszVersion, "2.0.0") != 0) {
    msSetError(MS_WFSERR,
               "WFS Server does not support VERSION %s.",
               "msWFSDispatch()", paramsObj->pszVersion);
    returnvalue = msWFSException(map, "version", "InvalidParameterValue",msWFSGetDefaultVersion(map));
    msWFSFreeParamsObj(paramsObj);
    free(paramsObj);
    paramsObj = NULL;
    return returnvalue;

  }
  
  nWFSVersion = msOWSParseVersionString(paramsObj->pszVersion);

  /* Since the function can still return MS_DONE, we add an extra service check to not call
     msOWSRequestLayersEnabled twice */
  if (strcasecmp(paramsObj->pszService, "WFS") == 0) {
    msOWSRequestLayersEnabled(map, "F", paramsObj->pszRequest, ows_request);
    if (ows_request->numlayers == 0) {
      msSetError(MS_WFSERR, "WFS request not enabled. Check wfs/ows_enable_request settings.", "msWFSDispatch()");
      returnvalue = msWFSException(map, "request", "InvalidParameterValue", paramsObj->pszVersion);
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
    }
  }

  returnvalue = MS_DONE;
  /* Continue dispatching...
   */
  if (strcasecmp(paramsObj->pszRequest, "DescribeFeatureType") == 0)
    returnvalue = msWFSDescribeFeatureType(map, paramsObj, ows_request, nWFSVersion);

  else if (strcasecmp(paramsObj->pszRequest, "GetFeature") == 0)
    returnvalue = msWFSGetFeature(map, paramsObj, requestobj, ows_request, nWFSVersion);

  else if (msWFSGetIndexUnsupportedOperation(paramsObj->pszRequest) >= 0 ) {
    /* Unsupported WFS request */
    msSetError(MS_WFSERR, "Unsupported WFS request: %s", "msWFSDispatch()",
               paramsObj->pszRequest);
    returnvalue = msWFSException(map, paramsObj->pszRequest, "OperationNotSupported", paramsObj->pszVersion);
  } else if (strcasecmp(paramsObj->pszService, "WFS") == 0) {
    /* Invalid WFS request */
    msSetError(MS_WFSERR, "Invalid WFS request: %s", "msWFSDispatch()",
               paramsObj->pszRequest);
    returnvalue = msWFSException(map, "request", "InvalidParameterValue", paramsObj->pszVersion);
  }

  /* This was not detected as a WFS request... let MapServer handle it */
  msWFSFreeParamsObj(paramsObj);
  free(paramsObj);
  paramsObj = NULL;
  return returnvalue;

#else
  msSetError(MS_WFSERR, "WFS server support is not available.",
             "msWFSDispatch()");
  return(MS_FAILURE);
#endif
}

/************************************************************************/
/*                           msWFSCreateParamsObj                       */
/*                                                                      */
/*      Create a parameter object, initialize it.                       */
/*      The caller should free the object using msWFSFreeParamsObj.     */
/************************************************************************/
wfsParamsObj *msWFSCreateParamsObj()
{
  wfsParamsObj *paramsObj = (wfsParamsObj *)calloc(1, sizeof(wfsParamsObj));
  MS_CHECK_ALLOC(paramsObj, sizeof(wfsParamsObj), NULL);

  paramsObj->nMaxFeatures = -1;
  paramsObj->nStartIndex = -1;

  return paramsObj;
}



/************************************************************************/
/*                            msWFSFreeParmsObj                         */
/*                                                                      */
/*      Free params object.                                             */
/************************************************************************/
void msWFSFreeParamsObj(wfsParamsObj *wfsparams)
{
  if (wfsparams) {
    free(wfsparams->pszVersion);
    free(wfsparams->pszUpdateSequence);
    free(wfsparams->pszRequest);
    free(wfsparams->pszService);
    free(wfsparams->pszTypeName);
    free(wfsparams->pszFilter);
    free(wfsparams->pszFilterLanguage);
    free(wfsparams->pszBbox);
    free(wfsparams->pszGeometryName);
    free(wfsparams->pszOutputFormat);
    free(wfsparams->pszFeatureId);
    free(wfsparams->pszSrs);
    free(wfsparams->pszResultType);
    free(wfsparams->pszPropertyName);
    free(wfsparams->pszAcceptVersions);
    free(wfsparams->pszSections);
    free(wfsparams->pszSortBy);
    free(wfsparams->pszLanguage);
  }
}

/************************************************************************/
/*                       msWFSGetDefaultVersion                         */
/************************************************************************/
static
const char *msWFSGetDefaultVersion(mapObj *map)
{
  const char* pszVersion =
    msOWSLookupMetadata(&(map->web.metadata), "F", "getcapabilities_version");
  if( pszVersion != NULL )
  {
      /* Check that the metadata value is one of the recognized versions */
      int iVersion = msOWSParseVersionString(pszVersion);
      int i;
      for( i = 0; i < wfsNumSupportedVersions; i ++ )
      {
          if( iVersion == wfsSupportedVersions[i] )
              return wfsSupportedVersionsStr[i];
      }
      /* If not, use the default (= latest) version */
      msDebug("msWFSGetDefaultVersion(): invalid value for "
              "wfs_getcapabilities_version: %s\n", pszVersion);
  }
  return WFS_LATEST_VERSION;
}

/************************************************************************/
/*                             msWFSSetParam                            */
/************************************************************************/
static int msWFSSetParam(char** ppszOut, cgiRequestObj *request, int i,
                         const char* pszExpectedParamName)
{
    if( strcasecmp(request->ParamNames[i], pszExpectedParamName) == 0 )
    {
        if( *ppszOut == NULL )
            *ppszOut = msStrdup(request->ParamValues[i]);
        return 1;
    }
    return 0;
}

/************************************************************************/
/*                            msWFSParseRequest                         */
/*                                                                      */
/*      Parse request into the params object.                           */
/************************************************************************/
int msWFSParseRequest(mapObj *map, cgiRequestObj *request, owsRequestObj *ows_request,
                      wfsParamsObj *wfsparams, int force_wfs_mode)
{
#ifdef USE_WFS_SVR
  int i = 0;

  if (!request || !wfsparams)
    return MS_FAILURE;

  if (request->NumParams > 0) {
    for(i=0; i<request->NumParams; i++) {
      if (request->ParamNames[i] && request->ParamValues[i]) {
        if( msWFSSetParam(&(wfsparams->pszVersion), request, i, "VERSION") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszUpdateSequence), request, i, "UPDATESEQUENCE") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszRequest), request, i, "REQUEST") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszService), request, i, "SERVICE") )
            ;

        else if (strcasecmp(request->ParamNames[i], "MAXFEATURES") == 0)
          wfsparams->nMaxFeatures = atoi(request->ParamValues[i]);

        /* WFS 2.0 */
        else if (strcasecmp(request->ParamNames[i], "COUNT") == 0)
          wfsparams->nMaxFeatures = atoi(request->ParamValues[i]);

        else if (strcasecmp(request->ParamNames[i], "STARTINDEX") == 0)
          wfsparams->nStartIndex = atoi(request->ParamValues[i]);

        else if( msWFSSetParam(&(wfsparams->pszBbox), request, i, "BBOX") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszSrs), request, i, "SRSNAME") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszResultType), request, i, "RESULTTYPE") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszTypeName), request, i, "TYPENAME") )
            ;

        /* WFS 2.0 */
        else if( msWFSSetParam(&(wfsparams->pszTypeName), request, i, "TYPENAMES") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszFilter), request, i, "FILTER") )
            ;

        /* WFS 2.0 */
        else if( msWFSSetParam(&(wfsparams->pszFilterLanguage), request, i, "FILTER_LANGUAGE") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszOutputFormat), request, i, "OUTPUTFORMAT") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszFeatureId), request, i, "FEATUREID") )
            ;

        /* WFS 2.0 */
        else if( msWFSSetParam(&(wfsparams->pszFeatureId), request, i, "RESOURCEID") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszPropertyName), request, i, "PROPERTYNAME") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszAcceptVersions), request, i, "ACCEPTVERSIONS") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszSections), request, i, "SECTIONS") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszSortBy), request, i, "SORTBY") )
            ;

        else if( msWFSSetParam(&(wfsparams->pszLanguage), request, i, "LANGUAGE") )
            ;
      }
    }
    /* version is optional is the GetCapabilities. If not */
    /* provided, set it. Default it to latest implemented version */
    /* or to the specified one in wfs_getcapabilities_version */
    if (wfsparams->pszVersion == NULL &&
        wfsparams->pszRequest &&
        strcasecmp(wfsparams->pszRequest, "GetCapabilities") == 0) {
      wfsparams->pszVersion = msStrdup(msWFSGetDefaultVersion(map));
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Parse the post request. It is assumed to be an XML document.    */
  /* -------------------------------------------------------------------- */
#ifdef USE_OGR
  if (request->postrequest != NULL) {

    CPLXMLNode *psRoot, *psQuery, *psFilter, *psTypeName,  *psPropertyName  = NULL;
    CPLXMLNode *psGetFeature = NULL;
    CPLXMLNode  *psGetCapabilities = NULL;
    CPLXMLNode *psDescribeFeature = NULL;
    CPLXMLNode *psOperation = NULL;
    const char *pszValue;
    char *pszSerializedFilter, *pszTmp, *pszTmp2 = NULL;
    char *pszSchemaLocation = NULL;
    int bMultiLayer = 0;

    psRoot = CPLParseXMLString(request->postrequest);
    if(map->debug == MS_DEBUGLEVEL_VVV) {
      msDebug("msWFSParseRequest(): WFS post request: %s\n", request->postrequest);
    }
    if (psRoot) {
      /* need to strip namespaces */
      CPLStripXMLNamespace(psRoot, NULL, 1);

      for( psOperation = psRoot;
           psOperation != NULL;
           psOperation = psOperation->psNext ) {
        if(psOperation->eType == CXT_Element) {

          /* keep schemaLocation so as to be able to validate against appropriate */
          /* wfs.xsd schema afterwards */
          if( pszSchemaLocation == NULL &&
              CPLGetXMLValue(psOperation, "schemaLocation", NULL) != NULL )
             pszSchemaLocation = msStrdup( CPLGetXMLValue(psOperation, "schemaLocation", NULL) );

          if(strcasecmp(psOperation->pszValue,"GetFeature")==0) {
            psGetFeature = psOperation;
            break;
          } else if(strcasecmp(psOperation->pszValue,"GetCapabilities")==0) {
            psGetCapabilities = psOperation;
            break;
          } else if(strcasecmp(psOperation->pszValue,
                               "DescribeFeatureType")==0) {
            psDescribeFeature = psOperation;
            break;
          }
          /* these are unsupported requests. Just set the  */
          /* request value and return; */
          else if (msWFSGetIndexUnsupportedOperation(psOperation->pszValue) >= 0) {
            int idx = msWFSGetIndexUnsupportedOperation(psOperation->pszValue);
            wfsparams->pszRequest = msStrdup(wfsUnsupportedOperations[idx]);
            break;
          }
        }
      }

      if( psOperation != NULL )
      {
        pszValue = CPLGetXMLValue(psOperation, "version",
                                  NULL);
        if (pszValue)
          wfsparams->pszVersion = msStrdup(pszValue);

        pszValue = CPLGetXMLValue(psOperation, "service",
                                  NULL);
        if (pszValue)
          wfsparams->pszService = msStrdup(pszValue);
      }

      /* -------------------------------------------------------------------- */
      /*      Parse the GetFeature                                            */
      /* -------------------------------------------------------------------- */
      if (psGetFeature) {
        wfsparams->pszRequest = msStrdup("GetFeature");

        pszValue = CPLGetXMLValue(psGetFeature,  "resultType",
                                  NULL);
        if (pszValue)
          wfsparams->pszResultType = msStrdup(pszValue);

        pszValue = CPLGetXMLValue(psGetFeature,  "maxFeatures",
                                  NULL);
        if (pszValue)
          wfsparams->nMaxFeatures = atoi(pszValue);
        else
        {
            /* WFS 2.0 */
            pszValue = CPLGetXMLValue(psGetFeature,  "count",
                                  NULL);
            if (pszValue)
                wfsparams->nMaxFeatures = atoi(pszValue);
        }

        pszValue = CPLGetXMLValue(psGetFeature,  "startIndex",
                                  NULL);
        if (pszValue)
          wfsparams->nStartIndex = atoi(pszValue);

        pszValue = CPLGetXMLValue(psGetFeature, "outputFormat",
                                  NULL);
        if (pszValue)
          wfsparams->pszOutputFormat = msStrdup(pszValue);

        psQuery = CPLGetXMLNode(psGetFeature, "Query");
        if (psQuery) {

          /* free typname and filter. There may have been */
          /* values if they were passed in the URL */
          if (wfsparams->pszTypeName)
            free(wfsparams->pszTypeName);
          wfsparams->pszTypeName = NULL;

          if (wfsparams->pszFilter)
            free(wfsparams->pszFilter);
          wfsparams->pszFilter = NULL;

          if (psQuery->psNext && psQuery->psNext->pszValue &&
              strcasecmp(psQuery->psNext->pszValue, "Query") == 0)
            bMultiLayer = 1;

          /* -------------------------------------------------------------------- */
          /*      Parse typenames and filters. If there are multiple queries,     */
          /*      typenames will be build with comma between thme                 */
          /*      (typename1,typename2,...) and filters will be build with        */
          /*      bracets enclosinf the filters :(filter1)(filter2)...            */
          /*      propertynames are stored like (property1,property2)(property1)  */
          /* -------------------------------------------------------------------- */
          while (psQuery &&  psQuery->pszValue &&
                 strcasecmp(psQuery->pszValue, "Query") == 0) {
            /* get SRS */
            pszValue = CPLGetXMLValue(psQuery,  "srsName",
                                      NULL);
            if (pszValue)
              wfsparams->pszSrs = msStrdup(pszValue);

            /* parse typenames */
            pszValue = CPLGetXMLValue(psQuery,
                                      "typename", NULL);
            if (pszValue) {
              msWFSBuildParamList(&(wfsparams->pszTypeName), pszValue, ",");
            }
            else
            {
                /* WFS 2.0 */
                pszValue = CPLGetXMLValue(psQuery,
                                      "typeNames", NULL);
                if (pszValue) {
                    msWFSBuildParamList(&(wfsparams->pszTypeName), pszValue, ",");
                }
            }

            /*parse property name*/
            psPropertyName = CPLGetXMLNode(psQuery, "PropertyName");
            pszTmp2= NULL;

            /*when there is no PropertyName, we outpout (!) so that it is parsed properly in GetFeature*/
            if (psPropertyName == NULL)
              pszTmp2 = msStrdup("!");

            while (psPropertyName) {
              if (!psPropertyName->pszValue ||
                  strcasecmp(psPropertyName->pszValue, "PropertyName") != 0) {
                psPropertyName = psPropertyName->psNext;
                continue;
              }
              pszValue = CPLGetXMLValue(psPropertyName, NULL, NULL);
              if (pszTmp2 == NULL) {
                pszTmp2 = msStrdup(pszValue);
              } else {
                pszTmp = msStrdup(pszTmp2);
                pszTmp2 = (char *)msSmallRealloc(pszTmp2, sizeof(char)* (strlen(pszTmp)+ strlen(pszValue)+2));
                sprintf(pszTmp2,"%s,%s",pszTmp, pszValue);
                msFree(pszTmp);
              }
              psPropertyName = psPropertyName->psNext;
            }
            if (pszTmp2) {
              pszTmp = msStrdup(pszTmp2);
              pszTmp2 = (char *)msSmallRealloc(pszTmp2, sizeof(char)* (strlen(pszTmp)+ 3));
              sprintf(pszTmp2,"(%s)", pszTmp);
              msFree(pszTmp);

              msWFSBuildParamList(&(wfsparams->pszPropertyName), pszTmp2, "");
              msFree(pszTmp2);
              pszTmp2 = NULL;
            }

            /* parse filter */
            psFilter = CPLGetXMLNode(psQuery, "Filter");

            if (psFilter) {
              if (!bMultiLayer) {
                char* pszCPLTmp = CPLSerializeXMLTree(psFilter);
                wfsparams->pszFilter = msStrdup(pszCPLTmp);
                CPLFree(pszCPLTmp);
              }
              else {
                pszSerializedFilter = CPLSerializeXMLTree(psFilter);
                pszTmp = (char *)msSmallMalloc(sizeof(char)*
                                               (strlen(pszSerializedFilter)+3));
                sprintf(pszTmp, "(%s)", pszSerializedFilter);
                CPLFree(pszSerializedFilter);

                msWFSBuildParamList(&(wfsparams->pszFilter), pszTmp, "");
                free(pszTmp);
              }
            }

            psQuery = psQuery->psNext;
          }/* while psQuery */
        }

        /* If no PropertyName at all was specified, then clear the field */
        /* that will be easier to generate valid 'next' and 'previous' uri */
        /* for WFS 2.0 GetFeature */
        if( wfsparams->pszPropertyName != NULL )
        {
            i = 0;
            while( strncmp(wfsparams->pszPropertyName + i, "(!)", 3) == 0 )
                i += 3;
            if( wfsparams->pszPropertyName[i] == '\0' )
            {
                msFree(wfsparams->pszPropertyName);
                wfsparams->pszPropertyName = NULL;
            }
        }

      }/* end of GetFeatures */

      /* -------------------------------------------------------------------- */
      /*      Parse GetCapabilities.                                          */
      /* -------------------------------------------------------------------- */
      if (psGetCapabilities) {
        CPLXMLNode* psAcceptVersions;
        CPLXMLNode* psSections;
          
        wfsparams->pszRequest = msStrdup("GetCapabilities");
        
        pszValue = CPLGetXMLValue(psGetCapabilities,  "updateSequence", NULL);
        if (pszValue)
            wfsparams->pszUpdateSequence = msStrdup(pszValue);

        /* version is optional for the GetCapabilities. If not */
        /* provided, set it. */
        if (wfsparams->pszVersion == NULL)
          wfsparams->pszVersion = msStrdup(msStrdup(msWFSGetDefaultVersion(map)));

        psAcceptVersions = CPLGetXMLNode(psGetCapabilities, "AcceptVersions");
        if( psAcceptVersions != NULL )
        {
            CPLXMLNode* psIter = psAcceptVersions->psChild;
            while( psIter != NULL )
            {
                if( psIter->eType == CXT_Element &&
                    strcmp(psIter->pszValue, "Version") == 0 &&
                    psIter->psChild != NULL &&
                    psIter->psChild->eType == CXT_Text )
                {
                    pszValue = psIter->psChild->pszValue;
                    msWFSBuildParamList(&(wfsparams->pszAcceptVersions), pszValue, ",");
                }
                psIter = psIter->psNext;
            }
        }

        psSections = CPLGetXMLNode(psGetCapabilities, "Sections");
        if( psSections != NULL )
        {
            CPLXMLNode* psIter = psSections->psChild;
            while( psIter != NULL )
            {
                if( psIter->eType == CXT_Element &&
                    strcmp(psIter->pszValue, "Section") == 0 &&
                    psIter->psChild != NULL &&
                    psIter->psChild->eType == CXT_Text )
                {
                    pszValue = psIter->psChild->pszValue;
                    msWFSBuildParamList(&(wfsparams->pszSections), pszValue, ",");
                }
                psIter = psIter->psNext;
            }
        }
      }/* end of GetCapabilites */
      /* -------------------------------------------------------------------- */
      /*      Parse DescribeFeatureType                                       */
      /* -------------------------------------------------------------------- */
      if (psDescribeFeature) {
        wfsparams->pszRequest = msStrdup("DescribeFeatureType");

        pszValue = CPLGetXMLValue(psDescribeFeature,
                                  "outputFormat",
                                  NULL);
        if (pszValue)
          wfsparams->pszOutputFormat = msStrdup(pszValue);

        psTypeName = CPLGetXMLNode(psDescribeFeature, "TypeName");
        if (psTypeName) {
          /* free typname and filter. There may have been */
          /* values if they were passed in the URL */
          if (wfsparams->pszTypeName)
            free(wfsparams->pszTypeName);
          wfsparams->pszTypeName = NULL;

          while (psTypeName && psTypeName->pszValue &&
                 strcasecmp(psTypeName->pszValue, "TypeName") == 0) {
            if (psTypeName->psChild && psTypeName->psChild->pszValue) {
              pszValue = psTypeName->psChild->pszValue;
              msWFSBuildParamList(&(wfsparams->pszTypeName), pszValue, ",");
            }
            psTypeName = psTypeName->psNext;
          }
        }

      }/* end of DescibeFeatureType */

      CPLDestroyXMLNode(psRoot);
    }

#if defined(USE_LIBXML2)
    {
        const char *schema_location=NULL, *validate=NULL;

        /*do we validate the xml ?*/
        validate = msOWSLookupMetadata(&(map->web.metadata), "FO", "validate_xml");
        if (validate && strcasecmp(validate, "true") == 0 &&
            (schema_location = msOWSLookupMetadata(&(map->web.metadata), "FO", "schemas_dir")))
        {
            if ((wfsparams->pszService  && strcmp(wfsparams->pszService, "WFS") == 0) ||
                force_wfs_mode)
            {
                char *schema_file =NULL;
                if (pszSchemaLocation != NULL && strstr(pszSchemaLocation, "wfs/1.0") != NULL )
                {
                    schema_file = msStringConcatenate(schema_file, schema_location);
                    schema_file = msStringConcatenate(schema_file, MS_OWSCOMMON_WFS_10_SCHEMA_LOCATION);
                }
                else if (pszSchemaLocation != NULL && strstr(pszSchemaLocation, "wfs/1.1") != NULL )
                {
                    schema_file = msStringConcatenate(schema_file, schema_location);
                    schema_file = msStringConcatenate(schema_file, MS_OWSCOMMON_WFS_11_SCHEMA_LOCATION);
                }
                else if (pszSchemaLocation != NULL && strstr(pszSchemaLocation, "wfs/2.0") != NULL )
                {
                    schema_file = msStringConcatenate(schema_file, schema_location);
                    schema_file = msStringConcatenate(schema_file, MS_OWSCOMMON_WFS_20_SCHEMA_LOCATION);
                }
                if( schema_file != NULL )
                {
                    if (msOWSSchemaValidation(schema_file, request->postrequest) != 0) {
                        msSetError(MS_WFSERR, "Invalid POST request.  XML is not valid", "msWFSParseRequest()");
                        return MS_FAILURE;
                    }
                    msFree(schema_file);
                }
            }
        }
    }
#endif

    msFree(pszSchemaLocation);

  }

#endif
#endif
  return MS_SUCCESS;
}


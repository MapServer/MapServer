/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  WFS server implementation
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2002, Daniel Morissette, DM Solutions Group Inc
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

MS_CVSID("$Id$")

#if defined(USE_WFS_SVR)

/* There is a dependency to GDAL/OGR for the GML driver and MiniXML parser */
#include "cpl_minixml.h"

#include "mapogcfilter.h"

/*
** msWFSException()
**
** Report current MapServer error in XML exception format.
*/

int msWFSException(mapObj *map, const char *locator, const char *code,
                   const char *version )
{
    const char *encoding;
    char *schemalocation = NULL;
    /* In WFS, exceptions are always XML.
    */

    if( version == NULL )
      version = "1.0.0";

    if( msOWSParseVersionString(version) >= OWS_1_1_0 )
      return msWFSException11( map, code, locator, version );

    encoding = msOWSLookupMetadata(&(map->web.metadata), "FO", "encoding");
    if (encoding)
        msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
    else
        msIO_printf("Content-type: text/xml%c%c",10,10);

    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "FO", "encoding", OWS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" ?>\n", "ISO-8859-1");

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

    /* clear error since we have already reported it */
    msResetErrorList();

    return MS_FAILURE; /* so we can call 'return msWFSException();' anywhere */
}



/*
**
*/
static void msWFSPrintRequestCap(const char *wmtver, const char *request,
                                 const char *script_url, 
                                 const char *format_tag, const char *formats, ...)
{
  va_list argp;
  const char *fmt;

  msIO_printf("    <%s>\n", request);

  /* We expect to receive a NULL-terminated args list of formats */
  if (format_tag != NULL)
  {
    msIO_printf("      <%s>\n", format_tag);
    va_start(argp, formats);
    fmt = formats;
    while(fmt != NULL)
    {
      msIO_printf("        <%s/>\n", fmt);
      fmt = va_arg(argp, const char *);
    }
    va_end(argp);
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

/* msWFSIsLayerSupported()
**
** Returns true (1) is this layer meets the requirements to be served as
** a WFS feature type.
*/
int msWFSIsLayerSupported(layerObj *lp)
{
    /* In order to be supported, lp->type must be specified, even for 
    ** layers that are OGR, SDE, SDO, etc connections.
    ** lp->dump must be explicitly set to TRUE in mapfile.
    */
    if (lp->dump && 
        (lp->type == MS_LAYER_POINT ||
         lp->type == MS_LAYER_LINE ||
         lp->type == MS_LAYER_POLYGON ) &&
        lp->connectiontype != MS_WMS && 
        lp->connectiontype != MS_GRATICULE)
    {
        return 1; /* true */
    }

    return 0; /* false */
}


/* msWFSGetGeomElementName()
**
** Return the geometry propery name base on the layer type
*/
const char *msWFSGetGeomElementName(mapObj *map, layerObj *lp)
{

    switch(lp->type)
    {
        case MS_LAYER_POINT:
          return "pointProperty";
        case MS_LAYER_LINE:
          return "lineStringProperty";
        case MS_LAYER_POLYGON:
          return "polygonProperty";
        default:
          break;
    }

    return "???unknown???";
    
}

/* msWFSGetGeomType()
**
** Return GML name for geometry type in this layer
** This is based on MapServer geometry type and layers with mixed geometries
** may not return the right feature type.
*/
#ifdef notdef /* not currently used */
static const char *msWFSGetGeomType(layerObj *lp)
{

    switch(lp->type)
    {
      case MS_LAYER_POINT:
        return "PointPropertyType";
      case MS_LAYER_LINE:
        return "LineStringPropertyType";
      case MS_LAYER_POLYGON:
        return "PolygonPropertyType";
      default:
        break;
    }

    return "???unknown???";
}
#endif /* def notdef */

/*
** msWFSDumpLayer()
*/
int msWFSDumpLayer(mapObj *map, layerObj *lp)
{
   rectObj ext;
   const char *pszWfsSrs = NULL;
   projectionObj poWfs;
   int result = 0;
   
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
   if (msOWSGetEPSGProj(&(map->projection),&(map->web.metadata),"FO",MS_TRUE) != NULL)
   {
       /* Map has a SRS.  Use it for all layers. */
       pszWfsSrs = msOWSGetEPSGProj(&(map->projection),&(map->web.metadata), "FO", MS_TRUE);
   }
   else
   {
       /* Map has no SRS.  Use layer SRS or produce a warning. */
       pszWfsSrs = msOWSGetEPSGProj(&(lp->projection),&(lp->metadata), "FO", MS_TRUE);
   }

   msOWSPrintEncodeParam(stdout, "(at least one of) MAP.PROJECTION, LAYER.PROJECTION or wfs_srs metadata", 
                  pszWfsSrs, OWS_WARN, "        <SRS>%s</SRS>\n", NULL);

   /* If layer has no proj set then use map->proj for bounding box. */
   if (msOWSGetLayerExtent(map, lp, "FO", &ext) == MS_SUCCESS)
   {
       msInitProjection(&poWfs);
       if (pszWfsSrs != NULL)
           result = msLoadProjectionString(&(poWfs), (char *)pszWfsSrs);
       
       if(lp->projection.numargs > 0) 
       {
           msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext), 
                                       &(lp->projection), &(poWfs), OWS_WFS);
       } 
       else 
       {
           msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext), 
                                       &(map->projection), &(poWfs), OWS_WFS);
       }
       msFreeProjection(&poWfs);
   }
   else
   {
       msIO_printf("<!-- WARNING: Optional LatLongBoundingBox could not be established for this layer.  Consider setting the EXTENT in the LAYER object, or wfs_extent metadata. Also check that your data exists in the DATA statement -->\n");
   }

   msOWSPrintURLType(stdout, &(lp->metadata), "FO", "metadataurl", 
                     OWS_NOERR, NULL, "MetadataURL", " type=\"%s\"", 
                     NULL, NULL, " format=\"%s\"", "%s", 
                     MS_TRUE, MS_FALSE, MS_FALSE, MS_TRUE, MS_TRUE, 
                     NULL, NULL, NULL, NULL, NULL, "        ");

   if (msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid") == NULL)
   {
       msIO_fprintf(stdout, "<!-- WARNING: Required Feature Id attribute (fid) not specified for this feature type. Make sure you set one of wfs_featureid, ows_featureid or gml_featureid metadata. -->\n");
   }

   msIO_printf("    </FeatureType>\n");
      
   return MS_SUCCESS;
}

/*
** msWFSGetCapabilities()
*/
int msWFSGetCapabilities(mapObj *map, wfsParamsObj *wfsparams, cgiRequestObj *req) 
{
  char *script_url=NULL, *script_url_encoded;
  const char *updatesequence=NULL;
  const char *wmtver=NULL;
  const char *encoding;
  char tmpString[OWS_VERSION_MAXLEN];
  int wfsSupportedVersions[] = {OWS_1_1_0, OWS_1_0_0};
  int wfsNumSupportedVersions = 2;

  int i=0, tmpInt=0;

  /* negotiate version */
  tmpInt = msOWSNegotiateVersion(msOWSParseVersionString(wfsparams->pszVersion), wfsSupportedVersions, 
                                 wfsNumSupportedVersions);

  /* set result as string and carry on */
  if (wfsparams->pszVersion)
    msFree(wfsparams->pszVersion);
  wfsparams->pszVersion = strdup(msOWSGetVersionString(tmpInt, tmpString));

  if( wfsparams->pszVersion == NULL ||  strncmp(wfsparams->pszVersion,"1.1",3) == 0 )
    return msWFSGetCapabilities11( map, wfsparams, req );

  /* Decide which version we're going to return... only 1.0.0 for now */
  wmtver = strdup("1.0.0");

  /* We need this server's onlineresource. */
  if ((script_url=msOWSGetOnlineResource(map, "FO", "onlineresource", req)) == NULL ||
      (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL)
  {
      msSetError(MS_WFSERR, "Server URL not found", "msWFSGetCapabilities()");
      return msWFSException(map, "mapserv", "NoApplicableCode", wmtver);
  }

  updatesequence = msOWSLookupMetadata(&(map->web.metadata), "FO", "updatesequence");

  if (!updatesequence)
    updatesequence = strdup("0");

  if (wfsparams->pszUpdateSequence != NULL) {
      i = msOWSNegotiateUpdateSequence(wfsparams->pszUpdateSequence, updatesequence);
      if (i == 0) { /* current */
          msSetError(MS_WFSERR, "UPDATESEQUENCE parameter (%s) is equal to server (%s)", "msWFSGetCapabilities()", wfsparams->pszUpdateSequence, updatesequence);
          return msWFSException(map, "updatesequence", "CurrentUpdateSequence", wmtver);
      }
      if (i > 0) { /* invalid */
          msSetError(MS_WFSERR, "UPDATESEQUENCE parameter (%s) is higher than server (%s)", "msWFSGetCapabilities()", wfsparams->pszUpdateSequence, updatesequence);
          return msWFSException(map, "updatesequence", "InvalidUpdateSequence", wmtver);
      }
  }

  encoding = msOWSLookupMetadata(&(map->web.metadata), "FO", "encoding");
    if (encoding)
        msIO_printf("Content-type: text/xml; charset=%s%c%c", encoding,10,10);
    else
        msIO_printf("Content-type: text/xml%c%c",10,10);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "FO", "encoding", OWS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" ?>\n",
                "ISO-8859-1");


  msIO_printf("<WFS_Capabilities \n"
         "   version=\"%s\" \n"
         "   updateSequence=\"%s\" \n"
         "   xmlns=\"http://www.opengis.net/wfs\" \n"
         "   xmlns:ogc=\"http://www.opengis.net/ogc\" \n"
         "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
         "   xsi:schemaLocation=\"http://www.opengis.net/wfs %s/wfs/%s/WFS-capabilities.xsd\">\n",
         wmtver, updatesequence,
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
  msWFSPrintRequestCap(wmtver, "DescribeFeatureType", script_url_encoded, "SchemaDescriptionLanguage", "XMLSCHEMA", NULL);
  msWFSPrintRequestCap(wmtver, "GetFeature", script_url_encoded, "ResultFormat", "GML2", NULL);

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

  for(i=0; i<map->numlayers; i++)
  {
      layerObj *lp;
      lp = GET_LAYER(map, i);

      if (lp->status == MS_DELETE)
         continue;

      /* List only vector layers in which DUMP=TRUE */
      if (msWFSIsLayerSupported(lp))
      {
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

  free(script_url);
  free(script_url_encoded);

  return MS_SUCCESS;
}

/*
** Helper functions for producing XML schema.
*/

static const char *msWFSGetGeometryType(char *type, int outputformat) 
{
  if(!type) return "???undefined???";
  
  if(strcasecmp(type, "point") == 0) {
    switch(outputformat) {
    case OWS_GML2:
    case OWS_GML3:
      return "PointPropertyType";
    }
  } else if(strcasecmp(type, "multipoint") == 0) {
    switch(outputformat) {
    case OWS_GML2:
    case OWS_GML3:
      return "MultiPointPropertyType";
    }
  } else if(strcasecmp(type, "line") == 0) {
    switch(outputformat) {
    case OWS_GML2:
      return "LineStringPropertyType";
    case OWS_GML3:
      return "CurvePropertyType";
    }
  } else if(strcasecmp(type, "multiline") == 0) {
    switch(outputformat) {
    case OWS_GML2:
      return "MultiLineStringPropertyType";
    case OWS_GML3:
      return "MultiCurvePropertyType";
    }
  } else if(strcasecmp(type, "polygon") == 0) {
    switch(outputformat) {
    case OWS_GML2:
      return "PolygonPropertyType";
    case OWS_GML3:
      return "SurfacePropertyType";
    }
  } else if(strcasecmp(type, "multipolygon") == 0) {
    switch(outputformat) {
    case OWS_GML2:
      return "MultiPolygonPropertyType";
    case OWS_GML3:
      return "MultiSurfacePropertyType";
    }
  }

  return "???unkown???";
}

static void msWFSWriteGeometryElement(FILE *stream, gmlGeometryListObj *geometryList, int outputformat, const char *tab)
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
    element_type = item->type;

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
  element_tab = (char *) malloc(sizeof(char)*strlen(tab)+3);
  if(!element_tab) return;
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

/*
** msWFSDescribeFeatureType()
*/
int msWFSDescribeFeatureType(mapObj *map, wfsParamsObj *paramsObj)
{
  int i, numlayers=0;
  char **layers = NULL;
  char **tokens;
  int n=0;

  const char *value;
  const char *user_namespace_prefix = "ms";
  const char *user_namespace_uri = "http://mapserver.gis.umn.edu/mapserver";
  char *user_namespace_uri_encoded = NULL;
  const char *collection_name = OWS_WFS_FEATURE_COLLECTION_NAME;
  char *encoded_name = NULL, *encoded;
  
  int outputformat = OWS_DEFAULT_SCHEMA; /* default output is GML 2.1 compliant schema*/

  gmlNamespaceListObj *namespaceList=NULL; /* for external application schema support */

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
                layers[i] = strdup(tokens[1]);
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

  /*set the output format to gml3 for wfs1.1*/
  if(paramsObj->pszVersion == NULL || strncmp(paramsObj->pszVersion,"1.1",3) == 0 )
    outputformat = OWS_SFE_SCHEMA;

  if (paramsObj->pszOutputFormat) {
    if(strcasecmp(paramsObj->pszOutputFormat, "XMLSCHEMA") == 0 ||
       strstr(paramsObj->pszOutputFormat, "gml/2")!= NULL)
      outputformat = OWS_DEFAULT_SCHEMA;
    else if(strcasecmp(paramsObj->pszOutputFormat, "SFE_XMLSCHEMA") == 0 ||
            strstr(paramsObj->pszOutputFormat, "gml/3")!= NULL)
      outputformat = OWS_SFE_SCHEMA;
    else {
      msSetError(MS_WFSERR, "Unsupported DescribeFeatureType outputFormat (%s).", "msWFSDescribeFeatureType()", paramsObj->pszOutputFormat);
      return msWFSException(map, "outputformat", "InvalidParameterValue", paramsObj->pszVersion);
    }
  }

  /* Validate layers */
  if (numlayers > 0) {
    for (i=0; i<numlayers; i++) {
      if (msGetLayerIndex(map, layers[i]) < 0) {
	      msSetError(MS_WFSERR, "Invalid typename (%s).", "msWFSDescribeFeatureType()", layers[i]);/* paramsObj->pszTypeName); */
              return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
      }
    }
  }

  /*
  ** retrieve any necessary external namespace/schema configuration information
  */
  namespaceList = msGMLGetNamespaces(&(map->web), "G");

  /*
  ** DescribeFeatureType response
  */
  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "encoding");
  if (value)
      msIO_printf("Content-type: text/xml; charset=%s%c%c", value,10,10);
  else
      msIO_printf("Content-type: text/xml%c%c",10,10);

  msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "FO", "encoding", OWS_NOERR,
			   "<?xml version='1.0' encoding=\"%s\" ?>\n",
			   "ISO-8859-1");

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_uri");
  if(value) user_namespace_uri = value;
  user_namespace_uri_encoded = msEncodeHTMLEntities(user_namespace_uri);

  value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_prefix");
  if(value) user_namespace_prefix = value;  
  if(user_namespace_prefix != NULL && msIsXMLTagValid(user_namespace_prefix) == MS_FALSE)
    msIO_printf("<!-- WARNING: The value '%s' is not valid XML namespace. -->\n", user_namespace_prefix);
    
  msIO_printf("<schema\n"
		"   targetNamespace=\"%s\" \n"
		"   xmlns:%s=\"%s\" \n"
		"   xmlns:ogc=\"http://www.opengis.net/ogc\"\n"
		"   xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n"
		"   xmlns=\"http://www.w3.org/2001/XMLSchema\"\n"
		"   xmlns:gml=\"http://www.opengis.net/gml\"\n",  
		user_namespace_uri_encoded, user_namespace_prefix,  user_namespace_uri_encoded);

  /* any additional namespaces */
  for(i=0; i<namespaceList->numnamespaces; i++) {
    if(namespaceList->namespaces[i].uri) {
      char *uri_encoded=NULL;

      uri_encoded = msEncodeHTMLEntities(namespaceList->namespaces[i].uri);
      msIO_printf("   xmlns:%s=\"%s\" \n", namespaceList->namespaces[i].prefix, uri_encoded);
      msFree(uri_encoded);
    }
  }

  msIO_printf("   elementFormDefault=\"qualified\" version=\"0.1\" >\n");

  encoded = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );
  if(outputformat == OWS_SFE_SCHEMA) /* reference GML 3.1.1 schema */
    msIO_printf("\n  <import namespace=\"http://www.opengis.net/gml\"\n"
		"          schemaLocation=\"%s/gml/3.1.1/base/gml.xsd\" />\n", encoded);
  else /* default GML 2.1.x schema */
    msIO_printf("\n  <import namespace=\"http://www.opengis.net/gml\"\n"
	      "          schemaLocation=\"%s/gml/2.1.2/feature.xsd\" />\n", encoded);
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
     kept here so that the behaviour with whs1.0 and gml3 output is preserved.
     We can use the wfs:FeatureCollection for wfs1.1*/
  if(outputformat == OWS_SFE_SCHEMA && strncmp(paramsObj->pszVersion,"1.1",3) != 0) {
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

    if ((numlayers == 0 || bFound) && msWFSIsLayerSupported(lp)) {

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
          char *encoded_type=NULL;

          itemList = msGMLGetItems(lp, "G"); /* GML-related metadata */
          constantList = msGMLGetConstants(lp, "G");
          groupList = msGMLGetGroups(lp, "G");
          geometryList = msGMLGetGeometries(lp, "G");

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
            msIO_printf("\n"
                      "  <element name=\"%s\" \n"
                      "           type=\"%s:%s\" \n"
                      "           substitutionGroup=\"gml:_Feature\" />\n\n",
                      encoded_name, layer_namespace_prefix, encoded_type);
            msFree(encoded_type);
          } else
            msIO_printf("\n"
                      "  <element name=\"%s\" \n"
                      "           type=\"%s:%sType\" \n"
                      "           substitutionGroup=\"gml:_Feature\" />\n\n",
                      encoded_name, layer_namespace_prefix, encoded_name);

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
** msWFSGetFeature()
*/
int msWFSGetFeature(mapObj *map, wfsParamsObj *paramsObj, cgiRequestObj *req)
  /* const char *wmtver, char **names, char **values, int numentries) */
{
  int         i, j, maxfeatures=-1;
  const char *typename="";
  char       *script_url=NULL, *script_url_encoded;
  rectObj     bbox;
  const char  *pszOutputSRS = NULL;
  
  char **layers = NULL;
  int numlayers = 0;
  
  char   *pszFilter = NULL;
  int bFilterSet = 0;
  int bBBOXSet = 0;
  int bFeatureIdSet = 0;

  char *pszNameSpace = NULL;
  const char *value;
  const char *user_namespace_prefix = "ms";
  const char *user_namespace_uri = "http://mapserver.gis.umn.edu/mapserver";
  char *user_namespace_uri_encoded = NULL;
  const char *collection_name = OWS_WFS_FEATURE_COLLECTION_NAME;
  char *encoded, *encoded_typename, *encoded_schema;
  const char *tmpmaxfeatures = NULL;

  const char *output_schema_format = "XMLSCHEMA";  
  int outputformat = OWS_GML2; /* default output is GML 2.1 */
   
  char **aFIDLayers = NULL;
  char **aFIDValues = NULL;
  int iFIDLayers = 0;
  int iNumberOfFeatures = 0;
  int iResultTypeHits = 0;
  
  gmlNamespaceListObj *namespaceList=NULL; /* for external application schema support */
  char **papszPropertyName = NULL;
  int nPropertyNames = 0;

  /*use msLayerGetShape instead of msLayerResultsGetShape of complex filter #3305*/
  int bUseGetShape = MS_FALSE;

  /* Default filter is map extents */
  bbox = map->extent;
  

  /* Read CGI parameters */
  /*  */
  /* __TODO__ Need to support XML encoded requests */
  /*  */

  if (paramsObj->pszResultType != NULL)
  {
    if (strcasecmp(paramsObj->pszResultType, "hits") == 0)
      iResultTypeHits = 1;
  }  

  /* typename is mandatory unlsess featureid is specfied. We do not
     support featureid */
  if (paramsObj->pszTypeName==NULL && paramsObj->pszFeatureId == NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: TYPENAME parameter missing", 
                 "msWFSGetFeature()");
      return msWFSException(map, "typename", "MissingParameterValue", paramsObj->pszVersion);
  }

  if(paramsObj->pszTypeName) {
    int j, k, z;
    const char *pszMapSRS = NULL;
    char **tokens;
    int n=0, i=0;
    char szTmp[256];
    const char *pszFullName = NULL;

    /* keep a ref for layer use. */
    typename = paramsObj->pszTypeName;
        
    /* Parse comma-delimited list of type names (layers) */
    /*  */
    /* __TODO__ Need to handle type grouping, e.g. "(l1,l2),l3,l4" */
    /*  */
    layers = msStringSplit(typename, ',', &numlayers);
    
    /* ==================================================================== */
    /*      TODO: check if the typename contains namespaces (ex cdf:Other), */
    /*      If that is the case extract only the layer name.                */
    /* ==================================================================== */
    
    if (layers==NULL || numlayers < 1) {
      msSetError(MS_WFSERR, "At least one type name required in TYPENAME parameter.", "msWFSGetFeature()");
      return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
    }
    
    tokens = msStringSplit(layers[0], ':', &n);
    if (tokens && n==2 && msGetLayerIndex(map, layers[0]) < 0) {
      /* pszNameSpace = strdup(tokens[0]); */
      msFreeCharArray(tokens, n);
      for (i=0; i<numlayers; i++) {
	tokens = msStringSplit(layers[i], ':', &n);
	if (tokens && n==2) {
	  free(layers[i]);
	  layers[i] = strdup(tokens[1]);
	}
	if (tokens)
	  msFreeCharArray(tokens, n);
      }
    }
    else
      msFreeCharArray(tokens, n);

   
    pszMapSRS = msOWSGetEPSGProj(&(map->projection), &(map->web.metadata), "FO", MS_TRUE);

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
	
	if (msWFSIsLayerSupported(lp) && lp->name && strcasecmp(lp->name, layers[k]) == 0) {
	  const char *pszThisLayerSRS;
          rectObj ext;
	  bLayerFound = MS_TRUE;
	  
	  lp->status = MS_ON;
	  if (lp->template == NULL) {
	    /* Force setting a template to enable query. */
	    lp->template = strdup("ttt.html");
	  }

          /* set the gml_include_items METADATA */
          if (paramsObj->pszPropertyName) 
          {
              pszPropertyName = paramsObj->pszPropertyName;

              /*we parse the propertyname parameter only once*/
              if (papszPropertyName == NULL)
              {
                  if (strlen(pszPropertyName) > 0 && pszPropertyName[0] == '(') 
                  {
                      tokens = msStringSplit(pszPropertyName+1, '(', &nPropertyNames);
                      /*expecting to always have a list of property names equal to
                        the number of layers(typename)*/
                      if (nPropertyNames != numlayers)
                      {
                          if (tokens) 
                            msFreeCharArray(tokens, nPropertyNames);
                          msSetError(MS_WFSERR, 
                                     "Optional PROPERTYNAME parameter. A list of properties may be specified for each type name. Example TYPENAME=name1&name2&PROPERTYNAME=(prop1,prop2)(prop1)", 
                                     "msWFSGetFeature()");
                          return msWFSException(map, "PROPERTYNAME", "InvalidParameterValue", paramsObj->pszVersion);
                      }
                      
                      papszPropertyName = (char **)malloc(sizeof(char *)*nPropertyNames);
                      for (i=0; i<nPropertyNames; i++) 
                      {
                          if (strlen(tokens[i]) > 0)
                          {
                              papszPropertyName[i] = strdup(tokens[i]);
                              /* remove trailing ) */
                              papszPropertyName[i][strlen(papszPropertyName[i])-1] = '\0';
                          }
                          else
                            papszPropertyName[i] = NULL; /*should return an error*/
                      }
                      if (tokens) 
                        msFreeCharArray(tokens, nPropertyNames);
                  }
                  else
                  {
                      msSetError(MS_WFSERR, 
                                     "Optional PROPERTYNAME parameter. A list of properties may be specified for each type name. Example TYPENAME=name1&name2&PROPERTYNAME=(prop1,prop2)(prop1)", 
                                     "msWFSGetFeature()");
                          return msWFSException(map, "PROPERTYNAME", "InvalidParameterValue", paramsObj->pszVersion);
                  }
              }
              /*do an alias replace for the current layer*/
              if (papszPropertyName && msLayerOpen(lp) == MS_SUCCESS && msLayerGetItems(lp) == MS_SUCCESS) 
              {
                  for(z=0; z<lp->numitems; z++) 
                  {
                      if (!lp->items[z] || strlen(lp->items[z]) <= 0)
                        continue;
                      sprintf(szTmp, "%s_alias", lp->items[z]);
                      pszFullName = msOWSLookupMetadata(&(lp->metadata), "G", szTmp);
                      if (pszFullName)
                        papszPropertyName[k] = msReplaceSubstring(papszPropertyName[i], pszFullName, lp->items[z]);
                  
                  }
              } 
                      

              if (papszPropertyName) 
              {
                  if (strlen(papszPropertyName[k]) > 0)
                  {
                      if (strcasecmp(papszPropertyName[k], "*") == 0)
                      {
                          msInsertHashTable(&(lp->metadata), "GML_INCLUDE_ITEMS", "all");
                      }
                      /*this character is only used internally and allows postrequest to 
                        have a proper property name parsing. It means do not affect what was set
                      in the map file, It is set necessary when a wfs post request is used with 
                      several query elements, with some having property names and some not*/
                      else if (strcasecmp(papszPropertyName[k], "!") == 0)
                      {
                      }
                      else
                      {
                          msInsertHashTable(&(lp->metadata), "GML_INCLUDE_ITEMS", papszPropertyName[k]);

                          /* exclude geometry if it was not asked for */
                          if (msOWSLookupMetadata(&(lp->metadata), "OFG", "geometries") != NULL) 
                            sprintf(szTmp, "%s", msOWSLookupMetadata(&(lp->metadata), "OFG", "geometries"));
                          else
                            sprintf(szTmp, OWS_GML_DEFAULT_GEOMETRY_NAME);
                  
                          if (strstr(papszPropertyName[k], szTmp) == NULL) 
                            msInsertHashTable(&(lp->metadata), "GML_GEOMETRIES", "none");
                      }
                  }
                  else /*empty string*/
                     msInsertHashTable(&(lp->metadata), "GML_GEOMETRIES", "none");

              }
          }
              

          
	  /* See comments in msWFSGetCapabilities() about the rules for SRS. */
	  if ((pszThisLayerSRS = pszMapSRS) == NULL) {
	    pszThisLayerSRS = msOWSGetEPSGProj(&(lp->projection), &(lp->metadata), "FO", MS_TRUE);
	  }
	  
	  if (pszThisLayerSRS == NULL) {
	    msSetError(MS_WFSERR, 
		       "Server config error: SRS must be set at least at the map or at the layer level.",
		       "msWFSGetFeature()");
	    return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
	  }

	  /* Keep track of output SRS.  If different from value */
	  /* from previous layers then this is an invalid request */
	  /* i.e. all layers in a single request must be in the */
	  /* same SRS. */
	  if (pszOutputSRS == NULL) {
	    pszOutputSRS = pszThisLayerSRS;
	  } else if (strcasecmp(pszThisLayerSRS,pszOutputSRS) != 0) {
	    msSetError(MS_WFSERR, 
		       "Invalid GetFeature Request: All TYPENAMES in a single GetFeature request must have been advertized in the same SRS.  Please check the capabilities and reformulate your request.",
		       "msWFSGetFeature()");
	    return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
	  }

          /* set the map extent to the layer extent */

          /* get the extent of the layer (bbox will get further filtered later */
          /* if the client specifies BBOX or a spatial filter */

          if (msOWSGetLayerExtent(map, lp, "FO", &ext) == MS_SUCCESS) {
            char szBuf[32];

            if (pszMapSRS != NULL && strncmp(pszMapSRS, "EPSG:", 5) == 0) {
              sprintf(szBuf, "init=epsg:%.10s", pszMapSRS+5);
                
              if (msLoadProjectionString(&(map->projection), szBuf) != 0) {
                 msSetError(MS_WFSERR, "msLoadProjectionString() failed: %s", "msWFSGetFeature()", szBuf);
                 return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
              }
            }

            if (msProjectionsDiffer(&map->projection, &lp->projection) == MS_TRUE) {
               msProjectRect(&lp->projection, &map->projection, &(ext));
            }

            bbox = map->extent = ext;
          }
        }
      }

      if (!bLayerFound) {
	/* Requested layer name was not in capabilities:
	 * either it just doesn't exist, or it's missing DUMP TRUE
	 */
	msSetError(MS_WFSERR, 
		   "TYPENAME '%s' doesn't exist in this server.  Please check the capabilities and reformulate your request.",
		   "msWFSGetFeature()", layers[k]);
	return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
      }
    }
  }
  if (papszPropertyName && nPropertyNames > 0)
  {
    for (i=0; i<nPropertyNames; i++)
      msFree(papszPropertyName[i]);

    msFree(papszPropertyName);
  }


  /* Validate outputformat */
  if (paramsObj->pszOutputFormat) {
    /* We support only GML2 and GML3 for now. */
    if(strcasecmp(paramsObj->pszOutputFormat, "GML2") == 0) {
      outputformat = OWS_GML2;
      output_schema_format = "XMLSCHEMA";
    } else if(strcasecmp(paramsObj->pszOutputFormat, "GML3") == 0) {
      outputformat = OWS_GML3;
      output_schema_format = "SFE_XMLSCHEMA";
    } else {
      outputformat = OWS_GML2;
      output_schema_format = "XMLSCHEMA";
    }
  }
  else
  {
      /*set the output format using the version if available*/
      if(paramsObj->pszVersion == NULL || strncmp(paramsObj->pszVersion,"1.1",3) == 0 )
      {
          outputformat = OWS_GML3;
          output_schema_format = "text/xml; subtype=gml/3.1.1";
      }
  }
  /* else if (strcasecmp(names[i], "PROPERTYNAME") == 0) */
  /* { */
  /*  */
  /* } */

  tmpmaxfeatures = msOWSLookupMetadata(&(map->web.metadata), "FO", "maxfeatures");
  if (tmpmaxfeatures)
    maxfeatures = atoi(tmpmaxfeatures);
  if (paramsObj->nMaxFeatures > 0) {
    if (maxfeatures < 0 || (maxfeatures > 0 && paramsObj->nMaxFeatures < maxfeatures))
      maxfeatures = paramsObj->nMaxFeatures;
  }

  /* if (strcasecmp(names[i], "FEATUREID") == 0) */
  /* { */
  /*  */
  /* } */
  
  if (paramsObj->pszFilter) {
    bFilterSet = 1;
    pszFilter = paramsObj->pszFilter;
  } 
  if (paramsObj->pszBbox) {
    char **tokens;
    int n;
    tokens = msStringSplit(paramsObj->pszBbox, ',', &n);
    if (tokens==NULL || n != 4) {
      msSetError(MS_WFSERR, "Wrong number of arguments for BBOX.", "msWFSGetFeature()");
      return msWFSException(map, "bbox", "InvalidParameterValue", paramsObj->pszVersion);
    }
    bbox.minx = atof(tokens[0]);
    bbox.miny = atof(tokens[1]);
    bbox.maxx = atof(tokens[2]);
    bbox.maxy = atof(tokens[3]);
    msFreeCharArray(tokens, n);
    bBBOXSet = 1;
    /* Note: BBOX SRS is implicit, it is the SRS of the selected */
    /* feature types, see pszOutputSRS in TYPENAMES above. */
  }
  if (paramsObj->pszFeatureId) {
    bFeatureIdSet = 1;
  }

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

      if (typename == NULL || strlen(typename) <= 0 || layers == NULL || numlayers <= 0) {
	msSetError(MS_WFSERR, 
                   "Required TYPENAME parameter missing for GetFeature with a FILTER parameter.", 
                   "msWFSGetFeature()");
	return msWFSException(map, "typename", "MissingParameterValue", paramsObj->pszVersion);
      }
       
      if (bBBOXSet) {
	msSetError(MS_WFSERR, 
                   "BBOX parameter and FILTER parameter are mutually exclusive in GetFeature.", 
                   "msWFSGetFeature()");
	return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
      }

      if (bFeatureIdSet) {
	msSetError(MS_WFSERR, 
                   "FEATUREID parameter and FILTER parameter are mutually exclusive in GetFeature.", 
                   "msWFSGetFeature()");
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
	
	if (tokens == NULL || nFilters <=0 || nFilters != numlayers) {
	  msSetError(MS_WFSERR, "Wrong number of FILTER attributes",
		     "msWFSGetFeature()");
	  return msWFSException(map, "filter", "InvalidParameterValue", paramsObj->pszVersion);
	}
         
	paszFilter = (char **)malloc(sizeof(char *)*nFilters);
	for (i=0; i<nFilters; i++)
	  paszFilter[i] = strdup(tokens[i]);
	
	if (tokens)
	  msFreeCharArray(tokens, nFilters);
      } else {
	nFilters=1;
	paszFilter = (char **)malloc(sizeof(char *)*nFilters);
	paszFilter[0] = pszFilter;
      }
        
      /* -------------------------------------------------------------------- */
      /*      run through the filters and build the class expressions.        */
      /*      TODO: items may have namespace prefixes, or reference aliases,  */
      /*      or groups. Need to be a bit more sophisticated here.            */
      /* -------------------------------------------------------------------- */
      for (i=0; i<nFilters; i++) {
	iLayerIndex = msGetLayerIndex(map, layers[i]);
	if (iLayerIndex < 0) {
	  msSetError(MS_WFSERR, "Invalid Typename in GetFeature : %s", "msWFSGetFeature()", layers[i]);
	  return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
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

        /* run filter.  If no results are found, do not throw exception */
        /* this is a null result */
        if( FLTApplyFilterToLayer(psNode, map, iLayerIndex, MS_FALSE) != MS_SUCCESS ) 
        {
            ms_error = msGetErrorObj();
	
            if(ms_error->code != MS_NOTFOUND)
            {
                msSetError(MS_WFSERR, "FLTApplyFilterToLayer() failed", "msWFSGetFeature()", pszFilter);
                return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
            }
        }
        
        if (bUseGetShape == MS_FALSE)
          bUseGetShape = (!FLTIsSimpleFilter(psNode));

        FLTFreeFilterEncodingNode( psNode );
        psNode = NULL;
      }

      if (paszFilter)
	free(paszFilter);
    }/* end if filter set */

    if (bFeatureIdSet)
    {
        char **tokens = NULL, **tokens1=NULL ;
        int nTokens = 0, j=0, nTokens1=0, k=0;
        FilterEncodingNode *psNode = NULL;

        /* Keep only selected layers, set to OFF by default. */
        for(j=0; j<map->numlayers; j++) 
        {
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
         if (tokens && nTokens >=1)
         {
             aFIDLayers = (char **)malloc(sizeof(char *)*nTokens);
             aFIDValues = (char **)malloc(sizeof(char *)*nTokens);
             for (j=0; j<nTokens; j++)
             {
                 aFIDLayers[j] = NULL;
                 aFIDValues[j] = NULL;
             }
             for (j=0; j<nTokens; j++)
             {
                 tokens1 = msStringSplit(tokens[j], '.', &nTokens1);
                 if (tokens1 && nTokens1 == 2)
                 {
                     for (k=0; k<iFIDLayers; k++)
                     {
                         if (strcasecmp(aFIDLayers[k], tokens1[0]) == 0)
                           break;
                     }
                     if (k == iFIDLayers)
                     {
                         aFIDLayers[iFIDLayers] = strdup(tokens1[0]);
                         iFIDLayers++;
                     }
                     if (aFIDValues[k] != NULL)
                       aFIDValues[k] = msStringConcatenate(aFIDValues[k], ",");
                     aFIDValues[k] = msStringConcatenate( aFIDValues[k], tokens1[1]);
                 }
                 else
                 {
                     msSetError(MS_WFSERR, 
		     "Invalid FeatureId in GetFeature. Expecting layername.value : %s", 
		     "msWFSGetFeature()", pszFilter);
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

         for (j=0; j< iFIDLayers; j++)
         {
             for (k=0; k<map->numlayers; k++) 
             {
                 layerObj *lp;
                 lp = GET_LAYER(map, k);
                 if (msWFSIsLayerSupported(lp) && lp->name && 
                     strcasecmp(lp->name, aFIDLayers[j]) == 0)
                 {
                     lp->status = MS_ON;
                     if (lp->template == NULL) {
                         /* Force setting a template to enable query. */
                         lp->template = strdup("ttt.html");
                     }
                     psNode = FLTCreateFeatureIdFilterEncoding(aFIDValues[j]);

                     if( FLTApplyFilterToLayer(psNode, map, lp->index, MS_FALSE) != MS_SUCCESS ) {
                       msSetError(MS_WFSERR, "FLTApplyFilterToLayer() failed", "msWFSGetFeature");
                       return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
                     }

                     if (bUseGetShape == MS_FALSE)
                       bUseGetShape = (!FLTIsSimpleFilter(psNode));

                     FLTFreeFilterEncodingNode( psNode );
                     psNode = NULL;
                     break;
                 }
             }
             if (k == map->numlayers)/*layer not found*/
             {
                 msSetError(MS_WFSERR, 
                            "Invalid typename given with FeatureId in GetFeature : %s", "msWFSGetFeature()", 
                            aFIDLayers[j]);

                 if (aFIDLayers && aFIDValues)
                 {      
                     for (j=0; j<iFIDLayers; j++)
                     {
                         msFree(aFIDLayers[j]);
                         msFree(aFIDValues[j]);
                     }
                     msFree(aFIDLayers);
                     msFree(aFIDValues);
                 }
                 return msWFSException(map, "featureid", "InvalidParameterValue", paramsObj->pszVersion);
             }
         }

         if (aFIDLayers && aFIDValues)
         {
             for (j=0; j<iFIDLayers; j++)
             {
                 msFree(aFIDLayers[j]);
                 msFree(aFIDValues[j]);
             }
             msFree(aFIDLayers);
             msFree(aFIDValues);
         }
    }
#endif

    if(layers)
      msFreeCharArray(layers, numlayers);


    /*
      srs is defined for wfs 1.1. If it is available. If it used to set 
      the map projection. For EPSG codes between 4000 and 5000 
      that coordinates order follow what is defined  in the ESPG database
      see #2899
  */
    if (strncmp(paramsObj->pszVersion,"1.1",3) == 0 && paramsObj->pszSrs && pszOutputSRS)
    {     
          /*check if the srs passed is valid. Assuming that it is an EPSG:xxx format,
            Or urn:ogc:def:crs:EPSG:xxx format. */
        if (strncasecmp(paramsObj->pszSrs, "EPSG:", 5) == 0)
        {
            if (strcasecmp(paramsObj->pszSrs, pszOutputSRS) != 0)
            {
                msSetError(MS_WFSERR, 
                           "Invalid GetFeature Request: SRSNAME value should be valid for all the TYPENAMES. Please check the capabilities and reformulate your request.",
                           "msWFSGetFeature()");
                return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
            }
                /*we load the projection sting in the map and possibly 
                  set the axis order*/
            msFreeProjection(&map->projection);
            msLoadProjectionStringEPSG(&(map->projection), paramsObj->pszSrs);
        }
        else if (strncasecmp(paramsObj->pszSrs, "urn:EPSG:geographicCRS:",23) == 0)
        {
            char epsg_string[100];
            const char *code;

            code = paramsObj->pszSrs + 23;
            
            sprintf( epsg_string, "EPSG:%s", code );
            if (strcasecmp(epsg_string, pszOutputSRS) != 0)
            {
                msSetError(MS_WFSERR, 
                           "Invalid GetFeature Request: SRSNAME value should be valid for all the TYPENAMES. Please check the capabilities and reformulate your request.",
                           "msWFSGetFeature()");
                return msWFSException(map, "typename", "InvalidParameterValue", paramsObj->pszVersion);
            }
                /*we load the projection sting in the map and possibly 
                  set the axis order*/
            msFreeProjection(&map->projection);
            msLoadProjectionStringEPSG(&(map->projection), epsg_string);
        }
    }
    /* Set map output projection to which output features should be reprojected */
    else if (pszOutputSRS && strncasecmp(pszOutputSRS, "EPSG:", 5) == 0) 
    {
        int nTmp =0;
        msFreeProjection(&map->projection);
        msInitProjection(&map->projection);

        if (strncmp(paramsObj->pszVersion,"1.1",3) == 0)
          nTmp = msLoadProjectionStringEPSG(&(map->projection), pszOutputSRS);
        else
          nTmp = msLoadProjectionString(&(map->projection), pszOutputSRS);

        if (nTmp != 0) {
            msSetError(MS_WFSERR, "msLoadProjectionString() failed", "msWFSGetFeature()");
            return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
        }
    }

    /*
    ** Perform Query (only BBOX for now)
    */
    /* __TODO__ Using a rectangle query may not be the most efficient way */
    /* to do things here. */
    if (!bFilterSet) {

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

    /* if no results where written (TODO: this needs to be GML2/3 specific I imagine */
    for(j=0; j<map->numlayers; j++) {
      if (GET_LAYER(map, j)->resultcache && GET_LAYER(map, j)->resultcache->numresults > 0)
      {
        iNumberOfFeatures += GET_LAYER(map, j)->resultcache->numresults;
      }
    }

    /*
    ** GetFeature response
    */

    if ((script_url=msOWSGetOnlineResource(map,"FO","onlineresource",req)) ==NULL ||
        (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL) {
      msSetError(MS_WFSERR, "Server URL not found", "msWFSGetFeature()");
      return msWFSException(map, "mapserv", "NoApplicableCode", paramsObj->pszVersion);
    }

    /*
    ** retrieve any necessary external namespace/schema configuration information
    */
    namespaceList = msGMLGetNamespaces(&(map->web), "G");

    value = msOWSLookupMetadata(&(map->web.metadata), "FO", "encoding");
    if (value)
        msIO_printf("Content-type: text/xml; charset=%s%c%c", value,10,10);
    else
        msIO_printf("Content-type: text/xml%c%c",10,10);

    msOWSPrintEncodeMetadata(stdout, &(map->web.metadata), "FO", 
                             "encoding", OWS_NOERR,
                             "<?xml version='1.0' encoding=\"%s\" ?>\n",
                             "ISO-8859-1");

    value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_uri");
    if(value) user_namespace_uri = value;
    user_namespace_uri_encoded = msEncodeHTMLEntities(user_namespace_uri);

    value = msOWSLookupMetadata(&(map->web.metadata), "FO", "namespace_prefix");
    if(value) user_namespace_prefix = value;

    if(user_namespace_prefix != NULL && msIsXMLTagValid(user_namespace_prefix) == MS_FALSE)
      msIO_printf("<!-- WARNING: The value '%s' is not valid XML namespace. -->\n", user_namespace_prefix);

    pszNameSpace = strdup(user_namespace_prefix);

    value = msOWSLookupMetadata(&(map->web.metadata), "FO", "feature_collection");
    if(value) collection_name = value;

    encoded = msEncodeHTMLEntities( paramsObj->pszVersion );
    encoded_typename = msEncodeHTMLEntities( typename );
    encoded_schema = msEncodeHTMLEntities( msOWSGetSchemasLocation(map) );
    if(outputformat == OWS_GML2) { /* use a wfs:FeatureCollection */
      msIO_printf("<wfs:FeatureCollection\n"
		  "   xmlns:%s=\"%s\"\n"  
		  "   xmlns:wfs=\"http://www.opengis.net/wfs\"\n"
		  "   xmlns:gml=\"http://www.opengis.net/gml\"\n"
		  "   xmlns:ogc=\"http://www.opengis.net/ogc\"\n"
      "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n",
      user_namespace_prefix, user_namespace_uri_encoded);

      /* any additional namespaces */
      for(i=0; i<namespaceList->numnamespaces; i++) {
        if(namespaceList->namespaces[i].uri) {
          char *uri_encoded=NULL;

          uri_encoded = msEncodeHTMLEntities(namespaceList->namespaces[i].uri);
          msIO_printf("   xmlns:%s=\"%s\" \n", namespaceList->namespaces[i].prefix, uri_encoded);
          msFree(uri_encoded);
        }
      }


		  msIO_printf("   xsi:schemaLocation=\"http://www.opengis.net/wfs %s/wfs/%s/WFS-basic.xsd \n"
		  "                       %s %sSERVICE=WFS&amp;VERSION=%s&amp;REQUEST=DescribeFeatureType&amp;TYPENAME=%s&amp;OUTPUTFORMAT=%s\">\n",
		  encoded_schema, encoded, user_namespace_uri_encoded, 
		  script_url_encoded, encoded, encoded_typename, output_schema_format);
    } 
    else 
    {
      if(paramsObj->pszVersion && strncmp(paramsObj->pszVersion,"1.1",3) == 0 )
      {
      msIO_printf("<wfs:FeatureCollection\n"
		  "   xmlns:%s=\"%s\"\n"  
		  "   xmlns:gml=\"http://www.opengis.net/gml\"\n"
		  "   xmlns:wfs=\"http://www.opengis.net/wfs\"\n"
		  "   xmlns:ogc=\"http://www.opengis.net/ogc\"\n"
      "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n",
                  user_namespace_prefix, user_namespace_uri_encoded);
      }
      else
      {
         msIO_printf("<%s:%s\n"
      "   version=\"1.0.0\"\n"
		  "   xmlns:%s=\"%s\"\n"  
		  "   xmlns:gml=\"http://www.opengis.net/gml\"\n"
		  "   xmlns:ogc=\"http://www.opengis.net/ogc\"\n"
      "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n",
      user_namespace_prefix, collection_name, user_namespace_prefix, user_namespace_uri_encoded);
      }
      /* any additional namespaces */
      for(i=0; i<namespaceList->numnamespaces; i++) {
        if(namespaceList->namespaces[i].uri) {
          char *uri_encoded=NULL;

          uri_encoded = msEncodeHTMLEntities(namespaceList->namespaces[i].uri);
          msIO_printf("   xmlns:%s=\"%s\" \n", namespaceList->namespaces[i].prefix, uri_encoded);
          msFree(uri_encoded);
        }
      }

     if(maxfeatures > 0 && maxfeatures < iNumberOfFeatures) {
         iNumberOfFeatures = maxfeatures;
     }

      if(paramsObj->pszVersion && strncmp(paramsObj->pszVersion,"1.1",3) == 0 )
        msIO_printf("   xsi:schemaLocation=\"%s %sSERVICE=WFS&amp;VERSION=%s&amp;REQUEST=DescribeFeatureType&amp;TYPENAME=%s&amp;OUTPUTFORMAT=%s  http://www.opengis.net/wfs http://schemas.opengis.net/wfs/1.1.0/wfs.xsd\" numberOfFeatures=\"%d\">\n",
		  user_namespace_uri_encoded, script_url_encoded, encoded, encoded_typename, output_schema_format, iNumberOfFeatures);
      else
        msIO_printf("   xsi:schemaLocation=\"%s %sSERVICE=WFS&amp;VERSION=%s&amp;REQUEST=DescribeFeatureType&amp;TYPENAME=%s&amp;OUTPUTFORMAT=%s\">\n",
		  user_namespace_uri_encoded, script_url_encoded, encoded, encoded_typename, output_schema_format);
    }

    msFree(encoded);
    msFree(encoded_schema);
    msFree(encoded_typename);

    /* handle case of maxfeatures = 0 */
    if(maxfeatures != 0 && iResultTypeHits == 0)
      msGMLWriteWFSQuery(map, stdout, maxfeatures, pszNameSpace, outputformat,bUseGetShape);

    if (((iNumberOfFeatures==0) || (maxfeatures == 0)) && iResultTypeHits == 0) {
      msIO_printf("   <gml:boundedBy>\n"); 
      msIO_printf("      <gml:null>missing</gml:null>\n");
      msIO_printf("   </gml:boundedBy>\n"); 
    }

    if(outputformat == OWS_GML2)
      msIO_printf("</wfs:FeatureCollection>\n\n");
    else
    {
      if(paramsObj->pszVersion && strncmp(paramsObj->pszVersion,"1.1",3) == 0)
        msIO_printf("</wfs:FeatureCollection>\n\n", user_namespace_prefix, collection_name);
      else
        msIO_printf("</%s:%s>\n\n", user_namespace_prefix, collection_name);
    }

    /*
    ** Done! Now a bit of clean-up.
    */    

    free(script_url);
    free(script_url_encoded);
    if (pszNameSpace)
      free(pszNameSpace);
    msFree(user_namespace_uri_encoded);
    msGMLFreeNamespaces(namespaceList);

    return MS_SUCCESS;
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
int msWFSDispatch(mapObj *map, cgiRequestObj *requestobj)
{
#ifdef USE_WFS_SVR
  int status;
  int returnvalue = MS_DONE;

  /* static char *wmtver = NULL, *request=NULL, *service=NULL; */
  wfsParamsObj *paramsObj;
  /*
  ** Populate the Params object based on the request
  */
  paramsObj = msWFSCreateParamsObj();
  /* TODO : store also parameters that are inside the map object */
  /* into the paramsObj.  */
  msWFSParseRequest(requestobj, paramsObj);

  /* If SERVICE is specified then it MUST be "WFS" */
  if (paramsObj->pszService != NULL && 
      strcasecmp(paramsObj->pszService, "WFS") != 0)
  {
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return MS_DONE;  /* Not a WFS request */
  }


  /* If SERVICE, VERSION and REQUEST not included than this isn't a WFS req*/
  if (paramsObj->pszService == NULL && paramsObj->pszVersion==NULL && 
      paramsObj->pszRequest==NULL)
  {
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return MS_DONE;  /* Not a WFS request */
  }

  /* VERSION *and* REQUEST *and* SERVICE required by all WFS requests including 
   * GetCapabilities.
   */
  if (paramsObj->pszVersion==NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: VERSION parameter missing", 
                 "msWFSDispatch()");
       
      returnvalue = msWFSException11(map, "version", "MissingParameterValue", "1.1.0");
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
  }

  if (paramsObj->pszRequest==NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: REQUEST parameter missing", 
                 "msWFSDispatch()");
      returnvalue = msWFSException(map, "request", "MissingParameterValue", paramsObj->pszVersion);
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
  }

  if (paramsObj->pszService==NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: SERVICE parameter missing", 
                 "msWFSDispatch()");
       
      returnvalue = msWFSException(map, "service", "MissingParameterValue", paramsObj->pszVersion);
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
  }

  if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
  {
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
  if (strcasecmp(paramsObj->pszRequest, "GetCapabilities") == 0 ) 
  {
      returnvalue = msWFSGetCapabilities(map, paramsObj, requestobj);
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
      strcmp(paramsObj->pszVersion, "1.1.0") != 0)
  {
      msSetError(MS_WFSERR, 
                 "WFS Server does not support VERSION %s.", 
                 "msWFSDispatch()", paramsObj->pszVersion);
      returnvalue = msWFSException11(map, "version", "InvalidParameterValue","1.1.0");
      msWFSFreeParamsObj(paramsObj);
      free(paramsObj);
      paramsObj = NULL;
      return returnvalue;
                                                                 
  }

  returnvalue = MS_DONE;
  /* Continue dispatching... 
   */
  if (strcasecmp(paramsObj->pszRequest, "DescribeFeatureType") == 0)
    returnvalue = msWFSDescribeFeatureType(map, paramsObj);

  else if (strcasecmp(paramsObj->pszRequest, "GetFeature") == 0)
    returnvalue = msWFSGetFeature(map, paramsObj, requestobj);


  else if (strcasecmp(paramsObj->pszRequest, "GetFeatureWithLock") == 0 ||
           strcasecmp(paramsObj->pszRequest, "LockFeature") == 0 ||
           strcasecmp(paramsObj->pszRequest, "Transaction") == 0 )
  {
      /* Unsupported WFS request */
      msSetError(MS_WFSERR, "Unsupported WFS request: %s", "msWFSDispatch()",
                 paramsObj->pszRequest);
      returnvalue = msWFSException(map, "request", "InvalidParameterValue", paramsObj->pszVersion);
  }
  else if (strcasecmp(paramsObj->pszService, "WFS") == 0)
  {
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
    if (paramsObj)
    {
        paramsObj->nMaxFeatures = -1;
    }

    return paramsObj;
}



/************************************************************************/
/*                            msWFSFreeParmsObj                         */
/*                                                                      */
/*      Free params object.                                             */
/************************************************************************/
void msWFSFreeParamsObj(wfsParamsObj *wfsparams)
{
    if (wfsparams)
    {
        if (wfsparams->pszVersion)
          free(wfsparams->pszVersion);
        if (wfsparams->pszUpdateSequence)
          free(wfsparams->pszUpdateSequence);
        if (wfsparams->pszRequest)
          free(wfsparams->pszRequest);
        if (wfsparams->pszService)
          free(wfsparams->pszService);
        if (wfsparams->pszTypeName)
          free(wfsparams->pszTypeName);
        if (wfsparams->pszFilter)
          free(wfsparams->pszFilter);
        if (wfsparams->pszFeatureId)
          free(wfsparams->pszFeatureId);
        if (wfsparams->pszOutputFormat)
          free(wfsparams->pszOutputFormat);
        if (wfsparams->pszSrs)
          free(wfsparams->pszSrs);
        if (wfsparams->pszResultType)
          free(wfsparams->pszResultType);
    }
}

/************************************************************************/
/*                            msWFSParseRequest                         */
/*                                                                      */
/*      Parse request into the params object.                           */
/************************************************************************/
void msWFSParseRequest(cgiRequestObj *request, wfsParamsObj *wfsparams)
{
#ifdef USE_WFS_SVR
    int i = 0;
        
    if (!request || !wfsparams)
      return;
    
    if (request->NumParams > 0)
    {
        for(i=0; i<request->NumParams; i++) 
        {
            if (request->ParamNames[i] && request->ParamValues[i])
            {
                if (strcasecmp(request->ParamNames[i], "VERSION") == 0)
                  wfsparams->pszVersion = strdup(request->ParamValues[i]);

                else if (strcasecmp(request->ParamNames[i], "UPDATESEQUENCE") == 0)
                  wfsparams->pszUpdateSequence = strdup(request->ParamValues[i]);

                else if (strcasecmp(request->ParamNames[i], "REQUEST") == 0)
                  wfsparams->pszRequest = strdup(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "SERVICE") == 0)
                  wfsparams->pszService = strdup(request->ParamValues[i]);
             
                else if (strcasecmp(request->ParamNames[i], "MAXFEATURES") == 0)
                  wfsparams->nMaxFeatures = atoi(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "BBOX") == 0)
                  wfsparams->pszBbox = strdup(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "SRSNAME") == 0)
                  wfsparams->pszSrs = strdup(request->ParamValues[i]);

                else if (strcasecmp(request->ParamNames[i], "RESULTTYPE") == 0)
                  wfsparams->pszResultType = strdup(request->ParamValues[i]);

                else if (strcasecmp(request->ParamNames[i], "TYPENAME") == 0)
                  wfsparams->pszTypeName = strdup(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "FILTER") == 0)
                  wfsparams->pszFilter = strdup(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "OUTPUTFORMAT") == 0)
                  wfsparams->pszOutputFormat = strdup(request->ParamValues[i]);

                 else if (strcasecmp(request->ParamNames[i], "FEATUREID") == 0)
                  wfsparams->pszFeatureId = strdup(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "PROPERTYNAME") == 0)
                  wfsparams->pszPropertyName = strdup(request->ParamValues[i]);
            }
        }
        /* version is optional is the GetCapabilities. If not */
        /* provided, set it. Default it to 1.1.0*/
        if (wfsparams->pszVersion == NULL &&
            wfsparams->pszRequest && 
            strcasecmp(wfsparams->pszRequest, "GetCapabilities") == 0)
          wfsparams->pszVersion = strdup("1.1.0");
    }
/* -------------------------------------------------------------------- */
/*      Parse the post request. It is assumed to be an XML document.    */
/* -------------------------------------------------------------------- */
#ifdef USE_OGR
    if (request->postrequest)
    {
        CPLXMLNode *psRoot, *psQuery, *psFilter, *psTypeName,  *psPropertyName  = NULL;
        CPLXMLNode *psGetFeature = NULL;
        CPLXMLNode  *psGetCapabilities = NULL;
        CPLXMLNode *psDescribeFeature = NULL;
        CPLXMLNode *psOperation = NULL;
        char *pszValue, *pszSerializedFilter, *pszTmp, *pszTmp2 = NULL;
        int bMultiLayer = 0;

        psRoot = CPLParseXMLString(request->postrequest);

        if (psRoot)
        {
            /* need to strip namespaces */
            CPLStripXMLNamespace(psRoot, NULL, 1);
            for( psOperation = psRoot; 
                 psOperation != NULL; 
                 psOperation = psOperation->psNext )
            {
                if(psOperation->eType == CXT_Element)
                {
                    if(strcasecmp(psOperation->pszValue,"GetFeature")==0)
                    {
                        psGetFeature = psOperation;
                        break;
                    }
                    else if(strcasecmp(psOperation->pszValue,"GetCapabilities")==0)
                    {
                        psGetCapabilities = psOperation;
                        pszValue = (char*)CPLGetXMLValue(psGetFeature,  "updateSequence", NULL);

                        if (pszValue)
                            wfsparams->pszUpdateSequence = strdup(pszValue);

                        break;
                    }
                    else if(strcasecmp(psOperation->pszValue,
                                       "DescribeFeatureType")==0)
                    {
                        psDescribeFeature = psOperation;
                        break;
                    }
                    /* these are unsupported requests. Just set the  */
                    /* request value and return; */
                    else if (strcasecmp(psOperation->pszValue, 
                                        "GetFeatureWithLock") == 0)
                    {
                        wfsparams->pszRequest = strdup("GetFeatureWithLock");
                         break;
                    }
                    else if (strcasecmp(psOperation->pszValue, 
                                        "LockFeature") == 0)
                    {
                        wfsparams->pszRequest = strdup("LockFeature");
                         break;
                    }
                    else if (strcasecmp(psOperation->pszValue, 
                                        "Transaction") == 0)
                    {
                        wfsparams->pszRequest = strdup("Transaction");
                        break;
                    }
                }
            }

/* -------------------------------------------------------------------- */
/*      Parse the GetFeature                                            */
/* -------------------------------------------------------------------- */
            if (psGetFeature)
            {
                wfsparams->pszRequest = strdup("GetFeature");
            
                pszValue = (char*)CPLGetXMLValue(psGetFeature,  "version", 
                                                 NULL);
                if (pszValue)
                  wfsparams->pszVersion = strdup(pszValue);
                
                pszValue = (char*)CPLGetXMLValue(psGetFeature,  "service", 
                                                 NULL);
                if (pszValue)
                  wfsparams->pszService = strdup(pszValue);

                pszValue = (char*)CPLGetXMLValue(psGetFeature,  "resultType",
                                                 NULL);
                if (pszValue)
                  wfsparams->pszResultType = strdup(pszValue);

                pszValue = (char*)CPLGetXMLValue(psGetFeature,  "maxFeatures", 
                                                 NULL);
                if (pszValue)
                  wfsparams->nMaxFeatures = atoi(pszValue);

                psQuery = CPLGetXMLNode(psGetFeature, "Query");
                if (psQuery)
                {
                   
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
/*      typenames will be build with comma between them                 */
/*      (typename1,typename2,...) and filters will be build with        */
/*      brackets enclosing the filters :(filter1)(filter2)...            */
/*      propertynames are stored like (property1,property2)(property1)  */
/* -------------------------------------------------------------------- */
                    while (psQuery &&  psQuery->pszValue && 
                           strcasecmp(psQuery->pszValue, "Query") == 0)
                    {
                        /* get SRS */
                        pszValue = (char*)CPLGetXMLValue(psGetFeature,  "srsName",
                                                 NULL);
                        if (pszValue)
                          wfsparams->pszSrs = strdup(pszValue);

                        /* parse typenames */
                        pszValue = (char*)CPLGetXMLValue(psQuery,  
                                                         "typename", NULL);
                        if (pszValue)
                        {
                            if (wfsparams->pszTypeName == NULL)
                              wfsparams->pszTypeName = strdup(pszValue);
                            else
                            {
                                pszTmp = strdup(wfsparams->pszTypeName);
                                wfsparams->pszTypeName = 
                                  (char *)realloc(wfsparams->pszTypeName,
                                                  sizeof(char)*
                                                  (strlen(pszTmp)+
                                                   strlen(pszValue)+2));
                                
                                sprintf(wfsparams->pszTypeName,"%s,%s",pszTmp, 
                                        pszValue);
                                free(pszTmp);
                            }
                        }
                        
                        /*parse property name*/
                        psPropertyName = CPLGetXMLNode(psQuery, "PropertyName");
                        pszTmp2= NULL;

                        /*when there is no PropertyName, we outpout (!) so that it is parsed properly in GetFeature*/
                        if (psPropertyName == NULL)
                          pszTmp2 = strdup("!");

                        while (psPropertyName)
                        {
                            if (!psPropertyName->pszValue || 
                                strcasecmp(psPropertyName->pszValue, "PropertyName") != 0)
                            {
                                psPropertyName = psPropertyName->psNext;
                                continue;
                            }
                            pszValue = (char *) CPLGetXMLValue(psPropertyName, NULL, NULL);
                            if (pszTmp2 == NULL) {
                                pszTmp2 = strdup(pszValue);
                            } 
                            else 
                            {
                                pszTmp = strdup(pszTmp2);
                                pszTmp2 = (char *)realloc(pszTmp2, sizeof(char)* (strlen(pszTmp)+ strlen(pszValue)+2));
                                sprintf(pszTmp2,"%s,%s",pszTmp, pszValue);
                                msFree(pszTmp);
                            }
                            psPropertyName = psPropertyName->psNext;
                        }
                        if (pszTmp2)
                        {
                            pszTmp = strdup(pszTmp2);
                            pszTmp2 = (char *)realloc(pszTmp2, sizeof(char)* (strlen(pszTmp)+ 3));
                            sprintf(pszTmp2,"(%s)", pszTmp);
                            msFree(pszTmp);

                            if (wfsparams->pszPropertyName == NULL)
                              wfsparams->pszPropertyName = strdup(pszTmp2);
                            else
                            {
                                pszTmp = strdup(wfsparams->pszPropertyName);
                                wfsparams->pszPropertyName =
                                  (char *)realloc(wfsparams->pszPropertyName,
                                                  sizeof(char)*(strlen(pszTmp)+ strlen(pszTmp2)+1));
                                sprintf(wfsparams->pszPropertyName,"%s%s",wfsparams->pszPropertyName,
                                        pszTmp2);
                                msFree(pszTmp);
                            }
                            msFree(pszTmp2);
                            pszTmp2 = NULL;
                        }
                            
                        /* parse filter */
                        psFilter = CPLGetXMLNode(psQuery, "Filter");

                        if (psFilter)
                        {
                            if (!bMultiLayer)
                              wfsparams->pszFilter = CPLSerializeXMLTree(psFilter);
                            else
                            {
                                pszSerializedFilter = CPLSerializeXMLTree(psFilter);
                                pszTmp = (char *)malloc(sizeof(char)*
                                                        (strlen(pszSerializedFilter)+3));
                                sprintf(pszTmp, "(%s)", pszSerializedFilter);
                                free(pszSerializedFilter);

                                if (wfsparams->pszFilter == NULL)
                                  wfsparams->pszFilter = strdup(pszTmp);
                                else
                                {
                                    pszSerializedFilter = strdup(wfsparams->pszFilter);
                                    wfsparams->pszFilter = 
                                      (char *)realloc(wfsparams->pszFilter,
                                                      sizeof(char)*
                                                      (strlen(pszSerializedFilter)+
                                                       strlen(pszTmp)+1));
                                    sprintf(wfsparams->pszFilter, "%s%s",
                                            pszSerializedFilter, pszTmp);

                                    free(pszSerializedFilter);
                                }
                                free(pszTmp);
                            }
                        }
                    
                        psQuery = psQuery->psNext;
                    }/* while psQuery */
                }
            }/* end of GetFeatures */

/* -------------------------------------------------------------------- */
/*      Parse GetCapabilities.                                          */
/* -------------------------------------------------------------------- */
            if (psGetCapabilities)
            {
                wfsparams->pszRequest = strdup("GetCapabilities");
                pszValue = (char*)CPLGetXMLValue(psGetCapabilities,  "version", 
                                                 NULL);
                /* version is optional is the GetCapabilities. If not */
                /* provided, set it. */
                if (pszValue)
                  wfsparams->pszVersion = strdup(pszValue);
                else
                  wfsparams->pszVersion = strdup("1.0.0");

                 pszValue = 
                   (char*)CPLGetXMLValue(psGetCapabilities, "service", 
                                         NULL);
                if (pszValue)
                  wfsparams->pszService = strdup(pszValue);
            }/* end of GetCapabilites */
/* -------------------------------------------------------------------- */
/*      Parse DescribeFeatureType                                       */
/* -------------------------------------------------------------------- */
            if (psDescribeFeature)
            {
                wfsparams->pszRequest = strdup("DescribeFeatureType");

                pszValue = (char*)CPLGetXMLValue(psDescribeFeature, "version", 
                                                 NULL);
                if (pszValue)
                  wfsparams->pszVersion = strdup(pszValue);

                pszValue = (char*)CPLGetXMLValue(psDescribeFeature, "service", 
                                                 NULL);
                if (pszValue)
                  wfsparams->pszService = strdup(pszValue);

                pszValue = (char*)CPLGetXMLValue(psDescribeFeature, 
                                                 "outputFormat", 
                                                 NULL);
                if (pszValue)
                  wfsparams->pszOutputFormat = strdup(pszValue);

                psTypeName = CPLGetXMLNode(psDescribeFeature, "TypeName");
                if (psTypeName)
                {
                    /* free typname and filter. There may have been */
                    /* values if they were passed in the URL */
                    if (wfsparams->pszTypeName)    
                      free(wfsparams->pszTypeName);
                    wfsparams->pszTypeName = NULL;

                    while (psTypeName && psTypeName->pszValue &&
                           strcasecmp(psTypeName->pszValue, "TypeName") == 0)
                    {
                        if (psTypeName->psChild && psTypeName->psChild->pszValue)
                        {
                            pszValue = psTypeName->psChild->pszValue;
                            if (wfsparams->pszTypeName == NULL)
                              wfsparams->pszTypeName = strdup(pszValue);
                            else
                            {
                                pszTmp = strdup(wfsparams->pszTypeName);
                                wfsparams->pszTypeName = 
                                  (char *)realloc(wfsparams->pszTypeName,
                                                  sizeof(char)*
                                                  (strlen(pszTmp)+
                                                   strlen(pszValue)+2));
                                
                                sprintf(wfsparams->pszTypeName,"%s,%s",pszTmp, 
                                        pszValue);
                                free(pszTmp);
                            }
                        }
                        psTypeName = psTypeName->psNext;
                    }
                }

            }/* end of DescibeFeatureType */
        }
    }
#endif
#endif
}


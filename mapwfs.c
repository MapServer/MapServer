/**********************************************************************
 * $Id$
 *
 * Name:     mapwfs.c
 * Project:  MapServer
 * Language: C
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
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 * $Log$
 * Revision 1.6  2002/10/28 15:26:42  dan
 * Fixed typo in DescribeFeatureType schemaLocation
 *
 * Revision 1.5  2002/10/09 02:29:03  dan
 * Initial implementation of WFS client support.
 *
 * Revision 1.4  2002/10/08 05:05:14  dan
 * Encode HTML entities in schema url in GML output
 *
 * Revision 1.3  2002/10/08 02:40:08  dan
 * Added WFS DescribeFeatureType
 *
 * Revision 1.2  2002/10/04 21:29:41  dan
 * WFS: Added GetCapabilities and basic GetFeature (still some work to do)
 *
 * Revision 1.1  2002/09/03 03:19:51  dan
 * Set the bases for WFS Server support + moved some WMS/WFS stuff to mapows.c
 *
 **********************************************************************/

#include "map.h"

#if defined(USE_WFS_SVR)

/* There is a dependency to GDAL/OGR for the GML driver and MiniXML parser */
#include "cpl_minixml.h"


/*
** msWFSException()
**
** Report current MapServer error in XML exception format.
*/

static int msWFSException(mapObj *map, const char *wmtversion) 
{
    /* In WFS, exceptions are always XML.
    */
    printf("Content-type: text/xml%c%c",10,10);

    printf("<WFS_Exception>\n");
    printf("  <Exception>\n");
    /* Optional <Locator> element currently unused. */
    printf("    <Message>\n");
    msWriteError(stdout);
    printf("    </Message>\n");
    printf("  </Exception>\n");
    printf("</WFS_Exception>\n");

    return MS_FAILURE; // so we can call 'return msWFSException();' anywhere
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

  printf("    <%s>\n", request);

  // We expect to receive a NULL-terminated args list of formats
  if (format_tag != NULL)
  {
    printf("      <%s>\n", format_tag);
    va_start(argp, formats);
    fmt = formats;
    while(fmt != NULL)
    {
      printf("        <%s/>\n", fmt);
      fmt = va_arg(argp, const char *);
    }
    va_end(argp);
    printf("      </%s>\n", format_tag);
  }

  printf("      <DCPType>\n");
  printf("        <HTTP>\n");
  printf("          <Get onlineResource=\"%s\" />\n", script_url);
  printf("        </HTTP>\n");
  printf("      </DCPType>\n");

#ifdef __TODO__
// POST method with XML encoding not yet supported.
  printf("      <DCPType>\n");
  printf("        <HTTP>\n");
  printf("          <Post onlineResource=\"%s\" />\n", script_url);
  printf("        </HTTP>\n");
  printf("      </DCPType>\n");
#endif

  printf("    </%s>\n", request);
}

/* msWFSIsLayerSupported()
**
** Returns true (1) is this layer meets the requirements to be served as
** a WFS feature type.
*/
static int msWFSIsLayerSupported(layerObj *lp)
{
    /* In order to be supported, lp->type must be specified, even for 
    ** layers that are OGR, SDE, SDO, etc connections.
    ** lp->dump must be explicitly set to TRUE in mapfile.
    */
    if (lp->dump && 
        (lp->type == MS_LAYER_POINT ||
         lp->type == MS_LAYER_LINE ||
         lp->type == MS_LAYER_POLYGON ) &&
        lp->connectiontype != MS_WMS )
    {
        return 1; /* true */
    }

    return 0; /* false */
}


/* msWFSGetSchemasLocation()
**
** schemas location is the root of the web tree where all WFS-related 
** schemas can be found on this server.  These URLs must exist in order 
** to validate xml.
**
** Use value of "wfs_schemas_location", otherwise return ".."
*/
static const char *msWFSGetSchemasLocation(mapObj *map)
{
    const char *schemas_location;

    schemas_location = msLookupHashTable(map->web.metadata, 
                                         "wfs_schemas_location");
    if (schemas_location == NULL)
        schemas_location = "..";

    return schemas_location;
}

/* msWFSGetGeomElementName()
**
** Return the Element name for the geometry in this layer, look for
** "wfs_geometry_element_name" metadata in layer or in map, or default 
** to "MS_GEOMETRY"
*/
const char *msWFSGetGeomElementName(mapObj *map, layerObj *lp)
{
    const char *name;

    if ((name = msLookupHashTable(lp->metadata, 
                                  "wfs_geometry_element_name")) == NULL &&
        (name = msLookupHashTable(map->web.metadata, 
                                  "wfs_geometry_element_name")) == NULL )
    {
        name = "MS_GEOMETRY";
    }

    return name;

}

/* msWFSGetGeomType()
**
** Return GML name for geometry type in this layer
** This is based on MapServer geometry type and layers with mixed geometries
** may not return the right feature type.
*/
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

/*
** msWFSDumpLayer()
*/
int msWFSDumpLayer(mapObj *map, layerObj *lp)
{
   rectObj ext;
   
   printf("    <FeatureType>\n");

   msOWSPrintParam("LAYER.NAME", lp->name, OWS_WARN, 
              "        <Name>%s</Name>\n", NULL);

   msOWSPrintMetadata(lp->metadata, "wfs_title", OWS_WARN,
                 "        <Title>%s</Title>\n", lp->name);

   msOWSPrintMetadata(lp->metadata, "wfs_abstract", OWS_NOERR,
                 "        <Abstract>%s</Abstract>\n", NULL);

   msOWSPrintMetadataList(lp->metadata, "wfs_keywordlist", 
                     "        <Keywords>\n", "        </Keywords>\n",
                     "          %s\n");

   // __TODO__ Add optional metadataURL with type, format, etc.

   // In WFS, every layer must have exactly one SRS and there is none at
   // the top level contrary to WMS
   if (msGetEPSGProj(&(lp->projection),lp->metadata, MS_FALSE) != NULL)
   {
       // If layer has a srs then try using it
       msOWSPrintParam("(at least one of) MAP.PROJECTION, LAYER.PROJECTION or wfs_srs metadata", 
                  msGetEPSGProj(&(lp->projection), lp->metadata, MS_FALSE),
                  OWS_WARN, "        <SRS>%s</SRS>\n", NULL);
   }
   else
   {
       // Layer has no SRS, try using map SRS or produce warning
       msOWSPrintParam("(at least one of) MAP.PROJECTION, LAYER.PROJECTION or wfs_srs metadata", 
                  msGetEPSGProj(&(map->projection), map->web.metadata, MS_FALSE),
                  OWS_WARN, "        <SRS>%s</SRS>\n", NULL);
   }

   // If layer has no proj set then use map->proj for bounding box.
   if (msOWSGetLayerExtent(map, lp, &ext) == MS_SUCCESS)
   {
       if(lp->projection.numargs > 0) 
       {
           msOWSPrintLatLonBoundingBox("        ", &(ext), &(lp->projection));
       } 
       else 
       {
           msOWSPrintLatLonBoundingBox("        ", &(ext), &(map->projection));
       }
   }
   else
   {
       printf("<!-- WARNING: Mandatory LatLonBoundingBox could not be established for this layer.  Consider setting wfs_extent metadata. -->\n");
   }

   printf("    </FeatureType>\n");
      
   return MS_SUCCESS;
}




/*
** msWFSGetCapabilities()
*/
int msWFSGetCapabilities(mapObj *map, const char *wmtver) 
{
  char *script_url=NULL, *script_url_encoded;
  int i;

  // Decide which version we're going to return... only 1.0.0 for now
  wmtver = "1.0.0";

  // We need this server's onlineresource.
  if ((script_url=msOWSGetOnlineResource(map, "wfs_onlineresource")) == NULL ||
      (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL)
  {
      return msWFSException(map, wmtver);
  }

  printf("Content-type: text/xml%c%c",10,10); 

  msOWSPrintMetadata(map->web.metadata, "wfs_encoding", OWS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" ?>\n",
                "ISO-8859-1");

  printf("<WFS_Capabilities \n"
         "   version=\"%s\" \n"
         "   updateSequence=\"0\" \n"
         "   xmlns=\"http://www.opengis.net/wfs\" \n"
         "   xmlns:ogc=\"http://www.opengis.net/ogc\" >\n",
         wmtver);

  // Report MapServer Version Information
  printf("\n<!-- %s -->\n\n", msGetVersion());

  /*
  ** SERVICE definition
  */
  printf("<Service>\n");
  printf("  <Name>MapServer WFS</Name>\n");

  // the majority of this section is dependent on appropriately named metadata in the WEB object
  msOWSPrintMetadata(map->web.metadata, "wfs_title", OWS_WARN,
                "  <Title>%s</Title>\n", map->name);
  msOWSPrintMetadata(map->web.metadata, "wfs_abstract", OWS_NOERR,
                "  <Abstract>%s</Abstract>\n", NULL);
  printf("  <OnlineResource>%s</OnlineResource>\n", script_url_encoded);

  msOWSPrintMetadataList(map->web.metadata, "wfs_keywordlist", 
                    "  <Keywords>\n", "  </Keywords>\n",
                    "    %s\n");
  
  msOWSPrintMetadata(map->web.metadata, "wfs_accessconstraints", OWS_NOERR,
                "  <AccessConstraints>%s</AccessConstraints>\n", NULL);
  msOWSPrintMetadata(map->web.metadata, "wfs_fees", OWS_NOERR,
                "  <Fees>%s</Fees>\n", NULL);

  printf("</Service>\n\n");

  /* 
  ** CAPABILITY definitions: list of supported requests
  */

  printf("<Capability>\n");

  printf("  <Request>\n");
  msWFSPrintRequestCap(wmtver, "GetCapabilities", script_url_encoded, 
                       NULL, NULL);
  msWFSPrintRequestCap(wmtver, "DescribeFeatureType", script_url_encoded, 
                       "SchemaDescriptionLanguage", "XMLSCHEMA", NULL);
  msWFSPrintRequestCap(wmtver, "GetFeature", script_url_encoded, 
                       "ResultFormat", "GML2", NULL);
  printf("  </Request>\n");
  printf("</Capability>\n\n");

  /* 
  ** FeatureTypeList: layers
  */

  printf("<FeatureTypeList>\n");

  // Operations supported... set default at top-level, and more operations
  // can be added inside each layer... for MapServer only query is supported
  printf("  <Operations>\n");
  printf("    <Query/>\n");
  printf("  </Operations>\n");

  for(i=0; i<map->numlayers; i++)
  {
      layerObj *lp;
      lp = &(map->layers[i]);

      // List only vector layers in which DUMP=TRUE
      if (msWFSIsLayerSupported(lp))
      {
          msWFSDumpLayer(map, lp);
      }
  }

  printf("</FeatureTypeList>\n\n");


  /*
  ** OGC Filter Capabilities ... for now we support only BBOX
  */

  printf("<ogc:Filter_Capabilities>\n");
  printf("  <ogc:Spatial_Capabilities>\n");
  printf("    <ogc:Spatial_Operators>\n");
  printf("      <ogc:BBOX/>\n");
  printf("    </ogc:Spatial_Operators>\n");
  printf("  </ogc:Spatial_Capabilities>\n");
  printf("</ogc:Filter_Capabilities>\n\n");

  /*
  ** Done!
  */
  printf("</WFS_Capabilities>\n");

  free(script_url);
  free(script_url_encoded);

  return MS_SUCCESS;
}



/*
** msWFSDescribeFeatureType()
*/
int msWFSDescribeFeatureType(mapObj *map, const char *wmtver, 
                             char **names, char **values, int numentries)
{
    int i, numlayers=0;
    char **layers = NULL;
    const char *myns_uri = NULL;

    for(i=0; map && i<numentries; i++) 
    {
        if(strcasecmp(names[i], "TYPENAME") == 0 && numlayers == 0) 
        {
            // Parse comma-delimited list of type names (layers)
            //
            // __TODO__ Need to handle type grouping, e.g. "(l1,l2),l3,l4"
            //
            layers = split(values[i], ',', &numlayers);
        } 
        else if (strcasecmp(names[i], "OUTPUTFORMAT") == 0)
        {
            /* We support only XMLSCHEMA for now.
             */
            if (strcasecmp(values[i], "XMLSCHEMA") != 0)
            {
                msSetError(MS_WFSERR, 
                        "Unsupported DescribeFeatureType outputFormat (%s).", 
                           "msWFSDescribeFeatureType()", values[i]);
                return msWFSException(map, wmtver);
            }
        }

    }

    /*
    ** DescribeFeatureType response
    */
    printf("Content-type: text/xml%c%c",10,10);

    msOWSPrintMetadata(map->web.metadata, "wfs_encoding", OWS_NOERR,
                       "<?xml version='1.0' encoding=\"%s\" ?>\n",
                       "ISO-8859-1");

    myns_uri = msLookupHashTable(map->web.metadata, "gml_uri");
    if (myns_uri == NULL)
        myns_uri = "http://www.ttt.org/myns";

    printf("<schema\n"
           "   targetNamespace=\"%s\" \n"
           "   xmlns:myns=\"%s\" \n"
           "   xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n"
           "   xmlns=\"http://www.w3.org/2001/XMLSchema\"\n"
           "   xmlns:gml=\"http://www.opengis.net/gml\"\n"
           "   elementFormDefault=\"qualified\" version=\"0.1\" >\n", 
           myns_uri, myns_uri );

    printf("\n"
           "  <import namespace=\"http://www.opengis.net/gml\" \n"
           "          schemaLocation=\"%s/gml/2.1/feature.xsd\" />\n",
           msWFSGetSchemasLocation(map));

    /*
    ** loop through layers 
    */
    for(i=0; i<map->numlayers; i++) 
    {
        layerObj *lp;
        int j, bFound = 0;

        lp = &(map->layers[i]);

        for (j=0; j<numlayers && !bFound; j++)
        {
            if ( lp->name && strcasecmp(lp->name, layers[j]) == 0)
            {
                bFound = 1;
            }
        }

        if ( (numlayers == 0 || bFound) && msWFSIsLayerSupported(lp) )
        {
            /*
            ** OK, describe this layer
            */

            printf("\n"
                   "  <element name=\"%s\" \n"
                   "           type=\"%s_Type\" \n"
                   "           substitutionGroup=\"gml:_Feature\" />\n\n",
                   lp->name, lp->name);

            printf("  <complexType name=\"%s_Type\">\n", lp->name);
            printf("    <complexContent>\n");
            printf("      <extension base=\"gml:AbstractFeatureType\">\n");
            printf("        <sequence>\n");

            printf("          <element name=\"%s\" \n"
                   "                   type=\"gml:%s\" \n"
                   "                   nillable=\"false\" />\n",
                   msWFSGetGeomElementName(map, lp),
                   msWFSGetGeomType(lp) );

            if (msLayerOpen(lp, map->shapepath) == MS_SUCCESS)
            {
                if (msLayerGetItems(lp) == MS_SUCCESS)
                {
                    int k;
                    for(k=0; k<lp->numitems; k++)
                        printf("          <element name=\"%s\" type=\"string\" />\n",
                               lp->items[k] );
                }

                msLayerClose(lp);
            }
            else
            {
                printf("\n\n<!-- ERROR: Failed openinig layer %s -->\n\n", 
                       lp->name);
            }

            printf("        </sequence>\n");
            printf("      </extension>\n");
            printf("    </complexContent>\n");
            printf("  </complexType>\n");

        }
    }

    /*
    ** Done!
    */
    printf("\n</schema>\n");

    if (layers)
        msFreeCharArray(layers, numlayers);

    return MS_SUCCESS;
}


/*
** msWFSGetFeature()
*/
int msWFSGetFeature(mapObj *map, const char *wmtver, 
                    char **names, char **values, int numentries)
{
    int         i, maxfeatures=-1;
    const char *typename="", *myns_uri;
    char       *script_url=NULL, *script_url_encoded;
    rectObj     bbox;

    // Default filter is map extents
    bbox = map->extent;

    // Read CGI parameters
    //
    // __TODO__ Need to support XML encoded requests
    //
    for(i=0; map && i<numentries; i++) 
    {
        if(strcasecmp(names[i], "TYPENAME") == 0) 
        {
            char **layers;
            int numlayers, j, k;

            // keep a ref for layer use.
            typename = values[i];

            // Parse comma-delimited list of type names (layers)
            //
            // __TODO__ Need to handle type grouping, e.g. "(l1,l2),l3,l4"
            //
            layers = split(values[i], ',', &numlayers);
            if (layers==NULL || numlayers < 1) 
            {
                msSetError(MS_WFSERR, 
                      "At least one type name required in TYPENAME parameter.",
                           "msWFSGetFeature()");
                return msWFSException(map, wmtver);
            }

            for(j=0; j<map->numlayers; j++) 
            {
                layerObj *lp;

                lp = &(map->layers[j]);

                // Keep only selected layers, set to OFF by default.
                lp->status = MS_OFF;

                for (k=0; k<numlayers; k++)
                {
                    if (msWFSIsLayerSupported(lp) && lp->name && 
                        strcasecmp(lp->name, layers[k]) == 0)
                    {
                        lp->status = MS_ON;
                        if (lp->template == NULL)
                        {
                            // Force setting a template to enable query.
                            lp->template = strdup("ttt.html");
                        }
                    }
                }
            }
       
            msFreeCharArray(layers, numlayers);
        } 
        else if (strcasecmp(names[i], "PROPERTYNAME") == 0)
        {
     
        }
        else if (strcasecmp(names[i], "MAXFEATURES") == 0) 
        {
            maxfeatures = atoi(values[i]);
        }
        else if (strcasecmp(names[i], "FEATUREID") == 0)
        {
     
        }
        else if (strcasecmp(names[i], "FILTER") == 0)
        {
     
        }
        else if (strcasecmp(names[i], "BBOX") == 0)
        {
            char **tokens;
            int n;
            tokens = split(values[i], ',', &n);
            if (tokens==NULL || n != 4) 
            {
                msSetError(MS_WFSERR, "Wrong number of arguments for BBOX.",
                           "msWFSGetFeature()");
                return msWFSException(map, wmtver);
            }
            bbox.minx = atof(tokens[0]);
            bbox.miny = atof(tokens[1]);
            bbox.maxx = atof(tokens[2]);
            bbox.maxy = atof(tokens[3]);
            msFreeCharArray(tokens, n);

            // __TODO__ SRS is implicit: the SRS of selected feature types
            // Need to handle that better in case layers and maps SRS differ

        }

    }

    if(typename == NULL) 
    {
        msSetError(MS_WFSERR, 
                   "Required TYPENAME parameter missing for GetFeature.", 
                   "msWFSGetFeature()");
        return msWFSException(map, wmtver);
    }

    /*
    ** Perform Query (only BBOX for now)
    */
    // __TODO__ Using a rectangle query may not be the most efficient way
    // to do things here.
    if(msQueryByRect(map, -1, bbox) != MS_SUCCESS)
    {
        errorObj   *ms_error;
        ms_error = msGetErrorObj();

        if(ms_error->code != MS_NOTFOUND) 
            return msWFSException(map, wmtver);
    }

    /*
    ** GetFeature response
    */

    myns_uri = msLookupHashTable(map->web.metadata, "gml_uri");
    if (myns_uri == NULL)
        myns_uri = "http://www.ttt.org/myns";

    if ((script_url=msOWSGetOnlineResource(map,"wfs_onlineresource")) ==NULL ||
        (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL)
    {
        return msWFSException(map, wmtver);
    }


    printf("Content-type: text/xml%c%c",10,10);

    msOWSPrintMetadata(map->web.metadata, "wfs_encoding", OWS_NOERR,
                       "<?xml version='1.0' encoding=\"%s\" ?>\n",
                       "ISO-8859-1");

    printf("<wfs:FeatureCollection\n"
           "   xmlns=\"%s\" \n"
           "   xmlns:wfs=\"http://www.opengis.net/wfs\"\n"
           "   xmlns:gml=\"http://www.opengis.net/gml\"\n"
           "   xmlns:xsi=\"http://www.opengis.net/xsi\"\n"
           "   xsi:schemaLocation=\"http://www.opengis.net/wfs %s/wfs/%s/WFS-basic.xsd \n"
           "                       %s %sSERVICE=WFS&amp;VERSION=%s&amp;REQUEST=DescribeFeatureType&amp;TYPENAME=%s\">\n", 
           myns_uri, 
           msWFSGetSchemasLocation(map), wmtver, 
           myns_uri, script_url_encoded, wmtver, typename);


    /* __TODO__ WFS expects homogenous geometry types, but our layers can
    **          contain mixed geometry types... how to deal with that???
    */
    msGMLWriteWFSQuery(map, stdout);

    /*
    ** Done!
    */
    printf("</wfs:FeatureCollection>\n\n");

    free(script_url);
    free(script_url_encoded);

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
int msWFSDispatch(mapObj *map, char **names, char **values, int numentries)
{
#ifdef USE_WFS_SVR
  int i, status;
  static char *wmtver = NULL, *request=NULL, *service=NULL;

  /*
  ** Process Params common to all requests
  */
  for(i=0; i<numentries; i++) {
    if (strcasecmp(names[i], "VERSION") == 0)
      wmtver = values[i];
    else if (strcasecmp(names[i], "REQUEST") == 0)
      request = values[i];
    else if (strcasecmp(names[i], "SERVICE") == 0)
      service = values[i];
  }

  /* If SERVICE is specified then it MUST be "WFS" */
  if (service != NULL && strcasecmp(service, "WFS") != 0)
      return MS_DONE;  /* Not a WFS request */

  /* If SERVICE, VERSION and REQUEST not included than this isn't a WFS req*/
  if (service == NULL && wmtver==NULL && request==NULL)
      return MS_DONE;  /* Not a WFS request */

  /* VERSION *and* REQUEST required by all WFS requests including 
   * GetCapabilities.
   */
  if (wmtver==NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: VERSION parameter missing", 
                 "msWFSDispatch()");
      return msWFSException(map, wmtver);
  }

  if (request==NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: REQUEST parameter missing", 
                 "msWFSDispatch()");
      return msWFSException(map, wmtver);
  }

  if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
      return msWFSException(map, wmtver);

  /*
  ** Start dispatching requests
  */
  if (strcasecmp(request, "GetCapabilities") == 0 ) 
      return msWFSGetCapabilities(map, wmtver);

  /*
  ** Validate VERSION against the versions that we support... we don't do this
  ** for GetCapabilities in order to allow version negociation.
  */
  if (strcmp(wmtver, "1.0.0") != 0)
  {
      msSetError(MS_WFSERR, 
                 "WFS Server does not support VERSION %s.", 
                 "msWFSDispatch()", wmtver);
      return msWFSException(map, wmtver);
  }

  /* Continue dispatching... 
   */
  if (strcasecmp(request, "DescribeFeatureType") == 0)
      return msWFSDescribeFeatureType(map, wmtver, names, values, numentries);
  else if (strcasecmp(request, "GetFeature") == 0)
      return msWFSGetFeature(map, wmtver, names, values, numentries);
  else if (strcasecmp(request, "GetFeatureWithLock") == 0 ||
           strcasecmp(request, "LockFeature") == 0 ||
           strcasecmp(request, "Transaction") == 0 )
  {
      // Unsupported WFS request
      msSetError(MS_WFSERR, "Unsupported WFS request: %s", "msWFSDispatch()",
                 request);
      return msWFSException(map, wmtver);
  }
  else if (strcasecmp(service, "WFS") == 0)
  {
      // Invalid WFS request
      msSetError(MS_WFSERR, "Invalid WFS request: %s", "msWFSDispatch()",
                 request);
      return msWFSException(map, wmtver);
  }

  // This was not detected as a WFS request... let MapServer handle it
  return MS_DONE;

#else
  msSetError(MS_WFSERR, "WFS server support is not available.", 
             "msWFSDispatch()");
  return(MS_FAILURE);
#endif
}


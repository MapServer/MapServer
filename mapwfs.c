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
 * Revision 1.34  2004/05/03 03:45:42  dan
 * Include map= param in default onlineresource of GetCapabilties if it
 * was explicitly set in QUERY_STRING (bug 643)
 *
 * Revision 1.33  2004/04/14 05:14:54  dan
 * Added ability to pass a default value to msOWSPrintMetadataList() (bug 616)
 *
 * Revision 1.32  2004/04/14 04:54:30  dan
 * Created msOWSLookupMetadata() and added namespaces lookup in all
 * msOWSPrint*Metadata() functions. Also pass namespaces=NULL everywhere
 * that calls those functions for now to avoid breaking something just
 * before the release. (bug 615, 568)
 *
 * Revision 1.31  2004/02/23 21:24:44  assefa
 * Name sapce missing for DescribeFeatureType request.
 *
 * Revision 1.30  2004/02/12 23:29:12  assefa
 * Ordrer was incorrect for element Keyword and Fees.
 *
 * Revision 1.29  2004/02/04 19:46:24  assefa
 * Add support for multiple spatial opertaors inside one filter.
 * Add support for opeartors DWithin and Intersect.
 *
 * Revision 1.28  2003/10/13 15:50:01  assefa
 * myns namespace is the default.
 *
 * Revision 1.27  2003/10/08 14:01:50  assefa
 * Write properly the geomelement.
 *
 * Revision 1.26  2003/10/08 13:10:17  assefa
 * Modify msWFSGetGeomElementName to return the gml propertytype.
 *
 * Revision 1.25  2003/10/08 00:01:35  assefa
 * Correct IsLikePropery with an OR.
 * Use of namespace if given in metadata.
 *
 * Revision 1.24  2003/10/06 13:03:19  assefa
 * Use of namespace. Correct execption return.
 *
 * Revision 1.23  2003/09/30 15:56:40  assefa
 * Typenames may have namespaces.
 *
 * Revision 1.22  2003/09/30 03:15:31  assefa
 * Add namespace ogc in Filter capabilities.
 *
 * Revision 1.21  2003/09/29 20:43:09  assefa
 * Query all layers only when filter is not set.
 *
 * Revision 1.20  2003/09/29 14:18:20  assefa
 * Support a diffrent was of giving the srs value for the gml Box element.
 *
 * Revision 1.19  2003/09/26 13:44:40  assefa
 * Add support for gml box with 2 <coord> elements.
 *
 * Revision 1.18  2003/09/22 22:53:20  assefa
 * Add ifdef USE_OGR where the MiniMXL Parser is used.
 *
 * Revision 1.17  2003/09/19 21:54:19  assefa
 * Add support fot the Post request.
 *
 * Revision 1.16  2003/09/10 19:58:24  assefa
 * Rename filterencoding.h file.
 *
 * Revision 1.15  2003/09/10 03:51:25  assefa
 * Add Filters in the Capabilities.
 *
 * Revision 1.14  2003/09/04 17:47:15  assefa
 * Add filterencoding tests.
 *
 * Revision 1.13  2003/02/05 04:40:11  sdlime
 * Removed shapepath as an argument from msLayerOpen and msSHPOpenFile. The shapefile opening routine now expects just a filename. So, you must use msBuildPath or msBuildPath3 to create a full qualified filename. Relatively simple change, but required lots of changes. Demo still works...
 *
 * Revision 1.12  2002/12/19 05:17:09  dan
 * Report WFS exceptions, and do not fail on WFS requests returning 0 features
 *
 * Revision 1.11  2002/12/18 22:36:22  dan
 * Sorted out projection handling in GetCapabilities and Getfeature
 *
 * Revision 1.10  2002/12/18 16:45:49  dan
 * Fixed WFS capabilities to validate against schema
 *
 * Revision 1.9  2002/12/17 04:50:43  dan
 * Fixed a few WFS XML validation issues
 *
 * Revision 1.8  2002/11/20 21:22:32  dan
 * Added msOWSGetSchemasLocation() for use by both WFS and WMS Map Context
 *
 * Revision 1.7  2002/10/28 20:31:21  dan
 * New support for WMS Map Context (from Julien)
 *
 * Revision 1.2  2002/10/22 20:03:57  julien
 * Add the mapcontext support
 *
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

#include "mapogcfilter.h"

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

    printf("<ServiceExceptionReport\n");
    printf("xmlns=\"http://www.opengis.net/ogc\" ");
    printf("xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
    printf("xsi:schemaLocation=\"http://www.opengis.net/ogc http://ogc.dmsolutions.ca/wfs/1.0/OGC-exception.xsd\">\n");
    printf("  <ServiceException>\n");
    /* Optional <Locator> element currently unused. */
    //printf("    <Message>\n");
    msWriteErrorXML(stdout);
    //printf("    </Message>\n");
    printf("  </ServiceException>\n");
    printf("</ServiceExceptionReport>\n");

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
  printf("      <DCPType>\n");
  printf("        <HTTP>\n");
  printf("          <Post onlineResource=\"%s\" />\n", script_url);
  printf("        </HTTP>\n");
  printf("      </DCPType>\n");


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

   msOWSPrintParam(stdout, "LAYER.NAME", lp->name, OWS_WARN, 
              "        <Name>%s</Name>\n", NULL);

   msOWSPrintMetadata(stdout, lp->metadata, NULL, "wfs_title", OWS_WARN,
                 "        <Title>%s</Title>\n", lp->name);

   msOWSPrintMetadata(stdout, lp->metadata, NULL, "wfs_abstract", OWS_NOERR,
                 "        <Abstract>%s</Abstract>\n", NULL);

   msOWSPrintMetadataList(stdout, lp->metadata, NULL, "wfs_keywordlist", 
                     "        <Keywords>\n", "        </Keywords>\n",
                     "          %s\n", NULL);

   // __TODO__ Add optional metadataURL with type, format, etc.

   // In WFS, every layer must have exactly one SRS and there is none at
   // the top level contrary to WMS
   //
   // So here is the way we'll deal with SRS:
   // 1- If a top-level map projection (or wfs_srs metadata) is set then
   //    all layers are advertized in the map's projection and they will
   //    be reprojected on the fly in the GetFeature request.
   // 2- If there is no top-level map projection (or wfs_srs metadata) then
   //    each layer is advertized in its own projection as defined in the
   //    layer's projection object or wfs_srs metadata.
   //
   if (msGetEPSGProj(&(map->projection),map->web.metadata, MS_TRUE) != NULL)
   {
       // Map has a SRS.  Use it for all layers.
       msOWSPrintParam(stdout, "(at least one of) MAP.PROJECTION, LAYER.PROJECTION or wfs_srs metadata", 
                  msGetEPSGProj(&(map->projection),map->web.metadata,MS_TRUE),
                  OWS_WARN, "        <SRS>%s</SRS>\n", NULL);
   }
   else
   {
       // Map has no SRS.  Use layer SRS or produce a warning.
       msOWSPrintParam(stdout, "(at least one of) MAP.PROJECTION, LAYER.PROJECTION or wfs_srs metadata", 
                  msGetEPSGProj(&(lp->projection), lp->metadata, MS_TRUE),
                  OWS_WARN, "        <SRS>%s</SRS>\n", NULL);
   }

   // If layer has no proj set then use map->proj for bounding box.
   if (msOWSGetLayerExtent(map, lp, &ext) == MS_SUCCESS)
   {
       if(lp->projection.numargs > 0) 
       {
           msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext), 
                                       &(lp->projection), OWS_WFS);
       } 
       else 
       {
           msOWSPrintLatLonBoundingBox(stdout, "        ", &(ext), 
                                       &(map->projection), OWS_WFS);
       }
   }
   else
   {
       printf("<!-- WARNING: Mandatory LatLongBoundingBox could not be established for this layer.  Consider setting wfs_extent metadata. -->\n");
   }

   printf("    </FeatureType>\n");
      
   return MS_SUCCESS;
}




/*
** msWFSGetCapabilities()
*/
int msWFSGetCapabilities(mapObj *map, const char *wmtver, cgiRequestObj *req) 
{
  char *script_url=NULL, *script_url_encoded;
  int i;

  // Decide which version we're going to return... only 1.0.0 for now
  wmtver = "1.0.0";

  // We need this server's onlineresource.
  if ((script_url=msOWSGetOnlineResource(map, "wfs_onlineresource", req)) == NULL ||
      (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL)
  {
      return msWFSException(map, wmtver);
  }

  printf("Content-type: text/xml%c%c",10,10); 

  msOWSPrintMetadata(stdout, map->web.metadata, NULL, "wfs_encoding", OWS_NOERR,
                "<?xml version='1.0' encoding=\"%s\" ?>\n",
                "ISO-8859-1");

  printf("<WFS_Capabilities \n"
         "   version=\"%s\" \n"
         "   updateSequence=\"0\" \n"
         "   xmlns=\"http://www.opengis.net/wfs\" \n"
         "   xmlns:ogc=\"http://www.opengis.net/ogc\" \n"
         "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
         "   xsi:schemaLocation=\"http://www.opengis.net/wfs %s/wfs/%s/WFS-capabilities.xsd\">\n", 
         wmtver,
         msOWSGetSchemasLocation(map), wmtver);

  // Report MapServer Version Information
  printf("\n<!-- %s -->\n\n", msGetVersion());

  /*
  ** SERVICE definition
  */
  printf("<Service>\n");
 printf("  <Name>MapServer WFS</Name>\n");

  // the majority of this section is dependent on appropriately named metadata in the WEB object
  msOWSPrintMetadata(stdout, map->web.metadata, NULL, "wfs_title", OWS_WARN,
                "  <Title>%s</Title>\n", map->name);
  msOWSPrintMetadata(stdout, map->web.metadata, NULL, "wfs_abstract", OWS_NOERR,
                "  <Abstract>%s</Abstract>\n", NULL);

  msOWSPrintMetadataList(stdout, map->web.metadata, NULL, "wfs_keywordlist", 
                    "  <Keywords>\n", "  </Keywords>\n",
                    "    %s\n", NULL);
  printf("  <OnlineResource>%s</OnlineResource>\n", script_url_encoded);

  msOWSPrintMetadata(stdout, map->web.metadata, NULL, "wfs_fees", OWS_NOERR,
                "  <Fees>%s</Fees>\n", NULL);
  
  msOWSPrintMetadata(stdout, map->web.metadata, NULL, "wfs_accessconstraints", OWS_NOERR,
                "  <AccessConstraints>%s</AccessConstraints>\n", NULL);

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
  printf("      <ogc:Intersect/>\n");
  printf("      <ogc:DWithin/>\n");
  printf("      <ogc:BBOX/>\n");
  printf("    </ogc:Spatial_Operators>\n");
  printf("  </ogc:Spatial_Capabilities>\n");

  printf("  <ogc:Scalar_Capabilities>\n");
  printf("    <ogc:Logical_Operators />\n");
  printf("    <ogc:Comparison_Operators>\n");
  printf("      <ogc:Simple_Comparisons />\n");
  printf("      <ogc:Like />\n");
  printf("      <ogc:Between />\n");
  printf("    </ogc:Comparison_Operators>\n");
  printf("  </ogc:Scalar_Capabilities>\n");

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
int msWFSDescribeFeatureType(mapObj *map, wfsParamsObj *paramsObj)
{
    int i, numlayers=0;
    char **layers = NULL;
    const char *myns_uri = NULL;
    char **tokens;
    int n=0;
    char *user_namespace_prefix = NULL;
    char *user_namespace_uri = NULL;

    if(paramsObj->pszTypeName && numlayers == 0) 
    {
        // Parse comma-delimited list of type names (layers)
        //
        // __TODO__ Need to handle type grouping, e.g. "(l1,l2),l3,l4"
        //
        layers = split(paramsObj->pszTypeName, ',', &numlayers);
        if (numlayers > 0)
        {
            //strip namespace if there is one :ex TYPENAME=cdf:Other
            tokens = split(layers[0], ':', &n);
            if (tokens && n==2 && msGetLayerIndex(map, layers[0]) < 0)
            {
                msFreeCharArray(tokens, n);
                for (i=0; i<numlayers; i++)
                {
                    tokens = split(layers[i], ':', &n);
                    if (tokens && n==2)
                    {
                        free(layers[i]);
                        layers[i] = strdup(tokens[1]);
                    }
                    if (tokens)
                      msFreeCharArray(tokens, n);
                }
            }
        }
    } 
    if (paramsObj->pszOutputFormat)
    {
        /* We support only XMLSCHEMA for now.
         */
        if (strcasecmp(paramsObj->pszOutputFormat, "XMLSCHEMA") != 0)
        {
            msSetError(MS_WFSERR, 
                       "Unsupported DescribeFeatureType outputFormat (%s).", 
                       "msWFSDescribeFeatureType()", paramsObj->pszOutputFormat);
            return msWFSException(map, paramsObj->pszVersion);
        }
    }

    //Validate layers
    if (numlayers > 0)
    {
        for (i=0; i<numlayers; i++)
        {
            if (msGetLayerIndex(map, layers[i]) < 0)
            {
                msSetError(MS_WFSERR, 
                       "Invalid typename (%s).", 
                       "msWFSDescribeFeatureType()", paramsObj->pszTypeName);
                return msWFSException(map, paramsObj->pszVersion);
            }
        }
    }
            
    /*
    ** DescribeFeatureType response
    */
    printf("Content-type: text/xml%c%c",10,10);

    msOWSPrintMetadata(stdout, map->web.metadata, NULL, "wfs_encoding", OWS_NOERR,
                       "<?xml version='1.0' encoding=\"%s\" ?>\n",
                       "ISO-8859-1");

    myns_uri = msLookupHashTable(map->web.metadata, "gml_uri");
    if (myns_uri == NULL)
        myns_uri = "http://www.ttt.org/myns";

    user_namespace_prefix =  
      msLookupHashTable(map->web.metadata, "wfs_namespace_prefix");
    user_namespace_uri =  
      msLookupHashTable(map->web.metadata, "wfs_namespace_uri");
    
    if (user_namespace_prefix && user_namespace_uri)
    {
        printf("<schema\n"
               "   targetNamespace=\"%s\" \n"
               "   xmlns:%s=\"%s\" \n"
               "   xmlns:ogc=\"http://www.opengis.net/ogc\"\n"
               "   xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n"
               "   xmlns=\"http://www.w3.org/2001/XMLSchema\"\n"
               "   xmlns:gml=\"http://www.opengis.net/gml\"\n"
               "   elementFormDefault=\"qualified\" version=\"0.1\" >\n", 
               user_namespace_uri, user_namespace_prefix,  user_namespace_uri);
    }
    else
     printf("<schema\n"
               "   targetNamespace=\"%s\" \n"
               "   xmlns:myns=\"%s\" \n"
               "   xmlns:ogc=\"http://www.opengis.net/ogc\"\n"
               "   xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n"
               "   xmlns=\"http://www.w3.org/2001/XMLSchema\"\n"
               "   xmlns:gml=\"http://www.opengis.net/gml\"\n"
               "   elementFormDefault=\"qualified\" version=\"0.1\" >\n", 
               myns_uri, myns_uri); 
    

    printf("\n"
           "  <import namespace=\"http://www.opengis.net/gml\" \n"
           "          schemaLocation=\"%s/gml/2.1.1/feature.xsd\" />\n",
           msOWSGetSchemasLocation(map));

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

            if (user_namespace_prefix)
              printf("\n"
                   "  <element name=\"%s\" \n"
                   "           type=\"%s:%s_Type\" \n"
                   "           substitutionGroup=\"gml:_Feature\" />\n\n",
                   lp->name, user_namespace_prefix, lp->name);
            else
              printf("\n"
                     "  <element name=\"%s\" \n"
                     "           type=\"myns:%s_Type\" \n"
                     "           substitutionGroup=\"gml:_Feature\" />\n\n",
                     lp->name, lp->name);

            printf("  <complexType name=\"%s_Type\">\n", lp->name);
            printf("    <complexContent>\n");
            printf("      <extension base=\"gml:AbstractFeatureType\">\n");
            printf("        <sequence>\n");

            printf("          <element ref=\"gml:%s\" minOccurs=\"0\" />\n",
                   msWFSGetGeomElementName(map, lp));
            /*
             printf("          <element name=\"%s\" \n"
                   "                   type=\"gml:%s\" \n"
                   "                   nillable=\"false\" />\n",
                   msWFSGetGeomElementName(map, lp),
                   msWFSGetGeomType(lp) );
            */

            if (msLayerOpen(lp) == MS_SUCCESS)
            {
                if (msLayerGetItems(lp) == MS_SUCCESS)
                {
                    int k;
                    char *name_gml = NULL;
                    char *description_gml = NULL;
                    name_gml = msLookupHashTable(lp->metadata, "wfs_gml_name_item");
                    description_gml = msLookupHashTable(lp->metadata, "wfs_gml_description_item");
                    for(k=0; k<lp->numitems; k++)
                    {
                         if (name_gml && strcmp(name_gml,  lp->items[k]) == 0)
                           continue;
                          if (description_gml && strcmp(description_gml,  lp->items[k]) == 0)
                            continue;
                        printf("          <element name=\"%s\" type=\"string\" />\n",
                               lp->items[k] );
                    }
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
int msWFSGetFeature(mapObj *map, wfsParamsObj *paramsObj, cgiRequestObj *req)
  //const char *wmtver, char **names, char **values, int numentries)
{
    int         i, maxfeatures=-1;
    const char *typename="", *myns_uri;
    char       *script_url=NULL, *script_url_encoded;
    rectObj     bbox;
    const char  *pszOutputSRS = NULL;
    
    char **layers = NULL;
    int numlayers = 0;

    char   *pszFilter = NULL;
    int bFilterSet = 0;
    int bBBOXSet = 0;
    char *pszNameSpace = NULL;
    char *user_namespace_prefix = NULL;
    char *user_namespace_uri = NULL;

    // Default filter is map extents
    bbox = map->extent;

    // Read CGI parameters
    //
    // __TODO__ Need to support XML encoded requests
    //

    
    if(paramsObj->pszTypeName) 
    {
        int j, k;
        const char *pszMapSRS = NULL;
        char **tokens;
        int n=0, i=0;
        // keep a ref for layer use.
        typename = paramsObj->pszTypeName;
        
        // Parse comma-delimited list of type names (layers)
        //
        // __TODO__ Need to handle type grouping, e.g. "(l1,l2),l3,l4"
        //
        layers = split(typename, ',', &numlayers);
/* ==================================================================== */
/*      check if the typename contains namespaces (ex cdf:Other),       */
/*      If that is the case extarct only the layer name.                */
/* ==================================================================== */
         
        if (layers==NULL || numlayers < 1) 
        {
            msSetError(MS_WFSERR, 
                       "At least one type name required in TYPENAME parameter.",
                     "msWFSGetFeature()");
            return msWFSException(map, paramsObj->pszVersion);
        }
        tokens = split(layers[0], ':', &n);
        if (tokens && n==2 && msGetLayerIndex(map, layers[0]) < 0)
        {
            //pszNameSpace = strdup(tokens[0]);
            msFreeCharArray(tokens, n);
            for (i=0; i<numlayers; i++)
            {
                tokens = split(layers[i], ':', &n);
                 if (tokens && n==2)
                 {
                     free(layers[i]);
                     layers[i] = strdup(tokens[1]);
                 }
                 if (tokens)
                    msFreeCharArray(tokens, n);
            }
        }

        pszMapSRS = msGetEPSGProj(&(map->projection),
                                  map->web.metadata, 
                                  MS_TRUE);

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
                    const char *pszThisLayerSRS;

                    lp->status = MS_ON;
                    if (lp->template == NULL)
                    {
                        // Force setting a template to enable query.
                        lp->template = strdup("ttt.html");
                    }

                    // See comments in msWFSGetCapabilities() about the
                    // rules for SRS.
                    if ((pszThisLayerSRS = pszMapSRS) == NULL)
                    {
                        pszThisLayerSRS = msGetEPSGProj(&(lp->projection),
                                                        lp->metadata, 
                                                        MS_TRUE);
                    }

                    if (pszThisLayerSRS == NULL)
                    {
                        msSetError(MS_WFSERR, 
                                   "Server config error: SRS must be set at least at the map or at the layer level.",
                                   "msWFSGetFeature()");
                        return msWFSException(map, paramsObj->pszVersion);
                    }

                    // Keep track of output SRS.  If different from value
                    // from previous layers then this is an invalid request
                    // i.e. all layers in a single request must be in the
                    // same SRS.
                    if (pszOutputSRS == NULL)
                    {
                        pszOutputSRS = pszThisLayerSRS;
                    }
                    else if (strcasecmp(pszThisLayerSRS,pszOutputSRS) != 0)
                    {
                        msSetError(MS_WFSERR, 
                                   "Invalid GetFeature Request: All TYPENAMES in a single GetFeature request must have been advertized in the same SRS.  Please check the capabilities and reformulate your request.",
                                   "msWFSGetFeature()");
                        return msWFSException(map, paramsObj->pszVersion);
                    }
                }
            }
        }
    }
    //Validate outputformat
    if (paramsObj->pszOutputFormat)
    {
        /* We support only GML2 for now.
         */
        if (strcasecmp(paramsObj->pszOutputFormat, "GML2") != 0)
        {
            msSetError(MS_WFSERR, 
                       "Unsupported GetFeature outputFormat (%s). Only GML2 is supported.", 
                       "msWFSDescribeFeatureType()", paramsObj->pszOutputFormat);
            return msWFSException(map, paramsObj->pszVersion);
        }
    }
    //else if (strcasecmp(names[i], "PROPERTYNAME") == 0)
    //{
    //        
    //}
    if (paramsObj->nMaxFeatures > 0) 
    {
        maxfeatures = paramsObj->nMaxFeatures;
    }
    //if (strcasecmp(names[i], "FEATUREID") == 0)
    //{
    //
    //}
    
    if (paramsObj->pszFilter)
    {
        bFilterSet = 1;
        pszFilter = paramsObj->pszFilter;
    }
    else if (paramsObj->pszBbox)
    {
        char **tokens;
        int n;
        tokens = split(paramsObj->pszBbox, ',', &n);
        if (tokens==NULL || n != 4) 
        {
            msSetError(MS_WFSERR, "Wrong number of arguments for BBOX.",
                       "msWFSGetFeature()");
            return msWFSException(map, paramsObj->pszVersion);
        }
        bbox.minx = atof(tokens[0]);
        bbox.miny = atof(tokens[1]);
        bbox.maxx = atof(tokens[2]);
        bbox.maxy = atof(tokens[3]);
        msFreeCharArray(tokens, n);

        // Note: BBOX SRS is implicit, it is the SRS of the selected
        // feature types, see pszOutputSRS in TYPENAMES above.
    }



#ifdef USE_OGR
    if (bFilterSet && pszFilter && strlen(pszFilter) > 0)
    {
        char **tokens = NULL;
        int nFilters;
        FilterEncodingNode *psNode = NULL;
        int iLayerIndex =1;
        char **paszFilter = NULL;

/* -------------------------------------------------------------------- */
/*      Validate the parameters. When a FILTER parameter is given,      */
/*      It needs the TYPENAME parameter for the layers. Also Filter     */
/*      is Mutually exclusive with FEATUREID and BBOX (see wfs specs    */
/*      1.0 section 13.7.3 on GetFeature)                               */
/*                                                                      */
/* -------------------------------------------------------------------- */
        if (typename == NULL || strlen(typename) <= 0 || 
            layers == NULL || numlayers <= 0)
        {
            msSetError(MS_WFSERR, 
                   "Required TYPENAME parameter missing for GetFeature with a FILTER parameter.", 
                   "msWFSGetFeature()");
            return msWFSException(map, paramsObj->pszVersion);
        }
        if (bBBOXSet)
        {
             msSetError(MS_WFSERR, 
                   "BBOX parameter and FILTER parameter are mutually exclusive in GetFeature.", 
                   "msWFSGetFeature()");
            return msWFSException(map, paramsObj->pszVersion);
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
        if (strlen(pszFilter) > 0 && pszFilter[0] == '(')        
        {
            tokens = split(pszFilter+1, '(', &nFilters);
            
            if (tokens == NULL || nFilters <=0 || nFilters != numlayers)
            {
                msSetError(MS_WFSERR, "Wrong number of FILTER attributes",
                           "msWFSGetFeature()");
                return msWFSException(map, paramsObj->pszVersion);
            }
            paszFilter = (char **)malloc(sizeof(char *)*nFilters);
            for (i=0; i<nFilters; i++)
              paszFilter[i] = strdup(tokens[i]);

            if (tokens)
              msFreeCharArray(tokens, nFilters);
        }
        else
        {
            nFilters=1;
            paszFilter = (char **)malloc(sizeof(char *)*nFilters);
            paszFilter[0] = pszFilter;
        }
        
/* -------------------------------------------------------------------- */
/*      run through the filters and build the class expressions.        */
/* -------------------------------------------------------------------- */
        for (i=0; i<nFilters; i++)
        {
            iLayerIndex = msGetLayerIndex(map, layers[i]);
            if (iLayerIndex < 0)
            {
                msSetError(MS_WFSERR, 
                   "Invalid Typename in GetFeature : %s", 
                   "msWFSGetFeature()", layers[i]);
                return msWFSException(map, paramsObj->pszVersion);
            }
            psNode = FLTParseFilterEncoding(paszFilter[i]);
            
            if (!psNode)
            {
                msSetError(MS_WFSERR, 
                   "Invalid or Unsupported FILTER in GetFeature : %s", 
                   "msWFSGetFeature()", pszFilter);
                return msWFSException(map, paramsObj->pszVersion);
            }
            FLTApplyFilterToLayer(psNode, map, iLayerIndex);
            
        }
        if (paszFilter)
          free(paszFilter);
    }//end if filter set
#endif
    if(layers)
      msFreeCharArray(layers, numlayers);

    if(typename == NULL) 
    {
        msSetError(MS_WFSERR, 
                   "Required TYPENAME parameter missing for GetFeature.", 
                   "msWFSGetFeature()");
        return msWFSException(map, paramsObj->pszFilter);
    }

    // Set map output projection to which output features should be reprojected
    if (pszOutputSRS && strncasecmp(pszOutputSRS, "EPSG:", 5) == 0)
    {
        char szBuf[32];
        sprintf(szBuf, "init=epsg:%.10s", pszOutputSRS+5);

        if (msLoadProjectionString(&(map->projection), szBuf) != 0)
          return msWFSException(map, paramsObj->pszVersion);
    }

    /*
    ** Perform Query (only BBOX for now)
    */
    // __TODO__ Using a rectangle query may not be the most efficient way
    // to do things here.
    if (!bFilterSet)
    {
        if(msQueryByRect(map, -1, bbox) != MS_SUCCESS)
        {
            errorObj   *ms_error;
            ms_error = msGetErrorObj();

            if(ms_error->code != MS_NOTFOUND) 
              return msWFSException(map, paramsObj->pszVersion);
        }
    }

    /*
    ** GetFeature response
    */

    myns_uri = msLookupHashTable(map->web.metadata, "gml_uri");
    if (myns_uri == NULL)
        myns_uri = "http://www.ttt.org/myns";

    if ((script_url=msOWSGetOnlineResource(map,"wfs_onlineresource",req)) ==NULL ||
        (script_url_encoded = msEncodeHTMLEntities(script_url)) == NULL)
    {
        return msWFSException(map, paramsObj->pszVersion);
    }


    printf("Content-type: text/xml%c%c",10,10);

    msOWSPrintMetadata(stdout, map->web.metadata, NULL, "wfs_encoding", OWS_NOERR,
                       "<?xml version='1.0' encoding=\"%s\" ?>\n",
                       "ISO-8859-1");

    user_namespace_uri =  
      msLookupHashTable(map->web.metadata, "wfs_namespace_uri");

    user_namespace_prefix = 
      msLookupHashTable(map->web.metadata, "wfs_namespace_prefix");
    if (user_namespace_prefix && user_namespace_uri)
      pszNameSpace = strdup(user_namespace_prefix);
    //else
    //  pszNameSpace = strdup("myns");

     

    if (user_namespace_prefix && user_namespace_uri)
    {
        printf("<wfs:FeatureCollection\n"
               "   xmlns:%s=\"%s\"\n"  
               "   xmlns:wfs=\"http://www.opengis.net/wfs\"\n"
               "   xmlns:gml=\"http://www.opengis.net/gml\"\n"
               "   xmlns:ogc=\"http://www.opengis.net/ogc\"\n"
               "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
               "   xsi:schemaLocation=\"http://www.opengis.net/wfs %s/wfs/%s/WFS-basic.xsd \n"
               "                       %s %sSERVICE=WFS&amp;VERSION=%s&amp;REQUEST=DescribeFeatureType&amp;TYP\
ENAME=%s\">\n",
              user_namespace_prefix, user_namespace_uri,
               msOWSGetSchemasLocation(map), paramsObj->pszVersion,
               user_namespace_uri, script_url_encoded, paramsObj->pszVersion, typename);
    }
    else
      printf("<wfs:FeatureCollection\n"
             "   xmlns=\"%s\"\n"
             "   xmlns:myns=\"%s\"\n"
             "   xmlns:wfs=\"http://www.opengis.net/wfs\"\n"
             "   xmlns:gml=\"http://www.opengis.net/gml\"\n"
             "   xmlns:ogc=\"http://www.opengis.net/ogc\"\n"
             "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
             "   xsi:schemaLocation=\"http://www.opengis.net/wfs %s/wfs/%s/WFS-basic.xsd \n"
             "                       %s %sSERVICE=WFS&amp;VERSION=%s&amp;REQUEST=DescribeFeatureType&amp;TYP\
ENAME=%s\">\n",
           myns_uri, myns_uri,
           msOWSGetSchemasLocation(map), paramsObj->pszVersion,
           myns_uri, script_url_encoded, paramsObj->pszVersion, typename);


    /* __TODO__ WFS expects homogenous geometry types, but our layers can
    **          contain mixed geometry types... how to deal with that???
    */
    msGMLWriteWFSQuery(map, stdout, maxfeatures, pszNameSpace);
    
    //if no results where written 
    for(i=0; i<map->numlayers; i++) 
    {
        if (map->layers[i].resultcache && 
            map->layers[i].resultcache->numresults > 0)
          break;
    }
    if (i==map->numlayers)
    {
        printf("   <gml:boundedBy>\n"); 
        printf("      <gml:null>inapplicable</gml:null>\n");
        printf("   </gml:boundedBy>\n"); 
    }
    /*
    ** Done!
    */
    

    printf("</wfs:FeatureCollection>\n\n");

    free(script_url);
    free(script_url_encoded);
    if (pszNameSpace)
      free(pszNameSpace);

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


  //static char *wmtver = NULL, *request=NULL, *service=NULL;
  wfsParamsObj *paramsObj;
  /*
  ** Populate the Params object based on the request
  */
  paramsObj = msWFSCreateParamsObj();
  //TODO : store also parameters that are inside the map object
  //into the paramsObj. 
  msWFSParseRequest(requestobj, paramsObj);

  /* If SERVICE is specified then it MUST be "WFS" */
  if (paramsObj->pszService != NULL && 
      strcasecmp(paramsObj->pszService, "WFS") != 0)
      return MS_DONE;  /* Not a WFS request */

  /* If SERVICE, VERSION and REQUEST not included than this isn't a WFS req*/
  if (paramsObj->pszService == NULL && paramsObj->pszVersion==NULL && 
      paramsObj->pszRequest==NULL)
      return MS_DONE;  /* Not a WFS request */

  /* VERSION *and* REQUEST required by all WFS requests including 
   * GetCapabilities.
   */
  if (paramsObj->pszVersion==NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: VERSION parameter missing", 
                 "msWFSDispatch()");
      return msWFSException(map, paramsObj->pszVersion);
  }

  if (paramsObj->pszRequest==NULL)
  {
      msSetError(MS_WFSERR, 
                 "Incomplete WFS request: REQUEST parameter missing", 
                 "msWFSDispatch()");
      return msWFSException(map, paramsObj->pszVersion);
  }

  if ((status = msOWSMakeAllLayersUnique(map)) != MS_SUCCESS)
      return msWFSException(map, paramsObj->pszVersion);

  /*
  ** Start dispatching requests
  */
  if (strcasecmp(paramsObj->pszRequest, "GetCapabilities") == 0 ) 
      return msWFSGetCapabilities(map, paramsObj->pszVersion, requestobj);

  /*
  ** Validate VERSION against the versions that we support... we don't do this
  ** for GetCapabilities in order to allow version negociation.
  */
  if (strcmp(paramsObj->pszVersion, "1.0.0") != 0)
  {
      msSetError(MS_WFSERR, 
                 "WFS Server does not support VERSION %s.", 
                 "msWFSDispatch()", paramsObj->pszVersion);
      return msWFSException(map, paramsObj->pszVersion);
  }

  /* Continue dispatching... 
   */
  if (strcasecmp(paramsObj->pszRequest, "DescribeFeatureType") == 0)
    return msWFSDescribeFeatureType(map, paramsObj);

  else if (strcasecmp(paramsObj->pszRequest, "GetFeature") == 0)
    return msWFSGetFeature(map, paramsObj, requestobj);


  else if (strcasecmp(paramsObj->pszRequest, "GetFeatureWithLock") == 0 ||
           strcasecmp(paramsObj->pszRequest, "LockFeature") == 0 ||
           strcasecmp(paramsObj->pszRequest, "Transaction") == 0 )
  {
      // Unsupported WFS request
      msSetError(MS_WFSERR, "Unsupported WFS request: %s", "msWFSDispatch()",
                 paramsObj->pszRequest);
      return msWFSException(map, paramsObj->pszVersion);
  }
  else if (strcasecmp(paramsObj->pszService, "WFS") == 0)
  {
      // Invalid WFS request
      msSetError(MS_WFSERR, "Invalid WFS request: %s", "msWFSDispatch()",
                 paramsObj->pszRequest);
      return msWFSException(map, paramsObj->pszVersion);
  }

  // This was not detected as a WFS request... let MapServer handle it
  return MS_DONE;

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
        if (wfsparams->pszRequest)
          free(wfsparams->pszRequest);
        if (wfsparams->pszService)
          free(wfsparams->pszService);
        if (wfsparams->pszTypeName)
          free(wfsparams->pszTypeName);
        if (wfsparams->pszFilter)
          free(wfsparams->pszFilter);
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

                else if (strcasecmp(request->ParamNames[i], "REQUEST") == 0)
                  wfsparams->pszRequest = strdup(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "SERVICE") == 0)
                  wfsparams->pszService = strdup(request->ParamValues[i]);
             
                else if (strcasecmp(request->ParamNames[i], "MAXFEATURES") == 0)
                  wfsparams->nMaxFeatures = atoi(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "BBOX") == 0)
                  wfsparams->pszBbox = strdup(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "TYPENAME") == 0)
                  wfsparams->pszTypeName = strdup(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "FILTER") == 0)
                  wfsparams->pszFilter = strdup(request->ParamValues[i]);
                
                else if (strcasecmp(request->ParamNames[i], "OUTPUTFORMAT") == 0)
                  wfsparams->pszOutputFormat = strdup(request->ParamValues[i]);
            }
        }
        //version is optional is the GetCapabilities. If not
        //provided, set it.
        if (wfsparams->pszRequest && 
            strcasecmp(wfsparams->pszRequest, "GetCapabilities") == 0)
          wfsparams->pszVersion = strdup("1.0.0");
    }
/* -------------------------------------------------------------------- */
/*      Parse the post request. It is assumed to be an XML document.    */
/* -------------------------------------------------------------------- */
#ifdef USE_OGR
    if (request->postrequest)
    {
        CPLXMLNode *psRoot, *psQuery, *psFilter, *psTypeName = NULL;
        CPLXMLNode *psGetFeature = NULL;
        CPLXMLNode  *psGetCapabilities = NULL;
        CPLXMLNode *psDescribeFeature = NULL;
        CPLXMLNode *psOperation = NULL;
        char *pszValue, *pszSerializedFilter, *pszTmp = NULL;
        int bMultiLayer = 0;

        psRoot = CPLParseXMLString(request->postrequest);

        if (psRoot)
        {
            //need to strip namespaces
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
                        break;
                    }
                    else if(strcasecmp(psOperation->pszValue,
                                       "DescribeFeatureType")==0)
                    {
                        psDescribeFeature = psOperation;
                        break;
                    }
                    //these are unsupported requests. Just set the 
                    //request value and return;
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


                pszValue = (char*)CPLGetXMLValue(psGetFeature,  "maxFeatures", 
                                                 NULL);
                if (pszValue)
                  wfsparams->nMaxFeatures = atoi(pszValue);

                psQuery = CPLGetXMLNode(psGetFeature, "Query");
                if (psQuery)
                {
                   
                    //free typname and filter. There may have been
                    //values if they were passed in the URL
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
/* -------------------------------------------------------------------- */
                    while (psQuery &&  psQuery->pszValue && 
                           strcasecmp(psQuery->pszValue, "Query") == 0)
                    {
                        //parse typenames
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

                        //parse filter
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
                    }//while psQuery
                }
            }//end of GetFeatures

/* -------------------------------------------------------------------- */
/*      Parse GetCapabilities.                                          */
/* -------------------------------------------------------------------- */
            if (psGetCapabilities)
            {
                wfsparams->pszRequest = strdup("GetCapabilities");
                pszValue = (char*)CPLGetXMLValue(psGetCapabilities,  "version", 
                                                 NULL);
                //version is optional is the GetCapabilities. If not
                //provided, set it.
                if (pszValue)
                  wfsparams->pszVersion = strdup(pszValue);
                else
                  wfsparams->pszVersion = strdup("1.0.0");

                 pszValue = 
                   (char*)CPLGetXMLValue(psGetCapabilities, "service", 
                                         NULL);
                if (pszValue)
                  wfsparams->pszService = strdup(pszValue);
            }//end of GetCapabilites
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
                    //free typname and filter. There may have been
                    //values if they were passed in the URL
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

            }//end of DescibeFeatureType
        }
    }
#endif
#endif
}


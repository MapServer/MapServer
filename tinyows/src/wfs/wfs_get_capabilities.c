/* 
  Copyright (c) <2007> <Barbara Philippot - Olivier Courtin>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE. 
*/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include "../ows/ows.h"


/*
 * Display what distributed computing platform is supported
 * And what entry point is for all operations
 * Assume that online_ressource figure in the metadata
 * Used for version 1.0.0
 */
static void wfs_get_capabilities_dcpt_100(const ows * o)
{
	assert(o != NULL);
	assert(o->metadata->online_resource != NULL);

	fprintf(o->output, "    <DCPType>\n");
	fprintf(o->output, "     <HTTP>\n");
	fprintf(o->output, "      <Get onlineResource=\"");
	fprintf(o->output, "%s", o->metadata->online_resource->buf);
	fprintf(o->output, "?\"/>\n");
	fprintf(o->output, "     </HTTP>\n");
	fprintf(o->output, "    </DCPType>\n");
	fprintf(o->output, "    <DCPType>\n");
	fprintf(o->output, "     <HTTP>\n");
	fprintf(o->output, "      <Post onlineResource=\"");
	fprintf(o->output, "%s", o->metadata->online_resource->buf);
	fprintf(o->output, "\"/>\n");
	fprintf(o->output, "     </HTTP>\n");
	fprintf(o->output, "    </DCPType>\n");
}


/*
 * Print a GML Object type passed in parameter
 * Used for version 1.1.0
 */
static void wfs_gml_object_type(ows * o, char *type)
{
	assert(o != NULL);
	assert(type != NULL);

	fprintf(o->output, "  <GMLObjectType>\n");
	fprintf(o->output, "   <Name>gml:%s</Name>\n", type);
	fprintf(o->output, "   <OutputFormats>\n");
	fprintf(o->output,
	   "    <Format>text/xml; subtype=gml/2.1.2</Format>\n");
	fprintf(o->output,
	   "    <Format>text/xml; subtype=gml/3.1.1</Format>\n");
	fprintf(o->output, "   </OutputFormats>\n");
	fprintf(o->output, "  </GMLObjectType>\n");
}


/* 
 * Defines the list of GML Object types that the WFS server would be capable
 * to serve.
 * Used for version 1.1.0
 */
static void wfs_gml_object_type_list(ows * o)
{
	assert(o != NULL);

	fprintf(o->output, " <SupportsGMLObjectTypeList>\n");
	wfs_gml_object_type(o, "AbstractGMLFeatureType");
	wfs_gml_object_type(o, "PointType");
	wfs_gml_object_type(o, "LineStringType");
	wfs_gml_object_type(o, "PolygonType");
	wfs_gml_object_type(o, "MultiPointType");
	wfs_gml_object_type(o, "MultiLineStringType");
	wfs_gml_object_type(o, "MultiPolygonType");
	fprintf(o->output, " </SupportsGMLObjectTypeList>\n");
}


/*
 * Specifies the list of requests that the WFS can handle
 * Used for version 1.0.0
 */
static void wfs_capability(ows * o)
{
	assert(o != NULL);

	fprintf(o->output, " <Capability>\n");
	fprintf(o->output, "  <Request>\n");
	fprintf(o->output, "   <GetCapabilities>\n");
	wfs_get_capabilities_dcpt_100(o);
	fprintf(o->output, "   </GetCapabilities>\n");
	fprintf(o->output, "   <DescribeFeatureType>\n");
	fprintf(o->output, "     <SchemaDescriptionLanguage>\n");
	fprintf(o->output, "        <XMLSCHEMA/>\n");
	fprintf(o->output, "     </SchemaDescriptionLanguage>\n");
	wfs_get_capabilities_dcpt_100(o);
	fprintf(o->output, "   </DescribeFeatureType>\n");
	fprintf(o->output, "   <GetFeature>\n");
	fprintf(o->output, "<ResultFormat>\n");
	fprintf(o->output, "<GML2/>\n");
	fprintf(o->output, "</ResultFormat>\n");
	wfs_get_capabilities_dcpt_100(o);
	fprintf(o->output, "   </GetFeature>\n");
	fprintf(o->output, "   <Transaction>\n");
	wfs_get_capabilities_dcpt_100(o);
	fprintf(o->output, "   </Transaction>\n");
	fprintf(o->output, "  </Request>\n");
	fprintf(o->output, " </Capability>\n");
}


/* 
 * Specifies the list of requests and its parameters that the WFS can handle
 * Used for version 1.1.0
 */
static void wfs_operations_metadata(ows * o)
{
	assert(o != NULL);

	fprintf(o->output, " <ows:OperationsMetadata>\n");

	fprintf(o->output, "  <ows:Operation name='GetCapabilities'>\n");
	ows_get_capabilities_dcpt(o);
	fprintf(o->output, "  <ows:Parameter name='AcceptVersions'>\n");
	fprintf(o->output, "  <ows:Value>1.1.0</ows:Value>\n");
	fprintf(o->output, "  <ows:Value>1.0.0</ows:Value>\n");
	fprintf(o->output, "  </ows:Parameter>\n");
	fprintf(o->output, "  <ows:Parameter name='AcceptFormats'>\n");
	fprintf(o->output, "  <ows:Value>text/xml</ows:Value>\n");
	fprintf(o->output, "  </ows:Parameter>\n");
	fprintf(o->output, "  <ows:Parameter name='Sections'>\n");
	fprintf(o->output, "  <ows:Value>ServiceIdentification</ows:Value>\n");
	fprintf(o->output, "  <ows:Value>ServiceProvider</ows:Value>\n");
	fprintf(o->output, "  <ows:Value>OperationsMetadata</ows:Value>\n");
	fprintf(o->output, "  <ows:Value>FeatureTypeList</ows:Value>\n");
	fprintf(o->output,
	   "  <ows:Value>ServesGMLObjectTypeList</ows:Value>\n");
	fprintf(o->output,
	   "  <ows:Value>SupportsGMLObjectTypeList</ows:Value>\n");
	fprintf(o->output, "  </ows:Parameter>\n");
	fprintf(o->output, "   </ows:Operation>\n");

	fprintf(o->output, "   <ows:Operation name='DescribeFeatureType'>\n");
	ows_get_capabilities_dcpt(o);
	fprintf(o->output, "  <ows:Parameter name='outputFormat'>\n");
	fprintf(o->output,
	   "  <ows:Value>text/xml; subtype=gml/3.1.1</ows:Value>\n");
	fprintf(o->output,
	   "  <ows:Value>text/xml; subtype=gml/2.1.2</ows:Value>\n");

	fprintf(o->output, "  </ows:Parameter>\n");
	fprintf(o->output, "   </ows:Operation>\n");

	fprintf(o->output, "   <ows:Operation name='GetFeature'>\n");
	ows_get_capabilities_dcpt(o);
	fprintf(o->output, "  <ows:Parameter name='resultType'>\n");
	fprintf(o->output, "  <ows:Value>results</ows:Value>\n");
	fprintf(o->output, "  <ows:Value>hits</ows:Value>\n");
	fprintf(o->output, "  </ows:Parameter>\n");
	fprintf(o->output, "   </ows:Operation>\n");

	fprintf(o->output, "   <ows:Operation name='Transaction'>\n");
	ows_get_capabilities_dcpt(o);
	fprintf(o->output, "   </ows:Operation>\n");

	fprintf(o->output, " </ows:OperationsMetadata>\n");
}


/*
 * Specifies the list of feature types available from the wfs
 * Used for both 1.0.0 && 1.1.0 versions
 */
static void wfs_feature_type_list(ows * o)
{
	ows_layer_node *ln;
	ows_geobbox *gb;

	buffer *srid;
	buffer *srs;
	list_node *keyword, *other_srid;
	int s;
	bool writable, retrievable;

	assert(o != NULL);

	srid = NULL;
	srs = NULL;
	writable = false;
	retrievable = false;

	fprintf(o->output, " <FeatureTypeList>\n");

	/* print global operations */
	fprintf(o->output, "  <Operations>\n");

	if (ows_layer_list_retrievable(o->layers))
	{
		if (ows_version_get(o->request->version) == 100)
			fprintf(o->output, "   <Query/>\n");
		else if (ows_version_get(o->request->version) == 110)
			fprintf(o->output, " <Operation>Query</Operation>\n");
		retrievable = true;
	}
	if (ows_layer_list_writable(o->layers))
	{
		if (ows_version_get(o->request->version) == 100)
		{
			fprintf(o->output, "   <Insert/>\n");
			fprintf(o->output, "   <Update/>\n");
			fprintf(o->output, "   <Delete/>\n");
		}
		else if (ows_version_get(o->request->version) == 110)
		{
			fprintf(o->output, "   <Operation>Insert</Operation>\n");
			fprintf(o->output, "   <Operation>Update</Operation>\n");
			fprintf(o->output, "   <Operation>Delete</Operation>\n");
		}
		writable = true;
	}

	fprintf(o->output, "  </Operations>\n");

	for (ln = o->layers->first; ln != NULL; ln = ln->next)
	{
		/* print each feature type */
		if (ows_layer_match_table(o, ln->layer->name))
		{

			fprintf(o->output, "<FeatureType>\n");

			/* name */
			if (ln->layer->name != NULL)
			{
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output, " <Name>");
				buffer_flush(ln->layer->name, o->output);
				fprintf(o->output, "</Name>\n");
			}

			/* title */
			if (ln->layer->title != NULL)
			{
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output, " <Title>");
				buffer_flush(ln->layer->title, o->output);
				fprintf(o->output, "</Title>\n");
			}

			/* abstract */
			if (ln->layer->abstract != NULL)
			{
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output, " <Abstract>");
				buffer_flush(ln->layer->abstract, o->output);
				fprintf(o->output, "</Abstract>\n");
			}

			/* keywords */
			if (ln->layer->keywords != NULL)
			{
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output, " <Keywords>");
				for (keyword = ln->layer->keywords->first; keyword != NULL;
				   keyword = keyword->next)
				{
					if (ows_version_get(o->request->version) == 100)
						fprintf(o->output, "%s,", keyword->value->buf);
					else if (ows_version_get(o->request->version) == 110)
					{
						fprintf(o->output, "  <Keyword>");
						fprintf(o->output, "%s", keyword->value->buf);
						fprintf(o->output, "  </Keyword>");
					}
				}
				if (ows_version_get(o->request->version) == 100)
					fprintf(o->output, "%s</Keywords>\n",
					   keyword->value->buf);
				else if (ows_version_get(o->request->version) == 110)
					fprintf(o->output, "</Keywords>\n");
			}

			/* SRS */
			srid = ows_srs_get_srid_from_layer(o, ln->layer->name);
			srs = ows_srs_get_from_a_srid(o, srid);
			if (srs->use != 0)
			{
				if (ows_version_get(o->request->version) == 100)
				{
					fprintf(o->output, " <SRS>");
					buffer_flush(srs, o->output);
					fprintf(o->output, "</SRS>\n");
				}
				else if (ows_version_get(o->request->version) == 110)
				{
					fprintf(o->output, " <DefaultSRS>");
					fprintf(o->output,
					   "http://www.opengis.net/gml/srs/epsg.xml#%s",
					   srid->buf);
					fprintf(o->output, "</DefaultSRS>\n");
					if (ln->layer->srid != NULL)
					{
						for (other_srid = ln->layer->srid->first;
						   other_srid != NULL;
						   other_srid = other_srid->next)
						{
							if (!buffer_cmp(srid, other_srid->value->buf))
							{
								fprintf(o->output, " <OtherSRS>");
								fprintf(o->output,
								   "http://www.opengis.net/gml/srs/epsg.xml#%s",
								   other_srid->value->buf);
								fprintf(o->output, "</OtherSRS>\n");
							}
						}
					}

				}
			}
			else
			{
				if (ows_version_get(o->request->version) == 100)
					fprintf(o->output, " <SRS></SRS>\n");
				else if (ows_version_get(o->request->version) == 110)
					fprintf(o->output, " <NoSRS/>");
			}

			/* operations */
			if (retrievable != ln->layer->retrievable
			   || writable != ln->layer->writable)
			{
				fprintf(o->output, "  <Operations>\n");
				if (retrievable == false && ln->layer->retrievable == true)
				{
					if (ows_version_get(o->request->version) == 100)
						fprintf(o->output, "   <Query/>\n");
					else if (ows_version_get(o->request->version) == 110)
						fprintf(o->output,
						   "   <Operation>Query</Operation>\n");
				}
				if (writable == false && ln->layer->writable == true)
				{
					if (ows_version_get(o->request->version) == 100)
					{
						fprintf(o->output, "   <Insert/>\n");
						fprintf(o->output, "   <Update/>\n");
						fprintf(o->output, "   <Delete/>\n");
					}
					else if (ows_version_get(o->request->version) == 110)
					{
						fprintf(o->output,
						   "   <Operation>Insert</Operation>\n");
						fprintf(o->output,
						   "   <Operation>Update</Operation>\n");
						fprintf(o->output,
						   "   <Operation>Delete</Operation>\n");
					}
				}
				fprintf(o->output, "  </Operations>\n");
			}

			/* boundaries */
			if (ln->layer->geobbox == NULL)
				gb = ows_geobbox_compute(o, ln->layer->name);
			else
			{
				gb = ows_geobbox_init();
				gb->west = ln->layer->geobbox->west;
				gb->east = ln->layer->geobbox->east;
				gb->south = ln->layer->geobbox->south;
				gb->north = ln->layer->geobbox->north;
			}

			for (s = 0; s < ln->layer->depth; s++)
				fprintf(o->output, " ");
			if (ows_version_get(o->request->version) == 100)
				fprintf(o->output, " <LatLongBoundingBox");
			else if (ows_version_get(o->request->version) == 110)
				fprintf(o->output, " <ows:WGS84BoundingBox>");

			assert(gb != NULL);
			if (gb->east != DBL_MIN)
			{
				if (ows_version_get(o->request->version) == 100)
				{
					if (gb->west < gb->east)
						fprintf(o->output, " minx='%f'", gb->west);
					else
						fprintf(o->output, " minx='%f'", gb->east);
					if (gb->north < gb->south)
						fprintf(o->output, " miny='%f'", gb->north);
					else
						fprintf(o->output, " miny='%f'", gb->south);
					if (gb->west < gb->east)
						fprintf(o->output, " maxx='%f'", gb->east);
					else
						fprintf(o->output, " maxx='%f'", gb->west);
					if (gb->north < gb->south)
						fprintf(o->output, " maxy='%f'", gb->south);
					else
						fprintf(o->output, " maxy='%f'", gb->north);
					fprintf(o->output, " />\n");
				}
				else if (ows_version_get(o->request->version) == 110)
				{
					fprintf(o->output,
					   " <ows:LowerCorner>%f %f</ows:LowerCorner>",
					   gb->west, gb->south);
					fprintf(o->output,
					   " <ows:UpperCorner>%f %f</ows:UpperCorner>",
					   gb->east, gb->north);
				}
			}
			else
			{
				if (ows_version_get(o->request->version) == 100)
				{
					fprintf(o->output, " minx='0'");
					fprintf(o->output, " miny='0'");
					fprintf(o->output, " maxx='0'");
					fprintf(o->output, " maxy='0'");
					fprintf(o->output, " />\n");
				}
				else if (ows_version_get(o->request->version) == 110)
				{
					fprintf(o->output,
					   " <ows:LowerCorner>0 0</ows:LowerCorner>");
					fprintf(o->output,
					   " <ows:UpperCorner>0 0</ows:UpperCorner>");
				}
			}
			if (ows_version_get(o->request->version) == 110)
				fprintf(o->output, " </ows:WGS84BoundingBox>\n");
			buffer_free(srid);
			buffer_free(srs);
			ows_geobbox_free(gb);

			fprintf(o->output, "</FeatureType>\n");
		}
	}

	fprintf(o->output, " </FeatureTypeList>\n");
}


/* 
 * Execute the wfs get capabilities 1.1.0 request
 * Valid against wfs_capabilities 1.1.0 schema
 */
static void wfs_get_capabilities_110(ows * o, wfs_request * wr)
{
	buffer *name;

	assert(o != NULL);
	assert(wr != NULL);

	/* check if metadata are given */
	if (o->metadata->type == NULL || o->metadata->versions == NULL)
		ows_error(o, OWS_ERROR_MISSING_METADATA,
		   "missing versions or type metadata in fileconf",
		   "GetCapabilities");

	if (wr->format == WFS_TEXT_XML)
		fprintf(o->output, "Content-Type: text/xml\n\n");
	else
		fprintf(o->output, "Content-Type: application/xml\n\n");
	fprintf(o->output, "<?xml version='1.0' encoding='UTF-8'?>\n");
	fprintf(o->output, "<WFS_Capabilities");
	fprintf(o->output, " version='1.1.0' updateSequence='0'\n");

	fprintf(o->output, "  xmlns='http://www.opengis.net/wfs'\n");
	fprintf(o->output,
	   "  xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'\n");
	fprintf(o->output, "  xmlns:ogc='http://www.opengis.net/ogc'\n");
	fprintf(o->output, "  xmlns:gml='http://www.opengis.net/gml'\n");
	fprintf(o->output, "  xmlns:ows='http://www.opengis.net/ows'\n");
	fprintf(o->output, "  xmlns:xlink='http://www.w3.org/1999/xlink'\n");
	fprintf(o->output, "  xsi:schemaLocation='http://www.opengis.net/wfs\n");
	fprintf(o->output, "  http://schemas.opengis.net/wfs/1.1.0/wfs.xsd' >\n");

	name = buffer_init();

	/* Service Identification Section : provides information about the 
       WFS service iself */
	buffer_add_str(name, "ServiceIdentification");
	if (wr->sections != NULL)
	{
		if (in_list(wr->sections, name)
		   || buffer_case_cmp(wr->sections->first->value, "all"))
			ows_service_identification(o);
	}
	else
		ows_service_identification(o);

	/* Service Provider Section : provides metadata about 
	   the organization operating the WFS server */
	buffer_empty(name);
	buffer_add_str(name, "ServiceProvider");
	if (wr->sections != NULL)
	{
		if (in_list(wr->sections, name)
		   || buffer_case_cmp(wr->sections->first->value, "all"))
			ows_service_provider(o);
	}
	else
		ows_service_provider(o);

	/* Operation Metadata Section : specifies the list of requests 
	   that the WFS can handle */
	buffer_empty(name);
	buffer_add_str(name, "OperationsMetadata");
	if (wr->sections != NULL)
	{
		if (in_list(wr->sections, name)
		   || buffer_case_cmp(wr->sections->first->value, "all"))
			wfs_operations_metadata(o);
	}
	else
		wfs_operations_metadata(o);


	/* FeatureType list Section : specifies the list of feature types 
	   available from the wfs */
	buffer_empty(name);
	buffer_add_str(name, "FeatureTypeList");
	if (wr->sections != NULL)
	{
		if (in_list(wr->sections, name)
		   || buffer_case_cmp(wr->sections->first->value, "all"))
			wfs_feature_type_list(o);
	}
	else
		wfs_feature_type_list(o);

	/* No ServesGMLObjectType list Section since there isn't any supported 
	   GML Object types not derived from gml:AbstractFeatureType */

	/* SupportsGMLObjectType list Section : defines the list of GML Object types
	   that the WFS server would be capable of serving */
	buffer_empty(name);
	buffer_add_str(name, "SupportsGMLObjectTypeList");
	if (wr->sections != NULL)
	{
		if (in_list(wr->sections, name)
		   || buffer_case_cmp(wr->sections->first->value, "all"))
			wfs_gml_object_type_list(o);
	}
	else
		wfs_gml_object_type_list(o);


	/* Filter Capabilities Section : describe what specific filter capabilties 
	   are supported by the wfs */
	fe_filter_capabilities_110(o);

	fprintf(o->output, "</WFS_Capabilities>\n");
	fclose(o->output);

	buffer_free(name);
}


/* 
 * Execute the wfs get capabilities request 1.0.0
 * Valid against schema wfs_capabilities 1.0.0
 */
static void wfs_get_capabilities_100(ows * o, wfs_request * wr)
{
	assert(o != NULL);
	assert(wr != NULL);

	/* check if metadata are given */
	if (o->metadata->name == NULL
	   || o->metadata->title == NULL
	   || o->metadata->online_resource == NULL)
		ows_error(o, OWS_ERROR_MISSING_METADATA,
		   "missing name or title or online_resource metadata in config file",
		   "GetCapabilities");

	fprintf(o->output, "Content-Type: application/xml\n\n");
	fprintf(o->output, "<?xml version='1.0' encoding='UTF-8'?>\n");
	fprintf(o->output, "<WFS_Capabilities\n");
	fprintf(o->output, "version='1.0.0' updateSequence='0'\n");
	fprintf(o->output, " xmlns='http://www.opengis.net/wfs'\n");
	fprintf(o->output,
	   " xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'\n");
	fprintf(o->output, "xmlns:ogc='http://www.opengis.net/ogc'");
	fprintf(o->output, " xsi:schemaLocation='http://www.opengis.net/wfs");
	fprintf(o->output, 
        " http://schemas.opengis.net/wfs/1.0.0/WFS-capabilities.xsd' >\n");

	/* Service Section : provides information about the service iself */
	ows_service_metadata(o);

	/* Capabilities Section : specifies the list of requests 
	   that the WFS can handle */
	wfs_capability(o);

	/* FeatureType list Section : specifies the list of feature types 
	   available from the wfs */
	wfs_feature_type_list(o);

	/* Filter Capabilities Section : describe what specific filter capabilties 
	   are supported by the wfs */
	fe_filter_capabilities_100(o);

	fprintf(o->output, "</WFS_Capabilities>\n");
	fclose(o->output);
}


/* 
 * Execute the wfs get capabilities request according to version
 */
void wfs_get_capabilities(ows * o, wfs_request * wr)
{
	int version;

	assert(o != NULL);
	assert(wr != NULL);

	version = ows_version_get(o->request->version);

	switch (version)
	{
	case 100:
		wfs_get_capabilities_100(o, wr);
		break;
	case 110:
		wfs_get_capabilities_110(o, wr);
		break;
	}
}


/*
 * vim: expandtab sw=4 ts=4
 */

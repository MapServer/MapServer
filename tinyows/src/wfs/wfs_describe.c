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

#include "../ows/ows.h"


/*
 * Describe the layer_name in GML according
 * with PostGIS table definition
 */
static void wfs_complex_type(ows * o, wfs_request * wr,
   buffer * layer_name)
{
	buffer *id_name;
	array *table;
	array_node *an;
	list *mandatory_prop;

	assert(o != NULL);
	assert(wr != NULL);
	assert(layer_name != NULL);

	mandatory_prop = ows_psql_not_null_properties(o, layer_name);

	assert(mandatory_prop != NULL);

	fprintf(o->output, "<xs:complexType name='");
	buffer_flush(layer_name, o->output);
	fprintf(o->output, "Type'>\n");
	fprintf(o->output, " <xs:complexContent>\n");
	fprintf(o->output,
	   "  <xs:extension base='gml:AbstractFeatureType'>\n");
	fprintf(o->output, "   <xs:sequence>\n");

	id_name = ows_psql_id_column(o, layer_name);
	table = ows_psql_describe_table(o, layer_name);

	assert(table != NULL);

	/* Output the description of the layer_name */
	for (an = table->first; an != NULL; an = an->next)
	{
		fprintf(o->output, "    <xs:element name ='");
		buffer_flush(an->key, o->output);
		fprintf(o->output, "' type='");
		fprintf(o->output, ows_psql_to_xsd(an->value));
		fprintf(o->output, "' ");
		if (in_list(mandatory_prop, an->key))
			fprintf(o->output, "minOccurs='1' ");
		else
			fprintf(o->output, "minOccurs='0' ");
		fprintf(o->output, "maxOccurs='1'/>\n");
	}

	fprintf(o->output, "   </xs:sequence>\n");
	fprintf(o->output, "  </xs:extension>\n");
	fprintf(o->output, " </xs:complexContent>\n");
	fprintf(o->output, "</xs:complexType>\n");

	array_free(table);
	buffer_free(id_name);
	list_free(mandatory_prop);
}


/* 
 * Execute the DescribeFeatureType request according to version
 * (GML version differ between WFS 1.0.0 and WFS 1.1.0)
 */
void wfs_describe_feature_type(ows * o, wfs_request * wr)
{
	int wfs_version;
	list_node *elemt, *ln;
	list *prefix, *typ;
	buffer *namespace;

	assert(o != NULL);
	assert(wr != NULL);

	wfs_version = ows_version_get(o->request->version);

	fprintf(o->output, "Content-Type: application/xml\n\n");
	fprintf(o->output, "<?xml version='1.0' encoding='UTF-8'?>\n");
	prefix = ows_layer_list_prefix(o->layers, wr->typename);

	/* if all layers belong to different prefixes, import the matching namespaces */
	if (prefix->first->next != NULL)
	{
		fprintf(o->output,
		   "<xs:schema xmlns:xs='http://www.w3.org/2001/XMLSchema' xmlns='http://www.w3.org/2001/XMLSchema' ");
		fprintf(o->output, "elementFormDefault='qualified'> ");

		for (elemt = prefix->first; elemt != NULL; elemt = elemt->next)
		{
			namespace = ows_layer_server(o->layers, elemt->value);
			fprintf(o->output, "<xs:import namespace='%s' ",
			   namespace->buf);
			fprintf(o->output, "schemaLocation='%s?service=WFS&amp;",
			   o->metadata->online_resource->buf);
			if (wfs_version == 100)
				fprintf(o->output,
				   "version=1.0.0&amp;request=DescribeFeatureType&amp;typeName=");
			else
				fprintf(o->output,
				   "version=1.1.0&amp;request=DescribeFeatureType&amp;typeName=");
			/* print the describeFeatureType request with typenames for each prefix */
			typ =
			   ows_layer_list_by_prefix(o->layers, wr->typename,
			   elemt->value);
			for (ln = typ->first; ln != NULL; ln = ln->next)
			{
				fprintf(o->output, "%s", ln->value->buf);
				if (ln->next != NULL)
					fprintf(o->output, ",");
			}
			list_free(typ);
			fprintf(o->output, "' />\n\n");
		}
		fprintf(o->output, "</xs:schema>\n");
	}
	/* if all layers belong to the same prefix, print the xsd schema describing features */
	else
	{
		namespace = ows_layer_server(o->layers, prefix->first->value);
		fprintf(o->output,
		   "<xs:schema targetNamespace='%s' ", namespace->buf);
		fprintf(o->output,
		   "xmlns:%s='%s' ", prefix->first->value->buf, namespace->buf);
		fprintf(o->output, "xmlns:ogc='http://www.opengis.net/ogc' ");
		fprintf(o->output, "xmlns:xs='http://www.w3.org/2001/XMLSchema' ");
		fprintf(o->output, "xmlns='http://www.w3.org/2001/XMLSchema' ");
		fprintf(o->output, "xmlns:gml='http://www.opengis.net/gml' ");
		fprintf(o->output, "elementFormDefault='qualified' ");

		if (wfs_version == 100)
			fprintf(o->output, "version='1.0'>\n");
		else
			fprintf(o->output, "version='1.1'>\n");

		fprintf(o->output,
		   "<xs:import namespace='http://www.opengis.net/gml'");

		if (wfs_version == 100)
			fprintf(o->output,
			   " schemaLocation='http://schemas.opengis.net/gml/2.1.2/feature.xsd'/>\n");
		else
			fprintf(o->output,
			   " schemaLocation='http://schemas.opengis.net/gml/3.1.1/base/gml.xsd'/>\n");

		/* Describe each feature type specified in the request */
		for (elemt = wr->typename->first; elemt != NULL;
		   elemt = elemt->next)

		{
			fprintf(o->output, "<xs:element name='");
			buffer_flush(elemt->value, o->output);
			fprintf(o->output, "' type='%s:", prefix->first->value->buf);
			buffer_flush(elemt->value, o->output);
			fprintf(o->output,
			   "Type' substitutionGroup='gml:_Feature' />\n");
			wfs_complex_type(o, wr, elemt->value);
		}

		fprintf(o->output, "</xs:schema>");
	}

	list_free(prefix);
}


/*
 * vim: expandtab sw=4 ts=4
 */

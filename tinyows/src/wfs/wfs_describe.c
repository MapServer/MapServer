/*
  Copyright (c) <2007-2012> <Barbara Philippot - Olivier Courtin>

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
#include <string.h>
#include <assert.h>

#include "../ows/ows.h"


/*
 * Describe the layer_name in GML according
 * with PostGIS table definition
 */
static void wfs_complex_type(ows * o, wfs_request * wr, buffer * layer_name)
{
    buffer *id_name;
    array *table;
    array_node *an;
    list *mandatory_prop;

    assert(o);
    assert(wr);
    assert(layer_name);

    mandatory_prop = ows_psql_not_null_properties(o, layer_name);

    fprintf(o->output, "<xs:complexType name='");
    buffer_flush(layer_name, o->output);
    fprintf(o->output, "Type'>\n");
    fprintf(o->output, " <xs:complexContent>\n");
    fprintf(o->output, "  <xs:extension base='gml:AbstractFeatureType'>\n");
    fprintf(o->output, "   <xs:sequence>\n");

    table = ows_psql_describe_table(o, layer_name);
    id_name = ows_psql_id_column(o, layer_name);

    assert(table);

    /* Output the description of the layer_name */
    for (an = table->first ; an ; an = an->next) {

         /* Handle GML namespace */
         if (            (ows_layer_get(o->layers, layer_name))->gml_ns 
              && in_list((ows_layer_get(o->layers, layer_name))->gml_ns, an->key)) {

              if (    !strcmp("name", an->key->buf)
                   || !strcmp("description", an->key->buf) 
                   || !strcmp("boundedBy", an->key->buf)) { continue; }

               fprintf(o->output, "    <xs:element ref='gml:%s'/>\n", an->key->buf);
               continue;
         }

         /* Avoid to expose PK if not specificaly wanted */
         if (id_name && buffer_cmp(an->key, id_name->buf) && !o->expose_pk) { continue; }

         fprintf(o->output, "    <xs:element name ='%s' type='%s' ",
             an->key->buf, ows_psql_to_xsd(an->value, o->request->version));

         if (mandatory_prop && in_list(mandatory_prop, an->key))
             fprintf(o->output, "nillable='false' minOccurs='1' ");
         else
             fprintf(o->output, "nillable='true' minOccurs='0' ");
	
      	 fprintf(o->output, "maxOccurs='1'/>\n");
    }

    fprintf(o->output, "   </xs:sequence>\n");
    fprintf(o->output, "  </xs:extension>\n");
    fprintf(o->output, " </xs:complexContent>\n");
    fprintf(o->output, "</xs:complexType>\n");
}


/*
 * Execute the DescribeFeatureType request according to version
 * (GML version differ between WFS 1.0.0 and WFS 1.1.0)
 */
void wfs_describe_feature_type(ows * o, wfs_request * wr)
{
    int wfs_version;
    list_node *elemt, *ln;
    list *ns_prefix, *typ;
    buffer *namespace;

    assert(o);
    assert(wr);

    wfs_version = ows_version_get(o->request->version);
    ns_prefix = ows_layer_list_ns_prefix(o->layers, wr->typename);
    if (!ns_prefix->first) {
            list_free(ns_prefix);
            ows_error(o, OWS_ERROR_CONFIG_FILE,
                    "Not a single layer is available. Check config file", "describe");
            return;
    }

         if (wr->format == WFS_GML212 || wr->format == WFS_XML_SCHEMA)
    	fprintf(o->output, "Content-Type: text/xml; subtype=gml/2.1.2;\n\n");
    else if (wr->format == WFS_GML311)
    	fprintf(o->output, "Content-Type: text/xml; subtype=gml/3.1.1;\n\n");

    fprintf(o->output, "<?xml version='1.0' encoding='%s'?>\n", o->encoding->buf);

    /* if all layers belong to different prefixes, import the matching namespaces */
    if (ns_prefix->first->next) {
        fprintf(o->output, "<xs:schema xmlns:xs='http://www.w3.org/2001/XMLSchema'");
        fprintf(o->output, " xmlns='http://www.w3.org/2001/XMLSchema'");
        fprintf(o->output, " elementFormDefault='qualified'> ");

        for (elemt = ns_prefix->first ; elemt ; elemt = elemt->next) {
            namespace = ows_layer_ns_uri(o->layers, elemt->value);
            fprintf(o->output, "<xs:import namespace='%s' ", namespace->buf);
            fprintf(o->output, "schemaLocation='%s?service=WFS&amp;version=",
                    o->online_resource->buf);

            if (wfs_version == 100)
                fprintf(o->output, "1.0.0&amp;request=DescribeFeatureType&amp;typename=");
            else
                fprintf(o->output, "1.1.0&amp;request=DescribeFeatureType&amp;typename=");

            /* print the describeFeatureType request with typenames for each prefix */
            typ = ows_layer_list_by_ns_prefix(o->layers, wr->typename, elemt->value);

            for (ln = typ->first ; ln ; ln = ln->next) {
                fprintf(o->output, "%s", ln->value->buf);

                if (ln->next) fprintf(o->output, ",");
            }

            list_free(typ);
            fprintf(o->output, "' />\n\n");
        }

        fprintf(o->output, "</xs:schema>\n");
    }
    /* if all layers belong to the same prefix, print the xsd schema describing features */
    else {
        namespace = ows_layer_ns_uri(o->layers, ns_prefix->first->value);
        fprintf(o->output, "<xs:schema targetNamespace='%s' ", namespace->buf);
        fprintf(o->output, "xmlns:%s='%s' ", ns_prefix->first->value->buf, namespace->buf);
        fprintf(o->output, "xmlns:ogc='http://www.opengis.net/ogc' ");
        fprintf(o->output, "xmlns:xs='http://www.w3.org/2001/XMLSchema' ");
        fprintf(o->output, "xmlns='http://www.w3.org/2001/XMLSchema' ");
        fprintf(o->output, "xmlns:gml='http://www.opengis.net/gml' ");
        fprintf(o->output, "elementFormDefault='qualified' ");

        if (wfs_version == 100) fprintf(o->output, "version='1.0'>\n");
        else                    fprintf(o->output, "version='1.1'>\n");

        fprintf(o->output, "<xs:import namespace='http://www.opengis.net/gml'");

        if (wfs_version == 100)
            fprintf(o->output, " schemaLocation='http://schemas.opengis.net/gml/2.1.2/feature.xsd'/>\n");
        else
            fprintf(o->output, " schemaLocation='http://schemas.opengis.net/gml/3.1.1/base/gml.xsd'/>\n");

        /* Describe each feature type specified in the request */
        for (elemt = wr->typename->first ; elemt ; elemt = elemt->next)
        {
            fprintf(o->output, "<xs:element name='");
            buffer_flush(elemt->value, o->output);
            fprintf(o->output, "' type='%s:", ns_prefix->first->value->buf);
            buffer_flush(elemt->value, o->output);
            fprintf(o->output, "Type' substitutionGroup='gml:_Feature' />\n");
            wfs_complex_type(o, wr, elemt->value);
        }

        fprintf(o->output, "</xs:schema>");
    }

    list_free(ns_prefix);
}


/*
 * Generate a WFS Schema related to current Server (all valid layers)
 * with GML XSD import 
 * This is needed by WFS Insert operation validation as libxml2 only handle
 * a single schema validation at a time
 */
buffer * wfs_generate_schema(ows * o, ows_version * version)
{
    list_node *elemt, *t;
    list *ns_prefix, *typename;
    buffer *namespace;
    buffer *schema;
    list * layers;
    int wfs_version;

    assert(o);
    assert(version);

    wfs_version = ows_version_get(version);
    schema = buffer_init();
    layers = ows_layer_list_having_storage(o->layers);

    buffer_add_str(schema, "<?xml version='1.0' encoding='utf-8'?>\n");

    ns_prefix = ows_layer_list_ns_prefix(o->layers, layers);

    buffer_add_str(schema, "<xs:schema xmlns:xs='http://www.w3.org/2001/XMLSchema'");
    buffer_add_str(schema, " xmlns='http://www.w3.org/2001/XMLSchema' elementFormDefault='qualified'>\n");
    buffer_add_str(schema, "<xs:import namespace='http://www.opengis.net/wfs' schemaLocation='");
    buffer_copy(schema, o->schema_dir);

    if (wfs_version == 100) buffer_add_str(schema, WFS_SCHEMA_100);
    else                    buffer_add_str(schema, WFS_SCHEMA_110);

    buffer_add_str(schema, "'/>\n");

    for (elemt = ns_prefix->first ; elemt ; elemt = elemt->next) {
        namespace = ows_layer_ns_uri(o->layers, elemt->value);
        buffer_add_str(schema, "<xs:import namespace='");
        buffer_copy(schema, namespace);
        buffer_add_str(schema, "' schemaLocation='");

        buffer_copy(schema, o->online_resource);
        buffer_add_str(schema, "?service=WFS&amp;request=DescribeFeatureType");

        if (elemt->next || elemt != ns_prefix->first) {
            buffer_add_str(schema, "&amp;Typename=");

            typename = ows_layer_list_by_ns_prefix(o->layers, layers, elemt->value);
            for (t = typename->first ; t ; t = t->next) {
        	buffer_copy(schema, t->value);
	  	if (t->next) buffer_add(schema, ',');
	    }
            list_free(typename);
        } 

        if (wfs_version == 100) buffer_add_str(schema, "&amp;version=1.0.0");
        else                    buffer_add_str(schema, "&amp;version=1.1.0");

        buffer_add_str(schema, "'/>\n");
    }

    buffer_add_str(schema, "</xs:schema>");
    list_free(ns_prefix);
    list_free(layers);

    return schema;
}


/*
 * vim: expandtab sw=4 ts=4
 */

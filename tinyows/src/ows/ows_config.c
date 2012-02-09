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


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libxml/xmlreader.h>

#include "ows.h"


/*
 * Parse the configuration file's tinyows element
 */
static void ows_parse_config_tinyows(ows * o, xmlTextReaderPtr r)
{
    xmlChar *a;
    int precision, log_level;

    assert(o);
    assert(r);

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "online_resource");
    if (a) {
        buffer_add_str(o->online_resource, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "schema_dir");
    if (a) {
        buffer_add_str(o->schema_dir, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "log");
    if (a) {
        o->log_file = buffer_init();
        buffer_add_str(o->log_file, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "log_level");
    if (a) {
        log_level = atoi((char *) a);
        if (log_level > 0 && log_level < 16) o->log_level = log_level;
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "degree_precision");
    if (a) {
        precision = atoi((char *) a);
        if (precision > 0 && precision < 12) o->degree_precision = precision;
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "meter_precision");
    if (a) {
        precision = atoi((char *) a);
        if (precision > 0 && precision < 12) o->meter_precision = precision;
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "display_bbox");
    if (a) {
        if (!atoi((char *) a)) o->display_bbox = false;
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "estimated_extent");
    if (a) {
        if (atoi((char *) a)) o->estimated_extent = true;
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "check_schema");
    if (a) {
        if (!atoi((char *) a)) o->check_schema = false;
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "check_valid_geom");
    if (a) {
        if (!atoi((char *) a)) o->check_valid_geom = false;
        xmlFree(a);
    }
    
    a = xmlTextReaderGetAttribute(r, (xmlChar *) "encoding");
    if (a) {
        buffer_add_str(o->encoding, (char *) a);
        xmlFree(a);
    } else buffer_add_str(o->encoding, OWS_DEFAULT_XML_ENCODING);

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "expose_pk");
    if (a) {
        if (atoi((char *) a)) o->expose_pk = true;
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "wfs_default_version");
    if (a) {
        ows_version_set_str(o->wfs_default_version, (char *) a);
        xmlFree(a);
    }
}


/*
 * Parse the configuration file's contact element
 */
static void ows_parse_config_contact(ows * o, xmlTextReaderPtr r)
{
    xmlChar *a;
    ows_contact *contact = ows_contact_init();

    assert(o);
    assert(r);

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "name");
    if (a) {
        contact->name = buffer_init();
        buffer_add_str(contact->name, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "site");
    if (a) {
        contact->site = buffer_init();
        buffer_add_str(contact->site, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "individual_name");
    if (a) {
        contact->indiv_name = buffer_init();
        buffer_add_str(contact->indiv_name, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "position");
    if (a) {
        contact->position = buffer_init();
        buffer_add_str(contact->position, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "phone");
    if (a) {
        contact->phone = buffer_init();
        buffer_add_str(contact->phone, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "fax");
    if (a) {
        contact->fax = buffer_init();
        buffer_add_str(contact->fax, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "online_resource");
    if (a) {
        contact->online_resource = buffer_init();
        buffer_add_str(contact->online_resource, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "address");
    if (a) {
        contact->address = buffer_init();
        buffer_add_str(contact->address, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "postcode");
    if (a) {
        contact->postcode = buffer_init();
        buffer_add_str(contact->postcode, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "city");
    if (a) {
        contact->city = buffer_init();
        buffer_add_str(contact->city, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "administrative_area");
    if (a) {
        contact->state = buffer_init();
        buffer_add_str(contact->state, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "country");
    if (a) {
        contact->country = buffer_init();
        buffer_add_str(contact->country, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "email");
    if (a) {
        contact->email = buffer_init();
        buffer_add_str(contact->email, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "hours_of_service");
    if (a) {
        contact->hours = buffer_init();
        buffer_add_str(contact->hours, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "contact_instructions");
    if (a) {
        contact->instructions = buffer_init();
        buffer_add_str(contact->instructions, (char *) a);
        xmlFree(a);
    }

    o->contact = contact;
}


/*
 * Parse the configuration file's metadata element
 */
static void ows_parse_config_metadata(ows * o, xmlTextReaderPtr r)
{
    xmlChar *a;

    assert(o);
    assert(r);

    o->metadata = ows_metadata_init();

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "name");
    if (a) {
        o->metadata->name = buffer_init();
        buffer_add_str(o->metadata->name, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "title");
    if (a) {
        o->metadata->title = buffer_init();
        buffer_add_str(o->metadata->title, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "keywords");
    if (a) {
        o->metadata->keywords = list_explode_str(',', (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "fees");
    if (a) {
        o->metadata->fees = buffer_init();
        buffer_add_str(o->metadata->fees, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "access_constraints");
    if (a) {
        o->metadata->access_constraints = buffer_init();
        buffer_add_str(o->metadata->access_constraints, (char *) a);
        xmlFree(a);
    }
}


/*
 * Parse the configuration file's abstract metadata element
 */
static void ows_parse_config_abstract(ows * o, xmlTextReaderPtr r)
{
    xmlChar *v;

    assert(r);
    assert(o);
    assert(o->metadata);

    /* FIXME should use XmlTextReader expand on metadata parent */
    xmlTextReaderRead(r);
    v = xmlTextReaderValue(r);

    if (v) {
        o->metadata->abstract = buffer_init();
        buffer_add_str(o->metadata->abstract, (char *) v);
        xmlFree(v);
    }
}


/*
 * Parse the configuration file's limits element
 */
static void ows_parse_config_limits(ows * o, xmlTextReaderPtr r)
{
    xmlChar *a;
    ows_geobbox *geo;

    assert(o);
    assert(r);

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "features");
    if (a) {
        o->max_features = atoi((char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "geobbox");
    if (a) {
        geo = ows_geobbox_init();
        if (ows_geobbox_set_from_str(o, geo, (char *) a)) o->max_geobbox = geo;
        else ows_geobbox_free(geo);
        xmlFree(a);
    }
}


/*
 * Parse the configuration file's pg element about connection information
 */
static void ows_parse_config_pg(ows * o, xmlTextReaderPtr r)
{
    xmlChar *a, *v;

    assert(o);
    assert(r);

    if (xmlTextReaderMoveToFirstAttribute(r) != 1) return;
    do {
        a = xmlTextReaderName(r);

        if (       !strcmp((char *) a, "host")
                || !strcmp((char *) a, "user")
                || !strcmp((char *) a, "password")
                || !strcmp((char *) a, "dbname")
                || !strcmp((char *) a, "port")) {
            v = xmlTextReaderValue(r);
            buffer_add_str(o->pg_dsn, (char *) a);
            buffer_add_str(o->pg_dsn, "=");
            buffer_add_str(o->pg_dsn, (char *) v);
            buffer_add_str(o->pg_dsn, " ");
            xmlFree(v);
        } else if (!strcmp((char *) a, "encoding")) { 
            v = xmlTextReaderValue(r); 
            buffer_add_str(o->db_encoding, (char *) v); 
            xmlFree(v); 
        }

        xmlFree(a);
    } while (xmlTextReaderMoveToNextAttribute(r) == 1);

    if (!o->db_encoding->use)
        buffer_add_str(o->db_encoding, OWS_DEFAULT_DB_ENCODING);
}


/*
 * Return layer's parent if there is one
 */
static ows_layer *ows_parse_config_layer_get_parent(const ows * o, int depth)
{
    ows_layer_node *ln;

    if (depth == 1) return (ows_layer *) NULL;

    ln = o->layers->last;

    if (ln->layer->depth < depth)  return ln->layer;
    if (ln->layer->depth == depth) return ln->layer->parent;

    for (/* empty */ ; ln ; ln = ln->prev)
        if (depth == ln->layer->depth)
            return ln->layer->parent;

    return (ows_layer *) NULL;
}


/*
 * Parse the configuration file's layer element and all child layers
 */
static void ows_parse_config_layer(ows * o, xmlTextReaderPtr r)
{
    ows_layer *layer;
    xmlChar *a;
    list *l;

    assert(o);
    assert(r);

    layer = ows_layer_init();

    layer->depth = xmlTextReaderDepth(r);
    layer->parent = ows_parse_config_layer_get_parent(o, layer->depth);

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "table");
    if (a) {
        buffer_add_str(layer->storage->table, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "name");
    if (a) {
        layer->name = buffer_init();
        buffer_add_str(layer->name, (char *) a);

	if (!layer->storage->table->use) buffer_add_str(layer->storage->table, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "title");
    if (a) {
        layer->title = buffer_init();
        buffer_add_str(layer->title, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "abstract");
    if (a) {
        layer->abstract = buffer_init();
        buffer_add_str(layer->abstract, (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "keywords");
    if (a) {
        layer->keywords = list_explode_str(',', (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "gml_ns");
    if (a) {
        layer->gml_ns = list_explode_str(',', (char *) a);
        xmlFree(a);
    }

    a = xmlTextReaderGetAttribute(r, (xmlChar *) "schema");
    if (a) {
        buffer_add_str(layer->storage->schema, (char *) a);
        xmlFree(a);
    } else buffer_add_str(layer->storage->schema, "public");

    /* inherits from layer parent and replaces with specified value if defined */
    a = xmlTextReaderGetAttribute(r, (xmlChar *) "retrievable");
    if (a && atoi((char *) a) == 1) {
        layer->retrievable = true;
        xmlFree(a);
    } else if (!a && layer->parent && layer->parent->retrievable)
        layer->retrievable = true;
    else
        xmlFree(a);

    /* inherits from layer parent and replaces with specified value if defined */
    a = xmlTextReaderGetAttribute(r, (xmlChar *) "writable");
    if (a && atoi((char *) a) == 1) {
        layer->writable = true;
        xmlFree(a);
    } else if (!a && layer->parent && layer->parent->writable)
        layer->writable = true;
    else xmlFree(a);

    /* inherits from layer parent and adds specified value */
    if (layer->parent && layer->parent->srid) {
        layer->srid = list_init();
        list_add_list(layer->srid, layer->parent->srid);
    }
    a = xmlTextReaderGetAttribute(r, (xmlChar *) "srid");
    if (a) {
        if (!layer->srid)
		layer->srid = list_explode_str(',', (char *) a);
	else {
        	l = list_explode_str(',', (char *) a);
        	list_add_list(layer->srid, l);
        	list_free(l);
	}
        xmlFree(a);
    }

    /* Inherits from layer parent and replaces with specified value if defined */
    a = xmlTextReaderGetAttribute(r, (xmlChar *) "geobbox");
    if (a) {
        layer->geobbox = ows_geobbox_init();
        ows_geobbox_set_from_str(o, layer->geobbox, (char *) a);
        xmlFree(a);
    } else if (!a && layer->parent && layer->parent->geobbox) {
        layer->geobbox = ows_geobbox_copy(layer->parent->geobbox);
    } else xmlFree(a);

    /* Inherits from layer parent and replaces with specified value if defined */
    a = xmlTextReaderGetAttribute(r, (xmlChar *) "ns_prefix");
    if (a) {
        buffer_add_str(layer->ns_prefix, (char *) a);
        xmlFree(a);
    } else if (!a && layer->parent && layer->parent->ns_prefix) {
        buffer_copy(layer->ns_prefix, layer->parent->ns_prefix);
    } else xmlFree(a);

    /* Inherits from layer parent and replaces with specified value if defined */
    a = xmlTextReaderGetAttribute(r, (xmlChar *) "ns_uri");
    if (a) {
        buffer_add_str(layer->ns_uri, (char *) a);
        xmlFree(a);
    } else if (!a && layer->parent && layer->parent->ns_uri) {
        buffer_copy(layer->ns_uri, layer->parent->ns_uri);
    } else xmlFree(a);

    ows_layer_list_add(o->layers, layer);
}


/*
 * Check if every mandatory element/property are
 * rightly set from config files
 */
static void ows_config_check(ows * o)
{
    if (!o->online_resource->use) {
        ows_error(o, OWS_ERROR_CONFIG_FILE, "No 'online_resource' property in tinyows element",
                  "parse_config_file");
        return;
    }

    if (!ows_version_check(o->wfs_default_version)) {
        ows_error(o, OWS_ERROR_CONFIG_FILE, "WFS Default version is not correct.", "parse_config_file");
        return;
    }

    if (!o->schema_dir->use) {
        ows_error(o, OWS_ERROR_CONFIG_FILE, "No 'schema_dir' property in tinyows element",
                  "parse_config_file");
        return;
    }

    if (!o->metadata) {
        ows_error(o, OWS_ERROR_CONFIG_FILE, "No 'metadata' element", "parse_config_file");
        return;
    }

    if (!o->metadata->name) {
        ows_error(o, OWS_ERROR_CONFIG_FILE, "No 'name' property in metadata element",
                  "parse_config_file");
        return;
    }

    if (!o->metadata->title) {
        ows_error(o, OWS_ERROR_CONFIG_FILE, "No 'title' property in metadata element",
                  "parse_config_file");
        return;
    }

    if (!o->pg_dsn->use) { ows_error(o, OWS_ERROR_CONFIG_FILE, "No 'pg' element",
                  "parse_config_file");
        return;
    }
}


/*
 * Parse the configuration file and initialize ows struct
 */
static void ows_parse_config_xml(ows * o, const char *filename)
{
    xmlTextReaderPtr r;
    const xmlChar *name;
    int ret;

    assert(o);
    assert(filename);

    r = xmlReaderForFile(filename, "UTF-8", 0);
    if (!r) {
        ows_error(o, OWS_ERROR_CONFIG_FILE, "Unable to open config file !", "parse_config_file");
        return;
    }

    if (!o->layers) o->layers = ows_layer_list_init();

    while ((ret = xmlTextReaderRead(r)) == 1) {
        if (xmlTextReaderNodeType(r) == XML_READER_TYPE_ELEMENT) {
            name = xmlTextReaderConstLocalName(r);

            if (!strcmp((char *) name, "tinyows"))
                ows_parse_config_tinyows(o, r);

            if (!strcmp((char *) name, "metadata"))
                ows_parse_config_metadata(o, r);

            if (!strcmp((char *) name, "abstract"))
                ows_parse_config_abstract(o, r);

            if (!strcmp((char *) name, "contact"))
                ows_parse_config_contact(o, r);

            if (!strcmp((char *) name, "pg"))
                ows_parse_config_pg(o, r);

            if (!strcmp((char *) name, "limits"))
                ows_parse_config_limits(o, r);

            if (!strcmp((char *) name, "layer"))
                ows_parse_config_layer(o, r);
        }
    }

    if (ret != 0) {
        xmlFreeTextReader(r);
        ows_error(o, OWS_ERROR_CONFIG_FILE, "Unable to open config file !", "parse_config_file");
        return;
    }

    xmlFreeTextReader(r);
}



/*
 * 
 */
void ows_parse_config(ows * o, const char *filename)
{
    assert(o);
    assert(filename);

    if (o->mapfile) ows_parse_config_mapfile(o, filename);
    else	    ows_parse_config_xml(o, filename);
    if (!o->exit) ows_config_check(o);
}


/*
 * vim: expandtab sw=4 ts=4
 */

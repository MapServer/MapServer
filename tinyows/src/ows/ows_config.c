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


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libxml/xmlreader.h>

#include "ows.h"


/* 
 * Parse the configuration file's contact element
 */
void ows_parse_config_contact(ows * o, xmlTextReaderPtr r)
{
	xmlChar *a;
	buffer *b;
	ows_contact *contact;

	assert(o != NULL);
	assert(r != NULL);

	contact = ows_contact_init();
	b = buffer_init();

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "name");
	if (a != NULL)
	{
		contact->name = buffer_init();
		buffer_add_str(contact->name, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "site");
	if (a != NULL)
	{
		contact->site = buffer_init();
		buffer_add_str(contact->site, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "individual_name");
	if (a != NULL)
	{
		contact->indiv_name = buffer_init();
		buffer_add_str(contact->indiv_name, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "position");
	if (a != NULL)
	{
		contact->position = buffer_init();
		buffer_add_str(contact->position, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "phone");
	if (a != NULL)
	{
		contact->phone = buffer_init();
		buffer_add_str(contact->phone, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "fax");
	if (a != NULL)
	{
		contact->fax = buffer_init();
		buffer_add_str(contact->fax, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "online_resource");
	if (a != NULL)
	{
		contact->online_resource = buffer_init();
		buffer_add_str(contact->online_resource, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "address");
	if (a != NULL)
	{
		contact->address = buffer_init();
		buffer_add_str(contact->address, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "postcode");
	if (a != NULL)
	{
		contact->postcode = buffer_init();
		buffer_add_str(contact->postcode, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "city");
	if (a != NULL)
	{
		contact->city = buffer_init();
		buffer_add_str(contact->city, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "administrative_area");
	if (a != NULL)
	{
		contact->state = buffer_init();
		buffer_add_str(contact->state, (char *) a);
		free(a);
	}


	a = xmlTextReaderGetAttribute(r, (xmlChar *) "country");
	if (a != NULL)
	{
		contact->country = buffer_init();
		buffer_add_str(contact->country, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "email");
	if (a != NULL)
	{
		contact->email = buffer_init();
		buffer_add_str(contact->email, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "hours_of_service");
	if (a != NULL)
	{
		contact->hours = buffer_init();
		buffer_add_str(contact->hours, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "contact_instructions");
	if (a != NULL)
	{
		contact->instructions = buffer_init();
		buffer_add_str(contact->instructions, (char *) a);
		free(a);
	}

	o->contact = contact;

	buffer_free(b);
}


/* 
 * Parse the configuration file's limits element
 */
void ows_parse_config_limits(ows * o, xmlTextReaderPtr r)
{
	xmlChar *a;
	ows_geobbox *geo;

	assert(o != NULL);
	assert(r != NULL);

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "width");
	if (a != NULL)
	{
		o->max_width = atoi((char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "height");
	if (a != NULL)
	{
		o->max_height = atoi((char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "layers");
	if (a != NULL)
	{
		o->max_layers = atoi((char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "features");
	if (a != NULL)
	{
		o->max_features = atoi((char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "geobbox");
	if (a != NULL)
	{
		geo = ows_geobbox_init();
		if (ows_geobbox_set_from_str(o, geo, (char *) a))
			o->max_geobbox = geo;
		else
			ows_geobbox_free(geo);
		free(a);
	}
}


/* 
 * Parse the configuration file's pg element about connection information
 */
void ows_parse_config_pg(ows * o, xmlTextReaderPtr r)
{
	xmlChar *a;
	xmlChar *v;
	buffer *b;

	assert(o != NULL);
	assert(r != NULL);

	b = buffer_init();

	if (xmlTextReaderMoveToFirstAttribute(r) != 1)
		return;

	do
	{
		a = xmlTextReaderName(r);
		if (strcmp((char *) a, "host") == 0
		   || strcmp((char *) a, "user") == 0
		   || strcmp((char *) a, "password") == 0
		   || strcmp((char *) a, "dbname") == 0
		   || strcmp((char *) a, "port") == 0)
		{
			buffer_add_str(b, (char *) a);
			buffer_add_str(b, "=");
			v = xmlTextReaderValue(r);
			buffer_add_str(b, (char *) v);
			buffer_add_str(b, " ");
			free(v);
		}
		free(a);
	}
	while (xmlTextReaderMoveToNextAttribute(r) == 1);

	o->pg_dsn = b;
}


/* 
 * Return layer's parent if there is one
 */
ows_layer *ows_parse_config_layer_get_parent(const ows * o, int depth)
{
	ows_layer_node *ln;

	if (depth == 1)
		return (ows_layer *) NULL;

	ln = o->layers->last;

	if (ln->layer->depth < depth)
		return ln->layer;

	if (ln->layer->depth == depth)
		return ln->layer->parent;

	for ( /* empty */ ; ln != NULL; ln = ln->prev)
		if (depth == ln->layer->depth)
			return ln->layer->parent;

	return (ows_layer *) NULL;
}


/* 
 * Parse the configuration file's layer element and all child layers
 */
void ows_parse_config_layer(ows * o, xmlTextReaderPtr r)
{
	ows_layer *layer;
	buffer *b;
	xmlChar *a;
	list *l;

	assert(o != NULL);
	assert(r != NULL);

	layer = ows_layer_init();

	layer->depth = xmlTextReaderDepth(r);
	layer->parent = ows_parse_config_layer_get_parent(o, layer->depth);

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "name");
	if (a != NULL)
	{
		layer->name = buffer_init();
		buffer_add_str(layer->name, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "title");
	if (a != NULL)
	{
		layer->title = buffer_init();
		buffer_add_str(layer->title, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "abstract");
	if (a != NULL)
	{
		layer->abstract = buffer_init();
		buffer_add_str(layer->abstract, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "keywords");
	if (a != NULL)
	{
		b = buffer_init();
		buffer_add_str(b, (char *) a);
		free(a);
		layer->keywords = list_explode(',', b);
		buffer_free(b);
	}

	/* inherits from layer parent and replaces with specified value if defined */
	a = xmlTextReaderGetAttribute(r, (xmlChar *) "queryable");
	if (a != NULL && atoi((char *) a) == 1)
	{
		layer->queryable = true;
		free(a);
	}
	else if (a == NULL && layer->parent != NULL
	   && layer->parent->queryable == true)
		layer->queryable = true;
	else
		free(a);

	/* inherits from layer parent and replaces with specified value if defined */
	a = xmlTextReaderGetAttribute(r, (xmlChar *) "retrievable");
	if (a != NULL && atoi((char *) a) == 1)
	{
		layer->retrievable = true;
		free(a);
	}
	else if (a == NULL && layer->parent != NULL
	   && layer->parent->retrievable == true)
		layer->retrievable = true;
	else
		free(a);

	/* inherits from layer parent and replaces with specified value if defined */
	a = xmlTextReaderGetAttribute(r, (xmlChar *) "writable");
	if (a != NULL && atoi((char *) a) == 1)
	{
		layer->writable = true;
		free(a);
	}
	else if (a == NULL && layer->parent != NULL
	   && layer->parent->writable == true)
		layer->writable = true;
	else
		free(a);

	/* inherits from layer parent and replaces with specified value if defined */
	a = xmlTextReaderGetAttribute(r, (xmlChar *) "opaque");
	if (a != NULL && atoi((char *) a) == 1)
	{
		layer->opaque = true;
		free(a);
	}
	else if (a == NULL && layer->parent != NULL
	   && layer->parent->opaque == true)
		layer->opaque = true;
	else
		free(a);

	/* inherits from layer parent and adds specified value */
	a = xmlTextReaderGetAttribute(r, (xmlChar *) "srid");
	if (layer->parent != NULL && layer->parent->srid != NULL)
	{
		layer->srid = list_init();
		list_add_list(layer->srid, layer->parent->srid);
	}
	if (a != NULL)
	{
		b = buffer_init();
		buffer_add_str(b, (char *) a);
		free(a);
		l = list_explode(',', b);
		if (layer->srid == NULL)
			layer->srid = list_init();
		list_add_list(layer->srid, l);
		buffer_free(b);
		list_free(l);
	}

	/* inherits from layer parent and adds specified value */
	a = xmlTextReaderGetAttribute(r, (xmlChar *) "styles");
	if (layer->parent != NULL && layer->parent->styles != NULL)
	{
		layer->styles = list_init();
		list_add_list(layer->styles, layer->parent->styles);
	}
	if (a != NULL)
	{
		b = buffer_init();
		buffer_add_str(b, (char *) a);
		free(a);
		l = list_explode(',', b);
		if (layer->styles == NULL)
			layer->styles = list_init();
		list_add_list(layer->styles, l);
		buffer_free(b);
		list_free(l);
	}

	/* inherits from layer parent and replaces with specified value if defined */
	a = xmlTextReaderGetAttribute(r, (xmlChar *) "geobbox");
	if (a != NULL)
	{
		layer->geobbox = ows_geobbox_init();
		ows_geobbox_set_from_str(o, layer->geobbox, (char *) a);
		free(a);
	}
	else if (a == NULL && layer->parent != NULL
	   && layer->parent->geobbox != NULL)
	{
		layer->geobbox = ows_geobbox_init();
		layer->geobbox->east = layer->parent->geobbox->east;
		layer->geobbox->west = layer->parent->geobbox->west;
		layer->geobbox->south = layer->parent->geobbox->south;
		layer->geobbox->north = layer->parent->geobbox->north;
	}
	else
		free(a);

	/* inherits from layer parent and replaces with specified value if defined */
	a = xmlTextReaderGetAttribute(r, (xmlChar *) "prefix");
	if (a != NULL)
	{
		layer->prefix = buffer_init();
		buffer_add_str(layer->prefix, (char *) a);
		free(a);
	}
	else if (a == NULL && layer->parent != NULL
	   && layer->parent->prefix != NULL)
	{
		layer->prefix = buffer_init();
		buffer_copy(layer->prefix, layer->parent->prefix);
	}
	else
		free(a);

	/* inherits from layer parent and replaces with specified value if defined */
	a = xmlTextReaderGetAttribute(r, (xmlChar *) "server");
	if (a != NULL)
	{
		layer->server = buffer_init();
		buffer_add_str(layer->server, (char *) a);
		free(a);
	}
	else if (a == NULL && layer->parent != NULL
	   && layer->parent->server != NULL)
	{
		layer->server = buffer_init();
		buffer_copy(layer->server, layer->parent->server);
	}
	else
		free(a);

	ows_layer_list_add(o->layers, layer);
}


/* 
 * Parse the configuration file's style element
 */
void ows_parse_config_style(ows * o, xmlTextReaderPtr r)
{
	xmlChar *a;

	assert(o != NULL);
	assert(r != NULL);

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "sld");
	if (a != NULL)
	{
		o->sld_path = buffer_init();
		buffer_add_str(o->sld_path, (char *) a);
		free(a);
	}

	a = xmlTextReaderGetAttribute(r, (xmlChar *) "writable");
	if (a != NULL)
	{
		if (atoi((char *) a) == 1)
			o->sld_writable = true;
		free(a);
	}
}


/* 
 * Parse the configuration file and initialize ows struct
 */
void ows_parse_config(ows * o, const char *filename)
{
	xmlTextReaderPtr r;
	const xmlChar *name;
	int ret;

	assert(o != NULL);
	assert(filename != NULL);

	xmlInitParser();
	r = xmlReaderForFile(filename, "UTF-8", 0);
	if (r == NULL)
	{
		xmlCleanupParser();
		ows_error(o, OWS_ERROR_CONFIG_FILE, "Unable to open config file !",
		   "parse_config_file");
	}

	if (o->layers == NULL)
		o->layers = ows_layer_list_init();

	ret = xmlTextReaderRead(r);
	while (ret == 1)
	{
		ret = xmlTextReaderRead(r);
		if (xmlTextReaderNodeType(r) == XML_READER_TYPE_ELEMENT)
		{
			name = xmlTextReaderConstLocalName(r);

			if (strcmp((char *) name, "contact") == 0)
				ows_parse_config_contact(o, r);

			if (strcmp((char *) name, "pg") == 0)
				ows_parse_config_pg(o, r);

			if (strcmp((char *) name, "limits") == 0)
				ows_parse_config_limits(o, r);

			if (strcmp((char *) name, "layer") == 0)
				ows_parse_config_layer(o, r);

			if (strcmp((char *) name, "style") == 0)
				ows_parse_config_style(o, r);
		}
	}
	if (ret != 0)
	{
		xmlFreeTextReader(r);
		xmlCleanupParser();
		ows_error(o, OWS_ERROR_CONFIG_FILE, "Unable to open config file !",
		   "parse_config_file");
	}

	xmlFreeTextReader(r);
	xmlCleanupParser();

}


/*
 * vim: expandtab sw=4 ts=4 
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libxml/xmlreader.h>

#include "../ows/ows.h"


void sld_parse_file(ows * o, const char *filename)
{
	xmlTextReaderPtr r;
	const xmlChar *name;
	int ret;

	assert(o != NULL);
	assert(filename != NULL);

	r = xmlReaderForFile(filename, "UTF-8", 0);
	if (r == NULL)
	{
		xmlCleanupParser();		/* FIXME really ? */
		ows_error(o, OWS_ERROR_CONFIG_FILE, "Unable to open config file !",
		   "SLD_parse");
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
		   "SLD_parse");
	}

	xmlFreeTextReader(r);
	xmlCleanupParser();
}


/*
 * vim: expandtab sw=4 ts=4 
 */

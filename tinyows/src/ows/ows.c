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
#include <string.h>
#include <ctype.h>

#include "ows.h"
#include "../ows_define.h"


/* 
 * Connect the ows to the database specified in configuration file
 */
static void ows_pg(ows * o, char *con_str)
{
	assert(o != NULL);
	assert(con_str != NULL);

	o->pg = PQconnectdb(con_str);

	if (PQstatus(o->pg) != CONNECTION_OK)
		ows_error(o, OWS_ERROR_CONNECTION_FAILED,
		   "connection to database failed", "init_OWS");
}


/*
 * Initialize an ows struct 
 */
static ows *ows_init()
{
	ows *o;

	o = malloc(sizeof(ows));
	assert(o != NULL);

	o->request = NULL;
	o->cgi = NULL;
	o->psql_requests = NULL;
	o->pg = NULL;
	o->pg_dsn = NULL;
	o->output = NULL;

	o->layers = NULL;

	o->max_width = 0;
	o->max_height = 0;
	o->max_layers = 0;
	o->max_features = 0;
	o->max_geobbox = NULL;

	o->metadata = NULL;
	o->contact = NULL;

	o->sld_path = NULL;
	o->sld_writable = 0;

	return o;
}


/* 
 * Flush an ows structure
 * Used for debug purpose
 */
#ifdef OWS_DEBUG
void ows_flush(ows * o, FILE * output)
{
	assert(o != NULL);
	assert(output != NULL);

	if (o->pg_dsn != NULL)
	{
		fprintf(output, "pg: ");
		buffer_flush(o->pg_dsn, output);
		fprintf(output, "\n");
	}
	if (o->metadata != NULL)
	{
		fprintf(output, "metadata: ");
		ows_metadata_flush(o->metadata, output);
		fprintf(output, "\n");
	}
	if (o->contact != NULL)
	{
		fprintf(output, "contact: ");
		ows_contact_flush(o->contact, output);
		fprintf(output, "\n");
	}

	if (o->cgi != NULL)
	{
		fprintf(output, "cgi: ");
		array_flush(o->cgi, output);
		fprintf(output, "\n");
	}
	if (o->psql_requests != NULL)
	{
		fprintf(output, "SQL requests: ");
		list_flush(o->psql_requests, output);
		fprintf(output, "\n");
	}

	if (o->layers != NULL)
	{
		fprintf(output, "layers: ");
		ows_layer_list_flush(o->layers, output);
		fprintf(output, "\n");
	}

	if (o->request != NULL)
	{
		fprintf(output, "request: ");
		ows_request_flush(o->request, output);
		fprintf(output, "\n");
	}

	fprintf(output, "max_width: %d\n", o->max_width);
	fprintf(output, "max_height: %d\n", o->max_height);
	fprintf(output, "max_layers: %d\n", o->max_layers);
	fprintf(output, "max_features: %d\n", o->max_features);
	if (o->max_geobbox != NULL)
	{
		fprintf(output, "max_geobbox: ");
		ows_geobbox_flush(o->max_geobbox, output);
		fprintf(output, "\n");
	}

	if (o->sld_path != NULL)
	{
		fprintf(output, "sld_path: ");
		buffer_flush(o->sld_path, output);
		fprintf(output, "\n");
	}
	fprintf(output, "sld_writable: %d\n", o->sld_writable);
}
#endif


/*
 * Release ows struct 
 */
void ows_free(ows * o)
{
	assert(o != NULL);

	if (o->pg != NULL)
		PQfinish(o->pg);

	if (o->pg_dsn != NULL)
		buffer_free(o->pg_dsn);

	if (o->cgi != NULL)
		array_free(o->cgi);

	if (o->psql_requests != NULL)
		list_free(o->psql_requests);

	if (o->layers != NULL)
		ows_layer_list_free(o->layers);

	if (o->request != NULL)
		ows_request_free(o->request);

	if (o->max_geobbox != NULL)
		ows_geobbox_free(o->max_geobbox);

	if (o->metadata != NULL)
		ows_metadata_free(o->metadata);

	if (o->contact != NULL)
		ows_contact_free(o->contact);

	if (o->sld_path != NULL)
		buffer_free(o->sld_path);

	free(o);
	o = NULL;
}


void ows_usage(ows * o) {
    printf("TinyOWS should be called by CGI throw a Web Server !\n\n");
    printf("___________\n");
    printf("Config File Path: %s\n", OWS_CONFIG_FILE_PATH);
    printf("PostGIS dsn: '%s'\n", o->pg_dsn->buf);
    printf("___________\n");
    printf("WFS 1.0.0 Basic Schema Path: %s\n", WFS_SCHEMA_100_BASIC_PATH);
    printf("WFS 1.0.0 Transactional Schema Path: %s\n", 
        WFS_SCHEMA_100_TRANS_PATH);
    printf("WFS 1.1.0 Schema Path: %s\n", WFS_SCHEMA_110_PATH);
    printf("___________\n");
}


int main(int argc, char *argv[])
{
	char *query;
	ows *o;

	o = ows_init();

	/* TODO add an alternative cache system */
	o->output = stdout;

	/* retrieve the query in HTTP request */
	query = cgi_getback_query(o);

	if (query != NULL && strlen(query) != 0)
	{
		/* initialize input array to store CGI values */
		/* The Content-Type of all POST KVP-encoded request entities 
		   must be 'application/x-www-form-urlencoded' */
		if (cgi_method_get()
		   || (cgi_method_post()
			  && strcmp(getenv("CONTENT_TYPE"),
				 "application/x-www-form-urlencoded") == 0))
			o->cgi = cgi_parse_kvp(o, query);
		else
			o->cgi = cgi_parse_xml(o, query);

		o->psql_requests = list_init();

		/* Parse the configuration file and initialize ows struct */
		ows_parse_config(o, OWS_CONFIG_FILE_PATH);

		/* Connect the ows to the database */
		ows_pg(o, o->pg_dsn->buf);

		/* Fill service's metadata */
		ows_metadata_fill(o, o->cgi);

		/* Process service request */
		o->request = ows_request_init();
		ows_request_check(o, o->request, o->cgi, query);

		/* Run the right OWS service */
		switch (o->request->service)
		{
		case WMS:
			o->request->request.wms = wms_request_init();
			wms_request_check(o, o->request->request.wms, o->cgi);
			wms(o, o->request->request.wms);
			break;
		case WFS:
			o->request->request.wfs = wfs_request_init();
			wfs_request_check(o, o->request->request.wfs, o->cgi);
			wfs(o, o->request->request.wfs);
			break;
		default:
			ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
			   "service unknown", "service");
		}
	} else if (argc > 1 && (strncmp(argv[1], "--help", 6) == 0
            || strncmp(argv[1], "--h", 3) == 0)) {
		ows_parse_config(o, OWS_CONFIG_FILE_PATH);
        ows_usage(o);
    } else {
			ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
			   "service unknown", "service");
    }
  
	ows_free(o);

	return EXIT_SUCCESS;
}


/*
 * vim: expandtab sw=4 ts=4 
 */

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
#include <limits.h>
#include <string.h>

#include "ows.h"


/*
 * Initialize an aws layer structure
 */
ows_layer_list *ows_layer_list_init()
{
	ows_layer_list *ll;

	ll = malloc(sizeof(ows_layer_list));
	assert(ll != NULL);

	ll->first = NULL;
	ll->last = NULL;

	return ll;
}


/*
 * Free an ows layer structure
 */
void ows_layer_list_free(ows_layer_list * ll)
{

	assert(ll != NULL);

	while (ll->first != NULL)
		ows_layer_node_free(ll, ll->first);

	ll->last = NULL;

	free(ll);
	ll = NULL;
}


/* 
 * Check if a layer matchs an existing table in PostGis
 */
bool ows_layer_match_table(ows * o, buffer * layer_name)
{
	PGresult *res;
	buffer *sql, *request_name, *parameters;
	bool ok;

	assert(o != NULL);
	assert(o->pg != NULL);
	assert(layer_name != NULL);

	ok = false;

	sql = buffer_init();
	buffer_add_str(sql, "SELECT count(*) ");
	buffer_add_str(sql, "FROM pg_class ");
	buffer_add_str(sql, "WHERE relname = $1");

	/* initialize the request's name and parameters */
	request_name = buffer_init();
	buffer_add_str(request_name, "match_table");
	parameters = buffer_init();
	buffer_add_str(parameters, "(text)");

	/* check if the request has already been executed */
	if (!in_list(o->psql_requests, request_name))
		ows_psql_prepare(o, request_name, parameters, sql);

	/* execute the request */
	buffer_empty(sql);
	buffer_add_str(sql, "EXECUTE match_table('");
	buffer_copy(sql, layer_name);
	buffer_add_str(sql, "')");

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);
	buffer_free(parameters);
	buffer_free(request_name);

	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
	{
		PQclear(res);

		return ok;
	}

	if (atoi(PQgetvalue(res, 0, 0)) == 1)
		ok = true;

	PQclear(res);

	return ok;
}


/* 
 * Check if all layers are retrievable (defined in configuration file)
 */
bool ows_layer_list_retrievable(const ows_layer_list * ll)
{
	ows_layer_node *ln;

	assert(ll != NULL);

	for (ln = ll->first; ln != NULL; ln = ln->next)
		if (ln->layer->retrievable == false)
			return false;

	return true;
}


/* 
 * Check if a layer is retrievable (defined in configuration file)
 */
bool ows_layer_retrievable(const ows_layer_list * ll, const buffer * name)
{
	ows_layer_node *ln;

	assert(ll != NULL);
	assert(name != NULL);

	for (ln = ll->first; ln != NULL; ln = ln->next)
		if (ln->layer->name != NULL
		   && ln->layer->name->use == name->use
		   && strcmp(ln->layer->name->buf, name->buf) == 0)
			return ln->layer->retrievable;

	return false;
}


/* 
 * Check if all layers are writable (defined in file conf)
 */
bool ows_layer_list_writable(const ows_layer_list * ll)
{
	ows_layer_node *ln;

	assert(ll != NULL);

	for (ln = ll->first; ln != NULL; ln = ln->next)
		if (ln->layer->writable == false)
			return false;

	return true;
}


/* 
 * Check if a layer is writable (defined in file conf)
 */
bool ows_layer_writable(const ows_layer_list * ll, const buffer * name)
{
	ows_layer_node *ln;

	assert(ll != NULL);
	assert(name != NULL);

	for (ln = ll->first; ln != NULL; ln = ln->next)
		if (ln->layer->name != NULL
		   && ln->layer->name->use == name->use
		   && strcmp(ln->layer->name->buf, name->buf) == 0)
			return ln->layer->writable;

	return false;
}


/*
 * Check if a layer is in a layer's list
 */
bool ows_layer_in_list(const ows_layer_list * ll, buffer * name)
{
	ows_layer_node *ln;

	assert(ll != NULL);
	assert(name != NULL);

	for (ln = ll->first; ln != NULL; ln = ln->next)
		if (ln->layer->name != NULL
		   && ln->layer->name->use == name->use
		   && strcmp(ln->layer->name->buf, name->buf) == 0)
			return true;

	return false;
}


/*
 * Check if a layer's list is in a other layer's list
 */
bool ows_layer_list_in_list(const ows_layer_list * ll, const list * l)
{
	list_node *ln;

	assert(ll != NULL);
	assert(l != NULL);

	for (ln = l->first; ln != NULL; ln = ln->next)
		if (!ows_layer_in_list(ll, ln->value))
			return false;

	return true;
}


/*
 * Return an array of prefixes and associated servers of a layer's list 
 */
array *ows_layer_list_namespaces(ows_layer_list * ll)
{
	array *namespaces;
	buffer *prefix, *server;
	ows_layer_node *ln;

	assert(ll != NULL);

	namespaces = array_init();

	for (ln = ll->first; ln != NULL; ln = ln->next)
	{
		if (ln->layer->prefix != NULL)
		{
			if (!array_is_key(namespaces, ln->layer->prefix->buf))
			{
				prefix = buffer_init();
				server = buffer_init();
				buffer_copy(prefix, ln->layer->prefix);
				buffer_copy(server, ln->layer->server);
				array_add(namespaces, prefix, server);
			}
		}
	}

	return namespaces;
}


/*
 * Return a list of layer names grouped by prefix
 */
list *ows_layer_list_by_prefix(ows_layer_list * ll, list * layer_name,
   buffer * prefix)
{
	list *typ;
	list_node *ln;
	buffer *layer_prefix;

	assert(ll != NULL);
	assert(layer_name != NULL);
	assert(prefix != NULL);

	typ = list_init();
	for (ln = layer_name->first; ln != NULL; ln = ln->next)
	{
		layer_prefix = ows_layer_prefix(ll, ln->value);
		if (buffer_cmp(layer_prefix, prefix->buf))
			list_add_by_copy(typ, ln->value);
		buffer_free(layer_prefix);
	}

	return typ;
}


/*
 * Retrieve a list of prefix used for a specified list of layers
 */
list *ows_layer_list_prefix(ows_layer_list * ll, list * layer_name)
{

	list_node *ln;
	list *ml_prefix;
	buffer *prefix;

	assert(ll != NULL);
	assert(layer_name != NULL);

	ml_prefix = list_init();

	for (ln = layer_name->first; ln != NULL; ln = ln->next)
	{
		prefix = ows_layer_prefix(ll, ln->value);
		if (!in_list(ml_prefix, prefix))
			list_add_by_copy(ml_prefix, prefix);
		buffer_free(prefix);
	}


	return ml_prefix;
}


/*
 * Retrieve the prefix linked to the specified layer
 */
buffer *ows_layer_prefix(ows_layer_list * ll, buffer * layer_name)
{
	ows_layer_node *ln;
	ows_layer *l;
	buffer *prefix;

	assert(ll != NULL);
	assert(layer_name != NULL);

	prefix = buffer_init();

	for (ln = ll->first; ln != NULL; ln = ln->next)
	{
		if (buffer_cmp(ln->layer->name, layer_name->buf))
		{
			l = ln->layer;
			while (l->prefix == NULL)
				l = l->parent;

			buffer_copy(prefix, l->prefix);
			return prefix;
		}
	}

	return prefix;
}


/*
 * Retrieve the server associated to the specified prefix
 */
buffer *ows_layer_server(ows_layer_list * ll, buffer * prefix)
{
	ows_layer_node *ln;

	assert(ll != NULL);
	assert(prefix != NULL);

	for (ln = ll->first; ln != NULL; ln = ln->next)
	{
		if (ln->layer->prefix != NULL)
		{
			if (buffer_cmp(ln->layer->prefix, prefix->buf))
				return ln->layer->server;
		}

	}

	return (buffer *) NULL;
}


/* 
 * Add a layer to a layer's list
 */
void ows_layer_list_add(ows_layer_list * ll, ows_layer * l)
{
	ows_layer_node *ln;

	assert(ll != NULL);
	assert(l != NULL);

	ln = ows_layer_node_init();

	ln->layer = l;

	if (ll->first == NULL)
	{
		ln->prev = NULL;
		ll->first = ln;
	}
	else
	{
		ln->prev = ll->last;
		ll->last->next = ln;
	}

	ll->last = ln;
	ll->last->next = NULL;
}


/* 
 * Initialize a layer node
 */
ows_layer_node *ows_layer_node_init()
{
	ows_layer_node *ln;

	ln = malloc(sizeof(ows_layer_node));
	assert(ln != NULL);

	ln->prev = NULL;
	ln->next = NULL;
	ln->layer = NULL;

	return ln;
}


/* 
 * Free a layer node
 */
void ows_layer_node_free(ows_layer_list * ll, ows_layer_node * ln)
{
	assert(ln != NULL);

	if (ln->prev != NULL)
		ln->prev = NULL;

	if (ln->next != NULL)
	{
		if (ll != NULL)
			ll->first = ln->next;
		ln->next = NULL;
	}
	else
	{
		if (ll != NULL)
			ll->first = NULL;
	}

	if (ln->layer != NULL)
		ows_layer_free(ln->layer);

	free(ln);
	ln = NULL;
}


/*
 * Flush a layer's list to a given file
 * (mainly to debug purpose)
 */
#ifdef OWS_DEBUG
void ows_layer_list_flush(ows_layer_list * ll, FILE * output)
{
	ows_layer_node *ln = NULL;

	assert(ll != NULL);
	assert(output != NULL);

	for (ln = ll->first; ln != NULL; ln = ln->next)
	{
		ows_layer_flush(ln->layer, output);
		fprintf(output, "-------------\n");
	}
}
#endif


/* 
 * Initialize a layer
 */
ows_layer *ows_layer_init()
{
	ows_layer *l;

	l = malloc(sizeof(ows_layer));
	assert(l != NULL);

	l->parent = NULL;
	l->depth = 0;
	l->title = NULL;
	l->name = NULL;
	l->abstract = NULL;
	l->keywords = NULL;
	l->queryable = false;
	l->retrievable = false;
	l->writable = false;
	l->opaque = false;
	l->srid = NULL;
	l->styles = NULL;
	l->geobbox = NULL;
	l->prefix = NULL;
	l->server = NULL;

	return l;
}


/* 
 * Free a layer
 */
void ows_layer_free(ows_layer * l)
{
	assert(l != NULL);

	if (l->title != NULL)
		buffer_free(l->title);

	if (l->name != NULL)
		buffer_free(l->name);

	if (l->abstract != NULL)
		buffer_free(l->abstract);

	if (l->keywords != NULL)
		list_free(l->keywords);

	if (l->srid != NULL)
		list_free(l->srid);

	if (l->styles != NULL)
		list_free(l->styles);

	if (l->geobbox != NULL)
		ows_geobbox_free(l->geobbox);

	if (l->prefix != NULL)
		buffer_free(l->prefix);

	if (l->server != NULL)
		buffer_free(l->server);

	free(l);
	l = NULL;
}


/*
 * Flush a layer to a given file
 * (mainly to debug purpose)
 */
#ifdef OWS_DEBUG
void ows_layer_flush(ows_layer * l, FILE * output)
{
	assert(l != NULL);
	assert(output != NULL);

	fprintf(output, "depth: %i\n", l->depth);
	if (l->parent != NULL)
	{
		if (l->parent->name != NULL)
			fprintf(output, "parent: %s\n", l->parent->name->buf);
		else if (l->parent->title != NULL)
			fprintf(output, "parent: %s\n", l->parent->title->buf);
	}

	fprintf(output, "queryable: %i\n", l->queryable ? 1 : 0);
	fprintf(output, "retrievable: %i\n", l->retrievable ? 1 : 0);
	fprintf(output, "opaque: %i\n", l->opaque ? 1 : 0);

	if (l->title != NULL)
	{
		fprintf(output, "title: ");
		buffer_flush(l->title, output);
		fprintf(output, "\n");
	}

	if (l->name != NULL)
	{
		fprintf(output, "name: ");
		buffer_flush(l->name, output);
		fprintf(output, "\n");
	}

	if (l->srid != NULL)
	{
		fprintf(output, "srid: ");
		list_flush(l->srid, output);
		fprintf(output, "\n");
	}

	if (l->keywords != NULL)
	{
		fprintf(output, "keyword: ");
		list_flush(l->keywords, output);
		fprintf(output, "\n");
	}

	if (l->geobbox != NULL)
	{
		fprintf(output, "geobbox: ");
		ows_geobbox_flush(l->geobbox, output);
		fprintf(output, "\n");
	}

	if (l->styles != NULL)
	{
		fprintf(output, "styles: ");
		list_flush(l->styles, output);
		fprintf(output, "\n");
	}
	if (l->prefix != NULL)
	{
		fprintf(output, "prefix: ");
		buffer_flush(l->prefix, output);
		fprintf(output, "\n");
	}

	if (l->server != NULL)
	{
		fprintf(output, "server: ");
		buffer_flush(l->server, output);
		fprintf(output, "\n");
	}
}
#endif


/*
 * vim: expandtab sw=4 ts=4 
 */

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
#include <libpq-fe.h>
#include <string.h>

#include "ows.h"


/*
 * Initialize proj structure
 */
ows_srs *ows_srs_init()
{
	ows_srs *c;

	c = malloc(sizeof(ows_srs));
	assert(c != NULL);

	c->srid = -1;
	c->auth_name = buffer_init();
	c->auth_srid = 0;
	c->is_unit_degree = true;

	return c;
}


/*
 * Free proj structure
 */
void ows_srs_free(ows_srs * c)
{
	assert(c != NULL);

	buffer_free(c->auth_name);

	free(c);
	c = NULL;
}


#ifdef OWS_DEBUG
/*
 * Print ows structure state into a file
 * (mainly for debug purpose)
 */
void ows_srs_flush(ows_srs * c, FILE * output)
{
	assert(c != NULL);
	assert(output != NULL);

	fprintf(output, "[\n");
	fprintf(output, " srid: %i\n", c->srid);
	fprintf(output, " auth_name: %s\n", c->auth_name->buf);
	fprintf(output, " auth_srid: %i\n", c->auth_srid);
	if (c->is_unit_degree)
		fprintf(output, " is_unit_degree: true\n]\n");
	else
		fprintf(output, " is_unit_degree: false\n]\n");
}
#endif


/*
 * Set projection value into srs structure
 */
bool ows_srs_set(ows * o, ows_srs * c, const buffer * auth_name,
   int auth_srid)
{
	PGresult *res;
	buffer *sql, *request_name, *parameters;

	assert(o != NULL);
	assert(o->pg != NULL);
	assert(c != NULL);
	assert(auth_name != NULL);

	sql = buffer_init();
	buffer_add_str(sql,
	   "SELECT srid, position('+units=m ' in proj4text) ");
	buffer_add_str(sql, "FROM spatial_ref_sys ");
	buffer_add_str(sql, "WHERE auth_name= $1 ");
	buffer_add_str(sql, "AND auth_srid= $2");

	/* initialize the request's name and parameters */
	request_name = buffer_init();
	buffer_add_str(request_name, "srs_set");
	parameters = buffer_init();
	buffer_add_str(parameters, "(text,number)");

	/* check if the request has already been executed */
	if (!in_list(o->psql_requests, request_name))
		ows_psql_prepare(o, request_name, parameters, sql);

	/* execute the request */
	buffer_empty(sql);
	buffer_add_str(sql, "EXECUTE srs_set('");
	buffer_copy(sql, auth_name);
	buffer_add_str(sql, "',");
	buffer_add_int(sql, auth_srid);
	buffer_add_str(sql, ")");

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);
	buffer_free(parameters);
	buffer_free(request_name);

	/* If query dont return exactly 1 result, it means projection is 
       not handled */
	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
	{
		PQclear(res);
		return false;
	}

	buffer_empty(c->auth_name);
	buffer_copy(c->auth_name, auth_name);
	c->auth_srid = auth_srid;

	c->srid = atoi(PQgetvalue(res, 0, 0));

	/* Such a way to know if units is meter or degree */
	if (atoi(PQgetvalue(res, 0, 1)) == 1)
		c->is_unit_degree = false;
	else
		c->is_unit_degree = true;

	PQclear(res);
	return true;
}


/*
 * Set projection value into srs structure
 */
bool ows_srs_set_from_srid(ows * o, ows_srs * s, int srid)
{
	PGresult *res;
	buffer *sql, *request_name, *parameters;

	assert(o != NULL);
	assert(s != NULL);

	if (srid == -1)
	{
		s->srid = -1;
		buffer_empty(s->auth_name);
		s->auth_srid = 0;
		s->is_unit_degree = true;

		return true;
	}

	sql = buffer_init();
	buffer_add_str(sql, "SELECT auth_name, auth_srid, ");
	buffer_add_str(sql, "position('+units=m ' in proj4text) ");
	buffer_add_str(sql, "FROM spatial_ref_sys WHERE srid = $1");

	/* initialize the request's name and parameters */
	request_name = buffer_init();
	buffer_add_str(request_name, "srs_set_from_srid");
	parameters = buffer_init();
	buffer_add_str(parameters, "(int)");

	/* check if the request has already been executed */
	if (!in_list(o->psql_requests, request_name))
		ows_psql_prepare(o, request_name, parameters, sql);

	/* execute the request */
	buffer_empty(sql);
	buffer_add_str(sql, "EXECUTE srs_set_from_srid(");
	buffer_add_int(sql, srid);
	buffer_add_str(sql, ")");

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);
	buffer_free(parameters);
	buffer_free(request_name);

	/* If query dont return exactly 1 result, it mean projection not handled */
	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
	{
		PQclear(res);
		return false;
	}

	buffer_add_str(s->auth_name, PQgetvalue(res, 0, 0));
	s->auth_srid = atoi(PQgetvalue(res, 0, 1));
	s->srid = srid;

	/* Such a way to know if units is meter or degree */
	if (atoi(PQgetvalue(res, 0, 2)) == 1)
		s->is_unit_degree = false;
	else
		s->is_unit_degree = true;

	PQclear(res);
	return true;
}


/* 
 * Check if a layer's srs has meter or degree units
 */
bool ows_srs_meter_units(ows * o, buffer * layer_name)
{
	buffer *sql, *request_name, *parameters;
	PGresult *res;
	buffer *srid;

	assert(o != NULL);
	assert(layer_name != NULL);

	sql = buffer_init();

	srid = ows_srs_get_srid_from_layer(o, layer_name);

	buffer_add_str(sql, "SELECT * FROM spatial_ref_sys ");
	buffer_add_str(sql, "WHERE srid = $1 ");
	buffer_add_str(sql, "AND proj4text like '%units=m%'");

	/* initialize the request's name and parameters */
	request_name = buffer_init();
	buffer_add_str(request_name, "srs_meter_units");
	parameters = buffer_init();
	buffer_add_str(parameters, "(text)");

	/* check if the request has already been executed */
	if (!in_list(o->psql_requests, request_name))
		ows_psql_prepare(o, request_name, parameters, sql);

	/* execute the request */
	buffer_empty(sql);
	buffer_add_str(sql, "EXECUTE srs_meter_units(");
	buffer_copy(sql, srid);
	buffer_add_str(sql, ")");

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);
	buffer_free(srid);
	buffer_free(parameters);
	buffer_free(request_name);

	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return false;
	}

	PQclear(res);

	return true;
}


/* 
 * Retrieve a srs from a layer
 */
buffer *ows_srs_get_srid_from_layer(ows * o, buffer * layer_name)
{
	buffer *sql, *request_name, *parameters;
	list *geom;
	PGresult *res;
	buffer *b;

	assert(o != NULL);
	assert(layer_name != NULL);

	sql = buffer_init();
	b = buffer_init();

	geom = ows_psql_geometry_column(o, layer_name);

	if (geom->first != NULL)
	{
		buffer_add_str(sql, "SELECT find_srid('',$1,$2)");

		/* initialize the request's name and parameters */
		request_name = buffer_init();
		buffer_add_str(request_name, "get_srid_from_layer");
		parameters = buffer_init();
		buffer_add_str(parameters, "(text,text)");

		/* check if the request has already been executed */
		if (!in_list(o->psql_requests, request_name))
			ows_psql_prepare(o, request_name, parameters, sql);

		/* execute the request */
		buffer_empty(sql);
		buffer_add_str(sql, "EXECUTE get_srid_from_layer('");
		buffer_copy(sql, layer_name);
		buffer_add_str(sql, "','");
		buffer_copy(sql, geom->first->value);
		buffer_add_str(sql, "')");

		res = PQexec(o->pg, sql->buf);
		buffer_free(sql);
		buffer_free(parameters);
		buffer_free(request_name);

		if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
		{
			list_free(geom);
			PQclear(res);
			buffer_free(sql);
			return b;
		}
		buffer_add_str(b, (PQgetvalue(res, 0, 0)));
		PQclear(res);
	}
	list_free(geom);

	return b;
}


/* 
 * Retrieve a list of srs from an srid list
 */
list *ows_srs_get_from_srid(ows * o, list * l)
{
	list_node *ln;
	buffer *b;
	list *srs;

	assert(o != NULL);
	assert(l != NULL);

	srs = list_init();
	if (l->size == 0)
		return srs;

	for (ln = l->first; ln != NULL; ln = ln->next)
	{
		b = ows_srs_get_from_a_srid(o, ln->value);
		list_add(srs, b);
	}

	return srs;
}


/* 
 * Retrieve a srs from a srid
 */
buffer *ows_srs_get_from_a_srid(ows * o, buffer * srid)
{
	buffer *b;
	buffer *sql, *request_name, *parameters;
	PGresult *res;

	assert(o != NULL);
	assert(srid != NULL);

	sql = buffer_init();
	buffer_add_str(sql, "SELECT auth_name||':'||auth_srid AS srs ");
	buffer_add_str(sql, "FROM spatial_ref_sys ");
	buffer_add_str(sql, "WHERE srid = $1");

	/* initialize the request's name and parameters */
	request_name = buffer_init();
	buffer_add_str(request_name, "get_from_a_srid");
	parameters = buffer_init();
	buffer_add_str(parameters, "(text)");

	/* check if the request has already been executed */
	if (!in_list(o->psql_requests, request_name))
		ows_psql_prepare(o, request_name, parameters, sql);

	/* execute the request */
	buffer_empty(sql);
	buffer_add_str(sql, "EXECUTE get_from_a_srid(");
	if (srid->use != 0)
		buffer_copy(sql, srid);
	else
		buffer_add_int(sql, -1);
	buffer_add_str(sql, ")");

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);
	buffer_free(parameters);
	buffer_free(request_name);

	b = buffer_init();
	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
	{
		PQclear(res);
		return b;
	}

	buffer_add_str(b, PQgetvalue(res, 0, 0));

	PQclear(res);

	return b;
}


/*
 * vim: expandtab sw=4 ts=4 
 */

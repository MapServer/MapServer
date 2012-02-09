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

#include "ows.h"


/*
 * Initialize a bbox structure
 */
ows_bbox *ows_bbox_init()
{
	ows_bbox *b;

	b = malloc(sizeof(ows_bbox));
	assert(b != NULL);

	b->xmin = DBL_MIN;
	b->ymin = DBL_MIN;
	b->xmax = DBL_MAX;
	b->ymax = DBL_MAX;

	b->srs = ows_srs_init();

	return b;
}


/*
 * Free a bbox structure
 */
void ows_bbox_free(ows_bbox * b)
{
	assert(b != NULL);

	ows_srs_free(b->srs);

	free(b);
	b = NULL;
}


/*
 * Set a given bbox with null area test
 */
bool ows_bbox_set(ows * o, ows_bbox * b, double xmin, double ymin,
   double xmax, double ymax, int srid)
{
	assert(o != NULL);
	assert(b != NULL);

	if (xmin >= xmax || ymin >= ymax)
		return false;

	if (fabs(xmin - DBL_MIN) < DBL_EPSILON
	   || fabs(ymin - DBL_MIN) < DBL_EPSILON
	   || fabs(xmax - DBL_MAX) < DBL_EPSILON
	   || fabs(ymax - DBL_MAX) < DBL_EPSILON)
		return false;

	b->xmin = xmin;
	b->xmax = xmax;
	b->ymin = ymin;
	b->ymax = ymax;

	return ows_srs_set_from_srid(o, b->srs, srid);
}


/*
 * Set a given bbox from a string like 'xmin,ymin,xmax,ymax'
 */
bool ows_bbox_set_from_str(ows * o, ows_bbox * bb, const char *str,
   int srid)
{
	buffer *b;
	list *l;
	double xmin, ymin, xmax, ymax;

	assert(o != NULL);
	assert(bb != NULL);
	assert(str != NULL);

	b = buffer_init();
	buffer_add_str(b, str);
	l = list_explode(',', b);
	buffer_free(b);

	if (l->size != 4)
	{
		list_free(l);
		return false;
	}

	/* FIXME atof don't handle error */
	xmin = atof(l->first->value->buf);
	ymin = atof(l->first->next->value->buf);
	xmax = atof(l->first->next->next->value->buf);
	ymax = atof(l->first->next->next->next->value->buf);

	list_free(l);

	return ows_bbox_set(o, bb, xmin, ymin, xmax, ymax, srid);
}


/*
 * Set a given bbox matching a feature collection's outerboundaries 
 * or a simple feature's outerboundaries
 * Bbox is set from a list containing one or several layer names 
 * and optionnaly one or several WHERE SQL statement following each layer name
 */
ows_bbox *ows_bbox_boundaries(ows * o, list * from, list * where)
{
	ows_bbox *bb;
	buffer *sql, *srid;
	list *geom;
	list_node *ln_from, *ln_where, *ln_geom;
	PGresult *res;

	assert(o != NULL);
	assert(from != NULL);
	assert(where != NULL);

	bb = ows_bbox_init();
	ln_geom = NULL;

	/* checks if from list and where list have the same size */
	if (from->size != where->size)
		return bb;

	sql = buffer_init();
	/* put into a buffer the SQL request calculating an extent */
	buffer_add_str(sql,
	   "SELECT xmin(g.extent), ymin(g.extent), xmax(g.extent), ymax(g.extent) FROM ");
	buffer_add_str(sql, "(SELECT Extent(");
	buffer_add_str(sql, "foo.the_geom");
	buffer_add_str(sql, ") FROM ( ");
	/* for each layer name or each geometry column, make an union between retrieved features */
	for (ln_from = from->first, ln_where = where->first; ln_from != NULL;
	   ln_from = ln_from->next, ln_where = ln_where->next)
	{
		geom = ows_psql_geometry_column(o, ln_from->value);
		assert(geom != NULL);
		for (ln_geom = geom->first; ln_geom != NULL;
		   ln_geom = ln_geom->next)
		{
			buffer_add_str(sql, " (select \"");
			buffer_copy(sql, ln_geom->value);
			buffer_add_str(sql, "\" As \"the_geom\" FROM \"");
			buffer_copy(sql, ln_from->value);
			buffer_add_str(sql, "\" ");
			buffer_copy(sql, ln_where->value);
			buffer_add_str(sql, ")");
			if (ln_geom->next != NULL)
				buffer_add_str(sql, " union all ");
		}
		if (ln_from->next != NULL)
			buffer_add_str(sql, " union all ");

		list_free(geom);
	}
	buffer_add_str(sql, " ) AS foo");
	buffer_add_str(sql, " ) AS g");

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);

	if (PQresultStatus(res) != PGRES_TUPLES_OK && PQntuples(res) != 4)
	{
		PQclear(res);
		return bb;
	}

	bb->xmin = atof(PQgetvalue(res, 0, 0));
	bb->ymin = atof(PQgetvalue(res, 0, 1));
	bb->xmax = atof(PQgetvalue(res, 0, 2));
	bb->ymax = atof(PQgetvalue(res, 0, 3));

	srid = ows_srs_get_srid_from_layer(o, from->first->value);
	if (ows_srs_set_from_srid(o, bb->srs, atoi(srid->buf)))
	{
		/* do nothing */
	}


	buffer_free(srid);
	PQclear(res);
	return bb;
}


/*
 * Transform a bbox from initial srid to another srid passed in parameter
 */
bool ows_bbox_transform(ows * o, ows_bbox * bb, int srid)
{
	buffer *sql, *request_name, *parameters;
	PGresult *res;

	assert(o != NULL);
	assert(bb != NULL);

	sql = buffer_init();
	buffer_add_str(sql, "SELECT xmin(g), ymin(g), xmax(g), ymax(g) FROM ");
	buffer_add_str(sql, "(SELECT transform(");
	buffer_add_str(sql, "setSRID('BOX($1 $2,$3 $4'::box2d,");
	buffer_add_str(sql, "$5))) AS g ) AS foo");

	/* initialize the request's name and parameters */
	request_name = buffer_init();
	buffer_add_str(request_name, "bbox_transform");
	parameters = buffer_init();
	buffer_add_str(parameters, "(numeric,numeric,numeric,numeric,int)");

	/* check if the request has already been executed */
	if (!in_list(o->psql_requests, request_name))
		ows_psql_prepare(o, request_name, parameters, sql);

	/* execute the request */
	buffer_empty(sql);
	buffer_add_str(sql, "EXECUTE bbox_transform(");
	buffer_add_double(sql, bb->xmin);
	buffer_add_str(sql, ",");
	buffer_add_double(sql, bb->ymin);
	buffer_add_str(sql, ",");
	buffer_add_double(sql, bb->xmax);
	buffer_add_str(sql, ",");
	buffer_add_double(sql, bb->ymax);
	buffer_add_str(sql, ",");
	buffer_add_int(sql, srid);
	buffer_add_str(sql, ")");

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);
	buffer_free(parameters);
	buffer_free(request_name);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		return false;
	}

	/* FIXME atof don't handle error */
	bb->xmin = atof(PQgetvalue(res, 0, 0));
	bb->ymin = atof(PQgetvalue(res, 0, 1));
	bb->xmax = atof(PQgetvalue(res, 0, 2));
	bb->ymax = atof(PQgetvalue(res, 0, 3));

	return ows_srs_set_from_srid(o, bb->srs, srid);
}


/*
 * Transform a geobbox into a bbox
 */
bool ows_bbox_set_from_geobbox(ows * o, ows_bbox * bb, ows_geobbox * geo)
{
	assert(bb != NULL);
	assert(geo != NULL);

	if (geo->west < geo->east)
	{
		bb->xmin = geo->west;
		bb->xmax = geo->east;
	}
	else
	{
		bb->xmax = geo->west;
		bb->xmin = geo->east;
	}

	if (geo->north < geo->south)
	{
		bb->ymin = geo->north;
		bb->ymax = geo->south;
	}
	else
	{
		bb->ymax = geo->north;
		bb->ymin = geo->south;
	}

	return ows_srs_set_from_srid(o, bb->srs, 4326);
}


#ifdef OWS_DEBUG
/*
 * Flush bbox value to a file (mainly to debug purpose)
 */
void ows_bbox_flush(const ows_bbox * b, FILE * output)
{
	assert(b != NULL);

	fprintf(output, "[%f,%f,%f,%f]\n", b->xmin, b->ymin, b->xmax, b->ymax);
}
#endif


/*
 * vim: expandtab sw=4 ts=4 
 */

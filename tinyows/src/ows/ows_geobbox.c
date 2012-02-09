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
 * Initialize a geobbox structure
 */
ows_geobbox *ows_geobbox_init()
{
	ows_geobbox *g;

	g = malloc(sizeof(ows_geobbox));
	assert(g != NULL);

	g->west = DBL_MIN;
	g->east = DBL_MIN;
	g->south = DBL_MIN;
	g->north = DBL_MIN;

	return g;
}


/*
 * Free a geobbox structure
 */
void ows_geobbox_free(ows_geobbox * g)
{
	assert(g != NULL);

	free(g);
	g = NULL;
}


/*
 * Set a given geobbox with null area and earth limits tests
 */
bool ows_geobbox_set(ows * o, ows_geobbox * g, double west, double east,
   double south, double north)
{
	assert(g != NULL);

	if (south < -90.0 || south > 90.0
	   || north < -90.0 || north > 90.0
	   || east < -180.0 || east > 180.0 || west < -180.0 || west > 180.0)
		return false;

	if (fabs(south - north) < DBL_EPSILON
	   || fabs(east - west) < DBL_EPSILON)
		return false;

	/* FIXME add a test to see if north is northern than south and so forth... */
	g->west = west;
	g->east = east;
	g->south = south;
	g->north = north;

	return true;
}


/*
 * Set a given geobbox from a bbox
 */
bool ows_geobbox_set_from_bbox(ows * o, ows_geobbox * g, ows_bbox * bb)
{
	double west, east, south, north;

	assert(g != NULL);
	assert(bb != NULL);

	if (bb->xmin < 0.0 || bb->xmax < 0.0)
	{
		west = bb->xmin;
		east = bb->xmax;
	}
	else
	{
		west = bb->xmax;
		east = bb->xmin;
	}

	if (bb->ymin < 0.0 || bb->ymax < 0.0)
	{
		south = bb->ymax;
		north = bb->ymin;
	}
	else
	{
		south = bb->ymin;
		north = bb->ymax;
	}

	return ows_geobbox_set(o, g, west, east, south, north);
}


/*
 * Set a given geobbox from a string like 'xmin,ymin,xmax,ymax'
 */
ows_geobbox *ows_geobbox_set_from_str(ows * o, ows_geobbox * g, char *str)
{
	ows_bbox *bb;

	assert(g != NULL);
	assert(str != NULL);

	bb = ows_bbox_init();
	ows_bbox_set_from_str(o, bb, str, 4326);

	ows_geobbox_set_from_bbox(o, g, bb);

	ows_bbox_free(bb);

	return g;
}


/*
 * Set a geobbox matching a layer's extent
 */
ows_geobbox *ows_geobbox_compute(ows * o, buffer * layer_name)
{
	buffer *sql;
	PGresult *res;
	ows_geobbox *g;
	ows_bbox *bb;
	list *geom;
	list_node *ln;

	assert(o != NULL);
	assert(layer_name != NULL);

	sql = buffer_init();

	geom = ows_psql_geometry_column(o, layer_name);
	assert(geom != NULL);

	buffer_add_str(sql,
	   "SELECT xmin(g.extent), ymin(g.extent), xmax(g.extent), ymax(g.extent) FROM ");
	buffer_add_str(sql, "(SELECT Extent(transform(foo.the_geom, 4326)) ");
	buffer_add_str(sql, "FROM (");
	for (ln = geom->first; ln != NULL; ln = ln->next)
	{
		buffer_add_str(sql, " (select \"");
		buffer_copy(sql, ln->value);
		buffer_add_str(sql, "\" As \"the_geom\" FROM \"");
		buffer_copy(sql, layer_name);
		buffer_add_str(sql, "\") ");
		if (ln->next != NULL)
			buffer_add_str(sql, " union all ");
	}
	buffer_add_str(sql, " ) AS foo");
	buffer_add_str(sql, " ) AS g");

	list_free(geom);

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);

	g = ows_geobbox_init();
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		return g;
	}

	bb = ows_bbox_init();
	/* FIXME atof don't handle error */
	ows_bbox_set(o, bb,
	   atof(PQgetvalue(res, 0, 0)),
	   atof(PQgetvalue(res, 0, 1)),
	   atof(PQgetvalue(res, 0, 2)), atof(PQgetvalue(res, 0, 3)), 4326);
	PQclear(res);

	ows_geobbox_set_from_bbox(o, g, bb);
	ows_bbox_free(bb);

	return g;
}


#ifdef OWS_DEBUG
/*
 * Flush bbox value to a file (mainly to debug purpose)
 */
void ows_geobbox_flush(const ows_geobbox * g, FILE * output)
{
	assert(g != NULL);
	assert(output != NULL);

	fprintf(output, "[W:%f,E:%f,S:%f,N:%f]\n", g->west, g->east, g->south,
	   g->north);
}
#endif


/*
 * vim: expandtab sw=4 ts=4 
 */

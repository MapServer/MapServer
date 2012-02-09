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
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>

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
 * Clone a geobbox
 */
ows_geobbox *ows_geobbox_copy(ows_geobbox *g)
{
    ows_geobbox *c;

    assert(g);
    c = malloc(sizeof(g));
    return memcpy(c, g, sizeof(g));
}


/*
 * Free a geobbox structure
 */
void ows_geobbox_free(ows_geobbox *g)
{
    assert(g);
    free(g);
    g = NULL;
}


/*
 * Set a given geobbox with null area and earth limits tests
 */
bool ows_geobbox_set(ows * o, ows_geobbox * g, double west, double east, double south, double north)
{
    double geom_tolerance = 0.01;

    assert(g);

    if (south + geom_tolerance < -90.0 || south - geom_tolerance > 90.0 || 
        north + geom_tolerance < -90.0 || north - geom_tolerance > 90.0 ||
        east + geom_tolerance < -180.0 || east - geom_tolerance > 180.0 ||
        west + geom_tolerance < -180.0 || west - geom_tolerance > 180.0)
        return false;

    if (fabs(south - north) < DBL_EPSILON || fabs(east - west) < DBL_EPSILON)
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

    assert(g);
    assert(bb);

    if (bb->ymin < 0.0 && bb->ymax < 0.0) {
        south = bb->ymax;
        north = bb->ymin;
        west = bb->xmax;
        east = bb->xmin;
    } else {
        south = bb->ymin;
        north = bb->ymax;
        west = bb->xmin;
        east = bb->xmax;
    }

    return ows_geobbox_set(o, g, west, east, south, north);
}


/*
 * Set a given geobbox from a string like 'xmin,ymin,xmax,ymax'
 */
ows_geobbox *ows_geobbox_set_from_str(ows * o, ows_geobbox * g, char *str)
{
    ows_bbox *bb;

    assert(g);
    assert(str);

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
    double xmin, ymin, xmax, ymax;
    buffer *sql;
    PGresult *res;
    ows_geobbox *g;
    ows_bbox *bb;
    list *geom;
    list_node *ln;
    bool first = true;

    assert(o);
    assert(layer_name);

    sql = buffer_init();

    geom = ows_psql_geometry_column(o, layer_name);
    assert(geom);

    g = ows_geobbox_init();
    xmin = ymin = xmax = ymax = 0.0;

    for (ln = geom->first; ln ; ln = ln->next)
    {
    	buffer_add_str(sql, "SELECT ST_xmin(g), ST_ymin(g), ST_xmax(g), ST_ymax(g) FROM ");
	if (o->estimated_extent) 
	{
		buffer_add_str(sql, "(SELECT ST_Transform(ST_SetSRID(ST_Estimated_Extent('");
    		buffer_copy(sql, ows_psql_schema_name(o, layer_name));
 	   	buffer_add_str(sql, "','");
    		buffer_copy(sql, layer_name);
	    	buffer_add_str(sql, "','");
	    	buffer_copy(sql, ln->value);
   	 	buffer_add_str(sql, "'), (SELECT ST_SRID(\"");
    		buffer_copy(sql, ln->value);
    		buffer_add_str(sql, "\") FROM ");
    		buffer_copy(sql, ows_psql_schema_name(o, layer_name));
    		buffer_add_str(sql, ".\"");
    		buffer_copy(sql, layer_name);
    		buffer_add_str(sql, "\" LIMIT 1)) ,4326) AS g) AS foo");
	} else {
		buffer_add_str(sql, "(SELECT ST_Transform(ST_SetSRID(ST_Extent(\"");
	    	buffer_copy(sql, ln->value);
   	 	buffer_add_str(sql, "\"), (SELECT ST_SRID(\"");
    		buffer_copy(sql, ln->value);
    		buffer_add_str(sql, "\") FROM ");
    		buffer_copy(sql, ows_psql_schema_name(o, layer_name));
    		buffer_add_str(sql, ".\"");
    		buffer_copy(sql, layer_name);
    		buffer_add_str(sql, "\" LIMIT 1)), 4326) AS g ");
    		buffer_add_str(sql, " FROM ");
    		buffer_copy(sql, ows_psql_schema_name(o, layer_name));
    		buffer_add_str(sql, ".\"");
    		buffer_copy(sql, layer_name);
    		buffer_add_str(sql, "\" ) AS foo");
	}

        res = ows_psql_exec(o, sql->buf);
    	buffer_empty(sql);
    	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        	PQclear(res);
    		buffer_free(sql);
        	return g;
    	}

	if (first || atof(PQgetvalue(res, 0, 0)) < xmin) xmin = atof(PQgetvalue(res, 0, 0));
	if (first || atof(PQgetvalue(res, 0, 1)) < ymin) ymin = atof(PQgetvalue(res, 0, 1));
	if (first || atof(PQgetvalue(res, 0, 2)) > xmax) xmax = atof(PQgetvalue(res, 0, 2));
	if (first || atof(PQgetvalue(res, 0, 3)) > ymax) ymax = atof(PQgetvalue(res, 0, 3));

	first = false;
	PQclear(res);
    }

    buffer_free(sql);

    bb = ows_bbox_init();
    ows_bbox_set(o, bb, xmin, ymin, xmax, ymax, 4326);
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
    assert(g);
    assert(output);

    fprintf(output, "[W:%f,E:%f,S:%f,N:%f]\n", g->west, g->east, g->south, g->north);
}
#endif


/*
 * vim: expandtab sw=4 ts=4
 */

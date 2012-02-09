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
#include <string.h>
#include <math.h>

#include "ows.h"


/*
 * Initialize a bbox structure
 */
ows_bbox *ows_bbox_init()
{
    ows_bbox *b;

    b = malloc(sizeof(ows_bbox));
    assert(b);

    b->xmin = -DBL_MAX + 1;
    b->ymin = -DBL_MAX + 1;
    b->xmax = DBL_MAX - 1;
    b->ymax = DBL_MAX - 1;

    b->srs = ows_srs_init();

    return b;
}


/*
 * Free a bbox structure
 */
void ows_bbox_free(ows_bbox * b)
{
    assert(b);

    ows_srs_free(b->srs);
    free(b);
    b = NULL;
}


/*
 * Set a given bbox with null area test
 */
bool ows_bbox_set(ows * o, ows_bbox * b, double xmin, double ymin, double xmax, double ymax, int srid)
{
    assert(o);
    assert(b);

    if (xmin >= xmax || ymin >= ymax) return false;

    if (    fabs(xmin - DBL_MAX) < DBL_EPSILON
         || fabs(ymin - DBL_MAX) < DBL_EPSILON
         || fabs(xmax - DBL_MAX) < DBL_EPSILON
         || fabs(ymax - DBL_MAX) < DBL_EPSILON) return false;

    b->xmin = xmin;
    b->xmax = xmax;
    b->ymin = ymin;
    b->ymax = ymax;

    if (srid == 4326) return ows_srs_set_geobbox(o, b->srs);

    return ows_srs_set_from_srid(o, b->srs, srid);
}


/*
 * Set a given bbox from a string like 'xmin,ymin,xmax,ymax'
 */
bool ows_bbox_set_from_str(ows * o, ows_bbox * bb, const char *str, int srid)
{
    double xmin, ymin, xmax, ymax;
    ows_srs *s;
    buffer *b;
    list *l;
    int srs = srid;
    bool ret_srs = true;

    assert(o);
    assert(bb);
    assert(str);

    b = buffer_init();
    buffer_add_str(b, str);
    l = list_explode(',', b);
    buffer_free(b);

    /* srs is optional since WFS 1.1 */
    if (l->size < 4 || l->size > 5) {
        list_free(l);
        return false;
    }

    xmin = strtod(l->first->value->buf, NULL);
    ymin = strtod(l->first->next->value->buf, NULL);
    xmax = strtod(l->first->next->next->value->buf, NULL);
    ymax = strtod(l->first->next->next->next->value->buf, NULL);
    /* TODO error handling */

    /* srs is optional since WFS 1.1 */
    if (l->size == 5) {
        s = ows_srs_init();
        ret_srs = ows_srs_set_from_srsname(o, s, l->last->value->buf);
        srs = s->srid;
        ows_srs_free(s);
    }

    list_free(l);
    if(!ret_srs) return false;

    return ows_bbox_set(o, bb, xmin, ymin, xmax, ymax, srs);
}


/*
 * Set a given bbox matching a feature collection's outerboundaries
 * or a simple feature's outerboundaries
 * Bbox is set from a list containing one or several layer names
 * and optionnaly one or several WHERE SQL statement following each layer name
 */
ows_bbox *ows_bbox_boundaries(ows * o, list * from, list * where, ows_srs * srs)
{
    ows_bbox *bb;
    buffer *sql;
    list *geom;
    list_node *ln_from, *ln_where, *ln_geom;
    PGresult *res;

    assert(o);
    assert(from);
    assert(where);
    assert(srs);

    bb = ows_bbox_init();

    if (from->size != where->size) return bb;

    sql = buffer_init();
    /* Put into a buffer the SQL request calculating an extent */
    buffer_add_str(sql, "SELECT ST_xmin(g.extent), ST_ymin(g.extent), ST_xmax(g.extent), ST_ymax(g.extent) FROM ");
    buffer_add_str(sql, "(SELECT ST_Extent(foo.the_geom) as extent FROM ( ");

    /* For each layer name or each geometry column, make an union between retrieved features */
    for (ln_from = from->first, ln_where = where->first; ln_from ; ln_from = ln_from->next, ln_where = ln_where->next) {

        geom = ows_psql_geometry_column(o, ln_from->value);

        for (ln_geom = geom->first ; ln_geom ; ln_geom = ln_geom->next) {
            buffer_add_str(sql, " (SELECT ST_Transform(\"");
            buffer_copy(sql, ln_geom->value);
            buffer_add_str(sql, "\"::geometry, ");
            buffer_add_int(sql, srs->srid);
            buffer_add_str(sql, ") AS \"the_geom\" FROM ");
            buffer_copy(sql, ows_psql_schema_name(o, ln_from->value));
            buffer_add_str(sql, ".\"");
            buffer_copy(sql, ows_psql_table_name(o, ln_from->value));
            buffer_add_str(sql, "\" ");
            buffer_copy(sql, ln_where->value);
            buffer_add_str(sql, ")");

            if (ln_geom->next) buffer_add_str(sql, " UNION ALL ");
        }

        if (ln_from->next) buffer_add_str(sql, " UNION ALL ");
    }

    buffer_add_str(sql, " ) AS foo) AS g");

    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK && PQntuples(res) != 4) {
        PQclear(res);
        return bb;
    }

    bb->xmin = strtod(PQgetvalue(res, 0, 0), NULL);
    bb->ymin = strtod(PQgetvalue(res, 0, 1), NULL);
    bb->xmax = strtod(PQgetvalue(res, 0, 2), NULL);
    bb->ymax = strtod(PQgetvalue(res, 0, 3), NULL);
    /* TODO Error handling */

    ows_srs_copy(bb->srs, srs);

    PQclear(res);
    return bb;
}


/*
 * Transform a bbox from initial srid to another srid passed in parameter
 */
bool ows_bbox_transform(ows * o, ows_bbox * bb, int srid)
{
    buffer *sql;
    PGresult *res;

    assert(o);
    assert(bb);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT xmin(g), ymin(g), xmax(g), ymax(g) FROM (SELECT ST_Transform(");
    ows_bbox_to_query(o, bb, sql);
    buffer_add_str(sql, ")) AS g ) AS foo");

    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return false;
    }

    bb->xmin = strtod(PQgetvalue(res, 0, 0), NULL);
    bb->ymin = strtod(PQgetvalue(res, 0, 1), NULL);
    bb->xmax = strtod(PQgetvalue(res, 0, 2), NULL);
    bb->ymax = strtod(PQgetvalue(res, 0, 3), NULL);
    /* TODO Error Handling */

    return ows_srs_set_from_srid(o, bb->srs, srid);
}


/*
 * Transform a geobbox into a bbox
 */
bool ows_bbox_set_from_geobbox(ows * o, ows_bbox * bb, ows_geobbox * geo)
{
    assert(bb);
    assert(geo);

    if (geo->west < geo->east) {
        bb->xmin = geo->west;
        bb->xmax = geo->east;
    } else {
        bb->xmax = geo->west;
        bb->xmin = geo->east;
    }

    if (geo->north < geo->south) {
        bb->ymin = geo->north;
        bb->ymax = geo->south;
    } else {
        bb->ymax = geo->north;
        bb->ymin = geo->south;
    }

    return ows_srs_set_geobbox(o, bb->srs);
}


/*
 * Convert a bbox to PostGIS query Polygon
 */
void ows_bbox_to_query(ows *o, ows_bbox *bbox, buffer *query)
{
    double x1, y1, x2, y2;

    assert(o);
    assert(bbox);
    assert(query);

    if (bbox->srs->is_reverse_axis) {
        x1 = bbox->ymin;
        y1 = bbox->xmin;
        x2 = bbox->ymax;
        y2 = bbox->xmax;
    } else {
        x1 = bbox->xmin;
        y1 = bbox->ymin;
        x2 = bbox->xmax;
        y2 = bbox->ymax;
    }

   /* We use explicit POLYGON geometry rather than BBOX 
      related to precision handle (Float4 vs Double)    */
    buffer_add_str(query, "'SRID=");
    buffer_add_int(query, bbox->srs->srid);
    buffer_add_str(query, ";POLYGON((");
    buffer_add_double(query, x1);
    buffer_add_str(query, " ");
    buffer_add_double(query, y1);
    buffer_add_str(query, ",");
    buffer_add_double(query, x1);
    buffer_add_str(query, " ");
    buffer_add_double(query, y2);
    buffer_add_str(query, ",");
    buffer_add_double(query, x2);
    buffer_add_str(query, " ");
    buffer_add_double(query, y2);
    buffer_add_str(query, ",");
    buffer_add_double(query, x2);
    buffer_add_str(query, " ");
    buffer_add_double(query, y1);
    buffer_add_str(query, ",");
    buffer_add_double(query, x1);
    buffer_add_str(query, " ");
    buffer_add_double(query, y1);
    buffer_add_str(query, "))'::geometry");

    /* FIXME what about geography ? */
}


#ifdef OWS_DEBUG
/*
 * Flush bbox value to a file (mainly to debug purpose)
 */
void ows_bbox_flush(const ows_bbox * b, FILE * output)
{
    assert(b);

    fprintf(output, "[%g,%g,%g,%g]\n", b->xmin, b->ymin, b->xmax, b->ymax);
    ows_srs_flush(b->srs, output);
}
#endif


/*
 * vim: expandtab sw=4 ts=4
 */

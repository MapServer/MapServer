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
#include <ctype.h>
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
    assert(c);

    c->srid = -1;
    c->auth_name = buffer_init();
    c->auth_srid = 0;
    c->is_degree = true;
    c->is_reverse_axis = false;
    c->is_eastern_axis = false;
    c->is_long = false;

    return c;
}


ows_srs *ows_srs_copy(ows_srs * d, ows_srs * s)
{
    assert(s);
    assert(d);

    d->srid = s->srid;
    buffer_copy(d->auth_name, s->auth_name);
    d->auth_srid = s->auth_srid;
    d->is_degree = s->is_degree;
    d->is_reverse_axis = s->is_reverse_axis;
    d->is_eastern_axis = s->is_eastern_axis;
    d->is_long = s->is_long;

    return d;
}


/*
 * Free proj structure
 */
void ows_srs_free(ows_srs * c)
{
    assert(c);

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
    assert(c);
    assert(output);

    fprintf(output, "[\n");
    fprintf(output, " srid: %i\n", c->srid);
    fprintf(output, " auth_name: %s\n", c->auth_name->buf);
    fprintf(output, " auth_srid: %i\n", c->auth_srid);

    if (c->is_degree) fprintf(output, " is_degree: true\n]\n");
    else              fprintf(output, " is_degree: false\n]\n");

    if (c->is_reverse_axis)
        fprintf(output, " is_reverse_axis: true\n]\n");
    else
        fprintf(output, " is_reverse_axis: false\n]\n");

    if (c->is_eastern_axis)
        fprintf(output, " is_eastern_axis: true\n]\n");
    else
        fprintf(output, " is_eastern_axis: false\n]\n");

    if (c->is_long)
        fprintf(output, " is_long: true\n]\n");
    else
        fprintf(output, " is_long: false\n]\n");
}
#endif


/*
 * Set projection value into srs structure
 */
bool ows_srs_set(ows * o, ows_srs * c, const buffer * auth_name, int auth_srid)
{
    PGresult *res;
    buffer *sql;

    assert(o);
    assert(c);
    assert(o->pg);
    assert(auth_name);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT srid, position('+units=m ' in proj4text)");
    buffer_add_str(sql, ", (position('AXIS[\"X\",NORTH]]' in srtext) + position('AXIS[\"Easting\",EAST]]' in srtext))");
    buffer_add_str(sql, " FROM spatial_ref_sys WHERE auth_name='");
    buffer_copy(sql, auth_name);
    buffer_add_str(sql, "' AND auth_srid=");
    buffer_add_int(sql, auth_srid);

    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    /* If query dont return exactly 1 result, it means projection is not handled */
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return false;
    }

    buffer_empty(c->auth_name);
    buffer_copy(c->auth_name, auth_name);
    c->auth_srid = auth_srid;

    c->srid = atoi(PQgetvalue(res, 0, 0));

    /* Such a way to know if units is meter or degree */
    if (atoi(PQgetvalue(res, 0, 1)) == 0)
        c->is_degree = true;
    else
        c->is_degree = false;

    /* Is northing-easting SRID ? */
    if (atoi(PQgetvalue(res, 0, 2)) != 0)
        c->is_eastern_axis = true;

    PQclear(res);
    return true;
}


/* 
 * Aim is to avoid all pg connection, 
 * not a big deal as we already know values
 */
bool ows_srs_set_geobbox(ows * o, ows_srs * s)
{
    assert(o);
    assert(s);
    
    s->srid = 4326;
    buffer_empty(s->auth_name);
    buffer_add_str(s->auth_name, "EPSG");
    s->auth_srid = 4326;
    s->is_degree = true;
    s->is_reverse_axis = false;
    s->is_eastern_axis = false;

    return true; 
}


/*
 * Set projection value into srs structure
 */
bool ows_srs_set_from_srid(ows * o, ows_srs * s, int srid)
{
    PGresult *res;
    buffer *sql;

    assert(o);
    assert(s);

    if (srid == -1 || srid == 0) {
        s->srid = -1;
        buffer_empty(s->auth_name);
        s->auth_srid = 0;
        s->is_degree = true;
	s->is_reverse_axis = false;
        s->is_eastern_axis = false;

        return true;
    }

    sql = buffer_init();
    buffer_add_str(sql, "SELECT auth_name, auth_srid, ");
    buffer_add_str(sql, "position('+units=m ' in proj4text), ");
    buffer_add_str(sql, "(position('AXIS[\"X\",NORTH]]' in srtext) + position('AXIS[\"Easting\",EAST]]' in srtext)) ");
    buffer_add_str(sql, "FROM spatial_ref_sys WHERE srid = '");
    buffer_add_int(sql, srid);
    buffer_add_str(sql, "'");

    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    /* If query dont return exactly 1 result, it mean projection not handled */
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return false;
    }

    buffer_add_str(s->auth_name, PQgetvalue(res, 0, 0));
    s->auth_srid = atoi(PQgetvalue(res, 0, 1));
    s->srid = srid;

    /* Such a way to know if units is meter or degree */
    if (atoi(PQgetvalue(res, 0, 2)) == 0)
        s->is_degree = true;
    else
        s->is_degree = false;

    /* Is northing-easting SRID ? */
    if (atoi(PQgetvalue(res, 0, 3)) != 0)
        s->is_eastern_axis = true;

    PQclear(res);
    return true;
}


/*
 * Set projection value into srs structure
 */
bool ows_srs_set_from_srsname(ows * o, ows_srs * s, const char *srsname)
{
    char sep;
    int srid = -1;
    const char *p = NULL;

    assert(o);
    assert(s);
    assert(srsname);

    /* Severals srsName formats are available...
     *  cf WFS 1.1.0 -> 9.2 (p36)
     *  cf ISO 19142 -> 7.9.2.4.4 (p34)
     *  cf RFC 5165 <http://tools.ietf.org/html/rfc5165>
     *  cf CITE WFS-1.1 (GetFeature-tc17.2)
     */

     /* SRS pattern like:    EPSG:4326
                             urn:EPSG:geographicCRS:4326
                             urn:ogc:def:crs:EPSG:4326
                             urn:ogc:def:crs:EPSG::4326
                             urn:ogc:def:crs:EPSG:6.6:4326
                             urn:x-ogc:def:crs:EPSG:6.6:4326
                             http://www.opengis.net/gml/srs/epsg.xml#4326
     */
     s->is_long = true;

     if (!strncmp((char *) srsname,        "EPSG:", 5)) {
         sep = ':';
         s->is_reverse_axis = false;
         s->is_long = false;

     } else if (!strncmp((char *) srsname, "urn:ogc:def:crs:EPSG:", 21)
             || !strncmp((char *) srsname, "urn:x-ogc:def:crs:EPSG:", 23)
             || !strncmp((char *) srsname, "urn:EPSG:geographicCRS:", 23)) {
         sep = ':';
         s->is_reverse_axis = true;

     } else if (!strncmp((char *) srsname, "http://www.opengis.net/gml/srs/epsg.xml#", 40)) {
         sep = '#';
         s->is_reverse_axis = false;
     } else return false;

     /*  Retrieve from last separator to the end of srsName string */
     for (p = srsname ; *p ; p++);
     for (--p ; *p != sep ; p--)
         if (!isdigit(*p)) return false;
     srid = atoi(++p);

     return ows_srs_set_from_srid(o, s, srid);
}


/*
 * Check if a layer's srs has meter or degree units
 */
bool ows_srs_meter_units(ows * o, buffer * layer_name)
{
    ows_layer_node * ln;

    assert(o);
    assert(layer_name);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage && !strcmp(ln->layer->name->buf, layer_name->buf))
            return !ln->layer->storage->is_degree;

    assert(0); /* Should not happen */
    return false;
}


/*
 * Retrieve a srs from a layer
 */
int ows_srs_get_srid_from_layer(ows * o, buffer * layer_name)
{
    ows_layer_node * ln;

    assert(o);
    assert(layer_name);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage && !strcmp(ln->layer->name->buf, layer_name->buf))
            return ln->layer->storage->srid;

    return -1;
}


/*
 * Retrieve a list of srs from an srid list
 */
list *ows_srs_get_from_srid(ows * o, list * l)
{
    list_node *ln;
    buffer *b;
    list *srs;

    assert(o);
    assert(l);

    srs = list_init();

    if (l->size == 0) return srs;

    for (ln = l->first; ln ; ln = ln->next) {
        b = ows_srs_get_from_a_srid(o, atoi(ln->value->buf));
        list_add(srs, b);
    }

    return srs;
}


/*
 * Retrieve a srs from a srid
 */
buffer *ows_srs_get_from_a_srid(ows * o, int srid)
{
    buffer *b;
    buffer *sql;
    PGresult *res;

    assert(o);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT auth_name||':'||auth_srid AS srs ");
    buffer_add_str(sql, "FROM spatial_ref_sys ");
    buffer_add_str(sql, "WHERE srid=");
    buffer_add_int(sql, srid);

    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    b = buffer_init();

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
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

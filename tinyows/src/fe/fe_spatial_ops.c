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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../ows/ows.h"


/*
 * Check if the string is a spatial operator
 */
bool fe_is_spatial_op(char *name)
{
    assert(name);

    /* case sensitive comparison because the gml standard specifies
       strictly the name of the operator */
    if (    !strcmp(name, "Equals")
         || !strcmp(name, "Disjoint")
         || !strcmp(name, "Touches")
         || !strcmp(name, "Within")
         || !strcmp(name, "Overlaps")
         || !strcmp(name, "Crosses")
         || !strcmp(name, "Intersects")
         || !strcmp(name, "Contains")
         || !strcmp(name, "DWithin")
         || !strcmp(name, "Beyond")
	 || !strcmp(name, "BBOX"))
        return true;

    return false;
}


/*
 * Transform syntax coordinates from GML 2.1.2 (x1,y1 x2,y2) into Postgis (x1 y1,x2 y2)
 */
static  buffer *fe_transform_coord_gml2_to_psql(buffer * coord)
{
    size_t i;
    assert(coord);

    /*check if the first separator is a comma else do nothing */
    if (check_regexp(coord->buf, "^[0-9.-]+,")) {
        for (i = 0; i < coord->use; i++) {
            if (coord->buf[i] == ' ')       coord->buf[i] = ',';
            else if (coord->buf[i] == ',')  coord->buf[i] = ' ';
        }
    }

    return coord;
}


/*
 * Write a polygon geometry according to postgresql syntax from GML bbox
 */
buffer *fe_envelope(ows * o, buffer * typename, filter_encoding * fe, buffer *envelope, xmlNodePtr n)
{
    list *coord_min, *coord_max, *coord_pair;
    xmlChar *content, *srsname;
    buffer *name, *tmp;
    ows_bbox *bbox;
    int srid_int;
    bool ret;
    ows_srs *s = NULL;

    assert(o);
    assert(n);
    assert(fe);
    assert(typename);
    assert(envelope);

    name = buffer_init();
    buffer_add_str(name, (char *) n->name);

    srsname = xmlGetProp(n, (xmlChar *) "srsName");
    if (srsname) {
	s = ows_srs_init();

	if (!ows_srs_set_from_srsname(o, s, (char *) srsname)) {
            fe->error_code = FE_ERROR_SRS;
            xmlFree(srsname);
            buffer_free(name);
            ows_srs_free(s);
            return envelope;
        }

        xmlFree(srsname);
        srid_int = s->srid;
    } else {
        if (o->request->request.wfs->srs)
            srid_int = o->request->request.wfs->srs->srid;
        else
            srid_int = ows_srs_get_srid_from_layer(o, typename);
    }

    n = n->children;
    while (n->type != XML_ELEMENT_NODE) n = n->next; /* Jump to next element if spaces */

    content = xmlNodeGetContent(n->children);

    /* GML3 */
    if (!strcmp((char *) n->name, "lowerCorner")) {
        if (!content || !check_regexp((char *) content, "[-]?[0-9]+([.][0-9]+)?([eE][-]?[0-9]+)? [-]?[0-9]+([.][0-9]+)?([eE][-]?[0-9]+)?")) {
            xmlFree(content);
            buffer_free(name);
            if (s) ows_srs_free(s);
            fe->error_code = FE_ERROR_BBOX;

	    return envelope;
        }
        coord_min = list_explode_str(' ', (char *) content);


        n = n->next;
        while (n->next && n->type != XML_ELEMENT_NODE) n = n->next; /* Jump to next element if spaces */
        xmlFree(content);
        content = xmlNodeGetContent(n->children);

        if (!content || !check_regexp((char *) content, "[-]?[0-9]+([.][0-9]+)?([eE][-]?[0-9]+)? [-]?[0-9]+([.][0-9]+)?([eE][-]?[0-9]+)?")) {
            xmlFree(content);
            buffer_free(name);
            list_free(coord_min);
            if (s) ows_srs_free(s);
            fe->error_code = FE_ERROR_BBOX;

	    return envelope;
        }
        coord_max = list_explode_str(' ', (char *) content);

    /* GML2 */
    } else if (!strcmp((char *) n->name, "coordinates")) {
        tmp = buffer_init();
        buffer_add_str(tmp, (char *) content);
        if (!check_regexp((char *) content, "^[-]?[0-9]+([.][0-9]+)?([eE][-]?[0-9]+)?,[-]?[0-9]+([.][0-9]+)?([eE][-]?[0-9]+)?[ ][-]?[0-9]+([.][0-9]+)?([eE][-]?[0-9]+)?,[-]?[0-9]+([.][0-9]+)?([eE][-]?[0-9]+)?$")) {
            xmlFree(content);
            buffer_free(name);
            if (s) ows_srs_free(s);
            fe->error_code = FE_ERROR_BBOX;

	    return envelope;
        }
        tmp = fe_transform_coord_gml2_to_psql(tmp);
        coord_pair = list_explode(',', tmp);
        coord_min = list_explode(' ', coord_pair->first->value);
        coord_max = list_explode(' ', coord_pair->first->next->value);
        buffer_free(tmp);
        list_free(coord_pair);
    } else {
    /* FIXME handle coord and pos */
        xmlFree(content);
        buffer_free(name);
        if (s) ows_srs_free(s);
        fe->error_code = FE_ERROR_BBOX;

        return envelope;
    }

    buffer_free(name);
    xmlFree(content);

    /* return the polygon's coordinates matching the bbox */
    bbox = ows_bbox_init();
    if (s && s->is_reverse_axis) {
    	ret = ows_bbox_set(o, bbox,
                          atof(coord_min->first->next->value->buf),
			  atof(coord_min->first->value->buf),
                          atof(coord_max->first->next->value->buf),
                          atof(coord_max->first->value->buf),
			  srid_int);
    } else {
    	ret = ows_bbox_set(o, bbox,
			  atof(coord_min->first->value->buf),
                          atof(coord_min->first->next->value->buf),
                          atof(coord_max->first->value->buf),
                          atof(coord_max->first->next->value->buf),
			  srid_int);
    }

    if (!ret) {
        fe->error_code = FE_ERROR_BBOX;
	list_free(coord_min);
        list_free(coord_max);
        ows_bbox_free(bbox);
        if (s) ows_srs_free(s);

	return envelope;
    }

    list_free(coord_min);
    list_free(coord_max);

    ows_bbox_to_query(o, bbox, envelope);
    ows_bbox_free(bbox);
    if (s) ows_srs_free(s);

    return envelope;
}


/*
 * Return the SQL request matching the spatial operator
 */
static buffer *fe_spatial_functions(ows * o, buffer * typename, filter_encoding * fe, xmlNodePtr n)
{
    ows_srs *s;
    buffer *geom;
    xmlNodePtr p;
    xmlChar *srsname;
    int srid = -1;

    assert(typename);
    assert(fe);
    assert(n);
    assert(o);

    if (!strcmp((char *) n->name, "Equals"))     buffer_add_str(fe->sql, " ST_Equals(");
    if (!strcmp((char *) n->name, "Disjoint"))   buffer_add_str(fe->sql, " ST_Disjoint(");
    if (!strcmp((char *) n->name, "Touches"))    buffer_add_str(fe->sql, " ST_Touches(");
    if (!strcmp((char *) n->name, "Within"))     buffer_add_str(fe->sql, " ST_Within(");
    if (!strcmp((char *) n->name, "Overlaps"))   buffer_add_str(fe->sql, " ST_Overlaps(");
    if (!strcmp((char *) n->name, "Crosses"))    buffer_add_str(fe->sql, " ST_Crosses(");
    if (!strcmp((char *) n->name, "Intersects")) buffer_add_str(fe->sql, " ST_Intersects(");
    if (!strcmp((char *) n->name, "Contains"))   buffer_add_str(fe->sql, " ST_Contains(");

    n = n->children;
    while (n->type != XML_ELEMENT_NODE) n = n->next; /* Jump to next element if spaces */

    p = n;
    n = n->next;

    /* jump to the next element if there are spaces */
    while (n->type != XML_ELEMENT_NODE) n = n->next;

    if (o->request->request.wfs->srs) srid = o->request->request.wfs->srs->srid;
    else srid = ows_srs_get_srid_from_layer(o, typename);

    if (!strcmp((char *) n->name, "Box") || !strcmp((char *) n->name, "Envelope")) {
        
        srsname = xmlGetProp(n, (xmlChar *) "srsName");
        if (srsname) {
	    s = ows_srs_init();
	    if (ows_srs_set_from_srsname(o, s, (char *) srsname)) srid = s->srid;
            ows_srs_free(s);
            xmlFree(srsname);
        }

        buffer_add(fe->sql, '"');
        fe->sql = fe_property_name(o, typename, fe, fe->sql, p, true, true);
        buffer_add(fe->sql, '"');
        buffer_add(fe->sql, ',');

        if (srid != ows_srs_get_srid_from_layer(o, typename)) {
            buffer_add_str(fe->sql, "ST_Transform(");
            fe->sql = fe_envelope(o, typename, fe, fe->sql, n);
            buffer_add(fe->sql, ',');
            buffer_add_int(fe->sql, ows_srs_get_srid_from_layer(o, typename));
            buffer_add(fe->sql, ')');
        } else fe->sql = fe_envelope(o, typename, fe, fe->sql, n);

   } else  {
        geom = ows_psql_gml_to_sql(o, n, srid);
        if (!geom) {
            fe->error_code = FE_ERROR_GEOMETRY;
            return fe->sql;
        }

        srid = ows_psql_geometry_srid(o, geom->buf);

        buffer_add(fe->sql, '"');
        fe->sql = fe_property_name(o, typename, fe, fe->sql, p, true, true);
        buffer_add(fe->sql, '"');
        buffer_add(fe->sql, ',');

        if (srid != ows_srs_get_srid_from_layer(o, typename))
            buffer_add_str(fe->sql, "ST_Transform(");

        buffer_add_str(fe->sql, "ST_SetSRID('");
        buffer_copy(fe->sql, geom);
        buffer_add_str(fe->sql, "'::geometry,");
        buffer_add_int(fe->sql, srid);
        buffer_add(fe->sql, ')');
        buffer_free(geom);

        if (srid != ows_srs_get_srid_from_layer(o, typename)) {
            buffer_add(fe->sql, ',');
            buffer_add_int(fe->sql, ows_srs_get_srid_from_layer(o, typename));
            buffer_add(fe->sql, ')');
        }
    }

    buffer_add(fe->sql, ')');

    return fe->sql;
}


/*
 * DWithin and Beyond operators : test if a geometry A is within (or beyond)
 * a specified distance of a geometry B
 */
static buffer *fe_distance_functions(ows * o, buffer * typename, filter_encoding * fe, xmlNodePtr n)
{
    xmlChar *content, *units;
    buffer *tmp, *op, *sql;
    float km;

    assert(o);
    assert(typename);
    assert(fe);
    assert(n);

    tmp = NULL;
    op = buffer_init();

    if (!strcmp((char *) n->name, "Beyond"))  buffer_add_str(op, " > ");
    if (!strcmp((char *) n->name, "DWithin")) buffer_add_str(op, " < ");

/* FIXME: as geography support available no need to keep this hack ! */

    /* parameters are passed with centroid function because
       Distance_sphere parameters must be points
       So to be able to calculate distance between lines or polygons
       whose coordinates are degree, centroid function must be used
       To be coherent, centroid is also used with Distance function */
    if (ows_srs_meter_units(o, typename))
        buffer_add_str(fe->sql, "ST_Distance(ST_centroid(");
    else
        buffer_add_str(fe->sql, "ST_Distance_sphere(ST_centroid(");

    n = n->children;
    while (n->type != XML_ELEMENT_NODE) n = n->next; /* Jump to next element if spaces */

    buffer_add(fe->sql, '"');
    fe->sql = fe_property_name(o, typename, fe, fe->sql, n, true, true);
    buffer_add(fe->sql, '"');

    buffer_add_str(fe->sql, "),ST_centroid('");

    n = n->next;

    while (n->type != XML_ELEMENT_NODE) n = n->next;

    /* display the geometry */
    sql = ows_psql_gml_to_sql(o, n, 0);
    if (sql) {
        buffer_copy(fe->sql, sql);
        buffer_free(sql);
    } else fe->error_code = FE_ERROR_GEOMETRY;

    buffer_add_str(fe->sql, "'))");

    n = n->next;

    while (n->type != XML_ELEMENT_NODE) n = n->next;

    units = xmlGetProp(n, (xmlChar *) "units");
    buffer_copy(fe->sql, op);
    content = xmlNodeGetContent(n->children);
/*FIXME add a regex check */

    /* units not strictly defined in Filter Encoding specification */
    if (!strcmp((char *) units, "meters") || !strcmp((char *) units, "#metre"))
        buffer_add_str(fe->sql, (char *) content);
    else if (!strcmp((char *) units, "kilometers") || !strcmp((char *) units, "#kilometre")) {
        km = atof((char *) content) * 1000.0;
        tmp = buffer_ftoa((double) km);
        buffer_copy(fe->sql, tmp);
        buffer_free(tmp);
    } else {
        fe->error_code = FE_ERROR_UNITS;
    }

    buffer_free(op);
    xmlFree(content);
    xmlFree(units);

    return fe->sql;
}


static buffer *fe_bbox_layer(ows *o, buffer *typename, buffer *sql, buffer *propertyname, buffer *envelope)
{
    int srid = -1;
    bool transform = false;

    assert(propertyname);
    assert(envelope);
    assert(sql);
    assert(o);

    buffer_add_str(sql, "(_ST_Intersects(");

    if (o->request->request.wfs->srs) {
        srid = o->request->request.wfs->srs->srid;
        transform = true;
    }

    if (transform) buffer_add_str(sql, "ST_Transform(");

    buffer_add(sql, '"');
    buffer_copy(sql, propertyname);
    buffer_add(sql, '"');

    if (transform) {
        buffer_add(sql, ',');
        buffer_add_int(sql, srid);
    }
    buffer_add(sql, ')');

    buffer_add_str(sql, ",");
    buffer_copy(sql, envelope);
    buffer_add_str(sql, ") AND ");
    if (transform) buffer_add_str(sql, "ST_Transform(");

    buffer_add(sql, '"');
    buffer_copy(sql, propertyname);
    buffer_add(sql, '"');

    if (transform) {
        buffer_add(sql, ',');
        buffer_add_int(sql, srid);
    }
    buffer_add(sql, ')');

    buffer_add_str(sql, " && ");
    buffer_copy(sql, envelope);
    buffer_add_str(sql, ")");
      
    return sql;
}


/*
 * BBOX operator : identify all geometries that spatially interact with the specified box
 */
static buffer *fe_bbox(ows * o, buffer * typename, filter_encoding * fe, xmlNodePtr n)
{
    buffer *property;
    list *columns;
    list_node *ln;
    buffer *envelope = NULL;

    assert(o);
    assert(typename);
    assert(fe);
    assert(n);

    n = n->children;
    while (n->type != XML_ELEMENT_NODE) n = n->next;

    columns = ows_psql_geometry_column(o, typename);

    /* Retrieve the property name */
    property = buffer_init();
    property = fe_property_name(o, typename, fe, property, n, true, false);
    if (fe->error_code != FE_NO_ERROR) {
        buffer_free(property);
        return fe->sql;
    }

    /* If no property name, so we have to check with each Geometry_columns */
    if (property->use == 0) {

        /* retrieve the geometry matching the bbox */
        if (!strcmp((char *) n->name, "Box") || !strcmp((char *) n->name, "Envelope")) {
            envelope = buffer_init();
            envelope = fe_envelope(o, typename, fe, envelope, n);
        } else { fe->error_code = FE_ERROR_FILTER; }

        buffer_add(fe->sql, '(');
        for (ln = columns->first ; ln ; ln = ln->next) {
            if (envelope) fe->sql = fe_bbox_layer(o, typename, fe->sql, ln->value, envelope);
            if      (ln->next && (fe->in_not == 0 || !fe->in_not%2)) buffer_add_str(fe->sql, " OR ");
            else if (ln->next && fe->in_not%2)                       buffer_add_str(fe->sql, " AND ");
            else                                                     buffer_add_str(fe->sql, ")");
        }

    } else {
        n = n->next;
        while (n->type != XML_ELEMENT_NODE) n = n->next;

        if (!strcmp((char *) n->name, "Box") || !strcmp((char *) n->name, "Envelope")) {
            if (!in_list(columns, property)) {
                  buffer_free(property);
                  fe->error_code = FE_ERROR_GEOM_PROPERTYNAME;
                  return fe->sql;
            }
            envelope = buffer_init();
            envelope = fe_envelope(o, typename, fe, envelope, n);
        } else { fe->error_code = FE_ERROR_FILTER; }

        if (envelope) fe->sql = fe_bbox_layer(o, typename, fe->sql, property, envelope);
    }

    if (envelope) buffer_free(envelope);
    buffer_free(property);
    return fe->sql;
}


/*
 * Print the SQL request matching spatial function (Equals,Disjoint, etc)
 * Warning : before calling this function,
 * Check if the node name is a spatial operator with fe_is_spatial_op()
 */
buffer *fe_spatial_op(ows * o, buffer * typename, filter_encoding * fe, xmlNodePtr n)
{
    assert(o);
    assert(typename);
    assert(fe);
    assert(n);

    /* case sensitive comparison because the gml standard specifies
       strictly the name of the operator */
    if (    !strcmp((char *) n->name, "Equals") 
         || !strcmp((char *) n->name, "Disjoint")
         || !strcmp((char *) n->name, "Touches")
         || !strcmp((char *) n->name, "Within")
         || !strcmp((char *) n->name, "Overlaps")
         || !strcmp((char *) n->name, "Crosses")
         || !strcmp((char *) n->name, "Intersects")
         || !strcmp((char *) n->name, "Contains"))
        fe->sql = fe_spatial_functions(o, typename, fe, n);
    else if (!strcmp((char *) n->name, "DWithin") || !strcmp((char *) n->name, "Beyond"))
        fe->sql = fe_distance_functions(o, typename, fe, n);
    else if (!strcmp((char *) n->name, "BBOX"))
        fe->sql = fe_bbox(o, typename, fe, n);
    else
        fe->error_code = FE_ERROR_FILTER;

    return fe->sql;
}


/*
 * vim: expandtab sw=4 ts=4
 */

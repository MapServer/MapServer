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
	assert(name != NULL);

	/* case sensitive comparison because the gml standard specifies 
	   strictly the name of the operator */
	if (strcmp(name, "Equals") == 0
	   || strcmp(name, "Disjoint") == 0
	   || strcmp(name, "Touches") == 0
	   || strcmp(name, "Within") == 0
	   || strcmp(name, "Overlaps") == 0
	   || strcmp(name, "Crosses") == 0
	   || strcmp(name, "Intersects") == 0
	   || strcmp(name, "Contains") == 0
	   || strcmp(name, "DWithin") == 0
	   || strcmp(name, "Beyond") == 0 || strcmp(name, "BBOX") == 0)
		return true;

	return false;
}


/*
 * Write a polygon geometry according to postgresql syntax from GML bbox 
 */
buffer *fe_envelope(ows * o, buffer * typename, filter_encoding * fe,
   xmlNodePtr n)
{
	xmlChar *content, *srsname;
	buffer *srid, *name, *tmp;
	list *coord_min, *coord_max, *coord_pair;

	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	name = buffer_init();
	buffer_add_str(name, (char *) n->name);

	/* BBOX is transformed into a polygon so that point corners are included */
	buffer_add_str(fe->sql, "setsrid('POLYGON((");

	srid = ows_srs_get_srid_from_layer(o, typename);
	srsname = xmlGetProp(n, (xmlChar *) "srsName");

	if (srsname != NULL)
	{
		if (!check_regexp((char *) srsname, srid->buf))
		{
			fe->error_code = FE_ERROR_SRS;
			xmlFree(srsname);
			buffer_free(name);
			buffer_free(srid);
			return fe->sql;
		}
	}

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	content = xmlNodeGetContent(n->children);

	/* GML3 */
	if (buffer_cmp(name, "Envelope"))
	{
		coord_min = list_explode_str(' ', (char *) content);

		n = n->next;
		/* jump to the next element if there are spaces */
		while (n->type != XML_ELEMENT_NODE)
			n = n->next;
		xmlFree(content);
		content = xmlNodeGetContent(n->children);

		coord_max = list_explode_str(' ', (char *) content);
	}
	/* GML2 */
	else
	{
		tmp = buffer_init();
		buffer_add_str(tmp, (char *) content);

		tmp = fe_transform_coord_gml_to_psql(tmp);

		coord_pair = list_explode(',', tmp);
		coord_min = list_explode(' ', coord_pair->first->value);
		coord_max = list_explode(' ', coord_pair->first->next->value);
		buffer_free(tmp);
		list_free(coord_pair);
	}

	/* display the polygon's coordinates matching the bbox */
	buffer_copy(fe->sql, coord_min->first->value);
	buffer_add_str(fe->sql, " ");
	buffer_copy(fe->sql, coord_min->first->next->value);
	buffer_add_str(fe->sql, ",");
	buffer_copy(fe->sql, coord_max->first->value);
	buffer_add_str(fe->sql, " ");
	buffer_copy(fe->sql, coord_min->first->next->value);
	buffer_add_str(fe->sql, ",");
	buffer_copy(fe->sql, coord_max->first->value);
	buffer_add_str(fe->sql, " ");
	buffer_copy(fe->sql, coord_max->first->next->value);
	buffer_add_str(fe->sql, ",");
	buffer_copy(fe->sql, coord_min->first->value);
	buffer_add_str(fe->sql, " ");
	buffer_copy(fe->sql, coord_max->first->next->value);
	buffer_add_str(fe->sql, ",");
	buffer_copy(fe->sql, coord_min->first->value);
	buffer_add_str(fe->sql, " ");
	buffer_copy(fe->sql, coord_min->first->next->value);

	list_free(coord_min);
	list_free(coord_max);

	buffer_add_str(fe->sql, "))'::geometry,");

	buffer_copy(fe->sql, srid);
	buffer_add_str(fe->sql, ")");

	xmlFree(srsname);
	xmlFree(content);
	buffer_free(srid);
	buffer_free(name);

	return fe->sql;
}


/*
 * Transform syntax coordinates from GML 2.1.2 (x1,y1 x2,y2) into Postgis (x1 y1,x2 y2)
 */
buffer *fe_transform_coord_gml_to_psql(buffer * coord)
{
	size_t i;

	assert(coord != NULL);

	/*check if the first separator is a comma else do nothing */
	if (check_regexp(coord->buf, "^[0-9.-]+,"))
	{
		for (i = 0; i < coord->use; i++)
		{
			if (coord->buf[i] == ' ')
				coord->buf[i] = ',';
			else if (coord->buf[i] == ',')
				coord->buf[i] = ' ';
		}
	}

	return coord;
}


/*
 * Transform syntax coordinates from GML 3.1.1 (x1 y1 x2 y2) into Postgis' (x1 y1,x2 y2)
 */
buffer *fe_transform_coord_gml3_to_psql(buffer * coord)
{
	size_t i;
	int nb_spaces;

	assert(coord != NULL);

	nb_spaces = 0;

	for (i = 0; i < coord->use; i++)
	{
		if (coord->buf[i] == ' ')
		{
			nb_spaces++;
			/* transform the second space separator into comma */
			if (nb_spaces == 2)
			{
				coord->buf[i] = ',';
				nb_spaces = 0;
			}
		}
	}

	return coord;
}


/*
 * Transform a geometry expressed in GML into Postgis geometry syntax
 */
buffer *fe_transform_geometry_gml_to_psql(ows * o, buffer * typename,
   filter_encoding * fe, xmlNodePtr n)
{
	xmlChar *content;
	xmlNodePtr node, node_coord;
	buffer *srid, *tmp, *geom_type;

	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	content = NULL;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	buffer_add_str(fe->sql, "setsrid('");
	buffer_add_str(fe->sql, (char *) n->name);

	geom_type = buffer_init();
	buffer_add_str(geom_type, (char *) n->name);

	/* print the geometry type */
	if (buffer_cmp(geom_type, "MultiPolygon"))
		buffer_add_str(fe->sql, "(((");
	else if (buffer_cmp(geom_type, "MultiLineString")
	   || buffer_cmp(geom_type, "Polygon"))
		buffer_add_str(fe->sql, "((");
	else if (buffer_cmp(geom_type, "MultiPoint")
	   || buffer_cmp(geom_type, "LineString")
	   || buffer_cmp(geom_type, "Point"))
		buffer_add_str(fe->sql, "(");
	else
	{
		fe->error_code = FE_ERROR_GEOMETRY;
		buffer_free(geom_type);
		return fe->sql;
	}

	n = n->children;
	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	/* print the coordinates */
	for (node = n; node != NULL; node = node->next)
	{
		if (node->type == XML_ELEMENT_NODE)
		{
			node_coord = node;
			/* go to node <coordinates> */
			while (strcmp((char *) node_coord->name, "coordinates") != 0
			   && strcmp((char *) node_coord->name, "posList") != 0
			   && strcmp((char *) node_coord->name, "pos") != 0)
			{
				/* jump to the next element if there are spaces */
				while (node_coord->type != XML_ELEMENT_NODE)
					node_coord = node_coord->next;
				if (strcmp((char *) node_coord->name, "coordinates") != 0
				   && strcmp((char *) node_coord->name, "posList") != 0
				   && strcmp((char *) node_coord->name, "pos") != 0)
					node_coord = node_coord->children;
			}

			tmp = buffer_init();

			content = xmlNodeGetContent(node_coord);
			buffer_add_str(tmp, (char *) content);
			if (strcmp((char *) node_coord->name, "coordinates") == 0)
				tmp = fe_transform_coord_gml_to_psql(tmp);
			else
				tmp = fe_transform_coord_gml3_to_psql(tmp);
			buffer_copy(fe->sql, tmp);
			buffer_free(tmp);
			xmlFree(content);
		}
		if (node->next != NULL)
		{
			if (node->next->type == XML_ELEMENT_NODE)
			{
				if (strcmp((char *) node->next->name,
					  "lineStringMember") == 0)
					buffer_add_str(fe->sql, "),(");
				else if (strcmp((char *) node->next->name,
					  "polygonMember") == 0)
					buffer_add_str(fe->sql, ")),((");
				else
					buffer_add_str(fe->sql, ",");
			}
		}
	}
	/* print the geometry type */
	if (buffer_cmp(geom_type, "MultiPolygon"))
		buffer_add_str(fe->sql, ")))");
	else if (buffer_cmp(geom_type, "MultiLineString")
	   || buffer_cmp(geom_type, "Polygon"))
		buffer_add_str(fe->sql, "))");
	else if (buffer_cmp(geom_type, "MultiPoint")
	   || buffer_cmp(geom_type, "LineString")
	   || buffer_cmp(geom_type, "Point"))
		buffer_add_str(fe->sql, ")");
	else
	{
		fe->error_code = FE_ERROR_GEOMETRY;
		buffer_free(geom_type);
		return fe->sql;
	}

	buffer_add_str(fe->sql, "'::geometry,");

	/* print the srid */
	srid = ows_srs_get_srid_from_layer(o, typename);
	buffer_copy(fe->sql, srid);
	buffer_add_str(fe->sql, ")");

	buffer_free(srid);
	buffer_free(geom_type);

	return fe->sql;
}


/* 
 * Return the SQL request matching the spatial operator
 */
static buffer *fe_spatial_functions(ows * o, buffer * typename,
   filter_encoding * fe, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	if (strcmp((char *) n->name, "Equals") == 0)
		buffer_add_str(fe->sql, " Equals(");
	if (strcmp((char *) n->name, "Disjoint") == 0)
		buffer_add_str(fe->sql, " Disjoint(");
	if (strcmp((char *) n->name, "Touches") == 0)
		buffer_add_str(fe->sql, " Touches(");
	if (strcmp((char *) n->name, "Within") == 0)
		buffer_add_str(fe->sql, " Within(");
	if (strcmp((char *) n->name, "Overlaps") == 0)
		buffer_add_str(fe->sql, " Overlaps(");
	if (strcmp((char *) n->name, "Crosses") == 0)
		buffer_add_str(fe->sql, " Crosses(");
	if (strcmp((char *) n->name, "Intersects") == 0)
		buffer_add_str(fe->sql, " Intersects(");
	if (strcmp((char *) n->name, "Contains") == 0)
		buffer_add_str(fe->sql, " Contains(");

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	fe->sql = fe_property_name(o, typename, fe, fe->sql, n);

	n = n->next;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	buffer_add_str(fe->sql, ",");
	if (strcmp((char *) n->name, "Box") == 0
	   || strcmp((char *) n->name, "Envelope") == 0)
		fe->sql = fe_envelope(o, typename, fe, n);
	else
		fe->sql = fe_transform_geometry_gml_to_psql(o, typename, fe, n);

	buffer_add_str(fe->sql, ")");

	return fe->sql;
}


/*
 * DWithin and Beyond operators : test if a geometry A is within (or beyond) 
 * a specified distance of a geometry B 
 */
static buffer *fe_distance_functions(ows * o, buffer * typename,
   filter_encoding * fe, xmlNodePtr n)
{
	xmlChar *content, *units;
	float km;
	buffer *tmp, *op;

	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	tmp = NULL;
	op = buffer_init();

	if (strcmp((char *) n->name, "Beyond") == 0)
		buffer_add_str(op, " > ");
	if (strcmp((char *) n->name, "DWithin") == 0)
		buffer_add_str(op, " < ");

	/* parameters are passed with centroid function because 
	   Distance_sphere parameters must be points
	   So to be able to calculate distance between lines or polygons 
	   whose coordinates are degree, centroid function must be used 
	   To be coherent, centroid is also used with Distance function */
	if (ows_srs_meter_units(o, typename))
		buffer_add_str(fe->sql, "Distance(centroid(");
	else
		buffer_add_str(fe->sql, "Distance_sphere(centroid(");

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	/* display the property name */
	fe->sql = fe_property_name(o, typename, fe, fe->sql, n);


	buffer_add_str(fe->sql, "),centroid(");

	n = n->next;

	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	/* display the geometry */
	fe->sql = fe_transform_geometry_gml_to_psql(o, typename, fe, n);

	buffer_add_str(fe->sql, "))");

	n = n->next;

	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	units = xmlGetProp(n, (xmlChar *) "units");
	buffer_copy(fe->sql, op);
	content = xmlNodeGetContent(n->children);

	/* units not strictly defined in Filter Encoding specification */
	if (strcmp((char *) units, "meters") == 0
	   || strcmp((char *) units, "#metre") == 0)
		buffer_add_str(fe->sql, (char *) content);
	else if (strcmp((char *) units, "kilometers") == 0
	   || strcmp((char *) units, "#kilometre") == 0)
	{
		km = atof((char *) content) * 1000.0;
		tmp = buffer_ftoa((double) km);
		buffer_copy(fe->sql, tmp);
		buffer_free(tmp);
	}
	else
	{
		fe->error_code = FE_ERROR_UNITS;
	}

	buffer_free(op);
	xmlFree(content);
	xmlFree(units);

	return fe->sql;
}


/* 
 * BBOX operator : identify all geometries that spatially interact with the specified box 
 */
static buffer *fe_bbox(ows * o, buffer * typename, filter_encoding * fe,
   xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	buffer_add_str(fe->sql, "not(disjoint(");

	n = n->children;

	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	/* display the property name */
	fe->sql = fe_property_name(o, typename, fe, fe->sql, n);

	n = n->next;

	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	buffer_add_str(fe->sql, ",");

	/* display the geometry matching the bbox */
	if (strcmp((char *) n->name, "Box") == 0
	   || strcmp((char *) n->name, "Envelope") == 0)
		fe->sql = fe_envelope(o, typename, fe, n);
	buffer_add_str(fe->sql, "))");

	return fe->sql;
}


/* 
 * Print the SQL request matching spatial function (Equals,Disjoint, etc)
 * Warning : before calling this function, 
 * Check if the node name is a spatial operator with fe_is_spatial_op() 
 */
buffer *fe_spatial_op(ows * o, buffer * typename, filter_encoding * fe,
   xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	/* case sensitive comparison because the gml standard specifies 
	   strictly the name of the operator */
	if (strcmp((char *) n->name, "Equals") == 0
	   || strcmp((char *) n->name, "Disjoint") == 0
	   || strcmp((char *) n->name, "Touches") == 0
	   || strcmp((char *) n->name, "Within") == 0
	   || strcmp((char *) n->name, "Overlaps") == 0
	   || strcmp((char *) n->name, "Crosses") == 0
	   || strcmp((char *) n->name, "Intersects") == 0
	   || strcmp((char *) n->name, "Contains") == 0)
		fe->sql = fe_spatial_functions(o, typename, fe, n);
	else if (strcmp((char *) n->name, "DWithin") == 0
	   || strcmp((char *) n->name, "Beyond") == 0)
		fe->sql = fe_distance_functions(o, typename, fe, n);
	else if (strcmp((char *) n->name, "BBOX") == 0)
		fe->sql = fe_bbox(o, typename, fe, n);
	else
		fe->error_code = FE_ERROR_FILTER;

	return fe->sql;
}


/*
 * vim: expandtab sw=4 ts=4
 */

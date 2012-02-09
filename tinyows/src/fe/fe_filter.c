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
 * Initialize filter_encoding structure
 */
filter_encoding *filter_encoding_init()
{
	filter_encoding *fe;

	fe = malloc(sizeof(filter_encoding));
	assert(fe != NULL);

	fe->sql = NULL;
	fe->error_code = FE_NO_ERROR;

	return fe;
}


/*
 * Release filter_encoding structure
 */
void filter_encoding_free(filter_encoding * fe)
{
	assert(fe != NULL);

	if (fe->sql != NULL)
		buffer_free(fe->sql);

	free(fe);
	fe = NULL;
}


#ifdef OWS_DEBUG
/*
 * Print filter_encoding structure
 */
void filter_encoding_flush(ows * o, filter_encoding * fe, FILE * output)
{
	assert(fe != NULL);
	assert(output != NULL);

	fprintf(output, "[\n");

	if (fe->sql != NULL)
	{
		fprintf(output, "sql -> ");
		buffer_flush(fe->sql, output);
		fprintf(o->output, "\n");
	}

	fprintf(output, " error code -> %d\n", fe->error_code);

	fprintf(output, "]\n");
}
#endif


/*
 * Parse an XML node, used for debug purposes
 */
void fe_node_flush(xmlNodePtr node, FILE * output)
{
	xmlNodePtr n;
	xmlAttr *att;
	xmlChar *content;

	assert(node != NULL);
	assert(output != NULL);

	content = NULL;

	/* parse the node */
	if (node->type == XML_ELEMENT_NODE)
	{
		for (n = node; n; n = n->next)
		{
			/* print the node name */
			fprintf(output, "name:%s\n", n->name);
			/* print the node properties */
			for (att = n->properties; att != NULL; att = att->next)
			{

				fprintf(output, "%s=%s\n", att->name,
				   (char *) xmlNodeGetContent(att->children));
			}
			if (n->children)
			{
				fe_node_flush(n->children, output);
			}
		}
	}
	/* print the node content */
	else if ((node->type == XML_CDATA_SECTION_NODE)
	   || (node->type == XML_TEXT_NODE))
	{
		content = xmlNodeGetContent(node);

		fprintf(output, "%s\n", content);

		xmlFree(content);
	}
}


/*
 * Recursive function which eval a prefixed expression and 
 * return the matching string
 */
buffer *fe_expression(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	xmlChar *content;

	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(sql != NULL);

	if (n == NULL)
		return sql;

	if (strcmp((char *) n->name, "Fe_Function") == 0)
	{
		sql = fe_function(o, typename, fe, sql, n);
		return sql;
	}

	/* open a bracket when there are grandchildren elements */
	if (n->children != NULL)
	{
		if (n->children->type == XML_ELEMENT_NODE)
		{
			if (n->children->children != NULL)
				buffer_add_str(sql, "(");
		}
		else
		{
			if (n->children->next != NULL)
				if (n->children->next->children != NULL)
					buffer_add_str(sql, "(");
		}
	}

	/* eval the left part of the expression */
	sql = fe_expression(o, typename, fe, sql, n->children);

	/* print the node */
	content = xmlNodeGetContent(n);
	if (strcmp((char *) n->name, "Add") == 0)
		buffer_add_str(sql, " + ");
	else if (strcmp((char *) n->name, "Sub") == 0)
		buffer_add_str(sql, " - ");
	else if (strcmp((char *) n->name, "Mul") == 0)
		buffer_add_str(sql, " * ");
	else if (strcmp((char *) n->name, "Div") == 0)
		buffer_add_str(sql, " / ");
	else if (strcmp((char *) n->name, "Literal") == 0)
	{
		/* strings must be written in quotation marks */
		if (check_regexp((char *) content, "^ + $") == 1)
			buffer_add_str(sql, "''");
		else
		{
			if (check_regexp((char *) content, "^[A-Za-z]") == 1
			   || check_regexp((char *) content, ".*-.*") == 1)
				buffer_add_str(sql, "'");

			buffer_add_str(sql, (char *) content);

			if (check_regexp((char *) content, "^[A-Za-z]") == 1
			   || check_regexp((char *) content, ".*-.*") == 1)
				buffer_add_str(sql, "'");
		}
	}
	else if (strcmp((char *) n->name, "PropertyName") == 0)
		sql = fe_property_name(o, typename, fe, sql, n);
	else if (n->type != XML_ELEMENT_NODE)
		sql = fe_expression(o, typename, fe, sql, n->next);
	xmlFree(content);

	/* eval the right part of the expression */
	if (n->children != NULL)
	{
		if (n->children->next != NULL)
		{
			if (n->children->type == XML_ELEMENT_NODE
			   && n->children->next->type == XML_ELEMENT_NODE)
				sql =
				   fe_expression(o, typename, fe, sql, n->children->next);
			else
				sql =
				   fe_expression(o, typename, fe, sql,
				   n->children->next->next);
			content = xmlNodeGetContent(n->children->next);
			/* close a bracket when there are not empty children elements */
			if (check_regexp((char *) content, " +") != 1)
				buffer_add_str(sql, ")");
			xmlFree(content);
		}
	}

	return sql;
}


/* 
 * Transform an Xpath expression into a string propertyname
 */
buffer *fe_xpath_property_name(ows * o, buffer * typename,
   buffer * property)
{
	buffer *prop_name;

	assert(o != NULL);
	assert(typename != NULL);
	assert(property != NULL);

	if (check_regexp(property->buf, "\\*\\[position"))
		/*delete the first characters '*[position()=' */
		buffer_shift(property, 13);
	else
		/*delete the first characters '*[' */
		buffer_shift(property, 2);

	/*delete the last character ']' */
	buffer_pop(property, 1);

	/* retrieve the matching column name from the number of the column */
	prop_name = ows_psql_column_name(o, typename, atoi(property->buf));
	buffer_empty(property);
	buffer_copy(property, prop_name);

	buffer_free(prop_name);

	return property;
}


/*
 * Check if propertyName is valid and return the appropriate string
 */
buffer *fe_property_name(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	xmlChar *content;
	array *prop_table;
	buffer *tmp;

	assert(o != NULL);
	assert(typename != NULL);
	assert(n != NULL);
	assert(fe != NULL);
	assert(sql != NULL);

	tmp = buffer_init();

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	prop_table = ows_psql_describe_table(o, typename);

	assert(prop_table != NULL);

	content = xmlNodeGetContent(n);
	buffer_add_str(tmp, (char *) content);

	/* check if propertyname is an Xpath expression */
	if (check_regexp(tmp->buf, "\\*\\["))
		tmp = fe_xpath_property_name(o, typename, tmp);

	/* remove namespaces */
	tmp = wfs_request_remove_namespaces(o, tmp);

	/* check if propertyname is available */
	if (array_is_key(prop_table, tmp->buf))
	{
		buffer_add_str(sql, "\"");
		buffer_copy(sql, tmp);
		buffer_add_str(sql, "\"");
	}
	else
	{
		fe->error_code = FE_ERROR_PROPERTYNAME;
		buffer_free(tmp);
		array_free(prop_table);
		xmlFree(content);
		return sql;
	}

	array_free(prop_table);
	buffer_free(tmp);
	xmlFree(content);

	return sql;
}


/*
 * Check if featureId or GmlObjectId are valid and return the appropriate string
 */
buffer *fe_feature_id(ows * o, buffer * typename, filter_encoding * fe,
   xmlNodePtr n)
{
	xmlChar *fid;
	buffer *buf_fid, *id_name;
	list *fe_list;
	bool feature_id, gid;

	assert(o != NULL);
	assert(typename != NULL);
	assert(n != NULL);
	assert(fe != NULL);

	fid = NULL;
	id_name = NULL;
	feature_id = false;
	gid = false;

	for (; n != NULL; n = n->next)
	{
		if (n->type == XML_ELEMENT_NODE)
		{
			buf_fid = buffer_init();

			/* retrieve the property fid */
			if (strcmp((char *) n->name, "FeatureId") == 0)
			{
				feature_id = true;
				/* only one type of identifier element must be included */
				if (gid == false)
					fid = xmlGetProp(n, (xmlChar *) "fid");
				else
				{
					fe->error_code = FE_ERROR_FID;
					buffer_free(buf_fid);
					return fe->sql;
				}
			}
			/* retrieve the property gml:id */
			if (strcmp((char *) n->name, "GmlObjectId") == 0)
			{
				gid = true;
				/* only one type of identifier element must be included */
				if (feature_id == false)
					fid = xmlGetProp(n, (xmlChar *) "id");
				else
				{
					fe->error_code = FE_ERROR_FID;
					buffer_free(buf_fid);
					return fe->sql;
				}
			}

			buffer_add_str(buf_fid, (char *) fid);
			fe_list = list_explode('.', buf_fid);

			/* check if the layer_name match the typename queried */
			if (fe_list->first->next != NULL)
			{
				if (buffer_cmp(fe_list->first->value,
					  typename->buf) == false)
				{
					fe->error_code = FE_ERROR_FEATUREID;
					list_free(fe_list);
					buffer_free(buf_fid);
					xmlFree(fid);
					return fe->sql;
				}
			}

			id_name = ows_psql_id_column(o, typename);
			/* if there is no id column, raise an error */
			if (id_name->use == 0)
			{
				fe->error_code = FE_ERROR_FEATUREID;
				list_free(fe_list);
				buffer_free(buf_fid);
				buffer_free(id_name);
				xmlFree(fid);
				return fe->sql;
			}
			buffer_copy(fe->sql, id_name);
			buffer_add_str(fe->sql, " = \'");
			buffer_copy(fe->sql, fe_list->last->value);
			buffer_add_str(fe->sql, "\'");
			buffer_free(id_name);
			list_free(fe_list);
			buffer_free(buf_fid);
			xmlFree(fid);
		}
		if (n->next != NULL)
			if (n->next->type == XML_ELEMENT_NODE)
				buffer_add_str(fe->sql, " OR ");
	}
	return fe->sql;
}


/*
 * Translate an XML filter to a filter encoding structure with a buffer 
 * containing a where condition of a SQL request usable into PostGis 
 * and an error code if an error occured
 */
filter_encoding *fe_filter(ows * o, filter_encoding * fe,
   buffer * typename, buffer * xmlchar)
{
	xmlDocPtr xmldoc;
	xmlNodePtr n;

	assert(o != NULL);
	assert(fe != NULL);
	assert(typename != NULL);
	assert(xmlchar != NULL);

	xmlInitParser();
	LIBXML_TEST_VERSION fe->sql = buffer_init();

	xmldoc = xmlParseMemory(xmlchar->buf, xmlchar->use);
	if (!xmldoc)
	{
		fe->error_code = FE_ERROR_FILTER;
		xmlFreeDoc(xmldoc);
		xmlCleanupParser();
		return fe;
	}

	n = xmldoc->children->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	/* Comparison Operators */
	if (fe_is_comparison_op((char *) n->name))
		fe->sql = fe_comparison_op(o, typename, fe, n);

	/* Spatial Operators */
	if (fe_is_spatial_op((char *) n->name))
		fe->sql = fe_spatial_op(o, typename, fe, n);

	/* Logical Operators */
	if (fe_is_logical_op((char *) n->name))
		fe->sql = fe_logical_op(o, typename, fe, n);
	/* FeatureId */
	if (strcmp((char *) n->name, "FeatureId") == 0)
		fe->sql = fe_feature_id(o, typename, fe, n);
	/* GmlObjectId */
	else if (ows_version_get(o->request->version) == 110
	   && strcmp((char *) n->name, "GmlObjectId") == 0)
		fe->sql = fe_feature_id(o, typename, fe, n);

	xmlFreeDoc(xmldoc);
	xmlCleanupParser();

	return fe;
}


/*
 * Transform bbox parameter into WHERE statement of a FE->SQL request
 */
buffer *fe_kvp_bbox(ows * o, wfs_request * wr, buffer * layer_name,
   ows_bbox * bbox)
{
	buffer *where;
	list *geom;
	list_node *ln;

	assert(o != NULL);
	assert(wr != NULL);
	assert(layer_name != NULL);
	assert(bbox != NULL);

	where = buffer_init();
	geom = ows_psql_geometry_column(o, layer_name);

	buffer_add_str(where, " WHERE");

	for (ln = geom->first; ln != NULL; ln = ln->next)
	{
		buffer_add_str(where, " not(disjoint(\"");
		buffer_copy(where, ln->value);
		buffer_add_str(where, "\",SetSRID('BOX(");
		buffer_add_double(where, wr->bbox->xmin);
		buffer_add_str(where, " ");
		buffer_add_double(where, wr->bbox->ymin);
		buffer_add_str(where, ",");
		buffer_add_double(where, wr->bbox->xmax);
		buffer_add_str(where, " ");
		buffer_add_double(where, wr->bbox->ymax);
		buffer_add_str(where, ")'::box2d,");
		buffer_add_int(where, wr->bbox->srs->srid);
		buffer_add_str(where, ")))");
		if (ln->next != NULL)
			buffer_add_str(where, " AND ");
	}
	list_free(geom);

	return where;
}


/*
 * Transform featureid parameter into WHERE statement of a FE->SQL request
 */
buffer *fe_kvp_featureid(ows * o, wfs_request * wr, buffer * layer_name,
   list * fid)
{
	buffer *id_name, *where;
	list *fe;
	list_node *ln;

	assert(o != NULL);
	assert(wr != NULL);
	assert(layer_name != NULL);
	assert(fid != NULL);

	where = buffer_init();

	id_name = ows_psql_id_column(o, layer_name);
	if (id_name->use == 0)
	{
		buffer_free(id_name);
		return where;
	}

	buffer_add_str(where, " WHERE ");

	for (ln = fid->first; ln != NULL; ln = ln->next)
	{
		fe = list_explode('.', ln->value);
		buffer_copy(where, id_name);
		buffer_add_str(where, " = '");
		buffer_copy(where, fe->last->value);
		buffer_add_str(where, "'");
		list_free(fe);
		if (ln->next != NULL)
			buffer_add_str(where, " OR ");
	}
	buffer_free(id_name);

	return where;
}


/*
 * vim: expandtab sw=4 ts=4
 */

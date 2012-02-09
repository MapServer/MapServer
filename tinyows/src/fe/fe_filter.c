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
 * Initialize filter_encoding structure
 */
filter_encoding *filter_encoding_init()
{
    filter_encoding *fe;

    fe = malloc(sizeof(filter_encoding));
    assert(fe);

    fe->sql = buffer_init();
    fe->error_code = FE_NO_ERROR;
    fe->in_not = 0;

    return fe;
}


/*
 * Release filter_encoding structure
 */
void filter_encoding_free(filter_encoding * fe)
{
    assert(fe);

    buffer_free(fe->sql);
    free(fe);
    fe = NULL;
}


#ifdef OWS_DEBUG
/*
 * Print filter_encoding structure
 */
void filter_encoding_flush(filter_encoding * fe, FILE * output)
{
    assert(fe);
    assert(output);

    fprintf(output, "[\n");

    if (fe->sql) {
        fprintf(output, "sql -> ");
        buffer_flush(fe->sql, output);
        fprintf(output, "\n");
    }

    fprintf(output, " error code -> %d\n]\n", fe->error_code);
}


/*
 * Parse an XML node, used for debug purposes
 */
void fe_node_flush(xmlNodePtr node, FILE * output)
{
    xmlNodePtr n;
    xmlAttr *att;
    xmlChar *content = NULL;

    assert(node);
    assert(output);

    if (node->type == XML_ELEMENT_NODE) {
        for (n = node; n; n = n->next) {
            fprintf(output, "name:%s\n", n->name);

            for (att = n->properties; att ; att = att->next)
                fprintf(output, "%s=%s\n", att->name, (char *) xmlNodeGetContent(att->children));

            if (n->children) fe_node_flush(n->children, output);
        }
    } else if ((node->type == XML_CDATA_SECTION_NODE) || (node->type == XML_TEXT_NODE)) {
        content = xmlNodeGetContent(node);
        fprintf(output, "%s\n", content);
        xmlFree(content);
    }
}
#endif


/*
 * Recursive function which eval a prefixed expression and
 * return the matching string
 */
buffer * fe_expression(ows * o, buffer * typename, filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
    char *escaped;
    xmlChar *content;
    bool isstring = false;

    assert(o);
    assert(typename);
    assert(fe);
    assert(sql);

    if (!n) return sql;
    if (!strcmp((char *) n->name, "Function")) return fe_function(o, typename, fe, sql, n);

    /* Open a bracket when there is a 'grandchildren' elements */
    if (n->children) {
             if (n->children->type == XML_ELEMENT_NODE && n->children->children) buffer_add_str(sql, "(");
        else if (n->children->next && (n->children->next->children)) buffer_add_str(sql, "(");
    }

    /* Eval the left part of the expression */
    sql = fe_expression(o, typename, fe, sql, n->children);

    content = xmlNodeGetContent(n);

         if (!strcmp((char *) n->name, "Add")) buffer_add_str(sql, " + ");
    else if (!strcmp((char *) n->name, "Sub")) buffer_add_str(sql, " - ");
    else if (!strcmp((char *) n->name, "Mul")) buffer_add_str(sql, " * ");
    else if (!strcmp((char *) n->name, "Div")) buffer_add_str(sql, " / ");
    else if (!strcmp((char *) n->name, "Literal")) {

        /* Strings must be written in quotation marks */
        if (check_regexp((char *) content, "^[[:space:]]+$") == 1) {
            buffer_add_str(sql, "''");      /* empty string */

        } else {
          if (!check_regexp((char *)content, "^[-+]?[0-9]*\\.?[0-9]+$")){
              isstring = true;
              buffer_add_str(sql, "'");
          }

          escaped = ows_psql_escape_string(o, (char *) content);
          if (escaped) {
              buffer_add_str(sql, escaped);
              free(escaped);
          }
          if (isstring) buffer_add_str(sql, "'");
        }

    } else if (!strcmp((char *) n->name, "PropertyName")) {
        buffer_add(sql, '"');
        sql = fe_property_name(o, typename, fe, sql, n, false, true);
        buffer_add(sql, '"');
    } else if (n->type != XML_ELEMENT_NODE) {
        sql = fe_expression(o, typename, fe, sql, n->next);
    }

    xmlFree(content);

    /* Eval the right part of the expression */
    if (n->children && n->children->next) {
        if (n->children->type == XML_ELEMENT_NODE && n->children->next->type == XML_ELEMENT_NODE)
             sql = fe_expression(o, typename, fe, sql, n->children->next);
        else sql = fe_expression(o, typename, fe, sql, n->children->next->next);

        content = xmlNodeGetContent(n->children->next);

        /* If children element is empty */
        if (check_regexp((char *) content, "^$") == 1) buffer_add_str(sql, "''");
        /* Close a bracket when there are not empty children elements */
        else if (check_regexp((char *) content, " +") != 1) buffer_add_str(sql, ")");

        xmlFree(content);
    }

    return sql;
}


/*
 * Transform an Xpath expression into a string propertyname
 */
buffer *fe_xpath_property_name(ows * o, buffer * typename, buffer * property)
{
    buffer *prop_name;

    assert(o);
    assert(typename);
    assert(property);

    if (check_regexp(property->buf, "\\*\\[position")) buffer_shift(property, 13); /* Remove '*[position()=' */
    else buffer_shift(property, 2); /* Remove '*[' */
    buffer_pop(property, 1);        /* Remove ']' */

    /* Retrieve the matching column name from the number of the column */
    prop_name = ows_psql_column_name(o, typename, atoi(property->buf));
    buffer_empty(property);
    buffer_copy(property, prop_name);

    buffer_free(prop_name);

    return property;
}


/*
 * Check if propertyName is valid and return the appropriate string
 */
buffer *fe_property_name(ows * o, buffer * typename, filter_encoding * fe, buffer * sql, xmlNodePtr n, 
                         bool check_geom_column, bool mandatory)
{
    xmlChar *content;
    array *prop_table;
    buffer *tmp;
    list *l;

    assert(o);
    assert(typename);
    assert(n);
    assert(fe);
    assert(sql);

    while (n->type != XML_ELEMENT_NODE) n = n->next;       /* Jump to the next element if there are spaces */

    prop_table = ows_psql_describe_table(o, typename);
    assert(prop_table); /* FIXME to remove */

    content = xmlNodeGetContent(n);
    tmp = buffer_from_str((char *) content);

    /* Check if propertyname is an Xpath expression */
    if (check_regexp(tmp->buf, "\\*\\[")) tmp = fe_xpath_property_name(o, typename, tmp);
   
    /* If propertyname have an Xpath suffix */
    /* FIXME i just can't understand meaning of this use case */
    if (check_regexp(tmp->buf, ".*\\[[0-9]+\\]")) 
    {
	l = list_explode('[', tmp);
	buffer_empty(tmp);
	buffer_copy(tmp, l->first->value);
	list_free(l);
    }

    /* Remove namespaces */
    tmp = wfs_request_remove_namespaces(o, tmp);

    /* Check if propertyname is available */
    if (array_is_key(prop_table, tmp->buf)) {
        buffer_copy(sql, tmp);
    } else if (mandatory) fe->error_code = FE_ERROR_PROPERTYNAME;

    if (mandatory && check_geom_column && !ows_psql_is_geometry_column(o, typename, tmp))
        fe->error_code = FE_ERROR_GEOM_PROPERTYNAME;

    buffer_free(tmp);
    xmlFree(content);

    return sql;
}


/*
 * Check if featureId or GmlObjectId are valid and return the appropriate string
 */
buffer *fe_feature_id(ows * o, buffer * typename, filter_encoding * fe, xmlNodePtr n)
{
    char *escaped;
    list *fe_list;
    bool feature_id, gid;
    xmlChar *fid = NULL;
    buffer *buf_fid, *id_name = NULL;

    assert(o);
    assert(typename);
    assert(n);
    assert(fe);

    for (feature_id = gid = false ; n ; n = n->next) {
        if (n->type == XML_ELEMENT_NODE) {
            buf_fid = buffer_init();

            /* retrieve the property fid */
            if (!strcmp((char *) n->name, "FeatureId")) {
                feature_id = true;

                /* only one type of identifier element must be included */
                if (!gid) fid = xmlGetProp(n, (xmlChar *) "fid");
                else {
                    fe->error_code = FE_ERROR_FID;
                    buffer_free(buf_fid);
                    return fe->sql;
                }
            }

            /* retrieve the property gml:id */
            if (!strcmp((char *) n->name, "GmlObjectId")) {
                gid = true;

                /* only one type of identifier element must be included */
                if (feature_id == false) fid = xmlGetProp(n, (xmlChar *) "id");
                else {
                    fe->error_code = FE_ERROR_FID;
                    buffer_free(buf_fid);
                    return fe->sql;
                }
            }
            buffer_add_str(buf_fid, (char *) fid);
            fe_list = list_explode('.', buf_fid);
            xmlFree(fid);

            /* Check if the layer_name match the typename queried */
            if (fe_list->first && !buffer_cmp(fe_list->first->value, typename->buf)) {
                buffer_add_str(fe->sql, " FALSE"); /* We still execute the query */
                list_free(fe_list);
                buffer_free(buf_fid);
                continue;
            }

            /* If there is no id column, raise an error */
            id_name = ows_psql_id_column(o, typename);
            if (!id_name || !id_name->use) {
                fe->error_code = FE_ERROR_FEATUREID;
                list_free(fe_list);
                buffer_free(buf_fid);
                return fe->sql;
            }

            buffer_copy(fe->sql, id_name);
            buffer_add_str(fe->sql, " = \'");
	    if (fe_list->last) escaped = ows_psql_escape_string(o, fe_list->last->value->buf);
            else               escaped = ows_psql_escape_string(o, buf_fid->buf);
            
            if (escaped) {
                buffer_add_str(fe->sql, escaped);
                free(escaped);
            }

            buffer_add_str(fe->sql, "\'");
            list_free(fe_list);
            buffer_free(buf_fid);
        }

        if (n->next && n->next->type == XML_ELEMENT_NODE) buffer_add_str(fe->sql, " OR ");
    }

    return fe->sql;
}


/*
 * Translate an XML filter to a filter encoding structure with a buffer
 * containing a where condition of a SQL request usable into PostGis
 * and an error code if an error occured
 */
filter_encoding *fe_filter(ows * o, filter_encoding * fe, buffer * typename, buffer * xmlchar)
{
    buffer * schema_path;
    xmlDocPtr xmldoc;
    xmlNodePtr n;
    int ret = -1;

    assert(o);
    assert(fe);
    assert(typename);
    assert(xmlchar);

    /* No validation if Filter came from KVP method 
       FIXME: really, but why ?                     */
    if (o->check_schema && o->request->method == OWS_METHOD_XML) {
        schema_path = buffer_init();
        buffer_copy(schema_path, o->schema_dir);

        if (ows_version_get(o->request->version) == 100) {
            buffer_add_str(schema_path, WFS_SCHEMA_100);
	    ret = ows_schema_validation(o, schema_path, xmlchar, true, WFS_SCHEMA_TYPE_100);
	} else { 
	    buffer_add_str(schema_path, WFS_SCHEMA_110);
	    ret = ows_schema_validation(o, schema_path, xmlchar, true, WFS_SCHEMA_TYPE_110);
	}

        if (ret != 0) {
            buffer_free(schema_path);
            fe->error_code = FE_ERROR_FILTER;
            return fe;
        }

        buffer_free(schema_path);
    }

    xmldoc = xmlParseMemory(xmlchar->buf, xmlchar->use);

    if (!xmldoc) {
        fe->error_code = FE_ERROR_FILTER;
        xmlFreeDoc(xmldoc);
        return fe;
    }

    if (!ows_libxml_check_namespace(o, xmldoc->children)) {
        fe->error_code = FE_ERROR_NAMESPACE;
        xmlFreeDoc(xmldoc);
        return fe;
    }

    n = xmldoc->children->children;

    /* jump to the next element if there are spaces */
    while (n->type != XML_ELEMENT_NODE) n = n->next;

         if (fe_is_comparison_op((char *) n->name))  fe->sql = fe_comparison_op(o, typename, fe, n);
         if (fe_is_spatial_op((char *) n->name))     fe->sql = fe_spatial_op(o, typename, fe, n);
         if (fe_is_logical_op((char *) n->name))     fe->sql = fe_logical_op(o, typename, fe, n);
         if (!strcmp((char *) n->name, "FeatureId")) fe->sql = fe_feature_id(o, typename, fe, n);
    else if (!strcmp((char *) n->name, "GmlObjectId") && ows_version_get(o->request->version) == 110)
        fe->sql = fe_feature_id(o, typename, fe, n); /* FIXME Is FeatureId should really have priority ? */

    xmlFreeDoc(xmldoc);

    return fe;
}


/*
 * Transform bbox parameter into WHERE statement of a FE->SQL request
 */
buffer *fe_kvp_bbox(ows * o, wfs_request * wr, buffer * layer_name, ows_bbox * bbox)
{
    buffer *where;
    list *geom;
    list_node *ln;
    int srid = -1;
    bool transform = false;

    assert(o);
    assert(wr);
    assert(layer_name);
    assert(bbox);

    where = buffer_init();
    geom = ows_psql_geometry_column(o, layer_name);
    buffer_add_str(where, " WHERE");

    if (wr->srs) { 
       srid = ows_srs_get_srid_from_layer(o, layer_name);
       transform = true; 
    }
    /* BBOX optional crsuri parameter since WFS 1.1 */
    if (wr->bbox->srs && (wr->bbox->srs->srid != wr->srs->srid)) {
       srid = wr->srs->srid;
       transform = true; 
    }

    for (ln = geom->first ; ln ; ln = ln->next) {

	/* We use _ST_Intersects and && operator rather than ST_Intersects for performances issues */
        buffer_add_str(where, " (_ST_Intersects(");
        if (transform) buffer_add_str(where, "ST_Transform(");
        buffer_add_str(where, "\"");
        buffer_copy(where, ln->value);
        buffer_add_str(where, "\",");
        if (transform) { 
            buffer_add_int(where, srid);
            buffer_add_str(where, "),");
        }

        if (transform) buffer_add_str(where, "ST_Transform(");
        ows_bbox_to_query(o, wr->bbox, where);
        if (transform) {
            buffer_add_str(where, ",");
            buffer_add_int(where, srid);
            buffer_add_str(where, ")");
        }
        buffer_add_str(where, ") AND ");
        if (transform) buffer_add_str(where, "ST_Transform(");
        buffer_add_str(where, "\"");
        buffer_copy(where, ln->value);
        buffer_add_str(where, "\"");
        if (transform) { 
            buffer_add_str(where, ",");
            buffer_add_int(where, srid);
            buffer_add_str(where, ")");
        }
        buffer_add_str(where, " && ");
        if (transform) buffer_add_str(where, "ST_Transform(");
        ows_bbox_to_query(o, wr->bbox, where);
        if (transform) {
            buffer_add_str(where, ",");
            buffer_add_int(where, srid);
            buffer_add_str(where, ")");
        }

        if (ln->next) buffer_add_str(where, ") OR ");
        else          buffer_add_str(where, ")");
    }

    return where;
}


/*
 * Transform featureid parameter into WHERE statement of a FE->SQL request
 */
buffer *fe_kvp_featureid(ows * o, wfs_request * wr, buffer * layer_name, list * fid)
{
    buffer *id_name, *where;
    char *escaped;
    list *fe;
    list_node *ln;

    assert(o);
    assert(wr);
    assert(layer_name);
    assert(fid);

    where = buffer_init();

    id_name = ows_psql_id_column(o, layer_name);
    if (!id_name || id_name->use == 0) return where;

    buffer_add_str(where, " WHERE ");

    for (ln = fid->first ; ln ; ln = ln->next) {
        fe = list_explode('.', ln->value);
        buffer_copy(where, id_name);
        buffer_add_str(where, " = '");
        escaped = ows_psql_escape_string(o, fe->last->value->buf);
        if (escaped) {
            buffer_add_str(where, escaped);
            free(escaped);
        }
        buffer_add_str(where, "'");
        list_free(fe);

        if (ln->next) buffer_add_str(where, " OR ");
    }

    return where;
}


/*
 * vim: expandtab sw=4 ts=4
 */

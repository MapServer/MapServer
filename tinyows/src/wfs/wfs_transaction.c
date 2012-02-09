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
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "../ows/ows.h"

/*
 * Execute the request sql matching a transaction
 * Return the result of the request (PGRES_COMMAND_OK or an error message)
 */
static buffer *wfs_execute_transaction_request(ows * o, wfs_request * wr, buffer * sql)
{
    buffer *result, *cmd_status;
    PGresult *res;

    assert(o);
    assert(sql);

    result = buffer_init();
    cmd_status = buffer_init();

    res = ows_psql_exec(o, sql->buf);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
        buffer_add_str(result, PQresultErrorMessage(res));
    else
        buffer_add_str(result, PQresStatus(PQresultStatus(res)));
    buffer_add_str(cmd_status, (char *) PQcmdStatus(res));

    if (check_regexp(cmd_status->buf, "^DELETE")) {
        cmd_status = buffer_replace(cmd_status, "DELETE ", "");
        wr->delete_results += atoi(cmd_status->buf);
    }

    if (check_regexp(cmd_status->buf, "^UPDATE")) {
        cmd_status = buffer_replace(cmd_status, "UPDATE ", "");
        wr->update_results += atoi(cmd_status->buf);
    }

    buffer_free(cmd_status);
    PQclear(res);

    return result;
}


/*
 * Summarize overall results of transaction request
 */
static void wfs_transaction_summary(ows * o, wfs_request * wr, buffer * result)
{
    alist_node *an;
    int nb = 0; 

    assert(o);
    assert(wr);
    assert(result);

    fprintf(o->output, "<wfs:TransactionSummary>\n");

    if (buffer_cmp(result, "PGRES_COMMAND_OK")) {

        if (wr->insert_results) {
            for (an = wr->insert_results->first ; an ; an = an->next) nb += an->value->size;
            fprintf(o->output, " <wfs:totalInserted>%d</wfs:totalInserted>\n", nb);
        }

        fprintf(o->output, "<wfs:totalUpdated>%d</wfs:totalUpdated>\n", wr->update_results);
        fprintf(o->output, " <wfs:totalDeleted>%d</wfs:totalDeleted>\n", wr->delete_results);
    }

    fprintf(o->output, "</wfs:TransactionSummary>\n");
}


/*
 * Report newly created feature instances
 */
static void wfs_transaction_insert_result(ows * o, wfs_request * wr, buffer * result)
{
    alist_node *an;
    list_node *ln;

    assert(o);
    assert(wr);
    assert(result);

    ln = NULL;

    /* check if there were Insert operations and if the command succeeded */
    if ((!cgi_method_get()) && (buffer_cmp(result, "PGRES_COMMAND_OK") && (wr->insert_results->first))) {

        if (ows_version_get(o->request->version) == 110)
            fprintf(o->output, "<wfs:InsertResults>\n");

        for (an = wr->insert_results->first ; an ; an = an->next) {

            if (ows_version_get(o->request->version) == 100) {
                fprintf(o->output, "<wfs:InsertResult handle=\"%s\">", an->key->buf);
                for (ln = an->value->first ; ln ; ln = ln->next)
                    fprintf(o->output, "<ogc:FeatureId fid=\"%s\"/>", ln->value->buf);
                fprintf(o->output, "</wfs:InsertResult>\n");
            } else {
                for (ln = an->value->first ; ln ; ln = ln->next) {
                    fprintf(o->output, "<wfs:Feature handle=\"%s\">\n", an->key->buf);
                    fprintf(o->output, " <ogc:FeatureId fid=\"%s\"/>\n", ln->value->buf);
                    fprintf(o->output, "</wfs:Feature>\n");
                }
            }
        }

        if (ows_version_get(o->request->version) == 110)
            fprintf(o->output, "</wfs:InsertResults>");
    }
}


/*
 * Report the overall result of the transaction request
 */
static void wfs_transaction_result(ows * o, wfs_request * wr, buffer * result, buffer * locator)
{
    assert(o);
    assert(wr);
    assert(result);

    if (!buffer_cmp(result, "PGRES_COMMAND_OK")) assert(locator);

    /* only if version = 1.0.0 or if command failed */
    if (ows_version_get(o->request->version) == 100
            || (ows_version_get(o->request->version) == 110
                && !buffer_cmp(result, "PGRES_COMMAND_OK"))) {

        if (ows_version_get(o->request->version) == 110)
            fprintf(o->output, "<wfs:TransactionResults>\n");
        else {
            fprintf(o->output, "<wfs:TransactionResult>\n");
            /* display status transaction only for 1.0.0 version */
            fprintf(o->output, "<wfs:Status>");

            if (buffer_cmp(result, "PGRES_COMMAND_OK"))
                 fprintf(o->output, "<wfs:SUCCESS/>");
            else fprintf(o->output, "<wfs:FAILED/>");

            fprintf(o->output, "</wfs:Status>\n");
        }


        if (ows_version_get(o->request->version) == 100)
            fprintf(o->output, "</wfs:TransactionResult>\n");
        else {
            fprintf(o->output, "</wfs:Action>\n");
            fprintf(o->output, "</wfs:TransactionResults>\n");
        }
    }
}


/*
 * Write the transaction response in XML
 */
static void wfs_transaction_response(ows * o, wfs_request * wr, buffer * result, buffer * locator)
{
    assert(o);
    assert(wr);
    assert(result);

    if (!buffer_cmp(result, "PGRES_COMMAND_OK")) {
           if (buffer_cmp(result, "No FE selector on DELETE statement"))
               wfs_error(o, wr, WFS_ERROR_MISSING_PARAMETER, result->buf, locator->buf);
	   else
               wfs_error(o, wr, WFS_ERROR_INVALID_PARAMETER, result->buf, locator->buf);
	   return;
    }

    fprintf(o->output, "Content-Type: application/xml\n\n");
    fprintf(o->output, "<?xml version='1.0' encoding='%s'?>\n", o->encoding->buf);

    if (ows_version_get(o->request->version) == 100)
         fprintf(o->output, "<wfs:WFS_TransactionResponse version=\"1.0.0\"\n");
    else fprintf(o->output, "<wfs:TransactionResponse version=\"1.1.0\"\n");

    fprintf(o->output, " xmlns:wfs=\"http://www.opengis.net/wfs\"\n");
    fprintf(o->output, " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    fprintf(o->output, " xmlns:ogc=\"http://www.opengis.net/ogc\"\n");

    if (ows_version_get(o->request->version) == 100) {
        fprintf(o->output, " xsi:schemaLocation='http://www.opengis.net/wfs");
        fprintf(o->output, " http://schemas.opengis.net/wfs/1.0.0/WFS-transaction.xsd'>\n");
    } else {
        fprintf(o->output, " xsi:schemaLocation='http://www.opengis.net/wfs");
        fprintf(o->output, " http://schemas.opengis.net/wfs/1.1.0/wfs.xsd'>\n");
    }

    if (ows_version_get(o->request->version) == 100) {
        wfs_transaction_insert_result(o, wr, result);
        wfs_transaction_result(o, wr, result, locator);
    } else {
        wfs_transaction_summary(o, wr, result);
        wfs_transaction_result(o, wr, result, locator);
        wfs_transaction_insert_result(o, wr, result);
    }

    if (ows_version_get(o->request->version) == 100)
         fprintf(o->output, "</wfs:WFS_TransactionResponse>\n");
    else fprintf(o->output, "</wfs:TransactionResponse>\n");
}


/*
 * Add a content node value to a buffer
 */
static buffer *wfs_retrieve_value(ows * o, wfs_request * wr, buffer * value, xmlDocPtr xmldoc, xmlNodePtr n)
{
    xmlChar *content;
    char *content_escaped;

    assert(o);
    assert(wr);
    assert(value);
    assert(n);

    content = xmlNodeGetContent(n);
    content_escaped = ows_psql_escape_string(o, (char *) content);

    if (!content_escaped)
    {
        xmlFree(content);
        buffer_free(value);
        xmlFreeDoc(xmldoc);
        ows_error(o, OWS_ERROR_FORBIDDEN_CHARACTER,
	    "Some forbidden character are present into the request", "transaction");
    }

    buffer_add_str(value, "'");
    buffer_add_str(value, content_escaped);
    buffer_add_str(value, "'");

    xmlFree(content);
    free(content_escaped);

    return value;
}


/*
 * Retrieve the layer's name
 */
static buffer *wfs_retrieve_typename(ows * o, wfs_request * wr, xmlNodePtr n)
{
    xmlAttr *att;
    xmlChar *content;
    buffer *typename;

    typename = buffer_init();
    content = NULL;

    for (att = n->properties ; att ; att = att->next) {

        if (!strcmp((char *) att->name, "typeName")) {
            content = xmlNodeGetContent(att->children);
            buffer_add_str(typename, (char *) content);
            wfs_request_remove_namespaces(o, typename);  /* FIXME need to be rewrite */

            if (!ows_layer_writable(o->layers, typename)) {
            	xmlFree(content);
    		return NULL;
	    }

            xmlFree(content);
    	    return typename;
        }
    }

    return NULL;
}


/*
 * Insert features into the database
 * Method POST, XML
 */
static buffer *wfs_insert_xml(ows * o, wfs_request * wr, xmlDocPtr xmldoc, xmlNodePtr n)
{
    buffer *values, *column, *layer_name, *layer_ns_prefix, *result, *sql, *gml;
    buffer *handle, *id_column, *fid_full_name, *dup_sql, *id;
    xmlNodePtr node, elemt;
    filter_encoding *fe;
    PGresult *res;
    array * table;
    char *escaped;
    list *l;
    ows_srs * srs_root;
    int srid_root = 0;
    xmlChar *attr = NULL;
    enum wfs_insert_idgen idgen = WFS_GENERATE_NEW;
    enum wfs_insert_idgen handle_idgen = WFS_GENERATE_NEW;

    assert(o);
    assert(wr);
    assert(xmldoc);
    assert(n);

    sql = buffer_init();
    handle = buffer_init();
    result = NULL;

    /* retrieve handle attribute to report it in transaction response */
    if (xmlHasProp(n, (xmlChar *) "handle")) {
        attr =  xmlGetProp(n, (xmlChar *) "handle");
        buffer_add_str(handle, (char *) attr);
        xmlFree(attr);
        attr = NULL;
    /* handle is optional in WFS Schema */
    } else buffer_add_str(handle, "TinyOWS-WFS-default-handle");

    /* idgen appears in WFS 1.1.0, default behaviour is GenerateNew id
       It is safe to not test WFS version, as schema validation 
       already do the job for us.
    */
    if (xmlHasProp(n, (xmlChar *) "idgen")) {
        attr =  xmlGetProp(n, (xmlChar *) "idgen");
             if (!strcmp((char *) attr, "ReplaceDuplicate")) handle_idgen = WFS_REPLACE_DUPLICATE;
        else if (!strcmp((char *) attr, "UseExisting"))      handle_idgen = WFS_USE_EXISTING;
        xmlFree(attr);
        attr = NULL;
    }

    /*
     * In WFS 1.1.0 default srsName could be define  
     * TODO: what about for WFS 1.0.0 ?
     */
    if (xmlHasProp(n, (xmlChar *) "srsName")) {
        attr =  xmlGetProp(n, (xmlChar *) "srsName");
        srs_root = ows_srs_init();

        if (!ows_srs_set_from_srsname(o, srs_root, (char *) attr)) {
             buffer_free(sql);
             buffer_free(handle);
       	     ows_srs_free(srs_root);
       	     xmlFree(attr);
             result = buffer_from_str("Unkwnown or wrong CRS used");
             return result;
	}
	
        srid_root = srs_root->srid;
        ows_srs_free(srs_root);
        xmlFree(attr);
        attr = NULL;
    }

    n = n->children;

    /* jump to the next element if there are spaces */
    while (n->type != XML_ELEMENT_NODE) n = n->next;

    /* Create Insert SQL query for each Typename */
    for ( /* Empty */ ; n ; n = n->next) {
        if (n->type != XML_ELEMENT_NODE) continue;

        id = buffer_init();
        values = buffer_init();
        layer_name = buffer_init();

        /* name of the table in which features must be inserted */
	buffer_add_str(layer_name, (char *) n->name);
        wfs_request_remove_namespaces(o, layer_name);

        if (!layer_name || !ows_layer_writable(o->layers, layer_name)) {
             buffer_free(id);
             buffer_free(handle);
             buffer_free(sql);
             buffer_free(values);
	     if (layer_name) buffer_free(layer_name);
             result = buffer_from_str("Error unknown or not writable Layer Name");
             return result;
	}

        idgen = handle_idgen;

        /* In GML 3 GML:id is used, in GML 2.1.2 fid is used.
         * In both cases no other attribute allowed in this element
         * and in both cases (f)id is optionnal !
         */ 
             if (xmlHasProp(n, (xmlChar *) "id"))  attr = xmlGetProp(n, (xmlChar *) "id");
        else if (xmlHasProp(n, (xmlChar *) "fid")) attr = xmlGetProp(n, (xmlChar *) "fid");
        
        if (attr) {
            buffer_empty(id);
            buffer_add_str(id, (char *) attr);
            xmlFree(attr);
        } else idgen = WFS_GENERATE_NEW;
        /* FIXME should we end on error if UseExisting or Replace without id set ? */

        id_column = ows_psql_id_column(o, layer_name);
	if (!id_column) {
             buffer_free(id);
             buffer_free(sql);
             buffer_free(handle);
             buffer_free(values);
             buffer_free(layer_name);
             result = buffer_from_str("Error unknown Layer Name or not id column available");
             return result;
	} 

        if (id->use) {
            l = list_explode('.', id);
	    if (l->last) {
	       buffer_empty(id);
               buffer_copy(id, l->last->value);
            }
            list_free(l);
        }

        layer_ns_prefix = ows_layer_ns_prefix(o->layers, layer_name);

        /* ReplaceDuplicate look if an ID is already used
         *
         * May not be safe if another transaction occur between
         * this select and the related insert !!!
         */
	   if (idgen == WFS_REPLACE_DUPLICATE) {
           dup_sql = buffer_init();

           buffer_add_str(dup_sql, "SELECT count(*) FROM "); 
           buffer_copy(dup_sql, ows_psql_schema_name(o, layer_name));
           buffer_add_str(dup_sql, ".\"");
           buffer_copy(dup_sql, ows_psql_table_name(o, layer_name));
           buffer_add_str(dup_sql, "\" WHERE "); 
           buffer_copy(dup_sql, id_column);
           buffer_add_str(dup_sql, "='"); 
           escaped = ows_psql_escape_string(o, id->buf);
           if (escaped) {
                buffer_add_str(dup_sql, escaped);
                free(escaped);
           }
           buffer_add_str(dup_sql, "';");

           res = ows_psql_exec(o, dup_sql->buf);
           if (PQresultStatus(res) != PGRES_TUPLES_OK || atoi((char *) PQgetvalue(res, 0, 0)) != 0)
               idgen = WFS_GENERATE_NEW;
           else
               idgen = WFS_USE_EXISTING;

           buffer_free(dup_sql);
           PQclear(res);
       }

        if (idgen == WFS_GENERATE_NEW) {
            buffer_free(id);
            id = ows_psql_generate_id(o, layer_name);
        }

        /* Retrieve the id of the inserted feature
         * to report it in transaction respons
         */
        fid_full_name = buffer_init();
        buffer_copy(fid_full_name, layer_name);
        buffer_add(fid_full_name, '.');
        buffer_copy(fid_full_name, id);
        alist_add(wr->insert_results, handle, fid_full_name);

        buffer_add_str(sql, "INSERT INTO ");
        buffer_copy(sql, ows_psql_schema_name(o, layer_name));
        buffer_add_str(sql, ".\"");
        buffer_copy(sql, ows_psql_table_name(o, layer_name));
        buffer_add_str(sql, "\" (\"");
        buffer_copy(sql, id_column);
        buffer_add_str(sql, "\"");

        node = n->children;

        /* Jump to the next element if spaces */
        while (node->type != XML_ELEMENT_NODE) node = node->next;

        /* Fill SQL fields and values at once */
        for ( /* empty */ ; node; node = node->next) {
            if (node->type == XML_ELEMENT_NODE && 
                 ( buffer_cmp(ows_layer_ns_uri(o->layers, layer_ns_prefix), (char *) node->ns->href)
                   || !strcmp("http://www.opengis.net/gml",     (char *) node->ns->href)
                   || !strcmp("http://www.opengis.net/gml/3.2", (char *) node->ns->href))) {

		  /* We have to ignore if not present in database, 
                     gml elements (name, description, boundedBy) */
                  if (    !strcmp("http://www.opengis.net/gml",     (char *) node->ns->href)
                       || !strcmp("http://www.opengis.net/gml/3.2", (char *) node->ns->href)) {
		      table = ows_psql_describe_table(o, layer_name);
		      if (!array_is_key(table, (char *) node->name)) continue;
		  }
                buffer_add(sql, ',');
                buffer_add(values, ',');

                column = buffer_from_str((char *) node->name);

                buffer_add_str(sql, "\"");
                escaped = ows_psql_escape_string(o, column->buf);
                if (escaped) {
                    buffer_add_str(sql, escaped);
                    free(escaped);
                }
                buffer_add_str(sql, "\"");

                /* If column's type is a geometry, transform the GML into WKT */
                if (ows_psql_is_geometry_column(o, layer_name, column)) {
                    elemt = node->children;

                    /* Jump to the next element if spaces */
                    while (elemt->type != XML_ELEMENT_NODE) elemt = elemt->next;

                    if (!strcmp((char *) elemt->name, "Box") ||
                        !strcmp((char *) elemt->name, "Envelope")) {

                        fe = filter_encoding_init();
                        fe->sql = fe_envelope(o, layer_name, fe, fe->sql, elemt);
			if (fe->error_code != FE_NO_ERROR) {
				result = fill_fe_error(o, fe);
             			buffer_free(sql);
             		        buffer_free(handle);
            		 	buffer_free(values);
                            	buffer_free(column);
                            	buffer_free(id);
             			buffer_free(layer_name);
                        	filter_encoding_free(fe);
				return result;
			}
                        buffer_copy(values, fe->sql);
                        filter_encoding_free(fe);

                    } else if (!strcmp((char *) elemt->name, "Null")) {
                        buffer_add_str(values, "''");
                    } else {
                        gml = ows_psql_gml_to_sql(o, elemt, srid_root);
                        if (gml) {
                            buffer_add_str(values, "'");
                            buffer_copy(values, gml);
                            buffer_add_str(values, "'");
                            buffer_free(gml);
                        } else {
                            buffer_free(sql);
                            buffer_free(values);
                            buffer_free(layer_name);
                            buffer_free(column);
                            buffer_free(id);

                            result = buffer_from_str("Error invalid Geometry");
                            return result;
                        }
                   }

                } else values = wfs_retrieve_value(o, wr, values, xmldoc, node);

                buffer_free(column);
            }
        }

        /* As 'id' could be NULL in GML */
        if (id->use) {  
            buffer_add_str(sql, ") VALUES ('");
            escaped = ows_psql_escape_string(o, id->buf);
            if (escaped) {
                buffer_add_str(sql, escaped);
                free(escaped);
            }
            buffer_add_str(sql, "'");
        } else buffer_add_str(sql, ") VALUES (null");

        buffer_copy(sql, values);
        buffer_add_str(sql, ") ");

        buffer_free(values);
        buffer_free(layer_name);
        buffer_free(id);

        /* Run the request to insert each feature */
        if(result) buffer_free(result);
        result = wfs_execute_transaction_request(o, wr, sql);
        if (!buffer_cmp(result, "PGRES_COMMAND_OK")) {
             buffer_free(sql);
             return result;
        }
        buffer_empty(sql);
    }

    buffer_free(sql);

    return result;
}


/*
 * Delete features in database
 * Method GET / KVP
 */
void wfs_delete(ows * o, wfs_request * wr)
{
    buffer *sql, *result, *where, *layer_name, *locator;
    int cpt, size;
    mlist_node *mln_fid;
    list_node *ln, *ln_typename, *ln_filter;
    list *fe;
    filter_encoding *filter;

    assert(o);
    assert(wr);

    sql = buffer_init();
    ln = NULL;
    ln_typename = NULL;
    ln_filter = NULL;
    mln_fid = NULL;
    where = NULL;
    size = 0;

    if (wr->typename) {
        size = wr->typename->size;
        ln_typename = wr->typename->first;
    }

    if (wr->filter) ln_filter = wr->filter->first;
    if (wr->featureid) {
        size = wr->featureid->size;
        mln_fid = wr->featureid->first;
    }

    /* delete elements layer by layer */
    for (cpt = 0; cpt < size; cpt++) {
        /* define a layer_name which match typename or featureid */
        layer_name = buffer_init();

        if (wr->typename) buffer_copy(layer_name, ln_typename->value);
        else {
            fe = list_explode('.', mln_fid->value->first->value);
            buffer_copy(layer_name, fe->first->value);
            list_free(fe);
        }

        /* FROM */
        buffer_add_str(sql, "DELETE FROM ");
        buffer_copy(sql, ows_psql_schema_name(o, layer_name));
        buffer_add_str(sql, ".\"");
        buffer_copy(sql, ows_psql_table_name(o, layer_name));
        buffer_add_str(sql, "\" ");

        /* WHERE : match featureid, bbox or filter */

        /* FeatureId */
        if (wr->featureid) {
            where = fe_kvp_featureid(o, wr, layer_name, mln_fid->value);

            if (!where->use) {
                buffer_free(where);
                buffer_free(sql);
                buffer_free(layer_name);
                wfs_error(o, wr, WFS_ERROR_NO_MATCHING, "error : an id_column is required to use featureid", "Delete");
                return;
            }
        }
        /* BBOX */
        else if (wr->bbox) where = fe_kvp_bbox(o, wr, layer_name, wr->bbox);

        /* Filter */
        else {
            if (ln_filter->value->use) {
                where = buffer_init();
                buffer_add_str(where, " WHERE ");
                filter = filter_encoding_init();
                filter = fe_filter(o, filter, layer_name, ln_filter->value);

                if (filter->error_code != FE_NO_ERROR) {
                    buffer_free(where);
                    buffer_free(sql);
                    buffer_free(layer_name);
                    fe_error(o, filter);
                    return;
                }

                buffer_copy(where, filter->sql);
                filter_encoding_free(filter);
            }
        }

        buffer_copy(sql, where);
        buffer_add_str(sql, "; ");
        buffer_free(where);
        buffer_free(layer_name);

        /*incrementation of the nodes */
        if (wr->featureid) mln_fid = mln_fid->next;
        if (wr->typename)  ln_typename = ln_typename->next;
        if (wr->filter)    ln_filter = ln_filter->next;
    }

    result = wfs_execute_transaction_request(o, wr, sql);

    locator = buffer_init();
    buffer_add_str(locator, "Delete");

    /* display the transaction response directly since
       there is only one operation using GET method */
    wfs_transaction_response(o, wr, result, locator);

    buffer_free(locator);
    buffer_free(result);
    buffer_free(sql);
}


/*
 * Delete features in database
 * Method POST / XML
 */
static buffer *wfs_delete_xml(ows * o, wfs_request * wr, xmlNodePtr n)
{
    buffer *typename, *xmlstring, *result, *sql, *s, *t;
    filter_encoding *filter;

    assert(o);
    assert(wr);
    assert(n);

    sql = buffer_init();
    s = t = NULL;

    buffer_add_str(sql, "DELETE FROM ");

    /*retrieve the name of the table in which features must be deleted */
    typename = wfs_retrieve_typename(o, wr, n);
    if (typename) s = ows_psql_schema_name(o, typename);
    if (typename) t = ows_psql_table_name(o, typename);

    if (!typename || !s || !t) {
        if (typename) buffer_free(typename);
        buffer_free(sql);
        result = buffer_from_str("Typename provided is unknown or not writable");

	return result;
    }

    buffer_copy(sql, s);
    buffer_add_str(sql, ".\"");
    buffer_copy(sql, t);
    buffer_add_str(sql, "\"");

    n = n->children;
    if (!n) {
        buffer_free(typename);
        buffer_free(sql);
        result = buffer_init();
        buffer_add_str(result, "No FE selector on DELETE statement");
        return result;
    }

    /* jump to the next element if there are spaces */
    while (n->type != XML_ELEMENT_NODE) n = n->next;
    buffer_add_str(sql, " WHERE ");

    /* put xml filter into a buffer */
    xmlstring = buffer_init();
    xmlstring = cgi_add_xml_into_buffer(xmlstring, n);

    filter = filter_encoding_init();
    filter = fe_filter(o, filter, typename, xmlstring);

    /* check if filter returned an error */
    if (filter->error_code != FE_NO_ERROR)
        result = fill_fe_error(o, filter);
    else {
        buffer_copy(sql, filter->sql);
        buffer_add_str(sql, ";");
        /* run the SQL request to delete all specified features */
        result = wfs_execute_transaction_request(o, wr, sql);
    }

    filter_encoding_free(filter);
    buffer_free(xmlstring);
    buffer_free(typename);
    buffer_free(sql);

    return result;
}


/*
 * Update features in database
 * Method POST / XML
 */
static buffer *wfs_update_xml(ows * o, wfs_request * wr, xmlDocPtr xmldoc, xmlNodePtr n)
{
    buffer *typename, *xmlstring, *result, *sql, *property_name, *values, *gml, *s, *t;
    filter_encoding *filter, *fe;
    xmlNodePtr node, elemt;
    xmlChar *content;
    char *escaped;
    array *table;
    ows_srs *srs_root;
    int srid_root = 0;
    xmlChar *attr = NULL;

    assert(o);
    assert(wr);
    assert(xmldoc);
    assert(n);

    sql = buffer_init();
    content = NULL;
    s = t = result = NULL;

    /*
     * In WFS 1.1.0 default srsName could be define  
     * TODO: what about for WFS 1.0.0 ?
     */
    if (xmlHasProp(n, (xmlChar *) "srsName")) {
        attr =  xmlGetProp(n, (xmlChar *) "srsName");
        srs_root = ows_srs_init();

        if (!ows_srs_set_from_srsname(o, srs_root, (char *) attr)) {
       	     ows_srs_free(srs_root);
       	     xmlFree(attr);
             buffer_add_str(result, "Unkwnown or wrong CRS used");
             return result;
	}
	
        srid_root = srs_root->srid;
        ows_srs_free(srs_root);
        xmlFree(attr);
        attr = NULL;
    }

    buffer_add_str(sql, "UPDATE ");

    /*retrieve the name of the table in which features must be updated */
    typename = wfs_retrieve_typename(o, wr, n);
    if (typename) s = ows_psql_schema_name(o, typename);
    if (typename) t = ows_psql_table_name(o, typename);

    if (!typename || !s || !t) {
        if (typename) buffer_free(typename);
        buffer_free(sql);
        result = buffer_from_str("Typename provided is unknown or not writable");
	return result;
    }

    buffer_copy(sql, s);
    buffer_add_str(sql, ".\"");
    buffer_copy(sql, t);
    buffer_add_str(sql, "\"");

    n = n->children;

    /* jump to the next element if there are spaces */
    while (n->type != XML_ELEMENT_NODE) n = n->next;

    buffer_add_str(sql, " SET ");

    /* write the fields and the new values of the features to update */
    for (; n; n = n->next) {
        values = buffer_init();

        if (n->type == XML_ELEMENT_NODE) {
            if (!strcmp((char *) n->name, "Property")) {
                node = n->children;

                while (node->type != XML_ELEMENT_NODE) node = node->next;

                /* We have to ignore if not present in database, 
                   gml elements (name, description, boundedBy) */
                if (    !strcmp("http://www.opengis.net/gml", (char *) node->ns->href)
                     || !strcmp("http://www.opengis.net/gml/3.2", (char *) node->ns->href)) {
		      table = ows_psql_describe_table(o, typename);
		      if (!array_is_key(table, (char *) node->name)) continue;
		}

                property_name = buffer_init();

                /* property name to update */
                if (!strcmp((char *) node->name, "Name")) {
                    content = xmlNodeGetContent(node);
                    buffer_add_str(property_name, (char *) content);
                    xmlFree(content);
                    wfs_request_remove_namespaces(o, property_name);
                    buffer_add_str(sql, "\"");
                    escaped = ows_psql_escape_string(o, property_name->buf);
                    if (escaped) {
                        buffer_add_str(sql, escaped);
                        free(escaped);
                    }
                    buffer_add_str(sql, "\"");
                }

                buffer_add_str(sql, " = ");
                node = node->next;

                /* jump to the next element if there are spaces */
                if (node && node->type != XML_ELEMENT_NODE) node = node->next;

                /* replacement value is optional, set to NULL if not defined */
                if (!node) buffer_add_str(sql, "NULL");
                else if (!strcmp((char *) node->name, "Value")) {
                    if (ows_psql_is_geometry_column(o, typename, property_name)) {
                        elemt = node->children;

                        /* jump to the next element if there are spaces */
                        while (elemt->type != XML_ELEMENT_NODE) elemt = elemt->next;

                        if (!strcmp((char *) elemt->name, "Box") ||
			    !strcmp((char *) elemt->name, "Envelope")) {

                            fe = filter_encoding_init();
                            fe->sql = buffer_init();
                            fe->sql = fe_envelope(o, typename, fe, fe->sql, elemt);

                	    if (fe->error_code != FE_NO_ERROR) {
                    		result = fill_fe_error(o, fe);
                   		buffer_free(values);
                   		buffer_free(sql);
                   		buffer_free(typename);
                		buffer_free(property_name);
                            	filter_encoding_free(fe);
                    		return result;
               		    }

                            filter_encoding_free(fe);
                            buffer_copy(values, fe->sql);

                        } else if (!strcmp((char *) elemt->name, "Null")) {
                            buffer_add_str(values, "''");
                        } else {
                            gml = ows_psql_gml_to_sql(o, elemt, srid_root);
                            if (gml) {
                                buffer_add_str(values, "'");
                                buffer_copy(values, gml);
                                buffer_add_str(values, "'");
                                buffer_free(gml);
                            } else {
                   		buffer_free(values);
                   		buffer_free(typename);
                		buffer_free(property_name);
                   		buffer_free(sql);
                    		result = buffer_from_str("Invalid GML Geometry");
				return result;
                            }
			}
                    } else values = wfs_retrieve_value(o, wr, values, xmldoc, node);

                    buffer_copy(sql, values);
                }
                buffer_free(property_name);
            }

            if (!strcmp((char *) n->name, "Filter")) {
                buffer_add_str(sql, " WHERE ");
                xmlstring = buffer_init();
                xmlstring = cgi_add_xml_into_buffer(xmlstring, n);

                filter = filter_encoding_init();
                filter = fe_filter(o, filter, typename, xmlstring);

                /* check if filter returned an error */
                if (filter->error_code != FE_NO_ERROR) {
                    result = fill_fe_error(o, filter);
                    buffer_free(xmlstring);
                    filter_encoding_free(filter);
                    buffer_free(values);
                    buffer_free(sql);
                    buffer_free(typename);
                    return result;

                } else {
                    buffer_copy(sql, filter->sql);
                    buffer_free(xmlstring);
                    filter_encoding_free(filter);
                }
            }
        }

        if (n->next && !strcmp((char *) n->next->name, "Property")) buffer_add_str(sql, ",");

        buffer_free(values);
    }

    buffer_add_str(sql, "; ");
    /* run the request to update the specified features */
    result = wfs_execute_transaction_request(o, wr, sql);

    buffer_free(typename);
    buffer_free(sql);

    return result;
}


/*
 * Parse XML operations to execute each transaction operation
 */
void wfs_parse_operation(ows * o, wfs_request * wr, buffer * op)
{
    xmlDocPtr xmldoc;
    xmlNodePtr n;
    xmlAttr *att;
    xmlChar *content;

    buffer *sql, *result, *end_transaction, *locator;

    assert(o);
    assert(wr);
    assert(op);

    sql = buffer_init();
    locator = buffer_init();
    wr->insert_results = alist_init();
    content = NULL;

    xmldoc = xmlParseMemory(op->buf, op->use);

    if (!xmldoc) {
        xmlFreeDoc(xmldoc);
        wfs_error(o, wr, WFS_ERROR_NO_MATCHING, "xml isn't valid", "transaction");
        return;
    }

    /* jump to the next element if there are spaces */
    n = xmldoc->children;
    while (n->type != XML_ELEMENT_NODE) n = n->next;
    n = n->children;
    while (n->type != XML_ELEMENT_NODE) n = n->next; /* FIXME really ? */

    /* initialize the transaction inside postgresql */
    buffer_add_str(sql, "BEGIN;");
    result = wfs_execute_transaction_request(o, wr, sql);

    buffer_empty(result);
    buffer_add_str(result, "PGRES_COMMAND_OK");
    buffer_empty(sql);

    /* go through the operations while transaction is successful */
    for ( /* empty */ ; n && (buffer_cmp(result, "PGRES_COMMAND_OK")) ; n = n->next) {
        if (n->type != XML_ELEMENT_NODE) continue;

             if (!strcmp((char *) n->name, "Insert")) {
            buffer_free(result);
            result = wfs_insert_xml(o, wr, xmldoc, n);
        } else if (!strcmp((char *) n->name, "Delete")) {
            buffer_free(result);
            result = wfs_delete_xml(o, wr, n);
        } else if (!strcmp((char *) n->name, "Update")) {
            buffer_free(result);
            result = wfs_update_xml(o, wr, xmldoc, n);
        }

        /* fill locator only if transaction failed */
        if (!buffer_cmp(result, "PGRES_COMMAND_OK")) {
            /* fill locator with  handle attribute if specified
               else with transaction name */
            if (n->properties) {
                att = n->properties;

                if (!strcmp((char *) att->name, "handle")) {
                    content = xmlNodeGetContent(att->children);
                    buffer_add_str(locator, (char *) content);
                    xmlFree(content);
                } else buffer_add_str(locator, (char *) n->name);
            } else     buffer_add_str(locator, (char *) n->name);
        }
    }

    /* end the transaction according to the result */
    if (buffer_cmp(result, "PGRES_COMMAND_OK")) buffer_add_str(sql, "COMMIT;");
    else                                        buffer_add_str(sql, "ROLLBACK;");

    end_transaction = wfs_execute_transaction_request(o, wr, sql);
    buffer_free(end_transaction);

    /* display the xml transaction response */
    wfs_transaction_response(o, wr, result, locator);

    buffer_free(sql);
    buffer_free(result);
    buffer_free(locator);

    xmlFreeDoc(xmldoc);
}


/*
 * vim: expandtab sw=4 ts=4
 */

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
#include <string.h>
#include <ctype.h>

#include "../ows/ows.h"


/* 
 * Execute the request sql matching a transaction
 * Return the result of the request (PGRES_COMMAND_OK or an error message)
 */
static buffer *wfs_execute_transaction_request(ows * o, wfs_request * wr,
   buffer * sql)
{
	buffer *result, *cmd_status;
	PGresult *res;

	assert(o != NULL);
	assert(sql != NULL);

	result = buffer_init();
	cmd_status = buffer_init();

	res = PQexec(o->pg, sql->buf);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
		buffer_add_str(result, PQresultErrorMessage(res));
	else
		buffer_add_str(result, PQresStatus(PQresultStatus(res)));

	buffer_add_str(cmd_status, (char *) PQcmdStatus(res));
	if (check_regexp(cmd_status->buf, "^DELETE"))
	{
		cmd_status = buffer_replace(cmd_status, "DELETE ", "");
		wr->delete_results = wr->delete_results + atoi(cmd_status->buf);
	}
	if (check_regexp(cmd_status->buf, "^UPDATE"))
	{
		cmd_status = buffer_replace(cmd_status, "UPDATE ", "");
		wr->update_results = wr->update_results + atoi(cmd_status->buf);
	}

	buffer_free(cmd_status);

	PQclear(res);

	return result;
}


/*
 * Summarize overall results of transaction request
 */
static void wfs_transaction_summary(ows * o, wfs_request * wr,
   buffer * result)
{
	mlist_node *mln;
	int insert_nb;

	assert(o != NULL);
	assert(wr != NULL);
	assert(result != NULL);

	mln = NULL;
	insert_nb = 0;

	fprintf(o->output, "<wfs:TransactionSummary>\n");
	if (buffer_cmp(result, "PGRES_COMMAND_OK"))
	{
		if (wr->insert_results != NULL)
		{
			for (mln = wr->insert_results->first; mln != NULL;
			   mln = mln->next)
				insert_nb = insert_nb + mln->value->size;
			fprintf(o->output, "<wfs:totalInserted>%d", insert_nb);
			fprintf(o->output, "</wfs:totalInserted>\n");
		}
		if (wr->delete_results != 0)
			fprintf(o->output, "<wfs:totalDeleted>%d</wfs:totalDeleted>\n",
			   wr->delete_results);
		if (wr->update_results != 0)
			fprintf(o->output, "<wfs:totalUpdated>%d</wfs:totalUpdated>\n",
			   wr->update_results);
	}
	fprintf(o->output, "</wfs:TransactionSummary>\n");
}


/*
 * Report newly created feature instances
 */
static void wfs_transaction_insert_result(ows * o, wfs_request * wr,
   buffer * result)
{
	mlist_node *mln;
	list_node *hdl, *ln;

	assert(o != NULL);
	assert(wr != NULL);
	assert(result != NULL);

	mln = NULL;
	hdl = NULL;
	ln = NULL;

	/* check if there were Insert operations and if the command succeeded */
	if ((!cgi_method_get()) && (buffer_cmp(result, "PGRES_COMMAND_OK")
		  && (wr->insert_results != NULL)))
	{
		if (wr->handle != NULL)
			hdl = wr->handle->first;
		if (ows_version_get(o->request->version) == 110)
			fprintf(o->output, "<wfs:InsertResults>\n");
		for (mln = wr->insert_results->first; mln != NULL; mln = mln->next)
		{
			if (ows_version_get(o->request->version) == 100)
			{
				if (wr->handle != NULL)
					fprintf(o->output, "<wfs:InsertResult handle=\"%s\">",
					   hdl->value->buf);
				else
					fprintf(o->output, "<wfs:InsertResult>");
			}
			else
			{
				if (wr->handle != NULL)
					fprintf(o->output, "<wfs:Feature handle=\"%s\">",
					   hdl->value->buf);
				else
					fprintf(o->output, "<wfs:Feature>");
			}

			for (ln = mln->value->first; ln != NULL; ln = ln->next)
				fprintf(o->output, "<ogc:FeatureId fid=\"%s\"/>",
				   ln->value->buf);

			if (ows_version_get(o->request->version) == 100)
				fprintf(o->output, "</wfs:InsertResult>\n");
			else
				fprintf(o->output, "</wfs:Feature>\n");

			if (wr->handle != NULL)
				hdl = hdl->next;
		}
		if (ows_version_get(o->request->version) == 110)
			fprintf(o->output, "<wfs:InsertResults>");
	}
}


/*
 * Report the overall result of the transaction request
 */
static void wfs_transaction_result(ows * o, wfs_request * wr,
   buffer * result, buffer * locator)
{
	assert(o != NULL);
	assert(wr != NULL);
	assert(result != NULL);
	if (!buffer_cmp(result, "PGRES_COMMAND_OK"))
		assert(locator != NULL);

	/* only if version = 1.0.0 or if command failed */
	if (ows_version_get(o->request->version) == 100
	   || (ows_version_get(o->request->version) == 110
		  && !buffer_cmp(result, "PGRES_COMMAND_OK")))
	{

		if (ows_version_get(o->request->version) == 110)
			fprintf(o->output, "<wfs:TransactionResults>\n");
		else
		{
			fprintf(o->output, "<wfs:TransactionResult>\n");
			/* display status transaction only for 1.0.0 version */
			fprintf(o->output, "<wfs:Status>");
			if (buffer_cmp(result, "PGRES_COMMAND_OK"))
				fprintf(o->output, "<wfs:SUCCESS/>");
			else
				fprintf(o->output, "<wfs:FAILED/>");
			fprintf(o->output, "</wfs:Status>\n");
		}
		/* place where the transaction failed */
		if (!buffer_cmp(result, "PGRES_COMMAND_OK"))
		{
			if (ows_version_get(o->request->version) == 100)
				fprintf(o->output, "<wfs:Locator>%s</wfs:Locator>\n",
				   locator->buf);
			else
				fprintf(o->output, "<wfs:Action locator=%s>\n",
				   locator->buf);
		}

		/* error message if the transaction failed */
		if (!buffer_cmp(result, "PGRES_COMMAND_OK"))
		{
			fprintf(o->output, " <wfs:Message>%s</wfs:Message>\n",
			   result->buf);
		}
		if (ows_version_get(o->request->version) == 100)
			fprintf(o->output, "</wfs:TransactionResult>\n");
		else
		{
			fprintf(o->output, "</wfs:Action>\n");
			fprintf(o->output, "</wfs:TransactionResults>\n");
		}
	}
}


/*
 * Write the transaction response in XML
 */
static void wfs_transaction_response(ows * o, wfs_request * wr,
   buffer * result, buffer * locator)
{
	assert(o != NULL);
	assert(wr != NULL);
	assert(result != NULL);

	fprintf(o->output, "Content-Type: application/xml\n\n");
	fprintf(o->output, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	if (ows_version_get(o->request->version) == 100)
		fprintf(o->output,
		   "<wfs:WFS_TransactionResponse version=\"1.0.0\"\n");
	else
		fprintf(o->output, "<wfs:TransactionResponse version=\"1.1.0\"\n");

	fprintf(o->output, " xmlns:wfs=\"http://www.opengis.net/wfs\"\n");
	fprintf(o->output,
	   " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
	fprintf(o->output, "xmlns:ogc=\"http://www.opengis.net/ogc\"");

	if (ows_version_get(o->request->version) == 100) {
		fprintf(o->output, " xsi:schemaLocation='http://www.opengis.net/wfs");
		fprintf(o->output, 
            " http://schemas.opengis.net/wfs/1.0.0/WFS-transaction.xsd' >\n");
    } else {
		fprintf(o->output, " xsi:schemaLocation='http://www.opengis.net/wfs");
		fprintf(o->output, 
            " http://schemas.opengis.net/wfs/1.1.0/wfs.xsd' >\n");
    }


	if (ows_version_get(o->request->version) == 100)
	{
		wfs_transaction_insert_result(o, wr, result);
		wfs_transaction_result(o, wr, result, locator);
	}
	else
	{
		wfs_transaction_summary(o, wr, result);
		wfs_transaction_result(o, wr, result, locator);
		wfs_transaction_insert_result(o, wr, result);
	}

	if (ows_version_get(o->request->version) == 100)
		fprintf(o->output, "</wfs:WFS_TransactionResponse>\n");
	else
		fprintf(o->output, "</wfs:TransactionResponse>\n");
}


/* 
 * Add a content node value to a buffer
 */
static buffer *wfs_retrieve_value(ows * o, wfs_request * wr,
   buffer * value, xmlNodePtr n)
{
	xmlChar *content;

	assert(o != NULL);
	assert(wr != NULL);
	assert(value != NULL);
	assert(n != NULL);

	content = xmlNodeGetContent(n);
	/*if the value is a string, must be in quotation marks */
	if (check_regexp((char *) content,
		  "^[A-Za-z]") == 1
	   || check_regexp((char *) content, ".*-.*") == 1)
		buffer_add_str(value, "'");

	buffer_add_str(value, (char *) content);

	if (check_regexp((char *) content,
		  "^[A-Za-z]") == 1
	   || check_regexp((char *) content, ".*-.*") == 1)
		buffer_add_str(value, "'");

	xmlFree(content);

	return value;
}


/*
 * Retrieve the layer's name
 */
static buffer *wfs_retrieve_typename(ows * o, wfs_request * wr,
   xmlNodePtr n)
{
	xmlAttr *att;
	xmlChar *content;
	buffer *typename;

	typename = buffer_init();
	content = NULL;

	for (att = n->properties; att != NULL; att = att->next)
	{
		if (strcmp((char *) att->name, "typeName") == 0)
		{
			content = xmlNodeGetContent(att->children);
			buffer_add_str(typename, (char *) content);
			wfs_request_remove_namespaces(o, typename);
			xmlFree(content);
		}
	}
	return typename;
}


/* 
 * Insert features into the database 
 * Method POST, XML
 */
static buffer *wfs_insert_xml(ows * o, wfs_request * wr, xmlNodePtr n)
{
	buffer *values, *column, *layer_name, *result, *sql, *id, *tmp;
	list *fid;
	filter_encoding *fe;
	xmlNodePtr node, elemt;
	xmlChar *content;

	assert(o != NULL);
	assert(wr != NULL);
	assert(n != NULL);

	sql = buffer_init();
	fid = list_init();
	content = NULL;

	/* retrieve handle attribute to report it in transaction response */
	if (n->properties != NULL)
	{
		if (strcmp((char *) n->properties->name, "handle") == 0)
		{
			wr->handle = list_init();
			content = xmlNodeGetContent(n->properties->children);
			tmp = buffer_init();
			buffer_add_str(tmp, (char *) content);
			list_add_by_copy(wr->handle, tmp);
			xmlFree(content);
			buffer_free(tmp);
		}
	}

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	/* insert elements layer by layer */
	for (; n; n = n->next)
	{
		if (n->type != XML_ELEMENT_NODE)
			continue;

		values = buffer_init();
		layer_name = buffer_init();

		/* name of the table in which features must be inserted */
		buffer_add_str(layer_name, (char *) n->name);
		wfs_request_remove_namespaces(o, layer_name);

		id = ows_psql_id_column(o, layer_name);

		buffer_add_str(sql, "INSERT INTO \"");
		buffer_copy(sql, layer_name);
		buffer_add_str(sql, "\" (");

		node = n->children;

		/* jump to the next element if there are spaces */
		while (node->type != XML_ELEMENT_NODE)
			node = node->next;

		/* fill fields and values at the same time */
		for (; node; node = node->next)
		{
			if (node->type == XML_ELEMENT_NODE)
			{
				column = buffer_init();
				buffer_add_str(column, (char *) node->name);

				buffer_add_str(sql, "\"");
				buffer_copy(sql, column);
				buffer_add_str(sql, "\"");
				/*if column's type is geometry, transform the gml into WKT */
				if (ows_psql_is_geometry_column(o, layer_name, column))
				{
					elemt = node->children;
					/* jump to the next element if there are spaces */
					while (elemt->type != XML_ELEMENT_NODE)
						elemt = elemt->next;

					fe = filter_encoding_init();
					fe->sql = buffer_init();
					if (strcmp((char *) elemt->name, "Box") == 0
					   || strcmp((char *) elemt->name, "Envelope") == 0)
					{
						fe->sql = fe_envelope(o, layer_name, fe, elemt);
						buffer_copy(values, fe->sql);
					}
					else if (strcmp((char *) elemt->name, "Null") == 0)
					{
						buffer_add_str(values, "''");
					}
					else
					{
						fe->sql =
						   fe_transform_geometry_gml_to_psql(o,
						   layer_name, fe, elemt);
						buffer_copy(values, fe->sql);
					}
					filter_encoding_free(fe);
				}
				else
				{
					content = xmlNodeGetContent(node);
					/* if it's about id column, ID is put into 
					   a multiple list of feature id to use them 
					   during the transaction_response */
					if (id->use != 0)
					{
						if (buffer_cmp(id, (char *) node->name))
						{
							tmp = buffer_init();
							buffer_copy(tmp, layer_name);
							buffer_add_str(tmp, ".");
							buffer_add_str(tmp, (char *) content);
							/* retrieve the id of the inserted feature 
							   to report it in transaction response */
							list_add(fid, tmp);
						}
					}
					xmlFree(content);
					values = wfs_retrieve_value(o, wr, values, node);
				}
				buffer_free(column);
			}
			if (node->next != NULL)
				if (node->next->type == XML_ELEMENT_NODE)
				{
					buffer_add_str(sql, ",");
					buffer_add_str(values, ",");
				}
		}

		buffer_add_str(sql, " ) VALUES (");

		buffer_copy(sql, values);
		buffer_add_str(sql, ")");

		buffer_add_str(sql, "; ");

		buffer_free(values);
		buffer_free(id);
		buffer_free(layer_name);
	}

	/* list of featureid inserted to be used during transaction response */
	wr->insert_results = mlist_init();
	mlist_add(wr->insert_results, fid);

	/* run the request to insert all features at the same time */
	result = wfs_execute_transaction_request(o, wr, sql);
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

	assert(o != NULL);
	assert(wr != NULL);

	sql = buffer_init();
	ln = NULL;
	ln_typename = NULL;
	ln_filter = NULL;
	mln_fid = NULL;
	where = NULL;
	size = 0;

	if (wr->typename != NULL)
	{
		size = wr->typename->size;
		ln_typename = wr->typename->first;
	}
	if (wr->filter != NULL)
		ln_filter = wr->filter->first;

	if (wr->featureid != NULL)
	{
		size = wr->featureid->size;
		mln_fid = wr->featureid->first;
	}

	/* delete elements layer by layer */
	for (cpt = 0; cpt < size; cpt++)
	{
		/* define a layer_name which match typename or featureid */
		layer_name = buffer_init();
		if (wr->typename != NULL)
			buffer_copy(layer_name, ln_typename->value);
		else
		{
			fe = list_explode('.', mln_fid->value->first->value);
			buffer_copy(layer_name, fe->first->value);
			list_free(fe);
		}

		/* FROM */
		buffer_add_str(sql, "DELETE FROM \"");
		buffer_copy(sql, layer_name);
		buffer_add_str(sql, "\" ");

		/* WHERE : match featureid, bbox or filter */

		/* FeatureId */
		if (wr->featureid != NULL)
		{
			where = fe_kvp_featureid(o, wr, layer_name, mln_fid->value);
			if (where->use == 0)
			{
				buffer_free(where);
				buffer_free(sql);
				buffer_free(layer_name);
				wfs_error(o, wr, WFS_ERROR_NO_MATCHING,
				   "error : an id_column is required to use featureid",
				   "Delete");
			}
		}
		/* BBOX */
		else if (wr->bbox != NULL)
			where = fe_kvp_bbox(o, wr, layer_name, wr->bbox);
		/* Filter */
		else
		{
			if (ln_filter->value->use != 0)
			{
				where = buffer_init();
				buffer_add_str(where, " WHERE ");
				filter = filter_encoding_init();
				filter =
				   fe_filter(o, filter, layer_name, ln_filter->value);
				if (filter->error_code != FE_NO_ERROR)
				{
					buffer_free(where);
					buffer_free(sql);
					buffer_free(layer_name);
					fe_error(o, filter);

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
		if (wr->featureid != NULL)
			mln_fid = mln_fid->next;
		if (wr->typename != NULL)
			ln_typename = ln_typename->next;
		if (wr->filter != NULL)
			ln_filter = ln_filter->next;
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
	buffer *typename, *xmlstring, *result, *sql;
	filter_encoding *filter;

	assert(o != NULL);
	assert(wr != NULL);
	assert(n != NULL);

	sql = buffer_init();
	xmlstring = buffer_init();

	buffer_add_str(sql, "DELETE FROM \"");

	/*retrieve the name of the table in which features must be deleted */
	typename = wfs_retrieve_typename(o, wr, n);
	buffer_copy(sql, typename);
	buffer_add_str(sql, "\"");

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	buffer_add_str(sql, " WHERE ");

	/* put xml filter into a buffer */
	xmlstring = cgi_add_xml_into_buffer(xmlstring, n);

	filter = filter_encoding_init();
	filter = fe_filter(o, filter, typename, xmlstring);

	/* check if filter returned an error */
	if (filter->error_code != FE_NO_ERROR)
		result = fill_fe_error(o, filter);
	else
	{
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
static buffer *wfs_update_xml(ows * o, wfs_request * wr, xmlNodePtr n)
{
	buffer *typename, *xmlstring, *result, *sql, *property_name;
	filter_encoding *filter;
	xmlNodePtr node;
	xmlChar *content;

	assert(o != NULL);
	assert(wr != NULL);
	assert(n != NULL);

	sql = buffer_init();
	content = NULL;

	buffer_add_str(sql, "UPDATE \"");

	/*retrieve the name of the table in which features must be updated */
	typename = wfs_retrieve_typename(o, wr, n);
	buffer_copy(sql, typename);
	buffer_add_str(sql, "\"");

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	buffer_add_str(sql, " SET ");

	/* write the fields and the new values of the features to update */
	for (; n; n = n->next)
	{
		if (n->type == XML_ELEMENT_NODE)
		{
			if (strcmp((char *) n->name, "Property") == 0)
			{
				node = n->children;
				while (node->type != XML_ELEMENT_NODE)
					node = node->next;

				/* property name to update */
				if (strcmp((char *) node->name, "Name") == 0)
				{
					property_name = buffer_init();
					content = xmlNodeGetContent(node);
					buffer_add_str(property_name, (char *) content);
					xmlFree(content);
					wfs_request_remove_namespaces(o, property_name);
					buffer_copy(sql, property_name);
				}

				buffer_add_str(sql, " = ");
				node = node->next;
				/* jump to the next element if there are spaces */
				if (node != NULL)
					if (node->type != XML_ELEMENT_NODE)
						node = node->next;

				/* replacement value is optional, set to NULL if not defined */
				if (node == NULL)
					buffer_add_str(sql, "NULL");
				else if (strcmp((char *) node->name, "Value") == 0)
					sql = wfs_retrieve_value(o, wr, sql, node);
			}
			if (strcmp((char *) n->name, "Filter") == 0)
			{
				buffer_add_str(sql, " WHERE ");
				xmlstring = buffer_init();
				xmlstring = cgi_add_xml_into_buffer(xmlstring, n);

				filter = filter_encoding_init();
				filter = fe_filter(o, filter, typename, xmlstring);

				/* check if filter returned an error */
				if (filter->error_code != FE_NO_ERROR)
				{
					result = fill_fe_error(o, filter);
					buffer_free(xmlstring);
					filter_encoding_free(filter);
					buffer_free(typename);
					buffer_free(sql);
					return result;
				}
				else
				{
					buffer_copy(sql, filter->sql);
					buffer_free(xmlstring);
					filter_encoding_free(filter);
				}
			}
		}
		if (n->next != NULL)
			if (strcmp((char *) n->next->name, "Property") == 0)
				buffer_add_str(sql, ",");
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

	assert(o != NULL);
	assert(wr != NULL);
	assert(op != NULL);

	sql = buffer_init();
	locator = buffer_init();
	wr->featureid = mlist_init();
	content = NULL;

	xmlInitParser();
	xmldoc = xmlParseMemory(op->buf, op->use);
	if (!xmldoc)
	{
		xmlFreeDoc(xmldoc);
		xmlCleanupParser();
		wfs_error(o, wr, WFS_ERROR_NO_MATCHING, "xml isn't valid",
		   "transaction");
	}
	n = xmldoc->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	/* initialize the transaction inside postgresql */
	buffer_add_str(sql, "BEGIN;");
	result = wfs_execute_transaction_request(o, wr, sql);

	buffer_empty(result);
	buffer_add_str(result, "PGRES_COMMAND_OK");
	buffer_empty(sql);

	/* go through the operations while transaction is successful */
	for (; (n != NULL) && (buffer_cmp(result, "PGRES_COMMAND_OK"));
	   n = n->next)
	{
		if (n->type != XML_ELEMENT_NODE)
			continue;

		if (strcmp((char *) n->name, "Insert") == 0)
		{
			buffer_free(result);
			result = wfs_insert_xml(o, wr, n);
		}
		if (strcmp((char *) n->name, "Delete") == 0)
		{
			buffer_free(result);
			result = wfs_delete_xml(o, wr, n);
		}
		if (strcmp((char *) n->name, "Update") == 0)
		{
			buffer_free(result);
			result = wfs_update_xml(o, wr, n);
		}
		/* fill locator only if transaction failed */
		if (!buffer_cmp(result, "PGRES_COMMAND_OK"))
		{
			/* fill locator with  handle attribute if specified 
			   else with transaction name */
			if (n->properties != NULL)
			{
				att = n->properties;
				if (strcmp((char *) att->name, "handle") == 0)
				{
					content = xmlNodeGetContent(att->children);
					buffer_add_str(locator, (char *) content);
					xmlFree(content);
				}
				else
					buffer_add_str(locator, (char *) n->name);
			}
			else
				buffer_add_str(locator, (char *) n->name);
		}

	}

	/* end the transaction according to the result */
	if (buffer_cmp(result, "PGRES_COMMAND_OK"))
		buffer_add_str(sql, "COMMIT;");
	else
		buffer_add_str(sql, "ROLLBACK;");

	end_transaction = wfs_execute_transaction_request(o, wr, sql);
	buffer_free(end_transaction);

	/* display the xml transaction response */
	wfs_transaction_response(o, wr, result, locator);

	buffer_free(sql);
	buffer_free(result);
	buffer_free(locator);

	xmlFreeDoc(xmldoc);
	xmlCleanupParser();
}


/*
 * vim: expandtab sw=4 ts=4
 */

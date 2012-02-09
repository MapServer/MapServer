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
 * Return expression matching the comparison operators following :
 * PropertyIsEqualTo, PropertyIsNotEqualTo, PropertyIsLessThan
 * PropertyIsGreaterThan, PropertyIsLessThanOrEqualTo, PropertyIsGreaterThanOrEqualTo
 */
static buffer *fe_binary_comparison_op(ows * o, buffer * typename,
   filter_encoding * fe, xmlNodePtr n)
{

	buffer *tmp, *type, *name;
	xmlChar *matchcase;
	bool bool_type, sensitive_case;

	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	bool_type = false;
	sensitive_case = true;

	tmp = buffer_init();
	name = buffer_init();

	buffer_add_str(name, (char *) n->name);

	/* by default, comparison is case sensitive */
	matchcase = xmlGetProp(n, (xmlChar *) "matchCase");
	if (matchcase != NULL)
	{
		if (strcmp((char *) matchcase, "false") == 0)
			sensitive_case = false;
	}
	xmlFree(matchcase);

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	/* if comparison isn't case sensitive, strings are passed in lower case */
	if (!sensitive_case)
		buffer_add_str(fe->sql, "lower(");


	tmp = fe_expression(o, typename, fe, tmp, n);
	buffer_copy(fe->sql, tmp);

	/* if property is a boolean, xml content( 1 or 0) must be transformed
	   into FE->SQL syntax (true or false) */
	if (buffer_cmp(name, "PropertyIsEqualTo")
	   || buffer_cmp(name, "PropertyIsNotEqualTo"))
	{
		type = ows_psql_type(o, typename, tmp);
		if (buffer_cmp(type, "bool"))
			bool_type = true;
		buffer_free(type);
	}


	if (!sensitive_case)
		buffer_add_str(fe->sql, ")");


	if (buffer_cmp(name, "PropertyIsEqualTo"))
		buffer_add_str(fe->sql, " = ");
	if (buffer_cmp(name, "PropertyIsNotEqualTo"))
		buffer_add_str(fe->sql, " != ");
	if (buffer_cmp(name, "PropertyIsLessThan"))
		buffer_add_str(fe->sql, " < ");
	if (buffer_cmp(name, "PropertyIsGreaterThan"))
		buffer_add_str(fe->sql, " > ");
	if (buffer_cmp(name, "PropertyIsLessThanOrEqualTo"))
		buffer_add_str(fe->sql, " <= ");
	if (buffer_cmp(name, "PropertyIsGreaterThanOrEqualTo"))
		buffer_add_str(fe->sql, " >= ");

	buffer_empty(tmp);

	n = n->next;

	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	if (!sensitive_case)
		buffer_add_str(fe->sql, "lower(");


	tmp = fe_expression(o, typename, fe, tmp, n);
	/* if property is a boolean, xml content( 1 or 0) must be transformed
	   into fe->sql (true or false) */
	if (bool_type)
	{
		if (buffer_cmp(tmp, "1"))
			buffer_add_str(fe->sql, "'t'");
		if (buffer_cmp(tmp, "0"))
			buffer_add_str(fe->sql, "'f'");
	}
	else
		buffer_copy(fe->sql, tmp);


	if (!sensitive_case)
		buffer_add_str(fe->sql, ")");

	buffer_free(tmp);
	buffer_free(name);

	return fe->sql;

}


/*
 * String comparison operator with pattern matching
 * FIX ME : remains a problem when escaping \* -> \% 
 */
static buffer *fe_property_is_like(ows * o, buffer * typename,
   filter_encoding * fe, xmlNodePtr n)
{
	xmlChar *content, *wildcard, *singlechar, *escape;
	buffer *pg_string;

	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	wildcard = xmlGetProp(n, (xmlChar *) "wildCard");
	singlechar = xmlGetProp(n, (xmlChar *) "singleChar");

	if (ows_version_get(o->request->version) == 100)
		escape = xmlGetProp(n, (xmlChar *) "escape");
	else
		escape = xmlGetProp(n, (xmlChar *) "escapeChar");


	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	fe->sql = fe_property_name(o, typename, fe, fe->sql, n);


	buffer_add_str(fe->sql, " Like E");

	n = n->next;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	content = xmlNodeGetContent(n->children);

	pg_string = buffer_init();
	buffer_add_str(pg_string, (char *) content);

	/* replace the wildcard,singlechar and escapechar by Postgrefe->sql's */
	if ((char *) wildcard != NULL && (char *) singlechar != NULL
	   && (char *) escape != NULL)
	{
		pg_string = buffer_replace(pg_string, (char *) escape, "\\\\");
		pg_string = buffer_replace(pg_string, (char *) wildcard, "%");
		pg_string = buffer_replace(pg_string, (char *) singlechar, "_");
	}
	else
	{
		fe->error_code = FE_ERROR_FILTER;
	}

	buffer_add_str(fe->sql, "'");
	buffer_copy(fe->sql, pg_string);
	buffer_add_str(fe->sql, "'");

	xmlFree(content);
	xmlFree(wildcard);
	xmlFree(singlechar);
	xmlFree(escape);
	buffer_free(pg_string);

	return fe->sql;
}


/*
 * Check if the value of a property is null
 */
static buffer *fe_property_is_null(ows * o, buffer * typename,
   filter_encoding * fe, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	fe->sql = fe_property_name(o, typename, fe, fe->sql, n);

	buffer_add_str(fe->sql, " isnull");

	return fe->sql;
}


/*
 * Check if property is between two boundary values
 */
static buffer *fe_property_is_between(ows * o, buffer * typename,
   filter_encoding * fe, xmlNodePtr n)
{
	buffer *tmp;

	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	tmp = buffer_init();

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	tmp = fe_expression(o, typename, fe, tmp, n);

	buffer_copy(fe->sql, tmp);
	buffer_empty(tmp);

	buffer_add_str(fe->sql, " Between ");

	n = n->next;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	tmp = fe_expression(o, typename, fe, tmp, n->children);

	buffer_copy(fe->sql, tmp);
	buffer_empty(tmp);

	buffer_add_str(fe->sql, " And ");

	n = n->next;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	tmp = fe_expression(o, typename, fe, tmp, n->children);

	buffer_copy(fe->sql, tmp);
	buffer_free(tmp);

	return fe->sql;
}


/* 
 * Check if the string is a comparison operator 
 */
bool fe_is_comparison_op(char *name)
{
	assert(name != NULL);

	/* case sensitive comparison because the gml standard specifies 
	   strictly the name of the operator */
	if (strcmp(name, "PropertyIsEqualTo") == 0
	   || strcmp(name, "PropertyIsNotEqualTo") == 0
	   || strcmp(name, "PropertyIsLessThan") == 0
	   || strcmp(name, "PropertyIsGreaterThan") == 0
	   || strcmp(name, "PropertyIsLessThanOrEqualTo") == 0
	   || strcmp(name, "PropertyIsGreaterThanOrEqualTo") == 0
	   || strcmp(name, "PropertyIsLike") == 0
	   || strcmp(name, "PropertyIsNull") == 0
	   || strcmp(name, "PropertyIsBetween") == 0)
		return true;

	return false;
}


/* 
 * Execute the matching function 
 * Warning : before calling this function, 
 * Check if n->name is a comparison operator with fe_is_comparison_op() 
 */
buffer *fe_comparison_op(ows * o, buffer * typename, filter_encoding * fe,
   xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	/* case sensitive comparison because the gml standard specifies 
	   strictly the name of the operator */
	if (strcmp((char *) n->name, "PropertyIsEqualTo") == 0
	   || strcmp((char *) n->name, "PropertyIsNotEqualTo") == 0
	   || strcmp((char *) n->name, "PropertyIsLessThan") == 0
	   || strcmp((char *) n->name, "PropertyIsGreaterThan") == 0
	   || strcmp((char *) n->name, "PropertyIsLessThanOrEqualTo") == 0
	   || strcmp((char *) n->name, "PropertyIsGreaterThanOrEqualTo") == 0)
		fe->sql = fe_binary_comparison_op(o, typename, fe, n);
	else if (strcmp((char *) n->name, "PropertyIsLike") == 0)
		fe->sql = fe_property_is_like(o, typename, fe, n);
	else if (strcmp((char *) n->name, "PropertyIsNull") == 0)
		fe->sql = fe_property_is_null(o, typename, fe, n);
	else if (strcmp((char *) n->name, "PropertyIsBetween") == 0)
		fe->sql = fe_property_is_between(o, typename, fe, n);
	else
		fe->error_code = FE_ERROR_FILTER;

	return fe->sql;
}


/*
 * vim: expandtab sw=4 ts=4
 */

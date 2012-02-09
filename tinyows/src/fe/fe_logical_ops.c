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
 * Check if the string is a logical operator 
 */
bool fe_is_logical_op(char *name)
{
	assert(name != NULL);

	/* case sensitive comparison because the gml standard specifies 
	   strictly the name of the operator */
	if (strcmp(name, "And") == 0
	   || strcmp(name, "Or") == 0 || strcmp(name, "Not") == 0)
		return true;

	return false;
}


/* 
 * If the logical operator is binary, go through the nodes recursively 
 * and fill the SQL request buffer 
 */
static buffer *fe_binary_logical_op(ows * o, buffer * typename,
   filter_encoding * fe, xmlNodePtr n)
{
	xmlNodePtr node;
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	buffer_add_str(fe->sql, "(");

	node = n->children;

	/* jump to the next element if there are spaces */
	while (node->type != XML_ELEMENT_NODE)
		node = node->next;

	/* execute the matching function's type */
	if (fe_is_logical_op((char *) node->name))
		fe->sql = fe_logical_op(o, typename, fe, node);
	else if (fe_is_spatial_op((char *) node->name))
		fe->sql = fe_spatial_op(o, typename, fe, node);
	else if (fe_is_comparison_op((char *) node->name))
		fe->sql = fe_comparison_op(o, typename, fe, node);

	if (strcmp((char *) n->name, "And") == 0)
		buffer_add_str(fe->sql, " AND ");
	else if (strcmp((char *) n->name, "Or") == 0)
		buffer_add_str(fe->sql, " OR ");

	node = node->next;

	/* jump to the next element if there are spaces */
	while (node->type != XML_ELEMENT_NODE)
		node = node->next;

	/* execute the matching function's type */
	if (fe_is_logical_op((char *) node->name))
		fe->sql = fe_logical_op(o, typename, fe, node);
	else if (fe_is_spatial_op((char *) node->name))
		fe->sql = fe_spatial_op(o, typename, fe, node);
	else if (fe_is_comparison_op((char *) node->name))
		fe->sql = fe_comparison_op(o, typename, fe, node);

	buffer_add_str(fe->sql, ")");

	return fe->sql;
}


/* 
 * If the logical operator is unary, go through the nodes recursively 
 * and fill the SQL request buffer
 */
static buffer *fe_unary_logical_op(ows * o, buffer * typename,
   filter_encoding * fe, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	buffer_add_str(fe->sql, "not(");

	n = n->children;

	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	/* execute the matching function's type */
	if (fe_is_logical_op((char *) n->name))
		fe->sql = fe_logical_op(o, typename, fe, n);
	else if (fe_is_spatial_op((char *) n->name))
		fe->sql = fe_spatial_op(o, typename, fe, n);
	else if (fe_is_comparison_op((char *) n->name))
		fe->sql = fe_comparison_op(o, typename, fe, n);

	buffer_add_str(fe->sql, ")");

	return fe->sql;
}


/* 
 * Execute the matching function (And, Or or Not)
 * Warning : before calling this function, 
 * Check if the node name is a logical operator with is_logical_op() 
 */
buffer *fe_logical_op(ows * o, buffer * typename, filter_encoding * fe,
   xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	/* case sensitive comparison because the gml standard specifies 
	   strictly the name of the operator */
	if (strcmp((char *) n->name, "And") == 0
	   || strcmp((char *) n->name, "Or") == 0)
		fe->sql = fe_binary_logical_op(o, typename, fe, n);
	else if (strcmp((char *) n->name, "Not") == 0)
		fe->sql = fe_unary_logical_op(o, typename, fe, n);
	else
		fe->error_code = FE_ERROR_FILTER;

	return fe->sql;
}


/*
 * vim: expandtab sw=4 ts=4
 */

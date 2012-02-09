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
 * Generic function for filter encoding functions with one argument 
 */
static buffer *fe_functions(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	sql = fe_expression(o, typename, fe, sql, n);

	buffer_add_str(sql, ")");

	return sql;
}


/* 
 * Generic function for aggregate filter encoding functions 
 */
static buffer *fe_aggregate_functions(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	n = n->children;

	/* jump to the next element if there are spaces */
	while (n->type != XML_ELEMENT_NODE)
		n = n->next;

	sql = fe_expression(o, typename, fe, sql, n);

	buffer_add_str(sql, ") from ");
	buffer_copy(sql, typename);
	buffer_add_str(sql, ")");

	return sql;
}


/* 
 * Calculate the absolute value of the argument 
 */
static buffer *fe_fct_abs(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "abs(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the arc cosine of the argument 
 */
static buffer *fe_fct_acos(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "acos(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the arc sine of the argument 
 */
static buffer *fe_fct_asin(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "asin(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the arc tangent of the argument 
 */
static buffer *fe_fct_atan(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "atan(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Return the average value for any number of fields 
 */
static buffer *fe_fct_avg(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "(Select avg(");

	sql = fe_aggregate_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Return the cube root of the argument 
 */
static buffer *fe_fct_cbrt(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "cbrt(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Return the smallest integer not less than argument 
 */
static buffer *fe_fct_ceil(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);

	buffer_add_str(sql, "ceil(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the cosine of the argument 
 */
static buffer *fe_fct_cos(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "cos(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the cotangent of the argument
 */
static buffer *fe_fct_cot(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "cot(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Return the number of rows
 */
static buffer *fe_fct_count(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "(Select count(");

	sql = fe_aggregate_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Transform the argument from radians to degrees
 */
static buffer *fe_fct_degrees(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "degrees(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the exponantial of the argument
 */
static buffer *fe_fct_exp(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "exp(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Return the largest integer not greater than argument
 */
static buffer *fe_fct_floor(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "floor(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Return the length of the argument
 */
static buffer *fe_fct_length(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "length(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the natural logarithm of the argument
 */
static buffer *fe_fct_ln(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "ln(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the base 10 logarithm of the argument
 */
static buffer *fe_fct_log(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "log(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Return the minimum of a set of rows
 */
static buffer *fe_fct_min(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "(Select Min(");

	sql = fe_aggregate_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Return the maximum of a set of rows
 */
static buffer *fe_fct_max(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "(Select Max(");

	sql = fe_aggregate_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Transform the argument from degrees to radians
 */
static buffer *fe_fct_radians(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "radians(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}

/* 
 * Return the round to nearest integer of the argument
 */
static buffer *fe_fct_round(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "round(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the sine of the argument
 */
static buffer *fe_fct_sin(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "sin(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the square root of the argument
 */
static buffer *fe_fct_sqrt(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "sqrt(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Calculate the tangent of the argument
 */
static buffer *fe_fct_tan(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "tan(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Truncate the argument toward zero
 */
static buffer *fe_fct_trunc(ows * o, buffer * typename,
   filter_encoding * fe, buffer * sql, xmlNodePtr n)
{
	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	buffer_add_str(sql, "trunc(");

	sql = fe_functions(o, typename, fe, sql, n);

	return sql;
}


/* 
 * Call the right function
 */
buffer *fe_function(ows * o, buffer * typename, filter_encoding * fe,
   buffer * sql, xmlNodePtr n)
{
	xmlChar *fct_name;

	assert(o != NULL);
	assert(typename != NULL);
	assert(fe != NULL);
	assert(n != NULL);
	assert(sql != NULL);

	fct_name = xmlGetProp(n, (xmlChar *) "name");

	assert(fct_name != NULL);

	if (strcmp((char *) fct_name, "abs") == 0)
		sql = fe_fct_abs(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "acos") == 0)
		sql = fe_fct_acos(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "asin") == 0)
		sql = fe_fct_asin(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "atan") == 0)
		sql = fe_fct_atan(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "avg") == 0)
		sql = fe_fct_avg(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "cbrt") == 0)
		sql = fe_fct_cbrt(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "ceil") == 0)
		sql = fe_fct_ceil(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "ceiling") == 0)
		sql = fe_fct_ceil(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "cos") == 0)
		sql = fe_fct_cos(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "cot") == 0)
		sql = fe_fct_cot(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "count") == 0)
		sql = fe_fct_count(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "degrees") == 0)
		sql = fe_fct_degrees(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "exp") == 0)
		sql = fe_fct_exp(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "floor") == 0)
		sql = fe_fct_floor(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "length") == 0)
		sql = fe_fct_length(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "ln") == 0)
		sql = fe_fct_ln(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "log") == 0)
		sql = fe_fct_log(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "min") == 0)
		sql = fe_fct_min(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "max") == 0)
		sql = fe_fct_max(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "radians") == 0)
		sql = fe_fct_radians(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "round") == 0)
		sql = fe_fct_round(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "sin") == 0)
		sql = fe_fct_sin(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "sqrt") == 0)
		sql = fe_fct_sqrt(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "tan") == 0)
		sql = fe_fct_tan(o, typename, fe, sql, n);
	if (strcmp((char *) fct_name, "trunc") == 0)
		sql = fe_fct_trunc(o, typename, fe, sql, n);
	xmlFree(fct_name);

	return sql;
}


/*
 * vim: expandtab sw=4 ts=4
 */

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
#include <stdio.h>				/* FILE */
#include <string.h>				/* strncmp */
#include <limits.h>
#include <assert.h>

#include "../ows/ows.h"


/*
 * Initialize a list structure
 */
list *list_init()
{

	list *l = NULL;

	l = malloc(sizeof(list));
	assert(l != NULL);

	l->first = NULL;
	l->last = NULL;
	l->size = 0;

	return l;
}


/*
 * Free a list structure
 */
void list_free(list * l)
{

	assert(l != NULL);

	while (l->first != NULL)
		list_node_free(l, l->first);


	l->last = NULL;

	free(l);
	l = NULL;
}


/*
 * Add a given buffer to the end of a list
 * Careful buffer is passed by reference, 
 * and must be free with list_free()
 */
void list_add(list * l, buffer * value)
{
	list_node *ln;

	assert(l != NULL);
	assert(value != NULL);
	assert(l->size < UINT_MAX);

	ln = list_node_init();

	ln->value = value;

	if (l->first == NULL)
	{
		ln->prev = NULL;
		l->first = ln;
	}
	else
	{
		ln->prev = l->last;
		l->last->next = ln;
	}

	l->last = ln;
	l->last->next = NULL;
	l->size++;
}


/*
 * Add a given buffer to the end of a list
 * Careful buffer is passed by reference, 
 * and must be free with list_free()
 */
void list_add_str(list * l, char *value)
{
	list_node *ln;

	assert(l != NULL);
	assert(value != NULL);
	assert(l->size < UINT_MAX);

	ln = list_node_init();

	ln->value = buffer_init();
	buffer_add_str(ln->value, value);

	if (l->first == NULL)
	{
		ln->prev = NULL;
		l->first = ln;
	}
	else
	{
		ln->prev = l->last;
		l->last->next = ln;
	}

	l->last = ln;
	l->last->next = NULL;
	l->size++;
}


/*
 * Add a given list to the end of a list
 * Careful list is passed by reference, 
 * and must be free with list_free()
 */
void list_add_list(list * l, list * l_to_add)
{
	list_node *ln, *ln_parse;

	assert(l != NULL);
	assert(l_to_add != NULL);

	for (ln_parse = l_to_add->first; ln_parse != NULL;
	   ln_parse = ln_parse->next)
	{
		if (!in_list(l, ln_parse->value))
		{
			ln = list_node_init();

			ln->value = buffer_init();
			buffer_copy(ln->value, ln_parse->value);

			if (l->first == NULL)
			{
				ln->prev = NULL;
				l->first = ln;
			}
			else
			{
				ln->prev = l->last;
				l->last->next = ln;
			}

			l->last = ln;
			l->last->next = NULL;
			l->size++;
		}
	}
}


/*
 * Add a given buffer by copy to the end of a list
 * Careful buffer is passed by reference, 
 * and must be free with list_free()
 */
void list_add_by_copy(list * l, buffer * value)
{
	list_node *ln;
	buffer *tmp;

	assert(l != NULL);
	assert(value != NULL);
	assert(l->size < UINT_MAX);

	ln = list_node_init();
	tmp = buffer_init();

	buffer_copy(tmp, value);
	ln->value = tmp;

	if (l->first == NULL)
	{
		ln->prev = NULL;
		l->first = ln;
	}
	else
	{
		ln->prev = l->last;
		l->last->next = ln;
	}

	l->last = ln;
	l->last->next = NULL;
	l->size++;

}


/*
 * Initialize a list node
 */
list_node *list_node_init()
{
	list_node *ln;

	ln = malloc(sizeof(list_node));
	assert(ln != NULL);

	ln->value = NULL;
	ln->prev = NULL;
	ln->next = NULL;

	return ln;
}


/*
 * Free a list node
 */
void list_node_free(list * l, list_node * ln)
{
	assert(ln != NULL);

	if (ln->prev != NULL)
		ln->prev = NULL;


	if (ln->next != NULL)
	{
		if (l != NULL)
			l->first = ln->next;
		ln->next = NULL;
	}
	else
	{
		if (l != NULL)
			l->first = NULL;
	}

	if (ln->value != NULL)
		buffer_free(ln->value);

	free(ln);
	ln = NULL;

}


/*
 * Check if a given buffer value is or not in the list
 */
bool in_list(const list * l, const buffer * value)
{
	list_node *ln;

	assert(l != NULL);
	assert(value != NULL);

	for (ln = l->first; ln != NULL; ln = ln->next)
		if (value->use == ln->value->use)
			if (buffer_case_cmp(value, ln->value->buf))
				return true;

	return false;
}


/*
 * Trunk an initial buffer into several pieces upon a separator char 
 * Careful returned list must then be free with list_free()
 */
list *list_explode(char separator, const buffer * value)
{
	size_t i;
	list *l;
	buffer *buf;

	assert(value != NULL);

	l = list_init();
	buf = buffer_init();

	for (i = 0; i < value->use; i++)
		if (value->buf[i] == separator)
		{
			/* add the buffer to the list */
			list_add(l, buf);
			buf = buffer_init();
		}
		else
			buffer_add(buf, value->buf[i]);


	list_add(l, buf);

	return l;
}


/*
 * Trunk an initial buffer into several pieces upon two separators 
 * Careful returned list must then be free with list_free()
 */
list *list_explode_start_end(char separator_start, char separator_end,
   buffer * value)
{
	size_t i;

	list *l;
	buffer *buf;

	assert(value != NULL);

	l = list_init();

	/* if first char doesn't match separator, list contains only one element */
	if (value->buf[0] != separator_start)
		list_add_by_copy(l, value);
	else
	{
		buf = buffer_init();
		for (i = 1; i < value->use; i++)
			if (value->buf[i] == separator_end)
				/* add the buffer to the list */
				list_add(l, buf);
			else if (value->buf[i] != separator_start)
				buffer_add(buf, value->buf[i]);
			else
				buf = buffer_init();
	}


	return l;
}


/*
 * Trunk an initial string into several pieces upon a separator char 
 * Careful returned list must then be free with list_free()
 */
list *list_explode_str(char separator, const char *value)
{
	size_t i;
	list *l;
	buffer *buf;


	assert(value != NULL);

	l = list_init();
	buf = buffer_init();

	for (i = 0; value[i] != '\0'; i++)
		if (value[i] == separator)
		{
			/* add the buffer to the list */
			list_add(l, buf);
			buf = buffer_init();
		}
		else
			buffer_add(buf, value[i]);


	list_add(l, buf);

	return l;
}


#ifdef OWS_DEBUG
/*
 * Flush a list to a given file
 * (mainly to debug purpose)
 */
void list_flush(const list * l, FILE * output)
{
	list_node *ln;

	assert(l != NULL);
	assert(output != NULL);

	for (ln = l->first; ln != NULL; ln = ln->next)
	{
		fprintf(output, "[");
		buffer_flush(ln->value, output);
		fprintf(output, "]\n");
	}
}
#endif


/*
 * vim: expandtab sw=4 ts=4
 */

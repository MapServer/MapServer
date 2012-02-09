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
 * Initialize an array structure
 */
array *array_init()
{
	array *arr = NULL;

	arr = malloc(sizeof(array));
	assert(arr != NULL);

	arr->first = NULL;
	arr->last = NULL;

	return arr;
}


/*
 * Free an array structure
 */
void array_free(array * a)
{
	array_node *an = NULL;
	array_node *an_to_free = NULL;

	assert(a != NULL);

	for (an = a->first; an != NULL; /* empty */ )
	{
		an_to_free = an;
		an = an->next;

		buffer_free(an_to_free->key);
		buffer_free(an_to_free->value);
		free(an_to_free);
		an_to_free = NULL;
	}

	free(a);
	a = NULL;
}


/*
 * Add a given buffer to the end of an array
 * Carefull key and value are passed by reference
 * and will be released by array_free !
 */
void array_add(array * a, buffer * key, buffer * value)
{
	array_node *an;

	assert(a != NULL);
	assert(key != NULL);
	assert(value != NULL);

	an = malloc(sizeof(array_node));
	assert(an != NULL);

	an->key = key;
	an->value = value;

	if (a->first == NULL)
		a->first = an;
	else
		a->last->next = an;

	a->last = an;
	a->last->next = NULL;
}


/*
 * Check if a given key string is or not in the array
 */
bool array_is_key(const array * a, const char *key)
{
	array_node *an;
	size_t ks;

	assert(a != NULL);
	assert(key != NULL);

	for (ks = strlen(key), an = a->first; an != NULL; an = an->next)
		if (ks == an->key->use)
			if (buffer_case_cmp(an->key, key))
				return true;

	return false;
}


/*
 * Return a value buffer from an array (from key value)
 * You must be sure key is defined for this array, see is_key() above
 * Carreful return a reference on the array buf itself !
 */
buffer *array_get(const array * a, const char *key)
{
	array_node *an;
	size_t ks;

	assert(a != NULL);
	assert(key != NULL);

	for (ks = strlen(key), an = a->first; an != NULL; an = an->next)
		if (ks == an->key->use)
			if (buffer_case_cmp(an->key, key))
				break;

	assert(an != NULL);

	return an->value;
}


#ifdef OWS_DEBUG
/*
 * Flush an array to a given file
 * (mainly to debug purpose)
 */
void array_flush(const array * a, FILE * output)
{
	array_node *an;

	assert(a != NULL);
	assert(output != NULL);

	for (an = a->first; an != NULL; an = an->next)
	{
		fprintf(output, "[");
		buffer_flush(an->key, output);
		fprintf(output, "] -> ");
		buffer_flush(an->value, output);
		fprintf(output, "\n");
	}
}
#endif


/*
 * vim: expandtab sw=4 ts=4
 */

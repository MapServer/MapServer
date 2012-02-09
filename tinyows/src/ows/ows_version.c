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

#include "ows.h"


/*
 * Initialize version structure
 */
ows_version *ows_version_init()
{
	ows_version *v;

	v = malloc(sizeof(ows_version));
	assert(v != NULL);

	v->major = 0;
	v->minor = 0;
	v->release = 0;

	return v;
}


/*
 * Fill a version structure
 */
void ows_version_set(ows_version * v, int major, int minor, int release)
{
	assert(v != NULL);

	v->major = major;
	v->minor = minor;
	v->release = release;
}


/*
 * Free a version structure
 */
void ows_version_free(ows_version * v)
{
	assert(v != NULL);

	free(v);
	v = NULL;
}


/*
 * Return an int related to an ows_version
 */
int ows_version_get(ows_version * v)
{
	assert(v != NULL);

	return v->major * 100 + v->minor * 10 + v->release;
}


#ifdef OWS_DEBUG
void ows_version_flush(ows_version * v, FILE * output)
{
	assert(v != NULL);
	assert(output != NULL);

	fprintf(output, "version: [%i,%i,%i]\n", v->major, v->minor,
	   v->release);
}
#endif

/*
 * vim: expandtab sw=4 ts=4 
 */

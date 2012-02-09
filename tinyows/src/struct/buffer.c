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
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <regex.h>

#include "../ows/ows.h"


#ifndef SIZE_MAX
#define SIZE_MAX (~(size_t)0)
#endif

/*
 * Realloc buffer memory space
 * use an exponential grow size to realloc (less realloc call)
 */
static void buffer_realloc(buffer * buf)
{
	assert(buf != NULL);

	buf->buf = realloc(buf->buf, buf->realloc * sizeof(char));
	assert(buf->buf != NULL);

	buf->size = buf->realloc;

	buf->realloc *= 2;
	if (buf->realloc >= SIZE_MAX)
		assert(true);

}


/*
 * Initialize buffer structure 
 */
buffer *buffer_init()
{
	buffer *buf;

	buf = malloc(sizeof(buffer));
	assert(buf != NULL);

	buf->buf = malloc(BUFFER_SIZE_INIT * sizeof(char));
	assert(buf->buf != NULL);

	buf->size = BUFFER_SIZE_INIT;
	buf->realloc = BUFFER_SIZE_INIT * 2;
	buf->use = 0;
	buf->buf[0] = '\0';

	return buf;
}


/*
 * Free a buffer structure and all data in it
 */
void buffer_free(buffer * buf)
{
	assert(buf != NULL);

	assert(buf->buf != NULL);

	free(buf->buf);
	buf->buf = NULL;


	free(buf);
	buf = NULL;
}


/*
 * Empty data from a given buffer 
 * (don't release memory, use buffer_free instead)
 */
void buffer_empty(buffer * buf)
{
	assert(buf != NULL);

	if (buf->use > 0)
		buf->buf[0] = '\0';
	buf->use = 0;
}


/*
 * Flush all data from a buffer to a stream
 * Not used *only* to debug purpose ;)
 */
void buffer_flush(buffer * buf, FILE * output)
{
	size_t i;
	int ret;

	assert(buf != NULL);
	assert(output != NULL);

	for (i = 0; i < buf->use; i++)
	{
		ret = fputc((int) buf->buf[i], output);
		assert(ret != EOF);
	}
}


/*
 * Add a char to the bottom of a buffer
 */
void buffer_add(buffer * buf, char c)
{
	assert(buf != NULL);

	if ((buf->use + 2) >= buf->size)
		buffer_realloc(buf);

	buf->buf[buf->use] = c;
	buf->buf[buf->use + 1] = '\0';
	buf->use++;
}


/*
 * Return a buffer from a double
 */
buffer *buffer_ftoa(double f)
{
	buffer *res;
	buffer *mant;
	int real;

	/* FIXME What about scientific notation ? */
	if (f >= 0.0)
		real = (int) floor(f);
	else
		real = (int) ceil(f);

	res = buffer_itoa(real);
	mant = buffer_itoa((int) ((fabs(f) - fabs(real)) * pow(10.0, 6)));
	buffer_add(res, '.');
	buffer_copy(res, mant);

	buffer_free(mant);

	return res;
}


/*
 * Add a double to a given buffer
 */
void buffer_add_double(buffer * buf, double f)
{
	buffer *b;

	assert(buf != NULL);

	b = buffer_ftoa(f);
	buffer_copy(buf, b);
	buffer_free(b);
}


/*
 * Add an int to a given buffer
 */
void buffer_add_int(buffer * buf, int i)
{
	buffer *b;

	assert(buf != NULL);

	b = buffer_itoa(i);
	buffer_copy(buf, b);
	buffer_free(b);
}


/*
 * Convert an integer to a buffer (base 10 only)
 */
buffer *buffer_itoa(int i)
{
	bool minus;
	buffer *buf;

	buf = buffer_init();

	if (i < 0)
	{
		i = -i;
		minus = true;
	}
	else
	{
		minus = false;
	}

	if (i == 0)
	{
		buffer_add(buf, '0');
	}
	else
	{
		while (i > 0)
		{
			/* 48 mean '0' char in ASCII */
			buffer_add_head(buf, (char) (i % 10 + 48));
			i /= 10;
		}
		if (minus)
			buffer_add_head(buf, '-');
	}

	return buf;
}


/*
 * Add a char to the top of a buffer
 */
void buffer_add_head(buffer * buf, char c)
{
	size_t i;

	assert(buf != NULL);

	if ((buf->use + 2) >= buf->size)
		buffer_realloc(buf);

	if (buf->use > 0)
		for (i = buf->use; i > 0; i--)
			buf->buf[i] = buf->buf[i - 1];

	buf->buf[0] = c;
	buf->buf[buf->use + 1] = '\0';
	buf->use++;
}

/*
 * Add a string to the top of a buffer
 */
void buffer_add_head_str(buffer * buf, char *str)
{
	int i;

	assert(buf != NULL);
	assert(str != NULL);

	for (i = strlen(str); i != 0; i--)
		buffer_add_head(buf, str[i - 1]);

}


/*
 * Add a string to a buffer
 */
void buffer_add_str(buffer * buf, const char *str)
{
	assert(buf != NULL);
	assert(str != NULL);

	while (*str++ != '\0')
		buffer_add(buf, *(str - 1));
}


/*
 * Check if a buffer string is the same than another
 */
bool buffer_cmp(const buffer * buf, const char *str)
{
	size_t i;

	assert(buf != NULL);
	assert(str != NULL);

	if (buf->use != strlen(str))
		return false;

	for (i = 0; i < buf->use; i++)
		if (buf->buf[i] != str[i])
			return false;

	return true;
}


/*
 * Check if a buffer string is the same than anoter for a specified length 
 * (insensitive case check)
 */
bool buffer_case_cmp(const buffer * buf, const char *str)
{
	size_t i;

	assert(buf != NULL);
	assert(str != NULL);

	if (buf->use != strlen(str))
		return false;

	for (i = 0; i < buf->use; i++)
		if (toupper(buf->buf[i]) != toupper(str[i]))
			return false;

	return true;
}


/*
 * Copy data from a buffer to an another
 */
void buffer_copy(buffer * dest, const buffer * src)
{
	size_t i;

	assert(dest != NULL);
	assert(src != NULL);

	for (i = 0; i < src->use; i++)
		buffer_add(dest, src->buf[i]);
}


/*
 * Delete last N chars from a buffer
 */
void buffer_pop(buffer * buf, size_t len)
{
	assert(buf != NULL);
	assert(len <= buf->use);

	buf->use -= len;
	buf->buf[buf->use] = '\0';
}


/*
 * Delete first N chars from a buffer
 */
void buffer_shift(buffer * buf, size_t len)
{
	size_t i;

	assert(buf != NULL);
	assert(len <= buf->use);

	for (i = len; i < buf->use; i++)
		buf->buf[i - len] = buf->buf[i];

	buf->use -= len;
	buf->buf[buf->use] = '\0';
}


/*
 * Replace all occurences of string 'before' inside the buffer by 
 * string 'after' 
 */
buffer *buffer_replace(buffer * buf, char *before, char *after)
{
	char *pos;
	buffer *new_buf, *rest;
	int length;

	assert(buf != NULL);
	assert(before != NULL);
	assert(after != NULL);

	new_buf = buffer_init();

	buffer_copy(new_buf, buf);

	/* look for first occurence */
	pos = strstr(new_buf->buf, before);

	while (pos != NULL)
	{
		length = strlen(pos);

		/* copy the first party of the string without occurence */
		buffer_pop(new_buf, length);

		/* add the string after */
		buffer_add_str(new_buf, after);

		/* add the remaining string */
		rest = buffer_init();
		buffer_copy(rest, buf);
		buffer_shift(rest, buf->use - length + strlen(before));
		buffer_copy(new_buf, rest);
		buffer_free(rest);


		/* search the next occurence */
		pos = strstr(new_buf->buf, before);

	}

	/* return the altered buffer */
	buffer_empty(buf);
	buffer_copy(buf, new_buf);
	buffer_free(new_buf);

	return buf;

}


/*
 * vim: expandtab sw=4 ts=4
 */

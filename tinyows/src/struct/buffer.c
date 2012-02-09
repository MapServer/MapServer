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
    assert(buf);

    buf->buf = realloc(buf->buf, buf->realloc * sizeof(char));
    assert(buf->buf);

    buf->size = buf->realloc;
    buf->realloc *= 4;

    if (buf->realloc >= SIZE_MAX) assert(false);
}


/*
 * Initialize buffer structure
 */
buffer *buffer_init()
{
    buffer *buf;

    buf = malloc(sizeof(buffer));
    assert(buf);

    buf->buf = malloc(BUFFER_SIZE_INIT * sizeof(char));
    assert(buf->buf);

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
    assert(buf);
    assert(buf->buf);

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
    assert(buf);
    if (buf->use > 0) buf->buf[0] = '\0';
    buf->use = 0;
}


/*
 * Flush all data from a buffer to a stream
 * Not used *only* to debug purpose ;)
 */
void buffer_flush(buffer * buf, FILE * output)
{
    assert(buf);
    assert(output);
    fprintf(output, "%s", buf->buf);
}


/*
 * Add a char to the bottom of a buffer
 */
void buffer_add(buffer * buf, char c)
{
    assert(buf);

    if ((buf->use + 1) >= buf->size) buffer_realloc(buf);

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

    res = buffer_init();
    /* FIXME use snprintf instead ! */
    while (res->size < 100) buffer_realloc(res);
    sprintf(res->buf, "%f", f);
    res->use = strlen(res->buf);

    return res;
}


/*
 * Add a double to a given buffer
 */
void buffer_add_double(buffer * buf, double f)
{
    buffer *b;

    assert(buf);

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

    assert(buf);

    b = buffer_itoa(i);
    buffer_copy(buf, b);
    buffer_free(b);
}


/*
 * Convert an integer to a buffer (base 10 only)
 */
buffer *buffer_itoa(int i)
{
    buffer *res;

    res = buffer_init();
    /* FIXME use snprintf instead ! */
    while (res->size < 100) buffer_realloc(res);
    sprintf(res->buf, "%i", i);
    res->use = strlen(res->buf);

    return res;
}


buffer *buffer_from_str(const char *str)
{
    buffer *b;

    assert(str);
    b = buffer_init();

    buffer_add_str(b, str);
    return b;
}


/*
 * Add a char to the top of a buffer
 */
void buffer_add_head(buffer * buf, char c)
{
    size_t i;

    assert(buf);

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

    assert(buf);
    assert(str);

    for (i = strlen(str); i != 0; i--)
        buffer_add_head(buf, str[i - 1]);
}


/*
 * Add a string to a buffer
 */
void buffer_add_str(buffer * buf, const char *str)
{
    assert(buf);
    assert(str);

    while (*str++) buffer_add(buf, *(str - 1));
}


/*
 * Check if a buffer string is the same than another
 */
bool buffer_cmp(const buffer * buf, const char *str)
{
    size_t i;

    assert(buf);
    assert(str);

    if (buf->use != strlen(str)) return false;

    for (i = 0; i < buf->use; i++)
        if (buf->buf[i] != str[i])
            return false;

    return true;
}


/*
 * Check if a buffer string is the same than another, on the n first char
 */
bool buffer_ncmp(const buffer * buf, const char *str, size_t n)
{
    size_t i;

    assert(buf);
    assert(str);

    if (buf->use < n) return false;

    for (i = 0 ; i < n ; i++)
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

    assert(buf);
    assert(str);

    if (buf->use != strlen(str)) return false;

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

    assert(dest);
    assert(src);

    for (i = 0; i < src->use; i++)
        buffer_add(dest, src->buf[i]);
}


/*
 * Delete last N chars from a buffer
 */
void buffer_pop(buffer * buf, size_t len)
{
    assert(buf);
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

    assert(buf);
    assert(len <= buf->use);

    if (len <= 0) return; /* nothing to do */

    for (i = len; i < buf->use; i++)
        buf->buf[i - len] = buf->buf[i];

    buf->use -= len;
    buf->buf[buf->use] = '\0';
}


/*
 * Replace all occurences of string 'before' inside the buffer by string 'after'
 */
buffer *buffer_replace(buffer * buf, char *before, char *after)
{
    buffer *new_buf, *rest;
    size_t length;
    char *pos;

    assert(before);
    assert(after);
    assert(buf);

    if (!strcmp(before, after)) return buf; /* To prevent infinite loop */

    new_buf = buffer_init();
    buffer_copy(new_buf, buf);

    pos = strstr(new_buf->buf, before); /* Look for first occurence */
    while (pos) {
        length = strlen(pos);
        
        buffer_pop(new_buf, length);    /* Copy the first part without occurence */
        buffer_add_str(new_buf, after); /* Add the string after */

        /* Add the remaining string */
        rest = buffer_init();
        buffer_copy(rest, buf);
        buffer_shift(rest, buf->use - length + strlen(before));
        buffer_copy(new_buf, rest);
        buffer_free(rest);
        
        pos = strstr(new_buf->buf, before); /* Search the next occurence */
    }

    buffer_empty(buf);
    buffer_copy(buf, new_buf);
    buffer_free(new_buf);

    return buf;
}


/*
 * Modify string to replace encoded characters by their true value
 * Function originaly written by Assefa
 *
 * The replacements performed are:
 *  &	-> &amp;
 *  "	-> &quot;
 *  <	-> &lt;
 *  >	-> &gt;
 */
buffer *buffer_encode_xml_entities_str(const char * str)
{
    buffer *buf;

    assert(str);
    buf = buffer_init();

    for( /* empty */ ; *str ; str++) {
    	switch(*str) {
	        case '&':
 	        buffer_add_str(buf, "&amp;");
       	        break;

            case '<':
                buffer_add_str(buf, "&lt;");
                break;

            case '>':
                buffer_add_str(buf, "&gt;");
                break;

            case '"':
                buffer_add_str(buf, "&quot;");
                break;

            case '\'':
                buffer_add_str(buf, "&#39;");
                break;

            default:
		buffer_add(buf, *str);
	    }
    }

    return buf;
}


/*
 * Modify string to replace encoded characters by their true value
 * for JSON output
 *
 * The replacements performed are:
 *  " -> \"
 */
buffer *buffer_encode_json_str(const char * str)
{
    buffer *buf;

    assert(str);
    buf = buffer_init();

    for( /* empty */ ; *str ; str++) {
        switch(*str) {
            case '"':
                    buffer_add_str(buf, "\\\"");
                break;

            default:
                    buffer_add(buf, *str);
            }
    }

    return buf;
}


/*
 * vim: expandtab sw=4 ts=4
 */

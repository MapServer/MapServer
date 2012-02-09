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
#include <stdio.h>              /* FILE */
#include <string.h>             /* strncmp */
#include <limits.h>
#include <assert.h>

#include "../ows/ows.h"


/*
 * Initialize a multiple list structure
 */
mlist *mlist_init()
{
    mlist *ml = NULL;

    ml = malloc(sizeof(mlist));
    assert(ml);

    ml->first = NULL;
    ml->last = NULL;
    ml->size = 0;

    return ml;
}


/*
 * Free a multiple list structure
 */
void mlist_free(mlist * ml)
{
    assert(ml);

    while (ml->first) mlist_node_free(ml, ml->first);

    ml->last = NULL;
    free(ml);
    ml = NULL;
}


/*
 * Add a given list to the end of a mlist
 * Careful list is passed by reference,
 * and must be free with mlist_free()
 */
void mlist_add(mlist * ml, list * value)
{
    mlist_node *mln;

    assert(ml);
    assert(value);
    assert(ml->size < UINT_MAX);

    mln = mlist_node_init();

    mln->value = value;

    if (!ml->first) {
        mln->prev = NULL;
        ml->first = mln;
    } else {
        mln->prev = ml->last;
        ml->last->next = mln;
    }

    ml->last = mln;
    ml->last->next = NULL;
    ml->size++;
}


/*
 * Initialize a multiple list node
 */
mlist_node *mlist_node_init()
{
    mlist_node *mln;

    mln = malloc(sizeof(mlist_node));
    assert(mln);

    mln->value = NULL;
    mln->prev = NULL;
    mln->next = NULL;

    return mln;
}


/*
 * Free a multiple list node
 */
void mlist_node_free(mlist * ml, mlist_node * mln)
{
    assert(mln);
    assert(ml);

    if (mln->prev)
        mln->prev = NULL;

    if (mln->next) {
        ml->first = mln->next;
        mln->next = NULL;
    } else
        ml->first = NULL;

    if (mln->value) list_free(mln->value);

    free(mln);
    mln = NULL;
}


/*
 * Trunk an initial buffer into several pieces upon two separators
 * Inside multiple list, each element is separated by a comma
 * Careful returned multiple list must then be free with mlist_free()
 */
mlist *mlist_explode(char separator_start, char separator_end, buffer * value)
{
    size_t i;
    mlist *ml;
    list *l;
    buffer *buf;

    assert(value);

    ml = mlist_init();

    /* if first char doesn't match separator, mlist contains only one element */
    if (value->buf[0] != separator_start) {
        /* according to wfs specification, elements inside a multiple list
           are separated by a comma */
        l = list_explode(',', value);
        mlist_add(ml, l);
    } else {
        buf = buffer_init();

        for (i = 1; i < value->use; i++)
            if (value->buf[i] == separator_end) {
                /* explode the mlist's element */
                l = list_explode(',', buf);
                /* add the list to the multiple list */
                mlist_add(ml, l);
                buffer_free(buf);
            } else if (value->buf[i] != separator_start) {
                buffer_add(buf, value->buf[i]);
            }
        /* separator start */
            else
                buf = buffer_init();

    }

    return ml;
}


#ifdef OWS_DEBUG
/*
 * Flush a mlist to a given file
 * (mainly to debug purpose)
 */
void mlist_flush(const mlist * ml, FILE * output)
{
    mlist_node *mln;

    assert(ml);
    assert(output);

    for (mln = ml->first ; mln ; mln = mln->next) {
        fprintf(output, "(\n");
        list_flush(mln->value, output);
        fprintf(output, ")\n");
    }
}
#endif


/*
 * vim: expandtab sw=4 ts=4
 */

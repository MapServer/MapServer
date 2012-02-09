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
 * Alist is an array of list
 */

/*
 * Initialize an alist structure
 */
alist *alist_init()
{
    alist *al = NULL;

    al = malloc(sizeof(alist));
    assert(al);

    al->first = NULL;
    al->last = NULL;

    return al;
}


/*
 * Free an alist structure
 */
void alist_free(alist * al)
{
    alist_node *an = NULL;
    alist_node *an_to_free = NULL;

    assert(al);

    for (an = al->first ; an ; /* empty */) {
        an_to_free = an;
        an = an->next;

        buffer_free(an_to_free->key);
        list_free(an_to_free->value);
        free(an_to_free);
        an_to_free = NULL;
    }

    free(al);
    al = NULL;
}


/*
 * Add a given buffer to the end of an alist entry
 * if key exist, value is add to the list
 * if not new key entry is created
 */
void alist_add(alist * al, buffer * key, buffer * value)
{
    alist_node *an;

    assert(al);
    assert(key);
    assert(value);

    if (!alist_is_key(al, key->buf)) {
        an = malloc(sizeof(alist_node));
        assert(an);

        an->key = key;
        an->value = list_init();

        if (!al->first) al->first = an;
        else            al->last->next = an;

        al->last = an;
        al->last->next = NULL;
    } else {
        for (an = al->first ; an ; an = an->next)
            if (buffer_case_cmp(an->key, key->buf))
                break;
    }

    list_add(an->value, value);
}


/*
 * Check if a given key string is or not in the alist
 */
bool alist_is_key(const alist * al, const char *key)
{
    alist_node *an;
    size_t ks;

    assert(al);
    assert(key);

    for (ks = strlen(key), an = al->first ; an ; an = an->next)
        if (ks == an->key->use)
            if (buffer_case_cmp(an->key, key))
                return true;

    return false;
}


/*
 * Return a value buffer from an alist (from key value)
 * You must be sure key is defined for this array, see is_key() above
 * Carreful return a reference on the alist buf itself !
 */
list *alist_get(const alist * al, const char *key)
{
    alist_node *an;
    size_t ks;

    assert(al);
    assert(key);

    for (ks = strlen(key), an = al->first ; an ; an = an->next) {
        if (ks == an->key->use)
            if (buffer_case_cmp(an->key, key))
                break;
    }

    assert(an);

    return an->value;
}


#ifdef OWS_DEBUG
/*
 * Flush an alist to a given file
 * (mainly to debug purpose)
 */
void alist_flush(const alist * al, FILE * output)
{
    alist_node *an;

    assert(al);
    assert(output);

    for (an = al->first ; an ; an = an->next) {
        fprintf(output, "[");
        buffer_flush(an->key, output);
        fprintf(output, "] -> ");
        list_flush(an->value, output);
        fprintf(output, "\n");
    }
}
#endif


/*
 * vim: expandtab sw=4 ts=4
 */

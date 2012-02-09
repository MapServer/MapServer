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
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include "ows.h"


/*
 * Initialize an aws layer structure
 */
ows_layer_list *ows_layer_list_init()
{
    ows_layer_list *ll;

    ll = malloc(sizeof(ows_layer_list));
    assert(ll);

    ll->first = NULL;
    ll->last = NULL;
    return ll;
}


/*
 * Free an ows layer structure
 */
void ows_layer_list_free(ows_layer_list * ll)
{
    assert(ll);

    while (ll->first) ows_layer_node_free(ll, ll->first);
    ll->last = NULL;
    free(ll);
    ll = NULL;
}


/*
 * Retrieve a Layer from a layer list or NULL if not found
 */
ows_layer * ows_layer_get(const ows_layer_list * ll, const buffer * name)
{
    ows_layer_node *ln;

    assert(ll);
    assert(name);

    for (ln = ll->first; ln ; ln = ln->next)
        if (ln->layer->name && !strcmp(ln->layer->name->buf, name->buf))
            return ln->layer;

    return (ows_layer *) NULL;
}


/*
 * Check if a layer matchs an existing table in PostGIS
 */
bool ows_layer_match_table(const ows * o, const buffer * name)
{
    ows_layer_node *ln;

    assert(o);
    assert(name);

    for (ln = o->layers->first; ln ; ln = ln->next)
        if (ln->layer->name && !strcmp(ln->layer->name->buf, name->buf)) {
	    if (ln->layer->storage) return true;
            else return false;
	}

    return false;
}


/*
 * Retrieve a list of layer name who have a storage object
 * (concrete layer so)
 */
list * ows_layer_list_having_storage(const ows_layer_list * ll)
{
    ows_layer_node *ln;
    list *l;

    assert(ll);

    l = list_init();

    for (ln = ll->first; ln ; ln = ln->next)
        if (ln->layer->storage)
            list_add_by_copy(l, ln->layer->name);

    return l;
}


/*
 * Check if all layers are retrievable (defined in configuration file)
 */
bool ows_layer_list_retrievable(const ows_layer_list * ll)
{
    ows_layer_node *ln;

    assert(ll);

    for (ln = ll->first; ln ; ln = ln->next)
        if (!ln->layer->retrievable) return false;

    return true;
}


/*
 * Check if a layer is retrievable (defined in configuration file)
 */
bool ows_layer_retrievable(const ows_layer_list * ll, const buffer * name)
{
    ows_layer_node *ln;

    assert(ll);
    assert(name);

    for (ln = ll->first; ln ; ln = ln->next)
        if (ln->layer->name && !strcmp(ln->layer->name->buf, name->buf))
            return ln->layer->retrievable;

    return false;
}


/*
 * Check if all layers are writable (defined in file conf)
 */
bool ows_layer_list_writable(const ows_layer_list * ll)
{
    ows_layer_node *ln;

    assert(ll);

    for (ln = ll->first; ln ; ln = ln->next)
        if (!ln->layer->writable) return false;

    return true;
}


/*
 * Check if a layer is writable (defined in file conf)
 */
bool ows_layer_writable(const ows_layer_list * ll, const buffer * name)
{
    ows_layer_node *ln;

    assert(ll);
    assert(name);

    for (ln = ll->first; ln ; ln = ln->next)
        if (ln->layer->name && !strcmp(ln->layer->name->buf, name->buf))
            return ln->layer->writable;

    return false;
}


/*
 * Check if a layer is in a layer's list
 */
bool ows_layer_in_list(const ows_layer_list * ll, buffer * name)
{
    ows_layer_node *ln;

    assert(ll);
    assert(name);

    for (ln = ll->first; ln ; ln = ln->next)
        if (ln->layer->name && !strcmp(ln->layer->name->buf, name->buf))
            return true;

    return false;
}


/*
 * Check if a layer's list is in a other layer's list
 */
bool ows_layer_list_in_list(const ows_layer_list * ll, const list * l)
{
    list_node *ln;

    assert(ll);
    assert(l);

    for (ln = l->first; ln ; ln = ln->next)
        if (!ows_layer_in_list(ll, ln->value)) return false;

    return true;
}


/*
 * Return an array of prefixes and associated NS URI of a layer's list
 */
array *ows_layer_list_namespaces(ows_layer_list * ll)
{
    buffer *ns_prefix, *ns_uri;
    ows_layer_node *ln;
    array *namespaces = array_init();

    assert(ll);

    for (ln = ll->first; ln ; ln = ln->next) {
        if (    ln->layer->ns_prefix && ln->layer->ns_prefix->use 
             && !array_is_key(namespaces, ln->layer->ns_prefix->buf)) {

            ns_prefix = buffer_init();
            ns_uri = buffer_init();
            buffer_copy(ns_prefix, ln->layer->ns_prefix);
            buffer_copy(ns_uri, ln->layer->ns_uri);
            array_add(namespaces, ns_prefix, ns_uri);
        }
    }

    return namespaces;
}


/*
 * Return a list of layer names grouped by prefix
 */
list *ows_layer_list_by_ns_prefix(ows_layer_list * ll, list * layer_name, buffer * ns_prefix)
{
    list *typ;
    list_node *ln;
    buffer *layer_ns_prefix;

    assert(ll);
    assert(layer_name);
    assert(ns_prefix);

    typ = list_init();

    for (ln = layer_name->first; ln ; ln = ln->next) {
        layer_ns_prefix = ows_layer_ns_prefix(ll, ln->value);

        if (buffer_cmp(layer_ns_prefix, ns_prefix->buf))
            list_add_by_copy(typ, ln->value);
    }

    return typ;
}


/*
 * Retrieve a list of prefix used for a specified list of layers
 */
list *ows_layer_list_ns_prefix(ows_layer_list * ll, list * layer_name)
{
    list_node *ln;
    list *ml_ns_prefix;
    buffer *ns_prefix;

    assert(ll);
    assert(layer_name);

    ml_ns_prefix = list_init();

    for (ln = layer_name->first; ln ; ln = ln->next) {
        ns_prefix = ows_layer_ns_prefix(ll, ln->value);

        if (!in_list(ml_ns_prefix, ns_prefix))
            list_add_by_copy(ml_ns_prefix, ns_prefix);
    }

    return ml_ns_prefix;
}


/*
 * Retrieve the prefix linked to the specified layer
 */
buffer *ows_layer_ns_prefix(ows_layer_list * ll, buffer * layer_name)
{
    ows_layer_node *ln;

    assert(ll);
    assert(layer_name);

    for (ln = ll->first; ln ; ln = ln->next)
        if (buffer_cmp(ln->layer->name, layer_name->buf))
            return ln->layer->ns_prefix;

    return (buffer *) NULL;
}


/*
 * Retrieve the ns_uri associated to the specified ns_prefix
 */
buffer *ows_layer_ns_uri(ows_layer_list * ll, buffer * ns_prefix)
{
    ows_layer_node *ln;

    assert(ll);
    assert(ns_prefix);

    for (ln = ll->first; ln ; ln = ln->next)
        if (buffer_cmp(ln->layer->ns_prefix, ns_prefix->buf))
            return ln->layer->ns_uri;

    return (buffer *) NULL;
}


/*
 * Add a layer to a layer's list
 */
void ows_layer_list_add(ows_layer_list * ll, ows_layer * l)
{
    ows_layer_node *ln = ows_layer_node_init();

    assert(ll);
    assert(l);

    ln->layer = l;
    if (!ll->first) {
        ln->prev = NULL;
        ll->first = ln;
    } else {
        ln->prev = ll->last;
        ll->last->next = ln;
    }
    ll->last = ln;
    ll->last->next = NULL;
}


/*
 * Initialize a layer node
 */
ows_layer_node *ows_layer_node_init()
{
    ows_layer_node *ln = malloc(sizeof(ows_layer_node));
    assert(ln);

    ln->prev = NULL;
    ln->next = NULL;
    ln->layer = NULL;

    return ln;
}


/*
 * Free a layer node
 */
void ows_layer_node_free(ows_layer_list * ll, ows_layer_node * ln)
{
    assert(ln);

    if (ln->prev) ln->prev = NULL;
    if (ln->next)
    {
        if (ll) ll->first = ln->next;
        ln->next = NULL;
    } else if (ll) ll->first = NULL;

    if (ln->layer) ows_layer_free(ln->layer);

    free(ln);
    ln = NULL;
}


/*
 * Flush a layer's list to a given file
 * (mainly to debug purpose)
 */
#ifdef OWS_DEBUG
void ows_layer_list_flush(ows_layer_list * ll, FILE * output)
{
    ows_layer_node *ln = NULL;
    assert(ll);
    assert(output);

    for (ln = ll->first; ln ; ln = ln->next) {
        ows_layer_flush(ln->layer, output);
        fprintf(output, "--------------------\n");
    }
}
#endif


/*
 * Initialize a layer
 */
ows_layer *ows_layer_init()
{
    ows_layer *l = malloc(sizeof(ows_layer));
    assert(l);

    l->parent = NULL;
    l->depth = 0;
    l->title = NULL;
    l->name = NULL;
    l->abstract = NULL;
    l->keywords = NULL;
    l->gml_ns = NULL;
    l->retrievable = false;
    l->writable = false;
    l->srid = NULL;
    l->geobbox = NULL;
    l->ns_prefix = buffer_init();
    l->ns_uri = buffer_init();
    l->storage = ows_layer_storage_init();

    return l;
}


/*
 * Free a layer
 */
void ows_layer_free(ows_layer * l)
{
    assert(l);

    if (l->title) 	buffer_free(l->title);
    if (l->name) 	buffer_free(l->name);
    if (l->abstract) 	buffer_free(l->abstract);
    if (l->keywords) 	list_free(l->keywords);
    if (l->gml_ns) 	list_free(l->gml_ns);
    if (l->srid)	list_free(l->srid);
    if (l->geobbox)	ows_geobbox_free(l->geobbox);
    if (l->ns_uri)	buffer_free(l->ns_uri);
    if (l->ns_prefix)	buffer_free(l->ns_prefix);
    if (l->storage)	ows_layer_storage_free(l->storage);

    free(l);
    l = NULL;
}


/*
 * Flush a layer to a given file
 * (mainly to debug purpose)
 */
#ifdef OWS_DEBUG
void ows_layer_flush(ows_layer * l, FILE * output)
{
    assert(l);
    assert(output);

    fprintf(output, "depth: %i\n", l->depth);

    if (l->parent) {
             if (l->parent->name)  fprintf(output, "parent: %s\n", l->parent->name->buf);
        else if (l->parent->title) fprintf(output, "parent: %s\n", l->parent->title->buf);
    }

    fprintf(output, "retrievable: %i\n", l->retrievable?1:0);
    fprintf(output, "writable: %i\n", l->writable?1:0);

    if (l->title) {
        fprintf(output, "title: ");
        buffer_flush(l->title, output);
        fprintf(output, "\n");
    }

    if (l->name) {
        fprintf(output, "name: ");
        buffer_flush(l->name, output);
        fprintf(output, "\n");
    }

    if (l->srid) {
        fprintf(output, "srid: ");
        list_flush(l->srid, output);
        fprintf(output, "\n");
    }

    if (l->keywords) {
        fprintf(output, "keyword: ");
        list_flush(l->keywords, output);
        fprintf(output, "\n");
    }

    if (l->gml_ns) {
        fprintf(output, "gml_ns: ");
        list_flush(l->gml_ns, output);
        fprintf(output, "\n");
    }

    if (l->geobbox) {
        fprintf(output, "geobbox: ");
        ows_geobbox_flush(l->geobbox, output);
        fprintf(output, "\n");
    }

    if (l->ns_prefix) {
        fprintf(output, "ns_prefix: ");
        buffer_flush(l->ns_prefix, output);
        fprintf(output, "\n");
    }

    if (l->ns_uri) {
        fprintf(output, "ns_uri: ");
        buffer_flush(l->ns_uri, output);
        fprintf(output, "\n");
    }

    if (l->storage) {
        fprintf(output, "storage: ");
        ows_layer_storage_flush(l->storage, output);
        fprintf(output, "\n");
    }
}
#endif

/*
 * vim: expandtab sw=4 ts=4
 */

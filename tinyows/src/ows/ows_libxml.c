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
#include <string.h>

#include "ows.h"


/*
 * Same prefix but not same URI 
 */
static bool ows_libxml_check_namespace_node(ows *o, xmlNodePtr n, xmlNsPtr *ns_doc) 
{
    xmlNsPtr *ns_node, *pn, *pd;
    bool ret= true;

    assert(o);
    assert(n);
    assert(ns_doc);

    ns_node = xmlGetNsList(n->doc, n);
    if (!ns_node) return ret;

    for (pn = ns_node ; *(pn) ; pn++) {
       for (pd = ns_doc ; *(pd) ; pd++) {

            if (    (*pn)->prefix && (*pd)->prefix && (*pn)->href && (*pd)->href
                 && !strcmp((char *) (*pn)->prefix, (char *) (*pd)->prefix)
                 &&  strcmp((char *) (*pn)->href,   (char *) (*pd)->href  )) {
                 ret = false;
                 break;
            }
        }
    }

    xmlFree(ns_node);
    return ret;
}


/*
 *  TODO
 */
bool ows_libxml_check_namespace(ows *o, xmlNodePtr n)
{
    xmlNsPtr *ns_doc;
    xmlNodePtr node;
    bool ret = true;

    assert(o);
    assert(n);

    ns_doc  = xmlGetNsList(n->doc, xmlDocGetRootElement(n->doc));
    if (!ns_doc) return false;

    for (node = n ; node ; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) continue;
        if (node->children && !(ret = ows_libxml_check_namespace(o, node->children))) break;
        if (!(ret = ows_libxml_check_namespace_node(o, node, ns_doc))) break;
    }
   
    xmlFree(ns_doc);
    return ret;
}


/*
 * vim: expandtab sw=4 ts=4
 */

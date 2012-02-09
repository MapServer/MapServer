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
#include <ctype.h>

#include "../ows/ows.h"


/*
 * Max query string length send via QUERY_STRING CGI
 */
#define CGI_QUERY_MAX 32768


/*
 * Return true if this cgi call was using a GET request, false otherwise
 */
bool cgi_method_get()
{
    char *method;

    method = getenv("REQUEST_METHOD");
    if (method && !strcmp(method, "GET")) return true;
    return false;
}


/*
 * Return true if this cgi call was using a POST request, false otherwise
 */
bool cgi_method_post()
{
    char *method;

    method = getenv("REQUEST_METHOD");
    if (method && !strcmp(method, "POST")) return true;
    return false;
}


/*
 * Return the string sent by CGI
 */
char *cgi_getback_query(ows * o)
{
    char *query;
    int query_size = 0;
    size_t result;

    if (cgi_method_get()) query = getenv("QUERY_STRING");
    else if (cgi_method_post()) {
        query_size = atoi(getenv("CONTENT_LENGTH"));

        query = malloc(sizeof(char) * query_size + 1);
        assert(query); /* FIXME Really ? */
        result = fread(query, query_size, 1, stdin);
        if (ferror(stdin)) {
            ows_error(o, OWS_ERROR_REQUEST_HTTP, "Error on QUERY input", "request");
            return NULL;
	}
        query[query_size] = '\0';
    }
    /* local tests */
    else query = getenv("QUERY_STRING");

    return query;
}


/*
 * Transform an hexadecimal string into ASCII string
 * Source : http://hoohoo.ncsa.uiuc.edu/docs/
 */
static char cgi_hexatochar(char *what)
{
    char digit;

    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));
    return (digit);
}


/*
 * Unescape an url
 * Source : http://hoohoo.ncsa.uiuc.edu/docs/
 */
static void cgi_unescape_url(char *url)
{
    int x, y;

    for (x = 0, y = 0; url[y]; ++x, ++y) {
        if ((url[x] = url[y]) == '%') {
            url[x] = cgi_hexatochar(&url[y + 1]);
            y += 2;
        }
    }

    url[x] = '\0';
}


/*
 * Transform url's plus into spaces
 */
static void cgi_plustospace(char *str)
{
    int x;

    for (x=0 ; str[x] ; x++)
        if (str[x] == '+') str[x] = ' ';
}

/*
 * Remove CR or LF in URL 
 */
static void cgi_remove_crlf(char *str)
{
    int x;

    for (x=0 ; str[x] ; x++)
        if (str[x] == '\n' || str[x] == '\r')
		str[x] = ' ';
}

/*
 * Parse char by char QUERY_STRING request and return an array key/value
 * (key are all lowercase)
 */
array *cgi_parse_kvp(ows * o, char *query)
{
    int i;
    bool in_key;
    buffer *key;
    buffer *val;
    array *arr;
    char string[2];

    assert(o);
    assert(query);

    key = buffer_init();
    val = buffer_init();
    arr = array_init();
    in_key = true;

    cgi_unescape_url(query);
    cgi_remove_crlf(query);
    cgi_plustospace(query);

    for (i = 0; i < CGI_QUERY_MAX && query[i] ; i++) {

        if (query[i] == '&') {

            in_key = true;

            array_add(arr, key, val);
            key = buffer_init();
            val = buffer_init();

        } else if (query[i] == '=') {
            /* char '=' inside filter key mustn't be taken into account */
            if ((!buffer_case_cmp(key, "filter") || !buffer_case_cmp(key, "outputformat")) && buffer_cmp(val, ""))
                in_key = false;
            else buffer_add(val, query[i]);
        }
        /* Check characters'CGI request */
        else {
            /* to check the regular expression, argument must be a string, not a char */
            string[0] = query[i];
            string[1] = '\0';

            if (in_key) {

                /* if word is key, only letters are allowed */
                if (check_regexp(string, "[A-Za-zà-ÿ]"))
                    buffer_add(key, tolower(query[i]));
                else {
                    buffer_free(key);
                    buffer_free(val);
                    array_free(arr);
                    ows_error(o, OWS_ERROR_MISSING_PARAMETER_VALUE,
                              "QUERY_STRING contains forbidden characters", "request");
                    return NULL;
                }
            } else {
                /* if word is filter key, more characters are allowed */
                if (                                 check_regexp(string, "[A-Za-zà-ÿ0-9.\\=;,():/\\*_ \\-]") 
                    || (buffer_cmp(key, "filter") && check_regexp(string, "[A-Za-zà-ÿ0-9.#\\,():/_<> %\"\'=\\*!\\-]|\\[|\\]")))
                    buffer_add(val, query[i]);
                else {
                    buffer_free(key);
                    buffer_free(val);
                    array_free(arr);
                    ows_error(o, OWS_ERROR_MISSING_PARAMETER_VALUE,
                              "QUERY_STRING contains forbidden characters", "request");
                    return NULL;
                }
            }
        }
    }

    if (i == CGI_QUERY_MAX) {
        buffer_free(key);
        buffer_free(val);
        array_free(arr);
        ows_error(o, OWS_ERROR_REQUEST_HTTP, "QUERY_STRING too long", "request");
        return NULL;
    }

    array_add(arr, key, val);

    return arr;
}


/*
 * Add to the array : node name and content element
 */
static array *cgi_add_node(array * arr, xmlNodePtr n)
{
    buffer *key, *val;
    xmlChar *content;

    assert(arr);
    assert(n);

    key = buffer_init();
    val = buffer_init();

    buffer_add_str(key, (char *) n->name);
    content = xmlNodeGetContent(n);
    buffer_add_str(val, (char *) content);
    xmlFree(content);

    array_add(arr, key, val);

    return arr;
}


/*
 * Add to the array : attribute name and content attribute
 */
static array *cgi_add_att(array * arr, xmlAttr * att)
{
    buffer *key, *val;
    xmlChar *content;

    assert(arr);
    assert(att);

    key = buffer_init();
    val = buffer_init();

    buffer_add_str(key, (char *) att->name);
    content = xmlNodeGetContent(att->children);
    buffer_add_str(val, (char *) content);
    xmlFree(content);

    array_add(arr, key, val);

    return arr;
}


/*
 * Add to the array the sortby element
 */
static array *cgi_add_sortby(array * arr, xmlNodePtr n)
{
    buffer *key, *val;
    xmlNodePtr elemt, node;
    xmlChar *content;

    assert(arr);
    assert(n);

    content = NULL;
    key = buffer_init();
    buffer_add_str(key, (char *) n->name);
    val = buffer_init();

    elemt = n->children;

    if (elemt->type != XML_ELEMENT_NODE) elemt = elemt->next;

    /* parse the properties to sort */
    for ( /* empty */ ; elemt ; elemt = elemt->next) {
        if (elemt->type == XML_ELEMENT_NODE) {
            node = elemt->children;
            if (node->type != XML_ELEMENT_NODE) node = node->next;

            /* add the property name */
            content = xmlNodeGetContent(node);
            buffer_add_str(val, (char *) content);
            xmlFree(content);

            buffer_add_str(val, " ");

            node = node->next;
            if (node->type != XML_ELEMENT_NODE) node = node->next;

            /* add the order */
            content = xmlNodeGetContent(node);
            buffer_add_str(val, (char *) content);
            xmlFree(content);

        }

        if (elemt->next && elemt->next->type == XML_ELEMENT_NODE)
		buffer_add_str(val, ",");
    }

    array_add(arr, key, val);

    return arr;
}


/*
 * Add to the array a buffer
 */
static array *cgi_add_buffer(array * arr, buffer * b, char *name)
{
    buffer *key, *val;

    assert(arr);
    assert(b);
    assert(name);

    key = buffer_init();
    buffer_add_str(key, name);

    /* if there is only one element in brackets, brackets are useless so delete them */
    if (check_regexp(b->buf, "^\\([^\\)]*\\)$") || check_regexp(b->buf, "^\\(.*position\\(\\).*\\)$")) {
        buffer_shift(b, 1);
        buffer_pop(b, 1);
    }

    val = buffer_init();
    buffer_copy(val, b);
    array_add(arr, key, val);

    return arr;
}


/*
 * Add element into the buffer
 */
static buffer *cgi_add_into_buffer(buffer * b, xmlNodePtr n, bool need_comma)
{
    xmlChar *content;

    assert(b);
    assert(n);

    if (need_comma) buffer_add_str(b, ",");

    content = xmlNodeGetContent(n);
    buffer_add_str(b, (char *) content);
    xmlFree(content);

    return b;
}


/*
 * Add the whole xml element into the buffer
 */
buffer *cgi_add_xml_into_buffer(buffer * element, xmlNodePtr n)
{
    xmlBufferPtr buf;
    xmlNsPtr * ns;
    int i;

    assert(element);
    assert(n);

    ns = xmlGetNsList(n->doc, n);

    for (i = 0 ; ns[i] ; i++)
        xmlNewNs(n, ns[i]->href, ns[i]->prefix);

    buf = xmlBufferCreate();
    xmlNodeDump(buf, n->doc, n, 0, 0);
    buffer_add_str(element, (char *) buf->content);

    xmlBufferFree(buf);
    xmlFree(ns);

    return element;
}


static bool is_node_ns_wfs(xmlNodePtr n) 
{
    if (n->ns && n->ns->href && (!strcmp("http://www.opengis.net/wfs", (char *) n->ns->href) 
                              || !strcmp("http://www.opengis.net/ogc", (char *) n->ns->href))) return true;
    return false;
}

/*
 * Parse the XML request and return an array key/value
 */
array *cgi_parse_xml(ows * o, char *query)
{
    buffer *key, *val, *operations, *prop, *filter, *typename;
    bool prop_need_comma, typ_need_comma;
    xmlDocPtr xmldoc;
    xmlAttr *att;
    array *arr;
    bool lock_error, unknown_error;
    xmlNodePtr node, n = NULL;

    assert(o);
    assert(query);

    prop_need_comma = typ_need_comma = false;
    lock_error = unknown_error = false;

    xmldoc = xmlParseMemory(query, strlen(query));

    if (!xmldoc || !(n = xmlDocGetRootElement(xmldoc))) {
        xmlFreeDoc(xmldoc);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "xml isn't valid", "request");
        return NULL;
    }

    arr = array_init();

    operations = buffer_init();
    prop = buffer_init();;
    filter = buffer_init();
    typename = buffer_init();

    /* First child processed aside because name node match value array instead of key array */
    key = buffer_from_str("request");
    val = buffer_from_str((char *) n->name);
    array_add(arr, key, val);

    /* Retrieve the namespace linked to the first node */
    if (n->ns && n->ns->href) {
        key = buffer_from_str("xmlns");
        val = buffer_from_str((char *) n->ns->href);
        array_add(arr, key, val);
    }

    /* Name element is put in key, and content element in value */
    for (att = n->properties ; att ; att = att->next)
        arr = cgi_add_att(arr, att);

    for (n = n->children; n; n = n->next) {
        if (n->type != XML_ELEMENT_NODE) continue; /* Eat spaces */
        if (!is_node_ns_wfs(n)) continue;          /* NS check */

        for (att = n->properties ; att ; att = att->next) {

            /* Add typename to the matching global buffer */
            if (!strcmp((char *) att->name, "typeName")) {
                typename = cgi_add_into_buffer(typename, att->children, typ_need_comma);
                typ_need_comma = true;
            
            } else arr = cgi_add_att(arr, att); /* Add name and content element in array */
        }

        if (is_node_ns_wfs(n) && !strcmp((char *) n->name, "TypeName")) {
            /* Add typename to the matching global buffer */
            typename = cgi_add_into_buffer(typename, n, typ_need_comma);
            typ_need_comma = true;
        }
        else if (is_node_ns_wfs(n) && !strcmp((char *) n->name, "LockID")) lock_error = true;
        /* If it's an operation, keep the xml to analyze it later */
        else if (  is_node_ns_wfs(n) 
                   && (    !strcmp((char *) n->name, "Insert")
                        || !strcmp((char *) n->name, "Delete")
                        || !strcmp((char *) n->name, "Update"))) {

            if (!operations->use) buffer_add_str(operations, "<operations>");
            operations = cgi_add_xml_into_buffer(operations, n); /* Add the whole xml operation to the buffer */
        }
        /* if node name match 'Query', parse the children elements */
        else if (is_node_ns_wfs(n) && !strcmp((char *) n->name, "Query")) {
            /* each query's propertynames and filter must be in brackets */
            buffer_add_str(prop, "(");
            buffer_add_str(filter, "(");

            for (node = n->children; node; node = node->next) {
                /*execute the process only if n is an element and not spaces for instance */
                if (node->type != XML_ELEMENT_NODE) continue;

                if (is_node_ns_wfs(node) && !strcmp((char *) node->name, "PropertyName")) {
                    /* add propertyname to the matching global buffer */
                    prop = cgi_add_into_buffer(prop, node, prop_need_comma);
                    prop_need_comma = true;
                } else if (is_node_ns_wfs(node) && !strcmp((char *) node->name, "Filter")) {
                    /* add the whole xml filter to the matching global buffer */
                    filter = cgi_add_xml_into_buffer(filter, node);
                } else if (is_node_ns_wfs(node) && !strcmp((char *) node->name, "SortBy")) {
                    /* add sortby element to the array */
                    arr = cgi_add_sortby(arr, node);
                } else {
                    /* add element to the array */
                    arr = cgi_add_node(arr, node);
                }
            }

            /* when there aren't any propertynames, an '*' must be included into 
               the buffer to be sur that propertyname's size and typename's size are similar */
            if (prop_need_comma == false) buffer_add_str(prop, "*");

            buffer_add_str(prop, ")");
            prop_need_comma = false;
            /* when there is no filter, an empty element must be included into
	       the list to be sur that filter's size and typename's size are similar */
            buffer_add_str(filter, ")");

        } else unknown_error = true;
    }

    /* operations */
    if (operations->use) {
        buffer_add_str(operations, "</operations>");
        arr = cgi_add_buffer(arr, operations, "operations");
    }

    /* propertyname */
    if (prop->use) {
        /* if buffer just contains a series of (*), propertyname not useful */
        if (!check_regexp(prop->buf, "^[\\(\\)\\*]+$")) {
            arr = cgi_add_buffer(arr, prop, "propertyname");
        }
    }

    /* filter */
    if (filter->use) {
        /* if buffer just contains a series of (), filter not useful */
        if (!check_regexp(filter->buf, "^[\\(\\)]+$")) {
            arr = cgi_add_buffer(arr, filter, "filter");
        }
    }

    /* typename */
    if (typename->use) arr = cgi_add_buffer(arr, typename, "typename");

    buffer_free(prop);
    buffer_free(operations);
    buffer_free(filter);
    buffer_free(typename);

    xmlFreeDoc(xmldoc);

    if (lock_error) {
        array_free(arr);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "LockID is not implemented", "request");
        return NULL;
    }
    if (unknown_error) {
        array_free(arr);
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Unknown or invalid Query", "request");
        return NULL;
    }

    return arr;
}


/*
 * vim: expandtab sw=4 ts=4
 */

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
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>

#include "ows.h"
#include "../ows_define.h"


/*
 * Initialize ows_request structure
 */
ows_request *ows_request_init()
{
    ows_request *or;
    or = malloc(sizeof(ows_request));
    assert(or);

    or->version = NULL;
    or->service = OWS_SERVICE_UNKNOWN;
    or->method = OWS_METHOD_UNKNOWN;
    or->request.wfs = NULL;

    return or;
}


/*
 * Release ows_request structure
 */
void ows_request_free(ows_request * or)
{
    assert(or);

    if (or->version) ows_version_free(or->version);

    switch (or->service) {
        case WFS:
            if (or->request.wfs) wfs_request_free(or->request.wfs);
            break;
        case OWS_SERVICE_UNKNOWN:
            break;
        default: assert(0); /* Should not happen */
    }

    free(or);
    or = NULL;
}


#ifdef OWS_DEBUG
/*
 * Flush an ows_request structure
 * Used for debug purpose
 */
void ows_request_flush(ows_request * or, FILE * output)
{
    assert(or);
    assert(output);

    fprintf(output, "method:%d\n", or->method);
    fprintf(output, "service:%d\n", or->service);

    if (or->version) {
        fprintf(output, "version:");
        ows_version_flush(or->version, output);
        fprintf(output, "\n");
    }
}
#endif


/*
 * Simple callback used to catch error and warning from libxml2 schema
 * validation
 */
static void libxml2_callback  (void * ctx, const char * msg, ...) {
    va_list varg;
    char * str;
    ows *o;

    o = (ows *) ctx;
    va_start(varg, msg);
    str = (char *)va_arg( varg, char *);
    ows_log(o, 1, str);

#ifdef OWS_DEBUG
    fprintf(stderr, "%s", str);
#endif
}


static xmlSchemaPtr ows_generate_schema(const ows *o, buffer * xml_schema, bool schema_is_file)
{
    xmlSchemaParserCtxtPtr ctxt;
    xmlSchemaPtr schema = NULL;

    assert(o);
    assert(xml_schema);

    /* Open XML Schema File */
    if (schema_is_file) ctxt = xmlSchemaNewParserCtxt(xml_schema->buf);
    else                ctxt = xmlSchemaNewMemParserCtxt(xml_schema->buf, xml_schema->use);

    xmlSchemaSetParserErrors(ctxt,
                             (xmlSchemaValidityErrorFunc) libxml2_callback,
                             (xmlSchemaValidityWarningFunc) libxml2_callback,
                             (void *) o);

    schema = xmlSchemaParse(ctxt);
    xmlSchemaFreeParserCtxt(ctxt);

    /* If XML Schema hasn't been rightly loaded */
    if (!schema) {
        xmlSchemaCleanupTypes();
	return NULL;
    }

    return schema;
}


/*
 * Valid an xml string against an XML schema
 */
int ows_schema_validation(ows *o, buffer *xml_schema, buffer *xml, bool schema_is_file, enum ows_schema_type schema_type)
{
    xmlSchemaPtr schema;
    xmlSchemaValidCtxtPtr schema_ctx;
    xmlDocPtr doc;
    int ret = -1;
    bool schema_generate = true;

    assert(o);
    assert(xml);
    assert(xml_schema);

    doc = xmlParseMemory(xml->buf, xml->use);
    if (!doc) return ret;
    if (!ows_libxml_check_namespace(o, doc->children)) {
        xmlFreeDoc(doc);
        return ret;
    }

    if (schema_type == WFS_SCHEMA_TYPE_100 && o->schema_wfs_100) {
	schema_generate = false;
	schema = o->schema_wfs_100;
    } else if (schema_type == WFS_SCHEMA_TYPE_110 && o->schema_wfs_110) {
	schema_generate = false;
	schema = o->schema_wfs_110;
    }

    if (schema_generate) schema = ows_generate_schema(o, xml_schema, schema_is_file);
    if (!schema) {
        xmlFreeDoc(doc);
        return ret;
    }

    schema_ctx = xmlSchemaNewValidCtxt(schema);
    xmlSchemaSetValidErrors(schema_ctx,
                            (xmlSchemaValidityErrorFunc) libxml2_callback,
                            (xmlSchemaValidityWarningFunc) libxml2_callback, (void *) o);
    if (schema_ctx) { 
        ret = xmlSchemaValidateDoc(schema_ctx, doc); /* validation */
        xmlSchemaFreeValidCtxt(schema_ctx);
    }
    xmlFreeDoc(doc);

    if (schema_generate) {
    	     if (schema_type == WFS_SCHEMA_TYPE_100) o->schema_wfs_100 = schema;
        else if (schema_type == WFS_SCHEMA_TYPE_110) o->schema_wfs_110 = schema;
    } 

    return ret;
}


/*
 * Check and fill version
 */
static ows_version *ows_request_check_version(ows * o, ows_request * or, const array * cgi)
{
    buffer *b;
    ows_version *v;
    list *l = NULL;

    assert(o);
    assert(or);
    assert(cgi);

    v = or->version;
    b = array_get(cgi, "version");

    if (buffer_cmp(b, "")) ows_version_set(o->request->version, 0, 0, 0);
    else {
        /* check if version format is x.y.z */
        if (b->use < 5) {
            ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                      "VERSION parameter is not valid (use x.y.z)", "version");
	    return v;
        }

        l = list_explode('.', b);
        if (l->size != 3) {
            list_free(l);
            ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                      "VERSION parameter is not valid (use x.y.z)", "version");
            return v;
        }

        if (    check_regexp(l->first->value->buf, "^[0-9]+$")
             && check_regexp(l->first->next->value->buf, "^[0-9]+$")
             && check_regexp(l->first->next->next->value->buf, "^[0-9]+$")) {
            v->major = atoi(l->first->value->buf);
            v->minor = atoi(l->first->next->value->buf);
            v->release = atoi(l->first->next->next->value->buf);
        } else {
            list_free(l);
            ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                      "VERSION parameter is not valid (use x.y.z)", "version");
            return v;
        }

        list_free(l);
    }

    return v;
}


/*
 * Check common OWS parameters's validity
 *(service, version, request and layers definition)
 */
void ows_request_check(ows * o, ows_request * or, const array * cgi, const char *query)
{
    list_node *srid;
    buffer *typename, *xmlstring, *schema, *b=NULL;
    ows_layer_node *ln = NULL;
    bool srsname = false;
    int valid = 0;

    assert(o);
    assert(or);
    assert(cgi);
    assert(query);

    /* check if SERVICE is set */
    if (!array_is_key(cgi, "service")) {
        /* Tests WFS 1.1.0 require a default value for requests
           encoded in XML if service is not set */
        if (cgi_method_get()) {
            ows_error(o, OWS_ERROR_MISSING_PARAMETER_VALUE, "SERVICE is not set", "SERVICE");
            return;
        } else {
            if (buffer_case_cmp(o->metadata->type, "WFS"))
                or->service = WFS;
            else {
                ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "service unknown", "service");
                return;
            }
        }
    } else {
        b = array_get(cgi, "service");

        if (buffer_case_cmp(b, "WFS"))
            or->service = WFS;
        else if (buffer_case_cmp(b, "")) {
            if (buffer_case_cmp(o->metadata->type, "WFS"))
                or->service = WFS;
            else { 
                ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "service unknown", "service");
                return;
            }
        } else {
                ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "service unknown", "service");
                return;
        }
    }


    /* check if REQUEST is set */
    if (!array_is_key(cgi, "request")) {
        ows_error(o, OWS_ERROR_MISSING_PARAMETER_VALUE, "REQUEST is not set", "REQUEST");
        return;
    } else b = array_get(cgi, "request");

    /* check if VERSION is set and init Version */
    or->version = ows_version_init();

    if (!array_is_key(cgi, "version") || buffer_cmp(array_get(cgi, "version"), "")) {
        if (!buffer_case_cmp(b, "GetCapabilities")) {
            /* WFS 1.1.0 with KVP need a version set */
            if (o->request->method == OWS_METHOD_KVP) {
                ows_error(o, OWS_ERROR_MISSING_PARAMETER_VALUE, "VERSION is not set", "VERSION");
                return;
            }
            /* WFS 1.1.0 require a default value for requests
               encoded in XML if version is not set */
            else if (o->request->method == OWS_METHOD_XML) {
                if (or->service == WFS) ows_version_set(o->request->version, 1, 1, 0);
            }
        }
    } else or->version = ows_request_check_version(o, or, cgi);

    /* check if layers have name or title and srs */
    for (ln = o->layers->first ; ln ; ln = ln->next) {

        if (!ln->layer->name) {
            ows_error(o, OWS_ERROR_CONFIG_FILE, "No layer name defined", "config_file");
            return;
        }

        if (!ows_layer_match_table(o, ln->layer->name)) continue;

        if (!ln->layer->title) {
            ows_error(o, OWS_ERROR_CONFIG_FILE, "No layer title defined", "config_file");
            return;
        }

        if (or->service == WFS) {
            if (!ln->layer->ns_prefix->use) {
                ows_error(o, OWS_ERROR_CONFIG_FILE, "No layer NameSpace prefix defined", "config_file");
                return;
            }

            if (!ln->layer->ns_uri->use) {
                ows_error(o, OWS_ERROR_CONFIG_FILE, "No layer NameSpace URI defined", "config_file");
                return;
            }
        }

        if (ln->layer->srid) {
            if (array_is_key(cgi, "srsname") && array_is_key(cgi, "typename")) {
                b = array_get(cgi, "srsname");
                typename = array_get(cgi, "typename");

                if (buffer_cmp(typename, ln->layer->name->buf)) {
                    if (    !check_regexp(b->buf, "^http://www.opengis.net")
                         && !check_regexp(b->buf, "^EPSG")
                         && !check_regexp(b->buf, "^urn:")) {
                        ows_error(o, OWS_ERROR_CONFIG_FILE, "srsname isn't valid", "srsName");
                        return;
                    }

                    for (srid = ln->layer->srid->first ; srid ; srid = srid->next) {
                        if (check_regexp(b->buf, srid->value->buf)) srsname = true;
                    }

                    if (srsname == false) {
                        ows_error(o, OWS_ERROR_CONFIG_FILE, "srsname doesn't match srid",
                                  "config_file");
                        return;
                    }
                }
            }
        }
    }

    /* check XML Validity */
    if ( (cgi_method_post() && (    !strcmp(getenv("CONTENT_TYPE"), "application/xml; charset=UTF-8")
                                 || !strcmp(getenv("CONTENT_TYPE"), "application/xml")
                                 || !strcmp(getenv("CONTENT_TYPE"), "text/xml")
                                 || !strcmp(getenv("CONTENT_TYPE"), "text/plain"))) 
         || (!cgi_method_post() && !cgi_method_get() && query[0] == '<') /* Unit test command line use case */ ) {

        if (or->service == WFS && o->check_schema) {
            xmlstring = buffer_from_str(query);

            if (ows_version_get(or->version) == 100) {
                schema = wfs_generate_schema(o, or->version);
                valid = ows_schema_validation(o, schema, xmlstring, false, WFS_SCHEMA_TYPE_100);
            } else {
                schema = wfs_generate_schema(o, or->version);
                valid = ows_schema_validation(o, schema, xmlstring, false, WFS_SCHEMA_TYPE_110);
            }

            buffer_free(schema);
            buffer_free(xmlstring);
            
            if (valid != 0) {
                ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "XML request isn't valid", "request");
                return;
            }
        }
    }
}


/*
 * vim: expandtab sw=4 ts=4
 */

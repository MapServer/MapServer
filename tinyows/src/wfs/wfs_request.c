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
 * Initialize wfs_request structure
 */
wfs_request *wfs_request_init()
{
    wfs_request *wr;

    wr = malloc(sizeof(wfs_request));
    assert(wr);

    wr->request = WFS_REQUEST_UNKNOWN;
    wr->format = WFS_FORMAT_UNKNOWN;
    wr->typename = NULL;
    wr->propertyname = NULL;
    wr->bbox = NULL;
    wr->srs = NULL;

    wr->maxfeatures = -1;
    wr->featureid = NULL;
    wr->filter = NULL;
    wr->operation = NULL;
    wr->handle = NULL;
    wr->resulttype = NULL;
    wr->sortby = NULL;
    wr->sections = NULL;

    wr->insert_results = NULL;
    wr->delete_results = 0;
    wr->update_results = 0;

    return wr;
}


#ifdef OWS_DEBUG
/*
 * Print wfs_request structure
 */
void wfs_request_flush(wfs_request * wr, FILE * output)
{
    assert(wr);
    assert(output);

    fprintf(output, "[\n");

    fprintf(output, " request -> %i\n", wr->request);
    fprintf(output, " format -> %i\n", wr->format);
    fprintf(output, " maxfeatures -> %i\n", wr->maxfeatures);

    if (wr->typename) {
        fprintf(output, " typename -> ");
        list_flush(wr->typename, output);
        fprintf(output, "\n");
    }

    if (wr->propertyname) {
        fprintf(output, "propertyname -> ");
        mlist_flush(wr->propertyname, output);
        fprintf(output, "\n");
    }

    if (wr->bbox) {
        fprintf(output, "bbox -> ");
        ows_bbox_flush(wr->bbox, output);
        fprintf(output, "\n");
    }

    if (wr->srs) {
        fprintf(output, "srs -> ");
        ows_srs_flush(wr->srs, output);
        fprintf(output, "\n");
    }

    if (wr->featureid) {
        fprintf(output, " featureid -> ");
        mlist_flush(wr->featureid, output);
        fprintf(output, "\n");
    }

    if (wr->filter) {
        fprintf(output, " filter -> ");
        list_flush(wr->filter, output);
        fprintf(output, "\n");
    }

    if (wr->operation) {
        fprintf(output, " operation -> ");
        buffer_flush(wr->operation, output);
        fprintf(output, "\n");
    }

    if (wr->handle) {
        fprintf(output, " handle -> ");
        list_flush(wr->handle, output);
        fprintf(output, "\n");
    }

    if (wr->resulttype) {
        fprintf(output, " resulttype -> ");
        buffer_flush(wr->resulttype, output);
        fprintf(output, "\n");
    }

    if (wr->sortby) {
        fprintf(output, " sortby -> ");
        buffer_flush(wr->sortby, output);
        fprintf(output, "\n");
    }

    if (wr->sections) {
        fprintf(output, " sections -> ");
        list_flush(wr->sections, output);
        fprintf(output, "\n");
    }

    if (wr->insert_results) {
        fprintf(output, " insert_results -> ");
        alist_flush(wr->insert_results, output);
        fprintf(output, "\n");
    }

    fprintf(output, "]\n");
}
#endif


/*
 * Release wfs_request structure
 */
void wfs_request_free(wfs_request * wr)
{
    assert(wr);

    if (wr->typename)       list_free(wr->typename);
    if (wr->propertyname)   mlist_free(wr->propertyname);
    if (wr->bbox)           ows_bbox_free(wr->bbox);
    if (wr->srs)            ows_srs_free(wr->srs);
    if (wr->featureid)      mlist_free(wr->featureid);
    if (wr->filter)         list_free(wr->filter);
    if (wr->operation)      buffer_free(wr->operation);
    if (wr->handle)         list_free(wr->handle);
    if (wr->resulttype)     buffer_free(wr->resulttype);
    if (wr->sortby)         buffer_free(wr->sortby);
    if (wr->sections)       list_free(wr->sections);
    if (wr->insert_results) alist_free(wr->insert_results);

    free(wr);
    wr = NULL;
}


/*
 * Check if version is 1.0.0 or 1.1.0
 */
static void wfs_request_check_version(ows * o, wfs_request * wr, const array * cgi)
{
    assert(o);
    assert(wr);
    assert(cgi);

    if (!array_is_key(cgi, "version"))
        memcpy(o->request->version, o->wfs_default_version, sizeof(ows_version));

    if (ows_version_get(o->request->version) != 100 && ows_version_get(o->request->version) != 110)
           ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
               "VERSION parameter is not valid (use 1.0.0 or 1.1.0)", "version");
}


/*
 * Remove prefix namespace from a buffer 
 * to expect return value could be used in SQL request
 */
buffer *wfs_request_remove_namespaces(ows * o, buffer * b)
{
    list *prefix;

    assert(o);
    assert(b);

    /* TODO could be rewrite without using a list */
    prefix = list_explode(':', b);

    if (prefix->first->next) {
        buffer_empty(b);
        buffer_copy(b, prefix->first->next->value);
    }

    list_free(prefix);

    return b;
}


/*
 * Check and fill typename parameter
 * Return a list of layers' names
 */
static list *wfs_request_check_typename(ows * o, wfs_request * wr, list * layer_name)
{
    buffer *b;
    list_node *ln;

    assert(o);
    assert(wr);
    assert(layer_name);

    if (array_is_key(o->cgi, "typename")) {
        b = array_get(o->cgi, "typename");
        wr->typename = list_explode(',', b);

        for (ln = wr->typename->first ; ln ; ln = ln->next) {
            
            ln->value = wfs_request_remove_namespaces(o, ln->value); /* Remove prefix */
            list_add_by_copy(layer_name, ln->value);                 /* Fill the global layer name list */

            /* Check if layer exists and have storage */
            if (!ows_layer_match_table(o, ln->value)) {
                list_free(layer_name);
                wfs_error(o, wr, WFS_ERROR_LAYER_NOT_DEFINED, "Unknown layer name", "typename");
                return NULL;
            }

            /* Check if layer is retrievable */
            if ((wr->request == WFS_GET_FEATURE || wr->request == WFS_DESCRIBE_FEATURE_TYPE) 
                 && !ows_layer_retrievable(o->layers, ln->value)) {
                list_free(layer_name);
                wfs_error(o, wr, WFS_ERROR_LAYER_NOT_RETRIEVABLE,
                          "Not retrievable layer(s), Forbidden operation.", "typename");
                return NULL;
            }

            /* Check if layer is writable, if request is a transaction operation */
            if (wr->operation && !ows_layer_writable(o->layers, ln->value)) {
                list_free(layer_name);
                wfs_error(o, wr, WFS_ERROR_LAYER_NOT_WRITABLE,
                          "Not writable layer(s), Forbidden Transaction Operation", "typename");
                return NULL;
            }
        }
    }

    return layer_name;
}


/*
 * Check and fill the featureid parameter
 * Return a list of layers' names if typename parameter is not defined
 */
static list *wfs_request_check_fid(ows * o, wfs_request * wr, list * layer_name)
{
    list *fe;
    mlist *f;
    buffer *b;
    list_node *ln, *ln_tpn;
    mlist_node *mln = NULL;

    assert(o);
    assert(wr);
    assert(layer_name);

    ln = ln_tpn = NULL;

    /* Featureid is not a mandatory parameter */
    if (!array_is_key(o->cgi, "featureid")) return layer_name; 

    b = array_get(o->cgi, "featureid");
    f = mlist_explode('(', ')', b);

    if (wr->typename) {
        /* Check if Typename and FeatureId size are similar */
        if (f->size != wr->typename->size) {
            mlist_free(f);
            wfs_error(o, wr, WFS_ERROR_INCORRECT_SIZE_PARAMETER,
                "featureid list and typename lists must have the same size", "");
            return NULL;
        }

        ln_tpn = wr->typename->first;
    }

    for (mln = f->first ; mln ; mln = mln->next) {
        for (ln = mln->value->first ; ln ; ln = ln->next) {
            fe = list_explode('.', ln->value);

            if (wr->typename) {
                /* Check the mapping between fid and typename */
                if (!buffer_cmp(fe->first->value, ln_tpn->value->buf)) {
                    list_free(layer_name);
                    list_free(fe);
                    mlist_free(f);
                    wfs_error(o, wr, WFS_ERROR_NO_MATCHING, "featureid values and typename values don't match", "");
                    return NULL;
                }
            }

            /* Check if featureid is well formed : layer.id */
            if (!fe->first->next) {
                list_free(layer_name);
                list_free(fe);
                mlist_free(f);
                ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "featureid must match layer.id", "GetFeature");
                return NULL;
            }

            /* If typename is NULL, fill the layer name list */
            if (!wr->typename) {
                fe->first->value = wfs_request_remove_namespaces(o, fe->first->value);

                if (!in_list(layer_name, fe->first->value))
                    list_add_by_copy(layer_name, fe->first->value);
            }

            /* Check if layer exists */
            if (!ows_layer_in_list(o->layers, fe->first->value)) {
                list_free(layer_name);
                list_free(fe);
                mlist_free(f);
                wfs_error(o, wr, WFS_ERROR_LAYER_NOT_DEFINED, "Unknown layer name", "GetFeature");
                return NULL;
            }

            /* Check if layer is retrievable if request is getFeature */
            if (wr->request == WFS_GET_FEATURE && !ows_layer_retrievable(o->layers, fe->first->value)) {
                list_free(layer_name);
                list_free(fe);
                mlist_free(f);
                wfs_error(o, wr, WFS_ERROR_LAYER_NOT_RETRIEVABLE,
                    "not-retrievable layer(s), GetFeature Operation impossible, change configuration file", "GetFeature");
                return NULL;
            }

            /* Check if layer is writable if request is a transaction operation */
            if (wr->operation && !ows_layer_writable(o->layers, fe->first->value)) {
                list_free(layer_name);
                list_free(fe);
                mlist_free(f);
                wfs_error(o, wr, WFS_ERROR_LAYER_NOT_WRITABLE,
                    "not-writable layer(s), Transaction Operation impossible, change configuration file", "Transaction");
                return NULL;
            }

            list_free(fe);
        }

        if (wr->typename) ln_tpn = ln_tpn->next;
    }

    wr->featureid = f;

    return layer_name;
}


/*
 * Check and fill the bbox and srsName parameter
 */
static void wfs_request_check_srs(ows * o, wfs_request * wr, list * layer_name)
{
    int srid, srid_tmp;
    buffer *b;
    list_node *ln;

    assert(o);
    assert(wr);
    assert(layer_name);

    wr->srs = ows_srs_init();

    /* srsName is not a mandatory parameter */
    if (!array_is_key(o->cgi, "srsname")) {
        
        /* We took the default SRS from the first requested layer */
        srid = ows_srs_get_srid_from_layer(o, layer_name->first->value);

        /* And check if all layers have the same SRS */
        if (wr->typename && wr->typename->first->next) {
            for (ln = layer_name->first->next ; ln ; ln = ln->next) {
                srid_tmp = ows_srs_get_srid_from_layer(o, ln->value);

                if (srid != srid_tmp) {
                    ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                        "Layers in TYPENAME must have the same SRS", "GetFeature");
                    return;
                }
            }
        }

        if(!ows_srs_set_from_srid(o, wr->srs, srid)) {
             ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                 "srsName value use an unsupported value, for requested layer(s)", "GetFeature");
             return;
        }

        if (wr->srs->is_degree && ows_version_get(o->request->version) == 110) {
            wr->srs->is_reverse_axis = true;
            wr->srs->is_long = true;
        } else if (ows_version_get(o->request->version) == 110) {
            wr->srs->is_long = true;
        }

    } else {
        b = array_get(o->cgi, "srsname");
        if (!ows_srs_set_from_srsname(o, wr->srs, b->buf)) {
             ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                 "srsName value use an unsupported value, for requested layer(s)", "GetFeature");
             return;
        }
    }
}


/*
 * Check and fill the bbox parameter
 */
static void wfs_request_check_bbox(ows * o, wfs_request * wr, list * layer_name)
{
    buffer *b;
    int srid = 4326; /* Default srid if not srs is provided since WFS 1.1.0 */

    assert(o);
    assert(wr);
    assert(layer_name);
    assert(wr->srs);
    
    if (!array_is_key(o->cgi, "bbox")) return;  /* BBOX is not mandatory */
  
    b = array_get(o->cgi, "bbox");
    wr->bbox = ows_bbox_init();

    if (ows_version_get(o->request->version) == 100) srid = wr->srs->srid;

    if (!ows_bbox_set_from_str(o, wr->bbox, b->buf, srid)) {
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Bad parameters for Bbox, must be Xmin,Ymin,Xmax,Ymax[,crsuri]", "NULL");
        return;
    }

#if 0
    /* Not use anymore related to BBOX with crsuri and srsName issue like:
       BBOX=246700,6780800,436400,6924000,EPSG:3067&srsname=urn:ogc:def:crs:EPSG::2393
    */
    if (wr->srs->is_reverse_axis) wr->bbox->srs->is_reverse_axis = true;
#endif
}


/*
 * Check and fill the output parameter
 */
static void wfs_request_check_output(ows * o, wfs_request * wr)
{
    assert(o);
    assert(wr);

    if (!array_is_key(o->cgi, "outputformat")) {
        /* put the default values according to WFS version and request name */
        if (ows_version_get(o->request->version) == 100) {
            if (wr->request == WFS_GET_FEATURE) wr->format = WFS_GML212;
            else  /* DescribeFeatureType */     wr->format = WFS_XML_SCHEMA;
        } else
            wr->format = WFS_GML311;
    } else {
        if (buffer_cmp(array_get(o->cgi, "outputformat"), "GML2")
                || buffer_cmp(array_get(o->cgi, "outputformat"), "text/xml; subtype=gml/2.1.2"))
            wr->format = WFS_GML212;
        else if (buffer_cmp(array_get(o->cgi, "outputformat"), "GML3")
                 || buffer_cmp(array_get(o->cgi, "outputformat"), "text/xml; subtype=gml/3.1.1"))
            wr->format = WFS_GML311;
        else if (buffer_cmp(array_get(o->cgi, "outputformat"), "JSON")
                || buffer_cmp(array_get(o->cgi, "outputformat"), "application/json"))
            wr->format = WFS_GEOJSON;
        else if (wr->request == WFS_DESCRIBE_FEATURE_TYPE
                 && buffer_cmp(array_get(o->cgi, "outputformat"), "XMLSCHEMA"))
            wr->format = WFS_XML_SCHEMA;
        else
            wfs_error(o, wr, WFS_ERROR_OUTPUT_FORMAT_NOT_SUPPORTED,
                      "OutputFormat is not supported", "GetFeature");
    }
}


/*
 * Check and fill the resultType parameter
 */
static void wfs_request_check_resulttype(ows * o, wfs_request * wr)
{
    buffer *b;

    assert(o);
    assert(wr);

    wr->resulttype = buffer_init();

    /* default value is 'results' */
    if (!array_is_key(o->cgi, "resulttype")) {
        buffer_add_str(wr->resulttype, "results");
        return;
    }

    b = array_get(o->cgi, "resulttype");

    if (buffer_cmp(b, "results") || buffer_cmp(b, "hits"))
        buffer_copy(wr->resulttype, b);
    else
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                  "ResultType isn't valid, must be results or hits",
                  "resultType");
}


/*
 * Check and fill the sortBy parameter
 */
static void wfs_request_check_sortby(ows * o, wfs_request * wr)
{
    buffer *b;
    list *l, *fe;
    list_node *ln = NULL;

    assert(o);
    assert(wr);

    /* sortBy is not a mandatory parameter */
    if (!array_is_key(o->cgi, "sortby")) return;

    b = array_get(o->cgi, "sortby");
    wr->sortby = buffer_init();
    l = list_explode(',', b);

    for (ln = l->first ; ln ; ln = ln->next) {
        fe = list_explode(' ', ln->value);
        /*remove namespaces */
        fe->first->value = wfs_request_remove_namespaces(o, fe->first->value);

        /* add quotation marks */
        buffer_add_head_str(fe->first->value, "\"");
        buffer_add_str(fe->first->value, "\"");

        /* put the order into postgresql syntax */
        if (fe->last->value && fe->last != fe->first) {
            if (buffer_cmp(fe->last->value, "D") || buffer_cmp(fe->last->value, "DESC")) {
                buffer_empty(fe->last->value);
                buffer_add_str(fe->last->value, "DESC");
            } else {
                buffer_empty(fe->last->value);
                buffer_add_str(fe->last->value, "ASC");
            }
        }

        buffer_copy(wr->sortby, fe->first->value);
        buffer_add_str(wr->sortby, " ");

        if (fe->last->value && fe->last != fe->first)
            buffer_copy(wr->sortby, fe->last->value);
        else buffer_add_str(wr->sortby, "ASC");

        if (ln->next) buffer_add_str(wr->sortby, ",");

        list_free(fe);
    }

    list_free(l);
}


/*
 * Check and fill the maxFeatures parameter
 */
static void wfs_request_check_maxfeatures(ows * o, wfs_request * wr)
{
    int mf;
    buffer *b;

    assert(o);
    assert(wr);

    /* maxFeatures is not a mandatory parameter */
    if (!array_is_key(o->cgi, "maxfeatures")) return;

    b = array_get(o->cgi, "maxfeatures");
    mf = atoi(b->buf);

    if (mf > 0) wr->maxfeatures = mf;
    else ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                  "MaxFeatures isn't valid, must be > 0", "GetFeature");
}


/*
 * Check and fill the propertyName parameter
 */
static void wfs_request_check_propertyname(ows * o, wfs_request * wr, list * layer_name)
{
    buffer *b;
    mlist *f;
    list *fe;
    list_node *ln, *ln_tpn;
    array *prop_table;
    mlist_node *mln = NULL;

    assert(o);
    assert(wr);
    assert(layer_name);

    ln = ln_tpn = NULL;

    /* propertyName is not a mandatory parameter */
    if (!array_is_key(o->cgi, "propertyname")) return;

    b = array_get(o->cgi, "propertyname");
    f = mlist_explode('(', ')', b);

    /*check if propertyname size and typename or fid size are similar */
    if (f->size != layer_name->size) {
        mlist_free(f);
        wfs_error(o, wr, WFS_ERROR_INCORRECT_SIZE_PARAMETER,
                  "propertyname list size and typename list size must be similar", "GetFeature");
        return;
    }

    for (mln = f->first, ln_tpn = layer_name->first ; mln ; mln = mln->next, ln_tpn = ln_tpn->next) {
        prop_table = ows_psql_describe_table(o, ln_tpn->value);

        for (ln = mln->value->first ; ln ; ln = ln->next) {
            fe = list_explode('.', ln->value);

            /*case layer_name.propertyname */
            if (fe->first->next) {
                /*check if propertyname values match typename or fid layer names */
                if (!buffer_cmp(fe->first->value, ln_tpn->value->buf)) {
                    list_free(fe);
                    mlist_free(f);
                    wfs_error(o, wr, WFS_ERROR_NO_MATCHING,
                              "propertyname values and typename values don't match", "GetFeature");
                    return;
                }

                /* keep only the propertyname (without the layer prefixed) */
                buffer_empty(ln->value);
                buffer_copy(ln->value, fe->last->value);
            }

            list_free(fe);

            /* if propertyname is an Xpath expression */
            if (check_regexp(ln->value->buf, "\\*\\["))
                ln->value = fe_xpath_property_name(o, ln_tpn->value, ln->value);

            /* remove namespaces */
            ln->value = wfs_request_remove_namespaces(o, ln->value);

            /* check if propertyname values are correct */
            if (!buffer_cmp(ln->value, "*") && !array_is_key(prop_table, ln->value->buf)) {
                mlist_free(f);
                ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                          "propertyname values not available", "GetFeature");
                return;
            }
        }
    }

    wr->propertyname = f;
}


/*
 * Check and fill the filter parameter
 */
static void wfs_request_check_filter(ows * o, wfs_request * wr)
{
    buffer *b, *filter;

    assert(o);
    assert(wr);

    if (!array_is_key(o->cgi, "filter")) return; /* Filter is not mandatory */

    b = array_get(o->cgi, "filter");
    filter = buffer_init();
    buffer_copy(filter, b);
    wr->filter = list_explode_start_end('(', ')', filter);

    if (wr->filter->size != wr->typename->size)
        wfs_error(o, wr, WFS_ERROR_INCORRECT_SIZE_PARAMETER,
            "Filter list size and typename list size must be similar", "GetFeature");

    buffer_free(filter);
}


/*
 * Check and fill the operation parameter
 * Only used for GET method and Delete Operation
 */
static void wfs_request_check_operation(ows * o, wfs_request * wr)
{
    assert(o);
    assert(wr);

    /* OPERATION parameter is mandatory */
    if (!array_is_key(o->cgi, "operation")) {
        ows_error(o, OWS_ERROR_MISSING_PARAMETER_VALUE, "Operation (Delete) must be specified", "Operation");
        return;
    }

    wr->operation = buffer_init();
    buffer_copy(wr->operation, array_get(o->cgi, "operation"));

    if (!buffer_cmp(wr->operation, "Delete")) {
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                  "Only Delete operation is supported with GET method, use POST method for insert and update operations",
                  "Transaction");
        return;
    }
}


/*
 * Check global parameters validity
 */
static void wfs_request_check_parameters(ows * o, wfs_request * wr)
{
    assert(o);
    assert(wr);

    if (!array_is_key(o->cgi, "typename") && !array_is_key(o->cgi, "featureid")) {
        ows_error(o, OWS_ERROR_MISSING_PARAMETER_VALUE, "TYPENAME or FEATUREID must be set", "request");
        return;
    }

    /* Test mutually exclusive parameters (FILTER, BBOX and FEATUREID) */
    if (    (array_is_key(o->cgi, "filter") && array_is_key(o->cgi, "bbox"))
         || (array_is_key(o->cgi, "filter") && array_is_key(o->cgi, "featureid"))
         || (array_is_key(o->cgi, "bbox")   && array_is_key(o->cgi, "featureid")))
        wfs_error(o, wr, WFS_ERROR_EXCLUSIVE_PARAMETERS,
           "FILTER, BBOX and FEATUREID are mutually exclusive, just use one of these parameters", "request");
}


/*
 * Check and fill all WFS get_capabilities parameter
 */
static void wfs_request_check_get_capabilities(ows * o, wfs_request * wr, const array * cgi)
{
    buffer *b;
    list *l;
    list_node *ln = NULL;
    bool version = false;

    assert(o);
    assert(wr);
    assert(cgi);

    /* If key version is not set, version = higher version */
    if (!array_is_key(cgi, "version")) {
            ows_version_set(o->request->version, 1, 1, 0);
    } else {
             if (ows_version_get(o->request->version) < 110) ows_version_set(o->request->version, 1, 0, 0);
        else if (ows_version_get(o->request->version) > 110) ows_version_set(o->request->version, 1, 1, 0);
    }

    /* 1.1.0 parameter : uses the first valid version */
    if (array_is_key(cgi, "acceptversions")) {
        b = array_get(cgi, "acceptversions");
        l = list_explode(',', b);

        for (ln = l->first ; ln ; ln = ln->next) {
            if (version == false) {
                if (buffer_cmp(ln->value, "1.0.0")) {
                    ows_version_set(o->request->version, 1, 0, 0);
                    version = true;
                } else if (buffer_cmp(ln->value, "1.1.0")) {
                    ows_version_set(o->request->version, 1, 1, 0);
                    version = true;
                }
            }
        }

        list_free(l);
        /* if versions weren't 1.0.0 or 1.1.0, raise an error */
        if (version == false) {
            ows_error(o, OWS_ERROR_VERSION_NEGOTIATION_FAILED,
                      "VERSION parameter is not valid (use 1.0.0 or 1.1.0)",
                      "AcceptVersions");
            return;
        }
    }

    /* TODO updateSequence is not yet implemented */
    if (array_is_key(cgi, "updatesequence")) {
        b = array_get(cgi, "updatesequence");

        if (!buffer_cmp(b, "0")) {
            ows_error(o, OWS_ERROR_INVALID_UPDATE_SEQUENCE,
                      "UPDATESEQUENCE value must be omit or 0, function not (yet) implemented",
                      "updateSequence");
            return;
        }
    }

    /* Sections */
    if (array_is_key(cgi, "sections")) {
        b = array_get(cgi, "sections");
        l = list_explode(',', b);
        wr->sections = l;
    }

    /* AcceptFormats */
    if (array_is_key(cgi, "acceptformats")) {
        b = array_get(cgi, "acceptformats");

             if (buffer_cmp(b, "text/xml"))        wr->format = WFS_TEXT_XML;
        else if (buffer_cmp(b, "application/xml")) wr->format = WFS_APPLICATION_XML;
        else ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                      "Not supported format : use text/xml or application/xml", "acceptFormats");
    }
}


/*
 * Check and fill all WFS describe_feature_type parameter
 */
static void wfs_request_check_describe_feature_type(ows * o, wfs_request * wr, const array * cgi)
{
    list *layer_name;
    ows_layer_node *ln = NULL;

    assert(cgi);
    assert(o);
    assert(wr);
    
    wfs_request_check_output(o, wr); /* OutputFormat */
    if(o->exit) return;

    /* Typename */
    layer_name = list_init();
    layer_name = wfs_request_check_typename(o, wr, layer_name);
    if(o->exit) return;
    list_free(layer_name);

    /* if no Typename parameter is given, retrieve all layers defined in configuration file */
    if (!array_is_key(cgi, "typename")) {
        wr->typename = list_init();

        for (ln = o->layers->first ; ln ; ln = ln->next)
            if (ows_layer_match_table(o, ln->layer->name))
                list_add_by_copy(wr->typename, ln->layer->name);
    }
}


/*
 * Check anf fill parameters of GetFeature request
 */
static void wfs_request_check_get_feature(ows * o, wfs_request * wr, const array * cgi)
{
    list *layer_name;

    assert(o);
    assert(wr);
    assert(cgi);

    wfs_request_check_parameters(o, wr);  /* Check parameters validity */

    layer_name = list_init();

    /* TypeName (required except if there is a FeatureId parameter) */
    if (!o->exit) layer_name = wfs_request_check_typename(o, wr, layer_name);

    /* FeatureId, if no TypeName defined, list of layer_name must be extracted from FeatureId */
    if (!o->exit) layer_name = wfs_request_check_fid(o, wr, layer_name);

    if (!o->exit) wfs_request_check_srs(o, wr, layer_name);          /* srsName */
    if (!o->exit) wfs_request_check_bbox(o, wr, layer_name);         /* BBOX=xmin,ymin,xmax,ymax[,srs] */
    if (!o->exit) wfs_request_check_propertyname(o, wr, layer_name); /* PropertyName */
    if (!o->exit) wfs_request_check_output(o, wr);                   /* outputFormat */
    if (!o->exit) wfs_request_check_resulttype(o, wr);               /* resultType */
    if (!o->exit) wfs_request_check_sortby(o, wr);                   /* sortBy */
    if (!o->exit) wfs_request_check_maxfeatures(o, wr);              /* maxFeatures */
    if (!o->exit) wfs_request_check_filter(o, wr);                   /* Filter */

    if (layer_name) list_free(layer_name);
}


/*
 * Check anf fill parameters of Transaction request
 */
static void wfs_request_check_transaction(ows * o, wfs_request * wr, const array * cgi)
{
    list *layer_name;

    assert(o);
    assert(wr);
    assert(cgi);

    layer_name = list_init();

    if (!o->exit) wfs_request_check_operation(o, wr);                         /* Operation */
    if (!o->exit) wfs_request_check_parameters(o, wr);                        /* General checks */
    if (!o->exit) layer_name = wfs_request_check_typename(o, wr, layer_name); /* Typename */
    if (!o->exit) layer_name = wfs_request_check_fid(o, wr, layer_name);      /* FeatureId */
    if (!o->exit) wfs_request_check_bbox(o, wr, layer_name);                  /* Bbox */ 
    if (!o->exit) wfs_request_check_filter(o, wr);                            /* Filter */

    list_free(layer_name);
}


/*
 * Check if wfs_request is valid
 */
void wfs_request_check(ows * o, wfs_request * wr, const array * cgi)
{
    buffer *b;

    assert(o);
    assert(wr);
    assert(cgi);

    b = array_get(cgi, "request");

    /* Service */
    if (o->request->service != WFS) {
        ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "bad service, should be WFS", "SERVICE");
        return;
    }

    /* Version */
    if (!buffer_case_cmp(b, "GetCapabilities")) {
        wfs_request_check_version(o, wr, cgi);
        if (o->exit) return;
    }

    /* Request */
           if (buffer_case_cmp(b, "GetCapabilities")) {
        wr->request = WFS_GET_CAPABILITIES;
        wfs_request_check_get_capabilities(o, wr, cgi);

    } else if (buffer_case_cmp(b, "DescribeFeatureType")) {
        wr->request = WFS_DESCRIBE_FEATURE_TYPE;
        wfs_request_check_describe_feature_type(o, wr, cgi);

    } else if (buffer_case_cmp(b, "GetFeature")) {
        wr->request = WFS_GET_FEATURE;
        wfs_request_check_get_feature(o, wr, cgi);

    } else if (buffer_case_cmp(b, "Transaction")) {
        wr->request = WFS_TRANSACTION;
        if (cgi_method_get()) wfs_request_check_transaction(o, wr, cgi);

    } else ows_error(o, OWS_ERROR_OPERATION_NOT_SUPPORTED,
                  "REQUEST is not supported", "REQUEST");
}


/*
 * Main function to call the right action's function
 */
void wfs(ows * o, wfs_request * wf)
{
    buffer *op;

    assert(o);
    assert(wf);

    /* Run the request's execution */
    switch (wf->request) {
        case WFS_GET_CAPABILITIES:
            wfs_get_capabilities(o, wf);
            break;

        case WFS_DESCRIBE_FEATURE_TYPE:
            wfs_describe_feature_type(o, wf);
            break;

        case WFS_GET_FEATURE:
            wfs_get_feature(o, wf);
            break;

        case WFS_TRANSACTION:

            if (cgi_method_get()) {
                if (buffer_cmp(wf->operation, "Delete"))
                    wfs_delete(o, wf);
                else {
                    ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                              "Only Delete Operation is supported with GET method, use POST instead for Insert and Update.",
                              "Transaction");
                    return;
                }

            } else {
                if (array_is_key(o->cgi, "operations")) {
                    op = array_get(o->cgi, "operations");
                    wfs_parse_operation(o, wf, op);
                } else {
                    ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, "Operation parameter must be set", "Transaction");
                    return;
                }
            }

            break;

        default:
            assert(0);              /* Should not happen */
    }
}


/*
 * vim: expandtab sw=4 ts=4
 */

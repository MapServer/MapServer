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


#ifndef OWS_H 
#define OWS_H
#include <stdio.h> 		/* FILE prototype */
#include <libpq-fe.h>
#include <libxml/xmlreader.h>


/* define OWS_DEBUG if needed */
#if 1
#define OWS_DEBUG
#endif

/* ========= Structures ========= */

enum Bool { 
	false,
	true
};

typedef enum Bool bool;

#define BUFFER_SIZE_INIT   32

typedef struct Buffer {
    size_t use; 		/** size used for data */
    size_t size;		/** memory available */
    size_t realloc;		/** size to next realloc */
    char * buf;			/** data */
} buffer;


typedef struct List_node {
    buffer * value;
    struct List_node * next;
    struct List_node * prev;
} list_node;

typedef struct List {
	list_node * first;
	list_node * last;
    unsigned int size;
} list;


typedef struct Mlist_node {
    list * value;
    struct Mlist_node * next;
    struct Mlist_node * prev;
} mlist_node;

typedef struct Mlist {
	mlist_node * first;
	mlist_node * last;
    unsigned int size;
} mlist;


typedef struct Array_node {
	buffer * key;
	buffer * value;
	struct Array_node * next;
} array_node;

typedef struct Array {
	array_node * first;
	array_node * last;
} array;


/* ========= OWS Common ========= */


typedef struct Ows_srs {
	int srid;
	buffer * auth_name;
	int auth_srid;
    bool is_unit_degree;
} ows_srs;


typedef struct Ows_bbox {
	double xmin;
	double ymin;
	double xmax;
	double ymax;
    ows_srs * srs;
} ows_bbox;

typedef struct Ows_geobbox {
	double east;
	double west;
	double south;
	double north;
} ows_geobbox;

typedef struct Ows_version {
    int major;
    int minor;
    int release;
} ows_version;

/*cf table 19 in OWS specification*/
enum ows_error_code {
    OWS_ERROR_OPERATION_NOT_SUPPORTED,
    OWS_ERROR_MISSING_PARAMETER_VALUE,
    OWS_ERROR_INVALID_PARAMETER_VALUE,
    OWS_ERROR_VERSION_NEGOTIATION_FAILED,
    OWS_ERROR_INVALID_UPDATE_SEQUENCE,
    OWS_ERROR_NO_APPLICABLE_CODE,
    /* Add-on to the spec on the same way */
    OWS_ERROR_CONNECTION_FAILED,
    OWS_ERROR_CONFIG_FILE,
    OWS_ERROR_REQUEST_SQL_FAILED,
    OWS_ERROR_REQUEST_HTTP,
    OWS_ERROR_FORBIDDEN_CHARACTER,
    OWS_ERROR_MISSING_METADATA,
    OWS_ERROR_NO_SRS_DEFINED
};


typedef struct Ows_layer {
    struct Ows_layer * parent;
    int depth;
    buffer * name;
    buffer * title;
    bool queryable;
    /* OGC NOTA cascaded is not yet implemented (cf WMS 1.3.0: 7.2.4.7.3) */
    /* OGC NOTA subsettable is not yet implemented (cf WMS 1.3.0: 7.2.4.7.5) */
    bool retrievable;
    bool writable;
    bool opaque;
    list * styles;
    list * srid;
    ows_geobbox * geobbox;
    buffer * abstract;
    list * keywords;
    buffer * prefix;
    buffer * server;
} ows_layer;

typedef struct Ows_layer_node {
    ows_layer * layer;
    struct Ows_layer_node * next;
    struct Ows_layer_node * prev;
} ows_layer_node;

typedef struct Ows_layer_list {
    ows_layer_node * first;
    ows_layer_node * last;
    unsigned int size;
} ows_layer_list;


typedef struct Ows_meta {
    buffer * type;
    list * versions;
    buffer * name;
    buffer * title;
    buffer * abstract;
    list * keywords;
    buffer * online_resource;
    buffer * fees;
    buffer * access_constraints;
} ows_meta;

typedef struct Ows_contact {
    buffer * name;
    buffer * site;
    buffer * indiv_name;
    buffer * position;
    buffer * phone;
    buffer * fax;
    buffer * online_resource;
    buffer * address;
    buffer * postcode;
    buffer * city;
    buffer * state;
    buffer * country;
    buffer * email;
    buffer * hours;
    buffer * instructions;
} ows_contact;

enum ows_service {
    WMS,
    WFS,
    OWS_SERVICE_UNKNOWN
};



/* ========= WMS ========= */

/* cf table E.1 in WMS 1.3.0 */

enum wms_error_code {
	WMS_ERROR_INVALID_FORMAT,
	WMS_ERROR_INVALID_CRS,
	WMS_ERROR_LAYER_NOT_DEFINED,
	WMS_ERROR_STYLE_NOT_DEFINED,
	WMS_ERROR_LAYER_NOT_QUERYABLE,
	WMS_ERROR_INVALID_POINT,
	WMS_ERROR_CURRENT_UPDATE_SEQUENCE,
	WMS_ERROR_INVALID_UPDATE_SEQUENCE,
	WMS_ERROR_MISSING_DIMENSION_VALUE,
	WMS_ERROR_INVALID_DIMENSION_VALUE,
	WMS_ERROR_OPERATION_NOT_SUPPORTED,
    /* Add-on to the spec on the same way */
    WMS_ERROR_INVALID_VERSION,
    WMS_ERROR_INVALID_BBOX,
    WMS_ERROR_INVALID_WIDTH,
    WMS_ERROR_INVALID_HEIGHT
};

enum wms_request {
	WMS_REQUEST_UNKNOWN,
	WMS_GET_CAPABILITIES,
	WMS_GET_MAP
};

enum wms_format {
	WMS_FORMAT_UNKNOWN,
	WMS_FORMAT_SVG
};

enum wms_exception {
	WMS_EXCEPTION_UNKNOWN,
	WMS_EXCEPTION_BLANK,
	WMS_EXCEPTION_INIMAGE,
	WMS_EXCEPTION_XML
};

typedef struct Wms_request {
    bool service;
    ows_version * version;
    enum wms_request request;
    int width;
    int height;
    enum wms_format format;
    enum wms_exception exception;
    list * layers;
    list * styles;
    ows_bbox * bbox;
    ows_srs * srs;
    int request_i;
    int request_j;
    int feature_count;
    bool transparent;
    buffer * bgcolor;
    /* TODO DIMENSION / TIME support */
    /* TODO SLD support */

} wms_request;


/* ========= WFS ========= */


enum wfs_error_code {
    WFS_ERROR_INVALID_VERSION,
    WFS_ERROR_OUTPUT_FORMAT_NOT_SUPPORTED,
    WFS_ERROR_LAYER_NOT_DEFINED,
    WFS_ERROR_LAYER_NOT_RETRIEVABLE,
    WFS_ERROR_LAYER_NOT_WRITABLE,
    WFS_ERROR_EXCLUSIVE_PARAMETERS,
    WFS_ERROR_INCORRECT_SIZE_PARAMETER,
    WFS_ERROR_NO_MATCHING
};

enum wfs_request {
	WFS_REQUEST_UNKNOWN,
	WFS_GET_CAPABILITIES,
	WFS_DESCRIBE_FEATURE_TYPE,
	WFS_GET_FEATURE,
	WFS_TRANSACTION
};

enum wfs_format {
	WFS_FORMAT_UNKNOWN,
	WFS_XML_SCHEMA,
	WFS_GML2,
	WFS_GML3,
	WFS_TEXT_XML,
	WFS_APPLICATION_XML
};


typedef struct Wfs_request {
    enum wfs_request request;
    enum wfs_format format;
    list * typename;
    ows_bbox * bbox;
    mlist * propertyname;
    int maxfeatures;
    mlist * featureid;
    list * filter;
    buffer * operation;
    list * handle;
    buffer * resulttype;
    buffer * sortby;
    list * sections;

  mlist * insert_results;
  int delete_results;
  int update_results;
  
} wfs_request;


/* ========= FE ========= */


enum fe_error_code {
    FE_NO_ERROR,
    FE_ERROR_FEATUREID,
    FE_ERROR_FILTER,
    FE_ERROR_BBOX,
    FE_ERROR_PROPERTYNAME,
    FE_ERROR_UNITS,
    FE_ERROR_GEOMETRY,
    FE_ERROR_FID,
    FE_ERROR_SRS
};

typedef struct Filter_encoding {
    buffer * sql;
    enum fe_error_code error_code;
} filter_encoding;


/* ========= OWS Request & Main ========= */

typedef struct Ows_request {
    ows_version * version;
    enum ows_service service;
    union {
        wfs_request * wfs;
        wms_request * wms;
        } request;
} ows_request;

typedef struct Ows {
    PGconn * pg;
    buffer * pg_dsn;
    FILE* output;

    ows_meta * metadata;
    ows_contact * contact;

    int max_width;
    int max_height;
    int max_layers;
    int max_features;
    ows_geobbox * max_geobbox;

    array * cgi;
    list * psql_requests;
    ows_layer_list * layers;
    ows_request * request;
  
    buffer * sld_path;
    bool sld_writable;               
} ows;


#include "../ows_api.h"

#endif /* OWS_H */


/*
 * vim: expandtab sw=4 ts=4 
 */

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


#ifndef OWS_STRUCT_H 
#define OWS_STRUCT_H

#include <stdio.h> 		/* FILE prototype */


/* ========= Structures ========= */

enum Bool { 
	false,
	true
};

typedef enum Bool bool;

#define BUFFER_SIZE_INIT   256

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


typedef struct Alist_node {
	buffer * key;
	list * value;
	struct Alist_node * next;
} alist_node;

typedef struct Alist {
	alist_node * first;
	alist_node * last;
} alist;


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

typedef struct Ows_layer_storage {
    buffer * schema;
    buffer * table;
    list * geom_columns;
    list * not_null_columns;
    int srid;
    buffer * pkey;
    buffer * pkey_sequence;
    int pkey_column_number;
    bool is_degree;
    array * attributes;
} ows_layer_storage;

typedef struct Ows_srs {
    int srid;
    buffer * auth_name;
    int auth_srid;
    bool is_degree;
    bool is_reverse_axis;
    bool is_eastern_axis;
    bool is_long;
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
    bool retrievable;
    bool writable;
    list * srid;
    ows_geobbox * geobbox;
    buffer * abstract;
    list * keywords;
    list * gml_ns;
    buffer * ns_prefix;
    buffer * ns_uri;
    buffer * encoding;
    ows_layer_storage * storage;
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

enum ows_method {
   OWS_METHOD_UNKNOWN,
   OWS_METHOD_KVP,
   OWS_METHOD_XML
};


/* ========= WFS ========= */


enum wfs_error_code {
    WFS_ERROR_INVALID_VERSION,
    WFS_ERROR_OUTPUT_FORMAT_NOT_SUPPORTED,
    WFS_ERROR_LAYER_NOT_DEFINED,
    WFS_ERROR_LAYER_NOT_RETRIEVABLE,
    WFS_ERROR_LAYER_NOT_WRITABLE,
    WFS_ERROR_EXCLUSIVE_PARAMETERS,
    WFS_ERROR_INCORRECT_SIZE_PARAMETER,
    WFS_ERROR_NO_MATCHING,
    WFS_ERROR_INVALID_PARAMETER,
    WFS_ERROR_MISSING_PARAMETER
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
	WFS_GML212,
	WFS_GML311,
	WFS_GML321,
	WFS_GEOJSON,
	WFS_TEXT_XML,
	WFS_APPLICATION_XML
};

enum wfs_insert_idgen {
	WFS_GENERATE_NEW,
	WFS_USE_EXISTING,
	WFS_REPLACE_DUPLICATE
};

enum ows_schema_type {
	WFS_SCHEMA_TYPE_100,
	WFS_SCHEMA_TYPE_110
};

typedef struct Wfs_request {
    enum wfs_request request;
    enum wfs_format format;
    list * typename;
    ows_bbox * bbox;
    mlist * propertyname;
    int maxfeatures;
    ows_srs * srs;
    mlist * featureid;
    list * filter;
    buffer * operation;
    list * handle;
    buffer * resulttype;
    buffer * sortby;
    list * sections;

    alist * insert_results;
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
    FE_ERROR_GEOM_PROPERTYNAME,
    FE_ERROR_UNITS,
    FE_ERROR_GEOMETRY,
    FE_ERROR_FID,
    FE_ERROR_SRS,
    FE_ERROR_FUNCTION,
    FE_ERROR_NAMESPACE
};

typedef struct Filter_encoding {
    bool in_not;
    buffer * sql;
    enum fe_error_code error_code;
} filter_encoding;


/* ========= OWS Request & Main ========= */

typedef struct Ows_request {
    ows_version * version;
    enum ows_method method;
    enum ows_service service;
    union {
        wfs_request * wfs;
        } request;
} ows_request;

#define OWS_DEFAULT_XML_ENCODING "UTF-8" 
#define OWS_DEFAULT_DB_ENCODING "UTF8" 

#define OWS_MAX_DOUBLE 1e15  /* %f vs %g */

typedef struct Ows {
    bool init;
    bool exit;
    PGconn * pg;
    bool mapfile;
    buffer * config_file;
    buffer * schema_dir;
    buffer * online_resource;
    buffer * pg_dsn;
    buffer * encoding;
    buffer * db_encoding;

    FILE* log;
    int log_level;
    buffer * log_file;

    FILE* output;

    ows_meta * metadata;
    ows_contact * contact;

    int degree_precision;
    int meter_precision;

    int max_features;
    ows_geobbox * max_geobbox;

    bool display_bbox;
    bool expose_pk;
    bool estimated_extent;

    bool check_schema;
    bool check_valid_geom;

    array * cgi;
    list * psql_requests;
    ows_layer_list * layers;
    ows_request * request;
    ows_version * wfs_default_version;
    ows_version * postgis_version;

    xmlSchemaPtr  schema_wfs_100;
    xmlSchemaPtr  schema_wfs_110;
} ows;

#endif /* OWS_STRUCT_H */


/*
 * vim: expandtab sw=4 ts=4 
 */

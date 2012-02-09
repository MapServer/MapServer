%{


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <libpq-fe.h>
#include <libxml/xmlreader.h>
#include "../ows_define.h"
#include "../ows_struct.h"
#include "../ows_api.h"

#define OWS_MAP_EATLINE char c ; for (c=input() ; c && c != '\n' ; c=input());
#define OWS_MAP_MAX_INCLUDE_DEPTH 5

#if 0
#define OWS_MAP_DBG fprintf(stderr, "State: {%i} %i \n", yy_top_state(), YY_START);
#else
#define OWS_MAP_DBG 
#endif


static buffer * mapfile_build_path(ows * o, char * fpath)
{
	buffer * path;
	
	assert(fpath);

	/* Path is absolute */
	if ((fpath[0] == '/') || (fpath[0] == '\\') || (strlen(fpath) && (fpath[1] == ':')))
		return buffer_from_str(fpath);

	/* Path is relative */
	path = buffer_init();
	buffer_copy(path, o->config_file);

 	for ( /* empty */ ; path->use ; path->use--) 
		if ((path->buf[path->use] == '/') || (path->buf[path->use] == '\\')) { path->use++ ; break; }	
	
	buffer_add_str(path, fpath);

	return path;
}

enum map_md_state {
    MAP_MD_UPDATESEQUENCE,
    MAP_MD_SCHEMAS_LOCATION,
    MAP_MD_ABSTRACT,
    MAP_MD_ACCESSCONSTRAINTS,
    MAP_MD_ENCODING,
    MAP_MD_FEES,
    MAP_MD_KEYWORDLIST,
    MAP_MD_MAXFEATURES,
    MAP_MD_NAMESPACE_PREFIX,
    MAP_MD_NAMESPACE_URI,
    MAP_MD_TITLE,
    MAP_MD_SRS,
    MAP_MD_FEATURE_COLLECTION,
    MAP_MD_CONTACT_NAME,
    MAP_MD_CONTACT_EMAIL,
    MAP_MD_CONTACT_INDIVIDUAL,
    MAP_MD_CONTACT_POSITION,
    MAP_MD_CONTACT_PHONE,
    MAP_MD_CONTACT_FAX,
    MAP_MD_CONTACT_ADDRESS,
    MAP_MD_CONTACT_CITY,
    MAP_MD_CONTACT_COUNTRY,
    MAP_MD_TOWS_ONLINERESOURCE,
    MAP_MD_TOWS_SCHEMA_DIR,
    MAP_MD_TOWS_DB_ENCODING,
    MAP_MD_TOWS_LOG,
    MAP_MD_TOWS_LOG_LEVEL,
    MAP_MD_TOWS_WFS_DEFAULT_VERSION,
    MAP_MD_TOWS_METER_PRECISION,
    MAP_MD_TOWS_DEGREE_PRECISION,
    MAP_MD_TOWS_DISPLAY_BBOX,
    MAP_MD_TOWS_ESTIMATED_EXTENT,
    MAP_MD_TOWS_CHECK_SCHEMA,
    MAP_MD_TOWS_CHECK_VALID_GEOM,
    MAP_MD_TOWS_EXPOSE_PK,
    MAP_MD_TOWS_GEOBBOX,
    MAP_MD_SKIP
};

enum map_lmd_state {
    MAP_LMD_EXCLUDE_ITEMS,
    MAP_LMD_FEATURE_ID,
    MAP_LMD_INCLUDE_ITEMS,
    MAP_LMD_XML_ITEMS,
    MAP_LMD_ABSTRACT,
    MAP_LMD_EXTENT,
    MAP_LMD_KEYWORDLIST,
    MAP_LMD_METADATAURL_FORMAT,
    MAP_LMD_METADATAURL_HREF,
    MAP_LMD_METADATAURL_TYPE,
    MAP_LMD_NAMESPACE_PREFIX,
    MAP_LMD_NAMESPACE_URI,
    MAP_LMD_SRS,
    MAP_LMD_TITLE,
    MAP_LMD_TOWS_TABLE,
    MAP_LMD_TOWS_SCHEMA,
    MAP_LMD_TOWS_RETRIEVABLE,
    MAP_LMD_TOWS_WRITABLE,
    MAP_LMD_TOWS_GEOBBOX,
    MAP_LMD_TOWS_GML_NS_LIST,
    MAP_LMD_SKIP
};

enum map_prop_state {
    MAP_NAME
};

enum map_layer_prop_state {
    MAP_LAYER_NAME,
    MAP_LAYER_CONNECTION
	
};

/*
 * Global Vars 
 */
ows *map_o = NULL;
ows_layer *map_l = NULL;
list * map_srid = NULL;
buffer *map_ns_prefix = NULL;
buffer *map_ns_uri = NULL;
bool map_is_postgis = false;
bool map_is_dump = false;
YY_BUFFER_STATE map_include_stack[OWS_MAP_MAX_INCLUDE_DEPTH];
int map_include_stack_ptr = 0;
enum map_md_state map_md_state;
enum map_lmd_state map_lmd_state;
enum map_prop_state map_prop_state;
enum map_layer_prop_state map_layer_prop_state;


static void metadata_key(char *yytext, bool quotes)
{
	char *p;

	/* remove surrounding quotes */
	if (quotes) {
		 for (p = yytext; *p ; p++); *(--p) = 0; yytext++;
	}

	if     (!strncmp("ows_updatesequence", yytext, 18))
		map_md_state = MAP_MD_UPDATESEQUENCE;
	else if(!strncmp("ows_schemas_location", yytext, 20))
		map_md_state = MAP_MD_SCHEMAS_LOCATION;
	else if(!strncmp("wfs_abstract", yytext, 12))
		map_md_state = MAP_MD_ABSTRACT;
	else if(!strncmp("wfs_accessconstraints", yytext, 21))
		map_md_state = MAP_MD_ACCESSCONSTRAINTS;
	else if(!strncmp("wfs_encoding", yytext, 12))
		map_md_state = MAP_MD_ENCODING;
	else if(!strncmp("wfs_fees", yytext, 8))
		map_md_state = MAP_MD_FEES;
	else if(!strncmp("wfs_keywordlist", yytext, 15))
		map_md_state = MAP_MD_KEYWORDLIST;
	else if(!strncmp("wfs_maxfeatures", yytext, 15))
		map_md_state = MAP_MD_MAXFEATURES;
	else if(!strncmp("wfs_namespace_prefix", yytext, 20))
		map_md_state = MAP_MD_NAMESPACE_PREFIX;
	else if(!strncmp("wfs_namespace_uri", yytext, 17))
		map_md_state = MAP_MD_NAMESPACE_URI;
	else if(!strncmp("wfs_title", yytext, 9))
		map_md_state = MAP_MD_TITLE;
	else if(!strncmp("wfs_srs", yytext, 7))
		map_md_state = MAP_MD_SRS;
	else if(!strncmp("wfs_feature_collection", yytext, 22))
		map_md_state = MAP_MD_FEATURE_COLLECTION;
	else if(!strncmp("ows_contactorganization", yytext, 23))
		map_md_state = MAP_MD_CONTACT_NAME;
	else if(!strncmp("ows_contactelectronicmailaddress", yytext, 32))
		map_md_state = MAP_MD_CONTACT_EMAIL;
	else if(!strncmp("ows_contactperson", yytext, 17))
		map_md_state = MAP_MD_CONTACT_INDIVIDUAL;
	else if(!strncmp("ows_contactposition", yytext, 19))
		map_md_state = MAP_MD_CONTACT_POSITION;
	else if(!strncmp("ows_contactphone", yytext, 16))
		map_md_state = MAP_MD_CONTACT_PHONE;
	else if(!strncmp("ows_address", yytext, 11))
		map_md_state = MAP_MD_CONTACT_ADDRESS;
	else if(!strncmp("ows_city", yytext, 8))
		map_md_state = MAP_MD_CONTACT_CITY;
	else if(!strncmp("ows_country", yytext, 11))
		map_md_state = MAP_MD_CONTACT_COUNTRY;
	else if(!strncmp("ows_contactfacsimiletelephone", yytext, 29))
		map_md_state = MAP_MD_CONTACT_FAX;
	else if(!strncmp("tinyows_onlineresource", yytext, 21))
		map_md_state = MAP_MD_TOWS_ONLINERESOURCE;
	else if(!strncmp("tinyows_db_encoding", yytext, 19))
		map_md_state = MAP_MD_TOWS_DB_ENCODING;
	else if(!strncmp("tinyows_schema_dir", yytext, 18))
		map_md_state = MAP_MD_TOWS_SCHEMA_DIR;
	else if(!strncmp("tinyows_log_level", yytext, 17))
		map_md_state = MAP_MD_TOWS_LOG_LEVEL;
	else if(!strncmp("tinyows_log", yytext, 11))
		map_md_state = MAP_MD_TOWS_LOG;
	else if(!strncmp("tinyows_wfs_default_version", yytext, 77))
		map_md_state = MAP_MD_TOWS_WFS_DEFAULT_VERSION;
	else if(!strncmp("tinyows_degree_precision", yytext, 24))
		map_md_state = MAP_MD_TOWS_DEGREE_PRECISION;
	else if(!strncmp("tinyows_meter_precision", yytext, 23))
		map_md_state = MAP_MD_TOWS_METER_PRECISION;
	else if(!strncmp("tinyows_display_bbox", yytext, 20))
		map_md_state = MAP_MD_TOWS_DISPLAY_BBOX;
	else if(!strncmp("tinyows_estimated_extent", yytext, 24))
		map_md_state = MAP_MD_TOWS_ESTIMATED_EXTENT;
	else if(!strncmp("tinyows_check_schema", yytext, 20))
		map_md_state = MAP_MD_TOWS_CHECK_SCHEMA;
	else if(!strncmp("tinyows_check_valid_geom", yytext, 24))
		map_md_state = MAP_MD_TOWS_CHECK_VALID_GEOM;
	else if(!strncmp("tinyows_expose_pk", yytext, 17))
		map_md_state = MAP_MD_TOWS_EXPOSE_PK;
	else if(!strncmp("tinyows_geobbox", yytext, 15))
		map_md_state = MAP_MD_TOWS_GEOBBOX;
	else map_md_state = MAP_MD_SKIP;
}

static void metadata_value(char *yytext, bool quotes)
{
	int i;
	char *p;
	ows_geobbox *g;
 
	/* remove surrounding quotes */
	if (quotes) {
		 for (p = yytext; *p ; p++); *(--p) = 0; yytext++;
	}

	switch(map_md_state)
	{
		case MAP_MD_SKIP: return;
		case MAP_MD_UPDATESEQUENCE: 		return; /* TODO */
		case MAP_MD_SCHEMAS_LOCATION:		return; /* TODO */
		case MAP_MD_FEATURE_COLLECTION:		return; /* TODO */
		case MAP_MD_ABSTRACT:	
			map_o->metadata->abstract = buffer_init();
        		buffer_add_str(map_o->metadata->abstract, yytext);
			return;
		case MAP_MD_ACCESSCONSTRAINTS:
			map_o->metadata->access_constraints = buffer_init();
       			buffer_add_str(map_o->metadata->access_constraints, yytext);	
			return;
		case MAP_MD_ENCODING:
       			buffer_add_str(map_o->encoding, yytext);	
			return;
		case MAP_MD_FEES:
			map_o->metadata->fees = buffer_init();
       			buffer_add_str(map_o->metadata->fees, yytext);	
			return;
		case MAP_MD_KEYWORDLIST:
			map_o->metadata->keywords = list_explode_str(',', yytext);
			return;
		case MAP_MD_MAXFEATURES:
			map_o->max_features = atoi(yytext);
			return;
		case MAP_MD_NAMESPACE_PREFIX:
			map_ns_prefix = buffer_init();
			buffer_add_str(map_ns_prefix, yytext);
			return;
		case MAP_MD_NAMESPACE_URI:
			map_ns_uri = buffer_init();
			buffer_add_str(map_ns_uri, yytext);
			return;
		case MAP_MD_TITLE:
		    	map_o->metadata->title = buffer_init();
        		buffer_add_str(map_o->metadata->title, yytext);
			return;
		case MAP_MD_SRS:
			map_srid = list_explode_str(',', yytext);
			return;
		case MAP_MD_CONTACT_NAME:
			if (!map_o->contact) map_o->contact=ows_contact_init();
			map_o->contact->name = buffer_from_str(yytext);
			return;
		case MAP_MD_CONTACT_EMAIL:
			if (!map_o->contact) map_o->contact=ows_contact_init();
			map_o->contact->email = buffer_from_str(yytext);
			return;
		case MAP_MD_CONTACT_INDIVIDUAL:
			if (!map_o->contact) map_o->contact=ows_contact_init();
			map_o->contact->indiv_name = buffer_from_str(yytext);
			return;
		case MAP_MD_CONTACT_POSITION:
			if (!map_o->contact) map_o->contact=ows_contact_init();
			map_o->contact->position = buffer_from_str(yytext);
			return;
		case MAP_MD_CONTACT_PHONE:
			if (!map_o->contact) map_o->contact=ows_contact_init();
			map_o->contact->phone = buffer_from_str(yytext);
			return;
		case MAP_MD_CONTACT_FAX:
			if (!map_o->contact) map_o->contact=ows_contact_init();
			map_o->contact->fax = buffer_from_str(yytext);
			return;
		case MAP_MD_CONTACT_ADDRESS:
			if (!map_o->contact) map_o->contact=ows_contact_init();
			map_o->contact->address = buffer_from_str(yytext);
			return;
		case MAP_MD_CONTACT_CITY:
			if (!map_o->contact) map_o->contact=ows_contact_init();
			map_o->contact->city = buffer_from_str(yytext);
			return;
		case MAP_MD_CONTACT_COUNTRY:
			if (!map_o->contact) map_o->contact=ows_contact_init();
			map_o->contact->country = buffer_from_str(yytext);
			return;
		case MAP_MD_TOWS_SCHEMA_DIR:
        		buffer_add_str(map_o->schema_dir, yytext);
			return;
		case MAP_MD_TOWS_ONLINERESOURCE:
		        buffer_add_str(map_o->online_resource, yytext);
			return;
		case MAP_MD_TOWS_DB_ENCODING:
			buffer_add_str(map_o->db_encoding, yytext);
			return;
		case MAP_MD_TOWS_LOG:
			map_o->log_file = buffer_init();
			buffer_add_str(map_o->log_file, yytext);
			return;
		case MAP_MD_TOWS_LOG_LEVEL:
			i = atoi(yytext);
			if (i > 0 && i < 16) map_o->log_level = i;
			return;
                case MAP_MD_TOWS_WFS_DEFAULT_VERSION:
			ows_version_set_str(map_o->wfs_default_version, yytext);
			return;
		case MAP_MD_TOWS_DEGREE_PRECISION:
			i = atoi(yytext);
			if (i > 0 && i < 12) map_o->degree_precision = i;
			return;
		case MAP_MD_TOWS_METER_PRECISION:
			i = atoi(yytext);
			if (i > 0 && i < 12) map_o->meter_precision = i;
			return;
		case MAP_MD_TOWS_DISPLAY_BBOX:
			if (!atoi(yytext)) map_o->display_bbox = false;
			return;
		case MAP_MD_TOWS_ESTIMATED_EXTENT:
			if (atoi(yytext)) map_o->estimated_extent = true;
			return;
		case MAP_MD_TOWS_CHECK_SCHEMA:
			if (!atoi(yytext)) map_o->check_schema = false;
			return;
		case MAP_MD_TOWS_CHECK_VALID_GEOM:
			if (!atoi(yytext)) map_o->check_valid_geom = false;
			return;
		case MAP_MD_TOWS_EXPOSE_PK:
			if (atoi(yytext)) map_o->check_valid_geom = true;
			return;
		case MAP_MD_TOWS_GEOBBOX:
			g = ows_geobbox_init();
        		if (ows_geobbox_set_from_str(map_o, g, yytext)) map_o->max_geobbox = g;
        		else ows_geobbox_free(g);
			return;
	}
}

static void metadata_layer_key(char *yytext, bool quotes)
{
	char *p;

	/* remove surrounding quotes */
	if (quotes) {
		 for (p = yytext; *p ; p++); *(--p) = 0; yytext++;
	}

	     if(!strncmp("gml_exclude_items", yytext, 17))
		map_lmd_state = MAP_LMD_EXCLUDE_ITEMS;
	else if(!strncmp("gml_featureid", yytext, 13) ||
                !strncmp("wfs_featureid", yytext, 13) ||
                !strncmp("ows_featureid", yytext, 13))
		map_lmd_state = MAP_LMD_FEATURE_ID;
	/* TODO groups handle */
	else if(!strncmp("gml_include_items", yytext, 17))
		map_lmd_state = MAP_LMD_INCLUDE_ITEMS;
	/* TODO ALIAS handle */
	/* TODO TYPE handle */
	else if(!strncmp("gml_xml_items", yytext, 13))
		map_lmd_state = MAP_LMD_XML_ITEMS;
	else if(!strncmp("wfs_abstract", yytext, 12))
		map_lmd_state = MAP_LMD_ABSTRACT;
	else if(!strncmp("wfs_extent", yytext, 10))
		map_lmd_state = MAP_LMD_EXTENT;
	else if(!strncmp("wfs_keywordlist", yytext, 15))
		map_lmd_state = MAP_LMD_KEYWORDLIST;
	else if(!strncmp("wfs_namespace_prefix", yytext, 20))
		map_lmd_state = MAP_LMD_NAMESPACE_PREFIX;
	else if(!strncmp("wfs_namespace_uri", yytext, 17))
		map_lmd_state = MAP_LMD_NAMESPACE_URI;
	else if(!strncmp("wfs_metadataurl_format", yytext, 22))
		map_lmd_state = MAP_LMD_METADATAURL_FORMAT;
	else if(!strncmp("wfs_metadataurl_href", yytext, 20))
		map_lmd_state = MAP_LMD_METADATAURL_HREF;
	else if(!strncmp("wfs_metadataurl_type", yytext, 20))
		map_lmd_state = MAP_LMD_METADATAURL_TYPE;
	else if(!strncmp("wfs_srs", yytext, 7))
		map_lmd_state = MAP_LMD_SRS;
	else if(!strncmp("wfs_title", yytext, 9))
		map_lmd_state = MAP_LMD_TITLE;
	else if(!strncmp("tinyows_table", yytext, 13))
		map_lmd_state = MAP_LMD_TOWS_TABLE;
	else if(!strncmp("tinyows_schema", yytext, 14))
		map_lmd_state = MAP_LMD_TOWS_SCHEMA;
	else if(!strncmp("tinyows_retrievable", yytext, 19))
		map_lmd_state = MAP_LMD_TOWS_RETRIEVABLE;
	else if(!strncmp("tinyows_writable", yytext, 16))
		map_lmd_state = MAP_LMD_TOWS_WRITABLE;
	else if(!strncmp("tinyows_geobbox", yytext, 15))
		map_md_state = MAP_LMD_TOWS_GEOBBOX;
	else if(!strncmp("tinyows_gml_ns_list", yytext, 19))
		map_lmd_state = MAP_LMD_TOWS_GML_NS_LIST;
	else map_lmd_state = MAP_LMD_SKIP;
}

static void metadata_layer_value(char *yytext, bool quotes)
{
	ows_geobbox *g;
	char *p;

	/* remove surrounding quotes */
	if (quotes) {
		 for (p = yytext; *p ; p++); *(--p) = 0; yytext++;
	}

	switch(map_lmd_state)
	{
	case MAP_LMD_SKIP: return;
	case MAP_LMD_EXCLUDE_ITEMS:
	case MAP_LMD_FEATURE_ID:
	case MAP_LMD_INCLUDE_ITEMS:
	case MAP_LMD_XML_ITEMS:
	case MAP_LMD_EXTENT:
	case MAP_LMD_METADATAURL_FORMAT:
	case MAP_LMD_METADATAURL_HREF:
	case MAP_LMD_METADATAURL_TYPE:
		 /* TODO */ return;

	case MAP_LMD_NAMESPACE_URI:
		buffer_add_str(map_l->ns_uri, yytext);
		return;
	case MAP_LMD_NAMESPACE_PREFIX:
		buffer_add_str(map_l->ns_prefix, yytext);
		return;
	case MAP_LMD_SRS:
		map_l->srid = list_explode_str(',', yytext);
		return;
	case MAP_LMD_KEYWORDLIST:
		map_l->keywords = list_explode_str(',', yytext);
		return;
	case MAP_LMD_ABSTRACT:
		map_l->abstract = buffer_init();
       		buffer_add_str(map_l->abstract, yytext);
		return;
	case MAP_LMD_TITLE:
		map_l->title = buffer_init();
       		buffer_add_str(map_l->title, yytext);
		return;
	case MAP_LMD_TOWS_SCHEMA:
       		buffer_add_str(map_l->storage->schema, yytext);
		return;
	case MAP_LMD_TOWS_TABLE:
       		buffer_add_str(map_l->storage->table, yytext);
		return;
        case MAP_LMD_TOWS_GML_NS_LIST:
		map_l->gml_ns = list_explode_str(',', yytext);
		return;
	case MAP_LMD_TOWS_WRITABLE:
		map_l->writable=atoi(yytext)?1:0;
		return;
	case MAP_LMD_TOWS_RETRIEVABLE:
		map_l->retrievable=atoi(yytext)?1:0;
		return;
	case MAP_LMD_TOWS_GEOBBOX:
		g = ows_geobbox_init();
       		if (ows_geobbox_set_from_str(map_o, g, yytext)) map_l->geobbox = g;
       		else ows_geobbox_free(g);
		return;
	}
}

static void properties(char *yytext, bool quotes)
{
	char *p;

	/* remove surrounding quotes */
	if (quotes) {
		 for (p = yytext; *p ; p++); *(--p) = 0; yytext++;
	}

	switch(map_prop_state) {
	case MAP_NAME:
		map_o->metadata->name = buffer_init();
       		buffer_add_str(map_o->metadata->name, yytext);
		return;
	}
	
}

static void layer_properties(char * yytext, bool quotes)
{
	char *p;

	/* remove surrounding quotes */
	if (quotes) {
		for (p = yytext; *p ; p++); *(--p) = 0; yytext++;
	}

	switch(map_layer_prop_state) {
	case MAP_LAYER_NAME:
		map_l->name = buffer_init();
       		buffer_add_str(map_l->name, yytext);
		return;
	case MAP_LAYER_CONNECTION:
		/* TODO: Having a Layer connection notion in TinyOWS */
		if (map_o->pg_dsn->use && strcmp(map_o->pg_dsn->buf, yytext))
			ows_error(map_o, OWS_ERROR_CONFIG_FILE,
				"MapFile have more than one single PostGIS db connection.",
				"parse_config_file");
		else if (!map_o->pg_dsn->use) buffer_add_str(map_o->pg_dsn, yytext);
		return;
	}
}


static void end_layer()
{
	if (!map_is_postgis) ows_layer_free(map_l);
	else {
		if (!map_l->storage->table->use)
			buffer_copy(map_l->storage->table, map_l->name);
		if (!map_l->storage->schema->use)
			buffer_add_str(map_l->storage->schema, "public");
		if (!map_is_dump) map_l->retrievable=map_l->writable = false;
		ows_layer_list_add(map_o->layers, map_l);
	}

		
	map_is_dump = map_is_postgis = false;
}


%}

%option stack
%option noyywrap
%option nounput
%option never-interactive

%s MAP
%s WEB
%s METADATA
%s METADATA_VALUE
%s PROPERTIES
%s LAYER_PROPERTIES
%s LAYER_METADATA
%s LAYER_METADATA_VALUE
%s LAYER_CONNECTIONTYPE
%s LAYER_DUMP
%s LAYER
%s FEATURE
%s GRID
%s JOIN
%s CLASS
%s STYLE
%s STYLE_PATTERN
%s LABEL
%s PROJECTION
%s QUERYMAP
%s SCALEBAR
%s LEGEND
%s REFERENCE
%s OUTPUTFORMAT
%s SYMBOL
%s PATTERN
%s POINTS
%s INCLUDE


%%

<INITIAL,MAP,LAYER,METADATA,LAYER_METADATA># { char c ; for (c=input() ; c && c != '\n' ; c=input()); }
<INITIAL,MAP,METADATA,LAYER,LAYER_METADATA>INCLUDE { yy_push_state(INCLUDE); OWS_MAP_DBG }


<MAP>LEGEND			{ /*LEGEND rule must appears before END... */
				  yy_push_state(LEGEND); OWS_MAP_DBG }

END 				{ 
				 OWS_MAP_EATLINE
				 if (yy_top_state() == INITIAL && YY_START != MAP) { 
					ows_error(map_o, OWS_ERROR_CONFIG_FILE,
					"MapFile Parse Error", "parse_config_file");
					yyterminate();}

				  if (YY_START == LAYER) { end_layer(); }
				  yy_pop_state(); OWS_MAP_DBG
				}

<INITIAL>MAP			{ yy_push_state(MAP); OWS_MAP_DBG }
<MAP>WEB			{ yy_push_state(WEB); OWS_MAP_DBG }
<MAP>QUERYMAP			{ yy_push_state(QUERYMAP); OWS_MAP_DBG }
<MAP>SYMBOLSET			{ OWS_MAP_EATLINE /* must be before SYMBOL */ }
<MAP>CONFIG			{ OWS_MAP_EATLINE }
<MAP>DATAPATTERN		{ OWS_MAP_EATLINE }
<MAP>TEMPLATEPATTERN		{ OWS_MAP_EATLINE }
<MAP>FONTSET			{ OWS_MAP_EATLINE }
<MAP>IMAGETYPE			{ OWS_MAP_EATLINE }
<MAP>SHAPEPATH			{ OWS_MAP_EATLINE }
<CLASS>EXPRESSION		{ OWS_MAP_EATLINE }
<CLASS>GROUP			{ OWS_MAP_EATLINE }
<CLASS>KEYIMAGE			{ OWS_MAP_EATLINE }
<CLASS>SYMBOL			{ OWS_MAP_EATLINE }
<CLASS>TEMPLATE			{ OWS_MAP_EATLINE }
<CLASS>TEXT			{ OWS_MAP_EATLINE }
<FEATURE>ITEMS			{ OWS_MAP_EATLINE }
<FEATURE>TEXT			{ OWS_MAP_EATLINE }
<FEATURE>WKT			{ OWS_MAP_EATLINE }
<LABEL>ANGLE			{ OWS_MAP_EATLINE }
<LABEL>COLOR			{ OWS_MAP_EATLINE }
<LABEL>ENCODING			{ OWS_MAP_EATLINE }
<LABEL>FONT			{ OWS_MAP_EATLINE }
<LABEL>OUTLINECOLOR		{ OWS_MAP_EATLINE }
<LABEL>PRIORITY			{ OWS_MAP_EATLINE }
<LABEL>SIZE			{ OWS_MAP_EATLINE }
<LABEL>WRAP			{ OWS_MAP_EATLINE }
<LAYER>CLASSITEM		{ OWS_MAP_EATLINE }
<LAYER>CLASSGROUP		{ OWS_MAP_EATLINE }
<LAYER>DATA			{ OWS_MAP_EATLINE }
<LAYER>FILTER			{ OWS_MAP_EATLINE }
<LAYER>FILTERITEM		{ OWS_MAP_EATLINE }
<LAYER>FOOTER			{ OWS_MAP_EATLINE }
<LAYER>GROUP			{ OWS_MAP_EATLINE }
<LAYER>HEADER			{ OWS_MAP_EATLINE }
<LAYER>LABELANGLEITEM		{ OWS_MAP_EATLINE }
<LAYER>LABELITEM		{ OWS_MAP_EATLINE }
<LEGEND>TEMPLATE		{ OWS_MAP_EATLINE }
<OUTPUTFORMAT>NAME		{ OWS_MAP_EATLINE }
<OUTPUTFORMAT>FORMATOPTION	{ OWS_MAP_EATLINE }
<REFERENCE>IMAGE		{ OWS_MAP_EATLINE }
<REFERENCE>MARKER		{ OWS_MAP_EATLINE }
<STYLE>ANGLE			{ OWS_MAP_EATLINE }
<STYLE>ANGLEITEM		{ OWS_MAP_EATLINE }
<STYLE>COLOR			{ OWS_MAP_EATLINE }
<STYLE>OPACITY			{ OWS_MAP_EATLINE }
<STYLE>OUTLINECOLOR		{ OWS_MAP_EATLINE }
<STYLE>SIZE			{ OWS_MAP_EATLINE }
<STYLE>SIZEITEM			{ OWS_MAP_EATLINE }
<STYLE>SYMBOL			{ OWS_MAP_EATLINE }
<STYLE>WIDTH			{ OWS_MAP_EATLINE }
<SYMBOL>FONT			{ OWS_MAP_EATLINE }
<SYMBOL>IMAGE			{ OWS_MAP_EATLINE }
<SYMBOL>NAME			{ OWS_MAP_EATLINE }
<WEB>EMPTY			{ OWS_MAP_EATLINE }
<WEB>ERROR			{ OWS_MAP_EATLINE }
<WEB>FOOTER			{ OWS_MAP_EATLINE }
<WEB>HEADER			{ OWS_MAP_EATLINE }
<WEB>IMAGEPATH			{ OWS_MAP_EATLINE }
<WEB>IMAGEURL			{ OWS_MAP_EATLINE }
<WEB>LOG			{ OWS_MAP_EATLINE }
<WEB>MAXTEMPLATE		{ OWS_MAP_EATLINE }
<WEB>TEMPLATE			{ OWS_MAP_EATLINE }

<MAP>SYMBOL			{ yy_push_state(SYMBOL); OWS_MAP_DBG }
<MAP>SCALEBAR			{ yy_push_state(SCALEBAR); OWS_MAP_DBG  }
<MAP>REFERENCE			{ yy_push_state(REFERENCE); OWS_MAP_DBG }
<MAP>OUTPUTFORMAT		{ yy_push_state(OUTPUTFORMAT); OWS_MAP_DBG }
<WEB>METADATA 			{ yy_push_state(METADATA); OWS_MAP_DBG }
<LAYER>GRID 			{ yy_push_state(GRID); OWS_MAP_DBG }
<LAYER>JOIN 			{ yy_push_state(JOIN); OWS_MAP_DBG }
<LAYER>METADATA 		{ yy_push_state(LAYER_METADATA); OWS_MAP_DBG }
<LAYER>CLASS 			{ yy_push_state(CLASS); OWS_MAP_DBG }
<CLASS>STYLE 			{ yy_push_state(STYLE); OWS_MAP_DBG }
<CLASS>LABEL 			{ yy_push_state(LABEL); OWS_MAP_DBG }
<MAP,LAYER>PROJECTION		{ yy_push_state(PROJECTION); OWS_MAP_DBG }
<SYMBOL,STYLE>PATTERN		{ yy_push_state(PATTERN); OWS_MAP_DBG }
<SYMBOL>POINTS			{ yy_push_state(POINTS); OWS_MAP_DBG }
<SCALEBAR,LEGEND>LABEL		{ yy_push_state(LABEL); OWS_MAP_DBG }
<MAP>LAYER 			{ yy_push_state(LAYER); map_l = ows_layer_init(); OWS_MAP_DBG }
<LAYER>FEATURE 			{ yy_push_state(FEATURE); OWS_MAP_DBG }
<MAP>NAME			{ BEGIN PROPERTIES; map_prop_state = MAP_NAME; }
<LAYER>NAME			{ BEGIN LAYER_PROPERTIES; map_layer_prop_state = MAP_LAYER_NAME; }
<LAYER>CONNECTION		{ BEGIN LAYER_PROPERTIES; map_layer_prop_state = MAP_LAYER_CONNECTION; }
<LAYER>CONNECTIONTYPE		{ BEGIN LAYER_CONNECTIONTYPE; }
<LAYER_CONNECTIONTYPE>POSTGIS	{ BEGIN LAYER; map_is_postgis=true; }
<LAYER_CONNECTIONTYPE>[A-Za-z]+	{ BEGIN LAYER; map_is_postgis=false; }
<LAYER>DUMP			{ BEGIN LAYER_DUMP; }
<LAYER_DUMP>TRUE		{ BEGIN LAYER; map_is_dump=true; }
<LAYER_DUMP>[A-Za-z]+		{ BEGIN LAYER; map_is_dump=false; }

<LAYER_METADATA,METADATA,METADATA_VALUE,LAYER_METADATA_VALUE,LAYER_PROPERTIES>[ \t\n] {}

<METADATA>\"[^\"]*\"|\'[^\']*\' 		{metadata_key(yytext, true); BEGIN METADATA_VALUE;}
<METADATA_VALUE>\"[^\"]*\"|\'[^\']*\'		{metadata_value(yytext, true); BEGIN METADATA;}
<LAYER_METADATA>\"[^\"]*\"|\'[^\']*\' 		{metadata_layer_key(yytext, true); BEGIN LAYER_METADATA_VALUE;}
<LAYER_METADATA_VALUE>\"[^\"]*\"|\'[^\']*\'	{metadata_layer_value(yytext, true); BEGIN LAYER_METADATA;}
<INCLUDE>\"[^\"]*\"|\'[^\']*\'			{

	 char *p, *q; 
         buffer *path;

	 yy_pop_state(); OWS_MAP_DBG

	 if(map_include_stack_ptr >= OWS_MAP_MAX_INCLUDE_DEPTH) {
		ows_error(map_o, OWS_ERROR_CONFIG_FILE,
		"MapFile: include nested too deeply", "parse_config_file");
		yyterminate();
         }

	 map_include_stack[map_include_stack_ptr] = YY_CURRENT_BUFFER;
         map_include_stack_ptr++;

	 for (p = q = yytext; *p ; p++); *(--p) = 0; q++; /* remove surrounding quotes */
         path = mapfile_build_path(map_o, q);
         yyin = fopen(path->buf, "r");
         buffer_free(path);

         if (!yyin) {
		ows_error(map_o, OWS_ERROR_CONFIG_FILE,
		"MapFile: include unable to open file", "parse_config_file");
		yyterminate();
	}

         yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
}


<PROPERTIES>\"[^\"]*\"|\'[^\']*\'		{properties(yytext, true);  BEGIN MAP;}
<PROPERTIES>[^ 	\n]*				{properties(yytext, false);  BEGIN MAP;}		
<LAYER_PROPERTIES>\"[^\"]*\"|\'[^\']*\'		{layer_properties(yytext, true);  BEGIN LAYER;}
<LAYER_PROPERTIES>[^ 	\n]*			{layer_properties(yytext, false);  BEGIN LAYER;}		
\"[^\"]*\"|\'[^\']*\' {}
. {}
[ \t\n_-] {}

<<EOF>> { 
	/* End of main file */
	if (map_include_stack_ptr == 0 && YY_START == 0) yyterminate(); 
	
	else if (map_include_stack_ptr) {
		fclose(YY_CURRENT_BUFFER->yy_input_file);
                yy_delete_buffer( YY_CURRENT_BUFFER );
                yy_switch_to_buffer(map_include_stack[--map_include_stack_ptr]);
	} else {
		ows_error(map_o, OWS_ERROR_CONFIG_FILE, "MapFile parse error",
			"parse_config_file"); 
		yyterminate();
	}
}

%%

void ows_parse_config_mapfile(ows * o, const char *filename)
{
	ows_layer_node *ln;

	assert(o);
	assert(filename);

	if (!(yyin = fopen(filename, "r"))) {
		ows_error(o, OWS_ERROR_CONFIG_FILE, "Unable to open MapFile !", 
				"parse_config_file");
		return;
	}

	map_o = o;
	map_o->metadata = ows_metadata_init();
	map_o->layers = ows_layer_list_init();

 	while( yylex() ) ;

	if (!map_o->encoding->use)    buffer_add_str(map_o->encoding, OWS_DEFAULT_XML_ENCODING);
	if (!map_o->db_encoding->use) buffer_add_str(map_o->db_encoding, OWS_DEFAULT_DB_ENCODING);
	
	if (map_srid) {
		for (ln = o->layers->first ; ln ; ln = ln->next)
			list_add_list(ln->layer->srid, map_srid);
		list_free(map_srid);
	}

	if (map_ns_uri) {
		for (ln = o->layers->first ; ln ; ln = ln->next)
			if (!ln->layer->ns_uri->use)
				buffer_copy(ln->layer->ns_uri, map_ns_uri); 
		buffer_free(map_ns_uri);
	}
	
	if (map_ns_prefix) {
		for (ln = o->layers->first ; ln ; ln = ln->next)
			if (!ln->layer->ns_prefix->use)
				buffer_copy(ln->layer->ns_prefix, map_ns_prefix); 
		buffer_free(map_ns_prefix);
	}

	fclose(yyin);
}

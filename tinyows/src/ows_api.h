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


void array_add (array * a, buffer * key, buffer * value);
void array_flush (const array * a, FILE * output);
void array_free (array * a);
buffer *array_get (const array * a, const char *key);
array *array_init ();
bool array_is_key (const array * a, const char *key);
alist *alist_init();
void alist_free(alist * al);
void alist_add(alist * al, buffer * key, buffer * value);
bool alist_is_key(const alist * al, const char *key);
list *alist_get(const alist * al, const char *key);
void alist_flush(const alist * al, FILE * output);
void buffer_add (buffer * buf, char c);
void buffer_add_double (buffer * buf, double f);
void buffer_add_head (buffer * buf, char c);
void buffer_add_head_str (buffer * buf, char *str);
void buffer_add_int (buffer * buf, int i);
void buffer_add_str (buffer * buf, const char *str);
buffer *buffer_from_str(const char *str);
bool buffer_cmp (const buffer * buf, const char *str);
bool buffer_ncmp(const buffer * buf, const char *str, size_t n);
bool buffer_case_cmp (const buffer * buf, const char *str);
void buffer_copy (buffer * dest, const buffer * src);
void buffer_empty (buffer * buf);
void buffer_flush (buffer * buf, FILE * output);
void buffer_free (buffer * buf);
buffer *buffer_ftoa (double f);
buffer *buffer_init ();
buffer *buffer_itoa (int i);
void buffer_pop (buffer * buf, size_t len);
buffer *buffer_replace (buffer * buf, char *before, char *after);
void buffer_shift (buffer * buf, size_t len);
buffer *buffer_encode_xml_entities_str(const char *str);
buffer *buffer_encode_json_str(const char *str);
buffer *cgi_add_xml_into_buffer (buffer * element, xmlNodePtr n);
char *cgi_getback_query (ows * o);
bool cgi_method_get ();
bool cgi_method_post ();
array *cgi_parse_kvp (ows * o, char *query);
array *cgi_parse_xml (ows * o, char *query);
bool check_regexp (const char *str_request, const char *str_regex);
buffer *fe_comparison_op (ows * o, buffer * typename, filter_encoding * fe, xmlNodePtr n);
buffer *fe_envelope (ows * o, buffer * typename, filter_encoding * fe, buffer *envelope, xmlNodePtr n);
void fe_error (ows * o, filter_encoding * fe);
buffer *fe_expression (ows * o, buffer * typename, filter_encoding * fe, buffer * sql, xmlNodePtr n);
buffer *fe_feature_id (ows * o, buffer * typename, filter_encoding * fe, xmlNodePtr n);
filter_encoding *fe_filter (ows * o, filter_encoding * fe, buffer * typename, buffer * xmlchar);
void fe_filter_capabilities_100 (const ows * o);
void fe_filter_capabilities_110 (const ows * o);
buffer *fe_function (ows * o, buffer * typename, filter_encoding * fe, buffer * sql, xmlNodePtr n);
bool fe_is_comparison_op (char *name);
bool fe_is_logical_op (char *name);
bool fe_is_spatial_op (char *name);
buffer *fe_kvp_bbox (ows * o, wfs_request * wr, buffer * layer_name, ows_bbox * bbox);
buffer *fe_kvp_featureid (ows * o, wfs_request * wr, buffer * layer_name, list * fid);
buffer *fe_logical_op (ows * o, buffer * typename, filter_encoding * fe, xmlNodePtr n);
void fe_node_flush (xmlNodePtr node, FILE * output);
buffer *fe_property_name (ows * o, buffer * typename, filter_encoding * fe, buffer * sql, xmlNodePtr n, bool check_geom_column, bool mandatory);
buffer *fe_spatial_op (ows * o, buffer * typename, filter_encoding * fe, xmlNodePtr n);
buffer *fe_xpath_property_name (ows * o, buffer * typename, buffer * property);
buffer *fill_fe_error (ows * o, filter_encoding * fe);
void filter_encoding_flush (filter_encoding * fe, FILE * output);
void filter_encoding_free (filter_encoding * fe);
filter_encoding *filter_encoding_init ();
bool in_list (const list * l, const buffer * value);
bool in_list_str (const list * l, const char * value);
void list_add (list * l, buffer * value);
void list_add_by_copy (list * l, buffer * value);
void list_add_list (list * l, list * l_to_add);
void list_add_str (list * l, char *value);
list *list_explode (char separator, const buffer * value);
list *list_explode_start_end (char separator_start, char separator_end, buffer * value);
list *list_explode_str (char separator, const char *value);
void list_flush (const list * l, FILE * output);
void list_free (list * l);
list *list_init ();
void list_node_free (list * l, list_node * ln);
list_node *list_node_init ();
int main (int argc, char *argv[]);
void mlist_add (mlist * ml, list * value);
mlist *mlist_explode (char separator_start, char separator_end, buffer * value);
void mlist_flush (const mlist * ml, FILE * output);
void mlist_free (mlist * ml);
mlist *mlist_init ();
void mlist_node_free (mlist * ml, mlist_node * mln);
mlist_node *mlist_node_init ();
ows_bbox *ows_bbox_boundaries (ows * o, list * from, list * where, ows_srs * srs);
void ows_bbox_flush (const ows_bbox * b, FILE * output);
void ows_bbox_free (ows_bbox * b);
ows_bbox *ows_bbox_init ();
bool ows_bbox_set (ows * o, ows_bbox * b, double xmin, double ymin, double xmax, double ymax, int srid);
bool ows_bbox_set_from_geobbox (ows * o, ows_bbox * bb, ows_geobbox * geo);
bool ows_bbox_set_from_str (ows * o, ows_bbox * bb, const char *str, int srid);
bool ows_bbox_transform (ows * o, ows_bbox * bb, int srid);
void ows_bbox_to_query(ows * o, ows_bbox *bbox, buffer *query);
void ows_contact_flush (ows_contact * contact, FILE * output);
void ows_contact_free (ows_contact * contact);
ows_contact *ows_contact_init ();
void ows_error (ows * o, enum ows_error_code code, char *message, char *locator);
void ows_flush (ows * o, FILE * output);
void ows_free (ows * o);
ows_geobbox *ows_geobbox_compute (ows * o, buffer * layer_name);
void ows_geobbox_flush (const ows_geobbox * g, FILE * output);
void ows_geobbox_free (ows_geobbox * g);
ows_geobbox *ows_geobbox_init ();
ows_geobbox *ows_geobbox_copy(ows_geobbox *g);
bool ows_geobbox_set (ows * o, ows_geobbox * g, double west, double east, double south, double north);
bool ows_geobbox_set_from_bbox (ows * o, ows_geobbox * g, ows_bbox * bb);
ows_geobbox *ows_geobbox_set_from_str (ows * o, ows_geobbox * g, char *str);
void ows_get_capabilities_dcpt (const ows * o, const char * req);
void ows_layer_flush (ows_layer * l, FILE * output);
void ows_layer_free (ows_layer * l);
bool ows_layer_in_list (const ows_layer_list * ll, buffer * name);
ows_layer *ows_layer_init ();
void ows_layer_list_add (ows_layer_list * ll, ows_layer * l);
list *ows_layer_list_by_ns_prefix (ows_layer_list * ll, list * layer_name, buffer * ns_prefix);
list *ows_layer_list_having_storage (const ows_layer_list * ll);
void ows_layer_list_flush (ows_layer_list * ll, FILE * output);
void ows_layer_list_free (ows_layer_list * ll);
bool ows_layer_list_in_list (const ows_layer_list * ll, const list * l);
ows_layer_list *ows_layer_list_init ();
array *ows_layer_list_namespaces (ows_layer_list * ll);
list *ows_layer_list_ns_prefix (ows_layer_list * ll, list * layer_name);
bool ows_layer_list_retrievable (const ows_layer_list * ll);
bool ows_layer_list_writable (const ows_layer_list * ll);
bool ows_layer_match_table (const ows * o, const buffer * name);
void ows_layer_node_free (ows_layer_list * ll, ows_layer_node * ln);
ows_layer_node *ows_layer_node_init ();
buffer *ows_layer_ns_prefix (ows_layer_list * ll, buffer * layer_name);
bool ows_layer_retrievable (const ows_layer_list * ll, const buffer * name);
buffer *ows_layer_ns_uri (ows_layer_list * ll, buffer * ns_prefix);
bool ows_layer_writable (const ows_layer_list * ll, const buffer * name);
void ows_metadata_fill (ows * o, array * cgi);
void ows_metadata_flush (ows_meta * metadata, FILE * output);
void ows_metadata_free (ows_meta * metadata);
ows_meta *ows_metadata_init ();
void ows_parse_config (ows * o, const char *filename);
ows_version * ows_psql_postgis_version(ows *o);
PGresult * ows_psql_exec(ows *o, const char *sql);
buffer *ows_psql_column_name (ows * o, buffer * layer_name, int number);
array *ows_psql_describe_table (ows * o, buffer * layer_name);
list *ows_psql_geometry_column (ows * o, buffer * layer_name);
buffer *ows_psql_schema_name(ows * o, buffer * layer_name);
buffer *ows_psql_table_name(ows * o, buffer * layer_name);
buffer *ows_psql_id_column (ows * o, buffer * layer_name);
int ows_psql_column_number_id_column(ows * o, buffer * layer_name);
bool ows_psql_is_geometry_column (ows * o, buffer * layer_name, buffer * column);
bool ows_psql_is_geometry_valid(ows * o, buffer * geom);
list *ows_psql_not_null_properties (ows * o, buffer * layer_name);
buffer *ows_psql_timestamp_to_xml_time (char *timestamp);
char *ows_psql_to_xsd (buffer * type, ows_version *version);
buffer *ows_psql_type (ows * o, buffer * layer_name, buffer * property);
buffer *ows_psql_generate_id (ows * o, buffer * layer_name);
int ows_psql_number_features(ows * o, list * from, list * where);
buffer * ows_psql_gml_to_sql(ows * o, xmlNodePtr n, int srid);
char *ows_psql_escape_string(ows *o, const char *content);
int ows_psql_geometry_srid(ows *o, const char *geom);
void ows_request_check (ows * o, ows_request * or, const array * cgi, const char *query);
void ows_request_flush (ows_request * or, FILE * output);
void ows_request_free (ows_request * or);
ows_request *ows_request_init ();
int ows_schema_validation (ows * o, buffer * xml_schema, buffer * xml, bool schema_is_file, enum ows_schema_type schema_type);
void ows_service_identification (const ows * o);
void ows_service_metadata (const ows * o);
void ows_service_provider (const ows * o);
void ows_srs_flush (ows_srs * c, FILE * output);
void ows_srs_free (ows_srs * c);
buffer *ows_srs_get_from_a_srid (ows * o, int srid);
list *ows_srs_get_from_srid (ows * o, list * l);
int ows_srs_get_srid_from_layer (ows * o, buffer * layer_name);
ows_srs *ows_srs_init ();
bool ows_srs_meter_units (ows * o, buffer * layer_name);
ows_srs *ows_srs_copy(ows_srs * d, ows_srs * s);
bool ows_srs_set_geobbox(ows * o, ows_srs * s);
bool ows_srs_set (ows * o, ows_srs * c, const buffer * auth_name, int auth_srid);
bool ows_srs_set_from_srid (ows * o, ows_srs * s, int srid);
bool ows_srs_set_from_srsname(ows * o, ows_srs * s, const char *srsname);
void ows_usage (ows * o);
void ows_version_flush (ows_version * v, FILE * output);
void ows_version_free (ows_version * v);
bool ows_version_check(ows_version *v);
bool ows_version_set_str(ows_version * v, char *str);
int ows_version_get (ows_version * v);
ows_version *ows_version_init ();
void ows_version_set (ows_version * v, int major, int minor, int release);
void wfs (ows * o, wfs_request * wf);
void wfs_delete (ows * o, wfs_request * wr);
void wfs_describe_feature_type (ows * o, wfs_request * wr);
buffer * wfs_generate_schema(ows * o, ows_version * version);
void wfs_error (ows * o, wfs_request * wf, enum wfs_error_code code, char *message, char *locator);
void wfs_get_capabilities (ows * o, wfs_request * wr);
void wfs_get_feature (ows * o, wfs_request * wr);
void wfs_gml_feature_member (ows * o, wfs_request * wr, buffer * layer_name, list * properties, PGresult * res);
void wfs_parse_operation (ows * o, wfs_request * wr, buffer * op);
void wfs_request_check (ows * o, wfs_request * wr, const array * cgi);
void wfs_request_flush (wfs_request * wr, FILE * output);
void wfs_request_free (wfs_request * wr);
wfs_request *wfs_request_init ();
buffer *wfs_request_remove_namespaces (ows * o, buffer * b);
ows_layer_storage * ows_layer_storage_init();
void ows_layer_storage_free(ows_layer_storage * storage);
void ows_layer_storage_flush(ows_layer_storage * storage, FILE * output);
void ows_layers_storage_fill(ows * o);
ows_layer * ows_layer_get(const ows_layer_list * ll, const buffer * name);
void ows_layers_storage_flush(ows * o, FILE * output);
void ows_log(ows *o, int log_level, const char *log);
void ows_parse_config_mapfile(ows *o, const char *filename);
bool ows_libxml_check_namespace(ows *o, xmlNodePtr n);

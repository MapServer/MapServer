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
#include <time.h>

#include "ows.h"


/*
 * Return the name of the id column from table matching layer name
 */
buffer *ows_psql_id_column(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o);
    assert(o->layers);
    assert(layer_name);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage
	    && !strcmp(ln->layer->name->buf, layer_name->buf))
            return ln->layer->storage->pkey;

    return NULL;
}


/*
 * Execute an SQL request 
 */
PGresult * ows_psql_exec(ows *o, const char *sql)
{
    assert(o);
    assert(sql);
    assert(o->pg);
     
    ows_log(o, 8, sql);
    return PQexecParams(o->pg, sql, 0, NULL, NULL, NULL, NULL, 0); 
}


/*
 * Return the column number of the id column from table matching layer name
 * (needed in wfs_get_feature only)
 */
int ows_psql_column_number_id_column(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o);
    assert(o->layers);
    assert(layer_name);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage
            && !strcmp(ln->layer->name->buf, layer_name->buf))
            return ln->layer->storage->pkey_column_number;

    return -1;
}


/*
 * Return geometry columns from the table matching layer name
 */
list *ows_psql_geometry_column(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o);
    assert(o->layers);
    assert(layer_name);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage
            && !strcmp(ln->layer->name->buf, layer_name->buf))
            return ln->layer->storage->geom_columns;

    return NULL;
}


/*
 * Return schema name from a given layer 
 */
buffer *ows_psql_schema_name(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o);
    assert(o->layers);
    assert(layer_name);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage
            && !strcmp(ln->layer->name->buf, layer_name->buf))
            return ln->layer->storage->schema;

    return NULL;
}


/*
 * Return table name from a given layer 
 */
buffer *ows_psql_table_name(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o);
    assert(o->layers);
    assert(layer_name);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage
            && !strcmp(ln->layer->name->buf, layer_name->buf))
            return ln->layer->storage->table;

    return NULL;
}


/*
 * Check if a given WKT geometry is or not valid
 */
bool ows_psql_is_geometry_valid(ows * o, buffer * geom)
{
    buffer *sql;
    PGresult *res;
    bool ret = false;

    assert(o);
    assert(geom);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT ST_isvalid(ST_geometryfromtext('");
    buffer_copy(sql, geom);
    buffer_add_str(sql, "', -1));");
    
    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1
	&& (char) PQgetvalue(res, 0, 0)[0] ==  't') ret = true;

    PQclear(res);
    return ret;
}


/*
 * Check if the specified column from a layer_name is (or not) a geometry column
 */
bool ows_psql_is_geometry_column(ows * o, buffer * layer_name, buffer * column)
{
    ows_layer_node *ln;

    assert(o);
    assert(o->layers);
    assert(layer_name);
    assert(column);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage
            && !strcmp(ln->layer->name->buf, layer_name->buf))
            return in_list(ln->layer->storage->geom_columns, column);

    return false;
}


/*
 * Return a list of not null properties from the table matching layer name
 */
list *ows_psql_not_null_properties(ows * o, buffer * layer_name)
{
    ows_layer_node *ln;

    assert(o);
    assert(o->layers);
    assert(layer_name);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage
            && !strcmp(ln->layer->name->buf, layer_name->buf))
            return ln->layer->storage->not_null_columns;

    return NULL;
}


/*
 * Return the column's name matching the specified number from table
 * (Only use in specific FE position function, so not directly inside
 *  storage handle mechanism)
 */
buffer *ows_psql_column_name(ows * o, buffer * layer_name, int number)
{
    buffer *sql;
    PGresult *res;
    buffer *column;

    assert(o);
    assert(layer_name);

    sql = buffer_init();
    column = buffer_init();

    buffer_add_str(sql, "SELECT a.attname FROM pg_class c, pg_attribute a, pg_type t WHERE c.relname ='");
    buffer_copy(sql, layer_name);
    buffer_add_str(sql, "' AND a.attnum > 0 AND a.attrelid = c.oid AND a.atttypid = t.oid AND a.attnum = ");
    buffer_add_int(sql, number);

    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return column;
    }

    buffer_add_str(column, PQgetvalue(res, 0, 0));
    PQclear(res);

    return column;
}


/*
 * Retrieve description of a table matching a given layer name
 */
array *ows_psql_describe_table(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o);
    assert(o->layers);
    assert(layer_name);

    for (ln = o->layers->first ; ln ; ln = ln->next)
        if (ln->layer->name && ln->layer->storage
            && !strcmp(ln->layer->name->buf, layer_name->buf))
            return ln->layer->storage->attributes;

    return NULL;
}


/* 
 * Retrieve an ows_version, related to current PostGIS version.
 */
ows_version * ows_psql_postgis_version(ows *o)
{
    list *l;
    PGresult * res;
    ows_version * v = NULL; 

    res = ows_psql_exec(o, "SELECT substr(postgis_full_version(), 10, 5)");
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
         PQclear(res);
         return NULL;
    }

    l = list_explode_str('.', (char *) PQgetvalue(res, 0, 0));

    if (    l->size == 3 
         && check_regexp(l->first->value->buf,       "^[0-9]+$")
         && check_regexp(l->first->next->value->buf, "^[0-9]+$")
         && check_regexp(l->last->value->buf,        "^[0-9]+$") )
        {
	    v = ows_version_init();
            v->major   = atoi(l->first->value->buf);
            v->minor   = atoi(l->first->next->value->buf);
            v->release = atoi(l->last->value->buf);
        }
 
    list_free(l);
    PQclear(res);
    return v;
}


/*
 * Convert a PostgreSql type to a valid
 * OGC XMLSchema's type
 */
char *ows_psql_to_xsd(buffer * type, ows_version *version)
{
    int wfs_version;

    assert(type);
    assert(version);

    wfs_version = ows_version_get(version); 
 
    if (buffer_case_cmp(type, "geometry")) return "gml:GeometryPropertyType";
    if (buffer_cmp(type, "geography")) return "gml:GeometryPropertyType";
    if (buffer_cmp(type, "int2")) return "short";
    if (buffer_cmp(type, "int4")) return "int";
    if (buffer_cmp(type, "int8")) return "long";
    if (buffer_cmp(type, "float4")) return "float";
    if (buffer_cmp(type, "float8")) return "double";
    if (buffer_cmp(type, "bool")) return "boolean";
    if (buffer_cmp(type, "bytea")) return "byte";
    if (buffer_cmp(type, "date")) return "date";
    if (buffer_cmp(type, "time")) return "time";
    if (buffer_ncmp(type, "numeric", 7)) return "decimal";
    if (buffer_ncmp(type, "timestamp", 9)) return "dateTime"; /* Could be also timestamptz */
    if (buffer_cmp(type, "POINT")) return "gml:PointPropertyType";
    if (buffer_cmp(type, "LINESTRING") && wfs_version == 100) return "gml:LineStringPropertyType";
    if (buffer_cmp(type, "LINESTRING") && wfs_version == 110) return "gml:CurvePropertyType";
    if (buffer_cmp(type, "POLYGON") && wfs_version == 100) return "gml:PolygonPropertyType";
    if (buffer_cmp(type, "POLYGON") && wfs_version == 110) return "gml:SurfacePropertyType";
    if (buffer_cmp(type, "MULTIPOINT")) return "gml:MultiPointPropertyType";
    if (buffer_cmp(type, "MULTILINESTRING") && wfs_version == 100) return "gml:MultiLineStringPropertyType";
    if (buffer_cmp(type, "MULTILINESTRING") && wfs_version == 110) return "gml:MultiCurvePropertyType";
    if (buffer_cmp(type, "MULTIPOLYGON") && wfs_version == 100) return "gml:MultiPolygonPropertyType";
    if (buffer_cmp(type, "MULTIPOLYGON") && wfs_version == 110) return "gml:MultiSurfacePropertyType";
    if (buffer_cmp(type, "GEOMETRYCOLLECTION")) return "gml:MultiGeometryPropertyType";

    return "string";
}


/*
 * Convert a date from PostgreSQL to XML
 */
buffer *ows_psql_timestamp_to_xml_time(char *timestamp)
{
    buffer *time;

    assert(timestamp);

    time = buffer_init();
    buffer_add_str(time, timestamp);
    if (!time->use) return time;

    buffer_replace(time, " ", "T");

    if (check_regexp(time->buf, "\\+"))
        buffer_add_str(time, ":00");
    else
        buffer_add_str(time, "Z");

    return time;
}


/*
 * Return the type of the property passed in parameter
 */
buffer *ows_psql_type(ows * o, buffer * layer_name, buffer * property)
{
    ows_layer_node *ln;

    assert(o);
    assert(o->layers);
    assert(layer_name);
    assert(property);

    for (ln = o->layers->first ; ln ; ln = ln->next) {
        if (ln->layer->name && ln->layer->storage
            && !strcmp(ln->layer->name->buf, layer_name->buf))
            return array_get(ln->layer->storage->attributes, property->buf);
    }

    return NULL;
}


/*
 * Generate a new buffer id supposed to be unique for a given layer name
 */
buffer *ows_psql_generate_id(ows * o, buffer * layer_name)
{
    ows_layer_node *ln;
    buffer * id, *sql_id;
    FILE *fp;
    PGresult * res;
    int i, seed_len;
    char * seed = NULL;

    assert(o);
    assert(o->layers);
    assert(layer_name);

    /* Retrieve layer node pointer */
    for (ln = o->layers->first ; ln ; ln = ln->next) {
        if (ln->layer->name && ln->layer->storage
            && !strcmp(ln->layer->name->buf, layer_name->buf)) break;
    }
    assert(ln);

    id = buffer_init();

    /* If PK have a sequence in PostgreSQL database,
     * retrieve next available sequence value
     */
     if (ln->layer->storage->pkey_sequence) {
         sql_id = buffer_init();
         buffer_add_str(sql_id, "SELECT nextval('");
         buffer_copy(sql_id, ln->layer->storage->pkey_sequence);
         buffer_add_str(sql_id, "');");
         res = ows_psql_exec(o, sql_id->buf);
         buffer_free(sql_id);

         if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1) {
             buffer_add_str(id, (char *) PQgetvalue(res, 0, 0));
             PQclear(res);
             return id;
         }

         /* FIXME: Shouldn't we return an error there instead ? */
         PQclear(res);
     } 
     
     /*
      * If we don't have a PostgreSQL Sequence, we will try to 
      * generate a pseudo random keystring using /dev/urandom
      * Will so work only on somes/commons Unix system
      */
     seed_len = 6;
     seed = malloc(sizeof(char) * (seed_len * 3 + 1));  /* multiply by 3 to be able to deal
                                                           with hex2dec conversion */ 
     assert(seed);
     seed[0] = '\0';

     fp = fopen("/dev/urandom","r");
     if (fp) {
         for (i=0 ; i<seed_len ; i++)
             sprintf(seed,"%s%03d", seed, fgetc(fp));
         fclose(fp);
         buffer_add_str(id, seed);
         free(seed);

         return id;
     } 
     free(seed);

    /* Case where we not using PostgreSQL sequence, 
     * and OS don't have a /dev/urandom support
     * This case don't prevent to produce ID collision
     * Don't use it unless really no others choices !!!
     */
     srand((int) (time(NULL) ^ rand() % 1000) + 42); 
     srand((rand() % 1000 ^ rand() % 1000) + 42); 
     buffer_add_int(id, rand());

     return id;
}


/*
 * Return the number of rows returned by the specified requests
 */
int ows_psql_number_features(ows * o, list * from, list * where)
{
    buffer *sql;
    PGresult *res;
    list_node *ln_from, *ln_where;
    int nb;

    assert(o);
    assert(from);
    assert(where);

    nb = 0;

    /* checks if from list and where list have the same size */
    if (from->size != where->size) return nb;


    for (ln_from = from->first, ln_where = where->first; 
	 ln_from;
         ln_from = ln_from->next, ln_where = ln_where->next) {
             sql = buffer_init();

             /* execute the request */
             buffer_add_str(sql, "SELECT count(*) FROM \"");
             buffer_copy(sql, ln_from->value);
             buffer_add_str(sql, "\" ");
             buffer_copy(sql, ln_where->value);
             res = ows_psql_exec(o, sql->buf);
             buffer_free(sql);

             if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                 PQclear(res);
                 return -1;
             }
             nb = nb + atoi(PQgetvalue(res, 0, 0));
             PQclear(res);
     }

    return nb;
}


static xmlNodePtr ows_psql_recursive_parse_gml(ows * o, xmlNodePtr n, xmlNodePtr result)
{
    xmlNodePtr c;

    assert(o);
    assert(n);

    if (result) return result;  /* avoid recursive loop */

    /* We are looking for the geometry part inside GML doc */
    for (; n ; n = n->next) {

        if (n->type != XML_ELEMENT_NODE) continue;

        /* Check on namespace GML 3 and GML 3.2 */
	if (    strcmp("http://www.opengis.net/gml",     (char *) n->ns->href)
             && strcmp("http://www.opengis.net/gml/3.2", (char *) n->ns->href)) continue;

        /* GML SF Geometries Types */
        if (   !strcmp((char *) n->name, "Point")
            || !strcmp((char *) n->name, "LineString")
            || !strcmp((char *) n->name, "LinearRing")
            || !strcmp((char *) n->name, "Curve")
            || !strcmp((char *) n->name, "Polygon")
            || !strcmp((char *) n->name, "Surface")
            || !strcmp((char *) n->name, "MultiPoint")
            || !strcmp((char *) n->name, "MultiLineString")
            || !strcmp((char *) n->name, "MultiCurve")
            || !strcmp((char *) n->name, "MultiPolygon")
            || !strcmp((char *) n->name, "MultiSurface")
            || !strcmp((char *) n->name, "MultiGeometry")) return n;

        /* Recursive exploration */
        if (n->children)
            for (c = n->children ; c ; c = c->next)
                if ((result = ows_psql_recursive_parse_gml(o, c, NULL)))
                    return result;
    }

    return NULL;
}


/*
 * Transform a GML geometry to PostGIS EWKT
 * Return NULL on error
 */
buffer * ows_psql_gml_to_sql(ows * o, xmlNodePtr n, int srid)
{
    PGresult *res;
    xmlNodePtr g;
    buffer *result, *sql, *gml;

    assert(o);
    assert(n);

    g = ows_psql_recursive_parse_gml(o, n, NULL);
    if (!g) return NULL;    /* No Geometry founded in GML doc */

    /* Retrieve the sub doc and launch GML parse via PostGIS */
    gml = buffer_init();
    cgi_add_xml_into_buffer(gml, g);
    
    sql = buffer_init();
    buffer_add_str(sql, "SELECT ST_GeomFromGML('");
    buffer_add_str(sql, gml->buf);

    if (ows_version_get(o->postgis_version) >= 200) {
       buffer_add_str(sql, "',");
       buffer_add_int(sql, srid);
       buffer_add_str(sql, ")");
    } else { 
       /* Means PostGIS 1.5 */
       buffer_add_str(sql, "')");
    }

    res = ows_psql_exec(o, sql->buf);
    buffer_free(gml);

    /* GML Parse errors cases */
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        buffer_free(sql);
        PQclear(res);
        return NULL;
    }

    result = buffer_init();
    buffer_add_str(result, PQgetvalue(res, 0, 0));
    PQclear(res);

    /* Check if geometry is valid */
    if (o->check_valid_geom) {

        buffer_empty(sql);
        buffer_add_str(sql, "SELECT ST_IsValid('");
        buffer_add_str(sql, result->buf);
        buffer_add_str(sql, "')");

        res = ows_psql_exec(o, sql->buf);

        if (    PQresultStatus(res) != PGRES_TUPLES_OK
             || PQntuples(res) != 1
             || (char) PQgetvalue(res, 0, 0)[0] !=  't') {
            buffer_free(sql);
            buffer_free(result);
            PQclear(res);
            return NULL;
        }
        PQclear(res);
    }

    buffer_free(sql);
 
    return result;
}


/* 
 * Use PostgreSQL native handling to escape string
 * string returned must be freed by the caller
 * return NULL on error
 */
char *ows_psql_escape_string(ows *o, const char *content)
{
    char *content_escaped;
    int error = 0;

    assert(o);
    assert(o->pg);
    assert(content);

    content_escaped = malloc(strlen(content) * 2 + 1);
    PQescapeStringConn(o->pg, content_escaped, content, strlen(content), &error);
    
    if (error != 0) return NULL;

    return content_escaped;
}


/*
 * Return SRID from a given geometry
 */
int ows_psql_geometry_srid(ows *o, const char *geom)
{
    int srid;
    buffer *sql;
    PGresult *res;
    
    assert(o);
    assert(o->pg);
    assert(geom);

    sql = buffer_from_str("SELECT ST_SRID('");
    buffer_add_str(sql, geom);
    buffer_add_str(sql, "'::geometry)");

    res = ows_psql_exec(o, sql->buf);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) srid = -1;
    else srid = atoi((char *) PQgetvalue(res, 0, 0));

    buffer_free(sql);
    PQclear(res);

    return srid;
}


/*
 * vim: expandtab sw=4 ts=4
 */

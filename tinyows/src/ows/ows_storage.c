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


ows_layer_storage * ows_layer_storage_init()
{
    ows_layer_storage * storage;

    storage = malloc(sizeof(ows_layer_storage));
    assert(storage);

    /* default values:  srid='-1' */
    storage->schema = buffer_init();
    storage->srid = -1;
    storage->geom_columns = list_init();
    storage->is_degree = true;
    storage->table = buffer_init();
    storage->pkey = NULL;
    storage->pkey_sequence = NULL;
    storage->pkey_column_number = -1;
    storage->attributes = array_init();
    storage->not_null_columns = NULL;

    return storage;
}


void ows_layer_storage_free(ows_layer_storage * storage)
{
    assert(storage);

    if (storage->schema)           buffer_free(storage->schema);
    if (storage->table)            buffer_free(storage->table);
    if (storage->pkey)             buffer_free(storage->pkey);
    if (storage->pkey_sequence)    buffer_free(storage->pkey_sequence);
    if (storage->geom_columns)     list_free(storage->geom_columns);
    if (storage->attributes)       array_free(storage->attributes);
    if (storage->not_null_columns) list_free(storage->not_null_columns);

    free(storage);
    storage = NULL;
}


#ifdef OWS_DEBUG
void ows_layer_storage_flush(ows_layer_storage * storage, FILE * output)
{
    assert(storage);
    assert(output);

    if (storage->schema) {
        fprintf(output, "schema: ");
        buffer_flush(storage->schema, output);
        fprintf(output, "\n");
    }

    if (storage->table) {
        fprintf(output, "table: ");
        buffer_flush(storage->table, output);
        fprintf(output, "\n");
    }

    if (storage->geom_columns) {
        fprintf(output, "geom_columns: ");
        list_flush(storage->geom_columns, output);
        fprintf(output, "\n");
    }

    fprintf(output, "srid: %i\n", storage->srid);
    fprintf(output, "is_degree: %i\n", storage->is_degree?1:0);

    if (storage->pkey) {
        fprintf(output, "pkey: ");
        buffer_flush(storage->pkey, output);
        fprintf(output, "\n");
    }

    fprintf(output, "pkey_column_number: %i\n", storage->pkey_column_number);

    if (storage->pkey_sequence) {
        fprintf(output, "pkey_sequence: ");
        buffer_flush(storage->pkey_sequence, output);
        fprintf(output, "\n");
    }

    if (storage->attributes) {
        fprintf(output, "attributes: ");
        array_flush(storage->attributes, output);
        fprintf(output, "\n");
    }

    if (storage->not_null_columns) {
        fprintf(output, "not_null_columns: ");
        list_flush(storage->not_null_columns, output);
        fprintf(output, "\n");
    }

}
#endif


/*
 * Retrieve not_null columns of a table related a given layer
 */
static void ows_storage_fill_not_null(ows * o, ows_layer * l)
{
    int i, nb_result;
    buffer *sql, *b;
    PGresult *res;

    assert(o);
    assert(l);
    assert(l->storage);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT a.attname AS field FROM pg_class c, pg_attribute a, pg_type t, pg_namespace n ");
    buffer_add_str(sql, "WHERE n.nspname = '");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND c.relname = '");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "' AND c.relnamespace = n.oid AND a.attnum > 0 AND a.attrelid = c.oid ");
    buffer_add_str(sql, "AND a.atttypid = t.oid AND a.attnotnull = 't' AND a.atthasdef='f'");

    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED, "Unable to access pg_* tables.", "not_null columns");
        return;
    }

    nb_result = PQntuples(res);
    if (nb_result) l->storage->not_null_columns = list_init();
    for (i = 0 ; i < nb_result ; i++) {
        b = buffer_init();
        buffer_add_str(b, PQgetvalue(res, i, 0));
        list_add(l->storage->not_null_columns, b);
    }

    PQclear(res);
}


/*
 * Retrieve pkey column of a table related a given layer
 * And if success try also to retrieve a related pkey sequence
 */
static void ows_storage_fill_pkey(ows * o, ows_layer * l)
{
    buffer *sql;
    PGresult *res;

    assert(o);
    assert(l);
    assert(l->storage);

    sql = buffer_init();

    buffer_add_str(sql, "SELECT c.column_name FROM information_schema.constraint_column_usage c, pg_namespace n ");
    buffer_add_str(sql, "WHERE n.nspname = '");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND c.table_name = '");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "' AND c.constraint_name = (");

    buffer_add_str(sql, "SELECT c.conname FROM pg_class r, pg_constraint c, pg_namespace n ");
    buffer_add_str(sql, "WHERE r.oid = c.conrelid AND relname = '");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "' AND r.relnamespace = n.oid AND n.nspname = '");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND c.contype = 'p')");

    res = ows_psql_exec(o, sql->buf);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        buffer_free(sql);
        ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED, "Unable to access pg_* tables.", "pkey column");
	return;
    }

    /* Layer could have no Pkey indeed... (An SQL view for example) */
    if (PQntuples(res) == 1) {
        l->storage->pkey = buffer_init();
        buffer_add_str(l->storage->pkey, PQgetvalue(res, 0, 0));
        buffer_empty(sql);
        PQclear(res);

        /* Retrieve the Pkey column number */
        buffer_add_str(sql, "SELECT a.attnum FROM pg_class c, pg_attribute a, pg_type t, pg_namespace n");
        buffer_add_str(sql, " WHERE a.attnum > 0 AND a.attrelid = c.oid AND a.atttypid = t.oid AND n.nspname='");
        buffer_copy(sql, l->storage->schema);
        buffer_add_str(sql, "' AND c.relname='");
        buffer_copy(sql, l->storage->table);
        buffer_add_str(sql, "' AND a.attname='");
        buffer_copy(sql, l->storage->pkey);
        buffer_add_str(sql, "'");
        res = ows_psql_exec(o, sql->buf);
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res);
            buffer_free(sql);
            ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED, "Unable to find pkey column number.", "pkey_column number");
	    return;
        }

        /* -1 because column number start at 1 */
        l->storage->pkey_column_number = atoi(PQgetvalue(res, 0, 0)) - 1;
        buffer_empty(sql);
        PQclear(res);

        /* Now try to find a sequence related to this Pkey */
        buffer_add_str(sql, "SELECT pg_get_serial_sequence('");
        buffer_copy(sql, l->storage->schema);
        buffer_add_str(sql, ".\"");
        buffer_copy(sql, l->storage->table);
	buffer_add_str(sql, "\"', '");
        buffer_copy(sql, l->storage->pkey);
	buffer_add_str(sql, "');");

        res = ows_psql_exec(o, sql->buf);
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res);
            buffer_free(sql);
            ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
                  "Unable to use pg_get_serial_sequence.", "pkey_sequence retrieve");
            return;
        }
        
        /* Even if no sequence found, this function return an empty row
         * so we must check that result string returned > 0 char
         */
        if (PQntuples(res) == 1 && strlen((char *) PQgetvalue(res, 0, 0)) > 0) {
            l->storage->pkey_sequence = buffer_init();
            buffer_add_str(l->storage->pkey_sequence, PQgetvalue(res, 0, 0));
        }
    }

    PQclear(res);
    buffer_free(sql);
}


/*
 * Retrieve columns name and type of a table related a given layer
 */
static void ows_storage_fill_attributes(ows * o, ows_layer * l)
{
    buffer *sql;
    PGresult *res;
    buffer *b, *t;
    int i, end;

    assert(o);
    assert(l);
    assert(l->storage);

    sql = buffer_init();

    buffer_add_str(sql, "SELECT a.attname AS field, t.typname AS type ");
    buffer_add_str(sql, "FROM pg_class c, pg_attribute a, pg_type t, pg_namespace n WHERE n.nspname = '");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND c.relname = '");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "' AND c.relnamespace = n.oid AND a.attnum > 0 AND a.attrelid = c.oid AND a.atttypid = t.oid");

    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED, "Unable to access pg_* tables.", "fill_attributes");
        return;
    }

    for (i = 0, end = PQntuples(res); i < end; i++) {
        b = buffer_init();
        t = buffer_init();
        buffer_add_str(b, PQgetvalue(res, i, 0));
	buffer_add_str(t, PQgetvalue(res, i, 1));

	/* If the column is a geometry, get its real geometry type */
	if (buffer_cmp(t, "geometry"))
	{
	  PGresult *geom_res;
	  buffer *geom_sql = buffer_init();
	  buffer_add_str(geom_sql, "SELECT type from geometry_columns where f_table_schema='");
	  buffer_copy(geom_sql, l->storage->schema);
	  buffer_add_str(geom_sql,"' and f_table_name='");
	  buffer_copy(geom_sql, l->storage->table);
	  buffer_add_str(geom_sql,"' and f_geometry_column='");
	  buffer_copy(geom_sql, b);
	  buffer_add_str(geom_sql,"';");
	  
          geom_res = ows_psql_exec(o, geom_sql->buf);
	  buffer_free(geom_sql);
	  
	  if (PQresultStatus(geom_res) != PGRES_TUPLES_OK || PQntuples(geom_res) == 0) {
	    PQclear(res);
	    PQclear(geom_res);
	    ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
		      "Unable to access geometry_columns table, try Populate_Geometry_Columns()", "fill_attributes");
	    return;
	  }
	  
	  buffer_empty(t);
	  buffer_add_str(t, PQgetvalue(geom_res, 0, 0));
          PQclear(geom_res);
	}
	
        array_add(l->storage->attributes, b, t);
    }
    PQclear(res);

}


static void ows_layer_storage_fill(ows * o, ows_layer * l, bool is_geom)
{
    buffer * sql;
    PGresult *res;
    int i, end;

    assert(o);
    assert(l);
    assert(l->storage);

    sql = buffer_init();
    if (is_geom) buffer_add_str(sql, "SELECT srid, f_geometry_column FROM geometry_columns");
    else         buffer_add_str(sql, "SELECT '4326', f_geography_column FROM geography_columns");

    buffer_add_str(sql, " WHERE f_table_schema='");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND f_table_name='");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "'");

    res = ows_psql_exec(o, sql->buf);
    buffer_empty(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);

        ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
	   "All config file layers are not availables in geometry_columns or geography_columns",
           "storage");
        return;
    }

    l->storage->srid = atoi(PQgetvalue(res, 0, 0));

    for (i = 0, end = PQntuples(res); i < end; i++)
        list_add_str(l->storage->geom_columns, PQgetvalue(res, i, 1));

    buffer_add_str(sql, "SELECT * FROM spatial_ref_sys WHERE srid=");
    buffer_add_str(sql, PQgetvalue(res, 0, 0));
    buffer_add_str(sql, " AND proj4text like '%%units=m%%'");

    PQclear(res);

    res = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    if (PQntuples(res) != 1)
        l->storage->is_degree = true;
    else
        l->storage->is_degree = false;

    PQclear(res);

    ows_storage_fill_attributes(o, l);
    ows_storage_fill_not_null(o, l);
    ows_storage_fill_pkey(o, l);
}


/*
 * Used by --check command line option
 */
void ows_layers_storage_flush(ows * o, FILE * output)
{
    ows_layer_node *ln;

    assert(o);
    assert(o->layers);

    for (ln = o->layers->first ; ln ; ln = ln->next) {
        if (ln->layer->storage) {
                fprintf(output, " - %s.%s (%i) -> %s.%s [",
			ln->layer->storage->schema->buf,
			ln->layer->storage->table->buf,
			ln->layer->storage->srid,
			ln->layer->ns_prefix->buf,
			ln->layer->name->buf);

                if (ln->layer->retrievable) fprintf(output, "R");
                if (ln->layer->writable)    fprintf(output, "W");
                fprintf(output, "]\n");
        }
    }
} 


void ows_layers_storage_fill(ows * o)
{
    PGresult *res, *res_g;
    ows_layer_node *ln;
    bool filled;
    buffer *sql;
    int i, end;

    assert(o);
    assert(o->layers);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT DISTINCT f_table_schema, f_table_name FROM geometry_columns");
    res = ows_psql_exec(o, sql->buf);
    buffer_empty(sql);

    buffer_add_str(sql, "SELECT DISTINCT f_table_schema, f_table_name FROM geography_columns");
    res_g = ows_psql_exec(o, sql->buf);
    buffer_free(sql);

    for (ln = o->layers->first ; ln ; ln = ln->next) {
        filled = false;

        for (i = 0, end = PQntuples(res); i < end; i++) {
            if (    buffer_cmp(ln->layer->storage->schema, (char *) PQgetvalue(res, i, 0)) 
		 && buffer_cmp(ln->layer->storage->table,  (char *) PQgetvalue(res, i, 1))) {
                ows_layer_storage_fill(o, ln->layer, true);
                filled = true;
            }
        }
            
        for (i = 0, end = PQntuples(res_g); i < end; i++) {
            if (    buffer_cmp(ln->layer->storage->schema, (char *) PQgetvalue(res_g, i, 0))
		 && buffer_cmp(ln->layer->storage->table,  (char *) PQgetvalue(res_g, i, 1))) {
                ows_layer_storage_fill(o, ln->layer, false);
                filled = true;
            }
        }

        if (!filled) {
            if (ln->layer->storage) ows_layer_storage_free(ln->layer->storage);
            ln->layer->storage = NULL;
        }
    }
    PQclear(res);
    PQclear(res_g);

}

/*
 * vim: expandtab sw=4 ts=4
 */

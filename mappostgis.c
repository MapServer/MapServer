/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  PostGIS CONNECTIONTYPE support.
 * Author:   Paul Ramsey <pramsey@cleverelephant.ca>
 *           Dave Blasby <dblasby@gmail.com>
 *
 ******************************************************************************
 * Copyright (c) 2008 Paul Ramsey
 * Copyright (c) 2002 Refractions Research
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

/*
** Some theory of operation:
**
** Build SQL from DATA statement and LAYER state. SQL is always of the form:
**
**    SELECT [this, that, other], geometry, uid 
**      FROM [table|(subquery) as sub] 
**      WHERE [box] AND [filter]
** 
** So the geometry always resides at layer->numitems and the uid always 
** resides at layer->numitems + 1
**
** Geometry is requested as Base64 encoded WKB. The endian is always requested
** as the client endianness.
**
** msPostGISLayerWhichShapes creates SQL based on DATA and LAYER state, 
** executes it, and places the un-read PGresult handle in the layerinfo->pgresult,
** setting the layerinfo->rownum to 0.
**
** msPostGISNextShape reads a row, increments layerinfo->rownum, and returns 
** MS_SUCCESS, until rownum reaches ntuples, and it returns MS_DONE instead.
**
*/

/* GNU needs this for strcasestr */
#define _GNU_SOURCE

#include <assert.h>
#include <string.h>
#include "mapserver.h"
#include "maptime.h"
#include "mappostgis.h"

#ifndef FLT_MAX
#define FLT_MAX 25000000.0
#endif

#ifdef USE_POSTGIS

MS_CVSID("$Id$")

/*
** msPostGISCloseConnection()
**
** Handler registered witih msConnPoolRegister so that Mapserver
** can clean up open connections during a shutdown.
*/
void msPostGISCloseConnection(void *pgconn) {
    PQfinish((PGconn*)pgconn);
}

/*
** msPostGISCreateLayerInfo()
*/
msPostGISLayerInfo *msPostGISCreateLayerInfo(void) {
    msPostGISLayerInfo *layerinfo = malloc(sizeof(msPostGISLayerInfo));
    layerinfo->sql = NULL;
    layerinfo->srid = NULL;
    layerinfo->uid = NULL;
    layerinfo->pgconn = NULL;
    layerinfo->pgresult = NULL;
    layerinfo->geomcolumn = NULL;
    layerinfo->fromsource = NULL;
    layerinfo->endian = 0;
    layerinfo->rownum = 0;
    return layerinfo;
}

/*
** msPostGISFreeLayerInfo()
*/
void msPostGISFreeLayerInfo(layerObj *layer) {
    msPostGISLayerInfo *layerinfo = NULL;
    layerinfo = (msPostGISLayerInfo*)layer->layerinfo;
    if ( layerinfo->sql ) free(layerinfo->sql);
    if ( layerinfo->uid ) free(layerinfo->uid);
    if ( layerinfo->srid ) free(layerinfo->srid);
    if ( layerinfo->geomcolumn ) free(layerinfo->geomcolumn);
    if ( layerinfo->fromsource ) free(layerinfo->fromsource);
    if ( layerinfo->pgresult ) PQclear(layerinfo->pgresult);
    if ( layerinfo->pgconn ) msConnPoolRelease(layer, layerinfo->pgconn);
    free(layerinfo);
    layer->layerinfo = NULL;
}


/*
** postgresqlNoticeHandler()
**
** Propagate messages from the database to the Mapserver log,
** set in PQsetNoticeProcessor during layer open.
*/
void postresqlNoticeHandler(void *arg, const char *message) {
    layerObj *lp;
    lp = (layerObj*)arg;

    if (lp->debug) {
        msDebug("%s\n", message);
    }
}

/*
** TODO, review and clean
*/
static int force_to_points(unsigned char *wkb, shapeObj *shape)
{
    /* we're going to make a 'line' for each entity (point, line or ring) in the geom collection */

    int     offset = 0;
    int     pt_offset;
    int     ngeoms;
    int     t, u, v;
    int     type, nrings, npoints;
    lineObj line = {0, NULL};

    shape->type = MS_SHAPE_NULL;  /* nothing in it */

    memcpy(&ngeoms, &wkb[5], 4);
    offset = 9;  /* where the first geometry is */
 
    for(t=0; t < ngeoms; t++) {
        memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

        if(type == 1) {
            /* Point */
            shape->type = MS_SHAPE_POINT;
            line.numpoints = 1;
            line.point = (pointObj *) malloc(sizeof(pointObj));

            memcpy(&line.point[0].x, &wkb[offset + 5], 8);
            memcpy(&line.point[0].y, &wkb[offset + 5 + 8], 8);
            offset += 5 + 16;
            msAddLine(shape, &line);
            free(line.point);
        } else if(type == 2) {
            /* Linestring */
            shape->type = MS_SHAPE_POINT;

            memcpy(&line.numpoints, &wkb[offset+5], 4); /* num points */
            line.point = (pointObj *) malloc(sizeof(pointObj) * line.numpoints);
            for(u = 0; u < line.numpoints; u++) {
                memcpy( &line.point[u].x, &wkb[offset+9 + (16 * u)], 8);
                memcpy( &line.point[u].y, &wkb[offset+9 + (16 * u)+8], 8);
            }
            offset += 9 + 16 * line.numpoints;  /* length of object */
            msAddLine(shape, &line);
            free(line.point);
        } else if(type == 3) {
            /* Polygon */
            shape->type = MS_SHAPE_POINT;

            memcpy(&nrings, &wkb[offset+5],4); /* num rings */
            /* add a line for each polygon ring */
            pt_offset = 0;
            offset += 9; /* now points at 1st linear ring */
            for(u = 0; u < nrings; u++) {
                /* for each ring, make a line */
                memcpy(&npoints, &wkb[offset], 4); /* num points */
                line.numpoints = npoints;
                line.point = (pointObj *) malloc(sizeof(pointObj)* npoints); 
                /* point struct */
                for(v = 0; v < npoints; v++)
                {
                    memcpy(&line.point[v].x, &wkb[offset + 4 + (16 * v)], 8);
                    memcpy(&line.point[v].y, &wkb[offset + 4 + (16 * v) + 8], 8);
                }
                /* make offset point to next linear ring */
                msAddLine(shape, &line);
                free(line.point);
                offset += 4 + 16 *npoints;
            }
        }
    }
    
    return MS_SUCCESS;
}

/* convert the wkb into lines */
/* points-> remove */
/* lines -> pass through */
/* polys -> treat rings as lines */

static int force_to_lines(unsigned char *wkb, shapeObj *shape)
{
    int     offset = 0;
    int     pt_offset;
    int     ngeoms;
    int     t, u, v;
    int     type, nrings, npoints;
    lineObj line = {0, NULL};

    shape->type = MS_SHAPE_NULL;  /* nothing in it */

    memcpy(&ngeoms, &wkb[5], 4);
    offset = 9;  /* were the first geometry is */
    for(t=0; t < ngeoms; t++) {
        memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

        /* cannot do anything with a point */

        if(type == 2) {
            /* Linestring */
            shape->type = MS_SHAPE_LINE;

            memcpy(&line.numpoints, &wkb[offset + 5], 4);
            line.point = (pointObj*) malloc(sizeof(pointObj) * line.numpoints );
            for(u=0; u < line.numpoints; u++) {
                memcpy(&line.point[u].x, &wkb[offset + 9 + (16 * u)], 8);
                memcpy(&line.point[u].y, &wkb[offset + 9 + (16 * u)+8], 8);
            }
            offset += 9 + 16 * line.numpoints;  /* length of object */
            msAddLine(shape, &line);
            free(line.point);
        } else if(type == 3) {
            /* polygon */
            shape->type = MS_SHAPE_LINE;

            memcpy(&nrings, &wkb[offset + 5], 4); /* num rings */
            /* add a line for each polygon ring */
            pt_offset = 0;
            offset += 9; /* now points at 1st linear ring */
            for(u = 0; u < nrings; u++) {
                /* for each ring, make a line */
                memcpy(&npoints, &wkb[offset], 4);
                line.numpoints = npoints;
                line.point = (pointObj*) malloc(sizeof(pointObj) * npoints); 
                /* point struct */
                for(v = 0; v < npoints; v++) {
                    memcpy(&line.point[v].x, &wkb[offset + 4 + (16 * v)], 8);
                    memcpy(&line.point[v].y, &wkb[offset + 4 + (16 * v) + 8], 8);
                }
                /* make offset point to next linear ring */
                msAddLine(shape, &line);
                free(line.point);
                offset += 4 + 16 * npoints;
            }
        }
    }

    return MS_SUCCESS;
}

/* point   -> reject */
/* line    -> reject */
/* polygon -> lines of linear rings */
static int force_to_polygons(unsigned char *wkb, shapeObj *shape)
{
    int     offset = 0;
    int     pt_offset;
    int     ngeoms;
    int     t, u, v;
    int     type, nrings, npoints;
    lineObj line = {0, NULL};

    shape->type = MS_SHAPE_NULL;  /* nothing in it */

    memcpy(&ngeoms, &wkb[5], 4);
    offset = 9;  /* were the first geometry is */
    for(t = 0; t < ngeoms; t++) {
        memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

        if(type == 3) {
            /* polygon */
            shape->type = MS_SHAPE_POLYGON;

            memcpy(&nrings, &wkb[offset + 5], 4); /* num rings */
            /* add a line for each polygon ring */
            pt_offset = 0;
            offset += 9; /* now points at 1st linear ring */
            for(u=0; u < nrings; u++) {
                /* for each ring, make a line */
                memcpy(&npoints, &wkb[offset], 4); /* num points */
                line.numpoints = npoints;
                line.point = (pointObj*) malloc(sizeof(pointObj) * npoints);
                for(v=0; v < npoints; v++) {
                    memcpy(&line.point[v].x, &wkb[offset + 4 + (16 * v)], 8);
                    memcpy(&line.point[v].y, &wkb[offset + 4 + (16 * v) + 8], 8);
                }
                /* make offset point to next linear ring */
                msAddLine(shape, &line);
                free(line.point);
                offset += 4 + 16 * npoints;
            }
        }
    }

    return MS_SUCCESS;
}

/* if there is any polygon in wkb, return force_polygon */
/* if there is any line in wkb, return force_line */
/* otherwise return force_point */

static int dont_force(unsigned char *wkb, shapeObj *shape)
{
    int     offset =0;
    int     ngeoms;
    int     type, t;
    int     best_type;

    best_type = MS_SHAPE_NULL;  /* nothing in it */

    memcpy(&ngeoms, &wkb[5], 4);
    offset = 9;  /* were the first geometry is */
    for(t = 0; t < ngeoms; t++) {
        memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

        if(type == 3) {
            best_type = MS_SHAPE_POLYGON;
        } else if(type ==2 && best_type != MS_SHAPE_POLYGON) {
            best_type = MS_SHAPE_LINE;
        } else if(type == 1 && best_type == MS_SHAPE_NULL) {
            best_type = MS_SHAPE_POINT;
        }
    }

    if(best_type == MS_SHAPE_POINT) {
        return force_to_points(wkb, shape);
    }
    if(best_type == MS_SHAPE_LINE) {
        return force_to_lines(wkb, shape);
    }
    if(best_type == MS_SHAPE_POLYGON) {
        return force_to_polygons(wkb, shape);
    }

    return MS_FAILURE; /* unknown type */
}

/* find the bounds of the shape */
static void find_bounds(shapeObj *shape)
{
    int     t, u;
    int     first_one = 1;

    for(t = 0; t < shape->numlines; t++) {
        for(u = 0; u < shape->line[t].numpoints; u++) {
            if(first_one) {
                shape->bounds.minx = shape->line[t].point[u].x;
                shape->bounds.maxx = shape->line[t].point[u].x;

                shape->bounds.miny = shape->line[t].point[u].y;
                shape->bounds.maxy = shape->line[t].point[u].y;
                first_one = 0;
            } else {
                if(shape->line[t].point[u].x < shape->bounds.minx) {
                    shape->bounds.minx = shape->line[t].point[u].x;
                }
                if(shape->line[t].point[u].x > shape->bounds.maxx) {
                    shape->bounds.maxx = shape->line[t].point[u].x;
                }

                if(shape->line[t].point[u].y < shape->bounds.miny) {
                    shape->bounds.miny = shape->line[t].point[u].y;
                }
                if(shape->line[t].point[u].y > shape->bounds.maxy) {
                    shape->bounds.maxy = shape->line[t].point[u].y;
                }
            }
        }
    }
}
/* TODO Review and clean above! */


static int msPostGISRetrievePgVersion(PGconn *pgconn) { 
#ifndef POSTGIS_HAS_SERVER_VERSION 
    int pgVersion = 0; 
    char *strVersion = NULL; 
    char *strParts[3] = { NULL, NULL, NULL }; 
    int i = 0, j = 0, len = 0; 
    int factor = 10000; 
 
    if (pgconn == NULL) { 
        msSetError(MS_QUERYERR, "Layer does not have a postgis connection.", "msPostGISRetrievePgVersion()"); 
        return(MS_FAILURE); 
    } 

    if (! PQparameterStatus(pgconn, "server_version") )
        return(MS_FAILURE); 
 
    strVersion = strdup(PQparameterStatus(pgconn, "server_version")); 
    if( ! strVersion ) 
        return MS_FAILURE; 
    strParts[j] = strVersion; 
    j++; 
    len = strlen(strVersion); 
    for( i = 0; i < len; i++ ) { 
        if( strVersion[i] == '.' ) { 
            strVersion[i] = '\0'; 
 
            if( j < 3 ) { 
                strParts[j] = strVersion + i + 1; 
                j++; 
            } 
            else { 
                free(strVersion); 
                msSetError(MS_QUERYERR, "Too many parts in version string.", "msPostGISRetrievePgVersion()"); 
                return MS_FAILURE; 
            } 
        } 
    } 
     
    for( j = 0; j < 3 && strParts[j]; j++ ) { 
        if( atoi(strParts[j]) ) { 
            pgVersion += factor * atoi(strParts[j]); 
        } 
        else { 
            free(strVersion); 
            msSetError(MS_QUERYERR, "Unable to parse version string.", "msPostGISRetrievePgVersion()"); 
            return MS_FAILURE; 
        } 
        factor = factor / 100; 
    } 
    free(strVersion); 
    return pgVersion; 
#else 
    return PQserverVersion(pgconn); 
#endif 
} 

/*
** msPostGISRetrievePK()
**
** Find out that the primary key is for this layer.
** The layerinfo->fromsource must already be populated and
** must not be a subquery.
*/
int msPostGISRetrievePK(layerObj *layer) {
    PGresult *pgresult = NULL;
    char *sql = 0;
    msPostGISLayerInfo *layerinfo = 0;
    int length;
    int pgVersion;
    char *pos_sep;
    char *schema = NULL;
    char *table = NULL;

    if (layer->debug) {
        msDebug("msPostGISRetrievePK called.\n");
    }

    layerinfo = (msPostGISLayerInfo *) layer->layerinfo;

    /* Attempt to separate fromsource into schema.table */
    pos_sep = strstr(layerinfo->fromsource, ".");
    if (pos_sep) {
        length = strlen(layerinfo->fromsource) - strlen(pos_sep);
        schema = (char*)malloc(length + 1);
        strncpy(schema, layerinfo->fromsource, length);
        schema[length] = '\0';

        length = strlen(pos_sep);
        table = (char*)malloc(length);
        strncpy(table, pos_sep + 1, length - 1);
        table[length - 1] = '\0';

        if (layer->debug) {
            msDebug("msPostGISRetrievePK(): Found schema %s, table %s.\n", schema, table);
        }
    }

    if (layerinfo->pgconn == NULL) {
        msSetError(MS_QUERYERR, "Layer does not have a postgis connection.", "msPostGISRetrievePK()");
        return(MS_FAILURE);
    }
    pgVersion = msPostGISRetrievePgVersion(layerinfo->pgconn);

    if (pgVersion < 70000) {
        if (layer->debug) {
            msDebug("msPostGISRetrievePK(): Major version below 7.\n");
        }
        return MS_FAILURE;
    }
    if (pgVersion < 70200) {
        if (layer->debug) {
            msDebug("msPostGISRetrievePK(): Version below 7.2.\n");
        }
        return MS_FAILURE;
    }
    if (pgVersion < 70300) {
        /*
        ** PostgreSQL v7.2 has a different representation of primary keys that
        ** later versions.  This currently does not explicitly exclude
        ** multicolumn primary keys.
        */
        static char *v72sql = "select b.attname from pg_class as a, pg_attribute as b, (select oid from pg_class where relname = '%s') as c, pg_index as d where d.indexrelid = a.oid and d.indrelid = c.oid and d.indisprimary and b.attrelid = a.oid and a.relnatts = 1";
        sql = malloc(strlen(layerinfo->fromsource) + strlen(v72sql));
        sprintf(sql, v72sql, layerinfo->fromsource);
    } else {
        /*
        ** PostgreSQL v7.3 and later treat primary keys as constraints.
        ** We only support single column primary keys, so multicolumn
        ** pks are explicitly excluded from the query.
        */
        if (schema && table) {
            static char *v73sql = "select attname from pg_attribute, pg_constraint, pg_class, pg_namespace where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = '%s' and pg_class.relnamespace = pg_namespace.oid and pg_namespace.nspname = '%s' and pg_constraint.conkey[2] is null";
            sql = malloc(strlen(schema) + strlen(table) + strlen(v73sql));
            sprintf(sql, v73sql, table, schema);
            free(table);
            free(schema);
        } else {
            static char *v73sql = "select attname from pg_attribute, pg_constraint, pg_class where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = '%s' and pg_table_is_visible(pg_class.oid) and pg_constraint.conkey[2] is null";
            sql = malloc(strlen(layerinfo->fromsource) + strlen(v73sql));
            sprintf(sql, v73sql, layerinfo->fromsource);
        }
    }

    if (layer->debug > 1) {
        msDebug("msPostGISRetrievePK: %s\n", sql);
    }

    layerinfo = (msPostGISLayerInfo *) layer->layerinfo;

    if (layerinfo->pgconn == NULL) {
        msSetError(MS_QUERYERR, "Layer does not have a postgis connection.", "msPostGISRetrievePK()");
        free(sql);
        return(MS_FAILURE);
    }

    pgresult = PQexecParams(layerinfo->pgconn, sql,0, NULL, NULL, NULL, NULL, 0);
    if ( !pgresult || PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        char *tmp1;
        char *tmp2 = NULL;

        tmp1 = "Error executing POSTGIS statement (msPostGISRetrievePK():";
        tmp2 = (char*)malloc(sizeof(char)*(strlen(tmp1) + strlen(sql) + 1));
        strcpy(tmp2, tmp1);
        strcat(tmp2, sql);
        msSetError(MS_QUERYERR, tmp2, "msPostGISRetrievePK()");
        free(tmp2);
        free(sql);
        return(MS_FAILURE);

    }

    if (PQntuples(pgresult) < 1) {
        if (layer->debug) {
            msDebug("msPostGISRetrievePK: No results found.\n");
        }
        PQclear(pgresult);
        free(sql);
        return MS_FAILURE;
    }
    if (PQntuples(pgresult) > 1) {
        if (layer->debug) {
            msDebug("msPostGISRetrievePK: Multiple results found.\n");
        }
        PQclear(pgresult);
        free(sql);
        return MS_FAILURE;
    }

    if (PQgetisnull(pgresult, 0, 0)) {
        if (layer->debug) {
            msDebug("msPostGISRetrievePK: Null result returned.\n");
        }
        PQclear(pgresult);
        free(sql);
        return MS_FAILURE;
    }

    layerinfo->uid = (char*)malloc(PQgetlength(pgresult, 0, 0) + 1);
    strcpy(layerinfo->uid, PQgetvalue(pgresult, 0, 0));

    PQclear(pgresult);
    free(sql);
    return MS_SUCCESS;
}


/*
** msPostGISParseData()
**
** Parse the DATA string for geometry column name, table name,
** unique id column, srid, and SQL string.
*/
int msPostGISParseData(layerObj *layer) {
    char *pos_opt, *pos_scn, *tmp, *pos_srid, *pos_uid, *data;
    int slength;
    msPostGISLayerInfo *layerinfo;

    assert(layer != NULL);
    assert(layer->layerinfo != NULL);

    layerinfo = (msPostGISLayerInfo*)(layer->layerinfo);

    if (layer->debug) {
        msDebug("msPostGISParseData called.\n");
    }

    if (!layer->data) {
        msSetError(MS_QUERYERR, "Missing DATA clause. DATA statement must contain 'geometry_column from table_name' or 'geometry_column from (sub-query) as sub'.", "msPostGISParseData()");
        return MS_FAILURE;
    }
    data = layer->data;

    /*
    ** Clean up any existing strings first, as we will be populating these fields.
    */
    if( layerinfo->srid ) {
        free(layerinfo->srid);
        layerinfo->srid = NULL;
    }
    if( layerinfo->uid ) {
        free(layerinfo->uid);
        layerinfo->uid = NULL;
    }
    if( layerinfo->geomcolumn ) {
        free(layerinfo->geomcolumn);
        layerinfo->geomcolumn = NULL;
    }
    if( layerinfo->fromsource ) {
        free(layerinfo->fromsource);
        layerinfo->fromsource = NULL;
    }
    
    /*
    ** Look for the optional ' using unique ID' string first.
    */
    pos_uid = strcasestr(data, " using unique ");
    if (pos_uid) {
        /* Find the end of this case 'using unique ftab_id using srid=33' */
        tmp = strstr(pos_uid + 14, " ");
        /* Find the end of this case 'using srid=33 using unique ftab_id' */
        if (!tmp) {
            tmp = pos_uid + strlen(pos_uid);
        }
        layerinfo->uid = (char*) malloc((tmp - (pos_uid + 14)) + 1);
        strncpy(layerinfo->uid, pos_uid + 14, tmp - (pos_uid + 14));
        (layerinfo->uid)[tmp - (pos_uid + 14)] = '\0'; /* null terminate it */
        msStringTrim(layerinfo->uid);
    }

    /*
    ** Look for the optional ' using srid=333 ' string next.
    */
    pos_srid = strcasestr(data, " using srid=");
    if (!pos_srid) {
        layerinfo->srid = (char*) malloc(1);
        (layerinfo->srid)[0] = '\0'; /* no SRID, so return just null terminator*/
    } else {
        slength = strspn(pos_srid + 12, "-0123456789");
        if (!slength) {
            msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable. You specified 'USING SRID' but didnt have any numbers! %s", "msPostGISParseData()", data);
            return MS_FAILURE;
        } else {
            layerinfo->srid = (char*) malloc(slength + 1);
            strncpy(layerinfo->srid, pos_srid + 12, slength);
            (layerinfo->srid)[slength] = '\0'; /* null terminate it */
            msStringTrim(layerinfo->srid);
        }
    }

    /*
    ** This is a little hack so the rest of the code works. 
    ** pos_opt should point to the start of the optional blocks.
    **
    ** If they are both set, return the smaller one. 
    */
    if (pos_srid && pos_uid) {
        pos_opt = (pos_srid > pos_uid) ? pos_uid : pos_srid;
    }
    /* If one or none is set, return the larger one. */
    else {
        pos_opt = (pos_srid > pos_uid) ? pos_srid : pos_uid;
    }
    /* No pos_opt? Move it to the end of the string. */
    if (!pos_opt) {
        pos_opt = data + strlen(data);
    }

    /*
    ** Scan for the 'geometry from table' or 'geometry from () as foo' clause. 
    */
    pos_scn = strcasestr(data, " from ");
    if (!pos_scn) {
        msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable. Must contain 'geometry from table' or 'geometry from (subselect) as foo'. %s", "msPostGISParseData()", data);
        return MS_FAILURE;
    }

    /* Copy the geometry column name */
    layerinfo->geomcolumn = (char*) malloc((pos_scn - data) + 1);
    strncpy(layerinfo->geomcolumn, data, pos_scn - data);
    (layerinfo->geomcolumn)[pos_scn - data] = '\0';
    msStringTrim(layerinfo->geomcolumn);

    /* Copy the table name or sub-select clause */
    layerinfo->fromsource = (char*) malloc((pos_opt - (pos_scn + 6)) + 1);
    strncpy(layerinfo->fromsource, pos_scn + 6, pos_opt - (pos_scn + 6));
    (layerinfo->fromsource)[pos_opt - (pos_scn + 6)] = '\0';
    msStringTrim(layerinfo->fromsource);

    /* Something is wrong, our goemetry column and table references are not there. */
    if (strlen(layerinfo->fromsource) < 1 || strlen(layerinfo->geomcolumn) < 1) {
        msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable.  Must contain 'geometry from table' or 'geometry from (subselect) as foo'. %s", "msPostGISParseData()", data);
        return MS_FAILURE;
    }

    /*
    ** We didn't find a ' using unique ' in the DATA string so try and find a 
    ** primary key on the table.
    */
    if ( ! (layerinfo->uid) ) {
        if ( strstr(layerinfo->fromsource, " ") ) {
            msSetError(MS_QUERYERR, "Error parsing PostGIS DATA variable.  You must specifiy 'using unique' when supplying a subselect in the data definition.", "msPostGISParseData()");
            return MS_FAILURE;
        }
        if ( msPostGISRetrievePK(layer) != MS_SUCCESS ) {
            /* No user specified unique id so we will use the PostgreSQL oid */
            /* TODO: Deprecate this, oids are deprecated in PostgreSQL */
            layerinfo->uid = strdup("oid");
        }
    }

    if (layer->debug) {
        msDebug("msPostGISParseData: unique_column=%s, srid=%s, geom_column_name=%s, table_name=%s\n", layerinfo->uid, layerinfo->srid, layerinfo->geomcolumn, layerinfo->fromsource);
    }
    return MS_SUCCESS;
}



/*
** Decode a hex character.
*/
static unsigned char msPostGISHexDecodeChar[256] = {
    /* not Base64 characters */
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    /* 0-9 */
    0,1,2,3,4,5,6,7,8,9,
    /* not Hex characters */
    64,64,64,64,64,64,64,
    /* A-F */
    10,11,12,13,14,15,
    /* not Hex characters */
    64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,10,11,12,13,14,15,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    /* not Hex characters (upper 128 characters) */
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
    };

/*
** Decode hex string "src" (null terminated) 
** into "dest" (not null terminated).
** Returns length of decoded array or 0 on failure.
*/
int msPostGISHexDecode(unsigned char *dest, const char *src, int srclen) {

    if (src && *src && (srclen % 2 == 0) ) {

        unsigned char *p = dest;
        int i;

        for ( i=0; i<srclen; i+=2 ) {
            register unsigned char b1=0, b2=0;
            register unsigned char c1 = src[i];
            register unsigned char c2 = src[i + 1];

            b1 = msPostGISHexDecodeChar[c1];
            b2 = msPostGISHexDecodeChar[c2];

            *p++ = (b1 << 4) | b2;

        }
        return(p-dest);
    }
    return 0;
}

/*
** Decode a base64 character.
*/
static unsigned char msPostGISBase64DecodeChar[256] = {
    /* not Base64 characters */
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,
    /*  +  */
    62,
    /* not Base64 characters */
    64,64,64,
    /*  /  */
    63,
    /* 0-9 */
    52,53,54,55,56,57,58,59,60,61,
    /* not Base64 characters */
    64,64,64,64,64,64,64,
    /* A-Z */
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
    /* not Base64 characters */
    64,64,64,64,64,64,
    /* a-z */
    26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    /* not Base64 characters */
    64,64,64,64,64,
    /* not Base64 characters (upper 128 characters) */
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
    };    
    
/*
** Decode base64 string "src" (null terminated) 
** into "dest" (not null terminated).
** Returns length of decoded array or 0 on failure.
*/
int msPostGISBase64Decode(unsigned char *dest, const char *src, int srclen) {

    if (src && *src) {

        unsigned char *p = dest;
        int i, j, k;
        unsigned char *buf = calloc(srclen + 1, sizeof(unsigned char));

        /* Drop illegal chars first */
        for (i=0, j=0; src[i]; i++) {
            unsigned char c = src[i];
            if ( (msPostGISBase64DecodeChar[c] != 64) || (c == '=') ) {
                buf[j++] = c;
            }
        }

        for (k=0; k<j; k+=4) {
            register unsigned char c1='A', c2='A', c3='A', c4='A';
            register unsigned char b1=0, b2=0, b3=0, b4=0;

            c1 = buf[k];

            if (k+1<j) {
                c2 = buf[k+1];
            }
            if (k+2<j) {
                c3 = buf[k+2];
            }
            if (k+3<j) {
                c4 = buf[k+3];
            }

            b1 = msPostGISBase64DecodeChar[c1];
            b2 = msPostGISBase64DecodeChar[c2];
            b3 = msPostGISBase64DecodeChar[c3];
            b4 = msPostGISBase64DecodeChar[c4];

            *p++=((b1<<2)|(b2>>4) );
            if (c3 != '=') {
                *p++=(((b2&0xf)<<4)|(b3>>2) );
            }
            if (c4 != '=') {
                *p++=(((b3&0x3)<<6)|b4 );
            }
        }
        free(buf);
        return(p-dest);
    }
    return 0;
}


/*
** msPostGISBuildSQLBox()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLBox(layerObj *layer, rectObj *rect, char *strSRID) {

    char *strBox = NULL;
    size_t sz;

    if (layer->debug) {
        msDebug("msPostGISBuildSQLBox called.\n");
    }

    if ( strSRID ) {
        static char *strBoxTemplate = "GeomFromText('POLYGON((%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g))',%s)";
        /* 10 doubles + 1 integer + template characters */
        sz = 10 * 22 + strlen(strSRID) + strlen(strBoxTemplate);
        strBox = (char*)malloc(sz+1); /* add space for terminating NULL */
        if ( sz <= snprintf(strBox, sz, strBoxTemplate,
                rect->minx, rect->miny,
                rect->minx, rect->maxy,
                rect->maxx, rect->maxy,
                rect->maxx, rect->miny,
                rect->minx, rect->miny,
                strSRID) )
	{
        	msSetError(MS_MISCERR,"Bounding box digits truncated.","msPostGISBuildSQLBox");
        	return 0;
	}
    } else {
        static char *strBoxTemplate = "GeomFromText('POLYGON((%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g))')";
        /* 10 doubles + template characters */
        sz = 10 * 22 + strlen(strBoxTemplate);
        strBox = (char*)malloc(sz+1); /* add space for terminating NULL */
        if ( sz <= snprintf(strBox, sz, strBoxTemplate,
                rect->minx, rect->miny,
                rect->minx, rect->maxy,
                rect->maxx, rect->maxy,
                rect->maxx, rect->miny,
                rect->minx, rect->miny) )
	{
        	msSetError(MS_MISCERR,"Bounding box digits truncated.","msPostGISBuildSQLBox");
        	return 0;
	}
	
    }

    return strBox;

}

/*
** msPostGISBuildSQLItems()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLItems(layerObj *layer) {

    char *strEndian = NULL;
    char *strGeom = NULL;
    char *strItems = NULL;
    msPostGISLayerInfo *layerinfo = NULL;

    if (layer->debug) {
        msDebug("msPostGISBuildSQLItems called.\n");
    }

    assert( layer->layerinfo != NULL);

    layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

    if ( ! layerinfo->geomcolumn ) {
        msSetError(MS_MISCERR, "layerinfo->geomcolumn is not initialized.", "msPostGISBuildSQLItems()");
        return 0;
    }

    /*
    ** Get the server to transform the geometry into our
    ** native endian before transmitting it to us..
    */
    if (layerinfo->endian == LITTLE_ENDIAN) {
        strEndian = "NDR";
    } else {
        strEndian = "XDR";
    }

    {
        /*
        ** We transfer the geometry from server to client as a
        ** base64 encoded WKB byte-array. We will have to decode this
        ** data once we get it. Forcing 2D removes ordinates we don't
        ** need, saving time. Forcing collection reduces the number
        ** of input types to handle.
        */

#if TRANSFER_ENCODING == 64
        static char *strGeomTemplate = "encode(AsBinary(force_collection(force_2d(\"%s\")),'%s'),'base64') as geom,\"%s\"";
#else
        static char *strGeomTemplate = "encode(AsBinary(force_collection(force_2d(\"%s\")),'%s'),'hex') as geom,\"%s\"";
#endif
        strGeom = (char*)malloc(strlen(strGeomTemplate) + strlen(strEndian) + strlen(layerinfo->geomcolumn) + strlen(layerinfo->uid));
        sprintf(strGeom, strGeomTemplate, layerinfo->geomcolumn, strEndian, layerinfo->uid);
    }

    if( layer->debug > 1 ) {
        msDebug("msPostGISBuildSQLItems: %d items requested.\n",layer->numitems);
    }

    /*
    ** Not requesting items? We just need geometry and unique id.
    */
    if (layer->numitems == 0) {
        strItems = strdup(strGeom);
    } 
    /*
    ** Build SQL to pull all the items.
    */
    else {
        int length = strlen(strGeom) + 2;
        int t;
        for ( t = 0; t < layer->numitems; t++ ) {
            length += strlen(layer->items[t]) + 3; /* itemname + "", */
        }
        strItems = (char*)malloc(length);
        strItems[0] = '\0';
        for ( t = 0; t < layer->numitems; t++ ) {
            strcat(strItems, "\"");
            strcat(strItems, layer->items[t]);
            strcat(strItems, "\",");
        }
        strcat(strItems, strGeom);
    }

    free(strGeom);
    return strItems;
}

/*
** msPostGISBuildSQLSRID()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLSRID(layerObj *layer) {

    char *strSRID = NULL;
    msPostGISLayerInfo *layerinfo = NULL;

    if (layer->debug) {
        msDebug("msPostGISBuildSQLSRID called.\n");
    }

    assert( layer->layerinfo != NULL);

    layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

    /* An SRID was already provided in the DATA line. */
    if ( layerinfo->srid && (strlen(layerinfo->srid) > 0) ) {
        strSRID = strdup(layerinfo->srid);
        if( layer->debug > 1 ) {
            msDebug("msPostGISBuildSQLSRID: SRID provided (%s)\n", strSRID);
        }
    }
    /*
    ** No SRID in data line, so extract target table from the 'fromsource'. 
    ** Either of form "thetable" (one word) or "(select ... from thetable)"
    ** or "(select ... from thetable where ...)".
    */
    else {
        char *f_table_name;
        char *strSRIDTemplate = "find_srid('','%s','%s')";
        char *pos = strstr(layerinfo->fromsource, " ");
        if( layer->debug > 1 ) {
            msDebug("msPostGISBuildSQLSRID: Building find_srid line.\n", strSRID);
        }

        if ( ! pos ) {
            /* target table is one word */
            f_table_name = strdup(layerinfo->fromsource);
            if( layer->debug > 1 ) {
                msDebug("msPostGISBuildSQLSRID: Found table (%s)\n", f_table_name);
            }
        } else {
            /* target table is hiding in sub-select clause */
            pos = strcasestr(layerinfo->fromsource, " from ");
            if ( pos ) {
                char *pos_paren;
                char *pos_space;
                pos += 6; /* should be start of table name */
                pos_paren = strstr(pos, ")"); /* first ) after table name */
                pos_space = strstr(pos, " "); /* first space after table name */
                if ( pos_space < pos_paren ) {
                    /* found space first */
                    f_table_name = (char*)malloc(pos_space - pos + 1);
                    strncpy(f_table_name, pos, pos_space - pos);
                    f_table_name[pos_space - pos] = '\0';
                } else {
                    /* found ) first */
                    f_table_name = (char*)malloc(pos_paren - pos + 1);
                    strncpy(f_table_name, pos, pos_paren - pos);
                    f_table_name[pos_paren - pos] = '\0';
                }
            } else {
                /* should not happen */
                return 0;
            }
        }
        strSRID = malloc(strlen(strSRIDTemplate) + strlen(f_table_name) + strlen(layerinfo->geomcolumn));
        sprintf(strSRID, strSRIDTemplate, f_table_name, layerinfo->geomcolumn);
        if ( f_table_name ) free(f_table_name);
    }
    return strSRID;
}

/*
** msPostGISBuildSQLFrom()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLFrom(layerObj *layer, rectObj *rect) {
    char *fromsource =0;
    char *strFrom = 0;
    static char *boxToken = "!BOX!";
    static int boxTokenLength = 5;
    msPostGISLayerInfo *layerinfo;

    if (layer->debug) {
        msDebug("msPostGISBuildSQLFrom called.\n");
    }

    assert( layer->layerinfo != NULL);

    layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

    if ( ! layerinfo->fromsource ) {
        msSetError(MS_MISCERR, "Layerinfo->fromsource is not initialized.", "msPostGISBuildSQLFrom()");
        return 0;
    }
    
    fromsource = layerinfo->fromsource;

    /*
    ** If there's a '!BOX!' in our source we need to substitute the
    ** current rectangle for it...
    */
    if ( strstr(fromsource, boxToken) && rect ) {
        char *result = NULL;
        char *strBox;
        char *strSRID;

        /* We see to set the SRID on the box, but to what SRID? */
        strSRID = msPostGISBuildSQLSRID(layer);
        if ( ! strSRID ) {
            return 0;
        }

        /* Create a suitable SQL string from the rectangle and SRID. */
        strBox = msPostGISBuildSQLBox(layer, rect, strSRID);
        if ( ! strBox ) {
            msSetError(MS_MISCERR, "Unable to build box SQL.", "msPostGISBuildSQLFrom()");
            if (strSRID) free(strSRID);
            return 0;
        }

        /* Do the substitution. */
        while ( strstr(fromsource,boxToken) ) {
            char    *start, *end;
            char    *oldresult = result;
            start = strstr(fromsource, boxToken);
            end = start + boxTokenLength;

            result = (char*)malloc((start - fromsource) + strlen(strBox) + strlen(end) +1);

            strncpy(result, fromsource, start - fromsource);
            strcpy(result + (start - fromsource), strBox);
            strcat(result, end);

            fromsource = result;
            if (oldresult != NULL)
                free(oldresult);
        }

        if (strSRID) free(strSRID);
        if (strBox) free(strBox);

    }
    /*
    ** Otherwise we can just take the fromsource "as is".
    */
    strFrom = strdup(fromsource);

    return strFrom;
}

/*
** msPostGISBuildSQLWhere()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQLWhere(layerObj *layer, rectObj *rect, long *uid) {
    char *strRect = 0;
    char *strFilter = 0;
    char *strUid = 0;
    char *strWhere = 0;
    char *strLimit = 0;
    size_t strRectLength = 0;
    size_t strFilterLength = 0;
    size_t strUidLength = 0;
    size_t strLimitLength = 0;
    int insert_and = 0;
    msPostGISLayerInfo *layerinfo;

    if (layer->debug) {
        msDebug("msPostGISBuildSQLWhere called.\n");
    }

    assert( layer->layerinfo != NULL);

    layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

    if ( ! layerinfo->fromsource ) {
        msSetError(MS_MISCERR, "Layerinfo->fromsource is not initialized.", "msPostGISBuildSQLFrom()");
        return 0;
    }

    /* Populate strLimit, if necessary. */
    if( layer->maxfeatures >= 0 ) {
        static char *strLimitTemplate = " limit %d";
        strLimit = malloc(strlen(strLimitTemplate) + 12);
        sprintf(strLimit, strLimitTemplate, layer->maxfeatures);
        strLimitLength = strlen(strLimit);
    }
    
    /* Populate strRect, if necessary. */
    if ( rect && layerinfo->geomcolumn ) {
        char *strBox = 0;
        char *strSRID = 0;
        size_t strBoxLength = 0;
        static char *strRectTemplate = "%s && %s";

        /* We see to set the SRID on the box, but to what SRID? */
        strSRID = msPostGISBuildSQLSRID(layer);
        if ( ! strSRID ) {
            return 0;
        }

        strBox = msPostGISBuildSQLBox(layer, rect, strSRID);
        if ( strBox ) {
            strBoxLength = strlen(strBox);
        }
        else {
            msSetError(MS_MISCERR, "Unable to build box SQL.", "msPostGISBuildSQLWhere()");
            return 0;
        }

        strRect = (char*)malloc(strlen(strRectTemplate) + strBoxLength + strlen(layerinfo->geomcolumn));
        sprintf(strRect, strRectTemplate, layerinfo->geomcolumn, strBox);
        strRectLength = strlen(strRect);
        if (strBox) free(strBox);
        if (strSRID) free(strSRID);
    }

    /* Populate strFilter, if necessary. */
    if ( layer->filter.string ) {
        static char *strFilterTemplate = "(%s)";
        strFilter = (char*)malloc(strlen(strFilterTemplate) + strlen(layer->filter.string));
        sprintf(strFilter, strFilterTemplate, layer->filter.string);
        strFilterLength = strlen(strFilter);
    }

    /* Populate strUid, if necessary. */
    if ( uid ) {
        static char *strUidTemplate = "\"%s\" = %ld";
        strUid = (char*)malloc(strlen(strUidTemplate) + strlen(layerinfo->uid) + 64);
        sprintf(strUid, strUidTemplate, layerinfo->uid, *uid);
        strUidLength = strlen(strUid);
    }

    strWhere = (char*)malloc(strRectLength + 5 + strFilterLength + 5 + strUidLength + strLimitLength);
    *strWhere = '\0';
    if ( strRect ) {
        strcat(strWhere, strRect);
        insert_and++;
        free(strRect);
    }
    if ( strFilter ) {
        if ( insert_and ) {
            strcat(strWhere, " and ");
        }
        strcat(strWhere, strFilter);
        free(strFilter);
        insert_and++;
    }
    if ( strUid ) {
        if ( insert_and ) {
            strcat(strWhere, " and ");
        }
        strcat(strWhere, strUid);
        free(strUid);
        insert_and++;
    }
    if ( strLimit ) {
        strcat(strWhere, strLimit);
        free(strLimit);
    }

    return strWhere;
}

/*
** msPostGISBuildSQL()
**
** Returns malloc'ed char* that must be freed by caller.
*/
char *msPostGISBuildSQL(layerObj *layer, rectObj *rect, long *uid) {

    msPostGISLayerInfo *layerinfo = 0;
    char *strFrom = 0;
    char *strItems = 0;
    char *strWhere = 0;
    char *strSQL = 0;
    static char *strSQLTemplate = "select %s from %s where %s";

    if (layer->debug) {
        msDebug("msPostGISBuildSQL called.\n");
    }

    assert( layer->layerinfo != NULL);

    layerinfo = (msPostGISLayerInfo *)layer->layerinfo;

    strItems = msPostGISBuildSQLItems(layer);
    if ( ! strItems ) {
        msSetError(MS_MISCERR, "Failed to build SQL items.", "msPostGISBuildSQL()");
        return 0;
    }

    strFrom = msPostGISBuildSQLFrom(layer, rect);
    if ( ! strFrom ) {
        msSetError(MS_MISCERR, "Failed to build SQL 'from'.", "msPostGISBuildSQL()");
        return 0;
    }

    strWhere = msPostGISBuildSQLWhere(layer, rect, uid);
    if ( ! strWhere ) {
        msSetError(MS_MISCERR, "Failed to build SQL 'where'.", "msPostGISBuildSQL()");
        return 0;
    }

    strSQL = malloc(strlen(strSQLTemplate) + strlen(strFrom) + strlen(strItems) + strlen(strWhere));
    sprintf(strSQL, strSQLTemplate, strItems, strFrom, strWhere);
    if (strItems) free(strItems);
    if (strFrom) free(strFrom);
    if (strWhere) free(strWhere);

    return strSQL;

}

int msPostGISReadShape(layerObj *layer, shapeObj *shape) {

    char *wkbstr = NULL;
    unsigned char *wkb = NULL;
    msPostGISLayerInfo *layerinfo = NULL;
    int result = 0;
    int wkbstrlen = 0;

    if (layer->debug) {
        msDebug("msPostGISReadShape called.\n");
    }

    assert(layer->layerinfo != NULL);
    layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

    /* Retrieve the geometry. */
    wkbstr = (char*)PQgetvalue(layerinfo->pgresult, layerinfo->rownum, layer->numitems );
    wkbstrlen = PQgetlength(layerinfo->pgresult, layerinfo->rownum, layer->numitems);
    
    if ( ! wkbstr ) {
        msSetError(MS_QUERYERR, "String encoded WKB returned is null!", "msPostGISReadShape()");
        return MS_FAILURE;
    }


    wkb = calloc(wkbstrlen, sizeof(char));
#if TRANSFER_ENCODING == 64
    result = msPostGISBase64Decode(wkb, wkbstr, wkbstrlen - 1);
#else
    result = msPostGISHexDecode(wkb, wkbstr, wkbstrlen);
#endif

    if( ! result ) {
        free(wkb);
        return MS_FAILURE;
    }
    
    switch (layer->type) {
        
    case MS_LAYER_POINT:
        result = force_to_points(wkb, shape);
        break;
        
    case MS_LAYER_LINE:
        result = force_to_lines(wkb, shape);
        break;
        
    case MS_LAYER_POLYGON:
        result = force_to_polygons(wkb, shape);
        break;
    
    case MS_LAYER_ANNOTATION:
    case MS_LAYER_QUERY:
    case MS_LAYER_CHART:
        result = dont_force(wkb, shape);
        break;

    case MS_LAYER_RASTER:
        msDebug( "Ignoring MS_LAYER_RASTER in msPostGISReadShape.\n" );
        break;

    case MS_LAYER_CIRCLE:
        msDebug( "Ignoring MS_LAYER_RASTER in msPostGISReadShape.\n" );
        break;

    default:
        msDebug( "Unsupported layer type in msPostGISReadShape()!\n" );
        break;
    }

    if (shape->type != MS_SHAPE_NULL) {
        int t;
        long uid;
        char *tmp;
        /* Found a drawable shape, so now retreive the attributes. */

        shape->values = (char**) malloc(sizeof(char*) * layer->numitems);
        for ( t = 0; t < layer->numitems; t++) {
            int size = PQgetlength(layerinfo->pgresult, layerinfo->rownum, t);
            char *val = (char*)PQgetvalue(layerinfo->pgresult, layerinfo->rownum, t);
            int isnull = PQgetisnull(layerinfo->pgresult, layerinfo->rownum, t);
            if ( isnull ) {
                shape->values[t] = strdup("");
            }
            else {
                shape->values[t] = (char*) malloc(size + 1);
                memcpy(shape->values[t], val, size);
                shape->values[t][size] = '\0'; /* null terminate it */
                msStringTrimBlanks(shape->values[t]);
            }
            if( layer->debug > 4 ) {
                msDebug("msPostGISReadShape: PQgetlength = %d\n", size);
            }
            if( layer->debug > 1 ) {
                msDebug("msPostGISReadShape: [%s] \"%s\"\n", layer->items[t], shape->values[t]);
            }
        }
        
        /* t is the geometry, t+1 is the uid */
        tmp = PQgetvalue(layerinfo->pgresult, layerinfo->rownum, t + 1);
        if( tmp ) {
            uid = strtol( tmp, NULL, 10 );
        }
        else {
            uid = 0;
        }
        if( layer->debug > 4 ) {
            msDebug("msPostGISReadShape: Setting shape->index = %d\n", uid);
            msDebug("msPostGISReadShape: Setting shape->tileindex = %d\n", layerinfo->rownum);
        }
        shape->index = uid;
        shape->tileindex = layerinfo->rownum;
        
        if( layer->debug > 2 ) {
            msDebug("msPostGISReadShape: [index] %d\n",  shape->index);
        }

        shape->numvalues = layer->numitems;

        find_bounds(shape);
    }
    
    if( layer->debug > 2 ) {
        char *tmp = msShapeToWKT(shape);
        msDebug("msPostGISReadShape: [shape] %s\n", tmp);
        free(tmp);
    }
    
    free(wkb);
    return MS_SUCCESS;
}

#endif /* USE_POSTGIS */


/*
** msPostGISLayerOpen()
**
** Registered vtable->LayerOpen function.
*/
int msPostGISLayerOpen(layerObj *layer) {
#ifdef USE_POSTGIS
    msPostGISLayerInfo  *layerinfo;
    int order_test = 1;

    assert(layer != NULL);

    if (layer->debug) {
        msDebug("msPostGISLayerOpen called: %s\n", layer->data);
    }

    if (layer->layerinfo) {
        if (layer->debug) {
            msDebug("msPostGISLayerOpen: Layer is already open!\n");
        }
        return MS_SUCCESS;  /* already open */
    }

    if (!layer->data) {
        msSetError(MS_QUERYERR, "Nothing specified in DATA statement.", "msPostGISLayerOpen()");
        return MS_FAILURE;
    }

    /*
    ** Initialize the layerinfo 
    **/
    layerinfo = msPostGISCreateLayerInfo();

    if (((char*) &order_test)[0] == 1) {
        layerinfo->endian = LITTLE_ENDIAN;
    } else {
        layerinfo->endian = BIG_ENDIAN;
    }

    /*
    ** Get a database connection from the pool.
    */
    layerinfo->pgconn = (PGconn *) msConnPoolRequest(layer);

    /* No connection in the pool, so set one up. */
    if (!layerinfo->pgconn) {
        char *conn_decrypted;
        if (layer->debug) {
            msDebug("msPostGISLayerOpen: No connection in pool, creating a fresh one.\n");
        }

        if (!layer->connection) {
            msSetError(MS_MISCERR, "Missing CONNECTION keyword.", "msPostGISLayerOpen()");
            return MS_FAILURE;
        }

        /*
        ** Decrypt any encrypted token in connection string and attempt to connect.
        */
        conn_decrypted = msDecryptStringTokens(layer->map, layer->connection);
        if (conn_decrypted == NULL) {
            return MS_FAILURE;  /* An error should already have been produced */
        }
        layerinfo->pgconn = PQconnectdb(conn_decrypted);
        msFree(conn_decrypted);
        conn_decrypted = NULL;

        /*
        ** Connection failed, return error message with passwords ***ed out.
        */
        if (!layerinfo->pgconn || PQstatus(layerinfo->pgconn) == CONNECTION_BAD) {
            char *index, *maskeddata;
            if (layer->debug)
                msDebug("msPostGISLayerOpen: Connection failure.\n");

            maskeddata = strdup(layer->connection);
            index = strstr(maskeddata, "password=");
            if (index != NULL) {
                index = (char*)(index + 9);
                while (*index != '\0' && *index != ' ') {
                    *index = '*';
                    index++;
                }
            }

            msSetError(MS_QUERYERR, "Database connection failed (%s) with connect string '%s'\nIs the database running? Is it allowing connections? Does the specified user exist? Is the password valid? Is the database on the standard port?", "msPostGISLayerOpen()", PQerrorMessage(layerinfo->pgconn), maskeddata);

            free(maskeddata);
            free(layerinfo);
            return MS_FAILURE;
        }

        /* Register to receive notifications from the database. */
        PQsetNoticeProcessor(layerinfo->pgconn, postresqlNoticeHandler, (void *) layer);

        /* Save this connection in the pool for later. */
        msConnPoolRegister(layer, layerinfo->pgconn, msPostGISCloseConnection);
    }
    else {
        /* Connection in the pool should be tested to see if backend is alive. */
        if( PQstatus(layerinfo->pgconn) != CONNECTION_OK ) {
            /* Uh oh, bad connection. Can we reset it? */
            PQreset(layerinfo->pgconn);
            if( PQstatus(layerinfo->pgconn) != CONNECTION_OK ) {
                /* Nope, time to bail out. */
                msSetError(MS_QUERYERR, "PostgreSQL database connection gone bad (%s)", "msPostGISLayerOpen()", PQerrorMessage(layerinfo->pgconn));
                return MS_FAILURE;
            }
            
        }
    }

    /* Save the layerinfo in the layerObj. */
    layer->layerinfo = (void*)layerinfo;

    return MS_SUCCESS;
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISLayerOpen()");
    return MS_FAILURE;
#endif
}

/*
** msPostGISLayerClose()
**
** Registered vtable->LayerClose function.
*/
int msPostGISLayerClose(layerObj *layer) {
#ifdef USE_POSTGIS

    if (layer->debug) {
        msDebug("msPostGISLayerClose called: %s\n", layer->data);
    }
    
    if( layer->layerinfo ) {
        msPostGISFreeLayerInfo(layer); 
    }

    return MS_SUCCESS;
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISLayerOpen()");
    return MS_FAILURE;
#endif
}


/*
** msPostGISLayerIsOpen()
**
** Registered vtable->LayerIsOpen function.
*/
int msPostGISLayerIsOpen(layerObj *layer) {
#ifdef USE_POSTGIS

    if (layer->debug) {
        msDebug("msPostGISLayerIsOpen called.\n");
    }

    if (layer->layerinfo)
        return MS_TRUE;
    else
        return MS_FALSE;
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISLayerIsOpen()");
    return MS_FAILURE;
#endif
}


/*
** msPostGISLayerFreeItemInfo()
**
** Registered vtable->LayerFreeItemInfo function.
*/
void msPostGISLayerFreeItemInfo(layerObj *layer) {
#ifdef USE_POSTGIS
    if (layer->debug) {
        msDebug("msPostGISLayerFreeItemInfo called.\n");
    }

    if (layer->iteminfo) {
        free(layer->iteminfo);
    }
    layer->iteminfo = NULL;
#endif
}

/*
** msPostGISLayerInitItemInfo()
**
** Registered vtable->LayerInitItemInfo function.
** Our iteminfo is list of indexes from 1..numitems.
*/
int msPostGISLayerInitItemInfo(layerObj *layer) {
#ifdef USE_POSTGIS
    int i;
    int *itemindexes ;

    if (layer->debug) {
        msDebug("msPostGISLayerInitItemInfo called.\n");
    }

    if (layer->numitems == 0) {
        return MS_SUCCESS;
    }

    if (layer->iteminfo) {
        free(layer->iteminfo);
    }

    layer->iteminfo = malloc(sizeof(int) * layer->numitems);
    if (!layer->iteminfo) {
        msSetError(MS_MEMERR, "Out of memory.", "msPostGISLayerInitItemInfo()");
        return MS_FAILURE;
    }
	
    itemindexes = (int*)layer->iteminfo;
    for (i = 0; i < layer->numitems; i++) {
        itemindexes[i] = i; /* Last item is always the geometry. The rest are non-geometry. */
    }

    return MS_SUCCESS;
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISLayerInitItemInfo()");
    return MS_FAILURE;
#endif
}

/*
** msPostGISLayerWhichShapes()
**
** Registered vtable->LayerWhichShapes function.
*/
int msPostGISLayerWhichShapes(layerObj *layer, rectObj rect) {
#ifdef USE_POSTGIS
    msPostGISLayerInfo *layerinfo = NULL;
    char *strSQL = NULL;
    PGresult *pgresult = NULL;

    assert(layer != NULL);
    assert(layer->layerinfo != NULL);

    if (layer->debug) {
        msDebug("msPostGISLayerWhichShapes called.\n");
    }

    /* Fill out layerinfo with our current DATA state. */
    if ( msPostGISParseData(layer) != MS_SUCCESS) {
        return MS_FAILURE;
    }

    /* 
    ** This comes *after* parsedata, because parsedata fills in 
    ** layer->layerinfo.
    */
    layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

    /* Build a SQL query based on our current state. */
    strSQL = msPostGISBuildSQL(layer, &rect, NULL);
    if ( ! strSQL ) {
        msSetError(MS_QUERYERR, "Failed to build query SQL.", "msPostGISLayerWhichShapes()");
        return MS_FAILURE;
    }

    if (layer->debug) {
        msDebug("msPostGISLayerWhichShapes query: %s\n", strSQL);
    }

    pgresult = PQexecParams(layerinfo->pgconn, strSQL,0, NULL, NULL, NULL, NULL, 0);

    if ( layer->debug > 1 ) {
        msDebug("msPostGISLayerWhichShapes query status: %s (%d)\n", PQresStatus(PQresultStatus(pgresult)), PQresultStatus(pgresult)); 
    }

    /* Something went wrong. */
    if (!pgresult || PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        if ( layer->debug ) {
	    msDebug("Error (%s) executing query: %s", "msPostGISLayerWhichShapes()\n", PQerrorMessage(layerinfo->pgconn), strSQL);
	}
        msSetError(MS_QUERYERR, "Error executing query: %s ", "msPostGISLayerWhichShapes()", PQerrorMessage(layerinfo->pgconn));
        free(strSQL);
        if (pgresult) {
            PQclear(pgresult);
        }
        return MS_FAILURE;
    }

    if ( layer->debug ) {
        msDebug("msPostGISLayerWhichShapes got %d records in result.\n", PQntuples(pgresult));
    }

    /* Clean any existing pgresult before storing current one. */
    if(layerinfo->pgresult) PQclear(layerinfo->pgresult);
    layerinfo->pgresult = pgresult;
    
    /* Clean any existing SQL before storing current. */
    if(layerinfo->sql) free(layerinfo->sql);
    layerinfo->sql = strSQL;
    
    layerinfo->rownum = 0;

    return MS_SUCCESS;
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISLayerWhichShapes()");
    return MS_FAILURE;
#endif
}

/*
** msPostGISLayerNextShape()
**
** Registered vtable->LayerNextShape function.
*/
int msPostGISLayerNextShape(layerObj *layer, shapeObj *shape) {
#ifdef USE_POSTGIS
    msPostGISLayerInfo  *layerinfo;

    if (layer->debug) {
        msDebug("msPostGISLayerNextShape called.\n");
    }

    assert(layer != NULL);
    assert(layer->layerinfo != NULL);

    layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

    shape->type = MS_SHAPE_NULL;

    /* 
    ** Roll through pgresult until we hit non-null shape (usually right away).
    */
    while (shape->type == MS_SHAPE_NULL) {
        if (layerinfo->rownum < PQntuples(layerinfo->pgresult)) {
            int rv;
            /* Retrieve this shape, cursor access mode. */
            rv = msPostGISReadShape(layer, shape);
            if( shape->type != MS_SHAPE_NULL ) {
                (layerinfo->rownum)++; /* move to next shape */
                return MS_SUCCESS;
            } else {
                (layerinfo->rownum)++; /* move to next shape */
            }
        } else {
            return MS_DONE;
        }
    }

    /* Found nothing, clean up and exit. */
    msFreeShape(shape);

    return MS_FAILURE;
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISLayerNextShape()");
    return MS_FAILURE;
#endif
}

/*
** msPostGISLayerResultsGetShape()
**
** Registered vtable->LayerGetShape function. For pulling from a prepared and 
** undisposed result set. We ignore the 'tile' parameter, as it means nothing to us.
*/
int msPostGISLayerResultsGetShape(layerObj *layer, shapeObj *shape, int tile, long record) {
#ifdef USE_POSTGIS
    
    PGresult *pgresult = NULL;
    msPostGISLayerInfo *layerinfo = NULL;
	int result = MS_SUCCESS;
	int status;

    assert(layer != NULL);
    assert(layer->layerinfo != NULL);

    if (layer->debug) {
        msDebug("msPostGISLayerResultsGetShape called for record = %i\n", record);
    }

    if( tile < 0 ) {
        msDebug("msPostGISLayerResultsGetShape called for record = %i\n", record);
        return msPostGISLayerResultsGetShape(layer, shape, tile, record);
    }

    layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

    /* Check the validity of the open result. */
    pgresult = layerinfo->pgresult;
    if ( ! pgresult ) {
        msSetError( MS_MISCERR,
                    "PostgreSQL result set is null.",
                    "msPostGISLayerResultsGetShape()");
        return MS_FAILURE;
    }
    status = PQresultStatus(pgresult);
    if ( layer->debug > 1 ) {
        msDebug("msPostGISLayerResultsGetShape query status: %s (%d)\n", PQresStatus(status), status);
    }    
    if( ! ( status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) ) {
        msSetError( MS_MISCERR,
                    "PostgreSQL result set is not ready.",
                    "msPostGISLayerResultsGetShape()");
        return MS_FAILURE;
    }

    /* Check the validity of the requested record number. */
    if( tile >= PQntuples(pgresult) ) {
        msDebug("msPostGISLayerResultsGetShape got request for (%d) but only has %d tuples.\n", tile, PQntuples(pgresult));
        msSetError( MS_MISCERR,
                    "Got request larger than result set.",
                    "msPostGISLayerResultsGetShape()");
        return MS_FAILURE;
    }

    layerinfo->rownum = tile; /* Only return one result. */

    /* We don't know the shape type until we read the geometry. */
    shape->type = MS_SHAPE_NULL;

	/* Return the shape, cursor access mode. */
    result = msPostGISReadShape(layer, shape);

    return (shape->type == MS_SHAPE_NULL) ? MS_FAILURE : MS_SUCCESS;
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISLayerResultsGetShape()");
    return MS_FAILURE;
#endif
}

/*
** msPostGISLayerGetShape()
**
** Registered vtable->LayerGetShape function. We ignore the 'tile' 
** parameter, as it means nothing to us.
*/
int msPostGISLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record) {
#ifdef USE_POSTGIS
    PGresult *pgresult;
    msPostGISLayerInfo *layerinfo;
    int result, num_tuples;
    char *strSQL = 0;

    assert(layer != NULL);
    assert(layer->layerinfo != NULL);

    if (layer->debug) {
        msDebug("msPostGISLayerGetShape called for record = %i\n", record);
    }

    /* Fill out layerinfo with our current DATA state. */
    if ( msPostGISParseData(layer) != MS_SUCCESS) {
        return MS_FAILURE;
    }

    /* 
    ** This comes *after* parsedata, because parsedata fills in 
    ** layer->layerinfo.
    */
    layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

    /* Build a SQL query based on our current state. */
    strSQL = msPostGISBuildSQL(layer, 0, &record);
    if ( ! strSQL ) {
        msSetError(MS_QUERYERR, "Failed to build query SQL.", "msPostGISLayerGetShape()");
        return MS_FAILURE;
    }

    if (layer->debug) {
        msDebug("msPostGISLayerGetShape query: %s\n", strSQL);
    }

    pgresult = PQexecParams(layerinfo->pgconn, strSQL,0, NULL, NULL, NULL, NULL, 0);

    /* Something went wrong. */
    if ( (!pgresult) || (PQresultStatus(pgresult) != PGRES_TUPLES_OK) ) {
        if ( layer->debug ) {
	    msDebug("Error (%s) executing SQL: %s", "msPostGISLayerGetShape()\n", PQerrorMessage(layerinfo->pgconn), strSQL );
        }            
        msSetError(MS_QUERYERR, "Error executing SQL: %s", "msPostGISLayerGetShape()", PQerrorMessage(layerinfo->pgconn));

        if (pgresult) {
            PQclear(pgresult);
        }
        free(strSQL);

        return MS_FAILURE;
    }

    /* Clean any existing pgresult before storing current one. */
    if(layerinfo->pgresult) PQclear(layerinfo->pgresult);
    layerinfo->pgresult = pgresult;

    /* Clean any existing SQL before storing current. */
    if(layerinfo->sql) free(layerinfo->sql);
    layerinfo->sql = strSQL;

    layerinfo->rownum = 0; /* Only return one result. */

    /* We don't know the shape type until we read the geometry. */
    shape->type = MS_SHAPE_NULL;

    num_tuples = PQntuples(pgresult);
    if (layer->debug) {
        msDebug("msPostGISLayerGetShape number of records: %d\n", num_tuples);
    }

    if (num_tuples > 0) {
        /* Get shape in random access mode. */
        result = msPostGISReadShape(layer, shape);
    }

    return (shape->type == MS_SHAPE_NULL) ? MS_FAILURE : ( (num_tuples > 0) ? MS_SUCCESS : MS_DONE );
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISLayerGetShape()");
    return MS_FAILURE;
#endif
}

/*
** msPostGISLayerGetItems()
**
** Registered vtable->LayerGetItems function. Query the database for
** column information about the requested layer. Rather than look in
** system tables, we just run a zero-cost query and read out of the
** result header.
*/
int msPostGISLayerGetItems(layerObj *layer) {
#ifdef USE_POSTGIS
    msPostGISLayerInfo *layerinfo;
    char *sql = 0;
    static char *strSQLTemplate = "select * from %s where false limit 0";
    PGresult *pgresult;
    int t;
    char *col;
    char found_geom = 0;
    int item_num;

    assert(layer != NULL);
    assert(layer->layerinfo != NULL);
    
    layerinfo = (msPostGISLayerInfo*) layer->layerinfo;
    
    assert(layerinfo->pgconn);

    if (layer->debug) {
        msDebug("msPostGISLayerGetItems called.\n");
    }

    /* Fill out layerinfo with our current DATA state. */
    if ( msPostGISParseData(layer) != MS_SUCCESS) {
        return MS_FAILURE;
    }

    layerinfo = (msPostGISLayerInfo*) layer->layerinfo;

    /*
    ** Both the "table" and "(select ...) as sub" cases can be handled with the 
    ** same SQL.
    */
    sql = (char*) malloc(strlen(strSQLTemplate) + strlen(layerinfo->fromsource));
    sprintf(sql, strSQLTemplate, layerinfo->fromsource);

    if (layer->debug) {
        msDebug("msPostGISLayerGetItems executing SQL: %s\n", sql);
    }

    pgresult = PQexecParams(layerinfo->pgconn, sql,0, NULL, NULL, NULL, NULL, 0);
    
    if ( (!pgresult) || (PQresultStatus(pgresult) != PGRES_TUPLES_OK) ) {
        if ( layer->debug ) {
	  msDebug("Error (%s) executing SQL: %s", "msPostGISLayerGetItems()\n", PQerrorMessage(layerinfo->pgconn), sql);
	}
        msSetError(MS_QUERYERR, "Error executing SQL: %s", "msPostGISLayerGetItems()", PQerrorMessage(layerinfo->pgconn));
        if (pgresult) {
            PQclear(pgresult);
        }
        free(sql);
        return MS_FAILURE;
    }

    free(sql);

    layer->numitems = PQnfields(pgresult) - 1; /* dont include the geometry column (last entry)*/
    layer->items = malloc(sizeof(char*) * (layer->numitems + 1)); /* +1 in case there is a problem finding geometry column */

    found_geom = 0; /* havent found the geom field */
    item_num = 0;

    for (t = 0; t < PQnfields(pgresult); t++) {
        col = PQfname(pgresult, t);
        if ( strcmp(col, layerinfo->geomcolumn) != 0 ) {
            /* this isnt the geometry column */
            layer->items[item_num] = strdup(col);
            item_num++;
        } else {
            found_geom = 1;
        }
    }

    PQclear(pgresult);

    if (!found_geom) {
        msSetError(MS_QUERYERR, "Tried to find the geometry column in the database, but couldn't find it.  Is it mis-capitalized? '%s'", "msPostGISLayerGetItems()", layerinfo->geomcolumn);
        return MS_FAILURE;
    }

    return msPostGISLayerInitItemInfo(layer);
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISLayerGetShape()");
    return MS_FAILURE;
#endif
}

/*
** msPostGISLayerGetExtent()
**
** Registered vtable->LayerGetExtent function.
**
** TODO: Update to use proper PostGIS functions to pull
** extent quickly and accurately when available.
*/
int msPostGISLayerGetExtent(layerObj *layer, rectObj *extent) {
    if (layer->debug) {
        msDebug("msPOSTGISLayerGetExtent called.\n");
    }

    extent->minx = extent->miny = -1.0 * FLT_MAX ;
    extent->maxx = extent->maxy = FLT_MAX;

    return MS_SUCCESS;

}

int msPostGISLayerSetTimeFilter(layerObj *lp, const char *timestring, const char *timefield)
{
    char *tmpstimestring = NULL;
    char *timeresolution = NULL;
    int timesresol = -1;
    char **atimes, **tokens = NULL;
    int numtimes=0,i=0,ntmp=0,nlength=0;
    char buffer[512];

    buffer[0] = '\0';

    if (!lp || !timestring || !timefield)
      return MS_FALSE;

    if( strchr(timestring,'\'') || strchr(timestring, '\\') ) {
       msSetError(MS_MISCERR, "Invalid time filter.", "msPostGISLayerSetTimeFilter()");
       return MS_FALSE;
    }

    if (strstr(timestring, ",") == NULL && 
        strstr(timestring, "/") == NULL) /* discrete time */
      tmpstimestring = strdup(timestring);
    else
    {
        atimes = msStringSplit (timestring, ',', &numtimes);
        if (atimes == NULL || numtimes < 1)
          return MS_FALSE;

        if (numtimes >= 1)
        {
            tokens = msStringSplit(atimes[0],  '/', &ntmp);
            if (ntmp == 2) /* ranges */
            {
                tmpstimestring = strdup(tokens[0]);
                msFreeCharArray(tokens, ntmp);
            }
            else if (ntmp == 1) /* multiple times */
            {
                tmpstimestring = strdup(atimes[0]);
            }
        }
        msFreeCharArray(atimes, numtimes);
    }
    if (!tmpstimestring)
      return MS_FALSE;
        
    timesresol = msTimeGetResolution((const char*)tmpstimestring);
    if (timesresol < 0)
      return MS_FALSE;

    free(tmpstimestring);

    switch (timesresol)
    {
        case (TIME_RESOLUTION_SECOND):
          timeresolution = strdup("second");
          break;

        case (TIME_RESOLUTION_MINUTE):
          timeresolution = strdup("minute");
          break;

        case (TIME_RESOLUTION_HOUR):
          timeresolution = strdup("hour");
          break;

        case (TIME_RESOLUTION_DAY):
          timeresolution = strdup("day");
          break;

        case (TIME_RESOLUTION_MONTH):
          timeresolution = strdup("month");
          break;

        case (TIME_RESOLUTION_YEAR):
          timeresolution = strdup("year");
          break;

        default:
          break;
    }

    if (!timeresolution)
      return MS_FALSE;

    /* where date_trunc('month', _cwctstamp) = '2004-08-01' */
    if (strstr(timestring, ",") == NULL && 
        strstr(timestring, "/") == NULL) /* discrete time */
    {
        if(lp->filteritem) free(lp->filteritem);
        lp->filteritem = strdup(timefield);
        if (&lp->filter)
        {
            /* if the filter is set and it's a string type, concatenate it with
               the time. If not just free it */
            if (lp->filter.type == MS_EXPRESSION)
            {
                strcat(buffer, "(");
                strcat(buffer, lp->filter.string);
                strcat(buffer, ") and ");
            }
            else
              freeExpression(&lp->filter);
        }
        

        strcat(buffer, "(");

        strcat(buffer, "date_trunc('");
        strcat(buffer, timeresolution);
        strcat(buffer, "', ");        
        strcat(buffer, timefield);
        strcat(buffer, ")");        
        
         
        strcat(buffer, " = ");
        strcat(buffer,  "'");
        strcat(buffer, timestring);
        /* make sure that the timestring is complete and acceptable */
        /* to the date_trunc function : */
        /* - if the resolution is year (2004) or month (2004-01),  */
        /* a complete string for time would be 2004-01-01 */
        /* - if the resolluion is hour or minute (2004-01-01 15), a  */
        /* complete time is 2004-01-01 15:00:00 */
        if (strcasecmp(timeresolution, "year")==0)
        {
            nlength = strlen(timestring);
            if (timestring[nlength-1] != '-')
              strcat(buffer,"-01-01");
            else
              strcat(buffer,"01-01");
        }            
        else if (strcasecmp(timeresolution, "month")==0)
        {
            nlength = strlen(timestring);
            if (timestring[nlength-1] != '-')
              strcat(buffer,"-01");
            else
              strcat(buffer,"01");
        }            
        else if (strcasecmp(timeresolution, "hour")==0)
        {
            nlength = strlen(timestring);
            if (timestring[nlength-1] != ':')
              strcat(buffer,":00:00");
            else
              strcat(buffer,"00:00");
        }            
        else if (strcasecmp(timeresolution, "minute")==0)
        {
            nlength = strlen(timestring);
            if (timestring[nlength-1] != ':')
              strcat(buffer,":00");
            else
              strcat(buffer,"00");
        }            
        

        strcat(buffer,  "'");

        strcat(buffer, ")");
        
        /* loadExpressionString(&lp->filter, (char *)timestring); */
        loadExpressionString(&lp->filter, buffer);

        free(timeresolution);
        return MS_TRUE;
    }
    
    atimes = msStringSplit (timestring, ',', &numtimes);
    if (atimes == NULL || numtimes < 1)
      return MS_FALSE;

    if (numtimes >= 1)
    {
        /* check to see if we have ranges by parsing the first entry */
        tokens = msStringSplit(atimes[0],  '/', &ntmp);
        if (ntmp == 2) /* ranges */
        {
            msFreeCharArray(tokens, ntmp);
            for (i=0; i<numtimes; i++)
            {
                tokens = msStringSplit(atimes[i],  '/', &ntmp);
                if (ntmp == 2)
                {
                    if (strlen(buffer) > 0)
                      strcat(buffer, " OR ");
                    else
                      strcat(buffer, "(");

                    strcat(buffer, "(");
                    
                    strcat(buffer, "date_trunc('");
                    strcat(buffer, timeresolution);
                    strcat(buffer, "', ");        
                    strcat(buffer, timefield);
                    strcat(buffer, ")");        
 
                    strcat(buffer, " >= ");
                    
                    strcat(buffer,  "'");

                    strcat(buffer, tokens[0]);
                    /* - if the resolution is year (2004) or month (2004-01),  */
                    /* a complete string for time would be 2004-01-01 */
                    /* - if the resolluion is hour or minute (2004-01-01 15), a  */
                    /* complete time is 2004-01-01 15:00:00 */
                    if (strcasecmp(timeresolution, "year")==0)
                    {
                        nlength = strlen(tokens[0]);
                        if (tokens[0][nlength-1] != '-')
                          strcat(buffer,"-01-01");
                        else
                          strcat(buffer,"01-01");
                    }            
                    else if (strcasecmp(timeresolution, "month")==0)
                    {
                        nlength = strlen(tokens[0]);
                        if (tokens[0][nlength-1] != '-')
                          strcat(buffer,"-01");
                        else
                          strcat(buffer,"01");
                    }            
                    else if (strcasecmp(timeresolution, "hour")==0)
                    {
                        nlength = strlen(tokens[0]);
                        if (tokens[0][nlength-1] != ':')
                          strcat(buffer,":00:00");
                        else
                          strcat(buffer,"00:00");
                    }            
                    else if (strcasecmp(timeresolution, "minute")==0)
                    {
                        nlength = strlen(tokens[0]);
                        if (tokens[0][nlength-1] != ':')
                          strcat(buffer,":00");
                        else
                          strcat(buffer,"00");
                    }            

                    strcat(buffer,  "'");
                    strcat(buffer, " AND ");

                    
                    strcat(buffer, "date_trunc('");
                    strcat(buffer, timeresolution);
                    strcat(buffer, "', ");        
                    strcat(buffer, timefield);
                    strcat(buffer, ")");  

                    strcat(buffer, " <= ");
                    
                    strcat(buffer,  "'");
                    strcat(buffer, tokens[1]);

                    /* - if the resolution is year (2004) or month (2004-01),  */
                    /* a complete string for time would be 2004-01-01 */
                    /* - if the resolluion is hour or minute (2004-01-01 15), a  */
                    /* complete time is 2004-01-01 15:00:00 */
                    if (strcasecmp(timeresolution, "year")==0)
                    {
                        nlength = strlen(tokens[1]);
                        if (tokens[1][nlength-1] != '-')
                          strcat(buffer,"-01-01");
                        else
                          strcat(buffer,"01-01");
                    }            
                    else if (strcasecmp(timeresolution, "month")==0)
                    {
                        nlength = strlen(tokens[1]);
                        if (tokens[1][nlength-1] != '-')
                          strcat(buffer,"-01");
                        else
                          strcat(buffer,"01");
                    }            
                    else if (strcasecmp(timeresolution, "hour")==0)
                    {
                        nlength = strlen(tokens[1]);
                        if (tokens[1][nlength-1] != ':')
                          strcat(buffer,":00:00");
                        else
                          strcat(buffer,"00:00");
                    }            
                    else if (strcasecmp(timeresolution, "minute")==0)
                    {
                        nlength = strlen(tokens[1]);
                        if (tokens[1][nlength-1] != ':')
                          strcat(buffer,":00");
                        else
                          strcat(buffer,"00");
                    }            

                    strcat(buffer,  "'");
                    strcat(buffer, ")");
                }
                 
                msFreeCharArray(tokens, ntmp);
            }
            if (strlen(buffer) > 0)
              strcat(buffer, ")");
        }
        else if (ntmp == 1) /* multiple times */
        {
            msFreeCharArray(tokens, ntmp);
            strcat(buffer, "(");
            for (i=0; i<numtimes; i++)
            {
                if (i > 0)
                  strcat(buffer, " OR ");

                strcat(buffer, "(");
                  
                strcat(buffer, "date_trunc('");
                strcat(buffer, timeresolution);
                strcat(buffer, "', ");        
                strcat(buffer, timefield);
                strcat(buffer, ")");   

                strcat(buffer, " = ");
                  
                strcat(buffer,  "'");

                strcat(buffer, atimes[i]);
                
                /* make sure that the timestring is complete and acceptable */
                /* to the date_trunc function : */
                /* - if the resolution is year (2004) or month (2004-01),  */
                /* a complete string for time would be 2004-01-01 */
                /* - if the resolluion is hour or minute (2004-01-01 15), a  */
                /* complete time is 2004-01-01 15:00:00 */
                if (strcasecmp(timeresolution, "year")==0)
                {
                    nlength = strlen(atimes[i]);
                    if (atimes[i][nlength-1] != '-')
                      strcat(buffer,"-01-01");
                    else
                      strcat(buffer,"01-01");
                }            
                else if (strcasecmp(timeresolution, "month")==0)
                {
                    nlength = strlen(atimes[i]);
                    if (atimes[i][nlength-1] != '-')
                      strcat(buffer,"-01");
                    else
                      strcat(buffer,"01");
                }            
                else if (strcasecmp(timeresolution, "hour")==0)
                {
                    nlength = strlen(atimes[i]);
                    if (atimes[i][nlength-1] != ':')
                      strcat(buffer,":00:00");
                    else
                      strcat(buffer,"00:00");
                }            
                else if (strcasecmp(timeresolution, "minute")==0)
                {
                    nlength = strlen(atimes[i]);
                    if (atimes[i][nlength-1] != ':')
                      strcat(buffer,":00");
                    else
                      strcat(buffer,"00");
                }            

                strcat(buffer,  "'");
                strcat(buffer, ")");
            } 
            strcat(buffer, ")");
        }
        else
        {
            msFreeCharArray(atimes, numtimes);
            return MS_FALSE;
        }

        msFreeCharArray(atimes, numtimes);

        /* load the string to the filter */
        if (strlen(buffer) > 0)
        {
            if(lp->filteritem) 
              free(lp->filteritem);
            lp->filteritem = strdup(timefield);     
            if (&lp->filter)
            {
                if (lp->filter.type == MS_EXPRESSION)
                {
                    strcat(buffer, "(");
                    strcat(buffer, lp->filter.string);
                    strcat(buffer, ") and ");
                }
                else
                  freeExpression(&lp->filter);
            }
            loadExpressionString(&lp->filter, buffer);
        }

        free(timeresolution);
        return MS_TRUE;
                 
    }
    
    return MS_FALSE;
}


char *msPostGISEscapeSQLParam(layerObj *layer, const char *pszString)
{
#ifdef USE_POSTGIS
    msPostGISLayerInfo *layerinfo = NULL;
    int nError;
    size_t nSrcLen;
    char* pszEscapedStr =NULL;

    if (layer && pszString && strlen(pszString) > 0)
    {
        if(!msPostGISLayerIsOpen(layer))
          msPostGISLayerOpen(layer);
    
        assert(layer->layerinfo != NULL);

        layerinfo = (msPostGISLayerInfo *) layer->layerinfo;
        nSrcLen = strlen(pszString);
        pszEscapedStr = (char*) malloc( 2 * nSrcLen + 1);
        PQescapeStringConn (layerinfo->pgconn, pszEscapedStr, pszString, nSrcLen, &nError);
        if (nError != 0)
        {
            free(pszEscapedStr);
            pszEscapedStr = NULL;
        }
    }
    return pszEscapedStr;
#else
    msSetError( MS_MISCERR,
                "PostGIS support is not available.",
                "msPostGISEscapeSQLParam()");
    return NULL;
#endif
}


int msPostGISLayerInitializeVirtualTable(layerObj *layer) {
    assert(layer != NULL);
    assert(layer->vtable != NULL);

    layer->vtable->LayerInitItemInfo = msPostGISLayerInitItemInfo;
    layer->vtable->LayerFreeItemInfo = msPostGISLayerFreeItemInfo;
    layer->vtable->LayerOpen = msPostGISLayerOpen;
    layer->vtable->LayerIsOpen = msPostGISLayerIsOpen;
    layer->vtable->LayerWhichShapes = msPostGISLayerWhichShapes;
    layer->vtable->LayerNextShape = msPostGISLayerNextShape;
    layer->vtable->LayerResultsGetShape = msPostGISLayerResultsGetShape; 
    layer->vtable->LayerGetShape = msPostGISLayerGetShape;
    layer->vtable->LayerClose = msPostGISLayerClose;
    layer->vtable->LayerGetItems = msPostGISLayerGetItems;
    layer->vtable->LayerGetExtent = msPostGISLayerGetExtent;
    layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;
    /* layer->vtable->LayerGetAutoStyle, not supported for this layer */
    layer->vtable->LayerCloseConnection = msPostGISLayerClose;
    layer->vtable->LayerSetTimeFilter = msPostGISLayerSetTimeFilter; 
    /* layer->vtable->LayerCreateItems, use default */
    /* layer->vtable->LayerGetNumFeatures, use default */

#ifdef USE_POSTGIS
    layer->vtable->LayerEscapeSQLParam = msPostGISEscapeSQLParam;
#endif
    return MS_SUCCESS;
}

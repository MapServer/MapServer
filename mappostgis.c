/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  PostGIS CONNECTIONTYPE support.
 * Author:   Dave Blaseby / Paul Ramsey, Refractions Research
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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

/* $Id$ */
#include <assert.h>
#include "mapserver.h"
#include "maptime.h"

#ifndef FLT_MAX
#define FLT_MAX 25000000.0
#endif

#ifdef USE_POSTGIS

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 2
#endif

#include "libpq-fe.h"
#include <string.h>
#include <ctype.h> /* tolower() */

MS_CVSID("$Id$")

typedef struct ms_POSTGIS_layer_info_t
{
    char        *sql;           /* sql query to send to DB */
    PGconn      *conn;          /* connection to db */
    long        row_num;        /* what row is the NEXT to be read (for random access) */
    PGresult    *query_result;  /* for fetching rows from the db */
    char        *urid_name;     /* name of user-specified unique identifier or OID */
    char        *user_srid;     /* zero length = calculate, non-zero means using this value! */
    int gBYTE_ORDER;            /* no longer a global variable */

    char        cursor_name[128]; /* active cursor (empty string if none) */

} msPOSTGISLayerInfo;

#define DATA_ERROR_MESSAGE \
    "%s" \
    "Error with POSTGIS data variable. You specified '%s'.\n" \
    "Standard ways of specifiying are : \n(1) 'geometry_column from geometry_table' \n(2) 'geometry_column from (sub query) as foo using unique column name using SRID=srid#' \n\n" \
    "Make sure you put in the 'using unique  column name' and 'using SRID=#' clauses in.\n\n" \
    "For more help, please see http://postgis.refractions.net/documentation/ \n\n" \
    "Mappostgis.c - version of Jan 23/2004.\n"


void postresql_NOTICE_HANDLER(void *arg, const char *message)
{
    layerObj *lp;
    lp = (layerObj*) arg;

    if (lp->debug) {
        msDebug("%s", message);
    }
}

msPOSTGISLayerInfo *getPostGISLayerInfo(const layerObj *layer)
{
    return layer->layerinfo;
}

void setPostGISLayerInfo(layerObj *layer, msPOSTGISLayerInfo *postgislayerinfo)
{
     layer->layerinfo = (void*) postgislayerinfo;
}

/* remove white space */
/* dont send in empty strings or strings with just " " in them! */
char* removeWhite(char *str)
{
    int     initial;
    char    *orig, *loc;

    initial = strspn(str, " ");
    if(initial) {
        memmove(str, str + initial, strlen(str) - initial + 1);
    }
    /* now final */
    if(strlen(str) == 0) {
        return str;
    }
    if(str[strlen(str) - 1] == ' ') {
        /* have to remove from end */
        orig = str;
        loc = &str[strlen(str) - 1];
        while((*loc = ' ') && (loc >orig)) {
            *loc = 0;
            loc--;
        }
    }
    return str;
}

/* TODO Take a look at glibc's strcasestr */
char *strstrIgnoreCase(const char *haystack, const char *needle)
{
    char    *hay_lower;
    char    *needle_lower;
    int     len_hay,len_need;
    int     t;
    char    *loc;
    int     match = -1;

    len_hay = strlen(haystack);
    len_need = strlen(needle);

    hay_lower = (char*) malloc(len_hay + 1);
    needle_lower =(char*) malloc(len_need + 1);


    for(t = 0; t < len_hay; t++) {
        hay_lower[t] = tolower(haystack[t]);
    }
    hay_lower[t] = 0;

    for(t = 0; t < len_need; t++) {
        needle_lower[t] = tolower(needle[t]);
    }
    needle_lower[t] = 0;

    loc = strstr(hay_lower, needle_lower);
    if(loc) {
        match = loc - hay_lower;
    }

    free(hay_lower);
    free(needle_lower);

    return (char *) (match < 0 ? NULL : haystack + match);
}

static int msPOSTGISLayerParseData(layerObj *layer, char **geom_column_name, char **table_name, char **urid_name, char **user_srid, int debug);

static void msPOSTGISCloseConnection(void *conn_handle)
{
    PQfinish((PGconn*) conn_handle);
}

/* takes a connection and ensures that it is in a valid transactional state */
/* performing PQreset() and/or ROLLBACK on it as necessary */
int msPOSTGISSanitizeConnection(PGconn *conn)
{
    int conn_bad = 0;
    PGTransactionStatusType trans_status;
    
    if (PQstatus(conn) == CONNECTION_BAD)
    {
        msDebug("Warning: resetting bad database connection due to PQstatus(conn) == CONNECTION_BAD in msPOSTGISSanitizeConnection()\n");
        conn_bad = 1;
    }
    else if (PQtransactionStatus(conn) == PQTRANS_UNKNOWN)
    {
	msDebug("Warning: resetting bad database connection due to PQtransactionStatus(conn) == PQTRANS_UNKNOWN in msPOSTGISSanitizeConnection()\n");
        conn_bad = 1;
    }

    /* if connection is in bad, PQreset() it */
    if (conn_bad)
    {
        PQreset(conn);
        if (PQstatus(conn) == CONNECTION_BAD)
        {
            msSetError(MS_QUERYERR, "Database connection status is CONNECTION_BAD even after attempt to PQreset() it: %s", "msPOSTGISSanitizeConnection()", PQerrorMessage(conn));
            return MS_FAILURE;
        }
        else if (PQtransactionStatus(conn) == PQTRANS_UNKNOWN) 
        {
            msSetError(MS_QUERYERR, "Database connection transaction status is PQTRANS_UNKNOWN even after attempt to PQreset() it: %s", "msPOSTGISSanitizeConnection()", PQerrorMessage(conn)); 
            return MS_FAILURE;
        }
    }

    if (PQtransactionStatus(conn) == PQTRANS_ACTIVE) /* no connection should have an active async call */
    {
        msSetError(MS_QUERYERR, "Refusing to sanitize a database connection with a pending asynchronous query (transaction status of PQTRANS_ACTIVE).", "msPOSTGISSanitizeConnection()");
	return MS_FAILURE;
    }

    trans_status = PQtransactionStatus(conn);
    if (trans_status == PQTRANS_INTRANS || trans_status == PQTRANS_INERROR) /* idle, in a transaction block or a failed transaction block */
    {
        PGresult *rb_res = PQexec(conn, "ROLLBACK");
        if (!rb_res || PQresultStatus(rb_res) != PGRES_COMMAND_OK) {
            msSetError(MS_QUERYERR, "Error executing POSTGIS ROLLBACK statement: %s", "msPOSTGISSanitizeConnection()", PQerrorMessage(conn));

            if(rb_res) {
                PQclear(rb_res);
            }

            return MS_FAILURE;
        }
        PQclear(rb_res);
    }
 
    return MS_SUCCESS;
}

/*static int gBYTE_ORDER = 0;*/

/* open up a connection to the postgresql database using the connection string in layer->connection */
/* ie. "host=192.168.50.3 user=postgres port=5555 dbname=mapserv" */
int msPOSTGISLayerOpen(layerObj *layer)
{
    msPOSTGISLayerInfo  *layerinfo;
    int                 order_test = 1;

    if(layer->debug) {
        msDebug("msPOSTGISLayerOpen called datastatement: %s\n", layer->data);
    }

    if(getPostGISLayerInfo(layer)) {
        if(layer->debug) {
            msDebug("msPOSTGISLayerOpen :: layer is already open!!\n");
        }
        return MS_SUCCESS;  /* already open */
    }

    if(!layer->data) {
        msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msPOSTGISLayerOpen()", "", "Error parsing POSTGIS data variable: nothing specified in DATA statement.\n\nMore Help:\n\n");

        return MS_FAILURE;
    }

    /* have to setup a connection to the database */

    layerinfo = (msPOSTGISLayerInfo*) malloc(sizeof(msPOSTGISLayerInfo));
    layerinfo->sql = NULL; /* calc later */
    layerinfo->row_num = 0;
    layerinfo->query_result = NULL;
    layerinfo->urid_name = NULL;
    layerinfo->user_srid = NULL;
    layerinfo->conn = NULL;
    layerinfo->gBYTE_ORDER = 0;
    layerinfo->cursor_name[0] = '\0';

    layerinfo->conn = (PGconn *) msConnPoolRequest(layer);
    if(!layerinfo->conn) {
        char *conn_decrypted;
        if(layer->debug) {
            msDebug("MSPOSTGISLayerOpen -- shared connection not available.\n");
        }

        if (!layer->connection) {
          msSetError(MS_MISCERR, "Missing CONNECTION keyword.", "msPOSTGISLayerOpen()");
          return MS_FAILURE;
        }

        /* Decrypt any encrypted token in connection and attempt to connect */
        conn_decrypted = msDecryptStringTokens(layer->map, layer->connection);
        if (conn_decrypted == NULL) {
          return(MS_FAILURE);  /* An error should already have been produced */
        }
   
        layerinfo->conn = PQconnectdb(conn_decrypted);

        msFree(conn_decrypted);
        conn_decrypted = NULL;

        if(!layerinfo->conn || PQstatus(layerinfo->conn) == CONNECTION_BAD) {
            char *index, *maskeddata;
            if (layer->debug)
                msDebug("msPOSTGISLayerOpen() FAILURE!!!\n");

            maskeddata = strdup(layer->connection);
            index = strstr(maskeddata, "password=");
            if(index != NULL) {
              index = (char *)(index + 9);
              while(*index != '\0' && *index != ' ') {
                  *index = '*';
                  index++;
              }
            }
            
            msSetError(MS_QUERYERR, "Couldn't make connection to DB with connect string '%s'.\n\nError reported was '%s'.\n\n\nThis error occured when trying to make a connection to the specified postgresql server.  \n\nMost commonly this is caused by \n(1) incorrect connection string \n(2) you didnt specify a 'user=...' in your connection string \n(3) the postmaster (postgresql server) isnt running \n(4) you are not allowing TCP/IP connection to the postmaster \n(5) your postmaster is not running on the correct port - if its not on 5432 you must specify a 'port=...' \n (6) the security on your system does not allow the webserver (usually user 'nobody') to make socket connections to the postmaster \n(7) you forgot to specify a 'host=...' if the postmaster is on a different machine\n(8) you made a typo \n  ", "msPOSTGISLayerOpen()", maskeddata, PQerrorMessage(layerinfo->conn));

            free(maskeddata);
            free(layerinfo);
            return MS_FAILURE;
        }

        msConnPoolRegister(layer, layerinfo->conn, msPOSTGISCloseConnection);

        PQsetNoticeProcessor(layerinfo->conn, postresql_NOTICE_HANDLER, (void *) layer);
    }


    if(((char *) &order_test)[0] == 1) {
        layerinfo->gBYTE_ORDER = LITTLE_ENDIAN;
    } else {
        layerinfo->gBYTE_ORDER = BIG_ENDIAN;
    }

    setPostGISLayerInfo(layer, layerinfo);
    return MS_SUCCESS;
}

/* Return MS_TRUE if layer is open, MS_FALSE otherwise. */
int msPOSTGISLayerIsOpen(layerObj *layer)
{
    return getPostGISLayerInfo(layer) ? MS_TRUE : MS_FALSE;
}


/* Free the itemindexes array in a layer. */
void msPOSTGISLayerFreeItemInfo(layerObj *layer)
{
    if(layer->debug) {
        msDebug("msPOSTGISLayerFreeItemInfo called\n");
    }

    if(layer->iteminfo) {
        free(layer->iteminfo);
    }
    layer->iteminfo = NULL;
}


/* allocate the iteminfo index array - same order as the item list */
int msPOSTGISLayerInitItemInfo(layerObj *layer)
{
    int     i;
    int     *itemindexes ;

    if (layer->debug) {
        msDebug("msPOSTGISLayerInitItemInfo called\n");
    }

    if(layer->numitems == 0) {
        return MS_SUCCESS;
    }

    if(layer->iteminfo) {
        free(layer->iteminfo);
    }

    layer->iteminfo = (int *) malloc(sizeof(int) * layer->numitems);
    if(!layer->iteminfo) {
        msSetError(MS_MEMERR, NULL, "msPOSTGISLayerInitItemInfo()");

        return MS_FAILURE;
    }

    itemindexes = (int*)layer->iteminfo;
    for(i = 0; i < layer->numitems; i++) {
        itemindexes[i] = i; /* last one is always the geometry one - the rest are non-geom */
    }

    return MS_SUCCESS;
}


/* Since we now have PostGIST 0.5, and 0.6  calling conventions, */
/* we have to attempt to handle the database in several ways.  If we do the wrong */
/* thing, then it'll throw an error and we can rollback and try again. */
/*  */
/* 2. attempt to do 0.6 calling convention (spatial ref system needed) */
/* 3. attempt to do 0.5 calling convention (no spatial ref system) */

/* The difference between 0.5 and 0.6 is that the bounding box must be */
/* declared to be in the same the same spatial reference system as the */
/* geometry column.  For 0.6, we determine the SRID of the column and then */
/* tag the bounding box as the same SRID. */

static int prepare_database(const char *geom_table, const char *geom_column, layerObj *layer, PGresult **sql_results, rectObj rect, char **query_string, const char *urid_name, const char *user_srid)
{
    msPOSTGISLayerInfo *layerinfo;
    PGresult    *result = 0, *beg_res = 0;
    char        *temp = 0;
    char        *columns_wanted = 0;
    char        *data_source = 0;
    char        *f_table_name = 0;
    char        box3d[200];         /* 19 characters + 4 doubles at 15 decimal places */
    char        *query_string_0_6 = 0;
    int         t;
    size_t      length;
    char        *pos_from, *pos_ftab, *pos_space, *pos_paren;

    layerinfo =  getPostGISLayerInfo(layer);
    
    beg_res = PQexec(layerinfo->conn, "BEGIN");
    if (!beg_res || PQresultStatus(beg_res) != PGRES_COMMAND_OK)
    {
        msSetError(MS_QUERYERR, "Error executing PostgreSQL BEGIN statement: %s", "prepare_database()", PQerrorMessage(layerinfo->conn));
        if(beg_res) {
            PQclear(beg_res);
        }
        msPOSTGISSanitizeConnection(layerinfo->conn);
        return MS_FAILURE;
    }
    PQclear(beg_res);

    /* Set the urid name */
    layerinfo->urid_name = (char *) malloc(strlen(urid_name) + 1);
    strcpy(layerinfo->urid_name, urid_name);

    /* Extract the proper f_table_name from the geom_table string.
     * We are expecting the geom_table to be either a single word
     * or a sub-select clause that possibly includes a join --
     *
     * (select column[,column[,...]] from ftab[ natural join table2]) as foo
     *
     * We are expecting whitespace or a ')' after the ftab name.
     *
     */

    pos_from = strstrIgnoreCase(geom_table, " from ");

    if(!pos_from) {
        f_table_name = (char *) malloc(strlen(geom_table) + 1);
        strcpy(f_table_name, geom_table);
    } else { /* geom_table is a sub-select clause */
        pos_ftab = pos_from + 6; /* This should be the start of the ftab name */
        pos_space = strstr(pos_ftab, " "); /* First space */

        /* TODO strrchr is POSIX and C99, rindex is neither */
#if defined(_WIN32) && !defined(__CYGWIN__)
        pos_paren = strrchr(pos_ftab, ')');
#else
        pos_paren = rindex(pos_ftab, ')');
#endif

        if(!pos_space || !pos_paren) {
            msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "prepare_database()", geom_table, "Error parsing POSTGIS data variable: Something is wrong with your subselect statement.\n\nMore Help:\n\n");

            return MS_FAILURE;
        }

        if (pos_paren < pos_space) { /* closing parenthesis preceeds any space */
            f_table_name = (char *) malloc(pos_paren - pos_ftab + 1);
            strncpy(f_table_name, pos_ftab, pos_paren - pos_ftab);
            f_table_name[pos_paren - pos_ftab] = '\0';
        } else {
            f_table_name = (char *) malloc(pos_space - pos_ftab + 1);
            strncpy(f_table_name, pos_ftab, pos_space - pos_ftab);
            f_table_name[pos_space - pos_ftab] = '\0';
        }
    }

    if(layer->numitems == 0) {
        columns_wanted = (char *) malloc(55 + strlen(geom_column) + strlen(urid_name) + 1);
        if (layerinfo->gBYTE_ORDER == LITTLE_ENDIAN) {
            sprintf(columns_wanted, "asbinary(force_collection(force_2d(%s)),'NDR'),%s::text", geom_column, urid_name);
        } else {
            sprintf(columns_wanted, "asbinary(force_collection(force_2d(%s)),'XDR'),%s::text", geom_column, urid_name);
        }
    } else {
        length = 55 + strlen(geom_column) + strlen(urid_name);
        for(t = 0; t < layer->numitems; t++) {
            length += strlen(layer->items[t]) + 9;
        }
        columns_wanted = (char *) malloc(length + 1);
        *columns_wanted = 0;
        for(t = 0; t < layer->numitems; t++) {
            strcat(columns_wanted, "\"");
            strcat(columns_wanted, layer->items[t]);
            strcat(columns_wanted, "\"");
            strcat(columns_wanted, "::text,");
        }
        temp = columns_wanted + strlen(columns_wanted);
        if(layerinfo->gBYTE_ORDER == LITTLE_ENDIAN) {
            sprintf(temp,"asbinary(force_collection(force_2d(%s)),'NDR'),%s::text", geom_column, urid_name);
        } else {
            sprintf(temp,"asbinary(force_collection(force_2d(%s)),'XDR'),%s::text", geom_column, urid_name);
        }
    }

    sprintf(box3d, "'BOX3D(%.15g %.15g,%.15g %.15g)'::BOX3D", rect.minx, rect.miny, rect.maxx, rect.maxy);

    /* substitute token '!BOX!' in geom_table with the box3d - do an unlimited # of subs */
    /* to not undo the work here, we need to make sure that data_source is malloc'd here */

     if(!strstr(geom_table, "!BOX!")) {
         data_source = (char *) malloc(strlen(geom_table) + 1);
         strcpy(data_source, geom_table);
     } else {
       char* result = NULL;
       while (strstr(geom_table,"!BOX!"))
	 {
	   /* need to do a substition */
	   char    *start, *end;
	   char    *oldresult = result;
	   start = strstr(geom_table,"!BOX!");
	   end = start+5;
	   
	   result = (char *)malloc((start - geom_table) + strlen(box3d) + strlen(end) +1);
	   
	   strncpy(result, geom_table, start - geom_table);
	   strcpy(result + (start - geom_table), box3d);
	   strcat(result, end);
	   
	   geom_table= result;
	   if (oldresult != NULL)
	     free(oldresult);
	 }
       /* if we're here, this will be a malloc'd string, so no need to 
	  copy it */
       data_source = (char *)geom_table;
     }

    /* Allocate buffer to fit the largest query string */
    query_string_0_6 = (char *) malloc(113 + 42 + strlen(columns_wanted) + strlen(data_source) + (layer->filter.string ? strlen(layer->filter.string) : 0) + 2 * strlen(geom_column) + strlen(box3d) + strlen(f_table_name) + strlen(user_srid) + 1);
    
    assert( layerinfo->cursor_name[0] == '\0' );
    strcpy( layerinfo->cursor_name, "mycursor" );

    if(!layer->filter.string) {
        if(!strlen(user_srid)) {
            sprintf(query_string_0_6, "DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s && setSRID(%s, find_srid('','%s','%s') )", columns_wanted, data_source, geom_column, box3d, f_table_name, geom_column);
        } else {
            /* use the user specified version */
            sprintf(query_string_0_6, "DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s && setSRID(%s, %s )", columns_wanted, data_source, geom_column, box3d, user_srid);
        }
    } else {
        if(!strlen(user_srid)) {
            sprintf(query_string_0_6, "DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE (%s) and (%s && setSRID( %s,find_srid('','%s','%s') ))", columns_wanted, data_source, layer->filter.string, geom_column, box3d, f_table_name, geom_column);
        } else {
            sprintf(query_string_0_6, "DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE (%s) and (%s && setSRID( %s,%s) )", columns_wanted, data_source, layer->filter.string, geom_column, box3d, user_srid);
        }
    }

    free(data_source);
    free(f_table_name);
    free(columns_wanted);

    /* set enable_seqscan=off not required (already done) */

    if(layer->debug) {
        msDebug("query_string_0_6:%s\n", query_string_0_6);
    }
    result = PQexecParams(layerinfo->conn, query_string_0_6,0, NULL, NULL, NULL, NULL, 0);

    if(!result || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
        msSetError(MS_QUERYERR, "Error declaring cursor: %s\nWith query string: %s\n", "prepare_database()", PQerrorMessage(layerinfo->conn), query_string_0_6);
        free(query_string_0_6);
        if (result)
        {
	    PQclear(result);
        }
        return MS_FAILURE;
    }
        
    *sql_results = result;

    *query_string = (char *) malloc(strlen(query_string_0_6) + 1);
    strcpy(*query_string, query_string_0_6);

    free(query_string_0_6);

    return MS_SUCCESS;
}


/* build the neccessary SQL */
/* allocate a cursor for the SQL query */
/* get ready to read from the cursor */
/*  */
/* For queries, we need to also retreive the OID for each of the rows */
/* So GetShape() can randomly access a row. */

int msPOSTGISLayerWhichShapes(layerObj *layer, rectObj rect)
{
    msPOSTGISLayerInfo  *layerinfo = 0;
    char    *query_str = 0;
    char    *table_name = 0;
    char    *geom_column_name = 0;
    char    *urid_name = 0;
    char    *user_srid = 0;
    int     set_up_result;

    if(layer->debug) {
        msDebug("msPOSTGISLayerWhichShapes called\n");
    }

    layerinfo = getPostGISLayerInfo(layer);

    if(!layerinfo) {
        /* layer not opened yet */
        msSetError(MS_QUERYERR, "msPOSTGISLayerWhichShapes called on unopened layer (layerinfo = NULL)", "msPOSTGISLayerWhichShapes()");

        return MS_FAILURE;
    }

    if(!layer->data) {
        msSetError(MS_QUERYERR, "Missing DATA clause in PostGIS Layer definition.  DATA statement must contain 'geometry_column from table_name' or 'geometry_column from (sub-query) as foo'.", "msPOSTGISLayerWhichShapes()");

        return MS_FAILURE;
    }

	if (msPOSTGISLayerParseData(layer, &geom_column_name, &table_name, &urid_name, &user_srid, layer->debug) != MS_SUCCESS) {
		return MS_FAILURE;
	}

    set_up_result = prepare_database(table_name, geom_column_name, layer, &(layerinfo->query_result), rect, &query_str, urid_name, user_srid);


    free(user_srid);
    free(urid_name);
    free(geom_column_name);
    free(table_name);

    if(set_up_result != MS_SUCCESS) {
        free(query_str);

        return set_up_result; /* relay error */
    }

    PQclear(layerinfo->query_result);

    layerinfo->sql = query_str;
    layerinfo->query_result = PQexec(layerinfo->conn, "FETCH ALL in mycursor");

    if(!layerinfo->query_result || PQresultStatus(layerinfo->query_result) != PGRES_TUPLES_OK) {
        msSetError(MS_QUERYERR, "Error executing POSTGIS SQL statement (in FETCH ALL): %s\n-%s\n", "msPOSTGISLayerWhichShapes()", query_str, PQerrorMessage(layerinfo->conn));

        if(layerinfo->query_result) {
          PQclear(layerinfo->query_result);
        }
        layerinfo->query_result = NULL;
        msPOSTGISSanitizeConnection(layerinfo->conn);

        return MS_FAILURE;
    }

    layerinfo->row_num = 0;

    return MS_SUCCESS;
}

/* Close the postgis record set and connection */
int msPOSTGISLayerClose(layerObj *layer)
{
    msPOSTGISLayerInfo  *layerinfo;
    PGresult *rollb_res;
    
    layerinfo = getPostGISLayerInfo(layer);

    if(layer->debug) {
        /* TODO Can layer->data be NULL? */
        msDebug("msPOSTGISLayerClose datastatement: %s\n", layer->data);
    }
    if(layer->debug && !layerinfo) {
        msDebug("msPOSTGISLayerClose -- layerinfo is  NULL\n");
    }

    if(layerinfo) {
        if(layerinfo->query_result) {
            if(layer->debug) {
                msDebug("msPOSTGISLayerClose -- closing query_result\n");
            }
            PQclear(layerinfo->query_result);
            layerinfo->query_result = NULL;
        } else if(layer->debug) {
            msDebug("msPOSTGISLayerClose -- query_result is NULL\n");
        }

        if( strlen(layerinfo->cursor_name) > 0 )
        {
            PGresult            *query_result;
            char                cmd_buffer[500];

            sprintf( cmd_buffer, "CLOSE %s", layerinfo->cursor_name );

            query_result = PQexec(layerinfo->conn, cmd_buffer );
            if(query_result) {
                PQclear(query_result);
            } else {
              if (msPOSTGISSanitizeConnection(layerinfo->conn) != MS_SUCCESS)
	      {
                return MS_FAILURE;
	      }
	    }

            layerinfo->cursor_name[0] = '\0';

            rollb_res = PQexec(layerinfo->conn, "ROLLBACK");
            if (!rollb_res || PQresultStatus(rollb_res) != PGRES_COMMAND_OK) {
                msSetError(MS_QUERYERR, "Error executing PostgreSQL ROLLBACK statement: %s", "msPOSTGISLayerClose()", PQerrorMessage(layerinfo->conn));
        
                if(rollb_res) {
                    PQclear(rollb_res);
                }
                msPOSTGISSanitizeConnection(layerinfo->conn);
        
                return MS_FAILURE;
            }
            PQclear(rollb_res);

        }

        msConnPoolRelease(layer, layerinfo->conn);
        layerinfo->conn = NULL;

        if(layerinfo->urid_name) {
            free(layerinfo->urid_name);
            layerinfo->urid_name = NULL;
        }

        if(layerinfo->sql) {
            free(layerinfo->sql);
            layerinfo->sql = NULL;
        }

        setPostGISLayerInfo(layer, NULL);
        free(layerinfo);
    }

    return MS_SUCCESS;
}

/* ******************************************************* */
/* wkb is assumed to be 2d (force_2d) */
/* and wkb is a GEOMETRYCOLLECTION (force_collection) */
/* and wkb is in the endian of this computer (asbinary(...,'[XN]DR')) */
/* each of the sub-geom inside the collection are point,linestring, or polygon */
/*  */
/* also, int is 32bits long */
/* double is 64bits long */
/* ******************************************************* */


/* convert the wkb into points */
/* points -> pass through */
/* lines->   constituent points */
/* polys->   treat ring like line and pull out the consituent points */

static int  force_to_points(char *wkb, shapeObj *shape)
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

static int  force_to_lines(char *wkb, shapeObj *shape)
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
static int  force_to_polygons(char  *wkb, shapeObj *shape)
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

static int  dont_force(char *wkb, shapeObj *shape)
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


/* find the next shape with the appropriate shape type (convert it if necessary) */
/* also, load in the attribute data */
/* MS_DONE => no more data */

int msPOSTGISLayerNextShape(layerObj *layer, shapeObj *shape)
{
    int     result;

    msPOSTGISLayerInfo  *layerinfo;

    layerinfo = getPostGISLayerInfo(layer);

    if(!layerinfo) {
        msSetError(MS_QUERYERR, "NextShape called with layerinfo = NULL", "msPOSTGISLayerNextShape()");

        return MS_FAILURE;
    }

    result = msPOSTGISLayerGetShapeRandom(layer, shape, &(layerinfo->row_num));
    /* getshaperandom will increment the row_num */
    /* layerinfo->row_num   ++; */

    return result;
}



/* Used by NextShape() to access a shape in the query set */
/* TODO: only fetch 1000 rows at a time.  This should check to see if the */
/* requested feature is in the set.  If it is, return it, otherwise */
/* grab the next 1000 rows. */
int msPOSTGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
    msPOSTGISLayerInfo  *layerinfo;
    char                *wkb;
    int                 result, t, size;
    char                *temp, *temp2;
    long                record_oid;


    layerinfo = getPostGISLayerInfo(layer);

    if(!layerinfo) {
        msSetError(MS_QUERYERR, "GetShape called with layerinfo = NULL", "msPOSTGISLayerGetShape()");

        return MS_FAILURE;
    }

    if(!layerinfo->conn) {
        msSetError(MS_QUERYERR, "NextShape called on POSTGIS layer with no connection to DB.", "msPOSTGISLayerGetShape()");

        return MS_FAILURE;
    }

    if(!layerinfo->query_result) {
        msSetError(MS_QUERYERR, "GetShape called on POSTGIS layer with invalid DB query results.", "msPOSTGISLayerGetShapeRandom()");

        return MS_FAILURE;
    }

    shape->type = MS_SHAPE_NULL;

    while(shape->type == MS_SHAPE_NULL) {
        if(*record < PQntuples(layerinfo->query_result)) {
            /* retreive an item */
            wkb = (char *)PQgetvalue(layerinfo->query_result, (*record), layer->numitems);
            switch(layer->type) {
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
                    msDebug( "Ignoring MS_LAYER_RASTER in mappostgis.c\n" );
                    break;

                case MS_LAYER_CIRCLE:
                    msDebug( "Ignoring MS_LAYER_RASTER in mappostgis.c\n" );
                    break;

                default:
                   msDebug( "Unsupported layer type in msPOSTGISLayerNextShape()!" );
                   break;
            }

            if(shape->type != MS_SHAPE_NULL) {
                /* have to retreive the attributes */
                shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
                for(t=0; t < layer->numitems; t++) {
                     temp = (char *) PQgetvalue(layerinfo->query_result, *record, t);
                     size = PQgetlength(layerinfo->query_result, *record, t);
                     temp2 = (char *) malloc(size + 1);
                     memcpy(temp2, temp, size);
                     temp2[size] = 0; /* null terminate it */

                     shape->values[t] = temp2;
                }
                temp = (char *) PQgetvalue(layerinfo->query_result, *record, t + 1); /* t is WKB, t+1 is OID */
                record_oid = strtol(temp, NULL, 10);

                shape->index = record_oid;
                shape->numvalues = layer->numitems;

                find_bounds(shape);
                (*record)++;        /* move to next shape */
                return MS_SUCCESS;
            } else {
                (*record)++; /* move to next shape */
            }
        } else {
            return MS_DONE;
        }
    }

    msFreeShape(shape);

    return MS_FAILURE;
}


/* Execute a query on the DB based on record being an OID. */

int msPOSTGISLayerGetShape(layerObj *layer, shapeObj *shape, long record)
{
    char    *query_str;
    char    *table_name = 0;
    char    *geom_column_name = 0;
    char    *urid_name = 0;
    char    *user_srid = 0;
    char    *columns_wanted = 0;
    size_t  length;
    char    *temp;

    PGresult            *query_result, *beg_res, *rollb_res, *close_res;
    msPOSTGISLayerInfo  *layerinfo;
    char                *wkb;
    int                 result, t, size, num_tuples;
    char                *temp1, *temp2;

    if(layer->debug) {
        msDebug("msPOSTGISLayerGetShape called for record = %i\n", record);
    }

    layerinfo = getPostGISLayerInfo(layer);

    if(!layerinfo) {
        /* Layer not open */
        msSetError(MS_QUERYERR, "msPOSTGISLayerGetShape called on unopened layer (layerinfo = NULL)", "msPOSTGISLayerGetShape()");

        return MS_FAILURE;
    }
    
    beg_res = PQexec(layerinfo->conn, "BEGIN");
    if (!beg_res || PQresultStatus(beg_res) != PGRES_COMMAND_OK)
    {
        msSetError(MS_QUERYERR, "Error executing PostgreSQL BEGIN statement: %s", "msPOSTGISLayerGetShape()", PQerrorMessage(layerinfo->conn));
        if(beg_res)
	{
            PQclear(beg_res);
        }
        msPOSTGISSanitizeConnection(layerinfo->conn);
        return MS_FAILURE;
    }
    PQclear(beg_res);

    if (msPOSTGISLayerParseData(layer, &geom_column_name, &table_name, &urid_name, &user_srid, layer->debug) != MS_SUCCESS) {
		return MS_FAILURE;
	}

    if(layer->numitems == 0) {
        /* Don't need the oid since its really record */
        columns_wanted = (char *) malloc(46 + strlen(geom_column_name) + 1);
        if(layerinfo->gBYTE_ORDER == LITTLE_ENDIAN) {
            sprintf(columns_wanted, "asbinary(force_collection(force_2d(%s)),'NDR')", geom_column_name);
        } else {
            sprintf(columns_wanted, "asbinary(force_collection(force_2d(%s)),'XDR')", geom_column_name);
        }
    } else {
        length = 46 + strlen(geom_column_name);
        for(t = 0; t < layer->numitems; t++) {
            length += strlen(layer->items[t]) + 9;
        }
        columns_wanted = (char *) malloc(length + 1);
        *columns_wanted = 0;
        for(t = 0; t < layer->numitems; t++) {
            strcat(columns_wanted, "\"");
            strcat(columns_wanted, layer->items[t]);
            strcat(columns_wanted, "\"");
            strcat(columns_wanted, "::text,");
        }
        temp = columns_wanted + strlen(columns_wanted);
        if(layerinfo->gBYTE_ORDER == LITTLE_ENDIAN) {
            sprintf(temp, "asbinary(force_collection(force_2d(%s)),'NDR')", geom_column_name);
        } else {
            sprintf(temp, "asbinary(force_collection(force_2d(%s)),'XDR')", geom_column_name);
        }
    }

    /* TODO Can a long always hold 3 * sizeof(long) decimal digits? */
    query_str = (char *) malloc(68 + strlen(columns_wanted) + strlen(table_name) + strlen(urid_name) + 3 * sizeof(long) + 1);
    sprintf(query_str, "DECLARE mycursor2 BINARY CURSOR FOR SELECT %s from %s WHERE %s = %li", columns_wanted, table_name, urid_name, record);

    if(layer->debug) {
        msDebug("msPOSTGISLayerGetShape: %s \n", query_str);
    }

    /* Cleanup parsed query parts */
    free(columns_wanted);
    free(user_srid);
    free(urid_name);
    free(geom_column_name);
    free(table_name);

    query_result = PQexecParams(layerinfo->conn, query_str,0, NULL, NULL, NULL, NULL, 0);

    if(!query_result || PQresultStatus(query_result) != PGRES_COMMAND_OK) {
        msSetError(MS_QUERYERR, "Error executing POSTGIS SQL statement (in FETCH ALL): %s\n-%s\nMore Help:", "msPOSTGISLayerGetShape()", query_str, PQerrorMessage(layerinfo->conn));

        if(query_result) {
          PQclear(query_result);
        }
        msPOSTGISSanitizeConnection(layerinfo->conn);

        free(query_str);

        return MS_FAILURE;

    }
    PQclear(query_result);

    query_result = PQexec(layerinfo->conn, "FETCH ALL in mycursor2");
    if( !query_result || PQresultStatus(query_result) !=  PGRES_TUPLES_OK) {
        msSetError(MS_QUERYERR, "Error executing POSTGIS SQL statement (in FETCH ALL): %s\n-%s\n", "msPOSTGISLayerGetShape()", query_str, PQerrorMessage(layerinfo->conn));

        if(query_result) {
          PQclear(query_result);
        }
        msPOSTGISSanitizeConnection(layerinfo->conn);

        free(query_str);

        return MS_FAILURE;
    }

    free(query_str);


    /* query has been done, so we can retreive the results */
    shape->type = MS_SHAPE_NULL;

    num_tuples = PQntuples(query_result);
    if(0 < num_tuples) {
        /* only need to get one shape */
        /* retreive an item */
        wkb = (char *) PQgetvalue(query_result, 0, layer->numitems);  
        /* layer->numitems is the wkt column */
        switch(layer->type) {
            case MS_LAYER_POINT:
                result = force_to_points(wkb, shape);
                break;

            case MS_LAYER_LINE:
                result = force_to_lines(wkb, shape);
                break;

            case MS_LAYER_POLYGON:
                result =    force_to_polygons(wkb, shape);
                break;

            case MS_LAYER_ANNOTATION:
            case MS_LAYER_QUERY:
            case MS_LAYER_CHART:
                result = dont_force(wkb,shape);
                break;

            case MS_LAYER_RASTER:
                msDebug("Ignoring MS_LAYER_RASTER in mappostgis.c\n");
                break;

            case MS_LAYER_CIRCLE:
                msDebug("Ignoring MS_LAYER_RASTER in mappostgis.c\n");
                break;

            default:
                msDebug("Ignoring unrecognised layer type.");
                break;
        }

        if(shape->type != MS_SHAPE_NULL) {
            /* have to retreive the attributes */
            shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
            for(t = 0; t < layer->numitems; t++) {
                 temp1= (char *) PQgetvalue(query_result, 0, t);
                 size = PQgetlength(query_result, 0, t);
                 temp2 = (char *) malloc(size + 1);
                 memcpy(temp2, temp1, size);
                 temp2[size] = 0; /* null terminate it */

                 shape->values[t] = temp2;

            }
            shape->index = record;
            shape->numvalues = layer->numitems;

            find_bounds(shape);
        }
    }

    PQclear(query_result);

    close_res = PQexec(layerinfo->conn, "CLOSE mycursor2");
    if (!close_res || PQresultStatus(close_res) != PGRES_COMMAND_OK)
    {
        msSetError(MS_QUERYERR, "Error executing PostgreSQL CLOSE statement.", "msPOSTGISLayerGetShape()");
        if (num_tuples > 0 && shape->type != MS_SHAPE_NULL)
        {
            msFreeShape(shape);
        }
        if (close_res)
        {
            PQclear(close_res);
        }
        msPOSTGISSanitizeConnection(layerinfo->conn);
        return MS_FAILURE;
    }
    PQclear(close_res);

    rollb_res = PQexec(layerinfo->conn, "ROLLBACK");
    if (!rollb_res || PQresultStatus(rollb_res) != PGRES_COMMAND_OK)
    {
        msSetError(MS_QUERYERR, "Error executing PostgreSQL ROLLBACK statement: %s", "msPOSTGISLayerClose()", PQerrorMessage(layerinfo->conn));
        if(rollb_res)
	{
            PQclear(rollb_res);
        }
        msPOSTGISSanitizeConnection(layerinfo->conn);
        return MS_FAILURE;
    }
    PQclear(rollb_res);

    return (shape->type == MS_SHAPE_NULL) ? MS_FAILURE : ( (num_tuples > 0) ? MS_SUCCESS : MS_DONE );

}




/* query the DB for info about the requested table */
/*  */
/* CHEAT: dont look in the system tables, get query optimization infomation */
/*  */
/* get the table name, return a list of the possible columns (except GEOMETRY column) */
/*  */
/* found out this is called during a query */

int msPOSTGISLayerGetItems(layerObj *layer)
{
    msPOSTGISLayerInfo  *layerinfo;
    char                *table_name = 0;
    char                *geom_column_name = 0;
    char                *urid_name = 0;
    char                *user_srid = 0;
    char                *sql = 0;

    PGresult            *query_result;
    int                 t;
    char                *col;
    char                found_geom = 0;

    int                 item_num;


    if(layer->debug) {
        msDebug("in msPOSTGISLayerGetItems  (find column names)\n");
    }

    layerinfo = getPostGISLayerInfo( layer);

    if(!layerinfo) {
        /* layer not opened yet */
        msSetError(MS_QUERYERR, "msPOSTGISLayerGetItems called on unopened layer", "msPOSTGISLayerGetItems()");

        return MS_FAILURE;
    }

    if(!layerinfo->conn) {
        msSetError(MS_QUERYERR, "msPOSTGISLayerGetItems called on POSTGIS layer with no connection to DB.", "msPOSTGISLayerGetItems()");

        return MS_FAILURE;
    }
    /* get the table name and geometry column name */

    if (msPOSTGISLayerParseData(layer, &geom_column_name, &table_name, &urid_name, &user_srid, layer->debug) != MS_SUCCESS) {
		return MS_FAILURE;
	}

    /* two cases here.  One, its a table (use select * from table) otherwise, just use the select clause */
    sql = (char *) malloc(36 + strlen(table_name) + 1);
    sprintf(sql, "SELECT * FROM %s WHERE false LIMIT 0", table_name); /* attempt the query, but dont actually do much (this might take some time if there is an order by!) */

    free(user_srid);
    free(urid_name);
    /* geom_column_name is needed later */
    free(table_name);

    query_result = PQexecParams(layerinfo->conn, sql,0, NULL, NULL, NULL, NULL, 0);

    if(!query_result || PQresultStatus(query_result) != PGRES_TUPLES_OK) {
        msSetError(MS_QUERYERR, "Error executing POSTGIS SQL statement (in msPOSTGISLayerGetItems): %s\n-%s\n", "msPOSTGISLayerGetItems()", sql, PQerrorMessage(layerinfo->conn));

        if(query_result) {
            PQclear(query_result);
        } else {
            msPOSTGISSanitizeConnection(layerinfo->conn);
	}

        free(sql);
        free(geom_column_name);

        return MS_FAILURE;
    }

    free(sql);

    layer->numitems = PQnfields(query_result) - 1; /* dont include the geometry column */
    layer->items = malloc(sizeof(char *) * (layer->numitems + 1)); /* +1 in case there is a problem finding goeometry column */
                                                                /* it will return an error if there is no geometry column found, */
                                                                /* so this isnt a problem */

    found_geom = 0; /* havent found the geom field */
    item_num = 0;

    for(t = 0; t < PQnfields(query_result); t++) {
        col = PQfname(query_result, t);
        if(strcmp(col, geom_column_name) != 0) {
            /* this isnt the geometry column */
            layer->items[item_num] = (char *) malloc(strlen(col) + 1);
            strcpy(layer->items[item_num], col);
            item_num++;
        } else {
            found_geom = 1;
        }
    }

    PQclear(query_result);

    if(!found_geom) {
        msSetError(MS_QUERYERR, "msPOSTGISLayerGetItems: tried to find the geometry column in the results from the database, but couldnt find it.  Is it miss-capitialized? '%s'", "msPOSTGISLayerGetItems()", geom_column_name);

	    free(geom_column_name);

        return MS_FAILURE;
    }

    free(geom_column_name);

    return msPOSTGISLayerInitItemInfo(layer);
}


/* we return an infinite extent */
/* we could call the SQL AGGREGATE extent(GEOMETRY), but that would take FOREVER */
/* to return (it has to read the entire table). */
/* So, we just tell it that we're everywhere and lets the spatial indexing figure things out for us */
/*  */
/* Never seen this function actually called */
int msPOSTGISLayerGetExtent(layerObj *layer, rectObj *extent)
{
    if(layer->debug) {
        msDebug("msPOSTGISLayerGetExtent called\n");
    }

    extent->minx = extent->miny = -1.0 * FLT_MAX ;
    extent->maxx = extent->maxy = FLT_MAX;

    return MS_SUCCESS;


        /* this should get the real extents,but it requires a table read */
        /* unforunately, there is no way to call this function from mapscript, so its */
        /* pretty useless.  Untested since you cannot actually call it. */

/*

PGresult   *query_result;
    char        sql[5000];

    msPOSTGISLayerInfo *layerinfo;

   char    table_name[5000];
      char    geom_column_name[5000];
      char    urid_name[5000];
    char    user_srid[5000];
    if (layer == NULL)
    {
                char tmp[5000];

                sprintf(tmp, "layer is null - have you opened the layer yet?");
                    msSetError(MS_QUERYERR, tmp,
                         "msPOSTGISLayerGetExtent()");

        return(MS_FAILURE);
    }



  layerinfo = (msPOSTGISLayerInfo *) layer->postgislayerinfo;

   msPOSTGISLayerParseData(layer->data, geom_column_name,table_name, urid_name,user_srid,layer->debug);

   sprintf(sql,"select extent(%s) from %s", geom_column_name,table_name);


    if (layerinfo->conn == NULL)
    {
                char tmp[5000];

                sprintf(tmp, "layer doesnt have a postgis connection - have you opened the layer yet?");
                    msSetError(MS_QUERYERR, tmp,
                         "msPOSTGISLayerGetExtent()");

        return(MS_FAILURE);
    }



    query_result = PQexec(layerinfo->conn, sql);
    if (!(query_result) || PQresultStatus(query_result) !=  PGRES_TUPLES_OK)
    {
        char tmp[5000];

        sprintf(tmp, "Error executing POSTGIS  SQL   statement (in msPOSTGISLayerGetExtent): %s", layerinfo->sql);
            msSetError(MS_QUERYERR, tmp,
                 "msPOSTGISLayerGetExtent()");

            PQclear(query_result);
        return(MS_FAILURE);
    }

    if (PQntuples(query_result) != 1)
    {
                char tmp[5000];

                sprintf(tmp, "Error executing POSTGIS  SQL   statement (in msPOSTGISLayerGetExtent) [doesnt have exactly 1 result]: %s", layerinfo->sql);
                    msSetError(MS_QUERYERR, tmp,
                         "msPOSTGISLayerGetExtent()");

                    PQclear(query_result);
        return(MS_FAILURE);
    }

    sscanf(PQgetvalue(query_result,0,0),"%lf %lf %lf %lf", &extent->minx,&extent->miny,&extent->maxx,&extent->maxy );

    PQclear(query_result);
    */

}

int msPOSTGISLayerRetrievePGVersion(layerObj *layer, int debug, int *major, int *minor, int *point) {
    PGresult *query_result;
    char *sql;
    msPOSTGISLayerInfo *layerinfo;
    char *tmp;

    sql = "select substring(version() from 12 for (position('on' in version()) - 13))";

    if(debug) {
        msDebug("msPOSTGISLayerRetrievePGVersion(): query = %s\n", sql);
    }
  
    layerinfo = (msPOSTGISLayerInfo *) layer->layerinfo;

    if(layerinfo->conn == NULL) {
        char *tmp1 = "Layer does not have a postgis connection.";
        msSetError(MS_QUERYERR, tmp1, "msPOSTGISLayerRetrievePGVersion()\n");
        return(MS_FAILURE);
    }

    query_result = PQexec(layerinfo->conn, sql);
    if(!(query_result) || PQresultStatus(query_result) != PGRES_TUPLES_OK) {
        char *tmp1;
        char *tmp2 = NULL;

        tmp1 = "Error executing POSTGIS statement (msPOSTGISLayerRetrievePGVersion():";
        tmp2 = (char *)malloc(sizeof(char)*(strlen(tmp1) + strlen(sql) + 1));
        strcpy(tmp2, tmp1);
        strcat(tmp2, sql);
        msSetError(MS_QUERYERR, tmp2, "msPOSTGISLayerRetrievePGVersion()");
        if(debug) {
          msDebug("msPOSTGISLayerRetrievePGVersion: No results returned.\n");
        }
        free(tmp2);
        msPOSTGISSanitizeConnection(layerinfo->conn);
        return(MS_FAILURE);

    }
    if(PQntuples(query_result) < 1) {
        if(debug) {
            msDebug("msPOSTGISLayerRetrievePGVersion: No results found.\n");
        }
        PQclear(query_result);
        return MS_FAILURE;
    } 
    if(PQgetisnull(query_result, 0, 0)) {
        if(debug) {
            msDebug("msPOSTGISLayerRetrievePGVersion: Null result returned.\n");
        }
        PQclear(query_result);
        return MS_FAILURE;
    }

    tmp = PQgetvalue(query_result, 0, 0);

    if(debug) {
        msDebug("msPOSTGISLayerRetrievePGVersion: Version String: %s\n", tmp);
    }

    *major = atoi(tmp);
    *minor = atoi(tmp + 2);
    *point = atoi(tmp + 4);
  
    if(debug) {
        msDebug("msPOSTGISLayerRetrievePGVersion(): Found version %i, %i, %i\n", *major, *minor, *point);
    }

    PQclear(query_result);
    return MS_SUCCESS;
}

int msPOSTGISLayerRetrievePK(layerObj *layer, char **urid_name, char* table_name, int debug) 
{
    PGresult   *query_result = 0;
    char        *sql = 0;
    msPOSTGISLayerInfo *layerinfo = 0;
    int major, minor, point, tmpint;
    int length;
    char *pos_sep, *schema, *table;
    schema = NULL;
    table = NULL;

     /* Attempt to separate table_name into schema.table */
    pos_sep = strstr(table_name, ".");
    if(pos_sep) 
    {
      length = strlen(table_name) - strlen(pos_sep);
      schema = (char *)malloc(length + 1);
      strncpy(schema, table_name, length);
      schema[length] = 0;

      length = strlen(pos_sep);
      table = (char *)malloc(length);
      strncpy(table, pos_sep + 1, length - 1);
      table[length - 1] = 0;
      
      if(debug)
      {
        msDebug("msPOSTGISLayerRetrievePK(): Found schema %s, table %s.\n", schema, table);
      }
    }
    
    if(msPOSTGISLayerRetrievePGVersion(layer, debug, &major, &minor, &point) == MS_FAILURE) 
    {
        if(debug)
        {
            msDebug("msPOSTGISLayerRetrievePK(): Unabled to retrieve version.\n");
        }
        return MS_FAILURE;
    }
    if(major < 7) 
    {
        if(debug)
        {
            msDebug("msPOSTGISLayerRetrievePK(): Major version below 7.\n");
        }
        return MS_FAILURE;
    }
    else if(major == 7 && minor < 2) 
    {
        if(debug)
        {
            msDebug("msPOSTGISLayerRetrievePK(): Version below 7.2.\n");
        }
        return MS_FAILURE;
    }
    if(major == 7 && minor == 2) {
        /*
         * PostgreSQL v7.2 has a different representation of primary keys that
         * later versions.  This currently does not explicitly exclude
         * multicolumn primary keys.
         */
        sql = malloc(strlen(table_name) + 234);
        sprintf(sql, "select b.attname from pg_class as a, pg_attribute as b, (select oid from pg_class where relname = '%s') as c, pg_index as d where d.indexrelid = a.oid and d.indrelid = c.oid and d.indisprimary and b.attrelid = a.oid and a.relnatts = 1", table_name);
    } else {
        /*
         * PostgreSQL v7.3 and later treat primary keys as constraints.
         * We only support single column primary keys, so multicolumn
         * pks are explicitly excluded from the query.
         */
        if(schema && table)
        {
            sql = malloc(strlen(schema) + strlen(table) + 376); 
            sprintf(sql, "select attname from pg_attribute, pg_constraint, pg_class, pg_namespace where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = '%s' and pg_class.relnamespace = pg_namespace.oid and pg_namespace.nspname = '%s' and pg_constraint.conkey[2] is null", table, schema);
            free(table);
            free(schema);
        }
        else
        {
            sql = malloc(strlen(table_name) + 325); 
            sprintf(sql, "select attname from pg_attribute, pg_constraint, pg_class where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = '%s' and pg_table_is_visible(pg_class.oid) and pg_constraint.conkey[2] is null", table_name);
      }
    }

    if (debug)
    {
       msDebug("msPOSTGISLayerRetrievePK: query = %s\n", sql);
    }

    layerinfo = (msPOSTGISLayerInfo *) layer->layerinfo;

    if(layerinfo->conn == NULL) 
    {
      char tmp1[42] = "Layer does not have a postgis connection.";
      msSetError(MS_QUERYERR, tmp1, "msPOSTGISLayerRetrievePK()");
      free(sql);
      return(MS_FAILURE);
    }

    query_result = PQexec(layerinfo->conn, sql);
    if(!(query_result) || PQresultStatus(query_result) != PGRES_TUPLES_OK) 
    {
      char *tmp1;
      char *tmp2 = NULL;

      tmp1 = "Error executing POSTGIS statement (msPOSTGISLayerRetrievePK():";
      tmp2 = (char *)malloc(sizeof(char)*(strlen(tmp1) + strlen(sql) + 1));
      strcpy(tmp2, tmp1);
      strcat(tmp2, sql);
      msSetError(MS_QUERYERR, tmp2, "msPOSTGISLayerRetrievePK()");
      free(tmp2);
      free(sql);
      msPOSTGISSanitizeConnection(layerinfo->conn);
      return(MS_FAILURE);

    }

    if(PQntuples(query_result) < 1) 
    {
      if(debug) 
      {
        msDebug("msPOSTGISLayerRetrievePK: No results found.\n");
      }
      PQclear(query_result);
      free(sql);
      return MS_FAILURE;
    }
    if(PQntuples(query_result) > 1)
    {
      if(debug)
      {
        msDebug("msPOSTGISLayerRetrievePK: Multiple results found.\n");
      }
      PQclear(query_result);
      free(sql);
      return MS_FAILURE;
    }

    if(PQgetisnull(query_result, 0, 0)) 
    {
      if(debug) 
      {
        msDebug("msPOSTGISLayerRetrievePK: Null result returned.\n");
      }
      PQclear(query_result);
      free(sql);
      return MS_FAILURE;
    }

    tmpint = PQgetlength(query_result, 0, 0);
    *urid_name = (char *)malloc(tmpint + 1);
    strcpy(*urid_name, PQgetvalue(query_result, 0, 0));

    PQclear(query_result);
    free(sql);
    return MS_SUCCESS;
}

/* Function to parse the Mapserver DATA parameter for geometry
 * column name, table name and name of a column to serve as a
 * unique record id
 */

static int msPOSTGISLayerParseData(layerObj *layer, char **geom_column_name, char **table_name, char **urid_name, char **user_srid, int debug)
{
    char    *pos_opt, *pos_scn, *tmp, *pos_srid, *pos_urid, *data;
    int     slength;

    data = layer->data;

    /* given a string of the from 'geom from ctivalues' or 'geom from () as foo'
     * return geom_column_name as 'geom'
     * and table name as 'ctivalues' or 'geom from () as foo'
     */

    /* First look for the optional ' using unique ID' string */
    pos_urid = strstrIgnoreCase(data, " using unique ");

    if(pos_urid) {
        /* CHANGE - protect the trailing edge for thing like 'using unique ftab_id using srid=33' */
        tmp = strstr(pos_urid + 14, " ");
        if(!tmp) {
            tmp = pos_urid + strlen(pos_urid);
        }
        *urid_name = (char *) malloc((tmp - (pos_urid + 14)) + 1);
        strncpy(*urid_name, pos_urid + 14, tmp - (pos_urid + 14));
        (*urid_name)[tmp - (pos_urid + 14)] = 0;
    }

    /* Find the srid */
    pos_srid = strstrIgnoreCase(data, " using SRID=");
    if(!pos_srid) {
        *user_srid = (char *) malloc(1);
        (*user_srid)[0] = 0;
    } else {
        slength = strspn(pos_srid + 12, "-0123456789");
        if(!slength) {
            msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msPOSTGISLayerParseData()", "Error parsing POSTGIS data variable: You specified 'using SRID=#' but didnt have any numbers!\n\nMore Help:\n\n", data);

            return MS_FAILURE;
        } else {
            *user_srid = (char *) malloc(slength + 1);
            strncpy(*user_srid, pos_srid + 12, slength);
            (*user_srid)[slength] = 0; /* null terminate it */
        }
    }


    /* this is a little hack so the rest of the code works.  If the ' using SRID=' comes before */
    /* the ' using unique ', then make sure pos_opt points to where the ' using SRID' starts! */
    
    /*pos_opt = (pos_srid > pos_urid) ? pos_urid : pos_srid; */
    if (!pos_srid && !pos_urid) { pos_opt = pos_urid; }
    else if (pos_srid && pos_urid) 
    { 
        pos_opt = (pos_srid > pos_urid)? pos_urid : pos_srid;
    }
    else { pos_opt = (pos_srid > pos_urid) ? pos_srid : pos_urid; }

    /* Scan for the table or sub-select clause */
    pos_scn = strstrIgnoreCase(data, " from ");
    if(!pos_scn) {
        msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msPOSTGISLayerParseData()", "Error parsing POSTGIS data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find ' from ').  More help: \n\n", data);

        return MS_FAILURE;
    }

    /* Copy the geometry column name */
    *geom_column_name = (char *) malloc((pos_scn - data) + 1);
    strncpy(*geom_column_name, data, pos_scn - data);
    (*geom_column_name)[pos_scn - data] = 0;

    /* Copy out the table name or sub-select clause */
    if(!pos_opt) {
        pos_opt = pos_scn + strlen(pos_scn);
    }
    *table_name = (char *) malloc((pos_opt - (pos_scn + 6)) + 1);
    strncpy(*table_name, pos_scn + 6, pos_opt - (pos_scn + 6));
    (*table_name)[pos_opt - (pos_scn + 6)] = 0;

    if(strlen(*table_name) < 1 || strlen(*geom_column_name) < 1) {
        msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msPOSTGISLayerParseData()", "Error parsing POSTGIS data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find a geometry_column or table/subselect).  More help: \n\n", data);

        return MS_FAILURE;
    }

    if( !pos_urid ) {
        if( msPOSTGISLayerRetrievePK(layer, urid_name, *table_name, debug) != MS_SUCCESS ) {
            /* No user specified unique id so we will use the Postgesql OID */
            *urid_name = (char *) malloc(4);
            strcpy(*urid_name, "OID");
        }
    }


    if(debug) {
        msDebug("msPOSTGISLayerParseData: unique column = %s, srid='%s', geom_column_name = %s, table_name=%s\n", *urid_name, *user_srid, *geom_column_name, *table_name);
    }

    return MS_SUCCESS;
}

#else

/* prototypes if postgis isnt supposed to be compiled */

int msPOSTGISLayerOpen(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerOpen called but unimplemented!  (mapserver not compiled with postgis support)", "msPOSTGISLayerOpen()");
    return MS_FAILURE;
}

int msPOSTGISLayerIsOpen(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msPOSTGISIsLayerOpen called but unimplemented!  (mapserver not compiled with postgis support)", "msPOSTGISLayerIsOpen()");
    return MS_FALSE;
}

void msPOSTGISLayerFreeItemInfo(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerFreeItemInfo called but unimplemented!(mapserver not compiled with postgis support)", "msPOSTGISLayerFreeItemInfo()");
}

int msPOSTGISLayerInitItemInfo(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerInitItemInfo called but unimplemented!(mapserver not compiled with postgis support)", "msPOSTGISLayerInitItemInfo()");
    return MS_FAILURE;
}

int msPOSTGISLayerWhichShapes(layerObj *layer, rectObj rect)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerWhichShapes called but unimplemented!(mapserver not compiled with postgis support)", "msPOSTGISLayerWhichShapes()");
    return MS_FAILURE;
}

int msPOSTGISLayerClose(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerClose called but unimplemented!(mapserver not compiled with postgis support)", "msPOSTGISLayerClose()");
    return MS_FAILURE;
}

int msPOSTGISLayerNextShape(layerObj *layer, shapeObj *shape)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerNextShape called but unimplemented!(mapserver not compiled with postgis support)", "msPOSTGISLayerNextShape()");
    return MS_FAILURE;
}

int msPOSTGISLayerGetShape(layerObj *layer, shapeObj *shape, long record)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerGetShape called but unimplemented!(mapserver not compiled with postgis support)", "msPOSTGISLayerGetShape()");
    return MS_FAILURE;
}

int msPOSTGISLayerGetExtent(layerObj *layer, rectObj *extent)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerGetExtent called but unimplemented!(mapserver not compiled with postgis support)", "msPOSTGISLayerGetExtent()");
    return MS_FAILURE;
}

int msPOSTGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerGetShapeRandom called but unimplemented!(mapserver not compiled with postgis support)", "msPOSTGISLayerGetShapeRandom()");
    return MS_FAILURE;
}

int msPOSTGISLayerGetItems(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msPOSTGISLayerGetItems called but unimplemented!(mapserver not compiled with postgis support)", "msPOSTGISLayerGetItems()");
    return MS_FAILURE;
}


/* end above's #ifdef USE_POSTGIS */
#endif

int msPOSTGISLayerSetTimeFilter(layerObj *lp, 
                                const char *timestring, 
                                const char *timefield)
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

int 
msPOSTGISLayerGetShapeVT(layerObj *layer, shapeObj *shape, int tile, long record)
{
    return msPOSTGISLayerGetShape(layer, shape, record);
}


char *msPOSTGISEscapeSQLParam(layerObj *layer, const char *pszString)
{
#ifdef USE_POSTGIS
    msPOSTGISLayerInfo *layerinfo = NULL;
    int nError;
    size_t nSrcLen;
    char* pszEscapedStr =NULL;

    if (layer && pszString && strlen(pszString) > 0)
    {
        if(!msPOSTGISLayerIsOpen(layer))
          msPOSTGISLayerOpen(layer);
    
        assert(layer->layerinfo != NULL);

        layerinfo = (msPOSTGISLayerInfo *) layer->layerinfo;
        nSrcLen = strlen(pszString);
        pszEscapedStr = (char*) malloc( 2 * nSrcLen + 1);
        PQescapeStringConn (layerinfo->conn, pszEscapedStr, pszString, nSrcLen, &nError);
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
                "msPOSTGISEscapeSQLParam()");
    return NULL;
#endif
}


int
msPOSTGISLayerInitializeVirtualTable(layerObj *layer)
{
    assert(layer != NULL);
    assert(layer->vtable != NULL);

    layer->vtable->LayerInitItemInfo = msPOSTGISLayerInitItemInfo;
    layer->vtable->LayerFreeItemInfo = msPOSTGISLayerFreeItemInfo;
    layer->vtable->LayerOpen = msPOSTGISLayerOpen;
    layer->vtable->LayerIsOpen = msPOSTGISLayerIsOpen;
    layer->vtable->LayerWhichShapes = msPOSTGISLayerWhichShapes;
    layer->vtable->LayerNextShape = msPOSTGISLayerNextShape;
    layer->vtable->LayerGetShape = msPOSTGISLayerGetShapeVT;

    layer->vtable->LayerClose = msPOSTGISLayerClose;

    layer->vtable->LayerGetItems = msPOSTGISLayerGetItems;
    layer->vtable->LayerGetExtent = msPOSTGISLayerGetExtent;

    layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;

    /* layer->vtable->LayerGetAutoStyle, not supported for this layer */
    layer->vtable->LayerCloseConnection = msPOSTGISLayerClose;
    
    layer->vtable->LayerSetTimeFilter = msPOSTGISLayerSetTimeFilter;
    /* layer->vtable->LayerCreateItems, use default */
    /* layer->vtable->LayerGetNumFeatures, use default */

    layer->vtable->LayerEscapeSQLParam = msPOSTGISEscapeSQLParam;

    return MS_SUCCESS;
}


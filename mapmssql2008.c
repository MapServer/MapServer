/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MS SQL 2008 (Katmai) Layer Connector
 * Author:   Richard Hillman - based on PostGIS and SpatialDB connectors
 *
 ******************************************************************************
 * Copyright (c) 2007 IS Consulting (www.mapdotnet.com)
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
 ******************************************************************************
 *
 * Revision 1.0  2007/7/1 
 * Created.
 *
 */

#define _CRT_SECURE_NO_WARNINGS 1

/* $Id$ */
#include <assert.h>
#include "mapserver.h"
#include "maptime.h"

#ifdef USE_MSSQL2008

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <string.h>
#include <ctype.h> /* tolower() */

MS_CVSID("$Id$")

/* Structure for connection to an ODBC database (Microsoft preferred way to connect to SQL Server 2005 from c/c++) */
typedef struct msODBCconn_t
{
    SQLHENV     henv;               /* ODBC HENV */
    SQLHDBC     hdbc;               /* ODBC HDBC */
    SQLHSTMT    hstmt;              /* ODBC HSTMNT */
    char        errorMessage[1024]; /* Last error message if any */
} msODBCconn;

typedef struct ms_MSSQL2008_layer_info_t
{
    char        *sql;           /* sql query to send to DB */
    long        row_num;        /* what row is the NEXT to be read (for random access) */
	char		*geom_column;	/* name of the actual geometry column parsed from the LAYER's DATA field */
	char		*geom_table;	/* the table name or sub-select decalred in the LAYER's DATA field */
    char        *urid_name;     /* name of user-specified unique identifier or OID */
    char        *user_srid;     /* zero length = calculate, non-zero means using this value! */
	char		*index_name;	/* hopefully this isn't necessary - but if the optimizer ain't cuttin' it... */

    msODBCconn * conn;          /* Connection to db */
} msMSSQL2008LayerInfo;

#define SQL_COLUMN_NAME_MAX_LENGTH 128
#define SQL_TABLE_NAME_MAX_LENGTH 128

#define DATA_ERROR_MESSAGE \
    "%s" \
    "Error with MSSQL2008 data variable. You specified '%s'.<br>\n" \
    "Standard ways of specifiying are : <br>\n(1) 'geometry_column from geometry_table' <br>\n(2) 'geometry_column from (&lt;sub query&gt;) as foo using unique &lt;column name&gt; using SRID=&lt;srid#&gt;' <br><br>\n\n" \
    "Make sure you utilize the 'using unique  &lt;column name&gt;' and 'using with &lt;index name&gt;' clauses in.\n\n<br><br>" \
    "For more help, please see http://www.mapdotnet.com \n\n<br><br>" \
    "mapmssql2008.c - version of 2007/7/1.\n"

/* Little dummy buffer used to clean up memory allocation */
char dummyBuffer[1];

msMSSQL2008LayerInfo *getMSSQL2008LayerInfo(const layerObj *layer)
{
    return layer->layerinfo;
}

void setMSSQL2008LayerInfo(layerObj *layer, msMSSQL2008LayerInfo *MSSQL2008layerinfo)
{
     layer->layerinfo = (void*) MSSQL2008layerinfo;
}

void handleSQLError(layerObj *layer)
{
   SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
   SQLINTEGER    NativeError;
   SQLSMALLINT   i, MsgLen;
   SQLRETURN  rc;
   msMSSQL2008LayerInfo *layerinfo = getMSSQL2008LayerInfo(layer);

   if (layerinfo == NULL)
       return;
   
    // Get the status records.
   i = 1;
   while ((rc = SQLGetDiagRec(SQL_HANDLE_STMT, layerinfo->conn->hstmt, i, SqlState, &NativeError,
            Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) 
   {
      if(layer->debug) 
      {
            msDebug("SQLError: %s\n", Msg);
      }
      i++;
   }
}

/* remove white space */
/* dont send in empty strings or strings with just " " in them! */
static char* removeWhite(char *str)
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
static char *strstrIgnoreCase(const char *haystack, const char *needle)
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

static int msMSSQL2008LayerParseData(layerObj *layer, char **geom_column_name, char **table_name, char **urid_name, char **user_srid, char **index_name, int debug);

/* Close connection and handles */
static void msMSSQL2008CloseConnection(void *conn_handle)
{
    msODBCconn * conn = (msODBCconn *) conn_handle;

    if (!conn)
    {
        return;
    }

	if (conn->hstmt)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, conn->hstmt);
	}
	if (conn->hdbc)
	{
		SQLDisconnect(conn->hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, conn->hdbc);
	}
	if (conn->henv)	
	{
		SQLFreeHandle(SQL_HANDLE_ENV, conn->henv);
	}

	free(conn);
}

/* Set the error string for the connection */
static void setConnError(msODBCconn *conn)
{
    SQLSMALLINT len;

    SQLGetDiagField(SQL_HANDLE_DBC, conn->hdbc, 1, SQL_DIAG_MESSAGE_TEXT, (SQLPOINTER) conn->errorMessage, sizeof(conn->errorMessage), &len);

    conn->errorMessage[len] = 0;
}

/* Connect to db */
static msODBCconn * mssql2008Connect(const char * connString)
{
    char fullConnString[1024];
    SQLRETURN rc;
    msODBCconn * conn = malloc(sizeof(msODBCconn));

    if (!conn)
    {
        return NULL;
    }

    memset(conn, 0, sizeof(*conn));

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &conn->henv);
    
    SQLSetEnvAttr(conn->henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0);

    SQLAllocHandle(SQL_HANDLE_DBC, conn->henv, &conn->hdbc);

    snprintf(fullConnString, sizeof(fullConnString), "DRIVER=SQL Server;%s", connString);

    {
        SQLCHAR outConnString[1024];
        SQLSMALLINT outConnStringLen;

        rc = SQLDriverConnect(conn->hdbc, NULL, fullConnString, SQL_NTS, outConnString, 1024, &outConnStringLen, SQL_DRIVER_NOPROMPT);
    }

    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
        setConnError(conn);

        return conn;
    }
 
    SQLAllocHandle(SQL_HANDLE_STMT, conn->hdbc, &conn->hstmt);

    return conn;
}

/* Set the error string for the statement execution */
static void setStmntError(msODBCconn *conn)
{
    SQLSMALLINT len;

    SQLGetDiagField(SQL_HANDLE_STMT, conn->hstmt, 1, SQL_DIAG_MESSAGE_TEXT, (SQLPOINTER) conn->errorMessage, sizeof(conn->errorMessage), &len);

    conn->errorMessage[len] = 0;
}

/* Execute SQL against connection. Set error string  if failed */
static int executeSQL(msODBCconn *conn, const char * sql)
{
    SQLRETURN rc;

    SQLCloseCursor(conn->hstmt);

    rc = SQLExecDirect(conn->hstmt, (char *) sql, SQL_NTS);

    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        return 1;
    }
    else
    {
        setStmntError(conn);
        return 0;
    }
}

/* Get columns name from query results */
static int columnName(msODBCconn *conn, int index, char *buffer, int bufferLength)
{
    SQLRETURN rc;

    SQLCHAR columnName[SQL_COLUMN_NAME_MAX_LENGTH + 1];
    SQLSMALLINT columnNameLen;
    SQLSMALLINT dataType;
    SQLUINTEGER columnSize;
    SQLSMALLINT decimalDigits;
    SQLSMALLINT nullable;

    rc = SQLDescribeCol(
         conn->hstmt,
         index,
         columnName,
         SQL_COLUMN_NAME_MAX_LENGTH,
         &columnNameLen,
         &dataType,
         &columnSize,
         &decimalDigits,
         &nullable);

    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        if (bufferLength < SQL_COLUMN_NAME_MAX_LENGTH + 1)
            strncpy(buffer, columnName, bufferLength);
        else
            strncpy(buffer, columnName, SQL_COLUMN_NAME_MAX_LENGTH + 1);
        return 1;
    }
    else
    {
        setStmntError(conn);
        return 0;
    }
}

/* open up a connection to the MS SQL 2008 database using the connection string in layer->connection */
/* ie. "driver=<driver>;server=<server>;database=<database>;integrated security=?;user id=<username>;password=<password>" */
int msMSSQL2008LayerOpen(layerObj *layer)
{
    msMSSQL2008LayerInfo  *layerinfo;
    char                *index, *maskeddata;
    int                 i, count;
	char *conn_decrypted = NULL;

    if(layer->debug) {
        msDebug("msMSSQL2008LayerOpen called datastatement: %s\n", layer->data);
    }

    if(getMSSQL2008LayerInfo(layer)) {
        if(layer->debug) {
            msDebug("msMSSQL2008LayerOpen :: layer is already open!!\n");
        }
        return MS_SUCCESS;  /* already open */
    }

    if(!layer->data) {
        msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerOpen()", "", "Error parsing MSSQL2008 data variable: nothing specified in DATA statement.<br><br>\n\nMore Help:<br><br>\n\n");

        return MS_FAILURE;
    }

    /* have to setup a connection to the database */

    layerinfo = (msMSSQL2008LayerInfo*) malloc(sizeof(msMSSQL2008LayerInfo));
    layerinfo->sql = NULL; /* calc later */
    layerinfo->row_num = 0;
	layerinfo->geom_column = NULL;
	layerinfo->geom_table = NULL;
    layerinfo->urid_name = NULL;
    layerinfo->user_srid = NULL;
	layerinfo->index_name = NULL;
    layerinfo->conn = NULL;

    layerinfo->conn = (msODBCconn *) msConnPoolRequest(layer);

    if(!layerinfo->conn) {
        if(layer->debug) {
            msDebug("MSMSSQL2008LayerOpen -- shared connection not available.\n");
        }
                
        /* Decrypt any encrypted token in connection and attempt to connect */
        conn_decrypted = msDecryptStringTokens(layer->map, layer->connection);
        if (conn_decrypted == NULL)
		{
			return(MS_FAILURE);  /* An error should already have been produced */
        }

		layerinfo->conn = mssql2008Connect(conn_decrypted);

		free(conn_decrypted);
		conn_decrypted = NULL;

        if(!layerinfo->conn || layerinfo->conn->errorMessage[0]) 
        {
            char *errMess = "Out of memory";

            msDebug("FAILURE!!!");
            maskeddata = (char *)malloc(strlen(layer->connection) + 1);
            strcpy(maskeddata, layer->connection);
            index = strstr(maskeddata, "password=");
            if(index != NULL) {
              index = (char *)(index + 9);
              count = (int)(strstr(index, " ") - index);
              for(i = 0; i < count; i++) {
                  strncpy(index, "*", (int)1);
                  index++;
              }
            }
            
            if (layerinfo->conn)
            {
                errMess = layerinfo->conn->errorMessage;
            }

            msSetError(MS_QUERYERR, 
                "Couldnt make connection to MS SQL Server 2008 with connect string '%s'.\n<br>\n"
                "Error reported was '%s'.\n<br>\n\n"
                "This error occured when trying to make a connection to the specified SQL server.  \n"
                "<br>\nMost commonly this is caused by <br>\n"
                "(1) incorrect connection string <br>\n"
                "(2) you didnt specify a 'user id=...' in your connection string <br>\n"
                "(3) SQL server isnt running <br>\n"
                "(4) TCPIP not enabled for SQL Client or server <br>\n\n", 
                "msMSSQL2008LayerOpen()", maskeddata, errMess);

            free(maskeddata);
            msMSSQL2008CloseConnection(layerinfo->conn);
            free(layerinfo);
            return MS_FAILURE;
        }

        msConnPoolRegister(layer, layerinfo->conn, msMSSQL2008CloseConnection);
    }

    setMSSQL2008LayerInfo(layer, layerinfo);

    if (msMSSQL2008LayerParseData(layer, &layerinfo->geom_column, &layerinfo->geom_table, &layerinfo->urid_name, &layerinfo->user_srid, &layerinfo->index_name, layer->debug) != MS_SUCCESS)
	{
		msSetError( MS_QUERYERR, "Could not parse the layer data", "msMSSQL2008LayerOpen()");
        return MS_FAILURE;
    } 

    return MS_SUCCESS;
}

/* Return MS_TRUE if layer is open, MS_FALSE otherwise. */
int msMSSQL2008LayerIsOpen(layerObj *layer)
{
    return getMSSQL2008LayerInfo(layer) ? MS_TRUE : MS_FALSE;
}


/* Free the itemindexes array in a layer. */
void msMSSQL2008LayerFreeItemInfo(layerObj *layer)
{
    if(layer->debug) {
        msDebug("msMSSQL2008LayerFreeItemInfo called\n");
    }

    if(layer->iteminfo) {
        free(layer->iteminfo);
    }

    layer->iteminfo = NULL;
}


/* allocate the iteminfo index array - same order as the item list */
int msMSSQL2008LayerInitItemInfo(layerObj *layer)
{
    int     i;
    int     *itemindexes ;

    if (layer->debug) {
        msDebug("msMSSQL2008LayerInitItemInfo called\n");
    }

    if(layer->numitems == 0) {
        return MS_SUCCESS;
    }

    if(layer->iteminfo) {
        free(layer->iteminfo);
    }

    layer->iteminfo = (int *) malloc(sizeof(int) * layer->numitems);
    if(!layer->iteminfo) {
        msSetError(MS_MEMERR, NULL, "msMSSQL2008LayerInitItemInfo()");

        return MS_FAILURE;
    }

    itemindexes = (int*)layer->iteminfo;

    for(i = 0; i < layer->numitems; i++) {
        itemindexes[i] = i; /* last one is always the geometry one - the rest are non-geom */
    }

    return MS_SUCCESS;
}

/* Prepare and execute the SQL statement for this layer */
static int prepare_database(layerObj *layer, rectObj rect, char **query_string)
{
    msMSSQL2008LayerInfo *layerinfo;
    char        *temp = 0;
    char        *columns_wanted = 0;
    char        *data_source = 0;
    char        *f_table_name = 0;
	char		*geom_table = 0;
	/*
		"Geometry::STGeomFromText('POLYGON(())',)" + terminator = 40 chars
		Plus 10 formatted doubles (15 digits of precision, a decimal point, a space/comma delimiter each = 17 chars each)
		Plus SRID + comma - if SRID is a long...we'll be safe with 10 chars 
	*/
	char        box3d[40 + 10 * 22 + 11];
    char        query_string_temp[10000];       /* Should be big enough */
    int         t;

    char        *pos_from, *pos_ftab, *pos_space, *pos_paren;

    char        *tmp2 = 0;
    char        *error_message = 0;

    layerinfo =  getMSSQL2008LayerInfo(layer);

    /* Extract the proper f_table_name from the geom_table string.
     * We are expecting the geom_table to be either a single word
     * or a sub-select clause that possibly includes a join --
     *
     * (select column[,column[,...]] from ftab[ natural join table2]) as foo
     *
     * We are expecting whitespace or a ')' after the ftab name.
     *
     */

	geom_table = _strdup(layerinfo->geom_table);
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
            msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "prepare_database()", geom_table, "Error parsing MSSQL2008 data variable: Something is wrong with your subselect statement.<br><br>\n\nMore Help:<br><br>\n\n");

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

    if(layer->numitems == 0) 
    {
        char buffer[1000];

        snprintf(buffer, sizeof(buffer), "%s.STAsBinary(),convert(varchar(36), %s)", layerinfo->geom_column, layerinfo->urid_name);

        columns_wanted = _strdup(buffer);
    } 
    else 
    {
        char buffer[10000] = "";

        for(t = 0; t < layer->numitems; t++) {
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "convert(varchar(max), %s),", layer->items[t]);
        }

        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%s.STAsBinary(),convert(varchar(36), %s)", layerinfo->geom_column, layerinfo->urid_name);

        columns_wanted = _strdup(buffer);
    }

    if (rect.minx == rect.maxx || rect.miny == rect.maxy)
    {
        /* create point shape for rectangles with zero area */
        sprintf(box3d, "Geometry::STGeomFromText('POINT(%.15g %.15g)',%s)", /* %s.STSrid)", */
		    rect.minx, rect.miny, layerinfo->user_srid);
    }
    else
    {
	    sprintf(box3d, "Geometry::STGeomFromText('POLYGON((%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g,%.15g %.15g))',%s)", /* %s.STSrid)", */
		    rect.minx, rect.miny, 
		    rect.maxx, rect.miny, 
		    rect.maxx, rect.maxy, 
		    rect.minx, rect.maxy,
		    rect.minx, rect.miny,
            layerinfo->user_srid
            );
    }

    /* substitute token '!BOX!' in geom_table with the box3d - do an unlimited # of subs */
    /* to not undo the work here, we need to make sure that data_source is malloc'd here */

    if(!strstr(geom_table, "!BOX!")) 
    {
        data_source = (char *) malloc(strlen(geom_table) + 1);
        strcpy(data_source, geom_table);
    } 
    else 
    {
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

       /* if we're here, this will be a malloc'd string, so no need to copy it */
        data_source = (char *)geom_table;
    }

	/* use the index hint if provided */
	if ( layerinfo->index_name )
	{
		/* given the template - figure out how much to malloc and malloc it */
		char *with_template = "%s WITH (INDEX(%s))";
		int need_len = strlen(data_source) + strlen(with_template) + strlen(layerinfo->index_name);
		char *tmp = (char*) malloc( need_len + 1 );
		sprintf( tmp, with_template, data_source, layerinfo->index_name );
		free(data_source);
		data_source = tmp;
	}

    if(!layer->filter.string) 
    {
        sprintf(query_string_temp, "SELECT %s from %s WHERE %s.STIntersects(%s) = 1 ", 
            columns_wanted, data_source, layerinfo->geom_column, box3d );
    } 
    else 
    {
        sprintf(query_string_temp, "SELECT %s from %s WHERE (%s) and %s.STIntersects(%s) = 1 ",
            columns_wanted, data_source, layer->filter.string, layerinfo->geom_column, box3d );
    }

    free(data_source);
    free(f_table_name);
    free(columns_wanted);

    if(layer->debug) {
        msDebug("query_string_temp:%s\n", query_string_temp);
    }

    if (executeSQL(layerinfo->conn, query_string_temp))
    {
        *query_string = _strdup(query_string_temp);

        return MS_SUCCESS;
    }
    else
    {
        msSetError(MS_QUERYERR, "Error executing MSSQL2008 SQL statement: %s\n-%s\n", "msMSSQL2008LayerGetShape()", query_string_temp, layerinfo->conn->errorMessage);

        return MS_FAILURE;
    }
}

/* Execute SQL query for this layer */
int msMSSQL2008LayerWhichShapes(layerObj *layer, rectObj rect)
{
    msMSSQL2008LayerInfo  *layerinfo = 0;
    char    *query_str = 0;
    char    *table_name = 0;
    char    *geom_column_name = 0;
    char    *urid_name = 0;
    char    *user_srid = 0;
	char	*index_name = 0;
    int     set_up_result;

    if(layer->debug) {
        msDebug("msMSSQL2008LayerWhichShapes called\n");
    }

    layerinfo = getMSSQL2008LayerInfo(layer);

    if(!layerinfo) {
        /* layer not opened yet */
        msSetError(MS_QUERYERR, "msMSSQL2008LayerWhichShapes called on unopened layer (layerinfo = NULL)", "msMSSQL2008LayerWhichShapes()");

        return MS_FAILURE;
    }

    if(!layer->data) {
        msSetError(MS_QUERYERR, "Missing DATA clause in MSSQL2008 Layer definition.  DATA statement must contain 'geometry_column from table_name' or 'geometry_column from (sub-query) as foo'.", "msMSSQL2008LayerWhichShapes()");

        return MS_FAILURE;
    }

	set_up_result = prepare_database(layer, rect, &query_str);

    if(set_up_result != MS_SUCCESS) {
        free(query_str);

        return set_up_result; /* relay error */
    }

    layerinfo->sql = query_str;
    layerinfo->row_num = 0;

    return MS_SUCCESS;
}

/* Close the MSSQL2008 record set and connection */
int msMSSQL2008LayerClose(layerObj *layer)
{
    msMSSQL2008LayerInfo  *layerinfo;

    layerinfo = getMSSQL2008LayerInfo(layer);

    if(layer->debug) 
    {
        char *data = "";

        if (layer->data)
        {
            data = layer->data;
        }

        msDebug("msMSSQL2008LayerClose datastatement: %s\n", data);
    }

    if(layer->debug && !layerinfo) {
        msDebug("msMSSQL2008LayerClose -- layerinfo is  NULL\n");
    }

    if(layerinfo) {
        msConnPoolRelease(layer, layerinfo->conn);

        layerinfo->conn = NULL;

		if(layerinfo->user_srid) {
			free(layerinfo->user_srid);
			layerinfo->user_srid = NULL;
		}

        if(layerinfo->urid_name) {
            free(layerinfo->urid_name);
            layerinfo->urid_name = NULL;
        }

		if(layerinfo->index_name) {
			free(layerinfo->index_name);
			layerinfo->index_name = NULL;
		}

        if(layerinfo->sql) {
            free(layerinfo->sql);
            layerinfo->sql = NULL;
        }

        setMSSQL2008LayerInfo(layer, NULL);
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
    offset = 9;  /* were the first geometry is */
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

/* ******************************************************* */
/* wkb assumed to be same endian as this machine.  */
/* Should be in little endian (default if created by Microsoft platforms) */
/* ******************************************************* */
/* convert the wkb into points */
/* points -> pass through */
/* lines->   constituent points */
/* polys->   treat ring like line and pull out the consituent points */
static int  force_to_shapeType(char *wkb, shapeObj *shape, int msShapeType)
{
    int     offset = 0;
    int     ngeoms = 1;
    int     u, v;
    int     type, nrings, npoints;
    lineObj line = {0, NULL};

    shape->type = MS_SHAPE_NULL;  /* nothing in it */

    do
    {
        ngeoms--;

        memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */

        if (type == 1)
        {
            /* Point */
            shape->type = msShapeType;
            line.numpoints = 1;
            line.point = (pointObj *) malloc(sizeof(pointObj));

            memcpy(&line.point[0].x, &wkb[offset + 5], 8);
            memcpy(&line.point[0].y, &wkb[offset + 5 + 8], 8);
            offset += 5 + 16;

            if (msShapeType == MS_SHAPE_POINT)
            {
                msAddLine(shape, &line);
            }

            free(line.point);
        } 
        else if(type == 2) 
        {
            /* Linestring */
            shape->type = msShapeType;

            memcpy(&line.numpoints, &wkb[offset+5], 4); /* num points */
            line.point = (pointObj *) malloc(sizeof(pointObj) * line.numpoints);
            for(u = 0; u < line.numpoints; u++) {
                memcpy( &line.point[u].x, &wkb[offset+9 + (16 * u)], 8);
                memcpy( &line.point[u].y, &wkb[offset+9 + (16 * u)+8], 8);
            }
            offset += 9 + 16 * line.numpoints;  /* length of object */

            if ((msShapeType == MS_SHAPE_POINT) || (msShapeType == MS_SHAPE_LINE))
            {
                msAddLine(shape, &line);
            }

            free(line.point);
        } 
        else if(type == 3) 
        {
            /* Polygon */
            shape->type = msShapeType;

            memcpy(&nrings, &wkb[offset+5],4); /* num rings */
            /* add a line for each polygon ring */
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
        else if(type >= 4 && type <= 7) 
        {
            int cnt = 0;

            offset += 5;

            memcpy(&cnt, &wkb[offset], 4);
            offset += 4;  /* were the first geometry is */

            ngeoms += cnt;
        }
    }
    while (ngeoms > 0);

    return MS_SUCCESS;
}

///* if there is any polygon in wkb, return force_polygon */
///* if there is any line in wkb, return force_line */
///* otherwise return force_point */
//static int  dont_force(char *wkb, shapeObj *shape)
//{
//    int     offset =0;
//    int     ngeoms = 1;
//    int     type;
//    int     best_type;
//    int     u;
//    int     nrings, npoints;
//
//    best_type = MS_SHAPE_NULL;  /* nothing in it */
//
//    do
//    {
//        ngeoms--;
//
//        memcpy(&type, &wkb[offset + 1], 4);  /* type of this geometry */
//
//        if(type == 3) {
//            best_type = MS_SHAPE_POLYGON;
//        } else if(type ==2 && best_type != MS_SHAPE_POLYGON) {
//            best_type = MS_SHAPE_LINE;
//        } else if(type == 1 && best_type == MS_SHAPE_NULL) {
//            best_type = MS_SHAPE_POINT;
//        }
//
//        if (type == 1)
//        {
//            /* Point */
//            offset += 5 + 16;
//        } 
//        else if(type == 2) 
//        {
//            int numPoints;
//
//            memcpy(&numPoints, &wkb[offset+5], 4); /* num points */
//            /* Linestring */
//            offset += 9 + 16 * numPoints;  /* length of object */
//        } 
//        else if(type == 3) 
//        {
//            /* Polygon */
//            memcpy(&nrings, &wkb[offset+5],4); /* num rings */
//            offset += 9; /* now points at 1st linear ring */
//            for(u = 0; u < nrings; u++) {
//                /* for each ring, make a line */
//                memcpy(&npoints, &wkb[offset], 4); /* num points */
//                offset += 4 + 16 *npoints;
//            }
//        } 
//        else if(type >= 4 && type <= 7) 
//        {
//            int cnt = 0;
//
//            offset += 5;
//
//            memcpy(&cnt, &wkb[offset], 4);
//            offset += 4;  /* were the first geometry is */
//
//            ngeoms += cnt;
//        }
//    }
//    while (ngeoms > 0);
//
//    return force_to_shapeType(wkb, shape, best_type);
//}
//
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

/* Used by NextShape() to access a shape in the query set */
int msMSSQL2008LayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
    msMSSQL2008LayerInfo  *layerinfo;
    int                 result;
	SQLINTEGER needLen = 0;
    SQLINTEGER retLen = 0;
	char dummyBuffer[1];
	char *wkbBuffer;
	char *valueBuffer;
	char oidBuffer[ 16 ];		/* assuming the OID will always be a long this should be enough */
    long record_oid;
	int t;

    /* for coercing single types into geometry collections */
    char *wkbTemp;
    int geomType;

    layerinfo = getMSSQL2008LayerInfo(layer);

    if(!layerinfo)
	{
        msSetError(MS_QUERYERR, "GetShape called with layerinfo = NULL", "msMSSQL2008LayerGetShape()");
        return MS_FAILURE;
    }

    if(!layerinfo->conn)
	{
        msSetError(MS_QUERYERR, "NextShape called on MSSQL2008 layer with no connection to DB.", "msMSSQL2008LayerGetShape()");
        return MS_FAILURE;
    }

    shape->type = MS_SHAPE_NULL;

    while(shape->type == MS_SHAPE_NULL) 
    {
        /* SQLRETURN rc = SQLFetchScroll(layerinfo->conn->hstmt, SQL_FETCH_ABSOLUTE, (SQLINTEGER) (*record) + 1); */

        /* We only do forward fetches. the parameter 'record' is ignored, but is incremented */
        SQLRETURN rc = SQLFetch(layerinfo->conn->hstmt);

        /* Any error assume out of recordset bounds */
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        {
            handleSQLError(layer);
            return MS_DONE;
        }

        /* retreive an item */

        {
            /* have to retrieve shape attributes */
            shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
            shape->numvalues = layer->numitems;

            for(t=0; t < layer->numitems; t++)
			{
				/* figure out how big the buffer needs to be */
                rc = SQLGetData(layerinfo->conn->hstmt, t + 1, SQL_C_BINARY, dummyBuffer, 0, &needLen);
                if (rc == SQL_ERROR)
                    handleSQLError(layer);

				if (needLen > 0)
                {
                    /* allocate the buffer - this will be a null-terminated string so alloc for the null too */
				    valueBuffer = (char*) malloc( needLen + 1 );
				    if ( valueBuffer == NULL )
				    {
					    msSetError( MS_QUERYERR, "Could not allocate value buffer.", "msMSSQL2008LayerGetShapeRandom()" );
					    return MS_FAILURE;
				    }

				    /* Now grab the data */
                    rc = SQLGetData(layerinfo->conn->hstmt, t + 1, SQL_C_BINARY, valueBuffer, needLen, &retLen);
                    if (rc == SQL_ERROR || rc == SQL_SUCCESS_WITH_INFO)
                        handleSQLError(layer);

				    /* Terminate the buffer */
                    valueBuffer[retLen] = 0; /* null terminate it */

				    /* Pop the value into the shape's value array */
                    shape->values[t] = valueBuffer;
                }
                else
                    /* Copy empty sting for NULL values */
                    shape->values[t] = strdup("");
            }

            /* Get shape geometry */
            {
				/* Set up to request the size of the buffer needed */
                rc = SQLGetData(layerinfo->conn->hstmt, layer->numitems + 1, SQL_C_BINARY, dummyBuffer, 0, &needLen);
                if (rc == SQL_ERROR)
                    handleSQLError(layer);

                /* allow space for coercion to geometry collection if needed*/
                wkbTemp = (char*)malloc(needLen+9);  

                /* write data above space allocated for geometry collection coercion */
                wkbBuffer = wkbTemp + 9; 

				if ( wkbBuffer == NULL )
				{
					msSetError( MS_QUERYERR, "Could not allocate value buffer.", "msMSSQL2008LayerGetShapeRandom()" );
					return MS_FAILURE;
				}

				/* Grab the WKB */
                rc = SQLGetData(layerinfo->conn->hstmt, layer->numitems + 1, SQL_C_BINARY, wkbBuffer, needLen, &retLen);
                if (rc == SQL_ERROR || rc == SQL_SUCCESS_WITH_INFO)
                    handleSQLError(layer);

                memcpy(&geomType, wkbBuffer + 1, 4);

                /* is this a single type? */
                if (geomType < 4)
                {
                    /* copy byte order marker (although we don't check it) */
                    wkbTemp[0] = wkbBuffer[0];
                    wkbBuffer = wkbTemp;

                    /* indicate type is geometry collection (although geomType + 3 would also work) */
                    wkbBuffer[1] = (char)7;
                    wkbBuffer[2] = (char)0;
                    wkbBuffer[3] = (char)0;
                    wkbBuffer[4] = (char)0;

                    /* indicate 1 shape */
                    wkbBuffer[5] = (char)1;
                    wkbBuffer[6] = (char)0;
                    wkbBuffer[7] = (char)0;
                    wkbBuffer[8] = (char)0;
                }

                switch(layer->type) 
                {
                    case MS_LAYER_POINT:
						result = force_to_points(wkbBuffer, shape);
                        break;

                    case MS_LAYER_LINE:
						result = force_to_lines(wkbBuffer, shape);
                        break;

                    case MS_LAYER_POLYGON:
						result = force_to_polygons(wkbBuffer, shape);
                        break;

                    case MS_LAYER_ANNOTATION:
                    case MS_LAYER_QUERY:
                    case MS_LAYER_CHART:
                        result = dont_force(wkbBuffer, shape);
                        break;

                    case MS_LAYER_RASTER:
                        msDebug( "Ignoring MS_LAYER_RASTER in mapMSSQL2008.c\n" );
                        break;

                    case MS_LAYER_CIRCLE:
                        msDebug( "Ignoring MS_LAYER_CIRCLE in mapMSSQL2008.c\n" );
                        break;

                    default:
                       msDebug( "Unsupported layer type in msMSSQL2008LayerNextShape()!" );
                       break;
                }

                //free(wkbBuffer);
                free(wkbTemp);
            }

            /* Next get unique id for row - since the OID shouldn't be larger than a long we'll assume billions as a limit */
            rc = SQLGetData(layerinfo->conn->hstmt, layer->numitems + 2, SQL_C_BINARY, oidBuffer, sizeof(oidBuffer) - 1, &retLen);
            if (rc == SQL_ERROR || rc == SQL_SUCCESS_WITH_INFO)
                    handleSQLError(layer);

			oidBuffer[retLen] = 0;
            record_oid = strtol(oidBuffer, NULL, 10);

            shape->index = record_oid;

            find_bounds(shape);
            (*record)++;        /* move to next shape */

            if(shape->type != MS_SHAPE_NULL) 
            {
                return MS_SUCCESS;
            }
            else
            {
                msDebug("msMSSQL2008LayerGetShapeRandom bad shape: %d\n", *record);
            }
            /* if (layer->type == MS_LAYER_POINT) {return MS_DONE;} */
        }
    }

    msFreeShape(shape);

    return MS_FAILURE;
}

/* find the next shape with the appropriate shape type (convert it if necessary) */
/* also, load in the attribute data */
/* MS_DONE => no more data */
int msMSSQL2008LayerNextShape(layerObj *layer, shapeObj *shape)
{
    int     result;

    msMSSQL2008LayerInfo  *layerinfo;

    layerinfo = getMSSQL2008LayerInfo(layer);

    if(!layerinfo)
	{
        msSetError(MS_QUERYERR, "NextShape called with layerinfo = NULL", "msMSSQL2008LayerNextShape()");
        return MS_FAILURE;
    }

    result = msMSSQL2008LayerGetShapeRandom(layer, shape, &(layerinfo->row_num));
    /* getshaperandom will increment the row_num */
    /* layerinfo->row_num   ++; */

    return result;
}

/* Execute a query on the DB based on record being an primary key. */
int msMSSQL2008LayerGetShape(layerObj *layer, shapeObj *shape, long record)
{
    char    *query_str;
    char    *columns_wanted = 0;

    msMSSQL2008LayerInfo  *layerinfo;
    int                 t;
    char buffer[32000] = "";

    if(layer->debug) {
        msDebug("msMSSQL2008LayerGetShape called for record = %i\n", record);
    }

    layerinfo = getMSSQL2008LayerInfo(layer);

    if(!layerinfo) {
        /* Layer not open */
        msSetError(MS_QUERYERR, "msMSSQL2008LayerGetShape called on unopened layer (layerinfo = NULL)", "msMSSQL2008LayerGetShape()");

        return MS_FAILURE;
    }

    if(layer->numitems == 0) 
    {
        snprintf(buffer, sizeof(buffer), "%s.STAsBinary(), convert(varchar(36), %s)", layerinfo->geom_column, layerinfo->urid_name);
        columns_wanted = _strdup(buffer);
    } 
    else 
    {
        for(t = 0; t < layer->numitems; t++) {
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "convert(varchar(max), %s),", layer->items[t]);
        }

        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%s.STAsBinary(), convert(varchar(36), %s)", layerinfo->geom_column, layerinfo->urid_name);

        columns_wanted = _strdup(buffer);
    }

	/* index_name is ignored here since the hint should be for the spatial index, not the PK index */
    snprintf(buffer, sizeof(buffer), "select %s from %s where %s = %d", columns_wanted, layerinfo->geom_table, layerinfo->urid_name, record);

    query_str = _strdup(buffer);

    if(layer->debug) {
        msDebug("msMSSQL2008LayerGetShape: %s \n", query_str);
    }

    free(columns_wanted);

    if (!executeSQL(layerinfo->conn, query_str))
	{
        msSetError(MS_QUERYERR, "Error executing MSSQL2008 SQL statement: %s\n-%s\n", "msMSSQL2008LayerGetShape()", 
            query_str, layerinfo->conn->errorMessage);

        free(query_str);

        return MS_FAILURE;
    }

    layerinfo->sql = query_str;
    layerinfo->row_num = 0;

	return msMSSQL2008LayerGetShapeRandom(layer, shape, &(layerinfo->row_num));
}

/* Query the DB for info about the requested table */
int msMSSQL2008LayerGetItems(layerObj *layer)
{
    msMSSQL2008LayerInfo  *layerinfo;
    char                *table_name = 0;
    char                *geom_column_name = 0;
    char                *urid_name = 0;
    char                *user_srid = 0;
	char				*index_name = 0;
    char                sql[1000];
    int                 t;
    char                found_geom = 0;
    int                 item_num;
    SQLSMALLINT cols = 0;

    if(layer->debug) 
    {
        msDebug("in msMSSQL2008LayerGetItems  (find column names)\n");
    }

    layerinfo = getMSSQL2008LayerInfo(layer);

    if(!layerinfo) 
    {
        /* layer not opened yet */
        msSetError(MS_QUERYERR, "msMSSQL2008LayerGetItems called on unopened layer", "msMSSQL2008LayerGetItems()");

        return MS_FAILURE;
    }

    if(!layerinfo->conn) 
    {
        msSetError(MS_QUERYERR, "msMSSQL2008LayerGetItems called on MSSQL2008 layer with no connection to DB.", "msMSSQL2008LayerGetItems()");

        return MS_FAILURE;
    }

    sprintf(sql, "SELECT top 0 * FROM %s", layerinfo->geom_table); 

    if (!executeSQL(layerinfo->conn, sql))
    {
        free(geom_column_name);

        return MS_FAILURE;
    }

    SQLNumResultCols (layerinfo->conn->hstmt, &cols);

    layer->numitems = cols - 1; /* dont include the geometry column */

    layer->items = malloc(sizeof(char *) * (layer->numitems + 1)); /* +1 in case there is a problem finding goeometry column */
                                                                /* it will return an error if there is no geometry column found, */
                                                                /* so this isnt a problem */

    found_geom = 0; /* havent found the geom field */
    item_num = 0;

    for(t = 0; t < cols; t++) 
    {
        char colBuff[256];

        columnName(layerinfo->conn, t + 1, colBuff, sizeof(colBuff));

        if(strcmp(colBuff, layerinfo->geom_column) != 0) {
            /* this isnt the geometry column */
            layer->items[item_num] = (char *) malloc(strlen(colBuff) + 1);
            strcpy(layer->items[item_num], colBuff);
            item_num++;
        } else {
            found_geom = 1;
        }
    }

    if(!found_geom)
	{
        msSetError(MS_QUERYERR, "msMSSQL2008LayerGetItems: tried to find the geometry column in the results from the database, but couldnt find it.  Is it miss-capitialized? '%s'", "msMSSQL2008LayerGetItems()", layerinfo->geom_column);
        return MS_FAILURE;
    }

    return msMSSQL2008LayerInitItemInfo(layer);
}

/* Get the layer extent as specified in the mapfile or a largest area */
/* covering all features */
int msMSSQL2008LayerGetExtent(layerObj *layer, rectObj *extent)
{
    if(layer->debug) {
        msDebug("msMSSQL2008LayerGetExtent called\n");
    }

    if (layer->extent.minx == -1.0 && layer->extent.miny == -1.0 &&
        layer->extent.maxx == -1.0 && layer->extent.maxy == -1.0)
    {
        extent->minx = extent->miny = -1.0 * FLT_MAX;
        extent->maxx = extent->maxy = FLT_MAX;
    }
    else
    {
        extent->minx = layer->extent.minx;
        extent->miny = layer->extent.miny;
        extent->maxx = layer->extent.maxx;
        extent->maxy = layer->extent.maxy;
    }

    return MS_SUCCESS;
}

/* Get primary key column of table */
int msMSSQL2008LayerRetrievePK(layerObj *layer, char **urid_name, char* table_name, int debug) 
{

    char        sql[1024];
    msMSSQL2008LayerInfo *layerinfo = 0;
    SQLRETURN rc;

    snprintf(sql, sizeof(sql), 
        "SELECT     convert(varchar(50), sys.columns.name) AS ColumnName, sys.indexes.name "
        "FROM         sys.columns INNER JOIN "
        "                     sys.indexes INNER JOIN "
        "                     sys.tables ON sys.indexes.object_id = sys.tables.object_id INNER JOIN "
        "                     sys.index_columns ON sys.indexes.object_id = sys.index_columns.object_id AND sys.indexes.index_id = sys.index_columns.index_id ON "
        "                     sys.columns.object_id = sys.index_columns.object_id AND sys.columns.column_id = sys.index_columns.column_id "
        "WHERE     (sys.indexes.is_primary_key = 1) AND (sys.tables.name = N'%s') ",
        table_name);
    
    if (debug)
    {
       msDebug("msMSSQL2008LayerRetrievePK: query = %s\n", sql);
    }

    layerinfo = (msMSSQL2008LayerInfo *) layer->layerinfo;

    if(layerinfo->conn == NULL) 
    {

      msSetError(MS_QUERYERR, "Layer does not have a MSSQL2008 connection.", "msMSSQL2008LayerRetrievePK()");

      return(MS_FAILURE);
    }

    /* error somewhere above here in this method */

    if(!executeSQL(layerinfo->conn, sql)) 
    {
        char *tmp1;
        char *tmp2 = NULL;

        tmp1 = "Error executing MSSQL2008 statement (msMSSQL2008LayerRetrievePK():";
        tmp2 = (char *)malloc(sizeof(char)*(strlen(tmp1) + strlen(sql) + 1));
        strcpy(tmp2, tmp1);
        strcat(tmp2, sql);
        msSetError(MS_QUERYERR, tmp2, "msMSSQL2008LayerRetrievePK()");
        free(tmp2);
        return(MS_FAILURE);
    }

    rc = SQLFetch(layerinfo->conn->hstmt);

    if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) 
    {
        if(debug) 
        {
            msDebug("msMSSQL2008LayerRetrievePK: No results found.\n");
        }

        return MS_FAILURE;
    }

    {
        char buff[100];
        SQLINTEGER retLen;
        rc = SQLGetData(layerinfo->conn->hstmt, 1, SQL_C_BINARY, buff, sizeof(buff), &retLen);

        rc = SQLFetch(layerinfo->conn->hstmt);

        if(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) 
        {
            if(debug) 
            {
                msDebug("msMSSQL2008LayerRetrievePK: Multiple primary key columns are not supported in MapServer\n");
            }

            return MS_FAILURE;
        }

        buff[retLen] = 0;

        *urid_name = _strdup(buff);
    }

    return MS_SUCCESS;
}

/* Function to parse the Mapserver DATA parameter for geometry
 * column name, table name and name of a column to serve as a
 * unique record id
 */
static int msMSSQL2008LayerParseData(layerObj *layer, char **geom_column_name, char **table_name, char **urid_name, char **user_srid, char **index_name, int debug)
{
    char    *pos_opt, *pos_scn, *tmp, *pos_srid, *pos_urid, *pos_indexHint, *data;
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
        *user_srid = (char *) malloc(2);
        (*user_srid)[0] = '0';
        (*user_srid)[1] = 0;
    } else {
        slength = strspn(pos_srid + 12, "-0123456789");
        if(!slength) {
            msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerParseData()", "Error parsing MSSQL2008 data variable: You specified 'using SRID=#' but didnt have any numbers!<br><br>\n\nMore Help:<br><br>\n\n", data);

            return MS_FAILURE;
        } else {
            *user_srid = (char *) malloc(slength + 1);
            strncpy(*user_srid, pos_srid + 12, slength);
            (*user_srid)[slength] = 0; /* null terminate it */
        }
    }

	pos_indexHint = strstrIgnoreCase(data, " using index ");
    if(pos_indexHint) {
        /* CHANGE - protect the trailing edge for thing like 'using unique ftab_id using srid=33' */
        tmp = strstr(pos_indexHint + 13, " ");
        if(!tmp) {
            tmp = pos_indexHint + strlen(pos_indexHint);
        }
        *index_name = (char *) malloc((tmp - (pos_indexHint + 13)) + 1);
        strncpy(*index_name, pos_indexHint + 13, tmp - (pos_indexHint + 13));
        (*index_name)[tmp - (pos_indexHint + 13)] = 0;
    }

    /* this is a little hack so the rest of the code works.  If the ' using SRID=' comes before */
    /* the ' using unique ', then make sure pos_opt points to where the ' using SRID' starts! */
    pos_opt = pos_urid;
	if ( !pos_opt || ( pos_srid && pos_srid < pos_opt ) ) pos_opt = pos_srid;
	if ( !pos_opt || ( pos_indexHint && pos_indexHint < pos_opt ) ) pos_opt = pos_indexHint;
	if ( !pos_opt ) pos_opt = data + strlen(data);

    /* Scan for the table or sub-select clause */
    pos_scn = strstrIgnoreCase(data, " from ");
    if(!pos_scn) {
        msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerParseData()", "Error parsing MSSQL2008 data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find ' from ').  More help: <br><br>\n\n", data);

        return MS_FAILURE;
    }

    /* Copy the geometry column name */
    *geom_column_name = (char *) malloc((pos_scn - data) + 1);
    strncpy(*geom_column_name, data, pos_scn - data);
    (*geom_column_name)[pos_scn - data] = 0;

    /* Copy out the table name or sub-select clause */
    *table_name = (char *) malloc((pos_opt - (pos_scn + 6)) + 1);
    strncpy(*table_name, pos_scn + 6, pos_opt - (pos_scn + 6));
    (*table_name)[pos_opt - (pos_scn + 6)] = 0;

    if(strlen(*table_name) < 1 || strlen(*geom_column_name) < 1) {
        msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerParseData()", "Error parsing MSSQL2008 data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find a geometry_column or table/subselect).  More help: <br><br>\n\n", data);

        return MS_FAILURE;
    }

    if( !pos_urid ) {
        if( msMSSQL2008LayerRetrievePK(layer, urid_name, *table_name, debug) != MS_SUCCESS ) {
            msSetError(MS_QUERYERR, DATA_ERROR_MESSAGE, "msMSSQL2008LayerParseData()", "No primary key defined for table, or primary key contains more that one column\n\n", 
                *table_name);

            return MS_FAILURE;
        }
    }

    if(debug) {
        msDebug("msMSSQL2008LayerParseData: unique column = %s, srid='%s', geom_column_name = %s, table_name=%s\n", *urid_name, *user_srid, *geom_column_name, *table_name);
    }

    return MS_SUCCESS;
}

#else

/* prototypes if MSSQL2008 isnt supposed to be compiled */

int msMSSQL2008LayerOpen(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerOpen called but unimplemented!  (mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerOpen()");
    return MS_FAILURE;
}

int msMSSQL2008LayerIsOpen(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msMSSQL2008IsLayerOpen called but unimplemented!  (mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerIsOpen()");
    return MS_FALSE;
}

void msMSSQL2008LayerFreeItemInfo(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerFreeItemInfo called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerFreeItemInfo()");
}

int msMSSQL2008LayerInitItemInfo(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerInitItemInfo called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerInitItemInfo()");
    return MS_FAILURE;
}

int msMSSQL2008LayerWhichShapes(layerObj *layer, rectObj rect)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerWhichShapes called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerWhichShapes()");
    return MS_FAILURE;
}

int msMSSQL2008LayerClose(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerClose called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerClose()");
    return MS_FAILURE;
}

int msMSSQL2008LayerNextShape(layerObj *layer, shapeObj *shape)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerNextShape called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerNextShape()");
    return MS_FAILURE;
}

int msMSSQL2008LayerGetShape(layerObj *layer, shapeObj *shape, long record)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetShape called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetShape()");
    return MS_FAILURE;
}

int msMSSQL2008LayerGetExtent(layerObj *layer, rectObj *extent)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetExtent called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetExtent()");
    return MS_FAILURE;
}

int msMSSQL2008LayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetShapeRandom called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetShapeRandom()");
    return MS_FAILURE;
}

int msMSSQL2008LayerGetItems(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msMSSQL2008LayerGetItems called but unimplemented!(mapserver not compiled with MSSQL2008 support)", "msMSSQL2008LayerGetItems()");
    return MS_FAILURE;
}


/* end above's #ifdef USE_MSSQL2008 */
#endif

int 
msMSSQL2008LayerGetShapeVT(layerObj *layer, shapeObj *shape, int tile, long record)
{
    return msMSSQL2008LayerGetShape(layer, shape, record);
}

#ifdef USE_MSSQL2008_PLUGIN

MS_DLL_EXPORT  int
PluginInitializeVirtualTable(layerVTableObj* vtable, layerObj *layer)
{
    assert(layer != NULL);
    assert(vtable != NULL);

    vtable->LayerInitItemInfo = msMSSQL2008LayerInitItemInfo;
    vtable->LayerFreeItemInfo = msMSSQL2008LayerFreeItemInfo;
    vtable->LayerOpen = msMSSQL2008LayerOpen;
    vtable->LayerIsOpen = msMSSQL2008LayerIsOpen;
    vtable->LayerWhichShapes = msMSSQL2008LayerWhichShapes;
    vtable->LayerNextShape = msMSSQL2008LayerNextShape;
    vtable->LayerGetShape = msMSSQL2008LayerGetShapeVT;
    vtable->LayerResultsGetShape = msMSSQL2008LayerGetShapeVT; /* no special version, use ...GetShape() */

    vtable->LayerClose = msMSSQL2008LayerClose;

    vtable->LayerGetItems = msMSSQL2008LayerGetItems;
    vtable->LayerGetExtent = msMSSQL2008LayerGetExtent;

    vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;

    /* vtable->LayerGetAutoStyle, not supported for this layer */
    vtable->LayerCloseConnection = msMSSQL2008LayerClose;
    
    vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;
    /* vtable->LayerCreateItems, use default */
    /* vtable->LayerGetNumFeatures, use default */

    return MS_SUCCESS;
}

#endif

int
msMSSQL2008LayerInitializeVirtualTable(layerObj *layer)
{
    assert(layer != NULL);
    assert(layer->vtable != NULL);

    layer->vtable->LayerInitItemInfo = msMSSQL2008LayerInitItemInfo;
    layer->vtable->LayerFreeItemInfo = msMSSQL2008LayerFreeItemInfo;
    layer->vtable->LayerOpen = msMSSQL2008LayerOpen;
    layer->vtable->LayerIsOpen = msMSSQL2008LayerIsOpen;
    layer->vtable->LayerWhichShapes = msMSSQL2008LayerWhichShapes;
    layer->vtable->LayerNextShape = msMSSQL2008LayerNextShape;
    layer->vtable->LayerGetShape = msMSSQL2008LayerGetShapeVT;
    layer->vtable->LayerResultsGetShape = msMSSQL2008LayerGetShapeVT; /* no special version, use ...GetShape() */
    
    layer->vtable->LayerClose = msMSSQL2008LayerClose;

    layer->vtable->LayerGetItems = msMSSQL2008LayerGetItems;
    layer->vtable->LayerGetExtent = msMSSQL2008LayerGetExtent;

    layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;

    /* layer->vtable->LayerGetAutoStyle, not supported for this layer */
    layer->vtable->LayerCloseConnection = msMSSQL2008LayerClose;
    
    layer->vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;
    /* layer->vtable->LayerCreateItems, use default */
    /* layer->vtable->LayerGetNumFeatures, use default */


    return MS_SUCCESS;
}

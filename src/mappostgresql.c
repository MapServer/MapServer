/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Postgres CONNECTIONTYPE support.
 * Author:   Mark Leslie, Refractions Research
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

typedef struct {
  PGconn *conn; /* connection to db */
  long row_num; /* what row is the NEXT to be read (for random access) */
  PGresult *query_result; /* for fetching rows from the db */
  int from_index;
  char *to_column;
  char *from_value;
  int layer_debug; /* there's no debug on the join, so use the layer */
} msPOSTGRESQLJoinInfo;

/************************************************************************/
/*                      msPOSTGRESQLJoinConnect()                       */
/*                                                                      */
/* Creates and populates the joininfo struct, including establishing    */
/* a connection to the database.  Since the join and layer won't always */
/* share connection details, there is currently no mechanism to use     */
/* pooled connections with joins.                                       */
/************************************************************************/

int msPOSTGRESQLJoinConnect(layerObj *layer, joinObj *join) {
  char *maskeddata, *temp, *sql, *column;
  char *conn_decrypted;
  int i, test;
  PGresult *query_result;
  msPOSTGRESQLJoinInfo *joininfo;

  if (join->joininfo)
    return MS_SUCCESS;

  joininfo = (msPOSTGRESQLJoinInfo *)malloc(sizeof(msPOSTGRESQLJoinInfo));
  if (!joininfo) {
    msSetError(MS_MEMERR, "Error allocating join info struct.",
               "msPOSTGRESQLJoinConnect()");
    return MS_FAILURE;
  }
  joininfo->conn = NULL;
  joininfo->row_num = 0;
  joininfo->query_result = NULL;
  joininfo->from_index = 0;
  joininfo->to_column = join->to;
  joininfo->from_value = NULL;
  joininfo->layer_debug = layer->debug;
  join->joininfo = joininfo;

  /*
   * We need three things at a minimum, the connection string, a table
   * name, and a column to join on.
   */
  if (!join->connection) {
    msSetError(MS_QUERYERR, "No connection information provided.",
               "MSPOSTGRESQLJoinConnect()");
    return MS_FAILURE;
  }
  if (!join->table) {
    msSetError(MS_QUERYERR, "No join table name found.",
               "msPOSTGRESQLJoinConnect()");
    return MS_FAILURE;
  }
  if (!joininfo->to_column) {
    msSetError(MS_QUERYERR, "No join to column name found.",
               "msPOSTGRESQLJoinConnect()");
    return MS_FAILURE;
  }

  /* Establish database connection */
  conn_decrypted = msDecryptStringTokens(layer->map, join->connection);
  if (conn_decrypted != NULL) {
    joininfo->conn = PQconnectdb(conn_decrypted);
    free(conn_decrypted);
  }
  if (!joininfo->conn || PQstatus(joininfo->conn) == CONNECTION_BAD) {
    maskeddata = (char *)malloc(strlen(layer->connection) + 1);
    strcpy(maskeddata, join->connection);
    temp = strstr(maskeddata, "password=");
    if (temp) {
      temp = (char *)(temp + 9);
      while (*temp != '\0' && *temp != ' ') {
        *temp = '*';
        temp++;
      }
    }
    msSetError(MS_QUERYERR,
               "Unable to connect to PostgreSQL using the string %s.\n  Error "
               "reported: %s\n",
               "msPOSTGRESQLJoinConnect()", maskeddata,
               PQerrorMessage(joininfo->conn));
    free(maskeddata);
    if (!joininfo->conn) {
      free(joininfo->conn);
    }
    free(joininfo);
    join->joininfo = NULL;
    return MS_FAILURE;
  }

  /* Determine the number and names of columns in the join table. */
  sql = (char *)malloc(36 + strlen(join->table) + 1);
  sprintf(sql, "SELECT * FROM %s WHERE false LIMIT 0", join->table);

  if (joininfo->layer_debug) {
    msDebug("msPOSTGRESQLJoinConnect(): executing %s.\n", sql);
  }

  query_result = PQexec(joininfo->conn, sql);
  if (!query_result || PQresultStatus(query_result) != PGRES_TUPLES_OK) {
    msSetError(MS_QUERYERR, "Error determining join items: %s.",
               "msPOSTGRESQLJoinConnect()", PQerrorMessage(joininfo->conn));
    if (query_result) {
      PQclear(query_result);
      query_result = NULL;
    }
    free(sql);
    return MS_FAILURE;
  }
  free(sql);
  join->numitems = PQnfields(query_result);
  join->items = malloc(sizeof(char *) * (join->numitems));

  /* We want the join-to column to be first in the list. */
  test = 1;
  for (i = 0; i < join->numitems; i++) {
    column = PQfname(query_result, i);
    if (strcmp(column, joininfo->to_column) != 0) {
      join->items[i + test] = (char *)malloc(strlen(column) + 1);
      strcpy(join->items[i + test], column);
    } else {
      test = 0;
      join->items[0] = (char *)malloc(strlen(column) + 1);
      strcpy(join->items[0], column);
    }
  }
  PQclear(query_result);
  query_result = NULL;
  if (test == 1) {
    msSetError(MS_QUERYERR, "Unable to find join to column: %s",
               "msPOSTGRESQLJoinConnect()", joininfo->to_column);
    return MS_FAILURE;
  }

  if (joininfo->layer_debug) {
    for (i = 0; i < join->numitems; i++) {
      msDebug("msPOSTGRESQLJoinConnect(): Column %d named %s\n", i,
              join->items[i]);
    }
  }

  /* Determine the index of the join from column. */
  for (i = 0; i < layer->numitems; i++) {
    if (strcasecmp(layer->items[i], join->from) == 0) {
      joininfo->from_index = i;
      break;
    }
  }

  if (i == layer->numitems) {
    msSetError(MS_JOINERR, "Item %s not found in layer %s.",
               "msPOSTGRESQLJoinConnect()", join->from, layer->name);
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                                                                      */
/*                       msPOSTGRESQLJoinPrepare()                      */
/* Sets up the joininfo to be ready to join against the given shape.    */
/* There's not much involved here, just freeing previous results and    */
/* resources, and setting the next value to join to.                    */
/************************************************************************/

int msPOSTGRESQLJoinPrepare(joinObj *join, shapeObj *shape) {

  /* We need a connection, and a shape with values to join to. */
  msPOSTGRESQLJoinInfo *joininfo = join->joininfo;
  if (!joininfo) {
    msSetError(MS_JOINERR, "Join has not been connected.",
               "msPOSTGRESQLJoinPrepare()");
    return MS_FAILURE;
  }

  if (!shape) {
    msSetError(MS_JOINERR, "Null shape provided for join.",
               "msPOSTGRESQLJoinPrepare()");
    return MS_FAILURE;
  }

  if (!shape->values) {
    msSetError(MS_JOINERR,
               "Shape has no attributes.  Kinda hard to join against.",
               "msPOSTGRESQLJoinPrepare()");
    return MS_FAILURE;
  }
  joininfo->row_num = 0;

  /* Free the previous join value, if any. */
  if (joininfo->from_value) {
    free(joininfo->from_value);
  }

  /* Free the previous results, if any. */
  if (joininfo->query_result) {
    PQclear(joininfo->query_result);
    joininfo->query_result = NULL;
  }

  /* Copy the next join value from the shape. */
  joininfo->from_value = msStrdup(shape->values[joininfo->from_index]);

  if (joininfo->layer_debug) {
    msDebug("msPOSTGRESQLJoinPrepare() preping for value %s.\n",
            joininfo->from_value);
  }

  return MS_SUCCESS;
}

/************************************************************************/
/*                       msPOSTGRESQLJoinNext()                         */
/*                                                                      */
/* The goal here is to populate join->values with the detail of the     */
/* join against the previously prepared shapeObj.  This will be called  */
/* only once for a one-to-one join, with msPOSTGRESQLJoinPrepare()      */
/* being called before each.  It will be called repeatedly for          */
/* one-to-many joins, until in returns MS_DONE.  To accommodate this,    */
/* we store the next row number and query results in the joininfo and   */
/* process the next tuple on each call.                                 */
/************************************************************************/
int msPOSTGRESQLJoinNext(joinObj *join) {
  msPOSTGRESQLJoinInfo *joininfo = join->joininfo;
  int i, length, row_count;
  char *sql, *columns;

  /* We need a connection, and a join value. */
  if (!joininfo || !joininfo->conn) {
    msSetError(MS_JOINERR, "Join has not been connected.\n",
               "msPOSTGRESQLJoinNext()");
    return MS_FAILURE;
  }

  if (!joininfo->from_value) {
    msSetError(MS_JOINERR, "Join has not been prepared.\n",
               "msPOSTGRESQLJoinNext()");
    return MS_FAILURE;
  }

  /* Free the previous results. */
  if (join->values) {
    msFreeCharArray(join->values, join->numitems);
    join->values = NULL;
  }

  /* We only need to execute the query if no results exist. */
  if (!joininfo->query_result) {
    /* Write the list of column names. */
    length = 0;
    for (i = 0; i < join->numitems; i++) {
      length += 8 + strlen(join->items[i]) + 2;
    }
    if (length > 1024 * 1024) {
      msSetError(MS_MEMERR, "Too many joins.\n", "msPOSTGRESQLJoinNext()");
      return MS_FAILURE;
    }

    columns = (char *)malloc(length + 1);
    if (!columns) {
      msSetError(MS_MEMERR, "Failure to malloc.\n", "msPOSTGRESQLJoinNext()");
      return MS_FAILURE;
    }

    columns[0] = 0;
    for (i = 0; i < join->numitems; i++) {
      strcat(columns, "\"");
      strcat(columns, join->items[i]);
      strcat(columns, "\"::text");
      if (i != join->numitems - 1) {
        strcat(columns, ", ");
      }
    }

    /* Create the query string. */
    const size_t nSize = 26 + strlen(columns) + strlen(join->table) +
                         strlen(join->to) + strlen(joininfo->from_value);
    sql = (char *)malloc(nSize);
    if (!sql) {
      msSetError(MS_MEMERR, "Failure to malloc.\n", "msPOSTGRESQLJoinNext()");
      return MS_FAILURE;
    }
    snprintf(sql, nSize, "SELECT %s FROM %s WHERE %s = '%s'", columns,
             join->table, join->to, joininfo->from_value);
    if (joininfo->layer_debug) {
      msDebug("msPOSTGRESQLJoinNext(): executing %s.\n", sql);
    }

    free(columns);

    joininfo->query_result = PQexec(joininfo->conn, sql);

    if (!joininfo->query_result ||
        PQresultStatus(joininfo->query_result) != PGRES_TUPLES_OK) {
      msSetError(MS_QUERYERR, "Error executing queri %s: %s\n",
                 "msPOSTGRESQLJoinNext()", sql, PQerrorMessage(joininfo->conn));
      if (joininfo->query_result) {
        PQclear(joininfo->query_result);
        joininfo->query_result = NULL;
      }
      free(sql);
      return MS_FAILURE;
    }
    free(sql);
  }
  row_count = PQntuples(joininfo->query_result);

  /* see if we're done processing this set */
  if (joininfo->row_num >= row_count) {
    return (MS_DONE);
  }
  if (joininfo->layer_debug) {
    msDebug("msPOSTGRESQLJoinNext(): fetching row %ld.\n", joininfo->row_num);
  }

  /* Copy the resulting values into the joinObj. */
  join->values = (char **)malloc(sizeof(char *) * join->numitems);
  for (i = 0; i < join->numitems; i++) {
    join->values[i] =
        msStrdup(PQgetvalue(joininfo->query_result, joininfo->row_num, i));
  }

  joininfo->row_num++;

  return MS_SUCCESS;
}

/************************************************************************/
/*                       msPOSTGRESQLJoinClose()                        */
/*                                                                      */
/* Closes the connection and frees the resources used by the joininfo.  */
/************************************************************************/

int msPOSTGRESQLJoinClose(joinObj *join) {
  msPOSTGRESQLJoinInfo *joininfo = join->joininfo;

  if (!joininfo) {
    msDebug("msPOSTGRESQLJoinClose() already close or never opened.\n");
    return MS_SUCCESS;
  }

  if (joininfo->query_result) {
    msDebug("msPOSTGRESQLJoinClose(): clearing query_result.\n");
    PQclear(joininfo->query_result);
    joininfo->query_result = NULL;
  }

  if (joininfo->conn) {
    msDebug("msPOSTGRESQLJoinClose(): closing connection.\n");
    PQfinish(joininfo->conn);
    joininfo->conn = NULL;
  }

  /* removed free(joininfo->to_column), see bug #2936 */

  if (joininfo->from_value) {
    free(joininfo->from_value);
  }

  free(joininfo);
  join->joininfo = NULL;

  return MS_SUCCESS;
}

#else /* not USE_POSTGIS */
int msPOSTGRESQLJoinConnect(layerObj *layer, joinObj *join) {
  msSetError(MS_QUERYERR, "PostgreSQL support not available.",
             "msPOSTGRESQLJoinConnect()");
  return MS_FAILURE;
}

int msPOSTGRESQLJoinPrepare(joinObj *join, shapeObj *shape) {
  msSetError(MS_QUERYERR, "PostgreSQL support not available.",
             "msPOSTGRESQLJoinPrepare()");
  return MS_FAILURE;
}

int msPOSTGRESQLJoinNext(joinObj *join) {
  msSetError(MS_QUERYERR, "PostgreSQL support not available.",
             "msPOSTGRESQLJoinNext()");
  return MS_FAILURE;
}

int msPOSTGRESQLJoinClose(joinObj *join) {
  msSetError(MS_QUERYERR, "PostgreSQL support not available.",
             "msPOSTGRESQLJoinClose()");
  return MS_FAILURE;
}
#endif

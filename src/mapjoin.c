/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implements MapServer joins.
 * Author:   Steve Lime and the MapServer team.
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

#include "mapserver.h"

#define ROW_ALLOCATION_SIZE 10

/* DBF/XBase function prototypes */
int msDBFJoinConnect(layerObj *layer, joinObj *join);
int msDBFJoinPrepare(joinObj *join, shapeObj *shape);
int msDBFJoinNext(joinObj *join);
int msDBFJoinClose(joinObj *join);
int msDBFJoinTable(layerObj *layer, joinObj *join, shapeObj *shape);

/* CSV (comma delimited text file) function prototypes */
int msCSVJoinConnect(layerObj *layer, joinObj *join);
int msCSVJoinPrepare(joinObj *join, shapeObj *shape);
int msCSVJoinNext(joinObj *join);
int msCSVJoinClose(joinObj *join);
int msCSVJoinTable(layerObj *layer, joinObj *join, shapeObj *shape);

/* MySQL function prototypes */
int msMySQLJoinConnect(layerObj *layer, joinObj *join);
int msMySQLJoinPrepare(joinObj *join, shapeObj *shape);
int msMySQLJoinNext(joinObj *join);
int msMySQLJoinClose(joinObj *join);
int msMySQLJoinTable(layerObj *layer, joinObj *join, shapeObj *shape);

/* PostgreSQL function prototypes */
int msPOSTGRESQLJoinConnect(layerObj *layer, joinObj *join);
int msPOSTGRESQLJoinPrepare(joinObj *join, shapeObj *shape);
int msPOSTGRESQLJoinNext(joinObj *join);
int msPOSTGRESQLJoinClose(joinObj *join);

/* wrapper function for DB specific join functions */
int msJoinConnect(layerObj *layer, joinObj *join) {
  switch (join->connectiontype) {
  case (MS_DB_XBASE):
    return msDBFJoinConnect(layer, join);
    break;
  case (MS_DB_CSV):
    return msCSVJoinConnect(layer, join);
    break;
  case (MS_DB_MYSQL):
    return msMySQLJoinConnect(layer, join);
    break;
  case (MS_DB_POSTGRES):
    return msPOSTGRESQLJoinConnect(layer, join);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.",
             "msJoinConnect()");
  return MS_FAILURE;
}

int msJoinPrepare(joinObj *join, shapeObj *shape) {
  switch (join->connectiontype) {
  case (MS_DB_XBASE):
    return msDBFJoinPrepare(join, shape);
    break;
  case (MS_DB_CSV):
    return msCSVJoinPrepare(join, shape);
    break;
  case (MS_DB_MYSQL):
    return msMySQLJoinPrepare(join, shape);
    break;
  case (MS_DB_POSTGRES):
    return msPOSTGRESQLJoinPrepare(join, shape);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.",
             "msJoinPrepare()");
  return MS_FAILURE;
}

int msJoinNext(joinObj *join) {
  switch (join->connectiontype) {
  case (MS_DB_XBASE):
    return msDBFJoinNext(join);
    break;
  case (MS_DB_CSV):
    return msCSVJoinNext(join);
    break;
  case (MS_DB_MYSQL):
    return msMySQLJoinNext(join);
    break;
  case (MS_DB_POSTGRES):
    return msPOSTGRESQLJoinNext(join);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msJoinNext()");
  return MS_FAILURE;
}

int msJoinClose(joinObj *join) {
  switch (join->connectiontype) {
  case (MS_DB_XBASE):
    return msDBFJoinClose(join);
    break;
  case (MS_DB_CSV):
    return msCSVJoinClose(join);
    break;
  case (MS_DB_MYSQL):
    return msMySQLJoinClose(join);
    break;
  case (MS_DB_POSTGRES):
    return msPOSTGRESQLJoinClose(join);
    break;
  default:
    break;
  }

  msSetError(MS_JOINERR, "Unsupported join connection type.", "msJoinClose()");
  return MS_FAILURE;
}

/*  */
/* XBASE join functions */
/*  */
typedef struct {
  DBFHandle hDBF;
  int fromindex, toindex;
  char *target;
  int nextrecord;
} msDBFJoinInfo;

int msDBFJoinConnect(layerObj *layer, joinObj *join) {
  int i;
  char szPath[MS_MAXPATHLEN];
  msDBFJoinInfo *joininfo;

  if (join->joininfo)
    return (MS_SUCCESS); /* already open */

  if (msCheckParentPointer(layer->map, "map") == MS_FAILURE)
    return MS_FAILURE;

  /* allocate a msDBFJoinInfo struct */
  joininfo = (msDBFJoinInfo *)malloc(sizeof(msDBFJoinInfo));
  if (!joininfo) {
    msSetError(MS_MEMERR, "Error allocating XBase table info structure.",
               "msDBFJoinConnect()");
    return (MS_FAILURE);
  }

  /* initialize any members that won't get set later on in this function */
  joininfo->target = NULL;
  joininfo->nextrecord = 0;

  join->joininfo = joininfo;

  /* open the XBase file */
  if ((joininfo->hDBF =
           msDBFOpen(msBuildPath3(szPath, layer->map->mappath,
                                  layer->map->shapepath, join->table),
                     "rb")) == NULL) {
    if ((joininfo->hDBF =
             msDBFOpen(msBuildPath(szPath, layer->map->mappath, join->table),
                       "rb")) == NULL) {
      msSetError(MS_IOERR, "(%s)", "msDBFJoinConnect()", join->table);
      return (MS_FAILURE);
    }
  }

  /* get "to" item index */
  if ((joininfo->toindex = msDBFGetItemIndex(joininfo->hDBF, join->to)) == -1) {
    msSetError(MS_DBFERR, "Item %s not found in table %s.",
               "msDBFJoinConnect()", join->to, join->table);
    return (MS_FAILURE);
  }

  /* get "from" item index   */
  for (i = 0; i < layer->numitems; i++) {
    if (strcasecmp(layer->items[i], join->from) == 0) { /* found it */
      joininfo->fromindex = i;
      break;
    }
  }

  if (i == layer->numitems) {
    msSetError(MS_JOINERR, "Item %s not found in layer %s.",
               "msDBFJoinConnect()", join->from, layer->name);
    return (MS_FAILURE);
  }

  /* finally store away the item names in the XBase table */
  join->numitems = msDBFGetFieldCount(joininfo->hDBF);
  join->items = msDBFGetItems(joininfo->hDBF);
  if (!join->items)
    return (MS_FAILURE);

  return (MS_SUCCESS);
}

int msDBFJoinPrepare(joinObj *join, shapeObj *shape) {
  msDBFJoinInfo *joininfo = join->joininfo;

  if (!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.",
               "msDBFJoinPrepare()");
    return (MS_FAILURE);
  }

  if (!shape) {
    msSetError(MS_JOINERR, "Shape to be joined is empty.",
               "msDBFJoinPrepare()");
    return (MS_FAILURE);
  }

  if (!shape->values) {
    msSetError(MS_JOINERR, "Shape to be joined has no attributes.",
               "msDBFJoinPrepare()");
    return (MS_FAILURE);
  }

  joininfo->nextrecord = 0; /* starting with the first record */

  if (joininfo->target)
    free(joininfo->target); /* clear last target */
  joininfo->target = msStrdup(shape->values[joininfo->fromindex]);

  return (MS_SUCCESS);
}

int msDBFJoinNext(joinObj *join) {
  int i, n;
  msDBFJoinInfo *joininfo = join->joininfo;

  if (!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.",
               "msDBFJoinNext()");
    return (MS_FAILURE);
  }

  if (!joininfo->target) {
    msSetError(MS_JOINERR, "No target specified, run msDBFJoinPrepare() first.",
               "msDBFJoinNext()");
    return (MS_FAILURE);
  }

  /* clear any old data */
  if (join->values) {
    msFreeCharArray(join->values, join->numitems);
    join->values = NULL;
  }

  n = msDBFGetRecordCount(joininfo->hDBF);

  for (i = joininfo->nextrecord; i < n; i++) { /* find a match */
    if (strcmp(joininfo->target, msDBFReadStringAttribute(joininfo->hDBF, i,
                                                          joininfo->toindex)) ==
        0)
      break;
  }

  if (i == n) { /* unable to do the join */
    if ((join->values = (char **)malloc(sizeof(char *) * join->numitems)) ==
        NULL) {
      msSetError(MS_MEMERR, NULL, "msDBFJoinNext()");
      return (MS_FAILURE);
    }
    for (i = 0; i < join->numitems; i++)
      join->values[i] = msStrdup("\0"); /* initialize to zero length strings */

    joininfo->nextrecord = n;
    return (MS_DONE);
  }

  if ((join->values = msDBFGetValues(joininfo->hDBF, i)) == NULL)
    return (MS_FAILURE);

  joininfo->nextrecord =
      i + 1; /* so we know where to start looking next time through */

  return (MS_SUCCESS);
}

int msDBFJoinClose(joinObj *join) {
  msDBFJoinInfo *joininfo = join->joininfo;

  if (!joininfo)
    return (MS_SUCCESS); /* already closed */

  if (joininfo->hDBF)
    msDBFClose(joininfo->hDBF);
  if (joininfo->target)
    free(joininfo->target);
  free(joininfo);
  joininfo = NULL;

  return (MS_SUCCESS);
}

/*  */
/* CSV (comma separated value) join functions */
/*  */
typedef struct {
  int fromindex, toindex;
  char *target;
  char ***rows;
  int numrows;
  int nextrow;
} msCSVJoinInfo;

int msCSVJoinConnect(layerObj *layer, joinObj *join) {
  int i;
  FILE *stream;
  char szPath[MS_MAXPATHLEN];
  msCSVJoinInfo *joininfo;
  char buffer[MS_BUFFER_LENGTH];

  if (join->joininfo)
    return (MS_SUCCESS); /* already open */
  if (msCheckParentPointer(layer->map, "map") == MS_FAILURE)
    return MS_FAILURE;

  /* allocate a msCSVJoinInfo struct */
  if ((joininfo = (msCSVJoinInfo *)malloc(sizeof(msCSVJoinInfo))) == NULL) {
    msSetError(MS_MEMERR, "Error allocating CSV table info structure.",
               "msCSVJoinConnect()");
    return (MS_FAILURE);
  }

  /* initialize any members that won't get set later on in this function */
  joininfo->target = NULL;
  joininfo->nextrow = 0;

  join->joininfo = joininfo;

  /* open the CSV file */
  if ((stream = fopen(msBuildPath3(szPath, layer->map->mappath,
                                   layer->map->shapepath, join->table),
                      "r")) == NULL) {
    if ((stream = fopen(msBuildPath(szPath, layer->map->mappath, join->table),
                        "r")) == NULL) {
      msSetError(MS_IOERR, "(%s)", "msCSVJoinConnect()", join->table);
      return (MS_FAILURE);
    }
  }

  /* once through to get the number of rows */
  joininfo->numrows = 0;
  while (fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL)
    joininfo->numrows++;
  rewind(stream);

  if ((joininfo->rows =
           (char ***)malloc(joininfo->numrows * sizeof(char **))) == NULL) {
    fclose(stream);
    msSetError(MS_MEMERR, "Error allocating rows.", "msCSVJoinConnect()");
    return (MS_FAILURE);
  }

  /* load the rows */
  i = 0;
  while (fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) {
    msStringTrimEOL(buffer);
    joininfo->rows[i] = msStringSplitComplex(buffer, ",", &(join->numitems),
                                             MS_ALLOWEMPTYTOKENS);
    i++;
  }
  fclose(stream);

  /* get "from" item index   */
  for (i = 0; i < layer->numitems; i++) {
    if (strcasecmp(layer->items[i], join->from) == 0) { /* found it */
      joininfo->fromindex = i;
      break;
    }
  }

  if (i == layer->numitems) {
    msSetError(MS_JOINERR, "Item %s not found in layer %s.",
               "msCSVJoinConnect()", join->from, layer->name);
    return (MS_FAILURE);
  }

  /* get "to" index (for now the user tells us which column, 1..n) */
  joininfo->toindex = atoi(join->to) - 1;
  if (joininfo->toindex < 0 || joininfo->toindex > join->numitems) {
    msSetError(MS_JOINERR, "Invalid column index %s.", "msCSVJoinConnect()",
               join->to);
    return (MS_FAILURE);
  }

  /* store away the column names (1..n) */
  if ((join->items = (char **)malloc(sizeof(char *) * join->numitems)) ==
      NULL) {
    msSetError(MS_MEMERR, "Error allocating space for join item names.",
               "msCSVJoinConnect()");
    return (MS_FAILURE);
  }
  for (i = 0; i < join->numitems; i++) {
    join->items[i] = (char *)malloc(12); /* plenty of space */
    sprintf(join->items[i], "%d", i + 1);
  }

  return (MS_SUCCESS);
}

int msCSVJoinPrepare(joinObj *join, shapeObj *shape) {
  msCSVJoinInfo *joininfo = join->joininfo;

  if (!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.",
               "msCSVJoinPrepare()");
    return (MS_FAILURE);
  }

  if (!shape) {
    msSetError(MS_JOINERR, "Shape to be joined is empty.",
               "msCSVJoinPrepare()");
    return (MS_FAILURE);
  }

  if (!shape->values) {
    msSetError(MS_JOINERR, "Shape to be joined has no attributes.",
               "msCSVJoinPrepare()");
    return (MS_FAILURE);
  }

  joininfo->nextrow = 0; /* starting with the first record */

  if (joininfo->target)
    free(joininfo->target); /* clear last target */
  joininfo->target = msStrdup(shape->values[joininfo->fromindex]);

  return (MS_SUCCESS);
}

int msCSVJoinNext(joinObj *join) {
  int i, j;
  msCSVJoinInfo *joininfo = join->joininfo;

  if (!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.",
               "msCSVJoinNext()");
    return (MS_FAILURE);
  }

  /* clear any old data */
  if (join->values) {
    msFreeCharArray(join->values, join->numitems);
    join->values = NULL;
  }

  for (i = joininfo->nextrow; i < joininfo->numrows; i++) { /* find a match */
    if (strcmp(joininfo->target, joininfo->rows[i][joininfo->toindex]) == 0)
      break;
  }

  if ((join->values = (char **)malloc(sizeof(char *) * join->numitems)) ==
      NULL) {
    msSetError(MS_MEMERR, NULL, "msCSVJoinNext()");
    return (MS_FAILURE);
  }

  if (i == joininfo->numrows) { /* unable to do the join     */
    for (j = 0; j < join->numitems; j++)
      join->values[j] = msStrdup("\0"); /* initialize to zero length strings */

    joininfo->nextrow = joininfo->numrows;
    return (MS_DONE);
  }

  for (j = 0; j < join->numitems; j++)
    join->values[j] = msStrdup(joininfo->rows[i][j]);

  joininfo->nextrow =
      i + 1; /* so we know where to start looking next time through */

  return (MS_SUCCESS);
}

int msCSVJoinClose(joinObj *join) {
  int i;
  msCSVJoinInfo *joininfo = join->joininfo;

  if (!joininfo)
    return (MS_SUCCESS); /* already closed */

  for (i = 0; i < joininfo->numrows; i++)
    msFreeCharArray(joininfo->rows[i], join->numitems);
  free(joininfo->rows);
  if (joininfo->target)
    free(joininfo->target);
  free(joininfo);
  joininfo = NULL;

  return (MS_SUCCESS);
}

#ifdef USE_MYSQL

#ifndef _mysql_h
#include <mysql/mysql.h>
#endif

char *DB_HOST = NULL;
char *DB_USER = NULL;
char *DB_PASSWD = NULL;
char *DB_DATABASE = NULL;
char *delim;

#define MYDEBUG if (0)

MYSQL_RES *msMySQLQuery(char *q, MYSQL *conn) {
  MYSQL_RES *qresult = NULL;
  if (mysql_query(conn, q) < 0) {
    mysql_close(conn);
    msSetError(MS_QUERYERR, "Bad mysql query (%s)", "msMySQLQuery()", q);
    return qresult;
  }
  if (!(qresult = mysql_store_result(conn))) {
    mysql_close(conn);
    msSetError(MS_QUERYERR, "mysql query failed (%s)", "msMySQLQuery()", q);
    return qresult;
  }
  return qresult;
}

/*  */
/* mysql join functions */
/*  */
typedef struct {
  MYSQL mysql, *conn;
  MYSQL_RES *qresult;
  MYSQL_ROW row;
  int rows;
  int fromindex;
  char *tocolumn;
  char *target;
  int nextrecord;
} msMySQLJoinInfo;
#endif

int msMySQLJoinConnect(layerObj *layer, joinObj *join) {
  (void)layer;
  (void)join;
#ifndef USE_MYSQL
  msSetError(MS_QUERYERR,
             "MySQL support not available (compile with --with-mysql)",
             "msMySQLJoinConnect()");
  return (MS_FAILURE);
#else
  int i;
  char qbuf[4000];
  char *conn_decrypted;
  msMySQLJoinInfo *joininfo;

  MYDEBUG if (setvbuf(stdout, NULL, _IONBF, 0)) { printf("Whoops..."); };
  if (join->joininfo)
    return (MS_SUCCESS); /* already open */

  /* allocate a msMySQLJoinInfo struct */
  joininfo = (msMySQLJoinInfo *)malloc(sizeof(msMySQLJoinInfo));
  if (!joininfo) {
    msSetError(MS_MEMERR, "Error allocating mysql table info structure.",
               "msMySQLJoinConnect()");
    return (MS_FAILURE);
  }

  /* initialize any members that won't get set later on in this function */
  joininfo->qresult = NULL;
  joininfo->target = NULL;
  joininfo->nextrecord = 0;

  join->joininfo = joininfo;

  /* open the mysql connection */

  if (join->connection == NULL) {
    msSetError(
        MS_QUERYERR,
        "Error parsing MYSQL JOIN: nothing specified in CONNECTION statement.",
        "msMySQLJoinConnect()");

    return (MS_FAILURE);
  }

  conn_decrypted = msDecryptStringTokens(layer->map, join->connection);
  if (conn_decrypted == NULL) {
    msSetError(
        MS_QUERYERR,
        "Error parsing MYSQL JOIN: unable to decrypt CONNECTION statement.",
        "msMySQLJoinConnect()");
    return (MS_FAILURE);
  }

  delim = msStrdup(":");
  DB_HOST = msStrdup(strtok(conn_decrypted, delim));
  DB_USER = msStrdup(strtok(NULL, delim));
  DB_PASSWD = msStrdup(strtok(NULL, delim));
  DB_DATABASE = msStrdup(strtok(NULL, delim));
  free(conn_decrypted);

  if (DB_HOST == NULL || DB_USER == NULL || DB_PASSWD == NULL ||
      DB_DATABASE == NULL) {
    msSetError(MS_QUERYERR,
               "DB param error: at least one of HOST, USER, PASSWD or DATABASE "
               "is null!",
               "msMySQLJoinConnect()");
    return MS_FAILURE;
  }
  if (strcmp(DB_PASSWD, "none") == 0)
    strcpy(DB_PASSWD, "");

#if MYSQL_VERSION_ID >= 40000
  mysql_init(&(joininfo->mysql));
  if (!(joininfo->conn = mysql_real_connect(
            &(joininfo->mysql), DB_HOST, DB_USER, DB_PASSWD, NULL, 0, NULL, 0)))
#else
  if (!(joininfo->conn =
            mysql_connect(&(joininfo->mysql), DB_HOST, DB_USER, DB_PASSWD)))
#endif
  {
    char tmp[4000];
    snprintf(tmp, sizeof(tmp),
             "Failed to connect to SQL server: Error: %s\nHost: "
             "%s\nUsername:%s\nPassword:%s\n",
             mysql_error(joininfo->conn), DB_HOST, DB_USER, DB_PASSWD);
    msSetError(MS_QUERYERR, "%s", "msMYSQLLayerOpen()", tmp);
    free(joininfo);
    return MS_FAILURE;
  }

  MYDEBUG printf("msMYSQLLayerOpen2 called<br>\n");
  if (mysql_select_db(joininfo->conn, DB_DATABASE) < 0) {
    mysql_close(joininfo->conn);
  }
  MYDEBUG printf("msMYSQLLayerOpen3 called<br>\n");
  if (joininfo->qresult != NULL) { /* query leftover */
    MYDEBUG printf("msMYSQLLayerOpen4 called<br>\n");
    mysql_free_result(joininfo->qresult);
  }
  MYDEBUG printf("msMYSQLLayerOpen5 called<br>\n");
  snprintf(qbuf, sizeof(qbuf), "SELECT count(%s) FROM %s", join->to,
           join->table);
  MYDEBUG printf("%s<br>\n", qbuf);
  if ((joininfo->qresult =
           msMySQLQuery(qbuf, joininfo->conn))) { /* There were some rows found,
                                                     write 'em out for debug */
    int numrows = mysql_affected_rows(joininfo->conn);
    MYDEBUG printf("%d rows<br>\n", numrows);
    for (i = 0; i < numrows; i++) {
      MYSQL_ROW row = mysql_fetch_row(joininfo->qresult);
      MYDEBUG printf("(%s)<BR>\n", row[0]);
      joininfo->rows = atoi(row[0]);
    }
  } else {
    msSetError(MS_DBFERR, "Item %s not found in table %s.",
               "msMySQLJoinConnect()", join->to, join->table);
    return (MS_FAILURE);
  }
  snprintf(qbuf, sizeof(qbuf), "EXPLAIN %s", join->table);
  if ((joininfo->qresult =
           msMySQLQuery(qbuf, joininfo->conn))) { /* There were some rows found,
                                                     write 'em out for debug */
    join->numitems = mysql_affected_rows(joininfo->conn);
    if ((join->items = (char **)malloc(sizeof(char *) * join->numitems)) ==
        NULL) {
      msSetError(MS_MEMERR, NULL, "msMySQLJoinConnect()");
      return (MS_FAILURE);
    }
    MYDEBUG printf("%d rows<br>\n", join->numitems);
    for (i = 0; i < join->numitems; i++) {
      MYSQL_ROW row = mysql_fetch_row(joininfo->qresult);
      MYDEBUG printf("(%s)<BR>\n", row[0]);
      join->items[i] = msStrdup(row[0]);
    }
  } else {
    msSetError(MS_DBFERR, "Item %s not found in table %s.",
               "msMySQLJoinConnect()", join->to, join->table);
    return (MS_FAILURE);
  }
  joininfo->tocolumn = msStrdup(join->to);

  /* get "from" item index   */
  for (i = 0; i < layer->numitems; i++) {
    if (strcasecmp(layer->items[i], join->from) == 0) { /* found it */
      joininfo->fromindex = i;
      break;
    }
  }

  if (i == layer->numitems) {
    msSetError(MS_JOINERR, "Item %s not found in layer %s.",
               "msMySQLJoinConnect()", join->from, layer->name);
    return (MS_FAILURE);
  }

  /* finally store away the item names in the XBase table */
  if (!join->items)
    return (MS_FAILURE);

  return (MS_SUCCESS);
#endif
}

int msMySQLJoinPrepare(joinObj *join, shapeObj *shape) {
  (void)join;
  (void)shape;
#ifndef USE_MYSQL
  msSetError(MS_QUERYERR,
             "MySQL support not available (compile with --with-mysql)",
             "msMySQLJoinPrepare()");
  return (MS_FAILURE);
#else
  msMySQLJoinInfo *joininfo = join->joininfo;

  if (!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.",
               "msMySQLJoinPrepare()");
    return (MS_FAILURE);
  }

  if (!shape) {
    msSetError(MS_JOINERR, "Shape to be joined is empty.",
               "msMySQLJoinPrepare()");
    return (MS_FAILURE);
  }

  if (!shape->values) {
    msSetError(MS_JOINERR, "Shape to be joined has no attributes.",
               "msMySQLJoinPrepare()");
    return (MS_FAILURE);
  }

  joininfo->nextrecord = 0; /* starting with the first record */

  if (joininfo->target)
    free(joininfo->target); /* clear last target */
  joininfo->target = msStrdup(shape->values[joininfo->fromindex]);

  return (MS_SUCCESS);
#endif
}

int msMySQLJoinNext(joinObj *join) {
  (void)join;
#ifndef USE_MYSQL
  msSetError(MS_QUERYERR,
             "MySQL support not available (compile with --with-mysql)",
             "msMySQLJoinNext()");
  return (MS_FAILURE);
#else
  int i;
  char qbuf[4000];
  msMySQLJoinInfo *joininfo = join->joininfo;

  if (!joininfo) {
    msSetError(MS_JOINERR, "Join connection has not be created.",
               "msMySQLJoinNext()");
    return (MS_FAILURE);
  }

  if (!joininfo->target) {
    msSetError(MS_JOINERR,
               "No target specified, run msMySQLJoinPrepare() first.",
               "msMySQLJoinNext()");
    return (MS_FAILURE);
  }

  /* clear any old data */
  if (join->values) {
    msFreeCharArray(join->values, join->numitems);
    join->values = NULL;
  }

  /* int n = joininfo->rows; */

  /* for(i=joininfo->nextrecord; i<n; i++) { // find a match */
  /* if(strcmp(joininfo->target, msMySQLReadStringAttribute(joininfo->conn, i,
   * joininfo->toindex)) == 0) break; */
  /* }   */
  snprintf(qbuf, sizeof(qbuf), "SELECT * FROM %s WHERE %s = %s", join->table,
           joininfo->tocolumn, joininfo->target);
  MYDEBUG printf("%s<BR>\n", qbuf);
  if ((joininfo->qresult =
           msMySQLQuery(qbuf, joininfo->conn))) { /* There were some rows found,
                                                     write 'em out for debug */
    int numrows = mysql_affected_rows(joininfo->conn);
    int numfields = mysql_field_count(joininfo->conn);
    MYDEBUG printf("%d rows<br>\n", numrows);
    if (numrows > 0) {
      MYSQL_ROW row = mysql_fetch_row(joininfo->qresult);
      for (i = 0; i < numfields; i++) {
        MYDEBUG printf("%s,", row[i]);
      }
      MYDEBUG printf("<BR>\n");
      free(join->values);
      if ((join->values = (char **)malloc(sizeof(char *) * join->numitems)) ==
          NULL) {
        msSetError(MS_MEMERR, NULL, "msMySQLJoinNext()");
        return (MS_FAILURE);
      }
      for (i = 0; i < join->numitems; i++) {
        /* join->values[i] = msStrdup("\0"); */ /* initialize to zero length
                                                   strings */
        join->values[i] =
            msStrdup(row[i]); /* initialize to zero length strings */
        /* rows = atoi(row[0]); */
      }
    } else {
      if ((join->values = (char **)malloc(sizeof(char *) * join->numitems)) ==
          NULL) {
        msSetError(MS_MEMERR, NULL, "msMySQLJoinNext()");
        return (MS_FAILURE);
      }
      for (i = 0; i < join->numitems; i++)
        join->values[i] =
            msStrdup("\0"); /* initialize to zero length strings */

      return (MS_DONE);
    }
  } else {
    msSetError(MS_QUERYERR, "Query error (%s)", "msMySQLJoinNext()", qbuf);
    return (MS_FAILURE);
  }

#ifdef __NOTDEF__
  if (i == n) { /* unable to do the join */
    if ((join->values = (char **)malloc(sizeof(char *) * join->numitems)) ==
        NULL) {
      msSetError(MS_MEMERR, NULL, "msMySQLJoinNext()");
      return (MS_FAILURE);
    }
    for (i = 0; i < join->numitems; i++)
      join->values[i] = msStrdup("\0"); /* initialize to zero length strings  */

    joininfo->nextrecord = n;
    return (MS_DONE);
  }

  if ((join->values = msMySQLGetValues(joininfo->conn, i)) == NULL)
    return (MS_FAILURE);

  joininfo->nextrecord =
      i + 1; /* so we know where to start looking next time through */
#endif /* __NOTDEF__ */

  return (MS_SUCCESS);
#endif
}

int msMySQLJoinClose(joinObj *join) {
  (void)join;
#ifndef USE_MYSQL
  msSetError(MS_QUERYERR,
             "MySQL support not available (compile with --with-mysql)",
             "msMySQLJoinClose()");
  return (MS_FAILURE);
#else
  msMySQLJoinInfo *joininfo = join->joininfo;

  if (!joininfo)
    return (MS_SUCCESS); /* already closed */

  mysql_close(joininfo->conn);
  if (joininfo->target)
    free(joininfo->target);
  free(joininfo);
  joininfo = NULL;

  return (MS_SUCCESS);
#endif
}

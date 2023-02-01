/******************************************************************************
 * Project: MapServer
 * Purpose: Native access to Oracle Spatial (SDO) data.
 * Author:  Fernando Simon (fsimon@univali.br)
 *          Rodrigo Becke Cabral
 *          Adriana Gomes Alves
 *
 * Notes: Developed under several funding agreements:
 *
 *   1) n.45/00 between CTTMAR/UNIVALI (www.cttmar.univali.br)
 *      and CEPSUL/IBAMA (www.ibama.gov.br)
 *
 *   2) CNPq (www.cnpq.br) under process 401263.03-7
 *
 *   3) FUNCITEC (www.funcitec.rct-sc.br) under process FCTP1523-031
 *
 ******************************************************************************
 * Copyright (c) 1996-2008 Regents of the University of Minnesota.
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
 *****************************************************************************/

#include "mapserver.h"
#include "maptime.h"
#include "mapows.h"
#include <assert.h>



#if defined(USE_ORACLESPATIAL) || defined(USE_ORACLE_PLUGIN)

#include <oci.h>
#include <ctype.h>

#define ARRAY_SIZE                 1024
#define QUERY_SIZE                 1
#define TEXT_SIZE                  4000 /* ticket #2260 */
#define TYPE_OWNER                 "MDSYS"
#define SDO_GEOMETRY               TYPE_OWNER".SDO_GEOMETRY"
#define SDO_ORDINATE_ARRAY         TYPE_OWNER".SDO_ORDINATE_ARRAY"
#define SDO_GEOMETRY_LEN           strlen( SDO_GEOMETRY )
#define FUNCTION_FILTER            1
#define FUNCTION_RELATE            2
#define FUNCTION_GEOMRELATE        3
#define FUNCTION_NONE              4
#define VERSION_8i                 1
#define VERSION_9i                 2
#define VERSION_10g                3
#define TOLERANCE                  0.001
#define NULLERRCODE                1405
#define TABLE_NAME_SIZE            2000

typedef
struct {
  OCINumber x;
  OCINumber y;
  OCINumber z;
} SDOPointObj;

typedef
struct {
  OCINumber gtype;
  OCINumber srid;
  SDOPointObj point;
  OCIArray *elem_info;
  OCIArray *ordinates;
} SDOGeometryObj;

typedef
struct {
  OCIInd _atomic;
  OCIInd x;
  OCIInd y;
  OCIInd z;
} SDOPointInd;

typedef
struct {
  OCIInd _atomic;
  OCIInd gtype;
  OCIInd srid;
  SDOPointInd point;
  OCIInd elem_info;
  OCIInd ordinates;
} SDOGeometryInd;

typedef
text item_text[TEXT_SIZE];

typedef
item_text item_text_array[ARRAY_SIZE];

typedef
item_text item_text_array_query[QUERY_SIZE];

typedef
ub2 query_dtype[ARRAY_SIZE];

typedef
struct {
  /*Oracle handlers (global to connection)*/
  OCIEnv *envhp;
  OCIError *errhp;
  OCISvcCtx *svchp;
  int last_oci_status;
  text last_oci_error[2048];
  /* This references counter is to avoid the cache freed if there are other layers that could use it */
  int ref_count;
} msOracleSpatialHandler;

typedef
struct {
  /* Oracle data handlers (global to connection) */
  OCIDescribe *dschp;
  OCIType *tdo;
} msOracleSpatialDataHandler;

typedef
struct {
  OCIStmt *stmthp;

  /* fetch data buffer */
  ub4 rows_count; /* total number of rows (so far) within cursor */
  ub4 row_num; /* current row index within cursor results */
  ub4 rows_fetched; /* total number of rows fetched into our buffer */
  ub4 row; /* current row index within our buffer */

  item_text_array *items; /* items buffer */
  item_text_array_query *items_query; /* items buffer */
  SDOGeometryObj *obj[ARRAY_SIZE]; /* spatial object buffer */
  SDOGeometryInd *ind[ARRAY_SIZE]; /* object indicator (null) buffer */

  int uniqueidindex; /*allows to keep which attribute id index is used as unique id*/


} msOracleSpatialStatement;

typedef
struct {
  /* oracle handlers */
  msOracleSpatialHandler *orahandlers;

  /* oracle data handlers */
  msOracleSpatialDataHandler *oradatahandlers;
  msOracleSpatialStatement *orastmt;

  /* Following items are setup by WhichShapes
   * used by NextShape, ResultGetShape
   * disposed by CloseLayer (if set)
   */
  msOracleSpatialStatement *orastmt2;
  /* Driver handling of pagination, enabled by default */
  int paging;

} msOracleSpatialLayerInfo;

static OCIType  *ordinates_tdo = NULL;
static OCIArray *ordinates;





/* local prototypes */
static int TRY( msOracleSpatialHandler *hand, sword status );
static int ERROR( char *routine, msOracleSpatialHandler *hand, msOracleSpatialDataHandler *dthand );
static void msSplitLogin( char *connection, mapObj *map, char **username, char **password, char **dblink );
static int msSplitData( char *data, char **geometry_column_name, char **table_name, char **unique, char **srid, char **indexfield, int *function, int * version);
static void msOCICloseConnection( void *layerinfo );
static msOracleSpatialHandler *msOCISetHandlers( char *username, char *password, char *dblink );
static int msOCISetDataHandlers( msOracleSpatialHandler *hand, msOracleSpatialDataHandler *dthand );
static void msOCICloseDataHandlers ( msOracleSpatialDataHandler *dthand );
static void msOCICloseHandlers( msOracleSpatialHandler *hand );
static void msOCIClearLayerInfo( msOracleSpatialLayerInfo *layerinfo );
static int msOCIOpenStatement( msOracleSpatialHandler *hand, msOracleSpatialStatement *sthand );
static void msOCIFinishStatement( msOracleSpatialStatement *sthand );
static int msOCIGet2DOrdinates( msOracleSpatialHandler *hand, SDOGeometryObj *obj, int s, int e, pointObj *pt );
static int msOCIGet3DOrdinates( msOracleSpatialHandler *hand, SDOGeometryObj *obj, int s, int e, pointObj *pt );
static int msOCIGet4DOrdinates( msOracleSpatialHandler *hand, SDOGeometryObj *obj, int s, int e, pointObj *pt );
static int msOCIConvertCircle( pointObj *pt );
static char * osFilteritem(layerObj *layer, int function, char *query_str, int mode);
static char * osAggrGetExtent(layerObj *layer, char *query_str, char *geom_column_name, char *table_name);
static char * osGeodeticData(int function, char *query_str, char *geom_column_name, char *index_column_name);
static char * osNoGeodeticData(int function, int version, char *query_str, char *geom_column_name, char *index_column_name);
static double osCalculateArcRadius(pointObj *pnt);
static void osCalculateArc(pointObj *pnt, int data3d, int data4d, double radius, double npoints, int side, lineObj arcline, shapeObj *shape);
static void osGenerateArc(shapeObj *shape, lineObj arcline, lineObj points, int i, int data3d, int data4d);
static void osShapeBounds ( shapeObj *shp );
static void osCloneShape(shapeObj *shape, shapeObj *newshape, int data3d, int data4d);
static void osPointCluster(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int interpretation, int data3d, int data4d);
static void osPoint(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d, int data4d);
static void osClosedPolygon(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int elem_type, int data3d, int data4d);
static void osRectangle(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d, int data4d);
static void osCircle(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d, int data4d);
static void osArcPolygon(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj arcpoints,int elem_type,int data3d, int data4d);
static int osGetOrdinates(msOracleSpatialDataHandler *dthand, msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, SDOGeometryInd *ind);
static int osCheck2DGtype(int pIntGtype);
static int osCheck3DGtype(int pIntGtype);
static int osCheck4DGtype(int pIntGtype);



/******************************************************************************
 *                          Local Helper Functions                            *
 ******************************************************************************/


/* if an error ocurred call msSetError, sets last_oci_status to MS_FAILURE and return 0;
 * otherwise returns 1 */
static int TRY( msOracleSpatialHandler *hand, sword status )
{
  sb4 errcode = 0;

  if (hand->last_oci_status == MS_FAILURE)
    return 0; /* error from previous call */

  switch (status) {
    case OCI_SUCCESS_WITH_INFO:
    case OCI_ERROR:
      OCIErrorGet((dvoid *)hand->errhp, (ub4)1, (text *)NULL, &errcode, hand->last_oci_error, (ub4)sizeof(hand->last_oci_error), OCI_HTYPE_ERROR );
      if (errcode == NULLERRCODE) {
        hand->last_oci_error[0] = (text)'\0';
        return 1;
      }
      hand->last_oci_error[sizeof(hand->last_oci_error)-1] = 0; /* terminate string!? */
      break;
    case OCI_NEED_DATA:
      strlcpy( (char *)hand->last_oci_error, "OCI_NEED_DATA", sizeof(hand->last_oci_error));
      break;
    case OCI_INVALID_HANDLE:
      strlcpy( (char *)hand->last_oci_error, "OCI_INVALID_HANDLE", sizeof(hand->last_oci_error));
      break;
    case OCI_STILL_EXECUTING:
      ((char*)hand->last_oci_error)[sizeof(hand->last_oci_error)-1] = 0;
      break;
    case OCI_CONTINUE:
      strlcpy( (char *)hand->last_oci_error, "OCI_CONTINUE", sizeof(hand->last_oci_error));
      break;
    default:
      return 1; /* no error */
  }

  /* if I got here, there was an error */
  hand->last_oci_status = MS_FAILURE;

  return 0; /* error! */
}

OCIType *get_tdo(char *typename, msOracleSpatialHandler *hand, msOracleSpatialDataHandler *dthand )
{
  OCIParam *paramp = NULL;
  OCIRef *type_ref = NULL;
  OCIType *tdoe = NULL;
  int success = 0;


  success = TRY( hand, OCIDescribeAny(hand->svchp, hand->errhp, (text *)typename,  (ub4)strlen((char *)typename), OCI_OTYPE_NAME, (ub1)1, (ub1)OCI_PTYPE_TYPE, dthand->dschp))
            &&TRY( hand, OCIAttrGet((dvoid *)dthand->dschp, (ub4)OCI_HTYPE_DESCRIBE, (dvoid *)&paramp, (ub4 *)0, (ub4)OCI_ATTR_PARAM, hand->errhp))
            &&TRY( hand, OCIAttrGet((dvoid *)paramp, (ub4)OCI_DTYPE_PARAM, (dvoid *)&type_ref, (ub4 *)0, (ub4)OCI_ATTR_REF_TDO, hand->errhp))
            &&TRY( hand, OCIObjectPin(hand->envhp, hand->errhp, type_ref, (OCIComplexObject *)0, OCI_PIN_ANY, OCI_DURATION_SESSION, OCI_LOCK_NONE, (dvoid **)&tdoe));
  if (success)
    return tdoe;

  /* if failure, return NULL*/
  return NULL;
}


/* check last_oci_status for MS_FAILURE (set by TRY()) if an error ocurred return 1;
 * otherwise, returns 0 */
static int ERROR( char *routine, msOracleSpatialHandler *hand, msOracleSpatialDataHandler *dthand )
{
  (void)dthand;
  if (hand->last_oci_status == MS_FAILURE) {
    /* there was an error */
    msSetError( MS_ORACLESPATIALERR, "OracleSpatial server returned an error, check logs for more details", routine );
    msDebug("OracleSpatial server returned an error in funtion (%s): %s.\n", routine, (char*)hand->last_oci_error );

    /* reset error flag */
    hand->last_oci_status = MS_SUCCESS;

    return 1; /* error processed */
  } else
    return 0; /* no error */
}

/* break layer->connection (username/password@dblink) into username, password and dblink */
static void msSplitLogin( char *connection, mapObj *map, char **username, char **password, char **dblink )
{
  char *src, *tgt, *conn_decrypted;
  size_t buffer_size = 0;

  /* bad 'connection' */
  if (connection == NULL) return;

  buffer_size = strlen(connection)+1;
  *username = (char*)malloc(buffer_size);
  *password = (char*)malloc(buffer_size);
  *dblink = (char*)malloc(buffer_size);

  /* clearup */
  **username = **password = **dblink = 0;

  /* Decrypt any encrypted token */
  conn_decrypted = msDecryptStringTokens(map, connection);
  if (conn_decrypted == NULL) return;

  /* ok, split connection */
  for( tgt=*username, src=conn_decrypted; *src; src++, tgt++ )
    if (*src=='/' || *src=='@')
      break;
    else
      *tgt = *src;
  *tgt = 0;
  if (*src == '/') {
    for( tgt=*password, ++src; *src; src++, tgt++ )
      if (*src == '@')
        break;
      else
        *tgt = *src;
    *tgt = 0;
  }
  if (*src == '@') {
    strlcpy( *dblink, ++src, buffer_size);
  }

  msFree(conn_decrypted);
}

/* break layer->data into geometry_column_name, table_name and srid */
static int msSplitData( char *data, char **geometry_column_name, char **table_name, char **unique, char **srid, char **indexfield, int *function, int *version )
{
  char *tok_from = "from";
  char *tok_using = "using";
  char *tok_unique = "unique";
  char *tok_srid = "srid";
  char *tok_indexfield="indexfield";
  char *tok_version = "version";
  char data_version[4] = "";
  char tok_function[11] = "";
  int parenthesis, i;
  char *src = data, *tgt;
  int table_name_size = TABLE_NAME_SIZE;
  size_t buffer_size = 0;

  /* bad 'data' */
  if (data == NULL)
    return 0;

  buffer_size = strlen(data)+1;
  *geometry_column_name = (char*)malloc(buffer_size);
  *unique = (char*)malloc(buffer_size);
  *srid = (char*)malloc(buffer_size);
  *indexfield=(char*)malloc(buffer_size);


  /* clearup */
  **geometry_column_name = **table_name = 0;

  /* parsing 'geometry_column_name' */
  for( ; *src && isspace( *src ); src++ ); /* skip blanks */
  for( tgt=*geometry_column_name; *src; src++, tgt++ )
    if (isspace( *src ))
      break;
    else
      *tgt = *src;
  *tgt = 0;

  /* parsing 'from' */
  for( ; *src && isspace( *src ); src++ ) ; /* skip blanks */
  for( ; *src && *tok_from && tolower(*src)==*tok_from; src++, tok_from++ );
  if (*tok_from != '\0')
    return 0;

  /* parsing 'table_name' or '(SELECT stmt)' */
  i = 0;
  for( ; *src && isspace( *src ); src++ ); /* skip blanks */
  for( tgt=*table_name, parenthesis=0; *src; src++, tgt++, ++i ) {
    if (*src == '(')
      parenthesis++;
    else if (*src == ')')
      parenthesis--;
    else if (parenthesis==0 && isspace( *src ))
      break; /* stop on spaces */
    /* double the size of the table_name array if necessary */
    if (i == table_name_size) {
      size_t tgt_offset = tgt - *table_name;
      table_name_size *= 2;
      *table_name = (char *) realloc(*table_name,sizeof(char *) * table_name_size);
      tgt = *table_name + tgt_offset;
    }
    *tgt = *src;
  }
  *tgt = 0;

  strlcpy( *unique, "", buffer_size);
  strlcpy( *srid, "NULL", buffer_size);
  strlcpy( *indexfield, "", buffer_size);

  *function = -1;
  *version = -1;

  /* parsing 'unique' */
  for( ; *src && isspace( *src ); src++ ) ; /* skip blanks */
  if (*src != '\0') {
    /* parse 'using' */
    for( ; *src && *tok_using && tolower(*src)==*tok_using; src++, tok_using++ );
    if (*tok_using != '\0')
      return 0;

    /* parsing 'unique' */
    for( ; *src && isspace( *src ); src++ ); /* skip blanks */
    for( ; *src && *tok_unique && tolower(*src)==*tok_unique; src++, tok_unique++ );

    if (*tok_unique == '\0') {
      for( ; *src && isspace( *src ); src++ ); /* skip blanks */
      if (*src == '\0')
        return 0;
      for( tgt=*unique; *src; src++, tgt++ )
        if (isspace( *src ))
          break;
        else
          *tgt = *src;
      *tgt = 0;

      if (*tok_unique != '\0')
        return 0;
    }

    /* parsing 'srid' */
    for( ; *src && isspace( *src ); src++ ); /* skip blanks */
    for( ; *src && *tok_srid && tolower(*src)==*tok_srid; src++, tok_srid++ );
    if (*tok_srid == '\0') {
      for( ; *src && isspace( *src ); src++ ); /* skip blanks */
      if (*src == '\0')
        return 0;
      for( tgt=*srid; *src; src++, tgt++ )
        if (isspace( *src ))
          break;
        else
          *tgt = *src;
      *tgt = 0;

      if (*tok_srid != '\0')
        return 0;
    }

    /* parsing 'indexfield' */
    for( ; *src && isspace( *src ); src++ ); /* skip blanks */
    for( ; *src && *tok_indexfield && tolower(*src)==*tok_indexfield; src++, tok_indexfield++ );

    if (*tok_indexfield == '\0') {
      for( ; *src && isspace( *src ); src++ ); /* skip blanks */
      if (*src == '\0')
        return 0;
      for( tgt=*indexfield; *src; src++, tgt++ )
        if (isspace( *src ))
          break;
        else
          *tgt = *src;
      *tgt = 0;

      if (*tok_indexfield != '\0')
        return 0;
    }

    /*parsing function/version */
    for( ; *src && isspace( *src ); src++ );
    if (*src != '\0') {
      for( tgt=tok_function; *src; src++, tgt++ )
        if (isspace( *src ))
          break;
        else
          *tgt = *src;
      *tgt = 0;
    }

    /*Upcase conversion for the FUNCTION/VERSION token*/
    for (i=0; tok_function[i] != '\0'; i++)
      tok_function[i] = toupper(tok_function[i]);

    if (strcmp(tok_function, "VERSION")) {
      if (!strcmp(tok_function, "FILTER") || !strcmp(tok_function, ""))
        *function = FUNCTION_FILTER;
      else if(!strcmp(tok_function, "RELATE"))
        *function = FUNCTION_RELATE;
      else if (!strcmp(tok_function,"GEOMRELATE"))
        *function = FUNCTION_GEOMRELATE;
      else if (!strcmp(tok_function,"NONE"))
        *function = FUNCTION_NONE;
      else {
        *function = -1;
        return 0;
      }

      /*parsing VERSION token when user defined one function*/
      for( ; *src && isspace( *src ); src++ );
      for( ; *src && *tok_version && tolower(*src)==*tok_version; src++, tok_version++ );
    } else {
      for(tgt = "VERSION"; *tgt && *tok_version && toupper(*tgt)==toupper(*tok_version); tgt++, tok_version++ );
      *function = FUNCTION_FILTER;
    }

    /*parsing version*/
    if (*tok_version == '\0') {
      for( ; *src && isspace( *src ); src++ ); /* skip blanks */
      for( tgt=data_version; *src; src++, tgt++ )
        if (isspace( *src ))
          break;
        else
          *tgt = *src;
      *tgt = 0;

      for (i=0; data_version[i] != '\0'; i++)
        data_version[i] = tolower(data_version[i]);

      if (!strcmp(data_version, "8i"))
        *version = VERSION_8i;
      else if(!strcmp(data_version, "9i"))
        *version = VERSION_9i;
      else if (!strcmp(data_version, "10g"))
        *version = VERSION_10g;
      else
        return 0;

    }

  }
  /* finish parsing */
  for( ; *src && isspace( *src ); src++ ); /* skip blanks */

  return (*src == '\0');
}


/******************************************************************************
 *                          OCI Helper Functions                              *
 ******************************************************************************/

/* create statement handle from database connection */
static int msOCIOpenStatement( msOracleSpatialHandler *hand, msOracleSpatialStatement *sthand)
{
  int success = 0;
  char * cmd  = "";

  /* allocate stmthp */
  success = TRY( hand, OCIHandleAlloc( (dvoid *)hand->envhp, (dvoid **)&sthand->stmthp, (ub4)OCI_HTYPE_STMT, (size_t)0, (dvoid **)0 ) );

  sthand->rows_count = 0;
  sthand->row_num = 0;
  sthand->rows_fetched = 0;
  sthand->row = 0;
  sthand->items = NULL;
  sthand->items_query = NULL;

  /* setting environment values to enable time parsing */

  cmd = "alter session set NLS_DATE_FORMAT='yyyy-mm-dd hh24:mi:ss'";
  success = TRY(hand, OCIStmtPrepare( sthand->stmthp, hand->errhp, (const OraText*)cmd, (ub4) strlen(cmd), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT));
  success = TRY(hand, OCIStmtExecute( hand->svchp, sthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ) );

  cmd = "alter session set NLS_TIMESTAMP_TZ_FORMAT='yyyy-mm-dd hh24:mi:ss'";
  success = TRY(hand, OCIStmtPrepare( sthand->stmthp, hand->errhp, (const OraText*)cmd, (ub4) strlen(cmd), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT));
  success = TRY(hand, OCIStmtExecute( hand->svchp, sthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ) );

  cmd = "alter session set NLS_TIMESTAMP_FORMAT = 'yyyy-mm-dd hh24:mi:ss'";
  success = TRY(hand, OCIStmtPrepare( sthand->stmthp, hand->errhp, (const OraText*)cmd, (ub4) strlen(cmd), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT));
  success = TRY(hand, OCIStmtExecute( hand->svchp, sthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ) );

  cmd = "alter session set time_zone = 'GMT'";
  success = TRY(hand, OCIStmtPrepare( sthand->stmthp, hand->errhp, (const OraText*)cmd, (ub4) strlen(cmd), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT));
  success = TRY(hand, OCIStmtExecute( hand->svchp, sthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ) );


  /* fprintf(stderr, "Creating statement handle at %p\n", sthand->stmthp); */

  return success;
}

/* create statement handle from database connection */
static void msOCIFinishStatement( msOracleSpatialStatement *sthand )
{
  if(sthand != NULL) {
    /* fprintf(stderr, "Freeing statement handle at %p\n", sthand->stmthp); */

    if (sthand->stmthp != NULL)
      OCIHandleFree( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT );
    if (sthand->items != NULL)
      free( sthand->items );
    if (sthand->items_query != NULL)
      free( sthand->items_query );
    memset(sthand, 0, sizeof( msOracleSpatialStatement ) );
    free(sthand);
  }
}

static int msOCISetDataHandlers(msOracleSpatialHandler *hand, msOracleSpatialDataHandler *dthand)
{
  int success = 0;
  OCIParam *paramp = NULL;
  OCIRef *type_ref = NULL;

  success = TRY( hand,
                 /* allocate dschp */
                 OCIHandleAlloc( hand->envhp, (dvoid **)&dthand->dschp, (ub4)OCI_HTYPE_DESCRIBE, (size_t)0, (dvoid **)0 ) )
            && TRY( hand,
                    /* describe SDO_GEOMETRY in svchp (dschp) */
                    OCIDescribeAny( hand->svchp, hand->errhp, (text *)SDO_GEOMETRY, (ub4)SDO_GEOMETRY_LEN, OCI_OTYPE_NAME, (ub1)1, (ub1)OCI_PTYPE_TYPE, dthand->dschp ) )
            && TRY( hand,
                    /* get param for SDO_GEOMETRY */
                    OCIAttrGet( (dvoid *)dthand->dschp, (ub4)OCI_HTYPE_DESCRIBE, (dvoid *)&paramp, (ub4 *)0,  (ub4)OCI_ATTR_PARAM, hand->errhp ) )
            && TRY( hand,
                    /* get type_ref for SDO_GEOMETRY */
                    OCIAttrGet( (dvoid *)paramp, (ub4)OCI_DTYPE_PARAM, (dvoid *)&type_ref, (ub4 *)0, (ub4)OCI_ATTR_REF_TDO, hand->errhp ) )
            && TRY( hand,
                    /* get TDO for SDO_GEOMETRY */
                    OCIObjectPin( hand->envhp, hand->errhp, type_ref, (OCIComplexObject *)0, OCI_PIN_ANY, OCI_DURATION_SESSION, OCI_LOCK_NONE, (dvoid **)&dthand->tdo ) );

  return success;
}

/* connect to database */
static msOracleSpatialHandler *msOCISetHandlers( char *username, char *password, char *dblink )
{
  int success;

  msOracleSpatialHandler *hand = NULL;

  hand = (msOracleSpatialHandler *) malloc( sizeof(msOracleSpatialHandler));
  if (hand == NULL) {
    msSetError(MS_MEMERR, NULL, "msOCISetHandlers()");
    return NULL;
  }
  memset( hand, 0, sizeof(msOracleSpatialHandler) );

  hand->ref_count = 1;
  hand->last_oci_status = MS_SUCCESS;
  hand->last_oci_error[0] = (text)'\0';

  success = TRY( hand,
                 /* allocate envhp */
#ifdef USE_THREAD
                 OCIEnvCreate( &hand->envhp, OCI_OBJECT|OCI_THREADED, (dvoid *)0, 0, 0, 0, (size_t) 0, (dvoid **)0 ) )
#else
                 OCIEnvCreate( &hand->envhp, OCI_OBJECT, (dvoid *)0, 0, 0, 0, (size_t) 0, (dvoid **)0 ) )
#endif
            && TRY( hand,
                    /* allocate errhp */
                    OCIHandleAlloc( (dvoid *)hand->envhp, (dvoid **)&hand->errhp, (ub4)OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0 ) )
            && TRY( hand,
                    /* logon */
                    OCILogon( hand->envhp, hand->errhp, &hand->svchp, (text *)username, strlen(username), (text *)password, strlen(password), (text *)dblink, strlen(dblink) ) );

  if ( !success ) {
    msDebug(   "Cannot create OCI Handlers. "
                "Connection failure."
                "Error: %s."
                "msOracleSpatialLayerOpen()\n", hand->last_oci_error);
    msSetError( MS_ORACLESPATIALERR,
                "Cannot create OCI Handlers. "
                "Connection failure. Check your logs and the connection string. ",
                "msOracleSpatialLayerOpen()");

    msOCICloseHandlers(hand);
    return NULL;
  }

  return hand;

}

/* disconnect from database */
static void msOCICloseHandlers( msOracleSpatialHandler *hand )
{
  if (hand->svchp != NULL)
    OCILogoff( hand->svchp, hand->errhp );
  if (hand->errhp != NULL)
    OCIHandleFree( (dvoid *)hand->errhp, (ub4)OCI_HTYPE_ERROR );
  if (hand->envhp != NULL)
    OCIHandleFree( (dvoid *)hand->envhp, (ub4)OCI_HTYPE_ENV );
  if (hand != NULL)
    memset( hand, 0, sizeof (msOracleSpatialHandler));
  free(hand);
}

static void msOCICloseDataHandlers( msOracleSpatialDataHandler *dthand )
{
  if (dthand->dschp != NULL)
    OCIHandleFree( (dvoid *)dthand->dschp, (ub4)OCI_HTYPE_DESCRIBE );
  if (dthand != NULL)
    memset( dthand, 0, sizeof (msOracleSpatialDataHandler));
  free(dthand);
}

static void msOCIClearLayerInfo( msOracleSpatialLayerInfo *layerinfo )
{
  if (layerinfo != NULL) {
    memset( layerinfo, 0, sizeof( msOracleSpatialLayerInfo ) );
    free(layerinfo);
  }
}

/*function that creates the correct sql for geoditical srid for version 9i*/
static char * osGeodeticData(int function, char *query_str, char *geom_column_name, char *index_column_name)
{
  char *filter_field=index_column_name[0]=='\0' ? geom_column_name : index_column_name;
  switch (function) {
    case FUNCTION_FILTER: {
      query_str = msStringConcatenate(query_str, "SDO_FILTER( ");
      query_str = msStringConcatenate(query_str, filter_field);
      query_str = msStringConcatenate(query_str, ", SDO_CS.VIEWPORT_TRANSFORM(MDSYS.SDO_GEOMETRY("
                "2003, 0, NULL,"
                "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                ":ordinates ), :srid),"
                "'querytype=window') = 'TRUE'");
      break;
    }
    case FUNCTION_RELATE: {
      query_str = msStringConcatenate(query_str, "SDO_RELATE( ");
      query_str = msStringConcatenate(query_str, filter_field);
      query_str = msStringConcatenate(query_str, ", SDO_CS.VIEWPORT_TRANSFORM(MDSYS.SDO_GEOMETRY("
                "2003, 0, NULL,"
                "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                ":ordinates ), :srid),"
                "'mask=anyinteract querytype=window') = 'TRUE'");
      break;
    }
    case FUNCTION_GEOMRELATE: {
      char tmpFloat[256];
      snprintf(tmpFloat, sizeof(tmpFloat), "%f", TOLERANCE);

      query_str = msStringConcatenate(query_str, "SDO_GEOM.RELATE( ");
      query_str = msStringConcatenate(query_str, filter_field);
      query_str = msStringConcatenate(query_str, ", SDO_CS.VIEWPORT_TRANSFORM(MDSYS.SDO_GEOMETRY("
                "2003, 0, NULL,"
                "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                ":ordinates), :srid),");
      query_str = msStringConcatenate(query_str, tmpFloat);
      query_str = msStringConcatenate(query_str,  ") = 'TRUE' AND ");
      query_str = msStringConcatenate(query_str, geom_column_name);
      query_str = msStringConcatenate(query_str,  " IS NOT NULL");
      break;
    }
    case FUNCTION_NONE: {
      break;
    }
    default: {
      query_str = msStringConcatenate(query_str, "SDO_FILTER( ");
      query_str = msStringConcatenate(query_str, filter_field);
      query_str = msStringConcatenate(query_str, ", SDO_CS.VIEWPORT_TRANSFORM(MDSYS.SDO_GEOMETRY("
                "2003, 0, NULL,"
                "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                ":ordinates), :srid),"
                "'querytype=window') = 'TRUE'");
    }
  }
  return query_str;
}

/*function that generate the correct sql for no geoditic srid's*/
static char * osNoGeodeticData(int function, int version, char *query_str, char *geom_column_name, char *index_column_name)
{
   char *filter_field= index_column_name[0]=='\0' ? geom_column_name : index_column_name;
   switch (function) {
    case FUNCTION_FILTER: {
      query_str = msStringConcatenate(query_str, "SDO_FILTER( ");
      query_str = msStringConcatenate(query_str, filter_field);
      query_str = msStringConcatenate(query_str, ", MDSYS.SDO_GEOMETRY("
                "2003, :srid, NULL,"
                "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                /*   "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g)" */
                ":ordinates"
                " ),'querytype=window') = 'TRUE'");
      break;
    }
    case FUNCTION_RELATE: {
      if (version == VERSION_10g) {
        query_str = msStringConcatenate(query_str, "SDO_ANYINTERACT( ");
        query_str = msStringConcatenate(query_str, filter_field);
        query_str = msStringConcatenate(query_str, ", MDSYS.SDO_GEOMETRY("
                  "2003, :srid, NULL,"
                  "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                  ":ordinates)) = 'TRUE'");
      } else {

        query_str = msStringConcatenate(query_str, "SDO_RELATE( ");
        query_str = msStringConcatenate(query_str, geom_column_name);
        query_str = msStringConcatenate(query_str, ", MDSYS.SDO_GEOMETRY("
                  "2003, :srid, NULL,"
                  "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                  ":ordinates),"
                  "'mask=anyinteract querytype=window') = 'TRUE'");
      }
      break;
    }
    case FUNCTION_GEOMRELATE: {
      char tmpFloat[256];
      snprintf(tmpFloat, sizeof(tmpFloat), "%f", TOLERANCE);

      query_str = msStringConcatenate(query_str, "SDO_GEOM.RELATE( ");
      query_str = msStringConcatenate(query_str, index_column_name);
      query_str = msStringConcatenate(query_str, ", 'anyinteract', MDSYS.SDO_GEOMETRY("
                "2003, :srid, NULL,"
                "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                ":ordinates),");
      query_str = msStringConcatenate(query_str, tmpFloat);
      query_str = msStringConcatenate(query_str, ") = 'TRUE' AND ");
      query_str = msStringConcatenate(query_str, geom_column_name);
      query_str = msStringConcatenate(query_str, " IS NOT NULL");
      break;
    }
    case FUNCTION_NONE: {
      break;
    }
    default: {
      query_str = msStringConcatenate(query_str, "SDO_FILTER( ");
      query_str = msStringConcatenate(query_str, filter_field);
      query_str = msStringConcatenate(query_str, ", MDSYS.SDO_GEOMETRY("
                "2003, :srid, NULL,"
                "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                ":ordinates),"
                "'querytype=window') = 'TRUE'");
    }
  }
  return query_str;
}

/* get ordinates from SDO buffer */
static int msOCIGet2DOrdinates( msOracleSpatialHandler *hand, SDOGeometryObj *obj, int s, int e, pointObj *pt )
{
  double x, y;
  int i, n, success = 1;
  boolean exists;
  OCINumber *oci_number;

  for( i=s, n=0; i < e && success; i+=2, n++ ) {
    success = TRY( hand,
                   OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
              && TRY( hand,
                      OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&x ) )
              && TRY( hand,
                      OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i+1, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
              && TRY( hand,
                      OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&y ) );

    if (success) {
      pt[n].x = x;
      pt[n].y = y;
    }
  }

  return success ? n : 0;
}

static int msOCIGet3DOrdinates( msOracleSpatialHandler *hand, SDOGeometryObj *obj, int s, int e, pointObj *pt )
{
  double x, y;
  int i, n, success = 1;
  boolean exists;
  double z;
  boolean numnull;
  OCINumber *oci_number;

  for( i=s, n=0; i < e && success; i+=3, n++ ) {
    success = TRY( hand,
                   OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
              && TRY( hand,
                      OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&x ) )
              && TRY( hand,
                      OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i+1, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
              && TRY( hand,
                      OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&y ) )
              && TRY( hand,
                      OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i+2, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
              ;
    if (success) {
      success = TRY(hand, OCINumberIsZero( hand->errhp, oci_number, (boolean *)&numnull));
      if (success) {
        success = TRY( hand, OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&z ) );
      } else {
        hand->last_oci_status = MS_SUCCESS;
        strlcpy( (char *)hand->last_oci_error, "Retrieve z value, but NULL value for z. Setting z to 0.", sizeof(hand->last_oci_error));
        z = 0;
        success = 1;
      }
    }
    if (success) {
      pt[n].x = x;
      pt[n].y = y;
      pt[n].z = z;
    }
  }

  return success ? n : 0;
}

static int msOCIGet4DOrdinates( msOracleSpatialHandler *hand, SDOGeometryObj *obj, int s, int e, pointObj *pt )
{
  double x, y;
  int i, n, success = 1;
  boolean exists;
  double z;
  boolean numnull;
  OCINumber *oci_number;

  for( i=s, n=0; i < e && success; i+=4, n++ ) {
    success = TRY( hand,
                   OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
              && TRY( hand,
                      OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&x ) )
              && TRY( hand,
                      OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i+1, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
              && TRY( hand,
                      OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&y ) )
              && TRY( hand,
                      OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i+2, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
              ;

    if (success) {
      success = TRY(hand, OCINumberIsZero( hand->errhp, oci_number, (boolean *)&numnull));
      if (success) {
        success = TRY( hand, OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&z ) );
      } else {
        hand->last_oci_status = MS_SUCCESS;
        strlcpy( (char *)hand->last_oci_error, "Retrieve z value, but NULL value for z. Setting z to 0.", sizeof(hand->last_oci_error));
        z = 0;
        success = 1;
      }
    }

    if (success) {
      pt[n].x = x;
      pt[n].y = y;
      pt[n].z = z;
    }
  }
  return success ? n : 0;
}

/* convert three-point circle to two-point rectangular bounds */
static int msOCIConvertCircle( pointObj *pt )
{
  pointObj ptaux;
  double dXa, dXb;
  double ma, mb;
  double cx, cy, r;
  int success;

  dXa = pt[1].x - pt[0].x;
  success = (fabs( dXa ) > 1e-8);
  if (!success) {
    /* switch points 1 & 2 */
    ptaux = pt[1];
    pt[1] = pt[2];
    pt[2] = ptaux;
    dXa = pt[1].x - pt[0].x;
    success = (fabs( dXa ) > 1e-8);
  }
  if (success) {
    dXb = pt[2].x - pt[1].x;
    success = (fabs( dXb ) > 1e-8);
    if (!success) {
      /* insert point 2 before point 0 */
      ptaux = pt[2];
      pt[2] = pt[1];
      pt[1] = pt[0];
      pt[0] = ptaux;
      dXb = dXa; /* segment A has become B */
      dXa = pt[1].x - pt[0].x; /* recalculate new segment A */
      success = (fabs( dXa ) > 1e-8);
    }
  }
  if (success) {
    ma = (pt[1].y - pt[0].y)/dXa;
    mb = (pt[2].y - pt[1].y)/dXb;
    success = (fabs( mb - ma ) > 1e-8);
  }
  if (!success)
    return 0;

  /* calculate center and radius */
  cx = (ma*mb*(pt[0].y - pt[2].y) + mb*(pt[0].x + pt[1].x) - ma*(pt[1].x + pt[2].x))/(2*(mb - ma));
  cy = (fabs( ma ) > 1e-8)
       ? ((pt[0].y + pt[1].y)/2 - (cx - (pt[0].x + pt[1].x)/2)/ma)
       : ((pt[1].y + pt[2].y)/2 - (cx - (pt[1].x + pt[2].x)/2)/mb);

  r  = sqrt( pow( pt[0].x - cx, 2 ) + pow( pt[0].y - cy, 2 ) );

  /* update pt buffer with rectangular bounds */
  pt[0].x = cx - r;
  pt[0].y = cy - r;
  pt[1].x = cx + r;
  pt[1].y = cy + r;

  return 1;
}

/*function that creates the correct sql for filter and filteritem*/
static char * osFilteritem(layerObj *layer, int function, char *query_str, int mode)
{
  if (layer->filter.native_string != NULL) {
    if (mode == 1) {
      query_str = msStringConcatenate(query_str, " WHERE ");
    } else {
      query_str = msStringConcatenate(query_str, " AND ");
    }
    size_t tmpBufSize = (strlen(layer->filter.native_string) + 3) * sizeof(char);
    char * tmpBuf = (char *) msSmallMalloc(tmpBufSize);
    snprintf(tmpBuf, tmpBufSize, " %s ", layer->filter.native_string);

    query_str = msStringConcatenate(query_str, tmpBuf);
    msFree(tmpBuf);

    if (function != FUNCTION_NONE) {
      query_str = msStringConcatenate(query_str, " AND ");
    }
  } else {
    if (function != FUNCTION_NONE) {
      query_str = msStringConcatenate(query_str, " WHERE ");
    }
  }

  /* Handle a native filter set as a PROCESSING option (#5001). */
  if ( msLayerGetProcessingKey(layer, "NATIVE_FILTER") != NULL ) {
     if ((function == FUNCTION_NONE)&&(layer->filter.native_string == NULL)) {
        query_str = msStringConcatenate(query_str, " WHERE ");
     }
     if ((function == FUNCTION_NONE)&&(layer->filter.native_string != NULL)) {
        query_str = msStringConcatenate(query_str, " AND ");
     }
     size_t tmpSz;

     const char * native_filter = msLayerGetProcessingKey(layer, "NATIVE_FILTER");
     char * tmpBuf = NULL;

     tmpSz = sizeof(char) * (strlen(native_filter) + 2 + 1);
     tmpBuf = msSmallMalloc(tmpSz);

     snprintf(tmpBuf, tmpSz, " %s ", native_filter);
     query_str = msStringConcatenate(query_str, tmpBuf);
     msFree(tmpBuf);

     if (function != FUNCTION_NONE) {
      query_str = msStringConcatenate(query_str, " AND ");
     }
  }
  return query_str;
}

static char * osAggrGetExtent(layerObj *layer, char *query_str, char *geom_column_name, char *table_name)
{
  char * query_str2 = NULL;
  int i = 0;

  query_str2 = msStringConcatenate(query_str2, "(SELECT");

  for (i = 0; i < layer->numitems; ++i) {
      size_t tmpSize = sizeof(char) * (strlen(layer->items[i]) + 1 /* '\0' */ + 2 /* " ", "," */);
      char * tmpItem = (char *) msSmallMalloc(tmpSize);
      snprintf(tmpItem, tmpSize, " %s,", layer->items[i]);

      query_str2 = msStringConcatenate(query_str2, tmpItem);

      msFree(tmpItem);
  }

  size_t tmpFromSize = sizeof(char) * (7 + strlen(geom_column_name) + strlen(table_name) + 1);
  char * tmpFrom = msSmallMalloc(tmpFromSize);
  snprintf(tmpFrom, tmpFromSize, "%s FROM %s", geom_column_name, table_name);

  query_str2 = msStringConcatenate(query_str2, tmpFrom);

  msFree(tmpFrom);

  query_str2 = osFilteritem(layer, FUNCTION_NONE, query_str2, 1);

  query_str = msStringConcatenate(query_str, "SELECT SDO_AGGR_MBR(");
  query_str = msStringConcatenate(query_str, geom_column_name);
  query_str = msStringConcatenate(query_str, ") AS GEOM from ");
  query_str = msStringConcatenate(query_str, query_str2);
  query_str = msStringConcatenate(query_str, ")");

  msFree(query_str2);

  if (layer->debug)
    msDebug("osAggrGetExtent was called: %s.\n", query_str);

  return query_str;
}

static double osCalculateArcRadius(pointObj *pnt)
{
  double rc;
  double r1, r2, r3;

  r1 = sqrt( pow(pnt[0].x-pnt[1].x,2) + pow(pnt[0].y-pnt[1].y,2) );
  r2 = sqrt( pow(pnt[1].x-pnt[2].x,2) + pow(pnt[1].y-pnt[2].y,2) );
  r3 = sqrt( pow(pnt[2].x-pnt[0].x,2) + pow(pnt[2].y-pnt[0].y,2) );
  rc = ( r1+r2+r3 )/2;

  return ( ( r1*r2*r3 )/( 4*sqrt( rc * (rc-r1) * (rc-r2) * (rc-r3) ) ) );
}

static void osCalculateArc(pointObj *pnt, int data3d, int data4d, double radius, double npoints, int side, lineObj arcline, shapeObj *shape)
{
  double length, ctrl, angle;
  double divbas, plusbas, cosbas, sinbas = 0;
  double zrange = 0;
  int i = 0;

  if ( npoints > 0 ) {
    length = sqrt(((pnt[1].x-pnt[0].x)*(pnt[1].x-pnt[0].x))+((pnt[1].y-pnt[0].y)*(pnt[1].y-pnt[0].y)));
    ctrl = length/(2*radius);

    if (data3d||data4d) {
      /* is this cast (long) legit... ? */
      zrange = labs((long)(pnt[0].z-pnt[1].z))/npoints;
      if ((pnt[0].z > pnt[1].z) && side == 1)
        zrange *= -1;
      else if ((pnt[0].z < pnt[1].z) && side == 1)
        zrange = zrange;
      else if ((pnt[0].z > pnt[1].z) && side != 1)
        zrange *= -1;
      else if ((pnt[0].z < pnt[1].z) && side != 1)
        zrange = zrange;
    }

    if( ctrl <= 1 ) {
      divbas = 2 * asin(ctrl);
      plusbas = divbas/(npoints);
      cosbas = (pnt[0].x-pnt[3].x)/radius;
      sinbas = (pnt[0].y-pnt[3].y)/radius;
      angle = plusbas;

      arcline.point = (pointObj *)malloc(sizeof(pointObj)*(npoints+1));
      arcline.point[0].x = pnt[0].x;
      arcline.point[0].y = pnt[0].y;
      if (data3d||data4d)
        arcline.point[0].z = pnt[0].z;

      for (i = 1; i <= npoints; i++) {
        if( side == 1) {
          arcline.point[i].x = pnt[3].x + radius * ((cosbas*cos(angle))-(sinbas*sin(angle)));
          arcline.point[i].y = pnt[3].y + radius * ((sinbas*cos(angle))+(cosbas*sin(angle)));
          if (data3d||data4d)
            arcline.point[i].z = pnt[0].z + (zrange*i);
        } else {
          if ( side == -1) {
            arcline.point[i].x = pnt[3].x + radius * ((cosbas*cos(angle))+(sinbas*sin(angle)));
            arcline.point[i].y = pnt[3].y + radius * ((sinbas*cos(angle))-(cosbas*sin(angle)));
            if (data3d||data4d)
              arcline.point[i].z = pnt[0].z + (zrange*i);
          } else {
            arcline.point[i].x = pnt[0].x;
            arcline.point[i].y = pnt[0].y;
            if (data3d||data4d)
              arcline.point[i].z = pnt[0].z;
          }
        }
        angle += plusbas;
      }

      arcline.numpoints = npoints+1;
      msAddLine( shape, &arcline );
      free (arcline.point);
    }
  } else {
    arcline.point = (pointObj *)malloc(sizeof(pointObj)*(2));
    arcline.point[0].x = pnt[0].x;
    arcline.point[0].y = pnt[0].y;
    arcline.point[1].x = pnt[1].x;
    arcline.point[1].y = pnt[1].y;

    if(data3d||data4d) {
      arcline.point[0].z = pnt[0].z;
      arcline.point[1].z = pnt[1].z;
    }

    arcline.numpoints = 2;

    msAddLine( shape, &arcline );
    free (arcline.point);
  }
}

/* Part of this function was based on Terralib function TeGenerateArc
 * found in TeGeometryAlgorith.cpp (www.terralib.org).
 * Part of this function was based on Dr. Ialo (Univali/Cttmar) functions. */
static void osGenerateArc(shapeObj *shape, lineObj arcline, lineObj points, int i, int data3d, int data4d)
{
  double mult, plus1, plus2, plus3, bpoint;
  double cx, cy;
  double radius, side, area, npoints;
  double dist1 = 0;
  double dist2 = 0;
  pointObj point5[4];

  mult = ( points.point[i+1].x - points.point[i].x )
         * ( points.point[i+2].y - points.point[i].y )
         - ( points.point[i+1].y - points.point[i].y )
         * ( points.point[i+2].x - points.point[i].x );

  if (mult) {
    /*point5 = (pointObj *)malloc(sizeof(pointObj)*(4));*/

    plus1 = points.point[i].x * points.point[i].x + points.point[i].y * points.point[i].y;
    plus2 = points.point[i+1].x * points.point[i+1].x + points.point[i+1].y * points.point[i+1].y;
    plus3 = points.point[i+2].x * points.point[i+2].x + points.point[i+2].y * points.point[i+2].y;

    bpoint = plus1 * ( points.point[i+1].y - points.point[i+2].y )
             + plus2 * ( points.point[i+2].y - points.point[i].y )
             + plus3 * ( points.point[i].y - points.point[i+1].y );

    cx = bpoint / (2.0 * mult);

    bpoint = plus1 * ( points.point[i+2].x - points.point[i+1].x )
             + plus2 * ( points.point[i].x - points.point[i+2].x )
             + plus3 * ( points.point[i+1].x - points.point[i].x );

    cy = bpoint / (2.0 * mult);

    dist1 = (points.point[i+1].x - points.point[i].x) * (cy - points.point[i].y);
    dist2 = (points.point[i+1].y - points.point[i].y) * (cx - points.point[i].x);
    side = 0;

    if((dist1-dist2) > 0)
      side = 1;
    else {
      if((dist1-dist2) < 0)
        side = -1;
    }

    point5[0] = points.point[i];
    point5[1] = points.point[i+1];
    point5[2] = points.point[i+2];
    point5[3].x = cx;
    point5[3].y = cy;

    radius = osCalculateArcRadius(point5);

    area = ((points.point[i].x + points.point[i+1].x) * (points.point[i+1].y - points.point[i].y))
           + ((points.point[i+1].x + points.point[i+2].x) * (points.point[i+2].y - points.point[i+1].y));

    npoints = labs((long)(area/radius));

    point5[0] = points.point[i];
    point5[1] = points.point[i+1];
    osCalculateArc(point5, data3d, data4d, radius, (npoints>1000)?1000:npoints, side, arcline, shape);

    point5[0] = points.point[i+1];
    point5[1] = points.point[i+2];
    osCalculateArc(point5, data3d, data4d, radius, (npoints>1000)?1000:npoints, side, arcline, shape);

  } else {
    arcline.point = (pointObj *)malloc(sizeof(pointObj)*(2));
    arcline.point[0].x = points.point[i].x;
    arcline.point[0].y = points.point[i].y;
    arcline.point[1].x = points.point[i+2].x;
    arcline.point[1].y = points.point[i+2].y;

    if (data3d||data4d) {
      arcline.point[0].z = points.point[i].z;
      arcline.point[1].z = points.point[i+2].z;
    }

    arcline.numpoints = 2;

    msAddLine( shape, &arcline );
    free (arcline.point);
  }
}

static void osShapeBounds ( shapeObj *shp )
{
  int i, f;

  if ( (shp->numlines > 0) && (shp->line[0].numpoints > 0) ) {
    shp->bounds.minx = shp->line[0].point[0].x;
    shp->bounds.maxx = shp->line[0].point[0].x;
    shp->bounds.miny = shp->line[0].point[0].y;
    shp->bounds.maxy = shp->line[0].point[0].y;
  }

  for ( i = 0 ; i < shp->numlines; i++) {
    for( f = 0; f < shp->line[i].numpoints; f++) {
      if (shp->line[i].point[f].x < shp->bounds.minx)
        shp->bounds.minx = shp->line[i].point[f].x;
      if (shp->line[i].point[f].x > shp->bounds.maxx)
        shp->bounds.maxx = shp->line[i].point[f].x;
      if (shp->line[i].point[f].y < shp->bounds.miny)
        shp->bounds.miny = shp->line[i].point[f].y;
      if (shp->line[i].point[f].y > shp->bounds.maxy)
        shp->bounds.maxy = shp->line[i].point[f].y;
    }
  }
}

static void osCloneShape(shapeObj *shape, shapeObj *newshape, int data3d, int data4d)
{
  int max_points = 0;
  int i,f,g;
  lineObj shapeline = {0, NULL};

  for (i = 0; i < shape->numlines; i++)
    max_points += shape->line[i].numpoints;

  if (max_points > 0)
    shapeline.point = (pointObj *)malloc( sizeof(pointObj)*max_points );

  g = 0;
  for ( i = 0 ; i < shape->numlines; i++) {
    for( f = 0; f < shape->line[i].numpoints && g <= max_points; f++, g++) {
      shapeline.point[g].x = shape->line[i].point[f].x;
      shapeline.point[g].y = shape->line[i].point[f].y;
      if (data3d||data4d)
        shapeline.point[g].z = shape->line[i].point[f].z;
    }
  }

  if (g) {
    shapeline.numpoints = g;
    newshape->type = shape->type; /* jimk: 2009/09/23 Fixes compound linestrings being converted to polygons */
    msAddLine( newshape, &shapeline );
  }
}

static void osPointCluster(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int interpretation, int data3d, int data4d)
{
  int n;

  n = (end - start)/2;
  /* n = interpretation; */

  /* if (n == interpretation) { */
  points.point = (pointObj *)malloc( sizeof(pointObj)*n );

  if (data3d)
    n = msOCIGet3DOrdinates( hand, obj, start, end, points.point );
  else if (data4d)
    n = msOCIGet4DOrdinates( hand, obj, start, end, points.point );
  else
    n = msOCIGet2DOrdinates( hand, obj, start, end, points.point );

  if (n == interpretation && n>0) {
    shape->type = MS_SHAPE_POINT;
    points.numpoints = n;
    msAddLine( shape, &points );
  }
  free( points.point );
  /* } */
}

static void osPoint(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d, int data4d)
{
  int n;

  if (data3d)
    n = msOCIGet3DOrdinates( hand, obj, start, end, pnt );
  else if (data4d)
    n = msOCIGet4DOrdinates( hand, obj, start, end,  pnt );
  else
    n = msOCIGet2DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */

  if (n == 1) {
    shape->type = MS_SHAPE_POINT;
    points.numpoints = 1;
    points.point = pnt;
    msAddLine( shape, &points );
  }
}

static void osClosedPolygon(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int elem_type, int data3d, int data4d)
{
  int n;

  n = (end - start)/2;
  points.point = (pointObj *)malloc( sizeof(pointObj)*n );

  if (data3d)
    n = msOCIGet3DOrdinates( hand, obj, start, end, points.point );
  else if (data4d)
    n = msOCIGet4DOrdinates( hand, obj, start, end, points.point );
  else
    n = msOCIGet2DOrdinates( hand, obj, start, end, points.point );

  if (n > 0) {
    shape->type = (elem_type==21) ? MS_SHAPE_LINE : MS_SHAPE_POLYGON;
    points.numpoints = n;
    msAddLine( shape, &points );
  }
  free( points.point );
}

static void osRectangle(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d, int data4d)
{
  int n;

  if (data3d)
    n = msOCIGet3DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */
  else if (data4d)
    n = msOCIGet4DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */
  else
    n = msOCIGet2DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */

  if (n == 2) {
    shape->type = MS_SHAPE_POLYGON;
    points.numpoints = 5;
    points.point = pnt;

    /* point5 [0] & [1] contains the lower-left and upper-right points of the rectangle */
    pnt[2] = pnt[1];
    pnt[1].x = pnt[0].x;
    pnt[3].x = pnt[2].x;
    pnt[3].y = pnt[0].y;
    pnt[4] = pnt[0];
    if (data3d||data4d) {
      pnt[1].z = pnt[0].z;
      pnt[3].z = pnt[2].z;
    }

    msAddLine( shape, &points );
  }
}

static void osCircle(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d, int data4d)
{
  int n;

  if (data3d)
    n = msOCIGet3DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */
  else if (data4d)
    n = msOCIGet4DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */
  else
    n = msOCIGet2DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */

  if (n == 3) {
    if (msOCIConvertCircle( pnt )) {
      shape->type = MS_SHAPE_POINT;
      points.numpoints = 2;
      points.point = pnt;
      msAddLine( shape, &points );
    } else {
      strlcpy( (char *)hand->last_oci_error, "Points in circle object are colinear", sizeof(hand->last_oci_error));
      hand->last_oci_status = MS_FAILURE;
    }
  }
}

static void osArcPolygon(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj arcpoints, int elem_type, int data3d, int data4d)
{
  int n, i;
  lineObj points = {0, NULL};

  n = (end - start)/2;
  points.point = (pointObj *)malloc( sizeof(pointObj)*n );

  if (data3d)
    n = msOCIGet3DOrdinates( hand, obj, start, end, points.point );
  else if (data4d)
    n = msOCIGet4DOrdinates( hand, obj, start, end, points.point );
  else
    n = msOCIGet2DOrdinates( hand, obj, start, end, points.point );

  if (n > 2) {
    shape->type = (elem_type==32) ? MS_SHAPE_POLYGON : MS_SHAPE_LINE;
    points.numpoints = n;

    for (i = 0; i < n-2; i = i+2)
      osGenerateArc(shape, arcpoints, points, i, data3d, data4d);
  }
  free (points.point);
}

static int osCheck2DGtype(int pIntGtype)
{
  if (pIntGtype > 2000 && pIntGtype < 2008) {
    if (pIntGtype != 2004)
      return MS_TRUE;
  }

  return MS_FALSE;
}

static int osCheck3DGtype(int pIntGtype)
{
  if (pIntGtype > 3000 && pIntGtype < 3308) {
    if (pIntGtype > 3007)
      pIntGtype-= 300;

    if (pIntGtype <= 3007 && pIntGtype != 3004)
      return MS_TRUE;
  }
  /*
  * Future version, untested
  * return (pIntGtype & 2208 && (pIntGtype & 3000 || pIntGtype & 3296) && pIntGtype & 3);
  */
  return MS_FALSE;
}

static int osCheck4DGtype(int pIntGtype)
{

  if (pIntGtype > 4000 && pIntGtype < 4408) {
    if (pIntGtype > 4007)
      pIntGtype-= 400;

    if (pIntGtype <= 4007 && pIntGtype != 4004)
      return MS_TRUE;


  }

  return MS_FALSE;
}



static int osGetOrdinates(msOracleSpatialDataHandler *dthand, msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, SDOGeometryInd *ind)
{
  int gtype, elem_type, compound_type;
  float compound_lenght, compound_count;
  ub4 etype;
  ub4 interpretation;
  int nelems, nords, data3d, data4d;
  int elem, ord_start, ord_end;
  boolean exists;
  OCINumber *oci_number;
  double x, y;
  double z;
  int success;
  lineObj points = {0, NULL};
  pointObj point5[5]; /* for quick access */
  shapeObj newshape; /* for compound polygons */

  /*stat the variables for compound polygons*/
  compound_lenght = 0;
  compound_type = 0;
  compound_count = -1;
  data3d = 0;
  data4d = 0;

  if (ind->_atomic != OCI_IND_NULL) { /* not a null object */
    nelems = nords = 0;
    success = TRY( hand, OCICollSize( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info, &nelems ) )
              && TRY( hand, OCICollSize( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, &nords ) )
              && TRY( hand, OCINumberToInt( hand->errhp, &(obj->gtype), (uword)sizeof(int), OCI_NUMBER_SIGNED, (dvoid *)&gtype ) );
    /*&& (nords%2==0 && nelems%3==0);*/ /* check %2==0 for 2D geometries; and %3==0 for element info triplets */

    if (success && osCheck2DGtype(gtype)) {
      success = (nords%2==0 && nelems%3==0)?1:0; /* check %2==0 for 2D geometries; and %3==0 for element info triplets */
    } else if (success && osCheck3DGtype(gtype)) {
      success = (nords%3==0 && nelems%3==0)?1:0; /* check %2==0 for 2D geometries; and %3==0 for element info triplets */
      data3d = 1;

    } else if (success && osCheck4DGtype(gtype)) {
      success = (nords%4==0 && nelems%3==0)?1:0; /* check %2==0 for 2D geometries; and %3==0 for element info triplets */
      data4d = 1;
    }


    if (success) {
      /* reading SDO_POINT from SDO_GEOMETRY for a 2D/3D point geometry */
      if ((gtype==2001 || gtype==3001 || gtype==4001 ) && ind->point._atomic == OCI_IND_NOTNULL && ind->point.x == OCI_IND_NOTNULL && ind->point.y == OCI_IND_NOTNULL) {


        success = TRY( hand, OCINumberToReal( hand->errhp, &(obj->point.x), (uword)sizeof(double), (dvoid *)&x ) )
                  && TRY( hand, OCINumberToReal( hand->errhp, &(obj->point.y), (uword)sizeof(double), (dvoid *)&y ) );

        if (ERROR( "osGetOrdinates()", hand, dthand ))
          return MS_FAILURE;

        shape->type = MS_SHAPE_POINT;

        points.numpoints = 1;
        points.point = point5;
        point5[0].x = x;
        point5[0].y = y;
        if (data3d || data4d) {
          if (ind->point.z == OCI_IND_NOTNULL) {
            success = TRY( hand, OCINumberToReal( hand->errhp, &(obj->point.z), (uword)sizeof(double), (dvoid *)&z ));
            if (ERROR( "osGetOrdinates()", hand, dthand ))
              return MS_FAILURE;
            else
              point5[0].z = z;
          } else
            point5[0].z = z;
        }
        msAddLine( shape, &points );
        return MS_SUCCESS;
      }
      /* if SDO_POINT not fetched, proceed reading elements (element info/ordinates) */
      success = TRY( hand, OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info,(sb4)0, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
                && TRY( hand, OCINumberToInt( hand->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_SIGNED, (dvoid *)&ord_end ) );

      elem = 0;
      ord_end--; /* shifts offset from 1..n to 0..n-1 */

      do {
        ord_start = ord_end;
        if (elem+3 >= nelems) { /* processing last element */
          ord_end = nords;
          success = 1;
        } else { /* get start ordinate position for next element which is ord_end for this element */
          success = TRY( hand, OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info, (sb4)elem+3, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
                    && TRY( hand, OCINumberToInt( hand->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_SIGNED, (dvoid *)&ord_end ) );

          ord_end--; /* shifts offset from 1..n to 0..n-1 */
        }
        success = (success
                   && TRY( hand, OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info, (sb4)elem+1, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0) )
                   && TRY( hand, OCINumberToInt( hand->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_UNSIGNED, (dvoid *)&etype ) )
                   && TRY( hand, OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info, (sb4)elem+2, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
                   && TRY( hand, OCINumberToInt( hand->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_UNSIGNED, (dvoid *)&interpretation ) ));

        if (ERROR( "osGetOrdinates()", hand, dthand ))
          return MS_FAILURE;

        if ( etype == 1005 || etype == 2005  || etype == 4 ) {
          compound_type = 1;
          compound_lenght = interpretation;
          compound_count = 0;
          msInitShape(&newshape);
        }

        elem_type = (etype == 1 && interpretation > 1) ? 10 : ((etype%10)*10 + interpretation);


        /* msDebug("osGetOrdinates shape->index = %ld\telem_type =  %d\n",shape->index, elem_type); */

        switch (elem_type) {
          case 10: /* point cluster with 'interpretation'-points */
            osPointCluster (hand, shape, obj, ord_start, ord_end, points, interpretation, data3d, data4d);
            break;
          case 11: /* point type */
            osPoint(hand, shape, obj, ord_start, ord_end, points, point5, data3d, data4d);
            break;
          case 21: /* line string whose vertices are connected by straight line segments */
            if (compound_type)
              osClosedPolygon(hand, &newshape, obj, ord_start, (compound_count<compound_lenght)?ord_end+2:ord_end, points, elem_type, data3d, data4d);
            else
              osClosedPolygon(hand, shape, obj, ord_start, ord_end, points, elem_type, data3d, data4d);
            break;
          case 22: /* compound type */
            if (compound_type)
              osArcPolygon(hand, &newshape, obj, ord_start, (compound_count<compound_lenght)?ord_end+2:ord_end , points, elem_type, data3d, data4d);
            else
              osArcPolygon(hand, shape, obj, ord_start, ord_end, points, elem_type, data3d, data4d);
            break;
          case 31: /* simple polygon with n points, last point equals the first one */
            osClosedPolygon(hand, shape, obj, ord_start, ord_end, points, elem_type, data3d, data4d);
            break;
          case 32: /* Polygon with arcs */
            osArcPolygon(hand, shape, obj, ord_start, ord_end, points, elem_type, data3d, data4d);
            break;
          case 33: /* rectangle defined by 2 points */
            osRectangle(hand, shape, obj, ord_start, ord_end, points, point5, data3d, data4d);
            break;
          case 34: /* circle defined by 3 points */
            osCircle(hand, shape, obj, ord_start, ord_end, points, point5, data3d, data4d);
            break;
        }

        if (compound_count >= compound_lenght) {
          osCloneShape(&newshape, shape, data3d, data4d);
          msFreeShape(&newshape);
        }

        if (ERROR( "osGetOrdinates()", hand, dthand ))
          return MS_FAILURE;

        /* prepare for next cycle */
        ord_start = ord_end;
        elem += 3;

        if (compound_type)
          compound_count++;

      } while (elem < nelems); /* end of element loop */
    } /* end of gtype big-if */
  } /* end of not-null-object if */

  if (compound_type){
    if (gtype == 2003 || gtype == 2007)
      shape->type = MS_SHAPE_POLYGON;
    msFreeShape(&newshape);
  }

  return MS_SUCCESS;
}

static void msOCICloseConnection( void *hand )
{
  msOCICloseHandlers( (msOracleSpatialHandler *)hand );
}

/* opens a layer by connecting to db with username/password@database stored in layer->connection */
int msOracleSpatialLayerOpen( layerObj *layer )
{
  char *username = NULL, *password = NULL, *dblink =  NULL;

  msOracleSpatialLayerInfo *layerinfo = NULL;
  msOracleSpatialDataHandler *dthand = NULL;
  msOracleSpatialStatement *sthand = NULL, *sthand2 = NULL;
  msOracleSpatialHandler *hand = NULL;

  if (layer->debug >=2)
    msDebug("msOracleSpatialLayerOpen called with: %s (Layer pointer %p)\n",layer->data, layer);

  if (layer->layerinfo != NULL) /* Skip if layer is already open */
    return MS_SUCCESS;

  if (layer->data == NULL) {
    msDebug(    "Error parsing OracleSpatial DATA variable. Must be:"
                "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'."
                "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE.\n");
    msSetError( MS_ORACLESPATIALERR,
                "Error parsing OracleSpatial DATA variable. More info in server logs", "msOracleSpatialLayerOpen()");

    return MS_FAILURE;
  }

  layerinfo = (msOracleSpatialLayerInfo *)malloc(sizeof(msOracleSpatialLayerInfo));
  dthand = (msOracleSpatialDataHandler *)malloc(sizeof(msOracleSpatialDataHandler));
  sthand = (msOracleSpatialStatement *)malloc(sizeof(msOracleSpatialStatement));
  sthand2 = (msOracleSpatialStatement *)malloc(sizeof(msOracleSpatialStatement));

  if ((dthand == NULL) || (layerinfo == NULL)) {
    msSetError(MS_MEMERR, NULL, "msOracleSpatialLayerOpen()");
    return MS_FAILURE;
  }

  memset( dthand, 0, sizeof(msOracleSpatialDataHandler) );
  memset( sthand, 0, sizeof(msOracleSpatialStatement) );
  memset( sthand2, 0, sizeof(msOracleSpatialStatement) );
  memset( layerinfo, 0, sizeof(msOracleSpatialLayerInfo) );
  layerinfo->paging = MS_TRUE;

  msSplitLogin( layer->connection, layer->map, &username, &password, &dblink );

  hand = (msOracleSpatialHandler *) msConnPoolRequest( layer );

  if( hand == NULL ) {

    hand = msOCISetHandlers( username, password, dblink );

    if (hand == NULL) {
      msOCICloseDataHandlers( dthand );
      msOCIFinishStatement( sthand );
      msOCIFinishStatement( sthand2 );
      msOCIClearLayerInfo( layerinfo );

      if (username) free(username);
      if (password) free(password);
      if (dblink) free(dblink);

      return MS_FAILURE;
    }

    if ( layer->debug >=2)
      msDebug("msOracleSpatialLayerOpen. Shared connection not available. Creating one.\n");

    msConnPoolRegister( layer, hand, msOCICloseConnection );
  } else {
    hand->ref_count++;
    hand->last_oci_status = MS_SUCCESS;
    hand->last_oci_error[0] = (text)'\0';
  }

  if (!(msOCISetDataHandlers(hand, dthand) & msOCIOpenStatement(hand, sthand) & msOCIOpenStatement(hand, sthand2))) {
    msSetError( MS_ORACLESPATIALERR,
                "Cannot create OCI LayerInfo. "
                "Connection failure.",
                "msOracleSpatialLayerOpen()");

    if (layer->debug>=2)
      msDebug("msOracleSpatialLayerOpen. Cannot create OCI LayerInfo. Connection failure.\n");

    msOCICloseDataHandlers ( dthand );
    msOCICloseHandlers( hand );
    msOCIClearLayerInfo( layerinfo );
  }
  layerinfo->orahandlers = hand;
  layerinfo->oradatahandlers = dthand;
  layerinfo->orastmt = sthand;
  layerinfo->orastmt2 = sthand2;
  layer->layerinfo = layerinfo;

  if (username) free(username);
  if (password) free(password);
  if (dblink) free(dblink);

  return layer->layerinfo != NULL ? MS_SUCCESS : MS_FAILURE;
}

/* return MS_TRUE if layer is open, MS_FALSE otherwise.*/
int msOracleSpatialLayerIsOpen(layerObj *layer)
{
  if (layer->layerinfo != NULL)
    return MS_TRUE;

  return MS_FALSE;
}

/* closes the layer, disconnecting from db if connected. layer->layerinfo is freed */
int msOracleSpatialLayerClose( layerObj *layer )
{
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;
  /*int lIntSuccessFree = 0;*/

  if (layer->debug>=2)
    msDebug("msOracleSpatialLayerClose was called. Layer: %p, Layer name: %s\n", layer, layer->name);

  if (layerinfo != NULL) {

    /*
     * Some errors with the OCIObjectFree with query function
     * If uncomment draw mode will work but no query modes
     * Need to investigate the error cause
     */
    /*lIntSuccessFree = TRY (layerinfo->orahandlers, OCIObjectFree(layerinfo->orahandlers->envhp, layerinfo->orahandlers->errhp, (dvoid *)layerinfo->obj, (ub2)OCI_OBJECTFREE_FORCE));
     *if (!lIntSuccessFree)
     *  msDebug("Error: %s\n", layerinfo->orahandlers->last_oci_error);
     */
    /* Release Statement Handles */
    if (layerinfo->orastmt != NULL) {
      msOCIFinishStatement(layerinfo->orastmt);
      layerinfo->orastmt = NULL;
    }
    if (layerinfo->orastmt2 != NULL) {
      msOCIFinishStatement(layerinfo->orastmt2);
      layerinfo->orastmt2 = NULL;
    }

    /* Release Datahandlers */
    if (layer->debug>=4)
      msDebug("msOracleSpatialLayerClose. Cleaning layerinfo handlers.\n");
    msOCICloseDataHandlers( layerinfo->oradatahandlers );
    layerinfo->oradatahandlers = NULL;

    /* Free the OCI cache only if there is no more layer that could use it */
    layerinfo->orahandlers->ref_count--;
    if (layerinfo->orahandlers->ref_count == 0)
      OCICacheFree (layerinfo->orahandlers->envhp, layerinfo->orahandlers->errhp, layerinfo->orahandlers->svchp);

    /* Release Mapserver Pool */
    if (layer->debug>=4)
      msDebug("msOracleSpatialLayerClose. Release the Oracle Pool.\n");
    msConnPoolRelease( layer, layerinfo->orahandlers );

    /* Release Layerinfo */
    layerinfo->orahandlers = NULL;

    msOCIClearLayerInfo( layerinfo );
    layer->layerinfo = NULL;
  }

  return MS_SUCCESS;
}

/* create SQL statement for retrieving shapes */
/* Sets up cursor for use in *NextShape and *GetShape */
int msOracleSpatialLayerWhichShapes( layerObj *layer, rectObj rect, int isQuery)
{
  int success, i;
  int function = 0;
  int version = 0;
  char * query_str = NULL;
  char query_str2[256];
  char *tmp_str=NULL, *tmp1_str=NULL;
  char *table_name;
  char *geom_column_name = NULL, *unique = NULL, *srid = NULL, *indexfield=NULL;
  OCIDefine *adtp = NULL;
  OCIDefine **items = NULL;
  OCINumber oci_number;
  OCIBind *bnd1p = NULL,  *bnd2p = NULL;

  int existunique = MS_FALSE;
  int rownumisuniquekey = MS_FALSE;
  int numitemsinselect = 0;

  /* get layerinfo */
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;
  msOracleSpatialDataHandler *dthand = NULL;
  msOracleSpatialHandler *hand = NULL;
  msOracleSpatialStatement *sthand = NULL;

  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerWhichShapes was called.\n");

  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR,
                "msOracleSpatialLayerWhichShapes called on unopened layer",
                "msOracleSpatialLayerWhichShapes()" );

    return MS_FAILURE;
  } else {
    dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
    hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    sthand = (msOracleSpatialStatement *)layerinfo->orastmt2;
  }

  /*init uniqueindex field*/
  sthand->uniqueidindex=0;

  table_name = (char *) malloc(sizeof(char) * TABLE_NAME_SIZE);
  /* parse geom_column_name and table_name */
  if (!msSplitData( layer->data, &geom_column_name, &table_name, &unique, &srid, &indexfield, &function, &version)) {
    msDebug( "Error parsing OracleSpatial DATA variable. Must be:"
                "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'."
                "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE."
                "Your data statement: (%s)"
                "in msOracleSpatialLayerWhichShapes()\n", layer->data );
    msSetError( MS_ORACLESPATIALERR,
                "Error parsing OracleSpatial DATA variable. More info in server logs",
                "msOracleSpatialLayerWhichShapes()" );

    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    free(table_name);
    return MS_FAILURE;
  }

  /*rownum will be the first in the select if no UNIQUE key is defined or
    will be added at the end of the select*/
  rownumisuniquekey = MS_TRUE;
  /*Define the unique*/
  if (unique[0] == '\0')
    strcpy( unique, "rownum" );
  else
    rownumisuniquekey = MS_FALSE;

  /* Check if the unique field is already in the items list */
  existunique = MS_FALSE;
  if (unique) {
    for( i=0; i < layer->numitems; ++i ) {
      if (strcasecmp(unique, layer->items[i])==0) {
        sthand->uniqueidindex = i;
        existunique = MS_TRUE;
        break;
      }
    }
  }

  /* If no SRID is provided, set it to -1 (NULL) for binding */
  if (strcmp(srid,"NULL") == 0)
    strcpy(srid,"-1");

  query_str = msStringConcatenate(query_str, "SELECT ");
  numitemsinselect = layer->numitems;
  /* allocate enough space for items */
  if (layer->numitems >= 0) {
    /*we will always add a rownumber in the selection*/
    numitemsinselect++;

    /*if unique key in the data is specfied and is not part of the current item lists,
      we should add it to the select*/
    if(existunique == MS_FALSE && rownumisuniquekey == MS_FALSE)
      numitemsinselect++;

    /*the usinque index is there was no uniqueid given or the uniqueid given was not part of the items lists*/
    if (existunique == MS_FALSE)
      sthand->uniqueidindex = layer->numitems;

    sthand->items = (item_text_array *)malloc( sizeof(item_text_array) * (numitemsinselect) );
    if (sthand->items == NULL) {
      msSetError( MS_ORACLESPATIALERR,"Cannot allocate layerinfo->items buffer","msOracleSpatialLayerWhichShapes()" );
      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if(indexfield) free(indexfield);
      if (query_str) free(query_str);
      free(table_name);
      return MS_FAILURE;
    }

    items = (OCIDefine **)malloc(sizeof(OCIDefine *)*(numitemsinselect));
    if (items == NULL) {
      msSetError( MS_ORACLESPATIALERR,"Cannot allocate items buffer","msOracleSpatialLayerWhichShapes()" );
      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if(indexfield) free(indexfield);
      free(table_name);
      if (query_str) free(query_str);
      return MS_FAILURE;
    }
    memset(items ,0,sizeof(OCIDefine *)*(numitemsinselect));
  }

  /* define SQL query */
  for( i=0; i < layer->numitems; ++i ) {
      query_str = msStringConcatenate(query_str, "\"");
      query_str = msStringConcatenate(query_str, layer->items[i]);
      query_str = msStringConcatenate(query_str, "\", ");
  }

  /*we add the uniqueid if it was not part of the current item list*/
  if(existunique == MS_FALSE && rownumisuniquekey == MS_FALSE) {
    query_str = msStringConcatenate(query_str, unique);
    query_str = msStringConcatenate(query_str, ",");
  }

  /*we always want to add rownum in the selection to allow paging to work*/
  query_str = msStringConcatenate(query_str, "rownum, ");

  query_str = msStringConcatenate(query_str, geom_column_name);
  query_str = msStringConcatenate(query_str, " FROM ");
  query_str = msStringConcatenate(query_str, table_name);

  query_str = osFilteritem(layer, function, query_str, 1);

  if (layerinfo->paging && layer->maxfeatures > 0 && layer->startindex < 0) {
    if (function == FUNCTION_NONE && layer->filter.native_string == NULL) {
      query_str = msStringConcatenate(query_str, " WHERE ");
    } else if (function == FUNCTION_NONE && layer->filter.native_string != NULL) {
      query_str = msStringConcatenate(query_str, " AND ");
    }
    // avoiding an allocation on the heap here sounds acceptable
    char tmpBuf[256];
    snprintf(tmpBuf, sizeof(tmpBuf), " ROWNUM<=%d ", layer->maxfeatures);
    query_str = msStringConcatenate(query_str, tmpBuf);

    if (function != FUNCTION_NONE) {
      query_str = msStringConcatenate(query_str, " AND ");
    }
  }

  if ((((atol(srid) >= 8192) && (atol(srid) <= 8330))
   || (atol(srid) == 2) || (atol(srid) == 5242888)
    || (atol(srid) == 2000001)) && (version == VERSION_9i)) {
    query_str = osGeodeticData(function, query_str, geom_column_name, indexfield);
  }
  else {
    query_str = osNoGeodeticData(function, version, query_str, geom_column_name, indexfield);
  }

  if( layer->sortBy.nProperties > 0 ) {
      msDebug("Layer sorting is requested\n");
      tmp1_str = msLayerBuildSQLOrderBy(layer);
      query_str = msStringConcatenate(query_str, " ORDER BY ");
      query_str = msStringConcatenate(query_str, tmp1_str);
      query_str = msStringConcatenate(query_str, "  ");
      msFree(tmp1_str);
    }

  /*assuming startindex starts at 1*/
  if (layerinfo->paging && layer->startindex > 0) {
    tmp1_str = msStrdup("SELECT * from (SELECT atmp.*, ROWNUM rnum from (");
    tmp_str = msStringConcatenate(tmp_str,  tmp1_str);
    msFree(tmp1_str);
    tmp_str = msStringConcatenate(tmp_str, query_str);
    /*oder by rowid does not seem to work with function using the spatial filter, at least
      on layers loaded using ogr in a 11g database*/
    if (function == FUNCTION_NONE  || function == FUNCTION_RELATE || function == FUNCTION_GEOMRELATE)
      tmp1_str = msStrdup( "order by rowid");
    /* Populate strOrderBy, if necessary */
    else
      tmp1_str = msStrdup("");

    if (layer->maxfeatures > 0) {
      snprintf(query_str2, sizeof(query_str2),  " %s) atmp where ROWNUM<=%d) where rnum >=%d",   tmp1_str,
               layer->maxfeatures+layer->startindex-1,  layer->startindex);
    } else {
      snprintf( query_str2, sizeof(query_str2),  " %s) atmp) where rnum >=%d",  tmp1_str, layer->startindex);
    }
    msFree(tmp1_str);

    tmp_str = msStringConcatenate(tmp_str,  query_str2);
    memset(query_str,0,strlen(query_str));
    query_str = msStringConcatenate(query_str, tmp_str);
    msFree(tmp_str);
  }


  if (layer->debug)
    msDebug("msOracleSpatialLayerWhichShapes. Using this Sql to retrieve the data: %s\n", query_str);

  if (layer->debug)
    msDebug("msOracleSpatialLayerWhichShapes. Bind values: srid:%s   minx:%f   miny:%f  maxx:%f   maxy:%f \n", srid, rect.minx, rect.miny, rect.maxx, rect.maxy);

  /* prepare */
  success = TRY( hand, OCIStmtPrepare( sthand->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) );

  if (layer->debug>=4)
    msDebug("msOracleSpatialLayerWhichShapes getting ordinate definition.\n");

  /* get the object definition of the ordinate array */
  ordinates_tdo = get_tdo(SDO_ORDINATE_ARRAY, hand, dthand );

  if (layer->debug>=4)
    msDebug("msOracleSpatialLayerWhichShapes converting to OCIColl.\n");

  /* initialized the collection array */
  success = TRY( hand, OCIObjectNew(hand->envhp, hand->errhp, hand->svchp, OCI_TYPECODE_VARRAY, ordinates_tdo, (dvoid *)NULL, OCI_DURATION_SESSION, FALSE, (dvoid **)&ordinates) );

  /* convert it to a OCI number and then append minx...maxy to the collection */
  success = TRY ( hand, OCINumberFromReal(hand->errhp, (dvoid *)&(rect.minx), (uword)sizeof(double),(dvoid *)&oci_number))
            &&TRY ( hand, OCICollAppend(hand->envhp, hand->errhp,(dvoid *) &oci_number,(dvoid *)0, (OCIColl *)ordinates))
            &&TRY ( hand, OCINumberFromReal(hand->errhp, (dvoid *)&(rect.miny), (uword)sizeof(double),(dvoid *)&oci_number))
            &&TRY ( hand, OCICollAppend(hand->envhp, hand->errhp,(dvoid *) &oci_number,(dvoid *)0, (OCIColl *)ordinates))
            &&TRY ( hand, OCINumberFromReal(hand->errhp, (dvoid *)&(rect.maxx), (uword)sizeof(double),(dvoid *)&oci_number))
            &&TRY ( hand, OCICollAppend(hand->envhp, hand->errhp,(dvoid *) &oci_number,(dvoid *)0, (OCIColl *)ordinates))
            &&TRY ( hand, OCINumberFromReal(hand->errhp, (dvoid *)&(rect.maxy), (uword)sizeof(double),(dvoid *)&oci_number))
            &&TRY ( hand, OCICollAppend(hand->envhp, hand->errhp,(dvoid *) &oci_number,(dvoid *)0, (OCIColl *)ordinates));


  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerWhichShapes bind by name and object.\n");

  /* do the actual binding */

  if (success && function != FUNCTION_NONE) {
    success = TRY( hand,
                   /* bind in srid */
                   OCIBindByName( sthand->stmthp, &bnd2p,  hand->errhp, (text *) ":srid", strlen(":srid"),(ub1 *) srid,  strlen(srid)+1, SQLT_STR,
                                  (dvoid *) 0, (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT) )
              && TRY(hand,
                     /* bind in rect by name */
                     OCIBindByName( sthand->stmthp, &bnd1p,  hand->errhp,  (text *)":ordinates", -1, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0,(ub2 *)0, (ub4)0, (ub4 *)0, (ub4)OCI_DEFAULT))
              && TRY(hand,
                     /* bind in rect object */
                     OCIBindObject(bnd1p, hand->errhp, ordinates_tdo, (dvoid **)&ordinates, (ub4 *)0, (dvoid **)0, (ub4 *)0));

    /* bind the variables from the bindvals hash */
    const char* bind_key = msFirstKeyFromHashTable(&layer->bindvals);
    while(bind_key != NULL) {
      const char* bind_value = msLookupHashTable(&layer->bindvals, bind_key);
      char* bind_tag = (char*)malloc(sizeof(char) * strlen(bind_key) + 2);
      sprintf(bind_tag, ":%s", bind_key);

      success = success && TRY(hand, OCIBindByName( sthand->stmthp, &bnd2p,  hand->errhp, (text *)bind_tag, strlen(bind_tag),(ub1 *) bind_value,  strlen(bind_value)+1, SQLT_STR,
                               (dvoid *) 0, (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));
      bind_key = msNextKeyFromHashTable(&layer->bindvals, bind_key);
    }
  }


  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerWhichShapes name and object now bound.\n");


  if (success && layer->numitems >= 0) {
    for( i=0; i < numitemsinselect && success; ++i ){
      if (i< numitemsinselect -1) {
        //msDebug("Defining by POS, for %s\n", layer->items[i]);
        //msDebug("datatype retrieved is %i\n", layer->iteminfo);
      }
      success = TRY( hand, OCIDefineByPos( sthand->stmthp, &items[i], hand->errhp, (ub4)i+1, (dvoid *)sthand->items[i], (sb4)TEXT_SIZE, SQLT_STR, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) );
    }
  }


  if (success) {
    int cursor_type = OCI_DEFAULT;
    if(isQuery) cursor_type =OCI_STMT_SCROLLABLE_READONLY;

    success = TRY( hand,
                   /* define spatial position adtp ADT object */
                   OCIDefineByPos( sthand->stmthp, &adtp, hand->errhp, (ub4)numitemsinselect+1, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
              && TRY( hand,
                      /* define object tdo from adtp */
                      OCIDefineObject( adtp, hand->errhp, dthand->tdo, (dvoid **)sthand->obj, (ub4 *)0, (dvoid **)sthand->ind, (ub4 *)0 ) )
              && TRY(hand,
                     /* execute */
                     OCIStmtExecute( hand->svchp, sthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)cursor_type ) )
              &&  TRY( hand,
                       /* get rows fetched */
                       OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROWS_FETCHED, hand->errhp ) )
              && TRY( hand,
                      /* get rows count */
                      OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_count, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ) );
  }

  if (!success) {
    msDebug( "Error: %s . "
                "Query statement: %s . "
                "Check your data statement."
                "msOracleSpatialLayerWhichShapes()\n", hand->last_oci_error, query_str );
    msSetError( MS_ORACLESPATIALERR,
                "Check your data statement and server logs",
                "msOracleSpatialLayerWhichShapes()");

    /* clean items */
    free(items);
    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    if (query_str) free(query_str);
    free(table_name);

    return MS_FAILURE;
  }

  /* should begin processing first row */
  sthand->row_num = sthand->row = 0;

  /* clean items */
  free(items);
  // TODO why not msFree() instead to avoid if statements ?
  if (geom_column_name) free(geom_column_name);
  if (srid) free(srid);
  if (unique) free(unique);
  if (indexfield) free(indexfield);
  if (query_str) free(query_str);
  free(table_name);

  return MS_SUCCESS;
}

/* fetch next shape from previous SELECT stmt (see *WhichShape()) */
int msOracleSpatialLayerNextShape( layerObj *layer, shapeObj *shape )
{
  SDOGeometryObj *obj;
  SDOGeometryInd *ind;
  int success, /*lIntSuccessFree,*/ i;

  /* get layerinfo */
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;
  msOracleSpatialDataHandler *dthand = NULL;
  msOracleSpatialHandler *hand = NULL;
  msOracleSpatialStatement *sthand = NULL;


  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerWhichShapes called on unopened layer", "msOracleSpatialLayerNextShape()" );
    return MS_FAILURE;
  } else {
    dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
    hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    sthand = (msOracleSpatialStatement *)layerinfo->orastmt2;
  }

  /* no rows fetched */
  if (sthand->rows_fetched == 0)
    return MS_DONE;

  if(layer->debug >=5 )
    msDebug("msOracleSpatialLayerNextShape on layer %p, row_num: %d\n", layer, sthand->row_num);

  do {
    /* is buffer empty? */
    if (sthand->row >= sthand->rows_fetched) {
      /* fetch more */
      success = TRY( hand, OCIStmtFetch2( sthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub2)OCI_FETCH_NEXT, (sb4)0, (ub4)OCI_DEFAULT ) )
                && TRY( hand, OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROWS_FETCHED, hand->errhp ) )
                && TRY( hand, OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_count, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ) );


      if (!success || sthand->rows_fetched == 0 || sthand->row_num >= sthand->rows_count) {
        hand->last_oci_status=MS_SUCCESS;
        return MS_DONE;
      }
      if(layer->debug >= 4 )
        msDebug("msOracleSpatialLayerNextShape on layer %p, Fetched %d more rows (%d total)\n", layer, sthand->rows_fetched, sthand->rows_count);

      sthand->row = 0; /* reset buffer row index */
    }

    /* set obj & ind for current row */
    obj = sthand->obj[ sthand->row ];
    ind = sthand->ind[ sthand->row ];

    /* get the items for the shape */
    shape->index = atol( (char *)(sthand->items[sthand->uniqueidindex][ sthand->row ])); /* Primary Key */
    shape->resultindex = sthand->row_num;
    shape->numvalues = layer->numitems;

    shape->values = (char **)malloc( sizeof(char*) * shape->numvalues );
    if (shape->values == NULL) {
      msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the values", "msOracleSpatialLayerNextShape()" );
      return MS_FAILURE;
    }

    for( i=0; i < shape->numvalues; ++i ) {
      shape->values[i] = (char *)malloc(strlen((char *)sthand->items[i][ sthand->row ])+1);
      if (shape->values[i] == NULL) {
        msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items", "msOracleSpatialLayerNextShape()" );
        return MS_FAILURE;
      } else {
        strcpy(shape->values[i], (char *)sthand->items[i][ sthand->row ]);
        shape->values[i][strlen((char *)sthand->items[i][ sthand->row ])] = '\0';
      }
    }

    /* fetch a layer->type object */
    success = osGetOrdinates(dthand, hand, shape, obj, ind);

    /* increment for next row */
    sthand->row_num++;
    sthand->row++;

    if (success != MS_SUCCESS) {
      return MS_FAILURE;
    }

    osShapeBounds(shape);
  } while(shape->type == MS_SHAPE_NULL);

  return MS_SUCCESS;
}

int msOracleSpatialLayerGetShape( layerObj *layer, shapeObj *shape, resultObj *record)
{
  int success, i;
  SDOGeometryObj *obj;
  SDOGeometryInd *ind;
  msOracleSpatialDataHandler *dthand = NULL;
  msOracleSpatialHandler *hand = NULL;
  msOracleSpatialLayerInfo *layerinfo;
  msOracleSpatialStatement *sthand = NULL;

  long shapeindex = record->shapeindex;
  int resultindex = record->resultindex;

  if(layer == NULL) {
    msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetShape called on unopened layer","msOracleSpatialLayerGetShape()" );
    return MS_FAILURE;
  }

  layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;

  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetShape called on unopened layer (layerinfo)","msOracleSpatialLayerGetShape()" );
    return MS_FAILURE;
  }

  /* If resultindex is set, fetch the shape from the resultcache, otherwise fetch it from the DB  */
  if (resultindex >= 0) {
    long buffer_first_row_num, buffer_last_row_num;

    /* get layerinfo */
    dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
    hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    sthand = (msOracleSpatialStatement *)layerinfo->orastmt2;

    if (layer->resultcache == NULL) {
      msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetShape called before msOracleSpatialLayerWhichShapes()","msOracleSpatialLayerGetShape()" );
      return MS_FAILURE;
    }

    if ((unsigned)resultindex >= sthand->rows_count) {
      if (layer->debug >= 5)
               msDebug("msOracleSpatialLayerGetShape problem with cursor. Trying to fetch record = %d of %d, falling back to GetShape\n", resultindex, sthand->rows_count);

      msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetShape record out of range","msOracleSpatialLayerGetShape()" );
      return MS_FAILURE;
    }

    if (layer->debug >= 5)
      msDebug("msOracleSpatialLayerGetShape was called. Using the record = %d of %d. (shape: %ld should equal pkey: %ld)\n",
              resultindex, layer->resultcache->numresults, layer->resultcache->results[resultindex].shapeindex, shapeindex);

    /* NOTE: with the way the resultcache works, we should see items in increasing order, but some may have been filtered out. */
    /* Best case: item in buffer */
    /* Next best case: item is in next fetch block */
    /* Worst case: item is random access */
    buffer_first_row_num = sthand->row_num - sthand->row; /* cursor id of first item in buffer */
    buffer_last_row_num  = buffer_first_row_num + sthand->rows_fetched - 1; /* cursor id of last item in buffer */
    if(resultindex >= buffer_first_row_num && resultindex <= buffer_last_row_num) { /* Item is in buffer. Calculate position in buffer */
      sthand->row += resultindex - sthand->row_num; /* move sthand row an row_num by offset from last call */
      sthand->row_num += resultindex - sthand->row_num;
    } else { /* Item is not in buffer. Fetch item from Oracle */
      if (layer->debug >= 4)
        msDebug("msOracleSpatialLayerGetShape: Fetching result from DB start: %ld end:%ld record: %d\n", buffer_first_row_num, buffer_last_row_num, resultindex);

      success = TRY( hand, OCIStmtFetch2( sthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub2)OCI_FETCH_ABSOLUTE, (sb4)resultindex+1, (ub4)OCI_DEFAULT ) )
                && TRY( hand, OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROWS_FETCHED, hand->errhp ) );

      sthand->row_num = resultindex;
      sthand->row = 0; /* reset row index */

      if(ERROR("msOracleSpatialLayerGetShape", hand, dthand))
          return MS_FAILURE;

      if (!success || sthand->rows_fetched == 0) {
        msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetShape could not fetch specified record.", "msOracleSpatialLayerGetShape()" );
        return MS_FAILURE;
      }
    }

    /* set obj & ind for current row */
    obj = sthand->obj[ sthand->row ];
    ind = sthand->ind[ sthand->row ];

    /* get the items for the shape */
    shape->index = shapeindex; /* By definition this is what we asked for */
    shape->numvalues = layer->numitems;

    shape->values = (char **)malloc( sizeof(char*) * shape->numvalues );
    if (shape->values == NULL) {
      msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the values", "msOracleSpatialLayerNextShape()" );
      return MS_FAILURE;
    }

    for( i=0; i < shape->numvalues; ++i ) {
      shape->values[i] = (char *)malloc(strlen((char *)sthand->items[i][ sthand->row ])+1);
      if (shape->values[i] == NULL) {
        msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items", "msOracleSpatialLayerNextShape()" );
        return MS_FAILURE;
      } else {
        strcpy(shape->values[i], (char *)sthand->items[i][ sthand->row ]);
        shape->values[i][strlen((char *)sthand->items[i][ sthand->row ])] = '\0';
      }
    }

    /* fetch a layer->type object */
    success = osGetOrdinates(dthand, hand, shape, obj, ind);

    if (success != MS_SUCCESS) {
      msSetError( MS_ORACLESPATIALERR, "Call to osGetOrdinates failed.", "msOracleSpatialLayerGetShape()" );
      return MS_FAILURE;
    }

    osShapeBounds(shape);
    if(shape->type == MS_SHAPE_NULL)  {
      msSetError( MS_ORACLESPATIALERR, "Shape type is null... this probably means a record number was requested that could not have beeen in a result set (as returned by NextShape).", "msOracleSpatialLayerGetShape()" );
      return MS_FAILURE;
    }

    return (MS_SUCCESS);
  } else { /* no resultindex, fetch the shape from the DB */
    char *table_name;
    char *query_str = NULL, *geom_column_name = NULL, *unique = NULL, *srid = NULL, *indexfield = NULL;
    int function = 0;
    int version = 0;
    sb2 *nullind = NULL;
    /*OCIDefine *adtp = NULL, *items[QUERY_SIZE] = { NULL };*/
    OCIDefine *adtp = NULL;
    OCIDefine  **items = NULL;

    if (layer->debug>=4)
      msDebug("msOracleSpatialLayerGetShape was called. Using the record = %ld.\n", shapeindex);

    dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
    hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    sthand = (msOracleSpatialStatement *) layerinfo->orastmt;

    /* allocate enough space for items */
    if (layer->numitems > 0) {
      if (sthand->items_query == NULL)
        sthand->items_query = (item_text_array_query *)malloc( sizeof(item_text_array_query) * (layer->numitems) );

      if (sthand->items_query == NULL) {
        msSetError( MS_ORACLESPATIALERR, "Cannot allocate layerinfo->items_query buffer", "msOracleSpatialLayerGetShape()" );
        return MS_FAILURE;
      }

      nullind = (sb2 *)malloc( sizeof(sb2) * (layer->numitems) );
      if (nullind == NULL) {
        msSetError( MS_ORACLESPATIALERR, "Cannot allocate nullind buffer", "msOracleSpatialLayerGetShape()" );
        return MS_FAILURE;
      }
      memset(nullind ,0, sizeof(sb2) * (layer->numitems) );

      items = (OCIDefine **)malloc(sizeof(OCIDefine *)*layer->numitems);
      if (items == NULL) {
        msSetError( MS_ORACLESPATIALERR,"Cannot allocate items buffer","msOracleSpatialLayerWhichShapes()" );

        /* clean nullind  */
        free(nullind);

        return MS_FAILURE;
      }
      memset(items ,0,sizeof(OCIDefine *)*layer->numitems);
    }

    table_name = (char *) malloc(sizeof(char) * TABLE_NAME_SIZE);
    if (!msSplitData( layer->data, &geom_column_name, &table_name, &unique, &srid, &indexfield, &function, &version )) {
      msDebug( "Error parsing OracleSpatial DATA variable. Must be: "
                  "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                  "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
                  "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE. "
                  "Your data statement: (%s) msOracleSpatialLayerGetShape()\n", layer->data );
      msSetError( MS_ORACLESPATIALERR,
                  "Error parsing OracleSpatial DATA variable. Check server logs. ",
                  "msOracleSpatialLayerGetShape()");

      /* clean nullind  */
      free(nullind);

      /* clean items */
      free(items);

      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      free(table_name);

      return MS_FAILURE;
    }

    /*Define the first query to retrive itens*/
    if (unique[0] == '\0') {
      msSetError( MS_ORACLESPATIALERR,
                  "Error parsing OracleSpatial DATA variable for query. To execute "
                  "query functions you need to define one "
                  "unique column [USING UNIQUE <#column>]",
                  "msOracleSpatialLayerGetShape()" );

      /* clean nullind  */
      free(nullind);

      /* clean items */
      free(items);

      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      free(table_name);

      return MS_FAILURE;
    } else {
      query_str = msStringConcatenate(query_str, "SELECT");
    }
    /*Define the query*/
    for( i = 0; i < layer->numitems; ++i ) {
      msStringConcatenate(query_str, " ");
      msStringConcatenate(query_str, layer->items[i]);
      msStringConcatenate(query_str, ",");
    }
    char tmpBuf[256];
    snprintf(tmpBuf, sizeof(tmpBuf), "%ld", shapeindex);
    query_str = msStringConcatenate(query_str, " ");
    query_str = msStringConcatenate(query_str, geom_column_name);
    query_str = msStringConcatenate(query_str, " FROM ");
    query_str = msStringConcatenate(query_str, table_name);
    query_str = msStringConcatenate(query_str, " WHERE ");
    query_str = msStringConcatenate(query_str, unique);
    query_str = msStringConcatenate(query_str, " = ");
    query_str = msStringConcatenate(query_str, tmpBuf);

    /*if (layer->filter.native_string != NULL)
      sprintf( query_str + strlen(query_str), " AND %s", (layer->filter.string));*/
    query_str = osFilteritem(layer, FUNCTION_NONE, query_str, 2);


    if (layer->debug)
      msDebug("msOracleSpatialLayerGetShape. Sql: %s\n", query_str);

    /*Prepare the handlers to the query*/
    success = TRY( hand,OCIStmtPrepare( sthand->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) );

    if (success && layer->numitems > 0) {
      for( i = 0; i < layer->numitems && success; i++ )
        success = TRY( hand, OCIDefineByPos( sthand->stmthp, &items[i], hand->errhp, (ub4)i+1, (dvoid *)sthand->items_query[i], (sb4)TEXT_SIZE, SQLT_STR, (sb2 *)&nullind[i], (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) );
    }

    if(!success) {
      msSetError( MS_ORACLESPATIALERR,
                  "Error: %s . "
                  "Query statement: %s . "
                  "Check your data statement.",
                  "msOracleSpatialLayerGetShape()\n", hand->last_oci_error, query_str );

      /* clean nullind  */
      free(nullind);

      /* clean items */
      free(items);

      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      if (query_str) free(query_str);
      free(table_name);

      return MS_FAILURE;
    }

    if (success) {
      success = TRY( hand, OCIDefineByPos( sthand->stmthp, &adtp, hand->errhp, (ub4)layer->numitems+1, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
                && TRY( hand, OCIDefineObject( adtp, hand->errhp, dthand->tdo, (dvoid **)sthand->obj, (ub4 *)0, (dvoid **)sthand->ind, (ub4 *)0 ) )
                && TRY (hand, OCIStmtExecute( hand->svchp, sthand->stmthp, hand->errhp, (ub4)QUERY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ))
                && TRY (hand, OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ));

    }

    if(!success) {
      msDebug( "Error: %s . "
                  "Query statement: %s ."
                  "Check your data statement."
                  "in msOracleSpatialLayerGetShape()\n", hand->last_oci_error, query_str );
      msSetError( MS_ORACLESPATIALERR,
                  "Error in Query statement. Check your server logs","msOracleSpatialLayerGetShape()");

      /* clean nullind  */
      free(nullind);

      /* clean items */
      free(items);

      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      if (query_str) free(query_str);
      free(table_name);

      return MS_FAILURE;
    }

    shape->type = MS_SHAPE_NULL;

    /* no rows fetched */
    if (sthand->rows_fetched == 0) {
      /* clean nullind  */
      free(nullind);

      /* clean items */
      free(items);

      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      if (query_str) free(query_str);
      free(table_name);

      return (MS_DONE);
    }

    obj = sthand->obj[ sthand->row ];
    ind = sthand->ind[ sthand->row ];

    /* get the items for the shape */
    shape->numvalues = layer->numitems;
    shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
    if (shape->values == NULL) {
      msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the values.", "msOracleSpatialLayerGetShape()" );

      /* clean nullind  */
      free(nullind);

      /* clean items */
      free(items);

      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      if (query_str) free(query_str);
      free(table_name);

      return MS_FAILURE;
    }

    shape->index = shapeindex;

    for( i = 0; i < layer->numitems; ++i ) {
      shape->values[i] = (char *)malloc(strlen((char *)sthand->items_query[sthand->row][i])+1);

      if (shape->values[i] == NULL) {
        msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items buffer.", "msOracleSpatialLayerGetShape()" );

        /* clean nullind  */
        free(nullind);

        /* clean items */
        free(items);

        if (geom_column_name) free(geom_column_name);
        if (srid) free(srid);
        if (unique) free(unique);
        if (indexfield) free(indexfield);
        if (query_str) free(query_str);
        free(table_name);

        return MS_FAILURE;
      } else {
        if (nullind[i] != OCI_IND_NULL) {
          strcpy(shape->values[i], (char *)sthand->items_query[sthand->row][i]);
          shape->values[i][strlen((char *)sthand->items_query[sthand->row][i])] = '\0';
        } else {
          shape->values[i][0] = '\0';
        }
      }
    }

    /* increment for next row */
    sthand->row_num++;
    sthand->row++;

    /* fetch a layer->type object */
    success = osGetOrdinates(dthand, hand, shape, obj, ind);
    if (success != MS_SUCCESS) {
      msSetError( MS_ORACLESPATIALERR, "Cannot execute query", "msOracleSpatialLayerGetShape()" );

      /* clean nullind  */
      free(nullind);

      /* clean items */
      free(items);

      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      if (query_str) free(query_str);
      free(table_name);

      return MS_FAILURE;
    }
    osShapeBounds(shape);
    sthand->row = sthand->row_num = 0;

    /* clean nullind  */
    free(nullind);

    /* clean items */
    free(items);

    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    if (query_str) free(query_str);
    free(table_name);

    return (MS_SUCCESS);
  }
}

int msOracleSpatialLayerInitItemInfo( layerObj *layer )
{
  int i;
  int *itemindexes ;

  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerInitItemInfo was called.\n");

  if (layer->numitems == 0)
    return MS_SUCCESS;

  if (layer->iteminfo)
    free( layer->iteminfo );

  if ((layer->iteminfo = (long *)malloc(sizeof(int)*layer->numitems))== NULL) {
    msSetError(MS_MEMERR, NULL, "msOracleSpatialLayerInitItemInfo()");
    return MS_FAILURE;
  }

  itemindexes = (int*)layer->iteminfo;



  for(i=0; i < layer->numitems; i++){
    itemindexes[i] = i;  /*last one is always the geometry one - the rest are non-geom*/
  }

  return MS_SUCCESS;
}

/* AutoProjection Support for RFC 37 #3333
 * TODO: Needs testing
 */
int msOracleSpatialLayerGetAutoProjection( layerObj *layer, projectionObj *projection )
{
  char *table_name;
  char *query_str, *geom_column_name = NULL, *unique = NULL, *srid = NULL, *indexfield=NULL;
  int success;
  int function = 0;
  int version = 0;
  char wktext[4000];

  OCIDefine *def1p = NULL;
  OCIBind *bnd1p = NULL,  *bnd2p = NULL;

  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;
  msOracleSpatialHandler *hand = NULL;
  msOracleSpatialStatement *sthand = NULL;

  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerGetAutoProjection was called.\n");

  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetAutoProjection called on unopened layer","msOracleSpatialLayerGetAutoProjection()");
    return MS_FAILURE;
  } else {
    hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    sthand = (msOracleSpatialStatement *) layerinfo->orastmt;
  }

  table_name = (char *) malloc(sizeof(char) * TABLE_NAME_SIZE);
  if (!msSplitData( layer->data, &geom_column_name, &table_name, &unique, &srid, &indexfield, &function, &version )) {
    msDebug(  "Error parsing OracleSpatial DATA variable. Must be: "
                "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
                "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE. "
                "Your data statement: (%s) "
                "in msOracleSpatialLayerGetAutoProjection()\n", layer->data );

    msSetError( MS_ORACLESPATIALERR,
                "Error parsing OracleSpatial DATA variable",
                "msOracleSpatialLayerGetAutoProjection()");

    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    free(table_name);

    return MS_FAILURE;
  }

  query_str = "SELECT wktext FROM mdsys.all_sdo_geom_metadata m, mdsys.cs_srs c WHERE c.srid = m.srid and m.owner||'.'||m.table_name = :table_name and m.column_name = :geo_col_name "
              "UNION SELECT wktext from mdsys.user_sdo_geom_metadata m, mdsys.cs_srs c WHERE c.srid = m.srid and m.table_name = :table_name and m.column_name = :geo_col_name";

  if (layer->debug>=2)
    msDebug("msOracleSpatialLayerGetAutoProjection. Using this Sql to retrieve the projection: %s.\n", query_str);

  /*Prepare the handlers to the query*/
  success = TRY( hand, OCIStmtPrepare( sthand->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT ) )
            && TRY( hand, OCIBindByName( sthand->stmthp, &bnd2p, hand->errhp, (text *) ":table_name", strlen(":table_name"), (ub1*) table_name, strlen(table_name)+1, SQLT_STR, (dvoid *) 0, (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT ) )
            && TRY( hand, OCIBindByName( sthand->stmthp, &bnd1p, hand->errhp, (text *) ":geo_col_name", strlen(":geo_col_name"), (ub1*) geom_column_name, strlen(geom_column_name)+1, SQLT_STR, (dvoid *) 0, (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT ) )
            && TRY( hand, OCIDefineByPos( sthand->stmthp, &def1p, hand->errhp, (ub4)1, (dvoid *)wktext, (sb4)4000, SQLT_STR, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) )
            && TRY( hand, OCIStmtExecute( hand->svchp, sthand->stmthp, hand->errhp, (ub4)0, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ) )
            && TRY( hand, OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROWS_FETCHED, hand->errhp ) );

  if(!success) {
    msDebug( "Error: %s . "
                "Query statement: %s . "
                "Check your data statement."
                "in msOracleSpatialLayerGetAutoProjection()\n", hand->last_oci_error, query_str );
    msSetError( MS_ORACLESPATIALERR,
                "Error "
                "Check your data statement and server logs",
                "msOracleSpatialLayerGetAutoProjection()" );

    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    free(table_name);
    return MS_FAILURE;
  }
  do {
    success = TRY( hand, OCIStmtFetch( sthand->stmthp, hand->errhp, (ub4)1, (ub2)OCI_FETCH_NEXT, (ub4)OCI_DEFAULT ) )
              && TRY( hand, OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROWS_FETCHED, hand->errhp ) );
    if (success && sthand->rows_fetched > 0) {

      if( layer->debug )
        msDebug("Found WKT projection for table %s: %s\n", table_name, wktext);

      if(wktext != NULL && projection != NULL)
        if(msOGCWKT2ProjectionObj(wktext, projection, layer->debug) == MS_FAILURE)
          return(MS_FAILURE);
    }
  } while (sthand->rows_fetched > 0);

  if (geom_column_name) free(geom_column_name);
  if (srid) free(srid);
  if (unique) free(unique);
  if (indexfield) free(indexfield);
  free(table_name);
  return MS_SUCCESS;
}

/**********************************************************************
 *             msOracleSpatialGetFieldDefn()
 *
 * Pass the field definitions through to the layer metadata in the
 * "gml_[item]_{type,width,precision}" set of metadata items for
 * defining fields.
 **********************************************************************/
static void
msOracleSpatialGetFieldDefn( layerObj *layer,
                             msOracleSpatialHandler *hand,
                             const char *item,
                             OCIParam *pard )

{
  const char *gml_type = "Character";
  char gml_width[32], gml_precision[32];
  int success;
  ub2 rzttype, nOCILen;

  gml_width[0] = '\0';
  gml_precision[0] = '\0';

  /* -------------------------------------------------------------------- */
  /*      Get basic parameter details.                                    */
  /* -------------------------------------------------------------------- */
  success =
    TRY( hand, OCIAttrGet ((dvoid *) pard,(ub4) OCI_DTYPE_PARAM,
                           (dvoid*)&rzttype,(ub4 *)0,
                           (ub4) OCI_ATTR_DATA_TYPE, hand->errhp ))
    && TRY( hand, OCIAttrGet ((dvoid *) pard,(ub4) OCI_DTYPE_PARAM,
                              (dvoid*)&nOCILen ,(ub4 *)0,
                              (ub4) OCI_ATTR_DATA_SIZE, hand->errhp ));

  if( !success )
    return;

  switch( rzttype ) {
    case SQLT_CHR:
    case SQLT_AFC:
      gml_type = "Character";
      if( nOCILen <= 4000 )
        sprintf( gml_width, "%d", nOCILen );
      break;

    case SQLT_NUM: {
      /* NOTE: OCI docs say this should be ub1 type, but we have
         determined that oracle is actually returning a short so we
         use that type and try to compensate for possible problems by
         initializing, and dividing by 256 if it is large. */
      unsigned short byPrecision = 0;
      sb1 nScale = 0;

      if( !TRY( hand, OCIAttrGet ((dvoid *) pard,(ub4) OCI_DTYPE_PARAM,
                                  (dvoid*)&byPrecision ,(ub4 *)0,
                                  (ub4) OCI_ATTR_PRECISION,
                                  hand->errhp ))
          || !TRY( hand, OCIAttrGet ((dvoid *) pard,(ub4) OCI_DTYPE_PARAM,
                                     (dvoid*)&nScale,(ub4 *)0,
                                     (ub4) OCI_ATTR_SCALE,
                                     hand->errhp )) )
        return;
      if( byPrecision > 255 )
        byPrecision = byPrecision / 256;

      if( nScale > 0 ) {
        gml_type = "Real";
        sprintf( gml_width, "%d", byPrecision );
        sprintf( gml_precision, "%d", nScale );
      } else if( nScale < 0 )
        gml_type = "Real";
      else {
        gml_type = "Integer";
        if( byPrecision < 38 )
          sprintf( gml_width, "%d", byPrecision );
      }
    }
    break;

    case SQLT_DAT:
    case SQLT_DATE:
      gml_type = "Date";
      break;
    case SQLT_TIMESTAMP:
    case SQLT_TIMESTAMP_TZ:
    case SQLT_TIMESTAMP_LTZ:
      gml_type = "DateTime";
      break;
    case SQLT_TIME:
    case SQLT_TIME_TZ:
      gml_type = "Time";
      break;

    default:
      gml_type = "Character";
  }

    msUpdateGMLFieldMetadata(layer, item, gml_type, gml_width, gml_precision, 0);
}

int msOracleSpatialLayerGetItems( layerObj *layer )
{
  char *rzt = "";
  ub2 rzttype = 0;
  char *flk = "";
  int function = 0;
  int version = 0;
  int existgeom;
  int count_item, flk_len, success, i;
  char *table_name;
  char * query_str = NULL, *geom_column_name = NULL, *unique = NULL, *srid = NULL, *indexfield=NULL;
  OCIParam *pard = (OCIParam *) 0;

  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *) layer->layerinfo;
  msOracleSpatialHandler *hand = NULL;
  msOracleSpatialStatement *sthand = NULL;
  int get_field_details = 0;
  const char *value;

  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerGetItems was called.\n");

  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetItems called on unopened layer", "msOracleSpatialLayerGetItems()" );
    return MS_FAILURE;
  } else {
    hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    sthand = (msOracleSpatialStatement *) layerinfo->orastmt;
  }

  /* Will we want to capture the field details? */
  if((value = msOWSLookupMetadata(&(layer->metadata), "G", "types")) != NULL
      && strcasecmp(value,"auto") == 0 )
    get_field_details = 1;

  table_name = (char *) malloc(sizeof(char) * TABLE_NAME_SIZE);
  if (!msSplitData(layer->data, &geom_column_name, &table_name, &unique, &srid, &indexfield, &function, &version)) {
    msDebug(   "Error parsing OracleSpatial DATA variable. Must be: "
                "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
                "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE. "
                "Your data statement: (%s)"
                "in msOracleSpatialLayerGetItems()\n", layer->data );
    msSetError( MS_ORACLESPATIALERR,
                "Error parsing OracleSpatial DATA variable. Check server logs. ",
                "msOracleSpatialLayerGetItems()");

    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    free(table_name);
    return MS_FAILURE;
  }
  query_str = msStringConcatenate(query_str, "SELECT * FROM ");
  query_str = msStringConcatenate(query_str, table_name);

  success =  TRY( hand, OCIStmtPrepare( sthand->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DESCRIBE_ONLY) )
             && TRY( hand, OCIStmtExecute( hand->svchp, sthand->stmthp, hand->errhp, (ub4)QUERY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DESCRIBE_ONLY ) )
             && TRY( hand, OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layer->numitems, (ub4 *)0, OCI_ATTR_PARAM_COUNT, hand->errhp) );


  if (!success) {
    msSetError( MS_QUERYERR, "Cannot retrieve column list", "msOracleSpatialLayerGetItems()" );
    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    if (query_str) free(query_str);
    free(table_name);
    return MS_FAILURE;
  }

  sthand->row_num = sthand->row = 0;
  layer->numitems = layer->numitems-1;

  layer->items = malloc (sizeof(char *) * (layer->numitems));
  if (layer->items == NULL) {
    msSetError( MS_ORACLESPATIALERR,"Cannot allocate items", "msOracleSpatialLayerGetItems()" );
    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    if (query_str) free(query_str);
    free(table_name);
    return MS_FAILURE;
  }

  if (layer->numitems > 0) {
    if (sthand->items_query == NULL)
      sthand->items_query = (item_text_array_query *)malloc( sizeof(item_text_array_query) * (layer->numitems) );

    if (sthand->items_query == NULL) {
      msSetError( MS_ORACLESPATIALERR,"Cannot allocate items buffer", "msOracleSpatialLayerGetItems()" );
      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      if (query_str) free(query_str);
      free(table_name);
      return MS_FAILURE;
    }
  }

  count_item = 0;
  existgeom = 0;

  /*Upcase conversion for the geom_column_name*/
  for (i=0; geom_column_name[i] != '\0'; i++)
    geom_column_name[i] = toupper(geom_column_name[i]);

  /*Retrive columns name from the user table*/
  for (i = 0; i <= layer->numitems; i++) {

    success = TRY( hand, OCIParamGet ((dvoid*) sthand->stmthp, (ub4)OCI_HTYPE_STMT,hand->errhp,(dvoid*)&pard, (ub4)i+1))
              && TRY( hand, OCIAttrGet ((dvoid *) pard,(ub4) OCI_DTYPE_PARAM,(dvoid*)&rzttype,(ub4 *)0, (ub4) OCI_ATTR_DATA_TYPE, hand->errhp ))
              && TRY( hand, OCIParamGet ((dvoid*) sthand->stmthp, (ub4)OCI_HTYPE_STMT,hand->errhp,(dvoid*)&pard, (ub4)i+1))
              && TRY( hand, OCIAttrGet ((dvoid *) pard,(ub4) OCI_DTYPE_PARAM,(dvoid*)&rzt,(ub4 *)&flk_len, (ub4) OCI_ATTR_NAME, hand->errhp ));

     /*  if (layer->debug)
            msDebug("msOracleSpatialLayerGetItems checking type. Column = %s Type = %d\n", rzt, rzttype);  */
    flk = (char *)malloc(sizeof(char*) * flk_len+1);
    if (flk == NULL) {
      msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items", "msOracleSpatialLayerGetItems()" );
      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (query_str) free(query_str);
      free(table_name);
      return MS_FAILURE;
    } else {
      //layer->iteminfo->dtype[i]= 1;
      strlcpy(flk, rzt, flk_len+1);
    }

    /*Comapre the column name (flk) with geom_column_name and ignore with true*/
    if (strcmp(flk, geom_column_name) != 0) {
      if (rzttype!=OCI_TYPECODE_BLOB) {
        layer->items[count_item] = (char *)malloc(sizeof(char) * flk_len+1);
        if (layer->items[count_item] == NULL) {
          msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items buffer", "msOracleSpatialLayerGetItems()" );
          if (geom_column_name) free(geom_column_name);
          if (srid) free(srid);
          if (unique) free(unique);
          if (query_str) free(query_str);
          free(table_name);
          return MS_FAILURE;
        }

        strcpy(layer->items[count_item], flk);
        count_item++;

       if( get_field_details )
          msOracleSpatialGetFieldDefn( layer, hand,
                                       layer->items[count_item-1],
                                       pard );
      }
    } else
      existgeom = 1;

    strcpy( rzt, "" );
    free(flk); /* Better?!*/
    flk_len = 0;
  }

  layer->numitems = count_item;

  if (!(existgeom)) {
    msSetError (MS_ORACLESPATIALERR, "No geometry column, check stmt", "msOracleSpatialLayerGetItems()" );
    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    if (query_str) free(query_str);
    free(table_name);
    return MS_FAILURE;
  }

  if (geom_column_name) free(geom_column_name);
  if (srid) free(srid);
  if (unique) free(unique);
  if (indexfield) free(indexfield);
  if (query_str) free(query_str);
  free(table_name);
  return msOracleSpatialLayerInitItemInfo( layer );
}

int msOracleSpatialLayerGetExtent(layerObj *layer, rectObj *extent)
{
  char *table_name;
  char * query_str = NULL, *geom_column_name = NULL, *unique = NULL, *srid = NULL, *indexfield=NULL;
  int success, i;
  int function = 0;
  int version = 0;
  SDOGeometryObj *obj = NULL;
  SDOGeometryInd *ind = NULL;
  shapeObj shape;
  rectObj bounds;
  /*OCIDefine *adtp = NULL, *items[QUERY_SIZE] = { NULL };*/
  OCIDefine *adtp = NULL;
  OCIDefine **items = NULL;

  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;
  msOracleSpatialDataHandler *dthand = NULL;
  msOracleSpatialHandler *hand = NULL;
  msOracleSpatialStatement *sthand = NULL;

  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerGetExtent was called.\n");

  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetExtent called on unopened layer","msOracleSpatialLayerGetExtent()");
    return MS_FAILURE;
  } else {
    dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
    hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    sthand = (msOracleSpatialStatement *) layerinfo->orastmt;
  }

  /* allocate enough space for items */
  if (layer->numitems > 0) {
    sthand->items_query = (item_text_array_query *)malloc( sizeof(item_text_array_query) * (layer->numitems) );
    if (sthand->items_query == NULL) {
      msSetError( MS_ORACLESPATIALERR, "Cannot allocate layerinfo->items buffer", "msOracleSpatialLayerGetExtent()" );
      return MS_FAILURE;
    }
    items = (OCIDefine **)malloc(sizeof(OCIDefine *)*layer->numitems);
    if (items == NULL) {
      msSetError( MS_ORACLESPATIALERR,"Cannot allocate items buffer","msOracleSpatialLayerWhichShapes()" );
      return MS_FAILURE;
    }
    memset(items ,0,sizeof(OCIDefine *)*layer->numitems);
  }

  table_name = (char *) malloc(sizeof(char) * TABLE_NAME_SIZE);
  if (!msSplitData( layer->data, &geom_column_name, &table_name, &unique, &srid, &indexfield, &function, &version )) {
    msDebug(   "Error parsing OracleSpatial DATA variable. Must be: "
                "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
                "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE. "
                "Your data statement: (%s) "
                "in msOracleSpatialLayerGetExtent()\n", layer->data );
    msSetError( MS_ORACLESPATIALERR,
                "Error parsing OracleSpatial DATA variable. Check server logs. ",
                "msOracleSpatialLayerGetExtent()");
    /* clean items */
    free(items);

    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    free(table_name);

    return MS_FAILURE;
  }

  query_str = osAggrGetExtent(layer, query_str, geom_column_name, table_name);

  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerGetExtent. Using this Sql to retrieve the extent: %s.\n", query_str);

  /*Prepare the handlers to the query*/
  success = TRY( hand,OCIStmtPrepare( sthand->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) );

  if (success && layer->numitems > 0) {
    for( i = 0; i < layer->numitems && success; ++i )
      success = TRY( hand, OCIDefineByPos( sthand->stmthp, &items[i], hand->errhp, (ub4)i+1, (dvoid *)sthand->items_query[i], (sb4)TEXT_SIZE, SQLT_STR, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) );
  }

  if(!success) {
    msDebug(   "Error: %s . "
                "Query statement: %s . "
                "Check your data statement."
                "in msOracleSpatialLayerGetExtent()\n", hand->last_oci_error, query_str );

    msSetError( MS_ORACLESPATIALERR,
                "Check your data statement and server logs",
                "msOracleSpatialLayerGetExtent()" );

    /* clean items */
    free(items);

    if (query_str) free(query_str);

    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    free(table_name);

    return MS_FAILURE;
  }

  if (success) {
    success = TRY( hand, OCIDefineByPos( sthand->stmthp, &adtp, hand->errhp, (ub4)layer->numitems+1, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
              && TRY( hand, OCIDefineObject( adtp, hand->errhp, dthand->tdo, (dvoid **)sthand->obj, (ub4 *)0, (dvoid **)sthand->ind, (ub4 *)0 ) )
              && TRY (hand, OCIStmtExecute( hand->svchp, sthand->stmthp, hand->errhp, (ub4)QUERY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ))
              && TRY (hand, OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ));

  }

  if(!success) {
    msDebug(    "Error: %s . "
                "Query statement: %s ."
                "Check your data statement."
                "in msOracleSpatialLayerGetExtent()\n", hand->last_oci_error, query_str );
    msSetError( MS_ORACLESPATIALERR,
                "Check your data statement and server logs",
                "msOracleSpatialLayerGetExtent()" );

    /* clean items */
    free(items);
    if (query_str) free(query_str);
    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    free(table_name);

    return MS_FAILURE;
  }

  /* should begin processing first row */
  sthand->row_num = sthand->row = 0;
  msInitShape( &shape );
  do {
    /* is buffer empty? */
    if (sthand->row_num >= sthand->rows_fetched) {
      /* fetch more */
      success = TRY( hand, OCIStmtFetch( sthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub2)OCI_FETCH_NEXT, (ub4)OCI_DEFAULT ) )
                && TRY( hand, OCIAttrGet( (dvoid *)sthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&sthand->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ) );

      if (!success || sthand->rows_fetched == 0 || sthand->row_num >= sthand->rows_fetched) {
        hand->last_oci_status=MS_SUCCESS;
        break;
      }

      sthand->row = 0; /* reset row index */
    }

    /* no rows fetched */
    if (sthand->rows_fetched == 0)
      break;

    obj = sthand->obj[ sthand->row ];
    ind = sthand->ind[ sthand->row ];

    /* get the items for the shape */
    shape.numvalues = layer->numitems;
    shape.values = (char **) malloc(sizeof(char *) * layer->numitems);
    if (shape.values == NULL) {
      msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the values.", "msOracleSpatialLayerGetExtent()" );
      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      if (query_str) free(query_str);
      free(table_name);
      return MS_FAILURE;
    }

    shape.index = sthand->row_num;

    for( i = 0; i < layer->numitems; ++i ) {
      shape.values[i] = (char *)malloc(strlen((char *)sthand->items_query[sthand->row][i])+1);

      if (shape.values[i] == NULL) {
        msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items buffer.", "msOracleSpatialLayerGetExtent()" );

        /* clean items */
        free(items);

        if (geom_column_name) free(geom_column_name);
        if (srid) free(srid);
        if (unique) free(unique);
        if (indexfield) free(indexfield);
        if (query_str) free(query_str);
        free(table_name);

        return MS_FAILURE;
      } else {
        strcpy(shape.values[i], (char *)sthand->items_query[sthand->row][i]);
        shape.values[i][strlen((char *)sthand->items_query[sthand->row][i])] = '\0';
      }
    }

    /* increment for next row */
    sthand->row_num++;
    sthand->row++;

    /* fetch a layer->type object */
    success = osGetOrdinates(dthand, hand, &shape, obj, ind);
    if (success != MS_SUCCESS) {
      msSetError( MS_ORACLESPATIALERR, "Cannot execute query", "msOracleSpatialLayerGetExtent()" );

      /* clean items */
      free(items);

      if (geom_column_name) free(geom_column_name);
      if (srid) free(srid);
      if (unique) free(unique);
      if (indexfield) free(indexfield);
      if (query_str) free(query_str);
      free(table_name);

      return MS_FAILURE;
    }

  } while(sthand->row <= sthand->rows_fetched);

  sthand->row = sthand->row_num = 0;

  osShapeBounds(&shape);
  bounds = shape.bounds;

  extent->minx = bounds.minx;
  extent->miny = bounds.miny;
  extent->maxx = bounds.maxx;
  extent->maxy = bounds.maxy;

  msFreeShape(&shape);

  /* clean items */
  free(items);

  if (geom_column_name) free(geom_column_name);
  if (srid) free(srid);
  if (unique) free(unique);
  if (indexfield) free(indexfield);
  if (query_str) free(query_str);
  free(table_name);

  return(MS_SUCCESS);
}

void msOracleSpatialLayerFreeItemInfo( layerObj *layer )
{
  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerFreeItemInfo was called.\n");

  if (layer->iteminfo)
    free(layer->iteminfo);

  layer->iteminfo = NULL;
  /* nothing to do */
}

int msOracleSpatialLayerGetAutoStyle( mapObj *map, layerObj *layer, classObj *c, shapeObj *shape )
{
  (void)map;
  (void)layer;
  (void)c;
  (void)shape;
  msSetError( MS_ORACLESPATIALERR, "Function not implemented yet", "msLayerGetAutoStyle()" );
  return MS_FAILURE;
}

/************************************************************************/
/*                       msOracleSpatialEscapePropertyName              */
/*                                                                      */
/*      Return the property name in a properly escaped and quoted form. */
/************************************************************************/
char *msOracleSpatialEscapePropertyName(layerObj *layer, const char* pszString)
{
  char* pszEscapedStr=NULL;
  int i, j = 0;

  if (layer && pszString && strlen(pszString) > 0) {
    int nLength = strlen(pszString);

    pszEscapedStr = (char*) msSmallMalloc( 1 + 2 * nLength + 1 + 1);
    //pszEscapedStr[j++] = '';

    for (i=0; i<nLength; i++) {
      char c = pszString[i];
      if (c == '"') {
        pszEscapedStr[j++] = '"';
        pszEscapedStr[j++] ='"';
      } else if (c == '\\') {
        pszEscapedStr[j++] = '\\';
        pszEscapedStr[j++] = '\\';
      } else
        pszEscapedStr[j++] = c;
    }
    //pszEscapedStr[j++] = '';
    pszEscapedStr[j++] = 0;

  }
  return pszEscapedStr;
}

int msOracleSpatialGetPaging(layerObj *layer)
{
  msOracleSpatialLayerInfo *layerinfo = NULL;

  //if (layer->debug)
  //  msDebug("msOracleSpatialLayerGetPaging was called.\n");

  if(!msOracleSpatialLayerIsOpen(layer))
    return MS_TRUE;

  assert( layer->layerinfo != NULL);
  layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;

  return layerinfo->paging;
}

void msOracleSpatialEnablePaging(layerObj *layer, int value)
{
  msOracleSpatialLayerInfo *layerinfo = NULL;

  if (layer->debug>=3)
    msDebug("msOracleSpatialLayerEnablePaging was called.\n");

  if(!msOracleSpatialLayerIsOpen(layer))
  {
    if(msOracleSpatialLayerOpen(layer) != MS_SUCCESS)
    {
      return;
    }
  }

  assert( layer->layerinfo != NULL);
  layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;

  layerinfo->paging = value;

  return;
}

int msOracleSpatialLayerTranslateFilter(layerObj *layer, expressionObj *filter, char *filteritem)
{
  char *native_string = NULL;

  int nodeCount = 0;

  int function = 0, version = 0, dwithin = 0, regexp_like = 0, case_ins = 0, ieq = 0;
  char *table_name = NULL;
  char *geom_column_name = NULL, *unique = NULL, *srid = NULL, *indexfield=NULL;
  char *snippet = NULL;
  char *strtmpl = NULL;
  double dfDistance = -1;

  table_name = (char *) malloc(sizeof(char) * TABLE_NAME_SIZE);
  if (!msSplitData( layer->data, &geom_column_name, &table_name, &unique, &srid, &indexfield, &function, &version )) {
    msDebug(   "Error parsing OracleSpatial DATA variable. Must be: "
                "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
                "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE. "
                "Your data statement: (%s)"
                "in msOracleSpatialLayerGetExtent()\n", layer->data );
    msSetError( MS_ORACLESPATIALERR,
                "Error parsing OracleSpatial DATA variable. Check server logs. ",
                "msOracleSpatialLayerGetExtent()");
    /* clean items */

    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    if (table_name) free(table_name);

    return MS_FAILURE;
  }

  //msDebug("filter.string was set: %s\n",filter->string);
  if(!filter->string) {
    if (geom_column_name) free(geom_column_name);
    if (srid) free(srid);
    if (unique) free(unique);
    if (indexfield) free(indexfield);
    if (table_name) free(table_name);

    return MS_SUCCESS;
  }
  /* for backwards compatibility we continue to allow SQL snippets as a string */

 if(filter->type == MS_STRING && filter->string && filteritem) { /* item/value pair */
    if(filter->flags & MS_EXP_INSENSITIVE) {
      native_string = msStringConcatenate(native_string, "upper(");
      native_string = msStringConcatenate(native_string, filteritem);
      native_string = msStringConcatenate(native_string, ") = upper(");
    } else {
      native_string = msStringConcatenate(native_string, filteritem);
      native_string = msStringConcatenate(native_string, " = ");
    }

    strtmpl = "'%s'";  /* don't have a type for the righthand literal so assume it's a string and we quote */
    snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(filter->string));
    sprintf(snippet, strtmpl, filter->string);  // TODO: escape filter->string (msPostGISEscapeSQLParam)
    native_string = msStringConcatenate(native_string, snippet);
    free(snippet);

    if(filter->flags & MS_EXP_INSENSITIVE) native_string = msStringConcatenate(native_string, ")");

    if (layer->debug>=2) msDebug("msOracleSpatialLayerTranslateFilter: **item/value combo: %s\n", native_string);
  } else if(filter->type == MS_REGEX && filter->string && filteritem) { /* item/regex pair */
    native_string = msStringConcatenate(native_string, "REGEXP_LIKE (");
    native_string = msStrdup(filteritem);
    native_string = msStringConcatenate(native_string, ", ");
    strtmpl = "'%s'";
    snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(filter->string));
    sprintf(snippet, strtmpl, filter->string); // TODO: escape filter->string (msPostGISEscapeSQLParam)
    native_string = msStringConcatenate(native_string, snippet);

    if(filter->flags & MS_EXP_INSENSITIVE) {
      native_string = msStringConcatenate(native_string, ",i ");
    }
    native_string = msStringConcatenate(native_string, " )");
    free(snippet);
  } else if(filter->type == MS_EXPRESSION) {

    tokenListNodeObjPtr node = NULL;
    //tokenListNodeObjPtr nextNode = NULL;

    if (layer->debug>2)
      msDebug("msOracleSpatialLayerTranslateFilter. String: %s \n", filter->string);
    if (layer->debug>2 && !filter->tokens)
      msDebug("msOracleSpatialLayerTranslateFilter. No tokens to process\n");

    if(!filter->tokens) return MS_SUCCESS; /* nothing to work from */

    if (layer->debug>2)
      msDebug("msOracleSpatialLayerTranslateFilter. There are tokens to process \n");

    /* try to convert tokens */
    node = filter->tokens;
    while (node != NULL) {
      //msDebug("token count :%i, token is %i\n", nodeCount, node->token);
      switch(node->token) {
        case '(':
          // if (buffer == MS_FALSE)
           native_string = msStringConcatenate(native_string, "( ");
           break;
        case ')':
           if (regexp_like == MS_TRUE) {
            if (case_ins == MS_TRUE) {
              native_string = msStringConcatenate(native_string, ",'i' ");
              case_ins = MS_FALSE;
            }
           regexp_like = MS_FALSE;
           native_string = msStringConcatenate(native_string, " )");
           msDebug("closing RE comparison\n");
           }
           native_string = msStringConcatenate(native_string, " )");
           break;
        case MS_TOKEN_LITERAL_NUMBER:
        {
          char buffer[32];
          if (dwithin == MS_TRUE) {
            dfDistance = node->tokenval.dblval;
            if (layer->units == MS_DD){
              dfDistance *= msInchesPerUnit(MS_DD,0)/msInchesPerUnit(MS_METERS,0);
              //msDebug("Converted Distance value is %lf\n", dfDistance);
            }
            snprintf(buffer, sizeof(buffer), "%.18g", dfDistance);
            native_string = msStringConcatenate(native_string, "'distance=");
            native_string = msStringConcatenate(native_string, buffer);
            native_string = msStringConcatenate(native_string, "'");
          } else {
            snprintf(buffer, sizeof(buffer), "%.18g", node->tokenval.dblval);
            native_string = msStringConcatenate(native_string, buffer);
          }
          break;
        }
        case MS_TOKEN_LITERAL_STRING:
          strtmpl = "%s";
          snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(node->tokenval.strval));
          sprintf(snippet, strtmpl, node->tokenval.strval);  // TODO: escape strval
          snippet = msReplaceSubstring(snippet,"'","''");
          native_string = msStringConcatenate(native_string, "'");
          if (ieq == MS_TRUE) {
            native_string = msStringConcatenate(native_string, "^");
          }
          native_string = msStringConcatenate(native_string, snippet);
          if (ieq == MS_TRUE) {
            native_string = msStringConcatenate(native_string, "$");
            ieq = MS_FALSE;
          }
          native_string = msStringConcatenate(native_string, "'");
          msFree(snippet);
           break;
        case MS_TOKEN_LITERAL_BOOLEAN:
          if(node->tokenval.dblval == MS_TRUE)
            native_string = msStringConcatenate(native_string, "'TRUE'");
          else
             native_string = msStringConcatenate(native_string, "'FALSE'");
           break;
        case MS_TOKEN_LITERAL_TIME:
        {
          int resolution = msTimeGetResolution(node->tokensrc);
          snippet = (char *) msSmallMalloc(128);
          switch(resolution) {
            case TIME_RESOLUTION_YEAR:
              strtmpl = "to_date('%d-01','YYYY-MM')";
              sprintf(snippet, strtmpl, (node->tokenval.tmval.tm_year+1900));
              break;
            case TIME_RESOLUTION_MONTH:
              strtmpl = "to_date('%d-%02d','YYYY-MM')";
              sprintf(snippet, strtmpl, (node->tokenval.tmval.tm_year+1900), (node->tokenval.tmval.tm_mon+1));
              break;
            case TIME_RESOLUTION_DAY:
              strtmpl = "to_date('%d-%02d-%02d','YYYY-MM-DD') ";
              sprintf(snippet, strtmpl, (node->tokenval.tmval.tm_year+1900), (node->tokenval.tmval.tm_mon+1), node->tokenval.tmval.tm_mday);
              break;
            case TIME_RESOLUTION_HOUR:
              strtmpl = "to_timestamp('%d-%02d-%02d %02d','YYYY-MM-DD hh24')";
              sprintf(snippet, strtmpl, (node->tokenval.tmval.tm_year+1900), (node->tokenval.tmval.tm_mon+1), node->tokenval.tmval.tm_mday, node->tokenval.tmval.tm_hour);
              break;
            case TIME_RESOLUTION_MINUTE:
              strtmpl = "to_timestamp('%d-%02d-%02d %02d:%02d','YYYY-MM-DD hh24:mi')";
              sprintf(snippet, strtmpl, (node->tokenval.tmval.tm_year+1900), (node->tokenval.tmval.tm_mon+1), node->tokenval.tmval.tm_mday, node->tokenval.tmval.tm_hour, node->tokenval.tmval.tm_min);
              break;
            case TIME_RESOLUTION_SECOND:
              strtmpl = "to_timestamp('%d-%02d-%02d %02d:%02d:%02d','YYYY-MM-DD hh24:mi:ss')";
              sprintf(snippet, strtmpl, (node->tokenval.tmval.tm_year+1900), (node->tokenval.tmval.tm_mon+1), node->tokenval.tmval.tm_mday, node->tokenval.tmval.tm_hour, node->tokenval.tmval.tm_min, node->tokenval.tmval.tm_sec);
              break;
          }
          native_string = msStringConcatenate(native_string, snippet);
          msFree(snippet);
          break;
        }
        case MS_TOKEN_LITERAL_SHAPE:
          native_string = msStringConcatenate(native_string, " SDO_GEOMETRY('");
          native_string = msStringConcatenate(native_string, msShapeToWKT(node->tokenval.shpval));
          native_string = msStringConcatenate(native_string, "'");
          if(srid && strcmp(srid, "") != 0) {
            native_string = msStringConcatenate(native_string, ", ");
            native_string = msStringConcatenate(native_string, srid);
          }
          native_string = msStringConcatenate(native_string, ") ");
          //msDebug("------> srid node value: %s", node->tokenval.shpval);
          break;
        case MS_TOKEN_BINDING_DOUBLE:
          native_string = msStringConcatenate(native_string, node->tokenval.bindval.item);
          break;
        case MS_TOKEN_BINDING_INTEGER:
          native_string = msStringConcatenate(native_string, node->tokenval.bindval.item);
          break;
        case MS_TOKEN_BINDING_STRING:
         if (node->next->token == MS_TOKEN_COMPARISON_RE || node->next->token == MS_TOKEN_COMPARISON_IRE
          || node->next->token == MS_TOKEN_COMPARISON_IEQ ) {
              native_string = msStringConcatenate(native_string, "REGEXP_LIKE( ");
          }
          strtmpl = "%s";
          snippet = (char *) msSmallMalloc(strlen(strtmpl) + strlen(node->tokenval.strval));
          sprintf(snippet, strtmpl, node->tokenval.strval);  // TODO: escape strval (msPostGISEscapeSQLParam)
          native_string = msStringConcatenate(native_string, snippet);
          free(snippet);
          break;
        case MS_TOKEN_BINDING_TIME:
          native_string = msStringConcatenate(native_string, node->tokenval.bindval.item);
          break;
        case MS_TOKEN_BINDING_SHAPE:
          native_string = msStringConcatenate(native_string, geom_column_name);
          break;
        case MS_TOKEN_BINDING_MAP_CELLSIZE:
        {
          char buffer[32];
          snprintf(buffer, sizeof(buffer), "%.18g", layer->map->cellsize);
          native_string = msStringConcatenate(native_string, buffer);
          break;
        }
        case MS_TOKEN_LOGICAL_AND:
          native_string = msStringConcatenate(native_string, " AND ");
          break;
        case MS_TOKEN_LOGICAL_OR:
          native_string = msStringConcatenate(native_string, " OR ");
          break;
        case MS_TOKEN_LOGICAL_NOT:
          native_string = msStringConcatenate(native_string, " NOT ");
          break;
        case MS_TOKEN_COMPARISON_EQ:
          native_string = msStringConcatenate(native_string, " = ");
          break;
        case MS_TOKEN_COMPARISON_NE:
          native_string = msStringConcatenate(native_string, " != ");
          break;
        case MS_TOKEN_COMPARISON_GT:
          native_string = msStringConcatenate(native_string, " > ");
          break;
        case MS_TOKEN_COMPARISON_GE:
          native_string = msStringConcatenate(native_string, " >= ");
          break;
        case MS_TOKEN_COMPARISON_LT:
          native_string = msStringConcatenate(native_string, " < ");
          break;
        case MS_TOKEN_COMPARISON_LE:
          native_string = msStringConcatenate(native_string, " <= ");
          break;
        case MS_TOKEN_COMPARISON_IEQ:
          // this is for  case insensitve equals check
          msDebug("got a IEQ comparison\n");
          regexp_like = MS_TRUE;
          case_ins = MS_TRUE;
          ieq = MS_TRUE;
          native_string = msStringConcatenate(native_string, ", ");
          break;
        case MS_TOKEN_COMPARISON_RE:
          // the regex formats that MS uses as Oracle users are different so just return and eval the regex's in MS
          //return MS_SUCCESS;
          msDebug("got a RE comparison\n");
          regexp_like = MS_TRUE;
          native_string = msStringConcatenate(native_string, ", ");
          break;
        case MS_TOKEN_COMPARISON_IRE:
          // the regex formats that MS uses as Oracle users are different so just return and eval the regex's in MS
          regexp_like = MS_TRUE;
          case_ins = MS_TRUE;
          native_string = msStringConcatenate(native_string, ", ");
          break;
        case MS_TOKEN_COMPARISON_IN:
          native_string = msStringConcatenate(native_string, " IN ");
          break;
        case MS_TOKEN_COMPARISON_INTERSECTS:
          native_string = msStringConcatenate(native_string, " ST_INTERSECTS ");
          break;
        case MS_TOKEN_COMPARISON_DISJOINT:
          native_string = msStringConcatenate(native_string, " NOT ST_INTERSECTS ");
          break;
        case MS_TOKEN_COMPARISON_TOUCHES:
          native_string = msStringConcatenate(native_string, " ST_TOUCH ");
          break;
        case MS_TOKEN_COMPARISON_OVERLAPS:
          native_string = msStringConcatenate(native_string, " ST_OVERLAPS ");
          break;
        case MS_TOKEN_COMPARISON_CROSSES:
          native_string = msStringConcatenate(native_string, " SDO_OVERLAPBDYDISJOINT ");
          break;
        case MS_TOKEN_COMPARISON_WITHIN:
          native_string = msStringConcatenate(native_string, " ST_INSIDE ");
          break;
        case MS_TOKEN_COMPARISON_CONTAINS:
          native_string = msStringConcatenate(native_string, " SDO_CONTAINS ");
          break;
        case MS_TOKEN_COMPARISON_EQUALS:
          native_string = msStringConcatenate(native_string, " SDO_EQUAL ");
          break;
        case MS_TOKEN_COMPARISON_BEYOND:
          //support for the wfs case
          dwithin = MS_TRUE;
          native_string = msStringConcatenate(native_string, "NOT SDO_WITHIN_DISTANCE ");
          break;
        case MS_TOKEN_COMPARISON_DWITHIN:
          dwithin = MS_TRUE;
          native_string = msStringConcatenate(native_string, "SDO_WITHIN_DISTANCE ");
          break;
        case MS_TOKEN_FUNCTION_LENGTH:
          break;
        case MS_TOKEN_FUNCTION_TOSTRING:
          break;
        case MS_TOKEN_FUNCTION_COMMIFY:
          break;
        case MS_TOKEN_FUNCTION_AREA:
          native_string = msStringConcatenate(native_string, "SDO_GEOM.SDO_AREA ");
          break;
        case MS_TOKEN_FUNCTION_ROUND:
          native_string = msStringConcatenate(native_string, "ROUND ");
          break;
        case MS_TOKEN_FUNCTION_FROMTEXT:
          native_string = msStringConcatenate(native_string, "SDO_GEOMETRY ");
          break;
        case MS_TOKEN_FUNCTION_BUFFER:
           // native_string = msStringConcatenate(native_string, " SDO_BUFFER ");
           //buffer = MS_TRUE;
          break;
        case MS_TOKEN_FUNCTION_DIFFERENCE:
          native_string = msStringConcatenate(native_string, "ST_DIFFERENCE ");
          break;
        case MS_TOKEN_FUNCTION_SIMPLIFY:
          native_string = msStringConcatenate(native_string, "SDO_UTIL.SIMPLIFY ");
          break;
        case MS_TOKEN_FUNCTION_SIMPLIFYPT:
          break;
        case MS_TOKEN_FUNCTION_GENERALIZE:
          native_string = msStringConcatenate(native_string, "SDO_UTIL.SIMPLIFY ");
          break;
        case ',':
          native_string = msStringConcatenate(native_string, ",");
          break;
        case '~':
          break;
        default:
          fprintf(stderr, "Translation to native SQL failed.\n");
          msFree(native_string);
          if (layer->debug) {
            msDebug("Token not caught, exiting: Token is %i\n", node->token);
          }

          if (geom_column_name) free(geom_column_name);
          if (srid) free(srid);
          if (unique) free(unique);
          if (indexfield) free(indexfield);
          if (table_name) free(table_name);

          return MS_SUCCESS; /* not an error */
        }
      nodeCount++;
      node = node->next;

      //fprintf(stderr, "native filter: %s\n", native_string);
    }

    filter->native_string = msStrdup(native_string);
    if (layer->debug>=4)
      msDebug("total filter tokens are %i\n,", nodeCount);
    msFree(native_string);
  }

  if (geom_column_name) free(geom_column_name);
  if (srid) free(srid);
  if (unique) free(unique);
  if (indexfield) free(indexfield);
  if (table_name) free(table_name);
  
  return MS_SUCCESS;
}


#else
/* OracleSpatial "not-supported" procedures */

int msOracleSpatialLayerOpen(layerObj *layer)
{
  (void)layer;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerOpen()" );
  return MS_FAILURE;
}

int msOracleSpatialLayerIsOpen(layerObj *layer)
{
  (void)layer;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerIsOpen()" );
  return MS_FALSE;
}

int msOracleSpatialLayerClose(layerObj *layer)
{
  (void)layer;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerClose()" );
  return MS_FAILURE;
}

int msOracleSpatialLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery)
{
  (void)layer;
  (void)rect;
  (void)isQuery;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerWhichShapes()" );
  return MS_FAILURE;
}

int msOracleSpatialLayerNextShape(layerObj *layer, shapeObj *shape)
{
  (void)layer;
  (void)shape;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerNextShape()" );
  return MS_FAILURE;
}

int msOracleSpatialLayerGetItems(layerObj *layer)
{
  (void)layer;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerGetItems()" );
  return MS_FAILURE;
}

int msOracleSpatialLayerGetShape( layerObj *layer, shapeObj *shape, resultObj *record )
{
  (void)layer;
  (void)shape;
  (void)record;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerGetShape()" );
  return MS_FAILURE;
}

int msOracleSpatialLayerGetExtent(layerObj *layer, rectObj *extent)
{
  (void)layer;
  (void)extent;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerGetExtent()" );
  return MS_FAILURE;
}

int msOracleSpatialLayerInitItemInfo(layerObj *layer)
{
  (void)layer;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerInitItemInfo()" );
  return MS_FAILURE;
}

void msOracleSpatialLayerFreeItemInfo(layerObj *layer)
{
  (void)layer;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerFreeItemInfo()" );
}

int msOracleSpatialLayerGetAutoStyle( mapObj *map, layerObj *layer, classObj *c, shapeObj *shape )
{
  (void)map;
  (void)layer;
  (void)c;
  (void)shape;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msLayerGetAutoStyle()" );
  return MS_FAILURE;
}

void msOracleSpatialEnablePaging(layerObj *layer, int value)
{
  (void)layer;
  (void)value;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msLayerEnablePaging()" );
  return;
}

int msOracleSpatialGetPaging(layerObj *layer)
{
  (void)layer;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msLayerGetPaging()" );
  return MS_FAILURE;
}

int msOracleSpatialLayerTranslateFilter(layerObj *layer, expressionObj *filter, char *filteritem)
{
  (void)layer;
  (void)filter;
  (void)filteritem;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msLayerTranslateFilter()" );
  return MS_FAILURE;
}

char *msOracleSpatialEscapePropertyName(layerObj *layer, const char* pszString)
{
  (void)layer;
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msLayerEscapePropertyName()" );
  return msStrdup(pszString);
}
#endif

#if defined USE_ORACLE_PLUGIN
MS_DLL_EXPORT  int
PluginInitializeVirtualTable(layerVTableObj* vtable, layerObj *layer)
{
  assert(layer != NULL);
  assert(vtable != NULL);

  vtable->LayerTranslateFilter = msOracleSpatialLayerTranslateFilter;


  vtable->LayerInitItemInfo = msOracleSpatialLayerInitItemInfo;
  vtable->LayerFreeItemInfo = msOracleSpatialLayerFreeItemInfo;
  vtable->LayerOpen = msOracleSpatialLayerOpen;
  vtable->LayerIsOpen = msOracleSpatialLayerIsOpen;
  vtable->LayerWhichShapes = msOracleSpatialLayerWhichShapes;
  vtable->LayerNextShape = msOracleSpatialLayerNextShape;
  vtable->LayerGetShape = msOracleSpatialLayerGetShape;
  vtable->LayerClose = msOracleSpatialLayerClose;
  vtable->LayerGetItems = msOracleSpatialLayerGetItems;
  vtable->LayerGetExtent = msOracleSpatialLayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  vtable->LayerCloseConnection = msOracleSpatialLayerClose;
  vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;
  vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
  //vtable->LayerSetTimeFilter = msOracleSpatialLayerSetTimeFilter;
  //vtable->LayerEscapePropertyName = msOracleSpatialEscapePropertyName;
  /* layer->vtable->LayerGetNumFeatures, use default */
  /* layer->vtable->LayerGetAutoProjection = msOracleSpatialLayerGetAutoProjection; Disabled until tested */
  vtable->LayerEnablePaging = msOracleSpatialEnablePaging;
  vtable->LayerGetPaging = msOracleSpatialGetPaging;

  return MS_SUCCESS;
}

#else /*if ORACLE_PLUGIN is defined, then this file is not used by libmapserver
        and therefre there is no need to include this function */
int msOracleSpatialLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerTranslateFilter = msOracleSpatialLayerTranslateFilter;


  layer->vtable->LayerInitItemInfo = msOracleSpatialLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msOracleSpatialLayerFreeItemInfo;
  layer->vtable->LayerOpen = msOracleSpatialLayerOpen;
  layer->vtable->LayerIsOpen = msOracleSpatialLayerIsOpen;
  layer->vtable->LayerWhichShapes = msOracleSpatialLayerWhichShapes;
  layer->vtable->LayerNextShape = msOracleSpatialLayerNextShape;
  layer->vtable->LayerGetShape = msOracleSpatialLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */
  layer->vtable->LayerClose = msOracleSpatialLayerClose;
  layer->vtable->LayerGetItems = msOracleSpatialLayerGetItems;
  layer->vtable->LayerGetExtent = msOracleSpatialLayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  layer->vtable->LayerCloseConnection = msOracleSpatialLayerClose;
  layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;
  layer->vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
  //layer->vtable->LayerSetTimeFilter = msOracleSpatialLayerSetTimeFilter;
  //layer->vtable->LayerEscapePropertyName = msOracleSpatialEscapePropertyName;
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */
  /* layer->vtable->LayerGetAutoProjection = msOracleSpatialLayerGetAutoProjection; Disabled until tested */
  layer->vtable->LayerEnablePaging = msOracleSpatialEnablePaging;
  layer->vtable->LayerGetPaging = msOracleSpatialGetPaging;

  return MS_SUCCESS;
}
#endif

/*****************************************************************************
 *                   -- MapServer link to OracleSpatial --                   *
 *                                                                           *
 *  Author: Rodrigo Becke Cabral (cabral@cttmar.univali.br)                  *
 *  Collaborator: Adriana Gomes Alves                                        *
 *  MapServer: MapServer 3.5 (nightly)                                       *
 *  Oracle: Oracle Enterprise 8.1.7 (linux)                                  *
 *                                                                           *
 *****************************************************************************
 = This piece for MapServer has been developed under the funding of
 = agreement n.45/00 between CTTMAR/UNIVALI (www.cttmar.univali.br)
 = and CEPSUL/IBAMA (www.ibama.gov.br). This is a typical open source
 = initiative. Feel free to use it.
 *****************************************************************************
 *
 * Revision 1.0   2001/09/28 10:42:29 (GMT-03:00)
 * Added OracleSpatial "partial" support. Displaying only the following
 * 2D geometries (gtype/etype/interpretation):
 * - 2001/NULL/NULL       point
 * - 2003/[?00]3/1        simple polygon
 * - 2003/[?00]3/3        rectangle
 * Note: Items should be soon supported. Don't use classitem or other *item
 * statements in mapfiles.
 *
 *****************************************************************************
 *
 * Using OracleSpatial:
 * - CONNECTIONTYPE oraclespatial
 * - CONNECTION 'username/password@database'
 * - DATA 'geometry_column FROM table_name'
 *   or
 *   DATA 'geometry_column FROM (SELECT stmt)'
 *
 *****************************************************************************
 * Notices above shall be included in all copies or portions of the software.
 * This piece is provided "AS IS", without warranties of any kind. Got it?
 *****************************************************************************/

#include "map.h"

#ifdef USE_ORACLESPATIAL

#include <oci.h>
#include <ctype.h>

#define ARRAY_SIZE                 64
#define TYPE_OWNER                 "MDSYS"
#define SDO_GEOMETRY               TYPE_OWNER".SDO_GEOMETRY"
#define SDO_GEOMETRY_LEN           strlen( SDO_GEOMETRY )

typedef
  struct {
    OCINumber x;
    OCINumber y;
    OCINumber z;
  }
  SDOPointObj;

typedef
  struct {
    OCINumber gtype;
    OCINumber srid;
    SDOPointObj point;
    OCIArray *elem_info;
    OCIArray *ordinates;
  }
  SDOGeometryObj;

typedef
  struct {
    OCIInd _atomic;
    OCIInd x;
    OCIInd y;
    OCIInd z;
  }
  SDOPointInd;

typedef
  struct {
    OCIInd _atomic;
    OCIInd gtype;
    OCIInd srid;
    SDOPointInd point;
    OCIInd elem_info;
    OCIInd ordinates;
  }
  SDOGeometryInd;

typedef struct {
  /* oracle handles */
  OCIEnv         *envhp;
  OCIError       *errhp;
  OCIServer      *srvhp;
  OCISvcCtx      *svchp;
  OCISession      *usrhp;
  /* query data */
  OCIStmt        *stmthp;
  OCIDescribe    *dschp;
  OCIType        *tdo;
  /* fetch buffer */
  int            rows_fetched;
  int            row_num;
  int            row;
  SDOGeometryObj *obj[ARRAY_SIZE]; /* spatial object buffer */
  SDOGeometryInd *ind[ARRAY_SIZE]; /* object indicator */
}
msOracleSpatialLayerInfo;

/* local prototypes */
static int TRY( msOracleSpatialLayerInfo *layerinfo, sword status );
static int ERROR( char *routine, msOracleSpatialLayerInfo *layerinfo );
static void msSplitLogin( char *connection, char *username, char *password, char *dblink );
static msOracleSpatialLayerInfo *msOCIConnect( char *username, char *password, char *dblink );
static void msOCIDisconnect( msOracleSpatialLayerInfo *layerinfo );
static int msOCIGetOrdinates( msOracleSpatialLayerInfo *layerinfo, SDOGeometryObj *obj, int s, int e, pointObj *pt );

/* local status */
static int last_oci_call_ms_status = MS_FAILURE;
static text last_oci_call_ms_error[2048] = "";

/* if an error ocurred call msSetError, sets last_oci_call_ms_status to MS_FAILURE and return 0;
 * otherwise returns 1 */
static int TRY( msOracleSpatialLayerInfo *layerinfo, sword status )
{
  sb4 errcode = 0;

  if (last_oci_call_ms_status == MS_FAILURE)
    return 0; /* error from previous call */

  switch (status) {
    case OCI_ERROR:
      OCIErrorGet(
        (dvoid *)layerinfo->errhp, (ub4)1, (text *)NULL, &errcode,
        last_oci_call_ms_error, (ub4)sizeof(last_oci_call_ms_error), OCI_HTYPE_ERROR );
      last_oci_call_ms_error[sizeof(last_oci_call_ms_error)-1] = 0; /* terminate string!? */
    break;
    case OCI_NEED_DATA: strcpy( last_oci_call_ms_error, "OCI_NEED_DATA" ); break;
    case OCI_INVALID_HANDLE: strcpy( last_oci_call_ms_error, "OCI_INVALID_HANDLE" ); break;
    case OCI_STILL_EXECUTING: strcpy( last_oci_call_ms_error, "OCI_STILL_EXECUTING"); break;
    case OCI_CONTINUE: strcpy( last_oci_call_ms_error, "OCI_CONTINUE" ); break;
    default:
      return 1; // no error
  }

  /* if I got here, there was an error */
  last_oci_call_ms_status = MS_FAILURE;

  return 0; /* error! */
}

/* check last_oci_call_ms_status for MS_FAILURE (set by TRY()) if an error ocurred, disconnect and return 1;
 * otherwise, returns 0 */
static int ERROR( char *routine, msOracleSpatialLayerInfo *layerinfo )
{
  if (last_oci_call_ms_status == MS_FAILURE) {
    /* there was an error */
    msSetError( MS_ORACLESPATIALERR, last_oci_call_ms_error, routine );
    msOCIDisconnect( layerinfo );
    return 1; /* error processed */
  }
  else
    return 0; /* no error */
}

/* break layer->connection (username/password@dblink) into username, password and dblink */
static void msSplitLogin( char *connection, char *username, char *password, char *dblink )
{
  char *src, *tgt;
  // clearup
  *username = *password = *dblink = 0;
  // bad 'connection'
  if (connection == NULL) return;
  // ok, split connection
  for( tgt=username, src=connection; *src; src++, tgt++ )
    if (*src=='/' || *src=='@') break;
    else *tgt = *src;
  *tgt = 0;
  if (*src == '/') {
    for( tgt=password, ++src; *src; src++, tgt++ )
      if (*src == '@') break;
      else *tgt = *src;
    *tgt = 0;
  }
  if (*src == '@')
    strcpy( dblink, ++src );
}

/* break layer->data into geometry_column_name and table_name (geometry_column from table_name) */
static int msSplitData( char *data, char *geometry_column_name, char *table_name )
{
  char *from = "from";
  char *src, *tgt;
  // clearup
  *geometry_column_name = *table_name = 0;
  // bad 'data'
  if (data == NULL) return 0;
  // ok, split data
  for( tgt=geometry_column_name, src=data; *src; src++, tgt++ )
    if (isspace( *src )) break;
    else *tgt = *src;
  *tgt = 0;
  for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
  for( ;*src && *from && tolower(*src)==*from; src++, from++ ) ; /* parse 'from' */
  if (!*from) {
    for( ;*src && isspace( *src ); src++ ); /* skip blanks */
    strcpy( table_name, src ); /* copy whatever it is after from */
    return *src!=0; /* src should point to non-empty string */
  }
  return 0;
}

/* connect to database */
static msOracleSpatialLayerInfo *msOCIConnect( char *username, char *password, char *dblink )
{
  int success;
  OCIParam *paramp = NULL;
  OCIRef *type_ref = NULL;

  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)malloc( sizeof(msOracleSpatialLayerInfo) );
  memset( layerinfo, 0, sizeof(msOracleSpatialLayerInfo) );

  success = TRY( layerinfo,
    /* allocate envhp */
    OCIEnvCreate( &layerinfo->envhp, OCI_OBJECT, (dvoid *)0, 0, 0, 0, (size_t) 0, (dvoid **)0 ) )
  && TRY( layerinfo,
    /* allocate srvhp */
    OCIHandleAlloc( (dvoid *)layerinfo->envhp, (dvoid **)&layerinfo->srvhp, (ub4)OCI_HTYPE_SERVER, (size_t)0, (dvoid **)0 ) )
  && TRY( layerinfo,
    /* allocate errhp */
    OCIHandleAlloc( (dvoid *)layerinfo->envhp, (dvoid **)&layerinfo->errhp, (ub4)OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0 ) )
  && TRY( layerinfo,
    /* attach srvhp to dblink (database) */
     OCIServerAttach( layerinfo->srvhp, layerinfo->errhp, (text *)dblink, strlen(dblink), (ub4)OCI_DEFAULT ) )
  && TRY( layerinfo,
    /* allocate svchp */
    OCIHandleAlloc( (dvoid *)layerinfo->envhp, (dvoid **)&layerinfo->svchp, (ub4)OCI_HTYPE_SVCCTX, (size_t)0, (dvoid **)0) )
  && TRY( layerinfo,
    /* set server (srvhp) for service context (svchp) */
    OCIAttrSet( (dvoid *)layerinfo->svchp, (ub4)OCI_HTYPE_SVCCTX, (dvoid *)layerinfo->srvhp, (ub4)0, (ub4)OCI_ATTR_SERVER, layerinfo->errhp ) )
  && TRY( layerinfo,
    /* allocate usrhp */
    OCIHandleAlloc( (dvoid *)layerinfo->envhp, (dvoid **)&layerinfo->usrhp, (ub4)OCI_HTYPE_SESSION, (size_t)0, (dvoid **)0) )
  && TRY( layerinfo,
    /* define username in usrhp */
    OCIAttrSet( (dvoid *)layerinfo->usrhp, (ub4)OCI_HTYPE_SESSION, (dvoid *)username, (ub4)strlen(username), (ub4)OCI_ATTR_USERNAME, layerinfo->errhp ) )
  && TRY( layerinfo,
    /* define password in usrhp */
    OCIAttrSet( (dvoid *)layerinfo->usrhp, (ub4)OCI_HTYPE_SESSION, (dvoid *)password, (ub4)strlen(password), (ub4)OCI_ATTR_PASSWORD, layerinfo->errhp ) )
  && TRY( layerinfo,
    /* begin session */
    OCISessionBegin( layerinfo->svchp, layerinfo->errhp, layerinfo->usrhp, OCI_CRED_RDBMS, OCI_DEFAULT ) )
  && TRY( layerinfo,
    /* set user (usrhp) for service context (svchp) */
    OCIAttrSet( (dvoid *)layerinfo->svchp, (ub4)OCI_HTYPE_SVCCTX, (dvoid *)layerinfo->usrhp, (ub4)0, (ub4)OCI_ATTR_SESSION, layerinfo->errhp ) )
  && TRY( layerinfo,
    /* allocate stmthp */
    OCIHandleAlloc( (dvoid *)layerinfo->envhp, (dvoid **)&layerinfo->stmthp, (ub4)OCI_HTYPE_STMT, (size_t)0, (dvoid **)0 ) )
  && TRY( layerinfo,
    /* allocate dschp */
    OCIHandleAlloc( layerinfo->envhp, (dvoid **)&layerinfo->dschp, (ub4)OCI_HTYPE_DESCRIBE, (size_t)0, (dvoid **)0 ) )
  && TRY( layerinfo,
    /* describe SDO_GEOMETRY in svchp (dschp) */
    OCIDescribeAny( layerinfo->svchp, layerinfo->errhp, (text *)SDO_GEOMETRY, (ub4)SDO_GEOMETRY_LEN, OCI_OTYPE_NAME, (ub1)1, (ub1)OCI_PTYPE_TYPE, layerinfo->dschp ) )
  && TRY( layerinfo,
    /* get param for SDO_GEOMETRY */
    OCIAttrGet( (dvoid *)layerinfo->dschp, (ub4)OCI_HTYPE_DESCRIBE, (dvoid *)&paramp, (ub4 *)0,  (ub4)OCI_ATTR_PARAM, layerinfo->errhp ) )
  && TRY( layerinfo,
    /* get type_ref for SDO_GEOMETRY */
    OCIAttrGet( (dvoid *)paramp, (ub4)OCI_DTYPE_PARAM, (dvoid *)&type_ref, (ub4 *)0, (ub4)OCI_ATTR_REF_TDO, layerinfo->errhp ) )
  && TRY( layerinfo,
    /* get TDO for SDO_GEOMETRY */
    OCIObjectPin( layerinfo->envhp, layerinfo->errhp, type_ref, (OCIComplexObject *)0, OCI_PIN_ANY, OCI_DURATION_SESSION, OCI_LOCK_NONE, (dvoid **)&layerinfo->tdo ) );

  if (ERROR( "msOCIConnect() in msOracleSpatialLayerOpen()", layerinfo )) {
    free( layerinfo );
    return NULL;
  }

  return layerinfo;
}

/* disconnect from database */
static void msOCIDisconnect( msOracleSpatialLayerInfo *layerinfo )
{
  if (layerinfo->dschp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->dschp, (ub4)OCI_HTYPE_DESCRIBE );
  if (layerinfo->stmthp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT );
  if (layerinfo->svchp != NULL && layerinfo->errhp != NULL && layerinfo->usrhp != NULL)
    OCISessionEnd( layerinfo->svchp, layerinfo->errhp, layerinfo->usrhp, (ub4)OCI_DEFAULT );
  if (layerinfo->usrhp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->usrhp, (ub4)OCI_HTYPE_SESSION );
  if (layerinfo->srvhp != NULL && layerinfo->errhp != NULL)  
    OCIServerDetach( layerinfo->srvhp, layerinfo->errhp, (ub4)OCI_DEFAULT );
  if (layerinfo->svchp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->svchp, (ub4)OCI_HTYPE_SVCCTX );
  if (layerinfo->srvhp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->srvhp, (ub4)OCI_HTYPE_SERVER );
  if (layerinfo->errhp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->errhp, (ub4)OCI_HTYPE_ERROR );
  if (layerinfo->envhp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->envhp, (ub4)OCI_HTYPE_ENV );
  /* zero layerinfo out. can't free it because it's not the actual layer->oraclespatiallayerinfo pointer
   * but a copy of it. the only function that frees layer->oraclespatiallayerinfo is msOracleSpatialLayerClose */
  memset( layerinfo, 0, sizeof( msOracleSpatialLayerInfo ) ); 
}

static int msOCIGetOrdinates( msOracleSpatialLayerInfo *layerinfo, SDOGeometryObj *obj, int s, int e, pointObj *pt )
{
  double x, y;
  int i, n, success = 1;
  boolean exists;
  OCINumber *oci_number;

  for( i=s, n=0; i<e && success; i+=2, n++ ) {
    success = TRY( layerinfo,
        OCICollGetElem( layerinfo->envhp, layerinfo->errhp, (OCIColl *)obj->ordinates,
          (sb4)i, (boolean *)&exists, (dvoid **)&oci_number, (dvoid **)0 ) )
      && TRY( layerinfo,
        OCINumberToReal( layerinfo->errhp, oci_number, (uword)sizeof(double), (dvoid *)&x ) )
      && TRY( layerinfo,
        OCICollGetElem( layerinfo->envhp, layerinfo->errhp, (OCIColl *)obj->ordinates,
          (sb4)i+1, (boolean *)&exists, (dvoid **)&oci_number, (dvoid **)0 ) )
      && TRY( layerinfo,
        OCINumberToReal( layerinfo->errhp, oci_number, (uword)sizeof(double), (dvoid *)&y ) );
    if (success) {
      pt[n].x = x;
      pt[n].y = y;
    }    
  }

  return success ? n : 0;
}

/* opens a layer by connecting to db with username/password@database stored in layer->connection */
int msOracleSpatialLayerOpen( layerObj *layer )
{
  char username[1024], password[1024], dblink[1024];

  if (layer->oraclespatiallayerinfo != NULL)
    return MS_SUCCESS;

  if (layer->data == NULL) {
    msSetError( MS_ORACLESPATIALERR, 
      "Missing DATA clause in OracleSpatial layer definition. DATA statement must contain 'geometry_column from table_name|(SELECT stmt)'.", 
      "msOracleSpatialLayerOpen()" );
    return MS_FAILURE;
  }

  last_oci_call_ms_status = MS_SUCCESS;
  msSplitLogin( layer->connection, username, password, dblink );
  layer->oraclespatiallayerinfo = msOCIConnect( username, password, dblink );

  return layer->oraclespatiallayerinfo != NULL ? MS_SUCCESS : MS_FAILURE;
}

/* closes the layer, disconnecting from db if connected. layer->oraclespatiallayerinfo is freed */
int msOracleSpatialLayerClose( layerObj *layer )
{ 
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->oraclespatiallayerinfo;
  if (layerinfo != NULL) {
    msOCIDisconnect( layerinfo );
    free( layerinfo );
    layer->oraclespatiallayerinfo = NULL;
  }
  return MS_SUCCESS;
}

/* create SQL statement for retrieving shapes */
int msOracleSpatialLayerWhichShapes( layerObj *layer, rectObj rect )
{
  int success, apply_window;
  char query_str[6000];
  char table_name[2000], geom_column_name[100];
  OCIDefine *adtp = NULL;

  /* get layerinfo */
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->oraclespatiallayerinfo;
  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR,
      "msOracleSpatialLayerWhichShapes called on unopened layer",
      "msOracleSpatialLayerWhichShapes()" );
    return MS_FAILURE;
  }

  /* parse geom_column_name and table_name */
  if (!msSplitData( layer->data, geom_column_name, table_name )) {
    msSetError( MS_ORACLESPATIALERR,
      "Error parsing OracleSpatial DATA variable. Must contain 'geometry_column from table_name|(SELECT stmt)'.", 
      "msOracleSpatialLayerWhichShapes()" );
    return MS_FAILURE;
  }

  /* define SQL query */
  apply_window = (table_name[0] != '('); /* table isn´t a SELECT statement */
  sprintf( query_str, "SELECT %s FROM %s", geom_column_name, table_name );
  if (apply_window)
    strcat( query_str, " WHERE " );
  if (layer->filter.string != NULL) {
    strcat( query_str, layer->filter.string );
    if (apply_window) strcat( query_str, " AND " );
  }
  if (apply_window)
    sprintf( query_str + strlen(query_str),
        "SDO_FILTER( %s, MDSYS.SDO_GEOMETRY("
          "2003, NULL, NULL,"
          "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
          "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g) ),"
        "'querytype=window') = 'TRUE'",
        geom_column_name, rect.minx, rect.miny, rect.maxx, rect.maxy );
      
  /* parse and execute SQL query */
  success = TRY( layerinfo,
    /* prepare */
    OCIStmtPrepare( layerinfo->stmthp, layerinfo->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) )
  && TRY( layerinfo,
    /* define spatial position adtp ADT object */
    OCIDefineByPos( layerinfo->stmthp, &adtp, layerinfo->errhp, (ub4)1, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
  && TRY( layerinfo,
    /* define object tdo from adtp */
    OCIDefineObject( adtp, layerinfo->errhp, layerinfo->tdo, (dvoid **)layerinfo->obj, (ub4 *)0, (dvoid **)layerinfo->ind, (ub4 *)0 ) )
  && TRY( layerinfo,
    /* execute */
    OCIStmtExecute( layerinfo->svchp, layerinfo->stmthp, layerinfo->errhp, (ub4)ARRAY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ) )
  &&  TRY( layerinfo,
    /* get rows fetched */
    OCIAttrGet( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, layerinfo->errhp ) );

  if (!success)
    sprintf( last_oci_call_ms_error + strlen(last_oci_call_ms_error), ". SQL statement: %s", query_str );
  
  if (ERROR( "msOracleSpatialLayerWhichShapes()", layerinfo ))
    return MS_FAILURE;

  /* should begin processing first row */
  layerinfo->row_num = layerinfo->row = 0; 

  return MS_SUCCESS;
}

/* fetch next shape from previous SELECT stmt (see *WhichShape()) */
int msOracleSpatialLayerNextShape( layerObj *layer, shapeObj *shape )
{
  SDOGeometryObj *obj;
  SDOGeometryInd *ind;
  int gtype, elem_type;
  ub4 etype;
  ub4 interpretation;
  int nelems, nords;
  int elem, ord_start, ord_end;
  boolean exists;
  OCINumber *oci_number;
  double x, y;
  int success, n;
  lineObj points;
  pointObj point5[5]; /* for quick access */

  /* get layerinfo */
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->oraclespatiallayerinfo;
  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR,
      "msOracleSpatialLayerWhichShapes called on unopened layer",
      "msOracleSpatialLayerWhichShapes()" );
    return MS_FAILURE;
  }

  /* at first, don't know what it is */
  msInitShape( shape );
//  shape->type = MS_SHAPE_NULL;

  /* no rows fetched */
  if (layerinfo->rows_fetched == 0)
    return MS_DONE;

  /* fetch a layer->type object */
  do {

    /* is buffer empty? */
    if (layerinfo->row_num >= layerinfo->rows_fetched) {
      /* fetch more */
      success = TRY( layerinfo,
        OCIStmtFetch( layerinfo->stmthp, layerinfo->errhp, (ub4)ARRAY_SIZE, (ub2)OCI_FETCH_NEXT, (ub4)OCI_DEFAULT ) )
      && TRY( layerinfo,
        OCIAttrGet( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, layerinfo->errhp ) );
      if (!success || layerinfo->rows_fetched == 0)
        return MS_DONE;
      layerinfo->row = 0; /* reset row index */
    }

    obj = layerinfo->obj[ layerinfo->row ];
    ind = layerinfo->ind[ layerinfo->row ];
    layerinfo->row_num++;
    layerinfo->row++;

    if (ind->_atomic != OCI_IND_NULL) { /* not a null object */
      nelems = nords = 0;
      success = TRY( layerinfo,
        OCICollSize( layerinfo->envhp, layerinfo->errhp, (OCIColl *)obj->elem_info, &nelems ) )
      && TRY( layerinfo,
        OCICollSize( layerinfo->envhp, layerinfo->errhp, (OCIColl *)obj->ordinates, &nords ) )
      && TRY( layerinfo,
        OCINumberToInt( layerinfo->errhp, &(obj->gtype), (uword)sizeof(int), OCI_NUMBER_SIGNED, (dvoid *)&gtype ) )
      && (nords%2==0 && nelems%3==0); /* check %2==0 for 2D geometries; and %3==0 for element info triplets */
      if (success
          && (gtype==2001 /* 2D point */
          ||  gtype==2002 /* 2D linestring */ 
          ||  gtype==2003 /* 2D polygon */
          ||  gtype==2005 /* 2D multipoint */
          ||  gtype==2006 /* 2D multilinestring */
          ||  gtype==2007 /* 2D multipolygon */ )) {
        /* reading SDO_POINT from SDO_GEOMETRY for a 2D point geometry */
        if (gtype==2001
              && ind->point._atomic == OCI_IND_NOTNULL
              && ind->point.x == OCI_IND_NOTNULL
              && ind->point.y == OCI_IND_NOTNULL) {
            success = TRY( layerinfo,
              OCINumberToReal( layerinfo->errhp, &(obj->point.x), (uword)sizeof(double), (dvoid *)&x ) )
            && TRY( layerinfo,
              OCINumberToReal( layerinfo->errhp, &(obj->point.y), (uword)sizeof(double), (dvoid *)&y ) );
            if (ERROR( "msOracleSpatialLayerNextShape()", layerinfo ))
              return MS_FAILURE;
            shape->type = MS_SHAPE_POINT;
            points.numpoints = 1;
            points.point = point5;
            point5[0].x = x;
            point5[0].y = y;
            msAddLine( shape, &points );
            continue; // jump to end-of-while below
        }
        /* if SDO_POINT not fetched, proceed reading elements (element info/ordinates) */
        success = TRY( layerinfo,
            OCICollGetElem( layerinfo->envhp, layerinfo->errhp, (OCIColl *)obj->elem_info,
              (sb4)0, (boolean *)&exists, (dvoid **)&oci_number, (dvoid **)0 ) )
          && TRY( layerinfo,
            OCINumberToInt( layerinfo->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_SIGNED, (dvoid *)&ord_end ) );
        elem = 0;
        ord_end--; /* shifts offset from 1..n to 0..n-1 */
        do {
          ord_start = ord_end;
          if (elem+3 >= nelems) {
            /* processing last element */
            ord_end = nords;
            success = 1;
          }
          else {
            /* get start ordinate position for next element which is ord_end for this element */
            success = TRY( layerinfo, 
                OCICollGetElem( layerinfo->envhp, layerinfo->errhp, (OCIColl *)obj->elem_info,
                  (sb4)elem+3, (boolean *)&exists, (dvoid **)&oci_number, (dvoid **)0 ) )
              && TRY( layerinfo,
                OCINumberToInt( layerinfo->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_SIGNED, (dvoid *)&ord_end ) );
            ord_end--; /* shifts offset from 1..n to 0..n-1 */
          }
          success = success
            && TRY( layerinfo,
              OCICollGetElem( layerinfo->envhp, layerinfo->errhp, (OCIColl *)obj->elem_info,
                (sb4)elem+1, (boolean *)&exists, (dvoid **)&oci_number, (dvoid **)0) )
            && TRY( layerinfo,
              OCINumberToInt( layerinfo->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_UNSIGNED, (dvoid *)&etype ) )
            && TRY( layerinfo,
              OCICollGetElem( layerinfo->envhp, layerinfo->errhp, (OCIColl *)obj->elem_info, 
                (sb4)elem+2, (boolean *)&exists, (dvoid **)&oci_number, (dvoid **)0 ) )
            && TRY( layerinfo, 
              OCINumberToInt( layerinfo->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_UNSIGNED, (dvoid *)&interpretation ) );
          if (ERROR( "msOracleSpatialLayerNextShape()", layerinfo ))
            return MS_FAILURE;
          elem_type = (etype==1 && interpretation>1) ? 10 : ((etype%10)*10 + interpretation);
          switch (elem_type) {
            case 10: /* point cluster with 'interpretation'-points */
              n = (ord_end - ord_start)/2;
              if (n == interpretation) {
                points.point = (pointObj *)malloc( sizeof(pointObj)*n );
                n = msOCIGetOrdinates( layerinfo, obj, ord_start, ord_end, points.point );
                if (n == interpretation && n>0) {
                  shape->type = MS_SHAPE_POINT;
                  points.numpoints = n;
                  msAddLine( shape, &points );
                }
                free( points.point );
              }
              break;
            case 11: /* point type */
              n = msOCIGetOrdinates( layerinfo, obj, ord_start, ord_end, point5 ); /* n must be < 5 */
              if (n == 1) {
                shape->type = MS_SHAPE_POINT;
                points.numpoints = 1;
                points.point = point5;
                msAddLine( shape, &points );
              }
              break;
            case 21: /* line string whose vertices are connected by straight line segments */
            case 31: /* simple polygon with n points, last point equals the first one */
              n = (ord_end - ord_start)/2;
              points.point = (pointObj *)malloc( sizeof(pointObj)*n );
              n = msOCIGetOrdinates( layerinfo, obj, ord_start, ord_end, points.point );
              if (n > 0) {
                shape->type = (elem_type==21) ? MS_SHAPE_LINE : MS_SHAPE_POLYGON;
                points.numpoints = n;
                msAddLine( shape, &points );
              }
              free( points.point );
              break;
            case 33: /* rectangle defined by 2 points */
              n = msOCIGetOrdinates( layerinfo, obj, ord_start, ord_end, point5 ); /* n must be < 5 */
              if (n == 2) {
                shape->type = MS_SHAPE_POLYGON;
                points.numpoints = 5;
                points.point = point5;
                /* point5 [0] & [1] contains the lower-left and upper-right points of the rectangle */
                point5[2] = point5[1];
                point5[1].x = point5[0].x;
                point5[3].x = point5[2].x;
                point5[3].y = point5[0].y;
                point5[4] = point5[0]; 
                msAddLine( shape, &points );
              }
              break;
          }
          if (ERROR( "msOracleSpatialLayerNextShape()", layerinfo )) 
            return MS_FAILURE;
          /* prepare for next cycle */
          ord_start = ord_end; 
          elem += 3;
        } while (elem < nelems); /* end of element loop */
      } /* end of gtype big-if */
    } /* end of not-null object test */
  } while (shape->type == MS_SHAPE_NULL);

  return MS_SUCCESS;
}

int msOracleSpatialLayerGetItems( layerObj *layer )
{ 
  msSetError( MS_ORACLESPATIALERR, "Function not implemented yet", "msOracleSpatialLayerGetItems()" );
  return MS_FAILURE; 
}

int msOracleSpatialLayerGetShape( layerObj *layer, shapeObj *shape, long record )
{ 
  msSetError( MS_ORACLESPATIALERR, "Function not implemented yet", "msOracleSpatialLayerGetShape()");
  return MS_FAILURE; 
}

int msOracleSpatialLayerGetExtent(layerObj *layer, rectObj *extent)
{ 
  msSetError( MS_ORACLESPATIALERR, "Function not implemented yet", "msOracleSpatialLayerGetExtent()");
  return MS_FAILURE; 
}

int msOracleSpatialLayerInitItemInfo(layerObj *layer)
{
  int i, *itemindexes;

  if (layer->numitems == 0)
    return MS_SUCCESS;

  if (layer->iteminfo)
    free( layer->iteminfo );

  if((layer->iteminfo=(int *)malloc(sizeof(int)*layer->numitems)) == NULL) {
    msSetError( MS_MEMERR, NULL, "msOracleSpatialLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  itemindexes = (int*)layer->iteminfo;
  for( i=0; i<layer->numitems; ++i ) 
    itemindexes[i] = i; // last one is always the geometry one - the rest are non-geom

  return MS_SUCCESS;
}

void msOracleSpatialLayerFreeItemInfo(layerObj *layer)
{ 
  if (layer->iteminfo)
    free(layer->iteminfo);
  layer->iteminfo = NULL;
}

int msOracleSpatialLayerGetAutoStyle( mapObj *map, layerObj *layer, classObj *c, int tile, long record )
{ 
  msSetError( MS_ORACLESPATIALERR, "Function not implemented yet", "msLayerGetAutoStyle()" );
  return MS_FAILURE; 
}

#else // OracleSpatial "not-supported" procedures

int msOracleSpatialLayerOpen(layerObj *layer)
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerOpen()" );
  return MS_FAILURE; 
}

int msOracleSpatialLayerClose(layerObj *layer)
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerClose()" );
  return MS_FAILURE; 
}

int msOracleSpatialLayerWhichShapes(layerObj *layer, rectObj rect)
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerWhichShapes()" );
  return MS_FAILURE; 
}

int msOracleSpatialLayerNextShape(layerObj *layer, shapeObj *shape)
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerNextShape()" );
  return MS_FAILURE; 
}

int msOracleSpatialLayerGetItems(layerObj *layer)
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerGetItems()" );
  return MS_FAILURE; 
}

int msOracleSpatialLayerGetShape( layerObj *layer, shapeObj *shape, long record )
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerGetShape()" );
  return MS_FAILURE; 
}

int msOracleSpatialLayerGetExtent(layerObj *layer, rectObj *extent)
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerGetExtent()" );
  return MS_FAILURE; 
}

int msOracleSpatialLayerInitItemInfo(layerObj *layer)
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerInitItemInfo()" );
  return MS_FAILURE; 
}

void msOracleSpatialLayerFreeItemInfo(layerObj *layer)
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerFreeItemInfo()" ); 
}

int msOracleSpatialLayerGetAutoStyle( mapObj *map, layerObj *layer, classObj *c, int tile, long record )
{ 
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msLayerGetAutoStyle()" );
  return MS_FAILURE; 
}

#endif

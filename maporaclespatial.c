/*****************************************************************************
 *             -- Oracle Spatial (SDO) support for MapServer --              *
 *                                                                           *
 *  Author: Rodrigo Becke Cabral (cabral@cttmar.univali.br)                  *
 *  Collaborator: Adriana Gomes Alves                                        *
 *  MapServer: MapServer 3.5 (cvs)                                           *
 *  Oracle: Oracle Enterprise 8.1.7 (linux)                                  *
 *                                                                           *
 *****************************************************************************
 = This piece for MapServer has been developed under the funding of
 = agreement n.45/00 between CTTMAR/UNIVALI (www.cttmar.univali.br)
 = and CEPSUL/IBAMA (www.ibama.gov.br). This is a typical open source
 = initiative. Feel free to use it.
 *****************************************************************************
 * $Id$
 *
 * Revision 1.8   $Date$
 * Updated mapfile DATA statement to include SRID number.
 * SRID fix in r1.6 proved to be inefficient and time-consuming.
 *
 * Revision 1.7   2002/01/19 18:29:25 [CVS-TIME]
 * Fixed bug in SQL statement when using "FILTER" in mapfile.
 *
 * Revision 1.6   2001/12/22 18:32:02 [CVS-TIME]
 * Fixed SRID mismatch error.
 *
 * Revision 1.5   2001/11/21 22:35:58 [CVS-TIME]
 * Added support for 2D circle geometries (gtype/etype/interpretation):
 * - (2003/[?00]3/4)
 *
 * Revision 1.4   2001/10/22 17:09:03 [CVS-TIME]
 * Added support for mapfile items.
 *
 * Revision 1.3   2001/10/18 11:39:04 [CVS-TIME]
 * Added support for the following 2D geometries:
 * - 2001/1/1         point
 * - 2001/1/n         multipoint
 * - 2002/2/1         line string
 *
 * Revision 1.2   2001/09/28 10:42:29 [GMT-03:00]
 * Added OracleSpatial "partial" support. Displaying only the following
 * 2D geometries (gtype/etype/interpretation):
 * - 2001/NULL/NULL   point
 * - 2003/[?00]3/1    simple polygon
 * - 2003/[?00]3/3    rectangle
 *
 * Revisions 1.0/1.1
 * OracleSpatial support stubbed in by sdlime.
 *
 *****************************************************************************
 *
 * Using OracleSpatial:
 * - CONNECTIONTYPE oraclespatial
 * - CONNECTION 'username/password@database'
 * - DATA 'geometry_column FROM table_name [USING SRID srid#]'
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

#define ARRAY_SIZE                 1
#define TEXT_SIZE                  256
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

typedef
  text
  item_text[TEXT_SIZE];

typedef
  item_text
  item_text_array[ARRAY_SIZE];

typedef struct {
  /* oracle handles */
  OCIEnv           *envhp;
  OCIError         *errhp;
  OCISvcCtx        *svchp;
  /* query data */
  OCIStmt          *stmthp;
  OCIDescribe      *dschp;
  OCIType          *tdo;
  /* fetch data */
  int              rows_fetched;
  int              row_num;
  int              row;
  item_text_array  *items; /* items buffer */
  SDOGeometryObj   *obj[ARRAY_SIZE]; /* spatial object buffer */
  SDOGeometryInd   *ind[ARRAY_SIZE]; /* object indicator */
}
msOracleSpatialLayerInfo;

/* local prototypes */
static int TRY( msOracleSpatialLayerInfo *layerinfo, sword status );
static int ERROR( char *routine, msOracleSpatialLayerInfo *layerinfo );
static void msSplitLogin( char *connection, char *username, char *password, char *dblink );
static int msSplitData( char *data, char *geometry_column_name, char *table_name, char *srid );
static msOracleSpatialLayerInfo *msOCIConnect( char *username, char *password, char *dblink );
static void msOCIDisconnect( msOracleSpatialLayerInfo *layerinfo );
static int msOCIGetOrdinates( msOracleSpatialLayerInfo *layerinfo, SDOGeometryObj *obj, int s, int e, pointObj *pt );
static int msOCIConvertCircle( pointObj *pt );

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

/* break layer->data into geometry_column_name, table_name and srid */
static int msSplitData( char *data, char *geometry_column_name, char *table_name, char *srid )
{
  char *tok_from = "from";
  char *tok_srid = "using srid";
  int parenthesis;
  char *src = data, *tgt;
  // clearup
  *geometry_column_name = *table_name = 0;
  // bad 'data'
  if (data == NULL) return 0; 
  // parsing 'geometry_column_name'
  for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
  for( tgt=geometry_column_name; *src; src++, tgt++ )
    if (isspace( *src )) break;
    else *tgt = *src;
  *tgt = 0;
  // parsing 'from'
  for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
  for( ;*src && *tok_from && tolower(*src)==*tok_from; src++, tok_from++ ) ;
  if (*tok_from != '\0') return 0;
  // parsing 'table_name' or '(SELECT stmt)'
  for( ;*src && isspace( *src ); src++ ); /* skip blanks */
  for( tgt=table_name, parenthesis=0; *src; src++, tgt++ ) {
    if (*src == '(') parenthesis++;
    else if (*src == ')') parenthesis--;
    else if (parenthesis==0 && isspace( *src )) break; /* stop on spaces */
    *tgt = *src;
  }
  *tgt = 0;
  // parsing 'srid'
  for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
  if (*src != '\0' && table_name[0]!='(') { // srid is defined
    // parse token
    for( ;*src && *tok_srid && tolower(*src)==*tok_srid; src++, tok_srid++ ) ;
    for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
    // do the parsing
    if (*tok_srid != '\0') return 0;
    for( tgt=srid; *src; src++, tgt++ )
      if (isspace( *src )) break;
      else *tgt = *src;
    *tgt = 0;
  }
  else
    strcpy( srid, "NULL" );
  // finish parsing
  for( ;*src && isspace( *src ); src++ ); /* skip blanks */
  return (*src == '\0');
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
    /* allocate errhp */
    OCIHandleAlloc( (dvoid *)layerinfo->envhp, (dvoid **)&layerinfo->errhp, (ub4)OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0 ) )
  && TRY( layerinfo,
    /* logon */
    OCILogon( layerinfo->envhp, layerinfo->errhp, &layerinfo->svchp, username, strlen(username), password, strlen(password), dblink, strlen(dblink) ) )
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
  if (layerinfo->svchp != NULL)
    OCILogoff( layerinfo->svchp, layerinfo->errhp );
  if (layerinfo->errhp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->errhp, (ub4)OCI_HTYPE_ERROR );
  if (layerinfo->envhp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->envhp, (ub4)OCI_HTYPE_ENV );
  if (layerinfo->items != NULL)
    free( layerinfo->items );
  /* zero layerinfo out. can't free it because it's not the actual layer->oraclespatiallayerinfo pointer
   * but a copy of it. the only function that frees layer->oraclespatiallayerinfo is msOracleSpatialLayerClose */
  memset( layerinfo, 0, sizeof( msOracleSpatialLayerInfo ) ); 
}

/* get ordinates from SDO buffer */
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
  if (!success) { // switch points 1 & 2
    ptaux = pt[1];
    pt[1] = pt[2];
    pt[2] = ptaux;
    dXa = pt[1].x - pt[0].x;
    success = (fabs( dXa ) > 1e-8);
  }
  if (success) {
    dXb = pt[2].x - pt[1].x;
    success = (fabs( dXb ) > 1e-8);
    if (!success) { // insert point 2 before point 0
      ptaux = pt[2];
      pt[2] = pt[1];
      pt[1] = pt[0];
      pt[0] = ptaux;
      dXb = dXa; // segment A has become B
      dXa = pt[1].x - pt[0].x; // recalculate new segment A
      success = (fabs( dXa ) > 1e-8);
    }
  }
  if (success) {
    ma = (pt[1].y - pt[0].y)/dXa;
    mb = (pt[2].y - pt[1].y)/dXb;
    success = (fabs( mb - ma ) > 1e-8);
  }
  if (!success) {
    strcpy( last_oci_call_ms_error, "Points in circle object are colinear" );
    last_oci_call_ms_status = MS_FAILURE;
    return 0;
  }

  /* calculate center and radius */
  cx = (ma*mb*(pt[0].y - pt[2].y) + mb*(pt[0].x + pt[1].x) - ma*(pt[1].x + pt[2].x))
       /(2*(mb - ma));
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

/* opens a layer by connecting to db with username/password@database stored in layer->connection */
int msOracleSpatialLayerOpen( layerObj *layer )
{
  char username[1024], password[1024], dblink[1024];

  if (layer->oraclespatiallayerinfo != NULL)
    return MS_SUCCESS;

  if (layer->data == NULL) {
    msSetError( MS_ORACLESPATIALERR, 
      "Missing DATA clause in OracleSpatial layer definition. DATA statement must be "
      "'geometry_column FROM table_name [USING SRID srid#]' or "
      "'geometry_column FROM (SELECT stmt)'.", 
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
  int success, apply_window, i;
  char query_str[6000];
  char table_name[2000], geom_column_name[100], srid[100];
  OCIDefine *adtp = NULL, *items[ARRAY_SIZE] = { NULL };

  /* get layerinfo */
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->oraclespatiallayerinfo;
  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR,
      "msOracleSpatialLayerWhichShapes called on unopened layer",
      "msOracleSpatialLayerWhichShapes()" );
    return MS_FAILURE;
  }

  /* allocate enough space for items */
  if (layer->numitems > 0) {
    layerinfo->items = (item_text_array *)malloc( sizeof(item_text_array) * layer->numitems );
    if (layerinfo->items == NULL) {
      msSetError( MS_ORACLESPATIALERR,
        "Cannot allocate items buffer",
        "msOracleSpatialLayerWhichShapes()" );
      return MS_FAILURE;
    }
  }

  /* parse geom_column_name and table_name */
  if (!msSplitData( layer->data, geom_column_name, table_name, srid )) {
    msSetError( MS_ORACLESPATIALERR,
      "Error parsing OracleSpatial DATA variable. Must be "
      "'geometry_column FROM table_name [USING SRID srid#]' or "
      "'geometry_column FROM (SELECT stmt)'.", 
      "msOracleSpatialLayerWhichShapes()" );
    return MS_FAILURE;
  }

  /* define SQL query */
  apply_window = (table_name[0] != '('); /* table isn´t a SELECT statement */
  strcpy( query_str, "SELECT rownum" );
  for( i=0; i<layer->numitems; ++i )
    sprintf( query_str + strlen(query_str), ", %s", layer->items[i] );
  sprintf( query_str + strlen(query_str), ", %s FROM %s", geom_column_name, table_name );
  if (apply_window || layer->filter.string != NULL)
    strcat( query_str, " WHERE " );
  if (layer->filter.string != NULL) {
    strcat( query_str, layer->filter.string );
    if (apply_window) strcat( query_str, " AND " );
  }
  if (apply_window)
    sprintf( query_str + strlen(query_str),
        "SDO_FILTER( %s.%s, MDSYS.SDO_GEOMETRY("
          "2003, %s, NULL,"
          "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
          "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g) ),"
        "'querytype=window') = 'TRUE'",
        table_name, geom_column_name, srid,
        rect.minx, rect.miny, rect.maxx, rect.maxy );

  /* parse and execute SQL query */
  success = TRY( layerinfo,
    /* prepare */
    OCIStmtPrepare( layerinfo->stmthp, layerinfo->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) );

  if (success && layer->numitems > 0) {
    for( i=0; i<layer->numitems && success; ++i )
      success = TRY( layerinfo,
        OCIDefineByPos( layerinfo->stmthp, &items[i], layerinfo->errhp, (ub4)i+2, (dvoid *)layerinfo->items[i], (sb4)TEXT_SIZE, SQLT_STR, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) );
  }

  if (success) {
    success = TRY( layerinfo,
      /* define spatial position adtp ADT object */
      OCIDefineByPos( layerinfo->stmthp, &adtp, layerinfo->errhp, (ub4)layer->numitems+2, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
    && TRY( layerinfo,
      /* define object tdo from adtp */
      OCIDefineObject( adtp, layerinfo->errhp, layerinfo->tdo, (dvoid **)layerinfo->obj, (ub4 *)0, (dvoid **)layerinfo->ind, (ub4 *)0 ) )
    && TRY( layerinfo,
      /* execute */
      OCIStmtExecute( layerinfo->svchp, layerinfo->stmthp, layerinfo->errhp, (ub4)ARRAY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ) )
    &&  TRY( layerinfo,
      /* get rows fetched */
      OCIAttrGet( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, layerinfo->errhp ) );
  }

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
  int success, i, n;
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

    /* set obj & ind for current row */
    obj = layerinfo->obj[ layerinfo->row ];
    ind = layerinfo->ind[ layerinfo->row ];

    /* get the items for the shape */
    shape->index = 0; /* remember to get a rownum */
    shape->numvalues = layer->numitems;
    shape->values = (char **)malloc( sizeof(char*) * shape->numvalues );
    for( i=0; i<shape->numvalues; ++i )
      shape->values[i] = strdup( layerinfo->items[i][ layerinfo->row ] );

    /* increment for next row */
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
            continue; /* jump to end of big-while below */
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
            case 34: /* circle defined by 3 points */
              n = msOCIGetOrdinates( layerinfo, obj, ord_start, ord_end, point5 ); /* n must be < 5 */
              if (n == 3) {
                if (msOCIConvertCircle( point5 )) {
                  shape->type = MS_SHAPE_POINT;
                  points.numpoints = 2;
                  points.point = point5;
                  msAddLine( shape, &points );
                }
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
    } /* end of not-null-object if */
  } while (shape->type == MS_SHAPE_NULL); /* end of big-while */

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

int msOracleSpatialLayerInitItemInfo( layerObj *layer )
{
  return MS_SUCCESS;
}

void msOracleSpatialLayerFreeItemInfo( layerObj *layer )
{ 
  /* nothing to do */
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

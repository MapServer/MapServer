/*****************************************************************************
 *             -- Oracle Spatial (SDO) support for MapServer --              *
 *                                                                           *
 *  Authors: Fernando Simon (fsimon@univali.br)                              * 
 *           Rodrigo Becke Cabral (cabral@univali.br)                        *
 *  Collaborator: Adriana Gomes Alves                                        *
 *  MapServer: MapServer 4.4 (cvs)                                           *
 *  Oracle: Oracle 9.2 Spatial Cartridge 9.2 release 9.0.1                   *
 *                                                                           *
 *****************************************************************************
 = This piece for MapServer was originally developed under the funding of
 = agreement n.45/00 between CTTMAR/UNIVALI (www.cttmar.univali.br)
 = and CEPSUL/IBAMA (www.ibama.gov.br).
 *****************************************************************************
 = Current development is funded by CNPq (www.cnpq.br) under 
 = process 401263.03-7
 = and FUNCITEC (www.funcitec.rct-sc.br) under process FCTP1523-031.
 *****************************************************************************
 * $Id$
 *
 * Revision 1.13  $Date$
 * Fixed declarations problems - Bug #1044
 *
 * Revision 1.12  2004/11/05 20:33:14 [CVS-TIME]
 * Added debug messages.
 *
 * Revision 1.11  2004/10/30 05:15:15 [CVS-TIME]
 * Connection Pool support.
 * New query item support.
 * New improve of performance.
 * Updated mapfile DATA statement to include more options.
 * Bug fix from version 1.9
 *
 * Revision 1.10  2004/10/21 04:30:54 [CVS-TIME]
 * MS_CVSID support added.
 *
 * Revision 1.9   2004/04/30 13:27:46 [CVS-TIME]
 * Query item support implemented.
 * Query map is not being generated yet.
 *
 * Revision 1.8   2003/03/18 04:56:28 [CVS-TIME]
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
 * - DATA 'geometry_column FROM <table>(SELECT stmt)'
 *   or
 *   DATA 'geometr_column FROM <table> USING UNIQUE <column>' 
 *   or
 *   DATA 'geometry_column FROM <table> [USING SRID srid#]'
 *   or
 *   DATA 'geometr_column FROM <table> USING FILTER'
 *   or
 *   DATA 'geometr_column FROM <table> USING RELATE'
 *   or
 *   DATA 'geometr_column FROM <table> USING GEOPMRELATE'
 *
 *   <table> can be:
 *   One database table name
 *   or
 *   (SELECT stmt)
 *
 *****************************************************************************
 * Notices above shall be included in all copies or portions of the software.
 * This piece is provided "AS IS", without warranties of any kind. Got it?
 *****************************************************************************/

#include "map.h"

MS_CVSID("$Id$")

#ifdef USE_ORACLESPATIAL

#include <oci.h>
#include <ctype.h>

#define ARRAY_SIZE                 1024
#define QUERY_SIZE                 1
#define TEXT_SIZE                  256
#define TYPE_OWNER                 "MDSYS"
#define SDO_GEOMETRY               TYPE_OWNER".SDO_GEOMETRY"
#define SDO_GEOMETRY_LEN           strlen( SDO_GEOMETRY )
#define FUNCTION_NONE              0
#define FUNCTION_FILTER            1
#define FUNCTION_RELATE            2
#define FUNCTION_GEOMRELATE        3
#define TOLERANCE                  0.000005
#define NULLERRCODE                1405

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

typedef
  item_text
  item_text_array_query[QUERY_SIZE];  

typedef struct {
  /*Oracle handlers*/
  OCIEnv *envhp;
  OCIError *errhp;
  OCISvcCtx *svchp;
} msOracleSpatialHandler;
    
typedef struct {
  /* oracle handlers */
  msOracleSpatialHandler *orahandlers;
#if 0
  OCIEnv           *envhp;
  OCIError         *errhp;
  OCISvcCtx        *svchp;
#endif
  /* query data */
  OCIStmt          *stmthp;
  OCIDescribe      *dschp;
  OCIType          *tdo;
  /* fetch data */
  int              rows_fetched;
  int              row_num;
  int              row;
  item_text_array  *items; /* items buffer */
  item_text_array_query  *items_query; /* items buffer */
  SDOGeometryObj   *obj[ARRAY_SIZE]; /* spatial object buffer */
  SDOGeometryInd   *ind[ARRAY_SIZE]; /* object indicator */
} msOracleSpatialLayerInfo;

/* local prototypes */
static int TRY( msOracleSpatialHandler *hand, sword status );
static int ERROR( char *routine, msOracleSpatialLayerInfo *layerinfo );
static void msSplitLogin( char *connection, char *username, char *password, char *dblink );
static int msSplitData( char *data, char *geometry_column_name, char *table_name, char* unique, char *srid, int *function);
static void msOCICloseConnection( void *layerinfo );
static msOracleSpatialHandler *msOCISetHandlers( char *username, char *password, char *dblink );
static void msOCISetLayerInfo( msOracleSpatialLayerInfo *layerinfo );
static void msOCIDisconnect( msOracleSpatialLayerInfo *layerinfo );
static void msOCICloseHandlers( msOracleSpatialHandler *hand );
static void msOCIClearLayerInfo( msOracleSpatialLayerInfo *layerinfo );
static int msOCIGetOrdinates( msOracleSpatialLayerInfo *layerinfo, SDOGeometryObj *obj, int s, int e, pointObj *pt );
static int msOCIConvertCircle( pointObj *pt );
static void osShapeBounds ( shapeObj *shp );
static void osPointCluster(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int interpretation);
static void osPoint(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt);
static void osClosedPolygon(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int elem_type);
static void osRectangle(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt);
static void osCircle(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt);
static int osGetOrdinates(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, SDOGeometryInd *ind);

/* local status */
static int last_oci_call_ms_status = MS_FAILURE;
static text last_oci_call_ms_error[2048] = "";

/* if an error ocurred call msSetError, sets last_oci_call_ms_status to MS_FAILURE and return 0;
 * otherwise returns 1 */
static int TRY( msOracleSpatialHandler *hand, sword status )
{
  sb4 errcode = 0;

  if (last_oci_call_ms_status == MS_FAILURE)
    return 0; /* error from previous call */

  switch (status) {
    case OCI_ERROR:
      OCIErrorGet((dvoid *)hand->errhp, (ub4)1, (text *)NULL, &errcode, last_oci_call_ms_error, (ub4)sizeof(last_oci_call_ms_error), OCI_HTYPE_ERROR );
      if (errcode == NULLERRCODE)
        return 1;
      last_oci_call_ms_error[sizeof(last_oci_call_ms_error)-1] = 0; /* terminate string!? */
    break;
    case OCI_NEED_DATA: 
      strcpy( last_oci_call_ms_error, "OCI_NEED_DATA" ); 
      break;
    case OCI_INVALID_HANDLE: 
      strcpy( last_oci_call_ms_error, "OCI_INVALID_HANDLE" ); 
      break;
    case OCI_STILL_EXECUTING: 
      strcpy( last_oci_call_ms_error, "OCI_STILL_EXECUTING"); 
      break;
    case OCI_CONTINUE: 
      strcpy( last_oci_call_ms_error, "OCI_CONTINUE" ); 
      break;
    default:
      return 1; /* no error */
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
  /* clearup */
  *username = *password = *dblink = 0;
  /* bad 'connection' */
  if (connection == NULL) return;
  /* ok, split connection */
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
static int msSplitData( char *data, char *geometry_column_name, char *table_name, char *unique, char *srid, int *function )
{
  char *tok_from = "from";
  char *tok_using = "using";
  char *tok_unique = "unique";
  char *tok_srid = "srid";  
  char tok_function[11] = "";
  int parenthesis, len;
  char *src = data, *tgt, *flk;    
  
  /* clearup */
  *geometry_column_name = *table_name = 0;
  
  /* bad 'data' */
  if (data == NULL) 
      return 0; 
  
  /* parsing 'geometry_column_name' */
  for( ;*src && isspace( *src ); src++ ); /* skip blanks */
  for( tgt=geometry_column_name; *src; src++, tgt++ )
    if (isspace( *src )) break;
    else *tgt = *src;
  *tgt = 0;
  
  /* parsing 'from' */
  for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
  for( ;*src && *tok_from && tolower(*src)==*tok_from; src++, tok_from++ );
  if (*tok_from != '\0') 
      return 0;
  
  /* parsing 'table_name' or '(SELECT stmt)' */
  for( ;*src && isspace( *src ); src++ ); /* skip blanks */
  for( tgt=table_name, parenthesis=0; *src; src++, tgt++ ){
    if (*src == '(') 
        parenthesis++;
    else if (*src == ')') 
        parenthesis--;
    else if (parenthesis==0 && isspace( *src )) 
        break; /* stop on spaces */
    *tgt = *src;
  }
  *tgt = 0;
    
  strcpy( unique, "" );
  strcpy( srid, "NULL" );    
  
  /* parsing 'unique' */
  for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
  if (*src != '\0'){ 
    /* expects using [unique <column>] [srid <srid>] */
    /* parse 'using' */
    for( ;*src && *tok_using && tolower(*src)==*tok_using; src++, tok_using++ );
    if (*tok_using != '\0') 
        return 0;
    /* parsing 'unique' */
    for( ;*src && isspace( *src ); src++ ); /* skip blanks */
    for( ;*src && *tok_unique && tolower(*src)==*tok_unique; src++, tok_unique++ );
    if (*tok_unique == '\0'){
       for( ;*src && isspace( *src ); src++ ); /* skip blanks */
       if (*src == '\0') return 0;
       for( tgt=unique; *src; src++, tgt++ )
          if (isspace( *src )) break;
          else *tgt = *src;
       *tgt = 0;
    }
    
    /* parsing 'srid' */
    for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
    for( ;*src && *tok_srid && tolower(*src)==*tok_srid; src++, tok_srid++ ) ;
    if (*tok_srid == '\0'){
       for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
       if (*src == '\0') return 0;
       for( tgt=srid; *src; src++, tgt++ )
          if (isspace( *src )) break;
          else *tgt = *src;
       *tgt = 0;
    }
    
#if 0
    mempcy(tok_function, inicio, len);
    tok_function[len] = '\0';
     
    flk[1] = '\0';   
    for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
    for( ;*src && !isspace( *src ) && strlen(tok_function) < 11 ; src++ )
    {
        flk[0] = *src;
        strcat (tok_function, flk);
    }
#endif
    
    /*parsing function*/
    for( ;*src && isspace( *src ); src++ );
    flk = src;
    for( ;*src && !isspace( *src ); src++ );
    len = src-flk;
    if (len > 10)
        strcpy(tok_function, "ERROR");
    else        
        strncpy(tok_function, flk, len);      
       
    if (!strcmp(tok_function, "FILTER") || !strcmp(tok_function, ""))
        *function = FUNCTION_FILTER;
    else if(!strcmp(tok_function, "RELATE"))
      *function = FUNCTION_RELATE;
    else if (!strcmp(tok_function,"GEOMRELATE"))
      *function = FUNCTION_GEOMRELATE;
    else
      *function = -1;      

    if (*tok_unique != '\0' && *tok_srid != '\0' && *function == -1)
      return 0;    
  }  
  /* finish parsing */
  for( ;*src && isspace( *src ); src++ ); /* skip blanks */
  return (*src == '\0');
}

static void msOCISetLayerInfo(msOracleSpatialLayerInfo *layerinfo)
{
  int success;
  OCIParam *paramp = NULL;
  OCIRef *type_ref = NULL;
  
  msOracleSpatialHandler *hand = layerinfo->orahandlers;
  
  success = TRY( hand,
    /* allocate stmthp */
    OCIHandleAlloc( (dvoid *)hand->envhp, (dvoid **)&layerinfo->stmthp, (ub4)OCI_HTYPE_STMT, (size_t)0, (dvoid **)0 ) )
  && TRY( hand,
    /* allocate dschp */
    OCIHandleAlloc( hand->envhp, (dvoid **)&layerinfo->dschp, (ub4)OCI_HTYPE_DESCRIBE, (size_t)0, (dvoid **)0 ) )
  && TRY( hand,
    /* describe SDO_GEOMETRY in svchp (dschp) */
    OCIDescribeAny( hand->svchp, hand->errhp, (text *)SDO_GEOMETRY, (ub4)SDO_GEOMETRY_LEN, OCI_OTYPE_NAME, (ub1)1, (ub1)OCI_PTYPE_TYPE, layerinfo->dschp ) )
  && TRY( hand,
    /* get param for SDO_GEOMETRY */
    OCIAttrGet( (dvoid *)layerinfo->dschp, (ub4)OCI_HTYPE_DESCRIBE, (dvoid *)&paramp, (ub4 *)0,  (ub4)OCI_ATTR_PARAM, hand->errhp ) )
  && TRY( hand,
    /* get type_ref for SDO_GEOMETRY */
    OCIAttrGet( (dvoid *)paramp, (ub4)OCI_DTYPE_PARAM, (dvoid *)&type_ref, (ub4 *)0, (ub4)OCI_ATTR_REF_TDO, hand->errhp ) )
  && TRY( hand,
    /* get TDO for SDO_GEOMETRY */
    OCIObjectPin( hand->envhp, hand->errhp, type_ref, (OCIComplexObject *)0, OCI_PIN_ANY, OCI_DURATION_SESSION, OCI_LOCK_NONE, (dvoid **)&layerinfo->tdo ) );

  if ( !success ) {
      layerinfo = NULL;
      /*free( layerinfo );*/
      /*return NULL;*/
  }
}

/* connect to database */
static msOracleSpatialHandler *msOCISetHandlers( char *username, char *password, char *dblink )
{
  int success;
  
  msOracleSpatialHandler *hand = (msOracleSpatialHandler *) malloc( sizeof(msOracleSpatialHandler));
  memset( hand, 0, sizeof(msOracleSpatialHandler) );

  success = TRY( hand,
    /* allocate envhp */
    OCIEnvCreate( &hand->envhp, OCI_OBJECT, (dvoid *)0, 0, 0, 0, (size_t) 0, (dvoid **)0 ) )
  && TRY( hand,
    /* allocate errhp */
    OCIHandleAlloc( (dvoid *)hand->envhp, (dvoid **)&hand->errhp, (ub4)OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0 ) )
  && TRY( hand,
    /* logon */
    OCILogon( hand->envhp, hand->errhp, &hand->svchp, username, strlen(username), password, strlen(password), dblink, strlen(dblink) ) );

  if ( !success ) {
      msOCICloseHandlers(hand);
      free( hand );
      return NULL;
  }
  
  return hand;  
    
}

static void msOCICloseHandlers( msOracleSpatialHandler *hand )
{
  if (hand->svchp != NULL)
    OCILogoff( hand->svchp, hand->errhp );
  if (hand->errhp != NULL)
    OCIHandleFree( (dvoid *)hand->errhp, (ub4)OCI_HTYPE_ERROR );
  if (hand->envhp != NULL)
    OCIHandleFree( (dvoid *)hand->envhp, (ub4)OCI_HTYPE_ENV );
  memset( hand, 0, sizeof (msOracleSpatialHandler));
}

static void msOCIClearLayerInfo( msOracleSpatialLayerInfo *layerinfo )
{
  if (layerinfo->dschp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->dschp, (ub4)OCI_HTYPE_DESCRIBE );
  if (layerinfo->stmthp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT );
  if (layerinfo->items != NULL)
    free( layerinfo->items );
  if (layerinfo->items_query != NULL)
    free( layerinfo->items_query );
  memset( layerinfo, 0, sizeof( msOracleSpatialLayerInfo ) ); 
}

/* disconnect from database */
static void msOCIDisconnect( msOracleSpatialLayerInfo *layerinfo )
{  
  msOracleSpatialHandler *hand = layerinfo->orahandlers;
  
  if (layerinfo->dschp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->dschp, (ub4)OCI_HTYPE_DESCRIBE );
  if (layerinfo->stmthp != NULL)
    OCIHandleFree( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT );
  if (hand->svchp != NULL)
    OCILogoff( hand->svchp, hand->errhp );
  if (hand->errhp != NULL)
    OCIHandleFree( (dvoid *)hand->errhp, (ub4)OCI_HTYPE_ERROR );
  if (hand->envhp != NULL)
    OCIHandleFree( (dvoid *)hand->envhp, (ub4)OCI_HTYPE_ENV );
  if (layerinfo->items != NULL)
    free( layerinfo->items );
  if (layerinfo->items_query != NULL)
    free( layerinfo->items_query );
  /* zero layerinfo out. can't free it because it's not the actual layer->layerinfo pointer
   * but a copy of it. the only function that frees layer->layerinfo is msOracleSpatialLayerClose */
  memset( hand, 0, sizeof (msOracleSpatialHandler));
  memset( layerinfo, 0, sizeof( msOracleSpatialLayerInfo ) ); 
}

/* get ordinates from SDO buffer */
static int msOCIGetOrdinates( msOracleSpatialLayerInfo *layerinfo, SDOGeometryObj *obj, int s, int e, pointObj *pt )
{
  double x, y;
  int i, n, success = 1;
  boolean exists;
  OCINumber *oci_number;

  msOracleSpatialHandler *hand = layerinfo->orahandlers;
  
  for( i=s, n=0; i < e && success; i+=2, n++ ) {
    success = TRY( hand,
        OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates,
          (sb4)i, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
      && TRY( hand,
        OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&x ) )
      && TRY( hand,
        OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates,
          (sb4)i+1, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
      && TRY( hand,
        OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&y ) );
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
  if (!success) { /* switch points 1 & 2 */
    ptaux = pt[1];
    pt[1] = pt[2];
    pt[2] = ptaux;
    dXa = pt[1].x - pt[0].x;
    success = (fabs( dXa ) > 1e-8);
  }
  if (success) {
    dXb = pt[2].x - pt[1].x;
    success = (fabs( dXb ) > 1e-8);
    if (!success) { /* insert point 2 before point 0 */
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
  if (!success) {
    strcpy( last_oci_call_ms_error, "Points in circle object are colinear" );
    last_oci_call_ms_status = MS_FAILURE;
    return 0;
  }

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

static void osShapeBounds ( shapeObj *shp )
{
  int i, f;
  if ( (shp->numlines > 0) && (shp->line[0].numpoints > 0) ){
    shp->bounds.minx = shp->line[0].point[0].x;
    shp->bounds.maxx = shp->line[0].point[0].x;
    shp->bounds.miny = shp->line[0].point[0].y;
    shp->bounds.maxy = shp->line[0].point[0].y;    
  }
  for ( i = 0 ; i < shp->numlines; i++){
    for( f = 0; f < shp->line[i].numpoints; f++){
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

static void osPointCluster(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int interpretation)
{
  int n;
  n = (end - start)/2;
  if (n == interpretation){
    points.point = (pointObj *)malloc( sizeof(pointObj)*n );
    n = msOCIGetOrdinates( layerinfo, obj, start, end, points.point );
    if (n == interpretation && n>0){
      shape->type = MS_SHAPE_POINT;
      points.numpoints = n;
      msAddLine( shape, &points );
    }
    free( points.point );
  }
}

static void osPoint(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt)
{
  int n;
  n = msOCIGetOrdinates( layerinfo, obj, start, end, pnt ); /* n must be < 5 */
  if (n == 1){
    shape->type = MS_SHAPE_POINT;
    points.numpoints = 1;
    points.point = pnt;
    msAddLine( shape, &points );
  }
}

static void osClosedPolygon(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int elem_type)
{
  int n;
  n = (end - start)/2;
  points.point = (pointObj *)malloc( sizeof(pointObj)*n );
  n = msOCIGetOrdinates( layerinfo, obj, start, end, points.point );
  if (n > 0){
    shape->type = (elem_type==21) ? MS_SHAPE_LINE : MS_SHAPE_POLYGON;
    points.numpoints = n;
    msAddLine( shape, &points );
  }
  free( points.point );
}

static void osRectangle(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt)
{
  int n;
  n = msOCIGetOrdinates( layerinfo, obj, start, end, pnt ); /* n must be < 5 */
  if (n == 2){
    shape->type = MS_SHAPE_POLYGON;
    points.numpoints = 5;
    points.point = pnt;
    /* point5 [0] & [1] contains the lower-left and upper-right points of the rectangle */
    pnt[2] = pnt[1];
    pnt[1].x = pnt[0].x;
    pnt[3].x = pnt[2].x;
    pnt[3].y = pnt[0].y;
    pnt[4] = pnt[0]; 
    msAddLine( shape, &points );
  }
}

static void osCircle(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt)
{
  int n;
  n = msOCIGetOrdinates( layerinfo, obj, start, end, pnt ); /* n must be < 5 */
  if (n == 3){
    if (msOCIConvertCircle( pnt )){
      shape->type = MS_SHAPE_POINT;
      points.numpoints = 2;
      points.point = pnt;
      msAddLine( shape, &points );
    }
  }
}

static int osGetOrdinates(msOracleSpatialLayerInfo *layerinfo, shapeObj *shape, SDOGeometryObj *obj, SDOGeometryInd *ind)
{
  int gtype, elem_type;
  ub4 etype;
  ub4 interpretation;
  int nelems, nords;
  int elem, ord_start, ord_end;
  boolean exists;
  OCINumber *oci_number;
  double x, y;
  int success;
  lineObj points = {0, NULL};
  pointObj point5[5]; /* for quick access */

  msOracleSpatialHandler *hand = layerinfo->orahandlers;
  
  if (ind->_atomic != OCI_IND_NULL)  /* not a null object */
  {
      nelems = nords = 0;
      success = TRY( hand, OCICollSize( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info, &nelems ) )
             && TRY( hand, OCICollSize( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, &nords ) )
             && TRY( hand, OCINumberToInt( hand->errhp, &(obj->gtype), (uword)sizeof(int), OCI_NUMBER_SIGNED, (dvoid *)&gtype ) )
             && (nords%2==0 && nelems%3==0); /* check %2==0 for 2D geometries; and %3==0 for element info triplets */
      if (success && (gtype==2001 || gtype==2002 || gtype==2003 || gtype==2005 || gtype==2006 || gtype==2007)){
        /* reading SDO_POINT from SDO_GEOMETRY for a 2D point geometry */
        if (gtype==2001 && ind->point._atomic == OCI_IND_NOTNULL && ind->point.x == OCI_IND_NOTNULL && ind->point.y == OCI_IND_NOTNULL){
            success = TRY( hand, OCINumberToReal( hand->errhp, &(obj->point.x), (uword)sizeof(double), (dvoid *)&x ) )
                   && TRY( hand, OCINumberToReal( hand->errhp, &(obj->point.y), (uword)sizeof(double), (dvoid *)&y ) );
            if (ERROR( "msOracleSpatialLayerNextShape()", layerinfo ))
              return MS_FAILURE;
            shape->type = MS_SHAPE_POINT;
            points.numpoints = 1;
            points.point = point5;
            point5[0].x = x;
            point5[0].y = y;
            msAddLine( shape, &points );
            return MS_SUCCESS;
            /*continue;*/ /* jump to end of big-while below */
        }
        /* if SDO_POINT not fetched, proceed reading elements (element info/ordinates) */
        success = TRY( hand, OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info,(sb4)0, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
               && TRY( hand, OCINumberToInt( hand->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_SIGNED, (dvoid *)&ord_end ) );
               
        elem = 0;
        ord_end--; /* shifts offset from 1..n to 0..n-1 */
        
        do{
          ord_start = ord_end;
          if (elem+3 >= nelems) /* processing last element */
          {
            ord_end = nords;
            success = 1;
          }
          else /* get start ordinate position for next element which is ord_end for this element */
          {
            success = TRY( hand, OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info, (sb4)elem+3, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
                   && TRY( hand, OCINumberToInt( hand->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_SIGNED, (dvoid *)&ord_end ) );
            ord_end--; /* shifts offset from 1..n to 0..n-1 */
          }
          success = (success 
                  && TRY( hand, OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info, (sb4)elem+1, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0) )
                  && TRY( hand, OCINumberToInt( hand->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_UNSIGNED, (dvoid *)&etype ) )
                  && TRY( hand, OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info, (sb4)elem+2, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
                  && TRY( hand, OCINumberToInt( hand->errhp, oci_number, (uword)sizeof(ub4), OCI_NUMBER_UNSIGNED, (dvoid *)&interpretation ) ));
          if (ERROR( "msOracleSpatialLayerNextShape()", layerinfo ))
          {
            return MS_FAILURE;
          }
          elem_type = (etype == 1 && interpretation > 1) ? 10 : ((etype%10)*10 + interpretation);
          switch (elem_type) {
            case 10: /* point cluster with 'interpretation'-points */
              osPointCluster (layerinfo, shape, obj, ord_start, ord_end, points, interpretation);
              break;
            case 11: /* point type */
              osPoint(layerinfo, shape, obj, ord_start, ord_end, points, point5);
              break;
            case 21: /* line string whose vertices are connected by straight line segments */
            case 31: /* simple polygon with n points, last point equals the first one */
              osClosedPolygon(layerinfo, shape, obj, ord_start, ord_end, points, elem_type);
              break;
            case 33: /* rectangle defined by 2 points */
              osRectangle(layerinfo, shape, obj, ord_start, ord_end, points, point5);
              break;
            case 34: /* circle defined by 3 points */
              osCircle(layerinfo, shape, obj, ord_start, ord_end, points, point5);
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
  return MS_SUCCESS;
}

static void msOCICloseConnection( void *hand )
{
    msOCICloseHandlers( (msOracleSpatialHandler *)hand );
}

/* opens a layer by connecting to db with username/password@database stored in layer->connection */
int msOracleSpatialLayerOpen( layerObj *layer )
{
  char username[1024], password[1024], dblink[1024];  
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)malloc( sizeof(msOracleSpatialLayerInfo) );    
  
  memset( layerinfo, 0, sizeof(msOracleSpatialLayerInfo) );  
  
  if (layer->debug)
    msDebug("msOracleSpatialLayerOpen called with: %s\n",layer->data);
  
  
  if (layer->layerinfo != NULL)
    return MS_SUCCESS;

  if (layer->data == NULL) {
   msSetError( MS_ORACLESPATIALERR, 
           "Error parsing OracleSpatial DATA variable. Must be:"
           "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
           "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'."
           "If want to set the FUNCTION statement you can use: FILTER, RELATE or GEOMRELATE.",
           "msOracleSpatialLayerOpen()");
    return MS_FAILURE;
  }

  last_oci_call_ms_status = MS_SUCCESS;
  msSplitLogin( layer->connection, username, password, dblink );
     
  layerinfo->orahandlers = (msOracleSpatialHandler *) msConnPoolRequest( layer );
    
  if( layerinfo->orahandlers == NULL ){
    layerinfo->orahandlers = msOCISetHandlers( username, password, dblink );    
    if (layerinfo->orahandlers == NULL){      
      msSetError( MS_ORACLESPATIALERR, 
           "Cannot create OCI Handlers. "
           "Connection failure.",
           "msOracleSpatialLayerOpen()");
      msOCIClearLayerInfo( layerinfo );
      free(layerinfo);
      return MS_FAILURE;
    }
    
    if ( layer->debug )
        msDebug("msOracleSpatialLayerOpen. Shared connection not available. Creating one.\n");
    
    msConnPoolRegister( layer, layerinfo->orahandlers, msOCICloseConnection );      
  } 
  
  msOCISetLayerInfo(layerinfo);
  
  if (layerinfo == NULL){
      msSetError( MS_ORACLESPATIALERR, 
           "Cannot create OCI LayerInfo. "
           "Connection failure.",
           "msOracleSpatialLayerOpen()");
      msOCICloseHandlers( layerinfo->orahandlers );
      msOCIClearLayerInfo( layerinfo );
      free(layerinfo);
      return MS_FAILURE;
  } 
  layer->layerinfo = layerinfo;
        
  return layer->layerinfo != NULL ? MS_SUCCESS : MS_FAILURE;
}

/* closes the layer, disconnecting from db if connected. layer->layerinfo is freed */
int msOracleSpatialLayerClose( layerObj *layer )
{ 
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;
  
  if (layer->debug)
    msDebug("msOracleSpatialLayerClose was called. Layer connection: %s\n",layer->connection);
  
  if (layerinfo != NULL){
    
    if (layer->debug)
      msDebug("msOracleSpatialLayerClose. Cleaning Oracle handlers.\n");
    
    msConnPoolRelease( layer, layerinfo->orahandlers );    
    layerinfo->orahandlers = NULL;
    
    if (layer->debug)
      msDebug("msOracleSpatialLayerClose. Cleaning layerinfo handlers.\n");
    
    msOCIClearLayerInfo( layerinfo );
    /*msOCIDisconnect( layerinfo );*/
    free( layerinfo );
    layer->layerinfo = NULL;
  }

  return MS_SUCCESS;
}

/* create SQL statement for retrieving shapes */
int msOracleSpatialLayerWhichShapes( layerObj *layer, rectObj rect )
{
  int success, i;
  int function = 0;
  char query_str[6000];
  char table_name[2000], geom_column_name[100], unique[100], srid[100];
  OCIDefine *adtp = NULL, *items[ARRAY_SIZE] = { NULL };
  
  /* get layerinfo */
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;      
  msOracleSpatialHandler *hand = layerinfo->orahandlers;

  if (layer->debug)
    msDebug("msOracleSpatialLayerWhichShapes was called.\n");  
    
  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR,
      "msOracleSpatialLayerWhichShapes called on unopened layer",
      "msOracleSpatialLayerWhichShapes()" );
    return MS_FAILURE;
  }

  /* parse geom_column_name and table_name */
  if (!msSplitData( layer->data, geom_column_name, table_name, unique, srid, &function)) {
    msSetError( MS_ORACLESPATIALERR, 
      "Error parsing OracleSpatial DATA variable. Must be:"
      "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
      "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'."
      "If want to set the FUNCTION statement you can use: FILTER, RELATE or GEOMRELATE."
      "Your data statement: %s", 
      "msOracleSpatialLayerWhichShapes()", layer->data );
    return MS_FAILURE;
  } 
  
  /*Define the unique*/
  if (unique[0] == '\0')
  { 
      strcpy( unique, "rownum" );
  }
  sprintf( query_str, "SELECT %s", unique );
  
  /* allocate enough space for items */
  if (layer->numitems >= 0) {
    layerinfo->items = (item_text_array *)malloc( sizeof(item_text_array) * (layer->numitems+1) );
    if (layerinfo->items == NULL) {
      msSetError( MS_ORACLESPATIALERR,
        "Cannot allocate items buffer",
        "msOracleSpatialLayerWhichShapes()" );
      return MS_FAILURE;
    }
  } 
    
  /* define SQL query */
  /*apply_window = (table_name[0] != '(');*/ /* table isn´t a SELECT statement */
  for( i=0; i<layer->numitems; ++i )
    sprintf( query_str + strlen(query_str), ", %s", layer->items[i] );
  sprintf( query_str + strlen(query_str), ", %s FROM %s", geom_column_name, table_name );
  
  strcat( query_str, " WHERE " );  
  if (layer->filter.string != NULL) 
  {
    sprintf (query_str + strlen(query_str), "%s AND ", layer->filter.string);   
  } 

  switch (function)
  {
      case FUNCTION_FILTER:
      {
          sprintf( query_str + strlen(query_str),
            "SDO_FILTER( %s, MDSYS.SDO_GEOMETRY("
             "2003, %s, NULL,"
             "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
             "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g) ),"
             "'querytype=window') = 'TRUE'",
          geom_column_name, srid, rect.minx, rect.miny, rect.maxx, rect.maxy );
          break;
      }
      case FUNCTION_RELATE:
      {
          sprintf( query_str + strlen(query_str),
            "SDO_RELATE( %s, MDSYS.SDO_GEOMETRY("
             "2003, %s, NULL,"
             "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
             "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g) ),"
             "'mask=anyinteract querytype=window') = 'TRUE'",
          geom_column_name, srid, rect.minx, rect.miny, rect.maxx, rect.maxy);
          break;
      }
      case FUNCTION_GEOMRELATE:
      {
          sprintf( query_str + strlen(query_str),
            "SDO_GEOM.RELATE( %s, 'anyinteract', MDSYS.SDO_GEOMETRY("
             "2003, %s, NULL,"
             "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
             "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g)),"
             "%f) = 'TRUE' AND %s IS NOT NULL",
          geom_column_name, srid, rect.minx, rect.miny, rect.maxx, rect.maxy, TOLERANCE, geom_column_name );
          break;
      }
      default:
      {
          sprintf( query_str + strlen(query_str),
            "SDO_FILTER( %s, MDSYS.SDO_GEOMETRY("
             "2003, %s, NULL,"
             "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
             "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g) ),"
             "'querytype=window') = 'TRUE'",
          geom_column_name, srid, rect.minx, rect.miny, rect.maxx, rect.maxy );
      }
  }
     
  /* prepare */
  success = TRY( hand, OCIStmtPrepare( layerinfo->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) );
   
  if (success && layer->numitems >= 0){
    for( i=0; i <= layer->numitems && success; ++i )
    {
      success = TRY( hand, OCIDefineByPos( layerinfo->stmthp, &items[i], hand->errhp, (ub4)i+1, (dvoid *)layerinfo->items[i], (sb4)TEXT_SIZE, SQLT_STR, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) );  
    }
  } 

  if (success) {
    success = TRY( hand,
      /* define spatial position adtp ADT object */
      OCIDefineByPos( layerinfo->stmthp, &adtp, hand->errhp, (ub4)layer->numitems+2, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
    && TRY( hand,
      /* define object tdo from adtp */
      OCIDefineObject( adtp, hand->errhp, layerinfo->tdo, (dvoid **)layerinfo->obj, (ub4 *)0, (dvoid **)layerinfo->ind, (ub4 *)0 ) )
    && TRY( hand,
      /* execute */
      OCIStmtExecute( hand->svchp, layerinfo->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ) )
    &&  TRY( hand,
      /* get rows fetched */
      OCIAttrGet( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ) );
  }
    
  if (!success){
    msSetError( MS_ORACLESPATIALERR, 
      "Error: %s . "
      "Query statement: %s . "
      "Check your data statement.", 
      "msOracleSpatialLayerWhichShapes()", last_oci_call_ms_error, query_str );
    return MS_FAILURE;

  }
  /* should begin processing first row */
  layerinfo->row_num = layerinfo->row = 0;  
  
  return MS_SUCCESS;
}

/* fetch next shape from previous SELECT stmt (see *WhichShape()) */
int msOracleSpatialLayerNextShape( layerObj *layer, shapeObj *shape )
{
  SDOGeometryObj *obj;
  SDOGeometryInd *ind;
  int success, i;    
  
  /* get layerinfo */
  msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;      
  msOracleSpatialHandler *hand = layerinfo->orahandlers;
  
  if (layerinfo == NULL) {
    msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerWhichShapes called on unopened layer", "msOracleSpatialLayerWhichShapes()" );
    return MS_FAILURE;
  }

  /* at first, don't know what it is */
  /*msInitShape( shape );*/
  
  /* no rows fetched */
  if (layerinfo->rows_fetched == 0)
    return MS_DONE;    
  
 do 
 {     
    /* is buffer empty? */
    if (layerinfo->row_num >= layerinfo->rows_fetched) 
    {    
      /* fetch more */
      success = TRY( hand, OCIStmtFetch( layerinfo->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub2)OCI_FETCH_NEXT, (ub4)OCI_DEFAULT ) ) 
             && TRY( hand, OCIAttrGet( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ) );      

      if (!success || layerinfo->rows_fetched == 0)
        return MS_DONE;

      if (layerinfo->row_num >= layerinfo->rows_fetched)
        return MS_DONE;      
      layerinfo->row = 0; /* reset row index */ 
    }   
    
    /* set obj & ind for current row */
    obj = layerinfo->obj[ layerinfo->row ];
    ind = layerinfo->ind[ layerinfo->row ];
    
    /* get the items for the shape */        
    shape->index = atol( (layerinfo->items[0][ layerinfo->row ]));
     
    shape->numvalues = layer->numitems;
    shape->values = (char **)malloc( sizeof(char*) * shape->numvalues );
    
    for( i=0; i < shape->numvalues; ++i )
      shape->values[i] = strdup( layerinfo->items[i+1][ layerinfo->row ] );

    /* increment for next row */
    layerinfo->row_num++;
    layerinfo->row++;  
  
    /* fetch a layer->type object */   
    success = osGetOrdinates(layerinfo, shape, obj, ind);
    if (success != MS_SUCCESS)
      return MS_FAILURE;
  }while(shape->type == MS_SHAPE_NULL);
    
  return MS_SUCCESS;
}

int msOracleSpatialLayerGetItems( layerObj *layer )
{    
    char *rzt = "";
    char *flk = "";
    int function = 0;
    int existgeom;
    int	count_item, flk_len, success, i;
    char query_str[6000], table_name[2000], geom_column_name[100], unique[100], srid[100];
    OCIParam *pard = (OCIParam *) 0;
    
    msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *) layer->layerinfo;         
    msOracleSpatialHandler *hand = layerinfo->orahandlers;

    if (layer->debug)
      msDebug("msOracleSpatialLayerGetItems was called.\n");
        
    if (layerinfo == NULL){
      msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetItems called on unopened layer", "msOracleSpatialLayerGetItems()" );
      return MS_FAILURE;
    }
     
    if (!msSplitData(layer->data, geom_column_name, table_name, unique, srid, &function)){
      msSetError( MS_ORACLESPATIALERR, 
          "Error parsing OracleSpatial DATA variable. Must be: "
          "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
          "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
          "If want to set the FUNCTION statement you can use: FILTER, RELATE or GEOMRELATE. "
          "Your data statement: %s", 
          "msOracleSpatialLayerWhichShapes()", layer->data );  
      return MS_FAILURE;
    }    
     
    sprintf( query_str, "SELECT * FROM %s", table_name );
    success =  TRY( hand, OCIStmtPrepare( layerinfo->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DESCRIBE_ONLY) )
            && TRY( hand, OCIStmtExecute( hand->svchp, layerinfo->stmthp, hand->errhp, (ub4)QUERY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DESCRIBE_ONLY ) )
            && TRY( hand, OCIAttrGet( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layer->numitems, (ub4 *)0, OCI_ATTR_PARAM_COUNT, hand->errhp) );
                 
    if (!success){
      msSetError( MS_QUERYERR, "Cannot retrieve column list", "msOracleSpatialLayerGetItems()" );
      return MS_FAILURE;
    }   
          
    layerinfo->row_num = layerinfo->row = 0;
    
    layer->numitems = layer->numitems-1;  
    layer->items = malloc (sizeof(char *) * (layer->numitems));     
         
    if (layer->numitems > 0){
      layerinfo->items_query = (item_text_array_query *)malloc( sizeof(item_text_array_query) * (layer->numitems) );
      if (layerinfo->items_query == NULL){
        msSetError( MS_ORACLESPATIALERR,"Cannot allocate items buffer", "msOracleSpatialLayerGetItems()" );
        return MS_FAILURE;
      }
    }
    
    count_item = 0;
    existgeom = 0;
     
    /*Upcase conversion for the geom_column_name*/
    for (i=0; geom_column_name[i] != '\0'; i++)
        geom_column_name[i] = toupper(geom_column_name[i]);
     
    /*Retrive columns name from the user table*/
    for (i = 0; i <= layer->numitems; i++){   
        success = TRY( hand, OCIParamGet ((dvoid*)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT,hand->errhp,(dvoid*)&pard, (ub4)i+1))
               && TRY( hand, OCIAttrGet ((dvoid *) pard,(ub4) OCI_DTYPE_PARAM,(dvoid*)&rzt,(ub4 *)&flk_len, (ub4) OCI_ATTR_NAME, hand->errhp ));
        
        /*It's possible to use strdup????? - verify */
        flk = (char *)malloc(flk_len+1 );
        memcpy(flk, rzt, flk_len);
        flk[flk_len] = '\0';
         
        /*Comapre the column name (flk) with geom_column_name and ignore with true*/
        if (strcmp(flk, geom_column_name) != 0){
            layer->items[count_item] = malloc(flk_len);
            strcpy(layer->items[count_item], flk);
            count_item++;
        }
        else
           existgeom = 1;
         
        strcpy( rzt, "" );
        strcpy( flk, "" );
        flk_len = 0;
    }     
    if (!(existgeom)){
        msSetError (MS_ORACLESPATIALERR, "No geometry column, check stmt", "msOracleSpatialLayerGetItems()" );
        return MS_FAILURE;      
    }
     
    return msOracleSpatialLayerInitItemInfo( layer );  
}

int msOracleSpatialLayerGetShape( layerObj *layer, shapeObj *shape, long record )
{
    char query_str[6000], table_name[2000], geom_column_name[100], unique[100], srid[100];
    int success, i;
    int function = 0;
    OCIDefine *adtp = NULL, *items[QUERY_SIZE] = { NULL };
    SDOGeometryObj *obj = NULL;
    SDOGeometryInd *ind = NULL;    

    msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;    
    msOracleSpatialHandler *hand = layerinfo->orahandlers;
    
    if (layer->debug)
      msDebug("msOracleSpatialLayerGetShape was called. Using the record = %ld.\n", record);
    
    if (layerinfo == NULL){
        msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetShape called on unopened layer","msOracleSpatialLayerGetShape()" );
        return MS_FAILURE;
    }

    /* allocate enough space for items */
    if (layer->numitems > 0){
      layerinfo->items_query = (item_text_array_query *)malloc( sizeof(item_text_array_query) * (layer->numitems) );
      if (layerinfo->items_query == NULL){
        msSetError( MS_ORACLESPATIALERR, "Cannot allocate items buffer", "msOracleSpatialLayerWhichShapes()" );
        return MS_FAILURE;
      }
    }

    if (!msSplitData( layer->data, geom_column_name, table_name, unique, srid, &function )) 
    {
      msSetError( MS_ORACLESPATIALERR, 
           "Error parsing OracleSpatial DATA variable. Must be: "
           "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
           "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
           "If want to set the FUNCTION statement you can use: FILTER, RELATE or GEOMRELATE. "
           "Your data statement: %s", 
           "msOracleSpatialLayerWhichShapes()", layer->data );
       return MS_FAILURE;
    }

    /*Define the first query to retrive itens*/
    if (unique[0] == '\0'){
        msSetError( MS_ORACLESPATIALERR, "Error parsing OracleSpatial DATA variable for query. To execute "
                                         "query functions you need to define one "
                                         "unique column [USING UNIQUE <column>#]", 
                                         "msOracleSpatialLayerGetShape()" );
        return MS_FAILURE;
    }
    else
        sprintf( query_str, "SELECT ");

    /*Define the query*/     
    for( i = 0; i < layer->numitems; ++i )
        sprintf( query_str + strlen(query_str), " %s,", layer->items[i] );
        
    sprintf( query_str + strlen(query_str), " %s FROM %s WHERE %s = %ld", geom_column_name, table_name, unique, record);
    
    if (layer->filter.string != NULL)
        sprintf( query_str + strlen(query_str), " AND %s", (layer->filter.string));

    if (layer->debug)
      msDebug("msOracleSpatialLayerGetShape. Sql: %s\n", query_str);
    
    /*Prepare the handlers to the query*/
    success = TRY( hand,OCIStmtPrepare( layerinfo->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) );

    if (success && layer->numitems > 0){
        for( i = 0; i < layer->numitems && success; ++i ){
            success = TRY( hand, OCIDefineByPos( layerinfo->stmthp, &items[i], hand->errhp, (ub4)i+1, (dvoid *)layerinfo->items_query[i], (sb4)TEXT_SIZE, SQLT_STR, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) );
        }
    }
    if(!success){
      msSetError( MS_ORACLESPATIALERR, 
        "Error: %s . "
        "Query statement: %s . "
        "Check your data statement.", 
        "msOracleSpatialLayerGetShape()", last_oci_call_ms_error, query_str );      
      return MS_FAILURE;
    }
    if (success){
        success = TRY( hand, OCIDefineByPos( layerinfo->stmthp, &adtp, hand->errhp, (ub4)layer->numitems+1, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
               && TRY( hand, OCIDefineObject( adtp, hand->errhp, layerinfo->tdo, (dvoid **)layerinfo->obj, (ub4 *)0, (dvoid **)layerinfo->ind, (ub4 *)0 ) )
               && TRY (hand, OCIStmtExecute( hand->svchp, layerinfo->stmthp, hand->errhp, (ub4)QUERY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ))
               && TRY (hand, OCIAttrGet( (dvoid *)layerinfo->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ));

    }
    if(!success){
      msSetError( MS_ORACLESPATIALERR, 
        "Error: %s . "
        "Query statement: %s ."
        "Check your data statement.", 
        "msOracleSpatialLayerGetShape()", last_oci_call_ms_error, query_str );
      return MS_FAILURE;
    }
  
    /* at first, don't know what it is */
    /*msInitShape( shape );*/
    shape->type = MS_SHAPE_NULL;

    /* no rows fetched */ 
    if (layerinfo->rows_fetched == 0)
        return (MS_DONE);
    
    obj = layerinfo->obj[ layerinfo->row ];
    ind = layerinfo->ind[ layerinfo->row ];

    /* get the items for the shape */
    shape->numvalues = layer->numitems;
    shape->values = (char **) malloc(sizeof(char *) * layer->numitems);    
    shape->index = record;
    
    for( i = 0; i < layer->numitems; ++i )
        shape->values[i] = strdup( layerinfo->items_query[layerinfo->row][i] );        
    
    /* increment for next row */
    layerinfo->row_num++;
    layerinfo->row++;

    /* fetch a layer->type object */       
    success = osGetOrdinates(layerinfo, shape, obj, ind);    
    if (success != MS_SUCCESS){
        msSetError( MS_ORACLESPATIALERR, "Cannot execute query", "msOracleSpatialLayerGetShape()" );
        return MS_FAILURE;
    }
    osShapeBounds(shape);
    layerinfo->row = layerinfo->row_num = 0;
    return (MS_SUCCESS);
}

int msOracleSpatialLayerGetExtent(layerObj *layer, rectObj *extent)
{ 
  msSetError( MS_ORACLESPATIALERR, "Function not implemented yet", "msOracleSpatialLayerGetExtent()");
  return MS_FAILURE; 
}

int msOracleSpatialLayerInitItemInfo( layerObj *layer )
{
    int i;
    int *itemindexes ;
    
    if (layer->debug)
      msDebug("msOracleSpatialLayerInitItemInfo was called.\n");
    
    if (layer->numitems == 0)
      return MS_SUCCESS;

    if (layer->iteminfo)
      free( layer->iteminfo );
    
    if ((layer->iteminfo = (long *)malloc(sizeof(int)*layer->numitems))== NULL){
        msSetError(MS_MEMERR, NULL, "msOracleSpatialLayerInitItemInfo()");
        return MS_FAILURE;
    }
    
    itemindexes = (int*)layer->iteminfo;
    
    for(i=0; i < layer->numitems; i++)
        itemindexes[i] = i;  /*last one is always the geometry one - the rest are non-geom*/
  
    return MS_SUCCESS;
}

void msOracleSpatialLayerFreeItemInfo( layerObj *layer )
{
    if (layer->debug)
      msDebug("msOracleSpatialLayerFreeItemInfo was called.\n");
      
    if (layer->iteminfo)
        free(layer->iteminfo);
    layer->iteminfo = NULL;
    /* nothing to do */
}

int msOracleSpatialLayerGetAutoStyle( mapObj *map, layerObj *layer, classObj *c, int tile, long record )
{ 
  msSetError( MS_ORACLESPATIALERR, "Function not implemented yet", "msLayerGetAutoStyle()" );
  return MS_FAILURE; 
}

#else
/* OracleSpatial "not-supported" procedures */

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

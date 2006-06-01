/*****************************************************************************
 *             -- Oracle Spatial (SDO) support for MapServer --              *
 *                                                                           *
 *  Authors: Fernando Simon (fsimon@univali.br)                              *
 *           Rodrigo Becke Cabral (cabral@univali.br)                        *
 *  Collaborator: Adriana Gomes Alves                                        *
 *  MapServer: MapServer 4.8-rc1 (cvs)                                       *
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
 * Revision 1.29        $Date$   fernando
 * Bug fix: #1593
 *
 * Revision 1.28        2005/10/28 01:09:42 [CVS-TIME] jani
 * MS RFC 3: Layer vtable architecture (bug 1477) 
 *
 * Revision 1.27        2005/09/13          [CVS-TIME] fernando
 * Bug fix: #1343, #1442 and #1469.
 *
 * Revision 1.26        2005/04/28 22:19:41 [CVS-TIME] fernando
 * and Revision 1.25    2005/04/21 15:09:28 [CVS-TIME] julien
 * Bug fix: #1244
 *
 * Revision 1.22
 * and Revision 1.21    2005/02/21 14:08:43 [CVS-TIME] fernando
 * Added the support for 3D Data.
 * Bug fix: #1144
 *
 * Revision 1.20        2005/02/14 19:42:44 [CVS-TIME] fernando
 * and Revision 1.19    2005/02/10 20:27:03 [CVS-TIME]
 * Bug fix: #1109.
 *
 * Revision 1.18        2005/02/04 13:10:46 [CVS-TIME] fernando
 * Bug fix: #1109, #1110, #1111, #1112, #1136,
 *           #1210, #1211, #1212, #1213.
 * Cleaned code.
 * Added debug messages for internal SQL's.
 *
 * Revision 1.15        2004/12/20 14:01:10 [CVS-TIME] fernando
 * Fixed problem with LayerClose function.
 * Added token NONE for DATA statement.
 *
 * Revision 1.14        2004/11/15 20:35:02 [CVS-TIME] dan
 * Added msLayerIsOpen() to all vector layer types (bug 1051)
 *
 * Revision 1.13        2004/11/15 20:35:02 [CVS-TIME] fernando
 * Fixed declarations problems - Bug #1044
 *
 * Revision 1.12        2004/11/05 20:33:14 [CVS-TIME] fernando
 * Added debug messages.
 *
 * Revision 1.11        2004/10/30 05:15:15 [CVS-TIME] fernando
 * Connection Pool support.
 * New query item support.
 * New improve of performance.
 * Updated mapfile DATA statement to include more options.
 * Bug fix from version 1.9
 *
 * Revision 1.10        2004/10/21 04:30:54 [CVS-TIME] frank
 * MS_CVSID support added.
 *
 * Revision 1.9         2004/04/30 13:27:46 [CVS-TIME] fernando
 * Query item support implemented.
 * Query map is not being generated yet.
 *
 * Revision 1.8         2003/03/18 04:56:28 [CVS-TIME] rodrigo
 * Updated mapfile DATA statement to include SRID number.
 * SRID fix in r1.6 proved to be inefficient and time-consuming.
 *
 * Revision 1.7         2002/01/19 18:29:25 [CVS-TIME] rodrigo
 * Fixed bug in SQL statement when using "FILTER" in mapfile.
 *
 * Revision 1.6         2001/12/22 18:32:02 [CVS-TIME] rodrigo
 * Fixed SRID mismatch error.
 *
 * Revision 1.5         2001/11/21 22:35:58 [CVS-TIME] rodrigo
 * Added support for 2D circle geometries (gtype/etype/interpretation):
 * - (2003/[?00]3/4)
 *
 * Revision 1.4         2001/10/22 17:09:03 [CVS-TIME] rodrigo
 * Added support for mapfile items.
 *
 * Revision 1.3         2001/10/18 11:39:04 [CVS-TIME] rodrigo
 * Added support for the following 2D geometries:
 * - 2001/1/1         point
 * - 2001/1/n         multipoint
 * - 2002/2/1         line string
 *
 * Revision 1.2         2001/09/28 10:42:29 [GMT-03:00] rodrigo
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
 * - DATA 'geometry_column FROM <table>'
 *   or
 *   DATA 'geometry_column FROM <table> [USING UNIQUE <column>' SRID #srid <function> VERSION <vcode>]'
 *       <function> can be:
 *           'FILTER', 'RELATE', GEOMRELATE' or 'NONE'
 *       <vcode> can be:
 *           '8i', '9i' or '10g'
 *
 *       <table> can be:
 *            One database table name
 *       or:
 *            (SELECT stmt)
 * - Parts of the CONNECTION string may be encrypted, see MS-RFC-18
 *
 *****************************************************************************
 * Notices above shall be included in all copies or portions of the software.
 * This piece is provided "AS IS", without warranties of any kind. Got it?
 *****************************************************************************/

#include "map.h"
#include <assert.h>

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
#define FUNCTION_FILTER            1
#define FUNCTION_RELATE            2
#define FUNCTION_GEOMRELATE        3
#define FUNCTION_NONE              4
#define VERSION_8i                 1
#define VERSION_9i                 2
#define VERSION_10g                3
#define TOLERANCE                  0.001
#define NULLERRCODE                1405

typedef
    struct
    {
        OCINumber x;
        OCINumber y;
        OCINumber z;
    } SDOPointObj;

typedef
    struct
    {
        OCINumber gtype;
        OCINumber srid;
        SDOPointObj point;
        OCIArray *elem_info;
        OCIArray *ordinates;
    } SDOGeometryObj;

typedef
    struct
    {
        OCIInd _atomic;
        OCIInd x;
        OCIInd y;
        OCIInd z;
    } SDOPointInd;

typedef
    struct
    {
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
    struct
    {
        /*Oracle handlers*/
        OCIEnv *envhp;
        OCIError *errhp;
        OCISvcCtx *svchp;
        int last_oci_status;
        text last_oci_error[2048];
    } msOracleSpatialHandler;

typedef
    struct
    {
        /* Oracle data handlers */
        OCIStmt *stmthp;
        OCIDescribe *dschp;
        OCIType *tdo;
    } msOracleSpatialDataHandler;

typedef
    struct
    {
        /* oracle handlers */
        msOracleSpatialHandler *orahandlers;

        /* oracle data handlers */
        msOracleSpatialDataHandler *oradatahandlers;

        /* fetch data */
        int rows_fetched;
        int row_num;
        int row;
        item_text_array *items; /* items buffer */
        item_text_array_query *items_query; /* items buffer */
        SDOGeometryObj *obj[ARRAY_SIZE]; /* spatial object buffer */
        SDOGeometryInd *ind[ARRAY_SIZE]; /* object indicator */
    } msOracleSpatialLayerInfo;

/* local prototypes */
static int TRY( msOracleSpatialHandler *hand, sword status );
static int ERROR( char *routine, msOracleSpatialHandler *hand, msOracleSpatialDataHandler *dthand );
static void msSplitLogin( char *connection, mapObj *map, char *username, char *password, char *dblink );
static int msSplitData( char *data, char *geometry_column_name, char *table_name, char* unique, char *srid, int *function, int * version);
static void msOCICloseConnection( void *layerinfo );
static msOracleSpatialHandler *msOCISetHandlers( char *username, char *password, char *dblink );
static int msOCISetDataHandlers( msOracleSpatialHandler *hand, msOracleSpatialDataHandler *dthand );
static void msOCICloseDataHandlers ( msOracleSpatialDataHandler *dthand );
static void msOCICloseHandlers( msOracleSpatialHandler *hand );
static void msOCIClearLayerInfo( msOracleSpatialLayerInfo *layerinfo );
static int msOCIGet2DOrdinates( msOracleSpatialHandler *hand, SDOGeometryObj *obj, int s, int e, pointObj *pt );
static int msOCIGet3DOrdinates( msOracleSpatialHandler *hand, SDOGeometryObj *obj, int s, int e, pointObj *pt );
static int msOCIConvertCircle( pointObj *pt );
static void osAggrGetExtent(layerObj *layer, char *query_str, char *geom_column_name, char *table_name);
static void osConvexHullGetExtent(layerObj *layer, char *query_str, char *geom_column_name, char *table_name);
static void osGeodeticData(int function, int version, char *query_str, char *geom_column_name, char *srid, rectObj rect);
static void osNoGeodeticData(int function, int version, char *query_str, char *geom_column_name, char *srid, rectObj rect);
static double osCalculateArcRadius(pointObj *pnt);
static void osCalculateArc(pointObj *pnt, int data3d, double area, double radius, double npoints, int side, lineObj arcline, shapeObj *shape);
static void osGenerateArc(shapeObj *shape, lineObj arcline, lineObj points, int i, int n, int data3d);
static void osShapeBounds ( shapeObj *shp );
static void osCloneShape(shapeObj *shape, shapeObj *newshape, int data3d);
static void osPointCluster(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int interpretation, int data3d);
static void osPoint(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d);
static void osClosedPolygon(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int elem_type, int data3d);
static void osRectangle(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d);
static void osCircle(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d);
static void osArcPolygon(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj arcpoints, int data3d);
static int osGetOrdinates(msOracleSpatialDataHandler *dthand, msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, SDOGeometryInd *ind);
static int osCheck2DGtype(int pIntGtype);
static int osCheck3DGtype(int pIntGtype);

/* if an error ocurred call msSetError, sets last_oci_status to MS_FAILURE and return 0;
 * otherwise returns 1 */
static int TRY( msOracleSpatialHandler *hand, sword status )
{
    sb4 errcode = 0;

    if (hand->last_oci_status == MS_FAILURE)
        return 0; /* error from previous call */

    switch (status)
    {
        case OCI_ERROR:
            OCIErrorGet((dvoid *)hand->errhp, (ub4)1, (text *)NULL, &errcode, hand->last_oci_error, (ub4)sizeof(hand->last_oci_error), OCI_HTYPE_ERROR );
            if (errcode == NULLERRCODE)
            {
                hand->last_oci_error[0] = (text)'\0';
                return 1;
            }
            hand->last_oci_error[sizeof(hand->last_oci_error)-1] = 0; /* terminate string!? */
            break;
        case OCI_NEED_DATA:
            strcpy( hand->last_oci_error, "OCI_NEED_DATA" );
            break;
        case OCI_INVALID_HANDLE:
            strcpy( hand->last_oci_error, "OCI_INVALID_HANDLE" );
            break;
        case OCI_STILL_EXECUTING:
            strcpy( hand->last_oci_error, "OCI_STILL_EXECUTING");
            break;
        case OCI_CONTINUE:
            strcpy( hand->last_oci_error, "OCI_CONTINUE" );
            break;
        default:
            return 1; /* no error */
    }

    /* if I got here, there was an error */
    hand->last_oci_status = MS_FAILURE;

    return 0; /* error! */
}

/* check last_oci_status for MS_FAILURE (set by TRY()) if an error ocurred return 1;
 * otherwise, returns 0 */
static int ERROR( char *routine, msOracleSpatialHandler *hand, msOracleSpatialDataHandler *dthand )
{
    if (hand->last_oci_status == MS_FAILURE) 
    {
        /* there was an error */
        msSetError( MS_ORACLESPATIALERR, hand->last_oci_error, routine );
        return 1; /* error processed */
     }
     else
         return 0; /* no error */
}

/* break layer->connection (username/password@dblink) into username, password and dblink */
static void msSplitLogin( char *connection, mapObj *map, char *username, char *password, char *dblink )
{
    char *src, *tgt, *conn_decrypted;
    /* clearup */
    *username = *password = *dblink = 0;
    /* bad 'connection' */
    if (connection == NULL) return;

    /* Decrypt any encrypted token */
    conn_decrypted = msDecryptStringTokens(map, connection);
    if (conn_decrypted == NULL) return;

    /* ok, split connection */
    for( tgt=username, src=conn_decrypted; *src; src++, tgt++ )
        if (*src=='/' || *src=='@')
            break;
        else
            *tgt = *src;
    *tgt = 0;
    if (*src == '/')
    {
        for( tgt=password, ++src; *src; src++, tgt++ )
            if (*src == '@')
                break;
            else
                *tgt = *src;
        *tgt = 0;
    }
    if (*src == '@')
        strcpy( dblink, ++src );

    msFree(conn_decrypted);
}

/* break layer->data into geometry_column_name, table_name and srid */
static int msSplitData( char *data, char *geometry_column_name, char *table_name, char *unique, char *srid, int *function, int *version )
{
    char *tok_from = "from";
    char *tok_using = "using";
    char *tok_unique = "unique";
    char *tok_srid = "srid";
    char *tok_version = "version";
    char data_version[3] = "";
    char tok_function[11] = "";
    int parenthesis, i;
    char *src = data, *tgt;

    /* clearup */
    *geometry_column_name = *table_name = 0;

    /* bad 'data' */
    if (data == NULL)
        return 0;

    /* parsing 'geometry_column_name' */
    for( ;*src && isspace( *src ); src++ ); /* skip blanks */
    for( tgt=geometry_column_name; *src; src++, tgt++ )
        if (isspace( *src ))
            break;
        else
            *tgt = *src;
    *tgt = 0;

    /* parsing 'from' */
    for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
    for( ;*src && *tok_from && tolower(*src)==*tok_from; src++, tok_from++ );
    if (*tok_from != '\0')
        return 0;

    /* parsing 'table_name' or '(SELECT stmt)' */
    for( ;*src && isspace( *src ); src++ ); /* skip blanks */
    for( tgt=table_name, parenthesis=0; *src; src++, tgt++ )
    {
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
    *function = -1;
    *version = -1;

    /* parsing 'unique' */
    for( ;*src && isspace( *src ); src++ ) ; /* skip blanks */
    if (*src != '\0')
    {
        /* parse 'using' */
        for( ;*src && *tok_using && tolower(*src)==*tok_using; src++, tok_using++ );
        if (*tok_using != '\0') 
            return 0;

        /* parsing 'unique' */
        for( ;*src && isspace( *src ); src++ ); /* skip blanks */
        for( ;*src && *tok_unique && tolower(*src)==*tok_unique; src++, tok_unique++ );

        if (*tok_unique == '\0')
        {
            for( ;*src && isspace( *src ); src++ ); /* skip blanks */
            if (*src == '\0') 
                return 0;
            for( tgt=unique; *src; src++, tgt++ )
                if (isspace( *src )) 
                    break;
                else 
                    *tgt = *src;
            *tgt = 0;

            if (*tok_unique != '\0')
                return 0;
        }

         /* parsing 'srid' */
        for( ;*src && isspace( *src ); src++ ); /* skip blanks */
        for( ;*src && *tok_srid && tolower(*src)==*tok_srid; src++, tok_srid++ );
        if (*tok_srid == '\0')
        {
            for( ;*src && isspace( *src ); src++ ); /* skip blanks */
            if (*src == '\0')
                 return 0;
            for( tgt=srid; *src; src++, tgt++ )
                if (isspace( *src ))
                    break;
                else 
                    *tgt = *src;
            *tgt = 0;

            if (*tok_srid != '\0')
                return 0;
        }

        /*parsing function/version */
        for( ;*src && isspace( *src ); src++ );
        if (*src != '\0')
        {
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

        if (strcmp(tok_function, "VERSION"))
        {
            if (!strcmp(tok_function, "FILTER") || !strcmp(tok_function, ""))
                *function = FUNCTION_FILTER;
            else if(!strcmp(tok_function, "RELATE"))
                *function = FUNCTION_RELATE;
            else if (!strcmp(tok_function,"GEOMRELATE"))
                *function = FUNCTION_GEOMRELATE;
            else if (!strcmp(tok_function,"NONE"))
                *function = FUNCTION_NONE;
            else
            {
                *function = -1;
                return 0;
            }

            /*parsing VERSION token when user defined one function*/
            for( ;*src && isspace( *src ); src++ );
            for( ;*src && *tok_version && tolower(*src)==*tok_version; src++, tok_version++ );
        }
        else
        {
            for(tgt = "VERSION";*tgt && *tok_version && toupper(*tgt)==toupper(*tok_version); tgt++, tok_version++ );
            *function = FUNCTION_FILTER;
        }

        /*parsing version*/
        if (*tok_version == '\0')
        {
            for( ;*src && isspace( *src ); src++ ); /* skip blanks */
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
    for( ;*src && isspace( *src ); src++ ); /* skip blanks */

    return (*src == '\0');
}

static int msOCISetDataHandlers(msOracleSpatialHandler *hand, msOracleSpatialDataHandler *dthand)
{
    int success = 0;
    OCIParam *paramp = NULL;
    OCIRef *type_ref = NULL;

    success = TRY( hand,
        /* allocate stmthp */
        OCIHandleAlloc( (dvoid *)hand->envhp, (dvoid **)&dthand->stmthp, (ub4)OCI_HTYPE_STMT, (size_t)0, (dvoid **)0 ) )
           && TRY( hand,
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

    msOracleSpatialHandler *hand = (msOracleSpatialHandler *) malloc( sizeof(msOracleSpatialHandler));
    memset( hand, 0, sizeof(msOracleSpatialHandler) );

    hand->last_oci_status = MS_SUCCESS;
    hand->last_oci_error[0] = (text)'\0';

    success = TRY( hand,
        /* allocate envhp */
        OCIEnvCreate( &hand->envhp, OCI_OBJECT, (dvoid *)0, 0, 0, 0, (size_t) 0, (dvoid **)0 ) )
           && TRY( hand,
        /* allocate errhp */
        OCIHandleAlloc( (dvoid *)hand->envhp, (dvoid **)&hand->errhp, (ub4)OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0 ) )
           && TRY( hand,
        /* logon */
        OCILogon( hand->envhp, hand->errhp, &hand->svchp, username, strlen(username), password, strlen(password), dblink, strlen(dblink) ) );

    if ( !success )
    {
        msSetError( MS_ORACLESPATIALERR,
                        "Cannot create OCI Handlers. "
                        "Connection failure. Check the connection string. "
                        "Error: %s.",
                        "msOracleSpatialLayerOpen()", hand->last_oci_error);

        msOCICloseHandlers(hand);
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
    if (hand != NULL)
        memset( hand, 0, sizeof (msOracleSpatialHandler));
    free(hand);
}

static void msOCICloseDataHandlers( msOracleSpatialDataHandler *dthand )
{
    if (dthand->dschp != NULL)
        OCIHandleFree( (dvoid *)dthand->dschp, (ub4)OCI_HTYPE_DESCRIBE );
    if (dthand->stmthp != NULL)
        OCIHandleFree( (dvoid *)dthand->stmthp, (ub4)OCI_HTYPE_STMT );
    if (dthand != NULL)
        memset( dthand, 0, sizeof (msOracleSpatialDataHandler));
    free(dthand);
}

static void msOCIClearLayerInfo( msOracleSpatialLayerInfo *layerinfo )
{
    if (layerinfo->items != NULL)
        free( layerinfo->items );
    if (layerinfo->items_query != NULL)
        free( layerinfo->items_query );
    if (layerinfo != NULL)
        memset( layerinfo, 0, sizeof( msOracleSpatialLayerInfo ) );
    free(layerinfo);
}

/*function taht creates the correct sql for geoditical srid for version 9i*/
static void osGeodeticData(int function, int version, char *query_str, char *geom_column_name, char *srid, rectObj rect)
{
    switch (function)
    {
        case FUNCTION_FILTER:
        {
            sprintf( query_str + strlen(query_str),
                     "SDO_FILTER( %s, SDO_CS.VIEWPORT_TRANSFORM(MDSYS.SDO_GEOMETRY("
                     "2003, 0, NULL,"
                     "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                     "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g) ), %s),"
                     "'querytype=window') = 'TRUE'",
                     geom_column_name, rect.minx, rect.miny, rect.maxx, rect.maxy, srid );
            break;
        }
        case FUNCTION_RELATE:
        {
            sprintf( query_str + strlen(query_str),
                     "SDO_RELATE( %s, SDO_CS.VIEWPORT_TRANSFORM(MDSYS.SDO_GEOMETRY("
                     "2003, 0, NULL,"
                     "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                     "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g) ), %s),"
                     "'mask=anyinteract querytype=window') = 'TRUE'",
                     geom_column_name, rect.minx, rect.miny, rect.maxx, rect.maxy, srid);
            break;
        }
        case FUNCTION_GEOMRELATE:
        {
            sprintf( query_str + strlen(query_str),
                     "SDO_GEOM.RELATE( %s, 'anyinteract', SDO_CS.VIEWPORT_TRANSFORM(MDSYS.SDO_GEOMETRY("
                     "2003, 0, NULL,"
                     "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                     "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g)), %s),"
                     "%f) = 'TRUE' AND %s IS NOT NULL",
                     geom_column_name, rect.minx, rect.miny, rect.maxx, rect.maxy, srid, TOLERANCE, geom_column_name );
            break;
        }
        case FUNCTION_NONE:
        {
            break;
        }
        default:
        {
            sprintf( query_str + strlen(query_str),
                     "SDO_FILTER( %s, SDO_CS.VIEWPORT_TRANSFORM(MDSYS.SDO_GEOMETRY("
                     "2003, 0, NULL,"
                     "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                     "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g) ), %s),"
                     "'querytype=window') = 'TRUE'",
                     geom_column_name, rect.minx, rect.miny, rect.maxx, rect.maxy, srid );
        }
    }
}

/*function that generate the correct sql for no geoditic srid's*/
static void osNoGeodeticData(int function, int version, char *query_str, char *geom_column_name, char *srid, rectObj rect)
{
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
            if (version == VERSION_10g)
            {
                sprintf( query_str + strlen(query_str),
                         "SDO_ANYINTERACT( %s, MDSYS.SDO_GEOMETRY("
                         "2003, %s, NULL,"
                         "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                         "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g))) = 'TRUE'",
                         geom_column_name, srid, rect.minx, rect.miny, rect.maxx, rect.maxy);
            }
            else
            {
                sprintf( query_str + strlen(query_str),
                         "SDO_RELATE( %s, MDSYS.SDO_GEOMETRY("
                         "2003, %s, NULL,"
                         "MDSYS.SDO_ELEM_INFO_ARRAY(1,1003,3),"
                         "MDSYS.SDO_ORDINATE_ARRAY(%.9g,%.9g,%.9g,%.9g) ),"
                         "'mask=anyinteract querytype=window') = 'TRUE'",
                         geom_column_name, srid, rect.minx, rect.miny, rect.maxx, rect.maxy);
            }
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
        case FUNCTION_NONE:
        {
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
}

/* get ordinates from SDO buffer */
static int msOCIGet2DOrdinates( msOracleSpatialHandler *hand, SDOGeometryObj *obj, int s, int e, pointObj *pt )
{
    double x, y;
    int i, n, success = 1;
    boolean exists;
    OCINumber *oci_number;

    for( i=s, n=0; i < e && success; i+=2, n++ ) 
    {
        success = TRY( hand,
          OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
               && TRY( hand,
          OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&x ) )
               && TRY( hand,
          OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i+1, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
               && TRY( hand,
          OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&y ) );

        if (success) 
        {
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
#ifdef USE_POINT_Z_M
    double z;
    boolean numnull;
#endif /* USE_POINT_Z_M */
    OCINumber *oci_number;

    for( i=s, n=0; i < e && success; i+=3, n++ ) 
    {
        success = TRY( hand,
          OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
               && TRY( hand,
          OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&x ) )
               && TRY( hand,
          OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i+1, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
               && TRY( hand,
          OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&y ) )
#ifdef USE_POINT_Z_M
               && TRY( hand,
          OCICollGetElem( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, (sb4)i+2, (boolean *)&exists, (dvoid *)&oci_number, (dvoid **)0 ) )
#endif /* USE_POINT_Z_M */
            ;
#ifdef USE_POINT_Z_M
        if (success)
        {
            success = TRY(hand, OCINumberIsZero( hand->errhp, oci_number, (boolean *)&numnull));
            if (success)
            {
                success = TRY( hand, OCINumberToReal( hand->errhp, oci_number, (uword)sizeof(double), (dvoid *)&z ) );
            }
            else
            {
                hand->last_oci_status = MS_SUCCESS;
                strcpy( hand->last_oci_error, "Retrieve z value, but NULL value for z. Setting z to 0." );
                z = 0;
                success = 1;
            }
        }
#endif /* USE_POINT_Z_M */
        if (success) 
        {
            pt[n].x = x;
            pt[n].y = y;
#ifdef USE_POINT_Z_M
            pt[n].z = z;
#endif /* USE_POINT_Z_M */
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
    if (!success)
    { /* switch points 1 & 2 */
        ptaux = pt[1];
        pt[1] = pt[2];
        pt[2] = ptaux;
        dXa = pt[1].x - pt[0].x;
        success = (fabs( dXa ) > 1e-8);
    }
    if (success)
    {
        dXb = pt[2].x - pt[1].x;
        success = (fabs( dXb ) > 1e-8);
        if (!success)
        { /* insert point 2 before point 0 */
            ptaux = pt[2];
            pt[2] = pt[1];
            pt[1] = pt[0];
            pt[0] = ptaux;
            dXb = dXa; /* segment A has become B */
            dXa = pt[1].x - pt[0].x; /* recalculate new segment A */
            success = (fabs( dXa ) > 1e-8);
        }
    }
    if (success)
    {
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

static void osAggrGetExtent(layerObj *layer, char *query_str, char *geom_column_name, char *table_name)
{
    char query_str2[6000];
    int i = 0;

    sprintf( query_str2, "(SELECT");
    for( i = 0; i < layer->numitems; ++i )
        sprintf( query_str2 + strlen(query_str2), " %s,", layer->items[i] );

    sprintf( query_str2 + strlen(query_str2), " %s FROM %s", geom_column_name, table_name);
    if (layer->filter.string != NULL)
        sprintf( query_str2 + strlen(query_str2), " WHERE %s", (layer->filter.string));

    sprintf( query_str, "SELECT SDO_AGGR_MBR(%s) AS GEOM from %s)", geom_column_name, query_str2);
}

static void osConvexHullGetExtent(layerObj *layer, char *query_str, char *geom_column_name, char *table_name)
{
    char query_str2[6000];
    int i = 0;

    sprintf( query_str2, "(SELECT");
    for( i = 0; i < layer->numitems; ++i )
        sprintf( query_str2 + strlen(query_str2), " %s,", layer->items[i] );

    sprintf( query_str2 + strlen(query_str2), " %s FROM %s", geom_column_name, table_name);
    if (layer->filter.string != NULL)
        sprintf( query_str2 + strlen(query_str2), " WHERE %s", (layer->filter.string));

    sprintf( query_str, "SELECT SDO_GEOM.SDO_CONVEXHULL(%s, %f) AS GEOM from %s)", geom_column_name, TOLERANCE, query_str2);
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

static void osCalculateArc(pointObj *pnt, int data3d, double area, double radius, double npoints, int side, lineObj arcline, shapeObj *shape)
{
    double length, ctrl, angle;  
    double divbas, plusbas, cosbas, sinbas = 0;
#ifdef USE_POINT_Z_M
    double zrange = 0;
#endif /* USE_POINT_Z_M */
    int i = 0;

    if ( npoints > 0 )
    {
        length = sqrt(((pnt[1].x-pnt[0].x)*(pnt[1].x-pnt[0].x))+((pnt[1].y-pnt[0].y)*(pnt[1].y-pnt[0].y)));
        ctrl = length/(2*radius);

#ifdef USE_POINT_Z_M
        if (data3d)
        {
            zrange = labs(pnt[0].z-pnt[1].z)/npoints;
            if ((pnt[0].z > pnt[1].z) && side == 1)
                zrange *= -1;
            else if ((pnt[0].z < pnt[1].z) && side == 1)
                    zrange = zrange;
            else if ((pnt[0].z > pnt[1].z) && side != 1)
                    zrange *= -1;
            else if ((pnt[0].z < pnt[1].z) && side != 1)
                    zrange = zrange;
        }
#endif /* USE_POINT_Z_M */

        if( ctrl <= 1 )
        {
            divbas = 2 * asin(ctrl);
            plusbas = divbas/(npoints);
            cosbas = (pnt[0].x-pnt[3].x)/radius;
            sinbas = (pnt[0].y-pnt[3].y)/radius; 
            angle = plusbas;

            arcline.point = (pointObj *)malloc(sizeof(pointObj)*(npoints+1));
            arcline.point[0].x = pnt[0].x;
            arcline.point[0].y = pnt[0].y;
#ifdef USE_POINT_Z_M
            if (data3d)
                arcline.point[0].z = pnt[0].z;
#endif /* USE_POINT_Z_M */

            for (i = 1; i <= npoints; i++)
            {
                if( side == 1)
                {
                    arcline.point[i].x = pnt[3].x + radius * ((cosbas*cos(angle))-(sinbas*sin(angle)));
                    arcline.point[i].y = pnt[3].y + radius * ((sinbas*cos(angle))+(cosbas*sin(angle)));
#ifdef USE_POINT_Z_M
                    if (data3d)
                        arcline.point[i].z = pnt[0].z + (zrange*i);
#endif /* USE_POINT_Z_M */
                }
                else
                {
                    if ( side == -1)
                    {
                        arcline.point[i].x = pnt[3].x + radius * ((cosbas*cos(angle))+(sinbas*sin(angle)));
                        arcline.point[i].y = pnt[3].y + radius * ((sinbas*cos(angle))-(cosbas*sin(angle)));
#ifdef USE_POINT_Z_M
                        if (data3d)
                            arcline.point[i].z = pnt[0].z + (zrange*i);
#endif /* USE_POINT_Z_M */
                    }
                    else
                    {
                         arcline.point[i].x = pnt[0].x;
                         arcline.point[i].y = pnt[0].y;
#ifdef USE_POINT_Z_M
                         if (data3d)
                             arcline.point[i].z = pnt[0].z;
#endif /* USE_POINT_Z_M */
                    }
                }
                angle += plusbas;
            }

            arcline.numpoints = npoints+1;
            msAddLine( shape, &arcline );
            free (arcline.point);
        }
    }
    else
    {
        arcline.point = (pointObj *)malloc(sizeof(pointObj)*(2));
        arcline.point[0].x = pnt[0].x;
        arcline.point[0].y = pnt[0].y;
        arcline.point[1].x = pnt[1].x;
        arcline.point[1].y = pnt[1].y;

#ifdef USE_POINT_Z_M
        if(data3d)
        {
            arcline.point[0].z = pnt[0].z;
            arcline.point[1].z = pnt[1].z;
        }
#endif /* USE_POINT_Z_M */

        arcline.numpoints = 2;

        msAddLine( shape, &arcline );
        free (arcline.point);
    }
}

/* Part of this function was based on Terralib function TeGenerateArc
 * found in TeGeometryAlgorith.cpp (www.terralib.org).
 * Part of this function was based on Dr. Ialo (Univali/Cttmar) functions. */
static void osGenerateArc(shapeObj *shape, lineObj arcline, lineObj points, int i, int n, int data3d)
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

    if (mult)
    {
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
        else
        {
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

        npoints = labs(area/radius);

        point5[0] = points.point[i];
        point5[1] = points.point[i+1];
        osCalculateArc(point5, data3d, area, radius, (npoints>1000)?1000:npoints, side, arcline, shape);

        point5[0] = points.point[i+1];
        point5[1] = points.point[i+2];
        osCalculateArc(point5, data3d, area, radius, (npoints>1000)?1000:npoints, side, arcline, shape);

    }
    else
    {
        arcline.point = (pointObj *)malloc(sizeof(pointObj)*(2));
        arcline.point[0].x = points.point[i].x; 
        arcline.point[0].y = points.point[i].y;
        arcline.point[1].x = points.point[i+2].x;
        arcline.point[1].y = points.point[i+2].y;

#ifdef USE_POINT_Z_M
        if (data3d)
        {
            arcline.point[0].z = points.point[i].z;
            arcline.point[1].z = points.point[i+2].z;
        }
#endif /* USE_POINT_Z_M */

        arcline.numpoints = 2;

        msAddLine( shape, &arcline );
        free (arcline.point);
    }
}

static void osShapeBounds ( shapeObj *shp )
{
    int i, f;

    if ( (shp->numlines > 0) && (shp->line[0].numpoints > 0) )
    {
        shp->bounds.minx = shp->line[0].point[0].x;
        shp->bounds.maxx = shp->line[0].point[0].x;
        shp->bounds.miny = shp->line[0].point[0].y;
        shp->bounds.maxy = shp->line[0].point[0].y;
    }

    for ( i = 0 ; i < shp->numlines; i++)
    {
        for( f = 0; f < shp->line[i].numpoints; f++)
        {
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

static void osCloneShape(shapeObj *shape, shapeObj *newshape, int data3d)
{
    int max_points = 0;
    int i,f,g;
    lineObj shapeline = {0, NULL};

    for (i = 0; i < shape->numlines; i++)
        max_points += shape->line[i].numpoints;

    if (max_points > 0)
        shapeline.point = (pointObj *)malloc( sizeof(pointObj)*max_points );

    g = 0;
    for ( i = 0 ; i < shape->numlines; i++)
    {
        for( f = 0; f < shape->line[i].numpoints && g <= max_points; f++, g++)
        {
            shapeline.point[g].x = shape->line[i].point[f].x;
            shapeline.point[g].y = shape->line[i].point[f].y;
#ifdef USE_POINT_Z_M
            if (data3d)
                shapeline.point[g].z = shape->line[i].point[f].z;
#endif /* USE_POINT_Z_M */
        }
    }

    if (g)
    {
        shapeline.numpoints = g;
        newshape->type = MS_SHAPE_POLYGON;
        msAddLine( newshape, &shapeline );
    }
}

static void osPointCluster(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int interpretation, int data3d)
{
    int n;

    n = (end - start)/2;
    if (n == interpretation)
    {
        points.point = (pointObj *)malloc( sizeof(pointObj)*n );

        if (data3d)
            n = msOCIGet3DOrdinates( hand, obj, start, end, points.point );
        else
            n = msOCIGet2DOrdinates( hand, obj, start, end, points.point );

        if (n == interpretation && n>0)
        {
            shape->type = MS_SHAPE_POINT;
            points.numpoints = n;
            msAddLine( shape, &points );
        }
        free( points.point );
    }
}

static void osPoint(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d)
{
    int n;

    if (data3d)
        n = msOCIGet3DOrdinates( hand, obj, start, end, pnt );
    else
        n = msOCIGet2DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */

    if (n == 1)
    {
        shape->type = MS_SHAPE_POINT;
        points.numpoints = 1;
        points.point = pnt;
        msAddLine( shape, &points );
    }
}

static void osClosedPolygon(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, int elem_type, int data3d)
{
    int n;

    n = (end - start)/2;
    points.point = (pointObj *)malloc( sizeof(pointObj)*n );

    if (data3d)
        n = msOCIGet3DOrdinates( hand, obj, start, end, points.point );
    else
        n = msOCIGet2DOrdinates( hand, obj, start, end, points.point );

    if (n > 0)
    {
        shape->type = (elem_type==21) ? MS_SHAPE_LINE : MS_SHAPE_POLYGON;
        points.numpoints = n;
        msAddLine( shape, &points );
    }
    free( points.point );
}

static void osRectangle(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d)
{
    int n;

    if (data3d)
        n = msOCIGet3DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */
    else
        n = msOCIGet2DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */

    if (n == 2)
    {
        shape->type = MS_SHAPE_POLYGON;
        points.numpoints = 5;
        points.point = pnt;

        /* point5 [0] & [1] contains the lower-left and upper-right points of the rectangle */
        pnt[2] = pnt[1];
        pnt[1].x = pnt[0].x;
        pnt[3].x = pnt[2].x;
        pnt[3].y = pnt[0].y;
        pnt[4] = pnt[0]; 
#ifdef USE_POINT_Z_M
        if (data3d)
        {
            pnt[1].z = pnt[0].z;
            pnt[3].z = pnt[2].z;
        }
#endif /* USE_POINT_Z_M */

        msAddLine( shape, &points );
    }
}

static void osCircle(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj points, pointObj *pnt, int data3d)
{
    int n;

    if (data3d)
        n = msOCIGet3DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */
    else
        n = msOCIGet2DOrdinates( hand, obj, start, end, pnt ); /* n must be < 5 */

    if (n == 3)
    {
        if (msOCIConvertCircle( pnt ))
        {
            shape->type = MS_SHAPE_POINT;
            points.numpoints = 2;
            points.point = pnt;
            msAddLine( shape, &points );
        }
        else
        {
            strcpy( hand->last_oci_error, "Points in circle object are colinear" );
            hand->last_oci_status = MS_FAILURE;
        }
    }
}

static void osArcPolygon(msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, int start, int end, lineObj arcpoints, int data3d)
{
    int n, i;
    lineObj points = {0, NULL};

    n = (end - start)/2;
    points.point = (pointObj *)malloc( sizeof(pointObj)*n );

    if (data3d)
        n = msOCIGet3DOrdinates( hand, obj, start, end, points.point );
    else
        n = msOCIGet2DOrdinates( hand, obj, start, end, points.point );

    if (n > 2)
    {
        shape->type = MS_SHAPE_LINE;
        points.numpoints = n;

        for (i = 0; i < n-2; i = i+2)
            osGenerateArc(shape, arcpoints, points, i, n, data3d);
    }
    free (points.point);
}

static int osCheck2DGtype(int pIntGtype)
{
   if (pIntGtype > 2000 && pIntGtype < 2008)
   {
      if (pIntGtype != 2004)
          return MS_TRUE;
   }

   return MS_FALSE;
}

static int osCheck3DGtype(int pIntGtype)
{
   if (pIntGtype > 3000 && pIntGtype < 3308)
   {
      if (pIntGtype > 3007)
          pIntGtype-= 300;

      if (pIntGtype < 3007 && pIntGtype != 3004)
          return MS_TRUE;
   }
   /*
   * Future version, untested
   * return (pIntGtype & 2208 && (pIntGtype & 3000 || pIntGtype & 3296) && pIntGtype & 3);
   */
   return MS_FALSE;   
}

static int osGetOrdinates(msOracleSpatialDataHandler *dthand, msOracleSpatialHandler *hand, shapeObj *shape, SDOGeometryObj *obj, SDOGeometryInd *ind)
{
    int gtype, elem_type, compound_type;
    float compound_lenght, compound_count;
    ub4 etype;
    ub4 interpretation;
    int nelems, nords, data3d;
    int elem, ord_start, ord_end;
    boolean exists;
    OCINumber *oci_number;
    double x, y;
#ifdef USE_POINT_Z_M
    double z;
#endif /* USE_POINT_Z_M */
    int success;
    lineObj points = {0, NULL};
    pointObj point5[5]; /* for quick access */
    shapeObj newshape; /* for compound polygons */

    /*stat the variables for compound polygons*/
    compound_lenght = 0;
    compound_type = 0;
    compound_count = -1;
    data3d = 0;

    if (ind->_atomic != OCI_IND_NULL)  /* not a null object */
    {
        nelems = nords = 0;
        success = TRY( hand, OCICollSize( hand->envhp, hand->errhp, (OCIColl *)obj->elem_info, &nelems ) )
               && TRY( hand, OCICollSize( hand->envhp, hand->errhp, (OCIColl *)obj->ordinates, &nords ) )
               && TRY( hand, OCINumberToInt( hand->errhp, &(obj->gtype), (uword)sizeof(int), OCI_NUMBER_SIGNED, (dvoid *)&gtype ) );
               /*&& (nords%2==0 && nelems%3==0);*/ /* check %2==0 for 2D geometries; and %3==0 for element info triplets */

        if (success && osCheck2DGtype(gtype))
            success = (nords%2==0 && nelems%3==0)?1:0; /* check %2==0 for 2D geometries; and %3==0 for element info triplets */
        else if (success && osCheck3DGtype(gtype))
        {
            success = (nords%3==0 && nelems%3==0)?1:0; /* check %2==0 for 2D geometries; and %3==0 for element info triplets */
            data3d = 1;
        }

        if (success)
        {
            /* reading SDO_POINT from SDO_GEOMETRY for a 2D/3D point geometry */
            if ((gtype==2001 || gtype==3001) && ind->point._atomic == OCI_IND_NOTNULL && ind->point.x == OCI_IND_NOTNULL && ind->point.y == OCI_IND_NOTNULL)
            {
                success = TRY( hand, OCINumberToReal( hand->errhp, &(obj->point.x), (uword)sizeof(double), (dvoid *)&x ) )
                       && TRY( hand, OCINumberToReal( hand->errhp, &(obj->point.y), (uword)sizeof(double), (dvoid *)&y ) );

                if (ERROR( "osGetOrdinates()", hand, dthand ))
                    return MS_FAILURE;

                shape->type = MS_SHAPE_POINT;

                points.numpoints = 1;
                points.point = point5;
                point5[0].x = x;
                point5[0].y = y;
#ifdef USE_POINT_Z_M
                if (data3d)
                {
                    if (ind->point.z == OCI_IND_NOTNULL)
                    {
                        success = TRY( hand, OCINumberToReal( hand->errhp, &(obj->point.z), (uword)sizeof(double), (dvoid *)&z ));
                        if (ERROR( "osGetOrdinates()", hand, dthand ))
                            return MS_FAILURE;
                        else
                            point5[0].z = z;
                    }
                    else
                        point5[0].z = 0;
                }
#endif /* USE_POINT_Z_M */
                msAddLine( shape, &points );
                return MS_SUCCESS;
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

              if (ERROR( "osGetOrdinates()", hand, dthand ))
                  return MS_FAILURE;

              if ( etype == 1005 || etype == 2005  || etype == 4 )
              {
                  compound_type = 1;
                  compound_lenght = interpretation;
                  compound_count = 0;
                  msInitShape(&newshape);
              }

              elem_type = (etype == 1 && interpretation > 1) ? 10 : ((etype%10)*10 + interpretation);

              switch (elem_type)
              {
                  case 10: /* point cluster with 'interpretation'-points */
                      osPointCluster (hand, shape, obj, ord_start, ord_end, points, interpretation, data3d);
                      break;
                  case 11: /* point type */
                      osPoint(hand, shape, obj, ord_start, ord_end, points, point5, data3d);
                      break;
                  case 21: /* line string whose vertices are connected by straight line segments */
                      if (compound_type)
                          osClosedPolygon(hand, &newshape, obj, ord_start, (compound_count<compound_lenght)?ord_end+2:ord_end, points, elem_type, data3d);
                      else
                          osClosedPolygon(hand, shape, obj, ord_start, ord_end, points, elem_type, data3d);
                      break;
                  case 22: /* compound type */
                      if (compound_type)
                          osArcPolygon(hand, &newshape, obj, ord_start, (compound_count<compound_lenght)?ord_end+2:ord_end , points, data3d);
                      else
                          osArcPolygon(hand, shape, obj, ord_start, ord_end, points, data3d);
                      break;
                  case 31: /* simple polygon with n points, last point equals the first one */
                      osClosedPolygon(hand, shape, obj, ord_start, ord_end, points, elem_type, data3d);
                      break;
                  case 33: /* rectangle defined by 2 points */
                      osRectangle(hand, shape, obj, ord_start, ord_end, points, point5, data3d);
                      break;
                  case 34: /* circle defined by 3 points */
                      osCircle(hand, shape, obj, ord_start, ord_end, points, point5, data3d);
                      break;
              }

              if (compound_count >= compound_lenght)
              {
                  osCloneShape(&newshape, shape, data3d);
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

    if (compound_type)
        msFreeShape(&newshape);

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

    msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)malloc(sizeof(msOracleSpatialLayerInfo));
    msOracleSpatialDataHandler *dthand = (msOracleSpatialDataHandler *)malloc(sizeof(msOracleSpatialDataHandler));
    msOracleSpatialHandler *hand = (msOracleSpatialHandler *)malloc(sizeof(msOracleSpatialHandler));

    memset( hand, 0, sizeof(msOracleSpatialHandler) );
    memset( dthand, 0, sizeof(msOracleSpatialDataHandler) );
    memset( layerinfo, 0, sizeof(msOracleSpatialLayerInfo) );

    if (layer->debug)
        msDebug("msOracleSpatialLayerOpen called with: %s\n",layer->data);

    if (layer->layerinfo != NULL)
        return MS_SUCCESS;

    if (layer->data == NULL) 
    {
        msSetError( MS_ORACLESPATIALERR, 
                    "Error parsing OracleSpatial DATA variable. Must be:"
                    "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                    "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'."
                    "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE.",
                    "msOracleSpatialLayerOpen()");

        return MS_FAILURE;
    }

    msSplitLogin( layer->connection, layer->map, username, password, dblink );

    hand = (msOracleSpatialHandler *) msConnPoolRequest( layer );

    if( hand == NULL )
    {

        hand = msOCISetHandlers( username, password, dblink );

        if (hand == NULL)
        {
            msOCICloseDataHandlers( dthand );
            msOCIClearLayerInfo( layerinfo );

            return MS_FAILURE;
        }

        if ( layer->debug )
            msDebug("msOracleSpatialLayerOpen. Shared connection not available. Creating one.\n");

        msConnPoolRegister( layer, hand, msOCICloseConnection );
    }
    else
    {
        hand->last_oci_status = MS_SUCCESS;
        hand->last_oci_error[0] = (text)'\0';
    }

    if (!msOCISetDataHandlers(hand, dthand))
    {
        msSetError( MS_ORACLESPATIALERR,
                    "Cannot create OCI LayerInfo. "
                    "Connection failure.",
                    "msOracleSpatialLayerOpen()");

        if (layer->debug)
            msDebug("msOracleSpatialLayerOpen. Cannot create OCI LayerInfo. Connection failure.\n");

        msOCICloseDataHandlers ( dthand );
        msOCICloseHandlers( hand );
        msOCIClearLayerInfo( layerinfo );
    }
    layerinfo->orahandlers = hand;
    layerinfo->oradatahandlers = dthand;
    layer->layerinfo = layerinfo;

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

    if (layer->debug)
        msDebug("msOracleSpatialLayerClose was called. Layer connection: %s\n",layer->connection);

    if (layerinfo != NULL)
    {
        if (layer->debug)
          msDebug("msOracleSpatialLayerClose. Cleaning layerinfo handlers.\n");
        msOCICloseDataHandlers( layerinfo->oradatahandlers );
        layerinfo->oradatahandlers = NULL;

        if (layer->debug)
          msDebug("msOracleSpatialLayerClose. Cleaning Oracle handlers.\n"); 
        msConnPoolRelease( layer, layerinfo->orahandlers );
        layerinfo->orahandlers = NULL;

        msOCIClearLayerInfo( layerinfo );
        layer->layerinfo = NULL;
    }

    return MS_SUCCESS;
}

/* create SQL statement for retrieving shapes */
int msOracleSpatialLayerWhichShapes( layerObj *layer, rectObj rect )
{
    int success, i;
    int function = 0;
    int version = 0;
    char query_str[6000];
    char table_name[2000], geom_column_name[100], unique[100], srid[100];
    OCIDefine *adtp = NULL, *items[ARRAY_SIZE] = { NULL };

    /* get layerinfo */
    msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;
    msOracleSpatialDataHandler *dthand = NULL;
    msOracleSpatialHandler *hand = NULL;

    if (layer->debug)
        msDebug("msOracleSpatialLayerWhichShapes was called.\n");  

    if (layerinfo == NULL)
    {
        msSetError( MS_ORACLESPATIALERR,
                    "msOracleSpatialLayerWhichShapes called on unopened layer",
                    "msOracleSpatialLayerWhichShapes()" );

        return MS_FAILURE;
    }
    else
    {
         dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
         hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    }

    /* parse geom_column_name and table_name */
    if (!msSplitData( layer->data, geom_column_name, table_name, unique, srid, &function, &version)) 
    {
        msSetError( MS_ORACLESPATIALERR, 
                    "Error parsing OracleSpatial DATA variable. Must be:"
                    "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                    "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'."
                    "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE."
                    "Your data statement: %s", 
                    "msOracleSpatialLayerWhichShapes()", layer->data );

        return MS_FAILURE;
    }

    /*Define the unique*/
    if (unique[0] == '\0')
        strcpy( unique, "rownum" );

    sprintf( query_str, "SELECT %s", unique );

    /* allocate enough space for items */
    if (layer->numitems >= 0)
    {
        layerinfo->items = (item_text_array *)malloc( sizeof(item_text_array) * (layer->numitems+1) );
        if (layerinfo->items == NULL)
        {
            msSetError( MS_ORACLESPATIALERR,"Cannot allocate items buffer","msOracleSpatialLayerWhichShapes()" );
            return MS_FAILURE;
        }
    }

    /* define SQL query */
    for( i=0; i < layer->numitems; ++i )
        sprintf( query_str + strlen(query_str), ", %s", layer->items[i] );

    sprintf( query_str + strlen(query_str), ", %s FROM %s", geom_column_name, table_name );

    if (layer->filter.string != NULL)
    {
        if (function == FUNCTION_NONE)
            sprintf (query_str + strlen(query_str), " WHERE %s ", layer->filter.string);
        else
            sprintf (query_str + strlen(query_str), " WHERE %s AND ", layer->filter.string);
    }
    else
    {
        if (function != FUNCTION_NONE)
            strcat( query_str, " WHERE " );
    }

    if ((((atol(srid) >= 8192) && (atol(srid) <= 8330)) || (atol(srid) == 2) || (atol(srid) == 5242888) || (atol(srid) == 2000001)) && (version == VERSION_9i))
        osGeodeticData(function, version, query_str, geom_column_name, srid, rect);
    else
        osNoGeodeticData(function, version, query_str, geom_column_name, srid, rect);

    if (layer->debug)
        msDebug("msOracleSpatialLayerWhichShapes. Using this Sql to retrieve the data: %s\n", query_str);

    /* prepare */
    success = TRY( hand, OCIStmtPrepare( dthand->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) );

    if (success && layer->numitems >= 0)
    {
        for( i=0; i <= layer->numitems && success; ++i )
            success = TRY( hand, OCIDefineByPos( dthand->stmthp, &items[i], hand->errhp, (ub4)i+1, (dvoid *)layerinfo->items[i], (sb4)TEXT_SIZE, SQLT_STR, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) );
    }

    if (success)
    {
        success = TRY( hand,
            /* define spatial position adtp ADT object */
            OCIDefineByPos( dthand->stmthp, &adtp, hand->errhp, (ub4)layer->numitems+2, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
               && TRY( hand,
            /* define object tdo from adtp */
            OCIDefineObject( adtp, hand->errhp, dthand->tdo, (dvoid **)layerinfo->obj, (ub4 *)0, (dvoid **)layerinfo->ind, (ub4 *)0 ) )
               && TRY( hand,
            /* execute */
            OCIStmtExecute( hand->svchp, dthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ) )
               &&  TRY( hand,
            /* get rows fetched */
            OCIAttrGet( (dvoid *)dthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ) );
    }

    if (!success)
    {
        msSetError( MS_ORACLESPATIALERR, 
                    "Error: %s . "
                    "Query statement: %s . "
                    "Check your data statement.", 
                    "msOracleSpatialLayerWhichShapes()", hand->last_oci_error, query_str );

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
    msOracleSpatialDataHandler *dthand = NULL;
    msOracleSpatialHandler *hand = NULL;

    if (layerinfo == NULL) 
    {
        msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerWhichShapes called on unopened layer", "msOracleSpatialLayerNextShape()" );
        return MS_FAILURE;
    }
    else
    {
        dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
        hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    }

    /* no rows fetched */
    if (layerinfo->rows_fetched == 0)
        return MS_DONE;

    do{
        /* is buffer empty? */
        if (layerinfo->row_num >= layerinfo->rows_fetched)
        {
            /* fetch more */
            success = TRY( hand, OCIStmtFetch( dthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub2)OCI_FETCH_NEXT, (ub4)OCI_DEFAULT ) ) 
                   && TRY( hand, OCIAttrGet( (dvoid *)dthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ) );

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
        if (shape->values == NULL)
        {
            msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the values", "msOracleSpatialLayerNextShape()" );
            return MS_FAILURE;
        }

        for( i=0; i < shape->numvalues; ++i )
        {
            shape->values[i] = (char *)malloc(strlen(layerinfo->items[i+1][ layerinfo->row ])+1);
            if (shape->values[i] == NULL)
            {
                msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items", "msOracleSpatialLayerNextShape()" );
                return MS_FAILURE;
            }
            else
            {
                strcpy(shape->values[i], layerinfo->items[i+1][ layerinfo->row ]);
                shape->values[i][strlen(layerinfo->items[i+1][ layerinfo->row ])] = '\0';
            }
        }

        /* increment for next row */
        layerinfo->row_num++;
        layerinfo->row++;

        /* fetch a layer->type object */   
        success = osGetOrdinates(dthand, hand, shape, obj, ind);
        if (success != MS_SUCCESS)
            return MS_FAILURE;

        osShapeBounds(shape);
    }while(shape->type == MS_SHAPE_NULL);

    return MS_SUCCESS;
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

    if ((layer->iteminfo = (long *)malloc(sizeof(int)*layer->numitems))== NULL)
    {
        msSetError(MS_MEMERR, NULL, "msOracleSpatialLayerInitItemInfo()");
        return MS_FAILURE;
    }

    itemindexes = (int*)layer->iteminfo;

    for(i=0; i < layer->numitems; i++)
        itemindexes[i] = i;  /*last one is always the geometry one - the rest are non-geom*/

    return MS_SUCCESS;
}

int msOracleSpatialLayerGetItems( layerObj *layer )
{
    char *rzt = "";
    char *flk = "";
    int function = 0;
    int version = 0;
    int existgeom;
    int count_item, flk_len, success, i;
    char query_str[6000], table_name[2000], geom_column_name[100], unique[100], srid[100];
    OCIParam *pard = (OCIParam *) 0;

    msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *) layer->layerinfo;
    msOracleSpatialDataHandler *dthand = NULL;
    msOracleSpatialHandler *hand = NULL;

    if (layer->debug)
        msDebug("msOracleSpatialLayerGetItems was called.\n");

    if (layerinfo == NULL)
    {
        msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetItems called on unopened layer", "msOracleSpatialLayerGetItems()" );
        return MS_FAILURE;
    }
    else
    {
        dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
        hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    }

    if (!msSplitData(layer->data, geom_column_name, table_name, unique, srid, &function, &version))
    {
        msSetError( MS_ORACLESPATIALERR, 
                    "Error parsing OracleSpatial DATA variable. Must be: "
                    "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                    "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
                    "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE. "
                    "Your data statement: %s", 
                    "msOracleSpatialLayerGetItems()", layer->data );

        return MS_FAILURE;
    }

    sprintf( query_str, "SELECT * FROM %s", table_name );

    success =  TRY( hand, OCIStmtPrepare( dthand->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DESCRIBE_ONLY) )
            && TRY( hand, OCIStmtExecute( hand->svchp, dthand->stmthp, hand->errhp, (ub4)QUERY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DESCRIBE_ONLY ) )
            && TRY( hand, OCIAttrGet( (dvoid *)dthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layer->numitems, (ub4 *)0, OCI_ATTR_PARAM_COUNT, hand->errhp) );

    if (!success)
    {
        msSetError( MS_QUERYERR, "Cannot retrieve column list", "msOracleSpatialLayerGetItems()" );
        return MS_FAILURE;
    }

    layerinfo->row_num = layerinfo->row = 0;
    layer->numitems = layer->numitems-1;

    layer->items = malloc (sizeof(char *) * (layer->numitems));
    if (layer->items == NULL)
    {
        msSetError( MS_ORACLESPATIALERR,"Cannot allocate items", "msOracleSpatialLayerGetItems()" );
        return MS_FAILURE;
    }

    if (layer->numitems > 0)
    {
        layerinfo->items_query = (item_text_array_query *)malloc( sizeof(item_text_array_query) * (layer->numitems) );
        if (layerinfo->items_query == NULL)
        {
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
    for (i = 0; i <= layer->numitems; i++)
    {
        success = TRY( hand, OCIParamGet ((dvoid*) dthand->stmthp, (ub4)OCI_HTYPE_STMT,hand->errhp,(dvoid*)&pard, (ub4)i+1))
               && TRY( hand, OCIAttrGet ((dvoid *) pard,(ub4) OCI_DTYPE_PARAM,(dvoid*)&rzt,(ub4 *)&flk_len, (ub4) OCI_ATTR_NAME, hand->errhp ));

        flk = (char *)malloc(sizeof(char*) * flk_len+1);
        if (flk == NULL)
        {
            msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items", "msOracleSpatialLayerGetItems()" );
            return MS_FAILURE;
        }
        else
        {
            strncpy(flk, rzt, flk_len);
            flk[flk_len] = '\0';
        }

        /*Comapre the column name (flk) with geom_column_name and ignore with true*/
        if (strcmp(flk, geom_column_name) != 0)
        {
            layer->items[count_item] = (char *)malloc(sizeof(char) * flk_len+1);
            if (layer->items[count_item] == NULL)
            {
                msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items buffer", "msOracleSpatialLayerGetItems()" );
                return MS_FAILURE;
            }
            else
            {
                strcpy(layer->items[count_item], flk);
            }
            count_item++;
        }
        else
            existgeom = 1;

        strcpy( rzt, "" );
        free(flk); /* Better?!*/
        flk_len = 0;
    }
    if (!(existgeom))
    {
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
    int version = 0;
    OCIDefine *adtp = NULL, *items[QUERY_SIZE] = { NULL };
    SDOGeometryObj *obj = NULL;
    SDOGeometryInd *ind = NULL;
    sb2 *nullind = NULL;

    msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;
    msOracleSpatialDataHandler *dthand = NULL;
    msOracleSpatialHandler *hand = NULL;

    if (layer->debug)
        msDebug("msOracleSpatialLayerGetShape was called. Using the record = %ld.\n", record);

    if (layerinfo == NULL)
    {
        msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetShape called on unopened layer","msOracleSpatialLayerGetShape()" );
        return MS_FAILURE;
    }
    else
    {
        dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
        hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    }

    /* allocate enough space for items */
    if (layer->numitems > 0)
    {
        layerinfo->items_query = (item_text_array_query *)malloc( sizeof(item_text_array_query) * (layer->numitems) );
        nullind = (sb2 *)malloc( sizeof(sb2) * (layer->numitems) );

        if (layerinfo->items_query == NULL)
        {
            msSetError( MS_ORACLESPATIALERR, "Cannot allocate items buffer", "msOracleSpatialLayerGetShape()" );
            return MS_FAILURE;
        }
    }

    if (!msSplitData( layer->data, geom_column_name, table_name, unique, srid, &function, &version ))
    {
        msSetError( MS_ORACLESPATIALERR,
                    "Error parsing OracleSpatial DATA variable. Must be: "
                    "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                    "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
                    "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE. "
                    "Your data statement: %s",
                    "msOracleSpatialLayerGetShape()", layer->data );

        return MS_FAILURE;
    }

    /*Define the first query to retrive itens*/
    if (unique[0] == '\0')
    {
        msSetError( MS_ORACLESPATIALERR, 
                    "Error parsing OracleSpatial DATA variable for query. To execute "
                    "query functions you need to define one "
                    "unique column [USING UNIQUE <#column>]", 
                    "msOracleSpatialLayerGetShape()" );

        return MS_FAILURE;
    }
    else
        sprintf( query_str, "SELECT");

    /*Define the query*/
    for( i = 0; i < layer->numitems; ++i )
        sprintf( query_str + strlen(query_str), " %s,", layer->items[i] );

    sprintf( query_str + strlen(query_str), " %s FROM %s WHERE %s = %ld", geom_column_name, table_name, unique, record);

    if (layer->filter.string != NULL)
        sprintf( query_str + strlen(query_str), " AND %s", (layer->filter.string));

    if (layer->debug)
      msDebug("msOracleSpatialLayerGetShape. Sql: %s\n", query_str);

    /*Prepare the handlers to the query*/
    success = TRY( hand,OCIStmtPrepare( dthand->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) );

    if (success && layer->numitems > 0)
    {
        for( i = 0; i < layer->numitems && success; ++i )
            success = TRY( hand, OCIDefineByPos( dthand->stmthp, &items[i], hand->errhp, (ub4)i+1, (dvoid *)layerinfo->items_query[i], (sb4)TEXT_SIZE, SQLT_STR, (sb2 *)&nullind[i], (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) );
    }

    if(!success)
    {
        msSetError( MS_ORACLESPATIALERR, 
                    "Error: %s . "
                    "Query statement: %s . "
                    "Check your data statement.",
                    "msOracleSpatialLayerGetShape()", hand->last_oci_error, query_str );

        return MS_FAILURE;
    }

    if (success)
    {
        success = TRY( hand, OCIDefineByPos( dthand->stmthp, &adtp, hand->errhp, (ub4)layer->numitems+1, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
               && TRY( hand, OCIDefineObject( adtp, hand->errhp, dthand->tdo, (dvoid **)layerinfo->obj, (ub4 *)0, (dvoid **)layerinfo->ind, (ub4 *)0 ) )
               && TRY (hand, OCIStmtExecute( hand->svchp, dthand->stmthp, hand->errhp, (ub4)QUERY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ))
               && TRY (hand, OCIAttrGet( (dvoid *)dthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ));

    }
    if(!success)
    {
        msSetError( MS_ORACLESPATIALERR, 
                    "Error: %s . "
                    "Query statement: %s ."
                    "Check your data statement.",
                    "msOracleSpatialLayerGetShape()", hand->last_oci_error, query_str );

        return MS_FAILURE;
    }

    shape->type = MS_SHAPE_NULL;

    /* no rows fetched */
    if (layerinfo->rows_fetched == 0)
        return (MS_DONE);

    obj = layerinfo->obj[ layerinfo->row ];
    ind = layerinfo->ind[ layerinfo->row ];

    /* get the items for the shape */
    shape->numvalues = layer->numitems;
    shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
    if (shape->values == NULL)
    {
        msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the values.", "msOracleSpatialLayerGetShape()" );
        return MS_FAILURE;
    }

    shape->index = record;

    for( i = 0; i < layer->numitems; ++i )
    {
        shape->values[i] = (char *)malloc(strlen(layerinfo->items_query[layerinfo->row][i])+1);

        if (shape->values[i] == NULL)
        {
            msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items buffer.", "msOracleSpatialLayerGetShape()" );
            return MS_FAILURE;
        }
        else
        {
            if (nullind[i] != OCI_IND_NULL)
            {
                strcpy(shape->values[i], layerinfo->items_query[layerinfo->row][i]);
                shape->values[i][strlen(layerinfo->items_query[layerinfo->row][i])] = '\0';
            }
            else
            {
                shape->values[i][0] = '\0';
            }
        }
    }

    /* increment for next row */
    layerinfo->row_num++;
    layerinfo->row++;

    /* fetch a layer->type object */
    success = osGetOrdinates(dthand, hand, shape, obj, ind);
    if (success != MS_SUCCESS)
    {
        msSetError( MS_ORACLESPATIALERR, "Cannot execute query", "msOracleSpatialLayerGetShape()" );
        return MS_FAILURE;
    }
    osShapeBounds(shape);
    layerinfo->row = layerinfo->row_num = 0;

    return (MS_SUCCESS);
}

int msOracleSpatialLayerGetExtent(layerObj *layer, rectObj *extent)
{
    char query_str[6000], table_name[2000], geom_column_name[100], unique[100], srid[100];
    int success, i;
    int function = 0;
    int version = 0;
    OCIDefine *adtp = NULL, *items[QUERY_SIZE] = { NULL };
    SDOGeometryObj *obj = NULL;
    SDOGeometryInd *ind = NULL;
    shapeObj shape;
    rectObj bounds;

    msOracleSpatialLayerInfo *layerinfo = (msOracleSpatialLayerInfo *)layer->layerinfo;
    msOracleSpatialDataHandler *dthand = NULL;
    msOracleSpatialHandler *hand = NULL;

    if (layer->debug)
        msDebug("msOracleSpatialLayerGetExtent was called.\n");

    if (layerinfo == NULL)
    {
        msSetError( MS_ORACLESPATIALERR, "msOracleSpatialLayerGetExtent called on unopened layer","msOracleSpatialLayerGetExtent()");
        return MS_FAILURE;
    }
    else
    {
        dthand = (msOracleSpatialDataHandler *)layerinfo->oradatahandlers;
        hand = (msOracleSpatialHandler *)layerinfo->orahandlers;
    }

    /* allocate enough space for items */
    if (layer->numitems > 0)
    {
        layerinfo->items_query = (item_text_array_query *)malloc( sizeof(item_text_array_query) * (layer->numitems) );
        if (layerinfo->items_query == NULL)
        {
            msSetError( MS_ORACLESPATIALERR, "Cannot allocate items buffer", "msOracleSpatialLayerGetExtent()" );
            return MS_FAILURE;
        }
    }

    if (!msSplitData( layer->data, geom_column_name, table_name, unique, srid, &function, &version )) 
    {
        msSetError( MS_ORACLESPATIALERR, 
                    "Error parsing OracleSpatial DATA variable. Must be: "
                    "'geometry_column FROM table_name [USING UNIQUE <column> SRID srid# FUNCTION]' or "
                    "'geometry_column FROM (SELECT stmt) [USING UNIQUE <column> SRID srid# FUNCTION]'. "
                    "If want to set the FUNCTION statement you can use: FILTER, RELATE, GEOMRELATE or NONE. "
                    "Your data statement: %s", 
                    "msOracleSpatialLayerGetExtent()", layer->data );
        return MS_FAILURE;
    }

    if (version == VERSION_10g)
        osAggrGetExtent(layer, query_str, geom_column_name, table_name);
    else
    {
        if (((atol(srid) < 8192) || (atol(srid) > 8330)) && (atol(srid) != 2) && (atol(srid) != 5242888) && (atol(srid) != 2000001))
        {
            if (version == VERSION_9i)
                osAggrGetExtent(layer, query_str, geom_column_name, table_name);
            else
                osConvexHullGetExtent(layer, query_str, geom_column_name, table_name);
        }
        else
            osConvexHullGetExtent(layer, query_str, geom_column_name, table_name);
    }

    if (layer->debug)
        msDebug("msOracleSpatialLayerGetExtent. Using this Sql to retrieve the extent: %s.\n", query_str);

    /*Prepare the handlers to the query*/
    success = TRY( hand,OCIStmtPrepare( dthand->stmthp, hand->errhp, (text *)query_str, (ub4)strlen(query_str), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT) );

    if (success && layer->numitems > 0)
    {
        for( i = 0; i < layer->numitems && success; ++i )
            success = TRY( hand, OCIDefineByPos( dthand->stmthp, &items[i], hand->errhp, (ub4)i+1, (dvoid *)layerinfo->items_query[i], (sb4)TEXT_SIZE, SQLT_STR, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT ) );
    }

    if(!success)
    {
        msSetError( MS_ORACLESPATIALERR,
                    "Error: %s . "
                    "Query statement: %s . "
                    "Check your data statement.",
                    "msOracleSpatialLayerGetExtent()", hand->last_oci_error, query_str );
        return MS_FAILURE;
    }

    if (success)
    {
        success = TRY( hand, OCIDefineByPos( dthand->stmthp, &adtp, hand->errhp, (ub4)layer->numitems+1, (dvoid *)0, (sb4)0, SQLT_NTY, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT) )
               && TRY( hand, OCIDefineObject( adtp, hand->errhp, dthand->tdo, (dvoid **)layerinfo->obj, (ub4 *)0, (dvoid **)layerinfo->ind, (ub4 *)0 ) )
               && TRY (hand, OCIStmtExecute( hand->svchp, dthand->stmthp, hand->errhp, (ub4)QUERY_SIZE, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT ))
               && TRY (hand, OCIAttrGet( (dvoid *)dthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ));

    }
    if(!success)
    {
        msSetError( MS_ORACLESPATIALERR, 
                    "Error: %s . "
                    "Query statement: %s ."
                    "Check your data statement.",
                    "msOracleSpatialLayerGetExtent()", hand->last_oci_error, query_str );

        return MS_FAILURE;
    }

    /* should begin processing first row */
    layerinfo->row_num = layerinfo->row = 0;
    msInitShape( &shape );
    do{
        /* is buffer empty? */
        if (layerinfo->row_num >= layerinfo->rows_fetched) 
        {
            /* fetch more */
            success = TRY( hand, OCIStmtFetch( dthand->stmthp, hand->errhp, (ub4)ARRAY_SIZE, (ub2)OCI_FETCH_NEXT, (ub4)OCI_DEFAULT ) ) 
                   && TRY( hand, OCIAttrGet( (dvoid *)dthand->stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&layerinfo->rows_fetched, (ub4 *)0, (ub4)OCI_ATTR_ROW_COUNT, hand->errhp ) );

            if (!success || layerinfo->rows_fetched == 0)
                break;

            if (layerinfo->row_num >= layerinfo->rows_fetched)
                break;

            layerinfo->row = 0; /* reset row index */
        }

        /* no rows fetched */
        if (layerinfo->rows_fetched == 0)
            break;

        obj = layerinfo->obj[ layerinfo->row ];
        ind = layerinfo->ind[ layerinfo->row ];

        /* get the items for the shape */
        shape.numvalues = layer->numitems;
        shape.values = (char **) malloc(sizeof(char *) * layer->numitems);
        if (shape.values == NULL)
        {
            msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the values.", "msOracleSpatialLayerGetExtent()" );
            return MS_FAILURE;
        }

        shape.index = layerinfo->row_num;

        for( i = 0; i < layer->numitems; ++i )
        {
            shape.values[i] = (char *)malloc(strlen(layerinfo->items_query[layerinfo->row][i])+1);

            if (shape.values[i] == NULL)
            {
                msSetError( MS_ORACLESPATIALERR, "No memory avaliable to allocate the items buffer.", "msOracleSpatialLayerGetExtent()" );
                return MS_FAILURE;
            }
            else
            {
                strcpy(shape.values[i], layerinfo->items_query[layerinfo->row][i]);
                shape.values[i][strlen(layerinfo->items_query[layerinfo->row][i])] = '\0';
            }
        }

        /* increment for next row */
        layerinfo->row_num++;
        layerinfo->row++;

        /* fetch a layer->type object */ 
        success = osGetOrdinates(dthand, hand, &shape, obj, ind);
        if (success != MS_SUCCESS)
        {
            msSetError( MS_ORACLESPATIALERR, "Cannot execute query", "msOracleSpatialLayerGetExtent()" );
            return MS_FAILURE;
        }

    }while(layerinfo->row <= layerinfo->rows_fetched);

    layerinfo->row = layerinfo->row_num = 0;

    osShapeBounds(&shape);
    bounds = shape.bounds;

    extent->minx = bounds.minx;
    extent->miny = bounds.miny;
    extent->maxx = bounds.maxx;
    extent->maxy = bounds.maxy;

    msFreeShape(&shape);

    return(MS_SUCCESS);
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

int msOracleSpatialLayerIsOpen(layerObj *layer)
{
  msSetError( MS_ORACLESPATIALERR, "OracleSpatial is not supported", "msOracleSpatialLayerIsOpen()" );
  return MS_FALSE;
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

int msOracleSpatialLayerGetShapeVT(layerObj *layer, shapeObj *shape, int tile, long record)
{
    return msOracleSpatialLayerGetShape(layer, shape, record);
}

int msOracleSpatialLayerInitializeVirtualTable(layerObj *layer)
{
    assert(layer != NULL);
    assert(layer->vtable != NULL);

    layer->vtable->LayerInitItemInfo = msOracleSpatialLayerInitItemInfo;
    layer->vtable->LayerFreeItemInfo = msOracleSpatialLayerFreeItemInfo;
    layer->vtable->LayerOpen = msOracleSpatialLayerOpen;
    layer->vtable->LayerIsOpen = msOracleSpatialLayerIsOpen;
    layer->vtable->LayerWhichShapes = msOracleSpatialLayerWhichShapes;
    layer->vtable->LayerNextShape = msOracleSpatialLayerNextShape;
    layer->vtable->LayerGetShape = msOracleSpatialLayerGetShapeVT;

    layer->vtable->LayerClose = msOracleSpatialLayerClose;
    layer->vtable->LayerGetItems = msOracleSpatialLayerGetItems;
    layer->vtable->LayerGetExtent = msOracleSpatialLayerGetExtent;

    /* layer->vtable->LayerGetAutoStyle, use default */

    layer->vtable->LayerCloseConnection = msOracleSpatialLayerClose;

    layer->vtable->LayerApplyFilterToLayer = msLayerApplyCondSQLFilterToLayer;

    layer->vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;
    /* layer->vtable->LayerCreateItems, use default */
    /* layer->vtable->LayerGetNumFeatures, use default */

    return MS_SUCCESS;
}


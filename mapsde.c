/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implements SDE CONNECTIONTYPE.
 * Author:   Steve Lime and Howard Butler
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
 *****************************************************************************/

#include <time.h>
#include <assert.h>

#include "mapserver.h"
#include "maperror.h"
#include "maptime.h"
#include "mapthread.h"

#ifdef USE_SDE
#include <sdetype.h> /* ESRI SDE Client Includes */
#include <sdeerno.h>

#define MS_SDE_MAXBLOBSIZE 1024*50 /* 50 kbytes */
#define MS_SDE_NULLSTRING "<null>"
#define MS_SDE_SHAPESTRING "<shape>"
#define MS_SDE_TIMEFMTSIZE 128 /* bytes */
#define MS_SDE_TIMEFMT "%H:%M:%S %m/%d/%Y"
#define MS_SDE_ROW_ID_COLUMN "SE_ROW_ID"

typedef struct {
  SE_CONNECTION connection;
  SE_STREAM stream;
} msSDEConnPoolInfo;

typedef struct {
  msSDEConnPoolInfo *connPoolInfo;
  SE_CONNECTION connection;
  SE_LAYERINFO layerinfo;
  SE_COORDREF coordref;
  SE_STREAM stream;
  SE_INT32 state_id;
  char *table;
  char *column;
  char *row_id_column;
  char *join_table;
  rectObj* extent;
  SE_COLUMN_DEF *basedefs;
  SE_COLUMN_DEF *joindefs;
  short *nBaseColumns;
  short *nJoinColumns;
  int bBigEndian;
} msSDELayerInfo;

typedef struct {
  SE_INT32 layerId;
  char *table;
  char *column;
  char *connection;
} layerId;

/*
 * Layer ID caching section.
 */

static int lcacheCount = 0;
static int lcacheMax = 0;
static layerId *lcache = NULL;

#endif

/* -------------------------------------------------------------------- */
/* msSDELayerIsOpen                                                     */
/* -------------------------------------------------------------------- */
/*     Returns MS_TRUE if layer is already opened, MS_FALSE otherwise   */
/* -------------------------------------------------------------------- */
int msSDELayerIsOpen(layerObj *layer)
{
#ifdef USE_SDE

  if(layer->layerinfo)
    return(MS_TRUE);

  return MS_FALSE;

#else
  msSetError(MS_MISCERR, "SDE support is not available.",
             "msSDELayerIsOpen()");
  return(MS_FALSE);
#endif
}

#ifdef USE_SDE


/************************************************************************/
/*       Start SDE/MapServer helper functions.                          */
/*                                                                      */
/************************************************************************/



/* -------------------------------------------------------------------- */
/* msSDECloseConnection                                                 */
/* -------------------------------------------------------------------- */
/*     Closes the SDE connection handle, which is given as a callback   */
/*     function to the connection pooling API                           */
/* -------------------------------------------------------------------- */
static void msSDECloseConnection( void *conn_handle )
{

  long status;
  msSDEConnPoolInfo *poolinfo = conn_handle;

  if (poolinfo) {
    if (poolinfo->stream) {
      SE_stream_free(poolinfo->stream);
    }
    if (poolinfo->connection) {
      status = SE_connection_free_all_locks (poolinfo->connection);
      if (status == SE_SUCCESS) {
        SE_connection_free(poolinfo->connection);
      }
    }
    msFree(poolinfo);
  }

}

/* -------------------------------------------------------------------- */
/* sde_error                                                            */
/* -------------------------------------------------------------------- */
/*     Reports more detailed error information from SDE                 */
/* -------------------------------------------------------------------- */
static void sde_error(long error_code, char *routine, char *sde_routine)
{
  char error_string[SE_MAX_MESSAGE_LENGTH];

  error_string[0] = '\0';
  SE_error_get_string(error_code, error_string);

  msSetError( MS_SDEERR,
              "%s: %s. (%ld)",
              routine,
              sde_routine,
              error_string,
              error_code);

  return;
}


/* -------------------------------------------------------------------- */
/* msSDELayerGetRowIDColumn                                             */
/* -------------------------------------------------------------------- */
/*     A helper function to return unique row ID column for             */
/*     an opened SDE layer.  The caller owns the string.                */
/* -------------------------------------------------------------------- */
char *msSDELayerGetRowIDColumn(layerObj *layer)
{
#ifdef USE_SDE
  SE_INT32 status, column_type;
  char* column_name;
  char* full_column_name;
  char* proc_key;

  SE_REGINFO registration;

  msSDELayerInfo *sde=NULL;
  sde = layer->layerinfo;

  if(!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "msSDELayerGetRowIDColumn()");
    return NULL;
  }

  column_name = (char*) msSmallMalloc(SE_QUALIFIED_COLUMN_LEN+1);
  column_name[0]='\0';

  proc_key = msLayerGetProcessingKey(layer,"OBJECTID");
  if (proc_key)
    strcpy(column_name,proc_key);

  if (proc_key) {
    if (layer->debug)
      msDebug("msSDELayerGetRowIDColumn(): Column was manually set to %s\n", column_name);
    return column_name;
  }
  full_column_name = (char*) msSmallMalloc(SE_QUALIFIED_COLUMN_LEN+1);
  full_column_name[0]='\0';

  /*
  ** if the state_id is the SE_DEFAULT_STATE_ID, we are
  ** assuming no versioned queries are happening at all
  ** and we are using the hardcoded row_id column.
  */
  if (sde->state_id == SE_DEFAULT_STATE_ID) {
    if(layer->debug) {
      msDebug("msSDELayerGetRowIDColumn(): State ID was "
              "SE_DEFAULT_STATE_ID, reverting to %s.\n",
              MS_SDE_ROW_ID_COLUMN);
    }
    strcpy(column_name,MS_SDE_ROW_ID_COLUMN);
  }

  /*
  ** if the state_id was not set to SE_DEFAULT_STATE_ID,
  ** check if the table is registered, and if so, use the
  ** registration info to tell us what the row_id column is.
  */
  status = SE_reginfo_create (&registration);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "msSDELayerGetRowIDColumn()",
                "SE_reginfo_create()");
    return(NULL);
  }

  status = SE_registration_get_info ( sde->connPoolInfo->connection,
                                      sde->table,
                                      registration);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "msSDELayerGetRowIDColumn()",
                "SE_registration_get_info()");
    SE_reginfo_free(registration);
    return(NULL);
  }

  status= SE_reginfo_get_rowid_column ( registration,
                                        column_name,
                                        &column_type);

  if(status != SE_SUCCESS) {
    sde_error(status,
              "msSDELayerGetRowIDColumn()",
              "SE_reginfo_get_rowid_column()");
    SE_reginfo_free(registration);
    return(NULL);
  }
  /* Free up the reginfo now that we're done with it */
  SE_reginfo_free(registration);

  /* if the table wasn't registered, return the hard-coded row_id column. */
  if (column_type == SE_REGISTRATION_ROW_ID_COLUMN_TYPE_NONE) {
    if(layer->debug) {
      msDebug("msSDELayerGetRowIDColumn(): Table was not registered, "
              "returning %s.\n",
              MS_SDE_ROW_ID_COLUMN);
    }
    strcpy(column_name, MS_SDE_ROW_ID_COLUMN);
  }


  proc_key = msLayerGetProcessingKey(layer,"ATTRIBUTE_QUALIFIED");
  if (sde->join_table ||
      (proc_key && strcasecmp( proc_key, "TRUE") == 0)) {
    strcat(full_column_name, sde->table);
    strcat(full_column_name, ".");
    strcat(full_column_name, column_name);
    msFree(column_name);

  } else {
    strcpy(full_column_name, column_name);
    msFree(column_name);
  }


  if (full_column_name) {
    return (full_column_name);
  } else {
    msFree(full_column_name);
    return(msStrdup(MS_SDE_ROW_ID_COLUMN));
  }

#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerGetRowIDColumn()");
  return(NULL);
#endif
}


/* -------------------------------------------------------------------- */
/* msSDELCacheAdd                                                       */
/* -------------------------------------------------------------------- */
/*     Adds a SDE layer to the global layer cache.                      */
/* -------------------------------------------------------------------- */
long msSDELCacheAdd( layerObj *layer,
                     SE_LAYERINFO layerinfo,
                     char *tableName,
                     char *columnName,
                     char *connectionString)
{

  layerId *lid = NULL;
  SE_INT32 status = 0;

  msAcquireLock( TLOCK_SDE );

  if (layer->debug) {
    msDebug( "%s: Caching id for %s, %s, %s\n", "msSDELCacheAdd()",
             tableName, columnName, connectionString);
  }

  /* Ensure the cache is large enough to hold the new item. */
  if(lcacheCount == lcacheMax) {
    lcacheMax += 10;
    lcache = (layerId *)realloc(lcache, sizeof(layerId) * lcacheMax);
    if(lcache == NULL) {
      msReleaseLock( TLOCK_SDE );
      msSetError(MS_MEMERR, NULL, "msSDELCacheAdd()");
      return (MS_FAILURE);
    }
  }

  /* Population the new lcache object. */
  lid = lcache + lcacheCount;
  lcacheCount++;

  status = SE_layerinfo_get_id(layerinfo, &lid->layerId);
  if(status != SE_SUCCESS) {
    msReleaseLock( TLOCK_SDE );
    sde_error(status, "msSDELCacheAdd()", "SE_layerinfo_get_id()");
    return(MS_FAILURE);
  }

  lid->table = msStrdup(tableName);
  lid->column = msStrdup(columnName);
  lid->connection = msStrdup(connectionString);

  msReleaseLock( TLOCK_SDE );
  return (MS_SUCCESS);
}

/* -------------------------------------------------------------------- */
/* msSDEGetLayerInfo                                                    */
/* -------------------------------------------------------------------- */
/*     Get a LayerInfo for the layer.  Cached layer is used if it       */
/*     exists in the cache.                                             */
/* -------------------------------------------------------------------- */
long msSDEGetLayerInfo(layerObj *layer,
                       SE_CONNECTION conn,
                       char *tableName,
                       char *columnName,
                       char *connectionString,
                       SE_LAYERINFO layerinfo)
{
  int i;
  long status;
  layerId *lid = NULL;

  /* If table or column are null, nothing can be done. */
  if(tableName == NULL) {
    msSetError( MS_MISCERR,
                "Missing table name.\n",
                "msSDEGetLayerInfo()");
    return (MS_FAILURE);
  }
  if(columnName == NULL) {
    msSetError( MS_MISCERR,
                "Missing column name.\n",
                "msSDEGetLayerInfo()");
    return (MS_FAILURE);
  }
  if(connectionString == NULL) {
    msSetError( MS_MISCERR,
                "Missing connection string.\n",
                "msSDEGetLayerInfo()");
    return (MS_FAILURE);
  }

  if (layer->debug) {
    msDebug("%s: Looking for layer by %s, %s, %s\n",
            "msSDEGetLayerInfo()",
            tableName,
            columnName,
            connectionString);
  }

  /* Search the lcache for the layer id. */
  for(i = 0; i < lcacheCount; i++) {
    lid = lcache + i;
    if(strcasecmp(lid->table, tableName) == 0 &&
        strcasecmp(lid->column, columnName) == 0 &&
        strcasecmp(lid->connection, connectionString) == 0) {
      status = SE_layer_get_info_by_id(conn, lid->layerId, layerinfo);
      if(status != SE_SUCCESS) {
        sde_error(status, "msSDEGetLayerInfo()", "SE_layer_get_info()");
        return(MS_FAILURE);
      }
      if (layer->debug) {
        msDebug( "%s: Matched layer to id %i.\n",
                 "msSDEGetLayerId()", lid->layerId);
      }
      return (MS_SUCCESS);

    }
  }
  if (layer->debug) {
    msDebug("%s: No cached layerid found.\n", "msSDEGetLayerInfo()");
  }

  /* No matches found, create one. */
  status = SE_layer_get_info( conn, tableName, columnName, layerinfo );
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDEGetLayerInfo()", "SE_layer_get_info()");
    return(MS_FAILURE);
  }

  status = msSDELCacheAdd(layer,
                          layerinfo,
                          tableName,
                          columnName,
                          connectionString);
  return(MS_SUCCESS);

}

/* -------------------------------------------------------------------- */
/* sdeShapeCopy                                                         */
/* -------------------------------------------------------------------- */
/*     Copies a SDE shape into a MapServer shapeObj                     */
/* -------------------------------------------------------------------- */
static int sdeShapeCopy(SE_SHAPE inshp, shapeObj *outshp)
{

  SE_POINT *points=NULL;
  SE_ENVELOPE envelope;
  SE_INT32 type, status;
  SE_INT32 *part_offsets = NULL;
  SE_INT32 *subpart_offsets = NULL;
  SE_INT32 num_parts = -1;
  SE_INT32 num_subparts = -1;
  SE_INT32 num_points = -1;

  lineObj line= {0,NULL};

  int i,j,k;

  status = SE_shape_get_type(inshp, &type);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeShapeCopy()", "SE_shape_get_type()");
    return(MS_FAILURE);
  }

  switch(type) {
    case(SG_NIL_SHAPE):
      return(MS_SUCCESS); /* skip null shapes */
      break;
    case(SG_POINT_SHAPE):
    case(SG_MULTI_POINT_SHAPE):
      outshp->type = MS_SHAPE_POINT;
      break;
    case(SG_LINE_SHAPE):
    case(SG_SIMPLE_LINE_SHAPE):
    case(SG_MULTI_LINE_SHAPE):
    case(SG_MULTI_SIMPLE_LINE_SHAPE):
      outshp->type = MS_SHAPE_LINE;
      break;
    case(SG_AREA_SHAPE):
    case(SG_MULTI_AREA_SHAPE):
      outshp->type = MS_SHAPE_POLYGON;
      break;
    default:
      msSetError( MS_SDEERR, "Unsupported SDE shape type.", "sdeShapeCopy()");
      return(MS_FAILURE);
  }


  status = SE_shape_get_num_parts (inshp, &num_parts, &num_subparts);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeShapeCopy()", "SE_shape_get_num_parts()");
    return(MS_FAILURE);
  }
  status = SE_shape_get_num_points (inshp, 0, 0, &num_points);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeShapeCopy()", "SE_shape_get_num_points()");
    return(MS_FAILURE);
  }

  part_offsets = (SE_INT32 *) msSmallMalloc( (num_parts + 1) * sizeof(SE_INT32));
  subpart_offsets = (SE_INT32 *) msSmallMalloc( (num_subparts + 1)  * sizeof(SE_INT32));
  part_offsets[num_parts] = num_subparts;
  subpart_offsets[num_subparts] = num_points;

  points = (SE_POINT *) msSmallMalloc (num_points*sizeof(SE_POINT));
  if(!points) {
    msSetError( MS_MEMERR, "Unable to allocate points array.", "sdeShapeCopy()");
    return(MS_FAILURE);
  }

  status = SE_shape_get_all_points(inshp, SE_DEFAULT_ROTATION, part_offsets, subpart_offsets, points, NULL, NULL);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeCopyShape()", "SE_shape_get_all_points()");
    return(MS_FAILURE);
  }

  k = 0; /* overall point counter */
  for(i=0; i<num_subparts; i++) {

    if( i == num_subparts-1)
      line.numpoints = num_points - subpart_offsets[i];
    else
      line.numpoints = subpart_offsets[i+1] - subpart_offsets[i];

    line.point = (pointObj *)malloc(sizeof(pointObj)*line.numpoints);
    MS_CHECK_ALLOC(line.point, sizeof(pointObj)*line.numpoints, MS_FAILURE);

    for(j=0; j < line.numpoints; j++) {
      line.point[j].x = points[k].x;
      line.point[j].y = points[k].y;
      k++;
    }

    msAddLine(outshp, &line);
    msFree(line.point);
  }

  msFree(part_offsets);
  msFree(subpart_offsets);
  msFree(points);

  /* finally copy the bounding box for the entire shape */
  status = SE_shape_get_extent(inshp, 0, &envelope);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeShapeCopy()", "SE_shape_get_extent()");
    return(MS_FAILURE);
  }
  outshp->bounds.minx = envelope.minx;
  outshp->bounds.miny = envelope.miny;
  outshp->bounds.maxx = envelope.maxx;
  outshp->bounds.maxy = envelope.maxy;

  return(MS_SUCCESS);
}

/* -------------------------------------------------------------------- */
/* sdeGetRecord                                                         */
/* -------------------------------------------------------------------- */
/*     Retrieves the current row as setup via the SDE stream query      */
/*     or row fetch routines.                                           */
/* -------------------------------------------------------------------- */
static int sdeGetRecord(layerObj *layer, shapeObj *shape)
{
  int i;
  SE_INT32 status;

  double doubleval;
  SE_INT32 longval;
  struct tm dateval;

  short shortval; /* new gdv */
  float floatval;

  SE_COLUMN_DEF *itemdefs;
  SE_SHAPE shapeval=0;
  msSDELayerInfo *sde;

  SE_BLOB_INFO blobval;

#ifdef SE_CLOB_TYPE
  SE_CLOB_INFO clobval;
#endif

#ifdef SE_NCLOB_TYPE
  SE_NCLOB_INFO nclobval;
  SE_WCHAR* nclobstring; /* null terminated */
#endif

#ifdef SE_NSTRING_TYPE
  SE_WCHAR* wide=NULL;
#endif


  if(!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "sdeGetRecord()");
    return MS_FAILURE;
  }

  sde = layer->layerinfo;

  if(layer->numitems > 0) {
    shape->numvalues = layer->numitems;
    shape->values = (char **) malloc (sizeof(char *)*layer->numitems);
    MS_CHECK_ALLOC(shape->values, sizeof(char *)*layer->numitems, MS_FAILURE);
  }

  status = SE_shape_create(NULL, &shapeval);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeGetRecord()", "SE_shape_create()");
    return(MS_FAILURE);
  }

  itemdefs = layer->iteminfo;
  for(i=0; i<layer->numitems; i++) {

    /* do something special */
    if(strcmp(layer->items[i],sde->row_id_column) == 0) {
      SE_INT32 shape_index;      

      status = SE_stream_get_integer(sde->connPoolInfo->stream, (short)(i+1), &shape_index);
      if(status != SE_SUCCESS) {
        sde_error(status, "sdeGetRecord()", "SE_stream_get_integer()");
        return(MS_FAILURE);
      }
      shape->index = shape_index;

      shape->values[i] = (char *)msSmallMalloc(64); /* should be enough */
      sprintf(shape->values[i], "%ld", shape->index);
      continue;
    }

    switch(itemdefs[i].sde_type) {
      case SE_SMALLINT_TYPE:
        /* changed by gdv */
        status = SE_stream_get_smallint(sde->connPoolInfo->stream, (short)(i+1), &shortval);
        if(status == SE_SUCCESS)
          shape->values[i] = msLongToString(shortval);
        else if(status == SE_NULL_VALUE)
          shape->values[i] = msStrdup(MS_SDE_NULLSTRING);
        else {
          sde_error(status, "sdeGetRecord()", "SE_stream_get_smallint()");
          return(MS_FAILURE);
        }
        break;
      case SE_INTEGER_TYPE:
        status = SE_stream_get_integer(sde->connPoolInfo->stream, (short)(i+1), &longval);
        if(status == SE_SUCCESS)
          shape->values[i] = msLongToString(longval);
        else if(status == SE_NULL_VALUE)
          shape->values[i] = msStrdup(MS_SDE_NULLSTRING);
        else {
          sde_error(status, "sdeGetRecord()", "SE_stream_get_integer()");
          return(MS_FAILURE);
        }
        break;
      case SE_FLOAT_TYPE:
        status = SE_stream_get_float(sde->connPoolInfo->stream, (short)(i+1), &floatval);
        if(status == SE_SUCCESS)
          shape->values[i] = msDoubleToString(floatval, MS_FALSE);
        else if(status == SE_NULL_VALUE)
          shape->values[i] = msStrdup(MS_SDE_NULLSTRING);
        else {
          sde_error( status, "sdeGetRecord()", "SE_stream_get_float()");
          return(MS_FAILURE);
        }
        break;
      case SE_DOUBLE_TYPE:
        status = SE_stream_get_double(sde->connPoolInfo->stream, (short) (i+1), &doubleval);
        if(status == SE_SUCCESS)
          shape->values[i] = msDoubleToString(doubleval, MS_FALSE);
        else if(status == SE_NULL_VALUE)
          shape->values[i] = msStrdup(MS_SDE_NULLSTRING);
        else {
          sde_error(status, "sdeGetRecord()", "SE_stream_get_double()");
          return(MS_FAILURE);
        }
        break;
      case SE_STRING_TYPE:
        shape->values[i] = (char *)msSmallMalloc(itemdefs[i].size+1);
        status = SE_stream_get_string(  sde->connPoolInfo->stream,
                                        (short) (i+1),
                                        shape->values[i]);
        if(status == SE_NULL_VALUE)
          shape->values[i][0] = '\0'; /* empty string */
        else if(status != SE_SUCCESS) {
          sde_error(  status,
                      "sdeGetRecord()",
                      "SE_stream_get_string()");
          return(MS_FAILURE);
        }
        break;
#ifdef SE_NSTRING_TYPE
      case SE_NSTRING_TYPE:
        wide = (SE_WCHAR *)msSmallMalloc(itemdefs[i].size*2*sizeof(SE_WCHAR)+1);
        memset(wide, 0, itemdefs[i].size*2*sizeof(SE_WCHAR)+1);
        status = SE_stream_get_nstring( sde->connPoolInfo->stream,
                                        (short) (i+1),
                                        wide);
        if(status == SE_NULL_VALUE) {
          shape->values[i] = (char *)msSmallMalloc(itemdefs[i].size*sizeof(char)+1);
          shape->values[i][0] = '\0'; /* empty string */
          msFree(wide);
        } else if(status != SE_SUCCESS) {
          sde_error(  status,
                      "sdeGetRecord()",
                      "SE_stream_get_string()");
          return(MS_FAILURE);
        } else {
          if (sde->bBigEndian)
            shape->values[i] = msConvertWideStringToUTF8((const wchar_t*) wide, "UTF-16BE");
          else
            shape->values[i] = msConvertWideStringToUTF8((const wchar_t*) wide, "UTF-16LE");
          msFree(wide);
          if (!shape->values[i]) {  /* There was an error */
            msSetError( MS_SDEERR,
                        "msConvertWideStringToUTF8()==NULL.",
                        "sdeGetRecord()");
            shape->values[i] = (char *)msSmallMalloc(itemdefs[i].size*sizeof(char)+1);
            shape->values[i][0] = '\0'; /* empty string */
          }
        }
        break;
#endif

#ifdef SE_UUID_TYPE

      case SE_UUID_TYPE:
        shape->values[i] = (char *)msSmallMalloc(itemdefs[i].size+1);
        status = SE_stream_get_uuid  (  sde->connPoolInfo->stream,
                                        (short) (i+1),
                                        shape->values[i]);
        if(status == SE_NULL_VALUE)
          shape->values[i][0] = '\0'; /* empty string */
        else if(status != SE_SUCCESS) {
          sde_error(  status,
                      "sdeGetRecord()",
                      "SE_stream_get_uuid()");
          return(MS_FAILURE);
        }
        break;

#endif

#ifdef SE_CLOB_TYPE

      case SE_CLOB_TYPE:
        memset(&clobval, 0, sizeof(clobval)); /* to prevent from the crash in SE_stream_get_clob */
        status = SE_stream_get_clob(sde->connPoolInfo->stream, (short) (i+1), &clobval);
        if(status == SE_SUCCESS) {
          shape->values[i] = (char *)msSmallMalloc(sizeof(char)*clobval.clob_length);
          shape->values[i] = memcpy(  shape->values[i],
                                      clobval.clob_buffer,
                                      clobval.clob_length);
          SE_clob_free(&clobval);
        } else if (status == SE_NULL_VALUE) {
          shape->values[i] = msStrdup(MS_SDE_NULLSTRING);
        } else {
          sde_error(  status,
                      "sdeGetRecord()",
                      "SE_stream_get_clob()");
          return(MS_FAILURE);
        }
        break;
#endif

#ifdef SE_CLOB_TYPE

      case SE_NCLOB_TYPE:
        memset(&nclobval, 0, sizeof(nclobval)); /* to prevent from the crash in SE_stream_get_nclob */
        status = SE_stream_get_nclob(sde->connPoolInfo->stream, (short) (i+1), &nclobval);
        if(status == SE_SUCCESS) {
          /* the returned string is not null-terminated */
          nclobstring = (SE_WCHAR*)malloc(sizeof(char)*(nclobval.nclob_length+2));
          memcpy(nclobstring, nclobval.nclob_buffer, nclobval.nclob_length);
          nclobstring[nclobval.nclob_length / 2] = '\0';

          if (sde->bBigEndian)
            shape->values[i] = msConvertWideStringToUTF8((const wchar_t*) nclobstring, "UTF-16BE");
          else
            shape->values[i] = msConvertWideStringToUTF8((const wchar_t*) nclobstring, "UTF-16LE");
      
          if (!shape->values[i]) {  /* There was an error */
            msSetError( MS_SDEERR,
                        "msConvertWideStringToUTF8()==NULL.",
                        "sdeGetRecord()");
            shape->values[i] = (char *)malloc(itemdefs[i].size*sizeof(char)+1);
            shape->values[i][0] = '\0'; /* empty string */
          }                

          SE_nclob_free(&nclobval);
          msFree(nclobstring);
        } else if (status == SE_NULL_VALUE) {
          shape->values[i] = msStrdup(MS_SDE_NULLSTRING);
        } else {
          sde_error(  status,
                      "sdeGetRecord()",
                      "SE_stream_get_nclob()");
          return(MS_FAILURE);
        }
        break;
#endif

      case SE_BLOB_TYPE:
        status = SE_stream_get_blob(sde->connPoolInfo->stream, (short) (i+1), &blobval);
        if(status == SE_SUCCESS) {
          shape->values[i] = (char *)msSmallMalloc(sizeof(char)*blobval.blob_length);
          shape->values[i] = memcpy(  shape->values[i],
                                      blobval.blob_buffer,
                                      blobval.blob_length);
          SE_blob_free(&blobval);
        } else if (status == SE_NULL_VALUE) {
          shape->values[i] = msStrdup(MS_SDE_NULLSTRING);
        } else {
          sde_error(  status,
                      "sdeGetRecord()",
                      "SE_stream_get_blob()");
          return(MS_FAILURE);
        }
        break;

      case SE_DATE_TYPE:
        status = SE_stream_get_date(sde->connPoolInfo->stream, (short)(i+1), &dateval);
        if(status == SE_SUCCESS) {
          shape->values[i] = (char *)msSmallMalloc(sizeof(char)*MS_SDE_TIMEFMTSIZE);
          strftime(   shape->values[i],
                      MS_SDE_TIMEFMTSIZE,
                      MS_SDE_TIMEFMT,
                      &dateval);
        } else if(status == SE_NULL_VALUE)
          shape->values[i] = msStrdup(MS_SDE_NULLSTRING);
        else {
          sde_error(  status,
                      "sdeGetRecord()",
                      "SE_stream_get_date()");
          return(MS_FAILURE);
        }
        break;
      case SE_SHAPE_TYPE:
        status = SE_stream_get_shape(sde->connPoolInfo->stream, (short)(i+1), shapeval);
        if(status == SE_SUCCESS)
          shape->values[i] = msStrdup(MS_SDE_SHAPESTRING);
        else if(status == SE_NULL_VALUE)
          shape->values[i] = msStrdup(MS_SDE_NULLSTRING);
        else {
          sde_error(  status,
                      "sdeGetRecord()",
                      "SE_stream_get_shape()");
          return(MS_FAILURE);
        }
        break;
      default:
        msSetError(   MS_SDEERR,
                      "Unknown SDE column type.",
                      "sdeGetRecord()");
        return(MS_FAILURE);
        break;
    } /* switch(itemdefs[i].sde_type) */
  } /*     for(i=0; i<layer->numitems; i++) { */

  if(SE_shape_is_nil(shapeval)) return(MS_SUCCESS);

  /* copy sde shape to a mapserver shape */
  status = sdeShapeCopy(shapeval, shape);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  /* clean up */
  SE_shape_free(shapeval);


  return(MS_SUCCESS);
}

static SE_QUERYINFO getSDEQueryInfo(layerObj *layer)
{
  SE_QUERYINFO query_info;
  long status;

  msSDELayerInfo *sde=NULL;

  if(!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "getSDEQueryInfo()");
    return(NULL);
  }

  sde = layer->layerinfo;

  /*
  ** See http://forums.esri.com/Thread.asp?c=2&f=59&t=108929&mc=4#msgid310273
  ** SE_queryinfo is a new SDE struct in ArcSDE 8.x that is a bit easier
  ** (and faster) to use.  HCB
  */
  status = SE_queryinfo_create (&query_info);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "getSDEQueryInfo()",
                "SE_queryinfo_create()");
    return(NULL);
  }

  /* set the tables -- just one at this point */
  status = SE_queryinfo_set_tables (  query_info,
                                      1,
                                      (const CHAR **) &(sde->table),
                                      NULL);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "getSDEQueryInfo()",
                "SE_queryinfo_create()");
    return(NULL);
  }

  /* set the "where" clause */
  if(!(layer->filter.native_string))
    /* set to empty string */
    status = SE_queryinfo_set_where_clause (query_info, (const CHAR * ) "");
  else
    /* set to the layer's filter.native_string */
    status = SE_queryinfo_set_where_clause (query_info, (const CHAR * ) (layer->filter.native_string));
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "getSDEQueryInfo()",
                "SE_queryinfo_set_where_clause()");
    return(NULL);
  }

  status = SE_queryinfo_set_columns(  query_info,
                                      layer->numitems,
                                      (const char **)layer->items);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "getSDEQueryInfo()",
                "SE_queryinfo_set_columns()");
    return(NULL);
  }

  /* Join the spatial and feature tables.  If we specify the type of join */
  /* we'll query faster than querying all tables individually (old method) */
  status = SE_queryinfo_set_query_type (query_info,SE_QUERYTYPE_JSF);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "getSDEQueryInfo()",
                "SE_queryinfo_set_query_type()");
    return(NULL);
  }

  return query_info;
}

static SE_SQL_CONSTRUCT* getSDESQLConstructInfo(layerObj *layer, long* id)
{
  SE_SQL_CONSTRUCT* sql;

  char *full_filter=NULL;
  char *pszId=NULL;
  long status;

  msSDELayerInfo *sde=NULL;
  full_filter = (char*) msSmallMalloc((1000+1)*sizeof (char));
  full_filter[0] = '\0';
  if(!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "getSDESQLConstructInfo()");
    return(NULL);
  }

  sde = layer->layerinfo;

  if (!sde->join_table) {
    msSetError( MS_SDEERR,
                "Join table is null, we should be using QueryInfo.",
                "getSDESQLConstructInfo()");
    return(NULL);
  }

  status = SE_sql_construct_alloc( 2, &sql);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "getSDESQLConstructInfo()",
                "SE_sql_construct_alloc()");
    return(NULL);
  }

  strcpy(sql->tables[0], sde->table); /* main table */
  strcpy(sql->tables[1], sde->join_table); /* join table */

  /* If we were given an ID *and* we have a join, we need to
     set our FILTER statement to reflect this. */
  if ((sde->join_table) && (id != NULL)) {
    pszId = msLongToString(*id);
    strcat(full_filter, layer->filter.native_string);
    strcat(full_filter, " AND ");
    strcat(full_filter, sde->row_id_column);
    strcat(full_filter, "=");
    strcat(full_filter, pszId);
    msFree(pszId);
    sql->where = msStrdup(full_filter);


  } else {
    sql->where = layer->filter.native_string;
  }

  msFree(full_filter);

  if (layer->debug) msDebug("WHERE statement: %s\n", sql->where);
  return sql;
}
#endif

/************************************************************************/
/*       Start SDE/MapServer library functions.                         */
/*                                                                      */
/************************************************************************/

/* -------------------------------------------------------------------- */
/* msSDELayerOpen                                                       */
/* -------------------------------------------------------------------- */
/*     Connects to SDE using the SDE C API.  Connections are pooled     */
/*     using the MapServer pooling API.  After a connection is made,    */
/*     a query stream is created, using the SDE version specified in    */
/*     the DATA string, or SDE.DEFAULT if not specified.  It is         */
/*     important to note that the SE_CONNECTION is shared across data   */
/*     layers, but the state_id for a layer's version is different,     */
/*     even for layers with the same version name.  These are *not*     */
/*     shared across layers.                                            */
/* -------------------------------------------------------------------- */
int msSDELayerOpen(layerObj *layer)
{
#ifdef USE_SDE
  long status=-1;
  int endian_test=1;
  char **params=NULL;
  char **data_params=NULL;
  char *join_table=NULL;

  int numparams=0;
  SE_ERROR hSDEError;
  SE_STATEINFO state;
  SE_VERSIONINFO version;

  SE_ENVELOPE envelope;

  msSDELayerInfo *sde = NULL;
  msSDEConnPoolInfo *poolinfo;

  /* allocate space for SDE structures */
  sde = (msSDELayerInfo *) malloc(sizeof(msSDELayerInfo));
  MS_CHECK_ALLOC(sde, sizeof(msSDELayerInfo), MS_FAILURE);

  sde->state_id = SE_BASE_STATE_ID;

  /* initialize the table and spatial column names */
  sde->table = NULL;
  sde->column = NULL;
  sde->row_id_column = NULL;
  sde->join_table = NULL;
  sde->basedefs = NULL;
  sde->joindefs = NULL;
  sde->extent = (rectObj *) msSmallMalloc(sizeof(rectObj));

  sde->nBaseColumns = (short *) msSmallMalloc(1*sizeof(short));
  *(sde->nBaseColumns) = 0;
  sde->nJoinColumns = (short *) msSmallMalloc(1*sizeof(short));
  *(sde->nJoinColumns) = 0;

  if(!sde->extent) {
    msSetError( MS_MEMERR,
                "Error allocating extent for SDE layer",
                "msSDELayerOpen()");
    return(MS_FAILURE);
  }

  /* request a connection and stream from the pool */
  poolinfo = (msSDEConnPoolInfo *)msConnPoolRequest( layer );

#ifdef SE_connection_test_server
  /* check the connection */
  if (poolinfo && (SE_connection_test_server(poolinfo->connection, 30) != SE_SUCCESS)) {
    msSDECloseConnection(poolinfo->connection);
  }
#endif

  /* If we weren't returned a connection and stream, initialize new ones */
  if (!poolinfo) {
    char *conn_decrypted;

    if (layer->debug)
      msDebug("msSDELayerOpen(): "
              "Layer %s opened from scratch.\n", layer->name);


    poolinfo = (msSDEConnPoolInfo *)malloc(sizeof *poolinfo);
    MS_CHECK_ALLOC(poolinfo, sizeof *poolinfo, MS_FAILURE);

    /* Decrypt any encrypted token in the connection string */
    conn_decrypted = msDecryptStringTokens(layer->map, layer->connection);

    /* An error should already have been produced */
    if (conn_decrypted == NULL) {
      return(MS_FAILURE);
    }
    /* Split the connection parameters and make sure we have enough of them */
    params = msStringSplit(conn_decrypted, ',', &numparams);
    if(!params) {
      msSetError( MS_MEMERR,
                  "Error splitting SDE connection information.",
                  "msSDELayerOpen()");
      msFree(conn_decrypted);
      return(MS_FAILURE);
    }
    msFree(conn_decrypted);
    conn_decrypted = NULL;

    if(numparams < 5) {
      msSetError( MS_SDEERR,
                  "Not enough SDE connection parameters specified.",
                  "msSDELayerOpen()");
      return(MS_FAILURE);
    }

    /* Create the connection handle and put into poolinfo->connection */
    status = SE_connection_create(params[0],
                                  params[1],
                                  params[2],
                                  params[3],
                                  params[4],
                                  &(hSDEError),
                                  &(poolinfo->connection));

    if(status != SE_SUCCESS) {
      sde_error(  status,
                  "msSDELayerOpen()",
                  "SE_connection_create()");
      return(MS_FAILURE);
    }

    /* ------------------------------------------------------------------------- */
    /* Set the concurrency type for the connection.  SE_UNPROTECTED_POLICY is    */
    /* suitable when only one thread accesses the specified connection.          */
    /* ------------------------------------------------------------------------- */
    status = SE_connection_set_concurrency( poolinfo->connection,
                                            SE_UNPROTECTED_POLICY);


    if(status != SE_SUCCESS) {
      sde_error(  status,
                  "msSDELayerOpen()",
                  "SE_connection_set_concurrency()");
      return(MS_FAILURE);
    }


    status = SE_stream_create(poolinfo->connection, &(poolinfo->stream));
    if(status != SE_SUCCESS) {
      sde_error(  status,
                  "msSDELayerOpen()",
                  "SE_stream_create()");
      return(MS_FAILURE);
    }

    /* Register the connection with the connection pooling API.  Give  */
    /* msSDECloseConnection as the function to call when we run out of layer  */
    /* instances using it */
    msConnPoolRegister(layer, poolinfo, msSDECloseConnection);
    msFreeCharArray(params, numparams); /* done with parameter list */
  } /* !poolinfo */

  /* Split the DATA member into its parameters using the comma */
  /* Periods (.) are used to denote table names and schemas in SDE,  */
  /* as are underscores (_). */
  data_params = msStringSplit(layer->data, ',', &numparams);
  if(!data_params) {
    msSetError( MS_MEMERR,
                "Error splitting SDE layer information.",
                "msSDELayerOpen()");
    return(MS_FAILURE);
  }

  if(numparams < 2) {
    msSetError( MS_SDEERR,
                "Not enough SDE layer parameters specified.",
                "msSDELayerOpen()");
    return(MS_FAILURE);
  }

  sde->table = msStrdup(data_params[0]);
  sde->column = msStrdup(data_params[1]);

  join_table = msLayerGetProcessingKey(layer,"JOINTABLE");
  if (join_table) {
    sde->join_table = msStrdup(join_table);
    /* msFree(join_table); */
  }
  if (numparams < 3) {
    /* User didn't specify a version, we won't use one */
    if (layer->debug) {
      msDebug("msSDELayerOpen(): Layer %s did not have a "
              "specified version.\n",
              layer->name);
    }
    sde->state_id = SE_DEFAULT_STATE_ID;
  } else {
    /* A version was specified... obtain the state_id */
    /* for it. */
    if (layer->debug) {
      msDebug("msSDELayerOpen(): Layer %s specified version %s.\n",
              layer->name,
              data_params[2]);
    }
    status = SE_versioninfo_create (&(version));
    if(status != SE_SUCCESS) {
      sde_error(  status,
                  "msSDELayerOpen()",
                  "SE_versioninfo_create()");
      return(MS_FAILURE);
    }
    status = SE_version_get_info(poolinfo->connection, data_params[2], version);

    if(status != SE_SUCCESS) {

      if (status == SE_INVALID_RELEASE) {
        /* The user has incongruent versions of SDE, ie 8.2 client and  */
        /* 8.3 server set the state_id to SE_DEFAULT_STATE_ID, which means    */
        /* no version queries are done */
        sde->state_id = SE_DEFAULT_STATE_ID;
      } else {
        sde_error(  status,
                    "msSDELayerOpen()",
                    "SE_version_get_info()");
        SE_versioninfo_free(version);
        return(MS_FAILURE);
      }
    } /* couldn't get version info */

  } /* version was specified */

  /* Get the STATEID from the given version and set the stream to  */
  /* that if we didn't already set it to SE_DEFAULT_STATE_ID.   */
  if (sde->state_id != SE_DEFAULT_STATE_ID) {
    status = SE_versioninfo_get_state_id(version, &sde->state_id);
    if(status != SE_SUCCESS) {
      sde_error(  status,
                  "msSDELayerOpen()",
                  "SE_versioninfo_get_state_id()");
      SE_versioninfo_free(version);
      return(MS_FAILURE);
    }

    SE_versioninfo_free(version);
    status = SE_stateinfo_create (&state);
    if(status != SE_SUCCESS) {
      sde_error(  status,
                  "msSDELayerOpen()",
                  "SE_stateinfo_create()");
      return(MS_FAILURE);
    }
    status = SE_state_get_info( poolinfo->connection,
                                sde->state_id,
                                state);
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_state_get_info()");
      SE_stateinfo_free (state);
      return(MS_FAILURE);
    }
    if (SE_stateinfo_is_open (state)) {
      /* If the state is open for edits, we shouldn't be querying from it */
      sde_error(  status,
                  "msSDELayerOpen()",
                  "SE_stateinfo_is_open() -- State for version is open");
      SE_stateinfo_free (state);
      return(MS_FAILURE);
    }
    SE_stateinfo_free (state);
  } /* if (!(sde->state_id == SE_DEFAULT_STATE_ID)) */


  /* done with the DATA stuff now */
  msFreeCharArray(data_params, numparams);

  status = SE_layerinfo_create(NULL, &(sde->layerinfo));
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_layerinfo_create()");
    return(MS_FAILURE);
  }


  status = msSDEGetLayerInfo( layer,
                              poolinfo->connection,
                              sde->table,
                              sde->column,
                              layer->connection,
                              sde->layerinfo);

  if(status != MS_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "msSDEGetLayerInfo()");
    return(MS_FAILURE);
  }

  status = SE_coordref_create(&(sde->coordref));
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_coordref_create()");
    return(MS_FAILURE);
  }

  status = SE_layerinfo_get_coordref(sde->layerinfo, sde->coordref);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_layerinfo_get_coordref()");
    return(MS_FAILURE);
  }

  /* Get the layer extent and hang it on the layerinfo */
  status = SE_layerinfo_get_envelope(sde->layerinfo, &envelope);
  if(status != SE_SUCCESS) {
    sde_error(status,
              "msSDELayerOpen()",
              "SE_layerinfo_get_envelope()");
    return(MS_FAILURE);
  }

  sde->extent->minx = envelope.minx;
  sde->extent->miny = envelope.miny;
  sde->extent->maxx = envelope.maxx;
  sde->extent->maxy = envelope.maxy;

  /* reset the stream */
  status = SE_stream_close(poolinfo->stream, 1);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_stream_close()");
    return(MS_FAILURE);
  }

  /* Determine if we are big or little- endian for */
  /* working with the encoding */
  if( *((unsigned char *) &endian_test) == 1 )
    sde->bBigEndian = MS_FALSE;
  else
    sde->bBigEndian = MS_TRUE;

  /* point to the SDE layer information  */
  /* (note this might actually be in another layer) */
  layer->layerinfo = sde;
  sde->connPoolInfo = poolinfo;
  return(MS_SUCCESS);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msSDELayerOpen()");
  return(MS_FAILURE);
#endif
}




/* -------------------------------------------------------------------- */
/* msSDELayerClose                                                      */
/* -------------------------------------------------------------------- */
/*     Closes the MapServer layer.  This doesn't necessarily close the  */
/*     connection to the layer.                                         */
/* -------------------------------------------------------------------- */
int  msSDELayerClose(layerObj *layer)
{
#ifdef USE_SDE


  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;

  /* Silently return if layer not opened. */
  if (!msSDELayerIsOpen(layer)) return MS_SUCCESS;

  if(layer->debug)
    msDebug("msSDELayerClose(): Closing layer %s.\n", layer->name);

  if (sde->layerinfo) SE_layerinfo_free(sde->layerinfo);
  if (sde->coordref) SE_coordref_free(sde->coordref);
  if (sde->table) msFree(sde->table);
  if (sde->column) msFree(sde->column);
  if (sde->row_id_column) msFree(sde->row_id_column);
  if (sde->join_table) msFree(sde->join_table);
  if (sde->extent) msFree(sde->extent);
  if (sde->nBaseColumns) msFree(sde->nBaseColumns);
  if (sde->nJoinColumns) msFree(sde->nJoinColumns);

  msConnPoolRelease( layer, sde->connPoolInfo );
  if (layer->layerinfo) msFree(layer->layerinfo);
  layer->layerinfo = NULL;
  return MS_SUCCESS;

#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerClose()");
  return(MS_FALSE);
#endif
}


/* -------------------------------------------------------------------- */
/* msSDELayerCloseConnection                                            */
/* -------------------------------------------------------------------- */
/* Virtual table function                                               */
/* -------------------------------------------------------------------- */
int msSDELayerCloseConnection(layerObj *layer)
{


#ifdef USE_SDE


  msSDELayerInfo *sde=NULL;

  if(!msSDELayerIsOpen(layer)) {
    return MS_SUCCESS;  /* already closed */
  }

  sde = layer->layerinfo;

  if(layer->debug)
    msDebug("msSDELayerCloseConnection(): Closing connection for layer %s.\n", layer->name);

  msConnPoolRelease( layer, sde->connPoolInfo );
  sde->connPoolInfo = NULL;
  return (MS_SUCCESS);
#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerClose()");
  return MS_FAILURE;
#endif

  return MS_SUCCESS;
}


/* -------------------------------------------------------------------- */
/* msSDELayerWhichShapes                                                */
/* -------------------------------------------------------------------- */
/*     starts a stream query using spatial filter.  Also limits the     */
/*     query by the layer's FILTER item as well.                        */
/* -------------------------------------------------------------------- */
int msSDELayerWhichShapes(layerObj *layer, rectObj rect, int isQuery)
{
#ifdef USE_SDE
  long status;
  SE_ENVELOPE envelope;
  SE_SHAPE shape=0;
  SE_FILTER constraint;
  SE_QUERYINFO query_info = NULL;
  SE_SQL_CONSTRUCT* sql = NULL;
  char* proc_value=NULL;
  int query_order=SE_SPATIAL_FIRST;

  msSDELayerInfo *sde=NULL;

  if(!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "msSDELayerWhichShapes()");
    return(MS_FAILURE);
  }

  sde = layer->layerinfo;

  /* use the cached layer's extent. */
  /* there is NO overlap, return MS_DONE */
  /* (FIX: use this in ALL which shapes functions) */
  if(sde->extent->minx > rect.maxx) return(MS_DONE);
  if(sde->extent->maxx < rect.minx) return(MS_DONE);
  if(sde->extent->miny > rect.maxy) return(MS_DONE);
  if(sde->extent->maxy < rect.miny) return(MS_DONE);

  /* set spatial constraint search shape */
  /* crop against SDE layer extent *argh* */
  envelope.minx = MS_MAX(rect.minx, sde->extent->minx);
  envelope.miny = MS_MAX(rect.miny, sde->extent->miny);
  envelope.maxx = MS_MIN(rect.maxx, sde->extent->maxx);
  envelope.maxy = MS_MIN(rect.maxy, sde->extent->maxy);

  if( envelope.minx == envelope.maxx && envelope.miny == envelope.maxy) {
    /* fudge a rectangle so we have a valid one for generate_rectangle */
    /* FIXME: use the real shape for the query and set the filter_type
       to be an appropriate type */
    envelope.minx = envelope.minx - 0.001;
    envelope.maxx = envelope.maxx + 0.001;
    envelope.miny = envelope.miny - 0.001;
    envelope.maxy = envelope.maxy + 0.001;
  }

  status = SE_shape_create(sde->coordref, &shape);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "msSDELayerWhichShapes()",
                "SE_shape_create()");
    return(MS_FAILURE);
  }

  status = SE_shape_generate_rectangle(&envelope, shape);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "msSDELayerWhichShapes()",
                "SE_shape_generate_rectangle()");
    return(MS_FAILURE);
  }
  constraint.filter.shape = shape;

  /* set spatial constraint column and table */
  strcpy(constraint.table, sde->table);
  strcpy(constraint.column, sde->column);

  /* set a couple of other spatial constraint properties */
  constraint.method = SM_ENVP;
  constraint.filter_type = SE_SHAPE_FILTER;
  constraint.truth = TRUE;

  if (!sde->join_table) {
    query_info = getSDEQueryInfo(layer);
    if (!query_info) {
      sde_error(  status,
                  "msSDELayerWhichShapes()",
                  "getSDEQueryInfo()");
      return(MS_FAILURE);
    }
  } else {
    if (!layer->filter.native_string) {
      sde_error(  -51,
                  "msSDELayerWhichShapes()",
                  "A join table is specified, but no FILTER is"
                  " defined that joins the two tables together");
      return(MS_FAILURE);
    }
    sql = getSDESQLConstructInfo(layer, NULL);
  }

  /* reset the stream */
  status = SE_stream_close(sde->connPoolInfo->stream, 1);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "msSDELayerWhichShapes()",
                "SE_stream_close()");
    return(MS_FAILURE);
  }
  /* Set the stream state back to the state_id of our user-specified version */
  /* This must be done every time after the stream is reset before the  */
  /* query happens. */

  if (sde->state_id != SE_DEFAULT_STATE_ID) {

    status =  SE_stream_set_state(sde->connPoolInfo->stream,
                                  sde->state_id,
                                  sde->state_id,
                                  SE_STATE_DIFF_NOCHECK);
    if(status != SE_SUCCESS) {
      sde_error(  status,
                  "msSDELayerOpen()",
                  "SE_stream_set_state()");
      return(MS_FAILURE);
    }
  }

  if (!sql) {
    status = SE_stream_query_with_info(sde->connPoolInfo->stream, query_info);
    if(status != SE_SUCCESS) {
      sde_error(status,
                "msSDELayerWhichShapes()",
                "SE_stream_query_with_info()");
      return(MS_FAILURE);
    }
  } else {
    status = SE_stream_query(sde->connPoolInfo->stream, layer->numitems, (const CHAR**) layer->items, sql);
    if(status != SE_SUCCESS) {
      sde_error(status,
                "msSDELayerWhichShapes()",
                "SE_stream_query()");
      return(MS_FAILURE);
    }
    /* Free up the sql now that we've queried */
    SE_sql_construct_free(sql);
  }
  proc_value = msLayerGetProcessingKey(layer,"QUERYORDER");
  if(proc_value && strcasecmp(proc_value, "ATTRIBUTE") == 0)
    query_order = SE_ATTRIBUTE_FIRST;

  status = SE_stream_set_spatial_constraints( sde->connPoolInfo->stream,
           query_order,
           FALSE,
           1,
           &constraint);

  if(status != SE_SUCCESS) {
    sde_error(  status,
                "msSDELayerWhichShapes()",
                "SE_stream_set_spatial_constraints()");
    return(MS_FAILURE);
  }

  /* *should* be ready to step through shapes now */
  status = SE_stream_execute(sde->connPoolInfo->stream);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "msSDELayerWhichShapes()",
                "SE_stream_execute()");
    return(MS_FAILURE);
  }

  /* clean-up */
  SE_shape_free(shape);
  SE_queryinfo_free (query_info);

  return(MS_SUCCESS);
#else
  msSetError(MS_MISCERR,
             "SDE support is not available.",
             "msSDELayerWhichShapes()");
  return(MS_FAILURE);
#endif
}

/* -------------------------------------------------------------------- */
/* msSDELayerNextShape                                                  */
/* -------------------------------------------------------------------- */
/*     Recursively gets the shapes for the SDE layer                    */
/* -------------------------------------------------------------------- */
int msSDELayerNextShape(layerObj *layer, shapeObj *shape)
{
#ifdef USE_SDE
  long status;

  msSDELayerInfo *sde=NULL;

  if(!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "msSDELayerNextShape()");
    return(MS_FAILURE);
  }

  sde = layer->layerinfo;

  /* fetch the next record from the stream */
  status = SE_stream_fetch(sde->connPoolInfo->stream);

  if(status == SE_FINISHED)
    return(MS_DONE);
  else if(status != MS_SUCCESS) {
    sde_error(  status,
                "msSDELayerNextShape()",
                "SE_stream_fetch()");
    return(MS_FAILURE);
  }

  /* get the shape and values (first column is the shape id,  */
  /* second is the shape itself) */
  status = sdeGetRecord(layer, shape);
  if(status != MS_SUCCESS)
    return(MS_FAILURE); /* something went wrong fetching the record/shape */

  if(shape->numlines == 0) /* null shape, skip it */
    return(msSDELayerNextShape(layer, shape));

  return(MS_SUCCESS);
#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerNextShape()");
  return(MS_FAILURE);
#endif
}

/* -------------------------------------------------------------------- */
/* msSDELayerGetExtent                                                  */
/* -------------------------------------------------------------------- */
/*     Returns the extent of the SDE layer                              */
/* -------------------------------------------------------------------- */
int msSDELayerGetExtent(layerObj *layer, rectObj *extent)
{
#ifdef USE_SDE

  msSDELayerInfo *sde = NULL;

  if(!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "msSDELayerGetExtent()");
    return(MS_FAILURE);
  }

  sde = layer->layerinfo;

  /* copy our cached extent members into the caller's extent */
  extent->minx = sde->extent->minx;
  extent->miny = sde->extent->miny;
  extent->maxx = sde->extent->maxx;
  extent->maxy = sde->extent->maxy;

  return(MS_SUCCESS);
#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerGetExtent()");
  return(MS_FAILURE);
#endif
}

/* -------------------------------------------------------------------- */
/* msSDELayerGetShape                                                   */
/* -------------------------------------------------------------------- */
/*     Queries SDE for a shape (and its attributes, if requested)       */
/*     given the ID (which is the MS_SDE_ROW_ID_COLUMN column           */
/* -------------------------------------------------------------------- */
int msSDELayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
#ifdef USE_SDE
  long status;

  SE_SQL_CONSTRUCT* sql;

  msSDELayerInfo *sde=NULL;

  long shapeindex = record->shapeindex;

  if(!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "msSDELayerGetShape()");
    return(MS_FAILURE);
  }

  sde = layer->layerinfo;

  /* must be at least one thing to retrieve (i.e. spatial column) */
  if(layer->numitems < 1) {
    msSetError( MS_MISCERR,
                "No items requested, SDE requires at least one item.",
                "msSDELayerGetShape()");
    return(MS_FAILURE);
  }

  /* reset the stream */
  status = SE_stream_close(sde->connPoolInfo->stream, 1);
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "msSDELayerGetShape()",
                "SE_stream_close()");
    return(MS_FAILURE);
  }

  if (!sde->join_table) {

    status = SE_stream_fetch_row(   sde->connPoolInfo->stream,
                                    sde->table,
                                    shapeindex,
                                    (short)(layer->numitems),
                                    (const char **)layer->items);

    if(status != SE_SUCCESS) {
      sde_error(  status,
                  "msSDELayerGetShape()",
                  "SE_stream_fetch_row()");
      return(MS_FAILURE);
    }
  } else {
    sql = getSDESQLConstructInfo(layer, &shapeindex);

    status = SE_stream_query(sde->connPoolInfo->stream, layer->numitems, (const CHAR**) layer->items, sql);
    ;
    if(status != SE_SUCCESS) {
      sde_error(status,
                "msSDELayerWhichShapes()",
                "SE_stream_query()");
      return(MS_FAILURE);
    }

    /* Free up the sql now that we've queried */
    SE_sql_construct_free(sql);

    /* *should* be ready to step through shapes now */
    status = SE_stream_execute(sde->connPoolInfo->stream);

    if(status != SE_SUCCESS) {
      sde_error(status,
                "SE_stream_execute()",
                "SE_stream_execute()");
      return(MS_FAILURE);
    }

    /* fetch the next record from the stream */
    status = SE_stream_fetch(sde->connPoolInfo->stream);

  }
  status = sdeGetRecord(layer, shape);
  if(status != MS_SUCCESS)
    return(MS_FAILURE); /* something went wrong fetching the record/shape */

  return(MS_SUCCESS);
#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerGetShape()");
  return(MS_FAILURE);
#endif
}

/* -------------------------------------------------------------------- */
/* msSDELayerGetSpatialColumn                                           */
/* -------------------------------------------------------------------- */
/*     A helper function to return the spatial column for               */
/*     an opened SDE layer                                              */
/*                                                                      */
/*     The caller owns the string after it is returned.                 */
/* -------------------------------------------------------------------- */
char *msSDELayerGetSpatialColumn(layerObj *layer)
{
#ifdef USE_SDE

  msSDELayerInfo *sde=NULL;

  if(!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "msSDELayerGetSpatialColumn()");
    return NULL;
  }

  sde = layer->layerinfo;

  return(msStrdup(sde->column));
#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerGetSpatialColumn()");
  return(NULL);
#endif
}
/* -------------------------------------------------------------------- */
/* msSDELayerInitItemInfo                                               */
/* -------------------------------------------------------------------- */
/* Connects to SDE and returns the column names and SDE's column        */
/* descriptions.  It stores them on layer->items and layer->iteminfo    */
/* If a join table has been specified through the processing option,    */
/* it's column definitions are appended to both lists.                  */
/* -------------------------------------------------------------------- */
int
msSDELayerInitItemInfo(layerObj *layer)
{
#ifdef USE_SDE

  int i,j;
  /*    short nBaseColumns, nJoinColumns; */
  long status;
  short nbasecol, njoincol;
  SE_COLUMN_DEF *all_itemdefs = NULL;

  msSDELayerInfo *sde = NULL;
  /*    nBaseColumns = 0; */
  /*    nJoinColumns = 0; */

  char *proc_key = NULL;

  if (!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR,
                "SDE layer has not been opened.",
                "msSDELayerInitItemInfo()");
    return(MS_FAILURE);
  }

  sde = layer->layerinfo;

  /*
  ** This insanity is because we keep around the number of
  ** columns we have along with the layer info.  If the total
  ** number of columns that we have doesn't match when we're
  ** called the second time around, we have to throw an error or
  ** msWhichShape will add an item onto our layer->items list, which
  ** in turn doesn't match the layer->iteminfo list of SDE column definitions.
  */

  nbasecol =*(sde->nBaseColumns);
  njoincol =*(sde->nJoinColumns);

  /* Hop right out again if we've already gotten the layer->iteminfo */
  if (layer->iteminfo && layer->items) {
    if (layer->debug)
      msDebug("Column information has already been gotten..."
              " returning from msSDELayerInitItemInfo\n");
    if (layer->numitems != ( nbasecol+ njoincol)) {
      /* if someone has modified the size of the items list,
      ** it is because it didn't find a column name (and we have
      ** already given them all because we have iteminfo and items
      ** If this is the case, we can't continue. */

      msSetError( MS_SDEERR,
                  "A specified CLASSITEM, FILTERITEM, or expression key cannot be found",
                  "msSDELayerInitItemInfo()");
      return(MS_FAILURE);
    }
    return (MS_SUCCESS);
  }
  if (layer->debug)
    msDebug("Getting all column information in msSDELayerInitItemInfo\n");

  if (!(sde->row_id_column)) {

    sde->row_id_column = msSDELayerGetRowIDColumn(layer);
  } else {
    /* Don't think this should happen.  If it does, it'd be good to know why. */
    if (layer->debug)
      msDebug ("RowID column has already been gotten... msSDELayerInitItemInfo\n");
  }

  status = SE_table_describe( sde->connPoolInfo->connection,
                              sde->table,
                              (short*)&(nbasecol),
                              &(sde->basedefs));
  if(status != SE_SUCCESS) {
    sde_error(  status,
                "msSDELayerInitItemInfo()",
                "SE_table_describe() (base table)");
    return(MS_FAILURE);
  }

  if (sde->join_table) {
    status = SE_table_describe( sde->connPoolInfo->connection,
                                sde->join_table,
                                (short*)&(njoincol),
                                &(sde->joindefs));

    if(status != SE_SUCCESS) {
      sde_error(  status,
                  "msSDELayerInitItemInfo()",
                  "SE_table_describe() (join table).  Did you specify the name of the join table properly?");
      return(MS_FAILURE);
    }
  }

  layer->numitems = nbasecol + njoincol;

  /* combine the itemdefs of both tables into one */
  all_itemdefs = (SE_COLUMN_DEF *) calloc( layer->numitems, sizeof(SE_COLUMN_DEF));
  MS_CHECK_ALLOC(all_itemdefs, sizeof(SE_COLUMN_DEF), MS_FAILURE);

  for(i=0; i<nbasecol; i++) all_itemdefs[i] = sde->basedefs[i];

  if (njoincol > 0) {
    for(i=0; i<njoincol; i++) {
      all_itemdefs[i+nbasecol]=sde->joindefs[i];
    }
  }

  if (!layer->iteminfo) {
    layer->iteminfo = (SE_COLUMN_DEF *) calloc( layer->numitems, sizeof(SE_COLUMN_DEF));
    if(!layer->iteminfo) {
      msSetError( MS_MEMERR,
                  "Error allocating SDE item  information.",
                  "msSDELayerInitItemInfo()");
      free(all_itemdefs);
      return(MS_FAILURE);
    }
  } else {
    /* Don't think this should happen.  If it does, it'd be good to know why. */
    if (layer->debug)
      msDebug ("layer->iteminfo has already been initialized... msSDELayerInitItemInfo\n");
  }

  if (!(layer->items)) {
    /* gather up all of the column names and put them onto layer->items */
    layer->items = (char **)malloc(layer->numitems*sizeof(char *)+10);
    MS_CHECK_ALLOC(layer->items, layer->numitems*sizeof(char *)+10, MS_FAILURE);
  } else {
    msDebug("layer->items has already been initialized!!!");
  }

  proc_key = msLayerGetProcessingKey(layer,"ATTRIBUTE_QUALIFIED");
  if (!sde->join_table &&
      (proc_key == NULL ||  strcasecmp( proc_key, "TRUE") != 0)) {
    for(i=0; i<layer->numitems; i++) layer->items[i] = msStrdup(all_itemdefs[i].column_name);
    for(i=0; i<layer->numitems; i++) { /* requested columns */
      for(j=0; j<layer->numitems; j++) { /* all columns */
        if(strcasecmp(layer->items[i], all_itemdefs[j].column_name) == 0) {
          /* found it */
          ((SE_COLUMN_DEF *)(layer->iteminfo))[i] = all_itemdefs[j];
          break;
        }
      }
    }
  } else {
    for(i=0; i<nbasecol; i++) {
      layer->items[i] = (char*) msSmallMalloc((SE_QUALIFIED_COLUMN_LEN+1)*sizeof (char));
      layer->items[i][0] = '\0';
      strcat(layer->items[i], sde->table);
      strcat(layer->items[i], ".");
      strcat(layer->items[i], all_itemdefs[i].column_name);
      ((SE_COLUMN_DEF *)(layer->iteminfo))[i] = all_itemdefs[i];

    }
    for(i=nbasecol; i<layer->numitems; i++) {
      layer->items[i] = (char*) msSmallMalloc((SE_QUALIFIED_COLUMN_LEN+1)*sizeof (char));
      layer->items[i][0] = '\0';

      strcat(layer->items[i], sde->join_table);
      strcat(layer->items[i], ".");
      strcat(layer->items[i], all_itemdefs[i].column_name);
      ((SE_COLUMN_DEF *)(layer->iteminfo))[i] = all_itemdefs[i];

    }
  }
  /* Tell the user which columns we've gotten */
  if (layer->debug)
    for(i=0; i<layer->numitems; i++)
      msDebug("msSDELayerInitItemInfo(): getting info for %s\n", layer->items[i]);

  *(sde->nJoinColumns) = njoincol;
  *(sde->nBaseColumns) = nbasecol;
  msFree(all_itemdefs);
  return MS_SUCCESS;

#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerInitItemInfo()");
  return(MS_FAILURE);
#endif
}



/* -------------------------------------------------------------------- */
/* msSDELayerCreateItems                                                */
/* -------------------------------------------------------------------- */
/*     Inits the stuff we'll be querying from SDE                       */
/* -------------------------------------------------------------------- */
int msSDELayerCreateItems(layerObj *layer, int nt)
{
  int status;
  /* status = msSDELayerCreateItems(layer, 0);    */
  status = msSDELayerInitItemInfo(layer);
  if (status != MS_SUCCESS) {
    msSetError( MS_MISCERR,
                "Unable to create SDE column info",
                "msSDELayerCreateItemsInfo()");
    return(MS_FAILURE);
  }
  return (MS_SUCCESS);
}

/* -------------------------------------------------------------------- */
/* msSDELayerGetItems                                                   */
/* -------------------------------------------------------------------- */
/*     Queries the SDE table's column names into layer->iteminfo        */
/* -------------------------------------------------------------------- */
int msSDELayerGetItems(layerObj *layer)
{
#ifdef USE_SDE
  int status;
  status = msSDELayerInitItemInfo(layer);

  if (status != MS_SUCCESS) {
    msSetError( MS_MISCERR,
                "Unable to create SDE column info",
                "msSDELayerGetItems()");
    return(MS_FAILURE);
  }
  return (MS_SUCCESS);

#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerGetItems()");
  return(MS_FAILURE);
#endif
}

/* -------------------------------------------------------------------- */
/* msSDELayerFreeItemInfo                                               */
/* -------------------------------------------------------------------- */
void msSDELayerFreeItemInfo(layerObj *layer)
{
#ifdef USE_SDE
  msSDELayerInfo *sde = NULL;
  int i;
  if (!msSDELayerIsOpen(layer)) {
    msSetError( MS_SDEERR, "SDE layer has not been opened.", "msSDELayerFreeItemInfo()");
  }
  sde = layer->layerinfo;
  if (sde != NULL && sde->basedefs) {
    SE_table_free_descriptions(sde->basedefs);
    sde->basedefs = NULL;
  }
  if (sde != NULL && sde->joindefs) {
    SE_table_free_descriptions(sde->joindefs);
    sde->joindefs = NULL;
  }
  if (layer->iteminfo) {
    msFree(layer->iteminfo);
    layer->iteminfo = NULL;
  }

  if (layer->items) {
    for (i=0; i< layer->numitems; i++) {
      msFree(layer->items[i]);
    }
    msFree(layer->items);
    layer->items = NULL;
    layer->numitems = 0;
  }

#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerFreeItemInfo()");
#endif
}

#ifdef USE_SDE_PLUGIN
MS_DLL_EXPORT  int
PluginInitializeVirtualTable(layerVTableObj *vtable, layerObj *layer)
{
  assert(layer != NULL);
  assert(vtable != NULL);

  /* vtable->LayerTranslateFilter, use default */

  vtable->LayerInitItemInfo = msSDELayerInitItemInfo;
  vtable->LayerFreeItemInfo = msSDELayerFreeItemInfo;
  vtable->LayerOpen = msSDELayerOpen;
  vtable->LayerIsOpen = msSDELayerIsOpen;
  vtable->LayerWhichShapes = msSDELayerWhichShapes;
  vtable->LayerNextShape = msSDELayerNextShape;
  vtable->LayerGetShape = msSDELayerGetShape;
  vtable->LayerClose = msSDELayerClose;
  vtable->LayerGetItems = msSDELayerGetItems;
  vtable->LayerGetExtent = msSDELayerGetExtent;

  /* vtable->LayerGetAutoStyle, use default */
  /* vtable->LayerApplyFilterToLayer, use default */

  /* SDE uses pooled connections, close from msCloseConnections */
  vtable->LayerCloseConnection = msSDELayerCloseConnection;

  vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;
  vtable->LayerCreateItems = msSDELayerCreateItems;
  /* vtable->LayerGetNumFeatures, use default */

  return MS_SUCCESS;
}
#endif

int
msSDELayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerInitItemInfo = msSDELayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msSDELayerFreeItemInfo;
  layer->vtable->LayerOpen = msSDELayerOpen;
  layer->vtable->LayerIsOpen = msSDELayerIsOpen;
  layer->vtable->LayerWhichShapes = msSDELayerWhichShapes;
  layer->vtable->LayerNextShape = msSDELayerNextShape;
  layer->vtable->LayerGetShape = msSDELayerGetShape;
  layer->vtable->LayerClose = msSDELayerClose;
  layer->vtable->LayerGetItems = msSDELayerGetItems;
  layer->vtable->LayerGetExtent = msSDELayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  layer->vtable->LayerCloseConnection = msSDELayerCloseConnection; /* SDE uses pooled connections, close from msCloseConnections */
  layer->vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;
  layer->vtable->LayerCreateItems = msSDELayerCreateItems;
  /* layer->vtable->LayerGetNumFeatures, use default */

  return MS_SUCCESS;
}




/******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.109.2.2  2007/02/27 19:52:20  hobu
 * do not double-grab the row_id column in msSDELayerGetItems (bug 2041)
 *
 * Revision 1.109.2.1  2006/10/31 17:08:52  hobu
 * make sure to initialize sde->row_id_column to NULL or we'll blow
 * up on close for operations like GetCapabilities
 *
 * Revision 1.109  2006/08/11 16:58:02  dan
 * Added ability to encrypt tokens (passwords, etc.) in database connection
 * strings (MS-RFC-18, bug 1792)
 *
 * Revision 1.108  2006/08/01 18:23:12  sdlime
 * Fixed a problem in the SDE code that prohibited setting SDE to only process the attribute portion of a query.
 *
 * Revision 1.107  2006/03/12 05:54:35  hobu
 * make sure to return a value in the failure case
 * for msSDELayerClose
 *
 * Revision 1.106  2006/03/10 17:11:08  hobu
 * add a PROCESSING directive to SDE to allow the user to specify
 * the query order (attributes or spatial first).
 *
 * Revision 1.105  2006/03/10 17:05:56  hobu
 * Fix 1699, where point queries in msSDEWhichShape were
 * causing the SDE C API to complain about creating an invalid
 * rectangle
 *
 * Revision 1.104  2006/02/24 22:41:50  sdlime
 * Added missing malloc in mapsde in msSDELayerGetItems().
 *
 * Revision 1.103  2006/02/24 20:24:23  hobu
 * make sure we check if layer->iteminfo exists before hammering it and reallocating
 * just use LayerClose to close layer information rather than the
 * LayerClose/LayerConnectionClose dichotomy that wasn't working
 *
 * Revision 1.102  2006/01/10 19:27:20  hobu
 * fix the memory allocation bug described in bug 1606
 *
 * Revision 1.101  2006/01/10 00:19:49  hobu
 * Fix up allocation of the ROW_ID columns and how the functions that
 * call it were using it. (bug 1605)
 *
 * Revision 1.100  2005/12/15 15:36:59  frank
 * avoid comment-in-comment warnings
 *
 * Revision 1.99  2005/12/08 19:44:46  hobu
 * oops.  Move msSDELayerGetRowIDColumn to the right spot for
 * non-SDE builds
 *
 * Revision 1.98  2005/12/08 19:07:36  hobu
 * switch off SDE raster support and fix up msSDELayerGetRowIDColumn
 *
 * Revision 1.97  2005/10/28 01:09:42  jani
 * MS RFC 3: Layer vtable architecture (bug 1477)
 *
 * Revision 1.96  2005/07/07 15:03:33  hobu
 * put thread locking around static data members
 * used in the lcache (msSDELCacheAdd)
 *
 * Revision 1.95  2005/06/14 16:03:34  dan
 * Updated copyright date to 2005
 *
 * Revision 1.94  2005/02/18 03:06:47  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.93  2004/11/18 23:06:10  hobu
 * allow bands and raster column for
 * SDE rasters to be specified with
 * PROCESSING directives
 *
 * Revision 1.92  2004/11/17 17:16:16  hobu
 * define USE_SDERASTER to turn on experimental SDE
 * raster support
 *
 * Revision 1.91  2004/11/15 20:35:02  dan
 * Added msLayerIsOpen() to all vector layer types (bug 1051)
 *
 * Revision 1.90  2004/11/15 18:42:49  hobu
 * do not silently return a MS_SUCCESS in msSDELayerOpen.
 * Instead, depend on the connection/layer pooling stuff to cache
 * the expensive calls.
 *
 * I also included the drawSDE() code that is the bulk of
 * SDE Raster support by Pirmin Kalberer of sourcepole.com.  It
 * isn't hooked up the rest of the MapServer raster machinery
 * yet.
 *
 * Revision 1.89  2004/11/03 20:31:38  hobu
 * first cut at implementing layer and stream caching
 * to go along with pool caching to improve the
 * response time for opening/closing layers
 *
 * Revision 1.88  2004/10/26 16:28:10  hobu
 * make sure to initialize sde->row_id_column
 * in msSDELayerOpen
 *
 * Revision 1.87  2004/10/26 16:06:04  hobu
 * check for the layer->layerinfo member
 * in msSDELayerClose before freeing
 *
 * Revision 1.86  2004/10/21 14:47:07  hobu
 * MSVC doesn't like it when you start
 * doing operations before you initialize
 * something.  Fixup the declaration of
 * msSDELayerInfo in sdeGetRecord
 *
 * Revision 1.85  2004/10/21 04:30:57  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 * Revision 1.84  2004/10/19 19:06:48  hobu
 * First cut of BLOB support.  Blobs
 * can't actually query yet, but NULL
 * blob columns in a table will not prevent
 * the user from querying the table.
 *
 * Revision 1.83  2004/10/19 16:51:00  hobu
 * if no version is specified, do not set
 * a default or do any versioned queries.
 *
 * Revision 1.82  2004/10/15 18:13:02  hobu
 * sde->state_id was being used before it was initialized
 * sde->connection was being set to null *after* the sde
 * structure was already freed in msSDELayerClose causing heap
 * corruption.  (Thanks Frank!)
 *
 * Revision 1.81  2004/10/14 15:20:08  hobu
 * make sure that we call SE_stream_free
 * when the layer is closed
 *
 * Revision 1.80  2004/10/11 14:34:26  hobu
 * change thread locking policy
 * to SE_UNPROTECTED_POLICY
 *
 * Revision 1.79  2004/10/04 22:13:49  hobu
 * touch up comments that weren't properly
 * closed that were causing gcc to complain
 *
 * Revision 1.78  2004/10/04 20:15:00  hobu
 * touch up commit comment
 *
 * Revision 1.77  2004/10/04 20:11:00  hobu
 * Rework of the style to fit code in under 80 columns and add visual breaks 
 * between the start and end of functions.  Normalized tabs to spaces wherever 
 * I found them, but it is still possible I missed a few
 *
*/

#include <time.h>
#include <assert.h>

#include "map.h"
#include "maperror.h"
#include "maptime.h"
#include "mapthread.h"



#ifdef USE_SDE
#include <sdetype.h> /* ESRI SDE Client Includes */
#include <sdeerno.h>

/*
#ifdef USE_SDERASTER
#include <sderaster.h>
#endif
*/

#define MS_SDE_MAXBLOBSIZE 1024*50 /* 50 kbytes */
#define MS_SDE_NULLSTRING "<null>"
#define MS_SDE_SHAPESTRING "<shape>"
#define MS_SDE_TIMEFMTSIZE 128 /* bytes */
#define MS_SDE_TIMEFMT "%T %m/%d/%Y"
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
  long state_id;
  char *table, *column, *row_id_column;
} msSDELayerInfo;

typedef struct {
  long layerId;
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
     free(poolinfo);
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
/*     an opened SDE layer                                              */
/* -------------------------------------------------------------------- */
char *msSDELayerGetRowIDColumn(layerObj *layer)
{
#ifdef USE_SDE
  long status, column_type; 
  char* column_name;
  SE_REGINFO registration;

  msSDELayerInfo *sde=NULL;
  sde = layer->layerinfo;


  column_name = (char*) malloc(SE_MAX_COLUMN_LEN);
  if(!sde) {
    msSetError( MS_SDEERR, 
                "SDE layer has not been opened.", 
                "msSDELayerGetSpatialColumn()");
    return(NULL);
  }   
  
  if (sde->state_id == SE_DEFAULT_STATE_ID) {
    if(layer->debug) 
      msDebug("msSDELayerGetRowIDColumn(): State ID was SE_DEFAULT_STATE_ID, "
              "reverting to %s.\n", 
              MS_SDE_ROW_ID_COLUMN);
      return(strdup(MS_SDE_ROW_ID_COLUMN));
  }
  else 
  {
    status = SE_reginfo_create (&registration);
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerGetRowIDColumn()", "SE_reginfo_create()");
      return(NULL);
    }
    
    status = SE_registration_get_info ( sde->connection, 
                                        sde->table, 
                                        registration);
    if(status != SE_SUCCESS) {
      sde_error(status, 
                "msSDELayerGetRowIDColumn()", 
                "SE_registration_get_info()");
      return(NULL);
    }
    
    status= SE_reginfo_get_rowid_column ( registration, 
                                          column_name, 
                                          &column_type);
    SE_reginfo_free(registration);
    if(status != SE_SUCCESS) {
      sde_error(status, 
                "msSDELayerGetRowIDColumn()", 
                "SE_reginfo_get_rowid_column()");
      return(NULL);
    }
    if (column_type == SE_REGISTRATION_ROW_ID_COLUMN_TYPE_NONE){
      if(layer->debug) {
        msDebug("msSDELayerGetRowIDColumn(): Table was not registered, "
        "returning %s.\n", 
        MS_SDE_ROW_ID_COLUMN);
      }
      return (MS_SDE_ROW_ID_COLUMN);
    }
    
    if (column_name){

      return (column_name); 
    }
    else {
      free(column_name);
      return(strdup(MS_SDE_ROW_ID_COLUMN));
    }
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
  int status = 0;
  
  msAcquireLock( TLOCK_SDE );
  
  if (layer->debug){
    msDebug( "%s: Caching id for %s, %s, %s\n", "msSDELCacheAdd()", 
            tableName, columnName, connectionString);
  }
  /*
   * Ensure the cache is large enough to hold the new item.
   */
  if(lcacheCount == lcacheMax)
  {
    lcacheMax += 10;
    lcache = (layerId *)realloc(lcache, sizeof(layerId) * lcacheMax);
    if(lcache == NULL)
    {
      msReleaseLock( TLOCK_SDE );
      msSetError(MS_MEMERR, NULL, "msSDELCacheAdd()");
      return (MS_FAILURE);
    }
  }

  /*
   * Population the new lcache object.
   */
  lid = lcache + lcacheCount;
  lcacheCount++;

  status = SE_layerinfo_get_id(layerinfo, &lid->layerId);
  if(status != SE_SUCCESS)
  {
        msReleaseLock( TLOCK_SDE );
        sde_error(status, "msSDELCacheAdd()", "SE_layerinfo_get_id()");
        return(MS_FAILURE);
  }
  lid->table = strdup(tableName);
  lid->column = strdup(columnName);
  lid->connection = strdup(connectionString);
  
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
  
  /*
   * If table or column are null, nothing can be done.
   */
  if(tableName == NULL)
  {
    msSetError( MS_MISCERR,
                "Missing table name.\n",
                "msSDEGetLayerInfo()");
    return (MS_FAILURE);
  }
  if(columnName == NULL)
  {
    msSetError( MS_MISCERR,
                "Missing column name.\n",
                "msSDEGetLayerInfo()");
    return (MS_FAILURE);
  }
  if(connectionString == NULL)
  {
    msSetError( MS_MISCERR,
                "Missing connection string.\n",
                "msSDEGetLayerInfo()");
    return (MS_FAILURE);
  }  

  if (layer->debug){
    msDebug("%s: Looking for layer by %s, %s, %s\n", "msSDEGetLayerInfo()",
          tableName, columnName, connectionString);
  }
  /*
   * Search the lcache for the layer id.
   */
  for(i = 0; i < lcacheCount; i++)
  {
    lid = lcache + i;
    if(strcasecmp(lid->table, tableName) == 0 &&
        strcasecmp(lid->column, columnName) == 0 &&
        strcasecmp(lid->connection, connectionString) == 0)
    {
      status = SE_layer_get_info_by_id(conn, lid->layerId, layerinfo);
      if(status != SE_SUCCESS) {
        sde_error(status, "msSDEGetLayerInfo()", "SE_layer_get_info()");
        return(MS_FAILURE);
      }
      else
      {
        if (layer->debug){
          msDebug( "%s: Matched layer to id %i.\n", 
                   "msSDEGetLayerId()", lid->layerId);
        }
        return (MS_SUCCESS);
      }
    }
  }
  if (layer->debug){
    msDebug("%s: No cached layerid found.\n", "msSDEGetLayerInfo()");
  }
  /*
   * No matches found, create one.
   */
  status = SE_layer_get_info( conn, tableName, columnName, layerinfo );
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDEGetLayerInfo()", "SE_layer_get_info()");
    return(MS_FAILURE);
  }
  else 
  {
    status = msSDELCacheAdd(layer, layerinfo, tableName, columnName, connectionString);
    return(MS_SUCCESS);
  }
}

/* -------------------------------------------------------------------- */
/* sdeShapeCopy                                                         */
/* -------------------------------------------------------------------- */
/*     Copies a SDE shape into a MapServer shapeObj                     */
/* -------------------------------------------------------------------- */
static int sdeShapeCopy(SE_SHAPE inshp, shapeObj *outshp) {

  SE_POINT *points=NULL;
  SE_ENVELOPE envelope;
  long type, status;
  long *part_offsets = NULL;
  long *subpart_offsets = NULL;
  long num_parts = -1;
  long num_subparts = -1;
  long num_points = -1;
  
  lineObj line={0,NULL};

  int i,j,k;

  status = SE_shape_get_type(inshp, &type);
  if(status != SE_SUCCESS) {
    sde_error(status, 
              "sdeCopyShape()", 
              "SE_shape_get_type()");
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
    msSetError( MS_SDEERR, 
                "Unsupported SDE shape type (%ld).", 
                "sdeCopyShape()", 
                type);
    return(MS_FAILURE);
  }


  SE_shape_get_num_parts (inshp, &num_parts, &num_subparts);
  SE_shape_get_num_points (inshp, 0, 0, &num_points); 
	 
  part_offsets = (long *) malloc( (num_parts + 1) * sizeof(long));
  subpart_offsets = (long *) malloc( (num_subparts + 1)	* sizeof(long));
  part_offsets[num_parts] = num_subparts;
  subpart_offsets[num_subparts]	= num_points;

  points = (SE_POINT *)malloc(num_points*sizeof(SE_POINT));
  if(!points) {
    msSetError( MS_MEMERR, 
                "Unable to allocate points array.", 
                "sdeCopyShape()");
    return(MS_FAILURE);
  }

  status = SE_shape_get_all_points( inshp, 
                                    SE_DEFAULT_ROTATION, 
                                    part_offsets, 
                                    subpart_offsets, 
                                    points, 
                                    NULL, 
                                    NULL);
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
    if(!line.point) {
      msSetError( MS_MEMERR, 
                  "Unable to allocate temporary point cache.", 
                  "sdeShapeCopy()");
      return(MS_FAILURE);
    }
     
    for(j=0; j < line.numpoints; j++) {
      line.point[j].x = points[k].x; 
      line.point[j].y = points[k].y;     
      k++;
    }

    msAddLine(outshp, &line);
    free(line.point);
  }

  free(part_offsets);
  free(subpart_offsets);
  free(points);

  /* finally copy the bounding box for the entire shape */
  SE_shape_get_extent(inshp, 0, &envelope);
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
static int sdeGetRecord(layerObj *layer, shapeObj *shape) {
  int i;
  long status;

  double doubleval;
  long longval;
  struct tm dateval;

  short shortval; /* new gdv */
  float floatval;

  SE_COLUMN_DEF *itemdefs;
  SE_SHAPE shapeval=0;
  SE_BLOB_INFO blobval;
 /* blobval = (SE_BLOB_INFO *) malloc(sizeof(SE_BLOB_INFO)); */
  msSDELayerInfo *sde;

  sde = layer->layerinfo;

  if(layer->numitems > 0) {
    shape->numvalues = layer->numitems;
    shape->values = (char **)malloc(sizeof(char *)*layer->numitems);
    if(!shape->values) {
      msSetError( MS_MEMERR, 
                  "Error allocation shape attribute array.", 
                  "sdeGetRecord()");
      return(MS_FAILURE);
    }
  }

  status = SE_shape_create(NULL, &shapeval);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeGetRecord()", "SE_shape_create()");
    return(MS_FAILURE);
  }

  itemdefs = layer->iteminfo;
  for(i=0; i<layer->numitems; i++) {

    if(strcmp(layer->items[i],sde->row_id_column) == 0) {
      status = SE_stream_get_integer(sde->stream, (short)(i+1), &shape->index);
      if(status != SE_SUCCESS) {
         sde_error(status, "sdeGetRecord()", "SE_stream_get_integer()");
         return(MS_FAILURE);
      }

      shape->values[i] = (char *)malloc(64); /* should be enough */
      sprintf(shape->values[i], "%ld", shape->index);
      continue;
    }    
    
    switch(itemdefs[i].sde_type) {
    case SE_SMALLINT_TYPE:
      /* changed by gdv */
      status = SE_stream_get_smallint(sde->stream, (short)(i+1), &shortval); 
      if(status == SE_SUCCESS)
        shape->values[i] = long2string(shortval);
      else if(status == SE_NULL_VALUE)
        shape->values[i] = strdup(MS_SDE_NULLSTRING);
      else {
        sde_error(status, "sdeGetRecord()", "SE_stream_get_smallint()");
        return(MS_FAILURE);
      }
      break;
    case SE_INTEGER_TYPE:
      status = SE_stream_get_integer(sde->stream, (short)(i+1), &longval);
      if(status == SE_SUCCESS)
        shape->values[i] = long2string(longval);
      else if(status == SE_NULL_VALUE)
        shape->values[i] = strdup(MS_SDE_NULLSTRING);
      else {
        sde_error(status, "sdeGetRecord()", "SE_stream_get_integer()");
        return(MS_FAILURE);
      }      
      break;
    case SE_FLOAT_TYPE:
      /* changed by gdv */
      status = SE_stream_get_float(sde->stream, (short)(i+1), &floatval); 
      if(status == SE_SUCCESS)
        shape->values[i] = double2string(floatval);
      else if(status == SE_NULL_VALUE)
        shape->values[i] = strdup(MS_SDE_NULLSTRING);
      else {     
        sde_error(status, "sdeGetRecord()", "SE_stream_get_float()");
        return(MS_FAILURE);
      }
      break;
    case SE_DOUBLE_TYPE:
      status = SE_stream_get_double(sde->stream, (short) (i+1), &doubleval);
      if(status == SE_SUCCESS)
        shape->values[i] = double2string(doubleval);
      else if(status == SE_NULL_VALUE)
        shape->values[i] = strdup(MS_SDE_NULLSTRING);
      else {     
        sde_error(status, "sdeGetRecord()", "SE_stream_get_double()");
        return(MS_FAILURE);
      }
      break;
    case SE_STRING_TYPE:
      shape->values[i] = (char *)malloc(itemdefs[i].size+1);
      status = SE_stream_get_string(sde->stream, 
                                    (short) (i+1), 
                                    shape->values[i]);
      if(status == SE_NULL_VALUE)
        shape->values[i][0] = '\0'; /* empty string */
      else if(status != SE_SUCCESS) {
        sde_error(status, "sdeGetRecord()", "SE_stream_get_string()");
        return(MS_FAILURE);
      }
      break;
    case SE_BLOB_TYPE:
        status = SE_stream_get_blob(sde->stream, (short) (i+1), &blobval);
        if(status == SE_SUCCESS) {
          shape->values[i] = (char *)malloc(sizeof(char)*blobval.blob_length);
          shape->values[i] = memcpy(shape->values[i],
                                    blobval.blob_buffer, 
                                    blobval.blob_length);
          SE_blob_free(&blobval);
        }
        else if (status == SE_NULL_VALUE) {
          shape->values[i] = strdup(MS_SDE_NULLSTRING);
        }
        else {
          sde_error(status, "sdeGetRecord()", "SE_stream_get_blob()");
          return(MS_FAILURE);
        }
      break;
    case SE_DATE_TYPE:
      status = SE_stream_get_date(sde->stream, (short)(i+1), &dateval);
      if(status == SE_SUCCESS) {
        shape->values[i] = (char *)malloc(sizeof(char)*MS_SDE_TIMEFMTSIZE);
        strftime( shape->values[i], 
                  MS_SDE_TIMEFMTSIZE, 
                  MS_SDE_TIMEFMT, 
                  &dateval);
      } else if(status == SE_NULL_VALUE)
        shape->values[i] = strdup(MS_SDE_NULLSTRING);
      else {     
        sde_error(status, "sdeGetRecord()", "SE_stream_get_date()");
        return(MS_FAILURE);
      }
      break;
    case SE_SHAPE_TYPE:
      status = SE_stream_get_shape(sde->stream, (short)(i+1), shapeval);
      if(status == SE_SUCCESS)
        shape->values[i] = strdup(MS_SDE_SHAPESTRING);
      else if(status == SE_NULL_VALUE)
        shape->values[i] = strdup(MS_SDE_NULLSTRING);
      else {
        sde_error(status, "sdeGetRecord()", "SE_stream_get_shape()");
        return(MS_FAILURE);
      }
      break;
    default: 
      msSetError(MS_SDEERR, "Unknown SDE column type.", "sdeGetRecord()");
      return(MS_FAILURE);
      break;
    }
  }

  if(SE_shape_is_nil(shapeval)) return(MS_SUCCESS);
  
  /* copy sde shape to a mapserver shape */
  status = sdeShapeCopy(shapeval, shape);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  /* clean up */
  SE_shape_free(shapeval);

  
  return(MS_SUCCESS);
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
int msSDELayerOpen(layerObj *layer) {
#ifdef USE_SDE
  long status=-1;
  char **params=NULL;
  char **data_params=NULL;
  int numparams=0;
  SE_ERROR error;
  SE_STATEINFO state;
  SE_VERSIONINFO version;

  msSDELayerInfo *sde;
  msSDEConnPoolInfo *poolinfo;


  /* layer already open, silently return */
  /* if(layer->layerinfo) return(MS_SUCCESS);  */

  /* allocate space for SDE structures */
  sde = (msSDELayerInfo *) malloc(sizeof(msSDELayerInfo));
  if(!sde) {
    msSetError( MS_MEMERR, 
                "Error allocating SDE layer structure.", 
                "msSDELayerOpen()");
    return(MS_FAILURE);
  }
 
  sde->state_id = SE_BASE_STATE_ID;
  
  /* initialize the table and spatial column names */
  sde->table = NULL;
  sde->column = NULL;
  sde->row_id_column = NULL;

  /* request a connection and stream from the pool */
  poolinfo = (msSDEConnPoolInfo *)msConnPoolRequest( layer ); 
  
  /* If we weren't returned a connection and stream, initialize new ones */
  if (!poolinfo) {
    char *conn_decrypted;

    if (layer->debug) 
      msDebug("msSDELayerOpen(): "
              "Layer %s opened from scratch.\n", layer->name);


    poolinfo = malloc(sizeof *poolinfo);
    if (!poolinfo) {
      return MS_FAILURE;
    } 		

    /* Decrypt any encrypted token in the connection string */
    conn_decrypted = msDecryptStringTokens(layer->map, layer->connection);
    if (conn_decrypted == NULL) {
        return(MS_FAILURE);  /* An error should already have been produced */
    }
    /* Split the connection parameters and make sure we have enough of them */
    params = split(conn_decrypted, ',', &numparams);
    if(!params) {
      msSetError( MS_MEMERR, 
                  "Error spliting SDE connection information.", 
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
                                  &error, 
                                  &(poolinfo->connection));

    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_connection_create()");
      return(MS_FAILURE);
    }

    /* ------------------------------------------------------------------------- */
    /* Set the concurrency type for the connection.  SE_UNPROTECTED_POLICY is    */
    /* suitable when only one thread accesses the specified connection.          */
    /* ------------------------------------------------------------------------- */
    status = SE_connection_set_concurrency( poolinfo->connection, 
                                            SE_UNPROTECTED_POLICY);

  
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_connection_set_concurrency()");
      return(MS_FAILURE);
    }
    

    status = SE_stream_create(poolinfo->connection, &(poolinfo->stream));
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_stream_create()");
    return(MS_FAILURE);
    }

    /* Register the connection with the connection pooling API.  Give  */
    /* msSDECloseConnection as the function to call when we run out of layer  */
    /* instances using it */
    msConnPoolRegister(layer, poolinfo, msSDECloseConnection);
    msFreeCharArray(params, numparams); /* done with parameter list */
  }

  /* Split the DATA member into its parameters using the comma */
  /* Periods (.) are used to denote table names and schemas in SDE,  */
  /* as are underscores (_). */
  data_params = split(layer->data, ',', &numparams);
  if(!data_params) {
    msSetError(MS_MEMERR, 
        "Error spliting SDE layer information.", "msSDELayerOpen()");
    return(MS_FAILURE);
  }

  if(numparams < 2) {
    msSetError(MS_SDEERR, 
    "Not enough SDE layer parameters specified.", "msSDELayerOpen()");
    return(MS_FAILURE);
  }

  sde->table = strdup(data_params[0]); 
  sde->column = strdup(data_params[1]);


  if (numparams < 3){ 
    /* User didn't specify a version, we won't use one */
    if (layer->debug) {
      msDebug("msSDELayerOpen(): Layer %s did not have a " 
              "specified version.\n", layer->name);
    } 
    sde->state_id = SE_DEFAULT_STATE_ID;
  } 
  else {
    if (layer->debug) {
      msDebug("msSDELayerOpen(): Layer %s specified version %s.\n", 
              layer->name, 
              data_params[2]);
    }
    status = SE_versioninfo_create (&(version));
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_versioninfo_create()");
      return(MS_FAILURE);
    }
    status = SE_version_get_info(poolinfo->connection, data_params[2], version);
    
    if(status != SE_SUCCESS) {
       
      if (status == SE_INVALID_RELEASE) {
        /* The user has incongruent versions of SDE, ie 8.2 client and  */
        /* 8.3 server set the state_id to SE_DEFAULT_STATE_ID, which means    */
        /* no version queries are done */
        sde->state_id = SE_DEFAULT_STATE_ID;
      }
      else {
        sde_error(status, "msSDELayerOpen()", "SE_version_get_info()");
        return(MS_FAILURE);
      }
    }
  
  }

  /* Get the STATEID from the given version and set the stream to  */
  /* that if we didn't already set it to SE_DEFAULT_STATE_ID.   */
  if (!(sde->state_id == SE_DEFAULT_STATE_ID)){
    status = SE_versioninfo_get_state_id(version, &sde->state_id);
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_versioninfo_get_state_id()");
      return(MS_FAILURE);
    }
    SE_versioninfo_free(version);
    status = SE_stateinfo_create (&state);
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_stateinfo_create()");
      return(MS_FAILURE);
    }    
    status = SE_state_get_info(poolinfo->connection, sde->state_id, state);
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_state_get_info()");
      return(MS_FAILURE);
    }  
    if (SE_stateinfo_is_open (state)) {
      /* If the state is open for edits, we shouldn't be querying from it */
      sde_error(status, 
                "msSDELayerOpen()", 
                "SE_stateinfo_is_open() -- State for version is open");
      return(MS_FAILURE);
    }
    SE_stateinfo_free (state); 

        msFreeCharArray(data_params, numparams);  
  } /* if (!(sde->state_id == SE_DEFAULT_STATE_ID)) */
  
  
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

  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_layer_get_info()");
    return(MS_FAILURE);
  }

  SE_coordref_create(&(sde->coordref));
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_coordref_create()");
    return(MS_FAILURE);
  }

  status = SE_layerinfo_get_coordref(sde->layerinfo, sde->coordref);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_layerinfo_get_coordref()");
    return(MS_FAILURE);
  }


  /* reset the stream */
  status = SE_stream_close(poolinfo->stream, 1);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_stream_close()");
    return(MS_FAILURE);
  }  


  /* point to the SDE layer information  */
  /* (note this might actually be in another layer) */
  layer->layerinfo = sde; 

  sde->connection = poolinfo->connection;
  sde->stream = poolinfo->stream;
  sde->connPoolInfo = poolinfo;


  return(MS_SUCCESS);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msSDELayerOpen()");
  return(MS_FAILURE);
#endif
}


/* -------------------------------------------------------------------- */
/* msSDELayerIsOpen                                                     */
/* -------------------------------------------------------------------- */
/*     Returns MS_TRUE if layer is already opened, MS_FALSE otherwise   */
/* -------------------------------------------------------------------- */
int msSDELayerIsOpen(layerObj *layer) {
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

/* -------------------------------------------------------------------- */
/* msSDELayerClose                                                      */
/* -------------------------------------------------------------------- */
/*     Closes the MapServer layer.  This doesn't necessarily close the  */
/*     connection to the layer.                                         */
/* -------------------------------------------------------------------- */
int  msSDELayerClose(layerObj *layer) {
#ifdef USE_SDE


  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;
  if (sde == NULL) return MS_SUCCESS;  /* Silently return if layer not opened. */

  if(layer->debug) 
    msDebug("msSDELayerClose(): Closing layer %s.\n", layer->name);
	
  if (sde->layerinfo) SE_layerinfo_free(sde->layerinfo);
  if (sde->coordref) SE_coordref_free(sde->coordref);
  if (sde->table) free(sde->table);
  if (sde->column) free(sde->column);
  if (sde->row_id_column) free(sde->row_id_column);

  msConnPoolRelease( layer, sde->connPoolInfo );  
  sde->connection = NULL;
  sde->connPoolInfo = NULL;
  if (layer->layerinfo) free(layer->layerinfo);
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
/*int msSDELayerCloseConnection(layerObj *layer) 
{
	

#ifdef USE_SDE


  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;
  if (sde == NULL) return;   Silently return if layer not opened./

  if(layer->debug)
    msDebug("msSDELayerCloseConnection(): Closing connection for layer %s.\n", layer->name);

  msConnPoolRelease( layer, sde->connPoolInfo );
  sde->connection = NULL;
  sde->connPoolInfo = NULL;

#else
  msSetError( MS_MISCERR,
              "SDE support is not available.",
              "msSDELayerClose()");
  return;
#endif

    return MS_SUCCESS;
}
*/

/* -------------------------------------------------------------------- */
/* msSDELayerWhichShapes                                                */
/* -------------------------------------------------------------------- */
/*     starts a stream query using spatial filter.  Also limits the     */
/*     query by the layer's FILTER item as well.                        */
/* -------------------------------------------------------------------- */
int msSDELayerWhichShapes(layerObj *layer, rectObj rect) {
#ifdef USE_SDE
  long status;
  SE_ENVELOPE envelope;
  SE_SHAPE shape=0;
  SE_FILTER constraint;
  SE_QUERYINFO query_info;
  char* proc_value=NULL;
  int query_order=SE_SPATIAL_FIRST;

  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;
  if(!sde) {
    msSetError( MS_SDEERR, 
                "SDE layer has not been opened.", 
                "msSDELayerWhichShapes()");
    return(MS_FAILURE);
  }

  status = SE_shape_create(sde->coordref, &shape);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_shape_create()");
    return(MS_FAILURE);
  }

  status = SE_layerinfo_get_envelope(sde->layerinfo, &envelope);
  if(status != SE_SUCCESS) {
    sde_error(status, 
              "msSDELayerWhichShapes()", 
              "SE_layerinfo_get_envelope()");
    return(MS_FAILURE);
  }
  
  /* there is NO overlap, return MS_DONE */
  /* (FIX: use this in ALL which shapes functions) */
  if(envelope.minx > rect.maxx) return(MS_DONE); 
  if(envelope.maxx < rect.minx) return(MS_DONE);
  if(envelope.miny > rect.maxy) return(MS_DONE);
  if(envelope.maxy < rect.miny) return(MS_DONE);

  /* set spatial constraint search shape */
  /* crop against SDE layer extent *argh* */
  envelope.minx = MS_MAX(rect.minx, envelope.minx); 
  envelope.miny = MS_MAX(rect.miny, envelope.miny);
  envelope.maxx = MS_MIN(rect.maxx, envelope.maxx);
  envelope.maxy = MS_MIN(rect.maxy, envelope.maxy);
  
  if( envelope.minx == envelope.maxx && envelope.miny == envelope.maxy){
        /* fudge a rectangle so we have a valid one for generate_rectangle */
        /* FIXME: use the real shape for the query and set the filter_type 
           to be an appropriate type */
        envelope.minx = envelope.minx - 0.001;
        envelope.maxx = envelope.maxx + 0.001;
        envelope.miny = envelope.miny - 0.001;
        envelope.maxy = envelope.maxy + 0.001;
    }

  status = SE_shape_generate_rectangle(&envelope, shape);
  if(status != SE_SUCCESS) {
    sde_error(status, 
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

  /* See http://forums.esri.com/Thread.asp?c=2&f=59&t=108929&mc=4#msgid310273 */
  /* SE_queryinfo is a new SDE struct in ArcSDE 8.x that is a bit easier  */
  /* (and faster) to use and will allow us to support joins in the future.  HCB */
  status = SE_queryinfo_create (&query_info);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_queryinfo_create()");
    return(MS_FAILURE);
  }

  /* set the tables -- just one at this point */
  status = SE_queryinfo_set_tables (query_info, 
                                    1, 
                                    (const CHAR **) &(sde->table),
                                    NULL);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_queryinfo_create()");
    return(MS_FAILURE);
  }

  /* set the "where" clause */
  if(!(layer->filter.string))
    /* set to empty string */
    status = SE_queryinfo_set_where_clause (query_info, 
                                            (const CHAR * ) "");
  else
    /* set to the layer's filter.string */
    status = SE_queryinfo_set_where_clause (query_info, 
                                 (const CHAR * ) strdup(layer->filter.string));
  if(status != SE_SUCCESS) {
    sde_error(status, 
              "msSDELayerWhichShapes()", 
              "SE_queryinfo_set_where_clause()");
    return(MS_FAILURE);
  }

  status = SE_queryinfo_set_columns(query_info, 
                                    layer->numitems, 
                                    (const char **)layer->items);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_queryinfo_set_columns()");
    return(MS_FAILURE);
  }
  
  /* Join the spatial and feature tables.  If we specify the type of join */
  /* we'll query faster than querying all tables individually (old method) */
  status = SE_queryinfo_set_query_type (query_info,SE_QUERYTYPE_JSF);
  if(status != SE_SUCCESS) {
    sde_error(status, 
              "msSDELayerWhichShapes()", 
              "SE_queryinfo_set_query_type()");
    return(MS_FAILURE);
  }
  


  /* reset the stream */
  status = SE_stream_close(sde->stream, 1);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerGetShape()", "SE_stream_close()");
    return(MS_FAILURE);
  }
  /* Set the stream state back to the state_id of our user-specified version */
  /* This must be done every time after the stream is reset before the  */
  /* query happens. */

  if (!(sde->state_id == SE_DEFAULT_STATE_ID)){

    status =  SE_stream_set_state(sde->stream, 
                                  sde->state_id, 
                                  sde->state_id, 
                                  SE_STATE_DIFF_NOCHECK); 
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_stream_set_state()");
      return(MS_FAILURE);
    }  
  } 

  status = SE_stream_query_with_info(sde->stream, query_info);
  if(status != SE_SUCCESS) {
    sde_error(status, 
              "msSDELayerWhichShapes()", 
              "SE_stream_query_with_info()");
    return(MS_FAILURE);
  }
  
  proc_value = msLayerGetProcessingKey(layer,"QUERYORDER");
  if(proc_value && strcasecmp(proc_value, "ATTRIBUTE") == 0)
    query_order = SE_ATTRIBUTE_FIRST;

  status = SE_stream_set_spatial_constraints( sde->stream, 
                                              query_order, 
                                              FALSE, 
                                              1, 
                                              &constraint);

  if(status != SE_SUCCESS) {
    sde_error(status, 
              "msSDELayerWhichShapes()", 
              "SE_stream_set_spatial_constraints()");
    return(MS_FAILURE);
  }
  
  /* *should* be ready to step through shapes now */
  status = SE_stream_execute(sde->stream); 
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_stream_execute()");
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
int msSDELayerNextShape(layerObj *layer, shapeObj *shape) {
#ifdef USE_SDE
  long status;

  msSDELayerInfo *sde=NULL;
  
  sde = layer->layerinfo;
  if(!sde) {
    msSetError( MS_SDEERR, 
                "SDE layer has not been opened.", 
                "msSDELayerNextShape()");
    return(MS_FAILURE);
  }

  /* fetch the next record from the stream */
  status = SE_stream_fetch(sde->stream);

  if(status == SE_FINISHED)
    return(MS_DONE);
  else if(status != MS_SUCCESS) {
    sde_error(status, "msSDELayerNextShape()", "SE_stream_fetch()");
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
/* msSDELayerGetItems                                                   */
/* -------------------------------------------------------------------- */
/*     Queries the SDE table's column names into layer->iteminfo        */
/* -------------------------------------------------------------------- */
int msSDELayerGetItems(layerObj *layer) {
#ifdef USE_SDE
  int i,j;
  short n;
  long status;

  SE_COLUMN_DEF *itemdefs;

  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;
  if(!sde) {
    msSetError( MS_SDEERR, 
                "SDE layer has not been opened.", 
                "msSDELayerGetItems()");
    return(MS_FAILURE);
  }
  
  if (!sde->row_id_column) {
    sde->row_id_column = (char*) malloc(SE_MAX_COLUMN_LEN);
  }
  sde->row_id_column = msSDELayerGetRowIDColumn(layer);

  status = SE_table_describe(sde->connection, sde->table, &n, &itemdefs);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerGetItems()", "SE_table_describe()");
    return(MS_FAILURE);
  }

  /* 2/27/07 - hobu.  If we can't get the row_id_column right from the table, we're screwed anyway.  
                      stop double-grabbing it here because else where in the code assumes that 
                      layer->items maps layer->itemdefs, which it doesn't if we have dups of
                      the OBJECTID (or other row_id column) */
  layer->numitems = n;

  layer->items = (char **)malloc(layer->numitems*sizeof(char *));
  if(!layer->items) {
    msSetError( MS_MEMERR, "Error allocating layer items array.", "msSDELayerGetItems()");
    return(MS_FAILURE);
  }

  for(i=0; i<n; i++) layer->items[i] = strdup(itemdefs[i].column_name);
  /* layer->items[n] = strdup(sde->row_id_column); */ /* row id */

  if (!layer->iteminfo){
    layer->iteminfo = (SE_COLUMN_DEF *) calloc( layer->numitems, sizeof(SE_COLUMN_DEF));
    if(!layer->iteminfo) {
      msSetError( MS_MEMERR, "Error allocating SDE item information.", "msSDELayerGetItems()");
      return(MS_FAILURE);
    }
  }

  for(i=0; i<layer->numitems; i++) { /* requested columns */
   /* if(strcmp(layer->items[i],sde->row_id_column) == 0)      
      continue; */

    for(j=0; j<n; j++) { /* all columns */
      if(strcasecmp(layer->items[i], itemdefs[j].column_name) == 0) { 
        /* found it */
        ((SE_COLUMN_DEF *)(layer->iteminfo))[i] = itemdefs[j];
        break;
      }
    }
  }
  
  SE_table_free_descriptions(itemdefs);

  return(MS_SUCCESS);
#else
  msSetError( MS_MISCERR, 
              "SDE support is not available.", 
              "msSDELayerGetItems()");
  return(MS_FAILURE);
#endif
}

/* -------------------------------------------------------------------- */
/* msSDELayerGetExtent                                                  */
/* -------------------------------------------------------------------- */
/*     Returns the extent of the SDE layer                              */
/* -------------------------------------------------------------------- */
int msSDELayerGetExtent(layerObj *layer, rectObj *extent) {
#ifdef USE_SDE
  long status;

  SE_ENVELOPE envelope;

  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;
  if(!sde) {
    msSetError(MS_SDEERR,
               "SDE layer has not been opened.", 
               "msSDELayerGetExtent()");
    return(MS_FAILURE);
  }

  status = SE_layerinfo_get_envelope(sde->layerinfo, &envelope);
  if(status != SE_SUCCESS) {
    sde_error(status, 
              "msSDELayerGetExtent()", 
              "SE_layerinfo_get_envelope()");
    return(MS_FAILURE);
  }
  
  extent->minx = envelope.minx;
  extent->miny = envelope.miny;
  extent->maxx = envelope.maxx;
  extent->maxy = envelope.maxy;

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
int msSDELayerGetShape(layerObj *layer, shapeObj *shape, long record) {

#ifdef USE_SDE
  long status;
  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;
  if(!sde) {
    msSetError( MS_SDEERR, 
                "SDE layer has not been opened.", 
                "msSDELayerGetExtent()");
    return(MS_FAILURE);
  }

  /* must be at least one thing to retrieve (i.e. spatial column) */
  if(layer->numitems < 1) { 
    msSetError( MS_MISCERR, 
                "No items requested, SDE requires at least one item.", 
                "msSDELayerGetShape()");
    return(MS_FAILURE);
  }



  /* reset the stream */
  status = SE_stream_close(sde->stream, 1);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerGetShape()", "SE_stream_close()");
    return(MS_FAILURE);
  }

  status = SE_stream_fetch_row( sde->stream, 
                                sde->table, 
                                record, 
                                (short)(layer->numitems), 
                                (const char **)layer->items);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerGetShape()", "SE_stream_fetch_row()");
    return(MS_FAILURE);
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
/* msSDELayerGetShapeVT                                                 */
/* -------------------------------------------------------------------- */
/* Overloaded version for virtual table                                 */
/* -------------------------------------------------------------------- */
int msSDELayerGetShapeVT(layerObj *layer, shapeObj *shape, int tile, long record) {
	return msSDELayerGetShape(layer, shape, record);
}

/* -------------------------------------------------------------------- */
/* msSDELayerInitItemInfo                                               */
/* -------------------------------------------------------------------- */
/*     Inits the stuff we'll be querying from SDE                       */
/* -------------------------------------------------------------------- */
int msSDELayerInitItemInfo(layerObj *layer)
{
#ifdef USE_SDE
  long status;
  short n;
  int i, j;

  SE_COLUMN_DEF *itemdefs;

  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;
  sde->row_id_column = msSDELayerGetRowIDColumn(layer);

  
  if(!sde) {
    msSetError( MS_SDEERR, 
                "SDE layer has not been opened.", 
                "msSDELayerInitItemInfo()");
    return(MS_FAILURE);
  }

  
  status = SE_table_describe(sde->connection, sde->table, &n, &itemdefs);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerGetItemInfo()", "SE_table_describe()");
    return(MS_FAILURE);
  }

if (!layer->iteminfo){
  layer->iteminfo = (SE_COLUMN_DEF *) calloc( layer->numitems, 
                                              sizeof(SE_COLUMN_DEF));
  if(!layer->iteminfo) {
    msSetError( MS_MEMERR,
                "Error allocating SDE item information.", 
                "msSDELayerInitItemInfo()");
    return(MS_FAILURE);
  }
}

  for(i=0; i<layer->numitems; i++) { /* requested columns */
    if(strcmp(layer->items[i],sde->row_id_column) == 0)      
      continue;

    for(j=0; j<n; j++) { /* all columns */
      if(strcasecmp(layer->items[i], itemdefs[j].column_name) == 0) { 
        /* found it */
        ((SE_COLUMN_DEF *)(layer->iteminfo))[i] = itemdefs[j];
        break;
      }
    }

    if(j == n) {
      msSetError( MS_MISCERR, 
                  "Item not found in SDE table.", 
                  "msSDELayerInitItemInfo()");
      return(MS_FAILURE);
    }
  }

  SE_table_free_descriptions(itemdefs);

  return(MS_SUCCESS);
#else
  msSetError( MS_MISCERR, 
              "SDE support is not available.", 
              "msSDELayerInitItemInfo()");
  return(MS_FAILURE);
#endif
}

/* -------------------------------------------------------------------- */
/* msSDELayerFreeItemInfo                                               */
/* -------------------------------------------------------------------- */
void msSDELayerFreeItemInfo(layerObj *layer)
{
#ifdef USE_SDE
  if(layer->iteminfo) {
    SE_table_free_descriptions((SE_COLUMN_DEF *)layer->iteminfo);  
    layer->iteminfo = NULL;
  }
#else
  msSetError( MS_MISCERR, 
              "SDE support is not available.", 
              "msSDELayerFreeItemInfo()");
#endif
}

/* -------------------------------------------------------------------- */
/* msSDELayerGetSpatialColumn                                           */
/* -------------------------------------------------------------------- */
/*     A helper function to return the spatial column for               */ 
/*     an opened SDE layer                                              */
/* -------------------------------------------------------------------- */
char *msSDELayerGetSpatialColumn(layerObj *layer)
{
#ifdef USE_SDE
  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;
  if(!sde) {
    msSetError( MS_SDEERR, 
                "SDE layer has not been opened.", 
                "msSDELayerGetSpatialColumn()");
    return(NULL);
  }

  return(strdup(sde->column));
#else
  msSetError( MS_MISCERR, 
              "SDE support is not available.", 
              "msSDELayerGetSpatialColumn()");
  return(NULL);
#endif
}



/* -------- Raster support -------- 
Turned off for now.

#define MAXCOLORS 256

#ifdef USE_SDERASTER
void msSDEGetColorMap(mapObj *map, layerObj *layer, gdImagePtr img, SE_RASBANDINFO rasterband, int cmap[]) { 
  // FIXME: size check for cmap[] ! 
  LONG rc, cm_bottom, cm, num_cm_entries;
  SE_COLORMAP_TYPE colormap_type;
  SE_COLORMAP_DATA_TYPE data_type;
  void* colormap_data;
  UCHAR* byte_colormap_data;
  USHORT* short_colormap_data;
  colorObj pixel;
  
  if (!SE_rasbandinfo_has_colormap(rasterband)) {
    if(layer->debug) msDebug("msSDEGetColorMap(): Generating color map\n");
    // TODO: other than 8 bit (from drawTIFF mapraster.c) 
    for (cm=0; cm<256; cm++) {
      pixel.red = pixel.green = pixel.blue = pixel.pen = cm;  offsite would be specified in range 0 to 255 

      // if(!MS_COMPARE_COLORS(pixel, layer->offsite))	     
      // cmap[cm] = msAddColorGD(map,img, 0, (cm>>4)*17, (cm>>4)*17, (cm>>4)*17); // use raster color 
      cmap[cm] = gdImageColorAllocate(img, (cm>>4)*17, (cm>>4)*17, (cm>>4)*17);
    }
    return;
  }

  if(layer->debug) msDebug("msSDEGetColorMap(): Fetching color map\n");
  
  // Fetch the colormap data from the rasterband_info structure. 
  rc = SE_rasbandinfo_get_colormap (rasterband,
                                    &colormap_type,
                                    &data_type,
                                    &num_cm_entries,
                                    &colormap_data);

  byte_colormap_data = colormap_data;
  short_colormap_data = colormap_data;

  switch (data_type) {
    case SE_COLORMAP_DATA_BYTE:
      switch(colormap_type) {
        case SE_COLORMAP_RGB:
          cm_bottom = num_cm_entries * 3;
          for (cm=0; cm < cm_bottom; cm++) {
            UCHAR col = byte_colormap_data[cm];
            cmap[cm] = gdImageColorAllocate(img, gdTrueColorGetRed(col), gdTrueColorGetGreen(col), gdTrueColorGetBlue(col));
          }
          break;
        case SE_COLORMAP_RGBA:
          cm_bottom = num_cm_entries * 4;
          for (cm=0; cm < cm_bottom; cm++) {
            UCHAR col = byte_colormap_data[cm];
            cmap[cm] = gdImageColorAllocateAlpha(img, gdTrueColorGetRed(col), gdTrueColorGetGreen(col), gdTrueColorGetBlue(col), gdTrueColorGetAlpha(col));
          }
          break;
        case SE_COLORMAP_NONE: break;
      }
      break;
    case SE_COLORMAP_DATA_SHORT:
      switch(colormap_type) {
        case SE_COLORMAP_RGB:
          cm_bottom = num_cm_entries * 3;
          for (cm=0; cm < cm_bottom; cm++) {
            USHORT col = short_colormap_data[cm];
            cmap[cm] = gdImageColorAllocate(img, gdTrueColorGetRed(col), gdTrueColorGetGreen(col), gdTrueColorGetBlue(col));
          }
          break;
        case SE_COLORMAP_RGBA:
          cm_bottom = num_cm_entries * 4;
          for(cm=0; cm < cm_bottom; cm++) {
            USHORT col = short_colormap_data[cm];
            cmap[cm] = gdImageColorAllocateAlpha(img, gdTrueColorGetRed(col), gdTrueColorGetGreen(col), gdTrueColorGetBlue(col), gdTrueColorGetAlpha(col));
          }
          break;
        case SE_COLORMAP_NONE: break;
      }
      break;
  }
}
#endif
*/

/************************************************************************/
/*                              drawSDE()                              */
/************************************************************************/
/*
int drawSDE(mapObj *map, layerObj *layer, gdImagePtr img)
{
#ifdef USE_SDERASTER
  long status;
  msSDELayerInfo *sde=NULL;
  char **params=NULL;
  int numparams=0;
  
  int i,j; // loop counters 
  int cmap[MAXCOLORS];  
  gdImagePtr rgbimg;
  int col;

  SE_SQL_CONSTRUCT   *sqlc;
  SHORT               num_cols;
  CHAR              **attrs;
  SE_RASTERATTR hAttr;
  SE_RASBANDINFO hRasBnd;
  SE_RASCONSTRAINT hConstraint;
  SE_RASTILEINFO hTile;
  LONG band_width, band_height, num_bands, pixel_type;
  LONG tile_width, tile_height;
  LONG rasterband_id, level, row, column, length;
  unsigned char *pixels;
  BOOL skip_level1;
  int backcol;
  int *tpix;
  SE_ENVELOPE env;
  LONG pixel_offset_x, pixel_offset_y;
  LFLOAT minx, maxx, miny, maxy;
  LFLOAT effminx = 0, effmaxx = 0, effminy = 0, effmaxy = 0;
  LFLOAT cell_size = 0;
  LFLOAT coord_tile_width = 0, coord_tile_height = 0, coord_offset_x = 0, coord_offset_y = 0;
  LONG constrminx = 0, constrminy = 0, constrmaxx = 0, constrmaxy = 0;

  char* proc_value;
  int red_band=0, blue_band=0, green_band=0, alpha_band=0;
  
  msSDELayerOpen(layer);

  sde = layer->layerinfo;
  if(!sde) {
    msSetError(MS_SDEERR, "SDE layer has not been opened.", "drawSDE()");
    return(MS_FAILURE);
  }

  if(layer->debug) msDebug("drawSDE(): Drawing layer %s.\n", layer->name);

  tpix = (int*) malloc(sizeof(int));

  // Allocate an SE_SQL_CONSTRUCT 
  status = SE_sql_construct_alloc (1, &sqlc);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_sql_construct_alloc");
    return(MS_FAILURE);
  }

  // Fill in the details of the SQL query 
  sqlc->where = (char *)malloc(20);
  sqlc->num_tables = 1;
  strcpy (sqlc->tables[0], sde->table);
  strcpy (sqlc->where, "");

  proc_value = NULL;
  proc_value = msLayerGetProcessingKey(layer,"BANDS");
  if (proc_value != NULL) {
    params = split(proc_value, ',', &numparams);
    if(!params) {
      msSetError(MS_MEMERR, 
          "Error spliting SDE raster bands PROCCESSING information.", "drawSDE()");
      return(MS_FAILURE);
    }
  
    if(numparams < 4) {
      msSetError(MS_SDEERR, 
      "Not enough SDE raster bands specified. Need 4 bands -- red,green,blue,alpha", "drawSDE()");
      return(MS_FAILURE);
    }
  
    red_band= atoi(params[0]); 
    green_band = atoi(params[1]);
    blue_band = atoi(params[2]);
    alpha_band = atoi(params[3]);
  }
  
  if (layer->debug) {
     msDebug("our bands were specified: red=%d, green=%d, blue=%d, alpha=%d\n", 
              red_band,green_band,blue_band,alpha_band);
  }
  // Define the number and names of the table columns to be returned 
  
  proc_value = NULL;
  proc_value = msLayerGetProcessingKey(layer,"RASTERCOLUMN");
  if (proc_value == NULL){
          msSetError(MS_MEMERR, 
          "Error spliting SDE raster column from PROCCESSING information.", "drawSDE()");
      return(MS_FAILURE);
  }
  num_cols = 1;
  attrs = (CHAR **) malloc (num_cols * sizeof(CHAR *));
  attrs[0] = proc_value;// sde->column; 

  proc_value = NULL;
  free(proc_value);
  
  // Define the stream query 
  status = SE_stream_query (sde->stream, num_cols, (const CHAR **) attrs, sqlc);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_stream_query");
    return(MS_FAILURE);
  }

  // Execute the query. 
  status = SE_stream_execute (sde->stream);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_stream_execute");
    return(MS_FAILURE);
  }

  // Fetch the first row. 
  status = SE_stream_fetch (sde->stream);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_stream_fetch");
    return(MS_FAILURE);
  }

  // Obtain the image metadata in the current row. 
  status = SE_rasterattr_create (&hAttr, FALSE);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_rasterattr_create");
    return(MS_FAILURE);
  }

  status = SE_stream_get_raster (sde->stream, 1, hAttr);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_stream_get_raster");
    return(MS_FAILURE);
  }

  minx = map->extent.minx;
  maxx = map->extent.maxx;
  miny = map->extent.miny;
  maxy = map->extent.maxy;
  if(layer->debug) msDebug("map->extent minx: %lf miny: %lf maxx: %lf maxy: %lf\n",
    map->extent.minx, map->extent.miny, map->extent.maxx, map->extent.maxy);      
  
  // Get raster band metadata 

  status = SE_rasterattr_get_rasterband_info (hAttr, &hRasBnd, 1);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_rasterattr_get_rasterband_info");
    return(MS_FAILURE);
  }
  
  status = SE_rasterattr_get_tile_size (hAttr, &tile_width, &tile_height);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_rasterattr_get_tile_size");
    return(MS_FAILURE);
  }
  
  status = SE_rasterattr_get_pixel_type (hAttr, &pixel_type);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_rasterattr_get_pixel_type");
    return(MS_FAILURE);
  }
  
  if(layer->debug) msDebug("Raster tile_width: %ld tile_height: %ld BitsPerPixel: %d\n",
    tile_width, tile_height, SE_PIXEL_TYPE_GET_DEPTH(pixel_type));

  if (SE_PIXEL_TYPE_GET_DEPTH(pixel_type) != 8) {
    sde_error(status, "drawSDE()", "Sorry, only 8 bits per pixel supported.");
    return(MS_FAILURE);
  }
    

  status = SE_rasterattr_get_max_level (hAttr, &level, &skip_level1);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_rasterattr_get_max_level");
    return(MS_FAILURE);
  }

  for (; level >= 0; level--) {
    if (layer->debug) msDebug("Try pyramid level: %ld \n",level);  

    // Get the pixel dimensions of the current level.  
    status = SE_rasterattr_get_image_size_by_level(hAttr,
                                               &band_width,
                                               &band_height,
                                               &pixel_offset_x,
                                               &pixel_offset_y,
                                               &num_bands,
                                               level);
    if(status != SE_SUCCESS) {
      sde_error(status, "drawSDE()", "SE_rasterattr_get_image_size_by_level");
      return(MS_FAILURE);
    }    
    if(layer->debug) msDebug("Raster band_width: %ld band_height: %ld pixel_offset_x: %ld pixel_offset_y: %ld num_bands: %ld\n",
      band_width, band_height, pixel_offset_x, pixel_offset_y, num_bands);
              
    status = SE_rasterattr_get_extent_by_level(hAttr, &env, &coord_offset_x, &coord_offset_y, level);
    if(status != SE_SUCCESS) {
      sde_error(status, "drawSDE()", "SE_rasterattr_get_extent_by_level()");
      return(MS_FAILURE);
    }
    if(layer->debug) msDebug("Raster extent minx: %lf miny: %lf maxx: %lf maxy: %lf offset_x: %lf offset_y: %lf\n",
      env.minx, env.miny, env.maxx, env.maxy, coord_offset_x, coord_offset_y);
  
    // calculate the pixel cell size in world coordinates 
    cell_size = (env.maxx - env.minx) / (band_width - 1);
        
    // compute the real world coordinate width and height 
    coord_tile_width = cell_size * tile_width;
    coord_tile_height = cell_size * tile_height;
  
    // compute the tile coordinates 
    effminx = MS_MAX(env.minx - coord_offset_x, minx);
    effmaxx = MS_MIN(env.maxx - coord_offset_x, maxx);
    effminy = MS_MAX(env.miny - coord_offset_y, miny);
    effmaxy = MS_MIN(env.maxy - coord_offset_y, maxy);
    constrminx = (LONG)((effminx - (env.minx - coord_offset_x)) / coord_tile_width);
    constrmaxx = (LONG)((effmaxx - (env.minx - coord_offset_x)) / coord_tile_width);
    constrminy = (LONG)(((env.maxy - coord_offset_y) - effmaxy) / coord_tile_height);
    constrmaxy = (LONG)(((env.maxy - coord_offset_y) - effminy) / coord_tile_height);
    
    if(layer->debug) msDebug("SE_rasconstraint_set_envelope minx: %ld miny: %ld maxx: %ld maxy: %ld\n",
      constrminx, constrminy, constrmaxx, constrmaxy);
    if(layer->debug) msDebug("Eff. SDE raster extend effminx: %lf effminy: %lf effmaxx: %lf effmaxy: %lf\n",
      effminx, effminy, effmaxx, effmaxy);

    if (((maxx-minx)/cell_size) >= img->sx && ((maxy-miny)/cell_size) >= img->sy)
      break; // use this level         
  }

  // make sure the user enter extent overlaps the image extent 
  if (minx > env.maxx || maxx < env.minx || miny > env.maxy || maxy < env.miny) {
    sde_error(status, "drawSDE()", "Invalid entry: The coordinate extent enter does not overlap image extent");
    return(MS_FAILURE);
  }
      
  // Add constraint & get the tiles. 

  status = SE_rasconstraint_create (&hConstraint);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_rasconstraint_create");
    return(MS_FAILURE);
  }
  
  if (level > 0) {
    status = SE_rasconstraint_set_level(hConstraint, level);
    if(status != SE_SUCCESS) {
      sde_error(status, "drawSDE()", "SE_rasconstraint_set_level");
      return(MS_FAILURE);
    }
  }

  status = SE_rasconstraint_set_envelope (hConstraint, constrminx, constrminy, constrmaxx, constrmaxy);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_rasconstraint_set_envelope");
    return(MS_FAILURE);
  }

  status = SE_stream_query_raster_tile (sde->stream, hConstraint);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_stream_query_raster_tile");
    return(MS_FAILURE);
  }

  status = SE_rastileinfo_create (&hTile);
  if(status != SE_SUCCESS) {
    sde_error(status, "drawSDE()", "SE_rastileinfo_create");
    return(MS_FAILURE);
  }

  // Setup RGB image 
  rgbimg = gdImageCreateTrueColor((constrmaxx-constrminx+1)*tile_width, (constrmaxy-constrminy+1)*tile_height);
  // int backcol = gdImageColorAllocate(rgbimg, map->imagecolor.red, map->imagecolor.green, map->imagecolor.blue); 
  backcol = gdImageColorAllocate(rgbimg, 0, 0, 0);
  gdImageFilledRectangle(rgbimg, 0, 0, rgbimg->sx, rgbimg->sy, backcol);
  
  // set up the color map (for 1-band images) 
  msSDEGetColorMap(map, layer, rgbimg, hRasBnd, cmap);

  while (SE_SUCCESS == status)
  {
    status = SE_stream_get_raster_tile (sde->stream, hTile);
    if (status == SE_SUCCESS)
    {
      status = SE_rastileinfo_get_band_id (hTile, &rasterband_id);
      if(status != SE_SUCCESS) {
        sde_error(status, "drawSDE()", "SE_rastileinfo_get_band_id");
        return(MS_FAILURE);
      }

      // status = SE_rastileinfo_get_level (hTile, &level);
      if(status != SE_SUCCESS) {
        sde_error(status, "drawSDE()", "SE_rastileinfo_get_level");
        return(MS_FAILURE);
      }
      if(layer->debug) msDebug("Got band from level %ld\n", level); 

      status = SE_rastileinfo_get_rowcol (hTile, &row, &column);
      if(status != SE_SUCCESS) {
        sde_error(status, "drawSDE()", "SE_rastileinfo_get_rowcol_number");
        return(MS_FAILURE);
      }

      status = SE_rastileinfo_get_pixel_data (hTile, (void*)&pixels, &length);
      if(status != SE_SUCCESS) {
        sde_error(status, "drawSDE()", "SE_rastileinfo_get_pixel_data");
        return(MS_FAILURE);
      }

      if (length==0) continue; // holes (partial tiles: length != tile_height*tile_width) 
      
      for(i=0; i<tile_height; i++)
        for(j=0; j<tile_width; j++) {
          if (num_bands == 1) {
            col = cmap[pixels[i*tile_width+j]];
          } else {
            col = pixels[i*tile_width+j];
            if (rasterband_id == red_band){
              col <<= 16;
            }
            else if (rasterband_id == green_band){
              col <<= 8; 
            }
            else if (rasterband_id == blue_band){
              col = col;
            }
            else if (rasterband_id == alpha_band){
              col = (col & 0x7F) << 24;
            }
            else {
              col = 0;
            }
          }
          tpix = &rgbimg->tpixels[(row-constrminy)*tile_height+i][(column-constrminx)*tile_width+j];

          if (rasterband_id == 1)
            *tpix = col;
          else
            *tpix |= col;
      }
    }
  }

  // Release memory 
  SE_rastileinfo_free(hTile);
  SE_rasconstraint_free(hConstraint);
  SE_rasterattr_free(hAttr);
  free (attrs);
  free (sqlc->where);
  free (&tpix);
  SE_sql_construct_free (sqlc);
  // ret = SE_stream_free (sde->stream); 

  // backcol = gdImageColorResolve(img, map->imagecolor.red, map->imagecolor.green, map->imagecolor.blue); 
  // gdImageFilledRectangle(img, 0, 0, img->sx, img->sy, backcol); 
  
  gdImageCopyResized(img, rgbimg, 0, 0, 
    (LONG) ((minx - (env.minx-coord_offset_x)-constrminx*coord_tile_width) / cell_size),
    (LONG) (((env.maxy - coord_offset_y) - maxy -constrminy*coord_tile_height) / cell_size),
    img->sx, img->sy,
    (LONG) ((maxx-minx)/cell_size), (LONG) ((maxy-miny)/cell_size));
    // int dstX, int dstY, int srcX, int srcY, int destW, int destH, int srcW, int srcH) 
  gdImageDestroy(rgbimg);
  
  // using pooled connections for SDE, closed when map file is closed 
  // msSDELayerClose(layer);  

  return(0);

} */

/* -------------------------------------------------------------------- */
/* msSDELayerCreateItems                                                */
/* -------------------------------------------------------------------- */
/* Special item allocator for SDE                                       */
/* -------------------------------------------------------------------- */
int
msSDELayerCreateItems(layerObj *layer,
                      int nt) 
{
#ifdef USE_SDE
    /* should be more than enough space, 
     * SDE always needs a couple of additional items  
     */

    layer->items = (char **)calloc(nt+2, sizeof(char *)); 
    if( ! layer->items) {
        msSetError(MS_MEMERR, NULL, "msSDELayerCreateItems()");
        return(MS_FAILURE);
      }
    layer->items[0] = msSDELayerGetRowIDColumn(layer); /* row id */
    layer->items[1] = msSDELayerGetSpatialColumn(layer);
    layer->numitems = 2;
    return MS_SUCCESS;

#else
  msSetError( MS_MISCERR, 
              "SDE support is not available.", 
              "msSDELayerGetRowIDColumn()");
  return(MS_FAILURE);
#endif
}

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
    layer->vtable->LayerGetShape = msSDELayerGetShapeVT;
    layer->vtable->LayerClose = msSDELayerClose;
    layer->vtable->LayerGetItems = msSDELayerGetItems;
    layer->vtable->LayerGetExtent = msSDELayerGetExtent;

    /* layer->vtable->LayerGetAutoStyle, use default */
    /* layer->vtable->LayerApplyFilterToLayer, use default */

    /* SDE uses pooled connections, close from msCloseConnections */
    /*layer->vtable->LayerCloseConnection = msSDELayerCloseConnection;*/

    layer->vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;
    layer->vtable->LayerCreateItems = msSDELayerCreateItems;
    /* layer->vtable->LayerGetNumFeatures, use default */

    return MS_SUCCESS;
}


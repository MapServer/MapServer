/******************************************************************************
 * $Id$
 *
 * $Log$
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

#include "map.h"
#include "maperror.h"

#ifdef USE_SDE
#include <sdetype.h> /* ESRI SDE Client Includes */
#include <sdeerno.h>

#define MS_SDE_MAXBLOBSIZE 1024*50 // 50 kbytes
#define MS_SDE_NULLSTRING "<null>"
#define MS_SDE_SHAPESTRING "<shape>"
#define MS_SDE_TIMEFMTSIZE 128 // bytes
#define MS_SDE_TIMEFMT "%T %m/%d/%Y"
#define MS_SDE_ROW_ID_COLUMN "SE_ROW_ID"

typedef struct { 
  SE_CONNECTION connection;
  SE_LAYERINFO layerinfo;
  SE_COORDREF coordref;
  SE_STREAM stream;
  long state_id;
  char *table, *column, *row_id_column;
} msSDELayerInfo;

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
  
  status= SE_connection_free_all_locks ((SE_CONNECTION)conn_handle);
  if (status == SE_SUCCESS) {
    SE_connection_free((SE_CONNECTION)conn_handle);
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
/* sdeShapeCopy                                                         */
/* -------------------------------------------------------------------- */
/*     Copies a SDE shape into a MapServer shapeObj                     */
/* -------------------------------------------------------------------- */
static int sdeShapeCopy(SE_SHAPE inshp, shapeObj *outshp) {
	
  long numparts, numsubparts, numpoints;
  long *subparts=NULL;
  SE_POINT *points=NULL;
  SE_ENVELOPE envelope;
  long type, status;

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
    return(MS_SUCCESS); // skip null shapes
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

  SE_shape_get_num_points(inshp, 0, 0, &numpoints);
  SE_shape_get_num_parts(inshp, &numparts, &numsubparts);

  if(numsubparts > 0) {
    subparts = (long *)malloc(numsubparts*sizeof(long));
    if(!subparts) {
      msSetError( MS_MEMERR, 
                  "Unable to allocate parts array.", 
                  "sdeCopyShape()");
      return(MS_FAILURE);
    }
  }

  points = (SE_POINT *)malloc(numpoints*sizeof(SE_POINT));
  if(!points) {
    msSetError( MS_MEMERR, 
                "Unable to allocate points array.", 
                "sdeCopyShape()");
    return(MS_FAILURE);
  }

  status = SE_shape_get_all_points( inshp, 
                                    SE_DEFAULT_ROTATION, 
                                    NULL, 
                                    subparts, 
                                    points, 
                                    NULL, 
                                    NULL);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeCopyShape()", "SE_shape_get_all_points()");
    return(MS_FAILURE);
  }

  k = 0; // overall point counter
  for(i=0; i<numsubparts; i++) {
    
    if( i == numsubparts-1)
      line.numpoints = numpoints - subparts[i];
    else
      line.numpoints = subparts[i+1] - subparts[i];

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

  free(subparts);
  free(points);

  // finally copy the bounding box for the entire shape
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

  short shortval; // new gdv
  float floatval;

  SE_COLUMN_DEF *itemdefs;
  SE_SHAPE shapeval=0;

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

      shape->values[i] = (char *)malloc(64); // should be enough
      sprintf(shape->values[i], "%ld", shape->index);
      continue;
    }    
    
    switch(itemdefs[i].sde_type) {
    case SE_SMALLINT_TYPE:
      // changed by gdv
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
      // changed by gdv
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
        shape->values[i][0] = '\0'; // empty string
      else if(status != SE_SUCCESS) {
        sde_error(status, "sdeGetRecord()", "SE_stream_get_string()");
        return(MS_FAILURE);
      }
      break;
    case SE_BLOB_TYPE:
      shape->values[i] = strdup("<blob>");
      msSetError( MS_SDEERR, 
                  "Retrieval of BLOBs is not yet supported.", 
                  "sdeGetRecord()");
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
  
  // copy sde shape to a mapserver shape
  status = sdeShapeCopy(shapeval, shape);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  // clean up
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
  int numparams=0;
  SE_ERROR error;
  SE_STATEINFO state;
  SE_VERSIONINFO version;

  msSDELayerInfo *sde;

  // layer already open, silently return
  if(layer->layerinfo) return(MS_SUCCESS); 

  // allocate space for SDE structures
  sde = (msSDELayerInfo *) malloc(sizeof(msSDELayerInfo));
  if(!sde) {
    msSetError( MS_MEMERR, 
                "Error allocating SDE layer structure.", 
                "msSDELayerOpen()");
    return(MS_FAILURE);
  }
 
  // initialize the table and spatial column names
  sde->table = sde->column = NULL;
  
  // initialize the connection using the connection pooling API
  sde->connection = (SE_CONNECTION) msConnPoolRequest( layer );
  
  // If we weren't returned a connection, initialize a new one
  if (!(sde->connection)) {
    if (layer->debug) 
      msDebug("msSDELayerOpen(): "
              "Layer %s opened from scratch.\n", layer->name);

    // Split the connection parameters and make sure we have enough of them
    params = split(layer->connection, ',', &numparams);
    if(!params) {
      msSetError( MS_MEMERR, 
                  "Error spliting SDE connection information.", 
                  "msSDELayerOpen()");
      return(MS_FAILURE);
    }
    if(numparams < 5) {
      msSetError( MS_SDEERR, 
                  "Not enough SDE connection parameters specified.", 
                  "msSDELayerOpen()");
      return(MS_FAILURE);
    }
  
    // Create the connection handle and put into the sde->connection
    status = SE_connection_create(params[0], 
                                  params[1], 
                                  params[2], 
                                  params[3], 
                                  params[4], 
                                  &error, 
                                  &(sde->connection));
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_connection_create()");
      return(MS_FAILURE);
    }
/* ------------------------------------------------------------------------- */
/* Set the concurrency for the connection.  This is to support threading.    */
/* ------------------------------------------------------------------------- */
    status = SE_connection_set_concurrency( sde->connection, 
                                            SE_UNPROTECTED_POLICY);
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_connection_set_concurrency()");
      return(MS_FAILURE);
    }
    
    // Register the connection with the connection pooling API.  Give 
    // msSDECloseConnection as the function to call when we run out of layer 
    // instances using it
    msConnPoolRegister(layer, sde->connection, msSDECloseConnection);
    msFreeCharArray(params, numparams); // done with parameter list
  }
  
  // Split the DATA member into its parameters using the comma
  // Periods (.) are use to denote table names and schemas in SDE, 
  // as are underscores (_).
  params = split(layer->data, ',', &numparams);
  if(!params) {
    msSetError(MS_MEMERR, 
        "Error spliting SDE layer information.", "msSDELayerOpen()");
    return(MS_FAILURE);
  }

  if(numparams < 2) {
    msSetError(MS_SDEERR, 
    "Not enough SDE layer parameters specified.", "msSDELayerOpen()");
    return(MS_FAILURE);
  }

  sde->table = params[0]; 
  sde->column = params[1];

  status = SE_versioninfo_create (&(version));
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_versioninfo_create()");
    return(MS_FAILURE);
  }
  if (numparams < 3){ 
    // User didn't specify a version, we'll use SDE.DEFAULT
    if (layer->debug) 
      msDebug("msSDELayerOpen(): Layer %s did not have a" 
              "specified version.  Using SDE.DEFAULT.\n", layer->name);
      status = SE_version_get_info(sde->connection, "SDE.DEFAULT", version);
  } 
  else {
    if (layer->debug) 
      msDebug("msSDELayerOpen(): Layer %s specified version %s.\n", 
              layer->name, 
              params[2]);
    status = SE_version_get_info(sde->connection, params[2], version);
  }
   
  if(status != SE_SUCCESS) {
    if (status == SE_INVALID_RELEASE) {
      // The user has incongruent versions of SDE, ie 8.2 client and 
      // 8.3 server set the state_id to SE_DEFAULT_STATE_ID, which means 
      // no version queries are done
      sde->state_id = SE_DEFAULT_STATE_ID;
    }
    else {
      sde_error(status, "msSDELayerOpen()", "SE_version_get_info()");
      return(MS_FAILURE);
    }
  } 
  // Get the STATEID from the given version and set the stream to 
  // that if we didn't already set it to SE_DEFAULT_STATE_ID.  
  if (!(sde->state_id == SE_DEFAULT_STATE_ID)){
    status = SE_versioninfo_get_state_id(version, &sde->state_id);
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_versioninfo_get_state_id()");
      return(MS_FAILURE);
    }
    status = SE_stateinfo_create (&state);
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_stateinfo_create()");
      return(MS_FAILURE);
    }    
    status = SE_state_get_info(sde->connection, sde->state_id, state);
    if(status != SE_SUCCESS) {
      sde_error(status, "msSDELayerOpen()", "SE_state_get_info()");
      return(MS_FAILURE);
    }  
    if (SE_stateinfo_is_open (state)) {
      // If the state is open for edits, we shouldn't be querying from it
      sde_error(status, 
                "msSDELayerOpen()", 
                "SE_stateinfo_is_open() -- State for version is open");
      return(MS_FAILURE);
    }
    SE_stateinfo_free (state); 
    SE_versioninfo_free(version);
  } // if (!(sde->state_id == SE_DEFAULT_STATE_ID))
  
  status = SE_layerinfo_create(NULL, &(sde->layerinfo));
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_layerinfo_create()");
    return(MS_FAILURE);
  }

  status = SE_layer_get_info( sde->connection, 
                              params[0], 
                              params[1], 
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

  status = SE_stream_create(sde->connection, &(sde->stream));
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_stream_create()");
    return(MS_FAILURE);
  }
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
  // point to the SDE layer information 
  // (note this might actually be in another layer)
  layer->layerinfo = sde; 

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
void msSDELayerClose(layerObj *layer) {
#ifdef USE_SDE

  msSDELayerInfo *sde=NULL;

  sde = layer->layerinfo;
  if (sde == NULL) return;  // Silently return if layer not opened.

  if(layer->debug) 
    msDebug("msSDELayerClose(): Closing layer %s.\n", layer->name);
	
  if (sde->layerinfo) SE_layerinfo_free(sde->layerinfo);
  if (sde->coordref) SE_coordref_free(sde->coordref);
  if(sde->table) free(sde->table);
  if(sde->column) free(sde->column);
  if(sde->row_id_column) free(sde->row_id_column);

  msConnPoolRelease( layer, sde->connection );  
  free(layer->layerinfo);
  layer->layerinfo = NULL;
  sde->connection = NULL;
#else
  msSetError( MS_MISCERR, 
              "SDE support is not available.", 
              "msSDELayerClose()");
  return;
#endif
}

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
  
  // there is NO overlap, return MS_DONE
  // (FIX: use this in ALL which shapes functions)
  if(envelope.minx > rect.maxx) return(MS_DONE); 
  if(envelope.maxx < rect.minx) return(MS_DONE);
  if(envelope.miny > rect.maxy) return(MS_DONE);
  if(envelope.maxy < rect.miny) return(MS_DONE);

  // set spatial constraint search shape
  // crop against SDE layer extent *argh*
  envelope.minx = MS_MAX(rect.minx, envelope.minx); 
  envelope.miny = MS_MAX(rect.miny, envelope.miny);
  envelope.maxx = MS_MIN(rect.maxx, envelope.maxx);
  envelope.maxy = MS_MIN(rect.maxy, envelope.maxy);

  status = SE_shape_generate_rectangle(&envelope, shape);
  if(status != SE_SUCCESS) {
    sde_error(status, 
              "msSDELayerWhichShapes()", 
              "SE_shape_generate_rectangle()");
    return(MS_FAILURE);
  }
  constraint.filter.shape = shape;

  // set spatial constraint column and table
  strcpy(constraint.table, sde->table);
  strcpy(constraint.column, sde->column);

  // set a couple of other spatial constraint properties
  constraint.method = SM_ENVP;
  constraint.filter_type = SE_SHAPE_FILTER;
  constraint.truth = TRUE;

  // See http://forums.esri.com/Thread.asp?c=2&f=59&t=108929&mc=4#msgid310273
  // SE_queryinfo is a new SDE struct in ArcSDE 8.x that is a bit easier 
  // (and faster) to use and will allow us to support joins in the future.  HCB
  status = SE_queryinfo_create (&query_info);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_queryinfo_create()");
    return(MS_FAILURE);
  }

  // set the tables -- just one at this point
  status = SE_queryinfo_set_tables (query_info, 
                                    1, 
                                    (const CHAR **) &(sde->table),
                                    NULL);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_queryinfo_create()");
    return(MS_FAILURE);
  }

  // set the "where" clause
  if(!(layer->filter.string))
    // set to empty string
    status = SE_queryinfo_set_where_clause (query_info, 
                                            (const CHAR * ) strdup(""));
  else
    // set to the layer's filter.string
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
  
  // Join the spatial and feature tables.  If we specify the type of join
  // we'll query faster than querying all tables individually (old method)
  status = SE_queryinfo_set_query_type (query_info,SE_QUERYTYPE_JSF);
  if(status != SE_SUCCESS) {
    sde_error(status, 
              "msSDELayerWhichShapes()", 
              "SE_queryinfo_set_query_type()");
    return(MS_FAILURE);
  }

  // reset the stream
  status = SE_stream_close(sde->stream, 1);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_stream_close()");
    return(MS_FAILURE);
  }
  
  //Set the stream state back to the state_id of our user-specified version
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

  status = SE_stream_set_spatial_constraints( sde->stream, 
                                              SE_SPATIAL_FIRST, 
                                              FALSE, 
                                              1, 
                                              &constraint);
  if(status != SE_SUCCESS) {
    sde_error(status, 
              "msSDELayerWhichShapes()", 
              "SE_stream_set_spatial_constraints()");
    return(MS_FAILURE);
  }
  
  // *should* be ready to step through shapes now
  status = SE_stream_execute(sde->stream); 
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_stream_execute()");
    return(MS_FAILURE);
  }

  // clean-up
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

  // fetch the next record from the stream
  status = SE_stream_fetch(sde->stream);

  if(status == SE_FINISHED)
    return(MS_DONE);
  else if(status != MS_SUCCESS) {
    sde_error(status, "msSDELayerNextShape()", "SE_stream_fetch()");
    return(MS_FAILURE);
  }

  // get the shape and values (first column is the shape id, 
  // second is the shape itself)
  status = sdeGetRecord(layer, shape);
  if(status != MS_SUCCESS)
    return(MS_FAILURE); // something went wrong fetching the record/shape

  if(shape->numlines == 0) // null shape, skip it
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

  sde->row_id_column = (char*) malloc(SE_MAX_COLUMN_LEN);
  sde->row_id_column = msSDELayerGetRowIDColumn(layer);
  
  status = SE_table_describe(sde->connection, sde->table, &n, &itemdefs);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerGetItems()", "SE_table_describe()");
    return(MS_FAILURE);
  }

  layer->numitems = n+1;

  layer->items = (char **)malloc(layer->numitems*sizeof(char *));
  if(!layer->items) {
    msSetError( MS_MEMERR, 
                "Error allocating layer items array.", 
                "msSDELayerGetItems()");
    return(MS_FAILURE);
  }

  for(i=0; i<n; i++) layer->items[i] = strdup(itemdefs[i].column_name);
  layer->items[n] = strdup(sde->row_id_column); // row id

  layer->iteminfo = (SE_COLUMN_DEF *) calloc( layer->numitems, 
                                              sizeof(SE_COLUMN_DEF));
  if(!layer->iteminfo) {
    msSetError( MS_MEMERR, 
                "Error allocating SDE item information.", 
                "msSDELayerGetItems()");
    return(MS_FAILURE);
  }


  
  for(i=0; i<layer->numitems; i++) { // requested columns
    if(strcmp(layer->items[i],sde->row_id_column) == 0)      
      continue;

    for(j=0; j<n; j++) { // all columns
      if(strcasecmp(layer->items[i], itemdefs[j].column_name) == 0) { 
        // found it
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

  // must be at least one thing to retrieve (i.e. spatial column)
  if(layer->numitems < 1) { 
    msSetError( MS_MISCERR, 
                "No items requested, SDE requires at least one item.", 
                "msSDELayerGetShape()");
    return(MS_FAILURE);
  }

  // reset the stream
  status = SE_stream_close(sde->stream, 1);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerGetShape()", "SE_stream_close()");
    return(MS_FAILURE);
  }

  // Set the stream state back to the state_id of our user-specified version
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
    return(MS_FAILURE); // something went wrong fetching the record/shape

  return(MS_SUCCESS);
#else
  msSetError( MS_MISCERR,  
              "SDE support is not available.", 
              "msSDELayerGetShape()");
  return(MS_FAILURE);
#endif
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

  sde->row_id_column = (char*) malloc(SE_MAX_COLUMN_LEN);
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

  layer->iteminfo = (SE_COLUMN_DEF *) calloc( layer->numitems, 
                                              sizeof(SE_COLUMN_DEF));
  if(!layer->iteminfo) {
    msSetError( MS_MEMERR,
                "Error allocating SDE item information.", 
                "msSDELayerInitItemInfo()");
    return(MS_FAILURE);
  }

  for(i=0; i<layer->numitems; i++) { // requested columns
    if(strcmp(layer->items[i],sde->row_id_column) == 0)      
      continue;

    for(j=0; j<n; j++) { // all columns
      if(strcasecmp(layer->items[i], itemdefs[j].column_name) == 0) { 
        // found it
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
  char* row_id_column;
  SE_REGINFO registration;

  msSDELayerInfo *sde=NULL;
  sde = layer->layerinfo;
  row_id_column = (char*) malloc(SE_MAX_COLUMN_LEN);
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
                                        strdup(sde->table), 
                                        registration);
    if(status != SE_SUCCESS) {
      sde_error(status, 
                "msSDELayerGetRowIDColumn()", 
                "SE_registration_get_info()");
      return(NULL);
    }
    
    status= SE_reginfo_get_rowid_column ( registration, 
                                          row_id_column, 
                                          &column_type);
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
      SE_reginfo_free(registration);
      return (MS_SDE_ROW_ID_COLUMN);
    }
    
    SE_reginfo_free(registration);
    if (row_id_column){
      return (strdup(row_id_column)); 
    }
    else {
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


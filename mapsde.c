#include "map.h"
#include "maperror.h"

/*
** Start SDE/MapServer helper functions.
*/

#ifdef USE_SDE
static void sde_error(long error_code, char *routine, char *sde_routine) {
  char error_string[SE_MAX_MESSAGE_LENGTH];

  error_string[0] = '\0';
  SE_error_get_string(error_code, error_string);

  sprintf(ms_error.message, "%s: %s. (%ld)", sde_routine, error_string, error_code);
  msSetError(MS_SDEERR, ms_error.message, routine);

  return;
}

static int sdeRectOverlap(SE_ENVELOPE *a, SE_ENVELOPE *b)
{
  if(a->minx > b->maxx) return(MS_FALSE);
  if(a->maxx < b->minx) return(MS_FALSE);
  if(a->miny > b->maxy) return(MS_FALSE);
  if(a->maxy < b->miny) return(MS_FALSE);
  return(MS_TRUE);
}

static int sdeRectContained(SE_ENVELOPE *a, SE_ENVELOPE *b)
{
  if(a->minx >= b->minx && a->maxx <= b->maxx)
    if(a->miny >= b->miny && a->maxy <= b->maxy)
      return(MS_TRUE);
  return(MS_FALSE);  
}

static int sdeShapeCopy(SE_SHAPE inshp, shapeObj *outshp) {
  long numparts, numsubparts, numpoints;
  long *subparts=NULL;
  SE_POINT *points=NULL;
  long type, status;

  lineObj line={0,NULL};

  int i,j,k,l;
  
  SE_shape_get_type(inshp, &type);

  if(type == SG_NIL_SHAPE) return(0); // skip null shapes

  SE_shape_get_num_points(inshp, 0, 0, &numpoints);
  SE_shape_get_num_parts(inshp, &numparts, &numsubparts);

  if(numsubparts > 0) {
    subparts = (long *)malloc(numsubparts*sizeof(long));
    if(!subparts) {
      msSetError(MS_MEMERR, "Unable to allocate parts array.", "sdeTransformShape()");
      return(-1);
    }
    
  }

  points = (SE_POINT *)malloc(numpoints*sizeof(SE_POINT));
  if(!points) {
    msSetError(MS_MEMERR, "Unable to allocate points array.", "sdeTransformShape()");
    return(-1);
  }

  status = SE_shape_get_all_points(inshp, SE_DEFAULT_ROTATION, NULL, subparts, points, NULL, NULL);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeTransformShape()", "SE_shape_get_all_points()");
    return(-1);
  }

  l = 0; // overall point counter
  for(i=0; i<numsubparts; i++) {
    
    if( i == numsubparts-1)
      line.numpoints = numpoints - subparts[i];
    else
      line.numpoints = subparts[i+1] - subparts[i];

    line.point = (pointObj *)malloc(sizeof(pointObj)*line.numpoints);
    if(!line.point) {
      msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", "sdeShapeCopy()");
      return(-1);
    }
     
    for(j=0; j < line.numpoints; j++) {
      line.point[j].x = points[l].x; 
      line.point[j].y = points[l].y;     
      l++;
    }

    msAddLine(outshp, &line);
    free(line.point);
  }

  free(subparts);
  free(points);

  return(0);
}
#endif

/*
** Start SDE/MapServer library functions.
*/

// connects, gets basic information and opens a stream
int msSDELayerOpen(layer) {
#ifdef USE_SDE
  long status;
  char **params;
  int numparams;

  SE_ERROR error;

  sdeLayerObj *sde;

  if (layer->sdelayerinfo) return MS_SUCCESS; // layer already open
 
  params = split(layer->connection, ',', &numparams);
  if(!params) {
    msSetError(MS_MEMERR, "Error spliting SDE connection information.", "msSDELayerOpen()");
    return(MS_FAILURE);
  }

  if(numparams < 5) {
    msSetError(MS_SDEERR, "Not enough SDE connection parameters specified.", "msSDELayerOpen()");
    return(MS_FAILURE);
  }

  sde = (sdeLayerObj *) malloc(sizeof(sdeLayerObj));
  if(!sde) {
    msSetError(MS_MEMERR, "Error allocating SDE layer structure.", "msSDELayerOpen()");
    return(MS_FAILURE);
  }
  layer->sdelayerinfo = sde;

  // initialize a few things
  sde->items = NULL;
  sde->numitems = 0;
  sde->table = sde->column = 0;

  status = SE_connection_create(params[0], params[1], params[2], params[3], params[4], &error, &(sde->connection));
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerOpen()", "SE_connection_create()");
    return(MS_FAILURE);
  }

  msFreeCharArray(params, numparams);

  return(MS_SUCCESS);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msSDELayerOpen()");
  return(MS_FAILURE);
#endif
}

void msSDELayerClose(layer) {
#ifdef USE_SDE
  SE_stream_free(layer->sdelayerinfo->stream);
  SE_connection_free(layer->sdelayerinfo->connection);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msSDELayerClose()");
  return;
#endif
}

int msSDELayerNextShape(layer, shape) {
#ifdef USE_SDE
  long id, status;
  sdeLayerObj *sde=NULL;

  sde = layer->sdelayerinfo;

  // fetch the next record from the stream
  status = SE_stream_fetch(sde->stream);
  if(status == SE_FINISHED)
    return(MS_DONE);
  else if(status != MS_SUCCESS) {
    sde_error(status, "msSDELayerNextShape()", "SE_stream_fetch()");
    return(MS_FAILURE);
  }

  // get the shape and attributes (first column is the shape, second is the shape id)

  if(SE_shape_is_nil(shape)) 
    return(msSDELayerNextShape(layer, shape));
  
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msSDELayerNextShape()");
  return(MS_FAILURE);
#endif
}

int msSDELayerGetShape(layerObj *layer, shapeObj *shape, int record, int allitems) {
#ifdef USE_SDE
  return(MS_FAILURE);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msSDELayerGetShape()");
  return(MS_FAILURE);
#endif
}	

// starts a stream query using spatial filter (and optionally attributes)
int msSDELayerWhichShapes(layerObj *layer, char *shapepath, rectObj rect, projectionObj *proj) {
#ifdef USE_SDE
  int i;
  long status;
  char **params, **columns;
  int numparams, numcolumns;

  SE_ERROR error;
  SE_ENVELOPE envelope;
  SE_SHAPE shape=0;
  SE_SQL_CONSTRUCT *sql;
  SE_FILTER constraint;

  sdeLayerObj *sde=NULL;

  sde = layer->sdelayerinfo;

  status = SE_shape_create(sde->coordref, &shape); // allocate early, might be threading problems
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_shape_create()");
    return(MS_FAILURE);
  }

  params = split(layer->data, ',', &numparams);
  if(!params) {
    msSetError(MS_MEMERR, "Error spliting SDE layer information.", "msSDELayerWhichShapes()");
    return(MS_FAILURE);
  }

  if(numparams < 2) {
    msSetError(MS_SDEERR, "Not enough SDE layer parameters specified.", "msSDELayerWhichShapes()");
    return(MS_FAILURE);
  }

  SE_layerinfo_create(NULL, &(sde->layerinfo));
  status = SE_layer_get_info(connection, params[0], params[1], sde->layerinfo);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_layer_get_info()");
    return(MS_FAILURE);
  }

  SE_coordref_create(&(sde->coordref));
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_coordref_create()");
    return(MS_FAILURE);
  }

  status = SE_layerinfo_get_coordref(sde->layerinfo, sde->coordref);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_layerinfo_get_coordref()");
    return(MS_FAILURE);
  }

  status = SE_layerinfo_get_envelope(sde->layerinfo, &envelope);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_layerinfo_get_envelope()");
    return(MS_FAILURE);
  }
  
  if(envelope.minx > rect.maxx) return(MS_DONE); // there is NO overlap, return MS_DONE (FIX: use this in ALL which shapes functions)
  if(envelope.maxx < rect.minx) return(MS_DONE);
  if(envelope.miny > rect.maxy) return(MS_DONE);
  if(envelope.maxy < rect.miny) return(MS_DONE);

  // set spatial constraint search shape
  envelope.minx = MS_MAX(rect.minx, envelope.minx); // crop against SDE layer extent *argh*
  envelope.miny = MS_MAX(rect.miny, envelope.miny);
  envelope.maxx = MS_MIN(rect.maxx, envelope.maxx);
  envelope.maxy = MS_MIN(rect.maxy, envelope.maxy);

  status = SE_shape_generate_rectangle(&envelope, shape);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_shape_generate_rectangle()");
    return(MS_FAILURE);
  }
  constraint.filter.shape = shape;

  // set spatial constraint column and table
  strcpy(constraint.table, param[0]);
  strcpy(constraint.column, param[1]);

  // set a couple of other spatial constraint properties
  constraint.method = SM_ENVP;
  constraint.filter_type = SE_SHAPE_FILTER;
  constraint.truth = TRUE;

  // set up the SQL statement
  status = SE_sql_construct_alloc((numparams-1), &sql);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_sql_construct_alloc()");
    return(-1);
  }

  strcpy(sql->tables[0], params[0]); // main table
  for(i=2; i<numparams; i++)
    strcpy(sql->tables[i-1], params[i]); // joined tables  

  numcolumns = layer->numitems + 2;
  columns = (char **)malloc(numcolumns*sizeof(char *));
  if(!columns) {
    msSetError(MS_MEMERR, "Error allocating columns array.", "msSDELayerWhichShapes()");
    return(MS_FAILURE);
  }

  column[0] = strdup(params[1]); // the shape  
  column[1] = strdup("SE_ROW_ID"); // row id
  for(i=0; i<layer->numitems; i++)
    column[i+2] = strdup(layer->items[i]); // any other items needed for labeling or classification

  // set the "where" clause
  if(!(layer->filter.string))
    sql->where = strdup("");
  else
    sql->where = strdup(layer->filter.string);

  status = SE_stream_create(sde->connection, &(sde->stream));
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_stream_create()");
    return(MS_FAILURE);
  }
    
  status = SE_stream_query(sde->stream, numcolumns, columns, sql);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_stream_query()");
    return(MS_FAILURE);
  }

  status = SE_stream_set_spatial_constraints(sde->stream, SE_SPATIAL_FIRST, FALSE, 1, &constraints);
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_stream_set_spatial_constraints()");
    return(MS_FAILURE);
  }

  status = SE_stream_execute(sde->stream); // *should* be ready to step through shapes now
  if(status != SE_SUCCESS) {
    sde_error(status, "msSDELayerWhichShapes()", "SE_stream_query()");
    return(MS_FAILURE);
  }

  // clean-up
  SE_shape_free(shape);
  SE_sql_construct_free(sql);
  msFreeCharArray(params, numparams);
  msFreeCharArray(columns, numcolumns);

  return(MS_SUCCESS);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msSDELayerWhichShapes()");
  return(MS_FAILURE);
#endif
}

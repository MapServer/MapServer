#include "map.h"
#include "maperror.h"

/* ESRI SDE Client Includes */
#ifdef USE_SDE
#include <sdetype.h>
#include <sdeerno.h>
#endif

#ifdef USE_SDE
static void sde_error(long error_code, char *routine, char *sde_routine) {
  char error_string[SE_MAX_MESSAGE_LENGTH];

  error_string[0] = '\0';
  SE_error_get_string(error_code, error_string);

  msSetError(MS_SDEERR, NULL, routine);
  sprintf(ms_error.message, "%s: %s (%ld)", sde_routine, error_string, error_code);

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

static int sdeTransformShape(rectObj extent, double cellsize, SE_SHAPE inshp, shapeObj *outshp) {
  long numparts, numpoints;
  long *parts=NULL;
  SE_POINT *points=NULL;
  long type, status;

  lineObj line={0,NULL};

  int i,j,k,l;
  
  SE_shape_get_type(inshp, &type);

  if(type == SG_NIL_SHAPE) return(0); // skip null shapes

  SE_shape_get_num_points(inshp, 0, 0, &numpoints);
  SE_shape_get_num_parts(inshp, &numparts, NULL);

  if(numparts > 0) {
    parts = (long *)malloc(numparts*sizeof(long));
    if(!parts) {
      msSetError(MS_MEMERR, "Unable to allocate parts array.", "sdeTransformShape()");
      return(-1);
    }
  }

  points = (SE_POINT *)malloc(numpoints*sizeof(SE_POINT));
  if(!parts) {
    msSetError(MS_MEMERR, "Unable to allocate parts array.", "sdeTransformShape()");
    return(-1);
  }

  status = SE_shape_get_all_points(inshp, SE_DEFAULT_ROTATION, parts, NULL, points, NULL, NULL);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeTransformShape()", "SE_shape_get_all_points()");
    return(-1);
  }

  l = 0;
  for(i=0; i<numparts; i++) {
    
    if( i == numparts-1)
      line.numpoints = numpoints - parts[i];
    else
      line.numpoints = parts[i+1] - parts[i];

    line.point = (pointObj *)malloc(sizeof(pointObj)*line.numpoints);
    if(!line.point) {
      msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", "sdeTransformShape()");
      return(-1);
    }
     
    line.point[0].x = MS_NINT((points[0].x - extent.minx)/cellsize);
    line.point[0].y = MS_NINT((extent.maxy - points[0].y)/cellsize);
 
    for(j=1, k=1; j < line.numpoints; j++ ) {
      
      line.point[k].x = MS_NINT((points[l].x - extent.minx)/cellsize); 
      line.point[k].y = MS_NINT((extent.maxy - points[l].y)/cellsize);
      
      if(k == 1) {
	if((line.point[0].x != line.point[1].x) || (line.point[0].y != line.point[1].y))
	  k++;
      } else {
	if((line.point[k-1].x != line.point[k].x) || (line.point[k-1].y != line.point[k].y)) {
	  if(((line.point[k-2].y - line.point[k-1].y)*(line.point[k-1].x - line.point[k].x)) == ((line.point[k-2].x - line.point[k-1].x)*(line.point[k-1].y - line.point[k].y))) {	    
	    line.point[k-1].x = line.point[k].x;
	    line.point[k-1].y = line.point[k].y;	
	  } else {
	    k++;
	  }
	}
      }

      l++;
    }

    line.numpoints = k; /* save actual number kept */
    msAddLine(outshp, &line);

    free(line.point);
  }

  return(0);
}
#endif

int msDrawSDELayer(mapObj *map, layerObj *layer, gdImagePtr img) {
#ifdef USE_SDE
  int i,j;

  SE_CONNECTION connection=0;
  SE_STREAM stream=0;
  SE_ERROR error;
  long status;

  SE_COORDREF coordref=NULL;
  SE_LAYERINFO layerinfo;

  SE_ENVELOPE rect, cliprect, shaperect;
  SE_SHAPE filtershape=0, shape=0, clipshape=0;
  short shape_is_null;

  SE_SQL_CONSTRUCT *sql;
  SE_FILTER filter;

  char *annotation=NULL;
  short annotation_is_null;

  char *columns[2]; // at most 2 - the shape, and optionally an annotation
  int numcolumns;
  
  char **params;
  int numparams;

  shapeObj shape2={0,NULL,{-1,-1,-1,-1},MS_NULL};
  pointObj annopnt;

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT))
    return(0);

  /* Set clipping rectangle (used by certain layer types only) */
  if(layer->transform && (layer->type == MS_POLYGON || layer->type == MS_POLYLINE)) {
    cliprect.minx = map->extent.minx - 2*map->cellsize; /* just a bit larger than the map extent */
    cliprect.miny = map->extent.miny - 2*map->cellsize;
    cliprect.maxx = map->extent.maxx + 2*map->cellsize;
    cliprect.maxy = map->extent.maxy + 2*map->cellsize;
  } else {
    cliprect.minx = map->extent.maxx;
    cliprect.maxx = map->extent.minx;
    cliprect.miny = map->extent.maxy;
    cliprect.maxy = map->extent.miny;
  }

  params = split(layer->connection, ',', &numparams);
  if(!params) {
    msSetError(MS_MEMERR, "Error spliting SDE connection information.", "msDrawSDELayer()");
    return(-1);
  }

  if(numparams < 5) {
    msSetError(MS_SDEERR, "Not enough SDE connection parameters specified.", "msDrawSDELayer()");
    return(-1);
  }

  status = SE_connection_create(params[0], params[1], params[2], params[3], params[4], &error, &connection);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_connection_create()");
    return(-1);
  }

  msFreeCharArray(params, numparams);

  status = SE_stream_create(connection, &stream);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_stream_create()");
    return(-1);
  }

  /*
  ** Get some basic information about the layer (error checking)
  */
  SE_layerinfo_create(NULL, &layerinfo);
  SE_coordref_create(&coordref);

  params = split(layer->data, ',', &numparams);
  if(!params) {
    msSetError(MS_MEMERR, "Error spliting SDE layer information.", "msDrawSDELayer()");
    return(-1);
  }

  if(numparams < 2) {
    msSetError(MS_SDEERR, "Not enough SDE layer parameters specified.", "msDrawSDELayer()");
    return(-1);
  }

  status = SE_layer_get_info(connection, params[0], params[1], layerinfo);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_layer_get_info()");
    return(-1);
  }

  status = SE_layerinfo_get_envelope(layerinfo, &rect);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_layerinfo_get_envelope()");
    return(-1);
  }
  
  if(rect.minx > map->extent.maxx) return(0); // msRectOverlap()
  if(rect.maxx < map->extent.minx) return(0);
  if(rect.miny > map->extent.maxy) return(0);
  if(rect.maxy < map->extent.miny) return(0);

  /*
  ** define the spatial filter, used with all classes
  */
  SE_layerinfo_get_coordref(layerinfo, coordref);
  SE_shape_create(coordref, &filtershape);

  rect.minx = MS_MAX(map->extent.minx, rect.minx);
  rect.miny = MS_MAX(map->extent.miny, rect.miny);
  rect.maxx = MS_MIN(map->extent.maxx, rect.maxx);
  rect.maxy = MS_MIN(map->extent.maxy, rect.maxy);

  status = SE_shape_generate_rectangle(&rect, filtershape);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_shape_generate_rectangle()");
    return(-1);
  }

  strcpy(filter.table, params[0]);
  strcpy(filter.column, params[1]);
  filter.filter.shape = filtershape;
  filter.method = SM_ENVP;
  filter.filter_type = SE_SHAPE_FILTER;

  status = SE_stream_set_spatial_constraints(stream, SE_SPATIAL_FIRST, FALSE, 1, &filter);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_stream_set_spatial_constraints()");
    return(-1);
  }

  /*
  ** define a portion of the SQL construct here, the where clause changes with each class
  */
  status = SE_sql_construct_alloc((numparams-1), &sql);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_sql_construct_alloc()");
    return(-1);
  }

  strcpy(sql->tables[0], params[0]);
  for(i=2; i<numparams; i++)
    strcpy(sql->tables[i-1], params[i]); // add any joined tables  
  
  /*
  ** set up column list
  */
  numcolumns = 1;
  columns[0] = strdup(params[1]);  

  if(layer->labelitem && layer->annotate) {
    numcolumns = 2;
    columns[1] = strdup(layer->labelitem);    
  }  

  msFreeCharArray(params, numparams);

  /*
  ** each class is a SQL statement, no expression means all features
  */
  for(i=0; i<layer->numclasses; i++) {
    
    if(!(layer->class[i].expression.string))
      sql->where = strdup("");
    else
      sql->where = strdup(layer->class[i].expression.string);

    status = SE_stream_query(stream, numcolumns, columns, sql);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_query()");
      return(-1);
    }

    status = SE_stream_bind_output_column(stream, 0, shape, &shape_is_null);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_bind_output_column()");
      return(-1);
    }

    if(numcolumns == 2) {
      status = SE_stream_bind_output_column(stream, 0, annotation, &annotation_is_null);
      if(status != SE_SUCCESS) {
	sde_error(status, "msDrawSDELayer()", "SE_stream_bind_output_column()");
	return(-1);
      }
    }

    status = SE_stream_execute(stream);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_query()");
      return(-1);
    }
    
    switch(layer->type) {
    case MS_ANNOTATION:
      break;
    case MS_POINT:
      break;
    case MS_LINE:
      break;
    case MS_POLYLINE:
      break;
    case MS_POLYGON:
      while(status == SE_SUCCESS) {
	status = SE_stream_fetch(stream);
	if(status != SE_SUCCESS) {
	  sde_error(status, "msDrawSDELayer()", "SE_stream_fetch()");
	  return(-1);
	}

	clipshape = shape;
	SE_shape_get_extent(shape, 0, &shaperect);
	if(!sdeRectContained(&shaperect, &cliprect)) {
	  if(!sdeRectOverlap(&shaperect, &cliprect)) continue;
	  SE_shape_clip(shape, &cliprect, clipshape);
	  if(SE_shape_is_nil(clipshape)) continue;
	}
	sdeTransformShape(map->extent, map->cellsize, clipshape, &shape2);

	msDrawShadeSymbol(&map->shadeset, img, &shape2, &(layer->class[i]));
	
	if(layer->annotate) {
	  if(msPolygonLabelPoint(&shape2, &annopnt, layer->class[i].label.minfeaturesize) != -1) {

	    if(!annotation)
	      annotation = layer->class[i].text.string;
	    
	    if(layer->labelcache)
	      msAddLabel(map, layer->index, i, -1, -1, annopnt, annotation, -1);
	    else
	      msDrawLabel(img, map, annopnt, annotation, &(layer->class[i].label));
	  }
	}

	SE_shape_free(shape);
	SE_shape_free(clipshape);
	msFreeShape(&shape2);

      }
      break;
    default:
      msSetError(MS_MISCERR, "Unknown or unsupported layer type.", "msDrawSDELayer()");
      return(-1);
    }

    free(sql->where);
  }

  SE_shape_free(filtershape);
  SE_stream_free(stream);
  SE_connection_free(connection);

  return(0);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msDrawSDELayer()");
  return(-1);
#endif
}

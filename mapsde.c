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
  sprintf(ms_error.message, "%s: %s. (%ld)", sde_routine, error_string, error_code);

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
  int i;

  double scalefactor=1;

  SE_CONNECTION connection=0;
  SE_STREAM stream=0;
  SE_ERROR error;
  long status;

  SE_COORDREF coordref=NULL;
  SE_LAYERINFO layerinfo;

  SE_ENVELOPE rect;
  SE_SHAPE filtershape=0, shape=0, clippedshape=0;
  short shape_is_null;

  SE_SQL_CONSTRUCT *sql;
  SE_FILTER filter;

  short annotate=MS_TRUE;
  char *annotation=NULL;
  short annotation_is_null;

  const char *columns[2]; // at most 2 - the shape, and optionally an annotation
  int numcolumns;
  
  char **params;
  int numparams;

  shapeObj transformedshape={0,NULL,{-1,-1,-1,-1},MS_NULL};
  pointObj annopnt;

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT))
    return(0);

  if(map->scale > 0) {
    if((layer->maxscale > 0) && (map->scale > layer->maxscale))
      return(0);
    if((layer->minscale > 0) && (map->scale <= layer->minscale))
      return(0);
    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
      annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
      annotate = MS_FALSE;
  }

  // apply scaling to symbols and fonts
  if(layer->symbolscale > 0) {
    scalefactor = layer->symbolscale/map->scale;
    for(i=0; i<layer->numclasses; i++) {
      layer->class[i].sizescaled = MS_NINT(layer->class[i].size * scalefactor);
      layer->class[i].sizescaled = MS_MAX(layer->class[i].sizescaled, layer->class[i].minsize);
      layer->class[i].sizescaled = MS_MIN(layer->class[i].sizescaled, layer->class[i].maxsize);
#ifdef USE_TTF
      if(layer->class[i].label.type == MS_TRUETYPE) { 
	layer->class[i].label.sizescaled = MS_NINT(layer->class[i].label.size * scalefactor);
	layer->class[i].label.sizescaled = MS_MAX(layer->class[i].label.sizescaled, layer->class[i].label.minsize);
	layer->class[i].label.sizescaled = MS_MIN(layer->class[i].label.sizescaled, layer->class[i].label.maxsize);
      }
#endif
    }
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

  fprintf(stderr, "SDE connection established...\n");

  msFreeCharArray(params, numparams);

  status = SE_stream_create(connection, &stream);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_stream_create()");
    return(-1);
  }

  fprintf(stderr, "SDE stream created...\n");

  /*
  ** Get some basic information about the layer (error checking)
  */
  SE_layerinfo_create(NULL, &layerinfo);

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

  SE_coordref_create(&coordref);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_coordref_create()");
    return(-1);
  }

  status = SE_layerinfo_get_coordref(layerinfo, coordref);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_layerinfo_get_coordref()");
    return(-1);
  }

  status = SE_layerinfo_get_envelope(layerinfo, &rect);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_layerinfo_get_envelope()");
    return(-1);
  }
  
  if(rect.minx > map->extent.maxx) return(0); // i.e. msRectOverlap()
  if(rect.maxx < map->extent.minx) return(0);
  if(rect.miny > map->extent.maxy) return(0);
  if(rect.maxy < map->extent.miny) return(0);

  fprintf(stderr, "layer envelope: %g %g %g %g\n", rect.minx, rect.miny, rect.maxx, rect.maxy);

  /*
  ** initialize a few shapes
  */
  status = SE_shape_create(coordref, &shape);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_shape_create()");
    return(-1);
  }

  status = SE_shape_create(coordref, &clippedshape);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_shape_create()");
    return(-1);
  }

  SE_shape_create(coordref, &filtershape);
  if(status != SE_SUCCESS) {
    sde_error(status, "msDrawSDELayer()", "SE_shape_create()");
    return(-1);
  }

  /* 
  ** define the spatial filter, used with all classes
  */ 
  if(layer->transform && (layer->type == MS_POLYGON || layer->type == MS_POLYLINE)) {
    rect.minx = MS_MAX(map->extent.minx - 2*map->cellsize, rect.minx); /* just a bit larger than the map extent */
    rect.miny = MS_MAX(map->extent.miny - 2*map->cellsize, rect.miny);
    rect.maxx = MS_MIN(map->extent.maxx + 2*map->cellsize, rect.maxx);
    rect.maxy = MS_MIN(map->extent.maxy + 2*map->cellsize, rect.maxy);
  } else {
    rect.minx = MS_MAX(map->extent.minx, rect.minx);
    rect.maxx = MS_MAX(map->extent.miny, rect.miny);
    rect.miny = MS_MIN(map->extent.maxx, rect.maxx);
    rect.maxy = MS_MIN(map->extent.maxy, rect.maxy);
  }

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
  filter.truth = TRUE;

  /* can't set the spatial constraints here, must wait till after SE_stream_query() is called */ 

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

  if(layer->labelitem && annotate) {
    numcolumns = 2;
    columns[1] = strdup(layer->labelitem);    
  }  

  msFreeCharArray(params, numparams);

  for(i=0; i<numcolumns; i++)
    fprintf(stderr, "%d:%s\n", i+1, columns[i]);

  /*
  ** each class is a SQL statement, no expression means all features
  */
  for(i=0; i<layer->numclasses; i++) {
    
    if(!(layer->class[i].expression.string))
      sql->where = strdup("");
    else
      sql->where = strdup(layer->class[i].expression.string);

    fprintf(stderr, "class %d where: %s\n", i, sql->where);

    status = SE_stream_query(stream, numcolumns, columns, sql);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_query()");
      return(-1);
    }

    fprintf(stderr, "attribute query set\n");

    status = SE_stream_set_spatial_constraints(stream, SE_SPATIAL_FIRST, FALSE, 1, &filter);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_set_spatial_constraints()");
      return(-1);
    }

    fprintf(stderr, "spatial query set\n");

    status = SE_stream_bind_output_column(stream, 1, shape, &shape_is_null);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_bind_output_column()");
      return(-1);
    }

    if(numcolumns == 2) {
      status = SE_stream_bind_output_column(stream, 2, annotation, &annotation_is_null);
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

    fprintf(stderr, "search executed...\n");
    
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
	if(status == SE_SUCCESS) {

	  if(SE_shape_is_nil(shape)) continue;

	  status = SE_shape_clip(shape, &rect, clippedshape);
	  if(status != SE_SUCCESS) {
	    sde_error(status, "msDrawSDELayer()", "SE_shape_clip()");
	    return(-1);
          }

	  if(SE_shape_is_nil(clippedshape)) continue;
	  sdeTransformShape(map->extent, map->cellsize, clippedshape, &transformedshape);
	  
	  msDrawShadeSymbol(&map->shadeset, img, &transformedshape, &(layer->class[i]));
	  
	  if(annotate) {
	    if(msPolygonLabelPoint(&transformedshape, &annopnt, layer->class[i].label.minfeaturesize) != -1) {
	      
	      if(!annotation)
		annotation = layer->class[i].text.string;
	      
	      if(layer->labelcache)
		msAddLabel(map, layer->index, i, -1, -1, annopnt, annotation, -1);
	      else
		msDrawLabel(img, map, annopnt, annotation, &(layer->class[i].label));
	    }
	  }
	  
	  msFreeShape(&transformedshape);
	} else {
	  if(status != SE_FINISHED) {
	    sde_error(status, "msDrawSDELayer()", "SE_stream_fetch()");
	    return(-1);
          }
	}
      }
      break;
    default:
      msSetError(MS_MISCERR, "Unknown or unsupported layer type.", "msDrawSDELayer()");
      return(-1);
    }
    
    free(sql->where);
  }

  SE_shape_free(shape);
  SE_shape_free(clippedshape);  
  SE_shape_free(filtershape);
  SE_stream_free(stream);
  SE_connection_free(connection);

  return(0);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msDrawSDELayer()");
  return(-1);
#endif
}

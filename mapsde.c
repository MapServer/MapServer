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

static int sdeTransformShapePoints(rectObj extent, double cellsize, SE_SHAPE inshp, shapeObj *outshp) 
{
  int i;
  SE_POINT *points=NULL;
  long type, status, numpoints;

  lineObj line={0,NULL};

  SE_shape_get_type(inshp, &type);

  if(type == SG_NIL_SHAPE) return(0); // skip null shapes

  SE_shape_get_num_points(inshp, 0, 0, &numpoints);

  points = (SE_POINT *)malloc(numpoints*sizeof(SE_POINT));
  if(!points) {
    msSetError(MS_MEMERR, "Unable to allocate points array.", "sdeTransformShape()");
    return(-1);
  }

  status = SE_shape_get_all_points(inshp, SE_DEFAULT_ROTATION, NULL, NULL, points, NULL, NULL);
  if(status != SE_SUCCESS) {
    sde_error(status, "sdeTransformShape()", "SE_shape_get_all_points()");
    return(-1);
  }

  line.point = (pointObj *)malloc(sizeof(pointObj)*numpoints);
  if(!line.point) {
    msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", "sdeTransformShape()");
    return(-1);
  }
   
  line.numpoints = 0;
  for(i=0; i<numpoints; i++) {
    if(points[i].x < extent.minx) continue;
    if(points[i].x > extent.maxx) continue;
    if(points[i].y < extent.miny) continue;
    if(points[i].y > extent.maxy) continue;
    
    line.point[line.numpoints].x = MS_NINT((points[i].x - extent.minx)/cellsize);
    line.point[line.numpoints].y = MS_NINT((extent.maxy - points[i].y)/cellsize);
    line.numpoints++;
  }

  msAddLine(outshp, &line);
  free(line.point);
  free(points);

  return(0);
}

static int sdeTransformShape(rectObj extent, double cellsize, SE_SHAPE inshp, shapeObj *outshp) {
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
      msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", "sdeTransformShape()");
      return(-1);
    }
     
    line.point[0].x = MS_NINT((points[l].x - extent.minx)/cellsize);
    line.point[0].y = MS_NINT((extent.maxy - points[l].y)/cellsize);
    l++;

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

  free(subparts);
  free(points);

  return(0);
}
#endif

int msDrawSDELayer(mapObj *map, layerObj *layer, gdImagePtr img) {
#ifdef USE_SDE
  int i,j;

  double scalefactor=1;
  double angle, length;

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

  SE_COLUMN_DEF annotation_def;
  short annotate=MS_TRUE;
  char *annotation=NULL;
  long annotation_long;
  double annotation_double;
  short annotation_is_null;

  const char *columns[2]; // at most 2 - the shape, and optionally an annotation
  int numcolumns;
  
  char **params;
  int numparams;

  shapeObj transformedshape={0,NULL,{-1,-1,-1,-1},MS_NULL};
  pointObj annopnt, *pnt;

  featureListNodeObjPtr shpcache=NULL, current=NULL;           

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

  if(layer->symbolscale > 0) scalefactor = layer->symbolscale/map->scale;

  for(i=0; i<layer->numclasses; i++) {
    layer->class[i].sizescaled = MS_NINT(layer->class[i].size * scalefactor);
    layer->class[i].sizescaled = MS_MAX(layer->class[i].sizescaled, layer->class[i].minsize);
    layer->class[i].sizescaled = MS_MIN(layer->class[i].sizescaled, layer->class[i].maxsize);
    layer->class[i].overlaysizescaled = MS_NINT(layer->class[i].overlaysize * scalefactor);
    layer->class[i].overlaysizescaled = MS_MAX(layer->class[i].overlaysizescaled, layer->class[i].overlayminsize);
    layer->class[i].overlaysizescaled = MS_MIN(layer->class[i].overlaysizescaled, layer->class[i].overlaymaxsize);
#ifdef USE_TTF
    if(layer->class[i].label.type == MS_TRUETYPE) { 
      layer->class[i].label.sizescaled = MS_NINT(layer->class[i].label.size * scalefactor);
      layer->class[i].label.sizescaled = MS_MAX(layer->class[i].label.sizescaled, layer->class[i].label.minsize);
      layer->class[i].label.sizescaled = MS_MIN(layer->class[i].label.sizescaled, layer->class[i].label.maxsize);
    }
#endif
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

  /*
  ** Get some basic information about the layer (error checking)
  */
  SE_layerinfo_create(NULL, &layerinfo);

  params = split(layer->data, '.', &numparams);
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
  rect.minx = MS_MAX(map->extent.minx - 2*map->cellsize, rect.minx); /* just a bit larger than the map extent */
  rect.miny = MS_MAX(map->extent.miny - 2*map->cellsize, rect.miny);
  rect.maxx = MS_MIN(map->extent.maxx + 2*map->cellsize, rect.maxx);
  rect.maxy = MS_MIN(map->extent.maxy + 2*map->cellsize, rect.maxy);

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

  /*
  ** each class is a SQL statement, no expression means all features
  */
  for(i=0; i<layer->numclasses; i++) {

    if(layer->class[c].sizescaled == 0) return;

    /*
    ** should be able to move this outside the loop, but the
    ** stream reset function doesn't want to work as advertised *sigh*
    */
    status = SE_stream_create(connection, &stream);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_create()");
      return(-1);
    }
    
    if(!(layer->class[i].expression.string))
      sql->where = strdup("");
    else
      sql->where = strdup(layer->class[i].expression.string);

    status = SE_stream_query(stream, numcolumns, columns, sql);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_query()");
      return(-1);
    }

    status = SE_stream_set_spatial_constraints(stream, SE_SPATIAL_FIRST, FALSE, 1, &filter);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_set_spatial_constraints()");
      return(-1);
    }

    status = SE_stream_bind_output_column(stream, 1, shape, &shape_is_null);
    if(status != SE_SUCCESS) {
      sde_error(status, "msDrawSDELayer()", "SE_stream_bind_output_column()");
      return(-1);
    }

    /*
    ** bind the annotation column, and allocate memory
    */
    if(numcolumns == 2) {
      status = SE_stream_describe_column(stream, 2, &annotation_def);
      if(status != SE_SUCCESS) {
    	sde_error(status, "msDrawSDELayer()", "SE_stream_describe_column()");
    	return(-1);
      }

      // don't know if this makes sense for non-string columns but it seems to work
      annotation = (char *)malloc(annotation_def.size);
      if(!annotation) {
	msSetError(MS_SDEERR, "Can only annotate with numeric or character types.", "msDrawSDELayer()");
	return(-1);
      }

      switch(annotation_def.sde_type) {
      case 1: // integers
      case 2:
	status = SE_stream_bind_output_column(stream, 2, &annotation_long, &annotation_is_null);
	break;
      case 3: // floats
      case 4:
	status = SE_stream_bind_output_column(stream, 2, &annotation_double, &annotation_is_null);
	break;
      case 5: // string
	status = SE_stream_bind_output_column(stream, 2, annotation, &annotation_is_null);
	break;
      default:
	msSetError(MS_SDEERR, "Can only annotate with numeric or character types.", "msDrawSDELayer()");
	return(-1);
	break;
      }
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
      msSetError(MS_SDEERR, "SDE annotation layers are not yet supported.", "msDrawSDELayer()");
      return(-1);
      break;
    case MS_POINT:
      while(status == SE_SUCCESS) {
	status = SE_stream_fetch(stream);
	if(status == SE_SUCCESS) {

	  if(SE_shape_is_nil(shape)) continue;
	  
	  if(sdeTransformShapePoints(map->extent, map->cellsize, shape, &transformedshape) == -1) continue;

	  for(j=0; j<transformedshape.line[0].numpoints; j++) {
	    pnt = &(transformedshape.line[0].point[j]); /* point to the correct point */

	    msDrawMarkerSymbol(&map->symbolset, img, &transformedshape, layer->class[i].symbol, layer->class[i].color, layer->class[i].backgroundcolor, layer->class[i].outlinecolor, layer->class[i].sizescaled);
	    if(layer->class[i].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, &transformedshape, layer->class[i].overlaysymbol, layer->class[i].overlaycolor, layer->class[i].overlaybackgroundcolor, layer->class[i].overlayoutlinecolor, layer->class[i].overlaysizescaled);

	    if(annotate) {	      
	      if(numcolumns == 2) {
		switch(annotation_def.sde_type) {
		case 1: // integers
		case 2:
		  sprintf(annotation, "%ld", annotation_long);
		  break;
		case 3: // floats
		case 4:
		  sprintf(annotation, "%g", annotation_double);
		  break;
		default:	     
		  break;
		}
	      } else {
		annotation = layer->class[i].text.string;
	      }

	      if(annotation) {
		if(layer->labelcache)
		  msAddLabel(map, layer->index, i, -1, -1, *pnt, annotation, -1);
		else
		  msDrawLabel(img, map, *pnt, annotation, &(layer->class[i].label));
	      }
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
    case MS_LINE:

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

	  msDrawLineSymbol(&map->symbolset, img, &transformedshape, layer->class[i].symbol, layer->class[i].color, layer->class[i].backgroundcolor, layer->class[i].outlinecolor, layer->class[i].sizescaled);

	  if(annotate) {
	    if(msPolylineLabelPoint(&transformedshape, &annopnt, layer->class[i].label.minfeaturesize, &angle, &length) != -1) {

	      if(layer->class[i].label.autoangle)
		layer->class[i].label.angle = angle;

	      if(numcolumns == 2) {
		switch(annotation_def.sde_type) {
		case 1: // integers
		case 2:
		  sprintf(annotation, "%ld", annotation_long);
		  break;
		case 3: // floats
		case 4:
		  sprintf(annotation, "%g", annotation_double);
		  break;
		default:	     
		  break;
		}
	      } else {
		annotation = layer->class[i].text.string;
	      }

	      if(annotation) {
		if(layer->labelcache)
		  msAddLabel(map, layer->index, i, -1, -1, annopnt, annotation, length);
		else
		  msDrawLabel(img, map, annopnt, annotation, &(layer->class[i].label));
	      }
	    }
	  }

	  if(layer->class[i].overlaysymbol >= 0) { // cache shape
	    transformedshape.classindex = i;
	    if(insertFeatureList(&shpcache, transformedshape) == NULL) return(-1);
	    msInitShape(&transformedshape);
	  } else
	    msFreeShape(&transformedshape);
	} else {
	  if(status != SE_FINISHED) {
	    sde_error(status, "msDrawSDELayer()", "SE_stream_fetch()");
	    return(-1);
          }
	}
      }

      if(shpcache) {	
	for(current=shpcache; current; current=current->next) {
	  i = current->shape.classindex;
	  msDrawLineSymbol(&map->symbolset, img, &current->shape, layer->class[i].overlaysymbol, layer->class[i].overlaycolor, layer->class[i].overlaybackgroundcolor, layer->class[i].overlayoutlinecolor, layer->class[i].overlaysizescaled);
	}
	freeFeatureList(shpcache);
      }

      break;
    case MS_POLYLINE:
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

	  msDrawLineSymbol(&map->symbolset, img, &transformedshape, layer->class[i].symbol, layer->class[i].color, layer->class[i].backgroundcolor, layer->class[i].outlinecolor, layer->class[i].sizescaled);

	  if(annotate) {
	    if(msPolygonLabelPoint(&transformedshape, &annopnt, layer->class[i].label.minfeaturesize) != -1) {
	      if(numcolumns == 2) {
		switch(annotation_def.sde_type) {
		case 1: // integers
		case 2:
		  sprintf(annotation, "%ld", annotation_long);
		  break;
		case 3: // floats
		case 4:
		  sprintf(annotation, "%g", annotation_double);
		  break;
		default:	     
		  break;
		}
	      } else {
		annotation = layer->class[i].text.string;
	      }

	      if(annotation) {
		if(layer->labelcache)
		  msAddLabel(map, layer->index, i, -1, -1, annopnt, annotation, -1);
		else
		  msDrawLabel(img, map, annopnt, annotation, &(layer->class[i].label));
	      }
	    }
	  }

 	  if(layer->class[i].overlaysymbol >= 0) { // cache shape
	    transformedshape.classindex = i;
	    if(insertFeatureList(&shpcache, transformedshape) == NULL) return(-1);
	    msInitShape(&transformedshape);
	  } else
	    msFreeShape(&transformedshape);
	} else {
	  if(status != SE_FINISHED) {
	    sde_error(status, "msDrawSDELayer()", "SE_stream_fetch()");
	    return(-1);
          }
	}
      }

      if(shpcache) {	
	for(current=shpcache; current; current=current->next) {
	  i = current->shape.classindex;
	  msDrawLineSymbol(&map->symbolset, img, &current->shape, layer->class[i].overlaysymbol, layer->class[i].overlaycolor, layer->class[i].overlaybackgroundcolor, layer->class[i].overlayoutlinecolor, layer->class[i].overlaysizescaled);
	}
	freeFeatureList(shpcache);
      }

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


	  msDrawShadeSymbol(&map->symbolset, img, &transformedshape, layer->class[i].symbol, layer->class[i].color, layer->class[i].backgroundcolor, layer->class[i].outlinecolor, layer->class[i].sizescaled);
	  if(layer->class[i].overlaysymbol >= 0) msDrawShadeSymbol(&map->symbolset, img, &transformedshape, layer->class[i].overlaysymbol, layer->class[i].overlaycolor, layer->class[i].overlaybackgroundcolor, layer->class[i].overlayoutlinecolor, layer->class[i].overlaysizescaled);
      
	  if(annotate) {
	    if(msPolygonLabelPoint(&transformedshape, &annopnt, layer->class[i].label.minfeaturesize) != -1) {
	      if(numcolumns == 2) {
		switch(annotation_def.sde_type) {
		case 1: // integers
		case 2:
		  sprintf(annotation, "%ld", annotation_long);
		  break;
		case 3: // floats
		case 4:
		  sprintf(annotation, "%g", annotation_double);
		  break;
		default:	     
		  break;
		}
	      } else {
		annotation = layer->class[i].text.string;
	      }

	      if(annotation) {
		if(layer->labelcache)
		  msAddLabel(map, layer->index, i, -1, -1, annopnt, annotation, -1);
		else
		  msDrawLabel(img, map, annopnt, annotation, &(layer->class[i].label));
	      }
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

    if(annotate && numcolumns == 2) free(annotation);
    free(sql->where);
    SE_stream_free(stream);   
  }

  SE_shape_free(shape);
  SE_shape_free(clippedshape);  
  SE_shape_free(filtershape);
  SE_connection_free(connection);

  for(i=0; i<numcolumns; i++) free(columns[i]);

  return(0);
#else
  msSetError(MS_MISCERR, "SDE support is not available.", "msDrawSDELayer()");
  return(-1);
#endif
}

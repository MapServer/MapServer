#include "map.h"

/*
** Iteminfo is a layer parameter that holds information necessary to retrieve an individual item for
** a particular source. It is an array built from a list of items. The type of iteminfo will vary by
** source. For shapefiles and OGR it is simply an array of integers where each value is an index for
** the item. For SDE it's a ESRI specific type that contains index and column type information. Two
** helper functions below initialize and free that structure member which is used locally by layer
** specific functions.
*/
static int layerInitItemInfo(layerObj *layer) {
  int status;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
  case(MS_TILED_SHAPEFILE):
    layer->iteminfo = (int *) msDBFGetItemIndexes(layer->shpfile.hDBF, layer->items, layer->numitems);
    if(!layer->iteminfo) return(MS_FAILURE);
    return(MS_SUCCESS);
    break;
  case(MS_INLINE):
    return(MS_SUCCESS); // inline shapes have no items
    break;
  case(MS_OGR):
    return msOGRLayerInitItemInfo(layer);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    status = msSDELayerInitItemInfo(layer);
    if(status != MS_SUCCESS) return(MS_FAILURE);
    return(MS_SUCCESS);
    break;
  default:
    break;
  }

  return(MS_FAILURE);
}

static void layerFreeItemInfo(layerObj *layer) 
{
  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
  case(MS_TILED_SHAPEFILE):
    if(layer->iteminfo) free(layer->iteminfo);
    layer->iteminfo = NULL;
    break;
  case(MS_INLINE):
    break;
  case(MS_OGR):
    msOGRLayerFreeItemInfo(layer);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    msSDELayerFreeItemInfo(layer);
    break;
  default:
    break;
  }

  return;
}

/*
** Does exactly what it implies, readies a layer for processing.
*/
int msLayerOpen(layerObj *layer, char *shapepath)
{
  if(layer->tileindex && layer->connectiontype == MS_SHAPEFILE)
    layer->connectiontype = MS_TILED_SHAPEFILE;

  // if(layer->tileindex && layer->connectiontype == MS_OGR)
  //   layer->connectiontype = MS_TILED_OGR;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    if(msSHPOpenFile(&(layer->shpfile), "rb", shapepath, layer->data) == -1) return(MS_FAILURE);
    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPOpenFile(layer, shapepath));
    break;
  case(MS_INLINE):
    layer->currentfeature = layer->features; // point to the begining of the feature list
    return(MS_SUCCESS);
    break;
  case(MS_OGR):
    return msOGRLayerOpen(layer);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return msSDELayerOpen(layer);
    break;
  default:
    break;
  }

  return(MS_FAILURE);
}

/*
** Performs a spatial, and optionally an attribute based feature search. The function basically
** prepares things so that candidate features can be accessed by query or drawing functions. For
** OGR and shapefiles this sets an internal bit vector that indicates whether a particular feature
** is to processed. For SDE it executes an SQL statement on the SDE server. Once run the msLayerNextShape
** function should be called to actually access the shapes.
*/
int msLayerWhichShapes(layerObj *layer, rectObj rect)
{
  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    return(msSHPWhichShapes(&(layer->shpfile), rect));
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPWhichShapes(layer, rect));
    break;
  case(MS_INLINE):
    return(MS_SUCCESS);
    break;
  case(MS_OGR):
    return msOGRLayerWhichShapes(layer, rect);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return(msSDELayerWhichShapes(layer, rect));
    break;
  default:
    break;
  }

  return(MS_FAILURE);
}

/*
** Called after msWhichShapes has been called to actually retrieve shapes within a given area
** and matching a vendor specific filter (i.e. layer FILTER attribute).
*/
int msLayerNextShape(layerObj *layer, shapeObj *shape) 
{
  int i, filter_passed;
  char **values=NULL;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    do {
      i = layer->shpfile.lastshape + 1;
      while(i<layer->shpfile.numshapes && !msGetBit(layer->shpfile.status,i)) i++; // next "in" shape
      layer->shpfile.lastshape = i;

      if(i == layer->shpfile.numshapes) return(MS_DONE); // nothing else to read

      filter_passed = MS_TRUE;  // By default accept ANY shape
      if(layer->numitems > 0 && layer->iteminfo) {
        values = msDBFGetValueList(layer->shpfile.hDBF, i, layer->iteminfo, layer->numitems);
        if(!values) return(MS_FAILURE);
        if ((filter_passed = msEvalExpression(&(layer->filter), layer->filteritemindex, values, layer->numitems)) != MS_TRUE) {
            msFreeCharArray(values, layer->numitems);
            values = NULL;
        }
      }
    } while(!filter_passed);  // Loop until both spatial and attribute filters match

    msSHPReadShape(layer->shpfile.hSHP, i, shape); // ok to read the data now
    shape->values = values;
    shape->numvalues = layer->numitems;
    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPNextShape(layer, shape));
  case(MS_INLINE):
    if(!(layer->currentfeature)) return(MS_DONE); // out of features    
    msCopyShape(&(layer->currentfeature->shape), shape);
    layer->currentfeature = layer->currentfeature->next;
    return(MS_SUCCESS);
    break;
  case(MS_OGR):
    return msOGRLayerNextShape(layer, shape);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return(msSDELayerNextShape(layer, shape));
    break;
  default:
    break;
  }

  return(MS_FAILURE);
}

/*
** Used to retrieve a shape by index. All data sources must be capable of random access using
** a record number of some sort.
*/
int msLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record)
{
  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    msSHPReadShape(layer->shpfile.hSHP, record, shape);
    if(layer->numitems > 0 && layer->iteminfo) {
      shape->numvalues = layer->numitems;
      shape->values = msDBFGetValueList(layer->shpfile.hDBF, record, layer->iteminfo, layer->numitems);
      if(!shape->values) return(MS_FAILURE);
    }
    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPGetShape(layer, shape, tile, record));
  case(MS_INLINE):
    msSetError(MS_MISCERR, "Cannot retrieve inline shapes randomly.", "msLayerGetShape()");
    return(MS_FAILURE);
    break;
  case(MS_OGR):
    return msOGRLayerGetShape(layer, shape, tile, record);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return(msSDELayerGetShape(layer, shape, record));
    break;
  default:
    break;
  }

  return(MS_FAILURE);
}

/*
** Closes resources used by a particular layer.
*/
void msLayerClose(layerObj *layer) 
{
  // no need for items once the layer is closed
  layerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }  

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    msSHPCloseFile(&(layer->shpfile));
    break;
  case(MS_TILED_SHAPEFILE):
    msTiledSHPClose(layer);
    break;
  case(MS_INLINE):
    break;
  case(MS_OGR):
    msOGRLayerClose(layer);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    msSDELayerClose(layer);
    break;
  default:
    break;
  }
}

/*
** Retrieves a list of attributes available for this layer. Most sources also set the iteminfo array
** at this point. This function is used when processing query results to expose attributes to query
** templates. At that point all attributes are fair game.
*/
int msLayerGetItems(layerObj *layer) 
{
  // clean up any previously allocated instances
  layerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
  case(MS_TILED_SHAPEFILE):    
    layer->numitems = msDBFGetFieldCount(layer->shpfile.hDBF);
    layer->items = msDBFGetItems(layer->shpfile.hDBF);    
    if(!layer->items) return(MS_FAILURE);
    layerInitItemInfo(layer);
    return(MS_SUCCESS);
    break;
  case(MS_INLINE):
    return(MS_SUCCESS); // inline shapes have no items
    break;
  case(MS_OGR):
    return(msOGRLayerGetItems(layer));
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return(msSDELayerGetItems(layer));
    break;
  default:
    break;
  }

  return(MS_FAILURE);
}

/*
** Returns extent of spatial coverage for a layer. Used for WMS compatability.
*/
int msLayerGetExtent(layerObj *layer, rectObj *extent) {
  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    *extent = layer->shpfile.bounds;
    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):
    *extent = layer->tileshpfile.bounds;
    return(MS_SUCCESS);
    break;
  case(MS_INLINE):
    /* __TODO__ need to compute extents */
    return(MS_FAILURE);
    break;
  case(MS_OGR):
    return(msOGRLayerGetExtent(layer, extent));
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return(msSDELayerGetExtent(layer, extent));
    break;
  default:
    break;
  }

  return(MS_FAILURE);
}

static int string2list(char **list, int *listsize, char *string)
{
  int i;

  for(i=0; i<(*listsize); i++)
    if(strcmp(list[i], string) == 0) return(i);

  list[i] = strdup(string);
  (*listsize)++;

  return(i);
}

static void expression2list(char **list, int *listsize, expressionObj *expression) 
{
  int i, j, l;
  char tmpstr1[1024], tmpstr2[1024];
  short in=MS_FALSE;

  j = 0;
  l = strlen(expression->string);
  for(i=0; i<l; i++) {
    if(expression->string[i] == '[') {
      in = MS_TRUE;
      tmpstr2[j] = expression->string[i];
      j++;
      continue;
    }
    if(expression->string[i] == ']') {
      in = MS_FALSE;

      tmpstr2[j] = expression->string[i];
      tmpstr2[j+1] = '\0';
      string2list(expression->items, &(expression->numitems), tmpstr2);

      tmpstr1[j-1] = '\0';
      expression->indexes[expression->numitems - 1] = string2list(list, listsize, tmpstr1);

      j = 0; // reset

      continue;
    }

    if(in) {
      tmpstr2[j] = expression->string[i];
      tmpstr1[j-1] = expression->string[i];
      j++;
    }
  }
}

/*
** This function builds a list of items necessary to draw or query a particular layer by
** examining the contents of the various xxxxitem parameters and expressions. That list is
** then used to set the iteminfo variable. SDE is kinda special, we always need to retrieve
** the shape column and a virtual record number column. Most sources should not have to 
** modify this function.
*/
int msLayerWhichItems(layerObj *layer, int classify, int annotate) 
{
  int i, nt=0, ne=0;

  // Cleanup any previous item selection
  layerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }

  if(classify && layer->classitem) nt++;
  if(classify && layer->filteritem) nt++;
  if(annotate && layer->labelitem) nt++;
  if(annotate && layer->labelsizeitem) nt++;
  if(annotate && layer->labelangleitem) nt++;

  ne = 0;
  if(classify && layer->filter.type == MS_EXPRESSION) { 
    ne = countChars(layer->filter.string, '[');
    if(ne > 0) {
      layer->filter.items = (char **)calloc(ne, sizeof(char *)); // should be more than enough space
      if(!(layer->filter.items)) {
	msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
	return(MS_FAILURE);
      }
      layer->filter.indexes = (int *)malloc(ne*sizeof(int));
      if(!(layer->filter.indexes)) {
	msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
	return(MS_FAILURE);
      }
      layer->filter.numitems = 0;
      nt += ne;
    }
  }

  for(i=0; i<layer->numclasses; i++) {
    ne = 0;
    if(classify && layer->class[i].expression.type == MS_EXPRESSION) { 
      ne = countChars(layer->class[i].expression.string, '[');
      if(ne > 0) {
	layer->class[i].expression.items = (char **)calloc(ne, sizeof(char *)); // should be more than enough space
	if(!(layer->class[i].expression.items)) {
	  msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
	  return(MS_FAILURE);
	}
	layer->class[i].expression.indexes = (int *)malloc(ne*sizeof(int));
	if(!(layer->class[i].expression.indexes)) {
	  msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
	  return(MS_FAILURE);
	}
	layer->class[i].expression.numitems = 0;
	nt += ne;
      }
    }

    ne = 0;
    if(annotate && layer->class[i].text.type == MS_EXPRESSION) { 
      ne = countChars(layer->class[i].text.string, '[');
      if(ne > 0) {
	layer->class[i].text.items = (char **)calloc(ne, sizeof(char *)); // should be more than enough space
	if(!(layer->class[i].text.items)) {
	  msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
	  return(MS_FAILURE);
	}
	layer->class[i].text.indexes = (int *)malloc(ne*sizeof(int));
	if(!(layer->class[i].text.indexes)) {
	  msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
	  return(MS_FAILURE);
	}
	layer->class[i].text.numitems = 0;
	nt += ne;
      }
    }
  }

  if(layer->connectiontype == MS_SDE) {
    layer->items = (char **)calloc(nt+2, sizeof(char *)); // should be more than enough space, SDE always needs a couple of additional items
    if(!layer->items) {
      msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
      return(MS_FAILURE);
    }
    layer->items[0] = msSDELayerGetRowIDColumn(); // row id
    layer->items[1] = msSDELayerGetSpatialColumn(layer);
    layer->numitems = 2;
  } else {
    if(nt == 0) return(MS_SUCCESS);
    layer->items = (char **)calloc(nt, sizeof(char *)); // should be more than enough space
    if(!layer->items) {
      msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
      return(MS_FAILURE);
    }
    layer->numitems = 0;
  }

  if(classify && layer->classitem) layer->classitemindex = string2list(layer->items, &(layer->numitems), layer->classitem);
  if(classify && layer->filteritem) layer->filteritemindex = string2list(layer->items, &(layer->numitems), layer->filteritem);
  if(annotate && layer->labelitem) layer->labelitemindex = string2list(layer->items, &(layer->numitems), layer->labelitem);
  if(annotate && layer->labelsizeitem) layer->labelsizeitemindex = string2list(layer->items, &(layer->numitems), layer->labelsizeitem);
  if(annotate && layer->labelangleitem) layer->labelangleitemindex = string2list(layer->items, &(layer->numitems), layer->labelangleitem);
  
  if(classify && layer->filter.type == MS_EXPRESSION) expression2list(layer->items, &(layer->numitems), &(layer->filter));

  for(i=0; i<layer->numclasses; i++) {
    if(classify && layer->class[i].expression.type == MS_EXPRESSION) expression2list(layer->items, &(layer->numitems), &(layer->class[i].expression));
    if(annotate && layer->class[i].text.type == MS_EXPRESSION) expression2list(layer->items, &(layer->numitems), &(layer->class[i].text));
  }

  // populate the iteminfo array
  return(layerInitItemInfo(layer));

  fprintf(stderr, "In msLayerWhichItems().\n");

  return(MS_SUCCESS);
}

/*
** A helper function to set the items to be retrieved with a particular shape. Unused at the moment but will be used
** from within MapScript. Should not need modification.
*/
int msLayerSetItems(layerObj *layer, char **items, int numitems)
{
  int i;

  // Cleanup any previous item selection
  layerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }

  // now allocate and set the layer item parameters 
  layer->items = (char **)malloc(sizeof(char *)*numitems);
  if(!layer->items) {
    msSetError(MS_MEMERR, NULL, "msLayerSetItems()");
    return(MS_FAILURE);
  }

  for(i=0; i<numitems; i++)
    layer->items[i] = strdup(items[i]);
  layer->numitems = numitems;

  // populate the iteminfo array
  return(layerInitItemInfo(layer));

  return(MS_SUCCESS);
}


/*
** Fills a classObj with style info from the specified shape.  This is used
** with STYLEITEM AUTO when rendering shapes.
** For optimal results, this should be called immediately after 
** GetNextShape() or GetShape() so that the shape doesn't have to be read
** twice.
** 
*/
int msLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c,
                        int tile, long record)
{
  switch(layer->connectiontype) {
  case(MS_OGR):
    return(msOGRLayerGetAutoStyle(map, layer, c, tile, record));
    break;
  case(MS_SHAPEFILE):
  case(MS_TILED_SHAPEFILE):
  case(MS_INLINE):
  case(MS_TILED_OGR):
  case(MS_SDE):
  default:
    msSetError(MS_MISCERR, "'STYLEITEM AUTO' not supported for this data source.", 
               "msLayerGetAutoStyle()");
    return(MS_FAILURE);    
    break;
  }

  return(MS_FAILURE);
}

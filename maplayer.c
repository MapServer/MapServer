#include "map.h"

int msLayerOpen(layerObj *layer, char *shapepath)
{
  if(layer->tileindex && layer->connectiontype == MS_SHAPEFILE)
    layer->connectiontype = MS_TILED_SHAPEFILE;

  // if(layer->tileindex && layer->connectiontype == MS_OGR)
  //   layer->connectiontype = MS_TILED_OGR;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    if(msSHPOpenFile(&(layer->shpfile), "rb", shapepath, layer->data) == -1) return(MS_FAILURE);
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPOpenFile(layer, shapepath));
    break;
  case(MS_INLINE):
    layer->currentfeature = layer->features; // point to the begining of the feature list
    break;
  case(MS_OGR):
    return msOGRLayerOpen(layer, shapepath);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return msSDELayerOpen(layer);
    break;
  default:
    break;
  }

  return(MS_SUCCESS);
}

int msLayerNextShape(layerObj *layer, char *shapepath, shapeObj *shape) 
{
  int i;
  char **values=NULL;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    i = layer->shpfile.lastshape + 1;
    while(i<layer->shpfile.numshapes && !msGetBit(layer->shpfile.status,i)) i++; // next "in" shape
    layer->shpfile.lastshape = i;

    if(i == layer->shpfile.numshapes) return(MS_DONE); // nothing else to read

    if(layer->numitems > 0) {
      values = msDBFGetValueList(layer->shpfile.hDBF, i, layer->items, &(layer->itemindexes), layer->numitems);
      if(!values) return(MS_FAILURE);

      if(msEvalExpression(&(layer->filter), layer->filteritemindex, values, layer->numitems) != MS_TRUE) return(msLayerNextShape(layer, shapepath, shape));
    }

    msSHPReadShape(layer->shpfile.hSHP, i, shape); // ok to read the data now
    shape->values = values;
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPNextShape(layer, shapepath, shape));
  case(MS_INLINE):
    if(!(layer->currentfeature)) return(MS_DONE); // out of features    
    msCopyShape(&(layer->currentfeature->shape), shape);
    layer->currentfeature = layer->currentfeature->next;
    break;
  case(MS_OGR):
    return msOGRLayerNextShape(layer, shapepath, shape);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return(msSDELayerNextShape(layer, shape));
    break;
  default:
    break;
  }

  return(MS_SUCCESS);
}

int msLayerGetShape(layerObj *layer, char *shapepath, shapeObj *shape, int tile, long record, int allitems)
{
  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    msSHPReadShape(layer->shpfile.hSHP, record, shape);

    if(allitems == MS_TRUE) {
      if(!layer->items) { // fill the items layer variable if not already filled
	layer->numitems = msDBFGetFieldCount(layer->shpfile.hDBF);
	layer->items = msDBFGetItems(layer->shpfile.hDBF);
	if(!layer->items) return(MS_FAILURE);
      }
      shape->numvalues = layer->numitems;
      shape->values = msDBFGetValues(layer->shpfile.hDBF, record);      
      if(!shape->values) return(MS_FAILURE);      
    } else if(layer->numitems > 0) {
      shape->numvalues = layer->numitems;
      shape->values = msDBFGetValueList(layer->shpfile.hDBF, record, layer->items, &(layer->itemindexes), layer->numitems);
      if(!shape->values) return(MS_FAILURE);
    }

    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPGetShape(layer, shapepath, shape, tile, record, allitems));
  case(MS_INLINE):
    msSetError(MS_MISCERR, "Cannot retrieve inline shapes randomly.", "msLayerGetShape()");
    return(MS_FAILURE);
    break;
  case(MS_OGR):
    return msOGRLayerGetShape(layer, shapepath, shape, tile,
                              record, allitems);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return(msSDELayerGetShape(layer, shape, record));
    break;
  default:
    break;
  }

  return(MS_SUCCESS);
}

void msLayerClose(layerObj *layer) 
{
  // no need for items once the layer is closed
  if(layer->numitems > 0) {
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

int msLayerGetItems(layerObj *layer, char ***items, int *numitems) {

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
  case(MS_TILED_SHAPEFILE):    
    *numitems = msDBFGetFieldCount(layer->shpfile.hDBF);
    *items = msDBFGetItems(layer->shpfile.hDBF);
    if(!(*items)) return(MS_FAILURE);
    break;
  case(MS_INLINE):
    return(MS_SUCCESS); // inline shapes have no items
    break;
  case(MS_OGR):
    return(msOGRLayerGetItems(layer, items, numitems));
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return(msSDELayerGetItems(layer, items, numitems));
    break;
  default:
    break;
  }

  return(MS_FAILURE);
}

int msLayerWhichShapes(layerObj *layer, char *shapepath, rectObj rect)
{
  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    return(msSHPWhichShapes(&(layer->shpfile), rect));
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPWhichShapes(layer, shapepath, rect));
    break;
  case(MS_INLINE):
    break;
  case(MS_OGR):
    return msOGRLayerWhichShapes(layer, shapepath, rect);
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    return(msSDELayerWhichShapes(layer, rect));
    break;
  default:
    break;
  }

  return(MS_SUCCESS);
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

static void expression2list(char **list, int *listsize, expressionObj *expression) {
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

int msLayerWhichItems(layerObj *layer, int classify, int annotate) 
{
  int i, nt=0, ne=0;

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

  if(nt == 0) return(MS_SUCCESS);

  layer->items = (char **)calloc(nt, sizeof(char *)); // should be more than enough space
  if(!layer->items) {
    msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
    return(MS_FAILURE);
  }
  layer->numitems = 0;

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

  return(MS_SUCCESS);
}

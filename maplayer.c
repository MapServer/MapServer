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
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    break;
  default:
    break;
  }

  return(MS_SUCCESS);
}

int msLayerNextShape(layerObj *layer, char *shapepath, shapeObj *shape, int attributes) 
{
  int i;

  fprintf(stderr, "next shape...\n");
    
  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    i = layer->shpfile.lastshape + 1;
    while(i<layer->shpfile.numshapes && !msGetBit(layer->shpfile.status,i)) i++; // next "in" shape

    if(i == layer->shpfile.numshapes) return(MS_DONE); // nothing else to read

    msSHPReadShape(layer->shpfile.hSHP, i, shape);
    layer->shpfile.lastshape = i;

    if(attributes == MS_TRUE && layer->numitems > 0) {
      shape->attributes = msDBFGetValueList(layer->shpfile.hDBF, i, layer->items, &(layer->itemindexes), layer->numitems);
      if(!shape->attributes) return(MS_FAILURE);
    } else if(attributes == MS_ALLITEMS) {
      if(!layer->items) { // fill the items layer variable if not already filled
	layer->numitems = msDBFGetFieldCount(layer->shpfile.hDBF);
	layer->items = msDBFGetItems(layer->shpfile.hDBF);
	if(!layer->items) return(MS_FAILURE);
      }
      shape->attributes = msDBFGetValues(layer->shpfile.hDBF, i);
      if(!shape->attributes) return(MS_FAILURE);
    }
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPNextShape(layer, shapepath, shape, attributes));
  case(MS_INLINE):
    if(!(layer->currentfeature)) return(MS_DONE); // out of features    
    msCopyShape(&(layer->currentfeature->shape), shape);
    layer->currentfeature = layer->currentfeature->next;
    break;
  case(MS_OGR):
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    break;
  default:
    break;
  }

  return(MS_SUCCESS);
}

int msLayerGetShape(layerObj *layer, char *shapepath, shapeObj *shape, int tile, int record, int attributes)
{
 switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    msSHPReadShape(layer->shpfile.hSHP, record, shape);

    if(attributes == MS_ALLITEMS) {
      if(!layer->items) { // fill the items layer variable if not already filled
	layer->numitems = msDBFGetFieldCount(layer->shpfile.hDBF);
	layer->items = msDBFGetItems(layer->shpfile.hDBF);
	if(!layer->items) return(MS_FAILURE);
      }
      shape->attributes = msDBFGetValues(layer->shpfile.hDBF, record);
      if(!shape->attributes) return(MS_FAILURE);
    } else if(attributes == MS_TRUE && layer->numitems > 0) {
      shape->attributes = msDBFGetValueList(layer->shpfile.hDBF, record, layer->items, &(layer->itemindexes), layer->numitems);
      if(!shape->attributes) return(MS_FAILURE);
    }
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPGetShape(layer, shapepath, shape, tile, record, attributes));
  case(MS_INLINE):
    msSetError(MS_MISCERR, "Cannot retrieve inline shapes randomly.", "msLayerGetShape()");
    return(MS_FAILURE);
    break;
  case(MS_OGR):
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    break;
  default:
    break;
  }

  return(MS_SUCCESS);
}

void msLayerClose(layerObj *layer) 
{
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
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    break;
  default:
    break;
  }
}

int msLayerWhichShapes(layerObj *layer, char *shapepath, rectObj rect, projectionObj *proj)
{
  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    layer->shpfile.status = msSHPWhichShapes(&(layer->shpfile), rect, &(layer->projection), proj);
    break;
  case(MS_TILED_SHAPEFILE):
    msTiledSHPWhichShapes(layer, shapepath, rect, proj);
    break;
  case(MS_INLINE):
    break;
  case(MS_OGR):
    break;
  case(MS_TILED_OGR):
    break;
  case(MS_SDE):
    break;
  default:
    break;
  }

  return(0);
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

static void expression2list(char **list, int *listsize, char *expression) {
  int i, j, l;
  char tmpstr[1024];
  short in=MS_FALSE;

  j = 0;
  l = strlen(expression);
  for(i=0; i<l; i++) {
    if(expression[i] == '[') {
      in = MS_TRUE;
      continue;
    }
    if(expression[i] == ']') {
      in = MS_FALSE;
      tmpstr[j] = '\0';
      string2list(list, listsize, tmpstr);
      j = 0; // reset
      continue;
    }

    if(in) {
      tmpstr[j] = expression[i];
      j++;
    }
  }
}

/*
** FIX: NEED TO ADD CODE FOR EACH LOGICAL/TEXT EXPRESSION ISOLATING THE INDEX LIST FOR EACH EXPRESSION!
*/
int msLayerWhichItems(layerObj *layer, int annotate) 
{
  int i, n=0;

  if(layer->classitem) n++;
  if(annotate && layer->labelitem) n++;
  if(annotate && layer->labelsizeitem) n++;
  if(annotate && layer->labelangleitem) n++;

  for(i=0; i<layer->numclasses; i++) {
    if(layer->class[i].expression.type == MS_EXPRESSION) n += countChars(layer->class[i].expression.string, '[');
    if(annotate && layer->class[i].text.type == MS_EXPRESSION) n += countChars(layer->class[i].text.string, '[');
  }

  if(n ==0) return(MS_SUCCESS);

  fprintf(stderr, "in msLayerWhichItems() [%d]...\n", n);

  layer->items = (char **)calloc(n, sizeof(char *)); // should be more than enough space
  if(!layer->items) {
    msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
    return(MS_FAILURE);
  }

  layer->numitems = 0;

  if(layer->classitem) layer->classitemindex = string2list(layer->items, &(layer->numitems), layer->classitem);
  if(annotate && layer->labelitem) layer->labelitemindex = string2list(layer->items, &(layer->numitems), layer->labelitem);
  if(annotate && layer->labelsizeitem) layer->labelitemindex = string2list(layer->items, &(layer->numitems), layer->labelsizeitem);
  if(annotate && layer->labelangleitem) layer->labelitemindex = string2list(layer->items, &(layer->numitems), layer->labelangleitem);
 
  for(i=0; i<layer->numclasses; i++) {
    if(layer->class[i].expression.type == MS_EXPRESSION) expression2list(layer->items, &(layer->numitems), layer->class[i].expression.string);
    if(annotate && layer->class[i].text.type == MS_EXPRESSION) expression2list(layer->items, &(layer->numitems), layer->class[i].text.string);
  }

  fprintf(stderr, "itemlist: ");
  for(i=0; i<layer->numitems; i++)
    fprintf(stderr, "%s ", layer->items[i]);
  fprintf(stderr, "\n");

  return(MS_SUCCESS);
}

#include "map.h"

/*
** Iteminfo is a layer parameter that holds information necessary to retrieve an individual item for
** a particular source. It is an array built from a list of items. The type of iteminfo will vary by
** source. For shapefiles and OGR it is simply an array of integers where each value is an index for
** the item. For SDE it's a ESRI specific type that contains index and column type information. Two
** helper functions below initialize and free that structure member which is used locally by layer
** specific functions.
*/
static int layerInitItemInfo(layerObj *layer) 
{
  shapefileObj *shpfile=NULL;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):    
    shpfile = layer->layerinfo;

    if(!shpfile) {
      msSetError(MS_SDEERR, "Shapefile layer has not been opened.", "layerInitItemInfo()");
      return(MS_FAILURE);
    }

    // iteminfo needs to be a bit more complex, a list of indexes plus the length of the list
    layer->iteminfo = (int *) msDBFGetItemIndexes(shpfile->hDBF, layer->items, layer->numitems);
    if(!layer->iteminfo) return(MS_FAILURE);
    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):
    // iteminfo needs to be a bit more complex, a list of indexes plus the length of the list
    return(msTiledSHPLayerInitItemInfo(layer));
    break;
  case(MS_INLINE):
    return(MS_SUCCESS); // inline shapes have no items
    break;
  case(MS_OGR):
    return(msOGRLayerInitItemInfo(layer));
    break;
  case(MS_WFS):
    return(msWFSLayerInitItemInfo(layer));
    break;
  case(MS_POSTGIS):
    return(msPOSTGISLayerInitItemInfo(layer));
    break;
  case(MS_MYGIS):
    return(msMYGISLayerInitItemInfo(layer));
    break;
  case(MS_SDE):
    return(msSDELayerInitItemInfo(layer));
    break;
  case(MS_ORACLESPATIAL):
    return(msOracleSpatialLayerInitItemInfo(layer));
    break;
  case(MS_GRATICULE):
    return(msGraticuleLayerInitItemInfo(layer));
    break;
  case(MS_RASTER):
    return(msRASTERLayerInitItemInfo(layer));
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
  case(MS_WFS):
    msOGRLayerFreeItemInfo(layer);
    break;
  case(MS_POSTGIS):
    msPOSTGISLayerFreeItemInfo(layer);
    break;
  case(MS_MYGIS):
    msMYGISLayerFreeItemInfo(layer);
    break;
  case(MS_SDE):
    msSDELayerFreeItemInfo(layer);
    break;
  case(MS_ORACLESPATIAL):
    msOracleSpatialLayerFreeItemInfo(layer);
    break;
  case(MS_GRATICULE):
    msGraticuleLayerFreeItemInfo(layer);
    break;
  case(MS_RASTER):
    msRASTERLayerFreeItemInfo(layer);
    break;
  default:
    break;
  }

  return;
}

/*
** Does exactly what it implies, readies a layer for processing.
*/
int msLayerOpen(layerObj *layer)
{
  char szPath[MS_MAXPATHLEN];
  shapefileObj *shpfile;

  if(layer->features && layer->connectiontype != MS_GRATICULE ) 
    layer->connectiontype = MS_INLINE;

  if(layer->tileindex && layer->connectiontype == MS_SHAPEFILE)
    layer->connectiontype = MS_TILED_SHAPEFILE;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    if(layer->layerinfo) return(MS_SUCCESS); // layer already open

    // allocate space for a shapefileObj using layer->layerinfo	
    shpfile = (shapefileObj *) malloc(sizeof(shapefileObj));
    if(!shpfile) {
      msSetError(MS_MEMERR, "Error allocating shapefileObj structure.", "msLayerOpen()");
      return(MS_FAILURE);
    }

    layer->layerinfo = shpfile;

    if(msSHPOpenFile(shpfile, "rb", msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, layer->data)) == -1) 
      if(msSHPOpenFile(shpfile, "rb", msBuildPath(szPath, layer->map->mappath, layer->data)) == -1)
        return(MS_FAILURE);
   
    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPOpenFile(layer));
    break;
  case(MS_INLINE):
    layer->currentfeature = layer->features; // point to the begining of the feature list
    return(MS_SUCCESS);
    break;
  case(MS_OGR):
    return(msOGRLayerOpen(layer,NULL));
    break;
  case(MS_WFS):
    return(msWFSLayerOpen(layer, NULL, NULL));
    break;
  case(MS_POSTGIS):
    return(msPOSTGISLayerOpen(layer));
    break;
  case(MS_MYGIS):
    return(msMYGISLayerOpen(layer));
    break;
  case(MS_SDE):
    return(msSDELayerOpen(layer));
    break;
  case(MS_ORACLESPATIAL):
    return(msOracleSpatialLayerOpen(layer));
    break;
  case(MS_GRATICULE):
    return(msGraticuleLayerOpen(layer));
    break;
  case(MS_RASTER):
    return(msRASTERLayerOpen(layer));
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
**
** Note that for shapefiles we apply any maxfeatures constraint at this point. That may be the only
** connection type where this is feasible.
*/
int msLayerWhichShapes(layerObj *layer, rectObj rect)
{
  int i, n1=0, n2=0;
  int status;
  shapefileObj *shpfile;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):    
    shpfile = layer->layerinfo;

    if(!shpfile) {
      msSetError(MS_SDEERR, "Shapefile layer has not been opened.", "msLayerWhichShapes()");
      return(MS_FAILURE);
    }

    status = msSHPWhichShapes(shpfile, rect, layer->debug);
    if(status != MS_SUCCESS) return(status);

    // now apply the maxshapes criteria (NOTE: this ignores the filter so you could get less than maxfeatures)
    if(layer->maxfeatures > 0) {
      for(i=0; i<shpfile->numshapes; i++)
        n1 += msGetBit(shpfile->status,i);
    
      if(n1 > layer->maxfeatures) {
        for(i=0; i<shpfile->numshapes; i++) {
          if(msGetBit(shpfile->status,i) && (n2 < (n1 - layer->maxfeatures))) {
	    msSetBit(shpfile->status,i,0);
	    n2++;
          }
        }
      }
    }

    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPWhichShapes(layer, rect));
    break;
  case(MS_INLINE):
    return(MS_SUCCESS);
    break;
  case(MS_OGR):
    return(msOGRLayerWhichShapes(layer, rect));
    break;
  case(MS_WFS):
    return(msWFSLayerWhichShapes(layer, rect));
    break;
  case(MS_POSTGIS):
    return(msPOSTGISLayerWhichShapes(layer, rect));
    break;
  case(MS_MYGIS):
    return(msMYGISLayerWhichShapes(layer, rect));
    break;
  case(MS_SDE):
    return(msSDELayerWhichShapes(layer, rect));
    break;
  case(MS_ORACLESPATIAL):
    return(msOracleSpatialLayerWhichShapes(layer, rect));
    break;
  case(MS_GRATICULE):
    return(msGraticuleLayerWhichShapes(layer, rect));
    break;
  case(MS_RASTER):
    return(msRASTERLayerWhichShapes(layer, rect));
    break;
  default:
    break;
  }

  return(MS_FAILURE);
}

/*
** Called after msWhichShapes has been called to actually retrieve shapes within a given area
** and matching a vendor specific filter (i.e. layer FILTER attribute).
**
** Shapefiles: NULL shapes (shapes with attributes but NO vertices are skipped)
*/
int msLayerNextShape(layerObj *layer, shapeObj *shape) 
{
  int i, filter_passed;
  char **values=NULL;
  shapefileObj *shpfile;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):    
    shpfile = layer->layerinfo;

    if(!shpfile) {
      msSetError(MS_SDEERR, "Shapefile layer has not been opened.", "msLayerNextShape()");
      return(MS_FAILURE);
    }

    do {
      i = shpfile->lastshape + 1;
      while(i<shpfile->numshapes && !msGetBit(shpfile->status,i)) i++; // next "in" shape
      shpfile->lastshape = i;

      if(i == shpfile->numshapes) return(MS_DONE); // nothing else to read

      filter_passed = MS_TRUE;  // By default accept ANY shape
      if(layer->numitems > 0 && layer->iteminfo) {
        values = msDBFGetValueList(shpfile->hDBF, i, layer->iteminfo, layer->numitems);
        if(!values) return(MS_FAILURE);
        if ((filter_passed = msEvalExpression(&(layer->filter), layer->filteritemindex, values, layer->numitems)) != MS_TRUE) {
            msFreeCharArray(values, layer->numitems);
            values = NULL;
        }
      }
    } while(!filter_passed);  // Loop until both spatial and attribute filters match

    msSHPReadShape(shpfile->hSHP, i, shape); // ok to read the data now

    // skip NULL shapes (apparently valid for shapefiles, at least ArcView doesn't care)
    if(shape->type == MS_SHAPE_NULL) return(msLayerNextShape(layer, shape));

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
  case(MS_WFS):
    return(msOGRLayerNextShape(layer, shape));
    break;
  case(MS_POSTGIS):
    return(msPOSTGISLayerNextShape(layer, shape));
    break;
  case(MS_MYGIS):
    return(msMYGISLayerNextShape(layer, shape));
    break;
  case(MS_SDE):
    return(msSDELayerNextShape(layer, shape));
    break;
  case(MS_ORACLESPATIAL):
    return(msOracleSpatialLayerNextShape(layer, shape));
    break;
  case(MS_GRATICULE):
    return(msGraticuleLayerNextShape(layer, shape));
    break;
  case(MS_RASTER):
    return(msRASTERLayerNextShape(layer, shape));
    break;
  default:
    break;
  }

  // TO DO! This is where dynamic joins will happen. Joined attributes will be
  // tagged on to the main attributes with the naming scheme [join name].[item name].
  // We need to leverage the iteminfo (I think) at this point

  return(MS_FAILURE);
}

/*
** Used to retrieve a shape by index. All data sources must be capable of random access using
** a record number of some sort.
*/
int msLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record)
{
  shapefileObj *shpfile;

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):    
    shpfile = layer->layerinfo;

    if(!shpfile) {
      msSetError(MS_SDEERR, "Shapefile layer has not been opened.", "msLayerGetShape()");
      return(MS_FAILURE);
    }

    // msSHPReadShape *should* return success or failure so we don't have to test here
    if(record < 0 || record >= shpfile->numshapes) {
      msSetError(MS_MISCERR, "Invalid feature id.", "msLayerGetShape()");
      return(MS_FAILURE);
    }

    msSHPReadShape(shpfile->hSHP, record, shape);
    if(layer->numitems > 0 && layer->iteminfo) {
      shape->numvalues = layer->numitems;
      shape->values = msDBFGetValueList(shpfile->hDBF, record, layer->iteminfo, layer->numitems);
      if(!shape->values) return(MS_FAILURE);
    }
    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPGetShape(layer, shape, tile, record));
  case(MS_INLINE):
    return msINLINELayerGetShape(layer, shape, record);
    //msSetError(MS_MISCERR, "Cannot retrieve inline shapes randomly.", "msLayerGetShape()");
    //return(MS_FAILURE);
    break;
  case(MS_OGR):
  case(MS_WFS):
    return(msOGRLayerGetShape(layer, shape, tile, record));
    break;
  case(MS_POSTGIS):
    return(msPOSTGISLayerGetShape(layer, shape, record));
    break;
  case(MS_MYGIS):
    return(msMYGISLayerGetShape(layer, shape, record));
    break;
  case(MS_SDE):
    return(msSDELayerGetShape(layer, shape, record));
    break;
  case(MS_ORACLESPATIAL):
    return(msOracleSpatialLayerGetShape(layer, shape, record));
    break;
  case(MS_GRATICULE):
    return(msGraticuleLayerGetShape(layer, shape, tile, record));
    break;
  case(MS_RASTER):
    return(msRASTERLayerGetShape(layer, shape, tile, record));
    break;
  default:
    break;
  }

  // TO DO! This is where dynamic joins will happen. Joined attributes will be
  // tagged on to the main attributes with the naming scheme [join name].[item name].

  return(MS_FAILURE);
}

/*
** Closes resources used by a particular layer.
*/
void msLayerClose(layerObj *layer) 
{
  shapefileObj *shpfile;

  // no need for items once the layer is closed
  layerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    shpfile = layer->layerinfo;
    if(!shpfile) return; // nothing to do
    msSHPCloseFile(shpfile);
    free(layer->layerinfo);
    layer->layerinfo = NULL;
    break;
  case(MS_TILED_SHAPEFILE):
    msTiledSHPClose(layer);
    break;
  case(MS_INLINE):
    break;
  case(MS_OGR):
    msOGRLayerClose(layer);
    break;
  case(MS_WFS):
    msWFSLayerClose(layer);
    break;
  case(MS_POSTGIS):
    msPOSTGISLayerClose(layer);
    break;
  case(MS_MYGIS):
    msMYGISLayerClose(layer);
    break;
  case(MS_SDE):
    // using pooled connections for SDE, closed when map file is closed
    // msSDELayerClose(layer); 
    break;
  case(MS_ORACLESPATIAL):
    msOracleSpatialLayerClose(layer);
    break;
  case(MS_GRATICULE):
    msGraticuleLayerClose(layer);
    break;
  case(MS_RASTER):
    msRASTERLayerClose(layer);
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
  shapefileObj *shpfile;

  // clean up any previously allocated instances
  layerFreeItemInfo(layer);
  if(layer->items) {
    msFreeCharArray(layer->items, layer->numitems);
    layer->items = NULL;
    layer->numitems = 0;
  }

  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    shpfile = layer->layerinfo;

    if(!shpfile) {
      msSetError(MS_SDEERR, "Shapefile layer has not been opened.", "msLayerGetItems()");
      return(MS_FAILURE);
    }

    layer->numitems = msDBFGetFieldCount(shpfile->hDBF);
    layer->items = msDBFGetItems(shpfile->hDBF);    
    if(!layer->items) return(MS_FAILURE);
    layerInitItemInfo(layer);
    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):    
    return(msTiledSHPLayerGetItems(layer));
    break;
  case(MS_INLINE):
    return(MS_SUCCESS); // inline shapes have no items
    break;
  case(MS_OGR):
    return(msOGRLayerGetItems(layer));
    break;
  case(MS_WFS):
    return(msWFSLayerGetItems(layer));
    break;
  case(MS_POSTGIS):
    return(msPOSTGISLayerGetItems(layer));
    break;
  case(MS_MYGIS):
    return(msMYGISLayerGetItems(layer));
    break;
  case(MS_SDE):
    return(msSDELayerGetItems(layer));
    break;
  case(MS_ORACLESPATIAL):
    return(msOracleSpatialLayerGetItems(layer));
    break;
  case(MS_GRATICULE):
    return(msGraticuleLayerGetItems(layer));
    break;
  case(MS_RASTER):
    return(msRASTERLayerGetItems(layer));
    break;
  default:
    break;
  }

  // TO DO! Need to add any joined itemd on to the core layer items, one long list! 

  return(MS_FAILURE);
}

/*
** Returns extent of spatial coverage for a layer. Used for WMS compatability.
*/
int msLayerGetExtent(layerObj *layer, rectObj *extent) 
{
  switch(layer->connectiontype) {
  case(MS_SHAPEFILE):
    *extent = ((shapefileObj*)layer->layerinfo)->bounds;
    return(MS_SUCCESS);
    break;
  case(MS_TILED_SHAPEFILE):
    return(msTiledSHPLayerGetExtent(layer, extent));
    break;
  case(MS_INLINE):
    /* __TODO__ need to compute extents */
    return(MS_FAILURE);
    break;
  case(MS_OGR):
  case(MS_WFS):
    return(msOGRLayerGetExtent(layer, extent));
    break;
  case(MS_POSTGIS):
    return(msPOSTGISLayerGetExtent(layer, extent));
    break;
  case(MS_MYGIS):
    return(msMYGISLayerGetExtent(layer, extent));
    break;
  case(MS_SDE):
    return(msSDELayerGetExtent(layer, extent));
    break;
  case(MS_ORACLESPATIAL):
    return(msOracleSpatialLayerGetExtent(layer, extent));
    break;
  case(MS_GRATICULE):
    return(msGraticuleLayerGetExtent(layer, extent));
    break;
  case(MS_RASTER):
    return(msRASTERLayerGetExtent(layer, extent));
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
    if(strcmp(list[i], string) == 0) { 
        // printf("string2list (duplicate): %s %d\n", string, i);
      return(i);
    }

  list[i] = strdup(string);
  (*listsize)++;

  // printf("string2list: %s %d\n", string, i);

  return(i);
}

// TO DO: this function really needs to use the lexer
static void expression2list(char **list, int *listsize, expressionObj *expression) 
{
  int i, j, l;
  char tmpstr1[1024], tmpstr2[1024];
  short in=MS_FALSE;
  int tmpint;

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

      tmpint = expression->numitems;

      tmpstr2[j] = expression->string[i];
      tmpstr2[j+1] = '\0';
      string2list(expression->items, &(expression->numitems), tmpstr2);

      if(tmpint != expression->numitems) { // not a duplicate, so no need to calculate the index
        tmpstr1[j-1] = '\0';
        expression->indexes[expression->numitems - 1] = string2list(list, listsize, tmpstr1);
      }

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

int msLayerWhichItemsNew(layerObj *layer, int classify, int annotate, char *metadata) 
{
  int status;
  int numchars;
//  int i;

  status = msLayerGetItems(layer); // get a list of all attributes available for this layer (including JOINs)
  if(status != MS_SUCCESS) return(status);

  // allocate space for the various item lists
  if(classify && layer->filter.type == MS_EXPRESSION) { 
    numchars = countChars(layer->filter.string, '[');
    if(numchars > 0) {
      layer->filter.items = (char **)calloc(numchars, sizeof(char *)); // should be more than enough space
      if(!(layer->filter.items)) {
	msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
	return(MS_FAILURE);
      }
      layer->filter.indexes = (int *)malloc(numchars*sizeof(int));
      if(!(layer->filter.indexes)) {
	msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
	return(MS_FAILURE);
      }
      layer->filter.numitems = 0;     
    }
  }

  // for(i=0; i<layer->numitems; i++) { }

  return(MS_SUCCESS);
}

/*
** This function builds a list of items necessary to draw or query a particular layer by
** examining the contents of the various xxxxitem parameters and expressions. That list is
** then used to set the iteminfo variable. SDE is kinda special, we always need to retrieve
** the shape column and a virtual record number column. Most sources should not have to 
** modify this function.
*/
int msLayerWhichItems(layerObj *layer, int classify, int annotate, char *metadata) 
{
  int i, nt=0, ne=0;
  
  //
  // TO DO! I have a better algorithm.
  // 
  // 1) call msLayerGetItems to get a complete list (including joins)
  // 2) loop though item-based parameters and expressions to identify items to keep
  // 3) based on 2) build a list of items
  //
  // This is more straight forward and robust, fixes the problem of [...] regular expressions
  // embeded in logical expressions. It also opens up to using dynamic joins anywhere...
  //


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
    layer->items[0] = msSDELayerGetRowIDColumn(layer); // row id
    layer->items[1] = msSDELayerGetSpatialColumn(layer);
    layer->numitems = 2;
  } else {
    //if(nt == 0) return(MS_SUCCESS);
    if(nt > 0) {
      layer->items = (char **)calloc(nt, sizeof(char *)); // should be more than enough space
      if(!layer->items) {
        msSetError(MS_MEMERR, NULL, "msLayerWhichItems()");
        return(MS_FAILURE);
      }
      layer->numitems = 0;
    }
  }

  if(nt > 0) {
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
  }

  if(metadata) {
    char **tokens;
    int n = 0;
    int j;
    int bFound = 0;
      
    tokens = split(metadata, ',', &n);
    if(tokens) {
      for(i=0; i<n; i++) {
        bFound = 0;
        for(j=0; j<layer->numitems; j++) {
          if(strcmp(tokens[i], layer->items[j]) == 0) {
            bFound = 1;
            break;
          }
        }
              
        if(!bFound) {
          layer->numitems++;
          layer->items =  (char **)realloc(layer->items, sizeof(char *)*(layer->numitems));
          layer->items[layer->numitems-1] = strdup(tokens[i]);
        }
      }
      msFreeCharArray(tokens, n);
    }
  }

  // populate the iteminfo array
  if (layer->numitems == 0)
       return(MS_SUCCESS);

  return(layerInitItemInfo(layer));
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
int msLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c, int tile, long record)
{
  switch(layer->connectiontype) {
  case(MS_OGR):
    return(msOGRLayerGetAutoStyle(map, layer, c, tile, record));
    break;
  case(MS_SHAPEFILE):
  case(MS_TILED_SHAPEFILE):
  case(MS_INLINE):
  case(MS_POSTGIS):
  case(MS_MYGIS):
  case(MS_SDE):
  case(MS_ORACLESPATIAL):
  case(MS_WFS):
  case(MS_GRATICULE):
  case(MS_RASTER):
  default:
    msSetError(MS_MISCERR, "'STYLEITEM AUTO' not supported for this data source.", "msLayerGetAutoStyle()");
    return(MS_FAILURE);    
    break;
  }

  return(MS_FAILURE);
}

/* Author: Cristoph Spoerri and Sean Gillies */
int msINLINELayerGetShape(layerObj *layer, shapeObj *shape, int shapeindex) {
    int i=0;
    featureListNodeObjPtr current;

    current = layer->features;
    while (current!=NULL && i!=shapeindex) {
        i++;
        current = current->next;
    }
    if (current == NULL) {
        msSetError(MS_SHPERR, "No inline feature with this index.",
                   "msINLINELayerGetShape()");
        return MS_FAILURE;
    } 
    
    if (msCopyShape(&(current->shape), shape) != MS_SUCCESS) {
        msSetError(MS_SHPERR, "Cannot retrieve inline shape. There some problem with the shape", "msLayerGetShape()");
        return MS_FAILURE;
    }
    return MS_SUCCESS;
}

/*
Returns the number of inline feature of a layer
*/
int msLayerGetNumFeatures(layerObj *layer) {
    int i = 0;
    featureListNodeObjPtr current;
    if (layer->connectiontype==MS_INLINE) {
        current = layer->features;
        while (current!=NULL) {
            i++;
            current = current->next;
        }
    }
    else {
        msSetError(MS_SHPERR, "Not an inline layer", "msLayerGetNumFeatures()");
        return MS_FAILURE;
    }
    return i;
}

void msLayerAddProcessing( layerObj *layer, const char *directive )

{
  int i, len;
  
  len = strcspn(directive, "="); // we only want to compare characters up to the equal sign
  for(i=0; i<layer->numprocessing; i++) { // check to see if option is already set
    if(strncasecmp(directive, layer->processing[i], len) == 0) {
	  free(layer->processing[i]);
	  layer->processing[i] = strdup(directive);
      return;
    }
  }

  layer->numprocessing++;
  if( layer->numprocessing == 1 )
    layer->processing = (char **) malloc(2*sizeof(char *));
  else
    layer->processing = (char **) realloc(layer->processing, sizeof(char*) * (layer->numprocessing+1) );
  layer->processing[layer->numprocessing-1] = strdup(directive);
  layer->processing[layer->numprocessing] = NULL;
}

char *msLayerGetProcessing( layerObj *layer, int proc_index) {
    if (proc_index < 0 || proc_index >= layer->numprocessing) {
        msSetError(MS_CHILDERR, "Invalid processing index.", "msLayerGetProcessing()");
        return NULL;
    }
    else {
        return layer->processing[proc_index];
    }
}

int msLayerClearProcessing( layerObj *layer ) {
    if (layer->numprocessing > 0) {
        msFreeCharArray( layer->processing, layer->numprocessing );
        layer->processing = NULL;
        layer->numprocessing = 0;
    }
    return layer->numprocessing;
}


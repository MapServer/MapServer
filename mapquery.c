#include "map.h"
#include "mapparser.h"

/*
** Match this with with unit enumerations is map.h
*/
static double inchesPerUnit[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};

/*
** This function fills the items and data buffers for a given join (record based).
** Now handles one-to-many joins correctly. Cleans up previous join before allocating
** space for the new one. 
*/ 
int msJoinDBFTables(joinObj *join, char *path, char *tile) {
  int i, j, idx;
  DBFHandle hDBF;
  int nrecs, *ids=NULL;

  char old_path[MS_PATH_LENGTH];

  getcwd(old_path, MS_PATH_LENGTH); /* save old working directory */
  if(path) chdir(path);
  if(tile) chdir(tile);

  /* first open the lookup table file */
  if((hDBF = DBFOpen(join->table, "rb")) == NULL) {
    sprintf(ms_error.message, "(%s)", join->table);
    msSetError(MS_IOERR, ms_error.message, "msJoinDBFTables()");
    chdir(old_path); /* restore old cwd */
    return(-1);
  }

  if((idx = msGetItemIndex(hDBF, join->to)) == -1) { 
    sprintf(ms_error.message, "Item %s not found.", join->to);   
    msSetError(MS_DBFERR, ms_error.message, "msJoinDBFTables()");    
    DBFClose(hDBF);
    chdir(old_path); /* restore old cwd */
    return(-1);
  }

  /*
  ** Now we need to pull the record and item names in to the join buffers
  */
  join->numitems =  DBFGetFieldCount(hDBF);
  if(!join->items) {
    join->items = msGetDBFItems(hDBF);
    if(!join->items) {
      chdir(old_path); /* restore old cwd */
      return(-1);
    }
  }

  nrecs = DBFGetRecordCount(hDBF);

  if(join->type == MS_SINGLE) { /* only one row */
    
    if((join->data = (char ***)malloc(sizeof(char **))) == NULL) {
      msSetError(MS_MEMERR, NULL, "msJoinDBFTables()");
      chdir(old_path); /* restore old cwd */
      return(-1);
    }
    
    for(i=0; i<nrecs; i++) { /* find a match */
      if(strcmp(join->match, DBFReadStringAttribute(hDBF, i, idx)) == 0)
	break;
    }  
    
    if(i == nrecs) { /* just return zero length strings */
      if((join->data[0] = (char **)malloc(sizeof(char *)*join->numitems)) == NULL) {
	msSetError(MS_MEMERR, NULL, "msJoinDBFTables()");
	chdir(old_path); /* restore old cwd */
	return(-1);
      }
      for(i=0; i<join->numitems; i++)
	join->data[0][i] = strdup("\0"); /* intialize to zero length strings */
    } else {
      if((join->data[0] = msGetDBFValues(hDBF,i)) == NULL) {
	chdir(old_path); /* restore old cwd */
	return(-1);
      }
    }

  } else {

    if(join->data) { /* free old data */
      for(i=0; i<join->numrecords; i++)
	msFreeCharArray(join->data[i], join->numitems);
      free(join->data);
      join->numrecords = 0;
    }

    ids = (int *)malloc(sizeof(int)*nrecs);
    if(!ids) {
      msSetError(MS_MEMERR, NULL, "msJoinDBFTables()");
      chdir(old_path); /* restore old cwd */
      return(-1);
    }
    
    j=0;
    for(i=0; i<nrecs; i++) { /* find the matches, save ids */
      if(strcmp(join->match, DBFReadStringAttribute(hDBF, i, idx)) == 0) {
	ids[j] = i;
	j++;
      }
    }
  
    join->numrecords = j;

    if(join->numrecords > 0) { /* save em */
      if((join->data = (char ***)malloc(sizeof(char **)*join->numrecords)) == NULL) {
	msSetError(MS_MEMERR, NULL, "msJoinDBFTables()");
	free(ids);
	chdir(old_path); /* restore old cwd */
	return(-1);
      }

      for(i=0; i<join->numrecords; i++) {
	join->data[i] = (char **)malloc(sizeof(char *)*join->numitems);
	if(!join->data[i]) {
	  msSetError(MS_MEMERR, NULL, "msJoinDBFTables()");
	  free(ids);
	  chdir(old_path); /* restore old cwd */
	  return(-1);
	}

	join->data[i] = msGetDBFValues(hDBF,ids[i]);
	if(!join->data[i]) {
	  free(ids);
	  chdir(old_path); /* restore old cwd */
	  return(-1);
	}
      }      
    }

    free(ids);
  }

  chdir(old_path); /* restore old cwd */
  DBFClose(hDBF);
  return(0);
}

extern int msyyresult; // result of parsing, true/false
extern int msyystate;
extern char *msyystring;
extern int msyyparse();

static int shpGetQueryIndex(DBFHandle hDBF, layerObj *layer, int record, int item)
{
  int i,j;
  int numitems=0;
  char *tmpstr=NULL, **values=NULL;
  int found=MS_FALSE;

  if((layer->numqueries == 1) && !(layer->query[0].expression.string)) /* no need to do lookup */
    return(0);

  for(i=0; i<layer->numqueries; i++) {
    switch(layer->query[i].expression.type) {
    case(MS_STRING):
      if(strcmp(layer->query[i].expression.string, DBFReadStringAttribute(hDBF, record, item)) == 0) /* got a match */
	found=MS_TRUE;
      break;
    case(MS_EXPRESSION):

      tmpstr = strdup(layer->query[i].expression.string);

      if(!values) {
	numitems = DBFGetFieldCount(hDBF);
	values = msGetDBFValues(hDBF, record);
      }

      if(!(layer->query[i].expression.items)) { // build cache of item replacements	
	char **items=NULL, substr[19];

	numitems = DBFGetFieldCount(hDBF);
	items = msGetDBFItems(hDBF);
	
	layer->query[i].expression.items = (char **)malloc(numitems);
	layer->query[i].expression.indexes = (int *)malloc(numitems);

	for(j=0; j<numitems; j++) {
	  sprintf(substr, "[%s]", items[j]);
	  if(strstr(tmpstr, substr) != NULL) {
	    layer->query[i].expression.indexes[layer->query[i].expression.numitems] = j;
	    layer->query[i].expression.items[layer->query[i].expression.numitems] = strdup(substr);
	    layer->query[i].expression.numitems++;
	  }
	}

	msFreeCharArray(items,numitems);
      }

      for(j=0; j<layer->query[i].expression.numitems; j++)
	tmpstr = gsub(tmpstr, layer->query[i].expression.items[j], values[(int)layer->query[i].expression.indexes[j]]);

      msyystate = 4; msyystring = tmpstr;
      if(msyyparse() != 0)
	return(-1);

      free(tmpstr);

      if(msyyresult) /* got a match */
	found=MS_TRUE;
      break;
    case(MS_REGEX):
      if(regexec(&(layer->query[i].expression.regex), DBFReadStringAttribute(hDBF, record, item), 0, NULL, 0) == 0) /* got a match */
	found=MS_TRUE;
      break;
    }

    if(found) {
      if(values) msFreeCharArray(values,numitems);
      return(i);
    }
  }

  return(-1); /* not found */
}

/*
** Allocates memory for query result set
*/
static queryResultObj *newResults(int numlayers)
{
  queryResultObj *results;
  int i;

  results = (queryResultObj *)malloc(sizeof(queryResultObj));
  if(!results) {
    msSetError(MS_MEMERR, NULL, "newResults()"); 
    return(NULL);
  }

  results->layers = (layerResultObj *)malloc(sizeof(layerResultObj)*numlayers);
  if(!results->layers) {
    msSetError(MS_MEMERR, NULL, "newResults()");
    return(NULL);
  }

  results->numlayers = numlayers;
  results->numresults = 0;
  results->numquerylayers = 0;
  results->currentlayer = results->currenttile =  results->currentshape = 0;
  
  for(i=0; i<numlayers; i++) { /* initialize a few things */
    results->layers[i].status = NULL;
    results->layers[i].index = NULL;
    results->layers[i].numresults = 0;
    results->layers[i].numshapes = 0;
  }

  return(results);
}

static int initLayerResult(layerResultObj *layer, int numshapes) {
  layer->numshapes = numshapes;

  layer->status = msAllocBitArray(layer->numshapes);
  layer->index = (char *)calloc(layer->numshapes, sizeof(char));

  if(!layer->status || !layer->index) {
    msSetError(MS_MEMERR, NULL, "initLayerResult()");
    return(-1);
  }
  
  return(0);
}

/*
** Frees memory for a query result set
*/
void msFreeQueryResults(queryResultObj *results)
{
  int i;

  if(!results) return;

  for(i=0; i<results->numlayers; i++) {
    free(results->layers[i].status);
    free(results->layers[i].index);    
  }
  free(results->layers);
  free(results);
}

int msSaveQuery(queryResultObj *results, char *filename) {
  FILE *stream;
  int i;

  if(!filename) {
    msSetError(MS_MISCERR, "No filename provided to save query to.", "msSaveQuery()");
    return(-1);
  }

  stream = fopen(filename, "wb");
  if(!stream) {
    sprintf(ms_error.message, "(%s)", filename);
    msSetError(MS_IOERR, ms_error.message, "msSaveQuery()");
    return(-1);
  }

  fwrite(&results->numlayers, sizeof(int), 1, stream);

  for(i=0; i<results->numlayers; i++) {
    if(results->layers[i].status) {
      fwrite(&i, sizeof(int), 1, stream);
      fwrite(&results->layers[i].numshapes, sizeof(int), 1, stream);
      fwrite(&results->layers[i].numresults, sizeof(int), 1, stream);
      if(results->layers[i].numresults > 0)
	fwrite(results->layers[i].status, msGetBitArraySize(results->layers[i].numshapes), 1, stream);
    }
  }

  fclose(stream);
  return(0);
}

queryResultObj *msLoadQuery(char *filename) {
  FILE *stream;
  queryResultObj *results=NULL;
  int i;

  if(!filename) {
    msSetError(MS_MISCERR, "No filename provided to load query from.", "msLoadQuery()");
    return(NULL);
  }

  stream = fopen(filename, "rb");
  if(!stream) {
    sprintf(ms_error.message, "(%s)", filename);
    msSetError(MS_IOERR, ms_error.message, "msLoadQuery()");    
    return(NULL);
  }

  fread(&i, sizeof(int), 1, stream);

  results = newResults(i);
  if(!results) {
    msSetError(MS_IOERR, NULL, "msLoadQuery()");
    return(NULL);
  }

  while(fread(&i, sizeof(int), 1, stream)) {
    fread(&results->layers[i].numshapes, sizeof(int), 1, stream);
    fread(&results->layers[i].numresults, sizeof(int), 1, stream);

    results->layers[i].status = msAllocBitArray(results->layers[i].numshapes);
    if(!results->layers[i].status) {
      msSetError(MS_MEMERR, NULL, "msLoadQuery()");
      msFreeQueryResults(results);
      return(NULL);
    }
    if(results->layers[i].numresults > 0)
      fread(results->layers[i].status, msGetBitArraySize(results->layers[i].numshapes), 1, stream);
    
    results->numquerylayers++;
  }

  fclose(stream);
  return(results);
}

/*
** This function builds a result set for a set of layers based on an item/value
** pairing. The value is actually a regular expression and all matching results
** are returned (depending on mode).
*/ 
queryResultObj *msQueryUsingItem(mapObj *map, char *layer, int mode, char *item, char *value)
{
  int i, l; /* counters */
  int start, stop=0;
  regex_t re; /* expression to be matched */
  queryResultObj *results=NULL;
  int itemIndex;
  shapefileObj shpfile;
  int index, queryItemIndex;
  char *status=NULL; // bit array

  if(regcomp(&re, value, REG_EXTENDED|REG_NOSUB) != 0) {
   msSetError(MS_REGEXERR, NULL, "msFindRecords()");
   return(NULL);
  }  

  /*
  ** Do we have query layer, if not we need to search all layers
  */
  if(layer != NULL) {
    if((start = msGetLayerIndex(map, layer)) == -1) {
      sprintf(ms_error.message, "Unable to find query layer %s.", layer);
      msSetError(MS_MISCERR, ms_error.message, "msQueryUsingItem()");
      return(NULL);
    }
    stop = start;
  } else {
    start = map->numlayers-1;
  }

  results = newResults(map->numlayers); /* results exist for each layer */
  if(!results) {
    msSetError(MS_MEMERR, NULL, "msQueryUsingItem()");
    return(NULL);
  }

  for(l=start; l>=stop; l--) {
     
    if((map->layers[l].status == MS_OFF) || (map->layers[l].numqueries == 0) || (map->layers[l].tileindex != NULL))
      continue;

    if(msOpenSHPFile(&shpfile, "rb", map->shapepath, map->tile, map->layers[l].data) == -1) return(NULL);

    if(initLayerResult(&results->layers[l], shpfile.numshapes) == -1) {
      msCloseSHPFile(&shpfile);
      msFreeQueryResults(results);
      return(NULL);
    }

    queryItemIndex = msGetItemIndex(shpfile.hDBF, map->layers[l].queryitem);

    if(strcmp(item, "rec#") == 0) {
      char **ids=NULL;
      int nids, id;

      ids = split(value, '|', &nids);
      
      for(i=0; i<nids; i++) {
	id = atoi(ids[i]);
	if(id >= 0 && id < shpfile.numshapes) { // valid id
	  index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	  if((index < 0) || (index >= map->layers[l].numqueries))
	    continue;
	
	  msSetBit(results->layers[l].status,id,1);
	  results->layers[l].index[id] = index;
	  results->layers[l].numresults++;
	  
	  if(mode == MS_SINGLE) { /* no need to go any further */
	    regfree(&re);
	    free(status);
	    msCloseSHPFile(&shpfile);
	    msFreeCharArray(ids, nids);
	    return(results);
	  }
	}
      }
      
      msFreeCharArray(ids, nids);
      continue;
    } else {
      if((itemIndex = msGetItemIndex(shpfile.hDBF, item)) == -1) { /* requested item doesn't exist in this table */
	msCloseSHPFile(&shpfile);
	continue;
      }
    }

    if(map->extent.minx != map->extent.maxx) { /* use extent as first cut */
#ifdef USE_PROJ
      status = msWhichShapesProj(&shpfile, map->extent, &(map->layers[l].projection), &(map->projection));
#else	    
      status = msWhichShapes(&shpfile, map->extent);
#endif      
    } else {
      status = msAllocBitArray(shpfile.numshapes);
      for(i=0; i<shpfile.numshapes; i++) msSetBit(status,i,1);
    }

    if(!status) {
      msSetError(MS_MEMERR, NULL, "msQueryUsingItem()");
      msCloseSHPFile(&shpfile);
      msFreeQueryResults(results);
      return(NULL);
    }    
    
    for(i=0; i<shpfile.numshapes; i++) {
      if(!msGetBit(status,i))
	continue; /* next shape */

      if(regexec(&re, DBFReadStringAttribute(shpfile.hDBF, i, itemIndex), 0, NULL, 0) == 0) { /* match */
	
	index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	if((index < 0) || (index >= map->layers[l].numqueries))
	  continue;
	
	msSetBit(results->layers[l].status,i,1);
	results->layers[l].index[i] = index;
	results->layers[l].numresults++;

	if(mode == MS_SINGLE) { /* no need to go any further */
	  regfree(&re);
	  free(status);
	  msCloseSHPFile(&shpfile);
	  return(results);
	}
      }
    }

    free(status);
    msCloseSHPFile(&shpfile);
  } /* next layer */

  regfree(&re); /* clear expression */

  for(l=start; l>=stop; l--) {
    results->numresults += results->layers[l].numresults;
    if(results->layers[l].status)
      results->numquerylayers++;
  }

  if(results->numresults > 0) return(results);
  
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryUsingItem()"); 
  msFreeQueryResults(results);
  return(NULL);
}

/*
** This function builds a result set based on a search rectangle. The
** rectangle is intersected with all *active* layers shape by shape.
*/
queryResultObj *msQueryUsingRect(mapObj *map, char *layer, rectObj *rect) {
  int i,j,l; /* counters */
  int start, stop=0;
  queryResultObj *results=NULL;
  char *status=NULL; // bit array
  shapefileObj shpfile;
  int index, queryItemIndex;

  shapeObj shape={0,NULL,{-1,-1,-1,-1},MS_NULL}, search_shape={0,NULL,{-1,-1,-1,-1},MS_NULL};

  if(layer) {
    if((start = msGetLayerIndex(map, layer)) == -1) {
      sprintf(ms_error.message, "Unable to find query layer %s.", layer);
      msSetError(MS_MISCERR, ms_error.message, "msQueryUsingRect()");
      return(NULL);
    }
    stop = start;
  } else {
    start = map->numlayers-1;
  }

  msRect2Polygon(*rect, &search_shape);

  results = newResults(map->numlayers); /* results exist for each layer */
  if(!results) {
    msSetError(MS_MEMERR, NULL, "msQueryUsingRect()");
    return(NULL);
  }

  for(l=start; l>=stop; l--) {

    if((map->layers[l].status == MS_OFF) || (map->layers[l].numqueries == 0) || (map->layers[l].tileindex != NULL))
      continue;

    if(map->scale > 0) {
      if((map->layers[l].maxscale > 0) && (map->scale > map->layers[l].maxscale))
	continue;
      if((map->layers[l].minscale > 0) && (map->scale <= map->layers[l].minscale))
	continue;
    }

    if(msOpenSHPFile(&shpfile, "rb",  map->shapepath, map->tile, map->layers[l].data) == -1) return(NULL);
       
#ifdef USE_PROJ
    status = msWhichShapesProj(&shpfile, *rect, &(map->layers[l].projection), &(map->projection));
#else	    
    status = msWhichShapes(&shpfile, *rect);
#endif
    if(!status) {
      msCloseSHPFile(&shpfile);
      msFreeQueryResults(results);
      return(NULL);
    }

    if(initLayerResult(&results->layers[l], shpfile.numshapes) == -1) {
      msCloseSHPFile(&shpfile);
      msFreeQueryResults(results);
      return(NULL);
    }

    queryItemIndex = msGetItemIndex(shpfile.hDBF, map->layers[l].queryitem);

    for(i=0;i<shpfile.numshapes;i++) { /* step through the shapes */ 

      if(!msGetBit(status,i)) continue; /* next shape */

#ifdef USE_PROJ
      SHPReadShapeProj(shpfile.hSHP, i, &shape, &(map->layers[l].projection), &(map->projection));
#else	    
      SHPReadShape(shpfile.hSHP, i, &shape);
#endif

      if(msRectContained(&shape.bounds, rect) == MS_TRUE) { /* if the whole shape is in, don't intersect */
	msFreeShape(&shape);
	index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	if((index >= 0) && (index < map->layers[l].numqueries)) {
	  msSetBit(results->layers[l].status,i,1);
	  results->layers[l].index[i] = index;
	  results->layers[l].numresults++;
	}
	continue; /* next shape */
      }

      switch(shpfile.type) { /* make sure shape actually intersects the rect */
      case MS_SHP_POINT:
      case MS_SHP_MULTIPOINT:

	for(j=0;j<shape.line[0].numpoints;j++) {
	  if(msPointInRect(&(shape.line[0].point[j]), rect) == MS_TRUE) {
	    index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	    if((index >= 0) && (index < map->layers[l].numqueries)) {
	      msSetBit(results->layers[l].status,i,1);
	      results->layers[l].index[i] = index;
	      results->layers[l].numresults++;
	    }
	    break;
	  }
	} /* next point */
	
	msFreeShape(&shape);
	
	break;
      case MS_SHP_ARC:

	if(msIntersectPolylinePolygon(&shape, &search_shape) == MS_TRUE) {
	  index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	  if((index >= 0) && (index < map->layers[l].numqueries)) {
	    msSetBit(results->layers[l].status,i,1);
	    results->layers[l].index[i] = index;
	    results->layers[l].numresults++;
	  }	  
	}

	msFreeShape(&shape);
	break;
      case MS_SHP_POLYGON:

	if(msIntersectPolygons(&shape, &search_shape) == MS_TRUE) {
	  index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	  if((index >= 0) && (index < map->layers[l].numqueries)) {
	    msSetBit(results->layers[l].status,i,1);
	    results->layers[l].index[i] = index;
	    results->layers[l].numresults++;
	  }
	}

	msFreeShape(&shape);
	break;
      default:
	break;
      }
    } /* next shape */

    free(status);
    msCloseSHPFile(&shpfile);

  } /* next layer */
 
  msFreeShape(&search_shape);

  for(l=start; l>=stop; l--) {
    results->numresults += results->layers[l].numresults;
    if(results->layers[l].status)
      results->numquerylayers++;
  }

  if(results->numresults > 0) return(results);
  
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryUsingRect()"); 
  msFreeQueryResults(results);
  return(NULL); 
}

/*
** This function searches multiple themes if necessary based on the state of a mapfile.
** If a query layer is supplied then only that layer is searched. Depending on mode it
** will return the first/closest feature or all features within a given tolerance. Alternatively
** a global buffer size can be specified in map units when an image based interface is not
** used as the front-end.
*/
queryResultObj *msQueryUsingPoint(mapObj *map, char *layer, int mode, pointObj p, double buffer)
{
  int i,j,l; /* counters */
  int start, stop=0;
  shapefileObj shpfile;
  double tolerance=0, d;
  rectObj rect;
  queryResultObj *results=NULL;
  char *status=NULL; // bit array 
  int index, queryItemIndex;

  shapeObj shape={0,NULL,{-1,-1,-1,-1},MS_NULL};

  /*
  ** Do we have query layer, if not we need to search all layers
  */
  if(layer) {
    if((start = msGetLayerIndex(map, layer)) == -1) {
      sprintf(ms_error.message, "Unable to find query layer %s.", layer);
      msSetError(MS_MISCERR, ms_error.message, "msQueryUsingPoint()");
      return(NULL);
    }
    stop = start;
  } else {
    start = map->numlayers-1;
  }
  
  results = newResults(map->numlayers); /* results exist for each layer */
  if(!results) {
    msSetError(MS_MEMERR, NULL, "msQueryUsingPoint()");
    return(NULL);
  }

  for(l=start; l>=stop; l--) {
    
    if((map->layers[l].status == MS_OFF) || (map->layers[l].numqueries == 0) || (map->layers[l].tileindex != NULL))
      continue;

    if(map->scale > 0) {
      if((map->layers[l].maxscale > 0) && (map->scale > map->layers[l].maxscale))
	continue;
      if((map->layers[l].minscale > 0) && (map->scale <= map->layers[l].minscale))
	continue;
    }

    if(msOpenSHPFile(&shpfile, "rb", map->shapepath, map->tile, map->layers[l].data) == -1) return(NULL);
  
    if(buffer <= 0) { /* use layer tolerance */ 
      if(map->layers[l].toleranceunits == MS_PIXELS)
	tolerance = map->layers[l].tolerance * msAdjustExtent(&(map->extent), map->width, map->height);
      else
	tolerance = map->layers[l].tolerance * (inchesPerUnit[map->layers[l].toleranceunits]/inchesPerUnit[map->units]);
    } else /* use buffer distance */
      tolerance = buffer;

    rect.minx = p.x - tolerance;
    rect.maxx = p.x + tolerance;
    rect.miny = p.y - tolerance;
    rect.maxy = p.y + tolerance;

#ifdef USE_PROJ
    status = msWhichShapesProj(&shpfile, rect, &(map->layers[l].projection), &(map->projection));
#else	    
    status = msWhichShapes(&shpfile, rect);    
#endif
    if(!status) {
      msCloseSHPFile(&shpfile);
      msFreeQueryResults(results);
      return(NULL);
    }

    if(initLayerResult(&results->layers[l], shpfile.numshapes) == -1) {
      msCloseSHPFile(&shpfile);
      msFreeQueryResults(results);
      return(NULL);
    }

    queryItemIndex = msGetItemIndex(shpfile.hDBF, map->layers[l].queryitem); /* which item */
 
    for(i=0;i<shpfile.numshapes;i++) { /* for each shape */

      if(!msGetBit(status,i)) continue;
      
      if((shpfile.type == MS_SHP_POINT) || (shpfile.type == MS_SHP_MULTIPOINT)) {

#ifdef USE_PROJ
	SHPReadShapeProj(shpfile.hSHP, i, &shape, &(map->layers[l].projection), &(map->projection));	    
#else	    
	SHPReadShape(shpfile.hSHP, i, &shape);	    
#endif

	d = msDistanceFromPointToMultipoint(&p, &shape.line[0]);
      } else {

#ifdef USE_PROJ
	SHPReadShapeProj(shpfile.hSHP, i, &shape, &(map->layers[l].projection), &(map->projection));	    
#else
	SHPReadShape(shpfile.hSHP, i, &shape);	    
#endif

	if(shpfile.type == MS_SHP_ARC)
	  d = msDistanceFromPointToPolyline(&p, &shape);
	else /* polygon */
	  d = msDistanceFromPointToPolygon(&p, &shape);	  
      }
      
      if( d <= tolerance ) { /* found one */
	index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	if((index < 0) || (index >= map->layers[l].numqueries))
	  continue;
	
	if(mode == MS_SINGLE) {
	  results->layers[l].numresults = 1;
	  for(j=0; j<i; j++) // de-select anything previously selected
	    msSetBit(results->layers[l].status,j,0);
	  tolerance = d;
	} else {
	  results->layers[l].numresults++;
	}
	
	msSetBit(results->layers[l].status,i,1);
	results->layers[l].index[i] = index;
      }
      
      msFreeShape(&shape);
    } /* next shape */
    
    free(status);
    msCloseSHPFile(&shpfile);
    
    if((results->layers[l].numresults > 0) && (mode == MS_SINGLE)) /* no need to search further */
      break;

  } /* next layer */

  for(l=start; l>=stop; l--) {
    results->numresults += results->layers[l].numresults;
    if(results->layers[l].status)
      results->numquerylayers++;
  }

  if(results->numresults > 0) return(results);

  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryUsingPoint()");
  msFreeQueryResults(results);
  return(NULL);
}

/*
** Generates a result set based on a previous set of results. At present the results MUST
** be based on a polygon layer. Holes and disjoint polygons should now be handled correctly.. 
*/
int msQueryUsingFeatures(mapObj *map, char *layer, queryResultObj *results)
{
  int start, stop=0;
  int l, i, j, search_layer=-1;
  shapefileObj search_shpfile, shpfile;
  char *status=NULL; // bit array
  int index, queryItemIndex;

  shapeObj search_shape={0,NULL,{-1,-1,-1,-1},MS_NULL}, shape={0,NULL,{-1,-1,-1,-1},MS_NULL};

  /*
  ** Do we have query layer, if not we need to search all layers
  */
  if(layer) {
    if((start = msGetLayerIndex(map, layer)) == -1) {      
      sprintf(ms_error.message, "Unable to find query layer %s.", layer);
      msSetError(MS_MISCERR, ms_error.message, "msQueryUsingFeatures()");
      return(-1);
    }
    stop = start;
  } else {
    start = map->numlayers-1;
  }

  /* 
  ** open shapefile to pull features from 
  */
  for(l=0;l<map->numlayers;l++) {
    if(results->layers[l].numresults > 0) {
      if(msOpenSHPFile(&search_shpfile, "rb", map->shapepath, map->tile, map->layers[l].data) == -1) 
	return(-1);
      search_layer = l; /* save the index value */      
      break;
    }
  }  

  for(l=start; l>=stop; l--) { /* each layer */
        
    if((l == search_layer) || (map->layers[l].status == MS_OFF) || (map->layers[l].numqueries == 0) || (map->layers[l].tileindex != NULL))
      continue;

    if(map->scale > 0) {
      if((map->layers[l].maxscale > 0) && (map->scale > map->layers[l].maxscale))
	continue;
      if((map->layers[l].minscale > 0) && (map->scale <= map->layers[l].minscale))
	continue;
    }

    if(msOpenSHPFile(&shpfile, "rb", map->shapepath, map->tile, map->layers[l].data) == -1) return(-1);

    if(initLayerResult(&results->layers[l], shpfile.numshapes) == -1) {
      msCloseSHPFile(&shpfile);
      msCloseSHPFile(&search_shpfile);
      return(-1);
    }

    queryItemIndex = msGetItemIndex(shpfile.hDBF, map->layers[l].queryitem);

    for(i=0; i<search_shpfile.numshapes; i++) { /* each result shape */

      if(!msGetBit(results->layers[search_layer].status, i)) continue;

      /*
      ** Retrieve shapes that *may* intersect the one we're working on
      */
#ifdef USE_PROJ
      SHPReadShapeProj(search_shpfile.hSHP, i, &search_shape, &(map->layers[l].projection), &(map->projection));
      status = msWhichShapesProj(&shpfile, search_shape.bounds, &(map->layers[l].projection), &(map->projection));
#else
      SHPReadShape(search_shpfile.hSHP, i, &search_shape); /* read the search polygon */
      status = msWhichShapes(&shpfile, search_shape.bounds);
#endif
      if(!status) {
	msCloseSHPFile(&shpfile);
	msCloseSHPFile(&search_shpfile);
	return(-1);
      }   
      
      switch(search_shpfile.type) { /* may eventually support other types than polygon */
      case MS_SHP_POLYGON:
	
	switch(shpfile.type) {
	case MS_SHP_POINT:
	case MS_SHP_MULTIPOINT:
	  
	  for(j=0; j<shpfile.numshapes; j++) {
	    
	    if(!msGetBit(status,j)) continue;
	    
	    if(results->layers[l].status[j]) continue; /* already found this feature */
	    
#ifdef USE_PROJ
	    SHPReadShapeProj(shpfile.hSHP, j, &shape, &(map->layers[l].projection), &(map->projection));
#else	    
	    SHPReadShape(shpfile.hSHP, j, &shape);	    
#endif
	    
	    if(msIntersectMultipointPolygon(&shape.line[0], &search_shape) == MS_TRUE) {
	      index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), j, queryItemIndex);
	      if((index >= 0) && (index < map->layers[l].numqueries)) {
		msSetBit(results->layers[l].status,j,1);
		results->layers[l].index[j] = index;
		results->layers[l].numresults++;
	      }
	    }	    
	    
	    msFreeShape(&shape);	
	  }	  
	  
	  break; /* next search shape */
	case MS_SHP_ARC:
	  
	  for(j=0; j<shpfile.numshapes; j++) {
	    
	    if(!msGetBit(status,j)) continue;
	    
	    if(msGetBit(results->layers[l].status,j)) continue; /* already found this feature */
	    
#ifdef USE_PROJ
	    SHPReadShapeProj(shpfile.hSHP, j, &shape, &(map->layers[l].projection), &(map->projection));	    
#else
	    SHPReadShape(shpfile.hSHP, j, &shape);	    
#endif
	    
	    if(msIntersectPolylinePolygon(&shape, &search_shape) == MS_TRUE) {
	      index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), j, queryItemIndex);
	      if((index >= 0) && (index < map->layers[l].numqueries)) {
		msSetBit(results->layers[l].status,j,1);
		results->layers[l].index[j] = index;
		results->layers[l].numresults++;
	      }		
	    }
	    
	    msFreeShape(&shape);
	  }
	  
	  break; /* next shape */
	case MS_SHP_POLYGON:
	  
	  for(j=0; j<shpfile.numshapes; j++) {
	    
	    if(!msGetBit(status,j)) continue;
	    
	    if(msGetBit(results->layers[l].status,j)) continue; /* already found this feature */
	    
#ifdef USE_PROJ
	    SHPReadShapeProj(shpfile.hSHP, j, &shape, &(map->layers[l].projection), &(map->projection));	    
#else
	    SHPReadShape(shpfile.hSHP, j, &shape);	    
#endif
	    
	    if(msIntersectPolygons(&shape, &search_shape) == MS_TRUE) {
	      index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), j, queryItemIndex);
	      if((index >= 0) && (index < map->layers[l].numqueries)) {
		msSetBit(results->layers[l].status,j,1);
		results->layers[l].index[j] = index;
		results->layers[l].numresults++;
	      }
	    }
	    msFreeShape(&shape);
	  }
	  break; /* next result shape/part */
	} /* end shpfile switch */
	break;
      default:
	msSetError(MS_MISCERR, "Selection layer must contain polygons.", "msQueryUsingFeatures()");
	msCloseSHPFile(&search_shpfile);
	return(-1);
      } /* end search_shpfile switch */
      
      free(status);
      msFreeShape(&search_shape);
      
    } /* next result shape */
    
    msCloseSHPFile(&shpfile);
    
  } /* next layer */

  msCloseSHPFile(&search_shpfile); /* done with selection layer */

  for(l=start; l>=stop; l--) {
    if(l != search_layer) {
      results->numresults += results->layers[l].numresults;
      if(results->layers[l].status)
	results->numquerylayers++;
    }
  }

  if(results->numresults > 0) return(0);

  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryUsingFeatures()");
  return(-1); 
}

/*
** Generates a result set based on a single shape, the shape is assumed to be a polygon at this point.
*/
queryResultObj *msQueryUsingShape(mapObj *map, char *layer, shapeObj *search_shape)
{
  int start, stop=0;
  int l, i, j;
  shapefileObj shpfile;
  char *status=NULL; // bit array
  int index, queryItemIndex;
  queryResultObj *results=NULL;
  shapeObj shape={0,NULL,{-1,-1,-1,-1},MS_NULL};

  /* do some checking on search_shape here... */

  /*
  ** Do we have query layer, if not we need to search all layers
  */
  if(layer) {
    if((start = msGetLayerIndex(map, layer)) == -1) {
      sprintf(ms_error.message, "Unable to find query layer %s.", layer);
      msSetError(MS_MISCERR, ms_error.message, "msFindUsingShape()");
      return(NULL);
    }
    stop = start;
  } else {
    start = map->numlayers-1;
  }

  if(search_shape->bounds.minx == search_shape->bounds.maxy) {
    search_shape->bounds.minx = search_shape->bounds.maxx = search_shape->line[0].point[0].x;
    search_shape->bounds.miny = search_shape->bounds.maxy = search_shape->line[0].point[0].y;
    
    for( i=0; i<search_shape->numlines; i++ ) {
      for( j=0; j<search_shape->line[i].numpoints; j++ ) {
	search_shape->bounds.minx = MS_MIN(search_shape->bounds.minx, search_shape->line[i].point[j].x);
	search_shape->bounds.maxx = MS_MAX(search_shape->bounds.maxx, search_shape->line[i].point[j].x);
	search_shape->bounds.miny = MS_MIN(search_shape->bounds.miny, search_shape->line[i].point[j].y);
	search_shape->bounds.maxy = MS_MAX(search_shape->bounds.maxy, search_shape->line[i].point[j].y);
      }
    }
  }

  results = newResults(map->numlayers); /* results exist for each layer */
  if(!results) {
    msSetError(MS_MEMERR, NULL, "msQueryUsingShape()");
    return(NULL);
  }

  for(l=start; l>=stop; l--) { /* each layer */
        
    if((map->layers[l].status == MS_OFF) || (map->layers[l].numqueries == 0) || (map->layers[l].tileindex != NULL))
      continue;

    if(map->scale > 0) {
      if((map->layers[l].maxscale > 0) && (map->scale > map->layers[l].maxscale))
	continue;
      if((map->layers[l].minscale > 0) && (map->scale <= map->layers[l].minscale))
	continue;
    }

    if(msOpenSHPFile(&shpfile, "rb", map->shapepath, map->tile, map->layers[l].data) == -1) return(NULL);

    if(initLayerResult(&results->layers[l], shpfile.numshapes) == -1) {
      msCloseSHPFile(&shpfile);
      return(NULL);
    }

    queryItemIndex = msGetItemIndex(shpfile.hDBF, map->layers[l].queryitem);
    
#ifdef USE_PROJ
    status = msWhichShapesProj(&shpfile, search_shape->bounds, &(map->layers[l].projection), &(map->projection));
#else
    status = msWhichShapes(&shpfile, search_shape->bounds);
#endif
    if(!status) {
      msCloseSHPFile(&shpfile);
      return(NULL);
    }   
      
    switch(shpfile.type) {
    case MS_SHP_POINT:
    case MS_SHP_MULTIPOINT:
      
      for(i=0; i<shpfile.numshapes; i++) {
	
	if(!msGetBit(status,i)) continue;
	
	if(results->layers[l].status[i]) continue; /* already found this feature */
	    
#ifdef USE_PROJ
	SHPReadShapeProj(shpfile.hSHP, i, &shape, &(map->layers[l].projection), &(map->projection));
#else	    
	SHPReadShape(shpfile.hSHP, i, &shape);	    
#endif
	
	if(msIntersectMultipointPolygon(&shape.line[0], search_shape) == MS_TRUE) {
	  index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	  if((index >= 0) && (index < map->layers[l].numqueries)) {
	    msSetBit(results->layers[l].status,i,1);
	    results->layers[l].index[i] = index;
	    results->layers[l].numresults++;
	  }
	}	    
	
	msFreeShape(&shape);	
      }
      
      break;
    case MS_SHP_ARC:
      
      for(i=0; i<shpfile.numshapes; i++) {
	
	if(!msGetBit(status,i)) continue;
	
	if(msGetBit(results->layers[l].status,i)) continue; /* already found this feature */
	
#ifdef USE_PROJ
	SHPReadShapeProj(shpfile.hSHP, i, &shape, &(map->layers[l].projection), &(map->projection));	    
#else
	SHPReadShape(shpfile.hSHP, i, &shape);	    
#endif
	
	if(msIntersectPolylinePolygon(&shape, search_shape) == MS_TRUE) {
	  index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	  if((index >= 0) && (index < map->layers[l].numqueries)) {
	    msSetBit(results->layers[l].status,i,1);
	    results->layers[l].index[i] = index;
	    results->layers[l].numresults++;
	  }		
	}
	
	msFreeShape(&shape);
      }
      
      break;
    case MS_SHP_POLYGON:
      
      for(i=0; i<shpfile.numshapes; i++) {
	
	if(!msGetBit(status,i)) continue;
	
	if(msGetBit(results->layers[l].status,i)) continue; /* already found this feature */
	
#ifdef USE_PROJ
	SHPReadShapeProj(shpfile.hSHP, i, &shape, &(map->layers[l].projection), &(map->projection));	    
#else
	SHPReadShape(shpfile.hSHP, i, &shape);
#endif
	
	if(msIntersectPolygons(&shape, search_shape) == MS_TRUE) {
	  index = shpGetQueryIndex(shpfile.hDBF, &(map->layers[l]), i, queryItemIndex);
	  if((index >= 0) && (index < map->layers[l].numqueries)) {
	    msSetBit(results->layers[l].status,i,1);
	    results->layers[l].index[i] = index;
	    results->layers[l].numresults++;
	  }
	}
	msFreeShape(&shape);
      }
      break; /* next result shape/part */
    default:
      break; /* unsupported shapefile type */
    } /* end shpfile switch */
  
    free(status);
    
    msCloseSHPFile(&shpfile);
    
  } /* next layer */

  for(l=start; l>=stop; l--) {
    results->numresults += results->layers[l].numresults;
    if(results->layers[l].status)
      results->numquerylayers++;
  }

  if(results->numresults > 0) return(results);

  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryUsingShape()");
  return(NULL);
}

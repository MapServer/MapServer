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
  if((hDBF = msDBFOpen(join->table, "rb")) == NULL) {
    sprintf(ms_error.message, "(%s)", join->table);
    msSetError(MS_IOERR, ms_error.message, "msJoinDBFTables()");
    chdir(old_path); /* restore old cwd */
    return(-1);
  }

  if((idx = msDBFGetItemIndex(hDBF, join->to)) == -1) { 
    sprintf(ms_error.message, "Item %s not found.", join->to);   
    msSetError(MS_DBFERR, ms_error.message, "msJoinDBFTables()");    
    msDBFClose(hDBF);
    chdir(old_path); /* restore old cwd */
    return(-1);
  }

  /*
  ** Now we need to pull the record and item names in to the join buffers
  */
  join->numitems =  msDBFGetFieldCount(hDBF);
  if(!join->items) {
    join->items = msDBFGetItems(hDBF);
    if(!join->items) {
      chdir(old_path); /* restore old cwd */
      return(-1);
    }
  }

  nrecs = msDBFGetRecordCount(hDBF);

  if(join->type == MS_SINGLE) { /* only one row */
    
    if((join->data = (char ***)malloc(sizeof(char **))) == NULL) {
      msSetError(MS_MEMERR, NULL, "msJoinDBFTables()");
      chdir(old_path); /* restore old cwd */
      return(-1);
    }
    
    for(i=0; i<nrecs; i++) { /* find a match */
      if(strcmp(join->match, msDBFReadStringAttribute(hDBF, i, idx)) == 0)
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
      if((join->data[0] = msDBFGetValues(hDBF,i)) == NULL) {
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
      if(strcmp(join->match, msDBFReadStringAttribute(hDBF, i, idx)) == 0) {
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

	join->data[i] = msDBFGetValues(hDBF,ids[i]);
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
  msDBFClose(hDBF);
  return(0);
}

extern int msyyresult; // result of parsing, true/false
extern int msyystate;
extern char *msyystring;
extern int msyyparse();

static int addResult(resultCacheObj *cache, int classindex, int shapeindex, int tileindex)
{
  int i;

  if(cache->numresults == cache->cachesize) { // just add it to the end
    cache->results = (resultCacheMemberObj *) realloc(cache->results, sizeof(resultCacheMemberObj)*(cache->cachesize+MS_RESULTCACHEINCREMENT));
    if(!cache->results) {
      msSetError(MS_MEMERR, "Realloc() error.", "addResult()");
      return(MS_FAILURE);
    }   
    cache->cachesize += MS_RESULTCACHEINCREMENT;
  }

  i = cache->numresults;

  cache->results[i].classindex = classindex;
  cache->results[i].tileindex = tileindex;
  cache->results[i].shapeindex = shapeindex;
  cache->numresults++;

  return(MS_SUCCESS);
}

int msSaveQuery(mapObj *map, char *filename) {
  FILE *stream;
  int i, j, n=0;

  if(!filename) {
    msSetError(MS_MISCERR, "No filename provided to save query to.", "msSaveQuery()");
    return(MS_FAILURE);
  }

  stream = fopen(filename, "wb");
  if(!stream) {
    sprintf(ms_error.message, "(%s)", filename);
    msSetError(MS_IOERR, ms_error.message, "msSaveQuery()");
    return(MS_FAILURE);
  }

  // count the number of layers with results
  for(i=0; i<map->numlayers; i++)
    if(map->layers[i].resultcache) n++;
  fwrite(&n, sizeof(int), 1, stream);

  // now write the result set for each layer
  for(i=0; i<map->numlayers; i++) {
    if(map->layers[i].resultcache) {
      fwrite(&i, sizeof(int), 1, stream); // layer index
      fwrite(&(map->layers[i].resultcache->numresults), sizeof(int), 1, stream); // number of results
      fwrite(&(map->layers[i].resultcache->bounds), sizeof(rectObj), 1, stream); // bounding box
      for(j=0; j<map->layers[i].resultcache->numresults; j++)
	fwrite(&(map->layers[i].resultcache->results[j]), sizeof(resultCacheMemberObj), 1, stream); // each result
    }
  }

  fclose(stream);
  return(MS_SUCCESS); 
}

int msLoadQuery(mapObj *map, char *filename) {
  FILE *stream;
  int i, j, k, n=0;

  if(!filename) {
    msSetError(MS_MISCERR, "No filename provided to load query from.", "msLoadQuery()");
    return(MS_FAILURE);
  }

  stream = fopen(filename, "rb");
  if(!stream) {
    sprintf(ms_error.message, "(%s)", filename);
    msSetError(MS_IOERR, ms_error.message, "msLoadQuery()");    
    return(MS_FAILURE);
  }

  fread(&n, sizeof(int), 1, stream);

  // now load the result set for each layer found in the query file
  for(i=0; i<n; i++) {
    fread(&j, sizeof(int), 1, stream); // layer index

    if(j<0 || j>map->numlayers) {
      msSetError(MS_MISCERR, "Invalid layer index loaded from query file.", "msLoadQuery()");
      return(MS_FAILURE);
    }
    
    // inialize the results for this layer
    map->layers[j].resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); // allocate and initialize the result cache

    fread(&(map->layers[j].resultcache->numresults), sizeof(int), 1, stream); // number of results   
    map->layers[j].resultcache->cachesize = map->layers[j].resultcache->numresults;

    fread(&(map->layers[j].resultcache->bounds), sizeof(rectObj), 1, stream); // bounding box

    map->layers[j].resultcache->results = (resultCacheMemberObj *) malloc(sizeof(resultCacheMemberObj)*map->layers[j].resultcache->numresults);

    for(k=0; k<map->layers[j].resultcache->numresults; k++)
      fread(&(map->layers[j].resultcache->results[k]), sizeof(resultCacheMemberObj), 1, stream); // each result
  }

  fclose(stream);
  return(MS_SUCCESS);
}

int msQueryByAttributes(mapObj *map, int qlayer)
{
  layerObj *lp;
  int status;
  
  rectObj searchrect;

  shapeObj shape;

  if(qlayer < 0 || qlayer >= map->numlayers) {
    msSetError(MS_MISCERR, "No query layer defined.", "msQueryByAttributes()"); 
    return(MS_FAILURE);
  }

  lp = &(map->layers[qlayer]);
  msInitShape(&shape);

  // build item list (no annotation)
  status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  // open this layer
  status = msLayerOpen(lp, map->shapepath);
  if(status != MS_SUCCESS) return(MS_FAILURE);
  
  // identify target shapes
  searchrect = map->extent;
#ifdef USE_PROJ     
  if((map->projection.numargs > 0) && (lp->projection.numargs > 0))
    msProjectRect(map->projection.proj, lp->projection.proj, &searchrect); // project the searchrect to source coords
#endif
  status = msLayerWhichShapes(lp, map->shapepath, searchrect);
  if(status == MS_DONE) { // no overlap
    msLayerClose(lp);
     msSetError(MS_NOTFOUND, "No matching record(s) found, layer and area of interest do not overlap.", "msQueryByRect()"); 
    return(MS_FAILURE);
  } else if(status != MS_SUCCESS) {
    msLayerClose(lp);
    return(MS_FAILURE);
  }

  lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); // allocate and initialize the result cache
  lp->resultcache->results = NULL;
  lp->resultcache->numresults = lp->resultcache->cachesize = 0;
  lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;
  
  while((status = msLayerNextShape(lp, map->shapepath, &shape)) == MS_SUCCESS) { // step through the shapes
    shape.classindex = msShapeGetClass(lp, &shape);
    
    if(shape.classindex == -1) { // not a valid shape
      msFreeShape(&shape);
      continue;
    }
    
    if(!(lp->class[shape.classindex].template)) { // no valid template
      msFreeShape(&shape);
      continue;
    }

    addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
    
    if(lp->resultcache->numresults == 1)
      lp->resultcache->bounds = shape.bounds;
    else
      msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
    
    msFreeShape(&shape);
  }

  // was anything found? 
  if(lp->resultcache && lp->resultcache->numresults > 0)
    return(MS_SUCCESS);
 
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByRect()"); 
  return(MS_FAILURE);
}

int msQueryByRect(mapObj *map, int qlayer, rectObj rect) 
{
  int i,l; /* counters */
  int start, stop=0;

  layerObj *lp;

  char status;
  shapeObj shape, searchshape;
  rectObj searchrect;

  msInitShape(&shape);
  msInitShape(&searchshape);

  msRectToPolygon(rect, &searchshape);

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  for(l=start; l>=stop; l--) {
    lp = &(map->layers[l]);

    if(lp->status == MS_OFF) continue;

    if(map->scale > 0) {
      if((lp->maxscale > 0) && (map->scale > lp->maxscale)) continue;
      if((lp->minscale > 0) && (map->scale <= lp->minscale)) continue;
    }

    status = MS_FAILURE;
    for(i=0; i<lp->numclasses; i++) {
      if(lp->class[i].template) {
	status = MS_SUCCESS;
	break;
      }
    }
    if(status != MS_SUCCESS) continue;

    // build item list (no annotation)
    status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    // open this layer
    status = msLayerOpen(lp, map->shapepath);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    // identify target shapes
    searchrect = rect;
#ifdef USE_PROJ
    if((map->projection.numargs > 0) && (lp->projection.numargs > 0))
      msProjectRect(map->projection.proj, lp->projection.proj, &searchrect); // project the searchrect to source coords
#endif
    status = msLayerWhichShapes(lp, map->shapepath, searchrect);
    if(status == MS_DONE) { // no overlap
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      return(MS_FAILURE);
    }

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); // allocate and initialize the result cache
    lp->resultcache->results = NULL;
    lp->resultcache->numresults = lp->resultcache->cachesize = 0;
    lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;

    while((status = msLayerNextShape(lp, map->shapepath, &shape)) == MS_SUCCESS) { // step through the shapes
      shape.classindex = msShapeGetClass(lp, &shape);

      if(shape.classindex == -1) { // not a valid shape
	msFreeShape(&shape);
	continue;
      }

      if(!(lp->class[shape.classindex].template)) { // no valid template
	msFreeShape(&shape);
	continue;
      }

#ifdef USE_PROJ
      if((lp->projection.numargs > 0) && (map->projection.numargs > 0))
	msProjectPolyline(lp->projection.proj, map->projection.proj, &shape);
#endif

      if(msRectContained(&shape.bounds, &rect) == MS_TRUE) { /* if the whole shape is in, don't intersect */	
	status = MS_TRUE;
      } else {
	switch(shape.type) { // make sure shape actually intersects the rect (ADD FUNCTIONS SPECIFIC TO RECTOBJ)
	case MS_POINT:
	  status = msIntersectMultipointPolygon(&shape.line[0], &searchshape);
	  break;
	case MS_LINE:
	  status = msIntersectPolylinePolygon(&shape, &searchshape);
	  break;
	case MS_POLYGON:
	  status = msIntersectPolygons(&shape, &searchshape);
	  break;
	default:
	  break;
	}
      }	

      if(status == MS_TRUE) {
	addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
	
	if(lp->resultcache->numresults == 1)
	  lp->resultcache->bounds = shape.bounds;
	else
	  msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
      }

      msFreeShape(&shape);
    } // next shape
      
    if(status != MS_DONE) return(MS_FAILURE);

    msLayerClose(lp);
  } // next layer
 
  msFreeShape(&searchshape);
 
  // was anything found?
  for(l=start; l>=stop; l--) {    
    if(map->layers[l].resultcache && map->layers[l].resultcache->numresults > 0)
      return(MS_SUCCESS);
  }
 
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByRect()"); 
  return(MS_FAILURE);
}

int msQueryByPoint(mapObj *map, int qlayer, int mode, pointObj p, double buffer)
{
  int i, l;
  int start, stop=0;

  double d, t;

  layerObj *lp;

  char status;
  rectObj rect, searchrect;
  shapeObj shape;

  msDebug("in msQueryByPoint()...\n");

  msInitShape(&shape);

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  msDebug("start = %d, stop = %d\n", start, stop);

  for(l=start; l>=stop; l--) {
    lp = &(map->layers[l]);

    if(lp->status == MS_OFF) continue;

    if(map->scale > 0) {
      if((lp->maxscale > 0) && (map->scale > lp->maxscale)) continue;
      if((lp->minscale > 0) && (map->scale <= lp->minscale)) continue;
    }

    status = MS_FAILURE;
    for(i=0; i<lp->numclasses; i++) {
      if(lp->class[i].template) {
	status = MS_SUCCESS;
	break;
      }
    }
    if(status != MS_SUCCESS) continue;

    msDebug("working on %s\n", lp->name);

    if(buffer <= 0) { // use layer tolerance
      if(lp->toleranceunits == MS_PIXELS)
	t = lp->tolerance * msAdjustExtent(&(map->extent), map->width, map->height);
      else
	t = lp->tolerance * (inchesPerUnit[lp->toleranceunits]/inchesPerUnit[map->units]);
    } else // use buffer distance
      t = buffer;

    rect.minx = p.x - t;
    rect.maxx = p.x + t;
    rect.miny = p.y - t;
    rect.maxy = p.y + t;

    // build item list (no annotation)
    status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    // open this layer
    status = msLayerOpen(lp, map->shapepath);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    // identify target shapes
    searchrect = rect;
#ifdef USE_PROJ
    if((map->projection.numargs > 0) && (lp->projection.numargs > 0))
      msProjectRect(map->projection.proj, lp->projection.proj, &searchrect); // project the searchrect to source coords
#endif
    status = msLayerWhichShapes(lp, map->shapepath, rect);
    if(status == MS_DONE) { // no overlap
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      return(MS_FAILURE);
    }

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); // allocate and initialize the result cache
    lp->resultcache->results = NULL;
    lp->resultcache->numresults = lp->resultcache->cachesize = 0;
    lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;

    while((status = msLayerNextShape(lp, map->shapepath, &shape)) == MS_SUCCESS) { // step through the shapes
      shape.classindex = msShapeGetClass(lp, &shape);
      if(shape.classindex == -1) { // not a valid shape
	msFreeShape(&shape);
	continue;
      }

      if(!(lp->class[shape.classindex].template)) { // no valid template
	msFreeShape(&shape);
	continue;
      }

#ifdef USE_PROJ
      if((lp->projection.numargs > 0) && (map->projection.numargs > 0))
	msProjectPolyline(lp->projection.proj, map->projection.proj, &shape);
#endif

      if(shape.type == MS_POINT)
	d = msDistanceFromPointToMultipoint(&p, &shape.line[0]);
      else if(shape.type == MS_LINE)
	d = msDistanceFromPointToPolyline(&p, &shape);
      else // MS_POLYGON
	d = msDistanceFromPointToPolygon(&p, &shape);	  

      if( d <= t ) { // found one
	if(mode == MS_SINGLE) {
	  lp->resultcache->numresults = 0;
	  addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
	  lp->resultcache->bounds = shape.bounds;
	  t = d; // next one must be closer
	} else {
	  addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
	  if(lp->resultcache->numresults == 1)
	    lp->resultcache->bounds = shape.bounds;
	  else
	    msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
	}
      }
 
      msFreeShape(&shape);	
    } // next shape

    if(status != MS_DONE) return(MS_FAILURE);

    msLayerClose(lp);

    if((lp->resultcache->numresults > 0) && (mode == MS_SINGLE)) // no need to search any further
      break;
  } // next layer

  msDebug("done searching\n");

  // was anything found?
  for(l=start; l>=stop; l--) {    
    if(map->layers[l].resultcache && map->layers[l].resultcache->numresults > 0)
      return(MS_SUCCESS);
  }
 
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByPoint()"); 
  return(MS_FAILURE);
}

int msQueryByFeatures(mapObj *map, int qlayer, int slayer)
{
  return(MS_FAILURE);
}

int msQueryByShape(mapObj *map, int qlayer, shapeObj *searchshape)
{
  int start, stop=0, i, l;
  shapeObj shape;
  layerObj *lp;
  char status;
  rectObj searchrect;

  // FIX: do some checking on searchshape here...
  if(searchshape->type != MS_POLYGON) {
    msSetError(MS_MISCERR, "Search shape MUST be a polygon.", "msQueryByShape()"); 
    return(MS_FAILURE);
  }

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  msComputeBounds(searchshape); // make sure an accurate extent exists
 
  for(l=start; l>=stop; l--) { /* each layer */
    lp = &(map->layers[l]);

    if(lp->status == MS_OFF) continue;
    
    if(map->scale > 0) {
      if((lp->maxscale > 0) && (map->scale > lp->maxscale)) continue;
      if((lp->minscale > 0) && (map->scale <= lp->minscale)) continue;
    }
   
    status = MS_FAILURE;
    for(i=0; i<lp->numclasses; i++) {
      if(lp->class[i].template) {
	status = MS_SUCCESS;
	break;
      }
    }
    if(status != MS_SUCCESS) continue;

    // build item list (no annotation)
    status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    // open this layer
    status = msLayerOpen(lp, map->shapepath);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    // identify target shapes
    searchrect = searchshape->bounds;
#ifdef USE_PROJ
    if((map->projection.numargs > 0) && (lp->projection.numargs > 0))
      msProjectRect(map->projection.proj, lp->projection.proj, &searchrect); // project the searchrect to source coords
#endif
    status = msLayerWhichShapes(lp, map->shapepath, searchrect);
    if(status == MS_DONE) { // no overlap
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      return(MS_FAILURE);
    }

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); // allocate and initialize the result cache
    lp->resultcache->results = NULL;
    lp->resultcache->numresults = lp->resultcache->cachesize = 0;
    lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;

    while((status = msLayerNextShape(lp, map->shapepath, &shape)) == MS_SUCCESS) { // step through the shapes
      shape.classindex = msShapeGetClass(lp, &shape);

      if(shape.classindex == -1) { // not a valid shape
	msFreeShape(&shape);
	continue;
      }

      if(!(lp->class[shape.classindex].template)) { // no valid template
	msFreeShape(&shape);
	continue;
      }

#ifdef USE_PROJ
      if((lp->projection.numargs > 0) && (map->projection.numargs > 0))
	msProjectPolyline(lp->projection.proj, map->projection.proj, &shape);
#endif

      switch(shape.type) { // make sure shape actually intersects the shape
      case MS_POINT:
	status = msIntersectMultipointPolygon(&shape.line[0], searchshape);	
	break;
      case MS_LINE:
	status = msIntersectPolylinePolygon(&shape, searchshape);
	break;
      case MS_POLYGON:
	status = msIntersectPolygons(&shape, searchshape);
	break;
      default:
	break;
      }

      if(status == MS_TRUE) {
	addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
	
	if(lp->resultcache->numresults == 1)
	  lp->resultcache->bounds = shape.bounds;
	else
	  msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
      }

      msFreeShape(&shape);
    } // next shape

    if(status != MS_DONE) return(MS_FAILURE);

    msLayerClose(lp);
  } // next layer

  // was anything found?
  for(l=start; l>=stop; l--) {    
    if(map->layers[l].resultcache && map->layers[l].resultcache->numresults > 0)
      return(MS_SUCCESS);
  }
 
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByShape()"); 
  return(MS_FAILURE);
}

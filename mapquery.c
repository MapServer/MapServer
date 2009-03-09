/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  layer query support.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "mapserver.h"

MS_CVSID("$Id$")

/*
** msIsLayerQueryable()  returns MS_TRUE/MS_FALSE
*/
int msIsLayerQueryable(layerObj *lp)
{
  int i;

  if ( lp->type == MS_LAYER_TILEINDEX )
    return MS_FALSE;

  if(lp->template && strlen(lp->template) > 0) return MS_TRUE;

  for(i=0; i<lp->numclasses; i++) {
    if(lp->class[i]->template && strlen(lp->class[i]->template) > 0)
        return MS_TRUE;
  }

  return MS_FALSE;
}

static int addResult(resultCacheObj *cache, int classindex, int shapeindex, int tileindex)
{
  int i;

  if(cache->numresults == cache->cachesize) { /* just add it to the end */
    if(cache->cachesize == 0)
      cache->results = (resultCacheMemberObj *) malloc(sizeof(resultCacheMemberObj)*MS_RESULTCACHEINCREMENT);
    else
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
    msSetError(MS_IOERR, "(%s)", "msSaveQuery()", filename);
    return(MS_FAILURE);
  }

  /* count the number of layers with results */
  for(i=0; i<map->numlayers; i++)
    if(GET_LAYER(map, i)->resultcache) n++;
  fwrite(&n, sizeof(int), 1, stream);

  /* now write the result set for each layer */
  for(i=0; i<map->numlayers; i++) {
    if(GET_LAYER(map, i)->resultcache) {
      fwrite(&i, sizeof(int), 1, stream); /* layer index */
      fwrite(&(GET_LAYER(map, i)->resultcache->numresults), sizeof(int), 1, stream); /* number of results */
      fwrite(&(GET_LAYER(map, i)->resultcache->bounds), sizeof(rectObj), 1, stream); /* bounding box */
      for(j=0; j<GET_LAYER(map, i)->resultcache->numresults; j++)
	fwrite(&(GET_LAYER(map, i)->resultcache->results[j]), sizeof(resultCacheMemberObj), 1, stream); /* each result */
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
    msSetError(MS_IOERR, "(%s)", "msLoadQuery()", filename);
    return(MS_FAILURE);
  }

  fread(&n, sizeof(int), 1, stream);

  /* now load the result set for each layer found in the query file */
  for(i=0; i<n; i++) {
    fread(&j, sizeof(int), 1, stream); /* layer index */

    if(j<0 || j>map->numlayers) {
      msSetError(MS_MISCERR, "Invalid layer index loaded from query file.", "msLoadQuery()");
      return(MS_FAILURE);
    }
    
    /* inialize the results for this layer */
    GET_LAYER(map, j)->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */

    fread(&(GET_LAYER(map, j)->resultcache->numresults), sizeof(int), 1, stream); /* number of results    */
    GET_LAYER(map, j)->resultcache->cachesize = GET_LAYER(map, j)->resultcache->numresults;

    fread(&(GET_LAYER(map, j)->resultcache->bounds), sizeof(rectObj), 1, stream); /* bounding box */

    GET_LAYER(map, j)->resultcache->results = (resultCacheMemberObj *) malloc(sizeof(resultCacheMemberObj)*GET_LAYER(map, j)->resultcache->numresults);

    for(k=0; k<GET_LAYER(map, j)->resultcache->numresults; k++)
      fread(&(GET_LAYER(map, j)->resultcache->results[k]), sizeof(resultCacheMemberObj), 1, stream); /* each result */
  }

  fclose(stream);
  return(MS_SUCCESS);
}


int _msQueryByIndex(mapObj *map, int qlayer, int tileindex, int shapeindex, 
                    int addtoquery)
{
  layerObj *lp;
  int status;
  
  shapeObj shape;

  if(qlayer < 0 || qlayer >= map->numlayers) {
    msSetError(MS_QUERYERR, "No query layer defined.", "msQueryByIndex()"); 
    return(MS_FAILURE);
  }

  lp = (GET_LAYER(map, qlayer));

  if(!msIsLayerQueryable(lp)) {
    msSetError(MS_QUERYERR, "Requested layer has no templates defined.", "msQueryByIndex()"); 
    return(MS_FAILURE);
  }

  msInitShape(&shape);

  if (!addtoquery) {  
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }
  }

  /* open this layer */
  status = msLayerOpen(lp);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  /* build item list (no annotation) since we do have to classify the shape */
  status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE, NULL);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  if (!addtoquery || lp->resultcache == NULL) {
    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    lp->resultcache->results = NULL;
    lp->resultcache->numresults = lp->resultcache->cachesize = 0;
    lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;
  }

  status = msLayerGetShape(lp, &shape, tileindex, shapeindex);
  if(status != MS_SUCCESS) {
    msSetError(MS_NOTFOUND, "Not valid record request.", "msQueryByIndex()"); 
    return(MS_FAILURE);
  }

  shape.classindex = msShapeGetClass(lp, &shape, map->scaledenom, NULL, 0);
  if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
    msSetError(MS_NOTFOUND, "Shape %d not valid against layer classification.", "msQueryByIndex()", shapeindex);
    msFreeShape(&shape);
    return(MS_FAILURE);
  }
    
  if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
    msFreeShape(&shape);
    msSetError(MS_NOTFOUND, "Shape does not have a valid template, no way to present results.", "msQueryByIndex()"); 
    return(MS_FAILURE);
  }

  addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
  if(lp->resultcache->numresults == 1)
    lp->resultcache->bounds = shape.bounds;
  else
    msMergeRect(&(lp->resultcache->bounds), &shape.bounds);

  msFreeShape(&shape);
  msLayerClose(lp);

  return(MS_SUCCESS);
}

/* msQueryByIndexAdd()
 *
 * Same function as msQueryByIndex but instead of freeing the current
 * result cache, the new shape will be added to the result.
 */
int msQueryByIndexAdd(mapObj *map, int qlayer, int tileindex, int shapeindex)
{
     return _msQueryByIndex(map, qlayer, tileindex, shapeindex, 1);
}

int msQueryByIndex(mapObj *map, int qlayer, int tileindex, int shapeindex)
{
    return _msQueryByIndex(map, qlayer, tileindex, shapeindex, 0);
}

int msQueryByAttributes(mapObj *map, int qlayer, char *qitem, char *qstring, int mode)
{
  layerObj *lp;
  int status;
  
  int old_filtertype=-1;
  char *old_filterstring=NULL, *old_filteritem=NULL;

  rectObj searchrect;

  shapeObj shape;
  
  int nclasses = 0;
  int *classgroup = NULL;

  if(qlayer < 0 || qlayer >= map->numlayers) {
    msSetError(MS_MISCERR, "No query layer defined.", "msQueryByAttributes()"); 
    return(MS_FAILURE);
  }

  lp = (GET_LAYER(map, qlayer));

  /* conditions may have changed since this layer last drawn, so set 
     layer->project true to recheck projection needs (Bug #673) */ 
  lp->project = MS_TRUE; 

  /* free any previous search results, do now in case one of the following tests fails */
  if(lp->resultcache) {
    if(lp->resultcache->results) free(lp->resultcache->results);
    free(lp->resultcache);
    lp->resultcache = NULL;
  }

  if(!msIsLayerQueryable(lp)) {
    msSetError(MS_QUERYERR, "Requested layer has no templates defined so is not queryable.", "msQueryByAttributes()"); 
    return(MS_FAILURE);
  }

  if(!qstring) {
    msSetError(MS_QUERYERR, "No query expression defined.", "msQueryByAttributes()"); 
    return(MS_FAILURE);
  }

  /* save any previously defined filter */
  if(lp->filter.string) {
    old_filtertype = lp->filter.type;
    old_filterstring = strdup(lp->filter.string);
    if(lp->filteritem) 
      old_filteritem = strdup(lp->filteritem);
  }

  /* apply the passed query parameters */
  if(qitem && qitem[0] != '\0') 
    lp->filteritem = strdup(qitem);
  else
    lp->filteritem = NULL;
  msLoadExpressionString(&(lp->filter), qstring);

  msInitShape(&shape);

  /* open this layer */
  status = msLayerOpen(lp);
  if(status != MS_SUCCESS) return(MS_FAILURE);
  
  /* build item list (no annotation) */
  status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE, NULL);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  /* identify target shapes */
  searchrect = map->extent;
#ifdef USE_PROJ  
  if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))  
    msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */
  else
    lp->project = MS_FALSE;
#endif

  status = msLayerWhichShapes(lp, searchrect);
  if(status == MS_DONE) { /* no overlap */
    msLayerClose(lp);
    msSetError(MS_NOTFOUND, "No matching record(s) found, layer and area of interest do not overlap.", "msQueryByAttributes()");
    return(MS_FAILURE);
  } else if(status != MS_SUCCESS) {
    msLayerClose(lp);
    return(MS_FAILURE);
  }

  lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
  lp->resultcache->results = NULL;
  lp->resultcache->numresults = lp->resultcache->cachesize = 0;
  lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;
  
  nclasses = 0;
  classgroup = NULL;
  if (lp->classgroup && lp->numclasses > 0)
      classgroup = msAllocateValidClassGroups(lp, &nclasses);

  while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

    shape.classindex = msShapeGetClass(lp, &shape, map->scaledenom, classgroup, nclasses );    
    if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
      msFreeShape(&shape);
      continue;
    }

    if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
      msFreeShape(&shape);
      continue;
    }

#ifdef USE_PROJ
    if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))
      msProjectShape(&(lp->projection), &(map->projection), &shape);
    else
      lp->project = MS_FALSE;
#endif

    addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
    
    if(lp->resultcache->numresults == 1)
      lp->resultcache->bounds = shape.bounds;
    else
      msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
    
    msFreeShape(&shape);

    if(mode == MS_SINGLE) { /* no need to look any further */
      status = MS_DONE;
      break;
    }
  }

  if (classgroup)
    msFree(classgroup);

  if(status != MS_DONE) return(MS_FAILURE);

  /* the FILTER set was just temporary, clean up here */
  freeExpression(&(lp->filter));
  if(lp->filteritem) {
    free(lp->filteritem);
    lp->filteritem = NULL;
    lp->filteritemindex = -1;
  }
  
  /* restore any previously defined filter */
  if(old_filterstring) {
    lp->filter.type = old_filtertype;
    lp->filter.string = strdup(old_filterstring);
    free(old_filterstring);
    if(old_filteritem) { 
      lp->filteritem = strdup(old_filteritem);
      free(old_filteritem);
    }
  }

  msLayerClose(lp);

  /* was anything found?  */
  if(lp->resultcache && lp->resultcache->numresults > 0)
    return(MS_SUCCESS);
 
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByAttributes()"); 
  return(MS_FAILURE);
}

int msQueryByRect(mapObj *map, int qlayer, rectObj rect) 
{
  int l; /* counters */
  int start, stop=0;

  layerObj *lp;

  char status;
  shapeObj shape, searchshape;
  rectObj searchrect;
  
  int nclasses = 0;
  int *classgroup = NULL;

  msInitShape(&shape);
  msInitShape(&searchshape);

  msRectToPolygon(rect, &searchshape);

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  for(l=start; l>=stop; l--) {
    lp = (GET_LAYER(map, l));

    /* conditions may have changed since this layer last drawn, so set 
       layer->project true to recheck projection needs (Bug #673) */ 
    lp->project = MS_TRUE; 

    /* free any previous search results, do it now in case one of the next few tests fail */
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }

    if(!msIsLayerQueryable(lp)) continue;
    if(lp->status == MS_OFF) continue;

    if(map->scaledenom > 0) {
      if((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom)) continue;
      if((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom)) continue;
    }

    if (lp->maxscaledenom <= 0 && lp->minscaledenom <= 0) {
      if((lp->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > lp->maxgeowidth)) continue;
      if((lp->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < lp->mingeowidth)) continue;
    }

    /* Raster layers are handled specially. */
    if( lp->type == MS_LAYER_RASTER ) {
      if( msRasterQueryByRect( map, lp, rect ) == MS_FAILURE)
        return MS_FAILURE;

      continue;
    }

    /* open this layer */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* build item list (no annotation) */
    status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* identify target shapes */
    searchrect = rect;
#ifdef USE_PROJ
    if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))
      msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */
    else
      lp->project = MS_FALSE;
#endif
    status = msLayerWhichShapes(lp, searchrect);
    if(status == MS_DONE) { /* no overlap */
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      return(MS_FAILURE);
    }

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    lp->resultcache->results = NULL;
    lp->resultcache->numresults = lp->resultcache->cachesize = 0;
    lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;

    nclasses = 0;
    classgroup = NULL;
    if (lp->classgroup && lp->numclasses > 0)
      classgroup = msAllocateValidClassGroups(lp, &nclasses);

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

      shape.classindex = msShapeGetClass(lp, &shape, map->scaledenom, classgroup, nclasses);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
	      msFreeShape(&shape);
	      continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
        msFreeShape(&shape);
	      continue;
      }

#ifdef USE_PROJ
      if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))
	msProjectShape(&(lp->projection), &(map->projection), &shape);
      else
	lp->project = MS_FALSE;
#endif

      if(msRectContained(&shape.bounds, &rect) == MS_TRUE) { /* if the whole shape is in, don't intersect */	
	      status = MS_TRUE;
      } else {
	switch(shape.type) { /* make sure shape actually intersects the rect (ADD FUNCTIONS SPECIFIC TO RECTOBJ) */
	case MS_SHAPE_POINT:
	  status = msIntersectMultipointPolygon(&shape, &searchshape);
	  break;
	case MS_SHAPE_LINE:
	  status = msIntersectPolylinePolygon(&shape, &searchshape);
	  break;
	case MS_SHAPE_POLYGON:
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
    } /* next shape */
      
    if (classgroup)
      msFree(classgroup);

    if(status != MS_DONE) return(MS_FAILURE);

    msLayerClose(lp);
  } /* next layer */
 
  msFreeShape(&searchshape);
 
  /* was anything found? */
  for(l=start; l>=stop; l--) {    
    if(GET_LAYER(map, l)->resultcache && GET_LAYER(map, l)->resultcache->numresults > 0)
      return(MS_SUCCESS);
  }
 
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByRect()"); 
  return(MS_FAILURE);
}

static int is_duplicate(resultCacheObj *resultcache, int shapeindex, int tileindex) {
  int i;

  for(i=0; i<resultcache->numresults; i++)    
    if(resultcache->results[i].shapeindex == shapeindex && resultcache->results[i].tileindex == tileindex) return(MS_TRUE);

  return(MS_FALSE);
}

int msQueryByFeatures(mapObj *map, int qlayer, int slayer)
{
  int i, l;
  int start, stop=0;
  layerObj *lp, *slp;
  char status;

  double distance, tolerance, layer_tolerance;

  rectObj searchrect;
  shapeObj shape, selectshape;
  int nclasses = 0;
  int *classgroup = NULL;

  msInitShape(&shape);
  msInitShape(&selectshape);

  if( map->debug )
      msDebug( "in msQueryByFeatures()\n" );

  /* is the selection layer valid and has it been queried */
  if(slayer < 0 || slayer >= map->numlayers) {
    msSetError(MS_QUERYERR, "Invalid selection layer index.", "msQueryByFeatures()");
    return(MS_FAILURE);
  }
  slp = (GET_LAYER(map, slayer));
  if(!slp->resultcache) {
    msSetError(MS_QUERYERR, "Selection layer has not been queried.", "msQueryByFeatures()");
    return(MS_FAILURE);
  }

  /* conditions may have changed since this layer last drawn, so set 
     layer->project true to recheck projection needs (Bug #673) */ 
  slp->project = MS_TRUE; 

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  status = msLayerOpen(slp);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  for(l=start; l>=stop; l--) {
    if(l == slayer) continue; /* skip the selection layer */
    
    lp = (GET_LAYER(map, l));

    /* conditions may have changed since this layer last drawn, so set 
       layer->project true to recheck projection needs (Bug #673) */ 
    lp->project = MS_TRUE; 

    /* free any previous search results, do it now in case one of the next few tests fail */
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }

    if(!msIsLayerQueryable(lp)) continue;    
    if(lp->status == MS_OFF) continue;
    
    if(map->scaledenom > 0) {
      if((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom)) continue;
      if((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom)) continue;
    }

    if (lp->maxscaledenom <= 0 && lp->minscaledenom <= 0) {
      if((lp->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > lp->maxgeowidth)) continue;
      if((lp->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < lp->mingeowidth)) continue;
    }

    /* Get the layer tolerance
       default is 3 for point and line layers, 0 for others */
    if(lp->tolerance == -1) {
      if(lp->status == MS_LAYER_POINT || lp->status == MS_LAYER_LINE)
        layer_tolerance = 3;
       else
        layer_tolerance = 0;
    } else
      layer_tolerance = lp->tolerance;
  
    if(lp->toleranceunits == MS_PIXELS)
      tolerance = layer_tolerance * msAdjustExtent(&(map->extent), map->width, map->height);
    else
      tolerance = layer_tolerance * (msInchesPerUnit(lp->toleranceunits,0)/msInchesPerUnit(map->units,0));
   
    /* open this layer */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);
    
    /* build item list (no annotation) */
    status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);
    
    /* for each selection shape */
    for(i=0; i<slp->resultcache->numresults; i++) {

      status = msLayerGetShape(slp, &selectshape, slp->resultcache->results[i].tileindex, slp->resultcache->results[i].shapeindex);
      if(status != MS_SUCCESS) {
	msLayerClose(lp);
	msLayerClose(slp);
	return(MS_FAILURE);
      }

      if(selectshape.type != MS_SHAPE_POLYGON && selectshape.type != MS_SHAPE_LINE) {
	msLayerClose(lp);
	msLayerClose(slp);
	msSetError(MS_QUERYERR, "Selection features MUST be polygons or lines.", "msQueryByFeatures()");
	return(MS_FAILURE);
      }
      
#ifdef USE_PROJ
      if(slp->project && msProjectionsDiffer(&(slp->projection), &(map->projection))) {
	msProjectShape(&(slp->projection), &(map->projection), &selectshape);
	msComputeBounds(&selectshape); /* recompute the bounding box AFTER projection */
      } else
	slp->project = MS_FALSE;
#endif

      /* identify target shapes */
      searchrect = selectshape.bounds;

#ifdef USE_PROJ
      if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))      
	msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */
      else
	lp->project = MS_FALSE;
#endif

      searchrect.minx -= tolerance; /* expand the search box to account for layer tolerances (e.g. buffered searches) */
      searchrect.maxx += tolerance;
      searchrect.miny -= tolerance;
      searchrect.maxy += tolerance;

      status = msLayerWhichShapes(lp, searchrect);
      if(status == MS_DONE) { /* no overlap */
	msLayerClose(lp);
	break; /* next layer */
      } else if(status != MS_SUCCESS) {
	msLayerClose(lp);
	msLayerClose(slp);
	return(MS_FAILURE);
      }

      if(i == 0) {
	lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
	lp->resultcache->results = NULL;
	lp->resultcache->numresults = lp->resultcache->cachesize = 0;
	lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;    
      }      

      nclasses = 0;
      classgroup = NULL;
      if (lp->classgroup && lp->numclasses > 0)
        classgroup = msAllocateValidClassGroups(lp, &nclasses);

      while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

	/* check for dups when there are multiple selection shapes */
	if(i > 0 && is_duplicate(lp->resultcache, shape.index, shape.tileindex)) continue;

	shape.classindex = msShapeGetClass(lp, &shape, map->scaledenom, classgroup, nclasses);
	if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
	  msFreeShape(&shape);
	  continue;
	}

	if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
	  msFreeShape(&shape);
	  continue;
	}
	
#ifdef USE_PROJ
	if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))	
	  msProjectShape(&(lp->projection), &(map->projection), &shape);
        else
	  lp->project = MS_FALSE;
#endif
 
	switch(selectshape.type) { /* may eventually support types other than polygon on line */
	case MS_SHAPE_POLYGON:	  
	  switch(shape.type) { /* make sure shape actually intersects the selectshape */
	  case MS_SHAPE_POINT:
	    if(tolerance == 0) /* just test for intersection */
	      status = msIntersectMultipointPolygon(&shape, &selectshape);
	    else { /* check distance, distance=0 means they intersect */
	      distance = msDistanceShapeToShape(&selectshape, &shape);
	      if(distance < tolerance) status = MS_TRUE;
            }
	    break;
	  case MS_SHAPE_LINE:
	    if(tolerance == 0) { /* just test for intersection */
	      status = msIntersectPolylinePolygon(&shape, &selectshape);
	    } else { /* check distance, distance=0 means they intersect */
	      distance = msDistanceShapeToShape(&selectshape, &shape);
	      if(distance < tolerance) status = MS_TRUE;
            }
	    break;
	  case MS_SHAPE_POLYGON:
	    if(tolerance == 0) /* just test for intersection */
	      status = msIntersectPolygons(&shape, &selectshape);
	    else { /* check distance, distance=0 means they intersect */
	      distance = msDistanceShapeToShape(&selectshape, &shape);
	      if(distance < tolerance) status = MS_TRUE;
            }
	    break;
	  default:
            status = MS_FALSE;
	    break;
	  }
	  break;
	case MS_SHAPE_LINE:
          switch(shape.type) { /* make sure shape actually intersects the selectshape */
          case MS_SHAPE_POINT:
            if(tolerance == 0) { /* just test for intersection */
              distance = msDistanceShapeToShape(&selectshape, &shape);
              if(distance == 0) status = MS_TRUE;
            } else {
	      distance = msDistanceShapeToShape(&selectshape, &shape);
              if(distance < tolerance) status = MS_TRUE;
            }
            break;
          case MS_SHAPE_LINE:
            if(tolerance == 0) { /* just test for intersection */
              status = msIntersectPolylines(&shape, &selectshape);
            } else { /* check distance, distance=0 means they intersect */
              distance = msDistanceShapeToShape(&selectshape, &shape);
              if(distance < tolerance) status = MS_TRUE;
            }
            break;
          case MS_SHAPE_POLYGON:
            if(tolerance == 0) /* just test for intersection */
              status = msIntersectPolylinePolygon(&selectshape, &shape);
            else { /* check distance, distance=0 means they intersect */
              distance = msDistanceShapeToShape(&selectshape, &shape);
              if(distance < tolerance) status = MS_TRUE;
            }
            break;
          default:
            status = MS_FALSE;
            break;
          }
          break;
	default:
	  break; /* should never get here as we test for selection shape type explicitly earlier */
	}

	if(status == MS_TRUE) {
	  addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);

	  if(lp->resultcache->numresults == 1)
	    lp->resultcache->bounds = shape.bounds;
	  else
	    msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
	}

	msFreeShape(&shape);
      } /* next shape */

      if (classgroup)
        msFree(classgroup);

      if(status != MS_DONE) return(MS_FAILURE);

      msFreeShape(&selectshape);
    } /* next selection shape */

    msLayerClose(lp);
  } /* next layer */

  msLayerClose(slp);

  /* was anything found? */
  for(l=start; l>=stop; l--) {  
    if(l == slayer) continue; /* skip the selection layer */

    if(GET_LAYER(map, l)->resultcache && GET_LAYER(map, l)->resultcache->numresults > 0)
      return(MS_SUCCESS);
  }

  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByFeatures()"); 
  return(MS_FAILURE);
}

/* msQueryByPoint()
 *
 * With mode=MS_SINGLE:
 *   Pass maxresults = 0 to have a single result across all layers (the closest
 *   shape from the first layer that finds a match).
 *   Pass maxresults = 1 to have up to one result per layer (the closest shape
 *   from each layer).
 *
 * With mode=MS_MULTIPLE:
 *   Pass maxresults = 0 to have an unlimited number of results.
 *   Pass maxresults > 0 to limit the number of results per layer (the shapes
 *   returned are the first ones found in each layer and are not necessarily
 *   the closest ones).
 */
int msQueryByPoint(mapObj *map, int qlayer, int mode, pointObj p, double buffer, int maxresults)
{
  int l;
  int start, stop=0;

  double d, t;
  double layer_tolerance;

  layerObj *lp;

  char status;
  rectObj rect, searchrect;
  shapeObj shape;
  int nclasses = 0;
  int *classgroup = NULL;

  msInitShape(&shape);

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  for(l=start; l>=stop; l--) {
    lp = (GET_LAYER(map, l));    

    /* conditions may have changed since this layer last drawn, so set 
       layer->project true to recheck projection needs (Bug #673) */ 
    lp->project = MS_TRUE; 

    /* free any previous search results, do it now in case one of the next few tests fail */
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }

    if(!msIsLayerQueryable(lp)) continue;
    if(lp->status == MS_OFF) continue;

    if(map->scaledenom > 0) {
      if((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom)) continue;
      if((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom)) continue;
    }

    if (lp->maxscaledenom <= 0 && lp->minscaledenom <= 0) {
      if((lp->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > lp->maxgeowidth)) continue;
      if((lp->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < lp->mingeowidth)) continue;
    }

    /* Raster layers are handled specially.  */
    if( lp->type == MS_LAYER_RASTER ) {
        if( msRasterQueryByPoint( map, lp, mode, p, buffer )
            == MS_FAILURE )
            return MS_FAILURE;
        continue;
    }

    /* Get the layer tolerance
       default is 3 for point and line layers, 0 for others */
    if(lp->tolerance == -1)
        if(lp->status == MS_LAYER_POINT || lp->status == MS_LAYER_LINE)
            layer_tolerance = 3;
        else
            layer_tolerance = 0;
    else
        layer_tolerance = lp->tolerance;

    if(buffer <= 0) { /* use layer tolerance */
      if(lp->toleranceunits == MS_PIXELS)
	t = layer_tolerance * msAdjustExtent(&(map->extent), map->width, map->height);
      else
	t = layer_tolerance * (msInchesPerUnit(lp->toleranceunits,0)/msInchesPerUnit(map->units,0));
    } else /* use buffer distance */
      t = buffer;

    rect.minx = p.x - t;
    rect.maxx = p.x + t;
    rect.miny = p.y - t;
    rect.maxy = p.y + t;

    /* open this layer */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* build item list (no annotation) */
    status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* identify target shapes */
    searchrect = rect;
#ifdef USE_PROJ
    if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))
      msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */
    else
      lp->project = MS_FALSE;
#endif
    status = msLayerWhichShapes(lp, searchrect);
    if(status == MS_DONE) { /* no overlap */
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      return(MS_FAILURE);
    }

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    lp->resultcache->results = NULL;
    lp->resultcache->numresults = lp->resultcache->cachesize = 0;
    lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;

    nclasses = 0;
    classgroup = NULL;
    if (lp->classgroup && lp->numclasses > 0)
      classgroup = msAllocateValidClassGroups(lp, &nclasses);

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

      shape.classindex = msShapeGetClass(lp, &shape, map->scaledenom, classgroup, nclasses);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
	msFreeShape(&shape);
	continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
	msFreeShape(&shape);
	continue;
      }

#ifdef USE_PROJ
      if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))
	msProjectShape(&(lp->projection), &(map->projection), &shape);
      else
	lp->project = MS_FALSE;
#endif

      d = msDistancePointToShape(&p, &shape);
      if( d <= t ) { /* found one */
	if(mode == MS_SINGLE) {
	  lp->resultcache->numresults = 0;
	  addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
	  lp->resultcache->bounds = shape.bounds;
	  t = d; /* next one must be closer */
	} else {
	  addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
	  if(lp->resultcache->numresults == 1)
	    lp->resultcache->bounds = shape.bounds;
	  else
	    msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
	}
      }
 
      msFreeShape(&shape);

      if (mode == MS_MULTIPLE && maxresults > 0 && lp->resultcache->numresults == maxresults) {
          status = MS_DONE;   /* got enough results for this layer */
          break;
      }

    } /* next shape */

    if (classgroup)
      msFree(classgroup);

    if(status != MS_DONE) return(MS_FAILURE);

    msLayerClose(lp);

    if((lp->resultcache->numresults > 0) && (mode == MS_SINGLE) && (maxresults == 0)) 
      break;   /* no need to search any further */
  } /* next layer */

  /* was anything found? */
  for(l=start; l>=stop; l--) {    
    if(GET_LAYER(map, l)->resultcache && GET_LAYER(map, l)->resultcache->numresults > 0)
      return(MS_SUCCESS);
  }
 
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByPoint()"); 
  return(MS_FAILURE);
}

int msQueryByShape(mapObj *map, int qlayer, shapeObj *selectshape)
{
  int start, stop=0, l;
  shapeObj shape;
  layerObj *lp;
  char status;
  double distance, tolerance, layer_tolerance;
  rectObj searchrect;

  int nclasses = 0;
  int *classgroup = NULL;

   msInitShape(&shape);

  /* FIX: do some checking on selectshape here... */
  if(selectshape->type != MS_SHAPE_POLYGON) {
    msSetError(MS_QUERYERR, "Search shape MUST be a polygon.", "msQueryByShape()"); 
    return(MS_FAILURE);
  }

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  msComputeBounds(selectshape); /* make sure an accurate extent exists */
 
  for(l=start; l>=stop; l--) { /* each layer */
    lp = (GET_LAYER(map, l));

    /* conditions may have changed since this layer last drawn, so set 
       layer->project true to recheck projection needs (Bug #673) */ 
    lp->project = MS_TRUE; 

    /* free any previous search results, do it now in case one of the next few tests fail */
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }

    if(!msIsLayerQueryable(lp)) continue;
    if(lp->status == MS_OFF) continue;
    
    if(map->scaledenom > 0) {
      if((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom)) continue;
      if((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom)) continue;
    }

    if (lp->maxscaledenom <= 0 && lp->minscaledenom <= 0) {
      if((lp->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > lp->maxgeowidth)) continue;
      if((lp->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < lp->mingeowidth)) continue;
    }

    /* Raster layers are handled specially. */
    if( lp->type == MS_LAYER_RASTER )
    {
        if( msRasterQueryByShape( map, lp, selectshape )
            == MS_FAILURE )
            return MS_FAILURE;

        continue;
    }

    /* Get the layer tolerance
       default is 3 for point and line layers, 0 for others */
    if(lp->tolerance == -1)
        if(lp->status == MS_LAYER_POINT || lp->status == MS_LAYER_LINE)
            layer_tolerance = 3;
        else
            layer_tolerance = 0;
    else
        layer_tolerance = lp->tolerance;

    if(lp->toleranceunits == MS_PIXELS)
      tolerance = layer_tolerance * msAdjustExtent(&(map->extent), map->width, map->height);
    else
      tolerance = layer_tolerance * (msInchesPerUnit(lp->toleranceunits,0)/msInchesPerUnit(map->units,0));
   
    /* open this layer */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* build item list (no annotation) */
    status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* identify target shapes */
    searchrect = selectshape->bounds;
#ifdef USE_PROJ
    if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))
      msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */
    else
      lp->project = MS_FALSE;
#endif

    searchrect.minx -= tolerance; /* expand the search box to account for layer tolerances (e.g. buffered searches) */
    searchrect.maxx += tolerance;
    searchrect.miny -= tolerance;
    searchrect.maxy += tolerance;

    status = msLayerWhichShapes(lp, searchrect);
    if(status == MS_DONE) { /* no overlap */
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      return(MS_FAILURE);
    }

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    lp->resultcache->results = NULL;
    lp->resultcache->numresults = lp->resultcache->cachesize = 0;
    lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;

    nclasses = 0;
    classgroup = NULL;
    if (lp->classgroup && lp->numclasses > 0)
      classgroup = msAllocateValidClassGroups(lp, &nclasses);

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

      shape.classindex = msShapeGetClass(lp, &shape, map->scaledenom, classgroup, nclasses);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
	msFreeShape(&shape);
	continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
	msFreeShape(&shape);
	continue;
      }

#ifdef USE_PROJ
      if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))
	msProjectShape(&(lp->projection), &(map->projection), &shape);
      else
	lp->project = MS_FALSE;
#endif

      switch(shape.type) { /* make sure shape actually intersects the shape */
      case MS_SHAPE_POINT:
        if(tolerance == 0) /* just test for intersection */
	  status = msIntersectMultipointPolygon(&shape, selectshape);
	else { /* check distance, distance=0 means they intersect */
	  distance = msDistanceShapeToShape(selectshape, &shape);
	  if(distance < tolerance) status = MS_TRUE;
        }
	break;
      case MS_SHAPE_LINE:
        if(tolerance == 0) { /* just test for intersection */
	  status = msIntersectPolylinePolygon(&shape, selectshape);
	} else { /* check distance, distance=0 means they intersect */
	  distance = msDistanceShapeToShape(selectshape, &shape);
	  if(distance < tolerance) status = MS_TRUE;
        }
	break;
      case MS_SHAPE_POLYGON:	
	if(tolerance == 0) /* just test for intersection */
	  status = msIntersectPolygons(&shape, selectshape);
	else { /* check distance, distance=0 means they intersect */
	  distance = msDistanceShapeToShape(selectshape, &shape);
	  if(distance < tolerance) status = MS_TRUE;
        }
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
    } /* next shape */

    if(status != MS_DONE) return(MS_FAILURE);

    msLayerClose(lp);
  } /* next layer */

  /* was anything found? */
  for(l=start; l>=stop; l--) {    
    if(GET_LAYER(map, l)->resultcache && GET_LAYER(map, l)->resultcache->numresults > 0)
      return(MS_SUCCESS);
  }
 
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByShape()"); 
  return(MS_FAILURE);
}


/************************************************************************/
/*                            msQueryByOperator                         */
/*                                                                      */
/*      query using a shape and a valid operator : all queries are       */
/*      done using geos.                                                */
/************************************************************************/
int msQueryByOperator(mapObj *map, int qlayer, shapeObj *selectshape,
                      int geos_operator)
{
#ifdef USE_GEOS
    int start, stop=0, l;
    shapeObj shape;
    layerObj *lp;
    int status;
    rectObj searchrect;
     
    int nclasses = 0;
    int *classgroup = NULL;
    double dfValue;

    msInitShape(&shape);

    /* this should not be a necessary test for uries using geos*/
    /*
      if(selectshape->type != MS_SHAPE_POLYGON) {
      msSetError(MS_QUERYERR, "Search shape MUST be a polygon.", "msQueryByShape()"); 
      return(MS_FAILURE);
      }
    */

    if(qlayer < 0 || qlayer >= map->numlayers)
      start = map->numlayers-1;
    else
      start = stop = qlayer;

    msComputeBounds(selectshape); /* make sure an accurate extent exists */
 
    for(l=start; l>=stop; l--) { /* each layer */
      lp = (GET_LAYER(map, l));

      /* conditions may have changed since this layer last drawn, so set 
         layer->project true to recheck projection needs (Bug #673) */ 
      lp->project = MS_TRUE; 

      /* free any previous search results, do it now in case one of the next few tests fail */
      if(lp->resultcache) {
        if(lp->resultcache->results) free(lp->resultcache->results);
        free(lp->resultcache);
        lp->resultcache = NULL;
      }

      if(!msIsLayerQueryable(lp)) continue;
      if(lp->status == MS_OFF) continue;
    
      if(map->scaledenom > 0) {
        if((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom)) continue;
        if((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom)) continue;
      }

      if (lp->maxscaledenom <= 0 && lp->minscaledenom <= 0) {
        if((lp->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > lp->maxgeowidth)) continue;
        if((lp->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < lp->mingeowidth)) continue;
      }

      /* Raster layers are handled specially. */
      if( lp->type == MS_LAYER_RASTER )
      {
        if( msRasterQueryByShape( map, lp, selectshape )
            == MS_FAILURE )
          return MS_FAILURE;

        continue;
      }

    
      /* open this layer */
      status = msLayerOpen(lp);
      if(status != MS_SUCCESS) return(MS_FAILURE);

      /* build item list (no annotation) */
      status = msLayerWhichItems(lp, MS_TRUE, MS_FALSE, NULL);
      if(status != MS_SUCCESS) return(MS_FAILURE);

/* identify target shapes */
      searchrect.minx = map->extent.minx;
      searchrect.miny = map->extent.miny;
      searchrect.maxx = map->extent.maxx;
      searchrect.maxy = map->extent.maxy;
      
#ifdef USE_PROJ
    if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))
      msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */
    else
      lp->project = MS_FALSE;
#endif
    status = msLayerWhichShapes(lp, searchrect);
    if(status == MS_DONE) { /* no overlap */
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      return(MS_FAILURE);
    }

      lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
      lp->resultcache->results = NULL;
      lp->resultcache->numresults = lp->resultcache->cachesize = 0;
      lp->resultcache->bounds.minx = lp->resultcache->bounds.miny = lp->resultcache->bounds.maxx = lp->resultcache->bounds.maxy = -1;

       classgroup = NULL;
       if (lp->classgroup && lp->numclasses > 0)
         classgroup = msAllocateValidClassGroups(lp, &nclasses);

      while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

        shape.classindex = msShapeGetClass(lp, &shape, map->scaledenom, classgroup, nclasses);
        if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
          msFreeShape(&shape);
          continue;
        }

        if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
          msFreeShape(&shape);
          continue;
        }

#ifdef USE_PROJ
        if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))
          msProjectShape(&(lp->projection), &(map->projection), &shape);
        else
          lp->project = MS_FALSE;
#endif

        switch(geos_operator) 
        { 
          case MS_GEOS_EQUALS:
            status = msGEOSEquals(&shape, selectshape);
            if (status != MS_TRUE && status != MS_FALSE)
              status = MS_FALSE;
            break;
        
          case MS_GEOS_DISJOINT:
            status = msGEOSDisjoint(&shape, selectshape);
            if (status != MS_TRUE && status != MS_FALSE)
              status = MS_FALSE;
            break;

          case MS_GEOS_TOUCHES:
            status = msGEOSTouches(&shape, selectshape);
            if (status != MS_TRUE && status != MS_FALSE)
              status = MS_FALSE;
            break;

          case MS_GEOS_OVERLAPS:
            status = msGEOSOverlaps(&shape, selectshape);
            if (status != MS_TRUE && status != MS_FALSE)
              status = MS_FALSE;
            break;

          case MS_GEOS_CROSSES:
            status = msGEOSCrosses(&shape, selectshape);
            if (status != MS_TRUE && status != MS_FALSE)
              status = MS_FALSE;
            break;
        
          case MS_GEOS_INTERSECTS:
            status = msGEOSIntersects(&shape, selectshape);
            if (status != MS_TRUE && status != MS_FALSE)
              status = MS_FALSE;
            break;

          case MS_GEOS_WITHIN:
            status = msGEOSWithin(&shape, selectshape);
            if (status != MS_TRUE && status != MS_FALSE)
              status = MS_FALSE;
            break;
          
          case MS_GEOS_CONTAINS:
            status = msGEOSContains(selectshape, &shape);
            if (status != MS_TRUE && status != MS_FALSE)
              status = MS_FALSE;
            break;

            /*beyond is opposite of dwithin use in filter encoding
              see ticket 2105, 2564*/
          case MS_GEOS_BEYOND:
            status = MS_FALSE;
            dfValue = msGEOSDistance(&shape, selectshape);
            if (dfValue > 0.0)
               status = MS_TRUE;
          break;

          /*dwithin is used with filter encoding (#2564)*/
          case MS_GEOS_DWITHIN:
            status = MS_FALSE;
            dfValue = msGEOSDistance(&shape, selectshape);
            if (dfValue == 0.0)
              status = MS_TRUE;

          break;

          default:
            msSetError(MS_QUERYERR, "Unknown GEOS Operator.", "msQueryByOperator()"); 
            return(MS_FAILURE);
        }

        if(status == MS_TRUE) {
          addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
	
          if(lp->resultcache->numresults == 1)
            lp->resultcache->bounds = shape.bounds;
          else
            msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
        }

        msFreeShape(&shape);
      } /* next shape */

      if(status != MS_DONE) return(MS_FAILURE);

      msLayerClose(lp);
    } /* next layer */

    /* was anything found? */
    for(l=start; l>=stop; l--) {    
      if(GET_LAYER(map, l)->resultcache && GET_LAYER(map, l)->resultcache->numresults > 0)
        return(MS_SUCCESS);
    }

 
    msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByOperator()"); 
    return(MS_FAILURE);
#else
  msSetError(MS_GEOSERR, "GEOS support is not available.", "msQueryByOperator()");
  return(MS_FAILURE);
#endif

}

/* msGetQueryResultBounds()
 *
 * Compute the BBOX of all query results, returns the number of layers found
 * that contained query results and were included in the BBOX.
 * i.e. if we return 0 then the value in bounds is invalid.
 */
int msGetQueryResultBounds(mapObj *map, rectObj *bounds)
{
  int i, found=0;
  rectObj tmpBounds;

  for(i=0; i<map->numlayers; i++) {

    layerObj *lp;
    lp = (GET_LAYER(map, i));

    if(!lp->resultcache) continue;
    if(lp->resultcache->numresults <= 0) continue;

    tmpBounds = lp->resultcache->bounds;

    if(found == 0) {
      *bounds = tmpBounds;
    } else {
      msMergeRect(bounds, &tmpBounds);
    }

    found++;
  }

  return found;
}


/* msQueryFree()
 *
 * Free layer's query results. If qlayer == -1, all layers will be treated.
 */
void msQueryFree(mapObj *map, int qlayer)
{
  int l; /* counters */
  int start, stop=0;
  layerObj *lp;
    
  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  for(l=start; l>=stop; l--) {
    lp = (GET_LAYER(map, l));
        
    if(lp->resultcache) {
      if(lp->resultcache->results) 
        free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }
  }
}
      

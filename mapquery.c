/******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.91  2006/02/02 01:00:11  sdlime
 * Moved the code to free previous query results a bit higher in the msQueryByAttribute() function.
 *
 * Revision 1.90  2006/02/02 00:53:16  sdlime
 * Fixed bug when doing repeated queries where you make a layer not queryable then it is possible the results for that layer can persist. (bug 1550)
 *
 * Revision 1.89  2006/01/01 02:42:39  sdlime
 * The bounds that are part of the result cache are projected so there is no need to do any projection when computing the overall result set bounds.
 *
 * Revision 1.88  2005/12/21 23:31:19  sdlime
 * Fixed an error that occurs when doing attribute queries against layers with a FILTER. The code to cache the existing filter blindly called strdup on a value that could be NULL.
 *
 * Revision 1.87  2005/06/14 16:03:34  dan
 * Updated copyright date to 2005
 *
 * Revision 1.86  2005/04/27 16:52:35  assefa
 * Bug 1335 : missing call to msInitShape in function msQueryByShape.
 *
 * Revision 1.85  2005/04/15 19:32:33  julien
 * Bug 1103: Set the default tolerance value based on the layer type.
 *
 * Revision 1.84  2005/02/18 03:06:47  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.83  2004/11/11 19:06:36  sdlime
 * Applied patch for bug 1003 (mapshape and projected layers).
 *
 * Revision 1.82  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"

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
    if(lp->class[i].template && strlen(lp->class[i].template) > 0)
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
    if(map->layers[i].resultcache) n++;
  fwrite(&n, sizeof(int), 1, stream);

  /* now write the result set for each layer */
  for(i=0; i<map->numlayers; i++) {
    if(map->layers[i].resultcache) {
      fwrite(&i, sizeof(int), 1, stream); /* layer index */
      fwrite(&(map->layers[i].resultcache->numresults), sizeof(int), 1, stream); /* number of results */
      fwrite(&(map->layers[i].resultcache->bounds), sizeof(rectObj), 1, stream); /* bounding box */
      for(j=0; j<map->layers[i].resultcache->numresults; j++)
	fwrite(&(map->layers[i].resultcache->results[j]), sizeof(resultCacheMemberObj), 1, stream); /* each result */
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

  /*
  ** Make sure the file at least has the right extension.
  */
  if(msEvalRegex("\\.qy$", filename) != MS_TRUE) return MS_FAILURE;

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
    map->layers[j].resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */

    fread(&(map->layers[j].resultcache->numresults), sizeof(int), 1, stream); /* number of results    */
    map->layers[j].resultcache->cachesize = map->layers[j].resultcache->numresults;

    fread(&(map->layers[j].resultcache->bounds), sizeof(rectObj), 1, stream); /* bounding box */

    map->layers[j].resultcache->results = (resultCacheMemberObj *) malloc(sizeof(resultCacheMemberObj)*map->layers[j].resultcache->numresults);

    for(k=0; k<map->layers[j].resultcache->numresults; k++)
      fread(&(map->layers[j].resultcache->results[k]), sizeof(resultCacheMemberObj), 1, stream); /* each result */
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

  lp = &(map->layers[qlayer]);

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

  shape.classindex = msShapeGetClass(lp, &shape, map->scale);
  if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex].status == MS_OFF))) { /* not a valid shape */
    msSetError(MS_NOTFOUND, "Shape %d not valid against layer classification.", "msQueryByIndex()", shapeindex);
    msFreeShape(&shape);
    return(MS_FAILURE);
  }
    
  if(!(lp->template) && !(lp->class[shape.classindex].template)) { /* no valid template */
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

  if(qlayer < 0 || qlayer >= map->numlayers) {
    msSetError(MS_MISCERR, "No query layer defined.", "msQueryByAttributes()"); 
    return(MS_FAILURE);
  }

  lp = &(map->layers[qlayer]);

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
  if(qitem) 
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
  
  while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

    shape.classindex = msShapeGetClass(lp, &shape, map->scale);    
    if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex].status == MS_OFF))) { /* not a valid shape */
      msFreeShape(&shape);
      continue;
    }
    
    if(!(lp->template) && !(lp->class[shape.classindex].template)) { /* no valid template */
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

  msInitShape(&shape);
  msInitShape(&searchshape);

  msRectToPolygon(rect, &searchshape);

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  for(l=start; l>=stop; l--) {
    lp = &(map->layers[l]);

    /* free any previous search results, do it now in case one of the next few tests fail */
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }

    if(!msIsLayerQueryable(lp)) continue;
    if(lp->status == MS_OFF) continue;

    if(map->scale > 0) {
      if((lp->maxscale > 0) && (map->scale > lp->maxscale)) continue;
      if((lp->minscale > 0) && (map->scale <= lp->minscale)) continue;
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

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

      shape.classindex = msShapeGetClass(lp, &shape, map->scale);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex].status == MS_OFF))) { /* not a valid shape */
	msFreeShape(&shape);
	continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex].template)) { /* no valid template */
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
	  status = msIntersectMultipointPolygon(&shape.line[0], &searchshape);
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
      
    if(status != MS_DONE) return(MS_FAILURE);

    msLayerClose(lp);
  } /* next layer */
 
  msFreeShape(&searchshape);
 
  /* was anything found? */
  for(l=start; l>=stop; l--) {    
    if(map->layers[l].resultcache && map->layers[l].resultcache->numresults > 0)
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

  msInitShape(&shape);
  msInitShape(&selectshape);

  if( map->debug )
      msDebug( "in msQueryByFeatures()\n" );

  /* is the selection layer valid and has it been queried */
  if(slayer < 0 || slayer >= map->numlayers) {
    msSetError(MS_QUERYERR, "Invalid selection layer index.", "msQueryByFeatures()");
    return(MS_FAILURE);
  }
  slp = &(map->layers[slayer]);
  if(!slp->resultcache) {
    msSetError(MS_QUERYERR, "Selection layer has not been queried.", "msQueryByFeatures()");
    return(MS_FAILURE);
  }

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  status = msLayerOpen(slp);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  for(l=start; l>=stop; l--) {
    if(l == slayer) continue; /* skip the selection layer */
    
    lp = &(map->layers[l]);

    /* free any previous search results, do it now in case one of the next few tests fail */
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }

    if(!msIsLayerQueryable(lp)) continue;    
    if(lp->status == MS_OFF) continue;
    
    if(map->scale > 0) {
      if((lp->maxscale > 0) && (map->scale > lp->maxscale)) continue;
      if((lp->minscale > 0) && (map->scale <= lp->minscale)) continue;
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
    
    /* for each selection shape */
    for(i=0; i<slp->resultcache->numresults; i++) {

      status = msLayerGetShape(slp, &selectshape, slp->resultcache->results[i].tileindex, slp->resultcache->results[i].shapeindex);
      if(status != MS_SUCCESS) {
	msLayerClose(lp);
	msLayerClose(slp);
	return(MS_FAILURE);
      }

      if(selectshape.type != MS_SHAPE_POLYGON) {
	msLayerClose(lp);
	msLayerClose(slp);
	msSetError(MS_QUERYERR, "Selection features MUST be polygons.", "msQueryByFeatures()");
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

      while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

	/* check for dups when there are multiple selection shapes */
	if(i > 0 && is_duplicate(lp->resultcache, shape.index, shape.tileindex)) continue;

	shape.classindex = msShapeGetClass(lp, &shape, map->scale);
	if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex].status == MS_OFF))) { /* not a valid shape */
	  msFreeShape(&shape);
	  continue;
	}

	if(!(lp->template) && !(lp->class[shape.classindex].template)) { /* no valid template */
	  msFreeShape(&shape);
	  continue;
	}
	
#ifdef USE_PROJ
	if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))	
	  msProjectShape(&(lp->projection), &(map->projection), &shape);
        else
	  lp->project = MS_FALSE;
#endif
 
	switch(selectshape.type) { /* may eventually support types other than polygon */
	case MS_SHAPE_POLYGON:
	  
	  switch(shape.type) { /* make sure shape actually intersects the selectshape */
	  case MS_SHAPE_POINT:
	    if(tolerance == 0) /* just test for intersection */
	      status = msIntersectMultipointPolygon(&shape.line[0], &selectshape);
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
	    break;
	  }
	  
	  if(status == MS_TRUE) {
	    addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
	    
	    if(lp->resultcache->numresults == 1)
	      lp->resultcache->bounds = shape.bounds;
	    else
	      msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
	  }

	  break;
	default:
	  break;
	}

	msFreeShape(&shape);
      } /* next shape */

      if(status != MS_DONE) return(MS_FAILURE);

      msFreeShape(&selectshape);
    } /* next selection shape */

    msLayerClose(lp);
  } /* next layer */

  msLayerClose(slp);

  /* was anything found? */
  for(l=start; l>=stop; l--) {  
    if(l == slayer) continue; /* skip the selection layer */

    if(map->layers[l].resultcache && map->layers[l].resultcache->numresults > 0)
      return(MS_SUCCESS);
  }

  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByFeatures()"); 
  return(MS_FAILURE);
}

int msQueryByPoint(mapObj *map, int qlayer, int mode, pointObj p, double buffer)
{
  int l;
  int start, stop=0;

  double d, t;
  double layer_tolerance;

  layerObj *lp;

  char status;
  rectObj rect, searchrect;
  shapeObj shape;

  msInitShape(&shape);

  if(qlayer < 0 || qlayer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = qlayer;

  for(l=start; l>=stop; l--) {
    lp = &(map->layers[l]);    

    /* free any previous search results, do it now in case one of the next few tests fail */
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }

    if(!msIsLayerQueryable(lp)) continue;
    if(lp->status == MS_OFF) continue;

    if(map->scale > 0) {
      if((lp->maxscale > 0) && (map->scale > lp->maxscale)) continue;
      if((lp->minscale > 0) && (map->scale <= lp->minscale)) continue;
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

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

      shape.classindex = msShapeGetClass(lp, &shape, map->scale);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex].status == MS_OFF))) { /* not a valid shape */
	msFreeShape(&shape);
	continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex].template)) { /* no valid template */
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
    } /* next shape */

    if(status != MS_DONE) return(MS_FAILURE);

    msLayerClose(lp);

    if((lp->resultcache->numresults > 0) && (mode == MS_SINGLE)) /* no need to search any further */
      break;
  } /* next layer */

  /* was anything found? */
  for(l=start; l>=stop; l--) {    
    if(map->layers[l].resultcache && map->layers[l].resultcache->numresults > 0)
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
    lp = &(map->layers[l]);

    /* free any previous search results, do it now in case one of the next few tests fail */
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }

    if(!msIsLayerQueryable(lp)) continue;
    if(lp->status == MS_OFF) continue;
    
    if(map->scale > 0) {
      if((lp->maxscale > 0) && (map->scale > lp->maxscale)) continue;
      if((lp->minscale > 0) && (map->scale <= lp->minscale)) continue;
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

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

      shape.classindex = msShapeGetClass(lp, &shape, map->scale);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex].status == MS_OFF))) { /* not a valid shape */
	msFreeShape(&shape);
	continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex].template)) { /* no valid template */
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
	  status = msIntersectMultipointPolygon(&shape.line[0], selectshape);
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
    if(map->layers[l].resultcache && map->layers[l].resultcache->numresults > 0)
      return(MS_SUCCESS);
  }
 
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByShape()"); 
  return(MS_FAILURE);
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
    lp = &(map->layers[i]);

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
    lp = &(map->layers[l]);
        
    if(lp->resultcache) {
      if(lp->resultcache->results) 
        free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }
  }
}
      

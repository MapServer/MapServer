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

int msInitQuery(queryObj *query)
{
  if(!query) return MS_FAILURE;

  msFreeQuery(query); /* clean up anything previously allocated */

  query->type = MS_QUERY_IS_NULL; /* nothing defined */
  query->mode = MS_QUERY_SINGLE;

  query->layer = query->slayer = -1;

  query->point.x = query->point.y = -1;
  query->buffer = 0.0;
  query->maxresults = 0;

  query->rect.minx = query->rect.miny = query->rect.maxx = query->rect.maxy = -1;
  query->shape = NULL;

  query->shapeindex = query->tileindex = -1;
  query->clear_resultcache = MS_TRUE; /* index queries allow the old results to persist */

  query->op = -1;

  query->item = query->str = NULL;

  return MS_SUCCESS;
}

void msFreeQuery(queryObj *query)
{
  if(query->shape) {
    msFreeShape(query->shape);
    free(query->shape);
  }

  if(query->item) free(query->item);
  if(query->str) free(query->str);
}

/*
** Wrapper for all query functions, just feed is a mapObj with a populated queryObj...
*/
int msExecuteQuery(mapObj *map)
{
  int tmp=-1, status;

  /* handle slayer/qlayer management for feature queries */
  if(map->query.slayer >= 0) {
    tmp = map->query.layer;
    map->query.layer = map->query.slayer;
  }

  switch(map->query.type) {
  case MS_QUERY_BY_POINT:
    status = msQueryByPoint(map);
    break;
  case MS_QUERY_BY_RECT:
    status = msQueryByRect(map);
    break;
  case MS_QUERY_BY_ATTRIBUTE:
    status = msQueryByAttributes(map);
    break;
  case MS_QUERY_BY_OPERATOR:
    status = msQueryByOperator(map);
    break;
  case MS_QUERY_BY_SHAPE:
    status = msQueryByShape(map);
    break;
  case MS_QUERY_BY_INDEX:
    status = msQueryByIndex(map);
    break;
  default:
    msSetError(MS_QUERYERR, "Malformed queryObj.", "msExecuteQuery()");
    return(MS_FAILURE);
  }

  if(map->query.slayer >= 0) {
    map->query.layer = tmp; /* restore layer */
    if(status == MS_SUCCESS) status = msQueryByFeatures(map);
  }

  return status;
}

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

  if(!filename) {
    msSetError(MS_MISCERR, "No filename provided to save query to.", "msSaveQuery()");
    return(MS_FAILURE);
  }

  stream = fopen(filename, "w");
  if(!stream) {
    msSetError(MS_IOERR, "(%s)", "msSaveQuery()", filename);
    return(MS_FAILURE);
  }

  fprintf(stream, "%s - Generated by msSaveQuery()\n", MS_QUERY_MAGIC_STRING);

  fprintf(stream, "%d %d %d %d\n", map->query.mode, map->query.type, map->query.layer, map->query.slayer); /* all queries */
  fprintf(stream, "%.15g %.15g %g %d\n", map->query.point.x, map->query.point.y, map->query.buffer, map->query.maxresults); /* by point */
  fprintf(stream, "%.15g %.15g %.15g %.15g\n", map->query.rect.minx, map->query.rect.miny, map->query.rect.maxx, map->query.rect.maxy); /* by rect */
  fprintf(stream, "%ld %ld %d\n", map->query.shapeindex, map->query.tileindex, map->query.clear_resultcache); /* by index */

  fprintf(stream, "%s\n", (map->query.item)?map->query.item:"NULL"); /* by attribute */
  fprintf(stream, "%s\n", (map->query.str)?map->query.str:"NULL");

  fprintf(stream, "%d\n", map->query.op); /* by operator */

  if(map->query.shape) { /* by shape/by operator */
    int i, j;
    shapeObj *s = map->query.shape;

    fprintf(stream, "%d\n", s->numlines);
    for(i=0; i<s->numlines; i++) {
      fprintf(stream, "%d\n", s->line[i].numpoints);
      for(j=0; j<s->line[i].numpoints; j++)
        fprintf(stream, "%.15g %.15g\n", s->line[i].point[j].x, s->line[i].point[j].y);
    }
  } else {
    fprintf(stream, "0\n"); /* no lines/parts */
  }

  fclose(stream);
  return(MS_SUCCESS); 
}

int msLoadQuery(mapObj *map, char *filename) {
  FILE *stream;
  char buffer[MS_BUFFER_LENGTH];
  int lineno;

  int numlines, numpoints;

  if(!filename) {
    msSetError(MS_MISCERR, "No filename provided to load query from.", "msLoadQuery()");
    return(MS_FAILURE);
  }

  /* 
  ** Make sure the file at least has the right extension. 
  */ 
  if(msEvalRegex("\\.qy$", filename) != MS_TRUE) {
    msSetError(MS_MISCERR, "Query file extension check failed on %s (extension must be .qy)", "msLoadQuery()", filename);
    return MS_FAILURE; 
  }

  stream = fopen(filename, "r");
  if(!stream) {
    msSetError(MS_IOERR, "(%s)", "msLoadQuery()", filename);
    return(MS_FAILURE);
  }

  /*
  ** Make sure the query magic string is the first line.
  */
  if(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) {
    if(!msCaseFindSubstring(buffer, MS_QUERY_MAGIC_STRING)) {
      msSetError(MS_WEBERR, "Missing magic string, %s doesn't look like a MapServer query file.", "msLoadQuery()", filename);
      fclose(stream);
      return MS_FAILURE;
    }
  }

  msInitQuery(&(map->query)); /* this will free any previous query as well */

  lineno = 2; /* line 1 is the magic string */
  while(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) {
    switch(lineno) {
    case 2:
      if(sscanf(buffer, "%d %d %d %d\n", &(map->query.mode), &(map->query.type), &(map->query.layer), &(map->query.slayer)) != 4) goto parse_error;
      break;
    case 3:
      if(sscanf(buffer, "%lf %lf %lf %d\n", &map->query.point.x, &map->query.point.y, &map->query.buffer, &map->query.maxresults) != 4) goto parse_error;
      break;
    case 4:
      if(sscanf(buffer, "%lf %lf %lf %lf\n", &map->query.rect.minx, &map->query.rect.miny, &map->query.rect.maxx, &map->query.rect.maxy) != 4) goto parse_error;
      break;
    case 5:
      if(sscanf(buffer, "%ld %ld %d\n", &map->query.shapeindex, &map->query.tileindex, &map->query.clear_resultcache) != 3) goto parse_error;
      break;
    case 6:
      if(strncmp(buffer, "NULL", 4) != 0) {
        map->query.item = strdup(buffer);
        msStringChop(map->query.item);
      }
      break;
    case 7:
      if(strncmp(buffer, "NULL", 4) != 0) {
        map->query.str = strdup(buffer);
        msStringChop(map->query.str);
      }
      break;
    case 8:
      if(sscanf(buffer, "%d\n", &map->query.op) != 1) goto parse_error;
      break;
    case 9:
      if(sscanf(buffer, "%d\n", &numlines) != 1) goto parse_error;

      if(numlines > 0) { /* load the rest of the shape */
        int i, j;
        lineObj line;

        map->query.shape = (shapeObj *) malloc(sizeof(shapeObj));
        msInitShape(map->query.shape);
        map->query.shape->type = MS_SHAPE_POLYGON; /* for now we just support polygons */

	for(i=0; i<numlines; i++) {
          if(fscanf(stream, "%d\n", &numpoints) != 1) goto parse_error;

	  line.numpoints = numpoints;
	  line.point = (pointObj *) malloc(line.numpoints*sizeof(pointObj));

          for(j=0; j<numpoints; j++)
            if(fscanf(stream, "%lf %lf\n", &line.point[j].x, &line.point[j].y) != 2) goto parse_error;

          msAddLine(map->query.shape, &line);
          free(line.point);
        }
      }
      break;
    default:
      break;
    }

    lineno++;
  }

  /* TODO: should we throw an error if lineno != 10? */

  /* force layer and slayer on */
  if(map->query.layer >= 0 && map->query.layer < map->numlayers) GET_LAYER(map, map->query.layer)->status = MS_ON;
  if(map->query.slayer >= 0 && map->query.slayer < map->numlayers) GET_LAYER(map, map->query.slayer)->status = MS_ON;

  fclose(stream);

  /* now execute the query */
  return msExecuteQuery(map);

  parse_error:
    msSetError(MS_MISCERR, "Parse error line %d.", "msLoadQuery()", lineno);
    fclose(stream);
    return MS_FAILURE;
}

int msQueryByIndex(mapObj *map)
{
  layerObj *lp;
  int status;

  shapeObj shape;

  if(map->query.type != MS_QUERY_BY_INDEX) {
    msSetError(MS_QUERYERR, "The query is not properly defined.", "msQueryByIndex()");
    return(MS_FAILURE);
  }

  if(map->query.layer < 0 || map->query.layer >= map->numlayers) {
    msSetError(MS_QUERYERR, "No query layer defined.", "msQueryByIndex()"); 
    return(MS_FAILURE);
  }

  lp = (GET_LAYER(map, map->query.layer));

  if(!msIsLayerQueryable(lp)) {
    msSetError(MS_QUERYERR, "Requested layer has no templates defined.", "msQueryByIndex()"); 
    return(MS_FAILURE);
  }

  if(map->query.clear_resultcache) {
    if(lp->resultcache) {
      if(lp->resultcache->results) free(lp->resultcache->results);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }
  }

  /* open this layer */
  status = msLayerOpen(lp);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  /* build item list, we want *all* items */
  status = msLayerWhichItems(lp, MS_TRUE, NULL);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  if(map->query.clear_resultcache || lp->resultcache == NULL) {
    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    initResultCache( lp->resultcache);
  }

  msInitShape(&shape);
  status = msLayerGetShape(lp, &shape, map->query.tileindex, map->query.shapeindex);
  if(status != MS_SUCCESS) {
    msSetError(MS_NOTFOUND, "Not valid record request.", "msQueryByIndex()"); 
    return(MS_FAILURE);
  }

  shape.classindex = msShapeGetClass(lp, &shape, map->scaledenom, NULL, 0);
  if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
    msSetError(MS_NOTFOUND, "Requested shape not valid against layer classification scheme.", "msQueryByIndex()");
    msFreeShape(&shape);
    msLayerClose(lp);
    return(MS_FAILURE);
  }

  if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
    msSetError(MS_NOTFOUND, "Requested shape does not have a valid template, no way to present results.", "msQueryByIndex()");
    msFreeShape(&shape);
    msLayerClose(lp);
    return(MS_FAILURE);
  }

  addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
  if(lp->resultcache->numresults == 1)
    lp->resultcache->bounds = shape.bounds;
  else
    msMergeRect(&(lp->resultcache->bounds), &shape.bounds);

  msFreeShape(&shape);
  /* msLayerClose(lp); */

  return(MS_SUCCESS);
}

void msRestoreOldFilter(layerObj *lp, int old_filtertype, char *old_filteritem, char *old_filterstring)
{
  freeExpression(&(lp->filter));
  if(lp->filteritem) {
    free(lp->filteritem);
    lp->filteritem = NULL;
    lp->filteritemindex = -1;
  }
  
  /* restore any previously defined filter */
  if(old_filterstring) {
    lp->filter.type = old_filtertype;
    lp->filter.string = old_filterstring;
    if(old_filteritem) { 
      lp->filteritem = old_filteritem;
    }
  }
}

int msQueryByAttributes(mapObj *map)
{
  layerObj *lp;
  int status;
  
  int old_filtertype=-1;
  char *old_filterstring=NULL, *old_filteritem=NULL;

  rectObj searchrect;

  shapeObj shape;
  
  int nclasses = 0;
  int *classgroup = NULL;

  if(map->query.type != MS_QUERY_BY_ATTRIBUTE) {
    msSetError(MS_QUERYERR, "The query is not properly defined.", "msQueryByAttribute()");
    return(MS_FAILURE);
  }

  if(map->query.layer < 0 || map->query.layer >= map->numlayers) {
    msSetError(MS_MISCERR, "No query layer defined.", "msQueryByAttributes()"); 
    return(MS_FAILURE);
  }

  lp = (GET_LAYER(map, map->query.layer));

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

  if(!map->query.str) {
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
  if(map->query.item && map->query.item[0] != '\0') 
    lp->filteritem = strdup(map->query.item);
  else
    lp->filteritem = NULL;
  msLoadExpressionString(&(lp->filter), map->query.str);

  msInitShape(&shape);

  /* open this layer */
  status = msLayerOpen(lp);
  if(status != MS_SUCCESS) {
    msRestoreOldFilter(lp, old_filtertype, old_filteritem, old_filterstring); /* manually reset the filter */
    return(MS_FAILURE);
  }
  
  /* build item list, we want *all* items */
  status = msLayerWhichItems(lp, MS_TRUE, NULL);
  if(status != MS_SUCCESS) {
    msRestoreOldFilter(lp, old_filtertype, old_filteritem, old_filterstring); /* manually reset the filter */
    return(MS_FAILURE);
  }

  /* identify candidate shapes */
  searchrect = map->query.rect;
#ifdef USE_PROJ  
  if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))  
    msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */
  else
    lp->project = MS_FALSE;
#endif

  status = msLayerWhichShapes(lp, searchrect);
  if(status == MS_DONE) { /* no overlap */
    msRestoreOldFilter(lp, old_filtertype, old_filteritem, old_filterstring); /* manually reset the filter */
    msLayerClose(lp);
    msSetError(MS_NOTFOUND, "No matching record(s) found, layer and area of interest do not overlap.", "msQueryByAttributes()");
    return(MS_FAILURE);
  } else if(status != MS_SUCCESS) {
    msRestoreOldFilter(lp, old_filtertype, old_filteritem, old_filterstring); /* manually reset the filter */
    msLayerClose(lp);
    return(MS_FAILURE);
  }

  lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
  initResultCache( lp->resultcache);
  
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

    addResult(lp->resultcache, shape.classindex, shape.index, shape.tileindex);
    
    if(lp->resultcache->numresults == 1)
      lp->resultcache->bounds = shape.bounds;
    else
      msMergeRect(&(lp->resultcache->bounds), &shape.bounds);
    
    msFreeShape(&shape);

    if(map->query.mode == MS_QUERY_SINGLE) { /* no need to look any further */
      status = MS_DONE;
      break;
    }
  }

  if (classgroup)
    msFree(classgroup);

  msRestoreOldFilter(lp, old_filtertype, old_filteritem, old_filterstring); /* manually reset the filter */

  if(status != MS_DONE) {
    msLayerClose(lp);
    return(MS_FAILURE);
  }

  /* was anything found? (if yes, don't close the layer) */
  if(lp->resultcache && lp->resultcache->numresults > 0)
    return(MS_SUCCESS);

  msLayerClose(lp);
  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByAttributes()"); 
  return(MS_FAILURE);
}

int msQueryByRect(mapObj *map) 
{
  int l; /* counters */
  int start, stop=0;

  layerObj *lp;

  char status;
  shapeObj shape, searchshape;
  rectObj searchrect;
  
  int nclasses = 0;
  int *classgroup = NULL;

  if(map->query.type != MS_QUERY_BY_RECT) {
    msSetError(MS_QUERYERR, "The query is not properly defined.", "msQueryByRect()");
    return(MS_FAILURE);
  }

  msInitShape(&shape);
  msInitShape(&searchshape);

  msRectToPolygon(map->query.rect, &searchshape);

  if(map->query.layer < 0 || map->query.layer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = map->query.layer;

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
      if( msRasterQueryByRect( map, lp, map->query.rect ) == MS_FAILURE)
        return MS_FAILURE;

      continue;
    }

    /* open this layer */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* build item list, we want *all* items */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* identify candidate shapes */
    searchrect = map->query.rect;
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
    initResultCache( lp->resultcache);

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

      if(msRectContained(&shape.bounds, &(map->query.rect)) == MS_TRUE) { /* if the whole shape is in, don't intersect */	
        status = MS_TRUE;
      } else {
	switch(shape.type) { /* make sure shape actually intersects the qrect (ADD FUNCTIONS SPECIFIC TO RECTOBJ) */
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

    if(lp->resultcache->numresults == 0) msLayerClose(lp); /* no need to keep the layer open */
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

int msQueryByFeatures(mapObj *map)
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

  if(map->debug) msDebug("in msQueryByFeatures()\n");

  /* is the selection layer valid and has it been queried */
  if(map->query.slayer < 0 || map->query.slayer >= map->numlayers) {
    msSetError(MS_QUERYERR, "Invalid selection layer index.", "msQueryByFeatures()");
    return(MS_FAILURE);
  }
  slp = (GET_LAYER(map, map->query.slayer));
  if(!slp->resultcache) {
    msSetError(MS_QUERYERR, "Selection layer has not been queried.", "msQueryByFeatures()");
    return(MS_FAILURE);
  }

  /* conditions may have changed since this layer last drawn, so set 
     layer->project true to recheck projection needs (Bug #673) */ 
  slp->project = MS_TRUE; 

  if(map->query.layer < 0 || map->query.layer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = map->query.layer;

  /* selection layers should already be open */
  /* status = msLayerOpen(slp);
  if(status != MS_SUCCESS) return(MS_FAILURE); */

  msInitShape(&shape); /* initialize a few things */
  msInitShape(&selectshape);

  for(l=start; l>=stop; l--) {
    if(l == map->query.slayer) continue; /* skip the selection layer */
    
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

    /* Get the layer tolerance default is 3 for point and line layers, 0 for others */
    if(lp->tolerance == -1) {
      if(lp->type == MS_LAYER_POINT || lp->type == MS_LAYER_LINE)
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
    
    /* build item list, we want *all* items */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);
    
    /* for each selection shape */
    for(i=0; i<slp->resultcache->numresults; i++) {

      status = msLayerResultsGetShape(slp, &selectshape, slp->resultcache->results[i].tileindex, slp->resultcache->results[i].shapeindex);
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

      searchrect.minx -= tolerance; /* expand the search box to account for layer tolerances (e.g. buffered searches) */
      searchrect.maxx += tolerance;
      searchrect.miny -= tolerance;
      searchrect.maxy += tolerance;

#ifdef USE_PROJ
      if(lp->project && msProjectionsDiffer(&(lp->projection), &(map->projection)))      
	msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */
      else
	lp->project = MS_FALSE;
#endif

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
        initResultCache( lp->resultcache);
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

    if(lp->resultcache->numresults == 0) msLayerClose(lp); /* no need to keep the layer open */
  } /* next layer */

  /* was anything found? */
  for(l=start; l>=stop; l--) {  
    if(l == map->query.slayer) continue; /* skip the selection layer */
    if(GET_LAYER(map, l)->resultcache && GET_LAYER(map, l)->resultcache->numresults > 0) return(MS_SUCCESS);
  }

  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByFeatures()"); 
  return(MS_FAILURE);
}

/* msQueryByPoint()
 *
 * With mode=MS_QUERY_SINGLE:
 *   Set maxresults = 0 to have a single result across all layers (the closest
 *     shape from the first layer that finds a match).
 *   Set maxresults = 1 to have up to one result per layer (the closest shape
 *     from each layer).
 *
 * With mode=MS_QUERY_MULTIPLE:
 *   Set maxresults = 0 to have an unlimited number of results.
 *   Set maxresults > 0 to limit the number of results per layer (the shapes
 *     returned are the first ones found in each layer and are not necessarily
 *     the closest ones).
 */
int msQueryByPoint(mapObj *map)
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

  if(map->query.type != MS_QUERY_BY_POINT) {
    msSetError(MS_QUERYERR, "The query is not properly defined.", "msQueryByPoint()");
    return(MS_FAILURE);
  }

  msInitShape(&shape);

  if(map->query.layer < 0 || map->query.layer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = map->query.layer;

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
      if( msRasterQueryByPoint( map, lp, map->query.mode, map->query.point, map->query.buffer, map->query.maxresults ) == MS_FAILURE )
        return MS_FAILURE;
      continue;
    }

    /* Get the layer tolerance default is 3 for point and line layers, 0 for others */
    if(lp->tolerance == -1) {
      if(lp->type == MS_LAYER_POINT || lp->type == MS_LAYER_LINE)
        layer_tolerance = 3;
      else
        layer_tolerance = 0;
    } else
      layer_tolerance = lp->tolerance;

    if(map->query.buffer <= 0) { /* use layer tolerance */
      if(lp->toleranceunits == MS_PIXELS)
	t = layer_tolerance * msAdjustExtent(&(map->extent), map->width, map->height);
      else
	t = layer_tolerance * (msInchesPerUnit(lp->toleranceunits,0)/msInchesPerUnit(map->units,0));
    } else /* use buffer distance */
      t = map->query.buffer;

    rect.minx = map->query.point.x - t;
    rect.maxx = map->query.point.x + t;
    rect.miny = map->query.point.y - t;
    rect.maxy = map->query.point.y + t;

    /* open this layer */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* build item list, we want *all* items */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
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
    initResultCache( lp->resultcache);

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

      d = msDistancePointToShape(&(map->query.point), &shape);
      if( d <= t ) { /* found one */
	if(map->query.mode == MS_QUERY_SINGLE) {
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

      if(map->query.mode == MS_QUERY_MULTIPLE && map->query.maxresults > 0 && lp->resultcache->numresults == map->query.maxresults) {
        status = MS_DONE;   /* got enough results for this layer */
        break;
      }
    } /* next shape */

    if (classgroup)
      msFree(classgroup);

    if(status != MS_DONE) return(MS_FAILURE);

    if(lp->resultcache->numresults == 0) msLayerClose(lp); /* no need to keep the layer open */

    if((lp->resultcache->numresults > 0) && (map->query.mode == MS_QUERY_SINGLE) && (map->query.maxresults == 0)) 
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

int msQueryByShape(mapObj *map)
{
  int start, stop=0, l;
  shapeObj shape, *qshape=NULL;
  layerObj *lp;
  char status;
  double distance, tolerance, layer_tolerance;
  rectObj searchrect;

  int nclasses = 0;
  int *classgroup = NULL;

  if(map->query.type != MS_QUERY_BY_SHAPE) {
    msSetError(MS_QUERYERR, "The query is not properly defined.", "msQueryByShape()");
    return(MS_FAILURE);
  }

  if(!(map->query.shape)) {
    msSetError(MS_QUERYERR, "Query shape is not defined.", "msQueryByShape()");
    return(MS_FAILURE);
  }
  if(map->query.shape->type != MS_SHAPE_POLYGON) {
    msSetError(MS_QUERYERR, "Query shape MUST be a polygon.", "msQueryByShape()"); 
    return(MS_FAILURE);
  }

  msInitShape(&shape);
  qshape = map->query.shape; /* for brevity */

  if(map->query.layer < 0 || map->query.layer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = map->query.layer;

  msComputeBounds(qshape); /* make sure an accurate extent exists */
 
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
    if( lp->type == MS_LAYER_RASTER ) {
      if( msRasterQueryByShape(map, lp, qshape) == MS_FAILURE )
        return MS_FAILURE;
      continue;
    }

    /* Get the layer tolerance default is 3 for point and line layers, 0 for others */
    if(lp->tolerance == -1) {
      if(lp->type == MS_LAYER_POINT || lp->type == MS_LAYER_LINE)
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

    /* build item list, we want *all* items */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* identify target shapes */
    searchrect = qshape->bounds;

    searchrect.minx -= tolerance; /* expand the search box to account for layer tolerances (e.g. buffered searches) */
    searchrect.maxx += tolerance;
    searchrect.miny -= tolerance;
    searchrect.maxy += tolerance;

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
    initResultCache( lp->resultcache);

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
	  status = msIntersectMultipointPolygon(&shape, qshape);
	else { /* check distance, distance=0 means they intersect */
	  distance = msDistanceShapeToShape(qshape, &shape);
	  if(distance < tolerance) status = MS_TRUE;
        }
	break;
      case MS_SHAPE_LINE:
        if(tolerance == 0) { /* just test for intersection */
	  status = msIntersectPolylinePolygon(&shape, qshape);
	} else { /* check distance, distance=0 means they intersect */
	  distance = msDistanceShapeToShape(qshape, &shape);
	  if(distance < tolerance) status = MS_TRUE;
        }
	break;
      case MS_SHAPE_POLYGON:	
	if(tolerance == 0) /* just test for intersection */
	  status = msIntersectPolygons(&shape, qshape);
	else { /* check distance, distance=0 means they intersect */
	  distance = msDistanceShapeToShape(qshape, &shape);
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

    if(lp->resultcache->numresults == 0) msLayerClose(lp); /* no need to keep the layer open */
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
/*      query using a shape and a valid operator : all queries are      */
/*      done using geos.                                                */
/************************************************************************/
int msQueryByOperator(mapObj *map)
{
#ifdef USE_GEOS
  int start, stop=0, l;
  shapeObj shape, *qshape=NULL;
  layerObj *lp;
  int status;
  rectObj searchrect;
     
  int nclasses = 0;
  int *classgroup = NULL;
  double dfValue;

  if(map->query.type != MS_QUERY_BY_OPERATOR) {
    msSetError(MS_QUERYERR, "The query is not properly defined.", "msQueryByOperator()");
    return(MS_FAILURE);
  }

  msInitShape(&shape);
  qshape = map->query.shape; /* for brevity */

  /* this should not be a necessary test for queries using geos */
  /*
    if(selectshape->type != MS_SHAPE_POLYGON) {
      msSetError(MS_QUERYERR, "Search shape MUST be a polygon.", "msQueryByShape()"); 
      return(MS_FAILURE);
    }
  */

  if(map->query.layer < 0 || map->query.layer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = map->query.layer;

  msComputeBounds(qshape); /* make sure an accurate extent exists */
 
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
    if( lp->type == MS_LAYER_RASTER ) {
      if( msRasterQueryByShape(map, lp, qshape) == MS_FAILURE )
        return MS_FAILURE;
      continue;
    }
    
    /* open this layer */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* build item list, we want *all* items */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* identify target shapes */
    searchrect = qshape->bounds;

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
    initResultCache( lp->resultcache);

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

      switch(map->query.op) 
      { 
        case MS_GEOS_EQUALS:
          status = msGEOSEquals(&shape, qshape);
          if (status != MS_TRUE && status != MS_FALSE)
            status = MS_FALSE;
          break;

        case MS_GEOS_DISJOINT:
          status = msGEOSDisjoint(&shape, qshape);
          if (status != MS_TRUE && status != MS_FALSE)
            status = MS_FALSE;
          break;

        case MS_GEOS_TOUCHES:
          status = msGEOSTouches(&shape, qshape);
          if (status != MS_TRUE && status != MS_FALSE)
            status = MS_FALSE;
          break;

        case MS_GEOS_OVERLAPS:
          status = msGEOSOverlaps(&shape, qshape);
          if (status != MS_TRUE && status != MS_FALSE)
            status = MS_FALSE;
          break;

        case MS_GEOS_CROSSES:
          status = msGEOSCrosses(&shape, qshape);
          if (status != MS_TRUE && status != MS_FALSE)
            status = MS_FALSE;
          break;

        case MS_GEOS_INTERSECTS:
          status = msGEOSIntersects(&shape, qshape);
          if (status != MS_TRUE && status != MS_FALSE)
            status = MS_FALSE;
          break;

        case MS_GEOS_WITHIN:
          status = msGEOSWithin(&shape, qshape);
          if (status != MS_TRUE && status != MS_FALSE)
            status = MS_FALSE;
          break;

        case MS_GEOS_CONTAINS:
          status = msGEOSContains(qshape, &shape);
          if (status != MS_TRUE && status != MS_FALSE)
            status = MS_FALSE;
          break;

        /* beyond is opposite of dwithin use in filter encoding see tickets #2105 and #2564 */
        case MS_GEOS_BEYOND:
          status = MS_FALSE;
          dfValue = msGEOSDistance(&shape, qshape);
          if (dfValue > 0.0)
            status = MS_TRUE;
          break;

        /* dwithin is used with filter encoding (#2564) */
        case MS_GEOS_DWITHIN:
          status = MS_FALSE;
          dfValue = msGEOSDistance(&shape, qshape);
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

    if(lp->resultcache->numresults == 0) msLayerClose(lp); /* no need to keep the layer open */
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

/* TODO: Rename this msFreeResultSet() or something along those lines... */
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

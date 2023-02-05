/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Layer query support.
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
#include "mapows.h"

/* This object is used by the various mapQueryXXXXX() functions. It stores
 * the total amount of shapes and their RAM footprint, when they are cached
 * in the resultCacheObj* of layers. This is the total number accross all queried
 * layers. However this is isn't persistant accross several calls to
 * mapQueryXXXXX(), if the resultCacheObj* objects weren't cleaned up. This
 * is not needed in the context of WFS, for which this is used for now.
 */
typedef struct
{
    int cachedShapeCountWarningEmitted;
    int cachedShapeCount;
    int cachedShapeRAMWarningEmitted;
    int cachedShapeRAM;
} queryCacheObj;

int msInitQuery(queryObj *query) {
  if(!query) return MS_FAILURE;

  msFreeQuery(query); /* clean up anything previously allocated */

  query->type = MS_QUERY_IS_NULL; /* nothing defined */
  query->mode = MS_QUERY_SINGLE;

  query->layer = query->slayer = -1;

  query->point.x = query->point.y = -1;
  query->buffer = 0.0;
  query->maxresults = 0; /* only used by msQueryByPoint() apparently */

  query->rect.minx = query->rect.miny = query->rect.maxx = query->rect.maxy = -1;
  query->shape = NULL;

  query->shapeindex = query->tileindex = -1;
  query->clear_resultcache = MS_TRUE; /* index queries allow the old results to persist */

  query->maxfeatures = -1;
  query->startindex = -1;
  query->only_cache_result_count = 0;
  
  query->filteritem = NULL;
  msInitExpression(&query->filter);
  
  query->cache_shapes = MS_FALSE;
  query->max_cached_shape_count = 0;
  query->max_cached_shape_ram_amount = 0;

  return MS_SUCCESS;
}

void msFreeQuery(queryObj *query)
{
  if(query->shape) {
    msFreeShape(query->shape);
    free(query->shape);
  }

  if(query->filteritem) free(query->filteritem);
  msFreeExpression(&query->filter);
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
    case MS_QUERY_BY_FILTER:
      status = msQueryByFilter(map);
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

static void initQueryCache(queryCacheObj* queryCache)
{
    queryCache->cachedShapeCountWarningEmitted = MS_FALSE;
    queryCache->cachedShapeCount = 0;
    queryCache->cachedShapeRAMWarningEmitted = MS_FALSE;
    queryCache->cachedShapeRAM = 0;
}

/** Check whether we should store the shape in resultCacheObj* given the
 * limits allowed in map->query.max_cached_shape_count and
 * map->query.max_cached_shape_ram_amount.
 */
static int canCacheShape(mapObj* map, queryCacheObj *queryCache,
                         shapeObj* shape, int shape_ram_size)
{
  if( !map->query.cache_shapes )
      return MS_FALSE;
  if( queryCache->cachedShapeCountWarningEmitted ||
      (map->query.max_cached_shape_count > 0 &&
      queryCache->cachedShapeCount >= map->query.max_cached_shape_count) )
  {
      if( !queryCache->cachedShapeCountWarningEmitted )
      {
          queryCache->cachedShapeCountWarningEmitted = MS_TRUE;
          msDebug("map->query.max_cached_shape_count = %d reached. "
                  "Next features will not be cached.\n",
                  map->query.max_cached_shape_count);
      }
      return MS_FALSE;
  }
  if( queryCache->cachedShapeRAMWarningEmitted ||
      (map->query.max_cached_shape_ram_amount > 0 &&
       queryCache->cachedShapeRAM + shape_ram_size > map->query.max_cached_shape_ram_amount) )
  {
      if( !queryCache->cachedShapeRAMWarningEmitted )
      {
          queryCache->cachedShapeRAMWarningEmitted = MS_TRUE;
          msDebug("map->query.max_cached_shape_ram_amount = %d reached after %d cached features. "
                  "Next features will not be cached.\n",
                  map->query.max_cached_shape_ram_amount,
                  queryCache->cachedShapeCount);
      }
      return MS_FALSE;
  }
  return MS_TRUE;
}

static int addResult(mapObj* map, resultCacheObj *cache,
                     queryCacheObj* queryCache, shapeObj *shape)
{
  int i;
  int shape_ram_size = (map->query.max_cached_shape_ram_amount > 0) ? 
                                            msGetShapeRAMSize( shape ) : 0;
  int store_shape = canCacheShape (map, queryCache, shape, shape_ram_size);

  if(cache->numresults == cache->cachesize) { /* just add it to the end */
    if(cache->cachesize == 0)
      cache->results = (resultObj *) malloc(sizeof(resultObj)*MS_RESULTCACHEINCREMENT);
    else
      cache->results = (resultObj *) realloc(cache->results, sizeof(resultObj)*(cache->cachesize+MS_RESULTCACHEINCREMENT));
    if(!cache->results) {
      msSetError(MS_MEMERR, "Realloc() error.", "addResult()");
      return(MS_FAILURE);
    }
    cache->cachesize += MS_RESULTCACHEINCREMENT;
  }

  i = cache->numresults;

  cache->results[i].classindex = shape->classindex;
  cache->results[i].tileindex = shape->tileindex;
  cache->results[i].shapeindex = shape->index;
  cache->results[i].resultindex = shape->resultindex;
  if( store_shape )
  {
      cache->results[i].shape = (shapeObj*)msSmallMalloc(sizeof(shapeObj));
      msInitShape(cache->results[i].shape);
      msCopyShape(shape, cache->results[i].shape);
      queryCache->cachedShapeCount ++;
      queryCache->cachedShapeRAM += shape_ram_size;
  }
  else
      cache->results[i].shape = NULL;
  cache->numresults++;

  cache->previousBounds = cache->bounds;
  if(cache->numresults == 1)
    cache->bounds = shape->bounds;
  else
    msMergeRect(&(cache->bounds), &(shape->bounds));

  return(MS_SUCCESS);
}

/*
** Serialize a query result set to disk.
*/
static int saveQueryResults(mapObj *map, char *filename)
{
  FILE *stream;
  int i, j, n=0;

  if(!filename) {
    msSetError(MS_MISCERR, "No filename provided to save query results to.", "saveQueryResults()");
    return MS_FAILURE;
  }

  stream = fopen(filename, "w");
  if(!stream) {
    msSetError(MS_IOERR, "(%s)", "saveQueryResults()", filename);
    return MS_FAILURE;
  }

  fprintf(stream, "%s - Generated by msSaveQuery()\n", MS_QUERY_RESULTS_MAGIC_STRING);

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
        fwrite(&(GET_LAYER(map, i)->resultcache->results[j]), sizeof(resultObj), 1, stream); /* each result */
    }
  }

  fclose(stream);
  return MS_SUCCESS;
}

static int loadQueryResults(mapObj *map, FILE *stream)
{
  int i, j, k, n=0;

  if(1 != fread(&n, sizeof(int), 1, stream)) {
    msSetError(MS_MISCERR,"failed to read query count from query file stream", "loadQueryResults()");
    return MS_FAILURE;
  }

  /* now load the result set for each layer found in the query file */
  for(i=0; i<n; i++) {
    if(1 != fread(&j, sizeof(int), 1, stream)) { /* layer index */
      msSetError(MS_MISCERR,"failed to read layer index from query file stream", "loadQueryResults()");
      return MS_FAILURE;
    }

    if(j<0 || j>map->numlayers) {
      msSetError(MS_MISCERR, "Invalid layer index loaded from query file.", "loadQueryResults()");
      return MS_FAILURE;
    }

    /* inialize the results for this layer */
    GET_LAYER(map, j)->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    MS_CHECK_ALLOC(GET_LAYER(map, j)->resultcache, sizeof(resultCacheObj), MS_FAILURE);

    if(1 != fread(&(GET_LAYER(map, j)->resultcache->numresults), sizeof(int), 1, stream) || (GET_LAYER(map, j)->resultcache->numresults < 0)) { /* number of results */
      msSetError(MS_MISCERR,"failed to read number of results from query file stream", "loadQueryResults()");
      free(GET_LAYER(map, j)->resultcache);
      GET_LAYER(map, j)->resultcache = NULL;
      return MS_FAILURE;
    }
    GET_LAYER(map, j)->resultcache->cachesize = GET_LAYER(map, j)->resultcache->numresults;

    if(1 != fread(&(GET_LAYER(map, j)->resultcache->bounds), sizeof(rectObj), 1, stream)) { /* bounding box */
      msSetError(MS_MISCERR,"failed to read bounds from query file stream", "loadQueryResults()");
      free(GET_LAYER(map, j)->resultcache);
      GET_LAYER(map, j)->resultcache = NULL;
      return MS_FAILURE;
    }

    GET_LAYER(map, j)->resultcache->results = (resultObj *) malloc(sizeof(resultObj)*GET_LAYER(map, j)->resultcache->numresults);
    if (GET_LAYER(map, j)->resultcache->results == NULL) {
      msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n", "loadQueryResults()",
                 __FILE__, __LINE__, (unsigned int)(sizeof(resultObj)*GET_LAYER(map, j)->resultcache->numresults));
      free(GET_LAYER(map, j)->resultcache);
      GET_LAYER(map, j)->resultcache = NULL;
      return MS_FAILURE;
    }

    for(k=0; k<GET_LAYER(map, j)->resultcache->numresults; k++) {
      if(1 != fread(&(GET_LAYER(map, j)->resultcache->results[k]), sizeof(resultObj), 1, stream)) { /* each result */
        msSetError(MS_MISCERR,"failed to read result %d from query file stream", "loadQueryResults()", k);
        free(GET_LAYER(map, j)->resultcache->results);
        free(GET_LAYER(map, j)->resultcache);
        GET_LAYER(map, j)->resultcache = NULL;
        return MS_FAILURE;
      }
      if(!GET_LAYER(map, j)->tileindex) GET_LAYER(map, j)->resultcache->results[k].tileindex = -1; /* reset the tile index for non-tiled layers */
      GET_LAYER(map, j)->resultcache->results[k].resultindex = -1; /* all results loaded this way have a -1 result (set) index */
    }
  }

  return MS_SUCCESS;
}

/*
** Serialize the parameters necessary to duplicate a query to disk. (TODO: add filter query...)
*/
static int saveQueryParams(mapObj *map, char *filename)
{
  FILE *stream;

  if(!filename) {
    msSetError(MS_MISCERR, "No filename provided to save query to.", "saveQueryParams()");
    return MS_FAILURE;
  }

  stream = fopen(filename, "w");
  if(!stream) {
    msSetError(MS_IOERR, "(%s)", "saveQueryParams()", filename);
    return MS_FAILURE;
  }

  fprintf(stream, "%s - Generated by msSaveQuery()\n", MS_QUERY_PARAMS_MAGIC_STRING);

  fprintf(stream, "%d %d %d %d\n", map->query.mode, map->query.type, map->query.layer, map->query.slayer); /* all queries */
  fprintf(stream, "%.15g %.15g %g %d\n", map->query.point.x, map->query.point.y, map->query.buffer, map->query.maxresults); /* by point */
  fprintf(stream, "%.15g %.15g %.15g %.15g\n", map->query.rect.minx, map->query.rect.miny, map->query.rect.maxx, map->query.rect.maxy); /* by rect */
  fprintf(stream, "%ld %ld %d\n", map->query.shapeindex, map->query.tileindex, map->query.clear_resultcache); /* by index */

  fprintf(stream, "%s\n", (map->query.filteritem)?map->query.filteritem:"NULL"); /* by filter */
  fprintf(stream, "%s\n", (map->query.filter.string)?map->query.filter.string:"NULL");

  if(map->query.shape) { /* by shape */
    int i, j;
    shapeObj *s = map->query.shape;

    fprintf(stream, "%d\n", s->type);
    fprintf(stream, "%d\n", s->numlines);
    for(i=0; i<s->numlines; i++) {
      fprintf(stream, "%d\n", s->line[i].numpoints);
      for(j=0; j<s->line[i].numpoints; j++)
        fprintf(stream, "%.15g %.15g\n", s->line[i].point[j].x, s->line[i].point[j].y);
    }
  } else {
    fprintf(stream, "%d\n", MS_SHAPE_NULL); /* NULL shape */
  }

  fclose(stream);
  return MS_SUCCESS;
}

static int loadQueryParams(mapObj *map, FILE *stream)
{
  char buffer[MS_BUFFER_LENGTH];
  int lineno;

  int shapetype, numlines, numpoints;

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
          map->query.filteritem = msStrdup(buffer);
          msStringChop(map->query.filteritem);
        }
        break;
      case 7:
        if(strncmp(buffer, "NULL", 4) != 0)
	  msLoadExpressionString(&map->query.filter, buffer); /* chop buffer */
        break;
      case 8:
        if(sscanf(buffer, "%d\n", &shapetype) != 1) goto parse_error;

        if(shapetype != MS_SHAPE_NULL) { /* load the rest of the shape */
          int i, j;
          lineObj line;

          map->query.shape = (shapeObj *) msSmallMalloc(sizeof(shapeObj));
          msInitShape(map->query.shape);
          map->query.shape->type = shapetype;

          if(fscanf(stream, "%d\n", &numlines) != 1) goto parse_error;
          for(i=0; i<numlines; i++) {
            if(fscanf(stream, "%d\n", &numpoints) != 1 || numpoints<0) goto parse_error;

            line.numpoints = numpoints;
            line.point = (pointObj *) msSmallMalloc(line.numpoints*sizeof(pointObj));

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

  /* now execute the query */
  return msExecuteQuery(map);

parse_error:
  msSetError(MS_MISCERR, "Parse error line %d.", "loadQueryParameters()", lineno);
  return MS_FAILURE;
}

/*
** Save (serialize) a query to disk. There are two methods, one saves the query parameters and the other saves
** all the shape indexes. Note the latter can be very slow against certain data sources but has a certain usefulness
** on occation.
*/
int msSaveQuery(mapObj *map, char *filename, int results)
{
  if(results)
    return saveQueryResults(map, filename);
  else
    return saveQueryParams(map, filename);
}

/*
** Loads a query file contained either serialized 1) query parameters or 2) query results (e.g. indexes).
*/
int msLoadQuery(mapObj *map, char *filename)
{
  FILE *stream;
  char buffer[MS_BUFFER_LENGTH];
  int retval=MS_FAILURE;

  if(!filename) {
    msSetError(MS_MISCERR, "No filename provided to load query from.", "msLoadQuery()");
    return MS_FAILURE;
  }

  /*
  ** Make sure the file at least has the right extension.
  */
  if(msEvalRegex("\\.qy$", filename) != MS_TRUE) {
    msSetError(MS_MISCERR, "Queryfile %s has incorrect file extension.",  "msLoadQuery()", filename);
    return MS_FAILURE;
  }

  /*
  ** Open the file and inspect the first line.
  */
  stream = fopen(filename, "r");
  if(!stream) {
    msSetError(MS_IOERR, "(%s)", "msLoadQuery()", filename);
    return MS_FAILURE;
  }

  if(fgets(buffer, MS_BUFFER_LENGTH, stream) != NULL) {
    /*
    ** Call correct reader based on the magic string.
    */
    if(strncasecmp(buffer, MS_QUERY_RESULTS_MAGIC_STRING, strlen(MS_QUERY_RESULTS_MAGIC_STRING)) == 0) {
      retval = loadQueryResults(map, stream);
    } else if(strncasecmp(buffer, MS_QUERY_PARAMS_MAGIC_STRING, strlen(MS_QUERY_PARAMS_MAGIC_STRING)) == 0) {
      retval = loadQueryParams(map, stream);
    } else {
      msSetError(MS_WEBERR, "Missing magic string, %s doesn't look like a MapServer query file.", "msLoadQuery()", filename);
      retval = MS_FAILURE;
    }
  } else {
    msSetError(MS_WEBERR, "Empty file or failed read for %s.", "msLoadQuery()", filename);
    retval = MS_FAILURE;
  }

  fclose(stream);
  return retval;
}

int msQueryByIndex(mapObj *map)
{
  layerObj *lp;
  int status;

  resultObj record;
  queryCacheObj queryCache;

  shapeObj shape;
  double minfeaturesize = -1;

  initQueryCache(&queryCache);

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
      cleanupResultCache(lp->resultcache);
      free(lp->resultcache);
      lp->resultcache = NULL;
    }
  }

  msLayerClose(lp); /* reset */
  status = msLayerOpen(lp);
  if(status != MS_SUCCESS) return(MS_FAILURE);
  /* disable driver paging */
  msLayerEnablePaging(lp, MS_FALSE);

  /* build item list, we want *all* items */
  status = msLayerWhichItems(lp, MS_TRUE, NULL);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  if(map->query.clear_resultcache || lp->resultcache == NULL) {
    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    MS_CHECK_ALLOC(lp->resultcache, sizeof(resultCacheObj), MS_FAILURE);
    initResultCache( lp->resultcache);
  }

  msInitShape(&shape);

  record.resultindex = -1;
  record.shapeindex = map->query.shapeindex;
  record.tileindex = map->query.tileindex;

  status = msLayerGetShape(lp, &shape, &record);
  if(status != MS_SUCCESS) {
    msSetError(MS_NOTFOUND, "Not valid record request.", "msQueryByIndex()");
    return(MS_FAILURE);
  }

  /*
   * The resultindex is used to retrieve a specific item from the result cache.
   * Usually, the row number will be used as resultindex. But when working with
   * databases and querying a single result, the row number is typically 0 and
   * thus useless as the index in the result cache. See #4926 #4076. Only shape
   * files are considered to have consistent row numbers.
   */
  if ( !(lp->connectiontype == MS_SHAPEFILE || lp->connectiontype == MS_TILED_SHAPEFILE) ) {
    shape.resultindex = -1;
  }

  if (lp->minfeaturesize > 0)
    minfeaturesize = Pix2LayerGeoref(map, lp, lp->minfeaturesize);

  /* Check if the shape size is ok to be drawn */
  if ( (shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) && (minfeaturesize > 0) ) {

    if (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE) {
      msSetError(MS_NOTFOUND, "Requested shape not valid against layer minfeaturesize.", "msQueryByIndex()");
      msFreeShape(&shape);
      msLayerClose(lp);
      return(MS_FAILURE);
    }
  }

  shape.classindex = msShapeGetClass(lp, map, &shape, NULL, 0);
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
  
  addResult(map, lp->resultcache, &queryCache, &shape);

  msFreeShape(&shape);
  /* msLayerClose(lp); */

  return(MS_SUCCESS);
}

static char *filterTranslateToLogical(expressionObj *filter, char *filteritem) {
  char *string = NULL;

  if(filter->type == MS_STRING && filteritem) {
    string = msStrdup("'[");
    string = msStringConcatenate(string, filteritem);
    string = msStringConcatenate(string, "]'");
    if(filter->flags & MS_EXP_INSENSITIVE)
      string = msStringConcatenate(string, " =* '");
    else
      string = msStringConcatenate(string, " = '");
    string = msStringConcatenate(string, filter->string);
    string = msStringConcatenate(string, "'");
  } else if(filter->type == MS_REGEX && filteritem) {
    string = msStrdup("'[");
    string = msStringConcatenate(string, filteritem);
    string = msStringConcatenate(string, "]'");
    if(filter->flags & MS_EXP_INSENSITIVE)
      string = msStringConcatenate(string, " ~* '");
    else
      string = msStringConcatenate(string, " ~ '");
    string = msStringConcatenate(string, filter->string);
    string = msStringConcatenate(string, "'");
  } else if(filter->type == MS_EXPRESSION) {
    string = msStrdup(filter->string);
  } else {
    /* native expression, nothing we can do - sigh */
  }

  return string;
}

static expressionObj mergeFilters(expressionObj *filter1, char *filteritem1, expressionObj *filter2, char *filteritem2) {
  expressionObj filter;

  char *tmpstr1=NULL;
  char *tmpstr2=NULL;

  msInitExpression(&filter);
  filter.type = MS_EXPRESSION; /* we're building a logical expression */

  tmpstr1 = filterTranslateToLogical(filter1, filteritem1);
  if(!tmpstr1) return filter; /* should only happen if the filter was a native filter */  

  tmpstr2 = filterTranslateToLogical(filter2, filteritem2);
  if(!tmpstr2) {
    msFree(tmpstr1);
    return filter; /* should only happen if the filter was a native filter */
  }

  filter.string = msStrdup(tmpstr1);
  filter.string = msStringConcatenate(filter.string, " AND ");
  filter.string = msStringConcatenate(filter.string, tmpstr2);

  msFree(tmpstr1);
  msFree(tmpstr2);

  return filter;
}

/*
** Query using common expression syntax.
*/
int msQueryByFilter(mapObj *map)
{
  int l;
  int start, stop=0;

  layerObj *lp;

  char status;

  char *old_filteritem=NULL;
  expressionObj old_filter;

  rectObj search_rect;
  const rectObj invalid_rect = MS_INIT_INVALID_RECT;

  shapeObj shape;
  int paging;

  int nclasses = 0;
  int *classgroup = NULL;
  double minfeaturesize = -1;
  queryCacheObj queryCache;

  initQueryCache(&queryCache);

  if(map->query.type != MS_QUERY_BY_FILTER) {
    msSetError(MS_QUERYERR, "The query is not properly defined.", "msQueryByFilter()");
    return(MS_FAILURE);
  }
  if(!map->query.filter.string) {
    msSetError(MS_QUERYERR, "Filter is not set.", "msQueryByFilter()");
    return(MS_FAILURE);
  }

  // fprintf(stderr, "in msQueryByFilter: filter=%s, filteritem=%s\n", map->query.filter.string, map->query.filteritem);

  msInitShape(&shape);

  if(map->query.layer < 0 || map->query.layer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = map->query.layer;

  for(l=start; l>=stop; l--) {
    reprojectionObj* reprojector = NULL;

    lp = (GET_LAYER(map, l));
    if (map->query.maxfeatures == 0)
      break; /* nothing else to do */
    else if (map->query.maxfeatures > 0)
      lp->maxfeatures = map->query.maxfeatures;

    /* using mapscript, the map->query.startindex will be unset... */
    if (lp->startindex > 1 && map->query.startindex < 0)
      map->query.startindex = lp->startindex;
    
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
    if(lp->type == MS_LAYER_RASTER) continue; /* ok to skip? */

    if(map->scaledenom > 0) {
      if((lp->maxscaledenom > 0) && (map->scaledenom > lp->maxscaledenom)) continue;
      if((lp->minscaledenom > 0) && (map->scaledenom <= lp->minscaledenom)) continue;
    }

    if (lp->maxscaledenom <= 0 && lp->minscaledenom <= 0) {
      if((lp->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > lp->maxgeowidth)) continue;
      if((lp->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < lp->mingeowidth)) continue;
    }

    paging = msLayerGetPaging(lp);
    msLayerClose(lp); /* reset */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) goto query_error;
    msLayerEnablePaging(lp, paging);

    /* disable driver paging */
    // msLayerEnablePaging(lp, MS_FALSE);
        
    old_filteritem = lp->filteritem; /* cache the existing filter/filteritem */
    msInitExpression(&old_filter);
    msCopyExpression(&old_filter, &lp->filter);
        
    /*
    ** Set the lp->filter and lp->filteritem (may need to merge). Remember filters are *always* MapServer syntax.
    */
    lp->filteritem = map->query.filteritem; /* re-point lp->filteritem */
    if(old_filter.string != NULL) { /* need to merge filters to create one logical expression */
      msFreeExpression(&lp->filter);
      lp->filter = mergeFilters(&map->query.filter, map->query.filteritem, &old_filter, old_filteritem);      
      if(!lp->filter.string) {
	msSetError(MS_MISCERR, "Filter merge failed, able to process query.", "msQueryByFilter()");
        goto query_error;
      }      
    } else {
      msCopyExpression(&lp->filter, &map->query.filter); /* apply new filter */
    }

    /* build item list, we want *all* items, note this *also* build tokens for the layer filter */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
    if(status != MS_SUCCESS) goto query_error;

    search_rect = map->query.rect;

    /* If only result count is needed, we can use msLayerGetShapeCount() */
    /* that has optimizations to avoid retrieving individual features */
    if( map->query.only_cache_result_count &&
        lp->template != NULL && /* always TRUE for WFS case */
        lp->minfeaturesize <= 0 )
    {
      int bUseLayerSRS = MS_FALSE;
      int numFeatures = -1;

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR) || defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
      /* Optimization to detect the case where a WFS query uses in fact the */
      /* whole layer extent, but expressed in a map SRS different from the layer SRS */
      /* In the case, we can directly request against the layer extent in its native SRS */
      if( lp->project &&
          memcmp( &search_rect, &invalid_rect, sizeof(search_rect) ) != 0 &&
          msProjectionsDiffer(&(lp->projection), &(map->projection)) )
      {
        rectObj layerExtent;
        if ( msOWSGetLayerExtent(map, lp, "FO", &layerExtent) == MS_SUCCESS)
        {
            rectObj ext = layerExtent;
            ext.minx -= 1e-5;
            ext.miny -= 1e-5;
            ext.maxx += 1e-5;
            ext.maxy += 1e-5;
            msProjectRect(&(lp->projection), &(map->projection), &ext);
            if( fabs(ext.minx - search_rect.minx) <= 2e-5 &&
                fabs(ext.miny - search_rect.miny) <= 2e-5 &&
                fabs(ext.maxx - search_rect.maxx) <= 2e-5 &&
                fabs(ext.maxy - search_rect.maxy) <= 2e-5 )
            {
              bUseLayerSRS = MS_TRUE;
              numFeatures = msLayerGetShapeCount(lp, layerExtent, &(lp->projection));
            }
        }
      }
#endif

      if( !bUseLayerSRS )
          numFeatures = msLayerGetShapeCount(lp, search_rect, &(map->projection));
      if( numFeatures >= 0 )
      {
        lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
        MS_CHECK_ALLOC(lp->resultcache, sizeof(resultCacheObj), MS_FAILURE);
        initResultCache( lp->resultcache);
        lp->resultcache->numresults = numFeatures;
        if (!msLayerGetPaging(lp) && map->query.startindex > 1) {
           lp->resultcache->numresults -= (map->query.startindex-1);
        }

        lp->filteritem = old_filteritem; /* point back to original value */
        msCopyExpression(&lp->filter, &old_filter); /* restore old filter */
        msFreeExpression(&old_filter);

        continue;
      }
      // Fallback in case of error (should not happen normally)
    }

    lp->project = msProjectionsDiffer(&(lp->projection), &(map->projection));
    if(lp->project && memcmp( &search_rect, &invalid_rect, sizeof(search_rect) ) != 0 )
      msProjectRect(&(map->projection), &(lp->projection), &search_rect); /* project the searchrect to source coords */

    status = msLayerWhichShapes(lp, search_rect, MS_TRUE);
    if(status == MS_DONE) { /* no overlap */
      lp->filteritem = old_filteritem; /* point back to original value */
      msCopyExpression(&lp->filter, &old_filter); /* restore old filter */
      msFreeExpression(&old_filter);
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) goto query_error;

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    initResultCache( lp->resultcache);

    nclasses = 0;
    classgroup = NULL;
    if (lp->classgroup && lp->numclasses > 0)
      classgroup = msAllocateValidClassGroups(lp, &nclasses);

    if (lp->minfeaturesize > 0)
      minfeaturesize = Pix2LayerGeoref(map, lp, lp->minfeaturesize);

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes - if necessary the filter is applied in msLayerNextShape(...) */

       /* Check if the shape size is ok to be drawn */
      if ( (shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) && (minfeaturesize > 0) ) {
        if (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE) {
          if( lp->debug >= MS_DEBUGLEVEL_V )
            msDebug("msQueryByFilter(): Skipping shape (%ld) because LAYER::MINFEATURESIZE is bigger than shape size\n", shape.index);
          msFreeShape(&shape);
          continue;
        }
      }

      shape.classindex = msShapeGetClass(lp, map, &shape, classgroup, nclasses);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
        msFreeShape(&shape);
        continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
        msFreeShape(&shape);
        continue;
      }

      if(lp->project) {
        if( reprojector == NULL ) {
            reprojector = msProjectCreateReprojector(&(lp->projection), &(map->projection));
            if( reprojector == NULL ) {
              msFreeShape(&shape);
              status = MS_FAILURE;
              break;
            }
        }
        msProjectShapeEx(reprojector, &shape);
      }

      /* Should we skip this feature? */
      if (!paging && map->query.startindex > 1) {
        --map->query.startindex;
        msFreeShape(&shape);
        continue;
      }
    
      if( map->query.only_cache_result_count )
        lp->resultcache->numresults ++;
      else
        addResult(map, lp->resultcache, &queryCache, &shape);
      msFreeShape(&shape);

      if(map->query.mode == MS_QUERY_SINGLE) { /* no need to look any further */
	status = MS_DONE;
	break;
      }

      /* check shape count */
      if(lp->maxfeatures > 0 && lp->maxfeatures == lp->resultcache->numresults) {
        status = MS_DONE;
        break;
      }
    } /* next shape */

    if(classgroup) msFree(classgroup);

    lp->filteritem = old_filteritem; /* point back to original value */
    msCopyExpression(&lp->filter, &old_filter); /* restore old filter */
    msFreeExpression(&old_filter);

    msProjectDestroyReprojector(reprojector);

    if(status != MS_DONE) goto query_error;
    if(!map->query.only_cache_result_count && lp->resultcache->numresults == 0) 
      msLayerClose(lp); /* no need to keep the layer open */
  } /* next layer */

  /* was anything found? */
  for(l=start; l>=stop; l--) {
    if(GET_LAYER(map, l)->resultcache && GET_LAYER(map, l)->resultcache->numresults > 0)
      return MS_SUCCESS;
  }

  msSetError(MS_NOTFOUND, "No matching record(s) found.", "msQueryByFilter()");
  return MS_FAILURE;

query_error:
  // msFree(lp->filteritem);
  // lp->filteritem = old_filteritem;
  // msCopyExpression(&lp->filter, &old_filter); /* restore old filter */
  // msFreeExpression(&old_filter);
  // msLayerClose(lp);
  return MS_FAILURE;
}

int msQueryByRect(mapObj *map)
{
  int l; /* counters */
  int start, stop=0;

  layerObj *lp;

  char status;
  shapeObj shape, searchshape;
  rectObj searchrect, searchrectInMapProj;
  const rectObj invalid_rect = MS_INIT_INVALID_RECT;
  double layer_tolerance = 0, tolerance = 0;

  int paging;
  int nclasses = 0;
  int *classgroup = NULL;
  double minfeaturesize = -1;
  queryCacheObj queryCache;

  initQueryCache(&queryCache);

  if(map->query.type != MS_QUERY_BY_RECT) {
    msSetError(MS_QUERYERR, "The query is not properly defined.", "msQueryByRect()");
    return(MS_FAILURE);
  }

  msInitShape(&shape);
  msInitShape(&searchshape);

  if(map->query.layer < 0 || map->query.layer >= map->numlayers)
    start = map->numlayers-1;
  else
    start = stop = map->query.layer;

  for(l=start; l>=stop; l--) {
    reprojectionObj* reprojector = NULL;
    lp = (GET_LAYER(map, l));
    /* Set the global maxfeatures */
    if (map->query.maxfeatures == 0)
      break; /* nothing else to do */
    else if (map->query.maxfeatures > 0)
      lp->maxfeatures = map->query.maxfeatures;

    /* using mapscript, the map->query.startindex will be unset... */
    if (lp->startindex > 1 && map->query.startindex < 0)
      map->query.startindex = lp->startindex;
    
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

    searchrect = map->query.rect;
    if(lp->tolerance > 0) {
      layer_tolerance = lp->tolerance;

      if(lp->toleranceunits == MS_PIXELS)
        tolerance = layer_tolerance * msAdjustExtent(&(map->extent), map->width, map->height);
      else
        tolerance = layer_tolerance * (msInchesPerUnit(lp->toleranceunits,0)/msInchesPerUnit(map->units,0));

      searchrect.minx -= tolerance;
      searchrect.maxx += tolerance;
      searchrect.miny -= tolerance;
      searchrect.maxy += tolerance;
    }
    searchrectInMapProj = searchrect;

    msRectToPolygon(searchrect, &searchshape);

    /* Raster layers are handled specially. */
    if( lp->type == MS_LAYER_RASTER ) {
      if( msRasterQueryByRect( map, lp, searchrect ) == MS_FAILURE)
        return MS_FAILURE;

      continue;
    }

    /* Paging could have been disabled before */
    paging = msLayerGetPaging(lp);
    msLayerClose(lp); /* reset */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) {
      msFreeShape(&searchshape);
      return(MS_FAILURE);
    }
    msLayerEnablePaging(lp, paging);

    /* build item list, we want *all* items */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
    if(status != MS_SUCCESS) {
      msFreeShape(&searchshape);
      return(MS_FAILURE);
    }

    /* If only result count is needed, we can use msLayerGetShapeCount() */
    /* that has optimizations to avoid retrieving individual features */
    if( map->query.only_cache_result_count &&
        lp->template != NULL && /* always TRUE for WFS case */
        lp->minfeaturesize <= 0 )
    {
      int bUseLayerSRS = MS_FALSE;
      int numFeatures = -1;

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR) || defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
      /* Optimization to detect the case where a WFS query uses in fact the */
      /* whole layer extent, but expressed in a map SRS different from the layer SRS */
      /* In the case, we can directly request against the layer extent in its native SRS */
      if( lp->project &&
          memcmp( &searchrect, &invalid_rect, sizeof(searchrect) ) != 0 &&
          msProjectionsDiffer(&(lp->projection), &(map->projection)) )
      {
        rectObj layerExtent;
        if ( msOWSGetLayerExtent(map, lp, "FO", &layerExtent) == MS_SUCCESS)
        {
            rectObj ext = layerExtent;
            ext.minx -= 1e-5;
            ext.miny -= 1e-5;
            ext.maxx += 1e-5;
            ext.maxy += 1e-5;
            msProjectRect(&(lp->projection), &(map->projection), &ext);
            if( fabs(ext.minx - searchrect.minx) <= 2e-5 &&
                fabs(ext.miny - searchrect.miny) <= 2e-5 &&
                fabs(ext.maxx - searchrect.maxx) <= 2e-5 &&
                fabs(ext.maxy - searchrect.maxy) <= 2e-5 )
            {
              bUseLayerSRS = MS_TRUE;
              numFeatures = msLayerGetShapeCount(lp, layerExtent, &(lp->projection));
            }
        }
      }
#endif

      if( !bUseLayerSRS )
          numFeatures = msLayerGetShapeCount(lp, searchrect, &(map->projection));
      if( numFeatures >= 0 )
      {
        lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
        MS_CHECK_ALLOC(lp->resultcache, sizeof(resultCacheObj), MS_FAILURE);
        initResultCache( lp->resultcache);
        lp->resultcache->numresults = numFeatures;
        if (!paging && map->query.startindex > 1) {
           lp->resultcache->numresults -= (map->query.startindex-1);
        }
        msFreeShape(&searchshape);
        continue;
      }
      // Fallback in case of error (should not happen normally)
    }

    lp->project = msProjectionsDiffer(&(lp->projection), &(map->projection));
    if(lp->project &&
       memcmp( &searchrect, &invalid_rect, sizeof(searchrect) ) != 0 )
      msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */

    status = msLayerWhichShapes(lp, searchrect, MS_TRUE);
    if(status == MS_DONE) { /* no overlap */
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      msFreeShape(&searchshape);
      return(MS_FAILURE);
    }

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    MS_CHECK_ALLOC(lp->resultcache, sizeof(resultCacheObj), MS_FAILURE);
    initResultCache( lp->resultcache);

    nclasses = 0;
    classgroup = NULL;
    if (lp->classgroup && lp->numclasses > 0)
      classgroup = msAllocateValidClassGroups(lp, &nclasses);

    if (lp->minfeaturesize > 0)
      minfeaturesize = Pix2LayerGeoref(map, lp, lp->minfeaturesize);

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

      /* Check if the shape size is ok to be drawn */
      if ( (shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) && (minfeaturesize > 0) ) {
        if (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE) {
          if( lp->debug >= MS_DEBUGLEVEL_V )
            msDebug("msQueryByRect(): Skipping shape (%ld) because LAYER::MINFEATURESIZE is bigger than shape size\n", shape.index);
          msFreeShape(&shape);
          continue;
        }
      }

      shape.classindex = msShapeGetClass(lp, map, &shape, classgroup, nclasses);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
        msFreeShape(&shape);
        continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
        msFreeShape(&shape);
        continue;
      }

      if(lp->project) {
        if( reprojector == NULL ) {
            reprojector = msProjectCreateReprojector(&(lp->projection), &(map->projection));
            if( reprojector == NULL ) {
              msFreeShape(&shape);
              status = MS_FAILURE;
              break;
            }
        }
        msProjectShapeEx(reprojector, &shape);
      }

      if(msRectContained(&shape.bounds, &searchrectInMapProj) == MS_TRUE) { /* if the whole shape is in, don't intersect */
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
        /* Should we skip this feature? */
        if (!paging && map->query.startindex > 1) {
          --map->query.startindex;
          msFreeShape(&shape);
          continue;
        }
        if( map->query.only_cache_result_count )
            lp->resultcache->numresults ++;
        else
            addResult(map, lp->resultcache, &queryCache, &shape);
        --map->query.maxfeatures;
      }
      msFreeShape(&shape);

      /* check shape count */
      if(lp->maxfeatures > 0 && lp->maxfeatures == lp->resultcache->numresults) {
        status = MS_DONE;
        break;
      }
      
    } /* next shape */

    if (classgroup)
      msFree(classgroup);

    msProjectDestroyReprojector(reprojector);

    if(status != MS_DONE) {
        msFreeShape(&searchshape);
        return(MS_FAILURE);
    }

    if( !map->query.only_cache_result_count &&
        lp->resultcache->numresults == 0) msLayerClose(lp); /* no need to keep the layer open */
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

static int is_duplicate(resultCacheObj *resultcache, int shapeindex, int tileindex)
{
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
  double minfeaturesize = -1;

  queryCacheObj queryCache;

  initQueryCache(&queryCache);

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
  slp->project = msProjectionsDiffer(&(slp->projection), &(map->projection));

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
    reprojectionObj* reprojector = NULL;
    if(l == map->query.slayer) continue; /* skip the selection layer */

    lp = (GET_LAYER(map, l));
    if (map->query.maxfeatures == 0)
      break; /* nothing else to do */
    else if (map->query.maxfeatures > 0)
      lp->maxfeatures = map->query.maxfeatures;

    /* using mapscript, the map->query.startindex will be unset... */
    if (lp->startindex > 1 && map->query.startindex < 0)
      map->query.startindex = lp->startindex;

    /* conditions may have changed since this layer last drawn, so set
       layer->project true to recheck projection needs (Bug #673) */
    lp->project = msProjectionsDiffer(&(lp->projection), &(map->projection));

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

    msLayerClose(lp); /* reset */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);
    msLayerEnablePaging(lp, MS_FALSE);

    /* build item list, we want *all* items */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* for each selection shape */
    for(i=0; i<slp->resultcache->numresults; i++) {

      status = msLayerGetShape(slp, &selectshape, &(slp->resultcache->results[i]));
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

      if(slp->project)
        msProjectShape(&(slp->projection), &(map->projection), &selectshape);

      /* identify target shapes */
      searchrect = selectshape.bounds;

      searchrect.minx -= tolerance; /* expand the search box to account for layer tolerances (e.g. buffered searches) */
      searchrect.maxx += tolerance;
      searchrect.miny -= tolerance;
      searchrect.maxy += tolerance;

      if(lp->project)
        msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */

      status = msLayerWhichShapes(lp, searchrect, MS_TRUE);
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
        MS_CHECK_ALLOC(lp->resultcache, sizeof(resultCacheObj), MS_FAILURE);
        initResultCache( lp->resultcache);

      }

      nclasses = 0;
      classgroup = NULL;
      if (lp->classgroup && lp->numclasses > 0)
        classgroup = msAllocateValidClassGroups(lp, &nclasses);

      if (lp->minfeaturesize > 0)
        minfeaturesize = Pix2LayerGeoref(map, lp, lp->minfeaturesize);

      while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

        /* check for dups when there are multiple selection shapes */
        if(i > 0 && is_duplicate(lp->resultcache, shape.index, shape.tileindex)) continue;


        /* Check if the shape size is ok to be drawn */
        if ( (shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) && (minfeaturesize > 0) ) {
          if (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE) {
            if( lp->debug >= MS_DEBUGLEVEL_V )
              msDebug("msQueryByFeature(): Skipping shape (%ld) because LAYER::MINFEATURESIZE is bigger than shape size\n", shape.index);
            msFreeShape(&shape);
            continue;
          }
        }

        shape.classindex = msShapeGetClass(lp, map, &shape, classgroup, nclasses);
        if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
          msFreeShape(&shape);
          continue;
        }

        if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
          msFreeShape(&shape);
          continue;
        }

        if(lp->project) {
            if( reprojector == NULL ) {
                reprojector = msProjectCreateReprojector(&(lp->projection), &(map->projection));
                if( reprojector == NULL ) {
                    msFreeShape(&shape);
                    status = MS_FAILURE;
                    break;
                }
            }
            msProjectShapeEx(reprojector, &shape);
        }

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
          /* Should we skip this feature? */
          if (!msLayerGetPaging(lp) && map->query.startindex > 1) {
            --map->query.startindex;
            msFreeShape(&shape);
            continue;
          }
          addResult(map, lp->resultcache, &queryCache, &shape);
        }
        msFreeShape(&shape);

        /* check shape count */
        if(lp->maxfeatures > 0 && lp->maxfeatures == lp->resultcache->numresults) {
          status = MS_DONE;
          break;
        }
      } /* next shape */

      if (classgroup)
        msFree(classgroup);

      msProjectDestroyReprojector(reprojector);

      msFreeShape(&selectshape);

      if(status != MS_DONE) return(MS_FAILURE);

    } /* next selection shape */

    if(lp->resultcache == NULL || lp->resultcache->numresults == 0) msLayerClose(lp); /* no need to keep the layer open */
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

  int paging;
  char status;
  rectObj rect, searchrect;
  shapeObj shape;
  int nclasses = 0;
  int *classgroup = NULL;
  double minfeaturesize = -1;

  queryCacheObj queryCache;

  initQueryCache(&queryCache);

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
    reprojectionObj* reprojector = NULL;
    lp = (GET_LAYER(map, l));
    if (map->query.maxfeatures == 0)
      break; /* nothing else to do */
    else if (map->query.maxfeatures > 0)
      lp->maxfeatures = map->query.maxfeatures;

    /* using mapscript, the map->query.startindex will be unset... */
    if (lp->startindex > 1 && map->query.startindex < 0)
      map->query.startindex = lp->startindex;
    
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
        t = layer_tolerance * MS_MAX(MS_CELLSIZE(map->extent.minx, map->extent.maxx, map->width),
                                     MS_CELLSIZE(map->extent.miny, map->extent.maxy, map->height));
      else
        t = layer_tolerance * (msInchesPerUnit(lp->toleranceunits,0)/msInchesPerUnit(map->units,0));
    } else /* use buffer distance */
      t = map->query.buffer;

    rect.minx = map->query.point.x - t;
    rect.maxx = map->query.point.x + t;
    rect.miny = map->query.point.y - t;
    rect.maxy = map->query.point.y + t;

    /* Paging could have been disabled before */
    paging = msLayerGetPaging(lp);
    msLayerClose(lp); /* reset */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);
    msLayerEnablePaging(lp, paging);

    /* build item list, we want *all* items */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* identify target shapes */
    searchrect = rect;
    lp->project = msProjectionsDiffer(&(lp->projection), &(map->projection));
    if(lp->project)
      msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */

    status = msLayerWhichShapes(lp, searchrect, MS_TRUE);
    if(status == MS_DONE) { /* no overlap */
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      return(MS_FAILURE);
    }

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    MS_CHECK_ALLOC(lp->resultcache, sizeof(resultCacheObj), MS_FAILURE);
    initResultCache( lp->resultcache);

    nclasses = 0;
    classgroup = NULL;
    if (lp->classgroup && lp->numclasses > 0)
      classgroup = msAllocateValidClassGroups(lp, &nclasses);

    if (lp->minfeaturesize > 0)
      minfeaturesize = Pix2LayerGeoref(map, lp, lp->minfeaturesize);

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

      /* Check if the shape size is ok to be drawn */
      if ( (shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) && (minfeaturesize > 0) ) {
        if (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE) {
          if( lp->debug >= MS_DEBUGLEVEL_V )
            msDebug("msQueryByPoint(): Skipping shape (%ld) because LAYER::MINFEATURESIZE is bigger than shape size\n", shape.index);
          msFreeShape(&shape);
          continue;
        }
      }

      shape.classindex = msShapeGetClass(lp, map, &shape, classgroup, nclasses);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
        msFreeShape(&shape);
        continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
        msFreeShape(&shape);
        continue;
      }

      if(lp->project) {
        if( reprojector == NULL ) {
            reprojector = msProjectCreateReprojector(&(lp->projection), &(map->projection));
            if( reprojector == NULL ) {
              msFreeShape(&shape);
              status = MS_FAILURE;
              break;
            }
        }
        msProjectShapeEx(reprojector, &shape);
      }

      d = msDistancePointToShape(&(map->query.point), &shape);
      if( d <= t ) { /* found one */

        /* Should we skip this feature? */
        if (!paging && map->query.startindex > 1) {
          --map->query.startindex;
          msFreeShape(&shape);
          continue;
        }

        if(map->query.mode == MS_QUERY_SINGLE) {
          cleanupResultCache(lp->resultcache);
          initQueryCache(&queryCache);
          addResult(map, lp->resultcache, &queryCache, &shape);
          t = d; /* next one must be closer */
        } else {
          addResult(map, lp->resultcache, &queryCache, &shape);
        }
      }

      msFreeShape(&shape);

      if(map->query.mode == MS_QUERY_MULTIPLE && map->query.maxresults > 0 && lp->resultcache->numresults == map->query.maxresults) {
        status = MS_DONE;   /* got enough results for this layer */
        break;
      }

      /* check shape count */
      if(lp->maxfeatures > 0 && lp->maxfeatures == lp->resultcache->numresults) {
        status = MS_DONE;
        break;
      }
    } /* next shape */

    if (classgroup)
      msFree(classgroup);

    msProjectDestroyReprojector(reprojector);

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
  double minfeaturesize = -1;
  queryCacheObj queryCache;

  initQueryCache(&queryCache);

  if(map->query.type != MS_QUERY_BY_SHAPE) {
    msSetError(MS_QUERYERR, "The query is not properly defined.", "msQueryByShape()");
    return(MS_FAILURE);
  }

  if(!(map->query.shape)) {
    msSetError(MS_QUERYERR, "Query shape is not defined.", "msQueryByShape()");
    return(MS_FAILURE);
  }
  if(map->query.shape->type != MS_SHAPE_POLYGON && map->query.shape->type != MS_SHAPE_LINE && map->query.shape->type != MS_SHAPE_POINT) {
    msSetError(MS_QUERYERR, "Query shape MUST be a polygon, line or point.", "msQueryByShape()");
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
    reprojectionObj* reprojector = NULL;
    lp = (GET_LAYER(map, l));
    if (map->query.maxfeatures == 0)
      break; /* nothing else to do */
    else if (map->query.maxfeatures > 0)
      lp->maxfeatures = map->query.maxfeatures;

    /* using mapscript, the map->query.startindex will be unset... */
    if (lp->startindex > 1 && map->query.startindex < 0)
      map->query.startindex = lp->startindex;

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

    msLayerClose(lp); /* reset */
    status = msLayerOpen(lp);
    if(status != MS_SUCCESS) return(MS_FAILURE);
    /* disable driver paging */
    msLayerEnablePaging(lp, MS_FALSE);

    /* build item list, we want *all* items */
    status = msLayerWhichItems(lp, MS_TRUE, NULL);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    /* identify target shapes */
    searchrect = qshape->bounds;

    searchrect.minx -= tolerance; /* expand the search box to account for layer tolerances (e.g. buffered searches) */
    searchrect.maxx += tolerance;
    searchrect.miny -= tolerance;
    searchrect.maxy += tolerance;

    lp->project = msProjectionsDiffer(&(lp->projection), &(map->projection));
    if(lp->project)
      msProjectRect(&(map->projection), &(lp->projection), &searchrect); /* project the searchrect to source coords */

    status = msLayerWhichShapes(lp, searchrect, MS_TRUE);
    if(status == MS_DONE) { /* no overlap */
      msLayerClose(lp);
      continue;
    } else if(status != MS_SUCCESS) {
      msLayerClose(lp);
      return(MS_FAILURE);
    }

    lp->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); /* allocate and initialize the result cache */
    MS_CHECK_ALLOC(lp->resultcache, sizeof(resultCacheObj), MS_FAILURE);
    initResultCache( lp->resultcache);

    nclasses = 0;
    if (lp->classgroup && lp->numclasses > 0)
      classgroup = msAllocateValidClassGroups(lp, &nclasses);

    if (lp->minfeaturesize > 0)
      minfeaturesize = Pix2LayerGeoref(map, lp, lp->minfeaturesize);

    while((status = msLayerNextShape(lp, &shape)) == MS_SUCCESS) { /* step through the shapes */

      /* Check if the shape size is ok to be drawn */
      if ( (shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) && (minfeaturesize > 0) ) {
        if (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE) {
          if( lp->debug >= MS_DEBUGLEVEL_V )
            msDebug("msQueryByShape(): Skipping shape (%ld) because LAYER::MINFEATURESIZE is bigger than shape size\n", shape.index);
          msFreeShape(&shape);
          continue;
        }
      }

      shape.classindex = msShapeGetClass(lp, map, &shape, classgroup, nclasses);
      if(!(lp->template) && ((shape.classindex == -1) || (lp->class[shape.classindex]->status == MS_OFF))) { /* not a valid shape */
        msFreeShape(&shape);
        continue;
      }

      if(!(lp->template) && !(lp->class[shape.classindex]->template)) { /* no valid template */
        msFreeShape(&shape);
        continue;
      }

      if(lp->project) {
        if( reprojector == NULL ) {
            reprojector = msProjectCreateReprojector(&(lp->projection), &(map->projection));
            if( reprojector == NULL ) {
              msFreeShape(&shape);
              status = MS_FAILURE;
              break;
            }
        }
        msProjectShapeEx(reprojector, &shape);
      }

      switch(qshape->type) { /* may eventually support types other than polygon or line */
        case MS_SHAPE_POLYGON:
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
          break;
        case MS_SHAPE_LINE:
          switch(shape.type) { /* make sure shape actually intersects the selectshape */
            case MS_SHAPE_POINT:
              if(tolerance == 0) { /* just test for intersection */
                distance = msDistanceShapeToShape(qshape, &shape);
                if(distance == 0) status = MS_TRUE;
              } else {
                distance = msDistanceShapeToShape(qshape, &shape);
                if(distance < tolerance) status = MS_TRUE;
              }
              break;
            case MS_SHAPE_LINE:
              if(tolerance == 0) { /* just test for intersection */
                status = msIntersectPolylines(&shape, qshape);
              } else { /* check distance, distance=0 means they intersect */
                distance = msDistanceShapeToShape(qshape, &shape);
                if(distance < tolerance) status = MS_TRUE;
              }
              break;
            case MS_SHAPE_POLYGON:
              if(tolerance == 0) /* just test for intersection */
                status = msIntersectPolylinePolygon(qshape, &shape);
              else { /* check distance, distance=0 means they intersect */
                distance = msDistanceShapeToShape(qshape, &shape);
                if(distance < tolerance) status = MS_TRUE;
              }
              break;
            default:
              status = MS_FALSE;
              break;
          }
          break;
        case MS_SHAPE_POINT:
          distance = msDistanceShapeToShape(qshape, &shape);
          status = MS_FALSE;
          if(tolerance == 0 && distance == 0) status = MS_TRUE; /* shapes intersect */
          else if(distance < tolerance) status = MS_TRUE; /* shapes are close enough */
          break;
        default:
          break; /* should never get here as we test for selection shape type explicitly earlier */
      }

      if(status == MS_TRUE) {
        /* Should we skip this feature? */
        if (!msLayerGetPaging(lp) && map->query.startindex > 1) {
          --map->query.startindex;
          msFreeShape(&shape);
          continue;
        }
        addResult(map, lp->resultcache, &queryCache, &shape);
      }
      msFreeShape(&shape);

      /* check shape count */
      if(lp->maxfeatures > 0 && lp->maxfeatures == lp->resultcache->numresults) {
        status = MS_DONE;
        break;
      }
    } /* next shape */

    free(classgroup);
    classgroup = NULL;

    msProjectDestroyReprojector(reprojector);

    if(status != MS_DONE) {
      return(MS_FAILURE);
    }

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

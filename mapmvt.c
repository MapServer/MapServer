/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapBox Vector Tile rendering.
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2015 Regents of the University of Minnesota.
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
 *****************************************************************************/

#include "mapserver.h"
#include "maptile.h"

#ifdef USE_PBF
#include "vector_tile.pb-c.h"
#include "mapows.h"
#include "uthash.h"
#include <float.h>

#define MOVETO 1
#define LINETO 2
#define CLOSEPATH 7

#define FEATURES_INCREMENT_SIZE 5

enum MS_RING_DIRECTION { MS_DIRECTION_INVALID_RING, MS_DIRECTION_CLOCKWISE, MS_DIRECTION_COUNTERCLOCKWISE };

typedef struct {
  char *value;
  unsigned int index;
  UT_hash_handle hh;
} value_lookup;

typedef struct {
  value_lookup *cache;
} value_lookup_table;

#define COMMAND(id, count) (((id) & 0x7) | ((count) << 3))
#define PARAMETER(n) (((n) << 1) ^ ((n) >> 31))

static double getTriangleHeight(lineObj *ring)
{
  int i;
  double s=0, b=0;

  if(ring->numpoints != 4) return -1; /* not a triangle */

  for(i=0; i<ring->numpoints-1; i++) {
    s += (ring->point[i].x*ring->point[i+1].y - ring->point[i+1].x*ring->point[i].y);
    b = MS_MAX(b, msDistancePointToPoint(&ring->point[i], &ring->point[i+1]));
  }

  return (MS_ABS(s/b));
}

static enum MS_RING_DIRECTION mvtGetRingDirection(lineObj *ring) {
  int i, sum=0;

  if(ring->numpoints < 4) return MS_DIRECTION_INVALID_RING;

  /* step throught the edges */
  for(i=0; i<ring->numpoints-1; i++) {
    sum += ring->point[i].x * ring->point[i+1].y - ring->point[i+1].x * ring->point[i].y;
  }

  return sum > 0 ? MS_DIRECTION_CLOCKWISE :
         sum < 0 ? MS_DIRECTION_COUNTERCLOCKWISE : MS_DIRECTION_INVALID_RING;
}

static void mvtReverseRingDirection(lineObj *ring) {
  pointObj temp;
  int start=1, end=ring->numpoints-2; /* first and last points are the same so skip 'em */

  while (start < end) {
    temp.x = ring->point[start].x; temp.y = ring->point[start].y;
    ring->point[start].x = ring->point[end].x; ring->point[start].y = ring->point[end].y;
    ring->point[end].x = temp.x; ring->point[end].y = temp.y;
    start++;
    end--;
  }
}

static void mvtReorderRings(shapeObj *shape, int *outers) {
  int i, j;
  int t1;
  lineObj t2; 

  for(i=0; i<(shape->numlines-1); i++) {
    for(j=0; j<(shape->numlines-i-1); j++) {
      if(outers[j] < outers[j+1]) {
        /* swap */
	t1 = outers[j];
        outers[j] = outers[j+1];
	outers[j+1] = t1;

	t2 = shape->line[j];
        shape->line[j] = shape->line[j+1];
	shape->line[j+1] = t2;
      }
    }
  }
}

static int mvtTransformShape(shapeObj *shape, rectObj *extent, int layer_type, int mvt_layer_extent) {
  double scale_x,scale_y;
  int i,j,outj;

  int *outers=NULL, ring_direction;

  scale_x = (double)mvt_layer_extent/(extent->maxx - extent->minx);
  scale_y = (double)mvt_layer_extent/(extent->maxy - extent->miny);

  if(layer_type == MS_LAYER_POLYGON) {
    outers = msGetOuterList(shape); /* compute before we muck with the shape */
    if(outers[0] == 0) /* first ring must be an outer */
      mvtReorderRings(shape, outers);
  }

  for(i=0;i<shape->numlines;i++) {
    for(j=0,outj=0;j<shape->line[i].numpoints;j++) {

      shape->line[i].point[outj].x = (int)((shape->line[i].point[j].x - extent->minx)*scale_x);
      shape->line[i].point[outj].y = mvt_layer_extent - (int)((shape->line[i].point[j].y - extent->miny)*scale_y);

      if(!outj || shape->line[i].point[outj].x != shape->line[i].point[outj-1].x || shape->line[i].point[outj].y != shape->line[i].point[outj-1].y)
        outj++; /* add the point to the shape only if it's the first one or if it's different than the previous one */
    }
    shape->line[i].numpoints = outj;

    if(layer_type == MS_LAYER_POLYGON) {
      if(shape->line[i].numpoints == 4 && getTriangleHeight(&shape->line[i]) < 1) {        
        shape->line[i].numpoints = 0; /* so it's not considered anymore */
        continue; /* next ring */
      }

      ring_direction = mvtGetRingDirection(&shape->line[i]);
      if(ring_direction == MS_DIRECTION_INVALID_RING)
        shape->line[i].numpoints = 0; /* so it's not considered anymore */
      else if((outers[i] && ring_direction != MS_DIRECTION_CLOCKWISE) || (!outers[i] && ring_direction != MS_DIRECTION_COUNTERCLOCKWISE))
        mvtReverseRingDirection(&shape->line[i]);
    }
  }

  msComputeBounds(shape); /* TODO: might need to limit this to just valid parts... */
  msFree(outers);

  return (shape->numlines == 0)?MS_FAILURE:MS_SUCCESS; /* sucess if at least one line */
}

static int mvtClipShape(shapeObj *shape, int layer_type, int buffer, int mvt_layer_extent) {
  rectObj tile_rect;
  tile_rect.minx = tile_rect.miny=-buffer*16;
  tile_rect.maxx = tile_rect.maxy=mvt_layer_extent + buffer*16;

  if(layer_type == MS_LAYER_POLYGON) {
    msClipPolygonRect(shape, tile_rect);
  } else if(layer_type == MS_LAYER_LINE) {
    msClipPolylineRect(shape, tile_rect);
  }

  /* success if at least one line and not a degenerate bounding box */
  if(shape->numlines > 0 && (layer_type == MS_LAYER_POINT || (shape->bounds.minx != shape->bounds.maxx || shape->bounds.miny != shape->bounds.maxy)))
    return MS_SUCCESS;
  else
    return MS_FAILURE;
}

static void freeMvtFeature( VectorTile__Tile__Feature *mvt_feature ) {
  if(mvt_feature->tags)
    msFree(mvt_feature->tags);
  if(mvt_feature->geometry)
    msFree(mvt_feature->geometry);
}

static void freeMvtValue( VectorTile__Tile__Value *mvt_value ) {
  if(mvt_value->string_value)
    msFree(mvt_value->string_value);
}

static void freeMvtLayer( VectorTile__Tile__Layer *mvt_layer ) {
  if(mvt_layer->keys) {
    for(unsigned i=0;i<mvt_layer->n_keys; i++) {
      msFree(mvt_layer->keys[i]);
    }
    msFree(mvt_layer->keys);
  }
  if(mvt_layer->values) {
    for(unsigned i=0;i<mvt_layer->n_values; i++) {
        freeMvtValue(mvt_layer->values[i]);
        msFree(mvt_layer->values[i]);
    }
    msFree(mvt_layer->values);
  }
  if(mvt_layer->features) {
    for(unsigned i=0;i<mvt_layer->n_features; i++) {
        freeMvtFeature(mvt_layer->features[i]);
        msFree(mvt_layer->features[i]);
    }
    msFree(mvt_layer->features);
  }
}

int mvtWriteShape( layerObj *layer, shapeObj *shape, VectorTile__Tile__Layer *mvt_layer,
                   gmlItemListObj *item_list, value_lookup_table *value_lookup_cache,
                   rectObj *unbuffered_bbox, int buffer) {
  VectorTile__Tile__Feature *mvt_feature;
  int i,j,iout;
  value_lookup *value;
  long int n_geometry;

  /* could consider an intersection test here */

  if(mvtTransformShape(shape, unbuffered_bbox, layer->type, mvt_layer->extent) != MS_SUCCESS) {
    return MS_SUCCESS; /* degenerate shape */
  }
  if(mvtClipShape(shape, layer->type, buffer, mvt_layer->extent) != MS_SUCCESS) {
    return MS_SUCCESS; /* no features left after clipping */
  }

  n_geometry = 0;
  if(layer->type == MS_LAYER_POINT) {
    for(i=0;i<shape->numlines;i++)
      n_geometry += shape->line[i].numpoints * 2;
    if(n_geometry)
      n_geometry++; /* one MOVETO */
  } else if(layer->type == MS_LAYER_LINE) {
    for(i=0;i<shape->numlines;i++)
      if(shape->line[i].numpoints >= 2) n_geometry += 2 + shape->line[i].numpoints * 2; /* one MOVETO, one LINETO */
  } else { /* MS_LAYER_POLYGON */
    for(i=0;i<shape->numlines;i++)
      if(shape->line[i].numpoints >= 4) n_geometry += 3 + (shape->line[i].numpoints-1) * 2; /* one MOVETO, one LINETO, one CLOSEPATH (don't consider last duplicate point) */
  }

  if(n_geometry == 0) return MS_SUCCESS;

  mvt_layer->features[mvt_layer->n_features++] = msSmallMalloc(sizeof(VectorTile__Tile__Feature));
  mvt_feature = mvt_layer->features[mvt_layer->n_features-1];
  vector_tile__tile__feature__init(mvt_feature);
  mvt_feature->n_tags = mvt_layer->n_keys * 2;
  mvt_feature->tags = msSmallMalloc(mvt_feature->n_tags * sizeof(uint32_t));
  mvt_feature->id = shape->index;
  mvt_feature->has_id = 1;

  if(layer->type == MS_LAYER_POLYGON)
    mvt_feature->type = VECTOR_TILE__TILE__GEOM_TYPE__POLYGON;
  else if(layer->type == MS_LAYER_LINE)
    mvt_feature->type = VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING;
  else
    mvt_feature->type = VECTOR_TILE__TILE__GEOM_TYPE__POINT;
  mvt_feature->has_type = 1;

  /* output values */
  for( i = 0, iout = 0; i < item_list->numitems; i++ ) {
    gmlItemObj *item = item_list->items + i;

    if( !item->visible )
      continue;

    UT_HASH_FIND_STR(value_lookup_cache->cache, shape->values[i], value);
    if(!value) {
      VectorTile__Tile__Value *mvt_value;
      value = msSmallMalloc(sizeof(value_lookup));
      value->value = msStrdup(shape->values[i]);
      value->index = mvt_layer->n_values;
      mvt_layer->values = msSmallRealloc(mvt_layer->values,(++mvt_layer->n_values)*sizeof(VectorTile__Tile__Value*));
      mvt_layer->values[mvt_layer->n_values-1] = msSmallMalloc(sizeof(VectorTile__Tile__Value));
      mvt_value = mvt_layer->values[mvt_layer->n_values-1];
      vector_tile__tile__value__init(mvt_value);

      if( item->type && EQUAL(item->type,"Integer")) {
        mvt_value->int_value = atoi(value->value);
        mvt_value->has_int_value = 1;
      } else if( item->type && EQUAL(item->type,"Long")) { /* signed */
	mvt_value->sint_value = atol(value->value);
	mvt_value->has_sint_value = 1;
      } else if( item->type && EQUAL(item->type,"Real")) {
        mvt_value->float_value = atof(value->value);
        mvt_value->has_float_value = 1;
      } else if( item->type && EQUAL(item->type,"Boolean") ) {
        if(EQUAL(value->value,"0") || EQUAL(value->value,"false"))
          mvt_value->bool_value = 0;
        else
          mvt_value->bool_value = 1;
        mvt_value->has_bool_value = 1;
      } else {
        mvt_value->string_value = msStrdup(value->value);
      }
      UT_HASH_ADD_KEYPTR(hh,value_lookup_cache->cache,value->value, strlen(value->value), value);
    }
    mvt_feature->tags[iout*2] = iout;
    mvt_feature->tags[iout*2+1] = value->index;

    iout++;
  }

  /* output geom */
  mvt_feature->n_geometry = n_geometry;
  mvt_feature->geometry = msSmallMalloc(mvt_feature->n_geometry * sizeof(uint32_t));

  if(layer->type == MS_LAYER_POINT) {
    int idx=0, lastx=0, lasty=0;
    mvt_feature->geometry[idx++] = COMMAND(MOVETO, mvt_feature->n_geometry-1);
    for(i=0;i<shape->numlines;i++) {
      for(j=0;j<shape->line[i].numpoints;j++) {
        mvt_feature->geometry[idx++] = PARAMETER(MS_NINT(shape->line[i].point[j].x)-lastx);
        mvt_feature->geometry[idx++] = PARAMETER(MS_NINT(shape->line[i].point[j].y)-lasty);
        lastx = MS_NINT(shape->line[i].point[j].x);
        lasty = MS_NINT(shape->line[i].point[j].y);
      }
    }
  } else { /* MS_LAYER_LINE or MS_LAYER_POLYGON */
    int numpoints;
    int idx=0, lastx=0, lasty=0;
    for(i=0;i<shape->numlines;i++) {

      if((layer->type == MS_LAYER_LINE && !(shape->line[i].numpoints >= 2)) || 
         (layer->type == MS_LAYER_POLYGON && !(shape->line[i].numpoints >= 4))) {        
        continue; /* skip malformed parts */
      }

      numpoints = (layer->type == MS_LAYER_LINE)?shape->line[i].numpoints:(shape->line[i].numpoints-1); /* don't consider last point for polygons */
      for(j=0;j<numpoints;j++) {
        if(j==0) {
          mvt_feature->geometry[idx++] = COMMAND(MOVETO, 1);
        } else if(j==1) {
          mvt_feature->geometry[idx++] = COMMAND(LINETO, numpoints-1);
        }
        mvt_feature->geometry[idx++] = PARAMETER(MS_NINT(shape->line[i].point[j].x)-lastx);
	mvt_feature->geometry[idx++] = PARAMETER(MS_NINT(shape->line[i].point[j].y)-lasty);
	lastx = MS_NINT(shape->line[i].point[j].x);
	lasty = MS_NINT(shape->line[i].point[j].y);
      }
      if(layer->type == MS_LAYER_POLYGON) {
        mvt_feature->geometry[idx++] = COMMAND(CLOSEPATH, 1);
      }
    }
  }

  return MS_SUCCESS;
}

static void freeMvtTile( VectorTile__Tile *mvt_tile ) {
  for(unsigned iLayer=0;iLayer<mvt_tile->n_layers;iLayer++) {
    freeMvtLayer(mvt_tile->layers[iLayer]);
    msFree(mvt_tile->layers[iLayer]);
  }
  msFree(mvt_tile->layers);
}

int msMVTWriteTile( mapObj *map, int sendheaders ) {
  int iLayer,retcode=MS_SUCCESS;
  unsigned len;
  void *buf;
  const char *mvt_extent = msGetOutputFormatOption(map->outputformat, "EXTENT", "4096");
  const char *mvt_buffer = msGetOutputFormatOption(map->outputformat, "EDGE_BUFFER", "10");
  int buffer = MS_ABS(atoi(mvt_buffer));
  VectorTile__Tile mvt_tile = VECTOR_TILE__TILE__INIT;
  mvt_tile.layers = msSmallCalloc(map->numlayers, sizeof(VectorTile__Tile__Layer*));

  /* make sure we have a scale and cellsize computed */
  map->cellsize = MS_CELLSIZE(map->extent.minx, map->extent.maxx, map->width);
  msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scaledenom);

  /* expand the map->extent so it goes from pixel center (MapServer) to pixel edge (OWS) */
  map->extent.minx -= map->cellsize * 0.5;
  map->extent.maxx += map->cellsize * 0.5;
  map->extent.miny -= map->cellsize * 0.5;
  map->extent.maxy += map->cellsize * 0.5;

  for( iLayer = 0; iLayer < map->numlayers; iLayer++ ) {
    int status=MS_SUCCESS;
    layerObj *layer = GET_LAYER(map, iLayer);
    int i;
    shapeObj shape;
    gmlItemListObj *item_list = NULL;
    VectorTile__Tile__Layer *mvt_layer;
    value_lookup_table value_lookup_cache = {NULL};
    value_lookup *cur_value_lookup, *tmp_value_lookup;
    rectObj rect;

    unsigned features_size = 0;

    if(!msLayerIsVisible(map, layer)) continue;

    if(layer->type != MS_LAYER_POINT && layer->type != MS_LAYER_POLYGON && layer->type != MS_LAYER_LINE)
      continue;

    status = msLayerOpen(layer);
    if(status != MS_SUCCESS) {
      retcode = status;
      goto layer_cleanup;
    }

    status = msLayerWhichItems(layer, MS_TRUE, NULL); /* we want all items - behaves like a query in that sense */
    if(status != MS_SUCCESS) {
      retcode = status;
      goto layer_cleanup;
    }

    /* -------------------------------------------------------------------- */
    /*      Will we need to reproject?                                      */
    /* -------------------------------------------------------------------- */
    layer->project = msProjectionsDiffer(&(layer->projection), &(map->projection));

    rect = map->extent;
    if(layer->project) msProjectRect(&(map->projection), &(layer->projection), &rect);

    status = msLayerWhichShapes(layer, rect, MS_TRUE);
    if(status == MS_DONE) { /* no overlap - that's ok */
      retcode = MS_SUCCESS;
      goto layer_cleanup;
    } else if(status != MS_SUCCESS) {
      retcode = status;
      goto layer_cleanup;
    }

    mvt_tile.layers[mvt_tile.n_layers++] = msSmallMalloc(sizeof(VectorTile__Tile__Layer));
    mvt_layer = mvt_tile.layers[mvt_tile.n_layers-1];
    vector_tile__tile__layer__init(mvt_layer);
    mvt_layer->version = 2;
    mvt_layer->name = layer->name;

    mvt_layer->extent = MS_ABS(atoi(mvt_extent));
    mvt_layer->has_extent = 1;

    /* -------------------------------------------------------------------- */
    /*      Create appropriate attributes on this layer.                    */
    /* -------------------------------------------------------------------- */
    item_list = msGMLGetItems( layer, "G" );
    assert( item_list->numitems == layer->numitems );

    mvt_layer->keys = msSmallMalloc(layer->numitems * sizeof(char*));

    for( i = 0; i < layer->numitems; i++ ) {
      gmlItemObj *item = item_list->items + i;

      if( !item->visible )
        continue;

      if( item->alias )
        mvt_layer->keys[mvt_layer->n_keys++] = msStrdup(item->alias);
      else
        mvt_layer->keys[mvt_layer->n_keys++] = msStrdup(item->name);
    }

    /* -------------------------------------------------------------------- */
    /*      Setup joins if needed.  This is likely untested.                */
    /* -------------------------------------------------------------------- */
    if(layer->numjoins > 0) {
      int j;
      for(j=0; j<layer->numjoins; j++) {
        status = msJoinConnect(layer, &(layer->joins[j]));
        if(status != MS_SUCCESS) {
          retcode = status;
          goto layer_cleanup;
        }
      }
    }

    mvt_layer->features = msSmallCalloc(FEATURES_INCREMENT_SIZE, sizeof(VectorTile__Tile__Feature*));
    features_size = FEATURES_INCREMENT_SIZE;

    msInitShape(&shape);
    while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
      
      if(layer->numclasses > 0) {
        shape.classindex = msShapeGetClass(layer, map, &shape, NULL, -1); /* Perform classification, and some annotation related magic. */
        if(shape.classindex < 0)
          goto feature_cleanup; /* no matching CLASS found, skip this feature */
      }

      /*
      ** prepare any necessary JOINs here (one-to-one only)
      */
      if( layer->numjoins > 0) {
        int j;

        for(j=0; j < layer->numjoins; j++) {
          if(layer->joins[j].type == MS_JOIN_ONE_TO_ONE) {
            msJoinPrepare(&(layer->joins[j]), &shape);
            msJoinNext(&(layer->joins[j])); /* fetch the first row */
          }
        }
      }

      if(mvt_layer->n_features == features_size) { /* need to allocate more space */
        features_size += FEATURES_INCREMENT_SIZE;
        mvt_layer->features = msSmallRealloc(mvt_layer->features, sizeof(VectorTile__Tile__Feature*)*(features_size));
      }

      if( layer->project ) {
        if( layer->reprojectorLayerToMap == NULL )
        {
            layer->reprojectorLayerToMap = msProjectCreateReprojector(
                &layer->projection, &map->projection);
        }
        if( layer->reprojectorLayerToMap )
            status = msProjectShapeEx(layer->reprojectorLayerToMap, &shape);
        else
            status = MS_FAILURE;
      }
      if( status == MS_SUCCESS ) {
        status = mvtWriteShape( layer, &shape, mvt_layer, item_list, &value_lookup_cache, &map->extent, buffer );
      }

      feature_cleanup:
      msFreeShape(&shape);
      if(status != MS_SUCCESS) goto layer_cleanup;
    } /* next shape */
layer_cleanup:
    msLayerClose(layer);
    msGMLFreeItems(item_list);
    UT_HASH_ITER(hh, value_lookup_cache.cache, cur_value_lookup, tmp_value_lookup) {
      msFree(cur_value_lookup->value);
      UT_HASH_DEL(value_lookup_cache.cache,cur_value_lookup);
      msFree(cur_value_lookup);
    }
    if(retcode != MS_SUCCESS) goto cleanup;
  } /* next layer */

  len = vector_tile__tile__get_packed_size(&mvt_tile); // This is the calculated packing length

  buf = msSmallMalloc(len); // Allocate memory
  vector_tile__tile__pack(&mvt_tile, buf);
  if( sendheaders ) {
    msIO_fprintf( stdout,
		  "Content-Length: %d\r\n"
		  "Content-Type: %s\r\n\r\n",
                  len, MS_IMAGE_MIME_TYPE(map->outputformat));
  }
  msIO_fwrite(buf,len,1,stdout);
  msFree(buf);

  cleanup:
  freeMvtTile(&mvt_tile);

  return retcode;
}

int msPopulateRendererVTableMVT(rendererVTableObj * renderer) {
  (void)renderer;
  return MS_SUCCESS;
}
#else
int msPopulateRendererVTableMVT(rendererVTableObj * renderer) {
  msSetError(MS_MISCERR, "Vector Tile Driver requested but support is not compiled in", "msPopulateRendererVTableMVT()");
  return MS_FAILURE;
}

int msMVTWriteTile( mapObj *map, int sendheaders ) {
  msSetError(MS_MISCERR, "Vector Tile support is not available.", "msMVTWriteTile()");
  return MS_FAILURE;
}
#endif

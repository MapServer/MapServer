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

#ifdef USE_PBF
#include "vector_tile.pb-c.h"
#include "mapows.h"
#include "uthash.h"

typedef struct {
  char *value;
  unsigned int index;
  UT_hash_handle hh;
} value_lookup;

typedef struct {
  value_lookup *cache;
} value_lookup_table;

static void freeMvtFeature( VectorTile__Tile__Feature *mvt_feature ) {
  if(mvt_feature->tags)
    free(mvt_feature->tags);
  if(mvt_feature->geometry)
    free(mvt_feature->geometry);
}

static void freeMvtValue( VectorTile__Tile__Value *mvt_value ) {
  if(mvt_value->string_value)
    free(mvt_value->string_value);
}

static void freeMvtLayer( VectorTile__Tile__Layer *mvt_layer ) {
  int i;
  if(mvt_layer->keys) {
    for(i=0;i<mvt_layer->n_keys; i++) {
      free(mvt_layer->keys[i]);
    }
    free(mvt_layer->keys);
  }
  if(mvt_layer->values) {
    for(i=0;i<mvt_layer->n_values; i++) {
        freeMvtValue(mvt_layer->values[i]);
        free(mvt_layer->values[i]);
    }
    free(mvt_layer->values);
  }
  if(mvt_layer->features) {
    for(i=0;i<mvt_layer->n_features; i++) {
        freeMvtFeature(mvt_layer->features[i]);
        free(mvt_layer->features[i]);
    }
    free(mvt_layer->features);
  }
}

int mvtWriteShape( layerObj *layer, shapeObj *shape, VectorTile__Tile__Layer *mvt_layer, gmlItemListObj *item_list, value_lookup_table *value_lookup_cache) {
  VectorTile__Tile__Feature *mvt_feature;
  int i,iout;
  value_lookup *value;
  mvt_layer->features[mvt_layer->n_features++] = msSmallMalloc(sizeof(VectorTile__Tile__Feature));
  mvt_feature = mvt_layer->features[mvt_layer->n_features-1];
  vector_tile__tile__feature__init(mvt_feature);
  mvt_feature->n_tags = mvt_layer->n_keys * 2;
  mvt_feature->tags = msSmallMalloc(mvt_feature->n_tags * sizeof(uint32_t));
  mvt_feature->id = shape->index;
  mvt_feature->has_id = 1;

  for( i = 0, iout = 0; i < item_list->numitems; i++ ) {
    gmlItemObj *item = item_list->items + i;

    if( !item->visible )
      continue;

    UT_HASH_FIND_STR(value_lookup_cache->cache,shape->values[i],value);
    if(!value) {
      VectorTile__Tile__Value *mvt_value;
      value = msSmallMalloc(sizeof(value_lookup));
      value->value = msStrdup(shape->values[i]);
      value->index = mvt_layer->n_values;
      mvt_layer->values = msSmallRealloc(mvt_layer->values,(++mvt_layer->n_values)*sizeof(VectorTile__Tile__Value*));
      mvt_layer->values[mvt_layer->n_values-1] = msSmallMalloc(sizeof(VectorTile__Tile__Value));
      mvt_value = mvt_layer->values[mvt_layer->n_values-1];
      vector_tile__tile__value__init(mvt_value);

      if( EQUAL(item->type,"Integer")) {
        mvt_value->int_value = atoi(value->value);
        mvt_value->has_int_value = 1;
      } else if( EQUAL(item->type,"Real") ) {
        mvt_value->float_value = atof(value->value);
        mvt_value->has_float_value = 1;
      } else if( EQUAL(item->type,"Boolean") ) {
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
  mvt_feature->n_geometry = shape->line[0].numpoints * 2;
  mvt_feature->geometry = msSmallMalloc(mvt_feature->n_geometry * sizeof(uint32_t));
  for(i=0;i<shape->line[0].numpoints;i++) {
    mvt_feature->geometry[2*i] = shape->line[0].point[i].x;
    mvt_feature->geometry[2*i+1] = shape->line[0].point[i].y;
  }
  return MS_SUCCESS;

}

static void freeMvtTile( VectorTile__Tile *mvt_tile ) {
  int iLayer;
  for(iLayer=0;iLayer<mvt_tile->n_layers;iLayer++) {
    freeMvtLayer(mvt_tile->layers[iLayer]);
    free(mvt_tile->layers[iLayer]);
  }
  free(mvt_tile->layers);
}

int msMVTWriteFromQuery( mapObj *map, outputFormatObj *format, int sendheaders ) {
  int iLayer,retcode=MS_SUCCESS;
  unsigned len;
  void *buf;
  VectorTile__Tile mvt_tile = VECTOR_TILE__TILE__INIT;
  mvt_tile.layers = msSmallCalloc(map->numlayers,sizeof(VectorTile__Tile__Layer*));

  for( iLayer = 0; iLayer < map->numlayers; iLayer++ ) {
    int status=MS_SUCCESS;
    int i;
    layerObj *layer = GET_LAYER(map, iLayer);
    shapeObj resultshape;
    gmlItemListObj *item_list = NULL;
    int  reproject = MS_FALSE;
    VectorTile__Tile__Layer *mvt_layer;
    value_lookup_table value_lookup_cache = {NULL};
    value_lookup *cur_value_lookup, *tmp_value_lookup;

    if( !layer->resultcache || layer->resultcache->numresults == 0 )
      continue;

    /* -------------------------------------------------------------------- */
    /*      Will we need to reproject?                                      */
    /* -------------------------------------------------------------------- */
    if(layer->transform == MS_TRUE
        && layer->project
        && msProjectionsDiffer(&(layer->projection),
                               &(layer->map->projection)) )
      reproject = MS_TRUE;

    mvt_tile.layers[mvt_tile.n_layers++] = msSmallMalloc(sizeof(VectorTile__Tile__Layer));
    mvt_layer = mvt_tile.layers[mvt_tile.n_layers-1];
    vector_tile__tile__layer__init(mvt_layer);
    mvt_layer->version = 1;
    mvt_layer->name = layer->name;

    /* -------------------------------------------------------------------- */
    /*      Create the corresponding OGR Layer.                             */

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

    mvt_layer->features = msSmallCalloc(layer->resultcache->numresults, sizeof(VectorTile__Tile__Feature*));

    msInitShape( &resultshape );

    /* -------------------------------------------------------------------- */
    /*      Loop over all the shapes in the resultcache.                    */
    /* -------------------------------------------------------------------- */
    for(i=0; i < layer->resultcache->numresults; i++) {

      msFreeShape(&resultshape); /* init too */

      /*
      ** Read the shape.
      */
      status = msLayerGetShape(layer, &resultshape, &(layer->resultcache->results[i]));
      if(status != MS_SUCCESS) {
        retcode = status;
        goto feature_cleanup;
      }

      if(layer->numclasses > 0) {
        /*
      ** Perform classification, and some annotation related magic.
      */
        resultshape.classindex =
            msShapeGetClass(layer, map, &resultshape, NULL, -1);

        if(resultshape.classindex < 0)
          goto feature_cleanup; /* no matching CLASS found, skip this feature */
      }


      /*
      ** prepare any necessary JOINs here (one-to-one only)
      */
      if( layer->numjoins > 0) {
        int j;

        for(j=0; j < layer->numjoins; j++) {
          if(layer->joins[j].type == MS_JOIN_ONE_TO_ONE) {
            msJoinPrepare(&(layer->joins[j]), &resultshape);
            msJoinNext(&(layer->joins[j])); /* fetch the first row */
          }
        }
      }

      if( reproject ) {
        status =
          msProjectShape(&layer->projection, &layer->map->projection,
                         &resultshape);
      }

      if( status == MS_SUCCESS )
        status = mvtWriteShape( layer, &resultshape, mvt_layer, item_list, &value_lookup_cache );

      if(status != MS_SUCCESS) {
        retcode = status;
        goto feature_cleanup;
      }

      feature_cleanup:
      msFreeShape(&resultshape);
      if(retcode != MS_SUCCESS)
        goto layer_cleanup;
    }
    layer_cleanup:
    msGMLFreeItems(item_list);
    UT_HASH_ITER(hh, value_lookup_cache.cache, cur_value_lookup, tmp_value_lookup) {
      free(cur_value_lookup->value);
      UT_HASH_DEL(value_lookup_cache.cache,cur_value_lookup);
      free(cur_value_lookup);
    }
    if(retcode != MS_SUCCESS)
      goto cleanup;
  }

  len = vector_tile__tile__get_packed_size( &mvt_tile); // This is the calculated packing length
  buf = msSmallMalloc (len);                     // Allocate memory
  vector_tile__tile__pack( &mvt_tile, buf);
  if( sendheaders ) {
    msIO_fprintf( stdout,
                  "Content-Length: %d\r\n"
                  "Content-Type: application/x-protobuf\r\n\r\n",
                  len);
  }
  msIO_fwrite(buf,len,1,stdout);

  cleanup:
  freeMvtTile(&mvt_tile);

  return retcode;
}

int msPopulateRendererVTableMVT(rendererVTableObj * renderer) {
  return MS_SUCCESS;
}
#else

int msPopulateRendererVTableMVT(rendererVTableObj * renderer) {
  msSetError(MS_MISCERR, "Vector Tile Driver requested but support is not compiled in",
             "msPopulateRendererVTableMVT()");
  return MS_FAILURE;
}

int msMVTWriteFromQuery( mapObj *map, outputFormatObj *format, int sendheaders ) {
  msSetError(MS_MISCERR, "Vector Tile support is not available.",
             "msMVTWriteFromQuery()");
  return MS_FAILURE;
}
#endif

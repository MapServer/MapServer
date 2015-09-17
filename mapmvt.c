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
#include <float.h>

typedef struct {
  char *value;
  unsigned int index;
  UT_hash_handle hh;
} value_lookup;

typedef struct {
  value_lookup *cache;
} value_lookup_table;

#define ZIGZAG(n) (((n) << 1) ^ ((n) >> 31))

static int mvtTransformShape(shapeObj *shape, rectObj *extent, int layer_type) {
  double scale_x,scale_y;
  int i,j,outi,outj,last_x,last_y;

  scale_x = 256.0/(extent->maxx - extent->minx);
  scale_y = 256.0/(extent->maxy - extent->miny);

  for(i=0,outi=0;i<shape->numlines;i++) {
    for(j=0,outj=0;j<shape->line[i].numpoints;j++) {
      shape->line[outi].point[outj].x = MS_NINT((shape->line[i].point[j].x - extent->minx)*scale_x);
      shape->line[outi].point[outj].y = 256 - MS_NINT((shape->line[i].point[j].y - extent->miny)*scale_y);
      if(!outj ||
         shape->line[outi].point[outj].x != shape->line[outi].point[outj-1].x ||
         shape->line[outi].point[outj].y != shape->line[outi].point[outj-1].y )
        /* add the point to the shape only if it's the first one or if it's dofferent than the previous one */
        outj++;
    }
    if((layer_type == MS_LAYER_POLYGON && outj>=4) ||
       (layer_type == MS_LAYER_LINE && outj>=2) ||
       (layer_type == MS_LAYER_POINT && outj >= 1)) {
          /* only add the shape if it's not degenerate */
          shape->line[outi].numpoints = outj;
          outi++;
    }
  }
  /*free points of unused lines*/
  while(shape->numlines > outi)
    free(shape->line[--shape->numlines].point);
  msComputeBounds(shape);
  return (shape->numlines == 0)?MS_FAILURE:MS_SUCCESS; /*sucess if at least one line*/
}

static int mvtClipShape(shapeObj *shape, int layer_type) {
  rectObj tile_rect = {0,0,256,256};
  if(layer_type == MS_LAYER_POLYGON) {
    msClipPolygonRect(shape,tile_rect);
  } else if(layer_type == MS_LAYER_LINE) {
    msClipPolylineRect(shape,tile_rect);
  }
  if(shape->numlines>0)
    return MS_SUCCESS;
  else
    return MS_FAILURE;
}


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

int mvtWriteShape( layerObj *layer, shapeObj *shape, VectorTile__Tile__Layer *mvt_layer,
                   gmlItemListObj *item_list, value_lookup_table *value_lookup_cache) {
  VectorTile__Tile__Feature *mvt_feature;
  int i,j,iout;
  value_lookup *value;

  if(mvtTransformShape(shape, &layer->map->query.rect, layer->type) != MS_SUCCESS) {
    /* degenerate shape */
    return MS_SUCCESS;
  }
  if(mvtClipShape(shape, layer->type) != MS_SUCCESS) {
    /* no features left after clipping */
    return MS_SUCCESS;
  }

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
  mvt_feature->n_geometry = 0;
  if(layer->type == MS_LAYER_POINT) {
    for(i=0;i<shape->numlines;i++)
      mvt_feature->n_geometry += shape->line[i].numpoints * 2;
    if(mvt_feature->n_geometry)
      mvt_feature->n_geometry++; /*command*/
  } else if(layer->type == MS_LAYER_LINE) {
    for(i=0;i<shape->numlines;i++)
      mvt_feature->n_geometry += 2 + shape->line[i].numpoints * 2; /*one moveto, one lineto*/
  } else {
    for(i=0;i<shape->numlines;i++)
      mvt_feature->n_geometry += 3 + shape->line[i].numpoints * 2; /*one moveto, one lineto, one closepolygon*/
  }
  mvt_feature->geometry = msSmallMalloc(mvt_feature->n_geometry * sizeof(uint32_t));

  if(layer->type == MS_LAYER_POINT) {
    int idx = 1,lastx=0,lasty=0;;
    mvt_feature->geometry[0] = ((mvt_feature->n_geometry - 1) << 3)/*number of geoms*/|1/*moveto*/;
    for(i=0;i<shape->numlines;i++) {
      for(j=0;j<shape->line[i].numpoints;j++) {
        mvt_feature->geometry[idx++] = ZIGZAG(MS_NINT(shape->line[i].point[j].x)-lastx);
        mvt_feature->geometry[idx++] = ZIGZAG(MS_NINT(shape->line[i].point[j].y)-lasty);
        lastx = MS_NINT(shape->line[i].point[j].x);
        lasty = MS_NINT(shape->line[i].point[j].y);
      }
    }
  } else {
    int idx = 0,lastx=0,lasty=0;
    for(i=0;i<shape->numlines;i++) {
      for(j=0;j<shape->line[i].numpoints;j++) {
        if(j==0) {
          mvt_feature->geometry[idx++] = (1 << 3)/*number of movetos*/|1/*moveto*/;
          mvt_feature->geometry[idx++] = ZIGZAG(MS_NINT(shape->line[i].point[0].x)-lastx);
          mvt_feature->geometry[idx++] = ZIGZAG(MS_NINT(shape->line[i].point[0].y)-lasty);
          lastx = MS_NINT(shape->line[i].point[0].x);
          lasty = MS_NINT(shape->line[i].point[0].y);
        } else {
          if(j==1) {
            mvt_feature->geometry[idx++] = ((shape->line[i].numpoints-1) << 3)/*number of linetos*/|2/*lineto*/;
            mvt_feature->geometry[idx++] = ZIGZAG(MS_NINT(shape->line[i].point[1].x)-lastx);
            mvt_feature->geometry[idx++] = ZIGZAG(MS_NINT(shape->line[i].point[1].y)-lasty);
            lastx = MS_NINT(shape->line[i].point[1].x);
            lasty = MS_NINT(shape->line[i].point[1].y);
          } else {
            mvt_feature->geometry[idx++] = ZIGZAG(MS_NINT(shape->line[i].point[j].x)-lastx);
            mvt_feature->geometry[idx++] = ZIGZAG(MS_NINT(shape->line[i].point[j].y)-lasty);
            lastx = MS_NINT(shape->line[i].point[j].x);
            lasty = MS_NINT(shape->line[i].point[j].y);
          }
        }
      }
      if(layer->type == MS_LAYER_POLYGON) {
        mvt_feature->geometry[idx++] = 7/*closepolygon*/;
      }
    }
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
    int i,layer_type;
    layerObj *layer = GET_LAYER(map, iLayer);
    shapeObj resultshape;
    gmlItemListObj *item_list = NULL;
    int  reproject = MS_FALSE;
    VectorTile__Tile__Layer *mvt_layer;
    value_lookup_table value_lookup_cache = {NULL};
    value_lookup *cur_value_lookup, *tmp_value_lookup;

    if( !layer->resultcache || layer->resultcache->numresults == 0 )
      continue;

    if(layer->type != MS_LAYER_POINT && layer->type != MS_LAYER_POLYGON && layer->type != MS_LAYER_LINE)
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
    mvt_layer->extent = 256;
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

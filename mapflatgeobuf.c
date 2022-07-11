/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implements support for FlatGeobuf access.
 * Authors:  Björn Harrtell
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

#define NEED_IGNORE_RET_VAL

#include <limits.h>
#include <assert.h>
#include "mapserver.h"
#include "mapows.h"

#include "flatgeobuf/flatgeobuf_c.h"

#include <cpl_conv.h>
#include <ogr_srs_api.h>

static void msFGBPassThroughFieldDefinitions(layerObj *layer, flatgeobuf_ctx *ctx)
{
  for (int i = 0; i < ctx->columns_len; i++) {
    char item[255];
    char gml_width[32], gml_precision[32];
    const char *gml_type = NULL;

    const flatgeobuf_column* column = &(ctx->columns[i]);
    strncpy(item, column->name, 255 - 1);

    gml_width[0] = '\0';
    gml_precision[0] = '\0';

    switch( column->type ) {
      case flatgeobuf_column_type_byte:
      case flatgeobuf_column_type_ubyte:
      case flatgeobuf_column_type_bool:
      case flatgeobuf_column_type_short:
      case flatgeobuf_column_type_ushort:
      case flatgeobuf_column_type_int:
      case flatgeobuf_column_type_uint:
        gml_type = "Integer";
        sprintf( gml_width, "%d", 4 );
        break;
      case flatgeobuf_column_type_long:
      case flatgeobuf_column_type_ulong:
        gml_type = "Long";
        sprintf( gml_width, "%d", 8 );
        break;
      case flatgeobuf_column_type_float:
      case flatgeobuf_column_type_double:
        gml_type = "Real";
        sprintf( gml_width, "%d", 8 );
        sprintf( gml_precision, "%d", 15 );
        break;
      case flatgeobuf_column_type_string:
      case flatgeobuf_column_type_json:
      case flatgeobuf_column_type_datetime:
      default:
        gml_type = "Character";
        sprintf( gml_width, "%d", 4096 );
        break;
    }

    msUpdateGMLFieldMetadata(layer, item, gml_type, gml_width, gml_precision, 0);
  }
}

void msFlatGeobufLayerFreeItemInfo(layerObj *layer)
{
  if (layer->iteminfo) {
    free(layer->iteminfo);
    layer->iteminfo = NULL;
  }
}

int msFlatGeobufLayerInitItemInfo(layerObj *layer)
{
  if(!layer->layerinfo) {
    msSetError(MS_FGBERR, "FlatGeobuf layer has not been opened.", "msFlatGeobufLayerInitItemInfo()");
    return MS_FAILURE;
  }

  msFlatGeobufLayerFreeItemInfo(layer);

  flatgeobuf_ctx *ctx;
  ctx = layer->layerinfo;
  if (!ctx)
    return MS_FAILURE;

  for (int j = 0; j < ctx->columns_len; j++)
  {
    ctx->columns[j].itemindex = -1;
    for (int i = 0; i < layer->numitems; i++)
    {
        if (strcasecmp(layer->items[i], ctx->columns[j].name) == 0)
        {
            ctx->columns[j].itemindex = i;
            break;
        }
    }
  }

  return MS_SUCCESS;
}

int msFlatGeobufLayerOpen(layerObj *layer)
{
  char szPath[MS_MAXPATHLEN];
  int ret;

  if(layer->layerinfo)
    return MS_SUCCESS;

  if (msCheckParentPointer(layer->map,"map") == MS_FAILURE)
    return MS_FAILURE;

  flatgeobuf_ctx *ctx = flatgeobuf_init_ctx();
  layer->layerinfo = ctx;

  ctx->file = VSIFOpenL(msBuildPath(szPath, layer->map->mappath, layer->data), "rb");
  if (!ctx->file)
    ctx->file = VSIFOpenL(msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, layer->data), "rb");
  if (!ctx->file) {
    layer->layerinfo = NULL;
    flatgeobuf_free_ctx(ctx);
    return MS_FAILURE;
  }

  ret = flatgeobuf_check_magicbytes(ctx);
  if (ret == -1) {
    layer->layerinfo = NULL;
    flatgeobuf_free_ctx(ctx);
    return MS_FAILURE;
  }

  ret = flatgeobuf_decode_header(ctx);
  if (ret == -1) {
    layer->layerinfo = NULL;
    flatgeobuf_free_ctx(ctx);
    return MS_FAILURE;
  }

  if (layer->projection.numargs > 0 &&
      EQUAL(layer->projection.args[0], "auto")) {
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference( NULL );
    char *pszWKT = NULL;
    if (ctx->srid > 0) {
      if (!(OSRImportFromEPSG(hSRS, ctx->srid) != OGRERR_NONE)) {
        OSRDestroySpatialReference(hSRS);
        flatgeobuf_free_ctx(ctx);
        return MS_FAILURE;
      }
      if( OSRExportToWkt( hSRS, &pszWKT ) != OGRERR_NONE) {
        OSRDestroySpatialReference(hSRS);
        flatgeobuf_free_ctx(ctx);
        return MS_FAILURE;
      }
    } else if (ctx->wkt == NULL) {
      OSRDestroySpatialReference(hSRS);
      return MS_SUCCESS;
    }
    int bOK = MS_FALSE;
    if (msOGCWKT2ProjectionObj(ctx->wkt ? ctx->wkt : pszWKT, &(layer->projection), layer->debug) == MS_SUCCESS)
      bOK = MS_TRUE;
    CPLFree(pszWKT);
    OSRDestroySpatialReference(hSRS);
    if( bOK != MS_TRUE )
      if( layer->debug || layer->map->debug )
        msDebug( "Unable to get SRS from FlatGeobuf '%s' for layer '%s'.\n", szPath, layer->name );
  }

  return MS_SUCCESS;
}

int msFlatGeobufLayerIsOpen(layerObj *layer)
{
  if(layer->layerinfo)
    return MS_TRUE;
  else
    return MS_FALSE;
}

int msFlatGeobufLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery)
{
  (void)isQuery;
  flatgeobuf_ctx *ctx;
  ctx = layer->layerinfo;
  if (!ctx)
    return MS_FAILURE;

  if (!ctx->has_extent || !ctx->index_node_size)
    return MS_SUCCESS;

  if(msRectOverlap(&ctx->bounds, &rect) != MS_TRUE)
    return MS_DONE;

  if (msRectContained(&ctx->bounds, &rect) == MS_FALSE && ctx->index_node_size > 0)
    flatgeobuf_index_search(ctx, &rect);
  else
    flatgeobuf_index_skip(ctx);

  return MS_SUCCESS;
}

int msFlatGeobufLayerNextShape(layerObj *layer, shapeObj *shape)
{
  flatgeobuf_ctx *ctx;
  ctx = layer->layerinfo;
  if (!ctx)
    return MS_FAILURE;

  do {
    if (ctx->search_result) {
      if (ctx->search_index >= ctx->search_result_len - 1)
        return MS_DONE;
      flatgeobuf_search_item item = ctx->search_result[ctx->search_index];
      if (VSIFSeekL(ctx->file, ctx->feature_offset + item.offset, SEEK_SET) == -1) {
          msSetError(MS_FGBERR, "Unable to seek in file", "msFlatGeobufLayerNextShape");
          return MS_FAILURE;
      }
      ctx->offset = ctx->feature_offset + item.offset;
      ctx->search_index++;
      ctx->feature_index = item.index;
    }
    int ret = flatgeobuf_decode_feature(ctx, layer, shape);
    if (ret == -1)
      return MS_FAILURE;
    shape->index = ctx->feature_index;
    if (!ctx->search_result)
      ctx->feature_index++;
    if (ctx->done)
      return MS_DONE;
    if (ctx->is_null_geom) {
        msFreeCharArray(shape->values, shape->numvalues);
        shape->values = NULL;
    }
  } while(ctx->is_null_geom);

  return MS_SUCCESS;
}

int msFlatGeobufLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
  
  (void)shape;
  (void)record;
  flatgeobuf_ctx *ctx;
  ctx = layer->layerinfo;
  if (!ctx)
    return MS_FAILURE;
  long i = record->shapeindex;
  if (i < 0 || (uint64_t) i >= ctx->features_count) {
    msSetError(MS_MISCERR, "Invalid feature id", "msFlatGeobufLayerGetShape");
    return MS_FAILURE;
  }
  uint64_t offset;
  flatgeobuf_read_feature_offset(ctx, i, &offset);
  if (VSIFSeekL(ctx->file, ctx->feature_offset + offset, SEEK_SET) == -1) {
      msSetError(MS_FGBERR, "Unable to seek in file", "msFlatGeobufLayerGetShape");
      return MS_FAILURE;
  }
  int ret = flatgeobuf_decode_feature(ctx, layer, shape);
  if (ret == -1)
    return MS_FAILURE;
  return MS_SUCCESS;
}

int msFlatGeobufLayerClose(layerObj *layer)
{
  flatgeobuf_ctx *ctx;
  ctx = layer->layerinfo;
  if (!ctx)
    return MS_SUCCESS;
  VSIFCloseL(ctx->file);
  flatgeobuf_free_ctx(ctx);
  layer->layerinfo = NULL;
  return MS_SUCCESS;
}

int msFlatGeobufLayerGetItems(layerObj *layer)
{
  const char *value;
  flatgeobuf_ctx *ctx;
  ctx = layer->layerinfo;
  if (!ctx)
    return MS_FAILURE;
  layer->numitems = ctx->columns_len;

  char **items = (char **) malloc(sizeof(char *) * ctx->columns_len);
  for (int i = 0; i < ctx->columns_len; i++)
    items[i] = msStrdup(ctx->columns[i].name);
  layer->items = items;

  if((value = msOWSLookupMetadata(&(layer->metadata), "G", "types")) != NULL
      && strcasecmp(value,"auto") == 0 )
    msFGBPassThroughFieldDefinitions(layer, ctx);

  return msLayerInitItemInfo(layer);
}

int msFlatGeobufLayerGetExtent(layerObj *layer, rectObj *extent)
{
  flatgeobuf_ctx *ctx;
  ctx = layer->layerinfo;
  extent->minx = ctx->xmin;
  extent->miny = ctx->ymin;
  extent->maxx = ctx->xmax;
  extent->maxy = ctx->ymax;
  return MS_SUCCESS;
}

int msFlatGeobufLayerSupportsCommonFilters(layerObj *layer)
{
  (void)layer;
  return MS_TRUE;
}

int msFlatGeobufLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerSupportsCommonFilters = msFlatGeobufLayerSupportsCommonFilters;
  layer->vtable->LayerInitItemInfo = msFlatGeobufLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msFlatGeobufLayerFreeItemInfo;
  layer->vtable->LayerOpen = msFlatGeobufLayerOpen;
  layer->vtable->LayerIsOpen = msFlatGeobufLayerIsOpen;
  layer->vtable->LayerWhichShapes = msFlatGeobufLayerWhichShapes;
  layer->vtable->LayerNextShape = msFlatGeobufLayerNextShape;
  layer->vtable->LayerGetShape = msFlatGeobufLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */
  layer->vtable->LayerClose = msFlatGeobufLayerClose;
  layer->vtable->LayerGetItems = msFlatGeobufLayerGetItems;
  layer->vtable->LayerGetExtent = msFlatGeobufLayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerCloseConnection, use default */
  layer->vtable->LayerSetTimeFilter = msLayerMakeBackticsTimeFilter;
  /* layer->vtable->LayerTranslateFilter, use default */
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */

  return MS_SUCCESS;
}

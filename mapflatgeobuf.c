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

/*static void msFGBPassThroughFieldDefinitions(layerObj *layer)
{
  int numitems, i;

  numitems = 0;

  for(i=0; i<numitems; i++) {
    char item[16];
    int  nWidth=0, nPrecision=0;
    char gml_width[32], gml_precision[32];
    const char *gml_type = NULL;

    //eType = msDBFGetFieldInfo( hDBF, i, item, &nWidth, &nPrecision );

    gml_width[0] = '\0';
    gml_precision[0] = '\0';

    switch( eType ) {
      case FTInteger:
        gml_type = "Integer";
        sprintf( gml_width, "%d", nWidth );
        break;

      case FTDouble:
        gml_type = "Real";
        sprintf( gml_width, "%d", nWidth );
        sprintf( gml_precision, "%d", nPrecision );
        break;

      case FTString:
      default:
        gml_type = "Character";
        sprintf( gml_width, "%d", nWidth );
        break;
    }

    //msUpdateGMLFieldMetadata(layer, item, gml_type, gml_width, gml_precision, 0);

  }
}*/

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

  /* iteminfo needs to be a bit more complex, a list of indexes plus the length of the list */
  msFlatGeobufLayerFreeItemInfo(layer);
  // TODO: figure out what this is...
  //layer->iteminfo = ;
  if( ! layer->iteminfo) {
    return MS_FAILURE;
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
  if (!ctx->file) {
    layer->layerinfo = NULL;
    free(ctx);
    return MS_FAILURE;
  }

  ret = flatgeobuf_check_magicbytes(ctx);
  if (ret == -1) {
    layer->layerinfo = NULL;
    flatgeobuf_free_ctx(ctx);
    free(ctx);
    return MS_FAILURE;
  }

  ret = flatgeobuf_decode_header(ctx);
  if (ret == -1) {
    layer->layerinfo = NULL;
    flatgeobuf_free_ctx(ctx);
    free(ctx);
    return MS_FAILURE;
  }

  /*if (layer->projection.numargs > 0 &&
      EQUAL(layer->projection.args[0], "auto"))
  {
    const char* pszPRJFilename = CPLResetExtension(szPath, "prj");
    int bOK = MS_FALSE;
    VSILFILE* fp = VSIFOpenL(pszPRJFilename, "rb");
    if( fp != NULL )
    {
        char szPRJ[2048];
        OGRSpatialReferenceH hSRS;
        int nRead;

        nRead = (int)VSIFReadL(szPRJ, 1, sizeof(szPRJ) - 1, fp);
        szPRJ[nRead] = '\0';
        hSRS = OSRNewSpatialReference(szPRJ);
        if( hSRS != NULL )
        {
            if( OSRMorphFromESRI( hSRS ) == OGRERR_NONE )
            {
                char* pszWKT = NULL;
                if( OSRExportToWkt( hSRS, &pszWKT ) == OGRERR_NONE )
                {
                    if( msOGCWKT2ProjectionObj(pszWKT, &(layer->projection),
                                               layer->debug ) == MS_SUCCESS )
                    {
                        bOK = MS_TRUE;
                    }
                }
                CPLFree(pszWKT);
            }
            OSRDestroySpatialReference(hSRS);
        }
      VSIFCloseL(fp);
    }

    if( bOK != MS_TRUE )
    {
        if( layer->debug || layer->map->debug ) {
            msDebug( "Unable to get SRS from FlatGeobuf '%s' for layer '%s'.\n", szPath, layer->name );
        }
    }
  }*/

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
  }

  int ret = flatgeobuf_decode_feature(ctx, shape);
  if (ret == -1)
    return MS_FAILURE;
  if (ctx->done)
    return MS_DONE;

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
  //long i = record->shapeindex;
  // TODO: use index to find feature, else iterate to it
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
  free(layer->layerinfo);
  layer->layerinfo = NULL;
  return MS_SUCCESS;
}

int msFlatGeobufLayerGetItems(layerObj *layer)
{
  // TODO: properties decode
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

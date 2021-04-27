/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of the union layer data provider (RFC-68).
 * Author:   Tamas Szekeres (szekerest@gmail.com).
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

#define _CRT_SECURE_NO_WARNINGS 1

/* $Id$ */
#include <assert.h>
#include "mapserver.h"



#define MSUNION_NUMITEMS        3
#define MSUNION_SOURCELAYERNAME        "Union_SourceLayerName"
#define MSUNION_SOURCELAYERNAMEINDEX   -100
#define MSUNION_SOURCELAYERGROUP        "Union_SourceLayerGroup"
#define MSUNION_SOURCELAYERGROUPINDEX   -101
#define MSUNION_SOURCELAYERVISIBLE        "Union_SourceLayerVisible"
#define MSUNION_SOURCELAYERVISIBLEINDEX   -102

typedef struct {
  int layerIndex;  /* current source layer index */
  int classIndex;  /* current class index */
  char* classText;   /* current class text (autostyle) */
  int layerCount;  /* number of the source layers */
  layerObj* layers; /* structure to the source layers */
  int *status;     /* the layer status */
  int *classgroup; /* current array of the valid classes */
  int nclasses;  /* number of the valid classes */
  reprojectionObj* reprojectorSrcLayerToLayer;
  int reprojectorCurSrcLayer;
} msUnionLayerInfo;

/* Close the the combined layer */
int msUnionLayerClose(layerObj * const layer)
{
  int i;
  msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

  if (!layerinfo)
    return MS_SUCCESS;

  if (!layer->map)
    return MS_FAILURE;

  msProjectDestroyReprojector(layerinfo->reprojectorSrcLayerToLayer);
  for (i = 0; i < layerinfo->layerCount; i++) {
    layerObj* const srclayer = &layerinfo->layers[i];
    msLayerClose(srclayer);
    freeLayer(srclayer);
  }
  msFree(layerinfo->layers);
  msFree(layerinfo->status);
  msFree(layerinfo->classgroup);
  msFree(layerinfo->classText);
  msFree(layerinfo);
  layer->layerinfo = NULL;

  return MS_SUCCESS;
}

int isScaleInRange(mapObj* map, layerObj * const layer)
{
  if(map->scaledenom > 0) {
    int i;
    /* layer scale boundaries should be checked first */
    if((layer->maxscaledenom > 0) && (map->scaledenom > layer->maxscaledenom))
      return MS_FALSE;

    if((layer->minscaledenom > 0) && (map->scaledenom <= layer->minscaledenom))
      return MS_FALSE;

    /* now check class scale boundaries (all layers *must* pass these tests) */
    if(layer->numclasses > 0) {
      for(i=0; i<layer->numclasses; i++) {
        if((layer->_class[i]->maxscaledenom > 0) && (map->scaledenom > layer->_class[i]->maxscaledenom))
          continue; /* can skip this one, next class */
        if((layer->_class[i]->minscaledenom > 0) && (map->scaledenom <= layer->_class[i]->minscaledenom))
          continue; /* can skip this one, next class */

        break; /* can't skip this class (or layer for that matter) */
      }
      if(i == layer->numclasses)
        return MS_FALSE;

    }

    if (layer->maxscaledenom <= 0 && layer->minscaledenom <= 0) {
      if((layer->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > layer->maxgeowidth))
        return MS_FALSE;

      if((layer->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < layer->mingeowidth))
        return MS_FALSE;
    }
  }
  return MS_TRUE;
}

int msUnionLayerOpen(layerObj * const layer)
{
  if (layer->layerinfo != NULL) {
    return MS_SUCCESS;  /* Nothing to do... layer is already opened */
  }

  if (!layer->connection) {
    msSetError(MS_MISCERR, "The CONNECTION option is not specified for layer: %s", "msUnionLayerOpen()", layer->name);
    return MS_FAILURE;
  }

  if (!layer->map) {
    msSetError(MS_MISCERR, "No map assigned to this layer: %s", "msUnionLayerOpen()", layer->name);
    return MS_FAILURE;
  }

  mapObj* map = layer->map;

  msUnionLayerInfo* layerinfo =(msUnionLayerInfo*)calloc(1, sizeof(msUnionLayerInfo));
  MS_CHECK_ALLOC(layerinfo, sizeof(msUnionLayerInfo), MS_FAILURE);

  layer->layerinfo = layerinfo;
  layerinfo->layerIndex = 0;

  layerinfo->classgroup = NULL;
  layerinfo->nclasses = 0;

  layerinfo->layerCount = 0;

  layerinfo->classText = NULL;
  layerinfo->reprojectorCurSrcLayer = -1;

  const char* pkey = msLayerGetProcessingKey(layer, "UNION_STATUS_CHECK");
  const bool status_check = (pkey && strcasecmp(pkey, "true") == 0);

  pkey = msLayerGetProcessingKey(layer, "UNION_SCALE_CHECK");
  const bool scale_check = !(pkey && strcasecmp(pkey, "false") == 0);

  pkey = msLayerGetProcessingKey(layer, "UNION_SRCLAYER_CLOSE_CONNECTION");

  const auto layerNames = msStringSplit(layer->connection, ',');

  if (layerNames.empty()) {
    msSetError(MS_MISCERR, "No source layers specified in layer: %s", "msUnionLayerOpen()", layer->name);
    msUnionLayerClose(layer);
    return MS_FAILURE;
  }

  const int layerCount = static_cast<int>(layerNames.size());
  layerinfo->layers =(layerObj*)malloc(layerCount * sizeof(layerObj));
  MS_CHECK_ALLOC(layerinfo->layers, layerCount * sizeof(layerObj), MS_FAILURE);

  layerinfo->status =(int*)malloc(layerCount * sizeof(int));
  MS_CHECK_ALLOC(layerinfo->status, layerCount * sizeof(int), MS_FAILURE);

  for(int i=0; i < layerCount; i++) {
    const char* layerName = layerNames[i].c_str();
    const int layerindex = msGetLayerIndex(map, layerName);
    if (layerindex >= 0 && layerindex < map->numlayers) {
      const layerObj* const srclayer = map->layers[layerindex];

      if (srclayer->type != layer->type) {
        msSetError(MS_MISCERR, "The type of the source layer doesn't match with the union layer: %s", "msUnionLayerOpen()", srclayer->name);
        msUnionLayerClose(layer);
        return MS_FAILURE;
      }

      /* we need to create a new layer in order make the single pass query to work */
      layerObj* dstlayer = &layerinfo->layers[i];
      if(initLayer(dstlayer, map) == -1) {
        msSetError(MS_MISCERR, "Cannot initialize source layer: %s", "msUnionLayerOpen()", srclayer->name);
        msUnionLayerClose(layer);
        return MS_FAILURE;
      }

      ++layerinfo->layerCount;

      if (msCopyLayer(dstlayer, srclayer) != MS_SUCCESS) {
        msSetError(MS_MISCERR, "Cannot copy source layer: %s", "msUnionLayerOpen()", srclayer->name);
        msUnionLayerClose(layer);
        return MS_FAILURE;
      }

      if (pkey) {
        /* override connection flag */
        msLayerSetProcessingKey(dstlayer, "CLOSE_CONNECTION", pkey);
      }

      /* check is we should skip this source (status check) */
      if (status_check && dstlayer->status == MS_OFF) {
        layerinfo->status[i] = MS_DONE;
        continue;
      }

      /* check is we should skip this source (scale check) */
      if (scale_check && isScaleInRange(map, dstlayer) == MS_FALSE) {
        layerinfo->status[i] = MS_DONE;
        continue;
      }

      layerinfo->status[i] = msLayerOpen(dstlayer);
      if (layerinfo->status[i] != MS_SUCCESS) {
        msUnionLayerClose(layer);
        return MS_FAILURE;
      }
    } else {
      msSetError(MS_MISCERR, "Invalid layer: %s", "msUnionLayerOpen()", layerName);
      msUnionLayerClose(layer);
      return MS_FAILURE;
    }
  }

  return MS_SUCCESS;
}

/* Return MS_TRUE if layer is open, MS_FALSE otherwise. */
int msUnionLayerIsOpen(layerObj * const layer)
{
  if (layer->layerinfo)
    return(MS_TRUE);
  else
    return(MS_FALSE);
}

/* Free the itemindexes array in a layer. */
void msUnionLayerFreeItemInfo(layerObj * const layer)
{
  int i;
  msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

  if (!layerinfo || !layer->map)
    return;

  msFree(layer->iteminfo);

  layer->iteminfo = NULL;

  for (i = 0; i < layerinfo->layerCount; i++) {
    msLayerFreeItemInfo(&(layerinfo->layers[i]));
    if(layerinfo->layers[i].items) {
      /* need to remove the source layer items */
      msFreeCharArray(layerinfo->layers[i].items, layerinfo->layers[i].numitems);
      layerinfo->layers[i].items = NULL;
      layerinfo->layers[i].numitems = 0;
    }
  }
}

/* clean up expression tokens */
void msUnionLayerFreeExpressionTokens(layerObj * const layer)
{
  int i,j;
  msFreeExpressionTokens(&(layer->filter));
  msFreeExpressionTokens(&(layer->cluster.group));
  msFreeExpressionTokens(&(layer->cluster.filter));
  for(i=0; i<layer->numclasses; i++) {
    msFreeExpressionTokens(&(layer->_class[i]->expression));
    msFreeExpressionTokens(&(layer->_class[i]->text));
    for(j=0; j<layer->_class[i]->numstyles; j++)
      msFreeExpressionTokens(&(layer->_class[i]->styles[j]->_geomtransform));
  }
}

/* allocate the iteminfo index array - same order as the item list */
int msUnionLayerInitItemInfo(layerObj * const layer)
{
  int i, numitems;
  int *itemindexes;
  char* itemlist = NULL;

  msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

  if(layer->numitems == 0) {
    return MS_SUCCESS;
  }

  if (!layerinfo || !layer->map)
    return MS_FAILURE;

  /* Cleanup any previous item selection */
  msUnionLayerFreeItemInfo(layer);

  layer->iteminfo = (int *) malloc(sizeof(int) * layer->numitems);
  MS_CHECK_ALLOC(layer->iteminfo, sizeof(int) * layer->numitems, MS_FAILURE);

  itemindexes = (int*)layer->iteminfo;

  /* check whether we require attributes from the source layers also */
  numitems = 0;
  for (i = 0; i < layer->numitems; i++) {
    if (EQUAL(layer->items[i], MSUNION_SOURCELAYERNAME))
      itemindexes[i] = MSUNION_SOURCELAYERNAMEINDEX;
    else if (EQUAL(layer->items[i], MSUNION_SOURCELAYERGROUP))
      itemindexes[i] = MSUNION_SOURCELAYERGROUPINDEX;
    else if (EQUAL(layer->items[i], MSUNION_SOURCELAYERVISIBLE))
      itemindexes[i] = MSUNION_SOURCELAYERVISIBLEINDEX;
    else {
      itemindexes[i] = numitems++;
      if (itemlist) {
        itemlist = msStringConcatenate(itemlist, ",");
        itemlist = msStringConcatenate(itemlist, layer->items[i]);
      } else {
        itemlist = msStrdup(layer->items[i]);
      }
    }
  }

  for (i = 0; i < layerinfo->layerCount; i++) {
    layerObj* const srclayer = &(layerinfo->layers[i]);

    if (layerinfo->status[i] != MS_SUCCESS)
      continue; /* skip empty layers */

    msUnionLayerFreeExpressionTokens(srclayer);

    if (itemlist) {
      /* get items requested by the union layer plus the required items */
      msLayerSetProcessingKey(srclayer, "ITEMS", itemlist);
      if (msLayerWhichItems(srclayer, MS_TRUE, NULL) != MS_SUCCESS) {
        msFree(itemlist);
        return MS_FAILURE;
      }
    } else {
      /* get only the required items */
      if (msLayerWhichItems(srclayer, MS_FALSE, NULL) != MS_SUCCESS)
        return MS_FAILURE;
    }
  }

  msFree(itemlist);
  return MS_SUCCESS;
}

// cppcheck-suppress passedByValue
int msUnionLayerWhichShapes(layerObj * const layer, rectObj rect, int isQuery)
{
  msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

  if (!layerinfo || !layer->map)
    return MS_FAILURE;

  for (int i = 0; i < layerinfo->layerCount; i++) {
    layerObj* const srclayer = &layerinfo->layers[i];

    if (layerinfo->status[i] != MS_SUCCESS)
      continue; /* skip empty layers */

    if (layer->styleitem && layer->numitems == 0) {
      /* need to initialize items */
      msUnionLayerFreeExpressionTokens(srclayer);

      /* get only the required items */
      if (msLayerWhichItems(srclayer, MS_FALSE, NULL) != MS_SUCCESS)
        return MS_FAILURE;
    }

    rectObj srcRect = rect;

    if(srclayer->transform == MS_TRUE && srclayer->project && layer->transform == MS_TRUE && layer->project &&msProjectionsDiffer(&(srclayer->projection), &(layer->projection)))
      msProjectRect(&layer->projection, &srclayer->projection, &srcRect); /* project the searchrect to source coords */

    layerinfo->status[i] = msLayerWhichShapes(srclayer, srcRect, isQuery);
    if (layerinfo->status[i] == MS_FAILURE)
      return MS_FAILURE;
  }

  layerinfo->layerIndex = 0;
  layerObj* const srclayer = &layerinfo->layers[0];

  msFree(layerinfo->classgroup);

  layerinfo->classgroup = NULL;
  layerinfo->nclasses = 0;

  if (srclayer->classgroup && srclayer->numclasses > 0)
    layerinfo->classgroup = msAllocateValidClassGroups(srclayer, &layerinfo->nclasses);

  return MS_SUCCESS;
}

static int BuildFeatureAttributes(layerObj * const layer, const layerObj* srclayer, shapeObj *shape)
{
  const int* const itemindexes = static_cast<const int*>(layer->iteminfo);

  char** values = static_cast<char**>(msSmallMalloc(sizeof(char*) * (layer->numitems)));
  MS_CHECK_ALLOC(values, layer->numitems * sizeof(char*), MS_FAILURE);;

  for (int i = 0; i < layer->numitems; i++) {
    if (itemindexes[i] == MSUNION_SOURCELAYERNAMEINDEX)
      values[i] = msStrdup(srclayer->name);
    else if (itemindexes[i] == MSUNION_SOURCELAYERGROUPINDEX)
      values[i] = msStrdup(srclayer->group);
    else if (itemindexes[i] == MSUNION_SOURCELAYERVISIBLEINDEX) {
      if (srclayer->status == MS_OFF)
        values[i] = msStrdup("0");
      else
        values[i] = msStrdup("1");
    } else if (shape->values[itemindexes[i]])
      values[i] = msStrdup(shape->values[itemindexes[i]]);
    else
      values[i] = msStrdup("");
  }

  if (shape->values)
    msFreeCharArray(shape->values, shape->numvalues);

  shape->values = values;
  shape->numvalues = layer->numitems;

  return MS_SUCCESS;
}

/* find the next shape with the appropriate shape type */
/* also, load in the attribute data */
/* MS_DONE => no more data */
int msUnionLayerNextShape(layerObj * const layer, shapeObj *shape)
{
  msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

  if (!layerinfo || !layer->map)
    return MS_FAILURE;

  if (layerinfo->layerIndex < 0 || layerinfo->layerIndex >= layerinfo->layerCount)
    return MS_FAILURE;

  int rv = MS_DONE;

  while (layerinfo->layerIndex < layerinfo->layerCount) {
    layerObj* srclayer = &layerinfo->layers[layerinfo->layerIndex];
    if (layerinfo->status[layerinfo->layerIndex] == MS_SUCCESS) {
      while ((rv = srclayer->vtable->LayerNextShape(srclayer, shape)) == MS_SUCCESS) {
        if(layer->styleitem) {
          /* need to retrieve the source layer classindex if styleitem AUTO is set */
          layerinfo->classIndex = msShapeGetClass(srclayer, layer->map, shape, layerinfo->classgroup, layerinfo->nclasses);
          if(layerinfo->classIndex < 0 || layerinfo->classIndex >= srclayer->numclasses) {
            /*  this shape is not visible, skip it */
            msFreeShape(shape);
            continue;
          }
          if(srclayer->styleitem && strcasecmp(srclayer->styleitem, "AUTO") != 0) {
            /* Generic feature style handling as per RFC-61 */
            msLayerGetFeatureStyle(layer->map, srclayer, srclayer->_class[layerinfo->classIndex], shape);
          }
          /* set up annotation */
          msFree(layerinfo->classText);
          layerinfo->classText = NULL;
          if(srclayer->_class[layerinfo->classIndex]->numlabels > 0) {
            /* pull text from the first label only */
            if(msGetLabelStatus(layer->map,layer,shape,srclayer->_class[layerinfo->classIndex]->labels[0]) == MS_ON) {
              layerinfo->classText = msShapeGetLabelAnnotation(layer,shape,srclayer->_class[layerinfo->classIndex]->labels[0]);
            }
          }
        }

        /* reproject to the target layer */
        if( layerinfo->reprojectorCurSrcLayer != layerinfo->layerIndex )
        {
            msProjectDestroyReprojector(layerinfo->reprojectorSrcLayerToLayer);
            layerinfo->reprojectorSrcLayerToLayer = NULL;
            layerinfo->reprojectorCurSrcLayer = layerinfo->layerIndex;
            if(srclayer->project && msProjectionsDiffer(&(srclayer->projection), &(layer->projection)))
                layerinfo->reprojectorSrcLayerToLayer = msProjectCreateReprojector(&(srclayer->projection), &(layer->projection));
            else
                srclayer->project = MS_FALSE;
        }
        if(layerinfo->reprojectorSrcLayerToLayer)
            msProjectShapeEx(layerinfo->reprojectorSrcLayerToLayer, shape);

        /* update the layer styles with the bound values */
        if(msBindLayerToShape(srclayer, shape, MS_FALSE) != MS_SUCCESS)
          return MS_FAILURE;

        shape->tileindex = layerinfo->layerIndex;

        /* construct the item array */
        if (layer->iteminfo)
          rv = BuildFeatureAttributes(layer, srclayer, shape);

        /* check the layer filter condition */
        if(layer->filter.string != NULL && layer->numitems > 0 && layer->iteminfo) {
          if (layer->filter.type == MS_EXPRESSION && layer->filter.tokens == NULL)
            msTokenizeExpression(&(layer->filter), layer->items, &(layer->numitems));

          if (!msEvalExpression(layer, shape, &(layer->filter), layer->filteritemindex)) {
            /* this shape is filtered */
            msFreeShape(shape);
            continue;
          }
        }

        return rv;
      }
    }

    ++layerinfo->layerIndex;
    if (layerinfo->layerIndex == layerinfo->layerCount) {
      layerinfo->layerIndex = 0;
      return MS_DONE;
    }

    /* allocate the classgroups for the next layer */
    msFree(layerinfo->classgroup);

    layerinfo->classgroup = NULL;
    layerinfo->nclasses = 0;

    if (srclayer->classgroup && srclayer->numclasses > 0)
      layerinfo->classgroup = msAllocateValidClassGroups(srclayer, &layerinfo->nclasses);
  }

  return rv;
}

/* Random access of the feature. */
int msUnionLayerGetShape(layerObj * const layer, shapeObj *shape, resultObj *record)
{
  const long tile = record->tileindex;
  msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

  if (!layerinfo || !layer->map)
    return MS_FAILURE;

  if (tile < 0 || tile >= layerinfo->layerCount) {
    msSetError(MS_MISCERR, "Invalid tile index: %s", "msUnionLayerGetShape()", layer->name);
    return MS_FAILURE;
  }

  layerObj* const srclayer = &layerinfo->layers[tile];
  record->tileindex = 0;
  int rv = srclayer->vtable->LayerGetShape(srclayer, shape, record);
  record->tileindex = tile;

  if (rv == MS_SUCCESS) {
    /* reproject to the target layer */
    if( layerinfo->reprojectorCurSrcLayer != tile )
    {
        msProjectDestroyReprojector(layerinfo->reprojectorSrcLayerToLayer);
        layerinfo->reprojectorSrcLayerToLayer = NULL;
        layerinfo->reprojectorCurSrcLayer = tile;
        if(srclayer->project && msProjectionsDiffer(&(srclayer->projection), &(layer->projection)))
            layerinfo->reprojectorSrcLayerToLayer = msProjectCreateReprojector(&(srclayer->projection), &(layer->projection));
        else
            srclayer->project = MS_FALSE;
    }
    if(layerinfo->reprojectorSrcLayerToLayer)
      msProjectShapeEx(layerinfo->reprojectorSrcLayerToLayer, shape);

    shape->tileindex = tile;

    /* construct the item array */
    if (layer->iteminfo)
      rv = BuildFeatureAttributes(layer, srclayer, shape);
  }

  return rv;
}

/* Query for the items collection */
int msUnionLayerGetItems(layerObj * const layer)
{
  /* we support certain built in attributes */
  layer->numitems = 2;
  layer->items = static_cast<char**>(msSmallMalloc(sizeof(char*) * (layer->numitems)));
  MS_CHECK_ALLOC(layer->items, layer->numitems * sizeof(char*), MS_FAILURE);
  layer->items[0] = msStrdup(MSUNION_SOURCELAYERNAME);
  layer->items[1] = msStrdup(MSUNION_SOURCELAYERGROUP);

  return msUnionLayerInitItemInfo(layer);
}

int msUnionLayerGetNumFeatures(layerObj * const layer)
{
  msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

  if (!layerinfo || !layer->map)
    return 0;

  int numFeatures = 0;

  for (int i = 0; i < layerinfo->layerCount; i++) {
    if (layerinfo->status[i] != MS_SUCCESS)
      continue; /* skip empty layers */

    int c = msLayerGetNumFeatures(&(layerinfo->layers[i]));
    if (c > 0)
      numFeatures += c;
  }

  return numFeatures;
}

static int msUnionLayerGetAutoStyle(mapObj *map, layerObj * const layer, classObj *c, shapeObj* shape)
{
  msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

  if (!layerinfo || !layer->map)
    return MS_FAILURE;

  if (shape->tileindex < 0 || shape->tileindex >= layerinfo->layerCount) {
    msSetError(MS_MISCERR, "Invalid tile index: %s", "msUnionLayerGetAutoStyle()", layer->name);
    return MS_FAILURE;
  }

  layerObj* const srclayer = &layerinfo->layers[shape->tileindex];

  if(srclayer->styleitem && strcasecmp(srclayer->styleitem, "AUTO") == 0) {
    const int tileindex = shape->tileindex;
    shape->tileindex = 0;
    int rv = msLayerGetAutoStyle(map, srclayer, c, shape);
    shape->tileindex = tileindex;
    return rv;
  } else {
    const classObj* src = srclayer->_class[layerinfo->classIndex];
    /* copy the style from the current class index */
    /* free any previous styles on the dst layer */

    resetClassStyle(c);

    for (int i = 0; i < src->numstyles; i++) {
      if (msMaybeAllocateClassStyle(c, i))
        return MS_FAILURE;

      if (msCopyStyle(c->styles[i], src->styles[i]) != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy style.", "msUnionLayerGetAutoStyle()");
        return MS_FAILURE;
      }
      /* remove the bindings on the style */
      for(int j=0; j<MS_STYLE_BINDING_LENGTH; j++) {
        msFree(c->styles[i]->bindings[j].item);
        c->styles[i]->bindings[j].item = NULL;
      }
      c->styles[i]->numbindings = 0;
    }

    for (int i = 0; i < src->numlabels; i++) {
      // RFC77 TODO: allocation need to be done, but is the right way (from mapcopy.c)?
      if (msGrowClassLabels(c) == NULL)
        return MS_FAILURE;
      initLabel(c->labels[i]);

      if (msCopyLabel(c->labels[i], src->labels[i]) != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy label.", "msUnionLayerGetAutoStyle()");
        return MS_FAILURE;
      }

      /* remove the bindings on the label */
      for(int j=0; j<MS_LABEL_BINDING_LENGTH; j++) {
        msFree(c->labels[i]->bindings[j].item);
        c->labels[i]->bindings[j].item = NULL;
      }
      c->labels[i]->numbindings = 0;
    }
    c->numlabels = src->numlabels;

    c->layer = layer;
    c->text.string = layerinfo->classText;
    layerinfo->classText = NULL;
  }
  return MS_SUCCESS;
}

int msUnionLayerCopyVirtualTable(layerVTableObj* vtable)
{
  vtable->LayerInitItemInfo = msUnionLayerInitItemInfo;
  vtable->LayerFreeItemInfo = msUnionLayerFreeItemInfo;
  vtable->LayerOpen = msUnionLayerOpen;
  vtable->LayerIsOpen = msUnionLayerIsOpen;
  vtable->LayerWhichShapes = msUnionLayerWhichShapes;
  vtable->LayerNextShape = msUnionLayerNextShape;
  vtable->LayerGetShape = msUnionLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */
  vtable->LayerClose = msUnionLayerClose;

  vtable->LayerGetItems = msUnionLayerGetItems;
  vtable->LayerCloseConnection = msUnionLayerClose;
  vtable->LayerGetAutoStyle = msUnionLayerGetAutoStyle;

  vtable->LayerGetNumFeatures = msUnionLayerGetNumFeatures;

  return MS_SUCCESS;
}

int msUnionLayerInitializeVirtualTable(layerObj * const layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  return msUnionLayerCopyVirtualTable(layer->vtable);
}

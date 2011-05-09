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

MS_CVSID("$Id$")

#define MSUNION_NUMITEMS        3
#define MSUNION_SOURCELAYERNAME        "Union:SourceLayerName"
#define MSUNION_SOURCELAYERNAMEINDEX   -100
#define MSUNION_SOURCELAYERGROUP        "Union:SourceLayerGroup"
#define MSUNION_SOURCELAYERGROUPINDEX   -101
#define MSUNION_SOURCELAYERVISIBLE        "Union:SourceLayerVisible"
#define MSUNION_SOURCELAYERVISIBLEINDEX   -102

typedef struct
{
    int layerIndex;  /* current source layer index */
    int classIndex;  /* current class index */
    int layerCount;  /* number of the source layers */
    layerObj* layers; /* structure to the source layers */
    int *status;     /* the layer status */
    int *classgroup; /* current array of the valid classes */
    int nclasses;  /* number of the valid classes */
} msUnionLayerInfo;

/* Close the the combined layer */
int msUnionLayerClose(layerObj *layer)
{   
    int i;
    msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

    if (!layerinfo)
        return MS_SUCCESS;

    if (!layer->map)
        return MS_FAILURE;

    for (i = 0; i < layerinfo->layerCount; i++)
    {
        msLayerClose(&layerinfo->layers[i]);
        freeLayer(&layerinfo->layers[i]);
    }
    msFree(layerinfo->layers);
    msFree(layerinfo->status);
    msFree(layerinfo->classgroup);
    msFree(layerinfo);
    layer->layerinfo = NULL;

    return MS_SUCCESS;
}

int msUnionLayerOpen(layerObj *layer)
{
    msUnionLayerInfo *layerinfo;
    char **layerNames;
    mapObj* map;
    int i;
    int layerCount;

    if (layer->layerinfo != NULL)
    {
      return MS_SUCCESS;  // Nothing to do... layer is already opened
    }
    
    if (!layer->connection)
    {
        msSetError(MS_MISCERR, "The CONNECTION option is not specified for layer: %s", layer->name);
        return MS_FAILURE;
    }

    if (!layer->map)
    {
        msSetError(MS_MISCERR, "No map assigned to this layer: %s", layer->name);
        return MS_FAILURE;
    }

    map = layer->map;

    layerinfo =(msUnionLayerInfo*)malloc(sizeof(msUnionLayerInfo));
    MS_CHECK_ALLOC(layerinfo, sizeof(msUnionLayerInfo), MS_FAILURE);

    layer->layerinfo = layerinfo;
    layerinfo->layerIndex = 0;

    layerinfo->classgroup = NULL;
    layerinfo->nclasses = 0;

    layerinfo->layerCount = 0;
    
    layerNames = msStringSplit(layer->connection, ',', &layerCount);

    if (layerCount == 0)
    {
        msSetError(MS_MISCERR, "No source layers specified in layer: %s", layer->name);
        if(layerNames)
                msFreeCharArray(layerNames, layerinfo->layerCount);
        msUnionLayerClose(layer);
        return MS_FAILURE;
    }

    layerinfo->layers =(layerObj*)malloc(layerCount * sizeof(layerObj));
    MS_CHECK_ALLOC(layerinfo->layers, layerCount * sizeof(layerObj), MS_FAILURE);

    layerinfo->status =(layerObj*)malloc(layerCount * sizeof(int));
    MS_CHECK_ALLOC(layerinfo->status, layerCount * sizeof(int), MS_FAILURE);

    for(i=0; i < layerCount; i++)
    {
        int layerindex = msGetLayerIndex(map, layerNames[i]);
        if (layerindex >= 0 && layerindex < map->numlayers)
        {
            layerObj* srclayer = map->layers[layerindex];

            if (srclayer->type != layer->type)
            {
                msSetError(MS_MISCERR, "The type of the source layer doesn't match with the union layer: %s", srclayer->name);
                if(layerNames)
                        msFreeCharArray(layerNames, layerinfo->layerCount);
                msUnionLayerClose(layer);
                return MS_FAILURE;
            }

            /* we need to create a new layer in order make the singlepass query to work */
            if(initLayer(&layerinfo->layers[i], map) == -1) 
            {
		        msSetError(MS_MISCERR, "Cannot initialize source layer: %s", srclayer->name);
                if(layerNames)
                        msFreeCharArray(layerNames, layerinfo->layerCount);
                msUnionLayerClose(layer);
                return MS_FAILURE;
            }

            ++layerinfo->layerCount; 

	        if (msCopyLayer(&layerinfo->layers[i], srclayer) != MS_SUCCESS)
            {
		        msSetError(MS_MISCERR, "Cannot copy source layer: %s", srclayer->name);
                if(layerNames)
                        msFreeCharArray(layerNames, layerinfo->layerCount);
                msUnionLayerClose(layer);
                return MS_FAILURE;
            }

            /* disable the connection pool for this layer */
            msLayerSetProcessingKey(&layerinfo->layers[i], "CLOSE_CONNECTION", "ALWAYS");

            layerinfo->status[i] = msLayerOpen(&layerinfo->layers[i]);
            if (layerinfo->status[i] != MS_SUCCESS)
            {
                if(layerNames)
                    msFreeCharArray(layerNames, layerinfo->layerCount);
                msUnionLayerClose(layer);
                return MS_FAILURE;
            }
        }
        else
        {
            msSetError(MS_MISCERR, "Invalid layer: %s", layerNames[i]);
            if(layerNames)
                msFreeCharArray(layerNames, layerinfo->layerCount);
            msUnionLayerClose(layer);
            return MS_FAILURE;
        }
    }

    if(layerNames)
          msFreeCharArray(layerNames, layerinfo->layerCount);
    
    return MS_SUCCESS;
}

/* Return MS_TRUE if layer is open, MS_FALSE otherwise. */
int msUnionLayerIsOpen(layerObj *layer)
{
    if (layer->layerinfo)
        return(MS_TRUE);
    else
        return(MS_FALSE);
}

/* Free the itemindexes array in a layer. */
void msUnionLayerFreeItemInfo(layerObj *layer)
{
    int i;
    msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

    if (!layerinfo || !layer->map)
        return;

    msFree(layer->iteminfo);

    layer->iteminfo = NULL;

    for (i = 0; i < layerinfo->layerCount; i++)
    {
        msLayerFreeItemInfo(&layerinfo->layers[i]);
        if(layerinfo->layers[i].items) 
        {
            /* need to remove the source layer items */
            msFreeCharArray(layerinfo->layers[i].items, layerinfo->layers[i].numitems);
            layerinfo->layers[i].items = NULL;
            layerinfo->layers[i].numitems = 0;
        }
    }
}

/* allocate the iteminfo index array - same order as the item list */
int msUnionLayerInitItemInfo(layerObj *layer)
{
    int i, j, numitems;
    int *itemindexes;

    msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

    if(layer->numitems == 0)
    {
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
    for (i = 0; i < layer->numitems; i++)
    {
        if (EQUAL(layer->items[i], MSUNION_SOURCELAYERNAME))
            itemindexes[i] = MSUNION_SOURCELAYERNAMEINDEX;
        else if (EQUAL(layer->items[i], MSUNION_SOURCELAYERGROUP))
            itemindexes[i] = MSUNION_SOURCELAYERGROUPINDEX;
        else if (EQUAL(layer->items[i], MSUNION_SOURCELAYERVISIBLE))
            itemindexes[i] = MSUNION_SOURCELAYERVISIBLEINDEX;
        else
            itemindexes[i] = numitems++;
    }

    for (i = 0; i < layerinfo->layerCount; i++)
    {
        layerObj* srclayer = &layerinfo->layers[i];
         /* Cleanup any previous item selection */
        msLayerFreeItemInfo(srclayer);
        if(srclayer->items) 
        {
            msFreeCharArray(srclayer->items, srclayer->numitems);
            srclayer->items = NULL;
            srclayer->numitems = 0;
        }

        if (numitems > 0)
        {
            /* now allocate and set the layer item parameters  */
            srclayer->items = (char **)malloc(sizeof(char *)*numitems);
            MS_CHECK_ALLOC(srclayer->items, sizeof(char *)*numitems, MS_FAILURE);
            srclayer->numitems = numitems;
            
            for (j = 0; j < layer->numitems; j++)
            {
                if (itemindexes[j] >= 0)
                    srclayer->items[itemindexes[j]] = msStrdup(layer->items[j]);
            }

            if (msLayerInitItemInfo(srclayer) != MS_SUCCESS)
                return MS_FAILURE;
        }
    }

    return MS_SUCCESS;
}

int msUnionLayerWhichShapes(layerObj *layer, rectObj rect)
{
    int i;
    layerObj* srclayer;
    rectObj srcRect;
    msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

    if (!layerinfo || !layer->map)
        return MS_FAILURE;

    for (i = 0; i < layerinfo->layerCount; i++)
    {
        layerObj* srclayer = &layerinfo->layers[i];
        srcRect = rect;
#ifdef USE_PROJ
        if(srclayer->transform == MS_TRUE && srclayer->project && layer->transform == MS_TRUE && layer->project &&msProjectionsDiffer(&(srclayer->projection), &(layer->projection)))
            msProjectRect(&layer->projection, &srclayer->projection, &srcRect); /* project the searchrect to source coords */
#endif        
        layerinfo->status[i] = msLayerWhichShapes(srclayer, srcRect);
        if (layerinfo->status[i] == MS_FAILURE)
            return MS_FAILURE;
    }

    layerinfo->layerIndex = 0;
    srclayer = &layerinfo->layers[0];
    
    msFree(layerinfo->classgroup);

    layerinfo->classgroup = NULL;
    layerinfo->nclasses = 0;

    if (srclayer->classgroup && srclayer->numclasses > 0)
        layerinfo->classgroup = msAllocateValidClassGroups(srclayer, &layerinfo->nclasses);

    return MS_SUCCESS;
}

static int BuildFeatureAttributes(layerObj *layer, layerObj* srclayer, shapeObj *shape)
{
    int i;
    char **values;
    int* itemindexes = layer->iteminfo;
    
    if (layer->numitems == srclayer->numitems)
        return MS_SUCCESS; /* we don't have custom attributes, no need to reconstruct the array */

    values = malloc(sizeof(char*) * (layer->numitems));
    MS_CHECK_ALLOC(values, layer->numitems * sizeof(char*), MS_FAILURE);;

    for (i = 0; i < layer->numitems; i++)
    {
        if (itemindexes[i] == MSUNION_SOURCELAYERNAMEINDEX)
            values[i] = msStrdup(srclayer->name);
        else if (itemindexes[i] == MSUNION_SOURCELAYERGROUPINDEX)
            values[i] = msStrdup(srclayer->group);
        else if (itemindexes[i] == MSUNION_SOURCELAYERVISIBLEINDEX)
        {
            if (srclayer->status == MS_OFF)
                values[i] = msStrdup("0");
            else
                values[i] = msStrdup("1");
        }
        else if (shape->values[itemindexes[i]])
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
int msUnionLayerNextShape(layerObj *layer, shapeObj *shape)
{
    int rv;
    layerObj* srclayer;
    msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

    if (!layerinfo || !layer->map)
        return MS_FAILURE;

    if (layerinfo->layerIndex < 0 || layerinfo->layerIndex >= layerinfo->layerCount)
        return MS_FAILURE;

    rv = MS_DONE;

    while (layerinfo->layerIndex < layerinfo->layerCount)
    {
        if (layerinfo->status[layerinfo->layerIndex] == MS_SUCCESS)
        {
            srclayer = &layerinfo->layers[layerinfo->layerIndex];
            while ((rv = srclayer->vtable->LayerNextShape(srclayer, shape)) == MS_SUCCESS)
            {
                if(layer->styleitem) 
                {
                    /* need to retrieve the source layer classindex if styleitem AUTO is set */
                    layerinfo->classIndex = msShapeGetClass(srclayer, layer->map, shape, layerinfo->classgroup, layerinfo->nclasses);
                    if(layerinfo->classIndex < 0 || layerinfo->classIndex >= srclayer->numclasses) 
                    {
                        // this shape is not visible, skip it
                        msFreeShape(shape);
                        if (rv == MS_SUCCESS)
                            continue;
                        else 
                            break;
                    }
                    if(srclayer->styleitem && strcasecmp(srclayer->styleitem, "AUTO") != 0) 
                    {
                        /* Generic feature style handling as per RFC-61 */
                        msLayerGetFeatureStyle(layer->map, srclayer, srclayer->class[layerinfo->classIndex], shape);
                    }
                }

#ifdef USE_PROJ
                /* reproject to the target layer */
                if(srclayer->project && msProjectionsDiffer(&(srclayer->projection), &(layer->projection)))
	                msProjectShape(&(srclayer->projection), &(layer->projection), shape);
                else
	                srclayer->project = MS_FALSE;
#endif
                
                shape->tileindex = layerinfo->layerIndex;

                /* construct the item array */
                if (layer->iteminfo)
                    rv = BuildFeatureAttributes(layer, srclayer, shape);

                /* check the layer filter condition */
                if(layer->filter.string != NULL && layer->numitems > 0 && layer->iteminfo)
                {
                    if (layer->filter.type == MS_EXPRESSION && layer->filter.tokens == NULL)
                        msTokenizeExpression(&(layer->filter), layer->items, &(layer->numitems));

                    if (!msEvalExpression(layer, shape, &(layer->filter), layer->filteritemindex)) 
                    {
                        /* this shape is filtered */
                        msFreeShape(shape);
                        continue;
                    }
                }

                return rv;
            }
        }

        ++layerinfo->layerIndex;
        if (layerinfo->layerIndex == layerinfo->layerCount)
        {
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
int msUnionLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
	int rv;
    layerObj* srclayer;
    long tile = record->tileindex;
    msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

    if (!layerinfo || !layer->map)
        return MS_FAILURE;

    if (tile < 0 || tile >= layerinfo->layerCount)
    {
        msSetError(MS_MISCERR, "Invalid tile index: %s", layer->name);
        return MS_FAILURE;
    }

    srclayer = &layerinfo->layers[tile];
    record->tileindex = 0;
    rv = srclayer->vtable->LayerGetShape(srclayer, shape, record);
    record->tileindex = tile;

    if (rv == MS_SUCCESS)
    {
#ifdef USE_PROJ
            /* reproject to the target layer */
            if(srclayer->project && msProjectionsDiffer(&(srclayer->projection), &(layer->projection)))
	            msProjectShape(&(srclayer->projection), &(layer->projection), shape);
            else
	            srclayer->project = MS_FALSE;
#endif
        shape->tileindex = tile;

        /* construct the item array */
        if (layer->iteminfo)
            rv = BuildFeatureAttributes(layer, srclayer, shape);
    }

    return rv;
}

/* Query for the items collection */
int msUnionLayerGetItems(layerObj *layer)
{
    /* we support certain built in attributes */
    layer->numitems = 2;
    layer->items = malloc(sizeof(char*) * (layer->numitems));
    MS_CHECK_ALLOC(layer->items, layer->numitems * sizeof(char*), MS_FAILURE);
    layer->items[0] = msStrdup(MSUNION_SOURCELAYERNAME);
    layer->items[1] = msStrdup(MSUNION_SOURCELAYERGROUP);

    return msUnionLayerInitItemInfo(layer);
}

int msUnionLayerGetNumFeatures(layerObj *layer)
{
    int i, c, numFeatures;
    msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

    if (!layerinfo || !layer->map)
        return 0;

    numFeatures = 0;

    for (i = 0; i < layerinfo->layerCount; i++)
    {
        c = msLayerGetNumFeatures(&layerinfo->layers[i]);
        if (c > 0)
            numFeatures += c;
    }

    return numFeatures;
}

static int msUnionLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c, shapeObj* shape)
{
    layerObj* srclayer;
    msUnionLayerInfo* layerinfo = (msUnionLayerInfo*)layer->layerinfo;

    if (!layerinfo || !layer->map)
        return MS_FAILURE;

    if (shape->tileindex < 0 || shape->tileindex >= layerinfo->layerCount)
    {
        msSetError(MS_MISCERR, "Invalid tile index: %s", layer->name);
        return MS_FAILURE;
    }

    srclayer = &layerinfo->layers[shape->tileindex];

    if(srclayer->styleitem && strcasecmp(srclayer->styleitem, "AUTO") == 0) 
    {
        int rv;
        int tileindex = shape->tileindex;
        shape->tileindex = 0;
        rv = msLayerGetAutoStyle(map, srclayer, c, shape);
        shape->tileindex = tileindex;
        return rv;
    }
    else
    {
        int i;
        classObj* src = srclayer->class[layerinfo->classIndex];
        /* copy the style from the current class index */
        /* free any previous styles on the dst layer */

        resetClassStyle(c);

        for (i = 0; i < src->numstyles; i++) 
        {
            if (msMaybeAllocateClassStyle(c, i))
                return MS_FAILURE;

            if (msCopyStyle(c->styles[i], src->styles[i]) != MS_SUCCESS) 
            {
                msSetError(MS_MEMERR, "Failed to copy style.", "msUnionLayerGetAutoStyle()");
                return MS_FAILURE;
            }
        }

        if (msCopyLabel(&(c->label), &(src->label)) != MS_SUCCESS) 
        {
            msSetError(MS_MEMERR, "Failed to copy label.", "msCopyClass()");
            return MS_FAILURE;
        }

        c->type = src->type;
        c->layer = layer;
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

    vtable->LayerClose = msUnionLayerClose;

    vtable->LayerGetItems = msUnionLayerGetItems;
    vtable->LayerCloseConnection = msUnionLayerClose;
    vtable->LayerGetAutoStyle = msUnionLayerGetAutoStyle;
    
    vtable->LayerGetNumFeatures = msUnionLayerGetNumFeatures;

    return MS_SUCCESS;
}

int msUnionLayerInitializeVirtualTable(layerObj *layer)
{
    assert(layer != NULL);
    assert(layer->vtable != NULL);

    return msUnionLayerCopyVirtualTable(layer->vtable);
}

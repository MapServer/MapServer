/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Implementation of plug-in layer functionality
 * Author:   Jani Averbach, SRC,LLC  <javerbach@extendthereach.com>
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
 */

#include "map.h"
#include "mapthread.h"

MS_CVSID("$Id$");

typedef struct {
    char *name;
    layerVTableObj vtable;
} VTFactoryItemObj;

typedef struct {
    unsigned int size;
    unsigned int first_free;
    VTFactoryItemObj * vtItems[MS_MAXLAYERS];
} VTFactoryObj;

static VTFactoryObj gVirtualTableFactory = {MS_MAXLAYERS, 0, {NULL}};


static VTFactoryItemObj *
createVTFItem(const char *name)
{
    VTFactoryItemObj *pVTFI;

    pVTFI = (VTFactoryItemObj *)malloc(sizeof(VTFactoryItemObj));
    if ( ! pVTFI) {
        return NULL;
    }
    pVTFI->name = strdup(name);
    memset(&pVTFI->vtable, 0, sizeof(layerVTableObj));

    return pVTFI;
}

static void
destroyVTFItem(VTFactoryItemObj **pVTFI)
{
    free((*pVTFI)->name);
    (*pVTFI)->name = NULL;
    memset(&(*pVTFI)->vtable, 0, sizeof(layerVTableObj));
    free(*pVTFI);
    *pVTFI = NULL;
}


static VTFactoryItemObj *
lookupVTFItem(VTFactoryObj *VTFactory,
              const char *key)
{
    unsigned int i;
    for (i=0; i < VTFactory->size && VTFactory->vtItems[i]; ++i) {
        if (0 == strcasecmp(key, VTFactory->vtItems[i]->name)) {
            return VTFactory->vtItems[i];
        }
    }
    return NULL;
}

static int
insertNewVTFItem(VTFactoryObj *pVTFactory, 
                 VTFactoryItemObj *pVTFI)
{
    unsigned int first_free = pVTFactory->first_free;
    if (first_free < pVTFactory->size) {
        pVTFactory->vtItems[first_free] = pVTFI;
        pVTFactory->first_free++;
        return MS_SUCCESS;
    }
    return MS_FAILURE;
}

static VTFactoryItemObj *
loadCustomLayerDLL(layerObj *layer, const char *library_path)
{
    int (*pfnPluginInitVTable)(layerVTableObj *, layerObj *);

    VTFactoryItemObj *pVTFI;

    pfnPluginInitVTable = msGetSymbol(library_path, "PluginInitializeVirtualTable");
    if ( ! pfnPluginInitVTable) {
        msSetError(MS_MISCERR, "Failed to load dynamic Layer LIB: %s", "loadCustomLayerDLL", library_path);
        return NULL;
    }

    pVTFI = createVTFItem(library_path);
    if ( ! pVTFI) {
        return NULL;
    }
    
    if (pfnPluginInitVTable(&pVTFI->vtable, layer)) {
        destroyVTFItem(&pVTFI);
        msSetError(MS_MISCERR, "Failed to initialize dynamic Layer: %s", "loadCustomLayerDLL", library_path);
        return NULL;
    }
    return pVTFI;
}

/*
 * copyVirtualTable
 *
 * copy one virtual table from dest to src. Both of dest and src
 * have to be allocated already.
 *
 * If src contains NULL fields, those are NOT copied over.
 *
 * Because of that, it is possible for plugin layer to use default 
 * layer API default functions,  just leave those function pointers to NULL.
 */
static void
copyVirtualTable(layerVTableObj *dest, 
                 const layerVTableObj *src)
{
    dest->LayerInitItemInfo = src->LayerInitItemInfo ? src->LayerInitItemInfo : dest->LayerInitItemInfo;
    dest->LayerFreeItemInfo = src->LayerFreeItemInfo ? src->LayerFreeItemInfo : dest->LayerFreeItemInfo;
    dest->LayerOpen = src->LayerOpen ? src->LayerOpen : dest->LayerOpen;
    dest->LayerIsOpen = src->LayerIsOpen ? src->LayerIsOpen : dest->LayerIsOpen;
    dest->LayerWhichShapes = src->LayerWhichShapes ? src->LayerWhichShapes : dest->LayerWhichShapes;
    dest->LayerNextShape = src->LayerNextShape ? src->LayerNextShape : dest->LayerNextShape;
    dest->LayerGetShape = src->LayerGetShape ? src->LayerGetShape : dest->LayerGetShape;
    dest->LayerClose = src->LayerClose ? src->LayerClose : dest->LayerClose;
    dest->LayerGetItems = src->LayerGetItems ? src->LayerGetItems : dest->LayerGetItems;
    dest->LayerGetExtent = src->LayerGetExtent ? src->LayerGetExtent : dest->LayerGetExtent;
    dest->LayerGetAutoStyle = src->LayerGetAutoStyle ? src->LayerGetAutoStyle : dest->LayerGetAutoStyle;
    dest->LayerCloseConnection = src->LayerCloseConnection ? src->LayerCloseConnection : dest->LayerCloseConnection;
    dest->LayerSetTimeFilter = src->LayerSetTimeFilter ? src->LayerSetTimeFilter : dest->LayerSetTimeFilter;
    dest->LayerApplyFilterToLayer = src->LayerApplyFilterToLayer ? src->LayerApplyFilterToLayer : dest->LayerApplyFilterToLayer;
    dest->LayerCreateItems = src->LayerCreateItems ? src->LayerCreateItems : dest->LayerCreateItems;
    dest->LayerGetNumFeatures = src->LayerGetNumFeatures ? src->LayerGetNumFeatures : dest->LayerGetNumFeatures;
}

int
msPluginLayerInitializeVirtualTable(layerObj *layer)
{
    VTFactoryItemObj *pVTFI;

    msAcquireLock(TLOCK_LAYER_VTABLE);
    
    pVTFI = lookupVTFItem(&gVirtualTableFactory, layer->plugin_library);
    if ( ! pVTFI) {
        pVTFI = loadCustomLayerDLL(layer, layer->plugin_library);
        if ( ! pVTFI) {
            msReleaseLock(TLOCK_LAYER_VTABLE);
            return MS_FAILURE;            
        }
        if (insertNewVTFItem(&gVirtualTableFactory, pVTFI)) {
            destroyVTFItem(&pVTFI);
            msReleaseLock(TLOCK_LAYER_VTABLE);
            return MS_FAILURE;
        }
    }
    msReleaseLock(TLOCK_LAYER_VTABLE);

    copyVirtualTable(layer->vtable, &pVTFI->vtable);
    return MS_SUCCESS;
}

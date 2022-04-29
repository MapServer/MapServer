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

#include <cpl_conv.h>
#include <ogr_srs_api.h>

FGBHandle msFGBOpenVirtualFile( VSILFILE * fpFGB)
{
}

/************************************************************************/
/*                              msFGBOpen()                             */
/*                                                                      */
/*      Open the FlatGeobuf file.                                       */
/************************************************************************/
FGBHandle msFGBOpen(const char * pszLayer)
{
  VSILFILE *fpFGB = VSIFOpenL(pszLayer, "rb");
  return msFGBOpenVirtualFile(fpFGB);
}

void msFGBClose(FGBHandle psFGB)
{
  VSIFCloseL( psFGB->fpFGB );
  free( psFGB );
}

int msFGBReadBounds( FGBHandle psFGB, int hEntity, rectObj *padBounds)
{
  // TODO: get rect

  return MS_SUCCESS;
}

int msFlatGeobufOpenHandle(flatgeobufObj *fgbfile, const char *filename, FGBHandle hFGB)
{
  assert(filename != NULL);
  assert(hFGB != NULL);

  /* initialize a few things */
  fgbfile->status = NULL;
  fgbfile->lastshape = -1;
  fgbfile->isopen = MS_FALSE;

  fgbfile->hFGB = hFGB;

  strlcpy(fgbfile->source, filename, sizeof(fgbfile->source));

  if( fgbfile->numshapes < 0 || fgbfile->numshapes > 256000000 ) {
    msSetError(MS_FGBERR, "Corrupted FlatGeobuf : numshapes = %d.",
               "msFlatGeobufOpen()", fgbfile->numshapes);
    msFGBClose(hFGB);
    return -1;
  }

  msFGBReadBounds( fgbfile->hFGB, -1, &(fgbfile->bounds));

  fgbfile->isopen = MS_TRUE;
  return(0); /* all o.k. */
}

int msFlatGeobufOpenVirtualFile(flatgeobufObj *fgbfile, const char *filename, VSILFILE * fpFGB, int log_failures)
{
  assert(filename != NULL);
  assert(fpFGB != NULL);

  /* open the file and get basic info */
  FGBHandle hFGB = msFGBOpenVirtualFile(fpFGB);
  if(!hFGB) {
    if( log_failures )
      msSetError(MS_IOERR, "(%s)", "msFlatGeobufOpenVirtualFile()", filename);
    return(-1);
  }

  return msFlatGeobufOpenHandle(fgbfile, filename, hFGB);
}

int msFlatGeobufOpen(flatgeobufObj *fgbfile, const char *filename, int log_failures)
{
  int i;
  char *dbfFilename;
  size_t bufferSize = 0;

  if(!filename) {
    if( log_failures )
      msSetError(MS_IOERR, "No (NULL) filename provided.", "msFlatGeobufOpen()");
    return(-1);
  }

  /* open the file and get basic info */
  FGBHandle hFGB = msFGBOpen(filename);

  if(!hFGB) {
    if( log_failures )
      msSetError(MS_IOERR, "(%s)", "msFlatGeobufOpen()", filename);
    return(-1);
  }

  return msFlatGeobufOpenHandle(fgbfile, filename, hFGB);
}

void msFlatGeobufClose(flatgeobufObj *fgbfile)
{
  if (fgbfile && fgbfile->isopen == MS_TRUE) { /* Silently return if called with NULL fgbfile by freeLayer() */
    if(fgbfile->hFGB) msFGBClose(fgbfile->hFGB);
    free(fgbfile->status);
    fgbfile->isopen = MS_FALSE;
  }
}

/* status array lives in the fgbfile, can return MS_SUCCESS/MS_FAILURE/MS_DONE */
int msFlatGeobufWhichShapes(flatgeobufObj *fgbfile, rectObj rect, int debug)
{
  int i;
  rectObj shaperect;
  char *filename;

  free(fgbfile->status);
  fgbfile->status = NULL;

  /* rect and source rect DON'T overlap... */
  if(msRectOverlap(&fgbfile->bounds, &rect) != MS_TRUE)
    return(MS_DONE);

  if(msRectContained(&fgbfile->bounds, &rect) == MS_TRUE) {
    fgbfile->status = msAllocBitArray(fgbfile->numshapes);
    if(!fgbfile->status) {
      msSetError(MS_MEMERR, NULL, "msFlatGeobufWhichShapes()");
      return(MS_FAILURE);
    }
    msSetAllBits(fgbfile->status, fgbfile->numshapes, 1);
  } else {
    // TODO: index search?
  }

  fgbfile->lastshape = -1;

  return(MS_SUCCESS); /* success */
}

static void msFGBPassThroughFieldDefinitions(layerObj *layer)
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

    /*switch( eType ) {
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
    }*/

    //msUpdateGMLFieldMetadata(layer, item, gml_type, gml_width, gml_precision, 0);

  }
}

void msFlatGeobufLayerFreeItemInfo(layerObj *layer)
{
  if(layer->iteminfo) {
    free(layer->iteminfo);
    layer->iteminfo = NULL;
  }
}

int msFlatGeobufLayerInitItemInfo(layerObj *layer)
{
  flatgeobufObj *fgbfile = layer->layerinfo;
  if( ! fgbfile) {
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
  flatgeobufObj *fgbfile;

  if(layer->layerinfo) return MS_SUCCESS; /* layer already open */

  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;

  /* allocate space for a flatgeobufObj using layer->layerinfo  */
  fgbfile = (flatgeobufObj *) malloc(sizeof(flatgeobufObj));
  MS_CHECK_ALLOC(fgbfile, sizeof(flatgeobufObj), MS_FAILURE);

  layer->layerinfo = fgbfile;

  if(msFlatGeobufOpen(fgbfile, msBuildPath3(szPath, layer->map->mappath, layer->map->shapepath, layer->data), MS_TRUE) == -1) {
    if(msFlatGeobufOpen(fgbfile, msBuildPath(szPath, layer->map->mappath, layer->data), MS_TRUE) == -1) {
      layer->layerinfo = NULL;
      free(fgbfile);
      return MS_FAILURE;
    }
  }

  if (layer->projection.numargs > 0 &&
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
  int status;
  flatgeobufObj *fgbfile;

  fgbfile = layer->layerinfo;

  if(!fgbfile) {
    msSetError(MS_FGBERR, "FlatGeobuf layer has not been opened.", "msFlatGeobufLayerWhichShapes()");
    return MS_FAILURE;
  }

  status = msFlatGeobufWhichShapes(fgbfile, rect, layer->debug);
  if(status != MS_SUCCESS) {
    return status;
  }

  return MS_SUCCESS;
}

int msFlatGeobufLayerNextShape(layerObj *layer, shapeObj *shape)
{
  return MS_SUCCESS;
}

int msFlatGeobufLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
  return MS_SUCCESS;
}

int msFlatGeobufLayerClose(layerObj *layer)
{
  flatgeobufObj *fgbfile;
  fgbfile = layer->layerinfo;
  if(!fgbfile) return MS_SUCCESS; /* nothing to do */

  msFlatGeobufClose(fgbfile);
  free(layer->layerinfo);
  layer->layerinfo = NULL;

  return MS_SUCCESS;
}

int msFlatGeobufLayerGetItems(layerObj *layer)
{
  flatgeobufObj *fgbfile;
  const char *value;

  fgbfile = layer->layerinfo;

  if(!fgbfile) {
    msSetError(MS_FGBERR, "FlatGeobuf layer has not been opened.", "msFlatGeobufLayerGetItems()");
    return MS_FAILURE;
  }

  // TODO: populate
  //layer->numitems = msFGBGetFieldCount(fgbfile->hFGB);
  //layer->items = msFGBGetItems(fgbfile->hFGB);
  layer->numitems = 0;
  if(layer->numitems == 0) return MS_SUCCESS; /* No items is a valid case (#3147) */
  if(!layer->items) return MS_FAILURE;

  /* -------------------------------------------------------------------- */
  /*      consider populating the field definitions in metadata.          */
  /* -------------------------------------------------------------------- */
  if((value = msOWSLookupMetadata(&(layer->metadata), "G", "types")) != NULL
      && strcasecmp(value,"auto") == 0 )
    msFGBPassThroughFieldDefinitions(layer);

  return msLayerInitItemInfo(layer);
}

int msFlatGeobufLayerGetExtent(layerObj *layer, rectObj *extent)
{
  *extent = ((flatgeobufObj*)layer->layerinfo)->bounds;
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

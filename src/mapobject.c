/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Functions for operating on a mapObj that don't belong in a
 *           more specific file such as mapfile.c, or mapdraw.c.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam
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

#include "gdal.h"
#include "cpl_conv.h"

void freeWeb(webObj *web);
void freeScalebar(scalebarObj *scalebar);
void freeReferenceMap(referenceMapObj *ref);
void freeLegend(legendObj *legend);

/************************************************************************/
/*                            msNewMapObj()                             */
/*                                                                      */
/*      Create a new initialized map object.                            */
/************************************************************************/

mapObj *msNewMapObj()
{
  mapObj *map = NULL;

  /* create an empty map, no layers etc... */
  map = (mapObj *)calloc(sizeof(mapObj),1);

  if(!map) {
    msSetError(MS_MEMERR, NULL, "msCreateMap()");
    return NULL;
  }

  if( initMap( map ) == -1 ) {
    msFreeMap(map);
    return NULL;
  }

  if( msPostMapParseOutputFormatSetup( map ) == MS_FAILURE ) {
    msFreeMap(map);
    return NULL;
  }

  return map;
}

/************************************************************************/
/*                             msFreeMap()                              */
/************************************************************************/

void msFreeMap(mapObj *map)
{
  int i;

  if(!map) return;

  /* printf("msFreeMap(): maybe freeing map at %p count=%d.\n",map, map->refcount); */
  if(MS_REFCNT_DECR_IS_NOT_ZERO(map)) {
    return;
  }
  if(map->debug >= MS_DEBUGLEVEL_VV)
    msDebug("msFreeMap(): freeing map at %p.\n",map);

  msCloseConnections(map);

  msFree(map->name);
  msFree(map->shapepath);
  msFree(map->mappath);

  msFreeProjection(&(map->projection));
  msFreeProjection(&(map->latlon));
  msProjectionContextReleaseToPool(map->projContext);

  msFreeLabelCache(&(map->labelcache));

  msFree(map->imagetype);

  msFreeFontSet(&(map->fontset));

  msFreeSymbolSet(&map->symbolset); /* free symbols */
  msFree(map->symbolset.filename);

  freeWeb(&(map->web));

  freeScalebar(&(map->scalebar));
  freeReferenceMap(&(map->reference));
  freeLegend(&(map->legend));

  for(i=0; i<map->maxlayers; i++) {
    if(GET_LAYER(map, i) != NULL) {
      GET_LAYER(map, i)->map = NULL;
      if(freeLayer((GET_LAYER(map, i))) == MS_SUCCESS)
        free(GET_LAYER(map, i));
    }
  }
  msFree(map->layers);

  if(map->layerorder)
    free(map->layerorder);

  msFree(map->templatepattern);
  msFree(map->datapattern);
  msFreeHashItems(&(map->configoptions));
  if(map->outputformat && map->outputformat->refcount > 0 && --map->outputformat->refcount < 1)
    msFreeOutputFormat(map->outputformat);

  for(i=0; i<map->numoutputformats; i++ ) {
    if(map->outputformatlist[i]->refcount > 0 && --map->outputformatlist[i]->refcount < 1)
      msFreeOutputFormat(map->outputformatlist[i]);
  }
  if(map->outputformatlist != NULL)
    msFree(map->outputformatlist);

  msFreeQuery(&(map->query));

#ifdef USE_V8_MAPSCRIPT
  if (map->v8context)
    msV8FreeContext(map);
#endif

  msFree(map);
}

/************************************************************************/
/*                         msGetConfigOption()                          */
/************************************************************************/

const char *msGetConfigOption( mapObj *map, const char *key)

{
  return msLookupHashTable( &(map->configoptions), key );
}

/************************************************************************/
/*                         msSetConfigOption()                          */
/************************************************************************/

int msSetConfigOption( mapObj *map, const char *key, const char *value)

{
  /* We have special "early" handling of this so that it will be */
  /* in effect when the projection blocks are parsed and pj_init is called. */
  if( strcasecmp(key,"PROJ_DATA") == 0 || strcasecmp(key,"PROJ_LIB") == 0 ) {
    /* value may be relative to map path */
    msSetPROJ_DATA( value, map->mappath );
  }

  /* Same for MS_ERRORFILE, we want it to kick in as early as possible
   * to catch parsing errors.
   * Value can be relative to mapfile, unless it's already absolute
   */
  if( strcasecmp(key,"MS_ERRORFILE") == 0 ) {
    if (msSetErrorFile( value, map->mappath ) != MS_SUCCESS)
      return MS_FAILURE;
  }

  if( msLookupHashTable( &(map->configoptions), key ) != NULL )
    msRemoveHashTable( &(map->configoptions), key );
  msInsertHashTable( &(map->configoptions), key, value );

  return MS_SUCCESS;
}

/************************************************************************/
/*                         msTestConfigOption()                         */
/************************************************************************/

int msTestConfigOption( mapObj *map, const char *key, int default_result )

{
  const char *result = msGetConfigOption( map, key );

  if( result == NULL )
    return default_result;

  if( strcasecmp(result,"YES") == 0
      || strcasecmp(result,"ON") == 0
      || strcasecmp(result,"TRUE") == 0 )
    return MS_TRUE;
  else
    return MS_FALSE;
}

/************************************************************************/
/*                      msApplyMapConfigOptions()                       */
/************************************************************************/

void msApplyMapConfigOptions( mapObj *map )

{
  const char *key;

  for( key = msFirstKeyFromHashTable( &(map->configoptions) );
       key != NULL;
       key = msNextKeyFromHashTable( &(map->configoptions), key ) ) {
    const char *value = msLookupHashTable( &(map->configoptions), key );
    if( strcasecmp(key,"PROJ_DATA") == 0 ||
        strcasecmp(key,"PROJ_LIB") == 0 ) {
      msSetPROJ_DATA( value, map->mappath );
    } else if( strcasecmp(key,"MS_ERRORFILE") == 0 ) {
      msSetErrorFile( value, map->mappath );
    } else {
      CPLSetConfigOption( key, value );
    }
  }
}

/************************************************************************/
/*                         msMapIgnoreMissingData()                               */
/************************************************************************/

int msMapIgnoreMissingData( mapObj *map )
{
  const char *result = msGetConfigOption( map, "ON_MISSING_DATA" );
  const int default_result =
#ifndef IGNORE_MISSING_DATA
    MS_MISSING_DATA_FAIL;
#else
    MS_MISSING_DATA_LOG;
#endif

  if( result == NULL )
    return default_result;

  if( strcasecmp(result,"FAIL") == 0 )
    return MS_MISSING_DATA_FAIL;
  else if( strcasecmp(result,"LOG") == 0 )
    return MS_MISSING_DATA_LOG;
  else if( strcasecmp(result,"IGNORE") == 0 )
    return MS_MISSING_DATA_IGNORE;

  return default_result;
}

/************************************************************************/
/*                           msMapSetExtent()                           */
/************************************************************************/

int msMapSetExtent( mapObj *map,
                    double minx, double miny, double maxx, double maxy)
{

  map->extent.minx = minx;
  map->extent.miny = miny;
  map->extent.maxx = maxx;
  map->extent.maxy = maxy;

  if (!MS_VALID_EXTENT(map->extent)) {
    msSetError(MS_MISCERR, "Given map extent is invalid. Check that it " \
               "is in the form: minx, miny, maxx, maxy", "setExtent()");
    return MS_FAILURE;
  }

  map->cellsize = msAdjustExtent(&(map->extent), map->width,
                                 map->height);

  /* if the map size is also set, recompute scale, ignore errors? */
  if( map->width != -1 || map->height != -1 )
    msCalculateScale(map->extent, map->units, map->width, map->height,
                     map->resolution, &(map->scaledenom));

  return msMapComputeGeotransform( map );
}

/************************************************************************/
/*                           msMapOffsetExtent()                        */
/************************************************************************/

int msMapOffsetExtent( mapObj *map, double x, double y)
{
  return msMapSetExtent( map,
                         map->extent.minx + x, map->extent.miny + y,
                         map->extent.maxx + x, map->extent.maxy + y);
}

/************************************************************************/
/*                           msMapScaleExtent()                         */
/************************************************************************/

int msMapScaleExtent( mapObj *map, double zoomfactor,
                      double minscaledenom, double maxscaledenom)
{
  double geo_width, geo_height, center_x, center_y, md;

  if (zoomfactor <= 0.0) {
    msSetError(MS_MISCERR, "The given zoomfactor is invalid", "msMapScaleExtent()");
  }

  geo_width = map->extent.maxx - map->extent.minx;
  geo_height = map->extent.maxy - map->extent.miny;

  center_x = map->extent.minx + geo_width * 0.5;
  center_y = map->extent.miny + geo_height * 0.5;

  geo_width *= zoomfactor;

  if (minscaledenom > 0 || maxscaledenom > 0) {
    /* ensure we are within the valid scale domain */
    md = (map->width-1)/(map->resolution * msInchesPerUnit(map->units, center_y));
    if (minscaledenom > 0 && geo_width < minscaledenom * md)
      geo_width = minscaledenom * md;
    if (maxscaledenom > 0 && geo_width > maxscaledenom * md)
      geo_width = maxscaledenom * md;
  }

  geo_width *= 0.5;
  geo_height = geo_width * map->height / map->width;

  return msMapSetExtent( map,
                         center_x - geo_width, center_y - geo_height,
                         center_x + geo_width, center_y + geo_height);
}

/************************************************************************/
/*                           msMapSetCenter()                           */
/************************************************************************/

int msMapSetCenter( mapObj *map, pointObj *center)
{
  return msMapOffsetExtent(map, center->x - (map->extent.minx + map->extent.maxx) * 0.5,
                           center->y - (map->extent.miny + map->extent.maxy) * 0.5);
}

/************************************************************************/
/*                           msMapSetRotation()                         */
/************************************************************************/

int msMapSetRotation( mapObj *map, double rotation_angle )

{
  map->gt.rotation_angle = rotation_angle;
  if( map->gt.rotation_angle != 0.0 )
    map->gt.need_geotransform = MS_TRUE;
  else
    map->gt.need_geotransform = MS_FALSE;

  return msMapComputeGeotransform( map );
}

/************************************************************************/
/*                             msMapSetSize()                           */
/************************************************************************/

int msMapSetSize( mapObj *map, int width, int height )

{
  map->width = width;
  map->height = height;

  return msMapComputeGeotransform( map ); /* like SetRotation -- sean */
}

/************************************************************************/
/*                      msMapComputeGeotransform()                      */
/************************************************************************/

extern int InvGeoTransform( double *gt_in, double *gt_out );

int msMapComputeGeotransformEx( mapObj * map, double resolutionX, double resolutionY )

{
  double rot_angle;
  double geo_width, geo_height, center_x, center_y;

  map->saved_extent = map->extent;

  rot_angle = map->gt.rotation_angle * MS_PI / 180.0;

  geo_width = map->extent.maxx - map->extent.minx;
  geo_height = map->extent.maxy - map->extent.miny;

  center_x = map->extent.minx + geo_width*0.5;
  center_y = map->extent.miny + geo_height*0.5;

  /*
  ** Per bug 1916 we have to adjust for the fact that map extents
  ** are based on the center of the edge pixels, not the outer
  ** edges as is expected in a geotransform.
  */
  map->gt.geotransform[1] =
    cos(rot_angle) * resolutionX;
  map->gt.geotransform[2] =
    sin(rot_angle) * resolutionY;
  map->gt.geotransform[0] = center_x
                            - (map->width * 0.5) * map->gt.geotransform[1]
                            - (map->height * 0.5) * map->gt.geotransform[2];

  map->gt.geotransform[4] =
    sin(rot_angle) * resolutionX;
  map->gt.geotransform[5] =
    - cos(rot_angle) * resolutionY;
  map->gt.geotransform[3] = center_y
                            - (map->width * 0.5) * map->gt.geotransform[4]
                            - (map->height * 0.5) * map->gt.geotransform[5];

  if( InvGeoTransform( map->gt.geotransform,
                       map->gt.invgeotransform ) )
    return MS_SUCCESS;
  else
    return MS_FAILURE;
}

int msMapComputeGeotransform( mapObj * map )

{
  /* Do we have all required parameters? */
  if( map->extent.minx == map->extent.maxx
      || map->width <= 1 || map->height <= 1 )
    return MS_FAILURE;

  const double geo_width = map->extent.maxx - map->extent.minx;
  const double geo_height = map->extent.maxy - map->extent.miny;
  return msMapComputeGeotransformEx(map,
                                    geo_width / (map->width-1),
                                    geo_height / (map->height-1));
}

/************************************************************************/
/*                         msMapPixelToGeoref()                         */
/************************************************************************/

void msMapPixelToGeoref( mapObj *map, double *x, double *y )

{
  (void)map;
  (void)x;
  (void)y;
  msSetError(MS_MISCERR, NULL, "msMapPixelToGeoref() not yet implemented");
}

/************************************************************************/
/*                         msMapGeorefToPixel()                         */
/************************************************************************/

void msMapGeorefToPixel( mapObj *map, double *x, double *y )

{
  (void)map;
  (void)x;
  (void)y;
  msSetError(MS_MISCERR, NULL, "msMapGeorefToPixel() not yet implemented");
}

/************************************************************************/
/*                        msMapSetFakedExtent()                         */
/************************************************************************/

int msMapSetFakedExtent( mapObj *map )

{
  int i;
  /* -------------------------------------------------------------------- */
  /*      Remember the original map extents so we can restore them        */
  /*      later.                                                          */
  /* -------------------------------------------------------------------- */
  map->saved_extent = map->extent;

  /* -------------------------------------------------------------------- */
  /*      Set extents such that the bottom left corner is 0,0 and the     */
  /*      top right is width,height.  Note this is upside down from       */
  /*      the normal sense of pixel/line coordiantes, but we do this      */
  /*      so that the normal "extent" concept of coordinates              */
  /*      increasing to the right, and up is maintained (like in          */
  /*      georeferenced coordinate systems).                              */
  /* -------------------------------------------------------------------- */
  map->extent.minx = 0;
  map->extent.maxx = map->width;
  map->extent.miny = 0;
  map->extent.maxy = map->height;
  map->cellsize = 1.0;

  /* -------------------------------------------------------------------- */
  /*      When we copy the geotransform into the projection object we     */
  /*      have to flip it to account for the preceeding upside-down       */
  /*      coordinate system.                                              */
  /* -------------------------------------------------------------------- */
  map->projection.gt = map->gt;

  map->projection.gt.geotransform[0]
  += map->height * map->gt.geotransform[2];
  map->projection.gt.geotransform[3]
  += map->height * map->gt.geotransform[5];

  map->projection.gt.geotransform[2] *= -1;
  map->projection.gt.geotransform[5] *= -1;

  for(i=0; i<map->numlayers; i++)
    GET_LAYER(map, i)->project = MS_TRUE;

  return InvGeoTransform( map->projection.gt.geotransform,
                          map->projection.gt.invgeotransform );
}

/************************************************************************/
/*                      msMapRestoreRealExtent()                        */
/************************************************************************/

int msMapRestoreRealExtent( mapObj *map )

{
  map->projection.gt.need_geotransform = MS_FALSE;
  map->extent = map->saved_extent;
  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);

  return MS_SUCCESS;
}

/************************************************************************/
/*                      msInsertLayer()                                 */
/************************************************************************/
/* Returns the index at which the layer was inserted
 */

int msInsertLayer(mapObj *map, layerObj *layer, int nIndex)
{
  if (!layer) {
    msSetError(MS_CHILDERR, "Can't insert a NULL Layer", "msInsertLayer()");
    return -1;
  }

  /* Ensure there is room for a new layer */
  if (map->numlayers == map->maxlayers) {
    if (msGrowMapLayers(map) == NULL)
      return -1;
  }

  /* msGrowMapLayers allocates the new layer which we don't need to do since we have 1 that we are inserting
                  not sure if it is possible for this to be non null otherwise, but better to check since this function
                  replaces the value */
  if (map->layers[map->numlayers]!=NULL)
    free(map->layers[map->numlayers]);

  /* Catch attempt to insert past end of layers array */
  if (nIndex >= map->numlayers) {
    msSetError(MS_CHILDERR, "Cannot insert layer beyond index %d",
               "msInsertLayer()", map->numlayers-1);
    return -1;
  } else if (nIndex < 0) { /* Insert at the end by default */
    map->layerorder[map->numlayers] = map->numlayers;
    GET_LAYER(map, map->numlayers) = layer;
    GET_LAYER(map, map->numlayers)->index = map->numlayers;
    GET_LAYER(map, map->numlayers)->map = map;
    MS_REFCNT_INCR(layer);
    map->numlayers++;
    return map->numlayers-1;
  } else  {
    /* Move existing layers at the specified nIndex or greater */
    /* to an index one higher */
    int i;
    for (i=map->numlayers; i>nIndex; i--) {
      GET_LAYER(map, i)=GET_LAYER(map, i-1);
      GET_LAYER(map, i)->index = i;
    }

    /* assign new layer to specified index */
    GET_LAYER(map, nIndex)=layer;
    GET_LAYER(map, nIndex)->index = nIndex;
    GET_LAYER(map, nIndex)->map = map;

    /* adjust layers drawing order */
    for (i=map->numlayers; i>nIndex; i--) {
      map->layerorder[i] = map->layerorder[i-1];
      if (map->layerorder[i] >= nIndex) map->layerorder[i]++;
    }
    for (i=0; i<nIndex; i++) {
      if (map->layerorder[i] >= nIndex) map->layerorder[i]++;
    }
    map->layerorder[nIndex] = nIndex;

    /* increment number of layers and return */
    MS_REFCNT_INCR(layer);
    map->numlayers++;
    return nIndex;
  }
}

/************************************************************************/
/*                           msRemoveLayer()                            */
/************************************************************************/
layerObj *msRemoveLayer(mapObj *map, int nIndex)
{
  int i;
  int order_index;
  layerObj *layer;

  if (nIndex < 0 || nIndex >= map->numlayers) {
    msSetError(MS_CHILDERR, "Cannot remove Layer, invalid index %d",
               "msRemoveLayer()", nIndex);
    return NULL;
  } else {
    layer=GET_LAYER(map, nIndex);
    /* msCopyLayer(layer, (GET_LAYER(map, nIndex))); */

    /* Iteratively copy the higher index layers down one index */
    for (i=nIndex; i<map->numlayers-1; i++) {
      /* freeLayer((GET_LAYER(map, i))); */
      /* initLayer((GET_LAYER(map, i)), map); */
      /* msCopyLayer(GET_LAYER(map, i), GET_LAYER(map, i+1)); */
      GET_LAYER(map, i)=GET_LAYER(map, i+1);
      GET_LAYER(map, i)->index = i;
    }
    /* Free the extra layer at the end */
    /* freeLayer((GET_LAYER(map, map->numlayers-1))); */
    GET_LAYER(map, map->numlayers-1)=NULL;

    /* Adjust drawing order */
    order_index = 0;
    for (i=0; i<map->numlayers; i++) {
      if (map->layerorder[i] > nIndex) map->layerorder[i]--;
      if (map->layerorder[i] == nIndex) {
        order_index = i;
        break;
      }
    }
    for (i=order_index; i<map->numlayers-1; i++) {
      map->layerorder[i] = map->layerorder[i+1];
      if (map->layerorder[i] > nIndex) map->layerorder[i]--;
    }

    /* decrement number of layers and return copy of removed layer */
    map->numlayers--;
    layer->map=NULL;
    MS_REFCNT_DECR(layer);
    return layer;
  }
}

/*
** Move the layer's order for drawing purpose. Moving it up here
** will have the effect of drawing the layer earlier.
*/
int msMoveLayerUp(mapObj *map, int nLayerIndex)
{
  int iCurrentIndex = -1;
  if (map && nLayerIndex < map->numlayers && nLayerIndex >=0) {
    for (int i=0; i<map->numlayers; i++) {
      if ( map->layerorder[i] == nLayerIndex) {
        iCurrentIndex = i;
        break;
      }
    }
    if (iCurrentIndex >=0) {
      /* we do not need to promote if it is the first one. */
      if (iCurrentIndex == 0)
        return MS_FAILURE;

      map->layerorder[iCurrentIndex] =
        map->layerorder[iCurrentIndex-1];
      map->layerorder[iCurrentIndex-1] = nLayerIndex;

      return MS_SUCCESS;
    }
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveLayerUp()",
             nLayerIndex);
  return MS_FAILURE;
}

/*
** Move the layer's order for drawing purpose. Moving it down here
** will have the effect of drawing the layer later.
*/
int msMoveLayerDown(mapObj *map, int nLayerIndex)
{
  int iCurrentIndex = -1;
  if (map && nLayerIndex < map->numlayers && nLayerIndex >=0) {
    for (int i=0; i<map->numlayers; i++) {
      if ( map->layerorder[i] == nLayerIndex) {
        iCurrentIndex = i;
        break;
      }
    }
    if (iCurrentIndex >=0) {
      /* we do not need to demote if it is the last one. */
      if (iCurrentIndex == map->numlayers-1)
        return MS_FAILURE;

      map->layerorder[iCurrentIndex] =
        map->layerorder[iCurrentIndex+1];
      map->layerorder[iCurrentIndex+1] = nLayerIndex;

      return MS_SUCCESS;
    }
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveLayerDown()",
             nLayerIndex);
  return MS_FAILURE;
}


/*
** Set the array used for the drawing order. The array passed must contain
** all the layer's index ordered by the drawing priority.
** Ex : for 3 layers in the map file, if
**                          panIndexes[0] = 2
**                          panIndexes[1] = 0
**                          panIndexes[2] = 1
**                          will set the darwing order to layer 2, layer 0,
**                          and then layer 1.
**
** Note : It is assumed that the index panIndexes has the same number of
**        of elements as the number of layers in the map.
** Return TRUE on success else FALSE.
*/
int msSetLayersdrawingOrder(mapObj *self, int *panIndexes)
{
  if (self && panIndexes) {
    const int nElements = self->numlayers;
    for (int i=0; i<nElements; i++) {
      int bFound = 0;
      for (int j=0; j<nElements; j++) {
        if (panIndexes[j] == i) {
          bFound = 1;
          break;
        }
      }
      if (!bFound)
        return 0;
    }
    /* -------------------------------------------------------------------- */
    /*    At this point the array is valid so update the layers order array.*/
    /* -------------------------------------------------------------------- */
    for (int i=0; i<nElements; i++) {
      self->layerorder[i] = panIndexes[i];
    }
    return 1;
  }
  return 0;
}


/* =========================================================================
   msMapLoadOWSParameters

   Function to support mapscript mapObj::loadOWSParameters
   ========================================================================= */

int msMapLoadOWSParameters(mapObj *map, cgiRequestObj *request,
                           const char *wmtver)
{
#ifdef USE_WMS_SVR
  int version;
  char *wms_exception_format = NULL;
  const char *wms_request= NULL;
  int result, i = 0;
  owsRequestObj ows_request;

  msOWSInitRequestObj(&ows_request);


  version = msOWSParseVersionString(wmtver);
  for(i=0; i<request->NumParams; i++) {
    if (strcasecmp(request->ParamNames[i], "EXCEPTIONS") == 0)
      wms_exception_format = request->ParamValues[i];
    else if (strcasecmp(request->ParamNames[i], "REQUEST") == 0)
      wms_request = request->ParamValues[i];

  }

  msOWSRequestLayersEnabled(map, "M", wms_request, &ows_request);

  result = msWMSLoadGetMapParams(map, version, request->ParamNames,
                                 request->ParamValues, request->NumParams,  wms_exception_format,
                                 wms_request, &ows_request);

  msOWSClearRequestObj(&ows_request);

  return result;

#else
  msSetError(MS_WMSERR, "WMS server support is not available.",
             "msMapLoadOWSParameters()");
  return MS_FAILURE;
#endif
}


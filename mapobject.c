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
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.15  2004/09/24 16:28:01  sean
 * mapscript can be again compiled without requiring WMS support, mapObj::loadOWSParameters will set an error in that case (bug 894).
 *
 * Revision 1.14  2004/09/10 13:47:46  sean
 * msMapSetSize returns value returned from msMapComputeGeotransform, just as msMapSetRotation
 *
 * Revision 1.13  2004/07/29 16:35:31  hobu
 * drop msRectIsValid and use MS_VALID_EXTENT
 *
 * Revision 1.12  2004/07/28 15:34:03  hobu
 * add a test after the input of an extent in msMapSetExtent to check for its validity.  Also convert tabs back to spaces.
 *
 * Revision 1.11  2004/07/28 14:24:03  hobu
 * Updated msMapSetExtent in mapobject.c to use msRectIsValid
 *
 * Revision 1.10  2004/07/27 19:03:37  sean
 * moved layer ordering functions from maputil.c to mapobject.c
 *
 * Revision 1.9  2004/07/23 14:12:07  frank
 * emit error if unimplemented georef<->pixel functions called
 *
 * Revision 1.8  2004/07/07 21:57:57  sean
 * Completed work on msInsertLayer and msRemoveLayer, added prototypes to map.h.  See bug 759.
 *
 * Revision 1.7  2004/07/07 04:34:40  sean
 * added msInsertLayer() and msRemoveLayer().  See mapserver bug 759.
 *
 * Revision 1.6  2004/06/24 17:30:11  frank
 * fixed typo in msTestConfigOption()
 *
 * Revision 1.5  2004/06/22 20:55:20  sean
 * Towards resolving issue 737 changed hashTableObj to a structure which contains a hashObj **items.  Changed all hash table access functions to operate on the target table by reference.  msFreeHashTable should not be used on the hashTableObj type members of mapserver structures, use msFreeHashItems instead.
 *
 * Revision 1.4  2004/06/14 17:34:22  frank
 * added msTestConfigOption
 *
 * Revision 1.3  2004/05/31 20:20:56  frank
 * ensure gdal.h is included for Config stuff
 *
 * Revision 1.2  2004/05/31 17:27:02  frank
 * Made msMapComputeGeotransform() return MS_SUCCESS or MS_FAILURE.
 *
 * Revision 1.1  2004/05/25 16:05:48  frank
 * New
 *
 */

#include "map.h"

#ifdef USE_GDAL
#  include "gdal.h"
#  include "cpl_conv.h"
#endif

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
    mapObj *map;

    /* create an empty map, no layers etc... */
    map = (mapObj *)calloc(sizeof(mapObj),1);

    if(!map)
    {
        msSetError(MS_MEMERR, NULL, "msCreateMap()");
        return NULL;
    }

    if( initMap( map ) == -1 )
        return NULL;

    if( msPostMapParseOutputFormatSetup( map ) == MS_FAILURE )
        return NULL;

    return map;
}

/************************************************************************/
/*                             msFreeMap()                              */
/************************************************************************/

void msFreeMap(mapObj *map) {
  int i;

  if(!map) return;
  msCloseConnections(map);

  msFree(map->name);
  msFree(map->shapepath);
  msFree(map->mappath);

  msFreeProjection(&(map->projection));
  msFreeProjection(&(map->latlon));

  msFreeLabelCache(&(map->labelcache));

  if( map->outputformat && --map->outputformat->refcount < 1 )
      msFreeOutputFormat( map->outputformat );

  for(i=0; i < map->numoutputformats; i++ ) {
      if( --map->outputformatlist[i]->refcount < 1 )
      msFreeOutputFormat( map->outputformatlist[i] );
  }
  if( map->outputformatlist != NULL )
      msFree( map->outputformatlist );

  msFree( map->imagetype );

  msFreeFontSet(&(map->fontset));

  msFreeSymbolSet(&map->symbolset); // free symbols
  msFree(map->symbolset.filename);

  freeWeb(&(map->web));

  freeScalebar(&(map->scalebar));
  freeReferenceMap(&(map->reference));
  freeLegend(&(map->legend));  

  for(i=0; i<map->numlayers; i++)
    freeLayer(&(map->layers[i]));
  msFree(map->layers);

  if (map->layerorder)
      free(map->layerorder);

  msFree(map->templatepattern);
  msFree(map->datapattern);
  msFreeHashItems(&(map->configoptions));
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

void msSetConfigOption( mapObj *map, const char *key, const char *value)

{
    // We have special "early" handling of this so that it will be
    // in effect when the projection blocks are parsed and pj_init is called.
    if( strcasecmp(key,"PROJ_LIB") == 0 )
        msSetPROJ_LIB( value );

    if( msLookupHashTable( &(map->configoptions), key ) != NULL )
        msRemoveHashTable( &(map->configoptions), key );
    msInsertHashTable( &(map->configoptions), key, value );
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
         key = msNextKeyFromHashTable( &(map->configoptions), key ) )
    {
        const char *value = msLookupHashTable( &(map->configoptions), key );
        if( strcasecmp(key,"PROJ_LIB") == 0 )
        {
            msSetPROJ_LIB( value );
        }
        else 
        {

#if defined(USE_GDAL) && GDAL_RELEASE_DATE > 20030601
            CPLSetConfigOption( key, value );
#endif
        }   
    }
}

/************************************************************************/
/*                            msMapSetExtent()                             */
/************************************************************************/

int msMapSetExtent( mapObj *map, 
                    double minx, double miny, double maxx, double maxy) 
{ 

    map->extent.minx = minx;
    map->extent.miny = miny;
    map->extent.maxx = maxx;
    map->extent.maxy = maxy;
    
    if (!MS_VALID_EXTENT(map->extent)) {
        msSetError(MS_MISCERR, "Given map extent is invalid.", "setExtent()"); 
        return MS_FAILURE;
    }
      
    map->cellsize = msAdjustExtent(&(map->extent), map->width, 
                                   map->height);
    msCalculateScale(map->extent, map->units, map->width, map->height, 
                     map->resolution, &(map->scale));

    return msMapComputeGeotransform( map );
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

    return msMapComputeGeotransform( map ); // like SetRotation -- sean
}

/************************************************************************/
/*                      msMapComputeGeotransform()                      */
/************************************************************************/

extern int InvGeoTransform( double *gt_in, double *gt_out );

int msMapComputeGeotransform( mapObj * map )

{
    double rot_angle;
    double geo_width, geo_height, center_x, center_y;

    map->saved_extent = map->extent;

    // Do we have all required parameters?
    if( map->extent.minx == map->extent.maxx 
        || map->width == 0 || map->height == 0 )
        return MS_FALSE;

    rot_angle = map->gt.rotation_angle * MS_PI / 180.0;

    geo_width = map->extent.maxx - map->extent.minx;
    geo_height = map->extent.maxy - map->extent.miny;

    center_x = map->extent.minx + geo_width*0.5;
    center_y = map->extent.miny + geo_height*0.5;

    map->gt.geotransform[1] = 
        cos(rot_angle) * geo_width / map->width;
    map->gt.geotransform[2] = 
        sin(rot_angle) * geo_height / map->height;
    map->gt.geotransform[0] = center_x 
        - (map->width * 0.5) * map->gt.geotransform[1]
        - (map->height * 0.5) * map->gt.geotransform[2];

    map->gt.geotransform[4] = 
        sin(rot_angle) * geo_width / map->width;
    map->gt.geotransform[5] = 
        - cos(rot_angle) * geo_height / map->height;
    map->gt.geotransform[3] = center_y 
        - (map->width * 0.5) * map->gt.geotransform[4]
        - (map->height * 0.5) * map->gt.geotransform[5];

    if( InvGeoTransform( map->gt.geotransform, 
                         map->gt.invgeotransform ) )
        return MS_SUCCESS;
    else
        return MS_FAILURE;
}

/************************************************************************/
/*                         msMapPixelToGeoref()                         */
/************************************************************************/

void msMapPixelToGeoref( mapObj *map, double *x, double *y )

{
    msSetError(MS_MISCERR, NULL, "msMapPixelToGeoref() not yet implemented");
}

/************************************************************************/
/*                         msMapGeorefToPixel()                         */
/************************************************************************/

void msMapGeorefToPixel( mapObj *map, double *x, double *y )

{
    msSetError(MS_MISCERR, NULL, "msMapGeorefToPixel() not yet implemented");
}

/************************************************************************/
/*                        msMapSetFakedExtent()                         */
/************************************************************************/

int msMapSetFakedExtent( mapObj *map )

{
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
    int i;

    // Possible to add another?
    if (map->numlayers == MS_MAXLAYERS) {
        msSetError(MS_CHILDERR, "Maximum number of Layer, %d, has been reached",
                   "msInsertLayer()", MS_MAXLAYERS);
        return -1;
    }
    // Catch attempt to insert past end of styles array
    else if (nIndex >= MS_MAXLAYERS) {
        msSetError(MS_CHILDERR, "Cannot insert Layer beyond index %d",
                   "msInsertLayer()", MS_MAXLAYERS-1);
        return -1;
    }
    else if (nIndex < 0) { // Insert at the end by default
        initLayer(&(map->layers[map->numlayers]), map);
        msCopyLayer(&(map->layers[map->numlayers]), layer);
        map->layerorder[map->numlayers] = map->numlayers;
        map->layers[map->numlayers].index = map->numlayers;
        map->numlayers++;
        return map->numlayers-1;
    }
    else if (nIndex >= 0 && nIndex < MS_MAXLAYERS) {
    
        // Move layers existing at the specified nIndex or greater
        // to a higher nIndex
        for (i=map->numlayers-1; i>=nIndex; i--) {
            map->layers[i+1] = map->layers[i];
            map->layers[i+1].index = i+1;
        }
        initLayer(&(map->layers[nIndex]), map);
        msCopyLayer(&(map->layers[nIndex]), layer);
        map->layers[map->numlayers].index = nIndex;

        /* adjust layers drawing order */
        
        // shift everything above this index one higher
        for (i=map->numlayers; i>nIndex; i--) {
            map->layerorder[i] = map->layerorder[i-1];
            if (map->layerorder[i] >= nIndex) map->layerorder[i]++;
        }
        for (i=0; i<nIndex; i++) {
            if (map->layerorder[i] >= nIndex) map->layerorder[i]++;
        }
        map->layerorder[nIndex] = nIndex;
        
        map->numlayers++;
        return nIndex;
    }
    else {
        msSetError(MS_CHILDERR, "Invalid index", "msInsertLayer()");
        return -1;
    }
}

layerObj *msRemoveLayer(mapObj *map, int nIndex) 
{
    int i;
    int order_index;
    layerObj *layer;
    
    if (nIndex < 0 || nIndex >= map->numlayers) {
        msSetError(MS_CHILDERR, "Cannot remove Layer, invalid index %d",
                   "msRemoveLayer()", nIndex);
        return NULL;
    }
    else {
        layer = (layerObj *) malloc(sizeof(layerObj));
        if (!layer) {
            msSetError(MS_MEMERR, 
                       "Failed to allocate layerObj to return as removed Layer",
                       "msRemoveLayer");
            return NULL;
        }
        initLayer(layer, NULL);
        //msInitProjection(&(layer->projection));
        msCopyLayer(layer, &(map->layers[nIndex]));
        for (i=nIndex; i<map->numlayers-1; i++) {
            map->layers[i] = map->layers[i+1];
            map->layers[i].index = i;
            //msCopyLayer(&map->layers[i], &map->layers[i+1]);
        }
        
        /* Adjust drawing order */
        order_index = 0;
        for (i=0; i<map->numlayers; i++) {
            if (map->layerorder[i] > nIndex) map->layerorder[i]--;
            if (map->layerorder[i] == nIndex) {
                order_index = i;
                break;
            }
        }
        for (i=order_index; i<map->numlayers; i++) {
            map->layerorder[i] = map->layerorder[i+1];
            if (map->layerorder[i] > nIndex) map->layerorder[i]--;
        }
        map->numlayers--;
            
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
    int i = 0;
    if (map && nLayerIndex < map->numlayers && nLayerIndex >=0)
    {
        for (i=0; i<map->numlayers; i++)
        {
            if ( map->layerorder[i] == nLayerIndex)
            {
                iCurrentIndex = i;
                break;
            }
        }
        if (iCurrentIndex >=0) 
        {
            // we do not need to promote if it is the first one.
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
    int i = 0;
    if (map && nLayerIndex < map->numlayers && nLayerIndex >=0)
    {
        for (i=0; i<map->numlayers; i++)
        {
            if ( map->layerorder[i] == nLayerIndex)
            {
                iCurrentIndex = i;
                break;
            }
        }
        if (iCurrentIndex >=0) 
        {
            // we do not need to demote if it is the last one.
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
    int nElements = 0;
    int i, j = 0;
    int bFound = 0;

    if (self && panIndexes)
    {
        nElements = self->numlayers;
        for (i=0; i<nElements; i++)
        {
            bFound = 0;
            for (j=0; j<nElements; j++)
            {
                if (panIndexes[j] == i)
                {
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
        for (i=0; i<nElements; i++)
        {
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
    int version;
#ifdef USE_WMS_SVR
    version = msOWSParseVersionString(wmtver);
    return msWMSLoadGetMapParams(map, version, request->ParamNames,
                                 request->ParamValues, request->NumParams);
#else
    msSetError(MS_WMSERR, "WMS server support is not available.",
               "msMapLoadOWSParameters()");
    return MS_FAILURE;
#endif
}


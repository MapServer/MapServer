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
 * Revision 1.1  2004/05/25 16:05:48  frank
 * New
 *
 */

#include "map.h"

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
  msFreeHashTable(map->configoptions);
  msFree(map);
}

/************************************************************************/
/*                         msGetConfigOption()                          */
/************************************************************************/

const char *msGetConfigOption( mapObj *map, const char *key)

{
    return msLookupHashTable( map->configoptions, key );
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

    if( msLookupHashTable( map->configoptions, key ) != NULL )
        msRemoveHashTable( map->configoptions, key );
    msInsertHashTable( map->configoptions, key, value );
}

/************************************************************************/
/*                      msApplyMapConfigOptions()                       */
/************************************************************************/

void msApplyMapConfigOptions( mapObj *map )

{
    const char *key;

    for( key = msFirstKeyFromHashTable( map->configoptions );
         key != NULL;
         key = msNextKeyFromHashTable( map->configoptions, key ) )
    {
        const char *value = msLookupHashTable( map->configoptions, key );
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
    // Check bounds
    if (minx > maxx || miny > maxy) {
        msSetError(MS_MISCERR, "Invalid bounds.", "setExtent()");
        return MS_FAILURE;
    }

    map->extent.minx = minx;
    map->extent.miny = miny;
    map->extent.maxx = maxx;
    map->extent.maxy = maxy;

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

    msMapComputeGeotransform( map );

    return MS_TRUE;
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

    return InvGeoTransform( map->gt.geotransform, 
                            map->gt.invgeotransform );
}

/************************************************************************/
/*                         msMapPixelToGeoref()                         */
/************************************************************************/

void msMapPixelToGeoref( mapObj *map, double *x, double *y )

{
}

/************************************************************************/
/*                         msMapGeorefToPixel()                         */
/************************************************************************/

void msMapGeorefToPixel( mapObj *map, double *x, double *y )

{
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

}


/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of query operations on rasters. 
 * Author:   Frank Warmerdam, warmerdam@pobox.com
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

#include <assert.h>
#include "mapserver.h"
#include "mapresample.h"
#include "mapthread.h"

MS_CVSID("$Id$")

int msRASTERLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record);
int msRASTERLayerGetItems(layerObj *layer);

#ifdef USE_GDAL

/* ==================================================================== */
/*      For now the rasterLayerInfo lives here since it is just used    */
/*      to hold information related to queries.                         */
/* ==================================================================== */
typedef struct {

    /* query cache results */
    int query_results;
    int query_alloc_max;
    int query_request_max;
    int query_result_hard_max;
    int raster_query_mode;
    int band_count;

    int refcount;

    rectObj which_rect;
    int     next_shape;

    double *qc_x;
    double *qc_y;
    float *qc_values;
    int    *qc_class;
    int    *qc_red;
    int    *qc_green;
    int    *qc_blue;
    int    *qc_count;
    int    *qc_tileindex;

    /* query bound in force */
    shapeObj *searchshape;

    /* Only nearest result to this point. */
    int      range_mode; /* MS_SINGLE, MS_MULTI or -1 (skip test) */
    double   range_dist;
    pointObj target_point;

    GDALColorTableH hCT;

    int      current_tile;

} rasterLayerInfo;

#define RQM_UNKNOWN               0
#define RQM_ENTRY_PER_PIXEL       1
#define RQM_HIST_ON_CLASS         2
#define RQM_HIST_ON_VALUE         3

extern int InvGeoTransform( double *gt_in, double *gt_out );
#define GEO_TRANS(tr,x,y)  ((tr)[0]+(tr)[1]*(x)+(tr)[2]*(y))

/************************************************************************/
/*                             addResult()                              */
/*                                                                      */
/*      this is a copy of the code in mapquery.c.  Should we prepare    */
/*      a small public API for managing results caches?                 */
/************************************************************************/

static int addResult(resultCacheObj *cache, int classindex, int shapeindex, int tileindex)
{
  int i;

  if(cache->numresults == cache->cachesize) { /* just add it to the end */
    if(cache->cachesize == 0)
      cache->results = (resultCacheMemberObj *) malloc(sizeof(resultCacheMemberObj)*MS_RESULTCACHEINCREMENT);
    else
      cache->results = (resultCacheMemberObj *) realloc(cache->results, sizeof(resultCacheMemberObj)*(cache->cachesize+MS_RESULTCACHEINCREMENT));
    if(!cache->results) {
      msSetError(MS_MEMERR, "Realloc() error.", "addResult()");
      return(MS_FAILURE);
    }   
    cache->cachesize += MS_RESULTCACHEINCREMENT;
  }

  i = cache->numresults;

  cache->results[i].classindex = classindex;
  cache->results[i].tileindex = tileindex;
  cache->results[i].shapeindex = shapeindex;
  cache->numresults++;

  return(MS_SUCCESS);
}

/************************************************************************/
/*                       msRasterLayerInfoFree()                        */
/************************************************************************/

static void msRasterLayerInfoFree( layerObj *layer )

{
    rasterLayerInfo *rlinfo = (rasterLayerInfo *) layer->layerinfo;

    if( rlinfo == NULL )
        return;

    if( rlinfo->qc_x != NULL )
    {
        free( rlinfo->qc_x );
        free( rlinfo->qc_y );
    }

    if( rlinfo->qc_values )
        free( rlinfo->qc_values );

    if( rlinfo->qc_class )
    {
        free( rlinfo->qc_class );
    }

    if( rlinfo->qc_red )
    {
        free( rlinfo->qc_red );
        free( rlinfo->qc_green );
        free( rlinfo->qc_blue );
    }
    
    if( rlinfo->qc_count != NULL )
        free( rlinfo->qc_count );

    if( rlinfo->qc_tileindex != NULL )
        free( rlinfo->qc_tileindex );

    free( rlinfo );

    layer->layerinfo = NULL;
}

/************************************************************************/
/*                    msRasterLayerInfoInitialize()                     */
/************************************************************************/

static void msRasterLayerInfoInitialize( layerObj *layer )

{
    rasterLayerInfo *rlinfo = (rasterLayerInfo *) layer->layerinfo;

    if( rlinfo != NULL )
        return;

    rlinfo = (rasterLayerInfo *) calloc(1,sizeof(rasterLayerInfo));
    layer->layerinfo = rlinfo;
    
    rlinfo->band_count = -1;
    rlinfo->raster_query_mode = RQM_ENTRY_PER_PIXEL;
    rlinfo->range_mode = -1; /* inactive */
    rlinfo->refcount = 0;
    
    /* We need to do this or the layer->layerinfo will be interpreted */
    /* as shapefile access info because the default connectiontype is */
    /* MS_SHAPEFILE. */
    if (layer->connectiontype != MS_WMS)
        layer->connectiontype = MS_RASTER;

    rlinfo->query_result_hard_max = 1000000;

    if( CSLFetchNameValue( layer->processing, "RASTER_QUERY_MAX_RESULT" ) 
        != NULL )
    {
        rlinfo->query_result_hard_max = 
            atoi(CSLFetchNameValue( layer->processing, "RASTER_QUERY_MAX_RESULT" ));
    }
}

/************************************************************************/
/*                       msRasterQueryAddPixel()                        */
/************************************************************************/

static void msRasterQueryAddPixel( layerObj *layer, pointObj *location, 
                                   float *values )

{
    rasterLayerInfo *rlinfo = (rasterLayerInfo *) layer->layerinfo;
    int red = 0, green = 0, blue = 0, nodata = FALSE;
    int p_class = -1;

    if( rlinfo->query_results == rlinfo->query_result_hard_max )
        return;

/* -------------------------------------------------------------------- */
/*      Is this our first time in?  If so, do an initial allocation     */
/*      for the data arrays suitable to our purposes.                   */
/* -------------------------------------------------------------------- */
    if( rlinfo->query_alloc_max == 0 )
    {
        rlinfo->query_alloc_max = 2;

        switch( rlinfo->raster_query_mode )
        {
          case RQM_ENTRY_PER_PIXEL:
            rlinfo->qc_x = (double *) 
                calloc(sizeof(double),rlinfo->query_alloc_max);
            rlinfo->qc_y = (double *) 
                calloc(sizeof(double),rlinfo->query_alloc_max);
            rlinfo->qc_values = (float *) 
                calloc(sizeof(float),
                       rlinfo->query_alloc_max*rlinfo->band_count);
            rlinfo->qc_red = (int *) 
                calloc(sizeof(int),rlinfo->query_alloc_max);
            rlinfo->qc_green = (int *) 
                calloc(sizeof(int),rlinfo->query_alloc_max);
            rlinfo->qc_blue = (int *) 
                calloc(sizeof(int),rlinfo->query_alloc_max);
            if( layer->numclasses > 0 )
                rlinfo->qc_class = (int *) 
                    calloc(sizeof(int),rlinfo->query_alloc_max);
            break;
           
          case RQM_HIST_ON_CLASS:
            break;
            
          case RQM_HIST_ON_VALUE:
            break;

          default:
            assert( FALSE );
        }
    }

/* -------------------------------------------------------------------- */
/*      Reallocate the data arrays larger if they are near the max      */
/*      now.                                                            */
/* -------------------------------------------------------------------- */
    if( rlinfo->query_results == rlinfo->query_alloc_max )
    {
        rlinfo->query_alloc_max = rlinfo->query_alloc_max * 2 + 100;

        if( rlinfo->qc_x != NULL )
            rlinfo->qc_x = realloc(rlinfo->qc_x,
                                   sizeof(double) * rlinfo->query_alloc_max);
        if( rlinfo->qc_y != NULL )
            rlinfo->qc_y = realloc(rlinfo->qc_y,
                                   sizeof(double) * rlinfo->query_alloc_max);
        if( rlinfo->qc_values != NULL )
            rlinfo->qc_values = 
                realloc(rlinfo->qc_values,
                        sizeof(float) * rlinfo->query_alloc_max 
                        * rlinfo->band_count );
        if( rlinfo->qc_class != NULL )
            rlinfo->qc_class = realloc(rlinfo->qc_class,
                                       sizeof(int) * rlinfo->query_alloc_max);
        if( rlinfo->qc_red != NULL )
            rlinfo->qc_red = realloc(rlinfo->qc_red,
                                       sizeof(int) * rlinfo->query_alloc_max);
        if( rlinfo->qc_green != NULL )
            rlinfo->qc_green = realloc(rlinfo->qc_green,
                                       sizeof(int) * rlinfo->query_alloc_max);
        if( rlinfo->qc_blue != NULL )
            rlinfo->qc_blue = realloc(rlinfo->qc_blue,
                                       sizeof(int) * rlinfo->query_alloc_max);
        if( rlinfo->qc_count != NULL )
            rlinfo->qc_count = realloc(rlinfo->qc_count,
                                       sizeof(int) * rlinfo->query_alloc_max);
        if( rlinfo->qc_count != NULL )
            rlinfo->qc_tileindex = realloc(rlinfo->qc_tileindex,
                                       sizeof(int) * rlinfo->query_alloc_max);
    }

/* -------------------------------------------------------------------- */
/*      Handle classification.                                          */
/*                                                                      */
/*      NOTE: The following is really quite inadequate to deal with     */
/*      classifications based on [red], [green] and [blue] as           */
/*      described in:                                                   */
/*       http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=1021         */
/* -------------------------------------------------------------------- */
    if( rlinfo->qc_class != NULL )
    {
        p_class = msGetClass_Float(layer, values[0] );

        if( p_class == -1 )
            nodata = TRUE;
        else
        {
            rlinfo->qc_class[rlinfo->query_results] = p_class;
            if( layer->class[p_class]->numstyles > 0 )
            {
                red   = layer->class[p_class]->styles[0]->color.red;
                green = layer->class[p_class]->styles[0]->color.green;
                blue  = layer->class[p_class]->styles[0]->color.blue;
            }
            else
            {
                red = green = blue = 0;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Handle colormap                                                 */
/* -------------------------------------------------------------------- */
    else if( rlinfo->hCT != NULL )
    {
        int pct_index = (int) floor(values[0]);
        GDALColorEntry sEntry;

        if( GDALGetColorEntryAsRGB( rlinfo->hCT, pct_index, &sEntry ) )
        {
            red = sEntry.c1;
            green = sEntry.c2;
            blue = sEntry.c3;

            if( sEntry.c4 == 0 )
                nodata = TRUE;
        }
        else
            nodata = TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Color derived from greyscale value.                             */
/* -------------------------------------------------------------------- */
    else
    {
        if( rlinfo->band_count >= 3 )
        {
            red = (int) MAX(0,MIN(255,values[0]));
            green = (int) MAX(0,MIN(255,values[1]));
            blue = (int) MAX(0,MIN(255,values[2]));
        }
        else
        {
            red = green = blue = (int) MAX(0,MIN(255,values[0]));
        }
    }

/* -------------------------------------------------------------------- */
/*      Record the color.                                               */
/* -------------------------------------------------------------------- */
    rlinfo->qc_red[rlinfo->query_results] = red;
    rlinfo->qc_green[rlinfo->query_results] = green;
    rlinfo->qc_blue[rlinfo->query_results] = blue;

/* -------------------------------------------------------------------- */
/*      Record spatial location.                                        */
/* -------------------------------------------------------------------- */
    if( rlinfo->qc_x != NULL )
    {
        rlinfo->qc_x[rlinfo->query_results] = location->x;
        rlinfo->qc_y[rlinfo->query_results] = location->y;
    }

/* -------------------------------------------------------------------- */
/*      Record actual pixel value(s).                                   */
/* -------------------------------------------------------------------- */
    if( rlinfo->qc_values != NULL )
        memcpy( rlinfo->qc_values + rlinfo->query_results * rlinfo->band_count,
                values, sizeof(float) * rlinfo->band_count );

/* -------------------------------------------------------------------- */
/*      Add to the results cache.                                       */
/* -------------------------------------------------------------------- */
    if( ! nodata )
    {
        addResult( layer->resultcache, p_class, rlinfo->query_results, 0 );
        rlinfo->query_results++;
    }
}

/************************************************************************/
/*                       msRasterQueryByRectLow()                       */
/************************************************************************/

static int 
msRasterQueryByRectLow(mapObj *map, layerObj *layer, GDALDatasetH hDS,
                       rectObj queryRect) 

{
    double	adfGeoTransform[6], adfInvGeoTransform[6];
    double      dfXMin, dfYMin, dfXMax, dfYMax, dfX, dfY, dfAdjustedRange;
    int         nWinXOff, nWinYOff, nWinXSize, nWinYSize;
    int         nRXSize, nRYSize;
    float       *pafRaster;
    int         nBandCount, *panBandMap, iPixel, iLine;
    CPLErr      eErr;
    rasterLayerInfo *rlinfo;
    rectObj     searchrect;
    int         needReproject = MS_FALSE;

    rlinfo = (rasterLayerInfo *) layer->layerinfo;

/* -------------------------------------------------------------------- */
/*      Reproject the search rect into the projection of this           */
/*      layer/file if needed.                                           */
/* -------------------------------------------------------------------- */
    searchrect = queryRect;
#ifdef USE_PROJ
    if(layer->project 
       && msProjectionsDiffer(&(layer->projection), &(map->projection)))
    {
        msProjectRect(&(map->projection), &(layer->projection), &searchrect);
        needReproject = MS_TRUE;
    }
    else
        layer->project = MS_FALSE;
#endif

/* -------------------------------------------------------------------- */
/*      Transform the rectangle in target ground coordinates to         */
/*      pixel/line extents on the file.  Process all 4 corners, to      */
/*      build extents.                                                  */
/* -------------------------------------------------------------------- */
    nRXSize = GDALGetRasterXSize( hDS );
    nRYSize = GDALGetRasterYSize( hDS );

    msGetGDALGeoTransform( hDS, map, layer, adfGeoTransform );
    InvGeoTransform( adfGeoTransform, adfInvGeoTransform );

    /* top left */
    dfXMin = dfXMax = GEO_TRANS(adfInvGeoTransform,
                                searchrect.minx, searchrect.maxy);
    dfYMin = dfYMax = GEO_TRANS(adfInvGeoTransform+3,
                                searchrect.minx, searchrect.maxy);
                                
    /* top right */
    dfX = GEO_TRANS(adfInvGeoTransform  , searchrect.maxx, searchrect.maxy);
    dfY = GEO_TRANS(adfInvGeoTransform+3, searchrect.maxx, searchrect.maxy);
    dfXMin = MIN(dfXMin,dfX);
    dfXMax = MAX(dfXMax,dfX);
    dfYMin = MIN(dfYMin,dfY);
    dfYMax = MAX(dfYMax,dfY);
    
    /* bottom left */
    dfX = GEO_TRANS(adfInvGeoTransform  , searchrect.minx, searchrect.miny);
    dfY = GEO_TRANS(adfInvGeoTransform+3, searchrect.minx, searchrect.miny);
    dfXMin = MIN(dfXMin,dfX);
    dfXMax = MAX(dfXMax,dfX);
    dfYMin = MIN(dfYMin,dfY);
    dfYMax = MAX(dfYMax,dfY);
    
    /* bottom right */
    dfX = GEO_TRANS(adfInvGeoTransform  , searchrect.maxx, searchrect.miny);
    dfY = GEO_TRANS(adfInvGeoTransform+3, searchrect.maxx, searchrect.miny);
    dfXMin = MIN(dfXMin,dfX);
    dfXMax = MAX(dfXMax,dfX);
    dfYMin = MIN(dfYMin,dfY);
    dfYMax = MAX(dfYMax,dfY);

/* -------------------------------------------------------------------- */
/*      Trim the rectangle to the area of the file itself, but out      */
/*      to the edges of the touched edge pixels.                        */
/* -------------------------------------------------------------------- */
    dfXMin = MAX(0.0,MIN(nRXSize,floor(dfXMin)));
    dfYMin = MAX(0.0,MIN(nRYSize,floor(dfYMin)));
    dfXMax = MAX(0.0,MIN(nRXSize,ceil(dfXMax)));
    dfYMax = MAX(0.0,MIN(nRYSize,ceil(dfYMax)));

/* -------------------------------------------------------------------- */
/*      Convert to integer offset/size values.                          */
/* -------------------------------------------------------------------- */
    nWinXOff = (int) dfXMin;
    nWinYOff = (int) dfYMin;
    nWinXSize = (int) (dfXMax - dfXMin);
    nWinYSize = (int) (dfYMax - dfYMin);

/* -------------------------------------------------------------------- */
/*      What bands are we operating on?                                 */
/* -------------------------------------------------------------------- */
    panBandMap = msGetGDALBandList( layer, hDS, 0, &nBandCount );
    
    if( rlinfo->band_count == -1 )
        rlinfo->band_count = nBandCount;

    if( nBandCount != rlinfo->band_count )
    {
        msSetError( MS_IMGERR, 
                    "Got %d bands, but expected %d bands.", 
                    "msRasterQueryByRectLow()", 
                    nBandCount, rlinfo->band_count );

        return -1;
    }

/* -------------------------------------------------------------------- */
/*      Try to load the raster data.  For now we just load the first    */
/*      band in the file.  Later we will deal with the various band     */
/*      selection criteria.                                             */
/* -------------------------------------------------------------------- */
    pafRaster = (float *) 
        calloc(sizeof(float),nWinXSize*nWinYSize*nBandCount);

#if defined(GDAL_VERSION_NUM) && GDAL_VERSION_NUM >= 1199
    eErr = GDALDatasetRasterIO( hDS, GF_Read, 
                                nWinXOff, nWinYOff, nWinXSize, nWinYSize,
                                pafRaster, nWinXSize, nWinYSize, GDT_Float32,
                                nBandCount, panBandMap, 
                                4 * nBandCount, 
                                4 * nBandCount * nWinXSize, 
                                4 );
#else
    /*
     * The above could actually be implemented for pre-1.2.0 GDALs
     * reading band by band, but it would be hard to do and test and would
     * be very rarely useful so we skip it.
     */
    msSetError(MS_IMGERR, 
               "Raster query support requires GDAL 1.2.0 or newer.", 
               "msRasterQueryByRectLow()" );
    free( pafRaster );
    return MS_FAILURE;
#endif

    
    if( eErr != CE_None )
    {
        msSetError( MS_IOERR, "GDALDatasetRasterIO() failed: %s", 
                    CPLGetLastErrorMsg(), 
                    "msRasterQueryByRectLow()" );

        free( pafRaster );
        return -1;
    }

/* -------------------------------------------------------------------- */
/*      Fetch color table for intepreting colors if needed.             */
/* -------------------------------------------------------------------- */
    rlinfo->hCT = GDALGetRasterColorTable( 
        GDALGetRasterBand( hDS, panBandMap[0] ) );

    free( panBandMap );

/* -------------------------------------------------------------------- */
/*      When computing whether pixels are within range we do it         */
/*      based on the center of the pixel to the target point but        */
/*      really it ought to be the nearest point on the pixel.  It       */
/*      would be too much trouble to do this rigerously, so we just     */
/*      add a fudge factor so that a range of zero will find the        */
/*      pixel the target falls in at least.                             */
/* -------------------------------------------------------------------- */
    dfAdjustedRange = 
        sqrt(adfGeoTransform[1] * adfGeoTransform[1]
             + adfGeoTransform[2] * adfGeoTransform[2]
             + adfGeoTransform[4] * adfGeoTransform[4]
             + adfGeoTransform[5] * adfGeoTransform[5]) * 0.5 * 1.41421356237
        + sqrt( rlinfo->range_dist );
    dfAdjustedRange = dfAdjustedRange * dfAdjustedRange;

/* -------------------------------------------------------------------- */
/*      Loop over all pixels determining which are "in".                */
/* -------------------------------------------------------------------- */
    for( iLine = 0; iLine < nWinYSize; iLine++ )
    {
        for( iPixel = 0; iPixel < nWinXSize; iPixel++ )
        {
            pointObj  sPixelLocation;

            if( rlinfo->query_results == rlinfo->query_result_hard_max )
                break;

            /* transform pixel/line to georeferenced */
            sPixelLocation.x = 
                GEO_TRANS(adfGeoTransform, 
                          iPixel + nWinXOff + 0.5, iLine + nWinYOff + 0.5 );
            sPixelLocation.y = 
                GEO_TRANS(adfGeoTransform+3, 
                          iPixel + nWinXOff + 0.5, iLine + nWinYOff + 0.5 );

            /* If projections differ, convert this back into the map  */
            /* projection for distance testing, and comprison to the  */
            /* search shape.  */
            if( needReproject )
                msProjectPoint( &(layer->projection), &(map->projection), 
                                &sPixelLocation );

            if( rlinfo->searchshape != NULL
                && msIntersectPointPolygon( &sPixelLocation, 
                                            rlinfo->searchshape ) == MS_FALSE )
                continue;

            if( rlinfo->range_mode >= 0 )
            {
                double dist;

                dist = (rlinfo->target_point.x - sPixelLocation.x) 
                    * (rlinfo->target_point.x - sPixelLocation.x) 
                    + (rlinfo->target_point.y - sPixelLocation.y)
                    * (rlinfo->target_point.y - sPixelLocation.y);

                if( dist >= dfAdjustedRange )
                    continue;

                /* If we can only have one feature, trim range and clear */
                /* previous result.  */
                if( rlinfo->range_mode == MS_SINGLE )
                {
                    rlinfo->range_dist = dist;
                    rlinfo->query_results = 0;
                }
            }

            msRasterQueryAddPixel( layer, &sPixelLocation, 
                                   pafRaster 
                                   + (iLine*nWinXSize + iPixel) * nBandCount );
        }
    }

/* -------------------------------------------------------------------- */
/*      Cleanup.                                                        */
/* -------------------------------------------------------------------- */
    free( pafRaster );

    return MS_SUCCESS;
}
#endif /* def USE_GDAL */

/************************************************************************/
/*                        msRasterQueryByRect()                         */
/************************************************************************/

int msRasterQueryByRect(mapObj *map, layerObj *layer, rectObj queryRect) 

{
#ifndef USE_GDAL
    msSetError( MS_IMGERR, 
                "Rasters queries only supported with GDAL support enabled.",
                "msRasterQueryByRect()" );
    return MS_FAILURE;
#else
    int status = MS_SUCCESS;
    char *filename=NULL;

    int t;
    int tileitemindex=-1;
    shapefileObj tilefile;
    char tilename[MS_PATH_LENGTH];
    int numtiles=1; /* always at least one tile */
    char szPath[MS_MAXPATHLEN];
    rectObj searchrect;
    rasterLayerInfo *rlinfo = NULL;
    char tiAbsFilePath[MS_MAXPATHLEN];
    char *tiAbsDirPath = NULL;

/* -------------------------------------------------------------------- */
/*      Get the layer info.                                             */
/* -------------------------------------------------------------------- */
    msRasterLayerInfoInitialize( layer );
    rlinfo = (rasterLayerInfo *) layer->layerinfo;

/* -------------------------------------------------------------------- */
/*      Clear old results cache.                                        */
/* -------------------------------------------------------------------- */
    if(layer->resultcache) {
      if(layer->resultcache->results) free(layer->resultcache->results);
      free(layer->resultcache);
      layer->resultcache = NULL;
    }

/* -------------------------------------------------------------------- */
/*      Initialize the results cache.                                   */
/* -------------------------------------------------------------------- */
    layer->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj)); 
    layer->resultcache->results = NULL;
    layer->resultcache->numresults = layer->resultcache->cachesize = 0;
    layer->resultcache->bounds.minx =
        layer->resultcache->bounds.miny =
        layer->resultcache->bounds.maxx =
        layer->resultcache->bounds.maxy = -1;

/* -------------------------------------------------------------------- */
/*      Check if we should really be acting on this layer and           */
/*      provide debug info in various cases.                            */
/* -------------------------------------------------------------------- */
    if(layer->debug > 0 || map->debug > 1)
        msDebug( "msRasterQueryByRect(%s): entering.\n", layer->name );

    if(!layer->data && !layer->tileindex) {
        if(layer->debug > 0 || map->debug > 0 )
            msDebug( "msRasterQueryByRect(%s): layer data and tileindex NULL ... doing nothing.", layer->name );
        return MS_SUCCESS;
    }

    if((layer->status != MS_ON) && layer->status != MS_DEFAULT ) {
        if(layer->debug > 0 )
            msDebug( "msRasterQueryByRect(%s): not status ON or DEFAULT, doing nothing.", layer->name );
        return MS_SUCCESS;
    }

/* -------------------------------------------------------------------- */
/*      Open tile index if we have one.                                 */
/* -------------------------------------------------------------------- */
    if(layer->tileindex) { /* we have in index file */
        if(msShapefileOpen(&tilefile, "rb", 
                         msBuildPath3(szPath, map->mappath, map->shapepath, 
                                      layer->tileindex), MS_TRUE) == -1) 
            if(msShapefileOpen(&tilefile, "rb", msBuildPath(szPath, map->mappath, layer->tileindex), MS_TRUE) == -1) 
                return(MS_FAILURE);    

        tileitemindex = msDBFGetItemIndex(tilefile.hDBF, layer->tileitem);
        if(tileitemindex == -1) 
            return(MS_FAILURE);

        searchrect = queryRect;

#ifdef USE_PROJ
        if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
            msProjectRect(&map->projection, &layer->projection, &searchrect); /* project the searchrect to source coords */
#endif
        status = msShapefileWhichShapes(&tilefile, searchrect, layer->debug);
        if(status != MS_SUCCESS) 
            numtiles = 0; /* could be MS_DONE or MS_FAILURE */
        else
            numtiles = tilefile.numshapes;
    }

/* -------------------------------------------------------------------- */
/*      Iterate over all tiles (just one in untiled case).              */
/* -------------------------------------------------------------------- */
    for(t=0; t<numtiles && status == MS_SUCCESS; t++) { 

        GDALDatasetH  hDS;

        rlinfo->current_tile = t;

/* -------------------------------------------------------------------- */
/*      Get filename.                                                   */
/* -------------------------------------------------------------------- */
        if(layer->tileindex) {
            if(!msGetBit(tilefile.status,t)) continue; /* on to next tile */
            if(layer->data == NULL) /* assume whole filename is in attribute field */
                filename = (char*)msDBFReadStringAttribute(tilefile.hDBF, t, tileitemindex);
            else {  
                sprintf(tilename,"%s/%s", msDBFReadStringAttribute(tilefile.hDBF, t, tileitemindex) , layer->data);
                filename = tilename;
            }
        } else {
            filename = layer->data;
        }

        if(strlen(filename) == 0) continue;

/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
        msGDALInitialize();

        /*
        ** If using a tileindex then build the path relative to that file if SHAPEPATH is not set.
        */
        if(layer->tileindex && !map->shapepath) { 
            msBuildPath(tiAbsFilePath, map->mappath, layer->tileindex); /* absolute path to tileindex file */
            tiAbsDirPath = msGetPath(tiAbsFilePath); /* tileindex file's directory */
            msBuildPath(szPath, tiAbsDirPath, filename); 
            free(tiAbsDirPath);
        } else {
            msTryBuildPath3(szPath, map->mappath, map->shapepath, filename);
        }

        msAcquireLock( TLOCK_GDAL );
        hDS = GDALOpen(szPath, GA_ReadOnly );
        
        if( hDS == NULL )
        {
            int ignore_missing = msMapIgnoreMissingData( map );
            msReleaseLock( TLOCK_GDAL );

            if ( ignore_missing == MS_MISSING_DATA_FAIL ) {
              if( layer->debug || map->debug )
                msSetError( MS_IMGERR, 
                            "Unable to open file %s for layer %s ... fatal error.\n%s", 
                            szPath, layer->name, CPLGetLastErrorMsg(),
                            "msRasterQueryByRect()" );
              return(MS_FAILURE);
            }
            if( ignore_missing == MS_MISSING_DATA_LOG ) {
              if( layer->debug || map->debug )
                msDebug( "Unable to open file %s for layer %s ... ignoring this missing data.\n%s", 
                         filename, layer->name, CPLGetLastErrorMsg() );
            }
            continue;
        }

/* -------------------------------------------------------------------- */
/*      Update projectionObj if AUTO.                                   */
/* -------------------------------------------------------------------- */
        if (layer->projection.numargs > 0 && 
            EQUAL(layer->projection.args[0], "auto"))
        {
            const char *pszWKT;

            pszWKT = GDALGetProjectionRef( hDS );

            if( pszWKT != NULL && strlen(pszWKT) > 0 )
            {
                if( msOGCWKT2ProjectionObj(pszWKT, &(layer->projection),
                                           layer->debug ) != MS_SUCCESS )
                {
                    char	szLongMsg[MESSAGELENGTH*2];
                    errorObj *ms_error = msGetErrorObj();

                    sprintf( szLongMsg, 
                             "%s\n"
                             "PROJECTION AUTO cannot be used for this "
                             "GDAL raster (`%s').",
                             ms_error->message, filename);
                    szLongMsg[MESSAGELENGTH-1] = '\0';

                    msSetError(MS_OGRERR, "%s","msDrawRasterLayer()",
                               szLongMsg);

                    msReleaseLock( TLOCK_GDAL );
                    return(MS_FAILURE);
                }
            }
        }

/* -------------------------------------------------------------------- */
/*      Perform actual query on this file.                              */
/* -------------------------------------------------------------------- */
        if( status == MS_SUCCESS )
            status = msRasterQueryByRectLow( map, layer, hDS, queryRect );

        GDALClose( hDS );
        msReleaseLock( TLOCK_GDAL );

    } /* next tile */

    if(layer->tileindex) /* tiling clean-up */
        msShapefileClose(&tilefile);    

/* -------------------------------------------------------------------- */
/*      On failure, or empty result set, cleanup the rlinfo since we    */
/*      likely won't ever have it accessed or cleaned up later.         */
/* -------------------------------------------------------------------- */
    if( status == MS_FAILURE || rlinfo->query_results == 0 )
        msRasterLayerInfoFree( layer );

    /* populate the items/numitems layer-level values */
    if( layer->layerinfo != NULL )
        msRASTERLayerGetItems(layer);

    return status;
#endif /* def USE_GDAL */
}

/************************************************************************/
/*                        msRasterQueryByShape()                        */
/************************************************************************/

int msRasterQueryByShape(mapObj *map, layerObj *layer, shapeObj *selectshape)

{
#ifndef USE_GDAL
    msSetError( MS_IMGERR, 
                "Rasters queries only supported with GDAL support enabled.",
                "msRasterQueryByRect()" );
    return MS_FAILURE;
#else
    rasterLayerInfo *rlinfo = NULL;
    int status;

    msRasterLayerInfoInitialize( layer );

    rlinfo = (rasterLayerInfo *) layer->layerinfo;
    rlinfo->searchshape = selectshape;

    msComputeBounds( selectshape );
    status = msRasterQueryByRect( map, layer, selectshape->bounds );

    rlinfo = (rasterLayerInfo *) layer->layerinfo;
    if( rlinfo )
        rlinfo->searchshape = NULL;

    return status;
#endif /* USE_GDAL  */
}

/************************************************************************/
/*                        msRasterQueryByPoint()                        */
/************************************************************************/

int msRasterQueryByPoint(mapObj *map, layerObj *layer, int mode, pointObj p, 
                         double buffer, int maxresults)
{
#ifndef USE_GDAL
    msSetError( MS_IMGERR, 
                "Rasters queries only supported with GDAL support enabled.",
                "msRasterQueryByRect()" );
    return MS_FAILURE;
#else
    int result;
    int previous_maxresults;
    double layer_tolerance;
    rectObj bufferRect;
    rasterLayerInfo *rlinfo = NULL;

    msRasterLayerInfoInitialize( layer );

    rlinfo = (rasterLayerInfo *) layer->layerinfo;

/* -------------------------------------------------------------------- */
/*      If the buffer is not set, then use layer tolerances             */
/*      instead.   The "buffer" distince is now in georeferenced        */
/*      units.  Note that tolerances in pixels are basically map        */
/*      display pixels, not underlying raster pixels.  It isn't         */
/*      clear that there is any way of requesting a buffer size in      */
/*      underlying pixels.                                              */
/* -------------------------------------------------------------------- */
    if(buffer <= 0) { /* use layer tolerance */

        /* Get the layer tolerance
           default is 3 for point and line layers, 0 for others */
        if(layer->tolerance == -1)
            if(layer->status == MS_LAYER_POINT || 
               layer->status == MS_LAYER_LINE)
                layer_tolerance = 3;
            else
                layer_tolerance = 0;
        else
            layer_tolerance = layer->tolerance;

        if(layer->toleranceunits == MS_PIXELS)
            buffer = layer_tolerance 
                * msAdjustExtent(&(map->extent), map->width, map->height);
        else
            buffer = layer_tolerance 
                * (msInchesPerUnit(layer->toleranceunits,0)
                   / msInchesPerUnit(map->units,0));
    }

/* -------------------------------------------------------------------- */
/*      Setup target point information, at this point they are in       */
/*      map coordinates.                                                */
/* -------------------------------------------------------------------- */
    rlinfo->range_dist = buffer * buffer;
    rlinfo->target_point = p;

/* -------------------------------------------------------------------- */
/*      if we are in the MS_SINGLE mode, first try a query with zero    */
/*      tolerance.  If this gets a raster pixel then we can be          */
/*      reasonably assured that it is the closest to the query          */
/*      point.  This will potentially be must more efficient than       */
/*      processing all pixels within the tolerance.                     */
/* -------------------------------------------------------------------- */
    if( mode == MS_SINGLE )
    {
        rectObj pointRect;

        pointRect.minx = p.x;
        pointRect.maxx = p.x;
        pointRect.miny = p.y;
        pointRect.maxy = p.y;

        rlinfo->range_mode = MS_SINGLE;
        result = msRasterQueryByRect( map, layer, pointRect );
        if( rlinfo->query_results > 0 )
            return result;
    }

/* -------------------------------------------------------------------- */
/*      Setup a rectangle that is everything within the designated      */
/*      range and do a search against that.                             */
/* -------------------------------------------------------------------- */
    bufferRect.minx = p.x - buffer;
    bufferRect.maxx = p.x + buffer;
    bufferRect.miny = p.y - buffer;
    bufferRect.maxy = p.y + buffer;

    rlinfo->range_mode = mode;

    if( maxresults != 0 )
    {
        previous_maxresults = rlinfo->query_result_hard_max;
        rlinfo->query_result_hard_max = maxresults;
    }

    result = msRasterQueryByRect( map, layer, bufferRect );

    if( rlinfo != NULL && maxresults != 0 )
        rlinfo->query_result_hard_max = previous_maxresults;

    return result;
#endif /* USE_GDAL  */
}

/************************************************************************/
/* ==================================================================== */
/*                          RASTER Query Layer                          */
/*                                                                      */
/*      The results of a raster query are treated as a set of shapes    */
/*      that can be accessed using the normal layer semantics.          */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                         msRASTERLayerOpen()                          */
/************************************************************************/

int msRASTERLayerOpen(layerObj *layer) 
{
#ifndef USE_GDAL
    msSetError( MS_IMGERR, 
                "Rasters queries only supported with GDAL support enabled.",
                "msRasterQueryByRect()" );
    return MS_FAILURE;
#else
    rasterLayerInfo *rlinfo;

    /* If we don't have info, initialize an empty one now */
    if( layer->layerinfo == NULL )
        msRasterLayerInfoInitialize( layer );
        
    rlinfo = (rasterLayerInfo *) layer->layerinfo;

    rlinfo->refcount = rlinfo->refcount + 1;

    return MS_SUCCESS;
#endif /* def USE_GDAL */
}

/************************************************************************/
/*                         msRASTERIsLayerOpen()                        */
/************************************************************************/

int msRASTERLayerIsOpen(layerObj *layer) 
{
#ifndef USE_GDAL
    msSetError( MS_IMGERR, 
                "Rasters queries only supported with GDAL support enabled.",
                "msRasterLayerIsOpen()" );
    return MS_FALSE;
#else
    if (layer->layerinfo)
        return MS_TRUE;
    return MS_FALSE;
#endif
}


/************************************************************************/
/*                     msRASTERLayerFreeItemInfo()                      */
/************************************************************************/
void msRASTERLayerFreeItemInfo(layerObj *layer) 
	{}

/************************************************************************/
/*                     msRASTERLayerInitItemInfo()                      */
/*                                                                      */
/*      Perhaps we should be validating the requested items here?       */
/************************************************************************/

int msRASTERLayerInitItemInfo(layerObj *layer) 
	{ return MS_SUCCESS; }

/************************************************************************/
/*                      msRASTERLayerWhichShapes()                      */
/************************************************************************/
int msRASTERLayerWhichShapes(layerObj *layer, rectObj rect) 

{
#ifndef USE_GDAL
    return MS_FAILURE;
#else
    rasterLayerInfo *rlinfo = (rasterLayerInfo *) layer->layerinfo;

    rlinfo->which_rect = rect;
    rlinfo->next_shape = 0;

    return MS_SUCCESS;
#endif /* def USE_GDAL */
}

/************************************************************************/
/*                         msRASTERLayerClose()                         */
/************************************************************************/

int msRASTERLayerClose(layerObj *layer) 
{
#ifndef USE_GDAL
    return MS_FAILURE;
#else
    rasterLayerInfo *rlinfo = (rasterLayerInfo *) layer->layerinfo;

    if( rlinfo != NULL )
    {
        rlinfo->refcount--;

        if( rlinfo->refcount < 0 )
            msRasterLayerInfoFree( layer );
    }
    return MS_SUCCESS;
#endif /* def USE_GDAL */
}

/************************************************************************/
/*                       msRASTERLayerNextShape()                       */
/************************************************************************/

int msRASTERLayerNextShape(layerObj *layer, shapeObj *shape)
{
#ifndef USE_GDAL
    return MS_FAILURE;
#else
    rasterLayerInfo *rlinfo = (rasterLayerInfo *) layer->layerinfo;

    if( rlinfo->next_shape < 0 
        || rlinfo->next_shape >= rlinfo->query_results )
    {
        msFreeShape(shape);
        shape->type = MS_SHAPE_NULL;
        return MS_DONE;
    }
    else 
        return msRASTERLayerGetShape( layer, shape, 0, rlinfo->next_shape++ );
#endif /* def USE_GDAL */
}

/************************************************************************/
/*                       msRASTERLayerGetShape()                        */
/************************************************************************/

int msRASTERLayerGetShape(layerObj *layer, shapeObj *shape, int tile, 
                          long record)

{
#ifndef USE_GDAL
    return MS_FAILURE;
#else
    rasterLayerInfo *rlinfo = (rasterLayerInfo *) layer->layerinfo;
    int i;

    msFreeShape(shape);
    shape->type = MS_SHAPE_NULL;

/* -------------------------------------------------------------------- */
/*      Validate requested record id.                                   */
/* -------------------------------------------------------------------- */
    if( record < 0 || record >= rlinfo->query_results )
    {
        msSetError(MS_MISCERR, 
                   "Out of range shape index requested.  Requested %d\n"
                   "but only %d shapes available.",
                   "msRASTERLayerGetShape()",
                   record, rlinfo->query_results );
        return MS_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Apply the geometry.                                             */
/* -------------------------------------------------------------------- */
    if( rlinfo->qc_x != NULL )
    {
	lineObj	line;
        pointObj point;

        shape->type = MS_SHAPE_POINT;
        
        line.numpoints = 1;
        line.point = &point;
        
        point.x = rlinfo->qc_x[record];
        point.y = rlinfo->qc_y[record];
#ifdef USE_POINT_Z_M
        point.m = 0.0;
#endif

        msAddLine( shape, &line );
        msComputeBounds( shape );
    }

/* -------------------------------------------------------------------- */
/*      Apply the requested items.                                      */
/* -------------------------------------------------------------------- */
    if( layer->numitems > 0 )
    {
        shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
        shape->numvalues = layer->numitems;
        
        for( i = 0; i < layer->numitems; i++ )
        {
            char szWork[1000];

            szWork[0] = '\0';
            if( EQUAL(layer->items[i],"x") && rlinfo->qc_x )
                sprintf( szWork, "%.8g", rlinfo->qc_x[record] );
            else if( EQUAL(layer->items[i],"y") && rlinfo->qc_y )
                sprintf( szWork, "%.8g", rlinfo->qc_y[record] );

            else if( EQUAL(layer->items[i],"value_list") && rlinfo->qc_values )
            {
                int iValue;

                for( iValue = 0; iValue < rlinfo->band_count; iValue++ )
                {
                    if( iValue != 0 )
                        strcat( szWork, "," );

                    sprintf( szWork+strlen(szWork), "%.8g", 
                             rlinfo->qc_values[record * rlinfo->band_count
                                               + iValue] );
                }
            }
            else if( EQUALN(layer->items[i],"value_",6) && rlinfo->qc_values )
            {
                int iValue = atoi(layer->items[i]+6);
                sprintf( szWork, "%.8g", 
                         rlinfo->qc_values[record*rlinfo->band_count+iValue] );
            }
            else if( EQUAL(layer->items[i],"class") && rlinfo->qc_class ) 
            {
                int p_class = rlinfo->qc_class[record];
                if( layer->class[p_class]->name != NULL )
                    sprintf( szWork, "%.999s", layer->class[p_class]->name );
                else
                    sprintf( szWork, "%d", p_class );
            }
            else if( EQUAL(layer->items[i],"red") && rlinfo->qc_red )
                sprintf( szWork, "%d", rlinfo->qc_red[record] );
            else if( EQUAL(layer->items[i],"green") && rlinfo->qc_green )
                sprintf( szWork, "%d", rlinfo->qc_green[record] );
            else if( EQUAL(layer->items[i],"blue") && rlinfo->qc_blue )
                sprintf( szWork, "%d", rlinfo->qc_blue[record] );
            else if( EQUAL(layer->items[i],"count") && rlinfo->qc_count )
                sprintf( szWork, "%d", rlinfo->qc_count[record] );

            shape->values[i] = strdup(szWork);
        }
    }

/* -------------------------------------------------------------------- */
/*      Eventually we should likey apply the geometry properly but      */
/*      we don't really care about the geometry for query purposes.     */
/* -------------------------------------------------------------------- */

    return MS_SUCCESS;
#endif /* def USE_GDAL */
}

/************************************************************************/
/*                       msRASTERLayerGetItems()                        */
/************************************************************************/

int msRASTERLayerGetItems(layerObj *layer)
{
#ifndef USE_GDAL
    return MS_FAILURE;
#else
    rasterLayerInfo *rlinfo = (rasterLayerInfo *) layer->layerinfo;

    if( rlinfo == NULL )
        return MS_FAILURE;

    layer->items = (char **) calloc(sizeof(char *),10);

    layer->numitems = 0;
    if( rlinfo->qc_x )
        layer->items[layer->numitems++] = strdup("x");
    if( rlinfo->qc_y )
        layer->items[layer->numitems++] = strdup("y");
    if( rlinfo->qc_values )
    {
        int i;
        for( i = 0; i < rlinfo->band_count; i++ )
        {
            char szName[100];
            sprintf( szName, "value_%d", i );
            layer->items[layer->numitems++] = strdup(szName);
        }
        layer->items[layer->numitems++] = strdup("value_list");
    }
    if( rlinfo->qc_class )
        layer->items[layer->numitems++] = strdup("class");
    if( rlinfo->qc_red )
        layer->items[layer->numitems++] = strdup("red");
    if( rlinfo->qc_green )
        layer->items[layer->numitems++] = strdup("green");
    if( rlinfo->qc_blue )
        layer->items[layer->numitems++] = strdup("blue");
    if( rlinfo->qc_count )
        layer->items[layer->numitems++] = strdup("count");

    return msRASTERLayerInitItemInfo(layer);
#endif /* def USE_GDAL */
}

/************************************************************************/
/*                       msRASTERLayerGetExtent()                       */
/************************************************************************/

int msRASTERLayerGetExtent(layerObj *layer, rectObj *extent)

{ 
#ifndef USE_GDAL
    return MS_FAILURE;
#else
  char szPath[MS_MAXPATHLEN];
  mapObj *map = layer->map;
  double adfGeoTransform[6];
  int nXSize, nYSize;
  GDALDatasetH hDS;
  CPLErr eErr = CE_Failure;

  /*
  ** For the time being we only automatically derive extents from
  ** single raster files.  Opening many raster files from a tile index
  ** in order to get the extent would be potentially very expensive.  We
  ** could - in theory - scan the tile index and build up an extent from
  ** the polygons but that is quite complicated code wise, so I am leaving
  ** that till someone cares more about the issue.  (#79)
  */
  if( !layer->data 
      || strlen(layer->data) == 0 
      || layer->tileindex != NULL )
  {
    /* should we be issuing a specific error about not supporting 
       extents for tileindexed raster layers? */
    return MS_FAILURE;
  }

  if( map == NULL )
    return MS_FAILURE;

  msTryBuildPath3(szPath, map->mappath, map->shapepath, layer->data);

  msAcquireLock( TLOCK_GDAL );
  hDS = GDALOpen(szPath, GA_ReadOnly );
  
  if( hDS != NULL )
  {
    nXSize = GDALGetRasterXSize( hDS );
    nYSize = GDALGetRasterYSize( hDS );
    eErr = GDALGetGeoTransform( hDS, adfGeoTransform );
    
    GDALClose( hDS );
  }
  
  msReleaseLock( TLOCK_GDAL );

  if( hDS == NULL || eErr != CE_None )
  {
      return MS_FAILURE;
  }

  /* If this appears to be an ungeoreferenced raster than flip it for
     mapservers purposes. */
  if( adfGeoTransform[5] == 1.0 && adfGeoTransform[3] == 0.0 )
  {
      adfGeoTransform[5] = -1.0;
      adfGeoTransform[3] = nYSize;
  }
  
  extent->minx = adfGeoTransform[0];
  extent->maxy = adfGeoTransform[3];
  
  extent->maxx = adfGeoTransform[0] + nXSize * adfGeoTransform[1];
  extent->miny = adfGeoTransform[3] + nYSize * adfGeoTransform[5];
  
  return MS_SUCCESS;
#endif /* def USE_GDAL */
}


/************************************************************************/
/*                     msRASTERLayerSetTimeFilter()                     */
/*                                                                      */
/*      This function is actually just used in the context of           */
/*      setting a filter on the tileindex for time based queries.       */
/*      For instance via WMS requests.  So it isn't really related      */
/*      to the "raster query" support at all.                           */
/*                                                                      */
/*      If a local shapefile tileindex is in use, we will set a         */
/*      backtics filter (shapefile compatible).  If another layer is    */
/*      being used as the tileindex then we will forward the            */
/*      SetTimeFilter call to it.  If there is no tileindex in          */
/*      place, we do nothing.                                           */
/************************************************************************/

int msRASTERLayerSetTimeFilter(layerObj *layer, const char *timestring, 
                               const char *timefield)
{
    int tilelayerindex;

/* -------------------------------------------------------------------- */
/*      If we don't have a tileindex the time filter has no effect.     */
/* -------------------------------------------------------------------- */
    if( layer->tileindex == NULL ) 
        return MS_SUCCESS;

/* -------------------------------------------------------------------- */
/*      Find the tileindex layer.                                       */
/* -------------------------------------------------------------------- */
    tilelayerindex = msGetLayerIndex(layer->map, layer->tileindex);

/* -------------------------------------------------------------------- */
/*      If we are using a local shapefile as our tileindex (that is     */
/*      to say, the tileindex name is not of another layer), then we    */
/*      just install a backtics style filter on the raster layer.       */
/*      This is propogated to the "working layer" created for the       */
/*      tileindex by code in mapraster.c.                               */
/* -------------------------------------------------------------------- */
    if( tilelayerindex == -1 )
        return msLayerMakeBackticsTimeFilter( layer, timestring, timefield );

/* -------------------------------------------------------------------- */
/*      Otherwise we invoke the tileindex layers SetTimeFilter          */
/*      method.                                                         */
/* -------------------------------------------------------------------- */
	if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
		return MS_FAILURE;
    return msLayerSetTimeFilter( layer->GET_LAYER(map,tilelayerindex),
                                 timestring, timefield );
}

/************************************************************************/
/*                msRASTERLayerInitializeVirtualTable()                 */
/************************************************************************/

int
msRASTERLayerInitializeVirtualTable(layerObj *layer)
{
    assert(layer != NULL);
    assert(layer->vtable != NULL);

    layer->vtable->LayerInitItemInfo = msRASTERLayerInitItemInfo;
    layer->vtable->LayerFreeItemInfo = msRASTERLayerFreeItemInfo;
    layer->vtable->LayerOpen = msRASTERLayerOpen;
    layer->vtable->LayerIsOpen = msRASTERLayerIsOpen;
    layer->vtable->LayerWhichShapes = msRASTERLayerWhichShapes;
    layer->vtable->LayerNextShape = msRASTERLayerNextShape;
    layer->vtable->LayerResultsGetShape = msRASTERLayerGetShape; /* no special version, use ...GetShape() */
    layer->vtable->LayerGetShape = msRASTERLayerGetShape;
    layer->vtable->LayerClose = msRASTERLayerClose;
    layer->vtable->LayerGetItems = msRASTERLayerGetItems;
    layer->vtable->LayerGetExtent = msRASTERLayerGetExtent;
    /* layer->vtable->LayerGetAutoStyle, use default */
    /* layer->vtable->LayerApplyFilterToLayer, use default */
    layer->vtable->LayerCloseConnection = msRASTERLayerClose;
    /* we use backtics for proper tileindex shapefile functioning */
    layer->vtable->LayerSetTimeFilter = msRASTERLayerSetTimeFilter;
    /* layer->vtable->LayerCreateItems, use default */
    /* layer->vtable->LayerGetNumFeatures, use default */

    return MS_SUCCESS;
}


/******************************************************************************
 * $Id$
 *
 * Project:  CFS OGC MapServer
 * Purpose:  Assorted code related to resampling rasters.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam, DM Solutions Group Inc
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
 * Revision 1.5  2001/03/30 01:13:53  dan
 * Take cellsize into account when calculating temporary buffer size in
 * msResampleGDALToMap().
 *
 * Revision 1.4  2001/03/21 04:02:56  frank
 * use pj_is_latlong
 *
 * Revision 1.3  2001/03/16 15:11:17  frank
 * fixed bugs for geographic, don't write interim.png
 *
 * Revision 1.2  2001/03/14 17:55:36  frank
 * fixed bug in non-GDAL case
 *
 * Revision 1.1  2001/03/14 17:39:16  frank
 * New
 *
 */

#include <assert.h>
#include "mapresample.h"

/* The amount of "extra" resolution we will load for our resampling source */
#define RES_RATIO	2.0

#ifdef USE_GDAL
int drawGDAL(mapObj *map, layerObj *layer, gdImagePtr img, 
             GDALDatasetH hDS );
#endif

/************************************************************************/
/*                       msSimpleRasterResample()                       */
/************************************************************************/

int msSimpleRasterResampler( gdImagePtr psSrcImage, int nOffsite, 
                             gdImagePtr psDstImage,
                             SimpleTransformer pfnTransform, void *pCBData )

{
    double	*x, *y; 
    int		nDstX, nDstY;
    int         *panSuccess;
    int		nDstXSize = gdImageSX(psDstImage);
    int		nDstYSize = gdImageSY(psDstImage);
    int		nSrcXSize = gdImageSX(psSrcImage);
    int		nSrcYSize = gdImageSY(psSrcImage);
    x = (double *) malloc( sizeof(double) * nDstXSize );
    y = (double *) malloc( sizeof(double) * nDstXSize );
    panSuccess = (int *) malloc( sizeof(int) * nDstXSize );

    for( nDstY = 0; nDstY < nDstYSize; nDstY++ )
    {
        for( nDstX = 0; nDstX < nDstXSize; nDstX++ )
        {
            x[nDstX] = nDstX + 0.5;
            y[nDstX] = nDstY + 0.5;
        }

        pfnTransform( pCBData, nDstXSize, x, y, panSuccess );
        
        for( nDstX = 0; nDstX < nDstXSize; nDstX++ )
        {
            int		nSrcX, nSrcY;
            int		nValue;

            if( !panSuccess[nDstX] )
                continue;

            nSrcX = (int) x[nDstX];
            nSrcY = (int) y[nDstX];

            if( x[nDstX] < 0.0 || y[nDstX] < 0.0
                || nSrcX >= nSrcXSize || nSrcY >= nSrcYSize )
                continue;

            nValue = psSrcImage->pixels[nSrcY][nSrcX];
            
            if( nValue == nOffsite )
                continue;

            psDstImage->pixels[nDstY][nDstX] = nValue; 
        }
    }

    free( panSuccess );
    free( x );
    free( y );

    return 0;
}

#ifdef USE_PROJ
typedef struct 
{
    PJ *psSrcProj;
    int bSrcIsGeographic;
    double adfSrcGeoTransform[6];
    PJ *psDstProj;
    int bDstIsGeographic;
    double adfDstGeoTransform[6];
} msProjTransformInfo;

/************************************************************************/
/*                       msInitProjTransformer()                        */
/************************************************************************/

void *msInitProjTransformer( projectionObj *psSrc, 
                             double *padfSrcGeoTransform, 
                             projectionObj *psDst, 
                             double *padfDstGeoTransform )

{
    msProjTransformInfo	*psPTInfo;

    psPTInfo = (msProjTransformInfo *) malloc(sizeof(msProjTransformInfo));

    psPTInfo->psSrcProj = psSrc->proj;
    psPTInfo->bSrcIsGeographic = pj_is_latlong(psSrc->proj);
    memcpy( psPTInfo->adfSrcGeoTransform, padfSrcGeoTransform, 
            sizeof(double) * 6 );

    psPTInfo->psDstProj = psDst->proj;
    psPTInfo->bDstIsGeographic = pj_is_latlong(psDst->proj);
    memcpy( psPTInfo->adfDstGeoTransform, padfDstGeoTransform, 
            sizeof(double) * 6 );

    return psPTInfo;
}

/************************************************************************/
/*                       msFreeProjTransformer()                        */
/************************************************************************/

void msFreeProjTransformer( void * pCBData )

{
    free( pCBData );
}

/************************************************************************/
/*                          msProjTransformer                           */
/************************************************************************/

int msProjTransformer( void *pCBData, int nPoints, 
                       double *x, double *y, int *panSuccess )

{
    int		i;
    msProjTransformInfo	*psPTInfo = (msProjTransformInfo*) pCBData;
    double	x_out;

/* -------------------------------------------------------------------- */
/*      Transform into destination georeferenced space.                 */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nPoints; i++ )
    {
        x_out = psPTInfo->adfDstGeoTransform[0] 
            + psPTInfo->adfDstGeoTransform[1] * x[i]
            + psPTInfo->adfDstGeoTransform[2] * y[i];
        y[i] = psPTInfo->adfDstGeoTransform[3] 
            + psPTInfo->adfDstGeoTransform[4] * x[i]
            + psPTInfo->adfDstGeoTransform[5] * y[i];
        x[i] = x_out;

        panSuccess[i] = 1;
    }
        
/* -------------------------------------------------------------------- */
/*      Transform from degrees to radians if geographic.                */
/* -------------------------------------------------------------------- */
    if( psPTInfo->bDstIsGeographic )
    {
        for( i = 0; i < nPoints; i++ )
        {
            x[i] = x[i] * DEG_TO_RAD;
            y[i] = y[i] * DEG_TO_RAD;
        }
    }

/* -------------------------------------------------------------------- */
/*      Transform back to source projection space.                      */
/* -------------------------------------------------------------------- */
    if( psPTInfo->psDstProj != NULL
        && psPTInfo->psSrcProj != NULL )
    {
        if( pj_transform( psPTInfo->psDstProj, psPTInfo->psSrcProj, 
                          nPoints, 1, x, y, NULL ) != 0 )
        {
            for( i = 0; i < nPoints; i++ )
                panSuccess[i] = 0;

            return FALSE;
        }
    }

/* -------------------------------------------------------------------- */
/*      Transform back to degrees if source is geographic.              */
/* -------------------------------------------------------------------- */
    if( psPTInfo->bSrcIsGeographic )
    {
        for( i = 0; i < nPoints; i++ )
        {
            x[i] = x[i] * DEG_TO_RAD;
            y[i] = y[i] * DEG_TO_RAD;
        }
    }

/* -------------------------------------------------------------------- */
/*      Transform to source raster space.                               */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nPoints; i++ )
    {
        x_out = (x[i] - psPTInfo->adfSrcGeoTransform[0])
            / psPTInfo->adfSrcGeoTransform[1];
        y[i] = (y[i] - psPTInfo->adfSrcGeoTransform[3])
            / psPTInfo->adfSrcGeoTransform[5];
        x[i] = x_out;
    }

    return 1;
}
#endif /* def USE_PROJ */

#ifdef USE_GDAL
/************************************************************************/
/*                        msResampleGDALToMap()                         */
/************************************************************************/

int msResampleGDALToMap( mapObj *map, layerObj *layer, gdImagePtr img, 
                         GDALDatasetH hDS )

{
/* -------------------------------------------------------------------- */
/*      We require PROJ.4 4.4.2 or later.  Earlier versions don't       */
/*      have PJD_GRIDSHIFT.                                             */
/* -------------------------------------------------------------------- */
#ifndef PJD_GRIDSHIFT 
    return 0;
#else
#define EDGE_STEPS    10
    int		i, nSamples=0, nSrcXSize, nSrcYSize, nDstXSize, nDstYSize;
    int		result;
    double      dfRatio;
    double	x[EDGE_STEPS*4+4], y[EDGE_STEPS*4+4], z[EDGE_STEPS*4+4];
    double	adfSrcGeoTransform[6], adfDstGeoTransform[6];
    rectObj	sSrcExtent;
    mapObj	sDummyMap;
    gdImagePtr  srcImg;
    void	*pTCBData;

/* -------------------------------------------------------------------- */
/*      We will require source and destination to have a valid          */
/*      projection object.                                              */
/* -------------------------------------------------------------------- */
    if( map->projection.proj == NULL
        || layer->projection.proj == NULL )
        return MS_PROJERR;

/* -------------------------------------------------------------------- */
/*      Initialize some information.                                    */
/* -------------------------------------------------------------------- */
    nDstXSize = gdImageSX(img);
    nDstYSize = gdImageSY(img);

    adfDstGeoTransform[0] = map->extent.minx;
    adfDstGeoTransform[1] = map->cellsize;
    adfDstGeoTransform[2] = 0.0;
    adfDstGeoTransform[3] = map->extent.maxy;
    adfDstGeoTransform[4] = 0.0;
    adfDstGeoTransform[5] = - map->cellsize;

    GDALGetGeoTransform( hDS, adfSrcGeoTransform );
    nSrcXSize = GDALGetRasterXSize( hDS );
    nSrcYSize = GDALGetRasterYSize( hDS );

/* -------------------------------------------------------------------- */
/*      We need to find the extents in the source layer projection      */
/*      of the output requested region.  We will accomplish this by     */
/*      collecting the extents of a region around the edge of the       */
/*      destination chunk.                                              */
/* -------------------------------------------------------------------- */

    /* Collect edges in map image pixel/line coordinates */
    for( dfRatio = 0.0; dfRatio <= 0.999; dfRatio += (1.0/EDGE_STEPS) )
    {
        assert( nSamples <= EDGE_STEPS*4 );
        x[nSamples  ] = dfRatio * nDstXSize;
        y[nSamples++] = 0.0;
        x[nSamples  ] = dfRatio * nDstXSize;
        y[nSamples++] = nDstYSize;
        x[nSamples  ] = 0.0;
        y[nSamples++] = dfRatio * nDstYSize;
        x[nSamples  ] = nDstXSize;
        y[nSamples++] = dfRatio * nDstYSize;
    }

    /* transform to map georeferenced units */
    for( i = 0; i < nSamples; i++ )
    {
        double		x_out, y_out; 

        x_out = map->extent.minx + x[i] * map->cellsize;
        y_out = map->extent.maxy - y[i] * map->cellsize;

        x[i] = x_out;
        y[i] = y_out;
        z[i] = 0.0;
    }

    if( pj_is_latlong(map->projection.proj) )
    {
        for( i = 0; i < nSamples; i++ )
        {
            x[i] = x[i] * DEG_TO_RAD;
            y[i] = y[i] * DEG_TO_RAD;
        }
    }

    /* transform to layer georeferenced coordinates. */
    if( pj_transform( map->projection.proj, layer->projection.proj, 
                      nSamples, 1, x, y, z ) != 0 )
    {
        char	szErrorMsg[2048];

        sprintf( szErrorMsg, 
                 "%spj_transform() failed for edge point(s).",
                 pj_strerrno(pj_errno));

        msSetError(MS_PROJERR, szErrorMsg, "msResampleGDALToMap()" );
        return MS_PROJERR;
    }

    if( pj_is_latlong(layer->projection.proj) )
    {
        for( i = 0; i < nSamples; i++ )
        {
            x[i] = x[i] * RAD_TO_DEG;
            y[i] = y[i] * RAD_TO_DEG;
        }
    }

    /* transform to layer raster coordinates, and collect bounds. */
    for( i = 0; i < nSamples; i++ )
    {
        double		x_out, y_out; 

        x_out = (x[i] - adfSrcGeoTransform[0]) / adfSrcGeoTransform[1];
        y_out = (y[i] - adfSrcGeoTransform[3]) / adfSrcGeoTransform[5];

        if( i == 0 )
        {
            sSrcExtent.minx = sSrcExtent.maxx = x_out;
            sSrcExtent.miny = sSrcExtent.maxy = y_out;
        }
        else
        {
            sSrcExtent.minx = MIN(sSrcExtent.minx, x_out);
            sSrcExtent.maxx = MAX(sSrcExtent.maxx, x_out);
            sSrcExtent.miny = MIN(sSrcExtent.miny, y_out);
            sSrcExtent.maxy = MAX(sSrcExtent.maxy, y_out);
        }
    }

/* -------------------------------------------------------------------- */
/*      Project desired extents out by 2 pixels, and then trip to       */
/*      available data.                                                 */
/* -------------------------------------------------------------------- */
    sSrcExtent.minx = floor(sSrcExtent.minx-1.0);
    sSrcExtent.maxx = ceil (sSrcExtent.maxx+1.0);
    sSrcExtent.miny = floor(sSrcExtent.miny-1.0);
    sSrcExtent.maxy = ceil (sSrcExtent.maxy+1.0);

    sSrcExtent.minx = MAX(0,sSrcExtent.minx);
    sSrcExtent.maxx = MIN(sSrcExtent.maxx, nSrcXSize );
    sSrcExtent.miny = MAX(sSrcExtent.miny, 0 );
    sSrcExtent.maxy = MIN(sSrcExtent.maxy, nSrcYSize );

    if( sSrcExtent.maxx <= sSrcExtent.minx 
        || sSrcExtent.maxy <= sSrcExtent.miny )
        return 0;
    
/* -------------------------------------------------------------------- */
/*      Decide on a resolution to read from the source image at.  We    */
/*      will operate from full resolution data, if we are requesting    */
/*      at near to full resolution.  Otherwise we will read the data    */
/*      at twice the resolution of the eventual map.                    */
/* -------------------------------------------------------------------- */
    
    if( (sSrcExtent.maxx - sSrcExtent.minx) > RES_RATIO * nDstXSize )
        sDummyMap.cellsize = 
            (adfSrcGeoTransform[1] * (sSrcExtent.maxx - sSrcExtent.minx))
            / (RES_RATIO * nDstXSize);
    else
        sDummyMap.cellsize = adfSrcGeoTransform[1];

    msDebug( "cellsize = %f\n", sDummyMap.cellsize );

    sDummyMap.extent.minx = sSrcExtent.minx * adfSrcGeoTransform[1]
        + adfSrcGeoTransform[0];
    sDummyMap.extent.maxx = sSrcExtent.maxx * adfSrcGeoTransform[1]
        + adfSrcGeoTransform[0];
    sDummyMap.extent.maxy = sSrcExtent.miny * adfSrcGeoTransform[5]
        + adfSrcGeoTransform[3];
    sDummyMap.extent.miny = sSrcExtent.maxy * adfSrcGeoTransform[5]
        + adfSrcGeoTransform[3];
    
/* -------------------------------------------------------------------- */
/*      Setup a dummy map object we can use to read from the source     */
/*      raster, with the newly established extents, and resolution.     */
/* -------------------------------------------------------------------- */
    srcImg = gdImageCreate( (int) ((sSrcExtent.maxx - 
                                    sSrcExtent.minx)/sDummyMap.cellsize+0.01), 
                            (int) ((sSrcExtent.maxy - 
                                    sSrcExtent.miny)/sDummyMap.cellsize+0.01));
    gdImagePaletteCopy( srcImg, img );

/* -------------------------------------------------------------------- */
/*      Draw into it, and then copy out the updated palette.            */
/* -------------------------------------------------------------------- */
    result = drawGDAL( &sDummyMap, layer, srcImg, hDS );
    if( result )
    {
        gdImageDestroy( srcImg );
        return result;
    }

    gdImagePaletteCopy( img, srcImg );

/* -------------------------------------------------------------------- */
/*      Setup transformations between our source image, and the         */
/*      target map image.                                               */
/* -------------------------------------------------------------------- */
    adfSrcGeoTransform[0] = sDummyMap.extent.minx;
    adfSrcGeoTransform[1] = sDummyMap.cellsize;
    adfSrcGeoTransform[2] = 0.0;
    adfSrcGeoTransform[3] = sDummyMap.extent.maxy;
    adfSrcGeoTransform[4] = 0.0;
    adfSrcGeoTransform[5] = -sDummyMap.cellsize;

    pTCBData = msInitProjTransformer( &(layer->projection), 
                                      adfSrcGeoTransform, 
                                      &(map->projection), 
                                      adfDstGeoTransform );
    
    if( pTCBData == NULL )
    {
        gdImageDestroy( srcImg );
        return MS_PROJERR;
    }

/* -------------------------------------------------------------------- */
/*      Perform the resampling.                                         */
/* -------------------------------------------------------------------- */
    result = msSimpleRasterResampler( srcImg, layer->offsite, img, 
                                      msProjTransformer, pTCBData );

/* -------------------------------------------------------------------- */
/*      cleanup                                                         */
/* -------------------------------------------------------------------- */
#ifdef notdef
    {
        FILE	*fp;

        fp = fopen( "interim.png", "wb" );
        gdImagePng( srcImg, fp );
        fclose( fp );
    }
#endif

    gdImageDestroy( srcImg );

    msFreeProjTransformer( pTCBData );
    
    return result;
#endif
}

#endif /* def USE_GDAL */

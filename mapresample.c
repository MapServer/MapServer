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
 * Revision 1.21  2001/11/02 22:41:57  dan
 * Test for nSrcX or nSrcY < 0 in msSimpleRasterResampler() to prevent crash
 * when accessing pixels buffer.
 *
 * Revision 1.20  2001/11/02 16:13:00  frank
 * fixed degree/radian conversion bug in msProjTransformer
 *
 * Revision 1.19  2001/09/10 13:33:18  frank
 * modified to avoid using layer->data since it may be NULL
 *
 * Revision 1.18  2001/09/05 13:24:47  frank
 * fixed last fix related to Z coordinate handling
 *
 * Revision 1.17  2001/09/04 13:20:52  frank
 * ensure Z is passed to pj_transform for datum shifts
 *
 * Revision 1.16  2001/08/22 04:33:01  dan
 * Try calling GDALReadWorldFile() if GDALGetGeoTransform() fails.
 * When resampling layers with an offsite then init temporary image BG
 * using the offsite color.
 *
 * Revision 1.15  2001/06/28 18:22:07  frank
 * improve debug message
 *
 * Revision 1.14  2001/06/25 17:47:16  frank
 * added linear approximator transformer
 *
 * Revision 1.13  2001/05/22 18:02:59  frank
 * ensure MIN and MAX are defined
 *
 * Revision 1.12  2001/05/22 02:47:09  frank
 * ifdef out transformer func if no USE_PROJ
 *
 * Revision 1.11  2001/05/03 16:25:18  frank
 * map->extents are for center of pixel
 *
 * Revision 1.10  2001/05/02 19:46:32  frank
 * Fixed bug 7, dealing with map extents that fail to reproject
 *
 * Revision 1.9  2001/04/26 15:08:21  dan
 * Return MS_FALSE instead of FALSE (undefined if gdal.h not included).
 *
 * Revision 1.8  2001/04/09 13:27:34  frank
 * implemented limited support for rotated GDAL data sources
 *
 * Revision 1.7  2001/04/07 17:32:38  frank
 * Fixed up quirk in sizing of srcImg.
 * Added true inverse geotransform support, and support for rotated source.
 * May still be problems with cellsize selection for very rotated source
 * images.
 *
 * Revision 1.6  2001/04/06 01:17:31  frank
 * use proj_api.h if available (PROJ.4.4.3)
 *
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

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
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
    int		nFailedPoints = 0, nSetPoints = 0;

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
            {
                nFailedPoints++;
                continue;
            }

            nSrcX = (int) x[nDstX];
            nSrcY = (int) y[nDstX];

            if( nSrcX < 0.0 || nSrcY < 0.0
                || nSrcX >= nSrcXSize || nSrcY >= nSrcYSize )
                continue;

            nValue = psSrcImage->pixels[nSrcY][nSrcX];
            
            if( nValue == nOffsite )
                continue;

            nSetPoints++;
            psDstImage->pixels[nDstY][nDstX] = nValue; 
        }
    }

    free( panSuccess );
    free( x );
    free( y );

/* -------------------------------------------------------------------- */
/*      Some debugging output.                                          */
/* -------------------------------------------------------------------- */
#ifndef notdef
    if( nFailedPoints > 0 )
    {
        char	szMsg[256];

        sprintf( szMsg, 
                 "msSimpleRasterResampler: "
                 "%d failed to transform, %d actually set.\n", 
                 nFailedPoints, nSetPoints );
        msDebug( szMsg );
    }
#endif

    return 0;
}

/************************************************************************/
/*                          InvGeoTransform()                           */
/*                                                                      */
/*      Invert a standard 3x2 "GeoTransform" style matrix with an       */
/*      implicit [1 0 0] final row.                                     */
/************************************************************************/

int InvGeoTransform( double *gt_in, double *gt_out )

{
    double	det, inv_det;

    /* we assume a 3rd row that is [1 0 0] */

    /* Compute determinate */

    det = gt_in[1] * gt_in[5] - gt_in[2] * gt_in[4];

    if( fabs(det) < 0.000000000000001 )
        return 0;

    inv_det = 1.0 / det;

    /* compute adjoint, and devide by determinate */

    gt_out[1] =  gt_in[5] * inv_det;
    gt_out[4] = -gt_in[4] * inv_det;

    gt_out[2] = -gt_in[2] * inv_det;
    gt_out[5] =  gt_in[1] * inv_det;

    gt_out[0] = ( gt_in[2] * gt_in[3] - gt_in[0] * gt_in[5]) * inv_det;
    gt_out[3] = (-gt_in[1] * gt_in[3] + gt_in[0] * gt_in[4]) * inv_det;

    return 1;
}

/************************************************************************/
/* ==================================================================== */
/*      PROJ.4 based transformer.					*/
/* ==================================================================== */
/************************************************************************/

#ifdef USE_PROJ
typedef struct 
{
    projPJ psSrcProj;
    int bSrcIsGeographic;
    double adfInvSrcGeoTransform[6];

    projPJ psDstProj;
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

/* -------------------------------------------------------------------- */
/*      Record source image information.  We invert the source          */
/*      transformation for more convenient inverse application in       */
/*      the transformer.                                                */
/* -------------------------------------------------------------------- */
    psPTInfo->psSrcProj = psSrc->proj;
    psPTInfo->bSrcIsGeographic = pj_is_latlong(psSrc->proj);
    
    if( !InvGeoTransform(padfSrcGeoTransform, 
                         psPTInfo->adfInvSrcGeoTransform) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Record destination image information.                           */
/* -------------------------------------------------------------------- */
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
        double *z;
        
        z = (double *) calloc(sizeof(double),nPoints);
        if( pj_transform( psPTInfo->psDstProj, psPTInfo->psSrcProj, 
                          nPoints, 1, x, y,  z) != 0 )
        {
            free( z );
            for( i = 0; i < nPoints; i++ )
                panSuccess[i] = 0;

            return MS_FALSE;
        }
        free( z );
    }

/* -------------------------------------------------------------------- */
/*      Transform back to degrees if source is geographic.              */
/* -------------------------------------------------------------------- */
    if( psPTInfo->bSrcIsGeographic )
    {
        for( i = 0; i < nPoints; i++ )
        {
            x[i] = x[i] * RAD_TO_DEG;
            y[i] = y[i] * RAD_TO_DEG;
        }
    }

/* -------------------------------------------------------------------- */
/*      Transform to source raster space.                               */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nPoints; i++ )
    {
        x_out = psPTInfo->adfInvSrcGeoTransform[0] 
            + psPTInfo->adfInvSrcGeoTransform[1] * x[i]
            + psPTInfo->adfInvSrcGeoTransform[2] * y[i];
        y[i] = psPTInfo->adfInvSrcGeoTransform[3] 
            + psPTInfo->adfInvSrcGeoTransform[4] * x[i]
            + psPTInfo->adfInvSrcGeoTransform[5] * y[i];
        x[i] = x_out;
    }

    return 1;
}
#endif /* def USE_PROJ */

/************************************************************************/
/* ==================================================================== */
/*      Approximate transformer.                                        */
/* ==================================================================== */
/************************************************************************/

typedef struct 
{
    SimpleTransformer pfnBaseTransformer;
    void             *pBaseCBData;

    double	      dfMaxError;
} msApproxTransformInfo;

/************************************************************************/
/*                      msInitApproxTransformer()                       */
/************************************************************************/

static void *msInitApproxTransformer( SimpleTransformer pfnBaseTransformer, 
                                      void *pBaseCBData,
                                      double dfMaxError )

{
    msApproxTransformInfo	*psATInfo;

    psATInfo = (msApproxTransformInfo *) malloc(sizeof(msApproxTransformInfo));
    psATInfo->pfnBaseTransformer = pfnBaseTransformer;
    psATInfo->pBaseCBData = pBaseCBData;
    psATInfo->dfMaxError = dfMaxError;

    return psATInfo;
}

/************************************************************************/
/*                      msFreeApproxTransformer()                       */
/************************************************************************/

static void msFreeApproxTransformer( void * pCBData )

{
    free( pCBData );
}

/************************************************************************/
/*                         msApproxTransformer                          */
/************************************************************************/

static int msApproxTransformer( void *pCBData, int nPoints, 
                                double *x, double *y, int *panSuccess )

{
    msApproxTransformInfo *psATInfo = (msApproxTransformInfo *) pCBData;
    double x2[3], y2[3], dfDeltaX, dfDeltaY, dfError, dfDist;
    int nMiddle, anSuccess2[3], i, bSuccess;

    nMiddle = (nPoints-1)/2;

/* -------------------------------------------------------------------- */
/*      Bail if our preconditions are not met, or if error is not       */
/*      acceptable.                                                     */
/* -------------------------------------------------------------------- */
    if( y[0] != y[nPoints-1] || y[0] != y[nMiddle]
        || x[0] == x[nPoints-1] || x[0] == x[nMiddle]
        || psATInfo->dfMaxError == 0.0 || nPoints <= 5 )
    {
        return psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, nPoints,
                                             x, y, panSuccess );
    }

/* -------------------------------------------------------------------- */
/*      Transform first, last and middle point.                         */
/* -------------------------------------------------------------------- */
    x2[0] = x[0];
    y2[0] = y[0];
    x2[1] = x[nMiddle];
    y2[1] = y[nMiddle];
    x2[2] = x[nPoints-1];
    y2[2] = y[nPoints-1];

    bSuccess = 
        psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, 3, x2, y2, 
                                      anSuccess2 );
    if( !bSuccess )
        return psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, nPoints,
                                             x, y, panSuccess );
    
/* -------------------------------------------------------------------- */
/*      Is the error at the middle acceptable relative to an            */
/*      interpolation of the middle position?                           */
/* -------------------------------------------------------------------- */
    dfDeltaX = (x2[2] - x2[0]) / (x[nPoints-1] - x[0]);
    dfDeltaY = (y2[2] - y2[0]) / (x[nPoints-1] - x[0]);

    dfError = fabs((x2[0] + dfDeltaX * (x[nMiddle] - x[0])) - x2[1])
        + fabs((y2[0] + dfDeltaY * (x[nMiddle] - x[0])) - y2[1]);

    if( dfError > psATInfo->dfMaxError )
    {
        bSuccess = 
            psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, nMiddle, 
                                          x, y, panSuccess );
            
        if( !bSuccess )
        {
            return psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, 
                                                 nPoints,
                                                 x, y, panSuccess );
        }

        bSuccess = 
            psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, 
                                          nPoints - nMiddle, 
                                          x+nMiddle, y+nMiddle, 
                                          panSuccess+nMiddle );

        if( !bSuccess )
        {
            return psATInfo->pfnBaseTransformer( psATInfo->pBaseCBData, 
                                                 nPoints,
                                                 x, y, panSuccess );
        }

        return 1;
    }

/* -------------------------------------------------------------------- */
/*      Error is OK, linearly interpolate all points along line.        */
/* -------------------------------------------------------------------- */
    for( i = nPoints-1; i >= 0; i-- )
    {
        dfDist = (x[i] - x[0]);
        y[i] = y2[0] + dfDeltaY * dfDist;
        x[i] = x2[0] + dfDeltaX * dfDist;
        panSuccess[i] = 1;
    }
    
    return 1;
}

/************************************************************************/
/*                       msTransformMapToSource()                       */
/*                                                                      */
/*      Compute the extents of the current map view if transformed      */
/*      onto the source raster.                                         */
/************************************************************************/

static int msTransformMapToSource( int nDstXSize, int nDstYSize, 
                                   double * adfDstGeoTransform,
                                   projectionObj *psDstProj,
                                   int nSrcXSize, int nSrcYSize, 
                                   double * adfInvSrcGeoTransform,
                                   projectionObj *psSrcProj,
                                   rectObj *psSrcExtent )

{
#ifdef USE_PROJ
#define EDGE_STEPS    10

    int		i, nSamples = 0;
    double      dfRatio;
    double	x[EDGE_STEPS*4+4], y[EDGE_STEPS*4+4], z[EDGE_STEPS*4+4];

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

        x_out = adfDstGeoTransform[0] + x[i] * adfDstGeoTransform[1];
        y_out = adfDstGeoTransform[3] + y[i] * adfDstGeoTransform[5];

        x[i] = x_out;
        y[i] = y_out;
        z[i] = 0.0;
    }

    if( pj_is_latlong(psDstProj->proj) )
    {
        for( i = 0; i < nSamples; i++ )
        {
            x[i] = x[i] * DEG_TO_RAD;
            y[i] = y[i] * DEG_TO_RAD;
        }
    }

    /* transform to layer georeferenced coordinates. */
    if( pj_transform( psDstProj->proj, psSrcProj->proj,
                      nSamples, 1, x, y, z ) != 0 )
    {
        msDebug( "msTransformMapToSource(): "
                 "pj_transform() failed.  Out of bounds?\n" );

        return MS_FALSE;
    }

    if( pj_is_latlong(psSrcProj->proj) )
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

        x_out =      adfInvSrcGeoTransform[0]
            +   x[i]*adfInvSrcGeoTransform[1]
            +   y[i]*adfInvSrcGeoTransform[2];
        y_out =      adfInvSrcGeoTransform[3]
            +   x[i]*adfInvSrcGeoTransform[4]
            +   y[i]*adfInvSrcGeoTransform[5];

        if( i == 0 )
        {
            psSrcExtent->minx = psSrcExtent->maxx = x_out;
            psSrcExtent->miny = psSrcExtent->maxy = y_out;
        }
        else
        {
            psSrcExtent->minx = MIN(psSrcExtent->minx, x_out);
            psSrcExtent->maxx = MAX(psSrcExtent->maxx, x_out);
            psSrcExtent->miny = MIN(psSrcExtent->miny, y_out);
            psSrcExtent->maxy = MAX(psSrcExtent->maxy, y_out);
        }
    }

    return MS_TRUE;
#else
    return MS_FALSE;
#endif
}


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
#if !defined(PJD_GRIDSHIFT) && !defined(PJ_VERSION)
    return 0;
#else
    int		nSrcXSize, nSrcYSize, nDstXSize, nDstYSize;
    int		result, bSuccess;
    double	adfSrcGeoTransform[6], adfDstGeoTransform[6];
    double      adfInvSrcGeoTransform[6];
    rectObj	sSrcExtent;
    mapObj	sDummyMap;
    gdImagePtr  srcImg;
    void	*pTCBData;
    void	*pACBData;

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

    adfDstGeoTransform[0] = map->extent.minx - map->cellsize*0.5;
    adfDstGeoTransform[1] = map->cellsize;
    adfDstGeoTransform[2] = 0.0;
    adfDstGeoTransform[3] = map->extent.maxy + map->cellsize*0.5;
    adfDstGeoTransform[4] = 0.0;
    adfDstGeoTransform[5] = - map->cellsize;

    if (GDALGetGeoTransform( hDS, adfSrcGeoTransform ) != CE_None
        && GDALGetDescription(hDS) != NULL )
    {
        GDALReadWorldFile(GDALGetDescription(hDS), "wld", adfSrcGeoTransform);
    }
    nSrcXSize = GDALGetRasterXSize( hDS );
    nSrcYSize = GDALGetRasterYSize( hDS );

    InvGeoTransform( adfSrcGeoTransform, adfInvSrcGeoTransform );

/* -------------------------------------------------------------------- */
/*      We need to find the extents in the source layer projection      */
/*      of the output requested region.  We will accomplish this by     */
/*      collecting the extents of a region around the edge of the       */
/*      destination chunk.                                              */
/* -------------------------------------------------------------------- */
    bSuccess = 
        msTransformMapToSource( nDstXSize, nDstYSize, adfDstGeoTransform,
                                &(map->projection),
                                nSrcXSize, nSrcYSize, adfInvSrcGeoTransform,
                                &(layer->projection),
                                &sSrcExtent );

/* -------------------------------------------------------------------- */
/*      If the transformation failed, it is likely that we have such    */
/*      broad extents that the projection transformation failed at      */
/*      points around the extents.  If so, we will assume we need       */
/*      the whole raster.  This and later assumptions are likely to     */
/*      result in the raster being loaded at a higher resolution        */
/*      than really needed but should give decent results.              */
/* -------------------------------------------------------------------- */
    if( !bSuccess )
    {
        sSrcExtent.minx = 0;
        sSrcExtent.maxx = nSrcXSize-1;
        sSrcExtent.miny = 0;
        sSrcExtent.maxy = nSrcYSize-1;
    }

/* -------------------------------------------------------------------- */
/*      Project desired extents out by 2 pixels, and then strip to      */
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

    msDebug( "msResampleGDALToMap in effect: cellsize = %f\n", 
             sDummyMap.cellsize );

    sDummyMap.extent.minx = adfSrcGeoTransform[0]
        + sSrcExtent.minx * adfSrcGeoTransform[1]
        + sSrcExtent.maxy * adfSrcGeoTransform[2];
    sDummyMap.extent.maxx = adfSrcGeoTransform[0]
        + sSrcExtent.maxx * adfSrcGeoTransform[1]
        + sSrcExtent.miny * adfSrcGeoTransform[2];
    sDummyMap.extent.miny = adfSrcGeoTransform[3]
        + sSrcExtent.minx * adfSrcGeoTransform[4]
        + sSrcExtent.maxy * adfSrcGeoTransform[5];
    sDummyMap.extent.maxy = adfSrcGeoTransform[3]
        + sSrcExtent.maxx * adfSrcGeoTransform[4]
        + sSrcExtent.miny * adfSrcGeoTransform[5];
    
/* -------------------------------------------------------------------- */
/*      Setup a dummy map object we can use to read from the source     */
/*      raster, with the newly established extents, and resolution.     */
/* -------------------------------------------------------------------- */
    srcImg = gdImageCreate( 
        (int) ((sDummyMap.extent.maxx - 
                sDummyMap.extent.minx)/sDummyMap.cellsize+0.01), 
        (int) ((sDummyMap.extent.maxy - 
                sDummyMap.extent.miny)/sDummyMap.cellsize+0.01));
    gdImagePaletteCopy( srcImg, img );

/* -------------------------------------------------------------------- */
/*      If layer has an offsite color then fill the BG of the           */
/*      temporary image with this color otherwise applying the          */
/*      offsite twice results in a solid color in the offsite area      */
/* -------------------------------------------------------------------- */
    if (layer->offsite >=0)
    {
        gdImageFilledRectangle(srcImg, 0, 0, 
                               gdImageSX(srcImg), gdImageSY(srcImg), 
                               layer->offsite);
    }

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
/*      It is cheaper to use linear approximations as long as our       */
/*      error is modest (less than 0.333 pixels).                       */
/* -------------------------------------------------------------------- */
    pACBData = msInitApproxTransformer( msProjTransformer, pTCBData, 0.333 );

/* -------------------------------------------------------------------- */
/*      Perform the resampling.                                         */
/* -------------------------------------------------------------------- */
    result = msSimpleRasterResampler( srcImg, layer->offsite, img, 
                                      msApproxTransformer, pACBData );

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
    msFreeApproxTransformer( pACBData );
    
    return result;
#endif
}

#endif /* def USE_GDAL */



/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of query operations on rasters. 
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.1  2004/03/09 21:11:08  frank
 * New
 *
 */

#include <assert.h>
#include "map.h"
#include "mapresample.h"
#include "mapthread.h"

/* ==================================================================== */
/*      For now the rasterLayerInfo lives here since it is just used    */
/*      to hold information related to queries.                         */
/* ==================================================================== */
typedef struct {

    /* query cache results */
    int query_results;
    int query_alloc_max;
    int query_request_max;
    int raster_query_mode

    double *qc_x;
    double *qc_y;
    double *qc_value;
    int    *qc_class;
    int    *qc_red;
    int    *qc_green;
    int    *qc_blue;
    int    *qc_count;
    int    *qc_tileindex;
} rasterLayerInfo;

#define RQM_UNKNOWN               0
#define RQM_ENTRY_PER_PIXEL       1
#define RQM_HIST_ON_CLASS         2
#define RQM_HIST_ON_VALUE         3

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
        free( rlinfo->qc_x );
    }

    if( rlinfo->qc_value )
        free( rlinfo->qc_value );

    if( rlinfo->qc_class )
    {
        free( rlinfo->qc_class );
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
}

/************************************************************************/
/*                       msRasterQueryAddPixel()                        */
/************************************************************************/



/************************************************************************/
/*                       msRasterQueryByRectLow()                       */
/************************************************************************/

static int 
msRasterQueryByRectLow(mapObj *map, layerObj *layer, GDALDatasetH hDS,
                       rectObj queryRect) 

{
    double	adfGeoTransform[6];
    
}

/************************************************************************/
/*                        msRasterQueryByRect()                         */
/************************************************************************/

int msRasterQueryByRect(mapObj *map, layerObj *layer, rectObj queryRect) 

{
    int status = MS_SUCCESS;
    char *filename=NULL;

    int t;
    int tileitemindex=-1;
    shapefileObj tilefile;
    char tilename[MS_PATH_LENGTH];
    int numtiles=1; /* always at least one tile */
    char szPath[MS_MAXPATHLEN];
    char cwd[MS_MAXPATHLEN];
    rectObj searchrect;

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

#ifndef USE_GDAL
    msSetError( MS_IMGERR, 
                "Rasters queries only supported with GDAL support enabled.",
                "msRasterQueryByRect()" );
    return MS_FAILURE;
#else
/* -------------------------------------------------------------------- */
/*      Open tile index if we have one.                                 */
/* -------------------------------------------------------------------- */
    if(layer->tileindex) { /* we have in index file */
        if(msSHPOpenFile(&tilefile, "rb", 
                         msBuildPath3(szPath, map->mappath, map->shapepath, 
                                      layer->tileindex)) == -1) 
            if(msSHPOpenFile(&tilefile, "rb", msBuildPath(szPath, map->mappath, layer->tileindex)) == -1) 
                return(MS_FAILURE);    

        tileitemindex = msDBFGetItemIndex(tilefile.hDBF, layer->tileitem);
        if(tileitemindex == -1) 
            return(MS_FAILURE);

        searchrect = queryRect;

#ifdef USE_PROJ
        if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
            msProjectRect(&map->projection, &layer->projection, &searchrect); // project the searchrect to source coords
#endif
        status = msSHPWhichShapes(&tilefile, searchrect, layer->debug);
        if(status != MS_SUCCESS) 
            numtiles = 0; // could be MS_DONE or MS_FAILURE
        else
            numtiles = tilefile.numshapes;
    }

/* -------------------------------------------------------------------- */
/*      Iterate over all tiles (just one in untiled case).              */
/* -------------------------------------------------------------------- */
    for(t=0; t<numtiles && status == MS_SUCCESS; t++) { 

        GDALDatasetH  hDS;

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

        msAcquireLock( TLOCK_GDAL );
        hDS = GDALOpen(
            msBuildPath3( szPath, map->mappath, map->shapepath, filename ), 
            GA_ReadOnly );
        
        if( hDS == NULL )
        {
            ms
            msReleaseLock( TLOCK_GDAL );
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

    } // next tile

    if(layer->tileindex) /* tiling clean-up */
        msSHPCloseFile(&tilefile);    

    return status;
}


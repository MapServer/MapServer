/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of support for output using GDAL.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam
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
 * Revision 1.3  2002/06/11 20:45:48  frank
 * fixed handling of temp files a bit
 *
 * Revision 1.2  2002/06/11 19:55:37  frank
 * added support for writing projection
 *
 * Revision 1.1  2002/06/11 13:49:52  frank
 * New
 *
 */

#include <assert.h>
#include "map.h"

#ifdef USE_GDAL

#include "gdal.h"
#include "ogr_srs_api.h"
#include "cpl_conv.h"
#include "cpl_string.h"

static char *msProjectionObjToWKT( projectionObj *proj );

/************************************************************************/
/*                           InitializeGDAL()                           */
/************************************************************************/

static void InitializeGDAL( void )

{
    static int    bGDALInitialized = 0;

    if( !bGDALInitialized )
    {
        GDALAllRegister();
        CPLPushErrorHandler( CPLQuietErrorHandler );
        bGDALInitialized = 1;
    }
}

/************************************************************************/
/*                          msSaveImageGDAL()                           */
/************************************************************************/

int msSaveImageGDAL( mapObj *map, gdImagePtr img, char *filename, 
                     outputFormatObj *format )

{
    int  bFileIsTemporary = MS_FALSE;
    GDALDatasetH hMemDS, hOutputDS;
    GDALDriverH  hMemDriver, hOutputDriver;
    int          nBands = 1;
    int          iLine;
    GByte       *pabyAlphaLine = NULL;
    char        **papszOptions = NULL;

    InitializeGDAL();

/* -------------------------------------------------------------------- */
/*      We will need to write the output to a temporary file and        */
/*      then stream to stdout if no filename is passed.                 */
/* -------------------------------------------------------------------- */
    if( filename == NULL )
    {
        filename = msTmpFile("/tmp/", "img.tmp");
        bFileIsTemporary = MS_TRUE;
    }
    
/* -------------------------------------------------------------------- */
/*      Create a memory dataset which we can use as a source for a      */
/*      CreateCopy().                                                   */
/* -------------------------------------------------------------------- */
    hMemDriver = GDALGetDriverByName( "MEM" );
    if( hMemDriver == NULL )
    {
        msSetError( MS_MISCERR, "Failed to find MEM driver.",
                    "msSaveImageGDAL()" );
        return MS_FAILURE;
    }
   
#if GD2_VERS > 1
    if( gdImageTrueColor( img ) && format->imagemode == MS_IMAGEMODE_RGB )
        nBands = 3;
    else if( gdImageTrueColor( img ) 
             && format->imagemode == MS_IMAGEMODE_RGBA )
    {
        pabyAlphaLine = (GByte *) calloc(img->sx,1);
        nBands = 4;
    }
#endif

    hMemDS = GDALCreate( hMemDriver, "msSaveImageGDAL_temp", 
                         img->sx, img->sy, nBands, GDT_Byte, NULL );
    if( hMemDS == NULL )
    {
        msSetError( MS_MISCERR, "Failed to create MEM dataset.",
                    "msSaveImageGDAL()" );
        return MS_FAILURE;
    }
    
/* -------------------------------------------------------------------- */
/*      Copy the gd image into the memory dataset.                      */
/* -------------------------------------------------------------------- */
    for( iLine = 0; iLine < img->sy; iLine++ )
    {
        int iBand;

        for( iBand = 0; iBand < nBands; iBand++ )
        {
            GDALRasterBandH hBand = GDALGetRasterBand( hMemDS, iBand+1 );

#if GD2_VERS > 1
            if( nBands > 1 && iBand < 3 )
            {
                // note, we need to fix handling of alpha!
                GDALRasterIO( hBand, GF_Write, 0, iLine, img->sx, 1, 
                              ((GByte *) img->tpixels[iLine])+(2-iBand), 
                              img->sx, 1, GDT_Byte, 4, 0 );
            }
            else if( nBands > 1 && iBand == 3 ) /* Alpha band */
            {
                int x;
                GByte *pabySrc = ((GByte *) img->tpixels[iLine])+3;

                for( x = 0; x < img->sx; x++ )
                {
                    if( *pabySrc == 127 )
                        pabyAlphaLine[x] = 0;
                    else
                        pabyAlphaLine[x] = 256 - 2 * *pabySrc;

                    pabySrc += 4;
                }

                GDALRasterIO( hBand, GF_Write, 0, iLine, img->sx, 1, 
                              pabyAlphaLine, img->sx, 1, GDT_Byte, 1, 0 );
            }
            else
#endif
            {
                GDALRasterIO( hBand, GF_Write, 0, iLine, img->sx, 1, 
                              img->pixels[iLine], img->sx, 1, GDT_Byte, 0, 0 );
            }
        }
    }

    if( pabyAlphaLine != NULL )
        free( pabyAlphaLine );

/* -------------------------------------------------------------------- */
/*      Attach the palette if appropriate.                              */
/* -------------------------------------------------------------------- */
    if( nBands == 1 )
    {
        GDALColorEntry sEntry;
        int  iColor;
        GDALColorTableH hCT;

        hCT = GDALCreateColorTable( GPI_RGB );

        for( iColor = 0; iColor < img->colorsTotal; iColor++ )
        {
            sEntry.c1 = img->red[iColor];
            sEntry.c2 = img->green[iColor];
            sEntry.c3 = img->blue[iColor];
            sEntry.c4 = 255;

            GDALSetColorEntry( hCT, iColor, &sEntry );
        }
        
        GDALSetRasterColorTable( GDALGetRasterBand( hMemDS, 1 ), hCT );

        GDALDestroyColorTable( hCT );
    }

/* -------------------------------------------------------------------- */
/*      Assign the projection and coordinate system to the memory       */
/*      dataset.                                                        */
/* -------------------------------------------------------------------- */

    if( map != NULL )
    {
        double adfGeoTransform[6];
        char *pszWKT;

        adfGeoTransform[0] = map->extent.minx;
        adfGeoTransform[1] = map->cellsize;
        adfGeoTransform[2] = 0.0;
        adfGeoTransform[3] = map->extent.maxy;
        adfGeoTransform[4] = 0.0;
        adfGeoTransform[5] = - map->cellsize;

        GDALSetGeoTransform( hMemDS, adfGeoTransform );

        pszWKT = msProjectionObjToWKT( &(map->projection) );
        if( pszWKT != NULL )
        {
            GDALSetProjection( hMemDS, pszWKT );
            CPLFree( pszWKT );
        }
    }

/* -------------------------------------------------------------------- */
/*      Create a disk image in the selected output format from the      */
/*      memory image.                                                   */
/* -------------------------------------------------------------------- */
    hOutputDriver = GDALGetDriverByName( format->driver+5 );
    if( hOutputDriver == NULL )
    {
        GDALClose( hMemDS );
        msSetError( MS_MISCERR, "Failed to find %s driver.",
                    "msSaveImageGDAL()", format->driver+5 );
        return MS_FAILURE;
    }

    papszOptions = (char**)calloc(sizeof(char *),(format->numformatoptions+1));
    memcpy( papszOptions, format->formatoptions, 
            sizeof(char *) * format->numformatoptions );
   
    hOutputDS = GDALCreateCopy( hOutputDriver, filename, hMemDS, FALSE, 
                                papszOptions, NULL, NULL );

    free( papszOptions );

    if( hOutputDS == NULL )
    {
        GDALClose( hMemDS );
        msSetError( MS_MISCERR, "Failed to find %s driver.",
                    "msSaveImageGDAL()", format->driver+5 );
        return MS_FAILURE;
    }

    /* closing the memory DS also frees all associated resources. */
    GDALClose( hMemDS );

    GDALClose( hOutputDS );

/* -------------------------------------------------------------------- */
/*      Is this supposed to be a temporary file?  If so, stream to      */
/*      stdout and delete the file.                                     */
/* -------------------------------------------------------------------- */
    if( bFileIsTemporary )
    {
        FILE *fp; 
        unsigned char block[4000];
        int bytes_read;

        fp = fopen( filename, "rb" );
        if( fp == NULL )
        {
            msSetError( MS_MISCERR, 
                        "Failed to open %s for streaming to stdout.",
                        "msSaveImageGDAL()", filename );
            return MS_FAILURE;
        }

        while( (bytes_read = fread(block, 1, sizeof(block), fp)) > 0 )
            fwrite( block, 1, bytes_read, stdout );

        fclose( fp );

        unlink( filename );
        free( filename );
    }
    
    return MS_SUCCESS;
}

/************************************************************************/
/*                       msInitGDALOutputFormat()                       */
/************************************************************************/

int msInitDefaultGDALOutputFormat( outputFormatObj *format )

{
    GDALDriverH hDriver; 

    InitializeGDAL();

/* -------------------------------------------------------------------- */
/*      check that this driver exists.                                  */
/* -------------------------------------------------------------------- */
    hDriver = GDALGetDriverByName( format->driver+5 );
    if( hDriver == NULL )
    {
        msSetError( MS_MISCERR, "No GDAL driver named `%s' available.", 
                    "msInitGDALOutputFormat()", format->driver+5 );
        return MS_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Initialize the object.                                          */
/* -------------------------------------------------------------------- */
    format->imagemode = MS_IMAGEMODE_RGB;
    format->renderer = MS_RENDER_WITH_GD;

    // Eventually we should have a way of deriving the mime type and extension
    // from the driver.

    if( strcasecmp(format->driver,"GDAL/GTiff") )
    {
        format->mimetype = strdup("image/tiff");
        format->extension = strdup("tif");
    }
    else if( strcasecmp(format->driver,"GDAL/HFA") )
    {
        format->extension = strdup("img");
    }
    
    return MS_SUCCESS;
}

/************************************************************************/
/*                        msProjectionObjToWKT()                        */
/*                                                                      */
/*      We stick to the C API for OGRSpatialReference object access     */
/*      to allow MapServer+GDAL to be built without C++                 */
/*      complications.                                                  */
/*                                                                      */
/*      Note that this function will return NULL on failure, and the    */
/*      returned string must be freed with CPLFree(), not msFree().     */
/************************************************************************/

char *msProjectionObjToWKT( projectionObj *projection )

{
    OGRSpatialReferenceH hSRS;
    char *pszWKT=NULL, *pszProj4;
    int  nLength = 0, i;
    OGRErr eErr;

    if( projection->proj == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Form arguments into a full Proj.4 definition string.            */
/* -------------------------------------------------------------------- */
    for( i = 0; i < projection->numargs; i++ )
        nLength += strlen(projection->args[i]) + 2;

    pszProj4 = (char *) CPLMalloc(nLength+2);
    pszProj4[0] = '\0';

    for( i = 0; i < projection->numargs; i++ )
    {
        strcat( pszProj4, "+" );
        strcat( pszProj4, projection->args[i] );
        strcat( pszProj4, " " );
    }

/* -------------------------------------------------------------------- */
/*      Ingest the string into OGRSpatialReference.                     */
/* -------------------------------------------------------------------- */
    hSRS = OSRNewSpatialReference( NULL );
    eErr =  OSRImportFromProj4( hSRS, pszProj4 );
    CPLFree( pszProj4 );

/* -------------------------------------------------------------------- */
/*      Export as a WKT string.                                         */
/* -------------------------------------------------------------------- */
    if( eErr == OGRERR_NONE )
        eErr = OSRExportToWkt( hSRS, &pszWKT );

    OSRDestroySpatialReference( hSRS );
    
    return pszWKT;
}

#endif /* def USE_GDAL */

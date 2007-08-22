/******************************************************************************
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
 * Revision 1.30  2005/10/31 15:58:11  frank
 * ensure TLOCK_GDAL released on error exits
 *
 * Revision 1.29  2005/09/13 23:51:04  frank
 * Pop all outstanding error handler in msGDALCleanup()
 *
 * Revision 1.28  2005/02/18 03:06:45  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.27  2005/01/15 20:03:14  frank
 * Fixed byte swapping error for 24bit images on bigendian systems.
 *
 * Revision 1.26  2004/11/04 21:06:09  frank
 * centralize 'stdout binary mode setting' for win32, add for gdal output
 *
 * Revision 1.25  2004/10/21 04:30:56  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 * Revision 1.24  2004/10/01 19:16:04  frank
 * use msIO_fwrite to write file to stdout
 *
 * Revision 1.23  2004/07/13 20:39:36  dan
 * Made msTmpFile() more robust using msBuildPath() to return absolute paths (bug 771)
 *
 * Revision 1.22  2004/05/25 15:56:26  frank
 * added rotation/geotransform support
 *
 * Revision 1.21  2004/05/18 14:42:51  frank
 * Removed unused variable.
 *
 * Revision 1.20  2004/05/06 21:06:08  frank
 * temporary filenames should not be altered with msBuildPath()
 *
 * Revision 1.19  2004/03/15 05:52:07  frank
 * only call CPLFreeConfig() with recent GDALs
 *
 * Revision 1.18  2004/03/13 20:11:42  frank
 * Added CONFIG option support on the mapObj
 *
 * Revision 1.17  2004/03/05 22:56:55  frank
 * improve GDAL cleanup
 *
 * Revision 1.16  2004/03/05 05:57:04  frank
 * support multi-band rawmode output
 *
 * Revision 1.15  2004/03/04 20:08:28  frank
 * added IMAGEMODE_BYTE (raw mode)
 *
 * Revision 1.14  2003/04/10 17:24:02  frank
 * Use format extension for file created in the temp directory.  This is mostly
 * intended to work around the fact that some GDAL drivers (such as ECW) will
 * replace the supplied extension with their own preferred extension.  If we
 * don't happen to use the preferred extension we will have a hard time opening
 * the file that is created.
 *
 * Revision 1.13  2002/12/16 18:59:58  assefa
 * Make sure map object is not null.
 *
 * Revision 1.12  2002/11/19 17:56:18  frank
 * don't call GDALDestroyDriverManager() if GDAL is old
 *
 * Revision 1.11  2002/11/19 05:47:29  frank
 * centralize GDAL initialization and cleanup
 *
 * Revision 1.10  2002/11/16 21:39:45  frank
 * fix support for generating colormaped files with transparency
 *
 * Revision 1.9  2002/09/20 03:44:06  sdlime
 * Swapped map_path for mappath for consistency.
 *
 * Revision 1.8  2002/09/17 13:08:28  julien
 * Remove all chdir() function and replace them with the new msBuildPath function.
 * This have been done to make MapServer thread safe. (Bug 152)
 *
 * Revision 1.7  2002/07/11 17:09:45  frank
 * corrected error reporting
 *
 * Revision 1.6  2002/06/21 18:34:31  frank
 * added support for INT16 and FLOAT imagemodes
 *
 * Revision 1.5  2002/06/13 19:55:57  frank
 * improve temporary file handling
 *
 * Revision 1.4  2002/06/12 21:20:32  frank
 * added automatic mimetype and extension setting
 *
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
#include "mapthread.h"

MS_CVSID("$Id$")

#ifdef USE_GDAL

#include "gdal.h"
#include "ogr_srs_api.h"
#include "cpl_conv.h"
#include "cpl_string.h"

static char *msProjectionObjToWKT( projectionObj *proj );
static int    bGDALInitialized = 0;

/************************************************************************/
/*                          msGDALInitialize()                          */
/************************************************************************/

void msGDALInitialize( void )

{
    if( !bGDALInitialized )
    {
        msAcquireLock( TLOCK_GDAL );

        GDALAllRegister();
        CPLPushErrorHandler( CPLQuietErrorHandler );
        msReleaseLock( TLOCK_GDAL );

        bGDALInitialized = 1;
    }
}

/************************************************************************/
/*                           msGDALCleanup()                            */
/************************************************************************/

void msGDALCleanup( void )

{
    if( bGDALInitialized )
    {
        int iRepeat = 5;

        msAcquireLock( TLOCK_GDAL );

        while( iRepeat-- )
            CPLPopErrorHandler();

#if GDAL_RELEASE_DATE > 20021001
        GDALDestroyDriverManager();
#endif

#if GDAL_RELEASE_DATE > 20040101
        CPLFreeConfig();
#endif

        msReleaseLock( TLOCK_GDAL );

        bGDALInitialized = 0;
    }
}


/************************************************************************/
/*                          msSaveImageGDAL()                           */
/************************************************************************/

int msSaveImageGDAL( mapObj *map, imageObj *image, char *filename )

{
    int  bFileIsTemporary = MS_FALSE;
    GDALDatasetH hMemDS, hOutputDS;
    GDALDriverH  hMemDriver, hOutputDriver;
    int          nBands = 1;
    int          iLine;
    GByte       *pabyAlphaLine = NULL;
    char        **papszOptions = NULL;
    outputFormatObj *format = image->format;
    GDALDataType eDataType = GDT_Byte;

    msGDALInitialize();

/* -------------------------------------------------------------------- */
/*      We will need to write the output to a temporary file and        */
/*      then stream to stdout if no filename is passed.                 */
/* -------------------------------------------------------------------- */
    if( filename == NULL )
    {
        const char *pszExtension = format->extension;
        if( pszExtension == NULL )
            pszExtension = "img.tmp";

        if( map != NULL && map->web.imagepath != NULL )
            filename = msTmpFile(map->mappath,map->web.imagepath,pszExtension);
        else
        {
#ifndef _WIN32
            filename = msTmpFile(NULL, "/tmp/", pszExtension );
#else
            filename = msTmpFile(NULL, "C:\\", pszExtension );
#endif
        }
            
        bFileIsTemporary = MS_TRUE;
    }
    
/* -------------------------------------------------------------------- */
/*      Establish the characteristics of our memory, and final          */
/*      dataset.                                                        */
/* -------------------------------------------------------------------- */
    if( format->imagemode == MS_IMAGEMODE_RGB )
    {
        nBands = 3;
        assert( gdImageTrueColor( image->img.gd ) );
    }
    else if( format->imagemode == MS_IMAGEMODE_RGBA )
    {
        pabyAlphaLine = (GByte *) calloc(image->width,1);
        nBands = 4;
        assert( gdImageTrueColor( image->img.gd ) );
    }
    else if( format->imagemode == MS_IMAGEMODE_INT16 )
    {
        nBands = format->bands;
        eDataType = GDT_Int16;
    }
    else if( format->imagemode == MS_IMAGEMODE_FLOAT32 )
    {
        nBands = format->bands;
        eDataType = GDT_Float32;
    }
    else if( format->imagemode == MS_IMAGEMODE_BYTE )
    {
        nBands = format->bands;
        eDataType = GDT_Byte;
    }
    else
    {
        assert( format->imagemode == MS_IMAGEMODE_PC256
                && !gdImageTrueColor( image->img.gd ) );
    }

/* -------------------------------------------------------------------- */
/*      Create a memory dataset which we can use as a source for a      */
/*      CreateCopy().                                                   */
/* -------------------------------------------------------------------- */
    msAcquireLock( TLOCK_GDAL );
    hMemDriver = GDALGetDriverByName( "MEM" );
    if( hMemDriver == NULL )
    {
        msReleaseLock( TLOCK_GDAL );
        msSetError( MS_MISCERR, "Failed to find MEM driver.",
                    "msSaveImageGDAL()" );
        return MS_FAILURE;
    }
   
    hMemDS = GDALCreate( hMemDriver, "msSaveImageGDAL_temp", 
                         image->width, image->height, nBands, 
                         eDataType, NULL );
    if( hMemDS == NULL )
    {
        msReleaseLock( TLOCK_GDAL );
        msSetError( MS_MISCERR, "Failed to create MEM dataset.",
                    "msSaveImageGDAL()" );
        return MS_FAILURE;
    }
    
/* -------------------------------------------------------------------- */
/*      Copy the gd image into the memory dataset.                      */
/* -------------------------------------------------------------------- */
    for( iLine = 0; iLine < image->height; iLine++ )
    {
        int iBand;

        for( iBand = 0; iBand < nBands; iBand++ )
        {
            GDALRasterBandH hBand = GDALGetRasterBand( hMemDS, iBand+1 );

            if( format->imagemode == MS_IMAGEMODE_INT16 )
            {
                GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1, 
                              image->img.raw_16bit + iLine * image->width
                              + iBand * image->width * image->height,
                              image->width, 1, GDT_Int16, 2, 0 );
                
            }
            else if( format->imagemode == MS_IMAGEMODE_FLOAT32 )
            {
                GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1, 
                              image->img.raw_float + iLine * image->width
                              + iBand * image->width * image->height,
                              image->width, 1, GDT_Float32, 4, 0 );
            }
            else if( format->imagemode == MS_IMAGEMODE_BYTE )
            {
                GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1, 
                              image->img.raw_byte + iLine * image->width
                              + iBand * image->width * image->height,
                              image->width, 1, GDT_Byte, 1, 0 );
            }
#if GD2_VERS > 1
            else if( nBands > 1 && iBand < 3 )
            {
                GByte *pabyData;
#ifdef CPL_MSB

                pabyData = ((GByte *) image->img.gd->tpixels[iLine])+iBand+1;
#else
                pabyData = ((GByte *) image->img.gd->tpixels[iLine])+(2-iBand);
#endif
                GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1, 
                              pabyData, image->width, 1, GDT_Byte, 4, 0 );
            }
            else if( nBands > 1 && iBand == 3 ) /* Alpha band */
            {
                int x;
#ifdef CPL_MSB
                GByte *pabySrc = ((GByte *) image->img.gd->tpixels[iLine]);
#else
                GByte *pabySrc = ((GByte *) image->img.gd->tpixels[iLine])+3;
#endif

                for( x = 0; x < image->width; x++ )
                {
                    if( *pabySrc == 127 )
                        pabyAlphaLine[x] = 0;
                    else
                        pabyAlphaLine[x] = 255 - 2 * *pabySrc;

                    pabySrc += 4;
                }

                GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1, 
                              pabyAlphaLine, image->width, 1, GDT_Byte, 1, 0 );
            }
#endif
            else
            {
                GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1, 
                              image->img.gd->pixels[iLine], 
                              image->width, 1, GDT_Byte, 0, 0 );
            }
        }
    }

    if( pabyAlphaLine != NULL )
        free( pabyAlphaLine );

/* -------------------------------------------------------------------- */
/*      Attach the palette if appropriate.                              */
/* -------------------------------------------------------------------- */
    if( format->imagemode == MS_IMAGEMODE_PC256 )
    {
        GDALColorEntry sEntry;
        int  iColor;
        GDALColorTableH hCT;

        hCT = GDALCreateColorTable( GPI_RGB );

        for( iColor = 0; iColor < image->img.gd->colorsTotal; iColor++ )
        {
            sEntry.c1 = image->img.gd->red[iColor];
            sEntry.c2 = image->img.gd->green[iColor];
            sEntry.c3 = image->img.gd->blue[iColor];

            if( iColor == gdImageGetTransparent( image->img.gd ) )
                sEntry.c4 = 0;
            else if( iColor == 0 
                     && gdImageGetTransparent( image->img.gd ) == -1 
                     && format->transparent )
                sEntry.c4 = 0;
            else
                sEntry.c4 = 255;

            GDALSetColorEntry( hCT, iColor, &sEntry );
        }
        
        GDALSetRasterColorTable( GDALGetRasterBand( hMemDS, 1 ), hCT );

        GDALDestroyColorTable( hCT );
    }

#if GDAL_VERSION_NUM > 1170
    else if( format->imagemode == MS_IMAGEMODE_RGB )
    {
        GDALSetRasterColorInterpretation( 
            GDALGetRasterBand( hMemDS, 1 ), GCI_RedBand );
        GDALSetRasterColorInterpretation( 
            GDALGetRasterBand( hMemDS, 2 ), GCI_GreenBand );
        GDALSetRasterColorInterpretation( 
            GDALGetRasterBand( hMemDS, 3 ), GCI_BlueBand );
    }
    else if( format->imagemode == MS_IMAGEMODE_RGBA )
    {
        GDALSetRasterColorInterpretation( 
            GDALGetRasterBand( hMemDS, 1 ), GCI_RedBand );
        GDALSetRasterColorInterpretation( 
            GDALGetRasterBand( hMemDS, 2 ), GCI_GreenBand );
        GDALSetRasterColorInterpretation( 
            GDALGetRasterBand( hMemDS, 3 ), GCI_BlueBand );
        GDALSetRasterColorInterpretation( 
            GDALGetRasterBand( hMemDS, 4 ), GCI_AlphaBand );
    }
#endif

/* -------------------------------------------------------------------- */
/*      Assign the projection and coordinate system to the memory       */
/*      dataset.                                                        */
/* -------------------------------------------------------------------- */

    if( map != NULL )
    {
        char *pszWKT;

        GDALSetGeoTransform( hMemDS, map->gt.geotransform );

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
        msReleaseLock( TLOCK_GDAL );
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
        msReleaseLock( TLOCK_GDAL );
        msSetError( MS_MISCERR, "Failed to create output %s file.\n%s",
                    "msSaveImageGDAL()", format->driver+5, 
                    CPLGetLastErrorMsg() );
        return MS_FAILURE;
    }

    /* closing the memory DS also frees all associated resources. */
    GDALClose( hMemDS );

    GDALClose( hOutputDS );

    msReleaseLock( TLOCK_GDAL );

/* -------------------------------------------------------------------- */
/*      Is this supposed to be a temporary file?  If so, stream to      */
/*      stdout and delete the file.                                     */
/* -------------------------------------------------------------------- */
    if( bFileIsTemporary )
    {
        FILE *fp; 
        unsigned char block[4000];
        int bytes_read;

        if( msIO_needBinaryStdout() == MS_FAILURE )
            return MS_FAILURE;

        fp = fopen( filename, "rb" );
        if( fp == NULL )
        {
            msSetError( MS_MISCERR, 
                        "Failed to open %s for streaming to stdout.",
                        "msSaveImageGDAL()", filename );
            return MS_FAILURE;
        }

        while( (bytes_read = fread(block, 1, sizeof(block), fp)) > 0 )
            msIO_fwrite( block, 1, bytes_read, stdout );

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

    msGDALInitialize();

/* -------------------------------------------------------------------- */
/*      check that this driver exists.  Note visiting drivers should    */
/*      be pretty threadsafe so don't bother acquiring the GDAL         */
/*      lock.                                                           */
/* -------------------------------------------------------------------- */
    hDriver = GDALGetDriverByName( format->driver+5 );
    if( hDriver == NULL )
    {
        msSetError( MS_MISCERR, "No GDAL driver named `%s' available.", 
                    "msInitGDALOutputFormat()", format->driver+5 );
        return MS_FAILURE;
    }

#ifdef GDAL_DCAP_CREATE
    if( GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) == NULL 
        && GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATECOPY, NULL ) == NULL )
    {
        msSetError( MS_MISCERR, "GDAL `%s' driver does not support output.", 
                    "msInitGDALOutputFormat()", format->driver+5 );
        return MS_FAILURE;
    }
#endif

/* -------------------------------------------------------------------- */
/*      Initialize the object.                                          */
/* -------------------------------------------------------------------- */
    format->imagemode = MS_IMAGEMODE_RGB;
    format->renderer = MS_RENDER_WITH_GD;

    /* Eventually we should have a way of deriving the mime type and extension */
    /* from the driver. */

#ifdef GDAL_DMD_MIMETYPE 
    if( GDALGetMetadataItem( hDriver, GDAL_DMD_MIMETYPE, NULL ) != NULL )
        format->mimetype = 
            strdup(GDALGetMetadataItem(hDriver,GDAL_DMD_MIMETYPE,NULL));
    if( GDALGetMetadataItem( hDriver, GDAL_DMD_EXTENSION, NULL ) != NULL )
        format->extension = 
            strdup(GDALGetMetadataItem(hDriver,GDAL_DMD_EXTENSION,NULL));

#else
    if( strcasecmp(format->driver,"GDAL/GTiff") )
    {
        format->mimetype = strdup("image/tiff");
        format->extension = strdup("tif");
    }
#endif
    
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

#else

void msGDALInitialize( void ) {}
void msGDALCleanup(void) {}


#endif /* def USE_GDAL */

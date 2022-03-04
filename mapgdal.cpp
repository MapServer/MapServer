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
#include "mapthread.h"
#include "mapgdal.h"
#include <assert.h>


#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_srs_api.h"

#include "gdal.h"

static int    bGDALInitialized = 0;

/************************************************************************/
/*                          msGDALInitialize()                          */
/************************************************************************/

void msGDALInitialize( void )

{
  if( !bGDALInitialized ) {
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
  if( bGDALInitialized ) {
    int iRepeat = 5;

      /*
      ** Cleanup any unreferenced but open datasets as will tend
      ** to exist due to deferred close requests.  We are careful
      ** to only close one file at a time before refecting the
      ** list as closing some datasets may cause others to be
      ** closed (subdatasets in a VRT for instance).
      */
    GDALDatasetH *pahDSList = NULL;
    int nDSCount = 0;
    int bDidSomething;

    msAcquireLock( TLOCK_GDAL );

    do {
        int i;
        GDALGetOpenDatasets( &pahDSList, &nDSCount );
        bDidSomething = FALSE;
        for( i = 0; i < nDSCount && !bDidSomething; i++ ) {
          if( GDALReferenceDataset( pahDSList[i] ) == 1 ) {
            GDALClose( pahDSList[i] );
            bDidSomething = TRUE;
          } else
            GDALDereferenceDataset( pahDSList[i] );
        }
    } while( bDidSomething );

    while( iRepeat-- )
      CPLPopErrorHandler();

    msReleaseLock( TLOCK_GDAL );

    bGDALInitialized = 0;
  }
}

/************************************************************************/
/*                          msCleanVSIDir()                             */
/*                                                                      */
/*      For the temporary /vsimem/msout directory we need to be sure    */
/*      things are clean before we start, and after we are done.        */
/************************************************************************/

void msCleanVSIDir( const char *pszDir )

{
  char **papszFiles = CPLReadDir( pszDir );
  int i, nFileCount = CSLCount( papszFiles );

  for( i = 0; i < nFileCount; i++ ) {
    if( strcasecmp(papszFiles[i],".") == 0
        || strcasecmp(papszFiles[i],"..") == 0 )
      continue;

    VSIUnlink( CPLFormFilename(pszDir, papszFiles[i], NULL) );
  }

  CSLDestroy( papszFiles );
}

/************************************************************************/
/*                          msSaveImageGDAL()                           */
/************************************************************************/

int msSaveImageGDAL( mapObj *map, imageObj *image, const char *filenameIn )

{
  int  bFileIsTemporary = MS_FALSE;
  GDALDatasetH hMemDS, hOutputDS;
  GDALDriverH  hMemDriver, hOutputDriver;
  int          nBands = 1;
  int          iLine;
  outputFormatObj *format = image->format;
  rasterBufferObj rb;
  GDALDataType eDataType = GDT_Byte;
  int bUseXmp = MS_FALSE;
  const char   *filename = NULL;
  char         *filenameToFree = NULL;
  const char   *gdal_driver_shortname = format->driver+5;

  msGDALInitialize();
  memset(&rb,0,sizeof(rasterBufferObj));

#ifdef USE_EXEMPI
  if( map != NULL ) {
    bUseXmp = msXmpPresent(map);
  }
#endif


  /* -------------------------------------------------------------------- */
  /*      Identify the proposed output driver.                            */
  /* -------------------------------------------------------------------- */
  msAcquireLock( TLOCK_GDAL );
  hOutputDriver = GDALGetDriverByName( gdal_driver_shortname );
  if( hOutputDriver == NULL ) {
    msReleaseLock( TLOCK_GDAL );
    msSetError( MS_MISCERR, "Failed to find %s driver.",
                "msSaveImageGDAL()", gdal_driver_shortname );
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      We will need to write the output to a temporary file and        */
  /*      then stream to stdout if no filename is passed.  If the         */
  /*      driver supports virtualio then we hold the temporary file in    */
  /*      memory, otherwise we try to put it in a reasonable temporary    */
  /*      file location.                                                  */
  /* -------------------------------------------------------------------- */
  if( filenameIn == NULL ) {
    const char *pszExtension = format->extension;
    if( pszExtension == NULL )
      pszExtension = "img.tmp";

    if( bUseXmp == MS_FALSE &&
        msGDALDriverSupportsVirtualIOOutput(hOutputDriver) ) {
      msCleanVSIDir( "/vsimem/msout" );
      filenameToFree = msTmpFile(map, NULL, "/vsimem/msout/", pszExtension );
    }

    if( filenameToFree == NULL && map != NULL)
      filenameToFree = msTmpFile(map, map->mappath,NULL,pszExtension);
    else if( filenameToFree == NULL ) {
      filenameToFree = msTmpFile(map, NULL, NULL, pszExtension );
    }
    filename = filenameToFree;

    bFileIsTemporary = MS_TRUE;
  }
  else {
    filename = filenameIn;
  }

  /* -------------------------------------------------------------------- */
  /*      Establish the characteristics of our memory, and final          */
  /*      dataset.                                                        */
  /* -------------------------------------------------------------------- */

  if( format->imagemode == MS_IMAGEMODE_RGB ) {
    nBands = 3;
    assert( MS_RENDERER_PLUGIN(format) && format->vtable->supports_pixel_buffer );
    if(MS_UNLIKELY(MS_FAILURE == format->vtable->getRasterBufferHandle(image,&rb))) {
      msReleaseLock( TLOCK_GDAL );
      return MS_FAILURE;
    }
  } else if( format->imagemode == MS_IMAGEMODE_RGBA ) {
    nBands = 4;
    assert( MS_RENDERER_PLUGIN(format) && format->vtable->supports_pixel_buffer );
    if(MS_UNLIKELY(MS_FAILURE == format->vtable->getRasterBufferHandle(image,&rb))) {
      msReleaseLock( TLOCK_GDAL );
      return MS_FAILURE;
    }
  } else if( format->imagemode == MS_IMAGEMODE_INT16 ) {
    nBands = format->bands;
    eDataType = GDT_Int16;
  } else if( format->imagemode == MS_IMAGEMODE_FLOAT32 ) {
    nBands = format->bands;
    eDataType = GDT_Float32;
  } else if( format->imagemode == MS_IMAGEMODE_BYTE ) {
    nBands = format->bands;
    eDataType = GDT_Byte;
  } else {
    msReleaseLock( TLOCK_GDAL );
    msSetError( MS_MEMERR, "Unknown format. This is a bug.", "msSaveImageGDAL()");
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      Create a memory dataset which we can use as a source for a      */
  /*      CreateCopy().                                                   */
  /* -------------------------------------------------------------------- */
  hMemDriver = GDALGetDriverByName( "MEM" );
  if( hMemDriver == NULL ) {
    msReleaseLock( TLOCK_GDAL );
    msSetError( MS_MISCERR, "Failed to find MEM driver.",
                "msSaveImageGDAL()" );
    return MS_FAILURE;
  }

  hMemDS = GDALCreate( hMemDriver, "msSaveImageGDAL_temp",
                       image->width, image->height, nBands,
                       eDataType, NULL );
  if( hMemDS == NULL ) {
    msReleaseLock( TLOCK_GDAL );
    msSetError( MS_MISCERR, "Failed to create MEM dataset.",
                "msSaveImageGDAL()" );
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      Copy the gd image into the memory dataset.                      */
  /* -------------------------------------------------------------------- */
  for( iLine = 0; iLine < image->height; iLine++ ) {
    int iBand;

    for( iBand = 0; iBand < nBands; iBand++ ) {
      CPLErr eErr;
      GDALRasterBandH hBand = GDALGetRasterBand( hMemDS, iBand+1 );

      if( format->imagemode == MS_IMAGEMODE_INT16 ) {
        eErr = GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1,
                      image->img.raw_16bit + iLine * image->width
                      + iBand * image->width * image->height,
                      image->width, 1, GDT_Int16, 2, 0 );

      } else if( format->imagemode == MS_IMAGEMODE_FLOAT32 ) {
        eErr = GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1,
                      image->img.raw_float + iLine * image->width
                      + iBand * image->width * image->height,
                      image->width, 1, GDT_Float32, 4, 0 );
      } else if( format->imagemode == MS_IMAGEMODE_BYTE ) {
        eErr = GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1,
                      image->img.raw_byte + iLine * image->width
                      + iBand * image->width * image->height,
                      image->width, 1, GDT_Byte, 1, 0 );
      } else {
        GByte *pabyData;
        unsigned char *pixptr = NULL;
        assert( rb.type == MS_BUFFER_BYTE_RGBA );
        switch(iBand) {
          case 0:
            pixptr = rb.data.rgba.r;
            break;
          case 1:
            pixptr = rb.data.rgba.g;
            break;
          case 2:
            pixptr = rb.data.rgba.b;
            break;
          case 3:
            pixptr = rb.data.rgba.a;
            break;
        }
        assert(pixptr);
        if( pixptr == NULL ) {
          msReleaseLock( TLOCK_GDAL );
          msSetError( MS_MISCERR, "Missing RGB or A buffer.\n",
                      "msSaveImageGDAL()" );
          GDALClose(hMemDS);
          return MS_FAILURE;
        }

        pabyData = (GByte *)(pixptr + iLine*rb.data.rgba.row_step);

        if( rb.data.rgba.a == NULL || iBand == 3 ) {
          eErr = GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1,
                        pabyData, image->width, 1, GDT_Byte,
                        rb.data.rgba.pixel_step, 0 );
        } else { /* We need to un-pre-multiple RGB by alpha. */
          GByte *pabyUPM = (GByte*) malloc(image->width);
          GByte *pabyAlpha= (GByte *)(rb.data.rgba.a + iLine*rb.data.rgba.row_step);
          int i;

          for( i = 0; i < image->width; i++ ) {
            int alpha = pabyAlpha[i*rb.data.rgba.pixel_step];

            if( alpha == 0 )
              pabyUPM[i] = 0;
            else {
              int result = (pabyData[i*rb.data.rgba.pixel_step] * 255) / alpha;

              if( result > 255 )
                result = 255;

              pabyUPM[i] = result;
            }
          }

          eErr = GDALRasterIO( hBand, GF_Write, 0, iLine, image->width, 1,
                        pabyUPM, image->width, 1, GDT_Byte, 1, 0 );
          free( pabyUPM );
        }
      }
      if( eErr != CE_None ) {
          msReleaseLock( TLOCK_GDAL );
          msSetError( MS_MISCERR, "GDALRasterIO() failed.\n",
                      "msSaveImageGDAL()" );
          GDALClose(hMemDS);
          return MS_FAILURE;
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Attach the palette if appropriate.                              */
  /* -------------------------------------------------------------------- */
    if( format->imagemode == MS_IMAGEMODE_RGB ) {
      GDALSetRasterColorInterpretation(
        GDALGetRasterBand( hMemDS, 1 ), GCI_RedBand );
      GDALSetRasterColorInterpretation(
        GDALGetRasterBand( hMemDS, 2 ), GCI_GreenBand );
      GDALSetRasterColorInterpretation(
        GDALGetRasterBand( hMemDS, 3 ), GCI_BlueBand );
    } else if( format->imagemode == MS_IMAGEMODE_RGBA ) {
      GDALSetRasterColorInterpretation(
        GDALGetRasterBand( hMemDS, 1 ), GCI_RedBand );
      GDALSetRasterColorInterpretation(
        GDALGetRasterBand( hMemDS, 2 ), GCI_GreenBand );
      GDALSetRasterColorInterpretation(
        GDALGetRasterBand( hMemDS, 3 ), GCI_BlueBand );
      GDALSetRasterColorInterpretation(
        GDALGetRasterBand( hMemDS, 4 ), GCI_AlphaBand );
    }

  /* -------------------------------------------------------------------- */
  /*      Assign the projection and coordinate system to the memory       */
  /*      dataset.                                                        */
  /* -------------------------------------------------------------------- */

  if( map != NULL ) {
    char *pszWKT;

    GDALSetGeoTransform( hMemDS, map->gt.geotransform );

    pszWKT = msProjectionObj2OGCWKT( &(map->projection) );
    if( pszWKT != NULL ) {
      GDALSetProjection( hMemDS, pszWKT );
      msFree( pszWKT );
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Possibly assign a nodata value.                                 */
  /* -------------------------------------------------------------------- */
  const char *nullvalue = msGetOutputFormatOption(format,"NULLVALUE",NULL);
  if( nullvalue != NULL ) {
    const double dfNullValue = atof(nullvalue);
    for( int iBand = 0; iBand < nBands; iBand++ ) {
      GDALRasterBandH hBand = GDALGetRasterBand( hMemDS, iBand+1 );
      GDALSetRasterNoDataValue( hBand, dfNullValue );
    }
  }

  /* -------------------------------------------------------------------- */
  /*  Try to save resolution in the output file.                          */
  /* -------------------------------------------------------------------- */
  if( image->resolution > 0 ) {
    char res[30];

    sprintf( res, "%lf", image->resolution );
    GDALSetMetadataItem( hMemDS, "TIFFTAG_XRESOLUTION", res, NULL );
    GDALSetMetadataItem( hMemDS, "TIFFTAG_YRESOLUTION", res, NULL );
    GDALSetMetadataItem( hMemDS, "TIFFTAG_RESOLUTIONUNIT", "2", NULL );
  }

  /* -------------------------------------------------------------------- */
  /*      Separate creation options from metadata items.                  */
  /* -------------------------------------------------------------------- */
  std::vector<char*> apszCreationOptions;
  for( int i = 0; i < format->numformatoptions; i++ )
  {
    char* option = format->formatoptions[i];

    // MDI stands for MetaDataItem
    if( STARTS_WITH(option, "mdi_") )
    {
      const char* option_without_band = option + strlen("mdi_");
      GDALMajorObjectH hObject = (GDALMajorObjectH)hMemDS;
      if( STARTS_WITH(option_without_band, "BAND_") )
      {
        int nBandNumber = atoi(option_without_band + strlen("BAND_"));
        if( nBandNumber > 0 && nBandNumber <= nBands )
        {
          const char* pszAfterBand = strchr(option_without_band + strlen("BAND_"), '_');
          if( pszAfterBand != NULL )
          {
              hObject = (GDALMajorObjectH)GDALGetRasterBand(hMemDS, nBandNumber);
              option_without_band = pszAfterBand + 1;
          }
        }
        else {
          msDebug("Invalid band number %d in metadata item option %s", nBandNumber, option);
        }
      }
      if( hObject ) {
        std::string osDomain(option_without_band);
        size_t nUnderscorePos = osDomain.find('_');
        if( nUnderscorePos != std::string::npos ) {
          std::string osKeyValue = osDomain.substr(nUnderscorePos + 1);
          osDomain.resize(nUnderscorePos);
          if( osDomain == "default" )
              osDomain.clear();
          size_t nEqualPos = osKeyValue.find('=');
          if( nEqualPos != std::string::npos )
          {
              GDALSetMetadataItem(hObject,
                                  osKeyValue.substr(0, nEqualPos).c_str(),
                                  osKeyValue.substr(nEqualPos + 1).c_str(),
                                  osDomain.c_str());
          }
        }
        else {
          msDebug("Invalid format in metadata item option %s", option);
        }
      }
    }
    else
    {
      apszCreationOptions.emplace_back(option);
    }
  }
  apszCreationOptions.emplace_back(nullptr);

  /* -------------------------------------------------------------------- */
  /*      Create a disk image in the selected output format from the      */
  /*      memory image.                                                   */
  /* -------------------------------------------------------------------- */
  hOutputDS = GDALCreateCopy( hOutputDriver, filename, hMemDS, FALSE,
                              &apszCreationOptions[0], NULL, NULL );


  if( hOutputDS == NULL ) {
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
  /*      Are we writing license info into the image?                     */
  /*      If so, add it to the temp file on disk now.                     */
  /* -------------------------------------------------------------------- */
#ifdef USE_EXEMPI
  if ( bUseXmp == MS_TRUE ) {
    if( msXmpWrite(map, filename) == MS_FAILURE ) {
      /* Something bad happened. */
      msSetError( MS_MISCERR, "XMP write to %s failed.\n",
                  "msSaveImageGDAL()", filename);
      return MS_FAILURE;
    }
  }
#endif

  /* -------------------------------------------------------------------- */
  /*      Is this supposed to be a temporary file?  If so, stream to      */
  /*      stdout and delete the file.                                     */
  /* -------------------------------------------------------------------- */
  if( bFileIsTemporary ) {
    FILE *fp;
    unsigned char block[4000];
    int bytes_read;

    if( msIO_needBinaryStdout() == MS_FAILURE )
      return MS_FAILURE;

    /* We aren't sure how far back GDAL exports the VSI*L API, so
       we only use it if we suspect we need it.  But we do need it if
       holding temporary file in memory. */
    fp = VSIFOpenL( filename, "rb" );
    if( fp == NULL ) {
      msSetError( MS_MISCERR,
                  "Failed to open %s for streaming to stdout.",
                  "msSaveImageGDAL()", filename );
      return MS_FAILURE;
    }

    while( (bytes_read = VSIFReadL(block, 1, sizeof(block), fp)) > 0 )
      msIO_fwrite( block, 1, bytes_read, stdout );

    VSIFCloseL( fp );

    VSIUnlink( filename );
    msCleanVSIDir( "/vsimem/msout" );

    msFree( filenameToFree );
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
  if( hDriver == NULL ) {
    msSetError( MS_MISCERR, "No GDAL driver named `%s' available.",
                "msInitGDALOutputFormat()", format->driver+5 );
    return MS_FAILURE;
  }

  if( GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) == NULL
      && GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATECOPY, NULL ) == NULL ) {
    msSetError( MS_MISCERR, "GDAL `%s' driver does not support output.",
                "msInitGDALOutputFormat()", format->driver+5 );
    return MS_FAILURE;
  }

  /* -------------------------------------------------------------------- */
  /*      Initialize the object.                                          */
  /* -------------------------------------------------------------------- */
  format->imagemode = MS_IMAGEMODE_RGB;
  format->renderer = MS_RENDER_WITH_AGG;

  if( GDALGetMetadataItem( hDriver, GDAL_DMD_MIMETYPE, NULL ) != NULL )
    format->mimetype =
      msStrdup(GDALGetMetadataItem(hDriver,GDAL_DMD_MIMETYPE,NULL));
  if( GDALGetMetadataItem( hDriver, GDAL_DMD_EXTENSION, NULL ) != NULL )
    format->extension =
      msStrdup(GDALGetMetadataItem(hDriver,GDAL_DMD_EXTENSION,NULL));

  return MS_SUCCESS;
}

char** msGetStringListFromHashTable(hashTableObj* table)
{
  struct hashObj *tp = NULL;
  int i;
  char** papszRet = NULL;

  if(!table) return NULL;
  if(msHashIsEmpty(table)) return NULL;

  for (i=0; i<MS_HASHSIZE; ++i) {
    if (table->items[i] != NULL) {
      for (tp=table->items[i]; tp!=NULL; tp=tp->next) {
        papszRet = CSLSetNameValue(papszRet, tp->key, tp->data);
      }
    }
  }
  return papszRet;
}


/************************************************************************/
/*                      msProjectionObj2OGCWKT()                        */
/*                                                                      */
/*      We stick to the C API for OGRSpatialReference object access     */
/*      to allow MapServer+GDAL to be built without C++                 */
/*      complications.                                                  */
/*                                                                      */
/*      Note that this function will return NULL on failure, and the    */
/*      returned string should be freed with msFree().                  */
/************************************************************************/

char *msProjectionObj2OGCWKT( projectionObj *projection )

{
  OGRSpatialReferenceH hSRS;
  char *pszWKT=NULL, *pszProj4, *pszInitEpsg=NULL;
  int  nLength = 0, i;
  OGRErr eErr;

  if( projection->proj == NULL )
    return NULL;

  hSRS = OSRNewSpatialReference( NULL );
  /* -------------------------------------------------------------------- */
  /*      Look for an EPSG-like projection argument                       */
  /* -------------------------------------------------------------------- */
  if( projection->numargs == 1 &&
        (pszInitEpsg = strcasestr(projection->args[0],"init=epsg:"))) {
     int nEpsgCode = atoi(pszInitEpsg + strlen("init=epsg:"));
     eErr = OSRImportFromEPSG(hSRS, nEpsgCode);
  } else {
    /* -------------------------------------------------------------------- */
    /*      Form arguments into a full Proj.4 definition string.            */
    /* -------------------------------------------------------------------- */
    for( i = 0; i < projection->numargs; i++ )
      nLength += strlen(projection->args[i]) + 2;

    pszProj4 = (char *) CPLMalloc(nLength+2);
    pszProj4[0] = '\0';

    for( i = 0; i < projection->numargs; i++ ) {
      strcat( pszProj4, "+" );
      strcat( pszProj4, projection->args[i] );
      strcat( pszProj4, " " );
    }

    /* -------------------------------------------------------------------- */
    /*      Ingest the string into OGRSpatialReference.                     */
    /* -------------------------------------------------------------------- */
    eErr =  OSRImportFromProj4( hSRS, pszProj4 );
    CPLFree( pszProj4 );
  }

  /* -------------------------------------------------------------------- */
  /*      Export as a WKT string.                                         */
  /* -------------------------------------------------------------------- */
  if( eErr == OGRERR_NONE )
    OSRExportToWkt( hSRS, &pszWKT );

  OSRDestroySpatialReference( hSRS );

  if( pszWKT ) {
    char *pszWKT2 = msStrdup(pszWKT);
    CPLFree( pszWKT );

    return pszWKT2;
  } else
    return NULL;
}

/************************************************************************/
/*                    msGDALDriverSupportsVirtualIOOutput()             */
/************************************************************************/

int msGDALDriverSupportsVirtualIOOutput( GDALDriverH hDriver )
{
    /* We need special testing here for the netCDF driver, since recent */
    /* GDAL versions advertize VirtualIO support, but this is only for the */
    /* read-side of the driver, not the write-side. */
    return GDALGetMetadataItem( hDriver, GDAL_DCAP_VIRTUALIO, NULL ) != NULL &&
           !EQUAL(GDALGetDescription(hDriver), "netCDF");
}

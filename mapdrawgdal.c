/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Code for drawing GDAL raster layers.  Called from 
 *           msDrawRasterLayerLow() in mapraster.c.  
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.14  2004/01/26 15:20:11  frank
 * added msGetGDALGeoTransform
 *
 * Revision 1.13  2004/01/15 19:49:23  frank
 * ensure geotransform is set on failure of GetGeotransform
 *
 * Revision 1.12  2003/10/29 21:23:03  frank
 * Added special logic to flip coordinate system y axis if the input raster image
 * is ungeoreferenced (transform is 0,1,0,0,0,1) to (0,1,0,ysize,0,-1) effectively
 * making the lower left corner the origin instead of the upper left.
 *
 * Revision 1.11  2003/10/07 14:34:56  frank
 * added (untested) nodata support for greyscale/colormapped images
 *
 * Revision 1.10  2003/09/30 13:08:57  frank
 * ensure only 128 colors are normally used in greyscale mode
 *
 * Revision 1.9  2003/02/28 20:58:41  frank
 * added preliminary support for the COLOR_MATCH_THRESHOLD
 *
 * Revision 1.8  2003/02/26 16:47:43  frank
 * dont crash on files with large color tables
 *
 * Revision 1.7  2003/02/24 21:20:54  frank
 * Added RAW_WINDOW support for use my mapresample.c
 *
 * Revision 1.6  2003/01/31 18:57:12  frank
 * fixed offsite support from pseudocolored or greyscale input: bug 274
 *
 * Revision 1.5  2003/01/21 05:55:05  frank
 * avoid core dumping when classifying 24bit image, fixes colors - bug 270
 *
 * Revision 1.4  2003/01/10 15:10:58  frank
 * fixed a few bugs in support for classified rasters
 *
 * Revision 1.3  2002/11/21 19:51:03  frank
 * avoid warnings
 *
 * Revision 1.2  2002/11/20 05:27:38  frank
 * initial 24 to 8 bit dithering
 *
 * Revision 1.1  2002/11/19 18:30:04  frank
 * New
 *
 */

#include <assert.h>
#include "map.h"
#include "mapresample.h"
#include "mapthread.h"

extern int InvGeoTransform( double *gt_in, double *gt_out );

#define MAXCOLORS 256
#define GEO_TRANS(tr,x,y)  ((tr)[0]+(tr)[1]*(x)+(tr)[2]*(y))

#if defined(USE_GDAL)

#include "gdal.h"
#include "cpl_string.h"

#if GDAL_VERSION_NUM > 1174
#  define ENABLE_DITHER
#endif

#ifdef ENABLE_DITHER
#include "gdal_alg.h"
#endif

static CPLErr 
LoadGDALImage( GDALRasterBandH hBand, int iColorIndex, 
               layerObj *layer, 
               int src_xoff, int src_yoff, int src_xsize, int src_ysize, 
               GByte *pabyBuffer, int dst_xsize, int dst_ysize );

#ifdef ENABLE_DITHER
static void Dither24to8( GByte *pabyRed, GByte *pabyGreen, GByte *pabyBlue,
                         GByte *pabyDithered, int xsize, int ysize, 
                         int bTransparent, colorObj transparentColor,
                         gdImagePtr gdImg );
#endif

/*
 * Stuff for allocating color cubes, and mapping between RGB values and
 * the corresponding color cube index.
 */

static int allocColorCube(mapObj *map, gdImagePtr img, int *panColorCube);

#define RED_LEVELS 5
#define RED_DIV 52
//#define GREEN_LEVELS 7
#define GREEN_LEVELS 5
//#define GREEN_DIV 37
#define GREEN_DIV 52
#define BLUE_LEVELS 5
#define BLUE_DIV 52

#define RGB_LEVEL_INDEX(r,g,b) ((r)*GREEN_LEVELS*BLUE_LEVELS + (g)*BLUE_LEVELS+(b))
#define RGB_INDEX(r,g,b) RGB_LEVEL_INDEX(((r)/RED_DIV),((g)/GREEN_DIV),((b)/BLUE_DIV))

/************************************************************************/
/*                       msDrawRasterLayerGDAL()                        */
/************************************************************************/

int msDrawRasterLayerGDAL(mapObj *map, layerObj *layer, imageObj *image, 
                          void *hDSVoid )

{
  int i,j, k; /* loop counters */
  int cmap[MAXCOLORS], cmap_set = FALSE;
  double adfGeoTransform[6], adfInvGeoTransform[6];
  int	dst_xoff, dst_yoff, dst_xsize, dst_ysize;
  int	src_xoff, src_yoff, src_xsize, src_ysize;
  int   anColorCube[256];
  double llx, lly, urx, ury;
  rectObj copyRect, mapRect;
  unsigned char *pabyRaw1, *pabyRaw2, *pabyRaw3, *pabyRawAlpha;
  int truecolor = FALSE;
  int red_band=0, green_band=0, blue_band=0, alpha_band=0, cmt=0;
  gdImagePtr gdImg = NULL;
  CPLErr  eErr;
  GDALDatasetH hDS = hDSVoid;
  GDALColorTableH hColorMap;
  GDALRasterBandH hBand1, hBand2, hBand3, hBandAlpha;

  memset( cmap, 0xff, MAXCOLORS * sizeof(int) );

/* -------------------------------------------------------------------- */
/*      Test the image format instead of the map format.                */
/*      Normally the map format and the image format should be the      */
/*      same but In somes cases like swf and pdf support, a temporary   */
/*      GD image object is created and used to render raster layers     */
/*      and then dumped into the SWF or the PDF file.                   */
/* -------------------------------------------------------------------- */
  
  //if( MS_RENDERER_GD(map->outputformat) )
  if( MS_RENDERER_GD(image->format) )
  {
      gdImg = image->img.gd;

#if GD2_VERS > 1
      truecolor = gdImageTrueColor( gdImg );
#endif

      if( CSLFetchNameValue( layer->processing, 
                             "COLOR_MATCH_THRESHOLD" ) != NULL )
      {
          cmt = MAX(0,atoi(CSLFetchNameValue( layer->processing, 
                                              "COLOR_MATCH_THRESHOLD" )));
      }
  }

  src_xsize = GDALGetRasterXSize( hDS );
  src_ysize = GDALGetRasterYSize( hDS );

  /*
   * If the RAW_WINDOW attribute is set, use that to establish what to
   * load.  This is normally just set by the mapresample.c module to avoid
   * problems with rotated maps.
   */

  if( CSLFetchNameValue( layer->processing, "RAW_WINDOW" ) != NULL )
  {
      char **papszTokens = 
          CSLTokenizeString( 
              CSLFetchNameValue( layer->processing, "RAW_WINDOW" ) );
      
      if( layer->debug )
          msDebug( "msDrawGDAL(%s): using RAW_WINDOW=%s\n",
                   layer->name,
                   CSLFetchNameValue( layer->processing, "RAW_WINDOW" ) );

      if( CSLCount(papszTokens) != 4 )
      {
          CSLDestroy( papszTokens );
          msSetError( MS_IMGERR, "RAW_WINDOW PROCESSING directive corrupt.",
                      "msDrawGDAL()" );
          return -1;
      }

      src_xoff = atoi(papszTokens[0]);
      src_yoff = atoi(papszTokens[1]);
      src_xsize = atoi(papszTokens[2]);
      src_ysize = atoi(papszTokens[3]);

      dst_xoff = 0;
      dst_yoff = 0;
      dst_xsize = image->width;
      dst_ysize = image->height;

      CSLDestroy( papszTokens );
  }

  /*
   * Compute the georeferenced window of overlap, and do nothing if there
   * is no overlap between the map extents, and the file we are operating on.
   */
  else if( layer->transform )
  {
      msGetGDALGeoTransform( hDS, map, layer, adfGeoTransform );
      InvGeoTransform( adfGeoTransform, adfInvGeoTransform );
      
      mapRect = map->extent;
      
      mapRect.minx -= map->cellsize*0.5;
      mapRect.maxx += map->cellsize*0.5;
      mapRect.miny -= map->cellsize*0.5;
      mapRect.maxy += map->cellsize*0.5;
      
      copyRect = mapRect;
      
      if( copyRect.minx < GEO_TRANS(adfGeoTransform,0,src_ysize) )
          copyRect.minx = GEO_TRANS(adfGeoTransform,0,src_ysize);
      if( copyRect.maxx > GEO_TRANS(adfGeoTransform,src_xsize,0) )
          copyRect.maxx = GEO_TRANS(adfGeoTransform,src_xsize,0);

      if( copyRect.miny < GEO_TRANS(adfGeoTransform+3,0,src_ysize) )
          copyRect.miny = GEO_TRANS(adfGeoTransform+3,0,src_ysize);
      if( copyRect.maxy > GEO_TRANS(adfGeoTransform+3,src_xsize,0) )
          copyRect.maxy = GEO_TRANS(adfGeoTransform+3,src_xsize,0);
      
      if( copyRect.minx >= copyRect.maxx || copyRect.miny >= copyRect.maxy )
          return 0;

      /*
       * Copy the source and destination raster coordinates.
       */
      llx = GEO_TRANS(adfInvGeoTransform+0,copyRect.minx,copyRect.miny);
      lly = GEO_TRANS(adfInvGeoTransform+3,copyRect.minx,copyRect.miny);
      urx = GEO_TRANS(adfInvGeoTransform+0,copyRect.maxx,copyRect.maxy);
      ury = GEO_TRANS(adfInvGeoTransform+3,copyRect.maxx,copyRect.maxy);
      
      src_xoff = MAX(0,(int) llx);
      src_yoff = MAX(0,(int) ury);
      src_xsize = MIN(MAX(0,(int) (urx - llx + 0.5)),
                      GDALGetRasterXSize(hDS) - src_xoff);
      src_ysize = MIN(MAX(0,(int) (lly - ury + 0.5)),
                      GDALGetRasterYSize(hDS) - src_yoff);

      if( src_xsize == 0 || src_ysize == 0 )
      {
          if( layer->debug )
              msDebug( "msDrawGDAL(): no apparent overlap between map view and this window(1).\n" );
          return 0;
      }

      dst_xoff = (int) ((copyRect.minx - mapRect.minx) / map->cellsize);
      dst_yoff = (int) ((mapRect.maxy - copyRect.maxy) / map->cellsize);
      dst_xsize = (int) ((copyRect.maxx - copyRect.minx) / map->cellsize + 0.5);
      dst_xsize = MIN(MAX(1,dst_xsize),image->width - dst_xoff);
      dst_ysize = (int) ((copyRect.maxy - copyRect.miny) / map->cellsize + 0.5);
      dst_ysize = MIN(MAX(1,dst_ysize),image->height - dst_yoff);
      
      if( dst_xsize == 0 || dst_ysize == 0 )
      {
          if( layer->debug )
              msDebug( "msDrawGDAL(): no apparent overlap between map view and this window(2).\n" );
          return 0;
      }
  }

  /*
   * If layer transforms are turned off, just map 1:1.
   */
  else
  {
      dst_xoff = src_xoff = 0;
      dst_yoff = src_yoff = 0;
      dst_xsize = src_xsize = MIN(image->width,src_xsize);
      dst_ysize = src_ysize = MIN(image->height,src_ysize);
  }

  /*
   * Set up the band selection.  We look for a BANDS directive in the 
   * the PROCESSING options.  If not found we default to red=1 or
   * red=1,green=2,blue=3 or red=1,green=2,blue=3,alpha=4. 
   */

  if( CSLFetchNameValue( layer->processing, "BANDS" ) == NULL )
  {
      red_band = 1;

      if( GDALGetRasterCount( hDS ) >= 4 
          && GDALGetRasterColorInterpretation( 
              GDALGetRasterBand( hDS, 4 ) ) == GCI_AlphaBand )
          alpha_band = 4;
      
      if( GDALGetRasterCount( hDS ) >= 3 )
      {
          green_band = 2;
          blue_band = 3;
      }
      hBand1 = GDALGetRasterBand( hDS, red_band );
      if( layer->numclasses != 0 
          || GDALGetRasterColorTable( hBand1 ) != NULL )
      {
          alpha_band = 0;
          green_band = 0;
          blue_band = 0;
      }
  }
  else
  {
      char **papszItems = CSLTokenizeStringComplex( 
          CSLFetchNameValue(layer->processing,"BANDS"), " ,", FALSE, FALSE );
      
      if( CSLCount(papszItems) > 3 )
          alpha_band = atoi(papszItems[3]);
      if( CSLCount(papszItems) > 2 )
      {
          green_band = atoi(papszItems[1]);
          blue_band = atoi(papszItems[2]);
      }
      if( CSLCount(papszItems) > 0 )
          red_band = atoi(papszItems[0]);
      else
      {
          msSetError( MS_IMGERR, "BANDS PROCESSING directive has no items.",
                      "msDrawGDAL()" );
          return -1;
      }
      CSLDestroy( papszItems );
  }

  if( layer->debug > 1 || (layer->debug > 0 && green_band != 0) )
  {
      msDebug( "msDrawGDAL(): red,green,blue,alpha bands = %d,%d,%d,%d\n", 
               red_band, green_band, blue_band, alpha_band );
  }

  /*
   * In RAWDATA mode we don't fool with colors.  Do the raw processing, 
   * and return from the function early.
   */
  hBand1 = GDALGetRasterBand( hDS, red_band );
  if( hBand1 == NULL )
      return -1;

  if( !gdImg )
  {
      void *pBuffer;
      GDALDataType eDataType;

      assert( MS_RENDERER_RAWDATA( image->format ) );

      if( image->format->imagemode == MS_IMAGEMODE_INT16 )
          eDataType = GDT_Int16;
      else if( image->format->imagemode == MS_IMAGEMODE_FLOAT32 )
          eDataType = GDT_Float32;
      else
          return -1;

      pBuffer = malloc(dst_xsize * dst_ysize 
                       * (GDALGetDataTypeSize(eDataType)/8) );
      if( pBuffer == NULL )
          return -1;

      eErr = GDALRasterIO( hBand1, GF_Read, 
                           src_xoff, src_yoff, src_xsize, src_ysize, 
                           pBuffer, dst_xsize, dst_ysize, eDataType, 0, 0 );
      if( eErr != CE_None )
      {
          msSetError( MS_IOERR, "GDALRasterIO() failed: %s", 
                      CPLGetLastErrorMsg(), "drawGDAL()" );
          free( pBuffer );
          return -1;
      }

      k = 0;
      for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
      {
          if( image->format->imagemode == MS_IMAGEMODE_INT16 )
          {
              for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
              {
                  image->img.raw_16bit[j + i * image->width] = 
                      ((GInt16 *) pBuffer)[k++];
              }
          }
          else if( image->format->imagemode == MS_IMAGEMODE_FLOAT32 )
          {
              for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
              {
                  image->img.raw_float[j + i * image->width] = 
                      ((float *) pBuffer)[k++];
              }
          }
      }

      free( pBuffer );

      return 0;
  }

  /*
   * Are we in a one band, RGB or RGBA situation?
   */
  hBand2 = hBand3 = hBandAlpha = NULL;

  if( green_band != 0 )
  {
      hBand1 = GDALGetRasterBand( hDS, red_band );
      hBand2 = GDALGetRasterBand( hDS, green_band );
      hBand3 = GDALGetRasterBand( hDS, blue_band );
      if( hBand1 == NULL || hBand2 == NULL || hBand3 == NULL )
          return -1;

      if( alpha_band != 0 )
          hBandAlpha = GDALGetRasterBand( hDS, alpha_band );
  }
      
  /*
   * Get colormap for this image.  If there isn't one, and we have only
   * one band create a greyscale colormap. 
   */
  if( hBand2 != NULL )
      hColorMap = NULL;
  else
  {
      hColorMap = GDALGetRasterColorTable( hBand1 );
      if( hColorMap != NULL )
          hColorMap = GDALCloneColorTable( hColorMap );
      else if( hBand2 == NULL )
      {
          hColorMap = GDALCreateColorTable( GPI_RGB );
          
          for( i = 0; i < 256; i++ )
          {
              GDALColorEntry sEntry;

              if( truecolor )
              {
                  sEntry.c1 = i;
                  sEntry.c2 = i;
                  sEntry.c3 = i;
                  sEntry.c4 = 255;
              }
              else
              {
                  /*
                  ** This special calculation is intended to use only 128
                  ** unique colors for greyscale in non-truecolor mode.
                  */

                  sEntry.c1 = i - i%2;
                  sEntry.c2 = i - i%2;
                  sEntry.c3 = i - i%2;
                  sEntry.c4 = 255;
              }
                  
              GDALSetColorEntry( hColorMap, i, &sEntry );
          }
      }

      /* 
      ** If we have a known NODATA value, mark it now as transparent. 
      */
      {
          int    bGotNoData;
          double dfNoDataValue = GDALGetRasterNoDataValue(hBand1,&bGotNoData);

          if( bGotNoData && dfNoDataValue >= 0 && dfNoDataValue <= 255.0 )
          {
              GDALColorEntry sEntry;
              
              memcpy( &sEntry, 
                      GDALGetColorEntry( hColorMap, (int) dfNoDataValue ),
                      sizeof(GDALColorEntry) );

              sEntry.c4 = 0;
              GDALSetColorEntry( hColorMap, (int) dfNoDataValue, &sEntry );
          }
      }
  }

  /*
   * Setup the mapping between source eightbit pixel values, and the
   * output images color table.  There are two general cases, where the
   * class colors are provided by the MAP file, or where we use the native
   * color table.
   */
  if(layer->numclasses > 0 && gdImg ) {
    int c, color_count;

    cmap_set = TRUE;

    if( hColorMap == NULL )
    {
        msSetError(MS_IOERR, 
                   "Attempt to classify 24bit image, this is unsupported.",
                   "drawGDAL()");
        return -1;
    }

    color_count = MIN(256,GDALGetColorEntryCount(hColorMap));
    for(i=0; i < color_count; i++) {
        colorObj pixel;
        GDALColorEntry sEntry;

        GDALGetColorEntryAsRGB( hColorMap, i, &sEntry );
            
        pixel.red = sEntry.c1;
        pixel.green = sEntry.c2;
        pixel.blue = sEntry.c3;
        pixel.pen = i;
        
	if(!MS_COMPARE_COLORS(pixel, layer->offsite))
        {
            c = msGetClass(layer, &pixel);
            
            if(c == -1)/* doesn't belong to any class, so handle like offsite*/
                cmap[i] = -1;
            else
            {
                RESOLVE_PEN_GD(gdImg, layer->class[c].styles[0].color);
                if( MS_TRANSPARENT_COLOR(layer->class[c].styles[0].color) )
                    cmap[i] = -1;
                else if( MS_VALID_COLOR(layer->class[c].styles[0].color))
                {
                    /* use class color */
                    cmap[i] = layer->class[c].styles[0].color.pen;
                }
                else /* Use raster color */
                    cmap[i] = msAddColorGD(map, gdImg, cmt,
                                           pixel.red, pixel.green, pixel.blue);
            }
        } else
            cmap[i] = -1;
    }
  } else if( hColorMap != NULL && !truecolor && gdImg ) {
    int color_count;
    cmap_set = TRUE;

    color_count = MIN(256,GDALGetColorEntryCount(hColorMap));

    for(i=0; i < color_count; i++) {
        GDALColorEntry sEntry;

        GDALGetColorEntryAsRGB( hColorMap, i, &sEntry );

        if( sEntry.c4 != 0 
            && (!MS_VALID_COLOR( layer->offsite )
                || layer->offsite.red != sEntry.c1
                || layer->offsite.green != sEntry.c2
                || layer->offsite.blue != sEntry.c3 ) )
            cmap[i] = msAddColorGD(map, gdImg, cmt, 
                                   sEntry.c1, sEntry.c2, sEntry.c3);
        else
            cmap[i] = -1;
    }
  }
#if GD2_VERS > 1
  else if( hBand2 == NULL && hColorMap != NULL )
  {
      int color_count;
      cmap_set = TRUE;

      color_count = MIN(256,GDALGetColorEntryCount(hColorMap));

      for(i=0; i < color_count; i++) {
          GDALColorEntry sEntry;
          
          GDALGetColorEntryAsRGB( hColorMap, i, &sEntry );

          if( sEntry.c4 != 0 
              && (!MS_VALID_COLOR( layer->offsite )
                  || layer->offsite.red != sEntry.c1
                  || layer->offsite.green != sEntry.c2
                  || layer->offsite.blue != sEntry.c3 ) )
              cmap[i] = gdTrueColorAlpha(sEntry.c1, sEntry.c2, sEntry.c3, 
                                         127 - (sEntry.c4 >> 1) );
          else
              cmap[i] = -1;
      }
  }
#endif
  else if( !truecolor && gdImg )
  {
      allocColorCube( map, gdImg, anColorCube );
  }
  else if( !truecolor )
  {
      msSetError(MS_IOERR, 
                 "Unsupported configuration for raster layer read via GDAL.",
                 "drawGDAL()");
      return -1;
  }

  /*
   * Read the entire region of interest in one gulp.  GDAL will take
   * care of downsampling and window logic.  
   */
  pabyRaw1 = (unsigned char *) malloc(dst_xsize * dst_ysize);
  if( pabyRaw1 == NULL )
      return -1;

  if( LoadGDALImage( hBand1, 1, layer, 
                     src_xoff, src_yoff, src_xsize, src_ysize, 
                     pabyRaw1, dst_xsize, dst_ysize ) == -1 )
  {
      free( pabyRaw1 );
      return -1;
  }

  /*
  ** Process single band using the provided, or greyscale colormap
  */
  if( hBand2 == NULL && !truecolor && gdImg )
  {
      assert( cmap_set );
      k = 0;

      for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
      {
          int	result;
          
          for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
          {
              result = cmap[pabyRaw1[k++]];
              if( result != -1 )
              {
#ifndef USE_GD_1_2
                  gdImg->pixels[i][j] = result;
#else
                  gdImg->pixels[j][i] = result;
#endif
              }
          }
      }

      assert( k == dst_xsize * dst_ysize );
  }

#if GD2_VERS > 1
  else if( hBand2 == NULL && truecolor && gdImg )
  {
      assert( cmap_set );

      k = 0;
      for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
      {
          int	result;
          
          for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
          {
              result = cmap[pabyRaw1[k++]];
              if( result != -1 )
                  gdImg->tpixels[i][j] = result;
          }
      }
  }
#endif

  /*
  ** Otherwise read green and blue data, and process through the color cube.
  */
  else if( hBand3 != NULL && gdImg )
  {
      pabyRaw2 = (unsigned char *) malloc(dst_xsize * dst_ysize);
      if( pabyRaw2 == NULL )
          return -1;
      
      if( LoadGDALImage( hBand2, 2, layer, 
                         src_xoff, src_yoff, src_xsize, src_ysize, 
                         pabyRaw2, dst_xsize, dst_ysize ) == -1 )
      {
          free( pabyRaw1 );
          free( pabyRaw2 );
          return -1;
      }

      pabyRaw3 = (unsigned char *) malloc(dst_xsize * dst_ysize);
      if( pabyRaw3 == NULL )
          return -1;
      
      if( LoadGDALImage( hBand3, 3, layer, 
                         src_xoff, src_yoff, src_xsize, src_ysize, 
                         pabyRaw3, dst_xsize, dst_ysize ) == -1 )
      {
          free( pabyRaw1 );
          free( pabyRaw2 );
          free( pabyRaw3 );
          return -1;
      }

      if( hBandAlpha != NULL )
      {
          pabyRawAlpha = (unsigned char *) malloc(dst_xsize * dst_ysize);
          if( pabyRawAlpha == NULL )
              return -1;
          
          if( LoadGDALImage( hBandAlpha, 4, layer, 
                             src_xoff, src_yoff, src_xsize, src_ysize, 
                             pabyRawAlpha, dst_xsize, dst_ysize ) == -1 )
          {
              free( pabyRaw1 );
              free( pabyRaw2 );
              free( pabyRaw3 );
              free( pabyRawAlpha );
              return -1;
          }
      }
      else
          pabyRawAlpha = NULL;

      /* Dithered 24bit to 8bit conversion */
      if( !truecolor && CSLFetchBoolean( layer->processing, "DITHER", FALSE ) )
      {
#ifdef ENABLE_DITHER
          unsigned char *pabyDithered;

          pabyDithered = (unsigned char *) malloc(dst_xsize * dst_ysize);
          if( pabyDithered == NULL )
              return -1;
          
          Dither24to8( pabyRaw1, pabyRaw2, pabyRaw3, pabyDithered,
                       dst_xsize, dst_ysize, image->format->transparent, 
                       map->imagecolor, gdImg );

          k = 0;
          for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
          {
              for( j = dst_xoff; j < dst_xoff + dst_xsize; j++, k++ )
              {
                  if( MS_VALID_COLOR( layer->offsite )
                      && pabyRaw1[k] == layer->offsite.red
                      && pabyRaw2[k] == layer->offsite.green
                      && pabyRaw3[k] == layer->offsite.blue )
                      continue;

                  if( pabyRawAlpha != NULL && pabyRawAlpha[k] == 0 )
                      continue;

#ifndef USE_GD_1_2
                  gdImg->pixels[i][j] = pabyDithered[k];
#else
                  gdImg->pixels[j][i] = pabyDithered[k];
#endif
              }
          }
          
          free( pabyDithered );
#else
          msSetError( MS_IMGERR, 
                      "DITHER not supported in this build.  Update to latest GDAL, and define the ENABLE_DITHER macro.", "drawGDAL()" );
          return -1;
#endif 
      }

      /* Color cubed 24bit to 8bit conversion. */
      else if( !truecolor )
      {
          k = 0;
          for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
          {
              for( j = dst_xoff; j < dst_xoff + dst_xsize; j++, k++ )
              {
                  int	cc_index;
                  
                  if( MS_VALID_COLOR( layer->offsite )
                      && pabyRaw1[k] == layer->offsite.red
                      && pabyRaw2[k] == layer->offsite.green
                      && pabyRaw3[k] == layer->offsite.blue )
                      continue;
                  
                  if( pabyRawAlpha != NULL && pabyRawAlpha[k] == 0 )
                      continue;

                  cc_index= RGB_INDEX(pabyRaw1[k],pabyRaw2[k],pabyRaw3[k]);
#ifndef USE_GD_1_2
                  gdImg->pixels[i][j] = anColorCube[cc_index];
#else
                  gdImg->pixels[j][i] = anColorCube[cc_index];
#endif
              }
          }
      }
#if GD2_VERS > 1
      else if( truecolor )
      {
          k = 0;
          for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
          {
              for( j = dst_xoff; j < dst_xoff + dst_xsize; j++, k++ )
              {
                  if( MS_VALID_COLOR( layer->offsite )
                      && pabyRaw1[k] == layer->offsite.red
                      && pabyRaw2[k] == layer->offsite.green
                      && pabyRaw3[k] == layer->offsite.blue )
                      continue;
                  
                  if( pabyRawAlpha == NULL || pabyRawAlpha[k] == 255 )
                  {
                      gdImg->tpixels[i][j] = 
                          gdTrueColor(pabyRaw1[k], pabyRaw2[k], pabyRaw3[k]);
                  }
                  else if( pabyRawAlpha[k] != 0 )
                  {
                      int gd_alpha = 127 - (pabyRawAlpha[k] >> 1);
                      int gd_color = gdTrueColorAlpha(
                          pabyRaw1[k], pabyRaw2[k], pabyRaw3[k], gd_alpha );
                      int gd_original_alpha, gd_new_alpha;

                      gd_original_alpha = 
                          gdTrueColorGetAlpha( gdImg->tpixels[i][j] );

                      /* I assume a fairly simple additive model for 
                         opaqueness.  Note that gdAlphaBlend() always returns
                         opaque values (alpha byte is 0). */

                      if( gd_original_alpha == 127 )
                      {
                          gdImg->tpixels[i][j] = gd_color;
                      }
                      else
                      {
                          gd_new_alpha = (127 - gd_alpha) + (127 - gd_original_alpha);
                          gd_new_alpha = MAX(0,127 - gd_new_alpha);
                      
                          gdImg->tpixels[i][j] = 
                              (gd_new_alpha << 24) + 
                              gdAlphaBlend( gdImg->tpixels[i][j], gd_color );
                      }
                  }
              }
          }
      }
#endif

      free( pabyRaw2 );
      free( pabyRaw3 );
      if( pabyRawAlpha != NULL )
          free( pabyRawAlpha );
  }

  free( pabyRaw1 );
  if( hColorMap != NULL )
      GDALDestroyColorTable( hColorMap );

  return 0;
}

/************************************************************************/
/*                           LoadGDALImage()                            */
/*                                                                      */
/*      This call will load and process one band of input for the       */
/*      selected rectangle, loading the result into the passed 8bit     */
/*      buffer.  The processing options include scaling.                */
/************************************************************************/

static CPLErr 
LoadGDALImage( GDALRasterBandH hBand, int iColorIndex,  layerObj *layer, 
               int src_xoff, int src_yoff, int src_xsize, int src_ysize, 
               GByte *pabyBuffer, int dst_xsize, int dst_ysize )
    
{
    CPLErr eErr;
    double dfScaleMin=0.0, dfScaleMax=255.0, dfScaleRatio;
    const char *pszScaleInfo;
    float *pafRawData;
    int   nPixelCount = dst_xsize * dst_ysize, i;

/* -------------------------------------------------------------------- */
/*      Fetch the scale processing option.                              */
/* -------------------------------------------------------------------- */
    pszScaleInfo = CSLFetchNameValue( layer->processing, "SCALE" );
    if( pszScaleInfo == NULL )
    {
        char szBandScalingName[20];

        sprintf( szBandScalingName, "SCALE_%d", iColorIndex );
        pszScaleInfo = CSLFetchNameValue( layer->processing, 
                                          szBandScalingName);
    }

    if( pszScaleInfo != NULL )
    {
        char **papszTokens;

        papszTokens = CSLTokenizeStringComplex( pszScaleInfo, " ,", 
                                                FALSE, FALSE );
        if( CSLCount(papszTokens) == 1
            && EQUAL(papszTokens[0],"AUTO") )
        {
            dfScaleMin = dfScaleMax = 0.0;
        }
        else if( CSLCount(papszTokens) != 2 )
        {
            msSetError( MS_MISCERR, 
                        "SCALE PROCESSING option unparsable for layer %s.",
                        layer->name, 
                        "msDrawGDAL()" );
            return -1;
        }
        else
        {
            dfScaleMin = atof(papszTokens[0]);
            dfScaleMax = atof(papszTokens[1]);
        }
        CSLDestroy( papszTokens );
    }
    
/* -------------------------------------------------------------------- */
/*      If we have no scaling parameters, just truncate to 8bit.        */
/* -------------------------------------------------------------------- */
    if( pszScaleInfo == NULL )
    {
        eErr = GDALRasterIO( hBand, GF_Read, 
                             src_xoff, src_yoff, src_xsize, src_ysize, 
                             pabyBuffer, dst_xsize, dst_ysize, GDT_Byte, 0,0);

        if( eErr != CE_None )
            msSetError( MS_IOERR, "GDALRasterIO() failed: %s", 
                        CPLGetLastErrorMsg(), "drawGDAL()" );

        if( eErr == CE_None )
            return 0;
        else
            return -1;
    }

/* -------------------------------------------------------------------- */
/*      Otherwise, allocate a temporary floating point buffer, and      */
/*      load into that.                                                 */
/* -------------------------------------------------------------------- */
    pafRawData = (float *) malloc(sizeof(float) * dst_xsize * dst_ysize );
    
    eErr = GDALRasterIO( hBand, GF_Read, 
                         src_xoff, src_yoff, src_xsize, src_ysize, 
                         pafRawData, dst_xsize, dst_ysize, GDT_Float32, 0, 0 );
    
    if( eErr != CE_None )
    {
        msSetError( MS_IOERR, "GDALRasterIO() failed: %s", 
                    CPLGetLastErrorMsg(), "drawGDAL()" );

        free( pafRawData );
        return -1;
    }

/* -------------------------------------------------------------------- */
/*      If we are using autoscaling, then compute the max and min       */
/*      now.  Perhaps we should eventually honour the offsite value     */
/*      as a nodata value, or get it from GDAL.                         */
/* -------------------------------------------------------------------- */
    if( dfScaleMin == dfScaleMax )
    {
        dfScaleMin = dfScaleMax = pafRawData[0];

        for( i = 1; i < nPixelCount; i++ )
        {
            dfScaleMin = MIN(dfScaleMin,pafRawData[i]);
            dfScaleMax = MAX(dfScaleMax,pafRawData[i]);
        }
        if( dfScaleMin == dfScaleMax )
            dfScaleMax = dfScaleMin + 1.0;
    }

    if( layer->debug > 0 )
        msDebug( "msDrawGDAL(%s): scaling to 8bit, src range=%g,%g\n",
                 layer->name, dfScaleMin, dfScaleMax );

/* -------------------------------------------------------------------- */
/*      Now process the data.                                           */
/* -------------------------------------------------------------------- */
    dfScaleRatio = 256.0 / (dfScaleMax - dfScaleMin);

    for( i = 0; i < nPixelCount; i++ )
    {
        float fScaledValue = (pafRawData[i] - dfScaleMin) * dfScaleRatio;

        if( fScaledValue < 0.0 )
            pabyBuffer[i] = 0;
        else if( fScaledValue > 255.0 )
            pabyBuffer[i] = 255;
        else
            pabyBuffer[i] = (int) fScaledValue;
    }

    free( pafRawData );

    return 0;
}

/************************************************************************/
/*                           allocColorCube()                           */
/*                                                                      */
/*      Allocate color table entries as best possible for a color       */
/*      cube.                                                           */
/************************************************************************/

static int allocColorCube(mapObj *map, gdImagePtr img, int *panColorCube)

{
    int	 r, g, b;
    int i = 0;
    int nGreyColors = 32;
    int nSpaceGreyColors = 8;
    int iColors = 0;
    int	red, green, blue;

    for( r = 0; r < RED_LEVELS; r++ )
    {
        for( g = 0; g < GREEN_LEVELS; g++ )
        {
            for( b = 0; b < BLUE_LEVELS; b++ )
            {                
                red = MS_MIN(255,r * (255 / (RED_LEVELS-1)));
                green = MS_MIN(255,g * (255 / (GREEN_LEVELS-1)));
                blue = MS_MIN(255,b * (255 / (BLUE_LEVELS-1)));

                panColorCube[RGB_LEVEL_INDEX(r,g,b)] = 
                    msAddColorGD(map, img, 1, red, green, blue );
                iColors++;
            }
        }
    }
/* -------------------------------------------------------------------- */
/*      Adding 32 grey colors                                           */
/* -------------------------------------------------------------------- */
    for (i=0; i<nGreyColors; i++)
    {
        red = i*nSpaceGreyColors;
        green = red;
        blue = red;
        if(iColors < 256)
        {
            panColorCube[iColors] = msAddColorGD(map,img,1,red,green,blue);
            iColors++;
        }
    }
    return MS_SUCCESS;
}

/************************************************************************/
/*                            Dither24to8()                             */
/*                                                                      */
/*      Wrapper for GDAL dithering algorithm.                           */
/************************************************************************/

#ifdef ENABLE_DITHER

static void Dither24to8( GByte *pabyRed, GByte *pabyGreen, GByte *pabyBlue,
                         GByte *pabyDithered, int xsize, int ysize, 
                         int bTransparent, colorObj transparent,
                         gdImagePtr gdImg )

{
    GDALDatasetH hDS;
    GDALDriverH  hDriver;
    char **papszBandOptions = NULL;
    char szDataPointer[120];
    GDALColorTableH hCT;
    int c;

/* -------------------------------------------------------------------- */
/*      Create the "memory" GDAL dataset referencing our working        */
/*      arrays.                                                         */
/* -------------------------------------------------------------------- */
    hDriver = GDALGetDriverByName( "MEM" );
    if( hDriver == NULL )
        return;

    hDS = GDALCreate( hDriver, "TempDitherDS", xsize, ysize, 0, GDT_Byte, 
                      NULL );

    /* Add Red Band */
    sprintf( szDataPointer, "%p", pabyRed );
    papszBandOptions = CSLSetNameValue( papszBandOptions, "DATAPOINTER", 
                                        szDataPointer );
    GDALAddBand( hDS, GDT_Byte, papszBandOptions );
                                        
    /* Add Green Band */
    sprintf( szDataPointer, "%p", pabyGreen );
    papszBandOptions = CSLSetNameValue( papszBandOptions, "DATAPOINTER", 
                                        szDataPointer );
    GDALAddBand( hDS, GDT_Byte, papszBandOptions );
                                        
    /* Add Blue Band */
    sprintf( szDataPointer, "%p", pabyBlue );
    papszBandOptions = CSLSetNameValue( papszBandOptions, "DATAPOINTER", 
                                        szDataPointer );
    GDALAddBand( hDS, GDT_Byte, papszBandOptions );
                                        
    /* Add Dithered Band */
    sprintf( szDataPointer, "%p", pabyDithered );
    papszBandOptions = CSLSetNameValue( papszBandOptions, "DATAPOINTER", 
                                        szDataPointer );
    GDALAddBand( hDS, GDT_Byte, papszBandOptions );

    CSLDestroy( papszBandOptions );
                                        
/* -------------------------------------------------------------------- */
/*      Create the color table.                                         */
/* -------------------------------------------------------------------- */
    hCT = GDALCreateColorTable( GPI_RGB );

    for (c = 0; c < gdImg->colorsTotal; c++) 
    {
        GDALColorEntry sEntry;
              
        sEntry.c1 = gdImg->red[c];
        sEntry.c2 = gdImg->green[c];
        sEntry.c3 = gdImg->blue[c];

        if( bTransparent 
            && transparent.red == sEntry.c1
            && transparent.green == sEntry.c2
            && transparent.blue == sEntry.c3 )
            sEntry.c4 = 0;
        else
            sEntry.c4 = 255;
        
        GDALSetColorEntry( hCT, c, &sEntry );
    }

/* -------------------------------------------------------------------- */
/*      Perform dithering.                                              */
/* -------------------------------------------------------------------- */
    GDALDitherRGB2PCT( GDALGetRasterBand( hDS, 1 ),
                       GDALGetRasterBand( hDS, 2 ),
                       GDALGetRasterBand( hDS, 3 ),
                       GDALGetRasterBand( hDS, 4 ),
                       hCT, NULL, NULL );

/* -------------------------------------------------------------------- */
/*      Cleanup.                                                        */
/* -------------------------------------------------------------------- */
    GDALDestroyColorTable( hCT );
    GDALClose( hDS );
}
#endif /* def ENABLE_DITHER */

/************************************************************************/
/*                       msGetGDALGeoTransform()                        */
/*                                                                      */
/*      Cover function that tries GDALGetGeoTransform(), a world        */
/*      file or OWS extents.                                            */
/************************************************************************/

int msGetGDALGeoTransform( GDALDatasetH hDS, mapObj *map, layerObj *layer, 
                           double *padfGeoTransform )

{
    rectObj  rect;

/* -------------------------------------------------------------------- */
/*      some GDAL drivers (ie. GIF) don't set geotransform on failure.  */
/* -------------------------------------------------------------------- */
    padfGeoTransform[0] = 0.0;
    padfGeoTransform[1] = 1.0;
    padfGeoTransform[2] = 0.0;
    padfGeoTransform[3] = GDALGetRasterYSize(hDS);
    padfGeoTransform[4] = 0.0;
    padfGeoTransform[5] = -1.0;
    
/* -------------------------------------------------------------------- */
/*      Try GDAL.                                                       */
/*                                                                      */
/*      Make sure that ymax is always at the top, and ymin at the       */
/*      bottom ... that is flip any files without the usual             */
/*      orientation.  This is intended to enable display of "raw"       */
/*      files with no coordinate system otherwise they break down in    */
/*      many ways.                                                      */
/* -------------------------------------------------------------------- */
    if (GDALGetGeoTransform( hDS, padfGeoTransform ) == CE_None )
    {
        if( padfGeoTransform[5] == 1.0 && padfGeoTransform[3] == 0.0 )
        {
            padfGeoTransform[5] = -1.0;
            padfGeoTransform[3] = GDALGetRasterYSize(hDS);
        }

        return MS_SUCCESS;
    }

/* -------------------------------------------------------------------- */
/*      Try worldfile.                                                  */
/* -------------------------------------------------------------------- */
    else if( GDALGetDescription(hDS) != NULL 
             && GDALReadWorldFile(GDALGetDescription(hDS), "wld", 
                                  padfGeoTransform) )
    {
        return MS_SUCCESS;
    }

/* -------------------------------------------------------------------- */
/*      Try OWS extent metadata.                                        */
/* -------------------------------------------------------------------- */
    else if( msOWSGetLayerExtent( map, layer, &rect ) == MS_SUCCESS )
    {
        padfGeoTransform[0] = rect.minx;
        padfGeoTransform[1] = (rect.maxx - rect.minx) /
            (double) GDALGetRasterXSize( hDS );
        padfGeoTransform[2] = 0;
        padfGeoTransform[3] = rect.maxy;
        padfGeoTransform[4] = 0;
        padfGeoTransform[5] = (rect.miny - rect.maxy) /
            (double) GDALGetRasterYSize( hDS );

        return MS_SUCCESS;
    }

/* -------------------------------------------------------------------- */
/*      We didn't find any info ... use the default.                    */
/* -------------------------------------------------------------------- */
    else
        return MS_FAILURE;
}

#endif

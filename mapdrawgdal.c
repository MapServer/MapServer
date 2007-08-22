/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Code for drawing GDAL raster layers.  Called from 
 *           msDrawRasterLayerLow() in mapraster.c.  
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.48.2.1  2007/02/05 05:56:19  frank
 * Fix support for OFFSITE for simple greyscale rasters (bug 2024).
 *
 * Revision 1.48  2006/02/22 03:52:04  frank
 * Incorporate range coloring support for rasters (bug 1673)
 *
 * Revision 1.47  2005/11/24 16:16:41  frank
 * fixed raster crash with some grass rasters (bug 1541)
 *
 * Revision 1.46  2005/08/25 22:01:50  frank
 * fix problem with ungeorerenced files (eg augsignals)
 *
 * Revision 1.45  2005/06/14 16:03:33  dan
 * Updated copyright date to 2005
 *
 * Revision 1.44  2005/05/06 17:29:30  sdlime
 * Removed debuging statement from last fix.
 *
 * Revision 1.43  2005/05/06 17:19:05  frank
 * bug 985/1015 - dont classify rasters without expressions
 *
 * Revision 1.42  2005/02/18 03:06:45  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.41  2005/01/23 20:40:23  frank
 * LoadGDALImage() should return int, not CPLErr.
 *
 * Revision 1.40  2005/01/19 21:20:13  frank
 * improve autoscaling - add small amount to scaling range - bug 1168
 *
 * Revision 1.39  2004/11/16 21:57:49  dan
 * Final pass at updating WMS/WFS client/server interfaces to lookup "ows_*"
 * metadata in addition to default "wms_*"/"wfs_*" metadata (bug 568)
 *
 * Revision 1.38  2004/11/15 21:39:15  frank
 * fix last fix: GDALPrintPointer to CPLPrintPointer
 *
 * Revision 1.37  2004/11/15 21:28:48  frank
 * Use GDALPrintPointer() to avoid pointer conversion errors on win32. Bug 722.
 *
 * Revision 1.36  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 * Revision 1.35  2004/10/21 02:23:24  frank
 * updated header license a bit, and added MS_CVSID
 *
 * Revision 1.34  2004/10/18 14:49:12  frank
 * implemented msAlphaBlend
 *
 * Revision 1.33  2004/10/18 13:05:14  frank
 * Fixed typos in alpha handling cleanup.
 *
 * Revision 1.32  2004/10/17 03:39:03  frank
 * added greyscale+alpha support - bug 965
 *
 * Revision 1.31  2004/09/29 17:12:13  frank
 * fixed casting issues to avoid warnings
 *
 * Revision 1.30  2004/09/03 14:24:07  frank
 * %p cannot be encoded and decoded properly on win32
 *
 * Revision 1.29  2004/05/12 19:40:19  frank
 * Fixed my last fix ... damn it.
 *
 * Revision 1.28  2004/05/11 22:35:06  frank
 * Avoid data init warning.
 *
 * Revision 1.27  2004/05/11 22:33:27  frank
 * bug 493: fixed last fix, memory corruption in some cases
 *
 * Revision 1.26  2004/05/05 04:14:39  frank
 * Fixed problem with computing dst_xsize and dst_ysize for rendering image.
 * In some subtle cases they would be one less than made sense.
 * http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=493
 *
 * Revision 1.25  2004/04/09 03:03:58  frank
 * improvement for msGetGDALBandList()
 *
 * Revision 1.24  2004/04/08 17:25:25  frank
 * added msGetGDALBandList()
 *
 * Revision 1.23  2004/03/11 23:03:56  assefa
 * Correct bug in a loop in function msDrawRasterLayerGDAL_16BitClassification
 *
 * Revision 1.22  2004/03/08 17:50:22  frank
 * completed 16bit classified support
 *
 * Revision 1.21  2004/03/05 23:18:52  frank
 * Added small TODO note.
 *
 * Revision 1.20  2004/03/05 22:58:04  frank
 * preliminary working implementation of msDrawRasterLayerGDAL_16BitClassification
 *
 * Revision 1.19  2004/03/05 19:55:10  frank
 * Dont call GDALDatasetRasterIO() in pre 1.2.0 GDAL builds
 *
 * Revision 1.18  2004/03/05 05:57:04  frank
 * support multi-band rawmode output
 *
 * Revision 1.17  2004/03/04 20:08:28  frank
 * added IMAGEMODE_BYTE (raw mode)
 *
 * Revision 1.16  2004/02/05 05:46:09  frank
 * don't call msOWSGetLayerExtent without OWS services being enabled
 *
 * Revision 1.15  2004/02/02 17:15:32  frank
 * class pens being reused inappropriately - bug 504
 *
 * Revision 1.14  2004/01/26 15:20:11  frank
 * added msGetGDALGeoTransform
 *
 * Revision 1.13  2004/01/15 19:49:23  frank
 * ensure geotransform is set on failure of GetGeotransform
 */

#include <assert.h>
#include "map.h"
#include "mapresample.h"
#include "mapthread.h"

MS_CVSID("$Id$")

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

static int
LoadGDALImage( GDALRasterBandH hBand, int iColorIndex, 
               layerObj *layer, 
               int src_xoff, int src_yoff, int src_xsize, int src_ysize, 
               GByte *pabyBuffer, int dst_xsize, int dst_ysize );
static int 
msDrawRasterLayerGDAL_RawMode(
    mapObj *map, layerObj *layer, imageObj *image, GDALDatasetH hDS, 
    int src_xoff, int src_yoff, int src_xsize, int src_ysize,
    int dst_xoff, int dst_yoff, int dst_xsize, int dst_ysize );

static int 
msDrawRasterLayerGDAL_16BitClassification(
    mapObj *map, layerObj *layer, imageObj *image, 
    GDALDatasetH hDS, GDALRasterBandH hBand,
    int src_xoff, int src_yoff, int src_xsize, int src_ysize,
    int dst_xoff, int dst_yoff, int dst_xsize, int dst_ysize );


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
/* #define GREEN_LEVELS 7 */
#define GREEN_LEVELS 5
/* #define GREEN_DIV 37 */
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
  unsigned char *pabyRaw1=NULL, *pabyRaw2=NULL, *pabyRaw3=NULL, 
                *pabyRawAlpha = NULL;
  int truecolor = FALSE, classified = FALSE;
  int red_band=0, green_band=0, blue_band=0, alpha_band=0, cmt=0;
  gdImagePtr gdImg = NULL;
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
  
  /* if( MS_RENDERER_GD(map->outputformat) ) */
  if( MS_RENDERER_GD(image->format) )
  {
      gdImg = image->img.gd;

      truecolor = gdImageTrueColor( gdImg );
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
      int dst_lrx, dst_lry;

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

      dst_lrx = (int) ((copyRect.maxx - mapRect.minx) / map->cellsize + 0.5);
      dst_lry = (int) ((mapRect.maxy - copyRect.miny) / map->cellsize + 0.5);
      dst_lrx = MAX(0,MIN(image->width,dst_lrx));
      dst_lry = MAX(0,MIN(image->height,dst_lry));
      
      dst_xsize = MAX(0,MIN(image->width,dst_lrx - dst_xoff));
      dst_ysize = MAX(0,MIN(image->height,dst_lry - dst_yoff));

      if( dst_xsize == 0 || dst_ysize == 0 )
      {
          if( layer->debug )
              msDebug( "msDrawGDAL(): no apparent overlap between map view and this window(2).\n" );
          return 0;
      }

#ifndef notdef
      if( layer->debug )
          msDebug( "msDrawGDAL(): src=%d,%d,%d,%d, dst=%d,%d,%d,%d\n", 
                   src_xoff, src_yoff, src_xsize, src_ysize, 
                   dst_xoff, dst_yoff, dst_xsize, dst_ysize );
#endif
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
   * In RAWDATA mode we don't fool with colors.  Do the raw processing, 
   * and return from the function early.
   */
  if( !gdImg )
  {
      assert( MS_RENDERER_RAWDATA( image->format ) );

      return msDrawRasterLayerGDAL_RawMode( 
          map, layer, image, hDS, 
          src_xoff, src_yoff, src_xsize, src_ysize, 
          dst_xoff, dst_yoff, dst_xsize, dst_ysize );
  }
  
  /*
   * Is this image classified?  We consider it classified if there are
   * classes with an expression string *or* a color range.  We don't want
   * to treat the raster as classified if there is just a bogus class here
   * to force inclusion in the legend.
   */
  for( i = 0; i < layer->numclasses; i++ )
  {
      int s;

      /* change colour based on colour range? */
      for(s=0; s<layer->class[i].numstyles; s++)
      {
          if( MS_VALID_COLOR(layer->class[i].styles[s].mincolor)
              && MS_VALID_COLOR(layer->class[i].styles[s].maxcolor) )
          {
              classified = TRUE;
              break;
          }
      }
      
      if( layer->class[i].expression.string != NULL )
      {
          classified = TRUE;
          break;
      }
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

      if( GDALGetRasterCount( hDS ) == 2 
          && GDALGetRasterColorInterpretation( 
              GDALGetRasterBand( hDS, 2 ) ) == GCI_AlphaBand )
          alpha_band = 2;
      
      hBand1 = GDALGetRasterBand( hDS, red_band );
      if( classified 
          || GDALGetRasterColorTable( hBand1 ) != NULL )
      {
          alpha_band = 0;
          green_band = 0;
          blue_band = 0;
      }
  }
  else
  {
      int band_count, *band_list;

      band_list = msGetGDALBandList( layer, hDS, 4, &band_count );
      if( band_list == NULL )
          return -1;
      
      if( band_count > 0 )
          red_band = band_list[0];
      else
          red_band = 0;
      if( band_count > 2 )
      {
          green_band = band_list[1];
          blue_band = band_list[2];
      }
      else	
      {
          green_band = 0;
          blue_band = 0;
      }

      if( band_count > 3 )
          alpha_band = band_list[3];
      else
          alpha_band = 0;

      free( band_list );
  }

  if( layer->debug > 1 || (layer->debug > 0 && green_band != 0) )
  {
      msDebug( "msDrawGDAL(): red,green,blue,alpha bands = %d,%d,%d,%d\n", 
               red_band, green_band, blue_band, alpha_band );
  }

  /*
   * Get band handles for PC256, RGB or RGBA cases.
   */
  hBand1 = GDALGetRasterBand( hDS, red_band );
  if( hBand1 == NULL )
      return -1;

  hBand2 = hBand3 = hBandAlpha = NULL;

  if( green_band != 0 )
  {
      hBand1 = GDALGetRasterBand( hDS, red_band );
      hBand2 = GDALGetRasterBand( hDS, green_band );
      hBand3 = GDALGetRasterBand( hDS, blue_band );
      if( hBand1 == NULL || hBand2 == NULL || hBand3 == NULL )
          return -1;
  }

  if( alpha_band != 0 )
      hBandAlpha = GDALGetRasterBand( hDS, alpha_band );

  /*
   * Wipe pen indicators for all our layer class colors if they exist.  
   * Sometimes temporary gdImg'es are used in which case previously allocated
   * pens won't generally apply.  See Bug 504.
   */
  if( gdImg && !truecolor )
  {
      int iClass;
      for( iClass = 0; iClass < layer->numclasses; iClass++ )
          layer->class[iClass].styles[0].color.pen = MS_PEN_UNSET;
  }

  /*
   * The logic for a classification rendering of non-8bit raster bands
   * is sufficiently different than the normal mechanism of loading
   * into an 8bit buffer, that we isolate it into it's own subfunction.
   */
  if( classified && gdImg 
     && hBand1 != NULL && GDALGetRasterDataType( hBand1 ) != GDT_Byte ) 
  {
      return msDrawRasterLayerGDAL_16BitClassification( 
          map, layer, image, hDS, hBand1,
          src_xoff, src_yoff, src_xsize, src_ysize, 
          dst_xoff, dst_yoff, dst_xsize, dst_ysize );
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
              colorObj pixel;
              GDALColorEntry sEntry;

              pixel.red = i;
              pixel.green = i;
              pixel.blue = i;
              pixel.pen = i;
              
              if(MS_COMPARE_COLORS(pixel, layer->offsite))
              {
                  sEntry.c1 = 0;
                  sEntry.c2 = 0;
                  sEntry.c3 = 0;
                  sEntry.c4 = 0; /* alpha set to zero */
              }
              else if( truecolor )
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
          double dfNoDataValue = msGetGDALNoDataValue( layer, hBand1, 
                                                       &bGotNoData);

          if( bGotNoData && dfNoDataValue >= 0 
              && dfNoDataValue < GDALGetColorEntryCount( hColorMap ) )
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
  if( classified && gdImg ) {
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
                int s;
                
                /* change colour based on colour range?  Currently we 
                   only address the greyscale case properly. */

                for(s=0; s<layer->class[c].numstyles; s++)
                {
                    if( MS_VALID_COLOR(layer->class[c].styles[s].mincolor)
                        && MS_VALID_COLOR(layer->class[c].styles[s].maxcolor) )
                        msValueToRange(&layer->class[c].styles[s],
                                       sEntry.c1 );
                }

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

  if( hBand2 != NULL && hBand3 != NULL )
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
          if( pabyRaw3 != NULL )
          {
              free( pabyRaw2 );
              free( pabyRaw3 );
          }

          free( pabyRawAlpha );
          return -1;
      }
  }
  else
      pabyRawAlpha = NULL;

/* -------------------------------------------------------------------- */
/*      Single band plus colormap with alpha blending to 8bit.          */
/* -------------------------------------------------------------------- */
  if( hBand2 == NULL && !truecolor && gdImg && hBandAlpha != NULL )
  {
      assert( cmap_set );
      k = 0;

      for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
      {
          int	result, alpha;
          
          for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
          {
              alpha = pabyRawAlpha[k];

              result = cmap[pabyRaw1[k++]];

              /* 
              ** We don't do alpha blending in non-truecolor mode, just
              ** threshold the point on/off at alpha=128.
              */

              if( result != -1 && alpha >= 128 )
                  gdImg->pixels[i][j] = result;
          }
      }

      assert( k == dst_xsize * dst_ysize );
  }

/* -------------------------------------------------------------------- */
/*      Single band plus colormap (no alpha) to 8bit.                   */
/* -------------------------------------------------------------------- */
  else if( hBand2 == NULL && !truecolor && gdImg )
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
                  gdImg->pixels[i][j] = result;
              }
          }
      }

      assert( k == dst_xsize * dst_ysize );
  }

/* -------------------------------------------------------------------- */
/*      Single band plus colormap and alpha to truecolor.               */
/* -------------------------------------------------------------------- */
  else if( hBand2 == NULL && truecolor && gdImg && hBandAlpha != NULL )
  {
      assert( cmap_set );

      k = 0;
      for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
      {
          int	result, alpha;
          
          for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
          {
              alpha = pabyRawAlpha[k];
              result = cmap[pabyRaw1[k++]];

              if( result == -1 || alpha < 2 )
                  /* do nothing - transparent */;
              else if( alpha > 253 )
                  gdImg->tpixels[i][j] = result;
              else
              {
                  /* mix alpha into "result" */
                  result += (127 - (alpha >> 1)) << 24;
                  gdImg->tpixels[i][j] = 
                      msAlphaBlend( gdImg->tpixels[i][j], result );
              }
          }
      }
  }

/* -------------------------------------------------------------------- */
/*      Single band plus colormap (no alpha) to truecolor               */
/* -------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------- */
/*      Input is 3 band RGB.  Alpha blending is mixed into the loop     */
/*      since this case is less commonly used and has lots of other     */
/*      overhead.                                                       */
/* -------------------------------------------------------------------- */
  else if( hBand3 != NULL && gdImg )
  {
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

                  gdImg->pixels[i][j] = pabyDithered[k];
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
                  gdImg->pixels[i][j] = anColorCube[cc_index];
              }
          }
      }
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
                      int gd_color;
                      int gd_alpha = 127 - (pabyRawAlpha[k] >> 1);

                      gd_color = gdTrueColorAlpha(
                          pabyRaw1[k], pabyRaw2[k], pabyRaw3[k], gd_alpha );

                      /* NOTE: GD versions prior to 2.0.12 didn't take
                         the source alpha into account at all */

                      gdImg->tpixels[i][j] = 
                          msAlphaBlend( gdImg->tpixels[i][j], gd_color );
                  }
              }
          }
      }
  }

  /*
  ** Cleanup
  */

  if( pabyRaw3 != NULL )
  {
      free( pabyRaw2 );
      free( pabyRaw3 );
  }

  if( pabyRawAlpha != NULL )
      free( pabyRawAlpha );

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

static int
LoadGDALImage( GDALRasterBandH hBand, int iColorIndex,  layerObj *layer, 
               int src_xoff, int src_yoff, int src_xsize, int src_ysize, 
               GByte *pabyBuffer, int dst_xsize, int dst_ysize )
    
{
    CPLErr eErr;
    double dfScaleMin=0.0, dfScaleMax=255.0, dfScaleRatio, dfNoDataValue;
    const char *pszScaleInfo;
    float *pafRawData;
    int   nPixelCount = dst_xsize * dst_ysize, i, bGotNoData = FALSE;

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
        float fScaledValue = (float) ((pafRawData[i]-dfScaleMin)*dfScaleRatio);

        if( fScaledValue < 0.0 )
            pabyBuffer[i] = 0;
        else if( fScaledValue > 255.0 )
            pabyBuffer[i] = 255;
        else
            pabyBuffer[i] = (int) fScaledValue;
    }

    free( pafRawData );

/* -------------------------------------------------------------------- */
/*      Report a warning if NODATA keyword was applied.  We are         */
/*      unable to utilize it since we can't return any pixels marked    */
/*      as nodata from this function.  Need to fix someday.             */
/* -------------------------------------------------------------------- */
    dfNoDataValue = msGetGDALNoDataValue( layer, hBand, &bGotNoData );
    if( bGotNoData )
        msDebug( "LoadGDALImage(%s): NODATA value %g in GDAL\n"
                 "file or PROCESSING directive ignored.  Not yet supported for\n"
                 "unclassified scaled data.\n",
                 layer->name, dfNoDataValue );

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

#if GDAL_VERSION_NUM <= 1240 
static int CPLPrintPointer( char *buffer, void *pData, int nMax )

{
#ifdef WIN32
    sprintf( buffer, "%ld", (long) pData );
#else
    sprintf( buffer, "%p", pData );
#endif

    return strlen( buffer );
}
#endif /* GDAL_VERSION_NUM <= 1240 */

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
    memset( szDataPointer, 0, sizeof(szDataPointer) );
    CPLPrintPointer( szDataPointer, pabyRed, sizeof(szDataPointer) );
    papszBandOptions = CSLSetNameValue( papszBandOptions, "DATAPOINTER", 
                                        szDataPointer );
    GDALAddBand( hDS, GDT_Byte, papszBandOptions );
                                        
    /* Add Green Band */
    memset( szDataPointer, 0, sizeof(szDataPointer) );
    CPLPrintPointer( szDataPointer, pabyGreen, sizeof(szDataPointer) );
    papszBandOptions = CSLSetNameValue( papszBandOptions, "DATAPOINTER", 
                                        szDataPointer );
    GDALAddBand( hDS, GDT_Byte, papszBandOptions );
                                        
    /* Add Blue Band */
    memset( szDataPointer, 0, sizeof(szDataPointer) );
    CPLPrintPointer( szDataPointer, pabyBlue, sizeof(szDataPointer) );
    papszBandOptions = CSLSetNameValue( papszBandOptions, "DATAPOINTER", 
                                        szDataPointer );
    GDALAddBand( hDS, GDT_Byte, papszBandOptions );
                                        
    /* Add Dithered Band */
    memset( szDataPointer, 0, sizeof(szDataPointer) );
    CPLPrintPointer( szDataPointer, pabyDithered, sizeof(szDataPointer) );
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
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR)
    else if( msOWSGetLayerExtent( map, layer, "MFCO", &rect ) == MS_SUCCESS )
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
#endif

/* -------------------------------------------------------------------- */
/*      We didn't find any info ... use the default.                    */
/*      Reset our default geotransform.  GDALGetGeoTransform() may      */
/*      have altered it even if GDALGetGeoTransform() failed.           */
/* -------------------------------------------------------------------- */
    else
    {
        padfGeoTransform[0] = 0.0;
        padfGeoTransform[1] = 1.0;
        padfGeoTransform[2] = 0.0;
        padfGeoTransform[3] = GDALGetRasterYSize(hDS);
        padfGeoTransform[4] = 0.0;
        padfGeoTransform[5] = -1.0;

        return MS_FAILURE;
    }
}

/************************************************************************/
/*                   msDrawRasterLayerGDAL_RawMode()                    */
/*                                                                      */
/*      Handle the loading and application of data in rawmode.          */
/************************************************************************/

static int 
msDrawRasterLayerGDAL_RawMode(
    mapObj *map, layerObj *layer, imageObj *image, GDALDatasetH hDS, 
    int src_xoff, int src_yoff, int src_xsize, int src_ysize,
    int dst_xoff, int dst_yoff, int dst_xsize, int dst_ysize )

{
    void *pBuffer;
    GDALDataType eDataType;
    int *band_list, band_count;
    int  i, j, k, band;
    CPLErr eErr;

    if( image->format->bands > 256 )
    {
        msSetError( MS_IMGERR, "Too many bands (more than 256).",
                    "msDrawRasterLayerGDAL_RawMode()" );
        return -1;
    }

/* -------------------------------------------------------------------- */
/*      What data type do we need?                                      */
/* -------------------------------------------------------------------- */
    if( image->format->imagemode == MS_IMAGEMODE_INT16 )
        eDataType = GDT_Int16;
    else if( image->format->imagemode == MS_IMAGEMODE_FLOAT32 )
        eDataType = GDT_Float32;
    else if( image->format->imagemode == MS_IMAGEMODE_BYTE )
        eDataType = GDT_Byte;
    else
        return -1;

/* -------------------------------------------------------------------- */
/*      Work out the band list.                                         */
/* -------------------------------------------------------------------- */
    band_list = msGetGDALBandList( layer, hDS, image->format->bands, 
                                   &band_count );
    if( band_list == NULL )
        return -1;

    if( band_count != image->format->bands )
    {
        free( band_list );
        msSetError( MS_IMGERR, "BANDS PROCESSING directive has wrong number of bands, expected %d, got %d.",
                    "msDrawRasterLayerGDAL_RawMode()",
                    image->format->bands, band_count );
        return -1;
    }

/* -------------------------------------------------------------------- */
/*      Allocate buffer, and read data into it.                         */
/* -------------------------------------------------------------------- */
    pBuffer = malloc(dst_xsize * dst_ysize * image->format->bands
                     * (GDALGetDataTypeSize(eDataType)/8) );
    if( pBuffer == NULL )
        return -1;

#if defined(GDAL_VERSION_NUM) && GDAL_VERSION_NUM >= 1199
    eErr = GDALDatasetRasterIO( hDS, GF_Read,  
                                src_xoff, src_yoff, src_xsize, src_ysize, 
                                pBuffer, dst_xsize, dst_ysize, eDataType, 
                                image->format->bands, band_list,
                                0, 0, 0 );
    free( band_list );

    if( eErr != CE_None )
    {
        msSetError( MS_IOERR, "GDALRasterIO() failed: %s", 
                    CPLGetLastErrorMsg(), 
                    "msDrawRasterLayerGDAL_RawMode()" );
        free( pBuffer );
        return -1;
    }
#else
    /*
     * The above could actually be implemented for pre-1.2.0 GDALs
     * reading band by band, but it would be hard to do and test and would
     * be very rarely useful so we skip it.
     */
    msSetError(MS_IMGERR, 
               "RAWMODE raster support requires GDAL 1.2.0 or newer.", 
               "msDrawRasterLayerGDAL_RawMode()" );
    free( pBuffer );
    return -1;
#endif

/* -------------------------------------------------------------------- */
/*      Transfer the data to the imageObj.                              */
/* -------------------------------------------------------------------- */
    k = 0;
    for( band = 0; band < image->format->bands; band++ )
    {
        for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
        {
            if( image->format->imagemode == MS_IMAGEMODE_INT16 )
            {
                for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
                {
                    image->img.raw_16bit[j + i * image->width
                                         + band*image->width*image->height] = 
                        ((GInt16 *) pBuffer)[k++];
                }
            }
            else if( image->format->imagemode == MS_IMAGEMODE_FLOAT32 )
            {
                for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
                {
                    image->img.raw_float[j + i * image->width
                                         + band*image->width*image->height] = 
                        ((float *) pBuffer)[k++];
                }
            }
            else if( image->format->imagemode == MS_IMAGEMODE_BYTE )
            {
                for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
                {
                    image->img.raw_byte[j + i * image->width
                                        + band*image->width*image->height] = 
                        ((unsigned char *) pBuffer)[k++];
                }
            }
        }
    }

    free( pBuffer );

    return 0;
}

/************************************************************************/
/*              msDrawRasterLayerGDAL_16BitClassifcation()              */
/*                                                                      */
/*      Handle the rendering of rasters going through a 16bit           */
/*      classification lookup instead of the more common 8bit one.      */
/*                                                                      */
/*      Currently we are using one data path where we load the          */
/*      raster into a floating point buffer, and then scale to          */
/*      16bit.  Eventually we could add optimizations for some of       */
/*      the 16bit cases at the cost of some complication.               */
/************************************************************************/

static int 
msDrawRasterLayerGDAL_16BitClassification(
    mapObj *map, layerObj *layer, imageObj *image, 
    GDALDatasetH hDS, GDALRasterBandH hBand,
    int src_xoff, int src_yoff, int src_xsize, int src_ysize,
    int dst_xoff, int dst_yoff, int dst_xsize, int dst_ysize )

{
    float *pafRawData;
    double dfScaleMin=0.0, dfScaleMax=0.0, dfScaleRatio;
    int   nPixelCount = dst_xsize * dst_ysize, i, nBucketCount=0;
    GDALDataType eDataType;
    float fDataMin=0.0, fDataMax=255.0, fNoDataValue;
    const char *pszScaleInfo;
    int  bUseIntegers = FALSE;
    int  *cmap, c, j, k, bGotNoData = FALSE, bGotFirstValue;
    gdImagePtr gdImg = image->img.gd;
    CPLErr eErr;

    assert( gdImg != NULL );

/* ==================================================================== */
/*      Read the requested data in one gulp into a floating point       */
/*      buffer.                                                         */
/* ==================================================================== */
    pafRawData = (float *) malloc(sizeof(float) * dst_xsize * dst_ysize );
    if( pafRawData == NULL )
    {
        msSetError( MS_MEMERR, "Out of memory allocating working buffer.",
                    "msDrawRasterLayerGDAL_16BitClassification()" );
        return -1;
    }
    
    eErr = GDALRasterIO( hBand, GF_Read, 
                         src_xoff, src_yoff, src_xsize, src_ysize, 
                         pafRawData, dst_xsize, dst_ysize, GDT_Float32, 0, 0 );
    
    if( eErr != CE_None )
    {
        free( pafRawData );
        msSetError( MS_IOERR, "GDALRasterIO() failed: %s", 
                    "msDrawRasterLayerGDAL_16BitClassification()",
                    CPLGetLastErrorMsg() );
        return -1;
    }

    fNoDataValue = (float) msGetGDALNoDataValue( layer, hBand, &bGotNoData );

/* ==================================================================== */
/*      Determine scaling.                                              */
/* ==================================================================== */
    eDataType = GDALGetRasterDataType( hBand );

/* -------------------------------------------------------------------- */
/*      Scan for absolute min/max of this block.                        */
/* -------------------------------------------------------------------- */
    bGotFirstValue = FALSE;
    
    for( i = 1; i < nPixelCount; i++ )
    {
        if( bGotNoData && pafRawData[i] == fNoDataValue )
            continue;

        if( !bGotFirstValue )
        {
            fDataMin = fDataMax = pafRawData[i];
            bGotFirstValue = TRUE;
        }
        else
        {
            fDataMin = MIN(fDataMin,pafRawData[i]);
            fDataMax = MAX(fDataMax,pafRawData[i]);
        }
    }

/* -------------------------------------------------------------------- */
/*      Fetch the scale processing option.                              */
/* -------------------------------------------------------------------- */
    pszScaleInfo = CSLFetchNameValue( layer->processing, "SCALE" );

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
            free( pafRawData );
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
/*      Special integer cases for scaling.                              */
/*                                                                      */
/*      TODO: Treat Int32 and UInt32 case the same way *if* the min     */
/*      and max are less than 65536 apart.                              */
/* -------------------------------------------------------------------- */
    if( eDataType == GDT_Byte || eDataType == GDT_Int16 
        || eDataType == GDT_UInt16 )
    {
        dfScaleMin = fDataMin - 0.5;
        dfScaleMax = fDataMax + 0.5;
        nBucketCount = (int) floor(fDataMax - fDataMin + 1.1);
        bUseIntegers = TRUE;
    }

/* -------------------------------------------------------------------- */
/*      General case if no scaling values provided in mapfile.          */
/* -------------------------------------------------------------------- */
    else if( dfScaleMin == 0.0 && dfScaleMax == 0.0 )
    {
        double dfEpsilon = (fDataMax - fDataMin) / (65536*2);
        dfScaleMin = fDataMin - dfEpsilon;
        dfScaleMax = fDataMax + dfEpsilon;
    }

/* -------------------------------------------------------------------- */
/*      How many buckets?  Only set if we don't already have a value.   */
/* -------------------------------------------------------------------- */
    if( nBucketCount == 0 )
    {
        const char *pszBuckets;

        pszBuckets = CSLFetchNameValue( layer->processing, "SCALE_BUCKETS" );
        if( pszBuckets == NULL )
            nBucketCount = 65536;
        else
        {
            nBucketCount = atoi(pszBuckets);
            if( nBucketCount < 2 )
            {
                free( pafRawData );
                msSetError( MS_MISCERR, 
                            "SCALE_BUCKETS PROCESSING option is not a value of 2 or more: %s.",
                            pszBuckets, 
                            "msDrawRasterLayerGDAL_16BitClassification()" );
                return -1;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Compute scaling ratio.                                          */
/* -------------------------------------------------------------------- */
    if( dfScaleMax == dfScaleMin )
        dfScaleMax = dfScaleMin + 1.0;

    dfScaleRatio = nBucketCount / (dfScaleMax - dfScaleMin);

    if( layer->debug > 0 )
        msDebug( "msDrawRasterGDAL_16BitClassification(%s):\n"
                 "  scaling to %d buckets from range=%g,%g.\n",
                 layer->name, nBucketCount, dfScaleMin, dfScaleMax );

/* ==================================================================== */
/*      Compute classification lookup table.                            */
/* ==================================================================== */

    cmap = (int *) calloc(sizeof(int),nBucketCount);

    for(i=0; i < nBucketCount; i++) 
    {
        double dfOriginalValue;

        cmap[i] = -1;

        dfOriginalValue = (i+0.5) / dfScaleRatio + dfScaleMin;
            
        c = msGetClass_Float(layer, (float) dfOriginalValue);
        if( c != -1 )
        {
            int s;

            /* change colour based on colour range? */
            for(s=0; s<layer->class[c].numstyles; s++)
            {
                if( MS_VALID_COLOR(layer->class[c].styles[s].mincolor)
                    && MS_VALID_COLOR(layer->class[c].styles[s].maxcolor) )
                    msValueToRange(&layer->class[c].styles[s],dfOriginalValue);
            }

            RESOLVE_PEN_GD(gdImg, layer->class[c].styles[0].color);
            if( MS_TRANSPARENT_COLOR(layer->class[c].styles[0].color) )
                cmap[i] = -1;
            else if( MS_VALID_COLOR(layer->class[c].styles[0].color))
            {
                /* use class color */
                cmap[i] = layer->class[c].styles[0].color.pen;
            }
        }
    }
    
/* ==================================================================== */
/*      Now process the data, applying to the working imageObj.         */
/* ==================================================================== */
    k = 0;

    for( i = dst_yoff; i < dst_yoff + dst_ysize; i++ )
    {
        int	result;
        
        for( j = dst_xoff; j < dst_xoff + dst_xsize; j++ )
        {
            float fRawValue = pafRawData[k++];
            int   iMapIndex;

            /* 
             * Skip nodata pixels ... no processing.
             */
            if( bGotNoData && fRawValue == fNoDataValue )
            {
                continue;
            }
            
            /*
             * The funny +1/-1 is to avoid odd rounding around zero.
             * We could use floor() but sometimes it is expensive. 
             */
            iMapIndex = (int) ((fRawValue - dfScaleMin) * dfScaleRatio+1)-1;

            if( iMapIndex >= nBucketCount || iMapIndex < 0 )
            {
                continue;
            }

            result = cmap[iMapIndex];
            if( result == -1 )
                continue;

            if( gdImageTrueColor( gdImg ) )
                gdImg->tpixels[i][j] = result;
            else
                gdImg->pixels[i][j] = result;
        }
    }

    free( pafRawData );
    free( cmap );

    assert( k == dst_xsize * dst_ysize );

    return 0;
}

/************************************************************************/
/*                          msGetGDALNoDataValue()                      */
/************************************************************************/
double 
msGetGDALNoDataValue( layerObj *layer, void *hBand, int *pbGotNoData )

{
    const char *pszNODATAOpt;

    *pbGotNoData = FALSE;

/* -------------------------------------------------------------------- */
/*      Check for a NODATA setting in the .map file.                    */
/* -------------------------------------------------------------------- */
    pszNODATAOpt = CSLFetchNameValue(layer->processing,"NODATA");
    if( pszNODATAOpt != NULL )
    {
        if( EQUAL(pszNODATAOpt,"OFF") || strlen(pszNODATAOpt) == 0 )
            return -1234567.0;
        if( !EQUAL(pszNODATAOpt,"AUTO") )
        {
            *pbGotNoData = TRUE;
            return atof(pszNODATAOpt);
        }
    }
    
/* -------------------------------------------------------------------- */
/*      Check for a NODATA setting on the raw file.                     */
/* -------------------------------------------------------------------- */
    if( hBand == NULL )
        return -1234567.0;
    else
        return GDALGetRasterNoDataValue( hBand, pbGotNoData );
}

/************************************************************************/
/*                         msGetGDALBandList()                          */
/*                                                                      */
/*      max_bands - pass zero for no limit.                             */
/*      band_count - location in which length of the return band        */
/*      list is placed.                                                 */
/*                                                                      */
/*      returns malloc() list of band numbers (*band_count long).       */
/************************************************************************/

int *msGetGDALBandList( layerObj *layer, void *hDS, 
                        int max_bands, int *band_count )

{
    int i, file_bands;
    int *band_list = NULL;

    file_bands = GDALGetRasterCount( hDS );
    if( file_bands == 0 )
    {
        msSetError( MS_IMGERR, 
                    "Attempt to operate on GDAL file with no bands, layer=%s.",
                    "msGetGDALBandList()", 
                    layer->name );
        
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Use all (or first) bands.                                       */
/* -------------------------------------------------------------------- */
    if( CSLFetchNameValue( layer->processing, "BANDS" ) == NULL )
    {
        if( max_bands > 0 )
            *band_count = MIN(file_bands,max_bands);
        else
            *band_count = file_bands;

        band_list = (int *) malloc(sizeof(int) * *band_count );
        for( i = 0; i < *band_count; i++ )
            band_list[i] = i+1;
        return band_list;
    }

/* -------------------------------------------------------------------- */
/*      get bands from BANDS processing directive.                      */
/* -------------------------------------------------------------------- */
    else
    {
        char **papszItems = CSLTokenizeStringComplex( 
            CSLFetchNameValue(layer->processing,"BANDS"), " ,", FALSE, FALSE );

        if( CSLCount(papszItems) == 0 )
        {
            CSLDestroy( papszItems );
            msSetError( MS_IMGERR, "BANDS PROCESSING directive has no items.",
                        "msGetGDALBandList()" );
            return NULL;
        }
        else if( max_bands != 0 && CSLCount(papszItems) > max_bands )
        {
            msSetError( MS_IMGERR, "BANDS PROCESSING directive has wrong number of bands, expected at most %d, got %d.",
                        "msGetGDALBandList()",
                        max_bands, CSLCount(papszItems) );
            CSLDestroy( papszItems );
            return NULL;
        }

        *band_count = CSLCount(papszItems);
        band_list = (int *) malloc(sizeof(int) * *band_count);

        for( i = 0; i < *band_count; i++ )
        {
            band_list[i] = atoi(papszItems[i]);
            if( band_list[i] < 1 || band_list[i] > GDALGetRasterCount(hDS) )
            {
                msSetError( MS_IMGERR, 
                            "BANDS PROCESSING directive includes illegal band '%s', should be from 1 to %d.", 
                            "msGetGDALBandList()",
                            papszItems[i], GDALGetRasterCount(hDS) );
                CSLDestroy( papszItems );
                CPLFree( band_list );
                return NULL;
            }
        }
        CSLDestroy( papszItems );

        return band_list;
    }
}

#endif /* def USE_GDAL */


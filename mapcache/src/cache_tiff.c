/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching: tiled tiff filesytem cache backend.
 * Author:   Thomas Bonfort, Frank Warmerdam and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 2011 Regents of the University of Minnesota.
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
 *****************************************************************************/

#ifdef USE_TIFF

#include "mapcache.h"
#include <apr_file_info.h>
#include <apr_strings.h>
#include <apr_file_io.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <tiffio.h>

#ifdef USE_GEOTIFF
#include "xtiffio.h"
#include "geovalues.h"
#include "geotiff.h"
#include "geo_normalize.h"
#include "geo_tiffp.h"
#include "geo_keyp.h"
#define MyTIFFOpen XTIFFOpen
#define MyTIFFClose XTIFFClose
#else
#define MyTIFFOpen TIFFOpen
#define MyTIFFClose TIFFClose
#endif


/**
 * \brief return filename for given tile
 * 
 * \param tile the tile to get the key from
 * \param path pointer to a char* that will contain the filename
 * \param r 
 * \private \memberof mapcache_cache_tiff
 */
static void _mapcache_cache_tiff_tile_key(mapcache_context *ctx, mapcache_tile *tile, char **path) {
   mapcache_cache_tiff *dcache = (mapcache_cache_tiff*)tile->tileset->cache;
   *path = dcache->filename_template;

   /*
    * generic template substitutions
    */
   if(strstr(*path,"{tileset}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{tileset}",
            tile->tileset->name);
   if(strstr(*path,"{grid}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{grid}",
            tile->grid_link->grid->name);
   if(tile->dimensions && strstr(*path,"{dim}")) {
      char *dimstring="";
      const apr_array_header_t *elts = apr_table_elts(tile->dimensions);
      int i = elts->nelts;
      while(i--) {
         apr_table_entry_t *entry = &(APR_ARRAY_IDX(elts,i,apr_table_entry_t));
         const char *dimval = mapcache_util_str_sanitize(ctx->pool,entry->val,"/.",'#');
         dimstring = apr_pstrcat(ctx->pool,dimstring,"#",dimval,NULL);
      }
      *path = mapcache_util_str_replace(ctx->pool,*path, "{dim}", dimstring);
   }
   
   
   while(strstr(*path,"{z}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{z}",
            apr_psprintf(ctx->pool,"%d",tile->z));
   /*
    * x and y replacing, when the tiff files are numbered with an increasing
    * x,y scheme (adjacent tiffs have x-x'=1 or y-y'=1
    */
   while(strstr(*path,"{div_x}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{div_x}",
            apr_psprintf(ctx->pool,"%d",tile->x/dcache->count_x));
   while(strstr(*path,"{div_y}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{div_y}",
            apr_psprintf(ctx->pool,"%d",tile->y/dcache->count_y));
   while(strstr(*path,"{inv_div_y}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_div_y}",
            apr_psprintf(ctx->pool,"%d",(tile->grid_link->grid->levels[tile->z]->maxy - tile->y - 1)/dcache->count_y));
   while(strstr(*path,"{inv_div_x}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_div_x}",
            apr_psprintf(ctx->pool,"%d",(tile->grid_link->grid->levels[tile->z]->maxx - tile->x - 1)/dcache->count_x));
   
   /*
    * x and y replacing, when the tiff files are numbered with the index
    * of their bottom-left tile
    * adjacent tiffs have x-x'=count_x or y-y'=count_y
    */
   while(strstr(*path,"{x}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{x}",
            apr_psprintf(ctx->pool,"%d",tile->x/dcache->count_x*dcache->count_x));
   while(strstr(*path,"{y}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{y}",
            apr_psprintf(ctx->pool,"%d",tile->y/dcache->count_y*dcache->count_y));
   while(strstr(*path,"{inv_y}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_y}",
            apr_psprintf(ctx->pool,"%d",(tile->grid_link->grid->levels[tile->z]->maxy - tile->y - 1)/dcache->count_y*dcache->count_y));
   while(strstr(*path,"{inv_x}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_x}",
            apr_psprintf(ctx->pool,"%d",(tile->grid_link->grid->levels[tile->z]->maxx - tile->x - 1)/dcache->count_x*dcache->count_y));
   if(!*path) {
      ctx->set_error(ctx,500, "failed to allocate tile key");
   }
}

#ifdef DEBUG
static void check_tiff_format(mapcache_context *ctx, mapcache_tile *tile, TIFF *hTIFF, const char *filename) {
   mapcache_cache_tiff *dcache = (mapcache_cache_tiff*)tile->tileset->cache;
   uint32 imwidth,imheight,tilewidth,tileheight;
   int16 planarconfig,orientation;
   uint16 compression;
   int rv;
   TIFFGetField( hTIFF, TIFFTAG_IMAGEWIDTH, &imwidth );
   TIFFGetField( hTIFF, TIFFTAG_IMAGELENGTH, &imheight );
   TIFFGetField( hTIFF, TIFFTAG_TILEWIDTH, &tilewidth );
   TIFFGetField( hTIFF, TIFFTAG_TILELENGTH, &tileheight );

   /* Test that the TIFF is tiled and not stripped */
   if(!TIFFIsTiled(hTIFF)) {
      ctx->set_error(ctx,500,"TIFF file \"%s\" is not tiled", filename);
      return;
   }
   
   /* check we have jpeg compression */
   rv = TIFFGetField( hTIFF, TIFFTAG_COMPRESSION, &compression );
   if(rv == 1 && compression != COMPRESSION_JPEG) {
      ctx->set_error(ctx,500,"TIFF file \"%s\" is not jpeg compressed",
            filename);
      return;
   }

   /* tiff must be pixel interleaved, not with a single image per band */
   rv = TIFFGetField( hTIFF, TIFFTAG_PLANARCONFIG, &planarconfig );
   if(rv == 1 && planarconfig != PLANARCONFIG_CONTIG) {
      ctx->set_error(ctx,500,"TIFF file \"%s\" is not pixel interleaved",
            filename);
      return;
   }
   
   /* is this test needed once we now we have JPEG ? */
   uint16 photometric;
   rv = TIFFGetField( hTIFF, TIFFTAG_PHOTOMETRIC, &photometric );
   if(rv == 1 && (photometric != PHOTOMETRIC_RGB && photometric != PHOTOMETRIC_YCBCR)) {
      ctx->set_error(ctx,500,"TIFF file \"%s\" is not RGB: %d",
            filename);
      return;
   }
   
   /* the default is top-left, but check just in case */
   rv = TIFFGetField( hTIFF, TIFFTAG_ORIENTATION, &orientation );
   if(rv == 1 && orientation != ORIENTATION_TOPLEFT) {
      ctx->set_error(ctx,500,"TIFF file \"%s\" is not top-left oriented",
            filename);
      return;
   }

   /* check that the tiff internal tiling aligns with the mapcache_grid we are using:
    * - the tiff tile size must match the grid tile size
    * - the number of tiles in each direction in the tiff must match what has been
    *   configured for the cache
    */
   mapcache_grid_level *level = tile->grid_link->grid->levels[tile->z];
   int ntilesx = MAPCACHE_MIN(dcache->count_x, level->maxx);
   int ntilesy = MAPCACHE_MIN(dcache->count_y, level->maxy);
   if( tilewidth != tile->grid_link->grid->tile_sx ||
         tileheight != tile->grid_link->grid->tile_sy ||
         imwidth != tile->grid_link->grid->tile_sx * ntilesx ||
         imheight != tile->grid_link->grid->tile_sy * ntilesy ) {
      ctx->set_error(ctx,500,"TIFF file %s imagesize (%d,%d) and tilesize (%d,%d).\
            Expected (%d,%d),(%d,%d)",filename,imwidth,imheight,tilewidth,tileheight,
            tile->grid_link->grid->tile_sx * ntilesx,
            tile->grid_link->grid->tile_sy * ntilesy,
            tile->grid_link->grid->tile_sx,
            tile->grid_link->grid->tile_sy);
      return;
   }

   /* TODO: more tests ? */

   /* ok, success */
}
#endif

static int _mapcache_cache_tiff_has_tile(mapcache_context *ctx, mapcache_tile *tile) {
   char *filename;
   TIFF *hTIFF;
   _mapcache_cache_tiff_tile_key(ctx, tile, &filename);
   mapcache_cache_tiff *dcache = (mapcache_cache_tiff*)tile->tileset->cache;
   if(GC_HAS_ERROR(ctx)) {
      return MAPCACHE_FALSE;
   }
   hTIFF = MyTIFFOpen(filename,"r");

   if(hTIFF) {
      do { 
         uint32 nSubType = 0;

         if( !TIFFGetField(hTIFF, TIFFTAG_SUBFILETYPE, &nSubType) )
            nSubType = 0;

         /* skip overviews and masks */
         if( (nSubType & FILETYPE_REDUCEDIMAGE) ||
               (nSubType & FILETYPE_MASK) )
            continue;


#ifdef DEBUG
         check_tiff_format(ctx,tile,hTIFF,filename);
         if(GC_HAS_ERROR(ctx)) {
            MyTIFFClose(hTIFF);
            return MAPCACHE_FALSE;
         }
#endif
         int tiff_offx, tiff_offy; /* the x and y offset of the tile inside the tiff image */
         int tiff_off; /* the index of the tile inside the list of tiles of the tiff image */
         
         mapcache_grid_level *level = tile->grid_link->grid->levels[tile->z];
         int ntilesx = MAPCACHE_MIN(dcache->count_x, level->maxx);
         int ntilesy = MAPCACHE_MIN(dcache->count_y, level->maxy);
         
         /* x offset of the tile along a row */
         tiff_offx = tile->x % ntilesx;

         /* 
          * y offset of the requested row. we inverse it as the rows are ordered
          * from top to bottom, whereas the tile y is bottom to top
          */
         tiff_offy = ntilesy - (tile->y % ntilesy) -1;
         tiff_off = tiff_offy * ntilesx + tiff_offx;
         
         toff_t  *offsets=NULL, *sizes=NULL;
         TIFFGetField( hTIFF, TIFFTAG_TILEOFFSETS, &offsets );
         TIFFGetField( hTIFF, TIFFTAG_TILEBYTECOUNTS, &sizes );
         MyTIFFClose(hTIFF);
         if( offsets[tiff_off] > 0 && sizes[tiff_off] > 0 ) {
            return MAPCACHE_TRUE;
         } else {
            return MAPCACHE_FALSE;
         }
      } while( TIFFReadDirectory( hTIFF ) );
      return MAPCACHE_FALSE; /* TIFF only contains overviews ? */
   }
   else
      return MAPCACHE_FALSE;
}

static void _mapcache_cache_tiff_delete(mapcache_context *ctx, mapcache_tile *tile) {
   ctx->set_error(ctx,500,"TIFF cache tile deleting not implemented");
}


/**
 * \brief get file content of given tile
 * 
 * fills the mapcache_tile::data of the given tile with content stored in the file
 * \private \memberof mapcache_cache_tiff
 * \sa mapcache_cache::tile_get()
 */
static int _mapcache_cache_tiff_get(mapcache_context *ctx, mapcache_tile *tile) {
   char *filename;
   TIFF *hTIFF;
   int rv;
   _mapcache_cache_tiff_tile_key(ctx, tile, &filename);
   mapcache_cache_tiff *dcache = (mapcache_cache_tiff*)tile->tileset->cache;
   if(GC_HAS_ERROR(ctx)) {
      return MAPCACHE_FALSE;
   }
#ifdef DEBUG
   ctx->log(ctx,MAPCACHE_DEBUG,"tile (%d,%d,%d) => filename %s)",
         tile->x,tile->y,tile->z,filename);
#endif
   
   hTIFF = MyTIFFOpen(filename,"r");

   /* 
    * we currrently have no way of knowing if the opening failed because the tif
    * file does not exist (which is not an error condition, as it only signals
    * that the requested tile does not exist in the cache), or if an other error
    * that should be signaled occured (access denied, not a tiff file, etc...)
    *
    * we ignore this case here and hope that further parts of the code will be
    * able to detect what's happening more precisely
    */

   if(hTIFF) {
      do { 
         uint32 nSubType = 0;

         if( !TIFFGetField(hTIFF, TIFFTAG_SUBFILETYPE, &nSubType) )
            nSubType = 0;

         /* skip overviews */
         if( nSubType & FILETYPE_REDUCEDIMAGE )
            continue;


#ifdef DEBUG
         check_tiff_format(ctx,tile,hTIFF,filename);
         if(GC_HAS_ERROR(ctx)) {
            MyTIFFClose(hTIFF);
            return MAPCACHE_FAILURE;
         }
#endif
         int tiff_offx, tiff_offy; /* the x and y offset of the tile inside the tiff image */
         int tiff_off; /* the index of the tile inside the list of tiles of the tiff image */
         
         /* 
          * compute the width and height of the full tiff file. This
          * is not simply the tile size times the number of tiles per
          * file for lower zoom levels
          */
         mapcache_grid_level *level = tile->grid_link->grid->levels[tile->z];
         int ntilesx = MAPCACHE_MIN(dcache->count_x, level->maxx);
         int ntilesy = MAPCACHE_MIN(dcache->count_y, level->maxy);
         
         /* x offset of the tile along a row */
         tiff_offx = tile->x % ntilesx;

         /* 
          * y offset of the requested row. we inverse it as the rows are ordered
          * from top to bottom, whereas the tile y is bottom to top
          */
         tiff_offy = ntilesy - (tile->y % ntilesy) -1;
         tiff_off = tiff_offy * ntilesx + tiff_offx;
         
         toff_t  *offsets=NULL, *sizes=NULL;
         
         /* get the offset of the jpeg data from the start of the file for each tile */
         rv = TIFFGetField( hTIFF, TIFFTAG_TILEOFFSETS, &offsets );
         if( rv != 1 ) {
            ctx->set_error(ctx,500,"Failed to read TIFF file \"%s\" tile offsets",
                  filename);
            MyTIFFClose(hTIFF);
            return MAPCACHE_FAILURE;
         }

         /* get the size of the jpeg data for each tile */
         rv = TIFFGetField( hTIFF, TIFFTAG_TILEBYTECOUNTS, &sizes );
         if( rv != 1 ) {
            ctx->set_error(ctx,500,"Failed to read TIFF file \"%s\" tile sizes",
                  filename);
            MyTIFFClose(hTIFF);
            return MAPCACHE_FAILURE;
         }

         /* 
          * the tile data exists for the given tiff_off if both offsets and size
          * are not zero for that index.
          * if not, the tiff file is sparse and is missing the requested tile
          */
         if( offsets[tiff_off] > 0 && sizes[tiff_off] > 0 ) {
            apr_file_t *f;
            apr_finfo_t finfo;
            apr_status_t ret;
            
            /* read the jpeg header (common to all tiles) */
            uint32 jpegtable_size = 0;
            unsigned char* jpegtable_ptr;
            rv = TIFFGetField( hTIFF, TIFFTAG_JPEGTABLES, &jpegtable_size, &jpegtable_ptr );
            if( rv != 1 || !jpegtable_ptr || !jpegtable_size) {
               /* there is no common jpeg header in the tiff tags */
               ctx->set_error(ctx,500,"Failed to read TIFF file \"%s\" jpeg table",
                     filename);
               MyTIFFClose(hTIFF);
               return MAPCACHE_FAILURE;
            }

            /* 
             * open the tiff file directly to access the jpeg image data with the given
             * offset
             */
            if((ret=apr_file_open(&f, filename, 
                        APR_FOPEN_READ|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,APR_OS_DEFAULT,
                        ctx->pool)) == APR_SUCCESS) {
               ret = apr_file_info_get(&finfo, APR_FINFO_MTIME, f);
               if(ret == APR_SUCCESS) {
                  /* 
                   * extract the file modification time. this isn't guaranteed to be the
                   * modification time of the actual tile, but it's the best we can do
                   */
                  tile->mtime = finfo.mtime;
               }
            
               void *bufptr;

               /* create a memory buffer to contain the jpeg data */
               tile->encoded_data = mapcache_buffer_create((jpegtable_size+sizes[tiff_off]-4),ctx->pool);

               /* 
                * copy the jpeg header to the beginning of the memory buffer,
                * omitting the last 2 bytes
                */
               memcpy(tile->encoded_data->buf,jpegtable_ptr,(jpegtable_size-2));

               /* advance the data pointer to after the header data */
               bufptr = tile->encoded_data->buf + (jpegtable_size-2);

               
               /* go to the specified offset in the tiff file, plus 2 bytes */
               apr_off_t off = offsets[tiff_off]+2;
               apr_file_seek(f,APR_SET,&off);

               /*
                * copy the jpeg body at the end of the memory buffer, accounting
                * for the two bytes we omitted in the previous step
                */
               apr_size_t bytes = sizes[tiff_off]-2;
               apr_file_read(f,bufptr,&bytes);

               /* check we have correctly read the requested number of bytes */
               if(bytes !=  sizes[tiff_off]-2) {
                  ctx->set_error(ctx,500,"failed to read jpeg body in \"%s\".\
                        (read %d of %d bytes)", filename,bytes,sizes[tiff_off]-2);
                  MyTIFFClose(hTIFF);
                  return MAPCACHE_FAILURE;
               }

               tile->encoded_data->size = (jpegtable_size+sizes[tiff_off]-4);

               /* finalize and cleanup */
               apr_file_close(f);
               MyTIFFClose(hTIFF);
               return MAPCACHE_SUCCESS;
            } else {
               /* shouldn't usually happen. we managed to open the file with TIFFOpen,
                * but apr_file_open failed to do so.
                * nothing much to do except bail out.
                */
               ctx->set_error(ctx,500,"apr_file_open failed on already open tiff file \"%s\", giving up .... ",
                     filename);
               MyTIFFClose(hTIFF);
               return MAPCACHE_FAILURE;
            }
         } else {
            /* sparse tiff file without the requested tile */
            MyTIFFClose(hTIFF);
            return MAPCACHE_CACHE_MISS;
         }
      } /* loop through the tiff directories if there are multiple ones */
      while( TIFFReadDirectory( hTIFF ) );

      /* 
       * should not happen?
       * finished looping through directories and didn't find anything suitable.
       * does the file only contain overviews?
       */
      MyTIFFClose(hTIFF);
      return MAPCACHE_CACHE_MISS;
   }
   /* failed to open tiff file */
   return MAPCACHE_CACHE_MISS;
}

/**
 * \brief write tile data to tiff
 * 
 * writes the content of mapcache_tile::data to tiff.
 * \returns MAPCACHE_FAILURE if there is no data to write, or if the tile isn't locked
 * \returns MAPCACHE_SUCCESS if the tile has been successfully written to tiff
 * \private \memberof mapcache_cache_tiff
 * \sa mapcache_cache::tile_set()
 */
static void _mapcache_cache_tiff_set(mapcache_context *ctx, mapcache_tile *tile) {
#ifdef USE_TIFF_WRITE
   char *filename;
   TIFF *hTIFF;
   int rv;
   int create;
   char errmsg[120];
   _mapcache_cache_tiff_tile_key(ctx, tile, &filename);
   mapcache_cache_tiff *dcache = (mapcache_cache_tiff*)tile->tileset->cache;
   mapcache_image_format_jpeg *format = (mapcache_image_format_jpeg*) dcache->format;
   if(GC_HAS_ERROR(ctx)) {
      return;
   }
#ifdef DEBUG
   ctx->log(ctx,MAPCACHE_DEBUG,"tile write (%d,%d,%d) => filename %s)",
         tile->x,tile->y,tile->z,filename);
#endif

   /*
    * create the directory where the tiff file will be stored
    */

   /* find the location of the last '/' in the string */
   char *hackptr1,*hackptr2;
   hackptr2 = hackptr1 = filename;
   while(*hackptr1) {
      if(*hackptr1 == '/')
         hackptr2 = hackptr1;
      hackptr1++;
   }
   *hackptr2 = '\0';
   
   if(APR_SUCCESS != (rv = apr_dir_make_recursive(filename,APR_OS_DEFAULT,ctx->pool))) {
       /* 
        * apr_dir_make_recursive sometimes sends back this error, although it should not.
        * ignore this one
        */
       if(!APR_STATUS_IS_EEXIST(rv)) {
          ctx->set_error(ctx, 500, "failed to create directory %s: %s",filename, apr_strerror(rv,errmsg,120));
          return;
       }
   }
   *hackptr2 = '/';
   
   int tilew = tile->grid_link->grid->tile_sx;
   int tileh = tile->grid_link->grid->tile_sy;
   
   if(!tile->raw_image) {
      tile->raw_image = mapcache_imageio_decode(ctx, tile->encoded_data);
      GC_CHECK_ERROR(ctx);
   }

   /* remap xrgb to rgb */
   unsigned char *rgb = (unsigned char*)malloc(tilew*tileh*3);
   int r,c;
   for(r=0;r<tile->raw_image->h;r++) {
      unsigned char *imptr = tile->raw_image->data + r * tile->raw_image->stride;
      unsigned char *rgbptr = rgb + r * tilew * 3;
      for(c=0;c<tile->raw_image->w;c++) {
         rgbptr[0] = imptr[2];
         rgbptr[1] = imptr[1];
         rgbptr[2] = imptr[0];
         rgbptr += 3;
         imptr += 4;
      }
   }

   /*
    * aquire a lock on the tiff file. This lock does not work for multiple threads of
    * a same process, but should work on network filesystems.
    * we previously aquired a thread lock so we should be ok here
    */

   while(mapcache_lock_or_wait_for_resource(ctx,filename) == MAPCACHE_FALSE);
   
   apr_file_t *ftiff;
   rv = apr_file_open(&ftiff,filename,APR_FOPEN_READ|APR_FOPEN_CREATE,APR_OS_DEFAULT,ctx->pool);
   if(rv != APR_SUCCESS) {
      ctx->set_error(ctx, 500,  "failed to remove existing file %s: %s",filename, apr_strerror(rv,errmsg,120));
      return; /* we could not delete the file */
   }

   apr_finfo_t finfo;
   rv = apr_file_info_get(&finfo, APR_FINFO_SIZE, ftiff);

   /*
    * check if the file exists by looking at its size
    */
   if(finfo.size) {
      hTIFF = MyTIFFOpen(filename,"r+");
      create = 0;
   } else {
      hTIFF = MyTIFFOpen(filename,"w+");
      create = 1;
   }
   if(!hTIFF) {
      ctx->set_error(ctx,500,"failed to open/create tiff file %s\n",filename);
      mapcache_unlock_resource(ctx,filename);
      apr_file_close(ftiff);
      return;
   }


   /* 
    * compute the width and height of the full tiff file. This
    * is not simply the tile size times the number of tiles per
    * file for lower zoom levels
    */
   mapcache_grid_level *level = tile->grid_link->grid->levels[tile->z];
   int ntilesx = MAPCACHE_MIN(dcache->count_x, level->maxx);
   int ntilesy = MAPCACHE_MIN(dcache->count_y, level->maxy);
   if(create) {
      /* populate the TIFF tags if we are creating the file */

      TIFFSetField( hTIFF, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT );
      TIFFSetField( hTIFF, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
      TIFFSetField( hTIFF, TIFFTAG_BITSPERSAMPLE, 8 );
      TIFFSetField( hTIFF, TIFFTAG_COMPRESSION, COMPRESSION_JPEG );
      TIFFSetField( hTIFF, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB );
      TIFFSetField( hTIFF, TIFFTAG_TILEWIDTH, tilew );
      TIFFSetField( hTIFF, TIFFTAG_TILELENGTH, tileh );
      TIFFSetField( hTIFF, TIFFTAG_IMAGEWIDTH, ntilesx * tilew );
      TIFFSetField( hTIFF, TIFFTAG_IMAGELENGTH, ntilesy * tileh );
      TIFFSetField( hTIFF, TIFFTAG_SAMPLESPERPIXEL,3 );

#ifdef USE_GEOTIFF
      double	adfPixelScale[3], adfTiePoints[6], bbox[4];

      GTIF *gtif = GTIFNew(hTIFF);
      if(gtif) {

         GTIFKeySet(gtif, GTRasterTypeGeoKey, TYPE_SHORT, 1,
               RasterPixelIsArea);

         GTIFKeySet( gtif, GeographicTypeGeoKey, TYPE_SHORT, 1, 
               0 );
         GTIFKeySet( gtif, GeogGeodeticDatumGeoKey, TYPE_SHORT,
               1, 0 );
         GTIFKeySet( gtif, GeogEllipsoidGeoKey, TYPE_SHORT, 1, 
               0 );
         GTIFKeySet( gtif, GeogSemiMajorAxisGeoKey, TYPE_DOUBLE, 1,
               0.0 );
         GTIFKeySet( gtif, GeogSemiMinorAxisGeoKey, TYPE_DOUBLE, 1,
               0.0 );
         switch(tile->grid_link->grid->unit) {
            case MAPCACHE_UNIT_FEET:
               GTIFKeySet( gtif, ProjLinearUnitsGeoKey, TYPE_SHORT, 1, 
                     Linear_Foot );
               break;
            case MAPCACHE_UNIT_METERS:
               GTIFKeySet( gtif, ProjLinearUnitsGeoKey, TYPE_SHORT, 1, 
                     Linear_Meter );
               break;
            case MAPCACHE_UNIT_DEGREES:
               GTIFKeySet(gtif, GeogAngularUnitsGeoKey, TYPE_SHORT, 0, 
                     Angular_Degree );
               break;
            default:
               break;
         }

         GTIFWriteKeys(gtif);
         GTIFFree(gtif);

         adfPixelScale[0] = adfPixelScale[1] = level->resolution;
         adfPixelScale[2] = 0.0;
         TIFFSetField( hTIFF, TIFFTAG_GEOPIXELSCALE, 3, adfPixelScale );


         /* top left tile x,y */
         int x,y;

         x = (tile->x / dcache->count_x)*(dcache->count_x); 
         y = (tile->y / dcache->count_y)*(dcache->count_y) + ntilesy - 1; 

         mapcache_grid_get_extent(ctx, tile->grid_link->grid,
               x,y,tile->z,bbox); 
         adfTiePoints[0] = 0.0;
         adfTiePoints[1] = 0.0;
         adfTiePoints[2] = 0.0;
         adfTiePoints[3] = bbox[0];
         adfTiePoints[4] = bbox[3];
         adfTiePoints[5] = 0.0;
         TIFFSetField( hTIFF, TIFFTAG_GEOTIEPOINTS, 6, adfTiePoints );
      }

#endif
   }
   TIFFSetField(hTIFF, TIFFTAG_JPEGQUALITY, format->quality); 
   if(format->photometric == MAPCACHE_PHOTOMETRIC_RGB) {
      TIFFSetField( hTIFF, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
   } else {
      TIFFSetField( hTIFF, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
   }
   TIFFSetField( hTIFF, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB );

   int tiff_offx, tiff_offy; /* the x and y offset of the tile inside the tiff image */
   int tiff_off; /* the index of the tile inside the list of tiles of the tiff image */

   /* x offset of the tile along a row */
   tiff_offx = tile->x % ntilesx;

   /* 
    * y offset of the requested row. we inverse it as the rows are ordered
    * from top to bottom, whereas the tile y is bottom to top
    */
   tiff_offy = ntilesy - (tile->y % ntilesy) -1;
   tiff_off = tiff_offy * ntilesx + tiff_offx;


   TIFFWriteEncodedTile(hTIFF, tiff_off, rgb, tilew*tileh*3);
   free(rgb);
   TIFFWriteCheck( hTIFF, 1, "cache_set()");

   if(create)
      TIFFWriteDirectory(hTIFF);
   TIFFFlush( hTIFF );


   MyTIFFClose(hTIFF);

   mapcache_unlock_resource(ctx,filename);
   apr_file_close(ftiff);
#else
   ctx->set_error(ctx,500,"tiff write support disabled by default");
#endif

}

/**
 * \private \memberof mapcache_cache_tiff
 */
static void _mapcache_cache_tiff_configuration_parse_xml(mapcache_context *ctx, ezxml_t node, mapcache_cache *cache, mapcache_cfg *config) {
   ezxml_t cur_node;
   mapcache_cache_tiff *dcache = (mapcache_cache_tiff*)cache;

   if ((cur_node = ezxml_child(node,"template")) != NULL) {
      dcache->filename_template = apr_pstrdup(ctx->pool,cur_node->txt);
   }
   ezxml_t xcount = ezxml_child(node,"xcount");
   if(xcount && xcount->txt && *xcount->txt) {
      char *endptr;
      dcache->count_x = (int)strtol(xcount->txt,&endptr,10);
      if(*endptr != 0) {
         ctx->set_error(ctx,400,"failed to parse xcount value %s for tiff cache %s", xcount->txt,cache->name);
         return;
      }
   }
   ezxml_t ycount = ezxml_child(node,"ycount");
   if(ycount && ycount->txt && *ycount->txt) {
      char *endptr;
      dcache->count_y = (int)strtol(ycount->txt,&endptr,10);
      if(*endptr != 0) {
         ctx->set_error(ctx,400,"failed to parse ycount value %s for tiff cache %s", ycount->txt,cache->name);
         return;
      }
   }
   ezxml_t xformat = ezxml_child(node,"format");
   char * format_name;
   if(xformat && xformat->txt && *xformat->txt) {
      format_name = xformat->txt;
   } else {
      format_name = "JPEG";
   }
   mapcache_image_format *pformat = mapcache_configuration_get_image_format(
         config,format_name);
   if(!pformat) {
      ctx->set_error(ctx,500,"TIFF cache %s references unknown image format %s",
            cache->name, format_name);
      return;
   }
   if(pformat->type != GC_JPEG) {
      ctx->set_error(ctx,500,"TIFF cache %s can only reference a JPEG image format",
            cache->name);
      return;
   }
   dcache->format = (mapcache_image_format_jpeg*)pformat;
}

/**
 * \private \memberof mapcache_cache_tiff
 */
static void _mapcache_cache_tiff_configuration_post_config(mapcache_context *ctx, mapcache_cache *cache,
      mapcache_cfg *cfg) {
   mapcache_cache_tiff *dcache = (mapcache_cache_tiff*)cache;
   /* check all required parameters are configured */
   if((!dcache->filename_template || !strlen(dcache->filename_template))) {
      ctx->set_error(ctx, 400, "tiff cache %s has no template pattern",dcache->cache.name);
      return;
   }
   if(dcache->count_x <= 0 || dcache->count_y <= 0) {
      ctx->set_error(ctx, 400, "tiff cache %s has invalid count (%d,%d)",dcache->count_x,dcache->count_y);
      return;
   }
}

/**
 * \brief creates and initializes a mapcache_tiff_cache
 */
mapcache_cache* mapcache_cache_tiff_create(mapcache_context *ctx) {
   mapcache_cache_tiff *cache = apr_pcalloc(ctx->pool,sizeof(mapcache_cache_tiff));
   if(!cache) {
      ctx->set_error(ctx, 500, "failed to allocate tiff cache");
      return NULL;
   }
   cache->cache.metadata = apr_table_make(ctx->pool,3);
   cache->cache.type = MAPCACHE_CACHE_TIFF;
   cache->cache.tile_delete = _mapcache_cache_tiff_delete;
   cache->cache.tile_get = _mapcache_cache_tiff_get;
   cache->cache.tile_exists = _mapcache_cache_tiff_has_tile;
   cache->cache.tile_set = _mapcache_cache_tiff_set;
   cache->cache.configuration_post_config = _mapcache_cache_tiff_configuration_post_config;
   cache->cache.configuration_parse_xml = _mapcache_cache_tiff_configuration_parse_xml;
   cache->count_x = 10;
   cache->count_y = 10;
#ifndef DEBUG
   TIFFSetWarningHandler(NULL);
   TIFFSetErrorHandler(NULL);
#endif
   return (mapcache_cache*)cache;
}

#endif

/* vim: ai ts=3 sts=3 et sw=3
*/

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
#include "xtiffio.h"
#endif


#ifdef USE_TIFF_WRITE
#if APR_HAS_THREADS
#include <apr_thread_mutex.h>
#define THREADLOCK_HASHARRAY_SIZE 64
static unsigned int _hash_key(char *key) {
  unsigned hashval;
  
  for(hashval=0; *key!='\0'; key++)
    hashval = *key + 31 * hashval;

  return(hashval % THREADLOCK_HASHARRAY_SIZE);
}
#endif
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
   if(strstr(*path,"{z}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{z}",
            apr_psprintf(ctx->pool,"%d",tile->z));
   if(tile->dimensions && strstr(*path,"{dim}")) {
      char *dimstring="";
      const apr_array_header_t *elts = apr_table_elts(tile->dimensions);
      int i = elts->nelts;
      while(i--) {
         apr_table_entry_t *entry = &(APR_ARRAY_IDX(elts,i,apr_table_entry_t));
         char *dimval = apr_pstrdup(ctx->pool,entry->val);
         char *iter = dimval;
         while(*iter) {
            /* replace dangerous characters by '#' */
            if(*iter == '.' || *iter == '/') {
               *iter = '#';
            }
            iter++;
         }
         dimstring = apr_pstrcat(ctx->pool,dimstring,"#",entry->key,"#",dimval,NULL);
      }
      *path = mapcache_util_str_replace(ctx->pool,*path, "{dim}", dimstring);
   }
   
   
   /*
    * x and y replacing, when the tiff files are numbered with an increasing
    * x,y scheme (adjacent tiffs have x-x'=1 or y-y'=1
    */
   if(strstr(*path,"{div_x}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{div_x}",
            apr_psprintf(ctx->pool,"%d",tile->x/dcache->count_x));
   if(strstr(*path,"{div_y}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{div_y}",
            apr_psprintf(ctx->pool,"%d",tile->y/dcache->count_y));
   if(strstr(*path,"{inv_div_y}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_div_y}",
            apr_psprintf(ctx->pool,"%d",(tile->grid_link->grid->levels[tile->z]->maxy - tile->y - 1)/dcache->count_y));
   if(strstr(*path,"{inv_div_x}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_div_x}",
            apr_psprintf(ctx->pool,"%d",(tile->grid_link->grid->levels[tile->z]->maxx - tile->x - 1)/dcache->count_x));
   
   /*
    * x and y replacing, when the tiff files are numbered with the index
    * of their bottom-left tile
    * adjacent tiffs have x-x'=count_x or y-y'=count_y
    */
   if(strstr(*path,"{x}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{x}",
            apr_psprintf(ctx->pool,"%d",tile->x/dcache->count_x*dcache->count_x));
   if(strstr(*path,"{y}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{y}",
            apr_psprintf(ctx->pool,"%d",tile->y/dcache->count_y*dcache->count_y));
   if(strstr(*path,"{inv_y}"))
      *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_y}",
            apr_psprintf(ctx->pool,"%d",(tile->grid_link->grid->levels[tile->z]->maxy - tile->y - 1)/dcache->count_y*dcache->count_y));
   if(strstr(*path,"{inv_x}"))
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
   hTIFF = TIFFOpen(filename,"r");

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
            TIFFClose(hTIFF);
            return MAPCACHE_FALSE;
         }
#endif
         int tiff_offx, tiff_offy; /* the x and y offset of the tile inside the tiff image */
         int tiff_off; /* the index of the tile inside the list of tiles of the tiff image */
         tiff_offx = tile->x % dcache->count_x;
         tiff_offy = dcache->count_y - tile->y % dcache->count_y - 1;
         tiff_off = tiff_offy * dcache->count_x + tiff_offx;
         toff_t  *offsets=NULL, *sizes=NULL;
         TIFFGetField( hTIFF, TIFFTAG_TILEOFFSETS, &offsets );
         TIFFGetField( hTIFF, TIFFTAG_TILEBYTECOUNTS, &sizes );
         TIFFClose(hTIFF);
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
   
   hTIFF = TIFFOpen(filename,"r");

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
            TIFFClose(hTIFF);
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
            TIFFClose(hTIFF);
            return MAPCACHE_FAILURE;
         }

         /* get the size of the jpeg data for each tile */
         rv = TIFFGetField( hTIFF, TIFFTAG_TILEBYTECOUNTS, &sizes );
         if( rv != 1 ) {
            ctx->set_error(ctx,500,"Failed to read TIFF file \"%s\" tile sizes",
                  filename);
            TIFFClose(hTIFF);
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
               TIFFClose(hTIFF);
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
               tile->data = mapcache_buffer_create((jpegtable_size+sizes[tiff_off]-4),ctx->pool);

               /* 
                * copy the jpeg header to the beginning of the memory buffer,
                * omitting the last 2 bytes
                */
               memcpy(tile->data->buf,jpegtable_ptr,(jpegtable_size-2));

               /* advance the data pointer to after the header data */
               bufptr = tile->data->buf + (jpegtable_size-2);

               
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
                  TIFFClose(hTIFF);
                  return MAPCACHE_FAILURE;
               }

               tile->data->size = (jpegtable_size+sizes[tiff_off]-4);

               /* finalize and cleanup */
               apr_file_close(f);
               TIFFClose(hTIFF);
               return MAPCACHE_SUCCESS;
            } else {
               /* shouldn't usually happen. we managed to open the file with TIFFOpen,
                * but apr_file_open failed to do so.
                * nothing much to do except bail out.
                */
               ctx->set_error(ctx,500,"apr_file_open failed on already open tiff file \"%s\", giving up .... ",
                     filename);
               TIFFClose(hTIFF);
               return MAPCACHE_FAILURE;
            }
         } else {
            /* sparse tiff file without the requested tile */
            TIFFClose(hTIFF);
            return MAPCACHE_CACHE_MISS;
         }
      } /* loop through the tiff directories if there are multiple ones */
      while( TIFFReadDirectory( hTIFF ) );

      /* 
       * should not happen?
       * finished looping through directories and didn't find anything suitable.
       * does the file only contain overviews?
       */
      TIFFClose(hTIFF);
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
   hackptr1 = filename;
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

#if APR_HAS_THREADS
   /*
    * aquire a thread lock for the filename. The filename is hashed to avoid all
    * threads locking here if they are to access different files
    */
   unsigned int threadkey;
   if(ctx->has_threads) {
      threadkey = _hash_key(filename);
      rv = apr_thread_mutex_lock(dcache->threadlocks[threadkey]);
      if(rv != APR_SUCCESS) {
         ctx->set_error(ctx,500,"failed to lock threadlock %d for file %s",threadkey,filename);
         return;
      }
   }
#endif

   /*
    * aquire a lock on the tiff file. This lock does not work for multiple threads of
    * a same process, but should work on network filesystems.
    * we previously aquired a thread lock so we should be ok here
    */
   apr_file_t *flock;
   rv = apr_file_open(&flock,filename,APR_FOPEN_READ|APR_FOPEN_CREATE,APR_OS_DEFAULT,ctx->pool);
   if(rv != APR_SUCCESS) {
      ctx->set_error(ctx, 500,  "failed to remove existing file %s: %s",filename, apr_strerror(rv,errmsg,120));
      return; /* we could not delete the file */
   }

   apr_file_lock(flock,APR_FLOCK_EXCLUSIVE); /* aquire the lock (this call is blocking) */
   apr_finfo_t finfo;
   rv = apr_file_info_get(&finfo, APR_FINFO_SIZE, flock);

   /*
    * check if the file exists by looking at its size
    */
   if(finfo.size) {
      hTIFF = TIFFOpen(filename,"r+");
      create = 0;
   } else {
      hTIFF = TIFFOpen(filename,"w+");
      create = 1;
   }
   if(!hTIFF) {
      ctx->set_error(ctx,500,"failed to open/create tiff file %s\n",filename);
      apr_file_unlock(flock);
      apr_file_close(flock);
      if(ctx->has_threads) {
         apr_thread_mutex_unlock(dcache->threadlocks[threadkey]);
      }
      return;
   }

   int tilew = tile->grid_link->grid->tile_sx;
   int tileh = tile->grid_link->grid->tile_sy;

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
      TIFFSetField( hTIFF, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB );
      TIFFSetField( hTIFF, TIFFTAG_BITSPERSAMPLE, 8 );
      TIFFSetField( hTIFF, TIFFTAG_COMPRESSION, COMPRESSION_JPEG );
      TIFFSetField( hTIFF, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB );
      TIFFSetField( hTIFF, TIFFTAG_TILEWIDTH, tilew );
      TIFFSetField( hTIFF, TIFFTAG_TILELENGTH, tileh );
      TIFFSetField( hTIFF, TIFFTAG_IMAGEWIDTH, ntilesx * tilew );
      TIFFSetField( hTIFF, TIFFTAG_IMAGELENGTH, ntilesy * tileh );
      TIFFSetField( hTIFF, TIFFTAG_SAMPLESPERPIXEL,3 );

#ifdef USE_GEOTIFF
      double	adfPixelScale[3], adfTiePoints[6];
#ifndef TIFFTAG_GEOPIXELSCALE
 #define TIFFTAG_GEOPIXELSCALE 33550
#endif
#ifndef TIFFTAG_GEOTIEPOINTS
 #define TIFFTAG_GEOTIEPOINTS 33922
#endif
      adfPixelScale[0] = adfPixelScale[1] = level->resolution;
      adfPixelScale[2] = 0.0;
      TIFFSetField( hTIFF, TIFFTAG_GEOPIXELSCALE, 3, adfPixelScale );

      adfTiePoints[0] = 0.0;
      adfTiePoints[1] = 0.0;
      adfTiePoints[2] = 0.0;
      adfTiePoints[3] = -20000000;
      adfTiePoints[4] = 20000000;
      adfTiePoints[5] = 0.0;
      TIFFSetField( hTIFF, TIFFTAG_GEOTIEPOINTS, 6, adfTiePoints );

      GTIF *psGTIF = GTIFNew( hTIFF );
      GTIFSetFromProj4( psGTIF, apr_psprintf(ctx->pool,"+init=%s",tile->grid_link->grid->srs) );
      GTIFWriteKeys( psGTIF );
      GTIFFree( psGTIF );
#endif
   }
   mapcache_image_format_jpeg *format = (mapcache_image_format_jpeg*) dcache->format;
   TIFFSetField(hTIFF, TIFFTAG_JPEGQUALITY, format->quality); 

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

   mapcache_image *tileimg = mapcache_imageio_decode(ctx, tile->data);

   /* remap xrgb to rgb */
   unsigned char *rgb = (unsigned char*)malloc(tilew*tileh*3);
   int r,c;
   for(r=0;r<tileimg->h;r++) {
      unsigned char *imptr = tileimg->data + r * tileimg->stride;
      unsigned char *rgbptr = rgb + r * tilew * 3;
      for(c=0;c<tileimg->w;c++) {
         rgbptr[0] = imptr[2];
         rgbptr[1] = imptr[1];
         rgbptr[2] = imptr[0];
         rgbptr += 3;
         imptr += 4;
      }
   }

   TIFFWriteEncodedTile(hTIFF, tiff_off, rgb, tilew*tileh*3);
   free(rgb);
   TIFFWriteCheck( hTIFF, 1, "cache_set()");

   if(create)
      TIFFWriteDirectory(hTIFF);
   TIFFFlush( hTIFF );


   TIFFClose(hTIFF);

   apr_file_unlock(flock);
   apr_file_close(flock);

#if APR_HAS_THREADS
   if(ctx->has_threads) {
      apr_thread_mutex_unlock(dcache->threadlocks[threadkey]);
   }
#endif
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

#ifdef USE_TIFF_WRITE
#if APR_HAS_THREADS
   if(ctx->has_threads) {
      /* create an array of thread locks */
      dcache->threadlocks = (apr_thread_mutex_t**)apr_pcalloc(ctx->pool,
            THREADLOCK_HASHARRAY_SIZE*sizeof(apr_thread_mutex_t*));
      int i;
      for(i=0;i<THREADLOCK_HASHARRAY_SIZE;i++) {
         apr_thread_mutex_create(&(dcache->threadlocks[i]),APR_THREAD_MUTEX_UNNESTED,ctx->pool);
      }
   }
#endif
#endif
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

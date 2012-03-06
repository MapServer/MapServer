/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching: filesytem cache backend.
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
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

#include "mapcache.h"
#include <apr_file_info.h>
#include <apr_strings.h>
#include <apr_file_io.h>
#include <string.h>
#include <errno.h>
#include <apr_mmap.h>

#ifdef HAVE_SYMLINK
#include <unistd.h>
#endif

static void _mapcache_cache_disk_blank_tile_key(mapcache_context *ctx, mapcache_tile *tile, unsigned char *color, char **path) {
   /* not implemented for template caches, as symlink_blank will never be set */
   *path = apr_psprintf(ctx->pool,"%s/%s/%s/blanks/%02X%02X%02X%02X.%s",
         ((mapcache_cache_disk*)tile->tileset->cache)->base_directory,
         tile->tileset->name,
         tile->grid_link->grid->name,
         color[0],
         color[1],
         color[2],
         color[3],
         tile->tileset->format?tile->tileset->format->extension:"png");
   if(!*path) {
      ctx->set_error(ctx,500, "failed to allocate blank tile key");
   }
}

/**
 * \brief return filename for given tile
 * 
 * \param tile the tile to get the key from
 * \param path pointer to a char* that will contain the filename
 * \param r 
 * \private \memberof mapcache_cache_disk
 */
static void _mapcache_cache_disk_tile_key(mapcache_context *ctx, mapcache_tile *tile, char **path) {
   mapcache_cache_disk *dcache = (mapcache_cache_disk*)tile->tileset->cache;
   if(dcache->base_directory) {
      char *start;
      start = apr_pstrcat(ctx->pool,
            dcache->base_directory,"/",
            tile->tileset->name,"/",
            tile->grid_link->grid->name,
            NULL);
      if(tile->dimensions) {
         const apr_array_header_t *elts = apr_table_elts(tile->dimensions);
         int i = elts->nelts;
         while(i--) {
            apr_table_entry_t *entry = &(APR_ARRAY_IDX(elts,i,apr_table_entry_t));
            const char *dimval = mapcache_util_str_sanitize(ctx->pool,entry->val,"/.",'#');
            start = apr_pstrcat(ctx->pool,start,"/",dimval,NULL);
         }
      }
      *path = apr_psprintf(ctx->pool,"%s/%02d/%03d/%03d/%03d/%03d/%03d/%03d.%s",
            start,
            tile->z,
            tile->x / 1000000,
            (tile->x / 1000) % 1000,
            tile->x % 1000,
            tile->y / 1000000,
            (tile->y / 1000) % 1000,
            tile->y % 1000,
            tile->tileset->format?tile->tileset->format->extension:"png");
   } else {
      *path = dcache->filename_template;
      *path = mapcache_util_str_replace(ctx->pool,*path, "{tileset}", tile->tileset->name);
      *path = mapcache_util_str_replace(ctx->pool,*path, "{grid}", tile->grid_link->grid->name);
      *path = mapcache_util_str_replace(ctx->pool,*path, "{ext}",
            tile->tileset->format?tile->tileset->format->extension:"png");
      if(strstr(*path,"{x}"))
         *path = mapcache_util_str_replace(ctx->pool,*path, "{x}",
               apr_psprintf(ctx->pool,"%d",tile->x));
      else
         *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_x}",
               apr_psprintf(ctx->pool,"%d",
                  tile->grid_link->grid->levels[tile->z]->maxx - tile->x - 1));
      if(strstr(*path,"{y}"))
         *path = mapcache_util_str_replace(ctx->pool,*path, "{y}",
               apr_psprintf(ctx->pool,"%d",tile->y));
      else
         *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_y}",
               apr_psprintf(ctx->pool,"%d",
                  tile->grid_link->grid->levels[tile->z]->maxy - tile->y - 1));
      if(strstr(*path,"{z}"))
         *path = mapcache_util_str_replace(ctx->pool,*path, "{z}",
               apr_psprintf(ctx->pool,"%d",tile->z));
      else
         *path = mapcache_util_str_replace(ctx->pool,*path, "{inv_z}",
               apr_psprintf(ctx->pool,"%d",
                  tile->grid_link->grid->nlevels - tile->z - 1));
      if(tile->dimensions) {
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
   }
   if(!*path) {
      ctx->set_error(ctx,500, "failed to allocate tile key");
   }
}

static int _mapcache_cache_disk_has_tile(mapcache_context *ctx, mapcache_tile *tile) {
   char *filename;
   apr_file_t *f;
   _mapcache_cache_disk_tile_key(ctx, tile, &filename);
   if(GC_HAS_ERROR(ctx)) {
      return MAPCACHE_FALSE;
   }
   if(apr_file_open(&f, filename, APR_FOPEN_READ,APR_OS_DEFAULT, ctx->pool) == APR_SUCCESS) {
      apr_file_close(f);
      return MAPCACHE_TRUE;
   }
   else
      return MAPCACHE_FALSE;
}

static void _mapcache_cache_disk_delete(mapcache_context *ctx, mapcache_tile *tile) {
   apr_status_t ret;
   char errmsg[120];
   char *filename;
   _mapcache_cache_disk_tile_key(ctx, tile, &filename);
   GC_CHECK_ERROR(ctx);

   ret = apr_file_remove(filename,ctx->pool);
   if(ret != APR_SUCCESS && !APR_STATUS_IS_ENOENT(ret)) {
      ctx->set_error(ctx, 500,  "failed to remove file %s: %s",filename, apr_strerror(ret,errmsg,120));
   }
}


/**
 * \brief get file content of given tile
 * 
 * fills the mapcache_tile::data of the given tile with content stored in the file
 * \private \memberof mapcache_cache_disk
 * \sa mapcache_cache::tile_get()
 */
static int _mapcache_cache_disk_get(mapcache_context *ctx, mapcache_tile *tile) {
   char *filename;
   apr_file_t *f;
   apr_finfo_t finfo;
   apr_status_t rv;
   apr_size_t size;
   apr_mmap_t *tilemmap;
     
   _mapcache_cache_disk_tile_key(ctx, tile, &filename);
   if(GC_HAS_ERROR(ctx)) {
      return MAPCACHE_FAILURE;
   }
   if((rv=apr_file_open(&f, filename, 
#ifndef NOMMAP
               APR_FOPEN_READ, APR_UREAD | APR_GREAD,
#else
               APR_FOPEN_READ|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,APR_OS_DEFAULT,
#endif
               ctx->pool)) == APR_SUCCESS) {
      rv = apr_file_info_get(&finfo, APR_FINFO_SIZE|APR_FINFO_MTIME, f);
      if(!finfo.size) {
         ctx->set_error(ctx, 500, "tile %s has no data",filename);
         return MAPCACHE_FAILURE;
      }

      size = finfo.size;
      /*
       * at this stage, we have a handle to an open file that contains data.
       * idealy, we should aquire a read lock, in case the data contained inside the file
       * is incomplete (i.e. if another process is currently writing to the tile).
       * currently such a lock is not set, as we don't want to loose performance on tile accesses.
       * any error that might happen at this stage should only occur if the tile isn't already cached,
       * i.e. normally only once.
       */
      tile->mtime = finfo.mtime;
      tile->encoded_data = mapcache_buffer_create(size,ctx->pool);

#ifndef NOMMAP

      rv = apr_mmap_create(&tilemmap,f,0,finfo.size,APR_MMAP_READ,ctx->pool);
      if(rv != APR_SUCCESS) {
         char errmsg[120];
         ctx->set_error(ctx, 500,  "mmap error: %s",apr_strerror(rv,errmsg,120));
         return MAPCACHE_FAILURE;
      }
      tile->encoded_data->buf = tilemmap->mm;
      tile->encoded_data->size = tile->encoded_data->avail = finfo.size;
#else
      //manually add the data to our buffer
      apr_file_read(f,(void*)tile->encoded_data->buf,&size);
      tile->encoded_data->size = size;
      tile->encoded_data->avail = size;
#endif
      apr_file_close(f);
      if(tile->encoded_data->size != finfo.size) {
         ctx->set_error(ctx, 500,  "failed to copy image data, got %d of %d bytes",(int)size, (int)finfo.size);
         return MAPCACHE_FAILURE;
      }
      return MAPCACHE_SUCCESS;
   } else {
      if(APR_STATUS_IS_ENOENT(rv)) {
         /* the file doesn't exist on the disk */
         return MAPCACHE_CACHE_MISS;
      } else {
            char *error = strerror(rv);
            ctx->set_error(ctx, 500,  "failed to open file %s: %s",filename, error);
            return MAPCACHE_FAILURE;
      }
   }
}

/**
 * \brief write tile data to disk
 * 
 * writes the content of mapcache_tile::data to disk.
 * \returns MAPCACHE_FAILURE if there is no data to write, or if the tile isn't locked
 * \returns MAPCACHE_SUCCESS if the tile has been successfully written to disk
 * \private \memberof mapcache_cache_disk
 * \sa mapcache_cache::tile_set()
 */
static void _mapcache_cache_disk_set(mapcache_context *ctx, mapcache_tile *tile) {
   apr_size_t bytes;
   apr_file_t *f;
   apr_status_t ret;
   char errmsg[120];
   char *filename, *hackptr1, *hackptr2=NULL;
   const int creation_retry = ((mapcache_cache_disk*)tile->tileset->cache)->creation_retry;

#ifdef DEBUG
   /* all this should be checked at a higher level */
   if(!tile->encoded_data && !tile->raw_image) {
      ctx->set_error(ctx,500,"attempting to write empty tile to disk");
      return;
   }
   if(!tile->encoded_data && !tile->tileset->format) {
      ctx->set_error(ctx,500,"received a raw tile image for a tileset with no format");
      return;
   }
#endif

   _mapcache_cache_disk_tile_key(ctx, tile, &filename);
   GC_CHECK_ERROR(ctx);

   /* find the location of the last '/' in the string */
   hackptr1 = filename;
   while(*hackptr1) {
      if(*hackptr1 == '/')
         hackptr2 = hackptr1;
      hackptr1++;
   }
   *hackptr2 = '\0';
   
   if(APR_SUCCESS != (ret = apr_dir_make_recursive(filename,APR_OS_DEFAULT,ctx->pool))) {
       /* 
        * apr_dir_make_recursive sometimes sends back this error, although it should not.
        * ignore this one
        */
       if(!APR_STATUS_IS_EEXIST(ret)) {
          ctx->set_error(ctx, 500, "failed to create directory %s: %s",filename, apr_strerror(ret,errmsg,120));
          return;
       }
   }
   *hackptr2 = '/';

   ret = apr_file_remove(filename,ctx->pool);
   if(ret != APR_SUCCESS && !APR_STATUS_IS_ENOENT(ret)) {
      ctx->set_error(ctx, 500,  "failed to remove file %s: %s",filename, apr_strerror(ret,errmsg,120));
   }

  
#ifdef HAVE_SYMLINK
   if(((mapcache_cache_disk*)tile->tileset->cache)->symlink_blank) {
      if(!tile->raw_image) {
         tile->raw_image = mapcache_imageio_decode(ctx, tile->encoded_data);
         GC_CHECK_ERROR(ctx);
      }
      if(mapcache_image_blank_color(tile->raw_image) != MAPCACHE_FALSE) {
         char *blankname;
         _mapcache_cache_disk_blank_tile_key(ctx,tile,tile->raw_image->data,&blankname);
         if(apr_file_open(&f, blankname, APR_FOPEN_READ, APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
            if(!tile->encoded_data) {
               tile->encoded_data = tile->tileset->format->write(ctx, tile->raw_image, tile->tileset->format);
               GC_CHECK_ERROR(ctx);
            }
            /* create the blank file */
            char *blankdirname = apr_psprintf(ctx->pool, "%s/%s/%s/blanks",
                        ((mapcache_cache_disk*)tile->tileset->cache)->base_directory,
                        tile->tileset->name,
                        tile->grid_link->grid->name);
            if(APR_SUCCESS != (ret = apr_dir_make_recursive(
                        blankdirname, APR_OS_DEFAULT,ctx->pool))) {
               if(!APR_STATUS_IS_EEXIST(ret)) {
                  ctx->set_error(ctx, 500,  "failed to create directory %s for blank tiles",blankdirname, apr_strerror(ret,errmsg,120));
                  return;
               }
            }

            /* aquire a lock on the blank file */
            int isLocked = mapcache_lock_or_wait_for_resource(ctx,blankname);

            if(isLocked == MAPCACHE_TRUE) {

               if((ret = apr_file_open(&f, blankname,
                           APR_FOPEN_CREATE|APR_FOPEN_WRITE|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
                           APR_OS_DEFAULT, ctx->pool)) != APR_SUCCESS) {
                  ctx->set_error(ctx, 500,  "failed to create file %s: %s",blankname, apr_strerror(ret,errmsg,120));
                  mapcache_unlock_resource(ctx,blankname);
                  return; /* we could not create the file */
               }

               bytes = (apr_size_t)tile->encoded_data->size;
               ret = apr_file_write(f,(void*)tile->encoded_data->buf,&bytes);
               if(ret != APR_SUCCESS) {
                  ctx->set_error(ctx, 500,  "failed to write data to file %s (wrote %d of %d bytes): %s",blankname, (int)bytes, (int)tile->encoded_data->size, apr_strerror(ret,errmsg,120));
                  mapcache_unlock_resource(ctx,blankname);
                  return; /* we could not create the file */
               }

               if(bytes != tile->encoded_data->size) {
                  ctx->set_error(ctx, 500,  "failed to write image data to %s, wrote %d of %d bytes", blankname, (int)bytes, (int)tile->encoded_data->size);
                  mapcache_unlock_resource(ctx,blankname);
                  return;
               }
               apr_file_close(f);
               mapcache_unlock_resource(ctx,blankname);
#ifdef DEBUG
               ctx->log(ctx,MAPCACHE_DEBUG,"created blank tile %s",blankname);
#endif
            }
         } else {
            apr_file_close(f);
         }

         int retry_count_create_symlink = 0;
         /*
          * depending on configuration symlink creation will retry if it fails.
          * this can happen on nfs mounted network storage.
          * the solution is to create the containing directory again and retry the symlink creation.
          */
         while(symlink(blankname,filename) != 0) {
            retry_count_create_symlink++;

            if(retry_count_create_symlink > creation_retry) {
               char *error = strerror(errno);
               ctx->set_error(ctx, 500, "failed to link tile %s to %s: %s",filename, blankname, error);
               return; /* we could not create the file */
            }

            *hackptr2 = '\0';

            if(APR_SUCCESS != (ret = apr_dir_make_recursive(filename,APR_OS_DEFAULT,ctx->pool))) {
               if(!APR_STATUS_IS_EEXIST(ret)) {
                  ctx->set_error(ctx, 500, "failed to create symlink, can not create directory %s: %s",filename, apr_strerror(ret,errmsg,120));
                  return; /* we could not create the file */
               }
            }

            *hackptr2 = '/';
         }
#ifdef DEBUG        
         ctx->log(ctx, MAPCACHE_DEBUG, "linked blank tile %s to %s",filename,blankname);
#endif
         return;
      }
   }
#endif /*HAVE_SYMLINK*/

   /* go the normal way: either we haven't configured blank tile detection, or the tile was not blank */
   
   if(!tile->encoded_data) {
      tile->encoded_data = tile->tileset->format->write(ctx, tile->raw_image, tile->tileset->format);
      GC_CHECK_ERROR(ctx);
   }

   int retry_count_create_file = 0;
   /*
    * depending on configuration file creation will retry if it fails.
    * this can happen on nfs mounted network storage.
    * the solution is to create the containing directory again and retry the file creation.
    */
   while((ret = apr_file_open(&f, filename,
         APR_FOPEN_CREATE|APR_FOPEN_WRITE|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
         APR_OS_DEFAULT, ctx->pool)) != APR_SUCCESS) {

      retry_count_create_file++;

      if(retry_count_create_file > creation_retry) {
         ctx->set_error(ctx, 500, "failed to create file %s: %s",filename, apr_strerror(ret,errmsg,120));
         return; /* we could not create the file */
      }

      *hackptr2 = '\0';

      if(APR_SUCCESS != (ret = apr_dir_make_recursive(filename,APR_OS_DEFAULT,ctx->pool))) {
         if(!APR_STATUS_IS_EEXIST(ret)) {
            ctx->set_error(ctx, 500, "failed to create file, can not create directory %s: %s",filename, apr_strerror(ret,errmsg,120));
            return; /* we could not create the file */
         }
      }

      *hackptr2 = '/';
   }

   bytes = (apr_size_t)tile->encoded_data->size;
   ret = apr_file_write(f,(void*)tile->encoded_data->buf,&bytes);
   if(ret != APR_SUCCESS) {
      ctx->set_error(ctx, 500,  "failed to write data to file %s (wrote %d of %d bytes): %s",filename, (int)bytes, (int)tile->encoded_data->size, apr_strerror(ret,errmsg,120));
      return; /* we could not create the file */
   }

   if(bytes != tile->encoded_data->size) {
      ctx->set_error(ctx, 500, "failed to write image data to %s, wrote %d of %d bytes", filename, (int)bytes, (int)tile->encoded_data->size);
   }
   ret = apr_file_close(f);
   if(ret != APR_SUCCESS) {
      ctx->set_error(ctx, 500,  "failed to close file %s:%s",filename, apr_strerror(ret,errmsg,120));
      return; /* we could not create the file */
   }

}

/**
 * \private \memberof mapcache_cache_disk
 */
static void _mapcache_cache_disk_configuration_parse_xml(mapcache_context *ctx, ezxml_t node, mapcache_cache *cache, mapcache_cfg *config) {
   ezxml_t cur_node;
   mapcache_cache_disk *dcache = (mapcache_cache_disk*)cache;

   if ((cur_node = ezxml_child(node,"base")) != NULL) {
      dcache->base_directory = apr_pstrdup(ctx->pool,cur_node->txt);
      /* we don't check for symlinking in the case where we are using a "template" cache type */
      if ((cur_node = ezxml_child(node,"symlink_blank")) != NULL) {
         if(strcasecmp(cur_node->txt,"false")){
#ifdef HAVE_SYMLINK
            dcache->symlink_blank = 1;
#else
            ctx->set_error(ctx,400,"cache %s: host system does not support file symbolic linking",cache->name);
            return;
#endif
         }
      }
   } else {
      if ((cur_node = ezxml_child(node,"template")) != NULL) {
         dcache->filename_template = apr_pstrdup(ctx->pool,cur_node->txt);
      }
   }

   if ((cur_node = ezxml_child(node,"creation_retry")) != NULL) {
      dcache->creation_retry = atoi(cur_node->txt);
   }
}

/**
 * \private \memberof mapcache_cache_disk
 */
static void _mapcache_cache_disk_configuration_post_config(mapcache_context *ctx, mapcache_cache *cache,
      mapcache_cfg *cfg) {
   mapcache_cache_disk *dcache = (mapcache_cache_disk*)cache;
   /* check all required parameters are configured */
   if((!dcache->base_directory || !strlen(dcache->base_directory)) &&
         (!dcache->filename_template || !strlen(dcache->filename_template))) {
      ctx->set_error(ctx, 400, "disk cache %s has no base directory or template",dcache->cache.name);
      return;
   }
}

/**
 * \brief creates and initializes a mapcache_disk_cache
 */
mapcache_cache* mapcache_cache_disk_create(mapcache_context *ctx) {
   mapcache_cache_disk *cache = apr_pcalloc(ctx->pool,sizeof(mapcache_cache_disk));
   if(!cache) {
      ctx->set_error(ctx, 500, "failed to allocate disk cache");
      return NULL;
   }
   cache->symlink_blank = 0;
   cache->creation_retry = 0;
   cache->cache.metadata = apr_table_make(ctx->pool,3);
   cache->cache.type = MAPCACHE_CACHE_DISK;
   cache->cache.tile_delete = _mapcache_cache_disk_delete;
   cache->cache.tile_get = _mapcache_cache_disk_get;
   cache->cache.tile_exists = _mapcache_cache_disk_has_tile;
   cache->cache.tile_set = _mapcache_cache_disk_set;
   cache->cache.configuration_post_config = _mapcache_cache_disk_configuration_post_config;
   cache->cache.configuration_parse_xml = _mapcache_cache_disk_configuration_parse_xml;
   return (mapcache_cache*)cache;
}

/* vim: ai ts=3 sts=3 et sw=3
*/

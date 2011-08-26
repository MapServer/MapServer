/*
 *  Copyright 2010 Thomas Bonfort
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "geocache.h"
#include <apr_file_info.h>
#include <apr_strings.h>
#include <apr_file_io.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_SYMLINK
#include <unistd.h>
#endif

static void _geocache_cache_disk_blank_tile_key(geocache_context *ctx, geocache_tile *tile, unsigned char *color, char **path) {
   *path = apr_psprintf(ctx->pool,"%s/%s/%s/blanks/%02X%02X%02X%02X.%s",
         ((geocache_cache_disk*)tile->tileset->cache)->base_directory,
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
 * \private \memberof geocache_cache_disk
 */
static void _geocache_cache_disk_tile_key(geocache_context *ctx, geocache_tile *tile, char **path) {
   char *start;
   start = apr_pstrcat(ctx->pool,
         ((geocache_cache_disk*)tile->tileset->cache)->base_directory,"/",
         tile->tileset->name,"/",
         tile->grid_link->grid->name,
         NULL);
   if(tile->dimensions) {
      const apr_array_header_t *elts = apr_table_elts(tile->dimensions);
      int i = elts->nelts;
      while(i--) {
         int lastisdot = 0;
         apr_table_entry_t *entry = &(APR_ARRAY_IDX(elts,i,apr_table_entry_t));
         char *iter = entry->val;
         while(*iter) {
            if(lastisdot) {
               if(*iter == '.') {
                  ctx->set_error(ctx,500,"invalid sequence .. in dimension %s",entry->key);
                  return;
               } else {
                  lastisdot = 0;
               }
            } else if(*iter == '.') {
               lastisdot = 1;
            }
            iter++;
         }
         start = apr_pstrcat(ctx->pool,start,"/",entry->key,"/",entry->val,NULL);
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
   if(!*path) {
      ctx->set_error(ctx,500, "failed to allocate tile key");
   }
  
}

static int _geocache_cache_disk_has_tile(geocache_context *ctx, geocache_tile *tile) {
   char *filename;
   apr_file_t *f;
   _geocache_cache_disk_tile_key(ctx, tile, &filename);
   if(GC_HAS_ERROR(ctx)) {
      return GEOCACHE_FALSE;
   }
   if(apr_file_open(&f, filename, APR_FOPEN_READ,APR_OS_DEFAULT, ctx->pool) == APR_SUCCESS)
       return GEOCACHE_TRUE;
   else
       return GEOCACHE_FALSE;
}

static void _geocache_cache_disk_delete(geocache_context *ctx, geocache_tile *tile) {
   apr_status_t ret;
   char errmsg[120];
   char *filename;
   apr_file_t *f;
   _geocache_cache_disk_tile_key(ctx, tile, &filename);
   GC_CHECK_ERROR(ctx);

   /* delete the tile file if it already exists */
   if((ret = apr_file_open(&f, filename,APR_FOPEN_READ,APR_OS_DEFAULT, ctx->pool)) == APR_SUCCESS) {
      apr_file_close(f);
      ret = apr_file_remove(filename,ctx->pool);
      if(ret != APR_SUCCESS) {
         ctx->set_error(ctx, 500,  "failed to remove existing file %s: %s",filename, apr_strerror(ret,errmsg,120));
         return; /* we could not delete the file */
      }
   } else {
      int code = 500;
      if(ret == ENOENT) {
         code = 404;
      }
      ctx->set_error(ctx, code,  "failed to remove file %s: %s",filename, apr_strerror(ret,errmsg,120));
      return; /* we could not open the file */
   }
}


/**
 * \brief get file content of given tile
 * 
 * fills the geocache_tile::data of the given tile with content stored in the file
 * \private \memberof geocache_cache_disk
 * \sa geocache_cache::tile_get()
 */
static int _geocache_cache_disk_get(geocache_context *ctx, geocache_tile *tile) {
   char *filename;
   apr_file_t *f;
   apr_finfo_t finfo;
   apr_status_t rv;
   apr_size_t size;
   _geocache_cache_disk_tile_key(ctx, tile, &filename);
   if(GC_HAS_ERROR(ctx)) {
      return GEOCACHE_FAILURE;
   }
   if((rv=apr_file_open(&f, filename, APR_FOPEN_READ|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
         APR_OS_DEFAULT, ctx->pool)) == APR_SUCCESS) {
      rv = apr_file_info_get(&finfo, APR_FINFO_SIZE|APR_FINFO_MTIME, f);
      if(!finfo.size) {
         ctx->set_error(ctx, 500, "tile %s has no data",filename);
         return GEOCACHE_FAILURE;
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
      tile->data = geocache_buffer_create(size,ctx->pool);
      //manually add the data to our buffer
      apr_file_read(f,(void*)tile->data->buf,&size);
      tile->data->size = size;
      apr_file_close(f);
      if(size != finfo.size) {
         ctx->set_error(ctx, 500,  "failed to copy image data, got %d of %d bytes",(int)size, (int)finfo.size);
         return GEOCACHE_FAILURE;
      }
      return GEOCACHE_SUCCESS;
   } else {
      if(rv == ENOENT) {
         /* the file doesn't exist on the disk */
         return GEOCACHE_CACHE_MISS;
      } else {
            char *error = strerror(rv);
            ctx->set_error(ctx, 500,  "failed to open file %s: %s",filename, error);
            return GEOCACHE_FAILURE;
      }
   }
}

/**
 * \brief write tile data to disk
 * 
 * writes the content of geocache_tile::data to disk.
 * \returns GEOCACHE_FAILURE if there is no data to write, or if the tile isn't locked
 * \returns GEOCACHE_SUCCESS if the tile has been successfully written to disk
 * \private \memberof geocache_cache_disk
 * \sa geocache_cache::tile_set()
 */
static void _geocache_cache_disk_set(geocache_context *ctx, geocache_tile *tile) {
   apr_size_t bytes;
   apr_file_t *f;
   apr_status_t ret;
   char errmsg[120];
   char *filename, *hackptr1, *hackptr2=NULL;
#ifdef DEBUG
   /* all this should be checked at a higher level */
   if(!tile->data || !tile->data->size) {
      ctx->set_error(ctx,500,"attempting to write empty tile to disk");
      return;
   }
#endif
   _geocache_cache_disk_tile_key(ctx, tile, &filename);
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
       ctx->set_error(ctx, 500, "failed to create directory %s: %s",filename, apr_strerror(ret,errmsg,120));
       return;
   }
   *hackptr2 = '/';

   /* delete the tile file if it already exists */
   if((ret = apr_file_open(&f, filename,APR_FOPEN_READ,APR_OS_DEFAULT, ctx->pool)) == APR_SUCCESS) {
      apr_file_close(f);
      ret = apr_file_remove(filename,ctx->pool);
      if(ret != APR_SUCCESS) {
         ctx->set_error(ctx, 500,  "failed to remove existing file %s: %s",filename, apr_strerror(ret,errmsg,120));
         return; /* we could not create the file */
      }
   }

  
#ifdef HAVE_SYMLINK
   if(((geocache_cache_disk*)tile->tileset->cache)->symlink_blank) {
      geocache_image *image = geocache_imageio_decode(ctx, tile->data);
      GC_CHECK_ERROR(ctx);
      if(geocache_image_blank_color(image) != GEOCACHE_FALSE) {
         char *blankname;
         _geocache_cache_disk_blank_tile_key(ctx,tile,image->data,&blankname);
         GC_CHECK_ERROR(ctx);
         ctx->global_lock_aquire(ctx);
         GC_CHECK_ERROR(ctx);
         if(apr_file_open(&f, blankname, APR_FOPEN_READ, APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
            /* create the blank file */
            char *blankdirname = apr_psprintf(ctx->pool, "%s/%s/%s/blanks",
                        ((geocache_cache_disk*)tile->tileset->cache)->base_directory,
                        tile->tileset->name,
                        tile->grid_link->grid->name);
            if(APR_SUCCESS != (ret = apr_dir_make_recursive(
                  blankdirname,
                  APR_OS_DEFAULT,ctx->pool))) {
               ctx->set_error(ctx, 500,  "failed to create directory %s for blank tiles",blankdirname, apr_strerror(ret,errmsg,120));
               ctx->global_lock_release(ctx);
               return;
            }
            if((ret = apr_file_open(&f, blankname,
                  APR_FOPEN_CREATE|APR_FOPEN_WRITE|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
                  APR_OS_DEFAULT, ctx->pool)) != APR_SUCCESS) {
               ctx->set_error(ctx, 500,  "failed to create file %s: %s",blankname, apr_strerror(ret,errmsg,120));
               ctx->global_lock_release(ctx);
               return; /* we could not create the file */
            }

            bytes = (apr_size_t)tile->data->size;
            ret = apr_file_write(f,(void*)tile->data->buf,&bytes);
            if(ret != APR_SUCCESS) {
               ctx->set_error(ctx, 500,  "failed to write data to file %s (wrote %d of %d bytes): %s",blankname, (int)bytes, (int)tile->data->size, apr_strerror(ret,errmsg,120));
               ctx->global_lock_release(ctx);
               return; /* we could not create the file */
            }

            if(bytes != tile->data->size) {
               ctx->set_error(ctx, 500,  "failed to write image data to %s, wrote %d of %d bytes", blankname, (int)bytes, (int)tile->data->size);
               ctx->global_lock_release(ctx);
               return;
            }
            apr_file_close(f);
#ifdef DEBUG
            ctx->log(ctx,GEOCACHE_DEBUG,"created blank tile %s",blankname);
#endif
         }
         ctx->global_lock_release(ctx);
         if(symlink(blankname,filename) != 0) {
            char *error = strerror(errno);
            ctx->set_error(ctx, 500,  "failed to link tile %s to %s: %s",filename, blankname, error);
            return; /* we could not create the file */
         }
#ifdef DEBUG        
         ctx->log(ctx, GEOCACHE_DEBUG, "linked blank tile %s to %s",filename,blankname);
#endif
         return;
      }
   }
#endif /*HAVE_SYMLINK*/

   /* go the normal way: either we haven't configured blank tile detection, or the tile was not blank */
   if((ret = apr_file_open(&f, filename,
         APR_FOPEN_CREATE|APR_FOPEN_WRITE|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
         APR_OS_DEFAULT, ctx->pool)) != APR_SUCCESS) {
      ctx->set_error(ctx, 500,  "failed to create file %s: %s",filename, apr_strerror(ret,errmsg,120));
      return; /* we could not create the file */
   }

   bytes = (apr_size_t)tile->data->size;
   ret = apr_file_write(f,(void*)tile->data->buf,&bytes);
   if(ret != APR_SUCCESS) {
      ctx->set_error(ctx, 500,  "failed to write data to file %s (wrote %d of %d bytes): %s",filename, (int)bytes, (int)tile->data->size, apr_strerror(ret,errmsg,120));
      return; /* we could not create the file */
   }

   if(bytes != tile->data->size) {
      ctx->set_error(ctx, 500, "failed to write image data to %s, wrote %d of %d bytes", filename, (int)bytes, (int)tile->data->size);
   }
   ret = apr_file_close(f);
   if(ret != APR_SUCCESS) {
      ctx->set_error(ctx, 500,  "failed to close file %s:%s",filename, apr_strerror(ret,errmsg,120));
      return; /* we could not create the file */
   }

}

/**
 * \private \memberof geocache_cache_disk
 */
static void _geocache_cache_disk_configuration_parse(geocache_context *ctx, ezxml_t node, geocache_cache *cache) {
   ezxml_t cur_node;
   geocache_cache_disk *dcache = (geocache_cache_disk*)cache;

   if ((cur_node = ezxml_child(node,"base")) != NULL) {
      dcache->base_directory = apr_pstrdup(ctx->pool,cur_node->txt);
   }
   
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
}
   
/**
 * \private \memberof geocache_cache_disk
 */
static void _geocache_cache_disk_configuration_check(geocache_context *ctx, geocache_cache *cache) {
   geocache_cache_disk *dcache = (geocache_cache_disk*)cache;
   /* check all required parameters are configured */
   if(!dcache->base_directory || !strlen(dcache->base_directory)) {
      ctx->set_error(ctx, 400, "disk cache %s has no base directory",dcache->cache.name);
      return;
   }
   
   apr_dir_t *basedir;
   apr_status_t rv;
   rv = apr_dir_open(&basedir,dcache->base_directory,ctx->pool);
   char errmsg[120];
   if(rv != APR_SUCCESS) {
      ctx->log(ctx,GEOCACHE_WARNING, "failed to open base directory %s for cache %s: %s."\
            " This might lead to problems later on if the apache user does not have the rights to create this directory",
            dcache->base_directory,cache->name,apr_strerror(rv,errmsg,120));
   }
   return;
}

/**
 * \brief creates and initializes a geocache_disk_cache
 */
geocache_cache* geocache_cache_disk_create(geocache_context *ctx) {
   geocache_cache_disk *cache = apr_pcalloc(ctx->pool,sizeof(geocache_cache_disk));
   if(!cache) {
      ctx->set_error(ctx, 500, "failed to allocate disk cache");
      return NULL;
   }
   cache->symlink_blank = 0;
   cache->cache.metadata = apr_table_make(ctx->pool,3);
   cache->cache.type = GEOCACHE_CACHE_DISK;
   cache->cache.tile_delete = _geocache_cache_disk_delete;
   cache->cache.tile_get = _geocache_cache_disk_get;
   cache->cache.tile_exists = _geocache_cache_disk_has_tile;
   cache->cache.tile_set = _geocache_cache_disk_set;
   cache->cache.configuration_check = _geocache_cache_disk_configuration_check;
   cache->cache.configuration_parse = _geocache_cache_disk_configuration_parse;
   return (geocache_cache*)cache;
}

/* vim: ai ts=3 sts=3 et sw=3
*/

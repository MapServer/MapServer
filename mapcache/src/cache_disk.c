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

#include "mapcache.h"
#include <apr_file_info.h>
#include <apr_strings.h>
#include <apr_file_io.h>
#include <string.h>
#include <errno.h>

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
            char *dimval = apr_pstrdup(ctx->pool,entry->val);
            char *iter = dimval;
            while(*iter) {
               /* replace dangerous characters by '#' */
               if(*iter == '.' || *iter == '/') {
                  *iter = '#';
               }
               iter++;
            }
            start = apr_pstrcat(ctx->pool,start,"/",entry->key,"/",dimval,NULL);
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
   apr_file_t *f;
   _mapcache_cache_disk_tile_key(ctx, tile, &filename);
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
   _mapcache_cache_disk_tile_key(ctx, tile, &filename);
   if(GC_HAS_ERROR(ctx)) {
      return MAPCACHE_FAILURE;
   }
   if((rv=apr_file_open(&f, filename, APR_FOPEN_READ|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
         APR_OS_DEFAULT, ctx->pool)) == APR_SUCCESS) {
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
      tile->data = mapcache_buffer_create(size,ctx->pool);
      //manually add the data to our buffer
      apr_file_read(f,(void*)tile->data->buf,&size);
      tile->data->size = size;
      apr_file_close(f);
      if(size != finfo.size) {
         ctx->set_error(ctx, 500,  "failed to copy image data, got %d of %d bytes",(int)size, (int)finfo.size);
         return MAPCACHE_FAILURE;
      }
      return MAPCACHE_SUCCESS;
   } else {
      if(rv == ENOENT) {
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
#ifdef DEBUG
   /* all this should be checked at a higher level */
   if(!tile->data || !tile->data->size) {
      ctx->set_error(ctx,500,"attempting to write empty tile to disk");
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

   /* delete the tile file if it already exists */
   if((ret = apr_file_open(&f, filename,APR_FOPEN_READ,APR_OS_DEFAULT, ctx->pool)) == APR_SUCCESS) {
      apr_file_close(f);
      ret = apr_file_remove(filename,ctx->pool);
      if(ret != APR_SUCCESS) {
         ctx->set_error(ctx, 500,  "failed to remove existing file %s: %s",filename, apr_strerror(ret,errmsg,120));
         return; /* we could not delete the file */
      }
   }

  
#ifdef HAVE_SYMLINK
   if(((mapcache_cache_disk*)tile->tileset->cache)->symlink_blank) {
      mapcache_image *image = mapcache_imageio_decode(ctx, tile->data);
      GC_CHECK_ERROR(ctx);
      if(mapcache_image_blank_color(image) != MAPCACHE_FALSE) {
         char *blankname;
         _mapcache_cache_disk_blank_tile_key(ctx,tile,image->data,&blankname);
         GC_CHECK_ERROR(ctx);
         ctx->global_lock_aquire(ctx);
         GC_CHECK_ERROR(ctx);
         if(apr_file_open(&f, blankname, APR_FOPEN_READ, APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
            /* create the blank file */
            char *blankdirname = apr_psprintf(ctx->pool, "%s/%s/%s/blanks",
                        ((mapcache_cache_disk*)tile->tileset->cache)->base_directory,
                        tile->tileset->name,
                        tile->grid_link->grid->name);
            if(APR_SUCCESS != (ret = apr_dir_make_recursive(
                  blankdirname,
                  APR_OS_DEFAULT,ctx->pool))) {
                  if(!APR_STATUS_IS_EEXIST(ret)) {
                     ctx->set_error(ctx, 500,  "failed to create directory %s for blank tiles",blankdirname, apr_strerror(ret,errmsg,120));
                     ctx->global_lock_release(ctx);
                     return;
                  }
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
            ctx->log(ctx,MAPCACHE_DEBUG,"created blank tile %s",blankname);
#endif
         } else {
            apr_file_close(f);
         }
         ctx->global_lock_release(ctx);
         if(symlink(blankname,filename) != 0) {
            char *error = strerror(errno);
            ctx->set_error(ctx, 500,  "failed to link tile %s to %s: %s",filename, blankname, error);
            return; /* we could not create the file */
         }
#ifdef DEBUG        
         ctx->log(ctx, MAPCACHE_DEBUG, "linked blank tile %s to %s",filename,blankname);
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
 * \private \memberof mapcache_cache_disk
 */
static void _mapcache_cache_disk_configuration_parse_xml(mapcache_context *ctx, ezxml_t node, mapcache_cache *cache) {
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
}

/**
 * \private \memberof mapcache_cache_disk
 */
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
static void _mapcache_cache_disk_configuration_parse_json(mapcache_context *ctx, cJSON *node, mapcache_cache *cache) {
   cJSON *tmp;
   mapcache_cache_disk *dcache = (mapcache_cache_disk*)cache;

   if((tmp = cJSON_GetObjectItem(node,"base_dir")) != NULL) {
      if(tmp->valuestring) {
         dcache->base_directory = apr_pstrdup(ctx->pool, tmp->valuestring);
      } else {
         ctx->set_error(ctx,400,"cache %s has invalid base_dir",cache->name);
         return;
      }
      if((tmp = cJSON_GetObjectItem(node,"symlink_blank")) != NULL) {
         if(tmp->valueint) {
#ifdef HAVE_SYMLINK
            dcache->symlink_blank = 1;
#else
            ctx->set_error(ctx,400,"cache %s: host system does not support file symbolic linking",cache->name);
            return;
#endif
         }
      }
   } else if((tmp = cJSON_GetObjectItem(node,"template")) != NULL) {
      if(tmp->valuestring) {
         dcache->filename_template = apr_pstrdup(ctx->pool, tmp->valuestring);
      } else {
         ctx->set_error(ctx,400,"cache %s has invalid template",cache->name);
         return;
      }
   }
}
#endif
   
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
   cache->cache.metadata = apr_table_make(ctx->pool,3);
   cache->cache.type = MAPCACHE_CACHE_DISK;
   cache->cache.tile_delete = _mapcache_cache_disk_delete;
   cache->cache.tile_get = _mapcache_cache_disk_get;
   cache->cache.tile_exists = _mapcache_cache_disk_has_tile;
   cache->cache.tile_set = _mapcache_cache_disk_set;
   cache->cache.configuration_post_config = _mapcache_cache_disk_configuration_post_config;
   cache->cache.configuration_parse_xml = _mapcache_cache_disk_configuration_parse_xml;
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
   cache->cache.configuration_parse_json = _mapcache_cache_disk_configuration_parse_json;
#endif
   return (mapcache_cache*)cache;
}

/* vim: ai ts=3 sts=3 et sw=3
*/

/*
 * disk_cache.c
 *
 *  Created on: Oct 10, 2010
 *      Author: tom
 */

#include "geocache.h"
#include <apr_file_info.h>
#include <apr_strings.h>
#include <apr_file_io.h>
#include <http_log.h>



char *_geocache_cache_disk_blank_tile_key(request_rec *r, geocache_tile *tile, unsigned char *color) {
   char *path = apr_psprintf(r->pool,"%s/%s/blanks/%02X%02X%02X%02X.%s",
         ((geocache_cache_disk*)tile->tileset->cache)->base_directory,
         tile->tileset->name,
         color[0],
         color[1],
         color[2],
         color[3],
         tile->tileset->format?tile->tileset->format->extension:"png");
   return path;
}
/**
 * \brief return filename for given tile
 * 
 * \param tile the tile to get the key from
 * \param path pointer to a char* that will contain the filename
 * \param r 
 * \private \memberof geocache_cache_disk
 */
int _geocache_cache_disk_tile_key(request_rec *r, geocache_tile *tile, char **path) {
   *path = apr_psprintf(r->pool,"%s/%s/%02d/%03d/%03d/%03d/%03d/%03d/%03d.%s",
         ((geocache_cache_disk*)tile->tileset->cache)->base_directory,
         tile->tileset->name,
         tile->z,
         tile->x / 1000000,
         (tile->x / 1000) % 1000,
         tile->x % 1000,
         tile->y / 1000000,
         (tile->y / 1000) % 1000,
         tile->y % 1000,
         tile->tileset->format?tile->tileset->format->extension:"png");
   return GEOCACHE_SUCCESS;
}

/**
 * \brief return directory path and filename for given tile
 * 
 * \param tile the tile to get the key from
 * \param path pointer to a char* that will contain the directory path
 * \param basename pointer to a char* that will contain the basename of the file (i.e. without the leading directory path) 
 * \param r 
 * \private \memberof geocache_cache_disk
 */
int _geocache_cache_disk_tile_key_split(request_rec *r, geocache_tile *tile, char **path, char **basename) {
   *path = apr_psprintf(r->pool,"%s/%s/%02d/%03d/%03d/%03d/%03d/%03d",
         ((geocache_cache_disk*)tile->tileset->cache)->base_directory,
         tile->tileset->name,
         tile->z,
         tile->x / 1000000,
         (tile->x / 1000) % 1000,
         tile->x % 1000,
         tile->y / 1000000,
         (tile->y / 1000) % 1000);
   *basename = apr_psprintf(r->pool,"%03d.%s",
         tile->y % 1000,
         tile->tileset->format?tile->tileset->format->extension:"png");
   return GEOCACHE_SUCCESS;
}

/**
 * \brief lock the given tile so other processes know it is being processed
 * 
 * this function creates a file with a .lck  extension and puts an exclusive lock on it
 * \sa geocache_cache::tile_lock()
 * \sa geocache_cache::tile_lock_exists()
 * \private \memberof geocache_cache_disk
 */
int _geocache_cache_disk_tile_lock(geocache_tile *tile, request_rec *r) {
   char *filename;
   char *basename;
   char *dirname;
   apr_file_t *f;
   apr_status_t rv;
   _geocache_cache_disk_tile_key_split(r, tile, &dirname, &basename);
   rv = apr_dir_make_recursive(dirname,APR_OS_DEFAULT,r->pool);
   if(rv != APR_SUCCESS) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to create directory %s",dirname);
      return GEOCACHE_FAILURE;
   }
   filename = apr_psprintf(r->pool,"%s/%s.lck",dirname,basename);
   /*create file, and fail if it already exists*/
   if(apr_file_open(&f, filename,
         APR_FOPEN_CREATE|APR_FOPEN_EXCL|APR_FOPEN_WRITE|APR_FOPEN_SHARELOCK|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
         APR_OS_DEFAULT, r->pool) != APR_SUCCESS) {
      /* 
       * opening failed, is this because we don't have write permissions, or because 
       * the file already exists?
       */
      if(apr_file_open(&f, filename, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, r->pool) != APR_SUCCESS) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to create file %s",filename);
         return GEOCACHE_FAILURE; /* we could not create the file */
      } else {
         ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "failed to create lockfile %s, because it already exists",filename);
         apr_file_close(f);
         return GEOCACHE_FILE_LOCKED; /* we have write access, but the file already exists */
      }
   }
   rv = apr_file_lock(f, APR_FLOCK_EXCLUSIVE|APR_FLOCK_NONBLOCK);
   if(rv != APR_SUCCESS) {
      if(rv == EAGAIN) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "####### TILE LOCK ######## file %s is already locked",filename);
      } else {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to lock file %s",filename);
      }
      return GEOCACHE_FAILURE;
   }
   tile->lock = f;
   return GEOCACHE_SUCCESS;
}

/**
 * \brief unlock a previously locked tile
 * 
 * \private \memberof geocache_cache_disk
 * \sa geocache_cache::tile_unlock()
 * \sa geocache_cache::tile_lock_exists()
 */
int _geocache_cache_disk_tile_unlock(geocache_tile *tile, request_rec *r) {
   apr_file_t *f = (apr_file_t*)tile->lock;
   const char *fname;
   if(!tile->lock) {
      char *filename;
      char *lockname;
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "###### TILE UNLOCK ######### attempting to unlock a tile not created by this thread");
      _geocache_cache_disk_tile_key(r, tile, &filename);
      lockname = apr_psprintf(r->pool,"%s.lck",filename);
      /*create file, and fail if it already exists*/
      if(apr_file_open(&f, lockname,APR_FOPEN_READ,APR_OS_DEFAULT, r->pool) != APR_SUCCESS) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "###### TILE UNLOCK ######### tile is not locked");
         return GEOCACHE_FAILURE;
      }
   } else {
      f = (apr_file_t*)tile->lock;
   }

   apr_file_name_get(&fname,f);
   apr_file_remove(fname,r->pool);

   int rv = apr_file_unlock(f);
   if(rv != APR_SUCCESS) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to unlock file");
      return GEOCACHE_FAILURE;
   }
   rv = apr_file_close(f);
   if(rv != APR_SUCCESS) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to close file");
      return GEOCACHE_FAILURE;
   }
   tile->lock = NULL;
   return rv;
}

/**
 * \brief query tile to check if the corresponding lockfile exists
 * \private \memberof geocache_cache_disk
 * \sa geocache_cache::tile_lock()
 * \sa geocache_cache::tile_unlock()
 */
int _geocache_cache_disk_tile_is_locked(geocache_tile *tile, request_rec *r) {
   char *filename;
   char *lockname;
   apr_file_t *f;
   if(tile->lock)
      return GEOCACHE_TRUE;
   _geocache_cache_disk_tile_key(r, tile, &filename);
   lockname = apr_psprintf(r->pool,"%s.lck",filename);
   if(apr_file_open(&f, lockname,APR_FOPEN_READ,APR_OS_DEFAULT, r->pool) != APR_SUCCESS) {
      return GEOCACHE_FALSE;
   } else {
      apr_file_close(f);
      return GEOCACHE_TRUE;
   }
}

/**
 * \brief wait for a lock on given tile
 * 
 * this function will not return until the lock on the tile has been removed
 * \private \memberof geocache_cache_disk
 * \sa geocache_cache::tile_lock_wait()
 */
int _geocache_cache_disk_tile_wait_for_lock(geocache_tile *tile, request_rec *r) {
   char *filename;
   char *lockname;
   apr_file_t *f;
#ifdef DEBUG
   if(tile->lock) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "### BUG ### waiting for a lock we have created ourself");
      return GEOCACHE_FAILURE;
   }
#endif
   _geocache_cache_disk_tile_key(r, tile, &filename);
   lockname = apr_psprintf(r->pool,"%s.lck",filename);
   if(apr_file_open(&f, lockname,APR_FOPEN_READ,APR_OS_DEFAULT, r->pool) != APR_SUCCESS) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "### BUG ### waiting for a lock on an unlocked file");
      return GEOCACHE_FAILURE;
   }
   apr_file_lock(f,APR_FLOCK_SHARED);
   apr_file_close(f);
   return GEOCACHE_SUCCESS;
}

/**
 * \brief get file content of given tile
 * 
 * fills the geocache_tile::data of the given tile with content stored in the file
 * \private \memberof geocache_cache_disk
 * \sa geocache_cache::tile_get()
 */
int _geocache_cache_disk_get(geocache_tile *tile, request_rec *r) {
   char *filename;
   apr_file_t *f;
   apr_finfo_t finfo;
   apr_status_t rv;
   apr_size_t size;
   _geocache_cache_disk_tile_key(r, tile, &filename);
   if(apr_file_open(&f, filename, APR_FOPEN_READ|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
         APR_OS_DEFAULT, r->pool) == APR_SUCCESS) {
      rv = apr_file_info_get(&finfo, APR_FINFO_SIZE|APR_FINFO_MTIME, f);
      if(!finfo.size) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "tile %s has no data",filename);
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
      tile->data = geocache_buffer_create(size,r->pool);
      //manually add the data to our buffer
      apr_file_read(f,(void*)tile->data->buf,&size);
      tile->data->size = size;
      apr_file_close(f);
      if(size != finfo.size) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to copy image data, got %d of %d bytes",(int)size, (int)finfo.size);
         return GEOCACHE_FAILURE;
      }
      return GEOCACHE_SUCCESS;
   }
   /* the file doesn't exist on the disk */
   return GEOCACHE_CACHE_MISS;
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
int _geocache_cache_disk_set(geocache_tile *tile, request_rec *r) {
   apr_size_t bytes;
   apr_file_t *f;
   char *filename;
#ifdef DEBUG
   /* all this should be checked at a higher level */
   if(!tile->data || !tile->data->size) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "attempting to write empty tile to disk");
      return GEOCACHE_FAILURE;
   }
   if(!tile->lock) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "attempting to write to an unlocked tile");
      return GEOCACHE_FAILURE;
   }
#endif
   _geocache_cache_disk_tile_key(r, tile, &filename);

   if(((geocache_cache_disk*)tile->tileset->cache)->symlink_blank) {
      geocache_image *image = geocache_imageio_decode(r, tile->data);
      if(geocache_image_blank_color(image) != GEOCACHE_FALSE) {
         char *blankname = _geocache_cache_disk_blank_tile_key(r,tile,image->data);
         if(apr_file_open(&f, blankname, APR_FOPEN_READ, APR_OS_DEFAULT, r->pool) != APR_SUCCESS) {
            /* create the blank file */
            if(APR_SUCCESS != apr_dir_make_recursive(apr_psprintf(r->pool, "%s/%s/blanks",((geocache_cache_disk*)tile->tileset->cache)->base_directory,tile->tileset->name),APR_OS_DEFAULT,r->pool)) {
               ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to create directory for blank tiles");
               return GEOCACHE_FAILURE;
            }
            if(apr_file_open(&f, blankname,
                  APR_FOPEN_CREATE|APR_FOPEN_WRITE|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
                  APR_OS_DEFAULT, r->pool) != APR_SUCCESS) {
               ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to create file %s",blankname);
               return GEOCACHE_FAILURE; /* we could not create the file */
            }
            
            bytes = (apr_size_t)tile->data->size;
            apr_file_write(f,(void*)tile->data->buf,&bytes);

            if(bytes != tile->data->size) {
               ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to write image data to disk, wrote %d of %d bytes",(int)bytes, (int)tile->data->size);
               return GEOCACHE_FAILURE;
            }
            apr_file_close(f);
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "created blank tile %s",blankname);
         }
         if(apr_file_link(blankname,filename) != GEOCACHE_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to link tile %s to %s",filename, blankname);
            return GEOCACHE_FAILURE; /* we could not create the file */
         }
         ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "linked blank tile %s to %s",filename,blankname);
         return GEOCACHE_SUCCESS;
      }
   }
   
   /* go the normal way: either we haven't configured blank tile detection, or the tile was not blank */
   if(apr_file_open(&f, filename,
         APR_FOPEN_CREATE|APR_FOPEN_WRITE|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
         APR_OS_DEFAULT, r->pool) != APR_SUCCESS) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to create file %s",filename);
      return GEOCACHE_FAILURE; /* we could not create the file */
   }

   bytes = (apr_size_t)tile->data->size;
   apr_file_write(f,(void*)tile->data->buf,&bytes);

   if(bytes != tile->data->size) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "failed to write image data to disk, wrote %d of %d bytes",(int)bytes, (int)tile->data->size);
      return GEOCACHE_FAILURE;
   }

   apr_file_close(f);
   return GEOCACHE_SUCCESS;
}

/**
 * \private \memberof geocache_cache_disk
 */
char* _geocache_cache_disk_configuration_parse(xmlNode *xml, geocache_cache *cache, apr_pool_t *pool) {
   xmlNode *cur_node;
   geocache_cache_disk *dcache = (geocache_cache_disk*)cache;
   for(cur_node = xml->children; cur_node; cur_node = cur_node->next) {
      if(cur_node->type != XML_ELEMENT_NODE) continue;
      if(!xmlStrcmp(cur_node->name, BAD_CAST "base")) {
         xmlChar* value = xmlNodeGetContent(cur_node);
         dcache->base_directory = (char*)value;
      }
      if(!xmlStrcmp(cur_node->name, BAD_CAST "symlink_blank")) {
         xmlChar* value = xmlNodeGetContent(cur_node);
         if(xmlStrcasecmp(value,BAD_CAST "false")) {
            dcache->symlink_blank = 1;
         }
      }
   }
   return NULL;
}

/**
 * \private \memberof geocache_cache_disk
 */
char* _geocache_cache_disk_configuration_check(geocache_cache *cache, apr_pool_t *pool) {
   apr_status_t status;
   geocache_cache_disk *dcache = (geocache_cache_disk*)cache;
   /* check all required parameters are configured */
   if(!dcache->base_directory || !strlen(dcache->base_directory)) {
      return apr_psprintf(pool,"disk cache %s has no base directory",dcache->cache.name);
   }
   
   /*create our directory for blank images*/
   status = apr_dir_make_recursive(dcache->base_directory,APR_OS_DEFAULT,pool);
   if(status != APR_SUCCESS) {
      return apr_psprintf(pool, "failed to create directory %s for cache %s",dcache->base_directory,dcache->cache.name );
   }
   
   /*win32 isn't buildable yet, but put this check in now for reminders*/
#ifdef _WIN32
   if(dcache->symlink_blank) {
      return apr_psprintf(pool, "linking blank tiles isn't supported on WIN32 due to platform limitations");
   }
#endif

   return NULL;
}

/**
 * \brief creates and initializes a geocache_disk_cache
 */
geocache_cache* geocache_cache_disk_create(apr_pool_t *pool) {
   geocache_cache_disk *cache = apr_pcalloc(pool,sizeof(geocache_cache_disk));
   cache->symlink_blank = 0;
   cache->cache.type = GEOCACHE_CACHE_DISK;
   cache->cache.tile_get = _geocache_cache_disk_get;
   cache->cache.tile_set = _geocache_cache_disk_set;
   cache->cache.configuration_check = _geocache_cache_disk_configuration_check;
   cache->cache.configuration_parse = _geocache_cache_disk_configuration_parse;
   cache->cache.tile_lock = _geocache_cache_disk_tile_lock;
   cache->cache.tile_unlock = _geocache_cache_disk_tile_unlock;
   cache->cache.tile_lock_exists = _geocache_cache_disk_tile_is_locked;
   cache->cache.tile_lock_wait = _geocache_cache_disk_tile_wait_for_lock;
   return (geocache_cache*)cache;
}


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
#include <apr_file_io.h>
#include <apr_strings.h>

#if THREADED_MPM
#include <unistd.h>
#endif


#ifdef USE_SEMLOCK
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
static const char *alphabet = "abcdefghijklmnopqrstuvwxyz"
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                       "0123456789"
                       "_";
static int encbase = 63; /* 26 + 26+ 10 + 1 */

static char* num_encode(apr_pool_t *pool, int num) {
   int n = num,r;
   int i = 0
   /* allocate 10 chars, should be amply sufficient:
    * yes, this *really* should be amply sufficient:
    * 10 chars, in 63 base = 10^63 different values.
    * the earth is 40000km wide, which means we will start having
    * a problem if a tile is less than 40000 * 1000 / 10^63 = 4e-56 meters wide*/;
   char *ret = apr_pcalloc(pool,10); 
   while(1) {
      r = n % encbase;
      n = n / encbase;
      ret[i] = alphabet[r];
      if(n==0) break;
      i++;
   }
   return ret;
}

#endif

char *mapcache_tileset_metatile_lock_key(mapcache_context *ctx, mapcache_metatile *mt) {
   char *lockname = apr_psprintf(ctx->pool,
#ifdef USE_SEMLOCK
         "/%s-%s-%s%s", /*x,y,z,tilesetname*/
         num_encode(ctx->pool,mt->x), num_encode(ctx->pool,mt->y),
         num_encode(ctx->pool,mt->z), 
#else
         "%s/"MAPCACHE_LOCKFILE_PREFIX"-%d-%d-%d-%s",
         ctx->config->lockdir,
         mt->z,mt->y,mt->x,
#endif
         mt->map.tileset->name);

   /* if the tileset has multiple grids, add the name of the current grid to the lock key*/
   if(mt->map.tileset->grid_links->nelts > 1) {
      lockname = apr_pstrcat(ctx->pool,lockname,mt->map.grid_link->grid->name,NULL);
   }

   if(mt->map.dimensions && !apr_is_empty_table(mt->map.dimensions)) {
      const apr_array_header_t *elts = apr_table_elts(mt->map.dimensions);
      int i;
      for(i=0;i<elts->nelts;i++) {
         apr_table_entry_t entry = APR_ARRAY_IDX(elts,i,apr_table_entry_t);
         char *dimvalue = apr_pstrdup(ctx->pool,entry.val);
         char *iter = dimvalue;
         while(*iter) {
            if(*iter == '/') *iter='_';
            iter++;
         }
         lockname = apr_pstrcat(ctx->pool,lockname,dimvalue,NULL);
      }

   }

#ifdef USE_SEMLOCK
#ifdef __APPLE__
#ifndef SEM_NAME_LEN
#define SEM_NAME_LEN 31
#endif
#endif

#ifdef SEM_NAME_LEN
   /* truncate the lockname to the number of allowed characters */
#warning "current platform only supports short semaphore names. you may see failed requests"
   if(strlen(lockname) >= SEM_NAME_LEN) {
      lockname[SEM_NAME_LEN-1]='\0';
   }
#endif
#endif

   return lockname;
}
/**
 * \brief lock the given metatile so other processes know it is being processed
 *
 * this function call is protected by a mutex
 *
 * this function creates a named semaphore and aquires a lock on it
 */
void mapcache_tileset_metatile_lock(mapcache_context *ctx, mapcache_metatile *mt) {
   char *lockname = mapcache_tileset_metatile_lock_key(ctx,mt);
#ifdef USE_SEMLOCK
   sem_t *lock;
   if ((lock = sem_open(lockname, O_CREAT|O_EXCL, 0644, 1)) == SEM_FAILED) {
      ctx->set_error(ctx,500, "failed to create posix semaphore %s: %s",lockname, strerror(errno));
      return;
   }
   sem_wait(lock);
   mt->lock = lock;
#else
   char errmsg[120];
   apr_file_t *lock;
   apr_status_t rv;
   rv = apr_file_open(&lock,lockname,APR_WRITE|APR_CREATE|APR_EXCL,APR_OS_DEFAULT,ctx->pool);
   if(rv!=APR_SUCCESS) {
      ctx->set_error(ctx,500, "failed to create lockfile %s: %s",
            lockname, apr_strerror(rv,errmsg,120));
      return;
   }
#if THREADED_MPM
#else
   /* we aquire an exclusive (i.e. write) lock on the file. For this to work, the file has to be opened with APR_WRITE mode */
   rv = apr_file_lock(lock,APR_FLOCK_EXCLUSIVE|APR_FLOCK_NONBLOCK);
   if(rv!=APR_SUCCESS) {
      ctx->set_error(ctx,500, "failed to aquire an exclusive lock on lockfile %s: %s",
            lockname, apr_strerror(rv,errmsg,120));
      return;
   }
#endif
   mt->lock = lock;
#endif
}

/**
 * \brief unlock a previously locked tile
 *
 * this function call is protected by a mutex
 *
 * \sa mapcache_cache::tile_unlock()
 * \sa mapcache_cache::tile_lock_exists()
 */
void mapcache_tileset_metatile_unlock(mapcache_context *ctx, mapcache_metatile *mt) {
   const char *lockname = mapcache_tileset_metatile_lock_key(ctx,mt);
#ifdef USE_SEMLOCK
   sem_t *lock = (sem_t*) mt->lock;
   if (!mt->lock) {
      ctx->set_error(ctx,500,"###### TILE UNLOCK ######### attempting to unlock tile %s not created by this thread", lockname);
      return;
   }
   sem_post(lock);
   sem_close(lock);
   sem_unlink(lockname);
   mt->lock = NULL;
#else
   char errmsg[120];
   apr_status_t rv;
   if (!mt->lock) {
      ctx->set_error(ctx,500,"###### TILE UNLOCK ######### attempting to unlock tile %s not created by this thread", lockname);
      return;
   }
   apr_file_t *lock = (apr_file_t*)mt->lock;
   mt->lock = NULL;
#if THREADED_MPM
#else
   rv = apr_file_unlock(lock);
   if(rv!=APR_SUCCESS) {
      ctx->set_error(ctx,500, "failed to unlock lockfile %s: %s",
            lockname, apr_strerror(rv,errmsg,120));
   }
#endif
   rv = apr_file_close(lock);
   if(rv!=APR_SUCCESS) {
      ctx->set_error(ctx,500, "failed to close lockfile %s: %s",
            lockname, apr_strerror(rv,errmsg,120));
   }
   rv = apr_file_remove(lockname,ctx->pool);
   if(rv!=APR_SUCCESS) {
      ctx->set_error(ctx,500, "failed to remove lockfile %s: %s",
            lockname, apr_strerror(rv,errmsg,120));
   }
#endif
}

/**
 * \brief query tile to check if the corresponding lockfile exists
 * 
 * this function call is protected by a mutex
 *
 * \sa mapcache_cache::tile_lock()
 * \sa mapcache_cache::tile_unlock()
 */
int mapcache_tileset_metatile_lock_exists(mapcache_context *ctx, mapcache_metatile *mt) {
   char *lockname;
   if (mt->lock)
      return MAPCACHE_TRUE;
   lockname = mapcache_tileset_metatile_lock_key(ctx, mt);
#ifdef USE_SEMLOCK
   sem_t *lock = sem_open(lockname, 0, 0644, 1) ;
   if (lock == SEM_FAILED) {
      if(errno == ENOENT) {
         return MAPCACHE_FALSE;
      } else {
         ctx->set_error(ctx,500,"lock_exists: failed to open mutex %s",lockname);
         return MAPCACHE_FALSE;
      }
   } else {
      sem_close(lock);
      return MAPCACHE_TRUE;
   }
#else
   apr_status_t rv;
   char errmsg[120];
   apr_file_t *lock;
   rv = apr_file_open(&lock,lockname,APR_READ,APR_OS_DEFAULT,ctx->pool);
   if(APR_STATUS_IS_ENOENT(rv)) {
      /* the lock file does not exist, the metatile is not locked */
      return MAPCACHE_FALSE;
   } else {
      /* the file exists, or there was an error opening it */
      if(rv != APR_SUCCESS) {
         /* we failed to open the file, bail out */
         ctx->set_error(ctx,500,"lock_exists: failed to open lockfile %s: %s",lockname,
               apr_strerror(rv,errmsg,120));
         return MAPCACHE_FALSE;
      } else {
#if THREADED_MPM
         apr_file_close(lock);
         return MAPCACHE_TRUE;
#else
         /* the file exists, which means there probably is a lock. make sure it isn't locked anyhow */
         rv = apr_file_lock(lock,APR_FLOCK_SHARED|APR_FLOCK_NONBLOCK);
         if(rv != APR_SUCCESS) {
            if(rv == APR_EAGAIN) {
               /* the file is locked */
               apr_file_close(lock);
               return MAPCACHE_TRUE;
            } else {
               ctx->set_error(ctx,500, "lock_exists: failed to aquire lock on lockfile %s: %s",
                        lockname, apr_strerror(rv,errmsg,120));
               rv = apr_file_close(lock);
               if(rv!=APR_SUCCESS) {
                  ctx->set_error(ctx,500, "lock_exists: failed to close lockfile %s: %s",
                        lockname, apr_strerror(rv,errmsg,120));
               }
               return MAPCACHE_FALSE;
            }
         } else {
            /* we managed to aquire a lock on the file, which means it isn't locked, even if it exists */
            /* unlock the file */
            rv = apr_file_unlock(lock);
            if(rv!=APR_SUCCESS) {
               ctx->set_error(ctx,500, "lock_exists: failed to unlock lockfile %s: %s",
                     lockname, apr_strerror(rv,errmsg,120));
            }
            rv = apr_file_close(lock);
            if(rv!=APR_SUCCESS) {
               ctx->set_error(ctx,500, "lock_exists: failed to close lockfile %s: %s",
                     lockname, apr_strerror(rv,errmsg,120));
            }
            //TODO: remove the file as we will fail later if not
            return MAPCACHE_FALSE;
         }
#endif
      }
   }
#endif
}

/**
 * \brief wait for a lock on given tile
 *
 * this function will not return until the lock on the tile has been removed
 * \sa mapcache_cache::tile_lock_wait()
 */
void mapcache_tileset_metatile_lock_wait(mapcache_context *ctx, mapcache_metatile *mt) {
  char *lockname;
  lockname = mapcache_tileset_metatile_lock_key(ctx, mt);

#ifdef DEBUG
  if (mt->lock) {
    ctx->set_error(ctx, 500, "### BUG ### lock_wait: waiting for a lock we have created ourself");
    return;
  }
#endif

#ifdef USE_SEMLOCK
  sem_t *lock;
  lock = sem_open(lockname, 0, 0644, 1) ;
  if( lock == SEM_FAILED ) {
     if(errno == ENOENT) {
        /* the lock doesn't exist (anymore?) */
        return; 
     } else {
        ctx->set_error(ctx,500, "lock_wait: failed to open semaphore %s",lockname);
        return;
     }
  } else {
     sem_wait(lock);
     sem_post(lock);
     sem_close(lock);
  }
#else
   char errmsg[120];
   apr_file_t *lock;
   apr_status_t rv;
   rv = apr_file_open(&lock,lockname,APR_READ,APR_OS_DEFAULT,ctx->pool);
   if(rv != APR_SUCCESS) {
      if(APR_STATUS_IS_ENOENT(rv)) {
         /* the lock file does not exist, it might have been removed since we checked.
          * anyhow, there's no use waiting anymore
          */
         return;
      } else {
         ctx->set_error(ctx,500, "lock_wait: failed to open lockfile %s: %s",
               lockname, apr_strerror(rv,errmsg,120));
         return;
      }
   }
#if THREADED_MPM
   apr_file_close(lock);
   while(!APR_STATUS_IS_ENOENT(rv)) {
      usleep(500000);
      rv = apr_file_open(&lock,lockname,APR_READ,APR_OS_DEFAULT,ctx->pool);
      if(rv == APR_SUCCESS) {
         apr_file_close(lock);
      }
   }
#else
   rv = apr_file_lock(lock,APR_FLOCK_SHARED);
   if(rv!=APR_SUCCESS) {
      ctx->set_error(ctx,500, "lock_wait: failed to aquire a shared lock on lockfile %s: %s",
            lockname, apr_strerror(rv,errmsg,120));
   }
   rv = apr_file_close(lock);
   if(rv!=APR_SUCCESS) {
      ctx->set_error(ctx,500, "lock_wait: failed to close lockfile %s: %s",
            lockname, apr_strerror(rv,errmsg,120));
   }
#endif

#endif
}

/* vim: ai ts=3 sts=3 et sw=3
*/

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
#include <apr_file_io.h>
#include <apr_strings.h>
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
   int i = 0;
   char *ret = apr_pcalloc(pool,10); /* allocate 10 chars, should be amply sufficient */
   while(1) {
      r = n % encbase;
      n = n / encbase;
      ret[i] = alphabet[r];
      if(n==0) break;
      i++;
   }
   return ret;
}

char *geocache_tileset_tile_lock_key(geocache_context *ctx, geocache_tile *tile) {
  return apr_psprintf(ctx->pool,
          "/%s%s-%s-%s",
          tile->tileset->name,
          num_encode(ctx->pool,tile->z), num_encode(ctx->pool,tile->y), num_encode(ctx->pool,tile->x));
}
/**
 * \brief lock the given tile so other processes know it is being processed
 *
 * this function call is protected by a mutex
 *
 * this function creates a named semaphore and aquires a lock on it
 * \sa geocache_cache::tile_lock()
 * \sa geocache_cache::tile_lock_exists()
 * \private \memberof geocache_cache_disk
 */
void geocache_tileset_tile_lock(geocache_context *ctx, geocache_tile *tile) {
   char *lockname = geocache_tileset_tile_lock_key(ctx,tile);
   sem_t *lock;
   if ((lock = sem_open(lockname, O_CREAT|O_EXCL, 0644, 1)) == SEM_FAILED) {
      ctx->set_error(ctx,500, "failed to create posix semaphore %s: %s",lockname, strerror(errno));
      return;
   }
   sem_wait(lock);
   tile->lock = lock;
}

/**
 * \brief unlock a previously locked tile
 *
 * this function call is protected by a mutex
 *
 * \sa geocache_cache::tile_unlock()
 * \sa geocache_cache::tile_lock_exists()
 */
void geocache_tileset_tile_unlock(geocache_context *ctx, geocache_tile *tile) {
   sem_t *lock = (sem_t*) tile->lock;
   const char *lockname = geocache_tileset_tile_lock_key(ctx,tile);
   if (!tile->lock) {
      ctx->set_error(ctx,500,"###### TILE UNLOCK ######### attempting to unlock tile %s not created by this thread", lockname);
      return;
   }
   sem_post(lock);
   sem_close(lock);
   sem_unlink(lockname);
   tile->lock = NULL;
}

/**
 * \brief query tile to check if the corresponding lockfile exists
 * 
 * this function call is protected by a mutex
 *
 * \sa geocache_cache::tile_lock()
 * \sa geocache_cache::tile_unlock()
 */
int geocache_tileset_tile_lock_exists(geocache_context *ctx, geocache_tile *tile) {
   char *lockname;
   if (tile->lock)
      return GEOCACHE_TRUE;
   lockname = geocache_tileset_tile_lock_key(ctx, tile);
   sem_t *lock = sem_open(lockname, 0, 0644, 1) ;
   if (lock == SEM_FAILED) {
      if(errno == ENOENT) {
         return GEOCACHE_FALSE;
      } else {
         ctx->set_error(ctx,500,"lock_exists: failed to open mutex %s",lockname);
         return GEOCACHE_FALSE;
      }
   } else {
      sem_close(lock);
      return GEOCACHE_TRUE;
   }
}

/**
 * \brief wait for a lock on given tile
 *
 * this function will not return until the lock on the tile has been removed
 * \sa geocache_cache::tile_lock_wait()
 */
void geocache_tileset_tile_lock_wait(geocache_context *ctx, geocache_tile *tile) {
  char *lockname;
  sem_t *lock;
#ifdef DEBUG
  if (tile->lock) {
    ctx->set_error(ctx, 500, "### BUG ### waiting for a lock we have created ourself");
    return;
  }
#endif
  lockname = geocache_tileset_tile_lock_key(ctx, tile);
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
}

/* vim: ai ts=3 sts=3 et sw=3
*/

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

char *geocache_tileset_metatile_lock_key(geocache_context *ctx, geocache_metatile *mt) {
   char *lockname = apr_psprintf(ctx->pool,
         "/%s-%s-%s%s", /*x,y,z,tilesetname*/
         num_encode(ctx->pool,mt->tile.x), num_encode(ctx->pool,mt->tile.y),
         num_encode(ctx->pool,mt->tile.z), mt->tile.tileset->name);
   if(mt->tile.tileset->grid_links->nelts > 1) {
      lockname = apr_pstrcat(ctx->pool,lockname,mt->tile.grid_link->grid->name,NULL);
   }
   if(mt->tile.dimensions && !apr_is_empty_table(mt->tile.dimensions)) {
      const apr_array_header_t *elts = apr_table_elts(mt->tile.dimensions);
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
#ifdef SEM_NAME_LEN
   /* truncate the lockname to the number of allowed characters */
#warning "current platform only supports short semaphore names. lock name max length: " SEM_NAME_LEN
   if(strlen(lockname) >= SEM_NAME_LEN) {
      lockname[SEM_NAME_LEN]='\0';
   }
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
void geocache_tileset_metatile_lock(geocache_context *ctx, geocache_metatile *mt) {
   char *lockname = geocache_tileset_metatile_lock_key(ctx,mt);
   sem_t *lock;
   if ((lock = sem_open(lockname, O_CREAT|O_EXCL, 0644, 1)) == SEM_FAILED) {
      ctx->set_error(ctx,500, "failed to create posix semaphore %s: %s",lockname, strerror(errno));
      return;
   }
   sem_wait(lock);
   mt->lock = lock;
}

/**
 * \brief unlock a previously locked tile
 *
 * this function call is protected by a mutex
 *
 * \sa geocache_cache::tile_unlock()
 * \sa geocache_cache::tile_lock_exists()
 */
void geocache_tileset_metatile_unlock(geocache_context *ctx, geocache_metatile *mt) {
   sem_t *lock = (sem_t*) mt->lock;
   const char *lockname = geocache_tileset_metatile_lock_key(ctx,mt);
   if (!mt->lock) {
      ctx->set_error(ctx,500,"###### TILE UNLOCK ######### attempting to unlock tile %s not created by this thread", lockname);
      return;
   }
   sem_post(lock);
   sem_close(lock);
   sem_unlink(lockname);
   mt->lock = NULL;
}

/**
 * \brief query tile to check if the corresponding lockfile exists
 * 
 * this function call is protected by a mutex
 *
 * \sa geocache_cache::tile_lock()
 * \sa geocache_cache::tile_unlock()
 */
int geocache_tileset_metatile_lock_exists(geocache_context *ctx, geocache_metatile *mt) {
   char *lockname;
   if (mt->lock)
      return GEOCACHE_TRUE;
   lockname = geocache_tileset_metatile_lock_key(ctx, mt);
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
void geocache_tileset_metatile_lock_wait(geocache_context *ctx, geocache_metatile *mt) {
  char *lockname;
  sem_t *lock;
#ifdef DEBUG
  if (mt->lock) {
    ctx->set_error(ctx, 500, "### BUG ### waiting for a lock we have created ourself");
    return;
  }
#endif
  lockname = geocache_tileset_metatile_lock_key(ctx, mt);
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

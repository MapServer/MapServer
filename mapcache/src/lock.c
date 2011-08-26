#include "geocache.h"
#include <apr_file_io.h>
#include <apr_strings.h>

char *geocache_tileset_tile_lock_key(geocache_context *ctx, geocache_tile *tile) {
  return apr_psprintf(ctx->pool,
          "%s/%s-%d-%d-%d.lck",
          tile->tileset->config->lockdir,
          tile->tileset->name,
          tile->z, tile->y, tile->x);
}
/**
 * \brief lock the given tile so other processes know it is being processed
 *
 * this function creates a file with a .lck  extension and puts an exclusive lock on it
 * \sa geocache_cache::tile_lock()
 * \sa geocache_cache::tile_lock_exists()
 * \private \memberof geocache_cache_disk
 */
void geocache_tileset_tile_lock(geocache_context *ctx, geocache_tile *tile) {
  char *lockname;
  apr_file_t *f;
  apr_status_t rv;
  lockname = geocache_tileset_tile_lock_key(ctx,tile);
  /*create file, and fail if it already exists*/
  if (apr_file_open(&f, lockname,
          APR_FOPEN_CREATE | APR_FOPEN_EXCL | APR_FOPEN_WRITE | APR_FOPEN_SHARELOCK | APR_FOPEN_BUFFERED | APR_FOPEN_BINARY,
          APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
    /*
     * opening failed, is this because we don't have write permissions, or because
     * the file already exists?
     */
    if (apr_file_open(&f, lockname, APR_FOPEN_CREATE | APR_FOPEN_WRITE, APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
      ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "failed to create lockfile %s", lockname);
      return; /* we could not create the file */
    } else {
      ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "failed to create lockfile %s, because it already exists", lockname);
      apr_file_close(f);
      return; /* we have write access, but the file already exists */
    }
  }
  rv = apr_file_lock(f, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK);
  if (rv != APR_SUCCESS) {
    if (rv == EAGAIN) {
      ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "####### TILE LOCK ######## file %s is already locked", lockname);
    } else {
      ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "failed to lock file %s", lockname);
    }
    return;
  }
  tile->lock = f;
}

/**
 * \brief unlock a previously locked tile
 *
 * \sa geocache_cache::tile_unlock()
 * \sa geocache_cache::tile_lock_exists()
 */
void geocache_tileset_tile_unlock(geocache_context *ctx, geocache_tile *tile) {
  apr_file_t *f = (apr_file_t*) tile->lock;
  const char *lockname;
  if (!tile->lock) {
    lockname = geocache_tileset_tile_lock_key(ctx, tile);
    ctx->log(ctx, GEOCACHE_ERROR, "###### TILE UNLOCK ######### attempting to unlock tile %s not created by this thread", lockname);

    /*fail if the lock does not exists*/
    if (apr_file_open(&f, lockname, APR_FOPEN_READ, APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
      ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "###### TILE UNLOCK ######### tile %s is not locked", lockname);
      return;
    }
  } else {
    f = (apr_file_t*) tile->lock;
    apr_file_name_get(&lockname, f);
  }

  
  apr_file_remove(lockname, ctx->pool);

  int rv = apr_file_unlock(f);
  if (rv != APR_SUCCESS) {
    ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "failed to unlock file %s", lockname);
    return;
  }
  rv = apr_file_close(f);
  if (rv != APR_SUCCESS) {
    ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "failed to close file %s", lockname);
    return;
  }
  tile->lock = NULL;
}

/**
 * \brief query tile to check if the corresponding lockfile exists
 * \sa geocache_cache::tile_lock()
 * \sa geocache_cache::tile_unlock()
 */
int geocache_tileset_tile_lock_exists(geocache_context *ctx, geocache_tile *tile) {
  char *lockname;
  apr_file_t *f;
  if (tile->lock)
    return GEOCACHE_TRUE;
  lockname = geocache_tileset_tile_lock_key(ctx, tile);
  if (apr_file_open(&f, lockname, APR_FOPEN_READ, APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
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
 * \sa geocache_cache::tile_lock_wait()
 */
void geocache_tileset_tile_lock_wait(geocache_context *ctx, geocache_tile *tile) {
  char *lockname;
  apr_file_t *f;
#ifdef DEBUG
  if (tile->lock) {
    ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "### BUG ### waiting for a lock we have created ourself");
    return;
  }
#endif
  lockname = geocache_tileset_tile_lock_key(ctx, tile);
  if (apr_file_open(&f, lockname, APR_FOPEN_READ, APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
    ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "### BUG ### waiting for a lock on an unlocked file");
    return;
  }
  apr_file_lock(f, APR_FLOCK_SHARED);
  apr_file_close(f);
}


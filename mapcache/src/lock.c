/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: OS-level locking support
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
#include <apr_file_io.h>
#include <apr_strings.h>
#include <apr_time.h>

char* lock_filename_for_resource(mapcache_context *ctx, const char *resource) {
   char *saferes = apr_pstrdup(ctx->pool,resource);
   char *safeptr = saferes;
   while(*safeptr) {
      if(*safeptr==' ' || *safeptr == '/' || *safeptr == '~' || *safeptr == '.') {
         *safeptr = '#';
      }
      safeptr++;
   }
   return apr_psprintf(ctx->pool,"%s/"MAPCACHE_LOCKFILE_PREFIX"%s.lck",
         ctx->config->lockdir,saferes);
}

int mapcache_lock_or_wait_for_resource(mapcache_context *ctx, char *resource) {
   char *lockname = lock_filename_for_resource(ctx,resource);
   apr_file_t *lockfile;
   apr_status_t rv;
   /* create the lockfile */
   rv = apr_file_open(&lockfile,lockname,APR_WRITE|APR_CREATE|APR_EXCL,APR_OS_DEFAULT,ctx->pool);

   /* if the file already exists, wait for it to disappear */
   /* TODO: check the lock isn't stale (i.e. too old) */
   if( rv != APR_SUCCESS ) {
      rv = apr_file_open(&lockfile,lockname,APR_READ,APR_OS_DEFAULT,ctx->pool);
#ifdef DEBUG
      if(!APR_STATUS_IS_ENOENT(rv)) {
         ctx->log(ctx, MAPCACHE_DEBUG, "waiting on resource lock %s", resource);
      }
#endif
      while(!APR_STATUS_IS_ENOENT(rv)) {
         /* sleep for the configured number of micro-seconds (default is 1/100th of a second) */
         apr_sleep(ctx->config->lock_retry_interval);
         rv = apr_file_open(&lockfile,lockname,APR_READ,APR_OS_DEFAULT,ctx->pool);
         if(rv == APR_SUCCESS) {
            apr_file_close(lockfile);
         }
      }
      return MAPCACHE_FALSE;
   } else {
      /* we acquired the lock */
      apr_file_close(lockfile);
      return MAPCACHE_TRUE;
   }
}

void mapcache_unlock_resource(mapcache_context *ctx, char *resource) {
   char *lockname = lock_filename_for_resource(ctx,resource);
   apr_file_remove(lockname,ctx->pool);
}

/* vim: ai ts=3 sts=3 et sw=3
*/

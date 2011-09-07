/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: automatic expanding buffer 
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
#include <stdlib.h>
#define INITIAL_BUFFER_SIZE 100

static void _mapcache_buffer_realloc(mapcache_buffer *buffer, size_t len) {
   unsigned char* newbuf ;
   while ( len > buffer->avail ) {
      buffer->avail += buffer->avail;
   }
   newbuf = realloc(buffer->buf, buffer->avail) ;
   if ( newbuf != buffer->buf ) {
      if ( buffer->buf )
         apr_pool_cleanup_kill(buffer->pool, buffer->buf, (void*)free) ;
      apr_pool_cleanup_register(buffer->pool, newbuf,(void*)free, apr_pool_cleanup_null);
      buffer->buf = newbuf ;
   }
}

mapcache_buffer *mapcache_buffer_create(size_t initialStorage, apr_pool_t* pool) {
   mapcache_buffer *buffer = apr_pcalloc(pool, sizeof(mapcache_buffer));
   if(!buffer) return NULL;
   buffer->pool = pool;
   buffer->avail = (initialStorage > INITIAL_BUFFER_SIZE) ? initialStorage : INITIAL_BUFFER_SIZE;
   buffer->buf = malloc(buffer->avail);
   apr_pool_cleanup_register(buffer->pool, buffer->buf,(void*)free, apr_pool_cleanup_null);
   return buffer;
}

int mapcache_buffer_append(mapcache_buffer *buffer, size_t len, void *data) {
   size_t total = buffer->size + len;
   if(total > buffer->avail)
      _mapcache_buffer_realloc(buffer,total);
   memcpy(buffer->buf + buffer->size, data, len);
   buffer->size += len;
   return len;
}
/* vim: ai ts=3 sts=3 et sw=3
*/

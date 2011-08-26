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

#include "yatc.h"
#include <stdlib.h>
#define INITIAL_BUFFER_SIZE 100

static void _yatc_buffer_realloc(yatc_buffer *buffer, size_t len) {
   char* newbuf ;
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

yatc_buffer *yatc_buffer_create(size_t initialStorage, apr_pool_t* pool) {
   yatc_buffer *buffer = apr_pcalloc(pool, sizeof(yatc_buffer));
   buffer->pool = pool;
   buffer->avail = (initialStorage > INITIAL_BUFFER_SIZE) ? initialStorage : INITIAL_BUFFER_SIZE;
   buffer->buf = malloc(buffer->avail);
   apr_pool_cleanup_register(buffer->pool, buffer->buf,(void*)free, apr_pool_cleanup_null);
   return buffer;
}

int yatc_buffer_append(yatc_buffer *buffer, size_t len, void *data) {
   size_t total = buffer->size + len;
   if(total > buffer->avail)
      _yatc_buffer_realloc(buffer,total);
   memcpy(&(buffer->buf[buffer->size]), data, len);
   buffer->size += len;
   return len;
}

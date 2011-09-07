/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
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

#include <apr_version.h>
#include <apr_tables.h>

#ifndef UTIL_H_
#define UTIL_H_

#define MAPCACHE_MAX(a,b) (((a)>(b))?(a):(b))
#define MAPCACHE_MIN(a,b) (((a)<(b))?(a):(b))

#ifndef APR_ARRAY_IDX
#define APR_ARRAY_IDX(ary,i,type) (((type *)(ary)->elts)[i])
#endif

#ifndef APR_ARRAY_PUSH
#define APR_ARRAY_PUSH(ary,type) (*((type *)apr_array_push(ary)))
#endif

#if APR_MAJOR_VERSION < 1 || (APR_MAJOR_VERSION < 2 && APR_MINOR_VERSION < 3)



/**
 * Create a new table whose contents are deep copied from the given
 * table. A deep copy operation copies all fields, and makes copies
 * of dynamically allocated memory pointed to by the fields.
 * @param p The pool to allocate the new table out of
 * @param t The table to clone
 * @return A deep copy of the table passed in
 */
APR_DECLARE(apr_table_t *) apr_table_clone(apr_pool_t *p,
                                           const apr_table_t *t);


#endif

#if APR_MAJOR_VERSION < 1
   #ifndef APR_FOPEN_READ
      #define APR_FOPEN_READ APR_READ
   #endif
   #ifndef APR_FOPEN_BUFFERED
      #define APR_FOPEN_BUFFERED APR_BUFFERED
   #endif

   #ifndef APR_FOPEN_BINARY
      #define APR_FOPEN_BINARY APR_BINARY
   #endif
   
   #ifndef APR_FOPEN_CREATE
      #define APR_FOPEN_CREATE APR_CREATE
   #endif


   #ifndef APR_FOPEN_WRITE
      #define APR_FOPEN_WRITE APR_WRITE
   #endif
   
   #ifndef APR_FOPEN_SHARELOCK
      #define APR_FOPEN_SHARELOCK APR_SHARELOCK
   #endif
#endif



#endif /* UTIL_H_ */
/* vim: ai ts=3 sts=3 et sw=3
*/

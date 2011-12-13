/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: common utility functions
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
#include "util.h"
#include <apr_strings.h>
#include <apr_tables.h>
#include <curl/curl.h>
#include <math.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif
const double mapcache_meters_per_unit[MAPCACHE_UNITS_COUNT] = {1.0,6378137.0 * 2.0 * M_PI / 360,0.3048};

int mapcache_util_extract_int_list(mapcache_context *ctx, const char* cargs,
      const char *sdelim, int **numbers, int *numbers_count) {
   char *last, *key, *endptr;
   char *args = apr_pstrdup(ctx->pool,cargs);
   int tmpcount=1;
   const char *delim = (sdelim)?sdelim:" ,\t\r\n";
   char sep;
   int i;
   *numbers_count = 0;
   i=strlen(delim);
   while(i--) {
      sep = delim[i];
      for(key=args;*key;key++) {
         if(*key == sep)
            tmpcount++;
      }
   }
   
   *numbers = (int*)apr_pcalloc(ctx->pool,tmpcount*sizeof(int));
   for (key = apr_strtok(args, delim, &last); key != NULL;
         key = apr_strtok(NULL, delim, &last)) {
      (*numbers)[(*numbers_count)++] = (int)strtol(key,&endptr,10);
      if(*endptr != 0)
         return MAPCACHE_FAILURE;
   }
   return MAPCACHE_SUCCESS;
}

int mapcache_util_extract_double_list(mapcache_context *ctx, const char* cargs, 
      const char *sdelim, double **numbers, int *numbers_count) {
   char *last, *key, *endptr;
   char *args = apr_pstrdup(ctx->pool,cargs);
   int tmpcount=1;
   const char *delim = (sdelim)?sdelim:" ,\t\r\n";
   char sep;
   int i;
   *numbers_count = 0;
   i=strlen(delim);
   while(i--) {
      sep = delim[i];
      for(key=args;*key;key++) {
         if(*key == sep)
            tmpcount++;
      }
   }
   *numbers = (double*)apr_pcalloc(ctx->pool,tmpcount*sizeof(double));
   for (key = apr_strtok(args, delim, &last); key != NULL;
         key = apr_strtok(NULL, delim, &last)) {
      (*numbers)[(*numbers_count)++] = strtod(key,&endptr);
      if(*endptr != 0)
         return MAPCACHE_FAILURE;
   }
   return MAPCACHE_SUCCESS;
}

char *mapcache_util_str_replace(apr_pool_t *pool, const char *string, const char *substr, const char *replacement ){
   char *tok = NULL;
   char *newstr = NULL;

   tok = strstr( string, substr );
   if( tok == NULL ) return apr_pstrdup( pool, string );
   newstr = apr_pcalloc(pool, strlen( string ) - strlen( substr ) + strlen( replacement ) + 1 );
   memcpy( newstr, string, tok - string );
   memcpy( newstr + (tok - string), replacement, strlen( replacement ) );
   memcpy( newstr + (tok - string) + strlen( replacement ), tok + strlen( substr ), strlen( string ) - strlen( substr ) - ( tok - string ) );
   memset( newstr + strlen( string ) - strlen( substr ) + strlen( replacement ), 0, 1 );
   return newstr;
}

const char* mapcache_util_str_sanitize(apr_pool_t *pool, const char *str, const char* from, char to) {
   size_t pos = strcspn(str,from);
   if(str[pos]) {
      str = apr_pstrdup(pool,str);
      while(str[pos]) {
         ((char*)str)[pos]=to;
         pos += strcspn(&str[pos],from);
      }
   }
   return str;
}

#if APR_MAJOR_VERSION < 1 || (APR_MAJOR_VERSION < 2 && APR_MINOR_VERSION < 3)
APR_DECLARE(apr_table_t *) apr_table_clone(apr_pool_t *p, const apr_table_t *t)
{
    const apr_array_header_t *array = apr_table_elts(t);
    apr_table_entry_t *elts = (apr_table_entry_t *) array->elts;
    apr_table_t *new = apr_table_make(p, array->nelts);
    int i;

    for (i = 0; i < array->nelts; i++) {
        apr_table_add(new, elts[i].key, elts[i].val);
    }

    return new;
}

#endif

int _mapcache_context_get_error_default(mapcache_context *ctx) {
    return ctx->_errcode;
}

char* _mapcache_context_get_error_msg_default(mapcache_context *ctx) {
    return ctx->_errmsg;
}

void _mapcache_context_set_exception_default(mapcache_context *ctx, char *key, char *msg, ...) {
   char *fullmsg;
   va_list args;
   if(!ctx->exceptions) {
      ctx->exceptions = apr_table_make(ctx->pool,1);
   }
  
   va_start(args,msg);
   fullmsg = apr_pvsprintf(ctx->pool,msg,args);
   va_end(args);
   apr_table_set(ctx->exceptions,key,fullmsg);
}

void _mapcache_context_set_error_default(mapcache_context *ctx, int code, char *msg, ...) {
    char *fmt;
    va_list args;
    va_start(args,msg);

    if(ctx->_errmsg) {
        fmt=apr_psprintf(ctx->pool,"%s\n%s",ctx->_errmsg,msg);
    } else {
        fmt=msg;
        ctx->_errcode = code;
    }
    ctx->_errmsg = apr_pvsprintf(ctx->pool,fmt,args);
    va_end(args);
}

void _mapcache_context_clear_error_default(mapcache_context *ctx) {
   ctx->_errcode = 0;
   ctx->_errmsg = NULL;
   if(ctx->exceptions) {
      apr_table_clear(ctx->exceptions);
   }
}


void mapcache_context_init(mapcache_context *ctx) {
    ctx->_errcode = 0;
    ctx->_errmsg = NULL;
    ctx->get_error = _mapcache_context_get_error_default;
    ctx->get_error_message = _mapcache_context_get_error_msg_default;
    ctx->set_error = _mapcache_context_set_error_default;
    ctx->set_exception = _mapcache_context_set_exception_default;
    ctx->clear_errors = _mapcache_context_clear_error_default;
}

void mapcache_context_copy(mapcache_context *src, mapcache_context *dst) {
   dst->_contenttype = src->_contenttype;
   dst->_errcode = src->_errcode;
   dst->_errmsg = src->_errmsg;
   dst->clear_errors = src->clear_errors;
   dst->clone = src->clone;
   dst->config = src->config;
   dst->get_error = src->get_error;
   dst->get_error_message = src->get_error_message;
   dst->get_instance_id = src->get_instance_id;
   dst->log = src->log;
   dst->set_error = src->set_error;
   dst->pool = src->pool;
   dst->set_exception = src->set_exception;
   dst->service = src->service;
   dst->has_threads = src->has_threads;
   dst->exceptions = src->exceptions;
}


/* vim: ai ts=3 sts=3 et sw=3
*/

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
#include "util.h"
#include <apr_strings.h>
#include <apr_tables.h>
#include <curl/curl.h>
#include <math.h>
#include <unistd.h>

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
   *numbers_count = 0;
   int i=strlen(delim);
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
   *numbers_count = 0;
   int i=strlen(delim);
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
}


void mapcache_context_init(mapcache_context *ctx) {
    ctx->_errcode = 0;
    ctx->_errmsg = NULL;
    ctx->get_error = _mapcache_context_get_error_default;
    ctx->get_error_message = _mapcache_context_get_error_msg_default;
    ctx->set_error = _mapcache_context_set_error_default;
    ctx->clear_errors = _mapcache_context_clear_error_default;
}

/* vim: ai ts=3 sts=3 et sw=3
*/

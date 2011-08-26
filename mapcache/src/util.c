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
#include "util.h"
#include <apr_strings.h>
#include <apr_tables.h>
#include <curl/curl.h>
#include <math.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif
const double geocache_meters_per_unit[GEOCACHE_UNITS_COUNT] = {1.0,6378137.0 * 2.0 * M_PI / 360,0.3048};

int geocache_util_extract_int_list(geocache_context *ctx, const char* cargs, const char sep, int **numbers,
      int *numbers_count) {
   char *last, *key, *endptr;
   char *args = apr_pstrdup(ctx->pool,cargs);
   int tmpcount=1;
   char delim[2];
   delim[0] = sep;
   delim[1] = 0;

   *numbers_count = 0;
   for(key=args;*key;key++) {
      if(*key == sep)
         tmpcount++;
   }
   *numbers = (int*)apr_pcalloc(ctx->pool,tmpcount*sizeof(int));
   for (key = apr_strtok(args, delim, &last); key != NULL;
         key = apr_strtok(NULL, delim, &last)) {
      (*numbers)[(*numbers_count)++] = (int)strtol(key,&endptr,10);
      if(*endptr != 0)
         return GEOCACHE_FAILURE;
   }
   return GEOCACHE_SUCCESS;
}

int geocache_util_extract_double_list(geocache_context *ctx, const char* cargs, const char sep, double **numbers,
      int *numbers_count) {
   char *last, *key, *endptr;
   char *args = apr_pstrdup(ctx->pool,cargs);
   int tmpcount=1;
   char delim[2];
   delim[0] = sep;
   delim[1] = 0;

   *numbers_count = 0;
   for(key=args;*key;key++) {
      if(*key == sep)
         tmpcount++;
   }
   *numbers = (double*)apr_pcalloc(ctx->pool,tmpcount*sizeof(double));
   for (key = apr_strtok(args, delim, &last); key != NULL;
         key = apr_strtok(NULL, delim, &last)) {
      (*numbers)[(*numbers_count)++] = strtod(key,&endptr);
      if(*endptr != 0)
         return GEOCACHE_FAILURE;
   }
   return GEOCACHE_SUCCESS;
}

#if APR_MAJOR_VERSION < 2 && APR_MINOR_VERSION < 3
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

geocache_error_code _geocache_context_get_error_default(geocache_context *ctx) {
    return ctx->_errcode;
}

char* _geocache_context_get_error_msg_default(geocache_context *ctx) {
    return ctx->_errmsg;
}

void _geocache_context_set_error_default(geocache_context *ctx, geocache_error_code code, char *msg, ...) {
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


void geocache_context_init(geocache_context *ctx) {
    ctx->_errcode = GEOCACHE_NO_ERROR;
    ctx->_errmsg = NULL;
    ctx->get_error = _geocache_context_get_error_default;
    ctx->get_error_message = _geocache_context_get_error_msg_default;
    ctx->set_error = _geocache_context_set_error_default;
}

/* vim: ai ts=3 sts=3 et sw=3
*/

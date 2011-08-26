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
#include <curl/curl.h>
#include <apr_hash.h>
#include <apr_strings.h>
#include <ctype.h>

#define MAX_STRING_LEN 10000

size_t _geocache_curl_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
   geocache_buffer *buffer = (geocache_buffer*)data;
   size_t realsize = size * nmemb;
   return geocache_buffer_append(buffer, realsize, ptr);
}

void geocache_http_request_url(geocache_context *ctx, char *url, apr_table_t *headers, geocache_buffer *data) {
   CURL *curl_handle;
   curl_handle = curl_easy_init();
   int ret;
   char error_msg[CURL_ERROR_SIZE];
   /* specify URL to get */
   curl_easy_setopt(curl_handle, CURLOPT_URL, url);
#ifdef DEBUG
   ctx->log(ctx, GEOCACHE_DEBUG, "##### START #####\ncurl requesting url %s",url);
#endif
   /* send all data to this function  */ 
   curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _geocache_curl_memory_callback);

   /* we pass our geocache_buffer struct to the callback function */ 
   curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)data);

   curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_msg);
   curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
   curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);


   struct curl_slist *curl_headers=NULL;
   if(headers) {
      const apr_array_header_t *array = apr_table_elts(headers);
      apr_table_entry_t *elts = (apr_table_entry_t *) array->elts;
      int i;
      for (i = 0; i < array->nelts; i++) {
         curl_headers = curl_slist_append(curl_headers, apr_pstrcat(ctx->pool,elts[i].key,": ",elts[i].val,NULL));
      }
   }
   if(!headers || !apr_table_get(headers,"User-Agent")) {
      curl_headers = curl_slist_append(curl_headers, "User-Agent: "GEOCACHE_USERAGENT);
   }
   curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, curl_headers);
   /* get it! */ 
   ret = curl_easy_perform(curl_handle);
   if(ret != CURLE_OK) {
      ctx->set_error(ctx, 502, "curl failed to request url %s : %s", url, error_msg);
   }
   /* cleanup curl stuff */ 
   curl_easy_cleanup(curl_handle);
}

void geocache_http_request_url_with_params(geocache_context *ctx, char *url, apr_table_t *params, apr_table_t *headers, geocache_buffer *data) {
   char *fullUrl = geocache_http_build_url(ctx,url,params);
   geocache_http_request_url(ctx,fullUrl,headers,data);
}

/* calculate the length of the string formed by key=value&, and add it to cnt */
static APR_DECLARE_NONSTD(int) _geocache_key_value_strlen_callback(void *cnt, const char *key, const char *value) {
   *((int*)cnt) += strlen(key) + 2 + ((value && *value) ? strlen(value) : 0);
   return 1;
}


static APR_DECLARE_NONSTD(int) _geocache_key_value_append_callback(void *cnt, const char *key, const char *value) {
#define _mystr *((char**)cnt)
   _mystr = apr_cpystrn(_mystr,key,MAX_STRING_LEN);
   *((_mystr)++) = '=';
   if(value && *value) {
      _mystr = apr_cpystrn(_mystr,value,MAX_STRING_LEN);
   }
   *((_mystr)++) = '&';
   return 1;
#undef _mystr
}

static char _geocache_x2c(const char *what)
{
    register char digit;
    digit = ((what[0] >= 'A') ? ((what[0] & 0xdf) - 'A') + 10
             : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10
              : (what[1] - '0'));
    return (digit);
}

#ifdef _WIN32
#define IS_SLASH(s) ((s == '/') || (s == '\\'))
#else
#define IS_SLASH(s) (s == '/')
#endif

int _geocache_unescape_url(char *url) {
   register int badesc, badpath;
   char *x, *y;

   badesc = 0;
   badpath = 0;
   /* Initial scan for first '%'. Don't bother writing values before
    * seeing a '%' */
   y = strchr(url, '%');
   if (y == NULL) {
      return GEOCACHE_SUCCESS;
   }
   for (x = y; *y; ++x, ++y) {
      if (*y != '%')
         *x = *y;
      else {
         if (!isxdigit(*(y + 1)) || !isxdigit(*(y + 2))) {
            badesc = 1;
            *x = '%';
         }
         else {
            *x = _geocache_x2c(y + 1);
            y += 2;
            if (IS_SLASH(*x) || *x == '\0')
               badpath = 1;
         }
      }
   }
   *x = '\0';
   if (badesc)
      return GEOCACHE_FAILURE;
   else if (badpath)
      return GEOCACHE_FAILURE;
   else
      return GEOCACHE_SUCCESS;
}



char* geocache_http_build_url(geocache_context *r, char *base, apr_table_t *params) {
   if(!apr_is_empty_table(params)) {
      int stringLength = 0, baseLength;
      char *builtUrl,*builtUrlPtr;
      char charToAppend=0;
      baseLength = strlen(base);

      /*calculate the length of the param string we are going to build */
      apr_table_do(_geocache_key_value_strlen_callback, (void*)&stringLength, params, NULL);

      if(strchr(base,'?')) {
         /* base already contains a '?' , shall we be adding a '&' to the end */ 
         if(base[baseLength-1] != '?' && base[baseLength-1] != '&') {
            charToAppend = '&';
         }
      } else {
         /* base does not contain a '?', we will be adding it */
         charToAppend='?';
      }

      /* add final \0 and eventual separator to add ('?' or '&') */
      stringLength += baseLength + ((charToAppend)?2:1);

      builtUrl = builtUrlPtr = apr_palloc(r->pool, stringLength);

      builtUrlPtr = apr_cpystrn(builtUrlPtr,base,MAX_STRING_LEN);
      if(charToAppend)
         *(builtUrlPtr++)=charToAppend;
      apr_table_do(_geocache_key_value_append_callback, (void*)&builtUrlPtr, params, NULL);
      *(builtUrlPtr-1) = '\0'; /*replace final '&' by a \0 */
      return builtUrl;
   } else {
      return base;
   }
}

/* Parse form data from a string. The input string is preserved. */
apr_table_t *geocache_http_parse_param_string(geocache_context *r, char *args_str) {
   apr_table_t *params;
   char *args = apr_pstrdup(r->pool,args_str);
   char *key;
   char *value;
   const char *delim = "&";
   char *last;
   if (args == NULL) {
      return apr_table_make(r->pool,0);
   }
   params = apr_table_make(r->pool,20);
   /* Split the input on '&' */
   for (key = apr_strtok(args, delim, &last); key != NULL;
         key = apr_strtok(NULL, delim, &last)) {
      /* key is a pointer to the key=value string */
      /*loop through key=value string to replace '+' by ' ' */
      for (value = key; *value; ++value) {
         if (*value == '+') {
            *value = ' ';
         }
      }

      /* split into Key / Value and unescape it */
      value = strchr(key, '=');
      if (value) {
         *value++ = '\0'; /* replace '=' by \0, thus terminating the key string */
         _geocache_unescape_url(key);
         _geocache_unescape_url(value);
      }
      else {
         value = "";
         _geocache_unescape_url(key);
      }
      /* Store key/value pair in our form hash. */
      apr_table_addn(params, key, value);
   }
   return params;
}


/* vim: ai ts=3 sts=3 et sw=3
*/

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
#include <curl/curl.h>
#include <apr_hash.h>
#include <apr_strings.h>
#include <http_log.h>
#include <stdio.h>


size_t _yatc_curl_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
   yatc_buffer *buffer = (yatc_buffer*)data;
   size_t realsize = size * nmemb;
   return yatc_buffer_append(buffer, realsize, ptr);
}

int yatc_http_request_url(request_rec *r, char *url, yatc_buffer *data) {
   CURL *curl_handle;
   curl_handle = curl_easy_init();

   /* specify URL to get */
   ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,"##### START #####\ncurl requesting url %s",url);
   curl_easy_setopt(curl_handle, CURLOPT_URL, url);

   /* send all data to this function  */ 
   curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _yatc_curl_memory_callback);

   /* we pass our yatc_buffer struct to the callback function */ 
   curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)data);

   /* some servers don't like requests that are made without a user-agent field, so we provide one */ 
   curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "mod_yatc/0.1");

   /* get it! */ 
   curl_easy_perform(curl_handle);

   /* cleanup curl stuff */ 
   curl_easy_cleanup(curl_handle);
   ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,"##### END #####\nrequested url %s",url);
   return 0;
}

int yatc_http_request_url_with_params(request_rec *r, char *url, apr_table_t *params, yatc_buffer *data) {
   char *fullUrl = yatc_http_build_url(r,url,params);
   return yatc_http_request_url(r,fullUrl,data);
}

/* calculate the length of the string formed by key=value&, and add it to cnt */
static APR_DECLARE_NONSTD(int) _yatc_key_value_strlen_callback(void *cnt, const char *key, const char *value) {
   *((int*)cnt) += strlen(key) + ((value && *value) ? strlen(value)+2 : 1);
   return 1;
}


static APR_DECLARE_NONSTD(int) _yatc_key_value_append_callback(void *cnt, const char *key, const char *value) {
#define _mystr *((char**)cnt)
   _mystr = apr_cpystrn(_mystr,key,MAX_STRING_LEN);
   if(value && *value) {
      *((_mystr)++) = '=';
      _mystr = apr_cpystrn(_mystr,value,MAX_STRING_LEN);
   }
   *((_mystr)++) = '&';
   return 1;
#undef _mystr
}

char* yatc_http_build_url(request_rec *r, char *base, apr_table_t *params) {
   if(!apr_is_empty_table(params)) {
      int stringLength = 0, baseLength;
      char *builtUrl,*builtUrlPtr;
      char charToAppend=0;
      baseLength = strlen(base);

      /*calculate the length of the param string we are going to build */
      apr_table_do(_yatc_key_value_strlen_callback, (void*)&stringLength, params, NULL);

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
      stringLength += baseLength + (charToAppend)?2:1;

      builtUrl = builtUrlPtr = apr_palloc(r->pool, stringLength);

      builtUrlPtr = apr_cpystrn(builtUrlPtr,base,MAX_STRING_LEN);
      if(charToAppend)
         *(builtUrlPtr++)=charToAppend;
      apr_table_do(_yatc_key_value_append_callback, (void*)&builtUrlPtr, params, NULL);
      *(builtUrlPtr-1) = '\0'; /*replace final '&' by a \0 */
      return builtUrl;
   } else {
      return base;
   }
}

/* Parse form data from a string. The input string is preserved. */
apr_table_t *yatc_http_parse_param_string(request_rec *r, char *args_str) {
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
         ap_unescape_url(key);
         ap_unescape_url(value);
      }
      else {
         value = "";
         ap_unescape_url(key);
      }
      /* Store key/value pair in our form hash. */
      apr_table_addn(params, key, value);
   }
   return params;
}



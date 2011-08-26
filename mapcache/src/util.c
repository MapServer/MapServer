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

#include <geocache.h>
#include <apr_strings.h>
#include <apr_tables.h>

int geocache_util_extract_int_list(char* args, const char sep, int **numbers,
      int *numbers_count, apr_pool_t *pool) {
   char *last, *key, *endptr;
   int tmpcount=1;
   char delim[2];
   delim[0] = sep;
   delim[1] = 0;

   *numbers_count = 0;
   for(key=args;*key;key++) {
      if(*key == sep)
         tmpcount++;
   }
   *numbers = (int*)apr_pcalloc(pool,tmpcount*sizeof(int));
   for (key = apr_strtok(args, delim, &last); key != NULL;
         key = apr_strtok(NULL, delim, &last)) {
      (*numbers)[(*numbers_count)++] = (int)strtol(key,&endptr,10);
      if(*endptr != 0)
         return GEOCACHE_FAILURE;
   }
   return GEOCACHE_SUCCESS;
}

int geocache_util_extract_double_list(char* args, const char sep, double **numbers,
      int *numbers_count, apr_pool_t *pool) {
   char *last, *key, *endptr;
   int tmpcount=1;
   char delim[2];
   delim[0] = sep;
   delim[1] = 0;

   *numbers_count = 0;
   for(key=args;*key;key++) {
      if(*key == sep)
         tmpcount++;
   }
   *numbers = (double*)apr_pcalloc(pool,tmpcount*sizeof(double));
   for (key = apr_strtok(args, delim, &last); key != NULL;
         key = apr_strtok(NULL, delim, &last)) {
      (*numbers)[(*numbers_count)++] = strtod(key,&endptr);
      if(*endptr != 0)
         return GEOCACHE_FAILURE;
   }
   return GEOCACHE_SUCCESS;
}



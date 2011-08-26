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
#include <apr_strings.h>
#include <math.h>
#include <sys/types.h>


static int _geocache_dimension_intervals_validate(geocache_context *ctx, geocache_dimension *dim, char **value) {
   int i;
   char *endptr;
   double val = strtod(*value,&endptr);
   *value = apr_psprintf(ctx->pool,"%g",val);
   if(*endptr != 0) {
      return GEOCACHE_FAILURE;
   }
   geocache_dimension_intervals *dimension = (geocache_dimension_intervals*)dim;
   for(i=0;i<dimension->nintervals;i++) {
      geocache_interval *interval = &dimension->intervals[i];
      if(val<interval->start || val>interval->end)
         continue;
      if(interval->resolution == 0)
         return GEOCACHE_SUCCESS;
      double diff = fmod((val - interval->start),interval->resolution);
      if(diff == 0.0)
         return GEOCACHE_SUCCESS;
   }
   return GEOCACHE_FAILURE;
}

static const char** _geocache_dimension_intervals_print(geocache_context *ctx, geocache_dimension *dim) {
   geocache_dimension_intervals *dimension = (geocache_dimension_intervals*)dim;
   const char **ret = (const char**)apr_pcalloc(ctx->pool,(dimension->nintervals+1)*sizeof(const char*));
   int i;
   for(i=0;i<dimension->nintervals;i++) {
      geocache_interval *interval = &dimension->intervals[i];
      ret[i] = apr_psprintf(ctx->pool,"%g/%g/%g",interval->start,interval->end,interval->resolution);
   }
   ret[i]=NULL;
   return ret;
}

static void _geocache_dimension_intervals_parse(geocache_context *ctx, geocache_dimension *dim,
      ezxml_t node) {
   const char *entry = node->txt;
   int count = 1;
   if(!entry || !*entry) {
      ctx->set_error(ctx,400,"failed to parse dimension values: none supplied");
      return;
   }
   geocache_dimension_intervals *dimension = (geocache_dimension_intervals*)dim;
   char *values = apr_pstrdup(ctx->pool,entry);
   char *key,*last;
   for(key=values;*key;key++) if(*key == ',') count++;

   dimension->intervals = (geocache_interval*)apr_pcalloc(ctx->pool,count*sizeof(geocache_interval));

   for (key = apr_strtok(values, ",", &last); key != NULL;
         key = apr_strtok(NULL, ",", &last)) {
      char *endptr;
      geocache_interval *interval = &dimension->intervals[dimension->nintervals];
      interval->start = strtod(key,&endptr);
      if(*endptr != '/') {
         ctx->set_error(ctx,400,"failed to parse min dimension value \"%s\" in \"%s\" for dimension %s",key,entry,dim->name);
         return;
      }
      key = endptr+1;
      
      interval->end = strtod(key,&endptr);
      if(*endptr != '/') {
         ctx->set_error(ctx,400,"failed to parse max dimension value \"%s\" in \"%s\" for dimension %s",key,entry,dim->name);
         return;
      }
      key = endptr+1;
      interval->resolution = strtod(key,&endptr);
      if(*endptr != '\0') {
         ctx->set_error(ctx,400,"failed to parse resolution dimension value \"%s\" in \"%s\" for dimension %s",key,entry,dim->name);
         return;
      }
      dimension->nintervals++;
   }

   if(!dimension->nintervals) {
      ctx->set_error(ctx, 400, "<dimension> \"%s\" has no intervals",dim->name);
      return;
   }
}

static int _geocache_dimension_regex_validate(geocache_context *ctx, geocache_dimension *dim, char **value) {
   geocache_dimension_regex *dimension = (geocache_dimension_regex*)dim;
#ifdef USE_PCRE
   int ovector[30];
   int rc = pcre_exec(dimension->pcregex,NULL,*value,strlen(*value),0,0,ovector,30);
   if(rc>0)
      return GEOCACHE_SUCCESS;
#else
   if(!regexec(dimension->regex,*value,0,0,0)) {
      return GEOCACHE_SUCCESS;
   }
#endif
   return GEOCACHE_FAILURE;
}

static const char** _geocache_dimension_regex_print(geocache_context *ctx, geocache_dimension *dim) {
   geocache_dimension_regex *dimension = (geocache_dimension_regex*)dim;
   const char **ret = (const char**)apr_pcalloc(ctx->pool,2*sizeof(const char*));
   ret[0]=dimension->regex_string;
   ret[1]=NULL;
   return ret;
}

static void _geocache_dimension_regex_parse(geocache_context *ctx, geocache_dimension *dim,
      ezxml_t node) {
   const char *entry = node->txt;
   if(!entry || !*entry) {
      ctx->set_error(ctx,400,"failed to parse dimension regex: none supplied");
      return;
   }
   geocache_dimension_regex *dimension = (geocache_dimension_regex*)dim;
   dimension->regex_string = apr_pstrdup(ctx->pool,entry);
#ifdef USE_PCRE
   const char *pcre_err;
   int pcre_offset;
   dimension->pcregex = pcre_compile(entry,0,&pcre_err, &pcre_offset,0);
   if(!dimension->pcregex) {
      ctx->set_error(ctx,400,"failed to compile regular expression \"%s\" for dimension \"%s\": %s",
            entry,dim->name,pcre_err);
      return;
   }
#else
   int rc = regcomp(dimension->regex, entry, REG_EXTENDED);
   if(rc) {
      char errmsg[200];
      regerror(rc,dimension->regex,errmsg,200);
      ctx->set_error(ctx,400,"failed to compile regular expression \"%s\" for dimension \"%s\": %s",
            entry,dim->name,errmsg);
      return;
   }
#endif

}

static int _geocache_dimension_values_validate(geocache_context *ctx, geocache_dimension *dim, char **value) {
   int i;
   geocache_dimension_values *dimension = (geocache_dimension_values*)dim;
   for(i=0;i<dimension->nvalues;i++) {
      if(dimension->case_sensitive) {
         if(!strcmp(*value,dimension->values[i]))
            return GEOCACHE_SUCCESS;
      } else {
         if(!strcasecmp(*value,dimension->values[i]))
            return GEOCACHE_SUCCESS;
      }
   }
   return GEOCACHE_FAILURE;
}

static const char** _geocache_dimension_values_print(geocache_context *ctx, geocache_dimension *dim) {
   geocache_dimension_values *dimension = (geocache_dimension_values*)dim;
   const char **ret = (const char**)apr_pcalloc(ctx->pool,(dimension->nvalues+1)*sizeof(const char*));
   int i;
   for(i=0;i<dimension->nvalues;i++) {
      ret[i] = dimension->values[i];
   }
   ret[i]=NULL;
   return ret;
}

static void _geocache_dimension_values_parse(geocache_context *ctx, geocache_dimension *dim,
      ezxml_t node) {
   int count = 1;
   const char *entry = node->txt;
   if(!entry || !*entry) {
      ctx->set_error(ctx,400,"failed to parse dimension values: none supplied");
      return;
   }

   geocache_dimension_values *dimension = (geocache_dimension_values*)dim;
   const char *case_sensitive = ezxml_attr(node,"case_sensitive");
   if(case_sensitive && !strcasecmp(case_sensitive,"true")) {
      dimension->case_sensitive = 1;
   }
   
   char *values = apr_pstrdup(ctx->pool,entry);
   char *key,*last;
   for(key=values;*key;key++) if(*key == ',') count++;

   dimension->values = (char**)apr_pcalloc(ctx->pool,count*sizeof(char*));

   for (key = apr_strtok(values, ",", &last); key != NULL;
         key = apr_strtok(NULL, ",", &last)) {
      dimension->values[dimension->nvalues]=key;
      dimension->nvalues++;
   }
   if(!dimension->nvalues) {
      ctx->set_error(ctx, 400, "<dimension> \"%s\" has no values",dim->name);
      return;
   }
}

geocache_dimension* geocache_dimension_values_create(apr_pool_t *pool) {
   geocache_dimension_values *dimension = apr_pcalloc(pool, sizeof(geocache_dimension_values));
   dimension->dimension.type = GEOCACHE_DIMENSION_VALUES;
   dimension->nvalues = 0;
   dimension->dimension.validate = _geocache_dimension_values_validate;
   dimension->dimension.parse = _geocache_dimension_values_parse;
   dimension->dimension.print_ogc_formatted_values = _geocache_dimension_values_print;
   return (geocache_dimension*)dimension;
}

geocache_dimension* geocache_dimension_time_create(apr_pool_t *pool) {
   geocache_dimension_time *dimension = apr_pcalloc(pool, sizeof(geocache_dimension_time));
   dimension->dimension.type = GEOCACHE_DIMENSION_TIME;
   dimension->nintervals = 0;
//   dimension->dimension.validate = _geocache_dimension_time_validate;
//   dimension->dimension.parse = _geocache_dimension_time_parse;
//   dimension->dimension.print_ogc_formatted_values = _geocache_dimension_time_print;
   return (geocache_dimension*)dimension;
}

geocache_dimension* geocache_dimension_intervals_create(apr_pool_t *pool) {
   geocache_dimension_intervals *dimension = apr_pcalloc(pool, sizeof(geocache_dimension_intervals));
   dimension->dimension.type = GEOCACHE_DIMENSION_INTERVALS;
   dimension->nintervals = 0;
   dimension->dimension.validate = _geocache_dimension_intervals_validate;
   dimension->dimension.parse = _geocache_dimension_intervals_parse;
   dimension->dimension.print_ogc_formatted_values = _geocache_dimension_intervals_print;
   return (geocache_dimension*)dimension;
}
geocache_dimension* geocache_dimension_regex_create(apr_pool_t *pool) {
   geocache_dimension_regex *dimension = apr_pcalloc(pool, sizeof(geocache_dimension_regex));
   dimension->dimension.type = GEOCACHE_DIMENSION_REGEX;
#ifndef USE_PCRE
   dimension->regex = (regex_t*)apr_pcalloc(pool, sizeof(regex_t));
#endif
   dimension->dimension.validate = _geocache_dimension_regex_validate;
   dimension->dimension.parse = _geocache_dimension_regex_parse;
   dimension->dimension.print_ogc_formatted_values = _geocache_dimension_regex_print;
   return (geocache_dimension*)dimension;
}
/* vim: ai ts=3 sts=3 et sw=3
*/

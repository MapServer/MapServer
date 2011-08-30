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
#include <apr_strings.h>
#include <math.h>
#include <sys/types.h>


static int _mapcache_dimension_intervals_validate(mapcache_context *ctx, mapcache_dimension *dim, char **value) {
   int i;
   char *endptr;
   double val = strtod(*value,&endptr);
   *value = apr_psprintf(ctx->pool,"%g",val);
   if(*endptr != 0) {
      return MAPCACHE_FAILURE;
   }
   mapcache_dimension_intervals *dimension = (mapcache_dimension_intervals*)dim;
   for(i=0;i<dimension->nintervals;i++) {
      mapcache_interval *interval = &dimension->intervals[i];
      if(val<interval->start || val>interval->end)
         continue;
      if(interval->resolution == 0)
         return MAPCACHE_SUCCESS;
      double diff = fmod((val - interval->start),interval->resolution);
      if(diff == 0.0)
         return MAPCACHE_SUCCESS;
   }
   return MAPCACHE_FAILURE;
}

static const char** _mapcache_dimension_intervals_print(mapcache_context *ctx, mapcache_dimension *dim) {
   mapcache_dimension_intervals *dimension = (mapcache_dimension_intervals*)dim;
   const char **ret = (const char**)apr_pcalloc(ctx->pool,(dimension->nintervals+1)*sizeof(const char*));
   int i;
   for(i=0;i<dimension->nintervals;i++) {
      mapcache_interval *interval = &dimension->intervals[i];
      ret[i] = apr_psprintf(ctx->pool,"%g/%g/%g",interval->start,interval->end,interval->resolution);
   }
   ret[i]=NULL;
   return ret;
}

#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
static void _mapcache_dimension_intervals_parse_json(mapcache_context *ctx, mapcache_dimension *dim,
      cJSON *node) {
}
#endif

static void _mapcache_dimension_intervals_parse_xml(mapcache_context *ctx, mapcache_dimension *dim,
      ezxml_t node) {
   const char *entry = node->txt;
   int count = 1;
   if(!entry || !*entry) {
      ctx->set_error(ctx,400,"failed to parse dimension values: none supplied");
      return;
   }
   mapcache_dimension_intervals *dimension = (mapcache_dimension_intervals*)dim;
   char *values = apr_pstrdup(ctx->pool,entry);
   char *key,*last;
   for(key=values;*key;key++) if(*key == ',') count++;

   dimension->intervals = (mapcache_interval*)apr_pcalloc(ctx->pool,count*sizeof(mapcache_interval));

   for (key = apr_strtok(values, ",", &last); key != NULL;
         key = apr_strtok(NULL, ",", &last)) {
      char *endptr;
      mapcache_interval *interval = &dimension->intervals[dimension->nintervals];
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

static int _mapcache_dimension_regex_validate(mapcache_context *ctx, mapcache_dimension *dim, char **value) {
   mapcache_dimension_regex *dimension = (mapcache_dimension_regex*)dim;
#ifdef USE_PCRE
   int ovector[30];
   int rc = pcre_exec(dimension->pcregex,NULL,*value,strlen(*value),0,0,ovector,30);
   if(rc>0)
      return MAPCACHE_SUCCESS;
#else
   if(!regexec(dimension->regex,*value,0,0,0)) {
      return MAPCACHE_SUCCESS;
   }
#endif
   return MAPCACHE_FAILURE;
}

static const char** _mapcache_dimension_regex_print(mapcache_context *ctx, mapcache_dimension *dim) {
   mapcache_dimension_regex *dimension = (mapcache_dimension_regex*)dim;
   const char **ret = (const char**)apr_pcalloc(ctx->pool,2*sizeof(const char*));
   ret[0]=dimension->regex_string;
   ret[1]=NULL;
   return ret;
}

#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
static void _mapcache_dimension_regex_parse_json(mapcache_context *ctx, mapcache_dimension *dim,
      cJSON *node) {
   mapcache_dimension_regex *dimension = (mapcache_dimension_regex*)dim;
   cJSON *tmp = cJSON_GetObjectItem(node,"regex");
   if(tmp && tmp->valuestring && *(tmp->valuestring)) {
      dimension->regex_string = apr_pstrdup(ctx->pool,tmp->valuestring);
#ifdef USE_PCRE
      const char *pcre_err;
      int pcre_offset;
      dimension->pcregex = pcre_compile(dimension->regex_string,0,&pcre_err, &pcre_offset,0);
      if(!dimension->pcregex) {
         ctx->set_error(ctx,400,"failed to compile regular expression \"%s\" for dimension \"%s\": %s",
               dimension->regex_string,dim->name,pcre_err);
         return;
      }
#else
      int rc = regcomp(dimension->regex, dimension->regex_string, REG_EXTENDED);
      if(rc) {
         char errmsg[200];
         regerror(rc,dimension->regex,errmsg,200);
         ctx->set_error(ctx,400,"failed to compile regular expression \"%s\" for dimension \"%s\": %s",
               dimension->regex_string,dim->name,errmsg);
         return;
      }
#endif
   }
   if(!dimension->regex_string) {
      ctx->set_error(ctx,400,"failed to parse dimension \"%s\" regex: none supplied", dim->name);
      return;
   }
}
#endif

static void _mapcache_dimension_regex_parse_xml(mapcache_context *ctx, mapcache_dimension *dim,
      ezxml_t node) {
   const char *entry = node->txt;
   if(!entry || !*entry) {
      ctx->set_error(ctx,400,"failed to parse dimension regex: none supplied");
      return;
   }
   mapcache_dimension_regex *dimension = (mapcache_dimension_regex*)dim;
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

static int _mapcache_dimension_values_validate(mapcache_context *ctx, mapcache_dimension *dim, char **value) {
   int i;
   mapcache_dimension_values *dimension = (mapcache_dimension_values*)dim;
   for(i=0;i<dimension->nvalues;i++) {
      if(dimension->case_sensitive) {
         if(!strcmp(*value,dimension->values[i]))
            return MAPCACHE_SUCCESS;
      } else {
         if(!strcasecmp(*value,dimension->values[i]))
            return MAPCACHE_SUCCESS;
      }
   }
   return MAPCACHE_FAILURE;
}

static const char** _mapcache_dimension_values_print(mapcache_context *ctx, mapcache_dimension *dim) {
   mapcache_dimension_values *dimension = (mapcache_dimension_values*)dim;
   const char **ret = (const char**)apr_pcalloc(ctx->pool,(dimension->nvalues+1)*sizeof(const char*));
   int i;
   for(i=0;i<dimension->nvalues;i++) {
      ret[i] = dimension->values[i];
   }
   ret[i]=NULL;
   return ret;
}

#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
static void _mapcache_dimension_values_parse_json(mapcache_context *ctx, mapcache_dimension *dim,
      cJSON *node) {
   cJSON *tmp;
   mapcache_dimension_values *dimension = (mapcache_dimension_values*)dim;
   tmp = cJSON_GetObjectItem(node,"values");
   if(tmp) {
      int i;
      dimension->values = (char**)apr_pcalloc(ctx->pool,cJSON_GetArraySize(tmp)*sizeof(char*));
      for(i=0;i<cJSON_GetArraySize(tmp);i++) {
         cJSON *val = cJSON_GetArrayItem(tmp,i);
         if(!val->valuestring || !*(val->valuestring)) {
            ctx->set_error(ctx,400,"invalid value for dimension %s",dim->name);
            return;
         }
         dimension->values[dimension->nvalues]=apr_pstrdup(ctx->pool,val->valuestring);
         dimension->nvalues++;
      }
   }
   tmp = cJSON_GetObjectItem(node,"case_sensitive");
   if(tmp && tmp->type == cJSON_True) {
      dimension->case_sensitive = 1;
   }
   if(!dimension->nvalues) {
      ctx->set_error(ctx, 400, "dimension \"%s\" has no values",dim->name);
      return;
   }
}
#endif

static void _mapcache_dimension_values_parse_xml(mapcache_context *ctx, mapcache_dimension *dim,
      ezxml_t node) {
   int count = 1;
   const char *entry = node->txt;
   if(!entry || !*entry) {
      ctx->set_error(ctx,400,"failed to parse dimension values: none supplied");
      return;
   }

   mapcache_dimension_values *dimension = (mapcache_dimension_values*)dim;
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

mapcache_dimension* mapcache_dimension_values_create(apr_pool_t *pool) {
   mapcache_dimension_values *dimension = apr_pcalloc(pool, sizeof(mapcache_dimension_values));
   dimension->dimension.type = MAPCACHE_DIMENSION_VALUES;
   dimension->nvalues = 0;
   dimension->dimension.validate = _mapcache_dimension_values_validate;
   dimension->dimension.configuration_parse_xml = _mapcache_dimension_values_parse_xml;
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
   dimension->dimension.configuration_parse_json = _mapcache_dimension_values_parse_json;
#endif
   dimension->dimension.print_ogc_formatted_values = _mapcache_dimension_values_print;
   return (mapcache_dimension*)dimension;
}

mapcache_dimension* mapcache_dimension_time_create(apr_pool_t *pool) {
   mapcache_dimension_time *dimension = apr_pcalloc(pool, sizeof(mapcache_dimension_time));
   dimension->dimension.type = MAPCACHE_DIMENSION_TIME;
   dimension->nintervals = 0;
//   dimension->dimension.validate = _mapcache_dimension_time_validate;
//   dimension->dimension.parse = _mapcache_dimension_time_parse;
//   dimension->dimension.print_ogc_formatted_values = _mapcache_dimension_time_print;
   return (mapcache_dimension*)dimension;
}

mapcache_dimension* mapcache_dimension_intervals_create(apr_pool_t *pool) {
   mapcache_dimension_intervals *dimension = apr_pcalloc(pool, sizeof(mapcache_dimension_intervals));
   dimension->dimension.type = MAPCACHE_DIMENSION_INTERVALS;
   dimension->nintervals = 0;
   dimension->dimension.validate = _mapcache_dimension_intervals_validate;
   dimension->dimension.configuration_parse_xml = _mapcache_dimension_intervals_parse_xml;
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
   dimension->dimension.configuration_parse_json = _mapcache_dimension_intervals_parse_json;
#endif
   dimension->dimension.print_ogc_formatted_values = _mapcache_dimension_intervals_print;
   return (mapcache_dimension*)dimension;
}
mapcache_dimension* mapcache_dimension_regex_create(apr_pool_t *pool) {
   mapcache_dimension_regex *dimension = apr_pcalloc(pool, sizeof(mapcache_dimension_regex));
   dimension->dimension.type = MAPCACHE_DIMENSION_REGEX;
#ifndef USE_PCRE
   dimension->regex = (regex_t*)apr_pcalloc(pool, sizeof(regex_t));
#endif
   dimension->dimension.validate = _mapcache_dimension_regex_validate;
   dimension->dimension.configuration_parse_xml = _mapcache_dimension_regex_parse_xml;
#ifdef ENABLE_UNMAINTAINED_JSON_PARSER
   dimension->dimension.configuration_parse_json = _mapcache_dimension_regex_parse_json;
#endif
   dimension->dimension.print_ogc_formatted_values = _mapcache_dimension_regex_print;
   return (mapcache_dimension*)dimension;
}
/* vim: ai ts=3 sts=3 et sw=3
*/

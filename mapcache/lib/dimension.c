/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: OGC dimensions
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
#include <apr_strings.h>
#include <math.h>
#include <sys/types.h>


static int _mapcache_dimension_intervals_validate(mapcache_context *ctx, mapcache_dimension *dim, char **value) {
   int i;
   char *endptr;
   mapcache_dimension_intervals *dimension;
   double val = strtod(*value,&endptr);
   *value = apr_psprintf(ctx->pool,"%g",val);
   if(*endptr != 0) {
      return MAPCACHE_FAILURE;
   }
   dimension = (mapcache_dimension_intervals*)dim;
   for(i=0;i<dimension->nintervals;i++) {
     double diff;
      mapcache_interval *interval = &dimension->intervals[i];
      if(val<interval->start || val>interval->end)
         continue;
      if(interval->resolution == 0)
         return MAPCACHE_SUCCESS;
      diff = fmod((val - interval->start),interval->resolution);
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


static void _mapcache_dimension_intervals_parse_xml(mapcache_context *ctx, mapcache_dimension *dim,
      ezxml_t node) {
  mapcache_dimension_intervals *dimension;
  char *key,*last;
  char *values;
   const char *entry = node->txt;
   int count = 1;
   if(!entry || !*entry) {
      ctx->set_error(ctx,400,"failed to parse dimension values: none supplied");
      return;
   }
   dimension = (mapcache_dimension_intervals*)dim;
   values = apr_pstrdup(ctx->pool,entry);
   
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


static void _mapcache_dimension_regex_parse_xml(mapcache_context *ctx, mapcache_dimension *dim,
      ezxml_t node) {
   mapcache_dimension_regex *dimension;
   int rc;
   const char *entry = node->txt;
   if(!entry || !*entry) {
      ctx->set_error(ctx,400,"failed to parse dimension regex: none supplied");
      return;
   }
   dimension = (mapcache_dimension_regex*)dim;
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
   rc = regcomp(dimension->regex, entry, REG_EXTENDED);
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


static void _mapcache_dimension_values_parse_xml(mapcache_context *ctx, mapcache_dimension *dim,
      ezxml_t node) {
   int count = 1;
   mapcache_dimension_values *dimension;
   const char *case_sensitive;
   char *key,*last;
   char *values;
   const char *entry = node->txt;
   if(!entry || !*entry) {
      ctx->set_error(ctx,400,"failed to parse dimension values: none supplied");
      return;
   }

   dimension = (mapcache_dimension_values*)dim;
   case_sensitive = ezxml_attr(node,"case_sensitive");
   if(case_sensitive && !strcasecmp(case_sensitive,"true")) {
      dimension->case_sensitive = 1;
   }
   
   values = apr_pstrdup(ctx->pool,entry);
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
   dimension->dimension.print_ogc_formatted_values = _mapcache_dimension_regex_print;
   return (mapcache_dimension*)dimension;
}
/* vim: ai ts=3 sts=3 et sw=3
*/

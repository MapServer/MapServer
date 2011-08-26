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

/** \addtogroup services */
/** @{ */


void geocache_service_dispatch_request(geocache_context *ctx, geocache_request **request, char *pathinfo, apr_table_t *params, geocache_cfg *config) {
   int i;
   
   /* skip empty pathinfo */
   if(!pathinfo) {
      ctx->set_error(ctx,404,"missing a service");
      return;
   }
   
   /*skip leading /'s */
   while((*pathinfo) == '/')
      ++pathinfo;
   
   for(i=0;i<GEOCACHE_SERVICES_COUNT;i++) {
      /* loop through the services that have been configured */
      int prefixlen;
      geocache_service *service = NULL;
      service = config->services[i];
      if(!service) continue; /* skip an unconfigured service */
      prefixlen = strlen(service->url_prefix);
      if(strncmp(service->url_prefix,pathinfo, prefixlen)) continue; /*skip a service who's prefix does not correspond */
      if(*(pathinfo+prefixlen)!='/' && *(pathinfo+prefixlen)!='\0') continue; /*we matched the prefix but there are trailing characters*/
      pathinfo += prefixlen; /* advance pathinfo to after the service prefix */
      service->parse_request(ctx,service,request,pathinfo,params,config);
      GC_CHECK_ERROR(ctx);
      (*request)->service = service;
      return;
   }
   ctx->set_error(ctx,404,"unknown service %s",pathinfo);
}

/** @} */





/* vim: ai ts=3 sts=3 et sw=3
*/

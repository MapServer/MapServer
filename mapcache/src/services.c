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

/**
 * \brief parse a WMS request
 * \private \memberof geocache_service_wms
 * \sa geocache_service::parse_request()
 */
geocache_request* _geocache_service_wms_parse_request(geocache_context *ctx, char *pathinfo, apr_table_t *params, geocache_cfg *config) {
   char *str = NULL;
   double *bbox;
   geocache_request *request = NULL;

   str = (char*)apr_table_get(params,"REQUEST");
   if(!str)
      str = (char*)apr_table_get(params,"request");
   if(!str || strcasecmp(str,"getmap")) {
      return NULL;
   }


   str = (char*)apr_table_get(params,"BBOX");
   if(!str)
      str = (char*)apr_table_get(params,"bbox");
   if(!str) {
      ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms request with no bbox");
      return NULL;
   } else {
      int nextents;
      if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(ctx, str,',',&bbox,&nextents) ||
            nextents != 4) {
         ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms request with invalid bbox");
         return NULL;
      }
   }

   str = (char*)apr_table_get(params,"LAYERS");
   if(!str)
      str = (char*)apr_table_get(params,"layers");
   if(!str) {
      ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms request with no layers");
      return NULL;
   } else {
      char *last, *key;
      int count=1;
      char *sep=",";
      request = (geocache_request*)apr_pcalloc(ctx->pool,sizeof(geocache_request));
      str = apr_pstrdup(ctx->pool,str);
      for(key=str;*key;key++) if(*key == ',') count++;
      request->ntiles = 0;
      request->tiles = (geocache_tile**)apr_pcalloc(ctx->pool,count * sizeof(geocache_tile*));
      for (key = apr_strtok(str, sep, &last); key != NULL;
            key = apr_strtok(NULL, sep, &last)) {
         geocache_tile *tile;
         geocache_tileset *tileset = geocache_configuration_get_tileset(config,key);
         if(!tileset) {
            ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms request with invalid layer %s", key);
            return NULL;
         }
         tile = geocache_tileset_tile_create(ctx->pool, tileset);
         if(!tile) {
            ctx->set_error(ctx, GEOCACHE_ALLOC_ERROR, "failed to allocate tile");
            return NULL;
         }
         geocache_tileset_tile_lookup(ctx, tile, bbox);
         if(GC_HAS_ERROR(ctx)) {
            return NULL;
         }
         request->tiles[request->ntiles++] = tile;
      }
   }

   /* TODO: check the size and srs we've received are compatible */

   return request;
}

/**
 * \brief parse a TMS request
 * \private \memberof geocache_service_tms
 * \sa geocache_service::parse_request()
 */
geocache_request* _geocache_service_tms_parse_request(geocache_context *ctx, char *pathinfo, apr_table_t *params, geocache_cfg *config) {
   int index = 0;
   char *last, *key, *endptr;
   geocache_tileset *tileset = NULL;
   geocache_tile *tile = NULL;
   geocache_request *request = NULL;
   /* parse a path_info like /1.0.0/global_mosaic/0/0/0.jpg */ 
   for (key = apr_strtok(pathinfo, "/", &last); key != NULL;
         key = apr_strtok(NULL, "/", &last)) {
      if(!*key) continue; /* skip an empty string, could happen if the url contains // */
      switch(++index) {
      case 1: /* version */
         if(strcmp("1.0.0",key)) {
            //r->log(r,GEOCACHE_INFO, "received tms request with invalid version %s", key);
            return NULL;
         }
         break;
      case 2: /* layer name */
         tileset = geocache_configuration_get_tileset(config,key);
         if(!tileset) {
            ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request with invalid layer %s", key);
            return NULL;
         }
         request = (geocache_request*)apr_pcalloc(ctx->pool,sizeof(geocache_request));
         request->ntiles = 1;
         request->tiles = (geocache_tile**)apr_pcalloc(ctx->pool,sizeof(geocache_tile*)); 
         tile = geocache_tileset_tile_create(ctx->pool, tileset);
         request->tiles[0]=tile;
         break;
      case 3:
         tile->z = (int)strtol(key,&endptr,10);
         if(*endptr != 0) {
            ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with invalid z %s", pathinfo, key);
            return NULL;
         }
         break;
      case 4:
         tile->x = (int)strtol(key,&endptr,10);
         if(*endptr != 0) {
            ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with invalid x %s", pathinfo, key);
            return NULL;
         }
         break;
      case 5:
         tile->y = (int)strtol(key,&endptr,10);
         if(*endptr != '.') {
            ctx->log(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with invalid y %s", pathinfo, key);
            return NULL;
         }
         break;
      default:
         ctx->log(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with invalid parameter %s", pathinfo, key);
         return NULL;
      }
   }
   if(index == 5) {
      return request;
   }
   else {
      ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with wrong number of arguments", pathinfo);
      return NULL;
   }
}

geocache_service* geocache_service_wms_create(geocache_context *ctx) {
   geocache_service_wms* service = (geocache_service_wms*)apr_pcalloc(ctx->pool, sizeof(geocache_service_wms));
   if(!service) {
      ctx->set_error(ctx, GEOCACHE_ALLOC_ERROR, "failed to allocate wms service");
      return NULL;
   }
   service->service.type = GEOCACHE_SERVICE_WMS;
   service->service.parse_request = _geocache_service_wms_parse_request;
   return (geocache_service*)service;
}

geocache_service* geocache_service_tms_create(geocache_context *ctx) {
   geocache_service_tms* service = (geocache_service_tms*)apr_pcalloc(ctx->pool, sizeof(geocache_service_tms));
   if(!service) {
      ctx->set_error(ctx, GEOCACHE_ALLOC_ERROR, "failed to allocate tms service");
      return NULL;
   }
   service->service.type = GEOCACHE_SERVICE_TMS;
   service->service.parse_request = _geocache_service_tms_parse_request;
   return (geocache_service*)service;
}

/** @} */






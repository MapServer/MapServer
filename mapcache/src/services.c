/*
 * services.c
 *
 *  Created on: Oct 11, 2010
 *      Author: tom
 */

#include "geocache.h"
#include <http_log.h>
#include <apr_strings.h>

geocache_request* _geocache_service_wms_parse_request(request_rec *r, apr_table_t *params, geocache_cfg *config) {
   char *str = NULL;
   double *bbox;
   geocache_request *request;

   str = (char*)apr_table_get(params,"REQUEST");
   if(!str)
      str = (char*)apr_table_get(params,"request");
   if(!str || strcasecmp(str,"getmap")) {
      return NULL;
   }
   request = (geocache_request*)apr_pcalloc(r->pool,sizeof(geocache_request));

   str = (char*)apr_table_get(params,"BBOX");
   if(!str)
      str = (char*)apr_table_get(params,"bbox");
   if(!str) {
      ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "received wms request with no bbox");
      return NULL;
   } else {
      int nextents;
      if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(str,',',&bbox,&nextents,r->pool) ||
            nextents != 4) {
         ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "received wms request with invalid bbox");
         return NULL;
      }
   }

   str = (char*)apr_table_get(params,"LAYERS");
   if(!str)
      str = (char*)apr_table_get(params,"layers");
   if(!str) {
      ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "received wms request with no layers");
      return NULL;
   } else {
      char *last, *key;
      int count=1, ret;
      char *sep=",";
      str = apr_pstrdup(r->pool,str);
      for(key=str;*key;key++) if(*key == ',') count++;
      request->ntiles = 0;
      request->tiles = (geocache_tile**)apr_pcalloc(r->pool,count * sizeof(geocache_tile*));
      for (key = apr_strtok(str, sep, &last); key != NULL;
            key = apr_strtok(NULL, sep, &last)) {
         geocache_tile *tile;
         geocache_tileset *tileset = geocache_configuration_get_tileset(config,key);
         if(!tileset) {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
                  "received wms request with invalid layer %s", key);
            return NULL;
         }
         tile = geocache_tileset_tile_create(tileset,r->pool);
         ret = geocache_tileset_tile_lookup(tile,bbox,r);
         if(ret != GEOCACHE_SUCCESS) {
            return NULL;
         }
         request->tiles[request->ntiles++] = tile;
      }
   }
   
   /* TODO: check the size and srs we've received are compatible */
   
   return request;

}

geocache_request* _geocache_service_tms_parse_request(request_rec *r, apr_table_t *params, geocache_cfg *config) {
   return NULL;
}


geocache_service* geocache_service_wms_create(apr_pool_t *pool) {
   geocache_service_wms* service = (geocache_service_wms*)apr_pcalloc(pool, sizeof(geocache_service_wms));
   service->service.type = GEOCACHE_SERVICE_WMS;
   service->service.parse_request = _geocache_service_wms_parse_request;
   return (geocache_service*)service;
}

geocache_service* geocache_service_tms_create(apr_pool_t *pool) {
   geocache_service_tms* service = (geocache_service_tms*)apr_pcalloc(pool, sizeof(geocache_service_tms));
   service->service.type = GEOCACHE_SERVICE_TMS;
   service->service.parse_request = _geocache_service_tms_parse_request;
   return (geocache_service*)service;
}






/*
 * services.c
 *
 *  Created on: Oct 11, 2010
 *      Author: tom
 */

#include "yatc.h"
#include <http_log.h>
#include <apr_strings.h>

yatc_request* _yatc_service_wms_parse_request(request_rec *r, apr_table_t *params, yatc_cfg *config) {
   char *str = NULL;
   double *bbox;
   yatc_request *request;

   str = (char*)apr_table_get(params,"REQUEST");
   if(!str)
      str = (char*)apr_table_get(params,"request");
   if(!str || strcasecmp(str,"getmap")) {
      return NULL;
   }
   request = (yatc_request*)apr_pcalloc(r->pool,sizeof(yatc_request));

   str = (char*)apr_table_get(params,"BBOX");
   if(!str)
      str = (char*)apr_table_get(params,"bbox");
   if(!str) {
      ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "received wms request with no bbox");
      return NULL;
   } else {
      int nextents;
      if(YATC_SUCCESS != yatc_util_extract_double_list(str,',',&bbox,&nextents,r->pool) ||
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
      request->tiles = (yatc_tile**)apr_pcalloc(r->pool,count * sizeof(yatc_tile*));
      for (key = apr_strtok(str, sep, &last); key != NULL;
            key = apr_strtok(NULL, sep, &last)) {
         yatc_tile *tile;
         yatc_tileset *tileset = yatc_configuration_get_tileset(config,key);
         if(!tileset) {
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
                  "received wms request with invalid layer %s", key);
            return NULL;
         }
         tile = yatc_tileset_tile_create(tileset,r->pool);
         ret = yatc_tileset_tile_lookup(tile,bbox,r);
         if(ret != YATC_SUCCESS) {
            return NULL;
         }
         request->tiles[request->ntiles++] = tile;
      }
   }
   
   /* TODO: check the size and srs we've received are compatible */
   
   return request;

}

yatc_request* _yatc_service_tms_parse_request(request_rec *r, apr_table_t *params, yatc_cfg *config) {
   return NULL;
}


yatc_service* yatc_service_wms_create(apr_pool_t *pool) {
   yatc_service_wms* service = (yatc_service_wms*)apr_pcalloc(pool, sizeof(yatc_service_wms));
   service->service.type = YATC_SERVICE_WMS;
   service->service.parse_request = _yatc_service_wms_parse_request;
   return (yatc_service*)service;
}

yatc_service* yatc_service_tms_create(apr_pool_t *pool) {
   yatc_service_tms* service = (yatc_service_tms*)apr_pcalloc(pool, sizeof(yatc_service_tms));
   service->service.type = YATC_SERVICE_TMS;
   service->service.parse_request = _yatc_service_tms_parse_request;
   return (yatc_service*)service;
}






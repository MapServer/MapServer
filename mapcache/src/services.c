/*
 * services.c
 *
 *  Created on: Oct 11, 2010
 *      Author: tom
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
geocache_request* _geocache_service_wms_parse_request(geocache_context *r, char *pathinfo, apr_table_t *params, geocache_cfg *config) {
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
      r->log(r,GEOCACHE_INFO, "received wms request with no bbox");
      return NULL;
   } else {
      int nextents;
      if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(str,',',&bbox,&nextents,r) ||
            nextents != 4) {
         r->log(r,GEOCACHE_INFO, "received wms request with invalid bbox");
         return NULL;
      }
   }

   str = (char*)apr_table_get(params,"LAYERS");
   if(!str)
      str = (char*)apr_table_get(params,"layers");
   if(!str) {
      r->log(r,GEOCACHE_INFO, "received wms request with no layers");
      return NULL;
   } else {
      char *last, *key;
      int count=1, ret;
      char *sep=",";
      request = (geocache_request*)apr_pcalloc(r->pool,sizeof(geocache_request));
      str = apr_pstrdup(r->pool,str);
      for(key=str;*key;key++) if(*key == ',') count++;
      request->ntiles = 0;
      request->tiles = (geocache_tile**)apr_pcalloc(r->pool,count * sizeof(geocache_tile*));
      for (key = apr_strtok(str, sep, &last); key != NULL;
            key = apr_strtok(NULL, sep, &last)) {
         geocache_tile *tile;
         geocache_tileset *tileset = geocache_configuration_get_tileset(config,key);
         if(!tileset) {
            r->log(r,GEOCACHE_INFO, "received wms request with invalid layer %s", key);
            return NULL;
         }
         tile = geocache_tileset_tile_create(tileset,r);
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

geocache_request* _geocache_service_wmts_parse_request(geocache_context *r, char *pathinfo, apr_table_t *params, geocache_cfg *config) {
   char *str = NULL;
   double scale,res;
   int tilerow,tilecol;
   char *endptr;
   int maxY;
   double geocache_meters_per_unit[] = {
         -1, /*GEOCACHE_UNIT_UNSET*/
         1.0, /*GEOCACHE_UNIT_METERS*/
         111118.752, /*GEOCACHE_UNIT_DEGREES*/
         0.3048 /*GEOCACHE_UNIT_FEET*/
   };
   geocache_request *request = NULL;

   str = (char*)apr_table_get(params,"SCALE");
   if(!str)
      str = (char*)apr_table_get(params,"scale");
   if(!str) {
      return NULL;
   } else {
      scale = strtod(str,&endptr);
      if(*endptr != 0) {
         r->log(r,GEOCACHE_INFO, "received wmts request with invalid scale %s", str);
         return NULL;
      }
   }

   str = (char*)apr_table_get(params,"TILEROW");
   if(!str)
      str = (char*)apr_table_get(params,"tilerow");
   if(!str) {
      return NULL;
   } else {
      tilerow = strtol(str,&endptr,10);
      if(*endptr != 0) {
         r->log(r,GEOCACHE_INFO, "received wmts request with invalid tilerow %s", str);
         return NULL;
      }
   }

   str = (char*)apr_table_get(params,"TILECOL");
   if(!str)
      str = (char*)apr_table_get(params,"tilecol");
   if(!str) {
      return NULL;
   } else {
      tilecol = strtol(str,&endptr,10);
      if(*endptr != 0) {
         r->log(r,GEOCACHE_INFO, "received wmts request with invalid tilecol %s", str);
         return NULL;
      }
   }
   str = (char*)apr_table_get(params,"LAYER");
   if(!str)
      str = (char*)apr_table_get(params,"layer");
   if(!str) {
      r->log(r,GEOCACHE_INFO, "received wmts request with no layer");
      return NULL;
   } else {
      geocache_tileset *tileset = geocache_configuration_get_tileset(config,str);
      if(!tileset) {
         r->log(r,GEOCACHE_INFO, "received wmts request with invalid layer %s",str);
         return NULL;
      }
      if(tileset->units == GEOCACHE_UNIT_UNSET) {
         r->log(r,GEOCACHE_INFO, "received wmts on layer %s that has no unit configured",str);
         return NULL;
      }
      request = (geocache_request*)apr_pcalloc(r->pool,sizeof(geocache_request));
      request->ntiles = 1;
      request->tiles = (geocache_tile**)apr_pcalloc(r->pool,sizeof(geocache_tile*)); 
      request->tiles[0] = geocache_tileset_tile_create(tileset,r);
      res = .00028 * scale / geocache_meters_per_unit[tileset->units];
      if(GEOCACHE_SUCCESS != geocache_tileset_get_level(tileset, &res, &request->tiles[0]->z, r)) {
         r->log(r,GEOCACHE_INFO, "received wmts on layer %s with invalid resolution / level", tileset->name);
         return NULL;
      }
      
      maxY = round((tileset->extent[3] - tileset->extent[1])/(res * tileset->tile_sy)) -1;
      request->tiles[0]->x = tilecol;
      request->tiles[0]->y = maxY - tilerow;
      return request;
   }
}

/**
 * \brief parse a TMS request
 * \private \memberof geocache_service_tms
 * \sa geocache_service::parse_request()
 */
geocache_request* _geocache_service_tms_parse_request(geocache_context *r, char *pathinfo, apr_table_t *params, geocache_cfg *config) {
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
            r->log(r,GEOCACHE_INFO, "received tms request with invalid version %s", key);
            return NULL;
         }
         break;
      case 2: /* layer name */
         tileset = geocache_configuration_get_tileset(config,key);
         if(!tileset) {
            r->log(r,GEOCACHE_INFO, "received tms request with invalid layer %s", key);
            return NULL;
         }
         request = (geocache_request*)apr_pcalloc(r->pool,sizeof(geocache_request));
         request->ntiles = 1;
         request->tiles = (geocache_tile**)apr_pcalloc(r->pool,sizeof(geocache_tile*)); 
         tile = geocache_tileset_tile_create(tileset,r);
         request->tiles[0]=tile;
         break;
      case 3:
         tile->z = (int)strtol(key,&endptr,10);
         if(*endptr != 0) {
            r->log(r,GEOCACHE_INFO, "received tms request %s with invalid z %s", pathinfo, key);
            return NULL;
         }
         break;
      case 4:
         tile->x = (int)strtol(key,&endptr,10);
         if(*endptr != 0) {
            r->log(r,GEOCACHE_INFO, "received tms request %s with invalid x %s", pathinfo, key);
            return NULL;
         }
         break;
      case 5:
         tile->y = (int)strtol(key,&endptr,10);
         if(*endptr != '.') {
            r->log(r,GEOCACHE_INFO, "received tms request %s with invalid y %s", pathinfo, key);
            return NULL;
         }
         break;
      default:
         r->log(r,GEOCACHE_INFO, "received tms request %s with invalid parameter %s", pathinfo, key);
         return NULL;
      }
   }
   if(index == 5) {
      return request;
   }
   else {
      r->log(r,GEOCACHE_INFO, "received tms request %s with wrong number of arguments", pathinfo);
      return NULL;
   }
}

geocache_service* geocache_service_wms_create(geocache_context *r) {
   geocache_service_wms* service = (geocache_service_wms*)apr_pcalloc(r->pool, sizeof(geocache_service_wms));
   service->service.type = GEOCACHE_SERVICE_WMS;
   service->service.parse_request = _geocache_service_wms_parse_request;
   return (geocache_service*)service;
}

geocache_service* geocache_service_tms_create(geocache_context *r) {
   geocache_service_tms* service = (geocache_service_tms*)apr_pcalloc(r->pool, sizeof(geocache_service_tms));
   service->service.type = GEOCACHE_SERVICE_TMS;
   service->service.parse_request = _geocache_service_tms_parse_request;
   return (geocache_service*)service;
}

geocache_service* geocache_service_wmts_create(geocache_context *r) {
   geocache_service_wmts* service = (geocache_service_wmts*)apr_pcalloc(r->pool, sizeof(geocache_service_wmts));
   service->service.type = GEOCACHE_SERVICE_WMTS;
   service->service.parse_request = _geocache_service_wmts_parse_request;
   return (geocache_service*)service;
}

/** @} */






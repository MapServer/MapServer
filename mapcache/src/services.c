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
static char *wms_capabilities_preamble = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"no\" ?>\n"
        "<!DOCTYPE WMT_MS_Capabilities SYSTEM\n"
            "\"http://schemas.opengeospatial.net/wms/1.1.1/WMS_MS_Capabilities.dtd\" [\n"
              "<!ELEMENT VendorSpecificCapabilities (TileSet*) >\n"
              "<!ELEMENT TileSet (SRS, BoundingBox?, Resolutions, Width, Height, Format, Layers*, Styles*) >\n"
              "<!ELEMENT Resolutions (#PCDATA) >\n"
              "<!ELEMENT Width (#PCDATA) >\n"
              "<!ELEMENT Height (#PCDATA) >\n"
              "<!ELEMENT Layers (#PCDATA) >\n"
              "<!ELEMENT Styles (#PCDATA) > ]>\n"
        "<WMT_MS_Capabilities version=\"1.1.1\">\n"
          "<Service>\n"
            "<Name>OGC:WMS</Name>\n"
            "<Title></Title>\n"
            "<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/>\n"
          "</Service>\n"
          "<Capability>\n"
            "<Request>\n"
              "<GetCapabilities>\n"
                "<Format>application/vnd.ogc.wms_xml</Format>\n"
                "<DCPType>\n"
                  "<HTTP>\n"
                    "<Get><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/></Get>\n"
                  "</HTTP>\n"
                "</DCPType>\n"
              "</GetCapabilities>\n"
              "<GetMap>\n"
                "<Format>image/png</Format>\n"
                "<Format>image/jpeg</Format>\n"
                "<DCPType>\n"
                  "<HTTP>\n"
                    "<Get><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s\"/></Get>\n"
                  "</HTTP>\n"
                "</DCPType>\n"
              "</GetMap>\n"
            "</Request>\n"
            "<Exception>\n"
              "<Format>text/plain</Format>\n"
            "</Exception>\n"
            "<VendorSpecificCapabilities>\n";


static char *wms_tileset = "<TileSet>\n"
                "<SRS>%s</SRS>\n"
                "<BoundingBox SRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n"
                "<Resolutions>%s</Resolutions>\n"
                "<Width>%d</Width>\n"
                "<Height>%d</Height>\n"
                "<Format>image/png</Format>\n"
                "<Layers>%s</Layers>\n"
                "<Styles></Styles>\n"
              "</TileSet>\n";

static char *wms_layer = "<Layer queryable=\"0\" opaque=\"0\" cascaded=\"1\">\n"
              "<Name>%s</Name>\n"
              "<Title>%s</Title>\n"
              "<SRS>%s</SRS>\n"
              "<BoundingBox srs=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n"
            "</Layer>\n";

geocache_request* _geocache_service_wms_capabilities(geocache_context *ctx, char *uri, geocache_cfg *cfg) {
   geocache_request *request = (geocache_request*)apr_pcalloc(ctx->pool,sizeof(geocache_request));
   request->type = GEOCACHE_REQUEST_GET_CAPABILITIES;
   char *host = uri;
   char *caps = apr_psprintf(ctx->pool,wms_capabilities_preamble,host,host,host);
   apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,cfg->tilesets);

   while(tileindex_index) {
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
      char *resolutions="";
      int i;
      for(i=0;i<tileset->grid->levels;i++) {
         resolutions = apr_psprintf(ctx->pool,"%s%f ",resolutions,tileset->grid->resolutions[i]);
      }
      char *tilesetcaps = apr_psprintf(ctx->pool,wms_tileset,
            tileset->grid->srs,
            tileset->grid->srs,
            tileset->grid->extents[0][0],
            tileset->grid->extents[0][1],
            tileset->grid->extents[0][2],
            tileset->grid->extents[0][3],
            resolutions,
            tileset->grid->tile_sx,
            tileset->grid->tile_sy,
            tileset->name);
      caps = apr_psprintf(ctx->pool,"%s%s",caps,tilesetcaps);
      tileindex_index = apr_hash_next(tileindex_index);
   }

   caps = apr_psprintf(ctx->pool,"%s%s",caps,"</VendorSpecificCapabilities>\n"
            "<UserDefinedSymbolization SupportSLD=\"0\" UserLayer=\"0\" UserStyle=\"0\" RemoteWFS=\"0\"/>\n"
            "<Layer>\n");

   tileindex_index = apr_hash_first(ctx->pool,cfg->tilesets);
   while(tileindex_index) {
         geocache_tileset *tileset;
         const void *key; apr_ssize_t keylen;
         apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
         char *layercaps = apr_psprintf(ctx->pool,wms_layer,
               tileset->name,
               tileset->name,
               tileset->grid->srs,
               tileset->grid->srs,
               tileset->grid->extents[0][0],
               tileset->grid->extents[0][1],
               tileset->grid->extents[0][2],
               tileset->grid->extents[0][3]);
         caps = apr_psprintf(ctx->pool,"%s%s",caps,layercaps);
         tileindex_index = apr_hash_next(tileindex_index);
      }

   caps = apr_psprintf(ctx->pool,"%s%s",caps,"</Layer>\n"
             "</Capability>\n"
           "</WMT_MS_Capabilities>\n");
   request->capabilities = caps;
   return request;
}

/**
 * \brief parse a WMS request
 * \private \memberof geocache_service_wms
 * \sa geocache_service::parse_request()
 */
geocache_request* _geocache_service_wms_parse_request(geocache_context *ctx, char *uri, char *pathinfo, apr_table_t *params, geocache_cfg *config) {
   char *str = NULL, *srs=NULL;
   int width=0, height=0;
   double *bbox;
   geocache_request *request = NULL;
   
   str = (char*)apr_table_get(params,"SERVICE");
   if(!str)
      str = (char*)apr_table_get(params,"service");
   if(!str || strcasecmp(str,"wms")) {
      return NULL;
   }
      
   str = (char*)apr_table_get(params,"REQUEST");
   if(!str)
      str = (char*)apr_table_get(params,"request");
   if(!str) {
      ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms with no request");
      return NULL;
   }
   if( ! strcasecmp(str,"getcapabilities") ) {
      return _geocache_service_wms_capabilities(ctx, uri, config);
   } else if( strcasecmp(str,"getmap")) {
      ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms with invalid request %s",str);
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

   str = (char*)apr_table_get(params,"WIDTH");
   if(!str)
      str = (char*)apr_table_get(params,"width");
   if(!str) {
      ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms request with no width");
      return NULL;
   } else {
      char *endptr;
      width = (int)strtol(str,&endptr,10);
      if(*endptr != 0 || width <= 0) {
         ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms request with invalid width");
         return NULL;
      }
   }

   str = (char*)apr_table_get(params,"HEIGHT");
   if(!str)
      str = (char*)apr_table_get(params,"height");
   if(!str) {
      ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms request with no height");
      return NULL;
   } else {
      char *endptr;
      height = (int)strtol(str,&endptr,10);
      if(*endptr != 0 || height <= 0) {
         ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms request with invalid height");
         return NULL;
      }
   }

   srs = (char*)apr_table_get(params,"SRS");
   if(!srs)
      srs = (char*)apr_table_get(params,"srs");
   if(!srs) {
      ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR, "received wms request with no srs");
      return NULL;
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
      request->type = GEOCACHE_REQUEST_GET_TILE;
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
         if(strcasecmp(tileset->grid->srs,srs)) {
            ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR,
                  "received wms request with invalid srs (got %s, expected %s)",
                  srs,tileset->grid->srs);
            return NULL;
         }
         if(tileset->grid->tile_sx != width) {
            ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR,
                  "received wms request with invalid width (got %d, expected %d)",
                  width,tileset->grid->tile_sx);
            return NULL;
         }
         if(tileset->grid->tile_sy != height) {
            ctx->set_error(ctx, GEOCACHE_REQUEST_ERROR,
                  "received wms request with invalid height (got %d, expected %d)",
                  height,tileset->grid->tile_sy);
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
   return request;
}

/**
 * \brief parse a TMS request
 * \private \memberof geocache_service_tms
 * \sa geocache_service::parse_request()
 */
geocache_request* _geocache_service_tms_parse_request(geocache_context *ctx, char *uri, char *pathinfo, apr_table_t *params, geocache_cfg *config) {
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






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


static char *wms_capabilities_preamble = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
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
            "<Title>%s</Title>\n"
            "<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s/wms?\"/>\n"
          "</Service>\n"
          "<Capability>\n"
            "<Request>\n"
              "<GetCapabilities>\n"
                "<Format>application/vnd.ogc.wms_xml</Format>\n"
                "<DCPType>\n"
                  "<HTTP>\n"
                    "<Get><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s/wms?\"/></Get>\n"
                  "</HTTP>\n"
                "</DCPType>\n"
              "</GetCapabilities>\n"
              "<GetMap>\n"
                "<Format>image/png</Format>\n"
                "<Format>image/jpeg</Format>\n"
                "<DCPType>\n"
                  "<HTTP>\n"
                    "<Get><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s/wms?\"/></Get>\n"
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
                "<Format>%s</Format>\n"
                "<Layers>%s</Layers>\n"
                "<Styles></Styles>\n"
              "</TileSet>\n";

static char *wms_bbox = 
               "<BoundingBox srs=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n"
               "<SRS>%s</SRS>\n";
static char *wms_layer = "<Layer queryable=\"0\" opaque=\"0\" cascaded=\"1\">\n"
              "<Name>%s</Name>\n"
              "<Title>%s</Title>\n"
              "<Abstract>%s</Abstract>\n"
              "%s" /*srs and bboxes*/
              "%s" /*dimensions*/
            "</Layer>\n";

void _create_capabilities_wms(geocache_context *ctx, geocache_request_get_capabilities *req, char *guessed_url, char *path_info, geocache_cfg *cfg) {
   geocache_request_get_capabilities_wms *request = (geocache_request_get_capabilities_wms*)req;
#ifdef DEBUG
   if(request->request.request.type != GEOCACHE_REQUEST_GET_CAPABILITIES) {
      ctx->set_error(ctx,400,"wrong wms capabilities request");
      return;
   }
#endif
   const char *title;
   const char *url = apr_table_get(cfg->metadata,"url");
   if(!url) {
      url = guessed_url;
   }
   
   title = apr_table_get(cfg->metadata,"title");
   if(!title) {
      title = "no title set, add some in metadata";
   }
   char *caps = apr_psprintf(ctx->pool,wms_capabilities_preamble,title,url,url,url);
   apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,cfg->tilesets);

   while(tileindex_index) {
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
      geocache_grid_link *grid_link = APR_ARRAY_IDX(tileset->grid_links,0,geocache_grid_link*);
      geocache_grid *grid = grid_link->grid;
      char *resolutions="";
      
      int i;
      for(i=0;i<grid->nlevels;i++) {
         resolutions = apr_psprintf(ctx->pool,"%s%.20f ",resolutions,grid->levels[i]->resolution);
      }
      char *tilesetcaps = apr_psprintf(ctx->pool,wms_tileset,
            grid->srs,
            grid->srs,
            grid->extent[0],
            grid->extent[1],
            grid->extent[2],
            grid->extent[3],
            resolutions,
            grid->tile_sx,
            grid->tile_sy,
            tileset->format->mime_type,
            tileset->name);
      caps = apr_psprintf(ctx->pool,"%s%s",caps,tilesetcaps);
      tileindex_index = apr_hash_next(tileindex_index);
   }

   caps = apr_psprintf(ctx->pool,"%s%s",caps,"</VendorSpecificCapabilities>\n"
            "<UserDefinedSymbolization SupportSLD=\"0\" UserLayer=\"0\" UserStyle=\"0\" RemoteWFS=\"0\"/>\n"
            "<Layer>\n");

   tileindex_index = apr_hash_first(ctx->pool,cfg->tilesets);
   while(tileindex_index) {
         int i;
         geocache_tileset *tileset;
         const void *key; apr_ssize_t keylen;
         apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
         char *srss="";
         const char *title = apr_table_get(tileset->metadata,"title");
         if(!title) {
            title = "no title set, add some in metadata";
         }
         const char *abstract = apr_table_get(tileset->metadata,"abstract");
         if(!abstract) {
            abstract = "no abstract set, add some in metadata";
         }
         char *dimensions = "";
         if(tileset->dimensions) {
            for(i=0;i<tileset->dimensions->nelts;i++) {
               geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
               dimensions = apr_psprintf(ctx->pool,"%s"
                     "<Dimension name=\"%s\" default=\"%s\"",
                     dimensions,
                     dimension->name,
                     dimension->default_value);
               if(dimension->unit) {
                  dimensions = apr_pstrcat(ctx->pool,dimensions,
                        " units=\"",dimension->unit,"\"",NULL);
               }
               const char **value = dimension->print_ogc_formatted_values(ctx,dimension);
               dimensions = apr_pstrcat(ctx->pool,dimensions,">",*value,NULL);
               value++;
               while(*value) {
                  dimensions = apr_pstrcat(ctx->pool,dimensions,",",*value,NULL);
                  value++;
               }
               dimensions = apr_pstrcat(ctx->pool,dimensions,"</Dimension>\n",NULL);
            }
         }
         for(i=0;i<tileset->grid_links->nelts;i++) {
            geocache_grid *grid = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*)->grid;
            char *bbox = apr_psprintf(ctx->pool,wms_bbox,
                  grid->srs,
                  grid->extent[0],
                  grid->extent[1],
                  grid->extent[2],
                  grid->extent[3],
                  grid->srs);
            srss = apr_pstrcat(ctx->pool,srss,bbox,NULL);
         }
         char *layercaps = apr_psprintf(ctx->pool,wms_layer,
               tileset->name,
               title,
               abstract,
               srss,
               dimensions);
         caps = apr_psprintf(ctx->pool,"%s%s",caps,layercaps);
         tileindex_index = apr_hash_next(tileindex_index);
      }

   caps = apr_psprintf(ctx->pool,"%s%s",caps,"</Layer>\n"
             "</Capability>\n"
           "</WMT_MS_Capabilities>\n");
   request->request.capabilities = caps;
   request->request.mime_type = apr_pstrdup(ctx->pool,"text/xml");
}

/**
 * \brief parse a WMS request
 * \private \memberof geocache_service_wms
 * \sa geocache_service::parse_request()
 */
void _geocache_service_wms_parse_request(geocache_context *ctx, geocache_service *this, geocache_request **request,
      const char *pathinfo, apr_table_t *params, geocache_cfg *config) {
   const char *str = NULL;
   const char *srs=NULL;
   int width=0, height=0;
   double *bbox;
   
   str = apr_table_get(params,"SERVICE");
   if(!str) {
      ctx->set_error(ctx,400,"received wms request with no service param");
      return;
   }
   if( strcasecmp(str,"wms") ) {
      ctx->set_error(ctx,400,"received wms request with invalid service param %s", str);
      return;
   }
      
   str = apr_table_get(params,"REQUEST");
   if(!str) {
      ctx->set_error(ctx, 400, "received wms with no request");
      return;
   }
   if( ! strcasecmp(str,"getcapabilities") ) {
      *request = (geocache_request*)
            apr_pcalloc(ctx->pool,sizeof(geocache_request_get_capabilities_wms));
      (*request)->type = GEOCACHE_REQUEST_GET_CAPABILITIES;
      return; /* OK */
   } else if( strcasecmp(str,"getmap")) {
      ctx->set_error(ctx, 501, "received wms with invalid request %s",str);
      return;
   }


   str = apr_table_get(params,"BBOX");
   if(!str) {
      ctx->set_error(ctx, 400, "received wms request with no bbox");
      return;
   } else {
      int nextents;
      if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(ctx, str,',',&bbox,&nextents) ||
            nextents != 4) {
         ctx->set_error(ctx, 400, "received wms request with invalid bbox");
         return;
      }
   }

   str = apr_table_get(params,"WIDTH");
   if(!str) {
      ctx->set_error(ctx, 400, "received wms request with no width");
      return;
   } else {
      char *endptr;
      width = (int)strtol(str,&endptr,10);
      if(*endptr != 0 || width <= 0) {
         ctx->set_error(ctx, 400, "received wms request with invalid width");
         return;
      }
   }

   str = apr_table_get(params,"HEIGHT");
   if(!str) {
      ctx->set_error(ctx, 400, "received wms request with no height");
      return;
   } else {
      char *endptr;
      height = (int)strtol(str,&endptr,10);
      if(*endptr != 0 || height <= 0) {
         ctx->set_error(ctx, 400, "received wms request with invalid height");
         return;
      }
   }

   srs = apr_table_get(params,"SRS");
   if(!srs) {
      ctx->set_error(ctx, 400, "received wms request with no srs");
      return;
   }

   str = apr_table_get(params,"LAYERS");
   if(!str) {
      ctx->set_error(ctx, 400, "received wms request with no layers");
      return;
   } else {
      char *last, *key, *layers;
      int count=1;
      char *sep=",";
      geocache_request_get_tile *req = (geocache_request_get_tile*)apr_pcalloc(
            ctx->pool,sizeof(geocache_request_get_tile));
      layers = apr_pstrdup(ctx->pool,str);
      req->request.type = GEOCACHE_REQUEST_GET_TILE;
      for(key=layers;*key;key++) if(*key == ',') count++;
      req->ntiles = 0;
      req->tiles = (geocache_tile**)apr_pcalloc(ctx->pool,count * sizeof(geocache_tile*));
      for (key = apr_strtok(layers, sep, &last); key != NULL;
            key = apr_strtok(NULL, sep, &last)) {
         geocache_tile *tile;
         geocache_tileset *tileset = geocache_configuration_get_tileset(config,key);
         if(!tileset) {
            ctx->set_error(ctx, 404, "received wms request with invalid layer %s", key);
            return;
         }
         int i;
         geocache_grid_link *grid_link = NULL;
         for(i=0;i<tileset->grid_links->nelts;i++){
            geocache_grid_link *sgrid = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
            if(strcasecmp(sgrid->grid->srs,srs)) continue;
            if(sgrid->grid->tile_sx != width) continue;
            if(sgrid->grid->tile_sy != height) continue;
            grid_link = sgrid;
            break;
         }
         if(!grid_link) {
               ctx->set_error(ctx, 400,
                     "received unsuitable wms request: no suitable <grid> found");
               return;
         }

         tile = geocache_tileset_tile_create(ctx->pool, tileset, grid_link);
         if(!tile) {
            ctx->set_error(ctx, 500, "failed to allocate tile");
            return;
         }
         geocache_tileset_tile_lookup(ctx, tile, bbox);
         GC_CHECK_ERROR(ctx);
         geocache_tileset_tile_validate(ctx,tile);
         GC_CHECK_ERROR(ctx);

         /*look for dimensions*/
         if(tileset->dimensions) {
            int i;
            tile->dimensions = apr_table_make(ctx->pool,tileset->dimensions->nelts);
            for(i=0;i<tileset->dimensions->nelts;i++) {
               geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
               const char *value;
               if((value = (char*)apr_table_get(params,dimension->name)) != NULL) {
                  char *tmpval = apr_pstrdup(ctx->pool,value);
                  int ok = dimension->validate(ctx,dimension,&tmpval);
                  GC_CHECK_ERROR(ctx);
                  if(ok == GEOCACHE_SUCCESS)
                     apr_table_setn(tile->dimensions,dimension->name,tmpval);
                  else {
                     ctx->set_error(ctx,400,"dimension \"%s\" value \"%s\" fails to validate",
                           dimension->name, value);
                     return;
                  }
               } else {
                  apr_table_setn(tile->dimensions,dimension->name,dimension->default_value);
               }
            }
         }
         req->tiles[req->ntiles++] = tile;
      }
      *request = (geocache_request*)req;
   }
}

geocache_service* geocache_service_wms_create(geocache_context *ctx) {
   geocache_service_wms* service = (geocache_service_wms*)apr_pcalloc(ctx->pool, sizeof(geocache_service_wms));
   if(!service) {
      ctx->set_error(ctx, 500, "failed to allocate wms service");
      return NULL;
   }
   service->service.url_prefix = apr_pstrdup(ctx->pool,"wms");
   service->service.type = GEOCACHE_SERVICE_WMS;
   service->service.parse_request = _geocache_service_wms_parse_request;
   service->service.create_capabilities_response = _create_capabilities_wms;
   return (geocache_service*)service;
}

/** @} */
/* vim: ai ts=3 sts=3 et sw=3
*/

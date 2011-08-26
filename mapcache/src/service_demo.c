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

static char *demo_head = 
      "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
      "  <head>\n"
      "    <style type=\"text/css\">\n"
      "    #map {\n"
      "    width: 100%;\n"
      "    height: 100%;\n"
      "    border: 1px solid black;\n"
      "    }\n"
      "    </style>\n"
      "    <script src=\"http://www.openlayers.org/api/OpenLayers.js\"></script>\n"
      "    <script type=\"text/javascript\">\n"
      "var map;\n"
      "function init(){\n"
      "    map = new OpenLayers.Map( 'map' );\n";

static char *demo_layer =
      "    var %s_%s_layer = new OpenLayers.Layer.WMS( \"%s-%s\",\n"
      "        \"%s\",{layers: '%s'},\n"
      "        { gutter:0,ratio:1,isBaseLayer:true,transitionEffect:'resize',\n"
      "          resolutions:[%s],\n"
      "          maxExtent: new OpenLayers.Bounds(%f,%f,%f,%f),\n"
      "          projection: new OpenLayers.Projection(\"%s\")\n"
      "        }\n"
      "    );\n";  

/**
 * \brief parse a demo request
 * \private \memberof geocache_service_demo
 * \sa geocache_service::parse_request()
 */
void _geocache_service_demo_parse_request(geocache_context *ctx, geocache_request **request,
      const char *cpathinfo, apr_table_t *params, geocache_cfg *config) {
   *request = (geocache_request*)apr_pcalloc(
               ctx->pool,sizeof(geocache_request_get_capabilities));
   (*request)->type = GEOCACHE_REQUEST_GET_CAPABILITIES;
}

void _create_capabilities_demo(geocache_context *ctx, geocache_request_get_capabilities *req,
      char *url, char *path_info, geocache_cfg *cfg) {
   geocache_request_get_capabilities *request = (geocache_request_get_capabilities*)req;
   request->mime_type = apr_pstrdup(ctx->pool,"text/html");
   const char *onlineresource = apr_table_get(cfg->metadata,"url");
   if(!onlineresource) {
      onlineresource = url;
   }
   char *caps = apr_pstrdup(ctx->pool,demo_head);
   apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,cfg->tilesets);
   char *layers="";
   while(tileindex_index) {
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
      int i,j;
      for(j=0;j<tileset->grid_links->nelts;j++) {
         char *resolutions="";
         geocache_grid *grid = APR_ARRAY_IDX(tileset->grid_links,j,geocache_grid_link*)->grid;
         layers = apr_psprintf(ctx->pool,"%s,%s_%s_layer",layers,tileset->name,grid->name);
         resolutions = apr_psprintf(ctx->pool,"%s%.20f",resolutions,grid->levels[0]->resolution);
         for(i=1;i<grid->nlevels;i++) {
            resolutions = apr_psprintf(ctx->pool,"%s,%.20f",resolutions,grid->levels[i]->resolution);
         }
         char *ol_layer = apr_psprintf(ctx->pool,demo_layer,
               tileset->name,
               grid->name,
               tileset->name,
               grid->name,
               apr_pstrcat(ctx->pool,onlineresource,"/wms?",NULL),
               tileset->name,resolutions,
               grid->extent[0],
               grid->extent[1],
               grid->extent[2],
               grid->extent[3],
               grid->srs);
         caps = apr_psprintf(ctx->pool,"%s%s",caps,ol_layer);
      }
      tileindex_index = apr_hash_next(tileindex_index);
   }
   /*skip leading comma */
   layers++;
   caps = apr_psprintf(ctx->pool,"%s"
         "    map.addLayers([%s]);\n"
               "    if(!map.getCenter())\n"
               "     map.zoomToMaxExtent();\n"
               "    map.addControl(new OpenLayers.Control.LayerSwitcher());\n"
               "    map.addControl(new OpenLayers.Control.MousePosition());\n"
               "}\n"
               "    </script>\n"
               "  </head>\n"
               "\n"
               "<body onload=\"init()\">\n"
               "    <div id=\"map\">\n"
               "    </div>\n"
               "</body>\n"
               "</html>\n",caps,layers);
   
   request->capabilities = caps;
   
}

geocache_service* geocache_service_demo_create(geocache_context *ctx) {
   geocache_service_demo* service = (geocache_service_demo*)apr_pcalloc(ctx->pool, sizeof(geocache_service_demo));
   if(!service) {
      ctx->set_error(ctx, 500, "failed to allocate demo service");
      return NULL;
   }
   service->service.url_prefix = apr_pstrdup(ctx->pool,"demo");
   service->service.type = GEOCACHE_SERVICE_DEMO;
   service->service.parse_request = _geocache_service_demo_parse_request;
   service->service.create_capabilities_response = _create_capabilities_demo;
   return (geocache_service*)service;
}

/** @} *//* vim: ai ts=3 sts=3 et sw=3
*/

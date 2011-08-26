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
      "    <title>mod-geocache demo service</title>\n"
      "    <style type=\"text/css\">\n"
      "    #map {\n"
      "    width: 100%;\n"
      "    height: 100%;\n"
      "    border: 1px solid black;\n"
      "    }\n"
      "    </style>\n"
      "    <script src=\"http://www.openlayers.org/api/OpenLayers.js\"></script>\n"
      "    <script type=\"text/javascript\">\n"
      "%s\n"
      "var map;\n"
      "function init(){\n"
      "    map = new OpenLayers.Map( 'map', {\n"
      "        displayProjection: new OpenLayers.Projection(\"EPSG:4326\")\n"
      "    } );\n";

static char *demo_ve_extra =
      "function QuadTree(tx, ty, zoom) {\n"
      "    var tname = '';\n"
      "    var i, j, mask, digit;\n"
      "    var zero = '0'.charCodeAt(0);\n"
      "\n"
      "    if (ty < 0) ty = 2 - ty;\n"
      "    ty = (Math.pow(2,zoom) - 1) - ty;\n"
      "\n"
      "    for (i=zoom, j=0; i>0; i--, j++) {\n"
      "        digit = 0;\n"
      "        mask = 1 << (i-1);\n"
      "        if (tx & mask) digit += 1;\n"
      "        if (ty & mask) digit += 2;\n"
      "        tname += String.fromCharCode(zero + digit);\n"
      "    }\n"
      "    return tname;\n"
      "}\n"
      "    function WGSQuadTree(tx, ty, zoom) {\n"
      "        var tname = '';\n"
      "        var i, n;\n"
      "        var zero = '0'.charCodeAt(0);\n"
      "\n"
      "        ty = (Math.pow(2,zoom-1) - 1) - ty;\n"
      "\n"
      "        for (i=zoom; i>0; i--) {\n"
      "            if (i == 1)\n"
      "                n = Math.floor(tx/2)*4 + tx%2 + (ty%2)*2;\n"
      "            else\n"
      "                n = (ty%2)*2 + tx%2;\n"
      "            if (n<0 || n>9)\n"
      "                return '';\n"
      "            tname = String.fromCharCode(zero + n) + tname;\n"
      "            tx = Math.floor(tx/2);\n"
      "            ty = Math.floor(ty/2);\n"
      "        }\n"
      "        return tname;\n"
      "    }\n"
      "function get_ve_url (bounds) {\n"
      "    var xoriginShift, yoriginShift, id;\n"
      "    if (this.sphericalMercator) {\n"
      "        xoriginShift = 2 * Math.PI * 6378137.0 / 2.0; // meters\n"
      "        yoriginShift = xoriginShift;\n"
      "    }\n"
      "    else {\n"
      "        xoriginShift = 180.0;\n"
      "        yoriginShift = 90;\n"
      "    }\n"
      "\n"
      "    var res = this.map.getResolution();\n"
      "    var x = Math.floor(Math.ceil(((bounds.left + bounds.right)/2.0 + xoriginShift) / res / this.tileSize.w) - 1);\n"
      "    var y = Math.floor(Math.ceil(((bounds.top + bounds.bottom)/2.0 + yoriginShift) / res / this.tileSize.h) - 1);\n"
      "    var z = this.map.getZoom();\n"
      "    if (this.sphericalMercator) {\n"
      "        id = QuadTree(x, y, z);\n"
      "    }\n"
      "    else {\n"
      "        id = WGSQuadTree(x, y, z);\n"
      "    }\n"
      "    var path = '?LAYER=' + this.options.layername + '&tile=' + id;\n"
      "    var url = this.url;\n"
      "    if (url instanceof Array) {\n"
      "        url = this.selectUrl(path, url);\n"
      "    }\n"
      "    return url + path;\n"
      "}\n";

static char *demo_layer_wms =
      "    var %s_%s_wms_layer = new OpenLayers.Layer.WMS( \"%s-%s-WMS\",\n"
      "        \"%s\",{layers: '%s'},\n"
      "        { gutter:0,buffer:0,isBaseLayer:true,transitionEffect:'resize',\n"
      "          resolutions:[%s],\n"
      "          units:\"%s\",\n"
      "          maxExtent: new OpenLayers.Bounds(%f,%f,%f,%f),\n"
      "          projection: new OpenLayers.Projection(\"%s\".toUpperCase()),\n"
      "          sphericalMercator: %s\n"
      "        }\n"
      "    );\n"
      "    map.addLayer(%s_%s_wms_layer)\n\n";

static char *demo_layer_tms =
      "    var %s_%s_tms_layer = new OpenLayers.Layer.TMS( \"%s-%s-TMS\",\n"
      "        \"%s\",\n"
      "        { layername: '%s@%s', type: \"%s\", serviceVersion:\"1.0.0\",\n"
      "          gutter:0,buffer:0,isBaseLayer:true,transitionEffect:'resize',\n"
      "          tileOrigin: new OpenLayers.LonLat(%f,%f),\n"
      "          resolutions:[%s],\n"
      "          units:\"%s\",\n"
      "          maxExtent: new OpenLayers.Bounds(%f,%f,%f,%f),\n"
      "          projection: new OpenLayers.Projection(\"%s\".toUpperCase()),\n"
      "          sphericalMercator: %s\n"
      "        }\n"
      "    );\n"
      "    map.addLayer(%s_%s_tms_layer)\n\n";

static char *demo_layer_wmts =
      "    var %s_%s_wmts_layer = new OpenLayers.Layer.WMTS({\n"
      "        name: \"%s-%s-WMTS\",\n"
      "        url: \"%s\",\n"
      "        layer: '%s',\n"
      "        matrixSet: '%s',\n"
      "        format: '%s',\n"
      "        style: 'default',\n"
      "        gutter:0,buffer:0,isBaseLayer:true,transitionEffect:'resize',\n"
      "        resolutions:[%s],\n"
      "        units:\"%s\",\n"
      "        maxExtent: new OpenLayers.Bounds(%f,%f,%f,%f),\n"
      "        projection: new OpenLayers.Projection(\"%s\".toUpperCase()),\n"
      "        sphericalMercator: %s\n"
      "      }\n"
      "    );\n"
      "    map.addLayer(%s_%s_wmts_layer)\n\n";

static char *demo_layer_ve =
      "    var %s_%s_ve_layer = new OpenLayers.Layer.TMS( \"%s-%s-VE\",\n"
      "        \"%s\",\n"
      "        { layername: '%s@%s',\n"
      "          getURL: get_ve_url,\n"
      "          gutter:0,buffer:0,isBaseLayer:true,transitionEffect:'resize',\n"
      "          resolutions:[%s],\n"
      "          units:\"%s\",\n"
      "          maxExtent: new OpenLayers.Bounds(%f,%f,%f,%f),\n"
      "          projection: new OpenLayers.Projection(\"%s\".toUpperCase()),\n"
      "          sphericalMercator: %s\n"
      "        }\n"
      "    );\n"
      "    map.addLayer(%s_%s_ve_layer)\n\n";

#ifdef USE_CAIRO
static char *demo_layer_singletile =
      "    var %s_%s_slayer = new OpenLayers.Layer.WMS( \"%s-%s (singleTile)\",\n"
      "        \"%s\",{layers: '%s'},\n"
      "        { gutter:0,ratio:1,isBaseLayer:true,transitionEffect:'resize',\n"
      "          resolutions:[%s],\n"
      "          units:\"%s\",\n"
      "          singleTile:true,\n"
      "          maxExtent: new OpenLayers.Bounds(%f,%f,%f,%f),\n"
      "          projection: new OpenLayers.Projection(\"%s\".toUpperCase()),\n"
      "          sphericalMercator: %s\n"
      "        }\n"
      "    );\n"
      "    map.addLayer(%s_%s_slayer)\n\n";
#endif

static char *demo_footer =
      "%s"
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
      "</html>\n";

/**
 * \brief parse a demo request
 * \private \memberof geocache_service_demo
 * \sa geocache_service::parse_request()
 */
void _geocache_service_demo_parse_request(geocache_context *ctx, geocache_service *this, geocache_request **request,
      const char *cpathinfo, apr_table_t *params, geocache_cfg *config) {
   geocache_request_get_capabilities_demo *drequest =
      (geocache_request_get_capabilities_demo*)apr_pcalloc(
            ctx->pool,sizeof(geocache_request_get_capabilities_demo));
   *request = (geocache_request*)drequest;
   (*request)->type = GEOCACHE_REQUEST_GET_CAPABILITIES;
   if(!cpathinfo || *cpathinfo=='\0' || !strcmp(cpathinfo,"/")) {
      /*we have no specified service, create the link page*/
      drequest->service = NULL;
      return;
   } else {
      cpathinfo++; /* skip the leading / */
      int i;
      for(i=0;i<GEOCACHE_SERVICES_COUNT;i++) {
         /* loop through the services that have been configured */
         int prefixlen;
         geocache_service *service = NULL;
         service = config->services[i];
         if(!service) continue; /* skip an unconfigured service */
         prefixlen = strlen(service->url_prefix);
         if(strncmp(service->url_prefix,cpathinfo, prefixlen)) continue; /*skip a service who's prefix does not correspond */
         if(*(cpathinfo+prefixlen)!='/' && *(cpathinfo+prefixlen)!='\0') continue; /*we matched the prefix but there are trailing characters*/
         drequest->service = service;
         return;
      }
      ctx->set_error(ctx,404,"demo service \"%s\" not recognised or not enabled",cpathinfo);
   }
}

void _create_demo_front(geocache_context *ctx, geocache_request_get_capabilities *req,
      const char *urlprefix) {
   req->mime_type = apr_pstrdup(ctx->pool,"text/html");
   char *caps = apr_pstrdup(ctx->pool,
         "<html><head><title>geocache demo landing page</title></head><body>");
   int i;
   for(i=0;i<GEOCACHE_SERVICES_COUNT;i++) {
      geocache_service *service = ctx->config->services[i];
      if(!service || service->type == GEOCACHE_SERVICE_DEMO) continue; /* skip an unconfigured service, and the demo one */
      caps = apr_pstrcat(ctx->pool,caps,"<a href=\"",urlprefix,"/demo/",service->url_prefix,"\">",
            service->url_prefix,"</a><br/>",NULL);
   }

   req->capabilities = caps;
}

void _create_demo_wms(geocache_context *ctx, geocache_request_get_capabilities *req,
         const char *url_prefix) {
   req->mime_type = apr_pstrdup(ctx->pool,"text/html");
   char *caps = apr_psprintf(ctx->pool,demo_head, "");
   char *ol_layer;
   apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,ctx->config->tilesets);
   while(tileindex_index) {
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
      int i,j;
      for(j=0;j<tileset->grid_links->nelts;j++) {
         char *resolutions="";
         char *unit="dd";
         char *smerc = "false";
         geocache_grid *grid = APR_ARRAY_IDX(tileset->grid_links,j,geocache_grid_link*)->grid;
         if(grid->unit == GEOCACHE_UNIT_METERS) {
            unit="m";
         } else if(grid->unit == GEOCACHE_UNIT_FEET) {
            unit="ft";
         }
         if(strstr(grid->srs, ":900913") || strstr(grid->srs, ":3857")) {
            smerc = "true";
         }

         resolutions = apr_psprintf(ctx->pool,"%s%.20f",resolutions,grid->levels[0]->resolution);         
         for(i=1;i<grid->nlevels;i++) {
            resolutions = apr_psprintf(ctx->pool,"%s,%.20f",resolutions,grid->levels[i]->resolution);
         }

         ol_layer = apr_psprintf(ctx->pool,demo_layer_wms,
               tileset->name,
               grid->name,
               tileset->name,
               grid->name,
               apr_pstrcat(ctx->pool,url_prefix,"/wms?",NULL),
               tileset->name,
               resolutions,
               unit,
               grid->extent[0],
               grid->extent[1],
               grid->extent[2],
               grid->extent[3],
               grid->srs,
               smerc,
               tileset->name,
               grid->name);
         caps = apr_psprintf(ctx->pool,"%s%s",caps,ol_layer);

#ifdef USE_CAIRO
         ol_layer = apr_psprintf(ctx->pool,demo_layer_singletile,
               tileset->name,
               grid->name,
               tileset->name,
               grid->name,
               apr_pstrcat(ctx->pool,url_prefix,"/wms?",NULL),
               tileset->name,resolutions,unit,
               grid->extent[0],
               grid->extent[1],
               grid->extent[2],
               grid->extent[3],
               grid->srs,
               smerc,
               tileset->name,
               grid->name);
         caps = apr_psprintf(ctx->pool,"%s%s",caps,ol_layer);
#endif
      }
      tileindex_index = apr_hash_next(tileindex_index);
   }
   caps = apr_psprintf(ctx->pool,demo_footer,caps);
   
   req->capabilities = caps;
}

void _create_demo_tms(geocache_context *ctx, geocache_request_get_capabilities *req,
         const char *url_prefix) {
   req->mime_type = apr_pstrdup(ctx->pool,"text/html");
   char *caps = apr_psprintf(ctx->pool,demo_head, "");
   char *ol_layer;
   apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,ctx->config->tilesets);
   while(tileindex_index) {
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
      int i,j;
      char *extension = "png";
      if (tileset->format && tileset->format->extension)
         extension = tileset->format->extension;
      for(j=0;j<tileset->grid_links->nelts;j++) {
         char *resolutions="";
         char *unit="dd";
         char *smerc = "false";
         geocache_grid *grid = APR_ARRAY_IDX(tileset->grid_links,j,geocache_grid_link*)->grid;
         if(grid->unit == GEOCACHE_UNIT_METERS) {
            unit="m";
         } else if(grid->unit == GEOCACHE_UNIT_FEET) {
            unit="ft";
         }
         if(strstr(grid->srs, ":900913") || strstr(grid->srs, ":3857")) {
            smerc = "true";
         }

         resolutions = apr_psprintf(ctx->pool,"%s%.20f",resolutions,grid->levels[0]->resolution);         
         for(i=1;i<grid->nlevels;i++) {
            resolutions = apr_psprintf(ctx->pool,"%s,%.20f",resolutions,grid->levels[i]->resolution);
         }

         ol_layer = apr_psprintf(ctx->pool, demo_layer_tms,
            tileset->name,
            grid->name,
            tileset->name,
            grid->name,
            apr_pstrcat(ctx->pool,url_prefix,"/tms/",NULL),
            tileset->name,
            grid->name,
            extension,
            grid->extent[0],
            grid->extent[1],
            resolutions,
            unit,
            grid->extent[0],
            grid->extent[1],
            grid->extent[2],
            grid->extent[3],
            grid->srs,
            smerc,
            tileset->name,
            grid->name);
         caps = apr_psprintf(ctx->pool,"%s%s",caps,ol_layer);
      }
      tileindex_index = apr_hash_next(tileindex_index);
   }
   caps = apr_psprintf(ctx->pool,demo_footer,caps);
   
   req->capabilities = caps;
}

void _create_demo_wmts(geocache_context *ctx, geocache_request_get_capabilities *req,
         const char *url_prefix) {
   req->mime_type = apr_pstrdup(ctx->pool,"text/html");
   char *caps = apr_psprintf(ctx->pool,demo_head, "");
   char *ol_layer;
   apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,ctx->config->tilesets);
   while(tileindex_index) {
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
      int i,j;
      char *mime_type = "image/png";
      if (tileset->format && tileset->format->mime_type)
         mime_type = tileset->format->mime_type;
      for(j=0;j<tileset->grid_links->nelts;j++) {
         char *resolutions="";
         char *unit="dd";
         char *smerc = "false";
         geocache_grid *grid = APR_ARRAY_IDX(tileset->grid_links,j,geocache_grid_link*)->grid;
         if(grid->unit == GEOCACHE_UNIT_METERS) {
            unit="m";
         } else if(grid->unit == GEOCACHE_UNIT_FEET) {
            unit="ft";
         }
         if(strstr(grid->srs, ":900913") || strstr(grid->srs, ":3857")) {
            smerc = "true";
         }

         resolutions = apr_psprintf(ctx->pool,"%s%.20f",resolutions,grid->levels[0]->resolution);         
         for(i=1;i<grid->nlevels;i++) {
            resolutions = apr_psprintf(ctx->pool,"%s,%.20f",resolutions,grid->levels[i]->resolution);
         }

         ol_layer = apr_psprintf(ctx->pool, demo_layer_wmts,
            tileset->name,
            grid->name,
            tileset->name,
            grid->name,
            apr_pstrcat(ctx->pool,url_prefix,"/wmts/",NULL),
            tileset->name,
            grid->name,
            mime_type,
            resolutions,
            unit,
            grid->extent[0],
            grid->extent[1],
            grid->extent[2],
            grid->extent[3],
            grid->srs,
            smerc,
            tileset->name,
            grid->name);
         caps = apr_psprintf(ctx->pool,"%s%s",caps,ol_layer);
      }
      tileindex_index = apr_hash_next(tileindex_index);
   }
   caps = apr_psprintf(ctx->pool,demo_footer,caps);
   
   req->capabilities = caps;
}

void _create_demo_ve(geocache_context *ctx, geocache_request_get_capabilities *req,
         const char *url_prefix) {
   req->mime_type = apr_pstrdup(ctx->pool,"text/html");
   char *caps = apr_psprintf(ctx->pool,demo_head, demo_ve_extra);
   char *ol_layer;
   apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,ctx->config->tilesets);
   while(tileindex_index) {
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
      int i,j;
      for(j=0;j<tileset->grid_links->nelts;j++) {
         char *resolutions="";
         char *unit="dd";
         char *smerc = "false";
         geocache_grid *grid = APR_ARRAY_IDX(tileset->grid_links,j,geocache_grid_link*)->grid;
         if(grid->unit == GEOCACHE_UNIT_METERS) {
            unit="m";
         } else if(grid->unit == GEOCACHE_UNIT_FEET) {
            unit="ft";
         }
         if(strstr(grid->srs, ":900913") || strstr(grid->srs, ":3857")) {
            smerc = "true";
         }

         resolutions = apr_psprintf(ctx->pool,"%s%.20f",resolutions,grid->levels[0]->resolution);         
         for(i=1;i<grid->nlevels;i++) {
            resolutions = apr_psprintf(ctx->pool,"%s,%.20f",resolutions,grid->levels[i]->resolution);
         }

         ol_layer = apr_psprintf(ctx->pool, demo_layer_ve,
            tileset->name,
            grid->name,
            tileset->name,
            grid->name,
            apr_pstrcat(ctx->pool,url_prefix,"/ve",NULL),
            tileset->name,
            grid->name,
            resolutions,
            unit,
            grid->extent[0],
            grid->extent[1],
            grid->extent[2],
            grid->extent[3],
            grid->srs,
            smerc,
            tileset->name,
            grid->name);
         caps = apr_psprintf(ctx->pool,"%s%s",caps,ol_layer);
      }
      tileindex_index = apr_hash_next(tileindex_index);
   }
   caps = apr_psprintf(ctx->pool,demo_footer,caps);
   
   req->capabilities = caps;
}

void _create_capabilities_demo(geocache_context *ctx, geocache_request_get_capabilities *req,
      char *url, char *path_info, geocache_cfg *cfg) {
   geocache_request_get_capabilities_demo *request = (geocache_request_get_capabilities_demo*)req;
   const char *onlineresource = apr_table_get(cfg->metadata,"url");
   if(!onlineresource) {
      onlineresource = url;
   }

   if(!request->service) {
      return _create_demo_front(ctx,req,onlineresource);
   } else {
      switch(request->service->type) {
         case GEOCACHE_SERVICE_WMS:
            return _create_demo_wms(ctx,req,onlineresource);
         case GEOCACHE_SERVICE_TMS:
            return _create_demo_tms(ctx,req,onlineresource);
         case GEOCACHE_SERVICE_WMTS:
            return _create_demo_wmts(ctx,req,onlineresource);
         case GEOCACHE_SERVICE_VE:
            return _create_demo_ve(ctx,req,onlineresource);
         case GEOCACHE_SERVICE_KML:
         case GEOCACHE_SERVICE_GMAPS:
            req->mime_type = apr_pstrdup(ctx->pool,"text/plain");
            req->capabilities = apr_pstrdup(ctx->pool,"not implemented");
            return;
         case GEOCACHE_SERVICE_DEMO:
            ctx->set_error(ctx,400,"selected service does not provide a demo page");
            return;
      }
   }


   
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

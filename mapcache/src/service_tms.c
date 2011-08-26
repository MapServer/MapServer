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


static const char *tms_0 = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
      "<Services>\n"
      "<TileMapService version=\"1.0.0\" href=\"%s/tms/1.0.0/\" />\n"
      "</Services>\n";

static const char *tms_1 = "<TileMap \n"
      "href=\"%s/tms/%s/%s/\"\n"
      "srs=\"%s\"\n"
      "title=\"%s\"\n"
      "profile=\"global-geodetic\" />";

static const char *tms_2="<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
      "<TileMap version=\"%s\" tilemapservice=\"%s/tms/%s/\">\n"
      "<Title>%s</Title>\n"
      "<Abstract>%s</Abstract>\n"
      "<SRS>%s</SRS>\n"
      "<BoundingBox minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\"/>\n"
      "<Origin x=\"%f\" y=\"%f\"/>\n"
      "<TileFormat width=\"%d\" height=\"%d\" mime-type=\"%s\" extension=\"%s\"/>\n"
      "<TileSets>\n";


void _create_capabilities_tms(geocache_context *ctx, geocache_request_get_capabilities *req, char *url, char *path_info, geocache_cfg *cfg) {
   geocache_request_get_capabilities_tms *request = (geocache_request_get_capabilities_tms*)req;
#ifdef DEBUG
   if(request->request.request.type != GEOCACHE_REQUEST_GET_CAPABILITIES) {
      ctx->set_error(ctx,GEOCACHE_ERROR,"wrong tms capabilities request");
      return;
   }
#endif
   char *caps;
   const char *onlineresource = apr_table_get(cfg->metadata,"url");
   if(!onlineresource) {
      onlineresource = url;
   }
   request->request.mime_type = apr_pstrdup(ctx->pool,"text/xml");
   if(!request->version) {
      caps = apr_psprintf(ctx->pool,tms_0,onlineresource);
   } else {
      if(!request->tileset) {
         caps = apr_psprintf(ctx->pool,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
               "<TileMapService version=\"%s\">\n"
               "<TileMaps>",
               request->version);
         apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,cfg->tilesets);

         while(tileindex_index) {
            geocache_tileset *tileset;
            char *tilesetcaps;
            const void *key; apr_ssize_t keylen;
            apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
            const char *title = apr_table_get(tileset->metadata,"title");
            if(!title) {
               title = "no title set, add some in metadata";
            }
            tilesetcaps = apr_psprintf(ctx->pool,tms_1,onlineresource,
                  request->version,tileset->name,tileset->grid->srs,title);
            caps = apr_psprintf(ctx->pool,"%s%s",caps,tilesetcaps);
            tileindex_index = apr_hash_next(tileindex_index);
         }
         caps = apr_psprintf(ctx->pool,"%s</TileMaps>\n</TileMapService>\n",caps);

      } else {
         geocache_tileset *tileset = request->tileset;
         geocache_grid *grid = tileset->grid;
         int i;
         const char *title = apr_table_get(tileset->metadata,"title");
         if(!title) {
            title = "no title set, add some in metadata";
         }
         const char *abstract = apr_table_get(tileset->metadata,"abstract");
         if(!abstract) {
            abstract = "no abstract set, add some in metadata";
         }
         caps = apr_psprintf(ctx->pool,tms_2,
               request->version, onlineresource, request->version,
               title,abstract, grid->srs,
               grid->extents[0][0], grid->extents[0][1],
               grid->extents[0][2], grid->extents[0][3],
               grid->extents[0][0], grid->extents[0][1],
               grid->tile_sx, grid->tile_sy,
               tileset->format->mime_type,
               tileset->format->extension
         );
         for(i=0;i<grid->levels;i++) {
            caps = apr_psprintf(ctx->pool,"%s\n<TileSet href=\"%s/%s/%s/%d\" units-per-pixel=\"%.20f\" order=\"%d\"/>",
                  caps,onlineresource,request->version,tileset->name,i,
                  grid->resolutions[i],i
            );
         }
         
         request->request.capabilities = apr_psprintf(ctx->pool,"info about layer %s",request->tileset->name);
         caps = apr_psprintf(ctx->pool,"%s</TileSets>\n</TileMap>\n",caps);
      }
   }
   request->request.capabilities = caps;


}

/**
 * \brief parse a TMS request
 * \private \memberof geocache_service_tms
 * \sa geocache_service::parse_request()
 */
void _geocache_service_tms_parse_request(geocache_context *ctx, geocache_request **request,
      const char *cpathinfo, apr_table_t *params, geocache_cfg *config) {
   int index = 0;
   char *last, *key, *endptr;
   geocache_tileset *tileset = NULL;
   char *pathinfo;
   int x,y,z;
   
   if(cpathinfo) {
      pathinfo = apr_pstrdup(ctx->pool,cpathinfo);
      /* parse a path_info like /1.0.0/global_mosaic/0/0/0.jpg */
      for (key = apr_strtok(pathinfo, "/", &last); key != NULL;
            key = apr_strtok(NULL, "/", &last)) {
         if(!*key) continue; /* skip an empty string, could happen if the url contains // */
         switch(++index) {
         case 1: /* version */
            if(strcmp("1.0.0",key)) {
               ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request with invalid version %s", key);
               return;
            }
            break;
         case 2: /* layer name */
            tileset = geocache_configuration_get_tileset(config,key);
            if(!tileset) {
               ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request with invalid layer %s", key);
               return;
            }
            break;
         case 3:
            z = (int)strtol(key,&endptr,10);
            if(*endptr != 0) {
               ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with invalid z %s", pathinfo, key);
               return;
            }
            break;
         case 4:
            x = (int)strtol(key,&endptr,10);
            if(*endptr != 0) {
               ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with invalid x %s", pathinfo, key);
               return;
            }
            break;
         case 5:
            y = (int)strtol(key,&endptr,10);
            if(*endptr != '.') {
               ctx->log(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with invalid y %s", pathinfo, key);
               return;
            }
            break;
         default:
            ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with invalid parameter %s", pathinfo, key);
            return;
         }
      }
   }
   if(index == 5) {
      geocache_request_get_tile *req = (geocache_request_get_tile*)apr_pcalloc(ctx->pool,sizeof(geocache_request_get_tile));
      req->request.type = GEOCACHE_REQUEST_GET_TILE;
      req->ntiles = 1;
      req->tiles = (geocache_tile**)apr_pcalloc(ctx->pool,sizeof(geocache_tile*));
      req->tiles[0] = geocache_tileset_tile_create(ctx->pool, tileset);
      req->tiles[0]->x = x;
      req->tiles[0]->y = y;
      req->tiles[0]->z = z;
      *request = (geocache_request*)req;
      return;
   } else if(index<3) {
      geocache_request_get_capabilities_tms *req = (geocache_request_get_capabilities_tms*)apr_pcalloc(
            ctx->pool,sizeof(geocache_request_get_capabilities_tms));
      req->request.request.type = GEOCACHE_REQUEST_GET_CAPABILITIES;
      if(index >= 2) {
         req->tileset = tileset;
      }
      if(index >= 1) {
         req->version = apr_pstrdup(ctx->pool,"1.0.0");
      }
      *request = (geocache_request*)req;
      return;
   }
   else {
      ctx->set_error(ctx,GEOCACHE_REQUEST_ERROR, "received tms request %s with wrong number of arguments", pathinfo);
      return;
   }
}

geocache_service* geocache_service_tms_create(geocache_context *ctx) {
   geocache_service_tms* service = (geocache_service_tms*)apr_pcalloc(ctx->pool, sizeof(geocache_service_tms));
   if(!service) {
      ctx->set_error(ctx, GEOCACHE_ALLOC_ERROR, "failed to allocate tms service");
      return NULL;
   }
   service->service.url_prefix = apr_pstrdup(ctx->pool,"tms");
   service->service.type = GEOCACHE_SERVICE_TMS;
   service->service.parse_request = _geocache_service_tms_parse_request;
   service->service.create_capabilities_response = _create_capabilities_tms;
   return (geocache_service*)service;
}

/** @} */
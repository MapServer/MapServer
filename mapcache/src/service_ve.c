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

#include "mapcache.h"
#include <apr_strings.h>
#include <math.h>

/** \addtogroup services */
/** @{ */



void _create_capabilities_ve(mapcache_context *ctx, mapcache_request_get_capabilities *req, char *url, char *path_info, mapcache_cfg *cfg) {
   ctx->set_error(ctx,501,"ve service does not support capapbilities");
}

/**
 * \brief parse a VE request
 * \private \memberof mapcache_service_ve
 * \sa mapcache_service::parse_request()
 */
void _mapcache_service_ve_parse_request(mapcache_context *ctx, mapcache_service *this, mapcache_request **request,
      const char *cpathinfo, apr_table_t *params, mapcache_cfg *config) {
   const char *layer,*quadkey;
   mapcache_tileset *tileset = NULL;
   mapcache_grid_link *grid_link = NULL;
   layer = apr_table_get(params,"layer");
   if(layer) {
      /*tileset not found directly, test if it was given as "name@grid" notation*/
      char *tname = apr_pstrdup(ctx->pool,layer);
      char *gname = tname;
      int i;
      while(*gname) {
         if(*gname == '@') {
            *gname = '\0';
            gname++;
            break;
         }
         gname++;
      }
      if(!gname) {
         ctx->set_error(ctx,404, "received ve request with invalid layer %s", layer);
         return;
      }
      tileset = mapcache_configuration_get_tileset(config,tname);
      if(!tileset) {
         ctx->set_error(ctx,404, "received ve request with invalid layer %s", tname);
         return;
      }
      for(i=0;i<tileset->grid_links->nelts;i++) {
         mapcache_grid_link *sgrid = APR_ARRAY_IDX(tileset->grid_links,i,mapcache_grid_link*);
         if(!strcmp(sgrid->grid->name,gname)) {
            grid_link = sgrid;
            break;
         }
      }
      if(!grid_link) {
         ctx->set_error(ctx,404, "received ve request with invalid grid %s", gname);
         return;
      }
   } else {
      ctx->set_error(ctx,400,"received ve request with no layer");
      return;
   }

   quadkey = apr_table_get(params,"tile");
   mapcache_tile *tile = mapcache_tileset_tile_create(ctx->pool,tileset,grid_link);
   if(quadkey) {
      int i;
      tile->z = strlen(quadkey);
      if(tile->z<1 || tile->z>=grid_link->grid->nlevels) {
         ctx->set_error(ctx,404,"received ve request with invalid z level %d\n",tile->z);
         return;
      }
      tile->x = tile->y = 0;
      for(i=tile->z; i; i--) {
         int mask = 1 << (i-1);
         switch(quadkey[tile->z-i]) {
            case '0':
               break;
            case '1':
               tile->x |= mask;
               break;
            case '2':
               tile->y |= mask;
               break;
            case '3':
               tile->x |= mask;
               tile->y |= mask;
               break;
            default:
               ctx->set_error(ctx,404, "Invalid Quadkey sequence");
               return;
         }
      }
   } else {
      ctx->set_error(ctx,400,"received ve request with no tile quadkey");
      return;
   }


   mapcache_request_get_tile *req = (mapcache_request_get_tile*)apr_pcalloc(ctx->pool,sizeof(mapcache_request_get_tile));
   req->request.type = MAPCACHE_REQUEST_GET_TILE;
   req->ntiles = 1;
   req->tiles = (mapcache_tile**)apr_pcalloc(ctx->pool,sizeof(mapcache_tile*));
   req->tiles[0] = tile;
   req->tiles[0]->y = grid_link->grid->levels[tile->z]->maxy - tile->y - 1;
   mapcache_tileset_tile_validate(ctx,req->tiles[0]);
   GC_CHECK_ERROR(ctx);
   *request = (mapcache_request*)req;
   return;
}

mapcache_service* mapcache_service_ve_create(mapcache_context *ctx) {
   mapcache_service_ve* service = (mapcache_service_ve*)apr_pcalloc(ctx->pool, sizeof(mapcache_service_ve));
   if(!service) {
      ctx->set_error(ctx, 500, "failed to allocate ve service");
      return NULL;
   }
   service->service.url_prefix = apr_pstrdup(ctx->pool,"ve");
   service->service.name = apr_pstrdup(ctx->pool,"ve");
   service->service.type = MAPCACHE_SERVICE_VE;
   service->service.parse_request = _mapcache_service_ve_parse_request;
   service->service.create_capabilities_response = _create_capabilities_ve;
   return (mapcache_service*)service;
}

/** @} */
/* vim: ai ts=3 sts=3 et sw=3
*/

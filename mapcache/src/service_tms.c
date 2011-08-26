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
#include <ezxml.h>
/** \addtogroup services */
/** @{ */


void _create_capabilities_tms(geocache_context *ctx, geocache_request_get_capabilities *req, char *url, char *path_info, geocache_cfg *cfg) {
   geocache_request_get_capabilities_tms *request = (geocache_request_get_capabilities_tms*)req;
#ifdef DEBUG
   if(request->request.request.type != GEOCACHE_REQUEST_GET_CAPABILITIES) {
      ctx->set_error(ctx,500,"wrong tms capabilities request");
      return;
   }
#endif
   ezxml_t caps;
   const char *onlineresource = apr_table_get(cfg->metadata,"url");
   if(!onlineresource) {
      onlineresource = url;
   }
   request->request.mime_type = apr_pstrdup(ctx->pool,"text/xml");
   if(!request->version) {
      caps = ezxml_new("Services");
      ezxml_t TileMapService = ezxml_add_child(caps,"TileMapService",0);
      ezxml_set_attr(TileMapService,"version","1.0");
      char* serviceurl = apr_pstrcat(ctx->pool,onlineresource,"/tms/1.0.0/",NULL);
      ezxml_set_attr(TileMapService,"href",serviceurl);
   } else {
      if(!request->tileset) {
         caps = ezxml_new("TileMapService");
         ezxml_set_attr(caps,"version",request->version);
         apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,cfg->tilesets);
         ezxml_t tilemaps = ezxml_add_child(caps,"TileMaps",0);
         while(tileindex_index) {
            geocache_tileset *tileset;
            int j;
            const void *key; apr_ssize_t keylen;
            apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
            const char *title = apr_table_get(tileset->metadata,"title");
            if(!title) {
               title = "no title set, add some in metadata";
            }
            for(j=0;j<tileset->grid_links->nelts;j++) {
               geocache_grid *grid = APR_ARRAY_IDX(tileset->grid_links,j,geocache_grid_link*)->grid;
               const char *profile = apr_table_get(grid->metadata,"profile");
               if(!profile) profile = "none";
               ezxml_t tilemap = ezxml_add_child(tilemaps,"TileMap",0);
               ezxml_set_attr(tilemap,"title",title);
               ezxml_set_attr(tilemap,"srs",grid->srs);
               if(profile)
                  ezxml_set_attr(tilemap,"profile",profile);
               char *href = apr_pstrcat(ctx->pool,onlineresource,"/tms/1.0.0/",tileset->name,"@",grid->name,NULL);
               ezxml_set_attr(tilemap,"href",href);
            }
            tileindex_index = apr_hash_next(tileindex_index);
         }
      } else {
         geocache_tileset *tileset = request->tileset;
         geocache_grid *grid = request->grid_link->grid;
         int i;
         double *extent = (request->grid_link->restricted_extent)?request->grid_link->restricted_extent:request->grid_link->grid->extent;
         const char *title = apr_table_get(tileset->metadata,"title");
         if(!title) {
            title = "no title set, add some in metadata";
         }
         const char *abstract = apr_table_get(tileset->metadata,"abstract");
         if(!abstract) {
            abstract = "no abstract set, add some in metadata";
         }
         caps = ezxml_new("TileMap");
         ezxml_set_attr(caps,"version",request->version);
         ezxml_set_attr(caps,"tilemapservice",
               apr_pstrcat(ctx->pool,onlineresource,"/tms/",request->version,"/",NULL));
         
         ezxml_set_txt(ezxml_add_child(caps,"Title",0),title);
         ezxml_set_txt(ezxml_add_child(caps,"Abstract",0),abstract);
         ezxml_set_txt(ezxml_add_child(caps,"SRS",0),grid->srs);
         
         ezxml_t bbox = ezxml_add_child(caps,"BoundingBox",0);
         ezxml_set_attr(bbox,"minx",apr_psprintf(ctx->pool,"%f",extent[0]));
         ezxml_set_attr(bbox,"miny",apr_psprintf(ctx->pool,"%f",extent[1]));
         ezxml_set_attr(bbox,"maxx",apr_psprintf(ctx->pool,"%f",extent[2]));
         ezxml_set_attr(bbox,"maxy",apr_psprintf(ctx->pool,"%f",extent[3]));
         
         ezxml_t origin = ezxml_add_child(caps,"Origin",0);
         ezxml_set_attr(origin,"x",apr_psprintf(ctx->pool,"%f",grid->extent[0]));
         ezxml_set_attr(origin,"y",apr_psprintf(ctx->pool,"%f",grid->extent[1]));
         
         ezxml_t tileformat = ezxml_add_child(caps,"TileFormat",0);
         ezxml_set_attr(tileformat,"width",apr_psprintf(ctx->pool,"%d",grid->tile_sx));
         ezxml_set_attr(tileformat,"height",apr_psprintf(ctx->pool,"%d",grid->tile_sy));
         if(tileset->format->mime_type) {
            ezxml_set_attr(tileformat,"mime-type",tileset->format->mime_type);
         } else {
            ezxml_set_attr(tileformat,"mime-type","image/unknown");
         }
         ezxml_set_attr(tileformat,"extension",tileset->format->extension);
         
         ezxml_t tilesets = ezxml_add_child(caps,"TileSets",0);
         for(i=0;i<grid->nlevels;i++) {
            ezxml_t xmltileset = ezxml_add_child(tilesets,"TileSet",0);
            char *order = apr_psprintf(ctx->pool,"%d",i);
            ezxml_set_attr(xmltileset,"href",
                  apr_pstrcat(ctx->pool,onlineresource,"/tms/",request->version,"/",
                     tileset->name,"@",grid->name,
                     "/",order,NULL));
            ezxml_set_attr(xmltileset,"units-per-pixel",apr_psprintf(ctx->pool,"%.20f",grid->levels[i]->resolution));
            ezxml_set_attr(xmltileset,"order",order);
         }
      }
   }
   char *tmpcaps = ezxml_toxml(caps);
   ezxml_free(caps);
   request->request.capabilities = apr_pstrdup(ctx->pool,tmpcaps);
   free(tmpcaps);


}

/**
 * \brief parse a TMS request
 * \private \memberof geocache_service_tms
 * \sa geocache_service::parse_request()
 */
void _geocache_service_tms_parse_request(geocache_context *ctx, geocache_service *this, geocache_request **request,
      const char *cpathinfo, apr_table_t *params, geocache_cfg *config) {
   int index = 0;
   char *last, *key, *endptr;
   geocache_tileset *tileset = NULL;
   geocache_grid_link *grid_link = NULL;
   char *pathinfo = NULL;
   int x=-1,y=-1,z=-1;
  
   if(this->type == GEOCACHE_SERVICE_GMAPS) {
      index++;
      /* skip the version part of the url */
   }
   if(cpathinfo) {
      pathinfo = apr_pstrdup(ctx->pool,cpathinfo);
      /* parse a path_info like /1.0.0/global_mosaic/0/0/0.jpg */
      for (key = apr_strtok(pathinfo, "/", &last); key != NULL;
            key = apr_strtok(NULL, "/", &last)) {
         if(!*key) continue; /* skip an empty string, could happen if the url contains // */
         switch(++index) {
         case 1: /* version */
            if(strcmp("1.0.0",key)) {
               ctx->set_error(ctx,404, "received tms request with invalid version %s", key);
               return;
            }
            break;
         case 2: /* layer name */
            tileset = geocache_configuration_get_tileset(config,key);
            if(!tileset) {
               /*tileset not found directly, test if it was given as "name@grid" notation*/
               char *tname = apr_pstrdup(ctx->pool,key);
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
                  ctx->set_error(ctx,404, "received tms request with invalid layer %s", key);
                  return;
               }
               tileset = geocache_configuration_get_tileset(config,tname);
               if(!tileset) {
                  ctx->set_error(ctx,404, "received tms request with invalid layer %s", tname);
                  return;
               }
               for(i=0;i<tileset->grid_links->nelts;i++) {
                  geocache_grid_link *sgrid = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
                  if(!strcmp(sgrid->grid->name,gname)) {
                     grid_link = sgrid;
                     break;
                  }
               }
               if(!grid_link) {
                  ctx->set_error(ctx,404, "received tms request with invalid grid %s", gname);
                  return;
               }

            } else {
               grid_link = APR_ARRAY_IDX(tileset->grid_links,0,geocache_grid_link*);
            }
            break;
         case 3:
            z = (int)strtol(key,&endptr,10);
            if(*endptr != 0) {
               ctx->set_error(ctx,404, "received tms request %s with invalid z %s", pathinfo, key);
               return;
            }
            break;
         case 4:
            x = (int)strtol(key,&endptr,10);
            if(*endptr != 0) {
               ctx->set_error(ctx,404, "received tms request %s with invalid x %s", pathinfo, key);
               return;
            }
            break;
         case 5:
            y = (int)strtol(key,&endptr,10);
            if(*endptr != '.') {
               ctx->set_error(ctx,404, "received tms request %s with invalid y %s", pathinfo, key);
               return;
            }
            break;
         default:
            ctx->set_error(ctx,404, "received tms request %s with invalid parameter %s", pathinfo, key);
            return;
         }
      }
   }
   if(index == 5) {
      geocache_request_get_tile *req = (geocache_request_get_tile*)apr_pcalloc(ctx->pool,sizeof(geocache_request_get_tile));
      req->request.type = GEOCACHE_REQUEST_GET_TILE;
      req->ntiles = 1;
      req->tiles = (geocache_tile**)apr_pcalloc(ctx->pool,sizeof(geocache_tile*));
      req->tiles[0] = geocache_tileset_tile_create(ctx->pool, tileset, grid_link);
      req->tiles[0]->x = x;
      if(((geocache_service_tms*)this)->reverse_y) {
         req->tiles[0]->y = grid_link->grid->levels[z]->maxy - y - 1;
      } else {
         req->tiles[0]->y = y;
      }
      req->tiles[0]->z = z;
      geocache_tileset_tile_validate(ctx,req->tiles[0]);
      GC_CHECK_ERROR(ctx);
      *request = (geocache_request*)req;
      return;
   } else if(index<3) {
      geocache_request_get_capabilities_tms *req = (geocache_request_get_capabilities_tms*)apr_pcalloc(
            ctx->pool,sizeof(geocache_request_get_capabilities_tms));
      req->request.request.type = GEOCACHE_REQUEST_GET_CAPABILITIES;
      if(index >= 2) {
         req->tileset = tileset;
         req->grid_link = grid_link;
      }
      if(index >= 1) {
         req->version = apr_pstrdup(ctx->pool,"1.0.0");
      }
      *request = (geocache_request*)req;
      return;
   }
   else {
      ctx->set_error(ctx,404, "received tms request %s with wrong number of arguments", pathinfo);
      return;
   }
}

geocache_service* geocache_service_tms_create(geocache_context *ctx) {
   geocache_service_tms* service = (geocache_service_tms*)apr_pcalloc(ctx->pool, sizeof(geocache_service_tms));
   if(!service) {
      ctx->set_error(ctx, 500, "failed to allocate tms service");
      return NULL;
   }
   service->service.url_prefix = apr_pstrdup(ctx->pool,"tms");
   service->service.name = apr_pstrdup(ctx->pool,"tms");
   service->service.type = GEOCACHE_SERVICE_TMS;
   service->reverse_y = 0;
   service->service.parse_request = _geocache_service_tms_parse_request;
   service->service.create_capabilities_response = _create_capabilities_tms;
   return (geocache_service*)service;
}

void _create_capabilities_gmaps(geocache_context *ctx, geocache_request_get_capabilities *req, char *url, char *path_info, geocache_cfg *cfg) {
   ctx->set_error(ctx,501,"gmaps service does not support capapbilities");
}

geocache_service* geocache_service_gmaps_create(geocache_context *ctx) {
   geocache_service_tms* service = (geocache_service_tms*)apr_pcalloc(ctx->pool, sizeof(geocache_service_tms));
   if(!service) {
      ctx->set_error(ctx, 500, "failed to allocate gmaps service");
      return NULL;
   }
   service->service.url_prefix = apr_pstrdup(ctx->pool,"gmaps");
   service->service.name = apr_pstrdup(ctx->pool,"gmaps");
   service->reverse_y = 1;
   service->service.type = GEOCACHE_SERVICE_GMAPS;
   service->service.parse_request = _geocache_service_tms_parse_request;
   service->service.create_capabilities_response = _create_capabilities_gmaps;
   return (geocache_service*)service;
}

/** @} */
/* vim: ai ts=3 sts=3 et sw=3
*/

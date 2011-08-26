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
              "<GetFeatureInfo>\n"
                "<Format>text/plain</Format>\n"
                "<Format>application/vnd.ogc.gml</Format>\n"
                "<DCPType>\n"
                  "<HTTP>\n"
                    "<Get>\n"
                      "<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s/wms?\" />\n"
                    "</Get>\n"
                  "</HTTP>\n"
                "</DCPType>\n"
              "</GetFeatureInfo>\n"
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
static char *wms_layer = "<Layer queryable=\"%d\" cascaded=\"1\">\n"
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
   char *caps = apr_psprintf(ctx->pool,wms_capabilities_preamble,title,url,url,url,url);
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
         if(tileset->wgs84bbox[0] != tileset->wgs84bbox[2]) {
            char *wgs84bbox = apr_psprintf(ctx->pool,"<LatLonBoundingBox minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\"/>\n",
                  tileset->wgs84bbox[0], tileset->wgs84bbox[1],
                  tileset->wgs84bbox[2], tileset->wgs84bbox[3]);
            srss = apr_pstrcat(ctx->pool,srss,wgs84bbox,NULL);
         }

         for(i=0;i<tileset->grid_links->nelts;i++) {
            geocache_grid_link *gridlink = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
            double *extent = gridlink->grid->extent;
            if(gridlink->restricted_extent)
               extent = gridlink->restricted_extent;
            char *bbox = apr_psprintf(ctx->pool,wms_bbox,
                  gridlink->grid->srs,
                  extent[0],
                  extent[1],
                  extent[2],
                  extent[3],
                  gridlink->grid->srs);
            int j;
            for(j=0;j<gridlink->grid->srs_aliases->nelts;j++) {
               bbox = apr_pstrcat(ctx->pool, bbox, "<SRS>",APR_ARRAY_IDX(gridlink->grid->srs_aliases,j,char*),"</SRS>\n",NULL);
            }
            srss = apr_pstrcat(ctx->pool,srss,bbox,NULL);
         }
         char *layercaps = apr_psprintf(ctx->pool,wms_layer,
               tileset->source->info_formats?1:0,
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
   int isGetMap=0;
   
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

   if( ! strcasecmp(str,"getmap")) {
      isGetMap = 1;
   } else {
      if( ! strcasecmp(str,"getcapabilities") ) {
         *request = (geocache_request*)
            apr_pcalloc(ctx->pool,sizeof(geocache_request_get_capabilities_wms));
         (*request)->type = GEOCACHE_REQUEST_GET_CAPABILITIES;
         return; /* OK */
      } else if( ! strcasecmp(str,"getfeatureinfo") ) {
         //nothing
      } else {
         ctx->set_error(ctx, 501, "received wms with invalid request %s",str);
         return;
      }
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

   if(isGetMap) {
      str = apr_table_get(params,"LAYERS");
      if(!str) {
         ctx->set_error(ctx, 400, "received wms request with no layers");
         return;
      } else {
         char *last, *key, *layers;
         int count=1;
         char *sep=",";
         geocache_request_get_map *map_req = NULL;
         geocache_request_get_tile *tile_req = NULL;
         layers = apr_pstrdup(ctx->pool,str);
         for(key=layers;*key;key++) if(*key == ',') count++;

         /* we have to loop twice on the requested layers, once to determine the request type,
          * and a second time to actually populate the request structure */

         /*first pass around requested layers to see if we have a getTile or a getMap request*/
         geocache_request_type type = GEOCACHE_REQUEST_GET_TILE;
         for (key = apr_strtok(layers, sep, &last); key != NULL;
               key = apr_strtok(NULL, sep, &last)) {
            geocache_tileset *tileset = geocache_configuration_get_tileset(config,key);
            if(!tileset) {
               ctx->set_error(ctx, 404, "received wms request with invalid layer %s", key);
               return;
            }
            int i;
            geocache_grid_link *grid_link = NULL;
            for(i=0;i<tileset->grid_links->nelts;i++){
               geocache_grid_link *sgrid = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
               /* look for a grid with a matching srs */
               if(strcasecmp(sgrid->grid->srs,srs)) {
                  /* look if the grid has some srs aliases */
                  int s;
                  for(s=0;s<sgrid->grid->srs_aliases->nelts;s++) {
                     char *srsalias = APR_ARRAY_IDX(sgrid->grid->srs_aliases,s,char*);
                     if(!strcasecmp(srsalias,srs)) break;
                  }
                  if(s==sgrid->grid->srs_aliases->nelts)
                     continue; /* no srs alias matches the requested srs */
               }
               grid_link = sgrid;
               break;
            }
            if(!grid_link) {
               ctx->set_error(ctx, 400,
                     "received unsuitable wms request: no <grid> with suitable srs found for layer %s",tileset->name);
               return;
            }

            /* verify we align on the tileset's grid */ 
            if(grid_link->grid->tile_sx != width || grid_link->grid->tile_sy != height ||
                  geocache_grid_is_bbox_aligned(ctx, grid_link->grid, bbox) != GEOCACHE_SUCCESS) {
               type = GEOCACHE_REQUEST_GET_MAP;
            }
         }

         if(type == GEOCACHE_REQUEST_GET_TILE) {
            tile_req = apr_pcalloc(ctx->pool, sizeof(geocache_request_get_tile));
            tile_req->request.type = GEOCACHE_REQUEST_GET_TILE;
            tile_req->tiles = apr_pcalloc(ctx->pool, count*sizeof(geocache_tile*));
            *request = (geocache_request*)tile_req;
         } else {
            map_req = apr_pcalloc(ctx->pool, sizeof(geocache_request_get_map));
            map_req->request.type = GEOCACHE_REQUEST_GET_MAP;
            map_req->maps = apr_pcalloc(ctx->pool, count*sizeof(geocache_map*));
            *request = (geocache_request*)map_req;
         }


         layers = apr_pstrdup(ctx->pool,str);
         for (key = apr_strtok(layers, sep, &last); key != NULL;
               key = apr_strtok(NULL, sep, &last)) {
            geocache_tileset *tileset = geocache_configuration_get_tileset(config,key);
            int i;
            geocache_grid_link *grid_link = NULL;
            for(i=0;i<tileset->grid_links->nelts;i++){
               grid_link = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
               if(strcasecmp(grid_link->grid->srs,srs)) continue;
               break;
            }
            apr_table_t *dimtable = NULL;
            if(type == GEOCACHE_REQUEST_GET_TILE) {
               geocache_tile *tile = geocache_tileset_tile_create(ctx->pool, tileset, grid_link);
#ifndef DEBUG
               geocache_grid_get_cell(ctx, grid_link->grid, bbox, &tile->x, &tile->y, &tile->z);
#else
               int ret = geocache_grid_get_cell(ctx, grid_link->grid, bbox, &tile->x, &tile->y, &tile->z);
               if(ret != GEOCACHE_SUCCESS) {
                  ctx->set_error(ctx,500,"###BUG###: grid_get_cell returned failure");
                  return;
               }
#endif
               geocache_tileset_tile_validate(ctx,tile);
               if(GC_HAS_ERROR(ctx)) {
                  /* don't bail out just yet, in case multiple tiles have been requested */
                  ctx->clear_errors(ctx);
               } else {
                  tile_req->tiles[tile_req->ntiles++] = tile;
               }
               dimtable = tile->dimensions;

            } else {
               geocache_map *map = geocache_tileset_map_create(ctx->pool,tileset,grid_link);
               map->width = width;
               map->height = height;
               map->extent[0] = bbox[0];
               map->extent[1] = bbox[1];
               map->extent[2] = bbox[2];
               map->extent[3] = bbox[3];
               map_req->maps[map_req->nmaps++] = map;
               dimtable = map->dimensions;
            }

            /*look for dimensions*/
            if(dimtable) {
               int i;
               for(i=0;i<tileset->dimensions->nelts;i++) {
                  geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
                  const char *value;
                  if((value = (char*)apr_table_get(params,dimension->name)) != NULL) {
                     char *tmpval = apr_pstrdup(ctx->pool,value);
                     int ok = dimension->validate(ctx,dimension,&tmpval);
                     GC_CHECK_ERROR(ctx);
                     if(ok == GEOCACHE_SUCCESS)
                        apr_table_setn(dimtable,dimension->name,tmpval);
                     else {
                        ctx->set_error(ctx,400,"dimension \"%s\" value \"%s\" fails to validate",
                              dimension->name, value);
                        return;
                     }
                  }
               }
            }
         }
         if(tile_req && tile_req->ntiles == 0) {
            ctx->set_error(ctx,404,"request for tile outside of restricted extent");
            return;
         }
      }
   } else {
      //getfeatureinfo
      str = apr_table_get(params,"QUERY_LAYERS");
      if(!str) {
         ctx->set_error(ctx, 400, "received wms getfeatureinfo request with no query layers");
         return;
      } else if(strstr(str,",")) {
         ctx->set_error(ctx,501, "wms getfeatureinfo not implemented for multiple layers");
         return;
      } else {
         geocache_tileset *tileset = geocache_configuration_get_tileset(config,str);
         if(!tileset) {
            ctx->set_error(ctx, 404, "received wms getfeatureinfo request with invalid layer %s", str);
            return;
         }
         if(!tileset->source->info_formats) {
            ctx->set_error(ctx, 404, "received wms getfeatureinfo request for unqueryable layer %s", str);
            return;
         }
         int i;
         geocache_grid_link *grid_link = NULL;
         for(i=0;i<tileset->grid_links->nelts;i++){
            geocache_grid_link *sgrid = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
            if(strcasecmp(sgrid->grid->srs,srs)) continue;
            grid_link = sgrid;
            break;
         }
         if(!grid_link) {
            ctx->set_error(ctx, 400,
                  "received unsuitable wms request: no <grid> with suitable srs found for layer %s",tileset->name);
            return;
         }
         int x,y;
         
         str = apr_table_get(params,"X");
         if(!str) {
            ctx->set_error(ctx, 400, "received wms getfeatureinfo request with no X");
            return;
         } else {
            char *endptr;
            x = (int)strtol(str,&endptr,10);
            if(*endptr != 0 || x <= 0 || x>=width) {
               ctx->set_error(ctx, 400, "received wms request with invalid X");
               return;
            }
         }

         str = apr_table_get(params,"Y");
         if(!str) {
            ctx->set_error(ctx, 400, "received wms getfeatureinfo request with no Y");
            return;
         } else {
            char *endptr;
            y = (int)strtol(str,&endptr,10);
            if(*endptr != 0 || y <= 0 || y>=height) {
               ctx->set_error(ctx, 400, "received wms request with invalid Y");
               return;
            }
         }
         
         geocache_feature_info *fi = geocache_tileset_feature_info_create(ctx->pool, tileset, grid_link);
         fi->i = x;
         fi->j = y;
         fi->format = apr_pstrdup(ctx->pool,apr_table_get(params,"INFO_FORMAT"));
         if(!fi->format) {
            ctx->set_error(ctx, 400, "received wms getfeatureinfo request with no INFO_FORMAT");
            return;
         }

         if(fi->map.dimensions) {
            int i;
            for(i=0;i<tileset->dimensions->nelts;i++) {
               geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
               const char *value;
               if((value = (char*)apr_table_get(params,dimension->name)) != NULL) {
                  char *tmpval = apr_pstrdup(ctx->pool,value);
                  int ok = dimension->validate(ctx,dimension,&tmpval);
                  GC_CHECK_ERROR(ctx);
                  if(ok == GEOCACHE_SUCCESS)
                     apr_table_setn(fi->map.dimensions,dimension->name,tmpval);
                  else {
                     ctx->set_error(ctx,400,"dimension \"%s\" value \"%s\" fails to validate",
                           dimension->name, value);
                     return;
                  }
               }
            }
         }
         fi->map.width = width;
         fi->map.height = height;
         fi->map.extent[0] = bbox[0];
         fi->map.extent[1] = bbox[1];
         fi->map.extent[2] = bbox[2];
         fi->map.extent[3] = bbox[3];
         geocache_request_get_feature_info *req_fi = apr_pcalloc(ctx->pool, sizeof(geocache_request_get_feature_info));
         req_fi->request.type = GEOCACHE_REQUEST_GET_FEATUREINFO;
         req_fi->fi = fi;
         *request = (geocache_request*)req_fi;

      }
   }
#ifdef DEBUG
   if((*request)->type != GEOCACHE_REQUEST_GET_TILE && (*request)->type != GEOCACHE_REQUEST_GET_MAP && 
         (*request)->type != GEOCACHE_REQUEST_GET_FEATUREINFO  ) {
      ctx->set_error(ctx,500,"BUG: request not gettile or getmap");
      return;
   }
#endif
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

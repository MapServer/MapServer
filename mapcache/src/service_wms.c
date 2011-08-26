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
#include "ezxml.h"

/** \addtogroup services */
/** @{ */

void _create_capabilities_wms(geocache_context *ctx, geocache_request_get_capabilities *req, char *guessed_url, char *path_info, geocache_cfg *cfg) {
   geocache_request_get_capabilities_wms *request = (geocache_request_get_capabilities_wms*)req;
#ifdef DEBUG
   if(request->request.request.type != GEOCACHE_REQUEST_GET_CAPABILITIES) {
      ctx->set_error(ctx,400,"wrong wms capabilities request");
      return;
   }
#endif
   ezxml_t caps, tmpxml;
   const char *title;
   const char *url = apr_table_get(cfg->metadata,"url");
   if(!url) {
      url = guessed_url;
   }
   url = apr_pstrcat(ctx->pool,url,"/wms?",NULL);
   caps = ezxml_new("WMT_MS_Capabilities");
   ezxml_set_attr(caps,"version","1.1.0");
/*
          "<Service>\n"
            "<Name>OGC:WMS</Name>\n"
            "<Title>%s</Title>\n"
            "<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s/wms?\"/>\n"
          "</Service>\n"
*/
   tmpxml = ezxml_add_child(caps,"Service",0);
   ezxml_set_txt(ezxml_add_child(tmpxml,"Name",0),"OGC:WMS");
   title = apr_table_get(cfg->metadata,"title");
   if(!title) {
      title = "no title set, add some in metadata";
   }
   ezxml_set_txt(ezxml_add_child(tmpxml,"Title",0),title);
   tmpxml = ezxml_add_child(tmpxml,"OnlineResource",0);
   ezxml_set_attr(tmpxml,"xmlns:xlink","http://www.w3.org/1999/xlink");
   ezxml_set_attr(tmpxml,"xlink:href",url);
   /*

      "<Capability>\n"
      "<Request>\n"
   */
   ezxml_t capxml = ezxml_add_child(caps,"Capability",0);
   ezxml_t reqxml = ezxml_add_child(capxml,"Request",0);
   /*
      "<GetCapabilities>\n"
      " <Format>application/vnd.ogc.wms_xml</Format>\n"
      " <DCPType>\n"
      "  <HTTP>\n"
      "   <Get><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s/wms?\"/></Get>\n"
      "  </HTTP>\n"
      " </DCPType>\n"
      "</GetCapabilities>\n"
      */
   tmpxml = ezxml_add_child(reqxml,"GetCapabilities",0);
   ezxml_set_txt(ezxml_add_child(tmpxml,"Format",0),"application/vnd.ogc.wms_xml");
   tmpxml = ezxml_add_child(tmpxml,"DCPType",0);
   tmpxml = ezxml_add_child(tmpxml,"HTTP",0);
   tmpxml = ezxml_add_child(tmpxml,"Get",0);
   tmpxml = ezxml_add_child(tmpxml,"OnlineResource",0);
   ezxml_set_attr(tmpxml,"xmlns:xlink","http://www.w3.org/1999/xlink");
   ezxml_set_attr(tmpxml,"xlink:href",url);

/* 
              "<GetMap>\n"
                "<Format>image/png</Format>\n"
                "<Format>image/jpeg</Format>\n"
                "<DCPType>\n"
                  "<HTTP>\n"
                    "<Get><OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:href=\"%s/wms?\"/></Get>\n"
                  "</HTTP>\n"
                "</DCPType>\n"
              "</GetMap>\n"
*/
   tmpxml = ezxml_add_child(reqxml,"GetMap",0);
   ezxml_set_txt(ezxml_add_child(tmpxml,"Format",0),"image/png");
   ezxml_set_txt(ezxml_add_child(tmpxml,"Format",0),"image/jpeg");
   tmpxml = ezxml_add_child(tmpxml,"DCPType",0);
   tmpxml = ezxml_add_child(tmpxml,"HTTP",0);
   tmpxml = ezxml_add_child(tmpxml,"Get",0);
   tmpxml = ezxml_add_child(tmpxml,"OnlineResource",0);
   ezxml_set_attr(tmpxml,"xmlns:xlink","http://www.w3.org/1999/xlink");
   ezxml_set_attr(tmpxml,"xlink:href",url);

   
/*
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
*/
   tmpxml = ezxml_add_child(reqxml,"GetFeatureInfo",0);
   ezxml_set_txt(ezxml_add_child(tmpxml,"Format",0),"text/plain");
   ezxml_set_txt(ezxml_add_child(tmpxml,"Format",0),"application/vnd.ogc.gml");
   tmpxml = ezxml_add_child(tmpxml,"DCPType",0);
   tmpxml = ezxml_add_child(tmpxml,"HTTP",0);
   tmpxml = ezxml_add_child(tmpxml,"Get",0);
   tmpxml = ezxml_add_child(tmpxml,"OnlineResource",0);
   ezxml_set_attr(tmpxml,"xmlns:xlink","http://www.w3.org/1999/xlink");
   ezxml_set_attr(tmpxml,"xlink:href",url);

/*
            "<Exception>\n"
              "<Format>text/plain</Format>\n"
            "</Exception>\n"
*/
      
   tmpxml = ezxml_add_child(capxml,"Exceptions",0);
   ezxml_set_txt(ezxml_add_child(tmpxml,"Format",0),"text/plain");

   ezxml_t vendorxml = ezxml_add_child(capxml,"VendorSpecificCapabilities",0);
   ezxml_t toplayer = ezxml_add_child(capxml,"Layer",0);
   
   apr_hash_index_t *tileindex_index = apr_hash_first(ctx->pool,cfg->tilesets);

   while(tileindex_index) {
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(tileindex_index,&key,&keylen,(void**)&tileset);
      
      ezxml_t layerxml = ezxml_add_child(toplayer,"Layer",0);
      ezxml_set_attr(layerxml, "cascaded", "1");
      ezxml_set_attr(layerxml, "queryable", tileset->source->info_formats?"1":"0"),
      ezxml_set_txt(ezxml_add_child(layerxml,"Name",0),tileset->name);

      ezxml_t tsxml = ezxml_add_child(vendorxml, "TileSet",0);
      
      /*optional layer title*/
      const char *title = apr_table_get(tileset->metadata,"title");
      if(title) {
         ezxml_set_txt(ezxml_add_child(layerxml,"Title",0),title);
      }

      /*optional layer abstract*/
      const char *abstract = apr_table_get(tileset->metadata,"abstract");
      if(abstract) {
         ezxml_set_txt(ezxml_add_child(layerxml,"Abstract",0),abstract);
      }

      if(tileset->wgs84bbox[0] != tileset->wgs84bbox[2]) {
         ezxml_t wgsxml = ezxml_add_child(layerxml,"LatLonBoundingBox",0);
         ezxml_set_attr(wgsxml,"minx",apr_psprintf(ctx->pool,"%f",tileset->wgs84bbox[0]));
         ezxml_set_attr(wgsxml,"miny",apr_psprintf(ctx->pool,"%f",tileset->wgs84bbox[1]));
         ezxml_set_attr(wgsxml,"maxx",apr_psprintf(ctx->pool,"%f",tileset->wgs84bbox[2]));
         ezxml_set_attr(wgsxml,"maxy",apr_psprintf(ctx->pool,"%f",tileset->wgs84bbox[3]));
      }

      if(tileset->dimensions) {
         int i;
         for(i=0;i<tileset->dimensions->nelts;i++) {
            geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
            ezxml_t dimxml = ezxml_add_child(layerxml,"Dimension",0);
            ezxml_set_attr(dimxml,"name",dimension->name);
            ezxml_set_attr(dimxml,"default",dimension->default_value);

            if(dimension->unit) {
               ezxml_set_attr(dimxml,"units",dimension->unit);
            }
            const char **value = dimension->print_ogc_formatted_values(ctx,dimension);
            char *dimval = apr_pstrdup(ctx->pool,*value);
            value++;
            while(*value) {
               dimval = apr_pstrcat(ctx->pool,dimval,",",*value,NULL);
               value++;
            }
            ezxml_set_txt(dimxml,dimval);
         }
      }


      int i;
      for(i=0;i<tileset->grid_links->nelts;i++) {
         geocache_grid_link *gridlink = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
         geocache_grid *grid = gridlink->grid;
         double *extent = grid->extent;
         if(gridlink->restricted_extent)
            extent = gridlink->restricted_extent;
         ezxml_t bboxxml = ezxml_add_child(layerxml,"BoundingBox",0);
         ezxml_set_attr(bboxxml,"SRS", grid->srs);
         ezxml_set_attr(bboxxml,"minx",apr_psprintf(ctx->pool,"%f",extent[0]));
         ezxml_set_attr(bboxxml,"miny",apr_psprintf(ctx->pool,"%f",extent[1]));
         ezxml_set_attr(bboxxml,"maxx",apr_psprintf(ctx->pool,"%f",extent[2]));
         ezxml_set_attr(bboxxml,"maxy",apr_psprintf(ctx->pool,"%f",extent[3]));
         ezxml_set_txt(ezxml_add_child(layerxml,"SRS",0),grid->srs);
         int j;
         for(j=0;j<gridlink->grid->srs_aliases->nelts;j++) {
            ezxml_set_txt(ezxml_add_child(layerxml,"SRS",0),APR_ARRAY_IDX(gridlink->grid->srs_aliases,j,char*));
         }


         if(i==0) {
            /*wms-c only supports one grid per layer, so we use the first of the tileset's grids */
            ezxml_set_txt(ezxml_add_child(tsxml,"SRS",0),grid->srs);
            tmpxml = ezxml_add_child(tsxml,"BoundingBox",0);
            ezxml_set_attr(tmpxml,"SRS",grid->srs);
            ezxml_set_attr(tmpxml,"minx",apr_psprintf(ctx->pool,"%f",grid->extent[0]));
            ezxml_set_attr(tmpxml,"miny",apr_psprintf(ctx->pool,"%f",grid->extent[1]));
            ezxml_set_attr(tmpxml,"maxx",apr_psprintf(ctx->pool,"%f",grid->extent[2]));
            ezxml_set_attr(tmpxml,"maxy",apr_psprintf(ctx->pool,"%f",grid->extent[3]));

            char *resolutions="";

            int i;
            for(i=0;i<grid->nlevels;i++) {
               resolutions = apr_psprintf(ctx->pool,"%s%.20f ",resolutions,grid->levels[i]->resolution);
            }
            ezxml_set_txt(ezxml_add_child(tsxml,"Resolutions",0),resolutions);
            ezxml_set_txt(ezxml_add_child(tsxml,"Width",0),apr_psprintf(ctx->pool,"%d",grid->tile_sx));
            ezxml_set_txt(ezxml_add_child(tsxml,"Height",0),apr_psprintf(ctx->pool,"%d", grid->tile_sy));
         }
      }
      if(tileset->format && tileset->format->mime_type) {
         ezxml_set_txt(ezxml_add_child(tsxml,"Format",0),tileset->format->mime_type);
      } else {
         ezxml_set_txt(ezxml_add_child(tsxml,"Format",0),"image/unknown");
      }
      ezxml_set_txt(ezxml_add_child(tsxml,"Layers",0),tileset->name);
      ezxml_set_txt(ezxml_add_child(tsxml,"Styles",0),"");
      tileindex_index = apr_hash_next(tileindex_index);
   }

   
   char *tmpcaps = ezxml_toxml(caps);
   ezxml_free(caps);
   static char *capheader=
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\
<!DOCTYPE WMT_MS_Capabilities SYSTEM \"http://schemas.opengis.net/wms/1.1.0/capabilities_1_1_0.dtd\"\
[\
 <!ELEMENT VendorSpecificCapabilities EMPTY>\
]>\n";
   request->request.capabilities = apr_pstrcat(ctx->pool,capheader,tmpcaps,NULL);
   free(tmpcaps);
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
            int tmpx,tmpy,tmpz;
            if(grid_link->grid->tile_sx != width || grid_link->grid->tile_sy != height ||
                  geocache_grid_get_cell(ctx, grid_link->grid, bbox, &tmpx,&tmpy,&tmpz) != GEOCACHE_SUCCESS) {
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

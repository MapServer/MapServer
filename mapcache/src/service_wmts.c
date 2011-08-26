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



static ezxml_t _wmts_capabilities() {
   ezxml_t node = ezxml_new("Capabilities");
   ezxml_set_attr(node,"xmlns","http://www.opengis.net/wmts/1.0");
   ezxml_set_attr(node,"xmlns:ows","http://www.opengis.net/ows/1.1");
   ezxml_set_attr(node,"xmlns:xlink","http://www.w3.org/1999/xlink");
   ezxml_set_attr(node,"xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
   ezxml_set_attr(node,"xmlns:gml","http://www.opengis.net/gml");
   ezxml_set_attr(node,"xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd");
   ezxml_set_attr(node,"version","1.0.0");
   return node;
}

static ezxml_t _wmts_service_identification(geocache_context *ctx, const char *title) {
   ezxml_t node = ezxml_new("ows:ServiceIdentification");
   ezxml_set_txt(ezxml_add_child(node,"ows:Title",0),title);
   ezxml_set_txt(ezxml_add_child(node,"ows:ServiceType",0),"OGC WMTS");
   ezxml_set_txt(ezxml_add_child(node,"ows:ServiceTypeVersion",0),"1.0.0");
   return node;
}

static ezxml_t _wmts_operations_metadata(geocache_context *ctx, const char *onlineresource, const char *operationstr) {
   ezxml_t operation = ezxml_new("ows:Operation");
   ezxml_set_attr(operation,"name",operationstr);
   ezxml_t dcp = ezxml_add_child(operation,"ows:DCP",0);
   ezxml_t http = ezxml_add_child(dcp,"ows:HTTP",0);
   ezxml_t get = ezxml_add_child(http,"ows:Get",0);
   ezxml_set_attr(get,"xlink:href",apr_pstrcat(ctx->pool,onlineresource,"wmts?",NULL));
   ezxml_t constraint = ezxml_add_child(get,"ows:Constraint",0);
   ezxml_set_attr(constraint,"name","GetEncoding");
   ezxml_t allowedvalues = ezxml_add_child(constraint,"ows:AllowedValues",0);
   ezxml_set_txt(ezxml_add_child(allowedvalues,"ows:Value",0),"KVP");
   return operation;

}

static ezxml_t _wmts_service_provider(geocache_context *ctx, const char *onlineresource, const char *contact) {
   ezxml_t node = ezxml_new("ows:ServiceProvider");
   ezxml_set_txt(ezxml_add_child(node,"ows:ProviderName",0),contact);
   ezxml_set_attr(ezxml_add_child(node,"ows:ProviderSite",0),"xlink:href",onlineresource);
   return node;
}


void _create_capabilities_wmts(geocache_context *ctx, geocache_request_get_capabilities *req, char *url, char *path_info, geocache_cfg *cfg) {
   geocache_request_get_capabilities_wmts *request = (geocache_request_get_capabilities_wmts*)req;
   ezxml_t caps;
   ezxml_t contents;
   ezxml_t operations_metadata;
#ifdef DEBUG
   if(request->request.request.type != GEOCACHE_REQUEST_GET_CAPABILITIES) {
      ctx->set_error(ctx,500,"wrong wmts capabilities request");
      return;
   }
#endif
   
   const char *title;
   const char *onlineresource = apr_table_get(cfg->metadata,"url");
   if(!onlineresource) {
      onlineresource = url;
   }
   
   title = apr_table_get(cfg->metadata,"title");
   if(!title) {
      title = "no title set, add some in metadata";
   }
   
   request->request.mime_type = apr_pstrdup(ctx->pool,"application/xml");
   
   caps = _wmts_capabilities();
   ezxml_insert(_wmts_service_identification(ctx,title),caps,0);
   ezxml_insert(_wmts_service_provider(ctx,onlineresource,"contact_todo"),caps,0);

   operations_metadata = ezxml_add_child(caps,"ows:OperationsMetadata",0);
   ezxml_insert(_wmts_operations_metadata(ctx,onlineresource,"GetCapabilities"),operations_metadata,0);
   ezxml_insert(_wmts_operations_metadata(ctx,onlineresource,"GetTile"),operations_metadata,0);
   ezxml_insert(_wmts_operations_metadata(ctx,onlineresource,"GetFeatureInfo"),operations_metadata,0);

   contents = ezxml_add_child(caps,"Contents",0);
   
   apr_hash_index_t *layer_index = apr_hash_first(ctx->pool,cfg->tilesets);
   while(layer_index) {
      int i;
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(layer_index,&key,&keylen,(void**)&tileset);
      
      ezxml_t layer = ezxml_add_child(contents,"Layer",0);
      const char *title = apr_table_get(tileset->metadata,"title");
      if(title) {
         ezxml_set_txt(ezxml_add_child(layer,"ows:Title",0),title);
      } else {
         ezxml_set_txt(ezxml_add_child(layer,"ows:Title",0),tileset->name);
      }
      const char *abstract = apr_table_get(tileset->metadata,"abstract");
      if(abstract) {
         ezxml_set_txt(ezxml_add_child(layer,"ows:Abstract",0),abstract);
      }

      ezxml_set_txt(ezxml_add_child(layer,"ows:Identifier",0),tileset->name);
      
      ezxml_t style = ezxml_add_child(layer,"Style",0);
      ezxml_set_attr(style,"isDefault","true");
      ezxml_set_txt(ezxml_add_child(style,"ows:Identifier",0),"default");
      
      if(tileset->format && tileset->format->mime_type)
         ezxml_set_txt(ezxml_add_child(layer,"Format",0),tileset->format->mime_type);
      else
         ezxml_set_txt(ezxml_add_child(layer,"Format",0),"image/unknown");
   

      char *dimensionstemplate="";
      if(tileset->dimensions) {
         for(i=0;i<tileset->dimensions->nelts;i++) {
            geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
            ezxml_t dim = ezxml_add_child(layer,"Dimension",0);
            ezxml_set_txt(ezxml_add_child(dim,"ows:Identifier",0),dimension->name);
            ezxml_set_txt(ezxml_add_child(dim,"Default",0),dimension->default_value);

            if(dimension->unit) {
               ezxml_set_txt(ezxml_add_child(dim,"UOM",0),dimension->unit);
            }
            const char **values = dimension->print_ogc_formatted_values(ctx,dimension);
            const char **value = values;
            while(*value) {
               ezxml_set_txt(ezxml_add_child(dim,"Value",0),*value);
               value++;
            }
            dimensionstemplate = apr_pstrcat(ctx->pool,dimensionstemplate,"{",dimension->name,"}/",NULL);
         }
      }
      if(tileset->source && tileset->source->info_formats) {
         int i;
         for(i=0;i<tileset->source->info_formats->nelts;i++) {
            char *iformat = APR_ARRAY_IDX(tileset->source->info_formats,i,char*);
            ezxml_set_txt(ezxml_add_child(layer,"InfoFormat",0),iformat);
            ezxml_t resourceurl = ezxml_add_child(layer,"ResourceURL",0);
            ezxml_set_attr(resourceurl,"format",iformat);
            ezxml_set_attr(resourceurl,"resourcetype","FeatureInfo");
            ezxml_set_attr(resourceurl,"template",
                  apr_pstrcat(ctx->pool,onlineresource,"wmts/1.0.0/",tileset->name,"/default/",
                     dimensionstemplate,"{TileMatrixSet}/{TileMatrix}/{TileRow}/{TileCol}.",apr_psprintf(ctx->pool,"%d",i),NULL));
         }
      }

      ezxml_t resourceurl = ezxml_add_child(layer,"ResourceURL",0);
      if(tileset->format && tileset->format->mime_type)
         ezxml_set_attr(resourceurl,"format",tileset->format->mime_type);
      else
         ezxml_set_attr(resourceurl,"format","image/unknown");
      ezxml_set_attr(resourceurl,"resourceType","tile");
      ezxml_set_attr(resourceurl,"template",
            apr_pstrcat(ctx->pool,onlineresource,"wmts/1.0.0/",tileset->name,"/default/",
               dimensionstemplate,"{TileMatrixSet}/{TileMatrix}/{TileRow}/{TileCol}.",
               ((tileset->format)?tileset->format->extension:"xxx"),NULL));
      

      if(tileset->wgs84bbox[0] != tileset->wgs84bbox[2]) {
         ezxml_t bbox = ezxml_add_child(layer,"ows:WGS84BoundingBox",0);
         ezxml_set_txt(ezxml_add_child(bbox,"ows:LowerCorner",0),
               apr_psprintf(ctx->pool,"%f %f",tileset->wgs84bbox[0], tileset->wgs84bbox[1]));
         ezxml_set_txt(ezxml_add_child(bbox,"ows:UpperCorner",0),
               apr_psprintf(ctx->pool,"%f %f",tileset->wgs84bbox[2], tileset->wgs84bbox[3]));
      }

      for(i=0;i<tileset->grid_links->nelts;i++) {
         geocache_grid_link *grid_link = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
         ezxml_t tmsetlnk = ezxml_add_child(layer,"TileMatrixSetLink",0);
         ezxml_set_txt(ezxml_add_child(tmsetlnk,"TileMatrixSet",0),grid_link->grid->name);

         if(grid_link->restricted_extent) {
            ezxml_t limits = ezxml_add_child(tmsetlnk,"TileMatrixSetLimits",0);
            int j;
            for(j=0;j<grid_link->grid->nlevels;j++) {
               ezxml_t matrixlimits = ezxml_add_child(limits,"TileMatrixLimits",0);
               ezxml_set_txt(ezxml_add_child(matrixlimits,"TileMatrix",0),
                        apr_psprintf(ctx->pool,"%s:%d",grid_link->grid->name,j));
               ezxml_set_txt(ezxml_add_child(matrixlimits,"MinTileRow",0),
                        apr_psprintf(ctx->pool,"%d",grid_link->grid_limits[j][0]));
               ezxml_set_txt(ezxml_add_child(matrixlimits,"MaxTileRow",0),
                        apr_psprintf(ctx->pool,"%d",grid_link->grid_limits[j][2]-1));
               ezxml_set_txt(ezxml_add_child(matrixlimits,"MinTileCol",0),
                        apr_psprintf(ctx->pool,"%d",grid_link->grid_limits[j][1]));
               ezxml_set_txt(ezxml_add_child(matrixlimits,"MaxTileCol",0),
                        apr_psprintf(ctx->pool,"%d",grid_link->grid_limits[j][3]-1));
            }
         }


         double *gbbox = grid_link->restricted_extent?grid_link->restricted_extent:grid_link->grid->extent;
         ezxml_t bbox = ezxml_add_child(layer,"ows:BoundingBox",0);
         ezxml_set_txt(ezxml_add_child(bbox,"ows:LowerCorner",0),
               apr_psprintf(ctx->pool,"%f %f",gbbox[0], gbbox[1]));
         ezxml_set_txt(ezxml_add_child(bbox,"ows:UpperCorner",0),
               apr_psprintf(ctx->pool,"%f %f",gbbox[2], gbbox[3]));
         ezxml_set_attr(bbox,"crs",geocache_grid_get_crs(ctx,grid_link->grid));
      }
      layer_index = apr_hash_next(layer_index);
   }

   
   
   apr_hash_index_t *grid_index = apr_hash_first(ctx->pool,cfg->grids);
   while(grid_index) {
      geocache_grid *grid;
      const void *key; apr_ssize_t keylen;
      int level;
      const char *WellKnownScaleSet;
      apr_hash_this(grid_index,&key,&keylen,(void**)&grid);
      
      WellKnownScaleSet = apr_table_get(grid->metadata,"WellKnownScaleSet");
     
      ezxml_t tmset = ezxml_add_child(contents,"TileMatrixSet",0);
      ezxml_set_txt(ezxml_add_child(tmset,"ows:Identifier",0),grid->name);
      const char *title = apr_table_get(grid->metadata,"title");
      if(title) {
         ezxml_set_txt(ezxml_add_child(tmset,"ows:Title",0),title);
      }
      ezxml_set_txt(ezxml_add_child(tmset,"ows:SupportedCRS",0),geocache_grid_get_crs(ctx,grid));

      ezxml_t bbox = ezxml_add_child(tmset,"ows:BoundingBox",0);

      ezxml_set_txt(ezxml_add_child(bbox,"LowerCorner",0),apr_psprintf(ctx->pool,"%f %f",
               grid->extent[0], grid->extent[1]));
      ezxml_set_txt(ezxml_add_child(bbox,"UpperCorner",0),apr_psprintf(ctx->pool,"%f %f",
               grid->extent[2], grid->extent[3]));

      
      if(WellKnownScaleSet) {
         ezxml_set_txt(ezxml_add_child(tmset,"WellKnownScaleSet",0),WellKnownScaleSet);
      }
      
      for(level=0;level<grid->nlevels;level++) {
         geocache_grid_level *glevel = grid->levels[level];
         ezxml_t tm = ezxml_add_child(tmset,"TileMatrix",0);
         ezxml_set_txt(ezxml_add_child(tm,"ows:Identifier",0),apr_psprintf(ctx->pool,"%d",level));
         double scaledenom = glevel->resolution * geocache_meters_per_unit[grid->unit] / 0.00028;
         ezxml_set_txt(ezxml_add_child(tm,"ScaleDenominator",0),apr_psprintf(ctx->pool,"%.20f",scaledenom));
         ezxml_set_txt(ezxml_add_child(tm,"TopLeftCorner",0),apr_psprintf(ctx->pool,"%f %f",
                  grid->extent[0],
                  grid->extent[1] + glevel->maxy * glevel->resolution * grid->tile_sy));
         ezxml_set_txt(ezxml_add_child(tm,"TileWidth",0),apr_psprintf(ctx->pool,"%d",grid->tile_sx));
         ezxml_set_txt(ezxml_add_child(tm,"TileHeight",0),apr_psprintf(ctx->pool,"%d",grid->tile_sy));
         ezxml_set_txt(ezxml_add_child(tm,"MatrixWidth",0),apr_psprintf(ctx->pool,"%d",glevel->maxx+1));
         ezxml_set_txt(ezxml_add_child(tm,"MatrixHeight",0),apr_psprintf(ctx->pool,"%d",glevel->maxy+1));
      }
      grid_index = apr_hash_next(grid_index);
   }
   char *tmpcaps = ezxml_toxml(caps);
   ezxml_free(caps);
   request->request.capabilities = apr_pstrcat(ctx->pool,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n",tmpcaps,NULL);
   free(tmpcaps);
}

/**
 * \brief parse a WMTS request
 * \private \memberof geocache_service_wmts
 * \sa geocache_service::parse_request()
 */
void _geocache_service_wmts_parse_request(geocache_context *ctx, geocache_service *this, geocache_request **request,
      const char *pathinfo, apr_table_t *params, geocache_cfg *config) {
   const char *str, *service = NULL, *style = NULL, *version = NULL, *layer = NULL, *matrixset = NULL,
               *matrix = NULL, *tilecol = NULL, *tilerow = NULL, *format = NULL, *extension = NULL,
               *infoformat = NULL, *fi_i = NULL, *fi_j = NULL;
   apr_table_t *dimtable = NULL;
   geocache_tileset *tileset = NULL;
   int row,col,level;
   service = apr_table_get(params,"SERVICE");
   if(service) {
      /*KVP Parsing*/
      if( strcasecmp(service,"wmts") ) {
         ctx->set_error(ctx,400,"received wmts request with invalid service param %s", service);
         return;
      }
      str = apr_table_get(params,"REQUEST");
      if(!str) {
         ctx->set_error(ctx, 400, "received wmts request with no request");
         return;
      }
      if( ! strcasecmp(str,"getcapabilities") ) {
         geocache_request_get_capabilities_wmts *req = (geocache_request_get_capabilities_wmts*)
            apr_pcalloc(ctx->pool,sizeof(geocache_request_get_capabilities_wmts));
         req->request.request.type = GEOCACHE_REQUEST_GET_CAPABILITIES;
         *request = (geocache_request*)req;
         return;
      } else if( ! strcasecmp(str,"gettile") || ! strcasecmp(str,"getfeatureinfo")) {
         /* extract our wnated parameters, they will be validated later on */
         tilerow = apr_table_get(params,"TILEROW");
         style = apr_table_get(params,"STYLE");
         tilecol = apr_table_get(params,"TILECOL");
         format = apr_table_get(params,"FORMAT");
         layer = apr_table_get(params,"LAYER");
         if(!layer) { /*we have to validate this now in order to be able to extract dimensions*/
            ctx->set_error(ctx, 400, "received wmts request with no layer");
            return;
         } else {
            tileset = geocache_configuration_get_tileset(config,layer);
            if(!tileset) {
               ctx->set_error(ctx, 400, "received wmts request with invalid layer %s",layer);
               return;
            }
         }
         matrixset = apr_table_get(params,"TILEMATRIXSET");
         matrix = apr_table_get(params,"TILEMATRIX");
         if(tileset->dimensions) {
            int i;
            dimtable = apr_table_make(ctx->pool,tileset->dimensions->nelts);
            for(i=0;i<tileset->dimensions->nelts;i++) {
               geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
               const char *value;
               if((value = apr_table_get(params,dimension->name)) != NULL) {
                  apr_table_set(dimtable,dimension->name,value);
               } else {
                  apr_table_set(dimtable,dimension->name,dimension->default_value);
               }
            }
         }
         if(!strcasecmp(str,"getfeatureinfo")) {
            infoformat = apr_table_get(params,"INFOFORMAT");
            fi_i = apr_table_get(params,"I");
            fi_j = apr_table_get(params,"J");
            if(!infoformat || !fi_i || !fi_j) {
               ctx->set_error(ctx, 400, "received wmts featureinfo request with missing infoformat, i or j");
               return;
            }
         }
      } else {
         ctx->set_error(ctx, 501, "received wmts request with invalid request %s",str);
         return;
      }
   } else {
      char *key, *last;
      for (key = apr_strtok(apr_pstrdup(ctx->pool,pathinfo), "/", &last); key != NULL;
            key = apr_strtok(NULL, "/", &last)) {
         if(!version) {
            version = key;
            if(strcmp(version,"1.0.0")) {
               ctx->set_error(ctx,404, "received wmts request with invalid version \"%s\" (expecting \"1.0.0\")", version);
               return;
            }
            continue;
         }
         if(!layer) {
            if(!strcmp(key,"WMTSCapabilities.xml")) {
               geocache_request_get_capabilities_wmts *req = (geocache_request_get_capabilities_wmts*)
                  apr_pcalloc(ctx->pool,sizeof(geocache_request_get_capabilities_wmts));
               req->request.request.type = GEOCACHE_REQUEST_GET_CAPABILITIES;
               *request = (geocache_request*)req;
               return;
            }
            layer = key;
            tileset = geocache_configuration_get_tileset(config,layer);
            if(!tileset) {
               ctx->set_error(ctx, 404, "received wmts request with invalid layer %s",layer);
               return;
            }
            continue;
         }
         if(!style) {
            style = key;
            continue;
         }
         if(tileset->dimensions) {
            if(!dimtable)
               dimtable = apr_table_make(ctx->pool,tileset->dimensions->nelts);
            int i = apr_table_elts(dimtable)->nelts;
            if(i != tileset->dimensions->nelts) {
               /*we still have some dimensions to parse*/
               geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
               apr_table_set(dimtable,dimension->name,key);
               continue;
            }
         }
         if(!matrixset) {
            matrixset = key;
            continue;
         }
         if(!matrix) {
            matrix=key;
            continue;
         }
         if(!tilerow) {
            tilerow = key;
            continue;
         }

         if(!tilecol) {
            /*if we have a get tile request this is the last element of the uri, and it will also contain the file extension*/
            
            /*split the string at the first '.'*/
            char *charptr = key;
            while(*charptr && *charptr != '.') charptr++;

            if(*charptr == '.') {
               /*replace '.' with '\0' and advance*/
               *charptr++ = '\0';
               tilecol = key;
               extension = charptr;
            } else {
               tilecol = key;
            }
            continue;
         }

         if(!fi_j) {
            fi_j = key;
            continue;
         }

         if(!fi_i) {
            /*split the string at the first '.'*/
            char *charptr = key;
            while(*charptr && *charptr != '.') charptr++;

            /*replace '.' with '\0' and advance*/
            *charptr++ = '\0';
            fi_i = key;
            extension = charptr;
            continue;
         }

         ctx->set_error(ctx,404,"received request with trailing data starting with %s",key);
         return;
      }
   }
      
   geocache_grid_link *grid_link = NULL;
   
   if(!style || strcmp(style,"default")) {
      ctx->set_error(ctx,404, "received request with invalid style \"%s\" (expecting \"default\")",style);
      return;
   }
   
   /*validate dimensions*/
   if(tileset->dimensions) {
      if(!dimtable) {
         ctx->set_error(ctx,404, "received request with no dimensions");
         return;
      }
      int i;
      for(i=0;i<tileset->dimensions->nelts;i++) {
         geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
         const char *value = apr_table_get(dimtable,dimension->name);
         if(!value) {
            ctx->set_error(ctx,404,"received request with no value for dimension \"%s\"",dimension->name);
            return;
         }
         char *tmpval = apr_pstrdup(ctx->pool,value);
         int ok = dimension->validate(ctx,dimension,&tmpval);
         GC_CHECK_ERROR(ctx);
         if(ok != GEOCACHE_SUCCESS) {
            ctx->set_error(ctx,404,"dimension \"%s\" value \"%s\" fails to validate",
                  dimension->name, value);
            return;
         }
         
         /* re-set the eventually modified value in the dimension table */
         apr_table_set(dimtable,dimension->name,tmpval);
         
      }
   }

   if(!matrixset) {
      ctx->set_error(ctx, 404, "received wmts request with no TILEMATRIXSET");
      return;
   } else {
      int i;
      for(i=0;i<tileset->grid_links->nelts;i++) {
         geocache_grid_link *sgrid = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
         if(strcmp(sgrid->grid->name,matrixset)) continue;
         grid_link = sgrid;
         break;
      }
      if(!grid_link) {
         ctx->set_error(ctx, 404, "received wmts request with invalid TILEMATRIXSET %s",matrixset);
         return;
      }
   }

   if(!matrix) {
      ctx->set_error(ctx, 404, "received wmts request with no TILEMATRIX");
      return;
   } else {
      char *endptr;
      level = (int)strtol(matrix,&endptr,10);
      if(*endptr != 0 || level < 0 || level >= grid_link->grid->nlevels) {
         ctx->set_error(ctx, 404, "received wmts request with invalid TILEMATRIX %s", matrix);
         return;
      }
   }
   
   if(!tilerow) {
      ctx->set_error(ctx, 404, "received wmts request with no TILEROW");
      return;
   } else {
      char *endptr;
      row = (int)strtol(tilerow,&endptr,10);
      if(*endptr != 0 || row < 0) {
         ctx->set_error(ctx, 404, "received wmts request with invalid TILEROW %s",tilerow);
         return;
      }
   }

   if(!tilecol) {
      ctx->set_error(ctx, 404, "received wmts request with no TILECOL");
      return;
   } else {
      char *endptr;
      col = (int)strtol(tilecol,&endptr,10);
      if(endptr == tilecol || col < 0) {
         ctx->set_error(ctx, 404, "received wmts request with invalid TILECOL %s",tilecol);
         return;
      }
   }

   if(!fi_j) { /*we have a getTile request*/

#ifdef PEDANTIC_WMTS_FORMAT_CHECK
      if(tileset->format) {
         if(!format && !extension) {
            ctx->set_error(ctx, 404, "received wmts request with no format");
            return;
         } else {
            if(format && tileset->format && strcmp(format,tileset->format->mime_type)) {
               ctx->set_error(ctx, 404, "received wmts request with invalid format \"%s\" (expecting %s)",
                     format,tileset->format->mime_type);
               return;
            }
            if(extension && tileset->format && strcmp(extension,tileset->format->extension)) {
               ctx->set_error(ctx, 404, "received wmts request with invalid extension \"%s\" (expecting %s)",
                     extension,tileset->format->extension);
               return;
            }
         }
      }
#endif


      geocache_request_get_tile *req = (geocache_request_get_tile*)apr_pcalloc(
            ctx->pool,sizeof(geocache_request_get_tile));

      req->request.type = GEOCACHE_REQUEST_GET_TILE;
      req->ntiles = 1;
      req->tiles = (geocache_tile**)apr_pcalloc(ctx->pool,sizeof(geocache_tile*));     


      req->tiles[0] = geocache_tileset_tile_create(ctx->pool, tileset, grid_link);
      if(!req->tiles[0]) {
         ctx->set_error(ctx, 500, "failed to allocate tile");
         return;
      }

      /*populate dimensions*/
      if(tileset->dimensions) {
         int i;
         req->tiles[0]->dimensions = apr_table_make(ctx->pool,tileset->dimensions->nelts);
         for(i=0;i<tileset->dimensions->nelts;i++) {
            geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
            const char *value = apr_table_get(dimtable,dimension->name);
            apr_table_set(req->tiles[0]->dimensions,dimension->name,value);
         }
      }

      req->tiles[0]->x = col;
      req->tiles[0]->y = grid_link->grid->levels[level]->maxy - row - 1;
      req->tiles[0]->z = level;

      geocache_tileset_tile_validate(ctx,req->tiles[0]);
      GC_CHECK_ERROR(ctx);

      *request = (geocache_request*)req;
      return;
   } else { /* we have a featureinfo request */
      if(!fi_i || (!infoformat && !extension)) {
         ctx->set_error(ctx,400,"received wmts featureinfo request with missing i,j, or format");
         return;
      }
            
      char *endptr;
      if(!tileset->source || !tileset->source->info_formats) {
         ctx->set_error(ctx,400,"tileset %s does not support featureinfo requests", tileset->name);
         return;
      }
      geocache_request_get_feature_info *req_fi = (geocache_request_get_feature_info*)apr_pcalloc(
            ctx->pool, sizeof(geocache_request_get_feature_info));
      req_fi->request.type = GEOCACHE_REQUEST_GET_FEATUREINFO;
      geocache_feature_info *fi = geocache_tileset_feature_info_create(ctx->pool, tileset, grid_link);
      req_fi->fi = fi;
      if(infoformat) {
         fi->format = (char*)infoformat;
      }
      if(extension) {
         int fi_index = (int)strtol(extension,&endptr,10);
         if(endptr == extension || fi_index < 0 || fi_index >= tileset->source->info_formats->nelts) {
            ctx->set_error(ctx, 404, "received wmts featureinfo request with invalid extension %s",extension);
            return;
         }
         fi->format = APR_ARRAY_IDX(tileset->source->info_formats,fi_index,char*);
      }


      fi->i = (int)strtol(fi_i,&endptr,10);
      if(*endptr != 0 || fi->i < 0 || fi->i >= grid_link->grid->tile_sx) {
         ctx->set_error(ctx, 404, "received wmts featureinfo request with invalid I %s",fi_i);
         return;
      }
      fi->j = (int)strtol(fi_j,&endptr,10);
      if(*endptr != 0 || fi->j < 0 || fi->j >= grid_link->grid->tile_sy) {
         ctx->set_error(ctx, 404, "received wmts featureinfo request with invalid J %s",fi_j);
         return;
      }
      fi->map.width = grid_link->grid->tile_sx;
      fi->map.height = grid_link->grid->tile_sy;
      geocache_grid_get_extent(ctx,grid_link->grid,
            col,
            grid_link->grid->levels[level]->maxy-row-1,
            level,
            fi->map.extent);
      *request = (geocache_request*)req_fi;
   }
}

geocache_service* geocache_service_wmts_create(geocache_context *ctx) {
   geocache_service_wmts* service = (geocache_service_wmts*)apr_pcalloc(ctx->pool, sizeof(geocache_service_wmts));
   if(!service) {
      ctx->set_error(ctx, 500, "failed to allocate wtms service");
      return NULL;
   }
   service->service.url_prefix = apr_pstrdup(ctx->pool,"wmts");
   service->service.name = apr_pstrdup(ctx->pool,"wmts");
   service->service.type = GEOCACHE_SERVICE_WMTS;
   service->service.parse_request = _geocache_service_wmts_parse_request;
   service->service.create_capabilities_response = _create_capabilities_wmts;
   return (geocache_service*)service;
}

/** @} */
/* vim: ai ts=3 sts=3 et sw=3
*/

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


static const char *wmts_0 = 
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<Capabilities xmlns=\"http://www.opengis.net/wmts/1.0\"\n"
      "xmlns:ows=\"http://www.opengis.net/ows/1.1\"\n"
      "xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
      "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
      "xmlns:gml=\"http://www.opengis.net/gml\" xsi:schemaLocation=\"http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd\"\n"
      "version=\"1.0.0\">\n"
      "<ows:ServiceIdentification>\n"
      "  <ows:Title>%s</ows:Title>\n"
      "  <ows:ServiceType>OGC WMTS</ows:ServiceType>\n"
      "  <ows:ServiceTypeVersion>1.0.0</ows:ServiceTypeVersion>\n"
      "</ows:ServiceIdentification>\n"
      "<ows:ServiceProvider>\n"
      "  <ows:ProviderName>%s</ows:ProviderName>\n"
      "  <ows:ProviderSite xlink:href=\"%s/wmts?\" />\n"
      "  <ows:ServiceContact>\n"
      "    <ows:IndividualName>%s</ows:IndividualName>\n"
      "  </ows:ServiceContact>\n"
      "</ows:ServiceProvider>\n"
      "<ows:OperationsMetadata>\n"
      "  <ows:Operation name=\"GetCapabilities\">\n"
      "    <ows:DCP>\n"
      "      <ows:HTTP>\n"
      "        <ows:Get xlink:href=\"%s/wmts?\">\n"
      "          <ows:Constraint name=\"GetEncoding\">\n"
      "            <ows:AllowedValues>\n"
      "              <ows:Value>KVP</ows:Value>\n"
      "            </ows:AllowedValues>\n"
      "          </ows:Constraint>\n"
      "        </ows:Get>\n"
      "      </ows:HTTP>\n"
      "    </ows:DCP>\n"
      "  </ows:Operation>\n"
      "  <ows:Operation name=\"GetTile\">\n"
      "    <ows:DCP>\n"
      "      <ows:HTTP>\n"
      "        <ows:Get xlink:href=\"%s/wmts?\">\n"
      "          <ows:Constraint name=\"GetEncoding\">\n"
      "            <ows:AllowedValues>\n"
      "              <ows:Value>KVP</ows:Value>\n"
      "            </ows:AllowedValues>\n"
      "          </ows:Constraint>\n"
      "        </ows:Get>\n"
      "      </ows:HTTP>\n"
      "    </ows:DCP>\n"
      "  </ows:Operation>\n"
      "</ows:OperationsMetadata>\n"
      "<Contents>\n";
      

static const char *wmts_matrix = 
      "    <TileMatrix>\n"
      "      <ows:Identifier>%d</ows:Identifier>\n"
      "      <ScaleDenominator>%.20f</ScaleDenominator>\n"
      "      <TopLeftCorner>%f %f</TopLeftCorner>\n"
      "      <TileWidth>%d</TileWidth>\n"
      "      <TileHeight>%d</TileHeight>\n"
      "      <MatrixWidth>%d</MatrixWidth>\n"
      "      <MatrixHeight>%d</MatrixHeight>\n"
      "    </TileMatrix>\n";

void _create_capabilities_wmts(geocache_context *ctx, geocache_request_get_capabilities *req, char *url, char *path_info, geocache_cfg *cfg) {
   geocache_request_get_capabilities_wmts *request = (geocache_request_get_capabilities_wmts*)req;
   char *caps;
#ifdef DEBUG
   if(request->request.request.type != GEOCACHE_REQUEST_GET_CAPABILITIES) {
      ctx->set_error(ctx,500,"wrong wms capabilities request");
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
   
   caps = apr_psprintf(ctx->pool,wmts_0,
         title, "providername_todo", onlineresource, "individualname_todo",
         onlineresource,onlineresource,onlineresource);
   
   
   apr_hash_index_t *layer_index = apr_hash_first(ctx->pool,cfg->tilesets);
   while(layer_index) {
      int i;
      geocache_tileset *tileset;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(layer_index,&key,&keylen,(void**)&tileset);
      
      const char *title = apr_table_get(tileset->metadata,"title");
      if(!title) {
         title = "no title set, add some in metadata";
      }
      const char *abstract = apr_table_get(tileset->metadata,"abstract");
      if(!abstract) {
         abstract = "no abstract set, add some in metadata";
      }

      char *dimensions="";
      char *dimensionstemplate="";
      if(tileset->dimensions) {
         for(i=0;i<tileset->dimensions->nelts;i++) {
            geocache_dimension *dimension = APR_ARRAY_IDX(tileset->dimensions,i,geocache_dimension*);
            dimensions = apr_psprintf(ctx->pool,"%s"
                  "    <Dimension>\n"
                  "      <ows:Identifier>%s</ows:Identifier>\n"
                  "      <Default>%s</Default>\n",
                  dimensions,
                  dimension->name,
                  dimension->default_value);
            if(dimension->unit) {
               dimensions = apr_pstrcat(ctx->pool,dimensions,
                  "      <UOM>",dimension->unit,"</UOM>\n",NULL);
            }
            const char **values = dimension->print_ogc_formatted_values(ctx,dimension);
            const char **value = values;
            while(*value) {
               dimensions = apr_pstrcat(ctx->pool,dimensions,"      <Value>",*value,"</Value>\n",NULL);
               value++;
            }
            dimensions = apr_pstrcat(ctx->pool,dimensions,"    </Dimension>\n",NULL);

            dimensionstemplate = apr_pstrcat(ctx->pool,dimensionstemplate,"{",dimension->name,"}/",NULL);
         }
      }
      char *tmsets = "";
      char *bboxes = "";
      
      if(tileset->wgs84bbox[0] != tileset->wgs84bbox[2]) {
         bboxes = apr_psprintf(ctx->pool,
               "    <ows:WGS84BoundingBox>\n"
               "      <ows:LowerCorner>%f %f</ows:LowerCorner>\n"
               "      <ows:UpperCorner>%f %f</ows:UpperCorner>\n"
               "    </ows:WGS84BoundingBox>\n",
               tileset->wgs84bbox[0], tileset->wgs84bbox[1],
               tileset->wgs84bbox[2], tileset->wgs84bbox[3]);
      }
      for(i=0;i<tileset->grid_links->nelts;i++) {
         char *matrixlimits = "";
         geocache_grid_link *grid_link = APR_ARRAY_IDX(tileset->grid_links,i,geocache_grid_link*);
         if(grid_link->restricted_extent) {
            int j;
            matrixlimits = "      <TileMatrixSetLimits>";
            for(j=0;j<grid_link->grid->nlevels;j++) {
               matrixlimits = apr_psprintf(ctx->pool,"%s\n"
                     "        <TileMatrixLimits>\n"
                     "          <TileMatrix>%s:%d</TileMatrix>\n"
                     "          <MinTileRow>%d</MinTileRow>\n"
                     "          <MaxTileRow>%d</MaxTileRow>\n"
                     "          <MinTileCol>%d</MinTileCol>\n"
                     "          <MaxTileCol>%d</MaxTileCol>\n"
                     "        </TileMatrixLimits>",
                     matrixlimits,
                     grid_link->grid->name,j,
                     grid_link->grid_limits[j][0],
                     grid_link->grid_limits[j][2]-1,
                     grid_link->grid_limits[j][1],
                     grid_link->grid_limits[j][3]-1);

            }
            matrixlimits = apr_pstrcat(ctx->pool,matrixlimits,"\n      </TileMatrixSetLimits>\n",NULL);
         }
         tmsets = apr_pstrcat(ctx->pool,tmsets,
               "    <TileMatrixSetLink>\n"
               "      <TileMatrixSet>",
               grid_link->grid->name,
               "</TileMatrixSet>\n",
               matrixlimits,
               "    </TileMatrixSetLink>\n",
               NULL);

         double *bbox = grid_link->restricted_extent?grid_link->restricted_extent:grid_link->grid->extent;
         bboxes = apr_psprintf(ctx->pool,"%s"
               "    <ows:BoundingBox>\n"
               "      <ows:CRS>%s</ows:CRS>\n"
               "      <ows:LowerCorner>%f %f</ows:LowerCorner>\n"
               "      <ows:UpperCorner>%f %f</ows:UpperCorner>\n"
               "    </ows:BoundingBox>\n",
               bboxes,
               geocache_grid_get_crs(ctx,grid_link->grid),
               bbox[0],bbox[1],
               bbox[2],bbox[3]);
      }
      caps = apr_psprintf(ctx->pool,"%s"
            "  <Layer>\n"
            "    <ows:Title>%s</ows:Title>\n"
            "    <ows:Abstract>%s</ows:Abstract>\n"
            "    <ows:Identifier>%s</ows:Identifier>\n"
            "    <Style isDefault=\"true\">\n"
            "      <ows:Identifier>default</ows:Identifier>\n"
            "    </Style>\n"
            "%s" /*dimensions*/
            "    <Format>%s</Format>\n"
            "%s" /*TileMatrixsetLinks*/
            "%s" /*BoundinBoxes*/
            "    <ResourceURL format=\"%s\" resourceType=\"tile\""
            " template=\"%s/wmts/1.0.0/%s/default/%s{TileMatrixSet}/{TileMatrix}/{TileRow}/{TileCol}.%s\"/>\n"
            "  </Layer>\n",caps,title,abstract,
            tileset->name,dimensions,tileset->format->mime_type,tmsets,bboxes,
            tileset->format->mime_type,onlineresource,
            tileset->name, dimensionstemplate,tileset->format->extension);
      layer_index = apr_hash_next(layer_index);
   }
   
   apr_hash_index_t *grid_index = apr_hash_first(ctx->pool,cfg->grids);

   while(grid_index) {
      geocache_grid *grid;
      char *epsgnum;
      const void *key; apr_ssize_t keylen;
      char *matrix;
      int level;
      const char *WellKnownScaleSet;
      apr_hash_this(grid_index,&key,&keylen,(void**)&grid);
      
      /*locate the number after epsg: in the grd srs*/
      epsgnum = strchr(grid->srs,':');
      if(!epsgnum) {
         epsgnum = grid->srs;
      } else {
         epsgnum++;
      }
      
      WellKnownScaleSet = apr_table_get(grid->metadata,"WellKnownScaleSet");
      
      caps = apr_psprintf(ctx->pool,"%s"
            "  <TileMatrixSet>\n"
            "    <ows:Identifier>%s</ows:Identifier>\n"
            "    <ows:SupportedCRS>%s</ows:SupportedCRS>\n",
            caps,grid->name,geocache_grid_get_crs(ctx,grid));
      
      if(WellKnownScaleSet) {
         caps = apr_psprintf(ctx->pool,"%s"
            "    <WellKnownScaleSet>%s</WellKnownScaleSet>\n",
            caps,WellKnownScaleSet);
      }
      
      for(level=0;level<grid->nlevels;level++) {
         double scaledenom = grid->levels[level]->resolution * geocache_meters_per_unit[grid->unit] / 0.00028;
         matrix = apr_psprintf(ctx->pool,wmts_matrix,
               level,
               scaledenom,
               grid->extent[0],grid->extent[3],
               grid->tile_sx, grid->tile_sy,
               grid->levels[level]->maxx, grid->levels[level]->maxy);
         caps = apr_psprintf(ctx->pool,"%s%s",caps,matrix);
      }
      caps = apr_pstrcat(ctx->pool,caps,"  </TileMatrixSet>\n",NULL);
      grid_index = apr_hash_next(grid_index);
   }
   
   caps = apr_pstrcat(ctx->pool,caps,"</Contents>\n</Capabilities>\n",NULL);
   request->request.capabilities = caps;
}

/**
 * \brief parse a WMTS request
 * \private \memberof geocache_service_wmts
 * \sa geocache_service::parse_request()
 */
void _geocache_service_wmts_parse_request(geocache_context *ctx, geocache_service *this, geocache_request **request,
      const char *pathinfo, apr_table_t *params, geocache_cfg *config) {
   const char *str, *service = NULL, *style = NULL, *version = NULL, *layer = NULL, *matrixset = NULL,
               *matrix = NULL, *tilecol = NULL, *tilerow = NULL, *format = NULL, *extension = NULL;
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
      } else if( ! strcasecmp(str,"gettile")) {
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
            /*this is the last element of the uri, and it will also contain the file extension*/
            
            /*split the string at the first '.'*/
            char *charptr = key;
            while(*charptr && *charptr != '.') charptr++;

            /*replace '.' with '\0' and advance*/
            *charptr++ = '\0';
            tilecol = key;
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
         ctx->set_error(ctx, 404, "received wms request with invalid TILEMATRIX %s", matrix);
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
         ctx->set_error(ctx, 404, "received wms request with invalid TILEROW %s",tilerow);
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
         ctx->set_error(ctx, 404, "received wms request with invalid TILECOL %s",tilecol);
         return;
      }
   }

   if(!format && !extension) {
      ctx->set_error(ctx, 404, "received wmts request with no format");
      return;
   } else {
      if(format && strcmp(format,tileset->format->mime_type)) {
         ctx->set_error(ctx, 404, "received wmts request with invalid format \"%s\" (expecting %s)",
               format,tileset->format->mime_type);
         return;
      }
      if(extension && strcmp(extension,tileset->format->extension)) {
         ctx->set_error(ctx, 404, "received wmts request with invalid extension \"%s\" (expecting %s)",
               extension,tileset->format->extension);
         return;
      }
   }


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
}

geocache_service* geocache_service_wmts_create(geocache_context *ctx) {
   geocache_service_wmts* service = (geocache_service_wmts*)apr_pcalloc(ctx->pool, sizeof(geocache_service_wmts));
   if(!service) {
      ctx->set_error(ctx, 500, "failed to allocate wtms service");
      return NULL;
   }
   service->service.url_prefix = apr_pstrdup(ctx->pool,"wmts");
   service->service.type = GEOCACHE_SERVICE_WMTS;
   service->service.parse_request = _geocache_service_wmts_parse_request;
   service->service.create_capabilities_response = _create_capabilities_wmts;
   return (geocache_service*)service;
}

/** @} */
/* vim: ai ts=3 sts=3 et sw=3
*/

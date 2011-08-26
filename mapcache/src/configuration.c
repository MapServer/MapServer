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
#include "ezxml.h"
#include <string.h>
#include <stdlib.h>
#include <apr_strings.h>
#include <apr_hash.h>
#include <apr_tables.h>
#include <apr_file_io.h>
#include <apr_file_info.h>

geocache_cfg* geocache_configuration_create(apr_pool_t *pool) {
   geocache_grid *grid;
   int i;
   double wgs84_resolutions[19]={
         1.40625000000000,
         0.703125000000000,
         0.351562500000000,
         0.175781250000000,
         8.78906250000000e-2,
         4.39453125000000e-2,
         2.19726562500000e-2,
         1.09863281250000e-2,
         5.49316406250000e-3,
         2.74658203125000e-3,
         1.37329101562500e-3,
         6.86645507812500e-4,
         3.43322753906250e-4,
         1.71661376953125e-4,
         8.58306884765625e-5,
         4.29153442382812e-5,
         2.14576721191406e-5,
         1.07288360595703e-5,
         5.36441802978516e-6};
   
   double google_resolutions[19] = {
         156543.0339280410,
         78271.51696402048,
         39135.75848201023,
         19567.87924100512,
         9783.939620502561,
         4891.969810251280,
         2445.984905125640,
         1222.992452562820,
         611.4962262814100,
         305.7481131407048,
         152.8740565703525,
         76.43702828517624,
         38.21851414258813,
         19.10925707129406,
         9.554628535647032,
         4.777314267823516,
         2.388657133911758,
         1.194328566955879,
         0.5971642834779395
   };
   
   
   
   
   double wgs84_extent[4]={-180,-90,180,90};
   double google_extent[4]={-20037508.3427892480,-20037508.3427892480,20037508.3427892480,20037508.3427892480};

   geocache_cfg *cfg = (geocache_cfg*)apr_pcalloc(pool, sizeof(geocache_cfg));
   cfg->caches = apr_hash_make(pool);
   cfg->sources = apr_hash_make(pool);
   cfg->tilesets = apr_hash_make(pool);
   cfg->grids = apr_hash_make(pool);
   cfg->image_formats = apr_hash_make(pool);
   cfg->metadata = apr_table_make(pool,3);

   geocache_configuration_add_image_format(cfg,
         geocache_imageio_create_png_format(pool,"PNG",GEOCACHE_COMPRESSION_FAST),
         "PNG");
   geocache_configuration_add_image_format(cfg,
         geocache_imageio_create_png_q_format(pool,"PNG",GEOCACHE_COMPRESSION_FAST,256),
         "PNG8");
   geocache_configuration_add_image_format(cfg,
         geocache_imageio_create_jpeg_format(pool,"JPEG",95),
         "JPEG");
   cfg->merge_format = geocache_configuration_get_image_format(cfg,"PNG");
   cfg->reporting = GEOCACHE_REPORT_MSG;

   grid = geocache_grid_create(pool);
   grid->name = apr_pstrdup(pool,"WGS84");
   apr_table_add(grid->metadata,"title","GoogleCRS84Quad");
   apr_table_add(grid->metadata,"wellKnownScaleSet","urn:ogc:def:wkss:OGC:1.0:GoogleCRS84Quad");
   apr_table_add(grid->metadata,"profile","global-geodetic");
   grid->srs = apr_pstrdup(pool,"EPSG:4326");
   grid->unit = GEOCACHE_UNIT_DEGREES;
   grid->tile_sx = grid->tile_sy = 256;
   grid->nlevels = 19;
   grid->extent[0] = wgs84_extent[0];
   grid->extent[1] = wgs84_extent[1];
   grid->extent[2] = wgs84_extent[2];
   grid->extent[3] = wgs84_extent[3];
   grid->levels = (geocache_grid_level**)apr_pcalloc(pool,
         grid->nlevels*sizeof(geocache_grid_level*));
   for(i=0; i<grid->nlevels; i++) {
      geocache_grid_level *level = (geocache_grid_level*)apr_pcalloc(pool,sizeof(geocache_grid_level));
      level->resolution = wgs84_resolutions[i];
      grid->levels[i] = level;
   }
   geocache_configuration_add_grid(cfg,grid,"WGS84");

   grid = geocache_grid_create(pool);
   grid->name = apr_pstrdup(pool,"GoogleMapsCompatible");
   grid->srs = apr_pstrdup(pool,"EPSG:3857");
   APR_ARRAY_PUSH(grid->srs_aliases,char*) = apr_pstrdup(pool,"EPSG:900913");
   apr_table_add(grid->metadata,"title","GoogleMapsCompatible");
   apr_table_add(grid->metadata,"profile","global-mercator");
   apr_table_add(grid->metadata,"wellKnownScaleSet","urn:ogc:def:wkss:OGC:1.0:GoogleMapsCompatible");
   grid->tile_sx = grid->tile_sy = 256;
   grid->nlevels = 19;
   grid->unit = GEOCACHE_UNIT_METERS;
   grid->extent[0] = google_extent[0];
   grid->extent[1] = google_extent[1];
   grid->extent[2] = google_extent[2];
   grid->extent[3] = google_extent[3];
   grid->levels = (geocache_grid_level**)apr_pcalloc(pool,
         grid->nlevels*sizeof(geocache_grid_level*));
   for(i=0; i<grid->nlevels; i++) {
      geocache_grid_level *level = (geocache_grid_level*)apr_pcalloc(pool,sizeof(geocache_grid_level));
      level->resolution = google_resolutions[i];
      grid->levels[i] = level;
   }
   geocache_configuration_add_grid(cfg,grid,"GoogleMapsCompatible");
   
   grid = geocache_grid_create(pool);
   grid->name = apr_pstrdup(pool,"g");
   grid->srs = apr_pstrdup(pool,"EPSG:900913");
   APR_ARRAY_PUSH(grid->srs_aliases,char*) = apr_pstrdup(pool,"EPSG:3857");
   apr_table_add(grid->metadata,"title","GoogleMapsCompatible");
   apr_table_add(grid->metadata,"profile","global-mercator");
   apr_table_add(grid->metadata,"wellKnownScaleSet","urn:ogc:def:wkss:OGC:1.0:GoogleMapsCompatible");
   grid->tile_sx = grid->tile_sy = 256;
   grid->nlevels = 19;
   grid->unit = GEOCACHE_UNIT_METERS;
   grid->extent[0] = google_extent[0];
   grid->extent[1] = google_extent[1];
   grid->extent[2] = google_extent[2];
   grid->extent[3] = google_extent[3];
   grid->levels = (geocache_grid_level**)apr_pcalloc(pool,
         grid->nlevels*sizeof(geocache_grid_level*));
   for(i=0; i<grid->nlevels; i++) {
      geocache_grid_level *level = (geocache_grid_level*)apr_pcalloc(pool,sizeof(geocache_grid_level));
      level->resolution = google_resolutions[i];
      grid->levels[i] = level;
   }
   geocache_configuration_add_grid(cfg,grid,"g");

   return cfg;
}

geocache_source *geocache_configuration_get_source(geocache_cfg *config, const char *key) {
   return (geocache_source*)apr_hash_get(config->sources, (void*)key, APR_HASH_KEY_STRING);
}

geocache_cache *geocache_configuration_get_cache(geocache_cfg *config, const char *key) {
   return (geocache_cache*)apr_hash_get(config->caches, (void*)key, APR_HASH_KEY_STRING);
}

geocache_grid *geocache_configuration_get_grid(geocache_cfg *config, const char *key) {
   return (geocache_grid*)apr_hash_get(config->grids, (void*)key, APR_HASH_KEY_STRING);
}

geocache_tileset *geocache_configuration_get_tileset(geocache_cfg *config, const char *key) {
   return (geocache_tileset*)apr_hash_get(config->tilesets, (void*)key, APR_HASH_KEY_STRING);
}

geocache_image_format *geocache_configuration_get_image_format(geocache_cfg *config, const char *key) {
   return (geocache_image_format*)apr_hash_get(config->image_formats, (void*)key, APR_HASH_KEY_STRING);
}

void geocache_configuration_add_source(geocache_cfg *config, geocache_source *source, const char * key) {
   apr_hash_set(config->sources, key, APR_HASH_KEY_STRING, (void*)source);
}

void geocache_configuration_add_grid(geocache_cfg *config, geocache_grid *grid, const char * key) {
   apr_hash_set(config->grids, key, APR_HASH_KEY_STRING, (void*)grid);
}

void geocache_configuration_add_tileset(geocache_cfg *config, geocache_tileset *tileset, const char * key) {
   tileset->config = config;
   apr_hash_set(config->tilesets, key, APR_HASH_KEY_STRING, (void*)tileset);
}

void geocache_configuration_add_cache(geocache_cfg *config, geocache_cache *cache, const char * key) {
   apr_hash_set(config->caches, key, APR_HASH_KEY_STRING, (void*)cache);
}

void geocache_configuration_add_image_format(geocache_cfg *config, geocache_image_format *format, const char * key) {
   apr_hash_set(config->image_formats, key, APR_HASH_KEY_STRING, (void*)format);
}

void parseMetadata(geocache_context *ctx, ezxml_t node, apr_table_t *metadata) {
   ezxml_t cur_node;
   for(cur_node = node->child; cur_node; cur_node = cur_node->sibling) {
      apr_table_add(metadata,cur_node->name, cur_node->txt);
   }
}

void parseDimensions(geocache_context *ctx, ezxml_t node, geocache_tileset *tileset) {
   ezxml_t dimension_node;
   apr_array_header_t *dimensions = apr_array_make(ctx->pool,1,sizeof(geocache_dimension*));
   for(dimension_node = ezxml_child(node,"dimension"); dimension_node; dimension_node = dimension_node->next) {
      char *name = (char*)ezxml_attr(dimension_node,"name");
      char *type = (char*)ezxml_attr(dimension_node,"type");
      char *unit = (char*)ezxml_attr(dimension_node,"unit");
      char *default_value = (char*)ezxml_attr(dimension_node,"default");
      
      geocache_dimension *dimension = NULL;
      
      if(!name || !strlen(name)) {
         ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in <dimension>");
         return;
      }

      if(type && *type) {
         if(!strcmp(type,"values")) {
            dimension = geocache_dimension_values_create(ctx->pool);
         } else if(!strcmp(type,"regex")) {
            dimension = geocache_dimension_regex_create(ctx->pool);
         } else if(!strcmp(type,"intervals")) {
            dimension = geocache_dimension_intervals_create(ctx->pool);
         } else if(!strcmp(type,"time")) {
            ctx->set_error(ctx,501,"time dimension type not implemented yet");
            return;
            dimension = geocache_dimension_time_create(ctx->pool);
         } else {
            ctx->set_error(ctx,400,"unknown dimension type \"%s\"",type);
            return;
         }
      } else {
         ctx->set_error(ctx,400, "mandatory attribute \"type\" not found in <dimensions>");
         return;
      }

      dimension->name = apr_pstrdup(ctx->pool,name);

      if(unit && *unit) {
         dimension->unit = apr_pstrdup(ctx->pool,unit);
      }

      if(default_value && *default_value) {
         dimension->default_value = apr_pstrdup(ctx->pool,default_value);
      } else {
         ctx->set_error(ctx,400,"dimension \"%s\" has no \"default\" attribute",dimension->name);
         return;
      }

      dimension->parse(ctx,dimension,dimension_node);
      GC_CHECK_ERROR(ctx);

      APR_ARRAY_PUSH(dimensions,geocache_dimension*) = dimension;
   }
   if(apr_is_empty_array(dimensions)) {
      ctx->set_error(ctx, 400, "<dimensions> for tileset \"%s\" has no dimensions defined (expecting <dimension> children)",tileset->name);
      return;
   }
   tileset->dimensions = dimensions;
}

void parseGrid(geocache_context *ctx, ezxml_t node, geocache_cfg *config) {
   char *name;
   double extent[4] = {0,0,0,0};
   geocache_grid *grid;
   ezxml_t cur_node;
   char *value;

   name = (char*)ezxml_attr(node,"name");
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in <grid>");
      return;
   }
   else {
      name = apr_pstrdup(ctx->pool, name);
      /* check we don't already have a grid defined with this name */
      if(geocache_configuration_get_grid(config, name)) {
         ctx->set_error(ctx, 400, "duplicate grid with name \"%s\"",name);
         return;
      }
   }
   grid = geocache_grid_create(ctx->pool);
   grid->name = name;

   if ((cur_node = ezxml_child(node,"extent")) != NULL) {
      double *values;
      int nvalues;
      value = apr_pstrdup(ctx->pool,cur_node->txt);
      if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(ctx, value, ' ', &values, &nvalues) ||
            nvalues != 4) {
         ctx->set_error(ctx, 400, "failed to parse extent array %s."
               "(expecting 4 space separated numbers, got %d (%f %f %f %f)"
               "eg <extent>-180 -90 180 90</extent>",
               value,nvalues,values[0],values[1],values[2],values[3]);
         return;
      }
      extent[0] = values[0];
      extent[1] = values[1];
      extent[2] = values[2];
      extent[3] = values[3];
   }

   if ((cur_node = ezxml_child(node,"metadata")) != NULL) {
      parseMetadata(ctx, cur_node, grid->metadata);
      GC_CHECK_ERROR(ctx);
   } 

   if ((cur_node = ezxml_child(node,"units")) != NULL) {
      if(!strcasecmp(cur_node->txt,"dd")) {
         grid->unit = GEOCACHE_UNIT_DEGREES;
      } else if(!strcasecmp(cur_node->txt,"m")) {
         grid->unit = GEOCACHE_UNIT_METERS;
      } else if(!strcasecmp(cur_node->txt,"ft")) {
         grid->unit = GEOCACHE_UNIT_FEET;
      } else {
         ctx->set_error(ctx, 400, "unknown unit %s for grid %s (valid values are \"dd\", \"m\", and \"ft\"",
               cur_node->txt, grid->name);
         return;
      }
   }
   if ((cur_node = ezxml_child(node,"srs")) != NULL) {
      grid->srs = apr_pstrdup(ctx->pool,cur_node->txt);
   }
   
   for(cur_node = ezxml_child(node,"srsalias"); cur_node; cur_node = cur_node->next) {
      value = apr_pstrdup(ctx->pool,cur_node->txt);
      APR_ARRAY_PUSH(grid->srs_aliases,char*) = value;
   }

   if ((cur_node = ezxml_child(node,"size")) != NULL) {
      value = apr_pstrdup(ctx->pool,cur_node->txt);
      int *sizes, nsizes;
      if(GEOCACHE_SUCCESS != geocache_util_extract_int_list(ctx, value, ' ', &sizes, &nsizes) ||
            nsizes != 2) {
         ctx->set_error(ctx, 400, "failed to parse size array %s in  grid %s"
               "(expecting two space separated integers, eg <size>256 256</size>",
               value, grid->name);
         return;
      }
      grid->tile_sx = sizes[0];
      grid->tile_sy = sizes[1];
   } 

   if ((cur_node = ezxml_child(node,"resolutions")) != NULL) {
      value = apr_pstrdup(ctx->pool,cur_node->txt);
      int nvalues;
      double *values;
      if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(ctx, value, ' ', &values, &nvalues) ||
            !nvalues) {
         ctx->set_error(ctx, 400, "failed to parse resolutions array %s."
               "(expecting space separated numbers, "
               "eg <resolutions>1 2 4 8 16 32</resolutions>",
               value);
         return;
      }
      grid->nlevels = nvalues;
      grid->levels = (geocache_grid_level**)apr_pcalloc(ctx->pool,
            grid->nlevels*sizeof(geocache_grid_level));
      while(nvalues--) {
         grid->levels[nvalues] = (geocache_grid_level*)apr_pcalloc(ctx->pool,
               sizeof(geocache_grid_level));
         grid->levels[nvalues]->resolution = values[nvalues];
      }
   }

   if(grid->srs == NULL) {
      ctx->set_error(ctx, 400, "grid \"%s\" has no srs configured."
            " You must add a <srs> tag.", grid->name);
      return;
   }
   if(extent[0] >= extent[2] || extent[1] >= extent[3]) {
      ctx->set_error(ctx, 400, "grid \"%s\" has no (or invalid) extent configured"
            " You must add/correct a <extent> tag.", grid->name);
      return;
   } else {
         grid->extent[0] = extent[0];
         grid->extent[1] = extent[1];
         grid->extent[2] = extent[2];
         grid->extent[3] = extent[3];
   }
   if(grid->tile_sx <= 0 || grid->tile_sy <= 0) {
      ctx->set_error(ctx, 400, "grid \"%s\" has no (or invalid) tile size configured"
            " You must add/correct a <size> tag.", grid->name);
      return;
   }
   if(!grid->nlevels) {
      ctx->set_error(ctx, 400, "grid \"%s\" has no resolutions configured."
            " You must add a <resolutions> tag.", grid->name);
      return;
   }
   geocache_configuration_add_grid(config,grid,name);
}

void parseSource(geocache_context *ctx, ezxml_t node, geocache_cfg *config) {
   ezxml_t cur_node;
   char *name = NULL, *type = NULL;


   name = (char*)ezxml_attr(node,"name");
   type = (char*)ezxml_attr(node,"type");
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in <source>");
      return;
   }
   else {
      name = apr_pstrdup(ctx->pool, name);
      /* check we don't already have a source defined with this name */
      if(geocache_configuration_get_source(config, name)) {
         ctx->set_error(ctx, 400, "duplicate source with name \"%s\"",name);
         return;
      }
   }
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"type\" not found in <source>");
      return;
   }
   geocache_source *source = NULL;
   if(!strcmp(type,"wms")) {
      source = geocache_source_wms_create(ctx);
   } else if(!strcmp(type,"gdal")) {
      source = geocache_source_gdal_create(ctx);
   } else {
      ctx->set_error(ctx, 400, "unknown source type %s for source \"%s\"", type, name);
      return;
   }
   if(source == NULL) {
      ctx->set_error(ctx, 400, "failed to parse source \"%s\"", name);
      return;
   }
   source->name = name;
   
   if ((cur_node = ezxml_child(node,"metadata")) != NULL) {
      parseMetadata(ctx, cur_node, source->metadata);
      GC_CHECK_ERROR(ctx);
   } 
   
   source->configuration_parse(ctx,node,source);
   GC_CHECK_ERROR(ctx);
   source->configuration_check(ctx,source);
   GC_CHECK_ERROR(ctx);
   geocache_configuration_add_source(config,source,name);
}

void parseFormat(geocache_context *ctx, ezxml_t node, geocache_cfg *config) {
   char *name = NULL,  *type = NULL;
   geocache_image_format *format = NULL;
   ezxml_t cur_node;
   name = (char*)ezxml_attr(node,"name");
   type = (char*)ezxml_attr(node,"type");
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in <format>");
      return;
   }
   name = apr_pstrdup(ctx->pool, name);
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"type\" not found in <format>");
      return;
   }
   if(!strcmp(type,"PNG")) {
      int colors = -1;
      geocache_compression_type compression = GEOCACHE_COMPRESSION_DEFAULT;
      if ((cur_node = ezxml_child(node,"compression")) != NULL) {
         if(!strcmp(cur_node->txt, "fast")) {
            compression = GEOCACHE_COMPRESSION_FAST;
         } else if(!strcmp(cur_node->txt, "best")) {
            compression = GEOCACHE_COMPRESSION_BEST;
         } else {
            ctx->set_error(ctx, 400, "unknown compression type %s for format \"%s\"", cur_node->txt, name);
            return;
         }
      }
      if ((cur_node = ezxml_child(node,"colors")) != NULL) {
         char *endptr;
         colors = (int)strtol(cur_node->txt,&endptr,10);
         if(*endptr != 0 || colors < 2 || colors > 256) {
            ctx->set_error(ctx, 400, "failed to parse colors \"%s\" for format \"%s\""
                  "(expecting an  integer between 2 and 256 "
                  "eg <colors>256</colors>",
                  cur_node->txt,name);
            return;
         }
      }

      if(colors == -1) {
         format = geocache_imageio_create_png_format(ctx->pool,
               name,compression);
      } else {
         format = geocache_imageio_create_png_q_format(ctx->pool,
               name,compression, colors);
      }
   } else if(!strcmp(type,"JPEG")){
      int quality = 95;
      if ((cur_node = ezxml_child(node,"quality")) != NULL) {
         char *endptr;
         quality = (int)strtol(cur_node->txt,&endptr,10);
         if(*endptr != 0 || quality < 1 || quality > 100) {
            ctx->set_error(ctx, 400, "failed to parse quality \"%s\" for format \"%s\""
                  "(expecting an  integer between 1 and 100 "
                  "eg <quality>90</quality>",
                  cur_node->txt,name);
            return;
         }
      }
      format = geocache_imageio_create_jpeg_format(ctx->pool,
            name,quality);
   } else if(!strcmp(type,"MIXED")){
      geocache_image_format *transparent=NULL, *opaque=NULL;
      if ((cur_node = ezxml_child(node,"transparent")) != NULL) {
         transparent = geocache_configuration_get_image_format(config,cur_node->txt);
      }
      if(!transparent) {
         ctx->set_error(ctx,400, "mixed format %s references unknown transparent format %s"
               "(order is important, format %s should appear first)",
               name,cur_node->txt,cur_node->txt);
      }
      if ((cur_node = ezxml_child(node,"opaque")) != NULL) {
         opaque = geocache_configuration_get_image_format(config,cur_node->txt);
      }
      if(!opaque) {
         ctx->set_error(ctx,400, "mixed format %s references unknown opaque format %s"
               "(order is important, format %s should appear first)",
               name,cur_node->txt,cur_node->txt);
      }
      format = geocache_imageio_create_mixed_format(ctx->pool,name,transparent, opaque);
   } else {
      ctx->set_error(ctx, 400, "unknown format type %s for format \"%s\"", type, name);
      return;
   }
   if(format == NULL) {
      ctx->set_error(ctx, 400, "failed to parse format \"%s\"", name);
      return;
   }


   geocache_configuration_add_image_format(config,format,name);
   return;
}

void parseCache(geocache_context *ctx, ezxml_t node, geocache_cfg *config) {
   char *name = NULL,  *type = NULL;
   geocache_cache *cache = NULL;
   name = (char*)ezxml_attr(node,"name");
   type = (char*)ezxml_attr(node,"type");
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in <cache>");
      return;
   }
   else {
      name = apr_pstrdup(ctx->pool, name);
      /* check we don't already have a cache defined with this name */
      if(geocache_configuration_get_cache(config, name)) {
         ctx->set_error(ctx, 400, "duplicate cache with name \"%s\"",name);
         return;
      }
   }
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"type\" not found in <cache>");
      return;
   }
   if(!strcmp(type,"disk")) {
      cache = geocache_cache_disk_create(ctx);
   } else if(!strcmp(type,"memcache")) {
#ifdef USE_MEMCACHE
      cache = geocache_cache_memcache_create(ctx);
#else
      ctx->set_error(ctx,400, "failed to add cache %s: memcache support is not available on this build",name);
      return;
#endif
   } else {
      ctx->set_error(ctx, 400, "unknown cache type %s for cache \"%s\"", type, name);
      return;
   }
   if(cache == NULL) {
      ctx->set_error(ctx, 400, "failed to parse cache \"%s\"", name);
      return;
   }
   cache->name = name;

   cache->configuration_parse(ctx,node,cache);
   GC_CHECK_ERROR(ctx);
   cache->configuration_check(ctx,cache);
   GC_CHECK_ERROR(ctx);
   geocache_configuration_add_cache(config,cache,name);
   return;
}



void parseTileset(geocache_context *ctx, ezxml_t node, geocache_cfg *config) {
   char *name = NULL;
   geocache_tileset *tileset = NULL;
   ezxml_t cur_node;
   char* value;
   int havewgs84bbox=0;
   name = (char*)ezxml_attr(node,"name");
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in <tileset>");
      return;
   }
   else {
      name = apr_pstrdup(ctx->pool, name);
      /* check we don't already have a cache defined with this name */
      if(geocache_configuration_get_tileset(config, name)) {
         ctx->set_error(ctx, 400, "duplicate tileset with name \"%s\"",name);
         return;
      }
   }
   tileset = geocache_tileset_create(ctx);
   tileset->name = name;
   
   if ((cur_node = ezxml_child(node,"metadata")) != NULL) {
      parseMetadata(ctx, cur_node, tileset->metadata);
      GC_CHECK_ERROR(ctx);
   }
   
   
   if ((value = (char*)apr_table_get(tileset->metadata,"wgs84boundingbox")) != NULL) {
      double *values;
      int nvalues;
      value = apr_pstrdup(ctx->pool,value);
      if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(ctx, value, ' ', &values, &nvalues) ||
            nvalues != 4) {
         ctx->set_error(ctx, 400, "failed to parse extent array %s."
               "(expecting 4 space separated numbers, got %d (%f %f %f %f)"
               "eg <wgs84bbox>-180 -90 180 90</wgs84bbox>",
               value,nvalues,values[0],values[1],values[2],values[3]);
         return;
      }
      tileset->wgs84bbox[0] = values[0];
      tileset->wgs84bbox[1] = values[1];
      tileset->wgs84bbox[2] = values[2];
      tileset->wgs84bbox[3] = values[3];
      havewgs84bbox = 1;
   }

   for(cur_node = ezxml_child(node,"grid"); cur_node; cur_node = cur_node->next) {
      int i;
      char *restrictedExtent = NULL;
      if (tileset->grid_links == NULL) {
         tileset->grid_links = apr_array_make(ctx->pool,1,sizeof(geocache_grid_link*));
      }
      geocache_grid *grid = geocache_configuration_get_grid(config, cur_node->txt);
      if(!grid) {
         ctx->set_error(ctx, 400, "tileset \"%s\" references grid \"%s\","
               " but it is not configured", name, cur_node->txt);
         return;
      }
      geocache_grid_link *gridlink = apr_pcalloc(ctx->pool,sizeof(geocache_grid_link));
      gridlink->grid = grid;
      gridlink->grid_limits = apr_pcalloc(ctx->pool,grid->nlevels*sizeof(int*));
      for(i=0;i<grid->nlevels;i++) {
         gridlink->grid_limits[i] = apr_pcalloc(ctx->pool,4*sizeof(int));
      }
      double *extent;
      restrictedExtent = (char*)ezxml_attr(cur_node,"restricted_extent");
      if(restrictedExtent) {
         int nvalues;
         restrictedExtent = apr_pstrdup(ctx->pool,restrictedExtent);
         if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(ctx, restrictedExtent, ' ', &gridlink->restricted_extent, &nvalues) ||
               nvalues != 4) {
            ctx->set_error(ctx, 400, "failed to parse extent array %s."
                  "(expecting 4 space separated numbers, "
                  "eg <grid restricted_extent=\"-180 -90 180 90\">foo</grid>",
                  restrictedExtent);
            return;
         }
         extent = gridlink->restricted_extent;
      } else {
         extent = grid->extent;
      }
      geocache_grid_compute_limits(grid,extent,gridlink->grid_limits);
      
      /* compute wgs84 bbox if it wasn't supplied already */
      if(!havewgs84bbox && !strcasecmp(grid->srs,"EPSG:4326")) {
         tileset->wgs84bbox[0] = extent[0];
         tileset->wgs84bbox[1] = extent[1];
         tileset->wgs84bbox[2] = extent[2];
         tileset->wgs84bbox[3] = extent[3];
      }
      APR_ARRAY_PUSH(tileset->grid_links,geocache_grid_link*) = gridlink;
   }

   if ((cur_node = ezxml_child(node,"dimensions")) != NULL) {
      parseDimensions(ctx, cur_node, tileset);
      GC_CHECK_ERROR(ctx);
   }


   if ((cur_node = ezxml_child(node,"cache")) != NULL) {
         geocache_cache *cache = geocache_configuration_get_cache(config, cur_node->txt);
         if(!cache) {
            ctx->set_error(ctx, 400, "tileset \"%s\" references cache \"%s\","
                  " but it is not configured", name, cur_node->txt);
            return;
         }
         tileset->cache = cache;
   }

   if ((cur_node = ezxml_child(node,"source")) != NULL) {
         geocache_source *source = geocache_configuration_get_source(config, cur_node->txt);
         if(!source) {
            ctx->set_error(ctx, 400, "tileset \"%s\" references source \"%s\","
                  " but it is not configured", name, cur_node->txt);
            return;
         }
         tileset->source = source;
   }

   if ((cur_node = ezxml_child(node,"metatile")) != NULL) {
      value = apr_pstrdup(ctx->pool,cur_node->txt);
         int *values, nvalues;
         if(GEOCACHE_SUCCESS != geocache_util_extract_int_list(ctx, cur_node->txt,' ', 
                  &values, &nvalues) ||
               nvalues != 2) {
            ctx->set_error(ctx, 400, "failed to parse metatile dimension %s."
                  "(expecting 2 space separated integers, "
                  "eg <metatile>5 5</metatile>",
                  cur_node->txt);
            return;
         }
         tileset->metasize_x = values[0];
         tileset->metasize_y = values[1];
   }
      

   if ((cur_node = ezxml_child(node,"watermark")) != NULL) {
         if(!*cur_node->txt) {
             ctx->set_error(ctx,400, "watermark config entry empty");
             return;
         }
         apr_pool_t *tmppool;
         apr_pool_create(&tmppool,ctx->pool);
         apr_file_t *f;
         apr_finfo_t finfo;
         int rv;
         if(apr_file_open(&f, cur_node->txt, APR_FOPEN_READ|APR_FOPEN_BUFFERED|APR_FOPEN_BINARY,
                APR_OS_DEFAULT, tmppool) != APR_SUCCESS) {
             ctx->set_error(ctx,500, "failed to open watermark image %s",cur_node->txt);
             return;
         }
         rv = apr_file_info_get(&finfo, APR_FINFO_SIZE, f);
         if(!finfo.size) {
            ctx->set_error(ctx, 500, "watermark %s has no data",cur_node->txt);
            return;
         }

         geocache_buffer *watermarkdata = geocache_buffer_create(finfo.size,tmppool);
         //manually add the data to our buffer
         apr_size_t size;
         apr_file_read(f,(void*)watermarkdata->buf,&size);
         watermarkdata->size = size;
         apr_file_close(f);
         if(size != finfo.size) {
            ctx->set_error(ctx, 500,  "failed to copy watermark image data, got %d of %d bytes",(int)size, (int)finfo.size);
            return;
         }
         tileset->watermark = geocache_imageio_decode(ctx,watermarkdata);
         GC_CHECK_ERROR(ctx);
         apr_pool_destroy(tmppool);
   }
      

   if ((cur_node = ezxml_child(node,"expires")) != NULL) {
         char *endptr;
         tileset->expires = (int)strtol(cur_node->txt,&endptr,10);
         if(*endptr != 0) {
            ctx->set_error(ctx, 400, "failed to parse expires %s."
                  "(expecting an  integer, "
                  "eg <expires>3600</expires>",
                  cur_node->txt);  
            return;
         }
   }

   if ((cur_node = ezxml_child(node,"metabuffer")) != NULL) {
         char *endptr;
         tileset->metabuffer = (int)strtol(cur_node->txt,&endptr,10);
         if(*endptr != 0) {
            ctx->set_error(ctx, 400, "failed to parse metabuffer %s."
                  "(expecting an  integer, "
                  "eg <metabuffer>1</metabuffer>",
                  cur_node->txt);
            return;
         }
   }
      
   if ((cur_node = ezxml_child(node,"format")) != NULL) {
         geocache_image_format *format = geocache_configuration_get_image_format(config,cur_node->txt);
         if(!format) {
            ctx->set_error(ctx, 400, "tileset \"%s\" references format \"%s\","
                  " but it is not configured",name,cur_node->txt);
            return;
         }
         tileset->format = format;
   }
   if(!tileset->format && tileset->source->type == GEOCACHE_SOURCE_GDAL) {
      ctx->set_error(ctx,400, "tileset \"%s\" references a gdal source. <format> tag is missing and mandatory in this case",
            tileset->name);
      return;
   }
   
   /* check we have all we want */
   if(tileset->cache == NULL) {
      /* TODO: we should allow tilesets with no caches */
      ctx->set_error(ctx, 400, "tileset \"%s\" has no cache configured."
            " You must add a <cache> tag.", tileset->name);
      return;
   }
   if(tileset->source == NULL) {
      ctx->set_error(ctx, 400, "tileset \"%s\" has no source configured."
            " You must add a <source> tag.", tileset->name);
      return;
   } 

   if(apr_is_empty_array(tileset->grid_links)) {
      ctx->set_error(ctx, 400, "tileset \"%s\" has no grids configured."
            " You must add a <grid> tag.", tileset->name);
      return;
   } else if(!havewgs84bbox) {
#ifdef USE_PROJ
      geocache_grid_link *sgrid = APR_ARRAY_IDX(tileset->grid_links,0,geocache_grid_link*);
      double *extent = sgrid->grid->extent;
      if(sgrid->restricted_extent) {
         extent = sgrid->restricted_extent;
      }
#endif
   }

   if(!tileset->format && (
         tileset->metasize_x != 1 ||
         tileset->metasize_y != 1 ||
         tileset->metabuffer != 0 ||
         tileset->watermark)) {
       if(tileset->watermark) {
          ctx->set_error(ctx,400,"tileset \"%s\" has no <format> configured, but it is needed for the watermark",
                   tileset->name);
          return;
       } else {
          ctx->set_error(ctx,400,"tileset \"%s\" has no <format> configured, but it is needed for metatiling",
                   tileset->name);
          return;
       }
   }

   geocache_configuration_add_tileset(config,tileset,name);
   return;
}

void parseServices(geocache_context *ctx, ezxml_t root, geocache_cfg *config) {
   ezxml_t node;
   if ((node = ezxml_child(root,"wms")) != NULL) {
      if(!node->txt || !*node->txt || strcmp(node->txt, "false")) {
         config->services[GEOCACHE_SERVICE_WMS] = geocache_service_wms_create(ctx);
      }
   }
   if ((node = ezxml_child(root,"wmts")) != NULL) {
      if(!node->txt || !*node->txt || strcmp(node->txt, "false")) {
         config->services[GEOCACHE_SERVICE_WMTS] = geocache_service_wmts_create(ctx);
      }
   }
   if ((node = ezxml_child(root,"ve")) != NULL) {
      if(!node->txt || !*node->txt || strcmp(node->txt, "false")) {
         config->services[GEOCACHE_SERVICE_VE] = geocache_service_ve_create(ctx);
      }
   }
   if ((node = ezxml_child(root,"tms")) != NULL) {
      if(!node->txt || !*node->txt || strcmp(node->txt, "false")) {
         config->services[GEOCACHE_SERVICE_TMS] = geocache_service_tms_create(ctx);
      }
   }
   if ((node = ezxml_child(root,"kml")) != NULL) {
      if(!node->txt || !*node->txt || strcmp(node->txt, "false")) {
         if(!config->services[GEOCACHE_SERVICE_TMS]) {
            ctx->set_error(ctx,400,"kml service requires the tms service to be active");
            return;
         }
         config->services[GEOCACHE_SERVICE_KML] = geocache_service_kml_create(ctx);
      }
   }

   if ((node = ezxml_child(root,"gmaps")) != NULL) {
      if(!node->txt || !*node->txt || strcmp(node->txt, "false")) {
         config->services[GEOCACHE_SERVICE_GMAPS] = geocache_service_gmaps_create(ctx);
      }
   }
   if ((node = ezxml_child(root,"demo")) != NULL) {
      if(!node->txt || !*node->txt || strcmp(node->txt, "false")) {
         config->services[GEOCACHE_SERVICE_DEMO] = geocache_service_demo_create(ctx);
         if(!config->services[GEOCACHE_SERVICE_WMS])
            config->services[GEOCACHE_SERVICE_WMS] = geocache_service_wms_create(ctx);
      }
   }

   if(!config->services[GEOCACHE_SERVICE_WMS] &&
         !config->services[GEOCACHE_SERVICE_TMS] &&
         !config->services[GEOCACHE_SERVICE_WMTS]) {
      ctx->set_error(ctx, 400, "no services configured."
            " You must add a <services> tag with <wmts/> <wms/> or <tms/> children");
      return;
   }
}

static void createEmptyImages(geocache_context *ctx, geocache_cfg *cfg) {
   unsigned int color=0;
   if(!strstr(cfg->merge_format->mime_type,"png")) {
      color = 0xffffffff;
   }
   cfg->empty_image = cfg->merge_format->create_empty_image(ctx, cfg->merge_format,
         256,256, color);
   GC_CHECK_ERROR(ctx);
}

void geocache_configuration_parse(geocache_context *ctx, const char *filename, geocache_cfg *config) {
   ezxml_t doc, node;
   doc = ezxml_parse_file(filename);
   if (doc == NULL) {
      ctx->set_error(ctx,400, "failed to parse file %s. Is it valid XML?", filename);
      goto cleanup;
   } else {
      const char *err = ezxml_error(doc);
      if(err && *err) {
         ctx->set_error(ctx,400, "failed to parse file %s: %s", filename, err);
         goto cleanup;
      }
   }

   if(strcmp(doc->name,"geocache")) {
      ctx->set_error(ctx,400, "failed to parse file %s. first node is not <geocache>", filename);
      goto cleanup;
   }

   for(node = ezxml_child(doc,"metadata"); node; node = node->next) {
      parseMetadata(ctx, node, config->metadata);
      if(GC_HAS_ERROR(ctx)) goto cleanup;
   }

   for(node = ezxml_child(doc,"source"); node; node = node->next) {
      parseSource(ctx, node, config);
      if(GC_HAS_ERROR(ctx)) goto cleanup;
   }

   for(node = ezxml_child(doc,"grid"); node; node = node->next) {
      parseGrid(ctx, node, config);
      if(GC_HAS_ERROR(ctx)) goto cleanup;
   }

   for(node = ezxml_child(doc,"format"); node; node = node->next) {
      parseFormat(ctx, node, config);
      if(GC_HAS_ERROR(ctx)) goto cleanup;
   }

   for(node = ezxml_child(doc,"cache"); node; node = node->next) {
      parseCache(ctx, node, config);
      if(GC_HAS_ERROR(ctx)) goto cleanup;
   }

   for(node = ezxml_child(doc,"tileset"); node; node = node->next) {
      parseTileset(ctx, node, config);
      if(GC_HAS_ERROR(ctx)) goto cleanup;
   }

   if ((node = ezxml_child(doc,"service")) != NULL) {
      ezxml_t service_node;
      for(service_node = node; service_node; service_node = service_node->next) {
         char *enabled = (char*)ezxml_attr(service_node,"enabled");
         char *type = (char*)ezxml_attr(service_node,"type");
         if(!strcasecmp(enabled,"true")) {
            if (!strcasecmp(type,"wms")) {
               geocache_service *new_service = geocache_service_wms_create(ctx);
               if(new_service->configuration_parse) {
                  new_service->configuration_parse(ctx,service_node,new_service);
               }
               config->services[GEOCACHE_SERVICE_WMS] = new_service;
            }
            else if (!strcasecmp(type,"tms")) {
               geocache_service *new_service = geocache_service_tms_create(ctx);
               if(new_service->configuration_parse) {
                  new_service->configuration_parse(ctx,service_node,new_service);
               }
               config->services[GEOCACHE_SERVICE_TMS] = new_service;
            }
            else if (!strcasecmp(type,"wmts")) {
               geocache_service *new_service = geocache_service_wmts_create(ctx);
               if(new_service->configuration_parse) {
                  new_service->configuration_parse(ctx,service_node,new_service);
               }
               config->services[GEOCACHE_SERVICE_WMTS] = new_service;
            }
            else if (!strcasecmp(type,"kml")) {
               geocache_service *new_service = geocache_service_kml_create(ctx);
               if(new_service->configuration_parse) {
                  new_service->configuration_parse(ctx,service_node,new_service);
               }
               config->services[GEOCACHE_SERVICE_KML] = new_service;
            }
            else if (!strcasecmp(type,"gmaps")) {
               geocache_service *new_service = geocache_service_gmaps_create(ctx);
               if(new_service->configuration_parse) {
                  new_service->configuration_parse(ctx,service_node,new_service);
               }
               config->services[GEOCACHE_SERVICE_GMAPS] = new_service;
            }
            else if (!strcasecmp(type,"ve")) {
               geocache_service *new_service = geocache_service_ve_create(ctx);
               if(new_service->configuration_parse) {
                  new_service->configuration_parse(ctx,service_node,new_service);
               }
               config->services[GEOCACHE_SERVICE_VE] = new_service;
            }
            else if (!strcasecmp(type,"demo")) {
               geocache_service *new_service = geocache_service_demo_create(ctx);
               if(new_service->configuration_parse) {
                  new_service->configuration_parse(ctx,service_node,new_service);
               }
               config->services[GEOCACHE_SERVICE_DEMO] = new_service;
            } else {
               ctx->set_error(ctx,400,"unknown <service> type %s",type);
            }
            if(GC_HAS_ERROR(ctx)) goto cleanup;
         }
      }
   }
   else if ((node = ezxml_child(doc,"services")) != NULL) {
      ctx->log(ctx,GEOCACHE_WARNING,"<services> tag is deprecated, use <service type=\"wms\" enabled=\"true|false\">");
      parseServices(ctx, node, config);
   } else {
      ctx->set_error(ctx, 400, "no <services> configured");
   }
   if(GC_HAS_ERROR(ctx)) goto cleanup;


   if ((node = ezxml_child(doc,"merge_format")) != NULL) {
      geocache_image_format *format = geocache_configuration_get_image_format(config,node->txt);
      if(!format) {
         ctx->set_error(ctx, 400, "merge_format tag references format %s but it is not configured",
               node->txt);
         goto cleanup;
      }
      config->merge_format = format;
   }

   if ((node = ezxml_child(doc,"errors")) != NULL) {
      if(!strcmp(node->txt,"log")) {
         config->reporting = GEOCACHE_REPORT_LOG;
      } else if(!strcmp(node->txt,"report")) {
         config->reporting = GEOCACHE_REPORT_MSG;
      } else if(!strcmp(node->txt,"empty_img")) {
         config->reporting = GEOCACHE_REPORT_EMPTY_IMG;
         createEmptyImages(ctx, config);
         if(GC_HAS_ERROR(ctx)) goto cleanup;
      } else if(!strcmp(node->txt, "report_img")) {
         config->reporting = GEOCACHE_REPORT_ERROR_IMG;
         ctx->set_error(ctx,501,"<errors>: report_img not implemented");
         goto cleanup;
      } else {
         ctx->set_error(ctx,400,"<errors>: unknown value %s (allowed are log, report, empty_img, report_img)",
               node->txt);
         goto cleanup;
      }
   }

   config->getmap_strategy = GEOCACHE_GETMAP_ERROR;
   if ((node = ezxml_child(doc,"full_wms")) != NULL) {
      if(!strcmp(node->txt,"assemble")) {
#ifdef USE_CAIRO
         config->getmap_strategy = GEOCACHE_GETMAP_ASSEMBLE;
#else
         ctx->set_error(ctx,400, "\"assemble\" <full_wms> not supported with current build. reconfigure with --enable-cairo");
#endif
      } else if(!strcmp(node->txt,"forward")) {
         config->getmap_strategy = GEOCACHE_GETMAP_FORWARD;
      } else if(*node->txt && strcmp(node->txt,"error")) {
         ctx->set_error(ctx,400, "unknown value %s for node <full_wms> (allowed values: assemble, getmap or error", node->txt);
         goto cleanup;
      }
   }

   if((node = ezxml_child(doc,"lock_dir")) != NULL) {
      config->lockdir = apr_pstrdup(ctx->pool, node->txt);
   } else {
      config->lockdir = apr_pstrdup(ctx->pool,"/tmp");
   }
   apr_dir_t *lockdir;
   apr_status_t rv;
   rv = apr_dir_open(&lockdir,config->lockdir,ctx->pool);
   char errmsg[120];
   if(rv != APR_SUCCESS) {
      ctx->set_error(ctx,500, "failed to open lock directory %s: %s"
            ,config->lockdir,apr_strerror(rv,errmsg,120));
      goto cleanup;
   }
   apr_finfo_t finfo;
   while ((apr_dir_read(&finfo, APR_FINFO_DIRENT|APR_FINFO_TYPE|APR_FINFO_NAME, lockdir)) == APR_SUCCESS) {
      if(finfo.filetype == APR_REG) {
         if(!strncmp(finfo.name, GEOCACHE_LOCKFILE_PREFIX, strlen(GEOCACHE_LOCKFILE_PREFIX))) {
            ctx->log(ctx,GEOCACHE_WARNING,"found old lockfile %s/%s, deleting it",config->lockdir,
                  finfo.name);
            rv = apr_file_remove(apr_psprintf(ctx->pool,"%s/%s",config->lockdir, finfo.name),ctx->pool);
            if(rv != APR_SUCCESS) {
               ctx->set_error(ctx,500, "failed to remove lockfile %s: %s",finfo.name,apr_strerror(rv,errmsg,120));
               goto cleanup;
            }

         }
      
      }
   }
   apr_dir_close(lockdir);


cleanup:
   ezxml_free(doc);
   return;
}
/* vim: ai ts=3 sts=3 et sw=3
*/

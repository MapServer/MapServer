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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <stdlib.h>
#include <apr_strings.h>
#include <apr_hash.h>
#include <apr_file_io.h>

geocache_cfg* geocache_configuration_create(apr_pool_t *pool) {
   geocache_grid *grid;
   int i;
   double wgs84_resolutions[16]={
         1.40625,
         0.703125,
         0.3515625,
         0.17578125,
         0.087890625,
         0.0439453125,
         0.02197265625,
         0.010986328125,
         0.0054931640625,
         0.00274658203125,
         0.001373291015625,
         0.0006866455078125,
         0.00034332275390625,
         0.000171661376953125,
         0.0000858306884765625,
         0.00004291534423828125};
   double google_resolutions[19] = {
         156543.0339,
         78271.51695,
         39135.758475,
         19567.8792375,
         9783.93961875,
         4891.96980938,
         2445.98490469,
         1222.99245234,
         611.496226172,
         305.748113086,
         152.874056543,
         76.4370282715,
         38.2185141357,
         19.1092570679,
         9.55462853394,
         4.77731426697,
         2.38865713348,
         1.19432856674,
         0.597164283371
   };
   double wgs84_extent[4]={-180,-90,180,90};
   double google_extent[4]={-20037508.3392,-20037508.3392,20037508.3392,20037508.3392};

   geocache_cfg *cfg = (geocache_cfg*)apr_pcalloc(pool, sizeof(geocache_cfg));
   cfg->caches = apr_hash_make(pool);
   cfg->sources = apr_hash_make(pool);
   cfg->tilesets = apr_hash_make(pool);
   cfg->grids = apr_hash_make(pool);
   cfg->image_formats = apr_hash_make(pool);

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
   cfg->lockdir = "/tmp/geocache_locks";
   cfg->reporting = GEOCACHE_REPORT_MSG;

   grid = geocache_grid_create(pool);
   grid->name = "WGS84";
   grid->srs = "epsg:4326";
   grid->tile_sx = grid->tile_sy = 256;
   grid->resolutions = wgs84_resolutions;
   grid->levels = 16;
   grid->extents = (double**)apr_pcalloc(pool,grid->levels*sizeof(double*));
   grid->resolutions = (double*)apr_pcalloc(pool,grid->levels*sizeof(double));
   for(i=0; i<grid->levels; i++) {
      grid->extents[i] = (double*)apr_pcalloc(pool,4*sizeof(double));
      grid->extents[i][0] = wgs84_extent[0];
      grid->extents[i][1] = wgs84_extent[1];
      grid->extents[i][2] = wgs84_extent[2];
      grid->extents[i][3] = wgs84_extent[3];
      grid->resolutions[i] = wgs84_resolutions[i];
   }
   geocache_configuration_add_grid(cfg,grid,"WGS84");

   grid = geocache_grid_create(pool);
   grid->name = "google";
   grid->srs = "epsg:900913";
   grid->tile_sx = grid->tile_sy = 256;
   grid->resolutions = google_resolutions;
   grid->levels = 19;
   grid->extents = (double**)apr_pcalloc(pool,grid->levels*sizeof(double*));
   grid->resolutions = (double*)apr_pcalloc(pool,grid->levels*sizeof(double));
   for(i=0; i<grid->levels; i++) {
      grid->extents[i] = (double*)apr_pcalloc(pool,4*sizeof(double));
      grid->extents[i][0] = google_extent[0];
      grid->extents[i][1] = google_extent[1];
      grid->extents[i][2] = google_extent[2];
      grid->extents[i][3] = google_extent[3];
      grid->resolutions[i] = google_resolutions[i];
   }
   geocache_configuration_add_grid(cfg,grid,"google");


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

int extractNameAndTypeAttributes(xmlDoc *doc, xmlAttr *attribute, char **name, char **type) {
   *name = *type = NULL;
   while(attribute && attribute->name && attribute->children)
   {
      if(!xmlStrcmp(attribute->name, BAD_CAST "name")) {
         *name = (char*)xmlNodeListGetString(doc, attribute->children, 1);
      }
      if(!xmlStrcmp(attribute->name, BAD_CAST "type")) {
         *type = (char*)xmlNodeListGetString(doc, attribute->children, 1);
      }
      attribute = attribute->next;
   }
   if(*name && *type) {
      return GEOCACHE_SUCCESS;
   } else {
      return GEOCACHE_FAILURE;
   }
}

void parseGrid(geocache_context *ctx, xmlNode *node, geocache_cfg *config) {
   char *name = NULL, *type = NULL;
   double extent[4] = {0,0,0,0};
   geocache_grid *grid;
   xmlNode *cur_node;
   char *value;
   if(xmlStrcmp(node->name, BAD_CAST "grid")) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "SEVERE: <%s> is not a grid tag",node->name);
      return;
   }
   extractNameAndTypeAttributes(node->doc, node->properties, &name, &type);
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "mandatory attribute \"name\" not found in <grid>");
      return;
   }
   else {
      /* check we don't already have a grid defined with this name */
      if(geocache_configuration_get_grid(config, name)) {
         ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "duplicate grid with name \"%s\"",name);
         return;
      }
   }
   grid = geocache_grid_create(ctx->pool);
   grid->name = name;
   for(cur_node = node->children; cur_node; cur_node = cur_node->next) {
      if(cur_node->type != XML_ELEMENT_NODE) continue;
      if(!xmlStrcmp(cur_node->name, BAD_CAST "extent")) {
         value = (char*)xmlNodeGetContent(cur_node);
         int nvalues;
         double *values;
         if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(ctx, value, ' ', &values, &nvalues) ||
               nvalues != 4) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse extent array %s."
                  "(expecting 4 space separated numbers, got %d (%f %f %f %f)"
                  "eg <extent>-180 -90 180 90</extent>",
                  value,nvalues,values[0],values[1],values[2],values[3]);
            return;
         }
         extent[0] = values[0];
         extent[1] = values[1];
         extent[2] = values[2];
         extent[3] = values[3];
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "srs")) {
         value = (char*)xmlNodeGetContent(cur_node);
         grid->srs = value;
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "size")) {
         value = (char*)xmlNodeGetContent(cur_node);
         int *sizes, nsizes;
         if(GEOCACHE_SUCCESS != geocache_util_extract_int_list(ctx, value, ' ', &sizes, &nsizes) ||
               nsizes != 2) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse size array %s in  grid %s"
                  "(expecting two space separated integers, eg <size>256 256</size>",
                  value, grid->name);
            return;
         }
         grid->tile_sx = sizes[0];
         grid->tile_sy = sizes[1];
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "resolutions")) {
         value = (char*)xmlNodeGetContent(cur_node);
         int nvalues;
         double *values;
         if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(ctx, value, ' ', &values, &nvalues) ||
               !nvalues) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse resolutions array %s."
                  "(expecting space separated numbers, "
                  "eg <resolutions>1 2 4 8 16 32</resolutions>",
                  value);
            return;
         }
         grid->resolutions = values;
         grid->levels = nvalues;
         xmlFree(value);
      }
   }

   if(grid->srs == NULL) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "grid \"%s\" has no srs configured."
            " You must add a <srs> tag.", grid->name);
      return;
   }
   if(extent[0] >= extent[2] || extent[1] >= extent[3]) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "grid \"%s\" has no (or invalid) extent configured"
            " You must add/correct a <extent> tag.", grid->name);
      return;
   }
   if(grid->tile_sx <= 0 || grid->tile_sy <= 0) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "grid \"%s\" has no (or invalid) tile size configured"
            " You must add/correct a <size> tag.", grid->name);
      return;
   }
   if(!grid->levels) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "grid \"%s\" has no resolutions configured."
            " You must add a <resolutions> tag.", grid->name);
      return;
   } else {
      int i;
      grid->extents = apr_pcalloc(ctx->pool, grid->levels * sizeof(double*));
      for(i=0;i<grid->levels;i++) {
         grid->extents[i] = apr_pcalloc(ctx->pool, 4 * sizeof(double*));
         grid->extents[i][0] = extent[0];
         grid->extents[i][1] = extent[1];
         grid->extents[i][2] = extent[2];
         grid->extents[i][3] = extent[3];
      }
   }
   geocache_configuration_add_grid(config,grid,name);
}

void parseSource(geocache_context *ctx, xmlNode *node, geocache_cfg *config) {
   if(xmlStrcmp(node->name, BAD_CAST "source")) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "SEVERE: found tag %s instead of <source>",node->name);
      return;
   }
   xmlNode *cur_node;
   char *name = NULL, *type = NULL;

   extractNameAndTypeAttributes(node->doc, node->properties, &name, &type);

   if(!name || !strlen(name)) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "mandatory attribute \"name\" not found in <source>");
      return;
   }
   else {
      /* check we don't already have a source defined with this name */
      if(geocache_configuration_get_source(config, name)) {
         ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "duplicate source with name \"%s\"",name);
         return;
      }
   }
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "mandatory attribute \"type\" not found in <source>");
      return;
   }
   geocache_source *source = NULL;
   if(!strcmp(type,"wms")) {
      source = geocache_source_wms_create(ctx);
   } else if(!strcmp(type,"gdal")) {
      source = geocache_source_gdal_create(ctx);
   } else {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "unknown source type %s for source \"%s\"", type, name);
      return;
   }
   if(source == NULL) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse source \"%s\"", name);
      return;
   }
   source->name = name;
   for(cur_node = node->children; cur_node; cur_node = cur_node->next) {
      if(cur_node->type != XML_ELEMENT_NODE) continue;
      if(!xmlStrcmp(cur_node->name, BAD_CAST "srs")) {
         char* value = (char*)xmlNodeGetContent(cur_node);
         source->srs = value;
      }
   }

   source->configuration_parse(ctx,node,source);
   GC_CHECK_ERROR(ctx);
   source->configuration_check(ctx,source);
   GC_CHECK_ERROR(ctx);
   geocache_configuration_add_source(config,source,name);
}

void parseFormat(geocache_context *ctx, xmlNode *node, geocache_cfg *config) {
   char *name = NULL,  *type = NULL;
   xmlChar *value = NULL;
   geocache_image_format *format = NULL;
   xmlNode *cur_node;
   if(xmlStrcmp(node->name, BAD_CAST "format")) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "SEVERE: <%s> is not a format tag",node->name);
   }
   extractNameAndTypeAttributes(node->doc, node->properties, &name, &type);
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "mandatory attribute \"name\" not found in <format>");
      return;
   }
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "mandatory attribute \"type\" not found in <format>");
      return;
   }
   if(!strcmp(type,"PNG")) {
      int colors = -1;
      geocache_compression_type compression = GEOCACHE_COMPRESSION_DEFAULT;
      for(cur_node = node->children; cur_node; cur_node = cur_node->next) {
         if(cur_node->type != XML_ELEMENT_NODE) continue;
         if(!xmlStrcmp(cur_node->name, BAD_CAST "compression")) {
            value = xmlNodeGetContent(cur_node);
            if(!xmlStrcmp(value, BAD_CAST "fast")) {
               compression = GEOCACHE_COMPRESSION_FAST;
            } else if(!xmlStrcmp(value, BAD_CAST "best")) {
               compression = GEOCACHE_COMPRESSION_BEST;
            } else {
               ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "unknown compression type %s for format \"%s\"", value, name);
               return;
            }
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "colors")) {
            value = xmlNodeGetContent(cur_node);
            char *endptr;
            colors = (int)strtol((char*)value,&endptr,10);
            if(*endptr != 0 || colors < 2 || colors > 256) {
               ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse colors \"%s\" for format \"%s\""
                     "(expecting an  integer between 2 and 256 "
                     "eg <colors>256</colors>",
                     value,name);
               return;
            }
            xmlFree(value);
         } else {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "unknown tag %s for format \"%s\"", cur_node->name, name);
            return;
         }
      }
      if(colors == -1) {
         format = geocache_imageio_create_png_format(ctx->pool,name,compression);
      } else {
         format = geocache_imageio_create_png_q_format(ctx->pool,name,compression, colors);
      }
   } else if(!strcmp(type,"JPEG")){
      int quality = 95;
      for(cur_node = node->children; cur_node; cur_node = cur_node->next) {
         if(cur_node->type != XML_ELEMENT_NODE) continue;
         if(!xmlStrcmp(cur_node->name, BAD_CAST "quality")) {
            value = xmlNodeGetContent(cur_node);
            char *endptr;
            quality = (int)strtol((char*)value,&endptr,10);
            if(*endptr != 0 || quality < 1 || quality > 100) {
               ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse quality \"%s\" for format \"%s\""
                     "(expecting an  integer between 1 and 100 "
                     "eg <quality>90</quality>",
                     value,name);
               return;
            }
            xmlFree(value);
         }
      }
      format = geocache_imageio_create_jpeg_format(ctx->pool,name,quality);
   } else {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "unknown format type %s for format \"%s\"", type, name);
      return;
   }
   if(format == NULL) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse format \"%s\"", name);
      return;
   }


   geocache_configuration_add_image_format(config,format,name);
   return;
}

void parseCache(geocache_context *ctx, xmlNode *node, geocache_cfg *config) {
   char *name = NULL,  *type = NULL;
   geocache_cache *cache = NULL;
   if(xmlStrcmp(node->name, BAD_CAST "cache")) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "SEVERE: <%s> is not a cache tag",node->name);
      return;
   }
   extractNameAndTypeAttributes(node->doc, node->properties, &name, &type);
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "mandatory attribute \"name\" not found in <cache>");
      return;
   }
   else {
      /* check we don't already have a cache defined with this name */
      if(geocache_configuration_get_cache(config, name)) {
         ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "duplicate cache with name \"%s\"",name);
         return;
      }
   }
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "mandatory attribute \"type\" not found in <cache>");
      return;
   }
   if(!strcmp(type,"disk")) {
      cache = geocache_cache_disk_create(ctx);
   } else {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "unknown cache type %s for cache \"%s\"", type, name);
      return;
   }
   if(cache == NULL) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse cache \"%s\"", name);
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



void parseTileset(geocache_context *ctx, xmlNode *node, geocache_cfg *config) {
   char *name = NULL, *type = NULL;
   geocache_tileset *tileset = NULL;
   xmlNode *cur_node;
   char* value;
   if(xmlStrcmp(node->name, BAD_CAST "tileset")) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "SEVERE: <%s> is not a tileset tag",node->name);
      return;
   }
   extractNameAndTypeAttributes(node->doc, node->properties, &name, &type);
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "mandatory attribute \"name\" not found in <tileset>");
      return;
   }
   else {
      /* check we don't already have a cache defined with this name */
      if(geocache_configuration_get_tileset(config, name)) {
         ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "duplicate tileset with name \"%s\"",name);
         return;
      }
   }
   tileset = geocache_tileset_create(ctx);
   tileset->name = name;

   for(cur_node = node->children; cur_node; cur_node = cur_node->next) {
      if(cur_node->type != XML_ELEMENT_NODE) continue;
      if(!xmlStrcmp(cur_node->name, BAD_CAST "grid")) {
         value = (char*)xmlNodeGetContent(cur_node);
         geocache_grid *grid = geocache_configuration_get_grid(config, value);
         if(!grid) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "tileset \"%s\" references grid \"%s\","
                  " but it is not configured", name, value);
            return;
         }
         tileset->grid = grid;
         xmlFree(BAD_CAST value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "cache")) {
         value = (char*)xmlNodeGetContent(cur_node);
         geocache_cache *cache = geocache_configuration_get_cache(config, value);
         if(!cache) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "tileset \"%s\" references cache \"%s\","
                  " but it is not configured", name, value);
            return;
         }
         tileset->cache = cache;
         xmlFree(BAD_CAST value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "source")) {
         value = (char*)xmlNodeGetContent(cur_node);
         geocache_source *source = geocache_configuration_get_source(config, value);
         if(!source) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "tileset \"%s\" references source \"%s\","
                  " but it is not configured", name, value);
            return;
         }
         tileset->source = source;
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "metatile")) {
         value = (char*)xmlNodeGetContent(cur_node);
         int *values, nvalues;
         if(GEOCACHE_SUCCESS != geocache_util_extract_int_list(ctx, value,' ', &values, &nvalues) ||
               nvalues != 2) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse metatile dimension %s."
                  "(expecting 2 space separated integers, "
                  "eg <metatile>5 5</metatile>",
                  value);
            return;
         }
         tileset->metasize_x = values[0];
         tileset->metasize_y = values[1];
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "expires")) {
         value = (char*)xmlNodeGetContent(cur_node);
         char *endptr;
         tileset->expires = (int)strtol(value,&endptr,10);
         if(*endptr != 0) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse expires %s."
                  "(expecting an  integer, "
                  "eg <expires>3600</expires>",
                  value);  
            return;
         }
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "metabuffer")) {
         value = (char*)xmlNodeGetContent(cur_node);
         char *endptr;
         tileset->metabuffer = (int)strtol(value,&endptr,10);
         if(*endptr != 0) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse metabuffer %s."
                  "(expecting an  integer, "
                  "eg <metabuffer>1</metabuffer>",
                  value);
            return;
         }
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "format")) {
         value = (char*)xmlNodeGetContent(cur_node);
         geocache_image_format *format = geocache_configuration_get_image_format(config,value);
         if(!format) {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "tileset \"%s\" references format \"%s\","
                  " but it is not configured",name,value);
            return;
         }
         tileset->format = format;
         xmlFree(value);
      } else {
         ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "tileset \"%s\" contains unknown tag \"%s\","
               " but it is not configured",name,cur_node->name);
         return;
      }
   }
   /* check we have all we want */
   if(tileset->cache == NULL) {
      /* TODO: we should allow tilesets with no caches */
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "tileset \"%s\" has no cache configured."
            " You must add a <cache> tag.", tileset->name);
      return;
   }
   if(tileset->source == NULL) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "tileset \"%s\" has no source configured."
            " You must add a <source> tag.", tileset->name);
      return;
   }

   if(tileset->grid == NULL) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "tileset \"%s\" has no grid configured."
            " You must add a <grid> tag.", tileset->name);
      return;
   }

   if(!tileset->format && (
         tileset->metasize_x != 1 ||
         tileset->metasize_y != 1 ||
         tileset->metabuffer != 0)) {
      tileset->format = config->merge_format;
   }


   geocache_configuration_add_tileset(config,tileset,name);
   return;
}

void geocache_configuration_parse(geocache_context *ctx, const char *filename, geocache_cfg *config) {
   xmlDocPtr doc;
   char *testlockfilename;
   apr_file_t *testlockfile;
   doc = xmlReadFile(filename, NULL, 0);
   if (doc == NULL) {
      ctx->set_error(ctx,GEOCACHE_PARSE_ERROR, "libxml2 failed to parse file %s. Is it valid XML?", filename);
      return;
   }

   xmlNode *root_element = xmlDocGetRootElement(doc);

   if(root_element->type == XML_ELEMENT_NODE && !xmlStrcmp(root_element->name, BAD_CAST "geocache")) {
      xmlNode *children = root_element->children;
      xmlNode *cur_node;
      for(cur_node = children; cur_node; cur_node = cur_node->next) {
         if(cur_node->type != XML_ELEMENT_NODE) continue;
         if(!xmlStrcmp(cur_node->name, BAD_CAST "source")) {
            parseSource(ctx, cur_node, config);
            GC_CHECK_ERROR(ctx);
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "cache")) {
            parseCache(ctx, cur_node, config);
            GC_CHECK_ERROR(ctx);
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "format")) {
            parseFormat(ctx, cur_node, config);
            GC_CHECK_ERROR(ctx);
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "grid")) {
            parseGrid(ctx, cur_node, config);
            GC_CHECK_ERROR(ctx);
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "tileset")) {
            continue; //we'll parse it in a second loop
         }  else if(!xmlStrcmp(cur_node->name, BAD_CAST "services")) {
            xmlNode *service_node;
            for(service_node = cur_node->children; service_node; service_node = service_node->next) {
               if(service_node->type != XML_ELEMENT_NODE) continue;
               if(!xmlStrcmp(service_node->name, BAD_CAST "wms")) {
                  xmlChar* value = xmlNodeGetContent(service_node);
                  if(!value || !*value || xmlStrcmp(value, BAD_CAST "false")) {
                     config->services[GEOCACHE_SERVICE_WMS] = geocache_service_wms_create(ctx);
                  }
                  xmlFree(value);
               } else if(!xmlStrcmp(service_node->name, BAD_CAST "tms")) {
                  xmlChar* value = xmlNodeGetContent(service_node);
                  if(!value || !*value || xmlStrcmp(value, BAD_CAST "false")) {
                     config->services[GEOCACHE_SERVICE_TMS] = geocache_service_tms_create(ctx);
                  }
                  xmlFree(value);
               }
            }
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "merge_format")) {
            char* value = (char*) xmlNodeGetContent(cur_node);
            geocache_image_format *format = geocache_configuration_get_image_format(config,value);
            if(!format) {
               ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "merge_format tag references format %s but it is not configured",
                     value);
               return;
               
            }
            config->merge_format = format;
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "lock_dir")) {
            xmlChar *value = xmlNodeGetContent(cur_node);
            config->lockdir = apr_pstrdup(ctx->pool, (char*)value);
            xmlFree(value);
         } else {
            ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "failed to parse geocache config file %s: unknown tag <%s>",
                  filename, cur_node->name);
            return;
         }

      }
      /* second loop to parse the tilesets */
      for(cur_node = children; cur_node; cur_node = cur_node->next) {
         if(cur_node->type != XML_ELEMENT_NODE) continue;
         else if(!xmlStrcmp(cur_node->name, BAD_CAST "tileset")) {
            parseTileset(ctx,cur_node,config);
            GC_CHECK_ERROR(ctx);
         }
      }

   } else {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, 
            "failed to parse geocache config file %s: "
            "document does not begin with <geocache> tag. found <%s>",
            filename,root_element->name);
      return;
   }

   /* check our lock directory is valid and writable */
   if(APR_SUCCESS != apr_dir_make_recursive(config->lockdir, APR_OS_DEFAULT, ctx->pool)) {
       ctx->set_error(ctx, GEOCACHE_DISK_ERROR, "failed to create lock directory %s",config->lockdir);
       return;
   }
   testlockfilename = apr_psprintf(ctx->pool,"%s/test.lock",config->lockdir);
   if(apr_file_open(&testlockfile, testlockfilename, APR_FOPEN_CREATE|APR_FOPEN_WRITE,
               APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS) {
       ctx->set_error(ctx, GEOCACHE_DISK_ERROR,  "failed to create test lockfile %s",testlockfilename);
       return; /* we could not create the file */
   }
   apr_file_close(testlockfile);
   apr_file_remove(testlockfilename,ctx->pool);

   if(!config->services[GEOCACHE_SERVICE_WMS] &&
         !config->services[GEOCACHE_SERVICE_TMS]) {
      ctx->set_error(ctx, GEOCACHE_PARSE_ERROR, "no services configured."
            " You must add a <services> tag with <wms/> or <tms/> children");
      return;
   }


   xmlFreeDoc(doc);
   xmlCleanupParser();

   return;
}

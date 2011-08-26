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

void geocache_configuration_parse(geocache_context *ctx, const char *filename, geocache_cfg *config) {
   int len = strlen(filename);
   const char *ext = &(filename[len-3]);
   if(strcasecmp(ext,"xml")) {
      geocache_configuration_parse_json(ctx,filename,config);
   } else {
      geocache_configuration_parse_xml(ctx,filename,config);
   }
   GC_CHECK_ERROR(ctx);

   if(!config->lockdir || !strlen(config->lockdir)) {
      config->lockdir = apr_pstrdup(ctx->pool, "/tmp");
   }
   apr_dir_t *lockdir;
   apr_status_t rv;
   rv = apr_dir_open(&lockdir,config->lockdir,ctx->pool);
   char errmsg[120];
   if(rv != APR_SUCCESS) {
      ctx->set_error(ctx,500, "failed to open lock directory %s: %s"
            ,config->lockdir,apr_strerror(rv,errmsg,120));
      return;
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
               return;
            }

         }
      
      }
   }
   apr_dir_close(lockdir);

   /* if we were suppplied with an onlineresource, make sure it ends with a / */
   char *url;
   if(NULL != (url = (char*)apr_table_get(config->metadata,"url"))) {
      char *urlend = url + strlen(url)-1;
      if(*urlend != '/') {
         url = apr_pstrcat(ctx->pool,url,"/",NULL);
         apr_table_setn(config->metadata,"url",url);
      }
   }
}

void geocache_configuration_post_config(geocache_context *ctx, geocache_cfg *config) {
   apr_hash_index_t *cachei = apr_hash_first(ctx->pool,config->caches);
   while(cachei) {
      geocache_cache *cache;
      const void *key; apr_ssize_t keylen;
      apr_hash_this(cachei,&key,&keylen,(void**)&cache);
      cache->configuration_post_config(ctx,cache,config);
      GC_CHECK_ERROR(ctx);
      cachei = apr_hash_next(cachei);
   }
} 


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

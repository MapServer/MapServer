#include "geocache.h"
#include <errno.h>


static void parse_extent_json(geocache_context *ctx, cJSON *node, double *extent) {
   int i = cJSON_GetArraySize(node);
   if(i != 4) {
      ctx->set_error(ctx,400,"invalid extent");
      return;
   }
   for(i=0;i<4;i++) {
      cJSON *item = cJSON_GetArrayItem(node,i);
      if(item->type != cJSON_Number) {
         ctx->set_error(ctx,400,"failed to parse extent (not a number)");
         return;
      }
      extent[i] = item->valuedouble;
   }
   if(extent[0] >= extent[2] || extent[1] >= extent[3]) {
      ctx->set_error(ctx, 400, "invalid extent [%f, %f, %f, %f]",
            extent[0],extent[1],extent[2],extent[3]);
      return;
   }
   
}

void parse_keyvalues(geocache_context *ctx, cJSON *node, apr_table_t *tbl) {
   if(!node->child) return;
   cJSON *child = node->child;
   while(child) {
      if(child->type != cJSON_String) {
         ctx->set_error(ctx,400,"key/values can only contain string values");
         return;
      }
      apr_table_set(tbl,child->string,child->valuestring);
      child = child->next;
   }
}

static void parse_grid_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
   char *name = NULL;
   int i;
   cJSON *tmp;
   geocache_grid *grid;

   tmp = cJSON_GetObjectItem(node,"name");
   if(tmp) name = tmp->valuestring;
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in grid");
      return;
   }
   else {
      name = apr_pstrdup(ctx->pool, name);
      /* check we don't already have a grid defined with this name */
      if(geocache_configuration_get_grid(cfg, name)) {
         ctx->set_error(ctx, 400, "duplicate grid with name \"%s\"",name);
         return;
      }
   }
   grid = geocache_grid_create(ctx->pool);
   grid->name = name;
   
   tmp = cJSON_GetObjectItem(node,"metadata");
   if(tmp) {
      parse_keyvalues(ctx,tmp,grid->metadata);
      GC_CHECK_ERROR(ctx);
   }


   tmp = cJSON_GetObjectItem(node,"extent");
   if(!tmp) {
      ctx->set_error(ctx, 400, "extent not found in grid \"%s\"",name);
      return;
   }
   parse_extent_json(ctx,tmp,grid->extent);
   GC_CHECK_ERROR(ctx);
   
   tmp = cJSON_GetObjectItem(node,"resolutions");
   if(!tmp) {
      ctx->set_error(ctx, 400, "resolutions not found in grid \"%s\"",name);
      return;
   }
   i = cJSON_GetArraySize(tmp);
   if(i == 0) {
      ctx->set_error(ctx,400,"resolutions for grid %s is not an array",name);
      return;
   }
   grid->levels = apr_pcalloc(ctx->pool,i*sizeof(geocache_grid_level*));
   grid->nlevels = i;
   for(i=0;i<grid->nlevels;i++) {
      cJSON *item = cJSON_GetArrayItem(tmp,i);
      if(item->type != cJSON_Number) {
         ctx->set_error(ctx,400,"failed to parse resolution %s in grid %s (not a number)", item->valuestring,name);
         return;
      }
      grid->levels[i] = apr_pcalloc(ctx->pool,sizeof(geocache_grid_level));
      grid->levels[i]->resolution = item->valuedouble;
   }
   
   tmp = cJSON_GetObjectItem(node,"size");
   if(!tmp) {
      ctx->set_error(ctx, 400, "size not found in grid \"%s\"",name);
      return;
   }
   i = cJSON_GetArraySize(tmp);
   if(i != 2) {
      ctx->set_error(ctx,400,"size for grid %s is not a correct array, expecting [width,height]",name);
      return;
   }
   for(i=0;i<2;i++) {
      cJSON *item = cJSON_GetArrayItem(tmp,i);
      if(item->type != cJSON_Number) {
         ctx->set_error(ctx,400,"failed to parse size %s in grid %s (not a number)", item->valuestring,name);
         return;
      }
      if(i==0)
         grid->tile_sx = item->valueint;
      else
         grid->tile_sy = item->valueint;
   }
   if(grid->tile_sx < 1 || grid->tile_sy < 1) {
      ctx->set_error(ctx,400,"grid %s size (%d,%d) is invalid", name,grid->tile_sx,grid->tile_sy);
      return;
   }

   tmp = cJSON_GetObjectItem(node,"units");
   if(tmp && tmp->valuestring) {
      if(!strcasecmp(tmp->valuestring,"dd"))
         grid->unit = GEOCACHE_UNIT_DEGREES;
      else if(!strcasecmp(tmp->valuestring,"m"))
         grid->unit = GEOCACHE_UNIT_METERS;
      else if(!strcasecmp(tmp->valuestring,"ft"))
         grid->unit = GEOCACHE_UNIT_FEET;
      else {
         ctx->set_error(ctx,400,"unknown unit %s for grid %s",tmp->valuestring,name);
         return;
      }
   }
   
   tmp = cJSON_GetObjectItem(node,"srs");
   if(tmp && tmp->valuestring) {
      grid->srs = apr_pstrdup(ctx->pool,tmp->valuestring);
   } else {
      ctx->set_error(ctx,400,"grid %s has no srs",name);
      return;
   }

   tmp = cJSON_GetObjectItem(node,"srsaliases");
   if(tmp) {
      i = cJSON_GetArraySize(tmp);
      while(i--) {
         cJSON *alias = cJSON_GetArrayItem(tmp,i);
         if(!alias || !alias->valuestring) {
            ctx->set_error(ctx,400,"failed to parse srsaliases for grid %s, expecting [\"alias1\",\"alias2\",..]",name);
         }
         APR_ARRAY_PUSH(grid->srs_aliases,char*) = apr_pstrdup(ctx->pool,alias->valuestring);
      }
   }


   geocache_configuration_add_grid(cfg,grid,name);   
}
static void parse_source_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
   char *name = NULL, *type = NULL;
   cJSON *tmp;
   geocache_source *source;

   tmp = cJSON_GetObjectItem(node,"name");
   if(tmp) name = tmp->valuestring;
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in source");
      return;
   }
   else {
      name = apr_pstrdup(ctx->pool, name);
      /* check we don't already have a cache defined with this name */
      if(geocache_configuration_get_source(cfg, name)) {
         ctx->set_error(ctx, 400, "duplicate source with name \"%s\"",name);
         return;
      }
   }
   tmp = cJSON_GetObjectItem(node,"type");
   if(tmp) type = tmp->valuestring;
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"type\" not found in source");
      return;
   }
   if(!strcmp(type,"wms")) {
      source = geocache_source_wms_create(ctx);
   } else {
      ctx->set_error(ctx,400,"unknown cache type %s",type);
      return;
   }
   GC_CHECK_ERROR(ctx);
   source->name = name;

   tmp = cJSON_GetObjectItem(node,"metadata");
   if(tmp) {
      parse_keyvalues(ctx,tmp,source->metadata);
      GC_CHECK_ERROR(ctx);
   }
   
   tmp = cJSON_GetObjectItem(node,"props");
   source->configuration_parse_json(ctx,tmp,source);
   GC_CHECK_ERROR(ctx);
   source->configuration_check(ctx,source);
   GC_CHECK_ERROR(ctx);
   geocache_configuration_add_source(cfg,source,name);


}
static void parse_cache_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
   char *name = NULL, *type = NULL;
   cJSON *tmp;
   geocache_cache *cache;

   tmp = cJSON_GetObjectItem(node,"name");
   if(tmp) name = tmp->valuestring;
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in cache");
      return;
   }
   else {
      name = apr_pstrdup(ctx->pool, name);
      /* check we don't already have a cache defined with this name */
      if(geocache_configuration_get_cache(cfg, name)) {
         ctx->set_error(ctx, 400, "duplicate cache with name \"%s\"",name);
         return;
      }
   }
   tmp = cJSON_GetObjectItem(node,"type");
   if(tmp) type = tmp->valuestring;
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"type\" not found in cache");
      return;
   }
   if(!strcmp(type,"disk")) {
      cache = geocache_cache_disk_create(ctx);
   } else if(!strcmp(type,"sqlite3")) {
#ifdef USE_SQLITE
      cache = geocache_cache_sqlite_create(ctx);
#else
      ctx->set_error(ctx,400, "failed to add cache \"%s\": sqlite support is not available on this build",name);
      return;
#endif
   } else if(!strcmp(type,"memcache")) {
#ifdef USE_MEMCACHE
      cache = geocache_cache_memcache_create(ctx);
#else
      ctx->set_error(ctx,400, "failed to add cache \"%s\": memcache support is not available on this build",name);
      return;
#endif
   } else {
      ctx->set_error(ctx,400,"unknown cache type %s",type);
      return;
   }
   GC_CHECK_ERROR(ctx);
   cache->name = name;
   
   tmp = cJSON_GetObjectItem(node,"metadata");
   if(tmp) {
      parse_keyvalues(ctx,tmp,cache->metadata);
      GC_CHECK_ERROR(ctx);
   }

   tmp = cJSON_GetObjectItem(node,"props");
   if(tmp) {
      cache->configuration_parse_json(ctx,tmp,cache);
      GC_CHECK_ERROR(ctx);
   }
   geocache_configuration_add_cache(cfg,cache,name);


}
static void parse_tileset_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
   char *name = NULL;
   cJSON *tmp;
   geocache_tileset *tileset = NULL;

   tmp = cJSON_GetObjectItem(node,"name");
   if(tmp) name = tmp->valuestring;
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in tileset");
      return;
   }
   else {
      name = apr_pstrdup(ctx->pool, name);
      /* check we don't already have a tileset defined with this name */
      if(geocache_configuration_get_tileset(cfg, name)) {
         ctx->set_error(ctx, 400, "duplicate tileset with name \"%s\"",name);
         return;
      }
   }
   tileset = geocache_tileset_create(ctx);
   tileset->name = name;
   tmp = cJSON_GetObjectItem(node,"metadata");
   if(tmp) {
      parse_keyvalues(ctx,tmp,tileset->metadata);
      GC_CHECK_ERROR(ctx);
   }
   
   tmp = cJSON_GetObjectItem(node,"wgs84_extent");
   if(tmp) {
      parse_extent_json(ctx,tmp,tileset->wgs84bbox);
      GC_CHECK_ERROR(ctx);
   }
   





}
static void parse_format_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
   char *name = NULL, *type = NULL;
   cJSON *tmp,*props;
   geocache_image_format *format = NULL;

   tmp = cJSON_GetObjectItem(node,"name");
   if(tmp) name = tmp->valuestring;
   if(!name || !strlen(name)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"name\" not found in format");
      return;
   }
   else {
      name = apr_pstrdup(ctx->pool, name);
      /* check we don't already have a format defined with this name */
      if(geocache_configuration_get_image_format(cfg, name)) {
         ctx->set_error(ctx, 400, "duplicate format with name \"%s\"",name);
         return;
      }
   }
   tmp = cJSON_GetObjectItem(node,"type");
   if(tmp) type = tmp->valuestring;
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"type\" not found in format %s",name);
      return;
   }
   

   props = cJSON_GetObjectItem(node,"props");
   
   if(!strcmp(type,"png")) {
      int colors = -1;
      geocache_compression_type compression = GEOCACHE_COMPRESSION_DEFAULT;
      if(props) {
         tmp = cJSON_GetObjectItem(props,"compression");
         if(tmp && tmp->valuestring && strlen(tmp->valuestring)) {
            if(!strcmp(tmp->valuestring, "fast")) {
               compression = GEOCACHE_COMPRESSION_FAST;
            } else if(!strcmp(tmp->valuestring, "best")) {
               compression = GEOCACHE_COMPRESSION_BEST;
            } else if(!strcmp(tmp->valuestring, "default")) {
               compression = GEOCACHE_COMPRESSION_DEFAULT;
            } else {
               ctx->set_error(ctx, 400, "unknown compression type %s for format \"%s\"", tmp->valuestring, name);
               return;
            }
         }
         tmp = cJSON_GetObjectItem(props,"colors");
         if(tmp && tmp->valueint && tmp->valueint>0 && tmp->valueint<=256) {
            colors = tmp->valueint;
         }

      }

      if(colors == -1) {
         format = geocache_imageio_create_png_format(ctx->pool, name, compression);
      } else {
         format = geocache_imageio_create_png_q_format(ctx->pool, name, compression, colors);
      }
   } else if(!strcmp(type,"jpeg") || !strcmp(type,"jpg")){
      int quality = 95;
      if(props) {
         tmp = cJSON_GetObjectItem(props,"quality");
         if(tmp && tmp->valueint && tmp->valueint>0 && tmp->valueint<=100) {
            quality = tmp->valueint;
         }
      }
      format = geocache_imageio_create_jpeg_format(ctx->pool,name,quality);
   } else if(!strcmp(type,"mixed")){
      geocache_image_format *transparent=NULL, *opaque=NULL;
      if(props) {
         tmp = cJSON_GetObjectItem(props,"opaque");
         if(!tmp || !tmp->valuestring) {
            ctx->set_error(ctx,400,"mixed format %s does not reference an opaque format",name);
            return;
         }
         opaque = geocache_configuration_get_image_format(cfg,tmp->valuestring);
         if(!opaque) {
            ctx->set_error(ctx,400, "mixed format %s references unknown opaque format %s"
                  "(order is important, format %s should appear first)",
                  name,tmp->valuestring,tmp->valuestring);
            return;
         }
         tmp = cJSON_GetObjectItem(props,"transparent");
         if(!tmp || !tmp->valuestring) {
            ctx->set_error(ctx,400,"mixed format %s does not reference an transparent format",name);
            return;
         }
         transparent = geocache_configuration_get_image_format(cfg,tmp->valuestring);
         if(!transparent) {
            ctx->set_error(ctx,400, "mixed format %s references unknown transparent format %s"
                  "(order is important, format %s should appear first)",
                  name,tmp->valuestring,tmp->valuestring);
            return;
         }
      } else {
         ctx->set_error(ctx,400,"mixed format %s has no props",name);
         return;
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

   tmp = cJSON_GetObjectItem(node,"metadata");
   if(tmp) {
      parse_keyvalues(ctx,tmp,format->metadata);
      GC_CHECK_ERROR(ctx);
   }

   geocache_configuration_add_image_format(cfg,format,name);
}
static void parse_service_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
}


void geocache_configuration_parse_json(geocache_context *ctx, const char *filename, geocache_cfg *config) {
   FILE *f=fopen(filename,"rb");
   int i;
   cJSON *root,*entry,*item;
   if(f==NULL) {
      ctx->set_error(ctx,400, "failed to open file %s: %s", filename,strerror(errno));
      return;
   }

   fseek(f,0,SEEK_END);
   long len=ftell(f);
   fseek(f,0,SEEK_SET);
   char *filedata = malloc(len+1);
   if(len != fread(filedata,1,len,f)) {
      ctx->set_error(ctx,400, "failed to read all bytes of %s", filename);
      goto cleanup;
   }

   root = cJSON_Parse(filedata);
   if(!root) {
      ctx->set_error(ctx,400, "failed to parse %s : %s", filename,cJSON_GetErrorPtr());
      goto cleanup;
   }

   entry = cJSON_GetObjectItem(root,"grids");
   if(entry) {
      for(i=0;i<cJSON_GetArraySize(entry);i++) {
         item = cJSON_GetArrayItem(entry,i);
         parse_grid_json(ctx,item,config);
         if(GC_HAS_ERROR(ctx)) goto cleanup;
      }
   }
   entry = cJSON_GetObjectItem(root,"sources");
   if(entry) {
      for(i=0;i<cJSON_GetArraySize(entry);i++) {
         item = cJSON_GetArrayItem(entry,i);
         parse_source_json(ctx,item,config);
         if(GC_HAS_ERROR(ctx)) goto cleanup;
      }
   }
   entry = cJSON_GetObjectItem(root,"caches");
   if(entry) {
      for(i=0;i<cJSON_GetArraySize(entry);i++) {
         item = cJSON_GetArrayItem(entry,i);
         parse_cache_json(ctx,item,config);
         if(GC_HAS_ERROR(ctx)) goto cleanup;
      }
   }
   entry = cJSON_GetObjectItem(root,"tilesets");
   if(entry) {
      for(i=0;i<cJSON_GetArraySize(entry);i++) {
         item = cJSON_GetArrayItem(entry,i);
         parse_tileset_json(ctx,item,config);
         if(GC_HAS_ERROR(ctx)) goto cleanup;
      }
   }
   entry = cJSON_GetObjectItem(root,"metadata");
   if(entry) {
      parse_keyvalues(ctx, entry, config->metadata);
      if(GC_HAS_ERROR(ctx)) goto cleanup;
   }
   entry = cJSON_GetObjectItem(root,"formats");
   if(entry) {
      for(i=0;i<cJSON_GetArraySize(entry);i++) {
         item = cJSON_GetArrayItem(entry,i);
         parse_format_json(ctx,item,config);
         if(GC_HAS_ERROR(ctx)) goto cleanup;
      }
   }
   entry = cJSON_GetObjectItem(root,"services");
   if(entry) {
      for(i=0;i<cJSON_GetArraySize(entry);i++) {
         item = cJSON_GetArrayItem(entry,i);
         parse_service_json(ctx,item,config);
         if(GC_HAS_ERROR(ctx)) goto cleanup;
      }
   }
   entry = cJSON_GetObjectItem(root,"config");
   if(entry) {
      /* lock_dir, error_reporting */
   }


cleanup:
   cJSON_Delete(root);
   fclose(f);
   free(filedata);
}

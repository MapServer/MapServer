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
}

static void parse_metadata(geocache_context *ctx, cJSON *node, apr_table_t *metadata) {
   if(!node->child) return;
   cJSON *child = node->child;
   while(child) {
      if(child->type != cJSON_String) {
         ctx->set_error(ctx,400,"metadata can only contain string values");
         return;
      }
      apr_table_set(metadata,child->string,child->valuestring);
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
      parse_metadata(ctx,tmp,grid->metadata);
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

   geocache_configuration_add_grid(cfg,grid,name);   
}
static void parse_source_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
}
static void parse_cache_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
}
static void parse_tileset_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
}
static void parse_metadata_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
}
static void parse_format_json(geocache_context *ctx, cJSON *node, geocache_cfg *cfg) {
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
      parse_metadata_json(ctx, entry, config);
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


cleanup:
   fclose(f);
   free(filedata);
}

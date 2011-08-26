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


static void parse_dimensions_json(geocache_context *ctx, cJSON *dims, geocache_tileset *tileset) {
   int i;
   apr_array_header_t *dimensions = apr_array_make(ctx->pool,1,sizeof(geocache_dimension*));
   for(i=0;i<cJSON_GetArraySize(dims);i++) {
      cJSON *node = cJSON_GetArrayItem(dims,i);
      char *type = NULL;
      cJSON *tmp;
      geocache_dimension *dimension;

      tmp = cJSON_GetObjectItem(node,"type");
      if(tmp) type = tmp->valuestring;
      if(!type || !strlen(type)) {
         ctx->set_error(ctx, 400, "mandatory \"type\" not found in dimension for tileset %s", tileset->name);
         return;
      }
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

      tmp = cJSON_GetObjectItem(node,"unit");
      if(tmp &&tmp->valuestring) {
         dimension->unit = apr_pstrdup(ctx->pool,tmp->valuestring);
      }
      tmp = cJSON_GetObjectItem(node,"name");
      if(tmp && tmp->valuestring) {
         dimension->name = apr_pstrdup(ctx->pool,tmp->valuestring);
      } else {
         ctx->set_error(ctx, 400, "mandatory \"name\" not found in dimension for tileset %s", tileset->name);
         return;
      }
      tmp = cJSON_GetObjectItem(node,"default");
      if(tmp && tmp->valuestring) {
         dimension->default_value = apr_pstrdup(ctx->pool,tmp->valuestring);
      } else {
         ctx->set_error(ctx, 400, "mandatory \"default\" not found in dimension for tileset %s", tileset->name);
         return;
      }


      tmp = cJSON_GetObjectItem(node,"props");
      if(!tmp || tmp->type != cJSON_Object) {
         ctx->set_error(ctx, 400, "mandatory \"props\" not found in dimension %s for tileset %s",
               dimension->name, tileset->name);
         return;
      } else {
         dimension->configuration_parse_json(ctx,dimension,tmp);
         GC_CHECK_ERROR(ctx);
      }
      APR_ARRAY_PUSH(dimensions,geocache_dimension*) = dimension;
   }
   if(apr_is_empty_array(dimensions)) {
      ctx->set_error(ctx, 400, "dimensions for tileset \"%s\" has no dimensions defined",tileset->name);
      return;
   }
   tileset->dimensions = dimensions;
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
   
   tmp = cJSON_GetObjectItem(node,"grids");
   if(tmp && cJSON_GetArraySize(tmp)) {
      tileset->grid_links = apr_array_make(ctx->pool,1,sizeof(geocache_grid_link*));
      int i;
      for(i=0;i<cJSON_GetArraySize(tmp);i++) {
         cJSON *jgrid = cJSON_GetArrayItem(tmp,i);
         cJSON *jchild;
         char *gridname;
         geocache_grid_link *gridlink = apr_pcalloc(ctx->pool,sizeof(geocache_grid_link));
         switch(jgrid->type) {
            case cJSON_String:
               gridname = jgrid->valuestring;
               break;
            case cJSON_Object:
               jchild = cJSON_GetObjectItem(jgrid,"ref");
               if(!jchild || jchild->type != cJSON_String || !jchild->valuestring) {
                  ctx->set_error(ctx, 400, "tileset \"%s\" references grid with no \"ref\"",name);
                  return;
               }
               gridname = jchild->valuestring;
               jchild = cJSON_GetObjectItem(jgrid,"extent");
               if(jchild) {
                  gridlink->restricted_extent = apr_pcalloc(ctx->pool,4*sizeof(double));
                  parse_extent_json(ctx,jchild,gridlink->restricted_extent);
                  GC_CHECK_ERROR(ctx);
               }
               break;
            default:
               ctx->set_error(ctx, 400, "tileset grid can either be a string or a {\"ref\":\"gridname\",\"extent\":{minx,miny,maxx,maxy} for tileset %s",name);
               return;
         }
         geocache_grid *grid = geocache_configuration_get_grid(cfg,gridname);
         if(!grid) {
            ctx->set_error(ctx, 400, "tileset %s references unknown grid \"%s\"",name,gridname);
            return;
         }
         
         gridlink->grid = grid;
         gridlink->grid_limits = apr_pcalloc(ctx->pool,grid->nlevels*sizeof(int*));
         for(i=0;i<grid->nlevels;i++) {
            gridlink->grid_limits[i] = apr_pcalloc(ctx->pool,4*sizeof(int));
         }
         double *extent;
         if(gridlink->restricted_extent &&
               gridlink->restricted_extent[2]>gridlink->restricted_extent[0] &&
               gridlink->restricted_extent[3]>gridlink->restricted_extent[1]) {
            extent = gridlink->restricted_extent;
         } else {
            extent = grid->extent;
         }
         geocache_grid_compute_limits(grid,extent,gridlink->grid_limits);

         /* compute wgs84 bbox if it wasn't supplied already */
         if(tileset->wgs84bbox[0] >= tileset->wgs84bbox[2] &&
               !strcasecmp(grid->srs,"EPSG:4326")) {
            tileset->wgs84bbox[0] = extent[0];
            tileset->wgs84bbox[1] = extent[1];
            tileset->wgs84bbox[2] = extent[2];
            tileset->wgs84bbox[3] = extent[3];
         }
         APR_ARRAY_PUSH(tileset->grid_links,geocache_grid_link*) = gridlink;
      }
   } else {
      ctx->set_error(ctx, 400, "tileset \"%s\" references no grids",name);
      return;
   }
   tmp = cJSON_GetObjectItem(node,"cache");
   if(!tmp || !tmp->valuestring) {
      ctx->set_error(ctx, 400, "tileset \"%s\" references no cache",name);
      return;
   } else {
      tileset->cache = geocache_configuration_get_cache(cfg,tmp->valuestring);
      if(!tileset->cache) {
         ctx->set_error(ctx, 400, "tileset \"%s\" references invalid cache \"%s\"",name,tmp->valuestring);
         return;
      }
   }
   tmp = cJSON_GetObjectItem(node,"source");
   if(!tmp || !tmp->valuestring) {
      ctx->set_error(ctx, 400, "tileset \"%s\" references no source",name);
      return;
   } else {
      tileset->source = geocache_configuration_get_source(cfg,tmp->valuestring);
      if(!tileset->source) {
         ctx->set_error(ctx, 400, "tileset \"%s\" references invalid source \"%s\"",name,tmp->valuestring);
         return;
      }
   }
   tmp = cJSON_GetObjectItem(node,"format");
   if(tmp && tmp->valuestring) {
      tileset->format = geocache_configuration_get_image_format(cfg,tmp->valuestring);
      if(!tileset->format) {
         ctx->set_error(ctx, 400, "tileset \"%s\" references invalid format \"%s\"",name,tmp->valuestring);
         return;
      }
   }

   //dimensions
   tmp = cJSON_GetObjectItem(node,"dimensions");
   if(tmp) {
      parse_dimensions_json(ctx,tmp,tileset);
      GC_CHECK_ERROR(ctx);
   }

   //metatile
   tmp = cJSON_GetObjectItem(node,"metatile");
   if(tmp && tmp->type == cJSON_Object) {
      cJSON *e = cJSON_GetObjectItem(tmp,"size");
      if(e->type == cJSON_Array && cJSON_GetArraySize(e) == 2 ) {
         tileset->metasize_x = cJSON_GetArrayItem(e,0)->valueint;
         tileset->metasize_y = cJSON_GetArrayItem(e,1)->valueint;
      }
      e = cJSON_GetObjectItem(tmp,"buffer");
      if(e && e->type == cJSON_Number) {
         tileset->metabuffer = e->valueint;
      }
   }
   
   //watermark
   tmp = cJSON_GetObjectItem(node,"watermark");
   if(tmp && tmp->valuestring) {
      geocache_tileset_add_watermark(ctx,tileset,tmp->valuestring);
      GC_CHECK_ERROR(ctx);
   }

   //expire and auto_expire
   tmp = cJSON_GetObjectItem(node,"expires");
   if(tmp && tmp->type == cJSON_Object) {
      cJSON *e = cJSON_GetObjectItem(tmp,"delay");
      if(e->type == cJSON_Number) {
         tileset->expires = e->valueint;
         e = cJSON_GetObjectItem(tmp,"auto");
         if(e->type == cJSON_True) {
            tileset->auto_expire = tileset->expires;
         }

      }
   }
   geocache_tileset_configuration_check(ctx,tileset);
   GC_CHECK_ERROR(ctx);
   
   geocache_configuration_add_tileset(cfg,tileset,name);




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
static void parse_service_json(geocache_context *ctx, cJSON *node, geocache_cfg *config) {
   cJSON *tmp = cJSON_GetObjectItem(node,"type");
   char *type = NULL;
   if(tmp) type = tmp->valuestring;
   if(!type || !strlen(type)) {
      ctx->set_error(ctx, 400, "mandatory attribute \"type\" not found in service");
      return;
   }
   tmp = cJSON_GetObjectItem(node,"enabled");
   if(tmp && tmp->type == cJSON_False) {
      return;
   }
   
   geocache_service *new_service;
   if (!strcasecmp(type,"wms")) {
      new_service = geocache_service_wms_create(ctx);
      config->services[GEOCACHE_SERVICE_WMS] = new_service;
   }
   else if (!strcasecmp(type,"tms")) {
      new_service = geocache_service_tms_create(ctx);
      config->services[GEOCACHE_SERVICE_TMS] = new_service;
   }
   else if (!strcasecmp(type,"wmts")) {
      new_service = geocache_service_wmts_create(ctx);
      config->services[GEOCACHE_SERVICE_WMTS] = new_service;
   }
   else if (!strcasecmp(type,"kml")) {
      new_service = geocache_service_kml_create(ctx);
      config->services[GEOCACHE_SERVICE_KML] = new_service;
   }
   else if (!strcasecmp(type,"gmaps")) {
      new_service = geocache_service_gmaps_create(ctx);
      config->services[GEOCACHE_SERVICE_GMAPS] = new_service;
   }
   else if (!strcasecmp(type,"ve")) {
      new_service = geocache_service_ve_create(ctx);
      config->services[GEOCACHE_SERVICE_VE] = new_service;
   }
   else if (!strcasecmp(type,"demo")) {
      new_service = geocache_service_demo_create(ctx);
      config->services[GEOCACHE_SERVICE_DEMO] = new_service;
   } else {
      ctx->set_error(ctx,400,"unknown <service> type %s",type);
      return;
   }
   
   tmp = cJSON_GetObjectItem(node,"props");
   if(tmp && new_service->configuration_parse_json) {
      new_service->configuration_parse_json(ctx,tmp,new_service,config);
   }
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
   entry = cJSON_GetObjectItem(root,"formats");
   if(entry) {
      for(i=0;i<cJSON_GetArraySize(entry);i++) {
         item = cJSON_GetArrayItem(entry,i);
         parse_format_json(ctx,item,config);
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

      /* directory where to store temporary lock files */
      cJSON *tmp = cJSON_GetObjectItem(entry,"lock_dir");
      if(tmp && tmp->valuestring) {
         config->lockdir = apr_pstrdup(ctx->pool,tmp->valuestring);
      }

      /* error reporting */
      tmp = cJSON_GetObjectItem(entry,"errors");
      if(tmp && tmp->valuestring) {
         if(!strcmp(tmp->valuestring,"log")) {
            config->reporting = GEOCACHE_REPORT_LOG;
         } else if(!strcmp(tmp->valuestring,"report")) {
            config->reporting = GEOCACHE_REPORT_MSG;
         } else if(!strcmp(tmp->valuestring,"empty_img")) {
            config->reporting = GEOCACHE_REPORT_EMPTY_IMG;
            geocache_image_create_empty(ctx, config);
            if(GC_HAS_ERROR(ctx)) goto cleanup;
         } else if(!strcmp(tmp->valuestring, "report_img")) {
            config->reporting = GEOCACHE_REPORT_ERROR_IMG;
            ctx->set_error(ctx,501,"<errors>: report_img not implemented");
            goto cleanup;
         } else {
            ctx->set_error(ctx,400,"<errors>: unknown value %s (allowed are log, report, empty_img, report_img)",
                  tmp->valuestring);
            goto cleanup;
         }
      }

      /* default image format */
      tmp = cJSON_GetObjectItem(entry,"default_format");
      if(tmp && tmp->valuestring) {
         config->default_image_format = geocache_configuration_get_image_format(config,tmp->valuestring);
         if(!config->default_image_format) {
            ctx->set_error(ctx, 400, "default_format \"%s\" does not exist",tmp->valuestring);
            return;
         }
      }
   }

   /* final checks */
   if(!config->lockdir) {
      config->lockdir = apr_pstrdup(ctx->pool,"/tmp");
   }


cleanup:
   //cJSON_Delete(root);
   fclose(f);
   //free(filedata);
}

#include "geocache.h"
#include <libxml/tree.h>
#include <apr_tables.h>
#include <apr_strings.h>

/**
 * \private \memberof geocache_source_gdal
 * \sa geocache_source::render_tile()
 */
void _geocache_source_gdal_render_tile(geocache_context *ctx, geocache_tile *tile) {
   geocache_source_gdal *gdal = (geocache_source_gdal*)tile->tileset->source;
   apr_table_t *params = apr_table_clone(ctx->pool,gdal->gdal_default_params);
   double bbox[4];
   geocache_tileset_tile_bbox(tile,bbox);
   apr_table_setn(params,"BBOX",apr_psprintf(ctx->pool,"%f,%f,%f,%f",bbox[0],bbox[1],bbox[2],bbox[3]));
   apr_table_setn(params,"WIDTH",apr_psprintf(ctx->pool,"%d",tile->sx));
   apr_table_setn(params,"HEIGHT",apr_psprintf(ctx->pool,"%d",tile->sy));
   apr_table_setn(params,"FORMAT","image/png");
   apr_table_setn(params,"SRS",tile->tileset->srs);
   
   apr_table_overlap(params,gdal->gdal_params,0);
        
   tile->data = geocache_buffer_create(1000,ctx->pool);
   geocache_http_request_url_with_params(ctx,gdal->url,params,tile->data);
}

/**
 * \private \memberof geocache_source_gdal
 * \sa geocache_source::render_metatile()
 */
void _geocache_source_gdal_render_metatile(geocache_context *ctx, geocache_metatile *tile) {
   geocache_source_gdal *gdal = (geocache_source_gdal*)tile->tile.tileset->source;
   
   apr_table_overlap(params,gdal->gdal_params,0);
        
   tile->tile.data = geocache_buffer_create(30000,ctx->pool);
   geocache_http_request_url_with_params(ctx,gdal->url,params,tile->tile.data);
   GC_CHECK_ERROR(ctx);
   
   if(!geocache_imageio_is_valid_format(ctx,tile->tile.data)) {
      ctx->set_error(ctx, GEOCACHE_SOURCE_GDAL, "gdal request failed for tileset %s: %d %d %d returned an unsupported format",
            tile->tile.tileset->name, tile->tile.x, tile->tile.y, tile->tile.z);
   }
}

/**
 * \private \memberof geocache_source_gdal
 * \sa geocache_source::configuration_parse()
 */
void _geocache_source_gdal_configuration_parse(geocache_context *ctx, xmlNode *xml, geocache_source *source) {
   xmlNode *cur_node;
   geocache_source_gdal *src = (geocache_source_gdal*)source;
   for(cur_node = xml->children; cur_node; cur_node = cur_node->next) {
      if(cur_node->type != XML_ELEMENT_NODE) continue;
      if(!xmlStrcmp(cur_node->name, BAD_CAST "data")) {
         char* value = (char*)xmlNodeGetContent(cur_node);
         src->data = value;
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "options")) {
         xmlNode *param_node;
         for(param_node = cur_node->children; param_node; param_node = param_node->next) {
            char *key,*value;
            if(param_node->type != XML_ELEMENT_NODE) continue;
            value = (char*)xmlNodeGetContent(param_node);
            key = apr_pstrdup(ctx->pool, (char*)param_node->name);
            apr_table_setn(src->gdal_params, key, value);
         }
      }
   }
}

/**
 * \private \memberof geocache_source_gdal
 * \sa geocache_source::configuration_check()
 */
void _geocache_source_gdal_configuration_check(geocache_context *ctx, geocache_source *source) {
   geocache_source_gdal *src = (geocache_source_gdal*)source;
   /* check all required parameters are configured */
   if(!strlen(src->data)) {
      ctx->set_error(ctx, GEOCACHE_SOURCE_GDAL, "gdal source %s has no data",source->name);
   }
   /* TODO: open source and check it accepts given options */
}

geocache_source* geocache_source_gdal_create(geocache_context *ctx) {
   geocache_source_gdal *source = apr_pcalloc(ctx->pool, sizeof(geocache_source_gdal));
   if(!source) {
      ctx->set_error(ctx, GEOCACHE_ALLOC_ERROR, "failed to allocate gdal source");
      return NULL;
   }
   geocache_source_init(ctx, &(source->source));
   source->source.type = GEOCACHE_SOURCE_GDAL;
   source->source.supports_metatiling = 1;
   source->source.render_tile = _geocache_source_gdal_render_tile;
   source->source.render_metatile = _geocache_source_gdal_render_metatile;
   source->source.configuration_check = _geocache_source_gdal_configuration_check;
   source->source.configuration_parse = _geocache_source_gdal_configuration_parse;
   source->options = apr_table_make(ctx->pool,4);
   return (geocache_source*)source;
}




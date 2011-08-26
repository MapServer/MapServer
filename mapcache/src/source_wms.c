#include "geocache.h"
#include <libxml/tree.h>
#include <apr_tables.h>
#include <apr_strings.h>
#include <http_log.h>

int _geocache_source_wms_render_tile(geocache_tile *tile, request_rec *r) {
   geocache_wms_source *wms = (geocache_wms_source*)tile->tileset->source;
   int ret;
   apr_table_t *params = apr_table_clone(r->pool,wms->wms_default_params);
   double bbox[4];
   geocache_tileset_tile_bbox(tile,bbox);
   apr_table_setn(params,"BBOX",apr_psprintf(r->pool,"%f,%f,%f,%f",bbox[0],bbox[1],bbox[2],bbox[3]));
   apr_table_setn(params,"WIDTH",apr_psprintf(r->pool,"%d",tile->sx));
   apr_table_setn(params,"HEIGHT",apr_psprintf(r->pool,"%d",tile->sy));
   apr_table_setn(params,"FORMAT","image/png");
   apr_table_setn(params,"SRS",tile->tileset->srs);
   
   apr_table_overlap(params,wms->wms_params,0);
        
   tile->data = geocache_buffer_create(1000,r->pool);
   ret = geocache_http_request_url_with_params(r,wms->url,params,tile->data);
      
   return GEOCACHE_SUCCESS;
}

int _geocache_source_wms_render_metatile(geocache_metatile *tile, request_rec *r) {
   geocache_wms_source *wms = (geocache_wms_source*)tile->tile.tileset->source;
   int ret;
   geocache_image_format_type format;
   apr_table_t *params = apr_table_clone(r->pool,wms->wms_default_params);
   apr_table_setn(params,"BBOX",apr_psprintf(r->pool,"%f,%f,%f,%f",
         tile->bbox[0],tile->bbox[1],tile->bbox[2],tile->bbox[3]));
   apr_table_setn(params,"WIDTH",apr_psprintf(r->pool,"%d",tile->tile.sx));
   apr_table_setn(params,"HEIGHT",apr_psprintf(r->pool,"%d",tile->tile.sy));
   apr_table_setn(params,"FORMAT","image/png");
   apr_table_setn(params,"SRS",tile->tile.tileset->srs);
   
   apr_table_overlap(params,wms->wms_params,0);
        
   tile->tile.data = geocache_buffer_create(30000,r->pool);
   ret = geocache_http_request_url_with_params(r,wms->url,params,tile->tile.data);
   if(ret != GEOCACHE_SUCCESS) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,"wms request failed for tileset %s: %d %d %d", tile->tile.tileset->name,
            tile->tile.x, tile->tile.y, tile->tile.z);
      return GEOCACHE_FAILURE;
   }
   
   format = geocache_imageio_header_sniff(r,tile->tile.data);
   if(format == GEOCACHE_IMAGE_FORMAT_UNKNOWN) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
            "wms request failed for tileset %s: %d %d %d returned an unsupported format",
            tile->tile.tileset->name, tile->tile.x, tile->tile.y, tile->tile.z);
      return GEOCACHE_FAILURE;
   }
   return GEOCACHE_SUCCESS;
}

char* _geocache_source_wms_configuration_parse(xmlNode *xml, geocache_source *source, apr_pool_t *pool) {
   xmlNode *cur_node;
   geocache_wms_source *src = (geocache_wms_source*)source;
   for(cur_node = xml->children; cur_node; cur_node = cur_node->next) {
      if(cur_node->type != XML_ELEMENT_NODE) continue;
      if(!xmlStrcmp(cur_node->name, BAD_CAST "url")) {
         char* value = (char*)xmlNodeGetContent(cur_node);
         src->url = value;
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "wmsparams")) {
         xmlNode *param_node;
         for(param_node = cur_node->children; param_node; param_node = param_node->next) {
            char *key,*value;
            if(param_node->type != XML_ELEMENT_NODE) continue;
            value = (char*)xmlNodeGetContent(param_node);
            key = apr_pstrdup(pool, (char*)param_node->name);
            apr_table_setn(src->wms_params, key, value);
         }
      }
   }
   return NULL;
}

char* _geocache_source_wms_configuration_check(geocache_source *source, apr_pool_t *pool) {
   geocache_wms_source *src = (geocache_wms_source*)source;
   /* check all required parameters are configured */
   if(!strlen(src->url)) {
      return apr_psprintf(pool,"wms source %s has no url",source->name);
   }
   if(!apr_table_get(src->wms_params,"LAYERS"))
      return apr_psprintf(pool,"wms source %s has no LAYERS", source->name);


   return NULL;
}

geocache_wms_source* geocache_source_wms_create(apr_pool_t *pool) {
   geocache_wms_source *source = apr_pcalloc(pool, sizeof(geocache_wms_source));
   geocache_source_init(&(source->source),pool);
   source->source.type = GEOCACHE_SOURCE_WMS;
   source->source.supports_metatiling = 1;
   source->source.render_tile = _geocache_source_wms_render_tile;
   source->source.render_metatile = _geocache_source_wms_render_metatile;
   source->source.configuration_check = _geocache_source_wms_configuration_check;
   source->source.configuration_parse = _geocache_source_wms_configuration_parse;
   source->wms_default_params = apr_table_make(pool,4);;
   source->wms_params = apr_table_make(pool,4);
   apr_table_add(source->wms_default_params,"VERSION","1.1.1");
   apr_table_add(source->wms_default_params,"REQUEST","GetMap");
   apr_table_add(source->wms_default_params,"SERVICE","WMS");
   apr_table_add(source->wms_default_params,"STYLES","");
   return source;
}




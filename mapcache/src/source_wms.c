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
#include <apr_tables.h>
#include <apr_strings.h>

/**
 * \private \memberof geocache_source_wms
 * \sa geocache_source::render_map()
 */
void _geocache_source_wms_render_map(geocache_context *ctx, geocache_map *map) {
    geocache_source_wms *wms = (geocache_source_wms*)map->tileset->source;
    apr_table_t *params = apr_table_clone(ctx->pool,wms->wms_default_params);
    apr_table_setn(params,"BBOX",apr_psprintf(ctx->pool,"%f,%f,%f,%f",
             map->extent[0],map->extent[1],map->extent[2],map->extent[3]));
    apr_table_setn(params,"WIDTH",apr_psprintf(ctx->pool,"%d",map->width));
    apr_table_setn(params,"HEIGHT",apr_psprintf(ctx->pool,"%d",map->height));
    apr_table_setn(params,"FORMAT","image/png");
    apr_table_setn(params,"SRS",map->grid_link->grid->srs);
 
    apr_table_overlap(params,wms->wms_params,0);
    if(map->dimensions && !apr_is_empty_table(map->dimensions)) {
       const apr_array_header_t *elts = apr_table_elts(map->dimensions);
       int i;
       for(i=0;i<elts->nelts;i++) {
          apr_table_entry_t entry = APR_ARRAY_IDX(elts,i,apr_table_entry_t);
          apr_table_setn(params,entry.key,entry.val);
       }
 
    }      
    map->data = geocache_buffer_create(30000,ctx->pool);
    geocache_http_request_url_with_params(ctx,wms->url,params,wms->http_headers,map->data);
    GC_CHECK_ERROR(ctx);
 
    if(!geocache_imageio_is_valid_format(ctx,map->data)) {
       char *returned_data = apr_pstrndup(ctx->pool,(char*)map->data->buf,map->data->size);
       ctx->set_error(ctx, 502, "wms request for tileset %s returned an unsupported format:\n%s",
             map->tileset->name, returned_data);
    }
}

/**
 * \private \memberof geocache_source_wms
 * \sa geocache_source::configuration_parse()
 */
void _geocache_source_wms_configuration_parse(geocache_context *ctx, ezxml_t node, geocache_source *source) {
   ezxml_t cur_node;
   geocache_source_wms *src = (geocache_source_wms*)source;


   if ((cur_node = ezxml_child(node,"url")) != NULL) {
      src->url = apr_pstrdup(ctx->pool,cur_node->txt);
   }
   if ((cur_node = ezxml_child(node,"wmsparams")) != NULL) {
      for(cur_node = cur_node->child; cur_node; cur_node = cur_node->sibling) {
         apr_table_set(src->wms_params, cur_node->name, cur_node->txt);
      }
   }
   if ((cur_node = ezxml_child(node,"http")) != NULL) {
      ezxml_t http_node;
      if((http_node = ezxml_child(cur_node,"headers")) != NULL) {
         ezxml_t header_node;
         for(header_node = http_node->child; header_node; header_node = header_node->sibling) {
            apr_table_set(src->http_headers, header_node->name, header_node->txt);
         }
      }
      /* TODO: parse <proxy> and <auth> elements */
   }
}

/**
 * \private \memberof geocache_source_wms
 * \sa geocache_source::configuration_check()
 */
void _geocache_source_wms_configuration_check(geocache_context *ctx, geocache_source *source) {
   geocache_source_wms *src = (geocache_source_wms*)source;
   /* check all required parameters are configured */
   if(!strlen(src->url)) {
      ctx->set_error(ctx, 400, "wms source %s has no url",source->name);
   }
   if(!apr_table_get(src->wms_params,"LAYERS")) {
      ctx->set_error(ctx, 400, "wms source %s has no LAYERS", source->name);
   }
}

geocache_source* geocache_source_wms_create(geocache_context *ctx) {
   geocache_source_wms *source = apr_pcalloc(ctx->pool, sizeof(geocache_source_wms));
   if(!source) {
      ctx->set_error(ctx, 500, "failed to allocate wms source");
      return NULL;
   }
   geocache_source_init(ctx, &(source->source));
   source->source.type = GEOCACHE_SOURCE_WMS;
   source->source.render_map = _geocache_source_wms_render_map;
   source->source.configuration_check = _geocache_source_wms_configuration_check;
   source->source.configuration_parse = _geocache_source_wms_configuration_parse;
   source->wms_default_params = apr_table_make(ctx->pool,4);;
   source->wms_params = apr_table_make(ctx->pool,4);
   source->http_headers = apr_table_make(ctx->pool,1);
   apr_table_add(source->wms_default_params,"VERSION","1.1.1");
   apr_table_add(source->wms_default_params,"REQUEST","GetMap");
   apr_table_add(source->wms_default_params,"SERVICE","WMS");
   apr_table_add(source->wms_default_params,"STYLES","");
   return (geocache_source*)source;
}



/* vim: ai ts=3 sts=3 et sw=3
*/

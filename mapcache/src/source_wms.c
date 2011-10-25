/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: WMS datasources
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/


#include "mapcache.h"
#include "ezxml.h"
#include <apr_tables.h>
#include <apr_strings.h>

/**
 * \private \memberof mapcache_source_wms
 * \sa mapcache_source::render_map()
 */
void _mapcache_source_wms_render_map(mapcache_context *ctx, mapcache_map *map) {
    mapcache_source_wms *wms = (mapcache_source_wms*)map->tileset->source;
    apr_table_t *params = apr_table_clone(ctx->pool,wms->wms_default_params);
    apr_table_setn(params,"BBOX",apr_psprintf(ctx->pool,"%f,%f,%f,%f",
             map->extent[0],map->extent[1],map->extent[2],map->extent[3]));
    apr_table_setn(params,"WIDTH",apr_psprintf(ctx->pool,"%d",map->width));
    apr_table_setn(params,"HEIGHT",apr_psprintf(ctx->pool,"%d",map->height));
    apr_table_setn(params,"FORMAT","image/png");
    apr_table_setn(params,"SRS",map->grid_link->grid->srs);
 
    apr_table_overlap(params,wms->getmap_params,0);
    if(map->dimensions && !apr_is_empty_table(map->dimensions)) {
       const apr_array_header_t *elts = apr_table_elts(map->dimensions);
       int i;
       for(i=0;i<elts->nelts;i++) {
          apr_table_entry_t entry = APR_ARRAY_IDX(elts,i,apr_table_entry_t);
          apr_table_setn(params,entry.key,entry.val);
       }
 
    }      
    map->encoded_data = mapcache_buffer_create(30000,ctx->pool);
    mapcache_http_do_request_with_params(ctx,wms->http,params,map->encoded_data,NULL,NULL);
    GC_CHECK_ERROR(ctx);
 
    if(!mapcache_imageio_is_valid_format(ctx,map->encoded_data)) {
       char *returned_data = apr_pstrndup(ctx->pool,(char*)map->encoded_data->buf,map->encoded_data->size);
       ctx->set_error(ctx, 502, "wms request for tileset %s returned an unsupported format:\n%s",
             map->tileset->name, returned_data);
    }
}

void _mapcache_source_wms_query(mapcache_context *ctx, mapcache_feature_info *fi) {
    mapcache_map *map = (mapcache_map*)fi;
    mapcache_source_wms *wms = (mapcache_source_wms*)map->tileset->source;
    
    apr_table_t *params = apr_table_clone(ctx->pool,wms->wms_default_params);
    apr_table_overlap(params,wms->getmap_params,0);
    apr_table_setn(params,"BBOX",apr_psprintf(ctx->pool,"%f,%f,%f,%f",
             map->extent[0],map->extent[1],map->extent[2],map->extent[3]));
    apr_table_setn(params,"REQUEST","GetFeatureInfo");
    apr_table_setn(params,"WIDTH",apr_psprintf(ctx->pool,"%d",map->width));
    apr_table_setn(params,"HEIGHT",apr_psprintf(ctx->pool,"%d",map->height));
    apr_table_setn(params,"SRS",map->grid_link->grid->srs);
    apr_table_setn(params,"X",apr_psprintf(ctx->pool,"%d",fi->i));
    apr_table_setn(params,"Y",apr_psprintf(ctx->pool,"%d",fi->j));
    apr_table_setn(params,"INFO_FORMAT",fi->format);
 
    apr_table_overlap(params,wms->getfeatureinfo_params,0);
    if(map->dimensions && !apr_is_empty_table(map->dimensions)) {
       const apr_array_header_t *elts = apr_table_elts(map->dimensions);
       int i;
       for(i=0;i<elts->nelts;i++) {
          apr_table_entry_t entry = APR_ARRAY_IDX(elts,i,apr_table_entry_t);
          apr_table_setn(params,entry.key,entry.val);
       }
 
    }      

    fi->data = mapcache_buffer_create(30000,ctx->pool);
    mapcache_http_do_request_with_params(ctx,wms->http,params,fi->data,NULL,NULL);
    GC_CHECK_ERROR(ctx);
   
}

/**
 * \private \memberof mapcache_source_wms
 * \sa mapcache_source::configuration_parse()
 */
void _mapcache_source_wms_configuration_parse_xml(mapcache_context *ctx, ezxml_t node, mapcache_source *source) {
   ezxml_t cur_node;
   mapcache_source_wms *src = (mapcache_source_wms*)source;


   if ((cur_node = ezxml_child(node,"getmap")) != NULL){
      ezxml_t gm_node;
      if ((gm_node = ezxml_child(cur_node,"params")) != NULL) {
         for(gm_node = gm_node->child; gm_node; gm_node = gm_node->sibling) {
            apr_table_set(src->getmap_params, gm_node->name, gm_node->txt);
         }
      } else {
         ctx->set_error(ctx,400,"wms source %s <getmap> has no <params> block (should contain at least <LAYERS> child)",source->name);
         return;
      }
   } else {
      ctx->set_error(ctx,400,"wms source %s has no <getmap> block",source->name);
      return;
   }
   if ((cur_node = ezxml_child(node,"getfeatureinfo")) != NULL){
      ezxml_t fi_node;
      if ((fi_node = ezxml_child(cur_node,"info_formats")) != NULL) {
         source->info_formats = apr_array_make(ctx->pool,3,sizeof(char*));
         char *iformats = apr_pstrdup(ctx->pool,fi_node->txt);
         char *key,*last;
         for (key = apr_strtok(iformats, "," , &last); key != NULL;
               key = apr_strtok(NULL, ",", &last)) {
            APR_ARRAY_PUSH(source->info_formats,char*) = key;
         }
      } else {
         ctx->set_error(ctx,400,"wms source %s <getfeatureinfo> has no <info_formats> tag",source->name);
         return;
      }
      if ((fi_node = ezxml_child(cur_node,"params")) != NULL) {
         for(fi_node = fi_node->child; fi_node; fi_node = fi_node->sibling) {
            apr_table_set(src->getfeatureinfo_params, fi_node->name, fi_node->txt);
         }
      } else {
         ctx->set_error(ctx,400,"wms source %s <getfeatureinfo> has no <params> block (should contain at least <QUERY_LAYERS> child)",source->name);
         return;
      }
   }
   if ((cur_node = ezxml_child(node,"http")) != NULL) {
      src->http = mapcache_http_configuration_parse_xml(ctx,cur_node);
   }
}

/**
 * \private \memberof mapcache_source_wms
 * \sa mapcache_source::configuration_check()
 */
void _mapcache_source_wms_configuration_check(mapcache_context *ctx, mapcache_source *source) {
   mapcache_source_wms *src = (mapcache_source_wms*)source;
   /* check all required parameters are configured */
   if(!src->http) {
      ctx->set_error(ctx, 400, "wms source %s has no <http> request configured",source->name);
   }
   if(!apr_table_get(src->getmap_params,"LAYERS")) {
      ctx->set_error(ctx, 400, "wms source %s has no LAYERS", source->name);
   }
   if(source->info_formats) {
      if(!apr_table_get(src->getfeatureinfo_params,"QUERY_LAYERS")) {
         ctx->set_error(ctx, 400, "wms source %s has no QUERY_LAYERS", source->name);
      }
   }
}

mapcache_source* mapcache_source_wms_create(mapcache_context *ctx) {
   mapcache_source_wms *source = apr_pcalloc(ctx->pool, sizeof(mapcache_source_wms));
   if(!source) {
      ctx->set_error(ctx, 500, "failed to allocate wms source");
      return NULL;
   }
   mapcache_source_init(ctx, &(source->source));
   source->source.type = MAPCACHE_SOURCE_WMS;
   source->source.render_map = _mapcache_source_wms_render_map;
   source->source.configuration_check = _mapcache_source_wms_configuration_check;
   source->source.configuration_parse_xml = _mapcache_source_wms_configuration_parse_xml;
   source->source.query_info = _mapcache_source_wms_query;
   source->wms_default_params = apr_table_make(ctx->pool,4);;
   source->getmap_params = apr_table_make(ctx->pool,4);
   source->getfeatureinfo_params = apr_table_make(ctx->pool,4);
   apr_table_add(source->wms_default_params,"VERSION","1.1.1");
   apr_table_add(source->wms_default_params,"REQUEST","GetMap");
   apr_table_add(source->wms_default_params,"SERVICE","WMS");
   apr_table_add(source->wms_default_params,"STYLES","");
   return (mapcache_source*)source;
}



/* vim: ai ts=3 sts=3 et sw=3
*/

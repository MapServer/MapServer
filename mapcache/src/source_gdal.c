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

#ifdef USE_GDAL
/**
 * \private \memberof geocache_source_gdal
 * \sa geocache_source::render_metatile()
 */
void _geocache_source_gdal_render_metatile(geocache_context *ctx, geocache_metatile *tile) {
   geocache_source_gdal *gdal = (geocache_source_gdal*)tile->tile.tileset->source;
        
   tile->tile.data = geocache_buffer_create(30000,ctx->pool);
   GC_CHECK_ERROR(ctx);
   
   if(!geocache_imageio_is_valid_format(ctx,tile->tile.data)) {
      char *returned_data = apr_pstrndup(ctx->pool,(char*)tile->tile.data->buf,tile->tile.data->size);
      ctx->set_error(ctx, GEOCACHE_SOURCE_GDAL_ERROR, "gdal request for tileset %s: %d %d %d returned an unsupported format:\n%s",
            tile->tile.tileset->name, tile->tile.x, tile->tile.y, tile->tile.z, returned_data);
   }
}

/**
 * \private \memberof geocache_source_gdal
 * \sa geocache_source::configuration_parse()
 */
void _geocache_source_gdal_configuration_parse(geocache_context *ctx, ezxml_t node, geocache_source *source) {
   ezxml_t cur_node;
   geocache_source_gdal *src = (geocache_source_gdal*)source;

   if ((cur_node = ezxml_child(node,"data")) != NULL) {
      src->datastr = apr_pstrdup(ctx->pool,cur_node->txt);
   }

   if ((cur_node = ezxml_child(node,"gdalparams")) != NULL) {
      for(cur_node = cur_node->child; cur_node; cur_node = cur_node->sibling) {
         apr_table_set(src->gdal_params, cur_node->name, cur_node->txt);
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
   if(!strlen(src->datastr)) {
      ctx->set_error(ctx, GEOCACHE_SOURCE_GDAL_ERROR, "gdal source %s has no data",source->name);
      return;
   }
   src->poDataset = (GDALDatasetH*)GDALOpen(src->datastr,GA_ReadOnly);
   if( src->poDataset == NULL ) {
      ctx->set_error(ctx, GEOCACHE_SOURCE_GDAL_ERROR, "gdalOpen failed on data %s", src->datastr);
      return;
   }
   
}
#endif //USE_GDAL

geocache_source* geocache_source_gdal_create(geocache_context *ctx) {
#ifdef USE_GDAL
   GDALAllRegister();
   geocache_source_gdal *source = apr_pcalloc(ctx->pool, sizeof(geocache_source_gdal));
   if(!source) {
      ctx->set_error(ctx, GEOCACHE_ALLOC_ERROR, "failed to allocate gdal source");
      return NULL;
   }
   geocache_source_init(ctx, &(source->source));
   source->source.type = GEOCACHE_SOURCE_GDAL;
   source->source.render_metatile = _geocache_source_gdal_render_metatile;
   source->source.configuration_check = _geocache_source_gdal_configuration_check;
   source->source.configuration_parse = _geocache_source_gdal_configuration_parse;
   source->gdal_params = apr_table_make(ctx->pool,4);
   return (geocache_source*)source;
#else
   ctx->set_error(ctx, GEOCACHE_SOURCE_GDAL_ERROR, "failed to create gdal source, GDAL support is not compiled in this version");
   return NULL;
#endif
}



/* vim: ai ts=3 sts=3 et sw=3
*/

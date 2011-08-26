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

#include <gdal.h>
#include <cpl_conv.h>

#include "gdal_alg.h"
#include "cpl_string.h"
#include "ogr_srs_api.h"

/**
 * \private \memberof geocache_source_gdal
 * \sa geocache_source::render_metatile()
 */
void _geocache_source_gdal_render_metatile(geocache_context *ctx, geocache_metatile *tile) {
   geocache_source_gdal *gdal = (geocache_source_gdal*)tile->tile.tileset->source;
   char *srcSRS = "", *dstSRS;        
   geocache_buffer *data = geocache_buffer_create(0,ctx->pool);
   GC_CHECK_ERROR(ctx);
   GDALDatasetH  hDataset;

   GDALAllRegister();
   OGRSpatialReferenceH hSRS;
   CPLErrorReset();

   hSRS = OSRNewSpatialReference( NULL );
   if( OSRSetFromUserInput( hSRS, tile->tile.grid->srs ) == OGRERR_NONE )
      OSRExportToWkt( hSRS, &dstSRS );
   else
   {
      ctx->set_error(ctx,GEOCACHE_SOURCE_GDAL_ERROR,"failed to parse gdal srs %s",tile->tile.grid->srs);
      return;
   }

   OSRDestroySpatialReference( hSRS );

   hDataset = GDALOpen( gdal->datastr, GA_ReadOnly );
   if( hDataset == NULL ) {
      ctx->set_error(ctx,GEOCACHE_SOURCE_GDAL_ERROR,"GDAL failed to open %s",gdal->datastr);
      return;
   }

   /* -------------------------------------------------------------------- */
   /*      Check that there's at least one raster band                     */
   /* -------------------------------------------------------------------- */
   if ( GDALGetRasterCount(hDataset) == 0 )
   {
      ctx->set_error(ctx,GEOCACHE_SOURCE_GDAL_ERROR,"raster %s has no bands",gdal->datastr);
      return;
   }

   if( GDALGetProjectionRef( hDataset ) != NULL 
         && strlen(GDALGetProjectionRef( hDataset )) > 0 )
      srcSRS = apr_pstrdup(ctx->pool,GDALGetProjectionRef( hDataset ));

   else if( GDALGetGCPProjection( hDataset ) != NULL
         && strlen(GDALGetGCPProjection(hDataset)) > 0 
         && GDALGetGCPCount( hDataset ) > 1 )
      srcSRS = apr_pstrdup(ctx->pool,GDALGetGCPProjection( hDataset ));

   GDALDriverH hDriver = GDALGetDriverByName( "MEM" );
   GDALDatasetH hDstDS;        
   /* -------------------------------------------------------------------- */
   /*      Create a transformation object from the source to               */
   /*      destination coordinate system.                                  */
   /* -------------------------------------------------------------------- */
   void *hTransformArg = 
      GDALCreateGenImgProjTransformer( hDataset, srcSRS, 
            NULL, dstSRS, 
            TRUE, 1000.0, 0 );

   if( hTransformArg == NULL ) {
      ctx->set_error(ctx,GEOCACHE_SOURCE_GDAL_ERROR,"gdal failed to create SRS transformation object");
      return;
   }

   /* -------------------------------------------------------------------- */
   /*      Get approximate output definition.                              */
   /* -------------------------------------------------------------------- */
   int nPixels, nLines;
   double adfDstGeoTransform[6];
   if( GDALSuggestedWarpOutput( hDataset, 
            GDALGenImgProjTransform, hTransformArg, 
            adfDstGeoTransform, &nPixels, &nLines )
         != CE_None )
   {
      ctx->set_error(ctx,GEOCACHE_SOURCE_GDAL_ERROR,"gdal failed to create suggested warp output");
      return;
   }

   GDALDestroyGenImgProjTransformer( hTransformArg );
   double dfXRes = (tile->bbox[2] - tile->bbox[0]) / tile->sx;
   double dfYRes = (tile->bbox[3] - tile->bbox[1]) / tile->sy;

   adfDstGeoTransform[0] = tile->bbox[0];
   adfDstGeoTransform[3] = tile->bbox[3];
   adfDstGeoTransform[1] = dfXRes;
   adfDstGeoTransform[5] = -dfYRes;
   hDstDS = GDALCreate( hDriver, "tempd_gdal_image", tile->sx, tile->sy, 4, GDT_Byte, NULL );

   /* -------------------------------------------------------------------- */
   /*      Write out the projection definition.                            */
   /* -------------------------------------------------------------------- */
   GDALSetProjection( hDstDS, dstSRS );
   GDALSetGeoTransform( hDstDS, adfDstGeoTransform );
   char               **papszWarpOptions = NULL;
   papszWarpOptions = CSLSetNameValue( papszWarpOptions, "INIT", "0" );



   /* -------------------------------------------------------------------- */
   /*      Create a transformation object from the source to               */
   /*      destination coordinate system.                                  */
   /* -------------------------------------------------------------------- */
   GDALTransformerFunc pfnTransformer = NULL;
   void               *hGenImgProjArg=NULL, *hApproxArg=NULL;
   hTransformArg = hGenImgProjArg = 
      GDALCreateGenImgProjTransformer( hDataset, srcSRS, 
            hDstDS, dstSRS, 
            TRUE, 1000.0, 0 );

   if( hTransformArg == NULL )
      exit( 1 );

   pfnTransformer = GDALGenImgProjTransform;

   hTransformArg = hApproxArg = 
      GDALCreateApproxTransformer( GDALGenImgProjTransform, 
            hGenImgProjArg, 0.125 );
   pfnTransformer = GDALApproxTransform;

   /* -------------------------------------------------------------------- */
   /*      Now actually invoke the warper to do the work.                  */
   /* -------------------------------------------------------------------- */
   GDALSimpleImageWarp( hDataset, hDstDS, 0, NULL, 
         pfnTransformer, hTransformArg,
         GDALDummyProgress, NULL, papszWarpOptions );

   CSLDestroy( papszWarpOptions );

   if( hApproxArg != NULL )
      GDALDestroyApproxTransformer( hApproxArg );

   if( hGenImgProjArg != NULL )
      GDALDestroyGenImgProjTransformer( hGenImgProjArg );

   if(GDALGetRasterCount(hDstDS) != 4) {
      ctx->set_error(ctx,GEOCACHE_SOURCE_GDAL_ERROR,"gdal did not create a 4 band image");
      return;
   }

   GDALRasterBandH *redband, *greenband, *blueband, *alphaband;

   redband = GDALGetRasterBand(hDstDS,1);
   greenband = GDALGetRasterBand(hDstDS,2);
   blueband = GDALGetRasterBand(hDstDS,3);
   alphaband = GDALGetRasterBand(hDstDS,4);

   unsigned char *rasterdata = apr_palloc(ctx->pool,tile->sx*tile->sy*4);
   data->buf = rasterdata;
   data->avail = tile->sx*tile->sy*4;
   data->size = tile->sx*tile->sy*4;

   GDALRasterIO(redband,GF_Read,0,0,tile->sx,tile->sy,(void*)(rasterdata),tile->sx,tile->sy,GDT_Byte,4,4*tile->sx);
   GDALRasterIO(greenband,GF_Read,0,0,tile->sx,tile->sy,(void*)(rasterdata+1),tile->sx,tile->sy,GDT_Byte,4,4*tile->sx);
   GDALRasterIO(blueband,GF_Read,0,0,tile->sx,tile->sy,(void*)(rasterdata+2),tile->sx,tile->sy,GDT_Byte,4,4*tile->sx);
   if(GDALGetRasterCount(hDataset)==4)
      GDALRasterIO(alphaband,GF_Read,0,0,tile->sx,tile->sy,(void*)(rasterdata+3),tile->sx,tile->sy,GDT_Byte,4,4*tile->sx);
   else {
      unsigned char *alphaptr;
      int i;
      for(alphaptr = rasterdata+3, i=0; i<tile->sx*tile->sy; i++, alphaptr+=4) {
         *alphaptr = 255;
      }
   }

   tile->imdata = geocache_image_create(ctx);
   tile->imdata->w = tile->sx;
   tile->imdata->h = tile->sy;
   tile->imdata->stride = tile->sx * 4;
   tile->imdata->data = rasterdata;


   GDALClose( hDstDS );
   GDALClose( hDataset);
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

/**********************************************************************
 * $Id: mapuv.c 12629 2011-10-06 18:06:34Z aboudreault $
 *
 * Project:  MapServer
 * Purpose:  UV Layer
 * Author:   Alan Boudreault (aboudreault@mapgears.com)
 *
 **********************************************************************
 * Copyright (c) 2011, Alan Boudreault, MapGears
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "mapserver.h"

#include <assert.h>
#include <math.h>
#include "mapows.h"
#include "mapresample.h"
#include "mapthread.h"

#define MSUVRASTER_NUMITEMS        6
#define MSUVRASTER_ANGLE    "uv_angle"
#define MSUVRASTER_ANGLEINDEX   -100
#define MSUVRASTER_MINUS_ANGLE    "uv_minus_angle"
#define MSUVRASTER_MINUSANGLEINDEX   -101
#define MSUVRASTER_LENGTH    "uv_length"
#define MSUVRASTER_LENGTHINDEX   -102
#define MSUVRASTER_LENGTH_2    "uv_length_2"
#define MSUVRASTER_LENGTH2INDEX   -103
#define MSUVRASTER_U    "u"
#define MSUVRASTER_UINDEX   -104
#define MSUVRASTER_V    "v"
#define MSUVRASTER_VINDEX   -105

#define RQM_UNKNOWN               0
#define RQM_ENTRY_PER_PIXEL       1
#define RQM_HIST_ON_CLASS         2
#define RQM_HIST_ON_VALUE         3

typedef struct {

  /* query cache results */
  int query_results;
  /* int query_alloc_max;
  int query_request_max;
  int query_result_hard_max;
  int raster_query_mode; */
  int band_count;

  int refcount;

  /* query bound in force
     shapeObj *searchshape;*/

  /* Only nearest result to this point.
  int      range_mode; MS_QUERY_SINGLE, MS_QUERY_MULTIPLE or -1 (skip test)
  double   range_dist;
  pointObj target_point;*/

  /* double   shape_tolerance; */

  float **u; /* u values */
  float **v; /* v values */
  int width;
  int height;
  rectObj extent;
  int     next_shape;
  int x, y; /* used internally in msUVRasterLayerNextShape() */

  mapObj* mapToUseForWhichShapes; /* set if the map->extent and map->projection are valid in msUVRASTERLayerWhichShapes() */

} uvRasterLayerInfo;

void msUVRASTERLayerUseMapExtentAndProjectionForNextWhichShapes(layerObj* layer,
                                                                mapObj* map)
{
  uvRasterLayerInfo *uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;
  uvlinfo->mapToUseForWhichShapes = map;
}

static int msUVRASTERLayerInitItemInfo(layerObj *layer)
{
  uvRasterLayerInfo *uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;
  int   i;
  int *itemindexes;
  int failed=0;

  if (layer->numitems == 0)
    return MS_SUCCESS;

  if( uvlinfo == NULL ) {
    msSetError(MS_MISCERR, "Assertion failed: GDAL layer not opened!!!",
               "msUVRASTERLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  if (layer->iteminfo)
    free(layer->iteminfo);

  if((layer->iteminfo = (int *)malloc(sizeof(int)*layer->numitems))== NULL) {
    msSetError(MS_MEMERR, NULL, "msUVRASTERLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  itemindexes = (int*)layer->iteminfo;
  for(i=0; i<layer->numitems; i++) {
    /* Special case for handling text string and angle coming from */
    /* OGR style strings.  We use special attribute snames. */
    if (EQUAL(layer->items[i], MSUVRASTER_ANGLE))
      itemindexes[i] = MSUVRASTER_ANGLEINDEX;
    else if (EQUAL(layer->items[i], MSUVRASTER_MINUS_ANGLE))
      itemindexes[i] = MSUVRASTER_MINUSANGLEINDEX;
    else if (EQUAL(layer->items[i], MSUVRASTER_LENGTH))
      itemindexes[i] = MSUVRASTER_LENGTHINDEX;
    else if (EQUAL(layer->items[i], MSUVRASTER_LENGTH_2))
      itemindexes[i] = MSUVRASTER_LENGTH2INDEX;
    else if (EQUAL(layer->items[i], MSUVRASTER_U))
      itemindexes[i] = MSUVRASTER_UINDEX;
    else if (EQUAL(layer->items[i], MSUVRASTER_V))
      itemindexes[i] = MSUVRASTER_VINDEX;
    else {
      itemindexes[i] = -1;
      msSetError(MS_OGRERR,
                 "Invalid Field name: %s",
                 "msUVRASTERLayerInitItemInfo()",
                 layer->items[i]);
      failed=1;
    }
  }

  return failed ? (MS_FAILURE) : (MS_SUCCESS);
}


void msUVRASTERLayerFreeItemInfo(layerObj *layer)
{
  if (layer->iteminfo)
    free(layer->iteminfo);
  layer->iteminfo = NULL;
}

static void msUVRasterLayerInfoInitialize(layerObj *layer)
{
  uvRasterLayerInfo *uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;

  if( uvlinfo != NULL )
    return;

  uvlinfo = (uvRasterLayerInfo *) msSmallCalloc(1,sizeof(uvRasterLayerInfo));
  layer->layerinfo = uvlinfo;

  uvlinfo->band_count = -1;
  /* uvlinfo->raster_query_mode = RQM_ENTRY_PER_PIXEL; */
  /* uvlinfo->range_mode = -1; /\* inactive *\/ */
  /* uvlinfo->refcount = 0; */
  /* uvlinfo->shape_tolerance = 0.0; */
  uvlinfo->u = NULL;
  uvlinfo->v = NULL;
  uvlinfo->width = 0;
  uvlinfo->height = 0;

  /* Set attribute type to Real, unless the user has explicitly set */
  /* something else. */
  {
      const char* const items[] = {
          MSUVRASTER_ANGLE,
          MSUVRASTER_MINUS_ANGLE,
          MSUVRASTER_LENGTH,
          MSUVRASTER_LENGTH_2,
          MSUVRASTER_U,
          MSUVRASTER_V,
      };
      size_t i;
      for( i = 0; i < sizeof(items)/sizeof(items[0]); ++i ) {
          char szTmp[100];
          snprintf(szTmp, sizeof(szTmp), "%s_type", items[i]);
          if (msOWSLookupMetadata(&(layer->metadata), "OFG", szTmp) == NULL) {
              snprintf(szTmp, sizeof(szTmp), "gml_%s_type", items[i]);
              msInsertHashTable(&(layer->metadata), szTmp, "Real");
          }
      }
  }

  /* uvlinfo->query_result_hard_max = 1000000; */

  /* if( CSLFetchNameValue( layer->processing, "RASTER_QUERY_MAX_RESULT" )  */
  /*     != NULL ) */
  /* { */
  /*     uvlinfo->query_result_hard_max =  */
  /*         atoi(CSLFetchNameValue( layer->processing, "RASTER_QUERY_MAX_RESULT" )); */
  /* } */
}

static void msUVRasterLayerInfoFree( layerObj *layer )

{
  uvRasterLayerInfo *uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;
  int i;

  if( uvlinfo == NULL )
    return;

  if (uvlinfo->u) {
    for (i=0; i<uvlinfo->width; ++i) {
      free(uvlinfo->u[i]);
    }
    free(uvlinfo->u);
  }

  if (uvlinfo->v) {
    for (i=0; i<uvlinfo->width; ++i) {
      free(uvlinfo->v[i]);
    }
    free(uvlinfo->v);
  }

  free( uvlinfo );

  layer->layerinfo = NULL;
}

int msUVRASTERLayerOpen(layerObj *layer)
{
  /* If we don't have info, initialize an empty one now */
  if( layer->layerinfo == NULL )
    msUVRasterLayerInfoInitialize( layer );
  if( layer->layerinfo == NULL )
    return MS_FAILURE;

  uvRasterLayerInfo* uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;

  uvlinfo->refcount = uvlinfo->refcount + 1;

  return MS_SUCCESS;
}

int msUVRASTERLayerIsOpen(layerObj *layer)
{
  if (layer->layerinfo)
    return MS_TRUE;
  return MS_FALSE;
}


int msUVRASTERLayerClose(layerObj *layer)
{
  uvRasterLayerInfo *uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;

  if( uvlinfo != NULL ) {
    uvlinfo->refcount--;

    if( uvlinfo->refcount < 1 )
      msUVRasterLayerInfoFree( layer );
  }
  return MS_SUCCESS;
}

int msUVRASTERLayerGetItems(layerObj *layer)
{
  uvRasterLayerInfo *uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;

  if( uvlinfo == NULL )
    return MS_FAILURE;

  layer->numitems = 0;
  layer->items = (char **) msSmallCalloc(sizeof(char *),10);;

  layer->items[layer->numitems++] = msStrdup(MSUVRASTER_ANGLE);
  layer->items[layer->numitems++] = msStrdup(MSUVRASTER_MINUS_ANGLE);
  layer->items[layer->numitems++] = msStrdup(MSUVRASTER_LENGTH);
  layer->items[layer->numitems++] = msStrdup(MSUVRASTER_LENGTH_2);
  layer->items[layer->numitems++] = msStrdup(MSUVRASTER_U);
  layer->items[layer->numitems++] = msStrdup(MSUVRASTER_V);
  layer->items[layer->numitems] = NULL;

  return msUVRASTERLayerInitItemInfo(layer);
}

/**********************************************************************
 *                     msUVRASTERGetValues()
 *
 * Special attribute names are used to return some UV params: uv_angle,
 * uv_length, u and v.
 **********************************************************************/
static char **msUVRASTERGetValues(layerObj *layer, float *u, float *v)
{
  char **values;
  int i = 0;
  char tmp[100];
  float size_scale;
  int *itemindexes = (int*)layer->iteminfo;

  if(layer->numitems == 0)
    return(NULL);

  if(!layer->iteminfo) { /* Should not happen... but just in case! */
    if (msUVRASTERLayerInitItemInfo(layer) != MS_SUCCESS)
      return NULL;
    itemindexes = (int*)layer->iteminfo; /* reassign after malloc */
  }

  if((values = (char **)malloc(sizeof(char *)*layer->numitems)) == NULL) {
    msSetError(MS_MEMERR, NULL, "msUVRASTERGetValues()");
    return(NULL);
  }

  /* -------------------------------------------------------------------- */
  /*    Determine desired size_scale.  Default to 1 if not otherwise set  */
  /* -------------------------------------------------------------------- */
  size_scale = 1;
  if( CSLFetchNameValue( layer->processing, "UV_SIZE_SCALE" ) != NULL ) {
    size_scale =
      atof(CSLFetchNameValue( layer->processing, "UV_SIZE_SCALE" ));
  }

  for(i=0; i<layer->numitems; i++) {
    if (itemindexes[i] == MSUVRASTER_ANGLEINDEX) {
      snprintf(tmp, 100, "%f", (atan2((double)*v, (double)*u) * 180 / MS_PI));
      values[i] = msStrdup(tmp);
    } else if (itemindexes[i] == MSUVRASTER_MINUSANGLEINDEX) {
      double minus_angle;
      minus_angle = (atan2((double)*v, (double)*u) * 180 / MS_PI)+180;
      if (minus_angle >= 360)
        minus_angle -= 360;
      snprintf(tmp, 100, "%f", minus_angle);
      values[i] = msStrdup(tmp);
    } else if ( (itemindexes[i] == MSUVRASTER_LENGTHINDEX) ||
                (itemindexes[i] == MSUVRASTER_LENGTH2INDEX) ) {
      float length = sqrt((*u**u)+(*v**v))*size_scale;

      if (itemindexes[i] == MSUVRASTER_LENGTHINDEX)
        snprintf(tmp, 100, "%f", length);
      else
        snprintf(tmp, 100, "%f", length/2);

      values[i] = msStrdup(tmp);
    } else if (itemindexes[i] == MSUVRASTER_UINDEX) {
      snprintf(tmp, 100, "%f",*u);
      values[i] = msStrdup(tmp);
    } else if (itemindexes[i] == MSUVRASTER_VINDEX) {
      snprintf(tmp, 100, "%f",*v);
      values[i] = msStrdup(tmp);
    } else {
      values[i] = NULL;
      }
  }

  return values;
}

rectObj msUVRASTERGetSearchRect( layerObj* layer, mapObj* map )
{
    rectObj searchrect = map->extent;
    int bDone = MS_FALSE;

    /* For UVRaster, it is important that the searchrect is not too large */
    /* to avoid insufficient intermediate raster resolution, which could */
    /* happen if we use the default code path, given potential reprojection */
    /* issues when using a map extent that is not in the validity area of */
    /* the layer projection. */
    if( !layer->projection.gt.need_geotransform &&
        !(msProjIsGeographicCRS(&(map->projection)) &&
        msProjIsGeographicCRS(&(layer->projection))) ) {
      rectObj layer_ori_extent;

      if( msLayerGetExtent(layer, &layer_ori_extent) == MS_SUCCESS ) {
        projectionObj map_proj;

        double map_extent_minx = map->extent.minx;
        double map_extent_miny = map->extent.miny;
        double map_extent_maxx = map->extent.maxx;
        double map_extent_maxy = map->extent.maxy;
        rectObj layer_extent = layer_ori_extent;

        /* Create a variant of map->projection without geotransform for */
        /* conveniency */
        msInitProjection(&map_proj);
        msCopyProjection(&map_proj, &map->projection);
        map_proj.gt.need_geotransform = MS_FALSE;
        if( map->projection.gt.need_geotransform ) {
            map_extent_minx = map->projection.gt.geotransform[0]
                + map->projection.gt.geotransform[1] * map->extent.minx
                + map->projection.gt.geotransform[2] * map->extent.miny;
            map_extent_miny = map->projection.gt.geotransform[3]
                + map->projection.gt.geotransform[4] * map->extent.minx
                + map->projection.gt.geotransform[5] * map->extent.miny;
            map_extent_maxx = map->projection.gt.geotransform[0]
                + map->projection.gt.geotransform[1] * map->extent.maxx
                + map->projection.gt.geotransform[2] * map->extent.maxy;
            map_extent_maxy = map->projection.gt.geotransform[3]
                + map->projection.gt.geotransform[4] * map->extent.maxx
                + map->projection.gt.geotransform[5] * map->extent.maxy;
        }

        /* Reproject layer extent to map projection */
        msProjectRect(&layer->projection, &map_proj, &layer_extent);

        if( layer_extent.minx <= map_extent_minx &&
            layer_extent.miny <= map_extent_miny &&
            layer_extent.maxx >= map_extent_maxx &&
            layer_extent.maxy >= map_extent_maxy ) {
            /* do nothing special if area to map is inside layer extent */
        }
        else {
            if( layer_extent.minx >= map_extent_minx &&
                layer_extent.maxx <= map_extent_maxx &&
                layer_extent.miny >= map_extent_miny &&
                layer_extent.maxy <= map_extent_maxy ) {
                /* if the area to map is larger than the layer extent, then */
                /* use full layer extent and add some margin to reflect the */
                /* proportion of the useful area over the requested bbox */
                double extra_x =
                (map_extent_maxx - map_extent_minx) /
                    (layer_extent.maxx - layer_extent.minx) *
                    (layer_ori_extent.maxx -  layer_ori_extent.minx);
                double extra_y =
                    (map_extent_maxy - map_extent_miny) /
                    (layer_extent.maxy - layer_extent.miny) *
                    (layer_ori_extent.maxy -  layer_ori_extent.miny);
                searchrect.minx = layer_ori_extent.minx - extra_x / 2;
                searchrect.maxx = layer_ori_extent.maxx + extra_x / 2;
                searchrect.miny = layer_ori_extent.miny - extra_y / 2;
                searchrect.maxy = layer_ori_extent.maxy + extra_y / 2;
            }
            else
            {
                /* otherwise clip the map extent with the reprojected layer */
                /* extent */
                searchrect.minx = MS_MAX( map_extent_minx, layer_extent.minx );
                searchrect.maxx = MS_MIN( map_extent_maxx, layer_extent.maxx );
                searchrect.miny = MS_MAX( map_extent_miny, layer_extent.miny );
                searchrect.maxy = MS_MIN( map_extent_maxy, layer_extent.maxy );
                /* and reproject into the layer projection */
                msProjectRect(&map_proj, &layer->projection, &searchrect);
            }
            bDone = MS_TRUE;
        }

        msFreeProjection(&map_proj);
      }
    }

    if( !bDone )
      msProjectRect(&map->projection, &layer->projection, &searchrect); /* project the searchrect to source coords */

    return searchrect;
}

int msUVRASTERLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery)
{
  uvRasterLayerInfo *uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;
  imageObj *image_tmp;
  outputFormatObj *outputformat = NULL;
  mapObj *map_tmp;
  double map_cellsize;
  unsigned int spacing;
  int width, height, u_src_off, v_src_off, i, x, y;
  char   **alteredProcessing = NULL, *saved_layer_mask;
  char **savedProcessing = NULL;
  int bHasLonWrap = MS_FALSE;
  double dfLonWrap = 0.0;
  rectObj oldLayerExtent = {0};
  char* oldLayerData = NULL;
  projectionObj oldLayerProjection={0};
  int ret;

  if (layer->debug)
    msDebug("Entering msUVRASTERLayerWhichShapes().\n");

  if( uvlinfo == NULL )
    return MS_FAILURE;

  if( CSLFetchNameValue( layer->processing, "BANDS" ) == NULL ) {
    msSetError( MS_MISCERR, "BANDS processing option is required for UV layer. You have to specified 2 bands.",
                "msUVRASTERLayerWhichShapes()" );
    return MS_FAILURE;
  }
  
  /*
  ** Allocate mapObj structure
  */
  map_tmp = (mapObj *)msSmallCalloc(sizeof(mapObj),1);
  if(initMap(map_tmp) == -1) { /* initialize this map */
    msFree(map_tmp);
    return(MS_FAILURE);
  }

  /* -------------------------------------------------------------------- */
  /*      Determine desired spacing.  Default to 32 if not otherwise set  */
  /* -------------------------------------------------------------------- */
  spacing = 32;
  if( CSLFetchNameValue( layer->processing, "UV_SPACING" ) != NULL ) {
    spacing =
      atoi(CSLFetchNameValue( layer->processing, "UV_SPACING" ));
    if( spacing == 0 )
        spacing = 32;
  }

  width = (int)(layer->map->width/spacing);
  height = (int)(layer->map->height/spacing);

  /* Initialize our dummy map */
  MS_INIT_COLOR(map_tmp->imagecolor, 255,255,255,255);
  map_tmp->resolution = layer->map->resolution;
  map_tmp->defresolution = layer->map->defresolution;

  outputformat = (outputFormatObj *) msSmallCalloc(1,sizeof(outputFormatObj));
  outputformat->bands = uvlinfo->band_count = 2;
  outputformat->name = NULL;
  outputformat->driver = NULL;
  outputformat->refcount = 0;
  outputformat->vtable = NULL;
  outputformat->device = NULL;
  outputformat->renderer = MS_RENDER_WITH_RAWDATA;
  outputformat->imagemode = MS_IMAGEMODE_FLOAT32;
  msAppendOutputFormat(map_tmp, outputformat);
  
  msCopyHashTable(&map_tmp->configoptions, &layer->map->configoptions);
  map_tmp->mappath = msStrdup(layer->map->mappath);
  map_tmp->shapepath = msStrdup(layer->map->shapepath);
  map_tmp->gt.rotation_angle = 0.0;

  /* Custom msCopyProjection() that removes lon_wrap parameter */
  {
    int i;

    map_tmp->projection.numargs = 0;
    map_tmp->projection.gt = layer->projection.gt;
    map_tmp->projection.automatic = layer->projection.automatic;

    for (i = 0; i < layer->projection.numargs; i++) {
      if( strncmp(layer->projection.args[i], "lon_wrap=",
                  strlen("lon_wrap=")) == 0 ) {
        bHasLonWrap = MS_TRUE;
        dfLonWrap = atof( layer->projection.args[i] + strlen("lon_wrap=") );
      }
      else {
        map_tmp->projection.args[map_tmp->projection.numargs ++] =
            msStrdup(layer->projection.args[i]);
      }
    }
    if (map_tmp->projection.numargs != 0) {
      msProcessProjection(&(map_tmp->projection));
    }

    map_tmp->projection.wellknownprojection = layer->projection.wellknownprojection;
  }

  /* Very special case to improve quality for rasters referenced from lon=0 to 360 */
  /* We create a temporary VRT that swiches the 2 hemispheres, and then we */
  /* modify the georeferncing to be in the more standard [-180, 180] range */
  /* and we adjust the layer->data, extent and projection accordingly */
  if( layer->tileindex == NULL &&
      uvlinfo->mapToUseForWhichShapes && bHasLonWrap && dfLonWrap == 180.0 )
  {
      rectObj layerExtent;
      msLayerGetExtent(layer, &layerExtent);
      if( layerExtent.minx == 0 && layerExtent.maxx == 360 )
      {
          GDALDatasetH hDS = NULL;
          char* decrypted_path;

          if( strncmp(layer->data, "<VRTDataset", strlen("<VRTDataset")) == 0 )
          {
            decrypted_path = msStrdup(layer->data);
          }
          else
          {
            char szPath[MS_MAXPATHLEN];
            msTryBuildPath3(szPath, layer->map->mappath,
                            layer->map->shapepath, layer->data);
            decrypted_path = msDecryptStringTokens( layer->map, szPath );
          }

          if( decrypted_path )
          {
              char** connectionoptions;
              GDALAllRegister();
              connectionoptions = msGetStringListFromHashTable(&(layer->connectionoptions));
              hDS = GDALOpenEx(decrypted_path,
                                        GDAL_OF_RASTER,
                                        NULL,
                                        (const char* const*)connectionoptions,
                                        NULL);
              CSLDestroy(connectionoptions);
          }
          if( hDS != NULL )
          {
              int iBand;
              int nXSize = GDALGetRasterXSize( hDS );
              int nYSize = GDALGetRasterYSize( hDS );
              int nBands = GDALGetRasterCount( hDS );
              int nMaxLen = 100 + nBands * (800 + 2 * strlen(decrypted_path));
              int nOffset = 0;
              char* pszInlineVRT = msSmallMalloc( nMaxLen );

              snprintf(pszInlineVRT, nMaxLen,
                "<VRTDataset rasterXSize=\"%d\" rasterYSize=\"%d\">",
                nXSize, nYSize);
              nOffset = strlen(pszInlineVRT);
              for( iBand = 1; iBand <= nBands; iBand++ )
              {
                  const char* pszDataType = "Byte";
                  switch( GDALGetRasterDataType(GDALGetRasterBand(hDS, iBand)) )
                  {
                      case GDT_Byte: pszDataType = "Byte"; break;
                      case GDT_Int16: pszDataType = "Int16"; break;
                      case GDT_UInt16: pszDataType = "UInt16"; break;
                      case GDT_Int32: pszDataType = "Int32"; break;
                      case GDT_UInt32: pszDataType = "UInt32"; break;
                      case GDT_Float32: pszDataType = "Float32"; break;
                      case GDT_Float64: pszDataType = "Float64"; break;
                      default: break;
                  }

                  snprintf( pszInlineVRT + nOffset, nMaxLen - nOffset,
                    "    <VRTRasterBand dataType=\"%s\" band=\"%d\">"
                    "        <SimpleSource>"
                    "            <SourceFilename relativeToVrt=\"1\"><![CDATA[%s]]></SourceFilename>"
                    "            <SourceBand>%d</SourceBand>"
                    "            <SrcRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" ySize=\"%d\"/>"
                    "            <DstRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" ySize=\"%d\"/>"
                    "        </SimpleSource>"
                    "        <SimpleSource>"
                    "            <SourceFilename relativeToVrt=\"1\"><![CDATA[%s]]></SourceFilename>"
                    "            <SourceBand>%d</SourceBand>"
                    "            <SrcRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" ySize=\"%d\"/>"
                    "            <DstRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" ySize=\"%d\"/>"
                    "        </SimpleSource>"
                    "    </VRTRasterBand>",
                    pszDataType, iBand,
                    decrypted_path, iBand,
                    nXSize / 2, 0, nXSize - nXSize / 2, nYSize,
                    0,          0, nXSize - nXSize / 2, nYSize,
                    decrypted_path, iBand,
                    0,                   0, nXSize / 2, nYSize,
                    nXSize - nXSize / 2, 0, nXSize / 2, nYSize );

                  nOffset += strlen(pszInlineVRT + nOffset);
              }
              snprintf(pszInlineVRT + nOffset, nMaxLen - nOffset,
                "</VRTDataset>");

              oldLayerExtent = layer->extent;
              oldLayerData = layer->data;
              oldLayerProjection = layer->projection;
              layer->extent.minx = -180;
              layer->extent.maxx = 180;
              layer->data = pszInlineVRT;
              layer->projection = map_tmp->projection;


              /* map_tmp->projection is actually layer->projection without lon_wrap */
              rect = uvlinfo->mapToUseForWhichShapes->extent;
              msProjectRect(&uvlinfo->mapToUseForWhichShapes->projection,
                            &map_tmp->projection, &rect);
              bHasLonWrap = MS_FALSE;

              GDALClose(hDS);
          }
          msFree( decrypted_path );
      }
  }

  if( isQuery )
  {
      /* For query mode, use layer->map->extent reprojected rather than */
      /* the provided rect. Generic query code will filter returned features. */
      rect = msUVRASTERGetSearchRect(layer, layer->map);
  }

  map_cellsize = MS_MAX(MS_CELLSIZE(rect.minx, rect.maxx,layer->map->width),
                        MS_CELLSIZE(rect.miny,rect.maxy,layer->map->height));
  map_tmp->cellsize = map_cellsize*spacing;
  map_tmp->extent.minx = rect.minx-(0.5*map_cellsize)+(0.5*map_tmp->cellsize);
  map_tmp->extent.miny = rect.miny-(0.5*map_cellsize)+(0.5*map_tmp->cellsize);
  map_tmp->extent.maxx = map_tmp->extent.minx+((width-1)*map_tmp->cellsize);
  map_tmp->extent.maxy = map_tmp->extent.miny+((height-1)*map_tmp->cellsize);

  if( bHasLonWrap && dfLonWrap == 180.0) {
    if( map_tmp->extent.minx >= 180 ) {
      /* Request on the right half of the shifted raster (= western hemisphere) */
      map_tmp->extent.minx -= 360;
      map_tmp->extent.maxx -= 360;
    }
    else if( map_tmp->extent.maxx >= 180.0 ) {
      /* Request spanning on the 2 hemispheres => drawing whole planet */
      /* Take only into account vertical resolution, as horizontal one */
      /* will be unreliable (assuming square pixels...) */
      map_cellsize = MS_CELLSIZE(rect.miny,rect.maxy,layer->map->height);
      map_tmp->cellsize = map_cellsize*spacing;

      width = 360.0 / map_tmp->cellsize;
      map_tmp->extent.minx = -180.0+(0.5*map_tmp->cellsize);
      map_tmp->extent.maxx = 180.0-(0.5*map_tmp->cellsize);
    }
  }

  if (layer->debug)
    msDebug("msUVRASTERLayerWhichShapes(): width: %d, height: %d, cellsize: %g\n",
            width, height, map_tmp->cellsize);

  if (layer->debug == MS_DEBUGLEVEL_VVV)
    msDebug("msUVRASTERLayerWhichShapes(): extent: %g %g %g %g\n",
            map_tmp->extent.minx, map_tmp->extent.miny,
            map_tmp->extent.maxx, map_tmp->extent.maxy);

  /* important to use that function, to compute map
     geotransform, used by the resampling*/
   msMapSetSize(map_tmp, width, height);

  if (layer->debug == MS_DEBUGLEVEL_VVV)
    msDebug("msUVRASTERLayerWhichShapes(): geotransform: %g %g %g %g %g %g\n",
            map_tmp->gt.geotransform[0], map_tmp->gt.geotransform[1],
            map_tmp->gt.geotransform[2], map_tmp->gt.geotransform[3],
            map_tmp->gt.geotransform[4], map_tmp->gt.geotransform[5]);

  uvlinfo->extent = map_tmp->extent;

  image_tmp = msImageCreate(width, height, map_tmp->outputformatlist[0],
                            NULL, NULL, map_tmp->resolution, map_tmp->defresolution,
                            &(map_tmp->imagecolor));

  /* Default set to AVERAGE resampling */
  if( CSLFetchNameValue( layer->processing, "RESAMPLE" ) == NULL ) {
    alteredProcessing = CSLDuplicate( layer->processing );
    alteredProcessing =
      CSLSetNameValue( alteredProcessing, "RESAMPLE",
                       "AVERAGE");
    savedProcessing = layer->processing;
    layer->processing = alteredProcessing;
  }

  /* disable masking at this level: we don't want to apply the mask at the raster level,
   * it will be applied with the correct cellsize and image size in the vector rendering
   * phase.
   */
  saved_layer_mask = layer->mask;
  layer->mask = NULL;
  ret = msDrawRasterLayerLow(map_tmp, layer, image_tmp, NULL );

  /* restore layer attributes if we went through the above on-the-fly VRT */
  if( oldLayerData )
  {
      msFree(layer->data);
      layer->data = oldLayerData;
      layer->extent = oldLayerExtent;
      layer->projection = oldLayerProjection;
  }

  /* restore layer mask */
  layer->mask = saved_layer_mask;

  /* restore the saved processing */
  if (alteredProcessing != NULL) {
    layer->processing = savedProcessing;
    CSLDestroy(alteredProcessing);
  }

  if( ret == MS_FAILURE) {
    msSetError(MS_MISCERR, "Unable to draw raster data.", "msUVRASTERLayerWhichShapes()");

    msFreeMap(map_tmp);
    msFreeImage(image_tmp);

    return MS_FAILURE;
  }

  /* free old query arrays */
  if (uvlinfo->u) {
    for (i=0; i<uvlinfo->width; ++i) {
      free(uvlinfo->u[i]);
    }
    free(uvlinfo->u);
  }

  if (uvlinfo->v) {
    for (i=0; i<uvlinfo->width; ++i) {
      free(uvlinfo->v[i]);
    }
    free(uvlinfo->v);
  }

  /* Update our uv layer structure */
  uvlinfo->width = width;
  uvlinfo->height = height;
  uvlinfo->query_results = width*height;

  uvlinfo->u = (float **)msSmallMalloc(sizeof(float *)*width);
  uvlinfo->v = (float **)msSmallMalloc(sizeof(float *)*width);

  for (x = 0; x < width; ++x) {
    uvlinfo->u[x] = (float *)msSmallMalloc(height * sizeof(float));
    uvlinfo->v[x] = (float *)msSmallMalloc(height * sizeof(float));

    for (y = 0; y < height; ++y) {
      u_src_off = v_src_off = x + y * width;
      v_src_off += width*height;

      uvlinfo->u[x][y] = image_tmp->img.raw_float[u_src_off];
      uvlinfo->v[x][y] = image_tmp->img.raw_float[v_src_off];

      /* null vector? update the number of results  */
      if (uvlinfo->u[x][y] == 0 && uvlinfo->v[x][y] == 0)
        --uvlinfo->query_results;
    }
  }

  msFreeImage(image_tmp); /* we do not need the imageObj anymore */
  msFreeMap(map_tmp);

  uvlinfo->next_shape = 0;

  return MS_SUCCESS;
}

int msUVRASTERLayerGetShape(layerObj *layer, shapeObj *shape, resultObj *record)
{
  uvRasterLayerInfo *uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;
  lineObj line ;
  pointObj point;
  int i, j, k, x=0, y=0;
  long shapeindex = record->shapeindex;

  msFreeShape(shape);
  shape->type = MS_SHAPE_NULL;

  if( shapeindex < 0 || shapeindex >= uvlinfo->query_results ) {
    msSetError(MS_MISCERR,
               "Out of range shape index requested.  Requested %ld\n"
               "but only %d shapes available.",
               "msUVRASTERLayerGetShape()",
               shapeindex, uvlinfo->query_results );
    return MS_FAILURE;
  }

  /* loop to the next non null vector */
  k = 0;
  for (i=0, x=-1; i<uvlinfo->width && k<=shapeindex; ++i, ++x) {
    for (j=0, y=-1; j<uvlinfo->height && k<=shapeindex; ++j, ++k, ++y) {
      if (uvlinfo->u[i][j] == 0 && uvlinfo->v[i][j] == 0)
        --k;
    }
  }

  point.x = Pix2Georef(x, 0, uvlinfo->width-1,
                       uvlinfo->extent.minx, uvlinfo->extent.maxx, MS_FALSE);
  point.y = Pix2Georef(y, 0, uvlinfo->height-1,
                       uvlinfo->extent.miny, uvlinfo->extent.maxy, MS_TRUE);
  if (layer->debug == MS_DEBUGLEVEL_VVV)
    msDebug("msUVRASTERLayerWhichShapes(): shapeindex: %ld, x: %g, y: %g\n",
            shapeindex, point.x, point.y);

  point.m = 0.0;

  shape->type = MS_SHAPE_POINT;
  line.numpoints = 1;
  line.point = &point;
  msAddLine( shape, &line );
  msComputeBounds( shape );

  shape->numvalues = layer->numitems;
  shape->values = msUVRASTERGetValues(layer, &uvlinfo->u[x][y], &uvlinfo->v[x][y]);
  shape->index = shapeindex;
  shape->resultindex = shapeindex;

  return MS_SUCCESS;

}

int msUVRASTERLayerNextShape(layerObj *layer, shapeObj *shape)
{
  uvRasterLayerInfo *uvlinfo = (uvRasterLayerInfo *) layer->layerinfo;

  if( uvlinfo->next_shape < 0
      || uvlinfo->next_shape >= uvlinfo->query_results ) {
    msFreeShape(shape);
    shape->type = MS_SHAPE_NULL;
    return MS_DONE;
  } else {
    resultObj record;

    record.shapeindex = uvlinfo->next_shape++;
    record.tileindex = 0;
    record.classindex = record.resultindex = -1;

    return msUVRASTERLayerGetShape( layer, shape, &record);
  }
}

/************************************************************************/
/*                       msUVRASTERLayerGetExtent()                     */
/* Simple copy of the maprasterquery.c file. might change in the future */
/************************************************************************/

int msUVRASTERLayerGetExtent(layerObj *layer, rectObj *extent)

{
  char szPath[MS_MAXPATHLEN];
  mapObj *map = layer->map;
  shapefileObj *tileshpfile;

  if( (!layer->data || strlen(layer->data) == 0)
      && layer->tileindex == NULL) {
    /* should we be issuing a specific error about not supporting
       extents for tileindexed raster layers? */
    return MS_FAILURE;
  }

  if( map == NULL )
    return MS_FAILURE;

  /* If the layer use a tileindex, return the extent of the tileindex shapefile/referenced layer */
  if (layer->tileindex) {
    const int tilelayerindex = msGetLayerIndex(map, layer->tileindex);
    if(tilelayerindex != -1) /* does the tileindex reference another layer */
      return msLayerGetExtent(GET_LAYER(map, tilelayerindex), extent);
    else {
      tileshpfile = (shapefileObj *) malloc(sizeof(shapefileObj));
      MS_CHECK_ALLOC(tileshpfile, sizeof(shapefileObj), MS_FAILURE);

      if(msShapefileOpen(tileshpfile, "rb", msBuildPath3(szPath, map->mappath, map->shapepath, layer->tileindex), MS_TRUE) == -1)
        if(msShapefileOpen(tileshpfile, "rb", msBuildPath(szPath, map->mappath, layer->tileindex), MS_TRUE) == -1)
          return MS_FAILURE;

      *extent = tileshpfile->bounds;
      msShapefileClose(tileshpfile);
      free(tileshpfile);
      return MS_SUCCESS;
    }
  }

  msTryBuildPath3(szPath, map->mappath, map->shapepath, layer->data);
  char* decrypted_path = msDecryptStringTokens( map, szPath );
  if( !decrypted_path )
      return MS_FAILURE;

  GDALAllRegister();

  char** connectionoptions = msGetStringListFromHashTable(&(layer->connectionoptions));
  GDALDatasetH hDS = GDALOpenEx(decrypted_path,
                                GDAL_OF_RASTER,
                                NULL,
                                (const char* const*)connectionoptions,
                                NULL);
  CSLDestroy(connectionoptions);
  msFree( decrypted_path );
  if( hDS == NULL ) {
    return MS_FAILURE;
  }

  const int nXSize = GDALGetRasterXSize( hDS );
  const int nYSize = GDALGetRasterYSize( hDS );
  double adfGeoTransform[6] = {0};
  const CPLErr eErr = GDALGetGeoTransform( hDS, adfGeoTransform );
  if( eErr != CE_None ) {
    GDALClose( hDS );
    return MS_FAILURE;
  }

  /* If this appears to be an ungeoreferenced raster than flip it for
     mapservers purposes. */
  if( adfGeoTransform[5] == 1.0 && adfGeoTransform[3] == 0.0 ) {
    adfGeoTransform[5] = -1.0;
    adfGeoTransform[3] = nYSize;
  }

  extent->minx = adfGeoTransform[0];
  extent->maxy = adfGeoTransform[3];

  extent->maxx = adfGeoTransform[0] + nXSize * adfGeoTransform[1];
  extent->miny = adfGeoTransform[3] + nYSize * adfGeoTransform[5];

  return MS_SUCCESS;
}


/************************************************************************/
/*                     msUVRASTERLayerSetTimeFilter()                   */
/*                                                                      */
/*      This function is actually just used in the context of           */
/*      setting a filter on the tileindex for time based queries.       */
/*      For instance via WMS requests.  So it isn't really related      */
/*      to the "raster query" support at all.                           */
/*                                                                      */
/*      If a local shapefile tileindex is in use, we will set a         */
/*      backtics filter (shapefile compatible).  If another layer is    */
/*      being used as the tileindex then we will forward the            */
/*      SetTimeFilter call to it.  If there is no tileindex in          */
/*      place, we do nothing.                                           */
/************************************************************************/

int msUVRASTERLayerSetTimeFilter(layerObj *layer, const char *timestring,
                                 const char *timefield)
{
  int tilelayerindex;

  /* -------------------------------------------------------------------- */
  /*      If we don't have a tileindex the time filter has no effect.     */
  /* -------------------------------------------------------------------- */
  if( layer->tileindex == NULL )
    return MS_SUCCESS;

  /* -------------------------------------------------------------------- */
  /*      Find the tileindex layer.                                       */
  /* -------------------------------------------------------------------- */
  tilelayerindex = msGetLayerIndex(layer->map, layer->tileindex);

  /* -------------------------------------------------------------------- */
  /*      If we are using a local shapefile as our tileindex (that is     */
  /*      to say, the tileindex name is not of another layer), then we    */
  /*      just install a backtics style filter on the raster layer.       */
  /*      This is propogated to the "working layer" created for the       */
  /*      tileindex by code in mapraster.c.                               */
  /* -------------------------------------------------------------------- */
  if( tilelayerindex == -1 )
    return msLayerMakeBackticsTimeFilter( layer, timestring, timefield );

  /* -------------------------------------------------------------------- */
  /*      Otherwise we invoke the tileindex layers SetTimeFilter          */
  /*      method.                                                         */
  /* -------------------------------------------------------------------- */
  if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
    return MS_FAILURE;
  return msLayerSetTimeFilter( layer->GET_LAYER(map,tilelayerindex),
                               timestring, timefield );
}


/************************************************************************/
/*                msRASTERLayerInitializeVirtualTable()                 */
/************************************************************************/

int
msUVRASTERLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerInitItemInfo = msUVRASTERLayerInitItemInfo;
  layer->vtable->LayerFreeItemInfo = msUVRASTERLayerFreeItemInfo;
  layer->vtable->LayerOpen = msUVRASTERLayerOpen;
  layer->vtable->LayerIsOpen = msUVRASTERLayerIsOpen;
  layer->vtable->LayerWhichShapes = msUVRASTERLayerWhichShapes;
  layer->vtable->LayerNextShape = msUVRASTERLayerNextShape;
  layer->vtable->LayerGetShape = msUVRASTERLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */
  layer->vtable->LayerClose = msUVRASTERLayerClose;
  layer->vtable->LayerGetItems = msUVRASTERLayerGetItems;
  layer->vtable->LayerGetExtent = msUVRASTERLayerGetExtent;
  /* layer->vtable->LayerGetAutoStyle, use default */
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  /* layer->vtable->LayerCloseConnection = msUVRASTERLayerClose; */
  /* we use backtics for proper tileindex shapefile functioning */
  layer->vtable->LayerSetTimeFilter = msUVRASTERLayerSetTimeFilter;
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */

  return MS_SUCCESS;
}

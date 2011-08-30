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

#include "mapcache.h"
#include <math.h>
#include <apr_strings.h>
/*
 * allocate and initialize a new tileset
 */
mapcache_grid* mapcache_grid_create(apr_pool_t *pool) {
   mapcache_grid* grid = (mapcache_grid*)apr_pcalloc(pool, sizeof(mapcache_grid));
   grid->metadata = apr_table_make(pool,3);
   grid->srs_aliases = apr_array_make(pool,0,sizeof(char*));
   grid->unit = MAPCACHE_UNIT_METERS;
   return grid;
}


/**
 * \brief compute the extent of a given tile in the grid given its x, y, and z.
 * \returns \extent the tile's extent
 */
void mapcache_grid_get_extent(mapcache_context *ctx, mapcache_grid *grid,
      int x, int y, int z, double *bbox) {
   double res  = grid->levels[z]->resolution;
   bbox[0] = grid->extent[0] + (res * x * grid->tile_sx);
   bbox[1] = grid->extent[1] + (res * y * grid->tile_sy);
   bbox[2] = grid->extent[0] + (res * (x + 1) * grid->tile_sx);
   bbox[3] = grid->extent[1] + (res * (y + 1) * grid->tile_sy);
}

const char* mapcache_grid_get_crs(mapcache_context *ctx, mapcache_grid *grid) {
   char *epsgnum;

   /*locate the number after epsg: in the grd srs*/
   epsgnum = strchr(grid->srs,':');
   if(!epsgnum) {
      epsgnum = grid->srs;
   } else {
      epsgnum++;
   }

   return apr_psprintf(ctx->pool,"urn:ogc:def:crs:EPSG::%s",epsgnum);
}

const char* mapcache_grid_get_srs(mapcache_context *ctx, mapcache_grid *grid) {
   return (const char*)grid->srs;
}

void mapcache_grid_compute_limits(const mapcache_grid *grid, const double *extent, int **limits, int tolerance) {
   int i;
   double epsilon = 0.0000001;
   for(i=0;i<grid->nlevels;i++) {
      mapcache_grid_level *level = grid->levels[i];
      double unitheight = grid->tile_sy * level->resolution;
      double unitwidth = grid->tile_sx * level->resolution;

      level->maxy = ceil((grid->extent[3]-grid->extent[1] - 0.01* unitheight)/unitheight);
      level->maxx = ceil((grid->extent[2]-grid->extent[0] - 0.01* unitwidth)/unitwidth);

      limits[i][0] = floor((extent[0] - grid->extent[0]) / unitwidth + epsilon) - tolerance;
      limits[i][2] = ceil((extent[2] - grid->extent[0]) / unitwidth - epsilon) + tolerance;
      limits[i][1] = floor((extent[1] - grid->extent[1]) / unitheight + epsilon) - tolerance;
      limits[i][3] = ceil((extent[3] - grid->extent[1]) / unitheight - epsilon) + tolerance;
      // to avoid requesting out-of-range tiles
      if (limits[i][0] < 0) limits[i][0] = 0;
      if (limits[i][2] > level->maxx) limits[i][2] = level->maxx;
      if (limits[i][1] < 0) limits[i][1] = 0;
      if (limits[i][3] > level->maxy) limits[i][3] = level->maxy;

   }
}

double mapcache_grid_get_resolution(double *bbox, int sx, int sy) {
   double rx =  mapcache_grid_get_horizontal_resolution(bbox,sx);
   double ry =  mapcache_grid_get_vertical_resolution(bbox,sy);
   return MAPCACHE_MAX(rx,ry);
}


double mapcache_grid_get_horizontal_resolution(double *bbox, int width) {
   return (bbox[2] - bbox[0]) / (double)width;
}

double mapcache_grid_get_vertical_resolution(double *bbox, int height) {
   return (bbox[3] - bbox[1]) / (double)height;
}

int mapcache_grid_get_level(mapcache_context *ctx, mapcache_grid *grid, double *resolution, int *level) {
   double max_diff = *resolution / (double)MAPCACHE_MAX(grid->tile_sx, grid->tile_sy);
   int i;
   for(i=0; i<grid->nlevels; i++) {
      if(fabs(grid->levels[i]->resolution - *resolution) < max_diff) {
         *resolution = grid->levels[i]->resolution;
         *level = i;
         return MAPCACHE_SUCCESS;
      }
   }
   return MAPCACHE_FAILURE;
}

void mapcache_grid_get_closest_level(mapcache_context *ctx, mapcache_grid *grid, double resolution, int *level) {
   double dst = fabs(grid->levels[0]->resolution - resolution);
   *level = 0;
   int i;
   for(i=1; i<grid->nlevels; i++) {
      double curdst = fabs(grid->levels[i]->resolution - resolution);
      if( curdst < dst) {
         dst = curdst;
         *level = i;
      }
   }
}

/*
 * update the tile by setting it's x,y,z value given a bbox.
 * will return MAPCACHE_TILESET_WRONG_RESOLUTION or MAPCACHE_TILESET_WRONG_EXTENT
 * if the bbox does not correspond to the tileset's configuration
 */
int mapcache_grid_get_cell(mapcache_context *ctx, mapcache_grid *grid, double *bbox,
      int *x, int *y, int *z) {
   double res = mapcache_grid_get_resolution(bbox,grid->tile_sx,grid->tile_sy);
   if(MAPCACHE_SUCCESS != mapcache_grid_get_level(ctx, grid, &res, z))
      return MAPCACHE_FAILURE;
   
   *x = (int)round((bbox[0] - grid->extent[0]) / (res * grid->tile_sx));
   *y = (int)round((bbox[1] - grid->extent[1]) / (res * grid->tile_sy));

   if((fabs(bbox[0] - (*x * res * grid->tile_sx) - grid->extent[0] ) / res > 1) ||
         (fabs(bbox[1] - (*y * res * grid->tile_sy) - grid->extent[1] ) / res > 1)) {
      return MAPCACHE_FAILURE;
   }
   return MAPCACHE_SUCCESS;
}


void mapcache_grid_get_xy(mapcache_context *ctx, mapcache_grid *grid, double dx, double dy,
        int z, int *x, int *y) {
#ifdef DEBUG
   if(z>=grid->nlevels) {
      ctx->set_error(ctx,500,"####BUG##### requesting invalid level");
      return;
   }
#endif
   double res = grid->levels[z]->resolution;
   *x = (int)((dx - grid->extent[0]) / (res * grid->tile_sx));
   *y = (int)((dy - grid->extent[1]) / (res * grid->tile_sy));
}
/* vim: ai ts=3 sts=3 et sw=3
*/

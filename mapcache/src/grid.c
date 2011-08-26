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
#include <math.h>
/*
 * allocate and initialize a new tileset
 */
geocache_grid* geocache_grid_create(apr_pool_t *pool) {
   geocache_grid* grid = (geocache_grid*)apr_pcalloc(pool, sizeof(geocache_grid));
   grid->metadata = apr_table_make(pool,3);
   return grid;
}

double geocache_grid_get_resolution(geocache_grid *grid, double *bbox) {
   double rx = (bbox[2] - bbox[0]) / (double)grid->tile_sx;
   double ry = (bbox[3] - bbox[1]) / (double)grid->tile_sy;
   return GEOCACHE_MAX(rx,ry);
}

void geocache_grid_get_level(geocache_context *ctx, geocache_grid *grid, double *resolution, int *level) {
   double max_diff = *resolution / (double)GEOCACHE_MAX(grid->tile_sx, grid->tile_sy);
   int i;
   for(i=0; i<grid->levels; i++) {
      if(fabs(grid->resolutions[i] - *resolution) < max_diff) {
         *resolution = grid->resolutions[i];
         *level = i;
         return;
      }
   }
   ctx->set_error(ctx, GEOCACHE_TILESET_ERROR, "grid %s: failed lookup for resolution %f", grid->name, *resolution);
}

void geocache_grid_get_xy(geocache_context *ctx, geocache_grid *grid, double dx, double dy,
        int z, int *x, int *y) {
    double res = grid->resolutions[z];
    *x = (int)((dx - grid->extents[z][0]) / (res * grid->tile_sx));
    *y = (int)((dy - grid->extents[z][1]) / (res * grid->tile_sy));
}
/* vim: ai ts=3 sts=3 et sw=3
*/

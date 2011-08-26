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

/*
 * allocate and initialize a new tileset
 */
geocache_grid* geocache_grid_create(apr_pool_t *pool) {
   geocache_grid* grid = (geocache_grid*)apr_pcalloc(pool, sizeof(geocache_grid));
   grid->metadata = apr_table_make(pool,3);
   return grid;
}
/* vim: ai ts=3 sts=3 et sw=3
*/

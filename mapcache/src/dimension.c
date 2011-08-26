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


int _geocache_dimension_values_validate(geocache_context *ctx, geocache_dimension *dimension, const char *value) {
   int i;
   geocache_dimension_values *dim = (geocache_dimension_values*)dimension;
#ifdef DEBUG
   if(dimension->type != GEOCACHE_DIMENSION_VALUES) {
      ctx->set_error(ctx, GEOCACHE_ERROR, "##### BUG ###### dimension values 1");
      return;
   }
#endif
   for(i=0;i<dim->nvalues;i++) {
      if(!strcmp(value,dim->values[i]))
         return GEOCACHE_SUCCESS;
   }
   return GEOCACHE_FAILURE;
}
geocache_dimension_values* geocache_dimension_values_create(apr_pool_t *pool) {
   geocache_dimension_values *dimension = apr_pcalloc(pool, sizeof(geocache_dimension_values));
   dimension->dimension.type = GEOCACHE_DIMENSION_VALUES;
   dimension->nvalues = 0;
   dimension->dimension.validate = _geocache_dimension_values_validate;
   return dimension;
}

/* vim: ai ts=3 sts=3 et sw=3
*/

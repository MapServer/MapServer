/*
 * source.c
 *
 *  Created on: Oct 10, 2010
 *      Author: tom
 */

#include "geocache.h"

void geocache_source_init(geocache_context *ctx, geocache_source *source) {
	source->data_extent[0] =
			source->data_extent[1] =
			source->data_extent[2] =
			source->data_extent[3] = -1;
	source->srs = NULL;
	source->supports_metatiling = 0;
}

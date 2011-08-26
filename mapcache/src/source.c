/*
 * source.c
 *
 *  Created on: Oct 10, 2010
 *      Author: tom
 */

#include "yatc.h"
#include <http_log.h>

static int _yatc_source_render_tile(yatc_tile *tile, request_rec *r) {
   return YATC_FAILURE;
}

static int _yatc_source_render_metatile(yatc_metatile *tile, request_rec *r) {
   return YATC_FAILURE;
}

void yatc_source_init(yatc_source *source, apr_pool_t *pool) {
	source->image_format = YATC_IMAGE_FORMAT_UNSPECIFIED;
	source->data_extent[0] =
			source->data_extent[1] =
			source->data_extent[2] =
			source->data_extent[3] = -1;
	source->srs = NULL;
	source->supports_metatiling = 0;
	source->render_metatile = _yatc_source_render_metatile;
	source->render_tile = _yatc_source_render_tile;
}

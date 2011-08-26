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
#include <apr_strings.h>


/**\addtogroup imageio_mixed */
/** @{ */

/**
 * \brief encode an image to mixed format
 * depending on the transparency of the provided image, use either one
 * of the two member encoding formats.
 * Intended to be used for creating png when transparency is present, and jpeg
 * when fully opaque
 * \sa geocache_image_format::write()
 */
static geocache_buffer* _geocache_imageio_mixed_encode( geocache_context *ctx,
      geocache_image *image, geocache_image_format *format) {
   geocache_image_format_mixed *f = (geocache_image_format_mixed*)format;
   if(geocache_image_has_alpha(image)) {
      return f->transparent->write(ctx,image,f->transparent);
   } else {
      return f->opaque->write(ctx,image,f->opaque);
   }
}

geocache_image_format* geocache_imageio_create_mixed_format(apr_pool_t *pool,
      char *name, geocache_image_format *transparent, geocache_image_format *opaque) {
   geocache_image_format_mixed *format = apr_pcalloc(pool, sizeof(geocache_image_format_mixed));
   format->format.name = name;
   format->transparent = transparent;
   format->opaque = opaque;
   format->format.extension = apr_pstrdup(pool,"xxx");
   format->format.mime_type = apr_pstrdup(pool,"image/png+jpeg");
   format->format.write = _geocache_imageio_mixed_encode;
   format->format.create_empty_image = transparent->create_empty_image;
   format->format.metadata = apr_table_make(pool,3);
   return (geocache_image_format*)format;
}

/** @} */

/* vim: ai ts=3 sts=3 et sw=3
*/

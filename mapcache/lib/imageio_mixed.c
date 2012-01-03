/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: Mixed PNG+JPEG format
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include "mapcache.h"
#include <apr_strings.h>


/**\addtogroup imageio_mixed */
/** @{ */

/**
 * \brief encode an image to mixed format
 * depending on the transparency of the provided image, use either one
 * of the two member encoding formats.
 * Intended to be used for creating png when transparency is present, and jpeg
 * when fully opaque
 * \sa mapcache_image_format::write()
 */
static mapcache_buffer* _mapcache_imageio_mixed_encode( mapcache_context *ctx,
      mapcache_image *image, mapcache_image_format *format) {
   mapcache_image_format_mixed *f = (mapcache_image_format_mixed*)format;
   if(mapcache_image_has_alpha(image)) {
      return f->transparent->write(ctx,image,f->transparent);
   } else {
      return f->opaque->write(ctx,image,f->opaque);
   }
}

mapcache_image_format* mapcache_imageio_create_mixed_format(apr_pool_t *pool,
      char *name, mapcache_image_format *transparent, mapcache_image_format *opaque) {
   mapcache_image_format_mixed *format = apr_pcalloc(pool, sizeof(mapcache_image_format_mixed));
   format->format.name = name;
   format->transparent = transparent;
   format->opaque = opaque;
   format->format.extension = apr_pstrdup(pool,"xxx");
   format->format.mime_type = NULL;
   format->format.write = _mapcache_imageio_mixed_encode;
   format->format.create_empty_image = transparent->create_empty_image;
   format->format.metadata = apr_table_make(pool,3);
   return (mapcache_image_format*)format;
}

/** @} */

/* vim: ai ts=3 sts=3 et sw=3
*/

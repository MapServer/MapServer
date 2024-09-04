/******************************************************************************
 * $Id: mapchart.c 11880 2011-07-07 19:51:37Z sdlime $
 *
 * Project:  MapServer
 * Purpose:  XMP embedded image metadata (MS-RFC-7X)
 * Author:   Paul Ramsey <pramsey@opengeo.org>
 *
 ******************************************************************************
 * Copyright (c) 1996-2007 Regents of the University of Minnesota.
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
 ****************************************************************************/

#include "mapserver.h"

#ifdef USE_EXEMPI

/* To pull parts out of hash keys */
#include <regex.h>

/* To write the XMP XML into the files */
#include <xmp.h>
#include <xmpconsts.h>

/**
 * Get standard Exempi namespace URI for a given namespace string
 */
static const char *msXmpUri(char *ns_name) {
  /* Creative Commons */
  if (strcmp(ns_name, "cc") == 0)
    return NS_CC;
  /* Dublin Core */
  else if (strcmp(ns_name, "dc") == 0)
    return NS_DC;
  /* XMP Meta */
  else if (strcmp(ns_name, "meta") == 0)
    return NS_XMP_META;
  /* XMP Rights */
  else if (strcmp(ns_name, "rights") == 0)
    return NS_XAP_RIGHTS;
  /* EXIF */
  else if (strcmp(ns_name, "exif") == 0)
    return NS_EXIF;
  /* TIFF */
  else if (strcmp(ns_name, "tiff") == 0)
    return NS_TIFF;
  /* Photoshop Camera Raw Schema */
  else if (strcmp(ns_name, "crs") == 0)
    return NS_CAMERA_RAW_SETTINGS;
  /* Photoshop */
  else if (strcmp(ns_name, "photoshop") == 0)
    return NS_PHOTOSHOP;
  else
    return NULL;
}

#endif

/**
 * Is there any XMP metadata in the map file for us to worry about?
 */
int msXmpPresent(mapObj *map) {
#ifdef USE_EXEMPI

  /* Read the WEB.METADATA */
  hashTableObj hash_metadata = map->web.metadata;
  const char *key = NULL;
  int rv = MS_FALSE;

  /* Check all the keys for "xmp_" start pattern */
  key = msFirstKeyFromHashTable(&hash_metadata);

  /* No first key? No license info. */
  if (!key)
    return MS_FALSE;

  do {
    /* Found one! Break out and return true */
    if (strcasestr(key, "xmp_") == key) {
      rv = MS_TRUE;
      break;
    }
  } while ((key = msNextKeyFromHashTable(&hash_metadata, key)));

  return rv;

#else
  (void)map;
  return MS_FALSE;
#endif
}

/**
 * Is there any XMP metadata in the map file for us to worry about?
 */
int msXmpWrite(mapObj *map, const char *filename) {
#ifdef USE_EXEMPI

  /* Should hold our keys */
  hashTableObj hash_metadata = map->web.metadata;
  /* Temporary place for custom name spaces */
  hashTableObj hash_ns;
  /* We use regex to strip out the namespace and XMP key value from the metadata
   * key */
  regex_t xmp_regex;
  const char *xmp_ns_str = "^xmp_(.+)_namespace$";
  const char *xmp_tag_str = "^xmp_(.+)_(.+)$";
  const char *key = NULL;
  static int regflags = REG_ICASE | REG_EXTENDED;

  /* XMP object and file pointers */
  XmpPtr xmp;
  XmpFilePtr f;

  /* Force the hash table to empty */
  hash_ns.numitems = 0;

  /* Prepare XMP library */
  xmp_init();
  f = xmp_files_open_new(filename, XMP_OPEN_FORUPDATE);
  if (!f) {
    msSetError(MS_MISCERR,
               "Unable to open temporary file '%s' to write XMP info",
               "msXmpWrite()", filename);
    return MS_FAILURE;
  }

  /* Create a new XMP structure if the file doesn't already have one */
  xmp = xmp_files_get_new_xmp(f);
  if (xmp == NULL)
    xmp = xmp_new_empty();

  /* Check we can write to the file */
  if (!xmp_files_can_put_xmp(f, xmp)) {
    msSetError(MS_MISCERR, "Unable to write XMP information to '%s'",
               "msXmpWrite()", filename);
    return MS_FAILURE;
  }

  /* Compile our "xmp_*_namespace" regex */
  if (regcomp(&xmp_regex, xmp_ns_str, regflags)) {
    msSetError(MS_MISCERR, "Unable compile regex '%s'", "msXmpWrite()",
               xmp_ns_str);
    return MS_FAILURE;
  }

  /* Check all the keys for "xmp_*_namespace" pattern */
  initHashTable(&hash_ns);
  key = msFirstKeyFromHashTable(&hash_metadata);

  /* No first key? No license info. We shouldn't get here. */
  if (!key)
    return MS_SUCCESS;

  do {
    /* Our regex has one match slot */
    regmatch_t matches[2];

    /* Found a custom namespace entry! Store it for later. */
    if (0 == regexec(&xmp_regex, key, 2, matches, 0)) {
      size_t ns_size = 0;
      char *ns_name = NULL;
      const char *ns_uri;

      /* Copy in the namespace name */
      ns_size = matches[1].rm_eo - matches[1].rm_so;
      ns_name = msSmallMalloc(ns_size + 1);
      memcpy(ns_name, key + matches[1].rm_so, ns_size);
      ns_name[ns_size] = 0; /* null terminate */

      /* Copy in the namespace uri */
      ns_uri = msLookupHashTable(&hash_metadata, key);
      msInsertHashTable(&hash_ns, ns_name, ns_uri);
      xmp_register_namespace(ns_uri, ns_name, NULL);
      msFree(ns_name);
    }
  } while ((key = msNextKeyFromHashTable(&hash_metadata, key)));
  /* Clean up regex */
  regfree(&xmp_regex);

  /* Compile our "xmp_*_*" regex */
  if (regcomp(&xmp_regex, xmp_tag_str, regflags)) {
    msFreeHashItems(&hash_ns);
    msSetError(MS_MISCERR, "Unable compile regex '%s'", "msXmpWrite()",
               xmp_tag_str);
    return MS_FAILURE;
  }

  /* Check all the keys for "xmp_*_*" pattern */
  key = msFirstKeyFromHashTable(&hash_metadata);
  for (; key != NULL; key = msNextKeyFromHashTable(&hash_metadata, key)) {
    /* Our regex has two match slots */
    regmatch_t matches[3];

    /* Found a namespace entry! Write it into XMP. */
    if (0 == regexec(&xmp_regex, key, 3, matches, 0)) {
      /* Get the namespace and tag name */
      size_t ns_name_size = matches[1].rm_eo - matches[1].rm_so;
      size_t ns_tag_size = matches[2].rm_eo - matches[2].rm_so;
      char *ns_name = msSmallMalloc(ns_name_size + 1);
      char *ns_tag = msSmallMalloc(ns_tag_size + 1);
      const char *ns_uri = NULL;
      memcpy(ns_name, key + matches[1].rm_so, ns_name_size);
      memcpy(ns_tag, key + matches[2].rm_so, ns_tag_size);
      ns_name[ns_name_size] = 0; /* null terminate */
      ns_tag[ns_tag_size] = 0;   /* null terminate */

      if (strcmp(ns_tag, "namespace") == 0) {
        msFree(ns_name);
        msFree(ns_tag);
        continue;
      }

      /* If this is a default name space?... */
      if ((ns_uri = msXmpUri(ns_name))) {
        xmp_register_namespace(ns_uri, ns_name, NULL);
        xmp_set_property(xmp, ns_uri, ns_tag,
                         msLookupHashTable(&hash_metadata, key), 0);
      }
      /* Or maybe it's a custom one?... */
      else if ((ns_uri = msLookupHashTable(&hash_ns, ns_name))) {
        xmp_set_property(xmp, ns_uri, ns_tag,
                         msLookupHashTable(&hash_metadata, key), 0);
      }
      /* Or perhaps we're screwed. */
      else {
        msSetError(MS_MISCERR,
                   "Unable to identify XMP namespace '%s' in metadata key '%s'",
                   "msXmpWrite()", ns_name, key);
        msFreeHashItems(&hash_ns);
        msFree(ns_name);
        msFree(ns_tag);
        return MS_FAILURE;
      }
      msFree(ns_name);
      msFree(ns_tag);
    }
  }

  /* Clean up regex */
  regfree(&xmp_regex);

  /* Write out the XMP */
  if (!xmp_files_put_xmp(f, xmp)) {
    msFreeHashItems(&hash_ns);
    msSetError(MS_MISCERR, "Unable to execute '%s' on pointer %p",
               "msXmpWrite()", "xmp_files_put_xmp", f);
    return MS_FAILURE;
  }

  /* Write out the file and flush */
  if (!xmp_files_close(f, XMP_CLOSE_SAFEUPDATE)) {
    msFreeHashItems(&hash_ns);
    msSetError(MS_MISCERR, "Unable to execute '%s' on pointer %p",
               "msXmpWrite()", "xmp_files_close", f);
    return MS_FAILURE;
  }

  msFreeHashItems(&hash_ns);
  xmp_free(xmp);
  xmp_terminate();

  return MS_SUCCESS;

#else
  (void)map;
  (void)filename;
  return MS_FAILURE;
#endif
}

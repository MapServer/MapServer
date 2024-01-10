/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Freetype Font helper functions
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2013 Regents of the University of Minnesota.
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

#include "mapserver.h"
#include "mapthread.h"
#include "fontcache.h"
#include "dejavu-sans-condensed.h"

#include "cpl_conv.h"

typedef struct {
  FT_Library library;
  face_element *face_cache;
  glyph_element *bitmap_glyph_cache;
} ft_cache;

#ifdef USE_THREAD
typedef struct ft_thread_cache ft_thread_cache;
struct ft_thread_cache {
  void *thread_id;
  ft_thread_cache *next;
  ft_cache cache;
};
ft_thread_cache *ft_caches;
int use_global_ft_cache;
#else
ft_cache global_ft_cache;
#endif

void msInitFontCache(ft_cache *c) {
  memset(c, 0, sizeof(ft_cache));
  FT_Init_FreeType(&c->library);
}

void msFreeFontCache(ft_cache *c) {
  /* ... TODO ... */
  face_element *cur_face, *tmp_face;
  glyph_element *cur_bitmap, *tmp_bitmap;
  UT_HASH_ITER(hh, c->face_cache, cur_face, tmp_face) {
    index_element *cur_index, *tmp_index;
    outline_element *cur_outline, *tmp_outline;
    glyph_element *cur_glyph, *tmp_glyph;
    UT_HASH_ITER(hh, cur_face->index_cache, cur_index, tmp_index) {
      UT_HASH_DEL(cur_face->index_cache, cur_index);
      free(cur_index);
    }
    UT_HASH_ITER(hh, cur_face->outline_cache, cur_outline, tmp_outline) {
      UT_HASH_DEL(cur_face->outline_cache, cur_outline);
      FT_Outline_Done(c->library, &cur_outline->outline);
      free(cur_outline);
    }
    UT_HASH_ITER(hh, cur_face->glyph_cache, cur_glyph, tmp_glyph) {
      UT_HASH_DEL(cur_face->glyph_cache, cur_glyph);
      free(cur_glyph);
    }
#ifdef USE_HARFBUZZ
    if (cur_face->hbfont) {
      hb_font_destroy(cur_face->hbfont->hbfont);
      hb_font_destroy(cur_face->hbfont->hbparentfont);
      hb_font_funcs_destroy(cur_face->hbfont->funcs);
      free(cur_face->hbfont);
    }
#endif
    FT_Done_Face(cur_face->face);
    free(cur_face->font);
    UT_HASH_DEL(c->face_cache, cur_face);
    free(cur_face);
  }
  FT_Done_FreeType(c->library);

  UT_HASH_ITER(hh, c->bitmap_glyph_cache, cur_bitmap, tmp_bitmap) {
    UT_HASH_DEL(c->bitmap_glyph_cache, cur_bitmap);
    free(cur_bitmap);
  }
  memset(c, 0, sizeof(ft_cache));
}

ft_cache *msGetFontCache() {
#ifndef USE_THREAD
  return &global_ft_cache;
#else
  void *nThreadId = 0;
  ft_thread_cache *prev = NULL, *cur = ft_caches;

  if (!use_global_ft_cache)
    nThreadId = msGetThreadId();

  if (cur != NULL && cur->thread_id == nThreadId)
    return &cur->cache;

  /* -------------------------------------------------------------------- */
  /*      Search for cache for this thread                                */
  /* -------------------------------------------------------------------- */
  msAcquireLock(TLOCK_TTF);

  cur = ft_caches;
  while (cur != NULL && cur->thread_id != nThreadId) {
    prev = cur;
    cur = cur->next;
  }

  /* -------------------------------------------------------------------- */
  /*      If we found it, make sure it is pushed to the front of the      */
  /*      link for faster finding next time, and return it.               */
  /* -------------------------------------------------------------------- */
  if (cur != NULL) {
    if (prev != NULL) {
      prev->next = cur->next;
      cur->next = ft_caches;
      ft_caches = cur;
    }

    msReleaseLock(TLOCK_TTF);
    return &cur->cache;
  }

  /* -------------------------------------------------------------------- */
  /*      Create a new context group for this thread.                     */
  /* -------------------------------------------------------------------- */
  cur = msSmallMalloc(sizeof(ft_thread_cache));
  cur->next = NULL;
  cur->thread_id = nThreadId;
  msInitFontCache(&cur->cache);
  cur->next = ft_caches;
  ft_caches = cur;

  msReleaseLock(TLOCK_TTF);

  return &cur->cache;
#endif
}

void msFontCacheSetup() {
#ifndef USE_THREAD
  ft_cache *c = msGetFontCache();
  msInitFontCache(c);
#else
  const char *use_global_cache =
      CPLGetConfigOption("MS_USE_GLOBAL_FT_CACHE", NULL);
  if (use_global_cache)
    use_global_ft_cache = atoi(use_global_cache);
  else
    use_global_ft_cache = 0;

  ft_caches = NULL;
#endif
}

void msFontCacheCleanup() {
#ifndef USE_THREAD
  ft_cache *c = msGetFontCache();
  msFreeFontCache(c);
#else
  ft_thread_cache *cur, *next;
  msAcquireLock(TLOCK_TTF);
  cur = ft_caches;
  while (cur != NULL) {
    msFreeFontCache(&cur->cache);
    next = cur->next;
    free(cur);
    cur = next;
  }
  ft_caches = NULL;
  msReleaseLock(TLOCK_TTF);
#endif
}

unsigned int msGetGlyphIndex(face_element *face, unsigned int unicode) {
  index_element *ic;
  if (face->face->charmap &&
      face->face->charmap->encoding == FT_ENCODING_MS_SYMBOL) {
    unicode |= 0xf000; /* why? */
  }
#ifdef USE_THREAD
  if (use_global_ft_cache)
    msAcquireLock(TLOCK_TTF);
#endif
  UT_HASH_FIND_INT(face->index_cache, &unicode, ic);
  if (!ic) {
    ic = msSmallMalloc(sizeof(index_element));
    ic->codepoint = FT_Get_Char_Index(face->face, unicode);
    ic->unicode = unicode;
    UT_HASH_ADD_INT(face->index_cache, unicode, ic);
  }
#ifdef USE_THREAD
  if (use_global_ft_cache)
    msReleaseLock(TLOCK_TTF);
#endif
  return ic->codepoint;
}

#define MS_DEFAULT_FONT_KEY "_ms_default_"

face_element *msGetFontFace(const char *key, fontSetObj *fontset) {
  face_element *fc;
  int error;
  ft_cache *cache = msGetFontCache();
  if (!key) {
    key = MS_DEFAULT_FONT_KEY;
  }
#ifdef USE_THREAD
  if (use_global_ft_cache)
    msAcquireLock(TLOCK_TTF);
#endif
  UT_HASH_FIND_STR(cache->face_cache, key, fc);
  if (!fc) {
    const char *fontfile = NULL;
    fc = msSmallCalloc(1, sizeof(face_element));
    if (fontset && strcmp(key, MS_DEFAULT_FONT_KEY)) {
      fontfile = msLookupHashTable(&(fontset->fonts), key);
      if (!fontfile) {
        msSetError(MS_MISCERR, "Could not find font with key \"%s\" in fontset",
                   "msGetFontFace()", key);
        free(fc);
#ifdef USE_THREAD
        if (use_global_ft_cache)
          msReleaseLock(TLOCK_TTF);
#endif
        return NULL;
      }
      error = FT_New_Face(cache->library, fontfile, 0, &(fc->face));
    } else {
      error = FT_New_Memory_Face(cache->library, dejavu_sans_condensed_ttf,
                                 dejavu_sans_condensed_ttf_len, 0, &(fc->face));
    }
    if (error) {
      msSetError(MS_MISCERR,
                 "Freetype was unable to load font file \"%s\" for key \"%s\"",
                 "msGetFontFace()", fontfile, key);
      free(fc);
#ifdef USE_THREAD
      if (use_global_ft_cache)
        msReleaseLock(TLOCK_TTF);
#endif
      return NULL;
    }
    if (!fc->face->charmap) {
      /* The font file has no unicode charmap, select an alternate one */
      if (FT_Select_Charmap(fc->face, FT_ENCODING_MS_SYMBOL))
        FT_Select_Charmap(fc->face, FT_ENCODING_APPLE_ROMAN);
      /* the previous calls may have failed, we ignore as there's nothing much
       * left to do */
    }
    fc->font = msStrdup(key);
    UT_HASH_ADD_KEYPTR(hh, cache->face_cache, fc->font, strlen(key), fc);
  }
#ifdef USE_THREAD
  if (use_global_ft_cache)
    msReleaseLock(TLOCK_TTF);
#endif
  return fc;
}

glyph_element *msGetGlyphByIndex(face_element *face, unsigned int size,
                                 unsigned int codepoint) {
  glyph_element *gc;
  glyph_element_key key;
  memset(&key, 0, sizeof(glyph_element_key));
  key.codepoint = codepoint;
  key.size = size;
#ifdef USE_THREAD
  if (use_global_ft_cache)
    msAcquireLock(TLOCK_TTF);
#endif
  UT_HASH_FIND(hh, face->glyph_cache, &key, sizeof(glyph_element_key), gc);
  if (!gc) {
    FT_Error error;
    gc = msSmallMalloc(sizeof(glyph_element));
    if (MS_NINT(size * 96.0 / 72.0) != face->face->size->metrics.x_ppem) {
      FT_Set_Pixel_Sizes(face->face, 0, MS_NINT(size * 96 / 72.0));
    }
    error =
        FT_Load_Glyph(face->face, key.codepoint,
                      FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING |
                          FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH);
    if (error) {
      msDebug("Unable to load glyph %u for font \"%s\". Using ? as fallback.\n",
              key.codepoint, face->font);
      // If we can't find a glyph then try to fallback to a question mark.
      unsigned int fallbackCodepoint = msGetGlyphIndex(face, 0x3F);
      error = FT_Load_Glyph(face->face, fallbackCodepoint,
                            FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP |
                                FT_LOAD_NO_HINTING |
                                FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH);
    }
    if (error) {
      msSetError(MS_MISCERR, "unable to load glyph %u for font \"%s\"",
                 "msGetGlyphByIndex()", key.codepoint, face->font);
      free(gc);
#ifdef USE_THREAD
      if (use_global_ft_cache)
        msReleaseLock(TLOCK_TTF);
#endif
      return NULL;
    }
    gc->metrics.minx = face->face->glyph->metrics.horiBearingX / 64.0;
    gc->metrics.maxx =
        gc->metrics.minx + face->face->glyph->metrics.width / 64.0;
    gc->metrics.maxy = face->face->glyph->metrics.horiBearingY / 64.0;
    gc->metrics.miny =
        gc->metrics.maxy - face->face->glyph->metrics.height / 64.0;
    gc->metrics.advance = face->face->glyph->metrics.horiAdvance / 64.0;
    gc->key = key;
    UT_HASH_ADD(hh, face->glyph_cache, key, sizeof(glyph_element_key), gc);
  }
#ifdef USE_THREAD
  if (use_global_ft_cache)
    msReleaseLock(TLOCK_TTF);
#endif
  return gc;
}

outline_element *msGetGlyphOutline(face_element *face, glyph_element *glyph) {
  outline_element *oc;
  outline_element_key key;
  ft_cache *cache = msGetFontCache();
  memset(&key, 0, sizeof(outline_element_key));
  key.glyph = glyph;
#ifdef USE_THREAD
  if (use_global_ft_cache)
    msAcquireLock(TLOCK_TTF);
#endif
  UT_HASH_FIND(hh, face->outline_cache, &key, sizeof(outline_element_key), oc);
  if (!oc) {
    FT_Matrix matrix;
    FT_Vector pen;
    FT_Error error;
    oc = msSmallMalloc(sizeof(outline_element));
    if (MS_NINT(glyph->key.size * 96.0 / 72.0) !=
        face->face->size->metrics.x_ppem) {
      FT_Set_Pixel_Sizes(face->face, 0, MS_NINT(glyph->key.size * 96 / 72.0));
    }
    matrix.xx = matrix.yy = 0x10000L;
    matrix.xy = matrix.yx = 0x00000L;
    pen.x = pen.y = 0;
    FT_Set_Transform(face->face, &matrix, &pen);
    error = FT_Load_Glyph(
        face->face, glyph->key.codepoint,
        FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP /*|FT_LOAD_IGNORE_TRANSFORM*/ |
            FT_LOAD_NO_HINTING | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH);
    if (error) {
      msDebug("Unable to load glyph %u for font \"%s\". Using ? as fallback.\n",
              glyph->key.codepoint, face->font);
      // If we can't find a glyph then try to fallback to a question mark.
      unsigned int fallbackCodepoint = msGetGlyphIndex(face, 0x3F);
      error = FT_Load_Glyph(
          face->face, fallbackCodepoint,
          FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP /*|FT_LOAD_IGNORE_TRANSFORM*/ |
              FT_LOAD_NO_HINTING | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH);
    }
    if (error) {
      msSetError(MS_MISCERR, "unable to load glyph %u for font \"%s\"",
                 "msGetGlyphOutline()", glyph->key.codepoint, face->font);
#ifdef USE_THREAD
      if (use_global_ft_cache)
        msReleaseLock(TLOCK_TTF);
#endif
      return NULL;
    }
    error = FT_Outline_New(cache->library, face->face->glyph->outline.n_points,
                           face->face->glyph->outline.n_contours, &oc->outline);
    (void)error;
    FT_Outline_Copy(&face->face->glyph->outline, &oc->outline);
    oc->key = key;
    UT_HASH_ADD(hh, face->outline_cache, key, sizeof(outline_element_key), oc);
  }
#ifdef USE_THREAD
  if (use_global_ft_cache)
    msReleaseLock(TLOCK_TTF);
#endif
  return oc;
}

int msIsGlyphASpace(glyphObj *glyph) {
  /* space or tab, for now */
  unsigned int space, tab;
  space = msGetGlyphIndex(glyph->face, 0x20);
  tab = msGetGlyphIndex(glyph->face, 0x9);
  return glyph->glyph->key.codepoint == space ||
         glyph->glyph->key.codepoint == tab;
}

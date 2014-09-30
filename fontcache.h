#include "mapserver.h"
#include "uthash.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#ifdef USE_FRIBIDI
 #if (defined(_WIN32) && !defined(__CYGWIN__)) || defined(HAVE_FRIBIDI2)
  #include "fribidi.h"
 #else
  #include <fribidi/fribidi.h>
 #endif
 #include <hb-ft.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef struct { /* this one can remain private */
  unsigned int unicode;
  unsigned int codepoint;
  UT_hash_handle hh;
} index_element;

typedef struct {
  void *hbparentfont;
  void *hbfont;
  void *funcs;
  int cursize;
} hb_font_element;

typedef struct {
  unsigned int codepoint;
  unsigned int size;
} glyph_element_key;

struct glyph_element{
  glyph_element_key key;
  glyph_metrics metrics;
  UT_hash_handle hh;
};

typedef struct {
  glyph_element *glyph;
} outline_element_key;

typedef struct {
  outline_element_key key;
  FT_Outline outline;
  UT_hash_handle hh;
} outline_element;

typedef struct {
  glyph_element *glyph;
} bitmap_element_key;

typedef struct {
  bitmap_element_key key;
  void *bitmap;
  UT_hash_handle hh;
} bitmap_element;

struct face_element{
  char *font;
  FT_Face face;
  index_element *index_cache;
  glyph_element *glyph_cache;
  outline_element *outline_cache;
  hb_font_element *hbfont;
  UT_hash_handle hh;
};


face_element* msGetFontFace(char *key, fontSetObj *fontset);
outline_element* msGetGlyphOutline(face_element *face, glyph_element *glyph);
glyph_element* msGetBitmapGlyph(rendererVTableObj *renderer, unsigned int size, unsigned int unicode);
unsigned int msGetGlyphIndex(face_element *face, unsigned int unicode);
glyph_element* msGetGlyphByIndex(face_element *face, unsigned int size, unsigned int codepoint);
int msIsGlyphASpace(glyphObj *glyph);

#ifdef __cplusplus
}
#endif

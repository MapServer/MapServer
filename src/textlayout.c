/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Text Layout functions, eventually using Harfbuzz and Fribidi/ICU
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

#include <float.h>
#include "mapserver.h"

#ifdef USE_ICONV
#include <iconv.h>
#include <wchar.h>
#endif
#include "fontcache.h"
#include FT_ADVANCES_H
#include FT_TYPES_H

typedef struct {
  unsigned int *unicodes;
  unsigned int *codepoints;
#ifdef USE_FRIBIDI
  FriBidiCharType *ctypes;
  FriBidiLevel *bidi_levels;
#endif
#ifdef USE_HARFBUZZ
  hb_script_t *scripts;
#endif
} TextInfo;

typedef struct {
  int offset; /* offset in TextInfo entries where the current run is starting */
  int length; /* number of unicode glyphs in this run */
#ifdef USE_FRIBIDI
  FriBidiLevel rtl;   /* bidi embedding level of run: -1 to skip shaping,
                         otherwise if pair:ltr, odd:rtl */
  hb_script_t script; /* script: latin, arabic, thai, etc... */
#endif
  int line_number;
  face_element *face; /* font face to use for this run */
} text_run;

#ifdef USE_HARFBUZZ
struct _ms_hb_user_data {
  text_run *run;
  TextInfo *info;
  int glyph_size;
};

const char *_ms_script_prefix_en = "en:";
const char *_ms_script_prefix_ar = "ar:";
const char *_ms_script_prefix_cn = "cn:";
const char *_ms_script_prefix_hy = "hy:";
const char *_ms_script_prefix_bn = "bn:";
const char *_ms_script_prefix_iu = "iu";
const char *_ms_script_prefix_chr = "chr:";
const char *_ms_script_prefix_cop = "cop:";
const char *_ms_script_prefix_ru = "ru:";
const char *_ms_script_prefix_hi = "hi:";
const char *_ms_script_prefix_ka = "ka:";
const char *_ms_script_prefix_el = "el:";
const char *_ms_script_prefix_gu = "gu:";
const char *_ms_script_prefix_pa = "pa:";
const char *_ms_script_prefix_ko = "ko:";
const char *_ms_script_prefix_he = "he:";
const char *_ms_script_prefix_ja = "ja:";
const char *_ms_script_prefix_kn = "kn:";
const char *_ms_script_prefix_lo = "lo:";
const char *_ms_script_prefix_ml = "ml:";
const char *_ms_script_prefix_mn = "mn:";
const char *_ms_script_prefix_or = "or:";
const char *_ms_script_prefix_syr = "syr:";
const char *_ms_script_prefix_ta = "ta:";
const char *_ms_script_prefix_te = "te:";
const char *_ms_script_prefix_th = "th:";
const char *_ms_script_prefix_bo = "bo:";
const char *_ms_script_prefix_am = "am:";
const char *_ms_script_prefix_km = "km:";
const char *_ms_script_prefix_my = "my:";
const char *_ms_script_prefix_si = "si:";
const char *_ms_script_prefix_dv = "dv:";
const char *_ms_script_prefix_bku = "bku:";
const char *_ms_script_prefix_hnn = "hnn:";
const char *_ms_script_prefix_tl = "tl:";
const char *_ms_script_prefix_tbw = "tbw:";
const char *_ms_script_prefix_uga = "uga:";
const char *_ms_script_prefix_bug = "bug:";
const char *_ms_script_prefix_peo = "peo:";
const char *_ms_script_prefix_syl = "syl:";
const char *_ms_script_prefix_nko = "nko:";

const char *prefix_from_script(hb_script_t script) {
  switch (script) {
  case HB_SCRIPT_LATIN:
    return _ms_script_prefix_en;
  case HB_SCRIPT_ARABIC:
    return _ms_script_prefix_ar;
  case HB_SCRIPT_HAN:
    return _ms_script_prefix_cn;
  case HB_SCRIPT_ARMENIAN:
    return _ms_script_prefix_hy;
  case HB_SCRIPT_BENGALI:
    return _ms_script_prefix_bn;
  case HB_SCRIPT_CANADIAN_ABORIGINAL:
    return _ms_script_prefix_iu;
  case HB_SCRIPT_CHEROKEE:
    return _ms_script_prefix_chr;
  case HB_SCRIPT_COPTIC:
    return _ms_script_prefix_cop;
  case HB_SCRIPT_CYRILLIC:
    return _ms_script_prefix_ru;
  case HB_SCRIPT_DEVANAGARI:
    return _ms_script_prefix_hi;
  case HB_SCRIPT_GEORGIAN:
    return _ms_script_prefix_ka;
  case HB_SCRIPT_GREEK:
    return _ms_script_prefix_el;
  case HB_SCRIPT_GUJARATI:
    return _ms_script_prefix_gu;
  case HB_SCRIPT_GURMUKHI:
    return _ms_script_prefix_pa;
  case HB_SCRIPT_HANGUL:
    return _ms_script_prefix_ko;
  case HB_SCRIPT_HEBREW:
    return _ms_script_prefix_he;
  case HB_SCRIPT_HIRAGANA:
    return _ms_script_prefix_ja;
  case HB_SCRIPT_KANNADA:
    return _ms_script_prefix_kn;
  case HB_SCRIPT_KATAKANA:
    return _ms_script_prefix_ja;
  case HB_SCRIPT_LAO:
    return _ms_script_prefix_lo;
  case HB_SCRIPT_MALAYALAM:
    return _ms_script_prefix_ml;
  case HB_SCRIPT_MONGOLIAN:
    return _ms_script_prefix_mn;
  case HB_SCRIPT_ORIYA:
    return _ms_script_prefix_or;
  case HB_SCRIPT_SYRIAC:
    return _ms_script_prefix_syr;
  case HB_SCRIPT_TAMIL:
    return _ms_script_prefix_ta;
  case HB_SCRIPT_TELUGU:
    return _ms_script_prefix_te;
  case HB_SCRIPT_THAI:
    return _ms_script_prefix_th;
  case HB_SCRIPT_TIBETAN:
    return _ms_script_prefix_bo;
  case HB_SCRIPT_ETHIOPIC:
    return _ms_script_prefix_am;
  case HB_SCRIPT_KHMER:
    return _ms_script_prefix_km;
  case HB_SCRIPT_MYANMAR:
    return _ms_script_prefix_my;
  case HB_SCRIPT_SINHALA:
    return _ms_script_prefix_si;
  case HB_SCRIPT_THAANA:
    return _ms_script_prefix_dv;
  case HB_SCRIPT_BUHID:
    return _ms_script_prefix_bku;
  case HB_SCRIPT_HANUNOO:
    return _ms_script_prefix_hnn;
  case HB_SCRIPT_TAGALOG:
    return _ms_script_prefix_tl;
  case HB_SCRIPT_TAGBANWA:
    return _ms_script_prefix_tbw;
  case HB_SCRIPT_UGARITIC:
    return _ms_script_prefix_uga;
  case HB_SCRIPT_BUGINESE:
    return _ms_script_prefix_bug;
  case HB_SCRIPT_OLD_PERSIAN:
    return _ms_script_prefix_peo;
  case HB_SCRIPT_SYLOTI_NAGRI:
    return _ms_script_prefix_syl;
  case HB_SCRIPT_NKO:
    return _ms_script_prefix_nko;
  default:
    return NULL;
  }
}

hb_feature_t hbfeatures[2] = {{HB_TAG('v', 'e', 'r', 't'), 0, 0, INT_MAX},
                              {HB_TAG('k', 'e', 'r', 'n'), 0, 0, INT_MAX}};

static hb_bool_t _ms_get_glyph_func(hb_font_t *font, void *font_data,
                                    hb_codepoint_t unicode,
                                    hb_codepoint_t variation_selector,
                                    hb_codepoint_t *glyph, void *user_data)

{
  (void)font;
  (void)variation_selector;
  (void)user_data;
  /* first check our run, as we have probably already computed this */
  int i;
  struct _ms_hb_user_data *ud = font_data;
  unsigned int *unicodes = ud->info->unicodes + ud->run->offset;

  for (i = 0; i < ud->run->length; i++) {
    if (unicodes[i] == unicode) {
      *glyph = *(ud->info->codepoints + ud->run->offset + i);
      return *glyph != 0;
    }
  }

  {
    FT_Face ft_face = ud->run->face->face;

#ifdef HAVE_FT_FACE_GETCHARVARIANTINDEX
    if ((variation_selector)) {
      *glyph =
          FT_Face_GetCharVariantIndex(ft_face, unicode, variation_selector);
      return *glyph != 0;
    }
#endif

    *glyph = FT_Get_Char_Index(ft_face, unicode);
    return *glyph != 0;
  }
}

#if HB_VERSION_ATLEAST(1, 2, 3)
static hb_bool_t _ms_get_nominal_glyph_func(hb_font_t *font, void *font_data,
                                            hb_codepoint_t unicode,
                                            hb_codepoint_t *glyph,
                                            void *user_data) {
  return _ms_get_glyph_func(font, font_data, unicode, 0, glyph, user_data);
}

static hb_bool_t _ms_get_variation_glyph_func(hb_font_t *font, void *font_data,
                                              hb_codepoint_t unicode,
                                              hb_codepoint_t variation_selector,
                                              hb_codepoint_t *glyph,
                                              void *user_data) {
  return _ms_get_glyph_func(font, font_data, unicode, variation_selector, glyph,
                            user_data);
}
#endif

static hb_position_t _ms_get_glyph_h_advance_func(hb_font_t *font,
                                                  void *font_data,
                                                  hb_codepoint_t glyph,
                                                  void *user_data) {
  (void)font;
  (void)user_data;
  struct _ms_hb_user_data *ud = font_data;
  glyph_element *glyphc =
      msGetGlyphByIndex(ud->run->face, ud->glyph_size, glyph);
  if (!glyphc)
    return 0;
  return glyphc->metrics.advance * 64;
}

static hb_position_t _ms_get_glyph_v_advance_func(hb_font_t *font,
                                                  void *font_data,
                                                  hb_codepoint_t glyph,
                                                  void *user_data) {
  (void)font;
  (void)font_data;
  (void)glyph;
  (void)user_data;
  return 0; /* we don't support vertical layouts */
}
#endif

static int check_single_font(fontSetObj *fontset, char *fontkey, text_run *run,
                             TextInfo *glyphs, int ignore_missing) {
  int i;
  face_element *fcache = NULL;
  if (fontset && fontkey) {
    char *fontkey2 = strchr(fontkey, ':'); /* try skipping prefix */
    if (fontkey2) {
      fcache = msGetFontFace(fontkey2 + 1, fontset);
    }
  }
  if (!fcache)
    fcache = msGetFontFace(fontkey, fontset);
  run->face = fcache;
  if (MS_UNLIKELY(!fcache))
    return MS_FAILURE;
  for (i = 0; i < run->length; i++) {
    unsigned int codepoint =
        msGetGlyphIndex(fcache, glyphs->unicodes[run->offset + i]);
    if (codepoint || ignore_missing) {
      if (codepoint == 0) {
        msDebug("Unable to find glyph for codepoint %u. Using ? as fallback.\n",
                glyphs->unicodes[run->offset + i]);
        codepoint = msGetGlyphIndex(fcache, '?');
      }
      glyphs->codepoints[run->offset + i] = codepoint;
    } else
      return MS_FAILURE;
  }
  return MS_SUCCESS;
}

static int get_face_for_run(fontSetObj *fontset, char *fontlist, text_run *run,
                            TextInfo *glyphs) {
  char *startfont, *endfont;
#if defined(USE_HARFBUZZ) && defined(USE_FRIBIDI)
  const char *prefix = NULL;
#endif

  if (!fontset || !fontlist) {
    int ok = check_single_font(fontset, fontlist, run, glyphs, 0);
    (void)ok;
    return MS_SUCCESS;
  }

#if defined(USE_HARFBUZZ) && defined(USE_FRIBIDI)
  if (run->rtl >= 0) {
    prefix = prefix_from_script(run->script);
  } else {
    prefix = _ms_script_prefix_en;
  }

  if (prefix) {
    /* we'll first look for a font who's prefixed by the current script prefix,
     * e.g, given the fontlist "arial,ar:arialuni,cn:cjk" check the "cjk" font
     * first for HAN scripts
     */
    int prefixlen = strlen(prefix);
    startfont = fontlist;
    for (;;) {
      if (!*startfont)
        break;
      endfont = strchr(startfont, ',');
      if (!strncmp(startfont, prefix, prefixlen)) {
        startfont += strlen(prefix);
        if (endfont)
          *endfont = 0;
        int ok = check_single_font(fontset, startfont, run, glyphs, 0);
        if (endfont) {
          *endfont = ',';
          if (ok == MS_SUCCESS)
            return MS_SUCCESS;
          startfont = endfont + 1; /* go to next font in list */
        } else {
          if (ok == MS_SUCCESS)
            return MS_SUCCESS;
          break;
        }
      }
      if (endfont)
        startfont = endfont + 1;
      else
        break;
    }
  }
#endif

  /* no prefix, or prefix search didn't return satisfying result */
  startfont = fontlist;
  for (;;) {
    if (!*startfont)
      break;
    endfont = strchr(startfont, ',');
    if (endfont)
      *endfont = 0;
    int ok = check_single_font(fontset, startfont, run, glyphs,
                               !endfont); /* ignore failing glyphs if we're
                                             using the last font in the list */
    if (endfont) {
      *endfont = ',';
      if (ok == MS_SUCCESS)
        return MS_SUCCESS;
      startfont = endfont + 1; /* go to next font in list */
    } else {
      if (ok == MS_SUCCESS)
        return MS_SUCCESS;
      break;
    }
  }

  return MS_FAILURE;
}

#ifdef USE_HARFBUZZ
hb_font_t *get_hb_font(struct _ms_hb_user_data *font_data) {
  face_element *fcache = font_data->run->face;
  hb_font_element *hbf = fcache->hbfont;
  FT_Face face = fcache->face;
  int reqsize = MS_NINT(font_data->glyph_size * 96.0 / 72.0);

  if (reqsize != fcache->face->size->metrics.x_ppem) {
    FT_Set_Pixel_Sizes(face, 0, reqsize);
  }

  if (!hbf) {
    hbf = msSmallMalloc(sizeof(hb_font_element));
    hbf->hbparentfont = hb_ft_font_create(face, NULL);
    hbf->hbfont = hb_font_create_sub_font(hbf->hbparentfont);
    hbf->funcs = hb_font_funcs_create();
    hb_font_funcs_set_glyph_h_advance_func(
        hbf->funcs, _ms_get_glyph_h_advance_func, NULL, NULL);
#if HB_VERSION_ATLEAST(1, 2, 3)
    hb_font_funcs_set_nominal_glyph_func(hbf->funcs, _ms_get_nominal_glyph_func,
                                         NULL, NULL);
    hb_font_funcs_set_variation_glyph_func(
        hbf->funcs, _ms_get_variation_glyph_func, NULL, NULL);
#else
    hb_font_funcs_set_glyph_func(hbf->funcs, _ms_get_glyph_func, NULL, NULL);
#endif
    hb_font_funcs_set_glyph_v_advance_func(
        hbf->funcs, _ms_get_glyph_v_advance_func, NULL, NULL);
    hbf->cursize = reqsize;
    fcache->hbfont = hbf;
    hb_font_set_funcs(hbf->hbfont, hbf->funcs, font_data, NULL);
  } else {
    if (hbf->cursize != reqsize) {
      hb_font_set_scale(hbf->hbparentfont,
                        ((uint64_t)face->size->metrics.x_scale *
                         (uint64_t)face->units_per_EM) >>
                            16,
                        ((uint64_t)face->size->metrics.y_scale *
                         (uint64_t)face->units_per_EM) >>
                            16);
      hb_font_set_ppem(hbf->hbparentfont, face->size->metrics.x_ppem,
                       face->size->metrics.y_ppem);
      hbf->cursize = reqsize;
    }
  }
  hb_font_set_funcs_data(hbf->hbfont, font_data, NULL);
  return hbf->hbfont;
}

/*
 *  Return non-zero (true) if the given unicode array contains
 *  only ASCII and ISO Latin-1 characters, otherwise return zero.
 */
int unicode_is_latin1(const unsigned int *unicode, long nglyphs) {
  long i;

  for (i = 0; i < nglyphs; i++) {
    if (unicode[i] < 0x2B0)
      continue;
    return 0;
  }
  return 1;
}

void get_scripts(unsigned int *cp, int len, hb_script_t *scripts) {
  int i;
  int backwards_scan = 0;
  hb_unicode_funcs_t *ufuncs = hb_unicode_funcs_get_default();
  hb_script_t last_script = HB_SCRIPT_UNKNOWN;

  // determine script (forward scan)
  for (i = 0; i < len; i++) {
    scripts[i] = hb_unicode_script(ufuncs, cp[i]);

    // common/inherit codepoints inherit script from context
    if (scripts[i] == HB_SCRIPT_COMMON || scripts[i] == HB_SCRIPT_INHERITED) {
      // unknown is not a valid context
      if (last_script != HB_SCRIPT_UNKNOWN)
        scripts[i] = last_script;
      else
        // do a backwards scan to check if next codepoint
        // contains a valid script for context
        backwards_scan = 1;
    } else {
      last_script = scripts[i];
    }
  }

  // determine script (backwards scan, if needed)
  last_script = HB_SCRIPT_UNKNOWN;
  for (i = len - 1; i >= 0 && backwards_scan; i--) {
    // common/inherit codepoints inherit script from context
    if (scripts[i] == HB_SCRIPT_COMMON || scripts[i] == HB_SCRIPT_INHERITED) {
      // unknown script is not a valid context
      if (last_script != HB_SCRIPT_UNKNOWN)
        scripts[i] = last_script;
    } else {
      last_script = scripts[i];
    }
  }
}
#endif

/* returns 1 if this is a codepoint we should skip. only checks \r for now */
static int skip_unicode(unsigned int unicode) {
  switch (unicode) {
  case '\r':
    return 1;
    break;
  default:
    return 0;
  }
}

#define MS_RTL_LTR 0
#define MS_RTL_RTL 1
#define MS_RTL_MIXED 2

struct line_desc {
  int length;
  int rtl;
};

int msLayoutTextSymbol(mapObj *map, textSymbolObj *ts, textPathObj *tgret) {
#define STATIC_GLYPHS 100
#define STATIC_LINES 10
  text_run static_runs[STATIC_GLYPHS];
  int i, nruns, start, ret = MS_SUCCESS;
  size_t text_num_bytes;
  char *inp;
  unsigned int static_unicodes[STATIC_GLYPHS];
  unsigned int static_codepoints[STATIC_GLYPHS];
#ifdef USE_FRIBIDI
  FriBidiCharType static_ctypes[STATIC_GLYPHS];
  FriBidiLevel static_bidi_levels[STATIC_GLYPHS];
#endif
#ifdef USE_HARFBUZZ
  hb_script_t static_scripts[STATIC_GLYPHS];
  hb_buffer_t *buf = NULL;
#endif
  struct line_desc static_line_descs[STATIC_LINES];
  int alloc_glyphs = 0;
  struct line_desc *line_descs = NULL;
  text_run *runs;
  double oldpeny = 3455, peny,
         penx = 0; /*oldpeny is set to an unreasonable default initial value */
  fontSetObj *fontset = NULL;

  TextInfo glyphs;
  int num_glyphs = 0;

  assert(
      ts->annotext &&
      *ts->annotext); /* ensure we have at least one character/glyph to treat */

  if (map)
    fontset = &map->fontset;
    /* go through iconv beforehand, so we know we're handling utf8 */
#ifdef USE_ICONV
  if (ts->label->encoding && strcasecmp(ts->label->encoding, "UTF-8")) {
    iconv_t cd;
    size_t len, bufleft;
    char *encoded_text, *outp;
    len = strlen(ts->annotext);
    bufleft = len * 6;
    encoded_text = msSmallMalloc(bufleft + 1);
    cd = iconv_open("UTF-8", ts->label->encoding);

    if (cd == (iconv_t)-1) {
      msSetError(MS_IDENTERR, "Encoding not supported by libiconv (%s).",
                 "msGetEncodedString()", ts->label->encoding);
      return MS_FAILURE;
    }

    inp = ts->annotext;
    outp = encoded_text;

    while (len > 0) {
      const size_t iconv_status = iconv(cd, &inp, &len, &outp, &bufleft);
      if (iconv_status == (size_t)-1) {
        break;
      }
    }

    text_num_bytes = outp - encoded_text;
    encoded_text[text_num_bytes] = 0;
    free(ts->annotext);
    ts->annotext = encoded_text;
    iconv_close(cd);
  } else
#endif
  {
    text_num_bytes = strlen(ts->annotext);
  }

  if (text_num_bytes == 0)
    return 0;

  if (text_num_bytes > STATIC_GLYPHS) {
#ifdef USE_FRIBIDI
    glyphs.bidi_levels = msSmallMalloc(text_num_bytes * sizeof(FriBidiLevel));
    glyphs.ctypes = msSmallMalloc(text_num_bytes * sizeof(FriBidiCharType));
#endif
    glyphs.unicodes = msSmallMalloc(text_num_bytes * sizeof(unsigned int));
    glyphs.codepoints = msSmallMalloc(text_num_bytes * sizeof(unsigned int));
#ifdef USE_HARFBUZZ
    glyphs.scripts = msSmallMalloc(text_num_bytes * sizeof(hb_script_t));
#endif
    runs = msSmallMalloc(text_num_bytes * sizeof(text_run));
  } else {
#ifdef USE_FRIBIDI
    glyphs.bidi_levels = static_bidi_levels;
    glyphs.ctypes = static_ctypes;
#endif
    glyphs.unicodes = static_unicodes;
    glyphs.codepoints = static_codepoints;
#ifdef USE_HARFBUZZ
    glyphs.scripts = static_scripts;
#endif
    runs = static_runs;
  }

  /* populate the unicode entries once and for all */
  inp = ts->annotext;
  while (*inp) {
    unsigned int unicode;
    inp += msUTF8ToUniChar(inp, &unicode);
    if (!skip_unicode(unicode)) {
      glyphs.unicodes[num_glyphs++] = unicode;
    }
  }

  if (ts->label->wrap || ts->label->maxlength > 0) {
    if (ts->label->wrap && ts->label->maxlength == 0) {
      for (i = 0; i < num_glyphs; i++) {
        /* replace all occurrences of the wrap character with a newline */
        if (glyphs.unicodes[i] == (unsigned)ts->label->wrap)
          glyphs.unicodes[i] = '\n';
      }
    } else {
      assert(ts->label->maxlength > 0);
      if (num_glyphs > ts->label->maxlength) {
        int num_cur_glyph_on_line =
            0; /*count for the number of glyphs on the current line*/
        for (i = 0; i < num_glyphs; i++) {
          /* wrap at wrap character or at ZERO WIDTH SPACE (unicode 0x200b), if
           * current line is too long */
          if ((glyphs.unicodes[i] == (unsigned)ts->label->wrap ||
               glyphs.unicodes[i] == (unsigned)0x200b) &&
              num_cur_glyph_on_line >= ts->label->maxlength) {
            glyphs.unicodes[i] = '\n';
            num_cur_glyph_on_line = 0;
          } else {
            num_cur_glyph_on_line++;
          }
        }
      }
    }
    /*
     * TODO RFC98: RFC40 negative label->wrap. This is left out for the moment
     * as it requires handling a realloc and imho is never used and is an
     * overly-complex use-case.
     */
  }

  /* split our text into runs (one for each line) */
  nruns = 0;
  start = 0;
  runs[0].offset = 0;
  runs[0].line_number = 0;
  for (i = 0; i < num_glyphs; i++) {
    if (glyphs.unicodes[i] != '\n')
      continue;
    runs[nruns].length = i - start; /* length of current line (without \n) */
    start = i + 1;                  /* start of next line */
    runs[nruns + 1].line_number = runs[nruns].line_number + 1;
    runs[nruns + 1].offset = start;
    nruns++;
  }
  /* unless the last glyph was a \n, we need to "close" the last run */
  if (glyphs.unicodes[num_glyphs - 1] != '\n') {
    runs[nruns].length = num_glyphs - start;
    nruns++;
  }

  if (runs[nruns - 1].line_number + 1 > STATIC_LINES) {
    line_descs = msSmallMalloc((runs[nruns - 1].line_number + 1) *
                               sizeof(struct line_desc));
  } else {
    line_descs = static_line_descs;
  }

#ifdef USE_FRIBIDI
  for (i = 0; i < nruns; i++) {
    /* check the run (at this stage, one run per line), decide if we need to go
     * through bidi and/or shaping */
    if (unicode_is_latin1(glyphs.unicodes + runs[i].offset, runs[i].length)) {
      runs[i].rtl = -1;
      line_descs[i].rtl = MS_RTL_LTR;
    } else {
      runs[i].rtl = 0;
    }
  }

  for (i = 0; i < nruns; i++) {
    /* split the text into bidi runs */
    if (runs[i].rtl >= 0) {
      int j, original_num_glyphs, original_offset;
      FriBidiLevel prevlevel;
      FriBidiParType dir = FRIBIDI_PAR_LTR;
      original_offset = runs[i].offset;
      original_num_glyphs = runs[i].length;
      fribidi_get_bidi_types(glyphs.unicodes + original_offset, runs[i].length,
                             glyphs.ctypes + original_offset);
      {
        FriBidiLevel level = fribidi_get_par_embedding_levels(
            glyphs.ctypes + original_offset, runs[i].length, &dir,
            glyphs.bidi_levels + runs[i].offset);
        (void)level;
      }
      /* if we have different embedding levels, create a run for each one */
      runs[i].rtl = prevlevel = glyphs.bidi_levels[original_offset];
      line_descs[runs[i].line_number].rtl =
          (prevlevel % 2) ? MS_RTL_RTL : MS_RTL_LTR;
      for (j = 1; j < original_num_glyphs; j++) {
        if (glyphs.bidi_levels[original_offset + j] != prevlevel) {
          line_descs[runs[i].line_number].rtl = MS_RTL_MIXED;
          /* create a new run for the different embedding level */
          nruns++;

          /* first move remaining runs */
          memmove(runs + i + 2, runs + i + 1,
                  (nruns - i - 2) * sizeof(text_run));

          i++;
          /* new run inherints line number */
          runs[i].line_number = runs[i - 1].line_number;
          runs[i].offset = original_offset + j;
          runs[i].length =
              (runs[i - 1].offset + runs[i - 1].length) - runs[i].offset;
          runs[i - 1].length = runs[i].offset - runs[i - 1].offset;

          /* new run starts at current position */
          runs[i].rtl = prevlevel = glyphs.bidi_levels[original_offset + j];
        }
      }
    }
  }
#else
  for (i = 0; i < nruns; i++) {
    line_descs[i].rtl = MS_RTL_LTR;
  }
#endif

#ifdef USE_FRIBIDI
  /* determine the scripts of each run, and split again into runs with identical
   * script */
  for (i = 0; i < nruns; i++) {
    if (runs[i].rtl == -1) {
      runs[i].script = HB_SCRIPT_LATIN;
      continue; /* skip runs we have determined we are latin (no shaping needed)
                 */
    } else {
      int j, original_num_glyphs, original_offset;
      hb_script_t prevscript;
      original_offset = runs[i].offset;
      original_num_glyphs = runs[i].length;
      get_scripts(glyphs.unicodes + original_offset, runs[i].length,
                  glyphs.scripts + original_offset);
      /* if we have different scripts, create a run for each one */
      runs[i].script = prevscript = glyphs.scripts[original_offset];
      for (j = 1; j < original_num_glyphs; j++) {
        if (glyphs.scripts[original_offset + j] != prevscript) {
          /* create a new run for the different embedding level */
          nruns++;

          /* first move remaining runs */
          memmove(runs + i + 2, runs + i + 1,
                  (nruns - i - 2) * sizeof(text_run));

          i++;
          /* new run inherints line number and rtl*/
          runs[i].line_number = runs[i - 1].line_number;
          runs[i].rtl = runs[i - 1].rtl;
          runs[i].offset = original_offset + j;
          runs[i].length =
              (runs[i - 1].offset + runs[i - 1].length) - runs[i].offset;
          runs[i - 1].length = runs[i].offset - runs[i - 1].offset;

          runs[i].script = prevscript = glyphs.scripts[original_offset + j];
        }
      }
    }
  }
#endif

  for (i = 0; i < nruns; i++) {
    ret = get_face_for_run(fontset, ts->label->font, runs + i, &glyphs);
    if (MS_UNLIKELY(ret == MS_FAILURE))
      goto cleanup;
  }

  /*
   * determine the font face to use for a given run. No splitting needed here
   * for now, as we suppose that the decomposition of each run into individual
   * bidi direction and script level is sufficient to ensure that a given run
   * can be represented by a single font (i.e. there's no need to look into
   * multiple fonts to find the glyphs of the run)
   */

  tgret->numlines = runs[nruns - 1].line_number + 1;
  tgret->bounds.bbox.minx = 0;
  tgret->bounds.bbox.miny = FLT_MAX;
  tgret->bounds.bbox.maxx = tgret->bounds.bbox.maxy = -FLT_MAX;

  for (i = 0; i < nruns; i++) {
    if (!runs[i].face)
      continue;
    peny = (1 - tgret->numlines + runs[i].line_number) * tgret->line_height;
    if (peny != oldpeny) {
      if (i > 0)
        line_descs[runs[i - 1].line_number].length = penx;
      if (penx > tgret->bounds.bbox.maxx)
        tgret->bounds.bbox.maxx = penx;
      oldpeny = peny;
      penx = 0;
    }
#if defined(USE_HARFBUZZ) && defined(USE_FRIBIDI)
    if (runs[i].rtl == -1 || runs[i].script == HB_SCRIPT_LATIN ||
        runs[i].script == HB_SCRIPT_COMMON)
#endif
    {
      /* use our basic shaper */
      unsigned int *codepoint = glyphs.codepoints + runs[i].offset;
      alloc_glyphs += runs[i].length;
      tgret->glyphs =
          msSmallRealloc(tgret->glyphs, alloc_glyphs * sizeof(glyphObj));
      for (int j = 0; j < runs[i].length; j++) {
        glyphObj *g = &tgret->glyphs[tgret->numglyphs + j];
        g->glyph =
            msGetGlyphByIndex(runs[i].face, tgret->glyph_size, *codepoint);
        g->face = runs[i].face;
        codepoint++;
        g->pnt.x = penx;
        g->pnt.y = peny;
        g->rot = 0.0;
        penx += g->glyph->metrics.advance;
        if (runs[i].line_number == 0 &&
            peny - g->glyph->metrics.maxy <
                tgret->bounds.bbox
                    .miny) /*compute minimal y, only for the first line */
          tgret->bounds.bbox.miny = peny - g->glyph->metrics.maxy;
        if (peny - g->glyph->metrics.miny > tgret->bounds.bbox.maxy)
          tgret->bounds.bbox.maxy = peny - g->glyph->metrics.miny;
      }
#if defined(USE_HARFBUZZ) && defined(USE_FRIBIDI)
    } else {
      struct _ms_hb_user_data user_data;
      hb_font_t *font;
      hb_glyph_info_t *glyph_info;
      hb_glyph_position_t *glyph_pos;
      if (!buf) {
        buf = hb_buffer_create();
      }
      user_data.info = &glyphs;
      user_data.run = runs + i;
      user_data.glyph_size = tgret->glyph_size;
      hb_buffer_clear_contents(buf);
      hb_buffer_set_script(buf, runs[i].script);
      font = get_hb_font(&user_data);
      hb_buffer_set_direction(buf, (runs[i].rtl % 2) ? HB_DIRECTION_RTL
                                                     : HB_DIRECTION_LTR);
      hb_buffer_add_utf32(buf, glyphs.unicodes + runs[i].offset, runs[i].length,
                          0, runs[i].length);
      hb_shape(font, buf, hbfeatures, 2);

      unsigned int glyph_count;
      glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
      glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
      alloc_glyphs += glyph_count;
      tgret->glyphs =
          msSmallRealloc(tgret->glyphs, alloc_glyphs * sizeof(glyphObj));
      for (unsigned j = 0; j < glyph_count; j++) {
        glyphObj *g = &tgret->glyphs[tgret->numglyphs + j];
        g->glyph = msGetGlyphByIndex(runs[i].face, tgret->glyph_size,
                                     glyph_info[j].codepoint);
        g->face = runs[i].face;
        g->pnt.x = penx + glyph_pos[j].x_offset / 64;
        g->pnt.y = peny - glyph_pos[j].y_offset / 64;
        g->rot = 0;
        penx += glyph_pos[j].x_advance / 64.0;
        /* peny -= glyph_pos[j].y_advance/64; // we don't do vertical layouts */
        if (runs[i].line_number == 0 &&
            peny - g->glyph->metrics.maxy <
                tgret->bounds.bbox
                    .miny) /*compute minimal y, only for the first line */
          tgret->bounds.bbox.miny = peny - g->glyph->metrics.maxy;
        if (peny - g->glyph->metrics.miny > tgret->bounds.bbox.maxy)
          tgret->bounds.bbox.maxy = peny - g->glyph->metrics.miny;
      }
#endif
    }
    tgret->numglyphs = alloc_glyphs;
    line_descs[runs[nruns - 1].line_number].length = penx;
    if (penx > tgret->bounds.bbox.maxx)
      tgret->bounds.bbox.maxx = penx;
  }

#ifdef USE_HARFBUZZ
  if (buf) {
    hb_buffer_destroy(buf);
  }
#endif

  if (tgret->numlines > 1) {
    int max_line_length = 0;
    int line = -1;
    double cur_line_offset = 0;
    int prev_default_align =
        MS_ALIGN_LEFT; /* if we have mixed rtl status, use the alignment of the
                          previous line. this defaults to left-alignment if the
                          first line is mixed */
    int cur_default_align = 0;
    for (i = 0; i < tgret->numlines; i++) {
      if (line_descs[i].length > max_line_length) {
        max_line_length = line_descs[i].length;
      }
    }
    oldpeny = 3455;
    for (i = 0; i < alloc_glyphs; i++) {
      if (tgret->glyphs[i].pnt.y != oldpeny) {
        oldpeny = tgret->glyphs[i].pnt.y;
        line++;
        /* compute offset to apply to coming line */
        switch (ts->label->align) {
        case MS_ALIGN_CENTER:
          cur_line_offset = (max_line_length - line_descs[line].length) / 2.0;
          break;
        case MS_ALIGN_RIGHT:
          cur_line_offset = (max_line_length - line_descs[line].length);
          break;
        case MS_ALIGN_LEFT:
          cur_line_offset = 0;
          break;
        case MS_ALIGN_DEFAULT:
        default:
          switch (line_descs[line].rtl) {
          case MS_RTL_MIXED:
            cur_default_align = prev_default_align;
            break;
          case MS_RTL_RTL:
            cur_default_align = prev_default_align = MS_RTL_RTL;
            break;
          case MS_RTL_LTR:
            cur_default_align = prev_default_align = MS_RTL_LTR;
            break;
          }
          switch (cur_default_align) {
          case MS_RTL_RTL:
            /* align to the right */
            cur_line_offset = (max_line_length - line_descs[line].length);
            break;
          case MS_RTL_LTR:
            cur_line_offset = 0;
            break;
          }
        }
      }
      tgret->glyphs[i].pnt.x += cur_line_offset;
    }
  }
  /*
   * msDebug("bounds for %s: %f %f %f
   * %f\n",ts->annotext,tgret->bounds.bbox.minx,tgret->bounds.bbox.miny,tgret->bounds.bbox.maxx,tgret->bounds.bbox.maxy);
   */

cleanup:
  if (line_descs != static_line_descs)
    free(line_descs);
  if (glyphs.codepoints != static_codepoints) {
#ifdef USE_FRIBIDI
    free(glyphs.bidi_levels);
    free(glyphs.ctypes);
#endif
    free(glyphs.codepoints);
#ifdef USE_HARFBUZZ
    free(glyphs.scripts);
#endif
    free(glyphs.unicodes);
    free(runs);
  }
  return ret;
}

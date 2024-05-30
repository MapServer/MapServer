/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implements imagemap outputformat support.
 * Author:   Attila Csipa
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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
#include "dxfcolor.h"

#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define MYDEBUG 0
#define DEBUG_IF if (MYDEBUG > 2)

/*
 * Client-side imagemap support was originally written by
 * Attila Csipa (http://prometheus.org.yu/me.php).  C. Scott Ananian
 * (http://cscott.net) cleaned up the code somewhat and made it more flexible:
 * you can now customize the generated HREFs and create mouseover and
 * mouseout attributes.
 *
 * Use
 *   IMAGETYPE imagemap
 * to select this driver.
 *
 * The following FORMATOPTIONs are available. If any are set to the empty
 * string the associated attribute will be suppressed.
 *   POLYHREF   the href string to use for the <area> elements.
 *              use a %s to interpolate the area title.
 *              default:   "javascript:Clicked('%s');"
 *   POLYMOUSEOVER the onMouseOver string to use for the <area> elements.
 *              use a %s to interpolate the area title.
 *              default:   "" (ie the attribute is suppressed)
 *   POLYMOUSEOUT the onMouseOut string to use for the <area> elements.
 *              use a %s to interpolate the area title.
 *              default:   "" (ie the attribute is suppressed)
 *   SYMBOLMOUSEOVER and SYMBOLMOUSEOUT are equivalent properties for
 *              <area> tags representing symbols, with the same defaults.
 *   MAPNAME    the string which will be used in the name attribute
 *              of the <map> tag.  There is no %s interpolation.
 *              default: "map1"
 *   SUPPRESS   if "yes", then we will suppress area declarations with
 *              no title.
 *              default: "NO"
 *
 * For example, http://vevo.verifiedvoting.org/verifier contains this
 * .map file fragment:
 *         OUTPUTFORMAT
 *              NAME imagemap
 *              DRIVER imagemap
 *              FORMATOPTION "POLYHREF=/verifier/map.php?state=%s"
 *              FORMATOPTION "SYMBOLHREF=#"
 *              FORMATOPTION "SUPPRESS=YES"
 *              FORMATOPTION "MAPNAME=map-48"
 *              FORMATOPTION "POLYMOUSEOUT=return nd();"
 *              FORMATOPTION "POLYMOUSEOVER=return overlib('%s');"
 *         END
 */

/*-------------------------------------------------------------------------*/

/* A pString is a variable-length (appendable) string, stored as a triple:
 * character pointer, allocated length, and used length.  The one wrinkle
 * is that we use pointers to the allocated size and character pointer,
 * in order to support referring to fields declared elsewhere (ie in the
 * 'image' data structure) for these.  The 'iprintf' function appends
 * to a pString. */
typedef struct pString {
  /* these two fields are somewhere else */
  char **string;
  int *alloc_size;
  /* this field is stored locally. */
  int string_len;
} pString;
/* These are the pStrings we declare statically.  One is the 'output'
 * imagemap/dxf file; parts of this live in another data structure and
 * are only referenced indirectly here.  The other contains layer-specific
 * information for the dxf output. */
static char *layerlist = NULL;
static int layersize = 0;
static pString imgStr, layerStr = {&layerlist, &layersize, 0};

/* Format strings for the various bits of a client-side imagemap. */
static const char *polyHrefFmt, *polyMOverFmt, *polyMOutFmt;
static const char *symbolHrefFmt, *symbolMOverFmt, *symbolMOutFmt;
static const char *mapName;
/* Should we suppress AREA declarations in imagemaps with no title? */
static int suppressEmpty = 0;

/* Prevent buffer-overrun and format-string attacks by "sanitizing" any
 * provided format string to fit the number of expected string arguments
 * (MAX) exactly. */
static const char *makeFmtSafe(const char *fmt, int MAX) {
  /* format string should have exactly 'MAX' %s */

  char *result = msSmallMalloc(strlen(fmt) + 1 + 3 * MAX), *cp;
  int numstr = 0, saw_percent = 0;

  strcpy(result, fmt);
  for (cp = result; *cp; cp++) {
    if (saw_percent) {
      if (*cp == '%') {
        /* safe */
      } else if (*cp == 's' && numstr < MAX) {
        numstr++; /* still safe */
      } else {
        /* disable this unsafe format string! */
        *(cp - 1) = ' ';
      }
      saw_percent = 0;
    } else if (*cp == '%')
      saw_percent = 1;
  }
  /* fixup format strings without enough %s in them */
  while (numstr < MAX) {
    strcpy(cp, "%.s"); /* print using zero-length precision */
    cp += 3;
    numstr++;
  }
  return result;
}

/* Append the given printf-style formatted string to the pString 'ps'.
 * This is much cleaner (and faster) than the technique this file
 * used to use! */
static void im_iprintf(pString *ps, const char *fmt, ...) {
  int n, remaining;
  va_list ap;
  do {
    remaining = *(ps->alloc_size) - ps->string_len;
    va_start(ap, fmt);
#if defined(HAVE_VSNPRINTF)
    n = vsnprintf((*(ps->string)) + ps->string_len, remaining, fmt, ap);
#else
    /* If vsnprintf() is not available then require a minimum
     * of 512 bytes of free space to prevent a buffer overflow
     * This is not fully bulletproof but should help, see bug 1613
     */
    if (remaining < 512)
      n = -1;
    else
      n = vsprintf((*(ps->string)) + ps->string_len, fmt, ap);
#endif
    va_end(ap);
    /* if that worked, we're done! */
    if (-1 < n && n < remaining) {
      ps->string_len += n;
      return;
    } else {                  /* double allocated string size */
      *(ps->alloc_size) *= 2; /* these keeps realloc linear.*/
      if (*(ps->alloc_size) < 1024)
        /* careful: initial size can be 0 */
        *(ps->alloc_size) = 1024;
      if (n > -1 && *(ps->alloc_size) <= (n + ps->string_len))
        /* ensure at least as much as what is needed */
        *(ps->alloc_size) = n + ps->string_len + 1;
      *(ps->string) = (char *)msSmallRealloc(*(ps->string), *(ps->alloc_size));
      /* if realloc doesn't work, we're screwed! */
    }
  } while (1); /* go back and try again. */
}

static int lastcolor = -1;
static int matchdxfcolor(colorObj col) {
  int best = 7;
  int delta = 128 * 255;
  int tcolor = 0;
  if (lastcolor != -1)
    return lastcolor;
  while (tcolor < 256 &&
         (ctable[tcolor].r != col.red || ctable[tcolor].g != col.green ||
          ctable[tcolor].b != col.blue)) {
    const int dr = ctable[tcolor].r - col.red;
    const int dg = ctable[tcolor].g - col.green;
    const int db = ctable[tcolor].b - col.blue;
    const int newdelta = dr * dr + dg * dg + db * db;
    if (newdelta < delta) {
      best = tcolor;
      delta = newdelta;
    }
    tcolor++;
  }
  if (tcolor >= 256)
    tcolor = best;
  /* DEBUG_IF printf("%d/%d/%d (%d/%d - %d), %d : %d/%d/%d<BR>\n",
   * ctable[tcolor].r, ctable[tcolor].g, ctable[tcolor].b, tcolor, best, delta,
   * lastcolor, col.red, col.green, col.blue); */
  lastcolor = tcolor;
  return tcolor;
}

static char *lname;
static int dxf;

void msImageStartLayerIM(mapObj *map, layerObj *layer, imageObj *image) {
  (void)map;
  (void)image;
  DEBUG_IF printf("ImageStartLayerIM\n<BR>");
  free(lname);
  if (layer->name)
    lname = msStrdup(layer->name);
  else
    lname = msStrdup("NONE");
  if (dxf == 2) {
    im_iprintf(&layerStr, "LAYER\n%s\n", lname);
  } else if (dxf) {
    im_iprintf(&layerStr,
               "  0\nLAYER\n  2\n%s\n"
               " 70\n  64\n 6\nCONTINUOUS\n",
               lname);
  }
  lastcolor = -1;
}

/*
 * Utility function to create a IM image. Returns
 * a pointer to an imageObj structure.
 */
imageObj *msImageCreateIM(int width, int height, outputFormatObj *format,
                          char *imagepath, char *imageurl, double resolution,
                          double defresolution) {
  imageObj *image = NULL;
  if (setvbuf(stdout, NULL, _IONBF, 0)) {
    printf("Whoops...");
  };
  DEBUG_IF printf("msImageCreateIM<BR>\n");
  if (width > 0 && height > 0) {
    image = (imageObj *)msSmallCalloc(1, sizeof(imageObj));
    imgStr.string = &(image->img.imagemap);
    imgStr.alloc_size = &(image->size);

    image->format = format;
    format->refcount++;

    image->width = width;
    image->height = height;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->resolution = resolution;
    image->resolutionfactor = resolution / defresolution;

    if (strcasecmp("ON", msGetOutputFormatOption(format, "DXF", "OFF")) == 0) {
      dxf = 1;
      im_iprintf(&layerStr, "  2\nLAYER\n 70\n  10\n");
    } else
      dxf = 0;

    if (strcasecmp("ON", msGetOutputFormatOption(format, "SCRIPT", "OFF")) ==
        0) {
      dxf = 2;
      im_iprintf(&layerStr, "");
    }

    /* get href formation string options */
    polyHrefFmt =
        makeFmtSafe(msGetOutputFormatOption(format, "POLYHREF",
                                            "javascript:Clicked('%s');"),
                    1);
    polyMOverFmt =
        makeFmtSafe(msGetOutputFormatOption(format, "POLYMOUSEOVER", ""), 1);
    polyMOutFmt =
        makeFmtSafe(msGetOutputFormatOption(format, "POLYMOUSEOUT", ""), 1);
    symbolHrefFmt =
        makeFmtSafe(msGetOutputFormatOption(format, "SYMBOLHREF",
                                            "javascript:SymbolClicked();"),
                    1);
    symbolMOverFmt =
        makeFmtSafe(msGetOutputFormatOption(format, "SYMBOLMOUSEOVER", ""), 1);
    symbolMOutFmt =
        makeFmtSafe(msGetOutputFormatOption(format, "SYMBOLMOUSEOUT", ""), 1);
    /* get name of client-side image map */
    mapName = msGetOutputFormatOption(format, "MAPNAME", "map1");
    /* should we suppress area declarations with no title? */
    if (strcasecmp("YES", msGetOutputFormatOption(format, "SUPPRESS", "NO")) ==
        0) {
      suppressEmpty = 1;
    }

    lname = msStrdup("NONE");
    *(imgStr.string) = msStrdup("");
    if (*(imgStr.string)) {
      *(imgStr.alloc_size) = imgStr.string_len = strlen(*(imgStr.string));
    } else {
      *(imgStr.alloc_size) = imgStr.string_len = 0;
    }
    if (imagepath) {
      image->imagepath = msStrdup(imagepath);
    }
    if (imageurl) {
      image->imageurl = msStrdup(imageurl);
    }

    return image;
  } else {
    msSetError(MS_IMGERR, "Cannot create IM image of size %d x %d.",
               "msImageCreateIM()", width, height);
  }
  return image;
}

/* ------------------------------------------------------------------------- */
/* Draw a single marker symbol of the specified size and color               */
/* ------------------------------------------------------------------------- */
void msDrawMarkerSymbolIM(mapObj *map, imageObj *img, pointObj *p,
                          styleObj *style, double scalefactor) {
  symbolObj *symbol;
  int ox, oy;
  double size;

  DEBUG_IF printf("msDrawMarkerSymbolIM\n<BR>");

  /* skip this, we don't do text */

  if (!p)
    return;

  if (style->symbol > map->symbolset.numsymbols || style->symbol < 0)
    return; /* no such symbol, 0 is OK */
  symbol = map->symbolset.symbol[style->symbol];
  ox = style->offsetx * scalefactor;
  oy = style->offsety * scalefactor;
  if (style->size == -1) {
    size = msSymbolGetDefaultSize(symbol);
    size = MS_NINT(size * scalefactor);
  } else
    size = MS_NINT(style->size * scalefactor);
  size = MS_MAX(size, style->minsize * img->resolutionfactor);
  size = MS_MIN(size, style->maxsize * img->resolutionfactor);

  if (size < 1)
    return; /* size too small */

  /* DEBUG_IF printf(".%d.%d.%d.", symbol->type, style->symbol, fc); */
  if (style->symbol ==
      0) { /* simply draw a single pixel of the specified color */

    if (dxf) {
      if (dxf == 2)
        im_iprintf(&imgStr, "POINT\n%.0f %.0f\n%d\n", p->x + ox, p->y + oy,
                   matchdxfcolor(style->color));
      else
        im_iprintf(&imgStr,
                   "  0\nPOINT\n 10\n%f\n 20\n%f\n 30\n0.0\n"
                   " 62\n%6d\n  8\n%s\n",
                   p->x + ox, p->y + oy, matchdxfcolor(style->color), lname);
    } else {
      im_iprintf(&imgStr, "<area ");
      if (strcmp(symbolHrefFmt, "%.s") != 0) {
        im_iprintf(&imgStr, "href=\"");
        im_iprintf(&imgStr, symbolHrefFmt, lname);
        im_iprintf(&imgStr, "\" ");
      }
      if (strcmp(symbolMOverFmt, "%.s") != 0) {
        im_iprintf(&imgStr, "onMouseOver=\"");
        im_iprintf(&imgStr, symbolMOverFmt, lname);
        im_iprintf(&imgStr, "\" ");
      }
      if (strcmp(symbolMOutFmt, "%.s") != 0) {
        im_iprintf(&imgStr, "onMouseOut=\"");
        im_iprintf(&imgStr, symbolMOutFmt, lname);
        im_iprintf(&imgStr, "\" ");
      }
      im_iprintf(&imgStr, "shape=\"circle\" coords=\"%.0f,%.0f, 3\" />\n",
                 p->x + ox, p->y + oy);
    }

    /* point2 = &( p->line[j].point[i] ); */
    /* if(point1->y == point2->y) {} */
    return;
  }
  DEBUG_IF printf("A");
  switch (symbol->type) {
  case (MS_SYMBOL_TRUETYPE):
    DEBUG_IF printf("T");
    break;
  case (MS_SYMBOL_PIXMAP):
    DEBUG_IF printf("P");
    break;
  case (MS_SYMBOL_VECTOR):
    DEBUG_IF printf("V");
    {
      double d, offset_x, offset_y;

      d = size / symbol->sizey;
      offset_x = MS_NINT(p->x - d * .5 * symbol->sizex + ox);
      offset_y = MS_NINT(p->y - d * .5 * symbol->sizey + oy);

      /* For now I only render filled vector symbols in imagemap, and no */
      /* dxf support available yet.  */
      if (symbol->filled && !dxf /* && fc >= 0 */) {
        /* char *title=(p->numvalues) ? p->values[0] : ""; */
        char *title = "";
        int j;

        im_iprintf(&imgStr, "<area ");
        if (strcmp(symbolHrefFmt, "%.s") != 0) {
          im_iprintf(&imgStr, "href=\"");
          im_iprintf(&imgStr, symbolHrefFmt, lname);
          im_iprintf(&imgStr, "\" ");
        }
        if (strcmp(symbolMOverFmt, "%.s") != 0) {
          im_iprintf(&imgStr, "onMouseOver=\"");
          im_iprintf(&imgStr, symbolMOverFmt, lname);
          im_iprintf(&imgStr, "\" ");
        }
        if (strcmp(symbolMOutFmt, "%.s") != 0) {
          im_iprintf(&imgStr, "onMouseOut=\"");
          im_iprintf(&imgStr, symbolMOutFmt, lname);
          im_iprintf(&imgStr, "\" ");
        }

        im_iprintf(&imgStr, "title=\"%s\" shape=\"poly\" coords=\"", title);

        for (j = 0; j < symbol->numpoints; j++) {
          im_iprintf(&imgStr, "%s %d,%d", j == 0 ? "" : ",",
                     (int)MS_NINT(d * symbol->points[j].x + offset_x),
                     (int)MS_NINT(d * symbol->points[j].y + offset_y));
        }
        im_iprintf(&imgStr, "\" />\n");
      } /* end of imagemap, filled case. */
    }
    break;

  default:
    DEBUG_IF printf("DEF");
    break;
  } /* end switch statement */

  return;
}

/* ------------------------------------------------------------------------- */
/* Draw a line symbol of the specified size and color                        */
/* ------------------------------------------------------------------------- */
void msDrawLineSymbolIM(mapObj *map, imageObj *img, shapeObj *p,
                        styleObj *style, double scalefactor_ignored) {
  (void)img;
  (void)scalefactor_ignored;
  symbolObj *symbol;
  int i, j, l;
  char first = 1;
  DEBUG_IF printf("msDrawLineSymbolIM<BR>\n");

  if (!p)
    return;
  if (p->numlines <= 0)
    return;

  if (style->symbol > map->symbolset.numsymbols || style->symbol < 0)
    return; /* no such symbol, 0 is OK */
  symbol = map->symbolset.symbol[style->symbol];

  if (suppressEmpty && p->numvalues == 0)
    return;                 /* suppress area with empty title */
  if (style->symbol == 0) { /* just draw a single width line */
    for (l = 0, j = 0; j < p->numlines; j++) {
      if (dxf == 2) {
        im_iprintf(&imgStr, "LINE\n%d\n", matchdxfcolor(style->color));
      } else if (dxf) {
        im_iprintf(&imgStr, "  0\nPOLYLINE\n 70\n     0\n 62\n%6d\n  8\n%s\n",
                   matchdxfcolor(style->color), lname);
      } else {
        char *title;

        first = 1;
        title = (p->numvalues) ? p->values[0] : "";
        im_iprintf(&imgStr, "<area ");
        if (strcmp(polyHrefFmt, "%.s") != 0) {
          im_iprintf(&imgStr, "href=\"");
          im_iprintf(&imgStr, polyHrefFmt, title);
          im_iprintf(&imgStr, "\" ");
        }
        if (strcmp(polyMOverFmt, "%.s") != 0) {
          im_iprintf(&imgStr, "onMouseOver=\"");
          im_iprintf(&imgStr, polyMOverFmt, title);
          im_iprintf(&imgStr, "\" ");
        }
        if (strcmp(polyMOutFmt, "%.s") != 0) {
          im_iprintf(&imgStr, "onMouseOut=\"");
          im_iprintf(&imgStr, polyMOutFmt, title);
          im_iprintf(&imgStr, "\" ");
        }
        im_iprintf(&imgStr, "title=\"%s\" shape=\"poly\" coords=\"", title);
      }
      /* point1 = &( p->line[j].point[p->line[j].numpoints-1] ); */
      for (i = 0; i < p->line[j].numpoints; i++, l++) {
        if (dxf == 2) {
          im_iprintf(&imgStr, "%.0f %.0f\n", p->line[j].point[i].x,
                     p->line[j].point[i].y);
        } else if (dxf) {
          im_iprintf(&imgStr, "  0\nVERTEX\n 10\n%f\n 20\n%f\n 30\n%f\n",
                     p->line[j].point[i].x, p->line[j].point[i].y, 0.0);
        } else {
          im_iprintf(&imgStr, "%s %.0f,%.0f", first ? "" : ",",
                     p->line[j].point[i].x, p->line[j].point[i].y);
        }
        first = 0;

        /* point2 = &( p->line[j].point[i] ); */
        /* if(point1->y == point2->y) {} */
      }
      im_iprintf(&imgStr, dxf ? (dxf == 2 ? "" : "  0\nSEQEND\n") : "\" />\n");
    }

    /* DEBUG_IF printf ("%d, ",strlen(img->img.imagemap) ); */
    return;
  }

  DEBUG_IF printf("-%d-", symbol->type);

  return;
}

/* ------------------------------------------------------------------------- */
/* Draw a shade symbol of the specified size and color                       */
/* ------------------------------------------------------------------------- */
void msDrawShadeSymbolIM(mapObj *map, imageObj *img, shapeObj *p,
                         styleObj *style, double scalefactor_ignored) {
  (void)img;
  (void)scalefactor_ignored;
  symbolObj *symbol;
  int i, j, l;
  char first = 1;

  DEBUG_IF printf("msDrawShadeSymbolIM\n<BR>");
  if (!p)
    return;
  if (p->numlines <= 0)
    return;

  symbol = map->symbolset.symbol[style->symbol];

  if (suppressEmpty && p->numvalues == 0)
    return; /* suppress area with empty title */
  if (style->symbol ==
      0) { /* simply draw a single pixel of the specified color //     */
    for (l = 0, j = 0; j < p->numlines; j++) {
      if (dxf == 2) {
        im_iprintf(&imgStr, "POLY\n%d\n", matchdxfcolor(style->color));
      } else if (dxf) {
        im_iprintf(&imgStr, "  0\nPOLYLINE\n 73\n     1\n 62\n%6d\n  8\n%s\n",
                   matchdxfcolor(style->color), lname);
      } else {
        char *title = (p->numvalues) ? p->values[0] : "";
        first = 1;
        im_iprintf(&imgStr, "<area ");
        if (strcmp(polyHrefFmt, "%.s") != 0) {
          im_iprintf(&imgStr, "href=\"");
          im_iprintf(&imgStr, polyHrefFmt, title);
          im_iprintf(&imgStr, "\" ");
        }
        if (strcmp(polyMOverFmt, "%.s") != 0) {
          im_iprintf(&imgStr, "onMouseOver=\"");
          im_iprintf(&imgStr, polyMOverFmt, title);
          im_iprintf(&imgStr, "\" ");
        }
        if (strcmp(polyMOutFmt, "%.s") != 0) {
          im_iprintf(&imgStr, "onMouseOut=\"");
          im_iprintf(&imgStr, polyMOutFmt, title);
          im_iprintf(&imgStr, "\" ");
        }
        im_iprintf(&imgStr, "title=\"%s\" shape=\"poly\" coords=\"", title);
      }

      /* point1 = &( p->line[j].point[p->line[j].numpoints-1] ); */
      for (i = 0; i < p->line[j].numpoints; i++, l++) {
        if (dxf == 2) {
          im_iprintf(&imgStr, "%.0f %.0f\n", p->line[j].point[i].x,
                     p->line[j].point[i].y);
        } else if (dxf) {
          im_iprintf(&imgStr, "  0\nVERTEX\n 10\n%f\n 20\n%f\n 30\n%f\n",
                     p->line[j].point[i].x, p->line[j].point[i].y, 0.0);
        } else {
          im_iprintf(&imgStr, "%s %.0f,%.0f", first ? "" : ",",
                     p->line[j].point[i].x, p->line[j].point[i].y);
        }
        first = 0;

        /* point2 = &( p->line[j].point[i] ); */
        /* if(point1->y == point2->y) {} */
      }
      im_iprintf(&imgStr, dxf ? (dxf == 2 ? "" : "  0\nSEQEND\n") : "\" />\n");
    }

    return;
  }
  /* DEBUG_IF printf ("d"); */
  DEBUG_IF printf("-%d-", symbol->type);
  return;
}

/*
 * Simply draws a label based on the label point and the supplied label object.
 */
int msDrawTextIM(mapObj *map, imageObj *img, pointObj labelPnt, char *string,
                 labelObj *label, double scalefactor) {
  (void)map;
  (void)img;
  DEBUG_IF printf("msDrawText<BR>\n");
  if (!string)
    return (0); /* not errors, just don't want to do anything */
  if (strlen(string) == 0)
    return (0);
  if (!dxf)
    return (0);

  if (dxf == 2) {
    im_iprintf(&imgStr, "TEXT\n%d\n%s\n%.0f\n%.0f\n%.0f\n",
               matchdxfcolor(label->color), string, labelPnt.x, labelPnt.y,
               -label->angle);
  } else {
    im_iprintf(&imgStr,
               "  0\nTEXT\n  1\n%s\n 10\n%f\n 20\n%f\n 30\n0.0\n 40\n%f\n "
               "50\n%f\n 62\n%6d\n  8\n%s\n 73\n   2\n 72\n   1\n",
               string, labelPnt.x, labelPnt.y, label->size * scalefactor * 100,
               -label->angle, matchdxfcolor(label->color), lname);
  }
  return (0);
}

/*
 * Save an image to a file named filename, if filename is NULL it goes to stdout
 */

int msSaveImageIM(imageObj *img, const char *filename, outputFormatObj *format)

{
  FILE *stream_to_free = NULL;
  FILE *stream;
  char workbuffer[5000];

  DEBUG_IF printf("msSaveImageIM\n<BR>");

  if (filename != NULL && strlen(filename) > 0) {
    stream_to_free = fopen(filename, "wb");
    if (!stream_to_free) {
      msSetError(MS_IOERR, "(%s)", "msSaveImage()", filename);
      return (MS_FAILURE);
    }
    stream = stream_to_free;
  } else { /* use stdout */

#ifdef _WIN32
    /*
     * Change stdout mode to binary on win32 platforms
     */
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
      msSetError(MS_IOERR, "Unable to change stdout to binary mode.",
                 "msSaveImage()");
      return (MS_FAILURE);
    }
#endif
    stream = stdout;
  }

  if (strcasecmp(format->driver, "imagemap") == 0) {
    DEBUG_IF printf("ALLOCD %d<BR>\n", img->size);
    /* DEBUG_IF printf("F %s<BR>\n", img->img.imagemap); */
    DEBUG_IF printf("FLEN %d<BR>\n", (int)strlen(img->img.imagemap));
    if (dxf == 2) {
      msIO_fprintf(stream, "%s", layerlist);
    } else if (dxf) {
      msIO_fprintf(
          stream,
          "  0\nSECTION\n  2\nHEADER\n  9\n$ACADVER\n  1\nAC1009\n0\nENDSEC\n  "
          "0\nSECTION\n  2\nTABLES\n  0\nTABLE\n%s0\nENDTAB\n0\nENDSEC\n  "
          "0\nSECTION\n  2\nBLOCKS\n0\nENDSEC\n  0\nSECTION\n  2\nENTITIES\n",
          layerlist);
    } else {
      msIO_fprintf(stream, "<map name=\"%s\" width=\"%d\" height=\"%d\">\n",
                   mapName, img->width, img->height);
    }
    const int nSize = sizeof(workbuffer);

    const int size = strlen(img->img.imagemap);
    if (size > nSize) {
      int iIndice = 0;
      while ((iIndice + nSize) <= size) {
        snprintf(workbuffer, sizeof(workbuffer), "%s",
                 img->img.imagemap + iIndice);
        workbuffer[nSize - 1] = '\0';
        msIO_fwrite(workbuffer, strlen(workbuffer), 1, stream);
        iIndice += nSize - 1;
      }
      if (iIndice < size) {
        sprintf(workbuffer, "%s", img->img.imagemap + iIndice);
        msIO_fprintf(stream, "%s", workbuffer);
      }
    } else
      msIO_fwrite(img->img.imagemap, size, 1, stream);
    if (strcasecmp("OFF",
                   msGetOutputFormatOption(format, "SKIPENDTAG", "OFF")) == 0) {
      if (dxf == 2)
        msIO_fprintf(stream, "END");
      else if (dxf)
        msIO_fprintf(stream, "0\nENDSEC\n0\nEOF\n");
      else
        msIO_fprintf(stream, "</map>");
    }
  } else {
    msSetError(MS_MISCERR, "Unknown output image type driver: %s.",
               "msSaveImage()", format->driver);
    if (stream_to_free != NULL)
      fclose(stream_to_free);
    return (MS_FAILURE);
  }

  if (stream_to_free != NULL)
    fclose(stream_to_free);
  return (MS_SUCCESS);
}

/*
 * Free gdImagePtr
 */
void msFreeImageIM(imageObj *img) { free(img->img.imagemap); }

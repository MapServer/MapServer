/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Labeling Implementation.
 * Author:   Steve Lime and the MapServer team.
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.86  2006/09/06 05:26:18  sdlime
 * Fixed a bug with curved labels where character advance array was not being gdFree'd. (bug 1899)
 *
 * Revision 1.85  2006/08/17 04:32:16  sdlime
 * Disable path following labels unless GD 2.0.29 or greater is available.
 *
 * Revision 1.84  2006/03/23 20:28:52  sdlime
 * Most recent patch for curved labels. (bug 1620)
 *
 * Revision 1.83  2006/03/22 23:31:20  sdlime
 * Applied latest patch for curved labels. (bug 1620)
 *
 * Revision 1.82  2006/02/18 20:59:13  sdlime
 * Initial code for curved labels. (bug 1620)
 *
 * Revision 1.81  2006/01/16 20:21:18  sdlime
 * Fixed error with image legends (shifted text) introduced by the 1449 bug fix. (bug 1607)
 *
 * Revision 1.80  2006/01/12 05:26:23  sdlime
 * Fixed spelling error in module name and replaced a // style comment.
 *
 * Revision 1.79  2005/11/28 04:24:59  sdlime
 * Changed msAddLabel() to use msCopyStyle() rather than doing on its own. This should fix some de-allocation errors folks have been having. (bug 1398)
 *
 * Revision 1.78  2005/08/31 18:30:03  sdlime
 * Applied patch suggested to adjust TTF baselines (bug 1449).
 *
 * Revision 1.77  2005/06/14 16:03:33  dan
 * Updated copyright date to 2005
 *
 * Revision 1.76  2005/05/19 04:09:34  sdlime
 * Removed the LINE_VERT_THRESHOLD test (bug 564) from PDF/SWF/SVG/imagemap drivers.
 *
 * Revision 1.75  2005/02/18 03:06:46  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.74  2005/01/11 00:24:28  frank
 * added labelObj arg to msAddLabel()
 *
 * Revision 1.73  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

/*
** maplabel.c: Routines to enable text drawing, BITMAP or TRUETYPE.
*/
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontt.h>
#include <gdfontmb.h>
#include <gdfontg.h>

#include "map.h"

MS_CVSID("$Id$")

int msAddLabel(mapObj *map, int layerindex, int classindex, int shapeindex, int tileindex, pointObj *point, labelPathObj *labelpath, char *string, double featuresize, labelObj *label )
{
  int i;
  char wrap[2];

  labelCacheMemberObj *cachePtr=NULL;
  layerObj *layerPtr=NULL;
  classObj *classPtr=NULL;

  if(!string) return(MS_SUCCESS); /* not an error */ 

  if(map->labelcache.numlabels == map->labelcache.cachesize) { /* just add it to the end */
    map->labelcache.labels = (labelCacheMemberObj *) realloc(map->labelcache.labels, sizeof(labelCacheMemberObj)*(map->labelcache.cachesize+MS_LABELCACHEINCREMENT));
    if(!map->labelcache.labels) {
      msSetError(MS_MEMERR, "Realloc() error.", "msAddLabel()");
      return(MS_FAILURE);
    }
    map->labelcache.cachesize += MS_LABELCACHEINCREMENT;
  }

  cachePtr = &(map->labelcache.labels[map->labelcache.numlabels]); /* set up a few pointers for clarity */
  layerPtr = &(map->layers[layerindex]);
  classPtr = &(map->layers[layerindex].class[classindex]);

  if( label == NULL )
      label = &(classPtr->label);

  cachePtr->layerindex = layerindex; /* so we can get back to this *raw* data if necessary */
  cachePtr->classindex = classindex;
  cachePtr->tileindex = tileindex;
  cachePtr->shapeindex = shapeindex;

  /* Store the label point or the label path (Bug #1620) */
  if ( point ) {

  cachePtr->point = *point; /* the actual label point */
  cachePtr->point.x = MS_NINT(cachePtr->point.x);
  cachePtr->point.y = MS_NINT(cachePtr->point.y);
    cachePtr->labelpath = NULL;
    
  } else if ( labelpath ) {
    int i;
    cachePtr->labelpath = labelpath;
    /* Use the middle point of the labelpath for mindistance calculations */
    i = labelpath->path.numpoints / 2;
    cachePtr->point.x = MS_NINT(labelpath->path.point[i].x);
    cachePtr->point.y = MS_NINT(labelpath->path.point[i].y);
  }

  cachePtr->text = strdup(string); /* the actual text */

  /* GD/Freetype recognizes \r\n as a true line wrap so we must turn the wrap character into that pattern */
  if(label->type != MS_BITMAP && label->wrap != '\0') {
    wrap[0] = label->wrap;
    wrap[1] = '\0';
    cachePtr->text = gsub(cachePtr->text, wrap, "\r\n");
  }

  /* copy the styles (only if there is an accompanying marker) */
  cachePtr->styles = NULL;
  cachePtr->numstyles = 0;
  if((layerPtr->type == MS_LAYER_ANNOTATION && classPtr->numstyles > 0) || layerPtr->type == MS_LAYER_POINT) {
    cachePtr->numstyles = classPtr->numstyles;
    cachePtr->styles = (styleObj *) malloc(sizeof(styleObj)*classPtr->numstyles);
    if (classPtr->numstyles > 0) {
      for(i=0; i<classPtr->numstyles; i++) {
	initStyle(&(cachePtr->styles[i]));
	msCopyStyle(&(cachePtr->styles[i]), &(classPtr->styles[i]));
      }
    }
  }

  /* copy the label */
  cachePtr->label = *label; /* this copies all non-pointers */
  if(label->font) cachePtr->label.font = strdup(label->font);

  cachePtr->featuresize = featuresize;

  cachePtr->poly = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(cachePtr->poly);

  cachePtr->status = MS_FALSE;

  if(layerPtr->type == MS_LAYER_POINT) { /* cache the marker placement, it's already on the map */
    rectObj rect;
    int w, h;

    if(map->labelcache.nummarkers == map->labelcache.markercachesize) { /* just add it to the end */
      map->labelcache.markers = (markerCacheMemberObj *) realloc(map->labelcache.markers, sizeof(markerCacheMemberObj)*(map->labelcache.cachesize+MS_LABELCACHEINCREMENT));
      if(!map->labelcache.markers) {
	msSetError(MS_MEMERR, "Realloc() error.", "msAddLabel()");
	return(MS_FAILURE);
      }
      map->labelcache.markercachesize+=MS_LABELCACHEINCREMENT;
    }

    i = map->labelcache.nummarkers;

    map->labelcache.markers[i].poly = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(map->labelcache.markers[i].poly);

    /* TO DO: at the moment only checks the bottom style, perhaps should check all of them */
    if(msGetMarkerSize(&map->symbolset, &(classPtr->styles[0]), &w, &h, layerPtr->scalefactor) != MS_SUCCESS)
      return(MS_FAILURE);

    rect.minx = MS_NINT(point->x - .5 * w);
    rect.miny = MS_NINT(point->y - .5 * h);
    rect.maxx = rect.minx + (w-1);
    rect.maxy = rect.miny + (h-1);
    msRectToPolygon(rect, map->labelcache.markers[i].poly);    
    map->labelcache.markers[i].id = map->labelcache.numlabels;

    map->labelcache.nummarkers++;
  }

  map->labelcache.numlabels++;

  return(MS_SUCCESS);
}

int msInitFontSet(fontSetObj *fontset)
{
    fontset->filename = NULL;
    
    /* fontset->fonts = NULL; */
    initHashTable(&(fontset->fonts));
    
    fontset->numfonts = 0;
    fontset->map = NULL;
    return( 0 );
}

int msFreeFontSet(fontSetObj *fontset)
{
    if (fontset->filename)
        free(fontset->filename);
    fontset->filename = NULL;
    if (&(fontset->fonts))
        msFreeHashItems(&(fontset->fonts));
    /* fontset->fonts = NULL; */
    fontset->numfonts = 0;    

    return( 0 );
}

int msLoadFontSet(fontSetObj *fontset, mapObj *map)
{
#ifdef USE_GD_FT
  FILE *stream;
  char buffer[MS_BUFFER_LENGTH];
  char alias[64], file1[MS_PATH_LENGTH], file2[MS_PATH_LENGTH];
  char *path;
  char szPath[MS_MAXPATHLEN];
  int i;
  int bFullPath = 0;

  if(fontset->numfonts != 0) /* already initialized */
    return(0);

  if(!fontset->filename)
    return(0);

  fontset->map = (mapObj *)map;

  path = getPath(fontset->filename);

  /* fontset->fonts = msCreateHashTable(); // create font hash */
  /* if(!fontset->fonts) { */
  /* msSetError(MS_HASHERR, "Error initializing font hash.", "msLoadFontSet()"); */
  /* return(-1); */
  /* } */

  stream = fopen( msBuildPath(szPath, fontset->map->mappath, fontset->filename), "r");
  if(!stream) {
    msSetError(MS_IOERR, "Error opening fontset %s.", "msLoadFontset()",
               fontset->filename);
    return(-1);
  }

  i = 0;
  while(fgets(buffer, MS_BUFFER_LENGTH, stream)) { /* while there's something to load */

    if(buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == ' ')
      continue; /* skip comments and blank lines */

    sscanf(buffer,"%s %s", alias,  file1);

    if (!file1 || !alias || (strlen(file1) <= 0))
      continue;
    
    bFullPath = 0;
#if defined(_WIN32) && !defined(__CYGWIN__)
    if (file1[0] == '\\' || (strlen(file1) > 1 && (file1[1] == ':')))
      bFullPath = 1;
#else
    if(file1[0] == '/')
      bFullPath = 1;
#endif

    if(bFullPath) { /* already full path */
      msInsertHashTable(&(fontset->fonts), alias, file1);
    } else {
      sprintf(file2, "%s%s", path, file1);
      /* msInsertHashTable(fontset->fonts, alias, file2); */

      /*
      ** msBuildPath is use here, but if we have to save the fontset file
      ** the msBuildPath must be done everywhere the fonts are used and 
      ** removed here.
      */
      msInsertHashTable(&(fontset->fonts), alias, 
                        msBuildPath(szPath, fontset->map->mappath, file2));
      
    }

    i++;
  }

  fontset->numfonts = i;
  fclose(stream); /* close the file */
  free(path);

  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msLoadFontSet()");
  return(-1);
#endif
}

/*
** Note: All these routines assume a reference point at the LL corner of the text. GD's
** bitmapped fonts use UL and this is compensated for. Note the rect is relative to the
** LL corner of the text to be rendered, this is first line for TrueType fonts.
*/

/* assumes an angle of 0 regardless of what's in the label object */
int msGetLabelSize(char *string, labelObj *label, rectObj *rect, fontSetObj *fontset, double scalefactor, int adjustBaseline)
{
  int size;

  if(label->type == MS_TRUETYPE) {
#ifdef USE_GD_FT
    int bbox[8];
    char *error=NULL, *font=NULL;

    size = MS_NINT(label->size*scalefactor);
    size = MS_MAX(size, label->minsize);
    size = MS_MIN(size, label->maxsize);

    font = msLookupHashTable(&(fontset->fonts), label->font);
    if(!font) {
      if(label->font) 
        msSetError(MS_TTFERR, "Requested font (%s) not found.", "msGetLabelSize()", label->font);
      else
        msSetError(MS_TTFERR, "Requested font (NULL) not found.", "msGetLabelSize()");
      return(-1);
    }


    error = gdImageStringFT(NULL, bbox, 0, font, size, 0, 0, 0, string);
    if(error) {
      msSetError(MS_TTFERR, error, "msGetLabelSize()");
      return(-1);
    }

    rect->minx = bbox[0];
    rect->miny = bbox[5];
    rect->maxx = bbox[2];
    rect->maxy = bbox[1];

    /* bug 1449 fix (adjust baseline) */
    if(adjustBaseline) {
      label->offsety += MS_NINT(((bbox[5] + bbox[1]) + size) / 2);
      label->offsetx += MS_NINT(bbox[0] / 2);
    }
#else
    msSetError(MS_TTFERR, "TrueType font support is not available.", "msGetLabelSize()");
    return(-1);
#endif
  } else { /* MS_BITMAP font */
    gdFontPtr fontPtr;
    char **token=NULL;
    int t, num_tokens, max_token_length=0;

    if((fontPtr = msGetBitmapFont(label->size)) == NULL)
      return(-1);

    if(label->wrap != '\0') {
      if((token = split(string, label->wrap, &(num_tokens))) == NULL)
	return(0);

      for(t=0; t<num_tokens; t++) /* what's the longest token */
	max_token_length = MS_MAX(max_token_length, (int) strlen(token[t]));

      rect->minx = 0;
      rect->miny = -(fontPtr->h * num_tokens);
      rect->maxx = fontPtr->w * max_token_length;
      rect->maxy = 0;

      msFreeCharArray(token, num_tokens);
    } else {
      rect->minx = 0;
      rect->miny = -fontPtr->h;
      rect->maxx = fontPtr->w * strlen(string);
      rect->maxy = 0;
    }
  }
  return(0);
}

/*
 * Return the size of the label, plus an array of individual character
 * offsets, as calculated by gdImageStringFTEx().  The callee is
 * responsible for freeing *offsets.
 */
int msGetLabelSizeEx(char *string, labelObj *label, rectObj *rect, fontSetObj *fontset, double scalefactor, int adjustBaseline, double **offsets) 
{
#if defined (USE_GD_FT) && defined (GD_HAS_FTEX_XSHOW)
    int size;
    char *s;
    int k;
    int bbox[8];
    char *error=NULL, *font=NULL;
    gdFTStringExtra strex;
    
    size = MS_NINT(label->size*scalefactor);
    size = MS_MAX(size, label->minsize);
    size = MS_MIN(size, label->maxsize);

    font = msLookupHashTable(&(fontset->fonts), label->font);
    if(!font) {
      if(label->font) 
        msSetError(MS_TTFERR, "Requested font (%s) not found.", "msGetLabelSizeEx()", label->font);
      else
        msSetError(MS_TTFERR, "Requested font (NULL) not found.", "msGetLabelSizeEx()");
      return(-1);
    }

    strex.flags = gdFTEX_XSHOW;
    error = gdImageStringFTEx(NULL, bbox, 0, font, size, 0, 0, 0, string, &strex);
    if(error) {
      msSetError(MS_TTFERR, error, "msGetLabelSizeEx()");
      return(-1);
    }

    *offsets = (double*)malloc( strlen(string) * sizeof(double) );
    
    s = strex.xshow;
    k = 0;
    while ( *s && k < strlen(string) ) {
      (*offsets)[k++] = atof(s);      
      while ( *s && *s != ' ')
        s++;
      if ( *s == ' ' )
        s++;
    }

    gdFree(strex.xshow); /* done with character advances */

    rect->minx = bbox[0];
    rect->miny = bbox[5];
    rect->maxx = bbox[2];
    rect->maxy = bbox[1];

    /* bug 1449 fix (adjust baseline) */
    if(adjustBaseline) {
      label->offsety += MS_NINT(((bbox[5] + bbox[1]) + size) / 2);
      label->offsetx += MS_NINT(bbox[0] / 2);
    }

    return MS_SUCCESS;
#else
    msSetError(MS_TTFERR, "TrueType font support is not available or is not current enough (requires 2.0.29 or higher).", "msGetLabelSizeEx()");
    return(-1);
#endif
}

gdFontPtr msGetBitmapFont(int size)
{
  switch(size) { /* set the font to use */
  case MS_TINY:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetTiny();
#else
    return(gdFontTiny);
#endif
    break;
  case MS_SMALL:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetSmall();
#else
    return(gdFontSmall);
#endif
    break;
  case MS_MEDIUM:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetMediumBold();
#else
    return(gdFontMediumBold);
#endif
    break;
  case MS_LARGE:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetLarge();
#else
    return(gdFontLarge);
#endif
    break;
  case MS_GIANT:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetGiant();
#else
    return(gdFontGiant);
#endif
    break;
  default:
    msSetError(MS_GDERR,"Invalid bitmap font. Must be one of tiny, small, medium, large or giant." , "msGetBitmapFont()");
    return(NULL);
  }
}

#define MARKER_SLOP 2

/* static pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly) */
pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly)
{
  pointObj q;
  double x1=0, y1=0, x2=0, y2=0;
  lineObj line={0,NULL};
  double sin_a,cos_a;
  double w, h, x, y;

  w = rect.maxx - rect.minx;
  h = rect.maxy - rect.miny;

  switch(position) {
  case MS_UL:
    x1 = -w - ox;
    y1 = -oy;
    break;
  case MS_UC:
    x1 = -(w/2.0);
    y1 = -oy - MARKER_SLOP;
    break;
  case MS_UR:
    x1 = ox;
    y1 = -oy;
    break;
  case MS_CL:
    x1 = -w - ox - MARKER_SLOP;
    y1 = (h/2.0);
    break;
  case MS_CC:
    x1 = -(w/2.0) + ox;
    y1 = (h/2.0) + oy;
    break;
  case MS_CR:
    x1 = ox + MARKER_SLOP;
    y1 = (h/2.0);
    break;
  case MS_LL:
    x1 = -w - ox;
    y1 = h + oy;
    break;
  case MS_LC:
    x1 = -(w/2.0);
    y1 = h + oy + MARKER_SLOP;
    break;
  case MS_LR:
    x1 = ox;
    y1 = h + oy;
    break;
  }

  sin_a = sin(MS_DEG_TO_RAD*angle);
  cos_a = cos(MS_DEG_TO_RAD*angle);

  x = x1 - rect.minx;
  y = rect.maxy - y1;
  q.x = p->x + MS_NINT(x * cos_a - (y) * sin_a);
  q.y = p->y - MS_NINT(x * sin_a + (y) * cos_a);

  if(poly) {
    line.point = (pointObj *)malloc(sizeof(pointObj)*5);
    line.numpoints = 5;

    x2 = x1 - buffer; /* ll */
    y2 = y1 + buffer;
    line.point[0].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    line.point[0].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);

    x2 = x1 - buffer; /* ul */
    y2 = y1 - h - buffer;
    line.point[1].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    line.point[1].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);

    x2 = x1 + w + buffer; /* ur */
    y2 = y1 - h - buffer;
    line.point[2].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    line.point[2].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);

    x2 = x1 + w + buffer; /* lr */
    y2 = y1 + buffer;
    line.point[3].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    line.point[3].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);

    line.point[4].x = line.point[0].x;
    line.point[4].y = line.point[0].y;

    msAddLine(poly, &line);
    msComputeBounds(poly); /* make sure the polygon has a bounding box for fast collision testing */
    free(line.point);
  }

  return(q);
}

/*
** is a label completely in the image (excluding label buffer)
*/
/* static int labelInImage(int width, int height, shapeObj *lpoly, int buffer) */
int labelInImage(int width, int height, shapeObj *lpoly, int buffer)
{
  int i,j;

  for(i=0; i<lpoly->numlines; i++) {
    for(j=1; j<lpoly->line[i].numpoints; j++) {
      if(lpoly->line[i].point[j].x < -buffer) return(MS_FALSE);
      if(lpoly->line[i].point[j].x >= width+buffer) return(MS_FALSE);
      if(lpoly->line[i].point[j].y < -buffer) return(MS_FALSE);
      if(lpoly->line[i].point[j].y >= height+buffer) return(MS_FALSE);
    }
  }

  return(MS_TRUE);
}

/*
** Variation on msIntersectPolygons. Label polygons aren't like shapefile polygons. They
** have no holes, and often do have overlapping parts (i.e. road symbols).
*/

/* static int intersectLabelPolygons(shapeObj *p1, shapeObj *p2) { */
int intersectLabelPolygons(shapeObj *p1, shapeObj *p2) {
  int c1,v1,c2,v2;
  pointObj *point;

  /* STEP 0: check bounding boxes */
  if(!msRectOverlap(&p1->bounds, &p2->bounds)) /* from alans@wunderground.com */
    return(MS_FALSE);

  /* STEP 1: look for intersecting line segments */
  for(c1=0; c1<p1->numlines; c1++)
    for(v1=1; v1<p1->line[c1].numpoints; v1++)
      for(c2=0; c2<p2->numlines; c2++)
	for(v2=1; v2<p2->line[c2].numpoints; v2++)
	  if(msIntersectSegments(&(p1->line[c1].point[v1-1]), &(p1->line[c1].point[v1]), &(p2->line[c2].point[v2-1]), &(p2->line[c2].point[v2])) ==  MS_TRUE)
	    return(MS_TRUE);

  /* STEP 2: polygon one completely contains two (only need to check one point from each part) */
  for(c2=0; c2<p2->numlines; c2++) {
    point = &(p2->line[c2].point[0]);
    for(c1=0; c1<p1->numlines; c1++) {
      if(msPointInPolygon(point, &p1->line[c1]) == MS_TRUE) /* ok, the point is in a polygon */
	return(MS_TRUE);
    }
  }

  /* STEP 3: polygon two completely contains one (only need to check one point from each part) */
  for(c1=0; c1<p1->numlines; c1++) {
    point = &(p1->line[c1].point[0]);
    for(c2=0; c2<p2->numlines; c2++) {
      if(msPointInPolygon(point, &p2->line[c2]) == MS_TRUE) /* ok, the point is in a polygon */
	return(MS_TRUE);
    }
  }

  return(MS_FALSE);
}

int msImageTruetypeArrow(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor)
{
#ifdef USE_GD_FT
  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msImageTruetypeArrow()");
  return(-1);
#endif
}

int msImageTruetypePolyline(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor)
{
#ifdef USE_GD_FT
  int i,j;
  double theta, length, current_length;
  labelObj label;
  pointObj point, label_point;
  shapeObj label_poly;
  rectObj label_rect;
  int label_width;
  int position, rot, gap, in;
  double rx, ry, size;

  symbolObj *symbol;

  msInitShape(&label_poly);

  symbol = &(symbolset->symbol[style->symbol]);

  initLabel(&label);
  rot = (symbol->gap < 0);
  label.type = MS_TRUETYPE;
  label.font = symbol->font;
  /* -- rescaling symbol and gap */
  if(style->size == -1) {
      size = msSymbolGetDefaultSize( symbol );
  }
  else
      size = style->size;
  if(size*scalefactor > style->maxsize) scalefactor = (float)style->maxsize/(float)size;
  if(size*scalefactor < style->minsize) scalefactor = (float)style->minsize/(float)size;
  gap = MS_ABS(symbol->gap)* (int) scalefactor;
  label.size = (int) (size * scalefactor);
  /* label.minsize = style->minsize; */
  /* label.maxsize = style->maxsize; */

  label.color = style->color; /* TODO: now assuming these colors should have previously allocated pen values */
  label.outlinecolor = style->outlinecolor;
  label.antialias = symbol->antialias;
  
  if(msGetLabelSize(symbol->character, &label, &label_rect, symbolset->fontset, scalefactor, MS_FALSE) == -1)
    return(-1);

  label_width = (int) label_rect.maxx - (int) label_rect.minx;

  for(i=0; i<p->numlines; i++) {
    current_length = gap+label_width/2.0; /* initial padding for each line */
    
    for(j=1;j<p->line[i].numpoints;j++) {
      length = sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2)));
      if(length==0)continue;
      rx = (p->line[i].point[j].x - p->line[i].point[j-1].x)/length;
      ry = (p->line[i].point[j].y - p->line[i].point[j-1].y)/length;  
      position = symbol->position;
      theta = asin(ry);
      if(rx < 0) {
        if(rot){
          theta += MS_PI;
                if((position == MS_UR)||(position == MS_UL)) position = MS_LC;
          if((position == MS_LR)||(position == MS_LL)) position = MS_UC;
        }else{
          if(position == MS_UC) position = MS_LC;
          else if(position == MS_LC) position = MS_UC;
        }
      }
      else theta = -theta;        
      if((position == MS_UR)||(position == MS_UL)) position = MS_UC;
      if((position == MS_LR)||(position == MS_LL)) position = MS_LC;
      label.angle = MS_RAD_TO_DEG * theta;

      in = 0;
      while(current_length <= length) {
        point.x = MS_NINT(p->line[i].point[j-1].x + current_length*rx);
        point.y = MS_NINT(p->line[i].point[j-1].y + current_length*ry);

        label_point = get_metrics(&point, position, label_rect, 0, 0, label.angle, 0, &label_poly);
        msDrawTextGD(img, label_point, symbol->character, &label, symbolset->fontset, scalefactor);

        current_length += label_width + gap;
        in = 1;
      }

      if(in){
        current_length -= length + label_width/2.0;
      } else current_length -= length;
    }
  }

  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msImageTruetypePolyline()");
  return(-1);
#endif
}

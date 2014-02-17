/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  GD rendering functions (using renderer plugin API)
 * Author:   Stephen Lime, Thomas Bonfort
 *
 ******************************************************************************
 * Copyright (c) 1996-2009 Regents of the University of Minnesota.
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
#include "mapserver-config.h"
#ifdef USE_GD

#include "mapserver.h"
#include "mapthread.h"
#include <time.h>
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontt.h>
#include <gdfontmb.h>
#include <gdfontg.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int msGDSetup()
{
#ifdef USE_GD_FREETYPE
  if (gdFontCacheSetup() != 0) {
    return MS_FAILURE;
  }
#endif
  return MS_SUCCESS;
}

void msGDCleanup(int signal)
{
#ifdef USE_GD_FREETYPE
  if(!signal) {
    /* there's a potential deadlock if we're killed by a signal and the font
     cache is already locked. We don't tear down the fontcache in this case
     to avoid it (issue 4093)*/
    gdFontCacheShutdown();
  }
#endif
}

#define MS_IMAGE_GET_GDIMAGEPTR(image) ((gdImagePtr) image->img.plugin)

fontMetrics bitmapFontMetricsGD[5];

gdFontPtr msGetBitmapFont(int size)
{
  switch(size) { /* set the font to use */
    case MS_TINY:
      return gdFontGetTiny();
      break;
    case MS_SMALL:
      return gdFontGetSmall();
      break;
    case MS_MEDIUM:
      return gdFontGetMediumBold();
      break;
    case MS_LARGE:
      return gdFontGetLarge();
      break;
    case MS_GIANT:
      return gdFontGetGiant();
      break;
    default:
      msSetError(MS_GDERR,"Invalid bitmap font. Must be one of tiny, small, medium, large or giant." , "msGetBitmapFont()");
      return(NULL);
  }
}

int msImageSetPenGD(gdImagePtr img, colorObj *color)
{
  if(MS_VALID_COLOR(*color))
    color->pen = gdImageColorResolve(img, color->red, color->green, color->blue);
  else
    color->pen = -1;

  return(MS_SUCCESS);
}

int startNewLayerGD(imageObj *img, mapObj *map, layerObj *layer)
{
  return MS_SUCCESS;
}

int closeNewLayerGD(imageObj *img, mapObj *map, layerObj *layer)
{
  return MS_SUCCESS;
}

/*
** GD driver-specific image handling functions.
*/
imageObj *createImageGD(int width, int height, outputFormatObj *format, colorObj* bg)
{
  imageObj *img = NULL;
  gdImagePtr ip;

  img = (imageObj *) calloc(1, sizeof (imageObj));
  MS_CHECK_ALLOC(img, sizeof (imageObj), NULL);

  /* we're only doing PC256 for the moment */
  ip = gdImageCreate(width, height);
  if(bg && MS_VALID_COLOR(*bg)) {
    gdImageColorAllocate(ip, bg->red, bg->green, bg->blue); /* set the background color */
  } else {
    gdImageColorAllocate(ip,117,17,91); /*random bg color (same one as picked in msResampleGDALToMap) */
  }
  if(format->transparent || !bg || !MS_VALID_COLOR(*bg)) {
    gdImageColorTransparent(ip, 0);
  }

  img->img.plugin = (void *) ip;
  return img;
}



int saveImageGD(imageObj *img, mapObj *map, FILE *fp, outputFormatObj *format)
{
  gdImagePtr ip;

  if(!img || !fp) return MS_FAILURE;
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;

  return saveGdImage(ip,fp,format);

}

int freeImageGD(imageObj *img)
{
  gdImagePtr ip;

  if(img) {
    ip = MS_IMAGE_GET_GDIMAGEPTR(img);
    if(ip) gdImageDestroy(ip);
  }

  return MS_SUCCESS;
}

/*
** GD driver-specific rendering functions.
*/
#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

#define SETPEN(ip, c) if(c && c->pen == MS_PEN_UNSET) c->pen = gdImageColorResolve(ip, c->red, c->green, c->blue)


static void imageScanline(gdImagePtr im, int x1, int x2, int y, int c)
{
  int x;

  /* TO DO: This fix (adding if/then) was to address circumstances in the polygon fill code */
  /* where x2 < x1. There may be something wrong in that code, but at any rate this is safer. */

  if(x1 < x2)
    for(x=x1; x<=x2; x++) gdImageSetPixel(im, x, y, c);
  else
    for(x=x2; x<=x1; x++) gdImageSetPixel(im, x, y, c);
}

/*
** Polygon fill. Based on "Concave Polygon Scan Conversion" by Paul
** Heckbert from "Graphics Gems", Academic Press, 1990.
**
*/
static void imageFilledPolygon(gdImagePtr im, shapeObj *p, int c)
{
  typedef struct {     /* a polygon edge */
    double x;          /* x coordinate of edge's intersection with current scanline */
    double dx;         /* change in x with respect to y */
    int i;             /* point index  */
    int l;             /* line number */
    int s;             /* scanline */
  } pEdge;

  pointObj *point1, *point2;

  int k, l, i, j, xl, xr, ymin, ymax, y, n,nvert, nact, m;
  int wrong_order;

  pEdge *edge, *temp;
  pEdge  **active;
  int *yhist, *edgeindex;

  if(p->numlines == 0) return;
  n=0;

  for(i=0; i<p->numlines; i++) {
    n += p->line[i].numpoints;
  }

  if(n == 0)   return;

  edge = (pEdge *) msSmallCalloc(n,sizeof(pEdge));           /* All edges in the polygon */
  edgeindex =  (int *) msSmallCalloc(n,sizeof(int));         /* Index to edges sorted by scanline */
  active = (pEdge **) msSmallCalloc(n,sizeof(pEdge*));       /* Pointers to active edges for current scanline */

  nvert=0;

  ymin= (int) ceil(p->line[0].point[0].y-0.5);
  ymax= (int) floor(p->line[0].point[0].y-0.5);

  /* populate the edge table */
  for(l=0; l<p->numlines; l++) {
    for(i=0; i < p->line[l].numpoints; i++) {
      j = i < p->line[l].numpoints -1 ? i+1 : 0;
      if (p->line[l].point[i].y  < p->line[l].point[j].y ) {
        point1 = &(p->line[l].point[i]);
        point2 = &(p->line[l].point[j]);
      } else {
        point2 = &(p->line[l].point[i]);
        point1 = &(p->line[l].point[j]);
      }

      edge[nvert].dx  = point2->y == point1->y ? 0 :  (point2->x - point1->x) / (point2->y - point1->y);
      edge[nvert].s = MS_NINT( p->line[l].point[i].y );  /* ceil( p->line[l].point[i].y  - 0.5 ); */
      edge[nvert].x = point1->x;
      edge[nvert].i = nvert;
      edge[nvert].l = l;

      ymin = MS_MIN(ymin,edge[nvert].s);
      ymax = MS_MAX(ymax,edge[nvert].s);

      nvert++;
    }
  }

  /* Use histogram sort to create a bucket-sorted edgeindex by scanline */
  yhist = (int*) msSmallCalloc(ymax - ymin + 2, sizeof(int));

  for(i=0; i<nvert; i++) {
    yhist[ edge[i].s - ymin + 1 ]++;
  }
  for(i=0; i<=(ymax - ymin); i++)  {/* Calculate starting point in edgeindex for each scanline */
    yhist[i+1] += yhist[i];
  }
  for(i=0; i<nvert; i++) { /* Bucket sort edges into edgeindex */
    y = edge[i].s;
    edgeindex[yhist[y-ymin]] = i;
    yhist[y-ymin]++;
  }
  free(yhist);

  k=0;
  nact=0;

  for (y=ymin; y<=ymax; y++) { /* step through scanlines */
    /* scanline y is at y+.5 in continuous coordinates */

    /* check vertices between previous scanline and current one, if any */
    for (; k<nvert && edge[edgeindex[k]].s <= y; k++) {
      i = edge[edgeindex[k]].i;

      /* vertex previous to i */
      if(i==0 || edge[i].l != edge[i-1].l)
        j = i +  p->line[edge[i].l].numpoints - 1;
      else
        j = i - 1;

      if (edge[j].s  <=  y  ) { /* old edge, remove from active list */
        for (m=0; m<nact && active[m]->i!=j; m++);
        if (m<nact) {
          nact--;
          active[m]=active[nact];
        }
      } else if (edge[j].s > y) { /* new edge,  insert into active list */
        active[nact]= & edge[j];
        nact++;
      }

      /* vertex next  after i */
      if(i==nvert-1 || edge[i].l != edge[i+1].l)
        j = i - p->line[edge[i].l].numpoints  + 1;
      else
        j = i + 1;

      if (edge[j].s  <=  y - 1 ) {     /* old edge, remove from active list */
        for (m=0; m<nact && active[m]->i!=i; m++);
        if (m<nact) {
          nact--;
          active[m]=active[nact];
        }
      } else if (edge[j].s > y ) { /* new edge, insert into active list */
        active[nact]= & edge[i];
        nact++;
      }
    }

    /* Sort active edges by x */
    do {
      wrong_order = 0;
      for(i=0; i < nact-1; i++) {
        if(active[i]->x > active[i+1]->x) {
          wrong_order = 1;
          SWAP(active[i], active[i+1], temp);
        }
      }
    } while(wrong_order);

    /* draw horizontal spans for scanline y */
    for (j=0; j<nact; j+=2) {
      /* j -> j+1 is inside,  j+1 -> j+2 is outside */
      xl = (int) MS_NINT(active[j]->x );
      xr = (int) (active[j+1]->x - 0.5) ;

      if(active[j]->x != active[j+1]->x)
        imageScanline(im, xl, xr, y, c);

      active[j]->x += active[j]->dx;  /* increment edge coords */
      active[j+1]->x += active[j+1]->dx;
    }
  }

  free(active);
  free(edgeindex);
  free(edge);
}

static void imagePolyline(gdImagePtr im, shapeObj *p, int c)
{
  int i, j;

  for (i=0; i < p->numlines; i++) {
    lineObj *line = &(p->line[i]);
    for(j=1; j<line->numpoints; j++) {
      pointObj *point1 = &line->point[j-1];
      pointObj *point2 = &line->point[j];

      gdImageLine(im, (int) point1->x, (int) point1->y, (int) point2->x, (int) point2->y, c);
    }
  }
}

int renderLineGD(imageObj *img, shapeObj *p, strokeStyleObj *stroke)
{
  gdImagePtr ip;
  int c;
  gdImagePtr brush=NULL;

  if(!img || !p || !stroke) return MS_FAILURE;
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;

  SETPEN(ip, stroke->color);
  c = stroke->color->pen;

  if(stroke->patternlength > 0) {
    int *style;
    int i, j, k=0;
    int sc;

    for(i=0; i<stroke->patternlength; i++)
      k += MS_NINT(stroke->pattern[i]);
    style = (int *) malloc (k * sizeof(int));
    MS_CHECK_ALLOC(style, k * sizeof(int), MS_FAILURE);

    sc = c; /* start with the color */

    k=0;
    for(i=0; i<stroke->patternlength; i++) {
      for(j=0; j<MS_NINT(stroke->pattern[i]); j++, k++) {
        style[k] = sc;
      }
      sc = ((sc==c)?gdTransparent:c);
    }

    gdImageSetStyle(ip, style, k);
    free(style);

    c = gdStyled;
  }

  if(stroke->width > 1) {
    int brush_fc;
    brush = gdImageCreate(ceil(stroke->width), ceil(stroke->width));
    gdImageColorAllocate(brush, gdImageRed(ip,0), gdImageGreen(ip, 0), gdImageBlue(ip, 0));
    gdImageColorTransparent(brush,0);
    brush_fc = gdImageColorAllocate(brush, gdImageRed(ip,stroke->color->pen),
                                    gdImageGreen(ip,stroke->color->pen), gdImageBlue(ip,stroke->color->pen));
    gdImageFilledEllipse(brush,MS_NINT(brush->sx/2),MS_NINT(brush->sy/2),
                         MS_NINT(stroke->width),MS_NINT(stroke->width), brush_fc);
    gdImageSetBrush(ip, brush);
    if(stroke->patternlength > 0) {
      c = gdStyledBrushed;
    } else {
      c = gdBrushed;
    }
  }

  /* finally draw something */
  imagePolyline(ip, p, c);

  /* clean up */
  if(stroke->width>1) {
    gdImageDestroy(brush);
  }
  return MS_SUCCESS;
}

int renderPolygonGD(imageObj *img, shapeObj *p, colorObj *color)
{
  gdImagePtr ip;

  if(!img || !p || !color) return MS_FAILURE;
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;
  SETPEN(ip, color);
  imageFilledPolygon(ip, p, color->pen);
  return MS_SUCCESS;
}

int renderGlyphsLineGD(imageObj *img, labelPathObj *labelpath, labelStyleObj *style, char *text)
{
  return MS_SUCCESS;
}

int renderGlyphsGD(imageObj *img, double x, double y, labelStyleObj *style, char *text)
{
#ifdef USE_GD_FREETYPE
  gdImagePtr ip;
  char *error=NULL;
  int bbox[8];
  int c = 0,oc = 0;
  x = MS_NINT(x);
  y = MS_NINT(y);
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;
  if(!text || !strlen(text)) return(MS_SUCCESS); /* not errors, just don't want to do anything */

  SETPEN(ip, style->color);
  SETPEN(ip, style->outlinecolor);

  if(style->antialias) {
    if(style->color)
      c = style->color->pen;
    if(style->outlinewidth > 0)
      oc = style->outlinecolor->pen;
  } else {
    if(style->color)
      c = -style->color->pen;
    if(style->outlinewidth > 0)
      oc = -style->outlinecolor->pen;
  }

  if(style->outlinewidth > 0) { /* handle the outline color */
    error = gdImageStringFT(ip, bbox, oc, style->fonts[0], style->size, style->rotation, x, y-1, text);
    if(error) {
      msSetError(MS_TTFERR, "%s", "msDrawTextGD()", error);
      return(MS_FAILURE);
    }

    gdImageStringFT(ip, bbox, oc, style->fonts[0], style->size, style->rotation, x, y+1, text);
    gdImageStringFT(ip, bbox, oc, style->fonts[0], style->size, style->rotation, x+1, y, text);
    gdImageStringFT(ip, bbox, oc, style->fonts[0], style->size, style->rotation, x-1, y, text);
    gdImageStringFT(ip, bbox, oc, style->fonts[0], style->size, style->rotation, x-1, y-1, text);
    gdImageStringFT(ip, bbox, oc, style->fonts[0], style->size, style->rotation, x-1, y+1, text);
    gdImageStringFT(ip, bbox, oc, style->fonts[0], style->size, style->rotation, x+1, y-1, text);
    gdImageStringFT(ip, bbox, oc, style->fonts[0], style->size, style->rotation, x+1, y+1, text);
  }

  if(style->color)
    gdImageStringFT(ip, bbox, c, style->fonts[0], style->size, style->rotation, x, y, text);
  return MS_SUCCESS;
#else
  msSetError(MS_TTFERR, "Freetype support not enabled in this GD build", "renderGlyphsGD()");
  return MS_FAILURE;
#endif
}

int renderEllipseSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style)
{
  /* check for trivial cases - 1x1 and 2x2, GD does not do these well */
  gdImagePtr ip;
  int w,h,fc,oc;
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;
  SETPEN(ip, style->color);
  SETPEN(ip, style->outlinecolor);
  fc = style->color ? style->color->pen : -1;
  oc = style->outlinecolor ? style->outlinecolor->pen : -1;

  if(oc==-1 && fc ==-1) {
    return MS_SUCCESS;
  }

  w = symbol->sizex * style->scale;
  h = symbol->sizey * style->scale;

  if(w==1 && h==1) {
    if(fc >= 0)
      gdImageSetPixel(ip, x, y, fc);
    else
      gdImageSetPixel(ip, x, y, oc);
    return MS_SUCCESS;
  }

  if(w==2 && h==2) {
    if(oc >= 0) {
      gdImageSetPixel(ip, x, y, oc);
      gdImageSetPixel(ip, x, y+1, oc);
      gdImageSetPixel(ip, x+1, y, oc);
      gdImageSetPixel(ip, x+1, y+1, oc);
    } else {
      gdImageSetPixel(ip, x, y, fc);
      gdImageSetPixel(ip, x, y+1, fc);
      gdImageSetPixel(ip, x+1, y, fc);
      gdImageSetPixel(ip, x+1, y+1, fc);
    }
    return MS_SUCCESS;
  }

  if(symbol->filled) {
    if(fc >= 0) gdImageFilledEllipse(ip, x, y, w, h, fc);
    if(oc >= 0) gdImageArc(ip, x, y, w, h, 0, 360, oc);
  } else {
    if(fc < 0) fc = oc; /* try the outline color */
    gdImageArc(ip, x, y, w, h, 0, 360, fc);
  }
  return MS_SUCCESS;
}

static void get_bbox(pointObj *poiList, int numpoints, double *minx, double *miny, double *maxx, double *maxy)
{
  int j;

  *minx = *maxx = poiList[0].x;
  *miny = *maxy = poiList[0].y;
  for(j=1; j<numpoints; j++) {
    if ((poiList[j].x==-99.0) || (poiList[j].y==-99.0)) continue;
    *minx = MS_MIN(*minx, poiList[j].x);
    *maxx = MS_MAX(*maxx, poiList[j].x);
    *miny = MS_MIN(*miny, poiList[j].y);
    *maxy = MS_MAX(*maxy, poiList[j].y);
  }

  return;
}

symbolObj* rotateVectorSymbolPoints(symbolObj *symbol, double angle_rad)
{
  double dp_x, dp_y, xcor, ycor;
  double sin_a, cos_a;
  double minx,miny,maxx,maxy;
  symbolObj *newSymbol;
  double TOL=0.00000000001;
  int i;

  angle_rad = -angle_rad;

  newSymbol = (symbolObj *) msSmallMalloc(sizeof(symbolObj));
  msCopySymbol(newSymbol, symbol, NULL);

  sin_a = sin(angle_rad);
  cos_a = cos(angle_rad);

  dp_x = symbol->sizex * .5; /* get the shift vector at 0,0 */
  dp_y = symbol->sizey * .5;

  /* center at 0,0 and rotate; then move back */
  for( i=0; i < symbol->numpoints; i++) {
    /* don't rotate PENUP commands (TODO: should use a constant here) */
    if ((symbol->points[i].x == -99.0) && (symbol->points[i].y == -99.0) ) {
      newSymbol->points[i].x = -99.0;
      newSymbol->points[i].y = -99.0;
      continue;
    }

    newSymbol->points[i].x = dp_x + ((symbol->points[i].x-dp_x)*cos_a - (symbol->points[i].y-dp_y)*sin_a);
    newSymbol->points[i].y = dp_y + ((symbol->points[i].x-dp_x)*sin_a + (symbol->points[i].y-dp_y)*cos_a);
  }

  /* get the new bbox of the symbol, because we need it to get the new dimensions of the new symbol */
  get_bbox(newSymbol->points, newSymbol->numpoints, &minx, &miny, &maxx, &maxy);
  if ( (fabs(minx)>TOL) || (fabs(miny)>TOL) ) {
    xcor = minx*-1.0; /* symbols always start at 0,0 so get the shift vector */
    ycor = miny*-1.0;
    for( i=0; i < newSymbol->numpoints; i++) {
      if ((newSymbol->points[i].x == -99.0) && (newSymbol->points[i].y == -99.0))
        continue;
      newSymbol->points[i].x = newSymbol->points[i].x + xcor;
      newSymbol->points[i].y = newSymbol->points[i].y + ycor;
    }

    /* update the bbox to get the final dimension values for the symbol */
    get_bbox(newSymbol->points, newSymbol->numpoints, &minx, &miny, &maxx, &maxy);
  }
  newSymbol->sizex = maxx;
  newSymbol->sizey = maxy;
  return newSymbol;
}

int renderVectorSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style)
{
  int bRotated = MS_FALSE;
  int j,k;
  gdImagePtr ip;
  gdPoint mPoints[MS_MAXVECTORPOINTS];
  gdPoint oldpnt,newpnt;
  int fc,oc;
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;


  SETPEN(ip, style->color);
  SETPEN(ip, style->outlinecolor);
  fc = style->color ? style->color->pen : -1;
  oc = style->outlinecolor ? style->outlinecolor->pen : -1;

  if(oc==-1 && fc ==-1) {
    return MS_SUCCESS;
  }

  if (style->rotation != 0.0) {
    bRotated = MS_TRUE;
    symbol = rotateVectorSymbolPoints(symbol, style->rotation);
    if(!symbol) {
      return MS_FAILURE;
    }
  }

  /* We avoid MS_NINT in this context because the potentially variable
         handling of 0.5 rounding is often a problem for symbols which are
         often an odd size (ie. 7pixels) and so if "p" is integral the
         value is always on a 0.5 boundary - bug 1716 */
  x -= style->scale*.5*symbol->sizex;
  y -= style->scale*.5*symbol->sizey;

  if(symbol->filled) { /* if filled */

    k = 0; /* point counter */
    for(j=0; j < symbol->numpoints; j++) {
      if((symbol->points[j].x == -99) && (symbol->points[j].y == -99)) { /* new polygon (PENUP) */
        if(k>2) {
          if(fc >= 0)
            gdImageFilledPolygon(ip, mPoints, k, fc);
          if(oc >= 0)
            gdImagePolygon(ip, mPoints, k, oc);
        }
        k = 0; /* reset point counter */
      } else {
        mPoints[k].x = MS_NINT(style->scale*symbol->points[j].x + x);
        mPoints[k].y = MS_NINT(style->scale*symbol->points[j].y + y);
        k++;
      }
    }

    if(fc >= 0)
      gdImageFilledPolygon(ip, mPoints, k, fc);
    if(oc >= 0)
      gdImagePolygon(ip, mPoints, k, oc);

  } else  { /* NOT filled */

    if(oc >= 0) fc = oc; /* try the outline color (reference maps sometimes do this when combining a box and a custom vector marker */

    oldpnt.x = MS_NINT(style->scale*symbol->points[0].x + x); /* convert first point in marker */
    oldpnt.y = MS_NINT(style->scale*symbol->points[0].y + y);

    gdImageSetThickness(ip, style->outlinewidth);

    for(j=1; j < symbol->numpoints; j++) { /* step through the marker */
      if((symbol->points[j].x != -99) || (symbol->points[j].y != -99)) {
        if((symbol->points[j-1].x == -99) && (symbol->points[j-1].y == -99)) { /* Last point was PENUP, now a new beginning */
          oldpnt.x = MS_NINT(style->scale*symbol->points[j].x + x);
          oldpnt.y = MS_NINT(style->scale*symbol->points[j].y + y);
        } else {
          newpnt.x = MS_NINT(style->scale*symbol->points[j].x + x);
          newpnt.y = MS_NINT(style->scale*symbol->points[j].y + y);
          gdImageLine(ip, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, fc);
          oldpnt = newpnt;
        }
      }
    } /* end for loop */

    gdImageSetThickness(ip, 1); /* restore thinkness */
  } /* end if-then-else */

  if(bRotated) {
    msFreeSymbol(symbol); /* clean up */
    msFree(symbol);
  }
  return MS_SUCCESS;
}

int renderTruetypeSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *s)
{
#ifdef USE_GD_FREETYPE
  int bbox[8];
  char *error;
  int c,oc = 0;
  gdImagePtr ip;
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;
  SETPEN(ip, s->color);
  SETPEN(ip, s->outlinecolor);

  if(s->style->antialias) {
    c = s->color->pen;
    if(s->outlinecolor)
      oc = s->outlinecolor->pen;
  } else {
    c = -s->color->pen;
    if(s->outlinecolor)
      oc = -s->outlinecolor->pen;
  }
  gdImageStringFT(NULL, bbox, c, symbol->full_font_path, s->scale, s->rotation, 0,0, symbol->character);

  x -=  (bbox[2] - bbox[0])/2 + bbox[0];
  y +=  (bbox[1] - bbox[5])/2 - bbox[1];

  if( s->outlinecolor ) {
    error = gdImageStringFT(ip, bbox, oc, symbol->full_font_path, s->scale, s->rotation, x, y-1, symbol->character);
    if(error) {
      msSetError(MS_TTFERR, "%s", "renderTruetypeSymbolGD()", error);
      return MS_FAILURE;
    }

    gdImageStringFT(ip, bbox, oc, symbol->full_font_path, s->scale, s->rotation, x, y+1, symbol->character);
    gdImageStringFT(ip, bbox, oc, symbol->full_font_path, s->scale, s->rotation, x+1, y, symbol->character);
    gdImageStringFT(ip, bbox, oc, symbol->full_font_path, s->scale, s->rotation, x-1, y, symbol->character);
    gdImageStringFT(ip, bbox, oc, symbol->full_font_path, s->scale, s->rotation, x+1, y+1, symbol->character);
    gdImageStringFT(ip, bbox, oc, symbol->full_font_path, s->scale, s->rotation, x+1, y-1, symbol->character);
    gdImageStringFT(ip, bbox, oc, symbol->full_font_path, s->scale, s->rotation, x-1, y+1, symbol->character);
    gdImageStringFT(ip, bbox, oc, symbol->full_font_path, s->scale, s->rotation, x-1, y-1, symbol->character);
  }
  if(s->color)
    gdImageStringFT(ip, bbox, c, symbol->full_font_path, s->scale, s->rotation, x, y, symbol->character);
  return MS_SUCCESS;
#else
  msSetError(MS_TTFERR, "Freetype support not enabled in this GD build", "renderTruetypeSymbolGD()");
  return MS_FAILURE;
#endif
}

gdImagePtr rotatePixmapGD(gdImagePtr img, double angle_rad)
{
  gdImagePtr rimg = NULL;
  double cos_a, sin_a;
  double x1 = 0.0, y1 = 0.0; /* destination rectangle */
  double x2 = 0.0, y2 = 0.0;
  double x3 = 0.0, y3 = 0.0;
  double x4 = 0.0, y4 = 0.0;

  long minx, miny, maxx, maxy;

  int width=0, height=0;
  /* int color; */

  sin_a = sin(angle_rad);
  cos_a = cos(angle_rad);

  /* compute distination rectangle (x1,y1 is known) */
  x1 = 0 ;
  y1 = 0 ;
  x2 = img->sy * sin_a;
  y2 = -img->sy * cos_a;
  x3 = (img->sx * cos_a) + (img->sy * sin_a);
  y3 = (img->sx * sin_a) - (img->sy * cos_a);
  x4 = (img->sx * cos_a);
  y4 = (img->sx * sin_a);

  minx = (long) MS_MIN(x1,MS_MIN(x2,MS_MIN(x3,x4)));
  miny = (long) MS_MIN(y1,MS_MIN(y2,MS_MIN(y3,y4)));
  maxx = (long) MS_MAX(x1,MS_MAX(x2,MS_MAX(x3,x4)));
  maxy = (long) MS_MAX(y1,MS_MAX(y2,MS_MAX(y3,y4)));

  width = (int)ceil(maxx-minx);
  height = (int)ceil(maxy-miny);

  /* create the new image based on the computed width/height */

  if (gdImageTrueColor(img)) {
    rimg = gdImageCreateTrueColor(width, height);
    gdImageAlphaBlending(rimg, 0);
    gdImageFilledRectangle(rimg, 0, 0, width, height, gdImageColorAllocateAlpha(rimg, 0, 0, 0, gdAlphaTransparent));
  } else {
    int tc = gdImageGetTransparent(img);
    rimg = gdImageCreate(width, height);
    if(tc != -1)
      gdImageColorTransparent(rimg, gdImageColorAllocate(rimg, gdImageRed(img, tc), gdImageGreen(img, tc), gdImageBlue(img, tc)));
  }
  if(!rimg) {
    msSetError(MS_GDERR,"failed to create rotated pixmap","rotatePixmapGD()");
    return NULL;
  }

  gdImageCopyRotated (rimg, img, width*0.5, height*0.5, 0, 0, gdImageSX(img), gdImageSY(img), angle_rad*MS_RAD_TO_DEG);
  return rimg;
}

int renderPixmapSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style)
{
  gdImagePtr ip,pp;
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;
  assert(symbol->pixmap_buffer && symbol->pixmap_buffer->type == MS_BUFFER_GD);
  pp = symbol->pixmap_buffer->data.gd_img;
  /* gdImageAlphaBlending(ip,1); */
  /* gdImageAlphaBlending(pp,1); */

  if(symbol->transparent)
    gdImageColorTransparent(pp,symbol->transparentcolor);
  if(style->scale == 1.0 && style->rotation == 0.0) { /* don't scale */
    x -= .5*symbol->pixmap_buffer->width;
    y -= .5*symbol->pixmap_buffer->height;
    gdImageCopy(ip, pp, x, y, 0, 0, symbol->pixmap_buffer->width,symbol->pixmap_buffer->height);
  } else {
    int bRotated = MS_FALSE;
    if(style->rotation) {
      bRotated = MS_TRUE;
      pp = rotatePixmapGD(pp,style->rotation);
    }
    x -=  .5*gdImageSX(pp)*style->scale;
    y -=  .5*gdImageSY(pp)*style->scale;
    gdImageCopyResampled(ip, pp, x, y, 0, 0,
                         (int)(gdImageSX(pp) * style->scale),
                         (int)(gdImageSY(pp) * style->scale),
                         gdImageSX(pp),gdImageSY(pp));
    if(bRotated) {
      gdImageDestroy(pp);
    }
  }
  /* gdImageAlphaBlending(ip,0); */
  return MS_SUCCESS;
}

int renderTileGD(imageObj *img, imageObj *tile, double x, double y)
{
  return MS_SUCCESS;
}

int renderPolygonTiledGD(imageObj *img, shapeObj *p,  imageObj *tile)
{
  gdImagePtr ip, tp;

  if(!img || !p || !tile) return MS_FAILURE;
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;
  if(!(tp = MS_IMAGE_GET_GDIMAGEPTR(tile))) return MS_FAILURE;
  gdImageColorTransparent(tp,0);
  gdImageSetTile(ip, tp);
  imageFilledPolygon(ip, p, gdTiled);
  return MS_SUCCESS;
}


int initializeRasterBufferGD(rasterBufferObj *rb, int width, int height, int mode)
{
  rb->type = MS_BUFFER_GD;
  rb->data.gd_img = gdImageCreate(width, height);
  if(!rb->data.gd_img)
    return MS_FAILURE;
  rb->width = width;
  rb->height= height;
  return MS_SUCCESS;
}

/*
** GD driver-specific raster buffer functions.
*/
int getRasterBufferHandleGD(imageObj *img, rasterBufferObj *rb)
{
  gdImagePtr gdImg = MS_IMAGE_GET_GDIMAGEPTR(img);
  rb->type = MS_BUFFER_GD;
  rb->data.gd_img = gdImg;
  rb->width = gdImg->sx;
  rb->height = gdImg->sy;
  return MS_SUCCESS;

}

int getRasterBufferCopyGD(imageObj *img, rasterBufferObj *rb)
{
  gdImagePtr gdImg = MS_IMAGE_GET_GDIMAGEPTR(img);
  int row;
  rb->type = MS_BUFFER_GD;
  rb->width = gdImg->sx;
  rb->height = gdImg->sy;
  rb->data.gd_img = gdImageCreate(gdImg->sx,gdImg->sy);
  rb->data.gd_img->transparent = gdImg->transparent;
  gdImagePaletteCopy(rb->data.gd_img,gdImg);
  for(row=0; row<gdImg->sy; row++)
    memcpy(rb->data.gd_img->pixels[row],gdImg->pixels[row],gdImg->sx);
  return MS_SUCCESS;
}

int mergeRasterBufferGD(imageObj *dest, rasterBufferObj *overlay, double opacity, int srcX, int srcY, int dstX, int dstY, int width, int height)
{
  assert(dest && overlay);
  assert(overlay->type == MS_BUFFER_GD);
  gdImageCopyMerge(MS_IMAGE_GET_GDIMAGEPTR(dest),overlay->data.gd_img,dstX,dstY,srcX,srcY,width,height,opacity*100);
  return MS_SUCCESS;
}

int getTruetypeTextBBoxGD(rendererVTableObj *renderer, char **fonts, int numfonts, double size, char *string, rectObj *rect, double **advances, int bAdjustBaseline)
{
#ifdef USE_GD_FREETYPE
  int bbox[8];
  char *error;
  if(advances) {
    char *s;
    int k;
    gdFTStringExtra strex;
    strex.flags = gdFTEX_XSHOW;
    error = gdImageStringFTEx(NULL, bbox, 0, fonts[0], size, 0, 0, 0, string, &strex);
    if(error) {
      msSetError(MS_TTFERR, "%s", "gdImageStringFTEx()", error);
      return(MS_FAILURE);
    }

    *advances = (double*)malloc( strlen(string) * sizeof(double) );
    MS_CHECK_ALLOC(*advances, strlen(string) * sizeof(double), MS_FAILURE);
    s = strex.xshow;
    k = 0;
    /* TODO this smells buggy and can cause errors at a higher level. strlen NOK here*/
    while ( *s && k < strlen(string) ) {
      (*advances)[k++] = atof(s);
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
    return MS_SUCCESS;
  } else {
    error = gdImageStringFT(NULL, bbox, 0, fonts[0], size, 0, 0, 0, string);
    if(error) {
      msSetError(MS_TTFERR, "%s", "msGetTruetypeTextBBoxGD()", error);
      return(MS_FAILURE);
    }

    rect->minx = bbox[0];
    rect->miny = bbox[5];
    rect->maxx = bbox[2];
    rect->maxy = bbox[1];
    return MS_SUCCESS;
  }
#else
  msSetError(MS_TTFERR, "Freetype support not enabled in this GD build", "getTruetypeTextBBoxGD()");
  return MS_FAILURE;
#endif
}

int renderBitmapGlyphsGD(imageObj *img, double x, double y, labelStyleObj *style, char *text)
{
  int size = MS_NINT(style->size);
  gdFontPtr fontPtr;
  gdImagePtr ip;
  int numlines=0,t;
  char **lines;
  if(!(ip = MS_IMAGE_GET_GDIMAGEPTR(img))) return MS_FAILURE;
  if(size<0 || size>4 || (fontPtr = msGetBitmapFont(size))==NULL) {
    msSetError(MS_RENDERERERR,"invalid bitmap font size", "renderBitmapGlyphsGD()");
    return MS_FAILURE;
  }

  SETPEN(ip, style->color);
  SETPEN(ip, style->outlinecolor);

  if(msCountChars(text,'\n')) {
    if((lines = msStringSplit((const char*)text, '\n', &(numlines))) == NULL)
      return(-1);
  } else {
    lines = &text;
    numlines = 1;
  }

  y -= fontPtr->h;
  for(t=0; t<numlines; t++) {
    if(style->outlinewidth > 0) {
      gdImageString(ip, fontPtr, x, y-1,   (unsigned char *) lines[t], style->outlinecolor->pen);
      gdImageString(ip, fontPtr, x, y+1,   (unsigned char *) lines[t], style->outlinecolor->pen);
      gdImageString(ip, fontPtr, x+1, y,   (unsigned char *) lines[t], style->outlinecolor->pen);
      gdImageString(ip, fontPtr, x-1, y,   (unsigned char *) lines[t], style->outlinecolor->pen);
      gdImageString(ip, fontPtr, x+1, y-1, (unsigned char *) lines[t], style->outlinecolor->pen);
      gdImageString(ip, fontPtr, x+1, y+1, (unsigned char *) lines[t], style->outlinecolor->pen);
      gdImageString(ip, fontPtr, x-1, y-1, (unsigned char *) lines[t], style->outlinecolor->pen);
      gdImageString(ip, fontPtr, x-1, y+1, (unsigned char *) lines[t], style->outlinecolor->pen);
    }
    if(style->color->pen != -1) {
      gdImageString(ip, fontPtr, x, y, (unsigned char *) lines[t], style->color->pen);
    }

    y += fontPtr->h; /* shift down */
  }


  if(lines != &text)
    msFreeCharArray(lines, numlines);
  return MS_SUCCESS;
}

/*
** Misc. cleanup functions for GD driver.
*/

int freeSymbolGD(symbolObj *s)
{
  return MS_SUCCESS;
}

int msPopulateRendererVTableGD( rendererVTableObj *renderer )
{
  int i;
  renderer->use_imagecache=0;
  renderer->supports_pixel_buffer=1;
  renderer->supports_transparent_layers = 0;
  renderer->supports_bitmap_fonts = 1;
  renderer->default_transform_mode = MS_TRANSFORM_ROUND;

  for(i=0; i<5; i++) {
    gdFontPtr f = msGetBitmapFont(i);
    bitmapFontMetricsGD[i].charWidth = f->w;
    bitmapFontMetricsGD[i].charHeight = f->h;
    renderer->bitmapFontMetrics[i] = &bitmapFontMetricsGD[i];
  }


  renderer->startLayer = startNewLayerGD;
  renderer->endLayer = closeNewLayerGD;
  renderer->renderLineTiled = NULL;
  renderer->renderLine = &renderLineGD;
  renderer->createImage = &createImageGD;
  renderer->saveImage = &saveImageGD;
  renderer->getRasterBufferHandle = &getRasterBufferHandleGD;
  renderer->getRasterBufferCopy = &getRasterBufferCopyGD;
  renderer->initializeRasterBuffer = initializeRasterBufferGD;
  renderer->loadImageFromFile = msLoadGDRasterBufferFromFile;
  renderer->renderPolygon = &renderPolygonGD;
  renderer->renderGlyphs = &renderGlyphsGD;
  renderer->renderBitmapGlyphs = &renderBitmapGlyphsGD;
  renderer->freeImage = &freeImageGD;
  renderer->renderEllipseSymbol = &renderEllipseSymbolGD;
  renderer->renderVectorSymbol = &renderVectorSymbolGD;
  renderer->renderTruetypeSymbol = &renderTruetypeSymbolGD;
  renderer->renderPixmapSymbol = &renderPixmapSymbolGD;
  renderer->mergeRasterBuffer = &mergeRasterBufferGD;
  renderer->getTruetypeTextBBox = &getTruetypeTextBBoxGD;
  renderer->renderTile = &renderTileGD;
  renderer->renderPolygonTiled = &renderPolygonTiledGD;
  renderer->freeSymbol = &freeSymbolGD;
  return MS_SUCCESS;
}

#endif

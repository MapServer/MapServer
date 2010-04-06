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
#include "mapserver.h"
#include "mapthread.h"
#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define MS_IMAGE_GET_GDIMAGEPTR(image) ((gdImagePtr) image->img.plugin)

void startNewLayerGD(imageObj *img, double opacity) {
}

void closeNewLayerGD(imageObj *img, double opacity) { 
}

/*
** GD driver-specific image handling functions.
*/
imageObj *createImageGD(int width, int height, outputFormatObj *format, colorObj* bg) 
{
  imageObj *img = NULL;
  gdImagePtr ip;

  fprintf(stderr, "in createImageGD() w=%d h=%d\n", width, height);

  img = (imageObj *) calloc(1, sizeof (imageObj));

  /* we're only doing PC256 for the moment */
  ip = gdImageCreate(width, height);
  gdImageColorAllocate(ip, bg->red, bg->green, bg->blue); /* set the background color */

  img->img.plugin = (void *) ip;
  return img;
}

int saveImageGD(imageObj *img, FILE *fp, outputFormatObj *format) 
{
  gdImagePtr ip;

  fprintf(stderr, "in saveImageGD()\n");

  if(!img || !fp) return MS_FAILURE;
  ip = MS_IMAGE_GET_GDIMAGEPTR(img);

  if(strcasecmp("ON", msGetOutputFormatOption(format, "INTERLACE", "ON")) == 0)
    gdImageInterlace(ip, 1);

  if(format->transparent)
    gdImageColorTransparent(ip, 0);

  if(strcasecmp(format->driver, "gd2/gif") == 0) {
#ifdef USE_GD_GIF
    gdImageGif(ip, fp);
#else
    msSetError(MS_MISCERR, "GIF output is not available.", "saveImageGD()");
    return(MS_FAILURE);
#endif
  } else if(strcasecmp(format->driver, "gd2/png") == 0) {
#ifdef USE_GD_PNG
    gdImagePng(ip, fp);
#else
    msSetError(MS_MISCERR, "PNG output is not available.", "saveImageGD()");
    return(MS_FAILURE);
#endif
  } else if(strcasecmp(format->driver, "gd2/jpeg") == 0) {
#ifdef USE_GD_JPEG
    gdImageJpeg(ip, fp, atoi(msGetOutputFormatOption( format, "QUALITY", "75")));
#else
    msSetError(MS_MISCERR, "JPEG output is not available.", "saveImageGD()");
    return(MS_FAILURE);
#endif
  } else {
    msSetError(MS_MISCERR, "Unknown or unsupported format.", "saveImageGD()");
    return(MS_FAILURE);
  }

  return MS_SUCCESS;
}

void freeImageGD(imageObj *img) 
{
  gdImagePtr ip;

  if(img) {
    ip = MS_IMAGE_GET_GDIMAGEPTR(img);
    if(ip) gdImageDestroy(ip);
  }
}

/*
** GD driver-specific rendering functions.
*/
#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

static void setPen(gdImagePtr ip, colorObj *c)
{
  if(MS_VALID_COLOR(*c))
    c->pen = gdImageColorResolve(ip, c->red, c->green, c->blue);
  else
    c->pen = -1;
}

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
** Heckbert from "Graphics Gems", Academic Press, 1990 
*/
static void imageFilledPolygon(gdImagePtr im, shapeObj *p, int c, int offsetx, int offsety)
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

  edge = (pEdge *) calloc(n,sizeof(pEdge));           /* All edges in the polygon */
  edgeindex =  (int *) calloc(n,sizeof(int));         /* Index to edges sorted by scanline */
  active = (pEdge **) calloc(n,sizeof(pEdge*));       /* Pointers to active edges for current scanline */
    
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
  yhist = (int*) calloc(ymax - ymin + 2, sizeof(int));
  for(i=0;i<nvert;i++) {
    yhist[ edge[i].s - ymin + 1 ]++;
  }
  for(i=0; i<=(ymax - ymin); i++)  {/* Calculate starting point in edgeindex for each scanline */
    yhist[i+1] += yhist[i]; 
  }
  for(i=0;i<nvert;i++){ /* Bucket sort edges into edgeindex */
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
        imageScanline(im, xl+offsetx, xr+offsetx, y+offsety, c);
               
      active[j]->x += active[j]->dx;	/* increment edge coords */
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

  for (i=0; i < p->numlines; i++)
    for(j=1; j<p->line[i].numpoints; j++)
      gdImageLine(im, (int) p->line[i].point[j-1].x, (int) p->line[i].point[j-1].y, (int) p->line[i].point[j].x, (int) p->line[i].point[j].y, c);
}

void renderLineGD(imageObj *img, shapeObj *p, strokeStyleObj *stroke) 
{
  gdImagePtr ip;
  int c;

  if(!img || !p || !stroke) return;
  ip = MS_IMAGE_GET_GDIMAGEPTR(img);

  if(stroke->color.pen == MS_PEN_UNSET) setPen(ip, &stroke->color);
  c = stroke->color.pen;

  imagePolyline(ip, p, c);
}

void renderPolygonGD(imageObj *img, shapeObj *p, colorObj *color) 
{  
  gdImagePtr ip;

  if(!img || !p || !color) return;
  ip = MS_IMAGE_GET_GDIMAGEPTR(img);
  if(color->pen == MS_PEN_UNSET) setPen(ip, color);
  imageFilledPolygon(ip, p, color->pen, 0, 0);
}

void renderGlyphsLineGD(imageObj *img, labelPathObj *labelpath, labelStyleObj *style, char *text) {
}

void renderGlyphsGD(imageObj *img,double x, double y, labelStyleObj *style, char *text) {
}

void renderEllipseSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style) {
}

void renderVectorSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style) {
}

void renderTruetypeSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *s) {
}

void renderPixmapSymbolGD(imageObj *img, double x, double y, symbolObj *symbol, symbolStyleObj *style) {
}

void renderTileGD(imageObj *img, imageObj *tile, double x, double y) {
}

void renderPolygonTiledGD(imageObj *img, shapeObj *p,  imageObj *tile) {
}

/*
** GD driver-specific raster buffer functions.
*/
void getRasterBufferGD(imageObj *img, rasterBufferObj *rb) {
}

void mergeRasterBufferGD(imageObj *img, rasterBufferObj *rb, double opacity, int dstX, int dstY) {
}

int getTruetypeTextBBoxGD(imageObj *img,char *font, double size, char *text, rectObj *rect, double **advances) {
  return MS_SUCCESS;
}

/*
** Misc. cleanup functions for GD driver.
*/
void freeTileGD(imageObj *tile) {
  freeImageGD(tile);
}

void freeSymbolGD(symbolObj *s) {
}

int msPopulateRendererVTableGD( rendererVTableObj *renderer ) {
  renderer->supports_imagecache=0;
  renderer->supports_pixel_buffer=0;
  renderer->supports_transparent_layers = 0;
  renderer->startNewLayer = startNewLayerGD;
  renderer->closeNewLayer = closeNewLayerGD;
  renderer->renderLine = &renderLineGD;
  renderer->createImage = &createImageGD;
  renderer->saveImage = &saveImageGD;
  renderer->getRasterBuffer = &getRasterBufferGD;
  renderer->transformShape = &msTransformShapeToPixel;
  renderer->renderPolygon = &renderPolygonGD;
  renderer->renderGlyphsLine = &renderGlyphsLineGD;
  renderer->renderGlyphs = &renderGlyphsGD;
  renderer->freeImage = &freeImageGD;
  renderer->renderEllipseSymbol = &renderEllipseSymbolGD;
  renderer->renderVectorSymbol = &renderVectorSymbolGD;
  renderer->renderTruetypeSymbol = &renderTruetypeSymbolGD;
  renderer->renderPixmapSymbol = &renderPixmapSymbolGD;
  renderer->mergeRasterBuffer = &mergeRasterBufferGD;
  renderer->getTruetypeTextBBox = &getTruetypeTextBBoxGD;
  renderer->renderTile = &renderTileGD;
  renderer->renderPolygonTiled = &renderPolygonTiledGD;
  renderer->freeTile = &freeTileGD;
  renderer->freeSymbol = &freeSymbolGD;
  return MS_SUCCESS;
}

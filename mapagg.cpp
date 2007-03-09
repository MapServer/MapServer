/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  AGG/GD rendering and other AGG related functions.
 * Author:   Steve Lime and the MapServer team.
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.4  2007/03/09 17:22:37  sdlime
 * Clean-up of formatting (removed tabs, standardized indentation), removed a few meaningless functions, put pLogFile checks in.
 *
 * Revision 1.3  2007/03/08 22:41:12  assefa
 * Correct polygon drawing.
 *
 * Revision 1.2  2007/03/07 20:56:13  sdlime
 * Trimmed a bunch of unnecessary history from mapagg.cpp.
 *
 * Revision 1.1  2007/03/06 11:05:00  novak
 * Initial Checkin
 *
 */

#ifdef USE_AGG

#include "map.h"
#include "mapthread.h"

#include <time.h>

#ifdef USE_ICONV
#include <iconv.h>
#endif

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "gd.h"
#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_rasterizer_outline.h"
#include "agg_rasterizer_outline_aa.h"

#include "agg_conv_transform.h"
#include "agg_conv_stroke.h"

#include "agg_scanline_p.h"
#include "agg_scanline_u.h"
#include "agg_path_storage.h"

#include "agg_renderer_base.h"
#include "agg_renderer_primitives.h"
#include "agg_renderer_scanline.h"
#include "agg_renderer_outline_aa.h"

#include "agg_pixfmt_rgb.h"
#include "agg_pixfmt_rgba.h"

#include "agg_dda_line.h"
#include "agg_ellipse_bresenham.h"
#include "mapagg.h"

typedef agg::pixfmt_alpha_blend_rgba<agg::blender_bgra32_plain,mapserv_row_ptr_cache<int>,int> pixelFormat;

MS_CVSID("$Id$")

static unsigned char PNGsig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; /* 89 50 4E 47 0D 0A 1A 0A hex */
static unsigned char JPEGsig[3] = {255, 216, 255}; /* FF D8 FF hex */

static FILE *pLogFile = NULL;
//------------------------------------------------------------------------------------------------------------
int msImageSetPenAGG(gdImagePtr img, colorObj *color) 
{
  return msImageSetPenGD(img, color);
}

//------------------------------------------------------------------------------------------------------------
/*
 * Take a pass through the mapObj and pre-allocate colors for layers that are ON or DEFAULT. This replicates the pre-4.0 behavior of
 * MapServer and should be used only with paletted images.
 */
void msPreAllocateColorsAGG(imageObj *image, mapObj *map) {
  msPreAllocateColorsGD(image, map);
}

//------------------------------------------------------------------------------------------------------------
/*
 * Utility function to create a GD image. Returns
 * a pointer to an imageObj structure.
 */  
imageObj *msImageCreateAGG(int width, int height, outputFormatObj *format, char *imagepath, char *imageurl) 
{
  imageObj *pNewImage = NULL;
  
  pNewImage  = msImageCreateGD(width, height, format, imagepath, imageurl);
  
  if(! pNewImage)
    return pNewImage;
    
  mapserv_row_ptr_cache<int>  *pRowCache = new mapserv_row_ptr_cache<int>(pNewImage->img.gd);
  
  if(! pRowCache)
    return NULL;
    
  pNewImage->imageextra  = (void *) pRowCache;

  /* things will break if this file cannot be created, should be removed eventually */  
  pLogFile  = fopen("/tmp/debug.log", "w");
  // pLogFile  = fopen("c:/tmp/debug.log", "w");
  
  return pNewImage;
}

//------------------------------------------------------------------------------------------------------------
/**
 * Utility function to initialize the color of an image.  The background
 * color is passed, but the outputFormatObj is consulted to see if the
 * transparency should be set (for RGBA images).   Note this function only
 * affects TrueColor images. 
 */  

//------------------------------------------------------------------------------------------------------------
void msImageInitAGG(imageObj *image, colorObj *background)
{
  if(image->format->imagemode == MS_IMAGEMODE_PC256) {
    gdImageColorAllocate(image->img.gd, background->red, background->green, background->blue);
    return;
  }

  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
  if(pRowCache == NULL) {
    fprintf(pLogFile, "msImageInitAGG pRowCache == NULL, extra is %08x\n", image->imageextra); 
    return;
  }

  if(pLogFile) fprintf(pLogFile, "msImageInitAGG Clearing Buffer\n"); 
  
  if(image->format->imagemode == MS_IMAGEMODE_RGBA) {
    agg::pixfmt_alpha_blend_rgba<agg::blender_bgra32,mapserv_row_ptr_cache<int>,int> thePixelFormat(*pRowCache);
     agg::renderer_base< agg::pixfmt_alpha_blend_rgba<agg::blender_bgra32,mapserv_row_ptr_cache<int>,int> > ren_base(thePixelFormat);

    ren_base.clear(agg::rgba(background->red / 255.0, background->green / 255.0, background->blue / 255.0, 0.0));
  } else {
    agg::pixfmt_alpha_blend_rgb< ms_blender_bgr24, mapserv_row_ptr_cache<int> > thePixelFormat(*pRowCache);
     agg::renderer_base< agg::pixfmt_alpha_blend_rgb< ms_blender_bgr24, mapserv_row_ptr_cache<int> > > ren_base(thePixelFormat);

    ren_base.clear(agg::rgba(background->red, background->green, background->blue));
  }

  msImageInitGD(image, background);
}

//------------------------------------------------------------------------------------------------------------
/* ===========================================================================
   msImageLoadGDCtx
   
   So that we can avoid passing a FILE* to GD, all gd image IO is now done
   through the fileIOCtx interface defined at the end of this file.  The
   old msImageLoadStreamGD function has been removed.
   ========================================================================= */   
imageObj *msImageLoadAGGCtx(gdIOCtx* ctx, const char *driver) 
{
  return msImageLoadGDCtx(ctx, driver);
}

//------------------------------------------------------------------------------------------------------------
/* msImageLoadAGG now calls msImageLoadAGGCtx to do the work, change
 * made as part of the resolution of bugs 550 and 1047 */
imageObj *msImageLoadAGG(const char *filename) 
{
  return msImageLoadGD(filename); 
}

//------------------------------------------------------------------------------------------------------------
static gdImagePtr createFuzzyBrush(int size, int r, int g, int b)
{
  gdImagePtr brush;
  int x, y, c, dx, dy;
  long bgcolor, color;
  int a;
  double d, min_d, max_d;
  double hardness=.5;
  
  if(pLogFile) fprintf(pLogFile, "createFuzzyBrush entry\n");

  if(size % 2 == 0) /* requested an even-sized brush, subtract one from size */
    size--;

  brush = gdImageCreateTrueColor(size+2, size+2);
  gdImageAlphaBlending(brush, 0); /* don't blend */

  bgcolor = gdImageColorAllocateAlpha(brush, 255, 255, 255, 127); /* fill the brush as transparent */
  gdImageFilledRectangle(brush, 0, 0, gdImageSX(brush), gdImageSY(brush), bgcolor);

  c = (gdImageSX(brush)-1)/2; /* center coordinate (x=y) */

  min_d = hardness*(size/2.0) - 0.5;
  max_d = gdImageSX(brush)/2.0;

  color = gdImageColorAllocateAlpha(brush, r, g, b, 0);
  gdImageFilledEllipse(brush, c, c, gdImageSX(brush), gdImageSY(brush), color); /* draw the base circle as opaque */

  for(y=0; y<gdImageSY(brush); y++) { /* each row */
    for(x=0; x<gdImageSX(brush); x++) { /* each column */
      color = gdImageGetPixel(brush, x, y);
      if(color == bgcolor) continue;

      dx = x - c;
      dy = y - c;
      d = sqrt((double)(dx*dx + dy*dy));

      if(d<min_d) continue; /* leave opaque */

      a = MS_NINT(127*(d/max_d));

      color = gdImageColorAllocateAlpha(brush, r, g, b, a);
      gdImageSetPixel(brush, x, y, color);
    }
  }

  return brush;
}

//------------------------------------------------------------------------------------------------------------
static gdImagePtr createBrush(gdImagePtr img, int width, int height, styleObj *style, int *fgcolor, int *bgcolor)
{
  gdImagePtr brush;
  
  if(pLogFile) fprintf(pLogFile, "createBrush entry\n");

  if(width == 0) width = 1; /* quick fix for bug 1776, should really handle with rounding when calling this function */
  if(height == 0) height = 1;

  if(!gdImageTrueColor(img)) {
    brush = gdImageCreate(width, height);
    if(style->backgroundcolor.pen >= 0)
      *bgcolor = gdImageColorAllocate(brush, style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);
    else {
      *bgcolor = gdImageColorAllocate(brush, gdImageRed(img,0), gdImageGreen(img, 0), gdImageBlue(img, 0));          
      gdImageColorTransparent(brush,0);
    }
    if(style->color.pen >= 0)
      *fgcolor = gdImageColorAllocate(brush, style->color.red, style->color.green, style->color.blue);
    else /* try outline color */
      *fgcolor = gdImageColorAllocate(brush, style->outlinecolor.red, style->outlinecolor.green, style->outlinecolor.blue); 
  } else {
    brush = gdImageCreateTrueColor(width, height);
    gdImageAlphaBlending(brush, 0);
    if(style->backgroundcolor.pen >= 0)
      *bgcolor = gdTrueColor(style->backgroundcolor.red, style->backgroundcolor.green, style->backgroundcolor.blue);      
    else
      *bgcolor = -1;      
    gdImageFilledRectangle(brush, 0, 0, width, height, *bgcolor);
    if(style->color.pen >= 0)
      *fgcolor = gdTrueColor(style->color.red, style->color.green, style->color.blue);
    else /* try outline color */
      *fgcolor = gdTrueColor(style->outlinecolor.red, style->outlinecolor.green, style->outlinecolor.blue);
  }

  return(brush);
}

//------------------------------------------------------------------------------------------------------------
/* Function to create a custom hatch symbol. */
static gdImagePtr createHatch(gdImagePtr img, int sx, int sy, rectObj *clip, styleObj *style, double scalefactor)
{
  gdImagePtr hatch;
  int x1, x2, y1, y2;
  int size, width;
  double angle;
  int fg, bg;

  if(pLogFile) fprintf(pLogFile, "createHatch entry\n");

  hatch = createBrush(img, sx, sy, style, &fg, &bg);

  if(style->antialias == MS_TRUE) {
    gdImageSetAntiAliased(hatch, fg);
    fg = gdAntiAliased;
  }

  if(style->size == -1)
    size = 1; /* TODO: Can we use msSymbolGetDefaultSize() here? See bug 751. */
  else
    size = style->size;

  size = MS_NINT(size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  width = MS_NINT(style->width*scalefactor);
  width = MS_MAX(width, style->minwidth);
  width = MS_MIN(width, style->maxwidth);
  gdImageSetThickness(hatch, width);

  /* normalize the angle (180 to 0, 0 is east, 90 is north 180 is west) */
  angle = fmod(style->angle, 360.0);
  if(angle < 0) angle += 360;
  if(angle >= 180) angle -= 180;

  if(angle >= 45 && angle <= 90) {
    x2 = (int)clip->minx; /* 0 */
    y2 = (int)clip->miny; /* 0 */
    y1 = (int)clip->maxy-1; /* sy-1  */
    x1 = (int) (x2 - (y2 - y1)/tan(-MS_DEG_TO_RAD*angle));

    size = MS_ABS(MS_NINT(size/sin(MS_DEG_TO_RAD*(angle))));

    while(x1 < clip->maxx) { /* sx */
      gdImageLine(hatch, x1, y1, x2, y2, fg);
      x1+=size;
      x2+=size; 
    }
  } else if(angle <= 135 && angle > 90) {
    x2 = (int)clip->minx; /* 0 */
    y2 = (int)clip->maxy-1; /* sy-1 */
    y1 = (int)clip->miny; /* 0 */
    x1 = (int) (x2 - (y2 - y1)/tan(-MS_DEG_TO_RAD*angle));

    size = MS_ABS(MS_NINT(size/sin(MS_DEG_TO_RAD*(angle))));

    while(x1 < clip->maxx) { /* sx */
      gdImageLine(hatch, x1, y1, x2, y2, fg);
      x1+=size;
      x2+=size;
    }
  } else if(angle >= 0 && angle < 45) {    
    x1 = (int)clip->minx; /* 0 */
    y1 = (int)clip->miny; /* 0 */
    x2 = (int)clip->maxx-1; /* sx-1 */
    y2 = (int)(y1 + (x2 - x1)*tan(-MS_DEG_TO_RAD*angle));

    size = MS_ABS(MS_NINT(size/cos(MS_DEG_TO_RAD*(angle))));

    while(y2 < clip->maxy) { /* sy */
      gdImageLine(hatch, x1, y1, x2, y2, fg);
      y1+=size;
      y2+=size;
    }
  } else if(angle < 180 && angle > 135) {
    x2 = (int)clip->maxx-1; /* sx-1 */
    y2 = (int)clip->miny; /* 0 */
    x1 = (int)clip->minx; /* 0 */
    y1 = (int) (y2 - (x2 - x1)*tan(-MS_DEG_TO_RAD*angle));

    size = MS_ABS(MS_NINT(size/cos(MS_DEG_TO_RAD*(angle))));

    while(y1 < clip->maxy) { /* sy */
      gdImageLine(hatch, x1, y1, x2, y2, fg);
      y1+=size;
      y2+=size;
    }
  }

  gdImageSetThickness(hatch, 1);
  return(hatch);
}

//------------------------------------------------------------------------------------------------------------
static void imageOffsetPolyline(gdImagePtr img, shapeObj *p, int color, int offsetx, int offsety)
{
  int i, j, first;
  int dx, dy, dx0=0, dy0=0, ox=0, oy=0, limit;
  double x, y, x0=0.0, y0=0.0, k=0.0, k0=0.0, q=0.0, q0=0.0;
  float par=(float)0.71;
  
  if(pLogFile) fprintf(pLogFile, "imageOffsetPolyline entry\n");

  if(offsety == -99) {
    limit = offsetx*offsetx/4;
    for (i = 0; i < p->numlines; i++) {
      first = 1;
      for(j=1; j<p->line[i].numpoints; j++) {        
        ox=0; oy=0;
        dx = (int)(p->line[i].point[j].x - p->line[i].point[j-1].x);
        dy = (int)(p->line[i].point[j].y - p->line[i].point[j-1].y);

        /* offset setting - quick approximation, may be changed with goniometric functions */
        if(dx==0) { /* vertical line */
          if(dy==0) continue; /* checking unique points */
          ox=(dy>0) ? -offsetx : offsetx;
        } else {
          k = (double)dy/(double)dx;
          if(MS_ABS(k)<0.5) {
            oy = (dx>0) ? offsetx : -offsetx;
          } else {
            if (MS_ABS(k)<2.1) {
              oy = (int) ((dx>0) ? offsetx*par : -offsetx*par);
              ox = (int) ((dy>0) ? -offsetx*par : offsetx*par);
            } else
              ox = (int)((dy>0) ? -offsetx : offsetx);
          }
          q = p->line[i].point[j-1].y+oy - k*(p->line[i].point[j-1].x+ox);
        }

        /* offset line points computation */
        if(first==1) { /* first point */
          first = 0;
          x = p->line[i].point[j-1].x+ox;
          y = p->line[i].point[j-1].y+oy;
        } else { /* middle points */
          if((dx*dx+dy*dy)>limit){ /* if the points are too close */
            if(dx0==0) { /* checking verticals */
              if(dx==0) continue;
              x = x0;
              y = k*x + q;
            } else {
              if(dx==0) {
                x = p->line[i].point[j-1].x+ox;
                y = k0*x + q0;
              } else {
                if(k==k0) continue; /* checking equal direction */
                x = (q-q0)/(k0-k);
                y = k*x+q;
              }
            }
          }else{/* need to be refined */
            x = p->line[i].point[j-1].x+ox;
            y = p->line[i].point[j-1].y+oy;
          }
          gdImageLine(img, (int)x0, (int)y0, (int)x, (int)y, color);
        }
        dx0 = dx; dy0 = dy; x0 = x, y0 = y; k0 = k; q0=q;
      }
      /* last point */
      if(first==0)gdImageLine(img, (int)x0, (int)y0, (int)(p->line[i].point[p->line[i].numpoints-1].x+ox), (int)(p->line[i].point[p->line[i].numpoints-1].y+oy), color);
    }
  } else { /* normal offset (eg. drop shadow) */
    for (i = 0; i < p->numlines; i++)
      for(j=1; j<p->line[i].numpoints; j++)
        gdImageLine(img, (int)p->line[i].point[j-1].x+offsetx, (int)p->line[i].point[j-1].y+offsety, (int)p->line[i].point[j].x+offsetx, (int)p->line[i].point[j].y+offsety, color);
  }
}

typedef enum {CLIP_LEFT, CLIP_MIDDLE, CLIP_RIGHT} CLIP_STATE;

#define CLIP_CHECK(min, a, max) ((a) < (min) ? CLIP_LEFT : ((a) > (max) ? CLIP_RIGHT : CLIP_MIDDLE));
#define ROUND(a)       ((a) + 0.5)
#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))
#define EDGE_CHECK(x0, x, x1) ((x) < MS_MIN((x0), (x1)) ? CLIP_LEFT : ((x) > MS_MAX((x0), (x1)) ? CLIP_RIGHT : CLIP_MIDDLE))

//------------------------------------------------------------------------------------------------------------
class CMapServerLine
{
public:
  CMapServerLine(shapeObj *p, int iLineIndex) : m_pShape(p), m_lineIndex(iLineIndex), m_index(0)
  {
  }

  void rewind(unsigned)
  {
    m_index = 0;
  }

  unsigned vertex(double *x, double *y)
  {
    if(m_index >= m_pShape->line[m_lineIndex].numpoints)
      return agg::path_cmd_stop;

    *x = m_pShape->line[m_lineIndex].point[m_index].x + 0.5;
    *y = m_pShape->line[m_lineIndex].point[m_index].y + 0.5;
    m_index++;
    
    if(m_index == 0)
      return agg::path_cmd_move_to;

    return agg::path_cmd_line_to;
  }
  
private:
  int      m_index;
  int      m_lineIndex;
  shapeObj  *m_pShape;
};

//------------------------------------------------------------------------------------------------------------
class CMapServerPolygon
{
public:
  CMapServerPolygon(shapeObj *p, int iLineIndex) : m_pShape(p), m_lineIndex(iLineIndex), m_index(0)
  {
  }
  
  void rewind(unsigned)
  {
    m_index = 0;
  }
  
  unsigned vertex(double *x, double *y)
  {
    if(m_index >= m_pShape->line[m_lineIndex].numpoints)
      return agg::path_cmd_stop;
    
    *x = m_pShape->line[m_lineIndex].point[m_index].x + 0.5;
    *y = m_pShape->line[m_lineIndex].point[m_index].y + 0.5;
    m_index++;
    
    if(m_index == 0)
      return agg::path_cmd_move_to;

    return agg::path_cmd_line_to;
  }
  
private:
  int      m_index;
  int      m_lineIndex;
  shapeObj  *m_pShape;
};

static void imagePolyline(imageObj *image, shapeObj *p, colorObj *color, int width, int offsetx, int offsety)
{
    int i, j;

    gdImagePtr img = image->img.gd;
  
    mapserv_row_ptr_cache<int> *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
    if(pRowCache == NULL) {
      fprintf(pLogFile, "imagePolyline pRowCache == NULL, extra is %08x\n", image->imageextra); 
      return;
    }
  
    pixelFormat thePixelFormat(*pRowCache);
    
    agg::renderer_base< pixelFormat > rbase(thePixelFormat);
            
    agg::rasterizer_scanline_aa<> ras;
    agg::path_storage ps;
    agg::conv_stroke<agg::path_storage> pg(ps);
    agg::scanline_p8 sl;

    //ren_base.reset_clipping(true);

    pg.width(width);
    //pg.generator().line_cap(agg::butt_cap);
    pg.generator().line_cap(agg::round_cap);
    //pg.generator().line_cap(agg::square_cap);
    //pg.generator().line_join(agg::miter_join);
    //pg.generator().line_join(agg::miter_join_revert);
    //pg.generator().line_join(agg::round_join);
    //pg.generator().line_join(agg::bevel_join);
    //pg.generator().line_join(agg::miter_join_round);

    for (i = 0; i < p->numlines; i++) {
      ps.move_to(p->line[i].point[0].x, p->line[i].point[0].y);
        
      for(j=1; j<p->line[i].numpoints; j++) {
        ps.line_to(p->line[i].point[j].x, p->line[i].point[j].y);
      }
    }

    ras.clip_box(0,0,image->width, image->height);
    ras.add_path(pg);
    agg::render_scanlines_aa_solid(ras, sl, rbase, 
                                   agg::rgba(((double) color->red) / 255.0, 
                                             ((double) color->green) / 255.0, 
                                             ((double) color->blue) / 255.0));
}


//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
static void imagePolyline2(imageObj *image, shapeObj *p, colorObj *color, int width, int offsetx, int offsety)
{
  int i, j;
  gdImagePtr img = image->img.gd;
  
  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
  if(pRowCache == NULL) {
    fprintf(pLogFile, "imagePolyline pRowCache == NULL, extra is %08x\n", image->imageextra); 
    return;
  }
  
  // if(image->format->imagemode == MS_IMAGEMODE_RGBA)
  {
    
    pixelFormat thePixelFormat(*pRowCache);
    agg::renderer_base< pixelFormat > ren_base(thePixelFormat);
    agg::renderer_primitives< agg::renderer_base< pixelFormat > > ren_prim(ren_base);
    agg::rasterizer_outline< agg::renderer_primitives< agg::renderer_base< pixelFormat > > > ras_al(ren_prim);
    
    agg::renderer_scanline_aa_solid< agg::renderer_base< pixelFormat > > ren_aa(ren_base);
    agg::scanline_p8 sl;
    agg::rasterizer_scanline_aa<> ras_aa;
    
    ren_base.reset_clipping(true);

#if 0    
    ren_prim.line_color(agg::rgba(((double) color->red) / 255.0,
                                  ((double) color->green) / 255.0,
                                  ((double) color->blue) / 255.0, 0.0));

    ren_prim.fill_color(agg::rgba(((double) color->red) / 255.0,
                                  ((double) color->green) / 255.0,
                                  ((double) color->blue) / 255.0, 0.0));
#endif
    
    for (i = 0; i < p->numlines; i++) {
      if(0) {
        for(j=1; j<p->line[i].numpoints; j++) {
          ren_prim.line((int)p->line[i].point[j-1].x << 8, 
                        (int)p->line[i].point[j-1].y << 8, 
                        (int)p->line[i].point[j].x << 8, 
                        (int)p->line[i].point[j].y << 8,
                        true);
        }        
      } else {
        CMapServerLine aLine(p,i);
        agg::conv_stroke<CMapServerLine> stroke(aLine);
        
        stroke.width(((float) width) / 2.0);
        stroke.line_cap(agg::round_cap);
        
        ren_aa.color( agg::rgba(((double) color->red) / 255.0, 
                                ((double) color->green) / 255.0, 
                                ((double) color->blue) / 255.0));
                    
        ras_aa.add_path(stroke);
        
        agg::render_scanlines(ras_aa, sl, ren_aa);
      }
    }    

    return;
  }

#if 0
  if(offsetx != 0 || offsety != 0) 
    imageOffsetPolyline(img, p, color, offsetx, offsety);
  else {
    for (i = 0; i < p->numlines; i++) {
      // line_color(color);
      for(j=1; j<p->line[i].numpoints; j++)
        // line(x1, y1, y1, y2, islast);
        gdImageLine(img, (int)p->line[i].point[j-1].x, 
                         (int)p->line[i].point[j-1].y, 
                         (int)p->line[i].point[j].x, 
                         (int)p->line[i].point[j].y, 
                         color);
    }
  }  
#endif
}
    
//------------------------------------------------------------------------------------------------------------
static void imageFilledPolygon2(imageObj *image, shapeObj *p, colorObj *color, int offsetx, int offsety)
{    
  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
  if(pRowCache == NULL) {
    fprintf(pLogFile, "imageFilledPolygon pRowCache == NULL, extra is %08x\n", image->imageextra); 
    return;
  }

  pixelFormat thePixelFormat(*pRowCache);
  agg::renderer_base< pixelFormat > ren_base(thePixelFormat);
    
  agg::renderer_scanline_aa_solid< agg::renderer_base< pixelFormat > > ren_aa(ren_base);
  agg::scanline_p8 sl;
  agg::rasterizer_scanline_aa<> ras_aa;
    
  ren_base.reset_clipping(true);
    
  {
    for (int i = 0; i < p->numlines; i++) {
      if(0) {
        for(int j=1; j<p->line[i].numpoints; j++) {
          // ren_prim.line((int)p->line[i].point[j-1].x << 8, 
          //               (int)p->line[i].point[j-1].y << 8, 
          //               (int)p->line[i].point[j].x << 8, 
          //               (int)p->line[i].point[j].y << 8,
          //               true);
        }        
      } else {
        ren_aa.color( agg::rgba(((double) color->red) / 255.0, 
                                ((double) color->green) / 255.0, 
                                ((double) color->blue) / 255.0, 0.01));

        agg::path_storage  path;
        
        path.move_to(p->line[i].point[0].x + 0.5, p->line[i].point[0].y + 0.5);
                    
        for(int j=1; j<p->line[i].numpoints; j++)
          path.line_to(p->line[i].point[j].x + 0.5, p->line[i].point[j].y + 0.5);
          
        path.close_polygon();
        ras_aa.add_path(path);
        
        agg::render_scanlines(ras_aa, sl, ren_aa);
      }
    }  
  }
}

static void imageFilledPolygon(imageObj *image, shapeObj *p, colorObj *color, int offsetx, int offsety)
{    
  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
  if(pRowCache == NULL) {
    fprintf(pLogFile, "imageFilledPolygon pRowCache == NULL, extra is %08x\n", image->imageextra); 
    return;
  }

  pixelFormat thePixelFormat(*pRowCache);
  agg::renderer_base< pixelFormat > ren_base(thePixelFormat);
    
  agg::renderer_scanline_aa_solid< agg::renderer_base< pixelFormat > > ren_aa(ren_base);
  agg::scanline_p8 sl;
  agg::rasterizer_scanline_aa<> ras_aa;
    
  ren_base.reset_clipping(true);
    
  ren_aa.color( agg::rgba(((double) color->red) / 255.0, 
                          ((double) color->green) / 255.0, 
                          ((double) color->blue) / 255.0, 1));

  agg::path_storage path;
  for(int i = 0; i < p->numlines; i++) {
    path.move_to(p->line[i].point[0].x, p->line[i].point[0].y);

    for(int j=1; j<p->line[i].numpoints; j++)
      path.line_to(p->line[i].point[j].x, p->line[i].point[j].y);
              
    path.close_polygon();
  }
  ras_aa.add_path(path);

  agg::render_scanlines(ras_aa, sl, ren_aa);
}

//------------------------------------------------------------------------------------------------------------
/* ---------------------------------------------------------------------------*/
/*       Stroke an ellipse with a line symbol of the specified size and color */
/* ---------------------------------------------------------------------------*/
void msCircleDrawLineSymbolAGG(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, styleObj *style, double scalefactor)
{
  if(pLogFile) fprintf(pLogFile, "msCircleDrawLineSymbolAGG entry\n");    
  msCircleDrawLineSymbolGD(symbolset, img, p, r, style, scalefactor);
}

//------------------------------------------------------------------------------------------------------------
/* ------------------------------------------------------------------------------- */
/*       Fill a circle with a shade symbol of the specified size and color         */
/* ------------------------------------------------------------------------------- */
void msCircleDrawShadeSymbolAGG(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, styleObj *style, double scalefactor)
{
  if(pLogFile) fprintf(pLogFile, "msCircleDrawShadeSymbolAGG entry\n");
  msCircleDrawShadeSymbolGD(symbolset, img, p, r, style, scalefactor);
}

//------------------------------------------------------------------------------------------------------------
void msDrawMarkerSymbolAGGTrueType(symbolObj *symbol, double d, double size, double angle, char bRotated, styleObj *style,
    int offset_x, int offset_y, int ox, int oy, gdImagePtr img, pointObj *p, symbolSetObj *symbolset, double angle_radians)
{
  char *error=NULL;
  char *font=NULL;
  int fc, oc, x, y;
  rectObj rect;
  
  int bbox[8];

  fc = style->color.pen;
  oc = style->outlinecolor.pen;

#ifdef USE_GD_FT
  font = msLookupHashTable(&(symbolset->fontset->fonts), symbol->font);
  if(!font) return;

  if(msGetCharacterSize(symbol->character, (int)size, font, &rect) != MS_SUCCESS) return;

  x = (int)(p->x + ox - (rect.maxx - rect.minx)/2 - rect.minx);
  y = (int)(p->y + oy - rect.maxy + (rect.maxy - rect.miny)/2);

  if(oc >= 0) {
    error = gdImageStringFT(img, bbox, ((symbol->antialias || style->antialias)?(oc):-(oc)), font, size, angle_radians, x, y-1, symbol->character);
    if(error) {
      msSetError(MS_TTFERR, error, "msDrawMarkerSymbolGD()");
      return;
    }

    gdImageStringFT(img, bbox, ((symbol->antialias || style->antialias)?(oc):-(oc)), font, size, angle_radians, x, y+1, symbol->character);
    gdImageStringFT(img, bbox, ((symbol->antialias || style->antialias)?(oc):-(oc)), font, size, angle_radians, x+1, y, symbol->character);
    gdImageStringFT(img, bbox, ((symbol->antialias || style->antialias)?(oc):-(oc)), font, size, angle_radians, x-1, y, symbol->character);
    gdImageStringFT(img, bbox, ((symbol->antialias || style->antialias)?(oc):-(oc)), font, size, angle_radians, x+1, y+1, symbol->character);
    gdImageStringFT(img, bbox, ((symbol->antialias || style->antialias)?(oc):-(oc)), font, size, angle_radians, x+1, y-1, symbol->character);
    gdImageStringFT(img, bbox, ((symbol->antialias || style->antialias)?(oc):-(oc)), font, size, angle_radians, x-1, y+1, symbol->character);
    gdImageStringFT(img, bbox, ((symbol->antialias || style->antialias)?(oc):-(oc)), font, size, angle_radians, x-1, y-1, symbol->character);
  }

  gdImageStringFT(img, bbox, ((symbol->antialias || style->antialias)?(fc):-(fc)), font, size, angle_radians, x, y, symbol->character);
#endif
}

//------------------------------------------------------------------------------------------------------------
void msDrawMarkerSymbolAGGPixmap(symbolObj *symbol, double d, double size, double angle, char bRotated, styleObj *style,
    int offset_x, int offset_y, int ox, int oy, gdImagePtr img, pointObj *p)
{
  if (symbol->sizey)
    d = size/symbol->sizey; /* compute the scaling factor (d) on the unrotated symbol */
  else
    d = 1;

  if (angle != 0.0 && angle != 360.0) {
    bRotated = MS_TRUE;
    symbol = msRotateSymbol(symbol, style->angle);
  }

  if(d == 1) { /* don't scale */
    offset_x = MS_NINT(p->x - .5*symbol->img->sx + ox);
    offset_y = MS_NINT(p->y - .5*symbol->img->sy + oy);
    gdImageCopy(img, symbol->img, offset_x, offset_y, 0, 0, symbol->img->sx, symbol->img->sy);
  } else {
    offset_x = MS_NINT(p->x - .5*symbol->img->sx*d + ox);
    offset_y = MS_NINT(p->y - .5*symbol->img->sy*d + oy);
    gdImageCopyResampled(img, symbol->img, offset_x, offset_y, 0, 0, (int)(symbol->img->sx*d), (int)(symbol->img->sy*d), symbol->img->sx, symbol->img->sy);
  }

  if(bRotated) {
    msFreeSymbol(symbol); /* clean up */
    msFree(symbol);
  }
}

//------------------------------------------------------------------------------------------------------------
void msDrawMarkerSymbolAGGEllipse(symbolObj *symbol, double d, double size, double angle, char bRotated, styleObj *style,
    int offset_x, int offset_y, int ox, int oy, gdImagePtr img, pointObj *p)
{
  int fc, oc, w, h, x, y;

  fc = style->color.pen;
  oc = style->outlinecolor.pen;
 
  w = MS_NINT((size*symbol->sizex/symbol->sizey)); /* ellipse size */
  h = MS_NINT(size);

  x = MS_NINT(p->x + ox); /* GD will center the ellipse on x,y */
  y = MS_NINT(p->y + oy);

  /* check for trivial cases - 1x1 and 2x2, GD does not do these well */
  if(w==1 && h==1) {
    gdImageSetPixel(img, x, y, fc);
    return;
  }

  if(w==2 && h==2) {
    gdImageSetPixel(img, x, y, fc);
    gdImageSetPixel(img, x, y+1, fc);
    gdImageSetPixel(img, x+1, y, fc);
    gdImageSetPixel(img, x+1, y+1, fc);
    return;
  }

  /* for a circle interpret the style angle as the size of the arc (for drawing pies) */
  if(w == h && style->angle != 360) {
    if(symbol->filled) {
      if(fc >= 0) gdImageFilledArc(img, x, y, w, h, 0, style->angle, fc, gdEdged|gdPie);
      if(oc >= 0) gdImageFilledArc(img, x, y, w, h, 0, style->angle, oc, gdEdged|gdNoFill);
    } else if(!symbol->filled) {
      if(fc < 0) fc = oc; /* try the outline color */
        if(fc < 0) return;
        gdImageFilledArc(img, x, y, w, h, 0, style->angle, fc, gdEdged|gdNoFill);
    }
  } else {
    if(symbol->filled) {
      if(fc >= 0) gdImageFilledEllipse(img, x, y, w, h, fc);        
      if(oc >= 0) gdImageArc(img, x, y, w, h, 0, 360, oc);
    } else if(!symbol->filled) {
      if(fc < 0) fc = oc; /* try the outline color */
      if(fc < 0) return;
      gdImageArc(img, x, y, w, h, 0, 360, fc);
    }
  }
}

//------------------------------------------------------------------------------------------------------------
void msDrawMarkerSymbolAGGVector(symbolObj *symbol, double d, double size, double angle, char bRotated, styleObj *style,
    int offset_x, int offset_y, int ox, int oy, gdImagePtr img, pointObj *p, imageObj *image )
{
  int fc, oc, j, k, width;
  gdPoint oldpnt,newpnt;
  gdPoint mPoints[MS_MAXVECTORPOINTS];

  shapeObj theShape;
    
  theShape.numlines = 1;
  theShape.type = MS_SHAPE_LINE;
  theShape.line = (lineObj *) malloc(sizeof(lineObj) * MS_MAXVECTORPOINTS);

  fc = style->color.pen;
  oc = style->outlinecolor.pen;

  d = size/symbol->sizey; /* compute the scaling factor (d) on the unrotated symbol */

  if(angle != 0.0 && angle != 360.0) {      
    bRotated = MS_TRUE;
    symbol = msRotateSymbol(symbol, style->angle);
  }

  /* 
	** We avoid MS_NINT in this context because the potentially variable
  ** handling of 0.5 rounding is often a problem for symbols which are
  ** often an odd size (ie. 7 pixels) and so if "p" is integral the 
  ** value is always on a 0.5 boundary - bug 1716 
  */
  offset_x = MS_NINT_GENERIC(p->x - d*.5*symbol->sizex + ox);
  offset_y = MS_NINT_GENERIC(p->y - d*.5*symbol->sizey + oy);
    
  if(symbol->filled) {      

    k = 0; /* point counter */
    for(j=0;j < symbol->numpoints;j++) {
      if((symbol->points[j].x < 0) && (symbol->points[j].x < 0)) { /* new polygon (PENUP) */
        theShape.line->numpoints = k;

        if(k>2) {
          if(fc >= 0)
            imageFilledPolygon(image, &theShape, &(style->color), offset_x, offset_y);
          if(oc >= 0)
            imagePolyline(image, &theShape, &(style->outlinecolor), width, offset_x, offset_y);
        }

          k = 0; /* reset point counter */
      } else {
        theShape.line->point[k].x = MS_NINT(d*symbol->points[j].x + offset_x);
        theShape.line->point[k].y = MS_NINT(d*symbol->points[j].y + offset_y);
        k++;
      }
    }
    
    theShape.line->numpoints = k;

    if(fc >= 0)
      imageFilledPolygon(image, &theShape, &(style->color), offset_x, offset_y);
    if(oc >= 0)
      imagePolyline(image, &theShape, &(style->outlinecolor), width, offset_x, offset_y);

  } else  { /* NOT filled */     

    if(fc < 0) fc = oc; /* try the outline color (reference maps sometimes do this when combining a box and a custom vector marker */
    if(fc < 0) return;
      
    mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
    if(pRowCache == NULL) {
      fprintf(pLogFile, "imagePolyline pRowCache == NULL, extra is %08x\n", image->imageextra); 
      free(theShape.line);
      return;
    }
    
    pixelFormat thePixelFormat(*pRowCache);
    agg::renderer_base< pixelFormat > ren_base(thePixelFormat);
    agg::renderer_primitives< agg::renderer_base< pixelFormat > > ren_prim(ren_base);
    agg::rasterizer_outline< agg::renderer_primitives< agg::renderer_base< pixelFormat > > > ras_al(ren_prim);
    
    ren_base.reset_clipping(true);

    ren_prim.line_color(agg::rgba(((double) style->color.red) / 255.0, 
                                  ((double) style->color.green) / 255.0, 
                                   ((double) style->color.blue) / 255.0, 0.0));

    ren_prim.fill_color(agg::rgba(((double) style->outlinecolor.red) / 255.0, 
                                  ((double) style->outlinecolor.green) / 255.0, 
                                  ((double) style->outlinecolor.blue) / 255.0, 0.0));

    oldpnt.x = MS_NINT(d*symbol->points[0].x + offset_x); /* convert first point in marker */
    oldpnt.y = MS_NINT(d*symbol->points[0].y + offset_y);
      
    for(j=1;j < symbol->numpoints;j++) { /* step through the marker */
      if((symbol->points[j].x < 0) && (symbol->points[j].x < 0)) {
        oldpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
        oldpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
      } else {
        if((symbol->points[j-1].x < 0) && (symbol->points[j-1].y < 0)) { /* Last point was PENUP, now a new beginning */
          oldpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
          oldpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
        } else {
          newpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
          newpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
    
          ren_prim.line((int) oldpnt.x << 8, (int)oldpnt.y << 8, 
                        (int)newpnt.x << 8, (int)newpnt.y << 8,
                        true);              

          oldpnt = newpnt;
        }
      }
    } /* end for loop */   
  } /* end if-then-else */

  if(bRotated) {
    msFreeSymbol(symbol); /* clean up */
    msFree(symbol);
  }
  
  free(theShape.line);
}

/* ------------------------------------------------------------------------------- */
/*       Draw a single marker symbol of the specified size and color               */
/* ------------------------------------------------------------------------------- */
void msDrawMarkerSymbolAGG(symbolSetObj *symbolset, imageObj *image, pointObj *p, styleObj *style, double scalefactor)
{
  if(pLogFile) fprintf(pLogFile, "msDrawMarkerSymbolAGG entry\n");
    
  char bRotated=MS_FALSE;
  symbolObj *symbol;
  int offset_x, offset_y;

  double size, d, angle, angle_radians;
  int width;

  int ox, oy, bc, fc, oc;

  if(!p) return;

  gdImagePtr img    = image->img.gd;
  
  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->outlinecolor));

  symbol = &(symbolset->symbol[style->symbol]);
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  oc = style->outlinecolor.pen;
  ox = style->offsetx; /* TODO: add scaling? */
  oy = style->offsety;

  if(style->size == -1) {
    size = msSymbolGetDefaultSize(&(symbolset->symbol[style->symbol]));
    size = MS_NINT(size*scalefactor);
  } else
    size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  width = MS_NINT(style->width*scalefactor);
  width = MS_MAX(width, style->minwidth);
  width = MS_MIN(width, style->maxwidth);

  angle = (style->angle) ? style->angle : 0.0;
  angle_radians = angle*MS_DEG_TO_RAD;

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; /* no such symbol, 0 is OK */
  if(fc<0 && oc<0 && symbol->type != MS_SYMBOL_PIXMAP) return; /* nothing to do (color not required for a pixmap symbol) */
  if(size < 1) return; /* size too small */

  if(style->symbol == 0 && fc >= 0) { /* simply draw a single pixel of the specified color */
    gdImageSetPixel(img, (int)(p->x + ox), (int)(p->y + oy), fc);
    return;
  }  

  switch(symbol->type) {
    case(MS_SYMBOL_TRUETYPE): /* TODO: Need to leverage the image cache! */
      msDrawMarkerSymbolAGGTrueType(symbol, d, size, angle, bRotated, style, offset_x, offset_y, ox, oy, img, p, symbolset, angle_radians);
    break;
    
    case(MS_SYMBOL_PIXMAP):
      msDrawMarkerSymbolAGGPixmap(symbol, d, size, angle, bRotated, style, offset_x, offset_y, ox, oy, img, p);
    break;
    
    case(MS_SYMBOL_ELLIPSE):
      msDrawMarkerSymbolAGGEllipse(symbol, d, size, angle, bRotated, style, offset_x, offset_y, ox, oy, img, p);
    break;
    
    case(MS_SYMBOL_VECTOR): /* TODO: Need to leverage the image cache! */
      msDrawMarkerSymbolAGGVector(symbol, d, size, angle, bRotated, style, offset_x, offset_y, ox, oy, img, p, image);
    break;

  default:
    break;
    
  }
          
  msDrawMarkerSymbolGD(symbolset, img, p, style, scalefactor);
}

//------------------------------------------------------------------------------------------------------------
/* ------------------------------------------------------------------------------- */
/*       Draw a line symbol of the specified size and color                        */
/* ------------------------------------------------------------------------------- */
void msDrawLineSymbolAGG(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor)
{
  gdImagePtr img  = image->img.gd;
    
  int width;
  int nwidth, size;
  symbolObj *symbol;

  symbol = &(symbolset->symbol[style->symbol]);
  if(style->size == -1)
    size = msSymbolGetDefaultSize(&(symbolset->symbol[style->symbol]));
  else
    size = style->size;

  size = MS_NINT(size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  width = MS_NINT(style->width*scalefactor);
  width = MS_MAX(width, style->minwidth);
  width = MS_MIN(width, style->maxwidth);
  
  if(style->symbol == 0) 
    nwidth = width;
  else
    nwidth = size;

  if(pLogFile) fprintf(pLogFile, "msDrawLineSymbolAGG entry\n"); 
  
  if(p->numlines > 0)
    imagePolyline(image, p, &style->color, nwidth, 0, 0);
  
  return;
#if 0  
  if(style->symbol == 0) {
    if(gdImageTrueColor(img) && width > 1 && style->antialias == MS_TRUE) { /* use a fuzzy brush */      
      if((brush = searchImageCache(symbolset->imagecache, style, width)) == NULL) {
        brush = createFuzzyBrush(width, gdImageRed(img, fc), gdImageGreen(img, fc), gdImageBlue(img, fc)); 
        symbolset->imagecache = addImageCache(symbolset->imagecache, &symbolset->imagecachesize, style, width, brush);
      }
      
      gdImageSetBrush(img, brush);
      imagePolyline(img, p, gdBrushed, ox, oy);
    } else {
      gdImageSetThickness(img, width);
          
      if(style->antialias == MS_TRUE) { 
        gdImageSetAntiAliased(img, fc);
        imagePolyline(image, p, gdAntiAliased, ox, oy);
        gdImageSetAntiAliased(img, -1);
      } else
        imagePolyline(image, p, fc, ox, oy);
          
      gdImageSetThickness(img, 1); /* reset */
    }

    return; /* done with easiest case */
  }

  msDrawLineSymbolGD(symbolset, img, p, style, scalefactor);
#endif    
}

//------------------------------------------------------------------------------------------------------------
/* ------------------------------------------------------------------------------- */
/*       Draw a shade symbol of the specified size and color                       */
/* ------------------------------------------------------------------------------- */
void msDrawShadeSymbolAGG(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor)
{
  gdImagePtr img  = image->img.gd;
    
  char bRotated=MS_FALSE;
  symbolObj *symbol;
  int i, k;
  gdPoint oldpnt, newpnt;
  gdPoint sPoints[MS_MAXVECTORPOINTS];
  gdImagePtr tile=NULL;
  int x, y, ox, oy;
  int tile_bc=-1, tile_fc=-1; /* colors (background and foreground) */
  int fc, bc, oc;
  double size, d, angle, angle_radians;
  int width;
  
  int bbox[8];
  rectObj rect;
  char *font=NULL;

  if(!p) return;
  if(p->numlines <= 0) return;

  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->outlinecolor));

  symbol = &(symbolset->symbol[style->symbol]);
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  oc = style->outlinecolor.pen;

  if(style->size == -1) {
    size = msSymbolGetDefaultSize(&(symbolset->symbol[style->symbol]));
    size = MS_NINT(size*scalefactor);
  } else
    size = MS_NINT(style->size*scalefactor);
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  width = MS_NINT(style->width*scalefactor);
  width = MS_MAX(width, style->minwidth);
  width = MS_MIN(width, style->maxwidth);

  angle = (style->angle) ? style->angle : 0.0;
  angle_radians = angle*MS_DEG_TO_RAD;

  ox = MS_NINT(style->offsetx*scalefactor); /* should we scale the offsets? */
  oy = MS_NINT(style->offsety*scalefactor);

  if(fc==-1 && oc!=-1 && symbol->type!=MS_SYMBOL_PIXMAP) { /* use msDrawLineSymbolGD() instead (POLYLINE) */
    msDrawLineSymbolGD(symbolset, img, p, style, scalefactor);
    return;
  }

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; /* no such symbol, 0 is OK   */
  if(fc < 0 && symbol->type!=MS_SYMBOL_PIXMAP) return; /* nothing to do (colors are not required with PIXMAP symbols) */
  if(size < 1) return; /* size too small */
      
  if(style->symbol == 0) { /* simply draw a single pixel of the specified color */    
    if(style->antialias==MS_TRUE) {
      colorObj thefc = { (fc & 0xff000000 >> 24),(fc & 0x00ff0000 >> 16),(fc & 0x0000ff00 >> 8),fc & 0x000000ff };
      imageFilledPolygon(image, p, &thefc, ox, oy); /* fill is NOT anti-aliased, the outline IS */
      if(oc>-1)
        gdImageSetAntiAliased(img, oc);
      else
        gdImageSetAntiAliased(img, fc);

      // imagePolyline(image, p, gdAntiAliased, ox, oy);
      colorObj thefc2 = { (fc & 0xff000000 >> 24),(fc & 0x00ff0000 >> 16),(fc & 0x0000ff00 >> 8),fc & 0x000000ff };
      imagePolyline(image, p, &thefc2, width, ox, oy);
    } else {
      imageFilledPolygon(image, p, &(style->color), ox, oy); /* fill is NOT anti-aliased, the outline IS */
      if(oc>-1) {
        imagePolyline(image, p, &(style->outlinecolor), width, ox, oy);
      }
    }

    return; /* done with easiest case */
  }
    
  msDrawShadeSymbolGD(symbolset, img, p, style, scalefactor);
}

//------------------------------------------------------------------------------------------------------------
/*
** Simply draws a label based on the label point and the supplied label object.
*/
int msDrawTextAGG(gdImagePtr img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor)
{
  return msDrawTextGD(img, labelPnt, string, label, fontset, scalefactor);
}

//------------------------------------------------------------------------------------------------------------
/*
 * Draw a label curved along a line
 */
int msDrawTextLineAGG(gdImagePtr img, char *string, labelObj *label, labelPathObj *labelpath, fontSetObj *fontset, double scalefactor)
{
  return msDrawTextLineGD(img, string, label, labelpath, fontset, scalefactor);
}

//------------------------------------------------------------------------------------------------------------
/* To DO: fix returned values to be MS_SUCCESS/MS_FAILURE */
int msDrawLabelCacheAGG(gdImagePtr img, mapObj *map)
{
  return msDrawLabelCacheGD(img, map);
}

//------------------------------------------------------------------------------------------------------------
/* ===========================================================================
   msSaveImageGD
   
   Save an image to a file named filename, if filename is NULL it goes to
   stdout.  This function wraps msSaveImageGDCtx.  --SG
   ======================================================================== */
int msSaveImageAGG(gdImagePtr img, char *filename, outputFormatObj *format)
{
  char *pFormatBuffer;
  char cGDFormat[128];
  int   iReturn = 0;
  
  pFormatBuffer = format->driver;
  
  strcpy(cGDFormat, "gd/");
  strcat(cGDFormat, &(format->driver[4]));
  
  format->driver = &cGDFormat[0];
  
  iReturn = msSaveImageGD(img, filename, format);
  
  format->driver = pFormatBuffer;
  
  return iReturn;
}

//------------------------------------------------------------------------------------------------------------
/* ===========================================================================
   msSaveImageGDCtx

   Save image data through gdIOCtx only.  All mapio conditional compilation
   definitions have been moved up to msSaveImageGD (bug 1047).
   ======================================================================== */
int msSaveImageAGGCtx(gdImagePtr img, gdIOCtx *ctx, outputFormatObj *format)
{
  char *pFormatBuffer;
  char cGDFormat[128];
  int   iReturn = 0;
  
  pFormatBuffer = format->driver;
  
  strcpy(cGDFormat, "gd/");
  strcat(cGDFormat, &(format->driver[4]));
  
  format->driver = &cGDFormat[0];
  
  iReturn = msSaveImageGDCtx(img, ctx, format);
  
  format->driver = pFormatBuffer;
  
  return iReturn;
}

//------------------------------------------------------------------------------------------------------------
/* ===========================================================================
   msSaveImageBufferGD

   Save image data to a unsigned char * buffer.  In the future we should try
   to merge this with msSaveImageStreamGD function.

   The returned buffer is owned by the caller. It should be freed with gdFree()
   ======================================================================== */

unsigned char *msSaveImageBufferAGG(gdImagePtr img, int *size_ptr,
                                   outputFormatObj *format)
{
  return msSaveImageBufferGD(img, size_ptr, format);
}

//------------------------------------------------------------------------------------------------------------
/**
 * Free gdImagePtr
 */
void msFreeImageAGG(gdImagePtr img)
{
  msFreeImageGD(img);
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

#endif

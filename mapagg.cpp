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
 *****************************************************************************/

#ifdef USE_AGG

#include "mapserver.h"
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
#include "agg_conv_dash.h"
#include "agg_conv_curve.h"

#include "agg_scanline_p.h"
#include "agg_scanline_u.h"
#include "agg_path_storage.h"

#include "agg_renderer_base.h"
#include "agg_renderer_primitives.h"
#include "agg_renderer_scanline.h"
#include "agg_renderer_outline_aa.h"

#include "agg_pixfmt_rgb.h"
#include "agg_pixfmt_rgba.h"

#include "agg_arc.h"
#include "agg_dda_line.h"
#include "agg_ellipse_bresenham.h"
#include "agg_ellipse.h"

#include "agg_span_allocator.h"
#include "agg_image_accessors.h"
#include "agg_span_pattern_rgba.h"
#include "agg_span_interpolator_linear.h"
#include "agg_span_image_filter_rgba.h"
#include "agg_trans_affine.h"
#include "agg_scanline_boolean_algebra.h"
#include "agg_scanline_storage_aa.h"
#include "math.h"
#include "mapagg.h"

#ifdef CPL_MSB
typedef agg::pixfmt_alpha_blend_rgba<agg::blender_argb32,mapserv_row_ptr_cache<int>,int> pixelFormat;
#else
typedef agg::pixfmt_alpha_blend_rgba<agg::blender_bgra32_plain,mapserv_row_ptr_cache<int>,int> pixelFormat;
/* typedef agg::pixfmt_alpha_blend_rgba<agg::blender_rgba32,mapserv_row_ptr_cache<int>,int> pixelFormat; */
#endif


MS_CVSID("$Id$")

int msImageSetPenAGG(gdImagePtr img, colorObj *color) 
{
  return msImageSetPenGD(img, color);
}

/*
** Take a pass through the mapObj and pre-allocate colors for layers that are ON or DEFAULT. This replicates the pre-4.0 behavior of
** MapServer and should be used only with paletted images.
**/
void msPreAllocateColorsAGG(imageObj *image, mapObj *map) {
  msPreAllocateColorsGD(image, map);
}


/*
** Utility function to create a GD image. Returns
** a pointer to an imageObj structure.
*/  
imageObj *msImageCreateAGG(int width, int height, outputFormatObj *format, char *imagepath, char *imageurl) 
{
  imageObj *pNewImage = NULL;
  
  pNewImage  = msImageCreateGD(width, height, format, imagepath, imageurl);
  
  if(!pNewImage)
    return pNewImage;
    
  mapserv_row_ptr_cache<int>  *pRowCache = new mapserv_row_ptr_cache<int>(pNewImage->img.gd);
  
  if(! pRowCache)
    return NULL;
    
  pNewImage->imageextra  = (void *) pRowCache;

  return pNewImage;
}

/*
** Utility function to initialize the color of an image.  The background
** color is passed, but the outputFormatObj is consulted to see if the
** transparency should be set (for RGBA images).   Note this function only
** affects TrueColor images. 
*/  
void msImageInitAGG(imageObj *image, colorObj *background)
{
  if(image->format->imagemode == MS_IMAGEMODE_PC256) {
    gdImageColorAllocate(image->img.gd, background->red, background->green, background->blue);
    return;
  }

  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
  if(pRowCache == NULL) {
    return;
  }

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

/* msImageLoadAGG now calls msImageLoadAGGCtx to do the work, change
 * made as part of the resolution of bugs 550 and 1047 */
imageObj *msImageLoadAGG(const char *filename) 
{
  return msImageLoadGD(filename); 
}

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
  shapeObj  *m_pShape;
  int      m_lineIndex;
  int      m_index;
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
  shapeObj  *m_pShape;
  int      m_lineIndex;
  int      m_index;
};

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
static void imagePolyline(imageObj *image, shapeObj *p, colorObj *color, double width, 
                            double offsetx, double offsety,
                          int dashstylelength, int *dashstyle)
{
  int i,k;
  
  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
  if(pRowCache == NULL)
    return;
  
  pixelFormat thePixelFormat(*pRowCache);
  agg::renderer_base< pixelFormat > ren_base(thePixelFormat);
  agg::renderer_primitives< agg::renderer_base< pixelFormat > > ren_prim(ren_base);
    
  agg::renderer_scanline_aa_solid< agg::renderer_base< pixelFormat > > ren_aa(ren_base);
  agg::scanline_p8 sl;
  agg::rasterizer_scanline_aa<> ras_aa;
    
  ren_base.reset_clipping(true);

  for (i = 0; i < p->numlines; i++) {
    CMapServerLine aLine(p,i);

    agg::conv_stroke<CMapServerLine> stroke(aLine);

    agg::conv_dash<CMapServerLine> dash(aLine);
    agg::conv_stroke<agg::conv_dash<CMapServerLine> > stroke_dash(dash);     
          
    if (dashstylelength <= 0) {
      stroke.width((width));
      stroke.line_cap(agg::round_cap);
      ras_aa.add_path(stroke);
    } else {
      for (k=0; k<dashstylelength; k=k+2) {
        if (k < dashstylelength-1)
          dash.add_dash(dashstyle[k], dashstyle[k+1]);
      }
      stroke_dash.width(((float) width));
      stroke_dash.line_cap(agg::round_cap);
      ras_aa.add_path(stroke_dash);
    }
        
    ren_aa.color( agg::rgba(((double) color->red) / 255.0, 
                            ((double) color->green) / 255.0, 
                            ((double) color->blue) / 255.0));
        
    agg::render_scanlines(ras_aa, sl, ren_aa);
  }

  return;
}

#ifdef notdef // unused for now

static void imageFilledPolygon2(imageObj *image, shapeObj *p, colorObj *color, int offsetx, int offsety)
{    
  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
  if(pRowCache == NULL)
    return;

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
#endif /* unused */

static void imageFilledPolygon(imageObj *image, shapeObj *p, colorObj *color, double offsetx, double offsety)
{    
  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
  
  if(pRowCache == NULL)
    return;

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
    path.move_to(p->line[i].point[0].x+offsetx, p->line[i].point[0].y+offsety);

    for(int j=1; j<p->line[i].numpoints; j++)
      path.line_to(p->line[i].point[j].x+offsetx, p->line[i].point[j].y+offsety);

    path.close_polygon();
  }
  ras_aa.add_path(path);

  agg::render_scanlines(ras_aa, sl, ren_aa);
}

/* 
** Function to create a custom hatch symbol based on an arbitrary angle. 
*/
static agg::path_storage createHatchAGG(int sx, int sy, double angle, double step)
{
  agg::path_storage path;
  angle = fmod(angle, 360.0);
  if(angle < 0) angle += 360;
  if(angle >= 180) angle -= 180;
  
  /*treat 2 easy cases which would cause divide by 0 in generic case*/
  if(angle==0||angle==180||angle==360) {
      for(double y=step/2;y<sy;y+=step) {
          path.move_to(0,y);
          path.line_to(sx,y);
      }
      return path;
  }
  if(angle==90||angle==270) {
        for(double x=step/2;x<sx;x+=step) {
            path.move_to(x,0);
            path.line_to(x,sy);
        }
        return path;
    }
  
  
  double theta = (90-angle)*MS_PI/180.;
  double ct = cos(theta);
  double st = sin(theta);
  double rmax = sqrt((double)(sx*sx+sy*sy));
  
  /*parametrize each line as r = x.cos(theta) + y.sin(theta)*/
  for(double r=-rmax;r<rmax;r+=step) {
      int inter=0;
      double x,y,pt[4];
      /*test for intersection with each side*/
     
      y=r/st;x=0;
      if(y>=0&&y<=sy) {
          pt[2*inter]=x;pt[2*inter+1]=y;
          inter++;
      }     
      x=sx;y=(r-sx*ct)/st;
      if(y>=0&&y<=sy) {
          pt[2*inter]=x;pt[2*inter+1]=y;
          inter++;
      }
      y=0;x=r/ct;
      if(x>=0&&x<=sx) {
          pt[2*inter]=x;pt[2*inter+1]=y;
          inter++;
      }
      y=sy;x=(r-sy*st)/ct;
      if(x>=0&&x<=sx) {
          pt[2*inter]=x;pt[2*inter+1]=y;
          inter++;
      }
      if(inter==2) {
          path.move_to(pt[0],pt[1]);
          path.line_to(pt[2],pt[3]);
      }
  }
  return path;
}

static agg::path_storage imageVectorSymbolAGG(symbolObj *symbol, double scale) {
    agg::path_storage path,tmppath;
    bool is_new=true;
    for(int i=0;i < symbol->numpoints;i++) {
        if((symbol->points[i].x < 0) && (symbol->points[i].y < 0)) { /* (PENUP) */
            path.concat_path(tmppath);
            tmppath.remove_all();
            is_new=true;
        } else {
            if(is_new) {
                path.move_to(scale*symbol->points[i].x,scale*symbol->points[i].y);
                is_new=false;                         
            }
            else {
                path.line_to(scale*symbol->points[i].x,scale*symbol->points[i].y);
            }
        }
    }
    if(!is_new) {
        path.concat_path(tmppath);
    }          
    return path;
}


static void imageTexturedPolygonAGG(imageObj *image, shapeObj *p, agg::rendering_buffer texture) {
    mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);

    if(pRowCache == NULL)
        return;

    pixelFormat thePixelFormat(*pRowCache);
    agg::renderer_base< pixelFormat > ren_base(thePixelFormat);
    agg::scanline_p8 sl;
    agg::rasterizer_scanline_aa<> ras_aa;
    agg::path_storage path;
    for(int i = 0; i < p->numlines; i++) {
        path.move_to(p->line[i].point[0].x, p->line[i].point[0].y);

        for(int j=1; j<p->line[i].numpoints; j++)
            path.line_to(p->line[i].point[j].x, p->line[i].point[j].y);

        path.close_polygon();
    }
    typedef agg::pixfmt_rgba32 pixfmt;
    typedef agg::rgba8 color_type;
    typedef agg::wrap_mode_repeat wrap_type;
    typedef agg::image_accessor_wrap<pixfmt,wrap_type,wrap_type> img_source_type;
    typedef agg::span_pattern_rgba<img_source_type> span_gen_type;
    agg::span_allocator<color_type> sa;
    pixfmt img_pixf(texture);
    img_source_type img_src(img_pixf);
    span_gen_type sg(img_src, 0, 0);
    ras_aa.add_path(path);
    agg::render_scanlines_aa(ras_aa, sl, ren_base, sa, sg);
}

/* ---------------------------------------------------------------------------*/
/*       Stroke an ellipse with a line symbol of the specified size and color */
/* ---------------------------------------------------------------------------*/
void msCircleDrawLineSymbolAGG(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, styleObj *style, double scalefactor)
{
  msCircleDrawLineSymbolGD(symbolset, img, p, r, style, scalefactor);
}

/* ------------------------------------------------------------------------------- */
/*       Fill a circle with a shade symbol of the specified size and color         */
/* ------------------------------------------------------------------------------- */
void msCircleDrawShadeSymbolAGG(symbolSetObj *symbolset, gdImagePtr img, pointObj *p, double r, styleObj *style, double scalefactor)
{
  msCircleDrawShadeSymbolGD(symbolset, img, p, r, style, scalefactor);
}

void msDrawMarkerSymbolAGGTrueType(symbolObj *symbol, double size, double angle, char bRotated, styleObj *style,
    int ox, int oy, gdImagePtr img, pointObj *p, symbolSetObj *symbolset, double angle_radians)
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
      msSetError(MS_TTFERR, error, "msDrawMarkerSymbolAGG()");
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

void msDrawMarkerSymbolAGGPixmap(symbolObj *symbol,  double size, double angle, char bRotated, styleObj *style,
    int ox, int oy, gdImagePtr img, pointObj *p)
{
  int offset_x, offset_y;
  double d;

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

void msDrawMarkerSymbolAGGEllipse(symbolObj *symbol, double size, double angle, char bRotated, styleObj *style,
    int ox, int oy, imageObj *image, pointObj *p)
{
  
  double w, h, x, y;
 
  w = size*symbol->sizex/symbol->sizey; /* ellipse size */
  h = size;

  x = p->x + ox; /* GD will center the ellipse on x,y */
  y = p->y + oy;
  
  
  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *> ( image->imageextra );

      if(pRowCache == NULL)
        return;

      pixelFormat thePixelFormat ( *pRowCache );
      agg::renderer_base< pixelFormat > ren_base ( thePixelFormat );
      agg::renderer_scanline_aa_solid< agg::renderer_base< pixelFormat > > ren_aa ( ren_base );
      agg::scanline_p8 sl;
      agg::rasterizer_scanline_aa<> ras_aa;
      ren_base.reset_clipping ( true );
      ren_aa.color ( agg::rgba ( ( ( double ) style->color.red ) / 255.0,
                     ( ( double ) style->color.green ) / 255.0,
                         ( ( double ) style->color.blue ) / 255.0,1.0 ) );
  
      
      agg::path_storage path;
      agg::ellipse ellipse(x,y,w/2,h/2);
      ellipse.approximation_scale ( 1 );
      path.concat_path(ellipse);
      if(symbol->filled) {
          ras_aa.add_path ( path );
          agg::render_scanlines ( ras_aa, sl, ren_aa );
          if(MS_VALID_COLOR(style->outlinecolor)) {
              ras_aa.reset();
              agg::conv_stroke<agg::path_storage> stroke(path);
              stroke.width(1);
              if(style->width!=-1)
                  stroke.width(style->width);
              stroke.line_join(agg::miter_join);
              stroke.line_cap(agg::butt_cap);
              ras_aa.add_path(stroke);
              ren_aa.color ( agg::rgba ( ( ( double ) style->outlinecolor.red ) / 255.0,
                             ( ( double ) style->outlinecolor.green ) / 255.0,
                                 ( ( double ) style->outlinecolor.blue ) / 255.0,1.0 ) );
              agg::render_scanlines(ras_aa, sl, ren_aa);
          }
      } else {
          ras_aa.reset();
          agg::conv_stroke<agg::path_storage> stroke(path);
          stroke.width(1);
          if(style->width!=-1)
              stroke.width(style->width);
          stroke.line_join(agg::miter_join);
          stroke.line_cap(agg::butt_cap);
          ras_aa.add_path(stroke);
          if(MS_VALID_COLOR(style->color)) {                       
              ren_aa.color ( agg::rgba ( ( ( double ) style->color.red ) / 255.0,
                      ( ( double ) style->color.green ) / 255.0,
                      ( ( double ) style->color.blue ) / 255.0,1.0 ) );
          }else if(MS_VALID_COLOR(style->outlinecolor)) {                       
              ren_aa.color ( agg::rgba ( ( ( double ) style->outlinecolor.red ) / 255.0,
                      ( ( double ) style->outlinecolor.green ) / 255.0,
                      ( ( double ) style->outlinecolor.blue ) / 255.0,1.0 ) );
          } else return;
          agg::render_scanlines(ras_aa, sl, ren_aa);

      }
}

void msDrawMarkerSymbolAGGVector(symbolObj *symbol, double size, double angle, char bRotated, styleObj *style,
    int ox, int oy, gdImagePtr img, pointObj *p, imageObj *image )
{
  double offset_x, offset_y;
  double d;
  mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *> ( image->imageextra );

    if(pRowCache == NULL)
      return;
    
    pixelFormat thePixelFormat ( *pRowCache );
    agg::renderer_base< pixelFormat > ren_base ( thePixelFormat );
    agg::renderer_scanline_aa_solid< agg::renderer_base< pixelFormat > > ren_aa ( ren_base );
    agg::scanline_p8 sl;
    agg::rasterizer_scanline_aa<> ras_aa;
  
    d = size/symbol->sizey; /* compute the scaling factor (d) on the unrotated symbol */

  if(angle != 0.0 && angle != 360.0) {      
    bRotated = MS_TRUE;
    symbol = msRotateSymbol(symbol, style->angle);
  }
  
  offset_x = p->x - d*.5*symbol->sizex + ox;
  offset_y = p->y - d*.5*symbol->sizey + oy;
  
  agg::path_storage path = imageVectorSymbolAGG(symbol,d);
  agg::trans_affine translation = agg::trans_affine_translation(offset_x, offset_y);
  path.transform(translation);
  ren_aa.color(agg::rgba(((double) style->color.red) / 255.0, 
          ((double) style->color.green) / 255.0, 
          ((double) style->color.blue) / 255.0, 1));
  if(symbol->filled) {
      ras_aa.add_path(path);
      agg::render_scanlines(ras_aa, sl, ren_aa);
      if(MS_VALID_COLOR(style->outlinecolor)) {
          ras_aa.reset();
          agg::conv_stroke<agg::path_storage> stroke(path);
          stroke.width(1);
          if(style->width!=-1)
              stroke.width(style->width);
          stroke.line_join(agg::miter_join);
          stroke.line_cap(agg::butt_cap);
          ras_aa.add_path(stroke);
          ren_aa.color ( agg::rgba ( ( ( double ) style->outlinecolor.red ) / 255.0,
                  ( ( double ) style->outlinecolor.green ) / 255.0,
                  ( ( double ) style->outlinecolor.blue ) / 255.0,1.0 ) );
          agg::render_scanlines(ras_aa, sl, ren_aa);
      }

  } else  { /* shade is a vector drawing */
      agg::conv_stroke <agg::path_storage > stroke(path);
      stroke.width(style->width);
      switch(symbol->linejoin) {
      case MS_CJC_ROUND:
          stroke.line_join(agg::round_join);
          break;
      case MS_CJC_MITER:
          stroke.line_join(agg::miter_join);
          break;
      case MS_CJC_BEVEL:
          stroke.line_join(agg::bevel_join);
          break;
      }
      switch(symbol->linecap) {
      case MS_CJC_BUTT:
          stroke.line_cap(agg::butt_cap);
          break;
      case MS_CJC_ROUND:
          stroke.line_cap(agg::round_cap);
          break;
      case MS_CJC_SQUARE:
          stroke.line_cap(agg::square_cap);
          break;
      }
      ras_aa.add_path(stroke);
      agg::render_scanlines(ras_aa, sl, ren_aa); 
  }
  if(bRotated) {
      msFreeSymbol(symbol); /* clean up */
      msFree(symbol);
  }
}

/* ------------------------------------------------------------------------------- */
/*       Draw a single marker symbol of the specified size and color               */
/* ------------------------------------------------------------------------------- */
void msDrawMarkerSymbolAGG(symbolSetObj *symbolset, imageObj *image, pointObj *p, styleObj *style, double scalefactor)
{
  char bRotated=MS_FALSE;
  symbolObj *symbol;

  double size, angle, angle_radians;
  int width;

  int ox, oy, bc, fc, oc;

  if(!p) return;

  gdImagePtr img    = image->img.gd;
  
  if(style->backgroundcolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->backgroundcolor));
  if(style->color.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->color));
  if(style->outlinecolor.pen == MS_PEN_UNSET) msImageSetPenGD(img, &(style->outlinecolor));

  symbol = symbolset->symbol[style->symbol];
  bc = style->backgroundcolor.pen;
  fc = style->color.pen;
  oc = style->outlinecolor.pen;
  ox = style->offsetx; /* TODO: add scaling? */
  oy = style->offsety;

  if(style->size == -1) {
    size = msSymbolGetDefaultSize(symbolset->symbol[style->symbol]);
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
    msDrawMarkerSymbolAGGTrueType(symbol, size, angle, bRotated, style, ox, oy, img, p, symbolset, angle_radians);
    break;    
  case(MS_SYMBOL_PIXMAP):
    msDrawMarkerSymbolAGGPixmap(symbol, size, angle, bRotated, style,  ox, oy, img, p);
    break;    
  case(MS_SYMBOL_ELLIPSE):
    msDrawMarkerSymbolAGGEllipse(symbol, size, angle, bRotated, style,  ox, oy, image, p);
    return;
    break;    
  case(MS_SYMBOL_VECTOR): /* TODO: Need to leverage the image cache! */
    msDrawMarkerSymbolAGGVector(symbol, size, angle, bRotated, style, ox, oy, img, p, image);
    return;
  break;
  default:
    break; 
  }

  /* fall back on GD */
  msDrawMarkerSymbolGD(symbolset, img, p, style, scalefactor);
}

/* ------------------------------------------------------------------------------- */
/*       Draw a line symbol of the specified size and color                        */
/* ------------------------------------------------------------------------------- */
void msDrawLineSymbolAGG(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor)
{
  gdImagePtr img  = image->img.gd;
    
  int width;
  int nwidth, size;
  symbolObj *symbol;

  colorObj color;

  symbol = symbolset->symbol[style->symbol];

  /*
  ** use agg for styles using symbol 0 and a width or symbol of type ellipse 
  ** (using circle symbol and size still seems to be the most common way of
  ** doing thick lines) 
  */
  if(style->symbol >= 0 && (style->symbol == 0 || symbol->type == MS_SYMBOL_ELLIPSE)) {
    color = style->color;
    if(!MS_VALID_COLOR(color)) {
      color = style->outlinecolor; /* try the outline color, polygons drawing thick outlines often do this */
      if(!MS_VALID_COLOR(color))
        return; /* no color, bail out... */
    }

    if(style->size == -1)
      size = (int) msSymbolGetDefaultSize(symbolset->symbol[style->symbol]);
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

    if(p->numlines > 0) {
      if(symbol->patternlength > 0)
        imagePolyline(image, p, &color, nwidth, 0, 0, symbol->patternlength, symbol->pattern); 
      else
       imagePolyline(image, p, &color, nwidth, 0, 0, 0, NULL);           
    }
  } else { 
    /* fall back on GD */
    msDrawLineSymbolGD(symbolset, img, p, style, scalefactor);
  }
}

/* ------------------------------------------------------------------------------- */
/*       Draw a shade symbol of the specified size and color                       */
/* ------------------------------------------------------------------------------- */
void msDrawShadeSymbolAGG(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor)
{
  gdImagePtr img  = image->img.gd;
    
  symbolObj *symbol;
  double ox,oy,size, angle, angle_radians,width;

  if(!p) return;
  if(p->numlines <= 0) return;

  symbol = symbolset->symbol[style->symbol];

  if(style->size == -1) {
    size = msSymbolGetDefaultSize(symbolset->symbol[style->symbol]);
    size = size*scalefactor;
  } else
    size = style->size*scalefactor;
  size = MS_MAX(size, style->minsize);
  size = MS_MIN(size, style->maxsize);

  width = style->width*scalefactor;
  width = MS_MAX(width, style->minwidth);
  width = MS_MIN(width, style->maxwidth);

  angle = (style->angle) ? style->angle : 0.0;
  angle_radians = angle*MS_DEG_TO_RAD;

  ox = style->offsetx*scalefactor; /* should we scale the offsets? */
  oy = style->offsety*scalefactor;

  if(!MS_VALID_COLOR(style->color) && MS_VALID_COLOR(style->outlinecolor) && symbol->type != MS_SYMBOL_PIXMAP) { /* use msDrawLineSymbolAGG() instead (POLYLINE) */
    msDrawLineSymbolAGG(symbolset, image, p, style, scalefactor);
    //imagePolyline2(image, p, &(style->outlinecolor), width, ox, oy, 0, NULL);
    return;
  }

  if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; /* no such symbol, 0 is OK   */
  if(!MS_VALID_COLOR(style->color) && symbol->type!=MS_SYMBOL_PIXMAP) return; /* nothing to do (colors are not required with PIXMAP symbols) */
  if(size < 1) return; /* size too small */
      
  if(style->symbol == 0) { /* simply draw a solid fill of the specified color */    
    imageFilledPolygon(image, p, &(style->color), ox, oy);
    if(MS_VALID_COLOR(style->outlinecolor))
      imagePolyline(image, p, &(style->outlinecolor), width, ox, oy, 0, NULL);

    return;
  }
  
  typedef agg::pixfmt_rgba32 pixfmt;
  typedef agg::rgba8 color_type;
  switch(symbol->type) {
      case(MS_SYMBOL_HATCH): {
          msComputeBounds(p);
          int pw=MS_NINT(p->bounds.maxx-p->bounds.minx)+1;
          int ph=MS_NINT(p->bounds.maxy-p->bounds.miny)+1;
          
          mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *>(image->imageextra);
          if(pRowCache == NULL)
              return;
          pixelFormat thePixelFormat(*pRowCache);
          agg::renderer_base< pixelFormat > ren_base(thePixelFormat);
          agg::renderer_scanline_aa_solid< agg::renderer_base< pixelFormat > > ren_aa(ren_base);
          agg::scanline_u8 sl1,sl2,sl;
          agg::rasterizer_scanline_aa<> ras1,ras2;
          agg::scanline_storage_aa8 storage;
          agg::scanline_storage_aa8 storage1;
          agg::scanline_storage_aa8 storage2;
          agg::path_storage hatch,polygon;
          
          if(MS_VALID_COLOR(style->backgroundcolor))
              imageFilledPolygon(image, p, &(style->backgroundcolor), ox, oy);
          
          hatch = createHatchAGG(pw,ph,style->angle,style->size);
          hatch.transform(agg::trans_affine_translation(p->bounds.minx, p->bounds.miny));
          agg::conv_stroke <agg::path_storage > stroke(hatch);
          stroke.width(style->width);
          ras1.add_path(stroke);                    
          agg::render_scanlines(ras1, sl, storage1);
          
          for(int i = 0; i < p->numlines; i++) {
              polygon.move_to(p->line[i].point[0].x+ox, p->line[i].point[0].y+oy);
              for(int j=1; j<p->line[i].numpoints; j++)
                  polygon.line_to(p->line[i].point[j].x+ox, p->line[i].point[j].y+oy);
              polygon.close_polygon();
          }
          ras2.add_path(polygon);
          agg::render_scanlines(ras2,sl,storage2);
          agg::sbool_combine_shapes_aa(agg::sbool_and, storage1, storage2, sl1, sl2, sl, storage);               
          ren_aa.color(agg::rgba(((double) style->color.red) / 255.0, 
                            ((double) style->color.green) / 255.0, 
                            ((double) style->color.blue) / 255.0, 1));
          agg::render_scanlines(storage, sl, ren_aa);
          if(MS_VALID_COLOR(style->outlinecolor))
              imagePolyline(image, p, &(style->outlinecolor), 1, ox, oy, 0, NULL);
          return;
      }
          break;
      case(MS_SYMBOL_VECTOR): {
          double d = size/symbol->sizey; /* compute the scaling factor (d) on the unrotated symbol */
          char bRotated=MS_FALSE;
          if (angle != 0.0 && angle != 360.0) {
            bRotated = MS_TRUE;
            symbol = msRotateSymbol(symbol, style->angle);
          }

          int pw = MS_NINT(symbol->sizex*d);    
          int ph = MS_NINT(symbol->sizey*d);

          if((pw <= 1) && (ph <= 1)) { /* No sense using a tile, just fill solid */
            imageFilledPolygon(image, p, &(style->color), ox, oy);
            if(MS_VALID_COLOR(style->outlinecolor))
              imagePolyline(image, p, &(style->outlinecolor), width, ox, oy, 0, NULL);     
            return;
          }

          /* create tile image */
          agg::int8u*           m_pattern;
          agg::rendering_buffer m_pattern_rbuf;
          m_pattern = new agg::int8u[pw * ph * 4];
          m_pattern_rbuf.attach(m_pattern, pw,ph, pw*4);
          pixfmt pixf(m_pattern_rbuf);
          agg::renderer_base<pixfmt> rb(pixf);
          agg::renderer_scanline_aa_solid<agg::renderer_base<pixfmt> > rs(rb);
          agg::scanline_p8 sl;
          agg::rasterizer_scanline_aa<> ras_aa;
          
          /*create path for the symbol*/
          agg::path_storage path = imageVectorSymbolAGG(symbol,d);
          
          if(MS_VALID_COLOR(style->backgroundcolor))
              rb.clear(agg::rgba(((double) style->backgroundcolor.red) / 255.0, 
                      ((double) style->backgroundcolor.green) / 255.0, 
                      ((double) style->backgroundcolor.blue) / 255.0, 1));
      
          rs.color(agg::rgba(((double) style->color.red) / 255.0, 
                  ((double) style->color.green) / 255.0, 
                  ((double) style->color.blue) / 255.0, 1));
          if(symbol->filled) {
              ras_aa.add_path(path);          
              
          } else  { /* shade is a vector drawing */
              agg::conv_stroke <agg::path_storage > stroke(path);
              stroke.width(style->width);
              switch(symbol->linejoin) {
                  case MS_CJC_ROUND:
                      stroke.line_join(agg::round_join);
                      break;
                  case MS_CJC_MITER:
                      stroke.line_join(agg::miter_join);
                      break;
                  case MS_CJC_BEVEL:
                      stroke.line_join(agg::bevel_join);
                      break;
              }
              switch(symbol->linecap) {
                  case MS_CJC_BUTT:
                      stroke.line_cap(agg::butt_cap);
                      break;
                  case MS_CJC_ROUND:
                      stroke.line_cap(agg::round_cap);
                      break;
                  case MS_CJC_SQUARE:
                      stroke.line_cap(agg::square_cap);
                      break;
              }
              ras_aa.add_path(stroke);   
          }
          agg::render_scanlines(ras_aa, sl, rs);

          imageTexturedPolygonAGG(image,p,m_pattern_rbuf);
          delete [] m_pattern;
          if(MS_VALID_COLOR(style->outlinecolor))
              imagePolyline(image, p, &(style->outlinecolor), 1, ox, oy, 0, NULL);
          

          if(bRotated) { /* free the rotated symbol */
              msFreeSymbol(symbol);
              msFree(symbol);
          }
      }
          return;
          break;
  }
    
  msDrawShadeSymbolGD(symbolset, img, p, style, scalefactor); /* fall back to GD */
}

/*
** Simply draws a label based on the label point and the supplied label object.
*/
int msDrawTextAGG(gdImagePtr img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, double scalefactor)
{
  return msDrawTextGD(img, labelPnt, string, label, fontset, scalefactor);
}

/*
 * Draw a label curved along a line
 */
int msDrawTextLineAGG(gdImagePtr img, char *string, labelObj *label, labelPathObj *labelpath, fontSetObj *fontset, double scalefactor)
{
  return msDrawTextLineGD(img, string, label, labelpath, fontset, scalefactor);
}

/* To DO: fix returned values to be MS_SUCCESS/MS_FAILURE */
int msDrawLabelCacheAGG(gdImagePtr img, mapObj *map)
{
  return msDrawLabelCacheGD(img, map);
}

/*
 * draw a filled pie slice centered on center_x,center_y
 * start and end angles are given clockwise in degrees; 0 is horizontal to the right ( -> ) 
 * the slice is filled with the styleObj's color and outlinecolor if specified
 * the styleObj's width parameter defines the width of the outline if appropriate
 * if the style contains an offset (first value used only), the slice will be offset
 * along the appropriate radius with respect to the center of the pie 
 */
void msPieSliceAGG ( imageObj *image, styleObj *style, double center_x, double center_y, double radius, double start, double end )
{
    mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *> ( image->imageextra );

    if(pRowCache == NULL)
      return;

    pixelFormat thePixelFormat ( *pRowCache );
    agg::renderer_base< pixelFormat > ren_base ( thePixelFormat );
    agg::renderer_scanline_aa_solid< agg::renderer_base< pixelFormat > > ren_aa ( ren_base );
    agg::scanline_p8 sl;
    agg::rasterizer_scanline_aa<> ras_aa;
    ren_base.reset_clipping ( true );
    ren_aa.color ( agg::rgba ( ( ( double ) style->color.red ) / 255.0,
                   ( ( double ) style->color.green ) / 255.0,
                       ( ( double ) style->color.blue ) / 255.0,1.0 ) );

    /* 
     * offset the center of the slice
     * NOTE: agg angles are anti-trigonometric
     * formula before simplification is (-(start+end)/2)*MS_PI/180 which is direction of offset
     */
    if(style->offsetx>0) {
        center_x+=style->offsetx*cos(((-start-end)*MS_PI/360.));
        center_y-=style->offsetx*sin(((-start-end)*MS_PI/360.));
    }

    /*create a path with pie slice*/
    agg::path_storage path;
    path.move_to ( center_x,center_y );
    /*NOTE: agg angles are anti-trigonometric*/
    agg::arc arc ( center_x,center_y,radius,radius,start*MS_PI/180.,end*MS_PI/180.,true );
    arc.approximation_scale ( 1 );
    path.concat_path(arc);
    path.line_to ( center_x,center_y );
    path.close_polygon();

    /*paint the fill of the slice*/
    ras_aa.add_path ( path );
    agg::render_scanlines ( ras_aa, sl, ren_aa );

    /*draw an outline of the slice*/
    if(MS_VALID_COLOR(style->outlinecolor)) {
        ras_aa.reset();
        agg::conv_stroke<agg::path_storage> stroke(path);
        stroke.width(1);
        if(style->width!=-1)
            stroke.width(style->width);
        stroke.line_join(agg::miter_join);
        stroke.line_cap(agg::butt_cap);
        ras_aa.add_path(stroke);
        ren_aa.color ( agg::rgba ( ( ( double ) style->outlinecolor.red ) / 255.0,
                       ( ( double ) style->outlinecolor.green ) / 255.0,
                           ( ( double ) style->outlinecolor.blue ) / 255.0,1.0 ) );
        agg::render_scanlines(ras_aa, sl, ren_aa);
    }
}

/*
 * draw a rectangle from point c1_x,c1_y to corner c2_x,c2_y
 * processes the styleObj's color and outlinecolor
 * styleObj's width defines width of outline
 */
void msFilledRectangleAGG ( imageObj *image, styleObj *style, double c1_x, double c1_y, 
                            double c2_x, double c2_y )
{
    mapserv_row_ptr_cache<int>  *pRowCache = static_cast<mapserv_row_ptr_cache<int>  *> ( image->imageextra );

    if(pRowCache == NULL)
      return;

    pixelFormat thePixelFormat ( *pRowCache );
    agg::renderer_base< pixelFormat > ren_base ( thePixelFormat );
    agg::renderer_scanline_aa_solid< agg::renderer_base< pixelFormat > > ren_aa ( ren_base );
    agg::scanline_p8 sl;
    agg::rasterizer_scanline_aa<> ras_aa;
    ren_base.reset_clipping ( true );
    ren_aa.color ( agg::rgba ( ( ( double ) style->color.red ) / 255.0,
                   ( ( double ) style->color.green ) / 255.0,
                       ( ( double ) style->color.blue ) / 255.0,1.0 ) );

    /*create the path of the rectangle*/
    agg::path_storage path;
    path.move_to ( c1_x,c1_y );
    path.line_to ( c2_x,c1_y );
    path.line_to ( c2_x,c2_y );
    path.line_to ( c1_x,c2_y );
    path.close_polygon();

    /*paint the fill of the slice*/
    ras_aa.add_path ( path );
    agg::render_scanlines ( ras_aa, sl, ren_aa );

    /*draw an outline of the slice*/
    if(MS_VALID_COLOR(style->outlinecolor)) {
        ras_aa.reset();
        agg::conv_stroke<agg::path_storage> stroke(path);
        stroke.width(1);
        if(style->width!=-1)
            stroke.width(style->width);
        stroke.line_join(agg::miter_join);
        stroke.line_cap(agg::butt_cap);
        ras_aa.add_path(stroke);
        ren_aa.color ( agg::rgba ( ( ( double ) style->outlinecolor.red ) / 255.0,
                       ( ( double ) style->outlinecolor.green ) / 255.0,
                           ( ( double ) style->outlinecolor.blue ) / 255.0,1.0 ) );
        agg::render_scanlines(ras_aa, sl, ren_aa);
    }
}

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
  if (strcasecmp(format->driver,"AGG/PNG24") == 0)
    strcat(cGDFormat,"png"); 
  else
    strcat(cGDFormat, &(format->driver[4]));
  
  format->driver = &cGDFormat[0];
  
  iReturn = msSaveImageGD(img, filename, format);
  
  format->driver = pFormatBuffer;
  
  return iReturn;
}

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

/* ===========================================================================
   msSaveImageBufferGD

   Save image data to a unsigned char * buffer.  In the future we should try
   to merge this with msSaveImageStreamGD function.

   The returned buffer is owned by the caller. It should be freed with gdFree()
   ======================================================================== */
unsigned char *msSaveImageBufferAGG(gdImagePtr img, int *size_ptr, outputFormatObj *format)
{
  char *pFormatBuffer;
  char *pszGDFormat = NULL;
  unsigned char *buf = NULL;

  pFormatBuffer = format->driver;

  pszGDFormat = msStringConcatenate(pszGDFormat, "gd/");
  if (strcasecmp(format->driver,"AGG/PNG24") == 0)
    pszGDFormat = msStringConcatenate(pszGDFormat, "png");
  else
    pszGDFormat = msStringConcatenate(pszGDFormat, &(format->driver[4]));

  format->driver = pszGDFormat;

  buf = msSaveImageBufferGD(img, size_ptr, format);

  format->driver = pFormatBuffer;

  msFree(pszGDFormat);

  return buf;
}

/*
** Free gdImagePtr
*/
void msFreeImageAGG(gdImagePtr img)
{
  msFreeImageGD(img);
}

void msTransformShapeAGG(shapeObj *shape, rectObj extent, double cellsize)
{
  int i,j,k; /* loop counters */
  double inv_cs = 1.0 / cellsize; /* invert and multiply much faster */

  if(shape->numlines == 0) return; /* nothing to transform */

  if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON) { /* remove co-linear vertices */
  
    for(i=0; i<shape->numlines; i++) { /* for each part */
      
      shape->line[i].point[0].x = MS_MAP2IMAGE_X_IC_DBL(shape->line[i].point[0].x, extent.minx, inv_cs);
      shape->line[i].point[0].y = MS_MAP2IMAGE_Y_IC_DBL(shape->line[i].point[0].y, extent.maxy, inv_cs);
      
      for(j=1, k=1; j < shape->line[i].numpoints; j++ ) {
	
        shape->line[i].point[k].x = MS_MAP2IMAGE_X_IC_DBL(shape->line[i].point[j].x, extent.minx, inv_cs);
        shape->line[i].point[k].y = MS_MAP2IMAGE_Y_IC_DBL(shape->line[i].point[j].y, extent.maxy, inv_cs);

        if(k == 1) {
          if((shape->line[i].point[0].x != shape->line[i].point[1].x) || (shape->line[i].point[0].y != shape->line[i].point[1].y))
            k++;
        } else {
          if((shape->line[i].point[k-1].x != shape->line[i].point[k].x) || (shape->line[i].point[k-1].y != shape->line[i].point[k].y)) {
            if(((shape->line[i].point[k-2].y - shape->line[i].point[k-1].y)*(shape->line[i].point[k-1].x - shape->line[i].point[k].x)) == ((shape->line[i].point[k-2].x - shape->line[i].point[k-1].x)*(shape->line[i].point[k-1].y - shape->line[i].point[k].y))) {	    
              shape->line[i].point[k-1].x = shape->line[i].point[k].x;
              shape->line[i].point[k-1].y = shape->line[i].point[k].y;	
            } else {
              k++;
            }
          }
        }
      }
      shape->line[i].numpoints = k; /* save actual number kept */
    }
  } else { /* points or untyped shapes */
    for(i=0; i<shape->numlines; i++) { /* for each part */
      for(j=1; j < shape->line[i].numpoints; j++ ) {
        shape->line[i].point[j].x = MS_MAP2IMAGE_X_IC(shape->line[i].point[j].x, extent.minx, inv_cs);
        shape->line[i].point[j].y = MS_MAP2IMAGE_Y_IC(shape->line[i].point[j].y, extent.maxy, inv_cs);
      }
    }
  }
}

/*
 * transform the alpha values of the pixels in im from gd-convention (127 to 0)
 * to agg convention (0 to 255)
 * NOTE/TODO: due to rounding an alpha value of 0 will never happen in agg
 */
void msAlphaGD2AGG(imageObj *im) {
    int x, y;
    for (y = 0; (y < im->img.gd->sy); y++) {
        for (x = 0; (x < im->img.gd->sx); x++) {
            int c = gdImageGetPixel(im->img.gd, x, y);
            int alpha=255-(((c) & 0x7F000000) >> 24)*2;
            /* Use gdImageTrueColorPixel() to access the tpixels[][] directly 
             * and avoid special treatment of some color values by 
             * gdImageSetPixel() */
            gdImageTrueColorPixel(im->img.gd, x, y) = ((c)&0x00FFFFFF)|(alpha<<24);
        }
    }
}

/*
 * transform the alpha values of the pixels in im from agg convention (0 to 255)
 * to gd-convention (127 to 0)
 */
void msAlphaAGG2GD(imageObj *im) {
    int x, y;
    for (y = 0; (y < im->img.gd->sy); y++) {
        for (x = 0; (x < im->img.gd->sx); x++) {
            int c = gdImageGetPixel(im->img.gd, x, y);
            int alpha=(255-(((c) & 0xFF000000) >> 24))/2;
            gdImageSetPixel(im->img.gd, x, y, ((c)&0x00FFFFFF)|(alpha<<24));
        }
    }
}

#endif

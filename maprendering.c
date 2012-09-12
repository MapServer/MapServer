/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Rendering utility functions
 * Author:   Thomas Bonfort and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
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
#include "mapcopy.h"

int computeLabelStyle(labelStyleObj *s, labelObj *l, fontSetObj *fontset,
                      double scalefactor, double resolutionfactor)
{
  INIT_LABEL_STYLE(*s);
  if(!MS_VALID_COLOR(l->color))
    return MS_FAILURE;
  if(l->size == -1)
    return MS_FAILURE;

  s->size = l->size;
  if(l->type == MS_TRUETYPE) {
    s->size *=  scalefactor;
    s->size = MS_MAX(s->size, l->minsize*resolutionfactor);
    s->size = MS_MIN(s->size, l->maxsize*resolutionfactor);
    if (!fontset) {
      msSetError(MS_TTFERR, "No fontset defined.","computeLabelStyle()");
      return (MS_FAILURE);
    }

    if (!l->font || !(*l->font)) {
      return (MS_FAILURE);
    }

    if(MS_FAILURE == msFontsetLookupFonts(l->font,&s->numfonts,fontset,s->fonts)) {
      return MS_FAILURE;
    }
  }
  s->rotation = l->angle * MS_DEG_TO_RAD;
  s->antialias = l->antialias;
  return MS_SUCCESS;
}

void computeSymbolStyle(symbolStyleObj *s, styleObj *src, symbolObj *symbol, double scalefactor,
    double resolutionfactor)
{
  double default_size;
  double target_size;
  double style_size;

  default_size = msSymbolGetDefaultSize(symbol);
  style_size = (src->size==-1)?default_size:src->size;

  INIT_SYMBOL_STYLE(*s);
  if(symbol->type == MS_SYMBOL_PIXMAP) {
    s->color = s->outlinecolor = NULL;
  } else if(symbol->filled || symbol->type == MS_SYMBOL_TRUETYPE) {
    if(MS_VALID_COLOR(src->color))
      s->color = &src->color;
    if(MS_VALID_COLOR(src->outlinecolor))
      s->outlinecolor = &src->outlinecolor;
  } else {
    if(MS_VALID_COLOR(src->color))
      s->outlinecolor = &(src->color);
    else if(MS_VALID_COLOR(src->outlinecolor))
      s->outlinecolor = &(src->outlinecolor);
    s->color = NULL;
  }


  if(MS_VALID_COLOR(src->backgroundcolor)) {
    s->backgroundcolor = &(src->backgroundcolor);
  }

  target_size = style_size * scalefactor;
  target_size = MS_MAX(target_size, src->minsize*resolutionfactor);
  target_size = MS_MIN(target_size, src->maxsize*resolutionfactor);
  s->scale = target_size / default_size;
  s->gap = src->gap * target_size / style_size;

  if(s->outlinecolor) {
    s->outlinewidth =  src->width * scalefactor;
    s->outlinewidth = MS_MAX(s->outlinewidth, src->minwidth*resolutionfactor);
    s->outlinewidth = MS_MIN(s->outlinewidth, src->maxwidth*resolutionfactor);
  } else {
    s->outlinewidth = 0;
  }

  s->rotation = src->angle * MS_DEG_TO_RAD;
}


#define COMPARE_COLORS(a,b) (\
    ((a).red==(b).red) && \
    ((a).green==(b).green) && \
    ((a).blue==(b).blue) && \
    ((a).alpha==(b).alpha))

tileCacheObj *searchTileCache(imageObj *img, symbolObj *symbol, symbolStyleObj *s, int width, int height)
{
  tileCacheObj *cur = img->tilecache;
  while(cur != NULL) {
    if( cur->width == width
        && cur->height == height
        && cur->symbol == symbol
        && cur->outlinewidth == s->outlinewidth
        && cur->rotation == s->rotation
        && cur->scale == s->scale
        && (!s->color || COMPARE_COLORS(cur->color,*s->color))
        && (!s->backgroundcolor || COMPARE_COLORS(cur->backgroundcolor,*s->backgroundcolor))
        && (!s->outlinecolor || COMPARE_COLORS(cur->outlinecolor,*s->outlinecolor)))
      return cur;
    cur = cur->next;
  }
  return NULL;
}

/* add a cached tile to the current image's cache */
tileCacheObj *addTileCache(imageObj *img,
                           imageObj *tile, symbolObj *symbol, symbolStyleObj *style, int width, int height)
{
  tileCacheObj *cachep;

  if(img->ntiles >= MS_IMAGECACHESIZE) { /* remove last element, size stays the same */
    cachep = img->tilecache;

    /*go to the before last cache object*/
    while(cachep->next && cachep->next->next) cachep = cachep->next;

    /*free the last tile's data*/
    msFreeImage(cachep->next->image);

    /*reuse the last tile object*/
    /* make the cache point to the start of the list*/
    cachep->next->next = img->tilecache;
    /* point the global cache to the new first */
    img->tilecache = cachep->next;
    /* the before last cache is now last, so it has no successor*/
    cachep->next = NULL;

  } else {
    img->ntiles += 1;
    cachep = (tileCacheObj*)malloc(sizeof(tileCacheObj));
    MS_CHECK_ALLOC(cachep, sizeof(tileCacheObj), NULL);
    cachep->next = img->tilecache;
    img->tilecache = cachep;
  }

  cachep = img->tilecache;

  cachep->image = tile;
  cachep->outlinewidth = style->outlinewidth;
  cachep->scale = style->scale;
  cachep->rotation = style->rotation;
  cachep->outlinewidth = style->outlinewidth;
  if(style->color) MS_COPYCOLOR(&cachep->color,style->color);
  if(style->outlinecolor) MS_COPYCOLOR(&cachep->outlinecolor,style->outlinecolor);
  if(style->backgroundcolor) MS_COPYCOLOR(&cachep->backgroundcolor,style->backgroundcolor);
  cachep->width = width;
  cachep->height = height;
  cachep->symbol = symbol;
  return(cachep);
}

imageObj *getTile(imageObj *img, symbolObj *symbol,  symbolStyleObj *s, int width, int height,
                  int seamlessmode)
{
  tileCacheObj *tile;
  rendererVTableObj *renderer = img->format->vtable;
  if(width==-1 || height == -1) {
    width=height=MS_MAX(symbol->sizex,symbol->sizey);
  }
  tile = searchTileCache(img,symbol,s,width,height);
  if(tile==NULL) {
    imageObj *tileimg;
    double p_x,p_y;
    tileimg = msImageCreate(width,height,img->format,NULL,NULL,img->resolution, img->resolution, NULL);
    if(!seamlessmode) {
      p_x = width/2.0;
      p_y = height/2.0;
      switch(symbol->type) {
        case (MS_SYMBOL_TRUETYPE):
          renderer->renderTruetypeSymbol(tileimg, p_x, p_y, symbol, s);
          break;
        case (MS_SYMBOL_PIXMAP):
          if(msPreloadImageSymbol(renderer,symbol) != MS_SUCCESS) {
            return NULL; /* failed to load image, renderer should have set the error message */
          }
          renderer->renderPixmapSymbol(tileimg, p_x, p_y, symbol, s);
          break;
        case (MS_SYMBOL_ELLIPSE):
          renderer->renderEllipseSymbol(tileimg, p_x, p_y,symbol, s);
          break;
        case (MS_SYMBOL_VECTOR):
          renderer->renderVectorSymbol(tileimg, p_x, p_y, symbol, s);
          break;

        case (MS_SYMBOL_SVG):
#ifdef USE_SVG_CAIRO
          if(msPreloadSVGSymbol(symbol) != MS_SUCCESS) {
            return NULL; //failed to load image, renderer should have set the error message
          }
          if (renderer->supports_svg) {
            if(renderer->renderSVGSymbol(tileimg, p_x, p_y, symbol, s) != MS_SUCCESS) {
              return NULL;
            }
          } else {
            if (msRenderRasterizedSVGSymbol(tileimg,p_x,p_y,symbol, s) != MS_SUCCESS) {
              return NULL;
            }
          }
#else
          msSetError(MS_SYMERR, "SVG symbol support is not enabled.", "getTile()");
          return NULL;
#endif
          break;
        default:
          break;
      }
    } else {
      /*
       * in seamless mode, we render the the symbol 9 times on a 3x3 grid to account for
       * antialiasing blending from one tile to the next. We finally keep the center tile
       */
      imageObj *tile3img = msImageCreate(width*3,height*3,img->format,NULL,NULL,
                                         img->resolution, img->resolution, NULL);
      int i,j;
      rasterBufferObj tmpraster;
      for(i=1; i<=3; i++) {
        p_x = (i+0.5)*width;
        for(j=1; j<=3; j++) {
          p_y = (j+0.5) * height;
          switch(symbol->type) {
            case (MS_SYMBOL_TRUETYPE):
              renderer->renderTruetypeSymbol(tile3img, p_x, p_y, symbol, s);
              break;
            case (MS_SYMBOL_PIXMAP):
              if(msPreloadImageSymbol(renderer,symbol) != MS_SUCCESS) {
                return NULL; /* failed to load image, renderer should have set the error message */
              }
              renderer->renderPixmapSymbol(tile3img, p_x, p_y, symbol, s);
              break;
            case (MS_SYMBOL_ELLIPSE):
              renderer->renderEllipseSymbol(tile3img, p_x, p_y,symbol, s);
              break;
            case (MS_SYMBOL_VECTOR):
              renderer->renderVectorSymbol(tile3img, p_x, p_y, symbol, s);
              break;
              /*we should never get into these cases since the seamlessmode mode seems to
                only be for vector symbols. But if that changes ...*/
            case (MS_SYMBOL_SVG):
#ifdef USE_SVG_CAIRO
              if(msPreloadSVGSymbol(symbol) != MS_SUCCESS) {
                return NULL; //failed to load image, renderer should have set the error message
              }
              if (renderer->supports_svg) {
                renderer->renderSVGSymbol(tile3img, p_x, p_y, symbol, s);
              } else {
                msRenderRasterizedSVGSymbol(tile3img,p_x,p_y,symbol, s);
              }
#else
              msSetError(MS_SYMERR, "SVG symbol support is not enabled.", "getTile()");
              return NULL;
#endif
              break;
            default:
              break;
          }
        }
      }

      MS_IMAGE_RENDERER(tile3img)->getRasterBufferHandle(tile3img,&tmpraster);
      renderer->mergeRasterBuffer(tileimg,
                                  &tmpraster,
                                  1.0,width,height,0,0,width,height
                                 );
      msFreeImage(tile3img);
    }
    tile = addTileCache(img,tileimg,symbol,s,width,height);
  }
  return tile->image;
}

int msImagePolylineMarkers(imageObj *image, shapeObj *p, symbolObj *symbol,
                           symbolStyleObj *style, double spacing,
                           double initialgap, int auto_angle)
{
  rendererVTableObj *renderer = MS_IMAGE_RENDERER(image);
  int i,j;
  pointObj point;
  double original_rotation = style->rotation;
  double symbol_width,symbol_height;
  int ret = MS_FAILURE;
  if(symbol->type != MS_SYMBOL_TRUETYPE) {
    symbol_width = MS_MAX(1,symbol->sizex*style->scale);
    symbol_height = MS_MAX(1,symbol->sizey*style->scale);
  } else {
    rectObj rect;
    if(MS_SUCCESS != renderer->getTruetypeTextBBox(renderer,&symbol->full_font_path,1,style->scale,
        symbol->character,&rect,NULL,0))
      return MS_FAILURE;
    symbol_width=rect.maxx-rect.minx;
    symbol_height=rect.maxy-rect.miny;
  }
  for(i=0; i<p->numlines; i++) {
    int line_in = 0;
    double line_length=0;
    double current_length;
    if(initialgap < 0) {
      current_length = spacing/2.0; /* initial padding for each line */
    } else {
      current_length = initialgap; /* initial padding for each line */
    }
    for(j=1; j<p->line[i].numpoints; j++) {
      double rx,ry,theta,length;
      length = sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2)));
      line_length += length;
      if(length==0)continue;
      rx = (p->line[i].point[j].x - p->line[i].point[j-1].x)/length;
      ry = (p->line[i].point[j].y - p->line[i].point[j-1].y)/length;

      if (auto_angle) {
        theta = asin(ry);
        if(rx < 0) {
          theta += MS_PI;
        } else theta = -theta;
        style->rotation = original_rotation + theta;
      }
      while (current_length <= length) {

        point.x = p->line[i].point[j - 1].x + current_length * rx;
        point.y = p->line[i].point[j - 1].y + current_length * ry;
        if(symbol->anchorpoint_x != 0.5 || symbol->anchorpoint_y != 0.5) {
          double ox, oy;
          ox = (0.5 - symbol->anchorpoint_x) * symbol_width;
          oy = (0.5 - symbol->anchorpoint_y) * symbol_height;
          if(style->rotation != 0) {
            double sina,cosa;
            double rox,roy;
            sina = sin(-style->rotation);
            cosa = cos(-style->rotation);
            rox = ox * cosa - oy * sina;
            roy = ox * sina + oy * cosa;
            point.x += rox;
            point.y += roy;
          } else {
            point.x += ox;
            point.y += oy;
          }
        }

        /* if the point is not in the map extent, skip it. (POLYLINE_NO_CLIP) */
        if ( (point.x < -(symbol_width) || point.x > (image->width+symbol_width)) ||
             (point.y < -(symbol_height) || point.y > (image->height+symbol_height)) ) {
          current_length += spacing;
          line_in=1;          
          continue;
        }
          
        switch (symbol->type) {
          case MS_SYMBOL_PIXMAP:
            ret = renderer->renderPixmapSymbol(image, point.x, point.y, symbol, style);
            break;
          case MS_SYMBOL_ELLIPSE:
            ret = renderer->renderEllipseSymbol(image, point.x, point.y, symbol, style);
            break;
          case MS_SYMBOL_VECTOR:
            ret = renderer->renderVectorSymbol(image, point.x, point.y, symbol, style);
            break;
          case MS_SYMBOL_TRUETYPE:
            ret = renderer->renderTruetypeSymbol(image, point.x, point.y, symbol, style);
            break;
        }
        if( ret != MS_SUCCESS)
          return ret;
        current_length += spacing;
        line_in=1;
      }

      current_length -= length;
    }

    /*
     * if we couldn't place a symbol on the line and no initialgap was
     * specified,  add one now we don't add the symbol if the line is shorter
     * than the length of the symbol itself
     */
    if(initialgap < 0 && !line_in && line_length>symbol_width) {

      /* total lengths of beginnning and end of current segment */
      double before_length=0,after_length=0;

      /*optimize*/
      line_length /= 2.0;

      for(j=1; j<p->line[i].numpoints; j++) {
        double length;
        length = sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2)));
        after_length += length;
        if(after_length>line_length) {
          double rx,ry,theta;
          /* offset where the symbol should be drawn on the current
           * segment */
          double offset = line_length - before_length;

          rx = (p->line[i].point[j].x - p->line[i].point[j-1].x)/length;
          ry = (p->line[i].point[j].y - p->line[i].point[j-1].y)/length;
          if (auto_angle) {
            theta = asin(ry);
            if(rx < 0) {
              theta += MS_PI;
            } else theta = -theta;
            style->rotation = original_rotation + theta;
          }

          point.x = p->line[i].point[j - 1].x + offset * rx;
          point.y = p->line[i].point[j - 1].y + offset * ry;
          switch (symbol->type) {
            case MS_SYMBOL_PIXMAP:
              ret = renderer->renderPixmapSymbol(image, point.x, point.y, symbol, style);
              break;
            case MS_SYMBOL_ELLIPSE:
              ret = renderer->renderEllipseSymbol(image, point.x, point.y, symbol, style);
              break;
            case MS_SYMBOL_VECTOR:
              ret = renderer->renderVectorSymbol(image, point.x, point.y, symbol, style);
              break;
            case MS_SYMBOL_TRUETYPE:
              ret = renderer->renderTruetypeSymbol(image, point.x, point.y, symbol, style);
              break;
          }
          break; /* we have rendered the single marker for this line */
        }
        before_length += length;
      }
    }

  }
  return ret;
}

int msDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p,
                     styleObj *style, double scalefactor)
{
  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
      rendererVTableObj *renderer = image->format->vtable;
      symbolObj *symbol;
      shapeObj *offsetLine = p;
      int i;
      double width;
      double finalscalefactor;

      if (p->numlines == 0)
        return MS_SUCCESS;

      if (style->symbol >= symbolset->numsymbols || style->symbol < 0)
        return MS_SUCCESS; /* no such symbol, 0 is OK   */

      symbol = symbolset->symbol[style->symbol];
      /* store a reference to the renderer to be used for freeing */
      symbol->renderer = renderer;

      width = style->width * scalefactor;
      width = MS_MIN(width,style->maxwidth*image->resolutionfactor);
      width = MS_MAX(width,style->minwidth*image->resolutionfactor);
      if(style->width != 0) {
        finalscalefactor = width / style->width;
      } else {
        finalscalefactor = 1.0;
      }

      if(style->offsety==-99) {
        offsetLine = msOffsetPolyline(p,style->offsetx * finalscalefactor ,-99);
      } else if(style->offsetx!=0 || style->offsety!=0) {
        offsetLine = msOffsetPolyline(p, style->offsetx * finalscalefactor,
                                      style->offsety * finalscalefactor);
      }
      if(style->symbol == 0 || (symbol->type==MS_SYMBOL_SIMPLE)) {
        strokeStyleObj s;
        s.linecap = style->linecap;
        s.linejoin = style->linejoin;
        s.linejoinmaxsize = style->linejoinmaxsize;
        s.width = width;
        s.patternlength = style->patternlength;
        for(i=0; i<s.patternlength; i++)
          s.pattern[i] = style->pattern[i] * finalscalefactor;
        s.patternoffset = (style->initialgap<=0) ? 0 : (style->initialgap * finalscalefactor);

        if(MS_VALID_COLOR(style->color))
          s.color = &style->color;
        else if(MS_VALID_COLOR(style->outlinecolor))
          s.color = &style->outlinecolor;
        else {
          /* msSetError(MS_MISCERR,"no color defined for line styling","msDrawLineSymbol()");
           * not really an error */
          return MS_SUCCESS;
        }
        renderer->renderLine(image,offsetLine,&s);
      } else {
        symbolStyleObj s;
        switch (symbol->type) {
          case (MS_SYMBOL_TRUETYPE): {
            if(!symbol->full_font_path)
              symbol->full_font_path =  msStrdup(msLookupHashTable(&(symbolset->fontset->fonts),
                                                 symbol->font));
            if(!symbol->full_font_path) {
              msSetError(MS_MEMERR,"allocation error", "msDrawMArkerSymbol()");
              return MS_FAILURE;
            }
          }
          break;
          case (MS_SYMBOL_PIXMAP): {
            if(!symbol->pixmap_buffer) {
              if(MS_SUCCESS != msPreloadImageSymbol(renderer,symbol))
                return MS_FAILURE;
            }
          }
          break;
        }

        INIT_SYMBOL_STYLE(s);
        computeSymbolStyle(&s,style,symbol,scalefactor,image->resolutionfactor);
        s.style = style;
        if(symbol->type == MS_SYMBOL_TRUETYPE) {
          if(!symbol->full_font_path)
            symbol->full_font_path =  msStrdup(msLookupHashTable(&(symbolset->fontset->fonts),
                                               symbol->font));
          assert(symbol->full_font_path);
        }

        /* compute a markerStyle and use it on the line */
        if(style->gap<0) {
          /* special function that treats any other symbol used as a marker, not a brush */
          msImagePolylineMarkers(image,offsetLine,symbol,&s,-s.gap,
                                 style->initialgap * finalscalefactor, 1);
        } else if(style->gap>0) {
          msImagePolylineMarkers(image,offsetLine,symbol,&s,s.gap,
                                 style->initialgap * finalscalefactor,0);
        } else {
          if(renderer->renderLineTiled != NULL) {
            int pw,ph;
            imageObj* tile=NULL;
            if(s.scale != 1) {
              pw = MS_NINT(symbol->sizex * s.scale);
              ph = MS_NINT(symbol->sizey * s.scale);
            } else {
              pw = symbol->sizex;
              ph = symbol->sizey;
            }
            if(pw<1) pw=1;
            if(ph<1) ph=1;
            tile = getTile(image, symbol,&s,pw,ph,0);
            renderer->renderLineTiled(image, offsetLine, tile);
          } else {
            msSetError(MS_RENDERERERR, "renderer does not support brushed lines", "msDrawLineSymbol()");
            return MS_FAILURE;
          }
        }
      }

      if(offsetLine!=p) {
        msFreeShape(offsetLine);
        msFree(offsetLine);
      }
    } else if( MS_RENDERER_IMAGEMAP(image->format) )
      msDrawLineSymbolIM(symbolset, image, p, style, scalefactor);
    else {
      msSetError(MS_RENDERERERR, "unsupported renderer", "msDrawShadeSymbol()");
      return MS_FAILURE;
    }
  } else {
    return MS_FAILURE;
  }
  return MS_SUCCESS;
}

int msDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor)
{
  int ret = MS_SUCCESS;
  if (!p)
    return MS_SUCCESS;
  if (p->numlines <= 0)
    return MS_SUCCESS;

  if (style->symbol >= symbolset->numsymbols || style->symbol < 0)
    return MS_SUCCESS; /* no such symbol, 0 is OK */

  /*
   * if only an outlinecolor was defined, and not a color,
   * switch to the line drawing function
   *
   * this behavior is kind of a mapfile hack, and must be
   * kept for backwards compatibility
   */
  if (symbolset->symbol[style->symbol]->type != MS_SYMBOL_PIXMAP && symbolset->symbol[style->symbol]->type != MS_SYMBOL_SVG ) {
    if (!MS_VALID_COLOR(style->color)) {
      if(MS_VALID_COLOR(style->outlinecolor))
        return msDrawLineSymbol(symbolset, image, p, style, scalefactor);
      else {
        /* just do nothing if no color has been set */
        return MS_SUCCESS;
      }
    }
  }
  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
      rendererVTableObj *renderer = image->format->vtable;
      shapeObj *offsetPolygon = NULL;
      symbolObj *symbol = symbolset->symbol[style->symbol];
      /* store a reference to the renderer to be used for freeing */
      if(style->symbol)
        symbol->renderer = renderer;

      if (style->offsetx != 0 || style->offsety != 0) {
        if(style->offsety==-99)
          offsetPolygon = msOffsetPolyline(p, style->offsetx*scalefactor, -99);
        else
          offsetPolygon = msOffsetPolyline(p, style->offsetx*scalefactor,style->offsety*scalefactor);
      } else {
        offsetPolygon=p;
      }
      /* simple polygon drawing, without any specific symbol.
       * also draws an optional outline */
      if(style->symbol == 0 || symbol->type == MS_SYMBOL_SIMPLE) {
        ret = renderer->renderPolygon(image,offsetPolygon,&style->color);
        if(ret != MS_SUCCESS) goto cleanup;
        if(MS_VALID_COLOR(style->outlinecolor)) {
          strokeStyleObj s;
          INIT_STROKE_STYLE(s);
          s.color = &style->outlinecolor;
          s.color->alpha = style->color.alpha;
          s.width = (style->width == 0)?scalefactor:style->width*scalefactor;
          s.width = MS_MIN(s.width, style->maxwidth);
          s.width = MS_MAX(s.width, style->minwidth);
          ret = renderer->renderLine(image,offsetPolygon,&s);
        }
        goto cleanup; /*finished plain polygon*/
      } else if(symbol->type == MS_SYMBOL_HATCH) {
        double width, spacing;
        double pattern[MS_MAXPATTERNLENGTH];
        int i;

        if(MS_VALID_COLOR(style->backgroundcolor)) {
          ret = renderer->renderPolygon(image,offsetPolygon, &style->backgroundcolor);
          if(ret != MS_SUCCESS) goto cleanup;
        }
        width = (style->width <= 0)?scalefactor:style->width*scalefactor;
        width = MS_MIN(width, style->maxwidth*image->resolutionfactor);
        width = MS_MAX(width, style->minwidth*image->resolutionfactor);
        spacing = (style->size <= 0)?scalefactor:style->size*scalefactor;
        spacing = MS_MIN(spacing, style->maxsize*image->resolutionfactor);
        spacing = MS_MAX(spacing, style->minsize*image->resolutionfactor);

        /* scale the pattern by the factor applied to the width */
        for(i=0; i<style->patternlength; i++) {
          pattern[i] = style->pattern[i]*width/style->width;
        }

        ret = msHatchPolygon(image,offsetPolygon,spacing,width,pattern,style->patternlength,style->angle, &style->color);
        goto cleanup;
      } else {
        symbolStyleObj s;
        int pw,ph;
        imageObj *tile;
        int seamless = 0;


        switch(symbol->type) {
          case MS_SYMBOL_PIXMAP:
            if(MS_SUCCESS != msPreloadImageSymbol(renderer,symbol)) {
              ret = MS_FAILURE;
              goto cleanup;
            }
            break;
          case MS_SYMBOL_TRUETYPE:
            if(!symbol->full_font_path)
              symbol->full_font_path =  msStrdup(msLookupHashTable(&(symbolset->fontset->fonts),
                                                 symbol->font));
            if(!symbol->full_font_path) {
              msSetError(MS_MEMERR,"allocation error", "msDrawMarkerSymbol()");
              ret = MS_FAILURE;
              goto cleanup;
            }
            break;
          case MS_SYMBOL_SVG:
#ifdef USE_SVG_CAIRO
            if(MS_SUCCESS != msPreloadSVGSymbol(symbol)) {
              ret = MS_FAILURE;
              goto cleanup;
            }
#else
            msSetError(MS_SYMERR, "SVG symbol support is not enabled.", "msDrawMarkerSymbol()");
            return MS_FAILURE;
#endif
            break;
          case MS_SYMBOL_VECTOR:
          case MS_SYMBOL_ELLIPSE:
            break;
          default:
            msSetError(MS_MISCERR,"unsupported symbol type %d", "msDrawShadeSymbol()", symbol->type);
            ret = MS_FAILURE;
            goto cleanup;
        }

        INIT_SYMBOL_STYLE(s);
        computeSymbolStyle(&s,style,symbol,scalefactor,image->resolutionfactor);
        s.style = style;

        if (!s.color && !s.outlinecolor && symbol->type != MS_SYMBOL_PIXMAP && symbol->type != MS_SYMBOL_SVG) {
          ret = MS_SUCCESS; /* nothing to do (colors are required except for PIXMAP symbols */
          goto cleanup;
        }

        if(s.backgroundcolor) {
          ret = renderer->renderPolygon(image,offsetPolygon, s.backgroundcolor);
          if(ret != MS_SUCCESS) goto cleanup;
        }

        if(s.scale != 1) {
          if (s.gap > 0) {
            pw = MS_MAX(MS_NINT(s.gap),symbol->sizex * s.scale);
            ph = MS_MAX(MS_NINT(s.gap),symbol->sizey * s.scale);
          } else {
            pw = MS_NINT(symbol->sizex * s.scale);
            ph = MS_NINT(symbol->sizey * s.scale);
          }
        } else {
          if (s.gap > 0) {
            pw = MS_MAX(s.gap,symbol->sizex);
            ph = MS_MAX(s.gap,symbol->sizey);
          } else {
            pw = symbol->sizex;
            ph = symbol->sizey;
          }
        }
        if(pw<1) pw=1;
        if(ph<1) ph=1;

        /* if we're doing vector symbols with an antialiased pixel rendererer, we want to enable
         * seamless mode, i.e. comute a tile that accounts for the blending of neighbouring
         * tiles at the tile border
         */
        if(symbol->type == MS_SYMBOL_VECTOR && style->gap == 0 &&
            (image->format->renderer == MS_RENDER_WITH_AGG ||
             image->format->renderer == MS_RENDER_WITH_CAIRO_RASTER)) {
          seamless = 1;
        }
        tile = getTile(image,symbol,&s,pw,ph,seamless);
        ret = renderer->renderPolygonTiled(image,offsetPolygon, tile);
      }

cleanup:
      if (offsetPolygon != p) {
        msFreeShape(offsetPolygon);
        msFree(offsetPolygon);
      }
      return ret;
    } else if( MS_RENDERER_IMAGEMAP(image->format) )
      msDrawShadeSymbolIM(symbolset, image, p, style, scalefactor);
  }
  return ret;
}

int msDrawMarkerSymbol(symbolSetObj *symbolset,imageObj *image, pointObj *p, styleObj *style,
                       double scalefactor)
{
  int ret = MS_SUCCESS;
  if (!p)
    return MS_SUCCESS;
  if (style->symbol >= symbolset->numsymbols || style->symbol <= 0)
    return MS_SUCCESS; /* no such symbol, 0 is OK   */

  if (image) {
    if(MS_RENDERER_PLUGIN(image->format)) {
      rendererVTableObj *renderer = image->format->vtable;
      symbolStyleObj s;
      double p_x,p_y;
      symbolObj *symbol = symbolset->symbol[style->symbol];
      /* store a reference to the renderer to be used for freeing */
      symbol->renderer = renderer;
      switch (symbol->type) {
        case (MS_SYMBOL_TRUETYPE): {
          if (!symbol->full_font_path)
            symbol->full_font_path = msStrdup(msLookupHashTable(&(symbolset->fontset->fonts),
                                              symbol->font));
          if (!symbol->full_font_path) {
            msSetError(MS_MEMERR, "allocation error", "msDrawMarkerSymbol()");
            return MS_FAILURE;
          }
        }
        break;
        case (MS_SYMBOL_PIXMAP): {
          if (!symbol->pixmap_buffer) {
            if (MS_SUCCESS != msPreloadImageSymbol(renderer, symbol))
              return MS_FAILURE;
          }
        }
        break;

        case (MS_SYMBOL_SVG): {
#ifdef USE_SVG_CAIRO
          if (!symbol->renderer_cache) {
            if (MS_SUCCESS != msPreloadSVGSymbol(symbol))
              return MS_FAILURE;
          }
#else
          msSetError(MS_SYMERR, "SVG symbol support is not enabled.", "msDrawMarkerSymbol()");
          return MS_FAILURE;
#endif
        }
        break;
      }

      s.style = style;
      computeSymbolStyle(&s,style,symbol,scalefactor,image->resolutionfactor);
      s.style = style;
      if (!s.color && !s.outlinecolor && symbol->type != MS_SYMBOL_PIXMAP &&
          symbol->type != MS_SYMBOL_SVG) {
        return MS_SUCCESS; // nothing to do if no color, except for pixmap symbols
      }



      /* TODO: skip the drawing of the symbol if it's smaller than a pixel ?
      if (s.size < 1)
       return; // size too small
       */

      p_x = p->x;
      p_y = p->y;

      if (style->polaroffsetpixel != 0 ||
          style->polaroffsetangle != 0) {
        double angle = style->polaroffsetangle * MS_DEG_TO_RAD;
        p_x +=  (style->polaroffsetpixel * cos(-angle)) * scalefactor;
        p_y +=  (style->polaroffsetpixel * sin(-angle)) * scalefactor;
      }

      p_x +=  style->offsetx * scalefactor;
      p_y +=  style->offsety * scalefactor;

      if(symbol->anchorpoint_x != 0.5 || symbol->anchorpoint_y != 0.5) {
        double sx,sy;
        double ox, oy;
        msGetMarkerSize(symbolset, style, &sx, &sy, scalefactor);
        ox = (0.5 - symbol->anchorpoint_x) * sx;
        oy = (0.5 - symbol->anchorpoint_y) * sy;
        if(s.rotation != 0) {
          double sina, cosa;
          double rox,roy;
          sina = sin(-s.rotation);
          cosa = cos(-s.rotation);
          rox = ox * cosa - oy * sina;
          roy = ox * sina + oy * cosa;
          p_x += rox;
          p_y += roy;
        } else {
          p_x += ox;
          p_y += oy;
        }
      }

      if(renderer->use_imagecache) {
        imageObj *tile = getTile(image, symbol, &s, -1, -1,0);
        if(tile!=NULL)
          return renderer->renderTile(image, tile, p_x, p_y);
        else {
          msSetError(MS_RENDERERERR, "problem creating cached tile", "msDrawMarkerSymbol()");
          return MS_FAILURE;
        }
      }
      switch (symbol->type) {
        case (MS_SYMBOL_TRUETYPE): {
          assert(symbol->full_font_path);
          ret = renderer->renderTruetypeSymbol(image, p_x, p_y, symbol, &s);

        }
        break;
        case (MS_SYMBOL_PIXMAP): {
          assert(symbol->pixmap_buffer);
          ret = renderer->renderPixmapSymbol(image,p_x,p_y,symbol,&s);
        }
        break;
        case (MS_SYMBOL_ELLIPSE): {
          ret = renderer->renderEllipseSymbol(image, p_x, p_y,symbol, &s);
        }
        break;
        case (MS_SYMBOL_VECTOR): {
          ret = renderer->renderVectorSymbol(image, p_x, p_y, symbol, &s);
        }
        break;
        case (MS_SYMBOL_SVG): {
          if (renderer->supports_svg) {
            ret = renderer->renderSVGSymbol(image, p_x, p_y, symbol, &s);
          } else {
#ifdef USE_SVG_CAIRO
            ret = msRenderRasterizedSVGSymbol(image, p_x,p_y, symbol, &s);
#else
            msSetError(MS_SYMERR, "SVG symbol support is not enabled.", "msDrawMarkerSymbol()");
            return MS_FAILURE;
#endif
          }
        }
        break;
        default:
          break;
      }
      return ret;
    } else if( MS_RENDERER_IMAGEMAP(image->format) )
      msDrawMarkerSymbolIM(symbolset, image, p, style, scalefactor);

  }
  return ret;
}





/*
** Render the text (no background effects) for a label.
 */
int msDrawText(imageObj *image, pointObj labelPnt, char *string,
               labelObj *label, fontSetObj *fontset, double scalefactor)
{
  int nReturnVal = -1;
  if (image) {
    if (MS_RENDERER_PLUGIN(image->format)) {
      labelStyleObj s;
      rendererVTableObj *renderer = image->format->vtable;
      double x, y;
      if (!string || !strlen(string))
        return (0); /* not errors, just don't want to do anything */


      if(computeLabelStyle(&s,label,fontset,scalefactor,image->resolutionfactor) == MS_FAILURE) {
        return MS_FAILURE;
      }
      if(s.rotation == 0 && !MS_RENDERER_KML(image->format)) {
        x = MS_NINT(labelPnt.x);
        y = MS_NINT(labelPnt.y);
      } else {
        x = labelPnt.x;
        y = labelPnt.y;
      }
      if (label->type == MS_TRUETYPE) {
        if(MS_VALID_COLOR(label->shadowcolor)) {
          s.color = &label->shadowcolor;
          /* FIXME labelpoint for rotated label */
          renderer->renderGlyphs(image,
                                 x + scalefactor * label->shadowsizex,y + scalefactor * label->shadowsizey,
                                 &s,string);
        }

        s.color= &label->color;
        if(MS_VALID_COLOR(label->outlinecolor)) {
          s.outlinecolor = &label->outlinecolor;
          s.outlinewidth = label->outlinewidth * s.size/label->size;
        }
        return renderer->renderGlyphs(image,x,y,&s,string);
      } else if(label->type == MS_BITMAP) {
        s.size = MS_NINT(s.size);
        s.color= &label->color;
        s.size = MS_MIN(s.size,5); /* only have 5 bitmap fonts */
        if(!renderer->supports_bitmap_fonts || !renderer->bitmapFontMetrics[MS_NINT(s.size)]) {
          msSetError(MS_RENDERERERR, "selected renderer does not support bitmap fonts or this particular size", "msDrawText()");
          return MS_FAILURE;
        }
        return renderer->renderBitmapGlyphs(image,x,y,&s,string);
      }
    } else if( MS_RENDERER_IMAGEMAP(image->format) )
      nReturnVal = msDrawTextIM(image, labelPnt, string, label, fontset, scalefactor);
  }
  return nReturnVal;
}

int msDrawTextLine(imageObj *image, char *string, labelObj *label, labelPathObj *labelpath, fontSetObj *fontset, double scalefactor)
{
  int nReturnVal = MS_SUCCESS;
  if(image) {
    if (MS_RENDERER_PLUGIN(image->format)) {

      rendererVTableObj *renderer = image->format->vtable;
      labelStyleObj s;
      if (!string || !strlen(string))
        return (MS_SUCCESS); /* not errors, just don't want to do anything */
      if(computeLabelStyle(&s, label, fontset, scalefactor,image->resolutionfactor) != MS_SUCCESS) return MS_FAILURE;
      if (label->type == MS_TRUETYPE) {
        if(renderer->renderGlyphsLine) {
          if(MS_VALID_COLOR(label->outlinecolor)) {
            s.outlinecolor = &(label->outlinecolor);
            s.outlinewidth = s.size/label->size * label->outlinewidth;
          } else {
            s.outlinewidth = 0;
            s.outlinecolor = NULL;
          }
          s.color = &(label->color);
          nReturnVal = renderer->renderGlyphsLine(image,labelpath,&s,string);
        } else {
          /*
           * if the renderer doesn't support direct labelPath usage,
           * decompose the string and send it down character by character
           */
          const char* string_ptr = string;
          int i;
          double x, y;
          char glyph[11];

          /*
           * we first render all the outlines if present, so that the
           * joining of characters stays correct
           */
          if(MS_VALID_COLOR(label->outlinecolor)) {
            s.outlinecolor = &(label->outlinecolor);
            s.outlinewidth = s.size/label->size * label->outlinewidth;
            for (i = 0; i < labelpath->path.numpoints; i++) {
              if (msGetNextGlyph(&string_ptr, glyph) == -1)
                break; /* Premature end of string??? */
              s.rotation = labelpath->angles[i];
              x = labelpath->path.point[i].x;
              y = labelpath->path.point[i].y;
              nReturnVal = renderer->renderGlyphs(image, x, y, &s, glyph);
              if(nReturnVal != MS_SUCCESS) {
                return nReturnVal;
              }
            }
            string_ptr = string; /* reset to beginning of string */
          }
          s.outlinecolor = NULL;
          s.outlinewidth = 0;
          s.color = &(label->color);
          for (i = 0; i < labelpath->path.numpoints; i++) {
            if (msGetNextGlyph(&string_ptr, glyph) == -1)
              break; /* Premature end of string??? */

            s.rotation = labelpath->angles[i];
            x = labelpath->path.point[i].x;
            y = labelpath->path.point[i].y;

            nReturnVal = renderer->renderGlyphs(image, x, y, &s, glyph);
            if(nReturnVal != MS_SUCCESS) {
              return nReturnVal;
            }
          }
        }
      }
    }
  }

  return nReturnVal;
}


/************************************************************************/
/*                          msCircleDrawLineSymbol                      */
/*                                                                      */
/************************************************************************/
int msCircleDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor)
{
  shapeObj *circle;
  if (!image) return MS_FAILURE;
  circle = msRasterizeArc(p->x, p->y, r, 0, 360, 0);
  if (!circle) return MS_FAILURE;
  msDrawLineSymbol(symbolset, image, circle, style, scalefactor);
  msFreeShape(circle);
  msFree(circle);
  return MS_SUCCESS;
}

int msCircleDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor)
{
  shapeObj *circle;
  if (!image) return MS_FAILURE;
  circle = msRasterizeArc(p->x, p->y, r, 0, 360, 0);
  if (!circle) return MS_FAILURE;
  msDrawShadeSymbol(symbolset, image, circle, style, scalefactor);
  msFreeShape(circle);
  msFree(circle);
  return MS_SUCCESS;
}

int msDrawPieSlice(symbolSetObj *symbolset, imageObj *image, pointObj *p, styleObj *style, double radius, double start, double end)
{

  shapeObj *circle;
  double center_x = p->x;
  double center_y = p->y;
  if (!image) return MS_FAILURE;
  if(style->offsetx>0) {
    center_x+=style->offsetx*cos(((-start-end)*MS_PI/360.));
    center_y-=style->offsetx*sin(((-start-end)*MS_PI/360.));
  }
  circle = msRasterizeArc(center_x, center_y, radius, start, end, 1);
  if (!circle) return MS_FAILURE;
  msDrawShadeSymbol(symbolset, image, circle, style, 1.0);
  msFreeShape(circle);
  msFree(circle);
  return MS_SUCCESS;
}

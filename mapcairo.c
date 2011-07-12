/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Cairo Rendering functions
 * Author:   Thomas Bonfort
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

#ifdef USE_CAIRO

#include <cairo.h>
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <cairo-pdf.h>
#include <cairo-svg.h>
#else
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>
#endif

# include <cairo-ft.h>
/*
#include <pango/pangocairo.h>
#include <glib.h>
*/
#include <string.h>
/*
#include <ft2build.h>
#include FT_FREETYPE_H
*/




/* trivial cache container */
struct facecache;
typedef struct facecache faceCacheObj;
struct facecache{
    cairo_font_face_t *face;
    FT_Face ftface;
    char *path;
    faceCacheObj *next;
};

int freeFaceCache(faceCacheObj *fc) {
    //free(fc->path);
    //cairo_font_face_destroy(fc->face);
    return MS_SUCCESS;
}

typedef struct {
	cairo_surface_t *surface;
    cairo_t *cr;
    FT_Library library;
    bufferObj *outputStream;
    faceCacheObj *facecache;
} cairo_renderer;



cairo_renderer *getCairoRenderer(imageObj *img) {
	return (cairo_renderer*)img->img.plugin;
}


inline int freeFontBackendCairo(cairo_renderer *r) {
    if(r->library)
        FT_Done_FreeType(r->library);
    if(r->facecache) {
        faceCacheObj *next,*cur;
        cur = r->facecache;
        do {
            next = cur->next;
            //freeFaceCache(cur);
            free(cur);
            cur=next;
        } while(cur);
    } 
    return MS_SUCCESS;
}

void freeImageCairo(imageObj *img) {
    cairo_renderer *r = getCairoRenderer(img);
    if(r) {
        cairo_surface_destroy(r->surface);
        cairo_destroy(r->cr);
        if(r->outputStream) {
            msBufferFree(r->outputStream);
            free(r->outputStream);
        }
        freeFontBackendCairo(r);
        msFree(r);
    }
    img->img.plugin=NULL;
}

static const cairo_user_data_key_t facekey;

cairo_font_face_t *getFontFace(cairo_renderer *r, char *font) {
    faceCacheObj *newface = NULL;
    faceCacheObj *cur=r->facecache;
    while(cur) {
        if(cur->path == font) return cur->face;
        cur = cur->next;
    }
    newface = malloc(sizeof(faceCacheObj));
    newface->next = r->facecache;
    r->facecache = newface;
    
    FT_New_Face(r->library, font, 0, &(newface->ftface));
    newface->face = cairo_ft_font_face_create_for_ft_face(newface->ftface, 0);

    cairo_font_face_set_user_data (newface->face, &facekey,
            &(newface->ftface), (cairo_destroy_func_t) FT_Done_Face);

    newface->path = font;
    return newface->face;
}

inline void msCairoSetSourceColor(cairo_t *cr, colorObj *c) {
	cairo_set_source_rgba(cr,c->red/255.0,c->green/255.0,c->blue/255.0,c->alpha/255.0);
}

void renderLineCairo(imageObj *img, shapeObj *p, strokeStyleObj *stroke) {
	cairo_renderer *r = getCairoRenderer(img);
	int i,j;
    cairo_new_path(r->cr);
	msCairoSetSourceColor(r->cr,&stroke->color);
	for(i=0;i<p->numlines;i++) {
		lineObj *l = &(p->line[i]);
		cairo_move_to(r->cr,l->point[0].x,l->point[0].y);
		for(j=1;j<l->numpoints;j++) {
			cairo_line_to(r->cr,l->point[j].x,l->point[j].y);
		}
	}
	if(stroke->patternlength>0) {
		cairo_set_dash(r->cr,stroke->pattern,stroke->patternlength,0);
	}
    switch(stroke->linecap) {
        case MS_CJC_BUTT:
            cairo_set_line_cap(r->cr,CAIRO_LINE_CAP_BUTT);
            break;
        case MS_CJC_SQUARE:
            cairo_set_line_cap(r->cr,CAIRO_LINE_CAP_SQUARE);
            break;
        case MS_CJC_ROUND:
        case MS_CJC_NONE:
        default:
            cairo_set_line_cap(r->cr,CAIRO_LINE_CAP_ROUND);
    }
	cairo_set_line_width (r->cr, stroke->width);
	cairo_stroke (r->cr);
	if(stroke->patternlength>0) {
		cairo_set_dash(r->cr,stroke->pattern,0,0);
	}
}

void renderPolygonCairo(imageObj *img, shapeObj *p, colorObj *c) {
	cairo_renderer *r = getCairoRenderer(img);
	int i,j;
    cairo_new_path(r->cr);
    cairo_set_fill_rule(r->cr,CAIRO_FILL_RULE_EVEN_ODD);
	msCairoSetSourceColor(r->cr,c);
	for(i=0;i<p->numlines;i++) {
		lineObj *l = &(p->line[i]);
		cairo_move_to(r->cr,l->point[0].x,l->point[0].y);
		for(j=1;j<l->numpoints;j++) {
			cairo_line_to(r->cr,l->point[j].x,l->point[j].y);
		}
		cairo_close_path(r->cr);
	}
	cairo_fill(r->cr);
}

void freeTileCairo(imageObj *tile) {
    freeImageCairo(tile);
}

void renderPolygonTiledCairo(imageObj *img, shapeObj *p,  imageObj *tile) {
    int i,j;
    cairo_renderer *r = getCairoRenderer(img);
    cairo_pattern_t *pattern = cairo_pattern_create_for_surface(tile);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
	
    cairo_set_source(r->cr, pattern);
    for (i = 0; i < p->numlines; i++) {
      lineObj *l = &(p->line[i]);
      cairo_move_to(r->cr, l->point[0].x, l->point[0].y);
      for (j = 1; j < l->numpoints; j++) {
        cairo_line_to(r->cr, l->point[j].x, l->point[j].y);
      }
      //cairo_close_path(r->cr);
    }
    cairo_fill(r->cr);
}

cairo_surface_t *createSurfaceFromBuffer(rasterBufferObj *b) {
    //TODO check for consistency;
    return cairo_image_surface_create_for_data (b->pixelbuffer,
        CAIRO_FORMAT_ARGB32,
        b->width,
        b->height,
        b->row_step);
}


void renderPixmapSymbolCairo(imageObj *img, double x, double y,symbolObj *symbol,
        symbolStyleObj *style) {
	cairo_renderer *r = getCairoRenderer(img);
	cairo_surface_t *im;
    rasterBufferObj *b = symbol->pixmap_buffer;
	im=createSurfaceFromBuffer(b);
	cairo_save(r->cr);
	cairo_translate (r->cr, x,y);
	cairo_rotate (r->cr, -style->rotation);
	cairo_scale  (r->cr, style->scale,style->scale);
	cairo_translate (r->cr, -0.5*b->width, -0.5*b->height);
	cairo_set_source_surface (r->cr, im, 0, 0);
	cairo_paint (r->cr);
	cairo_restore(r->cr);
}

void renderVectorSymbolCairo(imageObj *img, double x, double y, symbolObj *symbol,
		symbolStyleObj *style) {
	cairo_renderer *r = getCairoRenderer(img);
	cairo_path_t *sym;
	double ox=symbol->sizex*0.5,oy=symbol->sizey*0.5;
	if (symbol->renderer_cache == NULL) {
		int is_new = 1,i;
        cairo_new_path(r->cr);
		for (i = 0; i < symbol->numpoints; i++) {
			if ((symbol->points[i].x == -99) && (symbol->points[i].y == -99)) { // (PENUP)
				is_new = 1;
			} else {
				if (is_new) {
					cairo_move_to(r->cr,symbol->points[i].x-ox,symbol->points[i].y-oy);
					is_new = 0;
				} else {
					cairo_line_to(r->cr,symbol->points[i].x-ox,symbol->points[i].y-oy);
				}
			}
		}
		symbol->renderer_cache = (void*)cairo_copy_path(r->cr);
		cairo_new_path(r->cr);
	}
	sym=(cairo_path_t*)symbol->renderer_cache;
	cairo_save(r->cr);
	cairo_translate(r->cr,x-ox,y-oy);
	cairo_scale(r->cr,style->scale,style->scale);
	cairo_rotate(r->cr,-style->rotation);
	cairo_append_path(r->cr,sym);
	cairo_restore(r->cr);
	if(MS_VALID_COLOR(style->color)) {
		msCairoSetSourceColor(r->cr,&style->color);
		cairo_fill_preserve(r->cr);
	}
	if(style->outlinewidth>0) {
		msCairoSetSourceColor(r->cr,&style->outlinecolor);
		cairo_set_line_width (r->cr, style->outlinewidth);
		cairo_stroke_preserve(r->cr);
	}
	cairo_new_path(r->cr);
}

void renderTruetypeSymbolCairo(imageObj *img, double x, double y, symbolObj *symbol,
        symbolStyleObj *s) {
    
    cairo_renderer *r = getCairoRenderer(img);
    cairo_font_face_t *ff = getFontFace(r,symbol->full_font_path);

    int unicode;
    cairo_glyph_t glyph;
    cairo_text_extents_t extents;
    FT_Face *f;

    cairo_matrix_t trans;
    double ox,oy;
    cairo_save(r->cr); 
    cairo_set_font_face(r->cr,ff);
    cairo_set_font_size(r->cr,s->scale*96/72.0);


    f = cairo_font_face_get_user_data(
            cairo_get_font_face(r->cr),
            &facekey);
    msUTF8ToUniChar(symbol->character, &unicode);
    glyph.index = FT_Get_Char_Index(*f, unicode);
    cairo_glyph_extents(r->cr,&glyph,1,&extents);
	ox=extents.x_bearing+extents.width/2.;
	oy=extents.y_bearing+extents.height/2.;
    glyph.x=0;
    glyph.y=0;


	cairo_matrix_init_rotate(&trans,-s->rotation);

	cairo_matrix_transform_point(&trans,&ox,&oy);
	//cairo_translate(cr,-extents.width/2,-extents.height/2);

	cairo_translate(r->cr,x-ox,y-oy);
	cairo_rotate(r->cr, -s->rotation);
	
    cairo_glyph_path(r->cr,&glyph,1);
	
	if (s->outlinewidth) {
		msCairoSetSourceColor(r->cr, &s->outlinecolor);
		cairo_set_line_width(r->cr, s->outlinewidth);
		cairo_stroke_preserve(r->cr);
	}

	msCairoSetSourceColor(r->cr, &s->color);
	cairo_fill(r->cr);
    cairo_restore(r->cr);
}

void renderTileCairo(imageObj *img, imageObj *tile, double x, double y) {
	cairo_renderer *r = getCairoRenderer(img);
    cairo_surface_t *im = getCairoRenderer(tile)->surface;
	int w = cairo_image_surface_get_width (im);
	int h = cairo_image_surface_get_height (im);
	cairo_save(r->cr);
	cairo_translate(r->cr, MS_NINT(x-0.5 * w), MS_NINT(y -0.5 * h));
	cairo_set_source_surface(r->cr, im, 0, 0);
	cairo_pattern_set_filter (cairo_get_source (r->cr), CAIRO_FILTER_NEAREST);
	cairo_paint(r->cr);
	cairo_restore(r->cr);
}

#define CAIROLINESPACE 1.33

int getTruetypeTextBBoxCairo(imageObj *img,char *font, double size, char *text,
        		rectObj *rect, double **advances) {

    cairo_renderer *r = getCairoRenderer(img);
    cairo_font_face_t *ff = getFontFace(r,font);

    char *utfptr=text;
    int i,has_kerning,unicode;
    unsigned long previdx=0;
    int numglyphs = msGetNumGlyphs(text);
    cairo_glyph_t glyph;
    cairo_text_extents_t extents;
    double px=0,py=0;
    FT_Face *f;

    cairo_set_font_face(r->cr,ff);
    cairo_set_font_size(r->cr,size*96/72.0);

    f = cairo_font_face_get_user_data(
            cairo_get_font_face(r->cr),
            &facekey);
    has_kerning = FT_HAS_KERNING((*f));

    if(advances != NULL) {
        *advances = (double*)malloc(numglyphs*sizeof(double));
    }

    for(i=0;i<numglyphs;i++) {
        utfptr+=msUTF8ToUniChar(utfptr, &unicode);
        glyph.x=px;
        glyph.y=py;
        if(unicode=='\n') {
            py += ceil(size*CAIROLINESPACE);
            px = 0;
            previdx=0;
            continue;
        }
        glyph.index = FT_Get_Char_Index(*f, unicode);
        if( has_kerning && previdx ) {
            FT_Vector delta;
            FT_Get_Kerning( *f, previdx, glyph.index, FT_KERNING_DEFAULT, &delta );
            px += delta.x / 64.;
        }
        cairo_glyph_extents(r->cr,&glyph,1,&extents);
        
		if(i==0) {
			rect->minx = px+extents.x_bearing;
			rect->miny = py+extents.y_bearing;
			rect->maxx = px+extents.x_bearing+extents.width;
			rect->maxy = px+extents.y_bearing+extents.height;
		} else {
			rect->minx = MS_MIN(rect->minx,px+extents.x_bearing);
			rect->miny = MS_MIN(rect->miny,px+extents.y_bearing);
			rect->maxy = MS_MAX(rect->maxy,py+extents.y_bearing+extents.height);
			rect->maxx = MS_MAX(rect->maxx,px+extents.x_bearing+extents.width);
		}
        if(advances!=NULL)
            (*advances)[i]=extents.x_advance;
        px += extents.x_advance;
        previdx=glyph.index;
    }
	return MS_SUCCESS;
}

void renderGlyphsCairo(imageObj *img,double x, double y, labelStyleObj *style, char *text) {
    cairo_renderer *r = getCairoRenderer(img);

    cairo_font_face_t *ff = getFontFace(r,style->font);

    char *utfptr=text;
    int i,has_kerning,unicode;
    unsigned long previdx=0;
    int numglyphs = msGetNumGlyphs(text);
    cairo_glyph_t glyph;
    cairo_text_extents_t extents;
    double px=0,py=0;
    FT_Face *f;

    cairo_set_font_face(r->cr,ff);
    cairo_set_font_size(r->cr,style->size*96/72.0);

    cairo_save(r->cr);
    cairo_translate(r->cr,x,y);
    cairo_rotate(r->cr, -style->rotation);

    f = cairo_font_face_get_user_data(
            cairo_get_font_face(r->cr),
            &facekey);
    has_kerning = FT_HAS_KERNING((*f));
    for(i=0;i<numglyphs;i++) {
        utfptr+=msUTF8ToUniChar(utfptr, &unicode);
        glyph.x=px;
        glyph.y=py;
        if(unicode=='\n') {
            py += ceil(style->size*CAIROLINESPACE);
            px = 0;
            previdx=0;
            continue;
        }
        glyph.index = FT_Get_Char_Index(*f, unicode);
        if( has_kerning && previdx ) {
            FT_Vector delta;
            FT_Get_Kerning( *f, previdx, glyph.index, FT_KERNING_DEFAULT, &delta );
            px += delta.x / 64.;
        }
        cairo_glyph_extents(r->cr,&glyph,1,&extents);
        cairo_glyph_path(r->cr,&glyph,1);
        px += extents.x_advance;
        previdx=glyph.index;
    }

    /* NOT SUPPORTED
       if (style->shadowsizex !=0 || style->shadowsizey != 0) {
       cairo_save(r->cr);
       msCairoSetSourceColor(r->cr, &style->shadowcolor);
       cairo_rel_move_to(r->cr, style->shadowsizex, style->shadowsizey);
       cairo_fill(r->cr);
       cairo_restore(r->cr);
       }*/


    if (style->outlinewidth > 0) {
        cairo_save(r->cr);
        msCairoSetSourceColor(r->cr, &style->outlinecolor);
        cairo_set_line_width(r->cr, style->outlinewidth);
        cairo_stroke_preserve(r->cr);
        cairo_restore(r->cr);
    }

    msCairoSetSourceColor(r->cr, &style->color);
    cairo_fill(r->cr);
    cairo_restore(r->cr);
    return;
}


void renderGlyphsLineCairo(imageObj *img, labelPathObj *labelpath,
        labelStyleObj *style, char *text)
{

    cairo_renderer *r = getCairoRenderer(img);

    cairo_font_face_t *ff = getFontFace(r,style->font);

    char *utfptr=text;
    int i,unicode;
    cairo_glyph_t glyph;
    FT_Face *f;

    cairo_set_font_face(r->cr,ff);
    cairo_set_font_size(r->cr,style->size*96/72.0);


    f = cairo_font_face_get_user_data(
            cairo_get_font_face(r->cr),
            &facekey);
    for(i=0;i<labelpath->path.numpoints;i++) {
        cairo_save(r->cr);

        cairo_translate(r->cr,labelpath->path.point[i].x,labelpath->path.point[i].y);
        cairo_rotate(r->cr, -labelpath->angles[i]);
        utfptr+=msUTF8ToUniChar(utfptr, &unicode);
        glyph.x=0;
        glyph.y=0;
        if(unicode=='\n') {
            continue;
        }
        glyph.index = FT_Get_Char_Index(*f, unicode);
        cairo_glyph_path(r->cr,&glyph,1);
        cairo_restore(r->cr);
    }

    /* NOT SUPPORTED
       if (style->shadowsizex !=0 || style->shadowsizey != 0) {
       cairo_save(r->cr);
       msCairoSetSourceColor(r->cr, &style->shadowcolor);
       cairo_rel_move_to(r->cr, style->shadowsizex, style->shadowsizey);
       cairo_fill(r->cr);
       cairo_restore(r->cr);
       }*/


    if (style->outlinewidth > 0) {
        cairo_save(r->cr);
        msCairoSetSourceColor(r->cr, &style->outlinecolor);
        cairo_set_line_width(r->cr, style->outlinewidth);
        cairo_stroke_preserve(r->cr);
        cairo_restore(r->cr);
    }

    msCairoSetSourceColor(r->cr, &style->color);
    cairo_fill(r->cr);
    return;
    
}


cairo_status_t _stream_write_fn(void *b, const unsigned char *data, unsigned int length) {
    msBufferAppend((bufferObj*)b,(void*)data,length);
    return CAIRO_STATUS_SUCCESS;
}

imageObj* createImageCairo(int width, int height, outputFormatObj *format,colorObj* bg) {
	imageObj *image = NULL;
        cairo_renderer *r=NULL;
	if (format->imagemode != MS_IMAGEMODE_RGB && format->imagemode!= MS_IMAGEMODE_RGBA) {
		msSetError(MS_MISCERR,
		"Cairo driver only supports RGB or RGBA pixel models.","msImageCreateCairo()");
		return image;
	}
	if (width > 0 && height > 0) {
		image = (imageObj *) calloc(1, sizeof(imageObj));
		r = (cairo_renderer*)calloc(1,sizeof(cairo_renderer));
		if(!strcasecmp(format->driver,"cairo/pdf")) {
            r->outputStream = (bufferObj*)malloc(sizeof(bufferObj));
            msBufferInit(r->outputStream);
			r->surface = cairo_pdf_surface_create_for_stream(
                    _stream_write_fn,
                    r->outputStream,
                    width,height);
		} else if(!strcasecmp(format->driver,"cairo/svg")) {
			r->outputStream = (bufferObj*)malloc(sizeof(bufferObj));
            msBufferInit(r->outputStream);
            r->surface = cairo_svg_surface_create_for_stream(
                    _stream_write_fn,
                    r->outputStream,
                    width,height);
        }
		else {
            r->outputStream = NULL;
			if(format->imagemode == MS_IMAGEMODE_RGBA) {
				r->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
			} else {
				r->surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);
			}
		}
		r->cr = cairo_create(r->surface);

        FT_Init_FreeType(&(r->library));
		
        if(format->imagemode == MS_IMAGEMODE_RGBA) {
			cairo_save (r->cr);
			cairo_set_source_rgba (r->cr, 0,0,0,0);
			cairo_set_operator (r->cr, CAIRO_OPERATOR_SOURCE);
			cairo_paint (r->cr);
			cairo_restore (r->cr);
		} else {
			bg->alpha=255;
			msCairoSetSourceColor(r->cr,bg);
			cairo_paint(r->cr);
		}
		cairo_set_line_cap (r->cr,CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(r->cr,CAIRO_LINE_JOIN_ROUND);
		image->img.plugin = (void*)r;
	} else {
		msSetError(MS_IMGERR, "Cannot create GD image of size %dx%d.",
			"msImageCreateCairo()", width, height);
	}
	return image;
}

int saveImageCairo(imageObj *img, FILE *fp, outputFormatObj *format) {
	cairo_renderer *r = getCairoRenderer(img);
	if(!strcasecmp(img->format->driver,"cairo/pdf") || !strcasecmp(img->format->driver,"cairo/svg")) {
        cairo_surface_finish (r->surface);
        fwrite(r->outputStream->data,r->outputStream->size,1,fp);
	} else {
        // not supported
	}
	return MS_SUCCESS;
}

void *msCreateTileEllipseCairo(double width, double height, double angle,
		colorObj *c, colorObj *bc, colorObj *oc, double ow) {
	double s = (MS_MAX(width,height)+ow)*1.5;
	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
			s,s);
	cairo_t *cr = cairo_create(surface);
	//cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	//cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	if (bc && MS_VALID_COLOR(*bc)) {
		msCairoSetSourceColor(cr, bc);
		cairo_paint(cr);
	}
	cairo_save(cr);
	cairo_translate(cr,s/2,s/2);
	cairo_rotate(cr, -angle);
	cairo_scale(cr, width/2,height/2);
	cairo_arc(cr, 0, 0, 1, 0, 2 * MS_PI);
	cairo_restore(cr);
	if (c != NULL && MS_VALID_COLOR(*c)) {
		msCairoSetSourceColor(cr, c);
		cairo_fill_preserve(cr);
	}
	if (oc != NULL && MS_VALID_COLOR(*oc)) {
		cairo_set_line_width(cr, ow);
		msCairoSetSourceColor(cr, oc);
		cairo_stroke_preserve(cr);
	}
	cairo_new_path(cr);
	cairo_destroy(cr);
	return surface;

}

void renderEllipseSymbolCairo(imageObj *img, double x, double y, symbolObj *symbol,
        symbolStyleObj *style) {
	static cairo_path_t *arc=NULL;
	cairo_renderer *r = getCairoRenderer(img);
	
    //TODO memory leak on arc here, use the symbol's renderer_cache
    if(arc==NULL) {
		cairo_save(r->cr);
		cairo_set_line_cap(r->cr, CAIRO_LINE_CAP_BUTT);
		cairo_set_line_join(r->cr, CAIRO_LINE_JOIN_MITER);
		cairo_arc (r->cr, 0,0,1, 0, 2 * MS_PI);
		arc = cairo_copy_path(r->cr);
		cairo_new_path(r->cr);
		cairo_restore(r->cr);
	}
	cairo_save(r->cr);
	cairo_set_line_cap(r->cr, CAIRO_LINE_CAP_BUTT);
	cairo_set_line_join(r->cr, CAIRO_LINE_JOIN_MITER);
	cairo_translate(r->cr,x,y);
	cairo_rotate(r->cr,-style->rotation);
	cairo_scale(r->cr,symbol->sizex*style->scale,symbol->sizey*style->scale);
	cairo_append_path(r->cr,arc);
	cairo_restore(r->cr);
	if(MS_VALID_COLOR(style->color)) {
		msCairoSetSourceColor(r->cr, &style->color);
		cairo_fill_preserve(r->cr);
	}
	if(style->outlinewidth > 0) {
		cairo_set_line_width (r->cr, style->outlinewidth);
		msCairoSetSourceColor(r->cr, &style->outlinecolor);
		cairo_stroke_preserve(r->cr);
	}
	cairo_new_path(r->cr);
}



void startNewLayerCairo(imageObj *img, double opacity) {
	cairo_renderer *r = getCairoRenderer(img);
	cairo_push_group (r->cr);
}

void closeNewLayerCairo(imageObj *img, double opacity) {
	cairo_renderer *r = getCairoRenderer(img);
	cairo_pop_group_to_source (r->cr);
	cairo_paint_with_alpha (r->cr, opacity);
}


void getRasterBufferCairo(imageObj *img, rasterBufferObj *rb) {
	cairo_renderer *r = getCairoRenderer(img);
    unsigned char *pb = cairo_image_surface_get_data(r->surface);
    rb->pixelbuffer = pb;
    rb->row_step = cairo_image_surface_get_stride(r->surface);
    rb->pixel_step=4;
    rb->width = cairo_image_surface_get_width(r->surface);
    rb->height = cairo_image_surface_get_height(r->surface);
    rb->r = &(pb[2]);
    rb->g = &(pb[1]);
    rb->b = &(pb[0]);
    if(cairo_image_surface_get_format(r->surface) == CAIRO_FORMAT_ARGB32) {
        rb->a = &(pb[3]);
    } else {
        rb->a = NULL;
    }
}

void mergeRasterBufferCairo(imageObj *img, rasterBufferObj *rb, double opacity,
        int dstX, int dstY) {
    cairo_surface_t *src;
    cairo_format_t format;
    cairo_renderer *r = getCairoRenderer(img);
    if(rb->a)
        format = CAIRO_FORMAT_ARGB32;
    else
        format = CAIRO_FORMAT_RGB24;
    src = cairo_image_surface_create_for_data(rb->pixelbuffer,format,
            rb->width,rb->height,
            rb->row_step);
	cairo_set_source_surface (r->cr,src,0,0);
	cairo_paint_with_alpha(r->cr,opacity);
}

void freeSymbolCairo(symbolObj *s) {
	if(!s->renderer_cache)
		return;
	switch(s->type) {
	case MS_SYMBOL_VECTOR:
		cairo_path_destroy(s->renderer_cache);
		break;
	case MS_SYMBOL_PIXMAP:
		cairo_surface_destroy(s->renderer_cache);
		break;
	}
	s->renderer_cache=NULL;
}

#endif /*USE_CAIRO*/


int msPopulateRendererVTableCairoRaster( rendererVTableObj *renderer ) {
#ifdef USE_CAIRO
    renderer->supports_imagecache=0;
    renderer->supports_pixel_buffer=1;
    renderer->supports_transparent_layers = 1;
    renderer->startNewLayer = startNewLayerCairo;
    renderer->closeNewLayer = closeNewLayerCairo;
    renderer->renderLine=&renderLineCairo;
    renderer->createImage=&createImageCairo;
    renderer->saveImage=&saveImageCairo;
    renderer->getRasterBuffer=&getRasterBufferCairo;
    renderer->transformShape=&msTransformShapeAGG;
    renderer->renderPolygon=&renderPolygonCairo;
    renderer->renderGlyphsLine=&renderGlyphsLineCairo;
    renderer->renderGlyphs=&renderGlyphsCairo;
    renderer->freeImage=&freeImageCairo;
    renderer->renderEllipseSymbol = &renderEllipseSymbolCairo;
    renderer->renderVectorSymbol = &renderVectorSymbolCairo;
    renderer->renderTruetypeSymbol = &renderTruetypeSymbolCairo;
    renderer->renderPixmapSymbol = &renderPixmapSymbolCairo;
    renderer->mergeRasterBuffer = &mergeRasterBufferCairo;
    renderer->getTruetypeTextBBox = &getTruetypeTextBBoxCairo;
    renderer->renderTile = &renderTileCairo;
    renderer->renderPolygonTiled = &renderPolygonTiledCairo;
    renderer->freeTile = &freeTileCairo;
    renderer->freeSymbol = &freeSymbolCairo;
    return MS_SUCCESS;
#else
    msSetError(MS_MISCERR, "Cairo Driver requested but is not built in", 
            "msPopulateRendererVTableCairoRaster()");
    return MS_FAILURE;
#endif
}

inline int populateRendererVTableCairoVector( rendererVTableObj *renderer ) {
#ifdef USE_CAIRO
    renderer->supports_imagecache=0;
    renderer->supports_pixel_buffer=0;
    renderer->supports_transparent_layers = 1;
    renderer->startNewLayer = startNewLayerCairo;
    renderer->closeNewLayer = closeNewLayerCairo;
    renderer->renderLine=&renderLineCairo;
    renderer->createImage=&createImageCairo;
    renderer->saveImage=&saveImageCairo;
    renderer->getRasterBuffer=&getRasterBufferCairo;
    renderer->transformShape=&msTransformShapeAGG;
    renderer->renderPolygon=&renderPolygonCairo;
    renderer->renderGlyphsLine=&renderGlyphsLineCairo;
    renderer->renderGlyphs=&renderGlyphsCairo;
    renderer->freeImage=&freeImageCairo;
    renderer->renderEllipseSymbol = &renderEllipseSymbolCairo;
    renderer->renderVectorSymbol = &renderVectorSymbolCairo;
    renderer->renderTruetypeSymbol = &renderTruetypeSymbolCairo;
    renderer->renderPixmapSymbol = &renderPixmapSymbolCairo;
    renderer->mergeRasterBuffer = &mergeRasterBufferCairo;
    renderer->getTruetypeTextBBox = &getTruetypeTextBBoxCairo;
    renderer->renderTile = &renderTileCairo;
    renderer->renderPolygonTiled = &renderPolygonTiledCairo;
    renderer->freeTile = &freeTileCairo;
    renderer->freeSymbol = &freeSymbolCairo;
    return MS_SUCCESS;
#else
    msSetError(MS_MISCERR, "Cairo Driver requested but is not built in", 
            "msPopulateRendererVTableCairoRaster()");
    return MS_FAILURE;
#endif
}

int msPopulateRendererVTableCairoSVG( rendererVTableObj *renderer ) {
    return populateRendererVTableCairoVector(renderer);
}
int msPopulateRendererVTableCairoPDF( rendererVTableObj *renderer ) {
    return populateRendererVTableCairoVector(renderer);
}


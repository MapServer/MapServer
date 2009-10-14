#include "mapserver.h"
#include "mapcopy.h"

void initSymbolStyle(symbolStyleObj *ss) {
    ss->outlinewidth = 0;
    MS_INIT_COLOR(ss->color,-1,-1,-1);
    MS_INIT_COLOR(ss->outlinecolor,-1,-1,-1); 
    MS_INIT_COLOR(ss->backgroundcolor,-1,-1,-1);
    ss->scale = 1.0;
    ss->rotation = 0;
}

void initLabelStyle(labelStyleObj *ls) {
    ls->font = NULL;
    ls->size = 0;
    ls->rotation = 0;
    MS_INIT_COLOR(ls->color,-1,-1,-1);
    ls->outlinewidth = 0;
    MS_INIT_COLOR(ls->outlinecolor,-1,-1,-1);
    ls->shadowsizex = 0;
    ls->shadowsizey = 0;
    MS_INIT_COLOR(ls->shadowcolor,-1,-1,-1);
}


void initStrokeStyle(strokeStyleObj *ss) {
    ss->width = 0; 
    ss->patternlength = 0;
    MS_INIT_COLOR(ss->color,-1,-1,-1);
    ss->linecap = MS_CJC_ROUND;
    ss->linejoin = MS_CJC_ROUND;
}

int computeLabelStyle(labelStyleObj *s, labelObj *l, fontSetObj *fontset,
        double scalefactor) {
    initLabelStyle(s);
    if(!MS_VALID_COLOR(l->color))
        return MS_FAILURE;
    if(l->size == -1)
        return MS_FAILURE;
    s->size = l->size * scalefactor;
    s->size = MS_MAX(s->size, l->minsize);
    s->size = MS_MIN(s->size, l->maxsize);


    if(l->type == MS_TRUETYPE) {
        if (!fontset) {
            msSetError(MS_TTFERR, "No fontset defined.","msDrawText()");
            return (MS_FAILURE);
	    }

	    if (!l->font) {
            msSetError(MS_TTFERR, "No Trueype font defined.","msDrawText()");
            return (MS_FAILURE);
	    }

	    s->font = msLookupHashTable(&(fontset->fonts), l->font);
	    if (!s->font) {
	    	msSetError(MS_TTFERR, "Requested font (%s) not found.",
	    			"msDrawText()", l->font);
	    	return (MS_FAILURE);
	    }
        if(MS_VALID_COLOR(l->outlinecolor)) {
            MS_COPYCOLOR(&s->outlinecolor,&l->outlinecolor);
            s->outlinecolor.alpha=255;
            s->outlinewidth = s->size/l->size * l->outlinewidth;
        }
    }
    MS_COPYCOLOR(&s->color,&l->color);
    s->color.alpha=255;
    if(MS_VALID_COLOR(l->shadowcolor)) {
        MS_COPYCOLOR(&s->shadowcolor,&l->shadowcolor);
        l->shadowcolor.alpha=255;
        s->shadowsizex = scalefactor * l->shadowsizex;
        s->shadowsizey = scalefactor * l->shadowsizey;
    }
    s->rotation = l->angle * MS_DEG_TO_RAD;
    return MS_SUCCESS;
}
void computeSymbolStyle(symbolStyleObj *s, styleObj *src, symbolObj *symbol, double scalefactor) {
    int alpha;
    double default_size = msSymbolGetDefaultSize(symbol);
    double target_size = (src->size==-1)?default_size:src->size;
    
    if(symbol->filled || symbol->type == MS_SYMBOL_TRUETYPE) {
        MS_COPYCOLOR(&(s->color), &(src->color));
        MS_COPYCOLOR(&(s->outlinecolor),&(src->outlinecolor));
    } else {
        if(MS_VALID_COLOR(s->color))
            MS_COPYCOLOR(&(s->outlinecolor),&(src->color));
        else
            MS_COPYCOLOR(&(s->outlinecolor),&(src->outlinecolor));
        s->color.red = -1;
    }

    MS_COPYCOLOR(&(s->backgroundcolor), &(src->backgroundcolor));
    
    target_size *= scalefactor;
    target_size = MS_MAX(target_size, src->minsize);
    target_size = MS_MIN(target_size, src->maxsize);
    s->scale = target_size / default_size;
    
    if(MS_VALID_COLOR(s->outlinecolor)) {
        s->outlinewidth =  src->width * scalefactor;
        s->outlinewidth = MS_MAX(s->outlinewidth, src->minwidth);
        s->outlinewidth = MS_MIN(s->outlinewidth, src->maxwidth);
    } else {
        s->outlinewidth = 0;
    }
    
    s->rotation = src->angle * MS_DEG_TO_RAD;
    
    alpha = MS_NINT(src->opacity*2.55);
    s->color.alpha = alpha;
    s->outlinecolor.alpha = alpha;
    s->backgroundcolor.alpha = alpha;
}


#define COMPARE_COLORS(a,b) \
    ((a).red==(b).red) && \
    ((a).green==(b).green) && \
    ((a).blue==(b).blue) && \
    ((a).alpha==(b).alpha)

tileCacheObj *searchTileCache(imageObj *img, symbolObj *symbol, symbolStyleObj *s, int width, int height) {
    tileCacheObj *cur = img->tilecache;
    symbolStyleObj *c;
    while(cur != NULL) {
        c = &cur->style;
        if( cur->width == width
                && cur->height == height
                && cur->symbol == symbol
                && c->outlinewidth == s->outlinewidth
                && c->rotation == s->rotation
                && c->scale == s->scale
                && COMPARE_COLORS(c->color,s->color)
                && COMPARE_COLORS(c->backgroundcolor,s->backgroundcolor)
                && COMPARE_COLORS(c->outlinecolor,s->outlinecolor))
            return cur;
        cur = cur->next;
    }
    return NULL;
}

void copySymbolStyle(symbolStyleObj *dst, symbolStyleObj *src) {
    dst->outlinewidth = src->outlinewidth;
    dst->scale = src->scale;
    dst->rotation = src->rotation;
    dst->outlinewidth = src->outlinewidth;
    MS_COPYCOLOR(&dst->color,&src->color);
    MS_COPYCOLOR(&dst->outlinecolor,&src->outlinecolor);
    MS_COPYCOLOR(&dst->backgroundcolor,&src->backgroundcolor);
}

void freeTileCache(tileCacheObj *cache) {
    
}

/* add a cached tile to the current image's cache */
tileCacheObj *addTileCache(imageObj *img,
        void *data, symbolObj *symbol, symbolStyleObj *style, int width, int height) {
    tileCacheObj *cachep;

    if(img->ntiles >= MS_IMAGECACHESIZE) { /* remove last element, size stays the same */
        cachep = img->tilecache;

        /*go to the before last cache object*/
        while(cachep->next && cachep->next->next) cachep = cachep->next;

        /*free the last tile's data*/
        img->format->vtable->freeTile(cachep->next->data);

        /*reuse the last tile object*/
            /* make the cache point to the start of the list*/
        cachep->next->next = img->tilecache;
            /* point the global cache to the new first */
        img->tilecache = cachep->next;
            /* the before last cache is now last, so it has no successor*/
        cachep->next = NULL;
        
    } else {
        img->ntiles += 1;
        if((cachep = (tileCacheObj*)malloc(sizeof(tileCacheObj))) == NULL) {
            msSetError(MS_MEMERR, NULL, "addTileCache()");
            return(NULL);
        }
        cachep->next = img->tilecache;
        img->tilecache = cachep;
    }

    cachep = img->tilecache;

    cachep->data = data;
    copySymbolStyle(&cachep->style,style);
    cachep->width = width;
    cachep->height = height;
    cachep->symbol = symbol;
    return(cachep);
}

tileCacheObj *getTile(imageObj *img, symbolObj *symbol,  symbolStyleObj *s, int width, int height) {
	tileCacheObj *tile;
	rendererVTableObj *renderer = img->format->vtable;
    if(width==-1 || height == -1) {
        width=height=MS_MAX(symbol->sizex,symbol->sizey);
    }
	tile = searchTileCache(img,symbol,s,width,height);
	if(tile==NULL) {
        outputFormatObj format;
        imageObj *tileimg;
        double p_x,p_y;
        p_x = width/2.0;
        p_y = height/2.0;
        format.driver = img->format->driver;
        format.imagemode = MS_IMAGEMODE_RGBA;
        tileimg = renderer->createImage(width,height,&format,NULL);
        switch(symbol->type) {
            case (MS_SYMBOL_TRUETYPE):
                renderer->renderTruetypeSymbol(tileimg, p_x, p_y, symbol, s);
                break;
            case (MS_SYMBOL_PIXMAP): 
                renderer->renderPixmapSymbol(tileimg, p_x, p_y, symbol, s);
                break;
            case (MS_SYMBOL_ELLIPSE): 
                renderer->renderEllipseSymbol(tileimg, p_x, p_y,symbol, s);
                break;
            case (MS_SYMBOL_VECTOR): 
                renderer->renderVectorSymbol(tileimg, p_x, p_y, symbol, s);
                break;
            default:
                break;
        }
		tile = addTileCache(img,tileimg,symbol,s,width,height);
	}
	return tile;
}

void msImagePolylineMarkers(imageObj *image, shapeObj *p, symbolObj *symbol, 
        symbolStyleObj *style, double spacing, int auto_angle) {
    rendererVTableObj *renderer = image->format->vtable;
    int i,j;
    pointObj point;
    double original_rotation = style->rotation;
    double symbol_width;
    if(symbol->type != MS_SYMBOL_TRUETYPE)
        symbol_width = MS_MAX(1,symbol->sizex*style->scale);
    else {
        rectObj rect;
        renderer->getTruetypeTextBBox(image,symbol->full_font_path,style->scale,
                symbol->character,&rect,NULL);
        symbol_width=rect.maxx-rect.minx;
    }
    for(i=0; i<p->numlines; i++)
    {
        int line_in = 0;
        double current_length = (spacing+symbol_width)/2.0; // initial padding for each line
        double line_length=0;
        for(j=1;j<p->line[i].numpoints;j++)
        {
            double rx,ry,theta,length;
            int in;
            length = sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2)));
            line_length += length;
            if(length==0)continue;
            rx = (p->line[i].point[j].x - p->line[i].point[j-1].x)/length;
            ry = (p->line[i].point[j].y - p->line[i].point[j-1].y)/length;
            
            if (auto_angle) {
                theta = asin(ry);
                if(rx < 0) {
                    theta += MS_PI;
                }
                else theta = -theta;
                style->rotation = original_rotation + theta;
            }
            in = 0;
            while (current_length <= length) {
                point.x = p->line[i].point[j - 1].x + current_length * rx;
                point.y = p->line[i].point[j - 1].y + current_length * ry;
                switch (symbol->type) {
                    case MS_SYMBOL_PIXMAP:
                        renderer->renderPixmapSymbol(image, point.x, point.y, symbol, style);
                        break;
                    case MS_SYMBOL_ELLIPSE:
                        renderer->renderEllipseSymbol(image, point.x, point.y, symbol, style);
                        break;
                    case MS_SYMBOL_VECTOR:
                        renderer->renderVectorSymbol(image, point.x, point.y, symbol, style);
                        break;
                    case MS_SYMBOL_TRUETYPE:
                        renderer->renderTruetypeSymbol(image, point.x, point.y, symbol, style);
                        break;
                }
                current_length += symbol_width + spacing;
                in = 1;
                line_in=1;
            }

            if (in)
            {
                current_length -= length + symbol_width/2.0;
            }
            else current_length -= length;
        }
        
        /* if we couldn't place a symbol on the line, add one now
        we don't add the symbol if the line is shorter than the
        length of the symbol itself */
        if(!line_in && line_length>symbol_width) {
            
            /* total lengths of beginnning and end of current segment */
            double before_length=0,after_length=0;
            
            /*optimize*/
            line_length /= 2.0;

            for(j=1;j<p->line[i].numpoints;j++)
            {
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
                        }
                        else theta = -theta;
                        style->rotation = original_rotation + theta;
                    }

                    point.x = p->line[i].point[j - 1].x + offset * rx;
                    point.y = p->line[i].point[j - 1].y + offset * ry;
                    switch (symbol->type) {
                        case MS_SYMBOL_PIXMAP:
                            renderer->renderPixmapSymbol(image, point.x, point.y, symbol, style);
                            break;
                        case MS_SYMBOL_ELLIPSE:
                            renderer->renderEllipseSymbol(image, point.x, point.y, symbol, style);
                            break;
                        case MS_SYMBOL_VECTOR:
                            renderer->renderVectorSymbol(image, point.x, point.y, symbol, style);
                            break;
                        case MS_SYMBOL_TRUETYPE:
                            renderer->renderTruetypeSymbol(image, point.x, point.y, symbol, style);
                            break;
                    }
                    break;
                }
                before_length += length;
            }
        }

    }
}

rasterBufferObj *loadGDImg(gdImagePtr img) {
    rasterBufferObj *b = malloc(sizeof(rasterBufferObj));
    int row,col;
    b->width = img->sx;
	b->height = img->sy;
    b->pixelbuffer  = (unsigned char*)malloc(b->width*b->height*sizeof(int));
	b->row_step = b->width*sizeof(int);
    b->pixel_step = sizeof(int);
    b->b = &b->pixelbuffer[0];
    b->g = &b->pixelbuffer[1];
    b->b = &b->pixelbuffer[2];
    b->a = &b->pixelbuffer[3];

	for (row = 0; row < b->height; row++) {
		unsigned char* rowptr = &(b->pixelbuffer[row*b->row_step]);
		for (col = 0; col < b->width; col++) {
			int gdpix = gdImageGetTrueColorPixel(img, col, row);
			//extract the alpha value from the pixel
			int gdpixalpha = ((gdpix) & 0x7F000000) >> 24;

			if (gdpixalpha == 127) {//treat the fully transparent case
				((int*)rowptr)[col] = 0;
			} else if (gdpixalpha == 0) {//treat the fully opaque case
				((int*)rowptr)[col] = ((gdpix) & 0x00FFFFFF) | (255 << 24);
			} else {
				int alpha = 255 - (gdpixalpha << 1);
				((int*)rowptr)[col] = ((gdpix) & 0x00FFFFFF) | (alpha << 24);
			}
		}
	}
	return b;
}

void msDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, 
        styleObj *style, double scalefactor)
{
    if (image)
    {
		if (MS_RENDERER_PLUGIN(image->format)) {
			rendererVTableObj *renderer = image->format->vtable;
			symbolObj *symbol;
			shapeObj *offsetLine = p;
            int i;
            double width;
		    
            if (p->numlines == 0)
				return;
	
			if (style->symbol >= symbolset->numsymbols || style->symbol < 0)
				return; /* no such symbol, 0 is OK   */

			symbol = symbolset->symbol[style->symbol];
            /* store a reference to the renderer to be used for freeing */
            symbol->renderer = renderer;

			width = style->width * scalefactor;
            width = MS_MIN(width,style->maxwidth);
            width = MS_MAX(width,style->minwidth);
            
            if(style->offsety==-99) {
				offsetLine = msOffsetPolyline(p,style->offsetx * width,-99);
			} else if(style->offsetx!=0 || style->offsety!=0) {
                offsetLine = msOffsetPolyline(p,
                        style->offsetx * width, style->offsety * width); 
            } 
            if(style->symbol == 0 || (symbol->type==MS_SYMBOL_SIMPLE)) {
                strokeStyleObj s;
                s.linecap = style->linecap;
                s.linejoin = style->linejoin;
                s.linejoinmaxsize = style->linejoinmaxsize;
                s.width = width;
                s.patternlength = style->patternlength;
                for(i=0;i<s.patternlength;i++)
                    s.pattern[i] = style->pattern[i]*s.width;

                if(MS_VALID_COLOR(style->color))
                    MS_COPYCOLOR(&s.color,&style->color);
                else if(MS_VALID_COLOR(style->outlinecolor))
                    MS_COPYCOLOR(&s.color,&style->outlinecolor);
                else {
                    msSetError(MS_MISCERR,"no color defined for line styling","msDrawLineSymbol()");
                    //TODO throw error
                }
                s.color.alpha = MS_NINT(style->opacity * 2.55);

                renderer->renderLine(image,offsetLine,&s);
            }
			else {
                symbolStyleObj s;
                computeSymbolStyle(&s,style,symbol,scalefactor);
                if(symbol->type == MS_SYMBOL_TRUETYPE) {
                    if(!symbol->full_font_path)
                        symbol->full_font_path =  strdup(msLookupHashTable(&(symbolset->fontset->fonts),
                                    symbol->font));
                    if(!symbol->full_font_path)
                        return;
                }
                if(symbol->type == MS_SYMBOL_PIXMAP && symbol->img) {
                    if(!symbol->pixmap_buffer) {
                        symbol->pixmap_buffer = loadGDImg(symbol->img);
                    }
                }

                //compute a markerStyle and use it on the line
                if(style->gap<0) {
			        //special function that treats any other symbol used as a marker, not a brush
			        msImagePolylineMarkers(image,p,symbol,&s,-style->gap,1);
			    }
			    else if(style->gap>0){
			        msImagePolylineMarkers(image,p,symbol,&s,style->gap,0);
			    } else {
			    	//void* tile = getTile(image, symbolset, &s);
			    	//r->renderLineTiled(image, theShape, tile);
                }
            }
			
			if(offsetLine!=p)
				msFreeShape(offsetLine);
		}
    	else if( MS_RENDERER_GD(image->format) )
            msDrawLineSymbolGD(symbolset, image, p, style, scalefactor);
#ifdef USE_AGG
        else if( MS_RENDERER_AGG(image->format) )
            msDrawLineSymbolAGG(symbolset, image, p, style, scalefactor);
#endif
	else if( MS_RENDERER_IMAGEMAP(image->format) )
            msDrawLineSymbolIM(symbolset, image, p, style, scalefactor);

#ifdef USE_MING_FLASH
        else if( MS_RENDERER_SWF(image->format) )
            msDrawLineSymbolSWF(symbolset, image, p,  style, scalefactor);
#endif
#ifdef USE_PDF
        else if( MS_RENDERER_PDF(image->format) )
            msDrawLineSymbolPDF(symbolset, image, p,  style, scalefactor);
#endif
        else if( MS_RENDERER_SVG(image->format) )
            msDrawLineSymbolSVG(symbolset, image, p,  style, scalefactor);

    }
}

void msDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, shapeObj *p, styleObj *style, double scalefactor)
{
    if (!p)
		return;
	if (p->numlines <= 0)
		return;
    /* 
     * if only an outlinecolor was defined, and not a color,
     * switch to the line drawing function
     *
     * this behavior is kind of a mapfile hack, and must be
     * kept for backwards compatibility
     */
    if (style->symbol >= symbolset->numsymbols || style->symbol < 0)
				return; // no such symbol, 0 is OK
			

    if (symbolset->symbol[style->symbol]->type != MS_SYMBOL_PIXMAP) {
        if (!MS_VALID_COLOR(style->color)) {
            if(MS_VALID_COLOR(style->outlinecolor))
		        return msDrawLineSymbol(symbolset, image, p, style, scalefactor);
            else {
                /* just do nothing if no color has been set */
                return;
            }
        }
	}
    if (image)
    {
        if (MS_RENDERER_PLUGIN(image->format)) {
        	rendererVTableObj *renderer = image->format->vtable;
        	shapeObj *offsetPolygon = NULL;
        	symbolObj *symbol = symbolset->symbol[style->symbol];
            /* store a reference to the renderer to be used for freeing */
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
                style->color.alpha = MS_NINT(style->opacity*2.55);
                renderer->renderPolygon(image,offsetPolygon,&style->color);
                if(MS_VALID_COLOR(style->outlinecolor)) {
                    strokeStyleObj s;
                    MS_COPYCOLOR(&s.color,&style->outlinecolor);
                    s.patternlength = 0;
                    s.width = (style->width == 0)?scalefactor:style->width*scalefactor;

                    s.color.alpha = style->color.alpha;
                    renderer->renderLine(image,offsetPolygon,&s);
                }
                goto cleanup; /*finished plain polygon*/
            }
            else {
//temp code wgile symbols are unsupported
                colorObj red;
                MS_INIT_COLOR(red,255,0,0);
                red.alpha=255;
                renderer->renderPolygon(image,offsetPolygon,&red);
                goto cleanup; /*finished plain polygon*/
            }
#if 0		
            else {
        	    double size, angle_radians, width,scaling;
                
                /* polygons with advanced symbology */
                if (style->size == -1) {
			    	size = msSymbolGetDefaultSize(symbolset->symbol[style->symbol]);
			    	size = size * scalefactor;
			    } else
			    	size = style->size * scalefactor;
			    size = MS_MAX(size, style->minsize);
			    size = MS_MIN(size, style->maxsize);

			    width = style->width * scalefactor;
			    width = MS_MAX(width, style->minwidth);
			    width = MS_MIN(width, style->maxwidth);

			    if (symbol->sizey)
			    	scaling = size / symbol->sizey; /* compute the scaling factor (d) on the unrotated symbol */
			    else
			    	scaling = 1;

			    angle_radians = style->angle * MS_DEG_TO_RAD;

			    ox = style->offsetx * scalefactor; // should we scale the offsets?
			    oy = style->offsety * scalefactor;

			    if (!MS_VALID_COLOR(style->color) && symbol->type != MS_SYMBOL_PIXMAP)
			    	return; // nothing to do (colors are not required with PIXMAP symbols)
			    if (size < 1)
			    	return; // size too small

			    
			    if (style->symbol == 0 || symbol->type == MS_SYMBOL_SIMPLE) {
			    	// simply draw a solid fill and outline of the specified colors
			    	r->renderPolygon(image,offsetPolygon, &c, &oc,width);
			    } else {
			    	if(MS_VALID_COLOR(bc)) {
			    		r->renderPolygon(image,offsetPolygon, &bc, NULL,width);
			    	}
			    	void *tile;
			    	styleObj s;
			    	msCopyStyle(&s,style);
			    	s.size=scaling;
			    	s.width=width;
			    	s.angle=angle_radians;
			    	switch(symbol->type) {
			    	case MS_SYMBOL_TRUETYPE:
			    	case MS_SYMBOL_PIXMAP:
			    	case MS_SYMBOL_ELLIPSE:
			    	case MS_SYMBOL_VECTOR:
			    		tile = getTile(image,symbolset,&s);
			    	}
			    	r->renderPolygonTiled(image,offsetPolygon, &oc, ow, tile);
			    }
            }
#endif

cleanup:
			if (style->offsety == -99) {
				msFreeShape(offsetPolygon);
			}
            return;
		} else if( MS_RENDERER_GD(image->format) )
            msDrawShadeSymbolGD(symbolset, image, p, style, scalefactor);
#ifdef USE_AGG
        else if( MS_RENDERER_AGG(image->format) )
            msDrawShadeSymbolAGG(symbolset, image, p, style, scalefactor);
#endif
	else if( MS_RENDERER_IMAGEMAP(image->format) )
            msDrawShadeSymbolIM(symbolset, image, p, style, scalefactor);

#ifdef USE_MING_FLASH
        else if( MS_RENDERER_SWF(image->format) )
            msDrawShadeSymbolSWF(symbolset, image, p, style, scalefactor);
#endif
#ifdef USE_PDF
        else if( MS_RENDERER_PDF(image->format) )
            msDrawShadeSymbolPDF(symbolset, image, p, style, scalefactor);
#endif
        else if( MS_RENDERER_SVG(image->format) )
            msDrawShadeSymbolSVG(symbolset, image, p, style, scalefactor);
    }
}

void msDrawMarkerSymbol(symbolSetObj *symbolset,imageObj *image, pointObj *p, styleObj *style,
        double scalefactor)
{
    if (!p)
        return;
    if (style->symbol >= symbolset->numsymbols || style->symbol < 0)
        return; /* no such symbol, 0 is OK   */

   if (image)
   {
       if(MS_RENDERER_PLUGIN(image->format)) {
    	    rendererVTableObj *renderer = image->format->vtable;
            symbolStyleObj s;
       	    double p_x,p_y;
            symbolObj *symbol = symbolset->symbol[style->symbol];
    	    /* store a reference to the renderer to be used for freeing */
            symbol->renderer = renderer;
            
            computeSymbolStyle(&s,style,symbol,scalefactor);
            if (!MS_VALID_COLOR(s.color) && !MS_VALID_COLOR(s.outlinecolor) && symbol->type != MS_SYMBOL_PIXMAP)
				return; // nothing to do if no color, except for pixmap symbols
            
            if(symbol->type == MS_SYMBOL_PIXMAP && symbol->img) {
                if(!symbol->pixmap_buffer) {
                    symbol->pixmap_buffer = loadGDImg(symbol->img);
                }
            }
    	   
			
		    //TODO: skip the drawing of the symbol if it's smaller than a pixel ?
            /*
			if (s.size < 1)
				return; // size too small
            */

            p_x = p->x + style->offsetx * scalefactor;
            p_y = p->y + style->offsety * scalefactor;

			if(renderer->supports_imagecache) {
				tileCacheObj *tile = getTile(image, symbol, &s, -1, -1);
				if(tile!=NULL)
				    renderer->renderTile(image, (imageObj*)tile->data, p_x, p_y);
				return;
			}
			switch (symbol->type) {
			case (MS_SYMBOL_TRUETYPE): {
                if(!symbol->full_font_path)
				    symbol->full_font_path =  strdup(msLookupHashTable(&(symbolset->fontset->fonts),
						symbol->font));
                if(!symbol->full_font_path)
					return;
                renderer->renderTruetypeSymbol(image, p_x, p_y, symbol, &s);

			}
				break;
			case (MS_SYMBOL_PIXMAP): {
				renderer->renderPixmapSymbol(image,p_x,p_y,symbol,&s);
			}
				break;
			case (MS_SYMBOL_ELLIPSE): {
				renderer->renderEllipseSymbol(image, p_x, p_y,symbol, &s);
			}
				break;
			case (MS_SYMBOL_VECTOR): {
				renderer->renderVectorSymbol(image, p_x, p_y, symbol, &s);
			}
				break;
			default:
				break;
			}
       }
       else if( MS_RENDERER_GD(image->format) )
           msDrawMarkerSymbolGD(symbolset, image, p, style, scalefactor);
#ifdef USE_AGG
       else if( MS_RENDERER_AGG(image->format) )
           msDrawMarkerSymbolAGG(symbolset, image, p, style, scalefactor);
#endif
       else if( MS_RENDERER_IMAGEMAP(image->format) )
           msDrawMarkerSymbolIM(symbolset, image, p, style, scalefactor);

#ifdef USE_MING_FLASH
       else if( MS_RENDERER_SWF(image->format) )
           msDrawMarkerSymbolSWF(symbolset, image, p, style, scalefactor);
#endif
#ifdef USE_PDF
       else if( MS_RENDERER_PDF(image->format) )
           msDrawMarkerSymbolPDF(symbolset, image, p, style, scalefactor);
#endif
       else if( MS_RENDERER_SVG(image->format) )
           msDrawMarkerSymbolSVG(symbolset, image, p, style, scalefactor);

    }
}

/************************************************************************/
/*                          msCircleDrawLineSymbol                      */
/*                                                                      */
/*      Note : the map parameter is only used to be able to converet    */
/*      the color index to rgb values.                                  */
/************************************************************************/
void msCircleDrawLineSymbol(symbolSetObj *symbolset, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor)
{
    if (image)
    {
        if( MS_RENDERER_GD(image->format) )
            msCircleDrawLineSymbolGD(symbolset, image, p, r, style, scalefactor);
#ifdef USE_AGG
        else if( MS_RENDERER_AGG(image->format) )
            msCircleDrawLineSymbolAGG(symbolset, image, p, r, style, scalefactor);
#endif
	else if( MS_RENDERER_IMAGEMAP(image->format) )
            msCircleDrawLineSymbolIM(symbolset, image, p, r, style, scalefactor);
        else
            msSetError(MS_MISCERR, "Unknown image type",
                       "msCircleDrawLineSymbol()");
    }
}

void msCircleDrawShadeSymbol(symbolSetObj *symbolset, imageObj *image, pointObj *p, double r, styleObj *style, double scalefactor)
{
    if (image)
    {
        if( MS_RENDERER_GD(image->format) )
            msCircleDrawShadeSymbolGD(symbolset, image, p, r, style, scalefactor);
#ifdef USE_AGG
        else if( MS_RENDERER_AGG(image->format) )
            msCircleDrawShadeSymbolAGG(symbolset, image, p, r, style, scalefactor);
#endif
	else if( MS_RENDERER_IMAGEMAP(image->format) )
            msCircleDrawShadeSymbolIM(symbolset, image, p, r, style, scalefactor);

        else
             msSetError(MS_MISCERR, "Unknown image type",
                        "msCircleDrawShadeSymbol()");
    }
}


/*
** Render the text (no background effects) for a label.
 */
int msDrawText(imageObj *image, pointObj labelPnt, char *string,
		labelObj *label, fontSetObj *fontset, double scalefactor) {
	int nReturnVal = -1;
        labelStyleObj s;
 	if (image) {
		if (MS_RENDERER_PLUGIN(image->format)) {
			rendererVTableObj *renderer = image->format->vtable;
            double x, y;
			if (!string || !strlen(string))
				return (0); // not errors, just don't want to do anything
            
            
            computeLabelStyle(&s,label,fontset,scalefactor);
			x = labelPnt.x;
			y = labelPnt.y;
			if (label->type == MS_TRUETYPE) {
				renderer->renderGlyphs(image,x,y,&s,string);			}
		}
	else if( MS_RENDERER_GD(image->format) )
      nReturnVal = msDrawTextGD(image, labelPnt, string, label, fontset, scalefactor);
#ifdef USE_AGG
    else if( MS_RENDERER_AGG(image->format) )
      nReturnVal = msDrawTextAGG(image, labelPnt, string, label, fontset, scalefactor);
#endif
    else if( MS_RENDERER_IMAGEMAP(image->format) )
      nReturnVal = msDrawTextIM(image, labelPnt, string, label, fontset, scalefactor);
#ifdef USE_MING_FLASH
    else if( MS_RENDERER_SWF(image->format) )
      nReturnVal = draw_textSWF(image, labelPnt, string, label, fontset, scalefactor);
#endif
#ifdef USE_PDF
    else if( MS_RENDERER_PDF(image->format) )
      nReturnVal = msDrawTextPDF(image, labelPnt, string, label, fontset, scalefactor);
#endif
    else if( MS_RENDERER_SVG(image->format) )
      nReturnVal = msDrawTextSVG(image, labelPnt, string, label, fontset, scalefactor);
  }

  return nReturnVal;
}

int msDrawTextLine(imageObj *image, char *string, labelObj *label, labelPathObj *labelpath, fontSetObj *fontset, double scalefactor)
{
    int nReturnVal = -1;
    if(image) {
        if (MS_RENDERER_PLUGIN(image->format)) {
            rendererVTableObj *renderer = image->format->vtable;
            labelStyleObj s;
            if (!string || !strlen(string))
                return (0); // not errors, just don't want to do anything


            computeLabelStyle(&s,label,fontset,scalefactor);
            if (label->type == MS_TRUETYPE) {
                renderer->renderGlyphsLine(image,labelpath,&s,string);			
            }
        }
    else if( MS_RENDERER_GD(image->format) )
      nReturnVal = msDrawTextLineGD(image, string, label, labelpath, fontset, scalefactor);
#ifdef USE_AGG
    else if( MS_RENDERER_AGG(image->format) )
      nReturnVal = msDrawTextLineAGG(image, string, label, labelpath, fontset, scalefactor);
#endif
  }

  return nReturnVal;
}

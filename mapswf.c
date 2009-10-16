/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Macromedia flash output
 * Author:   Y. Assefa, DM Solutions Group (assefa@dmsolutions.ca)
 *
 * Note: 
 * This api is based on the ming swf library :
 *     http://www.opaque.net/ming/
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

#ifdef USE_MING_FLASH

#include <assert.h>
#if !defined(_WIN32)
#include <zlib.h>
#endif
#include "mapserver.h"
#include "mapswf.h"

MS_CVSID("$Id$")

static char gszFilename[128];
static char gszAction[256];
static char gszTmp[256];

#define MOUSEUP 1
#define MOUSEDOWN 2
#define MOUSEOVER 3
#define MOUSEOUT 4

/* -------------------------------------------------------------------- */
/*      prototypes.                                                     */
/* -------------------------------------------------------------------- */
SWFMovie GetCurrentMovie(mapObj *map, imageObj *image);

/************************************************************************/
/*                       void StoreDblData                              */
/*                                                                      */
/*      Keep tracks of the DblData allocated to properly free them      */
/*                        Ticket #2555                                  */
/************************************************************************/
void StoreDblData(unsigned char *oDblData, imageObj *image)
{
	int nDblData;
        
	((SWFObj *)image->img.swf)->nDblDatas += 1;
	nDblData = ((SWFObj *)image->img.swf)->nDblDatas;
	if (!((SWFObj *)image->img.swf)->DblDatas)
	{
		((SWFObj *)image->img.swf)->DblDatas =
			(unsigned char **)malloc(sizeof(oDblData)*1);
	}
    else
    {
        ((SWFObj *)image->img.swf)->DblDatas =
        	(unsigned char **)realloc(((SWFObj *)image->img.swf)->DblDatas,
        			sizeof(oDblData)*nDblData);
	}
	((SWFObj *)image->img.swf)->DblDatas[nDblData-1] = oDblData;
}

/************************************************************************/
/*                       void StoreShape                                */
/*                                                                      */
/*      Keep tracks of the SWFShape allocated to properly free them     */
/*                        Ticket #2555                                  */
/************************************************************************/
void StoreShape(SWFShape oShape, imageObj *image)
{
	int nShape;

	((SWFObj *)image->img.swf)->nShapes += 1;
	nShape = ((SWFObj *)image->img.swf)->nShapes;
	if (!((SWFObj *)image->img.swf)->Shapes)
	{
		((SWFObj *)image->img.swf)->Shapes =
	        	(SWFShape *)malloc(sizeof(SWFShape));
	}
	else
    {
        ((SWFObj *)image->img.swf)->Shapes =
        	(SWFShape *)realloc(((SWFObj *)image->img.swf)->Shapes,
        			sizeof(SWFShape)*nShape);
	}
	((SWFObj *)image->img.swf)->Shapes[nShape-1] = oShape;
}

/************************************************************************/
/*                       void StoreInput                                */
/*                                                                      */
/*      Keep tracks of the SWFInput allocated to properly free them     */
/*                        Ticket #2555                                  */
/************************************************************************/
void StoreInput(SWFInput oInput, imageObj *image)
{
	int nInput;

	((SWFObj *)image->img.swf)->nInputs += 1;
	nInput = ((SWFObj *)image->img.swf)->nInputs;
	if (!((SWFObj *)image->img.swf)->Inputs)
	{
		((SWFObj *)image->img.swf)->Inputs =
	        	(SWFInput *)malloc(sizeof(SWFInput));
	}
	else
    {
        ((SWFObj *)image->img.swf)->Inputs =
        	(SWFInput *)realloc(((SWFObj *)image->img.swf)->Inputs,
        			sizeof(SWFInput)*nInput);
	}
	((SWFObj *)image->img.swf)->Inputs[nInput-1] = oInput;
}

/************************************************************************/
/*                       void StoreBitmap                               */
/*                                                                      */
/*      Keep tracks of the SWFBitmap allocated to properly free them    */
/*                        Ticket #2555                                  */
/************************************************************************/
void StoreBitmap(SWFBitmap oBitmap, imageObj *image)
{
	int nBitmap;

	((SWFObj *)image->img.swf)->nBitmaps += 1;
	nBitmap = ((SWFObj *)image->img.swf)->nBitmaps;
	if (!((SWFObj *)image->img.swf)->Bitmaps)
	{
		((SWFObj *)image->img.swf)->Bitmaps =
	        	(SWFBitmap *)malloc(sizeof(SWFBitmap));
	}
	else
    {
        ((SWFObj *)image->img.swf)->Bitmaps =
        	(SWFBitmap *)realloc(((SWFObj *)image->img.swf)->Bitmaps,
        			sizeof(SWFBitmap)*nBitmap);
	}
	((SWFObj *)image->img.swf)->Bitmaps[nBitmap-1] = oBitmap;
}

/************************************************************************/
/*                       void StoreButton                               */
/*                                                                      */
/*      Keep tracks of the SWFButton allocated to properly free them    */
/*                        Ticket #2555                                  */
/************************************************************************/
void StoreButton(SWFButton oButton, imageObj *image)
{
	int nButton;

	((SWFObj *)image->img.swf)->nButtons += 1;
	nButton = ((SWFObj *)image->img.swf)->nButtons;
	if (!((SWFObj *)image->img.swf)->Buttons)
	{
		((SWFObj *)image->img.swf)->Buttons =
			(SWFButton *)malloc(sizeof(oButton));
	}
	else
	{
		((SWFObj *)image->img.swf)->Buttons =
			(SWFButton *)realloc(((SWFObj *)image->img.swf)->Buttons,
					sizeof(oButton)*nButton);
	}
	((SWFObj *)image->img.swf)->Buttons[nButton-1] = oButton;
}

/************************************************************************/
/*                       void StoreFont                                 */
/*                                                                      */
/*      Keep tracks of the SWFFont allocated to properly free them      */
/*                        Ticket #2555                                  */
/************************************************************************/
void StoreFont(SWFFont oFont, imageObj *image)
{
	int nFont;
        
	((SWFObj *)image->img.swf)->nFonts += 1;
	nFont = ((SWFObj *)image->img.swf)->nFonts;
	if (!((SWFObj *)image->img.swf)->Fonts)
	{
		((SWFObj *)image->img.swf)->Fonts =
			(SWFFont *)malloc(sizeof(oFont));
	}
	else
	{
		((SWFObj *)image->img.swf)->Fonts =
			(SWFFont *)realloc(((SWFObj *)image->img.swf)->Fonts,
					sizeof(oFont)*nFont);
	}
	((SWFObj *)image->img.swf)->Fonts[nFont-1] = oFont;
}

/************************************************************************/
/*                       void StoreText                                 */
/*                                                                      */
/*      Keep tracks of the SWFText allocated to properly free them      */
/*                        Ticket #2555                                  */
/************************************************************************/
void StoreText(SWFText oText, imageObj *image)
{
	int nText;
        
	((SWFObj *)image->img.swf)->nTexts += 1;
	nText = ((SWFObj *)image->img.swf)->nTexts;
	if (!((SWFObj *)image->img.swf)->Texts)
	{
		((SWFObj *)image->img.swf)->Texts =
			(SWFText *)malloc(sizeof(oText));
	}
	else
	{
		((SWFObj *)image->img.swf)->Texts =
			(SWFText *)realloc(((SWFObj *)image->img.swf)->Texts,
					sizeof(oText)*nText);
	}
	((SWFObj *)image->img.swf)->Texts[nText-1] = oText;
}

/************************************************************************/
/*                    gdImagePtr getTileImageFromSymbol                 */
/*                                                                      */
/*      Returns a gdimage from a symbol that will be used to fill       */
/*      polygons. Returns NULL on error.                                */
/*                                                                      */
/*       Returned image should only be destroyed by caller if the       */
/*      bDestroyImage is set to 1.                                      */
/*                                                                      */
/************************************************************************/
#ifdef unused
gdImagePtr getTileImageFromSymbol(mapObj *map, symbolSetObj *symbolset, 
                                  int sy, int fc, int bc, int oc, 
                                  double sz, int *bDestroyImage)
{
#ifdef undef
    symbolObj   *symbol;
    int         i;
    gdPoint     oldpnt,newpnt;
    gdPoint     sPoints[MS_MAXVECTORPOINTS];
    gdImagePtr tile = NULL;
    int         x,y;
    int         tile_bg=-1, tile_fg=-1; /*colors (background and foreground)*/
  
    double        scale=1.0;
  
    int         bbox[8];
    rectObj     rect;
    char        *font=NULL;

    colorObj    sFc;
    colorObj    sBc;
    colorObj    sOc;
    colorObj    sColor0;

/* -------------------------------------------------------------------- */
/*      validate params.                                                */
/* -------------------------------------------------------------------- */
    if (!map || !symbolset || sy > symbolset->numsymbols || sy < 0)
        return NULL;

/* -------------------------------------------------------------------- */
/*      extract the colors.                                             */
/* -------------------------------------------------------------------- */
    getRgbColor(map, fc, &sFc.red, &sFc.green, &sFc.blue);
    getRgbColor(map, bc, &sBc.red, &sBc.green, &sBc.blue);
    getRgbColor(map, oc, &sOc.red, &sOc.green, &sOc.blue);
    getRgbColor(map, 0, &sColor0.red, &sColor0.green, &sColor0.blue);
     
    symbol = &(symbolset->symbol[sy]);
    switch(symbol->type) 
    {
	case(MS_SYMBOL_TRUETYPE):

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
	    font = msLookupHashTable(&(symbolset->fontset->fonts), 
				     symbolset->symbol[sy]->font);
	    if(!font) 
	        return NULL;

	    if(msGetCharacterSize(symbol->character, sz, font, &rect) != MS_SUCCESS) 
	        return NULL;

	    x = rect.maxx - rect.minx;
	    y = rect.maxy - rect.miny;

	    tile = gdImageCreate(x, y);
	    if(bc >= 0)
	        tile_bg = 
		    gdImageColorAllocate(tile, sBc.red, sBc.green, sBc.blue);
               
	    else 
	    {
		tile_bg = gdImageColorAllocate(tile, sColor0.red,
					       sColor0.green,
					       sColor0.blue);
		gdImageColorTransparent(tile,0);
	    }
	    tile_fg = gdImageColorAllocate(tile, sFc.red, sFc.green, sFc.blue);
    
	    x = -rect.minx;
	    y = -rect.miny;

	    gdImageStringFT(tile, bbox, 
			    ((symbol->antialias)?(tile_fg):-(tile_fg)), 
			    font, sz, 0, x, y, symbol->character);

#endif
	    if (bDestroyImage)
	        *bDestroyImage = 1;

	    break;

        case(MS_SYMBOL_PIXMAP):

	    tile = symbol->img;
	    if (bDestroyImage)
	        *bDestroyImage = 0;

	    break;

        case(MS_SYMBOL_ELLIPSE):
   
	    scale = sz/symbol->sizey; /* sz ~ height in pixels */
	    x = MS_NINT(symbol->sizex*scale)+1;
	    y = MS_NINT(symbol->sizey*scale)+1;

	    if((x <= 1) && (y <= 1)) 
	    { /* No sense using a tile, just fill solid */
	        if (bDestroyImage)
		    *bDestroyImage = 0;
		tile = NULL;
		break;
	    }

	    tile = gdImageCreate(x, y);
	    if(bc >= 0)
	        tile_bg = gdImageColorAllocate(tile, sBc.red,
					       sBc.green, sBc.blue);
	    else 
	    {
		tile_bg = gdImageColorAllocate(tile, sColor0.red,
					       sColor0.green,
					       sColor0.blue);
		gdImageColorTransparent(tile,0);
	    }
	    tile_fg = gdImageColorAllocate(tile, sFc.red, sFc.green, sFc.blue);

	    x = MS_NINT(tile->sx/2);
	    y = MS_NINT(tile->sy/2);
	    
	    /*
	    ** draw in the tile image
	    */
	    gdImageArc(tile, x, y, MS_NINT(scale*symbol->points[0].x), 
		       MS_NINT(scale*symbol->points[0].y), 0, 360, tile_fg);
	    if(symbol->filled)
	        gdImageFillToBorder(tile, x, y, tile_fg, tile_fg);

	    if (bDestroyImage)
	        *bDestroyImage = 1;

	    break;

        case(MS_SYMBOL_VECTOR):

	    if(fc < 0)
	    {
		if (bDestroyImage)
		    *bDestroyImage = 0;
		tile = NULL;
		break;
	    }

	    scale = sz/symbol->sizey; /* sz ~ height in pixels */
	    x = MS_NINT(symbol->sizex*scale)+1;    
	    y = MS_NINT(symbol->sizey*scale)+1;

	    if((x <= 1) && (y <= 1)) 
	    { /* No sense using a tile, just fill solid */
		if (bDestroyImage)
		  *bDestroyImage = 0;
		tile = NULL;
		break;
	    }

	    /*
	    ** create tile image
	    */
	    tile = gdImageCreate(x, y);
	    if(bc >= 0)
	        tile_bg = gdImageColorAllocate(tile, sBc.red,
					       sBc.green, sBc.blue);
	    else 
	    {
		tile_bg = gdImageColorAllocate(tile, sColor0.red,
					       sColor0.green,
					       sColor0.blue);
		gdImageColorTransparent(tile,0);
	    }
	    tile_fg = gdImageColorAllocate(tile, sFc.red, sFc.green, sFc.blue);
   
	    /*
	    ** draw in the tile image
	    */
	    if(symbol->filled) 
	    {
	        for(i=0;i < symbol->numpoints;i++) 
		{
		    sPoints[i].x = MS_NINT(scale*symbol->points[i].x);
		    sPoints[i].y = MS_NINT(scale*symbol->points[i].y);
		}
		gdImageFilledPolygon(tile, sPoints, symbol->numpoints, tile_fg);
	    } 
	    else  
	    { /* shade is a vector drawing */

	        oldpnt.x = MS_NINT(scale*symbol->points[0].x); /* convert first point in shade smbol */
		oldpnt.y = MS_NINT(scale*symbol->points[0].y);

		/* step through the shade sy */
		for(i=1;i < symbol->numpoints;i++) {
		    if((symbol->points[i].x < 0) && (symbol->points[i].y < 0)) {
		        oldpnt.x = MS_NINT(scale*symbol->points[i].x);
			oldpnt.y = MS_NINT(scale*symbol->points[i].y);
		    } else {
		        if((symbol->points[i-1].x < 0) && (symbol->points[i-1].y < 0)) { /* Last point was PENUP, now a new beginning */
			    oldpnt.x = MS_NINT(scale*symbol->points[i].x);
			    oldpnt.y = MS_NINT(scale*symbol->points[i].y);
			} else {
			    newpnt.x = MS_NINT(scale*symbol->points[i].x);
			    newpnt.y = MS_NINT(scale*symbol->points[i].y);
			    gdImageLine(tile, oldpnt.x, oldpnt.y, newpnt.x, newpnt.y, tile_fg);
			    oldpnt = newpnt;
			}
		    }
		} /* end for loop */
	    }

	    if (bDestroyImage)
	        *bDestroyImage = 1;

	    break;

        default:
	    break;

    }

    return tile;
#endif /* undef */

    return NULL; 
}
#endif /* unused */



/************************************************************************/
/*                          SWFShape bitmap2shape                       */
/*                                                                      */
/*      Return a filled polygon shape using a bitmap.                   */
/************************************************************************/
SWFShape bitmap2shape(unsigned char *data,unsigned long size,int width,
                      int height, byte flags, imageObj *image)
{
    SWFShape oShape;
    SWFInput oBuffer;
    SWFFill oFill;
    SWFBitmap oBitmap;

    if (!data || width <= 0 || height <= 0)
        return NULL;

    oShape = newSWFShape();
    oBuffer = newSWFInput_buffer(data,size);
    StoreInput(oBuffer, image);
    oBitmap = newSWFBitmap_fromInput(oBuffer);
    StoreBitmap(oBitmap, image);
    
    oFill = SWFShape_addBitmapFill(oShape, oBitmap, flags);
    /* oFill = SWFShape_addBitmapFill(oShape, oBitmap, 0); */
    
    SWFShape_setRightFill(oShape, oFill);
    destroySWFFill(oFill);
    
    SWFShape_drawLine(oShape, (float)width, 0.0);
    SWFShape_drawLine(oShape, 0.0, (float)height);
    SWFShape_drawLine(oShape, (float)-width, 0.0);
    SWFShape_drawLine(oShape, 0.0, (float)-height);

    return oShape;
}


/************************************************************************/
/*                        unsigned char *bitmap2dbl                     */
/*                                                                      */
/*      Converts a bitmap to dbl bitmap suitable for ming.              */
/************************************************************************/
unsigned char *bitmap2dbl(unsigned char *data,int *size, int *bytesPerColor) 
{
    unsigned char *outdata,*dbldata;
    unsigned long outsize;
    int i,j;

    outsize = (int) floor(*size*1.01+12);
    dbldata =  (unsigned char *) malloc(outsize + 14);
    outdata=&dbldata[14];
    compress2(outdata,&outsize,data+6,*size-6,6);
    dbldata[0] = 'D';
    dbldata[1] = 'B';
    dbldata[2] = 'l';
    dbldata[3] = (unsigned char)(*bytesPerColor == 3 ? 1 : 2);
    dbldata[4] = (unsigned char)(((outsize+6) >> 24) & 0xFF);
    dbldata[5] = (unsigned char)(((outsize+6) >> 16) & 0xFF);
    dbldata[6] = (unsigned char)(((outsize+6) >>  8) & 0xFF);
    dbldata[7] = (unsigned char)(((outsize+6) >>  0) & 0xFF);
    for (i=8,j=0;i<14;i++,j++) {
        dbldata[i] = data[j];
    }
    *size = outsize + 14;
    
    return(dbldata);
}

/************************************************************************/
/*                         nsigned char *gd2bitmap                      */
/*                                                                      */
/*      Convert a gd image to a bitmap.                                 */
/************************************************************************/
unsigned char *gd2bitmap(gdImagePtr img,int *size, int *bytesPerColor) 
{
    int width,height;
    int alignedWidth;
    unsigned char *data;
    unsigned char *p;
    unsigned char *line;
    int i;
	
    width = img->sx;
    height = img->sy;
    alignedWidth = (width + 3) & ~3;
    *bytesPerColor = 3;
    if (img->transparent >= 0 ) *bytesPerColor += 1;

    *size = 6 + (img->colorsTotal * *bytesPerColor) + (alignedWidth * height);
    data= (unsigned char *)malloc(*size);

    p=data;
    *p++ = 3;
    *p++ = (unsigned char)((width>> 0) & 0xFF);
    *p++ = (unsigned char)((width >> 8) & 0xFF);
    *p++ = (unsigned char)((height >> 0) & 0xFF);
    *p++ = (unsigned char)((height >> 8) & 0xFF);
    *p++ = (unsigned char)(img->colorsTotal - 1);

    for (i=0;i<img->colorsTotal;++i) {
        if (*bytesPerColor == 3) {
            *p++ = (unsigned char)(img->red[i]);
            *p++ = (unsigned char)(img->green[i]);
            *p++ = (unsigned char)(img->blue[i]);
        } else {
            if (i != img->transparent) {
                *p++ = (unsigned char)(img->red[i]);
                *p++ = (unsigned char)(img->green[i]);
                *p++ = (unsigned char)(img->blue[i]);
                *p++ = 255;
            } else {
                *p++ = 0;
                *p++ = 0;
                *p++ = 0;
                *p++ = 0;
            }
        }
    }
    line=*(img->pixels);
    for (i=0;i<height;i++) {
        line = img->pixels[i];
        memset(p,1,alignedWidth);
        memcpy(p,line,width);
        p += alignedWidth;
    }

    return(data);
}

/************************************************************************/
/*                          SWFShape gdImage2Shape                      */
/*                                                                      */
/*      Utility function to create an SWF filled shape using a gd       */
/*      image.                                                          */
/*                                                                      */
/*      Conversion functions were provided by :                         */
/*            Jan Hartmann <jhart@frw.uva.nl>                           */
/************************************************************************/
SWFShape gdImage2Shape(gdImagePtr img, imageObj *image)
{
    unsigned char *data, *dbldata;
    int size;
    int bytesPerColor;
    SWFShape oShape;

    if (!img)
        return NULL;

    data = gd2bitmap(img, &size, &bytesPerColor);
    dbldata = bitmap2dbl(data,&size,&bytesPerColor);
    free(data); /* memory leak cf ticket #2555 */
    oShape = bitmap2shape(dbldata, size, img->sx, img->sy, SWFFILL_SOLID, image);
    StoreDblData(dbldata, image); /* memory leak cf ticket #2555 */
    return oShape;
}
    
/************************************************************************/
/*                       SWFButton BuildButtonFromGD                    */
/*                                                                      */
/*      Return a button object using a GD image.                        */
/************************************************************************/
SWFButton BuildButtonFromGD(gdImagePtr img, imageObj *image)/* , colorObj *psHighlightColor) */
{
    SWFShape oShape;
    SWFButton oButton;

    /* TODO : highlight of the pixmap symbol */

    if (!img)
        return NULL;

    oShape = gdImage2Shape(img, image);
    StoreShape(oShape, image);
    
    oButton = newSWFButton();
    SWFButton_addShape(oButton, oShape, 
                       SWFBUTTON_UP | SWFBUTTON_HIT | SWFBUTTON_DOWN | SWFBUTTON_OVER);

    return oButton;
}

/************************************************************************/
/*                        SWFShape BuildEllipseShape                    */
/*                                                                      */
/*      Build an circle shape centered at nX, nY with the width and     */
/*      height being the circumfernce.                                  */
/************************************************************************/
SWFShape BuildEllipseShape(int nX, int nY, int nWidth, int nHeight,
                           colorObj *psFillColor, colorObj *psOutlineColor)
{
    SWFShape oShape;
    SWFFill oFill;

    oShape = newSWFShape();

    if (psFillColor == NULL && psOutlineColor == NULL)
        return NULL;

    if (psOutlineColor)
        SWFShape_setLine(oShape, 0, (byte)psOutlineColor->red, 
                         (byte)psOutlineColor->green, (byte)psOutlineColor->blue, 0xff);
    
    if (psFillColor) {
    	/* correct memory leak  by catching the SWFFill*/
    	oFill = SWFShape_addSolidFill(oShape, (byte)psFillColor->red, 
    			(byte)psFillColor->green, 
    	        (byte)psFillColor->blue, 
    	        0xff);
        SWFShape_setRightFill(oShape, oFill);
        /* and destroying it */
        destroySWFFill(oFill);
    }
    SWFShape_movePenTo(oShape, (float)(nX-(nWidth/2)), (float)nY);


    /* TODO : we should maybe use SWFShape_drawArc(shape, r, 0, 360) */

    /* Bottom Left */
    SWFShape_drawCurveTo(oShape, (float)(nX-(nWidth/2)), 
                         (float)(nY+(nHeight/2)),
                         (float)nX, (float)(nY+(nHeight/2))); 
    /* Bottom Right */
    SWFShape_drawCurveTo(oShape, (float)(nX + (nWidth/2)), 
                         (float)(nY+(nHeight/2)),
                         (float)(nX+(nWidth/2)), (float)nY);
    /* Top Right */
     SWFShape_drawCurveTo(oShape, (float)(nX +(nWidth/2)), 
                          (float)(nY-(nHeight/2)),
                          (float)nX, (float)(nY-(nHeight/2)));
     /* Top Left */
     SWFShape_drawCurveTo(oShape, (float)(nX - (nWidth/2)), 
                          (float)(nY-(nHeight/2)),
                          (float)(nX-(nWidth/2)), (float)nY);

     return oShape;
}
     


/************************************************************************/
/*                       SWFButton BuildEllipseButton                   */
/*                                                                      */
/*      Returns a button object containg a circle shape.                */
/************************************************************************/
SWFButton BuildEllipseButton(int nX, int nY, int nWidth, int nHeight,
                             colorObj *psFillColor, colorObj *psOutlineColor,
                             colorObj *psHightlightColor,
                             int nLayerIndex, int nShapeIndex, imageObj *image)
{
    SWFShape oShape;
    SWFButton oButton;

    if (nX < 0 || nY < 0 || nWidth < 0 || nHeight < 0 || 
        (psFillColor == NULL && psOutlineColor == NULL))
        return NULL;
   
        
    oShape = BuildEllipseShape(nX, nY, nWidth, nHeight, psFillColor,
                               psOutlineColor);
    StoreShape(oShape, image);
    
    oButton = newSWFButton();
    SWFButton_addShape(oButton, oShape, 
                       SWFBUTTON_UP | SWFBUTTON_HIT | SWFBUTTON_DOWN);

    if (psHightlightColor)
    {
        oShape = BuildEllipseShape(nX, nY, nWidth, nHeight, psHightlightColor,
                                   NULL);
        StoreShape(oShape, image);
    }
    SWFButton_addShape(oButton, oShape, SWFBUTTON_OVER);
    
    if (nLayerIndex >=0 && nShapeIndex >= 0)
    {
        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEUP);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEUP);

        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEDOWN);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEDOWN);

        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEOVER);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEOVER);
        
        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEOUT);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEOUT);
    }


    return oButton;
}


/************************************************************************/
/*                        SWFShape BuildPolygonShape                    */
/*                                                                      */
/*      Build a polygon shape.                                          */
/************************************************************************/
SWFShape BuildPolygonShape(shapeObj *p, colorObj *psFillColor, 
                           colorObj *psOutlineColor, int width)
{
    int i, j;
    SWFShape oShape;
    SWFFill oFill;

    if (p && p->numlines > 0 && (psFillColor !=NULL || psOutlineColor != NULL))
    {
        oShape = newSWFShape();
        if (psOutlineColor)
            SWFShape_setLine(oShape, width, (byte)psOutlineColor->red, 
                             (byte)psOutlineColor->green, (byte)psOutlineColor->blue, 0xff);
        if (psFillColor)
        {
        	oFill = SWFShape_addSolidFill(oShape, 
        			(byte)psFillColor->red, 
                    (byte)psFillColor->green, 
                    (byte)psFillColor->blue,
                    0xff);
            SWFShape_setRightFill(oShape, oFill);
            destroySWFFill(oFill);
       }
       for (i = 0; i < p->numlines; i++)
       {
           if (p->line[i].numpoints)
           {
               SWFShape_movePenTo(oShape, (float)p->line[i].point[0].x, 
                                  (float)p->line[i].point[0].y); 
           }
           for(j=1; j<p->line[i].numpoints; j++)
           {
               SWFShape_drawLineTo(oShape, (float)p->line[i].point[j].x,
                                    (float)p->line[i].point[j].y);
           }
       }                                                 
       return oShape;
    }
    return NULL;
}
/************************************************************************/
/*          SWFShape  BuildShape(gdPoint adfPoints[], int nPoints,      */
/*                           colorObj *psFillColor,                     */
/*                           colorObj *psOutlineColor)                  */
/*                                                                      */
/*      Build a polygon shape. Used for symbols                         */
/************************************************************************/
SWFShape  BuildShape(gdPoint adfPoints[], int nPoints,  
                     colorObj *psFillColor,
                     colorObj *psOutlineColor)
{
    SWFShape oShape = newSWFShape();
    SWFFill oFill;
    int i = 0;

    if (psFillColor == NULL && psOutlineColor == NULL)
        return NULL;

    if (psFillColor)
    {
        if (psOutlineColor)
            SWFShape_setLine(oShape, 0, (byte)psOutlineColor->red, 
                             (byte)psOutlineColor->green, (byte)psOutlineColor->blue, 0xff);
        oFill = SWFShape_addSolidFill(oShape, 
        		(byte)psFillColor->red, 
                (byte)psFillColor->green, 
                (byte)psFillColor->blue, 
                0xff);
        SWFShape_setRightFill(oShape, oFill);
        destroySWFFill(oFill);
        /*SWFShape_setRightFill(oShape, 
                              SWFShape_addSolidFill(oShape, 0xFF, 
                                                    0, 
                                                    0, 0xff));*/
    }
    else
        SWFShape_setLine(oShape, 5, (byte)psOutlineColor->red, 
                         (byte)psOutlineColor->green, 
                         (byte)psOutlineColor->blue, 0xff);
    

    SWFShape_movePenTo(oShape, (float)adfPoints[0].x, (float)adfPoints[0].y);
    for (i=1; i<nPoints; i++)
    {
        SWFShape_drawLineTo(oShape, (float)adfPoints[i].x, 
                            (float)adfPoints[i].y);
    }
    /* close the polygon */
    SWFShape_drawLineTo(oShape, (float)adfPoints[0].x, (float)adfPoints[0].y);

    return oShape;
}


/************************************************************************/
/*        SWFShape  BuildShapeLine(gdPoint adfPoints[], int nPoints,    */
/*                               colorObj *psColor)                     */
/*                                                                      */
/*      Build a shape line : used for symbols.                          */
/************************************************************************/
SWFShape  BuildShapeLine(gdPoint adfPoints[], int nPoints,  
                         colorObj *psColor)
                         
{
    SWFShape oShape = newSWFShape();
    int i = 0;
    
    if (psColor == NULL || nPoints <= 0)
        return NULL;

    SWFShape_setLine(oShape, 0, (byte)psColor->red, 
                     (byte)psColor->green, (byte)psColor->blue, 0xff);


    SWFShape_movePenTo(oShape, (float)adfPoints[0].x, (float)adfPoints[0].y);

    for (i=1; i<nPoints; i++)
    {
        if (adfPoints[i].x >= 0 && adfPoints[i].y >= 0)
        {
            if (adfPoints[i-1].x < 0 && adfPoints[i-1].y < 0)
            {
                SWFShape_movePenTo(oShape, (float)adfPoints[i].x, 
                                   (float)adfPoints[i].y);
            }
            else
                SWFShape_drawLineTo(oShape, (float)adfPoints[i].x, 
                                    (float)adfPoints[i].y);
        }
    }
    return oShape;
}


/************************************************************************/
/*                      SWFButton   BuildButtonPolygon                  */
/*                                                                      */
/*      Build a button and add a polygon shape in it.                   */
/*                                                                      */
/************************************************************************/
SWFButton   BuildButtonPolygon(gdPoint adfPoints[], int nPoints, 
                               colorObj *psFillColor, 
                               colorObj *psOutlineColor,
                               colorObj *psHighlightColor,
                               int nLayerIndex, int nShapeIndex, imageObj *image)
                               
{
    SWFButton oButton;
    SWFShape oShape;
    /* int bFill = 0; */


    oButton = newSWFButton();
    oShape = BuildShape(adfPoints, nPoints, psFillColor, psOutlineColor);
    StoreShape(oShape, image);
    SWFButton_addShape(oButton, oShape, 
                       SWFBUTTON_UP | SWFBUTTON_HIT | SWFBUTTON_DOWN);

    if (psHighlightColor)
    {
        if(psFillColor) 
        {
        	oShape = BuildShape(adfPoints, nPoints, psHighlightColor, NULL);
        	StoreShape(oShape, image);
            SWFButton_addShape(oButton, oShape, SWFBUTTON_OVER);
        }
        else if (psOutlineColor)
        {
        	oShape = BuildShape(adfPoints, nPoints, NULL, psHighlightColor);
        	StoreShape(oShape, image);
            SWFButton_addShape(oButton, oShape, SWFBUTTON_OVER);
        }
    }
    
    if (nLayerIndex >=0 && nShapeIndex >= 0)
    {
        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEUP);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEUP);

        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEDOWN);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEDOWN);

        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEOVER);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEOVER);
        
        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEOUT);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEOUT);
    }

    return oButton;
}



/************************************************************************/
/*                       SWFButton   BuildButtonLine                    */
/*                                                                      */
/*      Build a button and add a line shape in it.                      */
/************************************************************************/
SWFButton   BuildButtonLine(gdPoint adfPoints[], int nPoints, 
                            colorObj *psFillColor, 
                            colorObj *psHighlightColor,
                            int nLayerIndex, int nShapeIndex)
{
    SWFButton b;
    /* int bFill = 0; */

    b = newSWFButton();

    if (psFillColor == NULL)
        return NULL;

    SWFButton_addShape(b, BuildShapeLine(adfPoints, nPoints,  
                                         psFillColor),
                       SWFBUTTON_UP | SWFBUTTON_HIT | SWFBUTTON_DOWN);

    if (psHighlightColor)
    {
      SWFButton_addShape(b, BuildShapeLine(adfPoints, nPoints, 
                                           psHighlightColor), 
                         SWFBUTTON_OVER);
    }
    
    if (nLayerIndex >=0 && nShapeIndex >= 0)
    {
        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEUP);
        SWFButton_addAction(b, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEUP);

        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEDOWN);
        SWFButton_addAction(b, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEDOWN);

        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEOVER);
        SWFButton_addAction(b, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEOVER);
        
        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEOUT);
        SWFButton_addAction(b, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEOUT);
    }
    
    return b;
}

/************************************************************************/
/*                        imageObj *msImageCreateSWF                    */
/*                                                                      */
/*      Utility function to create an image object of SWF type          */
/************************************************************************/
imageObj *msImageCreateSWF(int width, int height, outputFormatObj *format,
                           char *imagepath, char *imageurl, mapObj *map)
{

    imageObj    *image = NULL;
    char        *driver = strdup("GD/GIF");

    assert( strcasecmp(format->driver,"SWF") == 0 );
    image = (imageObj *)calloc(1,sizeof(imageObj));

    image->format = format;
    format->refcount++;

    image->width = width;
    image->height = height;
    image->imagepath = NULL;
    image->imageurl = NULL;
    image->resolution = map->resolution;
    image->resolutionfactor = map->resolution/map->defresolution;

    if (imagepath)
    {
        image->imagepath = strdup(imagepath);
    }
    if (imageurl)
    {
        image->imageurl = strdup(imageurl);
    }

    image->img.swf = (SWFObj *)malloc(sizeof(SWFObj));    
    ((SWFObj *)image->img.swf)->map = map;

    ((SWFObj *)image->img.swf)->nCurrentLayerIdx = -1;
    ((SWFObj *)image->img.swf)->nCurrentShapeIdx = -1;

    ((SWFObj *)image->img.swf)->nLayerMovies = 0;
    ((SWFObj *)image->img.swf)->pasMovies = NULL;
    ((SWFObj *)image->img.swf)->nCurrentMovie = -1;

    ((SWFObj *)image->img.swf)->nFonts = 0;
    ((SWFObj *)image->img.swf)->Fonts = NULL;

    ((SWFObj *)image->img.swf)->nTexts = 0;
    ((SWFObj *)image->img.swf)->Texts = NULL;

    ((SWFObj *)image->img.swf)->nShapes = 0;
    ((SWFObj *)image->img.swf)->Shapes = NULL;
    
    ((SWFObj *)image->img.swf)->nBitmaps = 0;
    ((SWFObj *)image->img.swf)->Bitmaps = NULL;

    ((SWFObj *)image->img.swf)->nInputs = 0;
    ((SWFObj *)image->img.swf)->Inputs = NULL;

    ((SWFObj *)image->img.swf)->nButtons = 0;
    ((SWFObj *)image->img.swf)->Buttons = NULL;

    ((SWFObj *)image->img.swf)->nDblDatas = 0;
    ((SWFObj *)image->img.swf)->DblDatas = NULL;

    ((SWFObj *)image->img.swf)->panLayerIndex = NULL;
    ((SWFObj *)image->img.swf)->nTmpCount = 0;

    /* initalize main movie */
    ((SWFObj *)image->img.swf)->sMainMovie = newSWFMovie();
    SWFMovie_setDimension(((SWFObj *)image->img.swf)->sMainMovie, (float)width, 
                          (float)height);
    SWFMovie_setBackground(((SWFObj *)image->img.swf)->sMainMovie, map->imagecolor.red,
                           map->imagecolor.green, map->imagecolor.blue);
/* -------------------------------------------------------------------- */
/*      if the output is a single movie, we crate a GD image that       */
/*      will be used to conating the rendering of al the layers.        */
/* -------------------------------------------------------------------- */
    if (strcasecmp(msGetOutputFormatOption(image->format,"OUTPUT_MOVIE",""), 
                   "MULTIPLE") == 0)
    {
      ((SWFObj *)image->img.swf)->imagetmp = NULL;
    }
    else
    {
#ifdef USE_GD_GIF
        driver = strdup("GD/GIF");
#else  

#ifdef USE_GD_PNG
        driver = strdup("GD/PNG");
#else

#ifdef USE_GD_JPEG
        driver = strdup("GD/JPEG");
#else

#ifdef USE_GD_WBMP
        driver = strdup("GD/WBMP");
#endif 

#endif
#endif
#endif
     
        ((SWFObj *)image->img.swf)->imagetmp = (imageObj *) 
          msImageCreateGD(map->width, map->height,  
                          msCreateDefaultOutputFormat(map, driver),
                          map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution);
    }
    free(driver);
    return image;
}
    
/************************************************************************/
/*                        void msImageStartLayerSWF                     */
/*                                                                      */
/*      Strart of layer drawing. Create a new movie for the layer.      */
/*                                                                      */
/************************************************************************/
void msImageStartLayerSWF(mapObj *map, layerObj *layer, imageObj *image)
{
    int         nTmp = 0;
    char        szAction[200];
    SWFAction   oAction;
    int         i = 0;
    int         n = -1;
    char        **tokens;
    char        *metadata;

    if (image && MS_DRIVER_SWF(image->format))
    {
/* -------------------------------------------------------------------- */
/*      If the output is not multiple layers, do nothing.               */
/* -------------------------------------------------------------------- */
        if (strcasecmp(msGetOutputFormatOption(image->format,"OUTPUT_MOVIE",
                                               ""), 
                       "MULTIPLE") != 0)
          return;

        ((SWFObj *)image->img.swf)->nLayerMovies++;
        nTmp = ((SWFObj *)image->img.swf)->nLayerMovies;
        if (!((SWFObj *)image->img.swf)->pasMovies)
        {
            ((SWFObj *)image->img.swf)->pasMovies = 
                (SWFMovie *)malloc(sizeof(SWFMovie)*nTmp);
            ((SWFObj *)image->img.swf)->panLayerIndex = 
                (int *)malloc(sizeof(int)*nTmp);
            
        }
        else
        {
            ((SWFObj *)image->img.swf)->pasMovies = 
                (SWFMovie *)realloc(((SWFObj *)image->img.swf)->pasMovies,
                                    sizeof(SWFMovie)*nTmp);
            ((SWFObj *)image->img.swf)->panLayerIndex = 
                (int *)realloc(((SWFObj *)image->img.swf)->panLayerIndex,
                                    sizeof(int)*nTmp);
        }

        ((SWFObj *)image->img.swf)->nCurrentMovie = nTmp -1;
        ((SWFObj *)image->img.swf)->pasMovies[nTmp -1] = newSWFMovie();
    
        /* keeps the layer index for each movie so we can use */
        /* it to get a movie index based on the layer index. */
        ((SWFObj *)image->img.swf)->panLayerIndex[nTmp -1] = layer->index;

        SWFMovie_setDimension(((SWFObj *)image->img.swf)->pasMovies[nTmp -1], 
                              (float)image->width, (float)image->height);
         SWFMovie_setBackground(((SWFObj *)image->img.swf)->pasMovies[nTmp -1], map->imagecolor.red,
                           map->imagecolor.green, map->imagecolor.blue);
        ((SWFObj *)image->img.swf)->nCurrentLayerIdx = layer->index;
        /* msLayerGetItems(layer); */

/* -------------------------------------------------------------------- */
/*      Start the Element array that will contain the values of the     */
/*      attributes.                                                     */
/* -------------------------------------------------------------------- */
        if( (metadata=msLookupHashTable(&(layer->metadata),"SWFDUMPATTRIBUTES"))
             != NULL )
        {
            tokens = msStringSplit(metadata, ',', &n);
            if (tokens && n > 0)
            {
                sprintf(gszAction, "nAttributes=%d;", n);
                oAction = compileSWFActionCode(gszAction);
                SWFMovie_add(((SWFObj *)image->img.swf)->pasMovies[nTmp -1], oAction);

                sprintf(gszAction, "%s", "Attributes=new Array();");
                oAction = compileSWFActionCode(gszAction);
                SWFMovie_add(((SWFObj *)image->img.swf)->pasMovies[nTmp -1], oAction);
                
                for (i=0; i<n; i++)
                {
                    sprintf(gszAction, "Attributes[%d]=\"%s\";", i, tokens[i]);
                    oAction = compileSWFActionCode(gszAction);
                    SWFMovie_add(((SWFObj *)image->img.swf)->pasMovies[nTmp -1], oAction);
                }
                
                sprintf(szAction, "%s", "Element=new Array();");
                oAction = compileSWFActionCode(szAction);
                SWFMovie_add(((SWFObj *)image->img.swf)->pasMovies[nTmp -1], oAction);
                msFreeCharArray(tokens, n);
            }
        }
    }
        
}


/************************************************************************/
/*                         void msDrawStartShapeSWF                     */
/*                                                                      */
/*      Strat rendering a shape.                                        */
/************************************************************************/
void msDrawStartShapeSWF(mapObj *map, layerObj *layer, imageObj *image,
                         shapeObj *shape)
{
    char *metadata = NULL;
    int  *panIndex = NULL;
    int  iIndex = 0;
    int i,j = 0;
    int bFound = 0;
    SWFAction   oAction;
    /* int nTmp = 0; */
    
    if (image &&  MS_DRIVER_SWF(image->format))
    {
        ((SWFObj *)image->img.swf)->nCurrentShapeIdx = shape->index;

        /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */

/* -------------------------------------------------------------------- */
/*      get an array of indexes corresponding to the attributes. We     */
/*      will use this array to retreive the values.                     */
/* -------------------------------------------------------------------- */
        if( (metadata=msLookupHashTable(&(layer->metadata),"SWFDUMPATTRIBUTES")) 
            != NULL )
        {
            char **tokens;
            int n = 0;
            tokens = msStringSplit(metadata, ',', &n);
            if (tokens && n > 0)
            {
                panIndex = (int *)malloc(sizeof(int)*n);
                iIndex = 0;
                for (i=0; i<n; i++)
                {
                    bFound = 0;
                    for (j=0; j<layer->numitems; j++)
                    {
                        if (strcmp(tokens[i], layer->items[j]) == 0)
                        {
                            bFound = 1;
                            break;
                        }
                    }
                    if (bFound)
                    {
                        panIndex[iIndex] = j;
                        iIndex++;
                    }
                }
            }
        }
/* -------------------------------------------------------------------- */
/*      Build the value string for the specified attributes.            */
/* -------------------------------------------------------------------- */
        if (panIndex)
        {
            sprintf(gszAction, "Element[%d]=new Array();", (int)shape->index);
            oAction = compileSWFActionCode(gszAction);
            /* SWFMovie_add(((SWFObj *)image->img.swf)->pasMovies[nTmp], oAction); */
            SWFMovie_add(GetCurrentMovie(map, image), oAction);

            for (i=0; i<iIndex; i++)
            {
                sprintf(gszAction, "Element[%d][%d]=\"%s\";", (int)shape->index,
                        i, shape->values[panIndex[i]]);
                oAction = compileSWFActionCode(gszAction);
                /* SWFMovie_add(((SWFObj *)image->img.swf)->pasMovies[nTmp], oAction); */
                SWFMovie_add(GetCurrentMovie(map, image), oAction);
                
            }
        }
    }
    else
        ((SWFObj *)image->img.swf)->nCurrentShapeIdx = -1;   
}


void msDrawStartShapeUsingIdxSWF(mapObj *map, layerObj *layer, imageObj *image,
                                 labelCacheMemberObj * cachePtr)
{
    shapeObj shape;
    int retval = 0;
    if (map && layer && image && cachePtr->shapeindex >=0)
    {
        msInitShape(&shape);
        retval = msLayerResultsGetShape(layer, &shape, cachePtr->tileindex,
                                 cachePtr->shapeindex); 
	  
        if (retval == MS_SUCCESS) 
        {
            msDrawStartShapeSWF(map, layer, image, &shape);
        } 
        else 
        {
            msSetError(MS_MISCERR, "Cannot find shape for shapeidx:%d", 
                       "msDrawStartShapeUsingIdxSWF()", cachePtr->shapeindex);
        }
    }
}
 





void AddMouseActions(SWFButton oButton, int nLayerIndex, int nShapeIndex)
{
    if (nLayerIndex >=0 && nShapeIndex >= 0)
    {
        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEUP);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEUP);

        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEDOWN);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEDOWN);

        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEOVER);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEOVER);
        
        sprintf(gszAction, "_root.ElementSelected(%d,%d,%d);", nLayerIndex, 
                nShapeIndex, MOUSEOUT);
        SWFButton_addAction(oButton, compileSWFActionCode(gszAction),
                            SWFBUTTON_MOUSEOUT);
    }
}

/* Set the name on a SWFButton to button_<layerid>_<shapeid>. 
 * It seems that the only way to make a button reachable/modifiable by 
 * name via Action Script is to embed it in its own little clip. 
 * Setting the name directly on the button itself results in 
 * selecting/modifying the whole layer.
 * See ticket #2336 for all the details.
 *
 * Returns a handle on the clip's SWFDisplayItem.
 */
static SWFDisplayItem _msSWFAddButtonWithId(imageObj *image, 
                                            SWFMovie oMainMovie, 
                                            SWFButton oButton, 
                                            int nLayerIndex, int nShapeIndex)
{
    SWFMovieClip myButtonMovieClip;
    SWFDisplayItem oDisplay;

    myButtonMovieClip = newSWFMovieClip();
    SWFMovieClip_add(myButtonMovieClip, oButton);
    SWFMovieClip_nextFrame(myButtonMovieClip);

    oDisplay = SWFMovie_add(oMainMovie, myButtonMovieClip);

    if (oDisplay)
    {
        sprintf(gszTmp, "button_%d_%d",nLayerIndex, nShapeIndex);
        ((SWFObj *)image->img.swf)->nTmpCount++;
        SWFDisplayItem_setName(oDisplay, gszTmp);
    }

    return oDisplay;
}

/************************************************************************/
/*                          msDrawMarkerSymbolSWF                       */
/*                                                                      */
/*      Draw symbols in an SWF Movie.                                   */
/************************************************************************/
void msDrawMarkerSymbolSWF(symbolSetObj *symbolset, imageObj *image, 
                           pointObj *p, styleObj *style, double scalefactor)
                           
{
    symbolObj *symbol;
    int offset_x, offset_y;
    double x, y;
    int size;
    int j;
    gdPoint mPoints[MS_MAXVECTORPOINTS];

    double scale=1.0;

    rectObj rect;
    char *font=NULL;
    int bbox[8];

    gdImagePtr imgtmp = NULL;

    SWFButton   oButton;
    SWFDisplayItem oDisplay = NULL;


    colorObj    sFc;
    colorObj    sBc;
    colorObj    sOc;
    colorObj    sColorHighlightObj;

    colorObj *psFillColor = NULL;
    colorObj *psOutlineColor = NULL;
    
    mapObj      *map;
    layerObj    *psLayerTmp = NULL;

    /* int nTmp = 0; */
    

    int nLayerIndex = -1;
    int nShapeIndex = -1;

    int fc = 0; /* only used for TTF  */

/* -------------------------------------------------------------------- */
/*      if not SWF, return.                                             */
/* -------------------------------------------------------------------- */
    if (image == NULL || !MS_DRIVER_SWF(image->format) )
        return;

    symbol = symbolset->symbol[style->symbol];
    if(style->size == -1) {
        size = (int)msSymbolGetDefaultSize( symbol );
        size = MS_NINT(size*scalefactor);
    }
    else
        size = MS_NINT(style->size*scalefactor);
    size = MS_MAX(size, style->minsize*image->resolutionfactor);
    size = MS_MIN(size, style->maxsize*image->resolutionfactor);


    if(style->symbol > symbolset->numsymbols || style->symbol < 0) /* no such symbol, 0 is OK */
        return;

    /* if(fc >= gdImageColorsTotal(img)) */ /* invalid color, -1 is valid */
    /* return; */

    if(size < 1) /* size too small */
        return;

/* -------------------------------------------------------------------- */
/*      extract the colors.                                             */
/* -------------------------------------------------------------------- */
    map = ((SWFObj *)image->img.swf)->map;
    sFc.red = style->color.red;
    sFc.green = style->color.green;
    sFc.blue = style->color.blue;
    
    sBc.red = style->backgroundcolor.red;
    sBc.green = style->backgroundcolor.green;
    sBc.blue = style->backgroundcolor.blue;

    sOc.red = style->outlinecolor.red;
    sOc.green = style->outlinecolor.green;
    sOc.blue = style->outlinecolor.blue;

    

    /* this comes from the map file querymap obj. */
    
    sColorHighlightObj.red = map->querymap.color.red;
    sColorHighlightObj.green = map->querymap.color.green;
    sColorHighlightObj.blue = map->querymap.color.blue;


/* -------------------------------------------------------------------- */
/*      the layer index and shape index will be set if the layer has    */
/*      the metadata SWFDUMPATTRIBUTES set.                             */
/*                                                                      */
/*      If they are set, we will write the Action Script (AS) for       */
/*      the attributes of the shape.                                    */
/* -------------------------------------------------------------------- */
    psLayerTmp = 
      ((GET_LAYER(((SWFObj *)image->img.swf)->map, ((SWFObj *)image->img.swf)->nCurrentLayerIdx)));

    if (msLookupHashTable(&(psLayerTmp->metadata), "SWFDUMPATTRIBUTES"))
    {
        nLayerIndex = ((SWFObj *)image->img.swf)->nCurrentLayerIdx;
        nShapeIndex = ((SWFObj *)image->img.swf)->nCurrentShapeIdx;
    }
    
/* -------------------------------------------------------------------- */
/*      Render the diffrent type of symbols.                            */
/* -------------------------------------------------------------------- */
    /* symbol = &(symbolset->symbol[sy]); */

    /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */

    switch(symbol->type) 
    {  
/* -------------------------------------------------------------------- */
/*      Symbol : true type.                                             */
/* -------------------------------------------------------------------- */
        case(MS_SYMBOL_TRUETYPE):

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
            font = msLookupHashTable(&(symbolset->fontset->fonts), symbol->font);
            if(!font) 
                return;
            
            if(msGetCharacterSize(symbol->character, size, font, &rect) != MS_SUCCESS) 
                return;
            
            x = p->x - (rect.maxx - rect.minx)/2 - rect.minx;
            y = p->y - rect.maxy + (rect.maxy - rect.miny)/2;  

            imgtmp = gdImageCreate((int)((rect.maxx - rect.minx)+2),
                                   (int)((rect.maxy - rect.miny)+2));

             /* gdImageStringFT(img, bbox, ((symbol->antialias)?(fc):-(fc)),  */
             /* font, sz, 0, x, y, symbol->character); */
            gdImageStringFT(imgtmp, bbox, ((symbol->antialias)?(fc):-(fc)), 
                            font, size, 0, 1, 1, symbol->character);
            /* oShape = gdImage2Shape(imgtmp); */
            oButton = BuildButtonFromGD(imgtmp, image);/* , NULL); */
            StoreButton(oButton, image);
            AddMouseActions(oButton, nLayerIndex, nShapeIndex);
            oDisplay = _msSWFAddButtonWithId(image,
                                             GetCurrentMovie(map, image),
                                             oButton, 
                                             nLayerIndex, nShapeIndex);
            SWFDisplayItem_moveTo(oDisplay, (float)x, (float)y);
            break;
#endif

/* -------------------------------------------------------------------- */
/*      Symbol : pixmap.                                                */
/* -------------------------------------------------------------------- */
        case(MS_SYMBOL_PIXMAP):
            if(size == 1) 
            { /* don't scale */
                offset_x = MS_NINT(p->x - .5*symbol->img->sx);
                offset_y = MS_NINT(p->y - .5*symbol->img->sy);
                /* gdImageCopy(img, symbol->img, offset_x, offset_y, 0, 0,  */
                /* symbol->img->sx, symbol->img->sy); */
                oButton = BuildButtonFromGD(symbol->img, image);/* , NULL); */
                StoreButton(oButton, image);
		AddMouseActions(oButton, nLayerIndex, nShapeIndex);

                oDisplay = _msSWFAddButtonWithId(image,
                                                 GetCurrentMovie(map, image),
                                                 oButton, 
                                                 nLayerIndex, nShapeIndex);
                SWFDisplayItem_moveTo(oDisplay, (float)offset_x, 
                                      (float)offset_y);
                
            } 
            else 
            {
                scale = size/symbol->img->sy;
                offset_x = MS_NINT(p->x - .5*symbol->img->sx*scale);
                offset_y = MS_NINT(p->y - .5*symbol->img->sy*scale);

                imgtmp = gdImageCreate((int)(symbol->img->sx*scale), 
                                       (int)(symbol->img->sy*scale));
                
                gdImageCopyResized(imgtmp, symbol->img, 0, 0, 0, 0,
                                   (int)(symbol->img->sx*scale), 
                                   (int)(symbol->img->sy*scale), symbol->img->sx, 
                                   symbol->img->sy);
            
                /* out = fopen("c:/tmp/ms_tmp/test.jpg", "wb"); */
                /* And save the image  -- could also use gdImageJpeg */
                /* gdImageJpeg(imgtmp, out, 0); */

                oButton = BuildButtonFromGD(imgtmp, image);/* , NULL); */
                StoreButton(oButton, image);
		AddMouseActions(oButton, nLayerIndex, nShapeIndex);

                oDisplay = _msSWFAddButtonWithId(image,
                                                 GetCurrentMovie(map, image),
                                                 oButton, 
                                                 nLayerIndex, nShapeIndex);
                
                SWFDisplayItem_moveTo(oDisplay, (float)offset_x, 
                                      (float)offset_y);

                /* gdImageDestroy(imgtmp); */
            }
            break;

/* -------------------------------------------------------------------- */
/*      symbol : Ellipse                                                */
/* -------------------------------------------------------------------- */
        case(MS_SYMBOL_ELLIPSE):
 
            scale = size/symbol->sizey;
            x = MS_NINT(symbol->sizex*scale)+1;
            y = MS_NINT(symbol->sizey*scale)+1;
            offset_x = MS_NINT(p->x - .5*x);
            offset_y = MS_NINT(p->y - .5*x);

            psFillColor = NULL;
            psOutlineColor = NULL;

            if (MS_VALID_COLOR(sFc))
                psFillColor = &sFc;
            if (MS_VALID_COLOR(sOc))
                psOutlineColor = &sOc;

            if(MS_VALID_COLOR(sOc)) 
            {
                if (!symbol->filled)
                    psFillColor = NULL;
                oButton = 
                  BuildEllipseButton(offset_x, offset_y,
                                     MS_NINT(scale*symbol->points[0].x),
                                     MS_NINT(scale*symbol->points[0].y),
                                     psFillColor, psOutlineColor,
                                     &sColorHighlightObj,
                                     nLayerIndex, nShapeIndex, image);
                if (oButton)
                {
                    /* Ticket #2555 memory leak : need to store buttons to properly destroy them */
                    StoreButton(oButton, image);
                    oDisplay = _msSWFAddButtonWithId(image,
                                                     GetCurrentMovie(map, image),
                                                     oButton, 
                                                     nLayerIndex, nShapeIndex);
                }
            
            } 
            else 
            {
                if(MS_VALID_COLOR(sFc)) 
                {
                    oButton = 
                        BuildEllipseButton(offset_x, offset_y,
                                           MS_NINT(scale*symbol->points[0].x),
                                           MS_NINT(scale*symbol->points[0].y),
                                           psFillColor, NULL, 
                                           &sColorHighlightObj,
                                           nLayerIndex, nShapeIndex, image);
                    if (oButton)
                    {
                        StoreButton(oButton, image);
                        oDisplay = _msSWFAddButtonWithId(image,
                                                         GetCurrentMovie(map, image),
                                                         oButton, 
                                                         nLayerIndex, nShapeIndex);
                    }
                }
            }
            break;

/* -------------------------------------------------------------------- */
/*      symbol : Vector.                                                */
/* -------------------------------------------------------------------- */
        case(MS_SYMBOL_VECTOR):

            scale = size/symbol->sizey;

            offset_x = MS_NINT(p->x - scale*.5*symbol->sizex);
            offset_y = MS_NINT(p->y - scale*.5*symbol->sizey);
    
/* -------------------------------------------------------------------- */
/*      Symbol Vector : Filled.                                         */
/* -------------------------------------------------------------------- */
            if(symbol->filled) 
            { 
                for(j=0;j < symbol->numpoints;j++) 
                {
                    mPoints[j].x = MS_NINT(scale*symbol->points[j].x + offset_x);
                    mPoints[j].y = MS_NINT(scale*symbol->points[j].y + offset_y);
                }
                
                psFillColor = NULL;
                psOutlineColor = NULL;

                if (MS_VALID_COLOR(sFc))
                    psFillColor = &sFc;
                if (MS_VALID_COLOR(sOc))
                    psOutlineColor = &sOc;
                
                oButton = BuildButtonPolygon(mPoints, symbol->numpoints,  
                                             psFillColor,  psOutlineColor,
                                             &sColorHighlightObj,
                                             nLayerIndex, nShapeIndex, image);
                StoreButton(oButton, image);

                oDisplay = _msSWFAddButtonWithId(image,
                                                 GetCurrentMovie(map, image),
                                                 oButton, 
                                                 nLayerIndex, nShapeIndex);

            }
/* -------------------------------------------------------------------- */
/*      symbol VECTOR : not filled.                                     */
/* -------------------------------------------------------------------- */
            else  
            {
                if(!MS_VALID_COLOR(sFc)) 
                  return;
      
                for(j=0;j < symbol->numpoints;j++) 
                {
                    mPoints[j].x = MS_NINT(scale*symbol->points[j].x + offset_x);
                    mPoints[j].y = MS_NINT(scale*symbol->points[j].y + offset_y);
                }

                psFillColor = &sFc;

                oButton = BuildButtonLine(mPoints, symbol->numpoints,  
                                          psFillColor,
                                          &sColorHighlightObj,
                                          nLayerIndex, nShapeIndex);
		StoreButton(oButton, image);
                oDisplay = _msSWFAddButtonWithId(image,
                                                 GetCurrentMovie(map, image),
                                                 oButton, 
                                                 nLayerIndex, nShapeIndex);

            } /* end if-then-else */

            break;
        default:
            break;
    } /* end switch statement */

    return;
}


/************************************************************************/
/*        SWFShape DrawShapePolyline(shapeObj *p, colorObj *psColor)    */
/*                                                                      */
/*      Draws a simple line using the color passed as argument.         */
/************************************************************************/
SWFShape DrawShapePolyline(shapeObj *p, colorObj *psColor, int pixwidth)
{
    int i, j;
    SWFShape oShape = NULL;
    int width = 0;

    /*the flash specification indicates a scale of twenty units per pixel
    but it does not to work like that at least when loading the swf
    in a browser. So for now just set the width value, It */
    width = pixwidth;/* 20 */

    if (p && psColor && p->numlines > 0)
    {
        oShape = newSWFShape();
        SWFShape_setLine(oShape, width, (byte)psColor->red, 
                         (byte)psColor->green, (byte)psColor->blue, 0xff);
        
        

        for (i = 0; i < p->numlines; i++)
        {
            SWFShape_movePenTo(oShape, (float)p->line[i].point[0].x, 
                               (float)p->line[i].point[0].y); 

            for(j=1; j<p->line[i].numpoints; j++)
            {
                SWFShape_drawLineTo(oShape, (float)p->line[i].point[j].x,
                                    (float)p->line[i].point[j].y);
            }
        }
    }
    return oShape;
}


/************************************************************************/
/*                          DrawShapeFilledPolygon                      */
/*                                                                      */
/*      Draws a filled polygon using the fill and outline color.        */
/************************************************************************/
SWFShape DrawShapeFilledPolygon(shapeObj *p, colorObj *psFillColor, 
                                colorObj *psOutlineColor, int width)
{
    return BuildPolygonShape(p, psFillColor, psOutlineColor, width);
}
 


/************************************************************************/
/*                         DrawButtonFilledPolygon                      */
/*                                                                      */
/*      returns a button with a polygon shape in it.                    */
/************************************************************************/
SWFButton DrawButtonFilledPolygon(shapeObj *p, colorObj *psFillColor, 
                                  colorObj *psOutlineColor, 
                                  colorObj *psHighlightColor, int nLayerIndex,
                                  int nShapeIndex, int width)
{
    SWFButton b;
    
    b = newSWFButton();

    SWFButton_addShape(b, BuildPolygonShape(p, psFillColor, psOutlineColor, width),
                       SWFBUTTON_UP | SWFBUTTON_HIT | SWFBUTTON_DOWN);

    if (psHighlightColor)
    {
        if(psFillColor)
            SWFButton_addShape(b, BuildPolygonShape(p, psHighlightColor, NULL,
                                                    width), 
                               SWFBUTTON_OVER);
        else if (psOutlineColor)
             SWFButton_addShape(b, BuildPolygonShape(p, NULL, psHighlightColor, 
                                                     width), 
                                SWFBUTTON_OVER);
    }

    if (nLayerIndex >=0 && nShapeIndex >= 0)
    {
        /* sprintf(gszAction, "_root.ElementSelected(%d,%d);", nLayerIndex,  */
        /* nShapeIndex); */
        /* SWFButton_addAction(b, compileSWFActionCode(gszAction), */
        /* SWFBUTTON_MOUSEDOWN); */
        AddMouseActions(b, nLayerIndex, nShapeIndex);
    }

     return b;
}


SWFButton DrawButtonPolyline(shapeObj *p, colorObj *psColor, 
                             colorObj *psHighlightColor, int nLayerIndex,
                             int nShapeIndex, int width)
{
    SWFButton b;
    
    b = newSWFButton();

    SWFButton_addShape(b, DrawShapePolyline(p, psColor, width),
                       SWFBUTTON_UP | SWFBUTTON_HIT | SWFBUTTON_DOWN);

    if (psHighlightColor)
    {
        SWFButton_addShape(b, DrawShapePolyline(p, psHighlightColor, width),
                           SWFBUTTON_OVER);
    }

    if (nLayerIndex >=0 && nShapeIndex >= 0)
    {
        /* sprintf(gszAction, "_root.ElementSelected(%d,%d);", nLayerIndex,  */
        /* nShapeIndex); */
        /* SWFButton_addAction(b, compileSWFActionCode(gszAction), */
        /* SWFBUTTON_MOUSEDOWN); */
        AddMouseActions(b, nLayerIndex, nShapeIndex);
    }

     return b;
}

/************************************************************************/
/*    SWFText DrawText(char *string, int nX, int nY, char *pszFontFile, */
/*                       double dfSize, colorObj *psColor)              */
/*                                                                      */
/*      Draws a text of size dfSize and color psColor at position       */
/*      nX,nY using the font pszFontFile.                               */
/************************************************************************/
SWFText DrawText(char *string, int nX, int nY, char *pszFontFile, 
                 double dfSize, colorObj *psColor, imageObj *image)
{
    SWFText     oText = NULL;
    SWFFont     oFont = NULL;
    FILE        *f = NULL;
 
    if (!string || !pszFontFile || !psColor)
        return NULL;

    f = fopen(pszFontFile, "rb"); 
    if (f)
    {
        oFont  = loadSWFFontFromFile(f);
        /* oFont = newSWFFont("_sans"); */
        fclose(f); 
        if (oFont)
        {     
            /* ticket #2555 memory leak */
            StoreFont(oFont, image);
	    
	    oText = newSWFText();
            SWFText_setFont(oText, oFont);
            SWFText_moveTo(oText, (float)nX, (float)nY);
            SWFText_setColor(oText, (byte)psColor->red, (byte)psColor->green, (byte)psColor->blue, 
                             0xff);
            SWFText_setHeight(oText, (float)dfSize);
            SWFText_addString(oText, string, NULL);
   
            return oText;
        }
    }
    return NULL;
}



/************************************************************************/
/*                         void msDrawLineSymbolSWF                     */
/*                                                                      */
/*      Draws a polyline.                                               */
/*                                                                      */
/*      TODO : lines with symbols is not yet implemented.               */
/************************************************************************/
void msDrawLineSymbolSWF(symbolSetObj *symbolset, imageObj *image, shapeObj *p, 
                         styleObj *style, double scalefactor)
{
    colorObj    sFc;
    colorObj    sBc;
    colorObj    sColorHighlightObj;
    mapObj      *map = NULL;
    /* int         nTmp = 0; */
    SWFShape    oShape;
    SWFButton   oButton;
    SWFDisplayItem oDisplay = NULL;
    int         nLayerIndex = -1;
    int         nShapeIndex = -1;
    layerObj    *psLayerTmp = NULL;
    int         size = 0;
    symbolObj *symbol;
    int width;

/* -------------------------------------------------------------------- */
/*      if not SWF, return.                                             */
/* -------------------------------------------------------------------- */
    if (image == NULL || !MS_DRIVER_SWF(image->format) )
        return;

    if(p == NULL || p->numlines <= 0)
      return;

    symbol = symbolset->symbol[style->symbol];
    if(style->size == -1) {
        size = (int)msSymbolGetDefaultSize( symbol );
        size = MS_NINT(size*scalefactor);
    }
    else
        size = MS_NINT(style->size*scalefactor);

    /* TODO: Don't get this modification, is it needed elsewhere? */
    if(size*scalefactor > style->maxsize*image->resolutionfactor) scalefactor = (float)style->maxsize*image->resolutionfactor/(float)size;
    if(size*scalefactor < style->minsize*image->resolutionfactor) scalefactor = (float)style->minsize*image->resolutionfactor/(float)size;
    size = MS_NINT(size*scalefactor);
    size = MS_MAX(size, style->minsize*image->resolutionfactor);
    size = MS_MIN(size, style->maxsize*image->resolutionfactor);

    width = MS_NINT(style->width*scalefactor);
    width = MS_MAX(width, style->minwidth*image->resolutionfactor);
    width = MS_MIN(width, style->maxwidth*image->resolutionfactor);

    if(style->symbol > symbolset->numsymbols || style->symbol < 0) /* no such symbol, 0 is OK */
      return;
    

    if (!MS_VALID_COLOR( style->color))
      return;

/* -------------------------------------------------------------------- */
/*      extract the colors.                                             */
/* -------------------------------------------------------------------- */
    map = ((SWFObj *)image->img.swf)->map;

    sFc.red = style->color.red;
    sFc.green = style->color.green;
    sFc.blue = style->color.blue;
    
    sBc.red = style->backgroundcolor.red;
    sBc.green = style->backgroundcolor.green;
    sBc.blue = style->backgroundcolor.blue;


    /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */

/* -------------------------------------------------------------------- */
/*      the layer index and shape index will be set if the layer has    */
/*      the metadata SWFDUMPATTRIBUTES set.                             */
/*                                                                      */
/*      If they are set, we will write the Action Script (AS) for       */
/*      the attributes of the shape.                                    */
/* -------------------------------------------------------------------- */
    psLayerTmp = 
      (GET_LAYER(((SWFObj *)image->img.swf)->map, ((SWFObj *)image->img.swf)->nCurrentLayerIdx));

    if (msLookupHashTable(&(psLayerTmp->metadata), "SWFDUMPATTRIBUTES"))
    {
        nLayerIndex = ((SWFObj *)image->img.swf)->nCurrentLayerIdx;
        nShapeIndex = ((SWFObj *)image->img.swf)->nCurrentShapeIdx;
    }

    /* this comes from the map file querymap obj. */
    sColorHighlightObj.red = map->querymap.color.red;
    sColorHighlightObj.green =  map->querymap.color.green;
    sColorHighlightObj.blue = map->querymap.color.blue;

    /* For now just draw lines without symbols. */
    /* if(sy == 0)  */
    /* {  */
        /* just draw a single width line */
        if (nLayerIndex < 0 || nShapeIndex < 0)
        {
            oShape = DrawShapePolyline(p, &sFc, width);
            StoreShape(oShape, image);
            SWFMovie_add(GetCurrentMovie(map, image), oShape);
        }
        else
        {
            oButton = DrawButtonPolyline(p, &sFc, &sColorHighlightObj, nLayerIndex, 
                                         nShapeIndex, width);
	    StoreButton(oButton, image);
            oDisplay = _msSWFAddButtonWithId(image,
                                             GetCurrentMovie(map, image),
                                             oButton, 
                                             nLayerIndex, nShapeIndex);
        }
        return;
        /* } */

    /* TODO : lines with symbols */
}



/************************************************************************/
/*                           msDrawShadeSymbolSWF                       */
/*                                                                      */
/*      Draw polygon features. Only supports solid filled polgons.      */
/************************************************************************/
void msDrawShadeSymbolSWF(symbolSetObj *symbolset, imageObj *image, 
                          shapeObj *p, styleObj *style, double scalefactor)
{
    colorObj    sFc;
    colorObj    sBc;
    colorObj    sOc;
    mapObj      *map = NULL;
    layerObj    *psLayerTmp = NULL;
    SWFShape    oShape;
    SWFButton   oButton;
    SWFDisplayItem oDisplay;

    /* int         nTmp = 0; */
    colorObj    *psFillColor = NULL;
    colorObj    *psOutlineColor = NULL;

    colorObj    sColorHighlightObj;

    gdImagePtr  tile = NULL;
    /* int         bDestroyImage = 0; */
    unsigned char *data, *dbldata;
    int         size;
    int         bytesPerColor;
    
    int         nLayerIndex = -1;
    int         nShapeIndex = -1;
    symbolObj   *symbol;
    int         width;

/* -------------------------------------------------------------------- */
/*      if not SWF, return.                                             */
/* -------------------------------------------------------------------- */
    if (image == NULL || !MS_DRIVER_SWF(image->format) )
      return;
    
    if(p == NULL || p->numlines <= 0)
      return;

    symbol = symbolset->symbol[style->symbol];
    if(style->size == -1) {
      size = (int)msSymbolGetDefaultSize( symbol );
      size = MS_NINT(size*scalefactor);
    }
    else
        size = MS_NINT(style->size*scalefactor);
    size = MS_MAX(size, style->minsize*image->resolutionfactor);
    size = MS_MIN(size, style->maxsize*image->resolutionfactor);


    width = MS_NINT(style->width*scalefactor);
    width = MS_MAX(width, style->minwidth*image->resolutionfactor);
    width = MS_MIN(width, style->maxwidth*image->resolutionfactor);

    if(style->symbol > symbolset->numsymbols || style->symbol < 0) /* no such symbol, 0 is OK */
        return;

    /* if(fc >= gdImageColorsTotal(img)) */ /* invalid color, -1 is valid */
    /* return; */

    if(size < 1) /* size too small */
        return;

/* -------------------------------------------------------------------- */
/*      the layer index and shape index will be set if the layer has    */
/*      the metadata SWFDUMPATTRIBUTES set.                             */
/*                                                                      */
/*      If they are set, we will write the Action Script (AS) for       */
/*      the attributes of the shape.                                    */
/* -------------------------------------------------------------------- */
    psLayerTmp = 
      (GET_LAYER(((SWFObj *)image->img.swf)->map, ((SWFObj *)image->img.swf)->nCurrentLayerIdx));

    if (msLookupHashTable(&(psLayerTmp->metadata), "SWFDUMPATTRIBUTES"))
    {
        nLayerIndex = ((SWFObj *)image->img.swf)->nCurrentLayerIdx;
        nShapeIndex = ((SWFObj *)image->img.swf)->nCurrentShapeIdx;
    }
 
/* -------------------------------------------------------------------- */
/*      extract the colors.                                             */
/* -------------------------------------------------------------------- */
    map = ((SWFObj *)image->img.swf)->map;

    sFc.red = style->color.red;
    sFc.green = style->color.green;
    sFc.blue = style->color.blue;
    
    sBc.red = style->backgroundcolor.red;
    sBc.green = style->backgroundcolor.green;
    sBc.blue = style->backgroundcolor.blue;

    sOc.red = style->outlinecolor.red;
    sOc.green = style->outlinecolor.green;
    sOc.blue = style->outlinecolor.blue;
    

    /* TODO : this should come from the map file. */
    sColorHighlightObj.red = map->querymap.color.red;
    sColorHighlightObj.green = map->querymap.color.green;
    sColorHighlightObj.blue =  map->querymap.color.blue;

    if (MS_VALID_COLOR(sFc))
        psFillColor = &sFc;
    if (MS_VALID_COLOR(sOc))
        psOutlineColor = &sOc;

    /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */
    
    if (size == 0)
    {
        if (nLayerIndex < 0 ||  nShapeIndex < 0)
        {
            oShape = DrawShapeFilledPolygon(p, psFillColor, psOutlineColor, 
                                            width);
            StoreShape(oShape, image);
            SWFMovie_add(GetCurrentMovie(map, image), oShape);
        }
        else
        {
            oButton = DrawButtonFilledPolygon(p, psFillColor, psOutlineColor,
                                              &sColorHighlightObj, nLayerIndex, 
                                              nShapeIndex, width);
            StoreButton(oButton, image);
            oDisplay = _msSWFAddButtonWithId(image,
                                             GetCurrentMovie(map, image),
                                             oButton, 
                                             nLayerIndex, nShapeIndex);
        }

        return;
    }

/* ==================================================================== */
/*      The idea is to create the tile image and fill the shape with it.*/
/*                                                                      */
/*      Won't work for now : files created are huge.                    */
/* ==================================================================== */

    /* tile = getTileImageFromSymbol(map, symbolset, sy, fc, bc, oc, sz, */
    /* &bDestroyImage); */

    if (tile)
    {
        /* out = fopen("c:/tmp/ms_tmp/tile.jpg", "wb"); */
                /* And save the image  -- could also use gdImageJpeg */
        /* gdImageJpeg(tile, out, 95); */
    }


    if (tile)
    {
        data = gd2bitmap(tile, &size, &bytesPerColor);
        dbldata = bitmap2dbl(data,&size,&bytesPerColor);
        oShape = bitmap2shape(dbldata, size, tile->sx, tile->sy, 
                              SWFFILL_SOLID, image);/* SWFFILL_TILED_BITMAP); */
        StoreShape(oShape, image);
    }
    else
    {
        if (MS_VALID_COLOR(sFc) || MS_VALID_COLOR(sOc))
        {
            if (nLayerIndex < 0 ||  nShapeIndex < 0)
            {
                oShape = DrawShapeFilledPolygon(p, psFillColor, psOutlineColor,
                                                width);
                StoreShape(oShape, image);
                SWFMovie_add(GetCurrentMovie(map, image), oShape);
            }
            else
            {
                oButton = DrawButtonFilledPolygon(p, psFillColor, psOutlineColor,
                                                  &sColorHighlightObj, nLayerIndex, 
                                                  nShapeIndex, width);
                StoreButton(oButton, image);
                oDisplay = _msSWFAddButtonWithId(image,
                                                 GetCurrentMovie(map, image),
                                                 oButton, 
                                                 nLayerIndex, nShapeIndex);
            }
        }
    }
    
}



/************************************************************************/
/*                             int draw_textSWF                         */
/*                                                                      */
/*      Renders a text using FDB fonts. FDB fonts are specific to       */
/*      ming library. Returns 0 on success and -1 on error.             */
/************************************************************************/
int draw_textSWF(imageObj *image, pointObj labelPnt, char *string, 
                labelObj *label, fontSetObj *fontset, double scalefactor)
{
    int         x, y;
    char        *font=NULL;
    /* int         nTmp = 0; */

    colorObj    sColor, sShadowColor;
    mapObj     *map = NULL;
    SWFText     oText = NULL, oShadowText=NULL;
    double         size = 0;
    int         drawShadow=0;
    char szPath[MS_MAXPATHLEN];
    SWFDisplayItem oShadowDisplay;
    SWFDisplayItem oDisplay; 
    int shadowsizex, shadowsizey;
  
/* -------------------------------------------------------------------- */
/*      if not SWF, return.                                             */
/* -------------------------------------------------------------------- */
    if (image == NULL || !MS_DRIVER_SWF(image->format))
        return 0;

/* -------------------------------------------------------------------- */
/*      validate arguments.                                             */
/* -------------------------------------------------------------------- */
    if(!string || strlen(string) == 0 || !label || !fontset)
        return(0); /* not an error, just don't want to do anything */

    if(strlen(string) == 0)
        return(0); /* not an error, just don't want to do anything */

    x = MS_NINT(labelPnt.x);
    y = MS_NINT(labelPnt.y);


    
/* char *error=NULL, *font=NULL; */
/* int bbox[8]; */
/* double angle_radians = MS_DEG_TO_RAD*label->angle; */

    size = label->size*scalefactor;
    shadowsizex = MS_NINT(label->shadowsizex*image->resolutionfactor);
    shadowsizey = MS_NINT(label->shadowsizey*image->resolutionfactor);

    if(!fontset) 
    {
        msSetError(MS_TTFERR, "No fontset defined.", "draw_textSWF()");
        return(-1);
    }

    if(!label->font) 
    {
        msSetError(MS_TTFERR, "No font defined.", "draw_textSWF()");
        return(-1);
    }

    font = msLookupHashTable(&(fontset->fonts), label->font);

    if(!font) 
    {
        msSetError(MS_TTFERR, "Requested font (%s) not found.", "draw_textSWF()",
                   label->font);
        return(-1);
    }

/* -------------------------------------------------------------------- */
/*      extract the colors.                                             */
/* -------------------------------------------------------------------- */
    map = ((SWFObj *)image->img.swf)->map;
    sColor.red = 0;
    sColor.green = 0;
    sColor.blue = 0;

    if (MS_VALID_COLOR(label->color))
    {
        sColor.red = label->color.red;
        sColor.green = label->color.green;
        sColor.blue = label->color.blue;
    }  
    else if (MS_VALID_COLOR(label->outlinecolor))
    {
        sColor.red = label->outlinecolor.red;
        sColor.green = label->outlinecolor.green;
        sColor.blue = label->outlinecolor.blue;
    }
    else
    {
        msSetError(MS_TTFERR, "Invalid color", "draw_textSWF()");
	return(-1);
    }
    if (MS_VALID_COLOR(label->shadowcolor))
    {
        sShadowColor.red = label->shadowcolor.red;
        sShadowColor.green = label->shadowcolor.green;
        sShadowColor.blue = label->shadowcolor.blue;
        drawShadow = 1; 
    }
    

/* ==================================================================== */
/*      Create a text object at "0,0".. this sets it's rotation         */
/*      axis.... We will then move the text SWFDisplayItem_moveTo       */
/*      and rotate it.                                                  */
/* ==================================================================== */
    /* first draw the shadow with a x+1,y+1 position or using the shadowsize if defined*/
    if (drawShadow) 
    {
        oShadowText = DrawText(string, 0, 0, msBuildPath(szPath, fontset->filename, font), size, &sShadowColor, image);
        if (oShadowText)
        {
        	StoreText(oShadowText, image);
            /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */
            oShadowDisplay = SWFMovie_add(GetCurrentMovie(map, image), oShadowText);
            if (shadowsizex > 0 && shadowsizey > 0)
            	SWFDisplayItem_moveTo(oShadowDisplay, (float)x + shadowsizex, 
                                    (float)y + shadowsizey);
            else
            	SWFDisplayItem_moveTo(oShadowDisplay, (float)x + 1, (float)y + 1);

            SWFDisplayItem_rotate(oShadowDisplay, (float)label->angle);
        }
    } 
    /* oText = DrawText(string, x, y, msBuildPath(szPath, fontset->filename, font), size, &sColor); */
    oText = DrawText(string, 0, 0, msBuildPath(szPath, fontset->filename, font), size, &sColor, image);
    if (oText)
    {
    	StoreText(oText, image);
        /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */
        oDisplay = SWFMovie_add(GetCurrentMovie(map, image), oText);
        SWFDisplayItem_moveTo(oDisplay, (float)x, (float)y);
        SWFDisplayItem_rotate(oDisplay, (float)label->angle);
    }
    
    return 0;
}


/************************************************************************/
/*                          int msGetLabelSizeSWF                       */
/*                                                                      */
/*      Iinialize the rect object with the size of the text. Return     */
/*      0 on sucess or -1.                                              */
/************************************************************************/
int msGetLabelSizeSWF(char *string, labelObj *label, rectObj *rect, 
                      fontSetObj *fontset, double scalefactor)
{
    SWFText     oText = NULL;
    SWFFont     oFont = NULL;
    char        *font;
    double      dfWidth = 0.0;
    FILE        *f = NULL;

    char szPath[MS_MAXPATHLEN];

    if (!string || strlen(string) == 0 || !label || !rect ||
        !fontset)
        return -1;

    font = msLookupHashTable(&(fontset->fonts), label->font);

    if(!font) 
    {
        if(label->font) 
            msSetError(MS_TTFERR, "Requested font (%s) not found.", 
                       "msGetLabelSizeSWF()", label->font);
        else
            msSetError(MS_TTFERR, "Requested font (NULL) not found.", 
                       "msGetLabelSizeSWF()" );
        return(-1);
    }
    
    f = fopen(msBuildPath(szPath, fontset->filename, font), "rb");
    if (f)
    {
        oFont  = loadSWFFontFromFile(f);
        fclose(f);
        if (oFont)
        {   
        	oText = newSWFText();
            SWFText_setFont(oText, oFont);
            /* SWFText_addString(oText, string, NULL); */
            dfWidth = 0.0;
            dfWidth = (double)SWFText_getStringWidth(oText, (unsigned char *)string);
        
            if (dfWidth <=0)
              return -1;

            destroySWFText(oText);
            destroySWFFont(oFont);
        }
    }

    rect->minx = 0;
    rect->miny = 0;
    rect->maxx = dfWidth;
    rect->maxy = label->size*scalefactor;
    
    return(0);
}


/************************************************************************/
/*                            msSWFGetMovieIndex                        */
/*                                                                      */
/*      Utility function to look in an array of int for a specific      */
/*      value. Returns the indice of the array on succsess or -1.       */
/************************************************************************/
static int msSWFGetMovieIndex(int *panIndex, int nSize, int nIndex)
{
    int i = 0;
    int nIndice = -1;

    if (panIndex && nSize > 0)
    {
        for (i=0; i<nSize; i++)
        {
            if (panIndex[i] == nIndex)
            {
                nIndice = i;
                break;
            }
        }
    }

    return nIndice;
}

/************************************************************************/
/*                         int msDrawLabelCacheSWF                      */
/*                                                                      */
/*       Draws labels saved in the cache. Each label is drawn to        */
/*      the approporiate movie pointer.                                 */
/************************************************************************/
/* ==================================================================== */
/*      This could easily be integrated with a more generic             */
/*      msDrawLabelCache.                                               */
/*       The only functions that need to be wrapped here are :          */
/*                                                                      */
/*        -  billboard                                                  */
/*        -  draw_text                                                  */
/*        - labelInImage                                                */
/* ==================================================================== */
int msDrawLabelCacheSWF(imageObj *image, mapObj *map)
{
    pointObj p;
    int i, j, l, priority;

    rectObj r;
  
    labelCacheMemberObj *cachePtr=NULL;
    layerObj *layerPtr=NULL;
    labelObj *labelPtr=NULL;

    int draw_marker;
    int marker_width, marker_height;
    int marker_offset_x, marker_offset_y, label_offset_x, label_offset_y;
    rectObj marker_rect;
    int label_mindistance=-1,
        label_buffer=0, map_edge_buffer=0;    

    int         nCurrentMovie = 0;
    int         bLayerOpen = 0;
    imageObj    *imagetmp = NULL;
    /* int         nTmp = -1; */
    SWFShape    oShape;

/* -------------------------------------------------------------------- */
/*      test for driver.                                                */
/* -------------------------------------------------------------------- */
    if (!image || !MS_DRIVER_SWF(image->format))
        return -1;

    marker_rect.minx = marker_rect.miny = marker_rect.maxx = marker_rect.maxy = 0;      
#ifdef USE_POINT_Z_M
    p.x = p.y = p.z = 0;
#else
    p.x = p.y = 0;
#endif
/* -------------------------------------------------------------------- */
/*      if the output is single (SWF file based on one raster that      */
/*      will contain all  the rendering), draw the lables using the     */
/*      GD driver.                                                      */
/* -------------------------------------------------------------------- */
    if(strcasecmp(msGetOutputFormatOption(image->format,
                                          "OUTPUT_MOVIE",""), 
                  "SINGLE") == 0)
    {
        int orig_renderer;
        imagetmp = (imageObj *)((SWFObj *)image->img.swf)->imagetmp;
        msImageInitGD( imagetmp, &map->imagecolor);
        orig_renderer=image->format->renderer;
        image->format->renderer = MS_RENDER_WITH_GD;
        msDrawLabelCache(imagetmp, map);
        image->format->renderer = orig_renderer;
        oShape = gdImage2Shape(imagetmp->img.gd, image);
        StoreShape(oShape, image);
        /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */
        SWFMovie_add(GetCurrentMovie(map, image), oShape);
        return 0;
    }
/* -------------------------------------------------------------------- */
/*      keep the current layer index.                                   */
/* -------------------------------------------------------------------- */
    nCurrentMovie = ((SWFObj *)image->img.swf)->nCurrentMovie;

    for(priority=MS_MAX_LABEL_PRIORITY-1; priority>=0; priority--) {
      labelCacheSlotObj *cacheslot;
      cacheslot = &(map->labelcache.slots[priority]);

      for(l=cacheslot->numlabels-1; l>=0; l--) {

        cachePtr = &(cacheslot->labels[l]); /* point to right spot in cache */

        layerPtr = (GET_LAYER(map, cachePtr->layerindex)); /* set a couple of other pointers, avoids nasty references */

/* ==================================================================== */
/*      set the current layer so the label will be drawn in the         */
/*      using the correct SWF handle.                                   */
/* ==================================================================== */
        

        ((SWFObj *)image->img.swf)->nCurrentMovie = 
          msSWFGetMovieIndex(((SWFObj *)image->img.swf)->panLayerIndex, 
                             ((SWFObj *)image->img.swf)->nLayerMovies,
                             cachePtr->layerindex);

        if (((SWFObj *)image->img.swf)->nCurrentMovie < 0)
          continue;
        
        /* msImageStartLayerSWF(map, layerPtr, image); */
        ((SWFObj *)image->img.swf)->nCurrentLayerIdx = cachePtr->layerindex;
/* ==================================================================== */
/*      at this point the layer (at the shape level is closed). So      */
/*      we will open it if necessary.                                   */
/* ==================================================================== */
        bLayerOpen = 0;
        if (msLookupHashTable(&(layerPtr->metadata), "SWFDUMPATTRIBUTES") &&
            layerPtr->numitems <= 0)
        {
            msLayerOpen(layerPtr);
            msLayerWhichItems(layerPtr, MS_FALSE, msLookupHashTable(&(layerPtr->metadata), "SWFDUMPATTRIBUTES"));
            bLayerOpen = 1;
        }
        
        
        /* ((SWFObj *)image->img.swf)->nCurrentShapeIdx = cachePtr->shapeidx; */
        msDrawStartShapeUsingIdxSWF(map, layerPtr, image,  cachePtr);
           
        
        /* classPtr = &(cachePtr->class); */
        labelPtr = &(cachePtr->label);

        if(!cachePtr->text)
            continue; /* not an error, just don't want to do anything */

        if(strlen(cachePtr->text) == 0)
            continue; /* not an error, just don't want to do anything */

        if(msGetLabelSizeSWF(cachePtr->text, labelPtr, &r, 
                             &(map->fontset), layerPtr->scalefactor) == -1)
            return(-1);

        label_offset_x = labelPtr->offsetx*layerPtr->scalefactor;
        label_offset_y = labelPtr->offsety*layerPtr->scalefactor;
        label_buffer = MS_NINT(labelPtr->buffer*image->resolutionfactor);
        label_mindistance = MS_NINT(labelPtr->mindistance*image->resolutionfactor);

        if(labelPtr->autominfeaturesize && (cachePtr->featuresize != -1) && ((r.maxx-r.minx) > cachePtr->featuresize))
            continue; /* label too large relative to the feature */

        draw_marker = marker_offset_x = marker_offset_y = 0; /* assume no marker */
        if((layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) || layerPtr->type == MS_LAYER_POINT) { /* there *is* a marker */

	    /* TO DO: at the moment only checks the bottom style, perhaps should check all of them */
            if(msGetMarkerSize(&map->symbolset, &(cachePtr->styles[0]), &marker_width, &marker_height, layerPtr->scalefactor) != MS_SUCCESS)
	      return(-1);
            
            marker_width = (int)(marker_width);
            marker_height = (int)(marker_height);

            marker_offset_x = MS_NINT(marker_width/2.0);
            marker_offset_y = MS_NINT(marker_height/2.0);      

            marker_rect.minx = MS_NINT(cachePtr->point.x - .5 * marker_width);
            marker_rect.miny = MS_NINT(cachePtr->point.y - .5 * marker_height);
            marker_rect.maxx = marker_rect.minx + (marker_width-1);
            marker_rect.maxy = marker_rect.miny + (marker_height-1);

            for(i=0; i<cachePtr->numstyles; i++)
              cachePtr->styles[i].size = (int)(cachePtr->styles[i].size * layerPtr->scalefactor);

            /* if(layerPtr->type == MS_LAYER_ANNOTATION) draw_marker = 1; */ /* actually draw the marker */
        }

        if(labelPtr->position == MS_AUTO) {

            if(layerPtr->type == MS_LAYER_LINE) {
                int position = MS_UC;

                for(j=0; j<2; j++) { /* Two angles or two positions, depending on angle. Steep angles will use the angle approach, otherwise we'll rotate between UC and LC. */

                    msFreeShape(cachePtr->poly);
                    cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

                    p = get_metrics(&(cachePtr->point), position, r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, label_buffer, cachePtr->poly);

                    if(draw_marker)
                        msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon */

                    /* Compare against image bounds, rendered labels and markers (sets cachePtr->status) */
                    msTestLabelCacheCollisions(&(map->labelcache), labelPtr, 
                                               map->width, map->height, 
                                               label_buffer, cachePtr, priority, l, label_mindistance, (r.maxx-r.minx));

                    if(cachePtr->status) /* found a suitable place for this label */
                        break;

                } /* next angle */

            } else {
                for(j=0; j<=7; j++) { /* loop through the outer label positions */

                    msFreeShape(cachePtr->poly);
                    cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

                    p = get_metrics(&(cachePtr->point), j, r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, label_buffer, cachePtr->poly);

                    if(draw_marker)
                        msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon */

                    /* Compare against image bounds, rendered labels and markers (sets cachePtr->status) */
                    msTestLabelCacheCollisions(&(map->labelcache), labelPtr, 
                                               map->width, map->height, 
                                               label_buffer, cachePtr, priority, l, label_mindistance, (r.maxx-r.minx));

                    if(cachePtr->status) /* found a suitable place for this label */
                        break;
                } /* next position */
            }

            if(labelPtr->force) cachePtr->status = MS_TRUE; /* draw in spite of collisions based on last position, need a *best* position */

        } else {

            cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

            if(labelPtr->position == MS_CC) /* don't need the marker_offset */
                p = get_metrics(&(cachePtr->point), labelPtr->position, r, label_offset_x, label_offset_y, labelPtr->angle, label_buffer, cachePtr->poly);
            else
                p = get_metrics(&(cachePtr->point), labelPtr->position, r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, label_buffer, cachePtr->poly);

            if(draw_marker)
                msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon, part of overlap tests */

            if(!labelPtr->force) { /* no need to check anything else */

              /* Compare against image bounds, rendered labels and markers (sets cachePtr->status) */
                msTestLabelCacheCollisions(&(map->labelcache), labelPtr, 
                                           map->width, map->height, 
                                           label_buffer, cachePtr, priority, l, label_mindistance, (r.maxx-r.minx));
            }
        } /* end position if-then-else */

        /* msImagePolyline(img, cachePtr->poly, 1); */

        if(!cachePtr->status)
            continue; /* next label */

        if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0){ /* need to draw a marker */
            for(i=0; i<cachePtr->numstyles; i++)
              msDrawMarkerSymbolSWF(&map->symbolset, image, &(cachePtr->point), 
                                    &(cachePtr->styles[i]), layerPtr->scalefactor);
        }

        /* TODO */
        /* if(labelPtr->backgroundcolor >= 0) */
        /* billboard(img, cachePtr->poly, labelPtr); */

        draw_textSWF(image, p, cachePtr->text, labelPtr, &(map->fontset), layerPtr->scalefactor); /* actually draw the label */

        if (bLayerOpen)
          msLayerClose(layerPtr);
        
      } /* next in cache */
    } /* next priority */

    
    ((SWFObj *)image->img.swf)->nCurrentMovie = nCurrentMovie;

    return(0);
}



/************************************************************************/
/*                              msDrawLabelSWF                          */
/*                                                                      */
/*      Draw a label.                                                   */
/************************************************************************/
int msDrawLabelSWF(imageObj *image, pointObj labelPnt, char *string, 
                   labelObj *label, fontSetObj *fontset, double scalefactor)
{
    pointObj p;
    rectObj r;
    int label_offset_x, label_offset_y;

    if (!image || !MS_DRIVER_SWF(image->format))
        return 0;

    if(!string)
        return(0); /* not an error, just don't want to do anything */

    if(strlen(string) == 0)
        return(0); /* not an error, just don't want to do anything */
    
    msGetLabelSizeSWF(string, label, &r, fontset, scalefactor);
    label_offset_x = label->offsetx*scalefactor;
    label_offset_y = label->offsety*scalefactor;

    p = get_metrics(&labelPnt, label->position, r, label_offset_x, 
                    label_offset_y, label->angle, 0, NULL);
    /* labelPnt.x += label->offsetx; */
    /* labelPnt.y += label->offsety; */
    return draw_textSWF(image, p, string, label, fontset, scalefactor);

}

/************************************************************************/
/*                          int msDrawWMSLayerSWF                       */
/*                                                                      */
/*      Use low level gd functions to draw a wms layer in a gd          */
/*      images and then save it in the movie,                           */
/************************************************************************/
int msDrawWMSLayerSWF(int nLayerId, httpRequestObj *pasReqInfo, 
                      int numRequests, mapObj *map, layerObj *layer, imageObj *image)
{
    imageObj            *image_tmp = NULL;
    SWFShape            oShape;
    int                 iReq = -1;
    const char         *driver = "GD/GIF";
    int         bFreeImage = 0;

#ifdef USE_GD_GIF
    driver = "GD/GIF";
#else  

#ifdef USE_GD_PNG
     driver = "GD/PNG";
#else

#ifdef USE_GD_JPEG
     driver = "GD/JPEG";
#else

#ifdef USE_GD_WBMP
     driver = "GD/WBMP";
#endif 

#endif
#endif
#endif


    if (!image || !MS_DRIVER_SWF(image->format) || image->width <= 0 ||
        image->height <= 0)
      return -1;

/* ==================================================================== */
/*      WMS requests are done simultaniously for all WMS layer.         */
/* ==================================================================== */
    for(iReq=0; iReq<numRequests; iReq++)
    {
        if (pasReqInfo[iReq].nLayerId == nLayerId)
          break;
    }
    if (iReq == numRequests)
      return MS_SUCCESS;     
/* -------------------------------------------------------------------- */
/*      create a temprary GD image and render in it.                    */
/* -------------------------------------------------------------------- */
    if (strcasecmp(msGetOutputFormatOption(image->format,
                                           "OUTPUT_MOVIE",""), 
                      "MULTIPLE") == 0)
    {
        image_tmp = msImageCreateGD(map->width, map->height,  
                                    msCreateDefaultOutputFormat(map, driver),
                                    map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution);
        bFreeImage = 1;
    }
    else
      image_tmp = (imageObj *)((SWFObj *)image->img.swf)->imagetmp;


    
    /* gdImageColorAllocate(image_tmp->img.gd,  */
    /* map->imagecolor.red, map->imagecolor.green,  */
    /* map->imagecolor.blue); */

    
    msImageInitGD( image_tmp, &map->imagecolor );

    if (msDrawWMSLayerLow(nLayerId, pasReqInfo, numRequests, map, layer, 
                          image_tmp) != -1)
    {
        oShape = gdImage2Shape(image_tmp->img.gd, image);
        /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */
        StoreShape(oShape, image);
        SWFMovie_add(GetCurrentMovie(map, image), oShape);
        
        if (bFreeImage)
          msFreeImage( image_tmp );
    }

    return MS_SUCCESS;

}


/************************************************************************/
/*                         int msDrawRasterLayerSWF                     */
/*                                                                      */
/*      Renders a raster layer in a temporary GD image and adds it      */
/*      in the current movie.                                           */
/************************************************************************/
int msDrawRasterLayerSWF(mapObj *map, layerObj *layer, imageObj *image)
{
    /* int         nTmp = 0; */
    SWFShape    oShape;
    outputFormatObj *format = NULL;
    imageObj    *image_tmp = NULL;
    int         bFreeImage = 0;

    if (!image || !MS_DRIVER_SWF(image->format) || image->width <= 0 ||
        image->height <= 0)
        return -1;
    
/* -------------------------------------------------------------------- */
/*      create a temprary GD image and render in it.                    */
/* -------------------------------------------------------------------- */
    format = msCreateDefaultOutputFormat( NULL, "GD/PC256" );
    if( format == NULL )
        return -1;

       if (strcasecmp(msGetOutputFormatOption(image->format,
                                              "OUTPUT_MOVIE",""), 
                      "MULTIPLE") == 0)
       {
           image_tmp = msImageCreate( image->width, image->height, format, 
                                      NULL, NULL, map );
           bFreeImage = 1;
       }
       else
         image_tmp = (imageObj *)((SWFObj *)image->img.swf)->imagetmp;

    if( image_tmp == NULL )
      return -1;

    if (msDrawRasterLayerLow(map, layer, image_tmp) != -1)
    {
        oShape = gdImage2Shape(image_tmp->img.gd, image);
        /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */
        StoreShape(oShape, image);
        SWFMovie_add(GetCurrentMovie(map, image), oShape);
        if (bFreeImage)
          msFreeImage( image_tmp );
    }

    return 0;
}

static int SWFIsInArray(int *panArray, int nSize, int nValue)
{
    int i = 0;
    if (panArray && nSize > 0)
    {
        for (i=0; i<nSize; i++)
        {
            if (panArray[i] == nValue)
              return MS_TRUE;
            if (panArray[i] > nValue)
              return MS_FALSE;
        }
    }

    return MS_FALSE;
}
    
/************************************************************************/
/*                            int msSaveImageSWF                        */
/*                                                                      */
/*      Save to SWF files.                                              */
/************************************************************************/
int msSaveImageSWF(imageObj *image, char *filename)
{
    int         i=0, j=0, k=0, nLayers=0, iMovie= 0;
    char        szBase[100];
    char        szExt[5];
    char        szTmp[20];
    int         nLength;
    int         iPointPos;
    int         iSlashPos;
    char        szAction[200];
    SWFAction   oAction;
    mapObj      *map = NULL;
    char        *pszRelativeName = NULL;
    int         iSaveResult; 
    int         bFileIsTemporary = MS_FALSE;
    const char *pszExtension = NULL;
    
    FILE        *fp; 
    unsigned char block[4000];
    int         bytes_read;
	
    int         bLoadAutomatically = MS_TRUE;
	
    /*do not load all the movie for the layers automatically  if flag is set to off
      Bug 1696*/
    if (strcasecmp(msGetOutputFormatOption(image->format,
                                           "LOAD_AUTOMATICALLY",""), "OFF") == 0)
      bLoadAutomatically = MS_FALSE;

    if (image && MS_DRIVER_SWF(image->format))/* && filename) */
    {
         map = ((SWFObj *)image->img.swf)->map;

/* -------------------------------------------------------------------- */
/*      We will need to write the output to a temporary file.           */
/* -------------------------------------------------------------------- */
         if( filename == NULL )
         {
             pszExtension = image->format->extension;
             if( pszExtension == NULL )
               pszExtension = "swf";

             if( map && map->web.imagepath && map->web.imageurl)
               filename = msTmpFile(map->mappath,map->web.imagepath,pszExtension);

             if (!filename)
               return(MS_FAILURE); 
            
             bFileIsTemporary = MS_TRUE;
         }

         if (!filename)
           return(MS_FAILURE); 
/* ==================================================================== */
/*      if the output is single movie, save the main movie and exit.    */
/* ==================================================================== */
        if (strcasecmp(msGetOutputFormatOption(image->format,
                                               "OUTPUT_MOVIE",""), 
                       "MULTIPLE") != 0)
        {

#ifdef MING_VERSION_03
            iSaveResult = SWFMovie_save(((SWFObj *)image->img.swf)->sMainMovie, filename, -1);
#else
            iSaveResult = SWFMovie_save(((SWFObj *)image->img.swf)->sMainMovie, filename);
#endif

/* -------------------------------------------------------------------- */
/*      Is this supposed to be a temporary file?  If so, stream to      */
/*      stdout and delete the file.                                     */
/* -------------------------------------------------------------------- */
            if( bFileIsTemporary )
            {
                if( msIO_needBinaryStdout() == MS_FAILURE )
                  return MS_FAILURE;

               fp = fopen( filename, "rb" );
               if( fp == NULL )
               {
                   msSetError( MS_MISCERR, 
                               "Failed to open %s for streaming to stdout.",
                               "msSaveImageSWF()", filename );
                   return MS_FAILURE;
               }

               while( (bytes_read = fread(block, 1, sizeof(block), fp)) > 0 )
                 msIO_fwrite( block, 1, bytes_read, stdout );

               fclose( fp );

               unlink( filename );
               free( filename );
        /*
                char *pszURL = (char *)malloc(sizeof(char)*(strlen(map->web.imageurl)+
                                                            strlen(filename)+
                                                            strlen(pszExtension)+2));

                sprintf(pszURL, "%s%s.%s", map->web.imageurl, msGetBasename(filename), 
                        pszExtension);
              
                msIO_printf("<OBJECT classid=\"clsid:D27CDB6E-AE6D-11cf-96B8-444553540000\" codebase=\"http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=5,0,0,0\" WIDTH=%d HEIGHT=%d> <PARAM NAME=movie VALUE=\"%s\"><PARAM NAME=quality VALUE=medium><EMBED src=\"%s\" quality=medium  WIDTH=%d HEIGHT=%d TYPE=\"application/x-shockwave-flash\" PLUGINSPAGE=\"http://www.macromedia.com/shockwave/download/index.cgi?P1_Prod_Version=ShockwaveFlash\"></EMBED></OBJECT>", map->width, map->height, pszURL, pszURL,  map->width, map->height);
              
                // unlink( filename ); 
                free( filename );
        */      
            }
 
            
            if (iSaveResult > 0)
              return(MS_SUCCESS);
            else
              return(MS_FAILURE); 
        }

/* -------------------------------------------------------------------- */
/*      If the output is MULTIPLE save individual movies as well as     */
/*      the main movie.                                                 */
/* -------------------------------------------------------------------- */
        
       
/* -------------------------------------------------------------------- */
/*      write some AS related to the map file.                          */
/* -------------------------------------------------------------------- */
        sprintf(szAction, "%s", "mapObj=new Object();");
        oAction = compileSWFActionCode(szAction);
        SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);

        sprintf(szAction, "mapObj.name=\"%s\";", ((SWFObj *)image->img.swf)->map->name);
        oAction = compileSWFActionCode(szAction);
        SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);
        
        sprintf(szAction, "mapObj.width=%d;", ((SWFObj *)image->img.swf)->map->width);
        oAction = compileSWFActionCode(szAction);
        SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);
        
        sprintf(szAction, "mapObj.height=%d;", 
                ((SWFObj *)image->img.swf)->map->height);
        oAction = compileSWFActionCode(szAction);
        SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);
        
        sprintf(szAction, "mapObj.extent=\"%f,%f,%f,%f\";", 
                ((SWFObj *)image->img.swf)->map->extent.minx,
                ((SWFObj *)image->img.swf)->map->extent.miny,
                ((SWFObj *)image->img.swf)->map->extent.maxx,
                ((SWFObj *)image->img.swf)->map->extent.maxy);
        oAction = compileSWFActionCode(szAction);
        SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);

        sprintf(szAction, "mapObj.numlayers=%d;", 
                ((SWFObj *)image->img.swf)->map->numlayers);
        oAction = compileSWFActionCode(szAction);
        SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);
        
        
        sprintf(szAction, "%s", "mapObj.layers=new Array();"); 
        oAction = compileSWFActionCode(szAction);
        SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);

/* -------------------------------------------------------------------- */
/*      Write class for layer object.                                   */
/* -------------------------------------------------------------------- */
        sprintf(szAction, "%s", "function LayerObj(name, type, fullname, relativename){ this.name=name; this.type=type; this.fullname=fullname; this.relativename=relativename;}"); 
        oAction = compileSWFActionCode(szAction);
        SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);

        
        
/* -------------------------------------------------------------------- */
/*      extract the name of the file to save and use it to save the     */
/*      layer files. For example for file name /tmp/ms_tmp/test.swf,    */
/*      the first layer name will be /tmp/ms_tmp/test_layer0.swf.       */
/* -------------------------------------------------------------------- */
        nLength = strlen(filename);
        iPointPos = -1;
        for (i=nLength-1; i>=0; i--)
        {
            if (filename[i] == '.')
            {
                iPointPos = i;
                break;
            }
        }
        szBase[0] = '\0';
        szExt[0] = '\0';
        if (iPointPos >= 0)
        {
            strncpy(szBase, filename, iPointPos);
            szBase[iPointPos] = '\0';
            strcpy(szExt, filename+iPointPos+1);
        } 
        

/* ==================================================================== */
/*      For layers that have not be drawn for some reason (status       */
/*      off, out of scale ...), It will output AS layer objects         */
/*      where the full name and relative name are set to "undefined"    */
/*      (See Bug 804 for more details).                                 */
/* ==================================================================== */
        nLayers = map->numlayers; /*((SWFObj *)image->img.swf)->nLayerMovies;*/

/* -------------------------------------------------------------------- */
/*      save layers.                                                    */
/* -------------------------------------------------------------------- */
        iMovie = -1;
        for (i=0; i<nLayers; i++)
        {
            if (SWFIsInArray(((SWFObj *)image->img.swf)->panLayerIndex,
                             ((SWFObj *)image->img.swf)->nLayerMovies, i))
            {
                iMovie++;
                /* -------------------------------------------------------------------- */
                /*      build full filename.                                            */
                /* -------------------------------------------------------------------- */
                sprintf(szTmp, "%s%d", "_layer_", i);
                gszFilename[0] = '\0';
                sprintf(gszFilename, szBase);
                strcat(gszFilename, szTmp);
                strcat(gszFilename, ".");
                strcat(gszFilename, szExt);
                /* -------------------------------------------------------------------- */
                /*      build relative name.                                            */
                /* -------------------------------------------------------------------- */
                nLength = strlen(gszFilename);
                iSlashPos = -1;
                for (j=nLength-1; j>=0; j--)
                {
                    if (gszFilename[j] == '/' || gszFilename[j] == '\\')
                    {
                        iSlashPos = j;
                        break;
                    }
                }
                if (iSlashPos >=0)
                {
                    gszTmp[0]='\0';
                    k = 0;
                    for (j=iSlashPos+1; j<nLength; j++)
                    {
                        gszTmp[k] = gszFilename[j];
                        k++;
                    }
                    gszTmp[k] = '\0';
                    pszRelativeName = gszTmp;
                }
                else
                  pszRelativeName = gszFilename;

                /* test */
                /* sprintf(gszFilename, "%s%d.swf", "c:/tmp/ms_tmp/layer_", i); */

                SWFMovie_setBackground(((SWFObj *)image->img.swf)->pasMovies[iMovie], 
                                       0xff, 0xff, 0xff);
            
#ifdef MING_VERSION_03
                SWFMovie_save(((SWFObj *)image->img.swf)->pasMovies[iMovie], gszFilename, -1);  
#else
                SWFMovie_save(((SWFObj *)image->img.swf)->pasMovies[iMovie], gszFilename);
#endif

                /* test */
                /* sprintf(gszFilename, "%s%d.swf", "layer_", i); */
            
                /* sprintf(szAction, "mapObj.layers[%d]=\"%s\";", i, gszFilename); */
                sprintf(szAction, "mapObj.layers[%d]= new LayerObj(\"%s\",\"%d\",\"%s\",\"%s\");", i, 
                        GET_LAYER(map, i)->name, GET_LAYER(map, i)->type, gszFilename,
                        pszRelativeName);
            
                oAction = compileSWFActionCode(szAction);
                SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);

                /* ==================================================================== */
                /*      when we have a temporary file, we are assuming that the         */
                /*      intention is to send the swf to the browser. So we use          */
                /*      output the URL for the loadMovieNum. Else we output only the    */
                /*      name of the file.                                               */
                /* ==================================================================== */
                /* sprintf(szAction, "loadMovie(\"%s\",%d);", gszFilename, i+1); */
				
                if ( bLoadAutomatically )
                {
                    if( bFileIsTemporary )
                      sprintf(szAction, "loadMovieNum(\"%s%s\",%d);", map->web.imageurl, 
	                          pszRelativeName, i+1);
                    else
                      sprintf(szAction, "loadMovieNum(\"%s\",%d);", pszRelativeName, i+1);

                    oAction = compileSWFActionCode(szAction);
                    SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);
                }
            } 
            else /*layer which was not drawn */
            {
                sprintf(szAction, 
                        "mapObj.layers[%d]= new LayerObj(\"%s\",\"%d\",\"undefined\",\"undefined\");", i, 
                        GET_LAYER(map, i)->name, GET_LAYER(map, i)->type);
            
                oAction = compileSWFActionCode(szAction);
                SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);
            }

        }
        sprintf(szAction, "stop();");
        oAction = compileSWFActionCode(szAction);
        SWFMovie_add(((SWFObj *)image->img.swf)->sMainMovie, oAction);

#ifdef MING_VERSION_03
        SWFMovie_save(((SWFObj *)image->img.swf)->sMainMovie, filename, -1); 
#else
        SWFMovie_save(((SWFObj *)image->img.swf)->sMainMovie, filename); 
#endif
        /* test */
        /* SWFMovie_save(((SWFObj *)image->img.swf)->sMainMovie, "c:/tmp/ms_tmp/main.swf");   */

        if( bFileIsTemporary )
        {
          fp = fopen( filename, "rb" );
               if( fp == NULL )
                 {
                   msSetError( MS_MISCERR, 
                               "Failed to open %s for streaming to stdout.",
                               "msSaveImageGDAL()", filename );
                   return MS_FAILURE;
                 }

               while( (bytes_read = fread(block, 1, sizeof(block), fp)) > 0 )
                 msIO_fwrite( block, 1, bytes_read, stdout );

               fclose( fp );

               /* unlink( filename ); */
               free( filename );
               /*
            char *pszURL = (char *)malloc(sizeof(char)*(strlen(map->web.imageurl)+
                                                        strlen(filename)+
                                                        strlen(pszExtension)+2));

            sprintf(pszURL, "%s%s.%s", map->web.imageurl, msGetBasename(filename), 
                    pszExtension);
            
            msIO_printf("<OBJECT classid=\"clsid:D27CDB6E-AE6D-11cf-96B8-444553540000\" codebase=\"http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=5,0,0,0\" WIDTH=%d HEIGHT=%d> <PARAM NAME=movie VALUE=\"%s\"><PARAM NAME=quality VALUE=medium> <EMBED src=\"%s\" quality=medium WIDTH=%d HEIGHT=%d TYPE=\"application/x-shockwave-flash\" PLUGINSPAGE=\"http://www.macromedia.com/shockwave/download/index.cgi?P1_Prod_Version=ShockwaveFlash\"></EMBED></OBJECT>", map->width, map->height, pszURL, pszURL,  map->width, map->height);
              
            // unlink( filename );
            free( filename );
               */
                
        }
        return(MS_SUCCESS);
    }

    return(MS_FAILURE);
}


/************************************************************************/
/*                           void msFreeImageSWF                        */
/*                                                                      */
/*      Free SWF Object structure                                       */
/************************************************************************/
void msFreeImageSWF(imageObj *image)
{
    int i = 0;
    if (image && MS_DRIVER_SWF(image->format) )
    {
    	/* fonts */
    	for (i=0; i<((SWFObj *)image->img.swf)->nFonts; i++)
    		destroySWFFont(((SWFObj *)image->img.swf)->Fonts[i]);
    	free(((SWFObj *)image->img.swf)->Fonts);
    	((SWFObj *)image->img.swf)->nFonts = 0;
    	/* texts */
    	for (i=0; i<((SWFObj *)image->img.swf)->nTexts; i++)
    		destroySWFText(((SWFObj *)image->img.swf)->Texts[i]);
    	free(((SWFObj *)image->img.swf)->Texts);
    	((SWFObj *)image->img.swf)->nFonts = 0;
    	/* bitmaps */
    	for (i=0; i<((SWFObj *)image->img.swf)->nBitmaps; i++)
    		destroySWFBitmap(((SWFObj *)image->img.swf)->Bitmaps[i]);
    	free(((SWFObj *)image->img.swf)->Bitmaps);
    	((SWFObj *)image->img.swf)->nBitmaps = 0;
    	/* inputs */
    	for (i=0; i<((SWFObj *)image->img.swf)->nInputs; i++)
    		destroySWFInput(((SWFObj *)image->img.swf)->Inputs[i]);
    	free(((SWFObj *)image->img.swf)->Inputs);
    	((SWFObj *)image->img.swf)->nInputs = 0;
    	/* shapes */
    	for (i=0; i<((SWFObj *)image->img.swf)->nShapes; i++)
    		destroySWFShape(((SWFObj *)image->img.swf)->Shapes[i]);
    	free(((SWFObj *)image->img.swf)->Shapes);
    	((SWFObj *)image->img.swf)->nShapes = 0;
    	/* buttons */
    	for (i=0; i<((SWFObj *)image->img.swf)->nButtons; i++)
    		/* need to check if some of the buttons disappeared or couldn't be created ! */
    		if (((SWFObj *)image->img.swf)->Buttons[i])
    			destroySWFButton(((SWFObj *)image->img.swf)->Buttons[i]);
    	free(((SWFObj *)image->img.swf)->Buttons);
    	((SWFObj *)image->img.swf)->nButtons = 0;
    	/* movies */
    	for (i=0; i<((SWFObj *)image->img.swf)->nLayerMovies; i++)
    		destroySWFMovie(((SWFObj *)image->img.swf)->pasMovies[i]);
    	free(((SWFObj *)image->img.swf)->pasMovies);
    	destroySWFMovie(((SWFObj *)image->img.swf)->sMainMovie);
        ((SWFObj *)image->img.swf)->nLayerMovies = 0;
        ((SWFObj *)image->img.swf)->nCurrentMovie = -1;

        /* DblData images */
        for (i=0; i<((SWFObj *)image->img.swf)->nDblDatas; i++)
        	free(((SWFObj *)image->img.swf)->DblDatas[i]);
        ((SWFObj *)image->img.swf)->nDblDatas = 0;
        free(((SWFObj *)image->img.swf)->DblDatas);
        
        ((SWFObj *)image->img.swf)->map = NULL;
        free(((SWFObj *)image->img.swf)->panLayerIndex);
        free(image->img.swf);
    }
}



/************************************************************************/
/*                           msTransformShapeSWF                        */
/*                                                                      */
/*      Transform geographic coordinated to output coordinates.         */
/************************************************************************/
void msTransformShapeSWF(shapeObj *shape, rectObj extent, double cellsize)
{
    int i,j; 

    if(shape->numlines == 0) return; 

    if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON) 
    { 
        for(i=0; i<shape->numlines; i++) 
        {
            for(j=0; j < shape->line[i].numpoints; j++ ) 
            {
                shape->line[i].point[j].x = 
                    (shape->line[i].point[j].x - extent.minx)/cellsize;
                shape->line[i].point[j].y = 
                    (extent.maxy - shape->line[i].point[j].y)/cellsize;
            }
        }      
        return;
    }
}

/************************************************************************/
/*                     int msDrawVectorLayerAsRasterSWF                 */
/*                                                                      */
/*      Draw a vector line as raster in a temporary GD image and        */
/*      save it.                                                        */
/************************************************************************/
int msDrawVectorLayerAsRasterSWF(mapObj *map, layerObj *layer, imageObj *image)
{
    imageObj    *imagetmp;
    /* int         nTmp = -1; */
    SWFShape    oShape;
    char        *driver = strdup("GD/GIF");
    int         bFreeImage = 0;

#ifdef USE_GD_GIF
    driver = strdup("GD/GIF");
#else  

#ifdef USE_GD_PNG
     driver = strdup("GD/PNG");
#else

#ifdef USE_GD_JPEG
     driver = strdup("GD/JPEG");
#else

#ifdef USE_GD_WBMP
     driver = strdup("GD/WBMP");
#endif 

#endif
#endif
#endif


    if (!image || !MS_DRIVER_SWF(image->format) )
      return MS_FAILURE;

   
    if (strcasecmp(msGetOutputFormatOption(image->format,
                                           "OUTPUT_MOVIE",""), 
                      "MULTIPLE") == 0)
    {
        imagetmp = msImageCreateGD(map->width, map->height,  
                                   msCreateDefaultOutputFormat(map, driver),
                                   map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution);
        bFreeImage = 1;
    }
    else
      imagetmp = (imageObj *)((SWFObj *)image->img.swf)->imagetmp;

    if (imagetmp)
    {
        msImageInitGD( imagetmp, &map->imagecolor );
        /* msLoadPalette(imagetmp->img.gd, &(map->palette), map->imagecolor); */
        msDrawVectorLayer(map, layer, imagetmp);
        
        oShape = gdImage2Shape(imagetmp->img.gd, image);
        /* nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie; */
        StoreShape(oShape, image);
        SWFMovie_add(GetCurrentMovie(map, image), oShape);
        
        if (bFreeImage)
          msFreeImage(imagetmp);

        return MS_SUCCESS;
    }

    return MS_FAILURE;
}



/************************************************************************/
/*          SWFMovie GetCurrentMovie(mapObj *map, imageObj *image)      */
/*                                                                      */
/*      Get the current movie : If the settings are 1movie per          */
/*      layer, it reurns the movie assocaited for the layer. Else       */
/*      return the main movie.                                          */
/************************************************************************/
SWFMovie GetCurrentMovie(mapObj *map, imageObj *image)
{
    int nTmp;

    if (!image || !map || !MS_DRIVER_SWF(image->format) )
      return NULL;

    if (strcasecmp(msGetOutputFormatOption(image->format,"OUTPUT_MOVIE",""), 
                   "MULTIPLE") == 0 && ((SWFObj *)image->img.swf)->nCurrentMovie >=0)
    {
        nTmp = ((SWFObj *)image->img.swf)->nCurrentMovie;
        return ((SWFObj *)image->img.swf)->pasMovies[nTmp];
    }
    else
      return ((SWFObj *)image->img.swf)->sMainMovie;
}

#endif

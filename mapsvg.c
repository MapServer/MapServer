/******************************************************************************
 * $Id$
 *
 * Name:     mapsvg.c
 * Project:  MapServer
 * Language: C
 * Purpose:  SVG output
 * Author:  DM Solutions Group (assefa@dmsolutions.ca)
 *
 *
 ******************************************************************************
 * Copyright (c) 1996-2004 Regents of the University of Minnesota.
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
 * Revision 1.1  2005/02/01 23:18:54  assefa
 * Initial version : svg output.
 *
 *
 */

#ifdef USE_SVG

#include <assert.h>
#include "map.h"

MS_CVSID("$Id$")


/************************************************************************/
/*                             msImageCreateSVG                         */
/*                                                                      */
/*      Create SVG image (called from prepareimage).                    */
/*                                                                      */
/*      Creates all necessary fileds to be able to output svg file.     */
/************************************************************************/
MS_DLL_EXPORT imageObj *msImageCreateSVG(int width, int height, 
                                         outputFormatObj *format, 
                                         char *imagepath, 
                                         char *imageurl, mapObj *map)
{
    double a, b , c, d, e, f;
    imageObj    *image = NULL;
    assert( strcasecmp(format->driver,"svg") == 0 );
    image = (imageObj *)calloc(1,sizeof(imageObj));

    image->format = format;
    format->refcount++;

    image->width = width;
    image->height = height;
    image->imagepath = NULL;
    image->imageurl = NULL;
    
    if (imagepath)
      image->imagepath = strdup(imagepath);

    if (imageurl)
        image->imageurl = strdup(imageurl);

    image->img.svg = (SVGObj *)malloc(sizeof(SVGObj));
    image->img.svg->filename = NULL;
    image->img.svg->map = map;

    if( map != NULL && map->web.imagepath != NULL )
      image->img.svg->filename = msTmpFile(map->mappath,map->web.imagepath,"svg");
    
    if (!image->img.svg->filename)
    {
        msSetError(MS_IOERR, "Failed to create temporary svg file.",
                    "msImageCreateSVG()" );
        return NULL;
    }
    
    image->img.svg->stream = fopen(image->img.svg->filename, "w" );

    if (!image->img.svg->stream)
    {
        msSetError(MS_IOERR, "Failed to open temporary svg file.",
                    "msImageCreateSVG()" );
        return NULL;
    }
    
    //initialize it with the svg header
    //TODO verify the encoding value (<?xml version="1.0" standalone="no"?>)

    msIO_fprintf(image->img.svg->stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    //TODO : seems to have a problem of dtd validation using IE (http://www.24help.info/printthread.php?t=52147)
    //msIO_fprintf(image->img.svg->stream, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
    msIO_fprintf(image->img.svg->stream, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11-flat.dtd\">\n");

    if (strcasecmp(msGetOutputFormatOption(image->format, "USE_GEOGRAPHICAL_COORDINATES",""), 
                   "TRUE") == 0)
    {
        a = width/(map->extent.maxx-map->extent.minx);
        b = 0;
        c = 0;
        d = -height/(map->extent.maxy - map->extent.miny);
        e = -width/(map->extent.maxx-map->extent.minx) *map->extent.minx;
        f = height/(map->extent.maxy-map->extent.miny)*map->extent.maxy;

        msIO_fprintf(image->img.svg->stream, "<svg version=\"1.1\" width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\" transform=\"matrix(%f,%f,%f,%f,%f,%f>)\">\n", 
                     width, height, a, b, c, d, e, f);
    }
    else
      msIO_fprintf(image->img.svg->stream, "<svg version=\"1.1\" width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\">\n",  width, height);

    
    return image;
}


/************************************************************************/
/*                              imagePolyline                           */
/*                                                                      */
/*      This function is similar to the imagePolyline function found    */
/*      in mapgd.c.                                                     */
/*                                                                      */
/*      Draws an svg line element.                                      */
/************************************************************************/
void static imagePolyline(FILE *fp, shapeObj *p, colorObj *color, int size,
                          int symbolstylelength, int *symbolstyle)
{
    int i, j, k;
    char *pszDashArray = NULL;
    char szTmp[100];

    if (!fp || !p || !color || size <0 )
      return;

    //the default size is 1.
    if (size <= 0)
      size = 1;


    for (i = 0; i < p->numlines; i++)
    {
        if (i == 0)
        {
            if (symbolstylelength > 0 && symbolstyle)
            {
                for (k=0; k<symbolstylelength; k++)
                {
                    if (k < symbolstylelength-1)
                      sprintf(szTmp, "%d, ", symbolstyle[k]);
                    else
                      sprintf(szTmp, "%d", symbolstyle[k]);

                    pszDashArray = strcatalloc(pszDashArray, szTmp);
                }
            
            msIO_fprintf(fp, "<polyline fill=\"none\" stroke=\"rgb(%d %d %d)\"  stroke-width=\"%d\" stroke-dasharray=\"%s\" points=\"",color->red, color->green, 
                         color->blue,size, pszDashArray);
            }
            else
              msIO_fprintf(fp, "<polyline fill=\"none\" stroke=\"rgb(%d %d %d)\"  stroke-width=\"%d\" points=\"",color->red, color->green, color->blue,size);
        
        }
        for(j=0; j<p->line[i].numpoints; j++)
        {
             msIO_fprintf(fp, " %f,%f", p->line[i].point[j].x, p->line[i].point[j].y);
        }
        
         if (i == (p->numlines -1))
            msIO_fprintf(fp, "\"/>\n");
    }
    
}




/************************************************************************/
/*                           msTransformShapeSVG                        */
/*                                                                      */
/*      Transform geographic coordinated to output coordinates.         */
/*                                                                      */
/*      This function uses output format options :                      */
/*        - if FULL_RESOLUTION is set to TRUE, there won't be any       */
/*      filtering done onthe shape coordinates. Default value is TRUE.  */
/*        - if USE_GEOGRAPHICAL_COORDINATES is set to TRUE, the         */
/*      shape is not transformed output coordinates (pixels) but        */
/*      written as is (geographical coordinates). The default value     */
/*      is FALSE.                                                       */
/*                                                                      */
/*      So of non of this parameters are specified, the defalut will    */
/*      output full resolution pixel coordinates.                       */
/************************************************************************/
void msTransformShapeSVG(shapeObj *shape, rectObj extent, double cellsize,
                         imageObj *image)
{
    int i,j; 
    int bFullRes, bUseGeoCoord;
    if (!image || !MS_DRIVER_SVG(image->format) )
      return;
    
    if(shape->numlines == 0) return; 



    bFullRes = 1;
    if (strcasecmp(msGetOutputFormatOption(image->format, "FULL_RESOLUTION",""), 
                   "FALSE") == 0)
      bFullRes = 0;

    bUseGeoCoord= 0;
    if (strcasecmp(msGetOutputFormatOption(image->format, "USE_GEOGRAPHICAL_COORDINATES",""), 
                   "TRUE") == 0)
      bUseGeoCoord = 1;

    //if it is full resolution and we want geographical output
    //just return, no transformations needed.
    if (bFullRes && bUseGeoCoord)
      return;

    //low resolution and pixel output
    if (!bFullRes && !bUseGeoCoord)
    {
        msTransformShapeToPixel(shape, extent, cellsize);
        return;
    }

    //full resolution, pixel output
    if (bFullRes && !bUseGeoCoord)
    {
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

    //low resolution, geographical output. 
    //Function comes from mapprimitive.c msTransformPixelToShape
    //TODO : adapat this function. For now just return fullres with geo coordinates
    if (!bFullRes && bUseGeoCoord)
    {
        return;
        
        /*
        if(shape->type == MS_SHAPE_LINE || shape->type == MS_SHAPE_POLYGON) { // remove co-linear vertices
  
            for(i=0; i<shape->numlines; i++) { // for each part
      
                shape->line[i].point[0].x = MS_MAP2IMAGE_X(shape->line[i].point[0].x, extent.minx, cellsize);
                shape->line[i].point[0].y = MS_MAP2IMAGE_Y(shape->line[i].point[0].y, extent.maxy, cellsize);
      
                for(j=1, k=1; j < shape->line[i].numpoints; j++ ) {
	
                    shape->line[i].point[k].x = MS_MAP2IMAGE_X(shape->line[i].point[j].x, extent.minx, cellsize);
                    shape->line[i].point[k].y = MS_MAP2IMAGE_Y(shape->line[i].point[j].y, extent.maxy, cellsize);

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
                shape->line[i].numpoints = k; // save actual number kept
            }
        } else { // points or untyped shapes
            for(i=0; i<shape->numlines; i++) { // for each part
                for(j=1; j < shape->line[i].numpoints; j++ ) {
                    shape->line[i].point[j].x = MS_MAP2IMAGE_X(shape->line[i].point[j].x, extent.minx, cellsize);
                    shape->line[i].point[j].y = MS_MAP2IMAGE_Y(shape->line[i].point[j].y, extent.maxy, cellsize);
                }
            }
        }
        */
    }
}

/************************************************************************/
/*                           msDrawLineSymbolSVG                        */
/*                                                                      */
/*      Draw line layers.                                               */
/************************************************************************/
MS_DLL_EXPORT void msDrawLineSymbolSVG(symbolSetObj *symbolset, 
                                       imageObj *image, 
                                       shapeObj *p, styleObj *style, 
                                       double scalefactor)
{
    //int styleDashed[100];
    int ox, oy;
    double size;
    int width;
    //gdPoint points[MS_MAXVECTORPOINTS];
    symbolObj *symbol;


/* -------------------------------------------------------------------- */
/*      if not SVG, return.                                             */
/* -------------------------------------------------------------------- */
    if (!image || !MS_DRIVER_SVG(image->format) )
        return;

    if(!p) return;
    if(p->numlines <= 0) return;

    if(style->size == -1) {
      size = (int)msSymbolGetDefaultSize( &( symbolset->symbol[style->symbol] ) );
    }
    else
      size = style->size;

    // TODO: Don't get this modification, is it needed elsewhere?
    if(size*scalefactor > style->maxsize) scalefactor = (float)style->maxsize/(float)size;
    if(size*scalefactor < style->minsize) scalefactor = (float)style->minsize/(float)size;
    size = MS_NINT(size*scalefactor);
    size = MS_MAX(size, style->minsize);
    size = MS_MIN(size, style->maxsize);

    width = MS_NINT(style->width*scalefactor);
    width = MS_MAX(width, style->minwidth);
    width = MS_MIN(width, style->maxwidth);

    if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; // no such symbol, 0 is OK

     if (!MS_VALID_COLOR( style->color))
      return;
    
    if(size < 1) return; // size too small

    //TODO : do we need offset ??
    ox = MS_NINT(style->offsetx*scalefactor);
    oy = (style->offsety == -99) ? -99 : (int)(style->offsety*scalefactor);

    symbol = &(symbolset->symbol[style->symbol]);


    //TODO : all lines are draw as filled lines. 
    
    //if default symbol 0 is used, use the style's width parameter
    // else use the symbol size.
    //TODO : size needs to be double ?
    if(style->symbol == 0) 
      imagePolyline(image->img.svg->stream, p, &style->color, width,
                    symbol->stylelength,  symbol->style);
    else
      imagePolyline(image->img.svg->stream, p, &style->color, (int)size,
                    symbol->stylelength,  symbol->style);
        return;
    

}

/************************************************************************/
/*                           msImageStartLayerSVG                       */
/************************************************************************/
void msImageStartLayerSVG(mapObj *map, layerObj *layer, imageObj *image)
{
    if (image && MS_DRIVER_SVG(image->format) && layer && map)// && filename)
        msIO_fprintf(image->img.svg->stream, "\n<!-- START LAYER %s -->\n", layer->name); 
}



static void imageFillPolygon(FILE *fp, shapeObj *p, colorObj *psFillColor, 
                             colorObj *psOutlineColor, int size,
                             int symbolstylelength, int *symbolstyle)
{
    int i, j, k;
    char *pszDashArray = NULL;
    char szTmp[100];


    if (!fp || !p || (psFillColor==NULL && psOutlineColor==NULL) || size <0 )
      return;

    //the default size is 1.
    if (size <= 0)
      size = 1;

    for (i = 0; i < p->numlines; i++)
    {
        if (i == 0)
        {
            pszDashArray = strcatalloc(pszDashArray,"");
            if (symbolstylelength > 0 && symbolstyle)
            {
                sprintf(szTmp, "stroke-dasharray=\"");
                pszDashArray = strcatalloc(pszDashArray, szTmp);
                for (k=0; k<symbolstylelength; k++)
                {
                    if (k < symbolstylelength-1)
                      sprintf(szTmp, "%d, ", symbolstyle[k]);
                    else
                      sprintf(szTmp, "%d\"", symbolstyle[k]);

                    pszDashArray = strcatalloc(pszDashArray, szTmp);
                }
            }
            if (psOutlineColor && psFillColor)
              msIO_fprintf(fp, "<polygon fill=\"rgb(%d %d %d)\" stroke=\"rgb(%d %d %d)\"  stroke-width=\"%d\" %s points=\"",
                           psFillColor->red, psFillColor->green, 
                           psFillColor->blue,
                           psOutlineColor->red, psOutlineColor->green, 
                           psOutlineColor->blue,
                           size, pszDashArray);
            
            else if (psOutlineColor)
            {
                msIO_fprintf(fp, "<polygon  stroke=\"rgb(%d %d %d)\"  stroke-width=\"%d\" %s points=\"",
                           psOutlineColor->red, psOutlineColor->green, 
                           psOutlineColor->blue,
                           size, pszDashArray);
            }
            else //fill color valid 
            {
                msIO_fprintf(fp, "<polygon fill=\"rgb(%d %d %d)\" stroke=\"rgb(%d %d %d)\"  stroke-width=\"%d\" %s points=\"",
                             psFillColor->red, psFillColor->green, 
                             psFillColor->blue);
                           
            }
            
           
        }
        for(j=0; j<p->line[i].numpoints; j++)
        {
            msIO_fprintf(fp, " %f,%f", p->line[i].point[j].x, p->line[i].point[j].y);
        }
        
        if (i == (p->numlines -1))
          msIO_fprintf(fp, "\"/>\n");
    }
    
}


/************************************************************************/
/*                           msDrawShadeSymbolSVG                       */
/*                                                                      */
/*      Drawing of POLYGON layers.                                      */
/************************************************************************/
void msDrawShadeSymbolSVG(symbolSetObj *symbolset, imageObj *image, 
                          shapeObj *p, styleObj *style, double scalefactor)
{
    colorObj    *psFillColor = NULL;
    colorObj    *psOutlineColor = NULL;
    colorObj    sFc;
    colorObj    sOc;
    symbolObj   *symbol;
    int         size;
/* -------------------------------------------------------------------- */
/*      if not svg, return.                                             */
/* -------------------------------------------------------------------- */
    if (image == NULL || !MS_DRIVER_SVG(image->format) )
      return;
    
    if(p == NULL || p->numlines <= 0)
      return;


    symbol = &(symbolset->symbol[style->symbol]);

    if(style->size == -1) 
    {
        size = (int)msSymbolGetDefaultSize( &( symbolset->symbol[style->symbol] ) );
        size = MS_NINT(size*scalefactor);
    }
    else
      size = MS_NINT(style->size*scalefactor);

    size = MS_MAX(size, style->minsize);
    size = MS_MIN(size, style->maxsize);

    if(style->symbol > symbolset->numsymbols || style->symbol < 0) /* no such symbol, 0 is OK */
        return;

    if(size < 1) /* size too small */
        return;

    sFc.red = style->color.red;
    sFc.green = style->color.green;
    sFc.blue = style->color.blue;

    sOc.red = style->outlinecolor.red;
    sOc.green = style->outlinecolor.green;
    sOc.blue = style->outlinecolor.blue;

    if (MS_VALID_COLOR(sFc))
      psFillColor = &sFc;
    if (MS_VALID_COLOR(sOc))
      psOutlineColor = &sOc;

    imageFillPolygon(image->img.svg->stream, p, psFillColor, psOutlineColor, size,
                     symbol->stylelength,  symbol->style);

}


static void drawSVGText(FILE *fp, int x, int y, char *string, double size, 
                        colorObj *psColor, colorObj *psOutlineColor,
                        char *pszFontFamilily,  char *pszFontStyle,
                        char *pszFontWeight, int nPosition)
{
    char *pszFontStyleString = NULL;
    char *pszFontWeightString = NULL;
    char *pszFillString = NULL, *pszStrokeString = NULL;
     char szTmp[100];

    pszFontStyleString = strcatalloc(pszFontStyleString, "");
    pszFontWeightString = strcatalloc(pszFontWeight, "");


    if (pszFontStyle)
    {
        sprintf(szTmp, "font-style=\"%s\"", pszFontStyle);
        pszFontStyleString = strcatalloc(pszFontStyleString, szTmp);
    }

    if (pszFontWeight)
    {
        sprintf(szTmp, "font-weight=\"%s\"", pszFontWeight);
        pszFontWeightString = strcatalloc(pszFontWeightString, szTmp);
    }
 
    pszFillString =  strcatalloc(pszFillString, "");
    if (psColor)
    {
        if (MS_VALID_COLOR(*psColor))
          sprintf(szTmp, "fill=\"rgb(%d %d %d)\"",
                  psColor->red, psColor->green  , psColor->blue);
        else
          sprintf(szTmp,"fill=\"none\"");

        pszFillString = strcatalloc(pszFillString, szTmp);
    }

    pszStrokeString =  strcatalloc(pszStrokeString, "");
    if (psOutlineColor && MS_VALID_COLOR(*psOutlineColor))
    {
        sprintf(szTmp, "stroke=\"rgb(%d %d %d)\" stroke-width=\"0.5\"",
                psOutlineColor->red, psOutlineColor->green  , psOutlineColor->blue);
        pszStrokeString = strcatalloc(pszStrokeString, szTmp);
    }

    
    msIO_fprintf(fp, "<text  x=\"%d\" y=\"%d\" font-famility=\"%s\" font-size=\"%f\" %s %s %s %s>%s</text>\n",
                 x, y, pszFontFamilily, size, 
                 pszFontStyleString, pszFontWeightString, 
                 pszFillString, pszStrokeString, string);

    
}
int msDrawTextSVG(imageObj *image, pointObj labelPnt, char *string, 
                 labelObj *label, fontSetObj *fontset, double scalefactor)
{
    char *error=NULL, *font=NULL;
    int bbox[8];
    double angle_radians = MS_DEG_TO_RAD*label->angle;
    double size;
    colorObj sColor, sOutlineColor;
    char **aszFontsParts = NULL;
    int nFontParts= 0;
    char *pszFontFamily = NULL, *pszFontStyle=NULL, *pszFontWeight=NULL;
    int x, y;

/* -------------------------------------------------------------------- */
/*      if not svg or invaid inputs, return.                            */
/* -------------------------------------------------------------------- */
    if (!image || !string || (strlen(string) == 0) || !label || !fontset || 
        !MS_DRIVER_SVG(image->format) )
      return (0);

    if( 0)//label->encoding != NULL ) 
    { /* converting the label encoding */
        string = msGetEncodedString(string, label->encoding);
        if(string == NULL) return(-1);
    }

    x = MS_NINT(labelPnt.x);
    y = MS_NINT(labelPnt.y);


#ifndef USE_GD_FT
    msSetError(MS_TTFERR, "TrueType font support is not available.", "msDrawTextSVG()");
    //if(label->encoding != NULL) msFree(string);
    return(-1); 
#endif

 
    if(label->type == MS_TRUETYPE) 
    {
        sColor.red = -1;
        sColor.green = -1;
        sColor.blue = -1;

        sOutlineColor.red = -1;
        sOutlineColor.green = -1;
        sOutlineColor.blue = -1;

        //TODO : check support for all parameters 
        //color, outlinecolor, shadowcolor, backgroundcolor, backgroundshadowcolor
        //position, offset, angle, buffer, antialias, wrap, encoding

        size = label->size*scalefactor;
        size = MS_MAX(size, label->minsize);
        size = MS_MIN(size, label->maxsize);

        if(!fontset) {
            msSetError(MS_TTFERR, "No fontset defined.", "msDrawTextSVG()");
            if(label->encoding != NULL) msFree(string);
            return(-1);
        }
        
        if(!label->font) {
            msSetError(MS_TTFERR, "No Trueype font defined.", "msDrawTextGD()");
            if(label->encoding != NULL) msFree(string);
            return(-1);
        }
        
        font = msLookupHashTable(&(fontset->fonts), label->font);
        if(!font) {
            msSetError(MS_TTFERR, "Requested font (%s) not found.", "msDrawTextSVg()", label->font);
            if(label->encoding != NULL) msFree(string);
            return(-1);
        }
    
        if (MS_VALID_COLOR(label->color))
        {
            sColor.red = label->color.red;
            sColor.green = label->color.green;
            sColor.blue = label->color.blue;
        }
        if (MS_VALID_COLOR(label->outlinecolor))
        {
            sOutlineColor.red = label->outlinecolor.red;
            sOutlineColor.green = label->outlinecolor.green;
            sOutlineColor.blue = label->outlinecolor.blue;
        }

        if (!MS_VALID_COLOR(label->color) && !MS_VALID_COLOR(label->outlinecolor))
        {
            msSetError(MS_TTFERR, "Invalid color", "draw_textSWF()");
            return(-1);
        }
        
/* -------------------------------------------------------------------- */
/*      parse font string :                                             */
/*      There are 3 parts to the name separated with - (example         */
/*      arial-italic-bold):                                             */
/*        - font-family,                                                */
/*        - font-style (italic, oblique, nomal)                         */
/*        - font-weight (normal | bold | bolder | lighter | 100 | 200 ..*/
/* -------------------------------------------------------------------- */
        //parse font string : 
        aszFontsParts = split(label->font, '-', &nFontParts);
 
        pszFontFamily = aszFontsParts[0];
        if (nFontParts == 3)
        {
            pszFontStyle = aszFontsParts[1];
            pszFontWeight = aszFontsParts[2];
        }
        else if (nFontParts == 2)
        {
            if (strcasecmp(aszFontsParts[1], "italic") == 0 ||
                strcasecmp(aszFontsParts[1], "oblique") == 0 ||
                strcasecmp(aszFontsParts[1], "normal") == 0)
              pszFontStyle = aszFontsParts[1];
            else
              pszFontWeight = aszFontsParts[1];
        }

        drawSVGText(image->img.svg->stream, x, y, string, size, 
                    &sColor, &sOutlineColor,
                    pszFontFamily,  pszFontStyle, pszFontWeight,
                    label->position);

        msFreeCharArray(aszFontsParts, nFontParts);

        return 0;
    }

    return -1;
}

void msDrawMarkerSymbolSVG(symbolSetObj *symbolset, imageObj *image, 
                          pointObj *p, styleObj *style, double scalefactor)
{

/* -------------------------------------------------------------------- */
/*      if not svg or invaid inputs, return.                            */
/* -------------------------------------------------------------------- */
    if (!image || !p || !style || !MS_DRIVER_SVG(image->format) )
      return;

    

    
}

/************************************************************************/
/*                              msSaveImageSVG                          */
/*                                                                      */
/*      Save SVG to file or send to standard output.                    */
/************************************************************************/
MS_DLL_EXPORT int msSaveImageSVG(imageObj *image, char *filename)
{
    FILE *fp = NULL;
    unsigned char block[4000];
    int bytes_read;
    char *sttt = NULL;

    //strlen(sttt);

    if (image && MS_DRIVER_SVG(image->format))// && filename)
    {
        msIO_fprintf(image->img.svg->stream, "</svg>\n"); 
        fclose(image->img.svg->stream);

        if (!filename)
        {
          //if( msIO_needBinaryStdout() == MS_FAILURE )
          //    return MS_FAILURE;

            fp = fopen(image->img.svg->filename, "rb" );
            if( fp == NULL )
            {
                msSetError( MS_MISCERR, 
                            "Failed to open %s for streaming to stdout.",
                            "msSaveImageSVG()", image->img.svg->filename);
                return MS_FAILURE;
            }
            
            while( (bytes_read = fread(block, 1, sizeof(block), fp)) > 0 )
              msIO_fwrite( block, 1, bytes_read, stdout );

            fclose( fp );
        }

    }
    return MS_SUCCESS;
}


MS_DLL_EXPORT void msFreeImageSVG(imageObj *image)
{
}


#endif

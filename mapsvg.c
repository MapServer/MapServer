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
 * Revision 1.4  2005/02/09 18:30:50  assefa
 * goSVG specific metadata added.
 *
 * Revision 1.3  2005/02/05 00:07:14  assefa
 * Add support for lables with encoding.
 *
 * Revision 1.2  2005/02/03 00:07:39  assefa
 * Add partial text and raster support.
 *
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
    double      a, b , c, d, e, f;
    imageObj    *image = NULL;
    static char epsgCode[20] ="";
    static char *value;
    int         nZoomInTH, nZoomOutTH, nScrollTH;


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

    if (! map->web.imagepath)
    {
        msSetError(MS_MISCERR, "web image path need to be set.",
                   "msImageCreateSVG");
        return NULL;
    }

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

    msIO_fprintf(image->img.svg->stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    //TODO : seems to have a problem of dtd validation using IE (http://www.24help.info/printthread.php?t=52147)
    //msIO_fprintf(image->img.svg->stream, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
    msIO_fprintf(image->img.svg->stream, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11-flat.dtd\">\n");


/* -------------------------------------------------------------------- */
/*      Special output for goSVG support.                               */
/* -------------------------------------------------------------------- */
    if (strcasecmp(msGetOutputFormatOption(image->format, "GOSVG",""),
                   "TRUE") == 0)
    {
        //transformation from geographical coordinates to pixel coordinates
        a = width/(map->extent.maxx-map->extent.minx);
        b = 0;
        c = 0;
        d = -height/(map->extent.maxy - map->extent.miny);
        e = -width/(map->extent.maxx-map->extent.minx) *map->extent.minx;
        f = height/(map->extent.maxy-map->extent.miny)*map->extent.maxy;

        msIO_fprintf(image->img.svg->stream, "<svg version=\"1.1\" width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:au=\"http://www.svgmovile.jp/2004/kddip\"  au:boundingBox=\"0 0 %d %d\">\n",  width, height, width, height);   
        msIO_fprintf(image->img.svg->stream, "<title>%s</title>\n", map->name);
        msIO_fprintf(image->img.svg->stream, "<metadata> \n");
        msIO_fprintf(image->img.svg->stream,"<rdf:RDF xmlns:rdf = \"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" \n");
        msIO_fprintf(image->img.svg->stream, "xmlns:crs = \"http://www.ogc.org/crs\" xmlns:svg=\"http://wwww.w3.org/2000/svg\"> \n");
        msIO_fprintf(image->img.svg->stream, "<rdf:Description> \n");
        msIO_fprintf(image->img.svg->stream, "<crs:CoordinateReferenceSystem svg:transform=\"matrix(%f,%f,%f,%f,%f,%f)\" \n", a, b, c, d, e, f);
        
        //get epsg code: TODO : test what if espg code is not set ?
        if (map->projection.numargs > 0 && 
            (value = strstr(map->projection.args[0], "init=epsg:")) != NULL && 
            strlen(value) < 20) 
        {
            sprintf(epsgCode, "%s", value+10);
            msIO_fprintf(image->img.svg->stream, "rdf:resource=\"http://www.opengis.net/gml/srs/epsg.xml#%s\"/> \n",epsgCode);
        }
            
        msIO_fprintf(image->img.svg->stream, "</rdf:Description> \n </rdf:RDF> \n"); 
        
        nZoomInTH = 70;
        nZoomOutTH = 100;
        nScrollTH = 10;
        
        nZoomInTH = atoi(msGetOutputFormatOption(image->format, "GOSVG_ZoomInTH","70"));
        nZoomOutTH = atoi(msGetOutputFormatOption(image->format, "GOSVG_ZoomOutTH","100"));
        nScrollTH = atoi(msGetOutputFormatOption(image->format, "GOSVG_ScrollTH","10"));

        msIO_fprintf(image->img.svg->stream, "<au:lbs protocol=\"maprequest\">\n");
        msIO_fprintf(image->img.svg->stream, 
                     "<au:zoomin th=\"%d\" xlink:href=\".\"/>\n", nZoomInTH);
        msIO_fprintf(image->img.svg->stream, 
                     "<au:zoomout th=\"%d\" xlink:href=\".\"/>\n", nZoomOutTH);
        msIO_fprintf(image->img.svg->stream, 
                     "<au:scroll th=\"%d\" xlink:href=\".\"/>\n", nScrollTH);
        msIO_fprintf(image->img.svg->stream, "</au:lbs>\n");

        msIO_fprintf(image->img.svg->stream,"</metadata>\n");
    }
    else
    {
        msIO_fprintf(image->img.svg->stream, "<svg version=\"1.1\" width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n",  width, height);   
    }
    
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
            
            msIO_fprintf(fp, "<polyline fill=\"none\" stroke=\"#%02x%02x%02x\"  stroke-width=\"%d\" stroke-dasharray=\"%s\" points=\"",color->red, color->green, 
                         color->blue,size, pszDashArray);
            }
            else
              msIO_fprintf(fp, "<polyline fill=\"none\" stroke=\"#%02x%02x%02x\"  stroke-width=\"%d\" points=\"",color->red, color->green, color->blue,size);
        
        }
        for(j=0; j<p->line[i].numpoints; j++)
        {
             msIO_fprintf(fp, " %d,%d", (int)p->line[i].point[j].x, (int)p->line[i].point[j].y);
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
    
    //if (strcasecmp(msGetOutputFormatOption(image->format, "USE_GEOGRAPHICAL_COORDINATES",""), 
    //              "TRUE") == 0)
    // bUseGeoCoord = 1;

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
                    /*
                    shape->line[i].point[j].x = 
                      (shape->line[i].point[j].x - extent.minx)/cellsize;
                    shape->line[i].point[j].y = 
                      (extent.maxy - shape->line[i].point[j].y)/cellsize;
                    
                    */
                    shape->line[i].point[j].x = MS_MAP2IMAGE_X(shape->line[i].point[j].x,
                                                               extent.minx, cellsize);
                    shape->line[i].point[j].y = MS_MAP2IMAGE_Y( shape->line[i].point[j].y,
                                                                extent.maxy,
                                                                cellsize);
                                                                
                    
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
              msIO_fprintf(fp, "<polygon fill=\"#%02x%02x%02x\" stroke=\"#%02x%02x%02x\"  stroke-width=\"%d\" %s points=\"",
                           psFillColor->red, psFillColor->green, 
                           psFillColor->blue,
                           psOutlineColor->red, psOutlineColor->green, 
                           psOutlineColor->blue,
                           size, pszDashArray);
            
            else if (psOutlineColor)
            {
                msIO_fprintf(fp, "<polygon  stroke=\"#%02x%02x%02x\"  stroke-width=\"%d\" %s points=\"",
                           psOutlineColor->red, psOutlineColor->green, 
                           psOutlineColor->blue,
                           size, pszDashArray);
            }
            else //fill color valid 
            {
                msIO_fprintf(fp, "<polygon fill=\"#%02x%02x%02x\" points=\"",
                             psFillColor->red, psFillColor->green, 
                             psFillColor->blue);
                           
            }
            
           
        }
        for(j=0; j<p->line[i].numpoints; j++)
        {
            msIO_fprintf(fp, " %d,%d", (int)p->line[i].point[j].x, (int)p->line[i].point[j].y);
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

#define Tcl_UniChar int
#define TCL_UTF_MAX 3

static int gdTcl_UtfToUniChar (char *str, Tcl_UniChar * chPtr)
{
  int byte;
  
  /*
   * Unroll 1 to 3 byte UTF-8 sequences, use loop to handle longer ones.
   */

  byte = *((unsigned char *) str);

  if (byte < 0xC0)
    {
      /*
       * Handles properly formed UTF-8 characters between
       * 0x01 and 0x7F.  Also treats \0 and naked trail
       * bytes 0x80 to 0xBF as valid characters representing
       * themselves.
       */

      *chPtr = (Tcl_UniChar) byte;
      return 1;
    }
  else if (byte < 0xE0)
    {
      if ((str[1] & 0xC0) == 0x80)
	{
	  /*
	   * Two-byte-character lead-byte followed
	   * by a trail-byte.
	   */

	  *chPtr = (Tcl_UniChar) (((byte & 0x1F) << 6) | (str[1] & 0x3F));
	  return 2;
	}
      /*
       * A two-byte-character lead-byte not followed by trail-byte
       * represents itself.
       */

      *chPtr = (Tcl_UniChar) byte;
      return 1;
    }
  else if (byte < 0xF0)
    {
      if (((str[1] & 0xC0) == 0x80) && ((str[2] & 0xC0) == 0x80))
	{
	  /*
	   * Three-byte-character lead byte followed by
	   * two trail bytes.
	   */

	  *chPtr = (Tcl_UniChar) (((byte & 0x0F) << 12)
				  | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F));
	  return 3;
	}
      /*
       * A three-byte-character lead-byte not followed by
       * two trail-bytes represents itself.
       */

      *chPtr = (Tcl_UniChar) byte;
      return 1;
    }
#if TCL_UTF_MAX > 3
  else
    {
      int ch, total, trail;

      total = totalBytes[byte];
      trail = total - 1;
      if (trail > 0)
	{
	  ch = byte & (0x3F >> trail);
	  do
	    {
	      str++;
	      if ((*str & 0xC0) != 0x80)
		{
		  *chPtr = byte;
		  return 1;
		}
	      ch <<= 6;
	      ch |= (*str & 0x3F);
	      trail--;
	    }
	  while (trail > 0);
	  *chPtr = ch;
	  return total;
	}
    }
#endif

  *chPtr = (Tcl_UniChar) byte;
  return 1;
}

static void drawSVGText(FILE *fp, int x, int y, char *string, double size, 
                        colorObj *psColor, colorObj *psOutlineColor,
                        char *pszFontFamilily,  char *pszFontStyle,
                        char *pszFontWeight, int nAnchorPosition,
                        double dfAngle, int bEncoding)
{
    char *pszFontStyleString = NULL;
    char *pszFontWeightString = NULL;
    char *pszFillString = NULL, *pszStrokeString = NULL;
    char *pszAngleString=NULL, *pszAngleAnchorString=NULL;
    char *pszFinalString=NULL;
    char *pszTmpStr = NULL;
    char szTmp[100];
    char *next;
    int len, i, ch;

    pszFontStyleString = strcatalloc(pszFontStyleString, "");
    pszFontWeightString = strcatalloc(pszFontWeightString, "");
    pszAngleString = strcatalloc(pszAngleString, "");
    pszAngleAnchorString = strcatalloc(pszAngleAnchorString, "");

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
          sprintf(szTmp, "fill=\"#%02x%02x%02x\"",
                  psColor->red, psColor->green  , psColor->blue);
        else
          sprintf(szTmp,"fill=\"none\"");

        pszFillString = strcatalloc(pszFillString, szTmp);
    }

    pszStrokeString =  strcatalloc(pszStrokeString, "");
    if (psOutlineColor && MS_VALID_COLOR(*psOutlineColor))
    {
        sprintf(szTmp, "stroke=\"#%02x%02x%02x\" stroke-width=\"0.5\"",
                psOutlineColor->red, psOutlineColor->green  , psOutlineColor->blue);
        pszStrokeString = strcatalloc(pszStrokeString, szTmp);
    }

    //angle
    if (dfAngle > 0.0)
    {
        sprintf(szTmp, "transform=\"rotate(%f %d %d)\"",
                -dfAngle, x, y);
        pszAngleString = strcatalloc(pszAngleString, szTmp);
    }

    //anchor point
    if (nAnchorPosition == MS_UL || nAnchorPosition == MS_CL ||
        nAnchorPosition == MS_LL)
    {
        sprintf(szTmp, "text-anchor=\"end\"");
        pszAngleAnchorString = strcatalloc(pszAngleAnchorString, szTmp);
    }
    else if (nAnchorPosition == MS_UC || nAnchorPosition == MS_CC ||
             nAnchorPosition == MS_LC)
    {
        sprintf(szTmp, "text-anchor=\"middle\"");
        pszAngleAnchorString = strcatalloc(pszAngleAnchorString, szTmp);
    }
    else if (nAnchorPosition == MS_UR || nAnchorPosition == MS_CR ||
             nAnchorPosition == MS_LR)
    {
        sprintf(szTmp, "text-anchor=\"start\"");
        pszAngleAnchorString = strcatalloc(pszAngleAnchorString, szTmp);
    }

    if (bEncoding)
    {
        pszTmpStr = strcatalloc(pszTmpStr, string);
        next = pszTmpStr;
        for (i=0; *next; i++)
        {
            ch = *next;
            
            
            len = gdTcl_UtfToUniChar (next, &ch);
            next += len;
        
            sprintf(szTmp, "&#x%x;",ch);
            pszFinalString = strcatalloc(pszFinalString, szTmp);
        }
    }
    else
      pszFinalString = string;

    //TODO : font size set to unit pt
    msIO_fprintf(fp, "<text  x=\"%d\" y=\"%d\" font-family=\"%s\" font-size=\"%dpt\" %s %s %s %s %s %s>%s</text>\n",
                 x, y, pszFontFamilily, (int)size, 
                 pszFontStyleString, pszFontWeightString, 
                 pszFillString, pszStrokeString, pszAngleString, 
                 pszAngleAnchorString, pszFinalString);

    if (pszFontStyleString)
      msFree(pszFontStyleString);
    if (pszFontWeightString)
      msFree(pszFontWeightString);
    if (pszFillString)
      msFree(pszFillString);
    if (pszStrokeString)
      msFree(pszStrokeString);
    if (pszAngleString)
      msFree(pszAngleString);
    if (pszAngleAnchorString)
      msFree(pszAngleAnchorString);
    if (bEncoding && pszFinalString)
      msFree(pszFinalString);
    
}


/************************************************************************/
/*                              msDrawTextSVG                           */
/************************************************************************/
int msDrawTextSVG(imageObj *image, pointObj labelPnt, char *string, 
                 labelObj *label, fontSetObj *fontset, double scalefactor)
{
    char  *font=NULL;
    double size;
    colorObj sColor, sOutlineColor;
    char **aszFontsParts = NULL;
    int nFontParts= 0;
    char *pszFontFamily = NULL, *pszFontStyle=NULL, *pszFontWeight=NULL;
    int x, y;
    int bEncoding = 0;

/* -------------------------------------------------------------------- */
/*      if not svg or invaid inputs, return.                            */
/* -------------------------------------------------------------------- */
    if (!image || !string || (strlen(string) == 0) || !label || !fontset || 
        !MS_DRIVER_SVG(image->format) )
      return (0);

    if(label->encoding != NULL ) 
    { /* converting the label encoding */
        
        string = msGetEncodedString(string, label->encoding);
        if(string == NULL) return(-1);
        bEncoding = 1;
    }

    //TODO : not transform it to integer is layer transform is false
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
            msSetError(MS_TTFERR, "Invalid color", "drawSVGText()");
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
        aszFontsParts = split(label->font, '_', &nFontParts);
 
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
                    label->position, label->angle, bEncoding);

        //msFreeCharArray(aszFontsParts, nFontParts);

        return 0;
    }

    return -1;
}


/************************************************************************/
/*                           msDrawLabelCacheSVG                        */
/*                                                                      */
/*      This function comes from mapgd.c (function                      */
/*      msDrawLabelCacheGD) with minor adjustments.                     */
/************************************************************************/
int msDrawLabelCacheSVG(imageObj *image, mapObj *map)
{
  pointObj p;
  int i, j, l;
  rectObj r;
  
  labelCacheMemberObj *cachePtr=NULL;
  layerObj *layerPtr=NULL;
  labelObj *labelPtr=NULL;

  int marker_width, marker_height;
  int marker_offset_x, marker_offset_y;
  rectObj marker_rect;

/* -------------------------------------------------------------------- */
/*      if not svg or invaid inputs, return.                            */
/* -------------------------------------------------------------------- */
    if (!image || !map || !MS_DRIVER_SVG(image->format) )
      return (0);

  
  for(l=map->labelcache.numlabels-1; l>=0; l--) {

    cachePtr = &(map->labelcache.labels[l]); // point to right spot in the label cache

    layerPtr = &(map->layers[cachePtr->layerindex]); // set a couple of other pointers, avoids nasty references
    labelPtr = &(cachePtr->label);

    if(!cachePtr->text || strlen(cachePtr->text) == 0)
      continue; // not an error, just don't want to do anything

    if(msGetLabelSize(cachePtr->text, labelPtr, &r, &(map->fontset), layerPtr->scalefactor) == -1)
      return(-1);

    if(labelPtr->autominfeaturesize && ((r.maxx-r.minx) > cachePtr->featuresize))
      continue; /* label too large relative to the feature */

    marker_offset_x = marker_offset_y = 0; /* assume no marker */
    if((layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) || layerPtr->type == MS_LAYER_POINT) { // there *is* a marker      

      // TO DO: at the moment only checks the bottom style, perhaps should check all of them
      if(msGetMarkerSize(&map->symbolset, &(cachePtr->styles[0]), &marker_width, &marker_height, layerPtr->scalefactor) != MS_SUCCESS)
	return(-1);

      marker_offset_x = MS_NINT(marker_width/2.0);
      marker_offset_y = MS_NINT(marker_height/2.0);      

      marker_rect.minx = MS_NINT(cachePtr->point.x - .5 * marker_width);
      marker_rect.miny = MS_NINT(cachePtr->point.y - .5 * marker_height);
      marker_rect.maxx = marker_rect.minx + (marker_width-1);
      marker_rect.maxy = marker_rect.miny + (marker_height-1); 
    }

    if(labelPtr->position == MS_AUTO) {

      if(layerPtr->type == MS_LAYER_LINE) {
	int position = MS_UC;

	for(j=0; j<2; j++) { /* Two angles or two positions, depending on angle. Steep angles will use the angle approach, otherwise we'll rotate between UC and LC. */

	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

	  if(j == 1) {
	    if(fabs(cos(labelPtr->angle)) < LINE_VERT_THRESHOLD)
	      labelPtr->angle += 180.0;
	    else
	      position = MS_LC;
	  }

	  p = get_metrics(&(cachePtr->point), position, r, (marker_offset_x + labelPtr->offsetx), (marker_offset_y + labelPtr->offsety), labelPtr->angle, labelPtr->buffer, cachePtr->poly);

	  if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
	    msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon

	  if(!labelPtr->partials) { // check against image first
	    if(labelInImage(map->width, map->height, cachePtr->poly, labelPtr->buffer) == MS_FALSE) {
	      cachePtr->status = MS_FALSE;
	      continue; // next angle
	    }
	  }

	  for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	    if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	      if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(!cachePtr->status)
	    continue; // next angle

	  for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
	    if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */

	      if((labelPtr->mindistance != -1) && (cachePtr->classindex == map->labelcache.labels[i].classindex) && (strcmp(cachePtr->text,map->labelcache.labels[i].text) == 0) && (msDistancePointToPoint(&(cachePtr->point), &(map->labelcache.labels[i].point)) <= labelPtr->mindistance)) { /* label is a duplicate */
		cachePtr->status = MS_FALSE;
		break;
	      }

	      if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(cachePtr->status) // found a suitable place for this label
	    break;

	} // next angle

      } else {
	for(j=0; j<=7; j++) { /* loop through the outer label positions */

	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

	  p = get_metrics(&(cachePtr->point), j, r, (marker_offset_x + labelPtr->offsetx), (marker_offset_y + labelPtr->offsety), labelPtr->angle, labelPtr->buffer, cachePtr->poly);

	  if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
	    msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon

	  if(!labelPtr->partials) { // check against image first
	    if(labelInImage(map->width, map->height, cachePtr->poly, labelPtr->buffer) == MS_FALSE) {
	      cachePtr->status = MS_FALSE;
	      continue; // next position
	    }
	  }

	  for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	    if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	      if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(!cachePtr->status)
	    continue; // next position

	  for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
	    if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */

	      if((labelPtr->mindistance != -1) && (cachePtr->classindex == map->labelcache.labels[i].classindex) && (strcmp(cachePtr->text,map->labelcache.labels[i].text) == 0) && (msDistancePointToPoint(&(cachePtr->point), &(map->labelcache.labels[i].point)) <= labelPtr->mindistance)) { /* label is a duplicate */
		cachePtr->status = MS_FALSE;
		break;
	      }

	      if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }

	  if(cachePtr->status) // found a suitable place for this label
	    break;
	} // next position
      }

      if(labelPtr->force) cachePtr->status = MS_TRUE; /* draw in spite of collisions based on last position, need a *best* position */

    } else {
      cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

      if(labelPtr->position == MS_CC) // don't need the marker_offset
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, labelPtr->offsetx, labelPtr->offsety, labelPtr->angle, labelPtr->buffer, cachePtr->poly);
      else
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, (marker_offset_x + labelPtr->offsetx), (marker_offset_y + labelPtr->offsety), labelPtr->angle, labelPtr->buffer, cachePtr->poly);

      if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
	msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon, part of overlap tests */

      if(!labelPtr->force) { // no need to check anything else

	if(!labelPtr->partials) {
	  if(labelInImage(map->width, map->height, cachePtr->poly, labelPtr->buffer) == MS_FALSE)
	    cachePtr->status = MS_FALSE;
	}

	if(!cachePtr->status)
	  continue; // next label

	for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	  if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	    if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
	      cachePtr->status = MS_FALSE;
	      break;
	    }
	  }
	}

	if(!cachePtr->status)
	  continue; // next label

	for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered label
	  if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */
	    if((labelPtr->mindistance != -1) && (cachePtr->classindex == map->labelcache.labels[i].classindex) && (strcmp(cachePtr->text, map->labelcache.labels[i].text) == 0) && (msDistancePointToPoint(&(cachePtr->point), &(map->labelcache.labels[i].point)) <= labelPtr->mindistance)) { /* label is a duplicate */
	      cachePtr->status = MS_FALSE;
	      break;
	    }

	    if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */          
	      cachePtr->status = MS_FALSE;
	      break;
	    }
	  }
	}
      }
    } /* end position if-then-else */

    /* imagePolyline(img, cachePtr->poly, 1, 0, 0); */

    if(!cachePtr->status)
      continue; /* next label */

    if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) { /* need to draw a marker */
      for(i=0; i<cachePtr->numstyles; i++)
        msDrawMarkerSymbolSVG(&map->symbolset, image, &(cachePtr->point), &(cachePtr->styles[i]), layerPtr->scalefactor);
    }

    //background not supported
    //if(MS_VALID_COLOR(labelPtr->backgroundcolor)) billboardGD(img, cachePtr->poly, labelPtr);
    msDrawTextSVG(image, p, cachePtr->text, labelPtr, &(map->fontset), layerPtr->scalefactor); // actually draw the label

  } // next label

  
  return(0);
}

/************************************************************************/
/*                          msDrawMarkerSymbolSVG                       */
/*                                                                      */
/*      Draw symbols.                                                   */
/************************************************************************/
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
    FILE *fp = NULL, *stream=NULL;
    unsigned char block[4000];
    int bytes_read;

    if (image && MS_DRIVER_SVG(image->format))// && filename)
    {
        msIO_fprintf(image->img.svg->stream, "</svg>\n"); 
        fclose(image->img.svg->stream);

        //TODO : what about filename
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
        else
        {
            stream = fopen(filename, "wb");
            if (!stream)
            {
                msSetError(MS_IOERR, "Unable to open file %s for writing",
                           "msSaveImageSVG()", filename);
                return MS_FAILURE;
            }
            fp = fopen(image->img.svg->filename, "rb" );
            if( fp == NULL )
            {
                msSetError( MS_MISCERR, 
                            "Failed to open %s for streaming to stdout.",
                            "msSaveImageSVG()", image->img.svg->filename);
                return MS_FAILURE;
            }

            while( (bytes_read = fread(block, 1, sizeof(block), fp)) > 0 )
              msIO_fwrite( block, 1, bytes_read, stream );

            fclose( fp );
            fclose(stream);
        }
        return MS_SUCCESS;
    }
    return MS_FAILURE;
   
}

int msDrawRasterLayerSVG(mapObj *map, layerObj *layer, imageObj *image)
{
    outputFormatObj *format = NULL;
    imageObj    *imagetmp = NULL;
    char        *pszTmpfile = NULL;
    char        *pszURL = NULL;

    if (!image || !map || !MS_DRIVER_SVG(image->format) || 
        image->width <= 0 ||image->height <= 0)
      return MS_FAILURE;

    if (!map->web.imagepath || !map->web.imageurl)
    {
        msSetError(MS_MISCERR, "web image path and imageurl need to be set.",
                   "msDrawRasterLayerSVG");
        return MS_FAILURE;
    }
/* -------------------------------------------------------------------- */
/*      create a temprary GD image and render in it.                    */
/* -------------------------------------------------------------------- */
    format = msCreateDefaultOutputFormat( NULL, "GD/PNG24" );
    if (!format)
      format = msCreateDefaultOutputFormat( NULL, "GD/JPEG" );

    if (!format)
    {
        msSetError(MS_MISCERR, "Unable to crete temporary GD image format (PNG or JPEG)",
                   "msDrawRasterLayerSVG");
        return MS_FAILURE;
    }

    imagetmp = msImageCreate(image->width, image->height, format, 
                               NULL, NULL, map );

    //TODO : msDrawRasterLayerLow returns 0 (ms_success) in some cases
    // without drawing anything (ex : if it does not fit in scale)
    if (msDrawRasterLayerLow(map, layer, imagetmp) != MS_FAILURE)
    {
        pszTmpfile = msTmpFile(map->mappath,map->web.imagepath,format->extension);
        if (!pszTmpfile)
        {
            msSetError(MS_IOERR, "Failed to create temporary svg file.",
                    "msImageCreateSVG()" );
            return MS_FAILURE;
        }
        msSaveImageGD(imagetmp->img.gd, pszTmpfile, format);
        pszURL = (char *)malloc(sizeof(char)*(strlen(map->web.imageurl)+
                                              strlen(pszTmpfile)+
                                              strlen(format->extension)+2));
        sprintf(pszURL, "%s%s.%s", map->web.imageurl, msGetBasename(pszTmpfile), 
                format->extension);
        msIO_fprintf(image->img.svg->stream, "\n<image xlink:href=\"%s\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" />\n", pszURL, map->width, map->height);

         msFreeImage(imagetmp);
         msFree(tmpfile);
         msFree(pszURL);

         //TODO : should we keep track of the file and delete it ?
         return MS_SUCCESS;
    }

    return MS_FAILURE;
         
 
}

/************************************************************************/
/*                              msFreeImageSVG                          */
/*                                                                      */
/*      TODO                                                            */
/************************************************************************/
MS_DLL_EXPORT void msFreeImageSVG(imageObj *image)
{
}


#endif

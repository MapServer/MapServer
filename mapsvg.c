/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  SVG output
 * Author:   Y. Assefa, DM Solutions Group (assefa@dmsolutions.ca)
 *
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


#include <stdarg.h>

#include <assert.h>
#ifdef USE_ZLIB
#include "zlib.h"
#endif
#include "mapserver.h"
#include "mapcopy.h"

MS_CVSID("$Id$")


static int msIO_fprintfgz(FILE *fp, int bCompressed, const char *format, ... )

{
    va_list args;
    int     return_val;
    char workBuf[8000];

    va_start( args, format );
#if defined(HAVE_VSNPRINTF)
    return_val = vsnprintf( workBuf, sizeof(workBuf), format, args );
#else
    return_val = vsprintf( workBuf, format, args);
#endif

    va_end( args );

    if( return_val < 0 || return_val >= sizeof(workBuf) )
        return -1;

    if (bCompressed)
    {

#ifdef USE_ZLIB
        return gzwrite( fp, workBuf, return_val);
#else
        return fwrite( workBuf, 1, return_val, fp );
#endif
    }
    else
      return fwrite( workBuf, 1, return_val, fp );
    
}

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
    image->resolution = map->resolution;
    image->resolutionfactor = map->resolution/map->defresolution;

    if (imagepath)
      image->imagepath = strdup(imagepath);

    if (imageurl)
        image->imageurl = strdup(imageurl);

    image->img.svg = (SVGObj *)malloc(sizeof(SVGObj));
    image->img.svg->filename = NULL;
    image->img.svg->map = map;
    image->img.svg->streamclosed = 0;

    if (! map->web.imagepath)
    {
        msSetError(MS_MISCERR, "web image path need to be set.",
                   "msImageCreateSVG");
        return NULL;
    }

#ifdef USE_ZLIB
    if (strcasecmp(msGetOutputFormatOption(image->format, 
                                           "COMPRESSED_OUTPUT",""),
                   "TRUE") == 0)
    {
        if( map != NULL && map->web.imagepath != NULL )
          image->img.svg->filename = msTmpFile(map->mappath,
                                                 map->web.imagepath,"svgz");
    }
    else
      if( map != NULL && map->web.imagepath != NULL )
          image->img.svg->filename = msTmpFile(map->mappath,
                                               map->web.imagepath,"svg"); 
#else
    if( map != NULL && map->web.imagepath != NULL )
          image->img.svg->filename = msTmpFile(map->mappath,
                                               map->web.imagepath,"svg"); 
#endif
    
    
    if (!image->img.svg->filename)
    {
        msSetError(MS_IOERR, "Failed to create temporary svg file.",
                    "msImageCreateSVG()" );
        return NULL;
    }

    if (strcasecmp(msGetOutputFormatOption(image->format, "COMPRESSED_OUTPUT",""),
                   "TRUE") == 0)
    {
#ifdef USE_ZLIB
        image->img.svg->stream = gzopen(image->img.svg->filename, "wb" );
        image->img.svg->compressed = 1;
#else
        image->img.svg->stream = fopen(image->img.svg->filename, "w" );
        image->img.svg->compressed = 0;
#endif
    }
    else
    {
        image->img.svg->stream = fopen(image->img.svg->filename, "w" );
        image->img.svg->compressed = 0;
    }
    

    if (!image->img.svg->stream)
    {
        msSetError(MS_IOERR, "Failed to open temporary svg file.",
                    "msImageCreateSVG()" );
        return NULL;
    }
    
    /* initialize it with the svg header */

    msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    /* TODO : seems to have a problem of dtd validation using IE (http://www.24help.info/printthread.php?t=52147) */
    /* msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"); */
    /* msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11-flat.dtd\">\n"); */


/* -------------------------------------------------------------------- */
/*      Special output for goSVG support.                               */
/* -------------------------------------------------------------------- */
    if (strcasecmp(msGetOutputFormatOption(image->format, "GOSVG",""),
                   "TRUE") == 0)
    {
        /* transformation from geographical coordinates to pixel coordinates */
        a = width/(map->extent.maxx-map->extent.minx);
        b = 0;
        c = 0;
        d = -height/(map->extent.maxy - map->extent.miny);
        e = -width/(map->extent.maxx-map->extent.minx) *map->extent.minx;
        f = height/(map->extent.maxy-map->extent.miny)*map->extent.maxy;

        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<svg version=\"1.1\" width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:au=\"http://www.svgmobile.jp/2004/kddip\" au:boundingBox=\"0 0 %d %d\">\n",  width, height, width, height);
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<title>%s</title>\n", map->name);
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<metadata>\n");
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed, "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n");
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "xmlns:crs=\"http://www.ogc.org/crs\" xmlns:svg=\"http://wwww.w3.org/2000/svg\">\n");
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<rdf:Description>\n");

        if (map->units == MS_DD && 
            (strcasecmp(msGetOutputFormatOption(image->format, 
                                               "GOSVG_LATLONG_TRANSFORMATION",""), 
                       "TRUE") == 0))
        {
            
            double plat = ((map->extent.maxy-map->extent.miny)/2) + map->extent.miny;
            double plon =(( map->extent.maxx-map->extent.minx)/2) + map->extent.minx;
            double wp = width;
            double hp = height;
            double r = ((map->extent.maxy-map->extent.miny)/2)*111320;/*1degree of long in meteres at latitude 0 */

            double circle=40000000;
            double res=hp;
            double aspect=width/height;/*hp/wp;*/
            double aspect_d;
            double wd;
            double hd;
            double dx;
            double dy;
            double x1;
            double y1;
            double x2;
            double y2;
            double a1;
            double d1;
            double e1;
            double f1;

            aspect_d = (hp)/(wp/cos(plat*(MS_PI/180.0)));
	    
	    if (aspect <= 1) {
		hd=(360*2*r)/circle;
		wd=hd/aspect_d;
	    } else {
		wd=(360*2*r)/(cos(plat*(MS_PI/180.0))*circle);
		hd=aspect_d*wd;
	    }
	    
	    x1=plon-wd/2;
	    y1=plat-hd/2;
	    x2=plon+wd/2;
	    y2=plat+hd/2;
	    
	    dy=y2-y1;
	    dx=x2-x1;
	    d1=-(res/dy);
	    f1=-d1*y1+hp;
	    
	    a1=cos(((y1+y2)/2)*(MS_PI/180.0))*(res/dy);
	    e1= -a1*x1;

            msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "\n<!-- transform parameters using catersian transformation : svg:transform=\"matrix(%f,%f,%f,%f,%f,%f)\"  -->\n", a, b, c, d, e, f);
            msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<crs:CoordinateReferenceSystem svg:transform=\"matrix(%f,%f,%f,%f,%f,%f)\"\n", a1, b, c, d1, e1, f1);
            

        }
        else
          msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<crs:CoordinateReferenceSystem svg:transform=\"matrix(%f,%f,%f,%f,%f,%f)\"\n", a, b, c, d, e, f);

        
        /* get epsg code: TODO : test what if espg code is not set ? */
        if (map->projection.numargs > 0 && 
            (value = strstr(map->projection.args[0], "init=epsg:")) != NULL && 
            strlen(value) < 20) 
        {
            sprintf(epsgCode, "%s", value+10);
            msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "rdf:resource=\"http://www.opengis.net/gml/srs/epsg.xml#%s\"/>\n",epsgCode);
        }
        else
          msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed, "/> \n");

        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "</rdf:Description>\n</rdf:RDF>\n"); 
        
        nZoomInTH = 70;
        nZoomOutTH = 100;
        nScrollTH = 10;
        
        nZoomInTH = atoi(msGetOutputFormatOption(image->format, "GOSVG_ZoomInTH","70"));
        nZoomOutTH = atoi(msGetOutputFormatOption(image->format, "GOSVG_ZoomOutTH","100"));
        nScrollTH = atoi(msGetOutputFormatOption(image->format, "GOSVG_ScrollTH","10"));

        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<au:lbs protocol=\"maprequest\">\n");
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  
                     "<au:zoomin th=\"%d\" xlink:href=\".\"/>\n", nZoomInTH);
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  
                     "<au:zoomout th=\"%d\" xlink:href=\".\"/>\n", nZoomOutTH);
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  
                     "<au:scroll th=\"%d\" xlink:href=\".\"/>\n", nScrollTH);
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "</au:lbs>\n");

        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed, "</metadata>\n");
    }
    else
    {
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "<svg version=\"1.1\" width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n",  width, height);
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
static void imagePolyline(FILE *fp, int bCompressed, shapeObj *p, 
                          colorObj *color, double size,
                          int symbolstylelength, int *symbolstyle, int bFullRes)
{
    int i, j, k;
    char *pszDashArray = NULL;
    char szTmp[100];

    if (!fp || !p || !color || size <0 )
      return;

    /* the default size is 1. */
    if (size <= 0)
      size = 1;


    for (i = 0; i < p->numlines; i++)
    {
        
        if (symbolstylelength > 0 && symbolstyle)
        {
            for (k=0; k<symbolstylelength; k++)
            {
                if (k < symbolstylelength-1)
                  sprintf(szTmp, "%d, ", symbolstyle[k]);
                else
                  sprintf(szTmp, "%d", symbolstyle[k]);
                
                pszDashArray = msStringConcatenate(pszDashArray, szTmp);
            }
            
            msIO_fprintfgz(fp, bCompressed, "<polyline fill=\"none\" stroke=\"#%02x%02x%02x\" stroke-width=\"%0.2f\" stroke-dasharray=\"%s\" points=\"",color->red, color->green, 
                           color->blue,size, pszDashArray);
        }
        else
          msIO_fprintfgz(fp, bCompressed,"<polyline fill=\"none\" stroke=\"#%02x%02x%02x\" stroke-width=\"%0.2f\" points=\"",color->red, color->green, color->blue,size);
        
        if (bFullRes) 
          msIO_fprintfgz(fp, bCompressed, "%.2f,%.2f", p->line[i].point[0].x, p->line[i].point[0].y); 
        else
          msIO_fprintfgz(fp, bCompressed, "%d,%d", (int)p->line[i].point[0].x, (int)p->line[i].point[0].y);
        
        for(j=1; j<p->line[i].numpoints; j++)
        {
            if (bFullRes)
              msIO_fprintfgz(fp, bCompressed, " %.2f,%.2f", p->line[i].point[j].x, p->line[i].point[j].y); 
            else
              msIO_fprintfgz(fp, bCompressed, " %d,%d", (int)p->line[i].point[j].x, (int)p->line[i].point[j].y);
            
        }
        
        
        msIO_fprintfgz(fp, bCompressed,"\"/>\n");
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



    bFullRes = 0;
    if (strcasecmp(msGetOutputFormatOption(image->format, "FULL_RESOLUTION",""), 
                   "TRUE") == 0)
      bFullRes = 1;

    bUseGeoCoord= 0;
    
    /* if (strcasecmp(msGetOutputFormatOption(image->format, 
                                           "USE_GEOGRAPHICAL_COORDINATES",""),
                                           "TRUE") == 0) */
    /* bUseGeoCoord = 1;*/

    /* if it is full resolution and we want geographical output */
    /* just return, no transformations needed. */
     if (bFullRes && bUseGeoCoord)
      return;

    /* low resolution and pixel output */
    if (!bFullRes && !bUseGeoCoord)
    {
        msTransformShapeToPixel(shape, extent, cellsize);
        return;
    }

    /* full resolution, pixel output */
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
                    
                    /*
                    shape->line[i].point[j].x = MS_MAP2IMAGE_X(shape->line[i].point[j].x,
                                                               extent.minx, cellsize);
                    shape->line[i].point[j].y = MS_MAP2IMAGE_Y( shape->line[i].point[j].y,
                                                                extent.maxy,
                                                                cellsize);
                    */                                          
                    
                }
            }      
            return;
        }
    }

    /* low resolution, geographical output.  */
    /* Function comes from mapprimitive.c msTransformPixelToShape */
    /* TODO : adapat this function. For now just return fullres with geo coordinates */
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
    /* int styleDashed[100]; */
    int ox, oy;
    double size;
    double width;
    /* gdPoint points[MS_MAXVECTORPOINTS]; */
    symbolObj *symbol;
    int bFullRes;
    int symbol_pattern[MS_MAXPATTERNLENGTH], i;

/* -------------------------------------------------------------------- */
/*      if not SVG, return.                                             */
/* -------------------------------------------------------------------- */
    if (!image || !MS_DRIVER_SVG(image->format) )
        return;

    
    if(!p) return;
    if(p->numlines <= 0) return;

    bFullRes = 0;
    if (strcasecmp(msGetOutputFormatOption(image->format, "FULL_RESOLUTION",""), 
                   "TRUE") == 0)
      bFullRes = 1;
    if(style->size == -1) {
      size = (double)msSymbolGetDefaultSize( symbolset->symbol[style->symbol] );
    }
    else
      size = style->size;

    /* TODO: Don't get this modification, is it needed elsewhere? */
    if(size*scalefactor > style->maxsize*image->resolutionfactor) scalefactor = (float)style->maxsize*image->resolutionfactor/(float)size;
    if(size*scalefactor < style->minsize*image->resolutionfactor) scalefactor = (float)style->minsize*image->resolutionfactor/(float)size;
    size = size*scalefactor;
    size = MS_MAX(size, style->minsize*image->resolutionfactor);
    size = MS_MIN(size, style->maxsize*image->resolutionfactor);

    width = style->width*scalefactor;
    width = MS_MAX(width, style->minwidth*image->resolutionfactor);
    width = MS_MIN(width, style->maxwidth*image->resolutionfactor);

    if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; /* no such symbol, 0 is OK */

     if (!MS_VALID_COLOR( style->color))
      return;
    
    if(size < 0) return; /* size too small */

    /* TODO : do we need offset ?? */
    ox = MS_NINT(style->offsetx*scalefactor);
    oy = (style->offsety == -99) ? -99 : (int)(style->offsety*scalefactor);

    symbol = symbolset->symbol[style->symbol];

    /* scale the symbol pattern */
    if (symbol->patternlength > 0)
    {
        for (i=0; i<symbol->patternlength; i++)
        {
	  symbol_pattern[i] = MS_NINT(symbol->pattern[i]*scalefactor);
        }
    }

    /* TODO : all lines are draw as filled lines.  */
    
    /* if default symbol 0 is used, use the style's width parameter */
    /* else use the symbol size. */
    /* TODO : size needs to be double ? */
    if(style->symbol == 0) 
      imagePolyline(image->img.svg->stream, image->img.svg->compressed, 
                    p, &style->color, width,
                    symbol->patternlength,  symbol_pattern, bFullRes);
    else
      imagePolyline(image->img.svg->stream, image->img.svg->compressed, 
                    p, &style->color, size,
                    symbol->patternlength,  symbol_pattern, bFullRes);
        return;
    

}

/************************************************************************/
/*                           msImageStartLayerSVG                       */
/************************************************************************/
void msImageStartLayerSVG(mapObj *map, layerObj *layer, imageObj *image)
{
    if (image && MS_DRIVER_SVG(image->format) && layer && map)/* && filename) */
    {
        if (strcasecmp(msGetOutputFormatOption(image->format, "GOSVG",""),
                       "TRUE") != 0)
          msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "\n<!-- START LAYER %s -->\n", layer->name); 
    }
}



static void imageFillPolygon(FILE *fp, int bCompressed, 
                             shapeObj *p, colorObj *psFillColor, 
                             colorObj *psOutlineColor, double size,
                             int symbolstylelength, int *symbolstyle, int bFullRes)
{
    int i, j, k,max;
    char *pszDashArray = NULL;
    char szTmp[100];

    if (!fp || !p || (psFillColor==NULL && psOutlineColor==NULL) || size <0 )
      return;

    /* the default size is 1. */
    if (size <= 0)
      size = 1;

    max=0;
    for (i = 0; i < p->numlines; i++)
    {
      if(p->line[i].numpoints > max)
        max=p->line[i].numpoints;
    }

    for (i = 0; i < p->numlines; i++)
    {
        if (i == 0)
        {
            pszDashArray = msStringConcatenate(pszDashArray,"");
            if (symbolstylelength > 0 && symbolstyle)
            {
                sprintf(szTmp, "stroke-dasharray=\"");
                pszDashArray = msStringConcatenate(pszDashArray, szTmp);
                for (k=0; k<symbolstylelength; k++)
                {
                    if (k < symbolstylelength-1)
                      sprintf(szTmp, "%d, ", symbolstyle[k]);
                    else
                      sprintf(szTmp, "%d\"", symbolstyle[k]);

                    pszDashArray = msStringConcatenate(pszDashArray, szTmp);
                }
            }
            if (max > 2)
            {
              if (psOutlineColor && psFillColor)
              /*msIO_fprintfgz(fp, bCompressed, "<polygon fill=\"#%02x%02x%02x\" stroke=\"#%02x%02x%02x\"  stroke-width=\"%d\" %s points=\"",
                           psFillColor->red, psFillColor->green, 
                           psFillColor->blue,
                           psOutlineColor->red, psOutlineColor->green, 
                           psOutlineColor->blue,
                           size, pszDashArray);*/
                msIO_fprintfgz(fp, bCompressed, "<path fill=\"#%02x%02x%02x\" stroke=\"#%02x%02x%02x\" stroke-width=\"%0.2f\" %s d=\"",
                           psFillColor->red, psFillColor->green, 
                           psFillColor->blue,
                           psOutlineColor->red, psOutlineColor->green, 
                           psOutlineColor->blue,
                           size, pszDashArray);
            
              else if (psOutlineColor)
              {
                /*msIO_fprintfgz(fp,  bCompressed, "<polygon  stroke=\"#%02x%02x%02x\"  stroke-width=\"%d\" %s points=\"",
                           psOutlineColor->red, psOutlineColor->green, 
                           psOutlineColor->blue,
                           size, pszDashArray);*/
                msIO_fprintfgz(fp,  bCompressed, "<path stroke=\"#%02x%02x%02x\" stroke-width=\"%0.2f\" %s style=\"fill:none\" d=\"",
                           psOutlineColor->red, psOutlineColor->green, 
                           psOutlineColor->blue,
                           size, pszDashArray);
              }
              else /* fill color valid  */
              {
                /*msIO_fprintfgz(fp,  bCompressed, "<polygon fill=\"#%02x%02x%02x\" points=\"",
                             psFillColor->red, psFillColor->green, 
                             psFillColor->blue);*/
                msIO_fprintfgz(fp,  bCompressed, "<path fill=\"#%02x%02x%02x\" d=\"",
                             psFillColor->red, psFillColor->green, 
                             psFillColor->blue);
                           
              }
            }
        }

        if(p->line[i].numpoints > 2)
        {
            if (bFullRes)
              msIO_fprintfgz(fp,  bCompressed, "M %.2f %.2f ", p->line[i].point[0].x, p->line[i].point[0].y);
            else
              msIO_fprintfgz(fp,  bCompressed, "M %d %d ", (int)p->line[i].point[0].x, (int)p->line[i].point[0].y);
        
            for(j=1; j<p->line[i].numpoints; j++)
            {
                if (bFullRes)
                  msIO_fprintfgz(fp,  bCompressed, "L %.2f %.2f ", p->line[i].point[j].x, p->line[i].point[j].y);
                else
                  msIO_fprintfgz(fp,  bCompressed, "L %d %d ", (int)p->line[i].point[j].x, (int)p->line[i].point[j].y);
            }

            /*msIO_fprintfgz(fp,  bCompressed, " %d,%d", (int)p->line[i].point[j].x, (int)p->line[i].point[j].y);*/
        
        }
        if (i == (p->numlines -1) && max > 2)
          msIO_fprintfgz(fp,  bCompressed,"z\"/>\n");
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
    double         size;
    int bFullRes = 0;
    int symbol_pattern[MS_MAXPATTERNLENGTH], i;

/* -------------------------------------------------------------------- */
/*      if not svg, return.                                             */
/* -------------------------------------------------------------------- */
    if (image == NULL || !MS_DRIVER_SVG(image->format) )
      return;
    
    if(p == NULL || p->numlines <= 0)
      return;

     bFullRes = 0;
    if (strcasecmp(msGetOutputFormatOption(image->format, "FULL_RESOLUTION",""), 
                   "TRUE") == 0)
      bFullRes = 1;

    symbol = symbolset->symbol[style->symbol];

    if(style->size == -1) 
    {
        size = (int)msSymbolGetDefaultSize( symbolset->symbol[style->symbol] );
        size = size*scalefactor;
    }
    else
      size = style->size*scalefactor;

    size = MS_MAX(size, style->minsize*image->resolutionfactor);
    size = MS_MIN(size, style->maxsize*image->resolutionfactor);

    /* scale the symbol pattern */
    if (symbol->patternlength > 0)
    {
        for (i=0; i<symbol->patternlength; i++)
        {
	  symbol_pattern[i] = MS_NINT(symbol->pattern[i]*scalefactor);
        }
    }

    if(style->symbol > symbolset->numsymbols || style->symbol < 0) /* no such symbol, 0 is OK */
        return;

    if(size < 0) /* size too small */
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

    imageFillPolygon(image->img.svg->stream, image->img.svg->compressed ,p, psFillColor, psOutlineColor, size,
                     symbol->patternlength,  symbol_pattern, bFullRes);

}

#define Tcl_UniChar int
#define TCL_UTF_MAX 3
/*comes from gd source code (ggdft.c).*/

#ifdef notdef /* disabled since not currently used */
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
#endif /* def notdef */

static void drawSVGText(FILE *fp, int bCompressed, int x, int y, 
                        char *string, double size, 
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
    /*    char *pszTmpStr = NULL;*/
    char szTmp[100];
    /*    char *next; */
    /*    int len, i, ch; */
    int nTokens = 0, i=0;
    char **tokens = NULL;

    pszFontStyleString = msStringConcatenate(pszFontStyleString, "");
    pszFontWeightString = msStringConcatenate(pszFontWeightString, "");
    pszAngleString = msStringConcatenate(pszAngleString, "");
    pszAngleAnchorString = msStringConcatenate(pszAngleAnchorString, "");

    if (pszFontStyle)
    {
        sprintf(szTmp, " font-style=\"%s\"", pszFontStyle);
        pszFontStyleString = msStringConcatenate(pszFontStyleString, szTmp);
    }

    if (pszFontWeight)
    {
        sprintf(szTmp, " font-weight=\"%s\"", pszFontWeight);
        pszFontWeightString = msStringConcatenate(pszFontWeightString, szTmp);
    }
 
    pszFillString =  msStringConcatenate(pszFillString, "");
    if (psColor)
    {
        if (MS_VALID_COLOR(*psColor))
          sprintf(szTmp, " fill=\"#%02x%02x%02x\"",
                  psColor->red, psColor->green  , psColor->blue);
        else
          sprintf(szTmp," fill=\"none\"");

        pszFillString = msStringConcatenate(pszFillString, szTmp);
    }

    pszStrokeString =  msStringConcatenate(pszStrokeString, "");
    if (psOutlineColor && MS_VALID_COLOR(*psOutlineColor))
    {
        sprintf(szTmp, " stroke=\"#%02x%02x%02x\" stroke-width=\"0.5\"",
                psOutlineColor->red, psOutlineColor->green  , psOutlineColor->blue);
        pszStrokeString = msStringConcatenate(pszStrokeString, szTmp);
    }

    /* angle */
    if (dfAngle > 0.0)
    {
        sprintf(szTmp, " transform=\"rotate(%f %d %d)\"",
                -dfAngle, x, y);
        pszAngleString = msStringConcatenate(pszAngleString, szTmp);
    }

    /*anchor positions are alreay taken into account when calculationg the
      label position. Do not write them here*/
    /* anchor point */
    

    pszFinalString = string;

    /*check for line breaks and produce multi lines if needed*/
    nTokens = 0;
    tokens = msStringSplit(pszFinalString, '\n', &nTokens);
    if (tokens && nTokens > 1)
    {
        /*+1 postion the text better. More inline with what we see froma raster(agg) output*/
        msIO_fprintfgz(fp, bCompressed,"<text x=\"%d\" y=\"%d\" font-family=\"%s\" font-size=\"%dpt\"%s%s%s%s%s>\n",
                       x, (y-((int)(size+1))), pszFontFamilily, (int)size, 
                     pszFontStyleString, pszFontWeightString, 
                       pszFillString, pszStrokeString, pszAngleString);
        for (i=0; i<nTokens; i++)
        {
            msIO_fprintfgz(fp, bCompressed,"<tspan x=\"%d\" dy=\"%dpt\">%s</tspan>\n",x,(int)size,tokens[i]);
        }       
        msIO_fprintfgz(fp, bCompressed, "</text>\n");
    }
    else
      /* TODO : font size set to unit pt */
      msIO_fprintfgz(fp, bCompressed,"<text x=\"%d\" y=\"%d\" font-family=\"%s\" font-size=\"%dpt\"%s%s%s%s%s>%s</text>\n" ,
                     x, y, pszFontFamilily, (int)size, 
                     pszFontStyleString, pszFontWeightString, 
                     pszFillString, pszStrokeString, pszAngleString, 
                     pszFinalString);

      
    if (tokens && nTokens > 0)
      msFreeCharArray(tokens, nTokens);
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
    { /* label encoding is set. expect UTF-8 as input */
        bEncoding = 1;
    }

    /* TODO : not transform it to integer is layer transform is false */
    x = MS_NINT(labelPnt.x);
    y = MS_NINT(labelPnt.y);


#ifndef USE_GD_FT
    msSetError(MS_TTFERR, "TrueType font support is not available.", "msDrawTextSVG()");
    /* if(label->encoding != NULL) msFree(string); */
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

        /* TODO : check support for all parameters  */
        /* color, outlinecolor, shadowcolor, backgroundcolor, backgroundshadowcolor */
        /* position, offset, angle, buffer, antialias, wrap, encoding */

        size = label->size*scalefactor;
        size = MS_MAX(size, label->minsize*image->resolutionfactor);
        size = MS_MIN(size, label->maxsize*image->resolutionfactor);

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
        /* parse font string :  */
        aszFontsParts = msStringSplit(label->font, '_', &nFontParts);
 
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

        drawSVGText(image->img.svg->stream, image->img.svg->compressed, x, y, 
                    string, size, 
                    &sColor, &sOutlineColor,
                    pszFontFamily,  pszFontStyle, pszFontWeight,
                    label->position, label->angle, bEncoding);

        /* msFreeCharArray(aszFontsParts, nFontParts); */

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
  int i, l, priority;
  rectObj r;
  
  labelCacheMemberObj *cachePtr=NULL;
  layerObj *layerPtr=NULL;
  labelObj *labelPtr=NULL;

  shapeObj billboard;
  lineObj billboard_line;
  pointObj billboard_points[5];

  int marker_width, marker_height, label_offset_x, label_offset_y;
  int marker_offset_x, marker_offset_y;
  rectObj marker_rect;
  int label_mindistance, label_buffer;

  label_mindistance=-1;
  label_buffer=0;

/* -------------------------------------------------------------------- */
/*      if not svg or invaid inputs, return.                            */
/* -------------------------------------------------------------------- */
    if (!image || !map || !MS_DRIVER_SVG(image->format) )
      return (0);

    billboard.numlines = 1;
    billboard.line = &billboard_line;
    billboard.line->numpoints = 5;
    billboard.line->point = billboard_points;
    
  for(priority=MS_MAX_LABEL_PRIORITY-1; priority>=0; priority--) {
   labelCacheSlotObj *cacheslot;
   cacheslot = &(map->labelcache.slots[priority]);

   for(l=cacheslot->numlabels-1; l>=0; l--) {

    cachePtr = &(cacheslot->labels[l]); /* point to right spot in the label cache */

    layerPtr = (GET_LAYER(map, cachePtr->layerindex)); /* set a couple of other pointers, avoids nasty references */
    labelPtr = &(cachePtr->label);

    if(!cachePtr->text || strlen(cachePtr->text) == 0)
      continue; /* not an error, just don't want to do anything */

    if(msGetLabelSize(image, cachePtr->text, labelPtr, &r, &(map->fontset), layerPtr->scalefactor, MS_TRUE,NULL) == -1)
      return(-1);

    label_offset_x = MS_NINT(labelPtr->offsetx*layerPtr->scalefactor);
    label_offset_y = MS_NINT(labelPtr->offsety*layerPtr->scalefactor);
    label_buffer = MS_NINT(labelPtr->buffer*image->resolutionfactor);
    label_mindistance = MS_NINT(labelPtr->mindistance*image->resolutionfactor);
    
    if(labelPtr->autominfeaturesize && (cachePtr->featuresize != -1) && ((r.maxx-r.minx) > cachePtr->featuresize))
      continue; /* label too large relative to the feature */

    marker_offset_x = marker_offset_y = 0; /* assume no marker */
    if((layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) || layerPtr->type == MS_LAYER_POINT) { /* there *is* a marker       */

      /* TO DO: at the moment only checks the bottom style, perhaps should check all of them */
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
              int positions[MS_POSITIONS_LENGTH], npositions=0;

              /*
	      ** If the ANNOTATION has an associated marker then the position is handled like a point regardless of underlying shape type. (#2993)
              **   (Note: might be able to re-order this for more speed.)
              */
              if(layerPtr->type == MS_LAYER_POLYGON || (layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->shapetype == MS_SHAPE_POLYGON && cachePtr->numstyles == 0)) {
		positions[0]=MS_CC; positions[1]=MS_UC; positions[2]=MS_LC; positions[3]=MS_CL; positions[4]=MS_CR;
                npositions = 5;
              } else if(layerPtr->type == MS_LAYER_LINE || (layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->shapetype == MS_SHAPE_LINE && cachePtr->numstyles == 0)) {
                positions[0]=MS_UC; positions[1]=MS_LC; positions[2]=MS_CC;
                npositions = 3;
              } else {
                positions[0]=MS_UL; positions[1]=MS_LR; positions[2]=MS_UR; positions[3]=MS_LL; positions[4]=MS_CR; positions[5]=MS_CL; positions[6]=MS_UC; positions[7]=MS_LC;
                npositions = 8;
              }

              for(i=0; i<npositions; i++) {
                msFreeShape(cachePtr->poly);
                cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

                p = get_metrics(&(cachePtr->point), positions[i], r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, label_buffer, cachePtr->poly);
                if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
                  msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon */

                /* Compare against image bounds, rendered labels and markers (sets cachePtr->status) */
                msTestLabelCacheCollisions(&(map->labelcache), labelPtr, image->width, image->height, label_buffer, cachePtr, priority, l, label_mindistance, (r.maxx-r.minx));

                if(cachePtr->status) /* found a suitable place for this label */ {
                  if(MS_VALID_COLOR(labelPtr->backgroundcolor))
                    get_metrics_line(&(cachePtr->point), positions[i], r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, 1, billboard.line);
                  break; /* ...out of position loop */
                }

              } /* next position */

              if(labelPtr->force) {

                  /*billboard wasn't initialized if no non-coliding position was found*/
                  if(!cachePtr->status && MS_VALID_COLOR(labelPtr->backgroundcolor))
                      get_metrics_line(&(cachePtr->point), positions[npositions-1], r,
                              (marker_offset_x + label_offset_x),
                              (marker_offset_y + label_offset_y),
                              labelPtr->angle, 1, billboard.line);

                  cachePtr->status = MS_TRUE; /* draw in spite of collisions based on last position, need a *best* position */
              }

            } else {
      cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

      if(labelPtr->position == MS_CC) { /* don't need the marker_offset */
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, label_offset_x, label_offset_y, labelPtr->angle, label_buffer, cachePtr->poly);
        if(MS_VALID_COLOR(labelPtr->backgroundcolor))
                  get_metrics_line(&(cachePtr->point), labelPtr->position, r, label_offset_x, label_offset_y, labelPtr->angle, 1, billboard.line);
      }
      else {
        p = get_metrics(&(cachePtr->point), labelPtr->position, r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, label_buffer, cachePtr->poly);
        if(MS_VALID_COLOR(labelPtr->backgroundcolor))
                  get_metrics_line(&(cachePtr->point), labelPtr->position, r, (marker_offset_x + label_offset_x), (marker_offset_y + label_offset_y), labelPtr->angle, 1, billboard.line);
      }
        
      if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0)
	msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon, part of overlap tests */

      if(!labelPtr->force) { /* no need to check anything else */

        /* Compare against image bounds, rendered labels and markers (sets cachePtr->status) */
        msTestLabelCacheCollisions(&(map->labelcache), labelPtr, 
                                   map->width, map->height, 
                                   label_buffer, cachePtr, priority, l, label_mindistance, (r.maxx-r.minx));
      }
    } /* end position if-then-else */

    /* imagePolyline(img, cachePtr->poly, 1, 0, 0); */

    if(!cachePtr->status)
      continue; /* next label */

    if(layerPtr->type == MS_LAYER_ANNOTATION && cachePtr->numstyles > 0) { /* need to draw a marker */
      for(i=0; i<cachePtr->numstyles; i++)
        msDrawMarkerSymbolSVG(&map->symbolset, image, &(cachePtr->point), &(cachePtr->styles[i]), layerPtr->scalefactor);
    }

    
    /*draw the background, only if we're not using FOLLOW labels*/
    if(!cachePtr->labelpath && MS_VALID_COLOR(labelPtr->backgroundcolor)) {
        styleObj style;

        initStyle(&style);                    
        if(MS_VALID_COLOR(labelPtr->backgroundshadowcolor)) {
            MS_COPYCOLOR(&(style.color),&(labelPtr->backgroundshadowcolor));
            style.offsetx = labelPtr->backgroundshadowsizex*image->resolutionfactor;
            style.offsety = labelPtr->backgroundshadowsizey*image->resolutionfactor;
            msDrawShadeSymbol(&map->symbolset,image,&billboard,&style,1);
            style.offsetx = 0;
            style.offsety = 0;
        }
        MS_COPYCOLOR(&(style.color), &(labelPtr->backgroundcolor));
        msDrawShadeSymbol(&map->symbolset, image, &billboard, &style, 1);
    }
    /* if(MS_VALID_COLOR(labelPtr->backgroundcolor)) billboardGD(img, cachePtr->poly, labelPtr); */
    msDrawTextSVG(image, p, cachePtr->text, labelPtr, &(map->fontset), layerPtr->scalefactor); /* actually draw the label */

   } /* next label */
  } /* next priority */

  
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
    char szTmp[100];
    symbolObj *symbol=NULL;
    double size,d;
    double width;
    int x,y,rx,ry;
    char *pszFill = NULL, *pszStroke=NULL;
    int bFillSetToNone = 0;
    int i, j, k;
    pointObj mPoints[MS_MAXVECTORPOINTS];
    pointObj oldpnt,newpnt;
    int offset_x, offset_y;
    char *pszFontFamily = NULL, *pszFontStyle=NULL, *pszFontWeight=NULL;
    char **aszFontsParts = NULL;
    int nFontParts= 0;
    char *font=NULL;
    rectObj rect;

/* -------------------------------------------------------------------- */
/*      if not svg or invaid inputs, return.                            */
/* -------------------------------------------------------------------- */
    if (!image || !p || !style || !MS_DRIVER_SVG(image->format) )
      return;
   
    if(!p) return;

    symbol = symbolset->symbol[style->symbol];

 
    if(style->size == -1) {
    size = msSymbolGetDefaultSize( symbolset->symbol[style->symbol] );
    size = MS_NINT(size*scalefactor);
    } else
      size = MS_NINT(style->size*scalefactor);
    size = MS_MAX(size, style->minsize*image->resolutionfactor);
    size = MS_MIN(size, style->maxsize*image->resolutionfactor);

    width = style->width*scalefactor;
    width = MS_MAX(width, style->minwidth*image->resolutionfactor);
    width = MS_MIN(width, style->maxwidth*image->resolutionfactor);

    if(style->symbol > symbolset->numsymbols || style->symbol < 0) return; /* no such symbol, 0 is OK */
    
    if (!MS_VALID_COLOR(style->color) && !MS_VALID_COLOR(style->outlinecolor) && 
        symbol->type != MS_SYMBOL_PIXMAP)
      return;

     if(size < 1) return; /* size too small */

     if(style->symbol == 0 )/*&& fc >= 0) {*/ /* simply draw a single pixel of the specified color */
     {
         /*gdImageSetPixel(img, (int)(p->x + ox), (int)(p->y + oy), fc);*/
         return;
     }

      switch(symbol->type) 
      { 
          case(MS_SYMBOL_TRUETYPE): 
#ifdef USE_GD_FT
            font = msLookupHashTable(&(symbolset->fontset->fonts), symbol->font);
            if(!font) return;

            if(msGetCharacterSize(symbol->character, (int)size, font, &rect) 
               != MS_SUCCESS) 
              return;

            x = (int)(p->x +  style->offsetx - (rect.maxx - rect.minx)/2 - rect.minx);
            y = (int)(p->y + style->offsety - rect.maxy + (rect.maxy - rect.miny)/2);

/* -------------------------------------------------------------------- */
/*      parse font string :                                             */
/*      There are 3 parts to the name separated with - (example         */
/*      arial-italic-bold):                                             */
/*        - font-family,                                                */
/*        - font-style (italic, oblique, nomal)                         */
/*        - font-weight (normal | bold | bolder | lighter | 100 | 200 ..*/
/* -------------------------------------------------------------------- */
            /* parse font string :  */
            aszFontsParts = msStringSplit(symbol->font, '_', &nFontParts);
 
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

            drawSVGText(image->img.svg->stream, image->img.svg->compressed, 
                        x, y, symbol->character, size, 
                        &style->color, &style->outlinecolor,
                        pszFontFamily,  pszFontStyle, pszFontWeight,
                        -1, -1,0);
            /* msFreeCharArray(aszFontsParts, nFontParts); */
           
#endif            
            break;

          case(MS_SYMBOL_PIXMAP):
            break;
            
          case(MS_SYMBOL_ELLIPSE):
            d = size/symbol->sizey;
            rx = MS_NINT((symbol->sizex*d)/2);
            ry = MS_NINT((symbol->sizey*d)/2);
            
            x = MS_NINT(p->x  + style->offsetx);
            y = MS_NINT(p->y  + style->offsety);

            /*TODO : style->angle */
            pszFill = msStringConcatenate(pszFill,"");
            if (MS_VALID_COLOR(style->color) && symbol->filled)
            {
                sprintf(szTmp, "fill=\"#%02x%02x%02x\"",style->color.red, 
                        style->color.green,
                        style->color.blue);
                pszFill = msStringConcatenate(pszFill, szTmp);
            }
            else
            {
                pszFill = msStringConcatenate(pszFill,"fill=\"none\"");
                bFillSetToNone =1;
            }
            pszStroke = msStringConcatenate(pszStroke, "");
            if (MS_VALID_COLOR(style->outlinecolor))
            {
                sprintf(szTmp, "stroke=\"#%02x%02x%02x\"",style->outlinecolor.red, 
                        style->outlinecolor.green,
                        style->outlinecolor.blue);
                pszStroke = msStringConcatenate(pszStroke, szTmp);
            } 
            else if (bFillSetToNone)
            {
                /*if the fill color is not setc (or the symbol is not filled) and 
                the outline color is not set, set the stroke to black. 
                This is the way the gd outputs reacts to this case */
                sprintf(szTmp, "stroke=\"#%02x%02x%02x\"",0,0,0);
                pszStroke = msStringConcatenate(pszStroke, szTmp);
            }
            

            msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,
                           "<ellipse cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" %s %s />\n", x, y, rx, ry, pszFill, pszStroke);
            msFree(pszFill);
            msFree(pszStroke);
            
            break;

          case(MS_SYMBOL_VECTOR):
            d = size/symbol->sizey;
            offset_x = MS_NINT(p->x - d*.5*symbol->sizex +  style->offsetx);
            offset_y = MS_NINT(p->y - d*.5*symbol->sizey +  style->offsety);

            pszFill = msStringConcatenate(pszFill,"");
            if (MS_VALID_COLOR(style->color) && symbol->filled)
            {
                sprintf(szTmp, "fill=\"#%02x%02x%02x\"",
                        style->color.red, 
                        style->color.green,
                        style->color.blue);
                pszFill = msStringConcatenate(pszFill, szTmp);
            }
            else
            {
                bFillSetToNone = 1;
                pszFill = msStringConcatenate(pszFill,"fill=\"none\"");
            }
            pszStroke = msStringConcatenate(pszStroke, "");
            if (MS_VALID_COLOR(style->outlinecolor))
            {
                sprintf(szTmp, "stroke=\"#%02x%02x%02x\"",
                        style->outlinecolor.red, 
                        style->outlinecolor.green,
                        style->outlinecolor.blue);
                pszStroke = msStringConcatenate(pszStroke, szTmp);
            }
            else if (bFillSetToNone)
            {
                /*if the fill color is not setc (or the symbol is not filled) and 
                the outline color is not set, set the stroke to black. 
                This is the way the gd outputs reacts to this case */
                sprintf(szTmp, "stroke=\"#%02x%02x%02x\"",0,0,0);
                pszStroke = msStringConcatenate(pszStroke, szTmp);
            }
            if (width <= 0)
              width = 1;

            if(symbol->filled) 
            { /* if filled */
                k = 0; /* point counter */
                for(j=0;j < symbol->numpoints;j++) 
                {
                    if((symbol->points[j].x < 0) && (symbol->points[j].y < 0)) 
                    { /* new polygon (PENUP) */
                        if(k>2) 
                        {
                            
                            msIO_fprintfgz(image->img.svg->stream, 
                                           image->img.svg->compressed,  
                                         "<polygon %s %s stroke-width=\"%0.2f\" points=\"", 
                                         pszFill, pszStroke, width);
                            
                            for (i=0; i<k;i++)
                            {
                                 msIO_fprintfgz(image->img.svg->stream, 
                                                image->img.svg->compressed,  
                                                " %d,%d", 
                                                (int)mPoints[i].x, 
                                                (int)mPoints[i].y);
                            }
                            msIO_fprintfgz(image->img.svg->stream, 
                                           image->img.svg->compressed,  
                                           "\"/>\n");
                        }
                        k = 0; /* reset point counter */
                    } 
                    else 
                    {
                        mPoints[k].x = MS_NINT(d*symbol->points[j].x + offset_x);
                        mPoints[k].y = MS_NINT(d*symbol->points[j].y + offset_y); 
                        k++;
                    }
                }

                msIO_fprintfgz(image->img.svg->stream, 
                               image->img.svg->compressed,  
                             "<polygon %s %s stroke-width=\"%0.2f\" points=\"", 
                             pszFill, pszStroke, width);
                            
                for (i=0; i<k;i++)
                {
                    msIO_fprintfgz(image->img.svg->stream, 
                                   image->img.svg->compressed, " %d,%d", 
                                 (int)mPoints[i].x, (int)mPoints[i].y);
                }
                msIO_fprintfgz(image->img.svg->stream, 
                               image->img.svg->compressed,  "\"/>\n");
                
            }
            else
            { /* NOT filled */     
                
                 

                oldpnt.x = MS_NINT(d*symbol->points[0].x + offset_x); /* convert first point in marker s */
                oldpnt.y = MS_NINT(d*symbol->points[0].y + offset_y);
      
                for(j=1;j < symbol->numpoints;j++) { /* step through the marker s */
                    if((symbol->points[j].x < 0) && (symbol->points[j].y < 0)) {
                        oldpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
                        oldpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
                    } else {
                        if((symbol->points[j-1].x < 0) && (symbol->points[j-1].y < 0)) { /* Last point was PENUP, now a new beginning */
                            oldpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
                            oldpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
                        } else {
                            newpnt.x = MS_NINT(d*symbol->points[j].x + offset_x);
                            newpnt.y = MS_NINT(d*symbol->points[j].y + offset_y);
                            msIO_fprintfgz(image->img.svg->stream, 
                                           image->img.svg->compressed,  
                              "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" %s %s stroke-width=\"%0.2f\" stroke-linecap=\"round\"/>\n",
                                         (int)oldpnt.x, (int)oldpnt.y, (int)newpnt.x, (int)newpnt.y,
                                         pszFill, pszStroke, width);
                            oldpnt = newpnt;
                        }
                    }
                } /* end for loop */   

            }
            break;
           
          default:
            break;
      }  
}



/************************************************************************/
/*                            msSaveImagetoFpSVG                        */
/*                                                                      */
/*      Save SVG to file pointer. It is an utility function and is      */
/*      now just used from php_mapscript.c                              */
/************************************************************************/
MS_DLL_EXPORT int msSaveImagetoFpSVG(imageObj *image, FILE *stream)
{
    unsigned char block[4000];
    int bytes_read;
    FILE *fp = NULL;

    if (image && MS_DRIVER_SVG(image->format) && stream)
    {
        if (!image->img.svg->streamclosed)
        {
            msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "</svg>\n"); 
            if (image->img.svg->compressed)
            {
#ifdef USE_ZLIB
                gzclose(image->img.svg->stream);
#else
                fclose(image->img.svg->stream);
#endif
            }
            else
              fclose(image->img.svg->stream);
           
            image->img.svg->streamclosed = 1;
        }
        fp = fopen(image->img.svg->filename, "rb" );
        if( fp == NULL )
        {
            msSetError( MS_MISCERR, 
                        "Failed to open %s for streaming to stdout.",
                        "msSaveImagetoFpSVG()", image->img.svg->filename);
            return MS_FAILURE;
        }
            
        while( (bytes_read = fread(block, 1, sizeof(block), fp)) > 0 )
          msIO_fwrite( block, 1, bytes_read, stream );

        fclose( fp );

        return MS_SUCCESS;
    }	
    
    return MS_FAILURE;
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

    if (image && MS_DRIVER_SVG(image->format))/* && filename) */
    {
        if (!image->img.svg->streamclosed)
        {
            msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "</svg>\n"); 
            if (image->img.svg->compressed)
            {
#ifdef USE_ZLIB
                gzclose(image->img.svg->stream);
#else
                fclose(image->img.svg->stream);
#endif
            }
            else
              fclose(image->img.svg->stream);
            image->img.svg->streamclosed = 1;
        }
        
        if (!filename)
        {
            if (image->img.svg->compressed)
            {
                if( msIO_needBinaryStdout() == MS_FAILURE ) 
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
                            "Failed to open temporaray svg file %s",
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



/************************************************************************/
/*                           msDrawRasterLayerSVG                       */
/*                                                                      */
/*      Draw raster layers.                                             */
/************************************************************************/
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

    /* TODO : msDrawRasterLayerLow returns 0 (ms_success) in some cases */
    /* without drawing anything (ex : if it does not fit in scale) */
    if (msDrawRasterLayerLow(map, layer, imagetmp) != MS_FAILURE)
    {
        pszTmpfile = msTmpFile(map->mappath,map->web.imagepath,format->extension);
        if (!pszTmpfile)
        {
            msSetError(MS_IOERR, "Failed to create temporary svg file.",
                    "msImageCreateSVG()" );
            return MS_FAILURE;
        }
        msSaveImageGD(imagetmp, pszTmpfile, format);
        pszURL = (char *)malloc(sizeof(char)*(strlen(map->web.imageurl)+
                                              strlen(pszTmpfile)+
                                              strlen(format->extension)+2));
        sprintf(pszURL, "%s%s.%s", map->web.imageurl, msGetBasename(pszTmpfile), 
                format->extension);
        msIO_fprintfgz(image->img.svg->stream, image->img.svg->compressed,  "\n<image xlink:href=\"%s\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"/>\n", pszURL, map->width, map->height);

         msFreeImage(imagetmp);
         msFree(pszTmpfile);
         msFree(pszURL);

         /* TODO : should we keep track of the file and delete it ? */
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

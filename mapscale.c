/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Scale object rendering. 
 * Author:   Steve Lime and the MapServer team.
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

#include "mapserver.h"

MS_CVSID("$Id$")

#define VMARGIN 3 /* buffer around the scalebar */
#define HMARGIN 3
#define VSPACING .8 /* spacing (% of font height) between scalebar and text */
#define VSLOP 5 /* makes things fit a bit better vertically */

/*
** Match this with with unit enumerations is mapserver.h
*/
static char *unitText[9]={"in", "ft", "mi", "m", "km", "dd", "??", "??", "NM"}; //MS_PIXEL and MS_PERCENTAGE not used
double inchesPerUnit[9]={1, 12, 63360.0, 39.3701, 39370.1, 4374754, 1, 1, 72913.3858 };


static double roundInterval(double d)
{
  if(d<.001)
    return(MS_NINT(d*10000)/10000.0);
  if(d<.01)
    return(MS_NINT(d*1000)/1000.0);
  if(d<.1)
    return(MS_NINT(d*100)/100.0);
  if(d<1)
    return(MS_NINT(d*10)/10.0);
  if(d<100)
    return(MS_NINT(d));
  if(d<1000)
    return(MS_NINT(d/10) * 10);
  if(d<10000)
    return(MS_NINT(d/100) * 100);
  if(d<100000)
    return(MS_NINT(d/1000) * 1000);
  if(d<1000000)
    return(MS_NINT(d/10000) * 10000);
  if(d<10000000)
    return(MS_NINT(d/100000) * 100000);
  if(d<100000000)
    return(MS_NINT(d/1000000) * 1000000);

  return(-1);
}

/*
** Calculate the approximate scale based on a few parameters. Note that this assumes the scale is
** the same in the x direction as in the y direction, so run msAdjustExtent(...) first.
*/
int msCalculateScale(rectObj extent, int units, int width, int height, double resolution, double *scale)
{
  double md, gd, center_y;

  /* if((extent.maxx == extent.minx) || (extent.maxy == extent.miny))   */
  if(!MS_VALID_EXTENT(extent)) {
    msSetError(MS_MISCERR, "Invalid image extent, minx=%lf, miny=%lf, maxx=%lf, maxy=%lf.", "msCalculateScale()", extent.minx, extent.miny, extent.maxx, extent.maxy);
    return(MS_FAILURE);
  }
  
  if((width <= 0) || (height <= 0)) {
    msSetError(MS_MISCERR, "Invalid image width or height.", "msCalculateScale()");
    return(MS_FAILURE);
  }

  switch (units) {
  case(MS_DD):
  case(MS_METERS):    
  case(MS_KILOMETERS):
  case(MS_MILES):
  case(MS_NAUTICALMILES):
  case(MS_INCHES):  
  case(MS_FEET):
    center_y = (extent.miny+extent.maxy)/2.0;
    md = (width-1)/(resolution*msInchesPerUnit(units, center_y)); /* remember, we use a pixel-center to pixel-center extent, hence the width-1 */
    gd = extent.maxx - extent.minx;
    *scale = gd/md;
    break;
  default:
    *scale = -1; /* this is not an error */
    break;
  }

  return(MS_SUCCESS);
}

double msInchesPerUnit(int units, double center_lat)
{
  double lat_adj = 1.0, ipu = 1.0;

  switch (units) {
  case(MS_METERS):    
  case(MS_KILOMETERS):
  case(MS_MILES):
  case(MS_NAUTICALMILES):
  case(MS_INCHES):  
  case(MS_FEET):
    ipu = inchesPerUnit[units]; 
    break;
  case(MS_DD):
    /* With geographical (DD) coordinates, we adjust the inchesPerUnit
     * based on the latitude of the center of the view. For this we assume
     * we have a perfect sphere and just use cos(lat) in our calculation.
     */
#ifdef ENABLE_VARIABLE_INCHES_PER_DEGREE
    if (center_lat != 0.0)
    {
        double cos_lat;
        cos_lat = cos(MS_PI*center_lat/180.0);
        lat_adj = sqrt(1+cos_lat*cos_lat)/sqrt(2.0);
    }
#endif
    ipu = inchesPerUnit[units]*lat_adj;
    break;
  default:
    break;
  }

  return ipu;
}


#define X_STEP_SIZE 5

/* TODO : the will be msDrawScalebarGD */
imageObj *msDrawScalebar(mapObj *map)
{
  int status;
  gdImagePtr img=NULL;
  char label[32];
  double i, msx;
  int j;
  int isx, sx, sy, ox, oy, state, dsx;
  pointObj p,p2;
  rectObj r;
  gdFontPtr fontPtr = NULL;
  imageObj      *image = NULL;
  outputFormatObj *format = NULL;
  int iFreeGDFont = 0;

  if(map->units == -1) {
    msSetError(MS_MISCERR, "Map units not set.", "msDrawScalebar()");
    return(NULL);
  }

  if(MS_RENDERER_PLUGIN(map->outputformat)) {
    msSetError(MS_MISCERR, "Scalebar not supported yet", "msDrawScalebar()");
    return(NULL);
  }
/*
 *  Allow scalebars to use TrueType fonts for labels (jnovak@novacell.com)
 *
 *  A string containing the ten decimal digits is rendered to compute an average cell size 
 *  for each number, which is used later to place labels on the scalebar.
 */
  if( map->scalebar.label.type == MS_TRUETYPE )
  {
#ifdef USE_GD_FT
	int bbox[8];
    char *error	= NULL;
    char *font	= NULL;
    char szTestString[] = "0123456789";

    fontPtr = (gdFontPtr) malloc( sizeof( gdFont ) );

	if(!fontPtr) {
      msSetError(MS_TTFERR, "fontPtr allocation failed.", "msDrawScalebar()");
      return(NULL);
    };

    if(! map->fontset.filename ) {
      msSetError(MS_TTFERR, "No fontset defined.", "msDrawScalebar()");
	  free( fontPtr );
      return(NULL);
    }

    if(! map->scalebar.label.font) {
      msSetError(MS_TTFERR, "No TrueType font defined.", "msDrawScalebar()");
	  free( fontPtr );
      return(NULL);
    }

    font = msLookupHashTable(&(map->fontset.fonts), map->scalebar.label.font);
    if(!font) {
      msSetError(MS_TTFERR, "Requested font (%s) not found.", "msDrawScalebar()", map->scalebar.label.font);
	  free( fontPtr );
      return(NULL);
    }

    error = gdImageStringFT( NULL, bbox, map->scalebar.label.outlinecolor.pen, font, map->scalebar.label.size, 0, 0, 0, szTestString );

    if(error) {
      msSetError(MS_TTFERR, "gdImageStringFT returned error %d.", "msDrawScalebar()", error );
	  free( fontPtr );
      return(NULL);
    }

	iFreeGDFont = 1;
	fontPtr->w	= (bbox[2] - bbox[0]) / strlen( szTestString );
	fontPtr->h  = (bbox[3] - bbox[5]);
#else
    msSetError(MS_TTFERR, "TrueType font support required.", "msDrawScalebar()");
    return(NULL);
#endif
  }
  else
    fontPtr = msGetBitmapFont(MS_NINT(map->scalebar.label.size));

  if(!fontPtr) return(NULL);

  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scaledenom);
  if(status != MS_SUCCESS)
  { 	
    if( iFreeGDFont ) free( fontPtr );
	return(NULL);
  }
  dsx = map->scalebar.width - 2*HMARGIN;
  do {
    msx = (map->cellsize * dsx)/(msInchesPerUnit(map->scalebar.units,0)/msInchesPerUnit(map->units,0));
    i = roundInterval(msx/map->scalebar.intervals);
    sprintf(label, "%g", map->scalebar.intervals*i); /* last label */
    isx = MS_NINT((i/(msInchesPerUnit(map->units,0)/msInchesPerUnit(map->scalebar.units,0)))/map->cellsize);  
    sx = (map->scalebar.intervals*isx) + MS_NINT((1.5 + strlen(label)/2.0 + strlen(unitText[map->scalebar.units]))*fontPtr->w);

    if(sx <= (map->scalebar.width - 2*HMARGIN)) break; /* it will fit */

    dsx -= X_STEP_SIZE; /* change the desired size in hopes that it will fit in user supplied width */
  } while(1);

  sy = (2*VMARGIN) + MS_NINT(VSPACING*fontPtr->h) + fontPtr->h + map->scalebar.height - VSLOP;

  /*Ensure we have an image format representing the options for the scalebar.*/
  msApplyOutputFormat( &format, map->outputformat, 
                       map->scalebar.transparent, 
                       map->scalebar.interlace, 
                       MS_NOOVERRIDE );

   /* create image */
  image = msImageCreateGD(map->scalebar.width, sy, format,
                          map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution);

  /* drop this reference to output format */
  msApplyOutputFormat( &format, NULL, 
                       MS_NOOVERRIDE, MS_NOOVERRIDE, MS_NOOVERRIDE );

  /* did we succeed in creating the image? */
  if(image)
    img = image->img.gd;
  else {
    msSetError(MS_GDERR, "Unable to initialize image.", "msDrawScalebar()");
	
    if( iFreeGDFont ) free( fontPtr );

    return(NULL);
  }

  msImageInitGD( image, &(map->scalebar.imagecolor));
  

  if (map->outputformat->imagemode == MS_IMAGEMODE_RGB || map->outputformat->imagemode == MS_IMAGEMODE_RGBA) gdImageAlphaBlending(image->img.gd, 1);

  switch(map->scalebar.align) {
  case(MS_ALIGN_LEFT):
    ox = HMARGIN;
    break;
  case(MS_ALIGN_RIGHT):
    ox = MS_NINT((map->scalebar.width - sx) + fontPtr->w);
    break;
  default:
    ox = MS_NINT((map->scalebar.width - sx)/2.0 + fontPtr->w/2.0); /* center the computed scalebar */
  }
  oy = VMARGIN;

  /* turn RGB colors into indexed values */
  msImageSetPenGD(img, &(map->scalebar.backgroundcolor));
  msImageSetPenGD(img, &(map->scalebar.color));
  msImageSetPenGD(img, &(map->scalebar.outlinecolor));

  switch(map->scalebar.style) {
  case(0):
    
    state = 1; /* 1 means filled */
    for(j=0; j<map->scalebar.intervals; j++) {
      if(state == 1)
	gdImageFilledRectangle(img, ox + j*isx, oy, ox + (j+1)*isx, oy + map->scalebar.height, map->scalebar.color.pen);
      else
	if(map->scalebar.backgroundcolor.pen >= 0) gdImageFilledRectangle(img, ox + j*isx, oy, ox + (j+1)*isx, oy + map->scalebar.height, map->scalebar.backgroundcolor.pen);

      if(map->scalebar.outlinecolor.pen >= 0)
	gdImageRectangle(img, ox + j*isx, oy, ox + (j+1)*isx, oy + map->scalebar.height, map->scalebar.outlinecolor.pen);

      sprintf(label, "%g", j*i);
      map->scalebar.label.position = MS_CC;
      p.x = ox + j*isx; /* + MS_NINT(fontPtr->w/2); */
      p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
      if(msGetLabelSize(NULL,label,&(map->scalebar.label), &r, 
              &(map->fontset), 1, MS_FALSE,NULL) == -1) return(NULL);
      p2 = get_metrics(&p, MS_CC, r, 0,0, 0, 0, NULL);
      msDrawTextGD(image, p2, label, &(map->scalebar.label), &(map->fontset), 1.0);

      state = -state;
    }
    sprintf(label, "%g", j*i);
    ox = ox + j*isx - MS_NINT((strlen(label)*fontPtr->w)/2.0);
    sprintf(label, "%g %s", j*i, unitText[map->scalebar.units]);
    map->scalebar.label.position = MS_CR;
    p.x = ox; /* + MS_NINT(fontPtr->w/2); */
    p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
    if(msGetLabelSize(NULL,label,&(map->scalebar.label), &r, 
            &(map->fontset), 1, MS_FALSE,NULL) == -1) return(NULL);
    p2 = get_metrics(&p, MS_CR, r, 0,0, 0, 0, NULL);
    msDrawTextGD(image, p2, label, &(map->scalebar.label), &(map->fontset), 1.0);

    break;

  case(1):

    gdImageLine(img, ox, oy, ox + isx*map->scalebar.intervals, oy, map->scalebar.color.pen); /* top line */

    state = 1; /* 1 means filled */
    for(j=0; j<map->scalebar.intervals; j++) {

      gdImageLine(img, ox + j*isx, oy, ox + j*isx, oy + map->scalebar.height, map->scalebar.color.pen); /* tick */
      
      sprintf(label, "%g", j*i);
      map->scalebar.label.position = MS_CC;
      p.x = ox + j*isx; /* + MS_NINT(fontPtr->w/2); */
      p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
      if(msGetLabelSize(NULL,label,&(map->scalebar.label), &r, 
              &(map->fontset), 1, MS_FALSE,NULL) == -1) return(NULL);
      p2 = get_metrics(&p, MS_CC, r, 0,0, 0, 0, NULL);
      msDrawTextGD(image, p2, label, &(map->scalebar.label), &(map->fontset), 1.0);

      state = -state;
    }
    
    gdImageLine(img, ox + j*isx, oy, ox + j*isx, oy + map->scalebar.height, map->scalebar.color.pen); /* last tick */

    sprintf(label, "%g", j*i);
    ox = ox + j*isx - MS_NINT((strlen(label)*fontPtr->w)/2.0);
    sprintf(label, "%g %s", j*i, unitText[map->scalebar.units]);
    map->scalebar.label.position = MS_CR;
    p.x = ox; /* + MS_NINT(fontPtr->w/2); */
    p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
    if(msGetLabelSize(NULL,label,&(map->scalebar.label), &r, 
            &(map->fontset), 1, MS_FALSE,NULL) == -1) return(NULL);
    p2 = get_metrics(&p, MS_CR, r, 0,0, 0, 0, NULL);
    msDrawTextGD(image, p2, label, &(map->scalebar.label), &(map->fontset), 1.0);

    break;
  default:
    msSetError(MS_MISCERR, "Unsupported scalebar style.", "msDrawScalebar()");
    return(NULL);

    if( iFreeGDFont ) free( fontPtr );
    break;
  }

  msClearScalebarPenValues( &(map->scalebar));

  if( iFreeGDFont ) free( fontPtr );
  return(image);

}

int msEmbedScalebar(mapObj *map, imageObj *img)
{
  int s,l;
  pointObj point;
  imageObj *image = NULL;

  s = msGetSymbolIndex(&(map->symbolset), "scalebar", MS_FALSE);
  if(s != -1) 
    msRemoveSymbol(&(map->symbolset), s); /* solves some caching issues in AGG with long-running processes */

  if(msGrowSymbolSet(&map->symbolset) == NULL)
    return -1;
  s = map->symbolset.numsymbols;
  map->symbolset.numsymbols++;
  initSymbol(map->symbolset.symbol[s]);
  
  image = msDrawScalebar(map);
  map->symbolset.symbol[s]->img =  image->img.gd; /* TODO  */
  if(!map->symbolset.symbol[s]->img) return(-1); /* something went wrong creating scalebar */

  map->symbolset.symbol[s]->type = MS_SYMBOL_PIXMAP; /* intialize a few things */
  map->symbolset.symbol[s]->name = strdup("scalebar");  
  map->symbolset.symbol[s]->sizex = map->symbolset.symbol[s]->img->sx;
  map->symbolset.symbol[s]->sizey = map->symbolset.symbol[s]->img->sy;

  /* in 8 bit, mark 0 as being transparent.  In 24bit hopefully already
     have transparency enabled ... */
  if(map->scalebar.transparent == MS_ON && !gdImageTrueColor(image->img.gd ) )
    gdImageColorTransparent(map->symbolset.symbol[s]->img, 0);

  switch(map->scalebar.position) {
  case(MS_LL):
    point.x = MS_NINT(map->symbolset.symbol[s]->img->sx/2.0);
    point.y = map->height - MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_LR):
    point.x = map->width - MS_NINT(map->symbolset.symbol[s]->img->sx/2.0);
    point.y = map->height - MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_LC):
    point.x = MS_NINT(map->width/2.0);
    point.y = map->height - MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_UR):
    point.x = map->width - MS_NINT(map->symbolset.symbol[s]->img->sx/2.0);
    point.y = MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_UL):
    point.x = MS_NINT(map->symbolset.symbol[s]->img->sx/2.0);
    point.y = MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  case(MS_UC):
    point.x = MS_NINT(map->width/2.0);
    point.y = MS_NINT(map->symbolset.symbol[s]->img->sy/2.0);
    break;
  }

  l = msGetLayerIndex(map, "__embed__scalebar");
  if(l == -1) {
    if (msGrowMapLayers(map) == NULL)
        return(-1);
    l = map->numlayers;
    map->numlayers++;
    if(initLayer((GET_LAYER(map, l)), map) == -1) return(-1);
    GET_LAYER(map, l)->name = strdup("__embed__scalebar");
    GET_LAYER(map, l)->type = MS_LAYER_ANNOTATION;

    if (msGrowLayerClasses( GET_LAYER(map, l) ) == NULL)
        return(-1);

    if(initClass(GET_LAYER(map, l)->class[0]) == -1) return(-1);
    GET_LAYER(map, l)->numclasses = 1; /* so we make sure to free it */
    
    /* update the layer order list with the layer's index. */
    map->layerorder[l] = l;
  }

  GET_LAYER(map, l)->opacity = MS_GD_ALPHA; /* to resolve bug 490 */
  GET_LAYER(map, l)->status = MS_ON;

  /* TODO: Change this when we get rid of MS_MAXSTYLES */
  if (msMaybeAllocateStyle(GET_LAYER(map, l)->class[0], 0)==MS_FAILURE) return MS_FAILURE;
  GET_LAYER(map, l)->class[0]->styles[0]->symbol = s;
  GET_LAYER(map, l)->class[0]->styles[0]->color.pen = -1;
  GET_LAYER(map, l)->class[0]->label.force = MS_TRUE;
  GET_LAYER(map, l)->class[0]->label.size = MS_MEDIUM; /* must set a size to have a valid label definition */
  GET_LAYER(map, l)->class[0]->label.priority = MS_MAX_LABEL_PRIORITY;

  if(map->scalebar.postlabelcache) /* add it directly to the image //TODO */
  {
#ifdef USE_AGG
      /* the next function will call the AGG renderer so we must make
       * sure the alpha channel of the image is in a coherent state.
       * the alpha values aren't actually switched unless the last 
       * layer of the map was a raster layer
       */
      if(MS_RENDERER_AGG(map->outputformat))
        msAlphaGD2AGG(img);
#endif
      msDrawMarkerSymbol(&map->symbolset, img, &point, GET_LAYER(map, l)->class[0]->styles[0], 1.0);
  } else {
    msAddLabel(map, l, 0, NULL, &point, NULL, " ", 1.0, NULL);
  }

  /* Mark layer as deleted so that it doesn't interfere with html legends or with saving maps */
  GET_LAYER(map, l)->status = MS_DELETE;

  /* Free image (but keep the GD image which is in the symbol cache now) */
  image->img.gd = NULL;
  msFreeImage( image );

  return(0);
}

/************************************************************************/
/* These two functions are used in PHP/Mapscript and Swig/Mapscript     */
/************************************************************************/

/************************************************************************/
/*  double GetDeltaExtentsUsingScale(double scale, int units,           */
/*                                   double centerLat, int width,       */
/*                                   double resolution)                 */
/*                                                                      */
/*      Utility function to return the maximum extent using the         */
/*      scale and the width of the image.                               */
/*                                                                      */
/*      Base on the function msCalculateScale (mapscale.c)              */
/************************************************************************/
double GetDeltaExtentsUsingScale(double scale, int units, double centerLat, int width, double resolution)
{
    double md = 0.0;
    double dfDelta = -1.0;

    if (scale <= 0 || width <=0)
      return -1;

    switch (units) 
    {
      case(MS_DD):
      case(MS_METERS):    
      case(MS_KILOMETERS):
      case(MS_MILES):
      case(MS_NAUTICALMILES):
      case(MS_INCHES):  
      case(MS_FEET):
        /* remember, we use a pixel-center to pixel-center extent, hence the width-1 */
        md = (width-1)/(resolution*msInchesPerUnit(units,centerLat));
        dfDelta = md * scale;
        break;
          
      default:
        break;
    }

    return dfDelta;
}

/************************************************************************/
/*    static double Pix2Georef(int nPixPos, int nPixMin, double nPixMax,*/
/*                              double dfGeoMin, double dfGeoMax,       */
/*                              bool bULisYOrig)                        */
/*                                                                      */
/*      Utility function to convert a pixel pos to georef pos. If       */
/*      bULisYOrig parameter is set to true then the upper left is      */
/*      considered to be the Y origin.                                  */
/*                                                                      */
/************************************************************************/
double Pix2Georef(int nPixPos, int nPixMin, int nPixMax, 
                         double dfGeoMin, double dfGeoMax, int bULisYOrig)
{
    double      dfWidthGeo = 0.0;
    int         nWidthPix = 0;
    double      dfPixToGeo = 0.0;
    double      dfPosGeo = 0.0;
    double      dfDeltaGeo = 0.0;
    int         nDeltaPix = 0;

    dfWidthGeo = dfGeoMax - dfGeoMin;
    nWidthPix = nPixMax - nPixMin;
   
    if (dfWidthGeo > 0.0 && nWidthPix > 0)
    {
        dfPixToGeo = dfWidthGeo / (double)nWidthPix;

        if (!bULisYOrig)
            nDeltaPix = nPixPos - nPixMin;
        else
            nDeltaPix = nPixMax - nPixPos;
        
        dfDeltaGeo = nDeltaPix * dfPixToGeo;

        dfPosGeo = dfGeoMin + dfDeltaGeo;
    }
    return (dfPosGeo);
}

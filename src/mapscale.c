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



#define VMARGIN 3 /* buffer around the scalebar */
#define HMARGIN 3
#define VSPACING .8 /* spacing (% of font height) between scalebar and text */
#define VSLOP 5 /* makes things fit a bit better vertically */

/*
** Match this with with unit enumerations is mapserver.h
*/
static char *unitText[9]= {"in", "ft", "mi", "m", "km", "dd", "??", "??", "NM"}; /* MS_PIXEL and MS_PERCENTAGE not used */
double inchesPerUnit[9]= {1, 12, 63360.0, 39.3701, 39370.1, 4374754, 1, 1, 72913.3858 };

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
  (void)center_lat;
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
      if (center_lat != 0.0) {
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

imageObj *msDrawScalebar(mapObj *map)
{
  int status;
  char label[32];
  double i, msx;
  int j;
  int isx, sx, sy, ox, oy, state, dsx;
  pointObj p;
  rectObj r;
  imageObj      *image = NULL;
  double fontWidth, fontHeight;
  outputFormatObj *format = NULL;
  strokeStyleObj strokeStyle = {0};
  shapeObj shape;
  lineObj line;
  pointObj points[5];
  textSymbolObj ts;
  rendererVTableObj *renderer;

  strokeStyle.patternlength=0;
  initTextSymbol(&ts);

  if((int)map->units == -1) {
    msSetError(MS_MISCERR, "Map units not set.", "msDrawScalebar()");
    return(NULL);
  }

  renderer = MS_MAP_RENDERER(map);
  if(!renderer || !MS_MAP_RENDERER(map)->supports_pixel_buffer) {
    msSetError(MS_MISCERR, "Outputformat not supported for scalebar", "msDrawScalebar()");
    return(NULL);
  }
  
  msPopulateTextSymbolForLabelAndString(&ts,&map->scalebar.label,msStrdup("0123456789"),1.0,map->resolution/map->defresolution, 0);

  /*
   *  A string containing the ten decimal digits is rendered to compute an average cell size
   *  for each number, which is used later to place labels on the scalebar.
   */
  

  if(msGetTextSymbolSize(map,&ts,&r) != MS_SUCCESS) {
    return NULL;
  }
  fontWidth = (r.maxx-r.minx)/10.0;
  fontHeight = r.maxy -r.miny;

  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scaledenom);
  if(status != MS_SUCCESS) {
    return(NULL);
  }
  dsx = map->scalebar.width - 2*HMARGIN;
  do {
    msx = (map->cellsize * dsx)/(msInchesPerUnit(map->scalebar.units,0)/msInchesPerUnit(map->units,0));
    i = roundInterval(msx/map->scalebar.intervals);
    snprintf(label, sizeof(label), "%g", map->scalebar.intervals*i); /* last label */
    isx = MS_NINT((i/(msInchesPerUnit(map->units,0)/msInchesPerUnit(map->scalebar.units,0)))/map->cellsize);
    sx = (map->scalebar.intervals*isx) + MS_NINT((1.5 + strlen(label)/2.0 + strlen(unitText[map->scalebar.units]))*fontWidth);

    if(sx <= (map->scalebar.width - 2*HMARGIN)) break; /* it will fit */

    dsx -= X_STEP_SIZE; /* change the desired size in hopes that it will fit in user supplied width */
  } while(1);

  sy = (2*VMARGIN) + MS_NINT(VSPACING*fontHeight) + fontHeight + map->scalebar.height - VSLOP;

  /*Ensure we have an image format representing the options for the scalebar.*/
  msApplyOutputFormat( &format, map->outputformat, map->scalebar.transparent);

  if(map->scalebar.transparent == MS_OFF) {
    if(!MS_VALID_COLOR(map->scalebar.imagecolor))
      MS_INIT_COLOR(map->scalebar.imagecolor,255,255,255,255);
  }
  image = msImageCreate(map->scalebar.width, sy, format,
                        map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution, &map->scalebar.imagecolor);

  /* did we succeed in creating the image? */
  if(!image) {
    msSetError(MS_MISCERR, "Unable to initialize image.", "msDrawScalebar()");
    return NULL;
  }
  image->map = map;

  /* drop this reference to output format */
  msApplyOutputFormat( &format, NULL, MS_NOOVERRIDE);

  switch(map->scalebar.align) {
    case(MS_ALIGN_LEFT):
      ox = HMARGIN;
      break;
    case(MS_ALIGN_RIGHT):
      ox = MS_NINT((map->scalebar.width - sx) + fontWidth);
      break;
    default:
      ox = MS_NINT((map->scalebar.width - sx)/2.0 + fontWidth/2.0); /* center the computed scalebar */
  }
  oy = VMARGIN;

  switch(map->scalebar.style) {
    case(0): {

      line.numpoints = 5;
      line.point = points;
      shape.line = &line;
      shape.numlines = 1;
      if(MS_VALID_COLOR(map->scalebar.color)) {
        INIT_STROKE_STYLE(strokeStyle);
        strokeStyle.color = &map->scalebar.outlinecolor;
        strokeStyle.color->alpha = 255;
        strokeStyle.width = 1;
      }
      map->scalebar.backgroundcolor.alpha = 255;
      map->scalebar.color.alpha = 255;
      state = 1; /* 1 means filled */
      for(j=0; j<map->scalebar.intervals; j++) {
        points[0].x = points[4].x = points[3].x = ox + j*isx + 0.5;
        points[0].y = points[4].y = points[1].y = oy + 0.5;
        points[1].x = points[2].x = ox + (j+1)*isx + 0.5;
        points[2].y = points[3].y = oy + map->scalebar.height + 0.5;
        if(state == 1 && MS_VALID_COLOR(map->scalebar.color))
          status = renderer->renderPolygon(image,&shape,&map->scalebar.color);
        else if(MS_VALID_COLOR(map->scalebar.backgroundcolor))
          status = renderer->renderPolygon(image,&shape,&map->scalebar.backgroundcolor);

        if(MS_UNLIKELY(status == MS_FAILURE)) {
          goto scale_cleanup;
        }

        if(strokeStyle.color) {
          status = renderer->renderLine(image,&shape,&strokeStyle);

          if(MS_UNLIKELY(status == MS_FAILURE)) {
            goto scale_cleanup;
          }
        }

        sprintf(label, "%g", j*i);
        map->scalebar.label.position = MS_CC;
        p.x = ox + j*isx; /* + MS_NINT(fontPtr->w/2); */
        p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontHeight);
        status = msDrawLabel(map,image,p,msStrdup(label),&map->scalebar.label,1.0);
        if(MS_UNLIKELY(status == MS_FAILURE)) {
          goto scale_cleanup;
        }
        state = -state;
      }
      sprintf(label, "%g", j*i);
      ox = ox + j*isx - MS_NINT((strlen(label)*fontWidth)/2.0);
      sprintf(label, "%g %s", j*i, unitText[map->scalebar.units]);
      map->scalebar.label.position = MS_CR;
      p.x = ox; /* + MS_NINT(fontPtr->w/2); */
      p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontHeight);
      status = msDrawLabel(map,image,p,msStrdup(label),&map->scalebar.label,1.0);
      if(MS_UNLIKELY(status == MS_FAILURE)) {
        goto scale_cleanup;
      }
      break;
    }
    case(1): {
      line.numpoints = 2;
      line.point = points;
      shape.line = &line;
      shape.numlines = 1;
      if(MS_VALID_COLOR(map->scalebar.color)) {
        strokeStyle.width = 1;
        strokeStyle.color = &map->scalebar.color;
      }

      points[0].y = points[1].y = oy;
      points[0].x = ox;
      points[1].x = ox + isx*map->scalebar.intervals;
      status = renderer->renderLine(image,&shape,&strokeStyle);
      if(MS_UNLIKELY(status == MS_FAILURE)) {
        goto scale_cleanup;
      }

      points[0].y = oy;
      points[1].y = oy + map->scalebar.height;
      p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontHeight);
      for(j=0; j<=map->scalebar.intervals; j++) {
        points[0].x = points[1].x = ox + j*isx;
        status = renderer->renderLine(image,&shape,&strokeStyle);
        if(MS_UNLIKELY(status == MS_FAILURE)) {
          goto scale_cleanup;
        }

        sprintf(label, "%g", j*i);
        if(j!=map->scalebar.intervals) {
          map->scalebar.label.position = MS_CC;
          p.x = ox + j*isx; /* + MS_NINT(fontPtr->w/2); */
        } else {
          sprintf(label, "%g %s", j*i, unitText[map->scalebar.units]);
          map->scalebar.label.position = MS_CR;
          p.x = ox + j*isx - MS_NINT((strlen(label)*fontWidth)/2.0);
        }
        status = msDrawLabel(map,image,p,msStrdup(label),&map->scalebar.label,1.0);
        if(MS_UNLIKELY(status == MS_FAILURE)) {
          goto scale_cleanup;
        }
      }
      break;
    }
    default:
      msSetError(MS_MISCERR, "Unsupported scalebar style.", "msDrawScalebar()");
      return(NULL);
  }

scale_cleanup:
  freeTextSymbol(&ts);
  if(MS_UNLIKELY(status == MS_FAILURE)) {
    msFreeImage(image);
    return NULL;
  }
  return(image);

}

int msEmbedScalebar(mapObj *map, imageObj *img)
{
  int l,index,s,status = MS_SUCCESS;
  pointObj point;
  imageObj *image = NULL;
  rendererVTableObj *renderer;
  symbolObj *embededSymbol;
  char* imageType = NULL;

  index = msGetSymbolIndex(&(map->symbolset), "scalebar", MS_FALSE);
  if(index != -1)
    msRemoveSymbol(&(map->symbolset), index); /* remove cached symbol in case the function is called multiple
                      times with different zoom levels */

  if((embededSymbol=msGrowSymbolSet(&map->symbolset)) == NULL)
    return MS_FAILURE;

  s = map->symbolset.numsymbols;
  map->symbolset.numsymbols++;

  if(!MS_RENDERER_PLUGIN(map->outputformat) || !MS_MAP_RENDERER(map)->supports_pixel_buffer) {
    imageType = msStrdup(map->imagetype); /* save format */
    if MS_DRIVER_CAIRO(map->outputformat)
      map->outputformat = msSelectOutputFormat( map, "cairopng" );
    else
      map->outputformat = msSelectOutputFormat( map, "png" );
    
    msInitializeRendererVTable(map->outputformat);
  }
  renderer = MS_MAP_RENDERER(map);

  image = msDrawScalebar(map);

  if (imageType) {
    map->outputformat = msSelectOutputFormat( map, imageType ); /* restore format */
    msFree(imageType);
  }

  if(!image) {
    return MS_FAILURE;
  }
  embededSymbol->pixmap_buffer = calloc(1,sizeof(rasterBufferObj));
  MS_CHECK_ALLOC(embededSymbol->pixmap_buffer, sizeof(rasterBufferObj), MS_FAILURE);

  if(MS_SUCCESS != renderer->getRasterBufferCopy(image,embededSymbol->pixmap_buffer)) {
    return MS_FAILURE;
  }

  embededSymbol->type = MS_SYMBOL_PIXMAP; /* intialize a few things */
  embededSymbol->name = msStrdup("scalebar");
  embededSymbol->sizex = embededSymbol->pixmap_buffer->width;
  embededSymbol->sizey = embededSymbol->pixmap_buffer->height;
  if(map->scalebar.transparent) {
    embededSymbol->transparent = MS_TRUE;
    embededSymbol->transparentcolor = 0;
  }

  switch(map->scalebar.position) {
    case(MS_LL):
      point.x = MS_NINT(embededSymbol->pixmap_buffer->width/2.0) + map->scalebar.offsetx;
      point.y = map->height - MS_NINT(embededSymbol->pixmap_buffer->height/2.0) - map->scalebar.offsety;
      break;
    case(MS_LR):
      point.x = map->width - MS_NINT(embededSymbol->pixmap_buffer->width/2.0) - map->scalebar.offsetx;
      point.y = map->height - MS_NINT(embededSymbol->pixmap_buffer->height/2.0) - map->scalebar.offsety;
      break;
    case(MS_LC):
      point.x = MS_NINT(map->width/2.0) + map->scalebar.offsetx;
      point.y = map->height - MS_NINT(embededSymbol->pixmap_buffer->height/2.0) - map->scalebar.offsety;
      break;
    case(MS_UR):
      point.x = map->width - MS_NINT(embededSymbol->pixmap_buffer->width/2.0) - map->scalebar.offsetx;
      point.y = MS_NINT(embededSymbol->pixmap_buffer->height/2.0) + map->scalebar.offsety;
      break;
    case(MS_UL):
      point.x = MS_NINT(embededSymbol->pixmap_buffer->width/2.0) + map->scalebar.offsetx;
      point.y = MS_NINT(embededSymbol->pixmap_buffer->height/2.0) + map->scalebar.offsety;
      break;
    case(MS_UC):
      point.x = MS_NINT(map->width/2.0) + map->scalebar.offsetx;
      point.y = MS_NINT(embededSymbol->pixmap_buffer->height/2.0) + map->scalebar.offsety;
      break;
  }

  l = msGetLayerIndex(map, "__embed__scalebar");
  if(l == -1) {
    if (msGrowMapLayers(map) == NULL)
      return(-1);
    l = map->numlayers;
    map->numlayers++;
    if(initLayer((GET_LAYER(map, l)), map) == -1) return(-1);
    GET_LAYER(map, l)->name = msStrdup("__embed__scalebar");
    GET_LAYER(map, l)->type = MS_LAYER_POINT;

    if (msGrowLayerClasses( GET_LAYER(map, l) ) == NULL)
      return(-1);

    if(initClass(GET_LAYER(map, l)->class[0]) == -1) return(-1);
    GET_LAYER(map, l)->numclasses = 1; /* so we make sure to free it */

    /* update the layer order list with the layer's index. */
    map->layerorder[l] = l;
  }

  GET_LAYER(map, l)->status = MS_ON;
  if(map->scalebar.postlabelcache) { /* add it directly to the image */
    if(msMaybeAllocateClassStyle(GET_LAYER(map, l)->class[0], 0)==MS_FAILURE) return MS_FAILURE;
    GET_LAYER(map, l)->class[0]->styles[0]->symbol = s;
    status = msDrawMarkerSymbol(map, img, &point, GET_LAYER(map, l)->class[0]->styles[0], 1.0);
    if(MS_UNLIKELY(status == MS_FAILURE)) {
      goto embed_cleanup;
    }
  } else {
    if(!GET_LAYER(map, l)->class[0]->labels) {
      if(msGrowClassLabels(GET_LAYER(map, l)->class[0]) == NULL) return MS_FAILURE;
      initLabel(GET_LAYER(map, l)->class[0]->labels[0]);
      GET_LAYER(map, l)->class[0]->numlabels = 1;
      GET_LAYER(map, l)->class[0]->labels[0]->force = MS_TRUE;
      GET_LAYER(map, l)->class[0]->labels[0]->size = MS_MEDIUM; /* must set a size to have a valid label definition */
      GET_LAYER(map, l)->class[0]->labels[0]->priority = MS_MAX_LABEL_PRIORITY;
    }
    if(GET_LAYER(map, l)->class[0]->labels[0]->numstyles == 0) {
      if(msGrowLabelStyles(GET_LAYER(map,l)->class[0]->labels[0]) == NULL)
        return(MS_FAILURE);
      GET_LAYER(map,l)->class[0]->labels[0]->numstyles = 1;
      initStyle(GET_LAYER(map,l)->class[0]->labels[0]->styles[0]);
      GET_LAYER(map,l)->class[0]->labels[0]->styles[0]->_geomtransform.type = MS_GEOMTRANSFORM_LABELPOINT;
    }
    GET_LAYER(map,l)->class[0]->labels[0]->styles[0]->symbol = s;
    status = msAddLabel(map, img, GET_LAYER(map, l)->class[0]->labels[0], l, 0, NULL, &point, -1, NULL);
    if(MS_UNLIKELY(status == MS_FAILURE)) {
      goto embed_cleanup;
    }
  }

embed_cleanup:
  /* Mark layer as deleted so that it doesn't interfere with html legends or with saving maps */
  GET_LAYER(map, l)->status = MS_DELETE;

  msFreeImage( image );
  return status;
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

  switch (units) {
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
  double      dfPosGeo = 0.0;

  const double dfWidthGeo = dfGeoMax - dfGeoMin;
  const int nWidthPix = nPixMax - nPixMin;

  if (dfWidthGeo > 0.0 && nWidthPix > 0) {
    const double dfPixToGeo = dfWidthGeo / (double)nWidthPix;

    int nDeltaPix;
    if (!bULisYOrig)
      nDeltaPix = nPixPos - nPixMin;
    else
      nDeltaPix = nPixMax - nPixPos;

    const double dfDeltaGeo = nDeltaPix * dfPixToGeo;

    dfPosGeo = dfGeoMin + dfDeltaGeo;
  }
  return (dfPosGeo);
}

/* This function converts a pixel value in geo ref. The return value is in
 * layer units. This function has been added for the purpose of the ticket #1340 */

double Pix2LayerGeoref(mapObj *map, layerObj *layer, int value)
{
  double cellsize = MS_MAX(MS_CELLSIZE(map->extent.minx, map->extent.maxx, map->width),
                           MS_CELLSIZE(map->extent.miny, map->extent.maxy, map->height));

  double resolutionFactor = map->resolution/map->defresolution;
  double unitsFactor = 1;

  if (layer->sizeunits != MS_PIXELS)
    unitsFactor = msInchesPerUnit(map->units,0)/msInchesPerUnit(layer->sizeunits,0);

  return value*cellsize*resolutionFactor*unitsFactor;
}

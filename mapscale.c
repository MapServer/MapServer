#include "map.h"

#define VMARGIN 5 /* buffer around the scalebar */
#define HMARGIN 5
#define VSPACING .8 /* spacing (% of font height) between scalebar and text */
#define VSLOP 5 /* makes things fit a bit better vertically */

/*
** Match this with with unit enumerations is map.h
*/
static char *unitText[5]={"in", "ft", "mi", "m", "km"};
static double inchesPerUnit[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};

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

  return(-1);
}

/*
** Calculate the approximate scale based on a few parameters. Note that this assumes the scale is
** the same in the x direction as in the y direction, so run msAdjustExtent(...) first.
*/
double msCalculateScale(rectObj extent, int units, int width, int height)
{
  double md, gd, scale;

  if((extent.maxx == extent.minx) || (extent.maxy == extent.miny))
    return(-1);

  if((width <= 0) || (height <= 0))
    return(-1);

  switch (units) {
  case(MS_DD):
  case(MS_METERS):    
  case(MS_KILOMETERS):
  case(MS_MILES):
  case(MS_INCHES):  
  case(MS_FEET):
    md = (width-1)/(MS_PPI*inchesPerUnit[units]);
    gd = extent.maxx - extent.minx;
    scale = gd/md;
    break;
  default:
    return(-1);
    break;
  }

  return(scale);
}

gdImagePtr msDrawScalebar(mapObj *map)
{
  gdImagePtr img=NULL;
  char label[32];
  double msx, i;
  int j; 
  int isx, sx, sy, ox, oy, state;
  pointObj p;
  gdFontPtr fontPtr;

  if(map->units == -1) {
    msSetError(MS_MISCERR, "Map units not set.", "msDrawScalebar()");
    return(NULL);
  }

  fontPtr = msGetBitmapFont(map->scalebar.label.size);
  if(!fontPtr) return(NULL);

  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  map->scale = msCalculateScale(map->extent, map->units, map->width, map->height);
  
  msx = (map->cellsize * map->scalebar.width)/(inchesPerUnit[map->scalebar.units]/inchesPerUnit[map->units]);
  i = roundInterval(msx/map->scalebar.intervals);
  isx = MS_NINT((i/(inchesPerUnit[map->units]/inchesPerUnit[map->scalebar.units]))/map->cellsize);
  
  switch(map->scalebar.style) {
  case(0):

    sprintf(label, "%g", map->scalebar.intervals*i);
    sx = (2*HMARGIN) + (map->scalebar.intervals*isx) + (1.5 + MS_NINT(strlen(label)/2.0) + (strlen(unitText[map->scalebar.units])))*fontPtr->w;
    sy = (2*VMARGIN) + MS_NINT(VSPACING*fontPtr->h) + fontPtr->h + map->scalebar.height - VSLOP;

    if((img = gdImageCreate(sx, sy)) == NULL) {
      msSetError(MS_GDERR, "Unable to initialize image.", "msDrawScalebar()");
      return(NULL);
    }
    
    if(msLoadPalette(img, &(map->palette), map->scalebar.imagecolor) == -1)  
      return(NULL);
    
    ox = HMARGIN + MS_NINT(fontPtr->w/2.0);
    oy = VMARGIN;

    state = 1; /* 1 means filled */
    for(j=0; j<map->scalebar.intervals; j++) {
      if(state == 1)
	gdImageFilledRectangle(img, ox + j*isx, oy, ox + (j+1)*isx, oy + map->scalebar.height, map->scalebar.color);
      else
	if(map->scalebar.backgroundcolor >= 0) gdImageFilledRectangle(img, ox + j*isx, oy, ox + (j+1)*isx, oy + map->scalebar.height, map->scalebar.backgroundcolor);

      if(map->scalebar.outlinecolor >= 0)
	gdImageRectangle(img, ox + j*isx, oy, ox + (j+1)*isx, oy + map->scalebar.height, map->scalebar.outlinecolor);

      sprintf(label, "%g", j*i);
      map->scalebar.label.position = MS_CC;
      p.x = ox + j*isx;
      p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
      msDrawLabel(img, p, label, &(map->scalebar.label), &(map->fontset));

      state = -state;
    }
    sprintf(label, "%g", j*i);
    ox = ox + j*isx - MS_NINT((strlen(label)*fontPtr->w)/2.0);
    sprintf(label, "%g %s", j*i, unitText[map->scalebar.units]);
    map->scalebar.label.position = MS_CR;
    p.x = ox;
    p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
    msDrawLabel(img, p, label, &(map->scalebar.label), &(map->fontset));
    break;
  case(1):
    sprintf(label, "%g", map->scalebar.intervals*i);
    sx = (2*HMARGIN) + (map->scalebar.intervals*isx) + (1.5 + MS_NINT(strlen(label)/2.0) + (strlen(unitText[map->scalebar.units])))*fontPtr->w;
    sy = (2*VMARGIN) + MS_NINT(VSPACING*fontPtr->h) + fontPtr->h + map->scalebar.height - VSLOP;

    if((img = gdImageCreate(sx, sy)) == NULL) {
      msSetError(MS_GDERR, "Unable to initialize image.", "msDrawScalebar()");
      return(NULL);
    }
    
    if(msLoadPalette(img, &(map->palette), map->scalebar.imagecolor) == -1)  
      return(NULL);
    
    ox = HMARGIN + MS_NINT(fontPtr->w/2.0);
    oy = VMARGIN;
    
    gdImageLine(img, ox, oy, ox + isx*map->scalebar.intervals, oy, map->scalebar.color); /* top line */

    for(j=0; j<map->scalebar.intervals; j++) {

      gdImageLine(img, ox + j*isx, oy, ox + j*isx, oy + map->scalebar.height, map->scalebar.color); /* tick */
      
      sprintf(label, "%g", j*i);
      map->scalebar.label.position = MS_CC;
      p.x = ox + j*isx;
      p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
      msDrawLabel(img, p, label, &(map->scalebar.label), &(map->fontset));

      state = -state;
    }
    
    gdImageLine(img, ox + j*isx, oy, ox + j*isx, oy + map->scalebar.height, map->scalebar.color); /* last tick */

    sprintf(label, "%g", j*i);
    ox = ox + j*isx - MS_NINT((strlen(label)*fontPtr->w)/2.0);
    sprintf(label, "%g %s", j*i, unitText[map->scalebar.units]);
    map->scalebar.label.position = MS_CR;
    p.x = ox;
    p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
    msDrawLabel(img, p, label, &(map->scalebar.label), &(map->fontset));
    break;
  default:
    msSetError(MS_MISCERR, "Unsupported scalebar style.", "msDrawScalebar()");
    return(NULL);
    break;
  }

  return(img);
}

int msEmbedScalebar(mapObj *map, gdImagePtr img)
{
  int s,l;
  pointObj point;

  s = msGetSymbolIndex(&(map->symbolset), "scalebar");
  if(s == -1) {
    s = map->symbolset.numsymbols;
    map->symbolset.numsymbols++;
  } else {
    if(map->symbolset.symbol[s].img) 
      gdImageDestroy(map->symbolset.symbol[s].img);
  }

  map->symbolset.symbol[s].img = msDrawScalebar(map);
  if(!map->symbolset.symbol[s].img) return(-1); // something went wrong creating scalebar

  map->symbolset.symbol[s].type = MS_SYMBOL_PIXMAP;
  map->symbolset.symbol[s].name = strdup("scalebar");  

  if(map->scalebar.transparent)
    gdImageColorTransparent(map->symbolset.symbol[s].img, 0);

  switch(map->scalebar.position) {
  case(MS_LL):
    point.x = MS_NINT(map->symbolset.symbol[s].img->sx/2.0);
    point.y = map->height - MS_NINT(map->symbolset.symbol[s].img->sy/2.0);
    break;
  case(MS_LR):
    point.x = map->width - MS_NINT(map->symbolset.symbol[s].img->sx/2.0);
    point.y = map->height - MS_NINT(map->symbolset.symbol[s].img->sy/2.0);
    break;
  case(MS_LC):
    point.x = MS_NINT(map->width/2.0);
    point.y = map->height - MS_NINT(map->symbolset.symbol[s].img->sy/2.0);
    break;
  case(MS_UR):
    point.x = map->width - MS_NINT(map->symbolset.symbol[s].img->sx/2.0);
    point.y = MS_NINT(map->symbolset.symbol[s].img->sy/2.0);
    break;
  case(MS_UL):
    point.x = MS_NINT(map->symbolset.symbol[s].img->sx/2.0);
    point.y = MS_NINT(map->symbolset.symbol[s].img->sy/2.0);
    break;
  case(MS_UC):
    point.x = MS_NINT(map->width/2.0);
    point.y = MS_NINT(map->symbolset.symbol[s].img->sy/2.0);
    break;
  }

  l = msGetLayerIndex(map, "scalebar");
  if(l == -1) {
    l = map->numlayers;
    map->numlayers++;

    if(initLayer(&(map->layers[l])) == -1) return(-1);
    map->layers[l].name = strdup("scalebar");
    map->layers[l].type = MS_ANNOTATION;
    map->layers[l].status = MS_ON;

    if(initClass(&(map->layers[l].class[0])) == -1) return(-1);    
  }

  map->layers[l].class[0].symbol = s;
  map->layers[l].class[0].color = 0;
  map->layers[l].class[0].label.force = MS_TRUE;

  if(map->scalebar.postlabelcache) // add it directly to the image
    msDrawMarkerSymbol(&map->symbolset, img, &point, map->layers[l].class[0].symbol, 0, -1, -1, 10);
  else
    msAddLabel(map, l, 0, -1, -1, point, " ", -1);

  return(0);
}

#include "map.h"

#define VMARGIN 3 /* buffer around the scalebar */
#define HMARGIN 3
#define VSPACING .8 /* spacing (% of font height) between scalebar and text */
#define VSLOP 5 /* makes things fit a bit better vertically */

/*
** Match this with with unit enumerations is map.h
*/
static char *unitText[5]={"in", "ft", "mi", "m", "km"};
double inchesPerUnit[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};


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
int msCalculateScale(rectObj extent, int units, int width, int height, int resolution, double *scale)
{
  double md, gd;

  // if((extent.maxx == extent.minx) || (extent.maxy == extent.miny))  
  if(!MS_VALID_EXTENT(extent.minx, extent.miny, extent.maxx, extent.maxy)) {
    msSetError(MS_MISCERR, "Invalid image extent.", "msCalculateScale()");
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
  case(MS_INCHES):  
  case(MS_FEET):
    md = width/(resolution*inchesPerUnit[units]); // was (width-1)
    gd = extent.maxx - extent.minx;
    *scale = gd/md;
    break;
  default:
    *scale = -1; // this is not an error
    break;
  }

  return(MS_SUCCESS);
}

#define X_STEP_SIZE 5

//TODO : the will be msDrawScalebarGD
imageObj *msDrawScalebar(mapObj *map)
{
  int status;
  gdImagePtr img=NULL;
  char label[32];
  double i, msx;
  int j;
  int isx, sx, sy, ox, oy, state, dsx;
  pointObj p;
  gdFontPtr fontPtr;
  imageObj      *image = NULL;

  if(map->units == -1) {
    msSetError(MS_MISCERR, "Map units not set.", "msDrawScalebar()");
    return(NULL);
  }

  fontPtr = msGetBitmapFont(map->scalebar.label.size);
  if(!fontPtr) return(NULL);

  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scale);
  if(status != MS_SUCCESS) return(NULL);
  
  dsx = map->scalebar.width - 2*HMARGIN;
  do {
    msx = (map->cellsize * dsx)/(inchesPerUnit[map->scalebar.units]/inchesPerUnit[map->units]);
    i = roundInterval(msx/map->scalebar.intervals);
    sprintf(label, "%g", map->scalebar.intervals*i); // last label
    isx = MS_NINT((i/(inchesPerUnit[map->units]/inchesPerUnit[map->scalebar.units]))/map->cellsize);  
    sx = (map->scalebar.intervals*isx) + MS_NINT((1.5 + strlen(label)/2.0 + strlen(unitText[map->scalebar.units]))*fontPtr->w);

    if(sx <= (map->scalebar.width - 2*HMARGIN)) break; // it will fit

    dsx -= X_STEP_SIZE; // change the desired size in hopes that it will fit in user supplied width
  } while(1);

  sy = (2*VMARGIN) + MS_NINT(VSPACING*fontPtr->h) + fontPtr->h + map->scalebar.height - VSLOP;
  
  //TODO
  
  image = msImageCreateGD(map->scalebar.width, sy, map->outputformat,
                          map->web.imagepath, map->web.imageurl);
  if (image)
      img = image->img.gd;
  else
  {
      msSetError(MS_GDERR, "Unable to initialize image.", "msDrawScalebar()");
    return(NULL);
  }
  //if((img = gdImageCreate(map->scalebar.width, sy)) == NULL) {
  //  msSetError(MS_GDERR, "Unable to initialize image.", "msDrawScalebar()");
  // return(NULL);
  //}
  
  if(msLoadPalette(img, &(map->palette), map->scalebar.imagecolor) == -1)  
    return(NULL);
  
  msImageInitGD( image, map->scalebar.imagecolor );

  ox = MS_NINT((map->scalebar.width - sx)/2.0 + fontPtr->w/2.0); // center the computed scalebar
  oy = VMARGIN;

  switch(map->scalebar.style) {
  case(0):
    
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
      p.x = ox + j*isx; // + MS_NINT(fontPtr->w/2);
      p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
      //TODO
      msDrawLabel(image, p, label, &(map->scalebar.label), &(map->fontset));

      state = -state;
    }
    sprintf(label, "%g", j*i);
    ox = ox + j*isx - MS_NINT((strlen(label)*fontPtr->w)/2.0);
    sprintf(label, "%g %s", j*i, unitText[map->scalebar.units]);
    map->scalebar.label.position = MS_CR;
    p.x = ox; // + MS_NINT(fontPtr->w/2);
    p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
    //TODO
    msDrawLabel(image, p, label, &(map->scalebar.label), &(map->fontset));
    break;

  case(1):

    gdImageLine(img, ox, oy, ox + isx*map->scalebar.intervals, oy, map->scalebar.color); /* top line */

    for(j=0; j<map->scalebar.intervals; j++) {

      gdImageLine(img, ox + j*isx, oy, ox + j*isx, oy + map->scalebar.height, map->scalebar.color); /* tick */
      
      sprintf(label, "%g", j*i);
      map->scalebar.label.position = MS_CC;
      p.x = ox + j*isx; // + MS_NINT(fontPtr->w/2);
      p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
      //TODO
      msDrawLabel(image, p, label, &(map->scalebar.label), &(map->fontset));

      state = -state;
    }
    
    gdImageLine(img, ox + j*isx, oy, ox + j*isx, oy + map->scalebar.height, map->scalebar.color); /* last tick */

    sprintf(label, "%g", j*i);
    ox = ox + j*isx - MS_NINT((strlen(label)*fontPtr->w)/2.0);
    sprintf(label, "%g %s", j*i, unitText[map->scalebar.units]);
    map->scalebar.label.position = MS_CR;
    p.x = ox; // + MS_NINT(fontPtr->w/2);
    p.y = oy + map->scalebar.height + MS_NINT(VSPACING*fontPtr->h);
    //TODO
    msDrawLabel(image, p, label, &(map->scalebar.label), &(map->fontset));
    break;
  default:
    msSetError(MS_MISCERR, "Unsupported scalebar style.", "msDrawScalebar()");
    return(NULL);
    break;
  }

  return(image);
}

int msEmbedScalebar(mapObj *map, gdImagePtr img)
{
  int s,l;
  pointObj point;
  imageObj *image = NULL;

  s = msGetSymbolIndex(&(map->symbolset), "scalebar");
  if(s == -1) {
    s = map->symbolset.numsymbols;
    map->symbolset.numsymbols++;
    initSymbol(&(map->symbolset.symbol[s]));
  } else {
    if(map->symbolset.symbol[s].img) 
      gdImageDestroy(map->symbolset.symbol[s].img);
  }
  
  image = msDrawScalebar(map);
  map->symbolset.symbol[s].img =  image->img.gd; //TODO 
  if(!map->symbolset.symbol[s].img) return(-1); // something went wrong creating scalebar

  map->symbolset.symbol[s].type = MS_SYMBOL_PIXMAP; // intialize a few things
  map->symbolset.symbol[s].name = strdup("scalebar");  

  if(map->scalebar.transparent == MS_ON)
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
    map->layers[l].type = MS_LAYER_ANNOTATION;
    map->layers[l].status = MS_ON;

    if(initClass(&(map->layers[l].class[0])) == -1) return(-1);    
      //Update the layer order list with the layer's index.
    map->layerorder[l] = l;
  }

  map->layers[l].class[0].symbol = s;
  map->layers[l].class[0].color = 0;
  map->layers[l].class[0].label.force = MS_TRUE;
  map->layers[l].class[0].label.size = map->layers[l].class[0].label.sizescaled = MS_MEDIUM; // must set a size to have a valid label definition

  if(map->scalebar.postlabelcache) // add it directly to the image //TODO
    msDrawMarkerSymbolGD(&map->symbolset, img, &point, map->layers[l].class[0].symbol, 0, -1, -1, 10);
  else
    msAddLabel(map, l, 0, -1, -1, point, " ", -1);

  return(0);
}

/**********************************************************************
 * $Id$
 *
 * Name:     mapswf.c
 * Project:  MapServer
 * Language: C
 * Purpose:  Macromedia flash output
 * Author:  DM Solutions Group (assefa@dmsolutions.ca)
 *
 * Note : this api is based on the ming swf library :
 *     http://www.opaque.net/ming/
 *
 **********************************************************************/

#ifdef USE_MING_FLASH

#include <assert.h>
#include "map.h"

static char gszFilename[128];


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
gdImagePtr getTileImageFromSymbol(mapObj *map, symbolSetObj *symbolset, 
                                  int sy, int fc, int bc, int oc, 
                                  double sz, int *bDestroyImage)
{
    symbolObj   *symbol;
    int         i;
    gdPoint     oldpnt,newpnt;
    gdPoint     sPoints[MS_MAXVECTORPOINTS];
    gdImagePtr tile;
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
          font = msLookupHashTable(symbolset->fontset->fonts, 
                                   symbolset->symbol[sy].font);
          if(!font) 
              return NULL;

          if(getCharacterSize(symbol->character, sz, font, &rect) == -1) 
              return NULL;

          x = rect.maxx - rect.minx;
          y = rect.maxy - rect.miny;

          tile = gdImageCreate(x, y);
          if(bc >= 0)
              tile_bg = 
                  gdImageColorAllocate(tile, sBc.red, sBc.green, sBc.blue);
               
          else 
          {
              tile_bg = gdImageColorAllocate(tile, sColor0.red, sColor0.green,
                                             sColor0.blue);
              gdImageColorTransparent(tile,0);
          }    
          tile_fg = gdImageColorAllocate(tile, sFc.red, sFc.green, sFc.blue);
    
          x = -rect.minx;
          y = -rect.miny;

#ifdef USE_GD_TTF
          gdImageStringTTF(tile, bbox, 
                           ((symbol->antialias)?(tile_fg):-(tile_fg)), 
                           font, sz, 0, x, y, symbol->character);
#else
          gdImageStringFT(tile, bbox, 
                          ((symbol->antialias)?(tile_fg):-(tile_fg)), 
                          font, sz, 0, x, y, symbol->character);
#endif    

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
   
          scale = sz/symbol->sizey; // sz ~ height in pixels
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
              tile_bg = gdImageColorAllocate(tile, sBc.red, sBc.green, 
                                             sBc.blue);
          else 
          {
              tile_bg = gdImageColorAllocate(tile, sColor0.red, sColor0.green,
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

          scale = sz/symbol->sizey; // sz ~ height in pixels
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
              tile_bg = gdImageColorAllocate(tile, sBc.red, sBc.green, 
                                             sBc.blue);
          else 
          {
              tile_bg = gdImageColorAllocate(tile, sColor0.red, sColor0.green,
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
}

/************************************************************************/
/*                          SWFShape bitmap2shape                       */
/*                                                                      */
/*      Return a fillef polygon shape using a bitmap.                   */
/************************************************************************/
SWFShape bitmap2shape(unsigned char *data,unsigned long size,int width,
                      int height, byte flags)
{
    SWFShape oShape;
    SWFFill oFill;
    SWFBitmap oBitmap;

    if (!data || width <= 0 || height <= 0)
        return NULL;

    oShape = newSWFShape();
    oBitmap = newSWFBitmap_fromInput(newSWFInput_buffer(data,size));

    oFill = SWFShape_addBitmapFill(oShape, oBitmap, flags);
    //oFill = SWFShape_addBitmapFill(oShape, oBitmap, 0);
                              
    SWFShape_setRightFill(oShape, oFill);

    SWFShape_drawLine(oShape, (float)width, 0.0);
    SWFShape_drawLine(oShape, 0.0, (float)height);
    SWFShape_drawLine(oShape, (float)-width, 0.0);
    SWFShape_drawLine(oShape, 0.0, (float)-height);

    //destroySWFBitmap(oBitmap);

    return oShape;
}


/************************************************************************/
/*                        unsigned char *bitmap2dbl                     */
/*                                                                      */
/*      Coverts a bitmap to dbl bitmap suitable for ming.               */
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
    dbldata[3] = *bytesPerColor == 3 ? 1 : 2;
    dbldata[4] = ((outsize+6) >> 24) & 0xFF;
    dbldata[5] = ((outsize+6) >> 16) & 0xFF;
    dbldata[6] = ((outsize+6) >>  8) & 0xFF;
    dbldata[7] = ((outsize+6) >>  0) & 0xFF;
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
    *p++ = (width>> 0) & 0xFF;
    *p++ = (width >> 8) & 0xFF;
    *p++ = (height >> 0) & 0xFF;
    *p++ = (height >> 8) & 0xFF;
    *p++ = img->colorsTotal - 1;

    for (i=0;i<img->colorsTotal;++i) {
        if (*bytesPerColor == 3) {
            *p++ = img->red[i];
            *p++ = img->green[i];
            *p++ = img->blue[i];
        } else {
            if (i != img->transparent) {
                *p++ = img->red[i];
                *p++ = img->green[i];
                *p++ = img->blue[i];
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
SWFShape gdImage2Shape(gdImagePtr img)
{
    unsigned char *data, *dbldata;
    int size;
    int bytesPerColor;
    SWFShape oShape;

    if (!img)
        return NULL;

    data = gd2bitmap(img, &size, &bytesPerColor);
    dbldata = bitmap2dbl(data,&size,&bytesPerColor);
    oShape = bitmap2shape(dbldata, size, img->sx, img->sy, SWFFILL_SOLID);

    return oShape;
}
    

/************************************************************************/
/*                       SWFButton BuildButtonFromGD                    */
/*                                                                      */
/*      Return a button object using a GD image.                        */
/************************************************************************/
SWFButton BuildButtonFromGD(gdImagePtr img, colorObj *psHighlightColor)
{
    SWFShape oShape;
    SWFButton oButton;

    if (!img)
        return NULL;

    oShape = gdImage2Shape(img);
    
    oButton = newSWFButton();
    SWFButton_addShape(oButton, oShape, 
                       SWFBUTTON_UP | SWFBUTTON_HIT | SWFBUTTON_DOWN);

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

    oShape = newSWFShape();

    if (psFillColor == NULL && psOutlineColor == NULL)
        return NULL;

    if (psOutlineColor)
        SWFShape_setLine(oShape, 0, psOutlineColor->red, 
                         psOutlineColor->green, psOutlineColor->blue, 0xff);
    
    if (psFillColor)
        SWFShape_setRightFill(oShape, 
                              SWFShape_addSolidFill(oShape, psFillColor->red, 
                                                    psFillColor->green, 
                                                    psFillColor->blue, 0xff));

    SWFShape_movePenTo(oShape, (float)(nX-(nWidth/2)), (float)nY);


    //TODO : we should maybe use SWFShape_drawArc(shape, r, 0, 360)

    //Bottom Left
    SWFShape_drawCurveTo(oShape, (float)(nX-(nWidth/2)), 
                         (float)(nY+(nHeight/2)),
                         (float)nX, (float)(nY+(nHeight/2))); 
    //Bottom Right
    SWFShape_drawCurveTo(oShape, (float)(nX + (nWidth/2)), 
                         (float)(nY+(nHeight/2)),
                         (float)(nX+(nWidth/2)), (float)nY);
    //Top Right
     SWFShape_drawCurveTo(oShape, (float)(nX +(nWidth/2)), 
                          (float)(nY-(nHeight/2)),
                          (float)nX, (float)(nY-(nHeight/2)));
     //Top Left
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
                             colorObj *psHightlightColor)
{
    SWFShape oShape;
    SWFButton oButton;

    if (nX < 0 || nY < 0 || nWidth < 0 || nHeight < 0 || 
        (psFillColor == NULL && psOutlineColor == NULL))
        return NULL;
   
        
    oShape = BuildEllipseShape(nX, nY, nWidth, nHeight, psFillColor,
                               psOutlineColor);

    oButton = newSWFButton();
    SWFButton_addShape(oButton, oShape, 
                       SWFBUTTON_UP | SWFBUTTON_HIT | SWFBUTTON_DOWN);

    if (psHightlightColor)
        oShape = BuildEllipseShape(nX, nY, nWidth, nHeight, psHightlightColor,
                                   NULL);
    SWFButton_addShape(oButton, oShape, SWFBUTTON_OVER);
    
    return oButton;
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
    int i = 0;

    if (psFillColor == NULL && psOutlineColor == NULL)
        return NULL;

    if (psFillColor)
    {
        if (psOutlineColor)
            SWFShape_setLine(oShape, 0, psOutlineColor->red, 
                             psOutlineColor->green, psOutlineColor->blue, 0xff);

        SWFShape_setRightFill(oShape, 
                              SWFShape_addSolidFill(oShape, psFillColor->red, 
                                                    psFillColor->green, 
                                                    psFillColor->blue, 0xff));
        /*SWFShape_setRightFill(oShape, 
                              SWFShape_addSolidFill(oShape, 0xFF, 
                                                    0, 
                                                    0, 0xff));*/
    }
    else
        SWFShape_setLine(oShape, 5, psOutlineColor->red, psOutlineColor->green, 
                         psOutlineColor->blue, 0xff);
    

    SWFShape_movePenTo(oShape, (float)adfPoints[0].x, (float)adfPoints[0].y);
    for (i=1; i<nPoints; i++)
    {
        SWFShape_drawLineTo(oShape, (float)adfPoints[i].x, 
                            (float)adfPoints[i].y);
    }
    //close the polygon
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

    SWFShape_setLine(oShape, 0, psColor->red, 
                     psColor->green, psColor->blue, 0xff);


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
                               colorObj *psHighlightColor)
{
    SWFButton b;
    //int bFill = 0;

    b = newSWFButton();

    SWFButton_addShape(b, BuildShape(adfPoints, nPoints,  
                                     psFillColor, psOutlineColor), 
                       SWFBUTTON_UP | SWFBUTTON_HIT | SWFBUTTON_DOWN);

    if (psHighlightColor)
    {
        if(psFillColor)
            SWFButton_addShape(b, BuildShape(adfPoints, nPoints, 
                                             psHighlightColor, NULL), 
                               SWFBUTTON_OVER);
        else if (psOutlineColor)
             SWFButton_addShape(b, BuildShape(adfPoints, nPoints, 
                                              NULL, psHighlightColor), 
                                SWFBUTTON_OVER);
    }
    
    //SWFButton_addShape(b, BuildShape(adfPoints, nPoints, 
    //                                    bFill, psColor), SWFBUTTON_HIT);
    //    SWFButton_addShape(b, BuildShape(adfPoints, nPoints, 
    //                                     bFill, psColor), SWFBUTTON_DOWN);

    
    SWFButton_addAction(b, compileSWFActionCode("_root.MouseEnter();"),
                        SWFBUTTON_MOUSEOVER);


    SWFButton_addAction(b, compileSWFActionCode("_root.text1=\"mouse_out\";"),
                        SWFBUTTON_MOUSEOUT);
  
    //SWFButton_addAction(b, compileSWFActionCode("_root.MouseOut();"),
    //                    SWFBUTTON_MOUSEOUT);

    //SWFButton_addAction(b, compileSWFActionCode("_root.MouseOut();"),
    //                     SWFBUTTON_MOUSEOUT);
    

    return b;
}



/************************************************************************/
/*                       SWFButton   BuildButtonLine                    */
/*                                                                      */
/*      Build a button and add a line shape in it.                      */
/************************************************************************/
SWFButton   BuildButtonLine(gdPoint adfPoints[], int nPoints, 
                            colorObj *psFillColor, 
                            colorObj *psHighlightColor)
{
    SWFButton b;
    //int bFill = 0;

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
    
    //SWFButton_addShape(b, BuildShape(adfPoints, nPoints, 
    //                                    bFill, psColor), SWFBUTTON_HIT);
    //    SWFButton_addShape(b, BuildShape(adfPoints, nPoints, 
    //                                     bFill, psColor), SWFBUTTON_DOWN);

    
    //SWFButton_addAction(b, compileSWFActionCode("_root.MouseEnter();"),
    //                    SWFBUTTON_MOUSEDOWN);


    //SWFButton_addAction(b, compileSWFActionCode("_root.text1=\"bbbbbbb\";"),
    //                    SWFBUTTON_MOUSEDOWN);
  
    //SWFButton_addAction(b, compileSWFActionCode("_root.MouseOut();"),
    //                    SWFBUTTON_MOUSEOUT);

    //SWFButton_addAction(b, compileSWFActionCode("_root.MouseOut();"),
    //                     SWFBUTTON_MOUSEOUT);
    

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

    imageObj *image = NULL;

    assert( strcasecmp(format->driver,"SWF") == 0 );

    image = (imageObj *)calloc(1,sizeof(imageObj));

    image->format = format;
    format->refcount++;

    image->width = width;
    image->height = height;
    image->imagepath = NULL;
    image->imageurl = NULL;
    
    if (imagepath)
    {
        image->imagepath = strdup(imagepath);
    }
    if (imageurl)
    {
        image->imageurl = strdup(imageurl);
    }

    image->img.swf = (SWFObj *)malloc(sizeof(SWFObj));    
    image->img.swf->map = map;

    image->img.swf->nLayerMovies = 0;
    image->img.swf->pasMovies = NULL;
    image->img.swf->nCurrentMovie = -1;

    //initalize main movie
    image->img.swf->sMainMovie = newSWFMovie();
    SWFMovie_setDimension(image->img.swf->sMainMovie, (float)width, 
                          (float)height);

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
    int nTmp = 0;
    if (image && MS_DRIVER_SWF(image->format) )
    {
        image->img.swf->nLayerMovies++;
        nTmp = image->img.swf->nLayerMovies;
        if (!image->img.swf->pasMovies)
        {
            image->img.swf->pasMovies = 
                (SWFMovie *)malloc(sizeof(SWFMovie)*nTmp);
        }
        else
        {
            image->img.swf->pasMovies = 
                (SWFMovie *)realloc(image->img.swf->pasMovies,
                                    sizeof(SWFMovie)*nTmp);
        }

        image->img.swf->nCurrentMovie = nTmp -1;
        image->img.swf->pasMovies[nTmp -1] = newSWFMovie();
    
        SWFMovie_setDimension(image->img.swf->pasMovies[nTmp -1], 
                              (float)image->width, (float)image->height);
        
    }
}

/************************************************************************/
/*                          msDrawMarkerSymbolSWF                       */
/*                                                                      */
/*      Draw symbols in an SWF Movie.                                   */
/************************************************************************/
void msDrawMarkerSymbolSWF(symbolSetObj *symbolset, imageObj *image, 
                           pointObj *p, 
                           int sy, int fc, int bc, int oc, double sz)
{
    symbolObj *symbol;
    int offset_x, offset_y, x, y;
    int j;
    gdPoint mPoints[MS_MAXVECTORPOINTS];

    double scale=1.0;

    rectObj rect;
    char *font=NULL;
    int bbox[8];

    gdImagePtr imgtmp = NULL;

    SWFButton   oButton;
    SWFDisplayItem oDisplay;


    colorObj    sFc;
    colorObj    sBc;
    colorObj    sOc;
    colorObj    sColorHighlightObj;

    colorObj *psFillColor = NULL;
    colorObj *psOutlineColor = NULL;
    
    mapObj *map;
    int nTmp = 0;

    FILE *out;

/* -------------------------------------------------------------------- */
/*      if not SWF, return.                                             */
/* -------------------------------------------------------------------- */
    if (image == NULL || !MS_DRIVER_SWF(image->format) )
        return;

    if(sy > symbolset->numsymbols || sy < 0) /* no such symbol, 0 is OK */
        return;

    //if(fc >= gdImageColorsTotal(img)) /* invalid color, -1 is valid */
    //   return;

    if(sz < 1) /* size too small */
        return;

/* -------------------------------------------------------------------- */
/*      extract the colors.                                             */
/* -------------------------------------------------------------------- */
    map = image->img.swf->map;
    getRgbColor(map, fc, &sFc.red, &sFc.green, &sFc.blue);
    getRgbColor(map, bc, &sBc.red, &sBc.green, &sBc.blue);
    getRgbColor(map, oc, &sOc.red, &sOc.green, &sOc.blue);

    //TODO : this should come from the map file.
    sColorHighlightObj.red = 0xff;
    sColorHighlightObj.green = 0;//0xff;
    sColorHighlightObj.blue = 0;
    
/*
    sFc.red = 0xff;
    sFc.green = 0; 
    sFc.blue = 0;
    
    sOc.red = 0;
    sOc.green = 0; 
    sOc.blue = 0;
*/

/* -------------------------------------------------------------------- */
/*      Render the diffrent type of symbols.                            */
/* -------------------------------------------------------------------- */
    symbol = &(symbolset->symbol[sy]);

    nTmp = image->img.swf->nCurrentMovie;

    switch(symbol->type) 
    {  
/* -------------------------------------------------------------------- */
/*      Symbol : true type.                                             */
/* -------------------------------------------------------------------- */
        case(MS_SYMBOL_TRUETYPE):

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
            font = msLookupHashTable(symbolset->fontset->fonts, symbol->font);
            if(!font) 
                return;
            
            if(getCharacterSize(symbol->character, sz, font, &rect) == -1) 
                return;
            
            x = p->x - (rect.maxx - rect.minx)/2 - rect.minx;
            y = p->y - rect.maxy + (rect.maxy - rect.miny)/2;  

            imgtmp = gdImageCreate((rect.maxx - rect.minx)+2,
                                   (rect.maxy - rect.miny)+2);

#ifdef USE_GD_TTF
            //gdImageStringTTF(img, bbox, ((symbol->antialias)?(fc):-(fc)), 
            //                font, sz, 0, x, y, symbol->character);
             gdImageStringTTF(imgtmp, bbox, ((symbol->antialias)?(fc):-(fc)), 
                              font, sz, 0, 1, 1, symbol->character);
             oButton = BuildButtonFromGD(imgtmp, NULL);
             //oShape = gdImage2Shape(imgtmp);
             oDisplay = SWFMovie_add(image->img.swf->pasMovies[nTmp], 
                                     oButton);
             SWFDisplayItem_moveTo(oDisplay, (float)x, (float)y);
#else
             //gdImageStringFT(img, bbox, ((symbol->antialias)?(fc):-(fc)), 
             //               font, sz, 0, x, y, symbol->character);
            gdImageStringFT(imgtmp, bbox, ((symbol->antialias)?(fc):-(fc)), 
                            font, sz, 0, 1, 1, symbol->character);
            //oShape = gdImage2Shape(imgtmp);
            oButton = BuildButtonFromGD(imgtmp, NULL);
            oDisplay = SWFMovie_add(image->img.swf->pasMovies[nTmp], 
                                     oButton);
            SWFDisplayItem_moveTo(oDisplay, (float)x, (float)y);
#endif

#endif

/* -------------------------------------------------------------------- */
/*      Symbol : pixmap.                                                */
/* -------------------------------------------------------------------- */
        case(MS_SYMBOL_PIXMAP):
            if(sz == 1) 
            { // don't scale
                offset_x = MS_NINT(p->x - .5*symbol->img->sx);
                offset_y = MS_NINT(p->y - .5*symbol->img->sy);
                //gdImageCopy(img, symbol->img, offset_x, offset_y, 0, 0, 
                //           symbol->img->sx, symbol->img->sy);
                oButton = BuildButtonFromGD(symbol->img, NULL);
                
                oDisplay = SWFMovie_add(image->img.swf->pasMovies[nTmp], 
                                        oButton);
                SWFDisplayItem_moveTo(oDisplay, (float)offset_x, 
                                      (float)offset_y);
                
            } 
            else 
            {
                scale = sz/symbol->img->sy;
                offset_x = MS_NINT(p->x - .5*symbol->img->sx*scale);
                offset_y = MS_NINT(p->y - .5*symbol->img->sy*scale);

                imgtmp = gdImageCreate(symbol->img->sx*scale, 
                                       symbol->img->sy*scale);
                
                gdImageCopyResized(imgtmp, symbol->img, 0, 0, 0, 0,
                                   symbol->img->sx*scale, 
                                   symbol->img->sy*scale, symbol->img->sx, 
                                   symbol->img->sy);
            
                out = fopen("e:/tmp/ms_tmp/test.jpg", "wb");
                /* And save the image  -- could also use gdImageJpeg */
                gdImageJpeg(imgtmp, out, 0);

                oButton = BuildButtonFromGD(imgtmp, NULL);
                
                oDisplay = SWFMovie_add(image->img.swf->pasMovies[nTmp], 
                                        oButton);
                SWFDisplayItem_moveTo(oDisplay, (float)offset_x, 
                                      (float)offset_y);

                //gdImageDestroy(imgtmp);
            }
            break;

/* -------------------------------------------------------------------- */
/*      symbol : Ellipse                                                */
/* -------------------------------------------------------------------- */
        case(MS_SYMBOL_ELLIPSE):
 
            scale = sz/symbol->sizey;
            x = MS_NINT(symbol->sizex*scale)+1;
            y = MS_NINT(symbol->sizey*scale)+1;
            offset_x = MS_NINT(p->x - .5*x);
            offset_y = MS_NINT(p->y - .5*x);

            psFillColor = NULL;
            psOutlineColor = NULL;

            if (fc >= 0)
                psFillColor = &sFc;
            if (oc >=0)
                psOutlineColor = &sOc;

            if(oc >= 0) 
            {
                if (!symbol->filled)
                    psFillColor = NULL;
                oButton = 
                    BuildEllipseButton(offset_x, offset_y,
                                       MS_NINT(scale*symbol->points[0].x),
                                       MS_NINT(scale*symbol->points[0].y),
                                       psFillColor, psOutlineColor,
                                       &sColorHighlightObj);
                SWFMovie_add(image->img.swf->pasMovies[nTmp], oButton);
            } 
            else 
            {
                if(fc >= 0) 
                {
                    oButton = 
                        BuildEllipseButton(offset_x, offset_y,
                                           MS_NINT(scale*symbol->points[0].x),
                                           MS_NINT(scale*symbol->points[0].y),
                                           psFillColor, NULL, 
                                           &sColorHighlightObj);

                    SWFMovie_add(image->img.swf->pasMovies[nTmp], oButton);
                }
            }
            break;

/* -------------------------------------------------------------------- */
/*      symbol : Vector.                                                */
/* -------------------------------------------------------------------- */
        case(MS_SYMBOL_VECTOR):

            scale = sz/symbol->sizey;

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

                if (fc >= 0)
                    psFillColor = &sFc;
                if (oc >=0)
                    psOutlineColor = &sOc;
                
                oButton = BuildButtonPolygon(mPoints, symbol->numpoints,  
                                             psFillColor,  psOutlineColor,
                                             &sColorHighlightObj);
                    
                SWFMovie_add(image->img.swf->pasMovies[nTmp], oButton);

            }
/* -------------------------------------------------------------------- */
/*      symbol VECTOR : not filled.                                     */
/* -------------------------------------------------------------------- */
            else  
            {
                if(fc < 0) return;
      
                for(j=0;j < symbol->numpoints;j++) 
                {
                    mPoints[j].x = MS_NINT(scale*symbol->points[j].x + offset_x);
                    mPoints[j].y = MS_NINT(scale*symbol->points[j].y + offset_y);
                }

                psFillColor = &sFc;

                oButton = BuildButtonLine(mPoints, symbol->numpoints,  
                                          psFillColor,
                                          &sColorHighlightObj);
                SWFMovie_add(image->img.swf->pasMovies[nTmp], oButton);

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
SWFShape DrawShapePolyline(shapeObj *p, colorObj *psColor)
{
    int i, j;
    SWFShape oShape;

    if (p && psColor && p->numlines > 0)
    {
        oShape = newSWFShape();
        SWFShape_setLine(oShape, 0, psColor->red, 
                         psColor->green, psColor->blue, 0xff);
        
        

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
                                colorObj *psOutlineColor)
{
    int i, j;
    SWFShape oShape;

    if (p && p->numlines > 0 && (psFillColor !=NULL || psOutlineColor != NULL))
    {
        oShape = newSWFShape();
        if (psOutlineColor)
            SWFShape_setLine(oShape, 0, psOutlineColor->red, 
                             psOutlineColor->green, psOutlineColor->blue, 0xff);
        if (psFillColor)
            SWFShape_setRightFill(oShape,
                                  SWFShape_addSolidFill(oShape, 
                                                        psFillColor->red, 
                                                        psFillColor->green, 
                                                        psFillColor->blue,
                                                        0xff));
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
/*    SWFText DrawText(char *string, int nX, int nY, char *pszFontFile, */
/*                       double dfSize, colorObj *psColor)              */
/*                                                                      */
/*      Draws a text of size dfSize and color psColor at position       */
/*      nX,nY using the font pszFontFile.                               */
/************************************************************************/
SWFText DrawText(char *string, int nX, int nY, char *pszFontFile, 
                 double dfSize, colorObj *psColor)
{
    SWFText    oText = NULL;
    SWFFont     oFont = NULL;

    if (!string || !pszFontFile || !psColor)
        return NULL;

    oFont  = loadSWFFontFromFile(fopen(pszFontFile, "rb"));
    if (oFont)
    {        
        oText = newSWFText();
        SWFText_setFont(oText, oFont);
        SWFText_moveTo(oText, (float)nX, (float)nY);
        SWFText_setColor(oText, psColor->red, psColor->green, psColor->blue, 
                         0xff);
        SWFText_setHeight(oText, (float)dfSize);
        SWFText_addString(oText, string, NULL);
   
        return oText;
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
                         int sy, int fc, int bc, double sz)
{
    colorObj    sFc;
    colorObj    sBc;
    mapObj      *map = NULL;
    int         nTmp = 0;
    SWFShape    oShape;
    
/* -------------------------------------------------------------------- */
/*      if not SWF, return.                                             */
/* -------------------------------------------------------------------- */
    if (image == NULL || !MS_DRIVER_SWF(image->format) )
        return;

    if(p == NULL || p->numlines <= 0)
        return;

    if(sy > symbolset->numsymbols || sy < 0) // no such symbol, 0 is OK
        return;
    
    if(fc < 0)// || (fc >= gdImageColorsTotal(img))) // invalid color
       return;

/* -------------------------------------------------------------------- */
/*      extract the colors.                                             */
/* -------------------------------------------------------------------- */
    map = image->img.swf->map;
    getRgbColor(map, fc, &sFc.red, &sFc.green, &sFc.blue);
    getRgbColor(map, bc, &sBc.red, &sBc.green, &sBc.blue);

    nTmp = image->img.swf->nCurrentMovie;

    //For now just draw lines without symbols.
    if(1)//sy == 0) 
    { 
        // just draw a single width line
        oShape = DrawShapePolyline(p, &sFc);
        SWFMovie_add(image->img.swf->pasMovies[nTmp], oShape);
        return;
    }

    //TODO : lines with symbols
}



/************************************************************************/
/*                           msDrawShadeSymbolSWF                       */
/*                                                                      */
/*      Draw polygon features. Only supports solid filled polgons.      */
/************************************************************************/
void msDrawShadeSymbolSWF(symbolSetObj *symbolset, imageObj *image, 
                          shapeObj *p, int sy, int fc, int bc, int oc, 
                          double sz)
{
    colorObj    sFc;
    colorObj    sBc;
    colorObj    sOc;
    mapObj      *map = NULL;
    SWFShape    oShape;
    int         nTmp = 0;
    colorObj    *psFillColor = NULL;
    colorObj    *psOutlineColor = NULL;
    
    gdImagePtr  tile = NULL;
    //int         bDestroyImage = 0;
    unsigned char *data, *dbldata;
    int         size;
    int         bytesPerColor;

    FILE        *out;

/* -------------------------------------------------------------------- */
/*      if not SWF, return.                                             */
/* -------------------------------------------------------------------- */
    if (image == NULL || !MS_DRIVER_SWF(image->format) )
        return;
    
    if(p == NULL || p->numlines <= 0)
        return;

    if(sy > symbolset->numsymbols || sy < 0) /* no such symbol, 0 is OK */
        return;

    //if(fc >= gdImageColorsTotal(img)) /* invalid color, -1 is valid */
    //    return;

    if(sz < 1) /* size too small */
        return;

/* -------------------------------------------------------------------- */
/*      extract the colors.                                             */
/* -------------------------------------------------------------------- */
    map = image->img.swf->map;
    getRgbColor(map, fc, &sFc.red, &sFc.green, &sFc.blue);
    getRgbColor(map, bc, &sBc.red, &sBc.green, &sBc.blue);
    getRgbColor(map, oc, &sOc.red, &sOc.green, &sOc.blue);

    if (fc > -1)
        psFillColor = &sFc;
    if (oc > -1)
        psOutlineColor = &sOc;

    nTmp = image->img.swf->nCurrentMovie;
    
    if (sy == 0)
    {
        oShape = DrawShapeFilledPolygon(p, psFillColor, psOutlineColor);
        SWFMovie_add(image->img.swf->pasMovies[nTmp], oShape);

        return;
    }

/* ==================================================================== */
/*      The idea is to create the tile image and fill the shape with it.*/
/*                                                                      */
/*      Won't work for now : files created are huge.                    */
/* ==================================================================== */

    //tile = getTileImageFromSymbol(map, symbolset, sy, fc, bc, oc, sz,
    //                              &bDestroyImage);

    if (tile)
    {
        out = fopen("e:/tmp/ms_tmp/tile.jpg", "wb");
                /* And save the image  -- could also use gdImageJpeg */
        gdImageJpeg(tile, out, 95);
    }


    if (tile)
    {
        data = gd2bitmap(tile, &size, &bytesPerColor);
        dbldata = bitmap2dbl(data,&size,&bytesPerColor);
        oShape = bitmap2shape(dbldata, size, tile->sx, tile->sy, 
                              SWFFILL_SOLID);//SWFFILL_TILED_BITMAP);
    }
    else
    {
        if (fc > -1 || oc > -1)
        {
            oShape = DrawShapeFilledPolygon(p, psFillColor, psOutlineColor);
           
        }
    }
    SWFMovie_add(image->img.swf->pasMovies[nTmp], oShape);
}



/************************************************************************/
/*                             int draw_textSWF                         */
/*                                                                      */
/*      Renders a text using FDB fonts. FDB fonts are specific to       */
/*      ming library. Returns 0 on success and -1 on error.             */
/************************************************************************/
int draw_textSWF(imageObj *image, pointObj labelPnt, char *string, 
                labelObj *label, fontSetObj *fontset)
{
    int         x, y;
    char        *font=NULL;
    int         nTmp = 0;

    colorObj    sColor;
    mapObj     *map = NULL;
    SWFText     oText = NULL;
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


    
//    char *error=NULL, *font=NULL;
//    int bbox[8];
//    double angle_radians = MS_DEG_TO_RAD*label->angle;

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

    font = msLookupHashTable(fontset->fonts, label->font);

    if(!font) 
    {
        msSetError(MS_TTFERR, "Requested font (%s) not found.", "draw_textSWF()",
                   label->font);
        return(-1);
    }

/* -------------------------------------------------------------------- */
/*      extract the colors.                                             */
/* -------------------------------------------------------------------- */
    map = image->img.swf->map;
    sColor.red = 0;
    sColor.green = 0;
    sColor.blue = 0;

    if (label->color > -1)
        getRgbColor(map, label->color, &sColor.red, &sColor.green, &sColor.blue);
    else if (label->outlinecolor > -1)
        getRgbColor(map, label->outlinecolor, &sColor.red, &sColor.green, 
                    &sColor.blue);
    else if (label->shadowcolor > -1)
        getRgbColor(map, label->shadowcolor, &sColor.red, &sColor.green, 
                    &sColor.blue);
    else
    {
        msSetError(MS_TTFERR, "Invalid color", "draw_textSWF()");
	return(-1);
    }

    oText = DrawText(string, x, y, font, label->sizescaled, &sColor);
    if (oText)
    {
        nTmp = image->img.swf->nCurrentMovie;
        SWFMovie_add(image->img.swf->pasMovies[nTmp], oText);
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
                      fontSetObj *fontset)
{
    SWFText    oText = NULL;
    SWFFont     oFont = NULL;
    char        *font;
    double      dfWidth;

    if (!string || strlen(string) == 0 || !label || !rect ||
        !fontset)
        return -1;

    font = msLookupHashTable(fontset->fonts, label->font);

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
    
    oFont  = loadSWFFontFromFile(fopen(font, "rb"));
    if (oFont)
    {
        oText = newSWFText();
        SWFText_setFont(oText, oFont);
        //SWFText_addString(oText, string, NULL);
        dfWidth = 0.0;
        dfWidth = (double)SWFText_getStringWidth(oText, string);
        
        if (dfWidth <=0)
            return -1;
    }

    rect->minx = 0;
    rect->miny = 0;
    rect->maxx = dfWidth;
    rect->maxy = label->sizescaled;
    
    return(0);
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
    int i, j, l;

    rectObj r;
  
    labelCacheMemberObj *cachePtr=NULL;
    classObj *classPtr=NULL;
    layerObj *layerPtr=NULL;
    labelObj *labelPtr=NULL;

    int draw_marker;
    int marker_width, marker_height;
    int marker_offset_x, marker_offset_y;
    rectObj marker_rect;
    
    int         nCurrentMovie = 0;

    if (!image || !MS_DRIVER_SWF(image->format))
        return -1;


/* -------------------------------------------------------------------- */
/*      keep the current layer index.                                   */
/* -------------------------------------------------------------------- */
    nCurrentMovie = image->img.swf->nCurrentMovie;

    for(l=map->labelcache.numlabels-1; l>=0; l--) {

        cachePtr = &(map->labelcache.labels[l]); /* point to right spot in cache */

        layerPtr = &(map->layers[cachePtr->layeridx]); /* set a couple of other pointers, avoids nasty references */

/* ==================================================================== */
/*      set the current layer so the label will be drawn in the         */
/*      using the correct SWF handle.                                   */
/* ==================================================================== */
        image->img.swf->nCurrentMovie = cachePtr->layeridx;
        
        classPtr = &(cachePtr->class);
        labelPtr = &(cachePtr->class.label);

        if(!cachePtr->string)
            continue; /* not an error, just don't want to do anything */

        if(strlen(cachePtr->string) == 0)
            continue; /* not an error, just don't want to do anything */

        if(msGetLabelSizeSWF(cachePtr->string, labelPtr, &r, 
                             &(map->fontset)) == -1)
            return(-1);

        if(labelPtr->autominfeaturesize && ((r.maxx-r.minx) > cachePtr->featuresize))
            continue; /* label too large relative to the feature */

        draw_marker = marker_offset_x = marker_offset_y = 0; /* assume no marker */
        if((layerPtr->type == MS_LAYER_ANNOTATION || layerPtr->type == MS_LAYER_POINT) && (classPtr->color >= 0 || classPtr->outlinecolor > 0 || classPtr->symbol > 0)) { // there *is* a marker

            msGetMarkerSize(&map->symbolset, classPtr, &marker_width, &marker_height);
            marker_offset_x = MS_NINT(marker_width/2.0);
            marker_offset_y = MS_NINT(marker_height/2.0);      

            marker_rect.minx = MS_NINT(cachePtr->point.x - .5 * marker_width);
            marker_rect.miny = MS_NINT(cachePtr->point.y - .5 * marker_height);
            marker_rect.maxx = marker_rect.minx + (marker_width-1);
            marker_rect.maxy = marker_rect.miny + (marker_height-1);

            if(layerPtr->type == MS_LAYER_ANNOTATION) draw_marker = 1; /* actually draw the marker */
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

                    if(draw_marker)
                        msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon

                    if(!labelPtr->partials) { // check against image first
                        if(labelInImage(map->width, map->height, cachePtr->poly,
                                        labelPtr->buffer) == MS_FALSE) {
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

                            if((labelPtr->mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= labelPtr->mindistance)) { /* label is a duplicate */
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

                    if(draw_marker)
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

                            if((labelPtr->mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= labelPtr->mindistance)) { /* label is a duplicate */
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

            if(draw_marker)
                msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon, part of overlap tests */

            if(!labelPtr->force) { // no need to check anything else

                if(!labelPtr->partials) {
                    if(labelInImage(map->width, map->height, cachePtr->poly, 
                                    labelPtr->buffer) == MS_FALSE)
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
                        if((labelPtr->mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string, map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= labelPtr->mindistance)) { /* label is a duplicate */
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

        /* msImagePolyline(img, cachePtr->poly, 1); */

        if(!cachePtr->status)
            continue; /* next label */

        if(draw_marker) { /* need to draw a marker */
            msDrawMarkerSymbolSWF(&map->symbolset, image, &(cachePtr->point), classPtr->symbol, classPtr->color, classPtr->backgroundcolor, classPtr->outlinecolor, classPtr->sizescaled);
            if(classPtr->overlaysymbol >= 0) msDrawMarkerSymbolSWF(&map->symbolset, image, &(cachePtr->point), classPtr->overlaysymbol, classPtr->overlaycolor, classPtr->overlaybackgroundcolor, classPtr->overlayoutlinecolor, classPtr->overlaysizescaled);
        }

        //TODO
        //if(labelPtr->backgroundcolor >= 0)
        //    billboard(img, cachePtr->poly, labelPtr);

        draw_textSWF(image, p, cachePtr->string, labelPtr, &(map->fontset)); /* actually draw the label */

    } /* next in cache */

    image->img.swf->nCurrentMovie = nCurrentMovie;

    return(0);
}



/************************************************************************/
/*                              msDrawLabelSWF                          */
/*                                                                      */
/*      Draw a label.                                                   */
/************************************************************************/
int msDrawLabelSWF(imageObj *image, pointObj labelPnt, char *string, 
                   labelObj *label, fontSetObj *fontset)
{
    pointObj p;
    rectObj r;

    if (!image || !MS_DRIVER_SWF(image->format))
        return 0;

    if(!string)
        return(0); /* not an error, just don't want to do anything */

    if(strlen(string) == 0)
        return(0); /* not an error, just don't want to do anything */
    
    msGetLabelSizeSWF(string, label, &r, fontset);
    p = get_metrics(&labelPnt, label->position, r, label->offsetx, 
                    label->offsety, label->angle, 0, NULL);
    //labelPnt.x += label->offsetx;
    //labelPnt.y += label->offsety;
    return draw_textSWF(image, p, string, label, fontset);

}

int msDrawWMSLayerSWF(mapObj *map, layerObj *layer, imageObj *image)
{
    int         nTmp = 0;
    gdImagePtr  imgtmp = NULL;
    SWFShape    oShape;

    if (!image || !MS_DRIVER_SWF(image->format) || image->width <= 0 ||
        image->height <= 0)
        return -1;
    
/* -------------------------------------------------------------------- */
/*      create a temprary GD image and render in it.                    */
/* -------------------------------------------------------------------- */
    imgtmp = gdImageCreate(image->width, image->height);
    gdImageColorAllocate(imgtmp, map->imagecolor.red, map->imagecolor.green, 
                         map->imagecolor.blue);

    if (msDrawWMSLayerGD(map, layer, imgtmp) != -1)
    {
        oShape = gdImage2Shape(imgtmp);
        nTmp = image->img.swf->nCurrentMovie;
        SWFMovie_add(image->img.swf->pasMovies[nTmp], oShape);
    }

    return 0;

}


/************************************************************************/
/*                         int msDrawRasterLayerSWF                     */
/*                                                                      */
/*      Renders a raster layer in a temporary GD image and adds it      */
/*      in the current movie.                                           */
/************************************************************************/
int msDrawRasterLayerSWF(mapObj *map, layerObj *layer, imageObj *image)
{
    int         nTmp = 0;
    gdImagePtr  imgtmp = NULL;
    SWFShape    oShape;

    if (!image || !MS_DRIVER_SWF(image->format) || image->width <= 0 ||
        image->height <= 0)
        return -1;
    
/* -------------------------------------------------------------------- */
/*      create a temprary GD image and render in it.                    */
/* -------------------------------------------------------------------- */
    imgtmp = gdImageCreate(image->width, image->height);

    if (msDrawRasterLayerGD(map, layer, imgtmp) != -1)
    {
        oShape = gdImage2Shape(imgtmp);
        nTmp = image->img.swf->nCurrentMovie;
        SWFMovie_add(image->img.swf->pasMovies[nTmp], oShape);
    }

    return 0;
}

/************************************************************************/
/*                            int msSaveImageSWF                        */
/*                                                                      */
/*      Save to SWF files.                                              */
/************************************************************************/
int msSaveImageSWF(imageObj *image, char *filename)
{
    int         i, nLayers = 0;
    char        szBase[100];
    char        szExt[5];
    char        szTmp[20];
    int         nLength;
    int         iPointPos;
    char        szAction[200];
    SWFAction   oAction;

    if (image && MS_DRIVER_SWF(image->format) && filename)
    {
        
        nLayers = image->img.swf->nLayerMovies;
        
/* -------------------------------------------------------------------- */
/*      extract the name of the file to save and use it to save the     */
/*      layer files. For example for file name /tmp/ms_tmp/test.swf,    */
/*      the first layer name will be /tmp/ms_tmp/test_layer0.swf.       */
/* -------------------------------------------------------------------- */
        nLength = strlen(filename);
        iPointPos = -1;
        for (i=0; i< nLength; i++)
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
            strcpy(szExt, filename+i+1);
        } 
        
        for (i=0; i<nLayers; i++)
        {
            sprintf(szTmp, "%s%d", "_layer_", i);
            gszFilename[0] = '\0';
            sprintf(gszFilename, szBase);
            strcat(gszFilename, szTmp);
            strcat(gszFilename, ".");
            strcat(gszFilename, szExt);

            SWFMovie_setBackground(image->img.swf->pasMovies[i], 
                                   0xff, 0xff, 0xff);
            SWFMovie_save(image->img.swf->pasMovies[i], gszFilename);  
            sprintf(szAction, "loadMovie(\"%s\",%d);", gszFilename, i+1);
            oAction = compileSWFActionCode(szAction);
            SWFMovie_add(image->img.swf->sMainMovie, oAction);
        }

        SWFMovie_save(image->img.swf->sMainMovie, filename);  
  
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
        destroySWFMovie(image->img.swf->sMainMovie);
        for (i=0; i<image->img.swf->nLayerMovies; i++)
            destroySWFMovie(image->img.swf->pasMovies[i]);

        image->img.swf->nLayerMovies = 0;
        image->img.swf->nCurrentMovie = -1;
        image->img.swf->map = NULL;
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

#endif


/*****************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Header files for Ming support (swf flash output) in Mapserver
 * Author:   Assefa Yewondwossen, DM Solutions Group (yassefa@dmsolutions.ca)
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
#include "ming.h"
#endif


/* SWF Object structure */
#ifdef USE_MING_FLASH
   
typedef struct  {
  mapObj *map;
  SWFMovie sMainMovie;
  int nLayerMovies;
  SWFMovie *pasMovies;
  int nCurrentMovie;
  int nCurrentLayerIdx;
  int nCurrentShapeIdx;
  
  SWFFont *Fonts; /* keep tracks of the fonts used to properly free them */
  int nFonts;
  SWFText *Texts; /* keep tracks of the texts used to properly free them */
  int nTexts;
  SWFShape *Shapes; /* keep tracks of the shapes used to properly free them */
  int nShapes;
  SWFButton *Buttons; /* keep tracks of the buttons used to properly free them */
  int nButtons;
  SWFBitmap *Bitmaps; /* keep tracks of the bitmaps used to properly free them */
  int nBitmaps;
  SWFInput *Inputs; /* keep tracks of the input buffers used to properly free them */
  int nInputs;
  unsigned char **DblDatas; /* keep tracks of the DblDatas used to properly free them */
  int nDblDatas;
  
  void    *imagetmp;  /*used when the output format is SINGLE */
                      /*(one movie for the whole map)*/
  int *panLayerIndex; /* keeps the layer index for every movie created. */
  int nTmpCount;
} SWFObj;
#endif

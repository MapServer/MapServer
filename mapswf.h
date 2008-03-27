/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  header file for Ming support (swf flash output) in Mapserver
 * Author:   Assefa Yewondwossen (yassefa@dmsolutions.ca)
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

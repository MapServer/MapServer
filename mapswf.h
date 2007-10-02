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
  
  
  void    *imagetmp;  /*used when the output format is SINGLE */
                      /*(one movie for the whole map)*/
  int *panLayerIndex; /* keeps the layer index for every movie created. */
  int nTmpCount;
} SWFObj;
#endif

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  header file for Ming support (swf flash output) in Mapserver
 * Author:   Assefa Yewondwossen (yassefa@dmsolutions.ca)
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * $Log$
 * Revision 1.1  2005/09/23 21:17:51  assefa
 * Header file for swf file.
 *
 *
 */


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
} SWFObj;
#endif

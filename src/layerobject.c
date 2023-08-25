/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Functions for operating on a layerObj that don't belong in a
 *           more specific file such as mapfile.c.
 *           Adapted from mapobject.c.
 * Author:   Sean Gillies, sgillies@frii.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Sean Gillies
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


/* ===========================================================================
   msInsertClass

   Returns the index at which the class was inserted.
   ======================================================================== */

int msInsertClass(layerObj *layer, classObj *classobj, int nIndex)
{
  int i;

  if (!classobj) {
    msSetError(MS_CHILDERR, "Cannot insert NULL class", "msInsertClass()");
    return -1;
  }

  /* Ensure there is room for a new class */
  if (msGrowLayerClasses(layer) == NULL) {
    return -1;
  }
  /* Catch attempt to insert past end of styles array */
  else if (nIndex >= layer->numclasses) {
    msSetError(MS_CHILDERR, "Cannot insert class beyond index %d",
               "msInsertClass()", layer->numclasses-1);
    return -1;
  } else if (nIndex < 0) { /* Insert at the end by default */
#ifndef __cplusplus
    layer->class[layer->numclasses]=classobj;
#else
    layer->_class[layer->numclasses]=classobj;
#endif
    /* set parent pointer */
    classobj->layer=layer;
    MS_REFCNT_INCR(classobj);
    layer->numclasses++;
    return layer->numclasses-1;
  } else {

    /* Copy classes existing at the specified nIndex or greater */
    /* to an index one higher */

#ifndef __cplusplus
    for (i=layer->numclasses-1; i>=nIndex; i--)
      layer->class[i+1] = layer->class[i];
    layer->class[nIndex]=classobj;
#else
    for (i=layer->numclasses-1; i>=nIndex; i--)
      layer->_class[i+1] = layer->_class[i];
    layer->_class[nIndex]=classobj;
#endif

    /* set parent pointer */
    classobj->layer=layer;
    MS_REFCNT_INCR(classobj);
    /* increment number of classes and return */
    layer->numclasses++;
    return nIndex;
  }
}

/* ===========================================================================
   msRemoveClass

   remove the class at an index from a layer, returning a copy
   ======================================================================== */

classObj *msRemoveClass(layerObj *layer, int nIndex)
{
  int i;
  classObj *classobj;

  if (nIndex < 0 || nIndex >= layer->numclasses) {
    msSetError(MS_CHILDERR, "Cannot remove class, invalid index %d",
               "removeClass()", nIndex);
    return NULL;
  } else {
#ifndef __cplusplus
    classobj=layer->class[nIndex];
#else
    classobj=layer->_class[nIndex];
#endif
    classobj->layer=NULL;
    MS_REFCNT_DECR(classobj);

    /* Iteratively copy the higher index classes down one index */
    for (i=nIndex; i<layer->numclasses-1; i++) {
#ifndef __cplusplus
      layer->class[i]=layer->class[i+1];
#else
      layer->_class[i]=layer->_class[i+1];
#endif
    }
#ifndef __cplusplus
    layer->class[i]=NULL;
#else
    layer->_class[i]=NULL;
#endif

    /* decrement number of layers and return copy of removed layer */
    layer->numclasses--;
    return classobj;
  }
}

/**
 * Move the class up inside the array of classes.
 */
int msMoveClassUp(layerObj *layer, int nClassIndex)
{
  classObj *psTmpClass = NULL;
  if (layer && nClassIndex < layer->numclasses && nClassIndex >0) {
    psTmpClass=layer->class[nClassIndex];

    layer->class[nClassIndex] = layer->class[nClassIndex-1];

    layer->class[nClassIndex-1] = psTmpClass;

    return(MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveClassUp()",
             nClassIndex);
  return (MS_FAILURE);
}


/**
 * Move the class down inside the array of classes.
 */
int msMoveClassDown(layerObj *layer, int nClassIndex)
{
  classObj *psTmpClass = NULL;
  if (layer && nClassIndex < layer->numclasses-1 && nClassIndex >=0) {
    psTmpClass=layer->class[nClassIndex];

    layer->class[nClassIndex] = layer->class[nClassIndex+1];

    layer->class[nClassIndex+1] = psTmpClass;

    return(MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveClassDown()",
             nClassIndex);
  return (MS_FAILURE);
}

/**
 * Set the extent of a layer.
 */

int msLayerSetExtent( layerObj *layer,
                      double minx, double miny, double maxx, double maxy)
{

  layer->extent.minx = minx;
  layer->extent.miny = miny;
  layer->extent.maxx = maxx;
  layer->extent.maxy = maxy;

  if (minx == -1.0 && miny == -1.0 && maxx == -1.0 && maxy == -1.0)
    return(MS_SUCCESS);

  if (!MS_VALID_EXTENT(layer->extent)) {
    msSetError(MS_MISCERR, "Given layer extent is invalid. minx=%lf, miny=%lf, maxx=%lf, maxy=%lf.", "msLayerSetExtent()", layer->extent.minx, layer->extent.miny, layer->extent.maxx, layer->extent.maxy);
    return(MS_FAILURE);
  }

  return(MS_SUCCESS);
}

/******************************************************************************
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
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************
 *
 * $Log$
 * Revision 1.11  2006/09/01 02:30:15  sdlime
 * Dan beat me to the bug 1428 fix. I took a bit futher by removing msLayerGetFilterString() from layerobject.c and refer to that in the mapscript getFilter/getFilterString methods.
 *
 * Revision 1.10  2005/02/18 03:06:44  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.9  2004/10/21 04:30:54  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include "map.h"

#ifdef USE_GDAL
#  include "gdal.h"
#  include "cpl_conv.h"
#endif

MS_CVSID("$Id$")

/* ===========================================================================
   msInsertClass

   Returns the index at which the class was inserted.
   ======================================================================== */
 
int msInsertClass(layerObj *layer, classObj *classobj, int nIndex) 
{
    int i;

    if (!classobj)
    {
        msSetError(MS_CHILDERR, "Cannot insert NULL class", "msInsertClass()");
        return -1;
    }
        
    /* Possible to add another? */
    if (layer->numclasses == MS_MAXCLASSES) {
        msSetError(MS_CHILDERR, "Max number of classes, %d, has been reached",
                   "msInsertClass()", MS_MAXCLASSES);
        return -1;
    }
    /* Catch attempt to insert past end of styles array */
    else if (nIndex >= MS_MAXCLASSES) {
        msSetError(MS_CHILDERR, "Cannot insert class beyond index %d",
                   "msInsertClass()", MS_MAXCLASSES-1);
        return -1;
    }
    else if (nIndex < 0) { /* Insert at the end by default */
#ifndef __cplusplus
        initClass(&(layer->class[layer->numclasses]));
        msCopyClass(&(layer->class[layer->numclasses]), classobj, layer);
#else
        initClass(&(layer->_class[layer->numclasses]));
        msCopyClass(&(layer->_class[layer->numclasses]), classobj, layer);
#endif
        layer->numclasses++;
        return layer->numclasses-1;
    }
    else if (nIndex >= 0 && nIndex < MS_MAXCLASSES) {
    
        /* Copy classes existing at the specified nIndex or greater */
        /* to an index one higher */

#ifndef __cplusplus
        initClass(&(layer->class[layer->numclasses]));
        for (i=layer->numclasses-1; i>=nIndex; i--)
            layer->class[i+1] = layer->class[i];
        initClass(&(layer->class[nIndex]));
        msCopyClass(&(layer->class[nIndex]), classobj, layer);
#else
        initClass(&(layer->_class[layer->numclasses]));
        for (i=layer->numclasses-1; i>=nIndex; i--)
            layer->_class[i+1] = layer->_class[i];
        initClass(&(layer->_class[nIndex]));
        msCopyClass(&(layer->_class[nIndex]), classobj, layer);
#endif

        /* increment number of layers and return */
        layer->numclasses++;
        return nIndex;
    }
    else {
        msSetError(MS_CHILDERR, "Invalid index", "msInsertClass()");
        return -1;
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
    
    if (nIndex < 0 || nIndex >= layer->numclasses)
    {
        msSetError(MS_CHILDERR, "Cannot remove class, invalid index %d",
                   "removeClass()", nIndex);
        return NULL;
    }
    else 
    {
        classobj = (classObj *) malloc(sizeof(classObj));
        if (!classobj) {
            msSetError(MS_MEMERR, 
                       "Failed to allocate classObj to return as removed Class",
                       "msRemoveClass");
            return NULL;
        }
        initClass(classobj);
#ifndef __cplusplus
        msCopyClass(classobj, &(layer->class[nIndex]), NULL);
#else
        msCopyClass(classobj, &(layer->_class[nIndex]), NULL);
#endif

        /* Iteratively copy the higher index classes down one index */
        for (i=nIndex; i<layer->numclasses-1; i++)
        {
#ifndef __cplusplus
            freeClass(&(layer->class[i]));
            initClass(&(layer->class[i]));
            msCopyClass(&layer->class[i], &layer->class[i+1], layer);
#else
            freeClass(&(layer->_class[i]));
            initClass(&(layer->_class[i]));
            msCopyClass(&layer->_class[i], &layer->_class[i+1], layer);
#endif
        }
        /* Free the extra class at the end */
#ifndef __cplusplus
        freeClass(&(layer->class[layer->numclasses-1]));
#else  
        freeClass(&(layer->_class[layer->numclasses-1]));
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
    if (layer && nClassIndex < layer->numclasses && nClassIndex >0)
    {
        psTmpClass = (classObj *)malloc(sizeof(classObj));
        initClass(psTmpClass);
        
        msCopyClass(psTmpClass, &layer->class[nClassIndex], layer);

        msCopyClass(&layer->class[nClassIndex], 
                    &layer->class[nClassIndex-1], layer);
        
        msCopyClass(&layer->class[nClassIndex-1], psTmpClass, layer);

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
    if (layer && nClassIndex < layer->numclasses-1 && nClassIndex >=0)
    {
        psTmpClass = (classObj *)malloc(sizeof(classObj));
        initClass(psTmpClass);
        
        msCopyClass(psTmpClass, &layer->class[nClassIndex], layer);

        msCopyClass(&layer->class[nClassIndex], 
                    &layer->class[nClassIndex+1], layer);
        
        msCopyClass(&layer->class[nClassIndex+1], psTmpClass, layer);

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

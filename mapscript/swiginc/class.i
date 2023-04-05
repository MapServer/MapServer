/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript classObj extensions
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             Seth Girvin

   ===========================================================================
   Copyright (c) 1996-2019 Regents of the University of Minnesota.
   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
 
   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.
 
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ===========================================================================
*/


%extend classObj {

    /// Create a new child classObj instance at the tail (highest index) of the 
    /// class array of the parent_layer. A class can be created outside the 
    /// context of a parent layer by omitting the layerObj constructor argument
    classObj(layerObj *layer=NULL) 
    {
        classObj *new_class=NULL;
        
        if (!layer)
        {
            new_class = (classObj *) malloc(sizeof(classObj));
            if (!new_class)
            {
                msSetError(MS_MEMERR,
                    "Could not allocate memory for new classObj instance",
                    "classObj()");
                return NULL;
            }
            if (initClass(new_class) == -1) return NULL;
            new_class->layer = NULL;
            return new_class;
        }
        else
        {
            if(msGrowLayerClasses(layer) == NULL)
                return NULL;
            if (initClass(layer->class[layer->numclasses]) == -1)
                return NULL;
            layer->class[layer->numclasses]->layer = layer;
            MS_REFCNT_INCR(layer->class[layer->numclasses]);
            layer->numclasses++;
            return (layer->class[layer->numclasses-1]);
        }

        return NULL;
    }

    ~classObj() 
    {
        if (self)
        {
            if (freeClass(self)==MS_SUCCESS) {
                free(self);
                self=NULL;
            }
        }
    }

    /// Update a class from a string snippet. Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int updateFromString(char *snippet)
    {
        return msUpdateClassFromString(self, snippet);
    }

    %newobject convertToString;
    /// Output the CLASS as a Mapfile string
    char* convertToString()
    {
        return msWriteClassToString(self);
    }

#if defined (SWIGJAVA) || defined (SWIGPHP)
    %newobject cloneClass;
    classObj *cloneClass() 
#else
    %newobject clone;
    /**
    Return an independent copy of the class without a parent layer

    .. note::

        In the Java & PHP modules this method is named ``cloneClass``.    
    */ 
    classObj *clone() 
#endif
    {
        classObj *new_class;

        new_class = (classObj *) malloc(sizeof(classObj));
        if (!new_class)
        {
            msSetError(MS_MEMERR,
                "Could not allocate memory for new classObj instance",
                "clone()");
            return NULL;
        }
        if (initClass(new_class) == -1)
        {
            msSetError(MS_MEMERR, "Failed to initialize Class",
                                  "clone()");
            return NULL;
        }
        new_class->layer = NULL;

        if (msCopyClass(new_class, self, self->layer) != MS_SUCCESS) {
            freeClass(new_class);
            free(new_class);
            new_class = NULL;
        }
        
        return new_class;
    }

  /// Set :ref:`EXPRESSION <mapfile-class-expression>` string where `expression` is a MapServer regular, 
  /// logical or string expression. Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
  int setExpression(char *expression) 
  {
    if (!expression || strlen(expression) == 0) {
       msFreeExpression(&self->expression);
       return MS_SUCCESS;
    }
    else return msLoadExpressionString(&self->expression, expression);
  }

  %newobject getExpressionString;
  /// Return a string representation of the :ref:`EXPRESSION <mapfile-class-expression>` enclosed in 
  /// the quote characters appropriate to the expression type
  char *getExpressionString() {
    return msGetExpressionString(&(self->expression));
  }

  /// Set :ref:`TEXT <mapfile-class-text>` string where `text` is a MapServer text expression.
  /// Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
  int setText(char *text) {
    if (!text || strlen(text) == 0) {
      msFreeExpression(&self->text);
      return MS_SUCCESS;
    }    
    else return msLoadExpressionString(&self->text, text);
  }

  %newobject getTextString;
  /// Return a string representation of :ref:`TEXT <mapfile-class-text>`
  char *getTextString() {
    return msGetExpressionString(&(self->text));
  }

  /// Draw the legend icon onto *image* at *dstx*, *dsty*.  Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
  int drawLegendIcon(mapObj *map, layerObj *layer, int width, int height, imageObj *dstImage, int dstX, int dstY) {    
    if(layer->sizeunits != MS_PIXELS) {
      map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
      layer->scalefactor = (msInchesPerUnit(layer->sizeunits,0)/msInchesPerUnit(map->units,0)) / map->cellsize;
    }
    else
      layer->scalefactor = map->resolution/map->defresolution;

  #if defined(WIN32) && defined(SWIGCSHARP)
    __try {
        return msDrawLegendIcon(map, layer, self, width, height, dstImage, dstX, dstY, MS_TRUE, NULL);
    }    
    __except(1 /*EXCEPTION_EXECUTE_HANDLER, catch every exception so it doesn't crash IIS*/) {  
        msSetError(MS_IMGERR, "Unhandled exception in drawing legend image 0x%08x", "msDrawMap()", GetExceptionCode());
    }
  #else    
    return msDrawLegendIcon(map, layer, self, width, height, dstImage, dstX, dstY, MS_TRUE, NULL);
  #endif
  }

  %newobject createLegendIcon;
  /// Draw and return a new legend icon
  imageObj *createLegendIcon(mapObj *map, layerObj *layer, int width, int height) {
    return msCreateLegendIcon(map, layer, self, width, height, MS_TRUE);
  } 

  %newobject getLabel;
  /// Return a reference to the :class:`labelObj` at *index* in the labels array
  labelObj *getLabel(int i) {
    if (i >= 0 && i < self->numlabels) {
      MS_REFCNT_INCR(self->labels[i]);
      return self->labels[i];
    } else {
      msSetError(MS_CHILDERR, "Invalid index: %d.", "getLabel()", i);
      return NULL;
    }
  }

#ifdef SWIGCSHARP
%apply SWIGTYPE *SETREFERENCE {labelObj *label};
#endif
  /// Add a :class:`labelObj` to the :class:`classObj` and return its index in the labels array
  int addLabel(labelObj *label) {
    return msAddLabelToClass(self, label);
  }
#ifdef SWIGCSHARP 
%clear labelObj *label;
#endif

  %newobject removeLabel;
  /// Remove the :class:`labelObj` at *index* from the labels array and return a
  /// reference to the :class:`labelObj`. numlabels is decremented, and the array is updated
  labelObj *removeLabel(int index) {
    labelObj* label = (labelObj *) msRemoveLabelFromClass(self, index);
    if (label) MS_REFCNT_INCR(label);
    return label;
  }
  
  // See https://github.com/mapserver/mapserver/issues/548 for more details about the Style methods

  %newobject getStyle;
  /// Return a reference to the :class:`styleObj` at *index* in the styles array
  styleObj *getStyle(int i) {
    if (i >= 0 && i < self->numstyles) {
      MS_REFCNT_INCR(self->styles[i]);
      return self->styles[i];
    } else {
      msSetError(MS_CHILDERR, "Invalid index: %d", "getStyle()", i);
      return NULL;
    }
  }

#ifdef SWIGCSHARP
%apply SWIGTYPE *SETREFERENCE {styleObj *style};
#endif
  /// Insert a **copy** of *style* into the styles array at index *index*
  /// Default is -1, or the end of the array.  Returns the index at which the style was inserted.
  int insertStyle(styleObj *style, int index=-1) {
    return msInsertStyle(self, style, index);
  }
#ifdef SWIGCSHARP 
%clear styleObj *style;
#endif

  %newobject removeStyle;
  /// Remove the :class:`styleObj` at *index* from the styles array and return a copy.
  styleObj *removeStyle(int index) {
    styleObj* style = (styleObj *) msRemoveStyle(self, index);
    if (style) MS_REFCNT_INCR(style);
    return style;
  }

  /// Swap the :class:`styleObj` at *index* with the styleObj at *index* - 1
  int moveStyleUp(int index) {
    return msMoveStyleUp(self, index);
  }

  /// Swap the :class:`styleObj` at *index* with the :class:`styleObj` at *index* + 1
  int moveStyleDown(int index) {
    return msMoveStyleDown(self, index);
  }

}

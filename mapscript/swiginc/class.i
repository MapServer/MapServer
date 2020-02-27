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

    // manually add parameter here or get mapscript.mapscript.layerObj in output docs
    %feature("autodoc", "classObj.__init__(layerObj layer=None)

Create a new child classObj instance at the tail (highest index) of the 
class array of the parent_layer. A class can be created outside the 
context of a parent layer by omitting the layerObj constructor argument") classObj;

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

    %feature("docstring") updateFromString 
    "Update a class from a string snippet. Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`";
    int updateFromString(char *snippet)
    {
        return msUpdateClassFromString(self, snippet, MS_FALSE);
    }

    %feature("docstring") convertToString 
    "Output the CLASS as a Mapfile string"
    %newobject convertToString;
    char* convertToString()
    {
        return msWriteClassToString(self);
    }

#if defined (SWIGJAVA) || defined (SWIGPHP)
    %newobject cloneClass;
    classObj *cloneClass() 
#else
    %feature("docstring") clone 
    "Return an independent copy of the class without a parent layer"
    %newobject clone;
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

  %feature("docstring") setExpression 
  "Set :mapfile:`EXPRESSION <class.html#index-4>` string where `expression` is a MapServer regular, logical or string expression. 
Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`";
  int setExpression(char *expression) 
  {
    if (!expression || strlen(expression) == 0) {
       msFreeExpression(&self->expression);
       return MS_SUCCESS;
    }
    else return msLoadExpressionString(&self->expression, expression);
  }

  %feature("docstring") getExpressionString 
  "Return a string representation of the :mapfile:`EXPRESSION <class.html#index-4>` enclosed in the quote characters appropriate to the expression type";
  %newobject getExpressionString;
  char *getExpressionString() {
    return msGetExpressionString(&(self->expression));
  }

  %feature("docstring") setText 
  "Set :mapfile:`TEXT <class.html#index-22>` string where `text` is a MapServer text expression.
Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`";
  int setText(char *text) {
    if (!text || strlen(text) == 0) {
      msFreeExpression(&self->text);
      return MS_SUCCESS;
    }    
    else return msLoadExpressionString(&self->text, text);
  }

  %feature("docstring") getTextString 
  "Return a string representation of :mapfile:`TEXT <class.html#index-22>`";
  %newobject getTextString;
  char *getTextString() {
    return msGetExpressionString(&(self->text));
  }

  %feature("docstring") getMetaData 
  "**To be removed in 8.0** -  use the metadata property";
  char *getMetaData(char *name) {
    char *value = NULL;
    if (!name) {
      msSetError(MS_HASHERR, "NULL key", "getMetaData");
    }
     
    value = (char *) msLookupHashTable(&(self->metadata), name);
    if (!value) {
      msSetError(MS_HASHERR, "Key %s does not exist", "getMetaData", name);
      return NULL;
    }
    return value;
  }

  %feature("docstring") setMetaData 
  "**To be removed in 8.0** -  use the metadata property";
  int setMetaData(char *name, char *value) {
    if (msInsertHashTable(&(self->metadata), name, value) == NULL)
        return MS_FAILURE;
    return MS_SUCCESS;
  }

  %feature("docstring") getFirstMetaDataKey 
  "**To be removed in 8.0** -  use the metadata property";
  char *getFirstMetaDataKey() {
    return (char *) msFirstKeyFromHashTable(&(self->metadata));
  }

  %feature("docstring") getNextMetaDataKey 
  "**To be removed in 8.0** -  use the metadata property";
  char *getNextMetaDataKey(char *lastkey) {
    return (char *) msNextKeyFromHashTable(&(self->metadata), lastkey);
  }

  %feature("docstring") drawLegendIcon 
  "Draw the legend icon onto *image* at *dstx*, *dsty*.  Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`";
  int drawLegendIcon(mapObj *map, layerObj *layer, int width, int height, imageObj *dstImage, int dstX, int dstY) {    
    if(layer->sizeunits != MS_PIXELS) {
      map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
      layer->scalefactor = (msInchesPerUnit(layer->sizeunits,0)/msInchesPerUnit(map->units,0)) / map->cellsize;
    }
    else
      layer->scalefactor = map->resolution/map->defresolution;
    
    return msDrawLegendIcon(map, layer, self, width, height, dstImage, dstX, dstY, MS_TRUE, NULL);
  }

  %feature("docstring") createLegendIcon 
  "Draw and return a new legend icon";
  %newobject createLegendIcon;
  imageObj *createLegendIcon(mapObj *map, layerObj *layer, int width, int height) {
    return msCreateLegendIcon(map, layer, self, width, height, MS_TRUE);
  } 

  %feature("docstring") getLabel 
  "Return a reference to the :class:`labelObj` at *index* in the labels array";
  %newobject getLabel;
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
  %feature("docstring") addLabel 
  "Add a :class:`labelObj` to the :class:`classObj` and return its index in the labels array";
  int addLabel(labelObj *label) {
    return msAddLabelToClass(self, label);
  }
#ifdef SWIGCSHARP 
%clear labelObj *label;
#endif

  %feature("docstring") removeLabel 
  "Remove the :class:`labelObj` at *index* from the labels array and return a
reference to the :class:`labelObj`.  numlabels is decremented, and the array is updated";
  %newobject removeLabel;
  labelObj *removeLabel(int index) {
    labelObj* label = (labelObj *) msRemoveLabelFromClass(self, index);
    if (label) MS_REFCNT_INCR(label);
    return label;
  }
  
  /* See Bugzilla issue 548 for more details about the *Style methods */
  %feature("docstring") getStyle 
  "Return a reference to the :class:`styleObj` at *index* in the styles array";
  %newobject getStyle;
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
  %feature("docstring") insertStyle 
  "Insert a **copy** of *style* into the styles array at index *index*.
Default is -1, or the end of the array.  Returns the index at which the style was inserted.";
  int insertStyle(styleObj *style, int index=-1) {
    return msInsertStyle(self, style, index);
  }
#ifdef SWIGCSHARP 
%clear styleObj *style;
#endif

  %feature("docstring") removeStyle 
  "Remove the :class:`styleObj` at *index* from the styles array and return a copy.";
  %newobject removeStyle;
  styleObj *removeStyle(int index) {
    styleObj* style = (styleObj *) msRemoveStyle(self, index);
    if (style) MS_REFCNT_INCR(style);
    return style;
  }

  %feature("docstring") moveStyleUp 
  "Swap the :class:`styleObj` at *index* with the styleObj at *index* - 1";
  int moveStyleUp(int index) {
    return msMoveStyleUp(self, index);
  }

  %feature("docstring") moveStyleDown 
  "Swap the :class:`styleObj` at *index* with the :class:`styleObj` at *index* + 1";
  int moveStyleDown(int index) {
    return msMoveStyleDown(self, index);
  }
}

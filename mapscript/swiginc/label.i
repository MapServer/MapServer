/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript labelObj extensions
   Author:   Steve Lime

   ===========================================================================
   Copyright (c) 1996-2007 Regents of the University of Minnesota.

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

%extend labelObj
{

  /**
  Create a new :class:`labelObj`. A :class:`labelObj` is associated with a 
  :class:`classObj` a :class:`scalebarObj` or a :class:`legendObj`. 
  An instance of :class:`labelObj` can exist outside of a :class:`classObj` container and be 
  explicitly inserted into the :class:`classObj`:
  
  >>> new_label = new labelObj()
  >>> the_class.addLabel(new_label)
  */
  labelObj() 
    {
      labelObj *label;
        
      label = (labelObj *)calloc(1, sizeof(labelObj));
      if (!label)
        return(NULL);
    
      initLabel(label);
      
      return(label);    	
    }

  ~labelObj() 
    {
      freeLabel(self);
    }

  /// Update a :class:`labelObj` from a string snippet. Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
  int updateFromString(char *snippet)
  {
    return msUpdateLabelFromString(self, snippet);
  }

  %newobject convertToString;
  /// Output the :class:`labelObj` object as a Mapfile string. Provides the inverse option for :func:`labelObj.updateFromString`.
  char* convertToString()
  {
    return msWriteLabelToString(self);
  }

  /// Remove the attribute binding for a specified label property.
  int removeBinding(int binding) 
  {
    if(binding < 0 || binding >= MS_LABEL_BINDING_LENGTH) return MS_FAILURE;

    if(self->bindings[binding].item) {
      free(self->bindings[binding].item);
      self->bindings[binding].item = NULL;
      self->bindings[binding].index = -1; 
      self->numbindings--;
    }

    return MS_SUCCESS;
  }

  /// Get the attribute binding for a specified label property. Returns NULL if there is no binding for this property.
  char *getBinding(int binding) 
  {
    if(binding < 0 || binding >= MS_LABEL_BINDING_LENGTH) return NULL;

    return self->bindings[binding].item;
  }

  /// Set the attribute binding for a specified label property. Binding constants look like this: ``MS_LABEL_BINDING_[attribute name]``:
  /// >>> new_label.setBinding(MS_LABEL_BINDING_COLOR, "FIELD_NAME_COLOR")
  int setBinding(int binding, char *item) 
  {
    if(!item) return MS_FAILURE;
    if(binding < 0 || binding >= MS_LABEL_BINDING_LENGTH) return MS_FAILURE;

    if(self->bindings[binding].item) {
      free(self->bindings[binding].item);
      self->bindings[binding].item = NULL; 
      self->bindings[binding].index = -1;
      self->numbindings--;
    }

    self->bindings[binding].item = msStrdup(item); 
    self->numbindings++;

    return MS_SUCCESS;
  }

  /// Set the label expression.
  int setExpression(char *expression) 
  {
    if (!expression || strlen(expression) == 0) {
       msFreeExpression(&self->expression);
       return MS_SUCCESS;
    }
    else return msLoadExpressionString(&self->expression, expression);
  }

  /// Returns the label expression string.
  %newobject getExpressionString;
  char *getExpressionString() {
    return msGetExpressionString(&(self->expression));
  }

  /// Set the label text.
  int setText(char *text) {
    if (!text || strlen(text) == 0) {
      msFreeExpression(&self->text);
      return MS_SUCCESS;
    }	
    else return msLoadExpressionString(&self->text, text);
  }

  /// Returns the label text string.
  %newobject getTextString;
  char *getTextString() {
    return msGetExpressionString(&(self->text));
  }

  %newobject getStyle;
  /// Return a reference to the :class:`styleObj` at index *i* in the styles array.
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

    /// Insert a copy of style into the styles array at index *index*. 
    /// Default is -1, or the end of the array. Returns the index at which the style was inserted.
    int insertStyle(styleObj *style, int index=-1) {
        return msInsertLabelStyle(self, style, index);
    }
#ifdef SWIGCSHARP 
%clear styleObj *style;
#endif

    %newobject removeStyle;
    /// Remove the styleObj at *index* from the styles array and return a copy.
    styleObj *removeStyle(int index) {
        styleObj* style = (styleObj *) msRemoveLabelStyle(self, index);
        if (style)
            MS_REFCNT_INCR(style);
            return style;
    }

    /// Swap the styleObj at *index* with the styleObj index - 1.
    int moveStyleUp(int index) {
        return msMoveLabelStyleUp(self, index);
    }

    /// Swap the styleObj at *index* with the styleObj index + 1.
    int moveStyleDown(int index) {
       return msMoveLabelStyleDown(self, index);
    }
}

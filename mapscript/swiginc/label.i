/* ===========================================================================
   $Id: $

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
  int updateFromString(char *snippet)
  {
    return msUpdateLabelFromString(self, snippet);
  }

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

  char *getBinding(int binding) 
  {
    if(binding < 0 || binding >= MS_LABEL_BINDING_LENGTH) return NULL;

    return self->bindings[binding].item;
  }

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

    self->bindings[binding].item = strdup(item); 
    self->numbindings++;

    return MS_SUCCESS;
  }

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
    int insertStyle(styleObj *style, int index=-1) {
        return msInsertLabelStyle(self, style, index);
    }
#ifdef SWIGCSHARP 
%clear styleObj *style;
#endif

    %newobject removeStyle;
    styleObj *removeStyle(int index) {
	styleObj* style = (styleObj *) msRemoveLabelStyle(self, index);
	if (style)
		MS_REFCNT_INCR(style);
        return style;
    }

    int moveStyleUp(int index) {
        return msMoveLabelStyleUp(self, index);
    }

    int moveStyleDown(int index) {
       return msMoveLabelStyleDown(self, index);
    }
}

/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript classObj extensions
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             
   ===========================================================================
   Copyright (c) 1996-2001 Regents of the University of Minnesota.
   
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
            layer->class[layer->numclasses]->type = layer->type;
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

    int updateFromString(char *snippet)
    {
        return msUpdateClassFromString(self, snippet, MS_FALSE);
    }

#ifdef SWIGJAVA
    %newobject cloneClass;
    classObj *cloneClass() 
#else
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

  int setExpression(char *expression) 
  {
    if (!expression || strlen(expression) == 0) {
       freeExpression(&self->expression);
       return MS_SUCCESS;
    }
    else return msLoadExpressionString(&self->expression, expression);
  }

  %newobject getExpressionString;
  char *getExpressionString() {
    return msGetExpressionString(&(self->expression));
  }

  int setText(char *text) {
    if (!text || strlen(text) == 0) {
      freeExpression(&self->text);
      return MS_SUCCESS;
    }	
    else return msLoadExpressionString(&self->text, text);
  }

  %newobject getTextString;
  char *getTextString() {
    return msGetExpressionString(&(self->text));
  }

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

  int setMetaData(char *name, char *value) {
    if (msInsertHashTable(&(self->metadata), name, value) == NULL)
        return MS_FAILURE;
    return MS_SUCCESS;
  }

  char *getFirstMetaDataKey() {
    return (char *) msFirstKeyFromHashTable(&(self->metadata));
  }
 
  char *getNextMetaDataKey(char *lastkey) {
    return (char *) msNextKeyFromHashTable(&(self->metadata), lastkey);
  }
  
  int drawLegendIcon(mapObj *map, layerObj *layer, int width, int height, imageObj *dstImage, int dstX, int dstY) {
    return msDrawLegendIcon(map, layer, self, width, height, dstImage, dstX, dstY);
  }
 
  %newobject createLegendIcon;
  imageObj *createLegendIcon(mapObj *map, layerObj *layer, int width, int height) {
    return msCreateLegendIcon(map, layer, self, width, height);
  } 

    /* See Bugzilla issue 548 for more details about the *Style methods */
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
        return msInsertStyle(self, style, index);
    }
#ifdef SWIGCSHARP 
%clear styleObj *style;
#endif

    %newobject removeStyle;
    styleObj *removeStyle(int index) {
	styleObj* style = (styleObj *) msRemoveStyle(self, index);
	if (style)
		MS_REFCNT_INCR(style);
        return style;
    }

    int moveStyleUp(int index) {
        return msMoveStyleUp(self, index);
    }

    int moveStyleDown(int index) {
       return msMoveStyleDown(self, index);
    }
}

/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Labeling Implementation.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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

/*
** maplabel.c: Routines to enable text drawing, BITMAP or TRUETYPE.
*/

#include "mapserver.h"




/**
 * replace wrap characters with \n , respecting maximal line length.
 *
 * returns a pointer to the newly allocated text. memory is controlled
 * inside this function, so the caller MUST use the pointer returned by
 * the function:
 * text = msWrapText(label,text);
 *
 * TODO/FIXME: function will produce erroneous/crashing? results
 * if the wrap character is encoded with multiple bytes
 *
 * see http://mapserver.org/development/rfc/ms-rfc-40.html
 * for a summary of how wrap/maxlength interact on the result
 * of the text transformation
 */
char *msWrapText(labelObj *label, char *text)
{
  char wrap;
  int maxlength;
  if(!text) /*not an error if no text*/
    return text;
  wrap = label->wrap;
  maxlength = label->maxlength;
  if(maxlength == 0) {
    if(wrap!='\0') {
      /* if maxlength = 0 *and* a wrap character was specified,
       * replace all wrap characters by \n
       * this is the traditional meaning of the wrap character
       */
      msReplaceChar(text, wrap, '\n');
    }
    /* if neither maxlength, nor wrap were specified,
     * don't transform this text */
    return text;
  } else if(maxlength>0) {
    if(wrap!='\0') {
      /* split input text at the wrap character, only if
       * the current line length is over maxlength */

      /* TODO: check if the wrap character is a valid byte
       * inside a multibyte utf8 glyph. if so, the msCountChars
       * will return an erroneous value */
      int numwrapchars = msCountChars(text,wrap);

      if(numwrapchars > 0) {
        if(label->encoding) {
          /* we have to use utf decoding functions here, so as not to
           * split a text line on a multibyte character */

          int num_cur_glyph_on_line = 0; /*count for the number of glyphs
                                                     on the current line*/
          char *textptr = text;
          char glyph[11]; /*storage for unicode fetching function*/
          int glyphlen = 0; /*size of current glyph in bytes*/
          while((glyphlen = msGetNextGlyph((const char**)&textptr,glyph))>0) {
            num_cur_glyph_on_line++;
            if(*glyph == wrap && num_cur_glyph_on_line>=(maxlength)) {
              /*FIXME (if wrap becomes something other than char):*/
              *(textptr-1)='\n'; /*replace wrap char with a \n*/
              num_cur_glyph_on_line=0; /*reset count*/
            }
          }
        } else {
          int cur_char_on_line = 0;
          char *textptr = text;
          while(*textptr != 0) {
            cur_char_on_line++;
            if(*textptr == wrap && cur_char_on_line>=maxlength) {
              *textptr='\n'; /*replace wrap char with a \n*/
              cur_char_on_line=0; /*reset count*/
            }
            textptr++;
          }
        }
        return text;
      } else {
        /*there are no characters available for wrapping*/
        return text;
      }
    } else {
      /* if no wrap character was specified, but a maxlength was,
       * don't draw this label if it is longer than the specified maxlength*/
      if(msGetNumGlyphs(text)>maxlength) {
        free(text);
        return NULL;
      } else {
        return text;
      }
    }
  } else {
    /* negative maxlength: we split lines unconditionally, i.e. without
    loooking for a wrap character*/
    int numglyphs,numlines;
    maxlength = -maxlength; /* use a positive value*/
    numglyphs = msGetNumGlyphs(text);
    numlines = (numglyphs-1) / maxlength + 1; /*count total number of lines needed
                                            after splitting*/
    if(numlines>1) {
      char *newtext = msSmallMalloc(strlen(text)+numlines+1);
      char *newtextptr = newtext;
      char *textptr = text;
      int glyphlen = 0, num_cur_glyph = 0;
      while((glyphlen = msGetNextGlyph((const char**)&textptr,newtextptr))>0) {
        num_cur_glyph++;
        newtextptr += glyphlen;
        if(num_cur_glyph%maxlength == 0 && num_cur_glyph != numglyphs) {
          /*we're at a split location, insert a newline*/
          *newtextptr = '\n';
          newtextptr++;
        }
      }
      free(text);
      return newtext;
    } else {
      /*no splitting needed, return the original*/
      return text;
    }
  }
}

char *msAlignText(mapObj *map, labelObj *label, char *text)
{
  double spacewidth=0.0; /*size of a single space, in fractional pixels*/
  int numlines;
  char **textlines,*newtext,*newtextptr;
  int *textlinelengths,*numspacesforpadding;
  int numspacestoadd,maxlinelength,i;
  rectObj label_rect;
  if(!msCountChars(text,'\n'))
    return text; /*only one line*/

  /*split text into individual lines
   * TODO: check if splitting on \n is utf8 safe*/
  textlines = msStringSplit(text,'\n',&numlines);

  /*
   * label->space_size_10 contains a cache for the horizontal size of a single
   * 'space' character, at size 10 for the current label
   * FIXME: in case of attribute binding for the FONT of the label, this cache will
   * be wrong for labels where the attributed font is different than the first
   * computed font. This shouldn't happen too often, and hopefully the size of a
   * space character shouldn't vary too much between different fonts*/
  if(label->space_size_10 == 0.0) {
    /*if the cache hasn't been initialized yet, or with pixmap fonts*/

    /* compute the size of 16 adjacent spaces. we can't do this for just one space,
     * as the labelSize computing functions return integer bounding boxes. we assume
     * that the integer rounding for such a number of spaces will be negligeable
     * compared to the actual size of thoses spaces */
    if(msGetLabelSize(map,label,".              .",10.0,&label_rect,NULL) != MS_SUCCESS) {
      /*error computing label size, we can't continue*/

      /*free the previously allocated split text*/
      while(numlines--)
        free(textlines[numlines]);
      free(textlines);
      return text;
    }

    /* this is the size of a single space character. for truetype fonts,
     * it's the size of a 10pt space. For pixmap fonts, it's the size
     * for the current label */
    spacewidth = (label_rect.maxx-label_rect.minx)/16.0;
    if(label->type == MS_TRUETYPE) {
      label->space_size_10=spacewidth; /*cache the computed size*/

      /*size of a space for current label size*/
      spacewidth = spacewidth * (double)label->size/10.0;
    }
  } else {
    spacewidth = label->space_size_10 * (double)label->size/10.0;
  }
  spacewidth = MS_MAX(1,spacewidth);


  /*length in pixels of each line*/
  textlinelengths = (int*)msSmallMalloc(numlines*sizeof(int));

  /*number of spaces that need to be added to each line*/
  numspacesforpadding = (int*)msSmallMalloc(numlines*sizeof(int));

  /*total number of spaces that need to be added*/
  numspacestoadd=0;

  /*length in pixels of the longest line*/
  maxlinelength=0;
  for(i=0; i<numlines; i++) {
    msGetLabelSize(map,label,textlines[i],label->size, &label_rect,NULL);
    textlinelengths[i] = label_rect.maxx-label_rect.minx;
    if(maxlinelength<textlinelengths[i])
      maxlinelength=textlinelengths[i];
  }
  for(i=0; i<numlines; i++) {
    /* number of spaces to add so the current line is
     * as long as the longest line */
    double nfracspaces = (maxlinelength - textlinelengths[i])/spacewidth;

    if(label->align == MS_ALIGN_CENTER) {
      numspacesforpadding[i]=MS_NINT(nfracspaces/2.0);
    } else {
      if(label->align == MS_ALIGN_RIGHT) {
        numspacesforpadding[i]=MS_NINT(nfracspaces);
      }
    }
    numspacestoadd+=numspacesforpadding[i];
  }

  /*allocate new text with room for the additional spaces needed*/
  newtext = (char*)msSmallMalloc(strlen(text)+1+numspacestoadd);
  newtextptr=newtext;
  for(i=0; i<numlines; i++) {
    int j;
    /*padd beginning of line with needed spaces*/
    for(j=0; j<numspacesforpadding[i]; j++) {
      *(newtextptr++)=' ';
    }
    /*copy original line*/
    strcpy(newtextptr,textlines[i]);
    /*place pointer at the char right after the current line*/
    newtextptr+=strlen(textlines[i])+1;
    if(i!=numlines-1) {
      /*put the \n back in (was taken away by msStringSplit)*/
      *(newtextptr-1)='\n';
    }
  }
  /*free the original text*/
  free(text);
  for(i=0; i<numlines; i++) {
    free(textlines[i]);
  }
  free(textlines);
  free(textlinelengths);
  free(numspacesforpadding);

  /*return the aligned text. note that the terminating \0 was added by the last
   * call to strcpy */
  return newtext;
}


/*
 * this function applies the label encoding and wrap parameters
 * to the supplied text
 * Note: it is the caller's responsibility to free the returned
 * char array
 */
char *msTransformLabelText(mapObj *map, labelObj *label, char *text)
{
  char *newtext = text;
  if(label->encoding)
    newtext = msGetEncodedString(text, label->encoding);
  else
    newtext=msStrdup(text);

  if(newtext && (label->wrap!='\0' || label->maxlength!=0)) {
    newtext = msWrapText(label, newtext);
  }

  if(newtext && label->align!=MS_ALIGN_LEFT ) {
    newtext = msAlignText(map, label, newtext);
  }

  return newtext;
}

int msAddLabelGroup(mapObj *map, int layerindex, int classindex, shapeObj *shape, pointObj *point, double featuresize)
{
  int i, priority, numactivelabels=0;
  labelCacheSlotObj *cacheslot;

  labelCacheMemberObj *cachePtr=NULL;
  layerObj *layerPtr=NULL;
  classObj *classPtr=NULL;

  layerPtr = (GET_LAYER(map, layerindex)); /* set up a few pointers for clarity */
  classPtr = GET_LAYER(map, layerindex)->class[classindex];

  if(classPtr->numlabels == 0) return MS_SUCCESS; /* not an error just nothing to do */
  for(i=0; i<classPtr->numlabels; i++) {
    if(classPtr->labels[i]->status == MS_ON) {
      numactivelabels++;
    }
  }
  if(numactivelabels == 0) return MS_SUCCESS;

  /* if the number of labels is 1 then call msAddLabel() accordingly */
  if(numactivelabels == 1) {
    for(i=0; i<classPtr->numlabels; i++) {
      if(classPtr->labels[i]->status == MS_ON)
        return msAddLabel(map, classPtr->labels[i], layerindex, classindex, shape, point, NULL, featuresize);
    }
  }

  if (layerPtr->type == MS_LAYER_ANNOTATION && (cachePtr->numlabels > 1 || classPtr->leader.maxdistance)) {
    msSetError(MS_MISCERR, "Multiple Labels and/or LEADERs are not supported with annotation layers", "msAddLabelGroup()");
    return MS_FAILURE;
  }

  /* check that the label intersects the layer mask */
  if(layerPtr->mask) {
    int maskLayerIdx = msGetLayerIndex(map,layerPtr->mask);
    layerObj *maskLayer = GET_LAYER(map,maskLayerIdx);
    unsigned char *alphapixptr;
    if(maskLayer->maskimage && MS_IMAGE_RENDERER(maskLayer->maskimage)->supports_pixel_buffer) {
      rasterBufferObj rb;
      int x,y;
      memset(&rb,0,sizeof(rasterBufferObj));
      MS_IMAGE_RENDERER(maskLayer->maskimage)->getRasterBufferHandle(maskLayer->maskimage,&rb);
      x = MS_NINT(point->x);
      y = MS_NINT(point->y);
#ifdef USE_GD
      if(rb.type == MS_BUFFER_BYTE_RGBA) {
        alphapixptr = rb.data.rgba.a+rb.data.rgba.row_step*y + rb.data.rgba.pixel_step*x;
        if(!*alphapixptr) {
          /* label point does not intersect mask */
          return MS_SUCCESS;
        }
      } else {
        if(!gdImageGetPixel(rb.data.gd_img,x,y))
          return MS_SUCCESS;
      }
#else
      assert(rb.type == MS_BUFFER_BYTE_RGBA);
      alphapixptr = rb.data.rgba.a+rb.data.rgba.row_step*y + rb.data.rgba.pixel_step*x;
      if(!*alphapixptr) {
        /* label point does not intersect mask */
        return MS_SUCCESS;
      }
#endif
    } else {
      msSetError(MS_MISCERR, "Layer (%s) references references a mask layer, but the selected renderer does not support them", "msAddLabelGroup()", layerPtr->name);
      return (MS_FAILURE);
    }
  }



  /* Validate label priority value and get ref on label cache for it */
  priority = classPtr->labels[0]->priority; /* take priority from the first label */
  if (priority < 1)
    priority = 1;
  else if (priority > MS_MAX_LABEL_PRIORITY)
    priority = MS_MAX_LABEL_PRIORITY;

  cacheslot = &(map->labelcache.slots[priority-1]);

  if(cacheslot->numlabels == cacheslot->cachesize) { /* just add it to the end */
    cacheslot->labels = (labelCacheMemberObj *) realloc(cacheslot->labels, sizeof(labelCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
    MS_CHECK_ALLOC(cacheslot->labels, sizeof(labelCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT), MS_FAILURE);
    cacheslot->cachesize += MS_LABELCACHEINCREMENT;
  }

  cachePtr = &(cacheslot->labels[cacheslot->numlabels]);

  cachePtr->layerindex = layerindex; /* so we can get back to this *raw* data if necessary */
  cachePtr->classindex = classindex;
  if(shape) {
    cachePtr->shapetype = shape->type;
  } else {
    cachePtr->shapetype = MS_SHAPE_POINT;
  }

  cachePtr->point = *point; /* the actual label point */
  cachePtr->labelpath = NULL;

  cachePtr->leaderline = NULL;
  cachePtr->leaderbbox = NULL;

  // cachePtr->text = msStrdup(string); /* the actual text */

  /* TODO: perhaps we can get rid of this next section and just store a marker size? Why do we cache the styles for a point layer? */

  /* copy the styles (only if there is an accompanying marker)
   * We cannot simply keep refs because the rendering code  might alters some members of the style objects
   */
  cachePtr->styles = NULL;
  cachePtr->numstyles = 0;
  if(layerPtr->type == MS_LAYER_ANNOTATION && classPtr->numstyles > 0) {
    cachePtr->numstyles = classPtr->numstyles;
    cachePtr->styles = (styleObj *) msSmallMalloc(sizeof(styleObj)*classPtr->numstyles);
    if (classPtr->numstyles > 0) {
      for(i=0; i<classPtr->numstyles; i++) {
        initStyle(&(cachePtr->styles[i]));
        msCopyStyle(&(cachePtr->styles[i]), classPtr->styles[i]);
      }
    }
  }

  /*
  ** copy the labels (we are guaranteed to have more than one):
  **   we cannot simply keep refs because the rendering code alters some members of the style objects
  */

  cachePtr->numlabels = 0;
  cachePtr->labels = (labelObj *) msSmallMalloc(sizeof(labelObj)*numactivelabels);
  for(i=0; i<classPtr->numlabels; i++) {
    if(classPtr->labels[i]->status == MS_OFF) continue;
    initLabel(&(cachePtr->labels[cachePtr->numlabels]));
    msCopyLabel(&(cachePtr->labels[cachePtr->numlabels]), classPtr->labels[i]);
    cachePtr->numlabels++;
  }
  assert(cachePtr->numlabels == numactivelabels);

  cachePtr->markerid = -1;

  cachePtr->featuresize = featuresize;

  //cachePtr->poly = (shapeObj *) msSmallMalloc(sizeof(shapeObj));
  //msInitShape(cachePtr->poly);
  cachePtr->poly = NULL;

  cachePtr->status = MS_FALSE;

  if(layerPtr->type == MS_LAYER_POINT && classPtr->numstyles > 0) {
    /* cache the marker placement, it's already on the map */
    /* TO DO: at the moment only checks the bottom style, perhaps should check all of them */
    /* #2347: after RFC-24 classPtr->styles could be NULL so we check it */
    rectObj rect;
    double w, h;
    if(msGetMarkerSize(&map->symbolset, classPtr->styles[0], &w, &h, layerPtr->scalefactor) != MS_SUCCESS)
      return(MS_FAILURE);

    if(cacheslot->nummarkers == cacheslot->markercachesize) { /* just add it to the end */
      cacheslot->markers = (markerCacheMemberObj *) realloc(cacheslot->markers, sizeof(markerCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
      MS_CHECK_ALLOC(cacheslot->markers, sizeof(markerCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT), MS_FAILURE);
      cacheslot->markercachesize+=MS_LABELCACHEINCREMENT;
    }

    i = cacheslot->nummarkers;

    cacheslot->markers[i].poly = (shapeObj *) msSmallMalloc(sizeof(shapeObj));
    msInitShape(cacheslot->markers[i].poly);

    rect.minx = (point->x - .5 * w);
    rect.miny = (point->y - .5 * h);
    rect.maxx = rect.minx + (w-1);
    rect.maxy = rect.miny + (h-1);
    msRectToPolygon(rect, cacheslot->markers[i].poly);
    cacheslot->markers[i].id = cacheslot->numlabels;

    cachePtr->markerid = i;

    cacheslot->nummarkers++;
  }

  cacheslot->numlabels++;

  /* Maintain main labelCacheObj.numlabels only for backwards compatibility */
  map->labelcache.numlabels++;

  return(MS_SUCCESS);
}

int msAddLabel(mapObj *map, labelObj *label, int layerindex, int classindex, shapeObj *shape, pointObj *point, labelPathObj *labelpath, double featuresize)
{
  int i;
  labelCacheSlotObj *cacheslot;

  labelCacheMemberObj *cachePtr=NULL;
  layerObj *layerPtr=NULL;
  classObj *classPtr=NULL;

  if(!label) return(MS_FAILURE); // RFC 77 TODO: set a proper message
  if(label->status == MS_OFF) return(MS_SUCCESS); /* not an error */
  if(!label->annotext) {
    /* check if we have a labelpnt style */
    for(i=0; i<label->numstyles; i++) {
      if(label->styles[i]->_geomtransform.type == MS_GEOMTRANSFORM_LABELPOINT)
        break;
    }
    if(i==label->numstyles) {
      /* label has no text or marker symbols */
      return MS_SUCCESS;
    }
  }

  layerPtr = (GET_LAYER(map, layerindex)); /* set up a few pointers for clarity */
  classPtr = GET_LAYER(map, layerindex)->class[classindex];

  if(classPtr->leader.maxdistance) {
    if (layerPtr->type == MS_LAYER_ANNOTATION) {
      msSetError(MS_MISCERR, "LEADERs are not supported on annotation layers", "msAddLabel()");
      return MS_FAILURE;
    }
    if(labelpath) {
      msSetError(MS_MISCERR, "LEADERs are not supported on ANGLE FOLLOW labels", "msAddLabel()");
      return MS_FAILURE;
    }
  }
  /* check that the label intersects the layer mask */

  if (layerPtr->mask) {
    int maskLayerIdx = msGetLayerIndex(map, layerPtr->mask);
    layerObj *maskLayer = GET_LAYER(map, maskLayerIdx);
    unsigned char *alphapixptr;
    if (maskLayer->maskimage && MS_IMAGE_RENDERER(maskLayer->maskimage)->supports_pixel_buffer) {
      rasterBufferObj rb;
      memset(&rb, 0, sizeof (rasterBufferObj));
      MS_IMAGE_RENDERER(maskLayer->maskimage)->getRasterBufferHandle(maskLayer->maskimage, &rb);
      if (point) {
        int x = MS_NINT(point->x);
        int y = MS_NINT(point->y);
#ifdef USE_GD
        if(rb.type == MS_BUFFER_BYTE_RGBA) {
          alphapixptr = rb.data.rgba.a+rb.data.rgba.row_step*y + rb.data.rgba.pixel_step*x;
          if(!*alphapixptr) {
            /* label point does not intersect mask */
            return MS_SUCCESS;
          }
        } else {
          if(!gdImageGetPixel(rb.data.gd_img,x,y)) {
            return MS_SUCCESS;
          }
        }
#else
        assert(rb.type == MS_BUFFER_BYTE_RGBA);
        alphapixptr = rb.data.rgba.a+rb.data.rgba.row_step*y + rb.data.rgba.pixel_step*x;
        if(!*alphapixptr) {
          /* label point does not intersect mask */
          return MS_SUCCESS;
        }
#endif
      } else if (labelpath) {
        int i = 0;
        for (i = 0; i < labelpath->path.numpoints; i++) {
          int x = MS_NINT(labelpath->path.point[i].x);
          int y = MS_NINT(labelpath->path.point[i].y);
#ifdef USE_GD
          if (rb.type == MS_BUFFER_BYTE_RGBA) {
            alphapixptr = rb.data.rgba.a + rb.data.rgba.row_step * y + rb.data.rgba.pixel_step*x;
            if (!*alphapixptr) {
              /* label point does not intersect mask */
              msFreeLabelPathObj(labelpath);
              return MS_SUCCESS;
            }
          } else {
            if (!gdImageGetPixel(rb.data.gd_img, x, y)) {
              msFreeLabelPathObj(labelpath);
              return MS_SUCCESS;
            }
          }
#else
          assert(rb.type == MS_BUFFER_BYTE_RGBA);
          alphapixptr = rb.data.rgba.a + rb.data.rgba.row_step * y + rb.data.rgba.pixel_step*x;
          if (!*alphapixptr) {
            /* label point does not intersect mask */
            msFreeLabelPathObj(labelpath);
            return MS_SUCCESS;
          }
#endif
        }
      }
    } else {
      msSetError(MS_MISCERR, "Layer (%s) references references a mask layer, but the selected renderer does not support them", "msAddLabel()", layerPtr->name);
      return (MS_FAILURE);
    }
  }



  /* Validate label priority value and get ref on label cache for it */
  if (label->priority < 1)
    label->priority = 1;
  else if (label->priority > MS_MAX_LABEL_PRIORITY)
    label->priority = MS_MAX_LABEL_PRIORITY;

  cacheslot = &(map->labelcache.slots[label->priority-1]);

  if(cacheslot->numlabels == cacheslot->cachesize) { /* just add it to the end */
    cacheslot->labels = (labelCacheMemberObj *) realloc(cacheslot->labels, sizeof(labelCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
    MS_CHECK_ALLOC(cacheslot->labels, sizeof(labelCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT), MS_FAILURE);
    cacheslot->cachesize += MS_LABELCACHEINCREMENT;
  }

  cachePtr = &(cacheslot->labels[cacheslot->numlabels]);

  cachePtr->layerindex = layerindex; /* so we can get back to this *raw* data if necessary */
  cachePtr->classindex = classindex;
  if(shape) {
    cachePtr->shapetype = shape->type;
  } else {
    cachePtr->shapetype = MS_SHAPE_POINT;
  }
  cachePtr->leaderline = NULL;
  cachePtr->leaderbbox = NULL;

  /* Store the label point or the label path (Bug #1620) */
  if ( point ) {
    cachePtr->point = *point; /* the actual label point */
    cachePtr->labelpath = NULL;
  } else {
    assert(labelpath);
    cachePtr->labelpath = labelpath;
    /* Use the middle point of the labelpath for mindistance calculations */
    cachePtr->point = labelpath->path.point[labelpath->path.numpoints / 2];
  }

  /* TODO: perhaps we can get rid of this next section and just store a marker size? Why do we cache the styles for a point layer? */

  /* copy the styles (only if there is an accompanying marker)
   * We cannot simply keeep refs because the rendering code alters some members of the style objects
   */
  cachePtr->styles = NULL;
  cachePtr->numstyles = 0;
  if(layerPtr->type == MS_LAYER_ANNOTATION && classPtr->numstyles > 0) {
    cachePtr->numstyles = classPtr->numstyles;
    cachePtr->styles = (styleObj *) msSmallMalloc(sizeof(styleObj)*classPtr->numstyles);
    if (classPtr->numstyles > 0) {
      for(i=0; i<classPtr->numstyles; i++) {
        initStyle(&(cachePtr->styles[i]));
        msCopyStyle(&(cachePtr->styles[i]), classPtr->styles[i]);
      }
    }
  }

  /* copy the label */
  cachePtr->numlabels = 1;
  cachePtr->labels = (labelObj *) msSmallMalloc(sizeof(labelObj));
  initLabel(cachePtr->labels);
  msCopyLabel(cachePtr->labels, label);

  cachePtr->markerid = -1;

  cachePtr->featuresize = featuresize;

  //cachePtr->poly = (shapeObj *) msSmallMalloc(sizeof(shapeObj));
  //msInitShape(cachePtr->poly);
  cachePtr->poly = NULL;

  cachePtr->status = MS_FALSE;

  if(layerPtr->type == MS_LAYER_POINT && classPtr->numstyles > 0) { /* cache the marker placement, it's already on the map */
    rectObj rect;
    double w, h;

    if(cacheslot->nummarkers == cacheslot->markercachesize) { /* just add it to the end */
      cacheslot->markers = (markerCacheMemberObj *) realloc(cacheslot->markers, sizeof(markerCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
      MS_CHECK_ALLOC(cacheslot->markers, sizeof(markerCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT), MS_FAILURE);
      cacheslot->markercachesize+=MS_LABELCACHEINCREMENT;
    }

    i = cacheslot->nummarkers;

    cacheslot->markers[i].poly = (shapeObj *) msSmallMalloc(sizeof(shapeObj));
    msInitShape(cacheslot->markers[i].poly);

    /* TO DO: at the moment only checks the bottom style, perhaps should check all of them */
    /* #2347: after RFC-24 classPtr->styles could be NULL so we check it */
    if(classPtr->styles != NULL) {
      if(msGetMarkerSize(&map->symbolset, classPtr->styles[0], &w, &h, layerPtr->scalefactor) != MS_SUCCESS)
        return(MS_FAILURE);
      rect.minx = point->x - .5 * w;
      rect.miny = point->y - .5 * h;
      rect.maxx = rect.minx + (w-1);
      rect.maxy = rect.miny + (h-1);
      msRectToPolygon(rect, cacheslot->markers[i].poly);
      cacheslot->markers[i].id = cacheslot->numlabels;

      cachePtr->markerid = i;

      cacheslot->nummarkers++;
    }
  }

  cacheslot->numlabels++;

  /* Maintain main labelCacheObj.numlabels only for backwards compatibility */
  map->labelcache.numlabels++;

  return(MS_SUCCESS);
}

/*
** Is a label completely in the image, reserving a gutter (in pixels) inside
** image for no labels (effectively making image larger. The gutter can be
** negative in cases where a label has a buffer around it.
*/
static int labelInImage(int width, int height, shapeObj *lpoly, int gutter)
{
  int i,j;

  /* do a bbox test first */
  if(lpoly->bounds.minx >= gutter &&
      lpoly->bounds.miny >= gutter &&
      lpoly->bounds.maxx < width-gutter &&
      lpoly->bounds.maxy < height-gutter) {
    return MS_TRUE;
  }

  for(i=0; i<lpoly->numlines; i++) {
    for(j=1; j<lpoly->line[i].numpoints; j++) {
      if(lpoly->line[i].point[j].x < gutter) return(MS_FALSE);
      if(lpoly->line[i].point[j].x >= width-gutter) return(MS_FALSE);
      if(lpoly->line[i].point[j].y < gutter) return(MS_FALSE);
      if(lpoly->line[i].point[j].y >= height-gutter) return(MS_FALSE);
    }
  }

  return(MS_TRUE);
}

/* msTestLabelCacheCollisions()
**
** Compares current label against labels already drawn and markers from cache and discards it
** by setting cachePtr->status=MS_FALSE if it is a duplicate, collides with another label,
** or collides with a marker.
**
** This function is used by the various msDrawLabelCacheXX() implementations.

int msTestLabelCacheCollisions(labelCacheObj *labelcache, labelObj *labelPtr,
                                int mapwidth, int mapheight, int buffer,
                                labelCacheMemberObj *cachePtr, int current_priority,
                                int current_label, int mindistance, double label_size);
*/

int msTestLabelCacheCollisions(mapObj *map, labelCacheMemberObj *cachePtr, shapeObj *poly,
                               int mindistance, int current_priority, int current_label)
{
  labelCacheObj *labelcache = &(map->labelcache);
  int i, p, ll, pp;
  double label_width = 0;
  labelCacheMemberObj *curCachePtr=NULL;

  /*
   * Check against image bounds first
   */
  if(!cachePtr->labels[0].partials) {
    if(labelInImage(map->width, map->height, poly, labelcache->gutter) == MS_FALSE) {
      return MS_FALSE;
    }
  }

  /* compute start index of first label to test: only test against rendered labels */
  if(current_label>=0) {
    i = current_label+1;
  } else {
    i = 0;
    current_label = -current_label;
  }

  /* Compare against all rendered markers from this priority level and higher.
  ** Labels can overlap their own marker and markers from lower priority levels
  */
  for (p=current_priority; p < MS_MAX_LABEL_PRIORITY; p++) {
    labelCacheSlotObj *markerslot;
    markerslot = &(labelcache->slots[p]);

    for ( ll = 0; ll < markerslot->nummarkers; ll++ ) {
      if ( !(p == current_priority && current_label == markerslot->markers[ll].id ) ) {  /* labels can overlap their own marker */
        if ( intersectLabelPolygons(markerslot->markers[ll].poly, poly ) == MS_TRUE ) {
          return MS_FALSE;
        }
      }
    }
  }

  if(mindistance > 0)
    label_width = poly->bounds.maxx - poly->bounds.minx;

  for(p=current_priority; p<MS_MAX_LABEL_PRIORITY; p++) {
    labelCacheSlotObj *cacheslot;
    cacheslot = &(labelcache->slots[p]);

    for(  ; i < cacheslot->numlabels; i++) {
      curCachePtr = &(cacheslot->labels[i]);

      if(curCachePtr->status == MS_TRUE) { /* compare bounding polygons and check for duplicates */

        /* skip testing against ourself */
        assert(p!=current_priority || i != current_label);

        /*
        ** Note 1: We add the label_size to the mindistance value when comparing because we do want the mindistance
        ** value between the labels and not only from point to point.
        **
        ** Note 2: We only check the first label (could be multiples (RFC 77)) since that is *by far* the most common
        ** use case. Could change in the future but it's not worth the overhead at this point.
        */
        if(mindistance >0  &&
            (cachePtr->layerindex == curCachePtr->layerindex) &&
            (cachePtr->classindex == curCachePtr->classindex) &&
            (cachePtr->labels[0].annotext && curCachePtr->labels[0].annotext &&
             strcmp(cachePtr->labels[0].annotext, curCachePtr->labels[0].annotext) == 0) &&
            (msDistancePointToPoint(&(cachePtr->point), &(curCachePtr->point)) <= (mindistance + label_width))) { /* label is a duplicate */
          return MS_FALSE;
        }

        if(intersectLabelPolygons(curCachePtr->poly, poly) == MS_TRUE) { /* polys intersect */
          return MS_FALSE;
        }
        if(curCachePtr->leaderline) {
          /* our poly against rendered leader lines */
          /* first do a bbox check */
          if(msRectOverlap(curCachePtr->leaderbbox, &(poly->bounds))) {
            /* look for intersecting line segments */
            for(ll=0; ll<poly->numlines; ll++)
              for(pp=1; pp<poly->line[ll].numpoints; pp++)
                if(msIntersectSegments(
                      &(poly->line[ll].point[pp-1]),
                      &(poly->line[ll].point[pp]),
                      &(curCachePtr->leaderline->point[0]),
                      &(curCachePtr->leaderline->point[1])) ==  MS_TRUE) {
                  return(MS_FALSE);
                }
          }

        }
        if(cachePtr->leaderline) {
          /* does our leader intersect current label */
          /* first do a bbox check */
          if(msRectOverlap(cachePtr->leaderbbox, &(curCachePtr->poly->bounds))) {
            /* look for intersecting line segments */
            for(ll=0; ll<curCachePtr->poly->numlines; ll++)
              for(pp=1; pp<curCachePtr->poly->line[ll].numpoints; pp++)
                if(msIntersectSegments(
                      &(curCachePtr->poly->line[ll].point[pp-1]),
                      &(curCachePtr->poly->line[ll].point[pp]),
                      &(cachePtr->leaderline->point[0]),
                      &(cachePtr->leaderline->point[1])) ==  MS_TRUE) {
                  return(MS_FALSE);
                }

          }
          if(curCachePtr->leaderline) {
            /* TODO: check intersection of leader lines, not only bbox test ? */
            if(msRectOverlap(curCachePtr->leaderbbox, cachePtr->leaderbbox)) {
              return MS_FALSE;
            }

          }
        }
      }
    } /* i */

    i = 0; /* Start over with 1st label of next slot */
  } /* p */
  return MS_TRUE;
}

/* msGetLabelCacheMember()
**
** Returns label cache members by index, making all members of the cache
** appear as if they were stored in a single array indexed from 0 to numlabels.
**
** Returns NULL if requested index is out of range.
*/
labelCacheMemberObj *msGetLabelCacheMember(labelCacheObj *labelcache, int i)
{
  if(i >= 0 && i < labelcache->numlabels) {
    int p;
    for(p=0; p<MS_MAX_LABEL_PRIORITY; p++) {
      if (i < labelcache->slots[p].numlabels )
        return &(labelcache->slots[p].labels[i]); /* Found it */
      else
        i -= labelcache->slots[p].numlabels; /* Check next slots */
    }
  }

  /* Out of range / not found */
  return NULL;
}


int msInitFontSet(fontSetObj *fontset)
{
  fontset->filename = NULL;

  /* fontset->fonts = NULL; */
  initHashTable(&(fontset->fonts));

  fontset->numfonts = 0;
  fontset->map = NULL;
  return( 0 );
}

int msFreeFontSet(fontSetObj *fontset)
{
  if (fontset->filename)
    free(fontset->filename);
  fontset->filename = NULL;
  if (&(fontset->fonts))
    msFreeHashItems(&(fontset->fonts));
  /* fontset->fonts = NULL; */
  fontset->numfonts = 0;

  return( 0 );
}

int msLoadFontSet(fontSetObj *fontset, mapObj *map)
{
  FILE *stream;
  char buffer[MS_BUFFER_LENGTH];
  char alias[64], file1[MS_PATH_LENGTH], file2[MS_PATH_LENGTH];
  char *path;
  char szPath[MS_MAXPATHLEN];
  int i;
  int bFullPath = 0;

  if(fontset->numfonts != 0) /* already initialized */
    return(0);

  if(!fontset->filename)
    return(0);

  fontset->map = (mapObj *)map;

  path = msGetPath(fontset->filename);

  /* fontset->fonts = msCreateHashTable(); // create font hash */
  /* if(!fontset->fonts) { */
  /* msSetError(MS_HASHERR, "Error initializing font hash.", "msLoadFontSet()"); */
  /* return(-1); */
  /* } */

  stream = fopen( msBuildPath(szPath, fontset->map->mappath, fontset->filename), "r");
  if(!stream) {
    msSetError(MS_IOERR, "Error opening fontset %s.", "msLoadFontset()",
               fontset->filename);
    return(-1);
  }

  i = 0;
  while(fgets(buffer, MS_BUFFER_LENGTH, stream)) { /* while there's something to load */

    if(buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == ' ')
      continue; /* skip comments and blank lines */

    sscanf(buffer,"%s %s", alias,  file1);

    if (!(*file1) || !(*alias) || (strlen(file1) <= 0))
      continue;

    bFullPath = 0;
#if defined(_WIN32) && !defined(__CYGWIN__)
    if (file1[0] == '\\' || (strlen(file1) > 1 && (file1[1] == ':')))
      bFullPath = 1;
#else
    if(file1[0] == '/')
      bFullPath = 1;
#endif

    if(bFullPath) { /* already full path */
      msInsertHashTable(&(fontset->fonts), alias, file1);
    } else {
      snprintf(file2, sizeof(file2), "%s%s", path, file1);
      /* msInsertHashTable(fontset->fonts, alias, file2); */

      /*
      ** msBuildPath is use here, but if we have to save the fontset file
      ** the msBuildPath must be done everywhere the fonts are used and
      ** removed here.
      */
      msInsertHashTable(&(fontset->fonts), alias,
                        msBuildPath(szPath, fontset->map->mappath, file2));

    }

    i++;
  }

  fontset->numfonts = i;
  fclose(stream); /* close the file */
  free(path);

  return(0);
}

int msGetTruetypeTextBBox(rendererVTableObj *renderer, char* fontstring, fontSetObj *fontset,
                          double size, char *string, rectObj *rect, double **advances, int bAdjustbaseline)
{
  outputFormatObj *format = NULL;
  int ret = MS_FAILURE;
  char *lookedUpFonts[MS_MAX_LABEL_FONTS];
  int numfonts;
  if(!renderer) {
    outputFormatObj *format = msCreateDefaultOutputFormat(NULL,"AGG/PNG","tmp");
    if(!format) {
      goto tt_cleanup;
    }
    msInitializeRendererVTable(format);
    renderer = format->vtable;
  }
  if(MS_FAILURE == msFontsetLookupFonts(fontstring, &numfonts, fontset, lookedUpFonts))
    goto tt_cleanup;
  ret = renderer->getTruetypeTextBBox(renderer,lookedUpFonts,numfonts,size,string,rect,advances,bAdjustbaseline);
tt_cleanup:
  if(format) {
    msFreeOutputFormat(format);
  }
  return ret;
}

int msGetRasterTextBBox(rendererVTableObj *renderer, int size, char *string, rectObj *rect)
{
  if(renderer && renderer->supports_bitmap_fonts) {
    int num_lines=1, cur_line_length=0, max_line_length=0;
    char *stringPtr = string;
    fontMetrics *fontPtr;
    if((fontPtr=renderer->bitmapFontMetrics[size]) == NULL) {
      msSetError(MS_MISCERR, "selected renderer does not support bitmap font size %d", "msGetRasterTextBBox()",size);
      return MS_FAILURE;
    }
    while(*stringPtr) {
      if(*stringPtr=='\n') {
        max_line_length = MS_MAX(cur_line_length,max_line_length);
        num_lines++;
        cur_line_length=0;
      } else {
        if(*stringPtr!='\r')
          cur_line_length++;
      }
      stringPtr++;
    }
    max_line_length = MS_MAX(cur_line_length,max_line_length);
    rect->minx = 0;
    rect->miny = -fontPtr->charHeight;
    rect->maxx = fontPtr->charWidth * max_line_length;
    rect->maxy = fontPtr->charHeight * (num_lines-1);
    return MS_SUCCESS;
  } else if(!renderer) {
    int ret = MS_FAILURE;
    outputFormatObj *format = msCreateDefaultOutputFormat(NULL,"AGG/PNG","tmp");
    if(!format) {
      msSetError(MS_MISCERR, "failed to create default format", "msGetRasterTextBBox()");
      return MS_FAILURE;
    }
    msInitializeRendererVTable(format);
    renderer = format->vtable;
    ret = msGetRasterTextBBox(renderer,size,string,rect);
    msFreeOutputFormat(format);
    return ret;
  } else {
    msSetError(MS_MISCERR, "selected renderer does not support raster fonts", "msGetRasterTextBBox()");
    return MS_FAILURE;
  }
}


char *msFontsetLookupFont(fontSetObj *fontset, char *fontKey)
{
  char *font;
  if(!fontKey) {
    msSetError(MS_TTFERR, "Requested font (NULL) not found.", "msFontsetLookupFont()");
    return NULL;
  }
  font = msLookupHashTable(&(fontset->fonts), fontKey);
  if(!font) {
    msSetError(MS_TTFERR, "Requested font (%s) not found.", "msFontsetLookupFont()", fontKey);
    return NULL;
  }
  return font;
}

int msFontsetLookupFonts(char* fontstring, int *numfonts, fontSetObj *fontset, char **lookedUpFonts)
{
  char *start,*ptr;
  *numfonts = 0;
  start = ptr = fontstring;
  while(*numfonts<MS_MAX_LABEL_FONTS) {
    if(*ptr==',') {
      if(start==ptr) { /*first char is a comma, or two successive commas*/
        start = ++ptr;
        continue;
      }
      *ptr = 0;
      lookedUpFonts[*numfonts] = msLookupHashTable(&(fontset->fonts), start);
      *ptr = ',';
      if (!lookedUpFonts[*numfonts]) {
        msSetError(MS_TTFERR, "Requested font (%s) not found.","msFontsetLookupFonts()", fontstring);
        return MS_FAILURE;
      }
      start = ++ptr;
      (*numfonts)++;
    } else if(*ptr==0) {
      if(start==ptr) { /* last char of string was a comma */
        return MS_SUCCESS;
      }
      lookedUpFonts[*numfonts] = msLookupHashTable(&(fontset->fonts), start);
      if (!lookedUpFonts[*numfonts]) {
        msSetError(MS_TTFERR, "Requested font (%s) not found.","msFontsetLookupFonts()", fontstring);
        return (MS_FAILURE);
      }
      (*numfonts)++;
      return MS_SUCCESS;
    } else {
      ptr++;
    }
  }
  msSetError(MS_TTFERR, "Requested font (%s) not has too many members (max is %d)",
             "msFontsetLookupFonts()", fontstring, MS_MAX_LABEL_FONTS);
  return MS_FAILURE;
}


/*
** Note: All these routines assume a reference point at the LL corner of the text. GD's
** bitmapped fonts use UL and this is compensated for. Note the rect is relative to the
** LL corner of the text to be rendered, this is first line for TrueType fonts.
*/

/* assumes an angle of 0 regardless of what's in the label object */
int msGetLabelSize(mapObj *map, labelObj *label, char *string, double size, rectObj *rect, double **advances)
{
  rendererVTableObj *renderer = NULL;

  if (map)
    renderer =MS_MAP_RENDERER(map);

  if(label->type == MS_TRUETYPE) {
    if(!label->font) {
      msSetError(MS_MISCERR, "label has no true type font", "msGetLabelSize()");
      return MS_FAILURE;
    }
    return msGetTruetypeTextBBox(renderer,label->font,&(map->fontset),size,string,rect,advances,1);
  } else if(label->type == MS_BITMAP) {
    if(renderer->supports_bitmap_fonts)
      return msGetRasterTextBBox(renderer,MS_NINT(label->size),string,rect);
    else {
      msSetError(MS_MISCERR, "renderer does not support bitmap fonts", "msGetLabelSize()");
      return MS_FAILURE;
    }
  } else {
    msSetError(MS_MISCERR, "unknown label type", "msGetLabelSize()");
    return MS_FAILURE;
  }
}

#define MARKER_SLOP 2

/*pointObj get_metrics_line(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, lineObj *poly)
 * called by get_metrics and drawLabelCache
 * the poly lineObj MUST have at least 5 points allocated in poly->point
 * it should also probably have poly->numpoints set to 5 */
pointObj get_metrics_line(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, lineObj *poly)
{
  pointObj q;
  double x1=0, y1=0, x2=0, y2=0;
  double sin_a,cos_a;
  double w, h, x, y;

  w = rect.maxx - rect.minx;
  h = rect.maxy - rect.miny;

  switch(position) {
    case MS_UL:
      x1 = -w - ox;
      y1 = -oy;
      break;
    case MS_UC:
      x1 = -(w/2.0);
      y1 = -oy - MARKER_SLOP;
      break;
    case MS_UR:
      x1 = ox;
      y1 = -oy;
      break;
    case MS_CL:
      x1 = -w - ox - MARKER_SLOP;
      y1 = (h/2.0);
      break;
    case MS_CC:
      x1 = -(w/2.0) + ox;
      y1 = (h/2.0) + oy;
      break;
    case MS_CR:
      x1 = ox + MARKER_SLOP;
      y1 = (h/2.0);
      break;
    case MS_LL:
      x1 = -w - ox;
      y1 = h + oy;
      break;
    case MS_LC:
      x1 = -(w/2.0);
      y1 = h + oy + MARKER_SLOP;
      break;
    case MS_LR:
      x1 = ox;
      y1 = h + oy;
      break;
  }

  sin_a = sin(MS_DEG_TO_RAD*angle);
  cos_a = cos(MS_DEG_TO_RAD*angle);

  x = x1 - rect.minx;
  y = rect.maxy - y1;
  q.x = p->x + (x * cos_a - (y) * sin_a);
  q.y = p->y - (x * sin_a + (y) * cos_a);

  if(poly) {
    /*
     * here we should/could have a test asserting that the poly lineObj
     * has at least 5 points available.
     */

    x2 = x1 - buffer; /* ll */
    y2 = y1 + buffer;
    poly->point[0].x = p->x + (x2 * cos_a - (-y2) * sin_a);
    poly->point[0].y = p->y - (x2 * sin_a + (-y2) * cos_a);

    x2 = x1 - buffer; /* ul */
    y2 = y1 - h - buffer;
    poly->point[1].x = p->x + (x2 * cos_a - (-y2) * sin_a);
    poly->point[1].y = p->y - (x2 * sin_a + (-y2) * cos_a);

    x2 = x1 + w + buffer; /* ur */
    y2 = y1 - h - buffer;
    poly->point[2].x = p->x + (x2 * cos_a - (-y2) * sin_a);
    poly->point[2].y = p->y - (x2 * sin_a + (-y2) * cos_a);

    x2 = x1 + w + buffer; /* lr */
    y2 = y1 + buffer;
    poly->point[3].x = p->x + (x2 * cos_a - (-y2) * sin_a);
    poly->point[3].y = p->y - (x2 * sin_a + (-y2) * cos_a);

    poly->point[4].x = poly->point[0].x;
    poly->point[4].y = poly->point[0].y;
  }

  return(q);
}


/* static pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly) */
pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly)
{
  lineObj newline;
  pointObj newpoints[5];
  pointObj rp;
  newline.numpoints=5;
  newline.point=newpoints;
  rp = get_metrics_line(p, position, rect, ox,oy, angle, buffer, &newline);
  if(poly) {
    msAddLine(poly,&newline);
    msComputeBounds(poly);
  }
  return rp;
}

/*
** Variation on msIntersectPolygons. Label polygons aren't like shapefile polygons. They
** have no holes, and often do have overlapping parts (i.e. road symbols).
*/

/* static int intersectLabelPolygons(shapeObj *p1, shapeObj *p2) { */
int intersectLabelPolygons(shapeObj *p1, shapeObj *p2)
{
  int c1,v1,c2,v2;
  pointObj *point;

  /* STEP 0: check bounding boxes */
  if(!msRectOverlap(&p1->bounds, &p2->bounds)) { /* from alans@wunderground.com */
    return(MS_FALSE);
  }

  /* STEP 1: look for intersecting line segments */
  for(c1=0; c1<p1->numlines; c1++)
    for(v1=1; v1<p1->line[c1].numpoints; v1++)
      for(c2=0; c2<p2->numlines; c2++)
        for(v2=1; v2<p2->line[c2].numpoints; v2++)
          if(msIntersectSegments(&(p1->line[c1].point[v1-1]), &(p1->line[c1].point[v1]), &(p2->line[c2].point[v2-1]), &(p2->line[c2].point[v2])) ==  MS_TRUE) {
            return(MS_TRUE);
          }

  /* STEP 2: polygon one completely contains two (only need to check one point from each part) */
  for(c2=0; c2<p2->numlines; c2++) {
    point = &(p2->line[c2].point[0]);
    for(c1=0; c1<p1->numlines; c1++) {
      if(msPointInPolygon(point, &p1->line[c1]) == MS_TRUE) /* ok, the point is in a polygon */
        return(MS_TRUE);
    }
  }

  /* STEP 3: polygon two completely contains one (only need to check one point from each part) */
  for(c1=0; c1<p1->numlines; c1++) {
    point = &(p1->line[c1].point[0]);
    for(c2=0; c2<p2->numlines; c2++) {
      if(msPointInPolygon(point, &p2->line[c2]) == MS_TRUE) /* ok, the point is in a polygon */
        return(MS_TRUE);
    }
  }

  return(MS_FALSE);
}

/* For MapScript, exactly the same the msInsertStyle */
int msInsertLabelStyle(labelObj *label, styleObj *style, int nStyleIndex)
{
  int i;

  if (!style) {
    msSetError(MS_CHILDERR, "Can't insert a NULL Style", "msInsertLabelStyle()");
    return -1;
  }

  /* Ensure there is room for a new style */
  if (msGrowLabelStyles(label) == NULL) {
    return -1;
  }
  /* Catch attempt to insert past end of styles array */
  else if (nStyleIndex >= label->numstyles) {
    msSetError(MS_CHILDERR, "Cannot insert style beyond index %d", "insertLabelStyle()", label->numstyles-1);
    return -1;
  } else if (nStyleIndex < 0) { /* Insert at the end by default */
    label->styles[label->numstyles]=style;
    MS_REFCNT_INCR(style);
    label->numstyles++;
    return label->numstyles-1;
  } else if (nStyleIndex >= 0 && nStyleIndex < label->numstyles) {
    /* Move styles existing at the specified nStyleIndex or greater */
    /* to a higher nStyleIndex */
    for (i=label->numstyles-1; i>=nStyleIndex; i--) {
      label->styles[i+1] = label->styles[i];
    }
    label->styles[nStyleIndex]=style;
    MS_REFCNT_INCR(style);
    label->numstyles++;
    return nStyleIndex;
  } else {
    msSetError(MS_CHILDERR, "Invalid nStyleIndex", "insertLabelStyle()");
    return -1;
  }
}

/**
 * Move the style up inside the array of styles.
 */
int msMoveLabelStyleUp(labelObj *label, int nStyleIndex)
{
  styleObj *psTmpStyle = NULL;
  if (label && nStyleIndex < label->numstyles && nStyleIndex >0) {
    psTmpStyle = (styleObj *)malloc(sizeof(styleObj));
    initStyle(psTmpStyle);

    msCopyStyle(psTmpStyle, label->styles[nStyleIndex]);

    msCopyStyle(label->styles[nStyleIndex],
                label->styles[nStyleIndex-1]);

    msCopyStyle(label->styles[nStyleIndex-1], psTmpStyle);

    return(MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveLabelStyleUp()",
             nStyleIndex);
  return (MS_FAILURE);
}


/**
 * Move the style down inside the array of styles.
 */
int msMoveLabelStyleDown(labelObj *label, int nStyleIndex)
{
  styleObj *psTmpStyle = NULL;

  if (label && nStyleIndex < label->numstyles-1 && nStyleIndex >=0) {
    psTmpStyle = (styleObj *)malloc(sizeof(styleObj));
    initStyle(psTmpStyle);

    msCopyStyle(psTmpStyle, label->styles[nStyleIndex]);

    msCopyStyle(label->styles[nStyleIndex],
                label->styles[nStyleIndex+1]);

    msCopyStyle(label->styles[nStyleIndex+1], psTmpStyle);

    return(MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveLabelStyleDown()",
             nStyleIndex);
  return (MS_FAILURE);
}

/**
 * Delete the style identified by the index and shift
 * styles that follows the deleted style.
 */
int msDeleteLabelStyle(labelObj *label, int nStyleIndex)
{
  int i = 0;
  if (label && nStyleIndex < label->numstyles && nStyleIndex >=0) {
    if (freeStyle(label->styles[nStyleIndex]) == MS_SUCCESS)
      msFree(label->styles[nStyleIndex]);
    for (i=nStyleIndex; i< label->numstyles-1; i++) {
      label->styles[i] = label->styles[i+1];
    }
    label->styles[label->numstyles-1] = NULL;
    label->numstyles--;
    return(MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msDeleteLabelStyle()",
             nStyleIndex);
  return (MS_FAILURE);
}

styleObj *msRemoveLabelStyle(labelObj *label, int nStyleIndex)
{
  int i;
  styleObj *style;
  if (nStyleIndex < 0 || nStyleIndex >= label->numstyles) {
    msSetError(MS_CHILDERR, "Cannot remove style, invalid nStyleIndex %d", "removeLabelStyle()", nStyleIndex);
    return NULL;
  } else {
    style=label->styles[nStyleIndex];
    for (i=nStyleIndex; i<label->numstyles-1; i++) {
      label->styles[i]=label->styles[i+1];
    }
    label->styles[label->numstyles-1]=NULL;
    label->numstyles--;
    MS_REFCNT_DECR(style);
    return style;
  }
}

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
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontt.h>
#include <gdfontmb.h>
#include <gdfontg.h>

#include "mapserver.h"

MS_CVSID("$Id$")


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
char *msWrapText(labelObj *label, char *text) {
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
    } else { /* negative maxlength: we split lines unconditionally, i.e. without
    loooking for a wrap character*/
        int numglyphs,numlines;
        maxlength = -maxlength; /* use a positive value*/
        numglyphs = msGetNumGlyphs(text);
        numlines = numglyphs / maxlength; /*count total number of lines needed
                                            after splitting*/
        if(numlines>1) {
            char *newtext = malloc(strlen(text)+numlines+1);
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

char *msAlignText(mapObj *map, imageObj *image, labelObj *label, char *text) {
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
        double size=0; /*initialize this here to avoid compiler warning*/
        if(label->type == MS_TRUETYPE) {
            size = label->size; /*keep a copy of the original size*/
            label->size=10;
        }
        /* compute the size of 16 adjacent spaces. we can't do this for just one space,
         * as the labelSize computing functions return integer bounding boxes. we assume
         * that the integer rounding for such a number of spaces will be negligeable
         * compared to the actual size of thoses spaces */ 
        if(msGetLabelSize(image, ".              .", label,&label_rect, 
                    &map->fontset, 1.0, MS_FALSE,NULL)==-1) { 
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
            label->size = size; /*restore original label size*/
            label->space_size_10=spacewidth; /*cache the computed size*/

            /*size of a space for current label size*/
            spacewidth = spacewidth * (double)label->size/10.0;
        }
    } else {
        spacewidth = label->space_size_10 * (double)label->size/10.0;
    }
   
    
    /*length in pixels of each line*/
    textlinelengths = (int*)malloc(numlines*sizeof(int));
    
    /*number of spaces that need to be added to each line*/
    numspacesforpadding = (int*)malloc(numlines*sizeof(int));
    
    /*total number of spaces that need to be added*/
    numspacestoadd=0;

    /*length in pixels of the longest line*/
    maxlinelength=0;
    for(i=0;i<numlines;i++) {
        msGetLabelSize(image, textlines[i], label, &label_rect, &map->fontset, 1.0, MS_FALSE, NULL);
        textlinelengths[i] = label_rect.maxx-label_rect.minx;
        if(maxlinelength<textlinelengths[i])
            maxlinelength=textlinelengths[i];
    }
    for(i=0;i<numlines;i++) {
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
    newtext = (char*)malloc(strlen(text)+1+numspacestoadd);
    newtextptr=newtext;
    for(i=0;i<numlines;i++) {
        int j;
        /*padd beginning of line with needed spaces*/
        for(j=0;j<numspacesforpadding[i];j++) {
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
    for(i=0;i<numlines;i++) {
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
char *msTransformLabelText(mapObj *map, imageObj* image,labelObj *label, char *text)
{
    char *newtext = text;
    if(label->encoding)
        newtext = msGetEncodedString(text, label->encoding);
    else
        newtext=strdup(text);
    
    if(newtext && (label->wrap!='\0' || label->maxlength!=0)) {
        newtext = msWrapText(label, newtext);
    }

    if(newtext && label->align!=MS_ALIGN_LEFT ) {
        newtext = msAlignText(map, image,label, newtext);
    }

    return newtext;
}

int msAddLabel(mapObj *map, int layerindex, int classindex, shapeObj *shape, pointObj *point, labelPathObj *labelpath, char *string, double featuresize, labelObj *label )
{
  int i;
  labelCacheSlotObj *cacheslot;

  labelCacheMemberObj *cachePtr=NULL;
  layerObj *layerPtr=NULL;
  classObj *classPtr=NULL;

  if(!string) return(MS_SUCCESS); /* not an error */ 

  layerPtr = (GET_LAYER(map, layerindex)); /* set up a few pointers for clarity */
  classPtr = GET_LAYER(map, layerindex)->class[classindex];

  if( label == NULL )
    label = &(classPtr->label);

  if(map->scaledenom > 0) {
    if((label->maxscaledenom != -1) && (map->scaledenom >= label->maxscaledenom))
      return(MS_SUCCESS);
    if((label->minscaledenom != -1) && (map->scaledenom < label->minscaledenom))
      return(MS_SUCCESS);
  }

  if(map->scaledenom > 0) {
    if((label->maxscaledenom != -1) && (map->scaledenom >= label->maxscaledenom))
        return(MS_SUCCESS);
    if((label->minscaledenom != -1) && (map->scaledenom < label->minscaledenom))
        return(MS_SUCCESS);
  }

  /* Validate label priority value and get ref on label cache for it */
  if (label->priority < 1)
    label->priority = 1;
  else if (label->priority > MS_MAX_LABEL_PRIORITY)
    label->priority = MS_MAX_LABEL_PRIORITY;

  cacheslot = &(map->labelcache.slots[label->priority-1]);

  if(cacheslot->numlabels == cacheslot->cachesize) { /* just add it to the end */
    cacheslot->labels = (labelCacheMemberObj *) realloc(cacheslot->labels, sizeof(labelCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
    if(!cacheslot->labels) {
      msSetError(MS_MEMERR, "Realloc() error.", "msAddLabel()");
      return(MS_FAILURE);
    }
   cacheslot->cachesize += MS_LABELCACHEINCREMENT;
  }

  cachePtr = &(cacheslot->labels[cacheslot->numlabels]);

  cachePtr->layerindex = layerindex; /* so we can get back to this *raw* data if necessary */
  cachePtr->classindex = classindex;

  if(shape) {
    cachePtr->tileindex = shape->tileindex;
    cachePtr->shapeindex = shape->index;
    cachePtr->shapetype = shape->type;
  } else {
    cachePtr->tileindex = cachePtr->shapeindex = -1;    
    cachePtr->shapetype = MS_SHAPE_POINT;
  }

  /* Store the label point or the label path (Bug #1620) */
  if ( point ) {
    cachePtr->point = *point; /* the actual label point */
    cachePtr->point.x = MS_NINT(cachePtr->point.x);
    cachePtr->point.y = MS_NINT(cachePtr->point.y);
    cachePtr->labelpath = NULL;
  } else if ( labelpath ) {
    int i;
    cachePtr->labelpath = labelpath;
    /* Use the middle point of the labelpath for mindistance calculations */
    i = labelpath->path.numpoints / 2;
    cachePtr->point.x = MS_NINT(labelpath->path.point[i].x);
    cachePtr->point.y = MS_NINT(labelpath->path.point[i].y);
  }

  cachePtr->text = strdup(string); /* the actual text */

  /* copy the styles (only if there is an accompanying marker)
   * We cannot simply keeep refs because the rendering code alters some members of the style objects
   */
  cachePtr->styles = NULL;
  cachePtr->numstyles = 0;
  if((layerPtr->type == MS_LAYER_ANNOTATION && classPtr->numstyles > 0) || layerPtr->type == MS_LAYER_POINT) {
    cachePtr->numstyles = classPtr->numstyles;
    cachePtr->styles = (styleObj *) malloc(sizeof(styleObj)*classPtr->numstyles);
    if (classPtr->numstyles > 0) {
      for(i=0; i<classPtr->numstyles; i++) {
	initStyle(&(cachePtr->styles[i]));
	msCopyStyle(&(cachePtr->styles[i]), classPtr->styles[i]);
      }
    }
  }

  /* copy the label */
  cachePtr->label = *label; /* this copies all non-pointers */
  if(label->font) cachePtr->label.font = strdup(label->font);

  cachePtr->featuresize = featuresize;

  cachePtr->poly = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(cachePtr->poly);

  cachePtr->status = MS_FALSE;

  if(layerPtr->type == MS_LAYER_POINT) { /* cache the marker placement, it's already on the map */
    rectObj rect;
    int w, h;

    if(cacheslot->nummarkers == cacheslot->markercachesize) { /* just add it to the end */
      cacheslot->markers = (markerCacheMemberObj *) realloc(cacheslot->markers, sizeof(markerCacheMemberObj)*(cacheslot->cachesize+MS_LABELCACHEINCREMENT));
      if(!cacheslot->markers) {
	msSetError(MS_MEMERR, "Realloc() error.", "msAddLabel()");
	return(MS_FAILURE);
      }
      cacheslot->markercachesize+=MS_LABELCACHEINCREMENT;
    }

    i = cacheslot->nummarkers;

    cacheslot->markers[i].poly = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(cacheslot->markers[i].poly);

    /* TO DO: at the moment only checks the bottom style, perhaps should check all of them */
    /* #2347: after RFC-24 classPtr->styles could be NULL so we check it */
    if(classPtr->styles != NULL) {
      if(msGetMarkerSize(&map->symbolset, classPtr->styles[0], &w, &h, layerPtr->scalefactor) != MS_SUCCESS) 
        return(MS_FAILURE);
    } else {
      msSetError(MS_MISCERR, "msAddLabel error: missing style definition for layer '%s'", "msAddLabel()", layerPtr->name);
      return(MS_FAILURE);
    }
    rect.minx = MS_NINT(point->x - .5 * w);
    rect.miny = MS_NINT(point->y - .5 * h);
    rect.maxx = rect.minx + (w-1);
    rect.maxy = rect.miny + (h-1);
    msRectToPolygon(rect, cacheslot->markers[i].poly);    
    cacheslot->markers[i].id = cacheslot->numlabels;

    cacheslot->nummarkers++;
  }

  cacheslot->numlabels++;

  /* Maintain main labelCacheObj.numlabels only for backwards compatibility */
  map->labelcache.numlabels++;

  return(MS_SUCCESS);
}

/* msTestLabelCacheCollisions()
**
** Compares current label against labels already drawn and markers from cache and discards it
** by setting cachePtr->status=MS_FALSE if it is a duplicate, collides with another label,
** or collides with a marker.
**
** This function is used by the various msDrawLabelCacheXX() implementations.
*/
void msTestLabelCacheCollisions(labelCacheObj *labelcache, labelObj *labelPtr, 
                                int mapwidth, int mapheight, int buffer,
                                labelCacheMemberObj *cachePtr, int current_priority, 
                                int current_label, int mindistance, double label_size)
{
    int i, p;

    /* Check against image bounds first 
    ** Pass mapwidth=-1 to skip this test
     */
    if(!labelPtr->partials && mapwidth > 0 && mapheight > 0) {
        if(labelInImage(mapwidth, mapheight, cachePtr->poly, buffer) == MS_FALSE) {
	    cachePtr->status = MS_FALSE;
            return;
        }
    }

    /* Compare against all rendered markers from this priority level and higher.
    ** Labels can overlap their own marker and markers from lower priority levels
    */
    for (p=current_priority; p < MS_MAX_LABEL_PRIORITY; p++) {
        labelCacheSlotObj *markerslot;
        markerslot = &(labelcache->slots[p]);

        for ( i = 0; i < markerslot->nummarkers; i++ ) {
            if ( !(p == current_priority && current_label == markerslot->markers[i].id) ) {  /* labels can overlap their own marker */
                if ( intersectLabelPolygons(markerslot->markers[i].poly, cachePtr->poly ) == MS_TRUE ) {
                    cachePtr->status = MS_FALSE;  /* polys intersect */
                    return;
                }
            }
        }
    }

    /* compare against rendered labels */
    i = current_label+1;

    for(p=current_priority; p<MS_MAX_LABEL_PRIORITY; p++) {
        labelCacheSlotObj *cacheslot;
        cacheslot = &(labelcache->slots[p]);

        for(  ; i < cacheslot->numlabels; i++) { 
            if(cacheslot->labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */
                /* We add the label_size to the mindistance value when comparing because we do want the mindistance 
                   value between the labels and not only from point to point. */
                if((mindistance != -1) && (cachePtr->classindex == cacheslot->labels[i].classindex) && (strcmp(cachePtr->text,cacheslot->labels[i].text) == 0) && (msDistancePointToPoint(&(cachePtr->point), &(cacheslot->labels[i].point)) <= (mindistance + label_size))) { /* label is a duplicate */
                    cachePtr->status = MS_FALSE;
                    return;
                }

                if(intersectLabelPolygons(cacheslot->labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
                    cachePtr->status = MS_FALSE;
                    return;
                }
            }
        } /* i */

        i = 0; /* Start over with 1st label of next slot */
    } /* p */
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
    if(i >= 0 && i < labelcache->numlabels)
    {
        int p;
        for(p=0; p<MS_MAX_LABEL_PRIORITY; p++)
        {
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
#ifdef USE_GD_FT
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
      sprintf(file2, "%s%s", path, file1);
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
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msLoadFontSet()");
  return(-1);
#endif
}

int msGetTruetypeTextBBox(imageObj *img, char *font, double size, char *string, rectObj *rect, double **advances) {
#ifdef USE_GD_FT
	if(img!=NULL && MS_RENDERER_PLUGIN(img->format)) {
		img->format->vtable->getTruetypeTextBBox(img,font,size,string,rect,advances);
		//printf("%s: %f %f %f %f\n",string,rect->minx,rect->miny,rect->maxx,rect->maxy);
		return MS_SUCCESS;
	} else 
#ifdef USE_AGG
	if(img!=NULL && MS_RENDERER_AGG(img->format)) {
        msGetTruetypeTextBBoxAGG(img,font,size,string,rect,advances);
        //printf("%s: %f %f %f %f\n",string,rect->minx,rect->miny,rect->maxx,rect->maxy);
        return MS_SUCCESS;
    } else
#endif
    {
        int bbox[8];
        char *error;
        if(advances) {
#if defined (GD_HAS_FTEX_XSHOW)
            char *s;
            int k;
            gdFTStringExtra strex;
            strex.flags = gdFTEX_XSHOW;
            error = gdImageStringFTEx(NULL, bbox, 0, font, size, 0, 0, 0, string, &strex);
            if(error) {
                msSetError(MS_TTFERR, error, "gdImageStringFTEx()");
                return(MS_FAILURE);
            }

            *advances = (double*)malloc( strlen(string) * sizeof(double) );
            s = strex.xshow;
            k = 0;
            while ( *s && k < strlen(string) ) {
                (*advances)[k++] = atof(s);      
                while ( *s && *s != ' ')
                    s++;
                if ( *s == ' ' )
                    s++;
            }

            gdFree(strex.xshow); /* done with character advances */

            rect->minx = bbox[0];
            rect->miny = bbox[5];
            rect->maxx = bbox[2];
            rect->maxy = bbox[1];
            return MS_SUCCESS;
#else
            msSetError(MS_TTFERR, "gdImageStringFTEx support is not available or is not current enough (requires 2.0.29 or higher).", "msGetTruetypeTextBBox()");
            return(-1);
#endif
        } else {
            error = gdImageStringFT(NULL, bbox, 0, font, size, 0, 0, 0, string);
            if(error) {
                msSetError(MS_TTFERR, error, "msGetTruetypeTextBBox()");
                return(MS_FAILURE);
            }

            rect->minx = bbox[0];
            rect->miny = bbox[5];
            rect->maxx = bbox[2];
            rect->maxy = bbox[1];
            return MS_SUCCESS;
        }
    }
#else
    /*shouldn't happen*/
    return(MS_FAILURE);
#endif
}

int msGetRasterTextBBox(imageObj *img, int size, char *string, rectObj *rect) {
#ifdef USE_AGG
    if(img!=NULL && MS_RENDERER_AGG(img->format)) {
        return msGetRasterTextBBoxAGG(img,size,string,rect); 
    } else
#endif
    {
        gdFontPtr fontPtr;
        char **token=NULL;
        int t, num_tokens, max_token_length=0;
        if((fontPtr = msGetBitmapFont(size)) == NULL)
            return(-1);

        if((token = msStringSplit(string, '\n', &(num_tokens))) == NULL)
            return(0);

        for(t=0; t<num_tokens; t++) /* what's the longest token */
            max_token_length = MS_MAX(max_token_length, (int) strlen(token[t]));

        rect->minx = 0;
        rect->miny = -(fontPtr->h * num_tokens);
        rect->maxx = fontPtr->w * max_token_length;
        rect->maxy = 0;

        msFreeCharArray(token, num_tokens);
        return MS_SUCCESS;
    }
}

/*
** Note: All these routines assume a reference point at the LL corner of the text. GD's
** bitmapped fonts use UL and this is compensated for. Note the rect is relative to the
** LL corner of the text to be rendered, this is first line for TrueType fonts.
*/

/* assumes an angle of 0 regardless of what's in the label object */
int msGetLabelSize(imageObj *img, char *string, labelObj *label, rectObj *rect, fontSetObj *fontset, double scalefactor, int adjustBaseline, double **advances)
{
  double size;
  if(label->type == MS_TRUETYPE) {
#ifdef USE_GD_FT
    char *font=NULL;

    size = label->size*scalefactor;
    if (img != NULL) {
      size = MS_MAX(size, label->minsize*img->resolutionfactor);
      size = MS_MIN(size, label->maxsize*img->resolutionfactor);
    }
    else {
      size = MS_MAX(size, label->minsize);
      size = MS_MIN(size, label->maxsize);
    }
    scalefactor = size / label->size;
    font = msLookupHashTable(&(fontset->fonts), label->font);
    if(!font) {
      if(label->font) 
        msSetError(MS_TTFERR, "Requested font (%s) not found.", "msGetLabelSize()", label->font);
      else
        msSetError(MS_TTFERR, "Requested font (NULL) not found.", "msGetLabelSize()");
      return(-1);
    }

    if(msGetTruetypeTextBBox(img,font,size,string,rect,advances)!=MS_SUCCESS)
        return -1;

    /* bug 1449 fix (adjust baseline) */
    if(adjustBaseline) {
      int nNewlines = msCountChars(string,'\n');
      if(!nNewlines) {
        label->offsety += (MS_NINT(((rect->miny + rect->maxy) + size) / 2))/scalefactor;
        label->offsetx += (MS_NINT(rect->minx / 2))/scalefactor;
      }
      else {
        rectObj rect2; /*bbox of first line only*/
        char* firstLine = msGetFirstLine(string);
        msGetTruetypeTextBBox(img,font,size,firstLine,&rect2,NULL);
        label->offsety += (MS_NINT(((rect2.miny+rect2.maxy) + size) / 2))/scalefactor;
        label->offsetx += (MS_NINT(rect2.minx / 2))/scalefactor;
        free(firstLine);
      }
    }
    return 0;
#else
    msSetError(MS_TTFERR, "TrueType font support is not available.", "msGetLabelSize()");
    return(-1);
#endif
  } else { /* MS_BITMAP font */
    msGetRasterTextBBox(img,MS_NINT(label->size),string,rect);
 }
  return(0);
}

gdFontPtr msGetBitmapFont(int size)
{
  switch(size) { /* set the font to use */
  case MS_TINY:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetTiny();
#else
    return(gdFontTiny);
#endif
    break;
  case MS_SMALL:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetSmall();
#else
    return(gdFontSmall);
#endif
    break;
  case MS_MEDIUM:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetMediumBold();
#else
    return(gdFontMediumBold);
#endif
    break;
  case MS_LARGE:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetLarge();
#else
    return(gdFontLarge);
#endif
    break;
  case MS_GIANT:
#ifdef GD_HAS_GETBITMAPFONT
    return gdFontGetGiant();
#else
    return(gdFontGiant);
#endif
    break;
  default:
    msSetError(MS_GDERR,"Invalid bitmap font. Must be one of tiny, small, medium, large or giant." , "msGetBitmapFont()");
    return(NULL);
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
  q.x = p->x + MS_NINT(x * cos_a - (y) * sin_a);
  q.y = p->y - MS_NINT(x * sin_a + (y) * cos_a);

  if(poly) {
      /*
       * here we should/could have a test asserting that the poly lineObj 
       * has at least 5 points available.
       */

    x2 = x1 - buffer; /* ll */
    y2 = y1 + buffer;
    poly->point[0].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    poly->point[0].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);

    x2 = x1 - buffer; /* ul */
    y2 = y1 - h - buffer;
    poly->point[1].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    poly->point[1].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);

    x2 = x1 + w + buffer; /* ur */
    y2 = y1 - h - buffer;
    poly->point[2].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    poly->point[2].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);

    x2 = x1 + w + buffer; /* lr */
    y2 = y1 + buffer;
    poly->point[3].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    poly->point[3].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);

    poly->point[4].x = poly->point[0].x;
    poly->point[4].y = poly->point[0].y;
  }

  return(q);
}


/* static pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly) */
pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly) {
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
** is a label completely in the image (excluding label buffer)
*/
/* static int labelInImage(int width, int height, shapeObj *lpoly, int buffer) */
int labelInImage(int width, int height, shapeObj *lpoly, int buffer)
{
  int i,j;

  for(i=0; i<lpoly->numlines; i++) {
    for(j=1; j<lpoly->line[i].numpoints; j++) {
      if(lpoly->line[i].point[j].x < -buffer) return(MS_FALSE);
      if(lpoly->line[i].point[j].x >= width+buffer) return(MS_FALSE);
      if(lpoly->line[i].point[j].y < -buffer) return(MS_FALSE);
      if(lpoly->line[i].point[j].y >= height+buffer) return(MS_FALSE);
    }
  }

  return(MS_TRUE);
}

/*
** Variation on msIntersectPolygons. Label polygons aren't like shapefile polygons. They
** have no holes, and often do have overlapping parts (i.e. road symbols).
*/

/* static int intersectLabelPolygons(shapeObj *p1, shapeObj *p2) { */
int intersectLabelPolygons(shapeObj *p1, shapeObj *p2) {
  int c1,v1,c2,v2;
  pointObj *point;

  /* STEP 0: check bounding boxes */
  if(!msRectOverlap(&p1->bounds, &p2->bounds)) /* from alans@wunderground.com */
    return(MS_FALSE);

  /* STEP 1: look for intersecting line segments */
  for(c1=0; c1<p1->numlines; c1++)
    for(v1=1; v1<p1->line[c1].numpoints; v1++)
      for(c2=0; c2<p2->numlines; c2++)
	for(v2=1; v2<p2->line[c2].numpoints; v2++)
	  if(msIntersectSegments(&(p1->line[c1].point[v1-1]), &(p1->line[c1].point[v1]), &(p2->line[c2].point[v2-1]), &(p2->line[c2].point[v2])) ==  MS_TRUE)
	    return(MS_TRUE);

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

int msImageTruetypeArrow(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor)
{
#ifdef USE_GD_FT
  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msImageTruetypeArrow()");
  return(-1);
#endif
}

int msImageTruetypePolyline(symbolSetObj *symbolset, imageObj *img, shapeObj *p, styleObj *style, double scalefactor)
{
#ifdef USE_GD_FT
  int i,j;
  double theta, length, current_length;
  labelObj label;
  pointObj point, label_point;
  rectObj label_rect;
  int label_width;
  int position, rot, gap, in;
  double rx, ry, size;
  symbolObj *symbol;


  symbol = symbolset->symbol[style->symbol];

  initLabel(&label);
  rot = (symbol->gap <= 0);
  label.type = MS_TRUETYPE;
  label.font = symbol->font;
  /* -- rescaling symbol and gap */
  if(style->size == -1) {
      size = msSymbolGetDefaultSize( symbol );
  }
  else
      size = style->size;
  if(size*scalefactor > style->maxsize*img->resolutionfactor) scalefactor = (float)style->maxsize*img->resolutionfactor/(float)size;
  if(size*scalefactor < style->minsize*img->resolutionfactor) scalefactor = (float)style->minsize*img->resolutionfactor/(float)size;
  gap = MS_ABS(symbol->gap)* (int) scalefactor;
  label.size = size ;/* "* scalefactor" removed: this is already scaled in msDrawTextGD()*/
  /* label.minsize = style->minsize; */
  /* label.maxsize = style->maxsize; */

  label.color = style->color; /* TODO: now assuming these colors should have previously allocated pen values */
  label.outlinecolor = style->outlinecolor;
  label.antialias = symbol->antialias;
  
  if(msGetLabelSize(NULL,symbol->character, &label, &label_rect, symbolset->fontset, scalefactor, MS_FALSE,NULL) == -1)
    return(-1);

  label_width = (int) label_rect.maxx - (int) label_rect.minx;

  for(i=0; i<p->numlines; i++) {
    current_length = gap+label_width/2.0; /* initial padding for each line */
    
    for(j=1;j<p->line[i].numpoints;j++) {
      length = sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2)));
      if(length==0)continue;
      rx = (p->line[i].point[j].x - p->line[i].point[j-1].x)/length;
      ry = (p->line[i].point[j].y - p->line[i].point[j-1].y)/length;  
      position = symbol->position;
      theta = asin(ry);
      if(rx < 0) {
        if(rot){
          theta += MS_PI;
                if((position == MS_UR)||(position == MS_UL)) position = MS_LC;
          if((position == MS_LR)||(position == MS_LL)) position = MS_UC;
        }else{
          if(position == MS_UC) position = MS_LC;
          else if(position == MS_LC) position = MS_UC;
        }
      }
      else theta = -theta;        
      if((position == MS_UR)||(position == MS_UL)) position = MS_UC;
      if((position == MS_LR)||(position == MS_LL)) position = MS_LC;
      
      label.angle = style->angle;
      if(rot)
          label.angle+=MS_RAD_TO_DEG * theta;
      
      in = 0;
      while(current_length <= length) {
        point.x = MS_NINT(p->line[i].point[j-1].x + current_length*rx);
        point.y = MS_NINT(p->line[i].point[j-1].y + current_length*ry);

        label_point = get_metrics(&point, (rot)?MS_CC:position, label_rect, 0, 0, label.angle, 0, NULL);
        msDrawTextGD(img, label_point, symbol->character, &label, symbolset->fontset, scalefactor);

        current_length += label_width + gap;
        in = 1;
      }

      if(in){
        current_length -= length + label_width/2.0;
      } else current_length -= length;
    }
  }

  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msImageTruetypePolyline()");
  return(-1);
#endif
}

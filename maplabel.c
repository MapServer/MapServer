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
 * see http://mapserver.gis.umn.edu/development/rfc/ms-rfc-40/
 * for a summary of how wrap/maxlength interact on the result
 * of the text transformation
 */
char *msWrapText(labelObj *label, char *text) {
    char wrap;
    int maxlength;
    if(!text)
        return text;
    wrap = label->wrap;
    maxlength = label->maxlength;
    if(maxlength == 0) {
        if(wrap!='\0') {
            /* if maxlength = 0 and a wrap character was specified, replace
             * all wrap characters by \n
             */
            msReplaceChar(text, wrap, '\n');
        }
        /* if neither maxlength, nor wrap were specified,
         * don't transform this text
         */
        return text;
    } else if(maxlength>0) {
        if(wrap!='\0') {
            int numwrapchars = msCountChars(text,wrap);
            if(numwrapchars > 0) {
                if(label->encoding) {
/*
                     * we have to use utf decoding functions here, so as not to
                     * split a text line on a multibyte character
                     */
                    int num_cur_glyph_on_line = 0;
                    char *textptr = text;
                    char glyph[11];
                    int glyphlen = 0;
                    while((glyphlen = msGetNextGlyph((const char**)&textptr,glyph))>0) {
                        num_cur_glyph_on_line++;
                        if(*glyph == wrap && num_cur_glyph_on_line>=(maxlength)) {
                            /*FIXME (if wrap becomes something other than char):*/
                            *(textptr-1)='\n';
                            num_cur_glyph_on_line=0;
                        }
                    }
                } else {
                    int cur_char_on_line = 0;
                    char *textptr = text;
                    while(*textptr != 0) {
                        cur_char_on_line++;
                        if(*textptr == wrap && cur_char_on_line>=maxlength) {
                            *textptr='\n';
                            cur_char_on_line=0;
                        }
                        textptr++;
                    }
                }
                return text;
            } else {
                /*text is shorter than fixed limit*/
                return text;
            }
        } else {
            /*don't draw this label if it is longer thant the specified length
             * and no wrap character was specified
             */
            if(msGetNumGlyphs(text)>maxlength) {
                free(text);
                return NULL;
            } else {
                return text;
            }
        }
    } else { /*maxlength<0*/
        int numglyphs,numlines;
        maxlength = -maxlength;
        numglyphs = msGetNumGlyphs(text);
        numlines = numglyphs / maxlength;
        if(numlines>1) {
            char *newtext = malloc(strlen(text)+numlines+1);
            char *newtextptr = newtext;
            char *textptr = text;
            int glyphlen = 0, num_cur_glyph = 0;
            while((glyphlen = msGetNextGlyph((const char**)&textptr,newtextptr))>0) {
                num_cur_glyph++;
                newtextptr += glyphlen;
                if(num_cur_glyph%maxlength == 0 && num_cur_glyph != numglyphs) {
                    *newtextptr = '\n';
                    newtextptr++;
                }
            }
            free(text);
            return newtext;
        } else {
            return text;
        }
    }
}

char *msAlignText(mapObj *map, imageObj *image, labelObj *label, char *text) {
    double spacewidth=0.0;
    int numlines;
    char **textlines,*newtext,*newtextptr;
    int *textlinelengths,*numspacesforpadding;
    int numspacestoadd,maxlinelength,i;
    rectObj label_rect;
    if(label->space_size_10 == 0.0) {
        int size;
        if(label->type == MS_TRUETYPE) {
            size = label->size;
            label->size=10;
        }
        if(msGetLabelSize(image, "                ", label, &label_rect, &map->fontset, 1.0, MS_FALSE)==-1)
            return text; /*error*/
        spacewidth = (label_rect.maxx-label_rect.minx)/16.0;
        if(label->type == MS_TRUETYPE) {
            label->size = size;
            label->space_size_10=spacewidth;
            spacewidth = spacewidth * (double)label->size/10.0;
        }
    } else {
        spacewidth = label->space_size_10 * (double)label->size/10.0;
    }
    textlines = msStringSplit(text,'\n',&numlines);
    if(numlines==1) {
        /*nothing to do*/
        free(textlines[0]);
        free(textlines);
        return text;
    }

    textlinelengths = (int*)malloc(numlines*sizeof(int));
    numspacesforpadding = (int*)malloc(numlines*sizeof(int));
    numspacestoadd=0;
    maxlinelength=0;
    for(i=0;i<numlines;i++) {
        msGetLabelSize(image, textlines[i], label, &label_rect, &map->fontset, 1.0, MS_FALSE);
        textlinelengths[i] = label_rect.maxx-label_rect.minx;
        if(maxlinelength<textlinelengths[i])
            maxlinelength=textlinelengths[i];
    }
    for(i=0;i<numlines;i++) {
        double nfracspaces = (maxlinelength - textlinelengths[i])/spacewidth;
        numspacesforpadding[i]=MS_NINT(((maxlinelength - textlinelengths[i])/2.0)/(spacewidth));
        if(label->align == MS_ALIGN_CENTER) {
            numspacesforpadding[i]=MS_NINT(nfracspaces/2.0);
        } else {
           if(label->align == MS_ALIGN_RIGHT) {
               numspacesforpadding[i]=MS_NINT(nfracspaces);
           }
        }
        numspacestoadd+=numspacesforpadding[i];
    }
    newtext = (char*)malloc(strlen(text)+1+numspacestoadd);
    newtextptr=newtext;
    for(i=0;i<numlines;i++) {
        int j;
        for(j=0;j<numspacesforpadding[i];j++) {
            *(newtextptr++)=' ';
        }
        strcpy(newtextptr,textlines[i]);
        newtextptr+=strlen(textlines[i])+1;
        if(i!=numlines-1) {
           *(newtextptr-1)='\n';
        }
    }
    free(text);
    for(i=0;i<numlines;i++) {
        free(textlines[i]);
    }
    free(textlines);
    free(textlinelengths);
    free(numspacesforpadding);
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

int msAddLabel(mapObj *map, int layerindex, int classindex, int shapeindex, int tileindex, pointObj *point, labelPathObj *labelpath, char *string, double featuresize, labelObj *label )
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
  cachePtr->tileindex = tileindex;
  cachePtr->shapeindex = shapeindex;

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
    if (classPtr->styles != NULL) {
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
                                labelCacheMemberObj *cachePtr, int current_priority, int current_label)
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

                if((labelPtr->mindistance != -1) && (cachePtr->classindex == cacheslot->labels[i].classindex) && (strcmp(cachePtr->text,cacheslot->labels[i].text) == 0) && (msDistancePointToPoint(&(cachePtr->point), &(cacheslot->labels[i].point)) <= labelPtr->mindistance)) { /* label is a duplicate */
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

    if (!file1 || !alias || (strlen(file1) <= 0))
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

/*
** Note: All these routines assume a reference point at the LL corner of the text. GD's
** bitmapped fonts use UL and this is compensated for. Note the rect is relative to the
** LL corner of the text to be rendered, this is first line for TrueType fonts.
*/

/* assumes an angle of 0 regardless of what's in the label object */
int msGetLabelSizeGD(char *string, labelObj *label, rectObj *rect, fontSetObj *fontset, double scalefactor, int adjustBaseline)
{
  int size;
  if(label->type == MS_TRUETYPE) {
#ifdef USE_GD_FT
    int bbox[8];
    char *error=NULL, *font=NULL;

    size = MS_NINT(label->size*scalefactor);
    size = MS_MAX(size, label->minsize);
    size = MS_MIN(size, label->maxsize);

    font = msLookupHashTable(&(fontset->fonts), label->font);
    if(!font) {
      if(label->font) 
        msSetError(MS_TTFERR, "Requested font (%s) not found.", "msGetLabelSize()", label->font);
      else
        msSetError(MS_TTFERR, "Requested font (NULL) not found.", "msGetLabelSize()");
      return(-1);
    }


    error = gdImageStringFT(NULL, bbox, 0, font, size, 0, 0, 0, string);
    if(error) {
      msSetError(MS_TTFERR, error, "msGetLabelSize()");
      return(-1);
    }

    rect->minx = bbox[0];
    rect->miny = bbox[5];
    rect->maxx = bbox[2];
    rect->maxy = bbox[1];

    /* bug 1449 fix (adjust baseline) */
    if(adjustBaseline) {
      int nNewlines = msCountChars(string,'\n');
      if(!nNewlines) {
        label->offsety += MS_NINT(((bbox[5] + bbox[1]) + size) / 2);
        label->offsetx += MS_NINT(bbox[0] / 2);
      }
      else {
        char* firstLine = msGetFirstLine(string);
        gdImageStringFT(NULL, bbox, 0, font, size, 0, 0, 0, firstLine);
        label->offsety += MS_NINT(((bbox[5] + bbox[1]) + size) / 2);
        label->offsetx += MS_NINT(bbox[0] / 2);
        free(firstLine);
      }
    }
#else
    msSetError(MS_TTFERR, "TrueType font support is not available.", "msGetLabelSize()");
    return(-1);
#endif
  } else { /* MS_BITMAP font */
    gdFontPtr fontPtr;
    char **token=NULL;
    int t, num_tokens, max_token_length=0;
    if((fontPtr = msGetBitmapFont(label->size)) == NULL)
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
    
  }
  return(0);
}

int msGetLabelSize(imageObj *img, char *string, labelObj *label, rectObj *rect, fontSetObj *fontset, double scalefactor, int adjustBaseline)
{
#ifdef USE_AGG
    if(img!= NULL && MS_RENDERER_AGG(img->format))
        return msGetLabelSizeAGG(img,string,label,rect,fontset,scalefactor,adjustBaseline);
    else
#endif
        return msGetLabelSizeGD(string,label,rect,fontset,scalefactor,adjustBaseline);
}

/*
 * Return the size of the label, plus an array of individual character
 * offsets, as calculated by gdImageStringFTEx().  The callee is
 * responsible for freeing *offsets.
 */
int msGetLabelSizeEx(char *string, labelObj *label, rectObj *rect, fontSetObj *fontset, double scalefactor, int adjustBaseline, double **offsets) 
{
#if defined (USE_GD_FT) && defined (GD_HAS_FTEX_XSHOW)
    int size;
    char *s;
    int k;
    int bbox[8];
    char *error=NULL, *font=NULL;
    gdFTStringExtra strex;
    
    size = MS_NINT(label->size*scalefactor);
    size = MS_MAX(size, label->minsize);
    size = MS_MIN(size, label->maxsize);

    font = msLookupHashTable(&(fontset->fonts), label->font);
    if(!font) {
      if(label->font) 
        msSetError(MS_TTFERR, "Requested font (%s) not found.", "msGetLabelSizeEx()", label->font);
      else
        msSetError(MS_TTFERR, "Requested font (NULL) not found.", "msGetLabelSizeEx()");
      return(-1);
    }

    strex.flags = gdFTEX_XSHOW;
    error = gdImageStringFTEx(NULL, bbox, 0, font, size, 0, 0, 0, string, &strex);
    if(error) {
      msSetError(MS_TTFERR, error, "msGetLabelSizeEx()");
      return(-1);
    }

    *offsets = (double*)malloc( strlen(string) * sizeof(double) );
    
    s = strex.xshow;
    k = 0;
    while ( *s && k < strlen(string) ) {
      (*offsets)[k++] = atof(s);      
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

    /* bug 1449 fix (adjust baseline) */
    if(adjustBaseline) {
      label->offsety += MS_NINT(((bbox[5] + bbox[1]) + size) / 2);
      label->offsetx += MS_NINT(bbox[0] / 2);
    }

    return MS_SUCCESS;
#else
    msSetError(MS_TTFERR, "TrueType font support is not available or is not current enough (requires 2.0.29 or higher).", "msGetLabelSizeEx()");
    return(-1);
#endif
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

int msImageTruetypePolyline(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor)
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
  if(size*scalefactor > style->maxsize) scalefactor = (float)style->maxsize/(float)size;
  if(size*scalefactor < style->minsize) scalefactor = (float)style->minsize/(float)size;
  gap = MS_ABS(symbol->gap)* (int) scalefactor;
  label.size = (int) (size * scalefactor);
  /* label.minsize = style->minsize; */
  /* label.maxsize = style->maxsize; */

  label.color = style->color; /* TODO: now assuming these colors should have previously allocated pen values */
  label.outlinecolor = style->outlinecolor;
  label.antialias = symbol->antialias;
  
  if(msGetLabelSizeGD(symbol->character, &label, &label_rect, symbolset->fontset, scalefactor, MS_FALSE) == -1)
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

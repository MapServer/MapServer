/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Various utility functions ... a real hodgepodge.
 * Author:   Steve Lime and the MapServer team.
 *
 * Notes: Some code (notably msAlphaBlend()) are directly derived from GD. See
 * the mapserver/GD-COPYING file for the GD license.  Use of this code in this
 * manner is compatible with the MapServer license.
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
 *****************************************************************************/


#include <time.h>

#include "mapserver.h"
#include "maptime.h"
#include "mapparser.h"
#include "mapthread.h"
#include "mapfile.h"
#include "mapcopy.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <process.h>
#endif

MS_CVSID("$Id$")

extern int msyyparse(void);
extern int msyylex(void);
extern int msyylex_destroy(void);
extern char *msyytext;

extern int msyyresult; /* result of parsing, true/false */
extern int msyystate;
extern char *msyystring;

/*
** Helper functions to convert from strings to other types or objects.
*/
static int bindIntegerAttribute(int *attribute, char *value)
{
  if(!value || strlen(value) == 0) return MS_FAILURE;
  *attribute = MS_NINT(atof(value)); /*use atof instead of atoi as a fix for bug 2394*/
  return MS_SUCCESS;
}

static int bindDoubleAttribute(double *attribute, char *value)
{
  if(!value || strlen(value) == 0) return MS_FAILURE;
  *attribute = atof(value);
  return MS_SUCCESS;
}

static int bindColorAttribute(colorObj *attribute, char *value)
{
  if(!value || strlen(value) == 0) return MS_FAILURE;

  if(value[0] == '#' && strlen(value) == 7) { /* got a hex color */
    char hex[2];

    hex[0] = value[1];
    hex[1] = value[2];
    attribute->red = msHexToInt(hex);
    hex[0] = value[3];
    hex[1] = value[4];
    attribute->green = msHexToInt(hex);
    hex[0] = value[5];
    hex[1] = value[6];
    attribute->blue = msHexToInt(hex);

    return MS_SUCCESS;
  } else { /* try a space delimited string */
    char **tokens=NULL;
    int numtokens=0;

    tokens = msStringSplit(value, ' ', &numtokens);
    if(tokens==NULL || numtokens != 3) {
      msFreeCharArray(tokens, numtokens);
      return MS_FAILURE; /* punt */
    }

    attribute->red = atoi(tokens[0]);
    attribute->green = atoi(tokens[1]);
    attribute->blue = atoi(tokens[2]);
    msFreeCharArray(tokens, numtokens);

    return MS_SUCCESS;
  }

  return MS_FAILURE; /* shouldn't get here */
}

/*
** Function to bind various layer properties to shape attributes.
*/
int msBindLayerToShape(layerObj *layer, shapeObj *shape, int querymapMode)
{
  int i, j;
  labelObj *label; /* for brevity */
  styleObj *style;

  if(!layer || !shape) return MS_FAILURE;

  for(i=0; i<layer->numclasses; i++) {

    /* check the styleObj's */
    for(j=0; j<layer->class[i]->numstyles; j++) {
      style = layer->class[i]->styles[j];

      if(style->numbindings > 0) {

        if(style->bindings[MS_STYLE_BINDING_SYMBOL].index != -1) {
          style->symbol = msGetSymbolIndex(&(layer->map->symbolset), shape->values[style->bindings[MS_STYLE_BINDING_SYMBOL].index], MS_TRUE);
          if(style->symbol == -1) style->symbol = 0; /* a reasonable default (perhaps should throw an error?) */
        }

        if(style->bindings[MS_STYLE_BINDING_ANGLE].index != -1) {
          style->angle = 360.0;
          bindDoubleAttribute(&style->angle, shape->values[style->bindings[MS_STYLE_BINDING_ANGLE].index]);
        }

        if(style->bindings[MS_STYLE_BINDING_SIZE].index != -1) {
          style->size = 1;
          bindDoubleAttribute(&style->size, shape->values[style->bindings[MS_STYLE_BINDING_SIZE].index]);
        }

        if(style->bindings[MS_STYLE_BINDING_WIDTH].index != -1) {
          style->width = 1;
          bindDoubleAttribute(&style->width, shape->values[style->bindings[MS_STYLE_BINDING_WIDTH].index]);
        }

        if(style->bindings[MS_STYLE_BINDING_COLOR].index != -1 && (querymapMode != MS_TRUE)) {
          MS_INIT_COLOR(style->color, -1,-1,-1);
          bindColorAttribute(&style->color, shape->values[style->bindings[MS_STYLE_BINDING_COLOR].index]);
        }

        if(style->bindings[MS_STYLE_BINDING_OUTLINECOLOR].index != -1 && (querymapMode != MS_TRUE)) {
          MS_INIT_COLOR(style->outlinecolor, -1,-1,-1);
          bindColorAttribute(&style->outlinecolor, shape->values[style->bindings[MS_STYLE_BINDING_OUTLINECOLOR].index]);
        }

        if(style->bindings[MS_STYLE_BINDING_OUTLINEWIDTH].index != -1) {
          style->outlinewidth = 1;
          bindDoubleAttribute(&style->outlinewidth, shape->values[style->bindings[MS_STYLE_BINDING_OUTLINEWIDTH].index]);
        }

        if(style->bindings[MS_STYLE_BINDING_OPACITY].index != -1) {
          style->opacity = 100;
          bindIntegerAttribute(&style->opacity, shape->values[style->bindings[MS_STYLE_BINDING_OPACITY].index]);
        }
      }
    } /* next styleObj */

    /* check the labelObj */
    label = &(layer->class[i]->label);

    if(label->numbindings > 0) {
      if(label->bindings[MS_LABEL_BINDING_ANGLE].index != -1) {
        label->angle = 0.0;
        bindDoubleAttribute(&label->angle, shape->values[label->bindings[MS_LABEL_BINDING_ANGLE].index]);
      }

      if(label->bindings[MS_LABEL_BINDING_SIZE].index != -1) {
        label->size = 1;
        bindDoubleAttribute(&label->size, shape->values[label->bindings[MS_LABEL_BINDING_SIZE].index]);
      }

      if(label->bindings[MS_LABEL_BINDING_COLOR].index != -1) {
        MS_INIT_COLOR(label->color, -1,-1,-1);
        bindColorAttribute(&label->color, shape->values[label->bindings[MS_LABEL_BINDING_COLOR].index]);
      }

      if(label->bindings[MS_LABEL_BINDING_OUTLINECOLOR].index != -1) {
        MS_INIT_COLOR(label->outlinecolor, -1,-1,-1);
        bindColorAttribute(&label->outlinecolor, shape->values[label->bindings[MS_LABEL_BINDING_OUTLINECOLOR].index]);
      }

      if(label->bindings[MS_LABEL_BINDING_FONT].index != -1) {
        msFree(label->font);
        label->font = strdup(shape->values[label->bindings[MS_LABEL_BINDING_FONT].index]);
      }

      if(label->bindings[MS_LABEL_BINDING_PRIORITY].index != -1) {
        label->priority = MS_DEFAULT_LABEL_PRIORITY;
        bindIntegerAttribute(&label->priority, shape->values[label->bindings[MS_LABEL_BINDING_PRIORITY].index]);
      }
    }
  } /* next classObj */

  return MS_SUCCESS;
}

/*
 * Used to get red, green, blue integers separately based upon the color index
 */
int getRgbColor(mapObj *map,int i,int *r,int *g,int *b) {
/* check index range */
    int status=1;
    *r=*g=*b=-1;
    if ((i > 0 ) && (i <= map->palette.numcolors) ) {
       *r=map->palette.colors[i-1].red;
       *g=map->palette.colors[i-1].green;
       *b=map->palette.colors[i-1].blue;
       status=0;
    }
    return status;
}

static int searchContextForTag(mapObj *map, char **ltags, char *tag, char *context, int requires)
{
  int i;

  if(!context) return MS_FAILURE;

  /*  printf("\tin searchContextForTag, searching %s for %s\n", context, tag); */

  if(strstr(context, tag) != NULL) return MS_SUCCESS; /* found the tag */

  /* check referenced layers for the tag too */
  for(i=0; i<map->numlayers; i++) {
    if(strstr(context, ltags[i]) != NULL) { /* need to check this layer */
      if(requires == MS_TRUE) {
        if(searchContextForTag(map, ltags, tag, GET_LAYER(map, i)->requires, MS_TRUE) == MS_SUCCESS) return MS_SUCCESS;
      } else {
        if(searchContextForTag(map, ltags, tag, GET_LAYER(map, i)->labelrequires, MS_FALSE) == MS_SUCCESS) return MS_SUCCESS;      
      }
    }
  }

  return MS_FAILURE;
}

/*
** Function to take a look at all layers with REQUIRES/LABELREQUIRES set to make sure there are no 
** recursive context requirements set (e.g. layer1 requires layer2 and layer2 requires layer1). This
** is bug 1059.
*/
int msValidateContexts(mapObj *map) 
{
  int i;
  char **ltags;
  int status = MS_SUCCESS;

  ltags = (char **) malloc(map->numlayers*sizeof(char *));
  for(i=0; i<map->numlayers; i++) {
    if(GET_LAYER(map, i)->name == NULL) {
      ltags[i] = strdup("[NULL]");
    } else {
      ltags[i] = (char *) malloc(sizeof(char)*strlen(GET_LAYER(map, i)->name) + 3);
      sprintf(ltags[i], "[%s]", GET_LAYER(map, i)->name);
    }
  }

  /* check each layer's REQUIRES and LABELREQUIRES parameters */
  for(i=0; i<map->numlayers; i++) { 
    /* printf("working on layer %s, looking for references to %s\n", GET_LAYER(map, i)->name, ltags[i]); */
    if(searchContextForTag(map, ltags, ltags[i], GET_LAYER(map, i)->requires, MS_TRUE) == MS_SUCCESS) {
      msSetError(MS_PARSEERR, "Recursion error found for REQUIRES parameter for layer %s.", "msValidateContexts", GET_LAYER(map, i)->name);
      status = MS_FAILURE;
      break;
    }
    if(searchContextForTag(map, ltags, ltags[i], GET_LAYER(map, i)->labelrequires, MS_FALSE) == MS_SUCCESS) {
      msSetError(MS_PARSEERR, "Recursion error found for LABELREQUIRES parameter for layer %s.", "msValidateContexts", GET_LAYER(map, i)->name);
      status = MS_FAILURE;
      break;
    }
    /* printf("done layer %s\n", GET_LAYER(map, i)->name); */
  }

  /* clean up */
  msFreeCharArray(ltags, map->numlayers);

  return status;
}

int msEvalContext(mapObj *map, layerObj *layer, char *context)
{
  int i, status;
  char *tmpstr1=NULL, *tmpstr2=NULL;
  int result;       /* result of expression parsing operation */

  if(!context) return(MS_TRUE); /* no context requirements */

  tmpstr1 = strdup(context);

  for(i=0; i<map->numlayers; i++) { /* step through all the layers */
    if(layer->index == i) continue; /* skip the layer in question */    
    if (GET_LAYER(map, i)->name == NULL) continue; /* Layer without name cannot be used in contexts */

    tmpstr2 = (char *)malloc(sizeof(char)*strlen(GET_LAYER(map, i)->name) + 3);
    sprintf(tmpstr2, "[%s]", GET_LAYER(map, i)->name);

    if(strstr(tmpstr1, tmpstr2)) {
      if(msLayerIsVisible(map, (GET_LAYER(map, i))))
          tmpstr1 = msReplaceSubstring(tmpstr1, tmpstr2, "1");
      else
          tmpstr1 = msReplaceSubstring(tmpstr1, tmpstr2, "0");
    }

    free(tmpstr2);
  }

  msAcquireLock( TLOCK_PARSER );
  msyystate = MS_TOKENIZE_EXPRESSION; msyystring = tmpstr1;
  status = msyyparse();
  result = msyyresult;
  msReleaseLock( TLOCK_PARSER );
  free(tmpstr1);

  if (status != 0) {
    msSetError(MS_PARSEERR, "Failed to parse context", "msEvalContext");
    return MS_FALSE; /* error in parse */
  }

  return result;
}

/* msEvalExpression()
 *
 * Evaluates a mapserver expression for a given set of attribute values and
 * returns the result of the expression (MS_TRUE or MS_FALSE)
 * May also return MS_FALSE in case of parsing errors or invalid expressions
 * (check the error stack if you care)
 *
 * Parser mutex added for type MS_EXPRESSION -- SG
 */
int msEvalExpression(expressionObj *expression, int itemindex, char **items, int numitems)
{
  int i;
  char *tmpstr=NULL, *tmpstr2=NULL;
  int status;
  int expresult;  /* result of logical expression parsing operation */
  
  if(!expression->string) return(MS_TRUE); /* empty expressions are ALWAYS true */

  switch(expression->type) {
  case(MS_STRING):
    if(itemindex == -1) {
      msSetError(MS_MISCERR, "Cannot evaluate expression, no item index defined.", "msEvalExpression()");
      return(MS_FALSE);
    }
    if(itemindex >= numitems) {
      msSetError(MS_MISCERR, "Invalid item index.", "msEvalExpression()");
      return(MS_FALSE);
    }
    if(expression->flags & MS_EXP_INSENSITIVE) {
      if(strcasecmp(expression->string, items[itemindex]) == 0) return(MS_TRUE); /* got a match */
    } else {
      if(strcmp(expression->string, items[itemindex]) == 0) return(MS_TRUE); /* got a match */
    }
    break;
  case(MS_EXPRESSION):
    tmpstr = strdup(expression->string);

    for(i=0; i<expression->numitems; i++) {
      tmpstr2 = strdup(items[expression->indexes[i]]);
      tmpstr2 = msReplaceSubstring(tmpstr2, "\'", "\\\'");
      tmpstr2 = msReplaceSubstring(tmpstr2, "\"", "\\\"");
      tmpstr = msReplaceSubstring(tmpstr, expression->items[i], tmpstr2);
      free(tmpstr2);
    }

    // fprintf(stderr, "exp; %s\n", tmpstr);

    msAcquireLock( TLOCK_PARSER );
    msyystate = MS_TOKENIZE_EXPRESSION;
    msyystring = tmpstr; /* set lexer state to EXPRESSION_STRING */
    status = msyyparse();
    expresult = msyyresult;
    msReleaseLock( TLOCK_PARSER );

    if (status != 0) {
      msSetError(MS_PARSEERR, "Failed to parse expression: %s", "msEvalExpression", tmpstr);
      free(tmpstr);
      return MS_FALSE;
    }

    free(tmpstr);
    return expresult;
    
    break;
  case(MS_REGEX):
    if(itemindex == -1) {
      msSetError(MS_MISCERR, "Cannot evaluate expression, no item index defined.", "msEvalExpression()");
      return(MS_FALSE);
    }
    if(itemindex >= numitems) {
      msSetError(MS_MISCERR, "Invalid item index.", "msEvalExpression()");
      return(MS_FALSE);
    }

    if(!expression->compiled) {
      if(expression->flags & MS_EXP_INSENSITIVE)
      {
        if(ms_regcomp(&(expression->regex), expression->string, MS_REG_EXTENDED|MS_REG_NOSUB|MS_REG_ICASE) != 0) { /* compile the expression */
          msSetError(MS_REGEXERR, "Invalid regular expression.", "msEvalExpression()");
          return(MS_FALSE);
        }
      }
      else
      {
        if(ms_regcomp(&(expression->regex), expression->string, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) { /* compile the expression */
          msSetError(MS_REGEXERR, "Invalid regular expression.", "msEvalExpression()");
          return(MS_FALSE);
        }
      }
      expression->compiled = MS_TRUE;
    }

    if(ms_regexec(&(expression->regex), items[itemindex], 0, NULL, 0) == 0) return(MS_TRUE); /* got a match */
    break;
  }

  return(MS_FALSE);
}

/*
 * int msShapeGetClass(layerObj *layer, shapeObj *shape)
 * {
 *   int i;
 * 
 *   for(i=0; i<layer->numclasses; i++) {
 *     if(layer->class[i]->status != MS_DELETE && msEvalExpression(&(layer->class[i]->expression), layer->classitemindex, shape->values, layer->numitems) == MS_TRUE)
 *       return(i);
 *   }
 * 
 *   return(-1); // no match
 * }
 */

int *msAllocateValidClassGroups(layerObj *lp, int *nclasses)
{
    int *classgroup = NULL;
    int nvalidclass = 0, i=0;

    if (!lp || !lp->classgroup || lp->numclasses <=0 || !nclasses)
      return NULL;

    classgroup = (int *)malloc(sizeof(int)*lp->numclasses);       
    nvalidclass = 0;
    for (i=0; i<lp->numclasses; i++)
    {
        if (lp->class[i]->group && strcasecmp(lp->class[i]->group, lp->classgroup) == 0)
        {
            classgroup[nvalidclass] = i;
            nvalidclass++;
        }
    }
    if (nvalidclass > 0)
    {
        classgroup = (int *)realloc(classgroup, sizeof(int)*nvalidclass);
        *nclasses = nvalidclass;
        return classgroup;
    }

    if (classgroup)
      msFree(classgroup);
    
    return NULL;
        
}       

int msShapeGetClass(layerObj *layer, shapeObj *shape, double scaledenom, int *classgroup, int numclasses)
{
  int i, iclass;


  /* INLINE features do not work with expressions, allow the classindex */
  /* value set prior to calling this function to carry through. */
  if(layer->connectiontype == MS_INLINE) {
    if(shape->classindex < 0 || shape->classindex >= layer->numclasses) return(-1);

    if(scaledenom > 0) {  /* verify scaledenom here */
      if((layer->class[shape->classindex]->maxscaledenom > 0) && (scaledenom > layer->class[shape->classindex]->maxscaledenom))
        return(-1); /* can skip this feature */
      if((layer->class[shape->classindex]->minscaledenom > 0) && (scaledenom <= layer->class[shape->classindex]->minscaledenom))
        return(-1); /* can skip this feature */
    }

    return(shape->classindex);
  }

  if (layer->numclasses > 0)
  {

      if (classgroup == NULL || numclasses <=0)
        numclasses = layer->numclasses;

      for(i=0; i<numclasses; i++) 
      {
          if (classgroup)
            iclass = classgroup[i];
          else
            iclass = i;

          if (iclass < 0 || iclass >= layer->numclasses)        
            continue; /*this should never happen but just in case*/

          if(scaledenom > 0) {  /* verify scaledenom here  */
              if((layer->class[iclass]->maxscaledenom > 0) && (scaledenom > layer->class[iclass]->maxscaledenom))
                continue; /* can skip this one, next class */
              if((layer->class[iclass]->minscaledenom > 0) && (scaledenom <= layer->class[iclass]->minscaledenom))
                continue; /* can skip this one, next class */
          }

          if(layer->class[iclass]->status != MS_DELETE && 
             msEvalExpression(&(layer->class[iclass]->expression), 
                              layer->classitemindex, shape->values, layer->numitems) == MS_TRUE)
          {
              return(iclass);
          }
      }
  }
  return(-1); /* no match */
}

char *msShapeGetAnnotation(layerObj *layer, shapeObj *shape)
{
  int i;
  char *tmpstr=NULL;

  if(layer->class[shape->classindex]->text.string) { /* test for global label first */
    tmpstr = strdup(layer->class[shape->classindex]->text.string);
    switch(layer->class[shape->classindex]->text.type) {
    case(MS_STRING):
      break;
    case(MS_EXPRESSION):
      tmpstr = strdup(layer->class[shape->classindex]->text.string);

      for(i=0; i<layer->class[shape->classindex]->text.numitems; i++)
        tmpstr = msReplaceSubstring(tmpstr, layer->class[shape->classindex]->text.items[i], shape->values[layer->class[shape->classindex]->text.indexes[i]]);
      break;
    }
  } else {
    if (shape->values && layer->labelitemindex >= 0)
        tmpstr = strdup(shape->values[layer->labelitemindex]);
  }

  return(tmpstr);
}

/*
** Adjusts an image size in one direction to fit an extent.
*/
int msAdjustImage(rectObj rect, int *width, int *height)
{
  if(*width == -1 && *height == -1) {
    msSetError(MS_MISCERR, "Cannot calculate both image height and width.", "msAdjustImage()");
    return(-1);
  }

  if(*width > 0)
    *height = MS_NINT((rect.maxy - rect.miny)/((rect.maxx - rect.minx)/(*width)));
  else
    *width = MS_NINT((rect.maxx - rect.minx)/((rect.maxy - rect.miny)/(*height)));

  return(0);
}

/*
** Make sure extent fits image window to be created. Returns cellsize of output image.
*/
double msAdjustExtent(rectObj *rect, int width, int height)
{
  double cellsize, ox, oy;

  cellsize = MS_MAX(MS_CELLSIZE(rect->minx, rect->maxx, width), MS_CELLSIZE(rect->miny, rect->maxy, height));

  if(cellsize <= 0) /* avoid division by zero errors */
    return(0);

  ox = MS_MAX(((width-1) - (rect->maxx - rect->minx)/cellsize)/2,0); /* these were width-1 and height-1 */
  oy = MS_MAX(((height-1) - (rect->maxy - rect->miny)/cellsize)/2,0);

  rect->minx = rect->minx - ox*cellsize;
  rect->miny = rect->miny - oy*cellsize;
  rect->maxx = rect->maxx + ox*cellsize;
  rect->maxy = rect->maxy + oy*cellsize;

  return(cellsize);
}

/*
** Rect must always contain a portion of bounds. If not, rect is 
** shifted to overlap by overlay percent. The dimensions of rect do
** not change but placement relative to bounds can.
*/
int msConstrainExtent(rectObj *bounds, rectObj *rect, double overlay) 
{
  double offset=0;

  /* check left edge, and if necessary the right edge of bounds */
  if(rect->maxx <= bounds->minx) {
    offset = overlay*(rect->maxx - rect->minx);
    rect->minx += offset; /* shift right */
    rect->maxx += offset;
  } else if(rect->minx >= bounds->maxx) {
    offset = overlay*(rect->maxx - rect->minx);
    rect->minx -= offset; /* shift left */
    rect->maxx -= offset;
  }

  /* check top edge, and if necessary the bottom edge of bounds */
  if(rect->maxy <= bounds->miny) {
    offset = overlay*(rect->maxy - rect->miny);
    rect->miny -= offset; /* shift down */
    rect->maxy -= offset;
  } else if(rect->miny >= bounds->maxy) {
    offset = overlay*(rect->maxy - rect->miny);
    rect->miny += offset; /* shift up */
    rect->maxy += offset;
  }

  return(MS_SUCCESS);
}

/*
** Generic function to save an image to a file.
**
** Note that map may be NULL. If it is set, then it is used for two things:
** - Deal with relative imagepaths (compute absolute path relative to map path)
** - Extract the georeferenced extents and coordinate system
**   of the map for writing out with the image when appropriate 
**   (primarily this means via msSaveImageGDAL() to something like GeoTIFF). 
**
** The filename is NULL when the image is supposed to be written to stdout. 
*/

int msSaveImage(mapObj *map, imageObj *img, char *filename)
{
    int nReturnVal = -1;
    char szPath[MS_MAXPATHLEN];
    struct mstimeval starttime, endtime;

    if(map && map->debug >= MS_DEBUGLEVEL_TUNING) 
        msGettimeofday(&starttime, NULL);

    if (img)
    {
        if (MS_RENDERER_PLUGIN(img->format)) {
            rendererVTableObj *renderer = img->format->vtable;
            FILE *stream;
            if(filename) 
                stream = fopen(msBuildPath(szPath, map->mappath, filename),"wb");
            else {
                if ( msIO_needBinaryStdout() == MS_FAILURE )
                    return MS_FAILURE;
                stream = stdout;
            }
            if(!stream)
                return MS_FAILURE;
            if(renderer->supports_pixel_buffer) {
                rasterBufferObj data;
                renderer->getRasterBuffer(img,&data);

                msSaveRasterBuffer(&data,stream,img->format );
            } else {
                renderer->saveImage(img, stream, img->format);
            }
            fclose(stream);
            return MS_SUCCESS;
        }
    	else if( MS_DRIVER_GD(img->format) )
        {
            if(map != NULL && filename != NULL )
                nReturnVal = msSaveImageGD(img, 
                                           msBuildPath(szPath, map->mappath, 
                                                       filename), 
                                           img->format );
            else
                nReturnVal = msSaveImageGD(img, filename, img->format);
        }
#ifdef USE_AGG
        else if( MS_DRIVER_AGG(img->format) )
        {
            if(map != NULL && filename != NULL )
                nReturnVal = msSaveImageAGG(img, 
                                           msBuildPath(szPath, map->mappath, 
                                                       filename), 
                                           img->format );
            else
                nReturnVal = msSaveImageAGG(img, filename, img->format);
        }
#endif
        else if( MS_DRIVER_IMAGEMAP(img->format) )
            nReturnVal = msSaveImageIM(img, filename, img->format);
#ifdef USE_GDAL
        else if( MS_DRIVER_GDAL(img->format) )
        {
           if (map != NULL && filename != NULL )
             nReturnVal = msSaveImageGDAL(map, img,
                                          msBuildPath(szPath, map->mappath, 
                                                      filename));
           else
             nReturnVal = msSaveImageGDAL(map, img, filename);
        }
#endif
#ifdef USE_MING_FLASH
        else if(MS_DRIVER_SWF(img->format) )
        {
            if (map != NULL && filename != NULL )
              nReturnVal = msSaveImageSWF(img, 
                                          msBuildPath(szPath, map->mappath, 
                                                      filename));
            else
              nReturnVal = msSaveImageSWF(img, filename);
        }

#endif
#ifdef USE_PDF
        else if( MS_RENDERER_PDF(img->format) )
        {
            if (map != NULL && filename != NULL )
              nReturnVal = msSaveImagePDF(img, 
                                          msBuildPath(szPath, map->mappath, 
                                                      filename));
            else
              nReturnVal = msSaveImagePDF(img, filename);
        }
#endif
        else if(MS_DRIVER_SVG(img->format) )
        {
            if (map != NULL && filename != NULL )
              nReturnVal = msSaveImageSVG(img, 
                                          msBuildPath(szPath, map->mappath, 
                                                      filename));
            else
              nReturnVal = msSaveImageSVG(img, filename);
        }

        else
            msSetError(MS_MISCERR, "Unknown image type", 
                       "msSaveImage()"); 
    }

    if(map && map->debug >= MS_DEBUGLEVEL_TUNING) {
      msGettimeofday(&endtime, NULL);
      msDebug("msSaveImage() total time: %.3fs\n", 
              (endtime.tv_sec+endtime.tv_usec/1.0e6)-
              (starttime.tv_sec+starttime.tv_usec/1.0e6) );
    }

    return nReturnVal;
}

/*
** Generic function to save an image to a byte array.
** - the return value is the pointer to the byte array 
** - size_ptr contains the number of bytes returned
** - format: the desired output format
**
** The caller is responsible to free the returned array
** The function returns NULL if the output format is not supported. 
*/

unsigned char *msSaveImageBuffer(imageObj* image, int *size_ptr, outputFormatObj *format)
{
    *size_ptr = 0;
    if( MS_RENDERER_PLUGIN(image->format)){
        rasterBufferObj data;
        rendererVTableObj *renderer = image->format->vtable;
        if(renderer->supports_pixel_buffer) {
            bufferObj buffer;
            msBufferInit(&buffer);
            renderer->getRasterBuffer(image,&data);
            msSaveRasterBufferToBuffer(&data,&buffer,format);
            return buffer.data;
            //don't free the bufferObj as we don't own the bytes anymore
        } else {
	        msSetError(MS_MISCERR, "Unsupported image type", "msSaveImageBuffer()");
            return NULL;
        }
    }
    else if( MS_DRIVER_GD(image->format) )
    {
        return msSaveImageBufferGD(image, size_ptr, format);
    }
#ifdef USE_AGG
    else if( MS_DRIVER_AGG(image->format) )
    {
        return msSaveImageBufferAGG(image, size_ptr, format);
    }
#endif   
	
	msSetError(MS_MISCERR, "Unsupported image type", "msSaveImage()");
    return NULL;
}

/**
 * Generic function to free the imageObj
 */
void msFreeImage(imageObj *image)
{
    if (image)
    {
        if(MS_RENDERER_PLUGIN(image->format)) {
            rendererVTableObj *renderer = image->format->vtable;
            if(renderer->supports_imagecache) {
                tileCacheObj *next,*cur = image->tilecache;
                while(cur) {
                    renderer->freeTile(cur->data);
                    next = cur->next;
                    free(cur);
                    cur = next;
                }
                image->ntiles = 0;
            }
        	renderer->freeImage(image);
        }
        else if( MS_RENDERER_GD(image->format) ) {
            if( image->img.gd != NULL )
                msFreeImageGD(image);
#ifdef USE_AGG
        } else if( MS_RENDERER_AGG(image->format) ) {
            msFreeImageAGG(image);
#endif
        } else if( MS_RENDERER_IMAGEMAP(image->format) )
            msFreeImageIM(image);
        else if( MS_RENDERER_RAWDATA(image->format) )
            msFree(image->img.raw_16bit);
#ifdef USE_MING_FLASH
        else if( MS_RENDERER_SWF(image->format) )
            msFreeImageSWF(image);
#endif
#ifdef USE_PDF
        else if( MS_RENDERER_PDF(image->format) )
            msFreeImagePDF(image);
#endif
        else if( MS_RENDERER_SVG(image->format) )
            msFreeImageSVG(image);
        else
            msSetError(MS_MISCERR, "Unknown image type", 
                       "msFreeImage()"); 

        if (image->imagepath)
            free(image->imagepath);
        if (image->imageurl)
            free(image->imageurl);

        if( --image->format->refcount < 1 )
            msFreeOutputFormat( image->format );

        image->imagepath = NULL;
        image->imageurl = NULL;

        msFree( image );
    }     
}

/*
** Return an array containing all the layer's index given a group name.
** If nothing is found, NULL is returned. The nCount is initalized
** to the number of elements in the returned array.
** Note : the caller of the function should free the array.
*/
int *msGetLayersIndexByGroup(mapObj *map, char *groupname, int *pnCount)
{
    int         i;
    int         iLayer = 0;
    int         *aiIndex;

    if(!groupname || !map || !pnCount)
    {
        return NULL;
    }

    aiIndex = (int *)malloc(sizeof(int) * map->numlayers);

    for(i=0;i<map->numlayers; i++)
    {
        if(!GET_LAYER(map, i)->group) /* skip it */
            continue;
        if(strcmp(groupname, GET_LAYER(map, i)->group) == 0)
        {
            aiIndex[iLayer] = i;
            iLayer++;
        }
    }

    if (iLayer == 0)
    {
        free(aiIndex);
        aiIndex = NULL;
        *pnCount = 0;
    }
    else
    {
        aiIndex = (int *)realloc(aiIndex, sizeof(int)* iLayer);
        *pnCount = iLayer;
    }

  return aiIndex;
}

/* ==================================================================== */
/*      Measured shape utility functions.                               */
/* ==================================================================== */


/************************************************************************/
/*        pointObj *msGetPointUsingMeasure(shapeObj *shape, double m)   */
/*                                                                      */
/*      Using a measured value get the XY location it corresonds        */
/*      to.                                                             */
/*                                                                      */
/************************************************************************/
pointObj *msGetPointUsingMeasure(shapeObj *shape, double m)
{
#ifdef USE_POINT_Z_M
    pointObj    *point = NULL;
    lineObj     line;
    double      dfMin = 0;
    double      dfMax = 0;
    int         i,j = 0;
    int         bFound = 0;
    double      dfFirstPointX = 0;
    double      dfFirstPointY = 0;
    double      dfFirstPointM = 0;
    double      dfSecondPointX = 0;
    double      dfSecondPointY = 0;
    double      dfSecondPointM = 0;
    double      dfCurrentM = 0;
    double      dfFactor = 0;

    if (shape &&  shape->numlines > 0)
    {
/* -------------------------------------------------------------------- */
/*      check fir the first value (min) and the last value(max) to      */
/*      see if the m is contained between these min and max.            */
/* -------------------------------------------------------------------- */
        line = shape->line[0];
        dfMin = line.point[0].m;
        line = shape->line[shape->numlines-1];
        dfMax = line.point[line.numpoints-1].m;

        if (m >= dfMin && m <= dfMax)
        {
            for (i=0; i<shape->numlines; i++)
            {
                line = shape->line[i];
                
                for (j=0; j<line.numpoints; j++)
                {
                    dfCurrentM = line.point[j].m;
                    if (dfCurrentM > m)
                    {
                        bFound = 1;
                        
                        dfSecondPointX = line.point[j].x;
                        dfSecondPointY = line.point[j].y;
                        dfSecondPointM = line.point[j].m;
                        
/* -------------------------------------------------------------------- */
/*      get the previous node xy values.                                */
/* -------------------------------------------------------------------- */
                        if (j > 0) /* not the first point of the line */
                        {
                            dfFirstPointX = line.point[j-1].x;
                            dfFirstPointY = line.point[j-1].y;
                            dfFirstPointM = line.point[j-1].m;
                        }
                        else /* get last point of previous line */
                        {
                            dfFirstPointX = shape->line[i-1].point[0].x;
                            dfFirstPointY = shape->line[i-1].point[0].y;
                            dfFirstPointM = shape->line[i-1].point[0].m;
                        }
                        break;
                    }
                }
            }
        }

        if (!bFound) 
          return NULL;

/* -------------------------------------------------------------------- */
/*      extrapolate the m value to get t he xy coordinate.              */
/* -------------------------------------------------------------------- */

        if (dfFirstPointM != dfSecondPointM) 
          dfFactor = (m-dfFirstPointM)/(dfSecondPointM - dfFirstPointM); 
        else
          dfFactor = 0;

        point = (pointObj *)malloc(sizeof(pointObj));
        
        point->x = dfFirstPointX + (dfFactor * (dfSecondPointX - dfFirstPointX));
        point->y = dfFirstPointY + 
            (dfFactor * (dfSecondPointY - dfFirstPointY));
        point->m = dfFirstPointM + 
            (dfFactor * (dfSecondPointM - dfFirstPointM));
        
        return point;
    }

    return NULL;
#else
    msSetError(MS_MISCERR, 
               "The \"m\" parameter for points is unavailable in your build.",
               "msGetPointUsingMeasure()");
    return NULL;
#endif /* USE_POINT_Z_M */
}


/************************************************************************/
/*       IntersectionPointLinepointObj *p, pointObj *a, pointObj *b)    */
/*                                                                      */
/*      Retunrs a point object corresponding to the intersection of     */
/*      point p and a line formed of 2 points : a and b.                */
/*                                                                      */
/*      Alorith base on :                                               */
/*      http://www.faqs.org/faqs/graphics/algorithms-faq/               */
/*                                                                      */
/*      Subject 1.02:How do I find the distance from a point to a line? */
/*                                                                      */
/*          Let the point be C (Cx,Cy) and the line be AB (Ax,Ay) to (Bx,By).*/
/*          Let P be the point of perpendicular projection of C on AB.  The parameter*/
/*          r, which indicates P's position along AB, is computed by the dot product */
/*          of AC and AB divided by the square of the length of AB:     */
/*                                                                      */
/*          (1)     AC dot AB                                           */
/*              r = ---------                                           */
/*                  ||AB||^2                                            */
/*                                                                      */
/*          r has the following meaning:                                */
/*                                                                      */
/*              r=0      P = A                                          */
/*              r=1      P = B                                          */
/*              r<0      P is on the backward extension of AB           */
/*              r>1      P is on the forward extension of AB            */
/*              0<r<1    P is interior to AB                            */
/*                                                                      */
/*          The length of a line segment in d dimensions, AB is computed by:*/
/*                                                                      */
/*              L = sqrt( (Bx-Ax)^2 + (By-Ay)^2 + ... + (Bd-Ad)^2)      */
/*                                                                      */
/*          so in 2D:                                                   */
/*                                                                      */
/*              L = sqrt( (Bx-Ax)^2 + (By-Ay)^2 )                       */
/*                                                                      */
/*          and the dot product of two vectors in d dimensions, U dot V is computed:*/
/*                                                                      */
/*              D = (Ux * Vx) + (Uy * Vy) + ... + (Ud * Vd)             */
/*                                                                      */
/*          so in 2D:                                                   */
/*                                                                      */
/*              D = (Ux * Vx) + (Uy * Vy)                               */
/*                                                                      */
/*          So (1) expands to:                                          */
/*                                                                      */
/*                  (Cx-Ax)(Bx-Ax) + (Cy-Ay)(By-Ay)                     */
/*              r = -------------------------------                     */
/*                                L^2                                   */
/*                                                                      */
/*          The point P can then be found:                              */
/*                                                                      */
/*              Px = Ax + r(Bx-Ax)                                      */
/*              Py = Ay + r(By-Ay)                                      */
/*                                                                      */
/*          And the distance from A to P = r*L.                         */
/*                                                                      */
/*          Use another parameter s to indicate the location along PC, with the */
/*          following meaning:                                          */
/*                 s<0      C is left of AB                             */
/*                 s>0      C is right of AB                            */
/*                 s=0      C is on AB                                  */
/*                                                                      */
/*          Compute s as follows:                                       */
/*                                                                      */
/*                  (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)                       */
/*              s = -----------------------------                       */
/*                              L^2                                     */
/*                                                                      */
/*                                                                      */
/*          Then the distance from C to P = |s|*L.                      */
/*                                                                      */
/************************************************************************/
pointObj *msIntersectionPointLine(pointObj *p, pointObj *a, pointObj *b)
{
    double r = 0;
    double L = 0;
    pointObj *result = NULL;

    if (p && a && b)
    {
        L = sqrt(((b->x - a->x)*(b->x - a->x)) + 
                 ((b->y - a->y)*(b->y - a->y)));

        if (L != 0)
          r = ((p->x - a->x)*(b->x - a->x) + (p->y - a->y)*(b->y - a->y))/(L*L);
        else
          r = 0;

        result = (pointObj *)malloc(sizeof(pointObj));
/* -------------------------------------------------------------------- */
/*      We want to make sure that the point returned is on the line     */
/*                                                                      */
/*              r=0      P = A                                          */
/*              r=1      P = B                                          */
/*              r<0      P is on the backward extension of AB           */
/*              r>1      P is on the forward extension of AB            */
/*                    0<r<1    P is interior to AB                      */
/* -------------------------------------------------------------------- */
        if (r < 0)
        {
            result->x = a->x;
            result->y = a->y;
        }
        else if (r > 1)
        {
            result->x = b->x;
            result->y = b->y;
        }
        else
        {
            result->x = a->x + r*(b->x - a->x);
            result->y = a->y + r*(b->y - a->y);
        }
#ifdef USE_POINT_Z_M
        result->m = 0;
#endif
    }

    return result;
}


/************************************************************************/
/*         pointObj *msGetMeasureUsingPoint(shapeObj *shape, pointObj   */
/*      *point)                                                         */
/*                                                                      */
/*      Calculate the intersection point betwwen the point and the      */
/*      shape and return the Measured value at the intersection.        */
/************************************************************************/
pointObj *msGetMeasureUsingPoint(shapeObj *shape, pointObj *point)
{       
    double      dfMinDist = 1e35;
    double      dfDist = 0;
    pointObj    oFirst;
    pointObj    oSecond;
    int         i, j = 0;
    lineObj     line;
    pointObj    *poIntersectionPt = NULL;
    double      dfFactor = 0;
    double      dfDistTotal, dfDistToIntersection = 0;

    if (shape && point)
    {
        for (i=0; i<shape->numlines; i++)
        {
            line = shape->line[i];
/* -------------------------------------------------------------------- */
/*      for each line (2 consecutive lines) get the distance between    */
/*      the line and the point and determine which line segment is      */
/*      the closeset to the point.                                      */
/* -------------------------------------------------------------------- */
            for (j=0; j<line.numpoints-1; j++)
            {
                dfDist = msDistancePointToSegment(point, &line.point[j], &line.point[j+1]);
                if (dfDist < dfMinDist)
                {
                    oFirst.x = line.point[j].x;
                    oFirst.y = line.point[j].y;
#ifdef USE_POINT_Z_M
                    oFirst.m = line.point[j].m;
#endif
                    
                    oSecond.x =  line.point[j+1].x;
                    oSecond.y =  line.point[j+1].y;
#ifdef USE_POINT_Z_M
                    oSecond.m =  line.point[j+1].m;
#endif

                    dfMinDist = dfDist;
                }
            }
        }
/* -------------------------------------------------------------------- */
/*      once we have the nearest segment, look for the x,y location     */
/*      which is the nearest intersection between the line and the      */
/*      point.                                                          */
/* -------------------------------------------------------------------- */
        poIntersectionPt = msIntersectionPointLine(point, &oFirst, &oSecond);
        if (poIntersectionPt)
        {
            dfDistTotal = sqrt(((oSecond.x - oFirst.x)*(oSecond.x - oFirst.x)) + 
                               ((oSecond.y - oFirst.y)*(oSecond.y - oFirst.y)));

            dfDistToIntersection = sqrt(((poIntersectionPt->x - oFirst.x)*
                                         (poIntersectionPt->x - oFirst.x)) + 
                                        ((poIntersectionPt->y - oFirst.y)*
                                         (poIntersectionPt->y - oFirst.y)));

            dfFactor = dfDistToIntersection / dfDistTotal;

#ifdef USE_POINT_Z_M
            poIntersectionPt->m = oFirst.m + (oSecond.m - oFirst.m)*dfFactor;
#endif

            return poIntersectionPt;
        }
    
    }
    return NULL;
}

/* ==================================================================== */
/*   End   Measured shape utility functions.                            */
/* ==================================================================== */


char **msGetAllGroupNames(mapObj *map, int *numTok)
{
    char        **papszGroups = NULL;
    int         bFound = 0;
    int         nCount = 0;
    int         i = 0, j = 0;

    *numTok = 0;
   
    if (!map->layerorder)
    {
       map->layerorder = (int*)malloc(map->numlayers * sizeof(int));

       /*
        * Initiate to default order
        */
       for (i=0; i<map->numlayers; i++)
         map->layerorder[i] = i;   
    }
   
    if (map != NULL && map->numlayers > 0)
    {
        nCount = map->numlayers;
        papszGroups = (char **)malloc(sizeof(char *)*nCount);

        for (i=0; i<nCount; i++)
            papszGroups[i] = NULL;
       
        for (i=0; i<nCount; i++)
        {
            layerObj *lp;
            lp = (GET_LAYER(map, map->layerorder[i]));

            bFound = 0;
            if (lp->group && lp->status != MS_DELETE)
            {
                for (j=0; j<*numTok; j++)
                {
                    if (papszGroups[j] &&
                        strcmp(lp->group, papszGroups[j]) == 0)
                    {
                        bFound = 1;
                        break;
                    }
                }
                if (!bFound)
                {
                    /* New group... add to the list of groups found */
                    papszGroups[(*numTok)] = strdup(lp->group);
                    (*numTok)++;
                }
            }
        }

    }
   
    return papszGroups;
}

/************************************************************************/
/*                         msForceTmpFileBase()                         */
/************************************************************************/

static int tmpCount = 0;
static char *ForcedTmpBase = NULL;

void msForceTmpFileBase( const char *new_base )
{
/* -------------------------------------------------------------------- */
/*      Clear previous setting, if any.                                 */
/* -------------------------------------------------------------------- */
    if( ForcedTmpBase != NULL )
    {
        free( ForcedTmpBase );
        ForcedTmpBase = NULL;
    }
    
    tmpCount = -1;

    if( new_base == NULL )
        return;

/* -------------------------------------------------------------------- */
/*      Record new base.                                                */
/* -------------------------------------------------------------------- */
    ForcedTmpBase = strdup( new_base );
    tmpCount = 0;
}

/**********************************************************************
 *                          msTmpFile()
 *
 * Generate a Unique temporary filename using:
 * 
 *    PID + timestamp + sequence# + extension
 * 
 * If msForceTmpFileBase() has been called to control the temporary filename
 * then the filename will be:
 *
 *    TmpBase + sequence# + extension
 * 
 * Returns char* which must be freed by caller.
 **********************************************************************/
char *msTmpFile(const char *mappath, const char *tmppath, const char *ext)
{
    char *tmpFname;
    char szPath[MS_MAXPATHLEN];
    const char *fullFname;
    char tmpId[128]; /* big enough for time + pid + ext */
    const char *tmpBase = NULL;

    if( ForcedTmpBase != NULL )
    {
        tmpBase = ForcedTmpBase;
    }
    else 
    {
        /* We'll use tmpId and tmpCount to generate unique filenames */
        sprintf(tmpId, "%lx_%x",(long)time(NULL),(int)getpid());
        tmpBase = tmpId;
    }

    if (ext == NULL)  ext = "";
    tmpFname = (char*)malloc(strlen(tmpBase) + 10  + strlen(ext) + 1);

    msAcquireLock( TLOCK_TMPFILE );
    sprintf(tmpFname, "%s_%x.%s", tmpBase, tmpCount++, ext);
    msReleaseLock( TLOCK_TMPFILE );

    fullFname = msBuildPath3(szPath, mappath, tmppath, tmpFname);
    free(tmpFname);

    if (fullFname)
        return strdup(fullFname);

    return NULL;
}

/**
 *  Generic function to Initalize an image object.
 */
imageObj *msImageCreate(int width, int height, outputFormatObj *format, 
                        char *imagepath, char *imageurl, mapObj *map)
{
    imageObj *image = NULL;
    if( MS_RENDERER_GD(format) )
    {
        image = msImageCreateGD(width, height, format,
                                imagepath, imageurl, map->resolution, map->defresolution);
        if( image != NULL && map) msImageInitGD( image, &map->imagecolor );
    }
    else if(MS_RENDERER_PLUGIN(format)) {
    	image = format->vtable->createImage(width,height,format,&map->imagecolor);
    	image->format = format;
		format->refcount++;

		image->width = width;
		image->height = height;
		image->imagepath = NULL;
		image->imageurl = NULL;
                image->tilecache = NULL;
                image->ntiles = 0;
                image->resolution = map->resolution;
                image->resolutionfactor = map->resolution/map->defresolution;

		if (imagepath)
			image->imagepath = strdup(imagepath);
		if (imageurl)
			image->imageurl = strdup(imageurl);

		return image;
    }
#ifdef USE_AGG
    else if( MS_RENDERER_AGG(format) )
    {
        image = msImageCreateAGG(width, height, format,
                                 imagepath, imageurl, map->resolution, map->defresolution);
        if( image != NULL && map) msImageInitAGG( image, &map->imagecolor );
    }
#endif
    else if( MS_RENDERER_RAWDATA(format) )
    {
        if( format->imagemode != MS_IMAGEMODE_INT16
            && format->imagemode != MS_IMAGEMODE_FLOAT32 
            && format->imagemode != MS_IMAGEMODE_BYTE )
        {
            msSetError(MS_IMGERR, 
                    "Attempt to use illegal imagemode with rawdata renderer.",
                       "msImageCreate()" );
            return NULL;
        }

        image = (imageObj *)calloc(1,sizeof(imageObj));

        if( format->imagemode == MS_IMAGEMODE_INT16 )
            image->img.raw_16bit = (short *) 
                calloc(sizeof(short),width*height*format->bands);
        else if( format->imagemode == MS_IMAGEMODE_FLOAT32 )
            image->img.raw_float = (float *) 
                calloc(sizeof(float),width*height*format->bands);
        else if( format->imagemode == MS_IMAGEMODE_BYTE )
            image->img.raw_byte = (unsigned char *) 
                calloc(sizeof(unsigned char),width*height*format->bands);

        if( image->img.raw_16bit == NULL )
        {
            msFree( image );
            msSetError(MS_IMGERR, 
                       "Attempt to allocate raw image failed, out of memory.",
                       "msImageCreate()" );
            return NULL;
        }
            
        image->format = format;
        format->refcount++;

        image->width = width;
        image->height = height;
        image->imagepath = NULL;
        image->imageurl = NULL;
        image->resolution = map->resolution;
        image->resolutionfactor = map->resolution/map->defresolution;

        if (imagepath)
            image->imagepath = strdup(imagepath);
        if (imageurl)
            image->imageurl = strdup(imageurl);
            
        return image;
    }
    else if( MS_RENDERER_IMAGEMAP(format) )
    {
        image = msImageCreateIM(width, height, format,
                                imagepath, imageurl, map->resolution, map->defresolution);
        if( image != NULL ) msImageInitIM( image );
    }
#ifdef USE_MING_FLASH
    else if( MS_RENDERER_SWF(format) && map )
    {
        image = msImageCreateSWF(width, height, format,
                                 imagepath, imageurl, map);
    }
#endif
#ifdef USE_PDF
    else if( MS_RENDERER_PDF(format) && map)
    {
        image = msImageCreatePDF(width, height, format,
                                 imagepath, imageurl, map);
    }
#endif
    
    else 
    {
        msSetError(MS_MISCERR, 
               "Unsupported renderer requested, unable to initialize image.", 
                   "msImageCreate()");
        return NULL;
    }

    if(!image) 
        msSetError(MS_GDERR, "Unable to initialize image.", "msImageCreate()");

    return image;
}


/**
 * Generic function to transorm the shape coordinates to output coordinates
 */
void  msTransformShape(shapeObj *shape, rectObj extent, double cellsize, 
                       imageObj *image)   
{
	if (image != NULL && MS_RENDERER_PLUGIN(image->format)) {
		image->format->vtable->transformShape(shape, extent, cellsize);

		return;
	}
#ifdef USE_MING_FLASH
    if (image != NULL && MS_RENDERER_SWF(image->format) )
    {
        if (strcasecmp(msGetOutputFormatOption(image->format, "FULL_RESOLUTION",""), 
                       "FALSE") == 0)
          msTransformShapeToPixel(shape, extent, cellsize);
        else
          msTransformShapeSWF(shape, extent, cellsize);
          

        return;
    }
#endif
#ifdef USE_PDF
    if (image != NULL && MS_RENDERER_PDF(image->format) )
    {
        if (strcasecmp(msGetOutputFormatOption(image->format, "FULL_RESOLUTION",""), 
                       "FALSE") == 0)
          msTransformShapeToPixel(shape, extent, cellsize);
        else
          msTransformShapePDF(shape, extent, cellsize);

        return;
    }
#endif
    if (image != NULL && MS_RENDERER_SVG(image->format) )
    {
        
        msTransformShapeSVG(shape, extent, cellsize, image);

        return;
    }
#ifdef USE_AGG
    if (image != NULL && MS_RENDERER_AGG(image->format) )
    {
        msTransformShapeAGG(shape, extent, cellsize);

        return;
    }
#endif

    msTransformShapeToPixel(shape, extent, cellsize);
}

/*
** Helper functions supplied as part of bug #2868 solution. Consider moving these to
** mapprimitive.c for more general use.
*/

/* vector difference */
static pointObj point_diff(const pointObj a, const pointObj b) {
  pointObj retv = {a.x-b.x,a.y-b.y
#ifdef USE_POINT_Z_M
    ,a.z-b.z,a.m-b.m
#endif
  };
  return retv;
}

/* vector sum */
static pointObj point_sum(const pointObj a, const pointObj b) {
  pointObj retv = {a.x+b.x,a.y+b.y
#ifdef USE_POINT_Z_M
    ,a.z+b.z,a.m+b.m
#endif
  };
  return retv;
}

/* vector multiply */
static pointObj point_mul(const pointObj a, double b) {
  pointObj retv= {a.x*b,a.y*b
#ifdef USE_POINT_Z_M
    ,a.z*b,a.m*b
#endif
  };
  return retv;
}

/* vector ??? */
static double point_abs2(const pointObj a) {
#ifdef USE_POINT_Z_M
  return a.x*a.x+a.y*a.y+a.z*a.z+a.m*a.m;
#else
  return a.x*a.x+a.y*a.y;
#endif
}

/* vector normal */
static pointObj point_norm(const pointObj a) {
  double lenmul;
  pointObj retv;

#ifdef USE_POINT_Z_M
  if (a.x==0 && a.y==0 && a.z==0 && a.m==0) 
#else
  if (a.x==0 && a.y==0) 
#endif
    return a;

  lenmul=1.0/sqrt(point_abs2(a));  /* this seems to be the costly operation */

  retv.x = a.x*lenmul;
  retv.y = a.y*lenmul;
#ifdef USE_POINT_Z_M
  retv.z = a.z*lenmul;
  retv.m = a.m*lenmul;
#endif

  return retv;
}

/* rotate a vector 90 degrees */
static pointObj point_rotz90(const pointObj a) {
  double nx=-1.0*a.y, ny=a.x;
  pointObj retv=a;
  retv.x=nx; retv.y=ny;
  return retv;
}

/* vector cross product (warning: z and m dimensions are ignored!) */
static double point_cross(const pointObj a, const pointObj b) {
  return a.x*b.y-a.y*b.x;
}

/* 
** For offset corner point calculation 1/sin() is used
** to avoid 1/0 division (and long spikes) we define a 
** limit for sin().
*/
#define CURVE_SIN_LIMIT 0.3

shapeObj *msOffsetPolyline(shapeObj *p, double offsetx, double offsety) {
  int i, j, first,idx;

  shapeObj *ret = (shapeObj*)malloc(sizeof(shapeObj));
  msInitShape(ret);
  ret->numlines = p->numlines;
  ret->line=(lineObj*)malloc(sizeof(lineObj)*ret->numlines);
  for(i=0;i<ret->numlines;i++) {
    ret->line[i].numpoints=p->line[i].numpoints;
    ret->line[i].point=(pointObj*)malloc(sizeof(pointObj)*ret->line[i].numpoints);
  }

  if(offsety == -99) { /* complex calculations */
    for (i = 0; i < p->numlines; i++) {
      pointObj old_pt, old_diffdir, old_offdir;
      idx=0;
      first = 1;

      /* saved metrics of the last processed point */
      if (p->line[i].numpoints>0)
        old_pt=p->line[i].point[0];
      for(j=1; j<p->line[i].numpoints; j++) {
        const pointObj pt = p->line[i].point[j]; /* place of the point */
        const pointObj diffdir = point_norm(point_diff(pt,old_pt)); /* direction of the line */
        const pointObj offdir = point_rotz90(diffdir); /* direction where the distance between the line and the offset is measured */
        pointObj offpt; /* this will be the corner point of the offset line */

        /* offset line points computation */
        if(first == 1) { /* first point */
          first = 0;
          offpt = point_sum(old_pt,point_mul(offdir,offsetx));
        } else { /* middle points */
          /* curve is the angle of the last and the current line's direction (supplementary angle of the shape's inner angle) */
          double sin_curve = point_cross(diffdir,old_diffdir);
          double cos_curve = point_cross(old_offdir,diffdir);
          if ((-1.0)*CURVE_SIN_LIMIT < sin_curve && sin_curve < CURVE_SIN_LIMIT) {
            /* do not calculate 1/sin, instead use a corner point approximation: average of the last and current offset direction and length */

            /* 
	    ** TODO: fair for obtuse inner angles, however, positive and negative
            ** acute inner angles would need special handling - similar to LINECAP
            ** to avoid drawing of long spikes
            */
            offpt = point_sum(old_pt, point_mul(point_sum(offdir, old_offdir),0.5*offsetx));
          } else {
            double base_shift = -1.0*(1.0+cos_curve)/sin_curve;
            offpt = point_sum(old_pt, point_mul(point_sum(point_mul(diffdir,base_shift),offdir), offsetx));
	  }
        }
        ret->line[i].point[idx]=offpt;
        idx++;
        old_pt=pt; old_diffdir=diffdir; old_offdir=offdir;
      }
	            
      /* last point */
      if(first == 0) {
        pointObj offpt=point_sum(old_pt,point_mul(old_offdir,offsetx));
        ret->line[i].point[idx]=offpt;
        idx++;
      }
            
      if(idx != p->line[i].numpoints) {
        /* printf("shouldn't happen :(\n"); */
        ret->line[i].numpoints=idx;
        ret->line=realloc(ret->line,ret->line[i].numpoints*sizeof(pointObj));
      }
    }
  } else { /* normal offset (eg. drop shadow) */
    for (i = 0; i < p->numlines; i++) {
      for(j=0; j<p->line[i].numpoints; j++) {
        ret->line[i].point[j].x=p->line[i].point[j].x+offsetx;
        ret->line[i].point[j].y=p->line[i].point[j].y+offsety;
      }
    }
  }

  return ret;
}

/*
-------------------------------------------------------------------------------
 msSetup()

 Contributed by Jerry Pisk in bug 1203.  Heads off potential race condition
 in initializing GD font cache with multiple threads.  Should be called from
 mapscript module initialization code.
-------------------------------------------------------------------------------
*/

int msSetup()
{
#ifdef USE_THREAD
   msThreadInit();
#endif

  /* Use MS_ERRORFILE and MS_DEBUGLEVEL env vars if set */
  if (msDebugInitFromEnv() != MS_SUCCESS)
    return MS_FAILURE;

#ifdef USE_GD_FT
  if (gdFontCacheSetup() != 0) {
    return MS_FAILURE;
   }
#endif

#ifdef USE_GEOS
  msGEOSSetup();
#endif

  return MS_SUCCESS;
}
  
/* This is intended to be a function to cleanup anything that "hangs around"
   when all maps are destroyed, like Registered GDAL drivers, and so forth. */
void msCleanup()
{
  msForceTmpFileBase( NULL );
  msConnPoolFinalCleanup();
  msyylex_destroy();

#ifdef USE_OGR
  msOGRCleanup();
#endif    
#ifdef USE_GDAL
  msGDALCleanup();
#endif    
#ifdef USE_PROJ
  pj_deallocate_grids();
  msSetPROJ_LIB( NULL );
#endif
#if defined(USE_WMS_LYR) || defined(USE_WFS_LYR)
  msHTTPCleanup();
#endif

#ifdef USE_GD_FT
  gdFontCacheShutdown(); 
#endif

#ifdef USE_GEOS
  msGEOSCleanup();
#endif
  msIO_Cleanup();

  msResetErrorList();

  /* Close/cleanup log/debug output. Keep this at the very end. */
  msDebugCleanup();
}

/************************************************************************/
/*                            msAlphaBlend()                            */
/*                                                                      */
/*      MapServer variation on gdAlphaBlend() that, I think, does a     */
/*      better job of merging a non-opaque color into an opaque         */
/*      color.  In particular from gd 2.0.12 on the GD                  */
/*      gdAlphaBlend() will give a transparent "dst" color influence    */
/*      in the result if the overlay is non-opaque.                     */
/*                                                                      */
/*      Note that "src" is layered over "dst".                          */
/************************************************************************/

int msAlphaBlend (int dst, int src)
{
    int src_alpha = gdTrueColorGetAlpha(src);
    int dst_alpha, alpha, red, green, blue;
    int src_weight, dst_weight, tot_weight;

/* -------------------------------------------------------------------- */
/*      Simple cases we want to handle fast.                            */
/* -------------------------------------------------------------------- */
    if( src_alpha == gdAlphaOpaque )
        return src;

    dst_alpha = gdTrueColorGetAlpha(dst);
    if( src_alpha == gdAlphaTransparent )
        return dst;
    if( dst_alpha == gdAlphaTransparent )
        return src;

/* -------------------------------------------------------------------- */
/*      What will the source and destination alphas be?  Note that      */
/*      the destination weighting is substantially reduced as the       */
/*      overlay becomes quite opaque.                                   */
/* -------------------------------------------------------------------- */
    src_weight = gdAlphaTransparent - src_alpha;
    dst_weight = (gdAlphaTransparent - dst_alpha) * src_alpha / gdAlphaMax;
    tot_weight = src_weight + dst_weight;
    
/* -------------------------------------------------------------------- */
/*      What red, green and blue result values will we use?             */
/* -------------------------------------------------------------------- */
    alpha = src_alpha * dst_alpha / gdAlphaMax;

    red = (gdTrueColorGetRed(src) * src_weight
           + gdTrueColorGetRed(dst) * dst_weight) / tot_weight;
    green = (gdTrueColorGetGreen(src) * src_weight
           + gdTrueColorGetGreen(dst) * dst_weight) / tot_weight;
    blue = (gdTrueColorGetBlue(src) * src_weight
           + gdTrueColorGetBlue(dst) * dst_weight) / tot_weight;

/* -------------------------------------------------------------------- */
/*      Return merged result.                                           */
/* -------------------------------------------------------------------- */
    return ((alpha << 24) + (red << 16) + (green << 8) + blue);
}

/*
 RFC 24: check if the parent pointer is NULL and raise an error otherwise
*/
int msCheckParentPointer(void* p, char *objname) {
    char* fmt="The %s parent object is null";
    char* msg=NULL;
    if (p == NULL) {
        if(objname != NULL) {
            msg=malloc( sizeof(char) * ( ( strlen(fmt)+strlen(objname) ) ) );
            if(msg == NULL) {
                msg="A required parent object is null";
            } else {
                sprintf(msg, "The %s parent object is null", objname);
            }
        } else {
            msg="A required parent object is null";
        }
        msSetError(MS_NULLPARENTERR, msg, "");
        return MS_FAILURE;
    }
    return MS_SUCCESS;
}

inline void msBufferInit(bufferObj *buffer) {
    buffer->data=NULL;
    buffer->size=0;
    buffer->available=0;
    buffer->_next_allocation_size = MS_DEFAULT_BUFFER_ALLOC;
}

inline void msBufferResize(bufferObj *buffer, size_t target_size){
    while(buffer->available <= target_size) {
        buffer->data = realloc(buffer->data,buffer->available+buffer->_next_allocation_size);
        buffer->available += buffer->_next_allocation_size;
        buffer->_next_allocation_size *= 2;
    }
}

inline void msBufferAppend(bufferObj *buffer, void *data, size_t length) {
    if(buffer->available < buffer->size+length) {
        msBufferResize(buffer,buffer->size+length);
    }
    memcpy(&(buffer->data[buffer->size]),data,length);
    buffer->size += length;
}

inline void msBufferFree(bufferObj *buffer) {
    if(buffer->available>0)
        free(buffer->data);
}


void msFreeRasterBuffer(rasterBufferObj *b) {
    msFree(b->pixelbuffer);
}

/*
** Issue #3043: Layer extent comparison short circuit.
**
** msExtentsOverlap()
**
** Returns MS_TRUE if map extent and layer extent overlap, 
** MS_FALSE if they are disjoint, and MS_UNKNOWN if there is 
** not enough info to calculate a deterministic answer.
**
*/
int msExtentsOverlap(mapObj *map, layerObj *layer)
{
    rectObj map_extent;
    rectObj layer_extent;
    
    /* No extent info? Nothing we can do, return MS_UNKNOWN. */
    if( (map->extent.minx == -1) && (map->extent.miny == -1) && (map->extent.maxx == -1 ) && (map->extent.maxy == -1) ) return MS_UNKNOWN;
    if( (layer->extent.minx == -1) && (layer->extent.miny == -1) && (layer->extent.maxx == -1 ) && (layer->extent.maxy == -1) ) return MS_UNKNOWN;
        
#ifdef USE_PROJ

    /* No map projection? Let someone else sort this out. */
    if( ! (map->projection.numargs > 0) ) 
        return MS_UNKNOWN;

    /* No layer projection? Perform naive comparison, because they are 
    ** in the same projection. */
    if( ! (layer->projection.numargs > 0) ) 
        return msRectOverlap( &(map->extent), &(layer->extent) );
    
    /* We need to transform our rectangles for comparison, 
    ** so we will work with copies and leave the originals intact. */
    MS_COPYRECT(&map_extent, &(map->extent) );
    MS_COPYRECT(&layer_extent, &(layer->extent) );

    /* Transform map extents into geographics for comparison. */
    if( msProjectRect(&(map->projection), &(map->latlon), &map_extent) )
        return MS_UNKNOWN;
        
    /* Transform layer extents into geographics for comparison. */
    if( msProjectRect(&(layer->projection), &(map->latlon), &layer_extent) )
        return MS_UNKNOWN;

    /* Simple case? Return simple answer. */
    if ( map_extent.minx < map_extent.maxx && layer_extent.minx < layer_extent.maxx )
        return msRectOverlap( &(map_extent), &(layer_extent) );
        
    /* Uh oh, one of the rects crosses the dateline!
    ** Let someone else handle it. */
    return MS_UNKNOWN;
   
#else
    /* No proj? Naive comparison. */
    if( msRectOverlap( &(map->extent), &(layer->extent) ) ) return MS_TRUE;
    return MS_FALSE;
#endif

}

/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  Various utility functions ... a real hodgepodge.
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.190  2006/08/15 17:24:56  dan
 * Fixed problem with PHP MapScript's saveWebImage() filename collisions
 * when mapscript was loaded in php.ini with PHP as an Apache DSO (bug 1322)
 *
 * Revision 1.189  2006/07/05 05:54:53  frank
 * implement per-thread io contexts
 *
 * Revision 1.188  2006/06/27 13:35:45  sdlime
 * Initial version of GEOS C-API code including SWIG bindings for most everything useful.
 *
 * Revision 1.187  2005/08/27 04:57:25  sdlime
 * Fixed a recursion problem in context evaluation function msEvalContext(). Had to disable the [raster] option though and will try to implement some other way. (bug 1283)
 *
 * Revision 1.186  2005/07/27 15:33:11  frank
 * Make sure we have room for big counters in temp file names.
 *
 * Revision 1.185  2005/07/27 15:01:32  frank
 * Fixes in msTmpFile() to ensure counter actually increments and that
 * forced basenames work again.  Bug 1312.
 *
 * Revision 1.184  2005/07/13 19:35:08  julien
 * Bug 1381: Support for case-insensitive Expression
 *
 * Revision 1.183  2005/06/24 13:32:32  assefa
 * in function msTmpFile : reinitialize always the sting tmpId. There was
 * a problem with this string not being initialized when the function was
 * call more than once : Bug 1312.
 *
 * Revision 1.182  2005/06/14 16:03:35  dan
 * Updated copyright date to 2005
 *
 * Revision 1.181  2005/05/27 15:00:12  dan
 * New regex wrappers to solve issues with previous version (bug 1354)
 *
 * Revision 1.180  2005/04/21 15:09:28  julien
 * Bug1244: Replace USE_SHAPE_Z_M by USE_POINT_Z_M
 *
 * Revision 1.179  2005/04/14 15:17:14  julien
 * Bug 1244: Remove Z and M from point by default to gain performance.
 *
 * Revision 1.178  2005/04/12 23:56:44  sean
 * change tmpId string to be non static in msTmpFile() (bug 1312).
 *
 * Revision 1.177  2005/04/07 17:23:16  assefa
 * Remove #ifdef USE_SVG. It was added during development.
 *
 * Revision 1.176  2005/02/18 05:07:26  frank
 * fixed return from msForceTmpFileBase
 *
 * Revision 1.175  2005/02/18 03:06:47  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.174  2005/02/09 21:51:17  frank
 * added msForceTmpFileBase and -tmpbase mapserv switch
 *
 * Revision 1.173  2005/02/09 20:43:50  assefa
 * Add SVG utility function msSaveImagetoFpSVG.
 *
 * Revision 1.172  2005/02/08 17:34:46  sean
 * call msThreadInit in msSetup (bug 1223).
 *
 * Revision 1.171  2005/02/06 19:31:32  sean
 * msSetup() function calls gdFontCacheSetup().  msSetup should be called immediately on mapscript module import to avoid race condition with font cache (bug 1203).
 *
 * Revision 1.170  2005/02/03 00:06:57  assefa
 * Add SVG function prototypes and related calls.
 *
 * Revision 1.169  2005/01/28 06:16:54  sdlime
 * Applied patch to make function prototypes ANSI C compliant. Thanks to Petter Reinholdtsen. This fixes but 1181.
 *
 * Revision 1.168  2005/01/20 01:59:19  sean
 * protect against labelitemindex < 0 take 2, > 0 changed to >= 0
 *
 * Revision 1.167  2005/01/20 00:53:47  sean
 * protect against labelitemindex < 0
 *
 * Revision 1.166  2004/12/07 21:33:28  sean
 * protect against getting a shape value when values is NULL in msShapeGetAnnotation().  also implement setValue for mapscript::shapeObj so we can define labels for inline features (bug 1114)
 *
 * Revision 1.165.2.1  2004/12/07 21:10:20  sean
 * protect against getting a shape value when values is NULL in msShapeGetAnnotation().  also implement setValue for mapscript::shapeObj so we can define labels for inline features (bug 1114)
 *
 * Revision 1.165  2004/11/22 16:19:42  sdlime
 * Trivial formating change.
 *
 * Revision 1.164  2004/11/22 15:44:42  sean
 * quick fixes to work around new requirement for non-null layer names in msValidateContext.  Set ltag to '[NULL]' for a NULL name in msValidateContext, skip null-named layers in msEvalContext (bug 1081).
 *
 * Revision 1.163  2004/11/22 03:43:54  sdlime
 * Added tests to mimimize the threat of recursion problems when evaluating LAYER REQUIRES or LABELREQUIRES expressions. Note that via MapScript it is possible to circumvent that test by defining layers with problems after running prepareImage. Other things crop up in that case too (symbol scaling dies) so it should be considered bad programming practice.
 *
 * Revision 1.162  2004/10/28 02:12:48  frank
 * Don't try and use msBuildPath() if filename is NULL (stdout) in msSaveImage().
 * This prevents "noise" calls to msSetError() by msBuildPath().
 *
 * Revision 1.161  2004/10/21 04:30:56  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include <time.h>

#include "map.h"
#include "mapparser.h"
#include "mapthread.h"

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
extern char *msyytext;

extern int msyyresult; /* result of parsing, true/false */
extern int msyystate;
extern char *msyystring;


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
        if(searchContextForTag(map, ltags, tag, map->layers[i].requires, MS_TRUE) == MS_SUCCESS) return MS_SUCCESS;
      } else {
        if(searchContextForTag(map, ltags, tag, map->layers[i].labelrequires, MS_FALSE) == MS_SUCCESS) return MS_SUCCESS;      
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
    if(map->layers[i].name == NULL) {
      ltags[i] = strdup("[NULL]");
    } else {
      ltags[i] = (char *) malloc(sizeof(char)*strlen(map->layers[i].name) + 3);
      sprintf(ltags[i], "[%s]", map->layers[i].name);
    }
  }

  /* check each layer's REQUIRES and LABELREQUIRES parameters */
  for(i=0; i<map->numlayers; i++) { 
    /* printf("working on layer %s, looking for references to %s\n", map->layers[i].name, ltags[i]); */
    if(searchContextForTag(map, ltags, ltags[i], map->layers[i].requires, MS_TRUE) == MS_SUCCESS) {
      msSetError(MS_PARSEERR, "Recursion error found for REQUIRES parameter for layer %s.", "msValidateContexts", map->layers[i].name);
      status = MS_FAILURE;
      break;
    }
    if(searchContextForTag(map, ltags, ltags[i], map->layers[i].labelrequires, MS_FALSE) == MS_SUCCESS) {
      msSetError(MS_PARSEERR, "Recursion error found for LABELREQUIRES parameter for layer %s.", "msValidateContexts", map->layers[i].name);
      status = MS_FAILURE;
      break;
    }
    /* printf("done layer %s\n", map->layers[i].name); */
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
    if (map->layers[i].name == NULL) continue; /* Layer without name cannot be used in contexts */

    tmpstr2 = (char *)malloc(sizeof(char)*strlen(map->layers[i].name) + 3);
    sprintf(tmpstr2, "[%s]", map->layers[i].name);

    if(strstr(tmpstr1, tmpstr2)) {
      if(msLayerIsVisible(map, &(map->layers[i])))
	tmpstr1 = gsub(tmpstr1, tmpstr2, "1");
      else
	tmpstr1 = gsub(tmpstr1, tmpstr2, "0");
    }

    free(tmpstr2);
  }

  msAcquireLock( TLOCK_PARSER );
  msyystate = 4; msyystring = tmpstr1;
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
  char *tmpstr=NULL;
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
    if(expression->flags & MS_EXP_INSENSITIVE)
    {
      if(strcasecmp(expression->string, items[itemindex]) == 0) return(MS_TRUE); /* got a match */
    }
    else
    {
      if(strcmp(expression->string, items[itemindex]) == 0) return(MS_TRUE); /* got a match */
    }
    break;
  case(MS_EXPRESSION):
    tmpstr = strdup(expression->string);

    for(i=0; i<expression->numitems; i++)      
      tmpstr = gsub(tmpstr, expression->items[i], items[expression->indexes[i]]);

    msAcquireLock( TLOCK_PARSER );
    msyystate = 4;
    msyystring = tmpstr; /* set lexer state to EXPRESSION_STRING */
    status = msyyparse();
    expresult = msyyresult;
    msReleaseLock( TLOCK_PARSER );
    free(tmpstr);

    if (status != 0)
    {
        msSetError(MS_PARSEERR, "Failed to parse expression",
                                "msEvalExpression");
        return MS_FALSE;
    }

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
 *     if(layer->class[i].status != MS_DELETE && msEvalExpression(&(layer->class[i].expression), layer->classitemindex, shape->values, layer->numitems) == MS_TRUE)
 *       return(i);
 *   }
 * 
 *   return(-1); // no match
 * }
 */

int msShapeGetClass(layerObj *layer, shapeObj *shape, double scale)
{
  int i;

  /* INLINE features do not work with expressions, allow the classindex */
  /* value set prior to calling this function to carry through. */
  if(layer->connectiontype == MS_INLINE) {
    if(shape->classindex < 0 || shape->classindex >= layer->numclasses) return(-1);

    if(scale > 0) {  /* verify scale here */
      if((layer->class[shape->classindex].maxscale > 0) && (scale > layer->class[shape->classindex].maxscale))
        return(-1); /* can skip this feature */
      if((layer->class[shape->classindex].minscale > 0) && (scale <= layer->class[shape->classindex].minscale))
        return(-1); /* can skip this feature */
    }

    return(shape->classindex);
  }

  for(i=0; i<layer->numclasses; i++) {
    
    if(scale > 0) {  /* verify scale here  */
      if((layer->class[i].maxscale > 0) && (scale > layer->class[i].maxscale))
        continue; /* can skip this one, next class */
      if((layer->class[i].minscale > 0) && (scale <= layer->class[i].minscale))
        continue; /* can skip this one, next class */
    }

    if(layer->class[i].status != MS_DELETE && msEvalExpression(&(layer->class[i].expression), layer->classitemindex, shape->values, layer->numitems) == MS_TRUE)
      return(i);
  }

  return(-1); /* no match */
}

char *msShapeGetAnnotation(layerObj *layer, shapeObj *shape)
{
  int i;
  char *tmpstr=NULL;

  if(layer->class[shape->classindex].text.string) { /* test for global label first */
    tmpstr = strdup(layer->class[shape->classindex].text.string);
    switch(layer->class[shape->classindex].text.type) {
    case(MS_STRING):
      break;
    case(MS_EXPRESSION):
      tmpstr = strdup(layer->class[shape->classindex].text.string);

      for(i=0; i<layer->class[shape->classindex].text.numitems; i++)
	tmpstr = gsub(tmpstr, layer->class[shape->classindex].text.items[i], shape->values[layer->class[shape->classindex].text.indexes[i]]);
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

  ox = MS_MAX((width - (rect->maxx - rect->minx)/cellsize)/2,0); /* these were width-1 and height-1 */
  oy = MS_MAX((height - (rect->maxy - rect->miny)/cellsize)/2,0);

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
    if (img)
    {
        if( MS_DRIVER_GD(img->format) )
        {
            if(map != NULL && filename != NULL )
                nReturnVal = msSaveImageGD(img->img.gd, 
                                           msBuildPath(szPath, map->mappath, 
                                                       filename), 
                                           img->format );
            else
                nReturnVal = msSaveImageGD(img->img.gd, filename, img->format);
        }
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

    return nReturnVal;
}

/**
 * Generic function to free the imageObj
 */
void msFreeImage(imageObj *image)
{
    if (image)
    {
        if( MS_RENDERER_GD(image->format) ) {
            if( image->img.gd != NULL )
                msFreeImageGD(image->img.gd);
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
        if(!map->layers[i].group) /* skip it */
            continue;
        if(strcmp(groupname, map->layers[i].group) == 0)
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
            lp = &(map->layers[map->layerorder[i]]);

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
                                imagepath, imageurl);
        if( image != NULL && map) msImageInitGD( image, &map->imagecolor );
    }

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
            
        if (imagepath)
            image->imagepath = strdup(imagepath);
        if (imageurl)
            image->imageurl = strdup(imageurl);
            
        return image;
    }
    else if( MS_RENDERER_IMAGEMAP(format) )
    {
        image = msImageCreateIM(width, height, format,
                                imagepath, imageurl);
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

    msTransformShapeToPixel(shape, extent, cellsize);
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
extern void lexer_cleanup(void);

void msCleanup()
{
  msForceTmpFileBase( NULL );
  msConnPoolFinalCleanup();
  lexer_cleanup();

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
#ifdef USE_GEOS
  msGEOSCleanup();
#endif
  msIO_Cleanup();

  msResetErrorList();
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

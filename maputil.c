#include "map.h"
#include "mapparser.h"

#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

extern int msyyparse();
extern int msyylex();
extern char *msyytext;

extern int msyyresult; // result of parsing, true/false
extern int msyystate;
extern char *msyystring;


/*
 * Used to get red, green, blue integers separately based upon the color index
 */
int getRgbColor(mapObj *map,int i,int *r,int *g,int *b) {
// check index range
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

int msEvalContext(mapObj *map, char *context)
{
  int i, status;
  char *tmpstr1=NULL, *tmpstr2=NULL;

  if(!context) return(MS_TRUE); // no context requirements

  tmpstr1 = strdup(context);

  for(i=0; i<map->numlayers; i++) { // step through all the layers
    if(strstr(tmpstr1, map->layers[i].name)) {
      tmpstr2 = (char *)malloc(sizeof(char)*strlen(map->layers[i].name) + 3);
      sprintf(tmpstr2, "[%s]", map->layers[i].name);

      if(map->layers[i].status == MS_OFF)
	tmpstr1 = gsub(tmpstr1, tmpstr2, "0");
      else
	tmpstr1 = gsub(tmpstr1, tmpstr2, "1");

      free(tmpstr2);
    }
  }

  msyystate = 4; msyystring = tmpstr1;
  status = msyyparse();
  free(tmpstr1);

  if(status != 0) return(MS_FALSE); // error in parse
  return(msyyresult);
}

int msEvalExpression(expressionObj *expression, int itemindex, char **items, int numitems)
{
  int i;
  char *tmpstr=NULL;
  int status;

  if(!expression->string) return(MS_TRUE); // empty expressions are ALWAYS true

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
    if(strcmp(expression->string, items[itemindex]) == 0) return(MS_TRUE); // got a match
    break;
  case(MS_EXPRESSION):
    tmpstr = strdup(expression->string);

    for(i=0; i<expression->numitems; i++)
      tmpstr = gsub(tmpstr, expression->items[i], items[expression->indexes[i]]);

    msyystate = 4; msyystring = tmpstr; // set lexer state to EXPRESSION_STRING
    status = msyyparse();
    free(tmpstr);

    if(status != 0) return(MS_FALSE); // error in parse

    return(msyyresult);
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
      if(regcomp(&(expression->regex), expression->string, REG_EXTENDED|REG_NOSUB) != 0) { // compile the expression
        msSetError(MS_REGEXERR, "Invalid regular expression.", "msEvalExpression()");
        return(MS_FALSE);
      }
      expression->compiled = MS_TRUE;
    }

    if(regexec(&(expression->regex), items[itemindex], 0, NULL, 0) == 0) return(MS_TRUE); // got a match
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

  for(i=0; i<layer->numclasses; i++) {
    
    if(scale > 0) {  // verify scale here 
      if((layer->class[i].maxscale > 0) && (scale > layer->class[i].maxscale))
        continue; // can skip this one, next class
      if((layer->class[i].minscale > 0) && (scale <= layer->class[i].minscale))
        continue; // can skip this one, next class
    }

    if(layer->class[i].status != MS_DELETE && msEvalExpression(&(layer->class[i].expression), layer->classitemindex, shape->values, layer->numitems) == MS_TRUE)
      return(i);
  }

  return(-1); // no match
}

char *msShapeGetAnnotation(layerObj *layer, shapeObj *shape)
{
  int i;
  char *tmpstr=NULL;

  if(layer->class[shape->classindex].text.string) { // test for global label first
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

  ox = MS_NINT(MS_MAX((width - (rect->maxx - rect->minx)/cellsize)/2,0)); // these were width-1 and height-1
  oy = MS_NINT(MS_MAX((height - (rect->maxy - rect->miny)/cellsize)/2,0));

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

  // check left edge, and if necessary the right edge of bounds
  if(rect->maxx <= bounds->minx) {
    offset = overlay*(rect->maxx - rect->minx);
    rect->minx += offset; // shift right
    rect->maxx += offset;
  } else if(rect->minx >= bounds->maxx) {
    offset = overlay*(rect->maxx - rect->minx);
    rect->minx -= offset; // shift left
    rect->maxx -= offset;
  }

  // check top edge, and if necessary the bottom edge of bounds
  if(rect->maxy <= bounds->miny) {
    offset = overlay*(rect->maxy - rect->miny);
    rect->miny -= offset; // shift down
    rect->maxy -= offset;
  } else if(rect->miny >= bounds->maxy) {
    offset = overlay*(rect->maxy - rect->miny);
    rect->miny += offset; // shift up
    rect->maxy += offset;
  }

  return(MS_SUCCESS);
}

/*
** Generic function to save an image to a file.
**
** Note that map is NULL for images other than the main map, and it is 
** intended to be used only to extract the georeferenced extents and coordinate
** system of the map for writing out with the image when appropriate 
** (primarily this means via msSaveImageGDAL() to something like GeoTIFF). 
**
** The filename is NULL when the image is supposed to be written to stdout. 
*/

int msSaveImage(mapObj *map, imageObj *img, char *filename)
{
    int nReturnVal = -1;
    if (img)
    {
        if( MS_DRIVER_GD(img->format) )
            nReturnVal =  msSaveImageGD(img->img.gd, filename, img->format );
#ifdef USE_GDAL
        else if( MS_DRIVER_GDAL(img->format) )
            nReturnVal = msSaveImageGDAL(map, img, filename);
#endif
#ifdef USE_MING_FLASH
        else if( MS_DRIVER_SWF(img->format) )
            nReturnVal = msSaveImageSWF(img, filename);
#endif
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
        if( MS_RENDERER_GD(image->format) )
            msFreeImageGD(image->img.gd);

        else if( MS_RENDERER_RAWDATA(image->format) )
            msFree(image->img.raw_16bit);
#ifdef USE_MING_FLASH
        else if( MS_RENDERER_SWF(image->format) )
            msFreeImageSWF(image);
#endif
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


/*
** Move the layer's order for drawing purpose. Moving it up here
** will have the effect of drawing the layer earlier. 
*/
int msMoveLayerUp(mapObj *map, int nLayerIndex)
{
    int iCurrentIndex = -1;
    int i = 0;
    if (map && nLayerIndex < map->numlayers && nLayerIndex >=0)
    {
        for (i=0; i<map->numlayers; i++)
        {
            if ( map->layerorder[i] == nLayerIndex)
            {
                iCurrentIndex = i;
                break;
            }
        }
        if (iCurrentIndex >=0) 
        {
            // we do not need to promote if it is the first one.
            if (iCurrentIndex == 0)
                return 0;

            map->layerorder[iCurrentIndex] =  
                map->layerorder[iCurrentIndex-1];
            map->layerorder[iCurrentIndex-1] = nLayerIndex;

            return 0;
        }
    }
    return -1;
}

/*
** Move the layer's order for drawing purpose. Moving it down here
** will have the effect of drawing the layer later. 
*/
int msMoveLayerDown(mapObj *map, int nLayerIndex)
{
    int iCurrentIndex = -1;
    int i = 0;
    if (map && nLayerIndex < map->numlayers && nLayerIndex >=0)
    {
        for (i=0; i<map->numlayers; i++)
        {
            if ( map->layerorder[i] == nLayerIndex)
            {
                iCurrentIndex = i;
                break;
            }
        }
        if (iCurrentIndex >=0) 
        {
            // we do not need to demote if it is the last one.
            if (iCurrentIndex == map->numlayers-1)
                return 0;

            map->layerorder[iCurrentIndex] =  
                map->layerorder[iCurrentIndex+1];
            map->layerorder[iCurrentIndex+1] = nLayerIndex;

            return 0;
        }
    }
    return -1;
}


/*
** Set the array used for the drawing order. The array passed must contain
** all the layer's index ordered by the drawing priority.
** Ex : for 3 layers in the map file, if 
**                          panIndexes[0] = 2
**                          panIndexes[1] = 0
**                          panIndexes[2] = 1
**                          will set the darwing order to layer 2, layer 0,
**                          and then layer 1.
**
** Note : It is assumed that the index panIndexes has the same number of
**        of elements as the number of layers in the map.
** Return TRUE on success else FALSE.
*/
int msSetLayersdrawingOrder(mapObj *self, int *panIndexes)
{
    int nElements = 0;
    int i, j = 0;
    int bFound = 0;

    if (self && panIndexes)
    {
        nElements = self->numlayers;
        for (i=0; i<nElements; i++)
        {
            bFound = 0;
            for (j=0; j<nElements; j++)
            {
                if (panIndexes[j] == i)
                {
                    bFound = 1;
                    break;
                }
            }
            if (!bFound)
                return 0;
        }
/* -------------------------------------------------------------------- */
/*    At this point the array is valid so update the layers order array.*/
/* -------------------------------------------------------------------- */
        for (i=0; i<nElements; i++)
        {
            self->layerorder[i] = panIndexes[i];
        }
        return 1;
    }
    return 0;
}


/*
** Return the projection string. 
*/
char *msGetProjectionString(projectionObj *proj)
{
    char        *pszPojString = NULL;
    char        *pszTmp = NULL;
    int         i = 0;

    if (proj)
    {
        for (i=0; i<proj->numargs; i++)
        {
            if (!proj->args[i] || strlen(proj->args[i]) <=0)
                continue;

            pszTmp = proj->args[i];
/* -------------------------------------------------------------------- */
/*      if narguments = 1 do not add a +.                               */
/* -------------------------------------------------------------------- */
            if (proj->numargs == 1)
            {
                pszPojString = 
                    malloc(sizeof(char) * strlen(pszTmp)+1);
                pszPojString[0] = '\0';
                strcat(pszPojString, pszTmp);
            }
            else
            {
/* -------------------------------------------------------------------- */
/*      Copy chaque argument and add a + between them.                  */
/* -------------------------------------------------------------------- */
                if (pszPojString == NULL)
                {
                    pszPojString = 
                        malloc(sizeof(char) * strlen(pszTmp)+2);
                    pszPojString[0] = '\0';
                    strcat(pszPojString, "+");
                    strcat(pszPojString, pszTmp);
                }
                else
                {
                    pszPojString =  
                        realloc(pszPojString,
                                sizeof(char) * (strlen(pszTmp)+ 
                                                strlen(pszPojString) + 2));
                    strcat(pszPojString, "+");
                    strcat(pszPojString, pszTmp);
                }
            }
        }
    }
    return pszPojString;
}

/* ==================================================================== */
/*      Measured shape utility functions.                               */
/* ==================================================================== */


/************************************************************************/
/*        pointObj *getPointUsingMeasure(shapeObj *shape, double m)     */
/*                                                                      */
/*      Using a measured value get the XY location it corresonds        */
/*      to.                                                             */
/*                                                                      */
/************************************************************************/
pointObj *getPointUsingMeasure(shapeObj *shape, double m)
{
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
                        if (j > 0) //not the first point of the line
                        {
                            dfFirstPointX = line.point[j-1].x;
                            dfFirstPointY = line.point[j-1].y;
                            dfFirstPointM = line.point[j-1].m;
                        }
                        else // get last point of previous line
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
        result->m = 0;
    }

    return result;
}


/************************************************************************/
/*         pointObj *getMeasureUsingPoint(shapeObj *shape, pointObj     */
/*      *point)                                                         */
/*                                                                      */
/*      Calculate the intersection point betwwen the point and the      */
/*      shape and return the Measured value at the intersection.        */
/************************************************************************/
pointObj *getMeasureUsingPoint(shapeObj *shape, pointObj *point)
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
                dfDist = msDistanceFromPointToLine(point, 
                                                   &line.point[j], 
                                                   &line.point[j+1]);
                if (dfDist < dfMinDist)
                {
                    oFirst.x = line.point[j].x;
                    oFirst.y = line.point[j].y;
                    oFirst.m = line.point[j].m;
                    
                    oSecond.x =  line.point[j+1].x;
                    oSecond.y =  line.point[j+1].y;
                    oSecond.m =  line.point[j+1].m;

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

            poIntersectionPt->m = oFirst.m + (oSecond.m - oFirst.m)*dfFactor;

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
            bFound = 0;
            if (map->layers[map->layerorder[i]].group)
            {
                for (j=0; j<*numTok; j++)
                {
                    if (papszGroups[j] &&
                        strcmp(map->layers[map->layerorder[i]].group, papszGroups[j]) == 0)
                    {
                        bFound = 1;
                        break;
                    }
                }
                if (!bFound)
                {
                    /* New group... add to the list of groups found */
                    papszGroups[(*numTok)] = strdup(map->layers[map->layerorder[i]].group);
                    (*numTok)++;
                }
            }
        }

    }
   
    return papszGroups;
}

/**********************************************************************
 *                          msTmpFile()
 *
 * Generate a Unique temporary filename using PID + timestamp + extension
 * char* returned must be freed by caller
 **********************************************************************/
char *msTmpFile(const char *path, const char *ext)
{
    char *tmpFname;
    static char tmpId[128]; /* big enough for time + pid */
    static int tmpCount = -1;

    if (tmpCount == -1)
    {
        /* We'll use tmpId and tmpCount to generate unique filenames */
        sprintf(tmpId, "%ld%d",(long)time(NULL),(int)getpid());
        tmpCount = 0;
    }

    if (path == NULL) path="";
    if (ext == NULL)  ext = "";

    tmpFname = (char*)malloc(strlen(path) + strlen(tmpId) + 4  + strlen(ext) + 1);

    sprintf(tmpFname, "%s%s%d.%s%c", path, tmpId, tmpCount++, ext, '\0');

    return tmpFname;
}

/**
 *  Generic function to Initalize an image object.
 */
imageObj *msImageCreate(int width, int height, outputFormatObj *format, 
                        char *imagepath, char *imageurl)
{
    imageObj *image = NULL;

    if( MS_RENDERER_GD(format) )
        image = msImageCreateGD(width, height, format,
                                imagepath, imageurl);

    else if( MS_RENDERER_RAWDATA(format) )
    {
        if( format->imagemode != MS_IMAGEMODE_INT16
            && format->imagemode != MS_IMAGEMODE_FLOAT32 )
        {
            msSetError(MS_IMGERR, 
                    "Attempt to use illegal imagemode with rawdata renderer.",
                       "msImageCreate()" );
            return NULL;
        }

        image = (imageObj *)calloc(1,sizeof(imageObj));

        if( format->imagemode == MS_IMAGEMODE_INT16 )
            image->img.raw_16bit = (short *) 
                calloc(sizeof(short),width*height);
        else if( format->imagemode == MS_IMAGEMODE_FLOAT32 )
            image->img.raw_float = (float *) 
                calloc(sizeof(float),width*height);

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
        msTransformShapeSWF(shape, extent, cellsize);
        return;
    }
#endif

    msTransformShapeToPixel(shape, extent, cellsize);
}
 

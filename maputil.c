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

static double inchesPerUnit[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};

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

int msShapeGetClass(layerObj *layer, shapeObj *shape)
{
  int i;

  for(i=0; i<layer->numclasses; i++) {
    if(layer->class[i].status != MS_DELETE &&
       msEvalExpression(&(layer->class[i].expression), layer->classitemindex, shape->values, layer->numitems) == MS_TRUE)
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

gdImagePtr msDrawMap(mapObj *map)
{
  int i;
  gdImagePtr img=NULL;
  layerObj *lp=NULL;
  int status;

  if(map->width == -1 || map->height == -1) {
    msSetError(MS_MISCERR, "Image dimensions not specified.", "msDrawMap()");
    return(NULL);
  }

  img = gdImageCreate(map->width, map->height);
  if(!img) {
    msSetError(MS_GDERR, "Unable to initialize image.", "msDrawMap()");
    return(NULL);
  }

  if(msLoadPalette(img, &(map->palette), map->imagecolor) == -1)
    return(NULL);

  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scale);
  if(status != MS_SUCCESS) return(NULL);

  for(i=0; i<map->numlayers; i++) {

    if (map->layerorder[i] != -1) {
       lp = &(map->layers[ map->layerorder[i]]);
       //lp = &(map->layers[i]);

       if(lp->postlabelcache) // wait to draw
         continue;

       status = msDrawLayer(map, lp, img);
       if(status != MS_SUCCESS) return(NULL);
    }
  }

  if(map->scalebar.status == MS_EMBED && !map->scalebar.postlabelcache)
    msEmbedScalebar(map, img);

  if(map->legend.status == MS_EMBED && !map->legend.postlabelcache)
    msEmbedLegend(map, img);

  if(msDrawLabelCache(img, map) == -1)
    return(NULL);

  for(i=0; i<map->numlayers; i++) { // for each layer, check for postlabelcache layers

    lp = &(map->layers[i]);

    if(!lp->postlabelcache)
      continue;

    status = msDrawLayer(map, lp, img);
    if(status != MS_SUCCESS) return(NULL);
  }

  if(map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache)
    msEmbedScalebar(map, img);

  if(map->legend.status == MS_EMBED && map->legend.postlabelcache)
    msEmbedLegend(map, img);

  return(img);
}

gdImagePtr msDrawQueryMap(mapObj *map)
{
  int i, status;
  gdImagePtr img=NULL;
  layerObj *lp=NULL;

  if(map->querymap.width != -1) map->width = map->querymap.width;
  if(map->querymap.height != -1) map->height = map->querymap.height;

  if(map->querymap.style == MS_NORMAL) return(msDrawMap(map)); // no need to do anything fancy

  if(map->width == -1 || map->height == -1) {
    msSetError(MS_MISCERR, "Image dimensions not specified.", "msDrawQueryMap()");
    return(NULL);
  }

  img = gdImageCreate( map->width, map->height);
  if(!img) {
    msSetError(MS_GDERR, "Unable to initialize image.", "msDrawQueryMap()");
    return(NULL);
  }

  if(msLoadPalette(img, &(map->palette), map->imagecolor) == -1)
    return(NULL);

  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scale);
  if(status != MS_SUCCESS) return(NULL);

  for(i=0; i<map->numlayers; i++) {
    lp = &(map->layers[i]);

    if(lp->postlabelcache) // wait to draw
      continue;

    status = msDrawQueryLayer(map, lp, img);
    if(status != MS_SUCCESS) return(NULL);
  }

  if(map->scalebar.status == MS_EMBED && !map->scalebar.postlabelcache)
    msEmbedScalebar(map, img);

  if(map->legend.status == MS_EMBED && !map->legend.postlabelcache)
    msEmbedLegend(map, img);

  if(msDrawLabelCache(img, map) == -1)
    return(NULL);

  for(i=0; i<map->numlayers; i++) { // for each layer, check for postlabelcache layers
    lp = &(map->layers[i]);

    if(!lp->postlabelcache)
      continue;

    status = msDrawQueryLayer(map, lp, img);
    if(status != MS_SUCCESS) return(NULL);
  }

  if(map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache)
    msEmbedScalebar(map, img);

  if(map->legend.status == MS_EMBED && map->legend.postlabelcache)
    msEmbedLegend(map, img);

  return(img);
}

/*
** Function to render an individual point, used as a helper function for mapscript only. Since a point
** can't carry attributes you can't do attribute based font size or angle.
*/
int msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, gdImagePtr img, int classindex, char *labeltext)
{
  int c;
  double scalefactor=1.0;

  c = classindex;

  if(layer->symbolscale > 0 && map->scale > 0) scalefactor = layer->symbolscale/map->scale;

  if(layer->sizeunits != MS_PIXELS) {
    layer->class[c].sizescaled = layer->class[c].size * (inchesPerUnit[layer->sizeunits]/inchesPerUnit[map->units]) / map->cellsize;
    layer->class[c].overlaysizescaled = layer->class[c].overlaysize * (inchesPerUnit[layer->sizeunits]/inchesPerUnit[map->units]) / map->cellsize;
  } else {
    layer->class[c].sizescaled = MS_NINT(layer->class[c].size * scalefactor);
    layer->class[c].sizescaled = MS_MAX(layer->class[c].sizescaled, layer->class[c].minsize);
    layer->class[c].sizescaled = MS_MIN(layer->class[c].sizescaled, layer->class[c].maxsize);
    layer->class[c].overlaysizescaled = layer->class[c].sizescaled - (layer->class[c].size - layer->class[c].overlaysize); // layer->class[c].overlaysizescaled = MS_NINT(layer->class[c].overlaysize * scalefactor);
    layer->class[c].overlaysizescaled = MS_MAX(layer->class[c].overlaysizescaled, layer->class[c].overlayminsize);
    layer->class[c].overlaysizescaled = MS_MIN(layer->class[c].overlaysizescaled, layer->class[c].overlaymaxsize);
  }

  if(layer->class[c].sizescaled == 0) return(MS_SUCCESS);

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  if(layer->class[c].label.type == MS_TRUETYPE) {
    layer->class[c].label.sizescaled = MS_NINT(layer->class[c].label.size * scalefactor);
    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
  }
#endif

#ifdef USE_PROJ
    if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
      msProjectPoint(&layer->projection, &map->projection, point);
#endif

  switch(layer->type) {
  case MS_LAYER_ANNOTATION:
    if(layer->transform) {
      if(!msPointInRect(point, &map->extent)) return(0);
      point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
      point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
    }

    if(labeltext) {
      if(layer->labelcache)
	msAddLabel(map, layer->index, c, -1, -1, *point, labeltext, -1);
      else {
	if(layer->class[c].color == -1) {
	  msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	  if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	}
	msDrawLabel(img, *point, labeltext, &layer->class[c].label, &map->fontset);
      }
    }
    break;

  case MS_LAYER_POINT:
    if(layer->transform) {
      if(!msPointInRect(point, &map->extent)) return(0);
      point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
      point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
    }

    msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);

    if(labeltext) {
      if(layer->labelcache)
	msAddLabel(map, layer->index, c, -1, -1, *point, labeltext, -1);
      else
	msDrawLabel(img, *point, labeltext, &layer->class[c].label, &map->fontset);
    }
    break;
  default:
    break; /* don't do anything with layer of other types */
  }

  return(MS_SUCCESS); /* all done, no cleanup */
}

/*
** Function to render an individual shape, the overlay boolean variable enables/disables drawing of the
** overlay symbology. This is necessary when drawing entire layers as proper overlay can only be achived
** through caching.
*/
int msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, gdImagePtr img, int overlay)
{
  int i,j,c;
  rectObj cliprect;
  pointObj annopnt, *point;
  double angle, length, scalefactor=1.0;
 
  pointObj center; // circle origin
  double r; // circle radius

  cliprect.minx = map->extent.minx - 2*map->cellsize; // set clipping rectangle just a bit larger than the map extent
  cliprect.miny = map->extent.miny - 2*map->cellsize;
  cliprect.maxx = map->extent.maxx + 2*map->cellsize;
  cliprect.maxy = map->extent.maxy + 2*map->cellsize;

  c = shape->classindex;

  if(layer->symbolscale > 0 && map->scale > 0) scalefactor = layer->symbolscale/map->scale;

  if(layer->sizeunits != MS_PIXELS) {
    layer->class[c].sizescaled = layer->class[c].size * (inchesPerUnit[layer->sizeunits]/inchesPerUnit[map->units]) / map->cellsize;
    layer->class[c].overlaysizescaled = layer->class[c].overlaysize * (inchesPerUnit[layer->sizeunits]/inchesPerUnit[map->units]) / map->cellsize;
  } else {
    layer->class[c].sizescaled = MS_NINT(layer->class[c].size * scalefactor);
    layer->class[c].sizescaled = MS_MAX(layer->class[c].sizescaled, layer->class[c].minsize);
    layer->class[c].sizescaled = MS_MIN(layer->class[c].sizescaled, layer->class[c].maxsize);
    layer->class[c].overlaysizescaled = layer->class[c].sizescaled - (layer->class[c].size - layer->class[c].overlaysize); // layer->class[c].overlaysizescaled = MS_NINT(layer->class[c].overlaysize * scalefactor);
    layer->class[c].overlaysizescaled = MS_MAX(layer->class[c].overlaysizescaled, layer->class[c].overlayminsize);
    layer->class[c].overlaysizescaled = MS_MIN(layer->class[c].overlaysizescaled, layer->class[c].overlaymaxsize);
  }

  if(layer->class[c].sizescaled == 0) return(MS_SUCCESS);

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  if(layer->class[c].label.type == MS_TRUETYPE) {
    layer->class[c].label.sizescaled = MS_NINT(layer->class[c].label.size * scalefactor);
    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
  }
#endif

  switch(layer->type) {
  case MS_LAYER_CIRCLE:
    if(shape->numlines != 1) return(MS_SUCCESS); // invalid shape
    if(shape->line[0].numpoints != 2) return(MS_SUCCESS); // invalid shape

    center.x = (shape->line[0].point[0].x + shape->line[0].point[1].x)/2.0;
    center.y = (shape->line[0].point[0].y + shape->line[0].point[1].y)/2.0;
    r = MS_ABS(center.x - shape->line[0].point[0].x);

#ifdef USE_PROJ
    if(msProjectionsDiffer(&(layer->projection), &(map->projection)))
        msProjectPoint(&layer->projection, &map->projection, &center);
#endif

    if(layer->transform) {
      center.x = MS_MAP2IMAGE_X(center.x, map->extent.minx, map->cellsize);
      center.y = MS_MAP2IMAGE_Y(center.y, map->extent.maxy, map->cellsize);
      r *= (inchesPerUnit[layer->units]/inchesPerUnit[map->units])/map->cellsize;      
    }

    if(layer->class[c].color < 0)
      msCircleDrawLineSymbol(&map->symbolset, img, &center, r, layer->class[c].symbol, layer->class[c].outlinecolor, layer->class[c].backgroundcolor, layer->class[c].sizescaled);
    else
      msCircleDrawShadeSymbol(&map->symbolset, img, &center, r, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);

    if(overlay && layer->class[c].overlaysymbol >= 0) {
      if(layer->class[c].overlaycolor < 0)
	msCircleDrawLineSymbol(&map->symbolset, img, &center, r, layer->class[c].overlaysymbol, layer->class[c].overlayoutlinecolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);
      else
        msCircleDrawShadeSymbol(&map->symbolset, img, &center, r, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
    }

    break;
  case MS_LAYER_ANNOTATION:
    if(!shape->text) return(MS_SUCCESS); // nothing to draw

#ifdef USE_PROJ
    if(msProjectionsDiffer(&(layer->projection), &(map->projection)))
       msProjectShape(&layer->projection, &map->projection, shape);
#endif

    switch(shape->type) {
    case(MS_SHAPE_LINE):
      if(layer->transform) {
	msClipPolylineRect(shape, cliprect);
	if(shape->numlines == 0) return(MS_SUCCESS);
	msTransformShape(shape, map->extent, map->cellsize);
      }

      if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) == MS_SUCCESS) {

	if(layer->labelangleitemindex != -1)
	  layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}

	if(layer->class[c].label.autoangle)
	  layer->class[c].label.angle = angle;

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, length);
	else {
	  if(layer->class[c].color != -1) {
	    msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	  }
	  msDrawLabel(img, annopnt, shape->text, &(layer->class[c].label), &map->fontset);
	}
      }

      break;
    case(MS_SHAPE_POLYGON):

      if(layer->transform) {
	msClipPolygonRect(shape, cliprect);
	if(shape->numlines == 0) return(MS_SUCCESS);
	msTransformShape(shape, map->extent, map->cellsize);
      }

      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) == MS_SUCCESS) {

	if(layer->labelangleitemindex != -1)
	  layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, length);
	else {
	  if(layer->class[c].color != -1) {
	    msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	  }
	  msDrawLabel(img, annopnt, shape->text, &(layer->class[c].label), &map->fontset);
	}
      }
      break;
    default: // points and anything with out a proper type
      for(j=0; j<shape->numlines;j++) {
	for(i=0; i<shape->line[j].numpoints;i++) {

	  point = &(shape->line[j].point[i]);

	  if(layer->transform) {
	    if(!msPointInRect(point, &map->extent)) return(MS_SUCCESS);
	    point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
	    point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
	  }

	  if(layer->labelangleitemindex != -1)
	    layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	  if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	    layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	  }

	  if(shape->text) {
	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, shape->tileindex, shape->index, *point, shape->text, -1);
	    else {
	      if(layer->class[c].color == -1) {
		msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
		if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	      }
	      msDrawLabel(img, *point, shape->text, &layer->class[c].label, &map->fontset);
	    }
	  }
	}
      }
    }
    break;

  case MS_LAYER_POINT:

#ifdef USE_PROJ
      if(msProjectionsDiffer(&(layer->projection), &(map->projection)))
          msProjectShape(&layer->projection, &map->projection, shape);
#endif

    for(j=0; j<shape->numlines;j++) {
      for(i=0; i<shape->line[j].numpoints;i++) {

	point = &(shape->line[j].point[i]);

	if(layer->transform) {
	  if(!msPointInRect(point, &map->extent)) continue; // next point
	  point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
	  point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
	}

	msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	if(overlay && layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);

	if(shape->text) {
	  if(layer->labelangleitemindex != -1)
	    layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	  if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	    layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	  }

	  if(layer->labelcache)
	    msAddLabel(map, layer->index, c, shape->tileindex, shape->index, *point, shape->text, -1);
	  else
	    msDrawLabel(img, *point, shape->text, &layer->class[c].label, &map->fontset);
	}
      }
    }
    break;

  case MS_LAYER_LINE:
    if(shape->type != MS_SHAPE_POLYGON && shape->type != MS_SHAPE_LINE){
      msSetError(MS_MISCERR, "Only polygon or line shapes can be drawn using a line layer definition.", "msDrawShape()");
      return(MS_FAILURE);
    }

#ifdef USE_PROJ
    if(msProjectionsDiffer(&(layer->projection), &(map->projection)))
        msProjectShape(&layer->projection, &map->projection, shape);
#endif

    if(layer->transform) {
      msClipPolylineRect(shape, cliprect);
      if(shape->numlines == 0) return(MS_SUCCESS);
      msTransformShape(shape, map->extent, map->cellsize);
    }

    msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].sizescaled);
    if(overlay && layer->class[c].overlaysymbol >= 0) msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);

    if(shape->text) {
      if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) == MS_SUCCESS) {
	if(layer->labelangleitemindex != -1)
	  layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}

	if(layer->class[c].label.autoangle)
	  layer->class[c].label.angle = angle;

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, length);
	else
	  msDrawLabel(img, annopnt, shape->text, &layer->class[c].label, &map->fontset);
      }
    }
    break;

  case MS_LAYER_POLYGON:
    if(shape->type != MS_SHAPE_POLYGON){
      msSetError(MS_MISCERR, "Only polygon shapes can be drawn using a POLYGON layer definition.", "msDrawShape()");
      return(MS_FAILURE);
    }

#ifdef USE_PROJ
    if(msProjectionsDiffer(&(layer->projection), &(map->projection))) 
        msProjectShape(&layer->projection, &map->projection, shape);
#endif

    if(layer->transform) {
      msClipPolygonRect(shape, cliprect);
      if(shape->numlines == 0) return(MS_SUCCESS);
      msTransformShape(shape, map->extent, map->cellsize);
    }

    if(layer->class[c].color < 0)
      msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].symbol, layer->class[c].outlinecolor, layer->class[c].backgroundcolor, layer->class[c].sizescaled);
    else
      msDrawShadeSymbol(&map->symbolset, img, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);

    if(overlay && layer->class[c].overlaysymbol >= 0) {
      if(layer->class[c].overlaycolor < 0)
	msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].overlaysymbol, layer->class[c].overlayoutlinecolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);
      else
        msDrawShadeSymbol(&map->symbolset, img, shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
    }

    if(shape->text) {
      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) == MS_SUCCESS) {
	if(layer->labelangleitemindex != -1)
	  layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, -1);
	else
	  msDrawLabel(img, annopnt, shape->text, &layer->class[c].label, &map->fontset);
      }
    }
    break;
  default:
    msSetError(MS_MISCERR, "Unknown layer type.", "msDrawShape()");
    return(MS_FAILURE);
  }

  return(MS_SUCCESS);
}

/*
** Function to draw a layer IF it already has a result cache associated with it. Called by msDrawQuery and via MapScript.
*/
int msDrawQueryLayer(mapObj *map, layerObj *layer, gdImagePtr img)
{
  int i, status;
  char annotate=MS_TRUE, cache=MS_FALSE;
  shapeObj shape;

  featureListNodeObjPtr shpcache=NULL, current=NULL;

  int colorbuffer[MS_MAXCLASSES];

  if(!layer->resultcache || map->querymap.style == MS_NORMAL) // done
    return(msDrawLayer(map, layer, img));

  if(!layer->data && !layer->tileindex && !layer->connection && !layer->features) 
   return(MS_SUCCESS); // no data associated with this layer, not an error since layer may be used as a template from MapScript

  if(layer->type == MS_LAYER_QUERY) return(MS_SUCCESS); // query only layers simply can't be drawn, not an error

  if(map->querymap.style == MS_HILITE) { // first, draw normally, but don't return
    status = msDrawLayer(map, layer, img);
    if(status != MS_SUCCESS) return(MS_FAILURE); // oops
  }

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) return(MS_SUCCESS);

  if(msEvalContext(map, layer->requires) == MS_FALSE) return(MS_SUCCESS);
  annotate = msEvalContext(map, layer->labelrequires);

  if(map->scale > 0) {
    if((layer->maxscale > 0) && (map->scale > layer->maxscale))
      return(MS_SUCCESS);
    if((layer->minscale > 0) && (map->scale <= layer->minscale))
      return(MS_SUCCESS);
    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
      annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
      annotate = MS_FALSE;
  }

  // if MS_HILITE, alter the first class (always at least 1 class)
  if(map->querymap.style == MS_HILITE) {
    for(i=0; i<layer->numclasses; i++) {
      colorbuffer[i] = layer->class[i].color; // save the color
      layer->class[i].color = map->querymap.color;
    }
  }

  // open this layer
  status = msLayerOpen(layer, map->shapepath);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  // build item list
  status = msLayerWhichItems(layer, MS_FALSE, annotate); // FIX: results have already been classified (this may change)
  if(status != MS_SUCCESS) return(MS_FAILURE);

  msInitShape(&shape);

  for(i=0; i<layer->resultcache->numresults; i++) {
    status = msLayerGetShape(layer, &shape, layer->resultcache->results[i].tileindex, layer->resultcache->results[i].shapeindex);
    if(status != MS_SUCCESS) return(MS_FAILURE);

    shape.classindex = layer->resultcache->results[i].classindex;
    if(layer->class[shape.classindex].status == MS_OFF) {
      msFreeShape(&shape);
      continue;
    }

    cache = MS_FALSE;
    if(layer->type == MS_LAYER_LINE || (layer->type == MS_LAYER_POLYGON && layer->class[shape.classindex].color < 0)) 
      cache = MS_TRUE; // only line/polyline layers need to (potentially) be cached with overlayed symbols

    if(annotate && (layer->class[shape.classindex].text.string || layer->labelitem) && layer->class[shape.classindex].label.size != -1)
      shape.text = msShapeGetAnnotation(layer, &shape);

    status = msDrawShape(map, layer, &shape, img, !cache); // if caching we DON'T want to do overlays at this time
    if(status != MS_SUCCESS) return(MS_FAILURE);

    if(shape.numlines == 0) { // once clipped the shape didn't need to be drawn
      msFreeShape(&shape);
      continue;
    }

    if(cache && layer->class[shape.classindex].overlaysymbol >= 0)
      if(insertFeatureList(&shpcache, &shape) == NULL) return(MS_FAILURE); // problem adding to the cache

    msFreeShape(&shape);
  }

  if(shpcache) {
    int c;

    for(current=shpcache; current; current=current->next) {
      c = current->shape.classindex;
      msDrawLineSymbol(&map->symbolset, img, &current->shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);
    }

    freeFeatureList(shpcache);
    shpcache = NULL;
  }

  // if MS_HILITE, restore values
  if(map->querymap.style == MS_HILITE)
    for(i=0; i<layer->numclasses; i++) layer->class[i].color = colorbuffer[i]; // restore the color

  msLayerClose(layer);

  return(MS_SUCCESS);
}

int msDrawLayer(mapObj *map, layerObj *layer, gdImagePtr img)
{
  int status;
  char annotate=MS_TRUE, cache=MS_FALSE;
  shapeObj shape;
  rectObj searchrect;

  featureListNodeObjPtr shpcache=NULL, current=NULL;

  if(!layer->data && !layer->tileindex && !layer->connection && !layer->features)
  return(MS_SUCCESS); // no data associated with this layer, not an error since layer may be used as a template from MapScript

  if(layer->type == MS_LAYER_QUERY) return(MS_SUCCESS);

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) return(MS_SUCCESS);

  if(msEvalContext(map, layer->requires) == MS_FALSE) return(MS_SUCCESS);
  annotate = msEvalContext(map, layer->labelrequires);

  if(map->scale > 0) {
    if((layer->maxscale > 0) && (map->scale > layer->maxscale))
      return(MS_SUCCESS);
    if((layer->minscale > 0) && (map->scale <= layer->minscale))
      return(MS_SUCCESS);
    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
      annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
      annotate = MS_FALSE;
  }

  // Redirect procesing of some layer types.
  if(layer->connectiontype == MS_WMS) return(msDrawWMSLayer(map, layer, img));

  if(layer->type == MS_LAYER_RASTER) return(msDrawRasterLayer(map, layer, img));

  // open this layer
  status = msLayerOpen(layer, map->shapepath);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  // build item list
  status = msLayerWhichItems(layer, MS_TRUE, annotate);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  // identify target shapes
  searchrect = map->extent;
#ifdef USE_PROJ
  if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
    msProjectRect(&map->projection, &layer->projection, &searchrect); // project the searchrect to source coords
#endif
  status = msLayerWhichShapes(layer, searchrect);
  if(status == MS_DONE) { // no overlap
    msLayerClose(layer);
    return(MS_SUCCESS);
  } else if(status != MS_SUCCESS)
    return(MS_FAILURE);

  // step through the target shapes
  msInitShape(&shape);

  while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {

    shape.classindex = msShapeGetClass(layer, &shape);
    if((shape.classindex == -1) || (layer->class[shape.classindex].status == MS_OFF)) {
      msFreeShape(&shape);
      continue;
    }

    cache = MS_FALSE;
    if(layer->type == MS_LAYER_LINE || (layer->type == MS_LAYER_POLYGON && layer->class[shape.classindex].color < 0)) 
      cache = MS_TRUE; // only line/polyline layers need to (potentially) be cached with overlayed symbols

    // With 'STYLEITEM AUTO', we will have the datasource fill the class'
    // style parameters for this shape.
    if (layer->styleitem && strcasecmp(layer->styleitem, "AUTO") == 0)
    {
        if (msLayerGetAutoStyle(map, layer, &(layer->class[shape.classindex]),
                                shape.tileindex, shape.index) != MS_SUCCESS)
        {
            return MS_FAILURE;
        }

        // Dynamic class update may have extended the color palette...
        if (!msUpdatePalette(img, &(map->palette)))
            return MS_FAILURE;

        // __TODO__ For now, we can't cache features with 'AUTO' style
        cache = MS_FALSE;
    }

    if(annotate && (layer->class[shape.classindex].text.string || layer->labelitem) && layer->class[shape.classindex].label.size != -1)
      shape.text = msShapeGetAnnotation(layer, &shape);

    status = msDrawShape(map, layer, &shape, img, !cache); // if caching we DON'T want to do overlays at this time
    if(status != MS_SUCCESS) {
      msLayerClose(layer);
      return(MS_FAILURE);
    }

    if(shape.numlines == 0) { // once clipped the shape didn't need to be drawn
      msFreeShape(&shape);
      continue;
    }

    if(cache && layer->class[shape.classindex].overlaysymbol >= 0)
      if(insertFeatureList(&shpcache, &shape) == NULL) return(MS_FAILURE); // problem adding to the cache

    msFreeShape(&shape);
  }

  if(status != MS_DONE) return(MS_FAILURE);

  if(shpcache) {
    int c;

    for(current=shpcache; current; current=current->next) {
      c = current->shape.classindex;
      msDrawLineSymbol(&map->symbolset, img, &current->shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlaysizescaled);
    }

    freeFeatureList(shpcache);
    shpcache = NULL;
  }

  msLayerClose(layer);
  return(MS_SUCCESS);
}

/*
** Save an image to a file named filename, if filename is NULL it goes to stdout
*/

int msSaveImage(gdImagePtr img, char *filename, int type, int transparent, int interlace, int quality)
{
  FILE *stream;

  if(filename != NULL && strlen(filename) > 0) {
    stream = fopen(filename, "wb");
    if(!stream) {
      msSetError(MS_IOERR, "(%s)", "msSaveImage()", filename);
      return(MS_FAILURE);
    }
  } else { /* use stdout */

#ifdef _WIN32
    /*
    ** Change stdout mode to binary on win32 platforms
    */
    if(_setmode( _fileno(stdout), _O_BINARY) == -1) {
      msSetError(MS_IOERR, "Unable to change stdout to binary mode.", "msSaveImage()");
      return(MS_FAILURE);
    }
#endif
    stream = stdout;
  }

  if(interlace)
    gdImageInterlace(img, 1);

  if(transparent)
    gdImageColorTransparent(img, 0);

  switch(type) {
  case(MS_GIF):
#ifdef USE_GD_GIF
    gdImageGif(img, stream);
#else
    msSetError(MS_MISCERR, "GIF output is not available.", "msSaveImage()");
    return(MS_FAILURE);
#endif
    break;
  case(MS_PNG):
#ifdef USE_GD_PNG
    gdImagePng(img, stream);
#else
    msSetError(MS_MISCERR, "PNG output is not available.", "msSaveImage()");
    return(MS_FAILURE);
#endif
    break;
  case(MS_JPEG):
#ifdef USE_GD_JPEG
    gdImageJpeg(img, stream, quality);
#else
     msSetError(MS_MISCERR, "JPEG output is not available.", "msSaveImage()");
     return(MS_FAILURE);
#endif
     break;
  case(MS_WBMP):
#ifdef USE_GD_WBMP
    gdImageWBMP(img, 1, stream);
#else
    msSetError(MS_MISCERR, "WBMP output is not available.", "msSaveImage()");
    return(MS_FAILURE);
#endif
    break;
  default:
    msSetError(MS_MISCERR, "Unknown output image type.", "msSaveImage()");
    return(MS_FAILURE);
  }

  if(filename != NULL && strlen(filename) > 0) fclose(stream);

  return(MS_SUCCESS);
}

void msFreeImage(gdImagePtr img)
{
  gdImageDestroy(img);
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

    if (map != NULL && map->numlayers > 0)
    {
        nCount = map->numlayers;
        papszGroups = (char **)malloc(sizeof(char *)*nCount);
       
        for (i=0; i<nCount; i++)
            papszGroups[i] = NULL;
       
        for (i=0; i<nCount; i++)
        {
            bFound = 0;
            if (map->layers[i].group)
            {
                for (j=0; j<*numTok; j++)
                {
                    if (papszGroups[j] &&
                        strcmp(map->layers[i].group, papszGroups[j]) == 0)
                    {
                        bFound = 1;
                        break;
                    }
                }
                if (!bFound)
                {
                    /* New group... add to the list of groups found */
                    papszGroups[(*numTok)] = strdup(map->layers[i].group);
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



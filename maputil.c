#include "map.h"
#include "mapparser.h"

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

int msShapeGetClass(layerObj *layer, shapeObj *shape)
{
  int i,j;
  char *tmpstr=NULL;
  char found=MS_FALSE;

  if((layer->numclasses == 1) && !(layer->class[0].expression.string)) /* no need to do lookup */
    return(0);

  for(i=0; i<layer->numclasses; i++) {
    if (layer->class[i].expression.string == NULL) /* Empty expression - always matches */
      found=MS_TRUE;
    else {
      switch(layer->class[i].expression.type) {
      case(MS_STRING):
        if(strcmp(layer->class[i].expression.string, shape->attributes[layer->classitemindex]) == 0) found=MS_TRUE; // got a match
        break;
      case(MS_EXPRESSION):
        tmpstr = strdup(layer->class[i].expression.string);        

        for(j=0; j<layer->numitems; j++) {
	  // FIX: need the right substr
	  tmpstr = gsub(tmpstr, layer->items[j], shape->attributes[layer->itemindexes[j]]);
	}

        msyystate = 4; msyystring = tmpstr;
        if(msyyparse() != 0)
	  return(-1);

        free(tmpstr);

        if(msyyresult) found=MS_TRUE; // got a match  
        break;
      case(MS_REGEX):
        if(regexec(&(layer->class[i].expression.regex), shape->attributes[layer->classitemindex], 0, NULL, 0) == 0) found=MS_TRUE; // got a match	  
        break;
      }
    }

    if(found) return(i);
  }

  return(-1); /* not found */
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
      for(i=0; i<layer->numitems; i++) {
	// FIX: need the right substr
	tmpstr = gsub(tmpstr, layer->items[i], shape->attributes[layer->itemindexes[i]]);
      }
      break;
    }
  } else {
    tmpstr = strdup(shape->attributes[layer->labelitemindex]);
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

  cellsize = MS_MAX((rect->maxx - rect->minx)/(width-1), (rect->maxy - rect->miny)/(height-1));

  if(cellsize <= 0) /* avoid division by zero errors */
    return(0);

  ox = MS_NINT(MS_MAX(((width-1) - (rect->maxx - rect->minx)/cellsize)/2,0));
  oy = MS_NINT(MS_MAX(((height-1) - (rect->maxy - rect->miny)/cellsize)/2,0));

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

  fprintf(stderr, "Drawing map...\n");

  if(map->width == -1 && map->height == -1) {
    msSetError(MS_MISCERR, "Image dimensions not specified.", "msDrawMap()");
    return(NULL);
  }

  if(map->width == -1 ||  map->height == -1)
    if(msAdjustImage(map->extent, &map->width, &map->height) == -1)
      return(NULL);

  img = gdImageCreate(map->width, map->height);
  if(!img) {
    msSetError(MS_GDERR, "Unable to initialize image.", "msDrawMap()");
    return(NULL);
  }  
  
  if(msLoadPalette(img, &(map->palette), map->imagecolor) == -1)
    return(NULL);
  
  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  map->scale = msCalculateScale(map->extent, map->units, map->width, map->height);

  for(i=0; i<map->numlayers; i++) { /* for each layer */

    lp = &(map->layers[i]);
    
    if(lp->postlabelcache) // wait to draw
      continue;

    status = msDrawLayer(map, lp, img);
    if(status != MS_SUCCESS) return(NULL);
  }

  if(map->scalebar.status == MS_EMBED && !map->scalebar.postlabelcache)
    msEmbedScalebar(map, img);

  if(map->legend.status == MS_EMBED && !map->legend.postlabelcache)
    msEmbedLegend(map, img);

  if(msDrawLabelCache(img, map) == -1) 
    return(NULL);

  for(i=0; i<map->numlayers; i++) { /* for each layer, check for postlabelcache layers */

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
  int i;
  gdImagePtr img=NULL;
  layerObj *lp=NULL;

  if(map->width == -1 && map->height == -1) {
    msSetError(MS_MISCERR, "Image dimensions not specified.", "msDrawQueryMap()");
    return(NULL);
  }

  if(map->width == -1 ||  map->height == -1)
    if(msAdjustImage(map->extent, &map->width, &map->height) == -1)
      return(NULL);

  if(map->querymap.width != -1) map->width = map->querymap.width;
  if(map->querymap.height != -1) map->height = map->querymap.height;

  img = gdImageCreate( map->width, map->height);
  if(!img) {
    msSetError(MS_GDERR, "Unable to initialize image.", "msDrawQueryMap()");
    return(NULL);
  }
  
  if(msLoadPalette(img, &(map->palette), map->imagecolor) == -1)
    return(NULL);

  map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
  map->scale = msCalculateScale(map->extent, map->units, map->width, map->height);

  for(i=0; i<map->numlayers; i++) { /* for each layer */

    lp = &(map->layers[i]);

    if(lp->postlabelcache) // wait to draw
      continue;

    msDrawQueryLayer(map, lp, img);
  }

  if(map->scalebar.status == MS_EMBED && !map->scalebar.postlabelcache)
    msEmbedScalebar(map, img);

  if(map->legend.status == MS_EMBED && !map->legend.postlabelcache)
    msEmbedLegend(map, img);

  if(msDrawLabelCache(img, map) == -1)
    return(NULL);
  
  for(i=0; i<map->numlayers; i++) { /* for each layer, check for postlabelcache layers */

    lp = &(map->layers[i]);
    
    if(!lp->postlabelcache) 
      continue;

    msDrawQueryLayer(map, lp, img);
  }

  if(map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache)
    msEmbedScalebar(map, img);

  if(map->legend.status == MS_EMBED && map->legend.postlabelcache)
    msEmbedLegend(map, img);

  return(img);
}

/*
** function to render an individual point
*/
int msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, gdImagePtr img, int classindex, char *labeltext)
{
  int c;
  char *text=NULL;
  double scalefactor=1.0;
  
  c = classindex;

  if(layer->symbolscale > 0 && map->scale > 0) scalefactor = layer->symbolscale/map->scale;

  layer->class[c].sizescaled = MS_NINT(layer->class[c].size * scalefactor);
  layer->class[c].sizescaled = MS_MAX(layer->class[c].sizescaled, layer->class[c].minsize);
  layer->class[c].sizescaled = MS_MIN(layer->class[c].sizescaled, layer->class[c].maxsize);
  layer->class[c].overlaysizescaled = layer->class[c].sizescaled - (layer->class[c].size - layer->class[c].overlaysize);
  // layer->class[c].overlaysizescaled = MS_NINT(layer->class[c].overlaysize * scalefactor);
  layer->class[c].overlaysizescaled = MS_MAX(layer->class[c].overlaysizescaled, layer->class[c].overlayminsize);
  layer->class[c].overlaysizescaled = MS_MIN(layer->class[c].overlaysizescaled, layer->class[c].overlaymaxsize);
#ifdef USE_TTF
  if(layer->class[c].label.type == MS_TRUETYPE) { 
    layer->class[c].label.sizescaled = MS_NINT(layer->class[c].label.size * scalefactor);
    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
  }
#endif      

  if(layer->class[c].sizescaled == 0) return(0);

#ifdef USE_PROJ
    if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
      msProjectPoint(layer->projection.proj, map->projection.proj, point);
#endif

  switch(layer->type) {      
  case MS_ANNOTATION:    
    if(layer->transform) {
      if(!msPointInRect(point, &map->extent)) return(0);
      point->x = MS_NINT((point->x - map->extent.minx)/map->cellsize); 
      point->y = MS_NINT((map->extent.maxy - point->y)/map->cellsize);
    }

    if(labeltext) text = labeltext;
    else text = layer->class[c].text.string;
      
    if(text) {
      if(layer->labelcache)
	msAddLabel(map, layer->index, c, -1, -1, *point, text, -1);
      else {
	if(layer->class[c].color == -1) {
	  msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	  if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	}
	msDrawLabel(img, *point, text, &layer->class[c].label, &map->fontset);
      }
    }
    break;

  case MS_POINT:
    if(layer->transform) {
      if(!msPointInRect(point, &map->extent)) return(0);
      point->x = MS_NINT((point->x - map->extent.minx)/map->cellsize); 
      point->y = MS_NINT((map->extent.maxy - point->y)/map->cellsize);
    }

    msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);

    if(labeltext) text = labeltext; 
    else text = layer->class[c].text.string;
	
    if(text) {
      if(layer->labelcache)
	msAddLabel(map, layer->index, c, -1, -1, *point, text, -1);
      else
	msDrawLabel(img, *point, text, &layer->class[c].label, &map->fontset);
    }
    break;
  default:
    break; /* don't do anything with layer of other types */
  }

  return(1); /* all done, no cleanup */
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
  pointObj annopnt;
  double angle, length;
  char *text=NULL;
  pointObj *point;

  cliprect.minx = map->extent.minx - 2*map->cellsize; // set clipping rectangle just a bit larger than the map extent
  cliprect.miny = map->extent.miny - 2*map->cellsize;
  cliprect.maxx = map->extent.maxx + 2*map->cellsize;
  cliprect.maxy = map->extent.maxy + 2*map->cellsize;

  c = shape->classindex;

  if(layer->class[c].sizescaled == 0) return(MS_SUCCESS);

#ifdef USE_PROJ
  if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
    msProjectPolyline(layer->projection.proj, map->projection.proj, shape);
#endif
   
  switch(layer->type) {      
  case MS_ANNOTATION:     

    fprintf(stderr, "drawing annotation shape...\n"); 

    switch(shape->type) {
    case(MS_LINE):
      if(layer->transform) {      
	msClipPolylineRect(shape, cliprect);
	if(shape->numlines == 0) return(MS_SUCCESS);
	msTransformShape(shape, map->extent, map->cellsize);
      }

      if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) != MS_SUCCESS) {
	if(shape->text) text = shape->text;
	else text = layer->class[c].text.string;
	
	if(layer->labelangleitemindex != -1) 
	  layer->class[c].label.angle = atof(shape->attributes[layer->labelangleitemindex])*MS_DEG_TO_RAD;
	
	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->attributes[layer->labelsizeitemindex])* ((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}
	
	if(layer->class[c].label.autoangle)
	  layer->class[c].label.angle = angle;
		
	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, text, length);
	else {
	  if(layer->class[c].color != -1) {
	    msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	  }
	  msDrawLabel(img, annopnt, text, &(layer->class[c].label), &map->fontset);	    
	}
      }

      break;
    case(MS_POLYGON):
      if(layer->transform) {      
	msClipPolygonRect(shape, cliprect);
	if(shape->numlines == 0) return(MS_SUCCESS);
	msTransformShape(shape, map->extent, map->cellsize);
      }

      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) != MS_SUCCESS) {
	if(shape->text) text = shape->text;
	else text = layer->class[c].text.string;

	if(layer->labelangleitemindex != -1) 
	  layer->class[c].label.angle = atof(shape->attributes[layer->labelangleitemindex])*MS_DEG_TO_RAD;
	
	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->attributes[layer->labelsizeitemindex])* ((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}
			
	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, text, length);
	else {
	  if(layer->class[c].color != -1) {
	    msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	  }
	  msDrawLabel(img, annopnt, text, &(layer->class[c].label), &map->fontset);	    
	}
      }
      break;
    default: // points and anything with out a proper type
      for(j=0; j<shape->numlines;j++) {
	for(i=0; i<shape->line[j].numpoints;i++) {
	  
	  point = &(shape->line[j].point[i]);
	  
	  if(layer->transform) {
	    if(!msPointInRect(point, &map->extent)) return(MS_SUCCESS);
	    point->x = MS_NINT((point->x - map->extent.minx)/map->cellsize); 
	    point->y = MS_NINT((map->extent.maxy - point->y)/map->cellsize);
	  }
	  
	  if(shape->text) text = shape->text;
	  else text = layer->class[c].text.string;

	  if(layer->labelangleitemindex != -1) 
	    layer->class[c].label.angle = atof(shape->attributes[layer->labelangleitemindex])*MS_DEG_TO_RAD;
	  
	  if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	    layer->class[c].label.sizescaled = atoi(shape->attributes[layer->labelsizeitemindex])* ((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
	    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	  }
	  
	  if(text) {
	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, shape->tileindex, shape->index, *point, text, -1);
	    else {
	      if(layer->class[c].color == -1) {
		msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
		if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	      }
	      msDrawLabel(img, *point, text, &layer->class[c].label, &map->fontset);
	    }
	  }
	}
      }
    }
    break;

  case MS_POINT:
    fprintf(stderr, "drawing point shape...\n"); 

    for(j=0; j<shape->numlines;j++) {
      for(i=0; i<shape->line[j].numpoints;i++) {
	  
	point = &(shape->line[j].point[i]);
	
	if(layer->transform) {
	  if(!msPointInRect(point, &map->extent)) return(MS_SUCCESS);
	  point->x = MS_NINT((point->x - map->extent.minx)/map->cellsize); 
	  point->y = MS_NINT((map->extent.maxy - point->y)/map->cellsize);
	}

	msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	if(overlay && layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);

	if(shape->text) text = shape->text; 
	else text = layer->class[c].text.string;


	if(text) {
	  if(layer->labelangleitemindex != -1) 
	    layer->class[c].label.angle = atof(shape->attributes[layer->labelangleitemindex])*MS_DEG_TO_RAD;
	  
	  if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	    layer->class[c].label.sizescaled = atoi(shape->attributes[layer->labelsizeitemindex])* ((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
	    layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	    layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	  }

	  if(layer->labelcache)
	    msAddLabel(map, layer->index, c, shape->tileindex, shape->index, *point, text, -1);
	  else
	    msDrawLabel(img, *point, text, &layer->class[c].label, &map->fontset);
	}
      }    
    }
    break;      

  case MS_LINE:    
    if(shape->type != MS_POLYGON && shape->type != MS_LINE){ 
      msSetError(MS_MISCERR, "Only polygon or line shapes can be drawn using a line layer definition.", "msDrawShape()");
      return(MS_FAILURE);
    }

    if(layer->transform) {
      msClipPolylineRect(shape, cliprect);
      if(shape->numlines == 0) return(MS_SUCCESS);
      msTransformShape(shape, map->extent, map->cellsize);
    }

    msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
    if(overlay && layer->class[c].overlaysymbol >= 0) msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
    
    if(shape->text) text = shape->text;
    else text = layer->class[c].text.string;
    
    if(text) {
      if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) != MS_SUCCESS) {
	if(layer->labelangleitemindex != -1) 
	  layer->class[c].label.angle = atof(shape->attributes[layer->labelangleitemindex])*MS_DEG_TO_RAD;
	
	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->attributes[layer->labelsizeitemindex])* ((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}
	
	if(layer->class[c].label.autoangle)
	  layer->class[c].label.angle = angle;

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, text, length);
	else
	  msDrawLabel(img, annopnt, text, &layer->class[c].label, &map->fontset);
      }
    }
    break;

  case MS_POLYLINE:
    if(shape->type != MS_POLYGON){ 
      msSetError(MS_MISCERR, "Only polygon shapes can be drawn using a polyline layer definition.", "msDrawShape()");
      return(MS_FAILURE);
    }

    if(layer->transform) {      
      msClipPolygonRect(shape, cliprect);
      if(shape->numlines == 0) return(MS_SUCCESS);
      msTransformShape(shape, map->extent, map->cellsize);
    }
    
    msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
    if(overlay && layer->class[c].overlaysymbol >= 0) msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	
    if(shape->text) text = shape->text; 
    else text = layer->class[c].text.string;
    
    if(text) {
      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) != MS_SUCCESS) {
	if(layer->labelangleitemindex != -1) 
	  layer->class[c].label.angle = atof(shape->attributes[layer->labelangleitemindex])*MS_DEG_TO_RAD;
	
	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->attributes[layer->labelsizeitemindex])* ((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}
	
	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, text, -1);
	else
	  msDrawLabel(img, annopnt, text, &layer->class[c].label, &map->fontset);
      }
    }
    break;
  case MS_POLYGON:
    // if(shape->type != MS_POLYGON){ 
    //   msSetError(MS_MISCERR, "Only polygon shapes can be drawn using a polygon layer definition.", "msDrawShape()");
    //   return(MS_FAILURE);
    // }

    if(layer->transform) {
      msClipPolygonRect(shape, cliprect);
      if(shape->numlines == 0) return(MS_SUCCESS);
      msTransformShape(shape, map->extent, map->cellsize);
    }

    msDrawShadeSymbol(&map->symbolset, img, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
    if(overlay && layer->class[c].overlaysymbol >= 0) msDrawShadeSymbol(&map->symbolset, img, shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
      
    if(shape->text) text = shape->text;
    else text = layer->class[c].text.string;

    if(text) {
      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) != MS_SUCCESS) {
	if(layer->labelangleitemindex != -1) 
	  layer->class[c].label.angle = atof(shape->attributes[layer->labelangleitemindex])*MS_DEG_TO_RAD;
	
	if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	  layer->class[c].label.sizescaled = atoi(shape->attributes[layer->labelsizeitemindex])* ((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
	  layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	  layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	}

	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, text, -1);
	else
	  msDrawLabel(img, annopnt, text, &layer->class[c].label, &map->fontset);
      }
    }
    break;      
  default:
    msSetError(MS_MISCERR, "Unknown layer type.", "msDrawShape()");
    return(MS_FAILURE);
  }

  return(MS_SUCCESS);
}

int msDrawQueryLayer(mapObj *map, layerObj *layer, gdImagePtr img)
{
  // use query results to guide drawing instead of *next* (i.e. use *get*)
  return(0);
}

int msDrawLayer(mapObj *map, layerObj *layer, gdImagePtr img)
{
  int i;
  char status;
  char annotate=MS_TRUE, cache=MS_FALSE;
  shapeObj shape;
  double scalefactor=1;

  featureListNodeObjPtr shpcache=NULL, current=NULL;

  if(layer->type == MS_RASTER) return(msDrawRasterLayer(map, layer, img));
  if(layer->type == MS_QUERY) return(MS_SUCCESS);

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) return(MS_SUCCESS);

  // FIX: MUCH OF THIS SCALE SHIT NEEDS TO BE MOVED

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

  if(layer->symbolscale > 0) scalefactor = layer->symbolscale/map->scale;
  
  for(i=0; i<layer->numclasses; i++) {
    layer->class[i].sizescaled = MS_NINT(layer->class[i].size * scalefactor);
    layer->class[i].sizescaled = MS_MAX(layer->class[i].sizescaled, layer->class[i].minsize);
    layer->class[i].sizescaled = MS_MIN(layer->class[i].sizescaled, layer->class[i].maxsize);
    layer->class[i].overlaysizescaled = layer->class[i].sizescaled - (layer->class[i].size - layer->class[i].overlaysize);
    // layer->class[i].overlaysizescaled = MS_NINT(layer->class[i].overlaysize * scalefactor);
    layer->class[i].overlaysizescaled = MS_MAX(layer->class[i].overlaysizescaled, layer->class[i].overlayminsize);
    layer->class[i].overlaysizescaled = MS_MIN(layer->class[i].overlaysizescaled, layer->class[i].overlaymaxsize);
#ifdef USE_TTF
    if(layer->class[i].label.type == MS_TRUETYPE) { 
      layer->class[i].label.sizescaled = MS_NINT(layer->class[i].label.size * scalefactor);
      layer->class[i].label.sizescaled = MS_MAX(layer->class[i].label.sizescaled, layer->class[i].label.minsize);
      layer->class[i].label.sizescaled = MS_MIN(layer->class[i].label.sizescaled, layer->class[i].label.maxsize);
    }
#endif
  }
  
  fprintf(stderr, "Drawing layer %s...\n", layer->name); 

  // open this layer
  status = msLayerOpen(layer, map->shapepath);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  fprintf(stderr, "opened layer...\n"); 

  // build item list
  status = msLayerWhichItems(layer, annotate);
  if(status != MS_SUCCESS) return(MS_FAILURE);

  fprintf(stderr, "built item list...\n");

  // identify target shapes
  status = msLayerWhichShapes(layer, map->shapepath, map->extent, &(map->projection));
  if(status != MS_SUCCESS) return(MS_FAILURE);

  fprintf(stderr, "run msWhichShapes()...\n");

  // step through the target shapes
  msInitShape(&shape);

  fprintf(stderr, "shape initialized...\n");

  if(layer->type == MS_LINE || layer->type == MS_POLYLINE) cache = MS_TRUE; // only line/polyline layers need to (potentially) be cached with overlayed symbols

  while((status = msLayerNextShape(layer, map->shapepath, &shape, MS_TRUE)) == MS_SUCCESS) {

    fprintf(stderr, "processing shape...\n");

    shape.classindex = msShapeGetClass(layer, &shape);    
    if(shape.classindex == -1) {
      msFreeShape(&shape);
      continue;
    }

    fprintf(stderr, "got class...\n");

    if(annotate && (layer->class[shape.classindex].text.string || layer->labelitem)) 
      shape.text = msShapeGetAnnotation(layer, &shape);

    if(shape.text) fprintf(stderr, "got annotation (%s)...\n", shape.text);

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

  if(status != MS_DONE) return(MS_FAILURE);

  if(shpcache) {
    int c;

    for(current=shpcache; current; current=current->next) {
      c = current->shape.classindex;
      msDrawLineSymbol(&map->symbolset, img, &current->shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
    }

    freeFeatureList(shpcache);
    shpcache = NULL;
  }

  fprintf(stderr, "done drawing layer %s...\n", layer->name); 

  return(MS_SUCCESS);
}

/*
** Save an image to a file named filename, if filename is NULL it goes to stdout
*/
#ifndef USE_GD_1_8 
int msSaveImage(gdImagePtr img, char *filename, int transparent, int interlace)
{
  FILE *stream;

  if(filename != NULL && strlen(filename) > 0) {
    stream = fopen(filename, "wb");
    if(!stream) {
      sprintf(ms_error.message, "(%s)", filename);
      msSetError(MS_IOERR, ms_error.message, "msSaveImage()");      
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

#ifndef USE_GD_1_6 
  gdImageGif(img, stream);
#else	
  gdImagePng(img, stream);
#endif
  
  if(filename != NULL && strlen(filename) > 0) fclose(stream);

  return(MS_SUCCESS);
}
#else
int msSaveImage(gdImagePtr img, char *filename, int type, int transparent, int interlace, int quality)
{
  FILE *stream;

  if(filename != NULL && strlen(filename) > 0) {
    stream = fopen(filename, "wb");
    if(!stream) {
       sprintf(ms_error.message, "(%s)", filename);
      msSetError(MS_IOERR, ms_error.message, "msSaveImage()");     
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

  if(type == MS_PNG)
    gdImagePng(img, stream);
  else if(type == MS_JPEG)
    gdImageJpeg(img, stream, quality);
  else
    gdImageWBMP(img, 1, stream);

  if(filename != NULL && strlen(filename) > 0) fclose(stream);

  return(MS_SUCCESS);
}
#endif

void msFreeImage(gdImagePtr img)
{
  gdImageDestroy(img);
}

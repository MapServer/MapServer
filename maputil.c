#include "map.h"
#include "mapparser.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

extern int msyyparse();
extern int msyylex();
extern char *msyytext;

/*
** Load item names into a character array
*/
char **msGetDBFItems(DBFHandle dbffile)
{
  char **items;
  int i, nFields;
  char fName[32];

  if((nFields = DBFGetFieldCount(dbffile)) == 0) {
    msSetError(MS_DBFERR, "File contains no data.", "msGetDBFItems()");
    return(NULL);
  }

  if((items = (char **)malloc(sizeof(char *)*nFields)) == NULL) {
    msSetError(MS_MEMERR, NULL, "msGetDBFItems()");
    return(NULL);
  }

  for(i=0;i<nFields;i++) {
    DBFGetFieldInfo(dbffile, i, fName, NULL, NULL);
    items[i] = strdup(fName);
  }

  return(items);
}

/*
** Load item values into a character array
*/
char **msGetDBFValues(DBFHandle dbffile, int record)
{
  char **values;
  int i, nFields;

  if((nFields = DBFGetFieldCount(dbffile)) == 0) {
    msSetError(MS_DBFERR, "File contains no data.", "msGetDBFValues()");
    return(NULL);
  }

  if((values = (char **)malloc(sizeof(char *)*nFields)) == NULL) {
    msSetError(MS_MEMERR, NULL, "msGetDBFValues()");
    return(NULL);
  }

  for(i=0;i<nFields;i++)
    values[i] = strdup(DBFReadStringAttribute(dbffile, record, i));

  return(values);
}

/*
** Which column number in the .DBF file does the item correspond to
*/
int msGetItemIndex(DBFHandle dbffile, char *name)
{
  int i;
  DBFFieldType dbfField;
  int fWidth,fnDecimals; /* field width and number of decimals */    
  char fName[32]; /* field name */

  if(!name) {
    msSetError(MS_MISCERR, "NULL item name passed.", "msGetItemIndex()");    
    return(-1);
  }

  /* does name exist as a field? */
  for(i=0;i<DBFGetFieldCount(dbffile);i++) {
    dbfField = DBFGetFieldInfo(dbffile,i,fName,&fWidth,&fnDecimals);
    if(strcasecmp(name,fName) == 0) /* found it */
      return(i);
  }

  msSetError(MS_DBFERR, NULL, "msFindRecords()");
  sprintf(ms_error.message, "Item %s not found.", name);
  return(-1); /* item not found */
}

extern int msyyresult; // result of parsing, true/false
extern int msyystate;
extern char *msyystring;
extern int msyyparse();

/* 
** Returns class index for a given layer and record, non-shapefile data. Used in mapraster.c as well.
*/
int getClassIndex(layerObj *layer, char *str)
{
  int i;
  char *tmpstr=NULL;

  if((layer->numclasses == 1) && !(layer->class[0].expression.string)) /* no need to do lookup */
    return(0);

  if(!str) return(-1);

  for(i=0; i<layer->numclasses; i++) {
    switch(layer->class[i].expression.type) {
    case(MS_STRING):
      if(strcmp(layer->class[i].expression.string, str) == 0) /* got a match */
	return(i);
      break;
    case(MS_REGEX):
      if(regexec(&(layer->class[i].expression.regex), str, 0, NULL, 0) == 0) /* got a match */
	return(i);
      break;
    case(MS_EXPRESSION):
      tmpstr = strdup(layer->class[i].expression.string);
      tmpstr = gsub(tmpstr, "[value]", str);

      msyystate = 4; msyystring = tmpstr;
      if(msyyparse() != 0)
	return(-1);

      free(tmpstr);

      if(msyyresult) /* got a match */
	return(i);
    }
  }

  return(-1); /* not found */
}

/* 
** Returns class index for a given layer and record, shapefile data only, supports logical expressions
*/
static int shpGetClassIndex(DBFHandle hDBF, layerObj *layer, int record, int item)
{
  int i,j;
  int numitems=0;
  char *tmpstr=NULL, **values=NULL;
  int found=MS_FALSE;

  if((layer->numclasses == 1) && !(layer->class[0].expression.string)) /* no need to do lookup */
    return(0);

  for(i=0; i<layer->numclasses; i++) {
    switch(layer->class[i].expression.type) {
    case(MS_STRING):
      if(strcmp(layer->class[i].expression.string, DBFReadStringAttribute(hDBF, record, item)) == 0) /* got a match */
	found=MS_TRUE;
      break;
    case(MS_EXPRESSION):
      tmpstr = strdup(layer->class[i].expression.string);

      if(!values) {
	numitems = DBFGetFieldCount(hDBF);
	values = msGetDBFValues(hDBF, record);
      }

      if(!(layer->class[i].expression.items)) { // build cache of item replacements	
	char **items=NULL, substr[19];

	numitems = DBFGetFieldCount(hDBF);
	items = msGetDBFItems(hDBF);

	layer->class[i].expression.items = (char **)malloc(numitems);
	layer->class[i].expression.indexes = (int *)malloc(numitems);

	for(j=0; j<numitems; j++) {
	  sprintf(substr, "[%s]", items[j]);
	  if(strstr(tmpstr, substr) != NULL) {
	    layer->class[i].expression.indexes[layer->class[i].expression.numitems] = j;
	    layer->class[i].expression.items[layer->class[i].expression.numitems] = strdup(substr);
	    layer->class[i].expression.numitems++;
	  }
	}

	msFreeCharArray(items,numitems);
      }

      for(j=0; j<layer->class[i].expression.numitems; j++)
	tmpstr = gsub(tmpstr, layer->class[i].expression.items[j], values[layer->class[i].expression.indexes[j]]);

      msyystate = 4; msyystring = tmpstr;
      if(msyyparse() != 0)
	return(-1);

      free(tmpstr);

      if(msyyresult) /* got a match */
	found=MS_TRUE;
      break;
    case(MS_REGEX):
      if(regexec(&(layer->class[i].expression.regex), DBFReadStringAttribute(hDBF, record, item), 0, NULL, 0) == 0) /* got a match */
	found=MS_TRUE;
      break;
    }

    if(found) {
      if(values) msFreeCharArray(values,numitems);
      return(i);
    }
  }

  return(-1); /* not found */
}

/*
** Return the annotation string to use for a given layer/class/feature combination.
** Feature and class numbers are assumed to be valid.
*/
static char *shpGetAnnotation(DBFHandle hDBF, classObj *class, int record, int item)
{
  int i;
  char *tmpstr=NULL;
  char **values;
  int numitems;

  if(class->text.string) { // test for global label first
    tmpstr = strdup(class->text.string);
    switch(class->text.type) {
    case(MS_STRING):
      break;
    case(MS_EXPRESSION):
      numitems = DBFGetFieldCount(hDBF);
      values = msGetDBFValues(hDBF, record);

      if(!(class->text.items)) { // build cache of item replacements	
	char **items=NULL, substr[19];

	numitems = DBFGetFieldCount(hDBF);
	items = msGetDBFItems(hDBF);

	class->text.items = (char **)malloc(numitems);
	class->text.indexes = (int *)malloc(numitems);

	for(i=0; i<numitems; i++) {
	  sprintf(substr, "[%s]", items[i]);
	  if(strstr(tmpstr, substr) != NULL) {
	    class->text.indexes[class->text.numitems] = i;
	    class->text.items[class->text.numitems] = strdup(substr);
	    class->text.numitems++;
	  }
	}

	msFreeCharArray(items,numitems);
      }

      for(i=0; i<class->text.numitems; i++)
	tmpstr = gsub(tmpstr, class->text.items[i], values[class->text.indexes[i]]);

      msFreeCharArray(values,numitems);
      break;
    }
  } else {
    if(item < 0) return(NULL);
    tmpstr = strdup(DBFReadStringAttribute(hDBF, record, item));
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

    // check for inline data
    if(lp->features) {
      msDrawInlineLayer(map, lp, img);
      continue;
    }

    // check for remote data
    if(lp->connection && lp->connectiontype == MS_SDE) {
      if(msDrawSDELayer(map, lp, img) == -1) return(NULL);
      continue;
    }

    if(lp->connection && lp->connectiontype == MS_OGR) {
      if(msDrawOGRLayer(map, lp, img) == -1) return(NULL);
      continue;
    }

    // must be local files
    if(lp->type == MS_RASTER) {
      if(msDrawRasterLayer(map, lp, img) == -1) return(NULL);
    } else {	
      if(msDrawShapefileLayer(map, lp, img, NULL) == -1) return(NULL);
    }

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

    // check for inline data
    if(lp->features) {
      msDrawInlineLayer(map, lp, img);
      continue;
    }

    // check for remote data
    if(lp->connection && lp->connectiontype == MS_SDE) {
      if(msDrawSDELayer(map, lp, img) == -1) return(NULL);
      continue;
    }

    if(lp->connection && lp->connectiontype == MS_OGR) {
      if(msDrawOGRLayer(map, lp, img) == -1) return(NULL);
      continue;
    }

    // must be local files
    if(lp->type == MS_RASTER) {
      if(msDrawRasterLayer(map, lp, img) == -1) return(NULL);
    } else {	
      if(msDrawShapefileLayer(map, lp, img, NULL) == -1) return(NULL);
    }
  }

  if(map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache)
    msEmbedScalebar(map, img);

  if(map->legend.status == MS_EMBED && map->legend.postlabelcache)
    msEmbedLegend(map, img);

  return(img);
}

gdImagePtr msDrawQueryMap(mapObj *map, queryResultObj *results)
{
  int i,j;
  gdImagePtr img=NULL;
  layerObj *lp=NULL;
  char color_buffer[MS_MAXCLASSES];

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

    if(lp->features)
      msDrawInlineLayer(map, lp, img);
    else {
      if(lp->type == MS_RASTER) { 
	if(msDrawRasterLayer(map, lp, img) == -1) return(NULL);
      } else {
	
	switch(map->querymap.style) {
	case(MS_NORMAL):
	  if(msDrawShapefileLayer(map, lp, img, NULL) == -1) return(NULL);
	  break;
	case(MS_HILITE): /* draw selected features in special color, other features get drawn normally */
	  if(msDrawShapefileLayer(map, lp, img, NULL) == -1) return(NULL); /* 1st, draw normally */
	  if(results->layers[i].status && results->layers[i].numresults > 0) {
	    for(j=0; j<lp->numclasses; j++) {
	      color_buffer[j] = lp->class[j].color; // save the color
	      lp->class[j].color = map->querymap.color;
	    }
	    if(msDrawShapefileLayer(map, lp, img, results->layers[i].status) == -1) return(NULL);
	    for(j=0; j<lp->numclasses; j++)	      
	      lp->class[j].color = color_buffer[j]; // restore the color
	  }
	  break;
	case(MS_SELECTED): /* draw only the selected features, normally */
	  if(!results->layers[i].status) {
	    if(msDrawShapefileLayer(map, lp, img, NULL) == -1) return(NULL);
	  } else {
	    if(results->layers[i].numresults > 0)
	      if(msDrawShapefileLayer(map, lp, img, results->layers[i].status) == -1) return(NULL);
	  }
	  break;
	}
      }
    }
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

    if(lp->features)
      msDrawInlineLayer(map, lp, img);
    else
      if(lp->type == MS_RASTER) {
	if(msDrawRasterLayer(map, lp, img) == -1) return(NULL);
      } else {	
	if(msDrawShapefileLayer(map, lp, img, NULL) == -1) return(NULL);
      }
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
int msDrawPoint(mapObj *map, layerObj *layer, pointObj *point, gdImagePtr img, char *class_string, char *label_string) 
{
  int c;
  char *text=NULL;
  double scalefactor=1;
  
  if((c = getClassIndex(layer, class_string)) == -1) return(0);

  // apply scaling to symbols and fonts
  if(layer->symbolscale > 0 && map->scale > 0) {
    scalefactor = layer->symbolscale/map->scale;
    layer->class[c].sizescaled = MS_NINT(layer->class[c].size * scalefactor);
    layer->class[c].sizescaled = MS_MAX(layer->class[c].sizescaled, layer->class[c].minsize);
    layer->class[c].sizescaled = MS_MIN(layer->class[c].sizescaled, layer->class[c].maxsize);
    layer->class[c].overlaysizescaled = MS_NINT(layer->class[c].overlaysize * scalefactor);
    layer->class[c].overlaysizescaled = MS_MAX(layer->class[c].overlaysizescaled, layer->class[c].overlayminsize);
    layer->class[c].overlaysizescaled = MS_MIN(layer->class[c].overlaysizescaled, layer->class[c].overlaymaxsize);
#ifdef USE_TTF
    if(layer->class[c].label.type == MS_TRUETYPE) { 
      layer->class[c].label.sizescaled = MS_NINT(layer->class[c].label.size * scalefactor);
      layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
      layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
    }
#endif      
  }

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

    if(label_string) text = label_string;
    else text = layer->class[c].text.string;
      
    if(text) {
      if(layer->labelcache)
	msAddLabel(map, layer->index, c, -1, -1, *point, text, -1);
      else {
	if(layer->class[c].color == -1) {
	  msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	  if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	}
	msDrawLabel(img, map, *point, text, &layer->class[c].label);
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

    if(label_string) text = label_string; 
    else text = layer->class[c].text.string;
	
    if(text) {
      if(layer->labelcache)
	msAddLabel(map, layer->index, c, -1, -1, *point, text, -1);
      else
	msDrawLabel(img, map, *point, text, &layer->class[c].label);
    }
    break;
  default:
    break; /* don't do anything with layer of other types */
  }

  return(1); /* all done, no cleanup */
}

/*
** function to render an individual shape
*/
int msDrawShape(mapObj *map, layerObj *layer, shapeObj *shape, gdImagePtr img, char *class_string, char *label_string) 
{
  int i,j,c;
  rectObj cliprect;
  pointObj annopnt;
  double angle, length;
  char *text=NULL;
  pointObj *point;
  double scalefactor=1;

  /* Set clipping rectangle */
  cliprect.minx = map->extent.minx - 2*map->cellsize; /* just a bit larger than the map extent */
  cliprect.miny = map->extent.miny - 2*map->cellsize;
  cliprect.maxx = map->extent.maxx + 2*map->cellsize;
  cliprect.maxy = map->extent.maxy + 2*map->cellsize;

  if((c = getClassIndex(layer, class_string)) == -1) return(0);

  // apply scaling to symbols and fonts
  if(layer->symbolscale > 0 && map->scale > 0) {
    scalefactor = layer->symbolscale/map->scale;
    layer->class[c].sizescaled = MS_NINT(layer->class[c].size * scalefactor);
    layer->class[c].sizescaled = MS_MAX(layer->class[c].sizescaled, layer->class[c].minsize);
    layer->class[c].sizescaled = MS_MIN(layer->class[c].sizescaled, layer->class[c].maxsize);
    layer->class[c].overlaysizescaled = MS_NINT(layer->class[c].overlaysize * scalefactor);
    layer->class[c].overlaysizescaled = MS_MAX(layer->class[c].overlaysizescaled, layer->class[c].overlayminsize);
    layer->class[c].overlaysizescaled = MS_MIN(layer->class[c].overlaysizescaled, layer->class[c].overlaymaxsize);
#ifdef USE_TTF
    if(layer->class[c].label.type == MS_TRUETYPE) { 
      layer->class[c].label.sizescaled = MS_NINT(layer->class[c].label.size * scalefactor);
      layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
      layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
    }
#endif      
  }

#ifdef USE_PROJ
  if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
    msProjectPolyline(layer->projection.proj, map->projection.proj, shape);
#endif
    
  switch(layer->type) {      
  case MS_ANNOTATION:     
    for(j=0; j<shape->numlines;j++) {
      for(i=0; i<shape->line[j].numpoints;i++) {

	point = &(shape->line[j].point[i]);
	  
	if(layer->transform) {
	  if(!msPointInRect(point, &map->extent)) return(0);
	  point->x = MS_NINT((point->x - map->extent.minx)/map->cellsize); 
	  point->y = MS_NINT((map->extent.maxy - point->y)/map->cellsize);
	}
	  
	if(label_string) text = label_string;
	else text = layer->class[c].text.string;

	if(text) {
	  if(layer->labelcache)
	    msAddLabel(map, layer->index, c, -1, -1, *point, text, -1);
	  else {
	    if(layer->class[c].color == -1) {
	      msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	      if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	    }
	    msDrawLabel(img, map, *point, text, &layer->class[c].label);
	  }
	}
      }    
    }
    break;

  case MS_POINT:
    for(j=0; j<shape->numlines;j++) {
      for(i=0; i<shape->line[j].numpoints;i++) {
	  
	point = &(shape->line[j].point[i]);
	
	if(layer->transform) {
	  if(!msPointInRect(point, &map->extent)) return(0);
	  point->x = MS_NINT((point->x - map->extent.minx)/map->cellsize); 
	  point->y = MS_NINT((map->extent.maxy - point->y)/map->cellsize);
	}

	msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, point, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);

	if(label_string) text = label_string; 
	else text = layer->class[c].text.string;

	if(text) {
	  if(layer->labelcache)
	    msAddLabel(map, layer->index, c, -1, -1, *point, text, -1);
	  else
	    msDrawLabel(img, map, *point, text, &layer->class[c].label);
	}
      }    
    }
    break;      

  case MS_LINE:
    if(layer->transform) {
      msClipPolylineRect(shape, cliprect, shape);
      if(shape->numlines == 0) return(0);
      msTransformPolygon(map->extent, map->cellsize, shape);
    }
    msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);

    if(label_string) text = label_string;
    else text = layer->class[c].text.string;
    
    if(text) {
      if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) != -1) {
	if(layer->class[c].label.autoangle)
	  layer->class[c].label.angle = angle;
	
	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, -1, -1, annopnt, text, length);
	else
	  msDrawLabel(img, map, annopnt, text, &layer->class[c].label);
      }
    }
    break;

  case MS_POLYLINE:
   if(layer->transform) {      
      msClipPolygonRect(shape, cliprect, shape);
      if(shape->numlines == 0) return(0);
      msTransformPolygon(map->extent, map->cellsize, shape);
    }

   msDrawLineSymbol(&map->symbolset, img, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);

    if(label_string) text = label_string; 
    else text = layer->class[c].text.string;

    if(text) {
      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) != -1) {
	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, -1, -1, annopnt, text, -1);
	else
	  msDrawLabel(img, map, annopnt, text, &layer->class[c].label);
      }
    }
    break;
  case MS_POLYGON: 
    if(layer->transform) {      
      msClipPolygonRect(shape, cliprect, shape);
      if(shape->numlines == 0) return(0);
      msTransformPolygon(map->extent, map->cellsize, shape);
    }

    msDrawShadeSymbol(&map->symbolset, img, shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
    if(layer->class[c].overlaysymbol >= 0) msDrawShadeSymbol(&map->symbolset, img, shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
      
    if(label_string) text = label_string; 
    else text = layer->class[c].text.string;

    if(text) {
      if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) != -1) {
	if(layer->labelcache)
	  msAddLabel(map, layer->index, c, -1, -1, annopnt, text, -1);
	else
	  msDrawLabel(img, map, annopnt, text, &layer->class[c].label);
      }
    }
    break;      
  default:
    msSetError(MS_MISCERR, "Unknown layer type.", "msDrawShape()");
    return(-1);
  }

  return(1); /* all done, no cleanup */
}

/*
** function to draw features defined within a map file
*/
int msDrawInlineLayer(mapObj *map, layerObj *layer, gdImagePtr img)
{
  int i,j,c;
  featureListNodeObjPtr current=NULL;
  rectObj cliprect;
  short annotate=MS_TRUE;
  pointObj annopnt;
  double angle, length;
  char *text;
  pointObj *pnt;
  double scalefactor=1;

  int overlay = MS_FALSE;

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT))
    return(0);

  if(map->scale > 0) {
    if((layer->maxscale > 0) && (map->scale > layer->maxscale))
      return(0);
    if((layer->minscale > 0) && (map->scale <= layer->minscale))
      return(0);
    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
      annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
      annotate = MS_FALSE;
  }

  // apply scaling to symbols and fonts
  if(layer->symbolscale > 0) {
    scalefactor = layer->symbolscale/map->scale;
    for(i=0; i<layer->numclasses; i++) {
      if(layer->class[i].overlaysymbol >= 0) overlay = MS_TRUE;
      layer->class[i].sizescaled = MS_NINT(layer->class[i].size * scalefactor);
      layer->class[i].sizescaled = MS_MAX(layer->class[i].sizescaled, layer->class[i].minsize);
      layer->class[i].sizescaled = MS_MIN(layer->class[i].sizescaled, layer->class[i].maxsize);
      layer->class[i].overlaysizescaled = MS_NINT(layer->class[i].overlaysize * scalefactor);
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
  }
 
  /* Set clipping rectangle (used by certain layer types only) */
  if(layer->transform && (layer->type == MS_POLYGON || layer->type == MS_POLYLINE)) {
      cliprect.minx = map->extent.minx - 2*map->cellsize; /* just a bit larger than the map extent */
      cliprect.miny = map->extent.miny - 2*map->cellsize;
      cliprect.maxx = map->extent.maxx + 2*map->cellsize;
      cliprect.maxy = map->extent.maxy + 2*map->cellsize;
  }
    
  switch(layer->type) {      
  case MS_ANNOTATION:
    if(!annotate) break;

    for(current=layer->features; current; current=current->next) {

      c = current->shape.classindex; // features *must* be pre-classified
 
#ifdef USE_PROJ
      if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
	msProjectPolyline(layer->projection.proj, map->projection.proj, &(current->shape));
#endif

      for(j=0; j<current->shape.numlines;j++) {
	for(i=0; i<current->shape.line[j].numpoints;i++) {

	  pnt = &(current->shape.line[j].point[i]);
	  
	  if(layer->transform) {
	    if(!msPointInRect(pnt, &map->extent)) continue;
	    pnt->x = MS_NINT((pnt->x - map->extent.minx)/map->cellsize); 
	    pnt->y = MS_NINT((map->extent.maxy - pnt->y)/map->cellsize);
	  }
	  
	  if(current->shape.text) text = current->shape.text; 
	  else text = layer->class[c].text.string;
	    
	  if(layer->labelcache)
	    msAddLabel(map, layer->index, c, -1, -1, *pnt, text, -1);
	  else {
	    if(layer->class[c].color == -1) {
	      msDrawMarkerSymbol(&map->symbolset, img, pnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	      if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, pnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	    }
	    msDrawLabel(img, map, *pnt, text, &layer->class[c].label);
	  }
	}    
      }

      pnt = NULL;
    }
    break;

  case MS_POINT:
    for(current=layer->features; current; current=current->next) {

      c = current->shape.classindex; // features *must* be pre-classified

#ifdef USE_PROJ
      if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
	msProjectPolyline(layer->projection.proj, map->projection.proj, &(current->shape));
#endif

      for(j=0; j<current->shape.numlines;j++) {
	for(i=0; i<current->shape.line[j].numpoints;i++) {
	  
	  pnt = &(current->shape.line[j].point[i]);

	  if(layer->transform) {
	    if(!msPointInRect(pnt, &map->extent)) continue;
	    pnt->x = MS_NINT((pnt->x - map->extent.minx)/map->cellsize); 
	    pnt->y = MS_NINT((map->extent.maxy - pnt->y)/map->cellsize);
	  }

	  msDrawMarkerSymbol(&map->symbolset, img, pnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	  if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, pnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	  
	  if(annotate) {
	    if(current->shape.text) text = current->shape.text;
	    else if(layer->class[c].text.string) text = layer->class[c].text.string;
	    else continue;

	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, -1, -1, *pnt, text, -1);
	    else
	      msDrawLabel(img, map, *pnt, text, &layer->class[c].label);
	  }
	}    
      }
    }
    break;      
   
  case MS_LINE:
    for(current=layer->features; current; current=current->next) {

      c = current->shape.classindex; // features *must* be pre-classified

#ifdef USE_PROJ
      if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
	msProjectPolyline(layer->projection.proj, map->projection.proj, &(current->shape));
#endif
     
      if(layer->transform) {
	msClipPolylineRect(&current->shape, map->extent, &current->shape);
	if(current->shape.numlines == 0) continue;
	msTransformPolygon(map->extent, map->cellsize, &current->shape);
      }
      msDrawLineSymbol(&map->symbolset, img, &current->shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
      
      if(annotate) {
	if(current->shape.text) text = current->shape.text; 
	else if(layer->class[c].text.string) text = layer->class[c].text.string;
	else continue;	

	if(msPolylineLabelPoint(&current->shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) != -1) {
	  if(layer->class[c].label.autoangle)
	    layer->class[c].label.angle = angle;

	  if(layer->labelcache)
	    msAddLabel(map, layer->index, c, -1, -1, annopnt, text, length);
	  else
	    msDrawLabel(img, map, annopnt, text, &layer->class[c].label);
	}
      }
    }   

    if(overlay) {
      for(current=layer->features; current; current=current->next) {
	c = current->shape.classindex;
	if(layer->class[c].overlaysymbol >= 0)
	  msDrawLineSymbol(&map->symbolset, img, &current->shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
      }
    }

    break;

  case MS_POLYLINE:
    for(current=layer->features; current; current=current->next) {

      c = current->shape.classindex; // features *must* be pre-classified

#ifdef USE_PROJ
      if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
	msProjectPolyline(layer->projection.proj, map->projection.proj, &(current->shape));
#endif
      
      if(layer->transform) {
	msClipPolygonRect(&current->shape, cliprect, &current->shape);
	if(current->shape.numlines == 0) continue;
	msTransformPolygon(map->extent, map->cellsize, &current->shape);
      }

      msDrawLineSymbol(&map->symbolset, img, &current->shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);

      if(annotate) {
	if(current->shape.text) text = current->shape.text; 
	else if(layer->class[c].text.string) text = layer->class[c].text.string;
	else continue;

	if(msPolygonLabelPoint(&current->shape, &annopnt, layer->class[c].label.minfeaturesize) != -1) {
	  if(layer->labelcache)
	    msAddLabel(map, layer->index, c, -1, -1, annopnt, text, -1);
	  else
	    msDrawLabel(img, map, annopnt, text, &layer->class[c].label);
	}
      }
    }

    if(overlay) {
      for(current=layer->features; current; current=current->next) {
	c = current->shape.classindex;
	if(layer->class[c].overlaysymbol >= 0)
	  msDrawLineSymbol(&map->symbolset, img, &current->shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
      }
    }

    break;
  case MS_POLYGON:
    for(current=layer->features; current; current=current->next) {
    
      c = current->shape.classindex; // features *must* be pre-classified

#ifdef USE_PROJ
      if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
	msProjectPolyline(layer->projection.proj, map->projection.proj, &(current->shape));
#endif
      
      if(layer->transform) {
	msClipPolygonRect(&current->shape, cliprect, &current->shape);
	if(current->shape.numlines == 0) continue;
	msTransformPolygon(map->extent, map->cellsize, &current->shape);
      }

      msDrawShadeSymbol(&map->symbolset, img, &current->shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
      if(layer->class[c].overlaysymbol >= 0) msDrawShadeSymbol(&map->symbolset, img, &current->shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
      
      if(annotate) {
	if(current->shape.text) text = current->shape.text; 
	else if(layer->class[c].text.string) text = layer->class[c].text.string;
	else continue;

	if(msPolygonLabelPoint(&current->shape, &annopnt, layer->class[c].label.minfeaturesize) != -1) {
	  if(layer->labelcache)
	    msAddLabel(map, layer->index, c, -1, -1, annopnt, text, -1);
	  else
	    msDrawLabel(img, map, annopnt, text, &layer->class[c].label);
	}
      }
    }    
    break;      
  default:
    msSetError(MS_MISCERR, "Unknown layer type.", "msDrawInlineLayer()");
    return(-1);
  }

  return(1); /* all done, no cleanup */
}

/*
** Function to draw features defined within a shape file. Can apply an external
** shape status array which must contain the same number of elements as the shapefile
** being drawn. The query_status array is not valid for tiled data.
*/
int msDrawShapefileLayer(mapObj *map, layerObj *layer, gdImagePtr img, char *query_status)
{
  shapefileObj shpfile;
  int i,j,k; /* counters */
  rectObj cliprect;
  char *status=NULL;

  char *filename=NULL;

  int c=-1; /* what class is a particular feature */

  int start_feature;
  int f; /* feature counter */

  shapeObj shape={0,NULL,{-1,-1,-1,-1},MS_NULL};
  pointObj *pnt;

  short annotate=MS_TRUE;
  pointObj annopnt;
  char *annotxt=NULL;

  int classItemIndex, labelItemIndex, labelAngleItemIndex, labelSizeItemIndex;
  
  int tileItemIndex=-1;
  int t;
  shapefileObj tilefile;
  char tilename[MS_PATH_LENGTH];
  int nTiles=1; /* always at least one tile */
  char *tileStatus=NULL;

  double angle, length; /* line labeling parameters */

  double scalefactor=1;

  if(!layer->data && !layer->tileindex)
    return(0);

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT))
    return(0);

  if(map->scale > 0) {
    if((layer->maxscale > 0) && (map->scale > layer->maxscale))
      return(0);
    if((layer->minscale > 0) && (map->scale <= layer->minscale))
      return(0);
    if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
      annotate = MS_FALSE;
    if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
      annotate = MS_FALSE;
  }  

  // apply scaling to symbols and fonts
  if(layer->symbolscale > 0) {
    scalefactor = layer->symbolscale/map->scale;
    for(i=0; i<layer->numclasses; i++) {
      layer->class[i].sizescaled = MS_NINT(layer->class[i].size * scalefactor);
      layer->class[i].sizescaled = MS_MAX(layer->class[i].sizescaled, layer->class[i].minsize);
      layer->class[i].sizescaled = MS_MIN(layer->class[i].sizescaled, layer->class[i].maxsize);
      layer->class[i].overlaysizescaled = MS_NINT(layer->class[i].overlaysize * scalefactor);
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
  }

  cliprect.minx = map->extent.minx - 2*map->cellsize; /* set clipping rectangle just a bit larger than the map extent */
  cliprect.miny = map->extent.miny - 2*map->cellsize;
  cliprect.maxx = map->extent.maxx + 2*map->cellsize;
  cliprect.maxy = map->extent.maxy + 2*map->cellsize;

  classItemIndex = labelItemIndex = labelAngleItemIndex = labelSizeItemIndex = -1;
   
  if(layer->tileindex) { /* we have in index file */

    if(msOpenSHPFile(&tilefile, map->shapepath, map->tile, layer->tileindex) == -1) return(-1);
    if((tileItemIndex = msGetItemIndex(tilefile.hDBF, layer->tileitem)) == -1) return(-1);
    
#ifdef USE_PROJ 
    tileStatus = msWhichShapesProj(&tilefile, map->extent, &(layer->projection), &(map->projection)); /* Which tiles should be processed? */
#else	    
    tileStatus = msWhichShapes(&tilefile, map->extent); /* which tiles should be processed? */
#endif
    
    nTiles = tilefile.numshapes;
  }

  for(t=0;t<nTiles;t++) { /* for each tile, there is always at least 1 tile */
    
    if(layer->tileindex) {
      if(msGetBit(tileStatus,t) == 0)
	continue; /* on to next tile */
      if(!layer->data) /* assume whole filename is in attribute field */
	filename = DBFReadStringAttribute(tilefile.hDBF, t, tileItemIndex);
      else {  
	sprintf(tilename,"%s/%s", DBFReadStringAttribute(tilefile.hDBF, t, tileItemIndex) , layer->data);
	filename = tilename;
      }
    } else {
      filename = layer->data;
    }

    if(strlen(filename) == 0) continue;

#ifndef IGNORE_MISSING_DATA
    if(msOpenSHPFile(&shpfile, map->shapepath, map->tile, filename) == -1) 
      return(-1);
#else
    if(msOpenSHPFile(&shpfile, map->shapepath, map->tile, filename) == -1) 
      continue; // skip it, next tile
#endif

    /* Find item numbers of any columns to be used */
    if(layer->classitem) {
      if((classItemIndex = msGetItemIndex(shpfile.hDBF, layer->classitem)) == -1)
	return(-1);
    }

    if(layer->labelitem && annotate) {
      if((labelItemIndex = msGetItemIndex(shpfile.hDBF, layer->labelitem)) == -1)
	return(-1);	
      labelAngleItemIndex = msGetItemIndex(shpfile.hDBF, layer->labelangleitem); /* not required */
      labelSizeItemIndex = msGetItemIndex(shpfile.hDBF, layer->labelsizeitem);
    }

    if(layer->transform == MS_TRUE) {
#ifdef USE_PROJ
      if((status = msWhichShapesProj(&shpfile, map->extent, &(layer->projection), &(map->projection))) == NULL) return(-1);
#else
      if((status = msWhichShapes(&shpfile, map->extent)) == NULL) return(-1);
#endif
    } else {
      status = msAllocBitArray(shpfile.numshapes);
      if(!status) {
	msSetError(MS_MEMERR, NULL, "msDrawShapefileLayer()");	
	msCloseSHPFile(&shpfile);
	return(-1);
      }
      for(i=0; i<shpfile.numshapes; i++)
	msSetBit(status,i,1); /* all shapes are in by default */
    }

    if(query_status && !(layer->tileindex)) { /* apply query_status array */
      for(i=0; i<shpfile.numshapes; i++)
	if(!msGetBit(query_status, i) || !msGetBit(status, i)) // if 1 in both then it stays 1, else 0
	  msSetBit(status,i,0);
    }
    
    start_feature = 0;
    if(layer->maxfeatures != -1) { /* user only wants maxFeatures, this only makes sense with sorted shapefiles */
      f = 0;
      for(i=shpfile.numshapes-1; i>=0; i--) {
	if(f > layer->maxfeatures)
	  break; /* at the quota, may loose a few to clipping, but that's ok */
	start_feature = i;
	f += msGetBit(status,i);
      }
    }
   
    switch(layer->type) {
    case MS_ANNOTATION:
      
      if(!annotate) break;
      
      switch(shpfile.type) {
      case MS_SHP_POINT:
      case MS_SHP_MULTIPOINT:

	for(i=start_feature;i<shpfile.numshapes;i++) {
	  
	  if(!msGetBit(status,i)) continue; /* next shape */

	  if((c = shpGetClassIndex(shpfile.hDBF, layer, i, classItemIndex)) == -1) continue; /* next shape */
	  
#ifdef USE_PROJ
	  SHPReadShapeProj(shpfile.hSHP, i, &shape, &(layer->projection), &(map->projection));	    
#else
	  SHPReadShape(shpfile.hSHP, i, &shape);	
#endif	  
	  
	  annotxt = shpGetAnnotation(shpfile.hDBF, &(layer->class[c]), i, labelItemIndex);
	  if(!annotxt) continue;	    

	  for(j=0; j<shape.line[0].numpoints; j++) {
	    pnt = &(shape.line[0].point[j]); /* point to the correct point */	      
	    
	    if(layer->transform) {
	      if(!msPointInRect(pnt, &map->extent)) continue;
	      pnt->x = MS_NINT((pnt->x - map->extent.minx)/map->cellsize); 
	      pnt->y = MS_NINT((map->extent.maxy - pnt->y)/map->cellsize);
	    }
	    
	    if(labelAngleItemIndex != -1)
	      layer->class[c].label.angle = DBFReadDoubleAttribute(shpfile.hDBF, i, labelAngleItemIndex)*MS_DEG_TO_RAD;
	    
	    if((labelSizeItemIndex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	      layer->class[c].label.sizescaled = DBFReadIntegerAttribute(shpfile.hDBF, i, labelSizeItemIndex)*scalefactor;
	      layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	      layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	    }
	    
	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, t, i, *pnt, annotxt, -1);
	    else {
	      if(layer->class[c].color != -1) {
		msDrawMarkerSymbol(&map->symbolset, img, pnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
		if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, pnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	      }
	      msDrawLabel(img, map, *pnt, annotxt, &(layer->class[c].label));
	    }
	  }

	  pnt = NULL;
	  msFreeShape(&shape);
	  free(annotxt);
	}
	break;
      case MS_SHP_ARC:
	
	for(i=start_feature;i<shpfile.numshapes;i++) {
	  
	  if(!msGetBit(status,i)) continue; /* next shape */	    
	  
	  if((c = shpGetClassIndex(shpfile.hDBF, layer, i, classItemIndex)) == -1) continue; /* next shape */
	  
#ifdef USE_PROJ
	  SHPReadShapeProj(shpfile.hSHP, i, &shape, &(layer->projection), &(map->projection));	    
#else	    
	  SHPReadShape(shpfile.hSHP, i, &shape);
#endif
	  
	  if(layer->transform) {
	    if(msRectContained(&shape.bounds, &map->extent) == MS_FALSE) {
	      if(msRectOverlap(&shape.bounds, &map->extent) == MS_FALSE) continue;
	      msClipPolylineRect(&shape, cliprect, &shape);
	      if(shape.numlines == 0) continue;
	    }
	    msTransformPolygon(map->extent, map->cellsize, &shape);	      
	  }

	  if(msPolylineLabelPoint(&shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) != -1) {
	    
	    if(labelAngleItemIndex != -1)
	      layer->class[c].label.angle = atof(DBFReadStringAttribute(shpfile.hDBF, i, labelAngleItemIndex))*MS_DEG_TO_RAD;
	    
	    if((labelSizeItemIndex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	      layer->class[c].label.sizescaled = atoi(DBFReadStringAttribute(shpfile.hDBF, i, labelSizeItemIndex))*scalefactor;
	      layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	      layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	    }

	    if(layer->class[c].label.autoangle)
	      layer->class[c].label.angle = angle;

	    annotxt = shpGetAnnotation(shpfile.hDBF, &(layer->class[c]), i, labelItemIndex);
	    if(!annotxt) continue;

	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, t, i, annopnt, annotxt, length);
	    else {
	      if(layer->class[c].color != -1) {
		msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
		if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	      }
	      msDrawLabel(img, map, annopnt, annotxt, &(layer->class[c].label));	    
	    }

	    free(annotxt);
	  }
	  
	  msFreeShape(&shape);
	}
	break;
	
      case MS_SHP_POLYGON:
	
	for(i=start_feature;i<shpfile.numshapes;i++) {
	  
	  if(!msGetBit(status,i)) continue; /* next shape */	
	  
	  if((c = shpGetClassIndex(shpfile.hDBF, layer, i, classItemIndex)) == -1) continue; /* next shape */
	  
#ifdef USE_PROJ	  
	  SHPReadShapeProj(shpfile.hSHP, i, &shape, &(layer->projection), &(map->projection));	    
#else
	  SHPReadShape(shpfile.hSHP, i, &shape);
#endif

	  if(layer->transform) {
	    if(msRectContained(&shape.bounds, &map->extent) == MS_FALSE) {
	      if(msRectOverlap(&shape.bounds, &map->extent) == MS_FALSE) continue;
	      msClipPolygonRect(&shape, cliprect, &shape);
	      if(shape.numlines == 0) continue;
	    }	      
	    msTransformPolygon(map->extent, map->cellsize, &shape);	      
	  }

	  if(msPolygonLabelPoint(&shape, &annopnt, layer->class[c].label.minfeaturesize) != -1) {
	    if(labelAngleItemIndex != -1)
	      layer->class[c].label.angle = atof(DBFReadStringAttribute(shpfile.hDBF, i, labelAngleItemIndex))*MS_DEG_TO_RAD;
	    
	    if((labelSizeItemIndex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	      layer->class[c].label.sizescaled = atoi(DBFReadStringAttribute(shpfile.hDBF, i, labelSizeItemIndex))*scalefactor;
	      layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	      layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	    }

	    annotxt = shpGetAnnotation(shpfile.hDBF, &(layer->class[c]), i, labelItemIndex);
	    if(!annotxt) continue;

	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, t, i, annopnt, annotxt, -1);
	    else {
	      if(layer->class[c].color != -1) {
		msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
		if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, &annopnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	      }
	      msDrawLabel(img, map, annopnt, annotxt, &(layer->class[c].label));	    
	    }

	    free(annotxt);
	  }
	}
	break;
	
      default:
	break;
      }
      
      break;
      
    case MS_POINT:
      
      for(i=start_feature;i<shpfile.numshapes;i++) {
	
	if(!msGetBit(status,i)) continue; /* next shape */
	
	if((c = shpGetClassIndex(shpfile.hDBF, layer, i, classItemIndex)) == -1) continue; /* next shape */

#ifdef USE_PROJ
        SHPReadShapeProj(shpfile.hSHP, i, &shape, &(layer->projection), &(map->projection));	    
#else
	SHPReadShape(shpfile.hSHP, i, &shape);	    
#endif

	for(j=0; j<shape.numlines;j++) {
	  for(k=0; k<shape.line[j].numpoints;k++) {

	    pnt = &(shape.line[j].point[k]); /* point to the correct point */

	    if(layer->transform) {
	      if(!msPointInRect(pnt, &map->extent)) continue;
	      pnt->x = MS_NINT((pnt->x - map->extent.minx)/map->cellsize); 
	      pnt->y = MS_NINT((map->extent.maxy - pnt->y)/map->cellsize);	      
	    } 

	    msDrawMarkerSymbol(&map->symbolset, img, pnt, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	    if(layer->class[c].overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, pnt, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);

	    if(annotate && (annotxt = shpGetAnnotation(shpfile.hDBF, &(layer->class[c]), i, labelItemIndex))) {
	      
	      if(labelAngleItemIndex != -1)
		layer->class[c].label.angle = DBFReadDoubleAttribute(shpfile.hDBF, i, labelAngleItemIndex)*MS_DEG_TO_RAD;
	      
	      if((labelSizeItemIndex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
		layer->class[c].label.sizescaled = DBFReadIntegerAttribute(shpfile.hDBF, i, labelSizeItemIndex)*scalefactor;
		layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
		layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	      }

	      if(layer->labelcache)
		msAddLabel(map, layer->index, c, t, i, *pnt, annotxt, -1);
	      else
		msDrawLabel(img, map, *pnt, annotxt, &layer->class[c].label);
	    }
	  }
	}
	
	pnt = NULL;
	msFreeShape(&shape);
	if(annotate)
	  free(annotxt);
      }	
      
      break;
            
    case MS_LINE:

      if((shpfile.type != MS_SHP_ARC) && (shpfile.type != MS_SHP_POLYGON)) { /* wrong shapefile type */
	msSetError(MS_MISCERR, "LINE layers must be ARC or POLYGON shapefiles.", "msDrawShapefileLayer()");
	return(-1);
      }
      
      for(i=start_feature;i<shpfile.numshapes;i++) {

	if(!msGetBit(status,i)) continue; /* next shape */

        if((c = shpGetClassIndex(shpfile.hDBF, layer, i, classItemIndex)) == -1) continue; /* next shape */

#ifdef USE_PROJ
	SHPReadShapeProj(shpfile.hSHP, i, &shape, &(layer->projection), &(map->projection));	    
#else
	SHPReadShape(shpfile.hSHP, i, &shape);
#endif

	if(layer->transform) {
	  if(!msRectContained(&shape.bounds, &map->extent)) {
	    if(msRectOverlap(&shape.bounds, &map->extent) == MS_FALSE) continue;
	    msClipPolylineRect(&shape, cliprect, &shape);
	    if(shape.numlines == 0) continue;
	  }
	  msTransformPolygon(map->extent, map->cellsize, &shape);
	}

	msDrawLineSymbol(&map->symbolset, img, &shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
		
	if(annotate && (annotxt = shpGetAnnotation(shpfile.hDBF, &(layer->class[c]), i, labelItemIndex))) {
	  if(msPolylineLabelPoint(&shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) != -1) {

	    if(layer->class[c].label.autoangle)
	      layer->class[c].label.angle = angle;

	    if(labelAngleItemIndex != -1)
	      layer->class[c].label.angle = atof(DBFReadStringAttribute(shpfile.hDBF, i, labelAngleItemIndex))*MS_DEG_TO_RAD;
	    
	    if((labelSizeItemIndex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	      layer->class[c].label.sizescaled = atoi(DBFReadStringAttribute(shpfile.hDBF, i, labelSizeItemIndex))*scalefactor;
	      layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	      layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	    }
	    
	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, t, i, annopnt, annotxt, length);
	    else
	      msDrawLabel(img, map, annopnt, annotxt, &(layer->class[c].label));

	    free(annotxt);
	  }
	}
	
	msFreeShape(&shape);
      }
      
      break;

    case MS_POLYLINE:

      if(shpfile.type != MS_SHP_POLYGON) { /* wrong shapefile type */
	msSetError(MS_MISCERR, "POLYGON layers must be POLYGON shapefiles.", "msDrawShapefileLayer()");
	return(-1);
      }
      
      for(i=start_feature;i<shpfile.numshapes;i++) {
	
	if(!msGetBit(status,i)) continue; /* next shape */
	
	if((c = shpGetClassIndex(shpfile.hDBF, layer, i, classItemIndex)) == -1) continue; /* next shape */

#ifdef USE_PROJ
	SHPReadShapeProj(shpfile.hSHP, i, &shape, &(layer->projection), &(map->projection));
#else
	SHPReadShape(shpfile.hSHP, i, &shape);
#endif
	
	if(layer->transform) {
	  if(!msRectContained(&shape.bounds, &cliprect)) {
	    if(msRectOverlap(&shape.bounds, &cliprect) == MS_FALSE) continue;
	    msClipPolygonRect(&shape, cliprect, &shape);
	    if(shape.numlines == 0) continue;
	  }
	  msTransformPolygon(map->extent, map->cellsize, &shape);
	}

	msDrawLineSymbol(&map->symbolset, img, &shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);

	if(annotate  && (annotxt = shpGetAnnotation(shpfile.hDBF, &(layer->class[c]), i, labelItemIndex))) {
	  if(msPolygonLabelPoint(&shape, &annopnt, layer->class[c].label.minfeaturesize) != -1) {
	    if(labelAngleItemIndex != -1)
	      layer->class[c].label.angle = atof(DBFReadStringAttribute(shpfile.hDBF, i, labelAngleItemIndex))*MS_DEG_TO_RAD;
	    
	    if((labelSizeItemIndex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	      layer->class[c].label.sizescaled = atoi(DBFReadStringAttribute(shpfile.hDBF, i, labelSizeItemIndex))*scalefactor;
	      layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	      layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	    }
	    
	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, t, i, annopnt, annotxt, -1);
	    else
	      msDrawLabel(img, map, annopnt, annotxt, &(layer->class[c].label));
	    
	    free(annotxt);
	  }
	}
	
	msFreeShape(&shape);
      }	
      
      break;

    case MS_POLYGON:
      
      if(shpfile.type != MS_SHP_POLYGON) { /* wrong shapefile type */
	msSetError(MS_MISCERR, "POLYGON layers must be POLYGON shapefiles.", "msDrawShapefileLayer()");
	return(-1);
      }
      
      for(i=start_feature;i<shpfile.numshapes;i++) {
	
	if(!msGetBit(status,i)) continue; /* next shape */
	
	if((c = shpGetClassIndex(shpfile.hDBF, layer, i, classItemIndex)) == -1) continue; /* next shape */

#ifdef USE_PROJ
	SHPReadShapeProj(shpfile.hSHP, i, &shape, &(layer->projection), &(map->projection));
#else
	SHPReadShape(shpfile.hSHP, i, &shape);
#endif
	
	if(layer->transform) {
	  if(!msRectContained(&shape.bounds, &cliprect)) {
	    if(msRectOverlap(&shape.bounds, &cliprect) == MS_FALSE) continue;
	    msClipPolygonRect(&shape, cliprect, &shape);
	    if(shape.numlines == 0) continue;
	  }
	  msTransformPolygon(map->extent, map->cellsize, &shape);
	}

	msDrawShadeSymbol(&map->symbolset, img, &shape, layer->class[c].symbol, layer->class[c].color, layer->class[c].backgroundcolor, layer->class[c].outlinecolor, layer->class[c].sizescaled);
	if(layer->class[c].overlaysymbol >= 0) 
	  msDrawShadeSymbol(&map->symbolset, img, &shape, layer->class[c].overlaysymbol, layer->class[c].overlaycolor, layer->class[c].overlaybackgroundcolor, layer->class[c].overlayoutlinecolor, layer->class[c].overlaysizescaled);
	
	if(annotate  && (annotxt = shpGetAnnotation(shpfile.hDBF, &(layer->class[c]), i, labelItemIndex))) {
	  if(msPolygonLabelPoint(&shape, &annopnt, layer->class[c].label.minfeaturesize) != -1) {
	    if(labelAngleItemIndex != -1)
	      layer->class[c].label.angle = atof(DBFReadStringAttribute(shpfile.hDBF, i, labelAngleItemIndex))*MS_DEG_TO_RAD;
	    
	    if((labelSizeItemIndex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
	      layer->class[c].label.sizescaled = atoi(DBFReadStringAttribute(shpfile.hDBF, i, labelSizeItemIndex))*scalefactor;
	      layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
	      layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
	    }
	    
	    if(layer->labelcache)
	      msAddLabel(map, layer->index, c, t, i, annopnt, annotxt, -1);
	    else
	      msDrawLabel(img, map, annopnt, annotxt, &(layer->class[c].label));
	    
	    free(annotxt);
	  }
	}
	
	msFreeShape(&shape);
      }	
      
      break;
      
    default:
      msSetError(MS_MISCERR, "Unknown or unsupported layer type.", "msDrawShapefileLayer()");
      return(-1);
    }		
    
    msCloseSHPFile(&shpfile);
    free(status);
    
  } /* next tile */
  
  if(layer->tileindex) { /* tiling clean-up */
    msCloseSHPFile(&tilefile);
    free(tileStatus);
  }

  return(0);
}

/*
** Save an image to a file named filename, if filename is NULL it goes to stdout
*/
int msSaveImage(gdImagePtr img, char *filename, int transparent, int interlace)
{
  FILE *stream;

  if(filename != NULL && strlen(filename) > 0) {
    stream = fopen(filename, "wb");
    if(!stream) {
      msSetError(MS_IOERR, NULL, "msSaveImage()");      
      sprintf(ms_error.message, "(%s)", filename);
      return(-1);
    }
  } else { /* use stdout */
    
#ifdef _WIN32
    /*
    ** Change stdout mode to binary on win32 platforms
    */
    if(_setmode( _fileno(stdout), _O_BINARY) == -1) {
      msSetError(MS_IOERR, "Unable to change stdout to binary mode.", "msSaveImage()");
      return(-1);
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

  return(0);
}

void msFreeImage(gdImagePtr img)
{
  gdImageDestroy(img);
}

gdImagePtr msDrawReferenceMap(mapObj *map) {
  FILE *stream;
  gdImagePtr img=NULL;
  double cellsize;
  int c=-1, oc=-1;
  int x1,y1,x2,y2;

  /* Allocate input and output images (same size) */
  stream = fopen(map->reference.image,"rb");
  if(!stream) {
    msSetError(MS_IOERR, NULL, "msDrawReferenceMap()");
    sprintf(ms_error.message, "(%s)", map->reference.image);
    return(NULL);
  }

#ifndef USE_GD_1_6 
  img = gdImageCreateFromGif(stream);
#else
  img = gdImageCreateFromPng(stream);
#endif
  if(!img) {
    msSetError(MS_GDERR, "Unable to initialize image.", "msDrawReferenceMap()");
    fclose(stream);
    return(NULL);
  }

  /* Re-map users extent to this image */
  cellsize = msAdjustExtent(&(map->reference.extent), img->sx, img->sy);

  /* allocate some colors */
  if(map->reference.outlinecolor.red != -1 && map->reference.outlinecolor.green != -1 && map->reference.outlinecolor.blue != -1)
    oc = gdImageColorAllocate(img, map->reference.outlinecolor.red, map->reference.outlinecolor.green, map->reference.outlinecolor.blue);
  if(map->reference.color.red != -1 && map->reference.color.green != -1 && map->reference.color.blue != -1)
    c = gdImageColorAllocate(img, map->reference.color.red, map->reference.color.green, map->reference.color.blue); 
  
  /* Remember 0,0 file coordinates equals minx,maxy map coordinates (e.g. UTM) */
  x1 = MS_NINT((map->extent.minx - map->reference.extent.minx)/cellsize);
  x2 = MS_NINT((map->extent.maxx - map->reference.extent.minx)/cellsize);
  y2 = MS_NINT((map->reference.extent.maxy - map->extent.miny)/cellsize);
  y1 = MS_NINT((map->reference.extent.maxy - map->extent.maxy)/cellsize);
  
  /* Add graphic element to the output image file */
  if(c != -1)
    gdImageFilledRectangle(img,x1,y1,x2,y2,c);
  if(oc != -1)
    gdImageRectangle(img,x1,y1,x2,y2,oc);
  
  fclose(stream);
  return(img);
}

/*
** maplabel.c: Routines to enable text drawing, BITMAP or TRUETYPE.
*/
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontt.h>
#include <gdfontmb.h>
#include <gdfontg.h>

#include "map.h"

// FIX: need to be handle the label wrapping

#define LINE_VERT_THRESHOLD .17 // max absolute value of cos of line angle, the closer to zero the more vertical the line must be

int msAddLabel(mapObj *map, int layeridx, int classidx, int tileidx, int shapeidx, pointObj point, char *string, double featuresize)
{
  int i;
  char wrap[2];

  if(!string) return(0); /* not an error */

  if(map->labelcache.numlabels == map->labelcache.cachesize) { /* just add it to the end */
    map->labelcache.labels = (labelCacheMemberObj *) realloc(map->labelcache.labels, sizeof(labelCacheMemberObj)*(map->labelcache.cachesize+MS_LABELCACHEINCREMENT));
    if(!map->labelcache.labels) {
      msSetError(MS_MEMERR, "Realloc() error.", "msAddLabel()");
      return(-1);
    }   
    map->labelcache.cachesize += MS_LABELCACHEINCREMENT;
  }

  i = map->labelcache.numlabels;  
  map->labelcache.labels[i].layeridx = layeridx;
  map->labelcache.labels[i].classidx = classidx;
  map->labelcache.labels[i].tileidx = tileidx;
  map->labelcache.labels[i].shapeidx = shapeidx;

  map->labelcache.labels[i].point = point;
  map->labelcache.labels[i].point.x = MS_NINT(map->labelcache.labels[i].point.x);
  map->labelcache.labels[i].point.y = MS_NINT(map->labelcache.labels[i].point.y);
  
  map->labelcache.labels[i].string = strdup(string);

  // GD/Freetype recognizes \r\n as a true line wrap so we must turn the wrap character into that pattern
  if(map->layers[layeridx].class[classidx].label.type != MS_BITMAP && map->layers[layeridx].class[classidx].label.wrap != '\0') {
    wrap[0] = map->layers[layeridx].class[classidx].label.wrap;
    wrap[1] = '\0';
    map->labelcache.labels[i].string = gsub(map->labelcache.labels[i].string, wrap, "\r\n");
  }

  map->labelcache.labels[i].size = map->layers[layeridx].class[classidx].label.sizescaled;
  map->labelcache.labels[i].angle = map->layers[layeridx].class[classidx].label.angle;
  map->labelcache.labels[i].featuresize = featuresize;

  map->labelcache.labels[i].poly = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(map->labelcache.labels[i].poly);

  map->labelcache.labels[i].status = MS_FALSE;

  if(map->layers[layeridx].type == MS_LAYER_POINT) { 
    rectObj rect;
    int w, h;

    if(map->labelcache.nummarkers == map->labelcache.markercachesize) { /* just add it to the end */
      map->labelcache.markers = (markerCacheMemberObj *) realloc(map->labelcache.markers, sizeof(markerCacheMemberObj)*(map->labelcache.cachesize+MS_LABELCACHEINCREMENT));
      if(!map->labelcache.markers) {
	msSetError(MS_MEMERR, "Realloc() error.", "msAddLabel()");
	return(-1);
      }   
      map->labelcache.markercachesize+=MS_LABELCACHEINCREMENT;
    }

    i = map->labelcache.nummarkers;

    map->labelcache.markers[i].poly = (shapeObj *) malloc(sizeof(shapeObj));
    msInitShape(map->labelcache.markers[i].poly);
   
    msGetMarkerSize(&map->symbolset, &(map->layers[layeridx].class[classidx]), &w, &h);
    rect.minx = MS_NINT(point.x - .5 * w);
    rect.miny = MS_NINT(point.y - .5 * h);
    rect.maxx = rect.minx + (w-1);
    rect.maxy = rect.miny + (h-1);
    msRectToPolygon(rect, map->labelcache.markers[i].poly);
    map->labelcache.markers[i].id = map->labelcache.numlabels;

    map->labelcache.nummarkers++;
  }

  map->labelcache.numlabels++;
  
  return(0);
}


int msLoadFontSet(fontSetObj *fontset) 
{
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  FILE *stream;
  char buffer[MS_BUFFER_LENGTH];
  char alias[64], file1[MS_PATH_LENGTH], file2[MS_PATH_LENGTH];
  char *path;
  int i;

  if(fontset->numfonts != 0) /* already initialized */
    return(0);

  if(!fontset->filename)
    return(0);

  path = getPath(fontset->filename);

  fontset->fonts = msCreateHashTable(); /* create font hash */  
  if(!fontset->fonts) {
    msSetError(MS_HASHERR, "Error initializing font hash.", "msLoadFontSet()");
    return(-1);
  }

  stream = fopen(fontset->filename, "r");
  if(!stream) {
    sprintf(ms_error.message, "Error opening fontset %s.", fontset->filename);
    msSetError(MS_IOERR, ms_error.message, "msLoadFontset()");
    return(-1);
  }
 
  i = 0;
  while(fgets(buffer, MS_BUFFER_LENGTH, stream)) { /* while there's something to load */

    if(buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == ' ')
      continue; /* skip comments and blank lines */

    sscanf(buffer,"%s %s", alias,  file1);

    if(file1[0] == '/') { /* already full path */      
      msInsertHashTable(fontset->fonts, alias, file1);
    } else {
      sprintf(file2, "%s%s", path, file1);      
      msInsertHashTable(fontset->fonts, alias, file2);
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

int msGetLabelSize(char *string, labelObj *label, rectObj *rect, fontSetObj *fontset) /* assumes an angle of 0 */
{
  if(label->type == MS_TRUETYPE) {
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    int bbox[8];
    char *error=NULL, *font=NULL;

    font = msLookupHashTable(fontset->fonts, label->font);
    if(!font) {
      if(label->font) sprintf(ms_error.message, "Requested font (%s) not found.", label->font);
      msSetError(MS_TTFERR, ms_error.message, "msGetLabelSize()");
      return(-1);
    }

#ifdef USE_GD_TTF
    error = gdImageStringTTF(NULL, bbox, 0, font, label->sizescaled, 0, 0, 0, string);
#else
    error = gdImageStringFT(NULL, bbox, 0, font, label->sizescaled, 0, 0, 0, string);
#endif
    if(error) {
      msSetError(MS_TTFERR, error, "msGetLabelSize()");
      return(-1);
    }    
    
    rect->minx = bbox[0];
    rect->miny = bbox[5];
    rect->maxx = bbox[2];
    rect->maxy = bbox[1];
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

    if(label->wrap != '\0') {
      if((token = split(string, label->wrap, &(num_tokens))) == NULL)
	return(0);

      for(t=0; t<num_tokens; t++) /* what's the longest token */
	max_token_length = MS_MAX(max_token_length, strlen(token[t]));
      
      rect->minx = 0;
      rect->miny = -(fontPtr->h * num_tokens);
      rect->maxx = fontPtr->w * max_token_length;
      rect->maxy = 0;

      msFreeCharArray(token, num_tokens);
    } else {
      rect->minx = 0;
      rect->miny = -fontPtr->h;
      rect->maxx = fontPtr->w * strlen(string);
      rect->maxy = 0;
    }
  }
  return(0);
}

gdFontPtr msGetBitmapFont(int size)
{
  switch(size) { /* set the font to use */
  case MS_TINY:
    return(gdFontTiny);
    break;
  case MS_SMALL:  
    return(gdFontSmall);
    break; 
  case MS_MEDIUM:
    return(gdFontMediumBold);
    break; 
  case MS_LARGE:
    return(gdFontLarge);
    break;  
  case MS_GIANT:
    return(gdFontGiant);
    break;
  default:
    msSetError(MS_GDERR,"Invalid bitmap font. Must be one of tiny, small, medium, large or giant." , "msGetBitmapFont()");    
    return(NULL);
  }
}

#define MARKER_SLOP 2

static pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly)
{
  pointObj q;
  double x1=0, y1=0, x2=0, y2=0;
  lineObj line={0,NULL};
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
    line.point = (pointObj *)malloc(sizeof(pointObj)*5);
    line.numpoints = 5;

    x2 = x1 - buffer; /* ll */
    y2 = y1 + buffer;
    line.point[0].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    line.point[0].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);
    
    x2 = x1 - buffer; /* ul */
    y2 = y1 - h - buffer;
    line.point[1].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    line.point[1].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);
    
    x2 = x1 + w + buffer; /* ur */
    y2 = y1 - h - buffer;
    line.point[2].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    line.point[2].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);
    
    x2 = x1 + w + buffer; /* lr */
    y2 = y1 + buffer;
    line.point[3].x = p->x + MS_NINT(x2 * cos_a - (-y2) * sin_a);
    line.point[3].y = p->y - MS_NINT(x2 * sin_a + (-y2) * cos_a);
    
    line.point[4].x = line.point[0].x;
    line.point[4].y = line.point[0].y;
    
    msAddLine(poly, &line);
    free(line.point);
  }

  return(q);
}

/*
** Simply draws a label based on the label point and the supplied label object. 
*/
static int draw_text(gdImagePtr img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset)
{
  int x, y;

  if(!string)
    return(0); /* not an error, just don't want to do anything */

  if(strlen(string) == 0)
    return(0); /* not an error, just don't want to do anything */

  x = MS_NINT(labelPnt.x);
  y = MS_NINT(labelPnt.y);

  if(label->type == MS_TRUETYPE) {
    char *error=NULL, *font=NULL;
    int bbox[8];
    double angle_radians = MS_DEG_TO_RAD*label->angle;

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    if(!fontset) {
      msSetError(MS_TTFERR, "No fontset defined.", "draw_text()");
      return(-1);
    }

    if(!label->font) {
      msSetError(MS_TTFERR, "No TrueType font defined.", "draw_text()");
      return(-1);
    }

    font = msLookupHashTable(fontset->fonts, label->font);
    if(!font) {
      sprintf(ms_error.message, "Requested font (%s) not found.", label->font);
      msSetError(MS_TTFERR, ms_error.message, "draw_text()");
      return(-1);
    }

    if(label->outlinecolor >= 0) { /* handle the outline color */
#ifdef USE_GD_TTF
      error = gdImageStringTTF(img, bbox, label->antialias*label->outlinecolor, font, label->sizescaled, angle_radians, x-1, y-1, string);
#else
      error = gdImageStringFT(img, bbox, label->antialias*label->outlinecolor, font, label->sizescaled, angle_radians, x-1, y-1, string);
#endif
      if(error) {
	msSetError(MS_TTFERR, error, "draw_text()");
	return(-1);
      }

#ifdef USE_GD_TTF
      gdImageStringTTF(img, bbox, label->antialias*label->outlinecolor, font, label->sizescaled, angle_radians, x-1, y+1, string);
      gdImageStringTTF(img, bbox, label->antialias*label->outlinecolor, font, label->sizescaled, angle_radians, x+1, y+1, string);
      gdImageStringTTF(img, bbox, label->antialias*label->outlinecolor, font, label->sizescaled, angle_radians, x+1, y-1, string);
#else
      gdImageStringFT(img, bbox, label->antialias*label->outlinecolor, font, label->sizescaled, angle_radians, x-1, y+1, string);
      gdImageStringFT(img, bbox, label->antialias*label->outlinecolor, font, label->sizescaled, angle_radians, x+1, y+1, string);
      gdImageStringFT(img, bbox, label->antialias*label->outlinecolor, font, label->sizescaled, angle_radians, x+1, y-1, string);
#endif      

    }

    if(label->shadowcolor >= 0) { /* handle the shadow color */
#ifdef USE_GD_TTF
      error = gdImageStringTTF(img, bbox, label->antialias*label->shadowcolor, font, label->sizescaled, angle_radians, x+label->shadowsizex, y+label->shadowsizey, string);
#else
      error = gdImageStringFT(img, bbox, label->antialias*label->shadowcolor, font, label->sizescaled, angle_radians, x+label->shadowsizex, y+label->shadowsizey, string);
#endif
      if(error) {
	msSetError(MS_TTFERR, error, "draw_text()");
	return(-1);
      }
    }

#ifdef USE_GD_TTF
    gdImageStringTTF(img, bbox, label->antialias*label->color, font, label->sizescaled, angle_radians, x, y, string);
#else
    gdImageStringFT(img, bbox, label->antialias*label->color, font, label->sizescaled, angle_radians, x, y, string);
#endif

#else
    msSetError(MS_TTFERR, "TrueType font support is not available.", "draw_text()");
    return(-1);
#endif

  } else { /* MS_BITMAP */
    char **token=NULL;
    int t, num_tokens;
    gdFontPtr fontPtr;
    
    if((fontPtr = msGetBitmapFont(label->size)) == NULL)
      return(-1);

    if(label->wrap != '\0') {
      if((token = split(string, label->wrap, &(num_tokens))) == NULL)
	return(-1);
      
      y -= fontPtr->h*num_tokens;
      for(t=0; t<num_tokens; t++) {
	if(label->outlinecolor >= 0) {
	  gdImageString(img, fontPtr, x-1, y-1, token[t], label->outlinecolor);
	  gdImageString(img, fontPtr, x-1, y+1, token[t], label->outlinecolor);
	  gdImageString(img, fontPtr, x+1, y+1, token[t], label->outlinecolor);
	  gdImageString(img, fontPtr, x+1, y-1, token[t], label->outlinecolor);
	}
	
	if(label->shadowcolor >= 0)
	  gdImageString(img, fontPtr, x+label->shadowsizex, y+label->shadowsizey, token[t], label->shadowcolor);
	
	gdImageString(img, fontPtr, x, y, token[t], label->color);

	y += fontPtr->h; /* shift down */
      }
      
      msFreeCharArray(token, num_tokens);  
    } else {
      y -= fontPtr->h;

      if(label->outlinecolor >= 0) {
	gdImageString(img, fontPtr, x-1, y-1, string, label->outlinecolor);
	gdImageString(img, fontPtr, x-1, y+1, string, label->outlinecolor);
	gdImageString(img, fontPtr, x+1, y+1, string, label->outlinecolor);
	gdImageString(img, fontPtr, x+1, y-1, string, label->outlinecolor);
      }

      if(label->shadowcolor >= 0)
	gdImageString(img, fontPtr, x+label->shadowsizex, y+label->shadowsizey, string, label->shadowcolor);
	
      gdImageString(img, fontPtr, x, y, string, label->color);
    }
  }

  return(0);
}

int msDrawLabel(gdImagePtr img, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset)
{
  if(!string)
    return(0); /* not an error, just don't want to do anything */

  if(strlen(string) == 0)
    return(0); /* not an error, just don't want to do anything */

  if(label->position != MS_XY) {
    pointObj p;
    rectObj r;

    if(msGetLabelSize(string, label, &r, fontset) == -1) return(-1);
    p = get_metrics(&labelPnt, label->position, r, label->offsetx, label->offsety, label->angle, 0, NULL);
    draw_text(img, p, string, label, fontset); /* actually draw the label */
  } else {
    labelPnt.x += label->offsetx;
    labelPnt.y += label->offsety;
    draw_text(img, labelPnt, string, label, fontset); /* actually draw the label */
  }
  
  return(0);
}

/*
** is a label completely in the image (excluding label buffer)
*/ 
static int labelInImage(int width, int height, shapeObj *lpoly, int buffer)
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

static double dist(pointObj a, pointObj b)
{
  double d;

  d = sqrt((pow((a.x-b.x),2) + pow((a.y-b.y),2)));
  return(d);
}

/* 
** Variation on msIntersectPolygons. Label polygons aren't like shapefile polygons. They
** have no holes, and often do have overlapping parts (i.e. road symbols).
*/
extern int intersectLines(pointObj a, pointObj b, pointObj c, pointObj d); // from mapsearch.c

static int intersectLabelPolygons(shapeObj *p1, shapeObj *p2) {
  int c1,v1,c2,v2;
  pointObj *point;

  /* STEP 1: look for intersecting line segments */
  for(c1=0; c1<p1->numlines; c1++)
    for(v1=1; v1<p1->line[c1].numpoints; v1++)
      for(c2=0; c2<p2->numlines; c2++)
	for(v2=1; v2<p2->line[c2].numpoints; v2++)
	  if(intersectLines(p1->line[c1].point[v1-1], p1->line[c1].point[v1], p2->line[c2].point[v2-1], p2->line[c2].point[v2]) ==  MS_TRUE)	    
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

void billboard(gdImagePtr img, shapeObj *shape, labelObj *label) 
{
  int i;
  shapeObj temp;

  msInitShape(&temp);
  msAddLine(&temp, &shape->line[0]);

  if(label->backgroundshadowcolor >= 0) {
    for(i=0; i<temp.line[0].numpoints; i++) {
      temp.line[0].point[i].x += label->backgroundshadowsizex;
      temp.line[0].point[i].y += label->backgroundshadowsizey;
    }
    msImageFilledPolygon(img, &temp, label->backgroundshadowcolor);
    for(i=0; i<temp.line[0].numpoints; i++) {
      temp.line[0].point[i].x -= label->backgroundshadowsizex;
      temp.line[0].point[i].y -= label->backgroundshadowsizey;
    }
  }

  msImageFilledPolygon(img, &temp, label->backgroundcolor);
  
  msFreeShape(&temp);
}
 
int msDrawLabelCache(gdImagePtr img, mapObj *map)
{
  pointObj p;
  int i, j, l;

  rectObj r;

  labelObj label;
  labelCacheMemberObj *cachePtr=NULL;
  classObj *classPtr=NULL;
  layerObj *layerPtr=NULL;

  int draw_marker;
  int marker_width, marker_height;
  int marker_offset_x, marker_offset_y;
  rectObj marker_rect;

  for(l=map->labelcache.numlabels-1; l>=0; l--) {

    cachePtr = &(map->labelcache.labels[l]); /* point to right spot in cache */

    layerPtr = &(map->layers[cachePtr->layeridx]); /* set a couple of other pointers, avoids nasty references */
    classPtr = &(layerPtr->class[cachePtr->classidx]);

    if(!cachePtr->string)
      continue; /* not an error, just don't want to do anything */
    
    if(strlen(cachePtr->string) == 0)
      continue; /* not an error, just don't want to do anything */

    label = classPtr->label;
    label.sizescaled = label.size = cachePtr->size;
    label.angle = cachePtr->angle;

    if(msGetLabelSize(cachePtr->string, &label, &r, &(map->fontset)) == -1)
      return(-1);

    if(label.autominfeaturesize && ((r.maxx-r.minx) > cachePtr->featuresize))
      continue; /* label too large relative to the feature */

    draw_marker = marker_offset_x = marker_offset_y = 0; /* assume no marker */
    if((layerPtr->type == MS_LAYER_ANNOTATION || layerPtr->type == MS_LAYER_POINT) && (classPtr->color >= 0 || classPtr->outlinecolor > 0)) { /* there *is* a marker */

      msGetMarkerSize(&map->symbolset, classPtr, &marker_width, &marker_height);
      marker_offset_x = MS_NINT(marker_width/2.0);
      marker_offset_y = MS_NINT(marker_height/2.0);

      marker_rect.minx = MS_NINT(cachePtr->point.x - .5 * marker_width);
      marker_rect.miny = MS_NINT(cachePtr->point.y - .5 * marker_height);
      marker_rect.maxx = marker_rect.minx + (marker_width-1);
      marker_rect.maxy = marker_rect.miny + (marker_height-1);

      if(layerPtr->type == MS_LAYER_ANNOTATION) draw_marker = 1; /* actually draw the marker */
    }
    
    if(label.position == MS_AUTO) {

      if(layerPtr->type == MS_LAYER_LINE) {
	int position = MS_UC;

	for(j=0; j<2; j++) { /* Two angles or two positions, depending on angle. Steep angles will use the angle approach, otherwise we'll rotate between UC and LC. */

	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

	  if(j == 1) {	   
	    if(fabs(cos(label.angle)) < LINE_VERT_THRESHOLD)	      
	      label.angle += 180.0;
	    else
	      position = MS_LC;
	  }

	  p = get_metrics(&(cachePtr->point), position, r, (marker_offset_x + label.offsetx), (marker_offset_y + label.offsety), label.angle, label.buffer, cachePtr->poly);
	  
	  if(draw_marker) 
	    msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon
	  
	  if(!label.partials) { // check against image first
	    if(labelInImage(img->sx, img->sy, cachePtr->poly, label.buffer) == MS_FALSE) {
	      cachePtr->status = MS_FALSE;
	      continue; // next angle
	    }
	  }
	  
	  for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	    if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	      if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }
	  
	  if(!cachePtr->status)
	    continue; // next angle
	  
	  for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
	    if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */
	      
	      if((label.mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= label.mindistance)) { /* label is a duplicate */
		cachePtr->status = MS_FALSE;
		break;
	      }
	      
	      if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }	
	  
	  if(cachePtr->status) // found a suitable place for this label
	    break;

	} // next angle

      } else {
	for(j=0; j<=7; j++) { /* loop through the outer label positions */	  
	  
	  msFreeShape(cachePtr->poly);
	  cachePtr->status = MS_TRUE; /* assume label *can* be drawn */
	  
	  p = get_metrics(&(cachePtr->point), j, r, (marker_offset_x + label.offsetx), (marker_offset_y + label.offsety), label.angle, label.buffer, cachePtr->poly);
	  
	  if(draw_marker) 
	    msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon
	  
	  if(!label.partials) { // check against image first
	    if(labelInImage(img->sx, img->sy, cachePtr->poly, label.buffer) == MS_FALSE) {
	      cachePtr->status = MS_FALSE;
	      continue; // next position
	    }
	  }
	  
	  for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	    if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	      if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }
	  
	  if(!cachePtr->status)
	    continue; // next position
	  
	  for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
	    if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */
	      
	      if((label.mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= label.mindistance)) { /* label is a duplicate */
		cachePtr->status = MS_FALSE;
		break;
	      }
	      
	      if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
		cachePtr->status = MS_FALSE;
		break;
	      }
	    }
	  }	
	  
	  if(cachePtr->status) // found a suitable place for this label
	    break;
	} // next position      
      }

      if(label.force) cachePtr->status = MS_TRUE; /* draw in spite of collisions based on last position, need a *best* position */

    } else {

      cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

      if(label.position == MS_CC) // don't need the marker_offset
        p = get_metrics(&(cachePtr->point), label.position, r, label.offsetx, label.offsety, label.angle, label.buffer, cachePtr->poly);
      else
        p = get_metrics(&(cachePtr->point), label.position, r, (marker_offset_x + label.offsetx), (marker_offset_y + label.offsety), label.angle, label.buffer, cachePtr->poly);

      if(draw_marker)
	msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon, part of overlap tests */

      if(!label.force) { // no need to check anything else
      
	if(!label.partials) {
	  if(labelInImage(img->sx, img->sy, cachePtr->poly, label.buffer) == MS_FALSE)
	    cachePtr->status = MS_FALSE;
	}

	if(!cachePtr->status)
	  continue; // next label

	for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
	  if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
	    if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
	      cachePtr->status = MS_FALSE;
	      break;
	    }
	  }
	}
	
	if(!cachePtr->status)
	  continue; // next label
	
	for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered label
	  if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */
	    if((label.mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string, map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= label.mindistance)) { /* label is a duplicate */
	      cachePtr->status = MS_FALSE;
	      break;
	    }
	    
	    if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
	      cachePtr->status = MS_FALSE;
	      break;
	    }
	  }
	}
      }
    } /* end position if-then-else */

    /* msImagePolyline(img, cachePtr->poly, 1); */

    if(!cachePtr->status)
      continue; /* next label */

    if(draw_marker) { /* need to draw a marker */
      msDrawMarkerSymbol(&map->symbolset, img, &(cachePtr->point), classPtr->symbol, classPtr->color, classPtr->backgroundcolor, classPtr->outlinecolor, classPtr->sizescaled);
      if(classPtr->overlaysymbol >= 0) msDrawMarkerSymbol(&map->symbolset, img, &(cachePtr->point), classPtr->overlaysymbol, classPtr->overlaycolor, classPtr->overlaybackgroundcolor, classPtr->overlayoutlinecolor, classPtr->overlaysizescaled);
    }

    if(label.backgroundcolor >= 0)
      billboard(img, cachePtr->poly, &label);

    draw_text(img, p, cachePtr->string, &label, &(map->fontset)); /* actually draw the label */

  } /* next in cache */

  return(0);
}

int msImageTruetypeArrow(gdImagePtr img, shapeObj *p, symbolObj *s, int color, int size, fontSetObj *fontset)
{
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msImageTruetypeArrow()");
  return(-1);
#endif
}

int msImageTruetypePolyline(gdImagePtr img, shapeObj *p, symbolObj *s, int color, int size, fontSetObj *fontset)
{
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  int i,j;
  double theta, length, current_length;
  labelObj label;
  pointObj point, label_point;
  shapeObj label_poly;
  rectObj label_rect;
  int label_width;
  int offset;
  double rx, ry;

  int blue, yellow;

  blue = gdImageColorAllocate(img, 0, 0, 255);
  yellow = gdImageColorAllocate(img, 255, 255, 0);

  msInitShape(&label_poly);

  initLabel(&label);
  label.type = MS_TRUETYPE;
  label.font = s->font;
  label.sizescaled = size;
  label.color = color;
  label.antialias = s->antialias;

  if(msGetLabelSize(s->character, &label, &label_rect, fontset) == -1)
    return(-1);

  label_width = label_rect.maxx - label_rect.minx;

  offset = 0; // initial padding
  for(i=0; i<p->numlines; i++) {

    for(j=1;j<p->line[i].numpoints;j++) {
      length = sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2)));
      theta = asin(MS_ABS(p->line[i].point[j].x - p->line[i].point[j-1].x)/length);

      if(p->line[i].point[j-1].x < p->line[i].point[j].x) { /* i.e. to the left */
	if(p->line[i].point[j-1].y < p->line[i].point[j].y) /* i.e. below */
	  label.angle = -(90.0 - MS_RAD_TO_DEG*theta);
	else
	  label.angle = (90.0 - MS_RAD_TO_DEG*theta);      
      } else {
	if(p->line[i].point[j-1].y < p->line[i].point[j].y) /* i.e. below */
	  label.angle = (90.0 - MS_RAD_TO_DEG*theta);
	else
	  label.angle = -(90.0 - MS_RAD_TO_DEG*theta);      
      }

      rx = (p->line[i].point[j].x - p->line[i].point[j-1].x)/length;
      ry = (p->line[i].point[j].y - p->line[i].point[j-1].y)/length;

      current_length = offset + label_width/2.0;
      while(current_length <= (length - label_width/2.0)) {
        point.x = MS_NINT(p->line[i].point[j-1].x + current_length*rx);
	point.y = MS_NINT(p->line[i].point[j-1].y + current_length*ry);

  	label_point = get_metrics(&point, MS_CC, label_rect, 0, 0, MS_DEG_TO_RAD*label.angle, 0, &label_poly);
        draw_text(img, label_point, s->character, &label, fontset);

  	// gdImageSetPixel(img, (int)point.x, (int)point.y, yellow);
        // gdImageSetPixel(img, (int)label_point.x, (int)label_point.y, blue);

	current_length += label_width + s->gap;
      }

      current_length -= label_width + s->gap; // revert back to last good length

      offset = MS_MAX((s->gap - MS_NINT(length - current_length)), 0);
    }
  }

  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msImageTruetypePolyline()");
  return(-1);
#endif
}

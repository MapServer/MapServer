/*
** maplabel.c: Routines to enable text drawing, BITMAP or TRUETYPE.
*/
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontt.h>
#include <gdfontmb.h>
#include <gdfontg.h>

#include "map.h"

//#define LINE_VERT_THRESHOLD .17 // max absolute value of cos of line angle, the closer to zero the more vertical the line must be

int msAddLabel(mapObj *map, int layerindex, int classindex, int shapeindex, int tileindex, pointObj *point, char *string, double featuresize, double scalefactor)
{
  int i;
  char wrap[2];

  layerObj *lp=NULL;
  classObj *cp=NULL;

  if(!string) return(MS_SUCCESS); /* not an error */

  if(map->labelcache.numlabels == map->labelcache.cachesize) { /* just add it to the end */
    map->labelcache.labels = (labelCacheMemberObj *) realloc(map->labelcache.labels, sizeof(labelCacheMemberObj)*(map->labelcache.cachesize+MS_LABELCACHEINCREMENT));
    if(!map->labelcache.labels) {
      msSetError(MS_MEMERR, "Realloc() error.", "msAddLabel()");
      return(MS_FAILURE);
    }
    map->labelcache.cachesize += MS_LABELCACHEINCREMENT;
  }

  i = map->labelcache.numlabels;
  map->labelcache.labels[i].layerindex = layerindex;
  map->labelcache.labels[i].classindex = classindex;
  map->labelcache.labels[i].tileindex = tileindex;
  map->labelcache.labels[i].shapeindex = shapeindex;

  lp = &(map->layers[layerindex]);
  cp = &(map->layers[layerindex].class[classindex]);

  map->labelcache.labels[i].point = *point;
  map->labelcache.labels[i].point.x = MS_NINT(map->labelcache.labels[i].point.x);
  map->labelcache.labels[i].point.y = MS_NINT(map->labelcache.labels[i].point.y);

  map->labelcache.labels[i].string = strdup(string);

  // GD/Freetype recognizes \r\n as a true line wrap so we must turn the wrap character into that pattern
  if(cp->label.type != MS_BITMAP && cp->label.wrap != '\0') {
    wrap[0] = cp->label.wrap;
    wrap[1] = '\0';
    map->labelcache.labels[i].string = gsub(map->labelcache.labels[i].string, wrap, "\r\n");
  }

  // copy the styles (only if there is an accompanying marker)
  map->labelcache.labels[i].styles = NULL;
  map->labelcache.labels[i].numstyles = 0;
  if(lp->type == MS_LAYER_ANNOTATION && MS_VALID_COLOR(&(cp->styles[0].color))) {
    map->labelcache.labels[i].styles = (styleObj *) malloc(sizeof(styleObj)*cp->numstyles);
    memcpy(map->labelcache.labels[i].styles, cp->styles, sizeof(styleObj)*cp->numstyles);
    map->labelcache.labels[i].numstyles = cp->numstyles;
  }

  // copy the label
  map->labelcache.labels[i].label = cp->label; // this copies all non-pointers
  if(cp->label.font) map->labelcache.labels[i].label.font = strdup(cp->label.font);

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  if(cp->label.type == MS_TRUETYPE) cp->label.size *= scalefactor;
#endif

  map->labelcache.labels[i].featuresize = featuresize;

  map->labelcache.labels[i].poly = (shapeObj *) malloc(sizeof(shapeObj));
  msInitShape(map->labelcache.labels[i].poly);

  map->labelcache.labels[i].status = MS_FALSE;

  if(lp->type == MS_LAYER_POINT) { // cache the marker placement
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

    msGetMarkerSize(&map->symbolset, &(cp->styles), cp->numstyles, &w, &h);
    w *= scalefactor;
    h *= scalefactor;

    rect.minx = MS_NINT(point->x - .5 * w);
    rect.miny = MS_NINT(point->y - .5 * h);
    rect.maxx = rect.minx + (w-1);
    rect.maxy = rect.miny + (h-1);
    msRectToPolygon(rect, map->labelcache.markers[i].poly);
    map->labelcache.markers[i].id = map->labelcache.numlabels;

    map->labelcache.nummarkers++;
  }

  map->labelcache.numlabels++;

  return(MS_SUCCESS);
}



int msInitFontSet(fontSetObj *fontset)
{
    fontset->filename = NULL;
    fontset->fonts = NULL;
    fontset->numfonts = 0;
    return( 0 );
}

int msFreeFontSet(fontSetObj *fontset)
{
    if (fontset->filename)
        free(fontset->filename);
    fontset->filename = NULL;
    if (fontset->fonts)
        msFreeHashTable(fontset->fonts);
    fontset->fonts = NULL;
    fontset->numfonts = 0;    

    return( 0 );
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
    msSetError(MS_IOERR, "Error opening fontset %s.", "msLoadFontset()",
               fontset->filename);
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
      if(label->font) 
          msSetError(MS_TTFERR, "Requested font (%s) not found.", 
                     "msGetLabelSize()", label->font);
      else
          msSetError(MS_TTFERR, "Requested font (NULL) not found.", 
                     "msGetLabelSize()" );
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

//static pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly)
pointObj get_metrics(pointObj *p, int position, rectObj rect, int ox, int oy, double angle, int buffer, shapeObj *poly)
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
** is a label completely in the image (excluding label buffer)
*/
//static int labelInImage(int width, int height, shapeObj *lpoly, int buffer)
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

//static double dist(pointObj a, pointObj b)
double dist(pointObj a, pointObj b)
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

//static int intersectLabelPolygons(shapeObj *p1, shapeObj *p2) {
int intersectLabelPolygons(shapeObj *p1, shapeObj *p2) {
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

int msImageTruetypeArrow(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor)
{
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msImageTruetypeArrow()");
  return(-1);
#endif
}

int msImageTruetypePolyline(symbolSetObj *symbolset, gdImagePtr img, shapeObj *p, styleObj *style, double scalefactor)
{
#if defined (USE_GD_FT) || defined (USE_GD_TTF)
  int i,j;
  double theta, length, current_length;
  labelObj label;
  pointObj point, label_point;
  shapeObj label_poly;
  rectObj label_rect;
  int label_width;
  int position, rot, gap, in;
  double rx, ry;

  symbolObj *symbol;

  msInitShape(&label_poly);

  symbol = &(symbolset->symbol[style->symbol]);

  initLabel(&label);
  label.type = MS_TRUETYPE;
  label.font = symbol->font;
  label.size = style->size*scalefactor;
  label.color = style->color; // TODO: now assuming these colors should have previously allocated pen values 
  label.outlinecolor = style->outlinecolor;
  label.antialias = symbol->antialias;
  
  if(msGetLabelSize(symbol->character, &label, &label_rect, symbolset->fontset) == -1)
    return(-1);

  label_width = label_rect.maxx - label_rect.minx;

  rot = (symbol->gap < 0);
  gap = MS_ABS(symbol->gap);

  for(i=0; i<p->numlines; i++) {
    current_length = gap+label_width/2.0; // initial padding for each line
    
    for(j=1;j<p->line[i].numpoints;j++) {
      length = sqrt((pow((p->line[i].point[j].x - p->line[i].point[j-1].x),2) + pow((p->line[i].point[j].y - p->line[i].point[j-1].y),2)));
      
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
      label.angle = MS_RAD_TO_DEG * theta;

      in = 0;
      while(current_length <= length) {
        point.x = MS_NINT(p->line[i].point[j-1].x + current_length*rx);
	point.y = MS_NINT(p->line[i].point[j-1].y + current_length*ry);

  	label_point = get_metrics(&point, position, label_rect, 0, 0, label.angle, 0, &label_poly);
        
        msDrawTextGD(img, label_point, symbol->character, &label, symbolset->fontset, scalefactor);

	current_length += label_width + gap;
	in = 1;
      }

      if(in){
        current_length -= label_width + gap; // revert back to last good length
	current_length += gap - length + label_width/2.0;
      } else current_length -= length;
    }
  }

  return(0);
#else
  msSetError(MS_TTFERR, "TrueType font support is not available.", "msImageTruetypePolyline()");
  return(-1);
#endif
}


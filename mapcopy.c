 /* $Id$
 *
 * Functions for coping maps, layers, classes, etc.
 *
 * Unit tests are written in Python using PyUnit and are in
 * mapscript/python/tests/testCopyMap.py.  The tests can be 
 * executed from the python directory as
 *
 *   python2 tests/testCopyMap.py
 * 
 */

#include "map.h"
#include "mapsymbol.h"

void copyProperty(void *dst, void *src, int size) {
  if (src == NULL) dst = NULL;
  else memcpy(dst, src, size);
}

char *copyStringProperty(char **dst, char *src) {
  if (src == NULL || strlen(src) <= 0) *dst = NULL; 
  else {
    if (*dst == NULL) *dst = strdup(src);
    else strcpy(*dst, src);
  }
  return *dst;
}

int msCopyProjection(projectionObj *dst, projectionObj *src) {
  int i;
  msInitProjection(dst);
  copyProperty(&(dst->numargs), &(src->numargs), sizeof(int));
  for (i = 0; i < dst->numargs; i++) {
    dst->args[i] = strdup(src->args[i]);
    //copyStringProperty(&(dst->args[i]), src->args[i]);
  }
  //memcpy(&(dst->proj), &(src->proj), sizeof(projPJ));
  if (dst->numargs != 0)
    msProcessProjection(dst);

  return(0);
}

int msCopyRect(rectObj *dst, rectObj *src) {
  copyProperty(&(dst->minx), &(src->minx), sizeof(double));
  copyProperty(&(dst->miny), &(src->miny), sizeof(double));
  copyProperty(&(dst->maxx), &(src->maxx), sizeof(double));
  copyProperty(&(dst->maxy), &(src->maxy), sizeof(double));

  return(0);
}

int msCopyPoint(pointObj *dst, pointObj *src) {
  copyProperty(&(dst->x), &(src->x), sizeof(double));
  copyProperty(&(dst->y), &(src->y), sizeof(double));
  copyProperty(&(dst->m), &(src->m), sizeof(double));  

  return(0);
}

int msCopyLine(lineObj *dst, lineObj *src) {
  int i;
  copyProperty(&(dst->numpoints), &(src->numpoints), sizeof(int));
  for (i = 0; i < dst->numpoints; i++) {
    msCopyPoint(&(dst->point[i]), &(src->point[i]));
  }

  return(0);
}

/*
int msCopyShapeObj(shapeObj *dst, shapeObj *src) {
  int i;
  copyProperty(&(dst->numlines), &(src->numlines), sizeof(int));
  for (i = 0; i < dst->numlines; i++) {
    msCopyLine(&(dst->line[i]), &(src->line[i]));
  }
  msCopyRect(&(dst->bounds), &(src->bounds));
  copyProperty(&(dst->type), &(src->type), sizeof(int));
  copyProperty(&(dst->index), &(src->index), sizeof(long));
  copyProperty(&(dst->tileindex), &(src->tileindex), sizeof(int));
  copyProperty(&(dst->classindex), &(src->classindex), sizeof(int));
  copyStringProperty(&(dst->text), src->text);
  copyProperty(&(dst->numvalues), &(src->numvalues), sizeof(int));
  for (i = 0; i < dst->numvalues; i++) {
    copyStringProperty(&(dst->values[i]), src->values[i]);
  }

  return(0);
}
*/

int msCopyItem(itemObj *dst, itemObj *src) {
  copyStringProperty(&(dst->name), src->name);
  copyProperty(&(dst->type), &(src->type), sizeof(long));
  copyProperty(&(dst->index), &(src->index), sizeof(int));
  copyProperty(&(dst->size), &(src->size), sizeof(int));
  copyProperty(&(dst->numdecimals), &(src->numdecimals), sizeof(short));

  return(0);
}

int msCopyHashTable(hashTableObj dst, hashTableObj src){
  struct hashObj *tp;
  int i;
  for (i=0;i<MS_HASHSIZE; i++) {
    if (src[i] != NULL) {
      for (tp=src[i]; tp!=NULL; tp=tp->next)
        msInsertHashTable(dst, tp->key, tp->data);
    }
  }
  return(0);
}


int msCopyFontSet(fontSetObj *dst, fontSetObj *src) {
  copyStringProperty(&(dst->filename), src->filename);
  //msCopyHashTable(&(dst->fonts), &(src->fonts));
  memcpy(&(dst->fonts), &(src->fonts), sizeof(hashTableObj));
  copyProperty(&(dst->numfonts), &(src->numfonts), sizeof(int));
  //copyProperty(&(dst->map), &map, sizeof(mapObj *));

  return(0);
}

int msCopyColor(colorObj *dst, colorObj *src) {
  copyProperty(&(dst->pen), &(src->pen), sizeof(int));
  copyProperty(&(dst->red), &(src->red), sizeof(int));
  copyProperty(&(dst->green), &(src->green), sizeof(int));
  copyProperty(&(dst->blue), &(src->blue), sizeof(int));

  return(0);
}


int msCopyExpression(expressionObj *dst,
                     expressionObj *src)
{
  int i;
  copyStringProperty(&(dst->string), src->string);
  copyProperty(&(dst->type), &(src->type), sizeof(int));
  //dst->numitems = 0;
  //dst->items = NULL;
  //dst->indexes = NULL;
  
  //copyProperty(&(dst->numitems), &(src->numitems), sizeof(int));
  //for (i = 0; i < src->numitems; i++) {
  //  copyProperty(&(dst->indexes[i]), &(src->indexes[i]), sizeof(int));
  //  copyStringProperty(&(dst->items[i]), src->items[i]);
  //}
  //dst->regex = NULL;
  dst->compiled = MS_FALSE;
  
  // Compile Regex
  /*if(regcomp(&(dst->regex), src->string, REG_EXTENDED|REG_NOSUB) != 0) {
    msSetError(MS_REGEXERR, "Failed to compile expression (%s).", "msEvalRegex()", dst->string);   
    dst->compiled = MS_FALSE;
  }
  else {
    dst->compiled = MS_TRUE;
  }*/

  return(0);
}

int msCopyJoin(joinObj *dst, joinObj *src) {
  int i, j;
  copyStringProperty(&(dst->name), src->name);
  copyProperty(&(dst->numitems), &(src->numitems), sizeof(int));
  copyProperty(&(dst->numrecords), &(src->numrecords), sizeof(int));
  for (i = 0; i < dst->numitems; i++) {
    copyStringProperty(&(dst->items[i]), src->items[i]);
    for (j = 0; j < dst->numrecords; j++) {
      copyStringProperty(&(dst->values[i][j]), src->values[i][j]);
    }
  }
  copyStringProperty(&(dst->table), src->table);
  copyStringProperty(&(dst->from), src->from);
  copyStringProperty(&(dst->to), src->to);
  copyStringProperty(&(dst->header), src->header);
#ifndef __cplusplus
  copyStringProperty(&(dst->template), src->template);
#else
  copyStringProperty(&(dst->_template), src->_template);
#endif
  copyStringProperty(&(dst->footer), src->footer);
  //copyStringProperty(&(dst->match), src->match);
  copyProperty(&(dst->type), &(src->type), sizeof(enum MS_JOIN_TYPE));
  copyStringProperty(&(dst->connection), src->connection);
  copyProperty(&(dst->connectiontype), &(src->connectiontype), sizeof(enum MS_JOIN_CONNECTION_TYPE));

  return(0);
}

int msCopyOutputFormat(outputFormatObj *dst,
                       outputFormatObj *src)
{
  int i;
  copyStringProperty(&(dst->name), src->name);
  copyStringProperty(&(dst->mimetype), src->mimetype);
  copyStringProperty(&(dst->driver), src->driver);
  copyStringProperty(&(dst->extension), src->extension);
  copyProperty(&(dst->renderer), &(src->renderer), sizeof(int));
  copyProperty(&(dst->imagemode), &(src->imagemode), sizeof(int));
  copyProperty(&(dst->transparent), &(src->transparent), sizeof(int));
  copyProperty(&(dst->numformatoptions), &(src->numformatoptions), sizeof(int));

  memcpy(&(dst->formatoptions), &(src->formatoptions), sizeof(char *)*dst->numformatoptions);

  //for (i = 0; i < dst->numformatoptions; i++) {
  //  memcpy(&(dst->formatoptions[i]), &(src->formatoptions[i]), );
  //}

  copyProperty(&(dst->refcount), &(src->refcount), sizeof(int));

  return(0);
}

int msCopyQueryMap(queryMapObj *dst, queryMapObj *src) {
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  copyProperty(&(dst->status), &(src->status), sizeof(int));
  copyProperty(&(dst->style), &(src->style), sizeof(int));
  msCopyColor(&(dst->color), &(src->color));

  return(0);
}

int msCopyLabel(labelObj *dst, labelObj *src) {
  copyStringProperty(&(dst->font), src->font);
  copyProperty(&(dst->type), &(src->type), sizeof(enum MS_FONT_TYPE));
  msCopyColor(&(dst->color), &(src->color));
  msCopyColor(&(dst->outlinecolor), &(src->outlinecolor));
  msCopyColor(&(dst->shadowcolor), &(src->shadowcolor));
  copyProperty(&(dst->shadowsizex), &(src->shadowsizex), sizeof(int));
  copyProperty(&(dst->shadowsizey), &(src->shadowsizey), sizeof(int));
  msCopyColor(&(dst->backgroundcolor), &(src->backgroundcolor));
  msCopyColor(&(dst->backgroundshadowcolor), &(src->backgroundshadowcolor));
  copyProperty(&(dst->backgroundshadowsizex), &(src->backgroundshadowsizex), sizeof(int));
  copyProperty(&(dst->backgroundshadowsizey), &(src->backgroundshadowsizey), sizeof(int));
  copyProperty(&(dst->size), &(src->size), sizeof(int));
  copyProperty(&(dst->sizescaled), &(src->sizescaled), sizeof(int));
  copyProperty(&(dst->minsize), &(src->minsize), sizeof(int));
  copyProperty(&(dst->maxsize), &(src->maxsize), sizeof(int));
  copyProperty(&(dst->position), &(src->position), sizeof(int));
  copyProperty(&(dst->offsetx), &(src->offsetx), sizeof(int));
  copyProperty(&(dst->offsety), &(src->offsety), sizeof(int));
  copyProperty(&(dst->angle), &(src->angle), sizeof(double));
  copyProperty(&(dst->autoangle), &(src->autoangle), sizeof(int));
  copyProperty(&(dst->buffer), &(src->buffer), sizeof(int));
  copyProperty(&(dst->antialias), &(src->antialias), sizeof(int));
  copyProperty(&(dst->wrap), &(src->wrap), sizeof(char));
  copyProperty(&(dst->minfeaturesize), &(src->minfeaturesize), sizeof(int));
  copyProperty(&(dst->autominfeaturesize), &(src->autominfeaturesize), sizeof(int));
  copyProperty(&(dst->mindistance), &(src->mindistance), sizeof(int));
  copyProperty(&(dst->partials), &(src->partials), sizeof(int));
  copyProperty(&(dst->force), &(src->force), sizeof(int));

  return(0);
}

int msCopyWeb(webObj *dst, webObj *src) {
  copyStringProperty(&(dst->log), src->log);
  copyStringProperty(&(dst->imagepath), src->imagepath);
  copyStringProperty(&(dst->imageurl), src->imageurl);
  //copyProperty(&(dst->map), &map, sizeof(mapObj *));
#ifndef __cplusplus
  copyStringProperty(&(dst->template), src->template);
#else
  copyStringProperty(&(dst->_template), src->_template);
#endif
  copyStringProperty(&(dst->header), src->header);
  copyStringProperty(&(dst->footer), src->footer);
  copyStringProperty(&(dst->empty), src->empty);
  copyStringProperty(&(dst->error), src->error);
  msCopyRect(&(dst->extent), &(src->extent));
  copyProperty(&(dst->minscale), &(src->minscale), sizeof(double));
  copyProperty(&(dst->maxscale), &(src->maxscale), sizeof(double));
  copyStringProperty(&(dst->mintemplate), src->mintemplate);
  copyStringProperty(&(dst->maxtemplate), src->maxtemplate);
  
  if (src->metadata)
  {
    dst->metadata = msCreateHashTable();
    msCopyHashTable((dst->metadata), (src->metadata));
  }
  //memcpy(&(dst->metadata), &(src->metadata), sizeof(hashTableObj));

  return(0);
}

int msCopyStyle(styleObj *dst, styleObj *src) {
  msCopyColor(&(dst->color), &(src->color));
  msCopyColor(&(dst->outlinecolor), &(src->outlinecolor));
  msCopyColor(&(dst->backgroundcolor), &(src->backgroundcolor));
  copyProperty(&(dst->symbol), &(src->symbol), sizeof(int));
  copyStringProperty(&(dst->symbolname), src->symbolname);
  copyProperty(&(dst->size), &(src->size), sizeof(int));
  copyProperty(&(dst->sizescaled), &(src->sizescaled), sizeof(int));
  copyProperty(&(dst->minsize), &(src->minsize), sizeof(int));
  copyProperty(&(dst->maxsize), &(src->maxsize), sizeof(int));
  copyProperty(&(dst->offsetx), &(src->offsetx), sizeof(int));
  copyProperty(&(dst->offsety), &(src->offsety), sizeof(int));

  return(0);
}

int msCopyClass(classObj *dst, classObj *src, layerObj *layer) {
  int i;
  msCopyExpression(&(dst->expression), &(src->expression));
  copyProperty(&(dst->status), &(src->status), sizeof(int));
  memcpy(&(dst->status), &(src->status), sizeof(int));

  copyProperty(&(dst->numstyles), &(src->numstyles), sizeof(int));
  for (i = 0; i < dst->numstyles; i++) {
    msCopyStyle(&(dst->styles[i]), &(src->styles[i]));
  }
  msCopyLabel(&(dst->label), &(src->label));
  copyStringProperty(&(dst->name), src->name);
  copyStringProperty(&(dst->title), src->title);
  msCopyExpression(&(dst->text), &(src->text));
#ifndef __cplusplus
  copyStringProperty(&(dst->template), src->template);
#else
  copyStringProperty(&(dst->_template), src->_template);
#endif
  copyProperty(&(dst->type), &(src->type), sizeof(int));

  if (src->metadata)
  {
    dst->metadata = msCreateHashTable();
    msCopyHashTable((dst->metadata), (src->metadata));
  }
  //memcpy(&(dst->metadata), &(src->metadata), sizeof(hashTableObj));

  copyProperty(&(dst->minscale), &(src->minscale), sizeof(double));
  copyProperty(&(dst->maxscale), &(src->maxscale), sizeof(double));
  copyProperty(&(dst->layer), &layer, sizeof(layerObj *));
  copyProperty(&(dst->debug), &(src->debug), sizeof(int));

  return(0);
}

int msCopyLabelCacheMember(labelCacheMemberObj *dst,
                           labelCacheMemberObj *src)
{
  int i;
  copyStringProperty(&(dst->string), src->string);
  copyProperty(&(dst->featuresize), &(src->featuresize), sizeof(double));
  copyProperty(&(dst->numstyles), &(src->numstyles), sizeof(int));
  for (i = 0; i < dst->numstyles; i++) {
    msCopyStyle(&(dst->styles[i]), &(src->styles[i]));
  }
  msCopyLabel(&(dst->label), &(src->label));
  copyProperty(&(dst->layerindex), &(src->layerindex), sizeof(int));
  copyProperty(&(dst->classindex), &(src->classindex), sizeof(int));
  copyProperty(&(dst->tileindex), &(src->tileindex), sizeof(int));
  copyProperty(&(dst->shapeindex), &(src->shapeindex), sizeof(int));
  msCopyPoint(&(dst->point), &(src->point));
  //msCopyShape(&(dst->poly), &(src->poly));
  copyProperty(&(dst->status), &(src->status), sizeof(int));

  return(0);
}

int msCopyMarkerCacheMember(markerCacheMemberObj *dst,
                            markerCacheMemberObj *src) 
{
  copyProperty(&(dst->id), &(src->id), sizeof(int));
  //msCopyShape(&(dst->poly), &(src->poly));

  return(0);
}

int msCopyLabelCache(labelCacheObj *dst, labelCacheObj *src) {
  int i;
  copyProperty(&(dst->numlabels), &(src->numlabels), sizeof(int));
  for (i = 0; i < dst->numlabels; i++) {
    msCopyLabelCacheMember(&(dst->labels[i]), &(src->labels[i]));
  }
  copyProperty(&(dst->cachesize), &(src->cachesize), sizeof(int));
  copyProperty(&(dst->nummarkers), &(src->nummarkers), sizeof(int));
  for (i = 0; i < dst->nummarkers; i++) {
    msCopyMarkerCacheMember(&(dst->markers[i]), &(src->markers[i]));
  }
  copyProperty(&(dst->markercachesize), &(src->markercachesize), sizeof(int));

  return(0);
}

int msCopyResultCacheMember(resultCacheMemberObj *dst,
                            resultCacheMemberObj *src)
{
  copyProperty(&(dst->shapeindex), &(src->shapeindex), sizeof(long));
  copyProperty(&(dst->tileindex), &(src->tileindex), sizeof(int));
  copyProperty(&(dst->classindex), &(src->classindex), sizeof(int));

  return(0);
}

int msCopyResultCache(resultCacheObj *dst, resultCacheObj *src) {
  int i;
  copyProperty(&(dst->cachesize), &(src->cachesize), sizeof(int));
  copyProperty(&(dst->numresults), &(src->numresults), sizeof(int));
  for (i = 0; i < dst->numresults; i++) {
    msCopyResultCacheMember(&(dst->results[i]), &(src->results[i]));
  }
  msCopyRect(&(dst->bounds), &(src->bounds));

  return(0);
}


int msCopySymbolSet(symbolSetObj *dst, symbolSetObj *src) {
  int i;
  copyStringProperty(&(dst->filename), src->filename);
  //copyProperty(&(dst->map), &map, sizeof(mapObj *));
  copyProperty(&(dst->fontset), &(src->fontset), sizeof(fontSetObj *));
  copyProperty(&(dst->numsymbols), &(src->numsymbols), sizeof(int));
  for (i = 0; i < dst->numsymbols; i++) {
    memcpy(&(dst->symbol[i]), &(src->symbol[i]), sizeof(symbolObj));
  }
  copyProperty(&(dst->imagecache), &(src->imagecache), sizeof(struct imageCacheObj));
  copyProperty(&(dst->imagecachesize), &(src->imagecachesize), sizeof(int));

  return(0);
}

int msCopyReferenceMap(referenceMapObj *dst, referenceMapObj *src) {
  initReferenceMap(dst);
  msCopyRect(&(dst->extent), &(src->extent));
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  msCopyColor(&(dst->color), &(src->color));
  msCopyColor(&(dst->outlinecolor), &(src->outlinecolor));
  copyStringProperty(&(dst->image), src->image);
  copyProperty(&(dst->status), &(src->status), sizeof(int));
  copyProperty(&(dst->marker), &(src->marker), sizeof(int));
  copyStringProperty(&(dst->markername), src->markername);
  copyProperty(&(dst->markersize), &(src->markersize), sizeof(int));
  copyProperty(&(dst->minboxsize), &(src->minboxsize), sizeof(int));
  copyProperty(&(dst->maxboxsize), &(src->maxboxsize), sizeof(int));
  //copyProperty(&(dst->map), &map, sizeof(mapObj *));

  return(0);
}

int msCopyScalebar(scalebarObj *dst, scalebarObj *src) {
  initScalebar(dst);
  msCopyColor(&(dst->imagecolor), &(src->imagecolor));
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  copyProperty(&(dst->style), &(src->style), sizeof(int));
  copyProperty(&(dst->intervals), &(src->intervals), sizeof(int));
  msCopyLabel(&(dst->label), &(src->label));
  msCopyColor(&(dst->color), &(src->color));
  msCopyColor(&(dst->backgroundcolor), &(src->backgroundcolor));
  msCopyColor(&(dst->outlinecolor), &(src->outlinecolor));
  copyProperty(&(dst->units), &(src->units), sizeof(int));
  copyProperty(&(dst->status), &(src->status), sizeof(int));
  copyProperty(&(dst->position), &(src->position), sizeof(int));
  copyProperty(&(dst->transparent), &(src->transparent), sizeof(int));
  copyProperty(&(dst->interlace), &(src->interlace), sizeof(int));
  copyProperty(&(dst->postlabelcache), &(src->postlabelcache), sizeof(int));

  return(0);
}

int msCopyLegend(legendObj *dst, legendObj *src) {
  msCopyColor(&(dst->imagecolor), &(src->imagecolor));
  msCopyLabel(&(dst->label), &(src->label));
  copyProperty(&(dst->keysizex), &(src->keysizex), sizeof(int));
  copyProperty(&(dst->keysizey), &(src->keysizey), sizeof(int));
  copyProperty(&(dst->keyspacingx), &(src->keyspacingx), sizeof(int));
  copyProperty(&(dst->keyspacingy), &(src->keyspacingy), sizeof(int));
  msCopyColor(&(dst->outlinecolor), &(src->outlinecolor));
  copyProperty(&(dst->status), &(src->status), sizeof(int));
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  copyProperty(&(dst->position), &(src->position), sizeof(int));
  copyProperty(&(dst->transparent), &(src->transparent), sizeof(int));
  copyProperty(&(dst->interlace), &(src->interlace), sizeof(int));
  copyProperty(&(dst->postlabelcache), &(src->postlabelcache), sizeof(int));
#ifndef __cplusplus
   copyStringProperty(&(dst->template), src->template);
#else
   copyStringProperty(&(dst->_template), src->_template);
#endif
  //copyProperty(&(dst->map), &map, sizeof(mapObj *));

  return(0);
}


int msCopyLayer(layerObj *dst, layerObj *src) {
  int i;
  featureListNodeObjPtr current;
  copyProperty(&(dst->index), &(src->index), sizeof(int));
  copyStringProperty(&(dst->classitem), src->classitem);
  copyProperty(&(dst->classitemindex), &(src->classitemindex), sizeof(int));
  copyProperty(&(dst->numclasses), &(src->numclasses), sizeof(int));
  for (i = 0; i < dst->numclasses; i++) {
#ifndef __cplusplus
    initClass(&(dst->class[i]));
    msCopyClass(&(dst->class[i]), &(src->class[i]), dst);
#else
    initClass(&(dst->_class[i]));
    msCopyClass(&(dst->_class[i]), &(src->_class[i]), dst);
#endif
  }
  copyStringProperty(&(dst->header), src->header);
  copyStringProperty(&(dst->footer), src->footer);
#ifndef __cplusplus
  copyStringProperty(&(dst->template), src->template);
#else
  copyStringProperty(&(dst->_template), src->_template);
#endif
  copyProperty(&(dst->resultcache), &(src->resultcache), sizeof(resultCacheObj *));
  copyStringProperty(&(dst->name), src->name); 
  copyStringProperty(&(dst->group), src->group); 
  copyProperty(&(dst->status), &(src->status), sizeof(int)); 
  copyStringProperty(&(dst->data), src->data); 
  copyProperty(&(dst->type), &(src->type), sizeof(enum MS_LAYER_TYPE));
  copyProperty(&(dst->annotate), &(src->annotate), sizeof(int)); 
  copyProperty(&(dst->tolerance), &(src->tolerance), sizeof(double)); 
  copyProperty(&(dst->toleranceunits), &(src->toleranceunits), sizeof(int));
  copyProperty(&(dst->symbolscale), &(src->symbolscale), sizeof(double)); 
  copyProperty(&(dst->scalefactor), &(src->scalefactor), sizeof(double)); 
  copyProperty(&(dst->minscale), &(src->minscale), sizeof(double));
  copyProperty(&(dst->maxscale), &(src->maxscale), sizeof(double));
  copyProperty(&(dst->labelminscale), &(src->labelminscale), sizeof(double));
  copyProperty(&(dst->labelmaxscale), &(src->labelmaxscale), sizeof(double));
  copyProperty(&(dst->sizeunits), &(src->sizeunits), sizeof(int)); 
  copyProperty(&(dst->maxfeatures), &(src->maxfeatures), sizeof(int));
  msCopyColor(&(dst->offsite), &(src->offsite));
  copyProperty(&(dst->transform), &(src->transform), sizeof(int)); 
  copyProperty(&(dst->labelcache), &(src->labelcache), sizeof(int));
  copyProperty(&(dst->postlabelcache), &(src->postlabelcache), sizeof(int)); 
  copyStringProperty(&(dst->labelitem), src->labelitem);
  copyStringProperty(&(dst->labelsizeitem), src->labelsizeitem);
  copyStringProperty(&(dst->labelangleitem), src->labelangleitem);
  copyProperty(&(dst->labelitemindex), &(src->labelitemindex), sizeof(int));
  copyProperty(&(dst->labelsizeitemindex), &(src->labelsizeitemindex), sizeof(int));
  copyProperty(&(dst->labelangleitemindex), &(src->labelangleitemindex), sizeof(int));
  copyStringProperty(&(dst->tileitem), src->tileitem);
  copyProperty(&(dst->tileitemindex), &(src->tileitemindex), sizeof(int));
  copyStringProperty(&(dst->tileindex), src->tileindex); 
  msCopyProjection(&(dst->projection), &(src->projection));
  copyProperty(&(dst->project), &(src->project), sizeof(int)); 
  copyProperty(&(dst->units), &(src->units), sizeof(int)); 

  current = src->features;
  while(current != NULL) {
    insertFeatureList(&(dst->features), &(current->shape));
    current = current->next;
  }

  copyStringProperty(&(dst->connection), src->connection);
  copyProperty(&(dst->connectiontype), &(src->connectiontype), sizeof(enum MS_CONNECTION_TYPE));
  copyProperty(&(dst->sameconnection), &(src->sameconnection), sizeof(layerObj *));
  
  msSHPOpenFile(&(dst->shpfile), "rb", src->data);
  msSHPOpenFile(&(dst->tileshpfile), "rb", src->tileindex);

  copyProperty(&(dst->ogrlayerinfo), &(src->ogrlayerinfo), sizeof(void)); 
  copyProperty(&(dst->sdelayerinfo), &(src->sdelayerinfo), sizeof(void)); 
  copyProperty(&(dst->postgislayerinfo), &(src->postgislayerinfo), sizeof(void)); 
  copyProperty(&(dst->oraclespatiallayerinfo), &(src->oraclespatiallayerinfo), sizeof(void));
  copyProperty(&(dst->wfslayerinfo), &(src->wfslayerinfo), sizeof(void)); 
  copyProperty(&(dst->numitems), &(src->numitems), sizeof(int));
  for (i = 0; i < dst->num_processing; i++) {
    copyStringProperty(&(dst->items[i]), src->items[i]);
  }
  copyProperty(&(dst->iteminfo), &(src->iteminfo), sizeof(void)); 
  msCopyExpression(&(dst->filter), &(src->filter));
  copyStringProperty(&(dst->filteritem), src->filteritem);
  copyProperty(&(dst->filteritemindex), &(src->filteritemindex), sizeof(int));
  copyStringProperty(&(dst->styleitem), src->styleitem); 
  copyProperty(&(dst->styleitemindex), &(src->styleitemindex), sizeof(int));
  copyStringProperty(&(dst->requires), src->requires); 
  copyStringProperty(&(dst->labelrequires), src->labelrequires);

  if (src->metadata)
  {
    dst->metadata = msCreateHashTable();
    msCopyHashTable((dst->metadata), (src->metadata));
  }
  //memcpy(&(dst->metadata), &(src->metadata), sizeof(hashTableObj));

  copyProperty(&(dst->transparency), &(src->transparency), sizeof(int)); 
  copyProperty(&(dst->dump), &(src->dump), sizeof(int));
  copyProperty(&(dst->debug), &(src->debug), sizeof(int));
  copyProperty(&(dst->num_processing), &(src->num_processing), sizeof(int));
  for (i = 0; i < dst->num_processing; i++) {
    copyStringProperty(&(dst->processing[i]), src->processing[i]);
  }
  copyProperty(&(dst->numjoins), &(src->numjoins), sizeof(int));
  for (i = 0; i < dst->num_processing; i++) {
    msCopyJoin(&(dst->joins[i]), &(src->joins[i]));
  }

  return(0);
}

int msCopyMap(mapObj *dst, mapObj *src) {
  int i;
  copyStringProperty(&(dst->name), src->name); 
  copyProperty(&(dst->status), &(src->status), sizeof(int)); 
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  copyProperty(&(dst->numlayers), &(src->numlayers), sizeof(int)); 
  for (i = 0; i < dst->numlayers; i++) {
    initLayer(&(dst->layers[i]), dst);
    msCopyLayer(&(dst->layers[i]), &(src->layers[i]));
  }
  msCopySymbolSet(&(dst->symbolset), &(src->symbolset));
  msCopyFontSet(&(dst->fontset), &(src->fontset));
  //msCopyLabelCache(&(dst->labelcache), &(src->labelcache));
  copyProperty(&(dst->transparent), &(src->transparent), sizeof(int)); 
  copyProperty(&(dst->interlace), &(src->interlace), sizeof(int)); 
  copyProperty(&(dst->imagequality), &(src->imagequality), sizeof(int)); 
  msCopyRect(&(dst->extent), &(src->extent));
  copyProperty(&(dst->cellsize), &(src->cellsize), sizeof(double)); 
  copyProperty(&(dst->units), &(src->units), sizeof(enum MS_UNITS));
  copyProperty(&(dst->scale), &(src->scale), sizeof(double)); 
  copyProperty(&(dst->resolution), &(src->resolution), sizeof(int));
  copyStringProperty(&(dst->shapepath), src->shapepath); 
  copyStringProperty(&(dst->mappath), src->mappath); 
  msCopyColor(&(dst->imagecolor), &(src->imagecolor));
  copyProperty(&(dst->numoutputformats), &(src->numoutputformats), sizeof(int));

  msCopyOutputFormat(dst->outputformat, src->outputformat);
  for (i = 0; i < src->numoutputformats; i++) {
    msCopyOutputFormat(dst->outputformatlist[i], src->outputformatlist[i]);
  }

  copyStringProperty(&(dst->imagetype), src->imagetype); 
  msCopyProjection(&(dst->projection), &(src->projection));
  msCopyProjection(&(dst->latlon), &(src->latlon));
  msCopyReferenceMap(&(dst->reference), &(src->reference));
  msCopyScalebar(&(dst->scalebar), &(src->scalebar));
  msCopyLegend(&(dst->legend), &(src->legend));
  msCopyQueryMap(&(dst->querymap), &(src->querymap));
  msCopyWeb(&(dst->web), &(src->web));
  for (i = 0; i < dst->numlayers; i++) {
    copyProperty(&(dst->layerorder[i]), &(src->layerorder[i]), sizeof(int));
  }
  copyProperty(&(dst->debug), &(src->debug), sizeof(int));
  copyStringProperty(&(dst->datapattern), src->datapattern);
  copyStringProperty(&(dst->templatepattern), src->templatepattern);   

  return(0);
}

#ifdef USE_MING_FLASH
int msCopySWF(SWFObj *dst, SWFObj *src) {
  int i;
  //copyProperty(&(dst->map), &map, sizeof(mapObj *));
  //msCopySWFMovie(&(dst->sMainMovie), &(src->sMainMovie), sizeof(SWFMovie));
  copyProperty(&(dst->nLayerMovies), &(src->nLayerMovies), sizeof(int));
  for (i = 0; i < dst->nLayerMovies; i++) {
    memcpy(&(dst->pasMovies[i]), &(src->pasMovies[i]), sizeof(SWFMovie));
  }
  copyProperty(&(dst->nCurrentMovie), &(src->nCurrentMovie), sizeof(int));
  copyProperty(&(dst->nCurrentLayerIdx), &(src->nCurrentLayerIdx), sizeof(int));
  copyProperty(&(dst->nCurrentShapeIdx), &(src->nCurrentShapeIdx), sizeof(int));
  copyProperty(&(dst->imagetmp), &(src->imagetmp), sizeof(void *));
                      
  return(0);
}
#endif

#ifdef notdefUSE_PDF
int msCopyPDF(PDFObj *dst, PDFObj *src) {
  //copyProperty(&(dst->map), &map, sizeof(mapObj *));
  copyProperty(&(dst->pdf), &(src->pdf), sizeof(PDF));

  return(0);
}
#endif

int msCopyImage(imageObj *dst, imageObj *src) {
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyStringProperty(&(dst->imagepath), src->imagepath);
  copyStringProperty(&(dst->imageurl), src->imageurl);
  copyProperty(&(dst->format), &(src->format), sizeof(outputFormatObj *));
  copyProperty(&(dst->renderer), &(src->renderer), sizeof(int));
 
  copyProperty(&(dst->img.gd), &(dst->img.gd), sizeof(gdImagePtr));
#ifdef USE_MING_FLASH
  copyProperty(&(dst->img.swf), &(dst->img.swf), sizeof(SWFObj *));
#endif
#ifdef USE_PDF
  copyProperty(&(dst->img.pdf), &(dst->img.pdf), sizeof(PDFObj *));
#endif
  copyProperty(&(dst->img.raw_16bit), &(src->img.raw_16bit), sizeof(short *));
  copyProperty(&(dst->img.raw_float), &(src->img.raw_float), sizeof(float *));
  
  return(0);
}




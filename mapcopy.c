/**********************************************************************
 * 
 * $Id$
 *
 * Project: MapServer
 * Purpose: Functions to allow copying/cloning of maps
 * Author:  Sean Gillies, sgillies@frii.com
 *
 * Notes: These functions are not in mapfile.c because that file is 
 * cumbersome enough as it is.  There is agreement that this code and
 * that in mapfile.c should eventually be split up by object into 
 * mapobj.c, layerobj.c, etc.  Or something like that.  
 *
 * Unit tests are written in Python using PyUnit and are in
 * mapscript/python/tests/testCopyMap.py.  The tests can be 
 * executed from the python directory as
 *
 *   python2 tests/testCopyMap.py
 *
 * I just find Python to be very handy for unit testing, that's all.
 *
 *********************************************************************/

#include "map.h"
#include "mapsymbol.h"

/***********************************************************************
 * Make the CVS Id available in mapcopy.o through 'strings'            *
 **********************************************************************/

#ifndef DISABLE_CVSID
#  define CPL_CVSID(string)     static char cpl_cvsid[] = string; \
static char *cvsid_aw() { return( cvsid_aw() ? ((char *) NULL) : cpl_cvsid ); }
#else
#  define CPL_CVSID(string)
#endif

CPL_CVSID("$Id$");

/***********************************************************************
 * copyProperty()                                                      *
 *                                                                     *
 * Wrapper for memcpy(), catches the case where we try to memcpy       *
 * with a NULL source pointer.  Used to copy properties (members)      *
 * that are not pointers, strings, or structures.                      *
 **********************************************************************/

void copyProperty(void *dst, void *src, int size) {
  if (src == NULL) dst = NULL;
  else memcpy(dst, src, size);
}

/***********************************************************************
 * copyStringProperty()                                                *
 *                                                                     *
 * Wrapper for strcpy()/strdup(), catches the cases where we try to    *
 * copy a NULL or pathological string.  Used to copy properties        *
 * (members) that are strings.                                         *
 **********************************************************************/

char *copyStringProperty(char **dst, char *src) {
  if (src == NULL || strlen(src) <= 0) *dst = NULL; 
  else {
    if (*dst == NULL) *dst = strdup(src);
    else strcpy(*dst, src);
  }
  return *dst;
}

/***********************************************************************
 * msCopyProjection()                                                  *
 *                                                                     *
 * Copy a projectionObj                                                *
 **********************************************************************/

int msCopyProjection(projectionObj *dst, projectionObj *src) {
  int i;
  if (msInitProjection(dst) != MS_SUCCESS) return(MS_FAILURE);
  copyProperty(&(dst->numargs), &(src->numargs), sizeof(int));
  for (i = 0; i < dst->numargs; i++) {
    dst->args[i] = strdup(src->args[i]);
  }
  if (dst->numargs != 0)
    if (msProcessProjection(dst) != MS_SUCCESS) return(MS_FAILURE);

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyRect()                                                        *
 *                                                                     *
 * Copy a rectObj                                                      *
 **********************************************************************/

int msCopyRect(rectObj *dst, rectObj *src) {
  copyProperty(&(dst->minx), &(src->minx), sizeof(double));
  copyProperty(&(dst->miny), &(src->miny), sizeof(double));
  copyProperty(&(dst->maxx), &(src->maxx), sizeof(double));
  copyProperty(&(dst->maxy), &(src->maxy), sizeof(double));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyPoint()                                                       *
 *                                                                     *
 * Copy a pointObj                                                     *
 **********************************************************************/

int msCopyPoint(pointObj *dst, pointObj *src) {
  copyProperty(&(dst->x), &(src->x), sizeof(double));
  copyProperty(&(dst->y), &(src->y), sizeof(double));
  copyProperty(&(dst->m), &(src->m), sizeof(double));  

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyLine()                                                        *
 *                                                                     *
 * Copy a lineObj, using msCopyPoint()                                 *
 **********************************************************************/
int msCopyLine(lineObj *dst, lineObj *src) {
  int i;
  copyProperty(&(dst->numpoints), &(src->numpoints), sizeof(int));
  for (i = 0; i < dst->numpoints; i++) {
    if (msCopyPoint(&(dst->point[i]), &(src->point[i])) != MS_SUCCESS) {
      return(MS_FAILURE);
    }
  }

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyShape()                                                       *
 *                                                                     *
 * Copy a shapeObj, using msCopyLine(), msCopyRect()                   *
 * Not completely implemented or tested.                               *
 **********************************************************************/
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

/***********************************************************************
 * msCopyItem()                                                        *
 *                                                                     *
 * Copy an itemObj                                                     *
 **********************************************************************/

int msCopyItem(itemObj *dst, itemObj *src) {
  copyStringProperty(&(dst->name), src->name);
  copyProperty(&(dst->type), &(src->type), sizeof(long));
  copyProperty(&(dst->index), &(src->index), sizeof(int));
  copyProperty(&(dst->size), &(src->size), sizeof(int));
  copyProperty(&(dst->numdecimals),&(src->numdecimals),sizeof(short));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyHashTable()                                                   *
 *                                                                     *
 * Copy a hashTableObj, using msInsertHashTable()                      *
 **********************************************************************/

int msCopyHashTable(hashTableObj dst, hashTableObj src){
  struct hashObj *tp;
  int i;
  for (i=0;i<MS_HASHSIZE; i++) {
    if (src[i] != NULL) {
      for (tp=src[i]; tp!=NULL; tp=tp->next)
        msInsertHashTable(dst, tp->key, tp->data);
    }
  }
  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyFontSet()                                                     *
 *                                                                     *
 * Copy a fontSetObj, using msCreateHashTable() and msCopyHashTable()  *
 **********************************************************************/

int msCopyFontSet(fontSetObj *dst, fontSetObj *src, mapObj *map) {
  dst->filename = strdup(src->filename);
  //copyStringProperty(&(dst->filename), src->filename);
  copyProperty(&(dst->numfonts), &(src->numfonts), sizeof(int));
  if (src->fonts) {
    dst->fonts = msCreateHashTable();
    if (msCopyHashTable((dst->fonts), (src->fonts)) != MS_SUCCESS)
      return(MS_FAILURE);
  }

  copyProperty(&(dst->map), &map, sizeof(mapObj *));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyColor()                                                       *
 *                                                                     *
 * Copy a colorObj                                                     *
 **********************************************************************/

int msCopyColor(colorObj *dst, colorObj *src) {
  copyProperty(&(dst->pen), &(src->pen), sizeof(int));
  copyProperty(&(dst->red), &(src->red), sizeof(int));
  copyProperty(&(dst->green), &(src->green), sizeof(int));
  copyProperty(&(dst->blue), &(src->blue), sizeof(int));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyExpression(                                                   *
 *                                                                     *
 * Copy an expressionObj, but only its string and type                 *
 **********************************************************************/

int msCopyExpression(expressionObj *dst, expressionObj *src)
{
  copyStringProperty(&(dst->string), src->string);
  copyProperty(&(dst->type), &(src->type), sizeof(int));
  dst->compiled = MS_FALSE;

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyJoin()                                                        *
 *                                                                     *
 * Copy a joinObj                                                      *
 **********************************************************************/

int msCopyJoin(joinObj *dst, joinObj *src)
{
  int i;
  copyStringProperty(&(dst->name), src->name);
  
  // not sure it makes sense to copy the values (or the items for that matter) since
  // since they are runtime additions to the mapfile, probably should be NULL with numitems=0
  copyProperty(&(dst->numitems), &(src->numitems), sizeof(int));
  for (i = 0; i < dst->numitems; i++) {
    copyStringProperty(&(dst->items[i]), src->items[i]);
    copyStringProperty(&(dst->values[i]), src->values[i]);  
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
  copyProperty(&(dst->type), &(src->type), sizeof(enum MS_JOIN_TYPE));
  copyStringProperty(&(dst->connection), src->connection);
  
  copyProperty(&(dst->connectiontype), &(src->connectiontype), sizeof(enum MS_JOIN_CONNECTION_TYPE));

  // TODO: need to handle joininfo (probably should be just set to NULL)
  dst->joininfo = NULL;

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyOutputFormat()                                                *
 **********************************************************************/

int msCopyOutputFormat(outputFormatObj *dst, outputFormatObj *src)
{
  copyStringProperty(&(dst->name), src->name);
  copyStringProperty(&(dst->mimetype), src->mimetype);
  copyStringProperty(&(dst->driver), src->driver);
  copyStringProperty(&(dst->extension), src->extension);
  copyProperty(&(dst->renderer), &(src->renderer), sizeof(int));
  copyProperty(&(dst->imagemode), &(src->imagemode), sizeof(int));
  copyProperty(&(dst->transparent), &(src->transparent), sizeof(int));
  copyProperty(&(dst->refcount), &(src->refcount), sizeof(int));
  
  copyProperty(&(dst->numformatoptions), &(src->numformatoptions),
               sizeof(int));
  
  if (dst->formatoptions > 0) {
      
    dst->formatoptions =
        (char **)malloc(sizeof(char *)*dst->numformatoptions);

    memcpy(&(dst->formatoptions), &(src->formatoptions), 
           sizeof(char *)*dst->numformatoptions);
  }

  if (!msOutputFormatValidate(dst)) return(MS_FAILURE);
  else return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyQueryMap()                                                    *
 *                                                                     *
 * Copy a queryMapObj, using msCopyColor()                             *
 **********************************************************************/

int msCopyQueryMap(queryMapObj *dst, queryMapObj *src) {
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  copyProperty(&(dst->status), &(src->status), sizeof(int));
  copyProperty(&(dst->style), &(src->style), sizeof(int));
  msCopyColor(&(dst->color), &(src->color));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyLabel()                                                       *
 *                                                                     *
 * Copy a labelObj, using msCopyColor()                                *
 **********************************************************************/

int msCopyLabel(labelObj *dst, labelObj *src) {
  int return_value;
  copyStringProperty(&(dst->font), src->font);
  copyProperty(&(dst->type), &(src->type), sizeof(enum MS_FONT_TYPE));
  
  return_value = msCopyColor(&(dst->color), &(src->color));
  if (return_value != MS_SUCCESS) return(MS_FAILURE);
  
  return_value = msCopyColor(&(dst->outlinecolor), &(src->outlinecolor));
  if (return_value != MS_SUCCESS) return(MS_FAILURE);
  
  return_value = msCopyColor(&(dst->shadowcolor), &(src->shadowcolor));
  if (return_value != MS_SUCCESS) return(MS_FAILURE);
  
  copyProperty(&(dst->shadowsizex), &(src->shadowsizex), sizeof(int));
  copyProperty(&(dst->shadowsizey), &(src->shadowsizey), sizeof(int));
  
  return_value = msCopyColor(&(dst->backgroundcolor),
                             &(src->backgroundcolor));
  if (return_value != MS_SUCCESS) return(MS_FAILURE);
  
  return_value = msCopyColor(&(dst->backgroundshadowcolor),
                             &(src->backgroundshadowcolor));
  if (return_value != MS_SUCCESS) return(MS_FAILURE);
  
  copyProperty(&(dst->backgroundshadowsizex),
               &(src->backgroundshadowsizex), sizeof(int));
  copyProperty(&(dst->backgroundshadowsizey),
               &(src->backgroundshadowsizey), sizeof(int));
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
  copyProperty(&(dst->minfeaturesize),&(src->minfeaturesize),sizeof(int));
  
  copyProperty(&(dst->autominfeaturesize),
               &(src->autominfeaturesize), sizeof(int));
  
  copyProperty(&(dst->mindistance), &(src->mindistance), sizeof(int));
  copyProperty(&(dst->partials), &(src->partials), sizeof(int));
  copyProperty(&(dst->force), &(src->force), sizeof(int));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyWeb()                                                         *
 *                                                                     *
 * Copy webObj, using msCopyRect(), msCreateHashTable(), and           *
 * msCopyHashTable()                                                   *
 **********************************************************************/

int msCopyWeb(webObj *dst, webObj *src, mapObj *map) {
  copyStringProperty(&(dst->log), src->log);
  copyStringProperty(&(dst->imagepath), src->imagepath);
  copyStringProperty(&(dst->imageurl), src->imageurl);
  copyProperty(&(dst->map), &map, sizeof(mapObj *));
#ifndef __cplusplus
  copyStringProperty(&(dst->template), src->template);
#else
  copyStringProperty(&(dst->_template), src->_template);
#endif
  copyStringProperty(&(dst->header), src->header);
  copyStringProperty(&(dst->footer), src->footer);
  copyStringProperty(&(dst->empty), src->empty);
  copyStringProperty(&(dst->error), src->error);
  
  if (msCopyRect(&(dst->extent), &(src->extent)) == MS_FAILURE) {
    msSetError(MS_MEMERR, "Failed to copy extent.", "msCopyWeb()");
    return(MS_FAILURE);
  }
  
  copyProperty(&(dst->minscale), &(src->minscale), sizeof(double));
  copyProperty(&(dst->maxscale), &(src->maxscale), sizeof(double));
  copyStringProperty(&(dst->mintemplate), src->mintemplate);
  copyStringProperty(&(dst->maxtemplate), src->maxtemplate);
  
  if (src->metadata)
  {
    dst->metadata = msCreateHashTable();
    if (msCopyHashTable((dst->metadata), (src->metadata)) != MS_SUCCESS)
      return(MS_FAILURE);
  }

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyStyle()                                                       *
 *                                                                     *
 * Copy a styleObj, using msCopyColor()                                *
 **********************************************************************/

int msCopyStyle(styleObj *dst, styleObj *src) {
  int return_value;
  
  return_value = msCopyColor(&(dst->color), &(src->color));
  if (return_value != MS_SUCCESS) return(MS_FAILURE);
  
  return_value = msCopyColor(&(dst->outlinecolor),&(src->outlinecolor));
  if (return_value != MS_SUCCESS) return(MS_FAILURE);
  
  return_value = msCopyColor(&(dst->backgroundcolor),
                             &(src->backgroundcolor));
  if (return_value != MS_SUCCESS) return(MS_FAILURE);
  
  copyProperty(&(dst->symbol), &(src->symbol), sizeof(int));
  copyStringProperty(&(dst->symbolname), src->symbolname);
  copyProperty(&(dst->size), &(src->size), sizeof(int));
  copyProperty(&(dst->sizescaled), &(src->sizescaled), sizeof(int));
  copyProperty(&(dst->minsize), &(src->minsize), sizeof(int));
  copyProperty(&(dst->maxsize), &(src->maxsize), sizeof(int));
  copyProperty(&(dst->offsetx), &(src->offsetx), sizeof(int));
  copyProperty(&(dst->offsety), &(src->offsety), sizeof(int));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyClass()                                                       *
 *                                                                     *
 * Copy a classObj, using msCopyExpression(), msCopyStyle(),           *
 * msCopyLabel(), msCreateHashTable(), msCopyHashTable()               *
 **********************************************************************/

int msCopyClass(classObj *dst, classObj *src, layerObj *layer) {
  int i, return_value;
  
  return_value = msCopyExpression(&(dst->expression),&(src->expression));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy expression.", "msCopyClass()");
    return(MS_FAILURE);
  }
  
  copyProperty(&(dst->status), &(src->status), sizeof(int));
  memcpy(&(dst->status), &(src->status), sizeof(int));

  copyProperty(&(dst->numstyles), &(src->numstyles), sizeof(int));

  for (i = 0; i < dst->numstyles; i++) {
    if (msCopyStyle(&(dst->styles[i]), &(src->styles[i])) != MS_SUCCESS)
    {
      msSetError(MS_MEMERR, "Failed to copy style.", "msCopyClass()");
      return(MS_FAILURE);
    }
  }
  
  if (msCopyLabel(&(dst->label), &(src->label)) != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy label.", "msCopyClass()");
    return(MS_FAILURE);
  }
  
  copyStringProperty(&(dst->keyimage), src->keyimage);
  copyStringProperty(&(dst->name), src->name);
  copyStringProperty(&(dst->title), src->title);

  if (msCopyExpression(&(dst->text), &(src->text)) != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy text.", "msCopyClass()");
    return(MS_FAILURE);
  }

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

  copyProperty(&(dst->minscale), &(src->minscale), sizeof(double));
  copyProperty(&(dst->maxscale), &(src->maxscale), sizeof(double));
  copyProperty(&(dst->layer), &layer, sizeof(layerObj *));
  copyProperty(&(dst->debug), &(src->debug), sizeof(int));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyLabelCacheMember()                                            *
 *                                                                     *
 * Copy a labelCacheMemberObj using msCopyStyle(), msCopyPoint()       *
 *                                                                     *
 * Note: since it seems most users will want to clone maps rather than *
 * make exact copies, this method might not get much use.              *
 **********************************************************************/

int msCopyLabelCacheMember(labelCacheMemberObj *dst,
                           labelCacheMemberObj *src)
{
  int i;
  copyStringProperty(&(dst->string), src->string);
  copyProperty(&(dst->featuresize),&(src->featuresize),sizeof(double));
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

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyMarkerCacheMember()                                           *
 *                                                                     *
 * Copy a markerCacheMemberObj                                         *
 **********************************************************************/

int msCopyMarkerCacheMember(markerCacheMemberObj *dst,
                            markerCacheMemberObj *src) 
{
  copyProperty(&(dst->id), &(src->id), sizeof(int));
  //msCopyShape(&(dst->poly), &(src->poly));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyLabelCache()                                                  *
 **********************************************************************/

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
  
  copyProperty(&(dst->markercachesize),
               &(src->markercachesize), sizeof(int));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyMarkerCacheMember()                                           *
 *                                                                     *
 * Copy a markerCacheMemberObj                                         *
 **********************************************************************/

int msCopyResultCacheMember(resultCacheMemberObj *dst,
                            resultCacheMemberObj *src)
{
  copyProperty(&(dst->shapeindex), &(src->shapeindex), sizeof(long));
  copyProperty(&(dst->tileindex), &(src->tileindex), sizeof(int));
  copyProperty(&(dst->classindex), &(src->classindex), sizeof(int));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyResultCache()                                                 *
 **********************************************************************/

int msCopyResultCache(resultCacheObj *dst, resultCacheObj *src) {
  int i;
  copyProperty(&(dst->cachesize), &(src->cachesize), sizeof(int));
  copyProperty(&(dst->numresults), &(src->numresults), sizeof(int));
  for (i = 0; i < dst->numresults; i++) {
    msCopyResultCacheMember(&(dst->results[i]), &(src->results[i]));
  }
  msCopyRect(&(dst->bounds), &(src->bounds));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopySymbol()                                                      *
 *                                                                     *
 * Copy a symbolObj, using mapfile.c:initSymbol(), msCopyPoint()       *
 * gdImageCreate(), gdImageCopy()                                      *
 **********************************************************************/

int msCopySymbol(symbolObj *dst, symbolObj *src, mapObj *map) {
  int i;
  initSymbol(dst);
  copyStringProperty(&(dst->name), src->name);
  copyProperty(&(dst->type), &(src->type), sizeof(int));
  copyProperty(&(dst->inmapfile), &(src->inmapfile), sizeof(int));
  copyProperty(&(dst->map), &map, sizeof(mapObj *));
  copyProperty(&(dst->sizex), &(src->sizex), sizeof(double)),
  copyProperty(&(dst->sizey), &(src->sizey), sizeof(double));
  for (i=0; i < MS_MAXVECTORPOINTS; i++) {
    if (msCopyPoint(&(dst->points[i]), &(src->points[i])) != MS_SUCCESS)
    {
      msSetError(MS_MEMERR, "Failed to copy point.", "msCopySymbol()");
      return(MS_FAILURE);
    }
  }
  copyProperty(&(dst->numpoints), &(src->numpoints), sizeof(int));
  copyProperty(&(dst->filled), &(src->filled), sizeof(int));
  copyProperty(&(dst->stylelength), &(src->stylelength), sizeof(int));
  
  copyProperty(&(dst->style), &(src->style),
               sizeof(int)*MS_MAXSTYLELENGTH);
  
  //gdImagePtr img;
  if (src->img) {
     if (dst->img) {
       gdFree(dst->img);
     }
     dst->img = gdImageCreate(src->img->sx, src->img->sy);
     gdImageCopy(dst->img, src->img, 0, 0, 0, 0,
                 src->img->sx, src->img->sy);
  }

  copyStringProperty(&(dst->imagepath), src->imagepath);
  copyProperty(&(dst->transparent), &(src->transparent),sizeof(int));
  
  copyProperty(&(dst->transparentcolor), &(src->transparentcolor),
               sizeof(int));
  
  copyStringProperty(&(dst->character), src->character);
  copyProperty(&(dst->antialias), &(src->antialias), sizeof(int));
  copyStringProperty(&(dst->font), src->font);
  copyProperty(&(dst->gap), &(src->gap), sizeof(int));
  copyProperty(&(dst->position), &(src->position), sizeof(int));
  copyProperty(&(dst->linecap), &(src->linecap), sizeof(int));
  copyProperty(&(dst->linejoin), &(src->linejoin), sizeof(int));
  
  copyProperty(&(dst->linejoinmaxsize), &(src->linejoinmaxsize),
               sizeof(double));

  return(MS_SUCCESS);
} 

/***********************************************************************
 * msCopySymbolSet()                                                   *
 *                                                                     *
 * Copy a symbolSetObj using msCopyFontSet(), msCopySymbol()           *
 **********************************************************************/

int msCopySymbolSet(symbolSetObj *dst, symbolSetObj *src, mapObj *map)
{
  int i, return_value;
  
  copyStringProperty(&(dst->filename), src->filename);
  copyProperty(&(dst->map), &map, sizeof(mapObj *));

  if (msCopyFontSet(dst->fontset, src->fontset, map) != MS_SUCCESS) {
    msSetError(MS_MEMERR,"Failed to copy fontset.","msCopySymbolSet()");
    return(MS_FAILURE);
  }
  
  copyProperty(&(dst->numsymbols), &(src->numsymbols), sizeof(int));
  
  for (i = 0; i < dst->numsymbols; i++) {
    return_value = msCopySymbol(&(dst->symbol[i]), &(src->symbol[i]), map);
    if (return_value != MS_SUCCESS) {
      msSetError(MS_MEMERR,"Failed to copy symbol.","msCopySymbolSet()");
      return(MS_FAILURE);
    }
  }

  copyProperty(&(dst->imagecachesize),
               &(src->imagecachesize), sizeof(int));
  
  // I have a feeling that the code below is not quite right - Sean
  copyProperty(&(dst->imagecache), &(src->imagecache),
               sizeof(struct imageCacheObj));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyReferenceMap()                                                *
 *                                                                     *
 * Copy a referenceMapObj using mapfile.c:initReferenceMap(),          *
 * msCopyRect(), msCopyColor()                                         *
 **********************************************************************/

int msCopyReferenceMap(referenceMapObj *dst, referenceMapObj *src,
                       mapObj *map)
{
  int return_value;

  initReferenceMap(dst);

  return_value = msCopyRect(&(dst->extent), &(src->extent));
  if (return_value != MS_SUCCESS)
  {
    msSetError(MS_MEMERR, "Failed to copy extent.",
               "msCopyReferenceMap()");
    return(MS_FAILURE);
  }

  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));

  return_value = msCopyColor(&(dst->color), &(src->color));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy color.",
               "msCopyReferenceMap()");
    return(MS_FAILURE);
  }

  return_value = msCopyColor(&(dst->outlinecolor),&(src->outlinecolor));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy outlinecolor.",
               "msCopyReferenceMap()");
    return(MS_FAILURE);
  }

  copyStringProperty(&(dst->image), src->image);
  copyProperty(&(dst->status), &(src->status), sizeof(int));
  copyProperty(&(dst->marker), &(src->marker), sizeof(int));
  copyStringProperty(&(dst->markername), src->markername);
  copyProperty(&(dst->markersize), &(src->markersize), sizeof(int));
  copyProperty(&(dst->minboxsize), &(src->minboxsize), sizeof(int));
  copyProperty(&(dst->maxboxsize), &(src->maxboxsize), sizeof(int));
  copyProperty(&(dst->map), &map, sizeof(mapObj *));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyScalebar()                                                    *
 *                                                                     *
 * Copy a scalebarObj, using initScalebar(), msCopyColor(),            *
 * and msCopyLabel()                                                   *
 **********************************************************************/

int msCopyScalebar(scalebarObj *dst, scalebarObj *src)
{
  int return_value;
  
  initScalebar(dst);
  
  return_value = msCopyColor(&(dst->imagecolor), &(src->imagecolor));
  if (return_value != MS_SUCCESS)
  {
    msSetError(MS_MEMERR,"Failed to copy imagecolor.",
               "msCopyScalebar()");
    
    return(MS_FAILURE);
  }
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  copyProperty(&(dst->style), &(src->style), sizeof(int));
  copyProperty(&(dst->intervals), &(src->intervals), sizeof(int));
  
  if (msCopyLabel(&(dst->label), &(src->label)) != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy label.","msCopyScalebar()");
    return(MS_FAILURE);
  }
  
  return_value = msCopyColor(&(dst->color), &(src->color));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy color.",
               "msCopyScalebar()");
    return(MS_FAILURE);
  }
  
  return_value = msCopyColor(&(dst->backgroundcolor),
                             &(src->backgroundcolor));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy backgroundcolor.",
               "msCopyScalebar()");
    return(MS_FAILURE);
  }
  
  return_value = msCopyColor(&(dst->outlinecolor),
                             &(src->outlinecolor));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy outlinecolor.",
               "msCopyScalebar()");
    return(MS_FAILURE);
  }
  
  copyProperty(&(dst->units), &(src->units), sizeof(int));
  copyProperty(&(dst->status), &(src->status), sizeof(int));
  copyProperty(&(dst->position), &(src->position), sizeof(int));
  copyProperty(&(dst->transparent), &(src->transparent), sizeof(int));
  copyProperty(&(dst->interlace), &(src->interlace), sizeof(int));
  
  copyProperty(&(dst->postlabelcache), &(src->postlabelcache),
               sizeof(int));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyLegend()                                                      *
 *                                                                     *
 * Copy a legendObj, using msCopyColor()                               *
 **********************************************************************/

int msCopyLegend(legendObj *dst, legendObj *src, mapObj *map)
{
  int return_value;
  
  return_value = msCopyColor(&(dst->imagecolor), &(src->imagecolor));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy imagecolor.",
               "msCopyLegend()");
    return(MS_FAILURE);
  }

  return_value = msCopyLabel(&(dst->label), &(src->label));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy label.",
               "msCopyLegend()");
    return(MS_FAILURE);
  }

  copyProperty(&(dst->keysizex), &(src->keysizex), sizeof(int));
  copyProperty(&(dst->keysizey), &(src->keysizey), sizeof(int));
  copyProperty(&(dst->keyspacingx), &(src->keyspacingx), sizeof(int));
  copyProperty(&(dst->keyspacingy), &(src->keyspacingy), sizeof(int));

  return_value = msCopyColor(&(dst->outlinecolor),&(src->outlinecolor));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy outlinecolor.",
               "msCopyLegend()");
    return(MS_FAILURE);
  }

  copyProperty(&(dst->status), &(src->status), sizeof(int));
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  copyProperty(&(dst->position), &(src->position), sizeof(int));
  copyProperty(&(dst->transparent), &(src->transparent), sizeof(int));
  copyProperty(&(dst->interlace), &(src->interlace), sizeof(int));

  copyProperty(&(dst->postlabelcache), &(src->postlabelcache),
               sizeof(int));

#ifndef __cplusplus
   copyStringProperty(&(dst->template), src->template);
#else
   copyStringProperty(&(dst->_template), src->_template);
#endif
  copyProperty(&(dst->map), &map, sizeof(mapObj *));

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyLayer()                                                       *
 *                                                                     *
 * Copy a layerObj, using mapfile.c:initClass(), msCopyClass(),        *
 * msCopyColor(), msCopyProjection(), msSHPOpenFile(),                 *
 * msCreateHashTable(), msCopyHashTable(), msCopyExpression()          *
 *                                                                     *
 * As it stands, we are not copying a layer's resultcache              *
 **********************************************************************/

int msCopyLayer(layerObj *dst, layerObj *src)
{
  int i, return_value;
  featureListNodeObjPtr current;
//  char szPath[MS_MAXPATHLEN];

  copyProperty(&(dst->index), &(src->index), sizeof(int));
  copyStringProperty(&(dst->classitem), src->classitem);

  copyProperty(&(dst->classitemindex), &(src->classitemindex),
               sizeof(int));

  copyProperty(&(dst->numclasses), &(src->numclasses), sizeof(int));

  for (i = 0; i < dst->numclasses; i++) {
#ifndef __cplusplus
    initClass(&(dst->class[i]));

    return_value = msCopyClass(&(dst->class[i]), &(src->class[i]), dst);
    if (return_value != MS_SUCCESS) {
      msSetError(MS_MEMERR, "Failed to copy class.", "msCopyLayer()");
      return(MS_FAILURE);
    }
#else
    initClass(&(dst->_class[i]));

    return_value = msCopyClass(&(dst->_class[i]), &(src->_class[i]), dst);
    if (return_value != MS_SUCCESS) {
      msSetError(MS_MEMERR, "Failed to copy _class.", "msCopyLayer()");
      return(MS_FAILURE);
    }
#endif
  }
  copyStringProperty(&(dst->header), src->header);
  copyStringProperty(&(dst->footer), src->footer);
#ifndef __cplusplus
  copyStringProperty(&(dst->template), src->template);
#else
  copyStringProperty(&(dst->_template), src->_template);
#endif
  
  //copyProperty(&(dst->resultcache), &(src->resultcache),
  //sizeof(resultCacheObj *));
  //
  copyStringProperty(&(dst->name), src->name); 
  copyStringProperty(&(dst->group), src->group); 
  copyProperty(&(dst->status), &(src->status), sizeof(int)); 
  copyStringProperty(&(dst->data), src->data); 
  copyProperty(&(dst->type), &(src->type), sizeof(enum MS_LAYER_TYPE));
  copyProperty(&(dst->annotate), &(src->annotate), sizeof(int)); 
  copyProperty(&(dst->tolerance), &(src->tolerance), sizeof(double)); 

  copyProperty(&(dst->toleranceunits), &(src->toleranceunits),
               sizeof(int));

  copyProperty(&(dst->symbolscale), &(src->symbolscale), sizeof(double)); 
  copyProperty(&(dst->scalefactor), &(src->scalefactor), sizeof(double)); 
  copyProperty(&(dst->minscale), &(src->minscale), sizeof(double));
  copyProperty(&(dst->maxscale), &(src->maxscale), sizeof(double));

  copyProperty(&(dst->labelminscale), &(src->labelminscale),
               sizeof(double));

  copyProperty(&(dst->labelmaxscale), &(src->labelmaxscale),
               sizeof(double));

  copyProperty(&(dst->sizeunits), &(src->sizeunits), sizeof(int)); 
  copyProperty(&(dst->maxfeatures), &(src->maxfeatures), sizeof(int));

  if (msCopyColor(&(dst->offsite), &(src->offsite)) != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy offsite.", "msCopyLayer()");
    return (MS_FAILURE);
  }

  copyProperty(&(dst->transform), &(src->transform), sizeof(int)); 
  copyProperty(&(dst->labelcache), &(src->labelcache), sizeof(int));

  copyProperty(&(dst->postlabelcache), &(src->postlabelcache),
               sizeof(int)); 

  copyStringProperty(&(dst->labelitem), src->labelitem);
  copyStringProperty(&(dst->labelsizeitem), src->labelsizeitem);
  copyStringProperty(&(dst->labelangleitem), src->labelangleitem);

  copyProperty(&(dst->labelitemindex), &(src->labelitemindex),
               sizeof(int));

  copyProperty(&(dst->labelsizeitemindex), &(src->labelsizeitemindex),
               sizeof(int));

  copyProperty(&(dst->labelangleitemindex), &(src->labelangleitemindex),
               sizeof(int));

  copyStringProperty(&(dst->tileitem), src->tileitem);
  copyProperty(&(dst->tileitemindex), &(src->tileitemindex),
               sizeof(int));

  copyStringProperty(&(dst->tileindex), src->tileindex); 

  return_value = msCopyProjection(&(dst->projection),&(src->projection));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy projection.", "msCopyLayer()");
    return(MS_FAILURE);
  }

  copyProperty(&(dst->project), &(src->project), sizeof(int)); 
  copyProperty(&(dst->units), &(src->units), sizeof(int)); 

  current = src->features;
  while(current != NULL) {
    insertFeatureList(&(dst->features), &(current->shape));
    current = current->next;
  }

  copyStringProperty(&(dst->connection), src->connection);
  copyProperty(&(dst->connectiontype), &(src->connectiontype),
               sizeof(enum MS_CONNECTION_TYPE));

  copyProperty(&(dst->sameconnection), &(src->sameconnection),
               sizeof(layerObj *));
  
  //msSHPOpenFile(&(dst->shpfile), "rb", msBuildPath(szPath, src->map->shapepath, src->data));
  //msSHPOpenFile(&(dst->tileshpfile), "rb", msBuildPath(szPath, src->map->shapepath, src->tileindex));

  copyProperty(&(dst->layerinfo), &(src->layerinfo), sizeof(void));

  copyProperty(&(dst->ogrlayerinfo),&(src->ogrlayerinfo),sizeof(void)); 
  copyProperty(&(dst->wfslayerinfo), &(src->wfslayerinfo),
               sizeof(void)); 

  copyProperty(&(dst->numitems), &(src->numitems), sizeof(int));

  for (i = 0; i < dst->numitems; i++) {
    copyStringProperty(&(dst->items[i]), src->items[i]);
  }

  copyProperty(&(dst->iteminfo), &(src->iteminfo), sizeof(void)); 

  return_value = msCopyExpression(&(dst->filter), &(src->filter));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy filter.", "msCopyLayer()");
    return(MS_FAILURE);
  }

  copyStringProperty(&(dst->filteritem), src->filteritem);
  copyProperty(&(dst->filteritemindex), &(src->filteritemindex),
               sizeof(int));
  
  copyStringProperty(&(dst->styleitem), src->styleitem); 
  copyProperty(&(dst->styleitemindex), &(src->styleitemindex),
               sizeof(int));
  
  copyStringProperty(&(dst->requires), src->requires); 
  copyStringProperty(&(dst->labelrequires), src->labelrequires);

  if (src->metadata)
  {
    dst->metadata = msCreateHashTable();
    msCopyHashTable((dst->metadata), (src->metadata));
  }

  copyProperty(&(dst->transparency), &(src->transparency), sizeof(int)); 
  copyProperty(&(dst->dump), &(src->dump), sizeof(int));
  copyProperty(&(dst->debug), &(src->debug), sizeof(int));
  copyProperty(&(dst->num_processing), &(src->num_processing),
               sizeof(int));
  
  for (i = 0; i < dst->num_processing; i++) {
    copyStringProperty(&(dst->processing[i]), src->processing[i]);
  }

  copyProperty(&(dst->numjoins), &(src->numjoins), sizeof(int));

  for (i = 0; i < dst->num_processing; i++) {
    return_value = msCopyJoin(&(dst->joins[i]), &(src->joins[i]));
    if (return_value != MS_SUCCESS) return(MS_FAILURE);
  }

  return(MS_SUCCESS);
}

/***********************************************************************
 * msCopyMap()                                                         *
 *                                                                     *
 * Copy a mapObj, using mapfile.c:initLayer(), msCopyLayer(),          *
 * msCopySymbolSet(), msCopyRect(), msCopyColor(), msCopyQueryMap()    *
 * msCopyLegend(), msCopyScalebar(), msCopyProjection()                *
 * msCopyOutputFormat(), msCopyWeb(), msCopyReferenceMap()             *
 **********************************************************************/

int msCopyMap(mapObj *dst, mapObj *src)
{
  int i, return_value;
  
  copyStringProperty(&(dst->name), src->name); 
  copyProperty(&(dst->status), &(src->status), sizeof(int)); 
  copyProperty(&(dst->height), &(src->height), sizeof(int));
  copyProperty(&(dst->width), &(src->width), sizeof(int));
  copyProperty(&(dst->numlayers), &(src->numlayers), sizeof(int)); 

  for (i = 0; i < dst->numlayers; i++) {
    initLayer(&(dst->layers[i]), dst);
    
    return_value = msCopyLayer(&(dst->layers[i]), &(src->layers[i]));
    if (return_value != MS_SUCCESS) {
      msSetError(MS_MEMERR, "Failed to copy layer.", "msCopyMap()");
      return(MS_FAILURE);
    }
  }
  
  return_value = msCopySymbolSet(&(dst->symbolset), &(src->symbolset),
                                 dst);
  if(return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy symbolset.", "msCopyMap()");
    return(MS_FAILURE);
  }
  
  if (msCopyFontSet(&(dst->fontset), &(src->fontset), dst) != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy fontset.", "msCopyMap()");
    return(MS_FAILURE);
  }
  
  //msCopyLabelCache(&(dst->labelcache), &(src->labelcache));
  copyProperty(&(dst->transparent), &(src->transparent), sizeof(int)); 
  copyProperty(&(dst->interlace), &(src->interlace), sizeof(int)); 
  copyProperty(&(dst->imagequality), &(src->imagequality), sizeof(int)); 

  if (msCopyRect(&(dst->extent), &(src->extent)) != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy extent.", "msCopyMap()");
    return(MS_FAILURE);
  }

  copyProperty(&(dst->cellsize), &(src->cellsize), sizeof(double)); 
  copyProperty(&(dst->units), &(src->units), sizeof(enum MS_UNITS));
  copyProperty(&(dst->scale), &(src->scale), sizeof(double)); 
  copyProperty(&(dst->resolution), &(src->resolution), sizeof(int));
  copyStringProperty(&(dst->shapepath), src->shapepath); 
  copyStringProperty(&(dst->mappath), src->mappath); 

  return_value = msCopyColor(&(dst->imagecolor), &(src->imagecolor));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy imagecolor.", "msCopyMap()");
    return(MS_FAILURE);
  }

  copyProperty(&(dst->numoutputformats), &(src->numoutputformats),
               sizeof(int));

  return_value = msCopyOutputFormat(dst->outputformat,src->outputformat);
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy outputformat.","msCopyMap()");
    return(MS_FAILURE);
  }

  for (i = 0; i < src->numoutputformats; i++) {
    return_value = msCopyOutputFormat(dst->outputformatlist[i],
                                      src->outputformatlist[i]);
    
    if (return_value != MS_SUCCESS) {
      msSetError(MS_MEMERR, "Failed to copy outputformat.",
                 "msCopyMap()");
      
      return(MS_FAILURE);
    }
  }

  copyStringProperty(&(dst->imagetype), src->imagetype); 
  
  return_value = msCopyProjection(&(dst->projection),&(src->projection));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy projection.", "msCopyMap()");
    return(MS_FAILURE);
  }
  
  return_value = msCopyProjection(&(dst->latlon), &(src->latlon));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy latlon.", "msCopyMap()");
    return(MS_FAILURE);
  }
  
  return_value = msCopyReferenceMap(&(dst->reference),&(src->reference),
                                    dst);
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy reference.", "msCopyMap()");
    return(MS_FAILURE);
  }
  
  return_value = msCopyScalebar(&(dst->scalebar), &(src->scalebar));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy scalebar.", "msCopyMap()");
    return(MS_FAILURE);
  }
  
  return_value = msCopyLegend(&(dst->legend), &(src->legend),dst);
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy legend.", "msCopyMap()");
    return(MS_FAILURE);
  }
  
  return_value = msCopyQueryMap(&(dst->querymap), &(src->querymap));
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy querymap.", "msCopyMap()");
    return(MS_FAILURE);
  }
  
  return_value = msCopyWeb(&(dst->web), &(src->web), dst);
  if (return_value != MS_SUCCESS) {
    msSetError(MS_MEMERR, "Failed to copy web.", "msCopyMap()");
    return(MS_FAILURE);
  }
  
  for (i = 0; i < dst->numlayers; i++) {
    copyProperty(&(dst->layerorder[i]), &(src->layerorder[i]), sizeof(int));
  }
  copyProperty(&(dst->debug), &(src->debug), sizeof(int));
  copyStringProperty(&(dst->datapattern), src->datapattern);
  copyStringProperty(&(dst->templatepattern), src->templatepattern);   

  return(MS_SUCCESS);
}


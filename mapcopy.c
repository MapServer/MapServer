/******************************************************************************
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
 * Revision 1.46  2006/05/26 21:46:27  tamas
 * Moving layerObj.sameconnection and msCheckConnection() internal to the MYGIS data provider.
 *
 * Revision 1.45  2005/12/14 19:13:47  sdlime
 * Patched mapfile read/write/copy routines to deal with browseformat and legendformat.
 *
 * Revision 1.44  2005/11/28 04:32:42  sdlime
 * Updated msCopyLabel() to copy encoding as well.
 *
 * Revision 1.43  2005/11/28 04:22:54  sdlime
 * Removed some unnecessary and potentially harmful if statements in msCopyStyle(). The macros should be cufficient.
 *
 * Revision 1.42  2005/06/29 14:14:23  sean
 * no longer copying layer and join items, as these will be initialized on opening of layer and joining of data (bug 1403)
 *
 * Revision 1.41  2005/06/28 16:33:23  sean
 * Fixed copy of layer processing directives with use of msLayerGetProcessing and msLayerAddProcessing (bug 1399). Also caught error in the upper bounds of looping over layer joins.
 *
 * Revision 1.40  2005/06/14 16:03:33  dan
 * Updated copyright date to 2005
 *
 * Revision 1.39  2005/04/25 06:41:55  sdlime
 * Applied Bill's newest gradient patch, more concise in the mapfile and potential to use via MapScript.
 *
 * Revision 1.38  2005/04/15 18:44:31  sdlime
 * Updated msCopyStyle to account for the various item indexes.
 *
 * Revision 1.37  2005/04/15 17:50:20  sdlime
 * Updated style copy function to handle the new gradient parameters (bug 1305).
 *
 * Revision 1.36  2005/02/18 03:06:45  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.35  2005/01/26 06:14:53  sdlime
 * Added style maxwidth/minwidth read/write/copy.
 *
 * Revision 1.34  2005/01/26 05:21:19  sdlime
 * Added support for reading/writing/copying a style width.
 *
 * Revision 1.33  2004/11/15 19:16:53  sean
 * Do not copy layerinfo, wfslayerinfo, or ogrlayerinfo in msCopyLayer (bug 1055).
 *
 * Revision 1.32  2004/10/21 04:30:55  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#include <assert.h>
#include "map.h"
#include "mapsymbol.h"

MS_CVSID("$Id$")

#include "mapcopy.h"

/***********************************************************************
 * msCopyProjection()                                                  *
 *                                                                     *
 * Copy a projectionObj                                                *
 **********************************************************************/

int msCopyProjection(projectionObj *dst, projectionObj *src) {

#ifdef USE_PROJ
    int i;
    
    MS_COPYSTELEM(numargs);

    for (i = 0; i < dst->numargs; i++) {
        /* Our destination consists of unallocated pointers */
        dst->args[i] = strdup(src->args[i]);
    }
    if (dst->numargs != 0) {
        if (msProcessProjection(dst) != MS_SUCCESS)
            return MS_FAILURE;

    }
#endif
    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyLine()                                                        *
 *                                                                     *
 * Copy a lineObj, using msCopyPoint()                                 *
 **********************************************************************/
int msCopyLine(lineObj *dst, lineObj *src) {
  
    int i;

    dst->numpoints = src->numpoints;
    for (i = 0; i < dst->numpoints; i++) {
        MS_COPYPOINT(&(dst->point[i]), &(src->point[i]));
    }

    return MS_SUCCESS;
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
  copyStringPropertyRealloc(&(dst->text), src->text);
  copyProperty(&(dst->numvalues), &(src->numvalues), sizeof(int));
  for (i = 0; i < dst->numvalues; i++) {
    copyStringPropertyRealloc(&(dst->values[i]), src->values[i]);
  }

  return(0);
}
*/

/**********************************************************************
 * msCopyItem()                                                        *
 *                                                                     *
 * Copy an itemObj                                                     *
 **********************************************************************/

int msCopyItem(itemObj *dst, itemObj *src) {
    
    MS_COPYSTRING(dst->name, src->name);
    MS_COPYSTELEM(type);
    MS_COPYSTELEM(index);
    MS_COPYSTELEM(size);
    MS_COPYSTELEM(numdecimals);

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyHashTable()                                                   *
 *                                                                     *
 * Copy a hashTableObj, using msInsertHashTable()                      *
 **********************************************************************/

int msCopyHashTable(hashTableObj *dst, hashTableObj *src) {
    const char *key=NULL;
    while (1) {
        key = msNextKeyFromHashTable(src, key);
        if (!key) 
            break;
        else 
            msInsertHashTable(dst, key, msLookupHashTable(src, key));
    }
    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyFontSet()                                                     *
 *                                                                     *
 * Copy a fontSetObj, using msCreateHashTable() and msCopyHashTable()  *
 **********************************************************************/

int msCopyFontSet(fontSetObj *dst, fontSetObj *src, mapObj *map)
{

    MS_COPYSTRING(dst->filename, src->filename);
    MS_COPYSTELEM(numfonts);
    if (&(src->fonts)) {
        /* if (!dst->fonts) */
        /* dst->fonts = msCreateHashTable(); */
        if (msCopyHashTable(&(dst->fonts), &(src->fonts)) != MS_SUCCESS)
            return MS_FAILURE;
    }

    dst->map = map;

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyExpression(                                                   *
 *                                                                     *
 * Copy an expressionObj, but only its string and type                 *
 **********************************************************************/

int msCopyExpression(expressionObj *dst, expressionObj *src)
{
    MS_COPYSTRING(dst->string, src->string);
    MS_COPYSTELEM(type);
    dst->compiled = MS_FALSE;

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyJoin()                                                        *
 *                                                                     *
 * Copy a joinObj                                                      *
 **********************************************************************/

int msCopyJoin(joinObj *dst, joinObj *src)
{
    MS_COPYSTRING(dst->name, src->name);

    /* makes no sense to copy the items or values
       since they are runtime additions to the mapfile */

    MS_COPYSTRING(dst->table, src->table);
    MS_COPYSTRING(dst->from, src->from);
    MS_COPYSTRING(dst->to, src->to);
    MS_COPYSTRING(dst->header, src->header);
#ifndef __cplusplus
    MS_COPYSTRING(dst->template, src->template);
#else
    MS_COPYSTRING(dst->_template, src->_template);
#endif
    MS_COPYSTRING(dst->footer, src->footer);
    dst->type = src->type;
    MS_COPYSTRING(dst->connection, src->connection);

    MS_COPYSTELEM(connectiontype);

    /* TODO: need to handle joininfo (probably should be just set to NULL) */
    dst->joininfo = NULL;

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyQueryMap()                                                    *
 *                                                                     *
 * Copy a queryMapObj, using msCopyColor()                             *
 **********************************************************************/

int msCopyQueryMap(queryMapObj *dst, queryMapObj *src) 
{
    MS_COPYSTELEM(height);
    MS_COPYSTELEM(width);
    MS_COPYSTELEM(status);
    MS_COPYSTELEM(style);
    MS_COPYCOLOR(&(dst->color), &(src->color));

  return MS_SUCCESS;
}

/***********************************************************************
 * msCopyLabel()                                                       *
 *                                                                     *
 * Copy a labelObj, using msCopyColor()                                *
 **********************************************************************/

int msCopyLabel(labelObj *dst, labelObj *src) 
{

    MS_COPYSTRING(dst->font, src->font);
    MS_COPYSTELEM(type);

    MS_COPYCOLOR(&(dst->color), &(src->color));
    MS_COPYCOLOR(&(dst->outlinecolor), &(src->outlinecolor));
    MS_COPYCOLOR(&(dst->shadowcolor), &(src->shadowcolor));

    MS_COPYSTELEM(shadowsizex);
    MS_COPYSTELEM(shadowsizey);

    MS_COPYCOLOR(&(dst->backgroundcolor), &(src->backgroundcolor));
    MS_COPYCOLOR(&(dst->backgroundshadowcolor), &(src->backgroundshadowcolor));

    MS_COPYSTELEM(backgroundshadowsizex);
    MS_COPYSTELEM(backgroundshadowsizey);
    MS_COPYSTELEM(size);
    MS_COPYSTELEM(minsize);
    MS_COPYSTELEM(maxsize);
    MS_COPYSTELEM(position);
    MS_COPYSTELEM(offsetx);
    MS_COPYSTELEM(offsety);
    MS_COPYSTELEM(angle);
    MS_COPYSTELEM(autoangle);
    MS_COPYSTELEM(autofollow);
    MS_COPYSTELEM(buffer);
    MS_COPYSTELEM(antialias);
    MS_COPYSTELEM(wrap);
    MS_COPYSTELEM(minfeaturesize);

    MS_COPYSTELEM(autominfeaturesize);

    MS_COPYSTELEM(mindistance);
    MS_COPYSTELEM(partials);
    MS_COPYSTELEM(force);

    MS_COPYSTRING(dst->encoding, src->encoding);

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyWeb()                                                         *
 *                                                                     *
 * Copy webObj, using msCopyRect(), msCreateHashTable(), and           *
 * msCopyHashTable()                                                   *
 **********************************************************************/

int msCopyWeb(webObj *dst, webObj *src, mapObj *map) 
{

    MS_COPYSTRING(dst->log, src->log);
    MS_COPYSTRING(dst->imagepath, src->imagepath);
    MS_COPYSTRING(dst->imageurl, src->imageurl);
    dst->map = map;
#ifndef __cplusplus
    MS_COPYSTRING(dst->template, src->template);
#else
    MS_COPYSTRING(dst->_template, src->_template);
#endif
    MS_COPYSTRING(dst->header, src->header);
    MS_COPYSTRING(dst->footer, src->footer);
    MS_COPYSTRING(dst->empty, src->empty);
    MS_COPYSTRING(dst->error, src->error);

    MS_COPYRECT(&(dst->extent), &(src->extent));

    MS_COPYSTELEM(minscale);
    MS_COPYSTELEM(maxscale);
    MS_COPYSTRING(dst->mintemplate, src->mintemplate);
    MS_COPYSTRING(dst->maxtemplate, src->maxtemplate);

    if (&(src->metadata)) {
        /* dst->metadata = msCreateHashTable(); */
        if (msCopyHashTable(&(dst->metadata), &(src->metadata)) != MS_SUCCESS)
            return MS_FAILURE;
    }

    MS_COPYSTRING(dst->queryformat, src->queryformat);
    MS_COPYSTRING(dst->legendformat, src->legendformat);
    MS_COPYSTRING(dst->browseformat, src->browseformat);

    return MS_SUCCESS ;
}

/***********************************************************************
 * msCopyStyle()                                                       *
 *                                                                     *
 * Copy a styleObj, using msCopyColor()                                *
 **********************************************************************/

int msCopyStyle(styleObj *dst, styleObj *src) 
{

    MS_COPYCOLOR(&(dst->color), &(src->color));
    MS_COPYCOLOR(&(dst->outlinecolor),&(src->outlinecolor));
    MS_COPYCOLOR(&(dst->backgroundcolor), &(src->backgroundcolor));
   
    MS_COPYCOLOR(&(dst->mincolor), &(src->mincolor));
    MS_COPYCOLOR(&(dst->maxcolor), &(src->maxcolor));

    MS_COPYSTRING(dst->symbolname, src->symbolname);

    MS_COPYSTELEM(symbol);
    MS_COPYSTELEM(size);
    MS_COPYSTELEM(minsize);
    MS_COPYSTELEM(maxsize);
    MS_COPYSTELEM(width);
    MS_COPYSTELEM(minwidth);
    MS_COPYSTELEM(maxwidth);
    MS_COPYSTELEM(offsetx);
    MS_COPYSTELEM(offsety);
    MS_COPYSTELEM(antialias);
    MS_COPYSTELEM(isachild);
    MS_COPYSTELEM(angle);
    MS_COPYSTELEM(minvalue);
    MS_COPYSTELEM(maxvalue);

    MS_COPYSTRING(dst->angleitem, src->angleitem);
    MS_COPYSTRING(dst->sizeitem, src->sizeitem);
    MS_COPYSTRING(dst->rangeitem, src->rangeitem);

    MS_COPYSTELEM(angleitemindex);
    MS_COPYSTELEM(sizeitemindex);
    MS_COPYSTELEM(rangeitemindex);

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyClass()                                                       *
 *                                                                     *
 * Copy a classObj, using msCopyExpression(), msCopyStyle(),           *
 * msCopyLabel(), msCreateHashTable(), msCopyHashTable()               *
 **********************************************************************/

int msCopyClass(classObj *dst, classObj *src, layerObj *layer) 
{
    int i, return_value;

    return_value = msCopyExpression(&(dst->expression),&(src->expression));
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy expression.", "msCopyClass()");
        return MS_FAILURE;
    }

    MS_COPYSTELEM(status);
    MS_COPYSTELEM(numstyles);

    for (i = 0; i < dst->numstyles; i++) {
        if (msCopyStyle(&(dst->styles[i]), &(src->styles[i])) != MS_SUCCESS) {
            msSetError(MS_MEMERR, "Failed to copy style.", "msCopyClass()");
            return MS_FAILURE;
        }
    }

    if (msCopyLabel(&(dst->label), &(src->label)) != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy label.", "msCopyClass()");
        return MS_FAILURE;
    }

    MS_COPYSTRING(dst->keyimage, src->keyimage);
    MS_COPYSTRING(dst->name, src->name);
    MS_COPYSTRING(dst->title, src->title);

    if (msCopyExpression(&(dst->text), &(src->text)) != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy text.", "msCopyClass()");
        return MS_FAILURE;
    }

#ifndef __cplusplus
    MS_COPYSTRING(dst->template, src->template);
#else
    MS_COPYSTRING(dst->_template, src->_template);
#endif
    MS_COPYSTELEM(type);

    if (&(src->metadata) != NULL) {
        /* dst->metadata = msCreateHashTable(); */
        msCopyHashTable(&(dst->metadata), &(src->metadata));
    }

    MS_COPYSTELEM(minscale);
    MS_COPYSTELEM(maxscale);
    MS_COPYSTELEM(layer);
    MS_COPYSTELEM(debug);

    return MS_SUCCESS;
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

    MS_COPYSTRING(dst->text, src->text);
    MS_COPYSTELEM(featuresize);
    MS_COPYSTELEM(numstyles);

    for (i = 0; i < dst->numstyles; i++) {
        msCopyStyle(&(dst->styles[i]), &(src->styles[i]));
    }

    msCopyLabel(&(dst->label), &(src->label));
    MS_COPYSTELEM(layerindex);
    MS_COPYSTELEM(classindex);
    MS_COPYSTELEM(tileindex);
    MS_COPYSTELEM(shapeindex);
    MS_COPYPOINT(&(dst->point), &(src->point));
    /* msCopyShape(&(dst->poly), &(src->poly)); */
    MS_COPYSTELEM(status);

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyMarkerCacheMember()                                           *
 *                                                                     *
 * Copy a markerCacheMemberObj                                         *
 **********************************************************************/

int msCopyMarkerCacheMember(markerCacheMemberObj *dst,
                            markerCacheMemberObj *src) 
{  
    MS_COPYSTELEM(id);
  
    /* msCopyShape(&(dst->poly), &(src->poly)); */
    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyLabelCache()                                                  *
 **********************************************************************/

int msCopyLabelCache(labelCacheObj *dst, labelCacheObj *src) 
{
    int i;
    MS_COPYSTELEM(numlabels);
    for (i = 0; i < dst->numlabels; i++) {
        msCopyLabelCacheMember(&(dst->labels[i]), &(src->labels[i]));
    }
    MS_COPYSTELEM(cachesize);
    MS_COPYSTELEM(nummarkers);
    for (i = 0; i < dst->nummarkers; i++) {
        msCopyMarkerCacheMember(&(dst->markers[i]), &(src->markers[i]));
    }

    MS_COPYSTELEM(markercachesize);

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyMarkerCacheMember()                                           *
 *                                                                     *
 * Copy a markerCacheMemberObj                                         *
 **********************************************************************/

int msCopyResultCacheMember(resultCacheMemberObj *dst,
                            resultCacheMemberObj *src)
{
    MS_COPYSTELEM(shapeindex);
    MS_COPYSTELEM(tileindex);
    MS_COPYSTELEM(classindex);

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyResultCache()                                                 *
 **********************************************************************/

int msCopyResultCache(resultCacheObj *dst, resultCacheObj *src) 
{
    int i;
    MS_COPYSTELEM(cachesize);
    MS_COPYSTELEM(numresults);
    for (i = 0; i < dst->numresults; i++) {
        msCopyResultCacheMember(&(dst->results[i]), &(src->results[i]));
    }
    MS_COPYRECT(&(dst->bounds), &(src->bounds));

    return MS_SUCCESS;
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

    initReferenceMap(dst);

    MS_COPYRECT(&(dst->extent), &(src->extent));

    MS_COPYSTELEM(height);
    MS_COPYSTELEM(width);


    MS_COPYCOLOR(&(dst->color), &(src->color));
    MS_COPYCOLOR(&(dst->outlinecolor),&(src->outlinecolor));
    MS_COPYSTRING(dst->image, src->image);

    MS_COPYSTELEM(status);
    MS_COPYSTELEM(marker);
    MS_COPYSTRING(dst->markername, src->markername);
    MS_COPYSTELEM(markersize);
    MS_COPYSTELEM(minboxsize);
    MS_COPYSTELEM(maxboxsize);
    dst->map = map;

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyScalebar()                                                    *
 *                                                                     *
 * Copy a scalebarObj, using initScalebar(), msCopyColor(),            *
 * and msCopyLabel()                                                   *
 **********************************************************************/

int msCopyScalebar(scalebarObj *dst, scalebarObj *src)
{

    initScalebar(dst);

    MS_COPYCOLOR(&(dst->imagecolor), &(src->imagecolor));
    MS_COPYSTELEM(height);
    MS_COPYSTELEM(width);
    MS_COPYSTELEM(style);
    MS_COPYSTELEM(intervals);

    if (msCopyLabel(&(dst->label), &(src->label)) != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy label.","msCopyScalebar()");
        return MS_FAILURE;
    }

    MS_COPYCOLOR(&(dst->color), &(src->color));
    MS_COPYCOLOR(&(dst->backgroundcolor), &(src->backgroundcolor));

    MS_COPYCOLOR(&(dst->outlinecolor), &(src->outlinecolor));

    MS_COPYSTELEM(units);
    MS_COPYSTELEM(status);
    MS_COPYSTELEM(position);
    MS_COPYSTELEM(transparent);
    MS_COPYSTELEM(interlace);
    MS_COPYSTELEM(postlabelcache);

    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyLegend()                                                      *
 *                                                                     *
 * Copy a legendObj, using msCopyColor()                               *
 **********************************************************************/

int msCopyLegend(legendObj *dst, legendObj *src, mapObj *map)
{
    int return_value;

    MS_COPYCOLOR(&(dst->imagecolor), &(src->imagecolor));

    return_value = msCopyLabel(&(dst->label), &(src->label));
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy label.",
            "msCopyLegend()");
        return MS_FAILURE;
    }

    MS_COPYSTELEM(keysizex);
    MS_COPYSTELEM(keysizey);
    MS_COPYSTELEM(keyspacingx);
    MS_COPYSTELEM(keyspacingy);

    MS_COPYCOLOR(&(dst->outlinecolor),&(src->outlinecolor));

    MS_COPYSTELEM(status);
    MS_COPYSTELEM(height);
    MS_COPYSTELEM(width);
    MS_COPYSTELEM(position);
    MS_COPYSTELEM(transparent);
    MS_COPYSTELEM(interlace);
    MS_COPYSTELEM(postlabelcache);

#ifndef __cplusplus
    MS_COPYSTRING(dst->template, src->template);
#else
    MS_COPYSTRING(dst->_template, src->_template);
#endif
    dst->map = map;

    return MS_SUCCESS;
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

    MS_COPYSTELEM(index);
    MS_COPYSTRING(dst->classitem, src->classitem);

    MS_COPYSTELEM(classitemindex);
    MS_COPYSTELEM(numclasses);

    for (i = 0; i < dst->numclasses; i++) {
#ifndef __cplusplus
        initClass(&(dst->class[i]));

        return_value = msCopyClass(&(dst->class[i]), &(src->class[i]), dst);
        if (return_value != MS_SUCCESS) {
            msSetError(MS_MEMERR, "Failed to copy class.", "msCopyLayer()");
            return MS_FAILURE;
        }
#else
        initClass(&(dst->_class[i]));

        return_value = msCopyClass(&(dst->_class[i]), &(src->_class[i]), dst);
        if (return_value != MS_SUCCESS) {
            msSetError(MS_MEMERR, "Failed to copy _class.", "msCopyLayer()");
            return MS_FAILURE;
        }
#endif
    }
    MS_COPYSTRING(dst->header, src->header);
    MS_COPYSTRING(dst->footer, src->footer);
#ifndef __cplusplus
    MS_COPYSTRING(dst->template, src->template);
#else
    MS_COPYSTRING(dst->_template, src->_template);
#endif

    MS_COPYSTRING(dst->name, src->name); 
    MS_COPYSTRING(dst->group, src->group); 
    MS_COPYSTRING(dst->data, src->data); 
    
    MS_COPYSTELEM(status);
    MS_COPYSTELEM(type);
    MS_COPYSTELEM(annotate);
    MS_COPYSTELEM(tolerance);
    MS_COPYSTELEM(toleranceunits);
    MS_COPYSTELEM(symbolscale);
    MS_COPYSTELEM(scalefactor);
    MS_COPYSTELEM(minscale);
    MS_COPYSTELEM(maxscale);

    MS_COPYSTELEM(labelminscale);
    MS_COPYSTELEM(labelmaxscale);

    MS_COPYSTELEM(sizeunits);
    MS_COPYSTELEM(maxfeatures);

    MS_COPYCOLOR(&(dst->offsite), &(src->offsite));

    MS_COPYSTELEM(transform);
    MS_COPYSTELEM(labelcache);
    MS_COPYSTELEM(postlabelcache); 

    MS_COPYSTRING(dst->labelitem, src->labelitem);
    MS_COPYSTRING(dst->labelsizeitem, src->labelsizeitem);
    MS_COPYSTRING(dst->labelangleitem, src->labelangleitem);

    MS_COPYSTELEM(labelitemindex);
    MS_COPYSTELEM(labelsizeitemindex);
    MS_COPYSTELEM(labelangleitemindex);

    MS_COPYSTRING(dst->tileitem, src->tileitem);
    MS_COPYSTELEM(tileitemindex);

    MS_COPYSTRING(dst->tileindex, src->tileindex); 

    return_value = msCopyProjection(&(dst->projection),&(src->projection));
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy projection.", "msCopyLayer()");
        return MS_FAILURE;
    }

    MS_COPYSTELEM(project); 
    MS_COPYSTELEM(units); 

    current = src->features;
    while(current != NULL) {
        insertFeatureList(&(dst->features), &(current->shape));
        current = current->next;
    }

    MS_COPYSTRING(dst->connection, src->connection);
    MS_COPYSTELEM(connectiontype);
    
    /* Do not copy *layerinfo, items, or iteminfo. these are all initialized
       when the copied layer is opened */

    return_value = msCopyExpression(&(dst->filter), &(src->filter));
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy filter.", "msCopyLayer()");
        return MS_FAILURE;
    }

    MS_COPYSTRING(dst->filteritem, src->filteritem);
    MS_COPYSTELEM(filteritemindex);

    MS_COPYSTRING(dst->styleitem, src->styleitem); 
    MS_COPYSTELEM(styleitemindex);

    MS_COPYSTRING(dst->requires, src->requires); 
    MS_COPYSTRING(dst->labelrequires, src->labelrequires);

    if (&(src->metadata)) {
        msCopyHashTable(&(dst->metadata), &(src->metadata));
    }

    MS_COPYSTELEM(transparency);
    MS_COPYSTELEM(dump);
    MS_COPYSTELEM(debug);

    /* No need to copy the numprocessing member, as it is incremented by
       msLayerAddProcessing */
    for (i = 0; i < src->numprocessing; i++) 
    {
        msLayerAddProcessing(dst, msLayerGetProcessing(src, i));
    }

    MS_COPYSTELEM(numjoins);

    for (i = 0; i < dst->numjoins; i++) {
        return_value = msCopyJoin(&(dst->joins[i]), &(src->joins[i]));
        if (return_value != MS_SUCCESS)
            return MS_FAILURE;
    }

		MS_COPYRECT(&(dst->extent), &(src->extent));
    
    return MS_SUCCESS;
}

/***********************************************************************
 * msCopyMap()                                                         *
 *                                                                     *
 * Copy a mapObj, using mapfile.c:initLayer(), msCopyLayer(),          *

 * msCopyLegend(), msCopyScalebar(), msCopyProjection()                *
 * msCopyOutputFormat(), msCopyWeb(), msCopyReferenceMap()             *
 **********************************************************************/

int msCopyMap(mapObj *dst, mapObj *src)
{
    int i, return_value;
    outputFormatObj *format;

    MS_COPYSTRING(dst->name, src->name); 
    MS_COPYSTELEM(status);
    MS_COPYSTELEM(height);
    MS_COPYSTELEM(width);
    MS_COPYSTELEM(numlayers);

    for (i = 0; i < dst->numlayers; i++) {
        initLayer(&(dst->layers[i]), dst);

        return_value = msCopyLayer(&(dst->layers[i]), &(src->layers[i]));
        if (return_value != MS_SUCCESS) {
            msSetError(MS_MEMERR, "Failed to copy layer.", "msCopyMap()");
            return MS_FAILURE;
        }
    }
    
    return_value = msCopyFontSet(&(dst->fontset), &(src->fontset), dst);
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy fontset.", "msCopyMap()");
        return MS_FAILURE;
    }
    
    return_value = msCopySymbolSet(&(dst->symbolset), &(src->symbolset), dst);
    if(return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy symbolset.", "msCopyMap()");
        return MS_FAILURE;
    }

    /* msCopyLabelCache(&(dst->labelcache), &(src->labelcache)); */
    MS_COPYSTELEM(transparent);
    MS_COPYSTELEM(interlace);
    MS_COPYSTELEM(imagequality);

    MS_COPYRECT(&(dst->extent), &(src->extent));

    MS_COPYSTELEM(cellsize);
    MS_COPYSTELEM(units);
    MS_COPYSTELEM(scale);
    MS_COPYSTELEM(resolution);
    MS_COPYSTRING(dst->shapepath, src->shapepath); 
    MS_COPYSTRING(dst->mappath, src->mappath); 

    MS_COPYCOLOR(&(dst->imagecolor), &(src->imagecolor));

    /* clear existing destination format list */
    if( dst->outputformat && --dst->outputformat->refcount < 1 )
    {
        msFreeOutputFormat( dst->outputformat );
        dst->outputformat = NULL;
    }

    for(i=0; i < dst->numoutputformats; i++ ) {
        if( --dst->outputformatlist[i]->refcount < 1 )
            msFreeOutputFormat( dst->outputformatlist[i] );
    }
    if( dst->outputformatlist != NULL )
        msFree( dst->outputformatlist );
    dst->outputformatlist = NULL;
    dst->outputformat = NULL;
    dst->numoutputformats = 0;

    for (i = 0; i < src->numoutputformats; i++)
        msAppendOutputFormat( dst, 
        msCloneOutputFormat( src->outputformatlist[i]) );

    /* set the active output format */
    MS_COPYSTRING(dst->imagetype, src->imagetype); 
    format = msSelectOutputFormat( dst, dst->imagetype );
    msApplyOutputFormat(&(dst->outputformat), format, MS_NOOVERRIDE, 
        MS_NOOVERRIDE, MS_NOOVERRIDE );

    return_value = msCopyProjection(&(dst->projection),&(src->projection));
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy projection.", "msCopyMap()");
        return MS_FAILURE;
    }

    /* No need to copy latlon projection */

    return_value = msCopyReferenceMap(&(dst->reference),&(src->reference),
        dst);
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy reference.", "msCopyMap()");
        return MS_FAILURE;
    }

    return_value = msCopyScalebar(&(dst->scalebar), &(src->scalebar));
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy scalebar.", "msCopyMap()");
        return MS_FAILURE;
    }

    return_value = msCopyLegend(&(dst->legend), &(src->legend),dst);
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy legend.", "msCopyMap()");
        return MS_FAILURE;
    }

    return_value = msCopyQueryMap(&(dst->querymap), &(src->querymap));
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy querymap.", "msCopyMap()");
        return MS_FAILURE;
    }

    return_value = msCopyWeb(&(dst->web), &(src->web), dst);
    if (return_value != MS_SUCCESS) {
        msSetError(MS_MEMERR, "Failed to copy web.", "msCopyMap()");
        return MS_FAILURE;
    }

    for (i = 0; i < dst->numlayers; i++) {
        MS_COPYSTELEM(layerorder[i]);
    }
    MS_COPYSTELEM(debug);
    MS_COPYSTRING(dst->datapattern, src->datapattern);
    MS_COPYSTRING(dst->templatepattern, src->templatepattern);   

    if( msCopyHashTable( &(dst->configoptions), &(src->configoptions) ) != MS_SUCCESS )
        return MS_FAILURE;

    return MS_SUCCESS;
}


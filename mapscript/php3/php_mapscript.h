/**********************************************************************
 * $Id$
 *
 * Name:     php_mapscript.c
 * Project:  PHP/MapScript extension for MapServer.
 * Language: ANSI C
 * Purpose:  Header file - prototypes / module definitions
 * Author:   Daniel Morissette, danmo@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 2000, Daniel Morissette
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log$
 * Revision 1.2  2000/06/28 20:22:02  dan
 * Sync with mapscript.i version 1.9
 *
 * Revision 1.1  2000/05/09 21:06:11  dan
 * Initial Import
 *
 * Revision 1.4  2000/04/26 16:10:02  daniel
 * Changes to build with ms_3.3.010
 *
 * Revision 1.3  2000/03/15 00:42:24  daniel
 * Finished conversion of all cover functions to real C
 *
 * Revision 1.2  2000/02/07 05:19:38  daniel
 * Added layerObj and classObj
 *
 * Revision 1.1  2000/01/31 08:38:43  daniel
 * First working version - only mapObj implemented with read-only properties
 *
 **********************************************************************/

#ifndef _PHP_MAPSCRIPT_H_INCLUDED_
#define _PHP_MAPSCRIPT_H_INCLUDED_

#include "map.h"

/*=====================================================================
 *                   Internal functions from mapscript_i.c
 *====================================================================*/

mapObj         *mapObj_new(char *filename);
void            mapObj_destroy(mapObj* self);
layerObj       *mapObj_getLayer(mapObj* self, int i);
layerObj       *mapObj_getLayerByName(mapObj* self, char *name);
int             mapObj_addColor(mapObj* self, int r, int g, int b);
int             mapObj_getSymbolByName(mapObj* self, int type, char *name);
void            mapObj_prepareQuery(mapObj* self);
gdImagePtr      mapObj_prepareImage(mapObj* self);
gdImagePtr      mapObj_draw(mapObj* self);
gdImagePtr      mapObj_drawQueryMap(mapObj* self, queryResultObj *results);
gdImagePtr      mapObj_drawLegend(mapObj* self);
gdImagePtr      mapObj_drawScalebar(mapObj* self);
gdImagePtr      mapObj_drawReferenceMap(mapObj* self);
int             mapObj_embedScalebar(mapObj* self, gdImagePtr img);
int             mapObj_embedLegend(mapObj* self, gdImagePtr img);
int             mapObj_drawLabelCache(mapObj* self, gdImagePtr img);
labelCacheMemberObj *mapObj_nextLabel(mapObj* self);
queryResultObj *mapObj_queryUsingPoint(mapObj* self, pointObj *point, 
                                       int mode, double buffer);
queryResultObj *mapObj_queryUsingRect(mapObj* self, rectObj *rect);
int             mapObj_queryUsingFeatures(mapObj* self, 
                                          queryResultObj *results);
int             mapObj_setProjection(mapObj* self, char *string);
int             mapObj_save(mapObj* self, char *filename);


queryResultObj *queryResultObj_new(char *filename);
void            queryResultObj_destroy(queryResultObj *self);
void            queryResultObj_free(queryResultObj *self);
int             queryResultObj_save(queryResultObj *self, char *filename);
shapeResultObj  queryResultObj_next(queryResultObj *self);
void            queryResultObj_rewind(queryResultObj *self);


layerObj       *layerObj_new(mapObj *map);
void            layerObj_destroy(layerObj* self);
classObj       *layerObj_getClass(layerObj *self, int i);
int             layerObj_draw(layerObj *self, mapObj *map, gdImagePtr img);
queryResultObj *layerObj_queryUsingPoint(layerObj *self, mapObj *map, 
                                         pointObj *point, int mode, 
                                         double buffer);
queryResultObj *layerObj_queryUsingRect(layerObj *self, mapObj *map, 
                                        rectObj *rect);
int             layerObj_queryUsingFeatures(layerObj *self, mapObj *map, 
                                            queryResultObj *results);
int             layerObj_setProjection(layerObj *self, char *string);


classObj       *classObj_new(layerObj *layer);
void            classObj_destroy(classObj* self);
int             classObj_setExpression(classObj *self, char *string);
int             classObj_setText(classObj *self,layerObj *layer,char *string);


queryObj       *queryObj_new(layerObj *layer);
void            queryObj_destroy(queryObj *self);
int             queryObj_setExpression(queryObj *self, char *string);

struct featureObj *featureObj_new(layerObj *layer);
void            featureObj_destroy(struct featureObj *self);
int             featureObj_add(struct featureObj *self, lineObj *p);


pointObj       *pointObj_new();
void            pointObj_destroy(pointObj *self);
int             pointObj_draw(pointObj *self, mapObj *map, layerObj *layer, 
                              gdImagePtr img, char *class_string, 
                              char *label_string);
double          pointObj_distanceToPoint(pointObj *self, pointObj *point);
double          pointObj_distanceToLine(pointObj *self, pointObj *a, 
                                        pointObj *b);
double          pointObj_distanceToShape(pointObj *self, shapeObj *shape);


lineObj        *lineObj_new();
void            lineObj_destroy(lineObj *self);
int             lineObj_add(lineObj *self, pointObj *p);


shapeObj       *shapeObj_new(int type);
void            shapeObj_destroy(shapeObj *self);
lineObj        *shapeObj_get(shapeObj *self, int i);
int             shapeObj_add(shapeObj *self, lineObj *line);
int             shapeObj_draw(shapeObj *self, mapObj *map, layerObj *layer, 
                              gdImagePtr img, char *class_string, 
                              char *label_string);

rectObj        *rectObj_new();
void            rectObj_destroy(rectObj *self);
double          rectObj_fit(rectObj *self, int width, int height);
int             rectObj_draw(rectObj *self, mapObj *map, layerObj *layer, 
                             gdImagePtr img, char *class_string, 
                             char *label_string);


shapefileObj   *shapefileObj_new(char *filename, int type);
void            shapefileObj_destroy(shapefileObj *self);
int             shapefileObj_get(shapefileObj *self, int i, shapeObj *shape);
int             shapefileObj_getTransformed(shapefileObj *self, mapObj *map, 
                                            int i, shapeObj *shape);
void            shapefileObj_getExtent(shapefileObj *self, int i, 
                                       rectObj *rect);
int             shapefileObj_add(shapefileObj *self, shapeObj *shape);


#endif /* _PHP_MAPSCRIPT_H_INCLUDED_ */



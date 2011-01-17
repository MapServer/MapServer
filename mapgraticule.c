/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Graticule Renderer
 * Author:   John Novak, Novacell Technologies (jnovak@novacell.com)
 *
 **********************************************************************
 * Copyright (c) 2003, John Novak, Novacell Technologies
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 ****************************************************************************/

#include "mapserver.h"
#include <assert.h>
#include "mapproject.h"

MS_CVSID("$Id$")

/**********************************************************************************************************************
 *
 */
typedef enum
{
   posBottom = 1,
   posTop,
   posLeft,
   posRight
} msGraticulePosition;

typedef enum
{
  lpDefault = 0,
  lpDDMMSS = 1,
  lpDDMM,
  lpDD
} msLabelProcessingType;

void DefineAxis( int iTickCountTarget, double *Min, double *Max, double *Inc );
static int _AdjustLabelPosition( layerObj *pLayer, shapeObj *pShape, msGraticulePosition ePosition );
static void _FormatLabel( layerObj *pLayer, shapeObj *pShape, double dDataToFormat );

int msGraticuleLayerInitItemInfo(layerObj *layer);

#define MAPGRATICULE_ARC_SUBDIVISION_DEFAULT   (256)
#define MAPGRATICULE_ARC_MINIMUM             (16)
#define MAPGRATICULE_FORMAT_STRING_DEFAULT      "%5.2g"
#define MAPGRATICULE_FORMAT_STRING_DDMMSS      "%3d %02d %02d"
#define MAPGRATICULE_FORMAT_STRING_DDMM         "%3d %02d"
#define MAPGRATICULE_FORMAT_STRING_DD                   "%3d"

/**********************************************************************************************************************
 *
 */
int msGraticuleLayerOpen(layerObj *layer) 
{
  graticuleObj *pInfo = (graticuleObj *) layer->layerinfo;
  
  if( pInfo == NULL )
    return MS_FAILURE;
  
  pInfo->dincrementlatitude = 15.0;
  pInfo->dincrementlongitude = 15.0;
  pInfo->dwhichlatitude = -90.0;
  pInfo->dwhichlongitude = -180.0;
  pInfo->bvertical = 1;

  if( layer->numclasses == 0 )
      msDebug( "GRID layer has no classes, nothing will be rendered.\n" );

  if( layer->numclasses == 0 || layer->class[0]->label.size == -1 )
    pInfo->blabelaxes = 0;
  else
    pInfo->blabelaxes = 1;
  
  if( pInfo->labelformat == NULL ) {
    pInfo->labelformat = (char *) malloc( strlen( MAPGRATICULE_FORMAT_STRING_DEFAULT ) + 1 );
    pInfo->ilabeltype = (int) lpDefault;
    strcpy( pInfo->labelformat, MAPGRATICULE_FORMAT_STRING_DEFAULT );
  } else if( strcmp( pInfo->labelformat, "DDMMSS" ) == 0 ) {
    pInfo->labelformat = (char *) malloc( strlen( MAPGRATICULE_FORMAT_STRING_DDMMSS ) + 1 );
    pInfo->ilabeltype = (int) lpDDMMSS;
    strcpy( pInfo->labelformat, MAPGRATICULE_FORMAT_STRING_DDMMSS );
  } else if( strcmp( pInfo->labelformat, "DDMM" )   == 0 ) {
    pInfo->labelformat = (char *) malloc( strlen( MAPGRATICULE_FORMAT_STRING_DDMM ) + 1 );
    pInfo->ilabeltype = (int) lpDDMM;
    strcpy( pInfo->labelformat, MAPGRATICULE_FORMAT_STRING_DDMM );
  } else if( strcmp( pInfo->labelformat, "DD" )   == 0 ) {
    pInfo->labelformat = (char *) malloc( strlen( MAPGRATICULE_FORMAT_STRING_DD ) + 1 );
    pInfo->ilabeltype = (int) lpDD;
    strcpy( pInfo->labelformat, MAPGRATICULE_FORMAT_STRING_DD );
  }
  
  return MS_SUCCESS;
}

/**********************************************************************************************************************
 * Return MS_TRUE if layer is open, MS_FALSE otherwise.
 */
int msGraticuleLayerIsOpen(layerObj *layer)
{
  if(layer->layerinfo)
    return MS_TRUE;

  return MS_FALSE;
}


/**********************************************************************************************************************
 *
 */
int msGraticuleLayerClose(layerObj *layer)
{
  graticuleObj *pInfo = (graticuleObj *) layer->layerinfo;
  
  if( pInfo->labelformat ) {
    free( pInfo->labelformat );
    pInfo->labelformat = NULL;
  }
   
  if( pInfo->pboundingpoints ) {
    free( pInfo->pboundingpoints );
    pInfo->pboundingpoints = NULL;
  }

  if( pInfo->pboundinglines ) {
    free( pInfo->pboundinglines );
    pInfo->pboundinglines = NULL;
  }
  
  return MS_SUCCESS;
}

/**********************************************************************************************************************
 *
 */
int msGraticuleLayerWhichShapes(layerObj *layer, rectObj rect)
{
  graticuleObj *pInfo = (graticuleObj *) layer->layerinfo;
  int iAxisTickCount = 0;
  rectObj rectMapCoordinates;

  if(msCheckParentPointer(layer->map,"map") == MS_FAILURE)
    return MS_FAILURE;

  pInfo->dstartlatitude = rect.miny;
  pInfo->dstartlongitude = rect.minx;
  pInfo->dendlatitude = rect.maxy;
  pInfo->dendlongitude = rect.maxx;
  pInfo->bvertical = 1;
  pInfo->extent   = rect;

  if( pInfo->minincrement > 0.0 ) {
    pInfo->dincrementlongitude = pInfo->minincrement;
    pInfo->dincrementlatitude = pInfo->minincrement;
  } else if( pInfo->maxincrement > 0.0 ) {
    pInfo->dincrementlongitude = pInfo->maxincrement;
    pInfo->dincrementlatitude = pInfo->maxincrement;
  } else {
    pInfo->dincrementlongitude = 0;
    pInfo->dincrementlatitude = 0;
  }
   
  if( pInfo->maxarcs > 0 )
    iAxisTickCount = (int) pInfo->maxarcs;
  else if( pInfo->minarcs > 0 )
    iAxisTickCount = (int) pInfo->minarcs;

  DefineAxis( iAxisTickCount, &pInfo->dstartlongitude, &pInfo->dendlongitude, &pInfo->dincrementlongitude);
  DefineAxis( iAxisTickCount, &pInfo->dstartlatitude, &pInfo->dendlatitude, &pInfo->dincrementlatitude);

  pInfo->dwhichlatitude   = pInfo->dstartlatitude;
  pInfo->dwhichlongitude = pInfo->dstartlongitude;

  if( pInfo->minincrement > 0.0 && pInfo->maxincrement > 0.0 && pInfo->minincrement == pInfo->maxincrement ) {
    pInfo->dincrementlongitude = pInfo->minincrement;
    pInfo->dincrementlatitude = pInfo->minincrement;
  } else if( pInfo->minincrement > 0.0 ) {
    pInfo->dincrementlongitude = pInfo->minincrement;
    pInfo->dincrementlatitude = pInfo->minincrement;
  } else if( pInfo->maxincrement > 0.0 ) {
    pInfo->dincrementlongitude = pInfo->maxincrement;
    pInfo->dincrementlatitude = pInfo->maxincrement;
  }

  /*
   * If using PROJ, project rect back into map system, and generate rect corner points in native system.
   * These lines will be used when generating labels to get correct placement at arc/rect edge intersections.
   */
  rectMapCoordinates = layer->map->extent;
  pInfo->pboundinglines   = (lineObj *)  malloc( sizeof( lineObj )  * 4 );
  pInfo->pboundingpoints = (pointObj *) malloc( sizeof( pointObj ) * 8 );

  {
    /*
     * top
     */
    pInfo->pboundinglines[0].numpoints   = 2;
    pInfo->pboundinglines[0].point = &pInfo->pboundingpoints[0];
    pInfo->pboundinglines[0].point[0].x = rectMapCoordinates.minx;
    pInfo->pboundinglines[0].point[0].y = rectMapCoordinates.maxy;
    pInfo->pboundinglines[0].point[1].x = rectMapCoordinates.maxx;
    pInfo->pboundinglines[0].point[1].y = rectMapCoordinates.maxy;

#ifdef USE_PROJ
    if(layer->project && msProjectionsDiffer(&(layer->projection), &(layer->map->projection)))
      msProjectLine(&layer->map->projection, &layer->projection, &pInfo->pboundinglines[0]);
    else
      layer->project = MS_FALSE;
#endif

    /*
     * bottom
     */
    pInfo->pboundinglines[1].numpoints = 2; 
    pInfo->pboundinglines[1].point = &pInfo->pboundingpoints[2];
    pInfo->pboundinglines[1].point[0].x = rectMapCoordinates.minx;
    pInfo->pboundinglines[1].point[0].y   = rectMapCoordinates.miny;
    pInfo->pboundinglines[1].point[1].x   = rectMapCoordinates.maxx;
    pInfo->pboundinglines[1].point[1].y = rectMapCoordinates.miny;

#ifdef USE_PROJ
    if(layer->project && msProjectionsDiffer(&(layer->projection), &(layer->map->projection)))
      msProjectLine(&layer->map->projection, &layer->projection, &pInfo->pboundinglines[1]);
    else
      layer->project = MS_FALSE;
#endif

    /*
     * left
     */
    pInfo->pboundinglines[2].numpoints = 2;
    pInfo->pboundinglines[2].point = &pInfo->pboundingpoints[4];
    pInfo->pboundinglines[2].point[0].x   = rectMapCoordinates.minx;
    pInfo->pboundinglines[2].point[0].y   = rectMapCoordinates.miny;
    pInfo->pboundinglines[2].point[1].x   = rectMapCoordinates.minx;
    pInfo->pboundinglines[2].point[1].y   = rectMapCoordinates.maxy;

#ifdef USE_PROJ
    if(layer->project && msProjectionsDiffer(&(layer->projection), &(layer->map->projection)))
      msProjectLine(&layer->map->projection, &layer->projection, &pInfo->pboundinglines[2]);
    else
      layer->project = MS_FALSE;
#endif

    /*
     * right
     */
    pInfo->pboundinglines[3].numpoints = 2;
    pInfo->pboundinglines[3].point = &pInfo->pboundingpoints[6];
    pInfo->pboundinglines[3].point[0].x   = rectMapCoordinates.maxx;
    pInfo->pboundinglines[3].point[0].y   = rectMapCoordinates.miny;
    pInfo->pboundinglines[3].point[1].x = rectMapCoordinates.maxx;
    pInfo->pboundinglines[3].point[1].y = rectMapCoordinates.maxy;

#ifdef USE_PROJ
    if(layer->project && msProjectionsDiffer(&(layer->projection), &(layer->map->projection)))
      msProjectLine(&layer->map->projection, &layer->projection, &pInfo->pboundinglines[3]);
    else
      layer->project = MS_FALSE;
#endif
  }

  return MS_SUCCESS;
}

/**********************************************************************************************************************
 *
 */
int msGraticuleLayerNextShape(layerObj *layer, shapeObj *shape)
{
  graticuleObj *pInfo = (graticuleObj *) layer->layerinfo;

  if( pInfo->minsubdivides <= 0.0 || pInfo->maxsubdivides <= 0.0 )
    pInfo->minsubdivides = pInfo->maxsubdivides = MAPGRATICULE_ARC_SUBDIVISION_DEFAULT;

  shape->numlines = 1;
  shape->type = MS_SHAPE_LINE;
  shape->line = (lineObj *) malloc(sizeof( lineObj ));
  shape->line->numpoints = (int) pInfo->maxsubdivides;   

  /*
   * Subdivide and draw current arc, rendering the arc labels first
   */
  if( pInfo->bvertical ) {
    int iPointIndex;
    double dArcDelta = (pInfo->dendlatitude - pInfo->dstartlatitude) / (double) shape->line->numpoints;
    double dArcPosition  = pInfo->dstartlatitude + dArcDelta;
    double dStartY, dDeltaX;
      
    switch( pInfo->ilabelstate ) {
    case 0:
      if(!pInfo->blabelaxes)  { /* Bottom */
        pInfo->ilabelstate++;
        shape->numlines = 0;
        return MS_SUCCESS;
      }

      dDeltaX = (pInfo->dwhichlongitude - pInfo->pboundinglines[1].point[0].x) / (pInfo->pboundinglines[1].point[1].x - pInfo->pboundinglines[1].point[0].x);
      if (dDeltaX < 0)
        dDeltaX=dDeltaX*-1;

      dStartY = (pInfo->pboundinglines[1].point[1].y - pInfo->pboundinglines[1].point[0].y) * dDeltaX + pInfo->pboundinglines[1].point[0].y;
      shape->line->numpoints = (int) 2;
      shape->line->point = (pointObj *) malloc( sizeof( pointObj ) * 2 );
            
      shape->line->point[0].x = pInfo->dwhichlongitude;
      shape->line->point[0].y = dStartY;
      shape->line->point[1].x = pInfo->dwhichlongitude;
      shape->line->point[1].y = dStartY + dArcDelta;

      _FormatLabel( layer, shape, shape->line->point[0].x );
      if(_AdjustLabelPosition( layer, shape, posBottom ) != MS_SUCCESS)
        return MS_FAILURE;

      pInfo->ilabelstate++;
      return MS_SUCCESS;

    case 1:
      if(!pInfo->blabelaxes) { /* Top */
        pInfo->ilabelstate++;
        shape->numlines  = 0;
        return MS_SUCCESS;
      }

      dDeltaX = (pInfo->dwhichlongitude - pInfo->pboundinglines[0].point[0].x) / (pInfo->pboundinglines[0].point[1].x - pInfo->pboundinglines[0].point[0].x );
      if (dDeltaX < 0)
        dDeltaX=dDeltaX*-1;

      dStartY = (pInfo->pboundinglines[0].point[1].y - pInfo->pboundinglines[0].point[0].y) * dDeltaX + pInfo->pboundinglines[0].point[1].y;
      shape->line->numpoints = (int) 2;
      shape->line->point = (pointObj *) malloc( sizeof( pointObj ) * 2 );

      shape->line->point[0].x = pInfo->dwhichlongitude;
      shape->line->point[0].y = dStartY - dArcDelta;
      shape->line->point[1].x = pInfo->dwhichlongitude;
      shape->line->point[1].y = dStartY;

      _FormatLabel( layer, shape, shape->line->point[0].x );
      if(_AdjustLabelPosition( layer, shape, posTop ) != MS_SUCCESS)
        return MS_FAILURE;

      pInfo->ilabelstate++;
      return MS_SUCCESS;

    case 2:
      shape->line->numpoints = (int) shape->line->numpoints + 1;
      shape->line->point = (pointObj *) malloc( sizeof( pointObj ) * shape->line->numpoints );

      shape->line->point[0].x = pInfo->dwhichlongitude;
      shape->line->point[0].y = pInfo->dstartlatitude;

      for( iPointIndex = 1; iPointIndex < shape->line->numpoints; iPointIndex++ ) {
        shape->line->point[iPointIndex].x   = pInfo->dwhichlongitude;
        shape->line->point[iPointIndex].y   = dArcPosition;
         
        dArcPosition += dArcDelta;
      }
            
      pInfo->ilabelstate = 0;
      pInfo->dwhichlongitude      += pInfo->dincrementlongitude;
      break;

    default:
      pInfo->ilabelstate    = 0;
      break;
    }
      
  } else {
    int iPointIndex;
    double dArcDelta = (pInfo->dendlongitude - pInfo->dstartlongitude) / (double) shape->line->numpoints;
    double dArcPosition   = pInfo->dstartlongitude + dArcDelta;
    double dStartX, dDeltaY;

    switch( pInfo->ilabelstate ) {
    case 0:
      if(!pInfo->blabelaxes) { /* Left side */
         pInfo->ilabelstate++;
         shape->numlines    = 0;
         return MS_SUCCESS;
      }

      dDeltaY = (pInfo->dwhichlatitude - pInfo->pboundinglines[2].point[0].y) / (pInfo->pboundinglines[2].point[1].y - pInfo->pboundinglines[2].point[0].y );
      if (dDeltaY < 0)
        dDeltaY= dDeltaY*-1;

      dStartX = (pInfo->pboundinglines[2].point[1].x - pInfo->pboundinglines[2].point[0].x) * dDeltaY + pInfo->pboundinglines[2].point[0].x;
      shape->line->numpoints = (int) 2;
      shape->line->point    = (pointObj *) malloc( sizeof( pointObj ) * 2 );
            
      shape->line->point[0].x = dStartX;
      shape->line->point[0].y = pInfo->dwhichlatitude;
      shape->line->point[1].x = dStartX + dArcDelta;
      shape->line->point[1].y = pInfo->dwhichlatitude;

      _FormatLabel( layer, shape, shape->line->point[0].y );
      if(_AdjustLabelPosition( layer, shape, posLeft ) != MS_SUCCESS)
        return MS_FAILURE;

      pInfo->ilabelstate++;
      return MS_SUCCESS;

    case 1:
      if(!pInfo->blabelaxes) { /* Right side */
        pInfo->ilabelstate++;
        shape->numlines    = 0;
        return MS_SUCCESS;
      }

      dDeltaY = (pInfo->dwhichlatitude - pInfo->pboundinglines[3].point[0].y) / (pInfo->pboundinglines[3].point[1].y - pInfo->pboundinglines[3].point[0].y );
      if (dDeltaY < 0)
        dDeltaY= dDeltaY*-1;

      dStartX = (pInfo->pboundinglines[3].point[1].x - pInfo->pboundinglines[3].point[0].x) * dDeltaY + pInfo->pboundinglines[3].point[0].x;
      shape->line->numpoints = (int) 2;
      shape->line->point = (pointObj *) malloc( sizeof( pointObj ) * 2 );

      shape->line->point[0].x = dStartX - dArcDelta;
      shape->line->point[0].y = pInfo->dwhichlatitude;
      shape->line->point[1].x = dStartX;
      shape->line->point[1].y = pInfo->dwhichlatitude;

      _FormatLabel( layer, shape, shape->line->point[0].y );
      if(_AdjustLabelPosition( layer, shape, posRight ) != MS_SUCCESS)
        return MS_FAILURE;

      pInfo->ilabelstate++;
      return MS_SUCCESS;

    case 2:
      shape->line->numpoints = (int) shape->line->numpoints + 1;
      shape->line->point = (pointObj *) malloc( sizeof( pointObj ) * shape->line->numpoints );

      shape->line->point[0].x = pInfo->dstartlongitude;
      shape->line->point[0].y = pInfo->dwhichlatitude;

      for(iPointIndex = 1; iPointIndex < shape->line->numpoints; iPointIndex++) {
        shape->line->point[iPointIndex].x   = dArcPosition;
        shape->line->point[iPointIndex].y   = pInfo->dwhichlatitude;
         
        dArcPosition += dArcDelta;
      }
            
      pInfo->ilabelstate    = 0;
      pInfo->dwhichlatitude += pInfo->dincrementlatitude;
      break;

    default:
      pInfo->ilabelstate    = 0;
      break;
    }
  }

  /*
   * Increment and move to next arc
   */
 
  if( pInfo->bvertical && pInfo->dwhichlongitude > pInfo->dendlongitude )   {
      pInfo->dwhichlatitude   = pInfo->dstartlatitude;
      pInfo->bvertical   = 0;
  }

  if (pInfo->dwhichlatitude >  pInfo->dendlatitude)
    return MS_DONE;

   

  return MS_SUCCESS;
}

/**********************************************************************************************************************
 *
 */
int msGraticuleLayerGetItems(layerObj *layer)
{
  char **ppItemName   = (char **) malloc( sizeof( char * ) );

  *ppItemName = (char *) malloc( 64 ); /* why is this necessary? */
  strcpy( *ppItemName, "Graticule" );

  layer->numitems   = 1;
  layer->items   = ppItemName;

  return MS_SUCCESS;
}

/**********************************************************************************************************************
 *
 */
int msGraticuleLayerInitItemInfo(layerObj *layer)
{
  return MS_SUCCESS;
}

/**********************************************************************************************************************
 *
 */
void msGraticuleLayerFreeItemInfo(layerObj *layer)
{
  return;
}

/**********************************************************************************************************************
 *
 */
int msGraticuleLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record)
{
  return 0;
}

/**********************************************************************************************************************
 *
 */
int msGraticuleLayerGetExtent(layerObj *layer, rectObj *extent)
{
  graticuleObj *pInfo = (graticuleObj  *) layer->layerinfo;

  if(pInfo) {
    *extent = pInfo->extent;  
    return MS_SUCCESS;
  }

  return MS_FAILURE;
}

/**********************************************************************************************************************
 *
 */
int msGraticuleLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c, int tile, long record)
{
  return MS_SUCCESS;
}


/************************************************************************/
/*                  msGraticuleLayerFreeIntersectionPoints              */
/*                                                                      */
/*      Free intersection object.                                       */
/************************************************************************/
void msGraticuleLayerFreeIntersectionPoints( graticuleIntersectionObj *psValue)
{
    int i=0;
    if (psValue)
    {
        for (i=0; i<psValue->nTop; i++)
          msFree(psValue->papszTopLabels[i]);
        msFree(psValue->papszTopLabels);
        msFree(psValue->pasTop);

        for (i=0; i<psValue->nBottom; i++)
          msFree(psValue->papszBottomLabels[i]);
        msFree(psValue->papszBottomLabels);
        msFree(psValue->pasBottom);


        for (i=0; i<psValue->nLeft; i++)
          msFree(psValue->papszLeftLabels[i]);
        msFree(psValue->papszLeftLabels);
        msFree(psValue->pasLeft);
        
        for (i=0; i<psValue->nRight; i++)
          msFree(psValue->papszRightLabels[i]);
        msFree(psValue->papszRightLabels);
        msFree(psValue->pasRight);

        msFree(psValue);
    }
}


/************************************************************************/
/*                  msGraticuleLayerInitIntersectionPoints              */
/*                                                                      */
/*      init intersection object.                                       */
/************************************************************************/
static void msGraticuleLayerInitIntersectionPoints( graticuleIntersectionObj *psValue)
{
    if (psValue)
    {
        psValue->nTop = 0;
        psValue->pasTop = NULL;
        psValue->papszTopLabels = NULL;
        psValue->nBottom = 0;
        psValue->pasBottom = NULL;
        psValue->papszBottomLabels = NULL;
        psValue->nLeft = 0;
        psValue->pasLeft = NULL;
        psValue->papszLeftLabels = NULL;
        psValue->nRight = 0;
        psValue->pasRight = NULL;
        psValue->papszRightLabels = NULL;
    }
}


/************************************************************************/
/*                  msGraticuleLayerGetIntersectionPoints               */
/*                                                                      */
/*      Utility function thar returns all intersection positions and    */
/*      labels (4 sides of the map) for a grid layer.                   */
/************************************************************************/
graticuleIntersectionObj *msGraticuleLayerGetIntersectionPoints(mapObj *map, 
                                                                layerObj *layer)
{
    
    shapeObj    shapegrid, tmpshape;
    rectObj     searchrect;
    int         status;
    pointObj oFirstPoint;
    pointObj oLastPoint;
    lineObj oLineObj;
    rectObj cliprect;
    graticuleObj   *pInfo  = NULL;
    double dfTmp;
    graticuleIntersectionObj *psValues = NULL;
    int i=0;

    if (layer->connectiontype != MS_GRATICULE)
      return NULL;

    pInfo  = (graticuleObj *) layer->layerinfo;

    /*set cellsize if bnot already set*/
    if (map->cellsize == 0)
      map->cellsize = msAdjustExtent(&(map->extent),map->width,map->height);

    psValues = (graticuleIntersectionObj *)malloc(sizeof(graticuleIntersectionObj));
    msGraticuleLayerInitIntersectionPoints(psValues);

    if(layer->transform == MS_TRUE)
    searchrect = map->extent;
    else {
      searchrect.minx = searchrect.miny = 0;
      searchrect.maxx = map->width-1;
      searchrect.maxy = map->height-1;
    }
  
#ifdef USE_PROJ
    if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
      msProjectRect(&map->projection, &layer->projection, &searchrect); /* project the searchrect to source coords */
#endif
 
    msLayerOpen(layer);
   
    status = msLayerWhichShapes(layer, searchrect);
    if(status == MS_DONE) { /* no overlap */
      msLayerClose(layer);
      return NULL;
    } else if(status != MS_SUCCESS) {
      msLayerClose(layer);
      return NULL;
    }
  
    /* step through the target shapes */
    msInitShape(&shapegrid);
    cliprect.minx = map->extent.minx- map->cellsize;
    cliprect.miny = map->extent.miny- map->cellsize;
    cliprect.maxx = map->extent.maxx + map->cellsize;
    cliprect.maxy = map->extent.maxy + map->cellsize;

    //clip using the layer projection
    //msProjectRect(&map->projection , &layer->projection,  &cliprect);

    while((status = msLayerNextShape(layer, &shapegrid)) == MS_SUCCESS) 
    {
        /*don't really need a class here*/
        /*
          shapegrid.classindex = msShapeGetClass(layer, &shapegrid, map->scaledenom, NULL, 0);
          if((shapegrid.classindex == -1) || (layer->class[shapegrid.classindex]->status == MS_OFF)) {
        msFreeShape(&shapegrid);
        continue;
        }
        */
       
      msInitShape(&tmpshape);
      msCopyShape(&shapegrid, &tmpshape);      
      //status = msDrawShape(map, layer, &tmpshape, image, -1);
      
      if(layer->project && msProjectionsDiffer(&(layer->projection), &(map->projection)))
        msProjectShape(&layer->projection, &map->projection, &shapegrid);

      msClipPolylineRect(&shapegrid, cliprect);
     
#ifdef USE_AGG
      if(MS_RENDERER_AGG(map->outputformat))
        msTransformShapeAGG(&shapegrid, map->extent, map->cellsize);
#else
      msTransformShapeToPixel(&shapegrid, map->extent, map->cellsize);
#endif
     
 
      if(shapegrid.numlines <= 0 || shapegrid.line[0].numpoints < 2) { /* once clipped the shape didn't need to be drawn */
        msFreeShape(&shapegrid);
        msFreeShape(&tmpshape);
        continue;
      }

      

      if(shapegrid.numlines >= 1 && shapegrid.line[0].numpoints >=2)// && shapegrid.text)
      {         
          int iTmpLine = 0;
          int nNumPoints = 0;
          /*grid code seems to retunr lines that can double cross the extenst??*/
          /*creating a more than one clipped shape. Take the shape that has the most
            points, which should be the most likley to be correct*/
          
          if (shapegrid.numlines > 1)
          {
              for (i=0; i<shapegrid.numlines; i++)
              {
                  if (shapegrid.line[i].numpoints > nNumPoints)
                  {
                      nNumPoints = shapegrid.line[i].numpoints;
                      iTmpLine = i;
                  }
              }
          }
          /* get the first and last point*/
          oFirstPoint.x = shapegrid.line[iTmpLine].point[0].x;
          oFirstPoint.y = shapegrid.line[iTmpLine].point[0].y;
          oLineObj = shapegrid.line[iTmpLine];
          oLastPoint.x = oLineObj.point[oLineObj.numpoints-1].x;
          oLastPoint.y = oLineObj.point[oLineObj.numpoints-1].y;          

          if ( pInfo->bvertical) /*vertical*/
            {
              /*SHAPES ARE DRAWN FROM BOTTOM TO TOP.*/
              /*Normally lines are drawn FROM BOTTOM TO TOP but not always for some reason, so
                make sure that firstpoint < lastpoint in y, We are in pixel coordinates so y increases as we 
              we go down*/
              if (oFirstPoint.y < oLastPoint.y)
                {
                  dfTmp = oFirstPoint.x;
                  oFirstPoint.x = oLastPoint.x;
                  oLastPoint.x = dfTmp;
                  dfTmp = oFirstPoint.y;
                  oFirstPoint.y = oLastPoint.y;
                  oLastPoint.y = dfTmp;
                      
                }

              /*first point should cross the BOTTOM base where y== map->height*/
                 
              if (abs ((int)oFirstPoint.y - map->height)  <=1)
              { 
                  char *pszLabel=NULL;
                  oFirstPoint.y = map->height;
                      
                  /*validate point is in map width/height*/
                  if (oFirstPoint.x < 0 || oFirstPoint.x > map->width)
                    continue;

                  /*validate point is in map width/height*/
                  if (oLastPoint.x < 0 || oLastPoint.x > map->width)
                    continue;

                  if (shapegrid.text)
                    pszLabel =  strdup(shapegrid.text);
                  else
                  {
                      _FormatLabel(layer, &tmpshape, tmpshape.line[0].point[tmpshape.line[0].numpoints-1].x );
                      if (tmpshape.text)
                        pszLabel = strdup(tmpshape.text);
                      else
                        pszLabel = strdup("unknown");
                  }
                  /*validate that the  value is not already in the array*/
                  if ( psValues->nBottom > 0)
                    {
                      //if (psValues->pasBottom[psValues->nBottom-1].x == oFirstPoint.x)
                      //continue;

                      for (i=0; i<psValues->nBottom; i++)
                        {
                          if (psValues->pasBottom[i].x == oFirstPoint.x)
                            break;
                        }
                      if (i  < psValues->nBottom)
                        continue;
                    }
                  if (psValues->pasBottom == NULL)
                    {
                      psValues->pasBottom = (pointObj *)malloc(sizeof(pointObj));
                      psValues->papszBottomLabels = (char **)malloc(sizeof(char *));
                      psValues->nBottom = 1;
                    }
                  else
                    {
                      psValues->nBottom++;
                      psValues->pasBottom = (pointObj *)realloc(psValues->pasBottom, sizeof(pointObj)*psValues->nBottom);
                      psValues->papszBottomLabels = (char **)realloc(psValues->papszBottomLabels, sizeof(char *)*psValues->nBottom);
                    }
                      
                  psValues->pasBottom[psValues->nBottom-1].x = oFirstPoint.x;
                  psValues->pasBottom[psValues->nBottom-1].y = oFirstPoint.y;
                  psValues->papszBottomLabels[psValues->nBottom-1] = strdup(pszLabel);

                  msFree(pszLabel); 

                }
              /*first point should cross the TOP base where y==0*/
              if (abs((int)oLastPoint.y) <=1)
              {
                  char *pszLabel=NULL;
                  oLastPoint.y = 0;

                  /*validate point is in map width/height*/
                  if (oLastPoint.x < 0 || oLastPoint.x > map->width)
                    continue;

                  if (shapegrid.text)
                    pszLabel =  strdup(shapegrid.text);
                  else
                  {
                      _FormatLabel(layer, &tmpshape, tmpshape.line[0].point[tmpshape.line[0].numpoints-1].x );
                       if (tmpshape.text)
                         pszLabel = strdup(tmpshape.text);
                       else
                         pszLabel = strdup("unknown");
                  }
                  /*validate if same value is not already there*/
                  if ( psValues->nTop > 0)
                    {
                      //if (psValues->pasTop[psValues->nTop-1].x == oLastPoint.x)
                      //continue;

                      for (i=0; i<psValues->nTop; i++)
                        {
                          if (psValues->pasTop[i].x == oLastPoint.x || 
                              strcasecmp(pszLabel, psValues->papszTopLabels[i]) == 0)
                            break;
                        }
                      if (i < psValues->nTop)
                        continue;
                    }
                  
                    
                  if (psValues->pasTop == NULL)
                    {
                      psValues->pasTop = (pointObj *)malloc(sizeof(pointObj));
                      psValues->papszTopLabels = (char **)malloc(sizeof(char *));
                      psValues->nTop = 1;
                    }
                  else
                    {
                      psValues->nTop++;
                      psValues->pasTop = (pointObj *)realloc(psValues->pasTop, sizeof(pointObj)*psValues->nTop);
                      psValues->papszTopLabels = (char **)realloc(psValues->papszTopLabels, sizeof(char *)*psValues->nTop);
                    }
                      
                  psValues->pasTop[psValues->nTop-1].x = oLastPoint.x;
                  psValues->pasTop[psValues->nTop-1].y = oLastPoint.y;
                  psValues->papszTopLabels[psValues->nTop-1] = strdup(pszLabel);

                  msFree(pszLabel); 
                }
            }
          else /*horzontal*/
            {
              /*Normally lines are drawn from left to right but not always for some reason, so
                make sure that firstpoint < lastpoint in x*/
              if (oFirstPoint.x > oLastPoint.x)
              {
                  
                  dfTmp = oFirstPoint.x;
                  oFirstPoint.x = oLastPoint.x;
                  oLastPoint.x = dfTmp;
                  dfTmp = oFirstPoint.y;
                  oFirstPoint.y = oLastPoint.y;
                  oLastPoint.y = dfTmp;
                      
                }
              /*first point should cross the LEFT base where x=0 axis*/
              if (abs((int)oFirstPoint.x) <=1)
              {
                  char *pszLabel=NULL;
                  oFirstPoint.x = 0;

                  /*validate point is in map width/height*/
                  if (oFirstPoint.y < 0 || oFirstPoint.y > map->height)
                    continue;

                  if (shapegrid.text)
                    pszLabel =  strdup(shapegrid.text);
                  else
                  {
                      _FormatLabel(layer, &tmpshape, tmpshape.line[0].point[tmpshape.line[0].numpoints-1].y );
                       if (tmpshape.text)
                         pszLabel = strdup(tmpshape.text);
                       else
                         pszLabel = strdup("unknown");
                  }

                  /*validate that the previous value is not the same*/
                  if ( psValues->nLeft > 0)
                    {
                      //if (psValues->pasLeft[psValues->nLeft-1].y == oFirstPoint.y)
                      // continue;

                      for (i=0; i<psValues->nLeft; i++)
                        {
                          if (psValues->pasLeft[i].y == oFirstPoint.y)
                            break;
                        }
                      if (i < psValues->nLeft)
                        continue;
                    }
                  if (psValues->pasLeft == NULL)
                    {
                      psValues->pasLeft = (pointObj *)malloc(sizeof(pointObj));
                      psValues->papszLeftLabels = (char **)malloc(sizeof(char *));
                      psValues->nLeft = 1;
                    }
                  else
                    {
                      psValues->nLeft++;
                      psValues->pasLeft = (pointObj *)realloc(psValues->pasLeft, sizeof(pointObj)*psValues->nLeft);
                      psValues->papszLeftLabels = (char **)realloc(psValues->papszLeftLabels, sizeof(char *)*psValues->nLeft);
                    }
                      
                  psValues->pasLeft[psValues->nLeft-1].x = oFirstPoint.x;
                  psValues->pasLeft[psValues->nLeft-1].y = oFirstPoint.y;
                  psValues->papszLeftLabels[psValues->nLeft-1] = strdup(pszLabel);
                  msFree(pszLabel); 
                }
              /*first point should cross the RIGHT base where x=map=>width axis*/
              if (abs((int)oLastPoint.x - map->width) <=1)
              {
                  char *pszLabel=NULL;
                  oLastPoint.x =  map->width;

                  /*validate point is in map width/height*/
                  if (oLastPoint.y < 0 || oLastPoint.y > map->height)
                    continue;

                  if (shapegrid.text)
                    pszLabel =  strdup(shapegrid.text);
                  else
                  {
                      _FormatLabel(layer, &tmpshape, tmpshape.line[0].point[tmpshape.line[0].numpoints-1].y );
                       if (tmpshape.text)
                         pszLabel = strdup(tmpshape.text);
                       else
                         pszLabel = strdup("unknown");
                  }

                  /*validate that the previous value is not the same*/
                  if ( psValues->nRight > 0)
                    {
                      //if (psValues->pasRight[psValues->nRight-1].y == oLastPoint.y)
                      // continue;
                      for (i=0; i<psValues->nRight; i++)
                        {     
                          if (psValues->pasRight[i].y == oLastPoint.y)
                            break;
                        }
                      if (i < psValues->nRight)
                        continue;
                    }
                  if (psValues->pasRight == NULL)
                    {
                      psValues->pasRight = (pointObj *)malloc(sizeof(pointObj));
                      psValues->papszRightLabels = (char **)malloc(sizeof(char *));
                      psValues->nRight = 1;
                    }
                  else
                    {
                      psValues->nRight++;
                      psValues->pasRight = (pointObj *)realloc(psValues->pasRight, sizeof(pointObj)*psValues->nRight);
                      psValues->papszRightLabels = (char **)realloc(psValues->papszRightLabels, sizeof(char *)*psValues->nRight);
                    }
                      
                  psValues->pasRight[psValues->nRight-1].x = oLastPoint.x;
                  psValues->pasRight[psValues->nRight-1].y = oLastPoint.y;
                  psValues->papszRightLabels[psValues->nRight-1] = strdup(pszLabel);

                  msFree(pszLabel); 
                }
            }
          msFreeShape(&shapegrid);
          msFreeShape(&tmpshape);
        }
      msInitShape(&shapegrid);
     

  
    }
    msLayerClose(layer);
    return psValues;
    
}


/**********************************************************************************************************************
 *
 */
int msGraticuleLayerInitializeVirtualTable(layerObj *layer)
{
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  layer->vtable->LayerInitItemInfo = msGraticuleLayerInitItemInfo; /* should use defaults for item info functions */
  layer->vtable->LayerFreeItemInfo = msGraticuleLayerFreeItemInfo;
  layer->vtable->LayerOpen = msGraticuleLayerOpen;
  layer->vtable->LayerIsOpen = msGraticuleLayerIsOpen;
  layer->vtable->LayerWhichShapes = msGraticuleLayerWhichShapes;
  layer->vtable->LayerNextShape = msGraticuleLayerNextShape;
  /* layer->vtable->LayerResultsGetShape, use default */
  layer->vtable->LayerGetShape = msGraticuleLayerGetShape;
  layer->vtable->LayerClose = msGraticuleLayerClose;
  layer->vtable->LayerGetItems = msGraticuleLayerGetItems;
  layer->vtable->LayerGetExtent = msGraticuleLayerGetExtent;
  layer->vtable->LayerGetAutoStyle = msGraticuleLayerGetAutoStyle;
  /* layer->vtable->LayerCloseConnection, use default */;
  layer->vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;
  /* layer->vtable->LayerApplyFilterToLayer, use default */
  /* layer->vtable->LayerCreateItems, use default */
  /* layer->vtable->LayerGetNumFeatures, use default */

  return MS_SUCCESS;
}

/**********************************************************************************************************************
 *
 */
static void _FormatLabel( layerObj *pLayer, shapeObj *pShape, double dDataToFormat )
{
  graticuleObj *pInfo = (graticuleObj  *) pLayer->layerinfo;
  char cBuffer[32];
  int iDegrees, iMinutes;
   
  switch( pInfo->ilabeltype ) {
  case lpDDMMSS:
    iDegrees = (int) dDataToFormat;
    dDataToFormat = fabs( dDataToFormat - (double) iDegrees );    
    iMinutes = (int) (dDataToFormat * 60.0);
    dDataToFormat = dDataToFormat - (((double) iMinutes) / 60.0);    
    sprintf( cBuffer, pInfo->labelformat, iDegrees, iMinutes, (int) (dDataToFormat * 3600.0) );
    break;
  case lpDDMM:
    iDegrees = (int) dDataToFormat;
    dDataToFormat = fabs( dDataToFormat - (double) iDegrees );    
    sprintf( cBuffer, pInfo->labelformat, iDegrees, (int) (dDataToFormat * 60.0) );
    break;
  case lpDD:
    iDegrees = (int) dDataToFormat;
    sprintf( cBuffer, pInfo->labelformat, iDegrees);
    break;  
  case lpDefault:
  default:
    sprintf( cBuffer, pInfo->labelformat, dDataToFormat );
  }
   
  pShape->text = strdup( cBuffer );
}

/**********************************************************************************************************************
 *
 *  Move label position into display area by adjusting underlying shape line.
 */
static int _AdjustLabelPosition( layerObj *pLayer, shapeObj *pShape, msGraticulePosition ePosition )
{
  graticuleObj *pInfo = (graticuleObj  *) pLayer->layerinfo;
  rectObj rectLabel;
  pointObj ptPoint;

  if( pInfo == NULL || pShape == NULL ) {
    msSetError(MS_MISCERR, "Assertion failed: Null shape or layerinfo!, ", "_AdjustLabelPosition()");
    return MS_FAILURE;
  }
 
  if(msCheckParentPointer(pLayer->map,"map")==MS_FAILURE)
    return MS_FAILURE;
   
  ptPoint = pShape->line->point[0];

#ifdef USE_PROJ
  if(pLayer->project && msProjectionsDiffer( &pLayer->projection, &pLayer->map->projection ))
    msProjectShape( &pLayer->projection, &pLayer->map->projection, pShape );
#endif

  if(pLayer->transform) 
    msTransformShapeToPixel(pShape, pLayer->map->extent, pLayer->map->cellsize);

  if(msGetLabelSize(NULL, pShape->text, &pLayer->class[0]->label, &rectLabel, &pLayer->map->fontset, 1.0, MS_FALSE,NULL) != 0)
    return MS_FAILURE;  /* msSetError already called */

  switch( ePosition ) {
  case posBottom:
    pShape->line->point[1].y = pLayer->map->height;
    pShape->line->point[0].y = pLayer->map->height - (fabs(rectLabel.maxy - rectLabel.miny) * 2 + 5);
    break;
  case posTop:
    pShape->line->point[1].y = 0;
    pShape->line->point[0].y = fabs(rectLabel.maxy - rectLabel.miny) * 2 + 5;
    break;
  case posLeft:
    pShape->line->point[1].x = 0;
    pShape->line->point[0].x = fabs(rectLabel.maxx - rectLabel.minx) * 2 + 5;
    break;
  case posRight:
    pShape->line->point[1].x = pLayer->map->width;
    pShape->line->point[0].x = pLayer->map->width - (fabs(rectLabel.maxx - rectLabel.minx) * 2 + 5);
    break;
  }

  if(pLayer->transform) 
    msTransformPixelToShape( pShape, pLayer->map->extent, pLayer->map->cellsize );
   
#ifdef USE_PROJ
  if(pLayer->project && msProjectionsDiffer( &pLayer->map->projection, &pLayer->projection ))
    msProjectShape( &pLayer->map->projection, &pLayer->projection, pShape );
#endif

  switch( ePosition ) {
  case posBottom:
  case posTop:
    pShape->line->point[1].x = ptPoint.x;
    pShape->line->point[0].x = ptPoint.x;
    break;
  case posLeft:
  case posRight:
    pShape->line->point[1].y = ptPoint.y;
    pShape->line->point[0].y = ptPoint.y;
    break;
  }

  return MS_SUCCESS;
}

/**********************************************************************************************************************
 **********************************************************************************************************************
 * DefineAxes - Copyright (c) 2000, Michael P.D. Bramley.
 *
 * Permission is granted to use this code without restriction as long as
 * this copyright notice appears in all source files.
 *
 * Minor tweaks to incrment calculations - jnovak
 */
void DefineAxis( int iTickCountTarget, double *Min, double *Max, double *Inc )
{
  /* define local variables... */

  double Test_inc,    /* candidate increment value */
         Test_min,    /* minimum scale value */
         Test_max,    /* maximum scale value */
         Range = *Max - *Min ;  /* range of data */

  int i = 0 ;   /* counter */

  /* don't create problems -- solve them */

  if( Range < 0 )  {
    *Inc = 0 ;
    return ;
  }

  /* handle special case of repeated values */

  else if( Range == 0)  {   
    *Min = ceil(*Max) - 1 ;
    *Max = *Min + 1 ;
    *Inc = 1 ;
    return ;
  }

  /* compute candidate for increment */

  Test_inc = pow( 10.0, ceil( log10( Range/10 ) ) ) ;
  if(*Inc > 0 && ( Test_inc < *Inc || Test_inc > *Inc ))
     Test_inc = *Inc;

   /* establish maximum scale value... */
   Test_max = ( (long)(*Max / Test_inc)) * Test_inc ;

   if( Test_max < *Max )
      Test_max += Test_inc ;

   /* establish minimum scale value... */
   Test_min = Test_max ;
   do {
     ++i ;
     Test_min -= Test_inc ;
   } while( Test_min > *Min ) ;

   /* subtracting small values can screw up the scale limits, */
   /* eg: if DefineAxis is called with (min,max)=(0.01, 0.1), */
   /* then the calculated scale is 1.0408E17 TO 0.05 BY 0.01. */
   /* the following if statment corrects for this... */

   /* if(fabs(Test_min) < 1E-10) */
   /* Test_min = 0 ; */

   /* adjust for too few tick marks */

   if(iTickCountTarget <= 0)
     iTickCountTarget   = MAPGRATICULE_ARC_MINIMUM;

   while(i < iTickCountTarget) {
     Test_inc /= 2 ;
     i *= 2 ;
   }

   if(i < 6 && 0) {
     Test_inc /= 2 ;
     if((Test_min + Test_inc) <= *Min)
       Test_min += Test_inc ;
     if((Test_max - Test_inc) >= *Max)
       Test_max -= Test_inc ;
   }

   /* pass back axis definition to caller */

   *Min = Test_min;
   *Max = Test_max;
   *Inc = Test_inc;
}

/**********************************************************************************************************************
 **********************************************************************************************************************/

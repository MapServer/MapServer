/******************************************************************************
 * $Id$ 
 *
 * Project:  MapServer
 * Purpose:  MapServer Tile Access API
 * Author:   Paul Ramsey <pramsey@cleverelephant.ca>
 *
 ******************************************************************************
 * Copyright (c) 2008, Paul Ramsey
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
 ****************************************************************************/

#include "maptile.h"
#include "mapproject.h"

/************************************************************************
 *                            msTileSetup                               *
 *                                                                      * 
 *  Called from mapserv.c, this is where the fun begins                 *
 *  Set up projections and test the parameters for legality.            *
 ************************************************************************/
int msTileSetup(mapservObj* msObj) {
#ifdef USE_TILE_API 

  char *outProjStr = NULL;

  /* 
  ** Ensure all the LAYERs have a projection. 
  */  
  if( msTileSetProjections(msObj->map) != 0 ) {
    return(MS_FAILURE);
  }

  /* 
  ** Set the projection string for this mode. 
  */
  if( msObj->TileMode == TILE_GMAP || msObj->TileMode == TILE_VE ) {
    outProjStr = SPHEREMERC_PROJ4;
  } else {
    return(MS_FAILURE); /* Huh? No mode? */
  }
  if( msLoadProjectionString(&(msObj->map->projection), outProjStr) != 0 ) {
    msSetError(MS_CGIERR, "Unable to load projection string.", "msTileSetup()");
    return(MS_FAILURE);
  }
  
  /*
  ** Set up the output extents for this tilemode and tile coordinates 
  */
  if( msObj->TileMode == TILE_GMAP ) {

    int num_coords = 0;
    char **coords = NULL;
    int x, y, zoom;
    double zoomfactor;
    
    if( msObj->TileCoords ) {
      coords = msStringSplit(msObj->TileCoords, ' ', &(num_coords));
      if( num_coords != 3 ) {
        msSetError(MS_WEBERR, "Invalid number of tile coordinates (should be three).", "msTileSetup()");
        return(MS_FAILURE);
      }
    } 
    else {
      msSetError(MS_WEBERR, "Tile parameter not set.", "msTileSetup()");
      return(MS_FAILURE);
    }
    
    x = strtol(coords[0], NULL, 10);
    y = strtol(coords[1], NULL, 10);
    zoom = strtol(coords[2], NULL, 10);

    zoomfactor = pow(2.0, (double)zoom);
    
    /*
    ** Check the input request for sanity.
    */
    if( x >= zoomfactor || y >= zoomfactor ) {
      msSetError(MS_CGIERR, "GMap tile coordinates are too large for supplied zoom.", "msTileSetup()");
      return(MS_FAILURE);
    }
    if( x < 0 || y < 0 ) {
      msSetError(MS_CGIERR, "GMap tile coordinates should not be less than zero.", "msTileSetup()");
      return(MS_FAILURE);
    }

  }
  else if ( msObj->TileMode == TILE_VE ) {
    
    if( strspn( msObj->TileCoords, "0123" ) < strlen( msObj->TileCoords ) ) {
      msSetError(MS_CGIERR, "VE tile name should only include characters 0, 1, 2 and 3.", "msTileSetup()");
      return(MS_FAILURE);
    }

  }
  else {
    return(MS_FAILURE); /* Huh? Should have a mode. */
  }
     
  return MS_SUCCESS;
#else
  msSetError(MS_CGIERR, "Tile API is not available.", "msTileSetup()");
  return(MS_FAILURE);
#endif
}

/************************************************************************
 *                            msTileSetExtent                           *
 *                                                                      * 
 *  Based on the input parameters, set the output extent for this       *
 *  tile.                                                               *
 ************************************************************************/
int msTileSetExtent(mapservObj* msObj) {
#ifdef USE_TILE_API 
  
  mapObj *map = msObj->map;
  double	dx, dy;

  if( msObj->TileMode == TILE_GMAP ) {

    int num_coords = 0;
    char **coords = NULL;
    int x, y, zoom;
    double zoomfactor, tilesize, xmin, xmax, ymin, ymax;
    
    if( msObj->TileCoords ) {
      coords = msStringSplit(msObj->TileCoords, ' ', &(num_coords));
      if( num_coords != 3 ) {
        msSetError(MS_WEBERR, "Invalid number of tile coordinates (should be three).", "msTileSetExtent()");
        return(MS_FAILURE);
      }
    } 
    else {
      msSetError(MS_WEBERR, "Tile parameter not set.", "msTileSetup()");
      return(MS_FAILURE);
    }    

    x = strtol(coords[0], NULL, 10);
    y = strtol(coords[1], NULL, 10);
    zoom = strtol(coords[2], NULL, 10);

    zoomfactor = pow(2.0, (double)zoom);
    
    /*
     * Calculate the ground extents of the tile request.
     */
    /* printf("X: %i  Y: %i  Z: %i\n",x,y,zoom); */
    tilesize = SPHEREMERC_GROUND_SIZE / zoomfactor;
    xmin = (x * tilesize) - (SPHEREMERC_GROUND_SIZE / 2.0);
    xmax = ((x + 1) * tilesize) - (SPHEREMERC_GROUND_SIZE / 2.0);
    ymin = (SPHEREMERC_GROUND_SIZE / 2.0) - ((y + 1) * tilesize);
    ymax = (SPHEREMERC_GROUND_SIZE / 2.0) - (y * tilesize);
    
    map->extent.minx = xmin;
    map->extent.maxx = xmax;
    map->extent.miny = ymin;
    map->extent.maxy = ymax;
  
  }
  else if( msObj->TileMode == TILE_VE ) {

    double minx = SPHEREMERC_GROUND_SIZE / -2.0;
    double miny = SPHEREMERC_GROUND_SIZE / -2.0;
    double maxx = SPHEREMERC_GROUND_SIZE / 2.0;
    double maxy = SPHEREMERC_GROUND_SIZE / 2.0;
    double zoom = 2.0;
    double tsize;
    int i = 0;
    char j = 0;
    
    for( i = 0; i < strlen( msObj->TileCoords ); i++ ) {
      j = msObj->TileCoords[i];
      tsize = SPHEREMERC_GROUND_SIZE / zoom;
      if( j == '1' || j == '3' ) minx += tsize;
      if( j == '0' || j == '2' ) maxx -= tsize;
      if( j == '2' || j == '3' ) maxy -= tsize;
      if( j == '0' || j == '1' ) miny += tsize;
      zoom *= 2.0;
    }
    
    map->extent.minx = minx;
    map->extent.maxx = maxx;
    map->extent.miny = miny;
    map->extent.maxy = maxy;

  }
  else {
    return(MS_FAILURE); /* Huh? Should have a mode. */
  }
  
  /* 
  ** Adjust the extents inwards by 1/2 pixel so they are from
  ** center-of-pixel to center-of-pixel, instead of edge-to-edge.
  ** This is the way mapserver does it.
  */
  dx = (map->extent.maxx - map->extent.minx) / map->width;
  map->extent.minx += dx*0.5;
  map->extent.maxx -= dx*0.5;
  dy = (map->extent.maxy - map->extent.miny) / map->height;
  map->extent.miny += dy*0.5;
  map->extent.maxy -= dy*0.5;
  
  /* 
  ** Set the output tile size.
  */
  msObj->ImgCols = SPHEREMERC_IMAGE_SIZE;
  msObj->ImgRows = SPHEREMERC_IMAGE_SIZE;
  map->width = SPHEREMERC_IMAGE_SIZE;
  map->height = SPHEREMERC_IMAGE_SIZE;

  if(map->debug) {
    msDebug( "msTileSetExtent (%f, %f) (%f, %f)\n", map->extent.minx, map->extent.miny, map->extent.maxx, map->extent.maxy);
  }
 
  return MS_SUCCESS;
#else
  msSetError(MS_CGIERR, "Tile API is not available.", "msTileSetExtent()");
  return(MS_FAILURE);
#endif
}


/************************************************************************
 *                            msTileSetProjections                      *
 *                                                                      * 
 *   Ensure that all the layers in the map file have a projection       *
 *   by copying the map-level projection to all layers than have no     *
 *   projection.                                                        *
 ************************************************************************/

int msTileSetProjections(mapObj* map) {

  char *mapProjStr = NULL;
  int i;
    
  if (map->projection.numargs <= 0) {
    msSetError(MS_WMSERR, "Cannot set new SRS on a map that doesn't "
                          "have any projection set. Please make sure your mapfile "
                          "has a PROJECTION defined at the top level.", 
                          "msTileSetProjectionst()");
    return(MS_FAILURE);
  }

  for(i=0; i<map->numlayers; i++) {
    /* This layer is turned on and needs a projection? */
    if (GET_LAYER(map, i)->projection.numargs <= 0 &&
        GET_LAYER(map, i)->status != MS_OFF &&
        GET_LAYER(map, i)->transform == MS_TRUE) {
   
      /* Fetch main map projection string only now that we need it */
      if (mapProjStr == NULL)
        mapProjStr = msGetProjectionString(&(map->projection));
      
      /* Set the projection to the map file projection */  
      if (msLoadProjectionString(&(GET_LAYER(map, i)->projection), mapProjStr) != 0) {
        msSetError(MS_CGIERR, "Unable to set projection on layer.", "msTileSetProjectionst()");
        return(MS_FAILURE);
      }
      GET_LAYER(map, i)->project = MS_TRUE;
    }
  }
  msFree(mapProjStr);
  return(MS_SUCCESS);
}



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

#ifdef USE_TILE_API
static void msTileResetMetatileLevel(mapObj *map)
{
  hashTableObj *meta = &(map->web.metadata);
  const char *zero = "0";

  /*  Is the tile_metatile_levetl set... */
  if(msLookupHashTable(meta, "tile_metatile_level") != NULL) {
    msRemoveHashTable(meta, "tile_metatile_level");
  }
  msInsertHashTable(meta, "tile_metatile_level", zero);
}
#endif

/************************************************************************
 *                            msTileGetGMapCoords                       *
 ************************************************************************/
static int msTileGetGMapCoords(const char *coordstring, int *x, int *y, int *zoom)
{

  int num_coords = 0;
  char **coords = NULL;

  if( coordstring ) {
    coords = msStringSplit(coordstring, ' ', &(num_coords));
    if( num_coords != 3 ) {
      msFreeCharArray(coords, num_coords);
      msSetError(MS_WEBERR, "Invalid number of tile coordinates (should be three).", "msTileSetup()");
      return MS_FAILURE;
    }
  } else {
    msSetError(MS_WEBERR, "Tile parameter not set.", "msTileSetup()");
    return MS_FAILURE;
  }

  if( x )
    *x = strtol(coords[0], NULL, 10);
  if( y )
    *y = strtol(coords[1], NULL, 10);
  if( zoom )
    *zoom = strtol(coords[2], NULL, 10);
  
  msFreeCharArray(coords, 3);
  return MS_SUCCESS;
}


/************************************************************************
 *                            msTileSetParams                           *
 ************************************************************************/
static void msTileGetParams(const mapservObj *msObj, tileParams *params)
{

  const char *value = NULL;
  const mapObj* map = msObj->map;
  const hashTableObj *meta = &(map->web.metadata);

  if( msObj->TileWidth < 0 )
      params->tile_width = SPHEREMERC_IMAGE_SIZE;
  else
      params->tile_width = msObj->TileWidth;
  if( msObj->TileHeight < 0 )
      params->tile_height = SPHEREMERC_IMAGE_SIZE;
  else
      params->tile_height = msObj->TileHeight;

  /* Check for tile buffer, set to buffer==0 as default */
  if((value = msLookupHashTable(meta, "tile_map_edge_buffer")) != NULL) {
    params->map_edge_buffer = atoi(value);
    if(map->debug)
      msDebug("msTileSetParams(): tile_map_edge_buffer = %d\n", params->map_edge_buffer);
  } else
    params->map_edge_buffer = 0;

  /* Check for metatile size, set to tile==metatile as default */
  if((value = msLookupHashTable(meta, "tile_metatile_level")) != NULL) {
    params->metatile_level = atoi(value);
    /* Quietly force metatile_level to be sane */
    if( params->metatile_level < 0 )
      params->metatile_level = 0;
    if( params->metatile_level > 2 )
      params->metatile_level = 2;
    if(map->debug)
      msDebug("msTileSetParams(): tile_metatile_level = %d\n", params->metatile_level);
  } else
    params->metatile_level = 0;

}

/************************************************************************
 *                            msTileExtractSubTile                      *
 *                                                                      *
 ************************************************************************/
static imageObj* msTileExtractSubTile(const mapservObj *msObj, const imageObj *img)
{

  int width, mini, minj;
  int zoom = 2;
  imageObj* imgOut = NULL;
  tileParams params;
  rendererVTableObj *renderer;
  rasterBufferObj imgBuffer;

  if( !MS_RENDERER_PLUGIN(msObj->map->outputformat)
      || msObj->map->outputformat->renderer != img->format->renderer ||
      ! MS_MAP_RENDERER(msObj->map)->supports_pixel_buffer ) {
    msSetError(MS_MISCERR,"unsupported or mixed renderers","msTileExtractSubTile()");
    return NULL;
  }
  renderer = MS_MAP_RENDERER(msObj->map);

  if (renderer->getRasterBufferHandle((imageObj*)img,&imgBuffer) != MS_SUCCESS) {
    return NULL;
  }


  /*
  ** Load the metatiling information from the map file.
  */
  msTileGetParams(msObj, &params);

  /*
  ** Initialize values for the metatile clip area.
  */
  width = img->width - 2*params.map_edge_buffer;
  mini = params.map_edge_buffer;
  minj = params.map_edge_buffer;
  
  if( msObj->TileMode == TILE_GMAP ) {
    int x, y, zoom;

    if( msObj->TileCoords ) {
      if( msTileGetGMapCoords(msObj->TileCoords, &x, &y, &zoom) == MS_FAILURE )
        return NULL;
    } else {
      msSetError(MS_WEBERR, "Tile parameter not set.", "msTileSetup()");
      return NULL;
    }

    if(msObj->map->debug)
      msDebug("msTileExtractSubTile(): gmaps coords (x: %d, y: %d)\n",x,y);

    /*
    ** The bottom N bits of the coordinates give us the subtile
    ** location relative to the metatile.
    */
    x = (0xffff ^ (0xffff << params.metatile_level)) & x;
    y = (0xffff ^ (0xffff << params.metatile_level)) & y;

    if(msObj->map->debug)
      msDebug("msTileExtractSubTile(): gmaps image coords (x: %d, y: %d)\n",x,y);

    mini = mini + x * params.tile_width;
    minj = minj + y * params.tile_height;

  } else if( msObj->TileMode == TILE_VE ) {
    int tsize;

    const int lenTileCoords = (int)strlen( msObj->TileCoords );
    if( lenTileCoords - params.metatile_level < 0 ) {
      return(NULL);
    }

    /*
    ** Process the last elements of the VE coordinate string to place the
    ** requested tile in the context of the metatile
    */
    for( int i = lenTileCoords - params.metatile_level;
         i < lenTileCoords;
         i++ ) {
      char j = msObj->TileCoords[i];
      tsize = width / zoom;
      if( j == '1' || j == '3' ) mini += tsize;
      if( j == '2' || j == '3' ) minj += tsize;
      zoom *= 2;
    }
  } else {
    return(NULL); /* Huh? Should have a mode. */
  }

  imgOut = msImageCreate(params.tile_width, params.tile_height, msObj->map->outputformat, NULL, NULL, msObj->map->resolution, msObj->map->defresolution, NULL);

  if( imgOut == NULL ) {
    return NULL;
  }

  if(msObj->map->debug)
    msDebug("msTileExtractSubTile(): extracting (%d x %d) tile, top corner (%d, %d)\n",params.tile_width, params.tile_height,mini,minj);



  if(MS_UNLIKELY(MS_FAILURE == renderer->mergeRasterBuffer(imgOut,&imgBuffer,1.0,mini, minj,0, 0,params.tile_width, params.tile_height))) {
    msFreeImage(imgOut);
    return NULL;
  }

  return imgOut;
}


/************************************************************************
 *                            msTileSetup                               *
 *                                                                      *
 *  Called from mapserv.c, this is where the fun begins                 *
 *  Set up projections and test the parameters for legality.            *
 ************************************************************************/
int msTileSetup(mapservObj* msObj)
{
#ifdef USE_TILE_API

  char *outProjStr = NULL;
  tileParams params;

  /*
  ** Load the metatiling information from the map file.
  */
  msTileGetParams(msObj, &params);

  /*
  ** Ensure all the LAYERs have a projection.
  */
  if( msMapSetLayerProjections(msObj->map) != 0 ) {
    return(MS_FAILURE);
  }

  /*
  ** Set the projection string for this mode.
  */
  if( msObj->TileMode == TILE_GMAP || msObj->TileMode == TILE_VE ) {
    outProjStr = SPHEREMERC_PROJ4;
  } else {
    return MS_FAILURE; /* Huh? No mode? */
  }
  if( msLoadProjectionString(&(msObj->map->projection), outProjStr) != 0 ) {
    msSetError(MS_CGIERR, "Unable to load projection string.", "msTileSetup()");
    return MS_FAILURE;
  }

  /*
  ** Set up the output extents for this tilemode and tile coordinates
  */
  if( msObj->TileMode == TILE_GMAP ) {

    int x, y, zoom;
    double zoomfactor;

    if( msObj->TileCoords ) {
      if( msTileGetGMapCoords(msObj->TileCoords, &x, &y, &zoom) == MS_FAILURE )
        return MS_FAILURE;
    } else {
      msSetError(MS_WEBERR, "Tile parameter not set.", "msTileSetup()");
      return MS_FAILURE;
    }

    if( params.metatile_level >= zoom ) {
      msTileResetMetatileLevel(msObj->map);
    }

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

  } else if ( msObj->TileMode == TILE_VE ) {

    if( strspn( msObj->TileCoords, "0123" ) < strlen( msObj->TileCoords ) ) {
      msSetError(MS_CGIERR, "VE tile name should only include characters 0, 1, 2 and 3.", "msTileSetup()");
      return(MS_FAILURE);
    }

    if( params.metatile_level >= (int)strlen(msObj->TileCoords) ) {
      msTileResetMetatileLevel(msObj->map);
    }

  } else {
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
int msTileSetExtent(mapservObj* msObj)
{
#ifdef USE_TILE_API

  mapObj *map = msObj->map;
  double  dx, dy, buffer;
  tileParams params;

  /* Read the tile-mode map file parameters */
  msTileGetParams(msObj, &params);

  if( msObj->TileMode == TILE_GMAP ) {
    int x, y, zoom;
    double zoomfactor, tilesize, xmin, xmax, ymin, ymax;

    if( msObj->TileCoords ) {
      if( msTileGetGMapCoords(msObj->TileCoords, &x, &y, &zoom) == MS_FAILURE )
        return MS_FAILURE;
    } else {
      msSetError(MS_WEBERR, "Tile parameter not set.", "msTileSetup()");
      return MS_FAILURE;
    }

    if(map->debug)
      msDebug("msTileSetExtent(): gmaps coords (x: %d, y: %d, z: %d)\n",x,y,zoom);

    /*
    ** If we are metatiling, adjust the zoom level appropriately,
    ** then scale back the x/y coordinates to match the new level.
    */
    if( params.metatile_level > 0 ) {
      zoom = zoom - params.metatile_level;
      x = x >> params.metatile_level;
      y = y >> params.metatile_level;
    }

    if(map->debug)
      msDebug("msTileSetExtent(): gmaps metacoords (x: %d, y: %d, z: %d)\n",x,y,zoom);

    zoomfactor = pow(2.0, (double)zoom);

    /*
    ** Calculate the ground extents of the tile request.
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

  } else if( msObj->TileMode == TILE_VE ) {

    double minx = SPHEREMERC_GROUND_SIZE / -2.0;
    double miny = SPHEREMERC_GROUND_SIZE / -2.0;
    double maxx = SPHEREMERC_GROUND_SIZE / 2.0;
    double maxy = SPHEREMERC_GROUND_SIZE / 2.0;
    double zoom = 2.0;
    double tsize;

    /*
    ** Walk down the VE URL string, adjusting the extent each time.
    ** For meta-tiling cases, we stop early, to draw a larger image.
    */
    for( int i = 0; i < (int)strlen( msObj->TileCoords ) - params.metatile_level; i++ ) {
      char j = msObj->TileCoords[i];
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

  } else {
    return(MS_FAILURE); /* Huh? Should have a mode. */
  }

  /*
  ** Set the output tile size.
  */
  map->width = params.tile_width << params.metatile_level;
  map->height = params.tile_height << params.metatile_level;

  if(map->debug)
    msDebug("msTileSetExtent(): base image size (%d x %d)\n",map->width,map->height);

  /*
  ** Add the gutters
  ** First calculate ground units in the buffer at current extent
  */
  buffer = params.map_edge_buffer * (map->extent.maxx - map->extent.minx) / (double)map->width;
  /*
  ** Then adjust the map extents out by that amount
  */
  map->extent.minx -= buffer;
  map->extent.maxx += buffer;
  map->extent.miny -= buffer;
  map->extent.maxy += buffer;
  /*
  ** Finally adjust the map image size by the pixel buffer
  */
  map->width += 2 * params.map_edge_buffer;
  map->height += 2 * params.map_edge_buffer;

  if(map->debug)
    msDebug("msTileSetExtent(): buffered image size (%d x %d)\n",map->width,map->height);

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
  ** Ensure the labelcache buffer is greater than the tile buffer.
  */
  if( params.map_edge_buffer > 0 ) {
    const char *value;
    hashTableObj *meta = &(map->web.metadata);
    char tilebufferstr[64];

    /* Write the tile buffer to a string */
    snprintf(tilebufferstr, sizeof(tilebufferstr), "-%d", params.map_edge_buffer);

    /* Hm, the labelcache buffer is set... */
    if((value = msLookupHashTable(meta, "labelcache_map_edge_buffer")) != NULL) {
      /* If it's too small, replace with a bigger one */
      if( params.map_edge_buffer > abs(atoi(value)) ) {
        msRemoveHashTable(meta, "labelcache_map_edge_buffer");
        msInsertHashTable(meta, "labelcache_map_edge_buffer", tilebufferstr);
      }
    }
    /* No labelcache buffer value? Then we use the tile buffer. */
    else {
      msInsertHashTable(meta, "labelcache_map_edge_buffer", tilebufferstr);
    }
  }

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
 *                            msDrawTile                                *
 *                                                                      *
 *   Draw the tile once with gutters, metatiling and buffers, then      *
 *   clip out the final tile.                                           *
 *   WARNING: Call msTileSetExtent() first or this will be a pointless  *
 *   fucnction call.                                                    *
 ************************************************************************/

imageObj* msTileDraw(mapservObj *msObj)
{
  imageObj *img;
  tileParams params;
  msTileGetParams(msObj, &params);
  img = msDrawMap(msObj->map, MS_FALSE);
  if( img == NULL )
    return NULL;
  if( params.metatile_level > 0 || params.map_edge_buffer > 0 ) {
    imageObj *tmp = msTileExtractSubTile(msObj, img);
    msFreeImage(img);
    if( tmp == NULL )
      return NULL;
    img = tmp;
  }
  return img;
}


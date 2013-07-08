/******************************************************************************
 * $id: mapraster.c 10772 2010-11-29 18:27:02Z aboudreault $
 *
 * Project:  MapServer
 * Purpose:  msDrawRasterLayer(): generic raster layer drawing.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *           Pete Olson (LMIC)
 *           Steve Lime
 *
 ******************************************************************************
 * Copyright (c) 1996-2010 Regents of the University of Minnesota.
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

#include <assert.h>
#include "mapserver.h"
#include "mapfile.h"
#include "mapresample.h"
#include "mapthread.h"



extern int msyylex_destroy(void);
extern int yyparse(parseObj *);

extern parseResultObj yypresult; /* result of parsing, true/false */

#ifdef USE_GDAL
#include "gdal.h"
#include "cpl_string.h"
#endif
#include "mapraster.h"

#define MAXCOLORS 256
#define BUFLEN 1024
#define HDRLEN 8

#define CVT(x) ((x) >> 8) /* converts to 8-bit color value */

#define NUMGRAYS 16

/************************************************************************/
/*                         msGetClass_String()                          */
/************************************************************************/

static int msGetClass_String( layerObj *layer, colorObj *color, const char *pixel_value )

{
  int i;
  const char *tmpstr1=NULL;
  int numitems;
  char *item_names[4] = { "pixel", "red", "green", "blue" };
  char *item_values[4];
  char red_value[8], green_value[8], blue_value[8];

  /* -------------------------------------------------------------------- */
  /*      No need to do a lookup in the case of one default class.        */
  /* -------------------------------------------------------------------- */
  if((layer->numclasses == 1) && !(layer->class[0]->expression.string)) /* no need to do lookup */
    return(0);

  /* -------------------------------------------------------------------- */
  /*      Setup values list for expressions.                              */
  /* -------------------------------------------------------------------- */
  numitems = 4;
  sprintf( red_value, "%d", color->red );
  sprintf( green_value, "%d", color->green );
  sprintf( blue_value, "%d", color->blue );

  item_values[0] = (char *)pixel_value;
  item_values[1] = red_value;
  item_values[2] = green_value;
  item_values[3] = blue_value;

  /* -------------------------------------------------------------------- */
  /*      Loop over classes till we find a match.                         */
  /* -------------------------------------------------------------------- */
  for(i=0; i<layer->numclasses; i++) {

    /* check for correct classgroup, if set */
    if ( layer->class[i]->group && layer->classgroup &&
         strcasecmp(layer->class[i]->group, layer->classgroup) != 0 )
      continue;

    /* Empty expression - always matches */
    if (layer->class[i]->expression.string == NULL)
      return(i);

    switch(layer->class[i]->expression.type) {

        /* -------------------------------------------------------------------- */
        /*      Simple string match                                             */
        /* -------------------------------------------------------------------- */
      case(MS_STRING):
        /* trim junk white space */
        tmpstr1= pixel_value;
        while( *tmpstr1 == ' ' )
          tmpstr1++;

        if(strcmp(layer->class[i]->expression.string, tmpstr1) == 0) return(i); /* matched */
        break;

        /* -------------------------------------------------------------------- */
        /*      Regular expression.  Rarely used for raster.                    */
        /* -------------------------------------------------------------------- */
      case(MS_REGEX):
        if(!layer->class[i]->expression.compiled) {
          if(ms_regcomp(&(layer->class[i]->expression.regex), layer->class[i]->expression.string, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) { /* compile the expression  */
            msSetError(MS_REGEXERR, "Invalid regular expression.", "msGetClass()");
            return(-1);
          }
          layer->class[i]->expression.compiled = MS_TRUE;
        }

        if(ms_regexec(&(layer->class[i]->expression.regex), pixel_value, 0, NULL, 0) == 0) return(i); /* got a match */
        break;

        /* -------------------------------------------------------------------- */
        /*      Parsed expression.                                              */
        /* -------------------------------------------------------------------- */
      case(MS_EXPRESSION): {
        int status;
        parseObj p;
        shapeObj dummy_shape;
        expressionObj *expression = &(layer->class[i]->expression);

        dummy_shape.numvalues = numitems;
        dummy_shape.values = item_values;

        if( expression->tokens == NULL )
          msTokenizeExpression( expression, item_names, &numitems );

        p.shape = &dummy_shape;
        p.expr = expression;
        p.expr->curtoken = p.expr->tokens; /* reset */
        p.type = MS_PARSE_TYPE_BOOLEAN;

        status = yyparse(&p);

        if (status != 0) {
          msSetError(MS_PARSEERR, "Failed to parse expression: %s", "msGetClass_FloatRGB", expression->string);
          return -1;
        }

        if( p.result.intval )
          return i;
        break;
      }
    }
  }

  return(-1); /* not found */
}

/************************************************************************/
/*                             msGetClass()                             */
/************************************************************************/

int msGetClass(layerObj *layer, colorObj *color, int colormap_index)
{
  char pixel_value[12];

  snprintf( pixel_value, sizeof(pixel_value), "%d", colormap_index );

  return msGetClass_String( layer, color, pixel_value );
}

/************************************************************************/
/*                          msGetClass_FloatRGB()                       */
/*                                                                      */
/*      Returns the class based on classification of a floating         */
/*      pixel value.                                                    */
/************************************************************************/

int msGetClass_FloatRGB(layerObj *layer, float fValue, int red, int green, int blue )
{
  char pixel_value[100];
  colorObj color;

  color.red = red;
  color.green = green;
  color.blue = blue;

  snprintf( pixel_value, sizeof(pixel_value), "%18g", fValue );

  return msGetClass_String( layer, &color, pixel_value );
}

#ifdef USE_GD
/************************************************************************/
/*                            msAddColorGD()                            */
/*                                                                      */
/*      Function to add a color to an existing color map.  It first     */
/*      looks for an exact match, then tries to add it to the end of    */
/*      the existing color map, and if all else fails it finds the      */
/*      closest color.                                                  */
/************************************************************************/

int msAddColorGD(mapObj *map, gdImagePtr img, int cmt, int r, int g, int b)
{
  int c;
  int ct = -1;
  int op = -1;
  long rd, gd, bd, dist;
  long mindist = 3*255*255;  /* init to max poss dist */

  if( gdImageTrueColor( img ) )
    return gdTrueColor( r, g, b );

  /*
  ** We want to avoid using a color that matches a transparent background
  ** color exactly.  If this is the case, we will permute the value slightly.
  ** When perterbing greyscale images we try to keep them greyscale, otherwise
  ** we just perterb the red component.
  */
  if( map->outputformat && map->outputformat->transparent
      && map->imagecolor.red == r
      && map->imagecolor.green == g
      && map->imagecolor.blue == b ) {
    if( r == 0 && g == 0 && b == 0 ) {
      r = g = b = 1;
    } else if( r == g && r == b ) {
      r = g = b = r-1;
    } else if( r == 0 ) {
      r = 1;
    } else {
      r = r-1;
    }
  }

  /*
  ** Find the nearest color in the color table.  If we get an exact match
  ** return it right away.
  */
  for (c = 0; c < img->colorsTotal; c++) {

    if (img->open[c]) {
      op = c; /* Save open slot */
      continue; /* Color not in use */
    }

    /* don't try to use the transparent color */
    if (map->outputformat && map->outputformat->transparent
        && img->red  [c] == map->imagecolor.red
        && img->green[c] == map->imagecolor.green
        && img->blue [c] == map->imagecolor.blue )
      continue;

    rd = (long)(img->red  [c] - r);
    gd = (long)(img->green[c] - g);
    bd = (long)(img->blue [c] - b);
    /* -------------------------------------------------------------------- */
    /*      special case for grey colors (r=g=b). we will try to find       */
    /*      either the nearest grey or a color that is almost grey.         */
    /* -------------------------------------------------------------------- */
    if (r == g && r == b) {
      if (img->red == img->green && img->red ==  img->blue)
        dist = rd*rd;
      else
        dist = rd * rd + gd * gd + bd * bd;
    } else
      dist = rd * rd + gd * gd + bd * bd;

    if (dist < mindist) {
      if (dist == 0) {
        return c; /* Return exact match color */
      }
      mindist = dist;
      ct = c;
    }
  }

  /* no exact match, is the closest within our "color match threshold"? */
  if( mindist <= cmt*cmt )
    return ct;

  /* no exact match.  If there are no open colors we return the closest
     color found.  */
  if (op == -1) {
    op = img->colorsTotal;
    if (op == gdMaxColors) { /* No room for more colors */
      return ct; /* Return closest available color */
    }
    img->colorsTotal++;
  }

  /* allocate a new exact match */
  img->red  [op] = r;
  img->green[op] = g;
  img->blue [op] = b;
  img->open [op] = 0;

  return op; /* Return newly allocated color */
}

#endif


#if defined(USE_GDAL)

/************************************************************************/
/*                      msRasterSetupTileLayer()                        */
/*                                                                      */
/*      Setup the tile layer.                                           */
/************************************************************************/

int msDrawRasterSetupTileLayer(mapObj *map, layerObj *layer,
                               rectObj* psearchrect,
                               int is_query,
                               int* ptilelayerindex, /* output */
                               int* ptileitemindex, /* output */
                               int* ptilesrsindex, /* output */
                               layerObj **ptlp  /* output */ )
{
    int i;
    char* requested_fields;
    int status;
    layerObj* tlp = NULL;

    *ptilelayerindex = msGetLayerIndex(layer->map, layer->tileindex);
    if(*ptilelayerindex == -1) { /* the tileindex references a file, not a layer */

      /* so we create a temporary layer */
      tlp = (layerObj *) malloc(sizeof(layerObj));
      MS_CHECK_ALLOC(tlp, sizeof(layerObj), MS_FAILURE);

      initLayer(tlp, map);
      *ptlp = tlp;

      /* set a few parameters for a very basic shapefile-based layer */
      tlp->name = msStrdup("TILE");
      tlp->type = MS_LAYER_TILEINDEX;
      tlp->data = msStrdup(layer->tileindex);

      if( is_query )
      {
        tlp->map = map;  /*needed when scaletokens are applied, to extract current map scale */
        for(i = 0; i < layer->numscaletokens; i++) {
          if(msGrowLayerScaletokens(tlp) == NULL) {
            return MS_FAILURE;
          }
          initScaleToken(&tlp->scaletokens[i]);
          msCopyScaleToken(&layer->scaletokens[i],&tlp->scaletokens[i]);
          tlp->numscaletokens++;
        }
      }

      if (layer->projection.numargs > 0 &&
        EQUAL(layer->projection.args[0], "auto"))
      {
          tlp->projection.numargs = 1;
          tlp->projection.args[0] = msStrdup("auto");
      }

      if (layer->filteritem)
        tlp->filteritem = msStrdup(layer->filteritem);
      if (layer->filter.string) {
        if (layer->filter.type == MS_EXPRESSION) {
          char* pszTmp =
            (char *)msSmallMalloc(sizeof(char)*(strlen(layer->filter.string)+3));
          sprintf(pszTmp,"(%s)",layer->filter.string);
          msLoadExpressionString(&tlp->filter, pszTmp);
          free(pszTmp);
        } else if (layer->filter.type == MS_REGEX ||
                   layer->filter.type == MS_IREGEX) {
          char* pszTmp =
            (char *)msSmallMalloc(sizeof(char)*(strlen(layer->filter.string)+3));
          sprintf(pszTmp,"/%s/",layer->filter.string);
          msLoadExpressionString(&tlp->filter, pszTmp);
          free(pszTmp);
        } else
          msLoadExpressionString(&tlp->filter, layer->filter.string);

        tlp->filter.type = layer->filter.type;
      }

    } else {
      if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
        return MS_FAILURE;
      tlp = (GET_LAYER(layer->map, *ptilelayerindex));
      *ptlp = tlp;
    }
    status = msLayerOpen(tlp);
    if(status != MS_SUCCESS) {
      return status;
    }

    /* fetch tileitem and tilesrs fields */
    requested_fields = (char*) msSmallMalloc(sizeof(char)*(strlen(layer->tileitem)+1+
                                    (layer->tilesrs ? strlen(layer->tilesrs) : 0) + 1));
    if( layer->tilesrs != NULL )
        sprintf(requested_fields, "%s,%s", layer->tileitem, layer->tilesrs);
    else
        strcpy(requested_fields, layer->tileitem);
    status = msLayerWhichItems(tlp, MS_FALSE, requested_fields);
    msFree(requested_fields);
    if(status != MS_SUCCESS) {
      return status;
    }

    /* get the tileitem and tilesrs index */
    for(i=0; i<tlp->numitems; i++) {
      if(strcasecmp(tlp->items[i], layer->tileitem) == 0) {
        *ptileitemindex = i;
      }
      if(layer->tilesrs != NULL &&
         strcasecmp(tlp->items[i], layer->tilesrs) == 0) {
        *ptilesrsindex = i;
      }
    }
    if(*ptileitemindex < 0) { /* didn't find it */
      msSetError(MS_MEMERR,
                 "Could not find attribute %s in tileindex.",
                 "msDrawRasterLayerLow()",
                 layer->tileitem);
      return MS_FAILURE;
    }
    if(layer->tilesrs != NULL && *ptilesrsindex < 0) { /* didn't find it */
      msSetError(MS_MEMERR,
                 "Could not find attribute %s in tileindex.",
                 "msDrawRasterLayerLow()",
                 layer->tilesrs);
      return MS_FAILURE;
    }

#ifdef USE_PROJ
    /* if necessary, project the searchrect to source coords */
    if((map->projection.numargs > 0) && (layer->projection.numargs > 0) &&
        !EQUAL(layer->projection.args[0], "auto")) {
      if( msProjectRect(&map->projection, &layer->projection, psearchrect)
          != MS_SUCCESS ) {
        msDebug( "msDrawRasterLayerLow(%s): unable to reproject map request rectangle into layer projection, canceling.\n", layer->name );
        return MS_FAILURE;
      }
    }
    else if((map->projection.numargs > 0) && (tlp->projection.numargs > 0) &&
        !EQUAL(tlp->projection.args[0], "auto")) {
      if( msProjectRect(&map->projection, &tlp->projection, psearchrect)
          != MS_SUCCESS ) {
        msDebug( "msDrawRasterLayerLow(%s): unable to reproject map request rectangle into layer projection, canceling.\n", layer->name );
        return MS_FAILURE;
      }
    }
#endif
    return msLayerWhichShapes(tlp, *psearchrect, MS_FALSE);
}

/************************************************************************/
/*                msDrawRasterCleanupTileLayer()                        */
/*                                                                      */
/*      Cleanup the tile layer.                                         */
/************************************************************************/

void msDrawRasterCleanupTileLayer(layerObj* tlp,
                                  int tilelayerindex)
{
    msLayerClose(tlp);
    if(tilelayerindex == -1) {
      freeLayer(tlp);
      free(tlp);
    }
}

/************************************************************************/
/*                   msDrawRasterIterateTileIndex()                     */
/*                                                                      */
/*      Iterate over the tile layer.                                    */
/************************************************************************/

int msDrawRasterIterateTileIndex(layerObj *layer,
                                 layerObj* tlp,
                                 shapeObj* ptshp, /* input-output */
                                 int tileitemindex,
                                 int tilesrsindex,
                                 char* tilename, /* output */
                                 size_t sizeof_tilename,
                                 char* tilesrsname, /* output */
                                 size_t sizeof_tilesrsname)
{
      int status;

      status = msLayerNextShape(tlp, ptshp);
      if( status == MS_FAILURE || status == MS_DONE ) {
        return status;
      }

      if(layer->data == NULL || strlen(layer->data) == 0 ) { /* assume whole filename is in attribute field */
        strlcpy( tilename, ptshp->values[tileitemindex], sizeof_tilename);
      } else
        snprintf(tilename, sizeof_tilename, "%s/%s", ptshp->values[tileitemindex], layer->data);

      tilesrsname[0] = '\0';

      if( tilesrsindex >= 0 )
      {
        if(ptshp->values[tilesrsindex] != NULL )
          strlcpy( tilesrsname, ptshp->values[tilesrsindex], sizeof_tilesrsname );
      }

      msFreeShape(ptshp); /* done with the shape */

      return status;
}

/************************************************************************/
/*                   msDrawRasterBuildRasterPath()                      */
/*                                                                      */
/*      Build the path of the raster to open.                           */
/************************************************************************/

int msDrawRasterBuildRasterPath(mapObj *map, layerObj *layer,
                                const char* filename,
                                char szPath[MS_MAXPATHLEN] /* output */)
{
    /*
    ** If using a tileindex then build the path relative to that file if SHAPEPATH is not set.
    */
    if(layer->tileindex && !map->shapepath) {
      char tiAbsFilePath[MS_MAXPATHLEN];
      char *tiAbsDirPath = NULL;

      msTryBuildPath(tiAbsFilePath, map->mappath, layer->tileindex); /* absolute path to tileindex file */
      tiAbsDirPath = msGetPath(tiAbsFilePath); /* tileindex file's directory */
      msBuildPath(szPath, tiAbsDirPath, filename);
      free(tiAbsDirPath);
    } else {
      msTryBuildPath3(szPath, map->mappath, map->shapepath, filename);
    }

    return MS_SUCCESS;
}

/************************************************************************/
/*                   msDrawRasterGetCPLErrorMsg()                       */
/*                                                                      */
/*      Return the CPL error message, and filter out sensitive info.    */
/************************************************************************/

const char* msDrawRasterGetCPLErrorMsg(const char* decrypted_path,
                                       const char* szPath)
{
      const char *cpl_error_msg = CPLGetLastErrorMsg();

      /* we wish to avoid reporting decrypted paths */
      if( cpl_error_msg != NULL
          && strstr(cpl_error_msg,decrypted_path) != NULL
          && strcmp(decrypted_path,szPath) != 0 )
        cpl_error_msg = NULL;

      /* we wish to avoid reporting the stock GDALOpen error messages */
      if( cpl_error_msg != NULL
          && (strstr(cpl_error_msg,"not recognised as a supported") != NULL
              || strstr(cpl_error_msg,"does not exist") != NULL) )
        cpl_error_msg = NULL;

      if( cpl_error_msg == NULL )
        cpl_error_msg = "";

      return cpl_error_msg;
}

/************************************************************************/
/*                   msDrawRasterLoadProjection()                       */
/*                                                                      */
/*      Handle TILESRS or PROJECTION AUTO for each tile.                */
/************************************************************************/

int msDrawRasterLoadProjection(layerObj *layer,
                               GDALDatasetH hDS,
                               const char* filename,
                               int tilesrsindex,
                               const char* tilesrsname)
{
    /*
    ** Generate the projection information if using TILESRS.
    */
    if( tilesrsindex >= 0 )
    {
        const char *pszWKT;
        if( tilesrsname[0] != '\0' )
            pszWKT = tilesrsname;
        else
            pszWKT = GDALGetProjectionRef( hDS );
        if( pszWKT != NULL && strlen(pszWKT) > 0 ) {
          if( msOGCWKT2ProjectionObj(pszWKT, &(layer->projection),
                                    layer->debug ) != MS_SUCCESS ) {
          char  szLongMsg[MESSAGELENGTH*2];
          errorObj *ms_error = msGetErrorObj();

          snprintf( szLongMsg, sizeof(szLongMsg),
                    "%s\n"
                    "PROJECTION '%s' cannot be used for this "
                    "GDAL raster (`%s').",
                    ms_error->message, pszWKT, filename);
          szLongMsg[MESSAGELENGTH-1] = '\0';

          msSetError(MS_OGRERR, "%s","msDrawRasterLayer()",
                     szLongMsg);

          return MS_FAILURE;
        }
      }
    }
    /*
    ** Generate the projection information if using AUTO.
    */
    else if (layer->projection.numargs > 0 &&
        EQUAL(layer->projection.args[0], "auto")) {
      const char *pszWKT;

      pszWKT = GDALGetProjectionRef( hDS );

      if( pszWKT != NULL && strlen(pszWKT) > 0 ) {
        if( msOGCWKT2ProjectionObj(pszWKT, &(layer->projection),
                                   layer->debug ) != MS_SUCCESS ) {
          char  szLongMsg[MESSAGELENGTH*2];
          errorObj *ms_error = msGetErrorObj();

          snprintf( szLongMsg, sizeof(szLongMsg),
                    "%s\n"
                    "PROJECTION AUTO cannot be used for this "
                    "GDAL raster (`%s').",
                    ms_error->message, filename);
          szLongMsg[MESSAGELENGTH-1] = '\0';

          msSetError(MS_OGRERR, "%s","msDrawRasterLayer()",
                     szLongMsg);

          return MS_FAILURE;
        }
      }
    }

    return MS_SUCCESS;
}
#endif // defined(USE_GDAL)

/************************************************************************/
/*                        msDrawRasterLayerLow()                        */
/*                                                                      */
/*      Check for various file types and act appropriately.  Handle     */
/*      tile indexing.                                                  */
/************************************************************************/

int msDrawRasterLayerLow(mapObj *map, layerObj *layer, imageObj *image,
                         rasterBufferObj *rb )
{
  /* -------------------------------------------------------------------- */
  /*      As of MapServer 6.0 GDAL is required for rendering raster       */
  /*      imagery.                                                        */
  /* -------------------------------------------------------------------- */
#if !defined(USE_GDAL)
  msSetError(MS_MISCERR,
             "Attempt to render a RASTER (or WMS) layer but without\n"
             "GDAL support enabled.  Raster rendering requires GDAL.",
             "msDrawRasterLayerLow()" );
  return MS_FAILURE;

#else /* defined(USE_GDAL) */
  int status, i, done;
  char *filename=NULL, tilename[MS_MAXPATHLEN], tilesrsname[1024];

  layerObj *tlp=NULL; /* pointer to the tile layer either real or temporary */
  int tileitemindex=-1, tilelayerindex=-1, tilesrsindex=-1;
  shapeObj tshp;

  char szPath[MS_MAXPATHLEN];
  char *decrypted_path;
  int final_status = MS_SUCCESS;

  rectObj searchrect;
  GDALDatasetH  hDS;
  double  adfGeoTransform[6];
  const char *close_connection;

  msGDALInitialize();

  if(layer->debug > 0 || map->debug > 1)
    msDebug( "msDrawRasterLayerLow(%s): entering.\n", layer->name );

  if(!layer->data && !layer->tileindex) {
    if(layer->debug == MS_TRUE)
      msDebug( "msDrawRasterLayerLow(%s): layer data and tileindex NULL ... doing nothing.", layer->name );
    return(0);
  }

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) {
    if(layer->debug == MS_TRUE)
      msDebug( "msDrawRasterLayerLow(%s): not status ON or DEFAULT, doing nothing.", layer->name );
    return(0);
  }

  if(map->scaledenom > 0) {
    if((layer->maxscaledenom > 0) && (map->scaledenom > layer->maxscaledenom)) {
      if(layer->debug == MS_TRUE)
        msDebug( "msDrawRasterLayerLow(%s): skipping, map scale %.2g > MAXSCALEDENOM=%g\n",
                 layer->name, map->scaledenom, layer->maxscaledenom );
      return(0);
    }
    if((layer->minscaledenom > 0) && (map->scaledenom <= layer->minscaledenom)) {
      if(layer->debug == MS_TRUE)
        msDebug( "msDrawRasterLayerLow(%s): skipping, map scale %.2g < MINSCALEDENOM=%g\n",
                 layer->name, map->scaledenom, layer->minscaledenom );
      return(0);
    }
  }

  if(layer->maxscaledenom <= 0 && layer->minscaledenom <= 0) {
    if((layer->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > layer->maxgeowidth)) {
      if(layer->debug == MS_TRUE)
        msDebug( "msDrawRasterLayerLow(%s): skipping, map width %.2g > MAXSCALEDENOM=%g\n", layer->name,
                 (map->extent.maxx - map->extent.minx), layer->maxgeowidth );
      return(0);
    }
    if((layer->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < layer->mingeowidth)) {
      if(layer->debug == MS_TRUE)
        msDebug( "msDrawRasterLayerLow(%s): skipping, map width %.2g < MINSCALEDENOM=%g\n", layer->name,
                 (map->extent.maxx - map->extent.minx), layer->mingeowidth );
      return(0);
    }
  }

  if(layer->tileindex) { /* we have an index file */
    msInitShape(&tshp);
    searchrect = map->extent;

    status = msDrawRasterSetupTileLayer(map, layer,
                           &searchrect,
                           MS_FALSE,
                           &tilelayerindex,
                           &tileitemindex,
                           &tilesrsindex,
                           &tlp);
    if(status != MS_SUCCESS) {
      if (status != MS_DONE)
        final_status = status;
      goto cleanup;
    }
  }

  done = MS_FALSE;
  while(done != MS_TRUE) {

    if(layer->tileindex) {
      status = msDrawRasterIterateTileIndex(layer, tlp, &tshp,
                                            tileitemindex, tilesrsindex,
                                            tilename, sizeof(tilename),
                                            tilesrsname, sizeof(tilesrsname));
      if( status == MS_FAILURE) {
        final_status = MS_FAILURE;
        break;
      }

      if(status == MS_DONE) break; /* no more tiles/images */
      filename = tilename;
    } else {
      filename = layer->data;
      done = MS_TRUE; /* only one image so we're done after this */
    }

    if(strlen(filename) == 0) continue;

    if(layer->debug == MS_TRUE)
      msDebug( "msDrawRasterLayerLow(%s): Filename is: %s\n", layer->name, filename);

    msDrawRasterBuildRasterPath(map, layer, filename, szPath);
    if(layer->debug == MS_TRUE)
      msDebug("msDrawRasterLayerLow(%s): Path is: %s\n", layer->name, szPath);

    /*
    ** Note: because we do decryption after the above path expansion
    ** which depends on actually finding a file, it essentially means that
    ** fancy path manipulation is essentially disabled when using encrypted
    ** components. But that is mostly ok, since stuff like sde,postgres and
    ** oracle georaster do not use real paths.
    */
    decrypted_path = msDecryptStringTokens( map, szPath );
    if( decrypted_path == NULL )
      return MS_FAILURE;

    msAcquireLock( TLOCK_GDAL );
    hDS = GDALOpenShared( decrypted_path, GA_ReadOnly );

    /*
    ** If GDAL doesn't recognise it, and it wasn't successfully opened
    ** Generate an error.
    */
    if(hDS == NULL) {
      int ignore_missing = msMapIgnoreMissingData(map);
      const char *cpl_error_msg = msDrawRasterGetCPLErrorMsg(decrypted_path, szPath);

      msFree( decrypted_path );
      decrypted_path = NULL;

      msReleaseLock( TLOCK_GDAL );

      if(ignore_missing == MS_MISSING_DATA_FAIL) {
        msSetError(MS_IOERR, "Corrupt, empty or missing file '%s' for layer '%s'. %s", "msDrawRasterLayerLow()", szPath, layer->name, cpl_error_msg );
        return(MS_FAILURE);
      } else if( ignore_missing == MS_MISSING_DATA_LOG ) {
        if( layer->debug || layer->map->debug ) {
          msDebug( "Corrupt, empty or missing file '%s' for layer '%s' ... ignoring this missing data.  %s\n", szPath, layer->name, cpl_error_msg );
        }
        continue;
      } else if( ignore_missing == MS_MISSING_DATA_IGNORE ) {
        continue;
      } else {
        /* never get here */
        msSetError(MS_IOERR, "msIgnoreMissingData returned unexpected value.", "msDrawRasterLayerLow()");
        return(MS_FAILURE);
      }
    }

    msFree( decrypted_path );
    decrypted_path = NULL;

    if( msDrawRasterLoadProjection(layer, hDS, filename, tilesrsindex, tilesrsname) != MS_SUCCESS )
    {
        msReleaseLock( TLOCK_GDAL );
        final_status = MS_FAILURE;
        break;
    }

    msGetGDALGeoTransform( hDS, map, layer, adfGeoTransform );

    /*
    ** We want to resample if the source image is rotated, if
    ** the projections differ or if resampling has been explicitly
    ** requested, or if the image has north-down instead of north-up.
    */
#ifdef USE_PROJ
    if( ((adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0
          || adfGeoTransform[5] > 0.0 || adfGeoTransform[1] < 0.0 )
         && layer->transform )
        || msProjectionsDiffer( &(map->projection),
                                &(layer->projection) )
        || CSLFetchNameValue( layer->processing, "RESAMPLE" ) != NULL ) {
      status = msResampleGDALToMap( map, layer, image, rb, hDS );
    } else
#endif
    {
      if( adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0 ) {
        if( layer->debug || map->debug )
          msDebug(
            "Layer %s has rotational coefficients but we\n"
            "are unable to use them, projections support\n"
            "needs to be built in.",
            layer->name );

      }
      status = msDrawRasterLayerGDAL(map, layer, image, rb, hDS );
    }

    if( status == -1 ) {
      GDALClose( hDS );
      msReleaseLock( TLOCK_GDAL );
      final_status = MS_FAILURE;
      break;
    }

    /*
    ** Should we keep this file open for future use?
    ** default to keeping open for single data files, and
    ** to closing for tile indexes
    */

    close_connection = msLayerGetProcessingKey( layer,
                       "CLOSE_CONNECTION" );

    if( close_connection == NULL && layer->tileindex == NULL )
      close_connection = "DEFER";

    if( close_connection != NULL
        && strcasecmp(close_connection,"DEFER") == 0 ) {
      GDALDereferenceDataset( hDS );
    } else {
      GDALClose( hDS );
    }
    msReleaseLock( TLOCK_GDAL );
  } /* next tile */

cleanup:
  if(layer->tileindex) { /* tiling clean-up */
    msDrawRasterCleanupTileLayer(tlp, tilelayerindex);
  }

  return final_status;

#endif /* defined(USE_GDAL) */
}

/************************************************************************/
/*                         msDrawReferenceMap()                         */
/************************************************************************/

imageObj *msDrawReferenceMap(mapObj *map)
{
  double cellsize;
  int x1,y1,x2,y2;
  char szPath[MS_MAXPATHLEN];

  imageObj   *image = NULL;
  styleObj style;


  rendererVTableObj *renderer = MS_MAP_RENDERER(map);
  rasterBufferObj *refImage = (rasterBufferObj*)calloc(1,sizeof(rasterBufferObj));
  MS_CHECK_ALLOC(refImage, sizeof(rasterBufferObj), NULL);

  if(MS_SUCCESS != renderer->loadImageFromFile(msBuildPath(szPath, map->mappath, map->reference.image),refImage)) {
    msSetError(MS_MISCERR,"error loading reference image %s","msDrawREferenceMap()",szPath);
    return NULL;
  }

  image = msImageCreate(refImage->width, refImage->height, map->outputformat,
                        map->web.imagepath, map->web.imageurl, map->resolution, map->defresolution, &(map->reference.color));

  renderer->mergeRasterBuffer(image,refImage,1.0,0,0,0,0,refImage->width, refImage->height);

  msFreeRasterBuffer(refImage);
  free(refImage);

  /* make sure the extent given in mapfile fits the image */
  cellsize = msAdjustExtent(&(map->reference.extent),
                            image->width, image->height);

  /* convert map extent to reference image coordinates */
  x1 = MS_MAP2IMAGE_X(map->extent.minx,  map->reference.extent.minx, cellsize);
  x2 = MS_MAP2IMAGE_X(map->extent.maxx,  map->reference.extent.minx, cellsize);
  y1 = MS_MAP2IMAGE_Y(map->extent.maxy,  map->reference.extent.maxy, cellsize);
  y2 = MS_MAP2IMAGE_Y(map->extent.miny,  map->reference.extent.maxy, cellsize);

  initStyle(&style);
  style.color = map->reference.color;
  style.outlinecolor = map->reference.outlinecolor;

  /* if extent are smaller than minbox size */
  /* draw that extent on the reference image */
  if( (abs(x2 - x1) > map->reference.minboxsize) ||
      (abs(y2 - y1) > map->reference.minboxsize) ) {
    shapeObj rect;
    lineObj line;
    pointObj points[5];
    msInitShape(&rect);

    line.point = points;
    line.numpoints = 5;
    rect.line = &line;
    rect.numlines = 1;
    rect.type = MS_SHAPE_POLYGON;

    line.point[0].x = x1;
    line.point[0].y = y1;
    line.point[1].x = x1;
    line.point[1].y = y2;
    line.point[2].x = x2;
    line.point[2].y = y2;
    line.point[3].x = x2;
    line.point[3].y = y1;
    line.point[4].x = line.point[0].x;
    line.point[4].y = line.point[0].y;

    line.numpoints = 5;

    if( map->reference.maxboxsize == 0 ||
        ((abs(x2 - x1) < map->reference.maxboxsize) &&
         (abs(y2 - y1) < map->reference.maxboxsize)) ) {
      msDrawShadeSymbol(&(map->symbolset), image, &rect, &style, 1.0);
    }

  } else { /* else draw the marker symbol */
    if( map->reference.maxboxsize == 0 ||
        ((abs(x2 - x1) < map->reference.maxboxsize) &&
         (abs(y2 - y1) < map->reference.maxboxsize)) ) {
      style.size = map->reference.markersize;

      /* if the marker symbol is specify draw this symbol else draw a cross */
      if(map->reference.marker != 0) {
        pointObj *point = NULL;

        point = msSmallMalloc(sizeof(pointObj));
        point->x = (double)(x1 + x2)/2;
        point->y = (double)(y1 + y2)/2;

        style.symbol = map->reference.marker;

        msDrawMarkerSymbol(&map->symbolset, image, point, &style, 1.0);
        free(point);
      } else if(map->reference.markername != NULL) {
        pointObj *point = NULL;

        point = msSmallMalloc(sizeof(pointObj));
        point->x = (double)(x1 + x2)/2;
        point->y = (double)(y1 + y2)/2;

        style.symbol = msGetSymbolIndex(&map->symbolset,  map->reference.markername, MS_TRUE);

        msDrawMarkerSymbol(&map->symbolset, image, point, &style, 1.0);
        free(point);
      } else {
        int x21, y21;
        shapeObj cross;
        lineObj lines[4];
        pointObj point[8];
        int i;
        /* determine the center point */
        x21 = MS_NINT((x1 + x2)/2);
        y21 = MS_NINT((y1 + y2)/2);

        msInitShape(&cross);
        cross.numlines = 4;
        cross.line = lines;
        for(i=0; i<4; i++) {
          cross.line[i].numpoints = 2;
          cross.line[i].point = &(point[2*i]);
        }
        /* draw a cross */
        cross.line[0].point[0].x = x21-8;
        cross.line[0].point[0].y = y21;
        cross.line[0].point[1].x = x21-3;
        cross.line[0].point[1].y = y21;
        cross.line[1].point[0].x = x21;
        cross.line[1].point[0].y = y21-8;
        cross.line[1].point[1].x = x21;
        cross.line[1].point[1].y = y21-3;
        cross.line[2].point[0].x = x21;
        cross.line[2].point[0].y = y21+3;
        cross.line[2].point[1].x = x21;
        cross.line[2].point[1].y = y21+8;
        cross.line[3].point[0].x = x21+3;
        cross.line[3].point[0].y = y21;
        cross.line[3].point[1].x = x21+8;
        cross.line[3].point[1].y = y21;

        msDrawLineSymbol(&(map->symbolset),image,&cross,&style,1.0);
      }
    }
  }

  return(image);
}
